/* Lot 61: dashed, dotted and double borders painted as bounded RECTs. */
static int l61_count_color(uint32_t color) {
    uint32_t i;
    int count = 0;
    for (i = 0; i < g_doc.nitems; i++)
        if (g_doc.items[i].kind == OS_FB_WEB_RECT && g_doc.items[i].color == color) count++;
    return count;
}

static void test_l61_bstyle_separate_dashed(void) {
    lot52_render("<body style='margin:0'><div id=x style='border:2px dashed #ff0000;width:20px;height:6px'></div></body>", 320);
    l61_box("x", 0, 0, 24, 10);
    TEST_ASSERT_EQUAL(0, l61_count(OS_FB_WEB_BORDER));
    l61_rect_at(0, 0, 4, 2, 0xFF0000);                       /* 4px dashes (2w), 4px gaps */
    l61_rect_at(8, 0, 4, 2, 0xFF0000);
    l61_rect_at(16, 0, 4, 2, 0xFF0000);
    l61_rect_at(16, 8, 4, 2, 0xFF0000);
    l61_rect_at(0, 2, 2, 4, 0xFF0000);                       /* sides between top and bottom */
    l61_rect_at(22, 2, 2, 4, 0xFF0000);
    TEST_ASSERT_TRUE(l61_paint(5, 0) == 0xFFFFFFFFu);
    TEST_ASSERT_EQUAL(8, l61_count_color(0xFF0000));
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);    /* The last dash is cut at the end of the line. */
    lot52_render("<body style='margin:0'><div id=x style='border:2px dashed #ff0000;width:6px;height:6px'></div></body>", 320);
    l61_rect_at(8, 0, 2, 2, 0xFF0000);
    l61_rect_at(8, 8, 2, 2, 0xFF0000);
}

static void test_l61_bstyle_separate_dotted(void) {
    lot52_render("<body style='margin:0'><div id=x style='border:1px dotted #ff0000;width:4px;height:2px'></div></body>", 320);
    l61_rect_at(0, 0, 1, 1, 0xFF0000);
    l61_rect_at(2, 0, 1, 1, 0xFF0000);
    l61_rect_at(4, 0, 1, 1, 0xFF0000);
    l61_rect_at(4, 3, 1, 1, 0xFF0000);
    TEST_ASSERT_TRUE(l61_paint(1, 0) == 0xFFFFFFFFu);
    TEST_ASSERT_EQUAL(8, l61_count_color(0xFF0000));
}

static void test_l61_bstyle_separate_double(void) {
    lot52_render("<body style='margin:0'><div id=x style='border:3px double #ff0000;width:4px;height:4px'>"
                 "</div><div id=y style='border:2px double #0000ff;width:4px;height:4px'></div></body>", 320);
    l61_box("x", 0, 0, 10, 10);
    l61_rect_at(0, 0, 10, 1, 0xFF0000);                      /* (w + 1) / 3 = 1px lines */
    l61_rect_at(0, 2, 10, 1, 0xFF0000);
    l61_rect_at(0, 9, 10, 1, 0xFF0000);
    l61_rect_at(9, 3, 1, 4, 0xFF0000);
    TEST_ASSERT_TRUE(l61_paint(5, 1) == 0xFFFFFFFFu);        /* the gap */
    TEST_ASSERT_TRUE(l61_paint(1, 5) == 0xFFFFFFFFu);
    /* Under 3px a double border is solid: one BORDER item. */
    TEST_ASSERT_EQUAL(1, l61_count(OS_FB_WEB_BORDER));
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l61_bstyle_long_lines_are_bounded(void) {
    lot52_render("<body style='margin:0'><div id=x style='border:1px dashed #ff0000;width:198px;height:2px'></div></body>", 320);
    l61_rect_at(0, 0, 3, 1, 0xFF0000);
    l61_rect_at(25, 0, 3, 1, 0xFF0000);                      /* period grows to len / 8 */
    l61_rect_at(175, 0, 3, 1, 0xFF0000);
    TEST_ASSERT_EQUAL(18, l61_count_color(0xFF0000));        /* 8 + 8 + 2 sides */
    TEST_ASSERT_EQUAL(0, g_doc.perf.items_dropped);
}

static void test_l61_bstyle_3d_styles_paint_solid_with_diagnostic(void) {
    lot52_render("<body style='margin:0'><div id=x style='border:2px groove #ff0000;width:4px;height:4px'></div>"
                 "<div id=y style='border:2px outset #ff0000;width:4px;height:4px'></div></body>", 320);
    TEST_ASSERT_EQUAL(2, l61_count(OS_FB_WEB_BORDER));
    TEST_ASSERT_EQUAL(1, l61_diag_count("border style painted solid"));
    lot52_render("<body style='margin:0'><div id=x style='border:2px solid #ff0000;width:4px;height:4px'></div></body>", 320);
    TEST_ASSERT_EQUAL(0, l61_diag_count("border style painted solid"));
}

static void test_l61_bstyle_collapsed_double_and_runs_split_by_style(void) {
    lot52_render("<body style='margin:0'><table id=t style='width:40px;border-collapse:collapse'><tr>"
                 "<td id=a style='border:6px double #ff0000;height:4px'></td>"
                 "<td id=b style='border:6px solid #ff0000;border-left:0;height:4px'></td></tr></table></body>", 320);
    l61_rect_at(0, 0, 23, 2, 0xFF0000);                      /* a's double top: 2px lines */
    l61_rect_at(0, 4, 23, 2, 0xFF0000);
    l61_rect_at(17, 0, 23, 6, 0xFF0000);                     /* b's solid top is its own run */
    l61_rect_at(17, 0, 2, 16, 0xFF0000);                     /* the wider double wins the middle */
    l61_rect_at(21, 0, 2, 16, 0xFF0000);
    TEST_ASSERT_TRUE(l61_paint(20, 8) == 0xFFFFFFFFu);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void lot61_bstyle_run(void) {
    RUN_TEST(test_l61_bstyle_separate_dashed);
    RUN_TEST(test_l61_bstyle_separate_dotted);
    RUN_TEST(test_l61_bstyle_separate_double);
    RUN_TEST(test_l61_bstyle_long_lines_are_bounded);
    RUN_TEST(test_l61_bstyle_3d_styles_paint_solid_with_diagnostic);
    RUN_TEST(test_l61_bstyle_collapsed_double_and_runs_split_by_style);
}
