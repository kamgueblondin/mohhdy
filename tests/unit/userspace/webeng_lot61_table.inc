/* Lot 61: border-collapse shares cell borders (uniform widest width). */
#define L61_COLLAPSE "<body style='margin:0'><table id=t style='width:60px;border-collapse:collapse'><tbody><tr>" \
    "<td id=a style='border:2px solid #ff0000'>A</td><td id=b style='border:2px solid #0000ff'>B</td></tr><tr>" \
    "<td id=c style='border:1px solid #00ff00'>C</td><td id=d style='border:2px solid #ff0000'>D</td></tr></tbody></table></body>"

/* Last painted colour at (x, y), borders drawn as their four edges. */
static uint32_t l61_paint(int x, int y) {
    uint32_t i, color = 0xFFFFFFFFu;
    for (i = 0; i < g_doc.nitems; i++) {
        const os_fb_web_item_t *it = &g_doc.items[i];
        int inside = x >= it->x && y >= it->y && x < it->x + it->w && y < it->y + it->h;
        if (!inside) continue;
        if (it->kind == OS_FB_WEB_RECT) color = it->color;
        else if (it->kind == OS_FB_WEB_BORDER) {
            int t = it->bold ? it->bold : 1;
            if (x < it->x + t || y < it->y + t || x >= it->x + it->w - t || y >= it->y + it->h - t) color = it->color;
        }
    }
    return color;
}

static void test_l61_collapse_shares_vertical_and_horizontal_edges(void) {
    lot52_render(L61_COLLAPSE, 320);
    TEST_ASSERT_EQUAL(2, lot52_node("t")->collapse_w);
    l61_box("t", 0, 0, 60, 26);
    l61_box("a", 0, 0, 31, 14);
    l61_box("b", 29, 0, 31, 14);
    l61_box("c", 0, 12, 31, 12);
    l61_box("d", 29, 12, 31, 14);
    l61_text_at("A", 2, 3);
    l61_text_at("B", 31, 3);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l61_collapse_shared_edge_paints_once(void) {
    lot52_render(L61_COLLAPSE, 320);
    /* The shared column x 29..30 is one 2px line, owned by the later cell. */
    TEST_ASSERT_EQUAL(0x0000FF, l61_paint(29, 6));
    TEST_ASSERT_EQUAL(0x0000FF, l61_paint(30, 6));
    TEST_ASSERT_TRUE(l61_paint(31, 6) != 0x0000FF);
    TEST_ASSERT_EQUAL(0xFF0000, l61_paint(0, 6));
    /* The shared row y 12..13 under a: c paints its 1px top over a's edge. */
    TEST_ASSERT_EQUAL(0x00FF00, l61_paint(5, 12));
    TEST_ASSERT_EQUAL(0xFF0000, l61_paint(5, 13));
}

static void test_l61_collapse_without_borders_keeps_zero_spacing(void) {
    lot52_render("<body style='margin:0'><table id=t style='width:60px;border-spacing:10px;border-collapse:collapse'><tr>"
        "<td id=a>A</td><td id=b>B</td></tr></table></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("t")->collapse_w);
    l61_box("a", 0, 0, 30, 10);
    l61_box("b", 30, 0, 30, 10);
}

static void test_l61_collapse_separate_keeps_double_borders(void) {
    lot52_render("<body style='margin:0'><table id=t style='width:60px;border-spacing:0'><tr>"
        "<td id=a style='border:2px solid #ff0000'>A</td><td id=b style='border:2px solid #0000ff'>B</td></tr></table></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("t")->collapse_w);
    l61_box("a", 0, 0, 30, 14);
    l61_box("b", 30, 0, 30, 14);
}

static void test_l61_collapse_with_spans_and_intrinsic_width(void) {
    lot52_render("<body style='margin:0'><div style='display:inline-block'><table id=t style='border-collapse:collapse'><tr>"
        "<td id=a style='border:3px solid #ff0000;width:10px'>A</td><td id=b colspan=2 style='border:1px solid #0000ff;width:20px'>B</td></tr><tr>"
        "<td id=c rowspan=2 style='border:3px solid #00ff00'>C</td><td id=d>D</td><td id=e>E</td></tr><tr>"
        "<td id=f>F</td><td id=g>G</td></tr></table></div></body>", 320);
    TEST_ASSERT_EQUAL(3, lot52_node("t")->collapse_w);
    l61_box("t", 0, 0, 32, 30);
    l61_box("a", 0, 0, 16, 16);
    l61_box("b", 13, 0, 19, 12);
    l61_box("c", 0, 13, 16, 17);
    l61_box("d", 13, 13, 11, 10);
    l61_box("e", 21, 13, 11, 10);
    l61_box("g", 21, 20, 11, 10);
}

static void test_l61_collapse_successive_layouts_are_stable(void) {
    int pass;
    lot52_render(L61_COLLAPSE, 320);
    for (pass = 0; pass < 2; pass++) {
        TEST_ASSERT_EQUAL(0, we_layout(&g_doc, 320));
        TEST_ASSERT_EQUAL(2, lot52_node("t")->collapse_w);
        l61_box("b", 29, 0, 31, 14);
        l61_box("d", 29, 12, 31, 14);
    }
}

static void lot61_table_run(void) {
    RUN_TEST(test_l61_collapse_shares_vertical_and_horizontal_edges);
    RUN_TEST(test_l61_collapse_shared_edge_paints_once);
    RUN_TEST(test_l61_collapse_without_borders_keeps_zero_spacing);
    RUN_TEST(test_l61_collapse_separate_keeps_double_borders);
    RUN_TEST(test_l61_collapse_with_spans_and_intrinsic_width);
    RUN_TEST(test_l61_collapse_successive_layouts_are_stable);
}
