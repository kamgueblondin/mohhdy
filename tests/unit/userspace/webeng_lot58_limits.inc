static void test_l58_limits_min_px(void) {
    lot52_render("<body style='margin:0'><div id=g style='width:200px'>"
        "<div id=a style='height:10px;background:red;width:20px;min-width:calc(20px + 30px);'>A</div>"
        "<div id=b style='height:5px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(50, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(200, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(5, lot52_node("b")->h);
    TEST_ASSERT_EQUAL(15, lot52_node("g")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l58_limits_min_percent(void) {
    lot52_render("<body style='margin:0'><div id=g style='width:200px'>"
        "<div id=a style='height:10px;background:red;width:20px;min-width:max(20px,50%);'>A</div>"
        "<div id=b style='height:5px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(100, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(200, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(5, lot52_node("b")->h);
    TEST_ASSERT_EQUAL(15, lot52_node("g")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l58_limits_max_px(void) {
    lot52_render("<body style='margin:0'><div id=g style='width:200px'>"
        "<div id=a style='height:10px;background:red;width:150px;max-width:calc(100px - 20px);'>A</div>"
        "<div id=b style='height:5px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(80, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(200, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(5, lot52_node("b")->h);
    TEST_ASSERT_EQUAL(15, lot52_node("g")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l58_limits_max_percent(void) {
    lot52_render("<body style='margin:0'><div id=g style='width:200px'>"
        "<div id=a style='height:10px;background:red;width:150px;max-width:min(120px,50%);'>A</div>"
        "<div id=b style='height:5px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(100, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(200, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(5, lot52_node("b")->h);
    TEST_ASSERT_EQUAL(15, lot52_node("g")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l58_limits_min_beats_max(void) {
    lot52_render("<body style='margin:0'><div id=g style='width:200px'>"
        "<div id=a style='height:10px;background:red;width:10px;min-width:calc(100px - 10px);max-width:calc(50px + 10px);'>"
        "A</div><div id=b style='height:5px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(90, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(200, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(5, lot52_node("b")->h);
    TEST_ASSERT_EQUAL(15, lot52_node("g")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l58_limits_negative_min(void) {
    lot52_render("<body style='margin:0'><div id=g style='width:200px'>"
        "<div id=a style='height:10px;background:red;width:40px;min-width:calc(10px - 20px);'>A</div>"
        "<div id=b style='height:5px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(200, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(5, lot52_node("b")->h);
    TEST_ASSERT_EQUAL(15, lot52_node("g")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l58_limits_negative_max(void) {
    lot52_render("<body style='margin:0'><div id=g style='width:200px'>"
        "<div id=a style='height:10px;background:red;width:40px;max-width:calc(10px - 20px);'>A</div>"
        "<div id=b style='height:5px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(200, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(5, lot52_node("b")->h);
    TEST_ASSERT_EQUAL(15, lot52_node("g")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l58_limits_limit_shrink_content(void) {
    lot52_render("<body style='margin:0'><div id=g style='width:200px'>"
        "<div id=a style='height:10px;background:red;width:max-content;max-width:calc(4px + 2px);'>A</div>"
        "<div id=b style='height:5px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(6, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(200, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(5, lot52_node("b")->h);
    TEST_ASSERT_EQUAL(15, lot52_node("g")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l58_limits_min_none_clears(void) {
    lot52_render("<body style='margin:0'><div id=g style='width:200px'>"
        "<div id=a style='height:10px;background:red;width:20px;min-width:calc(20px + 30px);min-width:none;'>A"
        "</div><div id=b style='height:5px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(200, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(5, lot52_node("b")->h);
    TEST_ASSERT_EQUAL(15, lot52_node("g")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l58_limits_max_none_clears(void) {
    lot52_render("<body style='margin:0'><div id=g style='width:200px'>"
        "<div id=a style='height:10px;background:red;width:150px;max-width:calc(50px + 10px);max-width:none;'>A"
        "</div><div id=b style='height:5px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(150, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(200, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(5, lot52_node("b")->h);
    TEST_ASSERT_EQUAL(15, lot52_node("g")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l58_limits_min_numeric_clears(void) {
    lot52_render("<body style='margin:0'><div id=g style='width:200px'>"
        "<div id=a style='height:10px;background:red;width:20px;min-width:calc(20px + 30px);min-width:30px;'>A"
        "</div><div id=b style='height:5px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(200, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(5, lot52_node("b")->h);
    TEST_ASSERT_EQUAL(15, lot52_node("g")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l58_limits_max_numeric_clears(void) {
    lot52_render("<body style='margin:0'><div id=g style='width:200px'>"
        "<div id=a style='height:10px;background:red;width:150px;max-width:calc(50px + 10px);max-width:80px;'>A"
        "</div><div id=b style='height:5px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(80, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(200, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(5, lot52_node("b")->h);
    TEST_ASSERT_EQUAL(15, lot52_node("g")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l58_limits_content_box(void) {
    lot52_render("<body style='margin:0'><div id=g style='width:200px'>"
        "<div id=a style='height:10px;background:red;width:calc(100px - 20px);padding:4px;border:2px solid blue;'>"
        "A</div><div id=b style='height:5px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(92, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(22, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(22, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(200, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(5, lot52_node("b")->h);
    TEST_ASSERT_EQUAL(6, find_text("A")->x);
    TEST_ASSERT_EQUAL(7, find_text("A")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l58_limits_border_box(void) {
    lot52_render("<body style='margin:0'><div id=g style='width:200px'>"
        "<div id=a style='height:10px;background:red;width:calc(100px - 20px);padding:4px;border:2px solid blue;box-sizing:border-box;height:22px;'>"
        "A</div><div id=b style='height:5px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(80, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(22, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(22, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(200, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(5, lot52_node("b")->h);
    TEST_ASSERT_EQUAL(6, find_text("A")->x);
    TEST_ASSERT_EQUAL(7, find_text("A")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l58_limits_border_min(void) {
    lot52_render("<body style='margin:0'><div id=g style='width:200px'>"
        "<div id=a style='height:10px;background:red;width:20px;min-width:calc(40px + 20px);padding:4px;border:2px solid blue;box-sizing:border-box;height:22px;'>"
        "A</div><div id=b style='height:5px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(60, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(22, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(22, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(200, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(5, lot52_node("b")->h);
    TEST_ASSERT_EQUAL(6, find_text("A")->x);
    TEST_ASSERT_EQUAL(7, find_text("A")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l58_limits_border_max(void) {
    lot52_render("<body style='margin:0'><div id=g style='width:200px'>"
        "<div id=a style='height:10px;background:red;width:100px;max-width:calc(40px + 20px);padding:4px;border:2px solid blue;box-sizing:border-box;height:22px;'>"
        "A</div><div id=b style='height:5px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(60, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(22, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(22, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(200, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(5, lot52_node("b")->h);
    TEST_ASSERT_EQUAL(6, find_text("A")->x);
    TEST_ASSERT_EQUAL(7, find_text("A")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l58_limits_percent_border_box(void) {
    lot52_render("<body style='margin:0'><div id=g style='width:200px'>"
        "<div id=a style='height:10px;background:red;width:calc(50% - 20px);padding:4px;border:2px solid blue;box-sizing:border-box;height:22px;'>"
        "A</div><div id=b style='height:5px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(80, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(22, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(22, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(200, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(5, lot52_node("b")->h);
    TEST_ASSERT_EQUAL(6, find_text("A")->x);
    TEST_ASSERT_EQUAL(7, find_text("A")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l58_limits_zero_border_content(void) {
    lot52_render("<body style='margin:0'><div id=g style='width:200px'>"
        "<div id=a style='height:10px;background:red;width:calc(0px);padding:4px;border:2px solid blue;box-sizing:border-box;height:22px;'>"
        "A</div><div id=b style='height:5px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(12, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(22, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(22, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(200, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(5, lot52_node("b")->h);
    TEST_ASSERT_EQUAL(6, find_text("A")->x);
    TEST_ASSERT_EQUAL(7, find_text("A")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}
