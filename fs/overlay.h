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
/* Lot 46 (US-G16): version 4 keeps the v3 records but spans the 64-sector
 * AIOV area (LBA 0-63) plus an extension area after the event journal
 * (OS_ATA_OVERLAY_EXT_LBA, 128 sectors; never inside the FAT16 volume,
 * LBA 64-4221, nor the journals, LBA 4222-4224). A snapshot is still
 * written as v3 when it fits the 32 KiB area with at most OV_SNAP_V3_NODES
 * records, so a small overlay keeps the exact v3 bytes. */
#define OV_SNAP_V4_VERSION 4
#define OV_SNAP_V3_NODES 128            /* node count stored in a v3 header */
#define OV_SNAP_NODES   288             /* RAM node slots (384-byte blocks), v4 header */
#define OV_SNAP_PATH    80
#define OV_SNAP_DATA    384
#define OV_SNAP_NODE    (1 + 1 + 2 + 4 + OV_SNAP_PATH + OV_SNAP_DATA)  /* v2 record */
#define OV_SNAP_REC_HDR 8U
#define OV_SNAP_SIZE    (64 * 512)      /* = OS_ATA_OVERLAY_SECTORS * 512 (v3 image) */
/* v4 image: primary area + extension area (sector s of the image is LBA s
 * below 64, OS_ATA_OVERLAY_EXT_LBA + s - 64 above, see ov_image_lba). */
#define OV_SNAP_MAX_SIZE (OS_ATA_OVERLAY_ALL_SECTORS * 512U)

#define OV_SNAP_V2_VERSION 2
#define OV_SNAP_V2_NODES   64
#define OV_SNAP_V2_PATH    80
#define OV_SNAP_V2_DATA    384
#define OV_SNAP_V2_NODE    (1 + 1 + 2 + 4 + OV_SNAP_V2_PATH + OV_SNAP_V2_DATA)
#define OV_SNAP_V2_SIZE    (16 + OV_SNAP_V2_NODES * OV_SNAP_V2_NODE)

/* Largest single overlay file. Lot 44 bound was 30720 (32 KiB area); Lot 46
 * (US-G16): 64 KiB, in a v4 image (96 KiB with the extension area: 64 KiB of
 * data plus its 171 block headers still leave about 29 KiB to other files).
 * One byte more is refused with OV_ERR_NOSPACE, nothing changes. */
#define OV_FILE_MAX_BYTES 65536U
#define OV_FILE_MAX_V3_BYTES 30720U   /* the Lot 44 bound, kept for the docs/tests */

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
/* Bytes the current overlay needs in a v3/v4 image (header included). */
uint32_t overlay_snapshot_bytes(void);
/* Lot 46: disk budget the mutations are checked against. OV_SNAP_MAX_SIZE
 * by default; OV_SNAP_SIZE when the disk has no extension area (a disk
 * image made before Lot 46, 4225 sectors): then every image stays v3 and
 * what is in RAM still always persists. Clamped to [OV_SNAP_SIZE, MAX]. */
void overlay_set_capacity(uint32_t bytes);
uint32_t overlay_capacity(void);
/* Disk LBA of sector s (0 .. OS_ATA_OVERLAY_ALL_SECTORS-1) of an image. */
uint32_t overlay_image_lba(uint32_t s);
/* Bytes of the image that starts at buf (16-byte header at least): 32 KiB
 * for v1/v2/v3, OV_SNAP_MAX_SIZE for v4, 0 when the magic is wrong. Lets a
 * reader of LBA 0-63 know if the extension area must be read too. */
uint32_t overlay_image_bytes(const uint8_t* buf);
int overlay_stat(const char* path, os_dirent_t* out);
int overlay_listdir(const char* path, os_dirent_t* out, int start, int max_n);
int overlay_listdir_page(const char* path, os_dirent_t* out, uint32_t start, int max_n);
int overlay_is_dir(const char* path);

int overlay_snapshot(uint8_t* buf, uint32_t max, uint32_t* out_size);
int overlay_restore(const uint8_t* buf, uint32_t n);
int overlay_load_disk(void);
int overlay_save_disk(void);
/* Tranche 4 slice 2: redirect() != 0 means the snapshot was queued for the
 * Ring 3 driver; kernel_written() is called after a Ring 0 PIO snapshot write. */
void overlay_set_disk_hooks(int (*redirect)(void), void (*kernel_written)(void));

#endif
