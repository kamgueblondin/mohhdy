static void test_l60_lines_column_1_2(void) {
    lot52_render("<body style='margin:0'><div id=p style='display:grid;width:160px;grid-template-columns:[L1] 20px [L2] 20px [L3"
        "] 20px [L4] 20px [L5] 20px [L6] 20px [L7] 20px [L8] 20px [L9];grid-template-rows:10px;align-items:stretch'><di"
        "v id=a style='grid-column:L1 / L2;background:red'>A</div></div><div id=b style='height:5px'></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(1, lot52_node("a")->grid_col_start);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->grid_row_start);
    TEST_ASSERT_EQUAL(1, lot52_node("a")->grid_col_span);
    TEST_ASSERT_EQUAL(1, lot52_node("a")->grid_row_span);
    TEST_ASSERT_NOT_NULL(find_text("A"));
    TEST_ASSERT_EQUAL(0, find_text("A")->x);
    TEST_ASSERT_EQUAL(1, find_text("A")->y);
    TEST_ASSERT_EQUAL(160, lot52_node("p")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("p")->h);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l60_lines_column_1_3(void) {
    lot52_render("<body style='margin:0'><div id=p style='display:grid;width:160px;grid-template-columns:[L1] 20px [L2] 20px [L3"
        "] 20px [L4] 20px [L5] 20px [L6] 20px [L7] 20px [L8] 20px [L9];grid-template-rows:10px;align-items:stretch'><di"
        "v id=a style='grid-column:L1 / L3;background:red'>A</div></div><div id=b style='height:5px'></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(1, lot52_node("a")->grid_col_start);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->grid_row_start);
    TEST_ASSERT_EQUAL(2, lot52_node("a")->grid_col_span);
    TEST_ASSERT_EQUAL(1, lot52_node("a")->grid_row_span);
    TEST_ASSERT_NOT_NULL(find_text("A"));
    TEST_ASSERT_EQUAL(0, find_text("A")->x);
    TEST_ASSERT_EQUAL(1, find_text("A")->y);
    TEST_ASSERT_EQUAL(160, lot52_node("p")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("p")->h);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l60_lines_column_1_4(void) {
    lot52_render("<body style='margin:0'><div id=p style='display:grid;width:160px;grid-template-columns:[L1] 20px [L2] 20px [L3"
        "] 20px [L4] 20px [L5] 20px [L6] 20px [L7] 20px [L8] 20px [L9];grid-template-rows:10px;align-items:stretch'><di"
        "v id=a style='grid-column:L1 / L4;background:red'>A</div></div><div id=b style='height:5px'></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(60, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(1, lot52_node("a")->grid_col_start);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->grid_row_start);
    TEST_ASSERT_EQUAL(3, lot52_node("a")->grid_col_span);
    TEST_ASSERT_EQUAL(1, lot52_node("a")->grid_row_span);
    TEST_ASSERT_NOT_NULL(find_text("A"));
    TEST_ASSERT_EQUAL(0, find_text("A")->x);
    TEST_ASSERT_EQUAL(1, find_text("A")->y);
    TEST_ASSERT_EQUAL(160, lot52_node("p")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("p")->h);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l60_lines_column_1_5(void) {
    lot52_render("<body style='margin:0'><div id=p style='display:grid;width:160px;grid-template-columns:[L1] 20px [L2] 20px [L3"
        "] 20px [L4] 20px [L5] 20px [L6] 20px [L7] 20px [L8] 20px [L9];grid-template-rows:10px;align-items:stretch'><di"
        "v id=a style='grid-column:L1 / L5;background:red'>A</div></div><div id=b style='height:5px'></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(80, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(1, lot52_node("a")->grid_col_start);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->grid_row_start);
    TEST_ASSERT_EQUAL(4, lot52_node("a")->grid_col_span);
    TEST_ASSERT_EQUAL(1, lot52_node("a")->grid_row_span);
    TEST_ASSERT_NOT_NULL(find_text("A"));
    TEST_ASSERT_EQUAL(0, find_text("A")->x);
    TEST_ASSERT_EQUAL(1, find_text("A")->y);
    TEST_ASSERT_EQUAL(160, lot52_node("p")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("p")->h);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l60_lines_column_1_6(void) {
    lot52_render("<body style='margin:0'><div id=p style='display:grid;width:160px;grid-template-columns:[L1] 20px [L2] 20px [L3"
        "] 20px [L4] 20px [L5] 20px [L6] 20px [L7] 20px [L8] 20px [L9];grid-template-rows:10px;align-items:stretch'><di"
        "v id=a style='grid-column:L1 / L6;background:red'>A</div></div><div id=b style='height:5px'></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(100, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(1, lot52_node("a")->grid_col_start);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->grid_row_start);
    TEST_ASSERT_EQUAL(5, lot52_node("a")->grid_col_span);
    TEST_ASSERT_EQUAL(1, lot52_node("a")->grid_row_span);
    TEST_ASSERT_NOT_NULL(find_text("A"));
    TEST_ASSERT_EQUAL(0, find_text("A")->x);
    TEST_ASSERT_EQUAL(1, find_text("A")->y);
    TEST_ASSERT_EQUAL(160, lot52_node("p")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("p")->h);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l60_lines_column_1_7(void) {
    lot52_render("<body style='margin:0'><div id=p style='display:grid;width:160px;grid-template-columns:[L1] 20px [L2] 20px [L3"
        "] 20px [L4] 20px [L5] 20px [L6] 20px [L7] 20px [L8] 20px [L9];grid-template-rows:10px;align-items:stretch'><di"
        "v id=a style='grid-column:L1 / L7;background:red'>A</div></div><div id=b style='height:5px'></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(120, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(1, lot52_node("a")->grid_col_start);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->grid_row_start);
    TEST_ASSERT_EQUAL(6, lot52_node("a")->grid_col_span);
    TEST_ASSERT_EQUAL(1, lot52_node("a")->grid_row_span);
    TEST_ASSERT_NOT_NULL(find_text("A"));
    TEST_ASSERT_EQUAL(0, find_text("A")->x);
    TEST_ASSERT_EQUAL(1, find_text("A")->y);
    TEST_ASSERT_EQUAL(160, lot52_node("p")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("p")->h);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l60_lines_column_1_8(void) {
    lot52_render("<body style='margin:0'><div id=p style='display:grid;width:160px;grid-template-columns:[L1] 20px [L2] 20px [L3"
        "] 20px [L4] 20px [L5] 20px [L6] 20px [L7] 20px [L8] 20px [L9];grid-template-rows:10px;align-items:stretch'><di"
        "v id=a style='grid-column:L1 / L8;background:red'>A</div></div><div id=b style='height:5px'></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(140, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(1, lot52_node("a")->grid_col_start);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->grid_row_start);
    TEST_ASSERT_EQUAL(7, lot52_node("a")->grid_col_span);
    TEST_ASSERT_EQUAL(1, lot52_node("a")->grid_row_span);
    TEST_ASSERT_NOT_NULL(find_text("A"));
    TEST_ASSERT_EQUAL(0, find_text("A")->x);
    TEST_ASSERT_EQUAL(1, find_text("A")->y);
    TEST_ASSERT_EQUAL(160, lot52_node("p")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("p")->h);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l60_lines_column_1_9(void) {
    lot52_render("<body style='margin:0'><div id=p style='display:grid;width:160px;grid-template-columns:[L1] 20px [L2] 20px [L3"
        "] 20px [L4] 20px [L5] 20px [L6] 20px [L7] 20px [L8] 20px [L9];grid-template-rows:10px;align-items:stretch'><di"
        "v id=a style='grid-column:L1 / L9;background:red'>A</div></div><div id=b style='height:5px'></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(160, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(1, lot52_node("a")->grid_col_start);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->grid_row_start);
    TEST_ASSERT_EQUAL(8, lot52_node("a")->grid_col_span);
    TEST_ASSERT_EQUAL(1, lot52_node("a")->grid_row_span);
    TEST_ASSERT_NOT_NULL(find_text("A"));
    TEST_ASSERT_EQUAL(0, find_text("A")->x);
    TEST_ASSERT_EQUAL(1, find_text("A")->y);
    TEST_ASSERT_EQUAL(160, lot52_node("p")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("p")->h);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l60_lines_column_2_3(void) {
    lot52_render("<body style='margin:0'><div id=p style='display:grid;width:160px;grid-template-columns:[L1] 20px [L2] 20px [L3"
        "] 20px [L4] 20px [L5] 20px [L6] 20px [L7] 20px [L8] 20px [L9];grid-template-rows:10px;align-items:stretch'><di"
        "v id=a style='grid-column:L2 / L3;background:red'>A</div></div><div id=b style='height:5px'></div></body>", 320);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(2, lot52_node("a")->grid_col_start);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->grid_row_start);
    TEST_ASSERT_EQUAL(1, lot52_node("a")->grid_col_span);
    TEST_ASSERT_EQUAL(1, lot52_node("a")->grid_row_span);
    TEST_ASSERT_NOT_NULL(find_text("A"));
    TEST_ASSERT_EQUAL(20, find_text("A")->x);
    TEST_ASSERT_EQUAL(1, find_text("A")->y);
    TEST_ASSERT_EQUAL(160, lot52_node("p")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("p")->h);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l60_lines_column_2_4(void) {
    lot52_render("<body style='margin:0'><div id=p style='display:grid;width:160px;grid-template-columns:[L1] 20px [L2] 20px [L3"
        "] 20px [L4] 20px [L5] 20px [L6] 20px [L7] 20px [L8] 20px [L9];grid-template-rows:10px;align-items:stretch'><di"
        "v id=a style='grid-column:L2 / L4;background:red'>A</div></div><div id=b style='height:5px'></div></body>", 320);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(2, lot52_node("a")->grid_col_start);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->grid_row_start);
    TEST_ASSERT_EQUAL(2, lot52_node("a")->grid_col_span);
    TEST_ASSERT_EQUAL(1, lot52_node("a")->grid_row_span);
    TEST_ASSERT_NOT_NULL(find_text("A"));
    TEST_ASSERT_EQUAL(20, find_text("A")->x);
    TEST_ASSERT_EQUAL(1, find_text("A")->y);
    TEST_ASSERT_EQUAL(160, lot52_node("p")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("p")->h);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l60_lines_column_2_5(void) {
    lot52_render("<body style='margin:0'><div id=p style='display:grid;width:160px;grid-template-columns:[L1] 20px [L2] 20px [L3"
        "] 20px [L4] 20px [L5] 20px [L6] 20px [L7] 20px [L8] 20px [L9];grid-template-rows:10px;align-items:stretch'><di"
        "v id=a style='grid-column:L2 / L5;background:red'>A</div></div><div id=b style='height:5px'></div></body>", 320);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(60, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(2, lot52_node("a")->grid_col_start);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->grid_row_start);
    TEST_ASSERT_EQUAL(3, lot52_node("a")->grid_col_span);
    TEST_ASSERT_EQUAL(1, lot52_node("a")->grid_row_span);
    TEST_ASSERT_NOT_NULL(find_text("A"));
    TEST_ASSERT_EQUAL(20, find_text("A")->x);
    TEST_ASSERT_EQUAL(1, find_text("A")->y);
    TEST_ASSERT_EQUAL(160, lot52_node("p")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("p")->h);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l60_lines_column_2_6(void) {
    lot52_render("<body style='margin:0'><div id=p style='display:grid;width:160px;grid-template-columns:[L1] 20px [L2] 20px [L3"
        "] 20px [L4] 20px [L5] 20px [L6] 20px [L7] 20px [L8] 20px [L9];grid-template-rows:10px;align-items:stretch'><di"
        "v id=a style='grid-column:L2 / L6;background:red'>A</div></div><div id=b style='height:5px'></div></body>", 320);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(80, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(2, lot52_node("a")->grid_col_start);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->grid_row_start);
    TEST_ASSERT_EQUAL(4, lot52_node("a")->grid_col_span);
    TEST_ASSERT_EQUAL(1, lot52_node("a")->grid_row_span);
    TEST_ASSERT_NOT_NULL(find_text("A"));
    TEST_ASSERT_EQUAL(20, find_text("A")->x);
    TEST_ASSERT_EQUAL(1, find_text("A")->y);
    TEST_ASSERT_EQUAL(160, lot52_node("p")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("p")->h);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l60_lines_column_2_7(void) {
    lot52_render("<body style='margin:0'><div id=p style='display:grid;width:160px;grid-template-columns:[L1] 20px [L2] 20px [L3"
        "] 20px [L4] 20px [L5] 20px [L6] 20px [L7] 20px [L8] 20px [L9];grid-template-rows:10px;align-items:stretch'><di"
        "v id=a style='grid-column:L2 / L7;background:red'>A</div></div><div id=b style='height:5px'></div></body>", 320);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(100, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(2, lot52_node("a")->grid_col_start);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->grid_row_start);
    TEST_ASSERT_EQUAL(5, lot52_node("a")->grid_col_span);
    TEST_ASSERT_EQUAL(1, lot52_node("a")->grid_row_span);
    TEST_ASSERT_NOT_NULL(find_text("A"));
    TEST_ASSERT_EQUAL(20, find_text("A")->x);
    TEST_ASSERT_EQUAL(1, find_text("A")->y);
    TEST_ASSERT_EQUAL(160, lot52_node("p")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("p")->h);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l60_lines_column_2_8(void) {
    lot52_render("<body style='margin:0'><div id=p style='display:grid;width:160px;grid-template-columns:[L1] 20px [L2] 20px [L3"
        "] 20px [L4] 20px [L5] 20px [L6] 20px [L7] 20px [L8] 20px [L9];grid-template-rows:10px;align-items:stretch'><di"
        "v id=a style='grid-column:L2 / L8;background:red'>A</div></div><div id=b style='height:5px'></div></body>", 320);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(120, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(2, lot52_node("a")->grid_col_start);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->grid_row_start);
    TEST_ASSERT_EQUAL(6, lot52_node("a")->grid_col_span);
    TEST_ASSERT_EQUAL(1, lot52_node("a")->grid_row_span);
    TEST_ASSERT_NOT_NULL(find_text("A"));
    TEST_ASSERT_EQUAL(20, find_text("A")->x);
    TEST_ASSERT_EQUAL(1, find_text("A")->y);
    TEST_ASSERT_EQUAL(160, lot52_node("p")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("p")->h);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l60_lines_column_2_9(void) {
    lot52_render("<body style='margin:0'><div id=p style='display:grid;width:160px;grid-template-columns:[L1] 20px [L2] 20px [L3"
        "] 20px [L4] 20px [L5] 20px [L6] 20px [L7] 20px [L8] 20px [L9];grid-template-rows:10px;align-items:stretch'><di"
        "v id=a style='grid-column:L2 / L9;background:red'>A</div></div><div id=b style='height:5px'></div></body>", 320);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(140, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(2, lot52_node("a")->grid_col_start);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->grid_row_start);
    TEST_ASSERT_EQUAL(7, lot52_node("a")->grid_col_span);
    TEST_ASSERT_EQUAL(1, lot52_node("a")->grid_row_span);
    TEST_ASSERT_NOT_NULL(find_text("A"));
    TEST_ASSERT_EQUAL(20, find_text("A")->x);
    TEST_ASSERT_EQUAL(1, find_text("A")->y);
    TEST_ASSERT_EQUAL(160, lot52_node("p")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("p")->h);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l60_lines_column_3_4(void) {
    lot52_render("<body style='margin:0'><div id=p style='display:grid;width:160px;grid-template-columns:[L1] 20px [L2] 20px [L3"
        "] 20px [L4] 20px [L5] 20px [L6] 20px [L7] 20px [L8] 20px [L9];grid-template-rows:10px;align-items:stretch'><di"
        "v id=a style='grid-column:L3 / L4;background:red'>A</div></div><div id=b style='height:5px'></div></body>", 320);
    TEST_ASSERT_EQUAL(40, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(3, lot52_node("a")->grid_col_start);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->grid_row_start);
    TEST_ASSERT_EQUAL(1, lot52_node("a")->grid_col_span);
    TEST_ASSERT_EQUAL(1, lot52_node("a")->grid_row_span);
    TEST_ASSERT_NOT_NULL(find_text("A"));
    TEST_ASSERT_EQUAL(40, find_text("A")->x);
    TEST_ASSERT_EQUAL(1, find_text("A")->y);
    TEST_ASSERT_EQUAL(160, lot52_node("p")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("p")->h);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l60_lines_column_3_5(void) {
    lot52_render("<body style='margin:0'><div id=p style='display:grid;width:160px;grid-template-columns:[L1] 20px [L2] 20px [L3"
        "] 20px [L4] 20px [L5] 20px [L6] 20px [L7] 20px [L8] 20px [L9];grid-template-rows:10px;align-items:stretch'><di"
        "v id=a style='grid-column:L3 / L5;background:red'>A</div></div><div id=b style='height:5px'></div></body>", 320);
    TEST_ASSERT_EQUAL(40, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(3, lot52_node("a")->grid_col_start);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->grid_row_start);
    TEST_ASSERT_EQUAL(2, lot52_node("a")->grid_col_span);
    TEST_ASSERT_EQUAL(1, lot52_node("a")->grid_row_span);
    TEST_ASSERT_NOT_NULL(find_text("A"));
    TEST_ASSERT_EQUAL(40, find_text("A")->x);
    TEST_ASSERT_EQUAL(1, find_text("A")->y);
    TEST_ASSERT_EQUAL(160, lot52_node("p")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("p")->h);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l60_lines_column_3_6(void) {
    lot52_render("<body style='margin:0'><div id=p style='display:grid;width:160px;grid-template-columns:[L1] 20px [L2] 20px [L3"
        "] 20px [L4] 20px [L5] 20px [L6] 20px [L7] 20px [L8] 20px [L9];grid-template-rows:10px;align-items:stretch'><di"
        "v id=a style='grid-column:L3 / L6;background:red'>A</div></div><div id=b style='height:5px'></div></body>", 320);
    TEST_ASSERT_EQUAL(40, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(60, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(3, lot52_node("a")->grid_col_start);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->grid_row_start);
    TEST_ASSERT_EQUAL(3, lot52_node("a")->grid_col_span);
    TEST_ASSERT_EQUAL(1, lot52_node("a")->grid_row_span);
    TEST_ASSERT_NOT_NULL(find_text("A"));
    TEST_ASSERT_EQUAL(40, find_text("A")->x);
    TEST_ASSERT_EQUAL(1, find_text("A")->y);
    TEST_ASSERT_EQUAL(160, lot52_node("p")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("p")->h);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l60_lines_column_3_7(void) {
    lot52_render("<body style='margin:0'><div id=p style='display:grid;width:160px;grid-template-columns:[L1] 20px [L2] 20px [L3"
        "] 20px [L4] 20px [L5] 20px [L6] 20px [L7] 20px [L8] 20px [L9];grid-template-rows:10px;align-items:stretch'><di"
        "v id=a style='grid-column:L3 / L7;background:red'>A</div></div><div id=b style='height:5px'></div></body>", 320);
    TEST_ASSERT_EQUAL(40, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(80, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(3, lot52_node("a")->grid_col_start);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->grid_row_start);
    TEST_ASSERT_EQUAL(4, lot52_node("a")->grid_col_span);
    TEST_ASSERT_EQUAL(1, lot52_node("a")->grid_row_span);
    TEST_ASSERT_NOT_NULL(find_text("A"));
    TEST_ASSERT_EQUAL(40, find_text("A")->x);
    TEST_ASSERT_EQUAL(1, find_text("A")->y);
    TEST_ASSERT_EQUAL(160, lot52_node("p")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("p")->h);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l60_lines_column_3_8(void) {
    lot52_render("<body style='margin:0'><div id=p style='display:grid;width:160px;grid-template-columns:[L1] 20px [L2] 20px [L3"
        "] 20px [L4] 20px [L5] 20px [L6] 20px [L7] 20px [L8] 20px [L9];grid-template-rows:10px;align-items:stretch'><di"
        "v id=a style='grid-column:L3 / L8;background:red'>A</div></div><div id=b style='height:5px'></div></body>", 320);
    TEST_ASSERT_EQUAL(40, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(100, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(3, lot52_node("a")->grid_col_start);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->grid_row_start);
    TEST_ASSERT_EQUAL(5, lot52_node("a")->grid_col_span);
    TEST_ASSERT_EQUAL(1, lot52_node("a")->grid_row_span);
    TEST_ASSERT_NOT_NULL(find_text("A"));
    TEST_ASSERT_EQUAL(40, find_text("A")->x);
    TEST_ASSERT_EQUAL(1, find_text("A")->y);
    TEST_ASSERT_EQUAL(160, lot52_node("p")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("p")->h);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l60_lines_column_3_9(void) {
    lot52_render("<body style='margin:0'><div id=p style='display:grid;width:160px;grid-template-columns:[L1] 20px [L2] 20px [L3"
        "] 20px [L4] 20px [L5] 20px [L6] 20px [L7] 20px [L8] 20px [L9];grid-template-rows:10px;align-items:stretch'><di"
        "v id=a style='grid-column:L3 / L9;background:red'>A</div></div><div id=b style='height:5px'></div></body>", 320);
    TEST_ASSERT_EQUAL(40, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(120, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(3, lot52_node("a")->grid_col_start);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->grid_row_start);
    TEST_ASSERT_EQUAL(6, lot52_node("a")->grid_col_span);
    TEST_ASSERT_EQUAL(1, lot52_node("a")->grid_row_span);
    TEST_ASSERT_NOT_NULL(find_text("A"));
    TEST_ASSERT_EQUAL(40, find_text("A")->x);
    TEST_ASSERT_EQUAL(1, find_text("A")->y);
    TEST_ASSERT_EQUAL(160, lot52_node("p")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("p")->h);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l60_lines_column_4_5(void) {
    lot52_render("<body style='margin:0'><div id=p style='display:grid;width:160px;grid-template-columns:[L1] 20px [L2] 20px [L3"
        "] 20px [L4] 20px [L5] 20px [L6] 20px [L7] 20px [L8] 20px [L9];grid-template-rows:10px;align-items:stretch'><di"
        "v id=a style='grid-column:L4 / L5;background:red'>A</div></div><div id=b style='height:5px'></div></body>", 320);
    TEST_ASSERT_EQUAL(60, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(4, lot52_node("a")->grid_col_start);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->grid_row_start);
    TEST_ASSERT_EQUAL(1, lot52_node("a")->grid_col_span);
    TEST_ASSERT_EQUAL(1, lot52_node("a")->grid_row_span);
    TEST_ASSERT_NOT_NULL(find_text("A"));
    TEST_ASSERT_EQUAL(60, find_text("A")->x);
    TEST_ASSERT_EQUAL(1, find_text("A")->y);
    TEST_ASSERT_EQUAL(160, lot52_node("p")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("p")->h);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l60_lines_column_4_6(void) {
    lot52_render("<body style='margin:0'><div id=p style='display:grid;width:160px;grid-template-columns:[L1] 20px [L2] 20px [L3"
        "] 20px [L4] 20px [L5] 20px [L6] 20px [L7] 20px [L8] 20px [L9];grid-template-rows:10px;align-items:stretch'><di"
        "v id=a style='grid-column:L4 / L6;background:red'>A</div></div><div id=b style='height:5px'></div></body>", 320);
    TEST_ASSERT_EQUAL(60, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(4, lot52_node("a")->grid_col_start);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->grid_row_start);
    TEST_ASSERT_EQUAL(2, lot52_node("a")->grid_col_span);
    TEST_ASSERT_EQUAL(1, lot52_node("a")->grid_row_span);
    TEST_ASSERT_NOT_NULL(find_text("A"));
    TEST_ASSERT_EQUAL(60, find_text("A")->x);
    TEST_ASSERT_EQUAL(1, find_text("A")->y);
    TEST_ASSERT_EQUAL(160, lot52_node("p")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("p")->h);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l60_lines_column_4_7(void) {
    lot52_render("<body style='margin:0'><div id=p style='display:grid;width:160px;grid-template-columns:[L1] 20px [L2] 20px [L3"
        "] 20px [L4] 20px [L5] 20px [L6] 20px [L7] 20px [L8] 20px [L9];grid-template-rows:10px;align-items:stretch'><di"
        "v id=a style='grid-column:L4 / L7;background:red'>A</div></div><div id=b style='height:5px'></div></body>", 320);
    TEST_ASSERT_EQUAL(60, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(60, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(4, lot52_node("a")->grid_col_start);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->grid_row_start);
    TEST_ASSERT_EQUAL(3, lot52_node("a")->grid_col_span);
    TEST_ASSERT_EQUAL(1, lot52_node("a")->grid_row_span);
    TEST_ASSERT_NOT_NULL(find_text("A"));
    TEST_ASSERT_EQUAL(60, find_text("A")->x);
    TEST_ASSERT_EQUAL(1, find_text("A")->y);
    TEST_ASSERT_EQUAL(160, lot52_node("p")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("p")->h);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l60_lines_column_4_8(void) {
    lot52_render("<body style='margin:0'><div id=p style='display:grid;width:160px;grid-template-columns:[L1] 20px [L2] 20px [L3"
        "] 20px [L4] 20px [L5] 20px [L6] 20px [L7] 20px [L8] 20px [L9];grid-template-rows:10px;align-items:stretch'><di"
        "v id=a style='grid-column:L4 / L8;background:red'>A</div></div><div id=b style='height:5px'></div></body>", 320);
    TEST_ASSERT_EQUAL(60, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(80, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(4, lot52_node("a")->grid_col_start);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->grid_row_start);
    TEST_ASSERT_EQUAL(4, lot52_node("a")->grid_col_span);
    TEST_ASSERT_EQUAL(1, lot52_node("a")->grid_row_span);
    TEST_ASSERT_NOT_NULL(find_text("A"));
    TEST_ASSERT_EQUAL(60, find_text("A")->x);
    TEST_ASSERT_EQUAL(1, find_text("A")->y);
    TEST_ASSERT_EQUAL(160, lot52_node("p")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("p")->h);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l60_lines_column_4_9(void) {
    lot52_render("<body style='margin:0'><div id=p style='display:grid;width:160px;grid-template-columns:[L1] 20px [L2] 20px [L3"
        "] 20px [L4] 20px [L5] 20px [L6] 20px [L7] 20px [L8] 20px [L9];grid-template-rows:10px;align-items:stretch'><di"
        "v id=a style='grid-column:L4 / L9;background:red'>A</div></div><div id=b style='height:5px'></div></body>", 320);
    TEST_ASSERT_EQUAL(60, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(100, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(4, lot52_node("a")->grid_col_start);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->grid_row_start);
    TEST_ASSERT_EQUAL(5, lot52_node("a")->grid_col_span);
    TEST_ASSERT_EQUAL(1, lot52_node("a")->grid_row_span);
    TEST_ASSERT_NOT_NULL(find_text("A"));
    TEST_ASSERT_EQUAL(60, find_text("A")->x);
    TEST_ASSERT_EQUAL(1, find_text("A")->y);
    TEST_ASSERT_EQUAL(160, lot52_node("p")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("p")->h);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l60_lines_column_5_6(void) {
    lot52_render("<body style='margin:0'><div id=p style='display:grid;width:160px;grid-template-columns:[L1] 20px [L2] 20px [L3"
        "] 20px [L4] 20px [L5] 20px [L6] 20px [L7] 20px [L8] 20px [L9];grid-template-rows:10px;align-items:stretch'><di"
        "v id=a style='grid-column:L5 / L6;background:red'>A</div></div><div id=b style='height:5px'></div></body>", 320);
    TEST_ASSERT_EQUAL(80, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(5, lot52_node("a")->grid_col_start);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->grid_row_start);
    TEST_ASSERT_EQUAL(1, lot52_node("a")->grid_col_span);
    TEST_ASSERT_EQUAL(1, lot52_node("a")->grid_row_span);
    TEST_ASSERT_NOT_NULL(find_text("A"));
    TEST_ASSERT_EQUAL(80, find_text("A")->x);
    TEST_ASSERT_EQUAL(1, find_text("A")->y);
    TEST_ASSERT_EQUAL(160, lot52_node("p")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("p")->h);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l60_lines_column_5_7(void) {
    lot52_render("<body style='margin:0'><div id=p style='display:grid;width:160px;grid-template-columns:[L1] 20px [L2] 20px [L3"
        "] 20px [L4] 20px [L5] 20px [L6] 20px [L7] 20px [L8] 20px [L9];grid-template-rows:10px;align-items:stretch'><di"
        "v id=a style='grid-column:L5 / L7;background:red'>A</div></div><div id=b style='height:5px'></div></body>", 320);
    TEST_ASSERT_EQUAL(80, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(5, lot52_node("a")->grid_col_start);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->grid_row_start);
    TEST_ASSERT_EQUAL(2, lot52_node("a")->grid_col_span);
    TEST_ASSERT_EQUAL(1, lot52_node("a")->grid_row_span);
    TEST_ASSERT_NOT_NULL(find_text("A"));
    TEST_ASSERT_EQUAL(80, find_text("A")->x);
    TEST_ASSERT_EQUAL(1, find_text("A")->y);
    TEST_ASSERT_EQUAL(160, lot52_node("p")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("p")->h);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l60_lines_column_5_8(void) {
    lot52_render("<body style='margin:0'><div id=p style='display:grid;width:160px;grid-template-columns:[L1] 20px [L2] 20px [L3"
        "] 20px [L4] 20px [L5] 20px [L6] 20px [L7] 20px [L8] 20px [L9];grid-template-rows:10px;align-items:stretch'><di"
        "v id=a style='grid-column:L5 / L8;background:red'>A</div></div><div id=b style='height:5px'></div></body>", 320);
    TEST_ASSERT_EQUAL(80, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(60, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(5, lot52_node("a")->grid_col_start);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->grid_row_start);
    TEST_ASSERT_EQUAL(3, lot52_node("a")->grid_col_span);
    TEST_ASSERT_EQUAL(1, lot52_node("a")->grid_row_span);
    TEST_ASSERT_NOT_NULL(find_text("A"));
    TEST_ASSERT_EQUAL(80, find_text("A")->x);
    TEST_ASSERT_EQUAL(1, find_text("A")->y);
    TEST_ASSERT_EQUAL(160, lot52_node("p")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("p")->h);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l60_lines_column_5_9(void) {
    lot52_render("<body style='margin:0'><div id=p style='display:grid;width:160px;grid-template-columns:[L1] 20px [L2] 20px [L3"
        "] 20px [L4] 20px [L5] 20px [L6] 20px [L7] 20px [L8] 20px [L9];grid-template-rows:10px;align-items:stretch'><di"
        "v id=a style='grid-column:L5 / L9;background:red'>A</div></div><div id=b style='height:5px'></div></body>", 320);
    TEST_ASSERT_EQUAL(80, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(80, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(5, lot52_node("a")->grid_col_start);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->grid_row_start);
    TEST_ASSERT_EQUAL(4, lot52_node("a")->grid_col_span);
    TEST_ASSERT_EQUAL(1, lot52_node("a")->grid_row_span);
    TEST_ASSERT_NOT_NULL(find_text("A"));
    TEST_ASSERT_EQUAL(80, find_text("A")->x);
    TEST_ASSERT_EQUAL(1, find_text("A")->y);
    TEST_ASSERT_EQUAL(160, lot52_node("p")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("p")->h);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l60_lines_column_6_7(void) {
    lot52_render("<body style='margin:0'><div id=p style='display:grid;width:160px;grid-template-columns:[L1] 20px [L2] 20px [L3"
        "] 20px [L4] 20px [L5] 20px [L6] 20px [L7] 20px [L8] 20px [L9];grid-template-rows:10px;align-items:stretch'><di"
        "v id=a style='grid-column:L6 / L7;background:red'>A</div></div><div id=b style='height:5px'></div></body>", 320);
    TEST_ASSERT_EQUAL(100, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(6, lot52_node("a")->grid_col_start);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->grid_row_start);
    TEST_ASSERT_EQUAL(1, lot52_node("a")->grid_col_span);
    TEST_ASSERT_EQUAL(1, lot52_node("a")->grid_row_span);
    TEST_ASSERT_NOT_NULL(find_text("A"));
    TEST_ASSERT_EQUAL(100, find_text("A")->x);
    TEST_ASSERT_EQUAL(1, find_text("A")->y);
    TEST_ASSERT_EQUAL(160, lot52_node("p")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("p")->h);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l60_lines_column_6_8(void) {
    lot52_render("<body style='margin:0'><div id=p style='display:grid;width:160px;grid-template-columns:[L1] 20px [L2] 20px [L3"
        "] 20px [L4] 20px [L5] 20px [L6] 20px [L7] 20px [L8] 20px [L9];grid-template-rows:10px;align-items:stretch'><di"
        "v id=a style='grid-column:L6 / L8;background:red'>A</div></div><div id=b style='height:5px'></div></body>", 320);
    TEST_ASSERT_EQUAL(100, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(6, lot52_node("a")->grid_col_start);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->grid_row_start);
    TEST_ASSERT_EQUAL(2, lot52_node("a")->grid_col_span);
    TEST_ASSERT_EQUAL(1, lot52_node("a")->grid_row_span);
    TEST_ASSERT_NOT_NULL(find_text("A"));
    TEST_ASSERT_EQUAL(100, find_text("A")->x);
    TEST_ASSERT_EQUAL(1, find_text("A")->y);
    TEST_ASSERT_EQUAL(160, lot52_node("p")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("p")->h);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l60_lines_column_6_9(void) {
    lot52_render("<body style='margin:0'><div id=p style='display:grid;width:160px;grid-template-columns:[L1] 20px [L2] 20px [L3"
        "] 20px [L4] 20px [L5] 20px [L6] 20px [L7] 20px [L8] 20px [L9];grid-template-rows:10px;align-items:stretch'><di"
        "v id=a style='grid-column:L6 / L9;background:red'>A</div></div><div id=b style='height:5px'></div></body>", 320);
    TEST_ASSERT_EQUAL(100, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(60, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(6, lot52_node("a")->grid_col_start);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->grid_row_start);
    TEST_ASSERT_EQUAL(3, lot52_node("a")->grid_col_span);
    TEST_ASSERT_EQUAL(1, lot52_node("a")->grid_row_span);
    TEST_ASSERT_NOT_NULL(find_text("A"));
    TEST_ASSERT_EQUAL(100, find_text("A")->x);
    TEST_ASSERT_EQUAL(1, find_text("A")->y);
    TEST_ASSERT_EQUAL(160, lot52_node("p")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("p")->h);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l60_lines_column_7_8(void) {
    lot52_render("<body style='margin:0'><div id=p style='display:grid;width:160px;grid-template-columns:[L1] 20px [L2] 20px [L3"
        "] 20px [L4] 20px [L5] 20px [L6] 20px [L7] 20px [L8] 20px [L9];grid-template-rows:10px;align-items:stretch'><di"
        "v id=a style='grid-column:L7 / L8;background:red'>A</div></div><div id=b style='height:5px'></div></body>", 320);
    TEST_ASSERT_EQUAL(120, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(7, lot52_node("a")->grid_col_start);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->grid_row_start);
    TEST_ASSERT_EQUAL(1, lot52_node("a")->grid_col_span);
    TEST_ASSERT_EQUAL(1, lot52_node("a")->grid_row_span);
    TEST_ASSERT_NOT_NULL(find_text("A"));
    TEST_ASSERT_EQUAL(120, find_text("A")->x);
    TEST_ASSERT_EQUAL(1, find_text("A")->y);
    TEST_ASSERT_EQUAL(160, lot52_node("p")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("p")->h);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l60_lines_column_7_9(void) {
    lot52_render("<body style='margin:0'><div id=p style='display:grid;width:160px;grid-template-columns:[L1] 20px [L2] 20px [L3"
        "] 20px [L4] 20px [L5] 20px [L6] 20px [L7] 20px [L8] 20px [L9];grid-template-rows:10px;align-items:stretch'><di"
        "v id=a style='grid-column:L7 / L9;background:red'>A</div></div><div id=b style='height:5px'></div></body>", 320);
    TEST_ASSERT_EQUAL(120, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(7, lot52_node("a")->grid_col_start);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->grid_row_start);
    TEST_ASSERT_EQUAL(2, lot52_node("a")->grid_col_span);
    TEST_ASSERT_EQUAL(1, lot52_node("a")->grid_row_span);
    TEST_ASSERT_NOT_NULL(find_text("A"));
    TEST_ASSERT_EQUAL(120, find_text("A")->x);
    TEST_ASSERT_EQUAL(1, find_text("A")->y);
    TEST_ASSERT_EQUAL(160, lot52_node("p")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("p")->h);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l60_lines_column_8_9(void) {
    lot52_render("<body style='margin:0'><div id=p style='display:grid;width:160px;grid-template-columns:[L1] 20px [L2] 20px [L3"
        "] 20px [L4] 20px [L5] 20px [L6] 20px [L7] 20px [L8] 20px [L9];grid-template-rows:10px;align-items:stretch'><di"
        "v id=a style='grid-column:L8 / L9;background:red'>A</div></div><div id=b style='height:5px'></div></body>", 320);
    TEST_ASSERT_EQUAL(140, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(8, lot52_node("a")->grid_col_start);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->grid_row_start);
    TEST_ASSERT_EQUAL(1, lot52_node("a")->grid_col_span);
    TEST_ASSERT_EQUAL(1, lot52_node("a")->grid_row_span);
    TEST_ASSERT_NOT_NULL(find_text("A"));
    TEST_ASSERT_EQUAL(140, find_text("A")->x);
    TEST_ASSERT_EQUAL(1, find_text("A")->y);
    TEST_ASSERT_EQUAL(160, lot52_node("p")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("p")->h);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l60_lines_row_1_2(void) {
    lot52_render("<body style='margin:0'><div id=p style='display:grid;width:160px;grid-template-columns:40px;grid-template-rows"
        ":[L1] 10px [L2] 10px [L3] 10px [L4] 10px [L5] 10px [L6] 10px [L7] 10px [L8] 10px [L9];align-items:stretch'><di"
        "v id=a style='grid-row:L1 / L2;background:red'>A</div></div><div id=b style='height:5px'></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->grid_col_start);
    TEST_ASSERT_EQUAL(1, lot52_node("a")->grid_row_start);
    TEST_ASSERT_EQUAL(1, lot52_node("a")->grid_col_span);
    TEST_ASSERT_EQUAL(1, lot52_node("a")->grid_row_span);
    TEST_ASSERT_NOT_NULL(find_text("A"));
    TEST_ASSERT_EQUAL(0, find_text("A")->x);
    TEST_ASSERT_EQUAL(1, find_text("A")->y);
    TEST_ASSERT_EQUAL(160, lot52_node("p")->w);
    TEST_ASSERT_EQUAL(80, lot52_node("p")->h);
    TEST_ASSERT_EQUAL(80, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l60_lines_row_1_3(void) {
    lot52_render("<body style='margin:0'><div id=p style='display:grid;width:160px;grid-template-columns:40px;grid-template-rows"
        ":[L1] 10px [L2] 10px [L3] 10px [L4] 10px [L5] 10px [L6] 10px [L7] 10px [L8] 10px [L9];align-items:stretch'><di"
        "v id=a style='grid-row:L1 / L3;background:red'>A</div></div><div id=b style='height:5px'></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->grid_col_start);
    TEST_ASSERT_EQUAL(1, lot52_node("a")->grid_row_start);
    TEST_ASSERT_EQUAL(1, lot52_node("a")->grid_col_span);
    TEST_ASSERT_EQUAL(2, lot52_node("a")->grid_row_span);
    TEST_ASSERT_NOT_NULL(find_text("A"));
    TEST_ASSERT_EQUAL(0, find_text("A")->x);
    TEST_ASSERT_EQUAL(1, find_text("A")->y);
    TEST_ASSERT_EQUAL(160, lot52_node("p")->w);
    TEST_ASSERT_EQUAL(80, lot52_node("p")->h);
    TEST_ASSERT_EQUAL(80, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l60_lines_row_1_4(void) {
    lot52_render("<body style='margin:0'><div id=p style='display:grid;width:160px;grid-template-columns:40px;grid-template-rows"
        ":[L1] 10px [L2] 10px [L3] 10px [L4] 10px [L5] 10px [L6] 10px [L7] 10px [L8] 10px [L9];align-items:stretch'><di"
        "v id=a style='grid-row:L1 / L4;background:red'>A</div></div><div id=b style='height:5px'></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->grid_col_start);
    TEST_ASSERT_EQUAL(1, lot52_node("a")->grid_row_start);
    TEST_ASSERT_EQUAL(1, lot52_node("a")->grid_col_span);
    TEST_ASSERT_EQUAL(3, lot52_node("a")->grid_row_span);
    TEST_ASSERT_NOT_NULL(find_text("A"));
    TEST_ASSERT_EQUAL(0, find_text("A")->x);
    TEST_ASSERT_EQUAL(1, find_text("A")->y);
    TEST_ASSERT_EQUAL(160, lot52_node("p")->w);
    TEST_ASSERT_EQUAL(80, lot52_node("p")->h);
    TEST_ASSERT_EQUAL(80, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l60_lines_row_1_5(void) {
    lot52_render("<body style='margin:0'><div id=p style='display:grid;width:160px;grid-template-columns:40px;grid-template-rows"
        ":[L1] 10px [L2] 10px [L3] 10px [L4] 10px [L5] 10px [L6] 10px [L7] 10px [L8] 10px [L9];align-items:stretch'><di"
        "v id=a style='grid-row:L1 / L5;background:red'>A</div></div><div id=b style='height:5px'></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(40, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->grid_col_start);
    TEST_ASSERT_EQUAL(1, lot52_node("a")->grid_row_start);
    TEST_ASSERT_EQUAL(1, lot52_node("a")->grid_col_span);
    TEST_ASSERT_EQUAL(4, lot52_node("a")->grid_row_span);
    TEST_ASSERT_NOT_NULL(find_text("A"));
    TEST_ASSERT_EQUAL(0, find_text("A")->x);
    TEST_ASSERT_EQUAL(1, find_text("A")->y);
    TEST_ASSERT_EQUAL(160, lot52_node("p")->w);
    TEST_ASSERT_EQUAL(80, lot52_node("p")->h);
    TEST_ASSERT_EQUAL(80, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l60_lines_row_1_6(void) {
    lot52_render("<body style='margin:0'><div id=p style='display:grid;width:160px;grid-template-columns:40px;grid-template-rows"
        ":[L1] 10px [L2] 10px [L3] 10px [L4] 10px [L5] 10px [L6] 10px [L7] 10px [L8] 10px [L9];align-items:stretch'><di"
        "v id=a style='grid-row:L1 / L6;background:red'>A</div></div><div id=b style='height:5px'></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(50, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->grid_col_start);
    TEST_ASSERT_EQUAL(1, lot52_node("a")->grid_row_start);
    TEST_ASSERT_EQUAL(1, lot52_node("a")->grid_col_span);
    TEST_ASSERT_EQUAL(5, lot52_node("a")->grid_row_span);
    TEST_ASSERT_NOT_NULL(find_text("A"));
    TEST_ASSERT_EQUAL(0, find_text("A")->x);
    TEST_ASSERT_EQUAL(1, find_text("A")->y);
    TEST_ASSERT_EQUAL(160, lot52_node("p")->w);
    TEST_ASSERT_EQUAL(80, lot52_node("p")->h);
    TEST_ASSERT_EQUAL(80, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l60_lines_row_1_7(void) {
    lot52_render("<body style='margin:0'><div id=p style='display:grid;width:160px;grid-template-columns:40px;grid-template-rows"
        ":[L1] 10px [L2] 10px [L3] 10px [L4] 10px [L5] 10px [L6] 10px [L7] 10px [L8] 10px [L9];align-items:stretch'><di"
        "v id=a style='grid-row:L1 / L7;background:red'>A</div></div><div id=b style='height:5px'></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(60, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->grid_col_start);
    TEST_ASSERT_EQUAL(1, lot52_node("a")->grid_row_start);
    TEST_ASSERT_EQUAL(1, lot52_node("a")->grid_col_span);
    TEST_ASSERT_EQUAL(6, lot52_node("a")->grid_row_span);
    TEST_ASSERT_NOT_NULL(find_text("A"));
    TEST_ASSERT_EQUAL(0, find_text("A")->x);
    TEST_ASSERT_EQUAL(1, find_text("A")->y);
    TEST_ASSERT_EQUAL(160, lot52_node("p")->w);
    TEST_ASSERT_EQUAL(80, lot52_node("p")->h);
    TEST_ASSERT_EQUAL(80, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l60_lines_row_1_8(void) {
    lot52_render("<body style='margin:0'><div id=p style='display:grid;width:160px;grid-template-columns:40px;grid-template-rows"
        ":[L1] 10px [L2] 10px [L3] 10px [L4] 10px [L5] 10px [L6] 10px [L7] 10px [L8] 10px [L9];align-items:stretch'><di"
        "v id=a style='grid-row:L1 / L8;background:red'>A</div></div><div id=b style='height:5px'></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(70, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->grid_col_start);
    TEST_ASSERT_EQUAL(1, lot52_node("a")->grid_row_start);
    TEST_ASSERT_EQUAL(1, lot52_node("a")->grid_col_span);
    TEST_ASSERT_EQUAL(7, lot52_node("a")->grid_row_span);
    TEST_ASSERT_NOT_NULL(find_text("A"));
    TEST_ASSERT_EQUAL(0, find_text("A")->x);
    TEST_ASSERT_EQUAL(1, find_text("A")->y);
    TEST_ASSERT_EQUAL(160, lot52_node("p")->w);
    TEST_ASSERT_EQUAL(80, lot52_node("p")->h);
    TEST_ASSERT_EQUAL(80, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l60_lines_row_1_9(void) {
    lot52_render("<body style='margin:0'><div id=p style='display:grid;width:160px;grid-template-columns:40px;grid-template-rows"
        ":[L1] 10px [L2] 10px [L3] 10px [L4] 10px [L5] 10px [L6] 10px [L7] 10px [L8] 10px [L9];align-items:stretch'><di"
        "v id=a style='grid-row:L1 / L9;background:red'>A</div></div><div id=b style='height:5px'></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(80, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->grid_col_start);
    TEST_ASSERT_EQUAL(1, lot52_node("a")->grid_row_start);
    TEST_ASSERT_EQUAL(1, lot52_node("a")->grid_col_span);
    TEST_ASSERT_EQUAL(8, lot52_node("a")->grid_row_span);
    TEST_ASSERT_NOT_NULL(find_text("A"));
    TEST_ASSERT_EQUAL(0, find_text("A")->x);
    TEST_ASSERT_EQUAL(1, find_text("A")->y);
    TEST_ASSERT_EQUAL(160, lot52_node("p")->w);
    TEST_ASSERT_EQUAL(80, lot52_node("p")->h);
    TEST_ASSERT_EQUAL(80, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l60_lines_row_2_3(void) {
    lot52_render("<body style='margin:0'><div id=p style='display:grid;width:160px;grid-template-columns:40px;grid-template-rows"
        ":[L1] 10px [L2] 10px [L3] 10px [L4] 10px [L5] 10px [L6] 10px [L7] 10px [L8] 10px [L9];align-items:stretch'><di"
        "v id=a style='grid-row:L2 / L3;background:red'>A</div></div><div id=b style='height:5px'></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->grid_col_start);
    TEST_ASSERT_EQUAL(2, lot52_node("a")->grid_row_start);
    TEST_ASSERT_EQUAL(1, lot52_node("a")->grid_col_span);
    TEST_ASSERT_EQUAL(1, lot52_node("a")->grid_row_span);
    TEST_ASSERT_NOT_NULL(find_text("A"));
    TEST_ASSERT_EQUAL(0, find_text("A")->x);
    TEST_ASSERT_EQUAL(11, find_text("A")->y);
    TEST_ASSERT_EQUAL(160, lot52_node("p")->w);
    TEST_ASSERT_EQUAL(80, lot52_node("p")->h);
    TEST_ASSERT_EQUAL(80, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l60_lines_row_2_4(void) {
    lot52_render("<body style='margin:0'><div id=p style='display:grid;width:160px;grid-template-columns:40px;grid-template-rows"
        ":[L1] 10px [L2] 10px [L3] 10px [L4] 10px [L5] 10px [L6] 10px [L7] 10px [L8] 10px [L9];align-items:stretch'><di"
        "v id=a style='grid-row:L2 / L4;background:red'>A</div></div><div id=b style='height:5px'></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->grid_col_start);
    TEST_ASSERT_EQUAL(2, lot52_node("a")->grid_row_start);
    TEST_ASSERT_EQUAL(1, lot52_node("a")->grid_col_span);
    TEST_ASSERT_EQUAL(2, lot52_node("a")->grid_row_span);
    TEST_ASSERT_NOT_NULL(find_text("A"));
    TEST_ASSERT_EQUAL(0, find_text("A")->x);
    TEST_ASSERT_EQUAL(11, find_text("A")->y);
    TEST_ASSERT_EQUAL(160, lot52_node("p")->w);
    TEST_ASSERT_EQUAL(80, lot52_node("p")->h);
    TEST_ASSERT_EQUAL(80, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l60_lines_row_2_5(void) {
    lot52_render("<body style='margin:0'><div id=p style='display:grid;width:160px;grid-template-columns:40px;grid-template-rows"
        ":[L1] 10px [L2] 10px [L3] 10px [L4] 10px [L5] 10px [L6] 10px [L7] 10px [L8] 10px [L9];align-items:stretch'><di"
        "v id=a style='grid-row:L2 / L5;background:red'>A</div></div><div id=b style='height:5px'></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->grid_col_start);
    TEST_ASSERT_EQUAL(2, lot52_node("a")->grid_row_start);
    TEST_ASSERT_EQUAL(1, lot52_node("a")->grid_col_span);
    TEST_ASSERT_EQUAL(3, lot52_node("a")->grid_row_span);
    TEST_ASSERT_NOT_NULL(find_text("A"));
    TEST_ASSERT_EQUAL(0, find_text("A")->x);
    TEST_ASSERT_EQUAL(11, find_text("A")->y);
    TEST_ASSERT_EQUAL(160, lot52_node("p")->w);
    TEST_ASSERT_EQUAL(80, lot52_node("p")->h);
    TEST_ASSERT_EQUAL(80, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l60_lines_row_2_6(void) {
    lot52_render("<body style='margin:0'><div id=p style='display:grid;width:160px;grid-template-columns:40px;grid-template-rows"
        ":[L1] 10px [L2] 10px [L3] 10px [L4] 10px [L5] 10px [L6] 10px [L7] 10px [L8] 10px [L9];align-items:stretch'><di"
        "v id=a style='grid-row:L2 / L6;background:red'>A</div></div><div id=b style='height:5px'></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(40, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->grid_col_start);
    TEST_ASSERT_EQUAL(2, lot52_node("a")->grid_row_start);
    TEST_ASSERT_EQUAL(1, lot52_node("a")->grid_col_span);
    TEST_ASSERT_EQUAL(4, lot52_node("a")->grid_row_span);
    TEST_ASSERT_NOT_NULL(find_text("A"));
    TEST_ASSERT_EQUAL(0, find_text("A")->x);
    TEST_ASSERT_EQUAL(11, find_text("A")->y);
    TEST_ASSERT_EQUAL(160, lot52_node("p")->w);
    TEST_ASSERT_EQUAL(80, lot52_node("p")->h);
    TEST_ASSERT_EQUAL(80, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l60_lines_row_2_7(void) {
    lot52_render("<body style='margin:0'><div id=p style='display:grid;width:160px;grid-template-columns:40px;grid-template-rows"
        ":[L1] 10px [L2] 10px [L3] 10px [L4] 10px [L5] 10px [L6] 10px [L7] 10px [L8] 10px [L9];align-items:stretch'><di"
        "v id=a style='grid-row:L2 / L7;background:red'>A</div></div><div id=b style='height:5px'></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(50, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->grid_col_start);
    TEST_ASSERT_EQUAL(2, lot52_node("a")->grid_row_start);
    TEST_ASSERT_EQUAL(1, lot52_node("a")->grid_col_span);
    TEST_ASSERT_EQUAL(5, lot52_node("a")->grid_row_span);
    TEST_ASSERT_NOT_NULL(find_text("A"));
    TEST_ASSERT_EQUAL(0, find_text("A")->x);
    TEST_ASSERT_EQUAL(11, find_text("A")->y);
    TEST_ASSERT_EQUAL(160, lot52_node("p")->w);
    TEST_ASSERT_EQUAL(80, lot52_node("p")->h);
    TEST_ASSERT_EQUAL(80, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l60_lines_row_2_8(void) {
    lot52_render("<body style='margin:0'><div id=p style='display:grid;width:160px;grid-template-columns:40px;grid-template-rows"
        ":[L1] 10px [L2] 10px [L3] 10px [L4] 10px [L5] 10px [L6] 10px [L7] 10px [L8] 10px [L9];align-items:stretch'><di"
        "v id=a style='grid-row:L2 / L8;background:red'>A</div></div><div id=b style='height:5px'></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(60, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->grid_col_start);
    TEST_ASSERT_EQUAL(2, lot52_node("a")->grid_row_start);
    TEST_ASSERT_EQUAL(1, lot52_node("a")->grid_col_span);
    TEST_ASSERT_EQUAL(6, lot52_node("a")->grid_row_span);
    TEST_ASSERT_NOT_NULL(find_text("A"));
    TEST_ASSERT_EQUAL(0, find_text("A")->x);
    TEST_ASSERT_EQUAL(11, find_text("A")->y);
    TEST_ASSERT_EQUAL(160, lot52_node("p")->w);
    TEST_ASSERT_EQUAL(80, lot52_node("p")->h);
    TEST_ASSERT_EQUAL(80, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l60_lines_row_2_9(void) {
    lot52_render("<body style='margin:0'><div id=p style='display:grid;width:160px;grid-template-columns:40px;grid-template-rows"
        ":[L1] 10px [L2] 10px [L3] 10px [L4] 10px [L5] 10px [L6] 10px [L7] 10px [L8] 10px [L9];align-items:stretch'><di"
        "v id=a style='grid-row:L2 / L9;background:red'>A</div></div><div id=b style='height:5px'></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(70, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->grid_col_start);
    TEST_ASSERT_EQUAL(2, lot52_node("a")->grid_row_start);
    TEST_ASSERT_EQUAL(1, lot52_node("a")->grid_col_span);
    TEST_ASSERT_EQUAL(7, lot52_node("a")->grid_row_span);
    TEST_ASSERT_NOT_NULL(find_text("A"));
    TEST_ASSERT_EQUAL(0, find_text("A")->x);
    TEST_ASSERT_EQUAL(11, find_text("A")->y);
    TEST_ASSERT_EQUAL(160, lot52_node("p")->w);
    TEST_ASSERT_EQUAL(80, lot52_node("p")->h);
    TEST_ASSERT_EQUAL(80, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l60_lines_row_3_4(void) {
    lot52_render("<body style='margin:0'><div id=p style='display:grid;width:160px;grid-template-columns:40px;grid-template-rows"
        ":[L1] 10px [L2] 10px [L3] 10px [L4] 10px [L5] 10px [L6] 10px [L7] 10px [L8] 10px [L9];align-items:stretch'><di"
        "v id=a style='grid-row:L3 / L4;background:red'>A</div></div><div id=b style='height:5px'></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->grid_col_start);
    TEST_ASSERT_EQUAL(3, lot52_node("a")->grid_row_start);
    TEST_ASSERT_EQUAL(1, lot52_node("a")->grid_col_span);
    TEST_ASSERT_EQUAL(1, lot52_node("a")->grid_row_span);
    TEST_ASSERT_NOT_NULL(find_text("A"));
    TEST_ASSERT_EQUAL(0, find_text("A")->x);
    TEST_ASSERT_EQUAL(21, find_text("A")->y);
    TEST_ASSERT_EQUAL(160, lot52_node("p")->w);
    TEST_ASSERT_EQUAL(80, lot52_node("p")->h);
    TEST_ASSERT_EQUAL(80, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l60_lines_row_3_5(void) {
    lot52_render("<body style='margin:0'><div id=p style='display:grid;width:160px;grid-template-columns:40px;grid-template-rows"
        ":[L1] 10px [L2] 10px [L3] 10px [L4] 10px [L5] 10px [L6] 10px [L7] 10px [L8] 10px [L9];align-items:stretch'><di"
        "v id=a style='grid-row:L3 / L5;background:red'>A</div></div><div id=b style='height:5px'></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->grid_col_start);
    TEST_ASSERT_EQUAL(3, lot52_node("a")->grid_row_start);
    TEST_ASSERT_EQUAL(1, lot52_node("a")->grid_col_span);
    TEST_ASSERT_EQUAL(2, lot52_node("a")->grid_row_span);
    TEST_ASSERT_NOT_NULL(find_text("A"));
    TEST_ASSERT_EQUAL(0, find_text("A")->x);
    TEST_ASSERT_EQUAL(21, find_text("A")->y);
    TEST_ASSERT_EQUAL(160, lot52_node("p")->w);
    TEST_ASSERT_EQUAL(80, lot52_node("p")->h);
    TEST_ASSERT_EQUAL(80, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l60_lines_row_3_6(void) {
    lot52_render("<body style='margin:0'><div id=p style='display:grid;width:160px;grid-template-columns:40px;grid-template-rows"
        ":[L1] 10px [L2] 10px [L3] 10px [L4] 10px [L5] 10px [L6] 10px [L7] 10px [L8] 10px [L9];align-items:stretch'><di"
        "v id=a style='grid-row:L3 / L6;background:red'>A</div></div><div id=b style='height:5px'></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->grid_col_start);
    TEST_ASSERT_EQUAL(3, lot52_node("a")->grid_row_start);
    TEST_ASSERT_EQUAL(1, lot52_node("a")->grid_col_span);
    TEST_ASSERT_EQUAL(3, lot52_node("a")->grid_row_span);
    TEST_ASSERT_NOT_NULL(find_text("A"));
    TEST_ASSERT_EQUAL(0, find_text("A")->x);
    TEST_ASSERT_EQUAL(21, find_text("A")->y);
    TEST_ASSERT_EQUAL(160, lot52_node("p")->w);
    TEST_ASSERT_EQUAL(80, lot52_node("p")->h);
    TEST_ASSERT_EQUAL(80, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l60_lines_row_3_7(void) {
    lot52_render("<body style='margin:0'><div id=p style='display:grid;width:160px;grid-template-columns:40px;grid-template-rows"
        ":[L1] 10px [L2] 10px [L3] 10px [L4] 10px [L5] 10px [L6] 10px [L7] 10px [L8] 10px [L9];align-items:stretch'><di"
        "v id=a style='grid-row:L3 / L7;background:red'>A</div></div><div id=b style='height:5px'></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(40, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->grid_col_start);
    TEST_ASSERT_EQUAL(3, lot52_node("a")->grid_row_start);
    TEST_ASSERT_EQUAL(1, lot52_node("a")->grid_col_span);
    TEST_ASSERT_EQUAL(4, lot52_node("a")->grid_row_span);
    TEST_ASSERT_NOT_NULL(find_text("A"));
    TEST_ASSERT_EQUAL(0, find_text("A")->x);
    TEST_ASSERT_EQUAL(21, find_text("A")->y);
    TEST_ASSERT_EQUAL(160, lot52_node("p")->w);
    TEST_ASSERT_EQUAL(80, lot52_node("p")->h);
    TEST_ASSERT_EQUAL(80, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l60_lines_row_3_8(void) {
    lot52_render("<body style='margin:0'><div id=p style='display:grid;width:160px;grid-template-columns:40px;grid-template-rows"
        ":[L1] 10px [L2] 10px [L3] 10px [L4] 10px [L5] 10px [L6] 10px [L7] 10px [L8] 10px [L9];align-items:stretch'><di"
        "v id=a style='grid-row:L3 / L8;background:red'>A</div></div><div id=b style='height:5px'></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(50, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->grid_col_start);
    TEST_ASSERT_EQUAL(3, lot52_node("a")->grid_row_start);
    TEST_ASSERT_EQUAL(1, lot52_node("a")->grid_col_span);
    TEST_ASSERT_EQUAL(5, lot52_node("a")->grid_row_span);
    TEST_ASSERT_NOT_NULL(find_text("A"));
    TEST_ASSERT_EQUAL(0, find_text("A")->x);
    TEST_ASSERT_EQUAL(21, find_text("A")->y);
    TEST_ASSERT_EQUAL(160, lot52_node("p")->w);
    TEST_ASSERT_EQUAL(80, lot52_node("p")->h);
    TEST_ASSERT_EQUAL(80, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l60_lines_row_3_9(void) {
    lot52_render("<body style='margin:0'><div id=p style='display:grid;width:160px;grid-template-columns:40px;grid-template-rows"
        ":[L1] 10px [L2] 10px [L3] 10px [L4] 10px [L5] 10px [L6] 10px [L7] 10px [L8] 10px [L9];align-items:stretch'><di"
        "v id=a style='grid-row:L3 / L9;background:red'>A</div></div><div id=b style='height:5px'></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(60, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->grid_col_start);
    TEST_ASSERT_EQUAL(3, lot52_node("a")->grid_row_start);
    TEST_ASSERT_EQUAL(1, lot52_node("a")->grid_col_span);
    TEST_ASSERT_EQUAL(6, lot52_node("a")->grid_row_span);
    TEST_ASSERT_NOT_NULL(find_text("A"));
    TEST_ASSERT_EQUAL(0, find_text("A")->x);
    TEST_ASSERT_EQUAL(21, find_text("A")->y);
    TEST_ASSERT_EQUAL(160, lot52_node("p")->w);
    TEST_ASSERT_EQUAL(80, lot52_node("p")->h);
    TEST_ASSERT_EQUAL(80, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l60_lines_row_4_5(void) {
    lot52_render("<body style='margin:0'><div id=p style='display:grid;width:160px;grid-template-columns:40px;grid-template-rows"
        ":[L1] 10px [L2] 10px [L3] 10px [L4] 10px [L5] 10px [L6] 10px [L7] 10px [L8] 10px [L9];align-items:stretch'><di"
        "v id=a style='grid-row:L4 / L5;background:red'>A</div></div><div id=b style='height:5px'></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->grid_col_start);
    TEST_ASSERT_EQUAL(4, lot52_node("a")->grid_row_start);
    TEST_ASSERT_EQUAL(1, lot52_node("a")->grid_col_span);
    TEST_ASSERT_EQUAL(1, lot52_node("a")->grid_row_span);
    TEST_ASSERT_NOT_NULL(find_text("A"));
    TEST_ASSERT_EQUAL(0, find_text("A")->x);
    TEST_ASSERT_EQUAL(31, find_text("A")->y);
    TEST_ASSERT_EQUAL(160, lot52_node("p")->w);
    TEST_ASSERT_EQUAL(80, lot52_node("p")->h);
    TEST_ASSERT_EQUAL(80, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l60_lines_row_4_6(void) {
    lot52_render("<body style='margin:0'><div id=p style='display:grid;width:160px;grid-template-columns:40px;grid-template-rows"
        ":[L1] 10px [L2] 10px [L3] 10px [L4] 10px [L5] 10px [L6] 10px [L7] 10px [L8] 10px [L9];align-items:stretch'><di"
        "v id=a style='grid-row:L4 / L6;background:red'>A</div></div><div id=b style='height:5px'></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->grid_col_start);
    TEST_ASSERT_EQUAL(4, lot52_node("a")->grid_row_start);
    TEST_ASSERT_EQUAL(1, lot52_node("a")->grid_col_span);
    TEST_ASSERT_EQUAL(2, lot52_node("a")->grid_row_span);
    TEST_ASSERT_NOT_NULL(find_text("A"));
    TEST_ASSERT_EQUAL(0, find_text("A")->x);
    TEST_ASSERT_EQUAL(31, find_text("A")->y);
    TEST_ASSERT_EQUAL(160, lot52_node("p")->w);
    TEST_ASSERT_EQUAL(80, lot52_node("p")->h);
    TEST_ASSERT_EQUAL(80, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l60_lines_row_4_7(void) {
    lot52_render("<body style='margin:0'><div id=p style='display:grid;width:160px;grid-template-columns:40px;grid-template-rows"
        ":[L1] 10px [L2] 10px [L3] 10px [L4] 10px [L5] 10px [L6] 10px [L7] 10px [L8] 10px [L9];align-items:stretch'><di"
        "v id=a style='grid-row:L4 / L7;background:red'>A</div></div><div id=b style='height:5px'></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->grid_col_start);
    TEST_ASSERT_EQUAL(4, lot52_node("a")->grid_row_start);
    TEST_ASSERT_EQUAL(1, lot52_node("a")->grid_col_span);
    TEST_ASSERT_EQUAL(3, lot52_node("a")->grid_row_span);
    TEST_ASSERT_NOT_NULL(find_text("A"));
    TEST_ASSERT_EQUAL(0, find_text("A")->x);
    TEST_ASSERT_EQUAL(31, find_text("A")->y);
    TEST_ASSERT_EQUAL(160, lot52_node("p")->w);
    TEST_ASSERT_EQUAL(80, lot52_node("p")->h);
    TEST_ASSERT_EQUAL(80, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l60_lines_row_4_8(void) {
    lot52_render("<body style='margin:0'><div id=p style='display:grid;width:160px;grid-template-columns:40px;grid-template-rows"
        ":[L1] 10px [L2] 10px [L3] 10px [L4] 10px [L5] 10px [L6] 10px [L7] 10px [L8] 10px [L9];align-items:stretch'><di"
        "v id=a style='grid-row:L4 / L8;background:red'>A</div></div><div id=b style='height:5px'></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(40, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->grid_col_start);
    TEST_ASSERT_EQUAL(4, lot52_node("a")->grid_row_start);
    TEST_ASSERT_EQUAL(1, lot52_node("a")->grid_col_span);
    TEST_ASSERT_EQUAL(4, lot52_node("a")->grid_row_span);
    TEST_ASSERT_NOT_NULL(find_text("A"));
    TEST_ASSERT_EQUAL(0, find_text("A")->x);
    TEST_ASSERT_EQUAL(31, find_text("A")->y);
    TEST_ASSERT_EQUAL(160, lot52_node("p")->w);
    TEST_ASSERT_EQUAL(80, lot52_node("p")->h);
    TEST_ASSERT_EQUAL(80, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l60_lines_row_4_9(void) {
    lot52_render("<body style='margin:0'><div id=p style='display:grid;width:160px;grid-template-columns:40px;grid-template-rows"
        ":[L1] 10px [L2] 10px [L3] 10px [L4] 10px [L5] 10px [L6] 10px [L7] 10px [L8] 10px [L9];align-items:stretch'><di"
        "v id=a style='grid-row:L4 / L9;background:red'>A</div></div><div id=b style='height:5px'></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(50, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->grid_col_start);
    TEST_ASSERT_EQUAL(4, lot52_node("a")->grid_row_start);
    TEST_ASSERT_EQUAL(1, lot52_node("a")->grid_col_span);
    TEST_ASSERT_EQUAL(5, lot52_node("a")->grid_row_span);
    TEST_ASSERT_NOT_NULL(find_text("A"));
    TEST_ASSERT_EQUAL(0, find_text("A")->x);
    TEST_ASSERT_EQUAL(31, find_text("A")->y);
    TEST_ASSERT_EQUAL(160, lot52_node("p")->w);
    TEST_ASSERT_EQUAL(80, lot52_node("p")->h);
    TEST_ASSERT_EQUAL(80, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l60_lines_row_5_6(void) {
    lot52_render("<body style='margin:0'><div id=p style='display:grid;width:160px;grid-template-columns:40px;grid-template-rows"
        ":[L1] 10px [L2] 10px [L3] 10px [L4] 10px [L5] 10px [L6] 10px [L7] 10px [L8] 10px [L9];align-items:stretch'><di"
        "v id=a style='grid-row:L5 / L6;background:red'>A</div></div><div id=b style='height:5px'></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(40, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->grid_col_start);
    TEST_ASSERT_EQUAL(5, lot52_node("a")->grid_row_start);
    TEST_ASSERT_EQUAL(1, lot52_node("a")->grid_col_span);
    TEST_ASSERT_EQUAL(1, lot52_node("a")->grid_row_span);
    TEST_ASSERT_NOT_NULL(find_text("A"));
    TEST_ASSERT_EQUAL(0, find_text("A")->x);
    TEST_ASSERT_EQUAL(41, find_text("A")->y);
    TEST_ASSERT_EQUAL(160, lot52_node("p")->w);
    TEST_ASSERT_EQUAL(80, lot52_node("p")->h);
    TEST_ASSERT_EQUAL(80, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l60_lines_row_5_7(void) {
    lot52_render("<body style='margin:0'><div id=p style='display:grid;width:160px;grid-template-columns:40px;grid-template-rows"
        ":[L1] 10px [L2] 10px [L3] 10px [L4] 10px [L5] 10px [L6] 10px [L7] 10px [L8] 10px [L9];align-items:stretch'><di"
        "v id=a style='grid-row:L5 / L7;background:red'>A</div></div><div id=b style='height:5px'></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(40, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->grid_col_start);
    TEST_ASSERT_EQUAL(5, lot52_node("a")->grid_row_start);
    TEST_ASSERT_EQUAL(1, lot52_node("a")->grid_col_span);
    TEST_ASSERT_EQUAL(2, lot52_node("a")->grid_row_span);
    TEST_ASSERT_NOT_NULL(find_text("A"));
    TEST_ASSERT_EQUAL(0, find_text("A")->x);
    TEST_ASSERT_EQUAL(41, find_text("A")->y);
    TEST_ASSERT_EQUAL(160, lot52_node("p")->w);
    TEST_ASSERT_EQUAL(80, lot52_node("p")->h);
    TEST_ASSERT_EQUAL(80, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l60_lines_row_5_8(void) {
    lot52_render("<body style='margin:0'><div id=p style='display:grid;width:160px;grid-template-columns:40px;grid-template-rows"
        ":[L1] 10px [L2] 10px [L3] 10px [L4] 10px [L5] 10px [L6] 10px [L7] 10px [L8] 10px [L9];align-items:stretch'><di"
        "v id=a style='grid-row:L5 / L8;background:red'>A</div></div><div id=b style='height:5px'></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(40, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->grid_col_start);
    TEST_ASSERT_EQUAL(5, lot52_node("a")->grid_row_start);
    TEST_ASSERT_EQUAL(1, lot52_node("a")->grid_col_span);
    TEST_ASSERT_EQUAL(3, lot52_node("a")->grid_row_span);
    TEST_ASSERT_NOT_NULL(find_text("A"));
    TEST_ASSERT_EQUAL(0, find_text("A")->x);
    TEST_ASSERT_EQUAL(41, find_text("A")->y);
    TEST_ASSERT_EQUAL(160, lot52_node("p")->w);
    TEST_ASSERT_EQUAL(80, lot52_node("p")->h);
    TEST_ASSERT_EQUAL(80, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l60_lines_row_5_9(void) {
    lot52_render("<body style='margin:0'><div id=p style='display:grid;width:160px;grid-template-columns:40px;grid-template-rows"
        ":[L1] 10px [L2] 10px [L3] 10px [L4] 10px [L5] 10px [L6] 10px [L7] 10px [L8] 10px [L9];align-items:stretch'><di"
        "v id=a style='grid-row:L5 / L9;background:red'>A</div></div><div id=b style='height:5px'></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(40, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(40, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->grid_col_start);
    TEST_ASSERT_EQUAL(5, lot52_node("a")->grid_row_start);
    TEST_ASSERT_EQUAL(1, lot52_node("a")->grid_col_span);
    TEST_ASSERT_EQUAL(4, lot52_node("a")->grid_row_span);
    TEST_ASSERT_NOT_NULL(find_text("A"));
    TEST_ASSERT_EQUAL(0, find_text("A")->x);
    TEST_ASSERT_EQUAL(41, find_text("A")->y);
    TEST_ASSERT_EQUAL(160, lot52_node("p")->w);
    TEST_ASSERT_EQUAL(80, lot52_node("p")->h);
    TEST_ASSERT_EQUAL(80, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l60_lines_row_6_7(void) {
    lot52_render("<body style='margin:0'><div id=p style='display:grid;width:160px;grid-template-columns:40px;grid-template-rows"
        ":[L1] 10px [L2] 10px [L3] 10px [L4] 10px [L5] 10px [L6] 10px [L7] 10px [L8] 10px [L9];align-items:stretch'><di"
        "v id=a style='grid-row:L6 / L7;background:red'>A</div></div><div id=b style='height:5px'></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(50, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->grid_col_start);
    TEST_ASSERT_EQUAL(6, lot52_node("a")->grid_row_start);
    TEST_ASSERT_EQUAL(1, lot52_node("a")->grid_col_span);
    TEST_ASSERT_EQUAL(1, lot52_node("a")->grid_row_span);
    TEST_ASSERT_NOT_NULL(find_text("A"));
    TEST_ASSERT_EQUAL(0, find_text("A")->x);
    TEST_ASSERT_EQUAL(51, find_text("A")->y);
    TEST_ASSERT_EQUAL(160, lot52_node("p")->w);
    TEST_ASSERT_EQUAL(80, lot52_node("p")->h);
    TEST_ASSERT_EQUAL(80, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l60_lines_row_6_8(void) {
    lot52_render("<body style='margin:0'><div id=p style='display:grid;width:160px;grid-template-columns:40px;grid-template-rows"
        ":[L1] 10px [L2] 10px [L3] 10px [L4] 10px [L5] 10px [L6] 10px [L7] 10px [L8] 10px [L9];align-items:stretch'><di"
        "v id=a style='grid-row:L6 / L8;background:red'>A</div></div><div id=b style='height:5px'></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(50, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->grid_col_start);
    TEST_ASSERT_EQUAL(6, lot52_node("a")->grid_row_start);
    TEST_ASSERT_EQUAL(1, lot52_node("a")->grid_col_span);
    TEST_ASSERT_EQUAL(2, lot52_node("a")->grid_row_span);
    TEST_ASSERT_NOT_NULL(find_text("A"));
    TEST_ASSERT_EQUAL(0, find_text("A")->x);
    TEST_ASSERT_EQUAL(51, find_text("A")->y);
    TEST_ASSERT_EQUAL(160, lot52_node("p")->w);
    TEST_ASSERT_EQUAL(80, lot52_node("p")->h);
    TEST_ASSERT_EQUAL(80, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l60_lines_row_6_9(void) {
    lot52_render("<body style='margin:0'><div id=p style='display:grid;width:160px;grid-template-columns:40px;grid-template-rows"
        ":[L1] 10px [L2] 10px [L3] 10px [L4] 10px [L5] 10px [L6] 10px [L7] 10px [L8] 10px [L9];align-items:stretch'><di"
        "v id=a style='grid-row:L6 / L9;background:red'>A</div></div><div id=b style='height:5px'></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(50, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->grid_col_start);
    TEST_ASSERT_EQUAL(6, lot52_node("a")->grid_row_start);
    TEST_ASSERT_EQUAL(1, lot52_node("a")->grid_col_span);
    TEST_ASSERT_EQUAL(3, lot52_node("a")->grid_row_span);
    TEST_ASSERT_NOT_NULL(find_text("A"));
    TEST_ASSERT_EQUAL(0, find_text("A")->x);
    TEST_ASSERT_EQUAL(51, find_text("A")->y);
    TEST_ASSERT_EQUAL(160, lot52_node("p")->w);
    TEST_ASSERT_EQUAL(80, lot52_node("p")->h);
    TEST_ASSERT_EQUAL(80, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l60_lines_row_7_8(void) {
    lot52_render("<body style='margin:0'><div id=p style='display:grid;width:160px;grid-template-columns:40px;grid-template-rows"
        ":[L1] 10px [L2] 10px [L3] 10px [L4] 10px [L5] 10px [L6] 10px [L7] 10px [L8] 10px [L9];align-items:stretch'><di"
        "v id=a style='grid-row:L7 / L8;background:red'>A</div></div><div id=b style='height:5px'></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(60, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->grid_col_start);
    TEST_ASSERT_EQUAL(7, lot52_node("a")->grid_row_start);
    TEST_ASSERT_EQUAL(1, lot52_node("a")->grid_col_span);
    TEST_ASSERT_EQUAL(1, lot52_node("a")->grid_row_span);
    TEST_ASSERT_NOT_NULL(find_text("A"));
    TEST_ASSERT_EQUAL(0, find_text("A")->x);
    TEST_ASSERT_EQUAL(61, find_text("A")->y);
    TEST_ASSERT_EQUAL(160, lot52_node("p")->w);
    TEST_ASSERT_EQUAL(80, lot52_node("p")->h);
    TEST_ASSERT_EQUAL(80, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l60_lines_row_7_9(void) {
    lot52_render("<body style='margin:0'><div id=p style='display:grid;width:160px;grid-template-columns:40px;grid-template-rows"
        ":[L1] 10px [L2] 10px [L3] 10px [L4] 10px [L5] 10px [L6] 10px [L7] 10px [L8] 10px [L9];align-items:stretch'><di"
        "v id=a style='grid-row:L7 / L9;background:red'>A</div></div><div id=b style='height:5px'></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(60, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->grid_col_start);
    TEST_ASSERT_EQUAL(7, lot52_node("a")->grid_row_start);
    TEST_ASSERT_EQUAL(1, lot52_node("a")->grid_col_span);
    TEST_ASSERT_EQUAL(2, lot52_node("a")->grid_row_span);
    TEST_ASSERT_NOT_NULL(find_text("A"));
    TEST_ASSERT_EQUAL(0, find_text("A")->x);
    TEST_ASSERT_EQUAL(61, find_text("A")->y);
    TEST_ASSERT_EQUAL(160, lot52_node("p")->w);
    TEST_ASSERT_EQUAL(80, lot52_node("p")->h);
    TEST_ASSERT_EQUAL(80, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l60_lines_row_8_9(void) {
    lot52_render("<body style='margin:0'><div id=p style='display:grid;width:160px;grid-template-columns:40px;grid-template-rows"
        ":[L1] 10px [L2] 10px [L3] 10px [L4] 10px [L5] 10px [L6] 10px [L7] 10px [L8] 10px [L9];align-items:stretch'><di"
        "v id=a style='grid-row:L8 / L9;background:red'>A</div></div><div id=b style='height:5px'></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(70, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->grid_col_start);
    TEST_ASSERT_EQUAL(8, lot52_node("a")->grid_row_start);
    TEST_ASSERT_EQUAL(1, lot52_node("a")->grid_col_span);
    TEST_ASSERT_EQUAL(1, lot52_node("a")->grid_row_span);
    TEST_ASSERT_NOT_NULL(find_text("A"));
    TEST_ASSERT_EQUAL(0, find_text("A")->x);
    TEST_ASSERT_EQUAL(71, find_text("A")->y);
    TEST_ASSERT_EQUAL(160, lot52_node("p")->w);
    TEST_ASSERT_EQUAL(80, lot52_node("p")->h);
    TEST_ASSERT_EQUAL(80, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}
