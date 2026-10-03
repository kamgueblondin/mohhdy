/* Tranche 4 - Ring 3 ATA PIO driver (slices 1, 2 and 3).
 *
 * Registers "ata-driver". While it holds the controller claim
 * (SYS_ATA_CLAIM), the kernel opens ports 0x1F0-0x1F7 and 0x3F6 in the TSS I/O
 * bitmap for this task only, so the IN/OUT below execute at CPL 3; kernel PIO
 * is refused meanwhile. Two request sources:
 *  - kernel jobs (slice 2): the overlay snapshot (LBA 0-63) is written or
 *    loaded here, up to OS_ATA_JOB_MAX_SECTORS sectors per chunk copied by
 *    SYS_ATA_JOB_FETCH/DONE;
 *  - kernel FAT sector jobs (slice 3): FAT16 (master) / FAT32 (slave) sector
 *    reads and writes of a task blocked in its syscall, served first;
 *  - "ata-client" IPC (slice 1): 64-byte sector windows, with client writes
 *    fenced off the overlay snapshot and the FAT areas;
 *  - Tranche 4 suite: whole FAT16/FAT32/overlay operations (OS_ATA_JOB_FS_OP).
 *    This task links its own copy of the FAT16/FAT32/overlay code, mounts
 *    the volumes through its own PIO, takes the overlay store over from the
 *    kernel (OS_ATA_FS_READY) and persists it itself (LBA 0-63, only the
 *    sectors that changed). The kernel only copies request/reply bytes and
 *    keeps the last published overlay image for the fallback.
 */
#include "os_syscalls.h"
#include "fs/fat16.h"
#include "fs/fat32.h"
#include "fs/fsop_exec.h"
#include "fs/overlay.h"

/* Lines go out in one syscall (OS_ATA_FS_LOG) once this task is the live
 * driver: a preempted driver no longer gets shell output cut into a counter
 * line. Before registration (or if refused) the bytes go out one by one. */
static char log_line[OS_ATA_FS_LOG_MAX];
static uint32_t log_length;
static void log_flush(void) {
    int rc;
    uint32_t i;
    if (log_length == 0U) return;
    asm volatile("int $0x80" : "=a"(rc) : "a"(SYS_ATA_FS), "b"(OS_ATA_FS_LOG), "c"(log_line), "d"(log_length));
    if (rc != 0)
        for (i = 0U; i < log_length; i++) asm volatile("int $0x80" : : "a"(SYS_PUTC), "b"(log_line[i]));
    log_length = 0U;
}
static void putc(char c) {
    log_line[log_length++] = c;
    if (c == '\n' || log_length == sizeof(log_line)) log_flush();
}
static void puts(const char* t) { int i = 0; while (t[i]) putc(t[i++]); }
static void putu(uint32_t v) {
    char b[11]; int n = 0;
    do { b[n++] = (char)('0' + v % 10U); v /= 10U; } while (v && n < 10);
    while (n) putc(b[--n]);
}
static void yield(void) { asm volatile("int $0x80" : : "a"(SYS_YIELD)); }

static int sc0(uint32_t n) { int r; asm volatile("int $0x80" : "=a"(r) : "a"(n)); return r; }
static int sc1(uint32_t n, uint32_t a) {
    int r; asm volatile("int $0x80" : "=a"(r) : "a"(n), "b"(a)); return r;
}
static int sc2(uint32_t n, uint32_t a, uint32_t b) {
    int r; asm volatile("int $0x80" : "=a"(r) : "a"(n), "b"(a), "c"(b)); return r;
}
static int sc3(uint32_t n, uint32_t a, uint32_t b, uint32_t c) {
    int r; asm volatile("int $0x80" : "=a"(r) : "a"(n), "b"(a), "c"(b), "d"(c)); return r;
}
static int sc4(uint32_t n, uint32_t a, uint32_t b, uint32_t c, uint32_t d) {
    int r; asm volatile("int $0x80" : "=a"(r) : "a"(n), "b"(a), "c"(b), "d"(c), "S"(d)); return r;
}
static int sc5(uint32_t n, uint32_t a, uint32_t b, uint32_t c, uint32_t d, uint32_t e) {
    int r; asm volatile("int $0x80" : "=a"(r) : "a"(n), "b"(a), "c"(b), "d"(c), "S"(d), "D"(e)); return r;
}

static inline uint8_t inb(uint16_t p) { uint8_t v; asm volatile("inb %1, %0" : "=a"(v) : "Nd"(p)); return v; }
static inline void outb(uint16_t p, uint8_t v) { asm volatile("outb %0, %1" : : "a"(v), "Nd"(p)); }
static inline uint16_t inw(uint16_t p) { uint16_t v; asm volatile("inw %1, %0" : "=a"(v) : "Nd"(p)); return v; }
static inline void outw(uint16_t p, uint16_t v) { asm volatile("outw %0, %1" : : "a"(v), "Nd"(p)); }

#define ATA_DATA 0x1F0
#define ATA_SECCOUNT 0x1F2
#define ATA_LBA0 0x1F3
#define ATA_LBA1 0x1F4
#define ATA_LBA2 0x1F5
#define ATA_DRIVE 0x1F6
#define ATA_CMD 0x1F7
#define ATA_ALT 0x3F6
#define ATA_TIMEOUT 500000U

static uint8_t sector[512];
static uint8_t job_buf[OS_ATA_JOB_MAX_SECTORS * 512U];
/* Slice 3 driver-side counters of FAT sectors moved by this task's PIO. */
static uint32_t fat_rd, fat_wr, fat_reported_rd, fat_reported_wr;

static void delay(void) { (void)inb(ATA_ALT); (void)inb(ATA_ALT); (void)inb(ATA_ALT); (void)inb(ATA_ALT); }

static int wait_bsy(void) {
    uint32_t i;
    for (i = 0; i < ATA_TIMEOUT; i++) { uint8_t s = inb(ATA_CMD); if (s == 0xFF) return -1; if (!(s & 0x80)) return 0; }
    return -1;
}
static int wait_drq(void) {
    uint32_t i;
    for (i = 0; i < ATA_TIMEOUT; i++) {
        uint8_t s = inb(ATA_CMD);
        if (s == 0xFF || (s & 0x21)) return -1;
        if (!(s & 0x80) && (s & 0x08)) return 0;
    }
    return -1;
}
/* A write or cache flush can keep BSY for a long time when the host disk is
 * slow (QEMU completes it with a host write/fsync; seen on CI runners), far
 * beyond one bounded spin. Keep waiting across cooperative turns instead of
 * failing the chunk: up to ATA_PATIENT_ROUNDS spins, yielding in between. */
#define ATA_PATIENT_ROUNDS 256U
static int wait_bsy_patient(void) {
    uint32_t r;
    for (r = 0; r < ATA_PATIENT_ROUNDS; r++) {
        if (wait_bsy() == 0) return 0;
        if (inb(ATA_CMD) == 0xFF) return -1;
        yield();
    }
    return -1;
}
static int wait_drq_patient(void) {
    uint32_t r;
    for (r = 0; r < ATA_PATIENT_ROUNDS; r++) {
        uint8_t s;
        if (wait_drq() == 0) return 0;
        s = inb(ATA_CMD);
        if (s == 0xFF || (s & 0x21)) return -1; /* error, not slowness */
        yield();
    }
    return -1;
}
static void select_lba_count(uint8_t drive, uint32_t lba, uint8_t count) {
    outb(ATA_DRIVE, (uint8_t)(0xE0 | (drive ? 0x10 : 0) | ((lba >> 24) & 0x0F)));
    delay();
    outb(ATA_SECCOUNT, count);
    outb(ATA_LBA0, (uint8_t)lba); outb(ATA_LBA1, (uint8_t)(lba >> 8)); outb(ATA_LBA2, (uint8_t)(lba >> 16));
}
static void select_lba(uint8_t drive, uint32_t lba) { select_lba_count(drive, lba, 1U); }
/* rep insw/outsw : QEMU traite la chaine en un seul exit, au lieu d'un exit par mot. */
static void pio_insw(uint8_t* out, uint32_t words) {
    asm volatile("cld; rep insw" : "+D"(out), "+c"(words) : "d"(ATA_DATA) : "memory");
}
static void pio_outsw(const uint8_t* in, uint32_t words) {
    asm volatile("cld; rep outsw" : "+S"(in), "+c"(words) : "d"(ATA_DATA) : "memory");
}
/* Une commande READ/WRITE SECTORS pour tout le bloc contigu (1..255). */
static int pio_transfer(uint8_t drive, uint32_t lba, uint8_t count, uint8_t* buf, int write) {
    uint32_t s;
    if (count == 0U) return -1;
    if (wait_bsy_patient() < 0) return -1;
    select_lba_count(drive, lba, count);
    outb(ATA_CMD, write ? 0x30 : 0x20);
    for (s = 0; s < count; s++) {
        if (wait_drq_patient() < 0) return -1;
        if (write) pio_outsw(buf + s * 512U, 256U);
        else pio_insw(buf + s * 512U, 256U);
    }
    return wait_bsy_patient();
}
static int pio_read(uint8_t drive, uint32_t lba, uint8_t* out) {
    return pio_transfer(drive, lba, 1U, out, 0);
}
static int pio_write(uint8_t drive, uint32_t lba, const uint8_t* in) {
    return pio_transfer(drive, lba, 1U, (uint8_t*)in, 1);
}
/* One cache flush per write job (was one per sector). */
static int pio_flush(void) {
    outb(ATA_CMD, 0xE7);
    return wait_bsy_patient();
}

/* Controller claim: ports are open only between claim and release. */
static void claim(void) { while (sc0(SYS_ATA_CLAIM) != 0) yield(); }
static void release(void) { (void)sc0(SYS_ATA_RELEASE); }

static void put32(uint8_t* p, uint32_t v) { p[0]=(uint8_t)v; p[1]=(uint8_t)(v>>8); p[2]=(uint8_t)(v>>16); p[3]=(uint8_t)(v>>24); }
static uint32_t get32(const uint8_t* p) { return p[0] | (p[1]<<8) | ((uint32_t)p[2]<<16) | ((uint32_t)p[3]<<24); }

static void print_counters(void) {
    os_ata_status_t st;
    if (sc1(SYS_ATA_STATUS, (uint32_t)&st) != 0) return;
    puts(" flushes="); putu(st.flush_done);
    puts(" loads="); putu(st.load_done);
    puts(" kpio="); putu(st.kernel_overlay_writes);
}

/* Slice 3: one line per burst of FAT jobs, printed once the queue is idle.
 * rd/wr are this driver's own counters; kfat is the kernel count of FAT
 * sectors moved by Ring 0 PIO while a driver was live (expected 0). */
static void report_fat_io(void) {
    os_ata_status_t st;
    if (fat_rd == fat_reported_rd && fat_wr == fat_reported_wr) return;
    fat_reported_rd = fat_rd;
    fat_reported_wr = fat_wr;
    if (sc1(SYS_ATA_STATUS, (uint32_t)&st) != 0) return;
    puts("atadriver fat io rd="); putu(fat_rd);
    puts(" wr="); putu(fat_wr);
    puts(" kfat="); putu(st.fat_kernel_pio_live);
    puts("\n");
}

/* One kernel job chunk. Returns 1 if a chunk was served. */
static void serve_fs_op(const os_ata_job_t* job);
static void ov_disk_note(uint32_t lba, uint32_t count, const uint8_t* data);

static int serve_job(void) {
    os_ata_job_t job;
    int32_t rc = 0;
    int res;
    if (sc2(SYS_ATA_JOB_FETCH, (uint32_t)&job, (uint32_t)job_buf) != 1) return 0;
    if (job.op == OS_ATA_JOB_FS_OP) {
        serve_fs_op(&job);
        return 1;
    }
    claim();
    if ((job.flags & OS_ATA_JOB_FLAG_DEBUG_CRASH) && job.op == OS_ATA_JOB_IO_WRITE) {
        /* Test hook (armed by the root shell via SYS_ATA_DEBUG): start the
         * sector write, push half of the data, then crash while holding the
         * claim with the transfer open (raw IN on a port outside the IOPB
         * raises #GP; the kernel kills this task). */
        uint32_t w;
        puts("atadriver debug crash mid-job\n");
        if (wait_bsy() == 0) {
            select_lba(job.drive ? 1U : 0U, job.lba);
            outb(ATA_CMD, 0x30);
            if (wait_drq() == 0)
                for (w = 0; w < 128U; w++)
                    outw(ATA_DATA, (uint16_t)(job_buf[2*w] | (job_buf[2*w+1] << 8)));
        }
        (void)inb(0x60);
        for (;;) yield();
    }
    if (job.count == 0U || job.count > 255U) rc = -1;
    else if (job.op == OS_ATA_JOB_WRITE || job.op == OS_ATA_JOB_IO_WRITE)
        rc = pio_transfer(job.drive ? 1U : 0U, job.lba, (uint8_t)job.count, job_buf, 1);
    else rc = pio_transfer(job.drive ? 1U : 0U, job.lba, (uint8_t)job.count, job_buf, 0);
    if (rc == 0 && (job.op == OS_ATA_JOB_WRITE || job.op == OS_ATA_JOB_IO_WRITE))
        rc = pio_flush();
    release();
    if (rc == 0 && job.drive == 0U && (job.op == OS_ATA_JOB_WRITE || job.op == OS_ATA_JOB_READ))
        ov_disk_note(job.lba, job.count, job_buf);
    res = sc3(SYS_ATA_JOB_DONE, (uint32_t)&job, (uint32_t)rc, (uint32_t)job_buf);
    if (res == OS_ATA_JOB_IO_DONE) {
        if (job.op == OS_ATA_JOB_IO_WRITE) fat_wr += job.count; else fat_rd += job.count;
    } else if (res == OS_ATA_JOB_FLUSH_DONE) {
        puts("atadriver snapshot flush ok gen="); putu(job.generation); print_counters(); puts("\n");
    } else if (res == OS_ATA_JOB_LOAD_DONE) {
        puts("atadriver snapshot load ok gen="); putu(job.generation); print_counters(); puts("\n");
    } else if (res == OS_ATA_JOB_LOAD_SKIPPED) {
        puts("atadriver snapshot load skipped\n");
    } else if (res == OS_ATA_JOB_FAILED) {
        puts("atadriver snapshot chunk failed\n");
    }
    return 1;
}

/* ========================================================================
 * Tranche 4 suite: the FS store served by this task's own code.
 * ======================================================================== */
#define OV_DISK_SECTORS_DRV 64U
static fat16_volume_t v16;
static fat32_volume_t v32;
static uint8_t v16_window[16U * 512U];
static int v16_ok, v32_ok, disk_ok;
static uint32_t store_flags;   /* flags the kernel accepted */
static int store_ready;
static uint8_t fs_req[OS_ATA_FSOP_BUFFER_SIZE];
static uint8_t fs_reply[OS_ATA_FSOP_BUFFER_SIZE];
static uint8_t ov_img[OV_DISK_SECTORS_DRV * 512U];  /* snapshot to write / handover */
static uint8_t ov_disk[OV_DISK_SECTORS_DRV * 512U]; /* what LBA 0-63 hold */
static int ov_disk_known;
/* Per-op state. */
static uint32_t op_gen, op_rd, op_wr;
static int op_noted, op_crash, ov_dirty;

static void ov_disk_note(uint32_t lba, uint32_t count, const uint8_t* data) {
    uint32_t i;
    if (!ov_disk_known || lba >= OV_DISK_SECTORS_DRV) return;
    if (lba + count > OV_DISK_SECTORS_DRV) count = OV_DISK_SECTORS_DRV - lba;
    for (i = 0; i < count * 512U; i++) ov_disk[lba * 512U + i] = data[i];
}

/* Overlay hooks: the store only marks itself dirty; serve_fs_op persists
 * once per op, after the op result is known. */
static int drv_overlay_redirect(void) { ov_dirty = 1; return 1; }
int ata_present(void) { return disk_ok; }
int ata_read_sectors(uint32_t lba, uint32_t count, void* buf) { (void)lba; (void)count; (void)buf; return -1; }
int ata_write_sectors(uint32_t lba, uint32_t count, const void* buf) { (void)lba; (void)count; (void)buf; return -1; }
/* Initrd lives in kernel RAM: read-only probes for the overlay rules. */
int initrd_is_file(const char* path) { int r = sc2(SYS_ATA_FS, OS_ATA_FS_INITRD_STAT, (uint32_t)path); return r > 0 && (r & OS_ATA_FS_INITRD_FILE); }
int initrd_is_dir(const char* path) { int r = sc2(SYS_ATA_FS, OS_ATA_FS_INITRD_STAT, (uint32_t)path); return r > 0 && (r & OS_ATA_FS_INITRD_DIR); }
int initrd_read_into(const char* path, char* buf, uint32_t max) {
    return sc4(SYS_ATA_FS, OS_ATA_FS_INITRD_READ, (uint32_t)path, (uint32_t)buf, max);
}

/* FAT sector callbacks (this task's PIO; claim held for the whole op). */
static int drv_read(uint8_t drive, uint32_t lba, void* buf) {
    int rc = pio_read(drive, lba, (uint8_t*)buf);
    if (rc == 0) op_rd++;
    return rc;
}
static int drv_write(uint8_t drive, uint32_t lba, const void* buf) {
    const uint8_t* in = (const uint8_t*)buf;
    int rc;
    if (op_crash) {
        /* Test hook (SYS_ATA_DEBUG, root shell): start the first sector write
         * of this FAT mutation, push half of it and fault holding the claim
         * with the transfer open. Nothing of the op is committed. */
        uint32_t w;
        puts("atadriver debug crash mid-job\n");
        if (wait_bsy() == 0) {
            select_lba(drive, lba);
            outb(ATA_CMD, 0x30);
            if (wait_drq() == 0)
                for (w = 0; w < 128U; w++) outw(ATA_DATA, (uint16_t)(in[2*w] | (in[2*w+1] << 8)));
        }
        (void)inb(0x60);
        for (;;) yield();
    }
    rc = pio_write(drive, lba, in);
    if (rc == 0) {
        op_wr++;
        if (!op_noted) {
            /* First completed sector: from now on the op is not replayable. */
            (void)sc3(SYS_ATA_FS, OS_ATA_FS_NOTE, op_gen, OS_ATA_FS_NOTE_SECTOR_WRITTEN);
            op_noted = 1;
        }
    }
    return rc;
}
static int v16_read(uint32_t lba, void* buf) { return drv_read(0, lba, buf); }
static int v16_reads(uint32_t lba, uint32_t count, void* buf) {
    uint32_t i;
    for (i = 0; i < count; i++) if (drv_read(0, lba + i, (uint8_t*)buf + i * 512U) != 0) return -1;
    return 0;
}
static int v16_write(uint32_t lba, const void* buf) { return drv_write(0, lba, buf); }
static int v32_read(uint32_t lba, void* buf) { return drv_read(1, lba, buf); }
static int v32_write(uint32_t lba, const void* buf) { return drv_write(1, lba, buf); }

static void fs_mount(void) {
    uint8_t st;
    os_ata_status_t ks;
    int want16 = 0, want32 = 0;
    /* Mount only what the kernel mounted at boot (it probed the drives):
     * FAT16 fences the master past LBA 64, FAT32 locks the slave. Probing a
     * missing slave from here would spin on its status register. */
    if (sc1(SYS_ATA_STATUS, (uint32_t)&ks) == 0) {
        want16 = ks.client_min_lba > OS_ATA_KERNEL_RESERVED_LBAS;
        want32 = ks.slave_write_locked != 0U;
    }
    claim();
    st = inb(ATA_CMD);
    disk_ok = !(st == 0xFF || st == 0x00);
    if (disk_ok && want16 && fat16_mount(&v16, v16_read, 64U) == 0 &&
        fat16_attach_read_window(&v16, v16_reads, v16_window, sizeof(v16_window)) == 0 &&
        fat16_attach_writer(&v16, v16_write) == 0) v16_ok = 1;
    if (disk_ok && want32 && fat32_mount(&v32, v32_read, 0U) == 0 &&
        fat32_attach_writer(&v32, v32_write) == 0) v32_ok = 1;
    release();
    op_rd = 0; /* mount reads are not FS op traffic */
    overlay_init();
    overlay_set_disk_hooks(drv_overlay_redirect, 0);
}

static int mem_eq(const uint8_t* a, const uint8_t* b, uint32_t n) {
    uint32_t i;
    for (i = 0; i < n; i++) if (a[i] != b[i]) return 0;
    return 1;
}

/* Handover: the kernel serialises its overlay store into ov_img in the same
 * syscall that makes this task the authority. Then read LBA 0-63 once as
 * the base for incremental writes. */
static void fs_try_ready(void) {
    uint32_t want = 0U, i;
    int rc;
    os_ata_status_t st;
    if (store_ready || !disk_ok) return;
    if (v16_ok) want |= OS_ATA_FS_STORE_FAT16;
    if (v32_ok) want |= OS_ATA_FS_STORE_FAT32;
    want |= OS_ATA_FS_STORE_OVERLAY;
    rc = sc4(SYS_ATA_FS, OS_ATA_FS_READY, want, (uint32_t)ov_img, sizeof(ov_img));
    if (rc == OS_ATA_FS_BUSY) return; /* a slice 2 job first, retry */
    if (rc < 0) {
        store_ready = -1;
        puts("atadriver store refused\n");
        return;
    }
    store_ready = 1;
    if (overlay_restore(ov_img, (uint32_t)rc) != 0) overlay_init();
    claim();
    ov_disk_known = 1;
    for (i = 0; i < OV_DISK_SECTORS_DRV && ov_disk_known; i++)
        if (pio_read(0, i, ov_disk + i * 512U) != 0) ov_disk_known = 0;
    release();
    store_flags = want;
    if (sc2(SYS_ATA_FS, OS_ATA_FS_STATUS, (uint32_t)&st) == 0) store_flags = st.fs_store_flags;
    puts("atadriver store ready flags="); putu(store_flags);
    puts(" fat16="); putu((store_flags & OS_ATA_FS_STORE_FAT16) ? 1U : 0U);
    puts(" fat32="); putu((store_flags & OS_ATA_FS_STORE_FAT32) ? 1U : 0U);
    puts(" overlay="); putu((store_flags & OS_ATA_FS_STORE_OVERLAY) ? 1U : 0U);
    puts(" image="); putu((uint32_t)rc);
    puts("\n");
}

/* Overlay persistence after a mutating op: publish the image to the kernel
 * mirror first (a torn disk write is then repaired by the Ring 0 fallback),
 * then write only the sectors that differ from what LBA 0-63 hold. */
static void fs_persist(int32_t result) {
    uint32_t size = 0U, i, s, wrote = 0U;
    int rc = 0;
    if (overlay_snapshot(ov_img, sizeof(ov_img), &size) != 0) {
        (void)sc3(SYS_ATA_FS, OS_ATA_FS_NOTE, op_gen, OS_ATA_FS_NOTE_PERSIST_FAILED);
        puts("atadriver snapshot build failed\n");
        return;
    }
    for (i = size; i < sizeof(ov_img); i++) ov_img[i] = 0;
    (void)sc5(SYS_ATA_FS, OS_ATA_FS_PUBLISH, op_gen, (uint32_t)ov_img, size, (uint32_t)result);
    for (s = 0; s < OV_DISK_SECTORS_DRV && rc == 0; s++) {
        if (ov_disk_known && mem_eq(ov_img + s * 512U, ov_disk + s * 512U, 512U)) continue;
        rc = pio_write(0, s, ov_img + s * 512U);
        if (rc == 0) {
            for (i = 0; i < 512U; i++) ov_disk[s * 512U + i] = ov_img[s * 512U + i];
            wrote++;
        }
    }
    if (rc == 0 && wrote) rc = pio_flush();
    if (rc != 0) {
        ov_disk_known = 0; /* next persist rewrites everything */
        (void)sc3(SYS_ATA_FS, OS_ATA_FS_NOTE, op_gen, OS_ATA_FS_NOTE_PERSIST_FAILED);
        puts("atadriver snapshot write failed\n");
        return;
    }
    ov_disk_known = 1;
    (void)sc3(SYS_ATA_FS, OS_ATA_FS_NOTE, op_gen, OS_ATA_FS_NOTE_PERSISTED);
    puts("atadriver snapshot flush ok gen="); putu(op_gen); print_counters();
    puts(" sectors="); putu(wrote); puts("\n");
}

static void serve_fs_op(const os_ata_job_t* job) {
    os_ata_fsop_request_t req;
    os_ata_fsop_reply_t* reply = (os_ata_fsop_reply_t*)fs_reply;
    const char* path;
    const char* path2;
    const uint8_t* in;
    uint32_t out_len = 0U, store;
    int32_t result = -1;
    int n;
    n = sc4(SYS_ATA_FS, OS_ATA_FS_REQUEST, (uint32_t)job, (uint32_t)fs_req, sizeof(fs_req));
    if (n < 0) return; /* stale: the kernel gave up on it */
    op_gen = job->generation;
    op_rd = op_wr = 0U;
    op_noted = 0;
    ov_dirty = 0;
    op_crash = (job->flags & OS_ATA_JOB_FLAG_DEBUG_CRASH) ? 1 : 0;
    if (fsop_decode(fs_req, (uint32_t)n, &req, &path, &path2, &in) == 0) {
        store = fsop_store_for(req.op);
        if (store_ready != 1 || (store_flags & store) == 0U) {
            result = OS_FAT16_NOT_MOUNTED;
        } else {
            claim();
            result = fsop_execute(&req, path, path2, in, fs_reply + sizeof(*reply),
                                  sizeof(fs_reply) - sizeof(*reply), &out_len,
                                  v16_ok ? &v16 : 0, v32_ok ? &v32 : 0);
            if (op_wr) (void)pio_flush();
            if (ov_dirty) fs_persist(result);
            release();
        }
    }
    reply->result = result;
    reply->out_len = out_len;
    reply->sectors_read = op_rd;
    reply->sectors_written = op_wr;
    fat_rd += op_rd;
    fat_wr += op_wr;
    (void)sc4(SYS_ATA_FS, OS_ATA_FS_DONE, (uint32_t)job, (uint32_t)fs_reply,
              (uint32_t)sizeof(*reply) + out_len);
}

void main(void) {
    os_ipc_message_t m;
    os_ipc_payload_t r;
    os_ata_status_t st;
    uint8_t status;
    uint32_t i;
    if (sc1(SYS_SERVICE_REGISTER, (uint32_t)"ata-driver") != 0) {
        puts("atadriver register failed\n");
        for (;;) yield();
    }
    claim();
    status = inb(ATA_CMD);
    release();
    if (status == 0xFF || status == 0x00) puts("atadriver ring3 pio no-disk");
    else puts("atadriver ring3 pio ready");
    print_counters();
    puts("\n");
    fs_mount();
    for (;;) {
        int client, served = 0;
        int32_t rc = -1;
        uint32_t lba, off, len;
        uint8_t drive;
        while (serve_job() && served < 8) served++;
        if (served < 8) report_fat_io();
        if (served < 8) fs_try_ready();
        if (sc1(SYS_IPC_RECV, (uint32_t)&m) != 0) { yield(); continue; }
        if (m.type != OS_IPC_ATA_READ && m.type != OS_IPC_ATA_WRITE) continue;
        for (i = 0; i < sizeof(r); i++) ((uint8_t*)&r)[i] = 0;
        r.type = OS_IPC_ATA_REPLY; r.request_id = m.request_id; r.size = 4U;
        client = sc1(SYS_SERVICE_LOOKUP, (uint32_t)OS_ATA_IPC_CLIENT_SERVICE);
        lba = get32(m.data); drive = m.data[4];
        off = (uint32_t)m.data[5] * 16U; len = m.data[6];
        if (sc1(SYS_ATA_STATUS, (uint32_t)&st) != 0) { st.client_min_lba = 0xFFFFFFFFU; st.slave_write_locked = 1U; }
        if (client <= 0 || client != m.sender_pid) {
            rc = OS_ATA_DRIVER_REQUIRED;
            puts("atadriver sector ipc refused\n");
        } else if (drive > 1 || len == 0 || len > OS_ATA_IPC_WINDOW || off + len > 512U) {
            rc = -1;
        } else if (m.type == OS_IPC_ATA_WRITE && drive == 0 && lba < OS_ATA_KERNEL_RESERVED_LBAS) {
            /* LBA 0-63 hold the overlay snapshot, written only via kernel jobs. */
            rc = OS_ATA_DRIVER_REQUIRED;
            puts("atadriver overlay region write refused\n");
        } else if (m.type == OS_IPC_ATA_WRITE &&
                   ((drive == 0 && lba < st.client_min_lba) || (drive == 1 && st.slave_write_locked))) {
            /* FAT16 volume on the master / FAT32 slave stay kernel-owned. */
            rc = OS_ATA_DRIVER_REQUIRED;
            puts("atadriver fat region write refused\n");
        } else if (m.type == OS_IPC_ATA_READ) {
            claim();
            rc = pio_read(drive, lba, sector);
            release();
            if (rc == 0) { for (i = 0; i < len; i++) r.data[4 + i] = sector[off + i]; r.size = 4U + len; rc = (int32_t)len; }
        } else {
            claim();
            rc = pio_read(drive, lba, sector);
            if (rc == 0) {
                for (i = 0; i < len; i++) sector[off + i] = m.data[8 + i];
                rc = pio_write(drive, lba, sector);
                if (rc == 0) rc = pio_flush();
                if (rc == 0) rc = (int32_t)len;
            }
            release();
        }
        put32(r.data, (uint32_t)rc);
        (void)sc2(SYS_IPC_SEND, (uint32_t)m.sender_pid, (uint32_t)&r);
    }
}
