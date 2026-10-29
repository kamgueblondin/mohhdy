static void test_l59_insets_right_positive(void) {
    lot52_render("<body style='margin:0'><div id=p style='width:100px;height:80px;'><div style='height:20px'></div><di"
        "v id=a style='position:relative;width:20px;height:10px;margin-left:30px;right:10px'><span>A</span></"
        "div><div id=b style='height:5px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_NOT_NULL(find_text("A"));
    TEST_ASSERT_EQUAL(20, find_text("A")->x);
    TEST_ASSERT_EQUAL(21, find_text("A")->y);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(30, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(100, lot52_node("p")->w);
    TEST_ASSERT_EQUAL(80, lot52_node("p")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l59_insets_right_negative(void) {
    lot52_render("<body style='margin:0'><div id=p style='width:100px;height:80px;'><div style='height:20px'></div><di"
        "v id=a style='position:relative;width:20px;height:10px;margin-left:30px;right:-10px'><span>A</span><"
        "/div><div id=b style='height:5px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(40, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_NOT_NULL(find_text("A"));
    TEST_ASSERT_EQUAL(40, find_text("A")->x);
    TEST_ASSERT_EQUAL(21, find_text("A")->y);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(30, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(100, lot52_node("p")->w);
    TEST_ASSERT_EQUAL(80, lot52_node("p")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l59_insets_bottom_positive(void) {
    lot52_render("<body style='margin:0'><div id=p style='width:100px;height:80px;'><div style='height:20px'></div><di"
        "v id=a style='position:relative;width:20px;height:10px;margin-left:30px;bottom:10px'><span>A</span><"
        "/div><div id=b style='height:5px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_NOT_NULL(find_text("A"));
    TEST_ASSERT_EQUAL(30, find_text("A")->x);
    TEST_ASSERT_EQUAL(11, find_text("A")->y);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(30, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(100, lot52_node("p")->w);
    TEST_ASSERT_EQUAL(80, lot52_node("p")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l59_insets_bottom_negative(void) {
    lot52_render("<body style='margin:0'><div id=p style='width:100px;height:80px;'><div style='height:20px'></div><di"
        "v id=a style='position:relative;width:20px;height:10px;margin-left:30px;bottom:-10px'><span>A</span>"
        "</div><div id=b style='height:5px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_NOT_NULL(find_text("A"));
    TEST_ASSERT_EQUAL(30, find_text("A")->x);
    TEST_ASSERT_EQUAL(31, find_text("A")->y);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(30, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(100, lot52_node("p")->w);
    TEST_ASSERT_EQUAL(80, lot52_node("p")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l59_insets_right_clamped_start(void) {
    lot52_render("<body style='margin:0'><div id=p style='width:100px;height:80px;'><div style='height:20px'></div><di"
        "v id=a style='position:relative;width:20px;height:10px;margin-left:30px;right:200px'><span>A</span><"
        "/div><div id=b style='height:5px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_NOT_NULL(find_text("A"));
    TEST_ASSERT_EQUAL(0, find_text("A")->x);
    TEST_ASSERT_EQUAL(21, find_text("A")->y);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(30, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(100, lot52_node("p")->w);
    TEST_ASSERT_EQUAL(80, lot52_node("p")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l59_insets_right_clamped_end(void) {
    lot52_render("<body style='margin:0'><div id=p style='width:100px;height:80px;'><div style='height:20px'></div><di"
        "v id=a style='position:relative;width:20px;height:10px;margin-left:30px;right:-200px'><span>A</span>"
        "</div><div id=b style='height:5px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(80, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_NOT_NULL(find_text("A"));
    TEST_ASSERT_EQUAL(80, find_text("A")->x);
    TEST_ASSERT_EQUAL(21, find_text("A")->y);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(30, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(100, lot52_node("p")->w);
    TEST_ASSERT_EQUAL(80, lot52_node("p")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l59_insets_bottom_clamped_start(void) {
    lot52_render("<body style='margin:0'><div id=p style='width:100px;height:80px;'><div style='height:20px'></div><di"
        "v id=a style='position:relative;width:20px;height:10px;margin-left:30px;bottom:200px'><span>A</span>"
        "</div><div id=b style='height:5px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_NOT_NULL(find_text("A"));
    TEST_ASSERT_EQUAL(30, find_text("A")->x);
    TEST_ASSERT_EQUAL(1, find_text("A")->y);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(30, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(100, lot52_node("p")->w);
    TEST_ASSERT_EQUAL(80, lot52_node("p")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l59_insets_bottom_clamped_end(void) {
    lot52_render("<body style='margin:0'><div id=p style='width:100px;height:80px;'><div style='height:20px'></div><di"
        "v id=a style='position:relative;width:20px;height:10px;margin-left:30px;bottom:-200px'><span>A</span"
        "></div><div id=b style='height:5px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(70, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_NOT_NULL(find_text("A"));
    TEST_ASSERT_EQUAL(30, find_text("A")->x);
    TEST_ASSERT_EQUAL(71, find_text("A")->y);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(30, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(100, lot52_node("p")->w);
    TEST_ASSERT_EQUAL(80, lot52_node("p")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l59_insets_left_priority(void) {
    lot52_render("<body style='margin:0'><div id=p style='width:100px;height:80px;'><div style='height:20px'></div><di"
        "v id=a style='position:relative;width:20px;height:10px;margin-left:30px;left:10px;right:20px'><span>"
        "A</span></div><div id=b style='height:5px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(40, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_NOT_NULL(find_text("A"));
    TEST_ASSERT_EQUAL(40, find_text("A")->x);
    TEST_ASSERT_EQUAL(21, find_text("A")->y);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(30, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(100, lot52_node("p")->w);
    TEST_ASSERT_EQUAL(80, lot52_node("p")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l59_insets_top_priority(void) {
    lot52_render("<body style='margin:0'><div id=p style='width:100px;height:80px;'><div style='height:20px'></div><di"
        "v id=a style='position:relative;width:20px;height:10px;margin-left:30px;top:10px;bottom:20px'><span>"
        "A</span></div><div id=b style='height:5px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_NOT_NULL(find_text("A"));
    TEST_ASSERT_EQUAL(30, find_text("A")->x);
    TEST_ASSERT_EQUAL(31, find_text("A")->y);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(30, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(100, lot52_node("p")->w);
    TEST_ASSERT_EQUAL(80, lot52_node("p")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l59_insets_left_zero_priority(void) {
    lot52_render("<body style='margin:0'><div id=p style='width:100px;height:80px;'><div style='height:20px'></div><di"
        "v id=a style='position:relative;width:20px;height:10px;margin-left:30px;left:0;right:20px'><span>A</"
        "span></div><div id=b style='height:5px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_NOT_NULL(find_text("A"));
    TEST_ASSERT_EQUAL(30, find_text("A")->x);
    TEST_ASSERT_EQUAL(21, find_text("A")->y);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(30, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(100, lot52_node("p")->w);
    TEST_ASSERT_EQUAL(80, lot52_node("p")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l59_insets_top_zero_priority(void) {
    lot52_render("<body style='margin:0'><div id=p style='width:100px;height:80px;'><div style='height:20px'></div><di"
        "v id=a style='position:relative;width:20px;height:10px;margin-left:30px;top:0;bottom:20px'><span>A</"
        "span></div><div id=b style='height:5px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_NOT_NULL(find_text("A"));
    TEST_ASSERT_EQUAL(30, find_text("A")->x);
    TEST_ASSERT_EQUAL(21, find_text("A")->y);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(30, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(100, lot52_node("p")->w);
    TEST_ASSERT_EQUAL(80, lot52_node("p")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l59_insets_auto_releases_left(void) {
    lot52_render("<body style='margin:0'><div id=p style='width:100px;height:80px;'><div style='height:20px'></div><di"
        "v id=a style='position:relative;width:20px;height:10px;margin-left:30px;left:10px;right:20px;left:au"
        "to'><span>A</span></div><div id=b style='height:5px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_NOT_NULL(find_text("A"));
    TEST_ASSERT_EQUAL(10, find_text("A")->x);
    TEST_ASSERT_EQUAL(21, find_text("A")->y);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(30, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(100, lot52_node("p")->w);
    TEST_ASSERT_EQUAL(80, lot52_node("p")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l59_insets_auto_releases_top(void) {
    lot52_render("<body style='margin:0'><div id=p style='width:100px;height:80px;'><div style='height:20px'></div><di"
        "v id=a style='position:relative;width:20px;height:10px;margin-left:30px;top:10px;bottom:20px;top:aut"
        "o'><span>A</span></div><div id=b style='height:5px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_NOT_NULL(find_text("A"));
    TEST_ASSERT_EQUAL(30, find_text("A")->x);
    TEST_ASSERT_EQUAL(1, find_text("A")->y);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(30, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(100, lot52_node("p")->w);
    TEST_ASSERT_EQUAL(80, lot52_node("p")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l59_insets_auto_releases_right(void) {
    lot52_render("<body style='margin:0'><div id=p style='width:100px;height:80px;'><div style='height:20px'></div><di"
        "v id=a style='position:relative;width:20px;height:10px;margin-left:30px;right:20px;right:auto'><span"
        ">A</span></div><div id=b style='height:5px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_NOT_NULL(find_text("A"));
    TEST_ASSERT_EQUAL(30, find_text("A")->x);
    TEST_ASSERT_EQUAL(21, find_text("A")->y);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(30, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(100, lot52_node("p")->w);
    TEST_ASSERT_EQUAL(80, lot52_node("p")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l59_insets_auto_releases_bottom(void) {
    lot52_render("<body style='margin:0'><div id=p style='width:100px;height:80px;'><div style='height:20px'></div><di"
        "v id=a style='position:relative;width:20px;height:10px;margin-left:30px;bottom:20px;bottom:auto'><sp"
        "an>A</span></div><div id=b style='height:5px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_NOT_NULL(find_text("A"));
    TEST_ASSERT_EQUAL(30, find_text("A")->x);
    TEST_ASSERT_EQUAL(21, find_text("A")->y);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(30, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(100, lot52_node("p")->w);
    TEST_ASSERT_EQUAL(80, lot52_node("p")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l59_insets_percent_right(void) {
    lot52_render("<body style='margin:0'><div id=p style='width:100px;height:80px;'><div style='height:20px'></div><di"
        "v id=a style='position:relative;width:20px;height:10px;margin-left:30px;right:10%'><span>A</span></d"
        "iv><div id=b style='height:5px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_NOT_NULL(find_text("A"));
    TEST_ASSERT_EQUAL(20, find_text("A")->x);
    TEST_ASSERT_EQUAL(21, find_text("A")->y);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(30, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(100, lot52_node("p")->w);
    TEST_ASSERT_EQUAL(80, lot52_node("p")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l59_insets_percent_bottom(void) {
    lot52_render("<body style='margin:0'><div id=p style='width:100px;height:80px;'><div style='height:20px'></div><di"
        "v id=a style='position:relative;width:20px;height:10px;margin-left:30px;bottom:10%'><span>A</span></"
        "div><div id=b style='height:5px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(12, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_NOT_NULL(find_text("A"));
    TEST_ASSERT_EQUAL(30, find_text("A")->x);
    TEST_ASSERT_EQUAL(13, find_text("A")->y);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(30, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(100, lot52_node("p")->w);
    TEST_ASSERT_EQUAL(80, lot52_node("p")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l59_insets_negative_percent(void) {
    lot52_render("<body style='margin:0'><div id=p style='width:100px;height:80px;'><div style='height:20px'></div><di"
        "v id=a style='position:relative;width:20px;height:10px;margin-left:30px;right:-10%;bottom:-10%'><spa"
        "n>A</span></div><div id=b style='height:5px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(40, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(28, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_NOT_NULL(find_text("A"));
    TEST_ASSERT_EQUAL(40, find_text("A")->x);
    TEST_ASSERT_EQUAL(29, find_text("A")->y);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(30, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(100, lot52_node("p")->w);
    TEST_ASSERT_EQUAL(80, lot52_node("p")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l59_insets_percent_left_top(void) {
    lot52_render("<body style='margin:0'><div id=p style='width:100px;height:80px;'><div style='height:20px'></div><di"
        "v id=a style='position:relative;width:20px;height:10px;margin-left:30px;left:10%;top:10%'><span>A</s"
        "pan></div><div id=b style='height:5px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(40, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(28, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_NOT_NULL(find_text("A"));
    TEST_ASSERT_EQUAL(40, find_text("A")->x);
    TEST_ASSERT_EQUAL(29, find_text("A")->y);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(30, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(100, lot52_node("p")->w);
    TEST_ASSERT_EQUAL(80, lot52_node("p")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l59_insets_math_right(void) {
    lot52_render("<body style='margin:0'><div id=p style='width:100px;height:80px;'><div style='height:20px'></div><di"
        "v id=a style='position:relative;width:20px;height:10px;margin-left:30px;right:calc(10% + 5px)'><span"
        ">A</span></div><div id=b style='height:5px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(15, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_NOT_NULL(find_text("A"));
    TEST_ASSERT_EQUAL(15, find_text("A")->x);
    TEST_ASSERT_EQUAL(21, find_text("A")->y);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(30, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(100, lot52_node("p")->w);
    TEST_ASSERT_EQUAL(80, lot52_node("p")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l59_insets_math_bottom(void) {
    lot52_render("<body style='margin:0'><div id=p style='width:100px;height:80px;'><div style='height:20px'></div><di"
        "v id=a style='position:relative;width:20px;height:10px;margin-left:30px;bottom:calc(10% + 2px)'><spa"
        "n>A</span></div><div id=b style='height:5px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_NOT_NULL(find_text("A"));
    TEST_ASSERT_EQUAL(30, find_text("A")->x);
    TEST_ASSERT_EQUAL(11, find_text("A")->y);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(30, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(100, lot52_node("p")->w);
    TEST_ASSERT_EQUAL(80, lot52_node("p")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l59_insets_math_right_negative(void) {
    lot52_render("<body style='margin:0'><div id=p style='width:100px;height:80px;'><div style='height:20px'></div><di"
        "v id=a style='position:relative;width:20px;height:10px;margin-left:30px;right:calc(0px - 15px)'><spa"
        "n>A</span></div><div id=b style='height:5px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(45, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_NOT_NULL(find_text("A"));
    TEST_ASSERT_EQUAL(45, find_text("A")->x);
    TEST_ASSERT_EQUAL(21, find_text("A")->y);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(30, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(100, lot52_node("p")->w);
    TEST_ASSERT_EQUAL(80, lot52_node("p")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l59_insets_math_bottom_negative(void) {
    lot52_render("<body style='margin:0'><div id=p style='width:100px;height:80px;'><div style='height:20px'></div><di"
        "v id=a style='position:relative;width:20px;height:10px;margin-left:30px;bottom:calc(0px - 15px)'><sp"
        "an>A</span></div><div id=b style='height:5px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(35, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_NOT_NULL(find_text("A"));
    TEST_ASSERT_EQUAL(30, find_text("A")->x);
    TEST_ASSERT_EQUAL(36, find_text("A")->y);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(30, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(100, lot52_node("p")->w);
    TEST_ASSERT_EQUAL(80, lot52_node("p")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l59_insets_inset_one(void) {
    lot52_render("<body style='margin:0'><div id=p style='width:100px;height:80px;'><div style='height:20px'></div><di"
        "v id=a style='position:relative;width:20px;height:10px;margin-left:30px;inset:10px'><span>A</span></"
        "div><div id=b style='height:5px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(40, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_NOT_NULL(find_text("A"));
    TEST_ASSERT_EQUAL(40, find_text("A")->x);
    TEST_ASSERT_EQUAL(31, find_text("A")->y);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(30, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(100, lot52_node("p")->w);
    TEST_ASSERT_EQUAL(80, lot52_node("p")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l59_insets_inset_two(void) {
    lot52_render("<body style='margin:0'><div id=p style='width:100px;height:80px;'><div style='height:20px'></div><di"
        "v id=a style='position:relative;width:20px;height:10px;margin-left:30px;inset:10px 20px'><span>A</sp"
        "an></div><div id=b style='height:5px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(50, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_NOT_NULL(find_text("A"));
    TEST_ASSERT_EQUAL(50, find_text("A")->x);
    TEST_ASSERT_EQUAL(31, find_text("A")->y);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(30, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(100, lot52_node("p")->w);
    TEST_ASSERT_EQUAL(80, lot52_node("p")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l59_insets_inset_three(void) {
    lot52_render("<body style='margin:0'><div id=p style='width:100px;height:80px;'><div style='height:20px'></div><di"
        "v id=a style='position:relative;width:20px;height:10px;margin-left:30px;inset:10px 20px 30px'><span>"
        "A</span></div><div id=b style='height:5px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(50, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_NOT_NULL(find_text("A"));
    TEST_ASSERT_EQUAL(50, find_text("A")->x);
    TEST_ASSERT_EQUAL(31, find_text("A")->y);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(30, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(100, lot52_node("p")->w);
    TEST_ASSERT_EQUAL(80, lot52_node("p")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l59_insets_inset_four(void) {
    lot52_render("<body style='margin:0'><div id=p style='width:100px;height:80px;'><div style='height:20px'></div><di"
        "v id=a style='position:relative;width:20px;height:10px;margin-left:30px;inset:10px 20px 30px 5px'><s"
        "pan>A</span></div><div id=b style='height:5px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(35, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_NOT_NULL(find_text("A"));
    TEST_ASSERT_EQUAL(35, find_text("A")->x);
    TEST_ASSERT_EQUAL(31, find_text("A")->y);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(30, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(100, lot52_node("p")->w);
    TEST_ASSERT_EQUAL(80, lot52_node("p")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l59_insets_inset_auto(void) {
    lot52_render("<body style='margin:0'><div id=p style='width:100px;height:80px;'><div style='height:20px'></div><di"
        "v id=a style='position:relative;width:20px;height:10px;margin-left:30px;inset:10px;inset:auto'><span"
        ">A</span></div><div id=b style='height:5px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_NOT_NULL(find_text("A"));
    TEST_ASSERT_EQUAL(30, find_text("A")->x);
    TEST_ASSERT_EQUAL(21, find_text("A")->y);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(30, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(100, lot52_node("p")->w);
    TEST_ASSERT_EQUAL(80, lot52_node("p")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l59_insets_inset_sides_only(void) {
    lot52_render("<body style='margin:0'><div id=p style='width:100px;height:80px;'><div style='height:20px'></div><di"
        "v id=a style='position:relative;width:20px;height:10px;margin-left:30px;inset:auto 10px auto auto'><"
        "span>A</span></div><div id=b style='height:5px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_NOT_NULL(find_text("A"));
    TEST_ASSERT_EQUAL(20, find_text("A")->x);
    TEST_ASSERT_EQUAL(21, find_text("A")->y);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(30, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(100, lot52_node("p")->w);
    TEST_ASSERT_EQUAL(80, lot52_node("p")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l59_insets_inset_bottom_only(void) {
    lot52_render("<body style='margin:0'><div id=p style='width:100px;height:80px;'><div style='height:20px'></div><di"
        "v id=a style='position:relative;width:20px;height:10px;margin-left:30px;inset:auto auto 10px'><span>"
        "A</span></div><div id=b style='height:5px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_NOT_NULL(find_text("A"));
    TEST_ASSERT_EQUAL(30, find_text("A")->x);
    TEST_ASSERT_EQUAL(11, find_text("A")->y);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(30, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(100, lot52_node("p")->w);
    TEST_ASSERT_EQUAL(80, lot52_node("p")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l59_insets_inset_percent(void) {
    lot52_render("<body style='margin:0'><div id=p style='width:100px;height:80px;'><div style='height:20px'></div><di"
        "v id=a style='position:relative;width:20px;height:10px;margin-left:30px;inset:10%'><span>A</span></d"
        "iv><div id=b style='height:5px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(40, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(28, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_NOT_NULL(find_text("A"));
    TEST_ASSERT_EQUAL(40, find_text("A")->x);
    TEST_ASSERT_EQUAL(29, find_text("A")->y);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(30, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(100, lot52_node("p")->w);
    TEST_ASSERT_EQUAL(80, lot52_node("p")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l59_insets_inset_functions(void) {
    lot52_render("<body style='margin:0'><div id=p style='width:100px;height:80px;'><div style='height:20px'></div><di"
        "v id=a style='position:relative;width:20px;height:10px;margin-left:30px;inset:calc(10% + 2px) calc(1"
        "0% + 5px)'><span>A</span></div><div id=b style='height:5px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(45, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_NOT_NULL(find_text("A"));
    TEST_ASSERT_EQUAL(45, find_text("A")->x);
    TEST_ASSERT_EQUAL(31, find_text("A")->y);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(30, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(100, lot52_node("p")->w);
    TEST_ASSERT_EQUAL(80, lot52_node("p")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l59_insets_inline_one(void) {
    lot52_render("<body style='margin:0'><div id=p style='width:100px;height:80px;'><div style='height:20px'></div><di"
        "v id=a style='position:relative;width:20px;height:10px;margin-left:30px;inset-inline:10px'><span>A</"
        "span></div><div id=b style='height:5px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(40, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_NOT_NULL(find_text("A"));
    TEST_ASSERT_EQUAL(40, find_text("A")->x);
    TEST_ASSERT_EQUAL(21, find_text("A")->y);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(30, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(100, lot52_node("p")->w);
    TEST_ASSERT_EQUAL(80, lot52_node("p")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l59_insets_inline_two(void) {
    lot52_render("<body style='margin:0'><div id=p style='width:100px;height:80px;'><div style='height:20px'></div><di"
        "v id=a style='position:relative;width:20px;height:10px;margin-left:30px;inset-inline:auto 10px'><spa"
        "n>A</span></div><div id=b style='height:5px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_NOT_NULL(find_text("A"));
    TEST_ASSERT_EQUAL(20, find_text("A")->x);
    TEST_ASSERT_EQUAL(21, find_text("A")->y);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(30, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(100, lot52_node("p")->w);
    TEST_ASSERT_EQUAL(80, lot52_node("p")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l59_insets_block_one(void) {
    lot52_render("<body style='margin:0'><div id=p style='width:100px;height:80px;'><div style='height:20px'></div><di"
        "v id=a style='position:relative;width:20px;height:10px;margin-left:30px;inset-block:10px'><span>A</s"
        "pan></div><div id=b style='height:5px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_NOT_NULL(find_text("A"));
    TEST_ASSERT_EQUAL(30, find_text("A")->x);
    TEST_ASSERT_EQUAL(31, find_text("A")->y);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(30, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(100, lot52_node("p")->w);
    TEST_ASSERT_EQUAL(80, lot52_node("p")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l59_insets_block_two(void) {
    lot52_render("<body style='margin:0'><div id=p style='width:100px;height:80px;'><div style='height:20px'></div><di"
        "v id=a style='position:relative;width:20px;height:10px;margin-left:30px;inset-block:auto 10px'><span"
        ">A</span></div><div id=b style='height:5px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_NOT_NULL(find_text("A"));
    TEST_ASSERT_EQUAL(30, find_text("A")->x);
    TEST_ASSERT_EQUAL(11, find_text("A")->y);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(30, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(100, lot52_node("p")->w);
    TEST_ASSERT_EQUAL(80, lot52_node("p")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l59_insets_logical_start(void) {
    lot52_render("<body style='margin:0'><div id=p style='width:100px;height:80px;'><div style='height:20px'></div><di"
        "v id=a style='position:relative;width:20px;height:10px;margin-left:30px;inset-inline-start:10px;inse"
        "t-block-start:10px'><span>A</span></div><div id=b style='height:5px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(40, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_NOT_NULL(find_text("A"));
    TEST_ASSERT_EQUAL(40, find_text("A")->x);
    TEST_ASSERT_EQUAL(31, find_text("A")->y);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(30, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(100, lot52_node("p")->w);
    TEST_ASSERT_EQUAL(80, lot52_node("p")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l59_insets_logical_end(void) {
    lot52_render("<body style='margin:0'><div id=p style='width:100px;height:80px;'><div style='height:20px'></div><di"
        "v id=a style='position:relative;width:20px;height:10px;margin-left:30px;inset-inline-end:10px;inset-"
        "block-end:10px'><span>A</span></div><div id=b style='height:5px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_NOT_NULL(find_text("A"));
    TEST_ASSERT_EQUAL(20, find_text("A")->x);
    TEST_ASSERT_EQUAL(11, find_text("A")->y);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(30, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(100, lot52_node("p")->w);
    TEST_ASSERT_EQUAL(80, lot52_node("p")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l59_insets_logical_auto(void) {
    lot52_render("<body style='margin:0'><div id=p style='width:100px;height:80px;'><div style='height:20px'></div><di"
        "v id=a style='position:relative;width:20px;height:10px;margin-left:30px;inset-inline-start:20px;inse"
        "t-inline-end:10px;inset-inline-start:auto'><span>A</span></div><div id=b style='height:5px'></div></"
        "div></body>", 320);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_NOT_NULL(find_text("A"));
    TEST_ASSERT_EQUAL(20, find_text("A")->x);
    TEST_ASSERT_EQUAL(21, find_text("A")->y);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(30, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(100, lot52_node("p")->w);
    TEST_ASSERT_EQUAL(80, lot52_node("p")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l59_insets_physical_after_logical(void) {
    lot52_render("<body style='margin:0'><div id=p style='width:100px;height:80px;'><div style='height:20px'></div><di"
        "v id=a style='position:relative;width:20px;height:10px;margin-left:30px;inset-inline-end:10px;right:"
        "20px'><span>A</span></div><div id=b style='height:5px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_NOT_NULL(find_text("A"));
    TEST_ASSERT_EQUAL(10, find_text("A")->x);
    TEST_ASSERT_EQUAL(21, find_text("A")->y);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(30, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(100, lot52_node("p")->w);
    TEST_ASSERT_EQUAL(80, lot52_node("p")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l59_insets_logical_after_physical(void) {
    lot52_render("<body style='margin:0'><div id=p style='width:100px;height:80px;'><div style='height:20px'></div><di"
        "v id=a style='position:relative;width:20px;height:10px;margin-left:30px;right:20px;inset-inline-end:"
        "10px'><span>A</span></div><div id=b style='height:5px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_NOT_NULL(find_text("A"));
    TEST_ASSERT_EQUAL(20, find_text("A")->x);
    TEST_ASSERT_EQUAL(21, find_text("A")->y);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(30, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(100, lot52_node("p")->w);
    TEST_ASSERT_EQUAL(80, lot52_node("p")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l59_insets_shorthand_then_longhand(void) {
    lot52_render("<body style='margin:0'><div id=p style='width:100px;height:80px;'><div style='height:20px'></div><di"
        "v id=a style='position:relative;width:20px;height:10px;margin-left:30px;inset:10px;left:5px;top:5px'"
        "><span>A</span></div><div id=b style='height:5px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(35, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(25, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_NOT_NULL(find_text("A"));
    TEST_ASSERT_EQUAL(35, find_text("A")->x);
    TEST_ASSERT_EQUAL(26, find_text("A")->y);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(30, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(100, lot52_node("p")->w);
    TEST_ASSERT_EQUAL(80, lot52_node("p")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l59_insets_longhand_then_shorthand(void) {
    lot52_render("<body style='margin:0'><div id=p style='width:100px;height:80px;'><div style='height:20px'></div><di"
        "v id=a style='position:relative;width:20px;height:10px;margin-left:30px;left:5px;top:5px;inset:10px'"
        "><span>A</span></div><div id=b style='height:5px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(40, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_NOT_NULL(find_text("A"));
    TEST_ASSERT_EQUAL(40, find_text("A")->x);
    TEST_ASSERT_EQUAL(31, find_text("A")->y);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(30, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(100, lot52_node("p")->w);
    TEST_ASSERT_EQUAL(80, lot52_node("p")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l59_insets_auto_then_right(void) {
    lot52_render("<body style='margin:0'><div id=p style='width:100px;height:80px;'><div style='height:20px'></div><di"
        "v id=a style='position:relative;width:20px;height:10px;margin-left:30px;inset:auto;right:10px;bottom"
        ":5px'><span>A</span></div><div id=b style='height:5px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(15, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_NOT_NULL(find_text("A"));
    TEST_ASSERT_EQUAL(20, find_text("A")->x);
    TEST_ASSERT_EQUAL(16, find_text("A")->y);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(30, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(100, lot52_node("p")->w);
    TEST_ASSERT_EQUAL(80, lot52_node("p")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l59_insets_uppercase_percent(void) {
    lot52_render("<body style='margin:0'><div id=p style='width:100px;height:80px;'><div style='height:20px'></div><di"
        "v id=a style='position:relative;width:20px;height:10px;margin-left:30px;RIGHT:10%;BOTTOM:10%'><span>"
        "A</span></div><div id=b style='height:5px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(12, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_NOT_NULL(find_text("A"));
    TEST_ASSERT_EQUAL(20, find_text("A")->x);
    TEST_ASSERT_EQUAL(13, find_text("A")->y);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(30, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(100, lot52_node("p")->w);
    TEST_ASSERT_EQUAL(80, lot52_node("p")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l59_insets_atomic_0(void) {
    lot52_render("<body style='margin:0'><div id=p style='width:100px;height:80px;'><div style='height:20px'></div><di"
        "v id=a style='position:relative;width:20px;height:10px;margin-left:30px;right:10px;bottom:10px;inset"
        ":1px 2px 3px 4px 5px'><span>A</span></div><div id=b style='height:5px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_NOT_NULL(find_text("A"));
    TEST_ASSERT_EQUAL(20, find_text("A")->x);
    TEST_ASSERT_EQUAL(11, find_text("A")->y);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(30, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(100, lot52_node("p")->w);
    TEST_ASSERT_EQUAL(80, lot52_node("p")->h);
    TEST_ASSERT_TRUE(lot52_has_diag("position offset refused"));
}

static void test_l59_insets_atomic_1(void) {
    lot52_render("<body style='margin:0'><div id=p style='width:100px;height:80px;'><div style='height:20px'></div><di"
        "v id=a style='position:relative;width:20px;height:10px;margin-left:30px;right:10px;bottom:10px;inset"
        "-inline:1px 2px 3px'><span>A</span></div><div id=b style='height:5px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_NOT_NULL(find_text("A"));
    TEST_ASSERT_EQUAL(20, find_text("A")->x);
    TEST_ASSERT_EQUAL(11, find_text("A")->y);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(30, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(100, lot52_node("p")->w);
    TEST_ASSERT_EQUAL(80, lot52_node("p")->h);
    TEST_ASSERT_TRUE(lot52_has_diag("position offset refused"));
}

static void test_l59_insets_atomic_2(void) {
    lot52_render("<body style='margin:0'><div id=p style='width:100px;height:80px;'><div style='height:20px'></div><di"
        "v id=a style='position:relative;width:20px;height:10px;margin-left:30px;right:10px;bottom:10px;inset"
        "-block:1px 2px 3px'><span>A</span></div><div id=b style='height:5px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_NOT_NULL(find_text("A"));
    TEST_ASSERT_EQUAL(20, find_text("A")->x);
    TEST_ASSERT_EQUAL(11, find_text("A")->y);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(30, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(100, lot52_node("p")->w);
    TEST_ASSERT_EQUAL(80, lot52_node("p")->h);
    TEST_ASSERT_TRUE(lot52_has_diag("position offset refused"));
}

static void test_l59_insets_atomic_3(void) {
    lot52_render("<body style='margin:0'><div id=p style='width:100px;height:80px;'><div style='height:20px'></div><di"
        "v id=a style='position:relative;width:20px;height:10px;margin-left:30px;right:10px;bottom:10px;inset"
        ":1px nonsense'><span>A</span></div><div id=b style='height:5px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_NOT_NULL(find_text("A"));
    TEST_ASSERT_EQUAL(20, find_text("A")->x);
    TEST_ASSERT_EQUAL(11, find_text("A")->y);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(30, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(100, lot52_node("p")->w);
    TEST_ASSERT_EQUAL(80, lot52_node("p")->h);
    TEST_ASSERT_TRUE(lot52_has_diag("position offset refused"));
}

static void test_l59_insets_atomic_4(void) {
    lot52_render("<body style='margin:0'><div id=p style='width:100px;height:80px;'><div style='height:20px'></div><di"
        "v id=a style='position:relative;width:20px;height:10px;margin-left:30px;right:10px;bottom:10px;inset"
        ":auto calc(2px * 3px)'><span>A</span></div><div id=b style='height:5px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_NOT_NULL(find_text("A"));
    TEST_ASSERT_EQUAL(20, find_text("A")->x);
    TEST_ASSERT_EQUAL(11, find_text("A")->y);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(30, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(100, lot52_node("p")->w);
    TEST_ASSERT_EQUAL(80, lot52_node("p")->h);
    TEST_ASSERT_TRUE(lot52_has_diag("position offset refused"));
}

static void test_l59_insets_atomic_5(void) {
    lot52_render("<body style='margin:0'><div id=p style='width:100px;height:80px;'><div style='height:20px'></div><di"
        "v id=a style='position:relative;width:20px;height:10px;margin-left:30px;right:10px;bottom:10px;inset"
        "-inline:20px calc(1px / 0)'><span>A</span></div><div id=b style='height:5px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_NOT_NULL(find_text("A"));
    TEST_ASSERT_EQUAL(20, find_text("A")->x);
    TEST_ASSERT_EQUAL(11, find_text("A")->y);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(30, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(100, lot52_node("p")->w);
    TEST_ASSERT_EQUAL(80, lot52_node("p")->h);
    TEST_ASSERT_TRUE(lot52_has_diag("position offset refused"));
}

static void test_l59_insets_atomic_6(void) {
    lot52_render("<body style='margin:0'><div id=p style='width:100px;height:80px;'><div style='height:20px'></div><di"
        "v id=a style='position:relative;width:20px;height:10px;margin-left:30px;right:10px;bottom:10px;inset"
        "-block:10px calc(20px + 1)'><span>A</span></div><div id=b style='height:5px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_NOT_NULL(find_text("A"));
    TEST_ASSERT_EQUAL(20, find_text("A")->x);
    TEST_ASSERT_EQUAL(11, find_text("A")->y);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(30, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(100, lot52_node("p")->w);
    TEST_ASSERT_EQUAL(80, lot52_node("p")->h);
    TEST_ASSERT_TRUE(lot52_has_diag("position offset refused"));
}

static void test_l59_insets_atomic_7(void) {
    lot52_render("<body style='margin:0'><div id=p style='width:100px;height:80px;'><div style='height:20px'></div><di"
        "v id=a style='position:relative;width:20px;height:10px;margin-left:30px;right:10px;bottom:10px;right"
        ":anchor(--x)'><span>A</span></div><div id=b style='height:5px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_NOT_NULL(find_text("A"));
    TEST_ASSERT_EQUAL(20, find_text("A")->x);
    TEST_ASSERT_EQUAL(11, find_text("A")->y);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(30, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(100, lot52_node("p")->w);
    TEST_ASSERT_EQUAL(80, lot52_node("p")->h);
    TEST_ASSERT_TRUE(lot52_has_diag("position offset refused"));
}

static void test_l59_insets_atomic_8(void) {
    lot52_render("<body style='margin:0'><div id=p style='width:100px;height:80px;'><div style='height:20px'></div><di"
        "v id=a style='position:relative;width:20px;height:10px;margin-left:30px;right:10px;bottom:10px;botto"
        "m:2001px'><span>A</span></div><div id=b style='height:5px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_NOT_NULL(find_text("A"));
    TEST_ASSERT_EQUAL(20, find_text("A")->x);
    TEST_ASSERT_EQUAL(11, find_text("A")->y);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(30, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(100, lot52_node("p")->w);
    TEST_ASSERT_EQUAL(80, lot52_node("p")->h);
    TEST_ASSERT_TRUE(lot52_has_diag("position offset refused"));
}

static void test_l59_insets_atomic_9(void) {
    lot52_render("<body style='margin:0'><div id=p style='width:100px;height:80px;'><div style='height:20px'></div><di"
        "v id=a style='position:relative;width:20px;height:10px;margin-left:30px;right:10px;bottom:10px;left:"
        "10%tail'><span>A</span></div><div id=b style='height:5px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_NOT_NULL(find_text("A"));
    TEST_ASSERT_EQUAL(20, find_text("A")->x);
    TEST_ASSERT_EQUAL(11, find_text("A")->y);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(30, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(100, lot52_node("p")->w);
    TEST_ASSERT_EQUAL(80, lot52_node("p")->h);
    TEST_ASSERT_TRUE(lot52_has_diag("position offset refused"));
}

static void test_l59_insets_atomic_10(void) {
    lot52_render("<body style='margin:0'><div id=p style='width:100px;height:80px;'><div style='height:20px'></div><di"
        "v id=a style='position:relative;width:20px;height:10px;margin-left:30px;right:10px;bottom:10px;top:1"
        ".5px'><span>A</span></div><div id=b style='height:5px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_NOT_NULL(find_text("A"));
    TEST_ASSERT_EQUAL(20, find_text("A")->x);
    TEST_ASSERT_EQUAL(11, find_text("A")->y);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(30, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(100, lot52_node("p")->w);
    TEST_ASSERT_EQUAL(80, lot52_node("p")->h);
    TEST_ASSERT_TRUE(lot52_has_diag("position offset refused"));
}

static void test_l59_insets_atomic_11(void) {
    lot52_render("<body style='margin:0'><div id=p style='width:100px;height:80px;'><div style='height:20px'></div><di"
        "v id=a style='position:relative;width:20px;height:10px;margin-left:30px;right:10px;bottom:10px;inset"
        ":1em'><span>A</span></div><div id=b style='height:5px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_NOT_NULL(find_text("A"));
    TEST_ASSERT_EQUAL(20, find_text("A")->x);
    TEST_ASSERT_EQUAL(11, find_text("A")->y);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(30, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(100, lot52_node("p")->w);
    TEST_ASSERT_EQUAL(80, lot52_node("p")->h);
    TEST_ASSERT_TRUE(lot52_has_diag("position offset refused"));
}

static void test_l59_insets_atomic_13(void) {
    lot52_render("<body style='margin:0'><div id=p style='width:100px;height:80px;'><div style='height:20px'></div><di"
        "v id=a style='position:relative;width:20px;height:10px;margin-left:30px;right:10px;bottom:10px;inset"
        ":calc(calc(calc(calc(calc(1px)))))'><span>A</span></div><div id=b style='height:5px'></div></div></b"
        "ody>", 320);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_NOT_NULL(find_text("A"));
    TEST_ASSERT_EQUAL(20, find_text("A")->x);
    TEST_ASSERT_EQUAL(11, find_text("A")->y);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(30, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(100, lot52_node("p")->w);
    TEST_ASSERT_EQUAL(80, lot52_node("p")->h);
    TEST_ASSERT_TRUE(lot52_has_diag("position offset refused"));
}
