#ifndef ATA_H
#define ATA_H

#include <stdint.h>

/* ATA PIO LBA28 primaire (0x1F0), maître ou esclave. No IRQ14. */
#define ATA_DRIVE_MASTER 0U
#define ATA_DRIVE_SLAVE  1U

int ata_init(void);
int ata_present(void);
int ata_present_drive(uint8_t drive);
int ata_read_sectors(uint32_t lba, uint32_t count, void* buf);
int ata_read_sectors_drive(uint8_t drive, uint32_t lba, uint32_t count, void* buf);
int ata_write_sectors(uint32_t lba, uint32_t count, const void* buf);
int ata_write_sectors_drive(uint8_t drive, uint32_t lba, uint32_t count, const void* buf);

#endif
