static void test_l57_flow_row(void) {
    lot52_render("<body style='margin:0'>"
        "<div id=g style='display:grid;width:120px;grid-template-columns:repeat(3,1fr);grid-auto-flow:row;'>"
        "<div id=a style='grid-column:span 2;height:10px'></div><div id=b style='grid-column:span 2;height:10px'></div>"
        "<div id=c style='height:10px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(80, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(80, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->h);
    TEST_ASSERT_EQUAL(80, lot52_node("c")->x);
    TEST_ASSERT_EQUAL(10, lot52_node("c")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("c")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("c")->h);
    TEST_ASSERT_EQUAL(2, g_doc.perf.grid_lines);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l57_flow_dense(void) {
    lot52_render("<body style='margin:0'>"
        "<div id=g style='display:grid;width:120px;grid-template-columns:repeat(3,1fr);grid-auto-flow:dense;'>"
        "<div id=a style='grid-column:span 2;height:10px'></div><div id=b style='grid-column:span 2;height:10px'></div>"
        "<div id=c style='height:10px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(80, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(80, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->h);
    TEST_ASSERT_EQUAL(80, lot52_node("c")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("c")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("c")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("c")->h);
    TEST_ASSERT_EQUAL(2, g_doc.perf.grid_lines);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l57_flow_row_dense(void) {
    lot52_render("<body style='margin:0'>"
        "<div id=g style='display:grid;width:120px;grid-template-columns:repeat(3,1fr);grid-auto-flow:row dense;'>"
        "<div id=a style='grid-column:span 2;height:10px'></div><div id=b style='grid-column:span 2;height:10px'></div>"
        "<div id=c style='height:10px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(80, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(80, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->h);
    TEST_ASSERT_EQUAL(80, lot52_node("c")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("c")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("c")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("c")->h);
    TEST_ASSERT_EQUAL(2, g_doc.perf.grid_lines);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l57_flow_dense_row(void) {
    lot52_render("<body style='margin:0'>"
        "<div id=g style='display:grid;width:120px;grid-template-columns:repeat(3,1fr);grid-auto-flow:dense row;'>"
        "<div id=a style='grid-column:span 2;height:10px'></div><div id=b style='grid-column:span 2;height:10px'></div>"
        "<div id=c style='height:10px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(80, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(80, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->h);
    TEST_ASSERT_EQUAL(80, lot52_node("c")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("c")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("c")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("c")->h);
    TEST_ASSERT_EQUAL(2, g_doc.perf.grid_lines);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l57_flow_column(void) {
    lot52_render("<body style='margin:0'>"
        "<div id=g style='display:grid;width:80px;grid-template-columns:repeat(2,1fr);grid-template-rows:repeat(3,10px);grid-auto-flow:column;'>"
        "<div id=a style='grid-row:span 2;height:20px'></div><div id=b style='grid-row:span 2;height:20px'></div>"
        "<div id=c style='height:10px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(40, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(20, lot52_node("b")->h);
    TEST_ASSERT_EQUAL(40, lot52_node("c")->x);
    TEST_ASSERT_EQUAL(20, lot52_node("c")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("c")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("c")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l57_flow_column_dense(void) {
    lot52_render("<body style='margin:0'>"
        "<div id=g style='display:grid;width:80px;grid-template-columns:repeat(2,1fr);grid-template-rows:repeat(3,10px);grid-auto-flow:column dense;'>"
        "<div id=a style='grid-row:span 2;height:20px'></div><div id=b style='grid-row:span 2;height:20px'></div>"
        "<div id=c style='height:10px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(40, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(20, lot52_node("b")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("c")->x);
    TEST_ASSERT_EQUAL(20, lot52_node("c")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("c")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("c")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l57_flow_dense_column(void) {
    lot52_render("<body style='margin:0'>"
        "<div id=g style='display:grid;width:80px;grid-template-columns:repeat(2,1fr);grid-template-rows:repeat(3,10px);grid-auto-flow:dense column;'>"
        "<div id=a style='grid-row:span 2;height:20px'></div><div id=b style='grid-row:span 2;height:20px'></div>"
        "<div id=c style='height:10px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(40, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(20, lot52_node("b")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("c")->x);
    TEST_ASSERT_EQUAL(20, lot52_node("c")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("c")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("c")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l57_column_rows_7(void) {
    lot52_render("<body style='margin:0'>"
        "<div id=g style='display:grid;width:80px;grid-template-columns:repeat(2,1fr);grid-template-rows:10px 20px;grid-auto-flow:column;'>"
        "<div id=a style='height:10px'></div><div id=b style='height:10px'></div><div id=c style='height:10px'></div>"
        "<div id=d style='height:10px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->h);
    TEST_ASSERT_EQUAL(40, lot52_node("c")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("c")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("c")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("c")->h);
    TEST_ASSERT_EQUAL(40, lot52_node("d")->x);
    TEST_ASSERT_EQUAL(10, lot52_node("d")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("d")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("d")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l57_column_rows_8(void) {
    lot52_render("<body style='margin:0'>"
        "<div id=g style='display:grid;width:80px;grid-template-columns:repeat(2,1fr);grid-template-rows:repeat(3,10px);grid-auto-flow:column;'>"
        "<div id=a style='height:10px'></div><div id=b style='height:10px'></div><div id=c style='height:10px'></div>"
        "<div id=d style='height:10px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("c")->x);
    TEST_ASSERT_EQUAL(20, lot52_node("c")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("c")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("c")->h);
    TEST_ASSERT_EQUAL(40, lot52_node("d")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("d")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("d")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("d")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l57_bad_flow_9(void) {
    lot52_render("<body style='margin:0'>"
        "<div id=g style='display:grid;width:120px;grid-template-columns:1fr 1fr;grid-auto-flow:column;grid-template-rows:repeat(2,10px);grid-auto-flow:bogus;'>"
        "<div id=a style='height:10px;background:red'></div><div id=b style='height:10px;background:blue'></div></div>"
        "</body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(60, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(60, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->h);
    TEST_ASSERT_TRUE(lot52_has_diag("grid flow refused"));
    TEST_ASSERT_EQUAL(1, lot52_node("g")->grid_column_flow);
}

static void test_l57_bad_flow_10(void) {
    lot52_render("<body style='margin:0'>"
        "<div id=g style='display:grid;width:120px;grid-template-columns:1fr 1fr;grid-auto-flow:column;grid-template-rows:repeat(2,10px);grid-auto-flow:row column;'>"
        "<div id=a style='height:10px;background:red'></div><div id=b style='height:10px;background:blue'></div></div>"
        "</body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(60, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(60, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->h);
    TEST_ASSERT_TRUE(lot52_has_diag("grid flow refused"));
    TEST_ASSERT_EQUAL(1, lot52_node("g")->grid_column_flow);
}

static void test_l57_bad_flow_11(void) {
    lot52_render("<body style='margin:0'>"
        "<div id=g style='display:grid;width:120px;grid-template-columns:1fr 1fr;grid-auto-flow:column;grid-template-rows:repeat(2,10px);grid-auto-flow:dense dense;'>"
        "<div id=a style='height:10px;background:red'></div><div id=b style='height:10px;background:blue'></div></div>"
        "</body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(60, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(60, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->h);
    TEST_ASSERT_TRUE(lot52_has_diag("grid flow refused"));
    TEST_ASSERT_EQUAL(1, lot52_node("g")->grid_column_flow);
}

static void test_l57_bad_flow_12(void) {
    lot52_render("<body style='margin:0'>"
        "<div id=g style='display:grid;width:120px;grid-template-columns:1fr 1fr;grid-auto-flow:column;grid-template-rows:repeat(2,10px);grid-auto-flow:column row;'>"
        "<div id=a style='height:10px;background:red'></div><div id=b style='height:10px;background:blue'></div></div>"
        "</body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(60, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(60, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->h);
    TEST_ASSERT_TRUE(lot52_has_diag("grid flow refused"));
    TEST_ASSERT_EQUAL(1, lot52_node("g")->grid_column_flow);
}

static void test_l57_bad_flow_13(void) {
    lot52_render("<body style='margin:0'>"
        "<div id=g style='display:grid;width:120px;grid-template-columns:1fr 1fr;grid-auto-flow:column;grid-template-rows:repeat(2,10px);grid-auto-flow:row dense dense;'>"
        "<div id=a style='height:10px;background:red'></div><div id=b style='height:10px;background:blue'></div></div>"
        "</body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(60, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(60, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->h);
    TEST_ASSERT_TRUE(lot52_has_diag("grid flow refused"));
    TEST_ASSERT_EQUAL(1, lot52_node("g")->grid_column_flow);
}

static void test_l57_bad_flow_14(void) {
    lot52_render("<body style='margin:0'>"
        "<div id=g style='display:grid;width:120px;grid-template-columns:1fr 1fr;grid-auto-flow:column;grid-template-rows:repeat(2,10px);grid-auto-flow:row junk;'>"
        "<div id=a style='height:10px;background:red'></div><div id=b style='height:10px;background:blue'></div></div>"
        "</body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(60, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(60, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->h);
    TEST_ASSERT_TRUE(lot52_has_diag("grid flow refused"));
    TEST_ASSERT_EQUAL(1, lot52_node("g")->grid_column_flow);
}

static void test_l57_implicit_row_15(void) {
    lot52_render("<body style='margin:0'>"
        "<div id=g style='display:grid;width:120px;grid-template-columns:1fr;grid-auto-rows:20px;'>"
        "<div id=a style='height:10px;background:red'></div><div id=b style='height:10px;background:blue'></div></div>"
        "</body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(120, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(20, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(120, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->h);
    TEST_ASSERT_EQUAL(40, lot52_node("g")->h);
    TEST_ASSERT_EQUAL(2, g_doc.perf.grid_lines);
}

static void test_l57_implicit_row_16(void) {
    lot52_render("<body style='margin:0'>"
        "<div id=g style='display:grid;width:120px;grid-template-columns:1fr;grid-auto-rows:minmax(20px,40px);'>"
        "<div id=a style='height:10px;background:red'></div><div id=b style='height:10px;background:blue'></div></div>"
        "</body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(120, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(20, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(120, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->h);
    TEST_ASSERT_EQUAL(40, lot52_node("g")->h);
    TEST_ASSERT_EQUAL(2, g_doc.perf.grid_lines);
}

static void test_l57_implicit_row_17(void) {
    lot52_render("<body style='margin:0'>"
        "<div id=g style='display:grid;width:120px;grid-template-columns:1fr;grid-auto-rows:fit-content(5px);'>"
        "<div id=a style='height:10px;background:red'></div><div id=b style='height:10px;background:blue'></div></div>"
        "</body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(120, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(5, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(120, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->h);
    TEST_ASSERT_EQUAL(10, lot52_node("g")->h);
    TEST_ASSERT_EQUAL(2, g_doc.perf.grid_lines);
}

static void test_l57_implicit_row_18(void) {
    lot52_render("<body style='margin:0'>"
        "<div id=g style='display:grid;width:120px;grid-template-columns:1fr;grid-auto-rows:auto;'>"
        "<div id=a style='height:10px;background:red'></div><div id=b style='height:10px;background:blue'></div></div>"
        "</body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(120, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(120, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->h);
    TEST_ASSERT_EQUAL(20, lot52_node("g")->h);
    TEST_ASSERT_EQUAL(2, g_doc.perf.grid_lines);
}

static void test_l57_implicit_col_19(void) {
    lot52_render("<body style='margin:0'>"
        "<div id=g style='display:grid;width:120px;grid-template-columns:60px;grid-auto-columns:20px;grid-auto-flow:column;grid-template-rows:10px;'>"
        "<div id=a style='height:10px;background:red'></div><div id=b style='height:10px;background:blue'></div></div>"
        "</body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(60, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(60, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l57_implicit_col_20(void) {
    lot52_render("<body style='margin:0'>"
        "<div id=g style='display:grid;width:120px;grid-template-columns:60px;grid-auto-columns:minmax(20px,40px);grid-auto-flow:column;grid-template-rows:10px;'>"
        "<div id=a style='height:10px;background:red'></div><div id=b style='height:10px;background:blue'></div></div>"
        "</body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(60, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(60, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l57_implicit_col_21(void) {
    lot52_render("<body style='margin:0'>"
        "<div id=g style='display:grid;width:120px;grid-template-columns:60px;grid-auto-columns:fit-content(5px);grid-auto-flow:column;grid-template-rows:10px;'>"
        "<div id=a style='height:10px;background:red'></div><div id=b style='height:10px;background:blue'></div></div>"
        "</body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(60, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(60, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l57_implicit_col_22(void) {
    lot52_render("<body style='margin:0'>"
        "<div id=g style='display:grid;width:120px;grid-template-columns:60px;grid-auto-columns:1fr;grid-auto-flow:column;grid-template-rows:10px;'>"
        "<div id=a style='height:10px;background:red'></div><div id=b style='height:10px;background:blue'></div></div>"
        "</body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(60, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(60, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(60, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}
