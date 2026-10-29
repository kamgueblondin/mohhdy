static void test_l57_cross_row_before(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:flex;width:100px;height:40px;align-items:center;'>"
        "<div id=a style='width:20px;height:10px;margin-top:auto'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
    TEST_ASSERT_EQUAL(1, g_doc.perf.flex_lines);
}

static void test_l57_cross_row_after(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:flex;width:100px;height:40px;align-items:center;'>"
        "<div id=a style='width:20px;height:10px;margin-bottom:auto'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
    TEST_ASSERT_EQUAL(1, g_doc.perf.flex_lines);
}

static void test_l57_cross_row_both(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:flex;width:100px;height:40px;align-items:center;'>"
        "<div id=a style='width:20px;height:10px;margin-top:auto;margin-bottom:auto'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(15, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
    TEST_ASSERT_EQUAL(1, g_doc.perf.flex_lines);
}

static void test_l57_cross_row_none(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:flex;width:100px;height:40px;align-items:center;'>"
        "<div id=a style='width:20px;height:10px;margin-top:0'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(15, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
    TEST_ASSERT_EQUAL(1, g_doc.perf.flex_lines);
}

static void test_l57_cross_column_before(void) {
    lot52_render("<body style='margin:0'>"
        "<div id=g style='display:flex;width:40px;height:100px;align-items:center;flex-direction:column;'>"
        "<div id=a style='width:10px;height:20px;margin-left:auto'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
    TEST_ASSERT_EQUAL(1, g_doc.perf.flex_lines);
}

static void test_l57_cross_column_after(void) {
    lot52_render("<body style='margin:0'>"
        "<div id=g style='display:flex;width:40px;height:100px;align-items:center;flex-direction:column;'>"
        "<div id=a style='width:10px;height:20px;margin-right:auto'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
    TEST_ASSERT_EQUAL(1, g_doc.perf.flex_lines);
}

static void test_l57_cross_column_both(void) {
    lot52_render("<body style='margin:0'>"
        "<div id=g style='display:flex;width:40px;height:100px;align-items:center;flex-direction:column;'>"
        "<div id=a style='width:10px;height:20px;margin-left:auto;margin-right:auto'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(15, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
    TEST_ASSERT_EQUAL(1, g_doc.perf.flex_lines);
}

static void test_l57_cross_column_none(void) {
    lot52_render("<body style='margin:0'>"
        "<div id=g style='display:flex;width:40px;height:100px;align-items:center;flex-direction:column;'>"
        "<div id=a style='width:10px;height:20px;margin-left:0'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(15, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
    TEST_ASSERT_EQUAL(1, g_doc.perf.flex_lines);
}

static void test_l57_cross_auto_overrides_start_top(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:flex;width:80px;height:40px;align-items:start'>"
        "<div id=a style='width:20px;height:10px;margin-top:auto'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(40, lot52_node("g")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l57_cross_auto_overrides_start_bottom(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:flex;width:80px;height:40px;align-items:start'>"
        "<div id=a style='width:20px;height:10px;margin-bottom:auto'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(40, lot52_node("g")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l57_cross_auto_overrides_end_top(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:flex;width:80px;height:40px;align-items:end'>"
        "<div id=a style='width:20px;height:10px;margin-top:auto'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(40, lot52_node("g")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l57_cross_auto_overrides_end_bottom(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:flex;width:80px;height:40px;align-items:end'>"
        "<div id=a style='width:20px;height:10px;margin-bottom:auto'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(40, lot52_node("g")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l57_cross_auto_overrides_center_top(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:flex;width:80px;height:40px;align-items:center'>"
        "<div id=a style='width:20px;height:10px;margin-top:auto'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(40, lot52_node("g")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l57_cross_auto_overrides_center_bottom(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:flex;width:80px;height:40px;align-items:center'>"
        "<div id=a style='width:20px;height:10px;margin-bottom:auto'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(40, lot52_node("g")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l57_cross_auto_overrides_stretch_top(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:flex;width:80px;height:40px;align-items:stretch'>"
        "<div id=a style='width:20px;height:10px;margin-top:auto'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(40, lot52_node("g")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l57_cross_auto_overrides_stretch_bottom(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:flex;width:80px;height:40px;align-items:stretch'>"
        "<div id=a style='width:20px;height:10px;margin-bottom:auto'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(40, lot52_node("g")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l57_cross_auto_row_reverse(void) {
    lot52_render("<body style='margin:0'><div style='display:flex;width:80px;height:40px;flex-direction:row-reverse'>"
        "<div id=a style='width:20px;height:10px;margin-top:auto;margin-left:auto'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(60, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l57_cross_auto_column_reverse(void) {
    lot52_render("<body style='margin:0'><div style='display:flex;width:80px;height:40px;flex-direction:column-reverse'>"
        "<div id=a style='width:20px;height:10px;margin-top:auto;margin-left:auto'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(60, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}
