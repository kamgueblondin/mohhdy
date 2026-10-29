static void test_l60_baseline_flex_plain_plain(void) {
    lot52_render("<body style='margin:0'><div id=p style='display:flex;width:120px;align-items:baseline'><div id=a style='width:"
        "40px;height:10px;padding-top:0px;padding-bottom:0px;border:0px solid red;margin-top:0px;margin-bottom:0px;flex"
        "-grow:0'>A</div><div id=b style='width:40px;height:10px;padding-top:0px;padding-bottom:0px;border:0px solid bl"
        "ue;margin-top:0px;margin-bottom:0px;flex-grow:0'>B</div></div><div id=c style='height:5px'></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(40, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->h);
    TEST_ASSERT_NOT_NULL(find_text("A"));
    TEST_ASSERT_NOT_NULL(find_text("B"));
    TEST_ASSERT_EQUAL(0, find_text("A")->x);
    TEST_ASSERT_EQUAL(40, find_text("B")->x);
    TEST_ASSERT_EQUAL(1, find_text("A")->y);
    TEST_ASSERT_EQUAL(1, find_text("B")->y);
    TEST_ASSERT_EQUAL(10, lot52_node("p")->h);
    TEST_ASSERT_EQUAL(10, lot52_node("c")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l60_baseline_flex_pad_top_plain(void) {
    lot52_render("<body style='margin:0'><div id=p style='display:flex;width:120px;align-items:baseline'><div id=a style='width:"
        "40px;height:10px;padding-top:5px;padding-bottom:0px;border:0px solid red;margin-top:0px;margin-bottom:0px;flex"
        "-grow:0'>A</div><div id=b style='width:40px;height:10px;padding-top:0px;padding-bottom:0px;border:0px solid bl"
        "ue;margin-top:0px;margin-bottom:0px;flex-grow:0'>B</div></div><div id=c style='height:5px'></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(15, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(40, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(5, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->h);
    TEST_ASSERT_NOT_NULL(find_text("A"));
    TEST_ASSERT_NOT_NULL(find_text("B"));
    TEST_ASSERT_EQUAL(0, find_text("A")->x);
    TEST_ASSERT_EQUAL(40, find_text("B")->x);
    TEST_ASSERT_EQUAL(6, find_text("A")->y);
    TEST_ASSERT_EQUAL(6, find_text("B")->y);
    TEST_ASSERT_EQUAL(15, lot52_node("p")->h);
    TEST_ASSERT_EQUAL(15, lot52_node("c")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l60_baseline_flex_pad_top_pad_top(void) {
    lot52_render("<body style='margin:0'><div id=p style='display:flex;width:120px;align-items:baseline'><div id=a style='width:"
        "40px;height:10px;padding-top:5px;padding-bottom:0px;border:0px solid red;margin-top:0px;margin-bottom:0px;flex"
        "-grow:0'>A</div><div id=b style='width:40px;height:10px;padding-top:5px;padding-bottom:0px;border:0px solid bl"
        "ue;margin-top:0px;margin-bottom:0px;flex-grow:0'>B</div></div><div id=c style='height:5px'></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(15, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(40, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(15, lot52_node("b")->h);
    TEST_ASSERT_NOT_NULL(find_text("A"));
    TEST_ASSERT_NOT_NULL(find_text("B"));
    TEST_ASSERT_EQUAL(0, find_text("A")->x);
    TEST_ASSERT_EQUAL(40, find_text("B")->x);
    TEST_ASSERT_EQUAL(6, find_text("A")->y);
    TEST_ASSERT_EQUAL(6, find_text("B")->y);
    TEST_ASSERT_EQUAL(15, lot52_node("p")->h);
    TEST_ASSERT_EQUAL(15, lot52_node("c")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l60_baseline_flex_pad_top_pad_bottom(void) {
    lot52_render("<body style='margin:0'><div id=p style='display:flex;width:120px;align-items:baseline'><div id=a style='width:"
        "40px;height:10px;padding-top:5px;padding-bottom:0px;border:0px solid red;margin-top:0px;margin-bottom:0px;flex"
        "-grow:0'>A</div><div id=b style='width:40px;height:10px;padding-top:0px;padding-bottom:7px;border:0px solid bl"
        "ue;margin-top:0px;margin-bottom:0px;flex-grow:0'>B</div></div><div id=c style='height:5px'></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(15, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(40, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(5, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(17, lot52_node("b")->h);
    TEST_ASSERT_NOT_NULL(find_text("A"));
    TEST_ASSERT_NOT_NULL(find_text("B"));
    TEST_ASSERT_EQUAL(0, find_text("A")->x);
    TEST_ASSERT_EQUAL(40, find_text("B")->x);
    TEST_ASSERT_EQUAL(6, find_text("A")->y);
    TEST_ASSERT_EQUAL(6, find_text("B")->y);
    TEST_ASSERT_EQUAL(22, lot52_node("p")->h);
    TEST_ASSERT_EQUAL(22, lot52_node("c")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l60_baseline_flex_pad_top_border(void) {
    lot52_render("<body style='margin:0'><div id=p style='display:flex;width:120px;align-items:baseline'><div id=a style='width:"
        "40px;height:10px;padding-top:5px;padding-bottom:0px;border:0px solid red;margin-top:0px;margin-bottom:0px;flex"
        "-grow:0'>A</div><div id=b style='width:40px;height:10px;padding-top:0px;padding-bottom:0px;border:2px solid bl"
        "ue;margin-top:0px;margin-bottom:0px;flex-grow:0'>B</div></div><div id=c style='height:5px'></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(15, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(40, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(3, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(44, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(14, lot52_node("b")->h);
    TEST_ASSERT_NOT_NULL(find_text("A"));
    TEST_ASSERT_NOT_NULL(find_text("B"));
    TEST_ASSERT_EQUAL(0, find_text("A")->x);
    TEST_ASSERT_EQUAL(42, find_text("B")->x);
    TEST_ASSERT_EQUAL(6, find_text("A")->y);
    TEST_ASSERT_EQUAL(6, find_text("B")->y);
    TEST_ASSERT_EQUAL(17, lot52_node("p")->h);
    TEST_ASSERT_EQUAL(17, lot52_node("c")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l60_baseline_flex_pad_top_margin_top(void) {
    lot52_render("<body style='margin:0'><div id=p style='display:flex;width:120px;align-items:baseline'><div id=a style='width:"
        "40px;height:10px;padding-top:5px;padding-bottom:0px;border:0px solid red;margin-top:0px;margin-bottom:0px;flex"
        "-grow:0'>A</div><div id=b style='width:40px;height:10px;padding-top:0px;padding-bottom:0px;border:0px solid bl"
        "ue;margin-top:6px;margin-bottom:0px;flex-grow:0'>B</div></div><div id=c style='height:5px'></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(1, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(15, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(40, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(6, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->h);
    TEST_ASSERT_NOT_NULL(find_text("A"));
    TEST_ASSERT_NOT_NULL(find_text("B"));
    TEST_ASSERT_EQUAL(0, find_text("A")->x);
    TEST_ASSERT_EQUAL(40, find_text("B")->x);
    TEST_ASSERT_EQUAL(7, find_text("A")->y);
    TEST_ASSERT_EQUAL(7, find_text("B")->y);
    TEST_ASSERT_EQUAL(16, lot52_node("p")->h);
    TEST_ASSERT_EQUAL(16, lot52_node("c")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l60_baseline_flex_pad_top_margin_bottom(void) {
    lot52_render("<body style='margin:0'><div id=p style='display:flex;width:120px;align-items:baseline'><div id=a style='width:"
        "40px;height:10px;padding-top:5px;padding-bottom:0px;border:0px solid red;margin-top:0px;margin-bottom:0px;flex"
        "-grow:0'>A</div><div id=b style='width:40px;height:10px;padding-top:0px;padding-bottom:0px;border:0px solid bl"
        "ue;margin-top:0px;margin-bottom:8px;flex-grow:0'>B</div></div><div id=c style='height:5px'></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(15, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(40, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(5, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->h);
    TEST_ASSERT_NOT_NULL(find_text("A"));
    TEST_ASSERT_NOT_NULL(find_text("B"));
    TEST_ASSERT_EQUAL(0, find_text("A")->x);
    TEST_ASSERT_EQUAL(40, find_text("B")->x);
    TEST_ASSERT_EQUAL(6, find_text("A")->y);
    TEST_ASSERT_EQUAL(6, find_text("B")->y);
    TEST_ASSERT_EQUAL(23, lot52_node("p")->h);
    TEST_ASSERT_EQUAL(23, lot52_node("c")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l60_baseline_flex_pad_top_tall(void) {
    lot52_render("<body style='margin:0'><div id=p style='display:flex;width:120px;align-items:baseline'><div id=a style='width:"
        "40px;height:10px;padding-top:5px;padding-bottom:0px;border:0px solid red;margin-top:0px;margin-bottom:0px;flex"
        "-grow:0'>A</div><div id=b style='width:40px;height:30px;padding-top:0px;padding-bottom:0px;border:0px solid bl"
        "ue;margin-top:0px;margin-bottom:0px;flex-grow:0'>B</div></div><div id=c style='height:5px'></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(15, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(40, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(5, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(30, lot52_node("b")->h);
    TEST_ASSERT_NOT_NULL(find_text("A"));
    TEST_ASSERT_NOT_NULL(find_text("B"));
    TEST_ASSERT_EQUAL(0, find_text("A")->x);
    TEST_ASSERT_EQUAL(40, find_text("B")->x);
    TEST_ASSERT_EQUAL(6, find_text("A")->y);
    TEST_ASSERT_EQUAL(6, find_text("B")->y);
    TEST_ASSERT_EQUAL(35, lot52_node("p")->h);
    TEST_ASSERT_EQUAL(35, lot52_node("c")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l60_baseline_flex_pad_top_all_edges(void) {
    lot52_render("<body style='margin:0'><div id=p style='display:flex;width:120px;align-items:baseline'><div id=a style='width:"
        "40px;height:10px;padding-top:5px;padding-bottom:0px;border:0px solid red;margin-top:0px;margin-bottom:0px;flex"
        "-grow:0'>A</div><div id=b style='width:40px;height:20px;padding-top:3px;padding-bottom:4px;border:1px solid bl"
        "ue;margin-top:2px;margin-bottom:5px;flex-grow:0'>B</div></div><div id=c style='height:5px'></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(1, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(15, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(40, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(2, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(42, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(29, lot52_node("b")->h);
    TEST_ASSERT_NOT_NULL(find_text("A"));
    TEST_ASSERT_NOT_NULL(find_text("B"));
    TEST_ASSERT_EQUAL(0, find_text("A")->x);
    TEST_ASSERT_EQUAL(41, find_text("B")->x);
    TEST_ASSERT_EQUAL(7, find_text("A")->y);
    TEST_ASSERT_EQUAL(7, find_text("B")->y);
    TEST_ASSERT_EQUAL(36, lot52_node("p")->h);
    TEST_ASSERT_EQUAL(36, lot52_node("c")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l60_baseline_flex_pad_top_short(void) {
    lot52_render("<body style='margin:0'><div id=p style='display:flex;width:120px;align-items:baseline'><div id=a style='width:"
        "40px;height:10px;padding-top:5px;padding-bottom:0px;border:0px solid red;margin-top:0px;margin-bottom:0px;flex"
        "-grow:0'>A</div><div id=b style='width:40px;height:2px;padding-top:0px;padding-bottom:0px;border:0px solid blu"
        "e;margin-top:0px;margin-bottom:0px;flex-grow:0'>B</div></div><div id=c style='height:5px'></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(15, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(40, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(5, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(2, lot52_node("b")->h);
    TEST_ASSERT_NOT_NULL(find_text("A"));
    TEST_ASSERT_NOT_NULL(find_text("B"));
    TEST_ASSERT_EQUAL(0, find_text("A")->x);
    TEST_ASSERT_EQUAL(40, find_text("B")->x);
    TEST_ASSERT_EQUAL(6, find_text("A")->y);
    TEST_ASSERT_EQUAL(6, find_text("B")->y);
    TEST_ASSERT_EQUAL(15, lot52_node("p")->h);
    TEST_ASSERT_EQUAL(15, lot52_node("c")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l60_baseline_flex_pad_bottom_plain(void) {
    lot52_render("<body style='margin:0'><div id=p style='display:flex;width:120px;align-items:baseline'><div id=a style='width:"
        "40px;height:10px;padding-top:0px;padding-bottom:7px;border:0px solid red;margin-top:0px;margin-bottom:0px;flex"
        "-grow:0'>A</div><div id=b style='width:40px;height:10px;padding-top:0px;padding-bottom:0px;border:0px solid bl"
        "ue;margin-top:0px;margin-bottom:0px;flex-grow:0'>B</div></div><div id=c style='height:5px'></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(17, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(40, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->h);
    TEST_ASSERT_NOT_NULL(find_text("A"));
    TEST_ASSERT_NOT_NULL(find_text("B"));
    TEST_ASSERT_EQUAL(0, find_text("A")->x);
    TEST_ASSERT_EQUAL(40, find_text("B")->x);
    TEST_ASSERT_EQUAL(1, find_text("A")->y);
    TEST_ASSERT_EQUAL(1, find_text("B")->y);
    TEST_ASSERT_EQUAL(17, lot52_node("p")->h);
    TEST_ASSERT_EQUAL(17, lot52_node("c")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l60_baseline_flex_border_plain(void) {
    lot52_render("<body style='margin:0'><div id=p style='display:flex;width:120px;align-items:baseline'><div id=a style='width:"
        "40px;height:10px;padding-top:0px;padding-bottom:0px;border:2px solid red;margin-top:0px;margin-bottom:0px;flex"
        "-grow:0'>A</div><div id=b style='width:40px;height:10px;padding-top:0px;padding-bottom:0px;border:0px solid bl"
        "ue;margin-top:0px;margin-bottom:0px;flex-grow:0'>B</div></div><div id=c style='height:5px'></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(44, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(14, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(44, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(2, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->h);
    TEST_ASSERT_NOT_NULL(find_text("A"));
    TEST_ASSERT_NOT_NULL(find_text("B"));
    TEST_ASSERT_EQUAL(2, find_text("A")->x);
    TEST_ASSERT_EQUAL(44, find_text("B")->x);
    TEST_ASSERT_EQUAL(3, find_text("A")->y);
    TEST_ASSERT_EQUAL(3, find_text("B")->y);
    TEST_ASSERT_EQUAL(14, lot52_node("p")->h);
    TEST_ASSERT_EQUAL(14, lot52_node("c")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l60_baseline_flex_border_pad_top(void) {
    lot52_render("<body style='margin:0'><div id=p style='display:flex;width:120px;align-items:baseline'><div id=a style='width:"
        "40px;height:10px;padding-top:0px;padding-bottom:0px;border:2px solid red;margin-top:0px;margin-bottom:0px;flex"
        "-grow:0'>A</div><div id=b style='width:40px;height:10px;padding-top:5px;padding-bottom:0px;border:0px solid bl"
        "ue;margin-top:0px;margin-bottom:0px;flex-grow:0'>B</div></div><div id=c style='height:5px'></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(3, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(44, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(14, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(44, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(15, lot52_node("b")->h);
    TEST_ASSERT_NOT_NULL(find_text("A"));
    TEST_ASSERT_NOT_NULL(find_text("B"));
    TEST_ASSERT_EQUAL(2, find_text("A")->x);
    TEST_ASSERT_EQUAL(44, find_text("B")->x);
    TEST_ASSERT_EQUAL(6, find_text("A")->y);
    TEST_ASSERT_EQUAL(6, find_text("B")->y);
    TEST_ASSERT_EQUAL(17, lot52_node("p")->h);
    TEST_ASSERT_EQUAL(17, lot52_node("c")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l60_baseline_flex_border_pad_bottom(void) {
    lot52_render("<body style='margin:0'><div id=p style='display:flex;width:120px;align-items:baseline'><div id=a style='width:"
        "40px;height:10px;padding-top:0px;padding-bottom:0px;border:2px solid red;margin-top:0px;margin-bottom:0px;flex"
        "-grow:0'>A</div><div id=b style='width:40px;height:10px;padding-top:0px;padding-bottom:7px;border:0px solid bl"
        "ue;margin-top:0px;margin-bottom:0px;flex-grow:0'>B</div></div><div id=c style='height:5px'></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(44, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(14, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(44, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(2, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(17, lot52_node("b")->h);
    TEST_ASSERT_NOT_NULL(find_text("A"));
    TEST_ASSERT_NOT_NULL(find_text("B"));
    TEST_ASSERT_EQUAL(2, find_text("A")->x);
    TEST_ASSERT_EQUAL(44, find_text("B")->x);
    TEST_ASSERT_EQUAL(3, find_text("A")->y);
    TEST_ASSERT_EQUAL(3, find_text("B")->y);
    TEST_ASSERT_EQUAL(19, lot52_node("p")->h);
    TEST_ASSERT_EQUAL(19, lot52_node("c")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l60_baseline_flex_border_border(void) {
    lot52_render("<body style='margin:0'><div id=p style='display:flex;width:120px;align-items:baseline'><div id=a style='width:"
        "40px;height:10px;padding-top:0px;padding-bottom:0px;border:2px solid red;margin-top:0px;margin-bottom:0px;flex"
        "-grow:0'>A</div><div id=b style='width:40px;height:10px;padding-top:0px;padding-bottom:0px;border:2px solid bl"
        "ue;margin-top:0px;margin-bottom:0px;flex-grow:0'>B</div></div><div id=c style='height:5px'></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(44, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(14, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(44, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(44, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(14, lot52_node("b")->h);
    TEST_ASSERT_NOT_NULL(find_text("A"));
    TEST_ASSERT_NOT_NULL(find_text("B"));
    TEST_ASSERT_EQUAL(2, find_text("A")->x);
    TEST_ASSERT_EQUAL(46, find_text("B")->x);
    TEST_ASSERT_EQUAL(3, find_text("A")->y);
    TEST_ASSERT_EQUAL(3, find_text("B")->y);
    TEST_ASSERT_EQUAL(14, lot52_node("p")->h);
    TEST_ASSERT_EQUAL(14, lot52_node("c")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l60_baseline_flex_border_margin_top(void) {
    lot52_render("<body style='margin:0'><div id=p style='display:flex;width:120px;align-items:baseline'><div id=a style='width:"
        "40px;height:10px;padding-top:0px;padding-bottom:0px;border:2px solid red;margin-top:0px;margin-bottom:0px;flex"
        "-grow:0'>A</div><div id=b style='width:40px;height:10px;padding-top:0px;padding-bottom:0px;border:0px solid bl"
        "ue;margin-top:6px;margin-bottom:0px;flex-grow:0'>B</div></div><div id=c style='height:5px'></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(4, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(44, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(14, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(44, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(6, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->h);
    TEST_ASSERT_NOT_NULL(find_text("A"));
    TEST_ASSERT_NOT_NULL(find_text("B"));
    TEST_ASSERT_EQUAL(2, find_text("A")->x);
    TEST_ASSERT_EQUAL(44, find_text("B")->x);
    TEST_ASSERT_EQUAL(7, find_text("A")->y);
    TEST_ASSERT_EQUAL(7, find_text("B")->y);
    TEST_ASSERT_EQUAL(18, lot52_node("p")->h);
    TEST_ASSERT_EQUAL(18, lot52_node("c")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l60_baseline_flex_border_margin_bottom(void) {
    lot52_render("<body style='margin:0'><div id=p style='display:flex;width:120px;align-items:baseline'><div id=a style='width:"
        "40px;height:10px;padding-top:0px;padding-bottom:0px;border:2px solid red;margin-top:0px;margin-bottom:0px;flex"
        "-grow:0'>A</div><div id=b style='width:40px;height:10px;padding-top:0px;padding-bottom:0px;border:0px solid bl"
        "ue;margin-top:0px;margin-bottom:8px;flex-grow:0'>B</div></div><div id=c style='height:5px'></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(44, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(14, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(44, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(2, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->h);
    TEST_ASSERT_NOT_NULL(find_text("A"));
    TEST_ASSERT_NOT_NULL(find_text("B"));
    TEST_ASSERT_EQUAL(2, find_text("A")->x);
    TEST_ASSERT_EQUAL(44, find_text("B")->x);
    TEST_ASSERT_EQUAL(3, find_text("A")->y);
    TEST_ASSERT_EQUAL(3, find_text("B")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("p")->h);
    TEST_ASSERT_EQUAL(20, lot52_node("c")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l60_baseline_flex_border_tall(void) {
    lot52_render("<body style='margin:0'><div id=p style='display:flex;width:120px;align-items:baseline'><div id=a style='width:"
        "40px;height:10px;padding-top:0px;padding-bottom:0px;border:2px solid red;margin-top:0px;margin-bottom:0px;flex"
        "-grow:0'>A</div><div id=b style='width:40px;height:30px;padding-top:0px;padding-bottom:0px;border:0px solid bl"
        "ue;margin-top:0px;margin-bottom:0px;flex-grow:0'>B</div></div><div id=c style='height:5px'></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(44, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(14, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(44, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(2, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(30, lot52_node("b")->h);
    TEST_ASSERT_NOT_NULL(find_text("A"));
    TEST_ASSERT_NOT_NULL(find_text("B"));
    TEST_ASSERT_EQUAL(2, find_text("A")->x);
    TEST_ASSERT_EQUAL(44, find_text("B")->x);
    TEST_ASSERT_EQUAL(3, find_text("A")->y);
    TEST_ASSERT_EQUAL(3, find_text("B")->y);
    TEST_ASSERT_EQUAL(32, lot52_node("p")->h);
    TEST_ASSERT_EQUAL(32, lot52_node("c")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l60_baseline_flex_border_all_edges(void) {
    lot52_render("<body style='margin:0'><div id=p style='display:flex;width:120px;align-items:baseline'><div id=a style='width:"
        "40px;height:10px;padding-top:0px;padding-bottom:0px;border:2px solid red;margin-top:0px;margin-bottom:0px;flex"
        "-grow:0'>A</div><div id=b style='width:40px;height:20px;padding-top:3px;padding-bottom:4px;border:1px solid bl"
        "ue;margin-top:2px;margin-bottom:5px;flex-grow:0'>B</div></div><div id=c style='height:5px'></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(4, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(44, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(14, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(44, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(2, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(42, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(29, lot52_node("b")->h);
    TEST_ASSERT_NOT_NULL(find_text("A"));
    TEST_ASSERT_NOT_NULL(find_text("B"));
    TEST_ASSERT_EQUAL(2, find_text("A")->x);
    TEST_ASSERT_EQUAL(45, find_text("B")->x);
    TEST_ASSERT_EQUAL(7, find_text("A")->y);
    TEST_ASSERT_EQUAL(7, find_text("B")->y);
    TEST_ASSERT_EQUAL(36, lot52_node("p")->h);
    TEST_ASSERT_EQUAL(36, lot52_node("c")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l60_baseline_flex_border_short(void) {
    lot52_render("<body style='margin:0'><div id=p style='display:flex;width:120px;align-items:baseline'><div id=a style='width:"
        "40px;height:10px;padding-top:0px;padding-bottom:0px;border:2px solid red;margin-top:0px;margin-bottom:0px;flex"
        "-grow:0'>A</div><div id=b style='width:40px;height:2px;padding-top:0px;padding-bottom:0px;border:0px solid blu"
        "e;margin-top:0px;margin-bottom:0px;flex-grow:0'>B</div></div><div id=c style='height:5px'></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(44, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(14, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(44, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(2, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(2, lot52_node("b")->h);
    TEST_ASSERT_NOT_NULL(find_text("A"));
    TEST_ASSERT_NOT_NULL(find_text("B"));
    TEST_ASSERT_EQUAL(2, find_text("A")->x);
    TEST_ASSERT_EQUAL(44, find_text("B")->x);
    TEST_ASSERT_EQUAL(3, find_text("A")->y);
    TEST_ASSERT_EQUAL(3, find_text("B")->y);
    TEST_ASSERT_EQUAL(14, lot52_node("p")->h);
    TEST_ASSERT_EQUAL(14, lot52_node("c")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l60_baseline_flex_margin_top_plain(void) {
    lot52_render("<body style='margin:0'><div id=p style='display:flex;width:120px;align-items:baseline'><div id=a style='width:"
        "40px;height:10px;padding-top:0px;padding-bottom:0px;border:0px solid red;margin-top:6px;margin-bottom:0px;flex"
        "-grow:0'>A</div><div id=b style='width:40px;height:10px;padding-top:0px;padding-bottom:0px;border:0px solid bl"
        "ue;margin-top:0px;margin-bottom:0px;flex-grow:0'>B</div></div><div id=c style='height:5px'></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(6, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(40, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(6, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->h);
    TEST_ASSERT_NOT_NULL(find_text("A"));
    TEST_ASSERT_NOT_NULL(find_text("B"));
    TEST_ASSERT_EQUAL(0, find_text("A")->x);
    TEST_ASSERT_EQUAL(40, find_text("B")->x);
    TEST_ASSERT_EQUAL(7, find_text("A")->y);
    TEST_ASSERT_EQUAL(7, find_text("B")->y);
    TEST_ASSERT_EQUAL(16, lot52_node("p")->h);
    TEST_ASSERT_EQUAL(16, lot52_node("c")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l60_baseline_flex_margin_bottom_plain(void) {
    lot52_render("<body style='margin:0'><div id=p style='display:flex;width:120px;align-items:baseline'><div id=a style='width:"
        "40px;height:10px;padding-top:0px;padding-bottom:0px;border:0px solid red;margin-top:0px;margin-bottom:8px;flex"
        "-grow:0'>A</div><div id=b style='width:40px;height:10px;padding-top:0px;padding-bottom:0px;border:0px solid bl"
        "ue;margin-top:0px;margin-bottom:0px;flex-grow:0'>B</div></div><div id=c style='height:5px'></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(40, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->h);
    TEST_ASSERT_NOT_NULL(find_text("A"));
    TEST_ASSERT_NOT_NULL(find_text("B"));
    TEST_ASSERT_EQUAL(0, find_text("A")->x);
    TEST_ASSERT_EQUAL(40, find_text("B")->x);
    TEST_ASSERT_EQUAL(1, find_text("A")->y);
    TEST_ASSERT_EQUAL(1, find_text("B")->y);
    TEST_ASSERT_EQUAL(18, lot52_node("p")->h);
    TEST_ASSERT_EQUAL(18, lot52_node("c")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l60_baseline_flex_tall_plain(void) {
    lot52_render("<body style='margin:0'><div id=p style='display:flex;width:120px;align-items:baseline'><div id=a style='width:"
        "40px;height:30px;padding-top:0px;padding-bottom:0px;border:0px solid red;margin-top:0px;margin-bottom:0px;flex"
        "-grow:0'>A</div><div id=b style='width:40px;height:10px;padding-top:0px;padding-bottom:0px;border:0px solid bl"
        "ue;margin-top:0px;margin-bottom:0px;flex-grow:0'>B</div></div><div id=c style='height:5px'></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(40, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->h);
    TEST_ASSERT_NOT_NULL(find_text("A"));
    TEST_ASSERT_NOT_NULL(find_text("B"));
    TEST_ASSERT_EQUAL(0, find_text("A")->x);
    TEST_ASSERT_EQUAL(40, find_text("B")->x);
    TEST_ASSERT_EQUAL(1, find_text("A")->y);
    TEST_ASSERT_EQUAL(1, find_text("B")->y);
    TEST_ASSERT_EQUAL(30, lot52_node("p")->h);
    TEST_ASSERT_EQUAL(30, lot52_node("c")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l60_baseline_flex_tall_pad_top(void) {
    lot52_render("<body style='margin:0'><div id=p style='display:flex;width:120px;align-items:baseline'><div id=a style='width:"
        "40px;height:30px;padding-top:0px;padding-bottom:0px;border:0px solid red;margin-top:0px;margin-bottom:0px;flex"
        "-grow:0'>A</div><div id=b style='width:40px;height:10px;padding-top:5px;padding-bottom:0px;border:0px solid bl"
        "ue;margin-top:0px;margin-bottom:0px;flex-grow:0'>B</div></div><div id=c style='height:5px'></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(5, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(40, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(15, lot52_node("b")->h);
    TEST_ASSERT_NOT_NULL(find_text("A"));
    TEST_ASSERT_NOT_NULL(find_text("B"));
    TEST_ASSERT_EQUAL(0, find_text("A")->x);
    TEST_ASSERT_EQUAL(40, find_text("B")->x);
    TEST_ASSERT_EQUAL(6, find_text("A")->y);
    TEST_ASSERT_EQUAL(6, find_text("B")->y);
    TEST_ASSERT_EQUAL(35, lot52_node("p")->h);
    TEST_ASSERT_EQUAL(35, lot52_node("c")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l60_baseline_flex_tall_pad_bottom(void) {
    lot52_render("<body style='margin:0'><div id=p style='display:flex;width:120px;align-items:baseline'><div id=a style='width:"
        "40px;height:30px;padding-top:0px;padding-bottom:0px;border:0px solid red;margin-top:0px;margin-bottom:0px;flex"
        "-grow:0'>A</div><div id=b style='width:40px;height:10px;padding-top:0px;padding-bottom:7px;border:0px solid bl"
        "ue;margin-top:0px;margin-bottom:0px;flex-grow:0'>B</div></div><div id=c style='height:5px'></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(40, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(17, lot52_node("b")->h);
    TEST_ASSERT_NOT_NULL(find_text("A"));
    TEST_ASSERT_NOT_NULL(find_text("B"));
    TEST_ASSERT_EQUAL(0, find_text("A")->x);
    TEST_ASSERT_EQUAL(40, find_text("B")->x);
    TEST_ASSERT_EQUAL(1, find_text("A")->y);
    TEST_ASSERT_EQUAL(1, find_text("B")->y);
    TEST_ASSERT_EQUAL(30, lot52_node("p")->h);
    TEST_ASSERT_EQUAL(30, lot52_node("c")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l60_baseline_flex_tall_border(void) {
    lot52_render("<body style='margin:0'><div id=p style='display:flex;width:120px;align-items:baseline'><div id=a style='width:"
        "40px;height:30px;padding-top:0px;padding-bottom:0px;border:0px solid red;margin-top:0px;margin-bottom:0px;flex"
        "-grow:0'>A</div><div id=b style='width:40px;height:10px;padding-top:0px;padding-bottom:0px;border:2px solid bl"
        "ue;margin-top:0px;margin-bottom:0px;flex-grow:0'>B</div></div><div id=c style='height:5px'></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(2, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(40, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(44, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(14, lot52_node("b")->h);
    TEST_ASSERT_NOT_NULL(find_text("A"));
    TEST_ASSERT_NOT_NULL(find_text("B"));
    TEST_ASSERT_EQUAL(0, find_text("A")->x);
    TEST_ASSERT_EQUAL(42, find_text("B")->x);
    TEST_ASSERT_EQUAL(3, find_text("A")->y);
    TEST_ASSERT_EQUAL(3, find_text("B")->y);
    TEST_ASSERT_EQUAL(32, lot52_node("p")->h);
    TEST_ASSERT_EQUAL(32, lot52_node("c")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l60_baseline_flex_tall_margin_top(void) {
    lot52_render("<body style='margin:0'><div id=p style='display:flex;width:120px;align-items:baseline'><div id=a style='width:"
        "40px;height:30px;padding-top:0px;padding-bottom:0px;border:0px solid red;margin-top:0px;margin-bottom:0px;flex"
        "-grow:0'>A</div><div id=b style='width:40px;height:10px;padding-top:0px;padding-bottom:0px;border:0px solid bl"
        "ue;margin-top:6px;margin-bottom:0px;flex-grow:0'>B</div></div><div id=c style='height:5px'></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(6, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(40, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(6, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->h);
    TEST_ASSERT_NOT_NULL(find_text("A"));
    TEST_ASSERT_NOT_NULL(find_text("B"));
    TEST_ASSERT_EQUAL(0, find_text("A")->x);
    TEST_ASSERT_EQUAL(40, find_text("B")->x);
    TEST_ASSERT_EQUAL(7, find_text("A")->y);
    TEST_ASSERT_EQUAL(7, find_text("B")->y);
    TEST_ASSERT_EQUAL(36, lot52_node("p")->h);
    TEST_ASSERT_EQUAL(36, lot52_node("c")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l60_baseline_flex_tall_margin_bottom(void) {
    lot52_render("<body style='margin:0'><div id=p style='display:flex;width:120px;align-items:baseline'><div id=a style='width:"
        "40px;height:30px;padding-top:0px;padding-bottom:0px;border:0px solid red;margin-top:0px;margin-bottom:0px;flex"
        "-grow:0'>A</div><div id=b style='width:40px;height:10px;padding-top:0px;padding-bottom:0px;border:0px solid bl"
        "ue;margin-top:0px;margin-bottom:8px;flex-grow:0'>B</div></div><div id=c style='height:5px'></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(40, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->h);
    TEST_ASSERT_NOT_NULL(find_text("A"));
    TEST_ASSERT_NOT_NULL(find_text("B"));
    TEST_ASSERT_EQUAL(0, find_text("A")->x);
    TEST_ASSERT_EQUAL(40, find_text("B")->x);
    TEST_ASSERT_EQUAL(1, find_text("A")->y);
    TEST_ASSERT_EQUAL(1, find_text("B")->y);
    TEST_ASSERT_EQUAL(30, lot52_node("p")->h);
    TEST_ASSERT_EQUAL(30, lot52_node("c")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l60_baseline_flex_tall_tall(void) {
    lot52_render("<body style='margin:0'><div id=p style='display:flex;width:120px;align-items:baseline'><div id=a style='width:"
        "40px;height:30px;padding-top:0px;padding-bottom:0px;border:0px solid red;margin-top:0px;margin-bottom:0px;flex"
        "-grow:0'>A</div><div id=b style='width:40px;height:30px;padding-top:0px;padding-bottom:0px;border:0px solid bl"
        "ue;margin-top:0px;margin-bottom:0px;flex-grow:0'>B</div></div><div id=c style='height:5px'></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(40, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(30, lot52_node("b")->h);
    TEST_ASSERT_NOT_NULL(find_text("A"));
    TEST_ASSERT_NOT_NULL(find_text("B"));
    TEST_ASSERT_EQUAL(0, find_text("A")->x);
    TEST_ASSERT_EQUAL(40, find_text("B")->x);
    TEST_ASSERT_EQUAL(1, find_text("A")->y);
    TEST_ASSERT_EQUAL(1, find_text("B")->y);
    TEST_ASSERT_EQUAL(30, lot52_node("p")->h);
    TEST_ASSERT_EQUAL(30, lot52_node("c")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l60_baseline_flex_tall_all_edges(void) {
    lot52_render("<body style='margin:0'><div id=p style='display:flex;width:120px;align-items:baseline'><div id=a style='width:"
        "40px;height:30px;padding-top:0px;padding-bottom:0px;border:0px solid red;margin-top:0px;margin-bottom:0px;flex"
        "-grow:0'>A</div><div id=b style='width:40px;height:20px;padding-top:3px;padding-bottom:4px;border:1px solid bl"
        "ue;margin-top:2px;margin-bottom:5px;flex-grow:0'>B</div></div><div id=c style='height:5px'></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(6, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(40, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(2, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(42, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(29, lot52_node("b")->h);
    TEST_ASSERT_NOT_NULL(find_text("A"));
    TEST_ASSERT_NOT_NULL(find_text("B"));
    TEST_ASSERT_EQUAL(0, find_text("A")->x);
    TEST_ASSERT_EQUAL(41, find_text("B")->x);
    TEST_ASSERT_EQUAL(7, find_text("A")->y);
    TEST_ASSERT_EQUAL(7, find_text("B")->y);
    TEST_ASSERT_EQUAL(36, lot52_node("p")->h);
    TEST_ASSERT_EQUAL(36, lot52_node("c")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l60_baseline_flex_tall_short(void) {
    lot52_render("<body style='margin:0'><div id=p style='display:flex;width:120px;align-items:baseline'><div id=a style='width:"
        "40px;height:30px;padding-top:0px;padding-bottom:0px;border:0px solid red;margin-top:0px;margin-bottom:0px;flex"
        "-grow:0'>A</div><div id=b style='width:40px;height:2px;padding-top:0px;padding-bottom:0px;border:0px solid blu"
        "e;margin-top:0px;margin-bottom:0px;flex-grow:0'>B</div></div><div id=c style='height:5px'></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(40, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(2, lot52_node("b")->h);
    TEST_ASSERT_NOT_NULL(find_text("A"));
    TEST_ASSERT_NOT_NULL(find_text("B"));
    TEST_ASSERT_EQUAL(0, find_text("A")->x);
    TEST_ASSERT_EQUAL(40, find_text("B")->x);
    TEST_ASSERT_EQUAL(1, find_text("A")->y);
    TEST_ASSERT_EQUAL(1, find_text("B")->y);
    TEST_ASSERT_EQUAL(30, lot52_node("p")->h);
    TEST_ASSERT_EQUAL(30, lot52_node("c")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l60_baseline_flex_all_edges_plain(void) {
    lot52_render("<body style='margin:0'><div id=p style='display:flex;width:120px;align-items:baseline'><div id=a style='width:"
        "40px;height:20px;padding-top:3px;padding-bottom:4px;border:1px solid red;margin-top:2px;margin-bottom:5px;flex"
        "-grow:0'>A</div><div id=b style='width:40px;height:10px;padding-top:0px;padding-bottom:0px;border:0px solid bl"
        "ue;margin-top:0px;margin-bottom:0px;flex-grow:0'>B</div></div><div id=c style='height:5px'></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(2, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(42, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(29, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(42, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(6, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->h);
    TEST_ASSERT_NOT_NULL(find_text("A"));
    TEST_ASSERT_NOT_NULL(find_text("B"));
    TEST_ASSERT_EQUAL(1, find_text("A")->x);
    TEST_ASSERT_EQUAL(42, find_text("B")->x);
    TEST_ASSERT_EQUAL(7, find_text("A")->y);
    TEST_ASSERT_EQUAL(7, find_text("B")->y);
    TEST_ASSERT_EQUAL(36, lot52_node("p")->h);
    TEST_ASSERT_EQUAL(36, lot52_node("c")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l60_baseline_flex_short_plain(void) {
    lot52_render("<body style='margin:0'><div id=p style='display:flex;width:120px;align-items:baseline'><div id=a style='width:"
        "40px;height:2px;padding-top:0px;padding-bottom:0px;border:0px solid red;margin-top:0px;margin-bottom:0px;flex-"
        "grow:0'>A</div><div id=b style='width:40px;height:10px;padding-top:0px;padding-bottom:0px;border:0px solid blu"
        "e;margin-top:0px;margin-bottom:0px;flex-grow:0'>B</div></div><div id=c style='height:5px'></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(2, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(40, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->h);
    TEST_ASSERT_NOT_NULL(find_text("A"));
    TEST_ASSERT_NOT_NULL(find_text("B"));
    TEST_ASSERT_EQUAL(0, find_text("A")->x);
    TEST_ASSERT_EQUAL(40, find_text("B")->x);
    TEST_ASSERT_EQUAL(1, find_text("A")->y);
    TEST_ASSERT_EQUAL(1, find_text("B")->y);
    TEST_ASSERT_EQUAL(10, lot52_node("p")->h);
    TEST_ASSERT_EQUAL(10, lot52_node("c")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l60_baseline_grid_plain_plain(void) {
    lot52_render("<body style='margin:0'><div id=p style='display:grid;width:120px;align-items:baseline;grid-template-columns:re"
        "peat(2,60px)'><div id=a style='width:40px;height:10px;padding-top:0px;padding-bottom:0px;border:0px solid red;"
        "margin-top:0px;margin-bottom:0px;flex-grow:0'>A</div><div id=b style='width:40px;height:10px;padding-top:0px;p"
        "adding-bottom:0px;border:0px solid blue;margin-top:0px;margin-bottom:0px;flex-grow:0'>B</div></div><div id=c s"
        "tyle='height:5px'></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(60, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->h);
    TEST_ASSERT_NOT_NULL(find_text("A"));
    TEST_ASSERT_NOT_NULL(find_text("B"));
    TEST_ASSERT_EQUAL(0, find_text("A")->x);
    TEST_ASSERT_EQUAL(60, find_text("B")->x);
    TEST_ASSERT_EQUAL(1, find_text("A")->y);
    TEST_ASSERT_EQUAL(1, find_text("B")->y);
    TEST_ASSERT_EQUAL(10, lot52_node("p")->h);
    TEST_ASSERT_EQUAL(10, lot52_node("c")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l60_baseline_grid_pad_top_plain(void) {
    lot52_render("<body style='margin:0'><div id=p style='display:grid;width:120px;align-items:baseline;grid-template-columns:re"
        "peat(2,60px)'><div id=a style='width:40px;height:10px;padding-top:5px;padding-bottom:0px;border:0px solid red;"
        "margin-top:0px;margin-bottom:0px;flex-grow:0'>A</div><div id=b style='width:40px;height:10px;padding-top:0px;p"
        "adding-bottom:0px;border:0px solid blue;margin-top:0px;margin-bottom:0px;flex-grow:0'>B</div></div><div id=c s"
        "tyle='height:5px'></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(15, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(60, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(5, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->h);
    TEST_ASSERT_NOT_NULL(find_text("A"));
    TEST_ASSERT_NOT_NULL(find_text("B"));
    TEST_ASSERT_EQUAL(0, find_text("A")->x);
    TEST_ASSERT_EQUAL(60, find_text("B")->x);
    TEST_ASSERT_EQUAL(6, find_text("A")->y);
    TEST_ASSERT_EQUAL(6, find_text("B")->y);
    TEST_ASSERT_EQUAL(15, lot52_node("p")->h);
    TEST_ASSERT_EQUAL(15, lot52_node("c")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l60_baseline_grid_pad_top_pad_top(void) {
    lot52_render("<body style='margin:0'><div id=p style='display:grid;width:120px;align-items:baseline;grid-template-columns:re"
        "peat(2,60px)'><div id=a style='width:40px;height:10px;padding-top:5px;padding-bottom:0px;border:0px solid red;"
        "margin-top:0px;margin-bottom:0px;flex-grow:0'>A</div><div id=b style='width:40px;height:10px;padding-top:5px;p"
        "adding-bottom:0px;border:0px solid blue;margin-top:0px;margin-bottom:0px;flex-grow:0'>B</div></div><div id=c s"
        "tyle='height:5px'></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(15, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(60, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(15, lot52_node("b")->h);
    TEST_ASSERT_NOT_NULL(find_text("A"));
    TEST_ASSERT_NOT_NULL(find_text("B"));
    TEST_ASSERT_EQUAL(0, find_text("A")->x);
    TEST_ASSERT_EQUAL(60, find_text("B")->x);
    TEST_ASSERT_EQUAL(6, find_text("A")->y);
    TEST_ASSERT_EQUAL(6, find_text("B")->y);
    TEST_ASSERT_EQUAL(15, lot52_node("p")->h);
    TEST_ASSERT_EQUAL(15, lot52_node("c")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l60_baseline_grid_pad_top_pad_bottom(void) {
    lot52_render("<body style='margin:0'><div id=p style='display:grid;width:120px;align-items:baseline;grid-template-columns:re"
        "peat(2,60px)'><div id=a style='width:40px;height:10px;padding-top:5px;padding-bottom:0px;border:0px solid red;"
        "margin-top:0px;margin-bottom:0px;flex-grow:0'>A</div><div id=b style='width:40px;height:10px;padding-top:0px;p"
        "adding-bottom:7px;border:0px solid blue;margin-top:0px;margin-bottom:0px;flex-grow:0'>B</div></div><div id=c s"
        "tyle='height:5px'></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(15, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(60, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(5, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(17, lot52_node("b")->h);
    TEST_ASSERT_NOT_NULL(find_text("A"));
    TEST_ASSERT_NOT_NULL(find_text("B"));
    TEST_ASSERT_EQUAL(0, find_text("A")->x);
    TEST_ASSERT_EQUAL(60, find_text("B")->x);
    TEST_ASSERT_EQUAL(6, find_text("A")->y);
    TEST_ASSERT_EQUAL(6, find_text("B")->y);
    TEST_ASSERT_EQUAL(22, lot52_node("p")->h);
    TEST_ASSERT_EQUAL(22, lot52_node("c")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l60_baseline_grid_pad_top_border(void) {
    lot52_render("<body style='margin:0'><div id=p style='display:grid;width:120px;align-items:baseline;grid-template-columns:re"
        "peat(2,60px)'><div id=a style='width:40px;height:10px;padding-top:5px;padding-bottom:0px;border:0px solid red;"
        "margin-top:0px;margin-bottom:0px;flex-grow:0'>A</div><div id=b style='width:40px;height:10px;padding-top:0px;p"
        "adding-bottom:0px;border:2px solid blue;margin-top:0px;margin-bottom:0px;flex-grow:0'>B</div></div><div id=c s"
        "tyle='height:5px'></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(15, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(60, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(3, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(44, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(14, lot52_node("b")->h);
    TEST_ASSERT_NOT_NULL(find_text("A"));
    TEST_ASSERT_NOT_NULL(find_text("B"));
    TEST_ASSERT_EQUAL(0, find_text("A")->x);
    TEST_ASSERT_EQUAL(62, find_text("B")->x);
    TEST_ASSERT_EQUAL(6, find_text("A")->y);
    TEST_ASSERT_EQUAL(6, find_text("B")->y);
    TEST_ASSERT_EQUAL(17, lot52_node("p")->h);
    TEST_ASSERT_EQUAL(17, lot52_node("c")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l60_baseline_grid_pad_top_margin_top(void) {
    lot52_render("<body style='margin:0'><div id=p style='display:grid;width:120px;align-items:baseline;grid-template-columns:re"
        "peat(2,60px)'><div id=a style='width:40px;height:10px;padding-top:5px;padding-bottom:0px;border:0px solid red;"
        "margin-top:0px;margin-bottom:0px;flex-grow:0'>A</div><div id=b style='width:40px;height:10px;padding-top:0px;p"
        "adding-bottom:0px;border:0px solid blue;margin-top:6px;margin-bottom:0px;flex-grow:0'>B</div></div><div id=c s"
        "tyle='height:5px'></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(1, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(15, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(60, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(6, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->h);
    TEST_ASSERT_NOT_NULL(find_text("A"));
    TEST_ASSERT_NOT_NULL(find_text("B"));
    TEST_ASSERT_EQUAL(0, find_text("A")->x);
    TEST_ASSERT_EQUAL(60, find_text("B")->x);
    TEST_ASSERT_EQUAL(7, find_text("A")->y);
    TEST_ASSERT_EQUAL(7, find_text("B")->y);
    TEST_ASSERT_EQUAL(16, lot52_node("p")->h);
    TEST_ASSERT_EQUAL(16, lot52_node("c")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l60_baseline_grid_pad_top_margin_bottom(void) {
    lot52_render("<body style='margin:0'><div id=p style='display:grid;width:120px;align-items:baseline;grid-template-columns:re"
        "peat(2,60px)'><div id=a style='width:40px;height:10px;padding-top:5px;padding-bottom:0px;border:0px solid red;"
        "margin-top:0px;margin-bottom:0px;flex-grow:0'>A</div><div id=b style='width:40px;height:10px;padding-top:0px;p"
        "adding-bottom:0px;border:0px solid blue;margin-top:0px;margin-bottom:8px;flex-grow:0'>B</div></div><div id=c s"
        "tyle='height:5px'></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(15, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(60, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(5, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->h);
    TEST_ASSERT_NOT_NULL(find_text("A"));
    TEST_ASSERT_NOT_NULL(find_text("B"));
    TEST_ASSERT_EQUAL(0, find_text("A")->x);
    TEST_ASSERT_EQUAL(60, find_text("B")->x);
    TEST_ASSERT_EQUAL(6, find_text("A")->y);
    TEST_ASSERT_EQUAL(6, find_text("B")->y);
    TEST_ASSERT_EQUAL(23, lot52_node("p")->h);
    TEST_ASSERT_EQUAL(23, lot52_node("c")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l60_baseline_grid_pad_top_tall(void) {
    lot52_render("<body style='margin:0'><div id=p style='display:grid;width:120px;align-items:baseline;grid-template-columns:re"
        "peat(2,60px)'><div id=a style='width:40px;height:10px;padding-top:5px;padding-bottom:0px;border:0px solid red;"
        "margin-top:0px;margin-bottom:0px;flex-grow:0'>A</div><div id=b style='width:40px;height:30px;padding-top:0px;p"
        "adding-bottom:0px;border:0px solid blue;margin-top:0px;margin-bottom:0px;flex-grow:0'>B</div></div><div id=c s"
        "tyle='height:5px'></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(15, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(60, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(5, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(30, lot52_node("b")->h);
    TEST_ASSERT_NOT_NULL(find_text("A"));
    TEST_ASSERT_NOT_NULL(find_text("B"));
    TEST_ASSERT_EQUAL(0, find_text("A")->x);
    TEST_ASSERT_EQUAL(60, find_text("B")->x);
    TEST_ASSERT_EQUAL(6, find_text("A")->y);
    TEST_ASSERT_EQUAL(6, find_text("B")->y);
    TEST_ASSERT_EQUAL(35, lot52_node("p")->h);
    TEST_ASSERT_EQUAL(35, lot52_node("c")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l60_baseline_grid_pad_top_all_edges(void) {
    lot52_render("<body style='margin:0'><div id=p style='display:grid;width:120px;align-items:baseline;grid-template-columns:re"
        "peat(2,60px)'><div id=a style='width:40px;height:10px;padding-top:5px;padding-bottom:0px;border:0px solid red;"
        "margin-top:0px;margin-bottom:0px;flex-grow:0'>A</div><div id=b style='width:40px;height:20px;padding-top:3px;p"
        "adding-bottom:4px;border:1px solid blue;margin-top:2px;margin-bottom:5px;flex-grow:0'>B</div></div><div id=c s"
        "tyle='height:5px'></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(1, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(15, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(60, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(2, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(42, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(29, lot52_node("b")->h);
    TEST_ASSERT_NOT_NULL(find_text("A"));
    TEST_ASSERT_NOT_NULL(find_text("B"));
    TEST_ASSERT_EQUAL(0, find_text("A")->x);
    TEST_ASSERT_EQUAL(61, find_text("B")->x);
    TEST_ASSERT_EQUAL(7, find_text("A")->y);
    TEST_ASSERT_EQUAL(7, find_text("B")->y);
    TEST_ASSERT_EQUAL(36, lot52_node("p")->h);
    TEST_ASSERT_EQUAL(36, lot52_node("c")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l60_baseline_grid_pad_top_short(void) {
    lot52_render("<body style='margin:0'><div id=p style='display:grid;width:120px;align-items:baseline;grid-template-columns:re"
        "peat(2,60px)'><div id=a style='width:40px;height:10px;padding-top:5px;padding-bottom:0px;border:0px solid red;"
        "margin-top:0px;margin-bottom:0px;flex-grow:0'>A</div><div id=b style='width:40px;height:2px;padding-top:0px;pa"
        "dding-bottom:0px;border:0px solid blue;margin-top:0px;margin-bottom:0px;flex-grow:0'>B</div></div><div id=c st"
        "yle='height:5px'></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(15, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(60, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(5, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(2, lot52_node("b")->h);
    TEST_ASSERT_NOT_NULL(find_text("A"));
    TEST_ASSERT_NOT_NULL(find_text("B"));
    TEST_ASSERT_EQUAL(0, find_text("A")->x);
    TEST_ASSERT_EQUAL(60, find_text("B")->x);
    TEST_ASSERT_EQUAL(6, find_text("A")->y);
    TEST_ASSERT_EQUAL(6, find_text("B")->y);
    TEST_ASSERT_EQUAL(15, lot52_node("p")->h);
    TEST_ASSERT_EQUAL(15, lot52_node("c")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l60_baseline_grid_pad_bottom_plain(void) {
    lot52_render("<body style='margin:0'><div id=p style='display:grid;width:120px;align-items:baseline;grid-template-columns:re"
        "peat(2,60px)'><div id=a style='width:40px;height:10px;padding-top:0px;padding-bottom:7px;border:0px solid red;"
        "margin-top:0px;margin-bottom:0px;flex-grow:0'>A</div><div id=b style='width:40px;height:10px;padding-top:0px;p"
        "adding-bottom:0px;border:0px solid blue;margin-top:0px;margin-bottom:0px;flex-grow:0'>B</div></div><div id=c s"
        "tyle='height:5px'></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(17, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(60, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->h);
    TEST_ASSERT_NOT_NULL(find_text("A"));
    TEST_ASSERT_NOT_NULL(find_text("B"));
    TEST_ASSERT_EQUAL(0, find_text("A")->x);
    TEST_ASSERT_EQUAL(60, find_text("B")->x);
    TEST_ASSERT_EQUAL(1, find_text("A")->y);
    TEST_ASSERT_EQUAL(1, find_text("B")->y);
    TEST_ASSERT_EQUAL(17, lot52_node("p")->h);
    TEST_ASSERT_EQUAL(17, lot52_node("c")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l60_baseline_grid_border_plain(void) {
    lot52_render("<body style='margin:0'><div id=p style='display:grid;width:120px;align-items:baseline;grid-template-columns:re"
        "peat(2,60px)'><div id=a style='width:40px;height:10px;padding-top:0px;padding-bottom:0px;border:2px solid red;"
        "margin-top:0px;margin-bottom:0px;flex-grow:0'>A</div><div id=b style='width:40px;height:10px;padding-top:0px;p"
        "adding-bottom:0px;border:0px solid blue;margin-top:0px;margin-bottom:0px;flex-grow:0'>B</div></div><div id=c s"
        "tyle='height:5px'></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(44, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(14, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(60, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(2, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->h);
    TEST_ASSERT_NOT_NULL(find_text("A"));
    TEST_ASSERT_NOT_NULL(find_text("B"));
    TEST_ASSERT_EQUAL(2, find_text("A")->x);
    TEST_ASSERT_EQUAL(60, find_text("B")->x);
    TEST_ASSERT_EQUAL(3, find_text("A")->y);
    TEST_ASSERT_EQUAL(3, find_text("B")->y);
    TEST_ASSERT_EQUAL(14, lot52_node("p")->h);
    TEST_ASSERT_EQUAL(14, lot52_node("c")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l60_baseline_grid_border_pad_top(void) {
    lot52_render("<body style='margin:0'><div id=p style='display:grid;width:120px;align-items:baseline;grid-template-columns:re"
        "peat(2,60px)'><div id=a style='width:40px;height:10px;padding-top:0px;padding-bottom:0px;border:2px solid red;"
        "margin-top:0px;margin-bottom:0px;flex-grow:0'>A</div><div id=b style='width:40px;height:10px;padding-top:5px;p"
        "adding-bottom:0px;border:0px solid blue;margin-top:0px;margin-bottom:0px;flex-grow:0'>B</div></div><div id=c s"
        "tyle='height:5px'></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(3, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(44, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(14, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(60, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(15, lot52_node("b")->h);
    TEST_ASSERT_NOT_NULL(find_text("A"));
    TEST_ASSERT_NOT_NULL(find_text("B"));
    TEST_ASSERT_EQUAL(2, find_text("A")->x);
    TEST_ASSERT_EQUAL(60, find_text("B")->x);
    TEST_ASSERT_EQUAL(6, find_text("A")->y);
    TEST_ASSERT_EQUAL(6, find_text("B")->y);
    TEST_ASSERT_EQUAL(17, lot52_node("p")->h);
    TEST_ASSERT_EQUAL(17, lot52_node("c")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l60_baseline_grid_border_pad_bottom(void) {
    lot52_render("<body style='margin:0'><div id=p style='display:grid;width:120px;align-items:baseline;grid-template-columns:re"
        "peat(2,60px)'><div id=a style='width:40px;height:10px;padding-top:0px;padding-bottom:0px;border:2px solid red;"
        "margin-top:0px;margin-bottom:0px;flex-grow:0'>A</div><div id=b style='width:40px;height:10px;padding-top:0px;p"
        "adding-bottom:7px;border:0px solid blue;margin-top:0px;margin-bottom:0px;flex-grow:0'>B</div></div><div id=c s"
        "tyle='height:5px'></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(44, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(14, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(60, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(2, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(17, lot52_node("b")->h);
    TEST_ASSERT_NOT_NULL(find_text("A"));
    TEST_ASSERT_NOT_NULL(find_text("B"));
    TEST_ASSERT_EQUAL(2, find_text("A")->x);
    TEST_ASSERT_EQUAL(60, find_text("B")->x);
    TEST_ASSERT_EQUAL(3, find_text("A")->y);
    TEST_ASSERT_EQUAL(3, find_text("B")->y);
    TEST_ASSERT_EQUAL(19, lot52_node("p")->h);
    TEST_ASSERT_EQUAL(19, lot52_node("c")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l60_baseline_grid_border_border(void) {
    lot52_render("<body style='margin:0'><div id=p style='display:grid;width:120px;align-items:baseline;grid-template-columns:re"
        "peat(2,60px)'><div id=a style='width:40px;height:10px;padding-top:0px;padding-bottom:0px;border:2px solid red;"
        "margin-top:0px;margin-bottom:0px;flex-grow:0'>A</div><div id=b style='width:40px;height:10px;padding-top:0px;p"
        "adding-bottom:0px;border:2px solid blue;margin-top:0px;margin-bottom:0px;flex-grow:0'>B</div></div><div id=c s"
        "tyle='height:5px'></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(44, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(14, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(60, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(44, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(14, lot52_node("b")->h);
    TEST_ASSERT_NOT_NULL(find_text("A"));
    TEST_ASSERT_NOT_NULL(find_text("B"));
    TEST_ASSERT_EQUAL(2, find_text("A")->x);
    TEST_ASSERT_EQUAL(62, find_text("B")->x);
    TEST_ASSERT_EQUAL(3, find_text("A")->y);
    TEST_ASSERT_EQUAL(3, find_text("B")->y);
    TEST_ASSERT_EQUAL(14, lot52_node("p")->h);
    TEST_ASSERT_EQUAL(14, lot52_node("c")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l60_baseline_grid_border_margin_top(void) {
    lot52_render("<body style='margin:0'><div id=p style='display:grid;width:120px;align-items:baseline;grid-template-columns:re"
        "peat(2,60px)'><div id=a style='width:40px;height:10px;padding-top:0px;padding-bottom:0px;border:2px solid red;"
        "margin-top:0px;margin-bottom:0px;flex-grow:0'>A</div><div id=b style='width:40px;height:10px;padding-top:0px;p"
        "adding-bottom:0px;border:0px solid blue;margin-top:6px;margin-bottom:0px;flex-grow:0'>B</div></div><div id=c s"
        "tyle='height:5px'></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(4, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(44, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(14, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(60, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(6, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->h);
    TEST_ASSERT_NOT_NULL(find_text("A"));
    TEST_ASSERT_NOT_NULL(find_text("B"));
    TEST_ASSERT_EQUAL(2, find_text("A")->x);
    TEST_ASSERT_EQUAL(60, find_text("B")->x);
    TEST_ASSERT_EQUAL(7, find_text("A")->y);
    TEST_ASSERT_EQUAL(7, find_text("B")->y);
    TEST_ASSERT_EQUAL(18, lot52_node("p")->h);
    TEST_ASSERT_EQUAL(18, lot52_node("c")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l60_baseline_grid_border_margin_bottom(void) {
    lot52_render("<body style='margin:0'><div id=p style='display:grid;width:120px;align-items:baseline;grid-template-columns:re"
        "peat(2,60px)'><div id=a style='width:40px;height:10px;padding-top:0px;padding-bottom:0px;border:2px solid red;"
        "margin-top:0px;margin-bottom:0px;flex-grow:0'>A</div><div id=b style='width:40px;height:10px;padding-top:0px;p"
        "adding-bottom:0px;border:0px solid blue;margin-top:0px;margin-bottom:8px;flex-grow:0'>B</div></div><div id=c s"
        "tyle='height:5px'></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(44, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(14, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(60, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(2, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->h);
    TEST_ASSERT_NOT_NULL(find_text("A"));
    TEST_ASSERT_NOT_NULL(find_text("B"));
    TEST_ASSERT_EQUAL(2, find_text("A")->x);
    TEST_ASSERT_EQUAL(60, find_text("B")->x);
    TEST_ASSERT_EQUAL(3, find_text("A")->y);
    TEST_ASSERT_EQUAL(3, find_text("B")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("p")->h);
    TEST_ASSERT_EQUAL(20, lot52_node("c")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l60_baseline_grid_border_tall(void) {
    lot52_render("<body style='margin:0'><div id=p style='display:grid;width:120px;align-items:baseline;grid-template-columns:re"
        "peat(2,60px)'><div id=a style='width:40px;height:10px;padding-top:0px;padding-bottom:0px;border:2px solid red;"
        "margin-top:0px;margin-bottom:0px;flex-grow:0'>A</div><div id=b style='width:40px;height:30px;padding-top:0px;p"
        "adding-bottom:0px;border:0px solid blue;margin-top:0px;margin-bottom:0px;flex-grow:0'>B</div></div><div id=c s"
        "tyle='height:5px'></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(44, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(14, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(60, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(2, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(30, lot52_node("b")->h);
    TEST_ASSERT_NOT_NULL(find_text("A"));
    TEST_ASSERT_NOT_NULL(find_text("B"));
    TEST_ASSERT_EQUAL(2, find_text("A")->x);
    TEST_ASSERT_EQUAL(60, find_text("B")->x);
    TEST_ASSERT_EQUAL(3, find_text("A")->y);
    TEST_ASSERT_EQUAL(3, find_text("B")->y);
    TEST_ASSERT_EQUAL(32, lot52_node("p")->h);
    TEST_ASSERT_EQUAL(32, lot52_node("c")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l60_baseline_grid_border_all_edges(void) {
    lot52_render("<body style='margin:0'><div id=p style='display:grid;width:120px;align-items:baseline;grid-template-columns:re"
        "peat(2,60px)'><div id=a style='width:40px;height:10px;padding-top:0px;padding-bottom:0px;border:2px solid red;"
        "margin-top:0px;margin-bottom:0px;flex-grow:0'>A</div><div id=b style='width:40px;height:20px;padding-top:3px;p"
        "adding-bottom:4px;border:1px solid blue;margin-top:2px;margin-bottom:5px;flex-grow:0'>B</div></div><div id=c s"
        "tyle='height:5px'></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(4, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(44, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(14, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(60, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(2, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(42, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(29, lot52_node("b")->h);
    TEST_ASSERT_NOT_NULL(find_text("A"));
    TEST_ASSERT_NOT_NULL(find_text("B"));
    TEST_ASSERT_EQUAL(2, find_text("A")->x);
    TEST_ASSERT_EQUAL(61, find_text("B")->x);
    TEST_ASSERT_EQUAL(7, find_text("A")->y);
    TEST_ASSERT_EQUAL(7, find_text("B")->y);
    TEST_ASSERT_EQUAL(36, lot52_node("p")->h);
    TEST_ASSERT_EQUAL(36, lot52_node("c")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l60_baseline_grid_border_short(void) {
    lot52_render("<body style='margin:0'><div id=p style='display:grid;width:120px;align-items:baseline;grid-template-columns:re"
        "peat(2,60px)'><div id=a style='width:40px;height:10px;padding-top:0px;padding-bottom:0px;border:2px solid red;"
        "margin-top:0px;margin-bottom:0px;flex-grow:0'>A</div><div id=b style='width:40px;height:2px;padding-top:0px;pa"
        "dding-bottom:0px;border:0px solid blue;margin-top:0px;margin-bottom:0px;flex-grow:0'>B</div></div><div id=c st"
        "yle='height:5px'></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(44, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(14, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(60, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(2, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(2, lot52_node("b")->h);
    TEST_ASSERT_NOT_NULL(find_text("A"));
    TEST_ASSERT_NOT_NULL(find_text("B"));
    TEST_ASSERT_EQUAL(2, find_text("A")->x);
    TEST_ASSERT_EQUAL(60, find_text("B")->x);
    TEST_ASSERT_EQUAL(3, find_text("A")->y);
    TEST_ASSERT_EQUAL(3, find_text("B")->y);
    TEST_ASSERT_EQUAL(14, lot52_node("p")->h);
    TEST_ASSERT_EQUAL(14, lot52_node("c")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l60_baseline_grid_margin_top_plain(void) {
    lot52_render("<body style='margin:0'><div id=p style='display:grid;width:120px;align-items:baseline;grid-template-columns:re"
        "peat(2,60px)'><div id=a style='width:40px;height:10px;padding-top:0px;padding-bottom:0px;border:0px solid red;"
        "margin-top:6px;margin-bottom:0px;flex-grow:0'>A</div><div id=b style='width:40px;height:10px;padding-top:0px;p"
        "adding-bottom:0px;border:0px solid blue;margin-top:0px;margin-bottom:0px;flex-grow:0'>B</div></div><div id=c s"
        "tyle='height:5px'></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(6, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(60, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(6, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->h);
    TEST_ASSERT_NOT_NULL(find_text("A"));
    TEST_ASSERT_NOT_NULL(find_text("B"));
    TEST_ASSERT_EQUAL(0, find_text("A")->x);
    TEST_ASSERT_EQUAL(60, find_text("B")->x);
    TEST_ASSERT_EQUAL(7, find_text("A")->y);
    TEST_ASSERT_EQUAL(7, find_text("B")->y);
    TEST_ASSERT_EQUAL(16, lot52_node("p")->h);
    TEST_ASSERT_EQUAL(16, lot52_node("c")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l60_baseline_grid_margin_bottom_plain(void) {
    lot52_render("<body style='margin:0'><div id=p style='display:grid;width:120px;align-items:baseline;grid-template-columns:re"
        "peat(2,60px)'><div id=a style='width:40px;height:10px;padding-top:0px;padding-bottom:0px;border:0px solid red;"
        "margin-top:0px;margin-bottom:8px;flex-grow:0'>A</div><div id=b style='width:40px;height:10px;padding-top:0px;p"
        "adding-bottom:0px;border:0px solid blue;margin-top:0px;margin-bottom:0px;flex-grow:0'>B</div></div><div id=c s"
        "tyle='height:5px'></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(60, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->h);
    TEST_ASSERT_NOT_NULL(find_text("A"));
    TEST_ASSERT_NOT_NULL(find_text("B"));
    TEST_ASSERT_EQUAL(0, find_text("A")->x);
    TEST_ASSERT_EQUAL(60, find_text("B")->x);
    TEST_ASSERT_EQUAL(1, find_text("A")->y);
    TEST_ASSERT_EQUAL(1, find_text("B")->y);
    TEST_ASSERT_EQUAL(18, lot52_node("p")->h);
    TEST_ASSERT_EQUAL(18, lot52_node("c")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l60_baseline_grid_tall_plain(void) {
    lot52_render("<body style='margin:0'><div id=p style='display:grid;width:120px;align-items:baseline;grid-template-columns:re"
        "peat(2,60px)'><div id=a style='width:40px;height:30px;padding-top:0px;padding-bottom:0px;border:0px solid red;"
        "margin-top:0px;margin-bottom:0px;flex-grow:0'>A</div><div id=b style='width:40px;height:10px;padding-top:0px;p"
        "adding-bottom:0px;border:0px solid blue;margin-top:0px;margin-bottom:0px;flex-grow:0'>B</div></div><div id=c s"
        "tyle='height:5px'></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(60, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->h);
    TEST_ASSERT_NOT_NULL(find_text("A"));
    TEST_ASSERT_NOT_NULL(find_text("B"));
    TEST_ASSERT_EQUAL(0, find_text("A")->x);
    TEST_ASSERT_EQUAL(60, find_text("B")->x);
    TEST_ASSERT_EQUAL(1, find_text("A")->y);
    TEST_ASSERT_EQUAL(1, find_text("B")->y);
    TEST_ASSERT_EQUAL(30, lot52_node("p")->h);
    TEST_ASSERT_EQUAL(30, lot52_node("c")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l60_baseline_grid_tall_pad_top(void) {
    lot52_render("<body style='margin:0'><div id=p style='display:grid;width:120px;align-items:baseline;grid-template-columns:re"
        "peat(2,60px)'><div id=a style='width:40px;height:30px;padding-top:0px;padding-bottom:0px;border:0px solid red;"
        "margin-top:0px;margin-bottom:0px;flex-grow:0'>A</div><div id=b style='width:40px;height:10px;padding-top:5px;p"
        "adding-bottom:0px;border:0px solid blue;margin-top:0px;margin-bottom:0px;flex-grow:0'>B</div></div><div id=c s"
        "tyle='height:5px'></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(5, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(60, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(15, lot52_node("b")->h);
    TEST_ASSERT_NOT_NULL(find_text("A"));
    TEST_ASSERT_NOT_NULL(find_text("B"));
    TEST_ASSERT_EQUAL(0, find_text("A")->x);
    TEST_ASSERT_EQUAL(60, find_text("B")->x);
    TEST_ASSERT_EQUAL(6, find_text("A")->y);
    TEST_ASSERT_EQUAL(6, find_text("B")->y);
    TEST_ASSERT_EQUAL(35, lot52_node("p")->h);
    TEST_ASSERT_EQUAL(35, lot52_node("c")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l60_baseline_grid_tall_pad_bottom(void) {
    lot52_render("<body style='margin:0'><div id=p style='display:grid;width:120px;align-items:baseline;grid-template-columns:re"
        "peat(2,60px)'><div id=a style='width:40px;height:30px;padding-top:0px;padding-bottom:0px;border:0px solid red;"
        "margin-top:0px;margin-bottom:0px;flex-grow:0'>A</div><div id=b style='width:40px;height:10px;padding-top:0px;p"
        "adding-bottom:7px;border:0px solid blue;margin-top:0px;margin-bottom:0px;flex-grow:0'>B</div></div><div id=c s"
        "tyle='height:5px'></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(60, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(17, lot52_node("b")->h);
    TEST_ASSERT_NOT_NULL(find_text("A"));
    TEST_ASSERT_NOT_NULL(find_text("B"));
    TEST_ASSERT_EQUAL(0, find_text("A")->x);
    TEST_ASSERT_EQUAL(60, find_text("B")->x);
    TEST_ASSERT_EQUAL(1, find_text("A")->y);
    TEST_ASSERT_EQUAL(1, find_text("B")->y);
    TEST_ASSERT_EQUAL(30, lot52_node("p")->h);
    TEST_ASSERT_EQUAL(30, lot52_node("c")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l60_baseline_grid_tall_border(void) {
    lot52_render("<body style='margin:0'><div id=p style='display:grid;width:120px;align-items:baseline;grid-template-columns:re"
        "peat(2,60px)'><div id=a style='width:40px;height:30px;padding-top:0px;padding-bottom:0px;border:0px solid red;"
        "margin-top:0px;margin-bottom:0px;flex-grow:0'>A</div><div id=b style='width:40px;height:10px;padding-top:0px;p"
        "adding-bottom:0px;border:2px solid blue;margin-top:0px;margin-bottom:0px;flex-grow:0'>B</div></div><div id=c s"
        "tyle='height:5px'></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(2, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(60, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(44, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(14, lot52_node("b")->h);
    TEST_ASSERT_NOT_NULL(find_text("A"));
    TEST_ASSERT_NOT_NULL(find_text("B"));
    TEST_ASSERT_EQUAL(0, find_text("A")->x);
    TEST_ASSERT_EQUAL(62, find_text("B")->x);
    TEST_ASSERT_EQUAL(3, find_text("A")->y);
    TEST_ASSERT_EQUAL(3, find_text("B")->y);
    TEST_ASSERT_EQUAL(32, lot52_node("p")->h);
    TEST_ASSERT_EQUAL(32, lot52_node("c")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l60_baseline_grid_tall_margin_top(void) {
    lot52_render("<body style='margin:0'><div id=p style='display:grid;width:120px;align-items:baseline;grid-template-columns:re"
        "peat(2,60px)'><div id=a style='width:40px;height:30px;padding-top:0px;padding-bottom:0px;border:0px solid red;"
        "margin-top:0px;margin-bottom:0px;flex-grow:0'>A</div><div id=b style='width:40px;height:10px;padding-top:0px;p"
        "adding-bottom:0px;border:0px solid blue;margin-top:6px;margin-bottom:0px;flex-grow:0'>B</div></div><div id=c s"
        "tyle='height:5px'></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(6, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(60, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(6, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->h);
    TEST_ASSERT_NOT_NULL(find_text("A"));
    TEST_ASSERT_NOT_NULL(find_text("B"));
    TEST_ASSERT_EQUAL(0, find_text("A")->x);
    TEST_ASSERT_EQUAL(60, find_text("B")->x);
    TEST_ASSERT_EQUAL(7, find_text("A")->y);
    TEST_ASSERT_EQUAL(7, find_text("B")->y);
    TEST_ASSERT_EQUAL(36, lot52_node("p")->h);
    TEST_ASSERT_EQUAL(36, lot52_node("c")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l60_baseline_grid_tall_margin_bottom(void) {
    lot52_render("<body style='margin:0'><div id=p style='display:grid;width:120px;align-items:baseline;grid-template-columns:re"
        "peat(2,60px)'><div id=a style='width:40px;height:30px;padding-top:0px;padding-bottom:0px;border:0px solid red;"
        "margin-top:0px;margin-bottom:0px;flex-grow:0'>A</div><div id=b style='width:40px;height:10px;padding-top:0px;p"
        "adding-bottom:0px;border:0px solid blue;margin-top:0px;margin-bottom:8px;flex-grow:0'>B</div></div><div id=c s"
        "tyle='height:5px'></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(60, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->h);
    TEST_ASSERT_NOT_NULL(find_text("A"));
    TEST_ASSERT_NOT_NULL(find_text("B"));
    TEST_ASSERT_EQUAL(0, find_text("A")->x);
    TEST_ASSERT_EQUAL(60, find_text("B")->x);
    TEST_ASSERT_EQUAL(1, find_text("A")->y);
    TEST_ASSERT_EQUAL(1, find_text("B")->y);
    TEST_ASSERT_EQUAL(30, lot52_node("p")->h);
    TEST_ASSERT_EQUAL(30, lot52_node("c")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l60_baseline_grid_tall_tall(void) {
    lot52_render("<body style='margin:0'><div id=p style='display:grid;width:120px;align-items:baseline;grid-template-columns:re"
        "peat(2,60px)'><div id=a style='width:40px;height:30px;padding-top:0px;padding-bottom:0px;border:0px solid red;"
        "margin-top:0px;margin-bottom:0px;flex-grow:0'>A</div><div id=b style='width:40px;height:30px;padding-top:0px;p"
        "adding-bottom:0px;border:0px solid blue;margin-top:0px;margin-bottom:0px;flex-grow:0'>B</div></div><div id=c s"
        "tyle='height:5px'></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(60, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(30, lot52_node("b")->h);
    TEST_ASSERT_NOT_NULL(find_text("A"));
    TEST_ASSERT_NOT_NULL(find_text("B"));
    TEST_ASSERT_EQUAL(0, find_text("A")->x);
    TEST_ASSERT_EQUAL(60, find_text("B")->x);
    TEST_ASSERT_EQUAL(1, find_text("A")->y);
    TEST_ASSERT_EQUAL(1, find_text("B")->y);
    TEST_ASSERT_EQUAL(30, lot52_node("p")->h);
    TEST_ASSERT_EQUAL(30, lot52_node("c")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l60_baseline_grid_tall_all_edges(void) {
    lot52_render("<body style='margin:0'><div id=p style='display:grid;width:120px;align-items:baseline;grid-template-columns:re"
        "peat(2,60px)'><div id=a style='width:40px;height:30px;padding-top:0px;padding-bottom:0px;border:0px solid red;"
        "margin-top:0px;margin-bottom:0px;flex-grow:0'>A</div><div id=b style='width:40px;height:20px;padding-top:3px;p"
        "adding-bottom:4px;border:1px solid blue;margin-top:2px;margin-bottom:5px;flex-grow:0'>B</div></div><div id=c s"
        "tyle='height:5px'></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(6, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(60, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(2, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(42, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(29, lot52_node("b")->h);
    TEST_ASSERT_NOT_NULL(find_text("A"));
    TEST_ASSERT_NOT_NULL(find_text("B"));
    TEST_ASSERT_EQUAL(0, find_text("A")->x);
    TEST_ASSERT_EQUAL(61, find_text("B")->x);
    TEST_ASSERT_EQUAL(7, find_text("A")->y);
    TEST_ASSERT_EQUAL(7, find_text("B")->y);
    TEST_ASSERT_EQUAL(36, lot52_node("p")->h);
    TEST_ASSERT_EQUAL(36, lot52_node("c")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l60_baseline_grid_tall_short(void) {
    lot52_render("<body style='margin:0'><div id=p style='display:grid;width:120px;align-items:baseline;grid-template-columns:re"
        "peat(2,60px)'><div id=a style='width:40px;height:30px;padding-top:0px;padding-bottom:0px;border:0px solid red;"
        "margin-top:0px;margin-bottom:0px;flex-grow:0'>A</div><div id=b style='width:40px;height:2px;padding-top:0px;pa"
        "dding-bottom:0px;border:0px solid blue;margin-top:0px;margin-bottom:0px;flex-grow:0'>B</div></div><div id=c st"
        "yle='height:5px'></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(60, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(2, lot52_node("b")->h);
    TEST_ASSERT_NOT_NULL(find_text("A"));
    TEST_ASSERT_NOT_NULL(find_text("B"));
    TEST_ASSERT_EQUAL(0, find_text("A")->x);
    TEST_ASSERT_EQUAL(60, find_text("B")->x);
    TEST_ASSERT_EQUAL(1, find_text("A")->y);
    TEST_ASSERT_EQUAL(1, find_text("B")->y);
    TEST_ASSERT_EQUAL(30, lot52_node("p")->h);
    TEST_ASSERT_EQUAL(30, lot52_node("c")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l60_baseline_grid_all_edges_plain(void) {
    lot52_render("<body style='margin:0'><div id=p style='display:grid;width:120px;align-items:baseline;grid-template-columns:re"
        "peat(2,60px)'><div id=a style='width:40px;height:20px;padding-top:3px;padding-bottom:4px;border:1px solid red;"
        "margin-top:2px;margin-bottom:5px;flex-grow:0'>A</div><div id=b style='width:40px;height:10px;padding-top:0px;p"
        "adding-bottom:0px;border:0px solid blue;margin-top:0px;margin-bottom:0px;flex-grow:0'>B</div></div><div id=c s"
        "tyle='height:5px'></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(2, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(42, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(29, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(60, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(6, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->h);
    TEST_ASSERT_NOT_NULL(find_text("A"));
    TEST_ASSERT_NOT_NULL(find_text("B"));
    TEST_ASSERT_EQUAL(1, find_text("A")->x);
    TEST_ASSERT_EQUAL(60, find_text("B")->x);
    TEST_ASSERT_EQUAL(7, find_text("A")->y);
    TEST_ASSERT_EQUAL(7, find_text("B")->y);
    TEST_ASSERT_EQUAL(36, lot52_node("p")->h);
    TEST_ASSERT_EQUAL(36, lot52_node("c")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l60_baseline_grid_short_plain(void) {
    lot52_render("<body style='margin:0'><div id=p style='display:grid;width:120px;align-items:baseline;grid-template-columns:re"
        "peat(2,60px)'><div id=a style='width:40px;height:2px;padding-top:0px;padding-bottom:0px;border:0px solid red;m"
        "argin-top:0px;margin-bottom:0px;flex-grow:0'>A</div><div id=b style='width:40px;height:10px;padding-top:0px;pa"
        "dding-bottom:0px;border:0px solid blue;margin-top:0px;margin-bottom:0px;flex-grow:0'>B</div></div><div id=c st"
        "yle='height:5px'></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(2, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(60, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->h);
    TEST_ASSERT_NOT_NULL(find_text("A"));
    TEST_ASSERT_NOT_NULL(find_text("B"));
    TEST_ASSERT_EQUAL(0, find_text("A")->x);
    TEST_ASSERT_EQUAL(60, find_text("B")->x);
    TEST_ASSERT_EQUAL(1, find_text("A")->y);
    TEST_ASSERT_EQUAL(1, find_text("B")->y);
    TEST_ASSERT_EQUAL(10, lot52_node("p")->h);
    TEST_ASSERT_EQUAL(10, lot52_node("c")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}
