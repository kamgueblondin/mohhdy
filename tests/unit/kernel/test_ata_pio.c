/* Kernel ATA PIO (kernel/ata.c) against a small IDE device model.
 *
 * The model follows QEMU hw/ide: a new command is ignored while BSY or DRQ
 * is set, data words go to the transfer in progress at its current LBA, and
 * SRST (device control bit 2) aborts the transfer. Regression for the
 * qemu-ata-driver CI flake (run 37095701787): after a transfer stopped half
 * way, the FAT16 fallback write landed in the stale transfer (LBA 6-9 of the
 * overlay snapshot) and reads returned zeros. */
#include "../../framework/unity.h"
#include <stdint.h>
#include <string.h>

#define ATA_PIO_HOST_HOOKS 1
#define ata_init pio_ata_init
#define ata_present pio_ata_present
#define ata_read_sectors pio_ata_read_sectors
#define ata_write_sectors pio_ata_write_sectors
#include "../../../kernel/ata.c"

#define DISK_SECTORS 256U
#define ST_BSY 0x80U
#define ST_DRDY 0x40U
#define ST_DRQ 0x08U

static uint8_t disk[DISK_SECTORS * 512U];
static uint8_t io_sector[512];
static uint8_t reg_drive, reg_count, reg_lba0, reg_lba1, reg_lba2;
static uint8_t status;
static int xfer_write;
static uint32_t xfer_lba, xfer_left, io_pos;
static uint32_t busy_polls;          /* BSY reported for the next N status reads */
static int stall_after_sector;       /* -1: none; else BSY stall after this LBA is written */
static uint32_t stall_polls;
static int stall_every_attempt;
static uint32_t ignored_commands;

static void model_reset(void) {
    memset(disk, 0, sizeof(disk));
    status = ST_DRDY;
    xfer_left = 0U;
    io_pos = 0U;
    busy_polls = 0U;
    stall_after_sector = -1;
    stall_polls = 0U;
    stall_every_attempt = 0;
    ignored_commands = 0U;
    reg_drive = 0xA0U;
}

static int slave_selected(void) { return (reg_drive & 0x10U) != 0U; }

static void load_sector(void) {
    if (xfer_lba < DISK_SECTORS) memcpy(io_sector, disk + xfer_lba * 512U, 512U);
    else memset(io_sector, 0, 512U);
}

unsigned char inb(unsigned short port) {
    if (port == 0x1F7 || port == 0x3F6) {
        if (slave_selected()) return 0x00U;
        if (busy_polls) {
            busy_polls--;
            return (uint8_t)(status | ST_BSY);
        }
        return status;
    }
    return 0U;
}

void outb(unsigned short port, unsigned char value) {
    uint32_t lba;
    switch (port) {
        case 0x1F2: reg_count = value; break;
        case 0x1F3: reg_lba0 = value; break;
        case 0x1F4: reg_lba1 = value; break;
        case 0x1F5: reg_lba2 = value; break;
        case 0x1F6: reg_drive = value; break;
        case 0x3F6:
            if (value & 0x04U) { xfer_left = 0U; io_pos = 0U; busy_polls = 0U; status = ST_BSY; }
            else status = ST_DRDY;
            break;
        case 0x1F7:
            if (status & (ST_BSY | ST_DRQ)) { ignored_commands++; break; }
            lba = (uint32_t)reg_lba0 | ((uint32_t)reg_lba1 << 8) | ((uint32_t)reg_lba2 << 16) |
                  ((uint32_t)(reg_drive & 0x0FU) << 24);
            xfer_left = reg_count ? reg_count : 256U;
            xfer_lba = lba;
            io_pos = 0U;
            if (value == 0x20) { xfer_write = 0; load_sector(); status = ST_DRDY | ST_DRQ; }
            else if (value == 0x30) { xfer_write = 1; status = ST_DRDY | ST_DRQ; }
            else if (value == 0xEC) { xfer_write = 0; xfer_left = 1U; memset(io_sector, 0, 512U); status = ST_DRDY | ST_DRQ; }
            else { xfer_left = 0U; status = ST_DRDY; }
            break;
        default: break;
    }
}

static void sector_done(void) {
    xfer_lba++;
    xfer_left--;
    io_pos = 0U;
    if (xfer_left == 0U) status = ST_DRDY;
    else if (!xfer_write) load_sector();
}

unsigned short ata_host_inw(unsigned short port) {
    uint16_t w = 0U;
    (void)port;
    if (!(status & ST_DRQ)) return 0U;
    w = (uint16_t)(io_sector[io_pos] | (io_sector[io_pos + 1U] << 8));
    io_pos += 2U;
    if (io_pos == 512U) sector_done();
    return w;
}

void ata_host_insw(void* out, uint32_t words) {
    uint16_t* p = (uint16_t*)out;
    uint32_t i;
    for (i = 0U; i < words; i++) {
        /* Reading while the device expects host data returns the I/O buffer. */
        if ((status & ST_DRQ) && xfer_write) { p[i] = 0U; continue; }
        p[i] = ata_host_inw(0x1F0);
    }
}

void ata_host_outsw(const void* in, uint32_t words) {
    const uint8_t* p = (const uint8_t*)in;
    uint32_t i;
    for (i = 0U; i < words; i++) {
        if (!(status & ST_DRQ) || !xfer_write) continue;
        io_sector[io_pos] = p[2U * i];
        io_sector[io_pos + 1U] = p[2U * i + 1U];
        io_pos += 2U;
        if (io_pos == 512U) {
            uint32_t written = xfer_lba;
            if (written < DISK_SECTORS) memcpy(disk + written * 512U, io_sector, 512U);
            sector_done();
            if (stall_after_sector >= 0 && (uint32_t)stall_after_sector == written) {
                busy_polls = stall_polls;
                if (!stall_every_attempt) stall_after_sector = -1;
            }
        }
    }
}

static uint8_t buf64[64U * 512U];

static void fill_pattern(void) {
    uint32_t i;
    for (i = 0U; i < sizeof(buf64); i++) buf64[i] = (uint8_t)(0xA0U + i / 512U);
}

static void start(void) {
    model_reset();
    TEST_ASSERT_EQUAL(0, pio_ata_init());
}

/* Old code: rc -1 after the stall with 57 sectors still expected by the
 * device, and the next 1-sector write landed at LBA 7. */
static void test_stalled_write_is_reset_and_next_write_hits_its_lba(void) {
    static const char payload[] = "viakernel";
    uint8_t sector[512];
    uint32_t before = ata_pio_recoveries();
    start();
    fill_pattern();
    stall_after_sector = 6;
    stall_polls = ATA_TIMEOUT + 10U;
    TEST_ASSERT_EQUAL(0, ata_write_sectors_drive(0, 0U, 64U, buf64));
    TEST_ASSERT_TRUE(ata_pio_recoveries() > before);
    TEST_ASSERT_EQUAL(0U, status & ST_DRQ);
    TEST_ASSERT_EQUAL_MEMORY(buf64, disk, sizeof(buf64));
    memset(sector, 0, sizeof(sector));
    memcpy(sector, payload, sizeof(payload) - 1U);
    TEST_ASSERT_EQUAL(0, ata_write_sectors_drive(0, 99U, 1U, sector));
    TEST_ASSERT_EQUAL_MEMORY(payload, disk + 99U * 512U, sizeof(payload) - 1U);
    TEST_ASSERT_EQUAL_MEMORY(buf64 + 7U * 512U, disk + 7U * 512U, 512U);
}

/* A transfer left half done by someone else (a Ring 3 driver killed between
 * sectors): the kernel read resets the channel instead of reading garbage
 * and does not feed the stale write. */
static void test_stale_drq_is_reset_before_kernel_read(void) {
    static const char entry[] = "T4S3    TXT";
    uint8_t out[512];
    uint8_t half[512];
    uint32_t before = ata_pio_recoveries();
    start();
    memcpy(disk + 97U * 512U, entry, sizeof(entry) - 1U);
    reg_count = 8U; reg_lba0 = 6U; reg_lba1 = 0U; reg_lba2 = 0U; reg_drive = 0xE0U;
    outb(0x1F7, 0x30);
    memset(half, 0x5A, sizeof(half));
    ata_host_outsw(half, 256U);
    ata_host_outsw(half, 256U);
    TEST_ASSERT_TRUE((status & ST_DRQ) != 0U);
    memset(out, 0xEE, sizeof(out));
    TEST_ASSERT_EQUAL(0, ata_read_sectors_drive(0, 97U, 1U, out));
    TEST_ASSERT_EQUAL_MEMORY(entry, out, sizeof(entry) - 1U);
    TEST_ASSERT_EQUAL(before + 1U, ata_pio_recoveries());
    TEST_ASSERT_EQUAL(0U, ignored_commands);
    TEST_ASSERT_EQUAL(0U, disk[8U * 512U]);
}

/* A device that stalls on every attempt: the kernel reports the error and
 * leaves the channel idle (no pending DRQ for the next command). */
static void test_persistent_stall_fails_and_leaves_channel_idle(void) {
    uint8_t sector[512];
    start();
    fill_pattern();
    stall_after_sector = 3;
    stall_polls = ATA_TIMEOUT + 10U;
    stall_every_attempt = 1;
    TEST_ASSERT_EQUAL(-1, ata_write_sectors_drive(0, 0U, 16U, buf64));
    TEST_ASSERT_EQUAL(0U, status & ST_DRQ);
    stall_after_sector = -1;
    memset(sector, 0x33, sizeof(sector));
    TEST_ASSERT_EQUAL(0, ata_write_sectors_drive(0, 120U, 1U, sector));
    TEST_ASSERT_EQUAL(0x33U, disk[120U * 512U]);
    TEST_ASSERT_EQUAL(0U, ignored_commands);
}

/* Normal path untouched: no recovery, exact sectors. */
static void test_clean_transfers_need_no_recovery(void) {
    uint8_t out[3U * 512U];
    uint32_t before = ata_pio_recoveries();
    start();
    fill_pattern();
    TEST_ASSERT_EQUAL(0, ata_write_sectors_drive(0, 10U, 3U, buf64));
    TEST_ASSERT_EQUAL(0, ata_read_sectors_drive(0, 10U, 3U, out));
    TEST_ASSERT_EQUAL_MEMORY(buf64, out, sizeof(out));
    TEST_ASSERT_EQUAL(before, ata_pio_recoveries());
    TEST_ASSERT_EQUAL(-1, ata_read_sectors_drive(1, 0U, 1U, out)); /* no slave */
}

int main(void) {
    unity_init();
    RUN_TEST(test_stalled_write_is_reset_and_next_write_hits_its_lba);
    RUN_TEST(test_stale_drq_is_reset_before_kernel_read);
    RUN_TEST(test_persistent_stall_fails_and_leaves_channel_idle);
    RUN_TEST(test_clean_transfers_need_no_recovery);
    unity_print_results();
    unity_cleanup();
    return unity_stats.tests_failed == 0 ? 0 : 1;
}
