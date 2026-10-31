/* Live window content: every line comes from the injected sources. */
#include "../../framework/unity.h"
#include "osui_panes.h"
#include <string.h>

static int f_listdir(const char *p, os_dirent_t *o, int m) {
    (void)m;
    if (strcmp(p, "/") != 0 && strcmp(p, "/web") != 0) return -1;
    strcpy(o[0].name, "bin"); o[0].flags = OS_DIRENT_DIR; o[0].size = 0;
    strcpy(o[1].name, "readme.txt"); o[1].flags = OS_DIRENT_FILE; o[1].size = 1234;
    return 2;
}
static int f_ps(os_proc_t *o, int m) {
    (void)m;
    memset(o, 0, sizeof(*o) * 2);
    o[0].pid = 1; o[0].type = OS_TASK_USER; strcpy(o[0].name, "shell");
    o[1].pid = 7; o[1].type = 0; strcpy(o[1].name, "idle");
    return 2;
}
static int f_mem(os_meminfo_t *mi) { mi->total_pages = 1000; mi->used_pages = 250; mi->free_pages = 750; return 0; }
static int f_mem_fail(os_meminfo_t *mi) { (void)mi; return -1; }
static unsigned f_ticks(void) { return 100 * 3723 + 5; }
static int g_vfs_down;
static int f_svc(const char *n, os_service_status_t *st) {
    if (strcmp(n, "vfs") == 0 && !g_vfs_down) { st->owner_pid = 3; st->queued_messages = 2; return 0; }
    return -2;
}
static char L[14][OSUI_PANES_LINE];
static char g_ran[32];
static int f_run(const char *cmd, char *out, int max) {
    const char *t = "p2p stats up=0\npeers 0\n";
    int i;
    strncpy(g_ran, cmd, sizeof(g_ran) - 1);
    for (i = 0; t[i] && i < max - 1; i++) out[i] = t[i];
    out[i] = 0;
    return i;
}
static int f_run_fail(const char *cmd, char *out, int max) { (void)cmd; (void)out; (void)max; return -1; }

/* UX3: P2P / Collab / Prod / Support show the captured output of the real
 * shell command; Browser lists the local pages of web-render. */
static void test_command_panes_and_browser(void) {
    osui_pane_src_t s = { f_listdir, f_ps, f_mem, f_ticks, f_svc, f_run };
    int n = osui_panes_fill(OS_FB_PANE_P2P, &s, L, 14);
    TEST_ASSERT_EQUAL_STRING("p2p-stats", g_ran);
    TEST_ASSERT_EQUAL(3, n);
    TEST_ASSERT_EQUAL_STRING("$ p2p-stats  (sortie du shell)", L[0]);
    TEST_ASSERT_EQUAL_STRING("p2p stats up=0", L[1]);
    TEST_ASSERT_EQUAL_STRING("peers 0", L[2]);
    osui_panes_fill(OS_FB_PANE_COLLAB, &s, L, 14);
    TEST_ASSERT_EQUAL_STRING("collab-balances", g_ran);
    osui_panes_fill(OS_FB_PANE_PROD, &s, L, 14);
    TEST_ASSERT_EQUAL_STRING("prod-metrics", g_ran);
    osui_panes_fill(OS_FB_PANE_SUPPORT, &s, L, 14);
    TEST_ASSERT_EQUAL_STRING("prod-diag", g_ran);
    TEST_ASSERT(osui_panes_command(OS_FB_PANE_STATUS) == 0);
    s.run = f_run_fail;
    n = osui_panes_fill(OS_FB_PANE_P2P, &s, L, 14);
    TEST_ASSERT_EQUAL(2, n);
    TEST_ASSERT_EQUAL_STRING("  indisponible", L[1]);
    s.run = 0;
    n = osui_panes_fill(OS_FB_PANE_PROD, &s, L, 14);
    TEST_ASSERT_EQUAL_STRING("  indisponible", L[1]);
    /* browser: files of /web (the mock lists bin/ and readme.txt) */
    n = osui_panes_fill(OS_FB_PANE_BROWSER, &s, L, 14);
    TEST_ASSERT_EQUAL(2, n);
    TEST_ASSERT_EQUAL_STRING("Barre d'adresse en haut : URL ou mots. Pages /web :", L[0]);
    TEST_ASSERT_EQUAL_STRING("  /web/readme.txt  1234 o", L[1]);
}

static void test_status_from_sources(void) {
    osui_pane_src_t s = { f_listdir, f_ps, f_mem, f_ticks, f_svc, 0 };
    int n = osui_panes_fill(OS_FB_PANE_STATUS, &s, L, 14);
    TEST_ASSERT_EQUAL(5, n);
    TEST_ASSERT_EQUAL_STRING("uptime 1:02:03  (PIT ticks 372305)", L[0]);
    TEST_ASSERT_EQUAL_STRING("memoire pages: 250 utilisees / 1000 (750 libres, 4 KB)", L[1]);
    TEST_ASSERT_EQUAL_STRING("taches noyau: 2 (ps)", L[2]);
    TEST_ASSERT_EQUAL_STRING("  pid 1 user shell", L[3]);
    TEST_ASSERT_EQUAL_STRING("  pid 7 kern idle", L[4]);
    s.meminfo = f_mem_fail;
    osui_panes_fill(OS_FB_PANE_STATUS, &s, L, 14);
    TEST_ASSERT_EQUAL_STRING("memoire: indisponible", L[1]);
}

static void test_fs_listing(void) {
    osui_pane_src_t s = { f_listdir, f_ps, f_mem, f_ticks, f_svc, 0 };
    int n = osui_panes_fill(OS_FB_PANE_FS, &s, L, 14);
    TEST_ASSERT_EQUAL(3, n);
    TEST_ASSERT_EQUAL_STRING("ls /  (2 entrees, sys_listdir)", L[0]);
    TEST_ASSERT_EQUAL_STRING("  d  bin/", L[1]);
    TEST_ASSERT_EQUAL_STRING("  -  readme.txt  1234", L[2]);
    s.listdir = 0;
    n = osui_panes_fill(OS_FB_PANE_FS, &s, L, 14);
    TEST_ASSERT_EQUAL(1, n);
    TEST_ASSERT_EQUAL_STRING("ls /: indisponible", L[0]);
}

static void test_admin_services(void) {
    osui_pane_src_t s = { f_listdir, f_ps, f_mem, f_ticks, f_svc, 0 };
    int n = osui_panes_fill(OS_FB_PANE_ADMIN, &s, L, 14);
    TEST_ASSERT_EQUAL(1 + osui_panes_service_count, n);
    TEST_ASSERT_EQUAL_STRING("services (service-status): 1 actifs / 9", L[0]);
    TEST_ASSERT_EQUAL_STRING("  vfs: actif pid 3 file 2", L[1]);
    TEST_ASSERT_EQUAL_STRING("  ai-engine: absent", L[2]);
    {
        /* vfs not published: on-demand server, not "absent" */
        g_vfs_down = 1;
        n = osui_panes_fill(OS_FB_PANE_ADMIN, &s, L, 14);
        g_vfs_down = 0;
        TEST_ASSERT(n > 1);
        TEST_ASSERT_EQUAL_STRING("services (service-status): 0 actifs / 9", L[0]);
        TEST_ASSERT_EQUAL_STRING("  vfs: a la demande (non resident, fichiers via le noyau)", L[1]);
    }
    /* Browser has live content since UX3 (local pages, see below); a pane
     * without content still fills nothing */
    TEST_ASSERT_EQUAL(0, osui_panes_fill(OS_FB_PANE_NONE, &s, L, 14));
    TEST_ASSERT_EQUAL(0, osui_panes_fill(OS_FB_PANE_ADMIN, 0, L, 14));
}

int main(void) {
    unity_init();
    RUN_TEST(test_status_from_sources);
    RUN_TEST(test_fs_listing);
    RUN_TEST(test_admin_services);
    RUN_TEST(test_command_panes_and_browser);
    unity_print_results();
    unity_cleanup();
    return (unity_stats.tests_failed == 0) ? 0 : 1;
}
