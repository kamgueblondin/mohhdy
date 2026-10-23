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

/* Lot 29: Transactional File Journaling, Tree Locking & Integrity Check (US-002 / US-034) */
#define VFS_MUTATION_MAX 32

#define VFS_MUTATION_OP_WRITE  1
#define VFS_MUTATION_OP_MKDIR  2
#define VFS_MUTATION_OP_RM     3
#define VFS_MUTATION_OP_RENAME 4

#define VFS_MUTATION_STATUS_UNCOMMITTED 0
#define VFS_MUTATION_STATUS_COMMITTED   1
#define VFS_MUTATION_STATUS_APPLIED     2

typedef struct {
    uint32_t seq_id;
    uint32_t tx_id;
    uint8_t op_type;
    uint8_t status;
    char path[RAMFS_PATH_MAX];
    char target_path[RAMFS_PATH_MAX];
    char data[RAMFS_CONTENT_MAX];
    uint32_t data_len;
} vfs_mutation_entry_t;

typedef struct {
    vfs_mutation_entry_t entries[VFS_MUTATION_MAX];
    uint32_t head;
    uint32_t count;
    uint32_t next_seq;
} vfs_mutation_log_t;

void vfs_mutation_log_init(vfs_mutation_log_t *log);
int vfs_mutation_log_append(vfs_mutation_log_t *log, uint32_t tx_id, uint8_t op_type, const char *path, const char *target_path, const char *data, uint32_t len, uint8_t status);
int vfs_mutation_roll_forward(vfs_mutation_log_t *log);

int vfs_dir_lock_tree(const char *dir_path, uint8_t lock_mode, uint32_t owner_id);
int vfs_dir_unlock_tree(const char *dir_path, uint32_t owner_id);

int vfs_integrity_check(int *out_cycle_detected, int *out_dangling_count);
/* Lot 28: Page Cache & Dirty Writeback Daemon (US-001 / US-033) */
#define VFS_PAGE_CACHE_SIZE 128
#define VFS_CLUSTER_SIZE    512
#define VFS_CACHE_MAX_BYTES (32 * 1024 * 1024)

#define VFS_IO_PRIO_READ_AHEAD   1
#define VFS_IO_PRIO_NORMAL_WRITE 2
#define VFS_IO_PRIO_URGENT_FLUSH 3

typedef struct {
    uint32_t cluster_id;
    uint8_t dirty;
    uint8_t ref_bit;
    uint32_t last_used;
    uint8_t data[VFS_CLUSTER_SIZE];
} vfs_page_entry_t;

typedef struct {
    vfs_page_entry_t pages[VFS_PAGE_CACHE_SIZE];
    uint32_t clock_hand;
    uint32_t hit_count;
    uint32_t miss_count;
    uint32_t flush_count;
} vfs_page_cache_t;

typedef struct {
    uint32_t cluster_id;
    uint8_t prio;
    uint8_t is_write;
    uint32_t length;
    uint8_t data[VFS_CLUSTER_SIZE];
} vfs_io_req_t;

#define VFS_IO_QUEUE_MAX 32
typedef struct {
    vfs_io_req_t reqs[VFS_IO_QUEUE_MAX];
    int head;
    int tail;
    int count;
} vfs_io_queue_t;

void vfs_page_cache_init(void);
int vfs_page_cache_read(uint32_t cluster_id, uint8_t *out_buf);
int vfs_page_cache_write(uint32_t cluster_id, const uint8_t *in_buf, uint8_t dirty);
int vfs_cache_flush(void);
int vfs_cache_sync(void);
int vfs_io_enqueue(uint32_t cluster_id, uint8_t prio, uint8_t is_write, const uint8_t *data, uint32_t len);
int vfs_io_process_coalesced(void);

const char *ramfs_strerror(int err);

#endif
