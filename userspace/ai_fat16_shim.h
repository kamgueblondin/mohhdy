#ifndef AI_FAT16_SHIM_H
#define AI_FAT16_SHIM_H
#include <stdint.h>
#include "../kernel/fs/fat16.h"
/* Serve fat16 reads of `name` on `volume` from data[0..size) (worker memory). */
int ai_fat16_shim_attach(fat16_volume_t* volume, const char* name,
                         const uint8_t* data, uint32_t size);
#endif
