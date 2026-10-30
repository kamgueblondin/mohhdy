/* Lot 62: percentage margins and paddings resolve against the width of the
 * containing block, vertical sides included; intrinsic sizes count them 0. */
static void test_l62_spacing_block_percentages(void) {
    lot52_render(L62_BODY "<div id=g style='width:200px;padding:0 10px'>"
                 "<div id=a style='margin:5% 10%;padding:10% 0 0 5%;height:10px'>A</div></div></body>", 320);
    l61_box("a", 30, 10, 160, 30);                       /* 5% and 10% of 200, padding-top 20 */
    TEST_ASSERT_EQUAL(40, find_text("A")->x);
    TEST_ASSERT_EQUAL(31, find_text("A")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->padding[0]);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->padding[3]);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l62_spacing_auto_negative_and_tenths(void) {
    lot52_render(L62_BODY "<div id=c style='width:100px;margin:10% auto'></div>"
                 "<div id=n style='margin-left:-10%;padding-right:33.3%;height:2px'></div>"
                 "<div id=t style='margin-left:12.5%;height:2px'></div></body>", 320);
    l61_box("c", 110, 32, 100, 0);
    l61_box("n", -32, 64, 352, 2);
    TEST_ASSERT_EQUAL(106, lot52_node("n")->padding[1]);  /* 33.3% of 320 rounds down */
    l61_box("t", 40, 66, 280, 2);
}

static void test_l62_spacing_cascade_order(void) {
    lot52_render(L62_BODY "<div id=a style='margin-left:10%;margin-left:4px;height:1px'></div>"
                 "<div id=b style='margin-left:4px;margin-left:10%;height:1px'></div>"
                 "<div id=c style='margin:10%;margin:0 0 0 2px;height:1px'></div>"
                 "<div id=d style='padding:10%;padding-left:3px;height:1px'></div>"
                 "<div id=e style='padding-inline:5%;padding-inline:1px;height:1px'></div>"
                 "<div id=f style='margin-inline:5% auto;width:100px;height:1px'></div></body>", 200);
    TEST_ASSERT_EQUAL(4, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(20, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(2, lot52_node("c")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("c")->margin[0]);
    TEST_ASSERT_EQUAL(3, lot52_node("d")->padding[3]);
    TEST_ASSERT_EQUAL(20, lot52_node("d")->padding[1]);
    TEST_ASSERT_EQUAL(1, lot52_node("e")->padding[3]);
    TEST_ASSERT_EQUAL(1, lot52_node("e")->padding[1]);
    TEST_ASSERT_EQUAL(10, lot52_node("f")->x);
    TEST_ASSERT_TRUE(lot52_node("f")->margin_auto & WE_AUTO_RIGHT);
    lot52_render(L62_BODY "<div style='width:200px'><div id=g style='padding-inline:10% 2px;height:1px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(20, lot52_node("g")->padding[3]);  /* logical pair percentages */
    TEST_ASSERT_EQUAL(2, lot52_node("g")->padding[1]);
    TEST_ASSERT_EQUAL(200, lot52_node("g")->w);
}

static void test_l62_spacing_follows_the_viewport(void) {
    lot52_render(L62_BODY "<div id=a style='margin-left:10%;height:4px'></div></body>", 320);
    TEST_ASSERT_EQUAL(32, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, we_layout_viewport(&g_doc, 200, 400));
    TEST_ASSERT_EQUAL(20, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(180, lot52_node("a")->w);
    /* the root box resolves against the viewport */
    lot52_render("<body style='margin:0 5%'><div id=b style='height:1px'></div></body>", 200);
    l61_box("b", 10, 0, 180, 1);
}

static void test_l62_spacing_intrinsic_counts_zero(void) {
    lot52_render(L62_BODY "<div style='width:200px'><span id=ib style='display:inline-block'>"
                 "<div id=f style='padding-left:50%'>ABCD</div></span></div></body>", 320);
    l61_box("ib", 0, 0, 32, 10);                         /* shrink-to-fit ignores the 50% */
    TEST_ASSERT_EQUAL(16, find_text("ABCD")->x);         /* then 50% of 32 */
    /* a second layout counts 0 again, not the value resolved by the first */
    TEST_ASSERT_EQUAL(0, we_layout_viewport(&g_doc, 320, 400));
    l61_box("ib", 0, 0, 32, 10);
    TEST_ASSERT_EQUAL(16, find_text("ABCD")->x);
}

static void test_l62_spacing_through_inline_and_flex(void) {
    lot52_render(L62_BODY "<div style='width:200px'>A<span><span id=ib style='display:inline-block;margin-left:10%'>B</span></span></div>"
                 "<div style='display:flex;width:200px'><div id=x style='margin-left:12.5%;width:20px'>x</div></div></body>", 320);
    TEST_ASSERT_EQUAL(28, find_text("B")->x);
    l61_box("x", 25, 10, 20, 10);
}

static void test_l62_spacing_refusals(void) {
    lot52_render(L62_BODY "<div id=a style='padding-left:4px;padding-left:-10%;padding-top:-1%;margin-top:3px;margin-top:10%%;height:1px'></div></body>", 320);
    TEST_ASSERT_EQUAL(4, lot52_node("a")->padding[3]);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->padding[0]);
    TEST_ASSERT_EQUAL(3, lot52_node("a")->margin[0]);
    TEST_ASSERT_EQUAL(2, l61_diag_count("padding refused"));
    lot52_render(L62_BODY "<div id=a style='margin-left:1001%;height:1px'></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
}

static void lot62_spacing_run(void) {
    RUN_TEST(test_l62_spacing_block_percentages);
    RUN_TEST(test_l62_spacing_auto_negative_and_tenths);
    RUN_TEST(test_l62_spacing_cascade_order);
    RUN_TEST(test_l62_spacing_follows_the_viewport);
    RUN_TEST(test_l62_spacing_intrinsic_counts_zero);
    RUN_TEST(test_l62_spacing_through_inline_and_flex);
    RUN_TEST(test_l62_spacing_refusals);
}
