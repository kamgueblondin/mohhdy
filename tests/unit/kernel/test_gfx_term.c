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

int main(void) {
    unity_init();
    RUN_TEST(test_layout_centered_and_scaled);
    RUN_TEST(test_cell_pixels_follow_font_and_attr);
    RUN_TEST(test_title_buttons_hit);
    unity_print_results();
    unity_cleanup();
    return (unity_stats.tests_failed == 0) ? 0 : 1;
}
