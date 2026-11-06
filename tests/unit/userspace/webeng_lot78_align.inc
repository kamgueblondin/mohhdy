/* Lot 78: justify/align-items and self in grid; flex/grid edge. */

static void test_l78_align_items_1(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:80px;height:40px;grid-template-columns:80px;grid-template-rows:40px;justify-items:end;align-items:start'><div id=a style='width:20px;height:10px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(60, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_align_items_2(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:80px;height:40px;grid-template-columns:80px;grid-template-rows:40px;justify-items:center;align-items:start'><div id=a style='width:20px;height:10px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_align_items_3(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:80px;height:40px;grid-template-columns:80px;grid-template-rows:40px;justify-items:start;align-items:end'><div id=a style='width:20px;height:10px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_align_items_4(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:80px;height:40px;grid-template-columns:80px;grid-template-rows:40px;justify-items:end;align-items:end'><div id=a style='width:20px;height:10px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(60, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_align_items_5(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:80px;height:40px;grid-template-columns:80px;grid-template-rows:40px;justify-items:center;align-items:end'><div id=a style='width:20px;height:10px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_align_items_6(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:80px;height:40px;grid-template-columns:80px;grid-template-rows:40px;justify-items:start;align-items:center'><div id=a style='width:20px;height:10px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(15, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_align_items_7(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:80px;height:40px;grid-template-columns:80px;grid-template-rows:40px;justify-items:end;align-items:center'><div id=a style='width:20px;height:10px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(60, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(15, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_align_items_8(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:80px;height:40px;grid-template-columns:80px;grid-template-rows:40px;justify-items:center;align-items:center'><div id=a style='width:20px;height:10px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(15, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_align_items_9(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:80px;height:40px;grid-template-columns:80px;grid-template-rows:40px;justify-items:start;align-items:start'><div id=a style='width:20px;height:10px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_align_items_10(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:80px;height:40px;grid-template-columns:80px;grid-template-rows:40px;justify-items:end;align-items:start'><div id=a style='width:20px;height:10px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(60, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_align_items_11(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:80px;height:40px;grid-template-columns:80px;grid-template-rows:40px;justify-items:center;align-items:start'><div id=a style='width:20px;height:10px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_align_items_12(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:80px;height:40px;grid-template-columns:80px;grid-template-rows:40px;justify-items:start;align-items:end'><div id=a style='width:20px;height:10px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_align_items_13(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:80px;height:40px;grid-template-columns:80px;grid-template-rows:40px;justify-items:end;align-items:end'><div id=a style='width:20px;height:10px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(60, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_align_items_14(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:80px;height:40px;grid-template-columns:80px;grid-template-rows:40px;justify-items:center;align-items:end'><div id=a style='width:20px;height:10px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_align_items_15(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:80px;height:40px;grid-template-columns:80px;grid-template-rows:40px;justify-items:start;align-items:center'><div id=a style='width:20px;height:10px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(15, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_align_items_16(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:80px;height:40px;grid-template-columns:80px;grid-template-rows:40px;justify-items:end;align-items:center'><div id=a style='width:20px;height:10px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(60, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(15, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_align_items_17(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:80px;height:40px;grid-template-columns:80px;grid-template-rows:40px;justify-items:center;align-items:center'><div id=a style='width:20px;height:10px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(15, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_align_items_18(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:80px;height:40px;grid-template-columns:80px;grid-template-rows:40px;justify-items:start;align-items:start'><div id=a style='width:20px;height:10px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_align_items_19(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:80px;height:40px;grid-template-columns:80px;grid-template-rows:40px;justify-items:end;align-items:start'><div id=a style='width:20px;height:10px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(60, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_align_items_20(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:80px;height:40px;grid-template-columns:80px;grid-template-rows:40px;justify-items:center;align-items:start'><div id=a style='width:20px;height:10px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_align_items_21(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:80px;height:40px;grid-template-columns:80px;grid-template-rows:40px;justify-items:start;align-items:end'><div id=a style='width:20px;height:10px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_align_items_22(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:80px;height:40px;grid-template-columns:80px;grid-template-rows:40px;justify-items:end;align-items:end'><div id=a style='width:20px;height:10px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(60, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_align_items_23(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:80px;height:40px;grid-template-columns:80px;grid-template-rows:40px;justify-items:center;align-items:end'><div id=a style='width:20px;height:10px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_align_items_24(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:80px;height:40px;grid-template-columns:80px;grid-template-rows:40px;justify-items:start;align-items:center'><div id=a style='width:20px;height:10px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(15, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_align_items_25(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:80px;height:40px;grid-template-columns:80px;grid-template-rows:40px;justify-items:end;align-items:center'><div id=a style='width:20px;height:10px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(60, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(15, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_align_items_26(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:80px;height:40px;grid-template-columns:80px;grid-template-rows:40px;justify-items:center;align-items:center'><div id=a style='width:20px;height:10px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(15, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_align_items_27(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:80px;height:40px;grid-template-columns:80px;grid-template-rows:40px;justify-items:start;align-items:start'><div id=a style='width:20px;height:10px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_align_items_28(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:80px;height:40px;grid-template-columns:80px;grid-template-rows:40px;justify-items:end;align-items:start'><div id=a style='width:20px;height:10px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(60, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_align_items_29(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:80px;height:40px;grid-template-columns:80px;grid-template-rows:40px;justify-items:center;align-items:start'><div id=a style='width:20px;height:10px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_align_items_30(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:80px;height:40px;grid-template-columns:80px;grid-template-rows:40px;justify-items:start;align-items:end'><div id=a style='width:20px;height:10px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_align_items_31(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:80px;height:40px;grid-template-columns:80px;grid-template-rows:40px;justify-items:end;align-items:end'><div id=a style='width:20px;height:10px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(60, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_align_items_32(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:80px;height:40px;grid-template-columns:80px;grid-template-rows:40px;justify-items:center;align-items:end'><div id=a style='width:20px;height:10px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_align_items_33(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:80px;height:40px;grid-template-columns:80px;grid-template-rows:40px;justify-items:start;align-items:center'><div id=a style='width:20px;height:10px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(15, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_align_items_34(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:80px;height:40px;grid-template-columns:80px;grid-template-rows:40px;justify-items:end;align-items:center'><div id=a style='width:20px;height:10px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(60, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(15, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_align_items_35(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:80px;height:40px;grid-template-columns:80px;grid-template-rows:40px;justify-items:center;align-items:center'><div id=a style='width:20px;height:10px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(15, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_align_items_36(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:80px;height:40px;grid-template-columns:80px;grid-template-rows:40px;justify-items:start;align-items:start'><div id=a style='width:20px;height:10px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_align_items_37(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:80px;height:40px;grid-template-columns:80px;grid-template-rows:40px;justify-items:end;align-items:start'><div id=a style='width:20px;height:10px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(60, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_align_items_38(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:80px;height:40px;grid-template-columns:80px;grid-template-rows:40px;justify-items:center;align-items:start'><div id=a style='width:20px;height:10px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_align_items_39(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:80px;height:40px;grid-template-columns:80px;grid-template-rows:40px;justify-items:start;align-items:end'><div id=a style='width:20px;height:10px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_align_items_40(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:80px;height:40px;grid-template-columns:80px;grid-template-rows:40px;justify-items:end;align-items:end'><div id=a style='width:20px;height:10px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(60, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_align_items_41(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:80px;height:40px;grid-template-columns:80px;grid-template-rows:40px;justify-items:center;align-items:end'><div id=a style='width:20px;height:10px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_align_items_42(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:80px;height:40px;grid-template-columns:80px;grid-template-rows:40px;justify-items:start;align-items:center'><div id=a style='width:20px;height:10px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(15, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_align_items_43(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:80px;height:40px;grid-template-columns:80px;grid-template-rows:40px;justify-items:end;align-items:center'><div id=a style='width:20px;height:10px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(60, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(15, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_align_items_44(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:80px;height:40px;grid-template-columns:80px;grid-template-rows:40px;justify-items:center;align-items:center'><div id=a style='width:20px;height:10px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(15, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_align_items_45(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:80px;height:40px;grid-template-columns:80px;grid-template-rows:40px;justify-items:start;align-items:start'><div id=a style='width:20px;height:10px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_align_items_46(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:80px;height:40px;grid-template-columns:80px;grid-template-rows:40px;justify-items:end;align-items:start'><div id=a style='width:20px;height:10px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(60, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_align_items_47(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:80px;height:40px;grid-template-columns:80px;grid-template-rows:40px;justify-items:center;align-items:start'><div id=a style='width:20px;height:10px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_align_items_48(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:80px;height:40px;grid-template-columns:80px;grid-template-rows:40px;justify-items:start;align-items:end'><div id=a style='width:20px;height:10px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_align_items_49(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:80px;height:40px;grid-template-columns:80px;grid-template-rows:40px;justify-items:end;align-items:end'><div id=a style='width:20px;height:10px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(60, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_align_items_50(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:80px;height:40px;grid-template-columns:80px;grid-template-rows:40px;justify-items:center;align-items:end'><div id=a style='width:20px;height:10px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_align_items_51(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:80px;height:40px;grid-template-columns:80px;grid-template-rows:40px;justify-items:start;align-items:center'><div id=a style='width:20px;height:10px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(15, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_align_items_52(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:80px;height:40px;grid-template-columns:80px;grid-template-rows:40px;justify-items:end;align-items:center'><div id=a style='width:20px;height:10px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(60, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(15, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_align_items_53(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:80px;height:40px;grid-template-columns:80px;grid-template-rows:40px;justify-items:center;align-items:center'><div id=a style='width:20px;height:10px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(15, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_align_items_54(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:80px;height:40px;grid-template-columns:80px;grid-template-rows:40px;justify-items:start;align-items:start'><div id=a style='width:20px;height:10px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_align_items_55(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:80px;height:40px;grid-template-columns:80px;grid-template-rows:40px;justify-items:end;align-items:start'><div id=a style='width:20px;height:10px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(60, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_align_items_56(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:80px;height:40px;grid-template-columns:80px;grid-template-rows:40px;justify-items:center;align-items:start'><div id=a style='width:20px;height:10px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_align_items_57(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:80px;height:40px;grid-template-columns:80px;grid-template-rows:40px;justify-items:start;align-items:end'><div id=a style='width:20px;height:10px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_align_items_58(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:80px;height:40px;grid-template-columns:80px;grid-template-rows:40px;justify-items:end;align-items:end'><div id=a style='width:20px;height:10px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(60, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_align_items_59(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:80px;height:40px;grid-template-columns:80px;grid-template-rows:40px;justify-items:center;align-items:end'><div id=a style='width:20px;height:10px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_align_items_60(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:80px;height:40px;grid-template-columns:80px;grid-template-rows:40px;justify-items:start;align-items:center'><div id=a style='width:20px;height:10px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(15, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_align_items_61(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:80px;height:40px;grid-template-columns:80px;grid-template-rows:40px;justify-items:end;align-items:center'><div id=a style='width:20px;height:10px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(60, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(15, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_align_items_62(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:80px;height:40px;grid-template-columns:80px;grid-template-rows:40px;justify-items:center;align-items:center'><div id=a style='width:20px;height:10px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(15, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_align_items_63(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:80px;height:40px;grid-template-columns:80px;grid-template-rows:40px;justify-items:start;align-items:start'><div id=a style='width:20px;height:10px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_align_items_64(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:80px;height:40px;grid-template-columns:80px;grid-template-rows:40px;justify-items:end;align-items:start'><div id=a style='width:20px;height:10px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(60, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_align_items_65(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:80px;height:40px;grid-template-columns:80px;grid-template-rows:40px;justify-items:center;align-items:start'><div id=a style='width:20px;height:10px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_align_items_66(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:80px;height:40px;grid-template-columns:80px;grid-template-rows:40px;justify-items:start;align-items:end'><div id=a style='width:20px;height:10px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_align_items_67(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:80px;height:40px;grid-template-columns:80px;grid-template-rows:40px;justify-items:end;align-items:end'><div id=a style='width:20px;height:10px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(60, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_align_items_68(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:80px;height:40px;grid-template-columns:80px;grid-template-rows:40px;justify-items:center;align-items:end'><div id=a style='width:20px;height:10px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_align_items_69(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:80px;height:40px;grid-template-columns:80px;grid-template-rows:40px;justify-items:start;align-items:center'><div id=a style='width:20px;height:10px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(15, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_align_items_70(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:80px;height:40px;grid-template-columns:80px;grid-template-rows:40px;justify-items:end;align-items:center'><div id=a style='width:20px;height:10px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(60, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(15, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_align_items_71(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:80px;height:40px;grid-template-columns:80px;grid-template-rows:40px;justify-items:center;align-items:center'><div id=a style='width:20px;height:10px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(15, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_align_items_72(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:80px;height:40px;grid-template-columns:80px;grid-template-rows:40px;justify-items:start;align-items:start'><div id=a style='width:20px;height:10px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_align_items_73(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:80px;height:40px;grid-template-columns:80px;grid-template-rows:40px;justify-items:end;align-items:start'><div id=a style='width:20px;height:10px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(60, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_align_items_74(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:80px;height:40px;grid-template-columns:80px;grid-template-rows:40px;justify-items:center;align-items:start'><div id=a style='width:20px;height:10px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_align_items_75(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:80px;height:40px;grid-template-columns:80px;grid-template-rows:40px;justify-items:start;align-items:end'><div id=a style='width:20px;height:10px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_align_items_76(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:80px;height:40px;grid-template-columns:80px;grid-template-rows:40px;justify-items:end;align-items:end'><div id=a style='width:20px;height:10px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(60, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_align_items_77(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:80px;height:40px;grid-template-columns:80px;grid-template-rows:40px;justify-items:center;align-items:end'><div id=a style='width:20px;height:10px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_align_items_78(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:80px;height:40px;grid-template-columns:80px;grid-template-rows:40px;justify-items:start;align-items:center'><div id=a style='width:20px;height:10px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(15, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_align_items_79(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:80px;height:40px;grid-template-columns:80px;grid-template-rows:40px;justify-items:end;align-items:center'><div id=a style='width:20px;height:10px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(60, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(15, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_align_items_80(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:80px;height:40px;grid-template-columns:80px;grid-template-rows:40px;justify-items:center;align-items:center'><div id=a style='width:20px;height:10px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(15, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_align_items_81(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:80px;height:40px;grid-template-columns:80px;grid-template-rows:40px;justify-items:start;align-items:start'><div id=a style='width:20px;height:10px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_align_items_82(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:80px;height:40px;grid-template-columns:80px;grid-template-rows:40px;justify-items:end;align-items:start'><div id=a style='width:20px;height:10px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(60, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_align_items_83(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:80px;height:40px;grid-template-columns:80px;grid-template-rows:40px;justify-items:center;align-items:start'><div id=a style='width:20px;height:10px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_align_items_84(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:80px;height:40px;grid-template-columns:80px;grid-template-rows:40px;justify-items:start;align-items:end'><div id=a style='width:20px;height:10px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_align_items_85(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:80px;height:40px;grid-template-columns:80px;grid-template-rows:40px;justify-items:end;align-items:end'><div id=a style='width:20px;height:10px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(60, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_align_items_86(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:80px;height:40px;grid-template-columns:80px;grid-template-rows:40px;justify-items:center;align-items:end'><div id=a style='width:20px;height:10px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_align_items_87(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:80px;height:40px;grid-template-columns:80px;grid-template-rows:40px;justify-items:start;align-items:center'><div id=a style='width:20px;height:10px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(15, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_align_items_88(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:80px;height:40px;grid-template-columns:80px;grid-template-rows:40px;justify-items:end;align-items:center'><div id=a style='width:20px;height:10px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(60, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(15, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_align_items_89(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:80px;height:40px;grid-template-columns:80px;grid-template-rows:40px;justify-items:center;align-items:center'><div id=a style='width:20px;height:10px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(15, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_align_items_90(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:80px;height:40px;grid-template-columns:80px;grid-template-rows:40px;justify-items:start;align-items:start'><div id=a style='width:20px;height:10px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_align_items_91(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:80px;height:40px;grid-template-columns:80px;grid-template-rows:40px;justify-items:end;align-items:start'><div id=a style='width:20px;height:10px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(60, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_align_items_92(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:80px;height:40px;grid-template-columns:80px;grid-template-rows:40px;justify-items:center;align-items:start'><div id=a style='width:20px;height:10px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_align_items_93(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:80px;height:40px;grid-template-columns:80px;grid-template-rows:40px;justify-items:start;align-items:end'><div id=a style='width:20px;height:10px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_align_items_94(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:80px;height:40px;grid-template-columns:80px;grid-template-rows:40px;justify-items:end;align-items:end'><div id=a style='width:20px;height:10px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(60, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_align_items_95(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:80px;height:40px;grid-template-columns:80px;grid-template-rows:40px;justify-items:center;align-items:end'><div id=a style='width:20px;height:10px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_align_items_96(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:80px;height:40px;grid-template-columns:80px;grid-template-rows:40px;justify-items:start;align-items:center'><div id=a style='width:20px;height:10px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(15, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_align_items_97(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:80px;height:40px;grid-template-columns:80px;grid-template-rows:40px;justify-items:end;align-items:center'><div id=a style='width:20px;height:10px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(60, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(15, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_align_items_98(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:80px;height:40px;grid-template-columns:80px;grid-template-rows:40px;justify-items:center;align-items:center'><div id=a style='width:20px;height:10px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(15, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_align_items_99(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:80px;height:40px;grid-template-columns:80px;grid-template-rows:40px;justify-items:start;align-items:start'><div id=a style='width:20px;height:10px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_align_items_100(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:80px;height:40px;grid-template-columns:80px;grid-template-rows:40px;justify-items:end;align-items:start'><div id=a style='width:20px;height:10px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(60, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_align_self_1(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:80px;height:40px;grid-template-columns:80px;grid-template-rows:40px;justify-items:start;align-items:start'><div id=a style='width:20px;height:10px;justify-self:end;align-self:end'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(60, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_align_self_2(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:80px;height:40px;grid-template-columns:80px;grid-template-rows:40px;justify-items:start;align-items:start'><div id=a style='width:20px;height:10px;justify-self:end;align-self:end'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(60, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_align_self_3(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:80px;height:40px;grid-template-columns:80px;grid-template-rows:40px;justify-items:start;align-items:start'><div id=a style='width:20px;height:10px;justify-self:end;align-self:end'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(60, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_align_self_4(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:80px;height:40px;grid-template-columns:80px;grid-template-rows:40px;justify-items:start;align-items:start'><div id=a style='width:20px;height:10px;justify-self:end;align-self:end'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(60, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_align_self_5(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:80px;height:40px;grid-template-columns:80px;grid-template-rows:40px;justify-items:start;align-items:start'><div id=a style='width:20px;height:10px;justify-self:end;align-self:end'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(60, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_align_self_6(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:80px;height:40px;grid-template-columns:80px;grid-template-rows:40px;justify-items:start;align-items:start'><div id=a style='width:20px;height:10px;justify-self:end;align-self:end'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(60, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_align_self_7(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:80px;height:40px;grid-template-columns:80px;grid-template-rows:40px;justify-items:start;align-items:start'><div id=a style='width:20px;height:10px;justify-self:end;align-self:end'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(60, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_align_self_8(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:80px;height:40px;grid-template-columns:80px;grid-template-rows:40px;justify-items:start;align-items:start'><div id=a style='width:20px;height:10px;justify-self:end;align-self:end'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(60, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_align_self_9(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:80px;height:40px;grid-template-columns:80px;grid-template-rows:40px;justify-items:start;align-items:start'><div id=a style='width:20px;height:10px;justify-self:end;align-self:end'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(60, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_align_self_10(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:80px;height:40px;grid-template-columns:80px;grid-template-rows:40px;justify-items:start;align-items:start'><div id=a style='width:20px;height:10px;justify-self:end;align-self:end'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(60, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_align_self_11(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:80px;height:40px;grid-template-columns:80px;grid-template-rows:40px;justify-items:start;align-items:start'><div id=a style='width:20px;height:10px;justify-self:end;align-self:end'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(60, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_align_self_12(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:80px;height:40px;grid-template-columns:80px;grid-template-rows:40px;justify-items:start;align-items:start'><div id=a style='width:20px;height:10px;justify-self:end;align-self:end'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(60, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_align_self_13(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:80px;height:40px;grid-template-columns:80px;grid-template-rows:40px;justify-items:start;align-items:start'><div id=a style='width:20px;height:10px;justify-self:end;align-self:end'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(60, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_align_self_14(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:80px;height:40px;grid-template-columns:80px;grid-template-rows:40px;justify-items:start;align-items:start'><div id=a style='width:20px;height:10px;justify-self:end;align-self:end'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(60, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_align_self_15(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:80px;height:40px;grid-template-columns:80px;grid-template-rows:40px;justify-items:start;align-items:start'><div id=a style='width:20px;height:10px;justify-self:end;align-self:end'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(60, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_align_self_16(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:80px;height:40px;grid-template-columns:80px;grid-template-rows:40px;justify-items:start;align-items:start'><div id=a style='width:20px;height:10px;justify-self:end;align-self:end'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(60, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_align_self_17(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:80px;height:40px;grid-template-columns:80px;grid-template-rows:40px;justify-items:start;align-items:start'><div id=a style='width:20px;height:10px;justify-self:end;align-self:end'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(60, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_align_self_18(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:80px;height:40px;grid-template-columns:80px;grid-template-rows:40px;justify-items:start;align-items:start'><div id=a style='width:20px;height:10px;justify-self:end;align-self:end'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(60, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_align_self_19(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:80px;height:40px;grid-template-columns:80px;grid-template-rows:40px;justify-items:start;align-items:start'><div id=a style='width:20px;height:10px;justify-self:end;align-self:end'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(60, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_align_self_20(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:80px;height:40px;grid-template-columns:80px;grid-template-rows:40px;justify-items:start;align-items:start'><div id=a style='width:20px;height:10px;justify-self:end;align-self:end'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(60, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_align_self_21(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:80px;height:40px;grid-template-columns:80px;grid-template-rows:40px;justify-items:start;align-items:start'><div id=a style='width:20px;height:10px;justify-self:end;align-self:end'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(60, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_align_self_22(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:80px;height:40px;grid-template-columns:80px;grid-template-rows:40px;justify-items:start;align-items:start'><div id=a style='width:20px;height:10px;justify-self:end;align-self:end'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(60, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_align_self_23(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:80px;height:40px;grid-template-columns:80px;grid-template-rows:40px;justify-items:start;align-items:start'><div id=a style='width:20px;height:10px;justify-self:end;align-self:end'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(60, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_align_self_24(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:80px;height:40px;grid-template-columns:80px;grid-template-rows:40px;justify-items:start;align-items:start'><div id=a style='width:20px;height:10px;justify-self:end;align-self:end'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(60, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_align_self_25(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:80px;height:40px;grid-template-columns:80px;grid-template-rows:40px;justify-items:start;align-items:start'><div id=a style='width:20px;height:10px;justify-self:end;align-self:end'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(60, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_align_self_26(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:80px;height:40px;grid-template-columns:80px;grid-template-rows:40px;justify-items:start;align-items:start'><div id=a style='width:20px;height:10px;justify-self:end;align-self:end'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(60, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_align_self_27(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:80px;height:40px;grid-template-columns:80px;grid-template-rows:40px;justify-items:start;align-items:start'><div id=a style='width:20px;height:10px;justify-self:end;align-self:end'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(60, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_align_self_28(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:80px;height:40px;grid-template-columns:80px;grid-template-rows:40px;justify-items:start;align-items:start'><div id=a style='width:20px;height:10px;justify-self:end;align-self:end'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(60, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_align_self_29(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:80px;height:40px;grid-template-columns:80px;grid-template-rows:40px;justify-items:start;align-items:start'><div id=a style='width:20px;height:10px;justify-self:end;align-self:end'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(60, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_align_self_30(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:80px;height:40px;grid-template-columns:80px;grid-template-rows:40px;justify-items:start;align-items:start'><div id=a style='width:20px;height:10px;justify-self:end;align-self:end'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(60, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_align_self_31(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:80px;height:40px;grid-template-columns:80px;grid-template-rows:40px;justify-items:start;align-items:start'><div id=a style='width:20px;height:10px;justify-self:end;align-self:end'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(60, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_align_self_32(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:80px;height:40px;grid-template-columns:80px;grid-template-rows:40px;justify-items:start;align-items:start'><div id=a style='width:20px;height:10px;justify-self:end;align-self:end'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(60, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_align_self_33(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:80px;height:40px;grid-template-columns:80px;grid-template-rows:40px;justify-items:start;align-items:start'><div id=a style='width:20px;height:10px;justify-self:end;align-self:end'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(60, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_align_self_34(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:80px;height:40px;grid-template-columns:80px;grid-template-rows:40px;justify-items:start;align-items:start'><div id=a style='width:20px;height:10px;justify-self:end;align-self:end'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(60, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_align_self_35(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:80px;height:40px;grid-template-columns:80px;grid-template-rows:40px;justify-items:start;align-items:start'><div id=a style='width:20px;height:10px;justify-self:end;align-self:end'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(60, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_align_self_36(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:80px;height:40px;grid-template-columns:80px;grid-template-rows:40px;justify-items:start;align-items:start'><div id=a style='width:20px;height:10px;justify-self:end;align-self:end'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(60, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_align_self_37(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:80px;height:40px;grid-template-columns:80px;grid-template-rows:40px;justify-items:start;align-items:start'><div id=a style='width:20px;height:10px;justify-self:end;align-self:end'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(60, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_align_self_38(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:80px;height:40px;grid-template-columns:80px;grid-template-rows:40px;justify-items:start;align-items:start'><div id=a style='width:20px;height:10px;justify-self:end;align-self:end'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(60, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_align_self_39(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:80px;height:40px;grid-template-columns:80px;grid-template-rows:40px;justify-items:start;align-items:start'><div id=a style='width:20px;height:10px;justify-self:end;align-self:end'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(60, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_align_self_40(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:80px;height:40px;grid-template-columns:80px;grid-template-rows:40px;justify-items:start;align-items:start'><div id=a style='width:20px;height:10px;justify-self:end;align-self:end'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(60, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_align_self_41(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:80px;height:40px;grid-template-columns:80px;grid-template-rows:40px;justify-items:start;align-items:start'><div id=a style='width:20px;height:10px;justify-self:end;align-self:end'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(60, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_align_self_42(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:80px;height:40px;grid-template-columns:80px;grid-template-rows:40px;justify-items:start;align-items:start'><div id=a style='width:20px;height:10px;justify-self:end;align-self:end'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(60, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_align_self_43(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:80px;height:40px;grid-template-columns:80px;grid-template-rows:40px;justify-items:start;align-items:start'><div id=a style='width:20px;height:10px;justify-self:end;align-self:end'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(60, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_align_self_44(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:80px;height:40px;grid-template-columns:80px;grid-template-rows:40px;justify-items:start;align-items:start'><div id=a style='width:20px;height:10px;justify-self:end;align-self:end'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(60, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_align_self_45(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:80px;height:40px;grid-template-columns:80px;grid-template-rows:40px;justify-items:start;align-items:start'><div id=a style='width:20px;height:10px;justify-self:end;align-self:end'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(60, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_align_self_46(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:80px;height:40px;grid-template-columns:80px;grid-template-rows:40px;justify-items:start;align-items:start'><div id=a style='width:20px;height:10px;justify-self:end;align-self:end'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(60, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_align_self_47(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:80px;height:40px;grid-template-columns:80px;grid-template-rows:40px;justify-items:start;align-items:start'><div id=a style='width:20px;height:10px;justify-self:end;align-self:end'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(60, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_align_self_48(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:80px;height:40px;grid-template-columns:80px;grid-template-rows:40px;justify-items:start;align-items:start'><div id=a style='width:20px;height:10px;justify-self:end;align-self:end'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(60, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_align_self_49(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:80px;height:40px;grid-template-columns:80px;grid-template-rows:40px;justify-items:start;align-items:start'><div id=a style='width:20px;height:10px;justify-self:end;align-self:end'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(60, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_align_self_50(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:80px;height:40px;grid-template-columns:80px;grid-template-rows:40px;justify-items:start;align-items:start'><div id=a style='width:20px;height:10px;justify-self:end;align-self:end'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(60, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_align_self_51(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:80px;height:40px;grid-template-columns:80px;grid-template-rows:40px;justify-items:start;align-items:start'><div id=a style='width:20px;height:10px;justify-self:end;align-self:end'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(60, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_align_self_52(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:80px;height:40px;grid-template-columns:80px;grid-template-rows:40px;justify-items:start;align-items:start'><div id=a style='width:20px;height:10px;justify-self:end;align-self:end'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(60, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_align_self_53(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:80px;height:40px;grid-template-columns:80px;grid-template-rows:40px;justify-items:start;align-items:start'><div id=a style='width:20px;height:10px;justify-self:end;align-self:end'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(60, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_align_self_54(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:80px;height:40px;grid-template-columns:80px;grid-template-rows:40px;justify-items:start;align-items:start'><div id=a style='width:20px;height:10px;justify-self:end;align-self:end'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(60, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_align_self_55(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:80px;height:40px;grid-template-columns:80px;grid-template-rows:40px;justify-items:start;align-items:start'><div id=a style='width:20px;height:10px;justify-self:end;align-self:end'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(60, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_align_self_56(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:80px;height:40px;grid-template-columns:80px;grid-template-rows:40px;justify-items:start;align-items:start'><div id=a style='width:20px;height:10px;justify-self:end;align-self:end'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(60, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_align_self_57(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:80px;height:40px;grid-template-columns:80px;grid-template-rows:40px;justify-items:start;align-items:start'><div id=a style='width:20px;height:10px;justify-self:end;align-self:end'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(60, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_align_self_58(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:80px;height:40px;grid-template-columns:80px;grid-template-rows:40px;justify-items:start;align-items:start'><div id=a style='width:20px;height:10px;justify-self:end;align-self:end'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(60, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_align_self_59(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:80px;height:40px;grid-template-columns:80px;grid-template-rows:40px;justify-items:start;align-items:start'><div id=a style='width:20px;height:10px;justify-self:end;align-self:end'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(60, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_align_self_60(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:80px;height:40px;grid-template-columns:80px;grid-template-rows:40px;justify-items:start;align-items:start'><div id=a style='width:20px;height:10px;justify-self:end;align-self:end'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(60, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_flex_grid_edge_1(void) {
    lot52_render("<body style='margin:0'><div id=p style='display:flex;width:160px;align-items:start'><div id=g style='display:grid;width:80px;grid-template-columns:40px 40px'><div id=a style='height:10px'></div><div id=b style='height:10px'></div></div><div id=t style='width:40px;height:10px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("g")->x);
    TEST_ASSERT_EQUAL(80, lot52_node("t")->x);
    TEST_ASSERT_EQUAL(40, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_flex_grid_edge_2(void) {
    lot52_render("<body style='margin:0'><div id=p style='display:flex;width:160px;align-items:start'><div id=g style='display:grid;width:80px;grid-template-columns:40px 40px'><div id=a style='height:10px'></div><div id=b style='height:10px'></div></div><div id=t style='width:40px;height:10px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("g")->x);
    TEST_ASSERT_EQUAL(80, lot52_node("t")->x);
    TEST_ASSERT_EQUAL(40, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_flex_grid_edge_3(void) {
    lot52_render("<body style='margin:0'><div id=p style='display:flex;width:160px;align-items:start'><div id=g style='display:grid;width:80px;grid-template-columns:40px 40px'><div id=a style='height:10px'></div><div id=b style='height:10px'></div></div><div id=t style='width:40px;height:10px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("g")->x);
    TEST_ASSERT_EQUAL(80, lot52_node("t")->x);
    TEST_ASSERT_EQUAL(40, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_flex_grid_edge_4(void) {
    lot52_render("<body style='margin:0'><div id=p style='display:flex;width:160px;align-items:start'><div id=g style='display:grid;width:80px;grid-template-columns:40px 40px'><div id=a style='height:10px'></div><div id=b style='height:10px'></div></div><div id=t style='width:40px;height:10px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("g")->x);
    TEST_ASSERT_EQUAL(80, lot52_node("t")->x);
    TEST_ASSERT_EQUAL(40, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_flex_grid_edge_5(void) {
    lot52_render("<body style='margin:0'><div id=p style='display:flex;width:160px;align-items:start'><div id=g style='display:grid;width:80px;grid-template-columns:40px 40px'><div id=a style='height:10px'></div><div id=b style='height:10px'></div></div><div id=t style='width:40px;height:10px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("g")->x);
    TEST_ASSERT_EQUAL(80, lot52_node("t")->x);
    TEST_ASSERT_EQUAL(40, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_flex_grid_edge_6(void) {
    lot52_render("<body style='margin:0'><div id=p style='display:flex;width:160px;align-items:start'><div id=g style='display:grid;width:80px;grid-template-columns:40px 40px'><div id=a style='height:10px'></div><div id=b style='height:10px'></div></div><div id=t style='width:40px;height:10px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("g")->x);
    TEST_ASSERT_EQUAL(80, lot52_node("t")->x);
    TEST_ASSERT_EQUAL(40, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_flex_grid_edge_7(void) {
    lot52_render("<body style='margin:0'><div id=p style='display:flex;width:160px;align-items:start'><div id=g style='display:grid;width:80px;grid-template-columns:40px 40px'><div id=a style='height:10px'></div><div id=b style='height:10px'></div></div><div id=t style='width:40px;height:10px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("g")->x);
    TEST_ASSERT_EQUAL(80, lot52_node("t")->x);
    TEST_ASSERT_EQUAL(40, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_flex_grid_edge_8(void) {
    lot52_render("<body style='margin:0'><div id=p style='display:flex;width:160px;align-items:start'><div id=g style='display:grid;width:80px;grid-template-columns:40px 40px'><div id=a style='height:10px'></div><div id=b style='height:10px'></div></div><div id=t style='width:40px;height:10px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("g")->x);
    TEST_ASSERT_EQUAL(80, lot52_node("t")->x);
    TEST_ASSERT_EQUAL(40, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_flex_grid_edge_9(void) {
    lot52_render("<body style='margin:0'><div id=p style='display:flex;width:160px;align-items:start'><div id=g style='display:grid;width:80px;grid-template-columns:40px 40px'><div id=a style='height:10px'></div><div id=b style='height:10px'></div></div><div id=t style='width:40px;height:10px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("g")->x);
    TEST_ASSERT_EQUAL(80, lot52_node("t")->x);
    TEST_ASSERT_EQUAL(40, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_flex_grid_edge_10(void) {
    lot52_render("<body style='margin:0'><div id=p style='display:flex;width:160px;align-items:start'><div id=g style='display:grid;width:80px;grid-template-columns:40px 40px'><div id=a style='height:10px'></div><div id=b style='height:10px'></div></div><div id=t style='width:40px;height:10px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("g")->x);
    TEST_ASSERT_EQUAL(80, lot52_node("t")->x);
    TEST_ASSERT_EQUAL(40, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_flex_grid_edge_11(void) {
    lot52_render("<body style='margin:0'><div id=p style='display:flex;width:160px;align-items:start'><div id=g style='display:grid;width:80px;grid-template-columns:40px 40px'><div id=a style='height:10px'></div><div id=b style='height:10px'></div></div><div id=t style='width:40px;height:10px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("g")->x);
    TEST_ASSERT_EQUAL(80, lot52_node("t")->x);
    TEST_ASSERT_EQUAL(40, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_flex_grid_edge_12(void) {
    lot52_render("<body style='margin:0'><div id=p style='display:flex;width:160px;align-items:start'><div id=g style='display:grid;width:80px;grid-template-columns:40px 40px'><div id=a style='height:10px'></div><div id=b style='height:10px'></div></div><div id=t style='width:40px;height:10px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("g")->x);
    TEST_ASSERT_EQUAL(80, lot52_node("t")->x);
    TEST_ASSERT_EQUAL(40, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_flex_grid_edge_13(void) {
    lot52_render("<body style='margin:0'><div id=p style='display:flex;width:160px;align-items:start'><div id=g style='display:grid;width:80px;grid-template-columns:40px 40px'><div id=a style='height:10px'></div><div id=b style='height:10px'></div></div><div id=t style='width:40px;height:10px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("g")->x);
    TEST_ASSERT_EQUAL(80, lot52_node("t")->x);
    TEST_ASSERT_EQUAL(40, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_flex_grid_edge_14(void) {
    lot52_render("<body style='margin:0'><div id=p style='display:flex;width:160px;align-items:start'><div id=g style='display:grid;width:80px;grid-template-columns:40px 40px'><div id=a style='height:10px'></div><div id=b style='height:10px'></div></div><div id=t style='width:40px;height:10px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("g")->x);
    TEST_ASSERT_EQUAL(80, lot52_node("t")->x);
    TEST_ASSERT_EQUAL(40, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_flex_grid_edge_15(void) {
    lot52_render("<body style='margin:0'><div id=p style='display:flex;width:160px;align-items:start'><div id=g style='display:grid;width:80px;grid-template-columns:40px 40px'><div id=a style='height:10px'></div><div id=b style='height:10px'></div></div><div id=t style='width:40px;height:10px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("g")->x);
    TEST_ASSERT_EQUAL(80, lot52_node("t")->x);
    TEST_ASSERT_EQUAL(40, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_flex_grid_edge_16(void) {
    lot52_render("<body style='margin:0'><div id=p style='display:flex;width:160px;align-items:start'><div id=g style='display:grid;width:80px;grid-template-columns:40px 40px'><div id=a style='height:10px'></div><div id=b style='height:10px'></div></div><div id=t style='width:40px;height:10px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("g")->x);
    TEST_ASSERT_EQUAL(80, lot52_node("t")->x);
    TEST_ASSERT_EQUAL(40, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_flex_grid_edge_17(void) {
    lot52_render("<body style='margin:0'><div id=p style='display:flex;width:160px;align-items:start'><div id=g style='display:grid;width:80px;grid-template-columns:40px 40px'><div id=a style='height:10px'></div><div id=b style='height:10px'></div></div><div id=t style='width:40px;height:10px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("g")->x);
    TEST_ASSERT_EQUAL(80, lot52_node("t")->x);
    TEST_ASSERT_EQUAL(40, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_flex_grid_edge_18(void) {
    lot52_render("<body style='margin:0'><div id=p style='display:flex;width:160px;align-items:start'><div id=g style='display:grid;width:80px;grid-template-columns:40px 40px'><div id=a style='height:10px'></div><div id=b style='height:10px'></div></div><div id=t style='width:40px;height:10px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("g")->x);
    TEST_ASSERT_EQUAL(80, lot52_node("t")->x);
    TEST_ASSERT_EQUAL(40, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_flex_grid_edge_19(void) {
    lot52_render("<body style='margin:0'><div id=p style='display:flex;width:160px;align-items:start'><div id=g style='display:grid;width:80px;grid-template-columns:40px 40px'><div id=a style='height:10px'></div><div id=b style='height:10px'></div></div><div id=t style='width:40px;height:10px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("g")->x);
    TEST_ASSERT_EQUAL(80, lot52_node("t")->x);
    TEST_ASSERT_EQUAL(40, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_flex_grid_edge_20(void) {
    lot52_render("<body style='margin:0'><div id=p style='display:flex;width:160px;align-items:start'><div id=g style='display:grid;width:80px;grid-template-columns:40px 40px'><div id=a style='height:10px'></div><div id=b style='height:10px'></div></div><div id=t style='width:40px;height:10px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("g")->x);
    TEST_ASSERT_EQUAL(80, lot52_node("t")->x);
    TEST_ASSERT_EQUAL(40, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_flex_grid_edge_21(void) {
    lot52_render("<body style='margin:0'><div id=p style='display:flex;width:160px;align-items:start'><div id=g style='display:grid;width:80px;grid-template-columns:40px 40px'><div id=a style='height:10px'></div><div id=b style='height:10px'></div></div><div id=t style='width:40px;height:10px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("g")->x);
    TEST_ASSERT_EQUAL(80, lot52_node("t")->x);
    TEST_ASSERT_EQUAL(40, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_flex_grid_edge_22(void) {
    lot52_render("<body style='margin:0'><div id=p style='display:flex;width:160px;align-items:start'><div id=g style='display:grid;width:80px;grid-template-columns:40px 40px'><div id=a style='height:10px'></div><div id=b style='height:10px'></div></div><div id=t style='width:40px;height:10px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("g")->x);
    TEST_ASSERT_EQUAL(80, lot52_node("t")->x);
    TEST_ASSERT_EQUAL(40, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_flex_grid_edge_23(void) {
    lot52_render("<body style='margin:0'><div id=p style='display:flex;width:160px;align-items:start'><div id=g style='display:grid;width:80px;grid-template-columns:40px 40px'><div id=a style='height:10px'></div><div id=b style='height:10px'></div></div><div id=t style='width:40px;height:10px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("g")->x);
    TEST_ASSERT_EQUAL(80, lot52_node("t")->x);
    TEST_ASSERT_EQUAL(40, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_flex_grid_edge_24(void) {
    lot52_render("<body style='margin:0'><div id=p style='display:flex;width:160px;align-items:start'><div id=g style='display:grid;width:80px;grid-template-columns:40px 40px'><div id=a style='height:10px'></div><div id=b style='height:10px'></div></div><div id=t style='width:40px;height:10px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("g")->x);
    TEST_ASSERT_EQUAL(80, lot52_node("t")->x);
    TEST_ASSERT_EQUAL(40, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_flex_grid_edge_25(void) {
    lot52_render("<body style='margin:0'><div id=p style='display:flex;width:160px;align-items:start'><div id=g style='display:grid;width:80px;grid-template-columns:40px 40px'><div id=a style='height:10px'></div><div id=b style='height:10px'></div></div><div id=t style='width:40px;height:10px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("g")->x);
    TEST_ASSERT_EQUAL(80, lot52_node("t")->x);
    TEST_ASSERT_EQUAL(40, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_flex_grid_edge_26(void) {
    lot52_render("<body style='margin:0'><div id=p style='display:flex;width:160px;align-items:start'><div id=g style='display:grid;width:80px;grid-template-columns:40px 40px'><div id=a style='height:10px'></div><div id=b style='height:10px'></div></div><div id=t style='width:40px;height:10px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("g")->x);
    TEST_ASSERT_EQUAL(80, lot52_node("t")->x);
    TEST_ASSERT_EQUAL(40, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_flex_grid_edge_27(void) {
    lot52_render("<body style='margin:0'><div id=p style='display:flex;width:160px;align-items:start'><div id=g style='display:grid;width:80px;grid-template-columns:40px 40px'><div id=a style='height:10px'></div><div id=b style='height:10px'></div></div><div id=t style='width:40px;height:10px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("g")->x);
    TEST_ASSERT_EQUAL(80, lot52_node("t")->x);
    TEST_ASSERT_EQUAL(40, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_flex_grid_edge_28(void) {
    lot52_render("<body style='margin:0'><div id=p style='display:flex;width:160px;align-items:start'><div id=g style='display:grid;width:80px;grid-template-columns:40px 40px'><div id=a style='height:10px'></div><div id=b style='height:10px'></div></div><div id=t style='width:40px;height:10px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("g")->x);
    TEST_ASSERT_EQUAL(80, lot52_node("t")->x);
    TEST_ASSERT_EQUAL(40, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_flex_grid_edge_29(void) {
    lot52_render("<body style='margin:0'><div id=p style='display:flex;width:160px;align-items:start'><div id=g style='display:grid;width:80px;grid-template-columns:40px 40px'><div id=a style='height:10px'></div><div id=b style='height:10px'></div></div><div id=t style='width:40px;height:10px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("g")->x);
    TEST_ASSERT_EQUAL(80, lot52_node("t")->x);
    TEST_ASSERT_EQUAL(40, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_flex_grid_edge_30(void) {
    lot52_render("<body style='margin:0'><div id=p style='display:flex;width:160px;align-items:start'><div id=g style='display:grid;width:80px;grid-template-columns:40px 40px'><div id=a style='height:10px'></div><div id=b style='height:10px'></div></div><div id=t style='width:40px;height:10px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("g")->x);
    TEST_ASSERT_EQUAL(80, lot52_node("t")->x);
    TEST_ASSERT_EQUAL(40, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_flex_grid_edge_31(void) {
    lot52_render("<body style='margin:0'><div id=p style='display:flex;width:160px;align-items:start'><div id=g style='display:grid;width:80px;grid-template-columns:40px 40px'><div id=a style='height:10px'></div><div id=b style='height:10px'></div></div><div id=t style='width:40px;height:10px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("g")->x);
    TEST_ASSERT_EQUAL(80, lot52_node("t")->x);
    TEST_ASSERT_EQUAL(40, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_flex_grid_edge_32(void) {
    lot52_render("<body style='margin:0'><div id=p style='display:flex;width:160px;align-items:start'><div id=g style='display:grid;width:80px;grid-template-columns:40px 40px'><div id=a style='height:10px'></div><div id=b style='height:10px'></div></div><div id=t style='width:40px;height:10px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("g")->x);
    TEST_ASSERT_EQUAL(80, lot52_node("t")->x);
    TEST_ASSERT_EQUAL(40, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_flex_grid_edge_33(void) {
    lot52_render("<body style='margin:0'><div id=p style='display:flex;width:160px;align-items:start'><div id=g style='display:grid;width:80px;grid-template-columns:40px 40px'><div id=a style='height:10px'></div><div id=b style='height:10px'></div></div><div id=t style='width:40px;height:10px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("g")->x);
    TEST_ASSERT_EQUAL(80, lot52_node("t")->x);
    TEST_ASSERT_EQUAL(40, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_flex_grid_edge_34(void) {
    lot52_render("<body style='margin:0'><div id=p style='display:flex;width:160px;align-items:start'><div id=g style='display:grid;width:80px;grid-template-columns:40px 40px'><div id=a style='height:10px'></div><div id=b style='height:10px'></div></div><div id=t style='width:40px;height:10px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("g")->x);
    TEST_ASSERT_EQUAL(80, lot52_node("t")->x);
    TEST_ASSERT_EQUAL(40, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_flex_grid_edge_35(void) {
    lot52_render("<body style='margin:0'><div id=p style='display:flex;width:160px;align-items:start'><div id=g style='display:grid;width:80px;grid-template-columns:40px 40px'><div id=a style='height:10px'></div><div id=b style='height:10px'></div></div><div id=t style='width:40px;height:10px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("g")->x);
    TEST_ASSERT_EQUAL(80, lot52_node("t")->x);
    TEST_ASSERT_EQUAL(40, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_flex_grid_edge_36(void) {
    lot52_render("<body style='margin:0'><div id=p style='display:flex;width:160px;align-items:start'><div id=g style='display:grid;width:80px;grid-template-columns:40px 40px'><div id=a style='height:10px'></div><div id=b style='height:10px'></div></div><div id=t style='width:40px;height:10px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("g")->x);
    TEST_ASSERT_EQUAL(80, lot52_node("t")->x);
    TEST_ASSERT_EQUAL(40, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_flex_grid_edge_37(void) {
    lot52_render("<body style='margin:0'><div id=p style='display:flex;width:160px;align-items:start'><div id=g style='display:grid;width:80px;grid-template-columns:40px 40px'><div id=a style='height:10px'></div><div id=b style='height:10px'></div></div><div id=t style='width:40px;height:10px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("g")->x);
    TEST_ASSERT_EQUAL(80, lot52_node("t")->x);
    TEST_ASSERT_EQUAL(40, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_flex_grid_edge_38(void) {
    lot52_render("<body style='margin:0'><div id=p style='display:flex;width:160px;align-items:start'><div id=g style='display:grid;width:80px;grid-template-columns:40px 40px'><div id=a style='height:10px'></div><div id=b style='height:10px'></div></div><div id=t style='width:40px;height:10px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("g")->x);
    TEST_ASSERT_EQUAL(80, lot52_node("t")->x);
    TEST_ASSERT_EQUAL(40, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_flex_grid_edge_39(void) {
    lot52_render("<body style='margin:0'><div id=p style='display:flex;width:160px;align-items:start'><div id=g style='display:grid;width:80px;grid-template-columns:40px 40px'><div id=a style='height:10px'></div><div id=b style='height:10px'></div></div><div id=t style='width:40px;height:10px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("g")->x);
    TEST_ASSERT_EQUAL(80, lot52_node("t")->x);
    TEST_ASSERT_EQUAL(40, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_flex_grid_edge_40(void) {
    lot52_render("<body style='margin:0'><div id=p style='display:flex;width:160px;align-items:start'><div id=g style='display:grid;width:80px;grid-template-columns:40px 40px'><div id=a style='height:10px'></div><div id=b style='height:10px'></div></div><div id=t style='width:40px;height:10px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("g")->x);
    TEST_ASSERT_EQUAL(80, lot52_node("t")->x);
    TEST_ASSERT_EQUAL(40, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void lot78_align_run(void) {
    RUN_TEST(test_l78_align_items_1);
    RUN_TEST(test_l78_align_items_2);
    RUN_TEST(test_l78_align_items_3);
    RUN_TEST(test_l78_align_items_4);
    RUN_TEST(test_l78_align_items_5);
    RUN_TEST(test_l78_align_items_6);
    RUN_TEST(test_l78_align_items_7);
    RUN_TEST(test_l78_align_items_8);
    RUN_TEST(test_l78_align_items_9);
    RUN_TEST(test_l78_align_items_10);
    RUN_TEST(test_l78_align_items_11);
    RUN_TEST(test_l78_align_items_12);
    RUN_TEST(test_l78_align_items_13);
    RUN_TEST(test_l78_align_items_14);
    RUN_TEST(test_l78_align_items_15);
    RUN_TEST(test_l78_align_items_16);
    RUN_TEST(test_l78_align_items_17);
    RUN_TEST(test_l78_align_items_18);
    RUN_TEST(test_l78_align_items_19);
    RUN_TEST(test_l78_align_items_20);
    RUN_TEST(test_l78_align_items_21);
    RUN_TEST(test_l78_align_items_22);
    RUN_TEST(test_l78_align_items_23);
    RUN_TEST(test_l78_align_items_24);
    RUN_TEST(test_l78_align_items_25);
    RUN_TEST(test_l78_align_items_26);
    RUN_TEST(test_l78_align_items_27);
    RUN_TEST(test_l78_align_items_28);
    RUN_TEST(test_l78_align_items_29);
    RUN_TEST(test_l78_align_items_30);
    RUN_TEST(test_l78_align_items_31);
    RUN_TEST(test_l78_align_items_32);
    RUN_TEST(test_l78_align_items_33);
    RUN_TEST(test_l78_align_items_34);
    RUN_TEST(test_l78_align_items_35);
    RUN_TEST(test_l78_align_items_36);
    RUN_TEST(test_l78_align_items_37);
    RUN_TEST(test_l78_align_items_38);
    RUN_TEST(test_l78_align_items_39);
    RUN_TEST(test_l78_align_items_40);
    RUN_TEST(test_l78_align_items_41);
    RUN_TEST(test_l78_align_items_42);
    RUN_TEST(test_l78_align_items_43);
    RUN_TEST(test_l78_align_items_44);
    RUN_TEST(test_l78_align_items_45);
    RUN_TEST(test_l78_align_items_46);
    RUN_TEST(test_l78_align_items_47);
    RUN_TEST(test_l78_align_items_48);
    RUN_TEST(test_l78_align_items_49);
    RUN_TEST(test_l78_align_items_50);
    RUN_TEST(test_l78_align_items_51);
    RUN_TEST(test_l78_align_items_52);
    RUN_TEST(test_l78_align_items_53);
    RUN_TEST(test_l78_align_items_54);
    RUN_TEST(test_l78_align_items_55);
    RUN_TEST(test_l78_align_items_56);
    RUN_TEST(test_l78_align_items_57);
    RUN_TEST(test_l78_align_items_58);
    RUN_TEST(test_l78_align_items_59);
    RUN_TEST(test_l78_align_items_60);
    RUN_TEST(test_l78_align_items_61);
    RUN_TEST(test_l78_align_items_62);
    RUN_TEST(test_l78_align_items_63);
    RUN_TEST(test_l78_align_items_64);
    RUN_TEST(test_l78_align_items_65);
    RUN_TEST(test_l78_align_items_66);
    RUN_TEST(test_l78_align_items_67);
    RUN_TEST(test_l78_align_items_68);
    RUN_TEST(test_l78_align_items_69);
    RUN_TEST(test_l78_align_items_70);
    RUN_TEST(test_l78_align_items_71);
    RUN_TEST(test_l78_align_items_72);
    RUN_TEST(test_l78_align_items_73);
    RUN_TEST(test_l78_align_items_74);
    RUN_TEST(test_l78_align_items_75);
    RUN_TEST(test_l78_align_items_76);
    RUN_TEST(test_l78_align_items_77);
    RUN_TEST(test_l78_align_items_78);
    RUN_TEST(test_l78_align_items_79);
    RUN_TEST(test_l78_align_items_80);
    RUN_TEST(test_l78_align_items_81);
    RUN_TEST(test_l78_align_items_82);
    RUN_TEST(test_l78_align_items_83);
    RUN_TEST(test_l78_align_items_84);
    RUN_TEST(test_l78_align_items_85);
    RUN_TEST(test_l78_align_items_86);
    RUN_TEST(test_l78_align_items_87);
    RUN_TEST(test_l78_align_items_88);
    RUN_TEST(test_l78_align_items_89);
    RUN_TEST(test_l78_align_items_90);
    RUN_TEST(test_l78_align_items_91);
    RUN_TEST(test_l78_align_items_92);
    RUN_TEST(test_l78_align_items_93);
    RUN_TEST(test_l78_align_items_94);
    RUN_TEST(test_l78_align_items_95);
    RUN_TEST(test_l78_align_items_96);
    RUN_TEST(test_l78_align_items_97);
    RUN_TEST(test_l78_align_items_98);
    RUN_TEST(test_l78_align_items_99);
    RUN_TEST(test_l78_align_items_100);
    RUN_TEST(test_l78_align_self_1);
    RUN_TEST(test_l78_align_self_2);
    RUN_TEST(test_l78_align_self_3);
    RUN_TEST(test_l78_align_self_4);
    RUN_TEST(test_l78_align_self_5);
    RUN_TEST(test_l78_align_self_6);
    RUN_TEST(test_l78_align_self_7);
    RUN_TEST(test_l78_align_self_8);
    RUN_TEST(test_l78_align_self_9);
    RUN_TEST(test_l78_align_self_10);
    RUN_TEST(test_l78_align_self_11);
    RUN_TEST(test_l78_align_self_12);
    RUN_TEST(test_l78_align_self_13);
    RUN_TEST(test_l78_align_self_14);
    RUN_TEST(test_l78_align_self_15);
    RUN_TEST(test_l78_align_self_16);
    RUN_TEST(test_l78_align_self_17);
    RUN_TEST(test_l78_align_self_18);
    RUN_TEST(test_l78_align_self_19);
    RUN_TEST(test_l78_align_self_20);
    RUN_TEST(test_l78_align_self_21);
    RUN_TEST(test_l78_align_self_22);
    RUN_TEST(test_l78_align_self_23);
    RUN_TEST(test_l78_align_self_24);
    RUN_TEST(test_l78_align_self_25);
    RUN_TEST(test_l78_align_self_26);
    RUN_TEST(test_l78_align_self_27);
    RUN_TEST(test_l78_align_self_28);
    RUN_TEST(test_l78_align_self_29);
    RUN_TEST(test_l78_align_self_30);
    RUN_TEST(test_l78_align_self_31);
    RUN_TEST(test_l78_align_self_32);
    RUN_TEST(test_l78_align_self_33);
    RUN_TEST(test_l78_align_self_34);
    RUN_TEST(test_l78_align_self_35);
    RUN_TEST(test_l78_align_self_36);
    RUN_TEST(test_l78_align_self_37);
    RUN_TEST(test_l78_align_self_38);
    RUN_TEST(test_l78_align_self_39);
    RUN_TEST(test_l78_align_self_40);
    RUN_TEST(test_l78_align_self_41);
    RUN_TEST(test_l78_align_self_42);
    RUN_TEST(test_l78_align_self_43);
    RUN_TEST(test_l78_align_self_44);
    RUN_TEST(test_l78_align_self_45);
    RUN_TEST(test_l78_align_self_46);
    RUN_TEST(test_l78_align_self_47);
    RUN_TEST(test_l78_align_self_48);
    RUN_TEST(test_l78_align_self_49);
    RUN_TEST(test_l78_align_self_50);
    RUN_TEST(test_l78_align_self_51);
    RUN_TEST(test_l78_align_self_52);
    RUN_TEST(test_l78_align_self_53);
    RUN_TEST(test_l78_align_self_54);
    RUN_TEST(test_l78_align_self_55);
    RUN_TEST(test_l78_align_self_56);
    RUN_TEST(test_l78_align_self_57);
    RUN_TEST(test_l78_align_self_58);
    RUN_TEST(test_l78_align_self_59);
    RUN_TEST(test_l78_align_self_60);
    RUN_TEST(test_l78_flex_grid_edge_1);
    RUN_TEST(test_l78_flex_grid_edge_2);
    RUN_TEST(test_l78_flex_grid_edge_3);
    RUN_TEST(test_l78_flex_grid_edge_4);
    RUN_TEST(test_l78_flex_grid_edge_5);
    RUN_TEST(test_l78_flex_grid_edge_6);
    RUN_TEST(test_l78_flex_grid_edge_7);
    RUN_TEST(test_l78_flex_grid_edge_8);
    RUN_TEST(test_l78_flex_grid_edge_9);
    RUN_TEST(test_l78_flex_grid_edge_10);
    RUN_TEST(test_l78_flex_grid_edge_11);
    RUN_TEST(test_l78_flex_grid_edge_12);
    RUN_TEST(test_l78_flex_grid_edge_13);
    RUN_TEST(test_l78_flex_grid_edge_14);
    RUN_TEST(test_l78_flex_grid_edge_15);
    RUN_TEST(test_l78_flex_grid_edge_16);
    RUN_TEST(test_l78_flex_grid_edge_17);
    RUN_TEST(test_l78_flex_grid_edge_18);
    RUN_TEST(test_l78_flex_grid_edge_19);
    RUN_TEST(test_l78_flex_grid_edge_20);
    RUN_TEST(test_l78_flex_grid_edge_21);
    RUN_TEST(test_l78_flex_grid_edge_22);
    RUN_TEST(test_l78_flex_grid_edge_23);
    RUN_TEST(test_l78_flex_grid_edge_24);
    RUN_TEST(test_l78_flex_grid_edge_25);
    RUN_TEST(test_l78_flex_grid_edge_26);
    RUN_TEST(test_l78_flex_grid_edge_27);
    RUN_TEST(test_l78_flex_grid_edge_28);
    RUN_TEST(test_l78_flex_grid_edge_29);
    RUN_TEST(test_l78_flex_grid_edge_30);
    RUN_TEST(test_l78_flex_grid_edge_31);
    RUN_TEST(test_l78_flex_grid_edge_32);
    RUN_TEST(test_l78_flex_grid_edge_33);
    RUN_TEST(test_l78_flex_grid_edge_34);
    RUN_TEST(test_l78_flex_grid_edge_35);
    RUN_TEST(test_l78_flex_grid_edge_36);
    RUN_TEST(test_l78_flex_grid_edge_37);
    RUN_TEST(test_l78_flex_grid_edge_38);
    RUN_TEST(test_l78_flex_grid_edge_39);
    RUN_TEST(test_l78_flex_grid_edge_40);
}
