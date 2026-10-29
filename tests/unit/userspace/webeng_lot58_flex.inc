static void test_l58_flex_basis_shrink(void) {
    lot52_render("<body style='margin:0'>"
        "<div id=g style='display:flex;width:100px;justify-content:start;align-items:start'>"
        "<div id=a style='height:10px;flex-basis:calc(60px + 20px)'></div>"
        "<div id=b style='height:10px;flex-basis:calc(40px * 2)'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(50, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(50, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(50, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->h);
    TEST_ASSERT_EQUAL(100, lot52_node("g")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("g")->h);
    TEST_ASSERT_EQUAL(1, g_doc.perf.flex_lines);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l58_flex_basis_percent(void) {
    lot52_render("<body style='margin:0'>"
        "<div id=g style='display:flex;width:100px;justify-content:start;align-items:start'>"
        "<div id=a style='height:10px;flex-basis:calc(50% - 10px)'></div>"
        "<div id=b style='height:10px;flex-basis:calc(50% - 10px)'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(40, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->h);
    TEST_ASSERT_EQUAL(100, lot52_node("g")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("g")->h);
    TEST_ASSERT_EQUAL(1, g_doc.perf.flex_lines);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l58_flex_basis_grow_zero(void) {
    lot52_render("<body style='margin:0'>"
        "<div id=g style='display:flex;width:100px;justify-content:start;align-items:start'>"
        "<div id=a style='height:10px;flex-basis:calc(10px + 10px);flex-grow:0'></div>"
        "<div id=b style='height:10px;flex-basis:calc(10px + 10px);flex-grow:1'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(20, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(80, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->h);
    TEST_ASSERT_EQUAL(100, lot52_node("g")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("g")->h);
    TEST_ASSERT_EQUAL(1, g_doc.perf.flex_lines);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l58_flex_basis_grow_weights(void) {
    lot52_render("<body style='margin:0'>"
        "<div id=g style='display:flex;width:100px;justify-content:start;align-items:start'>"
        "<div id=a style='height:10px;flex-basis:min(20px,10px);flex-grow:1'></div>"
        "<div id=b style='height:10px;flex-basis:min(20px,10px);flex-grow:3'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(30, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(70, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->h);
    TEST_ASSERT_EQUAL(100, lot52_node("g")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("g")->h);
    TEST_ASSERT_EQUAL(1, g_doc.perf.flex_lines);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l58_flex_basis_min_freeze(void) {
    lot52_render("<body style='margin:0'>"
        "<div id=g style='display:flex;width:100px;justify-content:start;align-items:start'>"
        "<div id=a style='height:10px;flex-basis:calc(80px);min-width:calc(60px + 10px)'></div>"
        "<div id=b style='height:10px;flex-basis:calc(80px)'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(70, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(70, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(30, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->h);
    TEST_ASSERT_EQUAL(100, lot52_node("g")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("g")->h);
    TEST_ASSERT_EQUAL(1, g_doc.perf.flex_lines);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l58_flex_basis_max_grow(void) {
    lot52_render("<body style='margin:0'>"
        "<div id=g style='display:flex;width:100px;justify-content:start;align-items:start'>"
        "<div id=a style='height:10px;flex-basis:calc(10px);flex-grow:1;max-width:calc(20px)'></div>"
        "<div id=b style='height:10px;flex-basis:calc(10px);flex-grow:1'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(20, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(80, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->h);
    TEST_ASSERT_EQUAL(100, lot52_node("g")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("g")->h);
    TEST_ASSERT_EQUAL(1, g_doc.perf.flex_lines);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l58_flex_width_percent(void) {
    lot52_render("<body style='margin:0'>"
        "<div id=g style='display:flex;width:100px;justify-content:start;align-items:start'>"
        "<div id=a style='height:10px;width:calc(50% - 10px);flex-grow:0'></div>"
        "<div id=b style='height:10px;width:20px;flex-grow:1'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(40, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(60, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->h);
    TEST_ASSERT_EQUAL(100, lot52_node("g")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("g")->h);
    TEST_ASSERT_EQUAL(1, g_doc.perf.flex_lines);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l58_flex_basis_content_width(void) {
    lot52_render("<body style='margin:0'>"
        "<div id=g style='display:flex;width:100px;justify-content:start;align-items:start'>"
        "<div id=a style='height:10px;width:calc(50% - 10px);flex-basis:auto'></div>"
        "<div id=b style='height:10px;width:20px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(40, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->h);
    TEST_ASSERT_EQUAL(100, lot52_node("g")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("g")->h);
    TEST_ASSERT_EQUAL(1, g_doc.perf.flex_lines);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l58_flex_basis_reset(void) {
    lot52_render("<body style='margin:0'>"
        "<div id=g style='display:flex;width:100px;justify-content:start;align-items:start'>"
        "<div id=a style='height:10px;flex-basis:calc(80px);flex-basis:20px'></div>"
        "<div id=b style='height:10px;flex-basis:calc(30px)'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(20, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(30, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->h);
    TEST_ASSERT_EQUAL(100, lot52_node("g")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("g")->h);
    TEST_ASSERT_EQUAL(1, g_doc.perf.flex_lines);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l58_flex_basis_zero(void) {
    lot52_render("<body style='margin:0'>"
        "<div id=g style='display:flex;width:100px;justify-content:start;align-items:start'>"
        "<div id=a style='height:10px;flex-basis:calc(10px - 10px);flex-grow:1'></div>"
        "<div id=b style='height:10px;flex-basis:calc(0px);flex-grow:1'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(50, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(50, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(50, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->h);
    TEST_ASSERT_EQUAL(100, lot52_node("g")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("g")->h);
    TEST_ASSERT_EQUAL(1, g_doc.perf.flex_lines);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l58_flex_column_basis_shrink(void) {
    lot52_render("<body style='margin:0'>"
        "<div id=g style='display:flex;flex-direction:column;width:100px;height:100px;justify-content:start;align-items:start'>"
        "<div id=a style='width:20px;flex-basis:calc(60px + 20px)'></div>"
        "<div id=b style='width:20px;flex-basis:calc(40px * 2)'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(50, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(50, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(50, lot52_node("b")->h);
    TEST_ASSERT_EQUAL(100, lot52_node("g")->h);
    TEST_ASSERT_EQUAL(1, g_doc.perf.flex_lines);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l58_flex_column_basis_percent(void) {
    lot52_render("<body style='margin:0'>"
        "<div id=g style='display:flex;flex-direction:column;width:100px;height:100px;justify-content:start;align-items:start'>"
        "<div id=a style='width:20px;flex-basis:calc(50% - 10px)'></div>"
        "<div id=b style='width:20px;flex-basis:calc(50% - 10px)'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(40, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(40, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(40, lot52_node("b")->h);
    TEST_ASSERT_EQUAL(100, lot52_node("g")->h);
    TEST_ASSERT_EQUAL(1, g_doc.perf.flex_lines);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l58_flex_column_basis_grow(void) {
    lot52_render("<body style='margin:0'>"
        "<div id=g style='display:flex;flex-direction:column;width:100px;height:100px;justify-content:start;align-items:start'>"
        "<div id=a style='width:20px;flex-basis:calc(10px);flex-grow:1'></div>"
        "<div id=b style='width:20px;flex-basis:calc(10px);flex-grow:3'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(30, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(70, lot52_node("b")->h);
    TEST_ASSERT_EQUAL(100, lot52_node("g")->h);
    TEST_ASSERT_EQUAL(1, g_doc.perf.flex_lines);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l58_flex_column_basis_min(void) {
    lot52_render("<body style='margin:0'>"
        "<div id=g style='display:flex;flex-direction:column;width:100px;height:100px;justify-content:start;align-items:start'>"
        "<div id=a style='width:20px;flex-basis:calc(80px);min-height:calc(60px + 10px)'></div>"
        "<div id=b style='width:20px;flex-basis:calc(80px)'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(70, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(70, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(30, lot52_node("b")->h);
    TEST_ASSERT_EQUAL(100, lot52_node("g")->h);
    TEST_ASSERT_EQUAL(1, g_doc.perf.flex_lines);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l58_flex_column_basis_max(void) {
    lot52_render("<body style='margin:0'>"
        "<div id=g style='display:flex;flex-direction:column;width:100px;height:100px;justify-content:start;align-items:start'>"
        "<div id=a style='width:20px;flex-basis:calc(10px);flex-grow:1;max-height:calc(20px)'></div>"
        "<div id=b style='width:20px;flex-basis:calc(10px);flex-grow:1'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(20, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(80, lot52_node("b")->h);
    TEST_ASSERT_EQUAL(100, lot52_node("g")->h);
    TEST_ASSERT_EQUAL(1, g_doc.perf.flex_lines);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l58_flex_column_basis_min_percent(void) {
    lot52_render("<body style='margin:0'>"
        "<div id=g style='display:flex;flex-direction:column;width:100px;height:100px;justify-content:start;align-items:start'>"
        "<div id=a style='width:20px;flex-basis:calc(80px);min-height:max(20px,70%)'></div>"
        "<div id=b style='width:20px;flex-basis:calc(80px)'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(70, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(70, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(30, lot52_node("b")->h);
    TEST_ASSERT_EQUAL(100, lot52_node("g")->h);
    TEST_ASSERT_EQUAL(1, g_doc.perf.flex_lines);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l58_flex_column_basis_max_percent(void) {
    lot52_render("<body style='margin:0'>"
        "<div id=g style='display:flex;flex-direction:column;width:100px;height:100px;justify-content:start;align-items:start'>"
        "<div id=a style='width:20px;flex-basis:calc(10px);flex-grow:1;max-height:min(40px,20%)'></div>"
        "<div id=b style='width:20px;flex-basis:calc(10px);flex-grow:1'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(20, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(80, lot52_node("b")->h);
    TEST_ASSERT_EQUAL(100, lot52_node("g")->h);
    TEST_ASSERT_EQUAL(1, g_doc.perf.flex_lines);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l58_flex_column_height_percent(void) {
    lot52_render("<body style='margin:0'>"
        "<div id=g style='display:flex;flex-direction:column;width:100px;height:100px;justify-content:start;align-items:start'>"
        "<div id=a style='width:20px;height:calc(50% - 10px)'></div>"
        "<div id=b style='width:20px;height:calc(30px)'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(40, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(40, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(30, lot52_node("b")->h);
    TEST_ASSERT_EQUAL(100, lot52_node("g")->h);
    TEST_ASSERT_EQUAL(1, g_doc.perf.flex_lines);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}
