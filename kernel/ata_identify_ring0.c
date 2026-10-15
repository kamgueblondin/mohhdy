/* Legacy image only. The strict image links ata_identify_absent.c instead,
 * so nm must not find kernel_ring0_ata_identify in build/mohhdy.bin. */
#include "ata.h"

extern unsigned char inb(unsigned short port);
extern void outb(unsigned short port, unsigned char data);

#define ATA_DATA     0x1F0
#define ATA_SECCOUNT 0x1F2
#define ATA_LBA0     0x1F3
#define ATA_LBA1     0x1F4
#define ATA_LBA2     0x1F5
#define ATA_CMD      0x1F7
#define ATA_STATUS   0x1F7
#define ATA_ALTSTAT  0x3F6
#define ATA_SR_ERR   0x01
#define ATA_SR_DRQ   0x08
#define ATA_SR_DF    0x20
#define ATA_SR_BSY   0x80
#define ATA_CMD_IDENTIFY 0xEC
#define ATA_TIMEOUT  2000000u

static unsigned short id_inw(unsigned short port) {
    unsigned short ret;
    asm volatile ("inw %1, %0" : "=a"(ret) : "dN"(port));
    return ret;
}

static void id_delay(void) {
    (void)inb(ATA_ALTSTAT);
    (void)inb(ATA_ALTSTAT);
    (void)inb(ATA_ALTSTAT);
    (void)inb(ATA_ALTSTAT);
}

static int id_wait_not_busy(void) {
    uint32_t i;
    for (i = 0; i < ATA_TIMEOUT; i++) {
        uint8_t st = inb(ATA_STATUS);
        if (st == 0xFF) return -1;
        if ((st & ATA_SR_BSY) == 0) return (int)st;
    }
    return -1;
}

static int id_wait_drq(void) {
    uint32_t i;
    for (i = 0; i < ATA_TIMEOUT; i++) {
        uint8_t st = inb(ATA_STATUS);
        if (st == 0xFF) return -1;
        if (st & (ATA_SR_ERR | ATA_SR_DF)) return -1;
        if ((st & ATA_SR_BSY) == 0 && (st & ATA_SR_DRQ)) return 0;
    }
    return -1;
}

/* Drain the 512-byte IDENTIFY payload. The strict image does not link this. */
int kernel_ring0_ata_identify(void) {
    int st;
    uint32_t i;
    outb(ATA_SECCOUNT, 0);
    outb(ATA_LBA0, 0);
    outb(ATA_LBA1, 0);
    outb(ATA_LBA2, 0);
    outb(ATA_CMD, ATA_CMD_IDENTIFY);
    id_delay();
    st = inb(ATA_STATUS);
    if (st == 0x00 || st == 0xFF) return -1;
    if (id_wait_not_busy() < 0) return -1;
    if (id_wait_drq() < 0) return -1;
    for (i = 0; i < 256U; i++) (void)id_inw(ATA_DATA);
    return 0;
}

int ata_boot_identify(void) {
    return kernel_ring0_ata_identify();
}
