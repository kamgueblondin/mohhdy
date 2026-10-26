/* test_osui_tools.c - UX4 desktop tool apps (task manager, viewer,
 * settings, P2P, PromptMessage run, GPT-2 chat) over injected sources. */
#include "../../framework/unity.h"
#include "../../framework/test_kernel.h"
#include "osui_tools.h"
#include "os_syscalls.h"
#include <string.h>
#include <stdio.h>


static int my_atoi(const char *s) { int v = 0; while (*s >= '0' && *s <= '9') v = v * 10 + (*s++ - '0'); return v; }
static char g_cmds[2048];
static int g_killed = -1;
static int g_steps;
static char g_model[32] = "gpt2.gguf";

/* Lot 46: a 200-character line, an empty line, a short line */
static char g_long_out[256];
static void make_long_out(void) {
    int i;
    for (i = 0; i < 200; i++) g_long_out[i] = (char)('a' + i % 26);
    strcpy(g_long_out + 200, "\n\nfin\n");
}

static int fake_run(const char *cmd, char *out, int max) {
    const char *r = "";
    if (!osui_tool_cmd_allowed(cmd)) return -1;
    strcat(g_cmds, cmd); strcat(g_cmds, "|");
    if (!strcmp(cmd, "ps")) r = "\x1b[33mPID  NAME   STATE\x1b[0m\n 1  kernel  run\n 7  shell   run\n 12  ps2driver wait\n";
    else if (!strcmp(cmd, "mem")) r = "mem total 64 MiB free 40 MiB\nUtilisation M\xc3\xa9moire\n";
    else if (!strncmp(cmd, "kill ", 5)) { g_killed = my_atoi(cmd + 5); r = g_killed == 1 ? "kill: processus protege (kernel)\n" : "Processus termine\n"; }
    else if (!strncmp(cmd, "service-status ", 15)) r = "service ok\n";
    else if (!strcmp(cmd, "date")) r = "date ok 2026-10-24 19:00:00 UTC\n";
    else if (!strcmp(cmd, "uptime")) r = "uptime 42 s\n";
    else if (!strcmp(cmd, "whoami")) r = "root\n";
    else if (!strcmp(cmd, "p2p-peers")) r = "p2p peers 1: bob 10.0.2.16\n";
    else if (!strcmp(cmd, "p2p-stats")) r = "p2p stats tx=3 rx=2\n";
    else if (!strncmp(cmd, "p2p-send ", 9)) r = "p2p-send ok bob\n";
    else if (!strncmp(cmd, "p2p-up ", 7)) r = "p2p-up ok alice\n";
    else if (!strcmp(cmd, "pm-run /home/long.pm")) r = g_long_out;
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
    else if (!strcmp(path, "/home/long.txt"))
        r = "0123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789"
            "0123456789012345678901234567890123456789012345678901234567890123456789\nfin\n";
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

static const osui_tool_src_t k_src = { fake_run, fake_read, fake_ai, fake_model, fake_screen, 0, 0 };
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
    TEST_ASSERT_EQUAL_STRING("PID  NAME   STATE", t.lines[0]);   /* ANSI colours dropped */
    TEST_ASSERT(has_line("free 40 MiB"));
    TEST_ASSERT(has_line("Utilisation Memoire"));          /* UTF-8 accent folded for the ASCII grid */
    TEST_ASSERT_EQUAL(1, t.sel);              /* first process row selected */
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

/* Lot 45 (US-G15): a tool window follows its grid (maximize / restore):
 * more rows shown, text rewrapped at the new width, input line at the
 * bottom of the bigger grid; back to the default grid on restore. */
static void test_window_grid_follows_size(void) {
    memset(&t, 0, sizeof(t));
    osui_tool_open(&t, OS_FB_APP_VIEW, "/home/long.txt", &k_src);
    TEST_ASSERT_EQUAL(76, (int)strlen(t.lines[0]));          /* default 78-col grid */
    osui_tool_set_view(&t, 40, 120, &k_src);
    TEST_ASSERT_EQUAL(119, (int)strlen(t.lines[0]));         /* rewrapped at 120 cols */
    osui_tool_render(&t, &v);
    TEST_ASSERT_EQUAL(119, (int)strlen(v.rows[0]));
    osui_tool_set_view(&t, 0, 0, &k_src);
    TEST_ASSERT_EQUAL(76, (int)strlen(t.lines[0]));
    memset(&t, 0, sizeof(t));
    osui_tool_open(&t, OS_FB_APP_NET, "", &k_src);
    osui_tool_render(&t, &v);
    TEST_ASSERT_EQUAL(OSUI_APP_ROWS, v.nrows);
    osui_tool_set_view(&t, 40, 120, &k_src);
    osui_tool_render(&t, &v);
    TEST_ASSERT_EQUAL(40, v.nrows);                          /* input row is the last of 40 */
    TEST_ASSERT_EQUAL(39, v.cur_row);
    TEST_ASSERT(strncmp(v.rows[39], "arg> ", 5) == 0);
    osui_tool_open(&t, OS_FB_APP_NET, "", &k_src);           /* reopened: default grid */
    osui_tool_render(&t, &v);
    TEST_ASSERT_EQUAL(OSUI_APP_ROWS, v.nrows);
}

/* Lot 46: output already shown is wrapped again when the window grows or
 * shrinks (soft wraps joined, hard line ends and empty lines kept) */
static void test_output_rewraps_at_resize(void) {
    int k;
    make_long_out();
    memset(&t, 0, sizeof(t));
    osui_tool_open(&t, OS_FB_APP_PMRUN, "/home/long.pm", &k_src);
    /* "$ pm-run ..." then 76 + 76 + 48 characters, "", "fin" */
    TEST_ASSERT_EQUAL(6, t.n);
    TEST_ASSERT_EQUAL(76, (int)strlen(t.lines[1]));
    TEST_ASSERT_EQUAL(1, t.cont[2]);
    TEST_ASSERT_EQUAL(48, (int)strlen(t.lines[3]));
    TEST_ASSERT_EQUAL_STRING("", t.lines[4]);
    TEST_ASSERT_EQUAL_STRING("fin", t.lines[5]);
    osui_tool_set_view(&t, 40, 120, &k_src);
    TEST_ASSERT_EQUAL(5, t.n);                              /* 119 + 81 */
    TEST_ASSERT_EQUAL(119, (int)strlen(t.lines[1]));
    TEST_ASSERT_EQUAL(81, (int)strlen(t.lines[2]));
    TEST_ASSERT_EQUAL((char)('a' + 119 % 26), t.lines[2][0]); /* no byte lost at the join */
    TEST_ASSERT_EQUAL_STRING("", t.lines[3]);
    TEST_ASSERT_EQUAL_STRING("fin", t.lines[4]);
    TEST_ASSERT(strncmp(t.lines[0], "$ pm-run /home/long.pm", 22) == 0);
    osui_tool_set_view(&t, 0, 0, &k_src);                   /* back to the default grid */
    TEST_ASSERT_EQUAL(6, t.n);
    for (k = 0; k < 76; k++) TEST_ASSERT_EQUAL((char)('a' + k % 26), t.lines[1][k]);
    TEST_ASSERT_EQUAL_STRING("fin", t.lines[5]);
}

/* Lot 46: notes after the output of a launched program (kernel capture) */
static void test_spawn_note(void) {
    char note[256];
    os_outcap_stat_t st;
    memset(&st, 0, sizeof(st));
    st.total = 120;
    TEST_ASSERT_EQUAL(0, osui_tool_spawn_note(note, (int)sizeof(note), &st));
    TEST_ASSERT_EQUAL_STRING("", note);
    st.total = 20000; st.dropped = 3616; st.live = 1; st.untracked = 2;
    TEST_ASSERT(osui_tool_spawn_note(note, (int)sizeof(note), &st) > 0);
    TEST_ASSERT(strstr(note, "[sortie du programme tronquee: 3616 octets perdus sur 20000 (16 Kio max)]") != 0);
    TEST_ASSERT(strstr(note, "[2 programme(s) non captures: table de 16 taches pleine]") != 0);
    TEST_ASSERT(strstr(note, "[1 programme(s) encore actif(s)") != 0);
    TEST_ASSERT_EQUAL(0, osui_tool_spawn_note(note, (int)sizeof(note), 0));
    TEST_ASSERT(osui_tool_spawn_note(note, 8, &st) <= 7);       /* bounded */
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
    /* UX5: real pixels (the compositor draws them), a caption row */
    TEST_ASSERT_EQUAL_STRING("image 4x2 affichee en pixels 4x2 fmt=pnm zoom=fit", t.lines[0]);
    TEST_ASSERT_EQUAL(4, t.img_w);
    TEST_ASSERT_EQUAL(2, t.img_h);
    {
        const unsigned char *px = osui_tool_image_rgb();
        TEST_ASSERT_EQUAL(0, px[0]);                 /* (0,0) black */
        TEST_ASSERT_EQUAL(255, px[2 * 3]);           /* (2,0) white */
        TEST_ASSERT_EQUAL(255, px[(4 + 3) * 3 + 1]); /* (3,1) white */
    }
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
    {
        /* decode: P6 colors, P3 maxval scaling, downscale keeps the aspect */
        static unsigned char rgb[OS_FB_IMG_MAX_W * OS_FB_IMG_MAX_H * 3];
        static const char p6[] = "P6 2 1 255\n\xff\x00\x00\x00\x00\xff";
        static char big[64 + 640 * 10];
        int w = 0, h = 0, sw = 0, sh = 0, n, i;
        TEST_ASSERT_EQUAL(0, osui_tool_image_decode(p6, (int)sizeof(p6) - 1, rgb, 320, 240, &w, &h, &sw, &sh));
        TEST_ASSERT_EQUAL(2, w); TEST_ASSERT_EQUAL(1, h);
        TEST_ASSERT_EQUAL(255, rgb[0]); TEST_ASSERT_EQUAL(0, rgb[1]); TEST_ASSERT_EQUAL(255, rgb[5]);
        TEST_ASSERT_EQUAL(0, osui_tool_image_decode("P3 1 1 15\n15 0 7\n", 16, rgb, 320, 240, &w, &h, 0, 0));
        TEST_ASSERT_EQUAL(255, rgb[0]); TEST_ASSERT_EQUAL(119, rgb[2]);
        n = sprintf(big, "P5 640 10 255\n");
        for (i = 0; i < 640 * 10; i++) big[n + i] = (char)(i % 640 < 320 ? 0 : 200);
        TEST_ASSERT_EQUAL(0, osui_tool_image_decode(big, n + 6400, rgb, 320, 240, &w, &h, &sw, &sh));
        TEST_ASSERT_EQUAL(320, w); TEST_ASSERT_EQUAL(5, h);
        TEST_ASSERT_EQUAL(640, sw); TEST_ASSERT_EQUAL(10, sh);
        TEST_ASSERT_EQUAL(0, rgb[0]); TEST_ASSERT_EQUAL(200, rgb[319 * 3]);
        TEST_ASSERT(osui_tool_image_decode(big, n + 100, rgb, 320, 240, &w, &h, 0, 0) < 0);   /* truncated */
        TEST_ASSERT(osui_tool_image_decode("hello", 5, rgb, 320, 240, &w, &h, 0, 0) < 0);
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
    RUN_TEST(test_window_grid_follows_size);
    RUN_TEST(test_spawn_note);
    RUN_TEST(test_output_rewraps_at_resize);
    RUN_TEST(test_settings_real_values);
    RUN_TEST(test_p2p_window);
    RUN_TEST(test_pm_run_window);
    RUN_TEST(test_chat_streams_and_cancels);
    unity_print_results();
    unity_cleanup();
    return (unity_stats.tests_failed == 0) ? 0 : 1;
}
