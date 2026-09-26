/* Tranche 4 - Ring 3 ATA PIO driver (first real slice).
 *
 * Registers "ata-driver". The kernel then opens ports 0x1F0-0x1F7 and 0x3F6 in
 * the TSS I/O bitmap for this task only, so the IN/OUT below execute at CPL 3.
 * Serves bounded sector windows over IPC to the "ata-client" owner only.
 * The kernel overlay snapshot still uses its own Ring 0 PIO path (degraded
 * fallback) because a 32 KiB snapshot does not fit the 96-byte IPC payload.
 */
#include "os_syscalls.h"

static void putc(char c) { asm volatile("int $0x80" : : "a"(SYS_PUTC), "b"(c)); }
static void puts(const char* t) { int i = 0; while (t[i]) putc(t[i++]); }
static void yield(void) { asm volatile("int $0x80" : : "a"(SYS_YIELD)); }

static int sc1(uint32_t n, uint32_t a) {
    int r; asm volatile("int $0x80" : "=a"(r) : "a"(n), "b"(a)); return r;
}
static int sc2(uint32_t n, uint32_t a, uint32_t b) {
    int r; asm volatile("int $0x80" : "=a"(r) : "a"(n), "b"(a), "c"(b)); return r;
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
static int pio_read(uint8_t drive, uint32_t lba) {
    uint32_t i;
    if (wait_bsy() < 0) return -1;
    select_lba(drive, lba); outb(ATA_CMD, 0x20);
    if (wait_drq() < 0) return -1;
    for (i = 0; i < 256; i++) { uint16_t w = inw(ATA_DATA); sector[2*i] = (uint8_t)w; sector[2*i+1] = (uint8_t)(w >> 8); }
    return wait_bsy();
}
static int pio_write(uint8_t drive, uint32_t lba) {
    uint32_t i;
    if (wait_bsy() < 0) return -1;
    select_lba(drive, lba); outb(ATA_CMD, 0x30);
    if (wait_drq() < 0) return -1;
    for (i = 0; i < 256; i++) outw(ATA_DATA, (uint16_t)(sector[2*i] | (sector[2*i+1] << 8)));
    outb(ATA_CMD, 0xE7); /* cache flush */
    return wait_bsy();
}

static void put32(uint8_t* p, uint32_t v) { p[0]=(uint8_t)v; p[1]=(uint8_t)(v>>8); p[2]=(uint8_t)(v>>16); p[3]=(uint8_t)(v>>24); }
static uint32_t get32(const uint8_t* p) { return p[0] | (p[1]<<8) | ((uint32_t)p[2]<<16) | ((uint32_t)p[3]<<24); }

void main(void) {
    os_ipc_message_t m;
    os_ipc_payload_t r;
    uint8_t status;
    uint32_t i;
    if (sc1(SYS_SERVICE_REGISTER, (uint32_t)"ata-driver") != 0) {
        puts("atadriver register failed\n");
        for (;;) yield();
    }
    /* Next scheduling pass loads the IOPB grant; yield once before touching ports. */
    yield();
    status = inb(ATA_CMD);
    if (status == 0xFF || status == 0x00) puts("atadriver ring3 pio no-disk\n");
    else puts("atadriver ring3 pio ready\n");
    for (;;) {
        int client;
        int32_t rc = -1;
        uint32_t lba, off, len;
        uint8_t drive;
        if (sc1(SYS_IPC_RECV, (uint32_t)&m) != 0) { yield(); continue; }
        if (m.type != OS_IPC_ATA_READ && m.type != OS_IPC_ATA_WRITE) continue;
        for (i = 0; i < sizeof(r); i++) ((uint8_t*)&r)[i] = 0;
        r.type = OS_IPC_ATA_REPLY; r.request_id = m.request_id; r.size = 4U;
        client = sc1(SYS_SERVICE_LOOKUP, (uint32_t)OS_ATA_IPC_CLIENT_SERVICE);
        lba = get32(m.data); drive = m.data[4];
        off = (uint32_t)m.data[5] * 16U; len = m.data[6];
        if (client <= 0 || client != m.sender_pid) {
            rc = OS_ATA_DRIVER_REQUIRED;
            puts("atadriver sector ipc refused\n");
        } else if (drive > 1 || len == 0 || len > OS_ATA_IPC_WINDOW || off + len > 512U) {
            rc = -1;
        } else if (m.type == OS_IPC_ATA_WRITE && drive == 0 && lba < OS_ATA_KERNEL_RESERVED_LBAS) {
            /* LBA 0-63 hold the kernel overlay snapshot (still Ring 0). */
            rc = OS_ATA_DRIVER_REQUIRED;
            puts("atadriver overlay region write refused\n");
        } else if (m.type == OS_IPC_ATA_READ) {
            rc = pio_read(drive, lba);
            if (rc == 0) { for (i = 0; i < len; i++) r.data[4 + i] = sector[off + i]; r.size = 4U + len; rc = (int32_t)len; }
        } else {
            rc = pio_read(drive, lba);
            if (rc == 0) {
                for (i = 0; i < len; i++) sector[off + i] = m.data[8 + i];
                rc = pio_write(drive, lba);
                if (rc == 0) rc = (int32_t)len;
            }
        }
        put32(r.data, (uint32_t)rc);
        (void)sc2(SYS_IPC_SEND, (uint32_t)m.sender_pid, (uint32_t)&r);
    }
}
