/* Lot 61: partial glyph clipping from the shared kernel font. */
#include "../../../kernel/gfx_font8.h"
#define L61_RW 96
#define L61_RH 32
static uint8_t l61_bm[2][L61_RH][L61_RW];

/* Paints the display list like the kernel pane: RECT fills, TEXT glyph bits
 * at x + bit (bold also x + bit + 1), rows 0..7 from the item y. */
static void l61_raster(int slot) {
    uint32_t i;
    memset(l61_bm[slot], 0, sizeof(l61_bm[slot]));
    for (i = 0; i < g_doc.nitems; i++) {
        const os_fb_web_item_t *it = &g_doc.items[i];
        int x, y, k, row, bit;
        if (it->kind == OS_FB_WEB_RECT) {
            for (y = it->y; y < it->y + it->h; y++)
                for (x = it->x; x < it->x + it->w; x++)
                    if (x >= 0 && y >= 0 && x < L61_RW && y < L61_RH) l61_bm[slot][y][x] = 1;
        } else if (it->kind == OS_FB_WEB_TEXT) {
            for (k = 0; it->text[k]; k++) {
                unsigned char ch = (unsigned char)it->text[k];
                if (ch < 32 || ch > 126) ch = '?';
                for (row = 0; row < 8; row++) {
                    unsigned m = gfx_font8[ch - 32][row];
                    if (it->bold) m |= m << 1;
                    for (bit = 0; bit < 9; bit++) {
                        x = it->x + k * 8 + bit; y = it->y + row;
                        if ((m >> bit) & 1u && x >= 0 && y >= 0 && x < L61_RW && y < L61_RH) l61_bm[slot][y][x] = 1;
                    }
                }
            }
        }
    }
}

/* The clipped page must paint exactly the unclipped pixels inside the box. */
static void l61_clip_matches(const char *open_html, const char *clip_html, int left, int top, int right, int bottom) {
    int x, y, lit = 0;
    lot52_render(open_html, 320);
    l61_raster(0);
    lot52_render(clip_html, 320);
    l61_raster(1);
    for (y = 0; y < L61_RH; y++)
        for (x = 0; x < L61_RW; x++) {
            int want = x >= left && x < right && y >= top && y < bottom ? l61_bm[0][y][x] : 0;
            TEST_ASSERT_EQUAL(want, l61_bm[1][y][x]);
            lit += want;
        }
    TEST_ASSERT_TRUE(lit > 0);
}

static void test_l61_clip_right_edge_is_pixel_exact(void) {
    l61_clip_matches("<body style='margin:0'><div style='width:80px;white-space:nowrap'>abcdef</div></body>",
        "<body style='margin:0'><div style='width:20px;height:10px;overflow:hidden;white-space:nowrap'>abcdef</div></body>",
        0, 0, 20, 10);
    TEST_ASSERT_NOT_NULL(find_text("ab"));
    TEST_ASSERT_EQUAL(16, find_text("ab")->w);
    TEST_ASSERT_TRUE(l61_count(OS_FB_WEB_RECT) > 0);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l61_clip_left_edge_is_pixel_exact(void) {
    l61_clip_matches("<body style='margin:0'><div style='margin-left:-4px;width:48px;white-space:nowrap'>abcdef</div></body>",
        "<body style='margin:0'><div style='width:24px;height:10px;overflow:hidden'><div style='margin-left:-4px;width:48px;white-space:nowrap'>abcdef</div></div></body>",
        0, 0, 24, 10);
    TEST_ASSERT_EQUAL(4, find_text("bc")->x);
}

static void test_l61_clip_top_and_bottom_are_pixel_exact(void) {
    l61_clip_matches("<body style='margin:0'><div style='width:60px'>WAg</div></body>",
        "<body style='margin:0'><div style='width:60px;height:5px;overflow:hidden'>WAg</div></body>", 0, 0, 60, 5);
    TEST_ASSERT_NULL(find_text("WAg"));
    l61_clip_matches("<body style='margin:0'><div style='margin-top:-4px;width:60px'>WAg</div></body>",
        "<body style='margin:0'><div style='width:60px;height:10px;overflow:hidden'><div style='margin-top:-4px'>WAg</div></div></body>", 0, 0, 60, 10);
}

static void test_l61_clip_bold_extra_column_is_cut(void) {
    l61_clip_matches("<body style='margin:0'><div style='width:60px'><b>ll</b></div></body>",
        "<body style='margin:0'><div style='width:13px;height:10px;overflow:hidden'><b>ll</b></div></body>", 0, 0, 13, 10);
    TEST_ASSERT_NOT_NULL(find_text("l"));
    TEST_ASSERT_EQUAL(1, find_text("l")->bold);
}

static void test_l61_clip_nested_boxes_intersect(void) {
    l61_clip_matches("<body style='margin:0'><div style='width:60px;white-space:nowrap'>xyzw</div></body>",
        "<body style='margin:0'><div style='width:30px;height:10px;overflow:hidden'><div style='width:21px;height:6px;overflow:hidden;white-space:nowrap'>xyzw</div></div></body>",
        0, 0, 21, 6);
}

static void test_l61_clip_runs_merge_down_columns(void) {
    int i, tall = 0;
    lot52_render("<body style='margin:0'><div style='width:4px;height:10px;overflow:hidden'>o</div></body>", 320);
    /* 'o' is 7e 66 66 66 7e from row 2: columns 1..2 of rows 3..5 merge
     * into one 2x3 run at y 4, the full rows 2 and 6 stay 3x1 at x 1. */
    for (i = 0; i < (int)g_doc.nitems; i++)
        if (g_doc.items[i].kind == OS_FB_WEB_RECT && g_doc.items[i].x == 1 && g_doc.items[i].w == 2 &&
            g_doc.items[i].y == 4 && g_doc.items[i].h == 3) tall++;
    TEST_ASSERT_EQUAL(1, tall);
    TEST_ASSERT_EQUAL(3, g_doc.nitems);
    l61_rect_is(0x000000, 1, 3, 3, 1);
    TEST_ASSERT_EQUAL(7, g_doc.items[2].y);
    TEST_ASSERT_EQUAL(3, g_doc.items[2].w);
    TEST_ASSERT_NULL(find_text("o"));
}

static void test_l61_clip_empty_row_cut_keeps_text(void) {
    lot52_render("<body style='margin:0'><div style='width:40px;height:8px;overflow:hidden'>one</div></body>", 320);
    TEST_ASSERT_NOT_NULL(find_text("one"));
    TEST_ASSERT_EQUAL(1, g_doc.nitems);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l61_clip_spaces_emit_nothing(void) {
    lot52_render("<body style='margin:0'><div style='width:20px;height:10px;overflow:hidden;white-space:pre'>a   b</div></body>", 320);
    TEST_ASSERT_NOT_NULL(find_text("a"));
    TEST_ASSERT_EQUAL(8, find_text("a")->w);
    TEST_ASSERT_EQUAL(1, g_doc.nitems);
}

static void test_l61_clip_fragment_budget_is_diagnosed(void) {
    lot52_render("<body style='margin:0'><div style='width:300px;height:6px;overflow:hidden;white-space:nowrap'>"
        "WWWWWWWWWWWWWWWWWWWWWWWWWWWWWW</div></body>", 320);
    TEST_ASSERT_TRUE(lot52_has_diag("clipping display budget"));
    TEST_ASSERT_EQUAL(1, (int)g_doc.perf.items_dropped);
    TEST_ASSERT_EQUAL(0, g_doc.nitems);
}

static void test_l61_clip_successive_layouts_are_stable(void) {
    uint32_t n;
    int pass;
    lot52_render("<body style='margin:0'><div style='width:20px;height:6px;overflow:hidden;white-space:nowrap'>abcdef</div></body>", 320);
    n = g_doc.nitems;
    l61_raster(0);
    for (pass = 0; pass < 2; pass++) {
        TEST_ASSERT_EQUAL(0, we_layout(&g_doc, 320));
        TEST_ASSERT_EQUAL(n, g_doc.nitems);
        l61_raster(1);
        TEST_ASSERT_EQUAL_MEMORY(l61_bm[0], l61_bm[1], sizeof(l61_bm[0]));
    }
}

static void lot61_clip_run(void) {
    RUN_TEST(test_l61_clip_right_edge_is_pixel_exact);
    RUN_TEST(test_l61_clip_left_edge_is_pixel_exact);
    RUN_TEST(test_l61_clip_top_and_bottom_are_pixel_exact);
    RUN_TEST(test_l61_clip_bold_extra_column_is_cut);
    RUN_TEST(test_l61_clip_nested_boxes_intersect);
    RUN_TEST(test_l61_clip_runs_merge_down_columns);
    RUN_TEST(test_l61_clip_empty_row_cut_keeps_text);
    RUN_TEST(test_l61_clip_spaces_emit_nothing);
    RUN_TEST(test_l61_clip_fragment_budget_is_diagnosed);
    RUN_TEST(test_l61_clip_successive_layouts_are_stable);
}
