/* Lot 64 round 2: per-side borders on boxes (outside collapsed tables),
 * the physical longhands, and the logical border properties. */

static void test_l64_edges_physical(void) {
    lot52_render(L64_BODY "<div id=a style='width:20px;height:10px;border-left:2px solid #ff0000;border-bottom-width:3px;border-bottom-style:dashed;"
                 "border-bottom-color:#00ff00;border-top-width:1px;border-top-style:solid;border-top-color:#0000ff'>x</div><div id=n>y</div></body>", 320);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
    l64_box("a", 0, 0, 22, 14);                               /* each side takes its own width */
    l61_text_at("x", 2, 2);
    l61_rect_at(0, 0, 22, 1, 0x0000FF);                       /* top and bottom across the box */
    l61_rect_at(0, 11, 6, 3, 0x00FF00);
    l61_rect_at(12, 11, 6, 3, 0x00FF00);
    l61_rect_at(0, 1, 2, 10, 0xFF0000);                       /* the left between them */
    TEST_ASSERT_EQUAL(0, l61_count(OS_FB_WEB_BORDER));
    l61_text_at("y", 0, 15);
    TEST_ASSERT_EQUAL(1, lot52_node("a")->own_edges);
    /* lists with two values per axis */
    lot52_render(L64_BODY "<div id=a style='width:10px;height:10px;border-style:solid dotted;border-width:2px 4px;border-color:#ff0000 #00ff00'></div></body>", 320);
    l64_box("a", 0, 0, 18, 14);
    l61_rect_at(0, 0, 18, 2, 0xFF0000);
    l61_rect_at(0, 12, 18, 2, 0xFF0000);
    l61_rect_at(0, 2, 4, 4, 0x00FF00);                        /* 4 px dots between the horizontal sides */
    l61_rect_at(14, 10, 4, 2, 0x00FF00);
    /* a style without a width takes the uniform width; none and hidden paint nothing */
    lot52_render(L64_BODY "<div id=a style='width:10px;height:10px;border:2px solid #ff0000;border-right-style:none;border-left-style:hidden;border-top-color:#0000ff'></div></body>", 320);
    l64_box("a", 0, 0, 10, 14);
    l61_rect_at(0, 0, 10, 2, 0x0000FF);
    l61_rect_at(0, 12, 10, 2, 0xFF0000);
    TEST_ASSERT_EQUAL(2, (int)g_doc.nitems);
    /* the uniform shorthand resets the sides; refused values change nothing */
    lot52_render(L64_BODY "<div id=a style='border-top:4px solid #0000ff;border:1px solid #ff0000;border-top-width:huge;border-left-style:wavy;"
                 "border-right-color:nope;width:10px;height:10px'></div></body>", 320);
    TEST_ASSERT_EQUAL(3, l61_diag_count("border edge refused"));
    TEST_ASSERT_EQUAL(0, lot52_node("a")->own_edges);
    TEST_ASSERT_EQUAL(1, l61_count(OS_FB_WEB_BORDER));
    /* radii with per-side borders: square, diagnosed */
    lot52_render(L64_BODY "<div style='width:10px;height:10px;border-top:2px solid #ff0000;border-radius:4px'></div></body>", 320);
    TEST_ASSERT_EQUAL(1, l61_diag_count("edges radius painted square"));
    l61_rect_at(0, 0, 10, 2, 0xFF0000);
}

static void test_l64_edges_logical(void) {
    /* horizontal ltr: inline-start is left, block-end bottom */
    lot52_render(L64_BODY "<div id=a style='width:10px;height:4px;border-inline-start:3px solid #ff0000;border-block-end:1px solid #00ff00'></div></body>", 320);
    l64_box("a", 0, 0, 13, 5);
    l61_rect_at(0, 0, 3, 4, 0xFF0000);
    l61_rect_at(0, 4, 13, 1, 0x00FF00);
    /* rtl: inline-start is right; a logical radius maps to the right corner */
    lot52_render(L64_BODY "<div style='direction:rtl;width:30px'><div id=r style='width:10px;height:4px;border-inline-start:3px solid #ff0000;"
                 "border-start-start-radius:4px'></div></div></body>", 320);
    l64_box("r", 17, 0, 13, 4);
    l61_rect_at(27, 0, 3, 4, 0xFF0000);
    TEST_ASSERT_EQUAL(4, lot52_node("r")->rad_x[1]);
    TEST_ASSERT_EQUAL(0, lot52_node("r")->rad_x[0]);
    lot52_render(L64_BODY "<div id=r style='border-start-end-radius:1px;border-end-start-radius:2px;border-end-end-radius:3px'></div>"
                 "<div id=s style='direction:rtl;border-start-end-radius:1px;border-end-start-radius:2px;border-end-end-radius:3px'></div></body>", 320);
    TEST_ASSERT_EQUAL(1, lot52_node("r")->rad_x[1]);
    TEST_ASSERT_EQUAL(2, lot52_node("r")->rad_x[3]);
    TEST_ASSERT_EQUAL(3, lot52_node("r")->rad_x[2]);
    TEST_ASSERT_EQUAL(1, lot52_node("s")->rad_x[0]);
    TEST_ASSERT_EQUAL(2, lot52_node("s")->rad_x[2]);
    TEST_ASSERT_EQUAL(3, lot52_node("s")->rad_x[3]);
    /* vertical-lr: block-start is left, inline-end bottom */
    lot52_render(L64_BODY "<div style='writing-mode:vertical-lr;height:20px'><div id=c style='width:6px;border-block-start:2px solid #ff0000;"
                 "border-inline-end:1px solid #0000ff'></div></div></body>", 320);
    l64_box("c", 0, 0, 8, 20);
    l61_rect_at(0, 0, 2, 20, 0xFF0000);
    l61_rect_at(2, 19, 6, 1, 0x0000FF);
    TEST_ASSERT_EQUAL(2, lot52_node("c")->edge_w[3]);          /* physical again after the layout */
    TEST_ASSERT_EQUAL(1, lot52_node("c")->edge_w[2]);
    /* vertical-rl: block-start is right */
    lot52_render(L64_BODY "<div style='writing-mode:vertical-rl;height:20px'><div id=c style='width:6px;border-block-start:2px solid #ff0000'>"
                 "</div></div></body>", 320);
    l61_rect_at(6, 0, 2, 20, 0xFF0000);
    TEST_ASSERT_EQUAL(2, lot52_node("c")->edge_w[1]);
}

static void test_l64_edges_logical_pairs(void) {
    lot52_render(L64_BODY "<div id=a style='width:20px;height:10px;border-inline-width:1px 3px;border-inline-style:solid;border-inline-color:#ff0000;"
                 "border-block-color:#00ff00 #0000ff;border-block-style:solid dotted;border-block-width:2px'></div></body>", 320);
    l64_box("a", 0, 0, 24, 14);
    l61_rect_at(0, 0, 24, 2, 0x00FF00);
    l61_rect_at(0, 12, 2, 2, 0x0000FF);                       /* dotted block-end */
    l61_rect_at(0, 2, 1, 10, 0xFF0000);
    l61_rect_at(21, 2, 3, 10, 0xFF0000);
    lot52_render(L64_BODY "<div id=a style='direction:rtl;width:20px;height:10px;border-inline:2px solid #ff0000;border-inline-start-width:5px'></div>"
                 "<div id=b style='border-block:1px dashed;border-block-end:none'></div></body>", 320);
    TEST_ASSERT_EQUAL(5, lot52_node("a")->edge_w[1]);
    TEST_ASSERT_EQUAL(2, lot52_node("a")->edge_w[3]);
    TEST_ASSERT_EQUAL(5, lot52_node("b")->edge_style[0]);
    TEST_ASSERT_EQUAL(1, lot52_node("b")->edge_style[2]);
    /* refusals leave both sides unchanged */
    lot52_render(L64_BODY "<div id=a style='border-inline:2px solid;border-inline:3px wavy;border-inline-width:1px 2px 3px;border-inline-style:solid wavy;"
                 "border-block-sideways:1px;border-inline-colour:red'></div></body>", 320);
    TEST_ASSERT_EQUAL(5, l61_diag_count("border edge refused"));
    TEST_ASSERT_EQUAL(2, lot52_node("a")->edge_w[1]);
    TEST_ASSERT_EQUAL(2, lot52_node("a")->edge_w[3]);
    TEST_ASSERT_EQUAL(WE_BS_SOLID, lot52_node("a")->edge_style[3]);
}

static void lot64_edges_run(void) {
    RUN_TEST(test_l64_edges_physical);
    RUN_TEST(test_l64_edges_logical);
    RUN_TEST(test_l64_edges_logical_pairs);
}
