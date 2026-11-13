#include <stdio.h>
/* test_osui_gui.c - Commande gui, bureau VBE QEMU, chat float. */

#include "../../framework/unity.h"
#include "../../framework/test_kernel.h"
#include "osui_runtime.h"
#include "osui_gui.h"
#include "webeng.h"
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
    TEST_ASSERT_EQUAL(8, g_scene.version); /* Lot 68: v8 text scale */
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
    /* Lot 69: "/search WORDS" is the Internet search, bare "/search" the Recherche panel */
    TEST_ASSERT_EQUAL(OSUI_GUI_ROUTE_HOST, osui_gui_route_line("/search mohhdy", "chat"));
    TEST_ASSERT_TRUE(osui_gui_route_line("/search", "chat") != OSUI_GUI_ROUTE_HOST);
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
    TEST_ASSERT_EQUAL(8, g_scene.version); /* Lot 68: v8 text scale */
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
    TEST_ASSERT_EQUAL(8, g_scene.version); /* Lot 68: v8 text scale */
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


/* Lot 72: a local UTF-8 page with Greek and Cyrillic: the source becomes
 * display bytes run by run (page reset at every tag), the engine gives
 * TEXT items with their page in pad, and the scene keeps the page bytes
 * as they are (Cyrillic "TA" is C2 B0, a valid UTF-8 pair) */
static void test_gui_web_glyph_pages(void) {
    uint32_t i;
    int greek = 0, cyr = 0, fr = 0;
    char src[96];
    int n;
    strcpy(src, "<p title=\"\xce\xb1\">\xce\xb2</p><p>\xce\xb3</p>");
    n = osui_gui_web_src_to_display(src, (int)strlen(src));
    TEST_ASSERT_EQUAL_STRING("<p title=\"\x8f\xe1\">\x8f\xe2</p><p>\x8f\xe3</p>", src);
    TEST_ASSERT_EQUAL((int)strlen(src), n);
    setup();
    osui_gui_set_ide_ops(fake_load, fake_save, fake_exec);
    strcpy(g_fake_file, "<p>\xd0\xa2\xd0\x90\xd0\xa1\xd0\xa1</p><p>\xce\x95\xce\xbb\xce\xbb\xce\xac\xce\xb4\xce\xb1</p><p>\xc3\xa9t\xc3\xa9</p>");
    g_fake_len = (int)strlen(g_fake_file);
    feed_str("web g.html");
    osui_gui_feed_key('\n', g_out, (int)sizeof(g_out));
    TEST_ASSERT_EQUAL(1, osui_gui_web_active());
    osui_gui_fill_scene(&g_scene);
    for (i = 0; i < g_scene.nweb; i++) {
        const os_fb_web_item_t *it = &g_scene.web[i];
        if (it->kind != OS_FB_WEB_TEXT) continue;
        if (strcmp(it->text, "\xc2\xb0\xc1\xc1") == 0 && ((it->pad >> 8) & 7) == 3) cyr = 1;
        if (strcmp(it->text, "\xc5\xeb\xeb\xdc\xe4\xe1") == 0 && ((it->pad >> 8) & 7) == 2) greek = 1;
        if (strcmp(it->text, "\xe9t\xe9") == 0 && ((it->pad >> 8) & 7) == 0) fr = 1;
    }
    TEST_ASSERT_EQUAL(1, cyr);
    TEST_ASSERT_EQUAL(1, greek);
    TEST_ASSERT_EQUAL(1, fr);
    osui_gui_feed_key(OS_VGA_KEY_ESC, g_out, (int)sizeof(g_out));
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
    TEST_ASSERT_EQUAL_STRING("Barre d'adresse en haut : URL ou mots. Pages /web :", g_scene.term[0]);
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
    static char pgm[FSOPS_FILE_MAX + 9000];
    int n, i, calls0, calls1, slot;
    const os_fb_image_t *img;
    setup();
    mfs_reset();
    umalloc_init(g_gui_arena, gui_grow);
    osui_gui_set_heap(urealloc);
    osui_gui_set_fs_io(&mfs_io);
    osui_gui_set_tool_ops(tg_run, tg_ai, tg_model, 0);
    /* a header comment pads the file past FSOPS_FILE_MAX: the heap path */
    n = sprintf(pgm, "P5\n# ");
    for (i = 0; i < FSOPS_FILE_MAX; i++) pgm[n++] = 'c';
    n += sprintf(pgm + n, "\n100 84 255\n");
    for (i = 0; i < 100 * 84; i++) pgm[n + i] = (char)(i % 100 < 50 ? 10 : 240);
    TEST_ASSERT(n + 8400 > FSOPS_FILE_MAX);
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

/* Lot 69: a desktop click arrives as an event; it runs as its own line and
 * the half-typed chat line stays (it used to become "//center"). */
static void test_lot69_desktop_cmd_event_keeps_typed_line(void) {
    setup();
    osui_gui_set_input("");
    feed_str("/browser\n");
    TEST_ASSERT_EQUAL_STRING("float", osui_get_chat_mode());
    feed_str("/");
    TEST_ASSERT_EQUAL_STRING("/", osui_gui_input());
    memset(g_out, 0, sizeof(g_out));
    osui_gui_feed_key(OS_FB_EVT, g_out, (int)sizeof(g_out));
    feed_str("cmd /center\n");
    TEST_ASSERT_EQUAL_STRING("center", osui_get_chat_mode());
    TEST_ASSERT_EQUAL_STRING("/", osui_gui_input());
    TEST_ASSERT_NULL(strstr(g_out, "commande inconnue"));
    osui_gui_set_input("");
}

/* Lot 69: in Browser-OS a line that is not a command goes to the address
 * bar: words become a search, an URL is opened. */
/* Lot 69: the shared close button of the full screen web window sends
 * "cmd /center": it closes the page (keys alone go to the page) and keeps
 * nothing typed into it. */
static void test_lot69_full_window_close_event(void) {
    setup();
    osui_gui_set_ide_ops(fake_load, fake_save, fake_exec);
    strcpy(g_fake_file, "<p>hi</p>");
    g_fake_len = (int)strlen(g_fake_file);
    osui_gui_set_input("");
    feed_str("web t.html");
    osui_gui_feed_key('\n', g_out, (int)sizeof(g_out));
    TEST_ASSERT_EQUAL(1, osui_gui_web_active());
    memset(g_out, 0, sizeof(g_out));
    osui_gui_feed_key(OS_FB_EVT, g_out, (int)sizeof(g_out));
    feed_str("cmd /center\n");
    TEST_ASSERT_EQUAL(0, osui_gui_web_active());
    TEST_ASSERT_NOT_NULL(strstr(g_out, "osui web close file=t.html"));
    osui_gui_set_input("");
}

static void test_lot69_browser_pane_address_line(void) {
    setup();
    osui_gui_set_input("");
    feed_str("/browser\n");
    TEST_ASSERT_EQUAL_STRING("browser", osui_get_pane());
    memset(g_out, 0, sizeof(g_out));
    feed_str("hello world\n");
    TEST_ASSERT_NULL(strstr(g_out, "commande inconnue"));
    /* the search goes to the network loader (none on the host), not to a
     * local file named "hello world" as "web hello world" would */
    TEST_ASSERT_NOT_NULL(strstr(g_out, "osui web error=no_network_loader"));
    TEST_ASSERT_NULL(strstr(g_out, "not_found"));
    TEST_ASSERT_EQUAL_STRING("", osui_gui_input());
    memset(g_out, 0, sizeof(g_out));
    feed_str("/web/index.html\n");      /* a slash line stays a command */
    TEST_ASSERT_NULL(strstr(g_out, "search:"));
    memset(g_out, 0, sizeof(g_out));
    feed_str("https://example.com/\n");
    TEST_ASSERT_NULL(strstr(g_out, "search:"));
    TEST_ASSERT_NOT_NULL(strstr(g_out, "osui web"));
}

/* Lot 71: a fetched page shown by sections, its form field focused by a
 * click, typed into, and submitted with Enter, through the page ops (the
 * shell side, userspace/shell_fetch.c, is tested in test_wf_form and
 * test_wf_section). */
we_doc_t *osui_gui_web_doc(void);
static int g_m_sec, g_m_keys, g_m_submits, g_m_focus = -1, g_m_set;
static char g_m_val[64];
static int m_section_src(int k, char *src, int cap) {
    char tmp[600];
    int n;
    n = snprintf(tmp, sizeof(tmp),
                 "<html><head><title>Sec %d</title></head><body><h1>Section %d</h1>"
                 "<p><a href=\"mohhdy-form:f:0\">[ %s%s ]</a> <a href=\"mohhdy-form:s:1\">[ Envoyer ]</a></p>"
                 "<p>texte</p></body></html>", k + 1, k + 1, g_m_val, g_m_focus == 0 ? "_" : "");
    if (n >= cap) return -1;
    memcpy(src, tmp, (size_t)n + 1);
    return n;
}
static int m_fetch(const char *url, char *src, int src_max, char *final_url, int url_max, char *status, int st_max) {
    (void)url_max; (void)st_max;
    snprintf(final_url, 64, "%s", url);
    snprintf(status, 64, "osui web fetch ok fake");
    g_m_sec = 0;
    return m_section_src(0, src, src_max);
}
static int m_sections(void) { return 2; }
static int m_current(void) { return g_m_sec; }
static int m_section(int k, char *src, int cap) { g_m_sec = k; return m_section_src(k, src, cap); }
static int m_form_link(const char *href) {
    if (strcmp(href, "mohhdy-form:f:0") == 0) { g_m_focus = 0; return 0; }
    if (strcmp(href, "mohhdy-form:s:1") == 0) return 1;
    return -1;
}
static int m_form_key(int key) {
    size_t l = strlen(g_m_val);
    if (g_m_focus < 0) return -1;
    g_m_keys++;
    if (key == '\n') return 1;
    if (key == OS_VGA_KEY_ESC) { g_m_focus = -1; return 2; }
    if (key == '\b') { if (l) g_m_val[l - 1] = 0; return 0; }
    if (key >= 0x20 && l + 1 < sizeof(g_m_val)) { g_m_val[l] = (char)key; g_m_val[l + 1] = 0; return 0; }
    return -1;
}
static int m_form_set(int n, const char *v) { if (n != 0) return -1; snprintf(g_m_val, sizeof(g_m_val), "%s", v); g_m_set++; return 0; }
static int m_submit(int n, char *src, int src_max, char *final_url, int url_max, char *status, int st_max) {
    (void)n; (void)url_max; (void)st_max;
    g_m_submits++;
    g_m_focus = -1;
    snprintf(final_url, 96, "https://example.com/search?q=%s", g_m_val);
    snprintf(status, 64, "osui web fetch ok fake method GET");
    return snprintf(src, (size_t)src_max, "<html><head><title>Resultats</title></head><body><p>ok %s</p></body></html>", g_m_val);
}
static int m_flist(char *out, int cap) { return snprintf(out, (size_t)cap, "osui web forms 1 fields 2\n"); }
static int find_link(const char *want, int *x, int *y) {
    char href[128];
    int px, py;
    for (py = 0; py < 300; py += 2)
        for (px = 0; px < 600; px += 2)
            if (we_link_at(osui_gui_web_doc(), px, py, href, (int)sizeof(href)) > 0 && strcmp(href, want) == 0) {
                *x = px; *y = py; return 1;
            }
    return 0;
}
static void test_lot71_web_sections_and_form(void) {
    static const osui_web_page_ops_t ops = { m_sections, m_current, m_section, m_form_link, m_form_key,
                                             m_form_set, m_submit, m_flist };
    char ev[64];
    int x, y, i;
    setup();
    osui_gui_set_web_fetch(m_fetch);
    osui_gui_set_web_page_ops(&ops);
    g_m_val[0] = 0; g_m_focus = -1; g_m_keys = g_m_submits = g_m_set = 0;
    memset(g_out, 0, sizeof(g_out));
    TEST_ASSERT_EQUAL(1, osui_gui_web_command("web-open https://example.com/", g_out, (int)sizeof(g_out)));
    TEST_ASSERT_EQUAL(1, osui_gui_web_active());
    TEST_ASSERT_NOT_NULL(strstr(g_out, "osui web page sections 2"));
    memset(g_out, 0, sizeof(g_out));
    osui_gui_web_command("web-section", g_out, (int)sizeof(g_out));
    TEST_ASSERT_NOT_NULL(strstr(g_out, "osui web section 1/2"));
    /* click the field: focus, then typing goes to it, not to the input line */
    TEST_ASSERT(find_link("mohhdy-form:f:0", &x, &y));
    snprintf(ev, sizeof(ev), "web click %d %d", x, y);
    memset(g_out, 0, sizeof(g_out));
    osui_gui_event(ev, g_out, (int)sizeof(g_out));
    TEST_ASSERT_NOT_NULL(strstr(g_out, "osui web form focus f:0"));
    TEST_ASSERT_EQUAL(0, g_m_focus);
    osui_gui_set_input("");
    for (i = 0; "mohhdy"[i]; i++) osui_gui_feed_key("mohhdy"[i], g_out, (int)sizeof(g_out));
    osui_gui_feed_key('x', g_out, (int)sizeof(g_out));
    osui_gui_feed_key('\b', g_out, (int)sizeof(g_out));
    TEST_ASSERT_EQUAL_STRING("mohhdy", g_m_val);
    TEST_ASSERT_EQUAL_STRING("", osui_gui_input());
    TEST_ASSERT_EQUAL_STRING("Sec 1", osui_gui_web_doc()->title);
    /* Enter submits; the answer page is shown with its final URL */
    memset(g_out, 0, sizeof(g_out));
    osui_gui_feed_key('\n', g_out, (int)sizeof(g_out));
    TEST_ASSERT_EQUAL(1, g_m_submits);
    TEST_ASSERT_NOT_NULL(strstr(g_out, "osui web form submit osui web fetch ok fake method GET"));
    TEST_ASSERT_EQUAL_STRING("Resultats", osui_gui_web_doc()->title);
    /* console path: same code (web-input then web-submit) */
    TEST_ASSERT_EQUAL(1, osui_gui_web_command("web-open https://example.com/", g_out, (int)sizeof(g_out)));
    memset(g_out, 0, sizeof(g_out));
    osui_gui_web_command("web-input 0 caf\xe9", g_out, (int)sizeof(g_out));
    TEST_ASSERT_NOT_NULL(strstr(g_out, "osui web input ok"));
    TEST_ASSERT_EQUAL_STRING("caf\xe9", g_m_val);
    memset(g_out, 0, sizeof(g_out));
    osui_gui_web_command("web-forms", g_out, (int)sizeof(g_out));
    TEST_ASSERT_NOT_NULL(strstr(g_out, "osui web forms 1 fields 2"));
    osui_gui_web_command("web-submit", g_out, (int)sizeof(g_out));
    TEST_ASSERT_EQUAL(2, g_m_submits);
    /* sections: web-section 2, and DOWN past the end of section 1 */
    osui_gui_web_command("web-open https://example.com/", g_out, (int)sizeof(g_out));
    memset(g_out, 0, sizeof(g_out));
    osui_gui_web_command("web-section 2", g_out, (int)sizeof(g_out));
    TEST_ASSERT_NOT_NULL(strstr(g_out, "osui web section 2/2"));
    TEST_ASSERT_EQUAL_STRING("Sec 2", osui_gui_web_doc()->title);
    osui_gui_web_command("web-section 1", g_out, (int)sizeof(g_out));
    memset(g_out, 0, sizeof(g_out));
    for (i = 0; i < 40 && g_m_sec == 0; i++) osui_gui_feed_key(OS_VGA_KEY_DOWN, g_out, (int)sizeof(g_out));
    TEST_ASSERT_EQUAL(1, g_m_sec);
    TEST_ASSERT_NOT_NULL(strstr(g_out, "osui web section 2/2"));
    for (i = 0; i < 40 && g_m_sec == 1; i++) osui_gui_feed_key(OS_VGA_KEY_UP, g_out, (int)sizeof(g_out));
    TEST_ASSERT_EQUAL(0, g_m_sec);
    memset(g_out, 0, sizeof(g_out));
    osui_gui_web_command("web-section 9", g_out, (int)sizeof(g_out));
    TEST_ASSERT_NOT_NULL(strstr(g_out, "error=out_of_range"));
    osui_gui_feed_key(OS_VGA_KEY_ESC, g_out, (int)sizeof(g_out));
    osui_gui_set_web_page_ops(0);
    osui_gui_set_web_fetch(0);
}

/* Lot 73: the GUI advances the animated images before it sends the atlas:
 * a frame change (new generation) resends it, no change sends nothing */
static os_fb_webimg_t g_m_atlas;
static int g_m_ticks, g_m_tick_changes;
static const os_fb_webimg_t *m_atlas(void) { return &g_m_atlas; }
static int m_anim_tick(void) {
    g_m_ticks++;
    if (g_m_tick_changes) { g_m_tick_changes--; g_m_atlas.gen++; return 1; }
    return 0;
}
const os_fb_webimg_t *osui_gui_test_webimg(int *calls);
static void test_lot73_anim_tick_resends_atlas(void) {
    static const osui_web_page_ops_t ops = { m_sections, m_current, m_section, m_form_link, m_form_key,
                                             m_form_set, m_submit, m_flist, 0, m_atlas, 0, m_anim_tick };
    static os_fb_scene_t scene;
    int calls0, calls;
    setup();
    memset(&g_m_atlas, 0, sizeof(g_m_atlas));
    g_m_atlas.magic = OS_FB_WEBIMG_MAGIC;
    g_m_atlas.gen = 5;
    g_m_ticks = 0; g_m_tick_changes = 0;
    osui_gui_set_web_fetch(m_fetch);
    osui_gui_set_web_page_ops(&ops);
    TEST_ASSERT_EQUAL(1, osui_gui_web_command("web-open https://example.com/", g_out, (int)sizeof(g_out)));
    osui_gui_fill_scene(&scene);
    TEST_ASSERT_TRUE(g_m_ticks >= 1);
    TEST_ASSERT_TRUE(osui_gui_test_webimg(&calls0) == &g_m_atlas);
    osui_gui_fill_scene(&scene);
    osui_gui_test_webimg(&calls);
    TEST_ASSERT_EQUAL(calls0, calls);                 /* same generation: not resent */
    g_m_tick_changes = 1;
    osui_gui_fill_scene(&scene);
    osui_gui_test_webimg(&calls);
    TEST_ASSERT_EQUAL(calls0 + 1, calls);             /* new frame: resent */
    TEST_ASSERT_EQUAL(6U, g_m_atlas.gen);
    osui_gui_fill_scene(&scene);
    osui_gui_test_webimg(&calls);
    TEST_ASSERT_EQUAL(calls0 + 1, calls);
    /* every scene asks: one tick per fill */
    g_m_ticks = 0;
    osui_gui_fill_scene(&scene);
    osui_gui_fill_scene(&scene);
    TEST_ASSERT_EQUAL(2, g_m_ticks);
    osui_gui_set_web_page_ops(0);
    osui_gui_set_web_fetch(0);
}

/* Lot 74: forms of the rendered page (webeng_finput.inc) in the GUI */
int osui_gui_form_url(const char *base, const char *action, char *dst, int max);
static char g_f_url[600], g_f_body[1100], g_f_ctype[160];
static int g_f_fetches, g_f_posts;
static const char *F_PAGE =
    "<html><head><title>Form</title></head><body>"
    "<form action=\"/find\" method=\"get\"><input id=q name=q value=\"ab\">"
    "<input type=checkbox id=c name=c value=on><button id=go type=submit>Go</button></form>"
    "<form action=\"post?x=1\" method=\"post\"><input id=p name=p value=\"v w\">"
    "<button id=send type=submit>Send</button></form></body></html>";
static int f_fetch(const char *url, char *src, int src_max, char *final_url, int url_max, char *status, int st_max) {
    (void)url_max; (void)st_max;
    g_f_fetches++;
    snprintf(g_f_url, sizeof(g_f_url), "%s", url);
    snprintf(final_url, 256, "%s", url);
    snprintf(status, 64, "osui web fetch ok fake");
    if (strchr(url, '?')) return snprintf(src, (size_t)src_max, "<html><head><title>Got</title></head><body>r</body></html>");
    return snprintf(src, (size_t)src_max, "%s", F_PAGE);
}
static int f_request(const char *url, const char *body, int body_len, const char *ctype, char *src, int src_max, char *final_url, int url_max,
                     char *status, int st_max) {
    (void)url_max; (void)st_max;
    g_f_posts++;
    snprintf(g_f_url, sizeof(g_f_url), "%s", url);
    snprintf(g_f_body, sizeof(g_f_body), "%.*s", body_len, body);
    snprintf(g_f_ctype, sizeof(g_f_ctype), "%s", ctype ? ctype : "");
    snprintf(final_url, 256, "%s", url);
    snprintf(status, 64, "osui web fetch ok fake method POST");
    if (strcmp(body, "p=fail") == 0) { snprintf(status, 64, "osui web fetch error=http_500"); return -1; }
    return snprintf(src, (size_t)src_max, "<html><head><title>Posted</title></head><body>ok</body></html>");
}
static int f_box(const char *id, int *x, int *y) {
    const we_doc_t *d = osui_gui_web_doc();
    int i;
    for (i = 1; i < (int)d->nnodes; i++)
        if (strcmp(d->nodes[i].id, id) == 0) { *x = d->nodes[i].x + 2; *y = d->nodes[i].y + 2; return 1; }
    return 0;
}
static void test_lot74_form_url(void) {
    char d[300];
    TEST_ASSERT_EQUAL(0, osui_gui_form_url("https://a.b/x/y.html?q#f", "/find?k=1", d, (int)sizeof(d)));
    TEST_ASSERT_EQUAL_STRING("https://a.b/find?k=1", d);
    TEST_ASSERT_EQUAL(0, osui_gui_form_url("https://a.b/x/y.html?q#f", "z?k=1", d, (int)sizeof(d)));
    TEST_ASSERT_EQUAL_STRING("https://a.b/x/z?k=1", d);
    TEST_ASSERT_EQUAL(0, osui_gui_form_url("https://a.b/x/y.html?q#f", "?k=1", d, (int)sizeof(d)));
    TEST_ASSERT_EQUAL_STRING("https://a.b/x/y.html?k=1", d);
    TEST_ASSERT_EQUAL(0, osui_gui_form_url("http://a.b", "z", d, (int)sizeof(d)));
    TEST_ASSERT_EQUAL_STRING("http://a.b/z", d);
    TEST_ASSERT_EQUAL(0, osui_gui_form_url("https://a.b/x", "//c.d/e", d, (int)sizeof(d)));
    TEST_ASSERT_EQUAL_STRING("https://c.d/e", d);
    TEST_ASSERT_EQUAL(0, osui_gui_form_url("https://a.b/x", "http://e.f/g?h", d, (int)sizeof(d)));
    TEST_ASSERT_EQUAL_STRING("http://e.f/g?h", d);
    TEST_ASSERT_EQUAL(0, osui_gui_form_url("https://a.b:8443/x/", "", d, (int)sizeof(d)));
    TEST_ASSERT_EQUAL_STRING("https://a.b:8443/x/", d);
    /* a local page: the query is dropped, the path resolved */
    TEST_ASSERT_EQUAL(0, osui_gui_form_url("/web/a.html", "?q=1", d, (int)sizeof(d)));
    TEST_ASSERT_EQUAL_STRING("/web/a.html", d);
    TEST_ASSERT_EQUAL(0, osui_gui_form_url("/web/a.html", "b.html?q=1", d, (int)sizeof(d)));
    TEST_ASSERT_EQUAL_STRING("/web/b.html", d);
    /* refusals: no base, no room */
    TEST_ASSERT_EQUAL(-1, osui_gui_form_url("", "x", d, (int)sizeof(d)));
    TEST_ASSERT_EQUAL(-1, osui_gui_form_url("https://a.b/", 0, d, (int)sizeof(d)));
    TEST_ASSERT_EQUAL(-1, osui_gui_form_url("https://a.b/", "/abcdefgh", d, 12));
    TEST_ASSERT_EQUAL(-1, osui_gui_form_url("https://a.b/", "https://abcdefgh", d, 10));
}
static void test_lot74_gui_forms(void) {
    static const osui_web_page_ops_t ops = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, f_request };
    char ev[64];
    int x, y, i;
    setup();
    g_f_fetches = g_f_posts = 0;
    osui_gui_set_web_fetch(f_fetch);
    osui_gui_set_web_page_ops(&ops);
    TEST_ASSERT_EQUAL(1, osui_gui_web_command("web-open https://ex.am/dir/page", g_out, (int)sizeof(g_out)));
    TEST_ASSERT_EQUAL_STRING("Form", osui_gui_web_doc()->title);
    memset(g_out, 0, sizeof(g_out));
    osui_gui_web_command("web-fstate", g_out, (int)sizeof(g_out));
    TEST_ASSERT_NOT_NULL(strstr(g_out, "osui web form2 state controls 5 focus -1"));
    TEST_ASSERT_NOT_NULL(strstr(g_out, "ctl 0 input id=q box"));
    TEST_ASSERT_NOT_NULL(strstr(g_out, "ctl 3 input id=p box"));
    /* desktop click on the text field: focus, keys go to it */
    TEST_ASSERT(f_box("q", &x, &y));
    snprintf(ev, sizeof(ev), "web click %d %d", x, y);
    memset(g_out, 0, sizeof(g_out));
    osui_gui_event(ev, g_out, (int)sizeof(g_out));
    TEST_ASSERT_NOT_NULL(strstr(g_out, "osui web form2 changed focus 0"));
    osui_gui_set_input("");
    osui_gui_feed_key('c', g_out, (int)sizeof(g_out));
    osui_gui_feed_key('\b', g_out, (int)sizeof(g_out));
    osui_gui_feed_key('d', g_out, (int)sizeof(g_out));
    TEST_ASSERT_EQUAL_STRING("", osui_gui_input());
    memset(g_out, 0, sizeof(g_out));
    osui_gui_web_command("web-fstate", g_out, (int)sizeof(g_out));
    TEST_ASSERT_NOT_NULL(strstr(g_out, "focus 0"));
    TEST_ASSERT_NOT_NULL(strstr(g_out, "=abd"));
    /* console: tab to the checkbox, space toggles it */
    memset(g_out, 0, sizeof(g_out));
    osui_gui_web_command("web-fkey tab", g_out, (int)sizeof(g_out));
    TEST_ASSERT_NOT_NULL(strstr(g_out, "osui web form2 changed focus 1"));
    osui_gui_web_command("web-fkey space", g_out, (int)sizeof(g_out));
    memset(g_out, 0, sizeof(g_out));
    osui_gui_web_command("web-fkey nosuchkey", g_out, (int)sizeof(g_out));
    TEST_ASSERT_NOT_NULL(strstr(g_out, "error=usage"));
    /* Enter on the page: GET through the fetch path, action resolved */
    memset(g_out, 0, sizeof(g_out));
    TEST_ASSERT(f_box("go", &x, &y));
    snprintf(ev, sizeof(ev), "web-fclick %d %d", x, y);
    osui_gui_web_command(ev, g_out, (int)sizeof(g_out));
    TEST_ASSERT_NOT_NULL(strstr(g_out, "osui web form2 submit GET https://ex.am/find?q=abd&c=on"));
    TEST_ASSERT_EQUAL_STRING("https://ex.am/find?q=abd&c=on", g_f_url);
    TEST_ASSERT_EQUAL_STRING("Got", osui_gui_web_doc()->title);
    /* back: a fresh form state; POST sends the body through request */
    TEST_ASSERT_EQUAL(1, osui_gui_web_command("web-open https://ex.am/dir/page", g_out, (int)sizeof(g_out)));
    memset(g_out, 0, sizeof(g_out));
    osui_gui_web_command("web-fstate", g_out, (int)sizeof(g_out));
    TEST_ASSERT_NOT_NULL(strstr(g_out, "focus -1"));
    TEST_ASSERT_NULL(strstr(g_out, "abd"));
    memset(g_out, 0, sizeof(g_out));
    osui_gui_web_command("web-ftype zz", g_out, (int)sizeof(g_out));
    TEST_ASSERT_NOT_NULL(strstr(g_out, "error=no_focus"));
    TEST_ASSERT(f_box("p", &x, &y));
    snprintf(ev, sizeof(ev), "web-fclick %d %d", x, y);
    osui_gui_web_command(ev, g_out, (int)sizeof(g_out));
    memset(g_out, 0, sizeof(g_out));
    osui_gui_web_command("web-ftype &z", g_out, (int)sizeof(g_out));
    TEST_ASSERT_NOT_NULL(strstr(g_out, "osui web form2 typed 2"));
    memset(g_out, 0, sizeof(g_out));
    osui_gui_web_command("web-fkey enter", g_out, (int)sizeof(g_out));
    TEST_ASSERT_EQUAL(1, g_f_posts);
    TEST_ASSERT_NOT_NULL(strstr(g_out, "submit POST https://ex.am/dir/post?x=1 body p=v+w%26z"));
    TEST_ASSERT_EQUAL_STRING("https://ex.am/dir/post?x=1", g_f_url);
    TEST_ASSERT_EQUAL_STRING("p=v+w%26z", g_f_body);
    /* Lot 75: request passes a content type (shell_page_request / ops.request) */
    TEST_ASSERT_TRUE(g_f_ctype[0] == 0 || strstr(g_f_ctype, "urlencoded") != 0);
    TEST_ASSERT_EQUAL_STRING("Posted", osui_gui_web_doc()->title);
    /* a failing POST keeps the status; no request op: refused */
    osui_gui_web_command("web-open https://ex.am/dir/page", g_out, (int)sizeof(g_out));
    osui_gui_web_command(ev, g_out, (int)sizeof(g_out));
    for (i = 0; i < 3; i++) osui_gui_web_command("web-fkey bs", g_out, (int)sizeof(g_out));
    osui_gui_web_command("web-ftype fail", g_out, (int)sizeof(g_out));
    memset(g_out, 0, sizeof(g_out));
    osui_gui_web_command("web-fkey enter", g_out, (int)sizeof(g_out));
    TEST_ASSERT_NOT_NULL(strstr(g_out, "osui web fetch error=http_500"));
    TEST_ASSERT_EQUAL_STRING("Form", osui_gui_web_doc()->title);
    osui_gui_set_web_page_ops(0);
    memset(g_out, 0, sizeof(g_out));
    osui_gui_web_command("web-fkey enter", g_out, (int)sizeof(g_out));
    TEST_ASSERT_NOT_NULL(strstr(g_out, "osui web form2 error=no_network"));
    /* a click outside any control is not a form action */
    memset(g_out, 0, sizeof(g_out));
    osui_gui_web_command("web-fclick 5000 5000", g_out, (int)sizeof(g_out));
    TEST_ASSERT_NOT_NULL(strstr(g_out, "osui web form2 none"));
    osui_gui_web_command("web-fkey esc", g_out, (int)sizeof(g_out));   /* blur, then ESC leaves the page */
    osui_gui_feed_key(OS_VGA_KEY_ESC, g_out, (int)sizeof(g_out));
    TEST_ASSERT_EQUAL(0, osui_gui_web_active());
    osui_gui_set_web_fetch(0);
}


/* Lot 75: multipart POST passes ctype through ops.request */
static const char *F_MP =
    "<html><head><title>MP</title></head><body>"
    "<form action=\"/up\" method=\"post\" enctype=\"multipart/form-data\">"
    "<input id=m name=msg value=hi><button id=go type=submit>Go</button></form></body></html>";
static int f_fetch_mp(const char *url, char *src, int src_max, char *final_url, int url_max, char *status, int st_max) {
    (void)url_max; (void)st_max; (void)url;
    g_f_fetches++;
    snprintf(final_url, 256, "%s", url);
    snprintf(status, 64, "osui web fetch ok fake");
    return snprintf(src, (size_t)src_max, "%s", F_MP);
}
static void test_lot75_gui_multipart_ctype(void) {
    static const osui_web_page_ops_t ops = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, f_request };
    char ev[64];
    int x, y;
    setup();
    g_f_fetches = g_f_posts = 0;
    g_f_ctype[0] = 0;
    osui_gui_set_web_fetch(f_fetch_mp);
    osui_gui_set_web_page_ops(&ops);
    TEST_ASSERT_EQUAL(1, osui_gui_web_command("web-open https://ex.am/mp", g_out, (int)sizeof(g_out)));
    TEST_ASSERT(f_box("go", &x, &y));
    snprintf(ev, sizeof(ev), "web-fclick %d %d", x, y);
    memset(g_out, 0, sizeof(g_out));
    osui_gui_web_command(ev, g_out, (int)sizeof(g_out));
    TEST_ASSERT_EQUAL(1, g_f_posts);
    TEST_ASSERT_NOT_NULL(strstr(g_f_ctype, "multipart/form-data; boundary="));
    TEST_ASSERT_NOT_NULL(strstr(g_f_body, "name=\"msg\""));
    TEST_ASSERT_NOT_NULL(strstr(g_f_body, "hi"));
    osui_gui_feed_key(OS_VGA_KEY_ESC, g_out, (int)sizeof(g_out));
    osui_gui_set_web_page_ops(0);
    osui_gui_set_web_fetch(0);
}

/* Lot 86: web-load, web-preview, web-fill and web-css reach the shell
 * through the page ops table (no direct shell_fetch.c call from the GUI) */
static char g_l86_last[160];
static int l86_load(const char *path, const char *url, char *status, int st_max) {
    snprintf(g_l86_last, sizeof(g_l86_last), "load|%s|%s", path, url);
    return snprintf(status, (size_t)st_max, "shell page loaded");
}
static int l86_preview(int field, char *out, int cap) {
    snprintf(g_l86_last, sizeof(g_l86_last), "preview|%d", field);
    return snprintf(out, (size_t)cap, "preview GET /q?x=1");
}
static int l86_fill(const char *line, char *out, int cap) {
    snprintf(g_l86_last, sizeof(g_l86_last), "fill|%s", line);
    return snprintf(out, (size_t)cap, "fill ok");
}
static int l86_css(const char *line, char *out, int cap) {
    snprintf(g_l86_last, sizeof(g_l86_last), "css|%s", line);
    return snprintf(out, (size_t)cap, "css ok");
}
static void test_lot86_shell_page_ops_routing(void) {
    static const osui_web_page_ops_t ops = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                                             l86_load, l86_preview, l86_fill, l86_css, 0, 0 };
    static const osui_web_page_ops_t none = { 0 };
    setup();
    /* without the shell entries every command answers, nothing crashes */
    osui_gui_set_web_page_ops(&none);
    memset(g_out, 0, sizeof(g_out));
    TEST_ASSERT_EQUAL(1, osui_gui_web_command("web-load /p.html https://ex.am/p", g_out, (int)sizeof(g_out)));
    TEST_ASSERT_NOT_NULL(strstr(g_out, "osui web load unavailable"));
    memset(g_out, 0, sizeof(g_out));
    TEST_ASSERT_EQUAL(1, osui_gui_web_command("web-preview 2", g_out, (int)sizeof(g_out)));
    TEST_ASSERT_NOT_NULL(strstr(g_out, "osui web preview unavailable"));
    memset(g_out, 0, sizeof(g_out));
    TEST_ASSERT_EQUAL(1, osui_gui_web_command("web-fill", g_out, (int)sizeof(g_out)));
    TEST_ASSERT_NOT_NULL(strstr(g_out, "osui web fill unavailable"));
    memset(g_out, 0, sizeof(g_out));
    TEST_ASSERT_EQUAL(1, osui_gui_web_command("web-css on", g_out, (int)sizeof(g_out)));
    TEST_ASSERT_NOT_NULL(strstr(g_out, "osui web css unavailable"));

    osui_gui_set_web_page_ops(&ops);
    memset(g_out, 0, sizeof(g_out));
    TEST_ASSERT_EQUAL(1, osui_gui_web_command("web-load   /p.html   https://ex.am/p", g_out, (int)sizeof(g_out)));
    TEST_ASSERT_EQUAL_STRING("load|/p.html|https://ex.am/p", g_l86_last);
    TEST_ASSERT_NOT_NULL(strstr(g_out, "shell page loaded"));
    memset(g_out, 0, sizeof(g_out));
    osui_gui_web_command("web-preview 12", g_out, (int)sizeof(g_out));
    TEST_ASSERT_EQUAL_STRING("preview|12", g_l86_last);
    TEST_ASSERT_NOT_NULL(strstr(g_out, "preview GET"));
    osui_gui_web_command("web-preview", g_out, (int)sizeof(g_out));
    TEST_ASSERT_EQUAL_STRING("preview|-1", g_l86_last);
    memset(g_out, 0, sizeof(g_out));
    osui_gui_web_command("web-fill 3 bob", g_out, (int)sizeof(g_out));
    TEST_ASSERT_EQUAL_STRING("fill|web-fill 3 bob", g_l86_last);
    TEST_ASSERT_NOT_NULL(strstr(g_out, "fill ok"));
    memset(g_out, 0, sizeof(g_out));
    osui_gui_web_command("web-css off", g_out, (int)sizeof(g_out));
    TEST_ASSERT_EQUAL_STRING("css|web-css off", g_l86_last);
    TEST_ASSERT_NOT_NULL(strstr(g_out, "css ok"));
    /* web-cssx is not web-css: the prefix needs a space or the end */
    g_l86_last[0] = 0;
    osui_gui_web_command("web-cssx", g_out, (int)sizeof(g_out));
    TEST_ASSERT_EQUAL_STRING("", g_l86_last);
    osui_gui_set_web_page_ops(0);
}

/* Lot 86: a field that takes the focus lists its remembered value, and
 * Down in the focused field fills it instead of scrolling the page */
static int g_l86_offers;
static int l86_fill_offers(const char *line, char *out, int cap) {
    (void)line;
    if (g_l86_offers) return snprintf(out, (size_t)cap, "web-fill field 0 offers 1\noffer 0 q mohhdy\n") > 0 ? 0 : -1;
    return snprintf(out, (size_t)cap, "web-fill field 0 offers 0\n") > 0 ? 0 : -1;
}
static int l86_form_key(int key) {
    if (g_m_focus >= 0 && key == OS_VGA_KEY_DOWN) {
        if (!g_l86_offers) return -1;
        snprintf(g_m_val, sizeof(g_m_val), "mohhdy");
        return 0;
    }
    return m_form_key(key);
}
static void test_lot86_focus_offers_and_down(void) {
    static const osui_web_page_ops_t ops = { m_sections, m_current, m_section, m_form_link, l86_form_key,
                                             m_form_set, m_submit, m_flist, 0, 0, 0, 0, 0,
                                             0, 0, l86_fill_offers, 0, 0, 0 };
    char ev[64];
    int x, y;
    setup();
    osui_gui_set_web_fetch(m_fetch);
    osui_gui_set_web_page_ops(&ops);
    g_m_val[0] = 0; g_m_focus = -1; g_m_keys = g_m_submits = g_m_set = 0;
    /* no remembered value: the focus line only, Down goes on to the page */
    g_l86_offers = 0;
    TEST_ASSERT_EQUAL(1, osui_gui_web_command("web-open https://example.com/", g_out, (int)sizeof(g_out)));
    TEST_ASSERT(find_link("mohhdy-form:f:0", &x, &y));
    snprintf(ev, sizeof(ev), "web click %d %d", x, y);
    memset(g_out, 0, sizeof(g_out));
    osui_gui_event(ev, g_out, (int)sizeof(g_out));
    TEST_ASSERT_NOT_NULL(strstr(g_out, "osui web form focus f:0"));
    TEST_ASSERT_NULL(strstr(g_out, "autofill"));
    osui_gui_feed_key(OS_VGA_KEY_DOWN, g_out, (int)sizeof(g_out));
    TEST_ASSERT_EQUAL_STRING("", g_m_val);
    osui_gui_feed_key(OS_VGA_KEY_ESC, g_out, (int)sizeof(g_out));   /* blur */
    /* a remembered value: listed on focus, Down fills the field */
    g_l86_offers = 1;
    TEST_ASSERT_EQUAL(1, osui_gui_web_command("web-open https://example.com/", g_out, (int)sizeof(g_out)));
    TEST_ASSERT(find_link("mohhdy-form:f:0", &x, &y));
    snprintf(ev, sizeof(ev), "web click %d %d", x, y);
    memset(g_out, 0, sizeof(g_out));
    osui_gui_event(ev, g_out, (int)sizeof(g_out));
    TEST_ASSERT_NOT_NULL(strstr(g_out, "osui web form focus f:0"));
    TEST_ASSERT_NOT_NULL(strstr(g_out, "offer 0 q mohhdy"));
    TEST_ASSERT_NOT_NULL(strstr(g_out, "osui web autofill key=Down"));
    osui_gui_feed_key(OS_VGA_KEY_DOWN, g_out, (int)sizeof(g_out));
    TEST_ASSERT_EQUAL_STRING("mohhdy", g_m_val);
    TEST_ASSERT_EQUAL(0, g_m_sec);   /* the page did not change section */
    memset(g_out, 0, sizeof(g_out));
    osui_gui_feed_key('\n', g_out, (int)sizeof(g_out));
    TEST_ASSERT_EQUAL(1, g_m_submits);
    osui_gui_feed_key(OS_VGA_KEY_ESC, g_out, (int)sizeof(g_out));
    osui_gui_set_web_page_ops(0);
    osui_gui_set_web_fetch(0);
}

/* Lot 87: the rendered page's own controls (form2) offer the remembered
 * value too: a click on the field lists it, Down takes it, Enter sends it.
 * Lot 86 only offered it on the reader's mohhdy-form links. */
static int g_l87_calls;
static char g_l87_url[128], g_l87_name[32];
static int l87_offer(const char *url, const char *name, char *out, int cap) {
    g_l87_calls++;
    snprintf(g_l87_url, sizeof(g_l87_url), "%s", url);
    snprintf(g_l87_name, sizeof(g_l87_name), "%s", name);
    if (strcmp(name, "q") == 0) return snprintf(out, (size_t)cap, "mohhdy os");
    if (strcmp(name, "p") == 0) return snprintf(out, (size_t)cap, "v w");   /* the value it holds */
    out[0] = 0;
    return -1;
}

/* Lot 87: three remembered values for q, newest first; "v w" is what p
 * already holds, so p lists the other two only. */
static int l87_offer_at(const char *url, const char *name, int idx, char *out, int cap) {
    static const char *const q[] = { "mohhdy os", "kernel", "qemu tablet" };
    static const char *const pv[] = { "v w", "old p" };
    (void)url;
    if (strcmp(name, "q") == 0 && idx >= 0 && idx < 3) return snprintf(out, (size_t)cap, "%s", q[idx]);
    if (strcmp(name, "p") == 0 && idx >= 0 && idx < 2) return snprintf(out, (size_t)cap, "%s", pv[idx]);
    out[0] = 0;
    return -1;
}
static void test_lot87_offer_list(void) {
    static const osui_web_page_ops_t ops = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, f_request,
                                             0, 0, 0, 0, l87_offer, l87_offer_at, 0 };
    static os_fb_scene_t scene;
    os_fb_web_item_t it[8];
    char ev[64], st[160];
    int x, y, n, i, texts = 0, hi = 0;
    setup();
    osui_gui_set_web_fetch(f_fetch);
    osui_gui_set_web_page_ops(&ops);
    TEST_ASSERT_EQUAL(1, osui_gui_web_command("web-open https://ex.am/dir/page", g_out, (int)sizeof(g_out)));
    TEST_ASSERT_EQUAL(0, osui_gui_offer_items(it, 8));          /* no focus, no list */
    TEST_ASSERT_EQUAL(-1, osui_gui_offer_state(st, (int)sizeof(st)));
    TEST_ASSERT(f_box("q", &x, &y));
    snprintf(ev, sizeof(ev), "web-fclick %d %d", x, y);
    memset(g_out, 0, sizeof(g_out));
    osui_gui_web_command(ev, g_out, (int)sizeof(g_out));
    TEST_ASSERT_NOT_NULL(strstr(g_out, "osui web autofill offer name=q key=Down values=3"));
    TEST_ASSERT_EQUAL(0, osui_gui_offer_state(st, (int)sizeof(st)));
    TEST_ASSERT_EQUAL_STRING("3 next 0 mohhdy os|kernel|qemu tablet", st);
    /* the drawn list: a white box, its border, one highlight, three rows */
    n = osui_gui_offer_items(it, 8);
    TEST_ASSERT_EQUAL(6, n);
    TEST_ASSERT_EQUAL(OS_FB_WEB_RECT, it[0].kind);
    TEST_ASSERT_EQUAL(OS_FB_WEB_BORDER, it[1].kind);
    TEST_ASSERT_EQUAL(it[0].x, it[1].x);
    TEST_ASSERT(it[0].w >= 11 * 8);                              /* "qemu tablet" fits */
    TEST_ASSERT_EQUAL(3 * 12 + 4, it[0].h);
    for (i = 2; i < n; i++) {
        if (it[i].kind == OS_FB_WEB_TEXT) {
            TEST_ASSERT(it[i].y > it[0].y && it[i].y < it[0].y + (int)it[0].h);
            TEST_ASSERT(it[i].x > it[0].x);
            texts++;
        } else if (it[i].kind == OS_FB_WEB_RECT) hi++;
    }
    TEST_ASSERT_EQUAL(3, texts);
    TEST_ASSERT_EQUAL(1, hi);
    TEST_ASSERT_EQUAL_STRING("mohhdy os", it[3].text);
    TEST_ASSERT_EQUAL(1, it[3].bold);                            /* the next pick */
    /* the list sits under the field (or above it at the page end) */
    TEST_ASSERT(it[0].y > y - 40);
    /* too small a buffer draws nothing rather than half a list */
    TEST_ASSERT_EQUAL(0, osui_gui_offer_items(it, 2));
    /* the scene carries it after the page items */
    memset(&scene, 0, sizeof(scene));
    osui_gui_fill_scene(&scene);
    TEST_ASSERT(scene.nweb >= 6);
    TEST_ASSERT_EQUAL(OS_FB_WEB_RECT, scene.web[scene.nweb - 6].kind);
    TEST_ASSERT_EQUAL_STRING("qemu tablet", scene.web[scene.nweb - 1].text);
    /* Down fills the newest, the list stays; Down again the next one */
    memset(g_out, 0, sizeof(g_out));
    osui_gui_web_command("web-fkey down", g_out, (int)sizeof(g_out));
    TEST_ASSERT_NOT_NULL(strstr(g_out, "osui web autofill applied name=q pick 1/3"));
    memset(g_out, 0, sizeof(g_out));
    osui_gui_web_command("web-fkey down", g_out, (int)sizeof(g_out));
    TEST_ASSERT_NOT_NULL(strstr(g_out, "pick 2/3"));
    memset(g_out, 0, sizeof(g_out));
    osui_gui_web_command("web-fstate", g_out, (int)sizeof(g_out));
    TEST_ASSERT_NOT_NULL(strstr(g_out, "value=kernel"));
    TEST_ASSERT_EQUAL(0, osui_gui_offer_state(st, (int)sizeof(st)));
    TEST_ASSERT_EQUAL_STRING("3 sel 1 mohhdy os|kernel|qemu tablet", st);
    n = osui_gui_offer_items(it, 8);
    for (i = 0; i < n; i++) if (it[i].kind == OS_FB_WEB_TEXT && it[i].bold) break;
    TEST_ASSERT(i < n);
    TEST_ASSERT_EQUAL_STRING("kernel", it[i].text);
    /* Down past the end wraps, Up goes back */
    osui_gui_web_command("web-fkey down", g_out, (int)sizeof(g_out));
    memset(g_out, 0, sizeof(g_out));
    osui_gui_web_command("web-fkey down", g_out, (int)sizeof(g_out));
    TEST_ASSERT_NOT_NULL(strstr(g_out, "pick 1/3"));
    memset(g_out, 0, sizeof(g_out));
    osui_gui_web_command("web-fkey up", g_out, (int)sizeof(g_out));
    TEST_ASSERT_NOT_NULL(strstr(g_out, "pick 3/3"));
    memset(g_out, 0, sizeof(g_out));
    osui_gui_web_command("web-fstate", g_out, (int)sizeof(g_out));
    TEST_ASSERT_NOT_NULL(strstr(g_out, "value=qemu tablet"));
    /* Esc closes the list, the field keeps its text and the focus */
    memset(g_out, 0, sizeof(g_out));
    osui_gui_web_command("web-fkey esc", g_out, (int)sizeof(g_out));
    TEST_ASSERT_NOT_NULL(strstr(g_out, "osui web autofill closed name=q"));
    TEST_ASSERT_EQUAL(0, osui_gui_offer_items(it, 8));
    memset(g_out, 0, sizeof(g_out));
    osui_gui_web_command("web-fstate", g_out, (int)sizeof(g_out));
    TEST_ASSERT_NOT_NULL(strstr(g_out, "value=qemu tablet"));
    /* Down after Esc is the field's own key again */
    memset(g_out, 0, sizeof(g_out));
    osui_gui_web_command("web-fkey down", g_out, (int)sizeof(g_out));
    TEST_ASSERT_NULL(strstr(g_out, "autofill applied"));
    /* Up on a fresh list (nothing taken yet) is not the list's */
    osui_gui_web_command("web-fkey tab", g_out, (int)sizeof(g_out));
    memset(g_out, 0, sizeof(g_out));
    osui_gui_web_command("web-fkey stab", g_out, (int)sizeof(g_out));
    TEST_ASSERT_NOT_NULL(strstr(g_out, "autofill offer name=q"));
    /* the text "qemu tablet" the field holds is left out of the list */
    TEST_ASSERT_NOT_NULL(strstr(g_out, "values=2"));
    memset(g_out, 0, sizeof(g_out));
    osui_gui_web_command("web-fkey up", g_out, (int)sizeof(g_out));
    TEST_ASSERT_NULL(strstr(g_out, "autofill applied"));
    /* the open list covers p (it is drawn over the page and takes the
     * clicks there): Esc closes it first */
    memset(g_out, 0, sizeof(g_out));
    osui_gui_web_command("web-fkey esc", g_out, (int)sizeof(g_out));
    TEST_ASSERT_NOT_NULL(strstr(g_out, "autofill closed name=q"));
    /* p holds "v w": only the older value is offered, alone, so Down
     * fills it and closes the list as in the one-value case */
    TEST_ASSERT(f_box("p", &x, &y));
    snprintf(ev, sizeof(ev), "web-fclick %d %d", x, y);
    memset(g_out, 0, sizeof(g_out));
    osui_gui_web_command(ev, g_out, (int)sizeof(g_out));
    TEST_ASSERT_NOT_NULL(strstr(g_out, "autofill offer name=p key=Down"));
    TEST_ASSERT_NULL(strstr(g_out, "values="));
    memset(g_out, 0, sizeof(g_out));
    osui_gui_web_command("web-fkey down", g_out, (int)sizeof(g_out));
    TEST_ASSERT_NOT_NULL(strstr(g_out, "autofill applied name=p"));
    TEST_ASSERT_NULL(strstr(g_out, "pick"));
    TEST_ASSERT_EQUAL(0, osui_gui_offer_items(it, 8));
    /* a new page drops the list */
    TEST_ASSERT(f_box("q", &x, &y));
    snprintf(ev, sizeof(ev), "web-fclick %d %d", x, y);
    osui_gui_web_command(ev, g_out, (int)sizeof(g_out));
    TEST_ASSERT(osui_gui_offer_items(it, 8) > 0);
    TEST_ASSERT_EQUAL(1, osui_gui_web_command("web-open https://ex.am/dir/page", g_out, (int)sizeof(g_out)));
    TEST_ASSERT_EQUAL(0, osui_gui_offer_items(it, 8));
    osui_gui_web_command("web-fkey esc", g_out, (int)sizeof(g_out));
    osui_gui_feed_key(OS_VGA_KEY_ESC, g_out, (int)sizeof(g_out));
    osui_gui_set_web_page_ops(0);
    osui_gui_set_web_fetch(0);
}

/* Lot 87: the list rows take a tablet click: the row's value goes in and
 * the list closes; a click on the list padding is swallowed; the field
 * under the list is not clicked through. */
/* Lot 87: a store the desktop can forget from (Delete on a row) */
static char g_dq[3][24];
static int g_dqn, g_drops;
static int l87d_offer_at(const char *url, const char *name, int idx, char *out, int cap) {
    (void)url;
    if (strcmp(name, "q") == 0 && idx >= 0 && idx < g_dqn) return snprintf(out, (size_t)cap, "%s", g_dq[idx]);
    out[0] = 0;
    return -1;
}
static int l87d_offer(const char *url, const char *name, char *out, int cap) {
    return l87d_offer_at(url, name, 0, out, cap);
}
static int l87d_drop(const char *url, const char *name, const char *value) {
    int i, k;
    (void)url;
    g_drops++;
    if (strcmp(name, "q") != 0) return -1;
    for (i = 0; i < g_dqn; i++) {
        if (strcmp(g_dq[i], value) != 0) continue;
        for (k = i; k + 1 < g_dqn; k++) memcpy(g_dq[k], g_dq[k + 1], sizeof(g_dq[k]));
        g_dqn--;
        return g_dqn;
    }
    return -1;
}

static void test_lot87_offer_delete(void) {
    static const osui_web_page_ops_t ops = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, f_request,
                                             0, 0, 0, 0, l87d_offer, l87d_offer_at, l87d_drop };
    os_fb_web_item_t it[8];
    char ev[64], st[160];
    int x, y;
    setup();
    snprintf(g_dq[0], sizeof(g_dq[0]), "mohhdy os");
    snprintf(g_dq[1], sizeof(g_dq[1]), "kernel");
    snprintf(g_dq[2], sizeof(g_dq[2]), "qemu tablet");
    g_dqn = 3;
    g_drops = 0;
    osui_gui_set_web_fetch(f_fetch);
    osui_gui_set_web_page_ops(&ops);
    TEST_ASSERT_EQUAL(1, osui_gui_web_command("web-open https://ex.am/dir/page", g_out, (int)sizeof(g_out)));
    /* Delete without an open list is nobody's */
    memset(g_out, 0, sizeof(g_out));
    osui_gui_web_command("web-fkey del", g_out, (int)sizeof(g_out));
    TEST_ASSERT_NOT_NULL(strstr(g_out, "osui web form2 none"));
    TEST_ASSERT(f_box("q", &x, &y));
    snprintf(ev, sizeof(ev), "web-fclick %d %d", x, y);
    memset(g_out, 0, sizeof(g_out));
    osui_gui_web_command(ev, g_out, (int)sizeof(g_out));
    TEST_ASSERT_NOT_NULL(strstr(g_out, "values=3"));
    /* an open list without a highlighted row: Delete forgets nothing */
    memset(g_out, 0, sizeof(g_out));
    osui_gui_web_command("web-fkey del", g_out, (int)sizeof(g_out));
    TEST_ASSERT_NOT_NULL(strstr(g_out, "osui web form2 none"));
    TEST_ASSERT_EQUAL(0, g_drops);
    TEST_ASSERT_EQUAL(0, osui_gui_offer_state(st, (int)sizeof(st)));
    TEST_ASSERT_EQUAL_STRING("3 next 0 mohhdy os|kernel|qemu tablet", st);
    /* highlight kernel, Delete: the row below takes its place and fills */
    osui_gui_web_command("web-fkey down", g_out, (int)sizeof(g_out));
    osui_gui_web_command("web-fkey down", g_out, (int)sizeof(g_out));
    memset(g_out, 0, sizeof(g_out));
    osui_gui_web_command("web-fkey del", g_out, (int)sizeof(g_out));
    TEST_ASSERT_NOT_NULL(strstr(g_out, "osui web autofill removed name=q row 2 left 2 pick 2/2"));
    TEST_ASSERT_EQUAL(2, g_dqn);
    TEST_ASSERT_EQUAL(0, osui_gui_offer_state(st, (int)sizeof(st)));
    TEST_ASSERT_EQUAL_STRING("2 sel 1 mohhdy os|qemu tablet", st);
    TEST_ASSERT_EQUAL(5, osui_gui_offer_items(it, 8));          /* box, border, highlight, 2 rows */
    memset(g_out, 0, sizeof(g_out));
    osui_gui_web_command("web-fstate", g_out, (int)sizeof(g_out));
    TEST_ASSERT_NOT_NULL(strstr(g_out, "value=qemu tablet"));
    /* the keyboard Delete key (desktop path) on the last row: the
     * highlight moves up */
    memset(g_out, 0, sizeof(g_out));
    osui_gui_feed_key(OS_TERM_KEY_DEL, g_out, (int)sizeof(g_out));
    TEST_ASSERT_NOT_NULL(strstr(g_out, "osui web autofill removed name=q row 2 left 1 pick 1/1"));
    memset(g_out, 0, sizeof(g_out));
    osui_gui_web_command("web-fstate", g_out, (int)sizeof(g_out));
    TEST_ASSERT_NOT_NULL(strstr(g_out, "value=mohhdy os"));
    /* the last value: the list closes and the field is empty again */
    memset(g_out, 0, sizeof(g_out));
    osui_gui_web_command("web-fkey del", g_out, (int)sizeof(g_out));
    TEST_ASSERT_NOT_NULL(strstr(g_out, "osui web autofill removed name=q row 1 left 0 closed"));
    TEST_ASSERT_EQUAL(0, g_dqn);
    TEST_ASSERT_EQUAL(0, osui_gui_offer_items(it, 8));
    TEST_ASSERT_EQUAL(-1, osui_gui_offer_state(st, (int)sizeof(st)));
    memset(g_out, 0, sizeof(g_out));
    osui_gui_web_command("web-fstate", g_out, (int)sizeof(g_out));
    TEST_ASSERT_NULL(strstr(g_out, "value=mohhdy"));
    TEST_ASSERT_EQUAL(3, g_drops);
    /* a new click offers nothing: the store is empty */
    memset(g_out, 0, sizeof(g_out));
    osui_gui_web_command(ev, g_out, (int)sizeof(g_out));
    TEST_ASSERT_NULL(strstr(g_out, "autofill offer"));
    osui_gui_web_command("web-fkey esc", g_out, (int)sizeof(g_out));
    osui_gui_feed_key(OS_VGA_KEY_ESC, g_out, (int)sizeof(g_out));
    osui_gui_set_web_page_ops(0);
    osui_gui_set_web_fetch(0);
}

static void test_lot87_offer_click(void) {
    static const osui_web_page_ops_t ops = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, f_request,
                                             0, 0, 0, 0, l87_offer, l87_offer_at, 0 };
    os_fb_web_item_t it[8];
    char ev[64];
    int x, y, n, lx, ly;
    setup();
    osui_gui_set_web_fetch(f_fetch);
    osui_gui_set_web_page_ops(&ops);
    TEST_ASSERT_EQUAL(1, osui_gui_web_command("web-open https://ex.am/dir/page", g_out, (int)sizeof(g_out)));
    TEST_ASSERT(f_box("q", &x, &y));
    snprintf(ev, sizeof(ev), "web-fclick %d %d", x, y);
    osui_gui_web_command(ev, g_out, (int)sizeof(g_out));
    n = osui_gui_offer_items(it, 8);
    TEST_ASSERT_EQUAL(6, n);
    lx = it[0].x + 10;
    ly = it[0].y;
    /* the top padding (above the first row): swallowed, the list stays */
    snprintf(ev, sizeof(ev), "web-fclick %d %d", lx, ly);
    memset(g_out, 0, sizeof(g_out));
    osui_gui_web_command(ev, g_out, (int)sizeof(g_out));
    TEST_ASSERT_NULL(strstr(g_out, "applied"));
    TEST_ASSERT_NULL(strstr(g_out, "form2"));
    TEST_ASSERT_EQUAL(6, osui_gui_offer_items(it, 8));
    /* the third row through the desktop event path */
    snprintf(ev, sizeof(ev), "web click %d %d", lx, ly + 2 + 2 * 12 + 5);
    memset(g_out, 0, sizeof(g_out));
    osui_gui_event(ev, g_out, (int)sizeof(g_out));
    TEST_ASSERT_NOT_NULL(strstr(g_out, "osui web autofill applied name=q pick 3/3 click"));
    TEST_ASSERT_EQUAL(0, osui_gui_offer_items(it, 8));
    memset(g_out, 0, sizeof(g_out));
    osui_gui_web_command("web-fstate", g_out, (int)sizeof(g_out));
    TEST_ASSERT_NOT_NULL(strstr(g_out, "value=qemu tablet"));
    /* the same place again, the list closed: the page gets the click */
    memset(g_out, 0, sizeof(g_out));
    osui_gui_event(ev, g_out, (int)sizeof(g_out));
    TEST_ASSERT_NULL(strstr(g_out, "autofill applied"));
    /* reopen (Tab away and back), the first row by the console command */
    osui_gui_web_command("web-fkey tab", g_out, (int)sizeof(g_out));
    osui_gui_web_command("web-fkey stab", g_out, (int)sizeof(g_out));
    n = osui_gui_offer_items(it, 8);
    TEST_ASSERT(n > 0);
    snprintf(ev, sizeof(ev), "web-fclick %d %d", it[0].x + 10, it[0].y + 2 + 5);
    memset(g_out, 0, sizeof(g_out));
    osui_gui_web_command(ev, g_out, (int)sizeof(g_out));
    TEST_ASSERT_NOT_NULL(strstr(g_out, "osui web autofill applied name=q pick 1/2 click"));
    memset(g_out, 0, sizeof(g_out));
    osui_gui_web_command("web-fstate", g_out, (int)sizeof(g_out));
    TEST_ASSERT_NOT_NULL(strstr(g_out, "value=mohhdy os"));
    osui_gui_web_command("web-fkey esc", g_out, (int)sizeof(g_out));
    osui_gui_feed_key(OS_VGA_KEY_ESC, g_out, (int)sizeof(g_out));
    osui_gui_set_web_page_ops(0);
    osui_gui_set_web_fetch(0);
}

static void test_lot87_form2_autofill(void) {
    static const osui_web_page_ops_t ops = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, f_request,
                                             0, 0, 0, 0, l87_offer, 0, 0 };
    static const osui_web_page_ops_t no_offer = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, f_request, 0, 0, 0, 0, 0, 0, 0 };
    char ev[64];
    int x, y;
    setup();
    g_f_fetches = g_f_posts = 0;
    g_l87_calls = 0;
    osui_gui_set_web_fetch(f_fetch);
    osui_gui_set_web_page_ops(&ops);
    TEST_ASSERT_EQUAL(1, osui_gui_web_command("web-open https://ex.am/dir/page", g_out, (int)sizeof(g_out)));
    TEST_ASSERT(f_box("q", &x, &y));
    snprintf(ev, sizeof(ev), "web click %d %d", x, y);
    memset(g_out, 0, sizeof(g_out));
    osui_gui_event(ev, g_out, (int)sizeof(g_out));
    TEST_ASSERT_NOT_NULL(strstr(g_out, "osui web form2 changed focus 0"));
    TEST_ASSERT_NOT_NULL(strstr(g_out, "osui web autofill offer name=q key=Down"));
    TEST_ASSERT_EQUAL_STRING("https://ex.am/dir/page", g_l87_url);
    TEST_ASSERT_EQUAL_STRING("q", g_l87_name);
    /* the value is not printed with the offer */
    TEST_ASSERT_NULL(strstr(g_out, "mohhdy os"));
    /* a second click on the focused field asks nothing again */
    g_l87_calls = 0;
    osui_gui_event(ev, g_out, (int)sizeof(g_out));
    TEST_ASSERT_EQUAL(0, g_l87_calls);
    /* Down takes it; the field holds the remembered text */
    memset(g_out, 0, sizeof(g_out));
    osui_gui_feed_key(OS_VGA_KEY_DOWN, g_out, (int)sizeof(g_out));
    TEST_ASSERT_NOT_NULL(strstr(g_out, "osui web autofill applied name=q"));
    memset(g_out, 0, sizeof(g_out));
    osui_gui_web_command("web-fstate", g_out, (int)sizeof(g_out));
    TEST_ASSERT_NOT_NULL(strstr(g_out, "value=mohhdy os"));
    /* a second Down is an ordinary key again */
    memset(g_out, 0, sizeof(g_out));
    osui_gui_feed_key(OS_VGA_KEY_DOWN, g_out, (int)sizeof(g_out));
    TEST_ASSERT_NULL(strstr(g_out, "autofill applied"));
    /* Enter sends the filled value */
    memset(g_out, 0, sizeof(g_out));
    osui_gui_web_command("web-fkey enter", g_out, (int)sizeof(g_out));
    TEST_ASSERT_NOT_NULL(strstr(g_out, "osui web form2 submit GET https://ex.am/find?q=mohhdy+os"));
    TEST_ASSERT_EQUAL_STRING("https://ex.am/find?q=mohhdy+os", g_f_url);
    /* a field that already holds the remembered value: no offer */
    TEST_ASSERT_EQUAL(1, osui_gui_web_command("web-open https://ex.am/dir/page", g_out, (int)sizeof(g_out)));
    TEST_ASSERT(f_box("p", &x, &y));
    snprintf(ev, sizeof(ev), "web-fclick %d %d", x, y);
    memset(g_out, 0, sizeof(g_out));
    osui_gui_web_command(ev, g_out, (int)sizeof(g_out));
    TEST_ASSERT_NOT_NULL(strstr(g_out, "changed focus"));
    TEST_ASSERT_EQUAL_STRING("p", g_l87_name);
    TEST_ASSERT_NULL(strstr(g_out, "autofill offer"));
    /* the console Down key on q (web-fkey) takes the offer like the desktop */
    TEST_ASSERT(f_box("q", &x, &y));
    snprintf(ev, sizeof(ev), "web-fclick %d %d", x, y);
    memset(g_out, 0, sizeof(g_out));
    osui_gui_web_command(ev, g_out, (int)sizeof(g_out));
    TEST_ASSERT_NOT_NULL(strstr(g_out, "autofill offer name=q"));
    /* moving the focus away drops the offer: Down on the checkbox is not it */
    osui_gui_web_command("web-fkey tab", g_out, (int)sizeof(g_out));
    memset(g_out, 0, sizeof(g_out));
    osui_gui_web_command("web-fkey down", g_out, (int)sizeof(g_out));
    TEST_ASSERT_NULL(strstr(g_out, "autofill applied"));
    /* the keyboard focus back on q offers it again, like a click */
    memset(g_out, 0, sizeof(g_out));
    osui_gui_web_command("web-fkey stab", g_out, (int)sizeof(g_out));
    TEST_ASSERT_NOT_NULL(strstr(g_out, "autofill offer name=q"));
    memset(g_out, 0, sizeof(g_out));
    osui_gui_web_command("web-fkey down", g_out, (int)sizeof(g_out));
    TEST_ASSERT_NOT_NULL(strstr(g_out, "autofill applied name=q"));
    /* without the op: no offer, nothing asked */
    osui_gui_set_web_page_ops(&no_offer);
    TEST_ASSERT_EQUAL(1, osui_gui_web_command("web-open https://ex.am/dir/page", g_out, (int)sizeof(g_out)));
    TEST_ASSERT(f_box("q", &x, &y));
    snprintf(ev, sizeof(ev), "web-fclick %d %d", x, y);
    g_l87_calls = 0;
    memset(g_out, 0, sizeof(g_out));
    osui_gui_web_command(ev, g_out, (int)sizeof(g_out));
    TEST_ASSERT_EQUAL(0, g_l87_calls);
    TEST_ASSERT_NULL(strstr(g_out, "autofill"));
    osui_gui_web_command("web-fkey esc", g_out, (int)sizeof(g_out));
    osui_gui_feed_key(OS_VGA_KEY_ESC, g_out, (int)sizeof(g_out));
    osui_gui_set_web_page_ops(0);
    osui_gui_set_web_fetch(0);
}

/* Lot 74: fragments: :target and the scroll to the element */
static int t_pages_load(const char *file, char *buf, int max) {
    const char *r = 0;
    if (!strcmp(file, "/web/t.html"))
        r = "<style>:target{color:#ff0000}div:target-within{color:#00ff00}</style><p><a href=\"#s3\">Go3</a> <a href=\"#zz\">Gone</a></p>"
            "<div style=\"height:600px\">pad</div><div id=w><h2 id=s3>Three</h2></div><div style=\"height:900px\">x</div>";
    if (!r || (int)strlen(r) > max) return -1;
    memcpy(buf, r, strlen(r));
    return (int)strlen(r);
}
static void test_lot74_gui_fragment(void) {
    char e[48], d[64];
    const we_doc_t *doc;
    int x, y, t;
    setup();
    osui_gui_set_ide_ops(t_pages_load, fake_save, fake_exec);
    feed_str("web /web/t.html");
    osui_gui_feed_key('\n', g_out, (int)sizeof(g_out));
    TEST_ASSERT_NOT_NULL(strstr(g_out, "osui web render file=/web/t.html"));
    doc = osui_gui_web_doc();
    t = we_find_id(doc, "s3");
    TEST_ASSERT(t > 0);
    TEST_ASSERT_EQUAL(0x000000, (int)(doc->nodes[t].color & 0xffffff));
    TEST_ASSERT(web_item("Go3", &x, &y));
    snprintf(e, sizeof(e), "web click %d %d", x, y);
    evt(e);
    TEST_ASSERT_NOT_NULL(strstr(g_out, "osui web link href=#s3"));
    TEST_ASSERT_NOT_NULL(strstr(g_out, "osui web target id=s3 scroll "));
    { const char *q = strstr(g_out, "scroll ") + 7; int v = 0; while (*q >= '0' && *q <= '9') v = v * 10 + (*q++ - '0'); TEST_ASSERT(v >= 600); }
    doc = osui_gui_web_doc();
    TEST_ASSERT_EQUAL(0xff0000, (int)(doc->nodes[we_find_id(doc, "s3")].color & 0xffffff));
    TEST_ASSERT_EQUAL(0x00ff00, (int)(doc->nodes[we_find_id(doc, "w")].color & 0xffffff));
    TEST_ASSERT_EQUAL(we_find_id(doc, "s3"), we_target_node(doc));
    /* back: the page without the fragment, no target */
    evt("web back");
    TEST_ASSERT_NOT_NULL(strstr(g_out, "osui web back file=/web/t.html"));
    doc = osui_gui_web_doc();
    TEST_ASSERT_EQUAL(-1, we_target_node(doc));
    TEST_ASSERT_EQUAL(0x000000, (int)(doc->nodes[we_find_id(doc, "s3")].color & 0xffffff));
    evt("web fwd");
    TEST_ASSERT_NOT_NULL(strstr(g_out, "osui web target id=s3"));
    /* an address with a fragment; an unknown id keeps the page at the top */
    memset(g_out, 0, sizeof(g_out));
    osui_gui_web_command("web-open /web/t.html#zz", g_out, (int)sizeof(g_out));
    TEST_ASSERT_NOT_NULL(strstr(g_out, "osui web target none id=zz"));
    TEST_ASSERT_EQUAL(-1, we_target_node(osui_gui_web_doc()));
    memset(g_out, 0, sizeof(g_out));
    osui_gui_web_command("web-open /web/t.html#%73%33", g_out, (int)sizeof(g_out));
    TEST_ASSERT_NOT_NULL(strstr(g_out, "osui web target id=s3"));
    memset(g_out, 0, sizeof(g_out));
    osui_gui_web_command("web-open /web/t.html#abcdefghijklmnopqrstuvwxyz0123456789", g_out, (int)sizeof(g_out));
    TEST_ASSERT_NOT_NULL(strstr(g_out, "osui web target error=too_long"));
    /* local links keep their fragment */
    TEST_ASSERT(osui_gui_web_resolve("/web/a.html", "u.html#k", d, (int)sizeof(d)) == 0);
    TEST_ASSERT_EQUAL_STRING("/web/u.html#k", d);
    TEST_ASSERT(osui_gui_web_resolve("/web/a.html", "u.html?q#k", d, (int)sizeof(d)) < 0);
    TEST_ASSERT(osui_gui_web_resolve("/web/a.html", "u.html#k", d, 13) < 0);
    TEST_ASSERT(osui_gui_web_resolve("/web/a.html", "#top", d, (int)sizeof(d)) < 0);
    osui_gui_set_ide_ops(0, 0, 0);
    feed_str("/center");
    osui_gui_feed_key('\n', g_out, (int)sizeof(g_out));
}


/* Lot 90: a value picked from the list and kept is accepted: the store is
 * told (page_offer_touch) so it dates that value today. Down with one
 * value, a click on a row, and Down / Up then leaving the field (Tab,
 * Esc) count; a pick the operator typed over does not. */
static int g_l90_touch;
static char g_l90_val[32], g_l90_url[128];
static int l90_touch(const char *url, const char *name, const char *value) {
    g_l90_touch++;
    snprintf(g_l90_val, sizeof(g_l90_val), "%s", value);
    snprintf(g_l90_url, sizeof(g_l90_url), "%s", url);
    return strcmp(name, "q") == 0 || strcmp(name, "p") == 0 ? 0 : -1;
}
static void test_lot90_offer_touch(void) {
    static const osui_web_page_ops_t ops = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, f_request,
                                             0, 0, 0, 0, l87_offer, l87_offer_at, 0, l90_touch };
    os_fb_web_item_t it[8];
    char ev[64];
    int x, y, n;
    unsigned base;
    setup();
    osui_gui_set_web_fetch(f_fetch);
    osui_gui_set_web_page_ops(&ops);
    g_l90_touch = 0;
    base = osui_gui_offer_touches();
    TEST_ASSERT_EQUAL(1, osui_gui_web_command("web-open https://ex.am/dir/page", g_out, (int)sizeof(g_out)));
    TEST_ASSERT(f_box("q", &x, &y));
    snprintf(ev, sizeof(ev), "web-fclick %d %d", x, y);
    osui_gui_web_command(ev, g_out, (int)sizeof(g_out));
    /* Down twice: previews, nothing accepted while the list is open */
    osui_gui_web_command("web-fkey down", g_out, (int)sizeof(g_out));
    osui_gui_web_command("web-fkey down", g_out, (int)sizeof(g_out));
    TEST_ASSERT_EQUAL(0, g_l90_touch);
    /* Tab away with "kernel" in the field: accepted */
    osui_gui_web_command("web-fkey tab", g_out, (int)sizeof(g_out));
    TEST_ASSERT_EQUAL(1, g_l90_touch);
    TEST_ASSERT_EQUAL_STRING("kernel", g_l90_val);
    TEST_ASSERT_EQUAL_STRING("https://ex.am/dir/page", g_l90_url);
    TEST_ASSERT_EQUAL(base + 1U, osui_gui_offer_touches());
    /* back on q (the list lists the two others), click the second row */
    osui_gui_web_command("web-fkey stab", g_out, (int)sizeof(g_out));
    n = osui_gui_offer_items(it, 8);
    TEST_ASSERT(n > 0);
    snprintf(ev, sizeof(ev), "web-fclick %d %d", it[0].x + 10, it[0].y + 2 + 12 + 5);
    osui_gui_web_command(ev, g_out, (int)sizeof(g_out));
    TEST_ASSERT_EQUAL(2, g_l90_touch);
    TEST_ASSERT_EQUAL_STRING("qemu tablet", g_l90_val);
    /* Down then Esc: accepted (the field keeps the value) */
    osui_gui_web_command("web-fkey tab", g_out, (int)sizeof(g_out));
    osui_gui_web_command("web-fkey stab", g_out, (int)sizeof(g_out));
    osui_gui_web_command("web-fkey down", g_out, (int)sizeof(g_out));
    osui_gui_web_command("web-fkey esc", g_out, (int)sizeof(g_out));
    TEST_ASSERT_EQUAL(3, g_l90_touch);
    TEST_ASSERT_EQUAL_STRING("mohhdy os", g_l90_val);
    /* Down, then the operator types over the pick, then Tab: not accepted */
    osui_gui_web_command("web-fkey tab", g_out, (int)sizeof(g_out));
    osui_gui_web_command("web-fkey stab", g_out, (int)sizeof(g_out));
    TEST_ASSERT(osui_gui_offer_items(it, 8) > 0);
    osui_gui_web_command("web-fkey down", g_out, (int)sizeof(g_out));
    osui_gui_web_command("web-ftype x", g_out, (int)sizeof(g_out));
    osui_gui_web_command("web-fkey tab", g_out, (int)sizeof(g_out));
    TEST_ASSERT_EQUAL(3, g_l90_touch);
    /* p: one value, Down fills it and closes the list: accepted at once */
    TEST_ASSERT(f_box("p", &x, &y));
    snprintf(ev, sizeof(ev), "web-fclick %d %d", x, y);
    osui_gui_web_command(ev, g_out, (int)sizeof(g_out));
    osui_gui_web_command("web-fkey down", g_out, (int)sizeof(g_out));
    TEST_ASSERT_EQUAL(4, g_l90_touch);
    TEST_ASSERT_EQUAL_STRING("old p", g_l90_val);
    TEST_ASSERT_EQUAL(base + 4U, osui_gui_offer_touches());
    osui_gui_web_command("web-fkey esc", g_out, (int)sizeof(g_out));
    osui_gui_feed_key(OS_VGA_KEY_ESC, g_out, (int)sizeof(g_out));
    osui_gui_set_web_page_ops(0);
    osui_gui_set_web_fetch(0);
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
    RUN_TEST(test_lot69_desktop_cmd_event_keeps_typed_line);
    RUN_TEST(test_lot69_browser_pane_address_line);
    RUN_TEST(test_lot69_full_window_close_event);
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
    RUN_TEST(test_gui_web_glyph_pages);
    RUN_TEST(test_gui_web_pipeline_sanitized);
    RUN_TEST(test_gui_mobile_pane);
    RUN_TEST(test_gui_live_panes);
    RUN_TEST(test_gui_apps_explorer_editor);
    RUN_TEST(test_gui_tool_apps_slots);
    RUN_TEST(test_gui_viewer_pixels_and_editor_size);
    RUN_TEST(test_gui_parity_panels);
    RUN_TEST(test_gui_web_links_history);
    RUN_TEST(test_lot71_web_sections_and_form);
    RUN_TEST(test_lot73_anim_tick_resends_atlas);
    RUN_TEST(test_lot74_form_url);
    RUN_TEST(test_lot74_gui_forms);
    RUN_TEST(test_lot75_gui_multipart_ctype);
    RUN_TEST(test_lot86_shell_page_ops_routing);
    RUN_TEST(test_lot86_focus_offers_and_down);
    RUN_TEST(test_lot87_form2_autofill);
    RUN_TEST(test_lot87_offer_list);
    RUN_TEST(test_lot90_offer_touch);
    RUN_TEST(test_lot87_offer_click);
    RUN_TEST(test_lot87_offer_delete);
    RUN_TEST(test_lot74_gui_fragment);
    unity_print_results();
    unity_cleanup();
    return (unity_stats.tests_failed == 0) ? 0 : 1;
}
