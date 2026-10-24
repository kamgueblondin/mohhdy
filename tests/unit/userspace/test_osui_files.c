/* Desktop file explorer model over the shared file library (mock VFS). */
#include "../../framework/unity.h"
#include "mockfs.h"
#include "osui_files.h"

static osui_files_t F;
static osui_app_view_t V;

static void type(const char *s) { while (*s) osui_files_key(&F, *s++); }

static int row_has(const char *needle) {
    int r;
    osui_files_render(&F, &V);
    for (r = 0; r < OSUI_APP_ROWS; r++) if (strstr(V.rows[r], needle)) return r;
    return -1;
}

static void test_browse_and_breadcrumbs(void) {
    mfs_reset();
    mfs_add("/home/docs", 1, 0, 0);
    osui_files_open(&F, &mfs_io, "/");
    TEST_ASSERT_EQUAL_STRING("/", F.cwd);
    TEST_ASSERT_EQUAL(2, F.n);                       /* home, ro: dirs */
    TEST_ASSERT_EQUAL_STRING("home", F.ent[0].name);
    osui_files_key(&F, '\n');                        /* open home */
    TEST_ASSERT_EQUAL_STRING("/home", F.cwd);
    TEST_ASSERT_EQUAL_STRING("docs", F.ent[0].name);  /* dirs first */
    osui_files_key(&F, '\n');
    TEST_ASSERT_EQUAL_STRING("/home/docs", F.cwd);
    osui_files_render(&F, &V);
    TEST_ASSERT_EQUAL_STRING("/ > home > docs ", V.rows[0]);
    TEST_ASSERT(strstr(V.title, "/home/docs") != 0);
    osui_files_click(&F, 0, 5, 0);                   /* "home" segment */
    TEST_ASSERT_EQUAL_STRING("/home", F.cwd);
    osui_files_key(&F, '\b');                        /* parent */
    TEST_ASSERT_EQUAL_STRING("/", F.cwd);
    osui_files_click(&F, 0, 0, 0);
    TEST_ASSERT_EQUAL_STRING("/", F.cwd);
}

static void test_keyboard_selection_and_open_file(void) {
    mfs_reset();
    osui_files_open(&F, &mfs_io, "/home");
    TEST_ASSERT_EQUAL(1, F.n);
    osui_files_key(&F, 31);
    osui_files_key(&F, 31);
    TEST_ASSERT_EQUAL(0, F.sel);                     /* clamped */
    TEST_ASSERT_EQUAL(OSUI_FILES_OPEN, osui_files_key(&F, '\n'));
    TEST_ASSERT_EQUAL_STRING("/home/notes.txt", F.open_path);
    TEST_ASSERT(row_has("notes.txt") == 2);
    TEST_ASSERT_EQUAL(OSUI_APP_A_SEL, V.attr[2][0]);
    TEST_ASSERT(strstr(V.rows[2], "16") != 0);       /* size */
    /* double click opens too */
    TEST_ASSERT_EQUAL(OSUI_FILES_NONE, osui_files_click(&F, 2, 4, 0));
    TEST_ASSERT_EQUAL(OSUI_FILES_OPEN, osui_files_click(&F, 2, 4, 1));
    TEST_ASSERT_EQUAL(OSUI_FILES_CLOSE, osui_files_key(&F, 27));
}

static void test_create_rename_copy_move_delete(void) {
    mfs_reset();
    osui_files_open(&F, &mfs_io, "/home");
    osui_files_key(&F, 'd'); type("docs"); osui_files_key(&F, '\n');
    TEST_ASSERT(mfs_find("/home/docs") >= 0);
    TEST_ASSERT_EQUAL_STRING("osui files mkdir ok /home/docs", F.log);
    TEST_ASSERT_EQUAL_STRING("docs", F.ent[F.sel].name);
    osui_files_key(&F, 'n'); type("a.txt"); osui_files_key(&F, '\n');
    TEST_ASSERT(mfs_find("/home/a.txt") >= 0);
    TEST_ASSERT_EQUAL_STRING("a.txt", F.ent[F.sel].name);
    /* existing name */
    osui_files_key(&F, 'n'); type("a.txt"); osui_files_key(&F, '\n');
    TEST_ASSERT(strstr(F.status, "existe deja") != 0);
    /* rename: the prompt starts with the current name */
    osui_files_key(&F, 'r');
    TEST_ASSERT_EQUAL_STRING("a.txt", F.input);
    osui_files_key(&F, '\b'); osui_files_key(&F, '\b'); osui_files_key(&F, '\b'); osui_files_key(&F, '\b');
    type("b.md"); osui_files_key(&F, '\n');
    TEST_ASSERT(mfs_find("/home/ab.md") >= 0);
    TEST_ASSERT_EQUAL_STRING("ab.md", F.ent[F.sel].name);
    /* copy into docs */
    osui_files_tool(&F, OSUI_FT_COPY);
    F.ilen = 0; F.input[0] = 0; type("docs"); osui_files_key(&F, '\n');
    TEST_ASSERT(mfs_find("/home/docs/ab.md") >= 0);
    /* move notes into docs */
    { int i; for (i = 0; i < F.n; i++) if (!strcmp(F.ent[i].name, "notes.txt")) F.sel = i; }
    osui_files_key(&F, 'm'); F.ilen = 0; F.input[0] = 0; type("/home/docs"); osui_files_key(&F, '\n');
    TEST_ASSERT(mfs_find("/home/docs/notes.txt") >= 0);
    TEST_ASSERT(mfs_find("/home/notes.txt") < 0);
    /* delete asks first; any key but o cancels */
    { int i; for (i = 0; i < F.n; i++) if (!strcmp(F.ent[i].name, "ab.md")) F.sel = i; }
    osui_files_key(&F, 0x14);
    TEST_ASSERT_EQUAL(OSUI_FILES_CONFIRM, F.mode);
    TEST_ASSERT(row_has("ab.md") >= 2);
    TEST_ASSERT(strstr(V.status, "Supprimer ab.md ?") != 0);
    osui_files_key(&F, 'n');
    TEST_ASSERT(mfs_find("/home/ab.md") >= 0);
    osui_files_key(&F, 0x14); osui_files_key(&F, 'o');
    TEST_ASSERT(mfs_find("/home/ab.md") < 0);
    TEST_ASSERT_EQUAL_STRING("osui files rm ok /home/ab.md", F.log);
    /* non-empty folder: error shown, nothing removed */
    { int i; for (i = 0; i < F.n; i++) if (!strcmp(F.ent[i].name, "docs")) F.sel = i; }
    osui_files_key(&F, 0x14); osui_files_key(&F, 'o');
    TEST_ASSERT(strstr(F.status, "repertoire non vide") != 0);
    TEST_ASSERT(mfs_find("/home/docs") >= 0);
}

static void test_read_only_and_info(void) {
    mfs_reset();
    osui_files_open(&F, &mfs_io, "/ro");
    osui_files_key(&F, 'd'); type("x"); osui_files_key(&F, '\n');
    TEST_ASSERT(strstr(F.status, "protege (initrd)") != 0);
    TEST_ASSERT_EQUAL_STRING("osui files mkdir error=protege (initrd) /ro/x", F.log);
    osui_files_key(&F, 0x14); osui_files_key(&F, 'o');
    TEST_ASSERT(mfs_find("/ro/readme.txt") >= 0);
    osui_files_key(&F, 'i');
    TEST_ASSERT_EQUAL_STRING("/ro/readme.txt  fichier 14 octets", F.status);
    /* prompt Escape cancels */
    osui_files_key(&F, 'n'); type("zz"); osui_files_key(&F, 27);
    TEST_ASSERT_EQUAL(OSUI_FILES_BROWSE, F.mode);
    TEST_ASSERT(mfs_find("/ro/zz") < 0);
}

static void test_toolbar_hit(void) {
    TEST_ASSERT_EQUAL(0, osui_app_tool_hit(osui_files_toolbar, 1));
    TEST_ASSERT_EQUAL(-1, osui_app_tool_hit(osui_files_toolbar, 6));
    TEST_ASSERT_EQUAL(1, osui_app_tool_hit(osui_files_toolbar, 10));
    TEST_ASSERT_EQUAL(OSUI_FT_INFO, osui_app_tool_hit(osui_files_toolbar, (int)(strstr(osui_files_toolbar, "Infos") - osui_files_toolbar)));
}

int main(void) {
    unity_init();
    RUN_TEST(test_browse_and_breadcrumbs);
    RUN_TEST(test_keyboard_selection_and_open_file);
    RUN_TEST(test_create_rename_copy_move_delete);
    RUN_TEST(test_read_only_and_info);
    RUN_TEST(test_toolbar_hit);
    unity_print_results();
    unity_cleanup();
    return (unity_stats.tests_failed == 0) ? 0 : 1;
}
