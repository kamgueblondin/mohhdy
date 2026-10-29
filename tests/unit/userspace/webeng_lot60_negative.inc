static void test_l60_negative_column_2_last(void) {
    lot52_render("<body style='margin:0'><div id=p style='display:grid;width:160px;grid-template-columns:repeat(2,20px);grid-tem"
        "plate-rows:10px;align-items:stretch'><div id=a style='grid-column:-2 / -1;background:blue'>A</div></div><div i"
        "d=b style='height:5px'></div></body>", 320);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(1, lot52_node("a")->grid_col_span);
    TEST_ASSERT_EQUAL(1, lot52_node("a")->grid_row_span);
    TEST_ASSERT_NOT_NULL(find_text("A"));
    TEST_ASSERT_EQUAL(20, find_text("A")->x);
    TEST_ASSERT_EQUAL(1, find_text("A")->y);
    TEST_ASSERT_EQUAL(10, lot52_node("p")->h);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l60_negative_column_2_all(void) {
    lot52_render("<body style='margin:0'><div id=p style='display:grid;width:160px;grid-template-columns:repeat(2,20px);grid-tem"
        "plate-rows:10px;align-items:stretch'><div id=a style='grid-column:1 / -1;background:blue'>A</div></div><div id"
        "=b style='height:5px'></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(2, lot52_node("a")->grid_col_span);
    TEST_ASSERT_EQUAL(1, lot52_node("a")->grid_row_span);
    TEST_ASSERT_NOT_NULL(find_text("A"));
    TEST_ASSERT_EQUAL(0, find_text("A")->x);
    TEST_ASSERT_EQUAL(1, find_text("A")->y);
    TEST_ASSERT_EQUAL(10, lot52_node("p")->h);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l60_negative_column_3_last(void) {
    lot52_render("<body style='margin:0'><div id=p style='display:grid;width:160px;grid-template-columns:repeat(3,20px);grid-tem"
        "plate-rows:10px;align-items:stretch'><div id=a style='grid-column:-2 / -1;background:blue'>A</div></div><div i"
        "d=b style='height:5px'></div></body>", 320);
    TEST_ASSERT_EQUAL(40, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(1, lot52_node("a")->grid_col_span);
    TEST_ASSERT_EQUAL(1, lot52_node("a")->grid_row_span);
    TEST_ASSERT_NOT_NULL(find_text("A"));
    TEST_ASSERT_EQUAL(40, find_text("A")->x);
    TEST_ASSERT_EQUAL(1, find_text("A")->y);
    TEST_ASSERT_EQUAL(10, lot52_node("p")->h);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l60_negative_column_3_all(void) {
    lot52_render("<body style='margin:0'><div id=p style='display:grid;width:160px;grid-template-columns:repeat(3,20px);grid-tem"
        "plate-rows:10px;align-items:stretch'><div id=a style='grid-column:1 / -1;background:blue'>A</div></div><div id"
        "=b style='height:5px'></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(60, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(3, lot52_node("a")->grid_col_span);
    TEST_ASSERT_EQUAL(1, lot52_node("a")->grid_row_span);
    TEST_ASSERT_NOT_NULL(find_text("A"));
    TEST_ASSERT_EQUAL(0, find_text("A")->x);
    TEST_ASSERT_EQUAL(1, find_text("A")->y);
    TEST_ASSERT_EQUAL(10, lot52_node("p")->h);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l60_negative_column_4_last(void) {
    lot52_render("<body style='margin:0'><div id=p style='display:grid;width:160px;grid-template-columns:repeat(4,20px);grid-tem"
        "plate-rows:10px;align-items:stretch'><div id=a style='grid-column:-2 / -1;background:blue'>A</div></div><div i"
        "d=b style='height:5px'></div></body>", 320);
    TEST_ASSERT_EQUAL(60, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(1, lot52_node("a")->grid_col_span);
    TEST_ASSERT_EQUAL(1, lot52_node("a")->grid_row_span);
    TEST_ASSERT_NOT_NULL(find_text("A"));
    TEST_ASSERT_EQUAL(60, find_text("A")->x);
    TEST_ASSERT_EQUAL(1, find_text("A")->y);
    TEST_ASSERT_EQUAL(10, lot52_node("p")->h);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l60_negative_column_4_all(void) {
    lot52_render("<body style='margin:0'><div id=p style='display:grid;width:160px;grid-template-columns:repeat(4,20px);grid-tem"
        "plate-rows:10px;align-items:stretch'><div id=a style='grid-column:1 / -1;background:blue'>A</div></div><div id"
        "=b style='height:5px'></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(80, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(4, lot52_node("a")->grid_col_span);
    TEST_ASSERT_EQUAL(1, lot52_node("a")->grid_row_span);
    TEST_ASSERT_NOT_NULL(find_text("A"));
    TEST_ASSERT_EQUAL(0, find_text("A")->x);
    TEST_ASSERT_EQUAL(1, find_text("A")->y);
    TEST_ASSERT_EQUAL(10, lot52_node("p")->h);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l60_negative_column_5_last(void) {
    lot52_render("<body style='margin:0'><div id=p style='display:grid;width:160px;grid-template-columns:repeat(5,20px);grid-tem"
        "plate-rows:10px;align-items:stretch'><div id=a style='grid-column:-2 / -1;background:blue'>A</div></div><div i"
        "d=b style='height:5px'></div></body>", 320);
    TEST_ASSERT_EQUAL(80, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(1, lot52_node("a")->grid_col_span);
    TEST_ASSERT_EQUAL(1, lot52_node("a")->grid_row_span);
    TEST_ASSERT_NOT_NULL(find_text("A"));
    TEST_ASSERT_EQUAL(80, find_text("A")->x);
    TEST_ASSERT_EQUAL(1, find_text("A")->y);
    TEST_ASSERT_EQUAL(10, lot52_node("p")->h);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l60_negative_column_5_all(void) {
    lot52_render("<body style='margin:0'><div id=p style='display:grid;width:160px;grid-template-columns:repeat(5,20px);grid-tem"
        "plate-rows:10px;align-items:stretch'><div id=a style='grid-column:1 / -1;background:blue'>A</div></div><div id"
        "=b style='height:5px'></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(100, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(5, lot52_node("a")->grid_col_span);
    TEST_ASSERT_EQUAL(1, lot52_node("a")->grid_row_span);
    TEST_ASSERT_NOT_NULL(find_text("A"));
    TEST_ASSERT_EQUAL(0, find_text("A")->x);
    TEST_ASSERT_EQUAL(1, find_text("A")->y);
    TEST_ASSERT_EQUAL(10, lot52_node("p")->h);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l60_negative_column_6_last(void) {
    lot52_render("<body style='margin:0'><div id=p style='display:grid;width:160px;grid-template-columns:repeat(6,20px);grid-tem"
        "plate-rows:10px;align-items:stretch'><div id=a style='grid-column:-2 / -1;background:blue'>A</div></div><div i"
        "d=b style='height:5px'></div></body>", 320);
    TEST_ASSERT_EQUAL(100, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(1, lot52_node("a")->grid_col_span);
    TEST_ASSERT_EQUAL(1, lot52_node("a")->grid_row_span);
    TEST_ASSERT_NOT_NULL(find_text("A"));
    TEST_ASSERT_EQUAL(100, find_text("A")->x);
    TEST_ASSERT_EQUAL(1, find_text("A")->y);
    TEST_ASSERT_EQUAL(10, lot52_node("p")->h);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l60_negative_column_6_all(void) {
    lot52_render("<body style='margin:0'><div id=p style='display:grid;width:160px;grid-template-columns:repeat(6,20px);grid-tem"
        "plate-rows:10px;align-items:stretch'><div id=a style='grid-column:1 / -1;background:blue'>A</div></div><div id"
        "=b style='height:5px'></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(120, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(6, lot52_node("a")->grid_col_span);
    TEST_ASSERT_EQUAL(1, lot52_node("a")->grid_row_span);
    TEST_ASSERT_NOT_NULL(find_text("A"));
    TEST_ASSERT_EQUAL(0, find_text("A")->x);
    TEST_ASSERT_EQUAL(1, find_text("A")->y);
    TEST_ASSERT_EQUAL(10, lot52_node("p")->h);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l60_negative_column_7_last(void) {
    lot52_render("<body style='margin:0'><div id=p style='display:grid;width:160px;grid-template-columns:repeat(7,20px);grid-tem"
        "plate-rows:10px;align-items:stretch'><div id=a style='grid-column:-2 / -1;background:blue'>A</div></div><div i"
        "d=b style='height:5px'></div></body>", 320);
    TEST_ASSERT_EQUAL(120, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(1, lot52_node("a")->grid_col_span);
    TEST_ASSERT_EQUAL(1, lot52_node("a")->grid_row_span);
    TEST_ASSERT_NOT_NULL(find_text("A"));
    TEST_ASSERT_EQUAL(120, find_text("A")->x);
    TEST_ASSERT_EQUAL(1, find_text("A")->y);
    TEST_ASSERT_EQUAL(10, lot52_node("p")->h);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l60_negative_column_7_all(void) {
    lot52_render("<body style='margin:0'><div id=p style='display:grid;width:160px;grid-template-columns:repeat(7,20px);grid-tem"
        "plate-rows:10px;align-items:stretch'><div id=a style='grid-column:1 / -1;background:blue'>A</div></div><div id"
        "=b style='height:5px'></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(140, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(7, lot52_node("a")->grid_col_span);
    TEST_ASSERT_EQUAL(1, lot52_node("a")->grid_row_span);
    TEST_ASSERT_NOT_NULL(find_text("A"));
    TEST_ASSERT_EQUAL(0, find_text("A")->x);
    TEST_ASSERT_EQUAL(1, find_text("A")->y);
    TEST_ASSERT_EQUAL(10, lot52_node("p")->h);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l60_negative_column_8_last(void) {
    lot52_render("<body style='margin:0'><div id=p style='display:grid;width:160px;grid-template-columns:repeat(8,20px);grid-tem"
        "plate-rows:10px;align-items:stretch'><div id=a style='grid-column:-2 / -1;background:blue'>A</div></div><div i"
        "d=b style='height:5px'></div></body>", 320);
    TEST_ASSERT_EQUAL(140, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(1, lot52_node("a")->grid_col_span);
    TEST_ASSERT_EQUAL(1, lot52_node("a")->grid_row_span);
    TEST_ASSERT_NOT_NULL(find_text("A"));
    TEST_ASSERT_EQUAL(140, find_text("A")->x);
    TEST_ASSERT_EQUAL(1, find_text("A")->y);
    TEST_ASSERT_EQUAL(10, lot52_node("p")->h);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l60_negative_column_8_all(void) {
    lot52_render("<body style='margin:0'><div id=p style='display:grid;width:160px;grid-template-columns:repeat(8,20px);grid-tem"
        "plate-rows:10px;align-items:stretch'><div id=a style='grid-column:1 / -1;background:blue'>A</div></div><div id"
        "=b style='height:5px'></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(160, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(8, lot52_node("a")->grid_col_span);
    TEST_ASSERT_EQUAL(1, lot52_node("a")->grid_row_span);
    TEST_ASSERT_NOT_NULL(find_text("A"));
    TEST_ASSERT_EQUAL(0, find_text("A")->x);
    TEST_ASSERT_EQUAL(1, find_text("A")->y);
    TEST_ASSERT_EQUAL(10, lot52_node("p")->h);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l60_negative_row_2_last(void) {
    lot52_render("<body style='margin:0'><div id=p style='display:grid;width:160px;grid-template-columns:40px;grid-template-rows"
        ":repeat(2,10px);align-items:stretch'><div id=a style='grid-row:-2 / -1;background:blue'>A</div></div><div id=b"
        " style='height:5px'></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(1, lot52_node("a")->grid_col_span);
    TEST_ASSERT_EQUAL(1, lot52_node("a")->grid_row_span);
    TEST_ASSERT_NOT_NULL(find_text("A"));
    TEST_ASSERT_EQUAL(0, find_text("A")->x);
    TEST_ASSERT_EQUAL(11, find_text("A")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("p")->h);
    TEST_ASSERT_EQUAL(20, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l60_negative_row_2_all(void) {
    lot52_render("<body style='margin:0'><div id=p style='display:grid;width:160px;grid-template-columns:40px;grid-template-rows"
        ":repeat(2,10px);align-items:stretch'><div id=a style='grid-row:1 / -1;background:blue'>A</div></div><div id=b "
        "style='height:5px'></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(1, lot52_node("a")->grid_col_span);
    TEST_ASSERT_EQUAL(2, lot52_node("a")->grid_row_span);
    TEST_ASSERT_NOT_NULL(find_text("A"));
    TEST_ASSERT_EQUAL(0, find_text("A")->x);
    TEST_ASSERT_EQUAL(1, find_text("A")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("p")->h);
    TEST_ASSERT_EQUAL(20, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l60_negative_row_3_last(void) {
    lot52_render("<body style='margin:0'><div id=p style='display:grid;width:160px;grid-template-columns:40px;grid-template-rows"
        ":repeat(3,10px);align-items:stretch'><div id=a style='grid-row:-2 / -1;background:blue'>A</div></div><div id=b"
        " style='height:5px'></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(1, lot52_node("a")->grid_col_span);
    TEST_ASSERT_EQUAL(1, lot52_node("a")->grid_row_span);
    TEST_ASSERT_NOT_NULL(find_text("A"));
    TEST_ASSERT_EQUAL(0, find_text("A")->x);
    TEST_ASSERT_EQUAL(21, find_text("A")->y);
    TEST_ASSERT_EQUAL(30, lot52_node("p")->h);
    TEST_ASSERT_EQUAL(30, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l60_negative_row_3_all(void) {
    lot52_render("<body style='margin:0'><div id=p style='display:grid;width:160px;grid-template-columns:40px;grid-template-rows"
        ":repeat(3,10px);align-items:stretch'><div id=a style='grid-row:1 / -1;background:blue'>A</div></div><div id=b "
        "style='height:5px'></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(1, lot52_node("a")->grid_col_span);
    TEST_ASSERT_EQUAL(3, lot52_node("a")->grid_row_span);
    TEST_ASSERT_NOT_NULL(find_text("A"));
    TEST_ASSERT_EQUAL(0, find_text("A")->x);
    TEST_ASSERT_EQUAL(1, find_text("A")->y);
    TEST_ASSERT_EQUAL(30, lot52_node("p")->h);
    TEST_ASSERT_EQUAL(30, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l60_negative_row_4_last(void) {
    lot52_render("<body style='margin:0'><div id=p style='display:grid;width:160px;grid-template-columns:40px;grid-template-rows"
        ":repeat(4,10px);align-items:stretch'><div id=a style='grid-row:-2 / -1;background:blue'>A</div></div><div id=b"
        " style='height:5px'></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(1, lot52_node("a")->grid_col_span);
    TEST_ASSERT_EQUAL(1, lot52_node("a")->grid_row_span);
    TEST_ASSERT_NOT_NULL(find_text("A"));
    TEST_ASSERT_EQUAL(0, find_text("A")->x);
    TEST_ASSERT_EQUAL(31, find_text("A")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("p")->h);
    TEST_ASSERT_EQUAL(40, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l60_negative_row_4_all(void) {
    lot52_render("<body style='margin:0'><div id=p style='display:grid;width:160px;grid-template-columns:40px;grid-template-rows"
        ":repeat(4,10px);align-items:stretch'><div id=a style='grid-row:1 / -1;background:blue'>A</div></div><div id=b "
        "style='height:5px'></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(40, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(1, lot52_node("a")->grid_col_span);
    TEST_ASSERT_EQUAL(4, lot52_node("a")->grid_row_span);
    TEST_ASSERT_NOT_NULL(find_text("A"));
    TEST_ASSERT_EQUAL(0, find_text("A")->x);
    TEST_ASSERT_EQUAL(1, find_text("A")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("p")->h);
    TEST_ASSERT_EQUAL(40, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l60_negative_row_5_last(void) {
    lot52_render("<body style='margin:0'><div id=p style='display:grid;width:160px;grid-template-columns:40px;grid-template-rows"
        ":repeat(5,10px);align-items:stretch'><div id=a style='grid-row:-2 / -1;background:blue'>A</div></div><div id=b"
        " style='height:5px'></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(40, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(1, lot52_node("a")->grid_col_span);
    TEST_ASSERT_EQUAL(1, lot52_node("a")->grid_row_span);
    TEST_ASSERT_NOT_NULL(find_text("A"));
    TEST_ASSERT_EQUAL(0, find_text("A")->x);
    TEST_ASSERT_EQUAL(41, find_text("A")->y);
    TEST_ASSERT_EQUAL(50, lot52_node("p")->h);
    TEST_ASSERT_EQUAL(50, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l60_negative_row_5_all(void) {
    lot52_render("<body style='margin:0'><div id=p style='display:grid;width:160px;grid-template-columns:40px;grid-template-rows"
        ":repeat(5,10px);align-items:stretch'><div id=a style='grid-row:1 / -1;background:blue'>A</div></div><div id=b "
        "style='height:5px'></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(50, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(1, lot52_node("a")->grid_col_span);
    TEST_ASSERT_EQUAL(5, lot52_node("a")->grid_row_span);
    TEST_ASSERT_NOT_NULL(find_text("A"));
    TEST_ASSERT_EQUAL(0, find_text("A")->x);
    TEST_ASSERT_EQUAL(1, find_text("A")->y);
    TEST_ASSERT_EQUAL(50, lot52_node("p")->h);
    TEST_ASSERT_EQUAL(50, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l60_negative_row_6_last(void) {
    lot52_render("<body style='margin:0'><div id=p style='display:grid;width:160px;grid-template-columns:40px;grid-template-rows"
        ":repeat(6,10px);align-items:stretch'><div id=a style='grid-row:-2 / -1;background:blue'>A</div></div><div id=b"
        " style='height:5px'></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(50, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(1, lot52_node("a")->grid_col_span);
    TEST_ASSERT_EQUAL(1, lot52_node("a")->grid_row_span);
    TEST_ASSERT_NOT_NULL(find_text("A"));
    TEST_ASSERT_EQUAL(0, find_text("A")->x);
    TEST_ASSERT_EQUAL(51, find_text("A")->y);
    TEST_ASSERT_EQUAL(60, lot52_node("p")->h);
    TEST_ASSERT_EQUAL(60, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l60_negative_row_6_all(void) {
    lot52_render("<body style='margin:0'><div id=p style='display:grid;width:160px;grid-template-columns:40px;grid-template-rows"
        ":repeat(6,10px);align-items:stretch'><div id=a style='grid-row:1 / -1;background:blue'>A</div></div><div id=b "
        "style='height:5px'></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(60, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(1, lot52_node("a")->grid_col_span);
    TEST_ASSERT_EQUAL(6, lot52_node("a")->grid_row_span);
    TEST_ASSERT_NOT_NULL(find_text("A"));
    TEST_ASSERT_EQUAL(0, find_text("A")->x);
    TEST_ASSERT_EQUAL(1, find_text("A")->y);
    TEST_ASSERT_EQUAL(60, lot52_node("p")->h);
    TEST_ASSERT_EQUAL(60, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l60_negative_row_7_last(void) {
    lot52_render("<body style='margin:0'><div id=p style='display:grid;width:160px;grid-template-columns:40px;grid-template-rows"
        ":repeat(7,10px);align-items:stretch'><div id=a style='grid-row:-2 / -1;background:blue'>A</div></div><div id=b"
        " style='height:5px'></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(60, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(1, lot52_node("a")->grid_col_span);
    TEST_ASSERT_EQUAL(1, lot52_node("a")->grid_row_span);
    TEST_ASSERT_NOT_NULL(find_text("A"));
    TEST_ASSERT_EQUAL(0, find_text("A")->x);
    TEST_ASSERT_EQUAL(61, find_text("A")->y);
    TEST_ASSERT_EQUAL(70, lot52_node("p")->h);
    TEST_ASSERT_EQUAL(70, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l60_negative_row_7_all(void) {
    lot52_render("<body style='margin:0'><div id=p style='display:grid;width:160px;grid-template-columns:40px;grid-template-rows"
        ":repeat(7,10px);align-items:stretch'><div id=a style='grid-row:1 / -1;background:blue'>A</div></div><div id=b "
        "style='height:5px'></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(70, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(1, lot52_node("a")->grid_col_span);
    TEST_ASSERT_EQUAL(7, lot52_node("a")->grid_row_span);
    TEST_ASSERT_NOT_NULL(find_text("A"));
    TEST_ASSERT_EQUAL(0, find_text("A")->x);
    TEST_ASSERT_EQUAL(1, find_text("A")->y);
    TEST_ASSERT_EQUAL(70, lot52_node("p")->h);
    TEST_ASSERT_EQUAL(70, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l60_negative_row_8_last(void) {
    lot52_render("<body style='margin:0'><div id=p style='display:grid;width:160px;grid-template-columns:40px;grid-template-rows"
        ":repeat(8,10px);align-items:stretch'><div id=a style='grid-row:-2 / -1;background:blue'>A</div></div><div id=b"
        " style='height:5px'></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(70, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(1, lot52_node("a")->grid_col_span);
    TEST_ASSERT_EQUAL(1, lot52_node("a")->grid_row_span);
    TEST_ASSERT_NOT_NULL(find_text("A"));
    TEST_ASSERT_EQUAL(0, find_text("A")->x);
    TEST_ASSERT_EQUAL(71, find_text("A")->y);
    TEST_ASSERT_EQUAL(80, lot52_node("p")->h);
    TEST_ASSERT_EQUAL(80, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l60_negative_row_8_all(void) {
    lot52_render("<body style='margin:0'><div id=p style='display:grid;width:160px;grid-template-columns:40px;grid-template-rows"
        ":repeat(8,10px);align-items:stretch'><div id=a style='grid-row:1 / -1;background:blue'>A</div></div><div id=b "
        "style='height:5px'></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(80, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(1, lot52_node("a")->grid_col_span);
    TEST_ASSERT_EQUAL(8, lot52_node("a")->grid_row_span);
    TEST_ASSERT_NOT_NULL(find_text("A"));
    TEST_ASSERT_EQUAL(0, find_text("A")->x);
    TEST_ASSERT_EQUAL(1, find_text("A")->y);
    TEST_ASSERT_EQUAL(80, lot52_node("p")->h);
    TEST_ASSERT_EQUAL(80, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}
