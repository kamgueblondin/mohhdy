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

/* Lot 30: VFS Virtual Layer & Multi-Mount Overlay Engine (US-003 / US-036) */
#define VFS_MOUNT_MAX 8
#define VFS_OVERLAY_LAYERS_MAX 4

typedef struct {
    char mount_point[RAMFS_PATH_MAX];
    char target_path[RAMFS_PATH_MAX];
    uint32_t flags;
    uint8_t active;
} vfs_mount_point_t;

typedef struct {
    vfs_mount_point_t mounts[VFS_MOUNT_MAX];
    int count;
} vfs_mount_table_t;

typedef struct {
    char lower_layers[VFS_OVERLAY_LAYERS_MAX][RAMFS_PATH_MAX];
    int lower_count;
    char upper_layer[RAMFS_PATH_MAX];
    char work_dir[RAMFS_PATH_MAX];
    uint8_t active;
} vfs_overlay_config_t;

void vfs_mount_table_init(vfs_mount_table_t *table);
int vfs_mount_point_add(vfs_mount_table_t *table, const char *mount_point, const char *target_path, uint32_t flags);
int vfs_mount_point_remove(vfs_mount_table_t *table, const char *mount_point);

int vfs_overlay_cow_write(const vfs_overlay_config_t *cfg, const char *relative_path, const char *data, uint32_t len);
int vfs_overlay_file_lookup(const vfs_overlay_config_t *cfg, const char *relative_path, char *out_found_path, int out_sz, uint8_t *out_is_whiteout);

int vfs_defrag_compact(int *out_nodes_moved, int *out_freed_slots);

/* Lot 31: VFS Distributed Replicated State & Write-Ahead Log Engine (US-004 / US-037) */
#define VFS_WAL_MAX_ENTRIES 32
#define VFS_DIST_LOCKS_MAX  16

typedef struct {
    uint32_t lsn;
    uint32_t node_id;
    uint8_t op_type;
    uint8_t applied;
    char path[RAMFS_PATH_MAX];
    char data[RAMFS_CONTENT_MAX];
    uint32_t data_len;
} vfs_wal_entry_t;

typedef struct {
    vfs_wal_entry_t entries[VFS_WAL_MAX_ENTRIES];
    uint32_t count;
    uint32_t last_applied_lsn;
    uint32_t next_lsn;
} vfs_wal_log_t;

typedef struct {
    char path[RAMFS_PATH_MAX];
    uint32_t holder_node_id;
    uint32_t lease_expire_tick;
    uint8_t active;
} vfs_dist_lock_t;

typedef struct {
    char path[RAMFS_PATH_MAX];
    uint32_t version;
    uint32_t hash;
    uint32_t size;
    uint8_t is_dir;
} vfs_remote_inode_t;

void vfs_wal_init(vfs_wal_log_t *wal);
int vfs_wal_log_append(vfs_wal_log_t *wal, uint32_t node_id, uint8_t op_type, const char *path, const char *data, uint32_t len);
int vfs_wal_checkpoint_apply(vfs_wal_log_t *wal, uint32_t target_lsn);

int vfs_distributed_lock_acquire(const char *path, uint32_t node_id, uint32_t lease_ticks, uint32_t current_tick);
int vfs_distributed_lock_release(const char *path, uint32_t node_id);

int vfs_distributed_reconcile(const vfs_remote_inode_t *remote_inodes, int remote_count, uint32_t remote_term, int *out_synced_count, int *out_conflict_count);

/* Lot 32: VFS Distributed Transactional Storage & MVCC Snapshot Engine (US-005 / US-038) */
#define VFS_MVCC_VERSIONS_MAX 16
#define VFS_MVCC_SNAPSHOTS_MAX 8
#define VFS_COMMIT_NODES_MAX 8

typedef struct {
    uint32_t version_id;
    char path[RAMFS_PATH_MAX];
    char content[RAMFS_CONTENT_MAX];
    uint32_t size;
    uint32_t timestamp;
    uint8_t active;
} vfs_mvcc_version_entry_t;

typedef struct {
    uint32_t snapshot_id;
    uint32_t snapshot_version;
    uint32_t timestamp;
    uint8_t active;
} vfs_mvcc_snapshot_t;

typedef struct {
    uint32_t node_id;
    uint8_t vote_prepared;
    uint32_t term;
} vfs_commit_node_vote_t;

typedef struct {
    uint32_t tx_id;
    char path[RAMFS_PATH_MAX];
    uint32_t target_version;
    vfs_commit_node_vote_t node_votes[VFS_COMMIT_NODES_MAX];
    int node_count;
    uint8_t phase; /* 0=PREPARE, 1=COMMIT, 2=ABORT */
} vfs_dist_tx_context_t;

int vfs_mvcc_snapshot_create(uint32_t snapshot_version, vfs_mvcc_snapshot_t *out_snap);
int vfs_mvcc_read_version(const char *path, uint32_t snapshot_version, char *out_buf, uint32_t buf_cap, uint32_t *out_size);
int vfs_distributed_commit_validate(const vfs_dist_tx_context_t *tx_ctx, int min_quorum, int *out_committed);
int vfs_compaction_journal_compact(vfs_wal_log_t *wal, int *out_reclaimed_slots, int *out_compacted_inodes);

/* Lot 33: VFS CRDT Replicated Conflict-Free Resolution & Distributed Transaction Isolation Engine (US-006 / US-039) */
#define VFS_CRDT_MAX_ELEMENTS 16
#define VFS_DIST_TX_ISO_READ_COMMITTED 1
#define VFS_DIST_TX_ISO_REPEATABLE_READ 2
#define VFS_DIST_TX_ISO_SERIALIZABLE 3

typedef struct {
    char path[RAMFS_PATH_MAX];
    uint32_t timestamp;
    uint32_t node_id;
    char value[RAMFS_CONTENT_MAX];
    uint8_t is_deleted;
} vfs_crdt_element_t;

typedef struct {
    vfs_crdt_element_t elements[VFS_CRDT_MAX_ELEMENTS];
    int count;
} vfs_crdt_node_t;

typedef struct {
    uint32_t tx_id;
    uint32_t isolation_level;
    uint32_t start_timestamp;
    uint8_t active;
    uint8_t read_only;
    char locked_paths[RAMFS_MAX_LIST][RAMFS_PATH_MAX];
    int lock_count;
} vfs_dist_tx_iso_t;

int vfs_crdt_merge(const vfs_crdt_node_t *local, const vfs_crdt_node_t *remote, vfs_crdt_node_t *merged);
int vfs_crdt_resolve_conflict(const char *path, uint32_t local_ts, uint32_t remote_ts, const char *local_val, const char *remote_val, char *out_val, uint32_t out_cap);
int vfs_dist_tx_begin(uint32_t tx_id, uint32_t isolation_level, vfs_dist_tx_iso_t *out_tx);
int vfs_dist_tx_isolation_check(const vfs_dist_tx_iso_t *tx, const char *path, uint8_t is_write);
int vfs_dist_tx_commit_iso(vfs_dist_tx_iso_t *tx);
int vfs_dist_tx_rollback_iso(vfs_dist_tx_iso_t *tx);

/* Lot 35: VFS PCIe NVMe Direct-I/O & Multi-Stream Transactional Journaling Engine (US-008 / US-041) */
#define VFS_NVME_QUEUE_MAX_ENTRIES 16
#define VFS_NVME_STREAMS_MAX        4
#define VFS_NVME_OP_READ            1
#define VFS_NVME_OP_WRITE           2
#define VFS_NVME_OP_FLUSH           3

typedef struct {
    uint8_t opcode;
    uint32_t nsid;
    uint64_t lba;
    uint32_t blocks;
    uint16_t stream_id;
    uint8_t status;
} vfs_nvme_cmd_t;

typedef struct {
    vfs_nvme_cmd_t sq[VFS_NVME_QUEUE_MAX_ENTRIES];
    vfs_nvme_cmd_t cq[VFS_NVME_QUEUE_MAX_ENTRIES];
    uint32_t head;
    uint32_t tail;
    uint32_t count;
    uint32_t completed_count;
} vfs_nvme_queue_t;

typedef struct {
    uint16_t stream_id;
    uint64_t lba;
    uint32_t len;
    uint8_t status; /* 0=pending, 1=flushed */
    char data[RAMFS_CONTENT_MAX];
} vfs_nvme_stream_entry_t;

typedef struct {
    vfs_nvme_stream_entry_t entries[VFS_NVME_QUEUE_MAX_ENTRIES];
    uint32_t count;
    uint32_t flushed_count;
} vfs_nvme_journal_t;

void vfs_nvme_direct_io_init(vfs_nvme_queue_t *q);
int vfs_nvme_direct_io_submit(vfs_nvme_queue_t *q, uint8_t opcode, uint32_t nsid, uint64_t lba, uint32_t blocks, uint16_t stream_id);

void vfs_nvme_multi_stream_journal_init(vfs_nvme_journal_t *j);
int vfs_nvme_multi_stream_journal_append(vfs_nvme_journal_t *j, uint16_t stream_id, uint64_t lba, const char *data, uint32_t len);
int vfs_nvme_multi_stream_journal_flush(vfs_nvme_journal_t *j, uint16_t stream_id, int *out_flushed);

/* Lot 37: VFS PCIe NVMe Dynamic Queue Pair Offloading & Transactional Erasure Coding Engine (US-009 / US-042) */
#define VFS_NVME_QP_MAX_ENTRIES 16
#define VFS_ERASURE_K 4
#define VFS_ERASURE_M 2
#define VFS_ERASURE_CHUNK_SZ 256

typedef struct {
    uint32_t qp_id;
    uint16_t sq_head;
    uint16_t sq_tail;
    uint16_t cq_head;
    uint16_t cq_tail;
    uint32_t submitted_count;
    uint32_t completed_count;
    vfs_nvme_cmd_t sq[VFS_NVME_QP_MAX_ENTRIES];
    vfs_nvme_cmd_t cq[VFS_NVME_QP_MAX_ENTRIES];
    uint8_t active;
} vfs_nvme_qp_t;

typedef struct {
    uint32_t stripe_id;
    uint8_t data_chunks[VFS_ERASURE_K][VFS_ERASURE_CHUNK_SZ];
    uint8_t parity_chunks[VFS_ERASURE_M][VFS_ERASURE_CHUNK_SZ];
    uint32_t chunk_len;
    uint8_t chunk_present[VFS_ERASURE_K + VFS_ERASURE_M]; /* 1 if intact, 0 if lost */
    uint8_t status; /* 0=uncommitted, 1=committed, 2=reconstructed */
} vfs_erasure_stripe_t;

void vfs_nvme_qp_init(vfs_nvme_qp_t *qp, uint32_t qp_id);
int vfs_nvme_qp_offload_submit(vfs_nvme_qp_t *qp, uint8_t opcode, uint32_t nsid, uint64_t lba, uint32_t blocks, uint16_t stream_id);
int vfs_nvme_qp_poll_completion(vfs_nvme_qp_t *qp, int *out_completed);

void vfs_erasure_stripe_init(vfs_erasure_stripe_t *stripe, uint32_t stripe_id, uint32_t chunk_len);
int vfs_erasure_encode(vfs_erasure_stripe_t *stripe, const uint8_t data[VFS_ERASURE_K][VFS_ERASURE_CHUNK_SZ]);
int vfs_erasure_reconstruct(vfs_erasure_stripe_t *stripe, int *out_reconstructed_chunks);
int vfs_erasure_commit_tx(vfs_erasure_stripe_t *stripe, uint32_t tx_id);

/* Lot 38: VFS PCIe NVMe Direct Memory Access (DMA) Scatter-Gather Engine & Distributed Erasure Coding Cross-Node Synchronization Engine (US-010 / US-043) */
#define VFS_NVME_SG_MAX_ENTRIES 16
#define VFS_ERASURE_DIST_NODES_MAX 8

typedef struct {
    uint64_t buffer_addr;
    uint32_t length;
    uint32_t flags; /* 0=data, 1=end */
} vfs_nvme_sg_entry_t;

typedef struct {
    uint32_t list_id;
    vfs_nvme_sg_entry_t entries[VFS_NVME_SG_MAX_ENTRIES];
    uint32_t entry_count;
    uint32_t total_bytes;
    uint8_t status; /* 0=idle, 1=prepared, 2=executed */
} vfs_nvme_sg_list_t;

typedef struct {
    uint32_t node_id;
    uint8_t chunks[VFS_ERASURE_K + VFS_ERASURE_M][VFS_ERASURE_CHUNK_SZ];
    uint8_t online;
    uint32_t rtt_ms;
} vfs_erasure_dist_node_t;

typedef struct {
    uint32_t stripe_id;
    vfs_erasure_stripe_t stripe;
    vfs_erasure_dist_node_t nodes[VFS_ERASURE_DIST_NODES_MAX];
    uint32_t node_count;
    uint32_t synced_nodes;
    uint8_t sync_status; /* 0=unsynced, 1=synced, 2=degraded */
} vfs_erasure_dist_stripe_t;

void vfs_nvme_sg_init(vfs_nvme_sg_list_t *sg, uint32_t list_id);
int vfs_nvme_sg_add_descriptor(vfs_nvme_sg_list_t *sg, uint64_t buffer_addr, uint32_t length, uint32_t flags);
int vfs_nvme_sg_execute(vfs_nvme_sg_list_t *sg, uint8_t opcode, uint64_t start_lba, int *out_transferred_bytes);

void vfs_erasure_dist_sync_init(vfs_erasure_dist_stripe_t *dstripe, uint32_t stripe_id, uint32_t chunk_len);
int vfs_erasure_dist_sync_node(vfs_erasure_dist_stripe_t *dstripe, uint32_t node_id, const uint8_t *chunk_data, uint32_t chunk_idx);
int vfs_erasure_dist_hot_reconstruct(vfs_erasure_dist_stripe_t *dstripe, uint32_t target_node_id, int *out_recovered_chunks);

/* Lot 39: VFS PCIe NVMe End-to-End Data Protection DIF/DIX & Distributed MVCC Garbage Collection Engine (US-011 / US-044) */
#define VFS_NVME_DIF_PI_SIZE 8
#define VFS_MVCC_GC_MAX_OBSOLETE 32

typedef struct {
    uint16_t guard_tag; /* CRC16 check */
    uint16_t app_tag;   /* Application tag */
    uint32_t ref_tag;   /* Lower 32-bit LBA reference tag */
} vfs_nvme_dif_pi_t;

typedef struct {
    uint32_t version_id;
    uint32_t inode_id;
    uint32_t block_id;
    uint32_t timestamp;
    uint8_t is_obsolete;
} vfs_mvcc_gc_entry_t;

typedef struct {
    vfs_mvcc_gc_entry_t obsolete_entries[VFS_MVCC_GC_MAX_OBSOLETE];
    int count;
    uint32_t reclaimed_inodes;
    uint32_t reclaimed_blocks;
    uint32_t min_active_version;
} vfs_mvcc_gc_engine_t;

uint16_t vfs_nvme_dif_crc16(const uint8_t *data, uint32_t len);
int vfs_nvme_dif_generate(const uint8_t *data, uint32_t len, uint32_t lba, uint16_t app_tag, vfs_nvme_dif_pi_t *out_pi);
int vfs_nvme_dif_verify(const uint8_t *data, uint32_t len, uint32_t lba, uint16_t app_tag, const vfs_nvme_dif_pi_t *pi);
int vfs_nvme_dix_format_block(const uint8_t *data, uint32_t data_len, uint32_t lba, uint16_t app_tag, uint8_t *out_block_with_pi, uint32_t *out_total_len);
int vfs_nvme_dix_strip_and_verify(const uint8_t *block_with_pi, uint32_t total_len, uint32_t lba, uint16_t app_tag, uint8_t *out_data, uint32_t *out_data_len);

void vfs_mvcc_gc_init(vfs_mvcc_gc_engine_t *gc, uint32_t min_active_version);
int vfs_mvcc_gc_mark_obsolete(vfs_mvcc_gc_engine_t *gc, uint32_t version_id, uint32_t inode_id, uint32_t block_id, uint32_t timestamp);
int vfs_mvcc_gc_collect_async(vfs_mvcc_gc_engine_t *gc, uint32_t watermark_version, int *out_reclaimed_count);
int vfs_mvcc_gc_stat(const vfs_mvcc_gc_engine_t *gc, uint32_t *out_reclaimed_inodes, uint32_t *out_reclaimed_blocks);

/* Lot 40: VFS PCIe NVMe Zoned Block Device (ZBD) Zone Append Engine & Multi-Master Distributed Metadata Rebalancing Engine (US-012 / US-045) */
#define RAMFS_NVME_ZBD_ZONES_MAX 8
#define VFS_ZBD_ZONE_EMPTY 0
#define VFS_ZBD_ZONE_IMPLICIT_OPEN 1
#define VFS_ZBD_ZONE_CLOSED 2
#define VFS_ZBD_ZONE_FULL 3

#define VFS_DIST_NODES_MAX 8
#define VFS_VECTOR_CLOCK_NODES 4

typedef struct {
    uint32_t zone_id;
    uint32_t wp;
    uint32_t capacity;
    uint8_t state;
    uint32_t write_count;
} vfs_nvme_zbd_zone_t;

typedef struct {
    vfs_nvme_zbd_zone_t zones[RAMFS_NVME_ZBD_ZONES_MAX];
    int zone_count;
    uint32_t total_appends;
    uint32_t total_resets;
} vfs_nvme_zbd_engine_t;

typedef struct {
    uint32_t clock[VFS_VECTOR_CLOCK_NODES];
} vfs_vector_clock_t;

typedef struct {
    uint32_t node_id;
    uint8_t active;
    uint32_t load_weight;
    uint32_t inode_count;
    vfs_vector_clock_t vclock;
} vfs_metadata_node_t;

typedef struct {
    vfs_metadata_node_t nodes[VFS_DIST_NODES_MAX];
    int node_count;
    uint32_t total_rebalances;
    uint32_t total_failovers;
    uint32_t total_conflicts_resolved;
} vfs_dist_metadata_rebalancer_t;

void vfs_nvme_zbd_init(vfs_nvme_zbd_engine_t *zbd, int zone_cnt, uint32_t zone_capacity);
int vfs_nvme_zbd_zone_alloc(vfs_nvme_zbd_engine_t *zbd, uint32_t *out_zone_id);
int vfs_nvme_zbd_zone_reset(vfs_nvme_zbd_engine_t *zbd, uint32_t zone_id);
int vfs_nvme_zbd_zone_append(vfs_nvme_zbd_engine_t *zbd, uint32_t zone_id, const uint8_t *data, uint32_t len, uint32_t *out_wp);

void vfs_dist_metadata_init(vfs_dist_metadata_rebalancer_t *reb);
int vfs_dist_metadata_register_node(vfs_dist_metadata_rebalancer_t *reb, uint32_t node_id, uint32_t initial_inodes);
int vfs_dist_metadata_node_failover(vfs_dist_metadata_rebalancer_t *reb, uint32_t failed_node_id, uint32_t target_node_id);
int vfs_dist_metadata_rebalance(vfs_dist_metadata_rebalancer_t *reb, int *out_migrated_inodes);
int vfs_dist_metadata_resolve_vector_conflict(const vfs_vector_clock_t *clock_a, const vfs_vector_clock_t *clock_b, vfs_vector_clock_t *out_merged_clock, int *out_conflict);

/* Lot 41: VFS PCIe NVMe Host Memory Buffer (HMB) & Dynamic Zone Allocation Engine (US-013 / US-046) */
#define RAMFS_NVME_HMB_PAGES_MAX 8
#define RAMFS_NVME_HMB_BUFFER_SIZE 2048
#define VFS_MULTI_REGION_ZONES_MAX 8

typedef struct {
    uint32_t page_idx;
    uint32_t phys_addr;
    uint8_t registered;
} vfs_nvme_hmb_page_t;

typedef struct {
    vfs_nvme_hmb_page_t pages[RAMFS_NVME_HMB_PAGES_MAX];
    int registered_page_count;
    uint32_t page_size;
    uint8_t buffer[RAMFS_NVME_HMB_BUFFER_SIZE];
    uint32_t head;
    uint32_t tail;
    uint32_t total_writes;
    uint32_t total_reads;
} vfs_nvme_hmb_engine_t;

typedef struct {
    uint32_t zone_id;
    uint32_t region_id;
    uint32_t capacity;
    uint32_t used_bytes;
    uint8_t active;
} vfs_multi_region_zone_t;

typedef struct {
    vfs_multi_region_zone_t zones[VFS_MULTI_REGION_ZONES_MAX];
    int zone_count;
    uint32_t failover_count;
    uint32_t rebalanced_stripes;
} vfs_multi_region_rebalancer_t;

void vfs_nvme_hmb_init(vfs_nvme_hmb_engine_t *hmb, uint32_t total_pages, uint32_t page_size_bytes);
int vfs_nvme_hmb_register_page(vfs_nvme_hmb_engine_t *hmb, uint32_t page_idx, uint32_t phys_addr);
int vfs_nvme_hmb_ring_write(vfs_nvme_hmb_engine_t *hmb, const uint8_t *data, uint32_t len, uint32_t *out_written);
int vfs_nvme_hmb_ring_read(vfs_nvme_hmb_engine_t *hmb, uint8_t *out_data, uint32_t max_len, uint32_t *out_read);

void vfs_multi_region_rebalancer_init(vfs_multi_region_rebalancer_t *reb);
int vfs_multi_region_add_zone(vfs_multi_region_rebalancer_t *reb, uint32_t zone_id, uint32_t region_id, uint32_t capacity);
int vfs_multi_region_failover(vfs_multi_region_rebalancer_t *reb, uint32_t failed_region_id, uint32_t target_region_id);
int vfs_multi_region_rebalance(vfs_multi_region_rebalancer_t *reb, int *out_rebalanced_stripes);

/* Lot 42: Storage VFS PCIe NVMe Single-Root I/O Virtualization (SR-IOV) & Multi-Tenant QoS Isolation Engine (US-014 / US-047) */
#define VFS_NVME_SRIOV_MAX_VFS 8
#define VFS_QOS_MAX_TENANTS 8

typedef struct {
    uint32_t vf_id;
    uint32_t pf_id;
    uint32_t bar_base_addr;
    uint32_t bar_size;
    uint32_t cmd_queue_head;
    uint32_t cmd_queue_tail;
    uint32_t cmds_processed;
    uint8_t active;
} vfs_nvme_sriov_vf_t;

typedef struct {
    vfs_nvme_sriov_vf_t vfs[VFS_NVME_SRIOV_MAX_VFS];
    int vf_count;
    uint32_t total_vf_cmds;
} vfs_nvme_sriov_engine_t;

typedef struct {
    uint32_t tenant_id;
    uint32_t iops_limit;
    uint32_t bw_limit_kbps;
    uint32_t tokens_iops;
    uint32_t tokens_bw;
    uint32_t latency_guarantee_us;
    uint32_t total_iops_consumed;
    uint32_t total_bw_consumed;
    uint8_t active;
} vfs_qos_tenant_t;

typedef struct {
    vfs_qos_tenant_t tenants[VFS_QOS_MAX_TENANTS];
    int tenant_count;
    uint32_t total_refills;
} vfs_qos_isolator_t;

void vfs_nvme_sriov_init(vfs_nvme_sriov_engine_t *engine);
int vfs_nvme_sriov_alloc_vf(vfs_nvme_sriov_engine_t *engine, uint32_t vf_id, uint32_t pf_id, uint32_t bar_base, uint32_t bar_size);
int vfs_nvme_sriov_submit_cmd(vfs_nvme_sriov_engine_t *engine, uint32_t vf_id, uint32_t cmd_id);

void vfs_qos_isolator_init(vfs_qos_isolator_t *qos);
int vfs_qos_register_tenant(vfs_qos_isolator_t *qos, uint32_t tenant_id, uint32_t iops_limit, uint32_t bw_limit_kbps, uint32_t latency_us);
int vfs_qos_consume_tokens(vfs_qos_isolator_t *qos, uint32_t tenant_id, uint32_t iops_req, uint32_t bw_req_kb, int *out_allowed);

/* Lot 43: VFS PCIe NVMe-over-Fabrics (NVMe-oF) & Multi-Tenant Resource Allocator Engine (US-015 / US-048) */
#define VFS_NVMF_MAX_TARGETS 8
#define VFS_RES_ALLOC_MAX_TENANTS 8

typedef struct {
    uint32_t target_id;
    uint32_t transport_type; /* 0=RDMA, 1=TCP */
    uint32_t sq_head;
    uint32_t sq_tail;
    uint32_t cq_head;
    uint32_t cq_tail;
    uint32_t cmds_submitted;
    uint32_t cmds_completed;
    uint8_t active;
} vfs_nvmf_target_t;

typedef struct {
    vfs_nvmf_target_t targets[VFS_NVMF_MAX_TARGETS];
    int target_count;
    uint32_t total_fabric_ops;
} vfs_nvmf_engine_t;

typedef struct {
    uint32_t tenant_id;
    uint32_t io_quota_max;
    uint32_t io_quota_used;
    uint32_t bw_max_kbps;
    uint32_t bw_current_kbps;
    uint32_t latency_guarantee_us;
    uint32_t total_allocated_io;
    uint8_t active;
} vfs_res_tenant_t;

typedef struct {
    vfs_res_tenant_t tenants[VFS_RES_ALLOC_MAX_TENANTS];
    int tenant_count;
    uint32_t total_allocations;
} vfs_res_allocator_t;

void vfs_nvmf_engine_init(vfs_nvmf_engine_t *engine);
int vfs_nvmf_register_target(vfs_nvmf_engine_t *engine, uint32_t target_id, uint32_t transport_type);
int vfs_nvmf_submit_sq(vfs_nvmf_engine_t *engine, uint32_t target_id, uint32_t cmd_id);
int vfs_nvmf_process_cq(vfs_nvmf_engine_t *engine, uint32_t target_id, int *out_completed_count);

void vfs_res_allocator_init(vfs_res_allocator_t *alloc);
int vfs_res_allocator_register_tenant(vfs_res_allocator_t *alloc, uint32_t tenant_id, uint32_t io_quota, uint32_t bw_kbps, uint32_t latency_us);
int vfs_res_allocator_allocate_io(vfs_res_allocator_t *alloc, uint32_t tenant_id, uint32_t requested_io, uint32_t requested_bw, int *out_granted);

#endif
