/* test_osui_gui.c - Commande gui, instantane HTML host, chat float. */

#include "../../framework/unity.h"
#include "../../framework/test_kernel.h"
#include "osui_runtime.h"
#include "osui_gui.h"
#include "os_syscalls.h"
#include <string.h>

static uint16_t g_cells[OS_VGA_ROWS * OS_VGA_COLS];
static char g_out[OSUI_OUT_MAX];
static char g_row[96];
static char g_snap[OSUI_SNAP_MAX];

static int run_line(const char *line) {
    memset(g_out, 0, sizeof(g_out));
    return osui_dispatch_line(line, g_out, (int)sizeof(g_out));
}

static void setup(void) {
    osui_runtime_init();
    osui_gui_set_input("");
}

static void test_gui_command_canonical(void) {
    setup();
    TEST_ASSERT_EQUAL(0, run_line("gui"));
    TEST_ASSERT(strstr(g_out, "canonical=gui") != NULL);
    TEST_ASSERT(strstr(g_out, "chrome=html_host") != NULL);
    TEST_ASSERT(strstr(g_out, "display_surface=html_host") != NULL);
    TEST_ASSERT(strstr(g_out, "us031_complete=false") != NULL);
    TEST_ASSERT(osui_gui_should_enter());
    osui_gui_ack_enter();
    TEST_ASSERT_EQUAL(0, run_line("graphics"));
    TEST_ASSERT(strstr(g_out, "canonical=gui") != NULL);
    TEST_ASSERT_EQUAL(0, run_line("desktop"));
    TEST_ASSERT(strstr(g_out, "chrome=html_host") != NULL);
}

static void test_vga_pointer_not_ascii_desktop(void) {
    setup();
    osui_gui_render(g_cells);
    osui_gui_ascii_row(g_cells, 0, g_row, (int)sizeof(g_row));
    TEST_ASSERT(strstr(g_row, "MOHHDY OS") != NULL);
    TEST_ASSERT(strstr(g_row, "html_host") != NULL);
    TEST_ASSERT(strstr(g_row, "us031=false") != NULL);
    osui_gui_ascii_row(g_cells, 5, g_row, (int)sizeof(g_row));
    TEST_ASSERT(strstr(g_row, "CHAT central") == NULL);
    osui_gui_ascii_row(g_cells, 3, g_row, (int)sizeof(g_row));
    TEST_ASSERT(strstr(g_row, "ASCII") != NULL);
}

static void test_snap_center_and_float(void) {
    setup();
    osui_gui_write_snap(g_snap, (int)sizeof(g_snap));
    TEST_ASSERT(strstr(g_snap, "OSUI-SNAP") != NULL);
    TEST_ASSERT(strstr(g_snap, "chrome=html_host") != NULL);
    TEST_ASSERT(strstr(g_snap, "chat_mode=center") != NULL);
    TEST_ASSERT(strstr(g_snap, "OSUI-END") != NULL);

    TEST_ASSERT_EQUAL(0, run_line("/browser"));
    TEST_ASSERT_EQUAL_STRING("float", osui_get_chat_mode());
    TEST_ASSERT_EQUAL_STRING("browser", osui_get_pane());
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

int main(void) {
    unity_init();
    RUN_TEST(test_gui_command_canonical);
    RUN_TEST(test_vga_pointer_not_ascii_desktop);
    RUN_TEST(test_snap_center_and_float);
    RUN_TEST(test_snap_stage_kind);
    RUN_TEST(test_gui_feed_slash_and_esc);
    unity_print_results();
    unity_cleanup();
    return (unity_stats.tests_failed == 0) ? 0 : 1;
}
