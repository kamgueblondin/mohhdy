/* Lot 79: grid-auto-flow dense packing (mutation-proved). */
static void test_l79_contract_doc_size(void) {
    TEST_ASSERT_TRUE(sizeof(we_doc_t) <= WE_DOCUMENT_MAX);
    TEST_ASSERT_TRUE(sizeof(we_doc_t) < 1048576u);
}

static void test_l79_dense_row_1(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:90px;grid-template-columns:repeat(3,1fr);grid-auto-flow:dense;align-items:stretch'><div id=a style='grid-column:span 2;height:8px'></div><div id=b style='grid-column:span 2;height:8px'></div><div id=c style='height:8px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(8, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(60, lot52_node("c")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("c")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_dense_row_2(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:90px;grid-template-columns:repeat(3,1fr);grid-auto-flow:row dense;align-items:stretch'><div id=a style='grid-column:span 2;height:8px'></div><div id=b style='grid-column:span 2;height:8px'></div><div id=c style='height:8px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(8, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(60, lot52_node("c")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("c")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_dense_row_3(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:90px;grid-template-columns:repeat(3,1fr);grid-auto-flow:dense row;align-items:stretch'><div id=a style='grid-column:span 2;height:8px'></div><div id=b style='grid-column:span 2;height:8px'></div><div id=c style='height:8px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(8, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(60, lot52_node("c")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("c")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_dense_row_4(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:90px;grid-template-columns:repeat(3,1fr);grid-auto-flow:dense;align-items:stretch'><div id=a style='grid-column:span 2;height:10px'></div><div id=b style='grid-column:span 2;height:10px'></div><div id=c style='height:10px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(60, lot52_node("c")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("c")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_dense_row_5(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:90px;grid-template-columns:repeat(3,1fr);grid-auto-flow:row dense;align-items:stretch'><div id=a style='grid-column:span 2;height:10px'></div><div id=b style='grid-column:span 2;height:10px'></div><div id=c style='height:10px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(60, lot52_node("c")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("c")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_dense_row_6(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:90px;grid-template-columns:repeat(3,1fr);grid-auto-flow:dense row;align-items:stretch'><div id=a style='grid-column:span 2;height:10px'></div><div id=b style='grid-column:span 2;height:10px'></div><div id=c style='height:10px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(60, lot52_node("c")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("c")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_dense_row_7(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:90px;grid-template-columns:repeat(3,1fr);grid-auto-flow:dense;align-items:stretch'><div id=a style='grid-column:span 2;height:12px'></div><div id=b style='grid-column:span 2;height:12px'></div><div id=c style='height:12px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(12, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(60, lot52_node("c")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("c")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_dense_row_8(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:90px;grid-template-columns:repeat(3,1fr);grid-auto-flow:row dense;align-items:stretch'><div id=a style='grid-column:span 2;height:12px'></div><div id=b style='grid-column:span 2;height:12px'></div><div id=c style='height:12px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(12, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(60, lot52_node("c")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("c")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_dense_row_9(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:90px;grid-template-columns:repeat(3,1fr);grid-auto-flow:dense row;align-items:stretch'><div id=a style='grid-column:span 2;height:12px'></div><div id=b style='grid-column:span 2;height:12px'></div><div id=c style='height:12px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(12, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(60, lot52_node("c")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("c")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_dense_row_10(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:90px;grid-template-columns:repeat(3,1fr);grid-auto-flow:dense;align-items:stretch'><div id=a style='grid-column:span 2;height:14px'></div><div id=b style='grid-column:span 2;height:14px'></div><div id=c style='height:14px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(14, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(60, lot52_node("c")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("c")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_dense_row_11(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:90px;grid-template-columns:repeat(3,1fr);grid-auto-flow:row dense;align-items:stretch'><div id=a style='grid-column:span 2;height:14px'></div><div id=b style='grid-column:span 2;height:14px'></div><div id=c style='height:14px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(14, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(60, lot52_node("c")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("c")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_dense_row_12(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:90px;grid-template-columns:repeat(3,1fr);grid-auto-flow:dense row;align-items:stretch'><div id=a style='grid-column:span 2;height:14px'></div><div id=b style='grid-column:span 2;height:14px'></div><div id=c style='height:14px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(14, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(60, lot52_node("c")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("c")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_dense_row_13(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:90px;grid-template-columns:repeat(3,1fr);grid-auto-flow:dense;align-items:stretch'><div id=a style='grid-column:span 2;height:16px'></div><div id=b style='grid-column:span 2;height:16px'></div><div id=c style='height:16px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(16, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(60, lot52_node("c")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("c")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_dense_row_14(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:90px;grid-template-columns:repeat(3,1fr);grid-auto-flow:row dense;align-items:stretch'><div id=a style='grid-column:span 2;height:16px'></div><div id=b style='grid-column:span 2;height:16px'></div><div id=c style='height:16px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(16, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(60, lot52_node("c")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("c")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_dense_row_15(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:90px;grid-template-columns:repeat(3,1fr);grid-auto-flow:dense row;align-items:stretch'><div id=a style='grid-column:span 2;height:16px'></div><div id=b style='grid-column:span 2;height:16px'></div><div id=c style='height:16px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(16, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(60, lot52_node("c")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("c")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_dense_row_16(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:90px;grid-template-columns:repeat(3,1fr);grid-auto-flow:dense;align-items:stretch'><div id=a style='grid-column:span 2;height:18px'></div><div id=b style='grid-column:span 2;height:18px'></div><div id=c style='height:18px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(18, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(60, lot52_node("c")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("c")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_dense_row_17(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:90px;grid-template-columns:repeat(3,1fr);grid-auto-flow:row dense;align-items:stretch'><div id=a style='grid-column:span 2;height:18px'></div><div id=b style='grid-column:span 2;height:18px'></div><div id=c style='height:18px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(18, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(60, lot52_node("c")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("c")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_dense_row_18(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:90px;grid-template-columns:repeat(3,1fr);grid-auto-flow:dense row;align-items:stretch'><div id=a style='grid-column:span 2;height:18px'></div><div id=b style='grid-column:span 2;height:18px'></div><div id=c style='height:18px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(18, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(60, lot52_node("c")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("c")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_dense_row_19(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:90px;grid-template-columns:repeat(3,1fr);grid-auto-flow:dense;align-items:stretch'><div id=a style='grid-column:span 2;height:20px'></div><div id=b style='grid-column:span 2;height:20px'></div><div id=c style='height:20px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(20, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(60, lot52_node("c")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("c")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_dense_row_20(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:90px;grid-template-columns:repeat(3,1fr);grid-auto-flow:row dense;align-items:stretch'><div id=a style='grid-column:span 2;height:20px'></div><div id=b style='grid-column:span 2;height:20px'></div><div id=c style='height:20px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(20, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(60, lot52_node("c")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("c")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_dense_row_21(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:90px;grid-template-columns:repeat(3,1fr);grid-auto-flow:dense row;align-items:stretch'><div id=a style='grid-column:span 2;height:20px'></div><div id=b style='grid-column:span 2;height:20px'></div><div id=c style='height:20px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(20, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(60, lot52_node("c")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("c")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_dense_row_22(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:120px;grid-template-columns:repeat(3,1fr);grid-auto-flow:dense;align-items:stretch'><div id=a style='grid-column:span 2;height:8px'></div><div id=b style='grid-column:span 2;height:8px'></div><div id=c style='height:8px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(8, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(80, lot52_node("c")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("c")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_dense_row_23(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:120px;grid-template-columns:repeat(3,1fr);grid-auto-flow:row dense;align-items:stretch'><div id=a style='grid-column:span 2;height:8px'></div><div id=b style='grid-column:span 2;height:8px'></div><div id=c style='height:8px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(8, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(80, lot52_node("c")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("c")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_dense_row_24(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:120px;grid-template-columns:repeat(3,1fr);grid-auto-flow:dense row;align-items:stretch'><div id=a style='grid-column:span 2;height:8px'></div><div id=b style='grid-column:span 2;height:8px'></div><div id=c style='height:8px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(8, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(80, lot52_node("c")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("c")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_dense_row_25(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:120px;grid-template-columns:repeat(3,1fr);grid-auto-flow:dense;align-items:stretch'><div id=a style='grid-column:span 2;height:10px'></div><div id=b style='grid-column:span 2;height:10px'></div><div id=c style='height:10px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(80, lot52_node("c")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("c")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_dense_row_26(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:120px;grid-template-columns:repeat(3,1fr);grid-auto-flow:row dense;align-items:stretch'><div id=a style='grid-column:span 2;height:10px'></div><div id=b style='grid-column:span 2;height:10px'></div><div id=c style='height:10px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(80, lot52_node("c")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("c")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_dense_row_27(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:120px;grid-template-columns:repeat(3,1fr);grid-auto-flow:dense row;align-items:stretch'><div id=a style='grid-column:span 2;height:10px'></div><div id=b style='grid-column:span 2;height:10px'></div><div id=c style='height:10px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(80, lot52_node("c")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("c")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_dense_row_28(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:120px;grid-template-columns:repeat(3,1fr);grid-auto-flow:dense;align-items:stretch'><div id=a style='grid-column:span 2;height:12px'></div><div id=b style='grid-column:span 2;height:12px'></div><div id=c style='height:12px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(12, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(80, lot52_node("c")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("c")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_dense_row_29(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:120px;grid-template-columns:repeat(3,1fr);grid-auto-flow:row dense;align-items:stretch'><div id=a style='grid-column:span 2;height:12px'></div><div id=b style='grid-column:span 2;height:12px'></div><div id=c style='height:12px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(12, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(80, lot52_node("c")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("c")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_dense_row_30(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:120px;grid-template-columns:repeat(3,1fr);grid-auto-flow:dense row;align-items:stretch'><div id=a style='grid-column:span 2;height:12px'></div><div id=b style='grid-column:span 2;height:12px'></div><div id=c style='height:12px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(12, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(80, lot52_node("c")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("c")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_dense_row_31(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:120px;grid-template-columns:repeat(3,1fr);grid-auto-flow:dense;align-items:stretch'><div id=a style='grid-column:span 2;height:14px'></div><div id=b style='grid-column:span 2;height:14px'></div><div id=c style='height:14px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(14, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(80, lot52_node("c")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("c")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_dense_row_32(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:120px;grid-template-columns:repeat(3,1fr);grid-auto-flow:row dense;align-items:stretch'><div id=a style='grid-column:span 2;height:14px'></div><div id=b style='grid-column:span 2;height:14px'></div><div id=c style='height:14px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(14, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(80, lot52_node("c")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("c")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_dense_row_33(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:120px;grid-template-columns:repeat(3,1fr);grid-auto-flow:dense row;align-items:stretch'><div id=a style='grid-column:span 2;height:14px'></div><div id=b style='grid-column:span 2;height:14px'></div><div id=c style='height:14px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(14, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(80, lot52_node("c")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("c")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_dense_row_34(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:120px;grid-template-columns:repeat(3,1fr);grid-auto-flow:dense;align-items:stretch'><div id=a style='grid-column:span 2;height:16px'></div><div id=b style='grid-column:span 2;height:16px'></div><div id=c style='height:16px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(16, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(80, lot52_node("c")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("c")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_dense_row_35(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:120px;grid-template-columns:repeat(3,1fr);grid-auto-flow:row dense;align-items:stretch'><div id=a style='grid-column:span 2;height:16px'></div><div id=b style='grid-column:span 2;height:16px'></div><div id=c style='height:16px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(16, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(80, lot52_node("c")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("c")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_dense_row_36(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:120px;grid-template-columns:repeat(3,1fr);grid-auto-flow:dense row;align-items:stretch'><div id=a style='grid-column:span 2;height:16px'></div><div id=b style='grid-column:span 2;height:16px'></div><div id=c style='height:16px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(16, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(80, lot52_node("c")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("c")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_dense_row_37(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:120px;grid-template-columns:repeat(3,1fr);grid-auto-flow:dense;align-items:stretch'><div id=a style='grid-column:span 2;height:18px'></div><div id=b style='grid-column:span 2;height:18px'></div><div id=c style='height:18px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(18, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(80, lot52_node("c")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("c")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_dense_row_38(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:120px;grid-template-columns:repeat(3,1fr);grid-auto-flow:row dense;align-items:stretch'><div id=a style='grid-column:span 2;height:18px'></div><div id=b style='grid-column:span 2;height:18px'></div><div id=c style='height:18px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(18, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(80, lot52_node("c")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("c")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_dense_row_39(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:120px;grid-template-columns:repeat(3,1fr);grid-auto-flow:dense row;align-items:stretch'><div id=a style='grid-column:span 2;height:18px'></div><div id=b style='grid-column:span 2;height:18px'></div><div id=c style='height:18px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(18, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(80, lot52_node("c")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("c")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_dense_row_40(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:120px;grid-template-columns:repeat(3,1fr);grid-auto-flow:dense;align-items:stretch'><div id=a style='grid-column:span 2;height:20px'></div><div id=b style='grid-column:span 2;height:20px'></div><div id=c style='height:20px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(20, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(80, lot52_node("c")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("c")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_dense_row_41(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:120px;grid-template-columns:repeat(3,1fr);grid-auto-flow:row dense;align-items:stretch'><div id=a style='grid-column:span 2;height:20px'></div><div id=b style='grid-column:span 2;height:20px'></div><div id=c style='height:20px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(20, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(80, lot52_node("c")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("c")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_dense_row_42(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:120px;grid-template-columns:repeat(3,1fr);grid-auto-flow:dense row;align-items:stretch'><div id=a style='grid-column:span 2;height:20px'></div><div id=b style='grid-column:span 2;height:20px'></div><div id=c style='height:20px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(20, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(80, lot52_node("c")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("c")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_dense_row_43(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:150px;grid-template-columns:repeat(3,1fr);grid-auto-flow:dense;align-items:stretch'><div id=a style='grid-column:span 2;height:8px'></div><div id=b style='grid-column:span 2;height:8px'></div><div id=c style='height:8px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(8, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(100, lot52_node("c")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("c")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_dense_row_44(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:150px;grid-template-columns:repeat(3,1fr);grid-auto-flow:row dense;align-items:stretch'><div id=a style='grid-column:span 2;height:8px'></div><div id=b style='grid-column:span 2;height:8px'></div><div id=c style='height:8px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(8, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(100, lot52_node("c")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("c")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_dense_row_45(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:150px;grid-template-columns:repeat(3,1fr);grid-auto-flow:dense row;align-items:stretch'><div id=a style='grid-column:span 2;height:8px'></div><div id=b style='grid-column:span 2;height:8px'></div><div id=c style='height:8px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(8, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(100, lot52_node("c")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("c")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_dense_row_46(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:150px;grid-template-columns:repeat(3,1fr);grid-auto-flow:dense;align-items:stretch'><div id=a style='grid-column:span 2;height:10px'></div><div id=b style='grid-column:span 2;height:10px'></div><div id=c style='height:10px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(100, lot52_node("c")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("c")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_dense_row_47(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:150px;grid-template-columns:repeat(3,1fr);grid-auto-flow:row dense;align-items:stretch'><div id=a style='grid-column:span 2;height:10px'></div><div id=b style='grid-column:span 2;height:10px'></div><div id=c style='height:10px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(100, lot52_node("c")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("c")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_dense_row_48(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:150px;grid-template-columns:repeat(3,1fr);grid-auto-flow:dense row;align-items:stretch'><div id=a style='grid-column:span 2;height:10px'></div><div id=b style='grid-column:span 2;height:10px'></div><div id=c style='height:10px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(100, lot52_node("c")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("c")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_dense_row_49(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:150px;grid-template-columns:repeat(3,1fr);grid-auto-flow:dense;align-items:stretch'><div id=a style='grid-column:span 2;height:12px'></div><div id=b style='grid-column:span 2;height:12px'></div><div id=c style='height:12px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(12, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(100, lot52_node("c")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("c")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_dense_row_50(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:150px;grid-template-columns:repeat(3,1fr);grid-auto-flow:row dense;align-items:stretch'><div id=a style='grid-column:span 2;height:12px'></div><div id=b style='grid-column:span 2;height:12px'></div><div id=c style='height:12px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(12, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(100, lot52_node("c")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("c")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_dense_row_51(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:150px;grid-template-columns:repeat(3,1fr);grid-auto-flow:dense row;align-items:stretch'><div id=a style='grid-column:span 2;height:12px'></div><div id=b style='grid-column:span 2;height:12px'></div><div id=c style='height:12px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(12, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(100, lot52_node("c")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("c")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_dense_row_52(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:150px;grid-template-columns:repeat(3,1fr);grid-auto-flow:dense;align-items:stretch'><div id=a style='grid-column:span 2;height:14px'></div><div id=b style='grid-column:span 2;height:14px'></div><div id=c style='height:14px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(14, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(100, lot52_node("c")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("c")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_dense_row_53(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:150px;grid-template-columns:repeat(3,1fr);grid-auto-flow:row dense;align-items:stretch'><div id=a style='grid-column:span 2;height:14px'></div><div id=b style='grid-column:span 2;height:14px'></div><div id=c style='height:14px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(14, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(100, lot52_node("c")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("c")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_dense_row_54(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:150px;grid-template-columns:repeat(3,1fr);grid-auto-flow:dense row;align-items:stretch'><div id=a style='grid-column:span 2;height:14px'></div><div id=b style='grid-column:span 2;height:14px'></div><div id=c style='height:14px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(14, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(100, lot52_node("c")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("c")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_dense_row_55(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:150px;grid-template-columns:repeat(3,1fr);grid-auto-flow:dense;align-items:stretch'><div id=a style='grid-column:span 2;height:16px'></div><div id=b style='grid-column:span 2;height:16px'></div><div id=c style='height:16px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(16, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(100, lot52_node("c")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("c")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_dense_row_56(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:150px;grid-template-columns:repeat(3,1fr);grid-auto-flow:row dense;align-items:stretch'><div id=a style='grid-column:span 2;height:16px'></div><div id=b style='grid-column:span 2;height:16px'></div><div id=c style='height:16px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(16, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(100, lot52_node("c")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("c")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_dense_row_57(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:150px;grid-template-columns:repeat(3,1fr);grid-auto-flow:dense row;align-items:stretch'><div id=a style='grid-column:span 2;height:16px'></div><div id=b style='grid-column:span 2;height:16px'></div><div id=c style='height:16px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(16, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(100, lot52_node("c")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("c")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_dense_row_58(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:150px;grid-template-columns:repeat(3,1fr);grid-auto-flow:dense;align-items:stretch'><div id=a style='grid-column:span 2;height:18px'></div><div id=b style='grid-column:span 2;height:18px'></div><div id=c style='height:18px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(18, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(100, lot52_node("c")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("c")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_dense_row_59(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:150px;grid-template-columns:repeat(3,1fr);grid-auto-flow:row dense;align-items:stretch'><div id=a style='grid-column:span 2;height:18px'></div><div id=b style='grid-column:span 2;height:18px'></div><div id=c style='height:18px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(18, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(100, lot52_node("c")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("c")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_dense_row_60(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:150px;grid-template-columns:repeat(3,1fr);grid-auto-flow:dense row;align-items:stretch'><div id=a style='grid-column:span 2;height:18px'></div><div id=b style='grid-column:span 2;height:18px'></div><div id=c style='height:18px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(18, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(100, lot52_node("c")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("c")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_dense_row_61(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:150px;grid-template-columns:repeat(3,1fr);grid-auto-flow:dense;align-items:stretch'><div id=a style='grid-column:span 2;height:20px'></div><div id=b style='grid-column:span 2;height:20px'></div><div id=c style='height:20px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(20, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(100, lot52_node("c")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("c")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_dense_row_62(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:150px;grid-template-columns:repeat(3,1fr);grid-auto-flow:row dense;align-items:stretch'><div id=a style='grid-column:span 2;height:20px'></div><div id=b style='grid-column:span 2;height:20px'></div><div id=c style='height:20px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(20, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(100, lot52_node("c")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("c")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_dense_row_63(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:150px;grid-template-columns:repeat(3,1fr);grid-auto-flow:dense row;align-items:stretch'><div id=a style='grid-column:span 2;height:20px'></div><div id=b style='grid-column:span 2;height:20px'></div><div id=c style='height:20px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(20, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(100, lot52_node("c")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("c")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_dense_row_64(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:180px;grid-template-columns:repeat(3,1fr);grid-auto-flow:dense;align-items:stretch'><div id=a style='grid-column:span 2;height:8px'></div><div id=b style='grid-column:span 2;height:8px'></div><div id=c style='height:8px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(8, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(120, lot52_node("c")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("c")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_dense_row_65(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:180px;grid-template-columns:repeat(3,1fr);grid-auto-flow:row dense;align-items:stretch'><div id=a style='grid-column:span 2;height:8px'></div><div id=b style='grid-column:span 2;height:8px'></div><div id=c style='height:8px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(8, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(120, lot52_node("c")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("c")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_dense_row_66(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:180px;grid-template-columns:repeat(3,1fr);grid-auto-flow:dense row;align-items:stretch'><div id=a style='grid-column:span 2;height:8px'></div><div id=b style='grid-column:span 2;height:8px'></div><div id=c style='height:8px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(8, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(120, lot52_node("c")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("c")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_dense_row_67(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:180px;grid-template-columns:repeat(3,1fr);grid-auto-flow:dense;align-items:stretch'><div id=a style='grid-column:span 2;height:10px'></div><div id=b style='grid-column:span 2;height:10px'></div><div id=c style='height:10px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(120, lot52_node("c")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("c")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_dense_row_68(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:180px;grid-template-columns:repeat(3,1fr);grid-auto-flow:row dense;align-items:stretch'><div id=a style='grid-column:span 2;height:10px'></div><div id=b style='grid-column:span 2;height:10px'></div><div id=c style='height:10px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(120, lot52_node("c")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("c")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_dense_row_69(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:180px;grid-template-columns:repeat(3,1fr);grid-auto-flow:dense row;align-items:stretch'><div id=a style='grid-column:span 2;height:10px'></div><div id=b style='grid-column:span 2;height:10px'></div><div id=c style='height:10px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(120, lot52_node("c")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("c")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_dense_row_70(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:180px;grid-template-columns:repeat(3,1fr);grid-auto-flow:dense;align-items:stretch'><div id=a style='grid-column:span 2;height:12px'></div><div id=b style='grid-column:span 2;height:12px'></div><div id=c style='height:12px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(12, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(120, lot52_node("c")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("c")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_dense_row_71(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:180px;grid-template-columns:repeat(3,1fr);grid-auto-flow:row dense;align-items:stretch'><div id=a style='grid-column:span 2;height:12px'></div><div id=b style='grid-column:span 2;height:12px'></div><div id=c style='height:12px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(12, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(120, lot52_node("c")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("c")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_dense_row_72(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:180px;grid-template-columns:repeat(3,1fr);grid-auto-flow:dense row;align-items:stretch'><div id=a style='grid-column:span 2;height:12px'></div><div id=b style='grid-column:span 2;height:12px'></div><div id=c style='height:12px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(12, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(120, lot52_node("c")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("c")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_dense_row_73(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:180px;grid-template-columns:repeat(3,1fr);grid-auto-flow:dense;align-items:stretch'><div id=a style='grid-column:span 2;height:14px'></div><div id=b style='grid-column:span 2;height:14px'></div><div id=c style='height:14px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(14, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(120, lot52_node("c")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("c")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_dense_row_74(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:180px;grid-template-columns:repeat(3,1fr);grid-auto-flow:row dense;align-items:stretch'><div id=a style='grid-column:span 2;height:14px'></div><div id=b style='grid-column:span 2;height:14px'></div><div id=c style='height:14px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(14, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(120, lot52_node("c")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("c")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_dense_row_75(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:180px;grid-template-columns:repeat(3,1fr);grid-auto-flow:dense row;align-items:stretch'><div id=a style='grid-column:span 2;height:14px'></div><div id=b style='grid-column:span 2;height:14px'></div><div id=c style='height:14px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(14, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(120, lot52_node("c")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("c")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_dense_row_76(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:180px;grid-template-columns:repeat(3,1fr);grid-auto-flow:dense;align-items:stretch'><div id=a style='grid-column:span 2;height:16px'></div><div id=b style='grid-column:span 2;height:16px'></div><div id=c style='height:16px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(16, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(120, lot52_node("c")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("c")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_dense_row_77(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:180px;grid-template-columns:repeat(3,1fr);grid-auto-flow:row dense;align-items:stretch'><div id=a style='grid-column:span 2;height:16px'></div><div id=b style='grid-column:span 2;height:16px'></div><div id=c style='height:16px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(16, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(120, lot52_node("c")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("c")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_dense_row_78(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:180px;grid-template-columns:repeat(3,1fr);grid-auto-flow:dense row;align-items:stretch'><div id=a style='grid-column:span 2;height:16px'></div><div id=b style='grid-column:span 2;height:16px'></div><div id=c style='height:16px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(16, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(120, lot52_node("c")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("c")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_dense_row_79(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:180px;grid-template-columns:repeat(3,1fr);grid-auto-flow:dense;align-items:stretch'><div id=a style='grid-column:span 2;height:18px'></div><div id=b style='grid-column:span 2;height:18px'></div><div id=c style='height:18px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(18, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(120, lot52_node("c")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("c")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_dense_row_80(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:180px;grid-template-columns:repeat(3,1fr);grid-auto-flow:row dense;align-items:stretch'><div id=a style='grid-column:span 2;height:18px'></div><div id=b style='grid-column:span 2;height:18px'></div><div id=c style='height:18px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(18, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(120, lot52_node("c")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("c")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_dense_row_81(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:180px;grid-template-columns:repeat(3,1fr);grid-auto-flow:dense row;align-items:stretch'><div id=a style='grid-column:span 2;height:18px'></div><div id=b style='grid-column:span 2;height:18px'></div><div id=c style='height:18px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(18, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(120, lot52_node("c")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("c")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_dense_row_82(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:180px;grid-template-columns:repeat(3,1fr);grid-auto-flow:dense;align-items:stretch'><div id=a style='grid-column:span 2;height:20px'></div><div id=b style='grid-column:span 2;height:20px'></div><div id=c style='height:20px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(20, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(120, lot52_node("c")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("c")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_dense_row_83(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:180px;grid-template-columns:repeat(3,1fr);grid-auto-flow:row dense;align-items:stretch'><div id=a style='grid-column:span 2;height:20px'></div><div id=b style='grid-column:span 2;height:20px'></div><div id=c style='height:20px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(20, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(120, lot52_node("c")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("c")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_dense_row_84(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:180px;grid-template-columns:repeat(3,1fr);grid-auto-flow:dense row;align-items:stretch'><div id=a style='grid-column:span 2;height:20px'></div><div id=b style='grid-column:span 2;height:20px'></div><div id=c style='height:20px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(20, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(120, lot52_node("c")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("c")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_sparse_row_85(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:90px;grid-template-columns:repeat(3,1fr);grid-auto-flow:row;align-items:stretch'><div id=a style='grid-column:span 2;height:8px'></div><div id=b style='grid-column:span 2;height:8px'></div><div id=c style='height:8px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(60, lot52_node("c")->x);
    TEST_ASSERT_EQUAL(8, lot52_node("c")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_sparse_row_86(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:90px;grid-template-columns:repeat(3,1fr);grid-auto-flow:row;align-items:stretch'><div id=a style='grid-column:span 2;height:10px'></div><div id=b style='grid-column:span 2;height:10px'></div><div id=c style='height:10px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(60, lot52_node("c")->x);
    TEST_ASSERT_EQUAL(10, lot52_node("c")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_sparse_row_87(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:90px;grid-template-columns:repeat(3,1fr);grid-auto-flow:row;align-items:stretch'><div id=a style='grid-column:span 2;height:12px'></div><div id=b style='grid-column:span 2;height:12px'></div><div id=c style='height:12px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(60, lot52_node("c")->x);
    TEST_ASSERT_EQUAL(12, lot52_node("c")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_sparse_row_88(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:90px;grid-template-columns:repeat(3,1fr);grid-auto-flow:row;align-items:stretch'><div id=a style='grid-column:span 2;height:14px'></div><div id=b style='grid-column:span 2;height:14px'></div><div id=c style='height:14px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(60, lot52_node("c")->x);
    TEST_ASSERT_EQUAL(14, lot52_node("c")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_sparse_row_89(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:120px;grid-template-columns:repeat(3,1fr);grid-auto-flow:row;align-items:stretch'><div id=a style='grid-column:span 2;height:8px'></div><div id=b style='grid-column:span 2;height:8px'></div><div id=c style='height:8px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(80, lot52_node("c")->x);
    TEST_ASSERT_EQUAL(8, lot52_node("c")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_sparse_row_90(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:120px;grid-template-columns:repeat(3,1fr);grid-auto-flow:row;align-items:stretch'><div id=a style='grid-column:span 2;height:10px'></div><div id=b style='grid-column:span 2;height:10px'></div><div id=c style='height:10px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(80, lot52_node("c")->x);
    TEST_ASSERT_EQUAL(10, lot52_node("c")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_sparse_row_91(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:120px;grid-template-columns:repeat(3,1fr);grid-auto-flow:row;align-items:stretch'><div id=a style='grid-column:span 2;height:12px'></div><div id=b style='grid-column:span 2;height:12px'></div><div id=c style='height:12px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(80, lot52_node("c")->x);
    TEST_ASSERT_EQUAL(12, lot52_node("c")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_sparse_row_92(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:120px;grid-template-columns:repeat(3,1fr);grid-auto-flow:row;align-items:stretch'><div id=a style='grid-column:span 2;height:14px'></div><div id=b style='grid-column:span 2;height:14px'></div><div id=c style='height:14px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(80, lot52_node("c")->x);
    TEST_ASSERT_EQUAL(14, lot52_node("c")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_sparse_row_93(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:150px;grid-template-columns:repeat(3,1fr);grid-auto-flow:row;align-items:stretch'><div id=a style='grid-column:span 2;height:8px'></div><div id=b style='grid-column:span 2;height:8px'></div><div id=c style='height:8px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(100, lot52_node("c")->x);
    TEST_ASSERT_EQUAL(8, lot52_node("c")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_sparse_row_94(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:150px;grid-template-columns:repeat(3,1fr);grid-auto-flow:row;align-items:stretch'><div id=a style='grid-column:span 2;height:10px'></div><div id=b style='grid-column:span 2;height:10px'></div><div id=c style='height:10px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(100, lot52_node("c")->x);
    TEST_ASSERT_EQUAL(10, lot52_node("c")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_sparse_row_95(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:150px;grid-template-columns:repeat(3,1fr);grid-auto-flow:row;align-items:stretch'><div id=a style='grid-column:span 2;height:12px'></div><div id=b style='grid-column:span 2;height:12px'></div><div id=c style='height:12px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(100, lot52_node("c")->x);
    TEST_ASSERT_EQUAL(12, lot52_node("c")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_sparse_row_96(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:150px;grid-template-columns:repeat(3,1fr);grid-auto-flow:row;align-items:stretch'><div id=a style='grid-column:span 2;height:14px'></div><div id=b style='grid-column:span 2;height:14px'></div><div id=c style='height:14px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(100, lot52_node("c")->x);
    TEST_ASSERT_EQUAL(14, lot52_node("c")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_sparse_row_97(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:180px;grid-template-columns:repeat(3,1fr);grid-auto-flow:row;align-items:stretch'><div id=a style='grid-column:span 2;height:8px'></div><div id=b style='grid-column:span 2;height:8px'></div><div id=c style='height:8px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(120, lot52_node("c")->x);
    TEST_ASSERT_EQUAL(8, lot52_node("c")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_sparse_row_98(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:180px;grid-template-columns:repeat(3,1fr);grid-auto-flow:row;align-items:stretch'><div id=a style='grid-column:span 2;height:10px'></div><div id=b style='grid-column:span 2;height:10px'></div><div id=c style='height:10px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(120, lot52_node("c")->x);
    TEST_ASSERT_EQUAL(10, lot52_node("c")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_sparse_row_99(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:180px;grid-template-columns:repeat(3,1fr);grid-auto-flow:row;align-items:stretch'><div id=a style='grid-column:span 2;height:12px'></div><div id=b style='grid-column:span 2;height:12px'></div><div id=c style='height:12px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(120, lot52_node("c")->x);
    TEST_ASSERT_EQUAL(12, lot52_node("c")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_sparse_row_100(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:180px;grid-template-columns:repeat(3,1fr);grid-auto-flow:row;align-items:stretch'><div id=a style='grid-column:span 2;height:14px'></div><div id=b style='grid-column:span 2;height:14px'></div><div id=c style='height:14px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(120, lot52_node("c")->x);
    TEST_ASSERT_EQUAL(14, lot52_node("c")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_dense_col_101(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:80px;grid-template-columns:repeat(2,1fr);grid-template-rows:repeat(3,8px);grid-auto-flow:column dense;align-items:stretch'><div id=a style='grid-row:span 2'></div><div id=b style='grid-row:span 2'></div><div id=c></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("c")->x);
    TEST_ASSERT_EQUAL(16, lot52_node("c")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_dense_col_102(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:80px;grid-template-columns:repeat(2,1fr);grid-template-rows:repeat(3,8px);grid-auto-flow:dense column;align-items:stretch'><div id=a style='grid-row:span 2'></div><div id=b style='grid-row:span 2'></div><div id=c></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("c")->x);
    TEST_ASSERT_EQUAL(16, lot52_node("c")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_dense_col_103(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:80px;grid-template-columns:repeat(2,1fr);grid-template-rows:repeat(3,10px);grid-auto-flow:column dense;align-items:stretch'><div id=a style='grid-row:span 2'></div><div id=b style='grid-row:span 2'></div><div id=c></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("c")->x);
    TEST_ASSERT_EQUAL(20, lot52_node("c")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_dense_col_104(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:80px;grid-template-columns:repeat(2,1fr);grid-template-rows:repeat(3,10px);grid-auto-flow:dense column;align-items:stretch'><div id=a style='grid-row:span 2'></div><div id=b style='grid-row:span 2'></div><div id=c></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("c")->x);
    TEST_ASSERT_EQUAL(20, lot52_node("c")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_dense_col_105(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:80px;grid-template-columns:repeat(2,1fr);grid-template-rows:repeat(3,12px);grid-auto-flow:column dense;align-items:stretch'><div id=a style='grid-row:span 2'></div><div id=b style='grid-row:span 2'></div><div id=c></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("c")->x);
    TEST_ASSERT_EQUAL(24, lot52_node("c")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_dense_col_106(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:80px;grid-template-columns:repeat(2,1fr);grid-template-rows:repeat(3,12px);grid-auto-flow:dense column;align-items:stretch'><div id=a style='grid-row:span 2'></div><div id=b style='grid-row:span 2'></div><div id=c></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("c")->x);
    TEST_ASSERT_EQUAL(24, lot52_node("c")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_dense_col_107(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:80px;grid-template-columns:repeat(2,1fr);grid-template-rows:repeat(3,14px);grid-auto-flow:column dense;align-items:stretch'><div id=a style='grid-row:span 2'></div><div id=b style='grid-row:span 2'></div><div id=c></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("c")->x);
    TEST_ASSERT_EQUAL(28, lot52_node("c")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_dense_col_108(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:80px;grid-template-columns:repeat(2,1fr);grid-template-rows:repeat(3,14px);grid-auto-flow:dense column;align-items:stretch'><div id=a style='grid-row:span 2'></div><div id=b style='grid-row:span 2'></div><div id=c></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("c")->x);
    TEST_ASSERT_EQUAL(28, lot52_node("c")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_dense_col_109(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:80px;grid-template-columns:repeat(2,1fr);grid-template-rows:repeat(3,16px);grid-auto-flow:column dense;align-items:stretch'><div id=a style='grid-row:span 2'></div><div id=b style='grid-row:span 2'></div><div id=c></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("c")->x);
    TEST_ASSERT_EQUAL(32, lot52_node("c")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_dense_col_110(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:80px;grid-template-columns:repeat(2,1fr);grid-template-rows:repeat(3,16px);grid-auto-flow:dense column;align-items:stretch'><div id=a style='grid-row:span 2'></div><div id=b style='grid-row:span 2'></div><div id=c></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("c")->x);
    TEST_ASSERT_EQUAL(32, lot52_node("c")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_dense_col_111(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:80px;grid-template-columns:repeat(2,1fr);grid-template-rows:repeat(3,18px);grid-auto-flow:column dense;align-items:stretch'><div id=a style='grid-row:span 2'></div><div id=b style='grid-row:span 2'></div><div id=c></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("c")->x);
    TEST_ASSERT_EQUAL(36, lot52_node("c")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_dense_col_112(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:80px;grid-template-columns:repeat(2,1fr);grid-template-rows:repeat(3,18px);grid-auto-flow:dense column;align-items:stretch'><div id=a style='grid-row:span 2'></div><div id=b style='grid-row:span 2'></div><div id=c></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("c")->x);
    TEST_ASSERT_EQUAL(36, lot52_node("c")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_dense_col_113(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:80px;grid-template-columns:repeat(2,1fr);grid-template-rows:repeat(3,20px);grid-auto-flow:column dense;align-items:stretch'><div id=a style='grid-row:span 2'></div><div id=b style='grid-row:span 2'></div><div id=c></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("c")->x);
    TEST_ASSERT_EQUAL(40, lot52_node("c")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_dense_col_114(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:80px;grid-template-columns:repeat(2,1fr);grid-template-rows:repeat(3,20px);grid-auto-flow:dense column;align-items:stretch'><div id=a style='grid-row:span 2'></div><div id=b style='grid-row:span 2'></div><div id=c></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("c")->x);
    TEST_ASSERT_EQUAL(40, lot52_node("c")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_flow_refuse_1(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:60px;grid-template-columns:1fr 1fr;grid-auto-flow:bogus'><div id=a style='height:8px'></div></div></body>", 480);
    TEST_ASSERT_TRUE(lot52_has_diag("grid flow refused"));
}

static void test_l79_flow_refuse_2(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:60px;grid-template-columns:1fr 1fr;grid-auto-flow:row column'><div id=a style='height:8px'></div></div></body>", 480);
    TEST_ASSERT_TRUE(lot52_has_diag("grid flow refused"));
}

static void test_l79_flow_refuse_3(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:60px;grid-template-columns:1fr 1fr;grid-auto-flow:dense dense'><div id=a style='height:8px'></div></div></body>", 480);
    TEST_ASSERT_TRUE(lot52_has_diag("grid flow refused"));
}

static void test_l79_flow_refuse_4(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:60px;grid-template-columns:1fr 1fr;grid-auto-flow:column row'><div id=a style='height:8px'></div></div></body>", 480);
    TEST_ASSERT_TRUE(lot52_has_diag("grid flow refused"));
}

static void test_l79_flow_refuse_5(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:60px;grid-template-columns:1fr 1fr;grid-auto-flow:row junk'><div id=a style='height:8px'></div></div></body>", 480);
    TEST_ASSERT_TRUE(lot52_has_diag("grid flow refused"));
}

static void test_l79_flow_refuse_6(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:60px;grid-template-columns:1fr 1fr;grid-auto-flow:masonry'><div id=a style='height:8px'></div></div></body>", 480);
    TEST_ASSERT_TRUE(lot52_has_diag("grid flow refused"));
}

static void test_l79_flow_refuse_7(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:60px;grid-template-columns:1fr 1fr;grid-auto-flow:row dense dense'><div id=a style='height:8px'></div></div></body>", 480);
    TEST_ASSERT_TRUE(lot52_has_diag("grid flow refused"));
}

static void test_l79_flow_refuse_8(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:60px;grid-template-columns:1fr 1fr;grid-auto-flow:foo'><div id=a style='height:8px'></div></div></body>", 480);
    TEST_ASSERT_TRUE(lot52_has_diag("grid flow refused"));
}

static void test_l79_dense_gap_115(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:60px;grid-template-columns:20px 20px 20px;column-gap:0px;grid-auto-flow:dense;align-items:stretch'><div id=a style='grid-column:span 2;height:8px'></div><div id=b style='grid-column:span 2;height:8px'></div><div id=c style='height:8px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(40, lot52_node("c")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("c")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_dense_gap_116(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:60px;grid-template-columns:20px 20px 20px;column-gap:0px;grid-auto-flow:dense;align-items:stretch'><div id=a style='grid-column:span 2;height:10px'></div><div id=b style='grid-column:span 2;height:10px'></div><div id=c style='height:10px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(40, lot52_node("c")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("c")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_dense_gap_117(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:60px;grid-template-columns:20px 20px 20px;column-gap:0px;grid-auto-flow:dense;align-items:stretch'><div id=a style='grid-column:span 2;height:12px'></div><div id=b style='grid-column:span 2;height:12px'></div><div id=c style='height:12px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(40, lot52_node("c")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("c")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_dense_gap_118(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:60px;grid-template-columns:20px 20px 20px;column-gap:0px;grid-auto-flow:dense;align-items:stretch'><div id=a style='grid-column:span 2;height:14px'></div><div id=b style='grid-column:span 2;height:14px'></div><div id=c style='height:14px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(40, lot52_node("c")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("c")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_dense_gap_119(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:60px;grid-template-columns:20px 20px 20px;column-gap:0px;grid-auto-flow:dense;align-items:stretch'><div id=a style='grid-column:span 2;height:16px'></div><div id=b style='grid-column:span 2;height:16px'></div><div id=c style='height:16px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(40, lot52_node("c")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("c")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_dense_gap_120(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:64px;grid-template-columns:20px 20px 20px;column-gap:2px;grid-auto-flow:dense;align-items:stretch'><div id=a style='grid-column:span 2;height:8px'></div><div id=b style='grid-column:span 2;height:8px'></div><div id=c style='height:8px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(44, lot52_node("c")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("c")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_dense_gap_121(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:64px;grid-template-columns:20px 20px 20px;column-gap:2px;grid-auto-flow:dense;align-items:stretch'><div id=a style='grid-column:span 2;height:10px'></div><div id=b style='grid-column:span 2;height:10px'></div><div id=c style='height:10px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(44, lot52_node("c")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("c")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_dense_gap_122(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:64px;grid-template-columns:20px 20px 20px;column-gap:2px;grid-auto-flow:dense;align-items:stretch'><div id=a style='grid-column:span 2;height:12px'></div><div id=b style='grid-column:span 2;height:12px'></div><div id=c style='height:12px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(44, lot52_node("c")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("c")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_dense_gap_123(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:64px;grid-template-columns:20px 20px 20px;column-gap:2px;grid-auto-flow:dense;align-items:stretch'><div id=a style='grid-column:span 2;height:14px'></div><div id=b style='grid-column:span 2;height:14px'></div><div id=c style='height:14px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(44, lot52_node("c")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("c")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_dense_gap_124(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:64px;grid-template-columns:20px 20px 20px;column-gap:2px;grid-auto-flow:dense;align-items:stretch'><div id=a style='grid-column:span 2;height:16px'></div><div id=b style='grid-column:span 2;height:16px'></div><div id=c style='height:16px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(44, lot52_node("c")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("c")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_dense_gap_125(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:68px;grid-template-columns:20px 20px 20px;column-gap:4px;grid-auto-flow:dense;align-items:stretch'><div id=a style='grid-column:span 2;height:8px'></div><div id=b style='grid-column:span 2;height:8px'></div><div id=c style='height:8px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(48, lot52_node("c")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("c")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_dense_gap_126(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:68px;grid-template-columns:20px 20px 20px;column-gap:4px;grid-auto-flow:dense;align-items:stretch'><div id=a style='grid-column:span 2;height:10px'></div><div id=b style='grid-column:span 2;height:10px'></div><div id=c style='height:10px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(48, lot52_node("c")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("c")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_dense_gap_127(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:68px;grid-template-columns:20px 20px 20px;column-gap:4px;grid-auto-flow:dense;align-items:stretch'><div id=a style='grid-column:span 2;height:12px'></div><div id=b style='grid-column:span 2;height:12px'></div><div id=c style='height:12px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(48, lot52_node("c")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("c")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_dense_gap_128(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:68px;grid-template-columns:20px 20px 20px;column-gap:4px;grid-auto-flow:dense;align-items:stretch'><div id=a style='grid-column:span 2;height:14px'></div><div id=b style='grid-column:span 2;height:14px'></div><div id=c style='height:14px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(48, lot52_node("c")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("c")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_dense_gap_129(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:68px;grid-template-columns:20px 20px 20px;column-gap:4px;grid-auto-flow:dense;align-items:stretch'><div id=a style='grid-column:span 2;height:16px'></div><div id=b style='grid-column:span 2;height:16px'></div><div id=c style='height:16px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(48, lot52_node("c")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("c")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_dense_gap_130(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:72px;grid-template-columns:20px 20px 20px;column-gap:6px;grid-auto-flow:dense;align-items:stretch'><div id=a style='grid-column:span 2;height:8px'></div><div id=b style='grid-column:span 2;height:8px'></div><div id=c style='height:8px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(52, lot52_node("c")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("c")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_dense_gap_131(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:72px;grid-template-columns:20px 20px 20px;column-gap:6px;grid-auto-flow:dense;align-items:stretch'><div id=a style='grid-column:span 2;height:10px'></div><div id=b style='grid-column:span 2;height:10px'></div><div id=c style='height:10px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(52, lot52_node("c")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("c")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_dense_gap_132(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:72px;grid-template-columns:20px 20px 20px;column-gap:6px;grid-auto-flow:dense;align-items:stretch'><div id=a style='grid-column:span 2;height:12px'></div><div id=b style='grid-column:span 2;height:12px'></div><div id=c style='height:12px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(52, lot52_node("c")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("c")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_dense_gap_133(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:72px;grid-template-columns:20px 20px 20px;column-gap:6px;grid-auto-flow:dense;align-items:stretch'><div id=a style='grid-column:span 2;height:14px'></div><div id=b style='grid-column:span 2;height:14px'></div><div id=c style='height:14px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(52, lot52_node("c")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("c")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_dense_gap_134(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:72px;grid-template-columns:20px 20px 20px;column-gap:6px;grid-auto-flow:dense;align-items:stretch'><div id=a style='grid-column:span 2;height:16px'></div><div id=b style='grid-column:span 2;height:16px'></div><div id=c style='height:16px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(52, lot52_node("c")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("c")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_dense_gap_135(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:76px;grid-template-columns:20px 20px 20px;column-gap:8px;grid-auto-flow:dense;align-items:stretch'><div id=a style='grid-column:span 2;height:8px'></div><div id=b style='grid-column:span 2;height:8px'></div><div id=c style='height:8px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(56, lot52_node("c")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("c")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_dense_gap_136(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:76px;grid-template-columns:20px 20px 20px;column-gap:8px;grid-auto-flow:dense;align-items:stretch'><div id=a style='grid-column:span 2;height:10px'></div><div id=b style='grid-column:span 2;height:10px'></div><div id=c style='height:10px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(56, lot52_node("c")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("c")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_dense_gap_137(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:76px;grid-template-columns:20px 20px 20px;column-gap:8px;grid-auto-flow:dense;align-items:stretch'><div id=a style='grid-column:span 2;height:12px'></div><div id=b style='grid-column:span 2;height:12px'></div><div id=c style='height:12px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(56, lot52_node("c")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("c")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_dense_gap_138(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:76px;grid-template-columns:20px 20px 20px;column-gap:8px;grid-auto-flow:dense;align-items:stretch'><div id=a style='grid-column:span 2;height:14px'></div><div id=b style='grid-column:span 2;height:14px'></div><div id=c style='height:14px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(56, lot52_node("c")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("c")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_dense_gap_139(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:76px;grid-template-columns:20px 20px 20px;column-gap:8px;grid-auto-flow:dense;align-items:stretch'><div id=a style='grid-column:span 2;height:16px'></div><div id=b style='grid-column:span 2;height:16px'></div><div id=c style='height:16px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(56, lot52_node("c")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("c")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_dense_gap_140(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:80px;grid-template-columns:20px 20px 20px;column-gap:10px;grid-auto-flow:dense;align-items:stretch'><div id=a style='grid-column:span 2;height:8px'></div><div id=b style='grid-column:span 2;height:8px'></div><div id=c style='height:8px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(60, lot52_node("c")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("c")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_dense_gap_141(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:80px;grid-template-columns:20px 20px 20px;column-gap:10px;grid-auto-flow:dense;align-items:stretch'><div id=a style='grid-column:span 2;height:10px'></div><div id=b style='grid-column:span 2;height:10px'></div><div id=c style='height:10px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(60, lot52_node("c")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("c")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_dense_gap_142(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:80px;grid-template-columns:20px 20px 20px;column-gap:10px;grid-auto-flow:dense;align-items:stretch'><div id=a style='grid-column:span 2;height:12px'></div><div id=b style='grid-column:span 2;height:12px'></div><div id=c style='height:12px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(60, lot52_node("c")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("c")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_dense_gap_143(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:80px;grid-template-columns:20px 20px 20px;column-gap:10px;grid-auto-flow:dense;align-items:stretch'><div id=a style='grid-column:span 2;height:14px'></div><div id=b style='grid-column:span 2;height:14px'></div><div id=c style='height:14px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(60, lot52_node("c")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("c")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_dense_gap_144(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:80px;grid-template-columns:20px 20px 20px;column-gap:10px;grid-auto-flow:dense;align-items:stretch'><div id=a style='grid-column:span 2;height:16px'></div><div id=b style='grid-column:span 2;height:16px'></div><div id=c style='height:16px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(60, lot52_node("c")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("c")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_dense_span_145(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:60px;grid-template-columns:repeat(3,20px);grid-auto-flow:dense;align-items:stretch'><div id=a style='grid-column:span 2;height:6px'></div><div id=b style='grid-column:span 2;height:6px'></div><div id=c style='height:6px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(40, lot52_node("c")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("c")->y);
    TEST_ASSERT_EQUAL(6, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_dense_span_146(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:60px;grid-template-columns:repeat(3,20px);grid-auto-flow:dense;align-items:stretch'><div id=a style='grid-column:span 2;height:8px'></div><div id=b style='grid-column:span 2;height:8px'></div><div id=c style='height:8px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(40, lot52_node("c")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("c")->y);
    TEST_ASSERT_EQUAL(8, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_dense_span_147(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:60px;grid-template-columns:repeat(3,20px);grid-auto-flow:dense;align-items:stretch'><div id=a style='grid-column:span 2;height:10px'></div><div id=b style='grid-column:span 2;height:10px'></div><div id=c style='height:10px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(40, lot52_node("c")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("c")->y);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_dense_span_148(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:60px;grid-template-columns:repeat(3,20px);grid-auto-flow:dense;align-items:stretch'><div id=a style='grid-column:span 2;height:12px'></div><div id=b style='grid-column:span 2;height:12px'></div><div id=c style='height:12px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(40, lot52_node("c")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("c")->y);
    TEST_ASSERT_EQUAL(12, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_dense_span_149(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:60px;grid-template-columns:repeat(3,20px);grid-auto-flow:dense;align-items:stretch'><div id=a style='grid-column:span 2;height:14px'></div><div id=b style='grid-column:span 2;height:14px'></div><div id=c style='height:14px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(40, lot52_node("c")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("c")->y);
    TEST_ASSERT_EQUAL(14, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_dense_span_150(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:60px;grid-template-columns:repeat(3,20px);grid-auto-flow:dense;align-items:stretch'><div id=a style='grid-column:span 2;height:16px'></div><div id=b style='grid-column:span 2;height:16px'></div><div id=c style='height:16px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(40, lot52_node("c")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("c")->y);
    TEST_ASSERT_EQUAL(16, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_dense_span_151(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:60px;grid-template-columns:repeat(3,20px);grid-auto-flow:dense;align-items:stretch'><div id=a style='grid-column:span 2;height:18px'></div><div id=b style='grid-column:span 2;height:18px'></div><div id=c style='height:18px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(40, lot52_node("c")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("c")->y);
    TEST_ASSERT_EQUAL(18, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_dense_span_152(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:60px;grid-template-columns:repeat(3,20px);grid-auto-flow:dense;align-items:stretch'><div id=a style='grid-column:span 2;height:20px'></div><div id=b style='grid-column:span 2;height:20px'></div><div id=c style='height:20px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(40, lot52_node("c")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("c")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_dense_span_153(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:60px;grid-template-columns:repeat(3,20px);grid-auto-flow:dense;align-items:stretch'><div id=a style='grid-column:span 2;height:22px'></div><div id=b style='grid-column:span 2;height:22px'></div><div id=c style='height:22px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(40, lot52_node("c")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("c")->y);
    TEST_ASSERT_EQUAL(22, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_dense_span_154(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:60px;grid-template-columns:repeat(3,20px);grid-auto-flow:dense;align-items:stretch'><div id=a style='grid-column:span 2;height:24px'></div><div id=b style='grid-column:span 2;height:24px'></div><div id=c style='height:24px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(40, lot52_node("c")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("c")->y);
    TEST_ASSERT_EQUAL(24, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_dense_span_155(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:80px;grid-template-columns:repeat(4,20px);grid-auto-flow:dense;align-items:stretch'><div id=a style='grid-column:span 3;height:6px'></div><div id=b style='grid-column:span 3;height:6px'></div><div id=c style='height:6px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(60, lot52_node("c")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("c")->y);
    TEST_ASSERT_EQUAL(6, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_dense_span_156(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:80px;grid-template-columns:repeat(4,20px);grid-auto-flow:dense;align-items:stretch'><div id=a style='grid-column:span 3;height:8px'></div><div id=b style='grid-column:span 3;height:8px'></div><div id=c style='height:8px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(60, lot52_node("c")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("c")->y);
    TEST_ASSERT_EQUAL(8, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_dense_span_157(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:80px;grid-template-columns:repeat(4,20px);grid-auto-flow:dense;align-items:stretch'><div id=a style='grid-column:span 3;height:10px'></div><div id=b style='grid-column:span 3;height:10px'></div><div id=c style='height:10px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(60, lot52_node("c")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("c")->y);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_dense_span_158(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:80px;grid-template-columns:repeat(4,20px);grid-auto-flow:dense;align-items:stretch'><div id=a style='grid-column:span 3;height:12px'></div><div id=b style='grid-column:span 3;height:12px'></div><div id=c style='height:12px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(60, lot52_node("c")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("c")->y);
    TEST_ASSERT_EQUAL(12, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_dense_span_159(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:80px;grid-template-columns:repeat(4,20px);grid-auto-flow:dense;align-items:stretch'><div id=a style='grid-column:span 3;height:14px'></div><div id=b style='grid-column:span 3;height:14px'></div><div id=c style='height:14px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(60, lot52_node("c")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("c")->y);
    TEST_ASSERT_EQUAL(14, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_dense_span_160(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:80px;grid-template-columns:repeat(4,20px);grid-auto-flow:dense;align-items:stretch'><div id=a style='grid-column:span 3;height:16px'></div><div id=b style='grid-column:span 3;height:16px'></div><div id=c style='height:16px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(60, lot52_node("c")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("c")->y);
    TEST_ASSERT_EQUAL(16, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_dense_span_161(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:80px;grid-template-columns:repeat(4,20px);grid-auto-flow:dense;align-items:stretch'><div id=a style='grid-column:span 3;height:18px'></div><div id=b style='grid-column:span 3;height:18px'></div><div id=c style='height:18px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(60, lot52_node("c")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("c")->y);
    TEST_ASSERT_EQUAL(18, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_dense_span_162(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:80px;grid-template-columns:repeat(4,20px);grid-auto-flow:dense;align-items:stretch'><div id=a style='grid-column:span 3;height:20px'></div><div id=b style='grid-column:span 3;height:20px'></div><div id=c style='height:20px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(60, lot52_node("c")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("c")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_dense_span_163(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:80px;grid-template-columns:repeat(4,20px);grid-auto-flow:dense;align-items:stretch'><div id=a style='grid-column:span 3;height:22px'></div><div id=b style='grid-column:span 3;height:22px'></div><div id=c style='height:22px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(60, lot52_node("c")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("c")->y);
    TEST_ASSERT_EQUAL(22, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_dense_span_164(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:80px;grid-template-columns:repeat(4,20px);grid-auto-flow:dense;align-items:stretch'><div id=a style='grid-column:span 3;height:24px'></div><div id=b style='grid-column:span 3;height:24px'></div><div id=c style='height:24px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(60, lot52_node("c")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("c")->y);
    TEST_ASSERT_EQUAL(24, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_dense_span_165(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:100px;grid-template-columns:repeat(5,20px);grid-auto-flow:dense;align-items:stretch'><div id=a style='grid-column:span 3;height:6px'></div><div id=b style='grid-column:span 3;height:6px'></div><div id=c style='height:6px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(60, lot52_node("c")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("c")->y);
    TEST_ASSERT_EQUAL(6, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_dense_span_166(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:100px;grid-template-columns:repeat(5,20px);grid-auto-flow:dense;align-items:stretch'><div id=a style='grid-column:span 3;height:8px'></div><div id=b style='grid-column:span 3;height:8px'></div><div id=c style='height:8px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(60, lot52_node("c")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("c")->y);
    TEST_ASSERT_EQUAL(8, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_dense_span_167(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:100px;grid-template-columns:repeat(5,20px);grid-auto-flow:dense;align-items:stretch'><div id=a style='grid-column:span 3;height:10px'></div><div id=b style='grid-column:span 3;height:10px'></div><div id=c style='height:10px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(60, lot52_node("c")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("c")->y);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_dense_span_168(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:100px;grid-template-columns:repeat(5,20px);grid-auto-flow:dense;align-items:stretch'><div id=a style='grid-column:span 3;height:12px'></div><div id=b style='grid-column:span 3;height:12px'></div><div id=c style='height:12px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(60, lot52_node("c")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("c")->y);
    TEST_ASSERT_EQUAL(12, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_dense_span_169(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:100px;grid-template-columns:repeat(5,20px);grid-auto-flow:dense;align-items:stretch'><div id=a style='grid-column:span 3;height:14px'></div><div id=b style='grid-column:span 3;height:14px'></div><div id=c style='height:14px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(60, lot52_node("c")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("c")->y);
    TEST_ASSERT_EQUAL(14, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_dense_span_170(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:100px;grid-template-columns:repeat(5,20px);grid-auto-flow:dense;align-items:stretch'><div id=a style='grid-column:span 3;height:16px'></div><div id=b style='grid-column:span 3;height:16px'></div><div id=c style='height:16px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(60, lot52_node("c")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("c")->y);
    TEST_ASSERT_EQUAL(16, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_dense_span_171(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:100px;grid-template-columns:repeat(5,20px);grid-auto-flow:dense;align-items:stretch'><div id=a style='grid-column:span 3;height:18px'></div><div id=b style='grid-column:span 3;height:18px'></div><div id=c style='height:18px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(60, lot52_node("c")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("c")->y);
    TEST_ASSERT_EQUAL(18, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_dense_span_172(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:100px;grid-template-columns:repeat(5,20px);grid-auto-flow:dense;align-items:stretch'><div id=a style='grid-column:span 3;height:20px'></div><div id=b style='grid-column:span 3;height:20px'></div><div id=c style='height:20px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(60, lot52_node("c")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("c")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_dense_span_173(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:100px;grid-template-columns:repeat(5,20px);grid-auto-flow:dense;align-items:stretch'><div id=a style='grid-column:span 3;height:22px'></div><div id=b style='grid-column:span 3;height:22px'></div><div id=c style='height:22px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(60, lot52_node("c")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("c")->y);
    TEST_ASSERT_EQUAL(22, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_dense_span_174(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:100px;grid-template-columns:repeat(5,20px);grid-auto-flow:dense;align-items:stretch'><div id=a style='grid-column:span 3;height:24px'></div><div id=b style='grid-column:span 3;height:24px'></div><div id=c style='height:24px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(60, lot52_node("c")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("c")->y);
    TEST_ASSERT_EQUAL(24, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_dense_packrow_175(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:80px;grid-template-columns:repeat(4,20px);grid-auto-flow:dense;align-items:stretch'><div id=a style='grid-column:span 2;height:8px'></div><div id=b style='grid-column:span 2;height:8px'></div><div id=c style='height:8px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_dense_packrow_176(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:80px;grid-template-columns:repeat(4,20px);grid-auto-flow:dense;align-items:stretch'><div id=a style='grid-column:span 2;height:10px'></div><div id=b style='grid-column:span 2;height:10px'></div><div id=c style='height:10px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_dense_packrow_177(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:80px;grid-template-columns:repeat(4,20px);grid-auto-flow:dense;align-items:stretch'><div id=a style='grid-column:span 2;height:12px'></div><div id=b style='grid-column:span 2;height:12px'></div><div id=c style='height:12px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_dense_packrow_178(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:100px;grid-template-columns:repeat(5,20px);grid-auto-flow:dense;align-items:stretch'><div id=a style='grid-column:span 2;height:8px'></div><div id=b style='grid-column:span 2;height:8px'></div><div id=c style='height:8px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_dense_packrow_179(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:100px;grid-template-columns:repeat(5,20px);grid-auto-flow:dense;align-items:stretch'><div id=a style='grid-column:span 2;height:10px'></div><div id=b style='grid-column:span 2;height:10px'></div><div id=c style='height:10px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_dense_packrow_180(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:100px;grid-template-columns:repeat(5,20px);grid-auto-flow:dense;align-items:stretch'><div id=a style='grid-column:span 2;height:12px'></div><div id=b style='grid-column:span 2;height:12px'></div><div id=c style='height:12px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_dense_packrow_181(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:120px;grid-template-columns:repeat(6,20px);grid-auto-flow:dense;align-items:stretch'><div id=a style='grid-column:span 2;height:8px'></div><div id=b style='grid-column:span 2;height:8px'></div><div id=c style='height:8px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_dense_packrow_182(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:120px;grid-template-columns:repeat(6,20px);grid-auto-flow:dense;align-items:stretch'><div id=a style='grid-column:span 2;height:10px'></div><div id=b style='grid-column:span 2;height:10px'></div><div id=c style='height:10px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_dense_packrow_183(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:120px;grid-template-columns:repeat(6,20px);grid-auto-flow:dense;align-items:stretch'><div id=a style='grid-column:span 2;height:12px'></div><div id=b style='grid-column:span 2;height:12px'></div><div id=c style='height:12px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void lot79_dense_run(void) {
    RUN_TEST(test_l79_contract_doc_size);
    RUN_TEST(test_l79_dense_row_1);
    RUN_TEST(test_l79_dense_row_2);
    RUN_TEST(test_l79_dense_row_3);
    RUN_TEST(test_l79_dense_row_4);
    RUN_TEST(test_l79_dense_row_5);
    RUN_TEST(test_l79_dense_row_6);
    RUN_TEST(test_l79_dense_row_7);
    RUN_TEST(test_l79_dense_row_8);
    RUN_TEST(test_l79_dense_row_9);
    RUN_TEST(test_l79_dense_row_10);
    RUN_TEST(test_l79_dense_row_11);
    RUN_TEST(test_l79_dense_row_12);
    RUN_TEST(test_l79_dense_row_13);
    RUN_TEST(test_l79_dense_row_14);
    RUN_TEST(test_l79_dense_row_15);
    RUN_TEST(test_l79_dense_row_16);
    RUN_TEST(test_l79_dense_row_17);
    RUN_TEST(test_l79_dense_row_18);
    RUN_TEST(test_l79_dense_row_19);
    RUN_TEST(test_l79_dense_row_20);
    RUN_TEST(test_l79_dense_row_21);
    RUN_TEST(test_l79_dense_row_22);
    RUN_TEST(test_l79_dense_row_23);
    RUN_TEST(test_l79_dense_row_24);
    RUN_TEST(test_l79_dense_row_25);
    RUN_TEST(test_l79_dense_row_26);
    RUN_TEST(test_l79_dense_row_27);
    RUN_TEST(test_l79_dense_row_28);
    RUN_TEST(test_l79_dense_row_29);
    RUN_TEST(test_l79_dense_row_30);
    RUN_TEST(test_l79_dense_row_31);
    RUN_TEST(test_l79_dense_row_32);
    RUN_TEST(test_l79_dense_row_33);
    RUN_TEST(test_l79_dense_row_34);
    RUN_TEST(test_l79_dense_row_35);
    RUN_TEST(test_l79_dense_row_36);
    RUN_TEST(test_l79_dense_row_37);
    RUN_TEST(test_l79_dense_row_38);
    RUN_TEST(test_l79_dense_row_39);
    RUN_TEST(test_l79_dense_row_40);
    RUN_TEST(test_l79_dense_row_41);
    RUN_TEST(test_l79_dense_row_42);
    RUN_TEST(test_l79_dense_row_43);
    RUN_TEST(test_l79_dense_row_44);
    RUN_TEST(test_l79_dense_row_45);
    RUN_TEST(test_l79_dense_row_46);
    RUN_TEST(test_l79_dense_row_47);
    RUN_TEST(test_l79_dense_row_48);
    RUN_TEST(test_l79_dense_row_49);
    RUN_TEST(test_l79_dense_row_50);
    RUN_TEST(test_l79_dense_row_51);
    RUN_TEST(test_l79_dense_row_52);
    RUN_TEST(test_l79_dense_row_53);
    RUN_TEST(test_l79_dense_row_54);
    RUN_TEST(test_l79_dense_row_55);
    RUN_TEST(test_l79_dense_row_56);
    RUN_TEST(test_l79_dense_row_57);
    RUN_TEST(test_l79_dense_row_58);
    RUN_TEST(test_l79_dense_row_59);
    RUN_TEST(test_l79_dense_row_60);
    RUN_TEST(test_l79_dense_row_61);
    RUN_TEST(test_l79_dense_row_62);
    RUN_TEST(test_l79_dense_row_63);
    RUN_TEST(test_l79_dense_row_64);
    RUN_TEST(test_l79_dense_row_65);
    RUN_TEST(test_l79_dense_row_66);
    RUN_TEST(test_l79_dense_row_67);
    RUN_TEST(test_l79_dense_row_68);
    RUN_TEST(test_l79_dense_row_69);
    RUN_TEST(test_l79_dense_row_70);
    RUN_TEST(test_l79_dense_row_71);
    RUN_TEST(test_l79_dense_row_72);
    RUN_TEST(test_l79_dense_row_73);
    RUN_TEST(test_l79_dense_row_74);
    RUN_TEST(test_l79_dense_row_75);
    RUN_TEST(test_l79_dense_row_76);
    RUN_TEST(test_l79_dense_row_77);
    RUN_TEST(test_l79_dense_row_78);
    RUN_TEST(test_l79_dense_row_79);
    RUN_TEST(test_l79_dense_row_80);
    RUN_TEST(test_l79_dense_row_81);
    RUN_TEST(test_l79_dense_row_82);
    RUN_TEST(test_l79_dense_row_83);
    RUN_TEST(test_l79_dense_row_84);
    RUN_TEST(test_l79_sparse_row_85);
    RUN_TEST(test_l79_sparse_row_86);
    RUN_TEST(test_l79_sparse_row_87);
    RUN_TEST(test_l79_sparse_row_88);
    RUN_TEST(test_l79_sparse_row_89);
    RUN_TEST(test_l79_sparse_row_90);
    RUN_TEST(test_l79_sparse_row_91);
    RUN_TEST(test_l79_sparse_row_92);
    RUN_TEST(test_l79_sparse_row_93);
    RUN_TEST(test_l79_sparse_row_94);
    RUN_TEST(test_l79_sparse_row_95);
    RUN_TEST(test_l79_sparse_row_96);
    RUN_TEST(test_l79_sparse_row_97);
    RUN_TEST(test_l79_sparse_row_98);
    RUN_TEST(test_l79_sparse_row_99);
    RUN_TEST(test_l79_sparse_row_100);
    RUN_TEST(test_l79_dense_col_101);
    RUN_TEST(test_l79_dense_col_102);
    RUN_TEST(test_l79_dense_col_103);
    RUN_TEST(test_l79_dense_col_104);
    RUN_TEST(test_l79_dense_col_105);
    RUN_TEST(test_l79_dense_col_106);
    RUN_TEST(test_l79_dense_col_107);
    RUN_TEST(test_l79_dense_col_108);
    RUN_TEST(test_l79_dense_col_109);
    RUN_TEST(test_l79_dense_col_110);
    RUN_TEST(test_l79_dense_col_111);
    RUN_TEST(test_l79_dense_col_112);
    RUN_TEST(test_l79_dense_col_113);
    RUN_TEST(test_l79_dense_col_114);
    RUN_TEST(test_l79_flow_refuse_1);
    RUN_TEST(test_l79_flow_refuse_2);
    RUN_TEST(test_l79_flow_refuse_3);
    RUN_TEST(test_l79_flow_refuse_4);
    RUN_TEST(test_l79_flow_refuse_5);
    RUN_TEST(test_l79_flow_refuse_6);
    RUN_TEST(test_l79_flow_refuse_7);
    RUN_TEST(test_l79_flow_refuse_8);
    RUN_TEST(test_l79_dense_gap_115);
    RUN_TEST(test_l79_dense_gap_116);
    RUN_TEST(test_l79_dense_gap_117);
    RUN_TEST(test_l79_dense_gap_118);
    RUN_TEST(test_l79_dense_gap_119);
    RUN_TEST(test_l79_dense_gap_120);
    RUN_TEST(test_l79_dense_gap_121);
    RUN_TEST(test_l79_dense_gap_122);
    RUN_TEST(test_l79_dense_gap_123);
    RUN_TEST(test_l79_dense_gap_124);
    RUN_TEST(test_l79_dense_gap_125);
    RUN_TEST(test_l79_dense_gap_126);
    RUN_TEST(test_l79_dense_gap_127);
    RUN_TEST(test_l79_dense_gap_128);
    RUN_TEST(test_l79_dense_gap_129);
    RUN_TEST(test_l79_dense_gap_130);
    RUN_TEST(test_l79_dense_gap_131);
    RUN_TEST(test_l79_dense_gap_132);
    RUN_TEST(test_l79_dense_gap_133);
    RUN_TEST(test_l79_dense_gap_134);
    RUN_TEST(test_l79_dense_gap_135);
    RUN_TEST(test_l79_dense_gap_136);
    RUN_TEST(test_l79_dense_gap_137);
    RUN_TEST(test_l79_dense_gap_138);
    RUN_TEST(test_l79_dense_gap_139);
    RUN_TEST(test_l79_dense_gap_140);
    RUN_TEST(test_l79_dense_gap_141);
    RUN_TEST(test_l79_dense_gap_142);
    RUN_TEST(test_l79_dense_gap_143);
    RUN_TEST(test_l79_dense_gap_144);
    RUN_TEST(test_l79_dense_span_145);
    RUN_TEST(test_l79_dense_span_146);
    RUN_TEST(test_l79_dense_span_147);
    RUN_TEST(test_l79_dense_span_148);
    RUN_TEST(test_l79_dense_span_149);
    RUN_TEST(test_l79_dense_span_150);
    RUN_TEST(test_l79_dense_span_151);
    RUN_TEST(test_l79_dense_span_152);
    RUN_TEST(test_l79_dense_span_153);
    RUN_TEST(test_l79_dense_span_154);
    RUN_TEST(test_l79_dense_span_155);
    RUN_TEST(test_l79_dense_span_156);
    RUN_TEST(test_l79_dense_span_157);
    RUN_TEST(test_l79_dense_span_158);
    RUN_TEST(test_l79_dense_span_159);
    RUN_TEST(test_l79_dense_span_160);
    RUN_TEST(test_l79_dense_span_161);
    RUN_TEST(test_l79_dense_span_162);
    RUN_TEST(test_l79_dense_span_163);
    RUN_TEST(test_l79_dense_span_164);
    RUN_TEST(test_l79_dense_span_165);
    RUN_TEST(test_l79_dense_span_166);
    RUN_TEST(test_l79_dense_span_167);
    RUN_TEST(test_l79_dense_span_168);
    RUN_TEST(test_l79_dense_span_169);
    RUN_TEST(test_l79_dense_span_170);
    RUN_TEST(test_l79_dense_span_171);
    RUN_TEST(test_l79_dense_span_172);
    RUN_TEST(test_l79_dense_span_173);
    RUN_TEST(test_l79_dense_span_174);
    RUN_TEST(test_l79_dense_packrow_175);
    RUN_TEST(test_l79_dense_packrow_176);
    RUN_TEST(test_l79_dense_packrow_177);
    RUN_TEST(test_l79_dense_packrow_178);
    RUN_TEST(test_l79_dense_packrow_179);
    RUN_TEST(test_l79_dense_packrow_180);
    RUN_TEST(test_l79_dense_packrow_181);
    RUN_TEST(test_l79_dense_packrow_182);
    RUN_TEST(test_l79_dense_packrow_183);
}
