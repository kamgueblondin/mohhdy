#ifndef OVERLAY_H
#define OVERLAY_H

#include <stdint.h>
#include "os_syscalls.h"

#define OV_OK            0
#define OV_ERR_NOTFOUND -1
#define OV_ERR_EXISTS   -2
#define OV_ERR_NOTDIR   -3
#define OV_ERR_ISDIR    -4
#define OV_ERR_NOTEMPTY -5
#define OV_ERR_NOSPACE  -6
#define OV_ERR_INVAL    -7
#define OV_ERR_PROTECTED -8

#define OV_SNAP_MAGIC   0x564F4941u /* 'AIOV' little-endian */
/* Lot 44 (US-G16): version 3 is a dense image of variable length records
 * (only used nodes, continuation blocks carry no path), zero padded to the
 * fixed 64-sector AIOV area (LBA 0-63; FAT16 starts at LBA 64). OV_SNAP_SIZE
 * is that disk budget: every mutation that would not fit is refused with
 * OV_ERR_NOSPACE before anything changes, so what is in RAM always persists.
 * Record: type u8 (0 file head, 1 dir, 2 file block), path_len u8,
 * data_len u16, size u32 (head: total file size, else 0), path, data. */
#define OV_SNAP_VERSION 3
#define OV_SNAP_V3_NODES 128            /* node count written in a v3 header */
#define OV_SNAP_NODES   288             /* RAM node slots (384-byte blocks), Lot 46: 96 KiB / 384 + dirs */
#define OV_SNAP_PATH    80
#define OV_SNAP_DATA    384
#define OV_SNAP_NODE    (1 + 1 + 2 + 4 + OV_SNAP_PATH + OV_SNAP_DATA)  /* v2 record */
#define OV_SNAP_REC_HDR 8U
#define OV_SNAP_SIZE    (64 * 512)      /* = OS_ATA_OVERLAY_SECTORS * 512 */

/* Lot 46 PR B (US-G16): version 4 = the v3 records over two disk areas.
 * Image sectors 0-63 stay at LBA 0-63 (AIOV, before the FAT16 volume at
 * LBA 64); image sectors 64-191 go to an extension area of 128 sectors
 * (64 KiB) placed after the FAT16 volume and after the journals (mount
 * 4222-4223, events 4224): ext_lba >= OV_EXT_MIN_LBA and >= the end of the
 * FAT16 volume. The atadriver enables it only when the disk is that large
 * (IDENTIFY sector count). Header v4 (32 bytes): magic, 4, node slots,
 * records, ext_lba, ext_sectors, 0, 0; records start at byte 32. Without
 * the extension the store writes v3 exactly as before (32 KiB, 30 KiB files). */
#define OV_SNAP_V4_VERSION 4
#define OV_SNAP_V4_HDR     32U
#define OV_EXT_SECTORS     128U
#define OV_EXT_MIN_LBA     4225U
#define OV_SNAP_SECTORS_MAX (64U + OV_EXT_SECTORS)
#define OV_SNAP_SIZE_MAX   (OV_SNAP_SECTORS_MAX * 512U)   /* 96 KiB */
#define OV_FILE_MAX_EXT_BYTES 65536U    /* largest file with the extension */

#define OV_SNAP_V2_VERSION 2
#define OV_SNAP_V2_NODES   64
#define OV_SNAP_V2_PATH    80
#define OV_SNAP_V2_DATA    384
#define OV_SNAP_V2_NODE    (1 + 1 + 2 + 4 + OV_SNAP_V2_PATH + OV_SNAP_V2_DATA)
#define OV_SNAP_V2_SIZE    (16 + OV_SNAP_V2_NODES * OV_SNAP_V2_NODE)

/* Largest single overlay file. Bounded by the 32 KiB AIOV disk area above:
 * 30 KiB of data plus record headers still fit with a few small files. */
#define OV_FILE_MAX_BYTES 30720U

/* Lecture de compatibilite pour les images overlay creees avant AOS-023. */
#define OV_SNAP_V1_VERSION 1
#define OV_SNAP_V1_NODES   32
#define OV_SNAP_V1_PATH    64
#define OV_SNAP_V1_DATA    256
#define OV_SNAP_V1_NODE    (1 + 1 + 2 + 4 + OV_SNAP_V1_PATH + OV_SNAP_V1_DATA)
#define OV_SNAP_V1_SIZE    (16 + OV_SNAP_V1_NODES * OV_SNAP_V1_NODE)

void overlay_init(void);
int overlay_mkdir(const char* path);
int overlay_write(const char* path, const char* data, uint32_t n);
int overlay_append(const char* path, const char* data, uint32_t n);
int overlay_unlink(const char* path);
int overlay_rename(const char* oldpath, const char* newpath);
int overlay_copy(const char* src, const char* dst);
int overlay_read(const char* path, char* buf, uint32_t max);
/* Bytes [offset, offset+max) of a file; 0 at or past the end. Lets the
 * kernel relay large files through the 8 KiB atadriver buffer in chunks. */
int overlay_read_at(const char* path, char* buf, uint32_t max, uint32_t offset);
/* Bytes the current overlay needs in a v3 image (header included). */
uint32_t overlay_snapshot_bytes(void);
int overlay_stat(const char* path, os_dirent_t* out);
int overlay_listdir(const char* path, os_dirent_t* out, int start, int max_n);
int overlay_listdir_page(const char* path, os_dirent_t* out, uint32_t start, int max_n);
int overlay_is_dir(const char* path);

int overlay_snapshot(uint8_t* buf, uint32_t max, uint32_t* out_size);
/* Lot 46 PR B: extension area (0 = none, v3 images, 32 KiB). ext_lba must
 * be >= OV_EXT_MIN_LBA; returns 0 or -1. A restored v4 image sets it too. */
int overlay_set_ext(uint32_t ext_lba);
uint32_t overlay_ext_lba(void);
uint32_t overlay_capacity(void);       /* disk bytes of the current layout */
uint32_t overlay_file_max(void);       /* largest file of the current layout */
/* sectors of an image (64, or 192 for v4) and the LBA of image sector s,
 * read from the header of buf (0 sectors when buf is no v4/v3 image) */
uint32_t overlay_image_sectors(const uint8_t* buf);
uint32_t overlay_image_ext_lba(const uint8_t* buf);
uint32_t overlay_image_lba(uint32_t ext_lba, uint32_t sector);
int overlay_restore(const uint8_t* buf, uint32_t n);
int overlay_load_disk(void);
int overlay_save_disk(void);
/* Tranche 4 slice 2: redirect() != 0 means the snapshot was queued for the
 * Ring 3 driver; kernel_written() is called after a Ring 0 PIO snapshot write. */
void overlay_set_disk_hooks(int (*redirect)(void), void (*kernel_written)(void));

#endif
