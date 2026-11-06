/* Lot 79: grid-auto-rows / grid-auto-columns and minmax tracks. */
static void test_l79_auto_rows_1(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:50px;grid-template-columns:50px;grid-auto-rows:8px;grid-auto-flow:row;align-items:stretch'><div id=i0 style='height:8px'></div><div id=i1 style='height:8px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("i0")->y);
    TEST_ASSERT_EQUAL(8, lot52_node("i0")->h);
    TEST_ASSERT_EQUAL(8, lot52_node("i1")->y);
    TEST_ASSERT_EQUAL(8, lot52_node("i1")->h);
    TEST_ASSERT_EQUAL(16, lot52_node("g")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_auto_rows_2(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:50px;grid-template-columns:50px;grid-auto-rows:8px;grid-auto-flow:row;align-items:stretch'><div id=i0 style='height:8px'></div><div id=i1 style='height:8px'></div><div id=i2 style='height:8px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("i0")->y);
    TEST_ASSERT_EQUAL(8, lot52_node("i0")->h);
    TEST_ASSERT_EQUAL(8, lot52_node("i1")->y);
    TEST_ASSERT_EQUAL(8, lot52_node("i1")->h);
    TEST_ASSERT_EQUAL(16, lot52_node("i2")->y);
    TEST_ASSERT_EQUAL(8, lot52_node("i2")->h);
    TEST_ASSERT_EQUAL(24, lot52_node("g")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_auto_rows_3(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:50px;grid-template-columns:50px;grid-auto-rows:8px;grid-auto-flow:row;align-items:stretch'><div id=i0 style='height:8px'></div><div id=i1 style='height:8px'></div><div id=i2 style='height:8px'></div><div id=i3 style='height:8px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("i0")->y);
    TEST_ASSERT_EQUAL(8, lot52_node("i0")->h);
    TEST_ASSERT_EQUAL(8, lot52_node("i1")->y);
    TEST_ASSERT_EQUAL(8, lot52_node("i1")->h);
    TEST_ASSERT_EQUAL(16, lot52_node("i2")->y);
    TEST_ASSERT_EQUAL(8, lot52_node("i2")->h);
    TEST_ASSERT_EQUAL(24, lot52_node("i3")->y);
    TEST_ASSERT_EQUAL(8, lot52_node("i3")->h);
    TEST_ASSERT_EQUAL(32, lot52_node("g")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_auto_rows_4(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:50px;grid-template-columns:50px;grid-auto-rows:8px;grid-auto-flow:row;align-items:stretch'><div id=i0 style='height:8px'></div><div id=i1 style='height:8px'></div><div id=i2 style='height:8px'></div><div id=i3 style='height:8px'></div><div id=i4 style='height:8px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("i0")->y);
    TEST_ASSERT_EQUAL(8, lot52_node("i0")->h);
    TEST_ASSERT_EQUAL(8, lot52_node("i1")->y);
    TEST_ASSERT_EQUAL(8, lot52_node("i1")->h);
    TEST_ASSERT_EQUAL(16, lot52_node("i2")->y);
    TEST_ASSERT_EQUAL(8, lot52_node("i2")->h);
    TEST_ASSERT_EQUAL(24, lot52_node("i3")->y);
    TEST_ASSERT_EQUAL(8, lot52_node("i3")->h);
    TEST_ASSERT_EQUAL(32, lot52_node("i4")->y);
    TEST_ASSERT_EQUAL(8, lot52_node("i4")->h);
    TEST_ASSERT_EQUAL(40, lot52_node("g")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_auto_rows_5(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:50px;grid-template-columns:50px;grid-auto-rows:10px;grid-auto-flow:row;align-items:stretch'><div id=i0 style='height:10px'></div><div id=i1 style='height:10px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("i0")->y);
    TEST_ASSERT_EQUAL(10, lot52_node("i0")->h);
    TEST_ASSERT_EQUAL(10, lot52_node("i1")->y);
    TEST_ASSERT_EQUAL(10, lot52_node("i1")->h);
    TEST_ASSERT_EQUAL(20, lot52_node("g")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_auto_rows_6(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:50px;grid-template-columns:50px;grid-auto-rows:10px;grid-auto-flow:row;align-items:stretch'><div id=i0 style='height:10px'></div><div id=i1 style='height:10px'></div><div id=i2 style='height:10px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("i0")->y);
    TEST_ASSERT_EQUAL(10, lot52_node("i0")->h);
    TEST_ASSERT_EQUAL(10, lot52_node("i1")->y);
    TEST_ASSERT_EQUAL(10, lot52_node("i1")->h);
    TEST_ASSERT_EQUAL(20, lot52_node("i2")->y);
    TEST_ASSERT_EQUAL(10, lot52_node("i2")->h);
    TEST_ASSERT_EQUAL(30, lot52_node("g")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_auto_rows_7(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:50px;grid-template-columns:50px;grid-auto-rows:10px;grid-auto-flow:row;align-items:stretch'><div id=i0 style='height:10px'></div><div id=i1 style='height:10px'></div><div id=i2 style='height:10px'></div><div id=i3 style='height:10px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("i0")->y);
    TEST_ASSERT_EQUAL(10, lot52_node("i0")->h);
    TEST_ASSERT_EQUAL(10, lot52_node("i1")->y);
    TEST_ASSERT_EQUAL(10, lot52_node("i1")->h);
    TEST_ASSERT_EQUAL(20, lot52_node("i2")->y);
    TEST_ASSERT_EQUAL(10, lot52_node("i2")->h);
    TEST_ASSERT_EQUAL(30, lot52_node("i3")->y);
    TEST_ASSERT_EQUAL(10, lot52_node("i3")->h);
    TEST_ASSERT_EQUAL(40, lot52_node("g")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_auto_rows_8(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:50px;grid-template-columns:50px;grid-auto-rows:10px;grid-auto-flow:row;align-items:stretch'><div id=i0 style='height:10px'></div><div id=i1 style='height:10px'></div><div id=i2 style='height:10px'></div><div id=i3 style='height:10px'></div><div id=i4 style='height:10px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("i0")->y);
    TEST_ASSERT_EQUAL(10, lot52_node("i0")->h);
    TEST_ASSERT_EQUAL(10, lot52_node("i1")->y);
    TEST_ASSERT_EQUAL(10, lot52_node("i1")->h);
    TEST_ASSERT_EQUAL(20, lot52_node("i2")->y);
    TEST_ASSERT_EQUAL(10, lot52_node("i2")->h);
    TEST_ASSERT_EQUAL(30, lot52_node("i3")->y);
    TEST_ASSERT_EQUAL(10, lot52_node("i3")->h);
    TEST_ASSERT_EQUAL(40, lot52_node("i4")->y);
    TEST_ASSERT_EQUAL(10, lot52_node("i4")->h);
    TEST_ASSERT_EQUAL(50, lot52_node("g")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_auto_rows_9(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:50px;grid-template-columns:50px;grid-auto-rows:12px;grid-auto-flow:row;align-items:stretch'><div id=i0 style='height:12px'></div><div id=i1 style='height:12px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("i0")->y);
    TEST_ASSERT_EQUAL(12, lot52_node("i0")->h);
    TEST_ASSERT_EQUAL(12, lot52_node("i1")->y);
    TEST_ASSERT_EQUAL(12, lot52_node("i1")->h);
    TEST_ASSERT_EQUAL(24, lot52_node("g")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_auto_rows_10(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:50px;grid-template-columns:50px;grid-auto-rows:12px;grid-auto-flow:row;align-items:stretch'><div id=i0 style='height:12px'></div><div id=i1 style='height:12px'></div><div id=i2 style='height:12px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("i0")->y);
    TEST_ASSERT_EQUAL(12, lot52_node("i0")->h);
    TEST_ASSERT_EQUAL(12, lot52_node("i1")->y);
    TEST_ASSERT_EQUAL(12, lot52_node("i1")->h);
    TEST_ASSERT_EQUAL(24, lot52_node("i2")->y);
    TEST_ASSERT_EQUAL(12, lot52_node("i2")->h);
    TEST_ASSERT_EQUAL(36, lot52_node("g")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_auto_rows_11(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:50px;grid-template-columns:50px;grid-auto-rows:12px;grid-auto-flow:row;align-items:stretch'><div id=i0 style='height:12px'></div><div id=i1 style='height:12px'></div><div id=i2 style='height:12px'></div><div id=i3 style='height:12px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("i0")->y);
    TEST_ASSERT_EQUAL(12, lot52_node("i0")->h);
    TEST_ASSERT_EQUAL(12, lot52_node("i1")->y);
    TEST_ASSERT_EQUAL(12, lot52_node("i1")->h);
    TEST_ASSERT_EQUAL(24, lot52_node("i2")->y);
    TEST_ASSERT_EQUAL(12, lot52_node("i2")->h);
    TEST_ASSERT_EQUAL(36, lot52_node("i3")->y);
    TEST_ASSERT_EQUAL(12, lot52_node("i3")->h);
    TEST_ASSERT_EQUAL(48, lot52_node("g")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_auto_rows_12(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:50px;grid-template-columns:50px;grid-auto-rows:12px;grid-auto-flow:row;align-items:stretch'><div id=i0 style='height:12px'></div><div id=i1 style='height:12px'></div><div id=i2 style='height:12px'></div><div id=i3 style='height:12px'></div><div id=i4 style='height:12px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("i0")->y);
    TEST_ASSERT_EQUAL(12, lot52_node("i0")->h);
    TEST_ASSERT_EQUAL(12, lot52_node("i1")->y);
    TEST_ASSERT_EQUAL(12, lot52_node("i1")->h);
    TEST_ASSERT_EQUAL(24, lot52_node("i2")->y);
    TEST_ASSERT_EQUAL(12, lot52_node("i2")->h);
    TEST_ASSERT_EQUAL(36, lot52_node("i3")->y);
    TEST_ASSERT_EQUAL(12, lot52_node("i3")->h);
    TEST_ASSERT_EQUAL(48, lot52_node("i4")->y);
    TEST_ASSERT_EQUAL(12, lot52_node("i4")->h);
    TEST_ASSERT_EQUAL(60, lot52_node("g")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_auto_rows_13(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:50px;grid-template-columns:50px;grid-auto-rows:14px;grid-auto-flow:row;align-items:stretch'><div id=i0 style='height:14px'></div><div id=i1 style='height:14px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("i0")->y);
    TEST_ASSERT_EQUAL(14, lot52_node("i0")->h);
    TEST_ASSERT_EQUAL(14, lot52_node("i1")->y);
    TEST_ASSERT_EQUAL(14, lot52_node("i1")->h);
    TEST_ASSERT_EQUAL(28, lot52_node("g")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_auto_rows_14(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:50px;grid-template-columns:50px;grid-auto-rows:14px;grid-auto-flow:row;align-items:stretch'><div id=i0 style='height:14px'></div><div id=i1 style='height:14px'></div><div id=i2 style='height:14px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("i0")->y);
    TEST_ASSERT_EQUAL(14, lot52_node("i0")->h);
    TEST_ASSERT_EQUAL(14, lot52_node("i1")->y);
    TEST_ASSERT_EQUAL(14, lot52_node("i1")->h);
    TEST_ASSERT_EQUAL(28, lot52_node("i2")->y);
    TEST_ASSERT_EQUAL(14, lot52_node("i2")->h);
    TEST_ASSERT_EQUAL(42, lot52_node("g")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_auto_rows_15(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:50px;grid-template-columns:50px;grid-auto-rows:14px;grid-auto-flow:row;align-items:stretch'><div id=i0 style='height:14px'></div><div id=i1 style='height:14px'></div><div id=i2 style='height:14px'></div><div id=i3 style='height:14px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("i0")->y);
    TEST_ASSERT_EQUAL(14, lot52_node("i0")->h);
    TEST_ASSERT_EQUAL(14, lot52_node("i1")->y);
    TEST_ASSERT_EQUAL(14, lot52_node("i1")->h);
    TEST_ASSERT_EQUAL(28, lot52_node("i2")->y);
    TEST_ASSERT_EQUAL(14, lot52_node("i2")->h);
    TEST_ASSERT_EQUAL(42, lot52_node("i3")->y);
    TEST_ASSERT_EQUAL(14, lot52_node("i3")->h);
    TEST_ASSERT_EQUAL(56, lot52_node("g")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_auto_rows_16(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:50px;grid-template-columns:50px;grid-auto-rows:14px;grid-auto-flow:row;align-items:stretch'><div id=i0 style='height:14px'></div><div id=i1 style='height:14px'></div><div id=i2 style='height:14px'></div><div id=i3 style='height:14px'></div><div id=i4 style='height:14px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("i0")->y);
    TEST_ASSERT_EQUAL(14, lot52_node("i0")->h);
    TEST_ASSERT_EQUAL(14, lot52_node("i1")->y);
    TEST_ASSERT_EQUAL(14, lot52_node("i1")->h);
    TEST_ASSERT_EQUAL(28, lot52_node("i2")->y);
    TEST_ASSERT_EQUAL(14, lot52_node("i2")->h);
    TEST_ASSERT_EQUAL(42, lot52_node("i3")->y);
    TEST_ASSERT_EQUAL(14, lot52_node("i3")->h);
    TEST_ASSERT_EQUAL(56, lot52_node("i4")->y);
    TEST_ASSERT_EQUAL(14, lot52_node("i4")->h);
    TEST_ASSERT_EQUAL(70, lot52_node("g")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_auto_rows_17(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:50px;grid-template-columns:50px;grid-auto-rows:16px;grid-auto-flow:row;align-items:stretch'><div id=i0 style='height:16px'></div><div id=i1 style='height:16px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("i0")->y);
    TEST_ASSERT_EQUAL(16, lot52_node("i0")->h);
    TEST_ASSERT_EQUAL(16, lot52_node("i1")->y);
    TEST_ASSERT_EQUAL(16, lot52_node("i1")->h);
    TEST_ASSERT_EQUAL(32, lot52_node("g")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_auto_rows_18(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:50px;grid-template-columns:50px;grid-auto-rows:16px;grid-auto-flow:row;align-items:stretch'><div id=i0 style='height:16px'></div><div id=i1 style='height:16px'></div><div id=i2 style='height:16px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("i0")->y);
    TEST_ASSERT_EQUAL(16, lot52_node("i0")->h);
    TEST_ASSERT_EQUAL(16, lot52_node("i1")->y);
    TEST_ASSERT_EQUAL(16, lot52_node("i1")->h);
    TEST_ASSERT_EQUAL(32, lot52_node("i2")->y);
    TEST_ASSERT_EQUAL(16, lot52_node("i2")->h);
    TEST_ASSERT_EQUAL(48, lot52_node("g")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_auto_rows_19(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:50px;grid-template-columns:50px;grid-auto-rows:16px;grid-auto-flow:row;align-items:stretch'><div id=i0 style='height:16px'></div><div id=i1 style='height:16px'></div><div id=i2 style='height:16px'></div><div id=i3 style='height:16px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("i0")->y);
    TEST_ASSERT_EQUAL(16, lot52_node("i0")->h);
    TEST_ASSERT_EQUAL(16, lot52_node("i1")->y);
    TEST_ASSERT_EQUAL(16, lot52_node("i1")->h);
    TEST_ASSERT_EQUAL(32, lot52_node("i2")->y);
    TEST_ASSERT_EQUAL(16, lot52_node("i2")->h);
    TEST_ASSERT_EQUAL(48, lot52_node("i3")->y);
    TEST_ASSERT_EQUAL(16, lot52_node("i3")->h);
    TEST_ASSERT_EQUAL(64, lot52_node("g")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_auto_rows_20(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:50px;grid-template-columns:50px;grid-auto-rows:16px;grid-auto-flow:row;align-items:stretch'><div id=i0 style='height:16px'></div><div id=i1 style='height:16px'></div><div id=i2 style='height:16px'></div><div id=i3 style='height:16px'></div><div id=i4 style='height:16px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("i0")->y);
    TEST_ASSERT_EQUAL(16, lot52_node("i0")->h);
    TEST_ASSERT_EQUAL(16, lot52_node("i1")->y);
    TEST_ASSERT_EQUAL(16, lot52_node("i1")->h);
    TEST_ASSERT_EQUAL(32, lot52_node("i2")->y);
    TEST_ASSERT_EQUAL(16, lot52_node("i2")->h);
    TEST_ASSERT_EQUAL(48, lot52_node("i3")->y);
    TEST_ASSERT_EQUAL(16, lot52_node("i3")->h);
    TEST_ASSERT_EQUAL(64, lot52_node("i4")->y);
    TEST_ASSERT_EQUAL(16, lot52_node("i4")->h);
    TEST_ASSERT_EQUAL(80, lot52_node("g")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_auto_rows_21(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:50px;grid-template-columns:50px;grid-auto-rows:18px;grid-auto-flow:row;align-items:stretch'><div id=i0 style='height:18px'></div><div id=i1 style='height:18px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("i0")->y);
    TEST_ASSERT_EQUAL(18, lot52_node("i0")->h);
    TEST_ASSERT_EQUAL(18, lot52_node("i1")->y);
    TEST_ASSERT_EQUAL(18, lot52_node("i1")->h);
    TEST_ASSERT_EQUAL(36, lot52_node("g")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_auto_rows_22(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:50px;grid-template-columns:50px;grid-auto-rows:18px;grid-auto-flow:row;align-items:stretch'><div id=i0 style='height:18px'></div><div id=i1 style='height:18px'></div><div id=i2 style='height:18px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("i0")->y);
    TEST_ASSERT_EQUAL(18, lot52_node("i0")->h);
    TEST_ASSERT_EQUAL(18, lot52_node("i1")->y);
    TEST_ASSERT_EQUAL(18, lot52_node("i1")->h);
    TEST_ASSERT_EQUAL(36, lot52_node("i2")->y);
    TEST_ASSERT_EQUAL(18, lot52_node("i2")->h);
    TEST_ASSERT_EQUAL(54, lot52_node("g")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_auto_rows_23(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:50px;grid-template-columns:50px;grid-auto-rows:18px;grid-auto-flow:row;align-items:stretch'><div id=i0 style='height:18px'></div><div id=i1 style='height:18px'></div><div id=i2 style='height:18px'></div><div id=i3 style='height:18px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("i0")->y);
    TEST_ASSERT_EQUAL(18, lot52_node("i0")->h);
    TEST_ASSERT_EQUAL(18, lot52_node("i1")->y);
    TEST_ASSERT_EQUAL(18, lot52_node("i1")->h);
    TEST_ASSERT_EQUAL(36, lot52_node("i2")->y);
    TEST_ASSERT_EQUAL(18, lot52_node("i2")->h);
    TEST_ASSERT_EQUAL(54, lot52_node("i3")->y);
    TEST_ASSERT_EQUAL(18, lot52_node("i3")->h);
    TEST_ASSERT_EQUAL(72, lot52_node("g")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_auto_rows_24(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:50px;grid-template-columns:50px;grid-auto-rows:18px;grid-auto-flow:row;align-items:stretch'><div id=i0 style='height:18px'></div><div id=i1 style='height:18px'></div><div id=i2 style='height:18px'></div><div id=i3 style='height:18px'></div><div id=i4 style='height:18px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("i0")->y);
    TEST_ASSERT_EQUAL(18, lot52_node("i0")->h);
    TEST_ASSERT_EQUAL(18, lot52_node("i1")->y);
    TEST_ASSERT_EQUAL(18, lot52_node("i1")->h);
    TEST_ASSERT_EQUAL(36, lot52_node("i2")->y);
    TEST_ASSERT_EQUAL(18, lot52_node("i2")->h);
    TEST_ASSERT_EQUAL(54, lot52_node("i3")->y);
    TEST_ASSERT_EQUAL(18, lot52_node("i3")->h);
    TEST_ASSERT_EQUAL(72, lot52_node("i4")->y);
    TEST_ASSERT_EQUAL(18, lot52_node("i4")->h);
    TEST_ASSERT_EQUAL(90, lot52_node("g")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_auto_rows_25(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:50px;grid-template-columns:50px;grid-auto-rows:20px;grid-auto-flow:row;align-items:stretch'><div id=i0 style='height:20px'></div><div id=i1 style='height:20px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("i0")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("i0")->h);
    TEST_ASSERT_EQUAL(20, lot52_node("i1")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("i1")->h);
    TEST_ASSERT_EQUAL(40, lot52_node("g")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_auto_rows_26(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:50px;grid-template-columns:50px;grid-auto-rows:20px;grid-auto-flow:row;align-items:stretch'><div id=i0 style='height:20px'></div><div id=i1 style='height:20px'></div><div id=i2 style='height:20px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("i0")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("i0")->h);
    TEST_ASSERT_EQUAL(20, lot52_node("i1")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("i1")->h);
    TEST_ASSERT_EQUAL(40, lot52_node("i2")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("i2")->h);
    TEST_ASSERT_EQUAL(60, lot52_node("g")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_auto_rows_27(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:50px;grid-template-columns:50px;grid-auto-rows:20px;grid-auto-flow:row;align-items:stretch'><div id=i0 style='height:20px'></div><div id=i1 style='height:20px'></div><div id=i2 style='height:20px'></div><div id=i3 style='height:20px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("i0")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("i0")->h);
    TEST_ASSERT_EQUAL(20, lot52_node("i1")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("i1")->h);
    TEST_ASSERT_EQUAL(40, lot52_node("i2")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("i2")->h);
    TEST_ASSERT_EQUAL(60, lot52_node("i3")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("i3")->h);
    TEST_ASSERT_EQUAL(80, lot52_node("g")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_auto_rows_28(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:50px;grid-template-columns:50px;grid-auto-rows:20px;grid-auto-flow:row;align-items:stretch'><div id=i0 style='height:20px'></div><div id=i1 style='height:20px'></div><div id=i2 style='height:20px'></div><div id=i3 style='height:20px'></div><div id=i4 style='height:20px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("i0")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("i0")->h);
    TEST_ASSERT_EQUAL(20, lot52_node("i1")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("i1")->h);
    TEST_ASSERT_EQUAL(40, lot52_node("i2")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("i2")->h);
    TEST_ASSERT_EQUAL(60, lot52_node("i3")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("i3")->h);
    TEST_ASSERT_EQUAL(80, lot52_node("i4")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("i4")->h);
    TEST_ASSERT_EQUAL(100, lot52_node("g")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_auto_rows_29(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:50px;grid-template-columns:50px;grid-auto-rows:24px;grid-auto-flow:row;align-items:stretch'><div id=i0 style='height:24px'></div><div id=i1 style='height:24px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("i0")->y);
    TEST_ASSERT_EQUAL(24, lot52_node("i0")->h);
    TEST_ASSERT_EQUAL(24, lot52_node("i1")->y);
    TEST_ASSERT_EQUAL(24, lot52_node("i1")->h);
    TEST_ASSERT_EQUAL(48, lot52_node("g")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_auto_rows_30(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:50px;grid-template-columns:50px;grid-auto-rows:24px;grid-auto-flow:row;align-items:stretch'><div id=i0 style='height:24px'></div><div id=i1 style='height:24px'></div><div id=i2 style='height:24px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("i0")->y);
    TEST_ASSERT_EQUAL(24, lot52_node("i0")->h);
    TEST_ASSERT_EQUAL(24, lot52_node("i1")->y);
    TEST_ASSERT_EQUAL(24, lot52_node("i1")->h);
    TEST_ASSERT_EQUAL(48, lot52_node("i2")->y);
    TEST_ASSERT_EQUAL(24, lot52_node("i2")->h);
    TEST_ASSERT_EQUAL(72, lot52_node("g")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_auto_rows_31(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:50px;grid-template-columns:50px;grid-auto-rows:24px;grid-auto-flow:row;align-items:stretch'><div id=i0 style='height:24px'></div><div id=i1 style='height:24px'></div><div id=i2 style='height:24px'></div><div id=i3 style='height:24px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("i0")->y);
    TEST_ASSERT_EQUAL(24, lot52_node("i0")->h);
    TEST_ASSERT_EQUAL(24, lot52_node("i1")->y);
    TEST_ASSERT_EQUAL(24, lot52_node("i1")->h);
    TEST_ASSERT_EQUAL(48, lot52_node("i2")->y);
    TEST_ASSERT_EQUAL(24, lot52_node("i2")->h);
    TEST_ASSERT_EQUAL(72, lot52_node("i3")->y);
    TEST_ASSERT_EQUAL(24, lot52_node("i3")->h);
    TEST_ASSERT_EQUAL(96, lot52_node("g")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_auto_rows_32(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:50px;grid-template-columns:50px;grid-auto-rows:24px;grid-auto-flow:row;align-items:stretch'><div id=i0 style='height:24px'></div><div id=i1 style='height:24px'></div><div id=i2 style='height:24px'></div><div id=i3 style='height:24px'></div><div id=i4 style='height:24px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("i0")->y);
    TEST_ASSERT_EQUAL(24, lot52_node("i0")->h);
    TEST_ASSERT_EQUAL(24, lot52_node("i1")->y);
    TEST_ASSERT_EQUAL(24, lot52_node("i1")->h);
    TEST_ASSERT_EQUAL(48, lot52_node("i2")->y);
    TEST_ASSERT_EQUAL(24, lot52_node("i2")->h);
    TEST_ASSERT_EQUAL(72, lot52_node("i3")->y);
    TEST_ASSERT_EQUAL(24, lot52_node("i3")->h);
    TEST_ASSERT_EQUAL(96, lot52_node("i4")->y);
    TEST_ASSERT_EQUAL(24, lot52_node("i4")->h);
    TEST_ASSERT_EQUAL(120, lot52_node("g")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_auto_cols_33(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:90px;height:20px;grid-template-rows:20px;grid-auto-columns:15px;grid-auto-flow:column;align-items:stretch'><div id=i0></div><div id=i1></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("i0")->x);
    TEST_ASSERT_EQUAL(15, lot52_node("i0")->w);
    TEST_ASSERT_EQUAL(15, lot52_node("i1")->x);
    TEST_ASSERT_EQUAL(15, lot52_node("i1")->w);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_auto_cols_34(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:90px;height:20px;grid-template-rows:20px;grid-auto-columns:15px;grid-auto-flow:column;align-items:stretch'><div id=i0></div><div id=i1></div><div id=i2></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("i0")->x);
    TEST_ASSERT_EQUAL(15, lot52_node("i0")->w);
    TEST_ASSERT_EQUAL(15, lot52_node("i1")->x);
    TEST_ASSERT_EQUAL(15, lot52_node("i1")->w);
    TEST_ASSERT_EQUAL(30, lot52_node("i2")->x);
    TEST_ASSERT_EQUAL(15, lot52_node("i2")->w);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_auto_cols_35(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:90px;height:20px;grid-template-rows:20px;grid-auto-columns:15px;grid-auto-flow:column;align-items:stretch'><div id=i0></div><div id=i1></div><div id=i2></div><div id=i3></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("i0")->x);
    TEST_ASSERT_EQUAL(15, lot52_node("i0")->w);
    TEST_ASSERT_EQUAL(15, lot52_node("i1")->x);
    TEST_ASSERT_EQUAL(15, lot52_node("i1")->w);
    TEST_ASSERT_EQUAL(30, lot52_node("i2")->x);
    TEST_ASSERT_EQUAL(15, lot52_node("i2")->w);
    TEST_ASSERT_EQUAL(45, lot52_node("i3")->x);
    TEST_ASSERT_EQUAL(15, lot52_node("i3")->w);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_auto_cols_36(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:90px;height:20px;grid-template-rows:20px;grid-auto-columns:20px;grid-auto-flow:column;align-items:stretch'><div id=i0></div><div id=i1></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("i0")->x);
    TEST_ASSERT_EQUAL(20, lot52_node("i0")->w);
    TEST_ASSERT_EQUAL(20, lot52_node("i1")->x);
    TEST_ASSERT_EQUAL(20, lot52_node("i1")->w);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_auto_cols_37(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:90px;height:20px;grid-template-rows:20px;grid-auto-columns:20px;grid-auto-flow:column;align-items:stretch'><div id=i0></div><div id=i1></div><div id=i2></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("i0")->x);
    TEST_ASSERT_EQUAL(20, lot52_node("i0")->w);
    TEST_ASSERT_EQUAL(20, lot52_node("i1")->x);
    TEST_ASSERT_EQUAL(20, lot52_node("i1")->w);
    TEST_ASSERT_EQUAL(40, lot52_node("i2")->x);
    TEST_ASSERT_EQUAL(20, lot52_node("i2")->w);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_auto_cols_38(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:90px;height:20px;grid-template-rows:20px;grid-auto-columns:20px;grid-auto-flow:column;align-items:stretch'><div id=i0></div><div id=i1></div><div id=i2></div><div id=i3></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("i0")->x);
    TEST_ASSERT_EQUAL(20, lot52_node("i0")->w);
    TEST_ASSERT_EQUAL(20, lot52_node("i1")->x);
    TEST_ASSERT_EQUAL(20, lot52_node("i1")->w);
    TEST_ASSERT_EQUAL(40, lot52_node("i2")->x);
    TEST_ASSERT_EQUAL(20, lot52_node("i2")->w);
    TEST_ASSERT_EQUAL(60, lot52_node("i3")->x);
    TEST_ASSERT_EQUAL(20, lot52_node("i3")->w);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_auto_cols_39(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:90px;height:20px;grid-template-rows:20px;grid-auto-columns:25px;grid-auto-flow:column;align-items:stretch'><div id=i0></div><div id=i1></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("i0")->x);
    TEST_ASSERT_EQUAL(25, lot52_node("i0")->w);
    TEST_ASSERT_EQUAL(25, lot52_node("i1")->x);
    TEST_ASSERT_EQUAL(25, lot52_node("i1")->w);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_auto_cols_40(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:90px;height:20px;grid-template-rows:20px;grid-auto-columns:25px;grid-auto-flow:column;align-items:stretch'><div id=i0></div><div id=i1></div><div id=i2></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("i0")->x);
    TEST_ASSERT_EQUAL(25, lot52_node("i0")->w);
    TEST_ASSERT_EQUAL(25, lot52_node("i1")->x);
    TEST_ASSERT_EQUAL(25, lot52_node("i1")->w);
    TEST_ASSERT_EQUAL(50, lot52_node("i2")->x);
    TEST_ASSERT_EQUAL(25, lot52_node("i2")->w);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_auto_cols_41(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:100px;height:20px;grid-template-rows:20px;grid-auto-columns:25px;grid-auto-flow:column;align-items:stretch'><div id=i0></div><div id=i1></div><div id=i2></div><div id=i3></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("i0")->x);
    TEST_ASSERT_EQUAL(25, lot52_node("i0")->w);
    TEST_ASSERT_EQUAL(25, lot52_node("i1")->x);
    TEST_ASSERT_EQUAL(25, lot52_node("i1")->w);
    TEST_ASSERT_EQUAL(50, lot52_node("i2")->x);
    TEST_ASSERT_EQUAL(25, lot52_node("i2")->w);
    TEST_ASSERT_EQUAL(75, lot52_node("i3")->x);
    TEST_ASSERT_EQUAL(25, lot52_node("i3")->w);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_auto_cols_42(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:90px;height:20px;grid-template-rows:20px;grid-auto-columns:30px;grid-auto-flow:column;align-items:stretch'><div id=i0></div><div id=i1></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("i0")->x);
    TEST_ASSERT_EQUAL(30, lot52_node("i0")->w);
    TEST_ASSERT_EQUAL(30, lot52_node("i1")->x);
    TEST_ASSERT_EQUAL(30, lot52_node("i1")->w);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_auto_cols_43(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:90px;height:20px;grid-template-rows:20px;grid-auto-columns:30px;grid-auto-flow:column;align-items:stretch'><div id=i0></div><div id=i1></div><div id=i2></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("i0")->x);
    TEST_ASSERT_EQUAL(30, lot52_node("i0")->w);
    TEST_ASSERT_EQUAL(30, lot52_node("i1")->x);
    TEST_ASSERT_EQUAL(30, lot52_node("i1")->w);
    TEST_ASSERT_EQUAL(60, lot52_node("i2")->x);
    TEST_ASSERT_EQUAL(30, lot52_node("i2")->w);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_auto_cols_44(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:120px;height:20px;grid-template-rows:20px;grid-auto-columns:30px;grid-auto-flow:column;align-items:stretch'><div id=i0></div><div id=i1></div><div id=i2></div><div id=i3></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("i0")->x);
    TEST_ASSERT_EQUAL(30, lot52_node("i0")->w);
    TEST_ASSERT_EQUAL(30, lot52_node("i1")->x);
    TEST_ASSERT_EQUAL(30, lot52_node("i1")->w);
    TEST_ASSERT_EQUAL(60, lot52_node("i2")->x);
    TEST_ASSERT_EQUAL(30, lot52_node("i2")->w);
    TEST_ASSERT_EQUAL(90, lot52_node("i3")->x);
    TEST_ASSERT_EQUAL(30, lot52_node("i3")->w);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_minmax_tpl_45(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:100px;grid-template-columns:minmax(20px,1fr) 1fr;align-items:stretch'><div id=a style='height:8px'></div><div id=b style='height:8px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(50, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(50, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_minmax_tpl_46(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:100px;grid-template-columns:minmax(20px,1fr) minmax(20px,1fr);align-items:stretch'><div id=a style='height:8px'></div><div id=b style='height:8px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(50, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(50, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_minmax_tpl_47(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:100px;grid-template-columns:minmax(30px,1fr) 1fr;align-items:stretch'><div id=a style='height:8px'></div><div id=b style='height:8px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(50, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(50, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_minmax_tpl_48(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:100px;grid-template-columns:minmax(30px,1fr) minmax(30px,1fr);align-items:stretch'><div id=a style='height:8px'></div><div id=b style='height:8px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(50, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(50, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_minmax_tpl_49(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:120px;grid-template-columns:minmax(40px,1fr) 1fr;align-items:stretch'><div id=a style='height:8px'></div><div id=b style='height:8px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(60, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(60, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_minmax_tpl_50(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:120px;grid-template-columns:minmax(40px,1fr) minmax(40px,1fr);align-items:stretch'><div id=a style='height:8px'></div><div id=b style='height:8px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(60, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(60, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_minmax_tpl_51(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:120px;grid-template-columns:minmax(50px,1fr) 1fr;align-items:stretch'><div id=a style='height:8px'></div><div id=b style='height:8px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(60, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(60, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_minmax_tpl_52(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:120px;grid-template-columns:minmax(50px,1fr) minmax(50px,1fr);align-items:stretch'><div id=a style='height:8px'></div><div id=b style='height:8px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(60, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(60, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_minmax_tpl_53(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:140px;grid-template-columns:minmax(60px,1fr) 1fr;align-items:stretch'><div id=a style='height:8px'></div><div id=b style='height:8px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(70, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(70, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_minmax_tpl_54(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:140px;grid-template-columns:minmax(60px,1fr) minmax(60px,1fr);align-items:stretch'><div id=a style='height:8px'></div><div id=b style='height:8px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(70, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(70, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_minmax_tpl_55(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:100px;grid-template-columns:minmax(70px,1fr) 1fr;align-items:stretch'><div id=a style='height:8px'></div><div id=b style='height:8px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(70, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(30, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_minmax_tpl_56(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:140px;grid-template-columns:minmax(70px,1fr) minmax(70px,1fr);align-items:stretch'><div id=a style='height:8px'></div><div id=b style='height:8px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(70, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(70, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_minmax_tpl_57(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:120px;grid-template-columns:minmax(80px,1fr) 1fr;align-items:stretch'><div id=a style='height:8px'></div><div id=b style='height:8px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(80, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(40, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_minmax_tpl_58(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:160px;grid-template-columns:minmax(80px,1fr) minmax(80px,1fr);align-items:stretch'><div id=a style='height:8px'></div><div id=b style='height:8px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(80, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(80, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_auto_mm_59(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;grid-template-columns:40px;grid-auto-rows:minmax(10px,20px);align-items:stretch'><div id=i0 style='height:10px'></div><div id=i1 style='height:10px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("i0")->y);
    TEST_ASSERT_EQUAL(10, lot52_node("i1")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_auto_mm_60(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;grid-template-columns:40px;grid-auto-rows:minmax(10px,20px);align-items:stretch'><div id=i0 style='height:10px'></div><div id=i1 style='height:10px'></div><div id=i2 style='height:10px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("i0")->y);
    TEST_ASSERT_EQUAL(10, lot52_node("i1")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("i2")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_auto_mm_61(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;grid-template-columns:40px;grid-auto-rows:minmax(12px,24px);align-items:stretch'><div id=i0 style='height:12px'></div><div id=i1 style='height:12px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("i0")->y);
    TEST_ASSERT_EQUAL(12, lot52_node("i1")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_auto_mm_62(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;grid-template-columns:40px;grid-auto-rows:minmax(12px,24px);align-items:stretch'><div id=i0 style='height:12px'></div><div id=i1 style='height:12px'></div><div id=i2 style='height:12px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("i0")->y);
    TEST_ASSERT_EQUAL(12, lot52_node("i1")->y);
    TEST_ASSERT_EQUAL(24, lot52_node("i2")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_auto_mm_63(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;grid-template-columns:40px;grid-auto-rows:minmax(8px,16px);align-items:stretch'><div id=i0 style='height:8px'></div><div id=i1 style='height:8px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("i0")->y);
    TEST_ASSERT_EQUAL(8, lot52_node("i1")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_auto_mm_64(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;grid-template-columns:40px;grid-auto-rows:minmax(8px,16px);align-items:stretch'><div id=i0 style='height:8px'></div><div id=i1 style='height:8px'></div><div id=i2 style='height:8px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("i0")->y);
    TEST_ASSERT_EQUAL(8, lot52_node("i1")->y);
    TEST_ASSERT_EQUAL(16, lot52_node("i2")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_auto_mm_65(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;grid-template-columns:40px;grid-auto-rows:minmax(15px,30px);align-items:stretch'><div id=i0 style='height:15px'></div><div id=i1 style='height:15px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("i0")->y);
    TEST_ASSERT_EQUAL(15, lot52_node("i1")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_auto_mm_66(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;grid-template-columns:40px;grid-auto-rows:minmax(15px,30px);align-items:stretch'><div id=i0 style='height:15px'></div><div id=i1 style='height:15px'></div><div id=i2 style='height:15px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("i0")->y);
    TEST_ASSERT_EQUAL(15, lot52_node("i1")->y);
    TEST_ASSERT_EQUAL(30, lot52_node("i2")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_auto_mm_67(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;grid-template-columns:40px;grid-auto-rows:minmax(20px,40px);align-items:stretch'><div id=i0 style='height:20px'></div><div id=i1 style='height:20px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("i0")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("i1")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_auto_mm_68(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;grid-template-columns:40px;grid-auto-rows:minmax(20px,40px);align-items:stretch'><div id=i0 style='height:20px'></div><div id=i1 style='height:20px'></div><div id=i2 style='height:20px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("i0")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("i1")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("i2")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_mixed_rows_69(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;grid-template-columns:40px;grid-template-rows:10px;grid-auto-rows:8px;align-items:stretch'><div id=a style='height:10px'></div><div id=b style='height:8px'></div><div id=c style='height:8px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(18, lot52_node("c")->y);
    TEST_ASSERT_EQUAL(26, lot52_node("g")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_mixed_rows_70(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;grid-template-columns:40px;grid-template-rows:10px;grid-auto-rows:12px;align-items:stretch'><div id=a style='height:10px'></div><div id=b style='height:12px'></div><div id=c style='height:12px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(22, lot52_node("c")->y);
    TEST_ASSERT_EQUAL(34, lot52_node("g")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_mixed_rows_71(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;grid-template-columns:40px;grid-template-rows:10px;grid-auto-rows:16px;align-items:stretch'><div id=a style='height:10px'></div><div id=b style='height:16px'></div><div id=c style='height:16px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(26, lot52_node("c")->y);
    TEST_ASSERT_EQUAL(42, lot52_node("g")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_mixed_rows_72(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;grid-template-columns:40px;grid-template-rows:14px;grid-auto-rows:8px;align-items:stretch'><div id=a style='height:14px'></div><div id=b style='height:8px'></div><div id=c style='height:8px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(14, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(22, lot52_node("c")->y);
    TEST_ASSERT_EQUAL(30, lot52_node("g")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_mixed_rows_73(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;grid-template-columns:40px;grid-template-rows:14px;grid-auto-rows:12px;align-items:stretch'><div id=a style='height:14px'></div><div id=b style='height:12px'></div><div id=c style='height:12px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(14, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(26, lot52_node("c")->y);
    TEST_ASSERT_EQUAL(38, lot52_node("g")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_mixed_rows_74(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;grid-template-columns:40px;grid-template-rows:14px;grid-auto-rows:16px;align-items:stretch'><div id=a style='height:14px'></div><div id=b style='height:16px'></div><div id=c style='height:16px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(14, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(30, lot52_node("c")->y);
    TEST_ASSERT_EQUAL(46, lot52_node("g")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_mixed_rows_75(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;grid-template-columns:40px;grid-template-rows:18px;grid-auto-rows:8px;align-items:stretch'><div id=a style='height:18px'></div><div id=b style='height:8px'></div><div id=c style='height:8px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(18, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(26, lot52_node("c")->y);
    TEST_ASSERT_EQUAL(34, lot52_node("g")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_mixed_rows_76(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;grid-template-columns:40px;grid-template-rows:18px;grid-auto-rows:12px;align-items:stretch'><div id=a style='height:18px'></div><div id=b style='height:12px'></div><div id=c style='height:12px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(18, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(30, lot52_node("c")->y);
    TEST_ASSERT_EQUAL(42, lot52_node("g")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_mixed_rows_77(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;grid-template-columns:40px;grid-template-rows:18px;grid-auto-rows:16px;align-items:stretch'><div id=a style='height:18px'></div><div id=b style='height:16px'></div><div id=c style='height:16px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(18, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(34, lot52_node("c")->y);
    TEST_ASSERT_EQUAL(50, lot52_node("g")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_ar2_1(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;grid-template-columns:40px;grid-auto-rows:5px;align-items:stretch'><div id=i0 style='height:5px'></div><div id=i1 style='height:5px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("i0")->y);
    TEST_ASSERT_EQUAL(5, lot52_node("i1")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_ar2_2(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;grid-template-columns:40px;grid-auto-rows:5px;align-items:stretch'><div id=i0 style='height:5px'></div><div id=i1 style='height:5px'></div><div id=i2 style='height:5px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("i0")->y);
    TEST_ASSERT_EQUAL(5, lot52_node("i1")->y);
    TEST_ASSERT_EQUAL(10, lot52_node("i2")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_ar2_3(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;grid-template-columns:40px;grid-auto-rows:5px;align-items:stretch'><div id=i0 style='height:5px'></div><div id=i1 style='height:5px'></div><div id=i2 style='height:5px'></div><div id=i3 style='height:5px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("i0")->y);
    TEST_ASSERT_EQUAL(5, lot52_node("i1")->y);
    TEST_ASSERT_EQUAL(10, lot52_node("i2")->y);
    TEST_ASSERT_EQUAL(15, lot52_node("i3")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_ar2_4(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;grid-template-columns:40px;grid-auto-rows:6px;align-items:stretch'><div id=i0 style='height:6px'></div><div id=i1 style='height:6px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("i0")->y);
    TEST_ASSERT_EQUAL(6, lot52_node("i1")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_ar2_5(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;grid-template-columns:40px;grid-auto-rows:6px;align-items:stretch'><div id=i0 style='height:6px'></div><div id=i1 style='height:6px'></div><div id=i2 style='height:6px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("i0")->y);
    TEST_ASSERT_EQUAL(6, lot52_node("i1")->y);
    TEST_ASSERT_EQUAL(12, lot52_node("i2")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_ar2_6(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;grid-template-columns:40px;grid-auto-rows:6px;align-items:stretch'><div id=i0 style='height:6px'></div><div id=i1 style='height:6px'></div><div id=i2 style='height:6px'></div><div id=i3 style='height:6px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("i0")->y);
    TEST_ASSERT_EQUAL(6, lot52_node("i1")->y);
    TEST_ASSERT_EQUAL(12, lot52_node("i2")->y);
    TEST_ASSERT_EQUAL(18, lot52_node("i3")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_ar2_7(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;grid-template-columns:40px;grid-auto-rows:7px;align-items:stretch'><div id=i0 style='height:7px'></div><div id=i1 style='height:7px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("i0")->y);
    TEST_ASSERT_EQUAL(7, lot52_node("i1")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_ar2_8(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;grid-template-columns:40px;grid-auto-rows:7px;align-items:stretch'><div id=i0 style='height:7px'></div><div id=i1 style='height:7px'></div><div id=i2 style='height:7px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("i0")->y);
    TEST_ASSERT_EQUAL(7, lot52_node("i1")->y);
    TEST_ASSERT_EQUAL(14, lot52_node("i2")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_ar2_9(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;grid-template-columns:40px;grid-auto-rows:7px;align-items:stretch'><div id=i0 style='height:7px'></div><div id=i1 style='height:7px'></div><div id=i2 style='height:7px'></div><div id=i3 style='height:7px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("i0")->y);
    TEST_ASSERT_EQUAL(7, lot52_node("i1")->y);
    TEST_ASSERT_EQUAL(14, lot52_node("i2")->y);
    TEST_ASSERT_EQUAL(21, lot52_node("i3")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_ar2_10(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;grid-template-columns:40px;grid-auto-rows:8px;align-items:stretch'><div id=i0 style='height:8px'></div><div id=i1 style='height:8px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("i0")->y);
    TEST_ASSERT_EQUAL(8, lot52_node("i1")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_ar2_11(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;grid-template-columns:40px;grid-auto-rows:8px;align-items:stretch'><div id=i0 style='height:8px'></div><div id=i1 style='height:8px'></div><div id=i2 style='height:8px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("i0")->y);
    TEST_ASSERT_EQUAL(8, lot52_node("i1")->y);
    TEST_ASSERT_EQUAL(16, lot52_node("i2")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_ar2_12(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;grid-template-columns:40px;grid-auto-rows:8px;align-items:stretch'><div id=i0 style='height:8px'></div><div id=i1 style='height:8px'></div><div id=i2 style='height:8px'></div><div id=i3 style='height:8px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("i0")->y);
    TEST_ASSERT_EQUAL(8, lot52_node("i1")->y);
    TEST_ASSERT_EQUAL(16, lot52_node("i2")->y);
    TEST_ASSERT_EQUAL(24, lot52_node("i3")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_ar2_13(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;grid-template-columns:40px;grid-auto-rows:9px;align-items:stretch'><div id=i0 style='height:9px'></div><div id=i1 style='height:9px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("i0")->y);
    TEST_ASSERT_EQUAL(9, lot52_node("i1")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_ar2_14(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;grid-template-columns:40px;grid-auto-rows:9px;align-items:stretch'><div id=i0 style='height:9px'></div><div id=i1 style='height:9px'></div><div id=i2 style='height:9px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("i0")->y);
    TEST_ASSERT_EQUAL(9, lot52_node("i1")->y);
    TEST_ASSERT_EQUAL(18, lot52_node("i2")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_ar2_15(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;grid-template-columns:40px;grid-auto-rows:9px;align-items:stretch'><div id=i0 style='height:9px'></div><div id=i1 style='height:9px'></div><div id=i2 style='height:9px'></div><div id=i3 style='height:9px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("i0")->y);
    TEST_ASSERT_EQUAL(9, lot52_node("i1")->y);
    TEST_ASSERT_EQUAL(18, lot52_node("i2")->y);
    TEST_ASSERT_EQUAL(27, lot52_node("i3")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_ar2_16(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;grid-template-columns:40px;grid-auto-rows:10px;align-items:stretch'><div id=i0 style='height:10px'></div><div id=i1 style='height:10px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("i0")->y);
    TEST_ASSERT_EQUAL(10, lot52_node("i1")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_ar2_17(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;grid-template-columns:40px;grid-auto-rows:10px;align-items:stretch'><div id=i0 style='height:10px'></div><div id=i1 style='height:10px'></div><div id=i2 style='height:10px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("i0")->y);
    TEST_ASSERT_EQUAL(10, lot52_node("i1")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("i2")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_ar2_18(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;grid-template-columns:40px;grid-auto-rows:10px;align-items:stretch'><div id=i0 style='height:10px'></div><div id=i1 style='height:10px'></div><div id=i2 style='height:10px'></div><div id=i3 style='height:10px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("i0")->y);
    TEST_ASSERT_EQUAL(10, lot52_node("i1")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("i2")->y);
    TEST_ASSERT_EQUAL(30, lot52_node("i3")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_ar2_19(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;grid-template-columns:40px;grid-auto-rows:11px;align-items:stretch'><div id=i0 style='height:11px'></div><div id=i1 style='height:11px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("i0")->y);
    TEST_ASSERT_EQUAL(11, lot52_node("i1")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_ar2_20(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;grid-template-columns:40px;grid-auto-rows:11px;align-items:stretch'><div id=i0 style='height:11px'></div><div id=i1 style='height:11px'></div><div id=i2 style='height:11px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("i0")->y);
    TEST_ASSERT_EQUAL(11, lot52_node("i1")->y);
    TEST_ASSERT_EQUAL(22, lot52_node("i2")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_ar2_21(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;grid-template-columns:40px;grid-auto-rows:11px;align-items:stretch'><div id=i0 style='height:11px'></div><div id=i1 style='height:11px'></div><div id=i2 style='height:11px'></div><div id=i3 style='height:11px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("i0")->y);
    TEST_ASSERT_EQUAL(11, lot52_node("i1")->y);
    TEST_ASSERT_EQUAL(22, lot52_node("i2")->y);
    TEST_ASSERT_EQUAL(33, lot52_node("i3")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_ar2_22(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;grid-template-columns:40px;grid-auto-rows:12px;align-items:stretch'><div id=i0 style='height:12px'></div><div id=i1 style='height:12px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("i0")->y);
    TEST_ASSERT_EQUAL(12, lot52_node("i1")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_ar2_23(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;grid-template-columns:40px;grid-auto-rows:12px;align-items:stretch'><div id=i0 style='height:12px'></div><div id=i1 style='height:12px'></div><div id=i2 style='height:12px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("i0")->y);
    TEST_ASSERT_EQUAL(12, lot52_node("i1")->y);
    TEST_ASSERT_EQUAL(24, lot52_node("i2")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_ar2_24(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;grid-template-columns:40px;grid-auto-rows:12px;align-items:stretch'><div id=i0 style='height:12px'></div><div id=i1 style='height:12px'></div><div id=i2 style='height:12px'></div><div id=i3 style='height:12px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("i0")->y);
    TEST_ASSERT_EQUAL(12, lot52_node("i1")->y);
    TEST_ASSERT_EQUAL(24, lot52_node("i2")->y);
    TEST_ASSERT_EQUAL(36, lot52_node("i3")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_ar2_25(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;grid-template-columns:40px;grid-auto-rows:13px;align-items:stretch'><div id=i0 style='height:13px'></div><div id=i1 style='height:13px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("i0")->y);
    TEST_ASSERT_EQUAL(13, lot52_node("i1")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_ar2_26(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;grid-template-columns:40px;grid-auto-rows:13px;align-items:stretch'><div id=i0 style='height:13px'></div><div id=i1 style='height:13px'></div><div id=i2 style='height:13px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("i0")->y);
    TEST_ASSERT_EQUAL(13, lot52_node("i1")->y);
    TEST_ASSERT_EQUAL(26, lot52_node("i2")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_ar2_27(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;grid-template-columns:40px;grid-auto-rows:13px;align-items:stretch'><div id=i0 style='height:13px'></div><div id=i1 style='height:13px'></div><div id=i2 style='height:13px'></div><div id=i3 style='height:13px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("i0")->y);
    TEST_ASSERT_EQUAL(13, lot52_node("i1")->y);
    TEST_ASSERT_EQUAL(26, lot52_node("i2")->y);
    TEST_ASSERT_EQUAL(39, lot52_node("i3")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_ar2_28(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;grid-template-columns:40px;grid-auto-rows:14px;align-items:stretch'><div id=i0 style='height:14px'></div><div id=i1 style='height:14px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("i0")->y);
    TEST_ASSERT_EQUAL(14, lot52_node("i1")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_ar2_29(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;grid-template-columns:40px;grid-auto-rows:14px;align-items:stretch'><div id=i0 style='height:14px'></div><div id=i1 style='height:14px'></div><div id=i2 style='height:14px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("i0")->y);
    TEST_ASSERT_EQUAL(14, lot52_node("i1")->y);
    TEST_ASSERT_EQUAL(28, lot52_node("i2")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_ar2_30(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;grid-template-columns:40px;grid-auto-rows:14px;align-items:stretch'><div id=i0 style='height:14px'></div><div id=i1 style='height:14px'></div><div id=i2 style='height:14px'></div><div id=i3 style='height:14px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("i0")->y);
    TEST_ASSERT_EQUAL(14, lot52_node("i1")->y);
    TEST_ASSERT_EQUAL(28, lot52_node("i2")->y);
    TEST_ASSERT_EQUAL(42, lot52_node("i3")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_ar2_31(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;grid-template-columns:40px;grid-auto-rows:15px;align-items:stretch'><div id=i0 style='height:15px'></div><div id=i1 style='height:15px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("i0")->y);
    TEST_ASSERT_EQUAL(15, lot52_node("i1")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_ar2_32(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;grid-template-columns:40px;grid-auto-rows:15px;align-items:stretch'><div id=i0 style='height:15px'></div><div id=i1 style='height:15px'></div><div id=i2 style='height:15px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("i0")->y);
    TEST_ASSERT_EQUAL(15, lot52_node("i1")->y);
    TEST_ASSERT_EQUAL(30, lot52_node("i2")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_ar2_33(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;grid-template-columns:40px;grid-auto-rows:15px;align-items:stretch'><div id=i0 style='height:15px'></div><div id=i1 style='height:15px'></div><div id=i2 style='height:15px'></div><div id=i3 style='height:15px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("i0")->y);
    TEST_ASSERT_EQUAL(15, lot52_node("i1")->y);
    TEST_ASSERT_EQUAL(30, lot52_node("i2")->y);
    TEST_ASSERT_EQUAL(45, lot52_node("i3")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_ar2_34(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;grid-template-columns:40px;grid-auto-rows:16px;align-items:stretch'><div id=i0 style='height:16px'></div><div id=i1 style='height:16px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("i0")->y);
    TEST_ASSERT_EQUAL(16, lot52_node("i1")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_ar2_35(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;grid-template-columns:40px;grid-auto-rows:16px;align-items:stretch'><div id=i0 style='height:16px'></div><div id=i1 style='height:16px'></div><div id=i2 style='height:16px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("i0")->y);
    TEST_ASSERT_EQUAL(16, lot52_node("i1")->y);
    TEST_ASSERT_EQUAL(32, lot52_node("i2")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_ar2_36(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;grid-template-columns:40px;grid-auto-rows:16px;align-items:stretch'><div id=i0 style='height:16px'></div><div id=i1 style='height:16px'></div><div id=i2 style='height:16px'></div><div id=i3 style='height:16px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("i0")->y);
    TEST_ASSERT_EQUAL(16, lot52_node("i1")->y);
    TEST_ASSERT_EQUAL(32, lot52_node("i2")->y);
    TEST_ASSERT_EQUAL(48, lot52_node("i3")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_ar2_37(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;grid-template-columns:40px;grid-auto-rows:17px;align-items:stretch'><div id=i0 style='height:17px'></div><div id=i1 style='height:17px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("i0")->y);
    TEST_ASSERT_EQUAL(17, lot52_node("i1")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_ar2_38(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;grid-template-columns:40px;grid-auto-rows:17px;align-items:stretch'><div id=i0 style='height:17px'></div><div id=i1 style='height:17px'></div><div id=i2 style='height:17px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("i0")->y);
    TEST_ASSERT_EQUAL(17, lot52_node("i1")->y);
    TEST_ASSERT_EQUAL(34, lot52_node("i2")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_ar2_39(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;grid-template-columns:40px;grid-auto-rows:17px;align-items:stretch'><div id=i0 style='height:17px'></div><div id=i1 style='height:17px'></div><div id=i2 style='height:17px'></div><div id=i3 style='height:17px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("i0")->y);
    TEST_ASSERT_EQUAL(17, lot52_node("i1")->y);
    TEST_ASSERT_EQUAL(34, lot52_node("i2")->y);
    TEST_ASSERT_EQUAL(51, lot52_node("i3")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_ar2_40(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;grid-template-columns:40px;grid-auto-rows:18px;align-items:stretch'><div id=i0 style='height:18px'></div><div id=i1 style='height:18px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("i0")->y);
    TEST_ASSERT_EQUAL(18, lot52_node("i1")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_ar2_41(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;grid-template-columns:40px;grid-auto-rows:18px;align-items:stretch'><div id=i0 style='height:18px'></div><div id=i1 style='height:18px'></div><div id=i2 style='height:18px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("i0")->y);
    TEST_ASSERT_EQUAL(18, lot52_node("i1")->y);
    TEST_ASSERT_EQUAL(36, lot52_node("i2")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_ar2_42(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;grid-template-columns:40px;grid-auto-rows:18px;align-items:stretch'><div id=i0 style='height:18px'></div><div id=i1 style='height:18px'></div><div id=i2 style='height:18px'></div><div id=i3 style='height:18px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("i0")->y);
    TEST_ASSERT_EQUAL(18, lot52_node("i1")->y);
    TEST_ASSERT_EQUAL(36, lot52_node("i2")->y);
    TEST_ASSERT_EQUAL(54, lot52_node("i3")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_ar2_43(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;grid-template-columns:40px;grid-auto-rows:19px;align-items:stretch'><div id=i0 style='height:19px'></div><div id=i1 style='height:19px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("i0")->y);
    TEST_ASSERT_EQUAL(19, lot52_node("i1")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_ar2_44(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;grid-template-columns:40px;grid-auto-rows:19px;align-items:stretch'><div id=i0 style='height:19px'></div><div id=i1 style='height:19px'></div><div id=i2 style='height:19px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("i0")->y);
    TEST_ASSERT_EQUAL(19, lot52_node("i1")->y);
    TEST_ASSERT_EQUAL(38, lot52_node("i2")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_ar2_45(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;grid-template-columns:40px;grid-auto-rows:19px;align-items:stretch'><div id=i0 style='height:19px'></div><div id=i1 style='height:19px'></div><div id=i2 style='height:19px'></div><div id=i3 style='height:19px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("i0")->y);
    TEST_ASSERT_EQUAL(19, lot52_node("i1")->y);
    TEST_ASSERT_EQUAL(38, lot52_node("i2")->y);
    TEST_ASSERT_EQUAL(57, lot52_node("i3")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_ar2_46(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;grid-template-columns:40px;grid-auto-rows:20px;align-items:stretch'><div id=i0 style='height:20px'></div><div id=i1 style='height:20px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("i0")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("i1")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_ar2_47(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;grid-template-columns:40px;grid-auto-rows:20px;align-items:stretch'><div id=i0 style='height:20px'></div><div id=i1 style='height:20px'></div><div id=i2 style='height:20px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("i0")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("i1")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("i2")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_ar2_48(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;grid-template-columns:40px;grid-auto-rows:20px;align-items:stretch'><div id=i0 style='height:20px'></div><div id=i1 style='height:20px'></div><div id=i2 style='height:20px'></div><div id=i3 style='height:20px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("i0")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("i1")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("i2")->y);
    TEST_ASSERT_EQUAL(60, lot52_node("i3")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_ar2_49(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;grid-template-columns:40px;grid-auto-rows:21px;align-items:stretch'><div id=i0 style='height:21px'></div><div id=i1 style='height:21px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("i0")->y);
    TEST_ASSERT_EQUAL(21, lot52_node("i1")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_ar2_50(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;grid-template-columns:40px;grid-auto-rows:21px;align-items:stretch'><div id=i0 style='height:21px'></div><div id=i1 style='height:21px'></div><div id=i2 style='height:21px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("i0")->y);
    TEST_ASSERT_EQUAL(21, lot52_node("i1")->y);
    TEST_ASSERT_EQUAL(42, lot52_node("i2")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_ar2_51(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;grid-template-columns:40px;grid-auto-rows:21px;align-items:stretch'><div id=i0 style='height:21px'></div><div id=i1 style='height:21px'></div><div id=i2 style='height:21px'></div><div id=i3 style='height:21px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("i0")->y);
    TEST_ASSERT_EQUAL(21, lot52_node("i1")->y);
    TEST_ASSERT_EQUAL(42, lot52_node("i2")->y);
    TEST_ASSERT_EQUAL(63, lot52_node("i3")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_ar2_52(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;grid-template-columns:40px;grid-auto-rows:22px;align-items:stretch'><div id=i0 style='height:22px'></div><div id=i1 style='height:22px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("i0")->y);
    TEST_ASSERT_EQUAL(22, lot52_node("i1")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_ar2_53(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;grid-template-columns:40px;grid-auto-rows:22px;align-items:stretch'><div id=i0 style='height:22px'></div><div id=i1 style='height:22px'></div><div id=i2 style='height:22px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("i0")->y);
    TEST_ASSERT_EQUAL(22, lot52_node("i1")->y);
    TEST_ASSERT_EQUAL(44, lot52_node("i2")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_ar2_54(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;grid-template-columns:40px;grid-auto-rows:22px;align-items:stretch'><div id=i0 style='height:22px'></div><div id=i1 style='height:22px'></div><div id=i2 style='height:22px'></div><div id=i3 style='height:22px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("i0")->y);
    TEST_ASSERT_EQUAL(22, lot52_node("i1")->y);
    TEST_ASSERT_EQUAL(44, lot52_node("i2")->y);
    TEST_ASSERT_EQUAL(66, lot52_node("i3")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_ar2_55(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;grid-template-columns:40px;grid-auto-rows:23px;align-items:stretch'><div id=i0 style='height:23px'></div><div id=i1 style='height:23px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("i0")->y);
    TEST_ASSERT_EQUAL(23, lot52_node("i1")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_ar2_56(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;grid-template-columns:40px;grid-auto-rows:23px;align-items:stretch'><div id=i0 style='height:23px'></div><div id=i1 style='height:23px'></div><div id=i2 style='height:23px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("i0")->y);
    TEST_ASSERT_EQUAL(23, lot52_node("i1")->y);
    TEST_ASSERT_EQUAL(46, lot52_node("i2")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_ar2_57(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;grid-template-columns:40px;grid-auto-rows:23px;align-items:stretch'><div id=i0 style='height:23px'></div><div id=i1 style='height:23px'></div><div id=i2 style='height:23px'></div><div id=i3 style='height:23px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("i0")->y);
    TEST_ASSERT_EQUAL(23, lot52_node("i1")->y);
    TEST_ASSERT_EQUAL(46, lot52_node("i2")->y);
    TEST_ASSERT_EQUAL(69, lot52_node("i3")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_ar2_58(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;grid-template-columns:40px;grid-auto-rows:24px;align-items:stretch'><div id=i0 style='height:24px'></div><div id=i1 style='height:24px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("i0")->y);
    TEST_ASSERT_EQUAL(24, lot52_node("i1")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_ar2_59(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;grid-template-columns:40px;grid-auto-rows:24px;align-items:stretch'><div id=i0 style='height:24px'></div><div id=i1 style='height:24px'></div><div id=i2 style='height:24px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("i0")->y);
    TEST_ASSERT_EQUAL(24, lot52_node("i1")->y);
    TEST_ASSERT_EQUAL(48, lot52_node("i2")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_ar2_60(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;grid-template-columns:40px;grid-auto-rows:24px;align-items:stretch'><div id=i0 style='height:24px'></div><div id=i1 style='height:24px'></div><div id=i2 style='height:24px'></div><div id=i3 style='height:24px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("i0")->y);
    TEST_ASSERT_EQUAL(24, lot52_node("i1")->y);
    TEST_ASSERT_EQUAL(48, lot52_node("i2")->y);
    TEST_ASSERT_EQUAL(72, lot52_node("i3")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_ar2_61(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;grid-template-columns:40px;grid-auto-rows:25px;align-items:stretch'><div id=i0 style='height:25px'></div><div id=i1 style='height:25px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("i0")->y);
    TEST_ASSERT_EQUAL(25, lot52_node("i1")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_ar2_62(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;grid-template-columns:40px;grid-auto-rows:25px;align-items:stretch'><div id=i0 style='height:25px'></div><div id=i1 style='height:25px'></div><div id=i2 style='height:25px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("i0")->y);
    TEST_ASSERT_EQUAL(25, lot52_node("i1")->y);
    TEST_ASSERT_EQUAL(50, lot52_node("i2")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_ar2_63(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;grid-template-columns:40px;grid-auto-rows:25px;align-items:stretch'><div id=i0 style='height:25px'></div><div id=i1 style='height:25px'></div><div id=i2 style='height:25px'></div><div id=i3 style='height:25px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("i0")->y);
    TEST_ASSERT_EQUAL(25, lot52_node("i1")->y);
    TEST_ASSERT_EQUAL(50, lot52_node("i2")->y);
    TEST_ASSERT_EQUAL(75, lot52_node("i3")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_ar2_64(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;grid-template-columns:40px;grid-auto-rows:26px;align-items:stretch'><div id=i0 style='height:26px'></div><div id=i1 style='height:26px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("i0")->y);
    TEST_ASSERT_EQUAL(26, lot52_node("i1")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_ar2_65(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;grid-template-columns:40px;grid-auto-rows:26px;align-items:stretch'><div id=i0 style='height:26px'></div><div id=i1 style='height:26px'></div><div id=i2 style='height:26px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("i0")->y);
    TEST_ASSERT_EQUAL(26, lot52_node("i1")->y);
    TEST_ASSERT_EQUAL(52, lot52_node("i2")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_ar2_66(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;grid-template-columns:40px;grid-auto-rows:26px;align-items:stretch'><div id=i0 style='height:26px'></div><div id=i1 style='height:26px'></div><div id=i2 style='height:26px'></div><div id=i3 style='height:26px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("i0")->y);
    TEST_ASSERT_EQUAL(26, lot52_node("i1")->y);
    TEST_ASSERT_EQUAL(52, lot52_node("i2")->y);
    TEST_ASSERT_EQUAL(78, lot52_node("i3")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_ar2_67(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;grid-template-columns:40px;grid-auto-rows:27px;align-items:stretch'><div id=i0 style='height:27px'></div><div id=i1 style='height:27px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("i0")->y);
    TEST_ASSERT_EQUAL(27, lot52_node("i1")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_ar2_68(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;grid-template-columns:40px;grid-auto-rows:27px;align-items:stretch'><div id=i0 style='height:27px'></div><div id=i1 style='height:27px'></div><div id=i2 style='height:27px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("i0")->y);
    TEST_ASSERT_EQUAL(27, lot52_node("i1")->y);
    TEST_ASSERT_EQUAL(54, lot52_node("i2")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_ar2_69(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;grid-template-columns:40px;grid-auto-rows:27px;align-items:stretch'><div id=i0 style='height:27px'></div><div id=i1 style='height:27px'></div><div id=i2 style='height:27px'></div><div id=i3 style='height:27px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("i0")->y);
    TEST_ASSERT_EQUAL(27, lot52_node("i1")->y);
    TEST_ASSERT_EQUAL(54, lot52_node("i2")->y);
    TEST_ASSERT_EQUAL(81, lot52_node("i3")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_ar2_70(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;grid-template-columns:40px;grid-auto-rows:28px;align-items:stretch'><div id=i0 style='height:28px'></div><div id=i1 style='height:28px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("i0")->y);
    TEST_ASSERT_EQUAL(28, lot52_node("i1")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_ar2_71(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;grid-template-columns:40px;grid-auto-rows:28px;align-items:stretch'><div id=i0 style='height:28px'></div><div id=i1 style='height:28px'></div><div id=i2 style='height:28px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("i0")->y);
    TEST_ASSERT_EQUAL(28, lot52_node("i1")->y);
    TEST_ASSERT_EQUAL(56, lot52_node("i2")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_ar2_72(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;grid-template-columns:40px;grid-auto-rows:28px;align-items:stretch'><div id=i0 style='height:28px'></div><div id=i1 style='height:28px'></div><div id=i2 style='height:28px'></div><div id=i3 style='height:28px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("i0")->y);
    TEST_ASSERT_EQUAL(28, lot52_node("i1")->y);
    TEST_ASSERT_EQUAL(56, lot52_node("i2")->y);
    TEST_ASSERT_EQUAL(84, lot52_node("i3")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_ar2_73(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;grid-template-columns:40px;grid-auto-rows:29px;align-items:stretch'><div id=i0 style='height:29px'></div><div id=i1 style='height:29px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("i0")->y);
    TEST_ASSERT_EQUAL(29, lot52_node("i1")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_ar2_74(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;grid-template-columns:40px;grid-auto-rows:29px;align-items:stretch'><div id=i0 style='height:29px'></div><div id=i1 style='height:29px'></div><div id=i2 style='height:29px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("i0")->y);
    TEST_ASSERT_EQUAL(29, lot52_node("i1")->y);
    TEST_ASSERT_EQUAL(58, lot52_node("i2")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_ar2_75(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;grid-template-columns:40px;grid-auto-rows:29px;align-items:stretch'><div id=i0 style='height:29px'></div><div id=i1 style='height:29px'></div><div id=i2 style='height:29px'></div><div id=i3 style='height:29px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("i0")->y);
    TEST_ASSERT_EQUAL(29, lot52_node("i1")->y);
    TEST_ASSERT_EQUAL(58, lot52_node("i2")->y);
    TEST_ASSERT_EQUAL(87, lot52_node("i3")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_ar2_76(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;grid-template-columns:40px;grid-auto-rows:30px;align-items:stretch'><div id=i0 style='height:30px'></div><div id=i1 style='height:30px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("i0")->y);
    TEST_ASSERT_EQUAL(30, lot52_node("i1")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_ar2_77(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;grid-template-columns:40px;grid-auto-rows:30px;align-items:stretch'><div id=i0 style='height:30px'></div><div id=i1 style='height:30px'></div><div id=i2 style='height:30px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("i0")->y);
    TEST_ASSERT_EQUAL(30, lot52_node("i1")->y);
    TEST_ASSERT_EQUAL(60, lot52_node("i2")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_ar2_78(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;grid-template-columns:40px;grid-auto-rows:30px;align-items:stretch'><div id=i0 style='height:30px'></div><div id=i1 style='height:30px'></div><div id=i2 style='height:30px'></div><div id=i3 style='height:30px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("i0")->y);
    TEST_ASSERT_EQUAL(30, lot52_node("i1")->y);
    TEST_ASSERT_EQUAL(60, lot52_node("i2")->y);
    TEST_ASSERT_EQUAL(90, lot52_node("i3")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_acols2_79(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:80px;height:16px;grid-template-rows:16px;grid-auto-columns:10px;grid-auto-flow:column;align-items:stretch'><div id=i0></div><div id=i1></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("i0")->x);
    TEST_ASSERT_EQUAL(10, lot52_node("i1")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_acols2_80(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:80px;height:16px;grid-template-rows:16px;grid-auto-columns:10px;grid-auto-flow:column;align-items:stretch'><div id=i0></div><div id=i1></div><div id=i2></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("i0")->x);
    TEST_ASSERT_EQUAL(10, lot52_node("i1")->x);
    TEST_ASSERT_EQUAL(20, lot52_node("i2")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_acols2_81(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:80px;height:16px;grid-template-rows:16px;grid-auto-columns:10px;grid-auto-flow:column;align-items:stretch'><div id=i0></div><div id=i1></div><div id=i2></div><div id=i3></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("i0")->x);
    TEST_ASSERT_EQUAL(10, lot52_node("i1")->x);
    TEST_ASSERT_EQUAL(20, lot52_node("i2")->x);
    TEST_ASSERT_EQUAL(30, lot52_node("i3")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_acols2_82(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:80px;height:16px;grid-template-rows:16px;grid-auto-columns:12px;grid-auto-flow:column;align-items:stretch'><div id=i0></div><div id=i1></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("i0")->x);
    TEST_ASSERT_EQUAL(12, lot52_node("i1")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_acols2_83(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:80px;height:16px;grid-template-rows:16px;grid-auto-columns:12px;grid-auto-flow:column;align-items:stretch'><div id=i0></div><div id=i1></div><div id=i2></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("i0")->x);
    TEST_ASSERT_EQUAL(12, lot52_node("i1")->x);
    TEST_ASSERT_EQUAL(24, lot52_node("i2")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_acols2_84(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:80px;height:16px;grid-template-rows:16px;grid-auto-columns:12px;grid-auto-flow:column;align-items:stretch'><div id=i0></div><div id=i1></div><div id=i2></div><div id=i3></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("i0")->x);
    TEST_ASSERT_EQUAL(12, lot52_node("i1")->x);
    TEST_ASSERT_EQUAL(24, lot52_node("i2")->x);
    TEST_ASSERT_EQUAL(36, lot52_node("i3")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_acols2_85(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:80px;height:16px;grid-template-rows:16px;grid-auto-columns:14px;grid-auto-flow:column;align-items:stretch'><div id=i0></div><div id=i1></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("i0")->x);
    TEST_ASSERT_EQUAL(14, lot52_node("i1")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_acols2_86(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:80px;height:16px;grid-template-rows:16px;grid-auto-columns:14px;grid-auto-flow:column;align-items:stretch'><div id=i0></div><div id=i1></div><div id=i2></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("i0")->x);
    TEST_ASSERT_EQUAL(14, lot52_node("i1")->x);
    TEST_ASSERT_EQUAL(28, lot52_node("i2")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_acols2_87(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:80px;height:16px;grid-template-rows:16px;grid-auto-columns:14px;grid-auto-flow:column;align-items:stretch'><div id=i0></div><div id=i1></div><div id=i2></div><div id=i3></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("i0")->x);
    TEST_ASSERT_EQUAL(14, lot52_node("i1")->x);
    TEST_ASSERT_EQUAL(28, lot52_node("i2")->x);
    TEST_ASSERT_EQUAL(42, lot52_node("i3")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_acols2_88(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:80px;height:16px;grid-template-rows:16px;grid-auto-columns:16px;grid-auto-flow:column;align-items:stretch'><div id=i0></div><div id=i1></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("i0")->x);
    TEST_ASSERT_EQUAL(16, lot52_node("i1")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_acols2_89(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:80px;height:16px;grid-template-rows:16px;grid-auto-columns:16px;grid-auto-flow:column;align-items:stretch'><div id=i0></div><div id=i1></div><div id=i2></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("i0")->x);
    TEST_ASSERT_EQUAL(16, lot52_node("i1")->x);
    TEST_ASSERT_EQUAL(32, lot52_node("i2")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_acols2_90(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:80px;height:16px;grid-template-rows:16px;grid-auto-columns:16px;grid-auto-flow:column;align-items:stretch'><div id=i0></div><div id=i1></div><div id=i2></div><div id=i3></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("i0")->x);
    TEST_ASSERT_EQUAL(16, lot52_node("i1")->x);
    TEST_ASSERT_EQUAL(32, lot52_node("i2")->x);
    TEST_ASSERT_EQUAL(48, lot52_node("i3")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_acols2_91(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:80px;height:16px;grid-template-rows:16px;grid-auto-columns:18px;grid-auto-flow:column;align-items:stretch'><div id=i0></div><div id=i1></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("i0")->x);
    TEST_ASSERT_EQUAL(18, lot52_node("i1")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_acols2_92(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:80px;height:16px;grid-template-rows:16px;grid-auto-columns:18px;grid-auto-flow:column;align-items:stretch'><div id=i0></div><div id=i1></div><div id=i2></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("i0")->x);
    TEST_ASSERT_EQUAL(18, lot52_node("i1")->x);
    TEST_ASSERT_EQUAL(36, lot52_node("i2")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_acols2_93(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:82px;height:16px;grid-template-rows:16px;grid-auto-columns:18px;grid-auto-flow:column;align-items:stretch'><div id=i0></div><div id=i1></div><div id=i2></div><div id=i3></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("i0")->x);
    TEST_ASSERT_EQUAL(18, lot52_node("i1")->x);
    TEST_ASSERT_EQUAL(36, lot52_node("i2")->x);
    TEST_ASSERT_EQUAL(54, lot52_node("i3")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_acols2_94(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:80px;height:16px;grid-template-rows:16px;grid-auto-columns:20px;grid-auto-flow:column;align-items:stretch'><div id=i0></div><div id=i1></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("i0")->x);
    TEST_ASSERT_EQUAL(20, lot52_node("i1")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_acols2_95(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:80px;height:16px;grid-template-rows:16px;grid-auto-columns:20px;grid-auto-flow:column;align-items:stretch'><div id=i0></div><div id=i1></div><div id=i2></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("i0")->x);
    TEST_ASSERT_EQUAL(20, lot52_node("i1")->x);
    TEST_ASSERT_EQUAL(40, lot52_node("i2")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_acols2_96(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:90px;height:16px;grid-template-rows:16px;grid-auto-columns:20px;grid-auto-flow:column;align-items:stretch'><div id=i0></div><div id=i1></div><div id=i2></div><div id=i3></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("i0")->x);
    TEST_ASSERT_EQUAL(20, lot52_node("i1")->x);
    TEST_ASSERT_EQUAL(40, lot52_node("i2")->x);
    TEST_ASSERT_EQUAL(60, lot52_node("i3")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_acols2_97(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:80px;height:16px;grid-template-rows:16px;grid-auto-columns:22px;grid-auto-flow:column;align-items:stretch'><div id=i0></div><div id=i1></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("i0")->x);
    TEST_ASSERT_EQUAL(22, lot52_node("i1")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_acols2_98(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:80px;height:16px;grid-template-rows:16px;grid-auto-columns:22px;grid-auto-flow:column;align-items:stretch'><div id=i0></div><div id=i1></div><div id=i2></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("i0")->x);
    TEST_ASSERT_EQUAL(22, lot52_node("i1")->x);
    TEST_ASSERT_EQUAL(44, lot52_node("i2")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_acols2_99(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:98px;height:16px;grid-template-rows:16px;grid-auto-columns:22px;grid-auto-flow:column;align-items:stretch'><div id=i0></div><div id=i1></div><div id=i2></div><div id=i3></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("i0")->x);
    TEST_ASSERT_EQUAL(22, lot52_node("i1")->x);
    TEST_ASSERT_EQUAL(44, lot52_node("i2")->x);
    TEST_ASSERT_EQUAL(66, lot52_node("i3")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_acols2_100(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:80px;height:16px;grid-template-rows:16px;grid-auto-columns:24px;grid-auto-flow:column;align-items:stretch'><div id=i0></div><div id=i1></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("i0")->x);
    TEST_ASSERT_EQUAL(24, lot52_node("i1")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_acols2_101(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:82px;height:16px;grid-template-rows:16px;grid-auto-columns:24px;grid-auto-flow:column;align-items:stretch'><div id=i0></div><div id=i1></div><div id=i2></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("i0")->x);
    TEST_ASSERT_EQUAL(24, lot52_node("i1")->x);
    TEST_ASSERT_EQUAL(48, lot52_node("i2")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_acols2_102(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:106px;height:16px;grid-template-rows:16px;grid-auto-columns:24px;grid-auto-flow:column;align-items:stretch'><div id=i0></div><div id=i1></div><div id=i2></div><div id=i3></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("i0")->x);
    TEST_ASSERT_EQUAL(24, lot52_node("i1")->x);
    TEST_ASSERT_EQUAL(48, lot52_node("i2")->x);
    TEST_ASSERT_EQUAL(72, lot52_node("i3")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_acols2_103(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:80px;height:16px;grid-template-rows:16px;grid-auto-columns:26px;grid-auto-flow:column;align-items:stretch'><div id=i0></div><div id=i1></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("i0")->x);
    TEST_ASSERT_EQUAL(26, lot52_node("i1")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_acols2_104(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:88px;height:16px;grid-template-rows:16px;grid-auto-columns:26px;grid-auto-flow:column;align-items:stretch'><div id=i0></div><div id=i1></div><div id=i2></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("i0")->x);
    TEST_ASSERT_EQUAL(26, lot52_node("i1")->x);
    TEST_ASSERT_EQUAL(52, lot52_node("i2")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_acols2_105(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:114px;height:16px;grid-template-rows:16px;grid-auto-columns:26px;grid-auto-flow:column;align-items:stretch'><div id=i0></div><div id=i1></div><div id=i2></div><div id=i3></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("i0")->x);
    TEST_ASSERT_EQUAL(26, lot52_node("i1")->x);
    TEST_ASSERT_EQUAL(52, lot52_node("i2")->x);
    TEST_ASSERT_EQUAL(78, lot52_node("i3")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_acols2_106(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:80px;height:16px;grid-template-rows:16px;grid-auto-columns:28px;grid-auto-flow:column;align-items:stretch'><div id=i0></div><div id=i1></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("i0")->x);
    TEST_ASSERT_EQUAL(28, lot52_node("i1")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_acols2_107(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:94px;height:16px;grid-template-rows:16px;grid-auto-columns:28px;grid-auto-flow:column;align-items:stretch'><div id=i0></div><div id=i1></div><div id=i2></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("i0")->x);
    TEST_ASSERT_EQUAL(28, lot52_node("i1")->x);
    TEST_ASSERT_EQUAL(56, lot52_node("i2")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_acols2_108(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:122px;height:16px;grid-template-rows:16px;grid-auto-columns:28px;grid-auto-flow:column;align-items:stretch'><div id=i0></div><div id=i1></div><div id=i2></div><div id=i3></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("i0")->x);
    TEST_ASSERT_EQUAL(28, lot52_node("i1")->x);
    TEST_ASSERT_EQUAL(56, lot52_node("i2")->x);
    TEST_ASSERT_EQUAL(84, lot52_node("i3")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_acols2_109(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:80px;height:16px;grid-template-rows:16px;grid-auto-columns:30px;grid-auto-flow:column;align-items:stretch'><div id=i0></div><div id=i1></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("i0")->x);
    TEST_ASSERT_EQUAL(30, lot52_node("i1")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_acols2_110(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:100px;height:16px;grid-template-rows:16px;grid-auto-columns:30px;grid-auto-flow:column;align-items:stretch'><div id=i0></div><div id=i1></div><div id=i2></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("i0")->x);
    TEST_ASSERT_EQUAL(30, lot52_node("i1")->x);
    TEST_ASSERT_EQUAL(60, lot52_node("i2")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_acols2_111(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:130px;height:16px;grid-template-rows:16px;grid-auto-columns:30px;grid-auto-flow:column;align-items:stretch'><div id=i0></div><div id=i1></div><div id=i2></div><div id=i3></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("i0")->x);
    TEST_ASSERT_EQUAL(30, lot52_node("i1")->x);
    TEST_ASSERT_EQUAL(60, lot52_node("i2")->x);
    TEST_ASSERT_EQUAL(90, lot52_node("i3")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_acols2_112(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:80px;height:16px;grid-template-rows:16px;grid-auto-columns:32px;grid-auto-flow:column;align-items:stretch'><div id=i0></div><div id=i1></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("i0")->x);
    TEST_ASSERT_EQUAL(32, lot52_node("i1")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_acols2_113(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:106px;height:16px;grid-template-rows:16px;grid-auto-columns:32px;grid-auto-flow:column;align-items:stretch'><div id=i0></div><div id=i1></div><div id=i2></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("i0")->x);
    TEST_ASSERT_EQUAL(32, lot52_node("i1")->x);
    TEST_ASSERT_EQUAL(64, lot52_node("i2")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_acols2_114(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:138px;height:16px;grid-template-rows:16px;grid-auto-columns:32px;grid-auto-flow:column;align-items:stretch'><div id=i0></div><div id=i1></div><div id=i2></div><div id=i3></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("i0")->x);
    TEST_ASSERT_EQUAL(32, lot52_node("i1")->x);
    TEST_ASSERT_EQUAL(64, lot52_node("i2")->x);
    TEST_ASSERT_EQUAL(96, lot52_node("i3")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_acols2_115(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:80px;height:16px;grid-template-rows:16px;grid-auto-columns:34px;grid-auto-flow:column;align-items:stretch'><div id=i0></div><div id=i1></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("i0")->x);
    TEST_ASSERT_EQUAL(34, lot52_node("i1")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_acols2_116(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:112px;height:16px;grid-template-rows:16px;grid-auto-columns:34px;grid-auto-flow:column;align-items:stretch'><div id=i0></div><div id=i1></div><div id=i2></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("i0")->x);
    TEST_ASSERT_EQUAL(34, lot52_node("i1")->x);
    TEST_ASSERT_EQUAL(68, lot52_node("i2")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_acols2_117(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:146px;height:16px;grid-template-rows:16px;grid-auto-columns:34px;grid-auto-flow:column;align-items:stretch'><div id=i0></div><div id=i1></div><div id=i2></div><div id=i3></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("i0")->x);
    TEST_ASSERT_EQUAL(34, lot52_node("i1")->x);
    TEST_ASSERT_EQUAL(68, lot52_node("i2")->x);
    TEST_ASSERT_EQUAL(102, lot52_node("i3")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_acols2_118(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:82px;height:16px;grid-template-rows:16px;grid-auto-columns:36px;grid-auto-flow:column;align-items:stretch'><div id=i0></div><div id=i1></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("i0")->x);
    TEST_ASSERT_EQUAL(36, lot52_node("i1")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_acols2_119(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:118px;height:16px;grid-template-rows:16px;grid-auto-columns:36px;grid-auto-flow:column;align-items:stretch'><div id=i0></div><div id=i1></div><div id=i2></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("i0")->x);
    TEST_ASSERT_EQUAL(36, lot52_node("i1")->x);
    TEST_ASSERT_EQUAL(72, lot52_node("i2")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_acols2_120(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:154px;height:16px;grid-template-rows:16px;grid-auto-columns:36px;grid-auto-flow:column;align-items:stretch'><div id=i0></div><div id=i1></div><div id=i2></div><div id=i3></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("i0")->x);
    TEST_ASSERT_EQUAL(36, lot52_node("i1")->x);
    TEST_ASSERT_EQUAL(72, lot52_node("i2")->x);
    TEST_ASSERT_EQUAL(108, lot52_node("i3")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_acols2_121(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:86px;height:16px;grid-template-rows:16px;grid-auto-columns:38px;grid-auto-flow:column;align-items:stretch'><div id=i0></div><div id=i1></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("i0")->x);
    TEST_ASSERT_EQUAL(38, lot52_node("i1")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_acols2_122(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:124px;height:16px;grid-template-rows:16px;grid-auto-columns:38px;grid-auto-flow:column;align-items:stretch'><div id=i0></div><div id=i1></div><div id=i2></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("i0")->x);
    TEST_ASSERT_EQUAL(38, lot52_node("i1")->x);
    TEST_ASSERT_EQUAL(76, lot52_node("i2")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_acols2_123(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:162px;height:16px;grid-template-rows:16px;grid-auto-columns:38px;grid-auto-flow:column;align-items:stretch'><div id=i0></div><div id=i1></div><div id=i2></div><div id=i3></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("i0")->x);
    TEST_ASSERT_EQUAL(38, lot52_node("i1")->x);
    TEST_ASSERT_EQUAL(76, lot52_node("i2")->x);
    TEST_ASSERT_EQUAL(114, lot52_node("i3")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_acols2_124(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:90px;height:16px;grid-template-rows:16px;grid-auto-columns:40px;grid-auto-flow:column;align-items:stretch'><div id=i0></div><div id=i1></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("i0")->x);
    TEST_ASSERT_EQUAL(40, lot52_node("i1")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_acols2_125(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:130px;height:16px;grid-template-rows:16px;grid-auto-columns:40px;grid-auto-flow:column;align-items:stretch'><div id=i0></div><div id=i1></div><div id=i2></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("i0")->x);
    TEST_ASSERT_EQUAL(40, lot52_node("i1")->x);
    TEST_ASSERT_EQUAL(80, lot52_node("i2")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_acols2_126(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:170px;height:16px;grid-template-rows:16px;grid-auto-columns:40px;grid-auto-flow:column;align-items:stretch'><div id=i0></div><div id=i1></div><div id=i2></div><div id=i3></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("i0")->x);
    TEST_ASSERT_EQUAL(40, lot52_node("i1")->x);
    TEST_ASSERT_EQUAL(80, lot52_node("i2")->x);
    TEST_ASSERT_EQUAL(120, lot52_node("i3")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void lot79_auto_run(void) {
    RUN_TEST(test_l79_ar2_1);
    RUN_TEST(test_l79_ar2_2);
    RUN_TEST(test_l79_ar2_3);
    RUN_TEST(test_l79_ar2_4);
    RUN_TEST(test_l79_ar2_5);
    RUN_TEST(test_l79_ar2_6);
    RUN_TEST(test_l79_ar2_7);
    RUN_TEST(test_l79_ar2_8);
    RUN_TEST(test_l79_ar2_9);
    RUN_TEST(test_l79_ar2_10);
    RUN_TEST(test_l79_ar2_11);
    RUN_TEST(test_l79_ar2_12);
    RUN_TEST(test_l79_ar2_13);
    RUN_TEST(test_l79_ar2_14);
    RUN_TEST(test_l79_ar2_15);
    RUN_TEST(test_l79_ar2_16);
    RUN_TEST(test_l79_ar2_17);
    RUN_TEST(test_l79_ar2_18);
    RUN_TEST(test_l79_ar2_19);
    RUN_TEST(test_l79_ar2_20);
    RUN_TEST(test_l79_ar2_21);
    RUN_TEST(test_l79_ar2_22);
    RUN_TEST(test_l79_ar2_23);
    RUN_TEST(test_l79_ar2_24);
    RUN_TEST(test_l79_ar2_25);
    RUN_TEST(test_l79_ar2_26);
    RUN_TEST(test_l79_ar2_27);
    RUN_TEST(test_l79_ar2_28);
    RUN_TEST(test_l79_ar2_29);
    RUN_TEST(test_l79_ar2_30);
    RUN_TEST(test_l79_ar2_31);
    RUN_TEST(test_l79_ar2_32);
    RUN_TEST(test_l79_ar2_33);
    RUN_TEST(test_l79_ar2_34);
    RUN_TEST(test_l79_ar2_35);
    RUN_TEST(test_l79_ar2_36);
    RUN_TEST(test_l79_ar2_37);
    RUN_TEST(test_l79_ar2_38);
    RUN_TEST(test_l79_ar2_39);
    RUN_TEST(test_l79_ar2_40);
    RUN_TEST(test_l79_ar2_41);
    RUN_TEST(test_l79_ar2_42);
    RUN_TEST(test_l79_ar2_43);
    RUN_TEST(test_l79_ar2_44);
    RUN_TEST(test_l79_ar2_45);
    RUN_TEST(test_l79_ar2_46);
    RUN_TEST(test_l79_ar2_47);
    RUN_TEST(test_l79_ar2_48);
    RUN_TEST(test_l79_ar2_49);
    RUN_TEST(test_l79_ar2_50);
    RUN_TEST(test_l79_ar2_51);
    RUN_TEST(test_l79_ar2_52);
    RUN_TEST(test_l79_ar2_53);
    RUN_TEST(test_l79_ar2_54);
    RUN_TEST(test_l79_ar2_55);
    RUN_TEST(test_l79_ar2_56);
    RUN_TEST(test_l79_ar2_57);
    RUN_TEST(test_l79_ar2_58);
    RUN_TEST(test_l79_ar2_59);
    RUN_TEST(test_l79_ar2_60);
    RUN_TEST(test_l79_ar2_61);
    RUN_TEST(test_l79_ar2_62);
    RUN_TEST(test_l79_ar2_63);
    RUN_TEST(test_l79_ar2_64);
    RUN_TEST(test_l79_ar2_65);
    RUN_TEST(test_l79_ar2_66);
    RUN_TEST(test_l79_ar2_67);
    RUN_TEST(test_l79_ar2_68);
    RUN_TEST(test_l79_ar2_69);
    RUN_TEST(test_l79_ar2_70);
    RUN_TEST(test_l79_ar2_71);
    RUN_TEST(test_l79_ar2_72);
    RUN_TEST(test_l79_ar2_73);
    RUN_TEST(test_l79_ar2_74);
    RUN_TEST(test_l79_ar2_75);
    RUN_TEST(test_l79_ar2_76);
    RUN_TEST(test_l79_ar2_77);
    RUN_TEST(test_l79_ar2_78);
    RUN_TEST(test_l79_acols2_79);
    RUN_TEST(test_l79_acols2_80);
    RUN_TEST(test_l79_acols2_81);
    RUN_TEST(test_l79_acols2_82);
    RUN_TEST(test_l79_acols2_83);
    RUN_TEST(test_l79_acols2_84);
    RUN_TEST(test_l79_acols2_85);
    RUN_TEST(test_l79_acols2_86);
    RUN_TEST(test_l79_acols2_87);
    RUN_TEST(test_l79_acols2_88);
    RUN_TEST(test_l79_acols2_89);
    RUN_TEST(test_l79_acols2_90);
    RUN_TEST(test_l79_acols2_91);
    RUN_TEST(test_l79_acols2_92);
    RUN_TEST(test_l79_acols2_93);
    RUN_TEST(test_l79_acols2_94);
    RUN_TEST(test_l79_acols2_95);
    RUN_TEST(test_l79_acols2_96);
    RUN_TEST(test_l79_acols2_97);
    RUN_TEST(test_l79_acols2_98);
    RUN_TEST(test_l79_acols2_99);
    RUN_TEST(test_l79_acols2_100);
    RUN_TEST(test_l79_acols2_101);
    RUN_TEST(test_l79_acols2_102);
    RUN_TEST(test_l79_acols2_103);
    RUN_TEST(test_l79_acols2_104);
    RUN_TEST(test_l79_acols2_105);
    RUN_TEST(test_l79_acols2_106);
    RUN_TEST(test_l79_acols2_107);
    RUN_TEST(test_l79_acols2_108);
    RUN_TEST(test_l79_acols2_109);
    RUN_TEST(test_l79_acols2_110);
    RUN_TEST(test_l79_acols2_111);
    RUN_TEST(test_l79_acols2_112);
    RUN_TEST(test_l79_acols2_113);
    RUN_TEST(test_l79_acols2_114);
    RUN_TEST(test_l79_acols2_115);
    RUN_TEST(test_l79_acols2_116);
    RUN_TEST(test_l79_acols2_117);
    RUN_TEST(test_l79_acols2_118);
    RUN_TEST(test_l79_acols2_119);
    RUN_TEST(test_l79_acols2_120);
    RUN_TEST(test_l79_acols2_121);
    RUN_TEST(test_l79_acols2_122);
    RUN_TEST(test_l79_acols2_123);
    RUN_TEST(test_l79_acols2_124);
    RUN_TEST(test_l79_acols2_125);
    RUN_TEST(test_l79_acols2_126);
    RUN_TEST(test_l79_auto_rows_1);
    RUN_TEST(test_l79_auto_rows_2);
    RUN_TEST(test_l79_auto_rows_3);
    RUN_TEST(test_l79_auto_rows_4);
    RUN_TEST(test_l79_auto_rows_5);
    RUN_TEST(test_l79_auto_rows_6);
    RUN_TEST(test_l79_auto_rows_7);
    RUN_TEST(test_l79_auto_rows_8);
    RUN_TEST(test_l79_auto_rows_9);
    RUN_TEST(test_l79_auto_rows_10);
    RUN_TEST(test_l79_auto_rows_11);
    RUN_TEST(test_l79_auto_rows_12);
    RUN_TEST(test_l79_auto_rows_13);
    RUN_TEST(test_l79_auto_rows_14);
    RUN_TEST(test_l79_auto_rows_15);
    RUN_TEST(test_l79_auto_rows_16);
    RUN_TEST(test_l79_auto_rows_17);
    RUN_TEST(test_l79_auto_rows_18);
    RUN_TEST(test_l79_auto_rows_19);
    RUN_TEST(test_l79_auto_rows_20);
    RUN_TEST(test_l79_auto_rows_21);
    RUN_TEST(test_l79_auto_rows_22);
    RUN_TEST(test_l79_auto_rows_23);
    RUN_TEST(test_l79_auto_rows_24);
    RUN_TEST(test_l79_auto_rows_25);
    RUN_TEST(test_l79_auto_rows_26);
    RUN_TEST(test_l79_auto_rows_27);
    RUN_TEST(test_l79_auto_rows_28);
    RUN_TEST(test_l79_auto_rows_29);
    RUN_TEST(test_l79_auto_rows_30);
    RUN_TEST(test_l79_auto_rows_31);
    RUN_TEST(test_l79_auto_rows_32);
    RUN_TEST(test_l79_auto_cols_33);
    RUN_TEST(test_l79_auto_cols_34);
    RUN_TEST(test_l79_auto_cols_35);
    RUN_TEST(test_l79_auto_cols_36);
    RUN_TEST(test_l79_auto_cols_37);
    RUN_TEST(test_l79_auto_cols_38);
    RUN_TEST(test_l79_auto_cols_39);
    RUN_TEST(test_l79_auto_cols_40);
    RUN_TEST(test_l79_auto_cols_41);
    RUN_TEST(test_l79_auto_cols_42);
    RUN_TEST(test_l79_auto_cols_43);
    RUN_TEST(test_l79_auto_cols_44);
    RUN_TEST(test_l79_minmax_tpl_45);
    RUN_TEST(test_l79_minmax_tpl_46);
    RUN_TEST(test_l79_minmax_tpl_47);
    RUN_TEST(test_l79_minmax_tpl_48);
    RUN_TEST(test_l79_minmax_tpl_49);
    RUN_TEST(test_l79_minmax_tpl_50);
    RUN_TEST(test_l79_minmax_tpl_51);
    RUN_TEST(test_l79_minmax_tpl_52);
    RUN_TEST(test_l79_minmax_tpl_53);
    RUN_TEST(test_l79_minmax_tpl_54);
    RUN_TEST(test_l79_minmax_tpl_55);
    RUN_TEST(test_l79_minmax_tpl_56);
    RUN_TEST(test_l79_minmax_tpl_57);
    RUN_TEST(test_l79_minmax_tpl_58);
    RUN_TEST(test_l79_auto_mm_59);
    RUN_TEST(test_l79_auto_mm_60);
    RUN_TEST(test_l79_auto_mm_61);
    RUN_TEST(test_l79_auto_mm_62);
    RUN_TEST(test_l79_auto_mm_63);
    RUN_TEST(test_l79_auto_mm_64);
    RUN_TEST(test_l79_auto_mm_65);
    RUN_TEST(test_l79_auto_mm_66);
    RUN_TEST(test_l79_auto_mm_67);
    RUN_TEST(test_l79_auto_mm_68);
    RUN_TEST(test_l79_mixed_rows_69);
    RUN_TEST(test_l79_mixed_rows_70);
    RUN_TEST(test_l79_mixed_rows_71);
    RUN_TEST(test_l79_mixed_rows_72);
    RUN_TEST(test_l79_mixed_rows_73);
    RUN_TEST(test_l79_mixed_rows_74);
    RUN_TEST(test_l79_mixed_rows_75);
    RUN_TEST(test_l79_mixed_rows_76);
    RUN_TEST(test_l79_mixed_rows_77);
}
