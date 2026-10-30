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

/* Lot 62: a positioned box with z-index auto paints after the in-flow
 * blocks (appendix E step 8); two static blocks keep tree order. */
static void test_l61_stack_without_z_keeps_tree_order(void) {
    lot52_render("<body style='margin:0'><div id=r style='position:relative;width:40px;height:20px;background:#ff0000'></div>"
                 "<div id=s style='margin-top:-10px;width:40px;height:20px;background:#0000ff'></div></body>", 320);
    TEST_ASSERT_TRUE(l61_index(0x0000FF) < l61_index(0xFF0000));
    TEST_ASSERT_EQUAL(0xFF0000, (int)l61_paint(5, 15));
    lot52_render("<body style='margin:0'><div id=r style='width:40px;height:20px;background:#ff0000'></div>"
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


/* opacity below 1 forms a z-index 0 stacking context (CSS Color 4 and
 * CSS 2.1 appendix E); opacity 0 removes the items. Lot 62: 0 < opacity < 1
 * is composited (red at alpha 128 over blue, then over the white canvas). */
static void test_l61_opacity_forms_a_context_over_later_flow(void) {
    lot52_render("<body style='margin:0'><div id=o style='opacity:0.5;height:10px;background:#ff0000'></div>"
                 "<div id=f style='margin-top:-5px;height:10px;background:#0000ff'></div></body>", 320);
    TEST_ASSERT_EQUAL(50, lot52_node("o")->transparency);
    TEST_ASSERT_TRUE(l61_index(0x0000FF) < l61_index(0x80007F));
    TEST_ASSERT_EQUAL(0x80007F, (int)l61_paint(5, 7));
    TEST_ASSERT_EQUAL(0xFF7F7F, (int)l61_paint(5, 2));
    TEST_ASSERT_EQUAL(0, l61_diag_count("opacity painted opaque"));
    /* opacity 1 (and above, clamped) is no context: tree order stays. */
    lot52_render("<body style='margin:0'><div id=o style='opacity:1.5;height:10px;background:#ff0000'></div>"
                 "<div id=f style='margin-top:-5px;height:10px;background:#0000ff'></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("o")->transparency);
    TEST_ASSERT_EQUAL(0x0000FF, (int)l61_paint(5, 7));
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l61_opacity_zero_removes_the_subtree_paint(void) {
    lot52_render("<body style='margin:0'><div id=o style='opacity:0;height:10px;background:#ff0000'>"
                 "<p id=c style='margin:0;background:#00ff00'>X</p></div><div id=f style='height:10px;background:#0000ff'></div></body>", 320);
    l61_box("o", 0, 0, 320, 10);                               /* layout is kept */
    l61_box("f", 0, 10, 320, 10);
    TEST_ASSERT_EQUAL(-1, l61_index(0xFF0000));
    TEST_ASSERT_EQUAL(-1, l61_index(0x00FF00));
    TEST_ASSERT_NULL(find_text("X"));
    TEST_ASSERT_EQUAL(1, (int)g_doc.nitems);
    TEST_ASSERT_EQUAL(0, l61_diag_count("opacity painted opaque"));
}

static void test_l61_opacity_with_z_index_and_static_z(void) {
    /* A positioned negative z context keeps its z with opacity. */
    lot52_render("<body style='margin:0'><div id=o style='opacity:.4;position:relative;z-index:-1;height:10px;background:#ff0000'></div>"
                 "<div id=f style='margin-top:-5px;height:10px;background:#0000ff'></div></body>", 320);
    TEST_ASSERT_EQUAL(60, lot52_node("o")->transparency);
    TEST_ASSERT_EQUAL(0x0000FF, (int)l61_paint(5, 7));
    /* z-index on a static box is ignored: the opacity context is z 0. */
    lot52_render("<body style='margin:0'><div id=o style='opacity:40%;z-index:-1;height:10px;background:#ff0000'></div>"
                 "<div id=f style='margin-top:-5px;height:10px;background:#0000ff'></div></body>", 320);
    TEST_ASSERT_EQUAL(60, lot52_node("o")->transparency);
    TEST_ASSERT_EQUAL(0x660099, (int)l61_paint(5, 7));          /* red at alpha 102 over blue */
}

static void test_l61_opacity_values_are_bounded(void) {
    lot52_render("<body><div id=a style='opacity:-2'></div><div id=b style='opacity:250%'></div>"
                 "<div id=c style='opacity:0.255'></div><div id=d style='opacity:half'></div>"
                 "<div id=e style='opacity:0.'></div><div id=f style='opacity:5px'></div></body>", 320);
    TEST_ASSERT_EQUAL(100, lot52_node("a")->transparency);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->transparency);
    TEST_ASSERT_EQUAL(75, lot52_node("c")->transparency);      /* two decimals kept */
    TEST_ASSERT_EQUAL(0, lot52_node("d")->transparency);
    TEST_ASSERT_EQUAL(0, lot52_node("e")->transparency);
    TEST_ASSERT_EQUAL(0, lot52_node("f")->transparency);
    TEST_ASSERT_EQUAL(3, l61_diag_count("opacity refused"));
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
    RUN_TEST(test_l61_opacity_forms_a_context_over_later_flow);
    RUN_TEST(test_l61_opacity_zero_removes_the_subtree_paint);
    RUN_TEST(test_l61_opacity_with_z_index_and_static_z);
    RUN_TEST(test_l61_opacity_values_are_bounded);
    RUN_TEST(test_l61_stack_text_follows_its_context);
    RUN_TEST(test_l61_stack_fixed_stays_last);
    RUN_TEST(test_l61_stack_depth_is_bounded);
    RUN_TEST(test_l61_stack_successive_layouts_are_stable);
}
