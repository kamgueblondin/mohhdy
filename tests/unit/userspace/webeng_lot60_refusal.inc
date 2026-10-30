static void test_l60_refusal_template_0(void) {
    lot52_render("<body style='margin:0'><div id=p style='display:grid;width:80px;grid-template-columns:[a] 20px [b] 20px [c];gr"
        "id-template-columns:[a b c] 20px'><div id=a style='grid-column:b / c;height:10px'>A</div></div><div id=b style"
        "='height:5px'></div></body>", 320);
    TEST_ASSERT_EQUAL(2, lot52_node("p")->grid_cols);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(2, lot52_node("a")->grid_col_start);
    TEST_ASSERT_EQUAL(1, lot52_node("a")->grid_col_span);
    TEST_ASSERT_NOT_NULL(find_text("A"));
    TEST_ASSERT_EQUAL(20, find_text("A")->x);
    TEST_ASSERT_EQUAL(1, find_text("A")->y);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->y);
    TEST_ASSERT_TRUE(lot52_has_diag("grid tracks refused"));
}

static void test_l60_refusal_template_1(void) {
    lot52_render("<body style='margin:0'><div id=p style='display:grid;width:80px;grid-template-columns:[a] 20px [b] 20px [c];gr"
        "id-template-columns:[] 20px'><div id=a style='grid-column:b / c;height:10px'>A</div></div><div id=b style='hei"
        "ght:5px'></div></body>", 320);
    TEST_ASSERT_EQUAL(2, lot52_node("p")->grid_cols);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(2, lot52_node("a")->grid_col_start);
    TEST_ASSERT_EQUAL(1, lot52_node("a")->grid_col_span);
    TEST_ASSERT_NOT_NULL(find_text("A"));
    TEST_ASSERT_EQUAL(20, find_text("A")->x);
    TEST_ASSERT_EQUAL(1, find_text("A")->y);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->y);
    TEST_ASSERT_TRUE(lot52_has_diag("grid tracks refused"));
}

static void test_l60_refusal_template_2(void) {
    lot52_render("<body style='margin:0'><div id=p style='display:grid;width:80px;grid-template-columns:[a] 20px [b] 20px [c];gr"
        "id-template-columns:[auto] 20px'><div id=a style='grid-column:b / c;height:10px'>A</div></div><div id=b style="
        "'height:5px'></div></body>", 320);
    TEST_ASSERT_EQUAL(2, lot52_node("p")->grid_cols);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(2, lot52_node("a")->grid_col_start);
    TEST_ASSERT_EQUAL(1, lot52_node("a")->grid_col_span);
    TEST_ASSERT_NOT_NULL(find_text("A"));
    TEST_ASSERT_EQUAL(20, find_text("A")->x);
    TEST_ASSERT_EQUAL(1, find_text("A")->y);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->y);
    TEST_ASSERT_TRUE(lot52_has_diag("grid tracks refused"));
}

static void test_l60_refusal_template_3(void) {
    lot52_render("<body style='margin:0'><div id=p style='display:grid;width:80px;grid-template-columns:[a] 20px [b] 20px [c];gr"
        "id-template-columns:[span] 20px'><div id=a style='grid-column:b / c;height:10px'>A</div></div><div id=b style="
        "'height:5px'></div></body>", 320);
    TEST_ASSERT_EQUAL(2, lot52_node("p")->grid_cols);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(2, lot52_node("a")->grid_col_start);
    TEST_ASSERT_EQUAL(1, lot52_node("a")->grid_col_span);
    TEST_ASSERT_NOT_NULL(find_text("A"));
    TEST_ASSERT_EQUAL(20, find_text("A")->x);
    TEST_ASSERT_EQUAL(1, find_text("A")->y);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->y);
    TEST_ASSERT_TRUE(lot52_has_diag("grid tracks refused"));
}

static void test_l60_refusal_template_4(void) {
    lot52_render("<body style='margin:0'><div id=p style='display:grid;width:80px;grid-template-columns:[a] 20px [b] 20px [c];gr"
        "id-template-columns:[none] 20px'><div id=a style='grid-column:b / c;height:10px'>A</div></div><div id=b style="
        "'height:5px'></div></body>", 320);
    TEST_ASSERT_EQUAL(2, lot52_node("p")->grid_cols);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(2, lot52_node("a")->grid_col_start);
    TEST_ASSERT_EQUAL(1, lot52_node("a")->grid_col_span);
    TEST_ASSERT_NOT_NULL(find_text("A"));
    TEST_ASSERT_EQUAL(20, find_text("A")->x);
    TEST_ASSERT_EQUAL(1, find_text("A")->y);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->y);
    TEST_ASSERT_TRUE(lot52_has_diag("grid tracks refused"));
}

static void test_l60_refusal_template_5(void) {
    lot52_render("<body style='margin:0'><div id=p style='display:grid;width:80px;grid-template-columns:[a] 20px [b] 20px [c];gr"
        "id-template-columns:[inherit] 20px'><div id=a style='grid-column:b / c;height:10px'>A</div></div><div id=b sty"
        "le='height:5px'></div></body>", 320);
    TEST_ASSERT_EQUAL(2, lot52_node("p")->grid_cols);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(2, lot52_node("a")->grid_col_start);
    TEST_ASSERT_EQUAL(1, lot52_node("a")->grid_col_span);
    TEST_ASSERT_NOT_NULL(find_text("A"));
    TEST_ASSERT_EQUAL(20, find_text("A")->x);
    TEST_ASSERT_EQUAL(1, find_text("A")->y);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->y);
    TEST_ASSERT_TRUE(lot52_has_diag("grid tracks refused"));
}

static void test_l60_refusal_template_6(void) {
    lot52_render("<body style='margin:0'><div id=p style='display:grid;width:80px;grid-template-columns:[a] 20px [b] 20px [c];gr"
        "id-template-columns:[a a] 20px'><div id=a style='grid-column:b / c;height:10px'>A</div></div><div id=b style='"
        "height:5px'></div></body>", 320);
    TEST_ASSERT_EQUAL(2, lot52_node("p")->grid_cols);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(2, lot52_node("a")->grid_col_start);
    TEST_ASSERT_EQUAL(1, lot52_node("a")->grid_col_span);
    TEST_ASSERT_NOT_NULL(find_text("A"));
    TEST_ASSERT_EQUAL(20, find_text("A")->x);
    TEST_ASSERT_EQUAL(1, find_text("A")->y);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->y);
    TEST_ASSERT_TRUE(lot52_has_diag("grid tracks refused"));
}

static void test_l60_refusal_template_7(void) {
    lot52_render("<body style='margin:0'><div id=p style='display:grid;width:80px;grid-template-columns:[a] 20px [b] 20px [c];gr"
        "id-template-columns:[abcdefghijklmnoo] 20px'><div id=a style='grid-column:b / c;height:10px'>A</div></div><div"
        " id=b style='height:5px'></div></body>", 320);
    TEST_ASSERT_EQUAL(2, lot52_node("p")->grid_cols);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(2, lot52_node("a")->grid_col_start);
    TEST_ASSERT_EQUAL(1, lot52_node("a")->grid_col_span);
    TEST_ASSERT_NOT_NULL(find_text("A"));
    TEST_ASSERT_EQUAL(20, find_text("A")->x);
    TEST_ASSERT_EQUAL(1, find_text("A")->y);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->y);
    TEST_ASSERT_TRUE(lot52_has_diag("grid tracks refused"));
}

static void test_l60_refusal_template_8(void) {
    lot52_render("<body style='margin:0'><div id=p style='display:grid;width:80px;grid-template-columns:[a] 20px [b] 20px [c];gr"
        "id-template-columns:[1a] 20px'><div id=a style='grid-column:b / c;height:10px'>A</div></div><div id=b style='h"
        "eight:5px'></div></body>", 320);
    TEST_ASSERT_EQUAL(2, lot52_node("p")->grid_cols);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(2, lot52_node("a")->grid_col_start);
    TEST_ASSERT_EQUAL(1, lot52_node("a")->grid_col_span);
    TEST_ASSERT_NOT_NULL(find_text("A"));
    TEST_ASSERT_EQUAL(20, find_text("A")->x);
    TEST_ASSERT_EQUAL(1, find_text("A")->y);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->y);
    TEST_ASSERT_TRUE(lot52_has_diag("grid tracks refused"));
}

static void test_l60_refusal_template_9(void) {
    lot52_render("<body style='margin:0'><div id=p style='display:grid;width:80px;grid-template-columns:[a] 20px [b] 20px [c];gr"
        "id-template-columns:[a 20px'><div id=a style='grid-column:b / c;height:10px'>A</div></div><div id=b style='hei"
        "ght:5px'></div></body>", 320);
    TEST_ASSERT_EQUAL(2, lot52_node("p")->grid_cols);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(2, lot52_node("a")->grid_col_start);
    TEST_ASSERT_EQUAL(1, lot52_node("a")->grid_col_span);
    TEST_ASSERT_NOT_NULL(find_text("A"));
    TEST_ASSERT_EQUAL(20, find_text("A")->x);
    TEST_ASSERT_EQUAL(1, find_text("A")->y);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->y);
    TEST_ASSERT_TRUE(lot52_has_diag("grid tracks refused"));
}

static void test_l60_refusal_template_10(void) {
    lot52_render("<body style='margin:0'><div id=p style='display:grid;width:80px;grid-template-columns:[a] 20px [b] 20px [c];gr"
        "id-template-columns:20px [a'><div id=a style='grid-column:b / c;height:10px'>A</div></div><div id=b style='hei"
        "ght:5px'></div></body>", 320);
    TEST_ASSERT_EQUAL(2, lot52_node("p")->grid_cols);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(2, lot52_node("a")->grid_col_start);
    TEST_ASSERT_EQUAL(1, lot52_node("a")->grid_col_span);
    TEST_ASSERT_NOT_NULL(find_text("A"));
    TEST_ASSERT_EQUAL(20, find_text("A")->x);
    TEST_ASSERT_EQUAL(1, find_text("A")->y);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->y);
    TEST_ASSERT_TRUE(lot52_has_diag("grid tracks refused"));
}

static void test_l60_refusal_template_11(void) {
    lot52_render("<body style='margin:0'><div id=p style='display:grid;width:80px;grid-template-columns:[a] 20px [b] 20px [c];gr"
        "id-template-columns:[a][b] 20px'><div id=a style='grid-column:b / c;height:10px'>A</div></div><div id=b style="
        "'height:5px'></div></body>", 320);
    TEST_ASSERT_EQUAL(2, lot52_node("p")->grid_cols);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(2, lot52_node("a")->grid_col_start);
    TEST_ASSERT_EQUAL(1, lot52_node("a")->grid_col_span);
    TEST_ASSERT_NOT_NULL(find_text("A"));
    TEST_ASSERT_EQUAL(20, find_text("A")->x);
    TEST_ASSERT_EQUAL(1, find_text("A")->y);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->y);
    TEST_ASSERT_TRUE(lot52_has_diag("grid tracks refused"));
}

static void test_l60_refusal_template_12(void) {
    lot52_render("<body style='margin:0'><div id=p style='display:grid;width:80px;grid-template-columns:[a] 20px [b] 20px [c];gr"
        "id-template-columns:[a] repeat(9,20px)'><div id=a style='grid-column:b / c;height:10px'>A</div></div><div id=b"
        " style='height:5px'></div></body>", 320);
    TEST_ASSERT_EQUAL(2, lot52_node("p")->grid_cols);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(2, lot52_node("a")->grid_col_start);
    TEST_ASSERT_EQUAL(1, lot52_node("a")->grid_col_span);
    TEST_ASSERT_NOT_NULL(find_text("A"));
    TEST_ASSERT_EQUAL(20, find_text("A")->x);
    TEST_ASSERT_EQUAL(1, find_text("A")->y);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->y);
    TEST_ASSERT_TRUE(lot52_has_diag("grid tracks refused"));
}

static void test_l60_refusal_template_13(void) {
    lot52_render("<body style='margin:0'><div id=p style='display:grid;width:80px;grid-template-columns:[a] 20px [b] 20px [c];gr"
        "id-template-columns:[a] 20px repeat(8,20px)'><div id=a style='grid-column:b / c;height:10px'>A</div></div><div"
        " id=b style='height:5px'></div></body>", 320);
    TEST_ASSERT_EQUAL(2, lot52_node("p")->grid_cols);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(2, lot52_node("a")->grid_col_start);
    TEST_ASSERT_EQUAL(1, lot52_node("a")->grid_col_span);
    TEST_ASSERT_NOT_NULL(find_text("A"));
    TEST_ASSERT_EQUAL(20, find_text("A")->x);
    TEST_ASSERT_EQUAL(1, find_text("A")->y);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->y);
    TEST_ASSERT_TRUE(lot52_has_diag("grid tracks refused"));
}

static void test_l60_refusal_template_14(void) {
    lot52_render("<body style='margin:0'><div id=p style='display:grid;width:80px;grid-template-columns:[a] 20px [b] 20px [c];gr"
        "id-template-columns:[a] repeat(2,[b] 20px)'><div id=a style='grid-column:b / c;height:10px'>A</div></div><div "
        "id=b style='height:5px'></div></body>", 320);
    TEST_ASSERT_EQUAL(2, lot52_node("p")->grid_cols);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(2, lot52_node("a")->grid_col_start);
    TEST_ASSERT_EQUAL(1, lot52_node("a")->grid_col_span);
    TEST_ASSERT_NOT_NULL(find_text("A"));
    TEST_ASSERT_EQUAL(20, find_text("A")->x);
    TEST_ASSERT_EQUAL(1, find_text("A")->y);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->y);
    TEST_ASSERT_TRUE(lot52_has_diag("grid tracks refused"));
}

static void test_l60_refusal_template_15(void) {
    lot52_render("<body style='margin:0'><div id=p style='display:grid;width:80px;grid-template-columns:[a] 20px [b] 20px [c];gr"
        "id-template-columns:[a] 20px [b] bogus'><div id=a style='grid-column:b / c;height:10px'>A</div></div><div id=b"
        " style='height:5px'></div></body>", 320);
    TEST_ASSERT_EQUAL(2, lot52_node("p")->grid_cols);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(2, lot52_node("a")->grid_col_start);
    TEST_ASSERT_EQUAL(1, lot52_node("a")->grid_col_span);
    TEST_ASSERT_NOT_NULL(find_text("A"));
    TEST_ASSERT_EQUAL(20, find_text("A")->x);
    TEST_ASSERT_EQUAL(1, find_text("A")->y);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->y);
    TEST_ASSERT_TRUE(lot52_has_diag("grid tracks refused"));
}

static void test_l60_refusal_template_16(void) {
    lot52_render("<body style='margin:0'><div id=p style='display:grid;width:80px;grid-template-columns:[a] 20px [b] 20px [c];gr"
        "id-template-columns:[a] 20px [b] 1.5px'><div id=a style='grid-column:b / c;height:10px'>A</div></div><div id=b"
        " style='height:5px'></div></body>", 320);
    TEST_ASSERT_EQUAL(2, lot52_node("p")->grid_cols);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(2, lot52_node("a")->grid_col_start);
    TEST_ASSERT_EQUAL(1, lot52_node("a")->grid_col_span);
    TEST_ASSERT_NOT_NULL(find_text("A"));
    TEST_ASSERT_EQUAL(20, find_text("A")->x);
    TEST_ASSERT_EQUAL(1, find_text("A")->y);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->y);
    TEST_ASSERT_TRUE(lot52_has_diag("grid tracks refused"));
}

static void test_l60_refusal_template_17(void) {
    lot52_render("<body style='margin:0'><div id=p style='display:grid;width:80px;grid-template-columns:[a] 20px [b] 20px [c];gr"
        "id-template-columns:[a] 20px [b] minmax(10px,nope)'><div id=a style='grid-column:b / c;height:10px'>A</div></d"
        "iv><div id=b style='height:5px'></div></body>", 320);
    TEST_ASSERT_EQUAL(2, lot52_node("p")->grid_cols);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(2, lot52_node("a")->grid_col_start);
    TEST_ASSERT_EQUAL(1, lot52_node("a")->grid_col_span);
    TEST_ASSERT_NOT_NULL(find_text("A"));
    TEST_ASSERT_EQUAL(20, find_text("A")->x);
    TEST_ASSERT_EQUAL(1, find_text("A")->y);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->y);
    TEST_ASSERT_TRUE(lot52_has_diag("grid tracks refused"));
}

static void test_l60_refusal_placement_0(void) {
    lot52_render("<body style='margin:0'><div id=p style='display:grid;width:80px;grid-template-columns:[start] 20px [mid] 20px "
        "[end]'><div id=a style='grid-column:mid / end;grid-column:0;height:10px'>A</div></div><div id=b style='height:"
        "5px'></div></body>", 320);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(2, lot52_node("a")->grid_col_start);
    TEST_ASSERT_EQUAL(1, lot52_node("a")->grid_col_span);
    TEST_ASSERT_NOT_NULL(find_text("A"));
    TEST_ASSERT_EQUAL(20, find_text("A")->x);
    TEST_ASSERT_EQUAL(1, find_text("A")->y);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->y);
    TEST_ASSERT_TRUE(lot52_has_diag("grid placement refused"));
}

static void test_l60_refusal_placement_1(void) {
    lot52_render("<body style='margin:0'><div id=p style='display:grid;width:80px;grid-template-columns:[start] 20px [mid] 20px "
        "[end]'><div id=a style='grid-column:mid / end;grid-column:9;height:10px'>A</div></div><div id=b style='height:"
        "5px'></div></body>", 320);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(2, lot52_node("a")->grid_col_start);
    TEST_ASSERT_EQUAL(1, lot52_node("a")->grid_col_span);
    TEST_ASSERT_NOT_NULL(find_text("A"));
    TEST_ASSERT_EQUAL(20, find_text("A")->x);
    TEST_ASSERT_EQUAL(1, find_text("A")->y);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->y);
    TEST_ASSERT_TRUE(lot52_has_diag("grid placement refused"));
}

static void test_l60_refusal_placement_2(void) {
    lot52_render("<body style='margin:0'><div id=p style='display:grid;width:80px;grid-template-columns:[start] 20px [mid] 20px "
        "[end]'><div id=a style='grid-column:mid / end;grid-column:8 / span 2;height:10px'>A</div></div><div id=b style"
        "='height:5px'></div></body>", 320);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(2, lot52_node("a")->grid_col_start);
    TEST_ASSERT_EQUAL(1, lot52_node("a")->grid_col_span);
    TEST_ASSERT_NOT_NULL(find_text("A"));
    TEST_ASSERT_EQUAL(20, find_text("A")->x);
    TEST_ASSERT_EQUAL(1, find_text("A")->y);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->y);
    TEST_ASSERT_TRUE(lot52_has_diag("grid placement refused"));
}

static void test_l60_refusal_placement_3(void) {
    lot52_render("<body style='margin:0'><div id=p style='display:grid;width:80px;grid-template-columns:[start] 20px [mid] 20px "
        "[end]'><div id=a style='grid-column:mid / end;grid-column:2 / 1;height:10px'>A</div></div><div id=b style='hei"
        "ght:5px'></div></body>", 320);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(2, lot52_node("a")->grid_col_start);
    TEST_ASSERT_EQUAL(1, lot52_node("a")->grid_col_span);
    TEST_ASSERT_NOT_NULL(find_text("A"));
    TEST_ASSERT_EQUAL(20, find_text("A")->x);
    TEST_ASSERT_EQUAL(1, find_text("A")->y);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->y);
    TEST_ASSERT_TRUE(lot52_has_diag("grid placement refused"));
}

static void test_l60_refusal_placement_4(void) {
    lot52_render("<body style='margin:0'><div id=p style='display:grid;width:80px;grid-template-columns:[start] 20px [mid] 20px "
        "[end]'><div id=a style='grid-column:mid / end;grid-column:span 0 / end;height:10px'>A</div></div><div id=b sty"
        "le='height:5px'></div></body>", 320);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(2, lot52_node("a")->grid_col_start);
    TEST_ASSERT_EQUAL(1, lot52_node("a")->grid_col_span);
    TEST_ASSERT_NOT_NULL(find_text("A"));
    TEST_ASSERT_EQUAL(20, find_text("A")->x);
    TEST_ASSERT_EQUAL(1, find_text("A")->y);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->y);
    TEST_ASSERT_TRUE(lot52_has_diag("grid placement refused"));
}

static void test_l60_refusal_placement_5(void) {
    lot52_render("<body style='margin:0'><div id=p style='display:grid;width:80px;grid-template-columns:[start] 20px [mid] 20px "
        "[end]'><div id=a style='grid-column:mid / end;grid-column:span 9 / end;height:10px'>A</div></div><div id=b sty"
        "le='height:5px'></div></body>", 320);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(2, lot52_node("a")->grid_col_start);
    TEST_ASSERT_EQUAL(1, lot52_node("a")->grid_col_span);
    TEST_ASSERT_NOT_NULL(find_text("A"));
    TEST_ASSERT_EQUAL(20, find_text("A")->x);
    TEST_ASSERT_EQUAL(1, find_text("A")->y);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->y);
    TEST_ASSERT_TRUE(lot52_has_diag("grid placement refused"));
}

static void test_l60_refusal_placement_6(void) {
    lot52_render("<body style='margin:0'><div id=p style='display:grid;width:80px;grid-template-columns:[start] 20px [mid] 20px "
        "[end]'><div id=a style='grid-column:mid / end;grid-column:span 2 / span 2;height:10px'>A</div></div><div id=b "
        "style='height:5px'></div></body>", 320);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(2, lot52_node("a")->grid_col_start);
    TEST_ASSERT_EQUAL(1, lot52_node("a")->grid_col_span);
    TEST_ASSERT_NOT_NULL(find_text("A"));
    TEST_ASSERT_EQUAL(20, find_text("A")->x);
    TEST_ASSERT_EQUAL(1, find_text("A")->y);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->y);
    TEST_ASSERT_TRUE(lot52_has_diag("grid placement refused"));
}

static void test_l60_refusal_placement_7(void) {
    lot52_render("<body style='margin:0'><div id=p style='display:grid;width:80px;grid-template-columns:[start] 20px [mid] 20px "
        "[end]'><div id=a style='grid-column:mid / end;grid-column:span 2 / auto;height:10px'>A</div></div><div id=b st"
        "yle='height:5px'></div></body>", 320);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(2, lot52_node("a")->grid_col_start);
    TEST_ASSERT_EQUAL(1, lot52_node("a")->grid_col_span);
    TEST_ASSERT_NOT_NULL(find_text("A"));
    TEST_ASSERT_EQUAL(20, find_text("A")->x);
    TEST_ASSERT_EQUAL(1, find_text("A")->y);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->y);
    TEST_ASSERT_TRUE(lot52_has_diag("grid placement refused"));
}

static void test_l60_refusal_placement_8(void) {
    lot52_render("<body style='margin:0'><div id=p style='display:grid;width:80px;grid-template-columns:[start] 20px [mid] 20px "
        "[end]'><div id=a style='grid-column:mid / end;grid-column:start / span 0;height:10px'>A</div></div><div id=b s"
        "tyle='height:5px'></div></body>", 320);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(2, lot52_node("a")->grid_col_start);
    TEST_ASSERT_EQUAL(1, lot52_node("a")->grid_col_span);
    TEST_ASSERT_NOT_NULL(find_text("A"));
    TEST_ASSERT_EQUAL(20, find_text("A")->x);
    TEST_ASSERT_EQUAL(1, find_text("A")->y);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->y);
    TEST_ASSERT_TRUE(lot52_has_diag("grid placement refused"));
}

static void test_l60_refusal_placement_9(void) {
    lot52_render("<body style='margin:0'><div id=p style='display:grid;width:80px;grid-template-columns:[start] 20px [mid] 20px "
        "[end]'><div id=a style='grid-column:mid / end;grid-column:start / span 9;height:10px'>A</div></div><div id=b s"
        "tyle='height:5px'></div></body>", 320);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(2, lot52_node("a")->grid_col_start);
    TEST_ASSERT_EQUAL(1, lot52_node("a")->grid_col_span);
    TEST_ASSERT_NOT_NULL(find_text("A"));
    TEST_ASSERT_EQUAL(20, find_text("A")->x);
    TEST_ASSERT_EQUAL(1, find_text("A")->y);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->y);
    TEST_ASSERT_TRUE(lot52_has_diag("grid placement refused"));
}

static void test_l60_refusal_placement_10(void) {
    lot52_render("<body style='margin:0'><div id=p style='display:grid;width:80px;grid-template-columns:[start] 20px [mid] 20px "
        "[end]'><div id=a style='grid-column:mid / end;grid-column:start / end / last;height:10px'>A</div></div><div id"
        "=b style='height:5px'></div></body>", 320);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(2, lot52_node("a")->grid_col_start);
    TEST_ASSERT_EQUAL(1, lot52_node("a")->grid_col_span);
    TEST_ASSERT_NOT_NULL(find_text("A"));
    TEST_ASSERT_EQUAL(20, find_text("A")->x);
    TEST_ASSERT_EQUAL(1, find_text("A")->y);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->y);
    TEST_ASSERT_TRUE(lot52_has_diag("grid placement refused"));
}

static void test_l60_refusal_placement_11(void) {
    lot52_render("<body style='margin:0'><div id=p style='display:grid;width:80px;grid-template-columns:[start] 20px [mid] 20px "
        "[end]'><div id=a style='grid-column:mid / end;grid-column:start 0 / end;height:10px'>A</div></div><div id=b st"
        "yle='height:5px'></div></body>", 320);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(2, lot52_node("a")->grid_col_start);
    TEST_ASSERT_EQUAL(1, lot52_node("a")->grid_col_span);
    TEST_ASSERT_NOT_NULL(find_text("A"));
    TEST_ASSERT_EQUAL(20, find_text("A")->x);
    TEST_ASSERT_EQUAL(1, find_text("A")->y);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->y);
    TEST_ASSERT_TRUE(lot52_has_diag("grid placement refused"));
}

static void test_l60_refusal_placement_12(void) {
    lot52_render("<body style='margin:0'><div id=p style='display:grid;width:80px;grid-template-columns:[start] 20px [mid] 20px "
        "[end]'><div id=a style='grid-column:mid / end;grid-column:abcdefghijklmnoo;height:10px'>A</div></div><div id=b"
        " style='height:5px'></div></body>", 320);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(2, lot52_node("a")->grid_col_start);
    TEST_ASSERT_EQUAL(1, lot52_node("a")->grid_col_span);
    TEST_ASSERT_NOT_NULL(find_text("A"));
    TEST_ASSERT_EQUAL(20, find_text("A")->x);
    TEST_ASSERT_EQUAL(1, find_text("A")->y);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->y);
    TEST_ASSERT_TRUE(lot52_has_diag("grid placement refused"));
}

static void test_l60_refusal_placement_13(void) {
    lot52_render("<body style='margin:0'><div id=p style='display:grid;width:80px;grid-template-columns:[start] 20px [mid] 20px "
        "[end]'><div id=a style='grid-column:mid / end;grid-column:start /;height:10px'>A</div></div><div id=b style='h"
        "eight:5px'></div></body>", 320);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(2, lot52_node("a")->grid_col_start);
    TEST_ASSERT_EQUAL(1, lot52_node("a")->grid_col_span);
    TEST_ASSERT_NOT_NULL(find_text("A"));
    TEST_ASSERT_EQUAL(20, find_text("A")->x);
    TEST_ASSERT_EQUAL(1, find_text("A")->y);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->y);
    TEST_ASSERT_TRUE(lot52_has_diag("grid placement refused"));
}

static void test_l60_refusal_placement_14(void) {
    lot52_render("<body style='margin:0'><div id=p style='display:grid;width:80px;grid-template-columns:[start] 20px [mid] 20px "
        "[end]'><div id=a style='grid-column:mid / end;grid-column:/ end;height:10px'>A</div></div><div id=b style='hei"
        "ght:5px'></div></body>", 320);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(2, lot52_node("a")->grid_col_start);
    TEST_ASSERT_EQUAL(1, lot52_node("a")->grid_col_span);
    TEST_ASSERT_NOT_NULL(find_text("A"));
    TEST_ASSERT_EQUAL(20, find_text("A")->x);
    TEST_ASSERT_EQUAL(1, find_text("A")->y);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->y);
    TEST_ASSERT_TRUE(lot52_has_diag("grid placement refused"));
}

static void test_l60_refusal_placement_15(void) {
    lot52_render("<body style='margin:0'><div id=p style='display:grid;width:80px;grid-template-columns:[start] 20px [mid] 20px "
        "[end]'><div id=a style='grid-column:mid / end;grid-column:0 / end;height:10px'>A</div></div><div id=b style='h"
        "eight:5px'></div></body>", 320);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(2, lot52_node("a")->grid_col_start);
    TEST_ASSERT_EQUAL(1, lot52_node("a")->grid_col_span);
    TEST_ASSERT_NOT_NULL(find_text("A"));
    TEST_ASSERT_EQUAL(20, find_text("A")->x);
    TEST_ASSERT_EQUAL(1, find_text("A")->y);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->y);
    TEST_ASSERT_TRUE(lot52_has_diag("grid placement refused"));
}

static void test_l60_refusal_placement_16(void) {
    lot52_render("<body style='margin:0'><div id=p style='display:grid;width:80px;grid-template-columns:[start] 20px [mid] 20px "
        "[end]'><div id=a style='grid-column:mid / end;grid-column:start / 0;height:10px'>A</div></div><div id=b style="
        "'height:5px'></div></body>", 320);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(2, lot52_node("a")->grid_col_start);
    TEST_ASSERT_EQUAL(1, lot52_node("a")->grid_col_span);
    TEST_ASSERT_NOT_NULL(find_text("A"));
    TEST_ASSERT_EQUAL(20, find_text("A")->x);
    TEST_ASSERT_EQUAL(1, find_text("A")->y);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->y);
    TEST_ASSERT_TRUE(lot52_has_diag("grid placement refused"));
}

static void test_l60_refusal_placement_17(void) {
    lot52_render("<body style='margin:0'><div id=p style='display:grid;width:80px;grid-template-columns:[start] 20px [mid] 20px "
        "[end]'><div id=a style='grid-column:mid / end;grid-column:-10 / end;height:10px'>A</div></div><div id=b style="
        "'height:5px'></div></body>", 320);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(2, lot52_node("a")->grid_col_start);
    TEST_ASSERT_EQUAL(1, lot52_node("a")->grid_col_span);
    TEST_ASSERT_NOT_NULL(find_text("A"));
    TEST_ASSERT_EQUAL(20, find_text("A")->x);
    TEST_ASSERT_EQUAL(1, find_text("A")->y);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->y);
    TEST_ASSERT_TRUE(lot52_has_diag("grid placement refused"));
}

static void test_l60_refusal_placement_18(void) {
    lot52_render("<body style='margin:0'><div id=p style='display:grid;width:80px;grid-template-columns:[start] 20px [mid] 20px "
        "[end]'><div id=a style='grid-column:mid / end;grid-column:start / 10;height:10px'>A</div></div><div id=b style"
        "='height:5px'></div></body>", 320);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(2, lot52_node("a")->grid_col_start);
    TEST_ASSERT_EQUAL(1, lot52_node("a")->grid_col_span);
    TEST_ASSERT_NOT_NULL(find_text("A"));
    TEST_ASSERT_EQUAL(20, find_text("A")->x);
    TEST_ASSERT_EQUAL(1, find_text("A")->y);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->y);
    TEST_ASSERT_TRUE(lot52_has_diag("grid placement refused"));
}
