/* Shared file operations (shell commands + desktop apps) over a mock VFS. */
#include "../../framework/unity.h"
#include "mockfs.h"
#include <stdio.h>
#include <string.h>

static void test_paths(void) {
    char o[128];
    fsops_resolve("/home", "notes.txt", o, 128); TEST_ASSERT_EQUAL_STRING("/home/notes.txt", o);
    fsops_resolve("/home/a", "../b/./c", o, 128); TEST_ASSERT_EQUAL_STRING("/home/b/c", o);
    fsops_resolve("/", "..", o, 128); TEST_ASSERT_EQUAL_STRING("/", o);
    fsops_resolve("/x", "/y//z/", o, 128); TEST_ASSERT_EQUAL_STRING("/y/z", o);
    fsops_parent("/home/notes.txt", o, 128); TEST_ASSERT_EQUAL_STRING("/home", o);
    fsops_parent("/home", o, 128); TEST_ASSERT_EQUAL_STRING("/", o);
    TEST_ASSERT_EQUAL_STRING("notes.txt", fsops_basename("/home/notes.txt"));
    TEST_ASSERT_EQUAL_STRING("/", fsops_basename("/"));
    fsops_join(o, 128, "/", "a"); TEST_ASSERT_EQUAL_STRING("/a", o);
    TEST_ASSERT(fsops_valid_name("a.txt"));
    TEST_ASSERT(!fsops_valid_name("a/b"));
    TEST_ASSERT(!fsops_valid_name(".."));
    TEST_ASSERT(!fsops_valid_name(""));
}

static void test_list_sorted_dirs_first(void) {
    os_dirent_t e[16];
    int n;
    mfs_reset();
    mfs_add("/home/b.txt", 0, "x", 1);
    mfs_add("/home/zdir", 1, 0, 0);
    mfs_add("/home/adir", 1, 0, 0);
    n = fsops_list(&mfs_io, "/home", e, 16);
    TEST_ASSERT_EQUAL(4, n);
    TEST_ASSERT_EQUAL_STRING("adir", e[0].name);
    TEST_ASSERT_EQUAL_STRING("zdir", e[1].name);
    TEST_ASSERT_EQUAL_STRING("b.txt", e[2].name);
    TEST_ASSERT_EQUAL_STRING("notes.txt", e[3].name);
    TEST_ASSERT_EQUAL(FSOPS_ERR_NOTDIR, fsops_list(&mfs_io, "/home/b.txt", e, 16));
}

static void test_create_rename_delete(void) {
    char d[128];
    char buf[64];
    mfs_reset();
    TEST_ASSERT_EQUAL(0, fsops_mkdir(&mfs_io, "/home/docs"));
    TEST_ASSERT_EQUAL(FSOPS_ERR_EXISTS, fsops_mkdir(&mfs_io, "/home/docs"));
    TEST_ASSERT_EQUAL(1, fsops_touch(&mfs_io, "/home/docs/a.txt"));
    TEST_ASSERT_EQUAL(0, fsops_touch(&mfs_io, "/home/docs/a.txt"));
    TEST_ASSERT_EQUAL(0, fsops_write(&mfs_io, "/home/docs/a.txt", "hello", 5));
    TEST_ASSERT_EQUAL(0, fsops_mv(&mfs_io, "/home/docs/a.txt", "/home/docs/b.txt", d, 128));
    TEST_ASSERT_EQUAL_STRING("/home/docs/b.txt", d);
    TEST_ASSERT_EQUAL(5, fsops_read(&mfs_io, "/home/docs/b.txt", buf, 64));
    TEST_ASSERT_EQUAL(FSOPS_ERR_NOTEMPTY, fsops_rmdir(&mfs_io, "/home/docs"));
    TEST_ASSERT_EQUAL(FSOPS_ERR_ISDIR, fsops_rm(&mfs_io, "/home/docs"));
    TEST_ASSERT_EQUAL(FSOPS_ERR_NOTDIR, fsops_rmdir(&mfs_io, "/home/docs/b.txt"));
    TEST_ASSERT_EQUAL(0, fsops_rm(&mfs_io, "/home/docs/b.txt"));
    TEST_ASSERT_EQUAL(0, fsops_rmdir(&mfs_io, "/home/docs"));
    TEST_ASSERT(mfs_find("/home/docs") < 0);
    TEST_ASSERT_EQUAL(FSOPS_ERR_NAME, fsops_mv(&mfs_io, "/home/notes.txt", "/home/..", d, 128));
}

static void test_copy_into_dir_and_large_file(void) {
    static char big[6000];
    static char back[8192];
    char d[128];
    int i;
    mfs_reset();
    for (i = 0; i < 6000; i++) big[i] = (char)('a' + i % 26);
    mfs_add("/home/big.txt", 0, big, 6000);
    mfs_add("/home/dst", 1, 0, 0);
    /* the old shell cp truncated files to 256 bytes */
    TEST_ASSERT_EQUAL(0, fsops_cp(&mfs_io, "/home/big.txt", "/home/dst", d, 128));
    TEST_ASSERT_EQUAL_STRING("/home/dst/big.txt", d);
    TEST_ASSERT_EQUAL(6000, fsops_read(&mfs_io, d, back, 8192));
    TEST_ASSERT(memcmp(big, back, 6000) == 0);
    TEST_ASSERT_EQUAL(FSOPS_ERR_EXISTS, fsops_cp(&mfs_io, "/home/big.txt", "/home/big.txt", d, 128));
    TEST_ASSERT_EQUAL(FSOPS_ERR_TOOBIG, fsops_read(&mfs_io, "/home/big.txt", back, 100));
    TEST_ASSERT_EQUAL(0, fsops_mv(&mfs_io, "/home/notes.txt", "/home/dst", d, 128));
    TEST_ASSERT_EQUAL_STRING("/home/dst/notes.txt", d);
}

static void test_read_only_initrd_errors(void) {
    char d[128];
    mfs_reset();
    TEST_ASSERT_EQUAL(FSOPS_ERR_RDONLY, fsops_mkdir(&mfs_io, "/ro/x"));
    TEST_ASSERT_EQUAL(FSOPS_ERR_RDONLY, fsops_rm(&mfs_io, "/ro/readme.txt"));
    TEST_ASSERT_EQUAL(FSOPS_ERR_RDONLY, fsops_write(&mfs_io, "/ro/readme.txt", "x", 1));
    TEST_ASSERT_EQUAL(FSOPS_ERR_RDONLY, fsops_mv(&mfs_io, "/ro/readme.txt", "/home/r.txt", d, 128));
    TEST_ASSERT_EQUAL(0, fsops_cp(&mfs_io, "/ro/readme.txt", "/home", d, 128));   /* copy out is allowed */
    TEST_ASSERT_EQUAL_STRING("protege (initrd)", fsops_strerror(FSOPS_ERR_RDONLY));
    TEST_ASSERT_EQUAL_STRING("existe deja", fsops_strerror(-2));
}

/* UX5: a store that keeps only a prefix (overlay node, 384 bytes) must not
 * look like a successful save. */
static int short_write(const char *p, const char *b, int n) { (void)p; (void)b; return n > 384 ? 384 : n; }
static void test_short_write_is_an_error(void) {
    fsops_io_t io = mfs_io;
    static char big[1000];
    mfs_reset();
    io.writefile = short_write;
    TEST_ASSERT_EQUAL(0, fsops_write(&io, "/home/s.txt", big, 384));
    TEST_ASSERT_EQUAL(FSOPS_ERR_TOOBIG, fsops_write(&io, "/home/s.txt", big, 1000));
    TEST_ASSERT_EQUAL_STRING("fichier trop grand", fsops_strerror(FSOPS_ERR_TOOBIG));
}

static char g_hits[512];
static void on_hit(void *ctx, const char *path, int line, const char *text, int len) {
    char b[160];
    (*(int *)ctx)++;
    snprintf(b, sizeof(b), "%s:%d:%.*s|", path, line, len, text);
    strcat(g_hits, b);
}

static void test_grep_recursive(void) {
    static char big[FSOPS_FILE_MAX + 100];
    int calls = 0, files = -1, cut = -1, n;
    mfs_reset();
    mfs_add("/home/src", 1, 0, 0);
    mfs_add("/home/src/a.c", 0, "int x;\nTODO fix\nend\n", 21);
    mfs_add("/home/src/deep", 1, 0, 0);
    mfs_add("/home/src/deep/b.txt", 0, "nothing\nTODO two", 16);
    memset(big, 'z', sizeof(big)); memcpy(big + sizeof(big) - 4, "TODO", 4);
    mfs_add("/home/big.txt", 0, big, (int)sizeof(big));
    g_hits[0] = 0;
    n = fsops_grep(&mfs_io, "/home", "TODO", 10, on_hit, &calls, &files, &cut);
    TEST_ASSERT_EQUAL(2, n);
    TEST_ASSERT_EQUAL(2, calls);
    TEST_ASSERT(strstr(g_hits, "/home/src/a.c:2:TODO fix|") != 0);
    TEST_ASSERT(strstr(g_hits, "/home/src/deep/b.txt:2:TODO two|") != 0);
    TEST_ASSERT(files >= 4);
    TEST_ASSERT_EQUAL(1, cut);               /* big.txt: only 8 KiB searched */
    /* max_hits stops early, a file argument searches that file */
    calls = 0;
    TEST_ASSERT_EQUAL(1, fsops_grep(&mfs_io, "/home", "TODO", 1, on_hit, &calls, 0, 0));
    TEST_ASSERT_EQUAL(1, fsops_grep(&mfs_io, "/home/src/a.c", "fix", 5, 0, 0, 0, 0));
    TEST_ASSERT_EQUAL(0, fsops_grep(&mfs_io, "/home", "absent", 5, 0, 0, 0, 0));
    TEST_ASSERT(fsops_grep(&mfs_io, "/nope", "x", 5, 0, 0, 0, 0) < 0);
}

int main(void) {
    unity_init();
    RUN_TEST(test_paths);
    RUN_TEST(test_list_sorted_dirs_first);
    RUN_TEST(test_create_rename_delete);
    RUN_TEST(test_copy_into_dir_and_large_file);
    RUN_TEST(test_read_only_initrd_errors);
    RUN_TEST(test_short_write_is_an_error);
    RUN_TEST(test_grep_recursive);
    unity_print_results();
    unity_cleanup();
    return (unity_stats.tests_failed == 0) ? 0 : 1;
}
