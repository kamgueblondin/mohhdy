#ifndef FAT_PIO_FALLBACK_H
#define FAT_PIO_FALLBACK_H

#include <stdint.h>

/* 1 on the legacy image, 0 on the strict image.
 * The strict image does not link kernel_ring0_fat_pio_fallback. */
int kernel_fat_pio_fallback_enabled(void);

/* FAT sector transfer used only when the Ring 3 driver cannot serve it.
 * Strict: always -1 (no port I/O). Legacy: Ring 0 PIO, and on success the
 * sectors are counted in fatkpio. Returns 0 or -1. */
int kernel_fat_sector_fallback(uint8_t drive, uint32_t lba, uint32_t count,
                               void* buffer, int write);

#endif
