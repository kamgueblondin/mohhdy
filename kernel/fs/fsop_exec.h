#ifndef MOHHDY_FSOP_EXEC_H
#define MOHHDY_FSOP_EXEC_H

/* Tranche 4 suite: FAT16/FAT32/overlay operation codec and dispatch.
 *
 * The same object is linked in the kernel (explicit FAT create/unlink/rename
 * naming rules for the boot and fallback paths, request encoding) and in the
 * Ring 3 atadriver (request decoding and execution against its own volumes
 * and its own overlay store). Pure: no port I/O, no syscall. */

#include <stdint.h>
#include "os_syscalls.h"
#include "fat16.h"
#include "fat32.h"

/* Explicit FAT mutations (8.3, LFN, sub-directory paths, mkdir/rmdir). */
int fatvfs_fat16_create(fat16_volume_t* v, const char* name, const char* data, uint32_t size, int is_dir);
int fatvfs_fat16_unlink(fat16_volume_t* v, const char* name);
int fatvfs_fat16_rename(fat16_volume_t* v, const char* old_name, const char* new_name);
int fatvfs_fat32_create(fat32_volume_t* v, const char* name, const char* data, uint32_t size, int is_dir);
int fatvfs_fat32_unlink(fat32_volume_t* v, const char* name);
int fatvfs_fat32_rename(fat32_volume_t* v, const char* old_name, const char* new_name);

/* OS_ATA_FS_STORE_* flag an op needs, 0 for an unknown op. */
uint32_t fsop_store_for(uint32_t op);
/* 1 if the op may change the disk (FAT) or the overlay store. */
int fsop_is_mutation(uint32_t op);
/* 1 for FAT16/FAT32 mutations (the debug crash hook targets those). */
int fsop_is_fat_mutation(uint32_t op);

/* Serialises a request: header, path\0, path2\0, input bytes. Returns the
 * encoded size or -1 (too large, path too long, bad arguments). */
int fsop_encode(uint8_t* buf, uint32_t capacity, uint32_t op, uint32_t arg0, uint32_t arg1,
                const char* path, const char* path2, const void* in, uint32_t in_len,
                uint32_t out_cap);
/* Validates and splits an encoded request (pointers into buf). 0 or -1. */
int fsop_decode(const uint8_t* buf, uint32_t length, os_ata_fsop_request_t* req,
                const char** path, const char** path2, const uint8_t** in);

/* Runs a decoded request against the given volumes and the overlay store
 * linked in this image. Writes at most out_cap bytes to out and returns the
 * operation result; *out_len gets the output size. A volume passed as NULL
 * answers OS_FAT16_NOT_MOUNTED. */
int32_t fsop_execute(const os_ata_fsop_request_t* req, const char* path, const char* path2,
                     const uint8_t* in, uint8_t* out, uint32_t out_cap, uint32_t* out_len,
                     fat16_volume_t* v16, fat32_volume_t* v32);

#endif
