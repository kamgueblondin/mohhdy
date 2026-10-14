#ifndef MOHHDY_BOOT_STORAGE_H
#define MOHHDY_BOOT_STORAGE_H
#include <stdint.h>

#define FAT16_ATA_READ_WINDOW_SECTORS 16U

int kernel_fat16_ata_read_sector(uint32_t lba, void* buffer);
int kernel_fat16_ata_read_sectors(uint32_t lba, uint32_t count, void* buffer);
int kernel_fat16_ata_write_sector(uint32_t lba, const void* buffer);
int kernel_fat32_ata_slave_read_sector(uint32_t lba, void* buffer);
int kernel_fat32_ata_slave_write_sector(uint32_t lba, const void* buffer);

/* Point a mounted volume at the Ring 0 fallback callbacks. */
void kernel_fat16_bind_io(void);
void kernel_fat32_bind_io(void);
void kernel_storage_refresh_fences(void);

/* Strict image defers the mount. Legacy image mounts from Ring 0. */
void kernel_boot_storage(void);
/* Ring 3 atadriver hands the boot sector and the overlay snapshot here.
 * 0 on success. Does not read the disk itself. */
int kernel_boot_storage_install(uint32_t kind, const uint8_t* data, uint32_t len);
#endif
