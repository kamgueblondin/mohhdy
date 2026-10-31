/* test_osui_parity.c - UX5 GUI parity: the family panels of
 * osui_parity.c and the registry rights model of the GUI tools
 * (osui_tool_cmd_check), over an injected shell. */
#include "../../framework/unity.h"
#include "../../framework/test_kernel.h"
#include "osui_tools.h"
#include "osui_parity.h"
#include "mohhdy_osui_bridge.h"
#include "os_syscalls.h"
#include <string.h>
#include <stdio.h>

#define K_UP 30
#define K_DOWN 31
#define K_ESC 27

static char g_cmds[4096];
static int g_runs;

static int reg_known(const char *name) {
    int i;
    for (i = 0; i < MOHHDY_SHELL_COMMAND_COUNT; i++) if (!strcmp(mohhdy_shell_commands[i], name)) return 1;
    return 0;
}

/* the shell: same check as shell_app_run, then a canned answer */
static int fake_run(const char *cmd, char *out, int max) {
    const char *r = "ok\n";
    if (osui_tool_cmd_check(cmd, reg_known) != OSUI_CMD_OK) return -1;
    g_runs++;
    strcat(g_cmds, cmd); strcat(g_cmds, "|");
    if (!strcmp(cmd, "net-status")) r = "=== Reseau bare-metal ===\nnet ok ip 10.0.2.15\n";
    else if (!strcmp(cmd, "ata-status")) r = "ata-status ok driver 2\n";
    else if (!strncmp(cmd, "grep -r ", 8)) r = "/samples/long.txt:3: ligne 3\ngrep ok 1\n";
    else if (!strcmp(cmd, "uptime")) r = "uptime 42 s\n";
    else if (!strncmp(cmd, "mkdir ", 6)) r = "mkdir ok\n";
    strncpy(out, r, (size_t)max - 1); out[max - 1] = 0;
    return (int)strlen(out);
}

static osui_tool_src_t k_src;
static osui_tool_t t;
static osui_app_view_t v;

static int has_line(const char *s) {
    int i;
    for (i = 0; i < t.n; i++) if (strstr(t.lines[i], s)) return 1;
    return 0;
}

static void setup_src(void) {
    memset(&k_src, 0, sizeof(k_src));
    k_src.run = fake_run;
    k_src.known = reg_known;
    g_cmds[0] = 0; g_runs = 0;
}

static void test_rights_model(void) {
    /* any registry command, with its arguments, outside the old allowlist */
    TEST_ASSERT_EQUAL(OSUI_CMD_OK, osui_tool_cmd_check("ata-status", reg_known));
    TEST_ASSERT_EQUAL(OSUI_CMD_OK, osui_tool_cmd_check("ls /samples", reg_known));
    TEST_ASSERT_EQUAL(OSUI_CMD_OK, osui_tool_cmd_check("prod-diag", reg_known));
    TEST_ASSERT_EQUAL(OSUI_CMD_OK, osui_tool_cmd_check("osui-audit", reg_known));
    TEST_ASSERT(!osui_tool_cmd_allowed("ata-status"));
    /* refused */
    TEST_ASSERT_EQUAL(OSUI_CMD_EMPTY, osui_tool_cmd_check("  ", reg_known));
    TEST_ASSERT_EQUAL(OSUI_CMD_META, osui_tool_cmd_check("ls; rm /x", reg_known));
    TEST_ASSERT_EQUAL(OSUI_CMD_META, osui_tool_cmd_check("cat a > b", reg_known));
    TEST_ASSERT_EQUAL(OSUI_CMD_META, osui_tool_cmd_check("echo $X", reg_known));
    TEST_ASSERT_EQUAL(OSUI_CMD_UNKNOWN, osui_tool_cmd_check("no-such-cmd", reg_known));
    TEST_ASSERT_EQUAL(OSUI_CMD_UNKNOWN, osui_tool_cmd_check("psx", reg_known));
    TEST_ASSERT_EQUAL(OSUI_CMD_TERMINAL, osui_tool_cmd_check("gui", reg_known));
    TEST_ASSERT_EQUAL(OSUI_CMD_TERMINAL, osui_tool_cmd_check("top", reg_known));
    TEST_ASSERT_EQUAL(OSUI_CMD_TERMINAL, osui_tool_cmd_check("pm-ide a.pm", reg_known));
    TEST_ASSERT_EQUAL(OSUI_CMD_TERMINAL, osui_tool_cmd_check("exit", reg_known));
    TEST_ASSERT_EQUAL(OSUI_CMD_TERMINAL, osui_tool_cmd_check("reboot", reg_known));
    TEST_ASSERT_EQUAL(OSUI_CMD_TERMINAL, osui_tool_cmd_check("osui-start", reg_known));
    /* without a registry the old allowlist stays */
    TEST_ASSERT_EQUAL(OSUI_CMD_OK, osui_tool_cmd_check("ps", 0));
    TEST_ASSERT_EQUAL(OSUI_CMD_UNKNOWN, osui_tool_cmd_check("ata-status", 0));
    TEST_ASSERT(strstr(osui_tool_cmd_reason(OSUI_CMD_TERMINAL), "terminal") != 0);
}

static void test_table_matches_registry(void) {
    int i, k, total = 0;
    char name[40];
    TEST_ASSERT_EQUAL(27, osui_par_family_count);
    for (i = 0; i < osui_par_family_count; i++) {
        const osui_par_family_t *f = &osui_par_families[i];
        TEST_ASSERT(f->kind >= OS_FB_APP_NET && f->kind <= OS_FB_APP_KIND_MAX);
        TEST_ASSERT(osui_par_family(f->kind) == f);
        TEST_ASSERT_EQUAL(f->kind, osui_par_kind_of(f->slash));
        TEST_ASSERT(osui_tool_is_panel(f->kind));
        for (k = 0; k < f->count; k++) {
            const osui_par_entry_t *e = &f->entries[k];
            osui_par_cmd_name(e, name, (int)sizeof(name));
            TEST_ASSERT(reg_known(name));
            /* reachable through the GUI rights model */
            TEST_ASSERT_EQUAL(OSUI_CMD_OK, osui_tool_cmd_check(e->cmd, reg_known));
            if (e->flags & OSUI_PAR_ARG) TEST_ASSERT(e->hint && e->hint[0]);
            total++;
        }
    }
    TEST_ASSERT(total >= 50);
    TEST_ASSERT_EQUAL(0, osui_par_kind_of("/nope"));
    /* existing desktop pane commands keep their meaning */
    TEST_ASSERT_EQUAL(0, osui_par_kind_of("/prod"));
    TEST_ASSERT_EQUAL(0, osui_par_kind_of("/collab"));
    TEST_ASSERT_EQUAL(0, osui_par_kind_of("/status"));
    TEST_ASSERT_EQUAL(0, osui_par_kind_of("/admin"));
    TEST_ASSERT(!osui_tool_is_panel(OS_FB_APP_TASKS));
}

static void test_build(void) {
    const osui_par_entry_t *ls = &osui_par_family(OS_FB_APP_STORE)->entries[3];
    char out[64];
    TEST_ASSERT_EQUAL_STRING("ls", ls->cmd);
    TEST_ASSERT_EQUAL(-1, osui_par_build(ls, "", out, (int)sizeof(out)));
    TEST_ASSERT_EQUAL(0, osui_par_build(ls, "/samples", out, (int)sizeof(out)));
    TEST_ASSERT_EQUAL_STRING("ls /samples", out);
    TEST_ASSERT_EQUAL(0, osui_par_build(&osui_par_family(OS_FB_APP_STORE)->entries[0], "ignored", out, (int)sizeof(out)));
    TEST_ASSERT_EQUAL_STRING("ata-status", out);
}

/* Lot 46: panel entries added for full parity; deploy-verify takes its
 * manifest (without it the shell always answered "manifest not found") */
static const osui_par_entry_t *entry_of(int kind, const char *cmd) {
    const osui_par_family_t *f = osui_par_family(kind);
    int i;
    for (i = 0; f && i < f->count; i++) if (!strcmp(f->entries[i].cmd, cmd)) return &f->entries[i];
    return 0;
}

static void test_lot46_entries(void) {
    char out[96];
    const osui_par_entry_t *e = entry_of(OS_FB_APP_PROD, "deploy-verify");
    TEST_ASSERT_NOT_NULL(e);
    TEST_ASSERT(e->flags & OSUI_PAR_ARG);
    TEST_ASSERT_EQUAL(-1, osui_par_build(e, "", out, (int)sizeof(out)));
    TEST_ASSERT_EQUAL(0, osui_par_build(e, "/l46.man", out, (int)sizeof(out)));
    TEST_ASSERT_EQUAL_STRING("deploy-verify /l46.man", out);
    /* state changes are confirmed (second Enter), reads are not */
    TEST_ASSERT(entry_of(OS_FB_APP_PROD, "deploy-make")->flags & OSUI_PAR_MUT);
    TEST_ASSERT(entry_of(OS_FB_APP_PROD, "prod-backup")->flags & OSUI_PAR_MUT);
    TEST_ASSERT(entry_of(OS_FB_APP_STORE, "persist-save")->flags & OSUI_PAR_MUT);
    TEST_ASSERT_EQUAL(0, entry_of(OS_FB_APP_STORE, "fat16-list")->flags);
    TEST_ASSERT(entry_of(OS_FB_APP_LOGS, "notify-push")->flags & OSUI_PAR_MUT);
    TEST_ASSERT(entry_of(OS_FB_APP_LOGS, "notify-ack")->flags & OSUI_PAR_MUT);
    TEST_ASSERT(entry_of(OS_FB_APP_SECURITY, "pm-ca-revoke")->flags & OSUI_PAR_MUT);
    TEST_ASSERT_EQUAL(OSUI_PAR_ARG, entry_of(OS_FB_APP_DEVTOOLS, "web-inspect")->flags);
    TEST_ASSERT_EQUAL(OSUI_PAR_ARG, entry_of(OS_FB_APP_DEVTOOLS, "web-a11y")->flags);
    TEST_ASSERT(entry_of(OS_FB_APP_NOTES, "migrate-import")->flags & OSUI_PAR_MUT);
}

/* Lot 50: /pm /proc /mesh are menus of existing shell commands.
 * /pm must not be the /pmrun launcher. /p2p stays the desktop pane.
 * .jpg opens in the shared viewer. */
static void test_lot50_entries(void) {
    char out[96];
    const osui_par_entry_t *e;
    TEST_ASSERT_EQUAL(OS_FB_APP_PM, osui_par_kind_of("/pm"));
    TEST_ASSERT_EQUAL(0, osui_par_kind_of("/pmrun"));
    TEST_ASSERT_EQUAL(OS_FB_APP_PROC, osui_par_kind_of("/proc"));
    TEST_ASSERT_EQUAL(OS_FB_APP_P2PMESH, osui_par_kind_of("/mesh"));
    /* /p2p is the existing desktop pane, not this menu */
    TEST_ASSERT_EQUAL(0, osui_par_kind_of("/p2p"));
    TEST_ASSERT_EQUAL(0, osui_par_kind_of("/peers"));
    e = entry_of(OS_FB_APP_PM, "pm-catalog");
    TEST_ASSERT_NOT_NULL(e);
    TEST_ASSERT_EQUAL(0, e->flags);
    e = entry_of(OS_FB_APP_PM, "pm-doc");
    TEST_ASSERT(e->flags & OSUI_PAR_ARG);
    TEST_ASSERT(e->hint && e->hint[0]);
    TEST_ASSERT_EQUAL(-1, osui_par_build(e, "", out, (int)sizeof(out)));
    TEST_ASSERT_EQUAL(0, osui_par_build(e, "/pm/hello.pm", out, (int)sizeof(out)));
    TEST_ASSERT_EQUAL_STRING("pm-doc /pm/hello.pm", out);
    e = entry_of(OS_FB_APP_PM, "pm-cross");
    TEST_ASSERT(e->flags & OSUI_PAR_ARG);
    TEST_ASSERT(strstr(e->hint, "py") != 0);
    TEST_ASSERT_EQUAL(OSUI_CMD_TERMINAL, osui_tool_cmd_check("pm-ide a.pm", reg_known));
    e = entry_of(OS_FB_APP_PROC, "task-priority");
    TEST_ASSERT_EQUAL(OSUI_PAR_ARG | OSUI_PAR_MUT, e->flags);
    TEST_ASSERT_EQUAL(0, osui_par_build(e, "1 2", out, (int)sizeof(out)));
    TEST_ASSERT_EQUAL_STRING("task-priority 1 2", out);
    TEST_ASSERT_EQUAL(0, entry_of(OS_FB_APP_PROC, "ps")->flags);
    TEST_ASSERT_EQUAL(0, entry_of(OS_FB_APP_PROC, "task-priority-child-status")->flags);
    TEST_ASSERT_EQUAL(0, entry_of(OS_FB_APP_P2PMESH, "p2p-stats")->flags);
    TEST_ASSERT_EQUAL(0, entry_of(OS_FB_APP_P2PMESH, "p2p-kv")->flags);
    e = entry_of(OS_FB_APP_P2PMESH, "p2p-cache");
    TEST_ASSERT_NOT_NULL(e);
    TEST_ASSERT_EQUAL(0, e->flags);
    /* the write form is the later entry of the same command */
    {
        int i, writes = 0;
        const osui_par_family_t *f = osui_par_family(OS_FB_APP_P2PMESH);
        for (i = 0; i < f->count; i++) {
            if (!strcmp(f->entries[i].cmd, "p2p-cache") && (f->entries[i].flags & OSUI_PAR_MUT)) {
                writes++;
                TEST_ASSERT_EQUAL(0, osui_par_build(&f->entries[i], "put k v", out, (int)sizeof(out)));
                TEST_ASSERT_EQUAL_STRING("p2p-cache put k v", out);
            }
        }
        TEST_ASSERT_EQUAL(1, writes);
    }
    TEST_ASSERT(osui_path_is_image("/images/mire-prog.jpg"));
    TEST_ASSERT(osui_path_is_image("/samples/mire.png"));
    TEST_ASSERT(osui_path_is_image("/samples/mire24.bmp"));
    TEST_ASSERT(osui_path_is_image("a.pgm"));
    TEST_ASSERT(osui_path_is_image("a.ppm"));
    TEST_ASSERT(osui_path_is_image("a.pnm"));
    TEST_ASSERT(!osui_path_is_image("/docs/readme.txt"));
    TEST_ASSERT(!osui_path_is_image("/images/mire.jpeg"));
    TEST_ASSERT(!osui_path_is_image(0));
}

/* Lot 51: /child /release /keys. Existing pane slashes stay free. */
static void test_lot51_entries(void) {
    char out[128];
    const osui_par_entry_t *e;
    TEST_ASSERT_EQUAL(OS_FB_APP_CHILD, osui_par_kind_of("/child"));
    TEST_ASSERT_EQUAL(OS_FB_APP_RELEASE, osui_par_kind_of("/release"));
    TEST_ASSERT_EQUAL(OS_FB_APP_KEYS, osui_par_kind_of("/keys"));
    TEST_ASSERT_EQUAL(0, osui_par_kind_of("/p2p"));
    TEST_ASSERT_EQUAL(0, osui_par_kind_of("/collab"));
    TEST_ASSERT_EQUAL(0, osui_par_kind_of("/prod"));
    TEST_ASSERT_EQUAL(OS_FB_APP_PROD, osui_par_kind_of("/ci"));
    TEST_ASSERT_EQUAL(0, entry_of(OS_FB_APP_CHILD, "child-exit-count")->flags);
    e = entry_of(OS_FB_APP_CHILD, "child-result");
    TEST_ASSERT_EQUAL(OSUI_PAR_ARG, e->flags);
    TEST_ASSERT_EQUAL(0, osui_par_build(e, "4", out, (int)sizeof(out)));
    TEST_ASSERT_EQUAL_STRING("child-result 4", out);
    TEST_ASSERT_EQUAL(OSUI_PAR_MUT, entry_of(OS_FB_APP_CHILD, "child-results-clear")->flags);
    TEST_ASSERT_EQUAL(0, entry_of(OS_FB_APP_RELEASE, "deploy-verify")->flags);
    e = entry_of(OS_FB_APP_RELEASE, "deploy-apply");
    TEST_ASSERT_EQUAL(OSUI_PAR_ARG | OSUI_PAR_MUT, e->flags);
    TEST_ASSERT_EQUAL(0, osui_par_build(e, "/deploy.man", out, (int)sizeof(out)));
    TEST_ASSERT_EQUAL_STRING("deploy-apply /deploy.man", out);
    TEST_ASSERT_EQUAL(OSUI_PAR_MUT, entry_of(OS_FB_APP_RELEASE, "prod-rollback")->flags);
    TEST_ASSERT_EQUAL(OSUI_PAR_ARG | OSUI_PAR_MUT, entry_of(OS_FB_APP_RELEASE, "prod-restore")->flags);
    TEST_ASSERT_EQUAL(0, entry_of(OS_FB_APP_KEYS, "collab-keys")->flags);
    e = entry_of(OS_FB_APP_KEYS, "collab-contract-deploy");
    TEST_ASSERT_EQUAL(OSUI_PAR_ARG | OSUI_PAR_MUT, e->flags);
    TEST_ASSERT_EQUAL(-1, osui_par_build(e, "", out, (int)sizeof(out)));
}

/* Lot 65 (US-G19..US-G24): every registry command of the pm-, task-,
 * child-, p2p- and collab- families (plus kill, kill-children, children)
 * is an entry of a desktop panel, unless it is terminal-only for the GUI
 * rights model or in the short list below, each with its reason. */
static int in_panel(const char *name) {
    int i, k;
    char n2[40];
    for (i = 0; i < osui_par_family_count; i++)
        for (k = 0; k < osui_par_families[i].count; k++) {
            osui_par_cmd_name(&osui_par_families[i].entries[k], n2, (int)sizeof(n2));
            if (!strcmp(n2, name)) return 1;
        }
    return 0;
}
static void test_lot65_families_complete(void) {
    static const char *const prefix[] = { "pm-", "task-", "child-", "p2p-", "collab-", 0 };
    static const char *const extra[] = { "kill", "kill-children", "children", 0 };
    /* blocking until a child exits / attack test hook */
    static const char *const off[] = { "wait", "wait-result", "wait-any-result", "collab-forge", 0 };
    int i, j, checked = 0, missing = 0;
    for (i = 0; i < MOHHDY_SHELL_COMMAND_COUNT; i++) {
        const char *c = mohhdy_shell_commands[i];
        int want = 0, skip = 0;
        for (j = 0; prefix[j]; j++) if (!strncmp(c, prefix[j], strlen(prefix[j]))) want = 1;
        for (j = 0; extra[j]; j++) if (!strcmp(c, extra[j])) want = 1;
        for (j = 0; off[j]; j++) if (!strcmp(c, off[j])) skip = 1;
        if (!want || skip || osui_tool_cmd_check(c, reg_known) == OSUI_CMD_TERMINAL) continue;
        checked++;
        if (!in_panel(c)) { printf("  not in a panel: %s\n", c); missing++; }
    }
    TEST_ASSERT_EQUAL(0, missing);
    TEST_ASSERT(checked >= 100);
    /* the line editors and the chat loop stay in the terminal */
    TEST_ASSERT_EQUAL(OSUI_CMD_TERMINAL, osui_tool_cmd_check("pm-edit a.pm", reg_known));
    TEST_ASSERT_EQUAL(OSUI_CMD_TERMINAL, osui_tool_cmd_check("pm-ide a.pm", reg_known));
    TEST_ASSERT_EQUAL(OSUI_CMD_TERMINAL, osui_tool_cmd_check("p2p-chat", reg_known));
    TEST_ASSERT(!in_panel("collab-forge"));
}

static void type(const char *s) { while (*s) osui_tool_key(&t, *s++, &k_src); }

static void test_panel_menu_run_and_back(void) {
    setup_src();
    memset(&t, 0, sizeof(t));
    osui_tool_open(&t, OS_FB_APP_NET, "", &k_src);
    TEST_ASSERT_EQUAL(0, g_runs);              /* opening runs nothing */
    TEST_ASSERT_EQUAL(0, t.sel);
    TEST_ASSERT(has_line("[net-status]"));
    TEST_ASSERT(has_line("[p2p-up NOM IP [CLE]] *"));
    osui_tool_render(&t, &v);
    TEST_ASSERT_EQUAL_STRING("Reseau", v.title);
    TEST_ASSERT_EQUAL_STRING("[Executer] [Menu] [Actualiser] [Fermer]", v.tool);
    TEST_ASSERT(strncmp(v.rows[OSUI_APP_ROWS - 1], "arg> ", 5) == 0);
    TEST_ASSERT_EQUAL(OSUI_TOOL_CHANGED, osui_tool_key(&t, '\n', &k_src));
    TEST_ASSERT_EQUAL_STRING("net-status|", g_cmds);
    TEST_ASSERT_EQUAL(1, t.mode);
    TEST_ASSERT(has_line("$ net-status"));
    TEST_ASSERT(has_line("net ok ip 10.0.2.15"));
    TEST_ASSERT(strstr(t.log, "osui panel run cmd=net-status ok") != 0);
    /* Actualiser re-runs the same command */
    TEST_ASSERT_EQUAL(OSUI_TOOL_CHANGED, osui_tool_tool(&t, 2, &k_src));
    TEST_ASSERT_EQUAL_STRING("net-status|net-status|", g_cmds);
    /* Esc goes back to the menu, a second Esc closes */
    TEST_ASSERT_EQUAL(OSUI_TOOL_CHANGED, osui_tool_key(&t, K_ESC, &k_src));
    TEST_ASSERT_EQUAL(0, t.mode);
    TEST_ASSERT_EQUAL(OSUI_TOOL_CLOSE, osui_tool_key(&t, K_ESC, &k_src));
    TEST_ASSERT_EQUAL(OSUI_TOOL_CLOSE, osui_tool_tool(&t, 3, &k_src));
}

static void test_panel_arg_and_confirm(void) {
    int i;
    setup_src();
    memset(&t, 0, sizeof(t));
    osui_tool_open(&t, OS_FB_APP_STORE, "", &k_src);
    for (i = 0; i < 3; i++) osui_tool_key(&t, K_DOWN, &k_src);   /* ls */
    osui_tool_key(&t, '\n', &k_src);
    TEST_ASSERT_EQUAL(0, g_runs);
    TEST_ASSERT(strstr(t.status, "saisissez d'abord") != 0);
    type("/samples");
    osui_tool_key(&t, '\n', &k_src);
    TEST_ASSERT_EQUAL_STRING("ls /samples|", g_cmds);
    TEST_ASSERT_EQUAL(0, t.ilen);
    /* mkdir (MUT): first Enter arms, second runs */
    osui_tool_tool(&t, 1, &k_src);
    TEST_ASSERT_EQUAL(0, t.mode);
    osui_tool_key(&t, K_DOWN, &k_src); osui_tool_key(&t, K_DOWN, &k_src);
    TEST_ASSERT_EQUAL(5, t.sel);
    type("/overlay/d");
    osui_tool_key(&t, '\n', &k_src);
    TEST_ASSERT_EQUAL(1, g_runs);
    TEST_ASSERT_EQUAL(5, t.armed_pid);
    TEST_ASSERT(strstr(t.status, "Entree encore") != 0);
    TEST_ASSERT(strstr(t.log, "osui panel armed cmd=mkdir /overlay/d") != 0);
    /* moving the selection disarms */
    osui_tool_key(&t, K_UP, &k_src);
    TEST_ASSERT_EQUAL(-1, t.armed_pid);
    osui_tool_key(&t, K_DOWN, &k_src);
    osui_tool_key(&t, '\n', &k_src);
    TEST_ASSERT_EQUAL(1, g_runs);
    osui_tool_key(&t, '\n', &k_src);
    TEST_ASSERT_EQUAL(2, g_runs);
    TEST_ASSERT(strstr(g_cmds, "mkdir /overlay/d|") != 0);
    /* a double click on a menu line runs it */
    osui_tool_tool(&t, 1, &k_src);
    TEST_ASSERT_EQUAL(OSUI_TOOL_CHANGED, osui_tool_click(&t, 0, 0, 1, &k_src));
    TEST_ASSERT(strstr(g_cmds, "ata-status|") != 0);
}

static void test_search_and_generic_cmd(void) {
    setup_src();
    memset(&t, 0, sizeof(t));
    osui_tool_open(&t, OS_FB_APP_SEARCH, "", &k_src);
    type("ligne /samples");
    osui_tool_key(&t, '\n', &k_src);
    TEST_ASSERT_EQUAL_STRING("grep -r ligne /samples|", g_cmds);
    TEST_ASSERT(has_line("/samples/long.txt:3"));
    /* /cmd: any registry command typed, refusals explained */
    memset(&t, 0, sizeof(t));
    osui_tool_open(&t, OS_FB_APP_CMD, "", &k_src);
    TEST_ASSERT_EQUAL(-1, t.sel);
    osui_tool_render(&t, &v);
    TEST_ASSERT(strncmp(v.rows[OSUI_APP_ROWS - 1], "cmd> ", 5) == 0);
    type("uptime");
    osui_tool_key(&t, '\n', &k_src);
    TEST_ASSERT(has_line("uptime 42 s"));
    type("top");
    osui_tool_key(&t, '\n', &k_src);
    TEST_ASSERT(has_line("commande refusee: top (commande reservee au terminal (interactive))"));
    type("ls | wc");
    osui_tool_key(&t, '\n', &k_src);
    TEST_ASSERT(has_line("caracteres shell interdits"));
    type("frobnicate");
    osui_tool_key(&t, '\n', &k_src);
    TEST_ASSERT(has_line("inconnue du registre"));
    TEST_ASSERT(strstr(t.log, "refused") != 0);
}

int main(void) {
    unity_init();
    RUN_TEST(test_rights_model);
    RUN_TEST(test_table_matches_registry);
    RUN_TEST(test_build);
    RUN_TEST(test_lot46_entries);
    RUN_TEST(test_lot50_entries);
    RUN_TEST(test_lot51_entries);
    RUN_TEST(test_lot65_families_complete);
    RUN_TEST(test_panel_menu_run_and_back);
    RUN_TEST(test_panel_arg_and_confirm);
    RUN_TEST(test_search_and_generic_cmd);
    unity_print_results();
    unity_cleanup();
    return (unity_stats.tests_failed == 0) ? 0 : 1;
}
