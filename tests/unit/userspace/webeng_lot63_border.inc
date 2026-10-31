/* Lot 63: rounded dashed and dotted borders: the pattern on the straight
 * parts between the corner boxes. Lot 64: an arc holding one period takes
 * the pattern too, a shorter one stays solid. */
static void test_l63_border_rounded_dotted(void) {
    lot52_render(L62_BODY "<div style='width:20px;height:12px;border:2px dotted #808080;border-radius:6px'></div></body>", 320);
    TEST_ASSERT_EQUAL(16, (int)g_doc.nitems);
    l61_rect_at(1, 4, 2, 2, 0x808080);                /* lot 64: two dots on the top-left arc */
    l61_rect_at(3, 1, 2, 2, 0x808080);
    TEST_ASSERT_EQUAL(0xFFFFFFFFu, l61_paint(2, 2)); /* between them */
    l61_rect_at(6, 0, 2, 2, 0x808080);                /* dots from x 6 to 18 */
    l61_rect_at(14, 0, 2, 2, 0x808080);
    l61_rect_at(0, 6, 2, 2, 0x808080);                /* one dot between the left arcs */
    l61_rect_at(3, 13, 2, 2, 0x808080);               /* the bottom-left arc, mirrored */
    l61_rect_at(1, 10, 2, 2, 0x808080);
    l61_rect_at(19, 1, 2, 2, 0x808080);
    l61_rect_at(21, 10, 2, 2, 0x808080);
    TEST_ASSERT_EQUAL(0xFFFFFFFFu, l61_paint(0, 0));
    TEST_ASSERT_EQUAL(0xFFFFFFFFu, l61_paint(23, 15));
    TEST_ASSERT_EQUAL(0, l61_diag_count("rounded effect painted square"));
    TEST_ASSERT_EQUAL(1, l61_diag_count("border-radius painted in steps"));
}

static void test_l63_border_rounded_dashed_mixed_corners(void) {
    lot52_render(L62_BODY "<div style='width:40px;height:12px;border:2px dashed #ff0000;border-radius:6px 0 0 0'></div></body>", 320);
    TEST_ASSERT_EQUAL(18, (int)g_doc.nitems);
    l61_rect_at(3, 0, 3, 2, 0xFF0000);
    l61_rect_at(6, 0, 4, 2, 0xFF0000);                /* 4 px dashes from the arc */
    l61_rect_at(38, 0, 4, 2, 0xFF0000);
    l61_rect_at(0, 14, 4, 2, 0xFF0000);               /* square corner: from x 0 */
    l61_rect_at(40, 14, 4, 2, 0xFF0000);
    l61_rect_at(42, 2, 2, 4, 0xFF0000);               /* square corner: below the top side */
    l61_rect_at(0, 6, 2, 4, 0xFF0000);
    TEST_ASSERT_EQUAL(0xFFFFFFFFu, l61_paint(43, 0));       /* a gap of the pattern, as on a square box */
    TEST_ASSERT_EQUAL(0xFFFFFFFFu, l61_paint(0, 0));
}

/* radii below the width: the sides cover the corner box's remainder */
static void test_l63_border_rounded_thick_and_elliptic(void) {
    int x, y, holes = 0;
    lot52_render(L62_BODY "<div style='width:20px;height:12px;border:4px dotted #ff0000;border-radius:2px'></div></body>", 320);
    l61_rect_at(1, 0, 1, 1, 0xFF0000);
    l61_rect_at(0, 1, 2, 1, 0xFF0000);
    l61_rect_at(2, 0, 4, 4, 0xFF0000);
    l61_rect_at(0, 2, 4, 4, 0xFF0000);
    for (y = 0; y < 4; y++) for (x = 0; x < 4; x++) if (l61_paint(x, y) == 0xFFFFFFFFu && (x || y)) holes++;
    TEST_ASSERT_EQUAL(0, holes);                      /* only the corner pixel stays out */
    lot52_render(L62_BODY "<div style='width:30px;height:10px;border:2px dashed #ff0000;border-radius:12px / 4px'></div></body>", 320);
    TEST_ASSERT_EQUAL(14, (int)g_doc.nitems);
    l61_rect_at(3, 2, 2, 2, 0xFF0000);                /* lot 64: one dash on the 12 x 4 arc */
    l61_rect_at(5, 1, 2, 2, 0xFF0000);
    TEST_ASSERT_EQUAL(0xFFFFFFFFu, l61_paint(9, 0));
    l61_rect_at(12, 0, 4, 2, 0xFF0000);               /* 10 px between the arcs: one dash and a part */
    l61_rect_at(20, 0, 2, 2, 0xFF0000);
    l61_rect_at(0, 4, 2, 4, 0xFF0000);
}

static void test_l63_border_rounded_hidden_and_square(void) {
    lot52_render(L62_BODY "<div style='visibility:hidden;width:20px;height:12px;border:2px dashed #ff0000;border-radius:6px'></div></body>", 320);
    TEST_ASSERT_EQUAL(0, (int)g_doc.nitems);
    lot52_render(L62_BODY "<div style='width:20px;height:12px;border:2px dashed #ff0000'></div></body>", 320);
    l61_rect_at(0, 0, 4, 2, 0xFF0000);                /* no radius: the lot 61 square path */
    l61_rect_at(0, 2, 2, 4, 0xFF0000);
    TEST_ASSERT_EQUAL(0, l61_diag_count("border-radius"));
}

static void lot63_border_run(void) {
    RUN_TEST(test_l63_border_rounded_dotted);
    RUN_TEST(test_l63_border_rounded_dashed_mixed_corners);
    RUN_TEST(test_l63_border_rounded_thick_and_elliptic);
    RUN_TEST(test_l63_border_rounded_hidden_and_square);
}
