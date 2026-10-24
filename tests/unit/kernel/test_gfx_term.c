/* VBE backend of the console grid: layout, glyph pixels, colours, diff. */
#include "../../framework/unity.h"
#include "gfx_term.h"
#include <string.h>

#define FW 1024
#define FH 768
static uint32_t fb[FW * FH];
static uint8_t font[GFX_TERM_FONT_BYTES];

static void test_layout_centered_and_scaled(void) {
    gfx_term_layout_t l;
    gfx_term_layout(1024, 768, &l);
    TEST_ASSERT_EQUAL(1, l.scale);
    TEST_ASSERT_EQUAL((1024 - l.w) / 2, l.x);
    TEST_ASSERT(l.gy > l.y);
    gfx_term_layout(1920, 1080, &l);
    TEST_ASSERT_EQUAL(2, l.scale);  /* maximized window: larger cells */
    gfx_term_layout(640, 400, &l);
    TEST_ASSERT_EQUAL(1, l.scale);
    TEST_ASSERT(l.w <= 640 && l.h <= 400);
}

static void test_cell_pixels_follow_font_and_attr(void) {
    gfx_term_layout_t l;
    uint16_t cells[GFX_TERM_COLS * GFX_TERM_ROWS];
    uint16_t shown[GFX_TERM_COLS * GFX_TERM_ROWS];
    int i, n;
    memset(font, 0, sizeof(font));
    font['A' * 32 + 0] = 0x80;          /* top left pixel of 'A' */
    for (i = 0; i < GFX_TERM_COLS * GFX_TERM_ROWS; i++) cells[i] = (uint16_t)(' ' | (0x07 << 8));
    cells[GFX_TERM_COLS + 3] = (uint16_t)('A' | (0x1E << 8)); /* yellow on blue */
    gfx_term_layout(FW, FH, &l);
    memset(shown, 0, sizeof(shown));
    n = gfx_term_render(fb, FW, FH, FW, &l, font, cells, shown, 1);
    TEST_ASSERT_EQUAL(GFX_TERM_COLS * GFX_TERM_ROWS, n);
    {
        int x = l.gx + 3 * GFX_TERM_CELL_W, y = l.gy + 1 * GFX_TERM_CELL_H;
        TEST_ASSERT(gfx_term_rgb(0x0E) == fb[y * FW + x]);
        TEST_ASSERT(gfx_term_rgb(0x01) == fb[y * FW + x + 1]);
        TEST_ASSERT(0 == fb[l.gy * FW + l.gx]);
    }
    /* only changed cells are painted again */
    TEST_ASSERT_EQUAL(0, gfx_term_render(fb, FW, FH, FW, &l, font, cells, shown, 0));
    cells[5] = (uint16_t)('A' | (0x07 << 8));
    TEST_ASSERT_EQUAL(1, gfx_term_render(fb, FW, FH, FW, &l, font, cells, shown, 0));
}

/* Title bar buttons and drag zone, normal and maximized. */
static void test_title_buttons_hit(void) {
    gfx_term_layout_t l;
    int bx, by, w;
    gfx_term_layout_at(FW, FH, 0, 0, 0, &l);
    for (w = GFX_TERM_HIT_MIN; w <= GFX_TERM_HIT_CLOSE; w++) {
        gfx_term_button_pos(&l, w, &bx, &by);
        TEST_ASSERT_EQUAL(w, gfx_term_hit(&l, bx + 7, by + 7));
    }
    TEST_ASSERT_EQUAL(GFX_TERM_HIT_TITLE, gfx_term_hit(&l, l.x + 40, l.y + 8));
    TEST_ASSERT_EQUAL(GFX_TERM_HIT_GRID, gfx_term_hit(&l, l.gx + 10, l.gy + 10));
    TEST_ASSERT_EQUAL(GFX_TERM_HIT_NONE, gfx_term_hit(&l, 2, 2));
    gfx_term_layout_at(FW, FH, 0, -150, -100, &l);
    {
        gfx_term_layout_t c;
        gfx_term_layout(FW, FH, &c);
        TEST_ASSERT_EQUAL(c.x - 150, l.x);
        TEST_ASSERT_EQUAL(c.y - 100, l.y);
    }
    gfx_term_layout_at(FW, FH, 0, -5000, 5000, &l);   /* clamped on screen */
    TEST_ASSERT_EQUAL(0, l.x);
    TEST_ASSERT_EQUAL(FH - l.h, l.y);
    gfx_term_layout_at(FW, FH, 1, 0, 0, &l);
    TEST_ASSERT_EQUAL(0, l.x);
    TEST_ASSERT_EQUAL(FW, l.w);
    gfx_term_button_pos(&l, GFX_TERM_HIT_CLOSE, &bx, &by);
    TEST_ASSERT(bx > FW - 40);
    TEST_ASSERT_EQUAL(GFX_TERM_HIT_CLOSE, gfx_term_hit(&l, bx + 3, by + 3));
}

/* The grid follows the window: 80x25 in a window, the screen when
 * maximized, smaller on a screen too small for 80x25. */
static void test_grid_follows_window(void) {
    gfx_term_layout_t l;
    gfx_term_layout_at(1024, 768, 0, 0, 0, &l);
    TEST_ASSERT_EQUAL(80, l.cols); TEST_ASSERT_EQUAL(25, l.rows);
    TEST_ASSERT_EQUAL(80 * 8 + 4, l.w);
    gfx_term_layout_at(1024, 768, 1, 0, 0, &l);
    TEST_ASSERT_EQUAL(127, l.cols); TEST_ASSERT_EQUAL(46, l.rows);
    TEST_ASSERT(l.gx + l.cols * 8 <= 1024 && l.gy + l.rows * 16 <= 768);
    gfx_term_layout_at(1920, 1080, 1, 0, 0, &l);
    TEST_ASSERT_EQUAL(2, l.scale);
    TEST_ASSERT_EQUAL((1920 - 4) / 16, l.cols);
    TEST_ASSERT_EQUAL((1080 - 4 - 40) / 32, l.rows);
    gfx_term_layout_at(640, 400, 0, 0, 0, &l);
    TEST_ASSERT_EQUAL(79, l.cols); TEST_ASSERT_EQUAL(23, l.rows);
    TEST_ASSERT(l.w <= 640 && l.h <= 400);
}

/* UX3: mouse selection helpers (cell under the pointer, copied text,
 * inverted cells). */
static void test_selection_text_and_highlight(void) {
    uint16_t cells[4 * 3];
    char out[64];
    const char *rows[3] = { "ab  ", "cdef", "g   " };
    gfx_term_layout_t l;
    int i, c, r;
    for (i = 0; i < 12; i++) cells[i] = (uint16_t)((uint8_t)rows[i / 4][i % 4] | (0x07 << 8));
    TEST_ASSERT_EQUAL(2, gfx_term_sel_text(cells, 4, 3, 0, 3, out, (int)sizeof(out)));
    TEST_ASSERT_EQUAL_STRING("ab", out);                    /* trailing blanks dropped */
    gfx_term_sel_text(cells, 4, 3, 9, 1, out, (int)sizeof(out));   /* either order */
    TEST_ASSERT_EQUAL_STRING("b\ncdef\ng", out);
    TEST_ASSERT_EQUAL(3, gfx_term_sel_text(cells, 4, 3, 0, 11, out, 4));   /* bounded */
    TEST_ASSERT_EQUAL_STRING("ab\n", out);
    gfx_term_sel_apply(cells, 12, 5, 6);
    TEST_ASSERT_EQUAL(0x70, cells[5] >> 8);
    TEST_ASSERT_EQUAL(0x70, cells[6] >> 8);
    TEST_ASSERT_EQUAL(0x07, cells[7] >> 8);
    TEST_ASSERT_EQUAL('d', cells[5] & 0xFF);
    gfx_term_layout(1024, 768, &l);
    TEST_ASSERT(gfx_term_cell_at(&l, l.gx, l.gy, &c, &r));
    TEST_ASSERT_EQUAL(0, c); TEST_ASSERT_EQUAL(0, r);
    TEST_ASSERT(gfx_term_cell_at(&l, l.gx + 3 * GFX_TERM_CELL_W * l.scale + 1, l.gy + 2 * GFX_TERM_CELL_H * l.scale, &c, &r));
    TEST_ASSERT_EQUAL(3, c); TEST_ASSERT_EQUAL(2, r);
    TEST_ASSERT(!gfx_term_cell_at(&l, l.gx - 1, l.gy, &c, &r));
    TEST_ASSERT(!gfx_term_cell_at(&l, l.gx, l.gy + l.rows * GFX_TERM_CELL_H * l.scale, &c, &r));
}

int main(void) {
    unity_init();
    RUN_TEST(test_layout_centered_and_scaled);
    RUN_TEST(test_cell_pixels_follow_font_and_attr);
    RUN_TEST(test_title_buttons_hit);
    RUN_TEST(test_grid_follows_window);
    RUN_TEST(test_selection_text_and_highlight);
    unity_print_results();
    unity_cleanup();
    return (unity_stats.tests_failed == 0) ? 0 : 1;
}
