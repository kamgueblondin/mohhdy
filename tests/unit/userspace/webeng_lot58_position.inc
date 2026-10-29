static void test_l58_position_left_top(void) {
    lot52_render("<body style='margin:0'><div id=g style='position:relative;width:200px;height:100px'>"
        "<div id=a style='position:absolute;width:20px;height:10px;background:red;left:calc(50% - 10px);top:calc(50% - 5px)'>"
        "</div><div id=b style='height:5px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(90, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(45, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(200, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(5, lot52_node("b")->h);
    TEST_ASSERT_EQUAL(100, lot52_node("g")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l58_position_right_bottom(void) {
    lot52_render("<body style='margin:0'><div id=g style='position:relative;width:200px;height:100px'>"
        "<div id=a style='position:absolute;width:20px;height:10px;background:red;right:calc(25% + 10px);bottom:calc(25% + 5px)'>"
        "</div><div id=b style='height:5px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(120, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(60, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(200, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(5, lot52_node("b")->h);
    TEST_ASSERT_EQUAL(100, lot52_node("g")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l58_position_left_min(void) {
    lot52_render("<body style='margin:0'><div id=g style='position:relative;width:200px;height:100px'>"
        "<div id=a style='position:absolute;width:20px;height:10px;background:red;left:min(50%,30px);top:0'></div>"
        "<div id=b style='height:5px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(200, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(5, lot52_node("b")->h);
    TEST_ASSERT_EQUAL(100, lot52_node("g")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l58_position_left_max(void) {
    lot52_render("<body style='margin:0'><div id=g style='position:relative;width:200px;height:100px'>"
        "<div id=a style='position:absolute;width:20px;height:10px;background:red;left:max(10%,30px);top:0'></div>"
        "<div id=b style='height:5px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(200, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(5, lot52_node("b")->h);
    TEST_ASSERT_EQUAL(100, lot52_node("g")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l58_position_left_clamp(void) {
    lot52_render("<body style='margin:0'><div id=g style='position:relative;width:200px;height:100px'>"
        "<div id=a style='position:absolute;width:20px;height:10px;background:red;left:clamp(10px,50%,40px);top:0'>"
        "</div><div id=b style='height:5px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(40, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(200, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(5, lot52_node("b")->h);
    TEST_ASSERT_EQUAL(100, lot52_node("g")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l58_position_top_min(void) {
    lot52_render("<body style='margin:0'><div id=g style='position:relative;width:200px;height:100px'>"
        "<div id=a style='position:absolute;width:20px;height:10px;background:red;top:min(50%,20px);left:0'></div>"
        "<div id=b style='height:5px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(200, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(5, lot52_node("b")->h);
    TEST_ASSERT_EQUAL(100, lot52_node("g")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l58_position_top_max(void) {
    lot52_render("<body style='margin:0'><div id=g style='position:relative;width:200px;height:100px'>"
        "<div id=a style='position:absolute;width:20px;height:10px;background:red;top:max(20%,30px);left:0'></div>"
        "<div id=b style='height:5px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(200, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(5, lot52_node("b")->h);
    TEST_ASSERT_EQUAL(100, lot52_node("g")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l58_position_negative_left(void) {
    lot52_render("<body style='margin:0'><div id=g style='position:relative;width:200px;height:100px'>"
        "<div id=a style='position:absolute;width:20px;height:10px;background:red;left:calc(20px - 30px);top:0'>"
        "</div><div id=b style='height:5px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(-10, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(200, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(5, lot52_node("b")->h);
    TEST_ASSERT_EQUAL(100, lot52_node("g")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l58_position_negative_top(void) {
    lot52_render("<body style='margin:0'><div id=g style='position:relative;width:200px;height:100px'>"
        "<div id=a style='position:absolute;width:20px;height:10px;background:red;left:0;top:calc(20px - 30px)'>"
        "</div><div id=b style='height:5px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(-10, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(200, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(5, lot52_node("b")->h);
    TEST_ASSERT_EQUAL(100, lot52_node("g")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l58_position_width_math(void) {
    lot52_render("<body style='margin:0'><div id=g style='position:relative;width:200px;height:100px'>"
        "<div id=a style='position:absolute;width:20px;height:10px;background:red;left:calc(20px);top:calc(10px);width:calc(50% - 20px)'>"
        "</div><div id=b style='height:5px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(80, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(200, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(5, lot52_node("b")->h);
    TEST_ASSERT_EQUAL(100, lot52_node("g")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l58_position_height_math(void) {
    lot52_render("<body style='margin:0'><div id=g style='position:relative;width:200px;height:100px'>"
        "<div id=a style='position:absolute;width:20px;height:10px;background:red;left:calc(20px);top:calc(10px);height:calc(50% - 20px)'>"
        "</div><div id=b style='height:5px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(200, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(5, lot52_node("b")->h);
    TEST_ASSERT_EQUAL(100, lot52_node("g")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l58_position_left_wins_right(void) {
    lot52_render("<body style='margin:0'><div id=g style='position:relative;width:200px;height:100px'>"
        "<div id=a style='position:absolute;width:20px;height:10px;background:red;left:calc(20px);right:calc(30px);top:0'>"
        "</div><div id=b style='height:5px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(200, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(5, lot52_node("b")->h);
    TEST_ASSERT_EQUAL(100, lot52_node("g")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l58_position_inset_reset(void) {
    lot52_render("<body style='margin:0'><div id=g style='position:relative;width:200px;height:100px'>"
        "<div id=a style='position:absolute;width:20px;height:10px;background:red;left:calc(50% - 10px);left:20px;top:0'>"
        "</div><div id=b style='height:5px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(200, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(5, lot52_node("b")->h);
    TEST_ASSERT_EQUAL(100, lot52_node("g")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l58_position_bad_inset_keeps(void) {
    lot52_render("<body style='margin:0'><div id=g style='position:relative;width:200px;height:100px'>"
        "<div id=a style='position:absolute;width:20px;height:10px;background:red;left:calc(20px);left:calc(10px / 0);top:0'>"
        "</div><div id=b style='height:5px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(200, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(5, lot52_node("b")->h);
    TEST_ASSERT_EQUAL(100, lot52_node("g")->h);
    TEST_ASSERT_TRUE(lot52_has_diag("math length refused"));
}

static void test_l58_position_relative_math(void) {
    lot52_render("<body style='margin:0'><div id=g style='width:200px;height:100px'>"
        "<div id=a style='position:relative;width:20px;height:10px;background:red;left:calc(50% - 10px);top:calc(50% - 5px)'>"
        "A</div><div id=b style='height:5px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(90, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(45, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(200, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(5, lot52_node("b")->h);
    TEST_ASSERT_EQUAL(90, find_text("A")->x);
    TEST_ASSERT_EQUAL(46, find_text("A")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l58_position_relative_clamped(void) {
    lot52_render("<body style='margin:0'><div id=g style='width:200px;height:100px'>"
        "<div id=a style='position:relative;width:20px;height:10px;background:red;left:calc(100%);top:calc(100%)'>"
        "A</div><div id=b style='height:5px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(180, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(90, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(200, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(5, lot52_node("b")->h);
    TEST_ASSERT_EQUAL(180, find_text("A")->x);
    TEST_ASSERT_EQUAL(91, find_text("A")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l58_position_relative_negative(void) {
    lot52_render("<body style='margin:0'><div id=g style='width:200px;height:100px'>"
        "<div id=a style='position:relative;width:20px;height:10px;background:red;left:calc(-20px);top:calc(-20px)'>"
        "A</div><div id=b style='height:5px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(200, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(5, lot52_node("b")->h);
    TEST_ASSERT_EQUAL(0, find_text("A")->x);
    TEST_ASSERT_EQUAL(1, find_text("A")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}
