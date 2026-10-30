/* Lot 62 round 2: percentage margins and paddings of grid items resolve
 * against the grid area width, those of absolute boxes against the padding
 * box of the containing block (the viewport for fixed boxes). */
static void test_l62_cb_grid_area_percentages(void) {
    lot52_render(L62_BODY "<div style='display:grid;grid-template-columns:100px 200px;width:300px'>"
                 "<div id=a style='padding-left:10%;height:10px'>x</div>"
                 "<div id=b style='margin-top:5%;margin-left:10%;height:10px'></div></div>"
                 "<div id=n style='height:1px'></div></body>", 320);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->padding[3]);  /* 10% of the 100px area, not of 300 */
    TEST_ASSERT_EQUAL(10, find_text("x")->x);
    l61_box("b", 120, 10, 180, 10);                       /* 5% and 10% of the 200px area */
    l61_box("n", 0, 20, 320, 1);                          /* the row counts the resolved margin */
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l62_cb_grid_tracks_count_zero(void) {
    lot52_render(L62_BODY "<span style='display:inline-block'><div style='display:grid;grid-template-columns:auto 40px'>"
                 "<div id=a style='padding-left:50%'>AB</div><div id=b>C</div></div></span></body>", 400);
    l61_box("a", 0, 0, 16, 10);                           /* the auto track ignores the 50% */
    l61_box("b", 16, 0, 40, 10);
    TEST_ASSERT_EQUAL(8, find_text("AB")->x);             /* then 50% of the 16px area */
}

static void test_l62_cb_absolute_padding_box(void) {
    lot52_render(L62_BODY "<div style='height:20px'></div><div style='position:relative;width:200px;height:100px;padding:10px'>"
                 "<div id=c style='position:absolute;left:0;top:0;margin-left:10%;padding-top:10%;width:20px;height:5px'></div></div></body>", 320);
    l61_box("c", 22, 20, 20, 27);                         /* 10% of the 220px padding box */
    TEST_ASSERT_EQUAL(22, lot52_node("c")->padding[0]);
}

static void test_l62_cb_absolute_chain_and_fixed(void) {
    lot52_render(L62_BODY "<div style='position:relative;width:100px;padding:0 50px;height:20px'>"
                 "<div style='position:absolute;width:200px'><div id=c style='position:absolute;left:0;margin-left:10%;width:4px;height:4px'></div></div></div>"
                 "<div id=f style='position:fixed;top:0;left:0;margin-left:25%;width:4px;height:4px'></div></body>", 400);
    TEST_ASSERT_EQUAL(70, lot52_node("c")->x);            /* static 50 + 10% of the inner 200 */
    TEST_ASSERT_EQUAL(100, lot52_node("f")->x);           /* 25% of the 400px viewport */
}

static void lot62_cb_run(void) {
    RUN_TEST(test_l62_cb_grid_area_percentages);
    RUN_TEST(test_l62_cb_grid_tracks_count_zero);
    RUN_TEST(test_l62_cb_absolute_padding_box);
    RUN_TEST(test_l62_cb_absolute_chain_and_fixed);
}
