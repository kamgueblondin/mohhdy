/* Lot 64: further cases of the lot's branches (logical sides in vertical
 * rtl, radii and budgets in vertical flows, layer lists, column plans,
 * breaks, edge longhands). */

static void test_l64_vertical_logical_cases(void) {
    lot52_render(L64_BODY "<div style='writing-mode:vertical-lr;direction:rtl;height:30px'><div id=c style='width:6px;height:5px;"
                 "margin-inline-start:4px;border-inline-start:2px solid #ff0000'>a</div></div></body>", 320);
    l64_box("c", 0, 19, 6, 7);                                /* inline-start is the bottom */
    l61_rect_at(0, 24, 6, 2, 0xFF0000);
    lot52_render(L64_BODY "<div id=v style='writing-mode:vertical-lr;inline-size:20px;block-size:12px'>ab</div></body>", 320);
    l64_box("v", 0, 0, 12, 20);
    lot52_render(L64_BODY "<div id=v style='writing-mode:vertical-lr;height:20px;border-start-start-radius:3px;border-end-end-radius:5px'>"
                 "</div></body>", 320);
    TEST_ASSERT_EQUAL(3, lot52_node("v")->rad_x[0]);          /* left and top */
    TEST_ASSERT_EQUAL(0, lot52_node("v")->rad_x[3]);
    TEST_ASSERT_EQUAL(5, lot52_node("v")->rad_x[2]);          /* right and bottom */
    lot52_render(L64_BODY "<div style='writing-mode:vertical-rl;height:20px'><div id=c style='width:6px;height:6px;"
                 "border-top-left-radius:4px 2px;background:#ff0000'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(4, lot52_node("c")->rad_x[0]);          /* physical again after the layout */
    TEST_ASSERT_EQUAL(2, lot52_node("c")->rad_y[0]);
    l61_rect_at(1, 0, 5, 6, 0xFF0000);
    l61_rect_at(0, 1, 1, 5, 0xFF0000);
    lot52_render(L64_BODY "<div style='writing-mode:vertical-rl;height:64px'>abcdefgh abcdefgh abcdefgh abcdefgh abcdefgh abcdefgh abcdefgh abcdefgh abcdefgh abcdefgh abcdefgh abcdefgh abcdefgh abcdefgh</div></body>", 320);
    TEST_ASSERT_EQUAL(96, (int)g_doc.nitems);
    TEST_ASSERT_TRUE(g_doc.perf.items_dropped > 0);
    TEST_ASSERT_EQUAL(1, l61_diag_count("display budget exceeded"));
}

static void test_l64_layer_and_shorthand_cases(void) {
    /* four layers, two sizes: the fourth (lowest) takes the second size */
    l63_bgi("<body style='margin:0'><div style='width:8px;height:2px;background-image:url(/images/mire-g1.png),url(/images/mire-g1.png),"
            "url(/images/mire-g1.png),url(/images/mire-ga16.png);background-size:1px 1px,6px 2px;background-repeat:no-repeat'></div></body>");
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
    l61_rect_at(5, 0, 1, 2, 0xC8C8C8);
    TEST_ASSERT_EQUAL(-1, l63_at(6, 0));
    l63_bgi("<body style='margin:0'><div style='width:8px;height:2px;background:url(/images/mire-ga16.png) 1px no-repeat 2px'></div></body>");
    TEST_ASSERT_EQUAL(1, l61_diag_count("background refused"));   /* a split position */
    TEST_ASSERT_EQUAL(0, (int)g_doc.nitems);
}

static void test_l64_column_cases(void) {
    lot52_render(L64_BODY "<div id=w style='column-count:4;width:20px;column-gap:0'><div>a</div><div>b</div><div>c</div><div>d</div></div></body>", 320);
    l61_text_at("c", 10, 1);                                  /* 4 columns of 5 px: 2 of 10 */
    l61_text_at("d", 10, 11);
    lot52_render(L64_BODY "<div style='display:flex;column-count:2;width:40px'><div>a</div><div>b</div></div></body>", 320);
    l61_text_at("b", 20, 1);                                  /* flex items, no columns */
    lot52_render(L64_BODY "<div style='column-count:2;width:40px;height:10px'><div>a</div><div>b</div><div>c</div><div>d</div></div></body>", 320);
    TEST_ASSERT_EQUAL(1, l61_diag_count("columns overflow"));  /* capped at the height */
    l61_text_at("b", 24, 1);
    l61_text_at("d", 72, 1);
    lot52_render(L64_BODY "<div style='column-count:2;width:40px'><div style='break-before:column;break-before:auto'>a</div><div>b</div></div>"
                 "<div style='column-count:2;width:40px'><div>c</div><div style='break-before:column;break-before:avoid'>d</div><div>e</div></div></body>", 320);
    l61_text_at("b", 24, 1);
    l61_text_at("d", 0, 21);                                  /* avoid clears the forced break */
    l61_text_at("e", 24, 11);
    lot52_render(L64_BODY "<div style='column-count:2;width:40px;column-rule:solid #ff0000'><div>a</div><div>b</div></div>"
                 "<div id=x style='column-width:50%;column-width:30px'>x</div></body>", 320);
    l61_rect_at(19, 0, 3, 10, 0xFF0000);                      /* medium: 3 px */
    TEST_ASSERT_EQUAL(1, l61_diag_count("columns refused"));
    TEST_ASSERT_EQUAL(30, lot52_node("x")->col_width);
}

static void test_l64_edge_cases(void) {
    lot52_render(L64_BODY "<div id=a style='width:4px;height:4px;border-top-width:65px;border-top-width:12px;border-top-style:solid'></div></body>", 320);
    TEST_ASSERT_EQUAL(1, l61_diag_count("border edge refused"));
    l64_box("a", 0, 0, 4, 12);                                /* capped at 8 */
    l61_rect_at(0, 0, 4, 8, 0x000000);
    lot52_render(L64_BODY "<div id=a style='border-inline-width:3px;border-inline-width:2px wide;border-inline-style:solid'>x</div></body>", 320);
    TEST_ASSERT_EQUAL(1, l61_diag_count("border edge refused"));
    TEST_ASSERT_EQUAL(3, lot52_node("a")->edge_w[3]);         /* neither side changed */
    TEST_ASSERT_EQUAL(3, lot52_node("a")->edge_w[1]);
    lot52_render(L64_BODY "<div id=a style='width:10px;height:10px;border-left-width:3px;border-top:1px solid #ff0000'>x</div></body>", 320);
    l64_box("a", 0, 0, 10, 11);                               /* no style: no width */
    l61_rect_at(0, 0, 10, 1, 0xFF0000);
    l61_text_at("x", 0, 2);
}

static void lot64_cases_run(void) {
    RUN_TEST(test_l64_vertical_logical_cases);
    RUN_TEST(test_l64_layer_and_shorthand_cases);
    RUN_TEST(test_l64_column_cases);
    RUN_TEST(test_l64_edge_cases);
}
