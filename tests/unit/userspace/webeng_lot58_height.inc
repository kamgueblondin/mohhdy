static void test_l58_height_height_fixed_sum(void) {
    lot52_render("<body style='margin:0'><div id=g style='width:200px;height:100px'>"
        "<div id=a style='height:10px;background:red;width:40px;height:calc(10px + 20px);'>A</div>"
        "<div id=b style='height:5px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(30, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(200, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(5, lot52_node("b")->h);
    TEST_ASSERT_EQUAL(100, lot52_node("g")->h);
    TEST_ASSERT_EQUAL(1, find_text("A")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l58_height_height_percent_sub(void) {
    lot52_render("<body style='margin:0'><div id=g style='width:200px;height:100px'>"
        "<div id=a style='height:10px;background:red;width:40px;height:calc(50% - 10px);'>A</div>"
        "<div id=b style='height:5px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(40, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(40, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(200, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(5, lot52_node("b")->h);
    TEST_ASSERT_EQUAL(100, lot52_node("g")->h);
    TEST_ASSERT_EQUAL(1, find_text("A")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l58_height_height_min(void) {
    lot52_render("<body style='margin:0'><div id=g style='width:200px;height:100px'>"
        "<div id=a style='height:10px;background:red;width:40px;height:min(60px,50%);'>A</div>"
        "<div id=b style='height:5px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(50, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(50, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(200, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(5, lot52_node("b")->h);
    TEST_ASSERT_EQUAL(100, lot52_node("g")->h);
    TEST_ASSERT_EQUAL(1, find_text("A")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l58_height_height_max(void) {
    lot52_render("<body style='margin:0'><div id=g style='width:200px;height:100px'>"
        "<div id=a style='height:10px;background:red;width:40px;height:max(20px,30%);'>A</div>"
        "<div id=b style='height:5px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(30, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(200, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(5, lot52_node("b")->h);
    TEST_ASSERT_EQUAL(100, lot52_node("g")->h);
    TEST_ASSERT_EQUAL(1, find_text("A")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l58_height_height_clamp(void) {
    lot52_render("<body style='margin:0'><div id=g style='width:200px;height:100px'>"
        "<div id=a style='height:10px;background:red;width:40px;height:clamp(20px,80%,60px);'>A</div>"
        "<div id=b style='height:5px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(60, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(60, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(200, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(5, lot52_node("b")->h);
    TEST_ASSERT_EQUAL(100, lot52_node("g")->h);
    TEST_ASSERT_EQUAL(1, find_text("A")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l58_height_height_negative(void) {
    lot52_render("<body style='margin:0'><div id=g style='width:200px;height:100px'>"
        "<div id=a style='height:10px;background:red;width:40px;height:calc(10px - 20px);'>A</div>"
        "<div id=b style='height:5px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(200, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(5, lot52_node("b")->h);
    TEST_ASSERT_EQUAL(100, lot52_node("g")->h);
    TEST_ASSERT_EQUAL(1, find_text("A")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l58_height_height_product(void) {
    lot52_render("<body style='margin:0'><div id=g style='width:200px;height:100px'>"
        "<div id=a style='height:10px;background:red;width:40px;height:calc(10px * 3);'>A</div>"
        "<div id=b style='height:5px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(30, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(200, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(5, lot52_node("b")->h);
    TEST_ASSERT_EQUAL(100, lot52_node("g")->h);
    TEST_ASSERT_EQUAL(1, find_text("A")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l58_height_height_division(void) {
    lot52_render("<body style='margin:0'><div id=g style='width:200px;height:100px'>"
        "<div id=a style='height:10px;background:red;width:40px;height:calc(100px / 4);'>A</div>"
        "<div id=b style='height:5px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(25, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(25, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(200, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(5, lot52_node("b")->h);
    TEST_ASSERT_EQUAL(100, lot52_node("g")->h);
    TEST_ASSERT_EQUAL(1, find_text("A")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l58_height_height_nested(void) {
    lot52_render("<body style='margin:0'><div id=g style='width:200px;height:100px'>"
        "<div id=a style='height:10px;background:red;width:40px;height:max(20px,min(60px,calc(80% - 10px)));'>A"
        "</div><div id=b style='height:5px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(60, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(60, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(200, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(5, lot52_node("b")->h);
    TEST_ASSERT_EQUAL(100, lot52_node("g")->h);
    TEST_ASSERT_EQUAL(1, find_text("A")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l58_height_height_zero_percent(void) {
    lot52_render("<body style='margin:0'><div id=g style='width:200px;height:100px'>"
        "<div id=a style='height:10px;background:red;width:40px;height:calc(0% + 20px);'>A</div>"
        "<div id=b style='height:5px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(20, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(200, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(5, lot52_node("b")->h);
    TEST_ASSERT_EQUAL(100, lot52_node("g")->h);
    TEST_ASSERT_EQUAL(1, find_text("A")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l58_height_minimum(void) {
    lot52_render("<body style='margin:0'><div id=g style='width:200px;height:100px'>"
        "<div id=a style='height:10px;background:red;width:40px;height:10px;min-height:calc(20px + 10px);'>A</div>"
        "<div id=b style='height:5px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(30, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(200, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(5, lot52_node("b")->h);
    TEST_ASSERT_EQUAL(100, lot52_node("g")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l58_height_maximum(void) {
    lot52_render("<body style='margin:0'><div id=g style='width:200px;height:100px'>"
        "<div id=a style='height:10px;background:red;width:40px;height:80px;max-height:calc(40px - 10px);'>A</div>"
        "<div id=b style='height:5px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(30, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(200, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(5, lot52_node("b")->h);
    TEST_ASSERT_EQUAL(100, lot52_node("g")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l58_height_minimum_percent(void) {
    lot52_render("<body style='margin:0'><div id=g style='width:200px;height:100px'>"
        "<div id=a style='height:10px;background:red;width:40px;height:10px;min-height:max(20px,50%);'>A</div>"
        "<div id=b style='height:5px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(50, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(50, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(200, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(5, lot52_node("b")->h);
    TEST_ASSERT_EQUAL(100, lot52_node("g")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l58_height_maximum_percent(void) {
    lot52_render("<body style='margin:0'><div id=g style='width:200px;height:100px'>"
        "<div id=a style='height:10px;background:red;width:40px;height:80px;max-height:min(70px,50%);'>A</div>"
        "<div id=b style='height:5px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(50, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(50, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(200, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(5, lot52_node("b")->h);
    TEST_ASSERT_EQUAL(100, lot52_node("g")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l58_height_inverted(void) {
    lot52_render("<body style='margin:0'><div id=g style='width:200px;height:100px'>"
        "<div id=a style='height:10px;background:red;width:40px;height:10px;min-height:calc(70px + 10px);max-height:calc(40px + 10px);'>"
        "A</div><div id=b style='height:5px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(80, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(80, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(200, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(5, lot52_node("b")->h);
    TEST_ASSERT_EQUAL(100, lot52_node("g")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l58_height_natural_minimum(void) {
    lot52_render("<body style='margin:0'><div id=g style='width:200px;height:100px'>"
        "<div id=a style='height:10px;background:red;width:40px;height:auto;min-height:calc(20px + 10px);'>A</div>"
        "<div id=b style='height:5px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(30, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(200, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(5, lot52_node("b")->h);
    TEST_ASSERT_EQUAL(100, lot52_node("g")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l58_height_natural_maximum(void) {
    lot52_render("<body style='margin:0'><div id=g style='width:200px;height:100px'>"
        "<div id=a style='height:10px;background:red;width:40px;height:auto;max-height:calc(10px - 5px);'>A</div>"
        "<div id=b style='height:5px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(5, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(5, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(200, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(5, lot52_node("b")->h);
    TEST_ASSERT_EQUAL(100, lot52_node("g")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l58_height_negative_maximum(void) {
    lot52_render("<body style='margin:0'><div id=g style='width:200px;height:100px'>"
        "<div id=a style='height:10px;background:red;width:40px;height:40px;max-height:calc(-10px);'>A</div>"
        "<div id=b style='height:5px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(200, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(5, lot52_node("b")->h);
    TEST_ASSERT_EQUAL(100, lot52_node("g")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l58_height_minimum_reset(void) {
    lot52_render("<body style='margin:0'><div id=g style='width:200px;height:100px'>"
        "<div id=a style='height:10px;background:red;width:40px;height:10px;min-height:calc(20px + 10px);min-height:20px;'>"
        "A</div><div id=b style='height:5px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(20, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(200, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(5, lot52_node("b")->h);
    TEST_ASSERT_EQUAL(100, lot52_node("g")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l58_height_maximum_reset(void) {
    lot52_render("<body style='margin:0'><div id=g style='width:200px;height:100px'>"
        "<div id=a style='height:10px;background:red;width:40px;height:80px;max-height:calc(40px - 10px);max-height:20px;'>"
        "A</div><div id=b style='height:5px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(20, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(200, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(5, lot52_node("b")->h);
    TEST_ASSERT_EQUAL(100, lot52_node("g")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l58_height_height_reset(void) {
    lot52_render("<body style='margin:0'><div id=g style='width:200px;height:100px'>"
        "<div id=a style='height:10px;background:red;width:40px;height:calc(40px + 10px);height:20px;'>A</div>"
        "<div id=b style='height:5px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(20, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(200, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(5, lot52_node("b")->h);
    TEST_ASSERT_EQUAL(100, lot52_node("g")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l58_height_height_auto_reset(void) {
    lot52_render("<body style='margin:0'><div id=g style='width:200px;height:100px'>"
        "<div id=a style='height:10px;background:red;width:40px;height:calc(40px + 10px);height:auto;'>A</div>"
        "<div id=b style='height:5px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(200, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(5, lot52_node("b")->h);
    TEST_ASSERT_EQUAL(100, lot52_node("g")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l58_height_indefinite_height(void) {
    lot52_render("<body style='margin:0'><div id=g style='width:200px'>"
        "<div id=a style='height:10px;background:red;width:40px;height:calc(50% - 10px);'>A</div>"
        "<div id=b style='height:5px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(200, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(5, lot52_node("b")->h);
    TEST_ASSERT_TRUE(lot52_has_diag("percent height indefinite"));
    TEST_ASSERT_TRUE(lot52_has_diag("math evaluation refused"));
}

static void test_l58_height_indefinite_min_height(void) {
    lot52_render("<body style='margin:0'><div id=g style='width:200px'>"
        "<div id=a style='height:10px;background:red;width:40px;min-height:calc(50% - 10px);'>A</div>"
        "<div id=b style='height:5px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(200, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(5, lot52_node("b")->h);
    TEST_ASSERT_TRUE(lot52_has_diag("percent height indefinite"));
    TEST_ASSERT_TRUE(lot52_has_diag("math evaluation refused"));
}

static void test_l58_height_indefinite_max_height(void) {
    lot52_render("<body style='margin:0'><div id=g style='width:200px'>"
        "<div id=a style='height:10px;background:red;width:40px;max-height:calc(50% - 10px);'>A</div>"
        "<div id=b style='height:5px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(200, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(5, lot52_node("b")->h);
    TEST_ASSERT_TRUE(lot52_has_diag("percent height indefinite"));
    TEST_ASSERT_TRUE(lot52_has_diag("math evaluation refused"));
}
