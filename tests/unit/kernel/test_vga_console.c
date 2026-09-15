#include "../../framework/unity.h"
#include "../../../kernel/vga_console.h"

static char glyph_at(int x, int y) {
    return (char)(vga_console_visible_cell(x, y) & 0xFF);
}

static void test_init_blanks_screen(void) {
    vga_console_init(0x07);
    TEST_ASSERT_EQUAL(' ', glyph_at(0, 0));
    TEST_ASSERT_EQUAL(' ', glyph_at(79, 24));
    TEST_ASSERT_EQUAL(0, vga_console_hist_count());
    TEST_ASSERT_EQUAL(0, vga_console_view_offset());
}

static void test_put_xy_writes_cell(void) {
    vga_console_init(0x07);
    vga_console_put_xy('A', 3, 4, 0x0F);
    TEST_ASSERT_EQUAL('A', glyph_at(3, 4));
#ifdef KERNEL_TEST
    TEST_ASSERT_EQUAL('A', (char)(vga_test_fb[4 * VGA_COLS + 3] & 0xFF));
#endif
}

static void test_scroll_saves_history(void) {
    int i;

    vga_console_init(0x07);
    vga_console_put_xy('T', 0, 0, 0x0F);
    for (i = 0; i < VGA_ROWS; i++) {
        vga_console_scroll();
    }
    TEST_ASSERT_EQUAL(VGA_ROWS, vga_console_hist_count());
    TEST_ASSERT_EQUAL(' ', glyph_at(0, 0));
}

static void test_page_up_shows_saved_line(void) {
    vga_console_init(0x07);
    vga_console_put_xy('Z', 0, 0, 0x0F);
    vga_console_scroll();
    TEST_ASSERT_EQUAL(1, vga_console_view_up(1));
    TEST_ASSERT_EQUAL('Z', glyph_at(0, 0));
    TEST_ASSERT_EQUAL(1, vga_console_view_offset());
}

static void test_page_down_returns_to_live(void) {
    vga_console_init(0x07);
    vga_console_put_xy('Z', 0, 0, 0x0F);
    vga_console_scroll();
    vga_console_view_up(1);
    TEST_ASSERT_EQUAL(0, vga_console_view_down(1));
    TEST_ASSERT_EQUAL(' ', glyph_at(0, 0));
}

static void test_put_while_scrolled_returns_live(void) {
    vga_console_init(0x07);
    vga_console_put_xy('Z', 0, 0, 0x0F);
    vga_console_scroll();
    vga_console_view_up(1);
    vga_console_put_xy('Q', 2, 3, 0x0F);
    TEST_ASSERT_EQUAL(0, vga_console_view_offset());
    TEST_ASSERT_EQUAL('Q', glyph_at(2, 3));
}

static void test_view_up_clamped_to_history(void) {
    vga_console_init(0x07);
    vga_console_put_xy('A', 0, 0, 0x0F);
    vga_console_scroll();
    TEST_ASSERT_EQUAL(1, vga_console_view_up(40));
}

static void test_cursor_inverts_visible_cell(void) {
    unsigned char attr;

    vga_console_init(0x07);
    vga_console_set_cursor(1, 2);
#ifdef KERNEL_TEST
    attr = (unsigned char)(vga_test_fb[2 * VGA_COLS + 1] >> 8);
    TEST_ASSERT_NOT_EQUAL(0x07, attr);
    TEST_ASSERT_EQUAL(' ', (char)(vga_test_fb[2 * VGA_COLS + 1] & 0xFF));
    TEST_ASSERT_EQUAL(0x07, (int)(vga_test_fb[0] >> 8));
#else
    (void)attr;
#endif
}

int main(void) {
    unity_init();
    RUN_TEST(test_init_blanks_screen);
    RUN_TEST(test_put_xy_writes_cell);
    RUN_TEST(test_scroll_saves_history);
    RUN_TEST(test_page_up_shows_saved_line);
    RUN_TEST(test_page_down_returns_to_live);
    RUN_TEST(test_put_while_scrolled_returns_live);
    RUN_TEST(test_view_up_clamped_to_history);
    RUN_TEST(test_cursor_inverts_visible_cell);
    unity_print_results();
    unity_cleanup();
    return (unity_stats.tests_failed == 0) ? 0 : 1;
}
