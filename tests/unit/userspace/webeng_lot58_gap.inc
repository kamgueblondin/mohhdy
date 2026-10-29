static void test_l58_gap_gap_calc(void) {
    lot52_render("<body style='margin:0'>"
        "<div id=g style='display:grid;width:100px;grid-template-columns:40px 40px;gap:calc(2px + 2px)'>"
        "<div id=a style='height:10px'></div><div id=b style='height:10px'></div><div id=c style='height:10px'>"
        "</div></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(44, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("c")->x);
    TEST_ASSERT_EQUAL(14, lot52_node("c")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("c")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("c")->h);
    TEST_ASSERT_EQUAL(24, lot52_node("g")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l58_gap_gap_two_functions(void) {
    lot52_render("<body style='margin:0'>"
        "<div id=g style='display:grid;width:100px;grid-template-columns:40px 40px;gap:calc(2px + 4px) min(10px,8px)'>"
        "<div id=a style='height:10px'></div><div id=b style='height:10px'></div><div id=c style='height:10px'>"
        "</div></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(48, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("c")->x);
    TEST_ASSERT_EQUAL(16, lot52_node("c")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("c")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("c")->h);
    TEST_ASSERT_EQUAL(26, lot52_node("g")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l58_gap_gap_mixed(void) {
    lot52_render("<body style='margin:0'>"
        "<div id=g style='display:grid;width:100px;grid-template-columns:40px 40px;gap:6px calc(2px + 6px)'>"
        "<div id=a style='height:10px'></div><div id=b style='height:10px'></div><div id=c style='height:10px'>"
        "</div></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(48, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("c")->x);
    TEST_ASSERT_EQUAL(16, lot52_node("c")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("c")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("c")->h);
    TEST_ASSERT_EQUAL(26, lot52_node("g")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l58_gap_axis_gaps(void) {
    lot52_render("<body style='margin:0'>"
        "<div id=g style='display:grid;width:100px;grid-template-columns:40px 40px;row-gap:calc(2px + 4px);column-gap:calc(2px + 6px)'>"
        "<div id=a style='height:10px'></div><div id=b style='height:10px'></div><div id=c style='height:10px'>"
        "</div></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(48, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("c")->x);
    TEST_ASSERT_EQUAL(16, lot52_node("c")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("c")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("c")->h);
    TEST_ASSERT_EQUAL(26, lot52_node("g")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l58_gap_gap_clamp(void) {
    lot52_render("<body style='margin:0'>"
        "<div id=g style='display:grid;width:100px;grid-template-columns:40px 40px;gap:clamp(2px,8px,6px)'>"
        "<div id=a style='height:10px'></div><div id=b style='height:10px'></div><div id=c style='height:10px'>"
        "</div></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(46, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("c")->x);
    TEST_ASSERT_EQUAL(16, lot52_node("c")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("c")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("c")->h);
    TEST_ASSERT_EQUAL(26, lot52_node("g")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l58_gap_gap_negative(void) {
    lot52_render("<body style='margin:0'>"
        "<div id=g style='display:grid;width:100px;grid-template-columns:40px 40px;gap:calc(-2px)'>"
        "<div id=a style='height:10px'></div><div id=b style='height:10px'></div><div id=c style='height:10px'>"
        "</div></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(40, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("c")->x);
    TEST_ASSERT_EQUAL(10, lot52_node("c")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("c")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("c")->h);
    TEST_ASSERT_EQUAL(20, lot52_node("g")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l58_gap_gap_reset(void) {
    lot52_render("<body style='margin:0'>"
        "<div id=g style='display:grid;width:100px;grid-template-columns:40px 40px;gap:calc(20px);gap:4px'>"
        "<div id=a style='height:10px'></div><div id=b style='height:10px'></div><div id=c style='height:10px'>"
        "</div></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(44, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("c")->x);
    TEST_ASSERT_EQUAL(14, lot52_node("c")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("c")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("c")->h);
    TEST_ASSERT_EQUAL(24, lot52_node("g")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l58_gap_axis_reset(void) {
    lot52_render("<body style='margin:0'>"
        "<div id=g style='display:grid;width:100px;grid-template-columns:40px 40px;row-gap:calc(20px);row-gap:4px;column-gap:calc(20px);column-gap:4px'>"
        "<div id=a style='height:10px'></div><div id=b style='height:10px'></div><div id=c style='height:10px'>"
        "</div></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(44, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("c")->x);
    TEST_ASSERT_EQUAL(14, lot52_node("c")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("c")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("c")->h);
    TEST_ASSERT_EQUAL(24, lot52_node("g")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l58_gap_bad_gap(void) {
    lot52_render("<body style='margin:0'>"
        "<div id=g style='display:grid;width:100px;grid-template-columns:40px 40px;gap:4px;gap:calc(10px / 0)'>"
        "<div id=a style='height:10px'></div><div id=b style='height:10px'></div><div id=c style='height:10px'>"
        "</div></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(44, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("c")->x);
    TEST_ASSERT_EQUAL(14, lot52_node("c")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("c")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("c")->h);
    TEST_ASSERT_EQUAL(24, lot52_node("g")->h);
    TEST_ASSERT_TRUE(lot52_has_diag("gap refused"));
}

static void test_l58_gap_bad_gap_second(void) {
    lot52_render("<body style='margin:0'>"
        "<div id=g style='display:grid;width:100px;grid-template-columns:40px 40px;gap:4px;gap:8px calc(10px / 0)'>"
        "<div id=a style='height:10px'></div><div id=b style='height:10px'></div><div id=c style='height:10px'>"
        "</div></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(44, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("c")->x);
    TEST_ASSERT_EQUAL(14, lot52_node("c")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("c")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("c")->h);
    TEST_ASSERT_EQUAL(24, lot52_node("g")->h);
    TEST_ASSERT_TRUE(lot52_has_diag("gap refused"));
}

static void test_l58_gap_percent_gap(void) {
    lot52_render("<body style='margin:0'>"
        "<div id=g style='display:grid;width:100px;grid-template-columns:40px 40px;gap:4px;gap:calc(10% + 2px)'>"
        "<div id=a style='height:10px'></div><div id=b style='height:10px'></div><div id=c style='height:10px'>"
        "</div></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(44, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("c")->x);
    TEST_ASSERT_EQUAL(14, lot52_node("c")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("c")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("c")->h);
    TEST_ASSERT_EQUAL(24, lot52_node("g")->h);
    TEST_ASSERT_TRUE(lot52_has_diag("gap refused"));
}

static void test_l58_gap_too_many_gap(void) {
    lot52_render("<body style='margin:0'>"
        "<div id=g style='display:grid;width:100px;grid-template-columns:40px 40px;gap:4px;gap:calc(2px) 3px 4px'>"
        "<div id=a style='height:10px'></div><div id=b style='height:10px'></div><div id=c style='height:10px'>"
        "</div></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(44, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("c")->x);
    TEST_ASSERT_EQUAL(14, lot52_node("c")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("c")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("c")->h);
    TEST_ASSERT_EQUAL(24, lot52_node("g")->h);
    TEST_ASSERT_TRUE(lot52_has_diag("gap refused"));
}

static void test_l58_gap_flex_gap_math(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:flex;flex-wrap:wrap;width:100px;gap:calc(2px + 2px)'>"
        "<div id=a style='width:40px;height:10px;flex-grow:0'></div>"
        "<div id=b style='width:40px;height:10px;flex-grow:0'></div>"
        "<div id=c style='width:40px;height:10px;flex-grow:0'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(44, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("c")->x);
    TEST_ASSERT_EQUAL(14, lot52_node("c")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("c")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("c")->h);
    TEST_ASSERT_EQUAL(2, g_doc.perf.flex_lines);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l58_gap_flex_gap_axes(void) {
    lot52_render("<body style='margin:0'>"
        "<div id=g style='display:flex;flex-wrap:wrap;width:100px;row-gap:calc(6px);column-gap:calc(8px)'>"
        "<div id=a style='width:40px;height:10px;flex-grow:0'></div>"
        "<div id=b style='width:40px;height:10px;flex-grow:0'></div>"
        "<div id=c style='width:40px;height:10px;flex-grow:0'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(48, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("c")->x);
    TEST_ASSERT_EQUAL(16, lot52_node("c")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("c")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("c")->h);
    TEST_ASSERT_EQUAL(2, g_doc.perf.flex_lines);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}
