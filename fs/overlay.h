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
#define OV_SNAP_VERSION 2
#define OV_SNAP_NODES   64
#define OV_SNAP_PATH    80
#define OV_SNAP_DATA    384
#define OV_SNAP_NODE    (1 + 1 + 2 + 4 + OV_SNAP_PATH + OV_SNAP_DATA)
#define OV_SNAP_SIZE    (16 + OV_SNAP_NODES * OV_SNAP_NODE)

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
int overlay_stat(const char* path, os_dirent_t* out);
int overlay_listdir(const char* path, os_dirent_t* out, int start, int max_n);
int overlay_listdir_page(const char* path, os_dirent_t* out, uint32_t start, int max_n);
int overlay_is_dir(const char* path);

int overlay_snapshot(uint8_t* buf, uint32_t max, uint32_t* out_size);
int overlay_restore(const uint8_t* buf, uint32_t n);
int overlay_load_disk(void);
int overlay_save_disk(void);

#endif
