/* Tranche 4 - Ring 3 ATA PIO driver (slices 1, 2 and 3).
 *
 * Registers "ata-driver". While it holds the controller claim
 * (SYS_ATA_CLAIM), the kernel opens ports 0x1F0-0x1F7 and 0x3F6 in the TSS I/O
 * bitmap for this task only, so the IN/OUT below execute at CPL 3; kernel PIO
 * is refused meanwhile. Two request sources:
 *  - kernel jobs (slice 2): the overlay snapshot (LBA 0-63) is written or
 *    loaded here, 8 sectors per chunk copied by SYS_ATA_JOB_FETCH/DONE;
 *  - kernel FAT sector jobs (slice 3): FAT16 (master) / FAT32 (slave) sector
 *    reads and writes of a task blocked in its syscall, served first;
 *  - "ata-client" IPC (slice 1): 64-byte sector windows, with client writes
 *    fenced off the overlay snapshot and the FAT areas.
 */
#include "os_syscalls.h"

static void putc(char c) { asm volatile("int $0x80" : : "a"(SYS_PUTC), "b"(c)); }
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
static void select_lba(uint8_t drive, uint32_t lba) {
    outb(ATA_DRIVE, (uint8_t)(0xE0 | (drive ? 0x10 : 0) | ((lba >> 24) & 0x0F)));
    delay();
    outb(ATA_SECCOUNT, 1);
    outb(ATA_LBA0, (uint8_t)lba); outb(ATA_LBA1, (uint8_t)(lba >> 8)); outb(ATA_LBA2, (uint8_t)(lba >> 16));
}
static int pio_read(uint8_t drive, uint32_t lba, uint8_t* out) {
    uint32_t i;
    if (wait_bsy() < 0) return -1;
    select_lba(drive, lba); outb(ATA_CMD, 0x20);
    if (wait_drq() < 0) return -1;
    for (i = 0; i < 256; i++) { uint16_t w = inw(ATA_DATA); out[2*i] = (uint8_t)w; out[2*i+1] = (uint8_t)(w >> 8); }
    return wait_bsy();
}
static int pio_write(uint8_t drive, uint32_t lba, const uint8_t* in) {
    uint32_t i;
    if (wait_bsy() < 0) return -1;
    select_lba(drive, lba); outb(ATA_CMD, 0x30);
    if (wait_drq() < 0) return -1;
    for (i = 0; i < 256; i++) outw(ATA_DATA, (uint16_t)(in[2*i] | (in[2*i+1] << 8)));
    outb(ATA_CMD, 0xE7); /* cache flush */
    return wait_bsy();
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
static int serve_job(void) {
    os_ata_job_t job;
    int32_t rc = 0;
    uint32_t s;
    int res;
    if (sc2(SYS_ATA_JOB_FETCH, (uint32_t)&job, (uint32_t)job_buf) != 1) return 0;
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
    for (s = 0; s < job.count && rc == 0; s++) {
        uint8_t drive = job.drive ? 1U : 0U;
        if (job.op == OS_ATA_JOB_WRITE || job.op == OS_ATA_JOB_IO_WRITE)
            rc = pio_write(drive, job.lba + s, job_buf + s * 512U);
        else rc = pio_read(drive, job.lba + s, job_buf + s * 512U);
    }
    release();
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
    for (;;) {
        int client, served = 0;
        int32_t rc = -1;
        uint32_t lba, off, len;
        uint8_t drive;
        while (serve_job() && served < 8) served++;
        if (served < 8) report_fat_io();
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
                if (rc == 0) rc = (int32_t)len;
            }
            release();
        }
        put32(r.data, (uint32_t)rc);
        (void)sc2(SYS_IPC_SEND, (uint32_t)m.sender_pid, (uint32_t)&r);
    }
}
