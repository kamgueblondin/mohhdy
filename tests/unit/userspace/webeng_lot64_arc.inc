/* Lot 64 D: dashed and dotted patterns along rounded corner arcs. */

static int l64_rects_in(int x0, int y0, int x1, int y1) {
    uint32_t i;
    int count = 0;
    for (i = 0; i < g_doc.nitems; i++) {
        const os_fb_web_item_t *it = &g_doc.items[i];
        if (it->kind == OS_FB_WEB_RECT && it->x >= x0 && it->y >= y0 && it->x + it->w <= x1 && it->y + it->h <= y1) count++;
    }
    return count;
}

static void test_l64_arc_dashed_periods(void) {
    /* centre line radius 15: L = 23 holds 3 periods of 8, a dash = 2 squares */
    lot52_render(L64_BODY "<div style='width:40px;height:40px;border:2px dashed #ff0000;border-radius:16px 0 0 0'></div></body>", 320);
    TEST_ASSERT_EQUAL(25, (int)g_doc.nitems);
    TEST_ASSERT_EQUAL(6, l64_rects_in(0, 0, 16, 16));
    l61_rect_at(1, 13, 2, 2, 0xFF0000);                       /* the first dash, near the left side */
    l61_rect_at(1, 11, 2, 2, 0xFF0000);
    l61_rect_at(4, 6, 2, 2, 0xFF0000);                        /* the middle one, at 45 degrees */
    l61_rect_at(5, 5, 2, 2, 0xFF0000);
    l61_rect_at(10, 1, 2, 2, 0xFF0000);
    l61_rect_at(12, 1, 2, 2, 0xFF0000);
    TEST_ASSERT_EQUAL(0xFFFFFFFFu, l61_paint(1, 8));          /* gaps between the dashes */
    TEST_ASSERT_EQUAL(0xFFFFFFFFu, l61_paint(7, 2));
    l61_rect_at(16, 0, 4, 2, 0xFF0000);                       /* the straight part restarts its pattern */
    l61_rect_at(0, 16, 2, 4, 0xFF0000);
    /* 1 px: dashes of 3 squares */
    lot52_render(L64_BODY "<div style='width:30px;height:30px;border:1px dashed #0000ff;border-radius:10px 0 0 0'></div></body>", 320);
    l61_rect_at(1, 9, 1, 1, 0x0000FF);
    l61_rect_at(1, 8, 1, 1, 0x0000FF);
    l61_rect_at(1, 7, 1, 1, 0x0000FF);
    l61_rect_at(2, 5, 1, 1, 0x0000FF);
}

static void test_l64_arc_dotted_and_cap(void) {
    /* 4 px dots: centre line radius 10, L = 15: two dots of 4 x 4 */
    lot52_render(L64_BODY "<div style='width:30px;height:30px;border:4px dotted #0000ff;border-radius:12px 0 0 0'></div></body>", 320);
    TEST_ASSERT_EQUAL(2, l64_rects_in(0, 0, 12, 12));
    l61_rect_at(1, 7, 4, 4, 0x0000FF);
    l61_rect_at(6, 1, 4, 4, 0x0000FF);
    TEST_ASSERT_EQUAL(0xFFFFFFFFu, l61_paint(4, 4));
    /* a long arc: at most WE_ARC_PERIODS periods, spread out */
    lot52_render(L64_BODY "<div style='width:100px;height:100px;border:2px dotted #ff0000;border-radius:50px 0 0 0'></div></body>", 320);
    TEST_ASSERT_EQUAL(4, l64_rects_in(0, 0, 50, 50));
    l61_rect_at(1, 40, 2, 2, 0xFF0000);
    l61_rect_at(8, 23, 2, 2, 0xFF0000);
    l61_rect_at(22, 9, 2, 2, 0xFF0000);
    l61_rect_at(39, 2, 2, 2, 0xFF0000);
    /* mirrored at the other corners */
    lot52_render(L64_BODY "<div style='width:30px;height:30px;border:4px dotted #0000ff;border-radius:12px'></div></body>", 320);
    l61_rect_at(1, 7, 4, 4, 0x0000FF);
    l61_rect_at(33, 7, 4, 4, 0x0000FF);                       /* 38 - 1 - 4 */
    l61_rect_at(33, 27, 4, 4, 0x0000FF);
    l61_rect_at(1, 27, 4, 4, 0x0000FF);
    l61_rect_at(6, 33, 4, 4, 0x0000FF);
    l61_rect_at(28, 1, 4, 4, 0x0000FF);
    /* an arc shorter than one period stays solid */
    lot52_render(L64_BODY "<div style='width:40px;height:12px;border:2px dashed #ff0000;border-radius:6px 0 0 0'></div></body>", 320);
    l61_rect_at(3, 0, 3, 2, 0xFF0000);
    l61_rect_at(0, 4, 2, 2, 0xFF0000);
}

static void lot64_arc_run(void) {
    RUN_TEST(test_l64_arc_dashed_periods);
    RUN_TEST(test_l64_arc_dotted_and_cap);
}
