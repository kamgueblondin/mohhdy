/* test_ramfs.c - Tests du VFS RAM et de la table de processus simulée */

#include "../../framework/unity.h"
#include "../../framework/test_kernel.h"
#include "../../../userspace/ramfs.h"
#include "procsim.h"
#include <string.h>

static void test_ramfs_seed_files(void) {
    ramfs_init();
    TEST_ASSERT(ramfs_is_dir("/"));
    TEST_ASSERT(ramfs_is_dir("/bin"));
    TEST_ASSERT(ramfs_is_dir("/home/user"));
    TEST_ASSERT(ramfs_is_file("/test.txt"));
    TEST_ASSERT(ramfs_is_file("/hello.txt"));
    TEST_ASSERT(ramfs_is_file("/config.cfg"));
    TEST_ASSERT(ramfs_node_count() > 8);
}

static void test_ramfs_resolve_relative(void) {
    char out[RAMFS_PATH_MAX];
    ramfs_resolve("/home/user", "docs", out, RAMFS_PATH_MAX);
    TEST_ASSERT_EQUAL_STRING("/home/user/docs", out);
    ramfs_resolve("/home/user", "..", out, RAMFS_PATH_MAX);
    TEST_ASSERT_EQUAL_STRING("/home", out);
    ramfs_resolve("/home/user", "../..", out, RAMFS_PATH_MAX);
    TEST_ASSERT_EQUAL_STRING("/", out);
    ramfs_resolve("/home", "/abs", out, RAMFS_PATH_MAX);
    TEST_ASSERT_EQUAL_STRING("/abs", out);
}

static void test_ramfs_mkdir_and_list(void) {
    ramfs_dirent_t ents[RAMFS_MAX_LIST];
    int n;
    int found = 0;
    ramfs_init();
    TEST_ASSERT_EQUAL(RAMFS_OK, ramfs_mkdir("/testdir"));
    TEST_ASSERT(ramfs_is_dir("/testdir"));
    TEST_ASSERT_EQUAL(RAMFS_ERR_EXISTS, ramfs_mkdir("/testdir"));
    n = ramfs_list("/", ents, RAMFS_MAX_LIST);
    TEST_ASSERT(n > 0);
    for (int i = 0; i < n; i++) {
        if (strcmp(ents[i].name, "testdir") == 0 && ents[i].is_dir) found = 1;
    }
    TEST_ASSERT(found);
}

static void test_ramfs_write_read_cat(void) {
    int size = 0;
    const char *data;
    ramfs_init();
    TEST_ASSERT_EQUAL(RAMFS_OK, ramfs_write("/notes.txt", "alpha\nbeta\ngamma\n", 17));
    TEST_ASSERT(ramfs_is_file("/notes.txt"));
    data = ramfs_read("/notes.txt", &size);
    TEST_ASSERT_NOT_NULL(data);
    TEST_ASSERT_EQUAL(17, size);
    TEST_ASSERT_EQUAL_STRING("alpha\nbeta\ngamma\n", data);
}

static void test_ramfs_rm_and_rmdir(void) {
    ramfs_init();
    TEST_ASSERT_EQUAL(RAMFS_OK, ramfs_mkdir("/empty"));
    TEST_ASSERT_EQUAL(RAMFS_OK, ramfs_rmdir("/empty"));
    TEST_ASSERT(!ramfs_exists("/empty"));

    TEST_ASSERT_EQUAL(RAMFS_ERR_ISDIR, ramfs_rm("/home"));
    TEST_ASSERT_EQUAL(RAMFS_ERR_NOTEMPTY, ramfs_rmdir("/home"));
    TEST_ASSERT_EQUAL(RAMFS_OK, ramfs_write("/tmpdel.txt", "x", 1));
    TEST_ASSERT_EQUAL(RAMFS_OK, ramfs_rm("/tmpdel.txt"));
    TEST_ASSERT(!ramfs_exists("/tmpdel.txt"));
    TEST_ASSERT_EQUAL(RAMFS_ERR_NOTFOUND, ramfs_rm("/nope.txt"));
}

static void test_ramfs_cp_mv(void) {
    int size = 0;
    const char *data;
    ramfs_init();
    TEST_ASSERT_EQUAL(RAMFS_OK, ramfs_cp("/hello.txt", "/hello2.txt"));
    data = ramfs_read("/hello2.txt", &size);
    TEST_ASSERT_NOT_NULL(data);
    TEST_ASSERT(size > 0);
    TEST_ASSERT_EQUAL(RAMFS_OK, ramfs_mv("/hello2.txt", "/hello3.txt"));
    TEST_ASSERT(!ramfs_exists("/hello2.txt"));
    TEST_ASSERT(ramfs_is_file("/hello3.txt"));
}

static void test_ramfs_grep_content(void) {
    int size = 0;
    const char *data;
    ramfs_init();
    data = ramfs_read("/ai_knowledge.txt", &size);
    TEST_ASSERT_NOT_NULL(data);
    TEST_ASSERT_NOT_NULL(strstr(data, "bonjour"));
    TEST_ASSERT_NULL(strstr(data, "zzzz-not-found"));
}

static void test_ramfs_parent_must_exist(void) {
    ramfs_init();
    TEST_ASSERT_EQUAL(RAMFS_ERR_NOTDIR, ramfs_mkdir("/no/such/dir"));
    TEST_ASSERT_EQUAL(RAMFS_ERR_NOTDIR, ramfs_write("/no/file.txt", "a", 1));
}

static void test_procsim_table_and_kill(void) {
    procsim_init();
    TEST_ASSERT_EQUAL(5, procsim_count());
    TEST_ASSERT_EQUAL(5, procsim_alive_count());
    TEST_ASSERT_EQUAL(-2, procsim_kill(0));
    TEST_ASSERT_EQUAL(-2, procsim_kill(1));
    TEST_ASSERT_EQUAL(0, procsim_kill(3));
    TEST_ASSERT_EQUAL(4, procsim_alive_count());
    {
        const procsim_entry_t *p = procsim_get_by_pid(3);
        TEST_ASSERT_NOT_NULL(p);
        TEST_ASSERT_EQUAL(0, p->alive);
        TEST_ASSERT_EQUAL('Z', p->state);
    }
    TEST_ASSERT_EQUAL(-1, procsim_kill(99));
}

static void test_ramfs_mv_directory(void) {
    ramfs_init();
    TEST_ASSERT_EQUAL(RAMFS_OK, ramfs_mkdir("/proj"));
    TEST_ASSERT_EQUAL(RAMFS_OK, ramfs_write("/proj/a.txt", "hi", 2));
    TEST_ASSERT_EQUAL(RAMFS_OK, ramfs_mv("/proj", "/proj2"));
    TEST_ASSERT(!ramfs_exists("/proj"));
    TEST_ASSERT(ramfs_is_dir("/proj2"));
    TEST_ASSERT(ramfs_is_file("/proj2/a.txt"));
}

static void test_lot26_vfs_quotas_acls_swap_flock(void) {
    ramfs_init();

    /* Quotas */
    TEST_ASSERT_EQUAL(RAMFS_OK, vfs_quota_set("/sandbox", 100, 5));
    TEST_ASSERT_EQUAL(RAMFS_OK, ramfs_mkdir("/sandbox"));
    TEST_ASSERT_EQUAL(RAMFS_OK, ramfs_write("/sandbox/file1.txt", "1234567890", 10));
    TEST_ASSERT_EQUAL(RAMFS_ERR_NOSPACE, vfs_quota_check("/sandbox", 150, 1));

    /* Access Control (ACLs) */
    TEST_ASSERT_EQUAL(RAMFS_OK, vfs_access_grant("/sandbox/file1.txt", "app_origin", VFS_RIGHT_READ));
    TEST_ASSERT_EQUAL(RAMFS_OK, vfs_access_control("/sandbox/file1.txt", "app_origin", VFS_RIGHT_READ));
    TEST_ASSERT_EQUAL(RAMFS_ERR_INVAL, vfs_access_control("/sandbox/file1.txt", "app_origin", VFS_RIGHT_WRITE));

    /* Atomic Swap */
    TEST_ASSERT_EQUAL(RAMFS_OK, ramfs_write("/file_a.txt", "AAA", 3));
    TEST_ASSERT_EQUAL(RAMFS_OK, ramfs_write("/file_b.txt", "BBB", 3));
    TEST_ASSERT_EQUAL(RAMFS_OK, vfs_atomic_swap("/file_a.txt", "/file_b.txt"));
    int sa = 0, sb = 0;
    const char *da = ramfs_read("/file_a.txt", &sa);
    const char *db = ramfs_read("/file_b.txt", &sb);
    TEST_ASSERT_EQUAL_STRING("BBB", da);
    TEST_ASSERT_EQUAL_STRING("AAA", db);

    /* File Locking (flock) */
    TEST_ASSERT_EQUAL(RAMFS_OK, vfs_flock("/file_a.txt", VFS_LOCK_SH));
    TEST_ASSERT_EQUAL(RAMFS_OK, vfs_flock("/file_a.txt", VFS_LOCK_SH));
    TEST_ASSERT_EQUAL(RAMFS_ERR_INVAL, vfs_flock("/file_a.txt", VFS_LOCK_EX));
    TEST_ASSERT_EQUAL(RAMFS_OK, vfs_flock("/file_a.txt", VFS_LOCK_UN));
    TEST_ASSERT_EQUAL(RAMFS_OK, vfs_flock("/file_a.txt", VFS_LOCK_UN));
    TEST_ASSERT_EQUAL(RAMFS_OK, vfs_flock("/file_a.txt", VFS_LOCK_EX));
    TEST_ASSERT_EQUAL(RAMFS_ERR_INVAL, vfs_flock("/file_a.txt", VFS_LOCK_SH));
    TEST_ASSERT_EQUAL(RAMFS_OK, vfs_flock("/file_a.txt", VFS_LOCK_UN));
}

static void test_lot25_vfs_mkdir_p_rm_r(void) {
    ramfs_dirent_t rents[RAMFS_MAX_LIST];
    int count;
    ramfs_init();
    TEST_ASSERT_EQUAL(RAMFS_OK, ramfs_mkdir_p("/web/assets/css"));
    TEST_ASSERT(ramfs_is_dir("/web"));
    TEST_ASSERT(ramfs_is_dir("/web/assets"));
    TEST_ASSERT(ramfs_is_dir("/web/assets/css"));
    TEST_ASSERT_EQUAL(RAMFS_OK, ramfs_write("/web/assets/css/style.css", "body { color: red; }", 20));
    TEST_ASSERT(ramfs_is_file("/web/assets/css/style.css"));

    count = ramfs_list_recursive("/web", rents, RAMFS_MAX_LIST);
    TEST_ASSERT(count >= 3);

    /* Directory copy */
    TEST_ASSERT_EQUAL(RAMFS_OK, ramfs_cp("/web", "/web2"));
    TEST_ASSERT(ramfs_is_dir("/web2"));
    TEST_ASSERT(ramfs_is_file("/web2/assets/css/style.css"));

    /* Recursive delete */
    TEST_ASSERT_EQUAL(RAMFS_OK, ramfs_rm_r("/web"));
    TEST_ASSERT(!ramfs_exists("/web"));
    TEST_ASSERT(!ramfs_exists("/web/assets/css/style.css"));
    TEST_ASSERT(ramfs_exists("/web2/assets/css/style.css"));
}

static void test_lot29_vfs_journaling_locks_integrity(void) {
    ramfs_init();

    vfs_mutation_log_t log;
    vfs_mutation_log_init(&log);

    vfs_mutation_log_append(&log, 101, VFS_MUTATION_OP_MKDIR, "/var/data", 0, 0, 0, VFS_MUTATION_STATUS_COMMITTED);
    vfs_mutation_log_append(&log, 101, VFS_MUTATION_OP_WRITE, "/var/data/test.txt", 0, "hello_lot29", 11, VFS_MUTATION_STATUS_COMMITTED);
    vfs_mutation_log_append(&log, 102, VFS_MUTATION_OP_WRITE, "/var/data/uncommitted.txt", 0, "draft", 5, VFS_MUTATION_STATUS_UNCOMMITTED);

    int applied = vfs_mutation_roll_forward(&log);
    TEST_ASSERT_EQUAL(2, applied);
    TEST_ASSERT(ramfs_exists("/var/data/test.txt"));
    TEST_ASSERT(!ramfs_exists("/var/data/uncommitted.txt"));

    int sz = 0;
    const char *content = ramfs_read("/var/data/test.txt", &sz);
    TEST_ASSERT_NOT_NULL(content);
    TEST_ASSERT_EQUAL(11, sz);

    TEST_ASSERT_EQUAL(0, vfs_dir_lock_tree("/var", VFS_LOCK_SH, 1));
    TEST_ASSERT_EQUAL(0, vfs_dir_lock_tree("/var/data", VFS_LOCK_SH, 2));
    TEST_ASSERT_EQUAL(-1, vfs_dir_lock_tree("/var", VFS_LOCK_EX, 3));
    TEST_ASSERT_EQUAL(0, vfs_dir_unlock_tree("/var", 1));
    TEST_ASSERT_EQUAL(0, vfs_dir_unlock_tree("/var/data", 2));

    TEST_ASSERT_EQUAL(0, vfs_dir_lock_tree("/var", VFS_LOCK_EX, 1));
    TEST_ASSERT_EQUAL(-1, vfs_dir_lock_tree("/var/data", VFS_LOCK_SH, 2));
    TEST_ASSERT_EQUAL(0, vfs_dir_unlock_tree("/var", 1));

    int cycle = -1, dangling = -1;
    TEST_ASSERT_EQUAL(0, vfs_integrity_check(&cycle, &dangling));
    TEST_ASSERT_EQUAL(0, cycle);
    TEST_ASSERT_EQUAL(0, dangling);
}

static void test_lot28_page_cache_and_io_queue(void) {
    vfs_page_cache_init();
    uint8_t buf[512];
    for (int i = 0; i < 512; i++) buf[i] = (uint8_t)i;

    TEST_ASSERT_EQUAL(0, vfs_page_cache_write(10, buf, 1));
    uint8_t read_buf[512];
    TEST_ASSERT_EQUAL(0, vfs_page_cache_read(10, read_buf));
    TEST_ASSERT_EQUAL(buf[0], read_buf[0]);

    TEST_ASSERT_EQUAL(1, vfs_cache_flush());
    TEST_ASSERT_EQUAL(0, vfs_cache_flush());

    TEST_ASSERT_EQUAL(0, vfs_io_enqueue(20, VFS_IO_PRIO_READ_AHEAD, 0, buf, 512));
    TEST_ASSERT_EQUAL(0, vfs_io_enqueue(15, VFS_IO_PRIO_URGENT_FLUSH, 1, buf, 512));
    TEST_ASSERT_EQUAL(2, vfs_io_process_coalesced());
}

static void test_lot30_vfs_mount_overlay_defrag(void) {
    ramfs_init();

    /* 1. Mount table test */
    vfs_mount_table_t mtable;
    vfs_mount_table_init(&mtable);
    TEST_ASSERT_EQUAL(RAMFS_OK, vfs_mount_point_add(&mtable, "/mnt/data", "/data", 0));
    TEST_ASSERT_EQUAL(RAMFS_ERR_EXISTS, vfs_mount_point_add(&mtable, "/mnt/data", "/data2", 0));
    TEST_ASSERT_EQUAL(RAMFS_OK, vfs_mount_point_remove(&mtable, "/mnt/data"));
    TEST_ASSERT_EQUAL(0, mtable.count);

    /* 2. Overlay FS COW write and file lookup */
    ramfs_mkdir_p("/lower1");
    ramfs_mkdir_p("/upper");
    ramfs_write("/lower1/file.txt", "lower_data", 10);

    vfs_overlay_config_t ov_cfg;
    ov_cfg.active = 1;
    ov_cfg.lower_count = 1;
    strcpy(ov_cfg.lower_layers[0], "/lower1");
    strcpy(ov_cfg.upper_layer, "/upper");
    strcpy(ov_cfg.work_dir, "/work");

    char found_path[RAMFS_PATH_MAX];
    uint8_t is_wh = 0;
    TEST_ASSERT_EQUAL(RAMFS_OK, vfs_overlay_file_lookup(&ov_cfg, "/file.txt", found_path, sizeof(found_path), &is_wh));
    TEST_ASSERT_EQUAL_STRING("/lower1/file.txt", found_path);

    /* COW Write */
    TEST_ASSERT_EQUAL(RAMFS_OK, vfs_overlay_cow_write(&ov_cfg, "/file.txt", "upper_data", 10));
    TEST_ASSERT_EQUAL(RAMFS_OK, vfs_overlay_file_lookup(&ov_cfg, "/file.txt", found_path, sizeof(found_path), &is_wh));
    TEST_ASSERT_EQUAL_STRING("/upper/file.txt", found_path);

    /* 3. Defrag compact */
    int moved = 0, freed = 0;
    TEST_ASSERT_EQUAL(RAMFS_OK, vfs_defrag_compact(&moved, &freed));
}

static void test_lot31_vfs_wal_locks_reconcile(void) {
    /* 1. WAL Log Append & Checkpoint Apply */
    vfs_wal_log_t wal;
    vfs_wal_init(&wal);

    int lsn1 = vfs_wal_log_append(&wal, 1, VFS_MUTATION_OP_MKDIR, "/dist_dir", NULL, 0);
    TEST_ASSERT_GREATER_THAN(0, lsn1);

    int lsn2 = vfs_wal_log_append(&wal, 1, VFS_MUTATION_OP_WRITE, "/dist_dir/data.txt", "dist_data", 9);
    TEST_ASSERT_GREATER_THAN(lsn1, lsn2);

    int applied = vfs_wal_checkpoint_apply(&wal, lsn2);
    TEST_ASSERT_EQUAL(2, applied);
    TEST_ASSERT_EQUAL(1, ramfs_exists("/dist_dir/data.txt"));

    /* 2. Distributed Lock Acquire & Release */
    TEST_ASSERT_EQUAL(RAMFS_OK, vfs_distributed_lock_acquire("/dist_dir/data.txt", 1, 100, 10));
    TEST_ASSERT_EQUAL(-1, vfs_distributed_lock_acquire("/dist_dir/data.txt", 2, 100, 20)); /* Held by 1 */
    TEST_ASSERT_EQUAL(RAMFS_OK, vfs_distributed_lock_acquire("/dist_dir/data.txt", 2, 100, 120)); /* Expired takeover */
    TEST_ASSERT_EQUAL(RAMFS_OK, vfs_distributed_lock_release("/dist_dir/data.txt", 2));

    /* 3. Distributed Reconcile */
    vfs_remote_inode_t rem_inodes[2];
    strcpy(rem_inodes[0].path, "/remote_dir");
    rem_inodes[0].is_dir = 1;
    rem_inodes[0].hash = 0;
    rem_inodes[0].size = 0;

    strcpy(rem_inodes[1].path, "/remote_dir/file.txt");
    rem_inodes[1].is_dir = 0;
    rem_inodes[1].hash = 12345;
    rem_inodes[1].size = 10;

    int synced = 0, conflicts = 0;
    TEST_ASSERT_EQUAL(RAMFS_OK, vfs_distributed_reconcile(rem_inodes, 2, 1, &synced, &conflicts));
    TEST_ASSERT_EQUAL(2, synced);
    TEST_ASSERT_EQUAL(0, conflicts);
}

static void test_lot32_vfs_mvcc_commit_compaction(void) {
    ramfs_init();
    ramfs_mkdir_p("/mvcc");
    ramfs_write("/mvcc/data.txt", "v1_content", 10);

    vfs_mvcc_snapshot_t snap;
    TEST_ASSERT_EQUAL(0, vfs_mvcc_snapshot_create(1, &snap));
    TEST_ASSERT_EQUAL(1, snap.snapshot_version);

    /* Modify live file */
    ramfs_write("/mvcc/data.txt", "v2_content_new", 14);

    vfs_mvcc_snapshot_t snap2;
    TEST_ASSERT_EQUAL(0, vfs_mvcc_snapshot_create(2, &snap2));

    /* Read via MVCC snapshot version 1 */
    char buf[64];
    uint32_t sz = 0;
    TEST_ASSERT_EQUAL(0, vfs_mvcc_read_version("/mvcc/data.txt", 1, buf, sizeof(buf), &sz));
    TEST_ASSERT_EQUAL_STRING("v1_content", buf);

    /* Read via MVCC snapshot version 2 */
    TEST_ASSERT_EQUAL(0, vfs_mvcc_read_version("/mvcc/data.txt", 2, buf, sizeof(buf), &sz));
    TEST_ASSERT_EQUAL_STRING("v2_content_new", buf);

    /* Distributed commit validation */
    vfs_dist_tx_context_t tx_ctx;
    tx_ctx.tx_id = 101;
    tx_ctx.node_count = 3;
    tx_ctx.phase = 0;
    tx_ctx.node_votes[0].vote_prepared = 1;
    tx_ctx.node_votes[1].vote_prepared = 1;
    tx_ctx.node_votes[2].vote_prepared = 0;

    int committed = 0;
    TEST_ASSERT_EQUAL(0, vfs_distributed_commit_validate(&tx_ctx, 2, &committed));
    TEST_ASSERT_EQUAL(1, committed);

    /* Journal compaction */
    vfs_wal_log_t wal;
    vfs_wal_init(&wal);
    vfs_wal_log_append(&wal, 1, 1, "/mvcc/data.txt", "v1_content", 10);

    int reclaimed = 0, compacted = 0;
    TEST_ASSERT_EQUAL(0, vfs_compaction_journal_compact(&wal, &reclaimed, &compacted));
    TEST_ASSERT_EQUAL(1, compacted);
    TEST_ASSERT_EQUAL(VFS_WAL_MAX_ENTRIES, reclaimed);
}

int main(void) {
    unity_init();

    RUN_TEST(test_ramfs_seed_files);
    RUN_TEST(test_ramfs_resolve_relative);
    RUN_TEST(test_ramfs_mkdir_and_list);
    RUN_TEST(test_ramfs_write_read_cat);
    RUN_TEST(test_ramfs_rm_and_rmdir);
    RUN_TEST(test_ramfs_cp_mv);
    RUN_TEST(test_ramfs_grep_content);
    RUN_TEST(test_ramfs_parent_must_exist);
    RUN_TEST(test_ramfs_mv_directory);
    RUN_TEST(test_lot25_vfs_mkdir_p_rm_r);
    RUN_TEST(test_lot26_vfs_quotas_acls_swap_flock);
    RUN_TEST(test_lot29_vfs_journaling_locks_integrity);
    RUN_TEST(test_procsim_table_and_kill);
    RUN_TEST(test_lot28_page_cache_and_io_queue);
    RUN_TEST(test_lot30_vfs_mount_overlay_defrag);
    RUN_TEST(test_lot31_vfs_wal_locks_reconcile);
    RUN_TEST(test_lot32_vfs_mvcc_commit_compaction);

    unity_print_results();
    unity_cleanup();
    return (unity_stats.tests_failed == 0) ? 0 : 1;
}
