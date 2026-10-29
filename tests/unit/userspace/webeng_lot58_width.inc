static void test_l58_width_sum_px(void) {
    lot52_render("<body style='margin:0'><div id=g style='width:200px'>"
        "<div id=a style='height:10px;background:red;width:calc(20px + 30px);'>A</div>"
        "<div id=b style='height:5px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(50, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(200, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(5, lot52_node("b")->h);
    TEST_ASSERT_EQUAL(0, find_text("A")->x);
    TEST_ASSERT_EQUAL(1, find_text("A")->y);
    TEST_ASSERT_EQUAL(15, lot52_node("g")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
    TEST_ASSERT_TRUE(lot52_node("a")->length[WE_LEN_WIDTH].count > 0);
    TEST_ASSERT_TRUE(lot52_node("a")->length[WE_LEN_WIDTH].count <= WE_MATH_NODES_MAX);
}

static void test_l58_width_sub_px(void) {
    lot52_render("<body style='margin:0'><div id=g style='width:200px'>"
        "<div id=a style='height:10px;background:red;width:calc(100px - 20px);'>A</div>"
        "<div id=b style='height:5px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(80, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(200, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(5, lot52_node("b")->h);
    TEST_ASSERT_EQUAL(0, find_text("A")->x);
    TEST_ASSERT_EQUAL(1, find_text("A")->y);
    TEST_ASSERT_EQUAL(15, lot52_node("g")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
    TEST_ASSERT_TRUE(lot52_node("a")->length[WE_LEN_WIDTH].count > 0);
    TEST_ASSERT_TRUE(lot52_node("a")->length[WE_LEN_WIDTH].count <= WE_MATH_NODES_MAX);
}

static void test_l58_width_sum_percent(void) {
    lot52_render("<body style='margin:0'><div id=g style='width:200px'>"
        "<div id=a style='height:10px;background:red;width:calc(25% + 10px);'>A</div><div id=b style='height:5px'>"
        "</div></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(60, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(200, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(5, lot52_node("b")->h);
    TEST_ASSERT_EQUAL(0, find_text("A")->x);
    TEST_ASSERT_EQUAL(1, find_text("A")->y);
    TEST_ASSERT_EQUAL(15, lot52_node("g")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
    TEST_ASSERT_TRUE(lot52_node("a")->length[WE_LEN_WIDTH].count > 0);
    TEST_ASSERT_TRUE(lot52_node("a")->length[WE_LEN_WIDTH].count <= WE_MATH_NODES_MAX);
}

static void test_l58_width_subtract_from_full(void) {
    lot52_render("<body style='margin:0'><div id=g style='width:200px'>"
        "<div id=a style='height:10px;background:red;width:calc(100% - 20px);'>A</div>"
        "<div id=b style='height:5px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(180, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(200, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(5, lot52_node("b")->h);
    TEST_ASSERT_EQUAL(0, find_text("A")->x);
    TEST_ASSERT_EQUAL(1, find_text("A")->y);
    TEST_ASSERT_EQUAL(15, lot52_node("g")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
    TEST_ASSERT_TRUE(lot52_node("a")->length[WE_LEN_WIDTH].count > 0);
    TEST_ASSERT_TRUE(lot52_node("a")->length[WE_LEN_WIDTH].count <= WE_MATH_NODES_MAX);
}

static void test_l58_width_min_percent(void) {
    lot52_render("<body style='margin:0'><div id=g style='width:200px'>"
        "<div id=a style='height:10px;background:red;width:min(120px,50%);'>A</div><div id=b style='height:5px'>"
        "</div></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(100, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(200, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(5, lot52_node("b")->h);
    TEST_ASSERT_EQUAL(0, find_text("A")->x);
    TEST_ASSERT_EQUAL(1, find_text("A")->y);
    TEST_ASSERT_EQUAL(15, lot52_node("g")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
    TEST_ASSERT_TRUE(lot52_node("a")->length[WE_LEN_WIDTH].count > 0);
    TEST_ASSERT_TRUE(lot52_node("a")->length[WE_LEN_WIDTH].count <= WE_MATH_NODES_MAX);
}

static void test_l58_width_max_percent(void) {
    lot52_render("<body style='margin:0'><div id=g style='width:200px'>"
        "<div id=a style='height:10px;background:red;width:max(20px,25%);'>A</div><div id=b style='height:5px'>"
        "</div></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(50, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(200, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(5, lot52_node("b")->h);
    TEST_ASSERT_EQUAL(0, find_text("A")->x);
    TEST_ASSERT_EQUAL(1, find_text("A")->y);
    TEST_ASSERT_EQUAL(15, lot52_node("g")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
    TEST_ASSERT_TRUE(lot52_node("a")->length[WE_LEN_WIDTH].count > 0);
    TEST_ASSERT_TRUE(lot52_node("a")->length[WE_LEN_WIDTH].count <= WE_MATH_NODES_MAX);
}

static void test_l58_width_clamp_high(void) {
    lot52_render("<body style='margin:0'><div id=g style='width:200px'>"
        "<div id=a style='height:10px;background:red;width:clamp(20px,50%,80px);'>A</div>"
        "<div id=b style='height:5px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(80, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(200, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(5, lot52_node("b")->h);
    TEST_ASSERT_EQUAL(0, find_text("A")->x);
    TEST_ASSERT_EQUAL(1, find_text("A")->y);
    TEST_ASSERT_EQUAL(15, lot52_node("g")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
    TEST_ASSERT_TRUE(lot52_node("a")->length[WE_LEN_WIDTH].count > 0);
    TEST_ASSERT_TRUE(lot52_node("a")->length[WE_LEN_WIDTH].count <= WE_MATH_NODES_MAX);
}

static void test_l58_width_clamp_low(void) {
    lot52_render("<body style='margin:0'><div id=g style='width:200px'>"
        "<div id=a style='height:10px;background:red;width:clamp(80px,20px,120px);'>A</div>"
        "<div id=b style='height:5px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(80, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(200, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(5, lot52_node("b")->h);
    TEST_ASSERT_EQUAL(0, find_text("A")->x);
    TEST_ASSERT_EQUAL(1, find_text("A")->y);
    TEST_ASSERT_EQUAL(15, lot52_node("g")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
    TEST_ASSERT_TRUE(lot52_node("a")->length[WE_LEN_WIDTH].count > 0);
    TEST_ASSERT_TRUE(lot52_node("a")->length[WE_LEN_WIDTH].count <= WE_MATH_NODES_MAX);
}

static void test_l58_width_clamp_inverted(void) {
    lot52_render("<body style='margin:0'><div id=g style='width:200px'>"
        "<div id=a style='height:10px;background:red;width:clamp(80px,20px,40px);'>A</div>"
        "<div id=b style='height:5px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(80, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(200, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(5, lot52_node("b")->h);
    TEST_ASSERT_EQUAL(0, find_text("A")->x);
    TEST_ASSERT_EQUAL(1, find_text("A")->y);
    TEST_ASSERT_EQUAL(15, lot52_node("g")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
    TEST_ASSERT_TRUE(lot52_node("a")->length[WE_LEN_WIDTH].count > 0);
    TEST_ASSERT_TRUE(lot52_node("a")->length[WE_LEN_WIDTH].count <= WE_MATH_NODES_MAX);
}

static void test_l58_width_clamp_mid(void) {
    lot52_render("<body style='margin:0'><div id=g style='width:200px'>"
        "<div id=a style='height:10px;background:red;width:clamp(20px,60px,80px);'>A</div>"
        "<div id=b style='height:5px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(60, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(200, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(5, lot52_node("b")->h);
    TEST_ASSERT_EQUAL(0, find_text("A")->x);
    TEST_ASSERT_EQUAL(1, find_text("A")->y);
    TEST_ASSERT_EQUAL(15, lot52_node("g")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
    TEST_ASSERT_TRUE(lot52_node("a")->length[WE_LEN_WIDTH].count > 0);
    TEST_ASSERT_TRUE(lot52_node("a")->length[WE_LEN_WIDTH].count <= WE_MATH_NODES_MAX);
}

static void test_l58_width_product_right(void) {
    lot52_render("<body style='margin:0'><div id=g style='width:200px'>"
        "<div id=a style='height:10px;background:red;width:calc(20px * 3);'>A</div><div id=b style='height:5px'>"
        "</div></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(60, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(200, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(5, lot52_node("b")->h);
    TEST_ASSERT_EQUAL(0, find_text("A")->x);
    TEST_ASSERT_EQUAL(1, find_text("A")->y);
    TEST_ASSERT_EQUAL(15, lot52_node("g")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
    TEST_ASSERT_TRUE(lot52_node("a")->length[WE_LEN_WIDTH].count > 0);
    TEST_ASSERT_TRUE(lot52_node("a")->length[WE_LEN_WIDTH].count <= WE_MATH_NODES_MAX);
}

static void test_l58_width_product_left(void) {
    lot52_render("<body style='margin:0'><div id=g style='width:200px'>"
        "<div id=a style='height:10px;background:red;width:calc(3 * 20px);'>A</div><div id=b style='height:5px'>"
        "</div></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(60, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(200, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(5, lot52_node("b")->h);
    TEST_ASSERT_EQUAL(0, find_text("A")->x);
    TEST_ASSERT_EQUAL(1, find_text("A")->y);
    TEST_ASSERT_EQUAL(15, lot52_node("g")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
    TEST_ASSERT_TRUE(lot52_node("a")->length[WE_LEN_WIDTH].count > 0);
    TEST_ASSERT_TRUE(lot52_node("a")->length[WE_LEN_WIDTH].count <= WE_MATH_NODES_MAX);
}

static void test_l58_width_division(void) {
    lot52_render("<body style='margin:0'><div id=g style='width:200px'>"
        "<div id=a style='height:10px;background:red;width:calc(100px / 4);'>A</div><div id=b style='height:5px'>"
        "</div></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(25, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(200, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(5, lot52_node("b")->h);
    TEST_ASSERT_EQUAL(0, find_text("A")->x);
    TEST_ASSERT_EQUAL(1, find_text("A")->y);
    TEST_ASSERT_EQUAL(15, lot52_node("g")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
    TEST_ASSERT_TRUE(lot52_node("a")->length[WE_LEN_WIDTH].count > 0);
    TEST_ASSERT_TRUE(lot52_node("a")->length[WE_LEN_WIDTH].count <= WE_MATH_NODES_MAX);
}

static void test_l58_width_integer_rounding(void) {
    lot52_render("<body style='margin:0'><div id=g style='width:200px'>"
        "<div id=a style='height:10px;background:red;width:calc(101px / 4);'>A</div><div id=b style='height:5px'>"
        "</div></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(25, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(200, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(5, lot52_node("b")->h);
    TEST_ASSERT_EQUAL(0, find_text("A")->x);
    TEST_ASSERT_EQUAL(1, find_text("A")->y);
    TEST_ASSERT_EQUAL(15, lot52_node("g")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
    TEST_ASSERT_TRUE(lot52_node("a")->length[WE_LEN_WIDTH].count > 0);
    TEST_ASSERT_TRUE(lot52_node("a")->length[WE_LEN_WIDTH].count <= WE_MATH_NODES_MAX);
}

static void test_l58_width_nested_number(void) {
    lot52_render("<body style='margin:0'><div id=g style='width:200px'>"
        "<div id=a style='height:10px;background:red;width:calc(20px * (2 + 1));'>A</div>"
        "<div id=b style='height:5px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(60, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(200, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(5, lot52_node("b")->h);
    TEST_ASSERT_EQUAL(0, find_text("A")->x);
    TEST_ASSERT_EQUAL(1, find_text("A")->y);
    TEST_ASSERT_EQUAL(15, lot52_node("g")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
    TEST_ASSERT_TRUE(lot52_node("a")->length[WE_LEN_WIDTH].count > 0);
    TEST_ASSERT_TRUE(lot52_node("a")->length[WE_LEN_WIDTH].count <= WE_MATH_NODES_MAX);
}

static void test_l58_width_nested_lengths(void) {
    lot52_render("<body style='margin:0'><div id=g style='width:200px'>"
        "<div id=a style='height:10px;background:red;width:calc((20px + 10px) * 2);'>A</div>"
        "<div id=b style='height:5px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(60, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(200, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(5, lot52_node("b")->h);
    TEST_ASSERT_EQUAL(0, find_text("A")->x);
    TEST_ASSERT_EQUAL(1, find_text("A")->y);
    TEST_ASSERT_EQUAL(15, lot52_node("g")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
    TEST_ASSERT_TRUE(lot52_node("a")->length[WE_LEN_WIDTH].count > 0);
    TEST_ASSERT_TRUE(lot52_node("a")->length[WE_LEN_WIDTH].count <= WE_MATH_NODES_MAX);
}

static void test_l58_width_nested_functions(void) {
    lot52_render("<body style='margin:0'><div id=g style='width:200px'>"
        "<div id=a style='height:10px;background:red;width:max(20px,min(80px,calc(50% - 10px)));'>A</div>"
        "<div id=b style='height:5px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(80, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(200, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(5, lot52_node("b")->h);
    TEST_ASSERT_EQUAL(0, find_text("A")->x);
    TEST_ASSERT_EQUAL(1, find_text("A")->y);
    TEST_ASSERT_EQUAL(15, lot52_node("g")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
    TEST_ASSERT_TRUE(lot52_node("a")->length[WE_LEN_WIDTH].count > 0);
    TEST_ASSERT_TRUE(lot52_node("a")->length[WE_LEN_WIDTH].count <= WE_MATH_NODES_MAX);
}

static void test_l58_width_nested_clamp(void) {
    lot52_render("<body style='margin:0'><div id=g style='width:200px'>"
        "<div id=a style='height:10px;background:red;width:calc(clamp(20px,50%,80px) - 10px);'>A</div>"
        "<div id=b style='height:5px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(70, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(200, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(5, lot52_node("b")->h);
    TEST_ASSERT_EQUAL(0, find_text("A")->x);
    TEST_ASSERT_EQUAL(1, find_text("A")->y);
    TEST_ASSERT_EQUAL(15, lot52_node("g")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
    TEST_ASSERT_TRUE(lot52_node("a")->length[WE_LEN_WIDTH].count > 0);
    TEST_ASSERT_TRUE(lot52_node("a")->length[WE_LEN_WIDTH].count <= WE_MATH_NODES_MAX);
}

static void test_l58_width_min_four_args(void) {
    lot52_render("<body style='margin:0'><div id=g style='width:200px'>"
        "<div id=a style='height:10px;background:red;width:min(90px,80px,70px,60px);'>A</div>"
        "<div id=b style='height:5px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(60, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(200, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(5, lot52_node("b")->h);
    TEST_ASSERT_EQUAL(0, find_text("A")->x);
    TEST_ASSERT_EQUAL(1, find_text("A")->y);
    TEST_ASSERT_EQUAL(15, lot52_node("g")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
    TEST_ASSERT_TRUE(lot52_node("a")->length[WE_LEN_WIDTH].count > 0);
    TEST_ASSERT_TRUE(lot52_node("a")->length[WE_LEN_WIDTH].count <= WE_MATH_NODES_MAX);
}

static void test_l58_width_max_four_args(void) {
    lot52_render("<body style='margin:0'><div id=g style='width:200px'>"
        "<div id=a style='height:10px;background:red;width:max(30px,40px,50px,60px);'>A</div>"
        "<div id=b style='height:5px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(60, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(200, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(5, lot52_node("b")->h);
    TEST_ASSERT_EQUAL(0, find_text("A")->x);
    TEST_ASSERT_EQUAL(1, find_text("A")->y);
    TEST_ASSERT_EQUAL(15, lot52_node("g")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
    TEST_ASSERT_TRUE(lot52_node("a")->length[WE_LEN_WIDTH].count > 0);
    TEST_ASSERT_TRUE(lot52_node("a")->length[WE_LEN_WIDTH].count <= WE_MATH_NODES_MAX);
}

static void test_l58_width_negative_result(void) {
    lot52_render("<body style='margin:0'><div id=g style='width:200px'>"
        "<div id=a style='height:10px;background:red;width:calc(20px - 40px);'>A</div>"
        "<div id=b style='height:5px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(200, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(5, lot52_node("b")->h);
    TEST_ASSERT_EQUAL(0, find_text("A")->x);
    TEST_ASSERT_EQUAL(1, find_text("A")->y);
    TEST_ASSERT_EQUAL(15, lot52_node("g")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
    TEST_ASSERT_TRUE(lot52_node("a")->length[WE_LEN_WIDTH].count > 0);
    TEST_ASSERT_TRUE(lot52_node("a")->length[WE_LEN_WIDTH].count <= WE_MATH_NODES_MAX);
}

static void test_l58_width_negative_literal(void) {
    lot52_render("<body style='margin:0'><div id=g style='width:200px'>"
        "<div id=a style='height:10px;background:red;width:calc(-20px + 40px);'>A</div>"
        "<div id=b style='height:5px'></div></div></body>", 320);
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
    TEST_ASSERT_EQUAL(15, lot52_node("g")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
    TEST_ASSERT_TRUE(lot52_node("a")->length[WE_LEN_WIDTH].count > 0);
    TEST_ASSERT_TRUE(lot52_node("a")->length[WE_LEN_WIDTH].count <= WE_MATH_NODES_MAX);
}

static void test_l58_width_unary_plus(void) {
    lot52_render("<body style='margin:0'><div id=g style='width:200px'>"
        "<div id=a style='height:10px;background:red;width:calc(+20px + 40px);'>A</div>"
        "<div id=b style='height:5px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(60, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(200, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(5, lot52_node("b")->h);
    TEST_ASSERT_EQUAL(0, find_text("A")->x);
    TEST_ASSERT_EQUAL(1, find_text("A")->y);
    TEST_ASSERT_EQUAL(15, lot52_node("g")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
    TEST_ASSERT_TRUE(lot52_node("a")->length[WE_LEN_WIDTH].count > 0);
    TEST_ASSERT_TRUE(lot52_node("a")->length[WE_LEN_WIDTH].count <= WE_MATH_NODES_MAX);
}

static void test_l58_width_percent_over_full(void) {
    lot52_render("<body style='margin:0'><div id=g style='width:200px'>"
        "<div id=a style='height:10px;background:red;width:calc(200% - 50px);'>A</div>"
        "<div id=b style='height:5px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(350, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(200, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(5, lot52_node("b")->h);
    TEST_ASSERT_EQUAL(0, find_text("A")->x);
    TEST_ASSERT_EQUAL(1, find_text("A")->y);
    TEST_ASSERT_EQUAL(15, lot52_node("g")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
    TEST_ASSERT_TRUE(lot52_node("a")->length[WE_LEN_WIDTH].count > 0);
    TEST_ASSERT_TRUE(lot52_node("a")->length[WE_LEN_WIDTH].count <= WE_MATH_NODES_MAX);
}

static void test_l58_width_negative_percent(void) {
    lot52_render("<body style='margin:0'><div id=g style='width:200px'>"
        "<div id=a style='height:10px;background:red;width:calc(100px + -25%);'>A</div>"
        "<div id=b style='height:5px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(50, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(200, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(5, lot52_node("b")->h);
    TEST_ASSERT_EQUAL(0, find_text("A")->x);
    TEST_ASSERT_EQUAL(1, find_text("A")->y);
    TEST_ASSERT_EQUAL(15, lot52_node("g")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
    TEST_ASSERT_TRUE(lot52_node("a")->length[WE_LEN_WIDTH].count > 0);
    TEST_ASSERT_TRUE(lot52_node("a")->length[WE_LEN_WIDTH].count <= WE_MATH_NODES_MAX);
}

static void test_l58_width_percent_product(void) {
    lot52_render("<body style='margin:0'><div id=g style='width:200px'>"
        "<div id=a style='height:10px;background:red;width:calc(25% * 2);'>A</div><div id=b style='height:5px'>"
        "</div></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(100, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(200, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(5, lot52_node("b")->h);
    TEST_ASSERT_EQUAL(0, find_text("A")->x);
    TEST_ASSERT_EQUAL(1, find_text("A")->y);
    TEST_ASSERT_EQUAL(15, lot52_node("g")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
    TEST_ASSERT_TRUE(lot52_node("a")->length[WE_LEN_WIDTH].count > 0);
    TEST_ASSERT_TRUE(lot52_node("a")->length[WE_LEN_WIDTH].count <= WE_MATH_NODES_MAX);
}

static void test_l58_width_percent_division(void) {
    lot52_render("<body style='margin:0'><div id=g style='width:200px'>"
        "<div id=a style='height:10px;background:red;width:calc(100% / 4);'>A</div><div id=b style='height:5px'>"
        "</div></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(50, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(200, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(5, lot52_node("b")->h);
    TEST_ASSERT_EQUAL(0, find_text("A")->x);
    TEST_ASSERT_EQUAL(1, find_text("A")->y);
    TEST_ASSERT_EQUAL(15, lot52_node("g")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
    TEST_ASSERT_TRUE(lot52_node("a")->length[WE_LEN_WIDTH].count > 0);
    TEST_ASSERT_TRUE(lot52_node("a")->length[WE_LEN_WIDTH].count <= WE_MATH_NODES_MAX);
}

static void test_l58_width_uppercase_function(void) {
    lot52_render("<body style='margin:0'><div id=g style='width:200px'>"
        "<div id=a style='height:10px;background:red;width:CALC(20PX + 30PX);'>A</div>"
        "<div id=b style='height:5px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(50, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(200, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(5, lot52_node("b")->h);
    TEST_ASSERT_EQUAL(0, find_text("A")->x);
    TEST_ASSERT_EQUAL(1, find_text("A")->y);
    TEST_ASSERT_EQUAL(15, lot52_node("g")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
    TEST_ASSERT_TRUE(lot52_node("a")->length[WE_LEN_WIDTH].count > 0);
    TEST_ASSERT_TRUE(lot52_node("a")->length[WE_LEN_WIDTH].count <= WE_MATH_NODES_MAX);
}

static void test_l58_width_min_one_arg(void) {
    lot52_render("<body style='margin:0'><div id=g style='width:200px'>"
        "<div id=a style='height:10px;background:red;width:min(40px);'>A</div><div id=b style='height:5px'></div>"
        "</div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(200, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(5, lot52_node("b")->h);
    TEST_ASSERT_EQUAL(0, find_text("A")->x);
    TEST_ASSERT_EQUAL(1, find_text("A")->y);
    TEST_ASSERT_EQUAL(15, lot52_node("g")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
    TEST_ASSERT_TRUE(lot52_node("a")->length[WE_LEN_WIDTH].count > 0);
    TEST_ASSERT_TRUE(lot52_node("a")->length[WE_LEN_WIDTH].count <= WE_MATH_NODES_MAX);
}

static void test_l58_width_max_one_arg(void) {
    lot52_render("<body style='margin:0'><div id=g style='width:200px'>"
        "<div id=a style='height:10px;background:red;width:max(40px);'>A</div><div id=b style='height:5px'></div>"
        "</div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(200, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(5, lot52_node("b")->h);
    TEST_ASSERT_EQUAL(0, find_text("A")->x);
    TEST_ASSERT_EQUAL(1, find_text("A")->y);
    TEST_ASSERT_EQUAL(15, lot52_node("g")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
    TEST_ASSERT_TRUE(lot52_node("a")->length[WE_LEN_WIDTH].count > 0);
    TEST_ASSERT_TRUE(lot52_node("a")->length[WE_LEN_WIDTH].count <= WE_MATH_NODES_MAX);
}

static void test_l58_width_literal_max(void) {
    lot52_render("<body style='margin:0'><div id=g style='width:200px'>"
        "<div id=a style='height:10px;background:red;width:calc(2000px);'>A</div><div id=b style='height:5px'>"
        "</div></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(2000, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(200, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(5, lot52_node("b")->h);
    TEST_ASSERT_EQUAL(0, find_text("A")->x);
    TEST_ASSERT_EQUAL(1, find_text("A")->y);
    TEST_ASSERT_EQUAL(15, lot52_node("g")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
    TEST_ASSERT_TRUE(lot52_node("a")->length[WE_LEN_WIDTH].count > 0);
    TEST_ASSERT_TRUE(lot52_node("a")->length[WE_LEN_WIDTH].count <= WE_MATH_NODES_MAX);
}

static void test_l58_width_used_coord_max(void) {
    lot52_render("<body style='margin:0'><div id=g style='width:200px'>"
        "<div id=a style='height:10px;background:red;width:calc(2000px * 8);'>A</div><div id=b style='height:5px'>"
        "</div></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(16000, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(200, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(5, lot52_node("b")->h);
    TEST_ASSERT_EQUAL(0, find_text("A")->x);
    TEST_ASSERT_EQUAL(1, find_text("A")->y);
    TEST_ASSERT_EQUAL(15, lot52_node("g")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
    TEST_ASSERT_TRUE(lot52_node("a")->length[WE_LEN_WIDTH].count > 0);
    TEST_ASSERT_TRUE(lot52_node("a")->length[WE_LEN_WIDTH].count <= WE_MATH_NODES_MAX);
}

static void test_l58_width_used_coord_clamped(void) {
    lot52_render("<body style='margin:0'><div id=g style='width:200px'>"
        "<div id=a style='height:10px;background:red;width:calc(2000px * 9);'>A</div><div id=b style='height:5px'>"
        "</div></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(16000, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(200, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(5, lot52_node("b")->h);
    TEST_ASSERT_EQUAL(0, find_text("A")->x);
    TEST_ASSERT_EQUAL(1, find_text("A")->y);
    TEST_ASSERT_EQUAL(15, lot52_node("g")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
    TEST_ASSERT_TRUE(lot52_node("a")->length[WE_LEN_WIDTH].count > 0);
    TEST_ASSERT_TRUE(lot52_node("a")->length[WE_LEN_WIDTH].count <= WE_MATH_NODES_MAX);
}

static void test_l58_width_negative_scalar(void) {
    lot52_render("<body style='margin:0'><div id=g style='width:200px'>"
        "<div id=a style='height:10px;background:red;width:calc(-2 * -20px);'>A</div><div id=b style='height:5px'>"
        "</div></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(200, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(5, lot52_node("b")->h);
    TEST_ASSERT_EQUAL(0, find_text("A")->x);
    TEST_ASSERT_EQUAL(1, find_text("A")->y);
    TEST_ASSERT_EQUAL(15, lot52_node("g")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
    TEST_ASSERT_TRUE(lot52_node("a")->length[WE_LEN_WIDTH].count > 0);
    TEST_ASSERT_TRUE(lot52_node("a")->length[WE_LEN_WIDTH].count <= WE_MATH_NODES_MAX);
}

static void test_l58_width_negative_divisor(void) {
    lot52_render("<body style='margin:0'><div id=g style='width:200px'>"
        "<div id=a style='height:10px;background:red;width:calc(-80px / -2);'>A</div><div id=b style='height:5px'>"
        "</div></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(200, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(5, lot52_node("b")->h);
    TEST_ASSERT_EQUAL(0, find_text("A")->x);
    TEST_ASSERT_EQUAL(1, find_text("A")->y);
    TEST_ASSERT_EQUAL(15, lot52_node("g")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
    TEST_ASSERT_TRUE(lot52_node("a")->length[WE_LEN_WIDTH].count > 0);
    TEST_ASSERT_TRUE(lot52_node("a")->length[WE_LEN_WIDTH].count <= WE_MATH_NODES_MAX);
}

static void test_l58_width_operator_precedence(void) {
    lot52_render("<body style='margin:0'><div id=g style='width:200px'>"
        "<div id=a style='height:10px;background:red;width:calc(20px + 10px * 3);'>A</div>"
        "<div id=b style='height:5px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(50, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(200, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(5, lot52_node("b")->h);
    TEST_ASSERT_EQUAL(0, find_text("A")->x);
    TEST_ASSERT_EQUAL(1, find_text("A")->y);
    TEST_ASSERT_EQUAL(15, lot52_node("g")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
    TEST_ASSERT_TRUE(lot52_node("a")->length[WE_LEN_WIDTH].count > 0);
    TEST_ASSERT_TRUE(lot52_node("a")->length[WE_LEN_WIDTH].count <= WE_MATH_NODES_MAX);
}

static void test_l58_width_division_precedence(void) {
    lot52_render("<body style='margin:0'><div id=g style='width:200px'>"
        "<div id=a style='height:10px;background:red;width:calc(100px - 20px / 2);'>A</div>"
        "<div id=b style='height:5px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(90, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(200, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(5, lot52_node("b")->h);
    TEST_ASSERT_EQUAL(0, find_text("A")->x);
    TEST_ASSERT_EQUAL(1, find_text("A")->y);
    TEST_ASSERT_EQUAL(15, lot52_node("g")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
    TEST_ASSERT_TRUE(lot52_node("a")->length[WE_LEN_WIDTH].count > 0);
    TEST_ASSERT_TRUE(lot52_node("a")->length[WE_LEN_WIDTH].count <= WE_MATH_NODES_MAX);
}

static void test_l58_width_left_associative_division(void) {
    lot52_render("<body style='margin:0'><div id=g style='width:200px'>"
        "<div id=a style='height:10px;background:red;width:calc(120px / 3 / 2);'>A</div>"
        "<div id=b style='height:5px'></div></div></body>", 320);
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
    TEST_ASSERT_EQUAL(15, lot52_node("g")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
    TEST_ASSERT_TRUE(lot52_node("a")->length[WE_LEN_WIDTH].count > 0);
    TEST_ASSERT_TRUE(lot52_node("a")->length[WE_LEN_WIDTH].count <= WE_MATH_NODES_MAX);
}

static void test_l58_width_left_associative_subtraction(void) {
    lot52_render("<body style='margin:0'><div id=g style='width:200px'>"
        "<div id=a style='height:10px;background:red;width:calc(100px - 30px - 20px);'>A</div>"
        "<div id=b style='height:5px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(50, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(200, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(5, lot52_node("b")->h);
    TEST_ASSERT_EQUAL(0, find_text("A")->x);
    TEST_ASSERT_EQUAL(1, find_text("A")->y);
    TEST_ASSERT_EQUAL(15, lot52_node("g")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
    TEST_ASSERT_TRUE(lot52_node("a")->length[WE_LEN_WIDTH].count > 0);
    TEST_ASSERT_TRUE(lot52_node("a")->length[WE_LEN_WIDTH].count <= WE_MATH_NODES_MAX);
}

static void test_l58_width_full_context_cancel(void) {
    lot52_render("<body style='margin:0'><div id=g style='width:200px'>"
        "<div id=a style='height:10px;background:red;width:calc(100% - 100% + 30px);'>A</div>"
        "<div id=b style='height:5px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(200, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(5, lot52_node("b")->h);
    TEST_ASSERT_EQUAL(0, find_text("A")->x);
    TEST_ASSERT_EQUAL(1, find_text("A")->y);
    TEST_ASSERT_EQUAL(15, lot52_node("g")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
    TEST_ASSERT_TRUE(lot52_node("a")->length[WE_LEN_WIDTH].count > 0);
    TEST_ASSERT_TRUE(lot52_node("a")->length[WE_LEN_WIDTH].count <= WE_MATH_NODES_MAX);
}

static void test_l58_width_negative_clamp(void) {
    lot52_render("<body style='margin:0'><div id=g style='width:200px'>"
        "<div id=a style='height:10px;background:red;width:clamp(-10px,-5px,20px);'>A</div>"
        "<div id=b style='height:5px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(200, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(5, lot52_node("b")->h);
    TEST_ASSERT_EQUAL(0, find_text("A")->x);
    TEST_ASSERT_EQUAL(1, find_text("A")->y);
    TEST_ASSERT_EQUAL(15, lot52_node("g")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
    TEST_ASSERT_TRUE(lot52_node("a")->length[WE_LEN_WIDTH].count > 0);
    TEST_ASSERT_TRUE(lot52_node("a")->length[WE_LEN_WIDTH].count <= WE_MATH_NODES_MAX);
}

static void test_l58_width_nested_min_max(void) {
    lot52_render("<body style='margin:0'><div id=g style='width:200px'>"
        "<div id=a style='height:10px;background:red;width:min(max(20px,60px),80px);'>A</div>"
        "<div id=b style='height:5px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(60, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(200, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(5, lot52_node("b")->h);
    TEST_ASSERT_EQUAL(0, find_text("A")->x);
    TEST_ASSERT_EQUAL(1, find_text("A")->y);
    TEST_ASSERT_EQUAL(15, lot52_node("g")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
    TEST_ASSERT_TRUE(lot52_node("a")->length[WE_LEN_WIDTH].count > 0);
    TEST_ASSERT_TRUE(lot52_node("a")->length[WE_LEN_WIDTH].count <= WE_MATH_NODES_MAX);
}

static void test_l58_width_zero_product(void) {
    lot52_render("<body style='margin:0'><div id=g style='width:200px'>"
        "<div id=a style='height:10px;background:red;width:calc(0 * 20px);'>A</div><div id=b style='height:5px'>"
        "</div></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(200, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(5, lot52_node("b")->h);
    TEST_ASSERT_EQUAL(0, find_text("A")->x);
    TEST_ASSERT_EQUAL(1, find_text("A")->y);
    TEST_ASSERT_EQUAL(15, lot52_node("g")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
    TEST_ASSERT_TRUE(lot52_node("a")->length[WE_LEN_WIDTH].count > 0);
    TEST_ASSERT_TRUE(lot52_node("a")->length[WE_LEN_WIDTH].count <= WE_MATH_NODES_MAX);
}

static void test_l58_width_zero_length(void) {
    lot52_render("<body style='margin:0'><div id=g style='width:200px'>"
        "<div id=a style='height:10px;background:red;width:calc(0px);'>A</div><div id=b style='height:5px'></div>"
        "</div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(200, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(5, lot52_node("b")->h);
    TEST_ASSERT_EQUAL(0, find_text("A")->x);
    TEST_ASSERT_EQUAL(1, find_text("A")->y);
    TEST_ASSERT_EQUAL(15, lot52_node("g")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
    TEST_ASSERT_TRUE(lot52_node("a")->length[WE_LEN_WIDTH].count > 0);
    TEST_ASSERT_TRUE(lot52_node("a")->length[WE_LEN_WIDTH].count <= WE_MATH_NODES_MAX);
}

static void test_l58_width_nested_scalar_min(void) {
    lot52_render("<body style='margin:0'><div id=g style='width:200px'>"
        "<div id=a style='height:10px;background:red;width:calc(20px * min(2,3));'>A</div>"
        "<div id=b style='height:5px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(200, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(5, lot52_node("b")->h);
    TEST_ASSERT_EQUAL(0, find_text("A")->x);
    TEST_ASSERT_EQUAL(1, find_text("A")->y);
    TEST_ASSERT_EQUAL(15, lot52_node("g")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
    TEST_ASSERT_TRUE(lot52_node("a")->length[WE_LEN_WIDTH].count > 0);
    TEST_ASSERT_TRUE(lot52_node("a")->length[WE_LEN_WIDTH].count <= WE_MATH_NODES_MAX);
}

static void test_l58_width_invalid_0(void) {
    lot52_render("<body style='margin:0'><div id=g style='width:200px'>"
        "<div id=a style='height:10px;background:red;width:calc(40px + 10px);width:calc(10px + 2);'>A</div>"
        "<div id=b style='height:5px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(50, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(200, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(5, lot52_node("b")->h);
    TEST_ASSERT_TRUE(lot52_has_diag("math length refused"));
    TEST_ASSERT_EQUAL(0, find_text("A")->x);
    TEST_ASSERT_EQUAL(1, find_text("A")->y);
    TEST_ASSERT_TRUE(lot52_node("a")->length[WE_LEN_WIDTH].count > 0);
}

static void test_l58_width_invalid_1(void) {
    lot52_render("<body style='margin:0'><div id=g style='width:200px'>"
        "<div id=a style='height:10px;background:red;width:calc(40px + 10px);width:calc(10px * 2px);'>A</div>"
        "<div id=b style='height:5px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(50, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(200, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(5, lot52_node("b")->h);
    TEST_ASSERT_TRUE(lot52_has_diag("math length refused"));
    TEST_ASSERT_EQUAL(0, find_text("A")->x);
    TEST_ASSERT_EQUAL(1, find_text("A")->y);
    TEST_ASSERT_TRUE(lot52_node("a")->length[WE_LEN_WIDTH].count > 0);
}

static void test_l58_width_invalid_2(void) {
    lot52_render("<body style='margin:0'><div id=g style='width:200px'>"
        "<div id=a style='height:10px;background:red;width:calc(40px + 10px);width:calc(10px / 2px);'>A</div>"
        "<div id=b style='height:5px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(50, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(200, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(5, lot52_node("b")->h);
    TEST_ASSERT_TRUE(lot52_has_diag("math length refused"));
    TEST_ASSERT_EQUAL(0, find_text("A")->x);
    TEST_ASSERT_EQUAL(1, find_text("A")->y);
    TEST_ASSERT_TRUE(lot52_node("a")->length[WE_LEN_WIDTH].count > 0);
}

static void test_l58_width_invalid_3(void) {
    lot52_render("<body style='margin:0'><div id=g style='width:200px'>"
        "<div id=a style='height:10px;background:red;width:calc(40px + 10px);width:calc(10px / 0);'>A</div>"
        "<div id=b style='height:5px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(50, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(200, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(5, lot52_node("b")->h);
    TEST_ASSERT_TRUE(lot52_has_diag("math length refused"));
    TEST_ASSERT_EQUAL(0, find_text("A")->x);
    TEST_ASSERT_EQUAL(1, find_text("A")->y);
    TEST_ASSERT_TRUE(lot52_node("a")->length[WE_LEN_WIDTH].count > 0);
}

static void test_l58_width_invalid_4(void) {
    lot52_render("<body style='margin:0'><div id=g style='width:200px'>"
        "<div id=a style='height:10px;background:red;width:calc(40px + 10px);width:calc(10px / (2 - 2));'>A</div>"
        "<div id=b style='height:5px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(50, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(200, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(5, lot52_node("b")->h);
    TEST_ASSERT_TRUE(lot52_has_diag("math length refused"));
    TEST_ASSERT_EQUAL(0, find_text("A")->x);
    TEST_ASSERT_EQUAL(1, find_text("A")->y);
    TEST_ASSERT_TRUE(lot52_node("a")->length[WE_LEN_WIDTH].count > 0);
}

static void test_l58_width_invalid_5(void) {
    lot52_render("<body style='margin:0'><div id=g style='width:200px'>"
        "<div id=a style='height:10px;background:red;width:calc(40px + 10px);width:calc(20px+10px);'>A</div>"
        "<div id=b style='height:5px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(50, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(200, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(5, lot52_node("b")->h);
    TEST_ASSERT_TRUE(lot52_has_diag("math length refused"));
    TEST_ASSERT_EQUAL(0, find_text("A")->x);
    TEST_ASSERT_EQUAL(1, find_text("A")->y);
    TEST_ASSERT_TRUE(lot52_node("a")->length[WE_LEN_WIDTH].count > 0);
}

static void test_l58_width_invalid_6(void) {
    lot52_render("<body style='margin:0'><div id=g style='width:200px'>"
        "<div id=a style='height:10px;background:red;width:calc(40px + 10px);width:calc(20px +10px);'>A</div>"
        "<div id=b style='height:5px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(50, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(200, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(5, lot52_node("b")->h);
    TEST_ASSERT_TRUE(lot52_has_diag("math length refused"));
    TEST_ASSERT_EQUAL(0, find_text("A")->x);
    TEST_ASSERT_EQUAL(1, find_text("A")->y);
    TEST_ASSERT_TRUE(lot52_node("a")->length[WE_LEN_WIDTH].count > 0);
}

static void test_l58_width_invalid_7(void) {
    lot52_render("<body style='margin:0'><div id=g style='width:200px'>"
        "<div id=a style='height:10px;background:red;width:calc(40px + 10px);width:calc(20px+ 10px);'>A</div>"
        "<div id=b style='height:5px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(50, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(200, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(5, lot52_node("b")->h);
    TEST_ASSERT_TRUE(lot52_has_diag("math length refused"));
    TEST_ASSERT_EQUAL(0, find_text("A")->x);
    TEST_ASSERT_EQUAL(1, find_text("A")->y);
    TEST_ASSERT_TRUE(lot52_node("a")->length[WE_LEN_WIDTH].count > 0);
}

static void test_l58_width_invalid_8(void) {
    lot52_render("<body style='margin:0'><div id=g style='width:200px'>"
        "<div id=a style='height:10px;background:red;width:calc(40px + 10px);width:calc(1.5px + 2px);'>A</div>"
        "<div id=b style='height:5px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(50, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(200, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(5, lot52_node("b")->h);
    TEST_ASSERT_TRUE(lot52_has_diag("math length refused"));
    TEST_ASSERT_EQUAL(0, find_text("A")->x);
    TEST_ASSERT_EQUAL(1, find_text("A")->y);
    TEST_ASSERT_TRUE(lot52_node("a")->length[WE_LEN_WIDTH].count > 0);
}

static void test_l58_width_invalid_9(void) {
    lot52_render("<body style='margin:0'><div id=g style='width:200px'>"
        "<div id=a style='height:10px;background:red;width:calc(40px + 10px);width:calc(20em + 2px);'>A</div>"
        "<div id=b style='height:5px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(50, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(200, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(5, lot52_node("b")->h);
    TEST_ASSERT_TRUE(lot52_has_diag("math length refused"));
    TEST_ASSERT_EQUAL(0, find_text("A")->x);
    TEST_ASSERT_EQUAL(1, find_text("A")->y);
    TEST_ASSERT_TRUE(lot52_node("a")->length[WE_LEN_WIDTH].count > 0);
}

static void test_l58_width_invalid_10(void) {
    lot52_render("<body style='margin:0'><div id=g style='width:200px'>"
        "<div id=a style='height:10px;background:red;width:calc(40px + 10px);width:calc(2001px);'>A</div>"
        "<div id=b style='height:5px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(50, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(200, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(5, lot52_node("b")->h);
    TEST_ASSERT_TRUE(lot52_has_diag("math length refused"));
    TEST_ASSERT_EQUAL(0, find_text("A")->x);
    TEST_ASSERT_EQUAL(1, find_text("A")->y);
    TEST_ASSERT_TRUE(lot52_node("a")->length[WE_LEN_WIDTH].count > 0);
}

static void test_l58_width_invalid_11(void) {
    lot52_render("<body style='margin:0'><div id=g style='width:200px'>"
        "<div id=a style='height:10px;background:red;width:calc(40px + 10px);width:calc(999999999999px);'>A</div>"
        "<div id=b style='height:5px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(50, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(200, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(5, lot52_node("b")->h);
    TEST_ASSERT_TRUE(lot52_has_diag("math length refused"));
    TEST_ASSERT_EQUAL(0, find_text("A")->x);
    TEST_ASSERT_EQUAL(1, find_text("A")->y);
    TEST_ASSERT_TRUE(lot52_node("a")->length[WE_LEN_WIDTH].count > 0);
}

static void test_l58_width_invalid_12(void) {
    lot52_render("<body style='margin:0'><div id=g style='width:200px'>"
        "<div id=a style='height:10px;background:red;width:calc(40px + 10px);width:min(10px,2);'>A</div>"
        "<div id=b style='height:5px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(50, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(200, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(5, lot52_node("b")->h);
    TEST_ASSERT_TRUE(lot52_has_diag("math length refused"));
    TEST_ASSERT_EQUAL(0, find_text("A")->x);
    TEST_ASSERT_EQUAL(1, find_text("A")->y);
    TEST_ASSERT_TRUE(lot52_node("a")->length[WE_LEN_WIDTH].count > 0);
}

static void test_l58_width_invalid_13(void) {
    lot52_render("<body style='margin:0'><div id=g style='width:200px'>"
        "<div id=a style='height:10px;background:red;width:calc(40px + 10px);width:clamp(10px,20px);'>A</div>"
        "<div id=b style='height:5px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(50, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(200, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(5, lot52_node("b")->h);
    TEST_ASSERT_TRUE(lot52_has_diag("math length refused"));
    TEST_ASSERT_EQUAL(0, find_text("A")->x);
    TEST_ASSERT_EQUAL(1, find_text("A")->y);
    TEST_ASSERT_TRUE(lot52_node("a")->length[WE_LEN_WIDTH].count > 0);
}

static void test_l58_width_invalid_14(void) {
    lot52_render("<body style='margin:0'><div id=g style='width:200px'>"
        "<div id=a style='height:10px;background:red;width:calc(40px + 10px);width:clamp(10px,20px,30px,40px);'>A"
        "</div><div id=b style='height:5px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(50, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(200, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(5, lot52_node("b")->h);
    TEST_ASSERT_TRUE(lot52_has_diag("math length refused"));
    TEST_ASSERT_EQUAL(0, find_text("A")->x);
    TEST_ASSERT_EQUAL(1, find_text("A")->y);
    TEST_ASSERT_TRUE(lot52_node("a")->length[WE_LEN_WIDTH].count > 0);
}

static void test_l58_width_invalid_15(void) {
    lot52_render("<body style='margin:0'><div id=g style='width:200px'>"
        "<div id=a style='height:10px;background:red;width:calc(40px + 10px);width:min();'>A</div>"
        "<div id=b style='height:5px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(50, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(200, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(5, lot52_node("b")->h);
    TEST_ASSERT_TRUE(lot52_has_diag("math length refused"));
    TEST_ASSERT_EQUAL(0, find_text("A")->x);
    TEST_ASSERT_EQUAL(1, find_text("A")->y);
    TEST_ASSERT_TRUE(lot52_node("a")->length[WE_LEN_WIDTH].count > 0);
}

static void test_l58_width_invalid_16(void) {
    lot52_render("<body style='margin:0'><div id=g style='width:200px'>"
        "<div id=a style='height:10px;background:red;width:calc(40px + 10px);width:max(1px,2px,3px,4px,5px);'>A"
        "</div><div id=b style='height:5px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(50, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(200, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(5, lot52_node("b")->h);
    TEST_ASSERT_TRUE(lot52_has_diag("math length refused"));
    TEST_ASSERT_EQUAL(0, find_text("A")->x);
    TEST_ASSERT_EQUAL(1, find_text("A")->y);
    TEST_ASSERT_TRUE(lot52_node("a")->length[WE_LEN_WIDTH].count > 0);
}

static void test_l58_width_invalid_17(void) {
    lot52_render("<body style='margin:0'><div id=g style='width:200px'>"
        "<div id=a style='height:10px;background:red;width:calc(40px + 10px);width:calc(20px + );'>A</div>"
        "<div id=b style='height:5px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(50, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(200, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(5, lot52_node("b")->h);
    TEST_ASSERT_TRUE(lot52_has_diag("math length refused"));
    TEST_ASSERT_EQUAL(0, find_text("A")->x);
    TEST_ASSERT_EQUAL(1, find_text("A")->y);
    TEST_ASSERT_TRUE(lot52_node("a")->length[WE_LEN_WIDTH].count > 0);
}

static void test_l58_width_invalid_18(void) {
    lot52_render("<body style='margin:0'><div id=g style='width:200px'>"
        "<div id=a style='height:10px;background:red;width:calc(40px + 10px);width:calc();'>A</div>"
        "<div id=b style='height:5px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(50, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(200, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(5, lot52_node("b")->h);
    TEST_ASSERT_TRUE(lot52_has_diag("math length refused"));
    TEST_ASSERT_EQUAL(0, find_text("A")->x);
    TEST_ASSERT_EQUAL(1, find_text("A")->y);
    TEST_ASSERT_TRUE(lot52_node("a")->length[WE_LEN_WIDTH].count > 0);
}

static void test_l58_width_invalid_19(void) {
    lot52_render("<body style='margin:0'><div id=g style='width:200px'>"
        "<div id=a style='height:10px;background:red;width:calc(40px + 10px);width:calc(10px)junk;'>A</div>"
        "<div id=b style='height:5px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(50, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(200, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(5, lot52_node("b")->h);
    TEST_ASSERT_TRUE(lot52_has_diag("math length refused"));
    TEST_ASSERT_EQUAL(0, find_text("A")->x);
    TEST_ASSERT_EQUAL(1, find_text("A")->y);
    TEST_ASSERT_TRUE(lot52_node("a")->length[WE_LEN_WIDTH].count > 0);
}

static void test_l58_width_invalid_20(void) {
    lot52_render("<body style='margin:0'><div id=g style='width:200px'>"
        "<div id=a style='height:10px;background:red;width:calc(40px + 10px);width:calc(10px));'>A</div>"
        "<div id=b style='height:5px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(50, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(200, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(5, lot52_node("b")->h);
    TEST_ASSERT_TRUE(lot52_has_diag("math length refused"));
    TEST_ASSERT_EQUAL(0, find_text("A")->x);
    TEST_ASSERT_EQUAL(1, find_text("A")->y);
    TEST_ASSERT_TRUE(lot52_node("a")->length[WE_LEN_WIDTH].count > 0);
}

static void test_l58_width_invalid_21(void) {
    lot52_render("<body style='margin:0'><div id=g style='width:200px'>"
        "<div id=a style='height:10px;background:red;width:calc(40px + 10px);width:calc((10px);'>A</div>"
        "<div id=b style='height:5px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(50, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(200, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(5, lot52_node("b")->h);
    TEST_ASSERT_TRUE(lot52_has_diag("math length refused"));
    TEST_ASSERT_EQUAL(0, find_text("A")->x);
    TEST_ASSERT_EQUAL(1, find_text("A")->y);
    TEST_ASSERT_TRUE(lot52_node("a")->length[WE_LEN_WIDTH].count > 0);
}

static void test_l58_width_invalid_22(void) {
    lot52_render("<body style='margin:0'><div id=g style='width:200px'>"
        "<div id=a style='height:10px;background:red;width:calc(40px + 10px);width:calc(10px 20px);'>A</div>"
        "<div id=b style='height:5px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(50, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(200, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(5, lot52_node("b")->h);
    TEST_ASSERT_TRUE(lot52_has_diag("math length refused"));
    TEST_ASSERT_EQUAL(0, find_text("A")->x);
    TEST_ASSERT_EQUAL(1, find_text("A")->y);
    TEST_ASSERT_TRUE(lot52_node("a")->length[WE_LEN_WIDTH].count > 0);
}

static void test_l58_width_invalid_23(void) {
    lot52_render("<body style='margin:0'><div id=g style='width:200px'>"
        "<div id=a style='height:10px;background:red;width:calc(40px + 10px);width:calc(2000px * 1000);'>A</div>"
        "<div id=b style='height:5px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(50, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(200, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(5, lot52_node("b")->h);
    TEST_ASSERT_TRUE(lot52_has_diag("math length refused"));
    TEST_ASSERT_EQUAL(0, find_text("A")->x);
    TEST_ASSERT_EQUAL(1, find_text("A")->y);
    TEST_ASSERT_TRUE(lot52_node("a")->length[WE_LEN_WIDTH].count > 0);
}

static void test_l58_width_invalid_24(void) {
    lot52_render("<body style='margin:0'><div id=g style='width:200px'>"
        "<div id=a style='height:10px;background:red;width:calc(40px + 10px);width:calc(calc(calc(calc(calc(10px)))));'>"
        "A</div><div id=b style='height:5px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(50, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(200, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(5, lot52_node("b")->h);
    TEST_ASSERT_TRUE(lot52_has_diag("math length refused"));
    TEST_ASSERT_EQUAL(0, find_text("A")->x);
    TEST_ASSERT_EQUAL(1, find_text("A")->y);
    TEST_ASSERT_TRUE(lot52_node("a")->length[WE_LEN_WIDTH].count > 0);
}

static void test_l58_width_invalid_25(void) {
    lot52_render("<body style='margin:0'><div id=g style='width:200px'>"
        "<div id=a style='height:10px;background:red;width:calc(40px + 10px);width:calc(1 + 2);'>A</div>"
        "<div id=b style='height:5px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(50, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(200, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(5, lot52_node("b")->h);
    TEST_ASSERT_TRUE(lot52_has_diag("math length refused"));
    TEST_ASSERT_EQUAL(0, find_text("A")->x);
    TEST_ASSERT_EQUAL(1, find_text("A")->y);
    TEST_ASSERT_TRUE(lot52_node("a")->length[WE_LEN_WIDTH].count > 0);
}

static void test_l58_width_invalid_26(void) {
    lot52_render("<body style='margin:0'><div id=g style='width:200px'>"
        "<div id=a style='height:10px;background:red;width:calc(40px + 10px);width:calc(10px ** 2);'>A</div>"
        "<div id=b style='height:5px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(50, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(200, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(5, lot52_node("b")->h);
    TEST_ASSERT_TRUE(lot52_has_diag("math length refused"));
    TEST_ASSERT_EQUAL(0, find_text("A")->x);
    TEST_ASSERT_EQUAL(1, find_text("A")->y);
    TEST_ASSERT_TRUE(lot52_node("a")->length[WE_LEN_WIDTH].count > 0);
}

static void test_l58_width_invalid_27(void) {
    lot52_render("<body style='margin:0'><div id=g style='width:200px'>"
        "<div id=a style='height:10px;background:red;width:calc(40px + 10px);width:calc(10px // 2);'>A</div>"
        "<div id=b style='height:5px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(50, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(200, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(5, lot52_node("b")->h);
    TEST_ASSERT_TRUE(lot52_has_diag("math length refused"));
    TEST_ASSERT_EQUAL(0, find_text("A")->x);
    TEST_ASSERT_EQUAL(1, find_text("A")->y);
    TEST_ASSERT_TRUE(lot52_node("a")->length[WE_LEN_WIDTH].count > 0);
}

static void test_l58_width_invalid_28(void) {
    lot52_render("<body style='margin:0'><div id=g style='width:200px'>"
        "<div id=a style='height:10px;background:red;width:calc(40px + 10px);width:calc(10px / -0);'>A</div>"
        "<div id=b style='height:5px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(50, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(200, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(5, lot52_node("b")->h);
    TEST_ASSERT_TRUE(lot52_has_diag("math length refused"));
    TEST_ASSERT_EQUAL(0, find_text("A")->x);
    TEST_ASSERT_EQUAL(1, find_text("A")->y);
    TEST_ASSERT_TRUE(lot52_node("a")->length[WE_LEN_WIDTH].count > 0);
}

static void test_l58_width_invalid_29(void) {
    lot52_render("<body style='margin:0'><div id=g style='width:200px'>"
        "<div id=a style='height:10px;background:red;width:calc(40px + 10px);width:calc(10px + var(--x));'>A</div>"
        "<div id=b style='height:5px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(50, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(200, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(5, lot52_node("b")->h);
    TEST_ASSERT_TRUE(lot52_has_diag("math length refused"));
    TEST_ASSERT_EQUAL(0, find_text("A")->x);
    TEST_ASSERT_EQUAL(1, find_text("A")->y);
    TEST_ASSERT_TRUE(lot52_node("a")->length[WE_LEN_WIDTH].count > 0);
}

static void test_l58_width_replacement_numeric(void) {
    lot52_render("<body style='margin:0'><div id=g style='width:200px'>"
        "<div id=a style='height:10px;background:red;width:calc(40px + 10px);width:70px;'>A</div>"
        "<div id=b style='height:5px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(70, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
    TEST_ASSERT_EQUAL(0, find_text("A")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->length[WE_LEN_WIDTH].count);
}

static void test_l58_width_replacement_percent(void) {
    lot52_render("<body style='margin:0'><div id=g style='width:200px'>"
        "<div id=a style='height:10px;background:red;width:calc(40px + 10px);width:25%;'>A</div>"
        "<div id=b style='height:5px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(50, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
    TEST_ASSERT_EQUAL(0, find_text("A")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->length[WE_LEN_WIDTH].count);
}

static void test_l58_width_replacement_auto(void) {
    lot52_render("<body style='margin:0'><div id=g style='width:200px'>"
        "<div id=a style='height:10px;background:red;width:calc(40px + 10px);width:auto;'>A</div>"
        "<div id=b style='height:5px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(200, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
    TEST_ASSERT_EQUAL(0, find_text("A")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->length[WE_LEN_WIDTH].count);
}

static void test_l58_width_replacement_minimum_content(void) {
    lot52_render("<body style='margin:0'><div id=g style='width:200px'>"
        "<div id=a style='height:10px;background:red;width:calc(40px + 10px);width:min-content;'>A</div>"
        "<div id=b style='height:5px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(8, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
    TEST_ASSERT_EQUAL(0, find_text("A")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->length[WE_LEN_WIDTH].count);
}

static void test_l58_width_replacement_maximum_content(void) {
    lot52_render("<body style='margin:0'><div id=g style='width:200px'>"
        "<div id=a style='height:10px;background:red;width:calc(40px + 10px);width:max-content;'>A</div>"
        "<div id=b style='height:5px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(8, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
    TEST_ASSERT_EQUAL(0, find_text("A")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->length[WE_LEN_WIDTH].count);
}

static void test_l58_width_replacement_fit_content(void) {
    lot52_render("<body style='margin:0'><div id=g style='width:200px'>"
        "<div id=a style='height:10px;background:red;width:calc(40px + 10px);width:fit-content;'>A</div>"
        "<div id=b style='height:5px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(8, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
    TEST_ASSERT_EQUAL(0, find_text("A")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->length[WE_LEN_WIDTH].count);
}

static void test_l58_width_replacement_invalid(void) {
    lot52_render("<body style='margin:0'><div id=g style='width:200px'>"
        "<div id=a style='height:10px;background:red;width:calc(40px + 10px);width:bogus;'>A</div>"
        "<div id=b style='height:5px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(50, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
    TEST_ASSERT_EQUAL(0, find_text("A")->x);
    TEST_ASSERT_TRUE(lot52_node("a")->length[WE_LEN_WIDTH].count > 0);
}
