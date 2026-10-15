/* Strict image: no Ring 0 FAT sector fallback. A dead atadriver is respawned
 * instead. This file deliberately does not define kernel_ring0_fat_pio_fallback. */
#include "fat_pio_fallback.h"

int kernel_fat_pio_fallback_enabled(void) { return 0; }

int kernel_fat_sector_fallback(uint8_t drive, uint32_t lba, uint32_t count,
                               void* buffer, int write) {
    (void)drive; (void)lba; (void)count; (void)buffer; (void)write;
    return -1;
}
