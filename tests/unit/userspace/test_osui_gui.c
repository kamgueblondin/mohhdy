#include <stdio.h>
/* test_osui_gui.c - Commande gui, bureau VBE QEMU, chat float. */

#include "../../framework/unity.h"
#include "../../framework/test_kernel.h"
#include "osui_runtime.h"
#include "osui_gui.h"
#include "os_syscalls.h"
#include "osui_ide.h"
#include <string.h>

static uint16_t g_cells[OS_VGA_ROWS * OS_VGA_COLS];
static char g_out[OSUI_OUT_MAX];
static char g_row[96];
static char g_snap[OSUI_SNAP_MAX];
static os_fb_scene_t g_scene;

static int run_line(const char *line) {
    memset(g_out, 0, sizeof(g_out));
    return osui_dispatch_line(line, g_out, (int)sizeof(g_out));
}

static void setup(void) {
    osui_runtime_init();
    osui_gui_set_input("");
    osui_gui_term_reset();
    osui_gui_set_program_eval(0);
}

static void test_gui_command_canonical(void) {
    setup();
    TEST_ASSERT_EQUAL(0, run_line("gui"));
    TEST_ASSERT(strstr(g_out, "canonical=gui") != NULL);
    TEST_ASSERT(strstr(g_out, "chrome=qemu_fb") != NULL);
    TEST_ASSERT(strstr(g_out, "display_surface=vbe_lfb") != NULL);
    TEST_ASSERT(strstr(g_out, "us031_complete=false") != NULL);
    TEST_ASSERT(osui_gui_should_enter());
    osui_gui_ack_enter();
    TEST_ASSERT_EQUAL(0, run_line("graphics"));
    TEST_ASSERT(strstr(g_out, "canonical=gui") != NULL);
    TEST_ASSERT_EQUAL(0, run_line("desktop"));
    TEST_ASSERT(strstr(g_out, "chrome=qemu_fb") != NULL);
}

static void test_vga_pointer_not_ascii_desktop(void) {
    setup();
    osui_gui_render(g_cells);
    osui_gui_ascii_row(g_cells, 0, g_row, (int)sizeof(g_row));
    TEST_ASSERT(strstr(g_row, "MOHHDY OS") != NULL);
    TEST_ASSERT(strstr(g_row, "qemu_fb") != NULL);
    TEST_ASSERT(strstr(g_row, "us031=true") != NULL);
    osui_gui_ascii_row(g_cells, 5, g_row, (int)sizeof(g_row));
    TEST_ASSERT(strstr(g_row, "CHAT central") == NULL);
    osui_gui_ascii_row(g_cells, 3, g_row, (int)sizeof(g_row));
    TEST_ASSERT(strstr(g_row, "ASCII") != NULL);
}

static void test_scene_center_and_float(void) {
    setup();
    osui_gui_fill_scene(&g_scene);
    TEST_ASSERT_EQUAL(OS_FB_MAGIC, g_scene.magic);
    TEST_ASSERT_EQUAL(OS_FB_CHAT_CENTER, g_scene.chat_mode);
    TEST_ASSERT_EQUAL(OS_FB_PANE_NONE, g_scene.pane);

    TEST_ASSERT_EQUAL(0, run_line("/browser"));
    TEST_ASSERT_EQUAL_STRING("float", osui_get_chat_mode());
    osui_gui_fill_scene(&g_scene);
    TEST_ASSERT_EQUAL(OS_FB_CHAT_FLOAT, g_scene.chat_mode);
    TEST_ASSERT_EQUAL(OS_FB_PANE_BROWSER, g_scene.pane);

    TEST_ASSERT_EQUAL(0, run_line("/center"));
    osui_gui_fill_scene(&g_scene);
    TEST_ASSERT_EQUAL(OS_FB_CHAT_CENTER, g_scene.chat_mode);

    TEST_ASSERT_EQUAL(0, run_line("/shell"));
    TEST_ASSERT_EQUAL_STRING("float", osui_get_chat_mode());
    TEST_ASSERT_EQUAL_STRING("shell", osui_get_pane());
    osui_gui_fill_scene(&g_scene);
    TEST_ASSERT_EQUAL(OS_FB_CHAT_FLOAT, g_scene.chat_mode);
    TEST_ASSERT_EQUAL(OS_FB_PANE_SHELL, g_scene.pane);
}

static void test_snap_center_and_float(void) {
    setup();
    osui_gui_write_snap(g_snap, (int)sizeof(g_snap));
    TEST_ASSERT(strstr(g_snap, "OSUI-SNAP") != NULL);
    TEST_ASSERT(strstr(g_snap, "chrome=qemu_fb") != NULL);
    TEST_ASSERT(strstr(g_snap, "chat_mode=center") != NULL);
    TEST_ASSERT(strstr(g_snap, "OSUI-END") != NULL);

    TEST_ASSERT_EQUAL(0, run_line("/browser"));
    TEST_ASSERT_EQUAL_STRING("float", osui_get_chat_mode());
    TEST_ASSERT_EQUAL_STRING("browser", osui_get_pane());
    TEST_ASSERT(osui_get_chat_x() >= 40);
    osui_gui_write_snap(g_snap, (int)sizeof(g_snap));
    TEST_ASSERT(strstr(g_snap, "chat_mode=float") != NULL);
    TEST_ASSERT(strstr(g_snap, "pane=browser") != NULL);

    TEST_ASSERT_EQUAL(0, run_line("/center"));
    TEST_ASSERT_EQUAL_STRING("center", osui_get_chat_mode());
    osui_gui_write_snap(g_snap, (int)sizeof(g_snap));
    TEST_ASSERT(strstr(g_snap, "chat_mode=center") != NULL);
}

static void test_snap_stage_kind(void) {
    setup();
    run_line("stage-prompt dessine un cercle");
    TEST_ASSERT(strstr(g_out, "kind=circle") != NULL);
    TEST_ASSERT_EQUAL_STRING("circle", osui_get_stage_kind());
    osui_gui_fill_scene(&g_scene);
    TEST_ASSERT_EQUAL(OS_FB_KIND_CIRCLE, g_scene.stage_kind);
    osui_gui_write_snap(g_snap, (int)sizeof(g_snap));
    TEST_ASSERT(strstr(g_snap, "kind=circle") != NULL);
    TEST_ASSERT(strstr(g_snap, "stage=presenting") != NULL);

    run_line("stage-prompt dessine trois boites");
    TEST_ASSERT(strstr(g_out, "kind=boxes") != NULL);
    osui_gui_write_snap(g_snap, (int)sizeof(g_snap));
    TEST_ASSERT(strstr(g_snap, "kind=boxes") != NULL);
}

static void test_gui_feed_slash_and_esc(void) {
    int rc;
    setup();
    osui_gui_set_input("");
    osui_gui_feed_key('/', g_out, (int)sizeof(g_out));
    osui_gui_feed_key('b', g_out, (int)sizeof(g_out));
    osui_gui_feed_key('r', g_out, (int)sizeof(g_out));
    osui_gui_feed_key('o', g_out, (int)sizeof(g_out));
    osui_gui_feed_key('w', g_out, (int)sizeof(g_out));
    osui_gui_feed_key('s', g_out, (int)sizeof(g_out));
    osui_gui_feed_key('e', g_out, (int)sizeof(g_out));
    osui_gui_feed_key('r', g_out, (int)sizeof(g_out));
    TEST_ASSERT_EQUAL_STRING("/browser", osui_gui_input());
    memset(g_out, 0, sizeof(g_out));
    rc = osui_gui_feed_key('\n', g_out, (int)sizeof(g_out));
    TEST_ASSERT_EQUAL(0, rc);
    TEST_ASSERT_EQUAL_STRING("float", osui_get_chat_mode());

    memset(g_out, 0, sizeof(g_out));
    rc = osui_gui_feed_key(OS_VGA_KEY_ESC, g_out, (int)sizeof(g_out));
    TEST_ASSERT_EQUAL(1, rc);
    TEST_ASSERT(strstr(g_out, "gui exit") != NULL);
}

static int fake_shell_eval(const char *line, char *captured, int cap_max) {
    const char *msg = "whoami ok root\n";
    int i = 0;
    (void)line;
    if (!captured || cap_max <= 0) return 1;
    while (msg[i] && i < cap_max - 1) {
        captured[i] = msg[i];
        i++;
    }
    captured[i] = 0;
    return 0;
}

static void test_gui_live_shell_eval(void) {
    char term[80];
    int n, rc;
    setup();
    osui_gui_term_reset();
    osui_gui_set_program_eval(fake_shell_eval);
    TEST_ASSERT_EQUAL(0, run_line("/shell"));
    osui_gui_set_input("whoami");
    memset(g_out, 0, sizeof(g_out));
    rc = osui_gui_feed_key('\n', g_out, (int)sizeof(g_out));
    TEST_ASSERT_EQUAL(0, rc);
    TEST_ASSERT_EQUAL_STRING("shell", osui_get_pane());
    TEST_ASSERT(osui_gui_term_count() >= 3);
    n = osui_gui_term_count();
    osui_gui_term_at(n - 1, term, (int)sizeof(term));
    TEST_ASSERT(strstr(term, "whoami ok") != NULL);
    osui_gui_fill_scene(&g_scene);
    TEST_ASSERT_EQUAL(OS_FB_PANE_SHELL, g_scene.pane);
    TEST_ASSERT(g_scene.nterm >= 3);
    osui_gui_write_snap(g_snap, (int)sizeof(g_snap));
    TEST_ASSERT(strstr(g_snap, "nterm=") != NULL);
    TEST_ASSERT(strstr(g_snap, "term=") != NULL);
    TEST_ASSERT(strstr(g_snap, "whoami ok") != NULL);

    osui_gui_set_input("!date");
    memset(g_out, 0, sizeof(g_out));
    rc = osui_gui_feed_key('\n', g_out, (int)sizeof(g_out));
    TEST_ASSERT_EQUAL(0, rc);
    osui_gui_set_program_eval(0);
}

static void test_vbe_p2p_collab_prod_panes(void) {
    setup();
    TEST_ASSERT_EQUAL(0, run_line("/p2p"));
    TEST_ASSERT_EQUAL_STRING("p2p", osui_get_pane());
    osui_gui_fill_scene(&g_scene);
    TEST_ASSERT_EQUAL(OS_FB_PANE_P2P, g_scene.pane);

    TEST_ASSERT_EQUAL(0, run_line("/collab"));
    TEST_ASSERT_EQUAL_STRING("collab", osui_get_pane());
    osui_gui_fill_scene(&g_scene);
    TEST_ASSERT_EQUAL(OS_FB_PANE_COLLAB, g_scene.pane);

    TEST_ASSERT_EQUAL(0, run_line("/prod"));
    TEST_ASSERT_EQUAL_STRING("prod", osui_get_pane());
    osui_gui_fill_scene(&g_scene);
    TEST_ASSERT_EQUAL(OS_FB_PANE_PROD, g_scene.pane);
}

static void test_gui_media_rendering(void) {
    setup();
    run_line("media-play audio stream1");
    osui_gui_render(g_cells);
    osui_gui_ascii_row(g_cells, 9, g_row, (int)sizeof(g_row));
    TEST_ASSERT(strstr(g_row, "MEDIA PLAY stream1") != NULL);

    osui_gui_write_snap(g_snap, (int)sizeof(g_snap));
    TEST_ASSERT(strstr(g_snap, "media=playing:stream1") != NULL);

    run_line("media-stop");
}

/* US-050: pm-ide typed on the desktop opens the full-screen editor. */
static char g_fake_file[512];
static int g_fake_len = -1;
static int g_fake_exec_action;
static int fake_load(const char *file, char *buf, int max) {
    (void)file;
    if (g_fake_len < 0 || g_fake_len > max) return -1;
    memcpy(buf, g_fake_file, (size_t)g_fake_len);
    return g_fake_len;
}
static int fake_save(const char *file, const char *buf, int len) {
    (void)file;
    if (len >= (int)sizeof(g_fake_file)) return -1;
    memcpy(g_fake_file, buf, (size_t)len);
    g_fake_file[len] = 0;
    g_fake_len = len;
    return 0;
}
static int fake_exec(int action, const char *file, const char *src, int len, char *msg, int max) {
    (void)file; (void)src; (void)len;
    g_fake_exec_action = action;
    if (action == OSUI_IDE_RUN) osui_gui_ide_output("1");   /* the program's say */
    strncpy(msg, action == OSUI_IDE_CHECK ? "ok statements 2" : "ok line 2 prints 1 expects 1 failed 0", (size_t)max - 1);
    msg[max - 1] = 0;
    return 0;
}
static void feed_str(const char *s) {
    while (*s) osui_gui_feed_key(*s++, g_out, (int)sizeof(g_out));
}
static void test_gui_pm_ide_fullscreen(void) {
    int i;
    setup();
    osui_gui_set_ide_ops(fake_load, fake_save, fake_exec);
    strcpy(g_fake_file, "say 1\n");
    g_fake_len = 6;
    feed_str("pm-ide t.pm");
    osui_gui_feed_key('\n', g_out, (int)sizeof(g_out));
    TEST_ASSERT_NOT_NULL(strstr(g_out, "osui pm-ide open fullscreen file=t.pm"));
    TEST_ASSERT_EQUAL(1, osui_gui_ide_active());
    osui_gui_fill_scene(&g_scene);
    TEST_ASSERT_EQUAL(OS_FB_PANE_IDE, g_scene.pane);
    TEST_ASSERT_EQUAL(OSUI_IDE_FRAME, g_scene.nterm);
    TEST_ASSERT_EQUAL(0, strncmp(g_scene.term[0], "pm-ide t.pm Ln 1 Col 1 /1", 25));
    TEST_ASSERT_EQUAL_STRING("  1> say 1", g_scene.term[1]); /* cursor on s: Ln/Col in the title */
    /* scene v3 carries the syntax classes: gutter, keyword, number (v4
     * keeps them and adds the web display list, Lot 6) */
    TEST_ASSERT_EQUAL(4, g_scene.version);
    TEST_ASSERT_EQUAL(OSUI_IDE_TOK_GUTTER, g_scene.term_attr[1][0]);
    TEST_ASSERT_EQUAL(OSUI_IDE_TOK_KEYWORD, g_scene.term_attr[1][5]);
    TEST_ASSERT_EQUAL(OSUI_IDE_TOK_NUMBER, g_scene.term_attr[1][9]);
    /* Arrows and ESC belong to the editor, not to the chat or the desktop. */
    osui_gui_feed_key(OS_VGA_KEY_DOWN, g_out, (int)sizeof(g_out));
    for (i = 0; i < 5; i++) osui_gui_feed_key(OS_VGA_KEY_RIGHT, g_out, (int)sizeof(g_out));
    osui_gui_feed_key('\n', g_out, (int)sizeof(g_out));
    feed_str("expect 1 == 1");
    osui_gui_write_snap(g_snap, (int)sizeof(g_snap));
    TEST_ASSERT_NOT_NULL(strstr(g_snap, " ide=t.pm ide_ln=2 ide_col=14 ide_lines=2 ide_mod=1"));
    TEST_ASSERT_NOT_NULL(strstr(g_snap, "cursor_row=  2> expect 1 == 1_"));
    TEST_ASSERT_EQUAL(0, osui_gui_feed_key(OS_VGA_KEY_ESC, g_out, (int)sizeof(g_out)));
    TEST_ASSERT_EQUAL(1, osui_gui_ide_active());
    osui_gui_feed_key('c', g_out, (int)sizeof(g_out));
    TEST_ASSERT_EQUAL(OSUI_IDE_CHECK, g_fake_exec_action);
    TEST_ASSERT_NOT_NULL(strstr(g_out, "osui pm-ide check ok statements 2"));
    osui_gui_feed_key(OS_VGA_KEY_ESC, g_out, (int)sizeof(g_out));
    osui_gui_feed_key('q', g_out, (int)sizeof(g_out));
    TEST_ASSERT_EQUAL(1, osui_gui_ide_active());   /* unsaved: refused */
    osui_gui_feed_key(OS_VGA_KEY_ESC, g_out, (int)sizeof(g_out));
    osui_gui_feed_key('w', g_out, (int)sizeof(g_out));
    TEST_ASSERT_NOT_NULL(strstr(g_out, "osui pm-ide saved file=t.pm"));
    TEST_ASSERT_EQUAL_STRING("say 1\nexpect 1 == 1\n", g_fake_file);
    osui_gui_feed_key(OS_VGA_KEY_ESC, g_out, (int)sizeof(g_out));
    osui_gui_feed_key('r', g_out, (int)sizeof(g_out));
    TEST_ASSERT_EQUAL(OSUI_IDE_RUN, g_fake_exec_action);
    TEST_ASSERT_NOT_NULL(strstr(g_out, "osui pm-ide run ok line 2"));
    TEST_ASSERT_NOT_NULL(strstr(g_out, " output 1"));
    osui_gui_write_snap(g_snap, (int)sizeof(g_snap));
    TEST_ASSERT_NOT_NULL(strstr(g_snap, " ide_out=1 ide_pane=1 "));
    osui_gui_fill_scene(&g_scene);
    TEST_ASSERT_EQUAL_STRING("---- output 1 line (ESC o hides) ----", g_scene.term[OSUI_IDE_VIEW - OSUI_IDE_OUT_VIEW]);
    TEST_ASSERT_EQUAL_STRING("    : 1", g_scene.term[OSUI_IDE_VIEW - OSUI_IDE_OUT_VIEW + 1]);
    TEST_ASSERT_EQUAL(OSUI_IDE_TOK_OUTPUT, g_scene.term_attr[OSUI_IDE_VIEW - OSUI_IDE_OUT_VIEW + 1][6]);
    /* a check does not touch the output pane */
    osui_gui_feed_key(OS_VGA_KEY_ESC, g_out, (int)sizeof(g_out));
    osui_gui_feed_key('c', g_out, (int)sizeof(g_out));
    osui_gui_write_snap(g_snap, (int)sizeof(g_snap));
    TEST_ASSERT_NOT_NULL(strstr(g_snap, " ide_out=1 ide_pane=1 "));
    osui_gui_feed_key(OS_VGA_KEY_ESC, g_out, (int)sizeof(g_out));
    osui_gui_feed_key('q', g_out, (int)sizeof(g_out));
    TEST_ASSERT_NOT_NULL(strstr(g_out, "osui pm-ide quit file=t.pm"));
    TEST_ASSERT_EQUAL(0, osui_gui_ide_active());
    osui_gui_fill_scene(&g_scene);
    TEST_ASSERT_TRUE(g_scene.pane != OS_FB_PANE_IDE);
    /* Missing file opens an empty buffer; no file name is a usage error. */
    g_fake_len = -1;
    feed_str("pm-ide n.pm");
    osui_gui_feed_key('\n', g_out, (int)sizeof(g_out));
    TEST_ASSERT_NOT_NULL(strstr(g_out, "open fullscreen new file=n.pm"));
    osui_gui_feed_key(OS_VGA_KEY_ESC, g_out, (int)sizeof(g_out));
    osui_gui_feed_key('q', g_out, (int)sizeof(g_out));
    feed_str("pm-ide");
    osui_gui_feed_key('\n', g_out, (int)sizeof(g_out));
    TEST_ASSERT_NOT_NULL(strstr(g_out, "error=usage"));
    TEST_ASSERT_EQUAL(0, osui_gui_ide_active());
    osui_gui_set_ide_ops(0, 0, 0);
}

/* Lot 6 (US-031, US-040, US-041, US-042): "web FILE" on the desktop runs
 * the page pipeline (components, @media, var()) and the guest web engine,
 * and the scene carries the display list for the web pane. */
static void test_gui_web_pane_pipeline(void) {
    uint32_t i;
    int red = 0, bold_yo = 0;
    setup();
    osui_gui_set_ide_ops(fake_load, fake_save, fake_exec);
    strcpy(g_fake_file,
           "<style>:root{--c:#ff0000} #a{background:var(--c);height:10px}"
           " @media (max-width:100px){#a{background:#00ff00}}</style>"
           "<template id=x-hi><b><slot></slot></b></template><div id=a>x</div><x-hi>yo</x-hi>");
    g_fake_len = (int)strlen(g_fake_file);
    feed_str("web t.html");
    osui_gui_feed_key('\n', g_out, (int)sizeof(g_out));
    TEST_ASSERT_NOT_NULL(strstr(g_out, "osui web render file=t.html"));
    TEST_ASSERT_NOT_NULL(strstr(g_out, " components 1"));
    TEST_ASSERT_NOT_NULL(strstr(g_out, " media_kept 0 media_dropped 1"));
    TEST_ASSERT_NOT_NULL(strstr(g_out, " vars 1"));
    TEST_ASSERT_EQUAL(1, osui_gui_web_active());
    osui_gui_fill_scene(&g_scene);
    TEST_ASSERT_EQUAL(OS_FB_PANE_WEB, g_scene.pane);
    TEST_ASSERT_EQUAL(4, g_scene.version);
    TEST_ASSERT_TRUE(g_scene.nweb > 0);
    TEST_ASSERT_NOT_NULL(strstr(g_scene.web_title, "web t.html"));
    for (i = 0; i < g_scene.nweb; i++) {
        if (g_scene.web[i].kind == OS_FB_WEB_RECT && g_scene.web[i].color == 0xFF0000U && g_scene.web[i].h == 10) red = 1;
        if (g_scene.web[i].kind == OS_FB_WEB_TEXT && strcmp(g_scene.web[i].text, "yo") == 0 && g_scene.web[i].bold) bold_yo = 1;
    }
    TEST_ASSERT_EQUAL(1, red);
    TEST_ASSERT_EQUAL(1, bold_yo);
    osui_gui_feed_key(OS_VGA_KEY_DOWN, g_out, (int)sizeof(g_out));
    osui_gui_feed_key(OS_VGA_KEY_ESC, g_out, (int)sizeof(g_out));
    TEST_ASSERT_NOT_NULL(strstr(g_out, "osui web close file=t.html"));
    TEST_ASSERT_EQUAL(0, osui_gui_web_active());
    osui_gui_fill_scene(&g_scene);
    TEST_ASSERT_TRUE(g_scene.pane != OS_FB_PANE_WEB);
    g_fake_len = -1;
    feed_str("web missing.html");
    osui_gui_feed_key('\n', g_out, (int)sizeof(g_out));
    TEST_ASSERT_NOT_NULL(strstr(g_out, "osui web error=not_found file=missing.html"));
    TEST_ASSERT_EQUAL(0, osui_gui_web_active());
    osui_gui_set_ide_ops(0, 0, 0);
}

/* Lot 9 (US-077): "mobile-ui ARGS" on the desktop goes to the hook, the
 * hook's display list fills the web pane, typed keys still reach the input
 * line (taps), ESC closes. */
static char g_mobile_args[64];
static int g_mobile_calls;
static int fake_mobile(const char *args, char *out, int out_max) {
    os_fb_web_item_t it[2];
    memset(it, 0, sizeof(it));
    strncpy(g_mobile_args, args, sizeof(g_mobile_args) - 1);
    g_mobile_calls++;
    it[0].kind = OS_FB_WEB_RECT; it[0].w = 320; it[0].h = 24; it[0].color = 0x101010U;
    it[1].kind = OS_FB_WEB_TEXT; it[1].w = 48; it[1].h = 8; it[1].color = 0xFFFFFFU; strcpy(it[1].text, "mobile");
    osui_gui_show_items("phone home", it, 2, 320, 448);
    snprintf(out, (size_t)out_max, "mobile-ui fake %s\n", args);
    return 0;
}

static void test_gui_mobile_pane(void) {
    setup();
    g_mobile_calls = 0;
    feed_str("mobile-ui 320x448");
    osui_gui_feed_key('\n', g_out, (int)sizeof(g_out));
    TEST_ASSERT_EQUAL(0, g_mobile_calls);          /* no hook: not taken */
    TEST_ASSERT_EQUAL(0, osui_gui_mobile_active());
    osui_gui_set_mobile_op(fake_mobile);
    feed_str("mobile-ui 320x448");
    osui_gui_feed_key('\n', g_out, (int)sizeof(g_out));
    TEST_ASSERT_EQUAL(1, g_mobile_calls);
    TEST_ASSERT_EQUAL_STRING("320x448", g_mobile_args);
    TEST_ASSERT_EQUAL(1, osui_gui_mobile_active());
    osui_gui_fill_scene(&g_scene);
    TEST_ASSERT_EQUAL(OS_FB_PANE_WEB, g_scene.pane);
    TEST_ASSERT_EQUAL(2, g_scene.nweb);
    TEST_ASSERT_EQUAL(448, g_scene.web_page_h);
    TEST_ASSERT_NOT_NULL(strstr(g_scene.web_title, "web mobile - phone home"));
    TEST_ASSERT_EQUAL_STRING("mobile", g_scene.web[1].text);
    feed_str("mobile-ui tap 40 60");
    osui_gui_fill_scene(&g_scene);
    TEST_ASSERT_NOT_NULL(strstr(g_scene.input, "mobile-ui tap 40 60"));
    osui_gui_feed_key('\n', g_out, (int)sizeof(g_out));
    TEST_ASSERT_EQUAL(2, g_mobile_calls);
    TEST_ASSERT_EQUAL_STRING("tap 40 60", g_mobile_args);
    TEST_ASSERT_NOT_NULL(strstr(g_out, "mobile-ui fake tap 40 60"));
    osui_gui_feed_key(OS_VGA_KEY_ESC, g_out, (int)sizeof(g_out));
    TEST_ASSERT_NOT_NULL(strstr(g_out, "osui web close file=mobile"));
    TEST_ASSERT_EQUAL(0, osui_gui_mobile_active());
    TEST_ASSERT_EQUAL(0, osui_gui_web_active());
    osui_gui_set_mobile_op(0);
}

/* Lot 7 (US-039): the page sanitizer runs first in the web pipeline:
 * script, iframe, on* handlers, javascript: URLs and unsafe CSS never
 * reach the engine nor the display list. */
static void test_gui_web_pipeline_sanitized(void) {
    uint32_t i;
    int pwned = 0, clean = 0, red = 0;
    setup();
    osui_gui_set_ide_ops(fake_load, fake_save, fake_exec);
    strcpy(g_fake_file,
           "<style>@import 'e.css'; #a{background:#ff0000;height:6px} #b{background:url(http://t/x)}</style>"
           "<div id=a onclick=\"x()\">clean</div><a href=\"javascript:x()\">l</a>"
           "<iframe src=\"http://e/\"></iframe><script>document.write('pwned')</script>");
    g_fake_len = (int)strlen(g_fake_file);
    feed_str("web h.html");
    osui_gui_feed_key('\n', g_out, (int)sizeof(g_out));
    TEST_ASSERT_NOT_NULL(strstr(g_out, "osui web render file=h.html"));
    TEST_ASSERT_NOT_NULL(strstr(g_out, " sanitized 6"));
    TEST_ASSERT_EQUAL(6, osui_gui_web_sanitized());
    osui_gui_fill_scene(&g_scene);
    for (i = 0; i < g_scene.nweb; i++) {
        if (g_scene.web[i].kind == OS_FB_WEB_TEXT && strstr(g_scene.web[i].text, "pwned")) pwned = 1;
        if (g_scene.web[i].kind == OS_FB_WEB_TEXT && strcmp(g_scene.web[i].text, "clean") == 0) clean = 1;
        if (g_scene.web[i].kind == OS_FB_WEB_RECT && g_scene.web[i].color == 0xFF0000U && g_scene.web[i].h == 6) red = 1;
    }
    TEST_ASSERT_EQUAL(0, pwned);
    TEST_ASSERT_EQUAL(1, clean);
    TEST_ASSERT_EQUAL(1, red);
    osui_gui_feed_key(OS_VGA_KEY_ESC, g_out, (int)sizeof(g_out));
    osui_gui_set_ide_ops(0, 0, 0);
}

int main(void) {
    unity_init();
    RUN_TEST(test_gui_command_canonical);
    RUN_TEST(test_vga_pointer_not_ascii_desktop);
    RUN_TEST(test_scene_center_and_float);
    RUN_TEST(test_snap_center_and_float);
    RUN_TEST(test_snap_stage_kind);
    RUN_TEST(test_gui_feed_slash_and_esc);
    RUN_TEST(test_gui_live_shell_eval);
    RUN_TEST(test_vbe_p2p_collab_prod_panes);
    RUN_TEST(test_gui_media_rendering);
    RUN_TEST(test_gui_pm_ide_fullscreen);
    RUN_TEST(test_gui_web_pane_pipeline);
    RUN_TEST(test_gui_web_pipeline_sanitized);
    RUN_TEST(test_gui_mobile_pane);
    unity_print_results();
    unity_cleanup();
    return (unity_stats.tests_failed == 0) ? 0 : 1;
}
