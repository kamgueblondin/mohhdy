static void test_l58_ratio_square(void) {
    lot52_render("<body style='margin:0'><div id=g style='width:200px'>"
        "<div id=a style='background:red;width:40px;aspect-ratio:1'></div><div id=b style='height:5px'></div>"
        "</div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(40, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(40, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(200, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(5, lot52_node("b")->h);
    TEST_ASSERT_EQUAL(45, lot52_node("g")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l58_ratio_wide(void) {
    lot52_render("<body style='margin:0'><div id=g style='width:200px'>"
        "<div id=a style='background:red;width:80px;aspect-ratio:2 / 1'></div><div id=b style='height:5px'></div>"
        "</div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(80, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(40, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(40, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(200, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(5, lot52_node("b")->h);
    TEST_ASSERT_EQUAL(45, lot52_node("g")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l58_ratio_tall(void) {
    lot52_render("<body style='margin:0'><div id=g style='width:200px'>"
        "<div id=a style='background:red;width:40px;aspect-ratio:1 / 2'></div><div id=b style='height:5px'></div>"
        "</div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(80, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(80, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(200, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(5, lot52_node("b")->h);
    TEST_ASSERT_EQUAL(85, lot52_node("g")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l58_ratio_rounded(void) {
    lot52_render("<body style='margin:0'><div id=g style='width:200px'>"
        "<div id=a style='background:red;width:101px;aspect-ratio:3 / 2'></div><div id=b style='height:5px'></div>"
        "</div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(101, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(67, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(67, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(200, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(5, lot52_node("b")->h);
    TEST_ASSERT_EQUAL(72, lot52_node("g")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l58_ratio_ratio_width_math(void) {
    lot52_render("<body style='margin:0'><div id=g style='width:200px'>"
        "<div id=a style='background:red;width:calc(50% - 20px);aspect-ratio:2'></div>"
        "<div id=b style='height:5px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(80, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(40, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(40, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(200, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(5, lot52_node("b")->h);
    TEST_ASSERT_EQUAL(45, lot52_node("g")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l58_ratio_ratio_min_width(void) {
    lot52_render("<body style='margin:0'><div id=g style='width:200px'>"
        "<div id=a style='background:red;width:20px;min-width:60px;aspect-ratio:2'></div>"
        "<div id=b style='height:5px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(60, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(30, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(200, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(5, lot52_node("b")->h);
    TEST_ASSERT_EQUAL(35, lot52_node("g")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l58_ratio_ratio_max_width(void) {
    lot52_render("<body style='margin:0'><div id=g style='width:200px'>"
        "<div id=a style='background:red;width:100px;max-width:60px;aspect-ratio:2'></div>"
        "<div id=b style='height:5px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(60, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(30, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(200, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(5, lot52_node("b")->h);
    TEST_ASSERT_EQUAL(35, lot52_node("g")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l58_ratio_ratio_min_height(void) {
    lot52_render("<body style='margin:0'><div id=g style='width:200px'>"
        "<div id=a style='background:red;width:80px;min-height:60px;aspect-ratio:2'></div>"
        "<div id=b style='height:5px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(80, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(60, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(60, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(200, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(5, lot52_node("b")->h);
    TEST_ASSERT_EQUAL(65, lot52_node("g")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l58_ratio_ratio_max_height(void) {
    lot52_render("<body style='margin:0'><div id=g style='width:200px'>"
        "<div id=a style='background:red;width:80px;max-height:20px;aspect-ratio:2'></div>"
        "<div id=b style='height:5px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(80, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(20, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(200, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(5, lot52_node("b")->h);
    TEST_ASSERT_EQUAL(25, lot52_node("g")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l58_ratio_ratio_min_math(void) {
    lot52_render("<body style='margin:0'><div id=g style='width:200px'>"
        "<div id=a style='background:red;width:80px;min-height:calc(50px + 10px);aspect-ratio:2'></div>"
        "<div id=b style='height:5px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(80, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(60, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(60, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(200, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(5, lot52_node("b")->h);
    TEST_ASSERT_EQUAL(65, lot52_node("g")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l58_ratio_ratio_max_math(void) {
    lot52_render("<body style='margin:0'><div id=g style='width:200px'>"
        "<div id=a style='background:red;width:80px;max-height:calc(10px + 10px);aspect-ratio:2'></div>"
        "<div id=b style='height:5px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(80, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(20, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(200, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(5, lot52_node("b")->h);
    TEST_ASSERT_EQUAL(25, lot52_node("g")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l58_ratio_ratio_explicit_height(void) {
    lot52_render("<body style='margin:0'><div id=g style='width:200px'>"
        "<div id=a style='background:red;width:80px;height:15px;aspect-ratio:2'></div>"
        "<div id=b style='height:5px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(80, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(15, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(15, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(200, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(5, lot52_node("b")->h);
    TEST_ASSERT_EQUAL(20, lot52_node("g")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l58_ratio_ratio_math_height(void) {
    lot52_render("<body style='margin:0'><div id=g style='width:200px'>"
        "<div id=a style='background:red;width:80px;height:calc(10px + 5px);aspect-ratio:2'></div>"
        "<div id=b style='height:5px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(80, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(15, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(15, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(200, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(5, lot52_node("b")->h);
    TEST_ASSERT_EQUAL(20, lot52_node("g")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l58_ratio_ratio_auto_reset(void) {
    lot52_render("<body style='margin:0'><div id=g style='width:200px'>"
        "<div id=a style='background:red;width:80px;aspect-ratio:2;aspect-ratio:auto'></div>"
        "<div id=b style='height:5px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(80, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(200, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(5, lot52_node("b")->h);
    TEST_ASSERT_EQUAL(5, lot52_node("g")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l58_ratio_ratio_zero_width(void) {
    lot52_render("<body style='margin:0'><div id=g style='width:200px'>"
        "<div id=a style='background:red;width:0;aspect-ratio:2'></div><div id=b style='height:5px'></div></div>"
        "</body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(200, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(5, lot52_node("b")->h);
    TEST_ASSERT_EQUAL(5, lot52_node("g")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l58_ratio_ratio_border_content(void) {
    lot52_render("<body style='margin:0'><div id=g style='width:200px'>"
        "<div id=a style='background:red;width:80px;aspect-ratio:2;padding:4px;border:2px solid red'></div>"
        "<div id=b style='height:5px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(92, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(52, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(52, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(200, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(5, lot52_node("b")->h);
    TEST_ASSERT_EQUAL(57, lot52_node("g")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l58_ratio_ratio_border_box(void) {
    lot52_render("<body style='margin:0'><div id=g style='width:200px'>"
        "<div id=a style='background:red;width:80px;aspect-ratio:2;padding:4px;border:2px solid red;box-sizing:border-box'>"
        "</div><div id=b style='height:5px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(80, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(40, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(40, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(200, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(5, lot52_node("b")->h);
    TEST_ASSERT_EQUAL(45, lot52_node("g")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l58_ratio_ratio_auto_width(void) {
    lot52_render("<body style='margin:0'><div id=g style='width:200px'><div id=a style='background:red;aspect-ratio:2'>"
        "</div><div id=b style='height:5px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(200, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(100, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(100, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(200, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(5, lot52_node("b")->h);
    TEST_ASSERT_EQUAL(105, lot52_node("g")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l58_ratio_ratio_maximum(void) {
    lot52_render("<body style='margin:0'><div id=g style='width:200px'>"
        "<div id=a style='background:red;width:2000px;aspect-ratio:1 / 2000'></div><div id=b style='height:5px'>"
        "</div></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(2000, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(16000, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(16000, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(200, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(5, lot52_node("b")->h);
    TEST_ASSERT_EQUAL(16000, lot52_node("g")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l58_ratio_ratio_minimum(void) {
    lot52_render("<body style='margin:0'><div id=g style='width:200px'>"
        "<div id=a style='background:red;width:2000px;aspect-ratio:2000 / 1'></div><div id=b style='height:5px'>"
        "</div></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(2000, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(1, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(1, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(200, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(5, lot52_node("b")->h);
    TEST_ASSERT_EQUAL(6, lot52_node("g")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l58_ratio_invalid_ratio_20(void) {
    lot52_render("<body style='margin:0'><div id=a style='width:80px;aspect-ratio:2;aspect-ratio:0'></div>"
        "<div id=b style='height:5px'></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(80, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(40, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(40, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(320, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(5, lot52_node("b")->h);
    TEST_ASSERT_TRUE(lot52_has_diag("aspect ratio refused"));
    TEST_ASSERT_EQUAL(2, lot52_node("a")->aspect_num);
    TEST_ASSERT_EQUAL(1, lot52_node("a")->aspect_den);
}

static void test_l58_ratio_invalid_ratio_21(void) {
    lot52_render("<body style='margin:0'><div id=a style='width:80px;aspect-ratio:2;aspect-ratio:1 / 0'></div>"
        "<div id=b style='height:5px'></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(80, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(40, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(40, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(320, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(5, lot52_node("b")->h);
    TEST_ASSERT_TRUE(lot52_has_diag("aspect ratio refused"));
    TEST_ASSERT_EQUAL(2, lot52_node("a")->aspect_num);
    TEST_ASSERT_EQUAL(1, lot52_node("a")->aspect_den);
}

static void test_l58_ratio_invalid_ratio_22(void) {
    lot52_render("<body style='margin:0'><div id=a style='width:80px;aspect-ratio:2;aspect-ratio:-1 / 2'></div>"
        "<div id=b style='height:5px'></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(80, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(40, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(40, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(320, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(5, lot52_node("b")->h);
    TEST_ASSERT_TRUE(lot52_has_diag("aspect ratio refused"));
    TEST_ASSERT_EQUAL(2, lot52_node("a")->aspect_num);
    TEST_ASSERT_EQUAL(1, lot52_node("a")->aspect_den);
}

static void test_l58_ratio_invalid_ratio_23(void) {
    lot52_render("<body style='margin:0'><div id=a style='width:80px;aspect-ratio:2;aspect-ratio:1.5'></div>"
        "<div id=b style='height:5px'></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(80, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(40, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(40, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(320, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(5, lot52_node("b")->h);
    TEST_ASSERT_TRUE(lot52_has_diag("aspect ratio refused"));
    TEST_ASSERT_EQUAL(2, lot52_node("a")->aspect_num);
    TEST_ASSERT_EQUAL(1, lot52_node("a")->aspect_den);
}

static void test_l58_ratio_invalid_ratio_24(void) {
    lot52_render("<body style='margin:0'><div id=a style='width:80px;aspect-ratio:2;aspect-ratio:2001 / 1'></div>"
        "<div id=b style='height:5px'></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(80, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(40, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(40, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(320, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(5, lot52_node("b")->h);
    TEST_ASSERT_TRUE(lot52_has_diag("aspect ratio refused"));
    TEST_ASSERT_EQUAL(2, lot52_node("a")->aspect_num);
    TEST_ASSERT_EQUAL(1, lot52_node("a")->aspect_den);
}

static void test_l58_ratio_invalid_ratio_25(void) {
    lot52_render("<body style='margin:0'><div id=a style='width:80px;aspect-ratio:2;aspect-ratio:1 / 2001'></div>"
        "<div id=b style='height:5px'></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(80, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(40, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(40, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(320, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(5, lot52_node("b")->h);
    TEST_ASSERT_TRUE(lot52_has_diag("aspect ratio refused"));
    TEST_ASSERT_EQUAL(2, lot52_node("a")->aspect_num);
    TEST_ASSERT_EQUAL(1, lot52_node("a")->aspect_den);
}

static void test_l58_ratio_invalid_ratio_26(void) {
    lot52_render("<body style='margin:0'><div id=a style='width:80px;aspect-ratio:2;aspect-ratio:auto 2 / 1'></div>"
        "<div id=b style='height:5px'></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(80, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(40, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(40, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(320, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(5, lot52_node("b")->h);
    TEST_ASSERT_TRUE(lot52_has_diag("aspect ratio refused"));
    TEST_ASSERT_EQUAL(2, lot52_node("a")->aspect_num);
    TEST_ASSERT_EQUAL(1, lot52_node("a")->aspect_den);
}

static void test_l58_ratio_invalid_ratio_27(void) {
    lot52_render("<body style='margin:0'><div id=a style='width:80px;aspect-ratio:2;aspect-ratio:2 / 1 junk'></div>"
        "<div id=b style='height:5px'></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(80, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(40, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(40, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(320, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(5, lot52_node("b")->h);
    TEST_ASSERT_TRUE(lot52_has_diag("aspect ratio refused"));
    TEST_ASSERT_EQUAL(2, lot52_node("a")->aspect_num);
    TEST_ASSERT_EQUAL(1, lot52_node("a")->aspect_den);
}

static void test_l58_ratio_invalid_ratio_28(void) {
    lot52_render("<body style='margin:0'><div id=a style='width:80px;aspect-ratio:2;aspect-ratio:1 /'></div>"
        "<div id=b style='height:5px'></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(80, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(40, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(40, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(320, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(5, lot52_node("b")->h);
    TEST_ASSERT_TRUE(lot52_has_diag("aspect ratio refused"));
    TEST_ASSERT_EQUAL(2, lot52_node("a")->aspect_num);
    TEST_ASSERT_EQUAL(1, lot52_node("a")->aspect_den);
}

static void test_l58_ratio_invalid_ratio_29(void) {
    lot52_render("<body style='margin:0'><div id=a style='width:80px;aspect-ratio:2;aspect-ratio:1 / 2 / 3'></div>"
        "<div id=b style='height:5px'></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(80, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(40, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(40, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(320, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(5, lot52_node("b")->h);
    TEST_ASSERT_TRUE(lot52_has_diag("aspect ratio refused"));
    TEST_ASSERT_EQUAL(2, lot52_node("a")->aspect_num);
    TEST_ASSERT_EQUAL(1, lot52_node("a")->aspect_den);
}
