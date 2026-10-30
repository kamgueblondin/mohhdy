static void test_l58_logical_logical_sizes(void) {
    lot52_render("<body style='margin:0'><div id=g style='width:200px;height:100px'>"
        "<div id=a style='height:10px;background:red;inline-size:60px;block-size:30px;'></div>"
        "<div id=b style='height:5px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(60, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(30, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(200, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(5, lot52_node("b")->h);
    TEST_ASSERT_EQUAL(100, lot52_node("g")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l58_logical_logical_math(void) {
    lot52_render("<body style='margin:0'><div id=g style='width:200px;height:100px'>"
        "<div id=a style='height:10px;background:red;inline-size:calc(50% - 20px);block-size:calc(10px + 20px);'>"
        "</div><div id=b style='height:5px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(80, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(30, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(200, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(5, lot52_node("b")->h);
    TEST_ASSERT_EQUAL(100, lot52_node("g")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l58_logical_logical_min_inline(void) {
    lot52_render("<body style='margin:0'><div id=g style='width:200px;height:100px'>"
        "<div id=a style='height:10px;background:red;inline-size:20px;min-inline-size:60px;block-size:10px;'>"
        "</div><div id=b style='height:5px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(60, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(200, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(5, lot52_node("b")->h);
    TEST_ASSERT_EQUAL(100, lot52_node("g")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l58_logical_logical_max_inline(void) {
    lot52_render("<body style='margin:0'><div id=g style='width:200px;height:100px'>"
        "<div id=a style='height:10px;background:red;inline-size:100px;max-inline-size:60px;block-size:10px;'>"
        "</div><div id=b style='height:5px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(60, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(200, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(5, lot52_node("b")->h);
    TEST_ASSERT_EQUAL(100, lot52_node("g")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l58_logical_logical_min_block(void) {
    lot52_render("<body style='margin:0'><div id=g style='width:200px;height:100px'>"
        "<div id=a style='height:10px;background:red;inline-size:60px;block-size:10px;min-block-size:30px;'></div>"
        "<div id=b style='height:5px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(60, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(30, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(200, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(5, lot52_node("b")->h);
    TEST_ASSERT_EQUAL(100, lot52_node("g")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l58_logical_logical_max_block(void) {
    lot52_render("<body style='margin:0'><div id=g style='width:200px;height:100px'>"
        "<div id=a style='height:10px;background:red;inline-size:60px;block-size:80px;max-block-size:30px;'></div>"
        "<div id=b style='height:5px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(60, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(30, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(200, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(5, lot52_node("b")->h);
    TEST_ASSERT_EQUAL(100, lot52_node("g")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l58_logical_logical_limits_math(void) {
    lot52_render("<body style='margin:0'><div id=g style='width:200px;height:100px'>"
        "<div id=a style='height:10px;background:red;inline-size:10px;min-inline-size:calc(40px + 20px);block-size:10px;min-block-size:calc(20px + 10px);'>"
        "</div><div id=b style='height:5px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(60, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(30, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(200, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(5, lot52_node("b")->h);
    TEST_ASSERT_EQUAL(100, lot52_node("g")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l58_logical_physical_after_logical(void) {
    lot52_render("<body style='margin:0'><div id=g style='width:200px;height:100px'>"
        "<div id=a style='height:10px;background:red;inline-size:60px;width:80px;block-size:30px;height:20px;'>"
        "</div><div id=b style='height:5px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(80, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(20, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(200, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(5, lot52_node("b")->h);
    TEST_ASSERT_EQUAL(100, lot52_node("g")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l58_logical_logical_after_physical(void) {
    lot52_render("<body style='margin:0'><div id=g style='width:200px;height:100px'>"
        "<div id=a style='height:10px;background:red;width:80px;inline-size:60px;height:20px;block-size:30px;'>"
        "</div><div id=b style='height:5px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(60, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(30, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(200, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(5, lot52_node("b")->h);
    TEST_ASSERT_EQUAL(100, lot52_node("g")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l58_logical_logical_percent_height(void) {
    lot52_render("<body style='margin:0'><div id=g style='width:200px;height:100px'>"
        "<div id=a style='height:10px;background:red;inline-size:25%;block-size:50%;'></div>"
        "<div id=b style='height:5px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(50, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(50, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(50, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(200, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(5, lot52_node("b")->h);
    TEST_ASSERT_EQUAL(100, lot52_node("g")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l58_logical_logical_padding_pairs(void) {
    lot52_render("<body style='margin:0'><div id=g style='width:200px'>"
        "<div id=a style='height:10px;background:red;width:40px;height:10px;padding-inline:4px 6px;padding-block:2px 3px;'>"
        "A</div><div id=b style='height:5px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(50, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(15, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
    TEST_ASSERT_EQUAL(4, find_text("A")->x);
}

static void test_l58_logical_logical_padding_one(void) {
    lot52_render("<body style='margin:0'><div id=g style='width:200px'>"
        "<div id=a style='height:10px;background:red;width:40px;height:10px;padding-inline:4px;padding-block:2px;'>"
        "A</div><div id=b style='height:5px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(48, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(14, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
    TEST_ASSERT_EQUAL(4, find_text("A")->x);
}

static void test_l58_logical_logical_padding_edges(void) {
    lot52_render("<body style='margin:0'><div id=g style='width:200px'>"
        "<div id=a style='height:10px;background:red;width:40px;height:10px;padding-inline-start:4px;padding-inline-end:6px;padding-block-start:2px;padding-block-end:3px;'>"
        "A</div><div id=b style='height:5px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(50, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(15, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
    TEST_ASSERT_EQUAL(4, find_text("A")->x);
}

static void test_l58_logical_logical_margin_pairs(void) {
    lot52_render("<body style='margin:0'><div id=g style='width:200px'>"
        "<div id=a style='height:10px;background:red;width:40px;height:10px;margin-inline:4px 6px;margin-block:2px 3px;'>"
        "A</div><div id=b style='height:5px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(4, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(2, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l58_logical_logical_margin_edges(void) {
    lot52_render("<body style='margin:0'><div id=g style='width:200px'>"
        "<div id=a style='height:10px;background:red;width:40px;height:10px;margin-inline-start:4px;margin-inline-end:6px;margin-block-start:2px;margin-block-end:3px;'>"
        "A</div><div id=b style='height:5px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(4, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(2, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l58_logical_logical_margin_one(void) {
    lot52_render("<body style='margin:0'><div id=g style='width:200px'>"
        "<div id=a style='height:10px;background:red;width:40px;height:10px;margin-inline:4px;margin-block:2px;'>A"
        "</div><div id=b style='height:5px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(4, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(2, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l58_logical_logical_margin_auto(void) {
    lot52_render("<body style='margin:0'><div id=g style='width:200px'>"
        "<div id=a style='height:10px;background:red;width:40px;height:10px;margin-inline:auto;'>A</div>"
        "<div id=b style='height:5px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(80, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l58_logical_logical_margin_start_auto(void) {
    lot52_render("<body style='margin:0'><div id=g style='width:200px'>"
        "<div id=a style='height:10px;background:red;width:40px;height:10px;margin-inline:auto 10px;'>A</div>"
        "<div id=b style='height:5px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(150, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l58_logical_logical_margin_end_auto(void) {
    lot52_render("<body style='margin:0'><div id=g style='width:200px'>"
        "<div id=a style='height:10px;background:red;width:40px;height:10px;margin-inline:10px auto;'>A</div>"
        "<div id=b style='height:5px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l58_logical_logical_pair_clear_auto(void) {
    lot52_render("<body style='margin:0'><div id=g style='width:200px'>"
        "<div id=a style='height:10px;background:red;width:40px;height:10px;margin-inline:auto;margin-inline:4px 6px;'>"
        "A</div><div id=b style='height:5px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(4, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l58_logical_physical_over_logical_padding(void) {
    lot52_render("<body style='margin:0'><div id=g style='width:200px'>"
        "<div id=a style='height:10px;background:red;width:40px;height:10px;padding-inline:4px 6px;padding-left:8px;'>"
        "A</div><div id=b style='height:5px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(54, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
    TEST_ASSERT_EQUAL(8, find_text("A")->x);
}

static void test_l58_logical_physical_over_logical_margin(void) {
    lot52_render("<body style='margin:0'><div id=g style='width:200px'>"
        "<div id=a style='height:10px;background:red;width:40px;height:10px;margin-inline:4px 6px;margin-left:8px;'>"
        "A</div><div id=b style='height:5px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(8, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l58_logical_padding_percent(void) {
    lot52_render("<body style='margin:0'><div id=g style='width:200px'>"
        "<div id=a style='height:10px;background:red;width:40px;padding-inline:4px 6px;padding-inline:10%px 2px;'>A"
        "</div><div id=b style='height:5px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(50, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(4, find_text("A")->x);
    TEST_ASSERT_TRUE(lot52_has_diag("logical spacing refused"));
}

static void test_l58_logical_padding_negative(void) {
    lot52_render("<body style='margin:0'><div id=g style='width:200px'>"
        "<div id=a style='height:10px;background:red;width:40px;padding-inline:4px 6px;padding-inline:-1px 2px;'>A"
        "</div><div id=b style='height:5px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(50, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(4, find_text("A")->x);
    TEST_ASSERT_TRUE(lot52_has_diag("logical spacing refused"));
}

static void test_l58_logical_padding_three(void) {
    lot52_render("<body style='margin:0'><div id=g style='width:200px'>"
        "<div id=a style='height:10px;background:red;width:40px;padding-inline:4px 6px;padding-inline:1px 2px 3px;'>"
        "A</div><div id=b style='height:5px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(50, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(4, find_text("A")->x);
    TEST_ASSERT_TRUE(lot52_has_diag("logical spacing refused"));
}

static void test_l58_logical_padding_math(void) {
    lot52_render("<body style='margin:0'><div id=g style='width:200px'>"
        "<div id=a style='height:10px;background:red;width:40px;padding-inline:4px 6px;padding-inline:calc(2px + 2px);'>"
        "A</div><div id=b style='height:5px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(50, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(4, find_text("A")->x);
    TEST_ASSERT_TRUE(lot52_has_diag("logical spacing refused"));
}

static void test_l58_logical_vertical_mode(void) {
    lot52_render("<body style='margin:0'><div id=g style='width:200px'>"
        "<div id=a style='height:10px;background:red;width:40px;padding-inline:4px 6px;writing-mode:vertical-rl;'>"
        "A</div><div id=b style='height:5px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(50, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(4, find_text("A")->x);
    TEST_ASSERT_TRUE(lot52_has_diag("writing mode refused"));
}

static void test_l58_logical_rtl_mode(void) {
    lot52_render("<body style='margin:0'><div id=g style='width:200px'>"
        "<div id=a style='height:10px;background:red;width:40px;padding-inline:4px 6px;direction:rtl;'>A</div>"
        "<div id=b style='height:5px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(50, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(4, find_text("A")->x);
    TEST_ASSERT_TRUE(lot52_has_diag("direction refused"));
}
