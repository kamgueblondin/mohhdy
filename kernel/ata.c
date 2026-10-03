#include "ata.h"
#include "os_syscalls.h"

extern unsigned char inb(unsigned short port);
extern void outb(unsigned short port, unsigned char data);

#define ATA_DATA     0x1F0
#define ATA_ERROR    0x1F1
#define ATA_SECCOUNT 0x1F2
#define ATA_LBA0     0x1F3
#define ATA_LBA1     0x1F4
#define ATA_LBA2     0x1F5
#define ATA_DRIVE    0x1F6
#define ATA_CMD      0x1F7
#define ATA_STATUS   0x1F7
#define ATA_ALTSTAT  0x3F6

#define ATA_SR_ERR  0x01
#define ATA_SR_DRQ  0x08
#define ATA_SR_DF   0x20
#define ATA_SR_BSY  0x80

#define ATA_CMD_READ_PIO  0x20
#define ATA_CMD_WRITE_PIO 0x30
#define ATA_CMD_IDENTIFY  0xEC

/* Status polls per wait. Each poll is a port read trapped by the
 * hypervisor; with cache=writethrough a sector write includes a host
 * fdatasync, whose latency spikes on shared CI disks. 4x the historical
 * bound: only a failing wait gets longer. */
#define ATA_TIMEOUT 2000000u

static int g_ata_present;
static uint8_t g_ata_drive_present[2];

#ifndef ATA_PIO_HOST_HOOKS
static unsigned short ata_inw(unsigned short port) {
    unsigned short ret;
    asm volatile ("inw %1, %0" : "=a"(ret) : "dN"(port));
    return ret;
}
#else
unsigned short ata_host_inw(unsigned short port);
#define ata_inw ata_host_inw
#endif

#ifndef ATA_PIO_HOST_HOOKS
/* Les transferts PIO d'un secteur sont contigus : rep insw/outsw evite une
 * boucle C de 256 acces port et ne requiert aucun buffer intermediaire. */
static void ata_insw(void* out, uint32_t words) {
    asm volatile ("cld; rep insw" : "+D"(out), "+c"(words) : "d"(ATA_DATA) : "memory");
}

static void ata_outsw(const void* in, uint32_t words) {
    asm volatile ("cld; rep outsw" : "+S"(in), "+c"(words) : "d"(ATA_DATA) : "memory");
}
#else
/* Unit tests (tests/unit/kernel/test_ata_pio.c) model the IDE device. */
void ata_host_insw(void* out, uint32_t words);
void ata_host_outsw(const void* in, uint32_t words);
#define ata_insw ata_host_insw
#define ata_outsw ata_host_outsw
#endif

static void ata_io_delay(void) {
    (void)inb(ATA_ALTSTAT);
    (void)inb(ATA_ALTSTAT);
    (void)inb(ATA_ALTSTAT);
    (void)inb(ATA_ALTSTAT);
}

static int ata_wait_not_busy(void) {
    uint32_t i;
    for (i = 0; i < ATA_TIMEOUT; i++) {
        uint8_t st = inb(ATA_STATUS);
        if (st == 0xFF) return -1;
        if ((st & ATA_SR_BSY) == 0) return (int)st;
    }
    return -1;
}

static int ata_wait_drq(void) {
    uint32_t i;
    for (i = 0; i < ATA_TIMEOUT; i++) {
        uint8_t st = inb(ATA_STATUS);
        if (st == 0xFF) return -1;
        if (st & (ATA_SR_ERR | ATA_SR_DF)) return -1;
        if ((st & ATA_SR_BSY) == 0 && (st & ATA_SR_DRQ)) return 0;
    }
    return -1;
}

static void ata_select_lba(uint8_t drive, uint32_t lba, uint8_t count) {
    outb(ATA_DRIVE, (uint8_t)(0xE0 | (drive ? 0x10U : 0U) | ((lba >> 24) & 0x0F)));
    ata_io_delay();
    outb(ATA_SECCOUNT, count);
    outb(ATA_LBA0, (uint8_t)lba);
    outb(ATA_LBA1, (uint8_t)(lba >> 8));
    outb(ATA_LBA2, (uint8_t)(lba >> 16));
}

int ata_present(void) { return g_ata_present; }

/* Tranche 4: a Ring 3 driver killed in the middle of a PIO transfer leaves
 * the channel with DRQ set; the device then ignores new commands (QEMU and
 * real drives alike). Software reset (SRST in the device control register)
 * aborts it before the Ring 0 fallback touches the controller. */
int ata_channel_reset(void) {
    uint32_t i;
    if (!g_ata_present) return -1;
    outb(ATA_ALTSTAT, 0x04); /* SRST */
    for (i = 0; i < 16U; i++) ata_io_delay();
    outb(ATA_ALTSTAT, 0x00); /* back to the power-on value (never changed elsewhere) */
    for (i = 0; i < 16U; i++) ata_io_delay();
    return ata_wait_not_busy() < 0 ? -1 : 0;
}

int ata_present_drive(uint8_t drive) {
    return drive < 2U && g_ata_drive_present[drive];
}

int ata_init(void) {
    int st;
    uint32_t i;

    g_ata_present = 0;
    g_ata_drive_present[0] = 0U; g_ata_drive_present[1] = 0U;

    outb(ATA_DRIVE, 0xA0);
    ata_io_delay();
    st = inb(ATA_STATUS);
    /* No controller / no drive: fail immediately so QEMU without -hda does not hang. */
    if (st == 0x00 || st == 0xFF) return -1;

    outb(ATA_SECCOUNT, 0);
    outb(ATA_LBA0, 0);
    outb(ATA_LBA1, 0);
    outb(ATA_LBA2, 0);
    outb(ATA_CMD, ATA_CMD_IDENTIFY);
    ata_io_delay();

    st = inb(ATA_STATUS);
    if (st == 0x00 || st == 0xFF) return -1;
    if (ata_wait_not_busy() < 0) return -1;
    if (ata_wait_drq() < 0) return -1;

    for (i = 0; i < 256; i++) {
        (void)ata_inw(ATA_DATA);
    }

    g_ata_present = 1;
    g_ata_drive_present[0] = 1U;
    outb(ATA_DRIVE, 0xB0); ata_io_delay();
    st = inb(ATA_STATUS);
    if (st != 0x00 && st != 0xFF) g_ata_drive_present[1] = 1U;
    return 0;
}

/* IRQ0 peut planifier une autre tâche au milieu d’un transfert PIO : le
 * contrôleur ATA primaire n’accepte qu’une commande à la fois. */
#ifndef ATA_PIO_HOST_HOOKS
static uint32_t ata_irq_save(void) {
    uint32_t flags;
    asm volatile("pushfl; popl %0; cli" : "=r"(flags) :: "memory");
    return flags;
}

static void ata_irq_restore(uint32_t flags) {
    if ((flags & (1U << 9)) != 0U) asm volatile("sti" ::: "memory");
}
#else
static uint32_t ata_irq_save(void) { return 0U; }
static void ata_irq_restore(uint32_t flags) { (void)flags; }
#endif

/* Tranche 4 slice 2: exclusion with the Ring 3 atadriver. The gate returns 0
 * while the live driver holds the controller; kernel PIO then refuses instead
 * of interleaving commands with the driver. */
static int (*g_ata_kernel_gate)(void);

void ata_set_kernel_gate(int (*gate)(void)) { g_ata_kernel_gate = gate; }

/* Recovery of a channel left in the middle of a transfer.
 *
 * QEMU (like real drives) ignores a new command while BSY or DRQ is set.
 * If a previous transfer stopped early (a kernel wait timed out on a slow
 * host disk, or a Ring 3 driver died between sectors without the claim
 * being seen), the next kernel command was silently dropped: its data was
 * pushed into the stale transfer at the stale LBA, and a READ returned
 * whatever the data register held. Seen on CI (qemu-ata-driver, run
 * 37095701787): the FAT16 fallback write of t4fb.txt landed at LBA 6-9,
 * inside the overlay snapshot, and read back as zeros.
 *
 * Every kernel command now starts on an idle channel (software reset if a
 * transfer is still pending), and a transfer that fails half way resets
 * the channel before returning, then is retried once from the start. A
 * whole-command retry is safe: same LBA, same sectors, same data. */
#define ATA_PIO_ATTEMPTS 2U
static uint32_t g_ata_pio_recoveries;

uint32_t ata_pio_recoveries(void) { return g_ata_pio_recoveries; }

static int ata_channel_idle(void) {
    uint8_t st = inb(ATA_STATUS);
    if (st == 0xFF) return -1;
    if ((st & (ATA_SR_BSY | ATA_SR_DRQ)) == 0U) return 0;
    /* A slow but finishing command clears BSY; DRQ alone means a stale
     * transfer that will never complete on its own. */
    if ((st & ATA_SR_DRQ) == 0U) {
        int w = ata_wait_not_busy();
        if (w >= 0 && ((uint8_t)w & ATA_SR_DRQ) == 0U) return 0;
    }
    g_ata_pio_recoveries++;
    if (ata_channel_reset() != 0) return -1;
    st = inb(ATA_STATUS);
    return (st & (ATA_SR_BSY | ATA_SR_DRQ)) == 0U ? 0 : -1;
}

static int ata_pio_once(uint8_t drive, uint32_t lba, uint32_t count, uint8_t* buf, int write) {
    uint32_t s;
    if (ata_channel_idle() != 0) return -1;
    ata_select_lba(drive, lba, (uint8_t)count);
    outb(ATA_CMD, write ? ATA_CMD_WRITE_PIO : ATA_CMD_READ_PIO);
    for (s = 0; s < count; s++) {
        if (ata_wait_drq() < 0) goto abort;
        if (write) ata_outsw(buf + s * 512U, 256U);
        else ata_insw(buf + s * 512U, 256U);
        if (ata_wait_not_busy() < 0) goto abort;
    }
    return 0;
abort:
    /* Never leave the device expecting or holding data. */
    g_ata_pio_recoveries++;
    (void)ata_channel_reset();
    return -1;
}

static int ata_pio(uint8_t drive, uint32_t lba, uint32_t count, uint8_t* buf, int write) {
    uint32_t attempt;
    uint32_t flags;
    int rc = -1;

    if (drive > ATA_DRIVE_SLAVE || !g_ata_drive_present[drive] || !buf || count == 0 || count > 256) return -1;
    if (lba + count < lba) return -1;
    if (g_ata_kernel_gate && !g_ata_kernel_gate()) return OS_ATA_CONTROLLER_BUSY;

    flags = ata_irq_save();
    for (attempt = 0U; attempt < ATA_PIO_ATTEMPTS && rc != 0; attempt++) {
        rc = ata_pio_once(drive, lba, count, buf, write);
    }
    ata_irq_restore(flags);
    return rc;
}

int ata_read_sectors_drive(uint8_t drive, uint32_t lba, uint32_t count, void* buf) {
    return ata_pio(drive, lba, count, (uint8_t*)buf, 0);
}

int ata_write_sectors_drive(uint8_t drive, uint32_t lba, uint32_t count, const void* buf) {
    return ata_pio(drive, lba, count, (uint8_t*)buf, 1);
}

int ata_read_sectors(uint32_t lba, uint32_t count, void* buf) {
    return ata_read_sectors_drive(ATA_DRIVE_MASTER, lba, count, buf);
}

int ata_write_sectors(uint32_t lba, uint32_t count, const void* buf) {
    return ata_write_sectors_drive(ATA_DRIVE_MASTER, lba, count, buf);
}
