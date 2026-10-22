/* ramfs.h - VFS RAM pedagogique pour le shell MOHHDY
 * Pas un vrai systeme de fichiers : table en memoire processus.
 */

#ifndef MOHHDY_RAMFS_H
#define MOHHDY_RAMFS_H

#include <stdint.h>

#define RAMFS_MAX_NODES     64
#define RAMFS_PATH_MAX      128
#define RAMFS_NAME_MAX      64
#define RAMFS_CONTENT_MAX   1024
#define RAMFS_MAX_LIST      32

#define RAMFS_OK            0
#define RAMFS_ERR_NOTFOUND  -1
#define RAMFS_ERR_EXISTS    -2
#define RAMFS_ERR_NOTDIR    -3
#define RAMFS_ERR_ISDIR     -4
#define RAMFS_ERR_NOTEMPTY  -5
#define RAMFS_ERR_NOSPACE   -6
#define RAMFS_ERR_INVAL     -7

#define VFS_LOCK_UN 0
#define VFS_LOCK_SH 1
#define VFS_LOCK_EX 2

#define VFS_RIGHT_READ  1
#define VFS_RIGHT_WRITE 2
#define VFS_RIGHT_EXEC  4

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
int ramfs_mkdir_p(const char *path);
int ramfs_rmdir(const char *path);
int ramfs_rm(const char *path);
int ramfs_rm_r(const char *path);
int ramfs_cp(const char *src, const char *dst);
int ramfs_mv(const char *src, const char *dst);

const char *ramfs_read(const char *path, int *size);
int ramfs_write(const char *path, const char *data, int len);

int ramfs_list(const char *path, ramfs_dirent_t *out, int max_n);
int ramfs_list_recursive(const char *path, ramfs_dirent_t *out, int max_n);
int ramfs_node_count(void);

int vfs_quota_set(const char *path, uint32_t max_bytes, uint32_t max_nodes);
int vfs_quota_check(const char *path, uint32_t bytes_to_add, uint32_t nodes_to_add);
int vfs_access_grant(const char *path, const char *origin, uint8_t rights);
int vfs_access_control(const char *path, const char *origin, uint8_t required_rights);
int vfs_atomic_swap(const char *path_a, const char *path_b);
int vfs_flock(const char *path, int lock_type);

int vfs_journal_begin(void);
int vfs_journal_commit(int tx_id);
int vfs_journal_rollback(int tx_id);

int vfs_snapshot_create(uint32_t *snapshot_id);
int vfs_snapshot_restore(uint32_t snapshot_id);

const char *ramfs_strerror(int err);

#endif
