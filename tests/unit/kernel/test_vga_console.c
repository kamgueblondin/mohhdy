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

static void test_desktop_blit_and_restore(void) {
    uint16_t frame[VGA_ROWS * VGA_COLS];
    int i;

    vga_console_init(0x07);
    for (i = 0; i < VGA_ROWS * VGA_COLS; i++) {
        frame[i] = (uint16_t)'#' | ((uint16_t)0x1F << 8);
    }
    frame[0] = (uint16_t)'G' | ((uint16_t)0x1F << 8);
    vga_desktop_blit(frame);
    TEST_ASSERT_EQUAL(1, vga_desktop_active());
    TEST_ASSERT_EQUAL('G', glyph_at(0, 0));
    TEST_ASSERT_EQUAL('#', glyph_at(1, 0));
#ifdef KERNEL_TEST
    TEST_ASSERT_EQUAL('G', (char)(vga_test_fb[0] & 0xFF));
#endif
    vga_console_put_xy('X', 2, 2, 0x0F);
    TEST_ASSERT_EQUAL('#', glyph_at(2, 2));
    vga_desktop_set(0);
    TEST_ASSERT_EQUAL(0, vga_desktop_active());
    TEST_ASSERT_EQUAL(' ', glyph_at(0, 0));
}

/* Lot 7 (US-001): with a Ring 3 owner the frame is rendered but not
 * painted by Ring 0; the driver copy and a Ring 0 repaint see the same. */
static void test_frame_routed_to_ring3_driver(void) {
    uint16_t cells[VGA_ROWS * VGA_COLS];
    uint32_t cx, cy, vis;
    int before;
    vga_test_route_ring3 = 0;
    vga_console_init(0x07);
    before = vga_test_ring0_paints;
    vga_console_put_xy('Q', 0, 0, 0x07);
    TEST_ASSERT_EQUAL(before + 1, vga_test_ring0_paints);
    vga_test_route_ring3 = 1;
    before = vga_test_ring0_paints;
    vga_console_put_xy('Z', 5, 1, 0x1E);
    vga_console_set_cursor(6, 1);
    TEST_ASSERT_EQUAL(before, vga_test_ring0_paints);          /* Ring 0 did not paint */
    TEST_ASSERT_EQUAL(' ', (char)(vga_test_fb[1 * VGA_COLS + 5] & 0xFF));
    vga_console_frame_copy(cells, &cx, &cy, &vis);
    TEST_ASSERT_EQUAL('Z', (char)(cells[1 * VGA_COLS + 5] & 0xFF));
    TEST_ASSERT_EQUAL(0x1E, (int)(cells[1 * VGA_COLS + 5] >> 8));
    TEST_ASSERT_EQUAL(6U, cx);
    TEST_ASSERT_EQUAL(1U, cy);
    TEST_ASSERT_EQUAL(1U, vis);
    vga_console_paint_ring0();                                 /* stall or death */
    TEST_ASSERT_EQUAL('Z', (char)(vga_test_fb[1 * VGA_COLS + 5] & 0xFF));
    vga_test_route_ring3 = 0;
}

/* Leaving the VBE desktop reuses the text RAM: the epoch tells the Ring 3
 * driver to repaint every cell instead of trusting its diff cache. */
static void test_vram_lost_bumps_epoch(void) {
    uint32_t e0 = vga_console_vram_epoch();
    vga_console_vram_lost();
    TEST_ASSERT_EQUAL(e0 + 1U, vga_console_vram_epoch());
    vga_console_vram_lost();
    TEST_ASSERT_EQUAL(e0 + 2U, vga_console_vram_epoch());
}

static uint16_t g_sink_cells[VGA_ROWS * VGA_COLS];
static int g_sink_calls;
static void test_sink(const uint16_t *cells, int cx, int cy, int vis) {
    int i;
    (void)cx; (void)cy; (void)vis;
    for (i = 0; i < VGA_ROWS * VGA_COLS; i++) g_sink_cells[i] = cells[i];
    g_sink_calls++;
}

static void feed(const char *s, int *x, int *y) {
    for (; *s; s++) {
        if (*s == '\n' || *x >= VGA_COLS) { *x = 0; (*y)++; if (*s == '\n') continue; }
        if (*y >= VGA_ROWS) { vga_console_scroll(); *y = VGA_ROWS - 1; }
        vga_console_put_xy(*s, *x, *y, (char)((*x % 7) + 1));
        (*x)++;
        vga_console_set_cursor(*x, *y);
    }
}

/* One console core, two backends: the same byte stream gives the same
 * cell grid on the VGA text page (Ring 0 paint) and in the VBE terminal
 * sink, scrolling and scrollback included. */
static void test_text_and_vbe_backends_same_grid(void) {
    static uint16_t text_grid[VGA_ROWS * VGA_COLS];
    const char *stream = "MOHHDY> help\nligne 1\nligne 2 tres longue ................................................................ fin\n";
    int x = 0, y = 0, i, k;
    vga_test_route_ring3 = 0;
    vga_console_set_gfx_sink(0);
    vga_console_init(0x07);
    for (k = 0; k < 12; k++) feed(stream, &x, &y);
    for (i = 0; i < VGA_ROWS * VGA_COLS; i++) text_grid[i] = vga_test_fb[i];

    vga_console_set_gfx_sink(test_sink);
    TEST_ASSERT_EQUAL(1, vga_console_gfx_sink_active());
    vga_console_init(0x07);
    x = 0; y = 0;
    g_sink_calls = 0;
    for (k = 0; k < 12; k++) feed(stream, &x, &y);
    TEST_ASSERT(g_sink_calls > 0);
    for (i = 0; i < VGA_ROWS * VGA_COLS; i++) TEST_ASSERT_EQUAL(text_grid[i], g_sink_cells[i]);
    /* scrollback works the same through the sink */
    TEST_ASSERT(vga_console_view_up(VGA_ROWS - 1) > 0);
    TEST_ASSERT(g_sink_cells[0] != text_grid[0] || g_sink_cells[VGA_COLS] != text_grid[VGA_COLS]);
    vga_console_view_live();
    vga_console_view_down(VGA_ROWS);
    for (i = 0; i < VGA_ROWS * VGA_COLS; i++) TEST_ASSERT_EQUAL(text_grid[i], g_sink_cells[i]);
    vga_console_set_gfx_sink(0);
    TEST_ASSERT_EQUAL(0, vga_console_gfx_sink_active());
}

int main(void) {
    unity_init();
    RUN_TEST(test_text_and_vbe_backends_same_grid);
    RUN_TEST(test_vram_lost_bumps_epoch);
    RUN_TEST(test_init_blanks_screen);
    RUN_TEST(test_put_xy_writes_cell);
    RUN_TEST(test_scroll_saves_history);
    RUN_TEST(test_page_up_shows_saved_line);
    RUN_TEST(test_page_down_returns_to_live);
    RUN_TEST(test_put_while_scrolled_returns_live);
    RUN_TEST(test_view_up_clamped_to_history);
    RUN_TEST(test_cursor_inverts_visible_cell);
    RUN_TEST(test_desktop_blit_and_restore);
    RUN_TEST(test_frame_routed_to_ring3_driver);
    unity_print_results();
    unity_cleanup();
    return (unity_stats.tests_failed == 0) ? 0 : 1;
}
