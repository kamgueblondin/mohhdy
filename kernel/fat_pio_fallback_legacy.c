/* Legacy image: FAT sector PIO after the Ring 3 driver is gone.
 * Symbol kernel_ring0_fat_pio_fallback is the strict-image guard. */
#include "fat_pio_fallback.h"
#include "ata.h"
#include "syscall/syscall.h"

int kernel_ring0_fat_pio_fallback(uint8_t drive, uint32_t lba, uint32_t count,
                                  void* buffer, int write) {
    int rc = write ? ata_write_sectors_drive(drive, lba, count, (const uint8_t*)buffer)
                   : ata_read_sectors_drive(drive, lba, count, buffer);
    if (rc == 0) syscall_ata_note_fat_kernel_pio(count);
    return rc;
}

int kernel_fat_pio_fallback_enabled(void) { return 1; }

int kernel_fat_sector_fallback(uint8_t drive, uint32_t lba, uint32_t count,
                               void* buffer, int write) {
    return kernel_ring0_fat_pio_fallback(drive, lba, count, buffer, write);
}
