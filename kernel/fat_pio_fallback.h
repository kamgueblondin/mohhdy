#ifndef FAT_PIO_FALLBACK_H
#define FAT_PIO_FALLBACK_H

#include <stdint.h>

/* Always 0. No image links a Ring 0 FAT sector fallback.
 * kernel_ring0_fat_pio_fallback is not a symbol in either image. */
int kernel_fat_pio_fallback_enabled(void);

/* FAT sector transfer used only when the Ring 3 driver cannot serve it.
 * Always -1 (no port I/O). A dead atadriver is respawned instead.
 * Returns 0 or -1. */
int kernel_fat_sector_fallback(uint8_t drive, uint32_t lba, uint32_t count,
                               void* buffer, int write);

#endif
