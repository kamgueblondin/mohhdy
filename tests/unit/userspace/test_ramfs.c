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

/* Multi-block files: beyond the former 1024-byte cap, grow/shrink in
 * place or move, blocks released on rm, NOSPACE when the pool is full. */
static char g_big[RAMFS_FILE_MAX + 16];
static void test_ramfs_multiblock(void) {
    const char* d; int size = 0, i, free0, free1;
    ramfs_init();
    free0 = ramfs_free_blocks();
    for (i = 0; i < 5000; i++) g_big[i] = (char)('a' + i % 26);
    TEST_ASSERT_EQUAL(RAMFS_OK, ramfs_write("/big.txt", g_big, 5000));
    d = ramfs_read("/big.txt", &size);
    TEST_ASSERT_EQUAL(5000, size);
    TEST_ASSERT_EQUAL(0, memcmp(d, g_big, 5000));
    TEST_ASSERT_EQUAL(0, d[5000]);
    TEST_ASSERT_EQUAL(free0 - 20, ramfs_free_blocks());   /* 5001 bytes = 20 blocks */
    /* grow past the run: relocated, content intact */
    TEST_ASSERT_EQUAL(RAMFS_OK, ramfs_write("/small.txt", "x", 1));
    TEST_ASSERT_EQUAL(RAMFS_OK, ramfs_write("/big.txt", g_big, 7000));
    d = ramfs_read("/big.txt", &size);
    TEST_ASSERT_EQUAL(7000, size);
    TEST_ASSERT_EQUAL(0, memcmp(d, g_big, 7000));
    TEST_ASSERT_EQUAL(0, strcmp(ramfs_read("/small.txt", &size), "x"));
    /* copy keeps all blocks; shrink releases the tail; rm frees all */
    TEST_ASSERT_EQUAL(RAMFS_OK, ramfs_cp("/big.txt", "/big2.txt"));
    d = ramfs_read("/big2.txt", &size);
    TEST_ASSERT_EQUAL(7000, size);
    TEST_ASSERT_EQUAL(0, memcmp(d, g_big, 7000));
    TEST_ASSERT_EQUAL(RAMFS_OK, ramfs_write("/big2.txt", "tiny", 4));
    free1 = ramfs_free_blocks();
    TEST_ASSERT_EQUAL(RAMFS_OK, ramfs_rm("/big.txt"));
    TEST_ASSERT_EQUAL(free1 + 28, ramfs_free_blocks());
    /* the per-file cap still applies (truncated like before) */
    for (i = 0; i < RAMFS_FILE_MAX + 10; i++) g_big[i] = 'z';
    TEST_ASSERT_EQUAL(RAMFS_OK, ramfs_write("/cap.txt", g_big, RAMFS_FILE_MAX + 10));
    (void)ramfs_read("/cap.txt", &size);
    TEST_ASSERT_EQUAL(RAMFS_FILE_MAX - 1, size);
    /* pool exhaustion: NOSPACE and no half-created node */
    for (i = 0; i < 40; i++) {
        char name[16] = "/fill00";
        name[5] = (char)('0' + i / 10); name[6] = (char)('0' + i % 10);
        if (ramfs_write(name, g_big, RAMFS_FILE_MAX - 1) != RAMFS_OK) {
            TEST_ASSERT_FALSE(ramfs_exists(name));
            break;
        }
    }
    TEST_ASSERT_TRUE(i < 40);
    TEST_ASSERT_EQUAL(0, strcmp(ramfs_read("/big2.txt", &size), "tiny"));
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
    RUN_TEST(test_ramfs_multiblock);
    RUN_TEST(test_procsim_table_and_kill);

    unity_print_results();
    unity_cleanup();
    return (unity_stats.tests_failed == 0) ? 0 : 1;
}
