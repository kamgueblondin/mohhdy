/* Lot 61: bounded stacking contexts for relative/absolute boxes with z-index. */
#define L61_STACK_PAIR "<body style='margin:0'><div id=a style='position:relative;z-index:2;height:10px;background:#ff0000'></div>" \
    "<div id=b style='position:relative;z-index:1;height:10px;background:#00ff00'>" \
    "<div id=c style='position:absolute;z-index:8;left:0;top:-5px;width:10px;height:10px;background:#0000ff'></div></div></body>"

static void test_l61_stack_relative_z_paints_over_later_flow(void) {
    lot52_render("<body style='margin:0'><div id=r style='position:relative;z-index:1;width:40px;height:20px;background:#ff0000'></div>"
                 "<div id=s style='margin-top:-10px;width:40px;height:20px;background:#0000ff'></div></body>", 320);
    l61_box("r", 0, 0, 40, 20);
    l61_box("s", 0, 10, 40, 20);
    TEST_ASSERT_TRUE(l61_index(0xFF0000) > l61_index(0x0000FF));
    TEST_ASSERT_EQUAL(0xFF0000, (int)l61_paint(5, 15));
    TEST_ASSERT_EQUAL(0x0000FF, (int)l61_paint(5, 25));
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l61_stack_without_z_keeps_tree_order(void) {
    lot52_render("<body style='margin:0'><div id=r style='position:relative;width:40px;height:20px;background:#ff0000'></div>"
                 "<div id=s style='margin-top:-10px;width:40px;height:20px;background:#0000ff'></div></body>", 320);
    TEST_ASSERT_TRUE(l61_index(0xFF0000) < l61_index(0x0000FF));
    TEST_ASSERT_EQUAL(0x0000FF, (int)l61_paint(5, 15));
}

static void test_l61_stack_context_groups_its_subtree(void) {
    /* c has z-index 8 but lives in b's context (z 1), below a (z 2). */
    lot52_render(L61_STACK_PAIR, 320);
    l61_box("c", 0, 5, 10, 10);
    TEST_ASSERT_EQUAL(0, l61_index(0x00FF00));
    TEST_ASSERT_EQUAL(1, l61_index(0x0000FF));
    TEST_ASSERT_EQUAL(2, l61_index(0xFF0000));
    TEST_ASSERT_EQUAL(0xFF0000, (int)l61_paint(5, 7));
    TEST_ASSERT_EQUAL(0x0000FF, (int)l61_paint(5, 12));
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l61_stack_equal_z_keeps_tree_order(void) {
    lot52_render("<body style='margin:0'><div id=a style='position:relative;z-index:3;height:10px;background:#ff0000'></div>"
                 "<div id=b style='position:relative;z-index:3;margin-top:-5px;height:10px;background:#00ff00'></div></body>", 320);
    TEST_ASSERT_TRUE(l61_index(0xFF0000) < l61_index(0x00FF00));
    TEST_ASSERT_EQUAL(0x00FF00, (int)l61_paint(5, 7));
}

static void test_l61_stack_negative_child_inside_its_parent_context(void) {
    /* p's own background first, then n (z -1) below p's in-flow child f,
     * then the z 1 sibling q. */
    lot52_render("<body style='margin:0'><div id=p style='position:relative;z-index:0;height:30px;background:#ff0000'>"
                 "<div id=n style='position:relative;z-index:-1;height:10px;background:#00ff00'></div>"
                 "<div id=f style='margin-top:-5px;height:10px;background:#0000ff'></div></div>"
                 "<div id=q style='position:relative;z-index:1;margin-top:-30px;width:5px;height:5px;background:#ffff00'></div></body>", 320);
    TEST_ASSERT_EQUAL(0, l61_index(0xFF0000));
    TEST_ASSERT_EQUAL(1, l61_index(0x00FF00));
    TEST_ASSERT_EQUAL(2, l61_index(0x0000FF));
    TEST_ASSERT_EQUAL(3, l61_index(0xFFFF00));
    TEST_ASSERT_EQUAL(0x0000FF, (int)l61_paint(10, 7));
    TEST_ASSERT_EQUAL(0x00FF00, (int)l61_paint(10, 2));
    TEST_ASSERT_EQUAL(0xFFFF00, (int)l61_paint(2, 2));
}

static void test_l61_stack_negative_below_zero_sibling(void) {
    lot52_render("<body style='margin:0'><div id=a style='position:relative;z-index:0;height:10px;background:#ff0000'></div>"
                 "<div id=b style='position:relative;z-index:-2;margin-top:-5px;height:10px;background:#00ff00'></div>"
                 "<div id=c style='margin-top:-5px;height:10px;background:#0000ff'></div></body>", 320);
    TEST_ASSERT_EQUAL(0, l61_index(0x00FF00));   /* z -2 */
    TEST_ASSERT_EQUAL(1, l61_index(0x0000FF));   /* in-flow content */
    TEST_ASSERT_EQUAL(2, l61_index(0xFF0000));   /* z 0 */
    TEST_ASSERT_EQUAL(0xFF0000, (int)l61_paint(5, 7));
    TEST_ASSERT_EQUAL(0x0000FF, (int)l61_paint(5, 12));
}

static void test_l61_stack_negative_order_and_root_background(void) {
    int pass;
    lot52_render("<html><body style='margin:0;background:#00ff00'>"
                 "<div id=b style='position:relative;z-index:-1;height:10px;background:#0000ff'></div>"
                 "<div id=c style='position:relative;z-index:-3;margin-top:-10px;height:10px;background:#ffff00'></div>"
                 "<div id=f style='margin-top:-5px;height:10px;background:#ff0000'></div></body></html>", 320);
    for (pass = 0; pass < 2; pass++) {
        TEST_ASSERT_EQUAL(0, l61_index(0x00FF00));   /* body: root background */
        TEST_ASSERT_EQUAL(1, l61_index(0xFFFF00));   /* z -3 */
        TEST_ASSERT_EQUAL(2, l61_index(0x0000FF));   /* z -1 */
        TEST_ASSERT_EQUAL(3, l61_index(0xFF0000));   /* in-flow */
        TEST_ASSERT_EQUAL(0x0000FF, (int)l61_paint(5, 2));
        TEST_ASSERT_EQUAL(0xFF0000, (int)l61_paint(5, 7));
        TEST_ASSERT_EQUAL(0, we_layout(&g_doc, 320));
    }
}

static void test_l61_stack_negative_absolute_goes_below_flow(void) {
    /* No relative z-index: the negative absolute alone enables the pass. */
    lot52_render("<body style='margin:0'><div id=a style='position:absolute;z-index:-1;left:0;top:0;width:10px;height:10px;background:#0000ff'></div>"
                 "<div id=f style='height:10px;background:#ff0000'></div></body>", 320);
    TEST_ASSERT_TRUE(l61_index(0x0000FF) < l61_index(0xFF0000));
    TEST_ASSERT_EQUAL(0xFF0000, (int)l61_paint(5, 5));
    lot52_render("<body style='margin:0'><div id=a style='position:absolute;z-index:1;left:0;top:0;width:10px;height:10px;background:#0000ff'></div>"
                 "<div id=f style='height:10px;background:#ff0000'></div></body>", 320);
    TEST_ASSERT_TRUE(l61_index(0x0000FF) > l61_index(0xFF0000));
}

static void test_l61_stack_text_follows_its_context(void) {
    lot52_render("<body style='margin:0'><div id=a style='position:relative;z-index:1;height:10px'>A</div>"
                 "<div id=b style='margin-top:-10px;height:10px;background:#0000ff'></div></body>", 320);
    TEST_ASSERT_EQUAL(OS_FB_WEB_RECT, g_doc.items[0].kind);
    TEST_ASSERT_EQUAL(OS_FB_WEB_TEXT, g_doc.items[1].kind);
    TEST_ASSERT_EQUAL(0, g_doc.items[1].x);
    TEST_ASSERT_EQUAL(1, g_doc.items[1].y);
}

static void test_l61_stack_fixed_stays_last(void) {
    lot52_render("<body style='margin:0'><div id=f style='position:fixed;left:0;top:0;width:10px;height:10px;background:#0000ff'></div>"
                 "<div id=a style='position:relative;z-index:8;height:10px;background:#ff0000'></div></body>", 320);
    TEST_ASSERT_TRUE(l61_index(0x0000FF) > l61_index(0xFF0000));
    TEST_ASSERT_EQUAL(0x0000FF, (int)l61_paint(5, 5));
}

static void test_l61_stack_depth_is_bounded(void) {
    static char html[2048];
    int k, pos = 0;
    pos += snprintf(html + pos, sizeof(html) - (size_t)pos, "<body style='margin:0'>");
    for (k = 0; k < 9; k++)
        pos += snprintf(html + pos, sizeof(html) - (size_t)pos, "<div style='position:relative;z-index:%d;height:10px'>", k % 3);
    pos += snprintf(html + pos, sizeof(html) - (size_t)pos, "<div id=x style='height:5px;background:#ff0000'></div>");
    for (k = 0; k < 9; k++) pos += snprintf(html + pos, sizeof(html) - (size_t)pos, "</div>");
    pos += snprintf(html + pos, sizeof(html) - (size_t)pos, "<div id=y style='margin-top:-10px;height:10px;background:#0000ff'></div></body>");
    lot52_render(html, 320);
    TEST_ASSERT_TRUE(lot52_has_diag("stacking depth"));
    TEST_ASSERT_TRUE(l61_index(0xFF0000) > l61_index(0x0000FF));
    l61_box("x", 0, 0, 320, 5);
}

static void test_l61_stack_successive_layouts_are_stable(void) {
    int pass;
    lot52_render(L61_STACK_PAIR, 320);
    for (pass = 0; pass < 3; pass++) {
        TEST_ASSERT_EQUAL(0, we_layout(&g_doc, 200 + 40 * pass));
        TEST_ASSERT_EQUAL(0, l61_index(0x00FF00));
        TEST_ASSERT_EQUAL(1, l61_index(0x0000FF));
        TEST_ASSERT_EQUAL(2, l61_index(0xFF0000));
        TEST_ASSERT_EQUAL(200 + 40 * pass, l61_rect(0xFF0000)->w);
        TEST_ASSERT_EQUAL(0, g_doc.ndiag);
    }
}

static void lot61_stack_run(void) {
    RUN_TEST(test_l61_stack_relative_z_paints_over_later_flow);
    RUN_TEST(test_l61_stack_without_z_keeps_tree_order);
    RUN_TEST(test_l61_stack_context_groups_its_subtree);
    RUN_TEST(test_l61_stack_equal_z_keeps_tree_order);
    RUN_TEST(test_l61_stack_negative_child_inside_its_parent_context);
    RUN_TEST(test_l61_stack_negative_below_zero_sibling);
    RUN_TEST(test_l61_stack_negative_order_and_root_background);
    RUN_TEST(test_l61_stack_negative_absolute_goes_below_flow);
    RUN_TEST(test_l61_stack_text_follows_its_context);
    RUN_TEST(test_l61_stack_fixed_stays_last);
    RUN_TEST(test_l61_stack_depth_is_bounded);
    RUN_TEST(test_l61_stack_successive_layouts_are_stable);
}
