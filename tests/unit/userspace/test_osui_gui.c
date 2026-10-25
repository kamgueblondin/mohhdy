#include <stdio.h>
/* test_osui_gui.c - Commande gui, bureau VBE QEMU, chat float. */

#include "../../framework/unity.h"
#include "../../framework/test_kernel.h"
#include "osui_runtime.h"
#include "osui_gui.h"
#include "osui_tools.h"
#include "os_syscalls.h"
#include "osui_ide.h"
#include "mockfs.h"
#include "umalloc.h"
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
    (void)osui_gui_take_pending(0, 0);
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

/* UI test: ESC with a half typed line clears it and keeps the desktop;
 * the next ESC (empty line) leaves the desktop as before. */
static void test_gui_esc_clears_input_first(void) {
    int rc;
    setup();
    osui_gui_set_input("hello wor");
    memset(g_out, 0, sizeof(g_out));
    rc = osui_gui_feed_key(OS_VGA_KEY_ESC, g_out, (int)sizeof(g_out));
    TEST_ASSERT_EQUAL(0, rc);
    TEST_ASSERT_EQUAL_STRING("", osui_gui_input());
    TEST_ASSERT(strstr(g_out, "osui gui input cleared") != NULL);
    TEST_ASSERT(strstr(g_out, "gui exit") == NULL);
    /* typing works again after the clear */
    osui_gui_feed_key('a', g_out, (int)sizeof(g_out));
    TEST_ASSERT_EQUAL_STRING("a", osui_gui_input());
    osui_gui_feed_key('\b', g_out, (int)sizeof(g_out));
    memset(g_out, 0, sizeof(g_out));
    rc = osui_gui_feed_key(OS_VGA_KEY_ESC, g_out, (int)sizeof(g_out));
    TEST_ASSERT_EQUAL(1, rc);
    TEST_ASSERT(strstr(g_out, "gui exit") != NULL);
}

/* UI test: the shell pane shows text, not raw color codes, and a line
 * that only held a color reset is not an empty row. */
static void test_gui_term_strips_ansi(void) {
    char term[80];
    setup();
    osui_gui_term_reset();
    osui_gui_term_push("\x1b[35m[IA] \x1b[0mquestion: hi\n\x1b[0m\nnext");
    TEST_ASSERT_EQUAL(2, osui_gui_term_count());
    osui_gui_term_at(0, term, (int)sizeof(term));
    TEST_ASSERT_EQUAL_STRING("[IA] question: hi", term);
    osui_gui_term_at(1, term, (int)sizeof(term));
    TEST_ASSERT_EQUAL_STRING("next", term);
    /* plain empty lines are kept */
    osui_gui_term_reset();
    osui_gui_term_push("a\n\nb");
    TEST_ASSERT_EQUAL(3, osui_gui_term_count());
}

/* UI test: the scene names the llm of the last chat turn (version 5)
 * and the chat window gets the newest messages. */
static void test_gui_scene_llm_and_newest_messages(void) {
    int i;
    char line[32];
    setup();
    osui_gui_fill_scene(&g_scene);
    TEST_ASSERT_EQUAL(7, g_scene.version);
    TEST_ASSERT_EQUAL_STRING("stub_echo", g_scene.llm);
    for (i = 0; i < 5; i++) {
        snprintf(line, sizeof(line), "chat note%d", i);
        TEST_ASSERT_EQUAL(0, run_line(line));
    }
    TEST_ASSERT_EQUAL(10, osui_msg_count());
    osui_gui_fill_scene(&g_scene);
    TEST_ASSERT_EQUAL(OS_FB_MSG_MAX, g_scene.nmsg);
    TEST_ASSERT_EQUAL_STRING("note4", g_scene.messages[OS_FB_MSG_MAX - 2]);
    TEST_ASSERT_EQUAL_STRING("note2", g_scene.messages[0]);
}

/* UI test: an unknown word typed in the chat box gets a visible hint in
 * the chat window, not only on the serial line. */
static void test_gui_unknown_command_hint(void) {
    char msg[OS_FB_MSG_LEN];
    int n;
    setup();
    osui_gui_set_input("bonjour");
    memset(g_out, 0, sizeof(g_out));
    TEST_ASSERT_EQUAL(0, osui_gui_feed_key('\n', g_out, (int)sizeof(g_out)));
    TEST_ASSERT(strstr(g_out, "commande inconnue") != NULL);
    n = osui_msg_count();
    TEST_ASSERT(n >= 1);
    osui_msg_at(n - 1, msg, (int)sizeof(msg));
    TEST_ASSERT(strstr(msg, "commande inconnue: bonjour") != NULL);
    TEST_ASSERT(strstr(msg, "ai <question>") != NULL);
}

/* The Shell is the console: "/shell" ends the desktop loop and asks for
 * the console terminal window; the desktop keeps no line editor, history
 * or command table of its own for it. */
static void test_gui_shell_opens_console_terminal(void) {
    char pending[96];
    int rc;
    setup();
    osui_gui_set_input("/shell");
    rc = osui_gui_feed_key('\n', g_out, (int)sizeof(g_out));
    TEST_ASSERT_EQUAL(1, rc);
    TEST_ASSERT_EQUAL(1, osui_gui_term_requested());
    TEST_ASSERT(strstr(g_out, "osui gui term chrome=vbe_term same_console=true") != NULL);
    TEST_ASSERT_EQUAL(0, osui_gui_take_pending(pending, (int)sizeof(pending)));
    TEST_ASSERT_EQUAL_STRING("", pending);
    TEST_ASSERT_EQUAL(0, osui_gui_term_requested());
    TEST_ASSERT(strcmp(osui_get_pane(), "shell") != 0); /* no empty shell pane left */
}

/* A shell line typed on the desktop is not run by the desktop: it is
 * handed to the console prompt (pending), which runs it with shell.c. */
static void test_gui_shell_pane_uses_console_shell(void) {
    static const char *const lines[] = {
        "help", "ls", "p2p-status", "ai hello", "net-status", "gui-status",
        "session-list", "/chat", "fs-list /", "pm-check ide.pm", "bonjour", 0
    };
    char pending[96];
    int i;
    for (i = 0; lines[i]; i++)
        TEST_ASSERT_EQUAL(OSUI_GUI_ROUTE_SHELL, osui_gui_route_line(lines[i], "shell"));
    TEST_ASSERT_EQUAL(OSUI_GUI_ROUTE_HOST, osui_gui_route_line("console", "shell"));
    TEST_ASSERT_EQUAL(OSUI_GUI_ROUTE_HOST, osui_gui_route_line("gui-exit", "shell"));
    TEST_ASSERT_EQUAL(OSUI_GUI_ROUTE_HOST, osui_gui_route_line("web /web/demo.html", "shell"));
    TEST_ASSERT_EQUAL(OSUI_GUI_ROUTE_HOST, osui_gui_route_line("pm-ide x.pm", "shell"));
    TEST_ASSERT_EQUAL(OSUI_GUI_ROUTE_OSUI, osui_gui_route_line("session-list", "none"));
    TEST_ASSERT_EQUAL(OSUI_GUI_ROUTE_SHELL, osui_gui_route_line("ai hello", "none"));
    TEST_ASSERT_EQUAL(OSUI_GUI_ROUTE_SHELL, osui_gui_route_line("!ls", "none"));

    setup();
    osui_gui_set_input("ai hello");
    TEST_ASSERT_EQUAL(1, osui_gui_feed_key('\n', g_out, (int)sizeof(g_out)));
    TEST_ASSERT_EQUAL(1, osui_gui_take_pending(pending, (int)sizeof(pending)));
    TEST_ASSERT_EQUAL_STRING("ai hello", pending);
    setup();
    osui_gui_set_input("!p2p-status");
    TEST_ASSERT_EQUAL(1, osui_gui_feed_key('\n', g_out, (int)sizeof(g_out)));
    TEST_ASSERT_EQUAL(1, osui_gui_take_pending(pending, (int)sizeof(pending)));
    TEST_ASSERT_EQUAL_STRING("p2p-status", pending);
    /* other desktop lines stay on the desktop */
    setup();
    osui_gui_set_input("/browser");
    TEST_ASSERT_EQUAL(0, osui_gui_feed_key('\n', g_out, (int)sizeof(g_out)));
    TEST_ASSERT_EQUAL(0, osui_gui_term_requested());
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
    osui_stage_tick(g_out, (int)sizeof(g_out));
    osui_gui_render(g_cells);
    osui_gui_ascii_row(g_cells, 9, g_row, (int)sizeof(g_row));
    TEST_ASSERT(strstr(g_row, "MEDIA PLAY stream1") != NULL);
    TEST_ASSERT(strstr(g_row, "type=audio") != NULL);
    TEST_ASSERT(strstr(g_row, "ticks=1") != NULL);

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
     * keeps them and adds the web display list, Lot 6; v5 adds llm; v6 pane_live; v7 app windows) */
    TEST_ASSERT_EQUAL(7, g_scene.version);
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
    TEST_ASSERT_EQUAL(7, g_scene.version);
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

static void test_lot12_vfs_and_browser_inspection_views(void) {
    char term[80];
    setup();
    osui_gui_term_reset();
    osui_gui_browser_inspect();
    TEST_ASSERT(osui_gui_term_count() >= 3);
    osui_gui_term_at(0, term, (int)sizeof(term));
    TEST_ASSERT(strstr(term, "Browser-OS Native Panel") != NULL);

    osui_gui_term_reset();
    osui_gui_vfs_inspect("/overlay/");
    TEST_ASSERT(osui_gui_term_count() >= 3);
    osui_gui_term_at(0, term, (int)sizeof(term));
    TEST_ASSERT(strstr(term, "VFS Sandbox Inspection") != NULL);
    TEST_ASSERT(strstr(term, "/overlay/") != NULL);

    TEST_ASSERT_EQUAL(0, run_line("/browser"));
    TEST_ASSERT_EQUAL_STRING("browser", osui_get_pane());

    TEST_ASSERT_EQUAL(0, run_line("/fs"));
    TEST_ASSERT_EQUAL_STRING("fs", osui_get_pane());
}

/* Statut / Fichiers / Admin windows carry live lines from the guest
 * syscalls (osui_panes.c); other panes keep the shell log in term[]. */
static unsigned pl_t;
static unsigned pl_ticks(void) { return pl_t; }
static int pl_list(const char *p, os_dirent_t *o, int m) {
    (void)p; (void)m;
    strcpy(o[0].name, "etc"); o[0].flags = OS_DIRENT_DIR; o[0].size = 0;
    return 1;
}
static int pl_svc(const char *n, os_service_status_t *st) {
    if (strcmp(n, "vfs") == 0) { st->owner_pid = 4; st->queued_messages = 0; return 0; }
    return -1;
}
static void test_gui_live_panes(void) {
    static const osui_pane_src_t src = { pl_list, 0, 0, pl_ticks, pl_svc, 0 };
    setup();
    osui_gui_set_pane_source(&src);
    pl_t = 100 * 61;
    feed_str("/status");
    osui_gui_feed_key('\n', g_out, (int)sizeof(g_out));
    osui_gui_fill_scene(&g_scene);
    TEST_ASSERT_EQUAL(OS_FB_PANE_STATUS, g_scene.pane);
    TEST_ASSERT_EQUAL(1, g_scene.pane_live);
    TEST_ASSERT_EQUAL_STRING("uptime 0:01:01  (PIT ticks 6100)", g_scene.term[0]);
    TEST_ASSERT_EQUAL_STRING("memoire: indisponible", g_scene.term[1]);
    /* refreshed about once a second, not on every frame */
    pl_t += 50;
    osui_gui_fill_scene(&g_scene);
    TEST_ASSERT_EQUAL_STRING("uptime 0:01:01  (PIT ticks 6100)", g_scene.term[0]);
    pl_t += 60;
    osui_gui_fill_scene(&g_scene);
    TEST_ASSERT_EQUAL_STRING("uptime 0:01:02  (PIT ticks 6210)", g_scene.term[0]);
    feed_str("/fs");
    osui_gui_feed_key('\n', g_out, (int)sizeof(g_out));
    osui_gui_fill_scene(&g_scene);
    TEST_ASSERT_EQUAL(OS_FB_PANE_FS, g_scene.pane);
    TEST_ASSERT_EQUAL_STRING("ls /  (1 entrees, sys_listdir)", g_scene.term[0]);
    TEST_ASSERT_EQUAL_STRING("  d  etc/", g_scene.term[1]);
    feed_str("/admin");
    osui_gui_feed_key('\n', g_out, (int)sizeof(g_out));
    osui_gui_fill_scene(&g_scene);
    TEST_ASSERT_EQUAL_STRING("  vfs: actif pid 4 file 0", g_scene.term[1]);
    /* UX3: Browser-OS lists the local pages of web-render */
    feed_str("/browser");
    osui_gui_feed_key('\n', g_out, (int)sizeof(g_out));
    osui_gui_fill_scene(&g_scene);
    TEST_ASSERT_EQUAL(1, g_scene.pane_live);
    TEST_ASSERT_EQUAL_STRING("Pages locales /web : tapez web-render /web/<page>", g_scene.term[0]);
    feed_str("/center");
    osui_gui_feed_key('\n', g_out, (int)sizeof(g_out));
    osui_gui_fill_scene(&g_scene);
    TEST_ASSERT_EQUAL(0, g_scene.pane_live);
    osui_gui_set_pane_source(0);
}

static void evt(const char *line) {
    char b[128];
    int i;
    snprintf(b, sizeof(b), "%c%s\n", OS_FB_EVT, line);
    g_out[0] = 0;
    for (i = 0; b[i]; i++) osui_gui_feed_key((unsigned char)b[i], g_out + strlen(g_out), (int)(sizeof(g_out) - strlen(g_out)));
}

/* Scene v7: file explorer and text editor windows over the shared file
 * library; kernel events (OS_FB_EVT) drive them, keys follow the focus. */
static void test_gui_apps_explorer_editor(void) {
    setup();
    mfs_reset();
    osui_gui_set_fs_io(&mfs_io);
    feed_str("/files /home");
    osui_gui_feed_key('\n', g_out, (int)sizeof(g_out));
    TEST_ASSERT_NOT_NULL(strstr(g_out, "osui files open dir=/home"));
    TEST_ASSERT_EQUAL(0, osui_gui_focus());
    osui_gui_fill_scene(&g_scene);
    TEST_ASSERT_EQUAL(OS_FB_APP_FILES, g_scene.app[0].kind);
    TEST_ASSERT_EQUAL(0, g_scene.app_focus);
    TEST_ASSERT_NOT_NULL(strstr(g_scene.app[0].rows[2], "notes.txt"));
    TEST_ASSERT_NOT_NULL(strstr(g_scene.app[0].tool, "[Infos]"));
    /* keys go to the explorer: new folder */
    feed_str("d"); feed_str("docs");
    g_out[0] = 0;
    osui_gui_feed_key('\n', g_out, (int)sizeof(g_out));
    TEST_ASSERT_NOT_NULL(strstr(g_out, "osui files mkdir ok /home/docs"));
    TEST_ASSERT(mfs_find("/home/docs") >= 0);
    TEST_ASSERT_EQUAL_STRING("", osui_gui_input());     /* not typed on the desktop */
    /* double click on notes.txt (row 3: docs is first) opens the editor */
    evt("app 0 dclick 3 4");
    TEST_ASSERT_NOT_NULL(strstr(g_out, "osui edit open file=/home/notes.txt"));
    TEST_ASSERT_EQUAL(1, osui_gui_focus());
    osui_gui_fill_scene(&g_scene);
    TEST_ASSERT_EQUAL(OS_FB_APP_EDIT, g_scene.app[1].kind);
    TEST_ASSERT_EQUAL_STRING("   1 ligne 1", g_scene.app[1].rows[0]);
    /* edit and save: same bytes as cat would show */
    osui_gui_feed_key(0x13, g_out, (int)sizeof(g_out));
    feed_str(" ok");
    osui_gui_fill_scene(&g_scene);
    TEST_ASSERT_EQUAL(1, g_scene.app[1].dirty);
    osui_gui_feed_key(27, g_out, (int)sizeof(g_out));
    g_out[0] = 0;
    osui_gui_feed_key('w', g_out, (int)sizeof(g_out));
    TEST_ASSERT_NOT_NULL(strstr(g_out, "osui edit saved file=/home/notes.txt"));
    TEST_ASSERT_EQUAL(19, mfs[mfs_find("/home/notes.txt")].len);
    TEST_ASSERT(memcmp(mfs[mfs_find("/home/notes.txt")].data, "ligne 1 ok\nligne 2\n", 19) == 0);
    /* toolbar click: "[Cherch]" opens the find prompt */
    evt("app 1 tool 70");
    osui_gui_fill_scene(&g_scene);
    TEST_ASSERT_NOT_NULL(strstr(g_scene.app[1].status, "Chercher:"));
    osui_gui_feed_key(27, g_out, (int)sizeof(g_out));
    /* focus back to the desktop: keys go to the input line */
    evt("focus d");
    feed_str("ab");
    TEST_ASSERT_EQUAL_STRING("ab", osui_gui_input());
    osui_gui_feed_key('\b', g_out, (int)sizeof(g_out));
    osui_gui_feed_key('\b', g_out, (int)sizeof(g_out));
    /* close with unsaved changes asks once */
    evt("focus 1");
    feed_str("z");
    evt("app 1 close");
    TEST_ASSERT_NOT_NULL(strstr(g_out, "osui edit close refused unsaved"));
    evt("app 1 close");
    TEST_ASSERT_NOT_NULL(strstr(g_out, "osui edit close file=/home/notes.txt"));
    osui_gui_fill_scene(&g_scene);
    TEST_ASSERT_EQUAL(0, g_scene.app[1].kind);
    /* read-only initrd: error in the editor status, file unchanged */
    feed_str("x");   /* focus went back to the explorer */
    evt("focus d");
    feed_str("/edit /ro/readme.txt");
    osui_gui_feed_key('\n', g_out, (int)sizeof(g_out));
    feed_str("Z");
    osui_gui_feed_key(27, g_out, (int)sizeof(g_out));
    g_out[0] = 0;
    osui_gui_feed_key('w', g_out, (int)sizeof(g_out));
    TEST_ASSERT_NOT_NULL(strstr(g_out, "osui edit save error=protege (initrd)"));
    TEST_ASSERT_EQUAL(14, mfs[mfs_find("/ro/readme.txt")].len);
    evt("app 1 close"); evt("app 1 close");
    evt("app 0 close");
    TEST_ASSERT_EQUAL(-1, osui_gui_focus());
}

/* UX4: tool apps in generic slots: several windows at once, open-with
 * from the explorer, ESC r from the editor, chat streaming on idle ticks. */
static char g_tcmds[512];
static int tg_run(const char *cmd, char *out, int max) {
    strcat(g_tcmds, cmd); strcat(g_tcmds, "|");
    snprintf(out, (size_t)max, "%s ok\n", cmd);
    return (int)strlen(out);
}
static int tg_steps;
static int tg_ai(const char *prompt, int first, char *piece, int max) {
    (void)prompt; (void)max;
    if (first) tg_steps = 0;
    if (tg_steps++ >= 2) return 0;
    strcpy(piece, tg_steps == 1 ? "Bonjour" : " le monde, ceci est une reponse.");
    return (int)strlen(piece);
}
static const char *tg_model(void) { return "gpt2.gguf"; }
static void test_gui_tool_apps_slots(void) {
    int i, guard = 0;
    setup();
    mfs_reset();
    g_tcmds[0] = 0;
    osui_gui_set_fs_io(&mfs_io);
    osui_gui_set_tool_ops(tg_run, tg_ai, tg_model, 0);
    feed_str("/files /home");
    osui_gui_feed_key('\n', g_out, (int)sizeof(g_out));
    evt("focus d");
    feed_str("/tasks");
    g_out[0] = 0;
    osui_gui_feed_key('\n', g_out, (int)sizeof(g_out));
    TEST_ASSERT_NOT_NULL(strstr(g_out, "osui tool open app=Gestionnaire des taches"));
    TEST_ASSERT(strstr(g_tcmds, "ps|mem|") != 0);
    TEST_ASSERT_EQUAL(1, osui_gui_focus());
    evt("focus d");
    feed_str("/settings");
    osui_gui_feed_key('\n', g_out, (int)sizeof(g_out));
    evt("focus d");
    feed_str("/peers");
    osui_gui_feed_key('\n', g_out, (int)sizeof(g_out));
    evt("focus d");
    feed_str("/chat");
    osui_gui_feed_key('\n', g_out, (int)sizeof(g_out));
    osui_gui_fill_scene(&g_scene);
    TEST_ASSERT_EQUAL(OS_FB_APP_FILES, g_scene.app[0].kind);
    TEST_ASSERT_EQUAL(OS_FB_APP_TASKS, g_scene.app[1].kind);
    TEST_ASSERT_EQUAL(OS_FB_APP_SETTINGS, g_scene.app[2].kind);
    TEST_ASSERT_EQUAL(OS_FB_APP_P2P, g_scene.app[3].kind);
    TEST_ASSERT_EQUAL(OS_FB_APP_CHAT, g_scene.app[4].kind);
    TEST_ASSERT_EQUAL(4, g_scene.app_focus);
    TEST_ASSERT_NOT_NULL(strstr(g_scene.app[1].tool, "[Tuer]"));
    /* chat: typed into the focused window, streamed on idle ticks */
    feed_str("Salut");
    osui_gui_feed_key('\n', g_out, (int)sizeof(g_out));
    g_out[0] = 0;
    TEST_ASSERT_EQUAL(1, osui_gui_idle(g_out, (int)sizeof(g_out)));
    osui_gui_fill_scene(&g_scene);
    {
        int found = 0;
        for (i = 0; i < OS_FB_APP_ROWS; i++) if (strstr(g_scene.app[4].rows[i], "IA: Bonjour")) found = 1;
        TEST_ASSERT(found);
    }
    while (guard++ < 5 && !strstr(g_out, "osui chat reply=")) osui_gui_idle(g_out, (int)sizeof(g_out));
    TEST_ASSERT_NOT_NULL(strstr(g_out, "osui chat reply=IA: Bonjour le monde"));
    /* a sixth app fits, a seventh is refused */
    evt("focus d");
    feed_str("/view /home/notes.txt");
    osui_gui_feed_key('\n', g_out, (int)sizeof(g_out));
    osui_gui_fill_scene(&g_scene);
    TEST_ASSERT_EQUAL(OS_FB_APP_VIEW, g_scene.app[5].kind);
    TEST_ASSERT_NOT_NULL(strstr(g_scene.app[5].rows[0], "ligne 1"));
    evt("focus d");
    feed_str("/pmrun /home/a.pm");
    g_out[0] = 0;
    osui_gui_feed_key('\n', g_out, (int)sizeof(g_out));
    TEST_ASSERT_NOT_NULL(strstr(g_out, "osui app error=no_slot"));
    /* close the viewer and the chat by their window events */
    evt("app 5 close");
    evt("app 4 close");
    osui_gui_fill_scene(&g_scene);
    TEST_ASSERT_EQUAL(0, g_scene.app[5].kind);
    TEST_ASSERT_EQUAL(0, g_scene.app[4].kind);
    /* explorer "open with": v opens the selected file in the viewer */
    evt("focus 0");
    osui_gui_feed_key(31, g_out, (int)sizeof(g_out));   /* down to notes.txt */
    g_out[0] = 0;
    osui_gui_feed_key('v', g_out, (int)sizeof(g_out));
    TEST_ASSERT_NOT_NULL(strstr(g_out, "osui tool open app=Visionneuse"));
    /* editor ESC r runs the saved file in the PromptMessage window */
    evt("app 4 close");
    evt("focus d");
    feed_str("/edit /home/notes.txt");
    osui_gui_feed_key('\n', g_out, (int)sizeof(g_out));
    osui_gui_feed_key(27, g_out, (int)sizeof(g_out));
    g_tcmds[0] = 0;
    g_out[0] = 0;
    osui_gui_feed_key('r', g_out, (int)sizeof(g_out));
    TEST_ASSERT_NOT_NULL(strstr(g_out, "osui tool open app=PromptMessage"));
    TEST_ASSERT(strstr(g_tcmds, "pm-run /home/notes.txt|") != 0);
    for (i = 0; i < OS_FB_APPS; i++) { char e[24]; snprintf(e, sizeof(e), "app %d close", i); evt(e); evt(e); }
    osui_gui_set_tool_ops(0, 0, 0, 0);
}

/* UX5 GUI parity: a family panel opens from its slash command, runs the
 * selected entry through the shell run hook, a panel argument prefills
 * the input line and the generic /cmd window asks the registry. */
static int tg_known(const char *name) { return !strcmp(name, "ata-status") || !strcmp(name, "uptime") || !strcmp(name, "net-status"); }
static int tg_run_checked(const char *cmd, char *out, int max) {
    if (osui_tool_cmd_check(cmd, tg_known) != OSUI_CMD_OK) return -1;
    return tg_run(cmd, out, max);
}
static void test_gui_parity_panels(void) {
    setup();
    mfs_reset();
    g_tcmds[0] = 0;
    osui_gui_set_fs_io(&mfs_io);
    osui_gui_set_tool_ops(tg_run_checked, tg_ai, tg_model, 0);
    osui_gui_set_tool_known(tg_known);
    feed_str("/disk");
    g_out[0] = 0;
    osui_gui_feed_key('\n', g_out, (int)sizeof(g_out));
    TEST_ASSERT_NOT_NULL(strstr(g_out, "osui tool open app=Stockage"));
    TEST_ASSERT_NOT_NULL(strstr(g_out, "osui tool line=Pilote ATA"));
    TEST_ASSERT_EQUAL(0, (int)strlen(g_tcmds));
    osui_gui_fill_scene(&g_scene);
    TEST_ASSERT_EQUAL(OS_FB_APP_STORE, g_scene.app[0].kind);
    TEST_ASSERT_NOT_NULL(strstr(g_scene.app[0].tool, "[Executer]"));
    g_out[0] = 0;
    osui_gui_feed_key('\n', g_out, (int)sizeof(g_out));
    TEST_ASSERT_EQUAL_STRING("ata-status|", g_tcmds);
    TEST_ASSERT_NOT_NULL(strstr(g_out, "osui panel run cmd=ata-status ok"));
    TEST_ASSERT_NOT_NULL(strstr(g_out, "osui panel out=ata-status ok"));
    /* generic window: the argument prefills the input line */
    evt("focus d");
    feed_str("/cmd uptime");
    osui_gui_feed_key('\n', g_out, (int)sizeof(g_out));
    osui_gui_fill_scene(&g_scene);
    TEST_ASSERT_EQUAL(OS_FB_APP_CMD, g_scene.app[1].kind);
    TEST_ASSERT_NOT_NULL(strstr(g_scene.app[1].rows[OS_FB_APP_ROWS - 1], "cmd> uptime"));
    g_out[0] = 0;
    osui_gui_feed_key('\n', g_out, (int)sizeof(g_out));
    TEST_ASSERT_NOT_NULL(strstr(g_out, "osui panel run cmd=uptime ok"));
    feed_str("reboot");
    g_out[0] = 0;
    osui_gui_feed_key('\n', g_out, (int)sizeof(g_out));
    TEST_ASSERT_NOT_NULL(strstr(g_out, "osui panel run cmd=reboot refused"));
    TEST_ASSERT(strstr(g_tcmds, "reboot") == 0);
    evt("app 1 close"); evt("app 1 close");
    evt("app 0 close"); evt("app 0 close");
    osui_gui_set_tool_known(0);
    osui_gui_set_tool_ops(0, 0, 0, 0);
}

/* UX5: viewer image sent in pixels to the compositor (once per image,
 * cleared on close), files over 8 KB through the heap; the editor grid
 * follows "app K size R C". */
const os_fb_image_t *osui_gui_test_image(int *calls);
static unsigned char g_gui_arena[1 << 20] __attribute__((aligned(16)));
static int gui_grow(unsigned want) { return want <= sizeof(g_gui_arena) ? (int)((want + 4095U) & ~4095U) : -1; }
static void test_gui_viewer_pixels_and_editor_size(void) {
    static char pgm[9000];
    int n, i, calls0, calls1, slot;
    const os_fb_image_t *img;
    setup();
    mfs_reset();
    umalloc_init(g_gui_arena, gui_grow);
    osui_gui_set_heap(urealloc);
    osui_gui_set_fs_io(&mfs_io);
    osui_gui_set_tool_ops(tg_run, tg_ai, tg_model, 0);
    n = sprintf(pgm, "P5 100 84 255\n");
    for (i = 0; i < 100 * 84; i++) pgm[n + i] = (char)(i % 100 < 50 ? 10 : 240);
    mfs_add("/home/big.pgm", 0, pgm, n + 8400);
    evt("focus d");
    feed_str("/view /home/big.pgm");
    g_out[0] = 0;
    osui_gui_feed_key('\n', g_out, (int)sizeof(g_out));
    TEST_ASSERT_NOT_NULL(strstr(g_out, "osui tool line=image 100x84 affichee en pixels 100x84"));
    (void)osui_gui_test_image(&calls0);
    osui_gui_fill_scene(&g_scene);
    img = osui_gui_test_image(&calls1);
    TEST_ASSERT_EQUAL(calls0 + 1, calls1);
    TEST_ASSERT_NOT_NULL(img);
    slot = -1;
    for (i = 0; i < OS_FB_APPS; i++) if (g_scene.app[i].kind == OS_FB_APP_VIEW) slot = i;
    TEST_ASSERT_EQUAL(slot, img->slot);
    TEST_ASSERT_EQUAL(OS_FB_IMG_MAGIC, (int)img->magic);
    TEST_ASSERT_EQUAL(100, img->w);
    TEST_ASSERT_EQUAL(84, img->h);
    TEST_ASSERT_EQUAL(10, img->rgb[0]);
    TEST_ASSERT_EQUAL(240, img->rgb[99 * 3]);
    osui_gui_fill_scene(&g_scene);                 /* unchanged: not sent again */
    (void)osui_gui_test_image(&calls0);
    TEST_ASSERT_EQUAL(calls1, calls0);
    {
        char e[24];
        snprintf(e, sizeof(e), "app %d close", slot);
        evt(e);
    }
    osui_gui_fill_scene(&g_scene);
    img = osui_gui_test_image(&calls1);
    TEST_ASSERT_NULL(img);                         /* closed: cleared */
    TEST_ASSERT_EQUAL(calls0 + 1, calls1);
    /* editor: default 22 rows, then the window grid from the kernel */
    evt("focus d");
    feed_str("/edit /home/notes.txt");
    osui_gui_feed_key('\n', g_out, (int)sizeof(g_out));
    osui_gui_fill_scene(&g_scene);
    slot = -1;
    for (i = 0; i < OS_FB_APPS; i++) if (g_scene.app[i].kind == OS_FB_APP_EDIT) slot = i;
    TEST_ASSERT(slot >= 0);
    TEST_ASSERT_EQUAL(OSUI_APP_ROWS, g_scene.app[slot].nrows);
    {
        char e[40];
        snprintf(e, sizeof(e), "app %d size 40 120", slot);
        evt(e);
        TEST_ASSERT_NOT_NULL(strstr(g_out, "osui edit view resized"));
    }
    osui_gui_fill_scene(&g_scene);
    TEST_ASSERT_EQUAL(40, g_scene.app[slot].nrows);
    for (i = 0; i < OS_FB_APPS; i++) { char e[24]; snprintf(e, sizeof(e), "app %d close", i); evt(e); evt(e); }
    osui_gui_set_tool_ops(0, 0, 0, 0);
    osui_gui_set_heap(0);
}

/* UX4: browser links (guest web engine hit test) and back / forward */
static int web_pages_load(const char *file, char *buf, int max) {
    const char *r = 0;
    if (!strcmp(file, "/web/index.html")) r = "<h1>Accueil</h1><p><a href=\"page2.html\">Page deux</a></p><p><a href=\"javascript:x()\">Script</a></p><p><a href=\"http://example.com/\">Distant</a></p>";
    else if (!strcmp(file, "/web/page2.html")) r = "<h1>Deux</h1><p><a href=\"/web/index.html\">Accueil</a></p>";
    if (!r || (int)strlen(r) > max) return -1;
    memcpy(buf, r, strlen(r));
    return (int)strlen(r);
}
static int web_item(const char *text, int *x, int *y) {
    int i;
    osui_gui_fill_scene(&g_scene);
    for (i = 0; i < (int)g_scene.nweb; i++)
        if (g_scene.web[i].kind == OS_FB_WEB_TEXT && strstr(g_scene.web[i].text, text)) { *x = g_scene.web[i].x + 3; *y = g_scene.web[i].y + 3; return 1; }
    return 0;
}
static void test_gui_web_links_history(void) {
    char e[48], d[64];
    int x, y;
    setup();
    osui_gui_set_ide_ops(web_pages_load, fake_save, fake_exec);
    feed_str("web /web/index.html");
    osui_gui_feed_key('\n', g_out, (int)sizeof(g_out));
    TEST_ASSERT_NOT_NULL(strstr(g_out, "osui web render file=/web/index.html"));
    TEST_ASSERT(web_item("Page deux", &x, &y));
    snprintf(e, sizeof(e), "web click %d %d", x, y);
    evt(e);
    TEST_ASSERT_NOT_NULL(strstr(g_out, "osui web link href=page2.html"));
    TEST_ASSERT_NOT_NULL(strstr(g_out, "osui web render file=/web/page2.html"));
    TEST_ASSERT(web_item("Deux", &x, &y));
    /* a click outside any link does nothing */
    evt("web click 900 5");
    TEST_ASSERT_EQUAL_STRING("", g_out);
    evt("web back");
    TEST_ASSERT_NOT_NULL(strstr(g_out, "osui web back file=/web/index.html"));
    evt("web back");
    TEST_ASSERT_NOT_NULL(strstr(g_out, "osui web back none"));
    evt("web fwd");
    TEST_ASSERT_NOT_NULL(strstr(g_out, "osui web fwd file=/web/page2.html"));
    evt("web back");
    /* a javascript: link is dropped by the sanitizer: no navigation */
    TEST_ASSERT(web_item("Script", &x, &y));
    snprintf(e, sizeof(e), "web click %d %d", x, y);
    evt(e);
    TEST_ASSERT(strstr(g_out, "osui web render") == 0);
    /* remote schemes are refused: local pages only */
    TEST_ASSERT(web_item("Distant", &x, &y));
    snprintf(e, sizeof(e), "web click %d %d", x, y);
    evt(e);
    TEST_ASSERT_NOT_NULL(strstr(g_out, "osui web link refused href=http://example.com/"));
    TEST_ASSERT(osui_gui_web_resolve("/web/a/b.html", "../c.html", d, (int)sizeof(d)) == 0);
    TEST_ASSERT_EQUAL_STRING("/web/c.html", d);
    TEST_ASSERT(osui_gui_web_resolve("/web/a.html", "http://x/y", d, (int)sizeof(d)) < 0);
    TEST_ASSERT(osui_gui_web_resolve("/web/a.html", "#top", d, (int)sizeof(d)) < 0);
    osui_gui_set_ide_ops(0, 0, 0);
    feed_str("/center");
    osui_gui_feed_key('\n', g_out, (int)sizeof(g_out));
}

int main(void) {
    unity_init();
    RUN_TEST(test_lot12_vfs_and_browser_inspection_views);
    RUN_TEST(test_gui_command_canonical);
    RUN_TEST(test_vga_pointer_not_ascii_desktop);
    RUN_TEST(test_scene_center_and_float);
    RUN_TEST(test_snap_center_and_float);
    RUN_TEST(test_snap_stage_kind);
    RUN_TEST(test_gui_feed_slash_and_esc);
    RUN_TEST(test_gui_esc_clears_input_first);
    RUN_TEST(test_gui_term_strips_ansi);
    RUN_TEST(test_gui_scene_llm_and_newest_messages);
    RUN_TEST(test_gui_unknown_command_hint);
    RUN_TEST(test_gui_shell_opens_console_terminal);
    RUN_TEST(test_gui_shell_pane_uses_console_shell);
    RUN_TEST(test_vbe_p2p_collab_prod_panes);
    RUN_TEST(test_gui_media_rendering);
    RUN_TEST(test_gui_pm_ide_fullscreen);
    RUN_TEST(test_gui_web_pane_pipeline);
    RUN_TEST(test_gui_web_pipeline_sanitized);
    RUN_TEST(test_gui_mobile_pane);
    RUN_TEST(test_gui_live_panes);
    RUN_TEST(test_gui_apps_explorer_editor);
    RUN_TEST(test_gui_tool_apps_slots);
    RUN_TEST(test_gui_viewer_pixels_and_editor_size);
    RUN_TEST(test_gui_parity_panels);
    RUN_TEST(test_gui_web_links_history);
    unity_print_results();
    unity_cleanup();
    return (unity_stats.tests_failed == 0) ? 0 : 1;
}
