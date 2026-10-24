/* test_osui_tools.c - UX4 desktop tool apps (task manager, viewer,
 * settings, P2P, PromptMessage run, GPT-2 chat) over injected sources. */
#include "../../framework/unity.h"
#include "../../framework/test_kernel.h"
#include "osui_tools.h"
#include "os_syscalls.h"
#include <string.h>


static int my_atoi(const char *s) { int v = 0; while (*s >= '0' && *s <= '9') v = v * 10 + (*s++ - '0'); return v; }
static char g_cmds[2048];
static int g_killed = -1;
static int g_steps;
static char g_model[32] = "gpt2.gguf";

static int fake_run(const char *cmd, char *out, int max) {
    const char *r = "";
    if (!osui_tool_cmd_allowed(cmd)) return -1;
    strcat(g_cmds, cmd); strcat(g_cmds, "|");
    if (!strcmp(cmd, "ps")) r = "PID  NAME   STATE\n 1  kernel  run\n 7  shell   run\n 12  ps2driver wait\n";
    else if (!strcmp(cmd, "mem")) r = "mem total 64 MiB free 40 MiB\n";
    else if (!strncmp(cmd, "kill ", 5)) { g_killed = my_atoi(cmd + 5); r = g_killed == 1 ? "kill: processus protege (kernel)\n" : "Processus termine\n"; }
    else if (!strncmp(cmd, "service-status ", 15)) r = "service ok\n";
    else if (!strcmp(cmd, "date")) r = "date ok 2026-10-24 19:00:00 UTC\n";
    else if (!strcmp(cmd, "uptime")) r = "uptime 42 s\n";
    else if (!strcmp(cmd, "whoami")) r = "root\n";
    else if (!strcmp(cmd, "p2p-peers")) r = "p2p peers 1: bob 10.0.2.16\n";
    else if (!strcmp(cmd, "p2p-stats")) r = "p2p stats tx=3 rx=2\n";
    else if (!strncmp(cmd, "p2p-send ", 9)) r = "p2p-send ok bob\n";
    else if (!strncmp(cmd, "p2p-up ", 7)) r = "p2p-up ok alice\n";
    else if (!strncmp(cmd, "pm-run ", 7)) r = "Bonjour depuis PM\npm-run ok\n";
    else if (!strncmp(cmd, "pm-check ", 9)) r = "pm-check ok\n";
    else if (!strncmp(cmd, "ai-model use ", 13)) { strcpy(g_model, cmd + 13); r = "Profil selectionne\n"; }
    strncpy(out, r, (size_t)max - 1); out[max - 1] = 0;
    return (int)strlen(out);
}

static int fake_read(const char *path, char *buf, int max) {
    const char *r;
    if (!strcmp(path, "/home/img.pgm")) r = "P2\n# c\n4 2\n255\n0 0 255 255\n0 0 255 255\n";
    else if (!strcmp(path, "/home/a.txt")) r = "ligne un\nligne deux\nligne trois\n";
    else return -2;
    strncpy(buf, r, (size_t)max); return (int)strlen(r);
}

static int fake_ai(const char *prompt, int first, char *piece, int max) {
    static const char *parts[] = { "Le systeme", " demarre", " vite et bien", " sur QEMU.", " Encore" };
    (void)prompt; (void)max;
    if (first) g_steps = 0;
    if (g_steps >= 5) return 0;
    strcpy(piece, parts[g_steps++]);
    return (int)strlen(piece);
}
static const char *fake_model(void) { return g_model; }
static int fake_screen(int *w, int *h) { *w = 1024; *h = 600; return 0; }

static const osui_tool_src_t k_src = { fake_run, fake_read, fake_ai, fake_model, fake_screen };
static osui_tool_t t;
static osui_app_view_t v;

static int has_line(const char *s) {
    int i;
    for (i = 0; i < t.n; i++) if (strstr(t.lines[i], s)) return 1;
    return 0;
}

static void test_allowlist(void) {
    TEST_ASSERT(osui_tool_cmd_allowed("ps"));
    TEST_ASSERT(osui_tool_cmd_allowed("kill 7"));
    TEST_ASSERT(osui_tool_cmd_allowed("p2p-send bob hi"));
    TEST_ASSERT(!osui_tool_cmd_allowed("psx"));
    TEST_ASSERT(!osui_tool_cmd_allowed("gui"));
    TEST_ASSERT(!osui_tool_cmd_allowed("pm-ide a.pm"));
    TEST_ASSERT(!osui_tool_cmd_allowed("ps; rm x"));
    TEST_ASSERT(!osui_tool_cmd_allowed("rm /x"));
    TEST_ASSERT_EQUAL(7, osui_tool_line_pid(" 7  shell   run"));
    TEST_ASSERT_EQUAL(-1, osui_tool_line_pid("PID  NAME"));
    TEST_ASSERT_EQUAL(-1, osui_tool_line_pid("mem total 64"));
}

static void test_task_manager_kill_with_confirmation(void) {
    memset(&t, 0, sizeof(t)); g_cmds[0] = 0; g_killed = -1;
    osui_tool_open(&t, OS_FB_APP_TASKS, "", &k_src);
    TEST_ASSERT(strstr(g_cmds, "ps|mem|") != 0);
    TEST_ASSERT(has_line("ps2driver"));
    TEST_ASSERT(has_line("free 40 MiB"));
    /* select the shell row (row 2), Tuer once arms, twice kills */
    TEST_ASSERT_EQUAL(OSUI_TOOL_CHANGED, osui_tool_click(&t, 2, 0, 0, &k_src));
    osui_tool_tool(&t, 1, &k_src);
    TEST_ASSERT_EQUAL(-1, g_killed);
    TEST_ASSERT(strstr(t.status, "confirmer") != 0);
    osui_tool_tool(&t, 1, &k_src);
    TEST_ASSERT_EQUAL(7, g_killed);
    TEST_ASSERT(strstr(t.log, "osui tasks kill pid=7") != 0);
    /* the kernel refusal (protected PID) reaches the status line */
    osui_tool_click(&t, 1, 0, 0, &k_src);
    osui_tool_tool(&t, 1, &k_src); osui_tool_tool(&t, 1, &k_src);
    TEST_ASSERT_EQUAL(1, g_killed);
    TEST_ASSERT(strstr(t.status, "protege") != 0);
    /* header row: nothing to kill */
    osui_tool_click(&t, 0, 0, 0, &k_src);
    g_killed = -1;
    osui_tool_tool(&t, 1, &k_src);
    TEST_ASSERT_EQUAL(-1, g_killed);
    /* services view */
    g_cmds[0] = 0;
    osui_tool_tool(&t, 2, &k_src);
    TEST_ASSERT(strstr(g_cmds, "service-status vfs|") != 0);
    TEST_ASSERT(strstr(g_cmds, "service-status ps2-driver|") != 0);
    osui_tool_render(&t, &v);
    TEST_ASSERT(strstr(v.title, "Gestionnaire") != 0);
    TEST_ASSERT(strstr(v.tool, "[Tuer]") != 0);
    TEST_ASSERT_EQUAL(OSUI_TOOL_CLOSE, osui_tool_tool(&t, 3, &k_src));
}

static void test_viewer_text_and_image(void) {
    memset(&t, 0, sizeof(t));
    osui_tool_open(&t, OS_FB_APP_VIEW, "/home/a.txt", &k_src);
    TEST_ASSERT_EQUAL(3, t.n);
    TEST_ASSERT_EQUAL_STRING("ligne deux", t.lines[1]);
    TEST_ASSERT(strstr(t.status, "3 lignes") != 0);
    TEST_ASSERT_EQUAL(OSUI_TOOL_EDIT, osui_tool_tool(&t, 2, &k_src));
    osui_tool_open(&t, OS_FB_APP_VIEW, "/home/img.pgm", &k_src);
    TEST_ASSERT_EQUAL(1, t.image);
    TEST_ASSERT_EQUAL_STRING("  @@", t.lines[0]);
    TEST_ASSERT_EQUAL(OSUI_TOOL_NONE, osui_tool_tool(&t, 2, &k_src));
    osui_tool_open(&t, OS_FB_APP_VIEW, "/missing", &k_src);
    TEST_ASSERT(has_line("lecture impossible"));
    {
        char out[4][OSUI_APP_COLS];
        static const char p5[] = "P5 2 1 255\n\x00\xff";
        TEST_ASSERT_EQUAL(1, osui_tool_image_ascii(p5, (int)sizeof(p5) - 1, out, 10, 4));
        TEST_ASSERT_EQUAL_STRING(" @", out[0]);
        TEST_ASSERT(osui_tool_image_ascii("hello", 5, out, 10, 4) < 0);
    }
}

static void test_settings_real_values(void) {
    memset(&t, 0, sizeof(t));
    osui_tool_open(&t, OS_FB_APP_SETTINGS, "", &k_src);
    TEST_ASSERT(has_line("date ok 2026-10-24"));
    TEST_ASSERT(has_line("1024x600"));
    TEST_ASSERT(has_line("US QWERTY"));
    TEST_ASSERT(has_line("root"));
}

static void test_p2p_window(void) {
    memset(&t, 0, sizeof(t)); g_cmds[0] = 0;
    osui_tool_open(&t, OS_FB_APP_P2P, "", &k_src);
    TEST_ASSERT(has_line("bob 10.0.2.16"));
    TEST_ASSERT(has_line("tx=3"));
    osui_tool_tool(&t, 4, &k_src);           /* Envoyer without text */
    TEST_ASSERT(strstr(t.status, "PAIR TEXTE") != 0);
    { const char *s = "bob salut"; while (*s) osui_tool_key(&t, *s++, &k_src); }
    osui_tool_render(&t, &v);
    TEST_ASSERT_EQUAL_STRING("p2p> bob salut", v.rows[OSUI_APP_ROWS - 1]);
    osui_tool_key(&t, '\n', &k_src);
    TEST_ASSERT(strstr(g_cmds, "p2p-send bob salut|") != 0);
    TEST_ASSERT(has_line("p2p-send ok bob"));
    { const char *s = "alice 10.0.2.15"; while (*s) osui_tool_key(&t, *s++, &k_src); }
    osui_tool_tool(&t, 2, &k_src);
    TEST_ASSERT(strstr(g_cmds, "p2p-up alice 10.0.2.15|") != 0);
}

static void test_pm_run_window(void) {
    memset(&t, 0, sizeof(t)); g_cmds[0] = 0;
    osui_tool_open(&t, OS_FB_APP_PMRUN, "/home/hello.pm", &k_src);
    TEST_ASSERT(strstr(g_cmds, "pm-run /home/hello.pm|") != 0);
    TEST_ASSERT(has_line("Bonjour depuis PM"));
    osui_tool_tool(&t, 1, &k_src);
    TEST_ASSERT(has_line("pm-check ok"));
    TEST_ASSERT_EQUAL(OSUI_TOOL_EDIT, osui_tool_tool(&t, 2, &k_src));
}

static void test_chat_streams_and_cancels(void) {
    int guard = 0;
    memset(&t, 0, sizeof(t)); g_cmds[0] = 0;
    strcpy(g_model, "gpt2.gguf");
    osui_tool_open(&t, OS_FB_APP_CHAT, "", &k_src);
    TEST_ASSERT(has_line("gpt2.gguf"));
    { const char *s = "Bonjour"; while (*s) osui_tool_key(&t, *s++, &k_src); }
    osui_tool_key(&t, '\n', &k_src);
    TEST_ASSERT_EQUAL(1, t.gen);
    TEST_ASSERT(has_line("Vous: Bonjour"));
    /* one piece per idle tick, visible after each */
    TEST_ASSERT_EQUAL(1, osui_tool_idle(&t, &k_src));
    TEST_ASSERT(has_line("IA: Le systeme"));
    while (t.gen && guard++ < 10) osui_tool_idle(&t, &k_src);
    TEST_ASSERT_EQUAL(0, t.gen);
    TEST_ASSERT(has_line("IA: Le systeme demarre vite et bien sur QEMU."));
    TEST_ASSERT(strstr(t.log, "phrase complete") != 0);
    /* Suite continues, Annuler stops between steps */
    osui_tool_tool(&t, 1, &k_src);
    TEST_ASSERT(t.gen);
    osui_tool_tool(&t, 2, &k_src);
    TEST_ASSERT_EQUAL(0, t.gen);
    TEST_ASSERT_EQUAL(0, osui_tool_idle(&t, &k_src));
    /* model picker goes through ai-model use */
    osui_tool_tool(&t, 3, &k_src);
    TEST_ASSERT(strstr(g_cmds, "ai-model use gpt2_124M.bin|") != 0);
    osui_tool_tool(&t, 3, &k_src);
    TEST_ASSERT(strstr(g_cmds, "ai-model use gpt2.gguf|") != 0);
    osui_tool_render(&t, &v);
    TEST_ASSERT_EQUAL(OSUI_APP_ROWS - 1, v.cur_row);
    TEST_ASSERT_EQUAL(OSUI_TOOL_CLOSE, osui_tool_tool(&t, 5, &k_src));
}

int main(void) {
    unity_init();
    RUN_TEST(test_allowlist);
    RUN_TEST(test_task_manager_kill_with_confirmation);
    RUN_TEST(test_viewer_text_and_image);
    RUN_TEST(test_settings_real_values);
    RUN_TEST(test_p2p_window);
    RUN_TEST(test_pm_run_window);
    RUN_TEST(test_chat_streams_and_cancels);
    unity_print_results();
    unity_cleanup();
    return (unity_stats.tests_failed == 0) ? 0 : 1;
}
