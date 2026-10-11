/* ramfs.h - VFS RAM pédagogique pour le shell MOHHDY
 * Pas un vrai système de fichiers : table en mémoire processus.
 */

#ifndef MOHHDY_RAMFS_H
#define MOHHDY_RAMFS_H

#define RAMFS_MAX_NODES     64
#define RAMFS_PATH_MAX      128
#define RAMFS_NAME_MAX      64
#define RAMFS_CONTENT_MAX   1024   /* one echo/redirect line, not a file cap */
/* Multi-block files: content lives in a pool of RAMFS_BLOCK-byte blocks; a
 * file uses a contiguous run (so ramfs_read keeps returning one pointer). */
#define RAMFS_BLOCK         256
#define RAMFS_BLOCKS        256
#define RAMFS_FILE_MAX      8192   /* bytes incl. the trailing NUL */
#define RAMFS_MAX_LIST      32

#define RAMFS_OK            0
#define RAMFS_ERR_NOTFOUND  -1
#define RAMFS_ERR_EXISTS    -2
#define RAMFS_ERR_NOTDIR    -3
#define RAMFS_ERR_ISDIR     -4
#define RAMFS_ERR_NOTEMPTY  -5
#define RAMFS_ERR_NOSPACE   -6
#define RAMFS_ERR_INVAL     -7

typedef struct {
    char name[RAMFS_NAME_MAX];
    int is_dir;
    int size;
} ramfs_dirent_t;

void ramfs_init(void);
void ramfs_resolve(const char *cwd, const char *path, char *out, int outsz);

int ramfs_exists(const char *path);
int ramfs_is_dir(const char *path);
int ramfs_is_file(const char *path);

int ramfs_mkdir(const char *path);
int ramfs_rmdir(const char *path);
int ramfs_rm(const char *path);
int ramfs_cp(const char *src, const char *dst);
int ramfs_mv(const char *src, const char *dst);

const char *ramfs_read(const char *path, int *size);
int ramfs_write(const char *path, const char *data, int len);

int ramfs_list(const char *path, ramfs_dirent_t *out, int max_n);
int ramfs_node_count(void);
int ramfs_free_blocks(void);

const char *ramfs_strerror(int err);

#endif
