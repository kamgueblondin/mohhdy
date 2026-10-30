/* Lot 56: absolute boxes, flex reverse/order/auto margins, rowspan,
 * border-spacing, and preserved whitespace. Each case locks a coordinate. */

static void test_l56_row_reverse_justify_matrix(void) {
    static const struct { const char *mode; int a, b, c; } cases[] = {
        {"start", 100, 80, 60}, {"flex-start", 100, 80, 60},
        {"end", 40, 20, 0}, {"flex-end", 40, 20, 0},
        {"center", 70, 50, 30},
        {"space-between", 100, 50, 0},
        {"space-around", 90, 50, 10},
        {"space-evenly", 85, 50, 15}
    };
    unsigned i;
    for (i = 0; i < sizeof(cases) / sizeof(cases[0]); i++) {
        char html[700];
        snprintf(html, sizeof(html),
            "<body style='margin:0'><div id=r style='display:flex;flex-direction:row-reverse;width:120px;justify-content:%s'>"
            "<div id=a style='width:20px;height:10px'></div>"
            "<div id=b style='width:20px;height:10px'></div>"
            "<div id=c style='width:20px;height:10px'></div></div></body>", cases[i].mode);
        lot52_render(html, 320);
        TEST_ASSERT_EQUAL(cases[i].a, lot52_node("a")->x);
        TEST_ASSERT_EQUAL(cases[i].b, lot52_node("b")->x);
        TEST_ASSERT_EQUAL(cases[i].c, lot52_node("c")->x);
        TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
        TEST_ASSERT_EQUAL(20, lot52_node("a")->w);
        TEST_ASSERT_EQUAL(10, lot52_node("r")->h);
        TEST_ASSERT_EQUAL(1, lot52_node("r")->flex_reverse);
        TEST_ASSERT_EQUAL(0, g_doc.ndiag);
    }
}

static void test_l56_column_reverse_justify_matrix(void) {
    static const struct { const char *mode; int a, b, c; } cases[] = {
        {"start", 100, 80, 60}, {"end", 40, 20, 0}, {"center", 70, 50, 30},
        {"space-between", 100, 50, 0}, {"space-around", 90, 50, 10}, {"space-evenly", 85, 50, 15}
    };
    unsigned i;
    for (i = 0; i < sizeof(cases) / sizeof(cases[0]); i++) {
        char html[700];
        snprintf(html, sizeof(html),
            "<body style='margin:0'><div id=r style='display:flex;flex-direction:column-reverse;width:40px;height:120px;justify-content:%s'>"
            "<div id=a style='width:20px;height:20px'></div>"
            "<div id=b style='width:20px;height:20px'></div>"
            "<div id=c style='width:20px;height:20px'></div></div></body>", cases[i].mode);
        lot52_render(html, 320);
        TEST_ASSERT_EQUAL(cases[i].a, lot52_node("a")->y);
        TEST_ASSERT_EQUAL(cases[i].b, lot52_node("b")->y);
        TEST_ASSERT_EQUAL(cases[i].c, lot52_node("c")->y);
        TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
        TEST_ASSERT_EQUAL(20, lot52_node("a")->w);
        TEST_ASSERT_EQUAL(20, lot52_node("a")->h);
        TEST_ASSERT_EQUAL(120, lot52_node("r")->h);
        TEST_ASSERT_EQUAL(1, lot52_node("r")->flex_dir);
        TEST_ASSERT_EQUAL(0, g_doc.ndiag);
    }
}

static void test_l56_reverse_without_justify_uses_main_start(void) {
    lot52_render("<body style='margin:0'><div id=r style='display:flex;flex-direction:row-reverse;width:120px'>"
        "<div id=a style='width:20px;height:10px'></div>"
        "<div id=b style='width:20px;height:10px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(100, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(80, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(10, lot52_node("r")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l56_reverse_gap_stays_between_items(void) {
    lot52_render("<body style='margin:0'><div id=r style='display:flex;flex-direction:row-reverse;width:120px;gap:10px'>"
        "<div id=a style='width:20px;height:10px'></div>"
        "<div id=b style='width:20px;height:10px'></div>"
        "<div id=c style='width:20px;height:10px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(100, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(70, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(40, lot52_node("c")->x);
    TEST_ASSERT_EQUAL(20, lot52_node("c")->w);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l56_reverse_does_not_change_cross_axis(void) {
    lot52_render("<body style='margin:0'><div id=r style='display:flex;flex-direction:row-reverse;width:80px;height:40px;align-items:center'>"
        "<div id=a style='width:20px;height:10px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(60, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(15, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("r")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l56_invalid_direction_keeps_reverse(void) {
    lot52_render("<body style='margin:0'><div id=r style='display:flex;flex-direction:row-reverse;flex-direction:sideways;width:80px'>"
        "<div id=a style='width:20px;height:10px'></div>"
        "<div id=b style='width:20px;height:10px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(1, lot52_node("r")->flex_reverse);
    TEST_ASSERT_EQUAL(0, lot52_node("r")->flex_dir);
    TEST_ASSERT_EQUAL(60, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(40, lot52_node("b")->x);
    TEST_ASSERT_TRUE(lot52_has_diag("flex-direction refused"));
}

static void test_l56_column_then_row_reverse_clears_column(void) {
    lot52_render("<body style='margin:0'><div id=r style='display:flex;flex-direction:column;flex-direction:row-reverse;width:80px'>"
        "<div id=a style='width:20px;height:10px'></div>"
        "<div id=b style='width:20px;height:10px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("r")->flex_dir);
    TEST_ASSERT_EQUAL(1, lot52_node("r")->flex_reverse);
    TEST_ASSERT_EQUAL(60, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l56_order_permutes_flex_items(void) {
    static const struct { int oa, ob, oc, ax, bx, cx; } cases[] = {
        {0, 0, 0, 0, 20, 40},
        {1, 0, -1, 40, 20, 0},
        {2, 2, 1, 20, 40, 0},
        {-8, 8, 0, 0, 40, 20},
        {8, -8, 0, 40, 0, 20}
    };
    unsigned i;
    for (i = 0; i < sizeof(cases) / sizeof(cases[0]); i++) {
        char html[500];
        snprintf(html, sizeof(html),
            "<body style='margin:0'><div style='display:flex;justify-content:flex-start;width:120px'>"
            "<div id=a style='order:%d;width:20px;height:10px'></div>"
            "<div id=b style='order:%d;width:20px;height:10px'></div>"
            "<div id=c style='order:%d;width:20px;height:10px'></div></div></body>",
            cases[i].oa, cases[i].ob, cases[i].oc);
        lot52_render(html, 320);
        TEST_ASSERT_EQUAL(cases[i].ax, lot52_node("a")->x);
        TEST_ASSERT_EQUAL(cases[i].bx, lot52_node("b")->x);
        TEST_ASSERT_EQUAL(cases[i].cx, lot52_node("c")->x);
        TEST_ASSERT_EQUAL((int8_t)cases[i].oa, lot52_node("a")->item_order);
        TEST_ASSERT_EQUAL(0, g_doc.ndiag);
    }
}

static void test_l56_order_tie_keeps_source_order(void) {
    lot52_render("<body style='margin:0'><div style='display:flex;justify-content:flex-start;width:90px'>"
        "<div id=a style='order:1;width:10px;height:10px'></div>"
        "<div id=b style='order:1;width:10px;height:10px'></div>"
        "<div id=c style='order:1;width:10px;height:10px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(20, lot52_node("c")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l56_order_applies_to_grid_auto_placement(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:90px;grid-template-columns:30px 30px 30px'>"
        "<div id=a style='order:1;height:10px'></div>"
        "<div id=b style='order:-1;height:10px'></div>"
        "<div id=c style='height:10px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(30, lot52_node("c")->x);
    TEST_ASSERT_EQUAL(60, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(10, lot52_node("g")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l56_order_out_of_range_keeps_previous(void) {
    static const char *bad[] = {"9", "-9", "1.5", "x"};
    unsigned i;
    for (i = 0; i < sizeof(bad) / sizeof(bad[0]); i++) {
        char html[400];
        snprintf(html, sizeof(html),
            "<body style='margin:0'><div style='display:flex;justify-content:flex-start;width:80px'>"
            "<div id=a style='order:2;order:%s;width:20px;height:10px'></div>"
            "<div id=b style='width:20px;height:10px'></div></div></body>", bad[i]);
        lot52_render(html, 320);
        TEST_ASSERT_EQUAL(2, lot52_node("a")->item_order);
        TEST_ASSERT_EQUAL(20, lot52_node("a")->x);
        TEST_ASSERT_EQUAL(0, lot52_node("b")->x);
        TEST_ASSERT_TRUE(lot52_has_diag("order refused"));
    }
}

static void test_l56_order_bounds_are_accepted(void) {
    lot52_render("<body style='margin:0'><div style='display:flex;justify-content:flex-start;width:80px'>"
        "<div id=a style='order:-8;width:20px;height:10px'></div>"
        "<div id=b style='order:8;width:20px;height:10px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(-8, lot52_node("a")->item_order);
    TEST_ASSERT_EQUAL(8, lot52_node("b")->item_order);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(20, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l56_reverse_and_order_together(void) {
    lot52_render("<body style='margin:0'><div id=r style='display:flex;flex-direction:row-reverse;width:60px'>"
        "<div id=a style='order:1;width:20px;height:10px'></div>"
        "<div id=b style='order:0;width:20px;height:10px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(40, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(10, lot52_node("r")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l56_column_reverse_and_order(void) {
    lot52_render("<body style='margin:0'><div id=r style='display:flex;flex-direction:column-reverse;width:30px;height:40px'>"
        "<div id=a style='order:1;height:10px'></div>"
        "<div id=b style='order:-1;height:10px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(30, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(40, lot52_node("r")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l56_margin_auto_centers_one_item(void) {
    lot52_render("<body style='margin:0'><div id=r style='display:flex;width:120px'>"
        "<div id=a style='width:20px;height:10px;margin-left:auto;margin-right:auto'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(50, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("r")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l56_margin_shorthand_auto_centers(void) {
    lot52_render("<body style='margin:0'><div style='display:flex;width:100px'>"
        "<div id=a style='width:20px;height:10px;margin:0 auto'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(40, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(WE_AUTO_LEFT | WE_AUTO_RIGHT, lot52_node("a")->margin_auto);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l56_margin_left_auto_pushes_to_the_end(void) {
    lot52_render("<body style='margin:0'><div style='display:flex;width:100px'>"
        "<div id=a style='width:20px;height:10px;margin-left:auto'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(80, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
}

static void test_l56_margin_right_auto_stays_at_the_start(void) {
    lot52_render("<body style='margin:0'><div style='display:flex;width:100px'>"
        "<div id=a style='width:20px;height:10px;margin-right:auto'></div>"
        "<div id=b style='width:20px;height:10px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(80, lot52_node("b")->x);
}

static void test_l56_two_margin_autos_share_free_space(void) {
    lot52_render("<body style='margin:0'><div style='display:flex;width:100px'>"
        "<div id=a style='width:20px;height:10px;margin-left:auto'></div>"
        "<div id=b style='width:20px;height:10px;margin-left:auto'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(80, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(100, lot52_node("b")->x + lot52_node("b")->w);
}

static void test_l56_margin_auto_remainder_goes_to_the_first(void) {
    lot52_render("<body style='margin:0'><div style='display:flex;width:101px'>"
        "<div id=a style='width:20px;height:10px;margin-left:auto;margin-right:auto'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(41, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(101, lot52_node("a")->x + lot52_node("a")->w + 40);
}

static void test_l56_margin_auto_with_gap(void) {
    lot52_render("<body style='margin:0'><div style='display:flex;width:100px;gap:10px'>"
        "<div id=a style='width:20px;height:10px;margin-left:auto'></div>"
        "<div id=b style='width:20px;height:10px;margin-left:auto'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(25, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(80, lot52_node("b")->x);
}

static void test_l56_margin_auto_disables_justify(void) {
    lot52_render("<body style='margin:0'><div style='display:flex;width:100px;justify-content:center'>"
        "<div id=a style='width:20px;height:10px;margin-left:auto'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(80, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l56_column_margin_auto_centers(void) {
    lot52_render("<body style='margin:0'><div id=r style='display:flex;flex-direction:column;width:40px;height:100px'>"
        "<div id=a style='height:20px;margin-top:auto;margin-bottom:auto'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(40, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(100, lot52_node("r")->h);
    TEST_ASSERT_EQUAL(40, lot52_node("a")->w);
}

static void test_l56_column_margin_remainder(void) {
    lot52_render("<body style='margin:0'><div style='display:flex;flex-direction:column;height:101px;width:20px'>"
        "<div id=a style='height:20px;margin-top:auto;margin-bottom:auto'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(41, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->h);
}

static void test_l56_reverse_margin_left_auto_stays_physical(void) {
    lot52_render("<body style='margin:0'><div style='display:flex;flex-direction:row-reverse;width:120px'>"
        "<div id=a style='width:20px;height:10px;margin-left:auto'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(100, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->w);
}

static void test_l56_reverse_margin_right_auto_pushes_left(void) {
    lot52_render("<body style='margin:0'><div style='display:flex;flex-direction:row-reverse;width:120px'>"
        "<div id=a style='width:20px;height:10px;margin-right:auto'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
}

static void test_l56_reverse_both_margins_auto_center(void) {
    lot52_render("<body style='margin:0'><div style='display:flex;flex-direction:row-reverse;width:120px'>"
        "<div id=a style='width:20px;height:10px;margin-left:auto;margin-right:auto'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(50, lot52_node("a")->x);
}

static void test_l56_auto_margin_length_is_zero(void) {
    lot52_render("<body style='margin:0'><div style='display:flex;width:80px'>"
        "<div id=a style='width:20px;height:10px;margin-left:4px;margin-left:auto'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->margin[3]);
    TEST_ASSERT_EQUAL(WE_AUTO_LEFT, lot52_node("a")->margin_auto);
    TEST_ASSERT_EQUAL(60, lot52_node("a")->x);
}

static void test_l56_fixed_margin_replaces_auto(void) {
    lot52_render("<body style='margin:0'><div style='display:flex;width:80px'>"
        "<div id=a style='width:20px;height:10px;margin-left:auto;margin-left:4px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->margin_auto & WE_AUTO_LEFT);
    TEST_ASSERT_EQUAL(4, lot52_node("a")->margin[3]);
    TEST_ASSERT_EQUAL(4, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l56_absolute_left_top_on_initial_block(void) {
    lot52_render("<body style='margin:0'><div id=a style='position:absolute;left:20px;top:10px;width:40px;height:16px'></div></body>", 320);
    TEST_ASSERT_EQUAL(1, lot52_node("a")->absolute);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(16, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(0, g_doc.nodes[0].h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l56_absolute_right_and_bottom(void) {
    lot52_render("<body style='margin:0'><div id=p style='position:relative;width:100px;height:40px'>"
        "<div id=a style='position:absolute;right:10px;bottom:5px;width:30px;height:15px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(60, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(15, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(40, lot52_node("p")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l56_absolute_stretch_both_edges(void) {
    lot52_render("<body style='margin:0'><div id=p style='position:relative;width:100px;height:40px'>"
        "<div id=a style='position:absolute;left:10px;right:15px;top:4px;bottom:6px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(4, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(75, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(100, lot52_node("p")->w);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l56_absolute_specified_width_ignores_extra_right(void) {
    lot52_render("<body style='margin:0'><div id=p style='position:relative;width:100px;height:20px'>"
        "<div id=a style='position:absolute;left:10px;right:10px;width:30px;height:8px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(8, lot52_node("a")->h);
}

static void test_l56_absolute_nested_relative_containing_block(void) {
    lot52_render("<body style='margin:0'><div id=o style='position:relative;padding:8px;width:100px;height:80px'>"
        "<div id=i style='position:relative;margin:10px;width:40px;height:30px'>"
        "<div id=a style='position:absolute;left:3px;top:4px;width:8px;height:6px'></div>"
        "</div></div></body>", 320);
    TEST_ASSERT_EQUAL(18, lot52_node("i")->x);
    TEST_ASSERT_EQUAL(18, lot52_node("i")->y);
    TEST_ASSERT_EQUAL(21, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(22, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(8, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(6, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l56_absolute_inside_absolute(void) {
    lot52_render("<body style='margin:0'><div id=o style='position:absolute;left:10px;top:10px;width:80px;height:60px'>"
        "<div id=a style='position:absolute;right:5px;bottom:5px;width:20px;height:10px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(10, lot52_node("o")->x);
    TEST_ASSERT_EQUAL(10, lot52_node("o")->y);
    TEST_ASSERT_EQUAL(65, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(55, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(0, g_doc.nodes[0].h);
}

static void test_l56_absolute_does_not_grow_parent(void) {
    lot52_render("<body style='margin:0'><div id=p style='position:relative;width:80px'>"
        "<div id=a style='position:absolute;left:0;top:30px;width:10px;height:40px'></div>"
        "<div id=b style='height:12px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(12, lot52_node("p")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(12, lot52_node("b")->h);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("a")->h);
}

static void test_l56_absolute_skipped_by_flex_min_content(void) {
    lot52_render("<body style='margin:0'><div id=r style='display:flex;width:min-content'>"
        "<div id=a style='position:absolute;width:80px;height:10px'></div>"
        "<div id=b style='width:20px;height:10px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(20, lot52_node("r")->w);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(10, lot52_node("r")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
}

static void test_l56_relative_offset_moves_absolute_with_parent(void) {
    lot52_render("<body style='margin:0'><div style='height:80px'><div id=p style='position:relative;left:5px;top:4px;width:100px;height:40px'>"
        "<div id=a style='position:absolute;left:10px;top:6px;width:12px;height:8px'></div></div></div></body>", 320);
    TEST_ASSERT_EQUAL(5, lot52_node("p")->x);
    TEST_ASSERT_EQUAL(4, lot52_node("p")->y);
    TEST_ASSERT_EQUAL(15, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(12, lot52_node("a")->w);
}

static void test_l56_absolute_percentage_width_uses_padding_edge(void) {
    lot52_render("<body style='margin:0'><div id=p style='position:relative;width:100px;height:30px;padding:10px'>"
        "<div id=a style='position:absolute;left:0;top:0;width:50%;height:10px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(60, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(120, lot52_node("p")->w);
}

static void test_l56_absolute_no_insets_use_padding_origin(void) {
    lot52_render("<body style='margin:0'><div id=p style='position:relative;width:80px;height:30px'>"
        "<div id=a style='position:absolute;width:16px;height:8px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(16, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(8, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(30, lot52_node("p")->h);
}

static void test_l56_static_clears_absolute(void) {
    lot52_render("<body style='margin:0'><div id=a style='position:absolute;position:static;width:20px;height:10px'></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->absolute);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->relative);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

/* Lot 61: position:fixed became valid; the refusal uses an invalid keyword. */
static void test_l56_invalid_position_is_refused_and_keeps_absolute(void) {
    lot52_render("<body style='margin:0'><div id=a style='position:absolute;left:4px;top:6px;width:10px;height:10px;position:floating'></div></body>", 320);
    TEST_ASSERT_EQUAL(1, lot52_node("a")->absolute);
    TEST_ASSERT_EQUAL(4, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(6, lot52_node("a")->y);
    TEST_ASSERT_TRUE(lot52_has_diag("position refused"));
}

static void test_l56_invalid_offset_keeps_previous(void) {
    lot52_render("<body style='margin:0'><div id=p style='position:relative;width:80px;height:20px'>"
        "<div id=a style='position:absolute;left:12px;left:anchor(foo);width:10px;height:10px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(12, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(WE_INSET_LEFT, lot52_node("a")->inset_flags & WE_INSET_LEFT);
    TEST_ASSERT_TRUE(lot52_has_diag("position offset refused"));
}

static void test_l56_pre_keeps_newline_and_spaces(void) {
    lot52_render("<body style='margin:0'><pre id=p>a  b\ncd</pre></body>", 320);
    TEST_ASSERT_EQUAL(WE_WS_PRE, lot52_node("p")->ws_mode);
    TEST_ASSERT_EQUAL(1, lot52_node("p")->nowrap);
    TEST_ASSERT_EQUAL(20, lot52_node("p")->h);
    TEST_ASSERT_NOT_NULL(find_text("a  b"));
    TEST_ASSERT_NOT_NULL(find_text("cd"));
    TEST_ASSERT_EQUAL(1, find_text("a  b")->y);
    TEST_ASSERT_EQUAL(11, find_text("cd")->y);
    TEST_ASSERT_EQUAL(32, find_text("a  b")->w);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l56_normal_collapses_newline_to_one_space(void) {
    lot52_render("<body style='margin:0'><div id=p>a\nb</div></body>", 320);
    TEST_ASSERT_EQUAL(10, lot52_node("p")->h);
    TEST_ASSERT_NOT_NULL(find_text("a b"));
    TEST_ASSERT_EQUAL(24, find_text("a b")->w);
    TEST_ASSERT_EQUAL(1, g_doc.perf.lines);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l56_pre_newline_between_spans_is_a_line(void) {
    lot52_render("<body style='margin:0'><pre id=p><span>a</span>\n<span>b</span></pre></body>", 320);
    TEST_ASSERT_EQUAL(20, lot52_node("p")->h);
    TEST_ASSERT_EQUAL(1, find_text("a")->y);
    TEST_ASSERT_EQUAL(11, find_text("b")->y);
    TEST_ASSERT_EQUAL(2, g_doc.perf.lines);
}

static void test_l56_pre_wrap_breaks_the_long_line(void) {
    lot52_render("<body style='margin:0'><div id=p style='white-space:pre-wrap;width:32px'>abc def</div></body>", 320);
    TEST_ASSERT_EQUAL(WE_WS_PRE_WRAP, lot52_node("p")->ws_mode);
    TEST_ASSERT_EQUAL(0, lot52_node("p")->nowrap);
    TEST_ASSERT_EQUAL(20, lot52_node("p")->h);
    TEST_ASSERT_NOT_NULL(find_text("abc "));
    TEST_ASSERT_EQUAL(1, find_text("abc ")->y);
    TEST_ASSERT_EQUAL(32, find_text("abc ")->w);
    TEST_ASSERT_EQUAL(11, find_text("def")->y);
    TEST_ASSERT_EQUAL(0, find_text("def")->x);
}

static void test_l56_pre_does_not_wrap(void) {
    lot52_render("<body style='margin:0'><div id=p style='white-space:pre;width:24px'>abc def</div></body>", 320);
    TEST_ASSERT_EQUAL(10, lot52_node("p")->h);
    TEST_ASSERT_NOT_NULL(find_text("abc def"));
    TEST_ASSERT_EQUAL(56, find_text("abc def")->w);
    TEST_ASSERT_EQUAL(1, g_doc.perf.lines);
}

static void test_l56_pre_wrap_min_content_is_the_longest_word(void) {
    lot52_render("<body style='margin:0'><div id=p style='white-space:pre-wrap;width:min-content'>abc def</div></body>", 320);
    TEST_ASSERT_EQUAL(24, lot52_node("p")->w);
    TEST_ASSERT_EQUAL(30, lot52_node("p")->h);
    TEST_ASSERT_EQUAL(24, lot52_node("p")->min_content);
    TEST_ASSERT_EQUAL(3, g_doc.perf.lines);
}

static void test_l56_pre_min_content_keeps_the_whole_line(void) {
    lot52_render("<body style='margin:0'><div id=p style='white-space:pre;width:min-content'>abc def</div></body>", 320);
    TEST_ASSERT_EQUAL(56, lot52_node("p")->w);
    TEST_ASSERT_EQUAL(56, lot52_node("p")->min_content);
    TEST_ASSERT_EQUAL(10, lot52_node("p")->h);
}

static void test_l56_pre_line_breaks_on_newline_and_collapses_spaces(void) {
    lot52_render("<body style='margin:0'><div id=p style='white-space:pre-line;width:80px'>a  b\ncd</div></body>", 320);
    TEST_ASSERT_EQUAL(WE_WS_PRE_LINE, lot52_node("p")->ws_mode);
    TEST_ASSERT_EQUAL(0, lot52_node("p")->nowrap);
    TEST_ASSERT_EQUAL(20, lot52_node("p")->h);
    TEST_ASSERT_NOT_NULL(find_text("a b"));
    TEST_ASSERT_EQUAL(1, find_text("a b")->y);
    TEST_ASSERT_EQUAL(24, find_text("a b")->w);
    TEST_ASSERT_NOT_NULL(find_text("cd"));
    TEST_ASSERT_EQUAL(11, find_text("cd")->y);
    TEST_ASSERT_EQUAL(0, find_text("cd")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l56_normal_resets_pre(void) {
    lot52_render("<body style='margin:0'><div id=p style='white-space:pre;white-space:normal;width:24px'>abc def</div></body>", 320);
    TEST_ASSERT_EQUAL(WE_WS_NORMAL, lot52_node("p")->ws_mode);
    TEST_ASSERT_EQUAL(0, lot52_node("p")->nowrap);
    TEST_ASSERT_EQUAL(20, lot52_node("p")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l56_tab_in_pre_is_one_space(void) {
    lot52_render("<body style='margin:0'><pre id=p>a\tb</pre></body>", 320);
    TEST_ASSERT_NOT_NULL(find_text("a b"));
    TEST_ASSERT_EQUAL(24, find_text("a b")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("p")->h);
}

static void test_l56_rowspan_two_blocks_the_column(void) {
    lot52_render("<body style='margin:0'><table id=t style='width:80px'><tr>"
        "<td id=a rowspan=2>A</td><td id=b>B</td></tr><tr><td id=c>C</td></tr></table></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(40, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(40, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(40, lot52_node("c")->x);
    TEST_ASSERT_EQUAL(10, lot52_node("c")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("t")->h);
    TEST_ASSERT_EQUAL(2, g_doc.perf.table_rows);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l56_rowspan_three_covers_each_row(void) {
    lot52_render("<body style='margin:0'><table id=t style='width:80px'><tr>"
        "<td id=a rowspan=3>A</td><td id=b>B</td></tr>"
        "<tr><td id=c>C</td></tr><tr><td id=d>D</td></tr></table></body>", 320);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(40, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(40, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(40, lot52_node("c")->x);
    TEST_ASSERT_EQUAL(10, lot52_node("c")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("d")->x);
    TEST_ASSERT_EQUAL(20, lot52_node("d")->y);
    TEST_ASSERT_EQUAL(30, lot52_node("t")->h);
    TEST_ASSERT_EQUAL(3, g_doc.perf.table_rows);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l56_rowspan_and_colspan_leave_the_corner(void) {
    lot52_render("<body style='margin:0'><table id=t style='width:90px'><tr>"
        "<td id=a colspan=2 rowspan=2>A</td><td id=b>B</td></tr>"
        "<tr><td id=c>C</td></tr>"
        "<tr><td id=d>D</td><td id=e>E</td><td id=f>F</td></tr></table></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(60, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(60, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(60, lot52_node("c")->x);
    TEST_ASSERT_EQUAL(10, lot52_node("c")->y);
    TEST_ASSERT_EQUAL(0, lot52_node("d")->x);
    TEST_ASSERT_EQUAL(20, lot52_node("d")->y);
    TEST_ASSERT_EQUAL(30, lot52_node("e")->x);
    TEST_ASSERT_EQUAL(60, lot52_node("f")->x);
    TEST_ASSERT_EQUAL(30, lot52_node("t")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l56_rowspan_taller_than_the_rows_grows_the_last(void) {
    lot52_render("<body style='margin:0'><table id=t style='width:80px'><tr>"
        "<td id=a rowspan=2 style='height:25px'>A</td><td id=s style='height:10px'></td></tr>"
        "<tr><td id=b style='height:10px'>B</td></tr></table></body>", 320);
    TEST_ASSERT_EQUAL(25, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(40, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(25, lot52_node("t")->h);
    TEST_ASSERT_EQUAL(40, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l56_rowspan_past_the_table_is_clamped(void) {
    lot52_render("<body style='margin:0'><table id=t style='width:80px'><tr>"
        "<td id=a rowspan=4>A</td><td id=b>B</td></tr></table></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->span_refused);
    TEST_ASSERT_EQUAL(40, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(40, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(10, lot52_node("t")->h);
    TEST_ASSERT_EQUAL(1, g_doc.perf.table_rows);
    TEST_ASSERT_TRUE(lot52_has_diag("table rowspan clamped"));
}

static void test_l56_rowspan_outside_one_to_eight_refuses_the_table(void) {
    static const char *bad[] = {"0", "9", "-1", "x"};
    unsigned i;
    for (i = 0; i < sizeof(bad) / sizeof(bad[0]); i++) {
        char html[400];
        snprintf(html, sizeof(html),
            "<body style='margin:0'><table id=t style='width:80px'><tr>"
            "<td id=a rowspan='%s'>A</td><td id=b>B</td></tr></table></body>", bad[i]);
        lot52_render(html, 320);
        TEST_ASSERT_EQUAL(1, lot52_node("a")->span_refused);
        TEST_ASSERT_EQUAL(0, lot52_node("t")->h);
        TEST_ASSERT_EQUAL(0, lot52_node("a")->w);
        TEST_ASSERT_EQUAL(0, g_doc.perf.table_rows);
        TEST_ASSERT_NULL(find_text("A"));
        TEST_ASSERT_TRUE(lot52_has_diag("table span refused"));
        TEST_ASSERT_TRUE(lot52_has_diag("table refused after span"));
    }
}

static void test_l56_border_spacing_separates_cells(void) {
    lot52_render("<body style='margin:0'><table id=t style='width:70px;border-spacing:10px'><tr>"
        "<td id=a>A</td><td id=b>B</td></tr><tr><td id=c>C</td><td id=d>D</td></tr></table></body>", 320);
    TEST_ASSERT_EQUAL(10, lot52_node("t")->border_spacing);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(40, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(30, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(0, lot52_node("c")->x);
    TEST_ASSERT_EQUAL(20, lot52_node("c")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("d")->x);
    TEST_ASSERT_EQUAL(30, lot52_node("t")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l56_border_collapse_drops_spacing(void) {
    lot52_render("<body style='margin:0'><table id=t style='width:70px;border-spacing:10px;border-collapse:collapse'><tr>"
        "<td id=a>A</td><td id=b>B</td></tr></table></body>", 320);
    TEST_ASSERT_EQUAL(1, lot52_node("t")->border_collapse);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(35, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(35, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(35, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("t")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l56_border_spacing_px_and_refusal(void) {
    lot52_render("<body style='margin:0'><table id=t style='width:70px;border-spacing:8px;border-spacing:99px'><tr>"
        "<td id=a>A</td><td id=b>B</td></tr></table></body>", 320);
    TEST_ASSERT_EQUAL(8, lot52_node("t")->border_spacing);
    TEST_ASSERT_EQUAL(39, lot52_node("b")->x);
    TEST_ASSERT_TRUE(lot52_has_diag("border spacing refused"));
}

static void test_l56_border_collapse_invalid_keeps_separate(void) {
    lot52_render("<body style='margin:0'><table id=t style='width:70px;border-spacing:10px;border-collapse:fold'><tr>"
        "<td id=a>A</td><td id=b>B</td></tr></table></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("t")->border_collapse);
    TEST_ASSERT_EQUAL(40, lot52_node("b")->x);
    TEST_ASSERT_TRUE(lot52_has_diag("border collapse refused"));
}

static void test_l56_spacing_with_rowspan(void) {
    lot52_render("<body style='margin:0'><table id=t style='width:70px;border-spacing:10px'><tr>"
        "<td id=a rowspan=2>A</td><td id=b>B</td></tr><tr><td id=c>C</td></tr></table></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(40, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(40, lot52_node("c")->x);
    TEST_ASSERT_EQUAL(20, lot52_node("c")->y);
    TEST_ASSERT_EQUAL(30, lot52_node("t")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l56_guest_page_boxes(void) {
    static const char *paths[] = {
        "initrd_content/web/lot56.html",
        "../initrd_content/web/lot56.html",
        "tests/../initrd_content/web/lot56.html"
    };
    FILE *f = 0;
    char html[4000];
    int n = 0, i;
    for (i = 0; i < 3 && !f; i++) f = fopen(paths[i], "rb");
    TEST_ASSERT_NOT_NULL(f);
    n = (int)fread(html, 1, sizeof(html) - 1, f);
    fclose(f);
    html[n] = 0;
    lot52_render(html, 640);
    TEST_ASSERT_EQUAL(80, lot52_node("ra")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("ra")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("ra")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("ra")->h);
    TEST_ASSERT_EQUAL(100, lot52_node("rb")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("rb")->y);
    TEST_ASSERT_EQUAL(0, lot52_node("host")->x);
    TEST_ASSERT_EQUAL(10, lot52_node("host")->y);
    TEST_ASSERT_EQUAL(100, lot52_node("host")->w);
    TEST_ASSERT_EQUAL(40, lot52_node("host")->h);
    TEST_ASSERT_EQUAL(10, lot52_node("abs")->x);
    TEST_ASSERT_EQUAL(18, lot52_node("abs")->y);
    TEST_ASSERT_EQUAL(30, lot52_node("abs")->w);
    TEST_ASSERT_EQUAL(12, lot52_node("abs")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("c1")->x);
    TEST_ASSERT_EQUAL(50, lot52_node("c1")->y);
    TEST_ASSERT_EQUAL(36, lot52_node("c1")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("c1")->h);
    TEST_ASSERT_EQUAL(44, lot52_node("c2")->x);
    TEST_ASSERT_EQUAL(50, lot52_node("c2")->y);
    TEST_ASSERT_EQUAL(36, lot52_node("c2")->w);
    TEST_ASSERT_EQUAL(0, lot52_node("note")->x);
    TEST_ASSERT_EQUAL(60, lot52_node("note")->y);
    TEST_ASSERT_EQUAL(640, lot52_node("note")->w);
    TEST_ASSERT_EQUAL(20, lot52_node("note")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("wa")->x);
    TEST_ASSERT_EQUAL(100, lot52_node("wa")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("wa")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("wa")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("fl")->x);
    TEST_ASSERT_EQUAL(110, lot52_node("fl")->y);
    TEST_ASSERT_EQUAL(24, lot52_node("fl")->w);
    TEST_ASSERT_EQUAL(16, lot52_node("fl")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("cleared")->x);
    TEST_ASSERT_EQUAL(126, lot52_node("cleared")->y);
    TEST_ASSERT_EQUAL(16, lot52_node("cleared")->w);
    TEST_ASSERT_EQUAL(8, lot52_node("cleared")->h);
    TEST_ASSERT_EQUAL(134, lot52_node("lines")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("lines")->h);
    TEST_ASSERT_EQUAL(174, g_doc.perf.page_h);
    TEST_ASSERT_NOT_NULL(find_text("ab"));
    TEST_ASSERT_NOT_NULL(find_text("cd"));
    TEST_ASSERT_EQUAL(61, find_text("ab")->y);
    TEST_ASSERT_EQUAL(71, find_text("cd")->y);
    TEST_ASSERT_EQUAL(135, find_text("hi")->y);
    TEST_ASSERT_EQUAL(155, find_text("yo")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}
