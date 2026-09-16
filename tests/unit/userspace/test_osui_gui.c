/* test_osui_gui.c - Bureau VGA 80x25 (chat central/flottant, scene). */

#include "../../framework/unity.h"
#include "../../framework/test_kernel.h"
#include "osui_runtime.h"
#include "osui_gui.h"
#include "os_syscalls.h"
#include <string.h>

static uint16_t g_cells[OS_VGA_ROWS * OS_VGA_COLS];
static char g_out[OSUI_OUT_MAX];
static char g_row[96];

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
    TEST_ASSERT(strstr(g_out, "chrome=vga_desktop") != NULL);
    TEST_ASSERT(strstr(g_out, "us031_complete=false") != NULL);
    TEST_ASSERT(osui_gui_should_enter());
    osui_gui_ack_enter();
    TEST_ASSERT_EQUAL(0, run_line("graphics"));
    TEST_ASSERT(strstr(g_out, "canonical=gui") != NULL);
    TEST_ASSERT_EQUAL(0, run_line("desktop"));
    TEST_ASSERT(strstr(g_out, "chrome=vga_desktop") != NULL);
}

static void test_center_chat_layout(void) {
    setup();
    osui_gui_render(g_cells);
    osui_gui_ascii_row(g_cells, 0, g_row, (int)sizeof(g_row));
    TEST_ASSERT(strstr(g_row, "MOHHDY OS") != NULL);
    TEST_ASSERT(strstr(g_row, "us031=false") != NULL);
    osui_gui_ascii_row(g_cells, 5, g_row, (int)sizeof(g_row));
    TEST_ASSERT(strstr(g_row, "CHAT central") != NULL);
}

static void test_float_chat_when_program_opens(void) {
    setup();
    TEST_ASSERT_EQUAL(0, run_line("/browser"));
    TEST_ASSERT(strstr(g_out, "chat_mode=float") != NULL || strstr(g_out, "pane=browser") != NULL);
    TEST_ASSERT_EQUAL_STRING("float", osui_get_chat_mode());
    TEST_ASSERT_EQUAL_STRING("browser", osui_get_pane());
    osui_gui_render(g_cells);
    osui_gui_ascii_row(g_cells, osui_get_chat_y(), g_row, (int)sizeof(g_row));
    TEST_ASSERT(strstr(g_row, "CHAT float") != NULL);
    osui_gui_ascii_row(g_cells, 2, g_row, (int)sizeof(g_row));
    TEST_ASSERT(strstr(g_row, "browser") != NULL);

    TEST_ASSERT_EQUAL(0, run_line("/center"));
    TEST_ASSERT(strstr(g_out, "chat_mode=center") != NULL);
    TEST_ASSERT_EQUAL_STRING("center", osui_get_chat_mode());
    osui_gui_render(g_cells);
    osui_gui_ascii_row(g_cells, 5, g_row, (int)sizeof(g_row));
    TEST_ASSERT(strstr(g_row, "CHAT central") != NULL);
}

static void test_stage_constructions_not_tiny_dump(void) {
    setup();
    run_line("stage-prompt dessine un cercle");
    TEST_ASSERT(strstr(g_out, "kind=circle") != NULL);
    TEST_ASSERT(strstr(g_out, "canvas=vga_desktop") != NULL);
    TEST_ASSERT_EQUAL_STRING("circle", osui_get_stage_kind());
    osui_gui_render(g_cells);
    {
        int y;
        int stars = 0;
        for (y = 1; y < 20; y++) {
            int x;
            osui_gui_ascii_row(g_cells, y, g_row, (int)sizeof(g_row));
            for (x = 0; g_row[x]; x++) {
                if (g_row[x] == '*') stars++;
            }
        }
        TEST_ASSERT(stars >= 8);
    }

    run_line("stage-prompt dessine trois boites");
    TEST_ASSERT(strstr(g_out, "[A] [B] [C]") != NULL);
    TEST_ASSERT(strstr(g_out, "kind=boxes") != NULL);
    osui_gui_render(g_cells);
    osui_gui_ascii_row(g_cells, 4, g_row, (int)sizeof(g_row));
    TEST_ASSERT(strchr(g_row, '+') != NULL);
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
    RUN_TEST(test_center_chat_layout);
    RUN_TEST(test_float_chat_when_program_opens);
    RUN_TEST(test_stage_constructions_not_tiny_dump);
    RUN_TEST(test_gui_feed_slash_and_esc);
    unity_print_results();
    unity_cleanup();
    return (unity_stats.tests_failed == 0) ? 0 : 1;
}
