static void test_l60_aliases_start_only(void) {
    lot52_render("<body style='margin:0'><div id=p style='display:grid;width:160px;grid-template-columns:[a] 20px [b] 20px [c] 2"
        "0px [d];grid-template-rows:20px;align-items:stretch'><div id=a style='grid-column:b;background:red'>A</div></d"
        "iv><div id=b style='height:5px'></div></body>", 320);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(2, lot52_node("a")->grid_col_start);
    TEST_ASSERT_EQUAL(1, lot52_node("a")->grid_col_span);
    TEST_ASSERT_EQUAL(3, lot52_node("p")->grid_cols);
    TEST_ASSERT_NOT_NULL(find_text("A"));
    TEST_ASSERT_EQUAL(20, find_text("A")->x);
    TEST_ASSERT_EQUAL(1, find_text("A")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l60_aliases_end_auto(void) {
    lot52_render("<body style='margin:0'><div id=p style='display:grid;width:160px;grid-template-columns:[a] 20px [b] 20px [c] 2"
        "0px [d];grid-template-rows:20px;align-items:stretch'><div id=a style='grid-column:b / auto;background:red'>A</"
        "div></div><div id=b style='height:5px'></div></body>", 320);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(2, lot52_node("a")->grid_col_start);
    TEST_ASSERT_EQUAL(1, lot52_node("a")->grid_col_span);
    TEST_ASSERT_EQUAL(3, lot52_node("p")->grid_cols);
    TEST_ASSERT_NOT_NULL(find_text("A"));
    TEST_ASSERT_EQUAL(20, find_text("A")->x);
    TEST_ASSERT_EQUAL(1, find_text("A")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l60_aliases_start_auto(void) {
    lot52_render("<body style='margin:0'><div id=p style='display:grid;width:160px;grid-template-columns:[a] 20px [b] 20px [c] 2"
        "0px [d];grid-template-rows:20px;align-items:stretch'><div id=a style='grid-column:auto / c;background:red'>A</"
        "div></div><div id=b style='height:5px'></div></body>", 320);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(2, lot52_node("a")->grid_col_start);
    TEST_ASSERT_EQUAL(1, lot52_node("a")->grid_col_span);
    TEST_ASSERT_EQUAL(3, lot52_node("p")->grid_cols);
    TEST_ASSERT_NOT_NULL(find_text("A"));
    TEST_ASSERT_EQUAL(20, find_text("A")->x);
    TEST_ASSERT_EQUAL(1, find_text("A")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l60_aliases_span_end(void) {
    lot52_render("<body style='margin:0'><div id=p style='display:grid;width:160px;grid-template-columns:[a] 20px [b] 20px [c] 2"
        "0px [d];grid-template-rows:20px;align-items:stretch'><div id=a style='grid-column:a / span 2;background:red'>A"
        "</div></div><div id=b style='height:5px'></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(1, lot52_node("a")->grid_col_start);
    TEST_ASSERT_EQUAL(2, lot52_node("a")->grid_col_span);
    TEST_ASSERT_EQUAL(3, lot52_node("p")->grid_cols);
    TEST_ASSERT_NOT_NULL(find_text("A"));
    TEST_ASSERT_EQUAL(0, find_text("A")->x);
    TEST_ASSERT_EQUAL(1, find_text("A")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l60_aliases_span_start(void) {
    lot52_render("<body style='margin:0'><div id=p style='display:grid;width:160px;grid-template-columns:[a] 20px [b] 20px [c] 2"
        "0px [d];grid-template-rows:20px;align-items:stretch'><div id=a style='grid-column:span 2 / d;background:red'>A"
        "</div></div><div id=b style='height:5px'></div></body>", 320);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(2, lot52_node("a")->grid_col_start);
    TEST_ASSERT_EQUAL(2, lot52_node("a")->grid_col_span);
    TEST_ASSERT_EQUAL(3, lot52_node("p")->grid_cols);
    TEST_ASSERT_NOT_NULL(find_text("A"));
    TEST_ASSERT_EQUAL(20, find_text("A")->x);
    TEST_ASSERT_EQUAL(1, find_text("A")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l60_aliases_named_numeric(void) {
    lot52_render("<body style='margin:0'><div id=p style='display:grid;width:160px;grid-template-columns:[a] 20px [b] 20px [c] 2"
        "0px [d];grid-template-rows:20px;align-items:stretch'><div id=a style='grid-column:b / 4;background:red'>A</div"
        "></div><div id=b style='height:5px'></div></body>", 320);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(2, lot52_node("a")->grid_col_start);
    TEST_ASSERT_EQUAL(2, lot52_node("a")->grid_col_span);
    TEST_ASSERT_EQUAL(3, lot52_node("p")->grid_cols);
    TEST_ASSERT_NOT_NULL(find_text("A"));
    TEST_ASSERT_EQUAL(20, find_text("A")->x);
    TEST_ASSERT_EQUAL(1, find_text("A")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l60_aliases_numeric_named(void) {
    lot52_render("<body style='margin:0'><div id=p style='display:grid;width:160px;grid-template-columns:[a] 20px [b] 20px [c] 2"
        "0px [d];grid-template-rows:20px;align-items:stretch'><div id=a style='grid-column:2 / d;background:red'>A</div"
        "></div><div id=b style='height:5px'></div></body>", 320);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(2, lot52_node("a")->grid_col_start);
    TEST_ASSERT_EQUAL(2, lot52_node("a")->grid_col_span);
    TEST_ASSERT_EQUAL(3, lot52_node("p")->grid_cols);
    TEST_ASSERT_NOT_NULL(find_text("A"));
    TEST_ASSERT_EQUAL(20, find_text("A")->x);
    TEST_ASSERT_EQUAL(1, find_text("A")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l60_aliases_named_negative(void) {
    lot52_render("<body style='margin:0'><div id=p style='display:grid;width:160px;grid-template-columns:[a] 20px [b] 20px [c] 2"
        "0px [d];grid-template-rows:20px;align-items:stretch'><div id=a style='grid-column:b / -1;background:red'>A</di"
        "v></div><div id=b style='height:5px'></div></body>", 320);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(2, lot52_node("a")->grid_col_start);
    TEST_ASSERT_EQUAL(2, lot52_node("a")->grid_col_span);
    TEST_ASSERT_EQUAL(3, lot52_node("p")->grid_cols);
    TEST_ASSERT_NOT_NULL(find_text("A"));
    TEST_ASSERT_EQUAL(20, find_text("A")->x);
    TEST_ASSERT_EQUAL(1, find_text("A")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l60_aliases_negative_named(void) {
    lot52_render("<body style='margin:0'><div id=p style='display:grid;width:160px;grid-template-columns:[a] 20px [b] 20px [c] 2"
        "0px [d];grid-template-rows:20px;align-items:stretch'><div id=a style='grid-column:-3 / d;background:red'>A</di"
        "v></div><div id=b style='height:5px'></div></body>", 320);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(2, lot52_node("a")->grid_col_start);
    TEST_ASSERT_EQUAL(2, lot52_node("a")->grid_col_span);
    TEST_ASSERT_EQUAL(3, lot52_node("p")->grid_cols);
    TEST_ASSERT_NOT_NULL(find_text("A"));
    TEST_ASSERT_EQUAL(20, find_text("A")->x);
    TEST_ASSERT_EQUAL(1, find_text("A")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l60_aliases_alias_start(void) {
    lot52_render("<body style='margin:0'><div id=p style='display:grid;width:160px;grid-template-columns:[a alpha] 20px [b beta]"
        " 20px [c gamma] 20px [d delta];grid-template-rows:20px;align-items:stretch'><div id=a style='grid-column:beta "
        "/ delta;background:red'>A</div></div><div id=b style='height:5px'></div></body>", 320);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(2, lot52_node("a")->grid_col_start);
    TEST_ASSERT_EQUAL(2, lot52_node("a")->grid_col_span);
    TEST_ASSERT_EQUAL(3, lot52_node("p")->grid_cols);
    TEST_ASSERT_NOT_NULL(find_text("A"));
    TEST_ASSERT_EQUAL(20, find_text("A")->x);
    TEST_ASSERT_EQUAL(1, find_text("A")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l60_aliases_alias_end(void) {
    lot52_render("<body style='margin:0'><div id=p style='display:grid;width:160px;grid-template-columns:[a alpha] 20px [b beta]"
        " 20px [c gamma] 20px [d delta];grid-template-rows:20px;align-items:stretch'><div id=a style='grid-column:alpha"
        " / gamma;background:red'>A</div></div><div id=b style='height:5px'></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(1, lot52_node("a")->grid_col_start);
    TEST_ASSERT_EQUAL(2, lot52_node("a")->grid_col_span);
    TEST_ASSERT_EQUAL(3, lot52_node("p")->grid_cols);
    TEST_ASSERT_NOT_NULL(find_text("A"));
    TEST_ASSERT_EQUAL(0, find_text("A")->x);
    TEST_ASSERT_EQUAL(1, find_text("A")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l60_aliases_repeat_between(void) {
    lot52_render("<body style='margin:0'><div id=p style='display:grid;width:160px;grid-template-columns:[a] repeat(3,20px) [d];"
        "grid-template-rows:20px;align-items:stretch'><div id=a style='grid-column:a / d;background:red'>A</div></div><"
        "div id=b style='height:5px'></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(60, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(1, lot52_node("a")->grid_col_start);
    TEST_ASSERT_EQUAL(3, lot52_node("a")->grid_col_span);
    TEST_ASSERT_EQUAL(3, lot52_node("p")->grid_cols);
    TEST_ASSERT_NOT_NULL(find_text("A"));
    TEST_ASSERT_EQUAL(0, find_text("A")->x);
    TEST_ASSERT_EQUAL(1, find_text("A")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l60_aliases_mixed_repeat(void) {
    lot52_render("<body style='margin:0'><div id=p style='display:grid;width:160px;grid-template-columns:[a] 20px [b] repeat(2,2"
        "0px) [d];grid-template-rows:20px;align-items:stretch'><div id=a style='grid-column:b / d;background:red'>A</di"
        "v></div><div id=b style='height:5px'></div></body>", 320);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(2, lot52_node("a")->grid_col_start);
    TEST_ASSERT_EQUAL(2, lot52_node("a")->grid_col_span);
    TEST_ASSERT_EQUAL(3, lot52_node("p")->grid_cols);
    TEST_ASSERT_NOT_NULL(find_text("A"));
    TEST_ASSERT_EQUAL(20, find_text("A")->x);
    TEST_ASSERT_EQUAL(1, find_text("A")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l60_aliases_duplicate_across_lines(void) {
    lot52_render("<body style='margin:0'><div id=p style='display:grid;width:160px;grid-template-columns:[a] 20px [a] 20px [d];g"
        "rid-template-rows:20px;align-items:stretch'><div id=a style='grid-column:a / d;background:red'>A</div></div><d"
        "iv id=b style='height:5px'></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(1, lot52_node("a")->grid_col_start);
    TEST_ASSERT_EQUAL(2, lot52_node("a")->grid_col_span);
    TEST_ASSERT_EQUAL(2, lot52_node("p")->grid_cols);
    TEST_ASSERT_NOT_NULL(find_text("A"));
    TEST_ASSERT_EQUAL(0, find_text("A")->x);
    TEST_ASSERT_EQUAL(1, find_text("A")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l60_aliases_adjacent_brackets(void) {
    lot52_render("<body style='margin:0'><div id=p style='display:grid;width:160px;grid-template-columns:[a]20px[b]20px[c];grid-"
        "template-rows:20px;align-items:stretch'><div id=a style='grid-column:a / c;background:red'>A</div></div><div i"
        "d=b style='height:5px'></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(1, lot52_node("a")->grid_col_start);
    TEST_ASSERT_EQUAL(2, lot52_node("a")->grid_col_span);
    TEST_ASSERT_EQUAL(2, lot52_node("p")->grid_cols);
    TEST_ASSERT_NOT_NULL(find_text("A"));
    TEST_ASSERT_EQUAL(0, find_text("A")->x);
    TEST_ASSERT_EQUAL(1, find_text("A")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l60_aliases_max_name(void) {
    lot52_render("<body style='margin:0'><div id=p style='display:grid;width:160px;grid-template-columns:[abcdefghijklmno] 20px "
        "[end];grid-template-rows:20px;align-items:stretch'><div id=a style='grid-column:abcdefghijklmno / end;backgrou"
        "nd:red'>A</div></div><div id=b style='height:5px'></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(1, lot52_node("a")->grid_col_start);
    TEST_ASSERT_EQUAL(1, lot52_node("a")->grid_col_span);
    TEST_ASSERT_EQUAL(1, lot52_node("p")->grid_cols);
    TEST_ASSERT_NOT_NULL(find_text("A"));
    TEST_ASSERT_EQUAL(0, find_text("A")->x);
    TEST_ASSERT_EQUAL(1, find_text("A")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l60_aliases_underscores(void) {
    lot52_render("<body style='margin:0'><div id=p style='display:grid;width:160px;grid-template-columns:[_start] 20px [middle_2"
        "] 20px [_end];grid-template-rows:20px;align-items:stretch'><div id=a style='grid-column:middle_2 / _end;backgr"
        "ound:red'>A</div></div><div id=b style='height:5px'></div></body>", 320);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(2, lot52_node("a")->grid_col_start);
    TEST_ASSERT_EQUAL(1, lot52_node("a")->grid_col_span);
    TEST_ASSERT_EQUAL(2, lot52_node("p")->grid_cols);
    TEST_ASSERT_NOT_NULL(find_text("A"));
    TEST_ASSERT_EQUAL(20, find_text("A")->x);
    TEST_ASSERT_EQUAL(1, find_text("A")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l60_aliases_case_distinct(void) {
    lot52_render("<body style='margin:0'><div id=p style='display:grid;width:160px;grid-template-columns:[a] 20px [A] 20px [end]"
        ";grid-template-rows:20px;align-items:stretch'><div id=a style='grid-column:A / end;background:red'>A</div></di"
        "v><div id=b style='height:5px'></div></body>", 320);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(2, lot52_node("a")->grid_col_start);
    TEST_ASSERT_EQUAL(1, lot52_node("a")->grid_col_span);
    TEST_ASSERT_EQUAL(2, lot52_node("p")->grid_cols);
    TEST_ASSERT_NOT_NULL(find_text("A"));
    TEST_ASSERT_EQUAL(20, find_text("A")->x);
    TEST_ASSERT_EQUAL(1, find_text("A")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l60_aliases_auto_span(void) {
    lot52_render("<body style='margin:0'><div id=p style='display:grid;width:160px;grid-template-columns:[a] 20px [b] 20px [c];g"
        "rid-template-rows:20px;align-items:stretch'><div id=a style='grid-column:auto / span 2;background:red'>A</div>"
        "</div><div id=b style='height:5px'></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->grid_col_start);
    TEST_ASSERT_EQUAL(2, lot52_node("a")->grid_col_span);
    TEST_ASSERT_EQUAL(2, lot52_node("p")->grid_cols);
    TEST_ASSERT_NOT_NULL(find_text("A"));
    TEST_ASSERT_EQUAL(0, find_text("A")->x);
    TEST_ASSERT_EQUAL(1, find_text("A")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l60_aliases_positive_end(void) {
    lot52_render("<body style='margin:0'><div id=p style='display:grid;width:160px;grid-template-columns:[a] 20px [b] 20px [c];g"
        "rid-template-rows:20px;align-items:stretch'><div id=a style='grid-column:auto / 3;background:red'>A</div></div"
        "><div id=b style='height:5px'></div></body>", 320);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(2, lot52_node("a")->grid_col_start);
    TEST_ASSERT_EQUAL(1, lot52_node("a")->grid_col_span);
    TEST_ASSERT_EQUAL(2, lot52_node("p")->grid_cols);
    TEST_ASSERT_NOT_NULL(find_text("A"));
    TEST_ASSERT_EQUAL(20, find_text("A")->x);
    TEST_ASSERT_EQUAL(1, find_text("A")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l60_aliases_negative_start_only(void) {
    lot52_render("<body style='margin:0'><div id=p style='display:grid;width:160px;grid-template-columns:[a] 20px [b] 20px [c];g"
        "rid-template-rows:20px;align-items:stretch'><div id=a style='grid-column:-2;background:red'>A</div></div><div "
        "id=b style='height:5px'></div></body>", 320);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(2, lot52_node("a")->grid_col_start);
    TEST_ASSERT_EQUAL(1, lot52_node("a")->grid_col_span);
    TEST_ASSERT_EQUAL(2, lot52_node("p")->grid_cols);
    TEST_ASSERT_NOT_NULL(find_text("A"));
    TEST_ASSERT_EQUAL(20, find_text("A")->x);
    TEST_ASSERT_EQUAL(1, find_text("A")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}
