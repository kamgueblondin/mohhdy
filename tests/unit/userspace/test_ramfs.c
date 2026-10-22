/* test_ramfs.c - Tests du VFS RAM et de la table de processus simulée */

#include "../../framework/unity.h"
#include "../../framework/test_kernel.h"
#include "ramfs.h"
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
    RUN_TEST(test_procsim_table_and_kill);

    unity_print_results();
    unity_cleanup();
    return (unity_stats.tests_failed == 0) ? 0 : 1;
}
