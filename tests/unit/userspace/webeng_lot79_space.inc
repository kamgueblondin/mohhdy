/* Lot 79: justify/align-content space-* in grid. */
static void test_l79_jc_1(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:30px;height:30px;grid-template-columns:15px 15px;grid-template-rows:20px;justify-content:space-between;align-items:stretch'><div id=c0></div><div id=c1></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("c0")->x);
    TEST_ASSERT_EQUAL(15, lot52_node("c1")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_jc_2(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:30px;height:30px;grid-template-columns:15px 15px;grid-template-rows:20px;justify-content:space-around;align-items:stretch'><div id=c0></div><div id=c1></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("c0")->x);
    TEST_ASSERT_EQUAL(15, lot52_node("c1")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_jc_3(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:30px;height:30px;grid-template-columns:15px 15px;grid-template-rows:20px;justify-content:space-evenly;align-items:stretch'><div id=c0></div><div id=c1></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("c0")->x);
    TEST_ASSERT_EQUAL(15, lot52_node("c1")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_jc_4(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:30px;height:30px;grid-template-columns:15px 15px;grid-template-rows:20px;justify-content:start;align-items:stretch'><div id=c0></div><div id=c1></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("c0")->x);
    TEST_ASSERT_EQUAL(15, lot52_node("c1")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_jc_5(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:30px;height:30px;grid-template-columns:15px 15px;grid-template-rows:20px;justify-content:end;align-items:stretch'><div id=c0></div><div id=c1></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("c0")->x);
    TEST_ASSERT_EQUAL(15, lot52_node("c1")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_jc_6(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:30px;height:30px;grid-template-columns:15px 15px;grid-template-rows:20px;justify-content:center;align-items:stretch'><div id=c0></div><div id=c1></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("c0")->x);
    TEST_ASSERT_EQUAL(15, lot52_node("c1")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_jc_7(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:50px;height:30px;grid-template-columns:15px 15px;grid-template-rows:20px;justify-content:space-between;align-items:stretch'><div id=c0></div><div id=c1></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("c0")->x);
    TEST_ASSERT_EQUAL(35, lot52_node("c1")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_jc_8(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:50px;height:30px;grid-template-columns:15px 15px;grid-template-rows:20px;justify-content:space-around;align-items:stretch'><div id=c0></div><div id=c1></div></div></body>", 480);
    TEST_ASSERT_EQUAL(5, lot52_node("c0")->x);
    TEST_ASSERT_EQUAL(30, lot52_node("c1")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_jc_9(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:50px;height:30px;grid-template-columns:15px 15px;grid-template-rows:20px;justify-content:space-evenly;align-items:stretch'><div id=c0></div><div id=c1></div></div></body>", 480);
    TEST_ASSERT_EQUAL(6, lot52_node("c0")->x);
    TEST_ASSERT_EQUAL(28, lot52_node("c1")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_jc_10(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:50px;height:30px;grid-template-columns:15px 15px;grid-template-rows:20px;justify-content:start;align-items:stretch'><div id=c0></div><div id=c1></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("c0")->x);
    TEST_ASSERT_EQUAL(15, lot52_node("c1")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_jc_11(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:50px;height:30px;grid-template-columns:15px 15px;grid-template-rows:20px;justify-content:end;align-items:stretch'><div id=c0></div><div id=c1></div></div></body>", 480);
    TEST_ASSERT_EQUAL(20, lot52_node("c0")->x);
    TEST_ASSERT_EQUAL(35, lot52_node("c1")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_jc_12(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:50px;height:30px;grid-template-columns:15px 15px;grid-template-rows:20px;justify-content:center;align-items:stretch'><div id=c0></div><div id=c1></div></div></body>", 480);
    TEST_ASSERT_EQUAL(10, lot52_node("c0")->x);
    TEST_ASSERT_EQUAL(25, lot52_node("c1")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_jc_13(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:70px;height:30px;grid-template-columns:15px 15px;grid-template-rows:20px;justify-content:space-between;align-items:stretch'><div id=c0></div><div id=c1></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("c0")->x);
    TEST_ASSERT_EQUAL(55, lot52_node("c1")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_jc_14(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:70px;height:30px;grid-template-columns:15px 15px;grid-template-rows:20px;justify-content:space-around;align-items:stretch'><div id=c0></div><div id=c1></div></div></body>", 480);
    TEST_ASSERT_EQUAL(10, lot52_node("c0")->x);
    TEST_ASSERT_EQUAL(45, lot52_node("c1")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_jc_15(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:70px;height:30px;grid-template-columns:15px 15px;grid-template-rows:20px;justify-content:space-evenly;align-items:stretch'><div id=c0></div><div id=c1></div></div></body>", 480);
    TEST_ASSERT_EQUAL(13, lot52_node("c0")->x);
    TEST_ASSERT_EQUAL(41, lot52_node("c1")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_jc_16(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:70px;height:30px;grid-template-columns:15px 15px;grid-template-rows:20px;justify-content:start;align-items:stretch'><div id=c0></div><div id=c1></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("c0")->x);
    TEST_ASSERT_EQUAL(15, lot52_node("c1")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_jc_17(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:70px;height:30px;grid-template-columns:15px 15px;grid-template-rows:20px;justify-content:end;align-items:stretch'><div id=c0></div><div id=c1></div></div></body>", 480);
    TEST_ASSERT_EQUAL(40, lot52_node("c0")->x);
    TEST_ASSERT_EQUAL(55, lot52_node("c1")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_jc_18(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:70px;height:30px;grid-template-columns:15px 15px;grid-template-rows:20px;justify-content:center;align-items:stretch'><div id=c0></div><div id=c1></div></div></body>", 480);
    TEST_ASSERT_EQUAL(20, lot52_node("c0")->x);
    TEST_ASSERT_EQUAL(35, lot52_node("c1")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_jc_19(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:90px;height:30px;grid-template-columns:15px 15px;grid-template-rows:20px;justify-content:space-between;align-items:stretch'><div id=c0></div><div id=c1></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("c0")->x);
    TEST_ASSERT_EQUAL(75, lot52_node("c1")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_jc_20(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:90px;height:30px;grid-template-columns:15px 15px;grid-template-rows:20px;justify-content:space-around;align-items:stretch'><div id=c0></div><div id=c1></div></div></body>", 480);
    TEST_ASSERT_EQUAL(15, lot52_node("c0")->x);
    TEST_ASSERT_EQUAL(60, lot52_node("c1")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_jc_21(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:90px;height:30px;grid-template-columns:15px 15px;grid-template-rows:20px;justify-content:space-evenly;align-items:stretch'><div id=c0></div><div id=c1></div></div></body>", 480);
    TEST_ASSERT_EQUAL(20, lot52_node("c0")->x);
    TEST_ASSERT_EQUAL(55, lot52_node("c1")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_jc_22(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:90px;height:30px;grid-template-columns:15px 15px;grid-template-rows:20px;justify-content:start;align-items:stretch'><div id=c0></div><div id=c1></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("c0")->x);
    TEST_ASSERT_EQUAL(15, lot52_node("c1")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_jc_23(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:90px;height:30px;grid-template-columns:15px 15px;grid-template-rows:20px;justify-content:end;align-items:stretch'><div id=c0></div><div id=c1></div></div></body>", 480);
    TEST_ASSERT_EQUAL(60, lot52_node("c0")->x);
    TEST_ASSERT_EQUAL(75, lot52_node("c1")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_jc_24(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:90px;height:30px;grid-template-columns:15px 15px;grid-template-rows:20px;justify-content:center;align-items:stretch'><div id=c0></div><div id=c1></div></div></body>", 480);
    TEST_ASSERT_EQUAL(30, lot52_node("c0")->x);
    TEST_ASSERT_EQUAL(45, lot52_node("c1")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_jc_25(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:45px;height:30px;grid-template-columns:15px 15px 15px;grid-template-rows:20px;justify-content:space-between;align-items:stretch'><div id=c0></div><div id=c1></div><div id=c2></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("c0")->x);
    TEST_ASSERT_EQUAL(15, lot52_node("c1")->x);
    TEST_ASSERT_EQUAL(30, lot52_node("c2")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_jc_26(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:45px;height:30px;grid-template-columns:15px 15px 15px;grid-template-rows:20px;justify-content:space-around;align-items:stretch'><div id=c0></div><div id=c1></div><div id=c2></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("c0")->x);
    TEST_ASSERT_EQUAL(15, lot52_node("c1")->x);
    TEST_ASSERT_EQUAL(30, lot52_node("c2")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_jc_27(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:45px;height:30px;grid-template-columns:15px 15px 15px;grid-template-rows:20px;justify-content:space-evenly;align-items:stretch'><div id=c0></div><div id=c1></div><div id=c2></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("c0")->x);
    TEST_ASSERT_EQUAL(15, lot52_node("c1")->x);
    TEST_ASSERT_EQUAL(30, lot52_node("c2")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_jc_28(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:45px;height:30px;grid-template-columns:15px 15px 15px;grid-template-rows:20px;justify-content:start;align-items:stretch'><div id=c0></div><div id=c1></div><div id=c2></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("c0")->x);
    TEST_ASSERT_EQUAL(15, lot52_node("c1")->x);
    TEST_ASSERT_EQUAL(30, lot52_node("c2")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_jc_29(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:45px;height:30px;grid-template-columns:15px 15px 15px;grid-template-rows:20px;justify-content:end;align-items:stretch'><div id=c0></div><div id=c1></div><div id=c2></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("c0")->x);
    TEST_ASSERT_EQUAL(15, lot52_node("c1")->x);
    TEST_ASSERT_EQUAL(30, lot52_node("c2")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_jc_30(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:45px;height:30px;grid-template-columns:15px 15px 15px;grid-template-rows:20px;justify-content:center;align-items:stretch'><div id=c0></div><div id=c1></div><div id=c2></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("c0")->x);
    TEST_ASSERT_EQUAL(15, lot52_node("c1")->x);
    TEST_ASSERT_EQUAL(30, lot52_node("c2")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_jc_31(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:65px;height:30px;grid-template-columns:15px 15px 15px;grid-template-rows:20px;justify-content:space-between;align-items:stretch'><div id=c0></div><div id=c1></div><div id=c2></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("c0")->x);
    TEST_ASSERT_EQUAL(25, lot52_node("c1")->x);
    TEST_ASSERT_EQUAL(50, lot52_node("c2")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_jc_32(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:65px;height:30px;grid-template-columns:15px 15px 15px;grid-template-rows:20px;justify-content:space-around;align-items:stretch'><div id=c0></div><div id=c1></div><div id=c2></div></div></body>", 480);
    TEST_ASSERT_EQUAL(3, lot52_node("c0")->x);
    TEST_ASSERT_EQUAL(25, lot52_node("c1")->x);
    TEST_ASSERT_EQUAL(46, lot52_node("c2")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_jc_33(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:65px;height:30px;grid-template-columns:15px 15px 15px;grid-template-rows:20px;justify-content:space-evenly;align-items:stretch'><div id=c0></div><div id=c1></div><div id=c2></div></div></body>", 480);
    TEST_ASSERT_EQUAL(5, lot52_node("c0")->x);
    TEST_ASSERT_EQUAL(25, lot52_node("c1")->x);
    TEST_ASSERT_EQUAL(45, lot52_node("c2")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_jc_34(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:65px;height:30px;grid-template-columns:15px 15px 15px;grid-template-rows:20px;justify-content:start;align-items:stretch'><div id=c0></div><div id=c1></div><div id=c2></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("c0")->x);
    TEST_ASSERT_EQUAL(15, lot52_node("c1")->x);
    TEST_ASSERT_EQUAL(30, lot52_node("c2")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_jc_35(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:65px;height:30px;grid-template-columns:15px 15px 15px;grid-template-rows:20px;justify-content:end;align-items:stretch'><div id=c0></div><div id=c1></div><div id=c2></div></div></body>", 480);
    TEST_ASSERT_EQUAL(20, lot52_node("c0")->x);
    TEST_ASSERT_EQUAL(35, lot52_node("c1")->x);
    TEST_ASSERT_EQUAL(50, lot52_node("c2")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_jc_36(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:65px;height:30px;grid-template-columns:15px 15px 15px;grid-template-rows:20px;justify-content:center;align-items:stretch'><div id=c0></div><div id=c1></div><div id=c2></div></div></body>", 480);
    TEST_ASSERT_EQUAL(10, lot52_node("c0")->x);
    TEST_ASSERT_EQUAL(25, lot52_node("c1")->x);
    TEST_ASSERT_EQUAL(40, lot52_node("c2")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_jc_37(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:85px;height:30px;grid-template-columns:15px 15px 15px;grid-template-rows:20px;justify-content:space-between;align-items:stretch'><div id=c0></div><div id=c1></div><div id=c2></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("c0")->x);
    TEST_ASSERT_EQUAL(35, lot52_node("c1")->x);
    TEST_ASSERT_EQUAL(70, lot52_node("c2")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_jc_38(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:85px;height:30px;grid-template-columns:15px 15px 15px;grid-template-rows:20px;justify-content:space-around;align-items:stretch'><div id=c0></div><div id=c1></div><div id=c2></div></div></body>", 480);
    TEST_ASSERT_EQUAL(6, lot52_node("c0")->x);
    TEST_ASSERT_EQUAL(35, lot52_node("c1")->x);
    TEST_ASSERT_EQUAL(63, lot52_node("c2")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_jc_39(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:85px;height:30px;grid-template-columns:15px 15px 15px;grid-template-rows:20px;justify-content:space-evenly;align-items:stretch'><div id=c0></div><div id=c1></div><div id=c2></div></div></body>", 480);
    TEST_ASSERT_EQUAL(10, lot52_node("c0")->x);
    TEST_ASSERT_EQUAL(35, lot52_node("c1")->x);
    TEST_ASSERT_EQUAL(60, lot52_node("c2")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_jc_40(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:85px;height:30px;grid-template-columns:15px 15px 15px;grid-template-rows:20px;justify-content:start;align-items:stretch'><div id=c0></div><div id=c1></div><div id=c2></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("c0")->x);
    TEST_ASSERT_EQUAL(15, lot52_node("c1")->x);
    TEST_ASSERT_EQUAL(30, lot52_node("c2")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_jc_41(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:85px;height:30px;grid-template-columns:15px 15px 15px;grid-template-rows:20px;justify-content:end;align-items:stretch'><div id=c0></div><div id=c1></div><div id=c2></div></div></body>", 480);
    TEST_ASSERT_EQUAL(40, lot52_node("c0")->x);
    TEST_ASSERT_EQUAL(55, lot52_node("c1")->x);
    TEST_ASSERT_EQUAL(70, lot52_node("c2")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_jc_42(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:85px;height:30px;grid-template-columns:15px 15px 15px;grid-template-rows:20px;justify-content:center;align-items:stretch'><div id=c0></div><div id=c1></div><div id=c2></div></div></body>", 480);
    TEST_ASSERT_EQUAL(20, lot52_node("c0")->x);
    TEST_ASSERT_EQUAL(35, lot52_node("c1")->x);
    TEST_ASSERT_EQUAL(50, lot52_node("c2")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_jc_43(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:105px;height:30px;grid-template-columns:15px 15px 15px;grid-template-rows:20px;justify-content:space-between;align-items:stretch'><div id=c0></div><div id=c1></div><div id=c2></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("c0")->x);
    TEST_ASSERT_EQUAL(45, lot52_node("c1")->x);
    TEST_ASSERT_EQUAL(90, lot52_node("c2")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_jc_44(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:105px;height:30px;grid-template-columns:15px 15px 15px;grid-template-rows:20px;justify-content:space-around;align-items:stretch'><div id=c0></div><div id=c1></div><div id=c2></div></div></body>", 480);
    TEST_ASSERT_EQUAL(10, lot52_node("c0")->x);
    TEST_ASSERT_EQUAL(45, lot52_node("c1")->x);
    TEST_ASSERT_EQUAL(80, lot52_node("c2")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_jc_45(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:105px;height:30px;grid-template-columns:15px 15px 15px;grid-template-rows:20px;justify-content:space-evenly;align-items:stretch'><div id=c0></div><div id=c1></div><div id=c2></div></div></body>", 480);
    TEST_ASSERT_EQUAL(15, lot52_node("c0")->x);
    TEST_ASSERT_EQUAL(45, lot52_node("c1")->x);
    TEST_ASSERT_EQUAL(75, lot52_node("c2")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_jc_46(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:105px;height:30px;grid-template-columns:15px 15px 15px;grid-template-rows:20px;justify-content:start;align-items:stretch'><div id=c0></div><div id=c1></div><div id=c2></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("c0")->x);
    TEST_ASSERT_EQUAL(15, lot52_node("c1")->x);
    TEST_ASSERT_EQUAL(30, lot52_node("c2")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_jc_47(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:105px;height:30px;grid-template-columns:15px 15px 15px;grid-template-rows:20px;justify-content:end;align-items:stretch'><div id=c0></div><div id=c1></div><div id=c2></div></div></body>", 480);
    TEST_ASSERT_EQUAL(60, lot52_node("c0")->x);
    TEST_ASSERT_EQUAL(75, lot52_node("c1")->x);
    TEST_ASSERT_EQUAL(90, lot52_node("c2")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_jc_48(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:105px;height:30px;grid-template-columns:15px 15px 15px;grid-template-rows:20px;justify-content:center;align-items:stretch'><div id=c0></div><div id=c1></div><div id=c2></div></div></body>", 480);
    TEST_ASSERT_EQUAL(30, lot52_node("c0")->x);
    TEST_ASSERT_EQUAL(45, lot52_node("c1")->x);
    TEST_ASSERT_EQUAL(60, lot52_node("c2")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_jc_49(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:60px;height:30px;grid-template-columns:15px 15px 15px 15px;grid-template-rows:20px;justify-content:space-between;align-items:stretch'><div id=c0></div><div id=c1></div><div id=c2></div><div id=c3></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("c0")->x);
    TEST_ASSERT_EQUAL(15, lot52_node("c1")->x);
    TEST_ASSERT_EQUAL(30, lot52_node("c2")->x);
    TEST_ASSERT_EQUAL(45, lot52_node("c3")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_jc_50(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:60px;height:30px;grid-template-columns:15px 15px 15px 15px;grid-template-rows:20px;justify-content:space-around;align-items:stretch'><div id=c0></div><div id=c1></div><div id=c2></div><div id=c3></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("c0")->x);
    TEST_ASSERT_EQUAL(15, lot52_node("c1")->x);
    TEST_ASSERT_EQUAL(30, lot52_node("c2")->x);
    TEST_ASSERT_EQUAL(45, lot52_node("c3")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_jc_51(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:60px;height:30px;grid-template-columns:15px 15px 15px 15px;grid-template-rows:20px;justify-content:space-evenly;align-items:stretch'><div id=c0></div><div id=c1></div><div id=c2></div><div id=c3></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("c0")->x);
    TEST_ASSERT_EQUAL(15, lot52_node("c1")->x);
    TEST_ASSERT_EQUAL(30, lot52_node("c2")->x);
    TEST_ASSERT_EQUAL(45, lot52_node("c3")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_jc_52(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:60px;height:30px;grid-template-columns:15px 15px 15px 15px;grid-template-rows:20px;justify-content:start;align-items:stretch'><div id=c0></div><div id=c1></div><div id=c2></div><div id=c3></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("c0")->x);
    TEST_ASSERT_EQUAL(15, lot52_node("c1")->x);
    TEST_ASSERT_EQUAL(30, lot52_node("c2")->x);
    TEST_ASSERT_EQUAL(45, lot52_node("c3")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_jc_53(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:60px;height:30px;grid-template-columns:15px 15px 15px 15px;grid-template-rows:20px;justify-content:end;align-items:stretch'><div id=c0></div><div id=c1></div><div id=c2></div><div id=c3></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("c0")->x);
    TEST_ASSERT_EQUAL(15, lot52_node("c1")->x);
    TEST_ASSERT_EQUAL(30, lot52_node("c2")->x);
    TEST_ASSERT_EQUAL(45, lot52_node("c3")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_jc_54(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:60px;height:30px;grid-template-columns:15px 15px 15px 15px;grid-template-rows:20px;justify-content:center;align-items:stretch'><div id=c0></div><div id=c1></div><div id=c2></div><div id=c3></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("c0")->x);
    TEST_ASSERT_EQUAL(15, lot52_node("c1")->x);
    TEST_ASSERT_EQUAL(30, lot52_node("c2")->x);
    TEST_ASSERT_EQUAL(45, lot52_node("c3")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_jc_55(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:80px;height:30px;grid-template-columns:15px 15px 15px 15px;grid-template-rows:20px;justify-content:space-between;align-items:stretch'><div id=c0></div><div id=c1></div><div id=c2></div><div id=c3></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("c0")->x);
    TEST_ASSERT_EQUAL(21, lot52_node("c1")->x);
    TEST_ASSERT_EQUAL(43, lot52_node("c2")->x);
    TEST_ASSERT_EQUAL(65, lot52_node("c3")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_jc_56(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:80px;height:30px;grid-template-columns:15px 15px 15px 15px;grid-template-rows:20px;justify-content:space-around;align-items:stretch'><div id=c0></div><div id=c1></div><div id=c2></div><div id=c3></div></div></body>", 480);
    TEST_ASSERT_EQUAL(2, lot52_node("c0")->x);
    TEST_ASSERT_EQUAL(22, lot52_node("c1")->x);
    TEST_ASSERT_EQUAL(42, lot52_node("c2")->x);
    TEST_ASSERT_EQUAL(62, lot52_node("c3")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_jc_57(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:80px;height:30px;grid-template-columns:15px 15px 15px 15px;grid-template-rows:20px;justify-content:space-evenly;align-items:stretch'><div id=c0></div><div id=c1></div><div id=c2></div><div id=c3></div></div></body>", 480);
    TEST_ASSERT_EQUAL(4, lot52_node("c0")->x);
    TEST_ASSERT_EQUAL(23, lot52_node("c1")->x);
    TEST_ASSERT_EQUAL(42, lot52_node("c2")->x);
    TEST_ASSERT_EQUAL(61, lot52_node("c3")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_jc_58(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:80px;height:30px;grid-template-columns:15px 15px 15px 15px;grid-template-rows:20px;justify-content:start;align-items:stretch'><div id=c0></div><div id=c1></div><div id=c2></div><div id=c3></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("c0")->x);
    TEST_ASSERT_EQUAL(15, lot52_node("c1")->x);
    TEST_ASSERT_EQUAL(30, lot52_node("c2")->x);
    TEST_ASSERT_EQUAL(45, lot52_node("c3")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_jc_59(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:80px;height:30px;grid-template-columns:15px 15px 15px 15px;grid-template-rows:20px;justify-content:end;align-items:stretch'><div id=c0></div><div id=c1></div><div id=c2></div><div id=c3></div></div></body>", 480);
    TEST_ASSERT_EQUAL(20, lot52_node("c0")->x);
    TEST_ASSERT_EQUAL(35, lot52_node("c1")->x);
    TEST_ASSERT_EQUAL(50, lot52_node("c2")->x);
    TEST_ASSERT_EQUAL(65, lot52_node("c3")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_jc_60(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:80px;height:30px;grid-template-columns:15px 15px 15px 15px;grid-template-rows:20px;justify-content:center;align-items:stretch'><div id=c0></div><div id=c1></div><div id=c2></div><div id=c3></div></div></body>", 480);
    TEST_ASSERT_EQUAL(10, lot52_node("c0")->x);
    TEST_ASSERT_EQUAL(25, lot52_node("c1")->x);
    TEST_ASSERT_EQUAL(40, lot52_node("c2")->x);
    TEST_ASSERT_EQUAL(55, lot52_node("c3")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_jc_61(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:100px;height:30px;grid-template-columns:15px 15px 15px 15px;grid-template-rows:20px;justify-content:space-between;align-items:stretch'><div id=c0></div><div id=c1></div><div id=c2></div><div id=c3></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("c0")->x);
    TEST_ASSERT_EQUAL(28, lot52_node("c1")->x);
    TEST_ASSERT_EQUAL(56, lot52_node("c2")->x);
    TEST_ASSERT_EQUAL(85, lot52_node("c3")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_jc_62(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:100px;height:30px;grid-template-columns:15px 15px 15px 15px;grid-template-rows:20px;justify-content:space-around;align-items:stretch'><div id=c0></div><div id=c1></div><div id=c2></div><div id=c3></div></div></body>", 480);
    TEST_ASSERT_EQUAL(5, lot52_node("c0")->x);
    TEST_ASSERT_EQUAL(30, lot52_node("c1")->x);
    TEST_ASSERT_EQUAL(55, lot52_node("c2")->x);
    TEST_ASSERT_EQUAL(80, lot52_node("c3")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_jc_63(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:100px;height:30px;grid-template-columns:15px 15px 15px 15px;grid-template-rows:20px;justify-content:space-evenly;align-items:stretch'><div id=c0></div><div id=c1></div><div id=c2></div><div id=c3></div></div></body>", 480);
    TEST_ASSERT_EQUAL(8, lot52_node("c0")->x);
    TEST_ASSERT_EQUAL(31, lot52_node("c1")->x);
    TEST_ASSERT_EQUAL(54, lot52_node("c2")->x);
    TEST_ASSERT_EQUAL(77, lot52_node("c3")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_jc_64(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:100px;height:30px;grid-template-columns:15px 15px 15px 15px;grid-template-rows:20px;justify-content:start;align-items:stretch'><div id=c0></div><div id=c1></div><div id=c2></div><div id=c3></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("c0")->x);
    TEST_ASSERT_EQUAL(15, lot52_node("c1")->x);
    TEST_ASSERT_EQUAL(30, lot52_node("c2")->x);
    TEST_ASSERT_EQUAL(45, lot52_node("c3")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_jc_65(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:100px;height:30px;grid-template-columns:15px 15px 15px 15px;grid-template-rows:20px;justify-content:end;align-items:stretch'><div id=c0></div><div id=c1></div><div id=c2></div><div id=c3></div></div></body>", 480);
    TEST_ASSERT_EQUAL(40, lot52_node("c0")->x);
    TEST_ASSERT_EQUAL(55, lot52_node("c1")->x);
    TEST_ASSERT_EQUAL(70, lot52_node("c2")->x);
    TEST_ASSERT_EQUAL(85, lot52_node("c3")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_jc_66(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:100px;height:30px;grid-template-columns:15px 15px 15px 15px;grid-template-rows:20px;justify-content:center;align-items:stretch'><div id=c0></div><div id=c1></div><div id=c2></div><div id=c3></div></div></body>", 480);
    TEST_ASSERT_EQUAL(20, lot52_node("c0")->x);
    TEST_ASSERT_EQUAL(35, lot52_node("c1")->x);
    TEST_ASSERT_EQUAL(50, lot52_node("c2")->x);
    TEST_ASSERT_EQUAL(65, lot52_node("c3")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_jc_67(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:120px;height:30px;grid-template-columns:15px 15px 15px 15px;grid-template-rows:20px;justify-content:space-between;align-items:stretch'><div id=c0></div><div id=c1></div><div id=c2></div><div id=c3></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("c0")->x);
    TEST_ASSERT_EQUAL(35, lot52_node("c1")->x);
    TEST_ASSERT_EQUAL(70, lot52_node("c2")->x);
    TEST_ASSERT_EQUAL(105, lot52_node("c3")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_jc_68(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:120px;height:30px;grid-template-columns:15px 15px 15px 15px;grid-template-rows:20px;justify-content:space-around;align-items:stretch'><div id=c0></div><div id=c1></div><div id=c2></div><div id=c3></div></div></body>", 480);
    TEST_ASSERT_EQUAL(7, lot52_node("c0")->x);
    TEST_ASSERT_EQUAL(37, lot52_node("c1")->x);
    TEST_ASSERT_EQUAL(67, lot52_node("c2")->x);
    TEST_ASSERT_EQUAL(97, lot52_node("c3")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_jc_69(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:120px;height:30px;grid-template-columns:15px 15px 15px 15px;grid-template-rows:20px;justify-content:space-evenly;align-items:stretch'><div id=c0></div><div id=c1></div><div id=c2></div><div id=c3></div></div></body>", 480);
    TEST_ASSERT_EQUAL(12, lot52_node("c0")->x);
    TEST_ASSERT_EQUAL(39, lot52_node("c1")->x);
    TEST_ASSERT_EQUAL(66, lot52_node("c2")->x);
    TEST_ASSERT_EQUAL(93, lot52_node("c3")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_jc_70(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:120px;height:30px;grid-template-columns:15px 15px 15px 15px;grid-template-rows:20px;justify-content:start;align-items:stretch'><div id=c0></div><div id=c1></div><div id=c2></div><div id=c3></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("c0")->x);
    TEST_ASSERT_EQUAL(15, lot52_node("c1")->x);
    TEST_ASSERT_EQUAL(30, lot52_node("c2")->x);
    TEST_ASSERT_EQUAL(45, lot52_node("c3")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_jc_71(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:120px;height:30px;grid-template-columns:15px 15px 15px 15px;grid-template-rows:20px;justify-content:end;align-items:stretch'><div id=c0></div><div id=c1></div><div id=c2></div><div id=c3></div></div></body>", 480);
    TEST_ASSERT_EQUAL(60, lot52_node("c0")->x);
    TEST_ASSERT_EQUAL(75, lot52_node("c1")->x);
    TEST_ASSERT_EQUAL(90, lot52_node("c2")->x);
    TEST_ASSERT_EQUAL(105, lot52_node("c3")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_jc_72(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:120px;height:30px;grid-template-columns:15px 15px 15px 15px;grid-template-rows:20px;justify-content:center;align-items:stretch'><div id=c0></div><div id=c1></div><div id=c2></div><div id=c3></div></div></body>", 480);
    TEST_ASSERT_EQUAL(30, lot52_node("c0")->x);
    TEST_ASSERT_EQUAL(45, lot52_node("c1")->x);
    TEST_ASSERT_EQUAL(60, lot52_node("c2")->x);
    TEST_ASSERT_EQUAL(75, lot52_node("c3")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_jc_73(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:30px;grid-template-columns:20px 20px;grid-template-rows:20px;justify-content:space-between;align-items:stretch'><div id=c0></div><div id=c1></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("c0")->x);
    TEST_ASSERT_EQUAL(20, lot52_node("c1")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_jc_74(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:30px;grid-template-columns:20px 20px;grid-template-rows:20px;justify-content:space-around;align-items:stretch'><div id=c0></div><div id=c1></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("c0")->x);
    TEST_ASSERT_EQUAL(20, lot52_node("c1")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_jc_75(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:30px;grid-template-columns:20px 20px;grid-template-rows:20px;justify-content:space-evenly;align-items:stretch'><div id=c0></div><div id=c1></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("c0")->x);
    TEST_ASSERT_EQUAL(20, lot52_node("c1")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_jc_76(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:30px;grid-template-columns:20px 20px;grid-template-rows:20px;justify-content:start;align-items:stretch'><div id=c0></div><div id=c1></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("c0")->x);
    TEST_ASSERT_EQUAL(20, lot52_node("c1")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_jc_77(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:30px;grid-template-columns:20px 20px;grid-template-rows:20px;justify-content:end;align-items:stretch'><div id=c0></div><div id=c1></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("c0")->x);
    TEST_ASSERT_EQUAL(20, lot52_node("c1")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_jc_78(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:30px;grid-template-columns:20px 20px;grid-template-rows:20px;justify-content:center;align-items:stretch'><div id=c0></div><div id=c1></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("c0")->x);
    TEST_ASSERT_EQUAL(20, lot52_node("c1")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_jc_79(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:60px;height:30px;grid-template-columns:20px 20px;grid-template-rows:20px;justify-content:space-between;align-items:stretch'><div id=c0></div><div id=c1></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("c0")->x);
    TEST_ASSERT_EQUAL(40, lot52_node("c1")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_jc_80(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:60px;height:30px;grid-template-columns:20px 20px;grid-template-rows:20px;justify-content:space-around;align-items:stretch'><div id=c0></div><div id=c1></div></div></body>", 480);
    TEST_ASSERT_EQUAL(5, lot52_node("c0")->x);
    TEST_ASSERT_EQUAL(35, lot52_node("c1")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_jc_81(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:60px;height:30px;grid-template-columns:20px 20px;grid-template-rows:20px;justify-content:space-evenly;align-items:stretch'><div id=c0></div><div id=c1></div></div></body>", 480);
    TEST_ASSERT_EQUAL(6, lot52_node("c0")->x);
    TEST_ASSERT_EQUAL(33, lot52_node("c1")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_jc_82(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:60px;height:30px;grid-template-columns:20px 20px;grid-template-rows:20px;justify-content:start;align-items:stretch'><div id=c0></div><div id=c1></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("c0")->x);
    TEST_ASSERT_EQUAL(20, lot52_node("c1")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_jc_83(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:60px;height:30px;grid-template-columns:20px 20px;grid-template-rows:20px;justify-content:end;align-items:stretch'><div id=c0></div><div id=c1></div></div></body>", 480);
    TEST_ASSERT_EQUAL(20, lot52_node("c0")->x);
    TEST_ASSERT_EQUAL(40, lot52_node("c1")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_jc_84(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:60px;height:30px;grid-template-columns:20px 20px;grid-template-rows:20px;justify-content:center;align-items:stretch'><div id=c0></div><div id=c1></div></div></body>", 480);
    TEST_ASSERT_EQUAL(10, lot52_node("c0")->x);
    TEST_ASSERT_EQUAL(30, lot52_node("c1")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_jc_85(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:80px;height:30px;grid-template-columns:20px 20px;grid-template-rows:20px;justify-content:space-between;align-items:stretch'><div id=c0></div><div id=c1></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("c0")->x);
    TEST_ASSERT_EQUAL(60, lot52_node("c1")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_jc_86(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:80px;height:30px;grid-template-columns:20px 20px;grid-template-rows:20px;justify-content:space-around;align-items:stretch'><div id=c0></div><div id=c1></div></div></body>", 480);
    TEST_ASSERT_EQUAL(10, lot52_node("c0")->x);
    TEST_ASSERT_EQUAL(50, lot52_node("c1")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_jc_87(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:80px;height:30px;grid-template-columns:20px 20px;grid-template-rows:20px;justify-content:space-evenly;align-items:stretch'><div id=c0></div><div id=c1></div></div></body>", 480);
    TEST_ASSERT_EQUAL(13, lot52_node("c0")->x);
    TEST_ASSERT_EQUAL(46, lot52_node("c1")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_jc_88(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:80px;height:30px;grid-template-columns:20px 20px;grid-template-rows:20px;justify-content:start;align-items:stretch'><div id=c0></div><div id=c1></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("c0")->x);
    TEST_ASSERT_EQUAL(20, lot52_node("c1")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_jc_89(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:80px;height:30px;grid-template-columns:20px 20px;grid-template-rows:20px;justify-content:end;align-items:stretch'><div id=c0></div><div id=c1></div></div></body>", 480);
    TEST_ASSERT_EQUAL(40, lot52_node("c0")->x);
    TEST_ASSERT_EQUAL(60, lot52_node("c1")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_jc_90(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:80px;height:30px;grid-template-columns:20px 20px;grid-template-rows:20px;justify-content:center;align-items:stretch'><div id=c0></div><div id=c1></div></div></body>", 480);
    TEST_ASSERT_EQUAL(20, lot52_node("c0")->x);
    TEST_ASSERT_EQUAL(40, lot52_node("c1")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_jc_91(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:100px;height:30px;grid-template-columns:20px 20px;grid-template-rows:20px;justify-content:space-between;align-items:stretch'><div id=c0></div><div id=c1></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("c0")->x);
    TEST_ASSERT_EQUAL(80, lot52_node("c1")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_jc_92(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:100px;height:30px;grid-template-columns:20px 20px;grid-template-rows:20px;justify-content:space-around;align-items:stretch'><div id=c0></div><div id=c1></div></div></body>", 480);
    TEST_ASSERT_EQUAL(15, lot52_node("c0")->x);
    TEST_ASSERT_EQUAL(65, lot52_node("c1")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_jc_93(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:100px;height:30px;grid-template-columns:20px 20px;grid-template-rows:20px;justify-content:space-evenly;align-items:stretch'><div id=c0></div><div id=c1></div></div></body>", 480);
    TEST_ASSERT_EQUAL(20, lot52_node("c0")->x);
    TEST_ASSERT_EQUAL(60, lot52_node("c1")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_jc_94(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:100px;height:30px;grid-template-columns:20px 20px;grid-template-rows:20px;justify-content:start;align-items:stretch'><div id=c0></div><div id=c1></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("c0")->x);
    TEST_ASSERT_EQUAL(20, lot52_node("c1")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_jc_95(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:100px;height:30px;grid-template-columns:20px 20px;grid-template-rows:20px;justify-content:end;align-items:stretch'><div id=c0></div><div id=c1></div></div></body>", 480);
    TEST_ASSERT_EQUAL(60, lot52_node("c0")->x);
    TEST_ASSERT_EQUAL(80, lot52_node("c1")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_jc_96(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:100px;height:30px;grid-template-columns:20px 20px;grid-template-rows:20px;justify-content:center;align-items:stretch'><div id=c0></div><div id=c1></div></div></body>", 480);
    TEST_ASSERT_EQUAL(30, lot52_node("c0")->x);
    TEST_ASSERT_EQUAL(50, lot52_node("c1")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_jc_97(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:60px;height:30px;grid-template-columns:20px 20px 20px;grid-template-rows:20px;justify-content:space-between;align-items:stretch'><div id=c0></div><div id=c1></div><div id=c2></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("c0")->x);
    TEST_ASSERT_EQUAL(20, lot52_node("c1")->x);
    TEST_ASSERT_EQUAL(40, lot52_node("c2")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_jc_98(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:60px;height:30px;grid-template-columns:20px 20px 20px;grid-template-rows:20px;justify-content:space-around;align-items:stretch'><div id=c0></div><div id=c1></div><div id=c2></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("c0")->x);
    TEST_ASSERT_EQUAL(20, lot52_node("c1")->x);
    TEST_ASSERT_EQUAL(40, lot52_node("c2")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_jc_99(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:60px;height:30px;grid-template-columns:20px 20px 20px;grid-template-rows:20px;justify-content:space-evenly;align-items:stretch'><div id=c0></div><div id=c1></div><div id=c2></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("c0")->x);
    TEST_ASSERT_EQUAL(20, lot52_node("c1")->x);
    TEST_ASSERT_EQUAL(40, lot52_node("c2")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_jc_100(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:60px;height:30px;grid-template-columns:20px 20px 20px;grid-template-rows:20px;justify-content:start;align-items:stretch'><div id=c0></div><div id=c1></div><div id=c2></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("c0")->x);
    TEST_ASSERT_EQUAL(20, lot52_node("c1")->x);
    TEST_ASSERT_EQUAL(40, lot52_node("c2")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_jc_101(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:60px;height:30px;grid-template-columns:20px 20px 20px;grid-template-rows:20px;justify-content:end;align-items:stretch'><div id=c0></div><div id=c1></div><div id=c2></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("c0")->x);
    TEST_ASSERT_EQUAL(20, lot52_node("c1")->x);
    TEST_ASSERT_EQUAL(40, lot52_node("c2")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_jc_102(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:60px;height:30px;grid-template-columns:20px 20px 20px;grid-template-rows:20px;justify-content:center;align-items:stretch'><div id=c0></div><div id=c1></div><div id=c2></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("c0")->x);
    TEST_ASSERT_EQUAL(20, lot52_node("c1")->x);
    TEST_ASSERT_EQUAL(40, lot52_node("c2")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_jc_103(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:80px;height:30px;grid-template-columns:20px 20px 20px;grid-template-rows:20px;justify-content:space-between;align-items:stretch'><div id=c0></div><div id=c1></div><div id=c2></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("c0")->x);
    TEST_ASSERT_EQUAL(30, lot52_node("c1")->x);
    TEST_ASSERT_EQUAL(60, lot52_node("c2")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_jc_104(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:80px;height:30px;grid-template-columns:20px 20px 20px;grid-template-rows:20px;justify-content:space-around;align-items:stretch'><div id=c0></div><div id=c1></div><div id=c2></div></div></body>", 480);
    TEST_ASSERT_EQUAL(3, lot52_node("c0")->x);
    TEST_ASSERT_EQUAL(30, lot52_node("c1")->x);
    TEST_ASSERT_EQUAL(56, lot52_node("c2")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_jc_105(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:80px;height:30px;grid-template-columns:20px 20px 20px;grid-template-rows:20px;justify-content:space-evenly;align-items:stretch'><div id=c0></div><div id=c1></div><div id=c2></div></div></body>", 480);
    TEST_ASSERT_EQUAL(5, lot52_node("c0")->x);
    TEST_ASSERT_EQUAL(30, lot52_node("c1")->x);
    TEST_ASSERT_EQUAL(55, lot52_node("c2")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_jc_106(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:80px;height:30px;grid-template-columns:20px 20px 20px;grid-template-rows:20px;justify-content:start;align-items:stretch'><div id=c0></div><div id=c1></div><div id=c2></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("c0")->x);
    TEST_ASSERT_EQUAL(20, lot52_node("c1")->x);
    TEST_ASSERT_EQUAL(40, lot52_node("c2")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_jc_107(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:80px;height:30px;grid-template-columns:20px 20px 20px;grid-template-rows:20px;justify-content:end;align-items:stretch'><div id=c0></div><div id=c1></div><div id=c2></div></div></body>", 480);
    TEST_ASSERT_EQUAL(20, lot52_node("c0")->x);
    TEST_ASSERT_EQUAL(40, lot52_node("c1")->x);
    TEST_ASSERT_EQUAL(60, lot52_node("c2")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_jc_108(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:80px;height:30px;grid-template-columns:20px 20px 20px;grid-template-rows:20px;justify-content:center;align-items:stretch'><div id=c0></div><div id=c1></div><div id=c2></div></div></body>", 480);
    TEST_ASSERT_EQUAL(10, lot52_node("c0")->x);
    TEST_ASSERT_EQUAL(30, lot52_node("c1")->x);
    TEST_ASSERT_EQUAL(50, lot52_node("c2")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_jc_109(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:100px;height:30px;grid-template-columns:20px 20px 20px;grid-template-rows:20px;justify-content:space-between;align-items:stretch'><div id=c0></div><div id=c1></div><div id=c2></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("c0")->x);
    TEST_ASSERT_EQUAL(40, lot52_node("c1")->x);
    TEST_ASSERT_EQUAL(80, lot52_node("c2")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_jc_110(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:100px;height:30px;grid-template-columns:20px 20px 20px;grid-template-rows:20px;justify-content:space-around;align-items:stretch'><div id=c0></div><div id=c1></div><div id=c2></div></div></body>", 480);
    TEST_ASSERT_EQUAL(6, lot52_node("c0")->x);
    TEST_ASSERT_EQUAL(40, lot52_node("c1")->x);
    TEST_ASSERT_EQUAL(73, lot52_node("c2")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_jc_111(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:100px;height:30px;grid-template-columns:20px 20px 20px;grid-template-rows:20px;justify-content:space-evenly;align-items:stretch'><div id=c0></div><div id=c1></div><div id=c2></div></div></body>", 480);
    TEST_ASSERT_EQUAL(10, lot52_node("c0")->x);
    TEST_ASSERT_EQUAL(40, lot52_node("c1")->x);
    TEST_ASSERT_EQUAL(70, lot52_node("c2")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_jc_112(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:100px;height:30px;grid-template-columns:20px 20px 20px;grid-template-rows:20px;justify-content:start;align-items:stretch'><div id=c0></div><div id=c1></div><div id=c2></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("c0")->x);
    TEST_ASSERT_EQUAL(20, lot52_node("c1")->x);
    TEST_ASSERT_EQUAL(40, lot52_node("c2")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_jc_113(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:100px;height:30px;grid-template-columns:20px 20px 20px;grid-template-rows:20px;justify-content:end;align-items:stretch'><div id=c0></div><div id=c1></div><div id=c2></div></div></body>", 480);
    TEST_ASSERT_EQUAL(40, lot52_node("c0")->x);
    TEST_ASSERT_EQUAL(60, lot52_node("c1")->x);
    TEST_ASSERT_EQUAL(80, lot52_node("c2")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_jc_114(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:100px;height:30px;grid-template-columns:20px 20px 20px;grid-template-rows:20px;justify-content:center;align-items:stretch'><div id=c0></div><div id=c1></div><div id=c2></div></div></body>", 480);
    TEST_ASSERT_EQUAL(20, lot52_node("c0")->x);
    TEST_ASSERT_EQUAL(40, lot52_node("c1")->x);
    TEST_ASSERT_EQUAL(60, lot52_node("c2")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_jc_115(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:120px;height:30px;grid-template-columns:20px 20px 20px;grid-template-rows:20px;justify-content:space-between;align-items:stretch'><div id=c0></div><div id=c1></div><div id=c2></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("c0")->x);
    TEST_ASSERT_EQUAL(50, lot52_node("c1")->x);
    TEST_ASSERT_EQUAL(100, lot52_node("c2")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_jc_116(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:120px;height:30px;grid-template-columns:20px 20px 20px;grid-template-rows:20px;justify-content:space-around;align-items:stretch'><div id=c0></div><div id=c1></div><div id=c2></div></div></body>", 480);
    TEST_ASSERT_EQUAL(10, lot52_node("c0")->x);
    TEST_ASSERT_EQUAL(50, lot52_node("c1")->x);
    TEST_ASSERT_EQUAL(90, lot52_node("c2")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_jc_117(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:120px;height:30px;grid-template-columns:20px 20px 20px;grid-template-rows:20px;justify-content:space-evenly;align-items:stretch'><div id=c0></div><div id=c1></div><div id=c2></div></div></body>", 480);
    TEST_ASSERT_EQUAL(15, lot52_node("c0")->x);
    TEST_ASSERT_EQUAL(50, lot52_node("c1")->x);
    TEST_ASSERT_EQUAL(85, lot52_node("c2")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_jc_118(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:120px;height:30px;grid-template-columns:20px 20px 20px;grid-template-rows:20px;justify-content:start;align-items:stretch'><div id=c0></div><div id=c1></div><div id=c2></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("c0")->x);
    TEST_ASSERT_EQUAL(20, lot52_node("c1")->x);
    TEST_ASSERT_EQUAL(40, lot52_node("c2")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_jc_119(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:120px;height:30px;grid-template-columns:20px 20px 20px;grid-template-rows:20px;justify-content:end;align-items:stretch'><div id=c0></div><div id=c1></div><div id=c2></div></div></body>", 480);
    TEST_ASSERT_EQUAL(60, lot52_node("c0")->x);
    TEST_ASSERT_EQUAL(80, lot52_node("c1")->x);
    TEST_ASSERT_EQUAL(100, lot52_node("c2")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_jc_120(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:120px;height:30px;grid-template-columns:20px 20px 20px;grid-template-rows:20px;justify-content:center;align-items:stretch'><div id=c0></div><div id=c1></div><div id=c2></div></div></body>", 480);
    TEST_ASSERT_EQUAL(30, lot52_node("c0")->x);
    TEST_ASSERT_EQUAL(50, lot52_node("c1")->x);
    TEST_ASSERT_EQUAL(70, lot52_node("c2")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_jc_121(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:80px;height:30px;grid-template-columns:20px 20px 20px 20px;grid-template-rows:20px;justify-content:space-between;align-items:stretch'><div id=c0></div><div id=c1></div><div id=c2></div><div id=c3></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("c0")->x);
    TEST_ASSERT_EQUAL(20, lot52_node("c1")->x);
    TEST_ASSERT_EQUAL(40, lot52_node("c2")->x);
    TEST_ASSERT_EQUAL(60, lot52_node("c3")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_jc_122(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:80px;height:30px;grid-template-columns:20px 20px 20px 20px;grid-template-rows:20px;justify-content:space-around;align-items:stretch'><div id=c0></div><div id=c1></div><div id=c2></div><div id=c3></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("c0")->x);
    TEST_ASSERT_EQUAL(20, lot52_node("c1")->x);
    TEST_ASSERT_EQUAL(40, lot52_node("c2")->x);
    TEST_ASSERT_EQUAL(60, lot52_node("c3")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_jc_123(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:80px;height:30px;grid-template-columns:20px 20px 20px 20px;grid-template-rows:20px;justify-content:space-evenly;align-items:stretch'><div id=c0></div><div id=c1></div><div id=c2></div><div id=c3></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("c0")->x);
    TEST_ASSERT_EQUAL(20, lot52_node("c1")->x);
    TEST_ASSERT_EQUAL(40, lot52_node("c2")->x);
    TEST_ASSERT_EQUAL(60, lot52_node("c3")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_jc_124(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:80px;height:30px;grid-template-columns:20px 20px 20px 20px;grid-template-rows:20px;justify-content:start;align-items:stretch'><div id=c0></div><div id=c1></div><div id=c2></div><div id=c3></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("c0")->x);
    TEST_ASSERT_EQUAL(20, lot52_node("c1")->x);
    TEST_ASSERT_EQUAL(40, lot52_node("c2")->x);
    TEST_ASSERT_EQUAL(60, lot52_node("c3")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_jc_125(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:80px;height:30px;grid-template-columns:20px 20px 20px 20px;grid-template-rows:20px;justify-content:end;align-items:stretch'><div id=c0></div><div id=c1></div><div id=c2></div><div id=c3></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("c0")->x);
    TEST_ASSERT_EQUAL(20, lot52_node("c1")->x);
    TEST_ASSERT_EQUAL(40, lot52_node("c2")->x);
    TEST_ASSERT_EQUAL(60, lot52_node("c3")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_jc_126(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:80px;height:30px;grid-template-columns:20px 20px 20px 20px;grid-template-rows:20px;justify-content:center;align-items:stretch'><div id=c0></div><div id=c1></div><div id=c2></div><div id=c3></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("c0")->x);
    TEST_ASSERT_EQUAL(20, lot52_node("c1")->x);
    TEST_ASSERT_EQUAL(40, lot52_node("c2")->x);
    TEST_ASSERT_EQUAL(60, lot52_node("c3")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_jc_127(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:100px;height:30px;grid-template-columns:20px 20px 20px 20px;grid-template-rows:20px;justify-content:space-between;align-items:stretch'><div id=c0></div><div id=c1></div><div id=c2></div><div id=c3></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("c0")->x);
    TEST_ASSERT_EQUAL(26, lot52_node("c1")->x);
    TEST_ASSERT_EQUAL(53, lot52_node("c2")->x);
    TEST_ASSERT_EQUAL(80, lot52_node("c3")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_jc_128(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:100px;height:30px;grid-template-columns:20px 20px 20px 20px;grid-template-rows:20px;justify-content:space-around;align-items:stretch'><div id=c0></div><div id=c1></div><div id=c2></div><div id=c3></div></div></body>", 480);
    TEST_ASSERT_EQUAL(2, lot52_node("c0")->x);
    TEST_ASSERT_EQUAL(27, lot52_node("c1")->x);
    TEST_ASSERT_EQUAL(52, lot52_node("c2")->x);
    TEST_ASSERT_EQUAL(77, lot52_node("c3")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_jc_129(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:100px;height:30px;grid-template-columns:20px 20px 20px 20px;grid-template-rows:20px;justify-content:space-evenly;align-items:stretch'><div id=c0></div><div id=c1></div><div id=c2></div><div id=c3></div></div></body>", 480);
    TEST_ASSERT_EQUAL(4, lot52_node("c0")->x);
    TEST_ASSERT_EQUAL(28, lot52_node("c1")->x);
    TEST_ASSERT_EQUAL(52, lot52_node("c2")->x);
    TEST_ASSERT_EQUAL(76, lot52_node("c3")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_jc_130(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:100px;height:30px;grid-template-columns:20px 20px 20px 20px;grid-template-rows:20px;justify-content:start;align-items:stretch'><div id=c0></div><div id=c1></div><div id=c2></div><div id=c3></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("c0")->x);
    TEST_ASSERT_EQUAL(20, lot52_node("c1")->x);
    TEST_ASSERT_EQUAL(40, lot52_node("c2")->x);
    TEST_ASSERT_EQUAL(60, lot52_node("c3")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_jc_131(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:100px;height:30px;grid-template-columns:20px 20px 20px 20px;grid-template-rows:20px;justify-content:end;align-items:stretch'><div id=c0></div><div id=c1></div><div id=c2></div><div id=c3></div></div></body>", 480);
    TEST_ASSERT_EQUAL(20, lot52_node("c0")->x);
    TEST_ASSERT_EQUAL(40, lot52_node("c1")->x);
    TEST_ASSERT_EQUAL(60, lot52_node("c2")->x);
    TEST_ASSERT_EQUAL(80, lot52_node("c3")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_jc_132(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:100px;height:30px;grid-template-columns:20px 20px 20px 20px;grid-template-rows:20px;justify-content:center;align-items:stretch'><div id=c0></div><div id=c1></div><div id=c2></div><div id=c3></div></div></body>", 480);
    TEST_ASSERT_EQUAL(10, lot52_node("c0")->x);
    TEST_ASSERT_EQUAL(30, lot52_node("c1")->x);
    TEST_ASSERT_EQUAL(50, lot52_node("c2")->x);
    TEST_ASSERT_EQUAL(70, lot52_node("c3")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_jc_133(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:120px;height:30px;grid-template-columns:20px 20px 20px 20px;grid-template-rows:20px;justify-content:space-between;align-items:stretch'><div id=c0></div><div id=c1></div><div id=c2></div><div id=c3></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("c0")->x);
    TEST_ASSERT_EQUAL(33, lot52_node("c1")->x);
    TEST_ASSERT_EQUAL(66, lot52_node("c2")->x);
    TEST_ASSERT_EQUAL(100, lot52_node("c3")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_jc_134(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:120px;height:30px;grid-template-columns:20px 20px 20px 20px;grid-template-rows:20px;justify-content:space-around;align-items:stretch'><div id=c0></div><div id=c1></div><div id=c2></div><div id=c3></div></div></body>", 480);
    TEST_ASSERT_EQUAL(5, lot52_node("c0")->x);
    TEST_ASSERT_EQUAL(35, lot52_node("c1")->x);
    TEST_ASSERT_EQUAL(65, lot52_node("c2")->x);
    TEST_ASSERT_EQUAL(95, lot52_node("c3")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_jc_135(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:120px;height:30px;grid-template-columns:20px 20px 20px 20px;grid-template-rows:20px;justify-content:space-evenly;align-items:stretch'><div id=c0></div><div id=c1></div><div id=c2></div><div id=c3></div></div></body>", 480);
    TEST_ASSERT_EQUAL(8, lot52_node("c0")->x);
    TEST_ASSERT_EQUAL(36, lot52_node("c1")->x);
    TEST_ASSERT_EQUAL(64, lot52_node("c2")->x);
    TEST_ASSERT_EQUAL(92, lot52_node("c3")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_jc_136(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:120px;height:30px;grid-template-columns:20px 20px 20px 20px;grid-template-rows:20px;justify-content:start;align-items:stretch'><div id=c0></div><div id=c1></div><div id=c2></div><div id=c3></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("c0")->x);
    TEST_ASSERT_EQUAL(20, lot52_node("c1")->x);
    TEST_ASSERT_EQUAL(40, lot52_node("c2")->x);
    TEST_ASSERT_EQUAL(60, lot52_node("c3")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_jc_137(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:120px;height:30px;grid-template-columns:20px 20px 20px 20px;grid-template-rows:20px;justify-content:end;align-items:stretch'><div id=c0></div><div id=c1></div><div id=c2></div><div id=c3></div></div></body>", 480);
    TEST_ASSERT_EQUAL(40, lot52_node("c0")->x);
    TEST_ASSERT_EQUAL(60, lot52_node("c1")->x);
    TEST_ASSERT_EQUAL(80, lot52_node("c2")->x);
    TEST_ASSERT_EQUAL(100, lot52_node("c3")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_jc_138(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:120px;height:30px;grid-template-columns:20px 20px 20px 20px;grid-template-rows:20px;justify-content:center;align-items:stretch'><div id=c0></div><div id=c1></div><div id=c2></div><div id=c3></div></div></body>", 480);
    TEST_ASSERT_EQUAL(20, lot52_node("c0")->x);
    TEST_ASSERT_EQUAL(40, lot52_node("c1")->x);
    TEST_ASSERT_EQUAL(60, lot52_node("c2")->x);
    TEST_ASSERT_EQUAL(80, lot52_node("c3")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_jc_139(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:140px;height:30px;grid-template-columns:20px 20px 20px 20px;grid-template-rows:20px;justify-content:space-between;align-items:stretch'><div id=c0></div><div id=c1></div><div id=c2></div><div id=c3></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("c0")->x);
    TEST_ASSERT_EQUAL(40, lot52_node("c1")->x);
    TEST_ASSERT_EQUAL(80, lot52_node("c2")->x);
    TEST_ASSERT_EQUAL(120, lot52_node("c3")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_jc_140(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:140px;height:30px;grid-template-columns:20px 20px 20px 20px;grid-template-rows:20px;justify-content:space-around;align-items:stretch'><div id=c0></div><div id=c1></div><div id=c2></div><div id=c3></div></div></body>", 480);
    TEST_ASSERT_EQUAL(7, lot52_node("c0")->x);
    TEST_ASSERT_EQUAL(42, lot52_node("c1")->x);
    TEST_ASSERT_EQUAL(77, lot52_node("c2")->x);
    TEST_ASSERT_EQUAL(112, lot52_node("c3")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_jc_141(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:140px;height:30px;grid-template-columns:20px 20px 20px 20px;grid-template-rows:20px;justify-content:space-evenly;align-items:stretch'><div id=c0></div><div id=c1></div><div id=c2></div><div id=c3></div></div></body>", 480);
    TEST_ASSERT_EQUAL(12, lot52_node("c0")->x);
    TEST_ASSERT_EQUAL(44, lot52_node("c1")->x);
    TEST_ASSERT_EQUAL(76, lot52_node("c2")->x);
    TEST_ASSERT_EQUAL(108, lot52_node("c3")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_jc_142(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:140px;height:30px;grid-template-columns:20px 20px 20px 20px;grid-template-rows:20px;justify-content:start;align-items:stretch'><div id=c0></div><div id=c1></div><div id=c2></div><div id=c3></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("c0")->x);
    TEST_ASSERT_EQUAL(20, lot52_node("c1")->x);
    TEST_ASSERT_EQUAL(40, lot52_node("c2")->x);
    TEST_ASSERT_EQUAL(60, lot52_node("c3")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_jc_143(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:140px;height:30px;grid-template-columns:20px 20px 20px 20px;grid-template-rows:20px;justify-content:end;align-items:stretch'><div id=c0></div><div id=c1></div><div id=c2></div><div id=c3></div></div></body>", 480);
    TEST_ASSERT_EQUAL(60, lot52_node("c0")->x);
    TEST_ASSERT_EQUAL(80, lot52_node("c1")->x);
    TEST_ASSERT_EQUAL(100, lot52_node("c2")->x);
    TEST_ASSERT_EQUAL(120, lot52_node("c3")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_jc_144(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:140px;height:30px;grid-template-columns:20px 20px 20px 20px;grid-template-rows:20px;justify-content:center;align-items:stretch'><div id=c0></div><div id=c1></div><div id=c2></div><div id=c3></div></div></body>", 480);
    TEST_ASSERT_EQUAL(30, lot52_node("c0")->x);
    TEST_ASSERT_EQUAL(50, lot52_node("c1")->x);
    TEST_ASSERT_EQUAL(70, lot52_node("c2")->x);
    TEST_ASSERT_EQUAL(90, lot52_node("c3")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_jc_145(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:50px;height:30px;grid-template-columns:25px 25px;grid-template-rows:20px;justify-content:space-between;align-items:stretch'><div id=c0></div><div id=c1></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("c0")->x);
    TEST_ASSERT_EQUAL(25, lot52_node("c1")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_jc_146(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:50px;height:30px;grid-template-columns:25px 25px;grid-template-rows:20px;justify-content:space-around;align-items:stretch'><div id=c0></div><div id=c1></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("c0")->x);
    TEST_ASSERT_EQUAL(25, lot52_node("c1")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_jc_147(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:50px;height:30px;grid-template-columns:25px 25px;grid-template-rows:20px;justify-content:space-evenly;align-items:stretch'><div id=c0></div><div id=c1></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("c0")->x);
    TEST_ASSERT_EQUAL(25, lot52_node("c1")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_jc_148(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:50px;height:30px;grid-template-columns:25px 25px;grid-template-rows:20px;justify-content:start;align-items:stretch'><div id=c0></div><div id=c1></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("c0")->x);
    TEST_ASSERT_EQUAL(25, lot52_node("c1")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_jc_149(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:50px;height:30px;grid-template-columns:25px 25px;grid-template-rows:20px;justify-content:end;align-items:stretch'><div id=c0></div><div id=c1></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("c0")->x);
    TEST_ASSERT_EQUAL(25, lot52_node("c1")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_jc_150(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:50px;height:30px;grid-template-columns:25px 25px;grid-template-rows:20px;justify-content:center;align-items:stretch'><div id=c0></div><div id=c1></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("c0")->x);
    TEST_ASSERT_EQUAL(25, lot52_node("c1")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_jc_151(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:70px;height:30px;grid-template-columns:25px 25px;grid-template-rows:20px;justify-content:space-between;align-items:stretch'><div id=c0></div><div id=c1></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("c0")->x);
    TEST_ASSERT_EQUAL(45, lot52_node("c1")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_jc_152(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:70px;height:30px;grid-template-columns:25px 25px;grid-template-rows:20px;justify-content:space-around;align-items:stretch'><div id=c0></div><div id=c1></div></div></body>", 480);
    TEST_ASSERT_EQUAL(5, lot52_node("c0")->x);
    TEST_ASSERT_EQUAL(40, lot52_node("c1")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_jc_153(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:70px;height:30px;grid-template-columns:25px 25px;grid-template-rows:20px;justify-content:space-evenly;align-items:stretch'><div id=c0></div><div id=c1></div></div></body>", 480);
    TEST_ASSERT_EQUAL(6, lot52_node("c0")->x);
    TEST_ASSERT_EQUAL(38, lot52_node("c1")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_jc_154(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:70px;height:30px;grid-template-columns:25px 25px;grid-template-rows:20px;justify-content:start;align-items:stretch'><div id=c0></div><div id=c1></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("c0")->x);
    TEST_ASSERT_EQUAL(25, lot52_node("c1")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_jc_155(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:70px;height:30px;grid-template-columns:25px 25px;grid-template-rows:20px;justify-content:end;align-items:stretch'><div id=c0></div><div id=c1></div></div></body>", 480);
    TEST_ASSERT_EQUAL(20, lot52_node("c0")->x);
    TEST_ASSERT_EQUAL(45, lot52_node("c1")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_jc_156(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:70px;height:30px;grid-template-columns:25px 25px;grid-template-rows:20px;justify-content:center;align-items:stretch'><div id=c0></div><div id=c1></div></div></body>", 480);
    TEST_ASSERT_EQUAL(10, lot52_node("c0")->x);
    TEST_ASSERT_EQUAL(35, lot52_node("c1")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_jc_157(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:90px;height:30px;grid-template-columns:25px 25px;grid-template-rows:20px;justify-content:space-between;align-items:stretch'><div id=c0></div><div id=c1></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("c0")->x);
    TEST_ASSERT_EQUAL(65, lot52_node("c1")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_jc_158(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:90px;height:30px;grid-template-columns:25px 25px;grid-template-rows:20px;justify-content:space-around;align-items:stretch'><div id=c0></div><div id=c1></div></div></body>", 480);
    TEST_ASSERT_EQUAL(10, lot52_node("c0")->x);
    TEST_ASSERT_EQUAL(55, lot52_node("c1")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_jc_159(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:90px;height:30px;grid-template-columns:25px 25px;grid-template-rows:20px;justify-content:space-evenly;align-items:stretch'><div id=c0></div><div id=c1></div></div></body>", 480);
    TEST_ASSERT_EQUAL(13, lot52_node("c0")->x);
    TEST_ASSERT_EQUAL(51, lot52_node("c1")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_jc_160(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:90px;height:30px;grid-template-columns:25px 25px;grid-template-rows:20px;justify-content:start;align-items:stretch'><div id=c0></div><div id=c1></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("c0")->x);
    TEST_ASSERT_EQUAL(25, lot52_node("c1")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_jc_161(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:90px;height:30px;grid-template-columns:25px 25px;grid-template-rows:20px;justify-content:end;align-items:stretch'><div id=c0></div><div id=c1></div></div></body>", 480);
    TEST_ASSERT_EQUAL(40, lot52_node("c0")->x);
    TEST_ASSERT_EQUAL(65, lot52_node("c1")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_jc_162(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:90px;height:30px;grid-template-columns:25px 25px;grid-template-rows:20px;justify-content:center;align-items:stretch'><div id=c0></div><div id=c1></div></div></body>", 480);
    TEST_ASSERT_EQUAL(20, lot52_node("c0")->x);
    TEST_ASSERT_EQUAL(45, lot52_node("c1")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_jc_163(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:110px;height:30px;grid-template-columns:25px 25px;grid-template-rows:20px;justify-content:space-between;align-items:stretch'><div id=c0></div><div id=c1></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("c0")->x);
    TEST_ASSERT_EQUAL(85, lot52_node("c1")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_jc_164(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:110px;height:30px;grid-template-columns:25px 25px;grid-template-rows:20px;justify-content:space-around;align-items:stretch'><div id=c0></div><div id=c1></div></div></body>", 480);
    TEST_ASSERT_EQUAL(15, lot52_node("c0")->x);
    TEST_ASSERT_EQUAL(70, lot52_node("c1")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_jc_165(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:110px;height:30px;grid-template-columns:25px 25px;grid-template-rows:20px;justify-content:space-evenly;align-items:stretch'><div id=c0></div><div id=c1></div></div></body>", 480);
    TEST_ASSERT_EQUAL(20, lot52_node("c0")->x);
    TEST_ASSERT_EQUAL(65, lot52_node("c1")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_jc_166(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:110px;height:30px;grid-template-columns:25px 25px;grid-template-rows:20px;justify-content:start;align-items:stretch'><div id=c0></div><div id=c1></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("c0")->x);
    TEST_ASSERT_EQUAL(25, lot52_node("c1")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_jc_167(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:110px;height:30px;grid-template-columns:25px 25px;grid-template-rows:20px;justify-content:end;align-items:stretch'><div id=c0></div><div id=c1></div></div></body>", 480);
    TEST_ASSERT_EQUAL(60, lot52_node("c0")->x);
    TEST_ASSERT_EQUAL(85, lot52_node("c1")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_jc_168(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:110px;height:30px;grid-template-columns:25px 25px;grid-template-rows:20px;justify-content:center;align-items:stretch'><div id=c0></div><div id=c1></div></div></body>", 480);
    TEST_ASSERT_EQUAL(30, lot52_node("c0")->x);
    TEST_ASSERT_EQUAL(55, lot52_node("c1")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_jc_169(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:75px;height:30px;grid-template-columns:25px 25px 25px;grid-template-rows:20px;justify-content:space-between;align-items:stretch'><div id=c0></div><div id=c1></div><div id=c2></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("c0")->x);
    TEST_ASSERT_EQUAL(25, lot52_node("c1")->x);
    TEST_ASSERT_EQUAL(50, lot52_node("c2")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_jc_170(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:75px;height:30px;grid-template-columns:25px 25px 25px;grid-template-rows:20px;justify-content:space-around;align-items:stretch'><div id=c0></div><div id=c1></div><div id=c2></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("c0")->x);
    TEST_ASSERT_EQUAL(25, lot52_node("c1")->x);
    TEST_ASSERT_EQUAL(50, lot52_node("c2")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_jc_171(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:75px;height:30px;grid-template-columns:25px 25px 25px;grid-template-rows:20px;justify-content:space-evenly;align-items:stretch'><div id=c0></div><div id=c1></div><div id=c2></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("c0")->x);
    TEST_ASSERT_EQUAL(25, lot52_node("c1")->x);
    TEST_ASSERT_EQUAL(50, lot52_node("c2")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_jc_172(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:75px;height:30px;grid-template-columns:25px 25px 25px;grid-template-rows:20px;justify-content:start;align-items:stretch'><div id=c0></div><div id=c1></div><div id=c2></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("c0")->x);
    TEST_ASSERT_EQUAL(25, lot52_node("c1")->x);
    TEST_ASSERT_EQUAL(50, lot52_node("c2")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_jc_173(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:75px;height:30px;grid-template-columns:25px 25px 25px;grid-template-rows:20px;justify-content:end;align-items:stretch'><div id=c0></div><div id=c1></div><div id=c2></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("c0")->x);
    TEST_ASSERT_EQUAL(25, lot52_node("c1")->x);
    TEST_ASSERT_EQUAL(50, lot52_node("c2")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_jc_174(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:75px;height:30px;grid-template-columns:25px 25px 25px;grid-template-rows:20px;justify-content:center;align-items:stretch'><div id=c0></div><div id=c1></div><div id=c2></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("c0")->x);
    TEST_ASSERT_EQUAL(25, lot52_node("c1")->x);
    TEST_ASSERT_EQUAL(50, lot52_node("c2")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_jc_175(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:95px;height:30px;grid-template-columns:25px 25px 25px;grid-template-rows:20px;justify-content:space-between;align-items:stretch'><div id=c0></div><div id=c1></div><div id=c2></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("c0")->x);
    TEST_ASSERT_EQUAL(35, lot52_node("c1")->x);
    TEST_ASSERT_EQUAL(70, lot52_node("c2")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_jc_176(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:95px;height:30px;grid-template-columns:25px 25px 25px;grid-template-rows:20px;justify-content:space-around;align-items:stretch'><div id=c0></div><div id=c1></div><div id=c2></div></div></body>", 480);
    TEST_ASSERT_EQUAL(3, lot52_node("c0")->x);
    TEST_ASSERT_EQUAL(35, lot52_node("c1")->x);
    TEST_ASSERT_EQUAL(66, lot52_node("c2")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_jc_177(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:95px;height:30px;grid-template-columns:25px 25px 25px;grid-template-rows:20px;justify-content:space-evenly;align-items:stretch'><div id=c0></div><div id=c1></div><div id=c2></div></div></body>", 480);
    TEST_ASSERT_EQUAL(5, lot52_node("c0")->x);
    TEST_ASSERT_EQUAL(35, lot52_node("c1")->x);
    TEST_ASSERT_EQUAL(65, lot52_node("c2")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_jc_178(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:95px;height:30px;grid-template-columns:25px 25px 25px;grid-template-rows:20px;justify-content:start;align-items:stretch'><div id=c0></div><div id=c1></div><div id=c2></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("c0")->x);
    TEST_ASSERT_EQUAL(25, lot52_node("c1")->x);
    TEST_ASSERT_EQUAL(50, lot52_node("c2")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_jc_179(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:95px;height:30px;grid-template-columns:25px 25px 25px;grid-template-rows:20px;justify-content:end;align-items:stretch'><div id=c0></div><div id=c1></div><div id=c2></div></div></body>", 480);
    TEST_ASSERT_EQUAL(20, lot52_node("c0")->x);
    TEST_ASSERT_EQUAL(45, lot52_node("c1")->x);
    TEST_ASSERT_EQUAL(70, lot52_node("c2")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_jc_180(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:95px;height:30px;grid-template-columns:25px 25px 25px;grid-template-rows:20px;justify-content:center;align-items:stretch'><div id=c0></div><div id=c1></div><div id=c2></div></div></body>", 480);
    TEST_ASSERT_EQUAL(10, lot52_node("c0")->x);
    TEST_ASSERT_EQUAL(35, lot52_node("c1")->x);
    TEST_ASSERT_EQUAL(60, lot52_node("c2")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_jc_181(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:115px;height:30px;grid-template-columns:25px 25px 25px;grid-template-rows:20px;justify-content:space-between;align-items:stretch'><div id=c0></div><div id=c1></div><div id=c2></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("c0")->x);
    TEST_ASSERT_EQUAL(45, lot52_node("c1")->x);
    TEST_ASSERT_EQUAL(90, lot52_node("c2")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_jc_182(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:115px;height:30px;grid-template-columns:25px 25px 25px;grid-template-rows:20px;justify-content:space-around;align-items:stretch'><div id=c0></div><div id=c1></div><div id=c2></div></div></body>", 480);
    TEST_ASSERT_EQUAL(6, lot52_node("c0")->x);
    TEST_ASSERT_EQUAL(45, lot52_node("c1")->x);
    TEST_ASSERT_EQUAL(83, lot52_node("c2")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_jc_183(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:115px;height:30px;grid-template-columns:25px 25px 25px;grid-template-rows:20px;justify-content:space-evenly;align-items:stretch'><div id=c0></div><div id=c1></div><div id=c2></div></div></body>", 480);
    TEST_ASSERT_EQUAL(10, lot52_node("c0")->x);
    TEST_ASSERT_EQUAL(45, lot52_node("c1")->x);
    TEST_ASSERT_EQUAL(80, lot52_node("c2")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_jc_184(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:115px;height:30px;grid-template-columns:25px 25px 25px;grid-template-rows:20px;justify-content:start;align-items:stretch'><div id=c0></div><div id=c1></div><div id=c2></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("c0")->x);
    TEST_ASSERT_EQUAL(25, lot52_node("c1")->x);
    TEST_ASSERT_EQUAL(50, lot52_node("c2")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_jc_185(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:115px;height:30px;grid-template-columns:25px 25px 25px;grid-template-rows:20px;justify-content:end;align-items:stretch'><div id=c0></div><div id=c1></div><div id=c2></div></div></body>", 480);
    TEST_ASSERT_EQUAL(40, lot52_node("c0")->x);
    TEST_ASSERT_EQUAL(65, lot52_node("c1")->x);
    TEST_ASSERT_EQUAL(90, lot52_node("c2")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_jc_186(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:115px;height:30px;grid-template-columns:25px 25px 25px;grid-template-rows:20px;justify-content:center;align-items:stretch'><div id=c0></div><div id=c1></div><div id=c2></div></div></body>", 480);
    TEST_ASSERT_EQUAL(20, lot52_node("c0")->x);
    TEST_ASSERT_EQUAL(45, lot52_node("c1")->x);
    TEST_ASSERT_EQUAL(70, lot52_node("c2")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_jc_187(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:135px;height:30px;grid-template-columns:25px 25px 25px;grid-template-rows:20px;justify-content:space-between;align-items:stretch'><div id=c0></div><div id=c1></div><div id=c2></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("c0")->x);
    TEST_ASSERT_EQUAL(55, lot52_node("c1")->x);
    TEST_ASSERT_EQUAL(110, lot52_node("c2")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_jc_188(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:135px;height:30px;grid-template-columns:25px 25px 25px;grid-template-rows:20px;justify-content:space-around;align-items:stretch'><div id=c0></div><div id=c1></div><div id=c2></div></div></body>", 480);
    TEST_ASSERT_EQUAL(10, lot52_node("c0")->x);
    TEST_ASSERT_EQUAL(55, lot52_node("c1")->x);
    TEST_ASSERT_EQUAL(100, lot52_node("c2")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_jc_189(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:135px;height:30px;grid-template-columns:25px 25px 25px;grid-template-rows:20px;justify-content:space-evenly;align-items:stretch'><div id=c0></div><div id=c1></div><div id=c2></div></div></body>", 480);
    TEST_ASSERT_EQUAL(15, lot52_node("c0")->x);
    TEST_ASSERT_EQUAL(55, lot52_node("c1")->x);
    TEST_ASSERT_EQUAL(95, lot52_node("c2")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_jc_190(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:135px;height:30px;grid-template-columns:25px 25px 25px;grid-template-rows:20px;justify-content:start;align-items:stretch'><div id=c0></div><div id=c1></div><div id=c2></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("c0")->x);
    TEST_ASSERT_EQUAL(25, lot52_node("c1")->x);
    TEST_ASSERT_EQUAL(50, lot52_node("c2")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_jc_191(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:135px;height:30px;grid-template-columns:25px 25px 25px;grid-template-rows:20px;justify-content:end;align-items:stretch'><div id=c0></div><div id=c1></div><div id=c2></div></div></body>", 480);
    TEST_ASSERT_EQUAL(60, lot52_node("c0")->x);
    TEST_ASSERT_EQUAL(85, lot52_node("c1")->x);
    TEST_ASSERT_EQUAL(110, lot52_node("c2")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_jc_192(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:135px;height:30px;grid-template-columns:25px 25px 25px;grid-template-rows:20px;justify-content:center;align-items:stretch'><div id=c0></div><div id=c1></div><div id=c2></div></div></body>", 480);
    TEST_ASSERT_EQUAL(30, lot52_node("c0")->x);
    TEST_ASSERT_EQUAL(55, lot52_node("c1")->x);
    TEST_ASSERT_EQUAL(80, lot52_node("c2")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_jc_193(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:100px;height:30px;grid-template-columns:25px 25px 25px 25px;grid-template-rows:20px;justify-content:space-between;align-items:stretch'><div id=c0></div><div id=c1></div><div id=c2></div><div id=c3></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("c0")->x);
    TEST_ASSERT_EQUAL(25, lot52_node("c1")->x);
    TEST_ASSERT_EQUAL(50, lot52_node("c2")->x);
    TEST_ASSERT_EQUAL(75, lot52_node("c3")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_jc_194(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:100px;height:30px;grid-template-columns:25px 25px 25px 25px;grid-template-rows:20px;justify-content:space-around;align-items:stretch'><div id=c0></div><div id=c1></div><div id=c2></div><div id=c3></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("c0")->x);
    TEST_ASSERT_EQUAL(25, lot52_node("c1")->x);
    TEST_ASSERT_EQUAL(50, lot52_node("c2")->x);
    TEST_ASSERT_EQUAL(75, lot52_node("c3")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_jc_195(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:100px;height:30px;grid-template-columns:25px 25px 25px 25px;grid-template-rows:20px;justify-content:space-evenly;align-items:stretch'><div id=c0></div><div id=c1></div><div id=c2></div><div id=c3></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("c0")->x);
    TEST_ASSERT_EQUAL(25, lot52_node("c1")->x);
    TEST_ASSERT_EQUAL(50, lot52_node("c2")->x);
    TEST_ASSERT_EQUAL(75, lot52_node("c3")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_jc_196(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:100px;height:30px;grid-template-columns:25px 25px 25px 25px;grid-template-rows:20px;justify-content:start;align-items:stretch'><div id=c0></div><div id=c1></div><div id=c2></div><div id=c3></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("c0")->x);
    TEST_ASSERT_EQUAL(25, lot52_node("c1")->x);
    TEST_ASSERT_EQUAL(50, lot52_node("c2")->x);
    TEST_ASSERT_EQUAL(75, lot52_node("c3")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_jc_197(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:100px;height:30px;grid-template-columns:25px 25px 25px 25px;grid-template-rows:20px;justify-content:end;align-items:stretch'><div id=c0></div><div id=c1></div><div id=c2></div><div id=c3></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("c0")->x);
    TEST_ASSERT_EQUAL(25, lot52_node("c1")->x);
    TEST_ASSERT_EQUAL(50, lot52_node("c2")->x);
    TEST_ASSERT_EQUAL(75, lot52_node("c3")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_jc_198(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:100px;height:30px;grid-template-columns:25px 25px 25px 25px;grid-template-rows:20px;justify-content:center;align-items:stretch'><div id=c0></div><div id=c1></div><div id=c2></div><div id=c3></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("c0")->x);
    TEST_ASSERT_EQUAL(25, lot52_node("c1")->x);
    TEST_ASSERT_EQUAL(50, lot52_node("c2")->x);
    TEST_ASSERT_EQUAL(75, lot52_node("c3")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_jc_199(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:120px;height:30px;grid-template-columns:25px 25px 25px 25px;grid-template-rows:20px;justify-content:space-between;align-items:stretch'><div id=c0></div><div id=c1></div><div id=c2></div><div id=c3></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("c0")->x);
    TEST_ASSERT_EQUAL(31, lot52_node("c1")->x);
    TEST_ASSERT_EQUAL(63, lot52_node("c2")->x);
    TEST_ASSERT_EQUAL(95, lot52_node("c3")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_jc_200(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:120px;height:30px;grid-template-columns:25px 25px 25px 25px;grid-template-rows:20px;justify-content:space-around;align-items:stretch'><div id=c0></div><div id=c1></div><div id=c2></div><div id=c3></div></div></body>", 480);
    TEST_ASSERT_EQUAL(2, lot52_node("c0")->x);
    TEST_ASSERT_EQUAL(32, lot52_node("c1")->x);
    TEST_ASSERT_EQUAL(62, lot52_node("c2")->x);
    TEST_ASSERT_EQUAL(92, lot52_node("c3")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_jc_201(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:120px;height:30px;grid-template-columns:25px 25px 25px 25px;grid-template-rows:20px;justify-content:space-evenly;align-items:stretch'><div id=c0></div><div id=c1></div><div id=c2></div><div id=c3></div></div></body>", 480);
    TEST_ASSERT_EQUAL(4, lot52_node("c0")->x);
    TEST_ASSERT_EQUAL(33, lot52_node("c1")->x);
    TEST_ASSERT_EQUAL(62, lot52_node("c2")->x);
    TEST_ASSERT_EQUAL(91, lot52_node("c3")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_jc_202(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:120px;height:30px;grid-template-columns:25px 25px 25px 25px;grid-template-rows:20px;justify-content:start;align-items:stretch'><div id=c0></div><div id=c1></div><div id=c2></div><div id=c3></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("c0")->x);
    TEST_ASSERT_EQUAL(25, lot52_node("c1")->x);
    TEST_ASSERT_EQUAL(50, lot52_node("c2")->x);
    TEST_ASSERT_EQUAL(75, lot52_node("c3")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_jc_203(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:120px;height:30px;grid-template-columns:25px 25px 25px 25px;grid-template-rows:20px;justify-content:end;align-items:stretch'><div id=c0></div><div id=c1></div><div id=c2></div><div id=c3></div></div></body>", 480);
    TEST_ASSERT_EQUAL(20, lot52_node("c0")->x);
    TEST_ASSERT_EQUAL(45, lot52_node("c1")->x);
    TEST_ASSERT_EQUAL(70, lot52_node("c2")->x);
    TEST_ASSERT_EQUAL(95, lot52_node("c3")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_jc_204(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:120px;height:30px;grid-template-columns:25px 25px 25px 25px;grid-template-rows:20px;justify-content:center;align-items:stretch'><div id=c0></div><div id=c1></div><div id=c2></div><div id=c3></div></div></body>", 480);
    TEST_ASSERT_EQUAL(10, lot52_node("c0")->x);
    TEST_ASSERT_EQUAL(35, lot52_node("c1")->x);
    TEST_ASSERT_EQUAL(60, lot52_node("c2")->x);
    TEST_ASSERT_EQUAL(85, lot52_node("c3")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_jc_205(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:140px;height:30px;grid-template-columns:25px 25px 25px 25px;grid-template-rows:20px;justify-content:space-between;align-items:stretch'><div id=c0></div><div id=c1></div><div id=c2></div><div id=c3></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("c0")->x);
    TEST_ASSERT_EQUAL(38, lot52_node("c1")->x);
    TEST_ASSERT_EQUAL(76, lot52_node("c2")->x);
    TEST_ASSERT_EQUAL(115, lot52_node("c3")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_jc_206(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:140px;height:30px;grid-template-columns:25px 25px 25px 25px;grid-template-rows:20px;justify-content:space-around;align-items:stretch'><div id=c0></div><div id=c1></div><div id=c2></div><div id=c3></div></div></body>", 480);
    TEST_ASSERT_EQUAL(5, lot52_node("c0")->x);
    TEST_ASSERT_EQUAL(40, lot52_node("c1")->x);
    TEST_ASSERT_EQUAL(75, lot52_node("c2")->x);
    TEST_ASSERT_EQUAL(110, lot52_node("c3")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_jc_207(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:140px;height:30px;grid-template-columns:25px 25px 25px 25px;grid-template-rows:20px;justify-content:space-evenly;align-items:stretch'><div id=c0></div><div id=c1></div><div id=c2></div><div id=c3></div></div></body>", 480);
    TEST_ASSERT_EQUAL(8, lot52_node("c0")->x);
    TEST_ASSERT_EQUAL(41, lot52_node("c1")->x);
    TEST_ASSERT_EQUAL(74, lot52_node("c2")->x);
    TEST_ASSERT_EQUAL(107, lot52_node("c3")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_jc_208(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:140px;height:30px;grid-template-columns:25px 25px 25px 25px;grid-template-rows:20px;justify-content:start;align-items:stretch'><div id=c0></div><div id=c1></div><div id=c2></div><div id=c3></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("c0")->x);
    TEST_ASSERT_EQUAL(25, lot52_node("c1")->x);
    TEST_ASSERT_EQUAL(50, lot52_node("c2")->x);
    TEST_ASSERT_EQUAL(75, lot52_node("c3")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_jc_209(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:140px;height:30px;grid-template-columns:25px 25px 25px 25px;grid-template-rows:20px;justify-content:end;align-items:stretch'><div id=c0></div><div id=c1></div><div id=c2></div><div id=c3></div></div></body>", 480);
    TEST_ASSERT_EQUAL(40, lot52_node("c0")->x);
    TEST_ASSERT_EQUAL(65, lot52_node("c1")->x);
    TEST_ASSERT_EQUAL(90, lot52_node("c2")->x);
    TEST_ASSERT_EQUAL(115, lot52_node("c3")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_jc_210(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:140px;height:30px;grid-template-columns:25px 25px 25px 25px;grid-template-rows:20px;justify-content:center;align-items:stretch'><div id=c0></div><div id=c1></div><div id=c2></div><div id=c3></div></div></body>", 480);
    TEST_ASSERT_EQUAL(20, lot52_node("c0")->x);
    TEST_ASSERT_EQUAL(45, lot52_node("c1")->x);
    TEST_ASSERT_EQUAL(70, lot52_node("c2")->x);
    TEST_ASSERT_EQUAL(95, lot52_node("c3")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_jc_211(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:160px;height:30px;grid-template-columns:25px 25px 25px 25px;grid-template-rows:20px;justify-content:space-between;align-items:stretch'><div id=c0></div><div id=c1></div><div id=c2></div><div id=c3></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("c0")->x);
    TEST_ASSERT_EQUAL(45, lot52_node("c1")->x);
    TEST_ASSERT_EQUAL(90, lot52_node("c2")->x);
    TEST_ASSERT_EQUAL(135, lot52_node("c3")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_jc_212(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:160px;height:30px;grid-template-columns:25px 25px 25px 25px;grid-template-rows:20px;justify-content:space-around;align-items:stretch'><div id=c0></div><div id=c1></div><div id=c2></div><div id=c3></div></div></body>", 480);
    TEST_ASSERT_EQUAL(7, lot52_node("c0")->x);
    TEST_ASSERT_EQUAL(47, lot52_node("c1")->x);
    TEST_ASSERT_EQUAL(87, lot52_node("c2")->x);
    TEST_ASSERT_EQUAL(127, lot52_node("c3")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_jc_213(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:160px;height:30px;grid-template-columns:25px 25px 25px 25px;grid-template-rows:20px;justify-content:space-evenly;align-items:stretch'><div id=c0></div><div id=c1></div><div id=c2></div><div id=c3></div></div></body>", 480);
    TEST_ASSERT_EQUAL(12, lot52_node("c0")->x);
    TEST_ASSERT_EQUAL(49, lot52_node("c1")->x);
    TEST_ASSERT_EQUAL(86, lot52_node("c2")->x);
    TEST_ASSERT_EQUAL(123, lot52_node("c3")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_jc_214(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:160px;height:30px;grid-template-columns:25px 25px 25px 25px;grid-template-rows:20px;justify-content:start;align-items:stretch'><div id=c0></div><div id=c1></div><div id=c2></div><div id=c3></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("c0")->x);
    TEST_ASSERT_EQUAL(25, lot52_node("c1")->x);
    TEST_ASSERT_EQUAL(50, lot52_node("c2")->x);
    TEST_ASSERT_EQUAL(75, lot52_node("c3")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_jc_215(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:160px;height:30px;grid-template-columns:25px 25px 25px 25px;grid-template-rows:20px;justify-content:end;align-items:stretch'><div id=c0></div><div id=c1></div><div id=c2></div><div id=c3></div></div></body>", 480);
    TEST_ASSERT_EQUAL(60, lot52_node("c0")->x);
    TEST_ASSERT_EQUAL(85, lot52_node("c1")->x);
    TEST_ASSERT_EQUAL(110, lot52_node("c2")->x);
    TEST_ASSERT_EQUAL(135, lot52_node("c3")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_jc_216(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:160px;height:30px;grid-template-columns:25px 25px 25px 25px;grid-template-rows:20px;justify-content:center;align-items:stretch'><div id=c0></div><div id=c1></div><div id=c2></div><div id=c3></div></div></body>", 480);
    TEST_ASSERT_EQUAL(30, lot52_node("c0")->x);
    TEST_ASSERT_EQUAL(55, lot52_node("c1")->x);
    TEST_ASSERT_EQUAL(80, lot52_node("c2")->x);
    TEST_ASSERT_EQUAL(105, lot52_node("c3")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_jc_217(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:60px;height:30px;grid-template-columns:30px 30px;grid-template-rows:20px;justify-content:space-between;align-items:stretch'><div id=c0></div><div id=c1></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("c0")->x);
    TEST_ASSERT_EQUAL(30, lot52_node("c1")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_jc_218(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:60px;height:30px;grid-template-columns:30px 30px;grid-template-rows:20px;justify-content:space-around;align-items:stretch'><div id=c0></div><div id=c1></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("c0")->x);
    TEST_ASSERT_EQUAL(30, lot52_node("c1")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_jc_219(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:60px;height:30px;grid-template-columns:30px 30px;grid-template-rows:20px;justify-content:space-evenly;align-items:stretch'><div id=c0></div><div id=c1></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("c0")->x);
    TEST_ASSERT_EQUAL(30, lot52_node("c1")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_jc_220(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:60px;height:30px;grid-template-columns:30px 30px;grid-template-rows:20px;justify-content:start;align-items:stretch'><div id=c0></div><div id=c1></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("c0")->x);
    TEST_ASSERT_EQUAL(30, lot52_node("c1")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_jc_221(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:60px;height:30px;grid-template-columns:30px 30px;grid-template-rows:20px;justify-content:end;align-items:stretch'><div id=c0></div><div id=c1></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("c0")->x);
    TEST_ASSERT_EQUAL(30, lot52_node("c1")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_jc_222(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:60px;height:30px;grid-template-columns:30px 30px;grid-template-rows:20px;justify-content:center;align-items:stretch'><div id=c0></div><div id=c1></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("c0")->x);
    TEST_ASSERT_EQUAL(30, lot52_node("c1")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_jc_223(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:80px;height:30px;grid-template-columns:30px 30px;grid-template-rows:20px;justify-content:space-between;align-items:stretch'><div id=c0></div><div id=c1></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("c0")->x);
    TEST_ASSERT_EQUAL(50, lot52_node("c1")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_jc_224(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:80px;height:30px;grid-template-columns:30px 30px;grid-template-rows:20px;justify-content:space-around;align-items:stretch'><div id=c0></div><div id=c1></div></div></body>", 480);
    TEST_ASSERT_EQUAL(5, lot52_node("c0")->x);
    TEST_ASSERT_EQUAL(45, lot52_node("c1")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_jc_225(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:80px;height:30px;grid-template-columns:30px 30px;grid-template-rows:20px;justify-content:space-evenly;align-items:stretch'><div id=c0></div><div id=c1></div></div></body>", 480);
    TEST_ASSERT_EQUAL(6, lot52_node("c0")->x);
    TEST_ASSERT_EQUAL(43, lot52_node("c1")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_jc_226(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:80px;height:30px;grid-template-columns:30px 30px;grid-template-rows:20px;justify-content:start;align-items:stretch'><div id=c0></div><div id=c1></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("c0")->x);
    TEST_ASSERT_EQUAL(30, lot52_node("c1")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_jc_227(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:80px;height:30px;grid-template-columns:30px 30px;grid-template-rows:20px;justify-content:end;align-items:stretch'><div id=c0></div><div id=c1></div></div></body>", 480);
    TEST_ASSERT_EQUAL(20, lot52_node("c0")->x);
    TEST_ASSERT_EQUAL(50, lot52_node("c1")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_jc_228(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:80px;height:30px;grid-template-columns:30px 30px;grid-template-rows:20px;justify-content:center;align-items:stretch'><div id=c0></div><div id=c1></div></div></body>", 480);
    TEST_ASSERT_EQUAL(10, lot52_node("c0")->x);
    TEST_ASSERT_EQUAL(40, lot52_node("c1")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_jc_229(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:100px;height:30px;grid-template-columns:30px 30px;grid-template-rows:20px;justify-content:space-between;align-items:stretch'><div id=c0></div><div id=c1></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("c0")->x);
    TEST_ASSERT_EQUAL(70, lot52_node("c1")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_jc_230(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:100px;height:30px;grid-template-columns:30px 30px;grid-template-rows:20px;justify-content:space-around;align-items:stretch'><div id=c0></div><div id=c1></div></div></body>", 480);
    TEST_ASSERT_EQUAL(10, lot52_node("c0")->x);
    TEST_ASSERT_EQUAL(60, lot52_node("c1")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_jc_231(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:100px;height:30px;grid-template-columns:30px 30px;grid-template-rows:20px;justify-content:space-evenly;align-items:stretch'><div id=c0></div><div id=c1></div></div></body>", 480);
    TEST_ASSERT_EQUAL(13, lot52_node("c0")->x);
    TEST_ASSERT_EQUAL(56, lot52_node("c1")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_jc_232(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:100px;height:30px;grid-template-columns:30px 30px;grid-template-rows:20px;justify-content:start;align-items:stretch'><div id=c0></div><div id=c1></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("c0")->x);
    TEST_ASSERT_EQUAL(30, lot52_node("c1")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_jc_233(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:100px;height:30px;grid-template-columns:30px 30px;grid-template-rows:20px;justify-content:end;align-items:stretch'><div id=c0></div><div id=c1></div></div></body>", 480);
    TEST_ASSERT_EQUAL(40, lot52_node("c0")->x);
    TEST_ASSERT_EQUAL(70, lot52_node("c1")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_jc_234(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:100px;height:30px;grid-template-columns:30px 30px;grid-template-rows:20px;justify-content:center;align-items:stretch'><div id=c0></div><div id=c1></div></div></body>", 480);
    TEST_ASSERT_EQUAL(20, lot52_node("c0")->x);
    TEST_ASSERT_EQUAL(50, lot52_node("c1")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_jc_235(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:120px;height:30px;grid-template-columns:30px 30px;grid-template-rows:20px;justify-content:space-between;align-items:stretch'><div id=c0></div><div id=c1></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("c0")->x);
    TEST_ASSERT_EQUAL(90, lot52_node("c1")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_jc_236(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:120px;height:30px;grid-template-columns:30px 30px;grid-template-rows:20px;justify-content:space-around;align-items:stretch'><div id=c0></div><div id=c1></div></div></body>", 480);
    TEST_ASSERT_EQUAL(15, lot52_node("c0")->x);
    TEST_ASSERT_EQUAL(75, lot52_node("c1")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_jc_237(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:120px;height:30px;grid-template-columns:30px 30px;grid-template-rows:20px;justify-content:space-evenly;align-items:stretch'><div id=c0></div><div id=c1></div></div></body>", 480);
    TEST_ASSERT_EQUAL(20, lot52_node("c0")->x);
    TEST_ASSERT_EQUAL(70, lot52_node("c1")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_jc_238(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:120px;height:30px;grid-template-columns:30px 30px;grid-template-rows:20px;justify-content:start;align-items:stretch'><div id=c0></div><div id=c1></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("c0")->x);
    TEST_ASSERT_EQUAL(30, lot52_node("c1")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_jc_239(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:120px;height:30px;grid-template-columns:30px 30px;grid-template-rows:20px;justify-content:end;align-items:stretch'><div id=c0></div><div id=c1></div></div></body>", 480);
    TEST_ASSERT_EQUAL(60, lot52_node("c0")->x);
    TEST_ASSERT_EQUAL(90, lot52_node("c1")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_jc_240(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:120px;height:30px;grid-template-columns:30px 30px;grid-template-rows:20px;justify-content:center;align-items:stretch'><div id=c0></div><div id=c1></div></div></body>", 480);
    TEST_ASSERT_EQUAL(30, lot52_node("c0")->x);
    TEST_ASSERT_EQUAL(60, lot52_node("c1")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_jc_241(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:90px;height:30px;grid-template-columns:30px 30px 30px;grid-template-rows:20px;justify-content:space-between;align-items:stretch'><div id=c0></div><div id=c1></div><div id=c2></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("c0")->x);
    TEST_ASSERT_EQUAL(30, lot52_node("c1")->x);
    TEST_ASSERT_EQUAL(60, lot52_node("c2")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_jc_242(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:90px;height:30px;grid-template-columns:30px 30px 30px;grid-template-rows:20px;justify-content:space-around;align-items:stretch'><div id=c0></div><div id=c1></div><div id=c2></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("c0")->x);
    TEST_ASSERT_EQUAL(30, lot52_node("c1")->x);
    TEST_ASSERT_EQUAL(60, lot52_node("c2")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_jc_243(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:90px;height:30px;grid-template-columns:30px 30px 30px;grid-template-rows:20px;justify-content:space-evenly;align-items:stretch'><div id=c0></div><div id=c1></div><div id=c2></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("c0")->x);
    TEST_ASSERT_EQUAL(30, lot52_node("c1")->x);
    TEST_ASSERT_EQUAL(60, lot52_node("c2")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_jc_244(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:90px;height:30px;grid-template-columns:30px 30px 30px;grid-template-rows:20px;justify-content:start;align-items:stretch'><div id=c0></div><div id=c1></div><div id=c2></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("c0")->x);
    TEST_ASSERT_EQUAL(30, lot52_node("c1")->x);
    TEST_ASSERT_EQUAL(60, lot52_node("c2")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_jc_245(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:90px;height:30px;grid-template-columns:30px 30px 30px;grid-template-rows:20px;justify-content:end;align-items:stretch'><div id=c0></div><div id=c1></div><div id=c2></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("c0")->x);
    TEST_ASSERT_EQUAL(30, lot52_node("c1")->x);
    TEST_ASSERT_EQUAL(60, lot52_node("c2")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_jc_246(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:90px;height:30px;grid-template-columns:30px 30px 30px;grid-template-rows:20px;justify-content:center;align-items:stretch'><div id=c0></div><div id=c1></div><div id=c2></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("c0")->x);
    TEST_ASSERT_EQUAL(30, lot52_node("c1")->x);
    TEST_ASSERT_EQUAL(60, lot52_node("c2")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_jc_247(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:110px;height:30px;grid-template-columns:30px 30px 30px;grid-template-rows:20px;justify-content:space-between;align-items:stretch'><div id=c0></div><div id=c1></div><div id=c2></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("c0")->x);
    TEST_ASSERT_EQUAL(40, lot52_node("c1")->x);
    TEST_ASSERT_EQUAL(80, lot52_node("c2")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_jc_248(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:110px;height:30px;grid-template-columns:30px 30px 30px;grid-template-rows:20px;justify-content:space-around;align-items:stretch'><div id=c0></div><div id=c1></div><div id=c2></div></div></body>", 480);
    TEST_ASSERT_EQUAL(3, lot52_node("c0")->x);
    TEST_ASSERT_EQUAL(40, lot52_node("c1")->x);
    TEST_ASSERT_EQUAL(76, lot52_node("c2")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_jc_249(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:110px;height:30px;grid-template-columns:30px 30px 30px;grid-template-rows:20px;justify-content:space-evenly;align-items:stretch'><div id=c0></div><div id=c1></div><div id=c2></div></div></body>", 480);
    TEST_ASSERT_EQUAL(5, lot52_node("c0")->x);
    TEST_ASSERT_EQUAL(40, lot52_node("c1")->x);
    TEST_ASSERT_EQUAL(75, lot52_node("c2")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_jc_250(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:110px;height:30px;grid-template-columns:30px 30px 30px;grid-template-rows:20px;justify-content:start;align-items:stretch'><div id=c0></div><div id=c1></div><div id=c2></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("c0")->x);
    TEST_ASSERT_EQUAL(30, lot52_node("c1")->x);
    TEST_ASSERT_EQUAL(60, lot52_node("c2")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_jc_251(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:110px;height:30px;grid-template-columns:30px 30px 30px;grid-template-rows:20px;justify-content:end;align-items:stretch'><div id=c0></div><div id=c1></div><div id=c2></div></div></body>", 480);
    TEST_ASSERT_EQUAL(20, lot52_node("c0")->x);
    TEST_ASSERT_EQUAL(50, lot52_node("c1")->x);
    TEST_ASSERT_EQUAL(80, lot52_node("c2")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_jc_252(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:110px;height:30px;grid-template-columns:30px 30px 30px;grid-template-rows:20px;justify-content:center;align-items:stretch'><div id=c0></div><div id=c1></div><div id=c2></div></div></body>", 480);
    TEST_ASSERT_EQUAL(10, lot52_node("c0")->x);
    TEST_ASSERT_EQUAL(40, lot52_node("c1")->x);
    TEST_ASSERT_EQUAL(70, lot52_node("c2")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_jc_253(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:130px;height:30px;grid-template-columns:30px 30px 30px;grid-template-rows:20px;justify-content:space-between;align-items:stretch'><div id=c0></div><div id=c1></div><div id=c2></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("c0")->x);
    TEST_ASSERT_EQUAL(50, lot52_node("c1")->x);
    TEST_ASSERT_EQUAL(100, lot52_node("c2")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_jc_254(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:130px;height:30px;grid-template-columns:30px 30px 30px;grid-template-rows:20px;justify-content:space-around;align-items:stretch'><div id=c0></div><div id=c1></div><div id=c2></div></div></body>", 480);
    TEST_ASSERT_EQUAL(6, lot52_node("c0")->x);
    TEST_ASSERT_EQUAL(50, lot52_node("c1")->x);
    TEST_ASSERT_EQUAL(93, lot52_node("c2")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_jc_255(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:130px;height:30px;grid-template-columns:30px 30px 30px;grid-template-rows:20px;justify-content:space-evenly;align-items:stretch'><div id=c0></div><div id=c1></div><div id=c2></div></div></body>", 480);
    TEST_ASSERT_EQUAL(10, lot52_node("c0")->x);
    TEST_ASSERT_EQUAL(50, lot52_node("c1")->x);
    TEST_ASSERT_EQUAL(90, lot52_node("c2")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_jc_256(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:130px;height:30px;grid-template-columns:30px 30px 30px;grid-template-rows:20px;justify-content:start;align-items:stretch'><div id=c0></div><div id=c1></div><div id=c2></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("c0")->x);
    TEST_ASSERT_EQUAL(30, lot52_node("c1")->x);
    TEST_ASSERT_EQUAL(60, lot52_node("c2")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_jc_257(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:130px;height:30px;grid-template-columns:30px 30px 30px;grid-template-rows:20px;justify-content:end;align-items:stretch'><div id=c0></div><div id=c1></div><div id=c2></div></div></body>", 480);
    TEST_ASSERT_EQUAL(40, lot52_node("c0")->x);
    TEST_ASSERT_EQUAL(70, lot52_node("c1")->x);
    TEST_ASSERT_EQUAL(100, lot52_node("c2")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_jc_258(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:130px;height:30px;grid-template-columns:30px 30px 30px;grid-template-rows:20px;justify-content:center;align-items:stretch'><div id=c0></div><div id=c1></div><div id=c2></div></div></body>", 480);
    TEST_ASSERT_EQUAL(20, lot52_node("c0")->x);
    TEST_ASSERT_EQUAL(50, lot52_node("c1")->x);
    TEST_ASSERT_EQUAL(80, lot52_node("c2")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_jc_259(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:150px;height:30px;grid-template-columns:30px 30px 30px;grid-template-rows:20px;justify-content:space-between;align-items:stretch'><div id=c0></div><div id=c1></div><div id=c2></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("c0")->x);
    TEST_ASSERT_EQUAL(60, lot52_node("c1")->x);
    TEST_ASSERT_EQUAL(120, lot52_node("c2")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_jc_260(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:150px;height:30px;grid-template-columns:30px 30px 30px;grid-template-rows:20px;justify-content:space-around;align-items:stretch'><div id=c0></div><div id=c1></div><div id=c2></div></div></body>", 480);
    TEST_ASSERT_EQUAL(10, lot52_node("c0")->x);
    TEST_ASSERT_EQUAL(60, lot52_node("c1")->x);
    TEST_ASSERT_EQUAL(110, lot52_node("c2")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_jc_261(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:150px;height:30px;grid-template-columns:30px 30px 30px;grid-template-rows:20px;justify-content:space-evenly;align-items:stretch'><div id=c0></div><div id=c1></div><div id=c2></div></div></body>", 480);
    TEST_ASSERT_EQUAL(15, lot52_node("c0")->x);
    TEST_ASSERT_EQUAL(60, lot52_node("c1")->x);
    TEST_ASSERT_EQUAL(105, lot52_node("c2")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_jc_262(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:150px;height:30px;grid-template-columns:30px 30px 30px;grid-template-rows:20px;justify-content:start;align-items:stretch'><div id=c0></div><div id=c1></div><div id=c2></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("c0")->x);
    TEST_ASSERT_EQUAL(30, lot52_node("c1")->x);
    TEST_ASSERT_EQUAL(60, lot52_node("c2")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_jc_263(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:150px;height:30px;grid-template-columns:30px 30px 30px;grid-template-rows:20px;justify-content:end;align-items:stretch'><div id=c0></div><div id=c1></div><div id=c2></div></div></body>", 480);
    TEST_ASSERT_EQUAL(60, lot52_node("c0")->x);
    TEST_ASSERT_EQUAL(90, lot52_node("c1")->x);
    TEST_ASSERT_EQUAL(120, lot52_node("c2")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_jc_264(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:150px;height:30px;grid-template-columns:30px 30px 30px;grid-template-rows:20px;justify-content:center;align-items:stretch'><div id=c0></div><div id=c1></div><div id=c2></div></div></body>", 480);
    TEST_ASSERT_EQUAL(30, lot52_node("c0")->x);
    TEST_ASSERT_EQUAL(60, lot52_node("c1")->x);
    TEST_ASSERT_EQUAL(90, lot52_node("c2")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_jc_265(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:120px;height:30px;grid-template-columns:30px 30px 30px 30px;grid-template-rows:20px;justify-content:space-between;align-items:stretch'><div id=c0></div><div id=c1></div><div id=c2></div><div id=c3></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("c0")->x);
    TEST_ASSERT_EQUAL(30, lot52_node("c1")->x);
    TEST_ASSERT_EQUAL(60, lot52_node("c2")->x);
    TEST_ASSERT_EQUAL(90, lot52_node("c3")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_jc_266(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:120px;height:30px;grid-template-columns:30px 30px 30px 30px;grid-template-rows:20px;justify-content:space-around;align-items:stretch'><div id=c0></div><div id=c1></div><div id=c2></div><div id=c3></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("c0")->x);
    TEST_ASSERT_EQUAL(30, lot52_node("c1")->x);
    TEST_ASSERT_EQUAL(60, lot52_node("c2")->x);
    TEST_ASSERT_EQUAL(90, lot52_node("c3")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_jc_267(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:120px;height:30px;grid-template-columns:30px 30px 30px 30px;grid-template-rows:20px;justify-content:space-evenly;align-items:stretch'><div id=c0></div><div id=c1></div><div id=c2></div><div id=c3></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("c0")->x);
    TEST_ASSERT_EQUAL(30, lot52_node("c1")->x);
    TEST_ASSERT_EQUAL(60, lot52_node("c2")->x);
    TEST_ASSERT_EQUAL(90, lot52_node("c3")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_jc_268(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:120px;height:30px;grid-template-columns:30px 30px 30px 30px;grid-template-rows:20px;justify-content:start;align-items:stretch'><div id=c0></div><div id=c1></div><div id=c2></div><div id=c3></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("c0")->x);
    TEST_ASSERT_EQUAL(30, lot52_node("c1")->x);
    TEST_ASSERT_EQUAL(60, lot52_node("c2")->x);
    TEST_ASSERT_EQUAL(90, lot52_node("c3")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_jc_269(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:120px;height:30px;grid-template-columns:30px 30px 30px 30px;grid-template-rows:20px;justify-content:end;align-items:stretch'><div id=c0></div><div id=c1></div><div id=c2></div><div id=c3></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("c0")->x);
    TEST_ASSERT_EQUAL(30, lot52_node("c1")->x);
    TEST_ASSERT_EQUAL(60, lot52_node("c2")->x);
    TEST_ASSERT_EQUAL(90, lot52_node("c3")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_jc_270(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:120px;height:30px;grid-template-columns:30px 30px 30px 30px;grid-template-rows:20px;justify-content:center;align-items:stretch'><div id=c0></div><div id=c1></div><div id=c2></div><div id=c3></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("c0")->x);
    TEST_ASSERT_EQUAL(30, lot52_node("c1")->x);
    TEST_ASSERT_EQUAL(60, lot52_node("c2")->x);
    TEST_ASSERT_EQUAL(90, lot52_node("c3")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_jc_271(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:140px;height:30px;grid-template-columns:30px 30px 30px 30px;grid-template-rows:20px;justify-content:space-between;align-items:stretch'><div id=c0></div><div id=c1></div><div id=c2></div><div id=c3></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("c0")->x);
    TEST_ASSERT_EQUAL(36, lot52_node("c1")->x);
    TEST_ASSERT_EQUAL(73, lot52_node("c2")->x);
    TEST_ASSERT_EQUAL(110, lot52_node("c3")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_jc_272(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:140px;height:30px;grid-template-columns:30px 30px 30px 30px;grid-template-rows:20px;justify-content:space-around;align-items:stretch'><div id=c0></div><div id=c1></div><div id=c2></div><div id=c3></div></div></body>", 480);
    TEST_ASSERT_EQUAL(2, lot52_node("c0")->x);
    TEST_ASSERT_EQUAL(37, lot52_node("c1")->x);
    TEST_ASSERT_EQUAL(72, lot52_node("c2")->x);
    TEST_ASSERT_EQUAL(107, lot52_node("c3")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_jc_273(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:140px;height:30px;grid-template-columns:30px 30px 30px 30px;grid-template-rows:20px;justify-content:space-evenly;align-items:stretch'><div id=c0></div><div id=c1></div><div id=c2></div><div id=c3></div></div></body>", 480);
    TEST_ASSERT_EQUAL(4, lot52_node("c0")->x);
    TEST_ASSERT_EQUAL(38, lot52_node("c1")->x);
    TEST_ASSERT_EQUAL(72, lot52_node("c2")->x);
    TEST_ASSERT_EQUAL(106, lot52_node("c3")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_jc_274(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:140px;height:30px;grid-template-columns:30px 30px 30px 30px;grid-template-rows:20px;justify-content:start;align-items:stretch'><div id=c0></div><div id=c1></div><div id=c2></div><div id=c3></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("c0")->x);
    TEST_ASSERT_EQUAL(30, lot52_node("c1")->x);
    TEST_ASSERT_EQUAL(60, lot52_node("c2")->x);
    TEST_ASSERT_EQUAL(90, lot52_node("c3")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_jc_275(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:140px;height:30px;grid-template-columns:30px 30px 30px 30px;grid-template-rows:20px;justify-content:end;align-items:stretch'><div id=c0></div><div id=c1></div><div id=c2></div><div id=c3></div></div></body>", 480);
    TEST_ASSERT_EQUAL(20, lot52_node("c0")->x);
    TEST_ASSERT_EQUAL(50, lot52_node("c1")->x);
    TEST_ASSERT_EQUAL(80, lot52_node("c2")->x);
    TEST_ASSERT_EQUAL(110, lot52_node("c3")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_jc_276(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:140px;height:30px;grid-template-columns:30px 30px 30px 30px;grid-template-rows:20px;justify-content:center;align-items:stretch'><div id=c0></div><div id=c1></div><div id=c2></div><div id=c3></div></div></body>", 480);
    TEST_ASSERT_EQUAL(10, lot52_node("c0")->x);
    TEST_ASSERT_EQUAL(40, lot52_node("c1")->x);
    TEST_ASSERT_EQUAL(70, lot52_node("c2")->x);
    TEST_ASSERT_EQUAL(100, lot52_node("c3")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_jc_277(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:160px;height:30px;grid-template-columns:30px 30px 30px 30px;grid-template-rows:20px;justify-content:space-between;align-items:stretch'><div id=c0></div><div id=c1></div><div id=c2></div><div id=c3></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("c0")->x);
    TEST_ASSERT_EQUAL(43, lot52_node("c1")->x);
    TEST_ASSERT_EQUAL(86, lot52_node("c2")->x);
    TEST_ASSERT_EQUAL(130, lot52_node("c3")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_jc_278(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:160px;height:30px;grid-template-columns:30px 30px 30px 30px;grid-template-rows:20px;justify-content:space-around;align-items:stretch'><div id=c0></div><div id=c1></div><div id=c2></div><div id=c3></div></div></body>", 480);
    TEST_ASSERT_EQUAL(5, lot52_node("c0")->x);
    TEST_ASSERT_EQUAL(45, lot52_node("c1")->x);
    TEST_ASSERT_EQUAL(85, lot52_node("c2")->x);
    TEST_ASSERT_EQUAL(125, lot52_node("c3")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_jc_279(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:160px;height:30px;grid-template-columns:30px 30px 30px 30px;grid-template-rows:20px;justify-content:space-evenly;align-items:stretch'><div id=c0></div><div id=c1></div><div id=c2></div><div id=c3></div></div></body>", 480);
    TEST_ASSERT_EQUAL(8, lot52_node("c0")->x);
    TEST_ASSERT_EQUAL(46, lot52_node("c1")->x);
    TEST_ASSERT_EQUAL(84, lot52_node("c2")->x);
    TEST_ASSERT_EQUAL(122, lot52_node("c3")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_jc_280(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:160px;height:30px;grid-template-columns:30px 30px 30px 30px;grid-template-rows:20px;justify-content:start;align-items:stretch'><div id=c0></div><div id=c1></div><div id=c2></div><div id=c3></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("c0")->x);
    TEST_ASSERT_EQUAL(30, lot52_node("c1")->x);
    TEST_ASSERT_EQUAL(60, lot52_node("c2")->x);
    TEST_ASSERT_EQUAL(90, lot52_node("c3")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_jc_281(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:160px;height:30px;grid-template-columns:30px 30px 30px 30px;grid-template-rows:20px;justify-content:end;align-items:stretch'><div id=c0></div><div id=c1></div><div id=c2></div><div id=c3></div></div></body>", 480);
    TEST_ASSERT_EQUAL(40, lot52_node("c0")->x);
    TEST_ASSERT_EQUAL(70, lot52_node("c1")->x);
    TEST_ASSERT_EQUAL(100, lot52_node("c2")->x);
    TEST_ASSERT_EQUAL(130, lot52_node("c3")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_jc_282(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:160px;height:30px;grid-template-columns:30px 30px 30px 30px;grid-template-rows:20px;justify-content:center;align-items:stretch'><div id=c0></div><div id=c1></div><div id=c2></div><div id=c3></div></div></body>", 480);
    TEST_ASSERT_EQUAL(20, lot52_node("c0")->x);
    TEST_ASSERT_EQUAL(50, lot52_node("c1")->x);
    TEST_ASSERT_EQUAL(80, lot52_node("c2")->x);
    TEST_ASSERT_EQUAL(110, lot52_node("c3")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_jc_283(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:180px;height:30px;grid-template-columns:30px 30px 30px 30px;grid-template-rows:20px;justify-content:space-between;align-items:stretch'><div id=c0></div><div id=c1></div><div id=c2></div><div id=c3></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("c0")->x);
    TEST_ASSERT_EQUAL(50, lot52_node("c1")->x);
    TEST_ASSERT_EQUAL(100, lot52_node("c2")->x);
    TEST_ASSERT_EQUAL(150, lot52_node("c3")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_jc_284(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:180px;height:30px;grid-template-columns:30px 30px 30px 30px;grid-template-rows:20px;justify-content:space-around;align-items:stretch'><div id=c0></div><div id=c1></div><div id=c2></div><div id=c3></div></div></body>", 480);
    TEST_ASSERT_EQUAL(7, lot52_node("c0")->x);
    TEST_ASSERT_EQUAL(52, lot52_node("c1")->x);
    TEST_ASSERT_EQUAL(97, lot52_node("c2")->x);
    TEST_ASSERT_EQUAL(142, lot52_node("c3")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_jc_285(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:180px;height:30px;grid-template-columns:30px 30px 30px 30px;grid-template-rows:20px;justify-content:space-evenly;align-items:stretch'><div id=c0></div><div id=c1></div><div id=c2></div><div id=c3></div></div></body>", 480);
    TEST_ASSERT_EQUAL(12, lot52_node("c0")->x);
    TEST_ASSERT_EQUAL(54, lot52_node("c1")->x);
    TEST_ASSERT_EQUAL(96, lot52_node("c2")->x);
    TEST_ASSERT_EQUAL(138, lot52_node("c3")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_jc_286(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:180px;height:30px;grid-template-columns:30px 30px 30px 30px;grid-template-rows:20px;justify-content:start;align-items:stretch'><div id=c0></div><div id=c1></div><div id=c2></div><div id=c3></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("c0")->x);
    TEST_ASSERT_EQUAL(30, lot52_node("c1")->x);
    TEST_ASSERT_EQUAL(60, lot52_node("c2")->x);
    TEST_ASSERT_EQUAL(90, lot52_node("c3")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_jc_287(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:180px;height:30px;grid-template-columns:30px 30px 30px 30px;grid-template-rows:20px;justify-content:end;align-items:stretch'><div id=c0></div><div id=c1></div><div id=c2></div><div id=c3></div></div></body>", 480);
    TEST_ASSERT_EQUAL(60, lot52_node("c0")->x);
    TEST_ASSERT_EQUAL(90, lot52_node("c1")->x);
    TEST_ASSERT_EQUAL(120, lot52_node("c2")->x);
    TEST_ASSERT_EQUAL(150, lot52_node("c3")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_jc_288(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:180px;height:30px;grid-template-columns:30px 30px 30px 30px;grid-template-rows:20px;justify-content:center;align-items:stretch'><div id=c0></div><div id=c1></div><div id=c2></div><div id=c3></div></div></body>", 480);
    TEST_ASSERT_EQUAL(30, lot52_node("c0")->x);
    TEST_ASSERT_EQUAL(60, lot52_node("c1")->x);
    TEST_ASSERT_EQUAL(90, lot52_node("c2")->x);
    TEST_ASSERT_EQUAL(120, lot52_node("c3")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_ac_289(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:16px;grid-template-columns:40px;grid-template-rows:8px 8px;align-content:space-between;justify-items:stretch;align-items:stretch'><div id=r0></div><div id=r1></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("r0")->y);
    TEST_ASSERT_EQUAL(8, lot52_node("r1")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_ac_290(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:16px;grid-template-columns:40px;grid-template-rows:8px 8px;align-content:space-around;justify-items:stretch;align-items:stretch'><div id=r0></div><div id=r1></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("r0")->y);
    TEST_ASSERT_EQUAL(8, lot52_node("r1")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_ac_291(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:16px;grid-template-columns:40px;grid-template-rows:8px 8px;align-content:space-evenly;justify-items:stretch;align-items:stretch'><div id=r0></div><div id=r1></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("r0")->y);
    TEST_ASSERT_EQUAL(8, lot52_node("r1")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_ac_292(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:16px;grid-template-columns:40px;grid-template-rows:8px 8px;align-content:start;justify-items:stretch;align-items:stretch'><div id=r0></div><div id=r1></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("r0")->y);
    TEST_ASSERT_EQUAL(8, lot52_node("r1")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_ac_293(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:16px;grid-template-columns:40px;grid-template-rows:8px 8px;align-content:end;justify-items:stretch;align-items:stretch'><div id=r0></div><div id=r1></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("r0")->y);
    TEST_ASSERT_EQUAL(8, lot52_node("r1")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_ac_294(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:16px;grid-template-columns:40px;grid-template-rows:8px 8px;align-content:center;justify-items:stretch;align-items:stretch'><div id=r0></div><div id=r1></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("r0")->y);
    TEST_ASSERT_EQUAL(8, lot52_node("r1")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_ac_295(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:36px;grid-template-columns:40px;grid-template-rows:8px 8px;align-content:space-between;justify-items:stretch;align-items:stretch'><div id=r0></div><div id=r1></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("r0")->y);
    TEST_ASSERT_EQUAL(28, lot52_node("r1")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_ac_296(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:36px;grid-template-columns:40px;grid-template-rows:8px 8px;align-content:space-around;justify-items:stretch;align-items:stretch'><div id=r0></div><div id=r1></div></div></body>", 480);
    TEST_ASSERT_EQUAL(5, lot52_node("r0")->y);
    TEST_ASSERT_EQUAL(23, lot52_node("r1")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_ac_297(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:36px;grid-template-columns:40px;grid-template-rows:8px 8px;align-content:space-evenly;justify-items:stretch;align-items:stretch'><div id=r0></div><div id=r1></div></div></body>", 480);
    TEST_ASSERT_EQUAL(6, lot52_node("r0")->y);
    TEST_ASSERT_EQUAL(21, lot52_node("r1")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_ac_298(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:36px;grid-template-columns:40px;grid-template-rows:8px 8px;align-content:start;justify-items:stretch;align-items:stretch'><div id=r0></div><div id=r1></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("r0")->y);
    TEST_ASSERT_EQUAL(8, lot52_node("r1")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_ac_299(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:36px;grid-template-columns:40px;grid-template-rows:8px 8px;align-content:end;justify-items:stretch;align-items:stretch'><div id=r0></div><div id=r1></div></div></body>", 480);
    TEST_ASSERT_EQUAL(20, lot52_node("r0")->y);
    TEST_ASSERT_EQUAL(28, lot52_node("r1")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_ac_300(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:36px;grid-template-columns:40px;grid-template-rows:8px 8px;align-content:center;justify-items:stretch;align-items:stretch'><div id=r0></div><div id=r1></div></div></body>", 480);
    TEST_ASSERT_EQUAL(10, lot52_node("r0")->y);
    TEST_ASSERT_EQUAL(18, lot52_node("r1")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_ac_301(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:46px;grid-template-columns:40px;grid-template-rows:8px 8px;align-content:space-between;justify-items:stretch;align-items:stretch'><div id=r0></div><div id=r1></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("r0")->y);
    TEST_ASSERT_EQUAL(38, lot52_node("r1")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_ac_302(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:46px;grid-template-columns:40px;grid-template-rows:8px 8px;align-content:space-around;justify-items:stretch;align-items:stretch'><div id=r0></div><div id=r1></div></div></body>", 480);
    TEST_ASSERT_EQUAL(7, lot52_node("r0")->y);
    TEST_ASSERT_EQUAL(30, lot52_node("r1")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_ac_303(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:46px;grid-template-columns:40px;grid-template-rows:8px 8px;align-content:space-evenly;justify-items:stretch;align-items:stretch'><div id=r0></div><div id=r1></div></div></body>", 480);
    TEST_ASSERT_EQUAL(10, lot52_node("r0")->y);
    TEST_ASSERT_EQUAL(28, lot52_node("r1")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_ac_304(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:46px;grid-template-columns:40px;grid-template-rows:8px 8px;align-content:start;justify-items:stretch;align-items:stretch'><div id=r0></div><div id=r1></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("r0")->y);
    TEST_ASSERT_EQUAL(8, lot52_node("r1")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_ac_305(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:46px;grid-template-columns:40px;grid-template-rows:8px 8px;align-content:end;justify-items:stretch;align-items:stretch'><div id=r0></div><div id=r1></div></div></body>", 480);
    TEST_ASSERT_EQUAL(30, lot52_node("r0")->y);
    TEST_ASSERT_EQUAL(38, lot52_node("r1")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_ac_306(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:46px;grid-template-columns:40px;grid-template-rows:8px 8px;align-content:center;justify-items:stretch;align-items:stretch'><div id=r0></div><div id=r1></div></div></body>", 480);
    TEST_ASSERT_EQUAL(15, lot52_node("r0")->y);
    TEST_ASSERT_EQUAL(23, lot52_node("r1")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_ac_307(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:56px;grid-template-columns:40px;grid-template-rows:8px 8px;align-content:space-between;justify-items:stretch;align-items:stretch'><div id=r0></div><div id=r1></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("r0")->y);
    TEST_ASSERT_EQUAL(48, lot52_node("r1")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_ac_308(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:56px;grid-template-columns:40px;grid-template-rows:8px 8px;align-content:space-around;justify-items:stretch;align-items:stretch'><div id=r0></div><div id=r1></div></div></body>", 480);
    TEST_ASSERT_EQUAL(10, lot52_node("r0")->y);
    TEST_ASSERT_EQUAL(38, lot52_node("r1")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_ac_309(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:56px;grid-template-columns:40px;grid-template-rows:8px 8px;align-content:space-evenly;justify-items:stretch;align-items:stretch'><div id=r0></div><div id=r1></div></div></body>", 480);
    TEST_ASSERT_EQUAL(13, lot52_node("r0")->y);
    TEST_ASSERT_EQUAL(34, lot52_node("r1")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_ac_310(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:56px;grid-template-columns:40px;grid-template-rows:8px 8px;align-content:start;justify-items:stretch;align-items:stretch'><div id=r0></div><div id=r1></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("r0")->y);
    TEST_ASSERT_EQUAL(8, lot52_node("r1")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_ac_311(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:56px;grid-template-columns:40px;grid-template-rows:8px 8px;align-content:end;justify-items:stretch;align-items:stretch'><div id=r0></div><div id=r1></div></div></body>", 480);
    TEST_ASSERT_EQUAL(40, lot52_node("r0")->y);
    TEST_ASSERT_EQUAL(48, lot52_node("r1")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_ac_312(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:56px;grid-template-columns:40px;grid-template-rows:8px 8px;align-content:center;justify-items:stretch;align-items:stretch'><div id=r0></div><div id=r1></div></div></body>", 480);
    TEST_ASSERT_EQUAL(20, lot52_node("r0")->y);
    TEST_ASSERT_EQUAL(28, lot52_node("r1")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_ac_313(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:24px;grid-template-columns:40px;grid-template-rows:8px 8px 8px;align-content:space-between;justify-items:stretch;align-items:stretch'><div id=r0></div><div id=r1></div><div id=r2></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("r0")->y);
    TEST_ASSERT_EQUAL(8, lot52_node("r1")->y);
    TEST_ASSERT_EQUAL(16, lot52_node("r2")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_ac_314(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:24px;grid-template-columns:40px;grid-template-rows:8px 8px 8px;align-content:space-around;justify-items:stretch;align-items:stretch'><div id=r0></div><div id=r1></div><div id=r2></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("r0")->y);
    TEST_ASSERT_EQUAL(8, lot52_node("r1")->y);
    TEST_ASSERT_EQUAL(16, lot52_node("r2")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_ac_315(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:24px;grid-template-columns:40px;grid-template-rows:8px 8px 8px;align-content:space-evenly;justify-items:stretch;align-items:stretch'><div id=r0></div><div id=r1></div><div id=r2></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("r0")->y);
    TEST_ASSERT_EQUAL(8, lot52_node("r1")->y);
    TEST_ASSERT_EQUAL(16, lot52_node("r2")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_ac_316(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:24px;grid-template-columns:40px;grid-template-rows:8px 8px 8px;align-content:start;justify-items:stretch;align-items:stretch'><div id=r0></div><div id=r1></div><div id=r2></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("r0")->y);
    TEST_ASSERT_EQUAL(8, lot52_node("r1")->y);
    TEST_ASSERT_EQUAL(16, lot52_node("r2")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_ac_317(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:24px;grid-template-columns:40px;grid-template-rows:8px 8px 8px;align-content:end;justify-items:stretch;align-items:stretch'><div id=r0></div><div id=r1></div><div id=r2></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("r0")->y);
    TEST_ASSERT_EQUAL(8, lot52_node("r1")->y);
    TEST_ASSERT_EQUAL(16, lot52_node("r2")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_ac_318(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:24px;grid-template-columns:40px;grid-template-rows:8px 8px 8px;align-content:center;justify-items:stretch;align-items:stretch'><div id=r0></div><div id=r1></div><div id=r2></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("r0")->y);
    TEST_ASSERT_EQUAL(8, lot52_node("r1")->y);
    TEST_ASSERT_EQUAL(16, lot52_node("r2")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_ac_319(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:44px;grid-template-columns:40px;grid-template-rows:8px 8px 8px;align-content:space-between;justify-items:stretch;align-items:stretch'><div id=r0></div><div id=r1></div><div id=r2></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("r0")->y);
    TEST_ASSERT_EQUAL(18, lot52_node("r1")->y);
    TEST_ASSERT_EQUAL(36, lot52_node("r2")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_ac_320(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:44px;grid-template-columns:40px;grid-template-rows:8px 8px 8px;align-content:space-around;justify-items:stretch;align-items:stretch'><div id=r0></div><div id=r1></div><div id=r2></div></div></body>", 480);
    TEST_ASSERT_EQUAL(3, lot52_node("r0")->y);
    TEST_ASSERT_EQUAL(18, lot52_node("r1")->y);
    TEST_ASSERT_EQUAL(32, lot52_node("r2")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_ac_321(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:44px;grid-template-columns:40px;grid-template-rows:8px 8px 8px;align-content:space-evenly;justify-items:stretch;align-items:stretch'><div id=r0></div><div id=r1></div><div id=r2></div></div></body>", 480);
    TEST_ASSERT_EQUAL(5, lot52_node("r0")->y);
    TEST_ASSERT_EQUAL(18, lot52_node("r1")->y);
    TEST_ASSERT_EQUAL(31, lot52_node("r2")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_ac_322(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:44px;grid-template-columns:40px;grid-template-rows:8px 8px 8px;align-content:start;justify-items:stretch;align-items:stretch'><div id=r0></div><div id=r1></div><div id=r2></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("r0")->y);
    TEST_ASSERT_EQUAL(8, lot52_node("r1")->y);
    TEST_ASSERT_EQUAL(16, lot52_node("r2")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_ac_323(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:44px;grid-template-columns:40px;grid-template-rows:8px 8px 8px;align-content:end;justify-items:stretch;align-items:stretch'><div id=r0></div><div id=r1></div><div id=r2></div></div></body>", 480);
    TEST_ASSERT_EQUAL(20, lot52_node("r0")->y);
    TEST_ASSERT_EQUAL(28, lot52_node("r1")->y);
    TEST_ASSERT_EQUAL(36, lot52_node("r2")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_ac_324(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:44px;grid-template-columns:40px;grid-template-rows:8px 8px 8px;align-content:center;justify-items:stretch;align-items:stretch'><div id=r0></div><div id=r1></div><div id=r2></div></div></body>", 480);
    TEST_ASSERT_EQUAL(10, lot52_node("r0")->y);
    TEST_ASSERT_EQUAL(18, lot52_node("r1")->y);
    TEST_ASSERT_EQUAL(26, lot52_node("r2")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_ac_325(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:54px;grid-template-columns:40px;grid-template-rows:8px 8px 8px;align-content:space-between;justify-items:stretch;align-items:stretch'><div id=r0></div><div id=r1></div><div id=r2></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("r0")->y);
    TEST_ASSERT_EQUAL(23, lot52_node("r1")->y);
    TEST_ASSERT_EQUAL(46, lot52_node("r2")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_ac_326(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:54px;grid-template-columns:40px;grid-template-rows:8px 8px 8px;align-content:space-around;justify-items:stretch;align-items:stretch'><div id=r0></div><div id=r1></div><div id=r2></div></div></body>", 480);
    TEST_ASSERT_EQUAL(5, lot52_node("r0")->y);
    TEST_ASSERT_EQUAL(23, lot52_node("r1")->y);
    TEST_ASSERT_EQUAL(41, lot52_node("r2")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_ac_327(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:54px;grid-template-columns:40px;grid-template-rows:8px 8px 8px;align-content:space-evenly;justify-items:stretch;align-items:stretch'><div id=r0></div><div id=r1></div><div id=r2></div></div></body>", 480);
    TEST_ASSERT_EQUAL(7, lot52_node("r0")->y);
    TEST_ASSERT_EQUAL(23, lot52_node("r1")->y);
    TEST_ASSERT_EQUAL(38, lot52_node("r2")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_ac_328(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:54px;grid-template-columns:40px;grid-template-rows:8px 8px 8px;align-content:start;justify-items:stretch;align-items:stretch'><div id=r0></div><div id=r1></div><div id=r2></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("r0")->y);
    TEST_ASSERT_EQUAL(8, lot52_node("r1")->y);
    TEST_ASSERT_EQUAL(16, lot52_node("r2")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_ac_329(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:54px;grid-template-columns:40px;grid-template-rows:8px 8px 8px;align-content:end;justify-items:stretch;align-items:stretch'><div id=r0></div><div id=r1></div><div id=r2></div></div></body>", 480);
    TEST_ASSERT_EQUAL(30, lot52_node("r0")->y);
    TEST_ASSERT_EQUAL(38, lot52_node("r1")->y);
    TEST_ASSERT_EQUAL(46, lot52_node("r2")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_ac_330(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:54px;grid-template-columns:40px;grid-template-rows:8px 8px 8px;align-content:center;justify-items:stretch;align-items:stretch'><div id=r0></div><div id=r1></div><div id=r2></div></div></body>", 480);
    TEST_ASSERT_EQUAL(15, lot52_node("r0")->y);
    TEST_ASSERT_EQUAL(23, lot52_node("r1")->y);
    TEST_ASSERT_EQUAL(31, lot52_node("r2")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_ac_331(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:64px;grid-template-columns:40px;grid-template-rows:8px 8px 8px;align-content:space-between;justify-items:stretch;align-items:stretch'><div id=r0></div><div id=r1></div><div id=r2></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("r0")->y);
    TEST_ASSERT_EQUAL(28, lot52_node("r1")->y);
    TEST_ASSERT_EQUAL(56, lot52_node("r2")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_ac_332(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:64px;grid-template-columns:40px;grid-template-rows:8px 8px 8px;align-content:space-around;justify-items:stretch;align-items:stretch'><div id=r0></div><div id=r1></div><div id=r2></div></div></body>", 480);
    TEST_ASSERT_EQUAL(6, lot52_node("r0")->y);
    TEST_ASSERT_EQUAL(28, lot52_node("r1")->y);
    TEST_ASSERT_EQUAL(49, lot52_node("r2")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_ac_333(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:64px;grid-template-columns:40px;grid-template-rows:8px 8px 8px;align-content:space-evenly;justify-items:stretch;align-items:stretch'><div id=r0></div><div id=r1></div><div id=r2></div></div></body>", 480);
    TEST_ASSERT_EQUAL(10, lot52_node("r0")->y);
    TEST_ASSERT_EQUAL(28, lot52_node("r1")->y);
    TEST_ASSERT_EQUAL(46, lot52_node("r2")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_ac_334(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:64px;grid-template-columns:40px;grid-template-rows:8px 8px 8px;align-content:start;justify-items:stretch;align-items:stretch'><div id=r0></div><div id=r1></div><div id=r2></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("r0")->y);
    TEST_ASSERT_EQUAL(8, lot52_node("r1")->y);
    TEST_ASSERT_EQUAL(16, lot52_node("r2")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_ac_335(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:64px;grid-template-columns:40px;grid-template-rows:8px 8px 8px;align-content:end;justify-items:stretch;align-items:stretch'><div id=r0></div><div id=r1></div><div id=r2></div></div></body>", 480);
    TEST_ASSERT_EQUAL(40, lot52_node("r0")->y);
    TEST_ASSERT_EQUAL(48, lot52_node("r1")->y);
    TEST_ASSERT_EQUAL(56, lot52_node("r2")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_ac_336(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:64px;grid-template-columns:40px;grid-template-rows:8px 8px 8px;align-content:center;justify-items:stretch;align-items:stretch'><div id=r0></div><div id=r1></div><div id=r2></div></div></body>", 480);
    TEST_ASSERT_EQUAL(20, lot52_node("r0")->y);
    TEST_ASSERT_EQUAL(28, lot52_node("r1")->y);
    TEST_ASSERT_EQUAL(36, lot52_node("r2")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_ac_337(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:20px;grid-template-columns:40px;grid-template-rows:10px 10px;align-content:space-between;justify-items:stretch;align-items:stretch'><div id=r0></div><div id=r1></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("r0")->y);
    TEST_ASSERT_EQUAL(10, lot52_node("r1")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_ac_338(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:20px;grid-template-columns:40px;grid-template-rows:10px 10px;align-content:space-around;justify-items:stretch;align-items:stretch'><div id=r0></div><div id=r1></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("r0")->y);
    TEST_ASSERT_EQUAL(10, lot52_node("r1")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_ac_339(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:20px;grid-template-columns:40px;grid-template-rows:10px 10px;align-content:space-evenly;justify-items:stretch;align-items:stretch'><div id=r0></div><div id=r1></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("r0")->y);
    TEST_ASSERT_EQUAL(10, lot52_node("r1")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_ac_340(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:20px;grid-template-columns:40px;grid-template-rows:10px 10px;align-content:start;justify-items:stretch;align-items:stretch'><div id=r0></div><div id=r1></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("r0")->y);
    TEST_ASSERT_EQUAL(10, lot52_node("r1")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_ac_341(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:20px;grid-template-columns:40px;grid-template-rows:10px 10px;align-content:end;justify-items:stretch;align-items:stretch'><div id=r0></div><div id=r1></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("r0")->y);
    TEST_ASSERT_EQUAL(10, lot52_node("r1")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_ac_342(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:20px;grid-template-columns:40px;grid-template-rows:10px 10px;align-content:center;justify-items:stretch;align-items:stretch'><div id=r0></div><div id=r1></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("r0")->y);
    TEST_ASSERT_EQUAL(10, lot52_node("r1")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_ac_343(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:40px;grid-template-columns:40px;grid-template-rows:10px 10px;align-content:space-between;justify-items:stretch;align-items:stretch'><div id=r0></div><div id=r1></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("r0")->y);
    TEST_ASSERT_EQUAL(30, lot52_node("r1")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_ac_344(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:40px;grid-template-columns:40px;grid-template-rows:10px 10px;align-content:space-around;justify-items:stretch;align-items:stretch'><div id=r0></div><div id=r1></div></div></body>", 480);
    TEST_ASSERT_EQUAL(5, lot52_node("r0")->y);
    TEST_ASSERT_EQUAL(25, lot52_node("r1")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_ac_345(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:40px;grid-template-columns:40px;grid-template-rows:10px 10px;align-content:space-evenly;justify-items:stretch;align-items:stretch'><div id=r0></div><div id=r1></div></div></body>", 480);
    TEST_ASSERT_EQUAL(6, lot52_node("r0")->y);
    TEST_ASSERT_EQUAL(23, lot52_node("r1")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_ac_346(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:40px;grid-template-columns:40px;grid-template-rows:10px 10px;align-content:start;justify-items:stretch;align-items:stretch'><div id=r0></div><div id=r1></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("r0")->y);
    TEST_ASSERT_EQUAL(10, lot52_node("r1")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_ac_347(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:40px;grid-template-columns:40px;grid-template-rows:10px 10px;align-content:end;justify-items:stretch;align-items:stretch'><div id=r0></div><div id=r1></div></div></body>", 480);
    TEST_ASSERT_EQUAL(20, lot52_node("r0")->y);
    TEST_ASSERT_EQUAL(30, lot52_node("r1")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_ac_348(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:40px;grid-template-columns:40px;grid-template-rows:10px 10px;align-content:center;justify-items:stretch;align-items:stretch'><div id=r0></div><div id=r1></div></div></body>", 480);
    TEST_ASSERT_EQUAL(10, lot52_node("r0")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("r1")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_ac_349(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:50px;grid-template-columns:40px;grid-template-rows:10px 10px;align-content:space-between;justify-items:stretch;align-items:stretch'><div id=r0></div><div id=r1></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("r0")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("r1")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_ac_350(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:50px;grid-template-columns:40px;grid-template-rows:10px 10px;align-content:space-around;justify-items:stretch;align-items:stretch'><div id=r0></div><div id=r1></div></div></body>", 480);
    TEST_ASSERT_EQUAL(7, lot52_node("r0")->y);
    TEST_ASSERT_EQUAL(32, lot52_node("r1")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_ac_351(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:50px;grid-template-columns:40px;grid-template-rows:10px 10px;align-content:space-evenly;justify-items:stretch;align-items:stretch'><div id=r0></div><div id=r1></div></div></body>", 480);
    TEST_ASSERT_EQUAL(10, lot52_node("r0")->y);
    TEST_ASSERT_EQUAL(30, lot52_node("r1")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_ac_352(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:50px;grid-template-columns:40px;grid-template-rows:10px 10px;align-content:start;justify-items:stretch;align-items:stretch'><div id=r0></div><div id=r1></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("r0")->y);
    TEST_ASSERT_EQUAL(10, lot52_node("r1")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_ac_353(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:50px;grid-template-columns:40px;grid-template-rows:10px 10px;align-content:end;justify-items:stretch;align-items:stretch'><div id=r0></div><div id=r1></div></div></body>", 480);
    TEST_ASSERT_EQUAL(30, lot52_node("r0")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("r1")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_ac_354(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:50px;grid-template-columns:40px;grid-template-rows:10px 10px;align-content:center;justify-items:stretch;align-items:stretch'><div id=r0></div><div id=r1></div></div></body>", 480);
    TEST_ASSERT_EQUAL(15, lot52_node("r0")->y);
    TEST_ASSERT_EQUAL(25, lot52_node("r1")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_ac_355(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:60px;grid-template-columns:40px;grid-template-rows:10px 10px;align-content:space-between;justify-items:stretch;align-items:stretch'><div id=r0></div><div id=r1></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("r0")->y);
    TEST_ASSERT_EQUAL(50, lot52_node("r1")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_ac_356(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:60px;grid-template-columns:40px;grid-template-rows:10px 10px;align-content:space-around;justify-items:stretch;align-items:stretch'><div id=r0></div><div id=r1></div></div></body>", 480);
    TEST_ASSERT_EQUAL(10, lot52_node("r0")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("r1")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_ac_357(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:60px;grid-template-columns:40px;grid-template-rows:10px 10px;align-content:space-evenly;justify-items:stretch;align-items:stretch'><div id=r0></div><div id=r1></div></div></body>", 480);
    TEST_ASSERT_EQUAL(13, lot52_node("r0")->y);
    TEST_ASSERT_EQUAL(36, lot52_node("r1")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_ac_358(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:60px;grid-template-columns:40px;grid-template-rows:10px 10px;align-content:start;justify-items:stretch;align-items:stretch'><div id=r0></div><div id=r1></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("r0")->y);
    TEST_ASSERT_EQUAL(10, lot52_node("r1")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_ac_359(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:60px;grid-template-columns:40px;grid-template-rows:10px 10px;align-content:end;justify-items:stretch;align-items:stretch'><div id=r0></div><div id=r1></div></div></body>", 480);
    TEST_ASSERT_EQUAL(40, lot52_node("r0")->y);
    TEST_ASSERT_EQUAL(50, lot52_node("r1")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_ac_360(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:60px;grid-template-columns:40px;grid-template-rows:10px 10px;align-content:center;justify-items:stretch;align-items:stretch'><div id=r0></div><div id=r1></div></div></body>", 480);
    TEST_ASSERT_EQUAL(20, lot52_node("r0")->y);
    TEST_ASSERT_EQUAL(30, lot52_node("r1")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_ac_361(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:30px;grid-template-columns:40px;grid-template-rows:10px 10px 10px;align-content:space-between;justify-items:stretch;align-items:stretch'><div id=r0></div><div id=r1></div><div id=r2></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("r0")->y);
    TEST_ASSERT_EQUAL(10, lot52_node("r1")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("r2")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_ac_362(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:30px;grid-template-columns:40px;grid-template-rows:10px 10px 10px;align-content:space-around;justify-items:stretch;align-items:stretch'><div id=r0></div><div id=r1></div><div id=r2></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("r0")->y);
    TEST_ASSERT_EQUAL(10, lot52_node("r1")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("r2")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_ac_363(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:30px;grid-template-columns:40px;grid-template-rows:10px 10px 10px;align-content:space-evenly;justify-items:stretch;align-items:stretch'><div id=r0></div><div id=r1></div><div id=r2></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("r0")->y);
    TEST_ASSERT_EQUAL(10, lot52_node("r1")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("r2")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_ac_364(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:30px;grid-template-columns:40px;grid-template-rows:10px 10px 10px;align-content:start;justify-items:stretch;align-items:stretch'><div id=r0></div><div id=r1></div><div id=r2></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("r0")->y);
    TEST_ASSERT_EQUAL(10, lot52_node("r1")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("r2")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_ac_365(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:30px;grid-template-columns:40px;grid-template-rows:10px 10px 10px;align-content:end;justify-items:stretch;align-items:stretch'><div id=r0></div><div id=r1></div><div id=r2></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("r0")->y);
    TEST_ASSERT_EQUAL(10, lot52_node("r1")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("r2")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_ac_366(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:30px;grid-template-columns:40px;grid-template-rows:10px 10px 10px;align-content:center;justify-items:stretch;align-items:stretch'><div id=r0></div><div id=r1></div><div id=r2></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("r0")->y);
    TEST_ASSERT_EQUAL(10, lot52_node("r1")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("r2")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_ac_367(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:50px;grid-template-columns:40px;grid-template-rows:10px 10px 10px;align-content:space-between;justify-items:stretch;align-items:stretch'><div id=r0></div><div id=r1></div><div id=r2></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("r0")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("r1")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("r2")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_ac_368(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:50px;grid-template-columns:40px;grid-template-rows:10px 10px 10px;align-content:space-around;justify-items:stretch;align-items:stretch'><div id=r0></div><div id=r1></div><div id=r2></div></div></body>", 480);
    TEST_ASSERT_EQUAL(3, lot52_node("r0")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("r1")->y);
    TEST_ASSERT_EQUAL(36, lot52_node("r2")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_ac_369(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:50px;grid-template-columns:40px;grid-template-rows:10px 10px 10px;align-content:space-evenly;justify-items:stretch;align-items:stretch'><div id=r0></div><div id=r1></div><div id=r2></div></div></body>", 480);
    TEST_ASSERT_EQUAL(5, lot52_node("r0")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("r1")->y);
    TEST_ASSERT_EQUAL(35, lot52_node("r2")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_ac_370(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:50px;grid-template-columns:40px;grid-template-rows:10px 10px 10px;align-content:start;justify-items:stretch;align-items:stretch'><div id=r0></div><div id=r1></div><div id=r2></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("r0")->y);
    TEST_ASSERT_EQUAL(10, lot52_node("r1")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("r2")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_ac_371(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:50px;grid-template-columns:40px;grid-template-rows:10px 10px 10px;align-content:end;justify-items:stretch;align-items:stretch'><div id=r0></div><div id=r1></div><div id=r2></div></div></body>", 480);
    TEST_ASSERT_EQUAL(20, lot52_node("r0")->y);
    TEST_ASSERT_EQUAL(30, lot52_node("r1")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("r2")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_ac_372(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:50px;grid-template-columns:40px;grid-template-rows:10px 10px 10px;align-content:center;justify-items:stretch;align-items:stretch'><div id=r0></div><div id=r1></div><div id=r2></div></div></body>", 480);
    TEST_ASSERT_EQUAL(10, lot52_node("r0")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("r1")->y);
    TEST_ASSERT_EQUAL(30, lot52_node("r2")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_ac_373(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:60px;grid-template-columns:40px;grid-template-rows:10px 10px 10px;align-content:space-between;justify-items:stretch;align-items:stretch'><div id=r0></div><div id=r1></div><div id=r2></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("r0")->y);
    TEST_ASSERT_EQUAL(25, lot52_node("r1")->y);
    TEST_ASSERT_EQUAL(50, lot52_node("r2")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_ac_374(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:60px;grid-template-columns:40px;grid-template-rows:10px 10px 10px;align-content:space-around;justify-items:stretch;align-items:stretch'><div id=r0></div><div id=r1></div><div id=r2></div></div></body>", 480);
    TEST_ASSERT_EQUAL(5, lot52_node("r0")->y);
    TEST_ASSERT_EQUAL(25, lot52_node("r1")->y);
    TEST_ASSERT_EQUAL(45, lot52_node("r2")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_ac_375(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:60px;grid-template-columns:40px;grid-template-rows:10px 10px 10px;align-content:space-evenly;justify-items:stretch;align-items:stretch'><div id=r0></div><div id=r1></div><div id=r2></div></div></body>", 480);
    TEST_ASSERT_EQUAL(7, lot52_node("r0")->y);
    TEST_ASSERT_EQUAL(25, lot52_node("r1")->y);
    TEST_ASSERT_EQUAL(42, lot52_node("r2")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_ac_376(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:60px;grid-template-columns:40px;grid-template-rows:10px 10px 10px;align-content:start;justify-items:stretch;align-items:stretch'><div id=r0></div><div id=r1></div><div id=r2></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("r0")->y);
    TEST_ASSERT_EQUAL(10, lot52_node("r1")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("r2")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_ac_377(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:60px;grid-template-columns:40px;grid-template-rows:10px 10px 10px;align-content:end;justify-items:stretch;align-items:stretch'><div id=r0></div><div id=r1></div><div id=r2></div></div></body>", 480);
    TEST_ASSERT_EQUAL(30, lot52_node("r0")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("r1")->y);
    TEST_ASSERT_EQUAL(50, lot52_node("r2")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_ac_378(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:60px;grid-template-columns:40px;grid-template-rows:10px 10px 10px;align-content:center;justify-items:stretch;align-items:stretch'><div id=r0></div><div id=r1></div><div id=r2></div></div></body>", 480);
    TEST_ASSERT_EQUAL(15, lot52_node("r0")->y);
    TEST_ASSERT_EQUAL(25, lot52_node("r1")->y);
    TEST_ASSERT_EQUAL(35, lot52_node("r2")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_ac_379(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:70px;grid-template-columns:40px;grid-template-rows:10px 10px 10px;align-content:space-between;justify-items:stretch;align-items:stretch'><div id=r0></div><div id=r1></div><div id=r2></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("r0")->y);
    TEST_ASSERT_EQUAL(30, lot52_node("r1")->y);
    TEST_ASSERT_EQUAL(60, lot52_node("r2")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_ac_380(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:70px;grid-template-columns:40px;grid-template-rows:10px 10px 10px;align-content:space-around;justify-items:stretch;align-items:stretch'><div id=r0></div><div id=r1></div><div id=r2></div></div></body>", 480);
    TEST_ASSERT_EQUAL(6, lot52_node("r0")->y);
    TEST_ASSERT_EQUAL(30, lot52_node("r1")->y);
    TEST_ASSERT_EQUAL(53, lot52_node("r2")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_ac_381(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:70px;grid-template-columns:40px;grid-template-rows:10px 10px 10px;align-content:space-evenly;justify-items:stretch;align-items:stretch'><div id=r0></div><div id=r1></div><div id=r2></div></div></body>", 480);
    TEST_ASSERT_EQUAL(10, lot52_node("r0")->y);
    TEST_ASSERT_EQUAL(30, lot52_node("r1")->y);
    TEST_ASSERT_EQUAL(50, lot52_node("r2")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_ac_382(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:70px;grid-template-columns:40px;grid-template-rows:10px 10px 10px;align-content:start;justify-items:stretch;align-items:stretch'><div id=r0></div><div id=r1></div><div id=r2></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("r0")->y);
    TEST_ASSERT_EQUAL(10, lot52_node("r1")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("r2")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_ac_383(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:70px;grid-template-columns:40px;grid-template-rows:10px 10px 10px;align-content:end;justify-items:stretch;align-items:stretch'><div id=r0></div><div id=r1></div><div id=r2></div></div></body>", 480);
    TEST_ASSERT_EQUAL(40, lot52_node("r0")->y);
    TEST_ASSERT_EQUAL(50, lot52_node("r1")->y);
    TEST_ASSERT_EQUAL(60, lot52_node("r2")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_ac_384(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:70px;grid-template-columns:40px;grid-template-rows:10px 10px 10px;align-content:center;justify-items:stretch;align-items:stretch'><div id=r0></div><div id=r1></div><div id=r2></div></div></body>", 480);
    TEST_ASSERT_EQUAL(20, lot52_node("r0")->y);
    TEST_ASSERT_EQUAL(30, lot52_node("r1")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("r2")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_ac_385(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:24px;grid-template-columns:40px;grid-template-rows:12px 12px;align-content:space-between;justify-items:stretch;align-items:stretch'><div id=r0></div><div id=r1></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("r0")->y);
    TEST_ASSERT_EQUAL(12, lot52_node("r1")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_ac_386(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:24px;grid-template-columns:40px;grid-template-rows:12px 12px;align-content:space-around;justify-items:stretch;align-items:stretch'><div id=r0></div><div id=r1></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("r0")->y);
    TEST_ASSERT_EQUAL(12, lot52_node("r1")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_ac_387(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:24px;grid-template-columns:40px;grid-template-rows:12px 12px;align-content:space-evenly;justify-items:stretch;align-items:stretch'><div id=r0></div><div id=r1></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("r0")->y);
    TEST_ASSERT_EQUAL(12, lot52_node("r1")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_ac_388(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:24px;grid-template-columns:40px;grid-template-rows:12px 12px;align-content:start;justify-items:stretch;align-items:stretch'><div id=r0></div><div id=r1></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("r0")->y);
    TEST_ASSERT_EQUAL(12, lot52_node("r1")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_ac_389(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:24px;grid-template-columns:40px;grid-template-rows:12px 12px;align-content:end;justify-items:stretch;align-items:stretch'><div id=r0></div><div id=r1></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("r0")->y);
    TEST_ASSERT_EQUAL(12, lot52_node("r1")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_ac_390(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:24px;grid-template-columns:40px;grid-template-rows:12px 12px;align-content:center;justify-items:stretch;align-items:stretch'><div id=r0></div><div id=r1></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("r0")->y);
    TEST_ASSERT_EQUAL(12, lot52_node("r1")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_ac_391(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:44px;grid-template-columns:40px;grid-template-rows:12px 12px;align-content:space-between;justify-items:stretch;align-items:stretch'><div id=r0></div><div id=r1></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("r0")->y);
    TEST_ASSERT_EQUAL(32, lot52_node("r1")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_ac_392(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:44px;grid-template-columns:40px;grid-template-rows:12px 12px;align-content:space-around;justify-items:stretch;align-items:stretch'><div id=r0></div><div id=r1></div></div></body>", 480);
    TEST_ASSERT_EQUAL(5, lot52_node("r0")->y);
    TEST_ASSERT_EQUAL(27, lot52_node("r1")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_ac_393(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:44px;grid-template-columns:40px;grid-template-rows:12px 12px;align-content:space-evenly;justify-items:stretch;align-items:stretch'><div id=r0></div><div id=r1></div></div></body>", 480);
    TEST_ASSERT_EQUAL(6, lot52_node("r0")->y);
    TEST_ASSERT_EQUAL(25, lot52_node("r1")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_ac_394(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:44px;grid-template-columns:40px;grid-template-rows:12px 12px;align-content:start;justify-items:stretch;align-items:stretch'><div id=r0></div><div id=r1></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("r0")->y);
    TEST_ASSERT_EQUAL(12, lot52_node("r1")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_ac_395(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:44px;grid-template-columns:40px;grid-template-rows:12px 12px;align-content:end;justify-items:stretch;align-items:stretch'><div id=r0></div><div id=r1></div></div></body>", 480);
    TEST_ASSERT_EQUAL(20, lot52_node("r0")->y);
    TEST_ASSERT_EQUAL(32, lot52_node("r1")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_ac_396(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:44px;grid-template-columns:40px;grid-template-rows:12px 12px;align-content:center;justify-items:stretch;align-items:stretch'><div id=r0></div><div id=r1></div></div></body>", 480);
    TEST_ASSERT_EQUAL(10, lot52_node("r0")->y);
    TEST_ASSERT_EQUAL(22, lot52_node("r1")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_ac_397(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:54px;grid-template-columns:40px;grid-template-rows:12px 12px;align-content:space-between;justify-items:stretch;align-items:stretch'><div id=r0></div><div id=r1></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("r0")->y);
    TEST_ASSERT_EQUAL(42, lot52_node("r1")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_ac_398(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:54px;grid-template-columns:40px;grid-template-rows:12px 12px;align-content:space-around;justify-items:stretch;align-items:stretch'><div id=r0></div><div id=r1></div></div></body>", 480);
    TEST_ASSERT_EQUAL(7, lot52_node("r0")->y);
    TEST_ASSERT_EQUAL(34, lot52_node("r1")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_ac_399(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:54px;grid-template-columns:40px;grid-template-rows:12px 12px;align-content:space-evenly;justify-items:stretch;align-items:stretch'><div id=r0></div><div id=r1></div></div></body>", 480);
    TEST_ASSERT_EQUAL(10, lot52_node("r0")->y);
    TEST_ASSERT_EQUAL(32, lot52_node("r1")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_ac_400(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:54px;grid-template-columns:40px;grid-template-rows:12px 12px;align-content:start;justify-items:stretch;align-items:stretch'><div id=r0></div><div id=r1></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("r0")->y);
    TEST_ASSERT_EQUAL(12, lot52_node("r1")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_ac_401(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:54px;grid-template-columns:40px;grid-template-rows:12px 12px;align-content:end;justify-items:stretch;align-items:stretch'><div id=r0></div><div id=r1></div></div></body>", 480);
    TEST_ASSERT_EQUAL(30, lot52_node("r0")->y);
    TEST_ASSERT_EQUAL(42, lot52_node("r1")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_ac_402(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:54px;grid-template-columns:40px;grid-template-rows:12px 12px;align-content:center;justify-items:stretch;align-items:stretch'><div id=r0></div><div id=r1></div></div></body>", 480);
    TEST_ASSERT_EQUAL(15, lot52_node("r0")->y);
    TEST_ASSERT_EQUAL(27, lot52_node("r1")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_ac_403(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:64px;grid-template-columns:40px;grid-template-rows:12px 12px;align-content:space-between;justify-items:stretch;align-items:stretch'><div id=r0></div><div id=r1></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("r0")->y);
    TEST_ASSERT_EQUAL(52, lot52_node("r1")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_ac_404(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:64px;grid-template-columns:40px;grid-template-rows:12px 12px;align-content:space-around;justify-items:stretch;align-items:stretch'><div id=r0></div><div id=r1></div></div></body>", 480);
    TEST_ASSERT_EQUAL(10, lot52_node("r0")->y);
    TEST_ASSERT_EQUAL(42, lot52_node("r1")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_ac_405(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:64px;grid-template-columns:40px;grid-template-rows:12px 12px;align-content:space-evenly;justify-items:stretch;align-items:stretch'><div id=r0></div><div id=r1></div></div></body>", 480);
    TEST_ASSERT_EQUAL(13, lot52_node("r0")->y);
    TEST_ASSERT_EQUAL(38, lot52_node("r1")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_ac_406(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:64px;grid-template-columns:40px;grid-template-rows:12px 12px;align-content:start;justify-items:stretch;align-items:stretch'><div id=r0></div><div id=r1></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("r0")->y);
    TEST_ASSERT_EQUAL(12, lot52_node("r1")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_ac_407(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:64px;grid-template-columns:40px;grid-template-rows:12px 12px;align-content:end;justify-items:stretch;align-items:stretch'><div id=r0></div><div id=r1></div></div></body>", 480);
    TEST_ASSERT_EQUAL(40, lot52_node("r0")->y);
    TEST_ASSERT_EQUAL(52, lot52_node("r1")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_ac_408(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:64px;grid-template-columns:40px;grid-template-rows:12px 12px;align-content:center;justify-items:stretch;align-items:stretch'><div id=r0></div><div id=r1></div></div></body>", 480);
    TEST_ASSERT_EQUAL(20, lot52_node("r0")->y);
    TEST_ASSERT_EQUAL(32, lot52_node("r1")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_ac_409(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:36px;grid-template-columns:40px;grid-template-rows:12px 12px 12px;align-content:space-between;justify-items:stretch;align-items:stretch'><div id=r0></div><div id=r1></div><div id=r2></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("r0")->y);
    TEST_ASSERT_EQUAL(12, lot52_node("r1")->y);
    TEST_ASSERT_EQUAL(24, lot52_node("r2")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_ac_410(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:36px;grid-template-columns:40px;grid-template-rows:12px 12px 12px;align-content:space-around;justify-items:stretch;align-items:stretch'><div id=r0></div><div id=r1></div><div id=r2></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("r0")->y);
    TEST_ASSERT_EQUAL(12, lot52_node("r1")->y);
    TEST_ASSERT_EQUAL(24, lot52_node("r2")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_ac_411(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:36px;grid-template-columns:40px;grid-template-rows:12px 12px 12px;align-content:space-evenly;justify-items:stretch;align-items:stretch'><div id=r0></div><div id=r1></div><div id=r2></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("r0")->y);
    TEST_ASSERT_EQUAL(12, lot52_node("r1")->y);
    TEST_ASSERT_EQUAL(24, lot52_node("r2")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_ac_412(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:36px;grid-template-columns:40px;grid-template-rows:12px 12px 12px;align-content:start;justify-items:stretch;align-items:stretch'><div id=r0></div><div id=r1></div><div id=r2></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("r0")->y);
    TEST_ASSERT_EQUAL(12, lot52_node("r1")->y);
    TEST_ASSERT_EQUAL(24, lot52_node("r2")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_ac_413(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:36px;grid-template-columns:40px;grid-template-rows:12px 12px 12px;align-content:end;justify-items:stretch;align-items:stretch'><div id=r0></div><div id=r1></div><div id=r2></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("r0")->y);
    TEST_ASSERT_EQUAL(12, lot52_node("r1")->y);
    TEST_ASSERT_EQUAL(24, lot52_node("r2")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_ac_414(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:36px;grid-template-columns:40px;grid-template-rows:12px 12px 12px;align-content:center;justify-items:stretch;align-items:stretch'><div id=r0></div><div id=r1></div><div id=r2></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("r0")->y);
    TEST_ASSERT_EQUAL(12, lot52_node("r1")->y);
    TEST_ASSERT_EQUAL(24, lot52_node("r2")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_ac_415(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:56px;grid-template-columns:40px;grid-template-rows:12px 12px 12px;align-content:space-between;justify-items:stretch;align-items:stretch'><div id=r0></div><div id=r1></div><div id=r2></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("r0")->y);
    TEST_ASSERT_EQUAL(22, lot52_node("r1")->y);
    TEST_ASSERT_EQUAL(44, lot52_node("r2")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_ac_416(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:56px;grid-template-columns:40px;grid-template-rows:12px 12px 12px;align-content:space-around;justify-items:stretch;align-items:stretch'><div id=r0></div><div id=r1></div><div id=r2></div></div></body>", 480);
    TEST_ASSERT_EQUAL(3, lot52_node("r0")->y);
    TEST_ASSERT_EQUAL(22, lot52_node("r1")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("r2")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_ac_417(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:56px;grid-template-columns:40px;grid-template-rows:12px 12px 12px;align-content:space-evenly;justify-items:stretch;align-items:stretch'><div id=r0></div><div id=r1></div><div id=r2></div></div></body>", 480);
    TEST_ASSERT_EQUAL(5, lot52_node("r0")->y);
    TEST_ASSERT_EQUAL(22, lot52_node("r1")->y);
    TEST_ASSERT_EQUAL(39, lot52_node("r2")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_ac_418(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:56px;grid-template-columns:40px;grid-template-rows:12px 12px 12px;align-content:start;justify-items:stretch;align-items:stretch'><div id=r0></div><div id=r1></div><div id=r2></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("r0")->y);
    TEST_ASSERT_EQUAL(12, lot52_node("r1")->y);
    TEST_ASSERT_EQUAL(24, lot52_node("r2")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_ac_419(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:56px;grid-template-columns:40px;grid-template-rows:12px 12px 12px;align-content:end;justify-items:stretch;align-items:stretch'><div id=r0></div><div id=r1></div><div id=r2></div></div></body>", 480);
    TEST_ASSERT_EQUAL(20, lot52_node("r0")->y);
    TEST_ASSERT_EQUAL(32, lot52_node("r1")->y);
    TEST_ASSERT_EQUAL(44, lot52_node("r2")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_ac_420(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:56px;grid-template-columns:40px;grid-template-rows:12px 12px 12px;align-content:center;justify-items:stretch;align-items:stretch'><div id=r0></div><div id=r1></div><div id=r2></div></div></body>", 480);
    TEST_ASSERT_EQUAL(10, lot52_node("r0")->y);
    TEST_ASSERT_EQUAL(22, lot52_node("r1")->y);
    TEST_ASSERT_EQUAL(34, lot52_node("r2")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_ac_421(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:66px;grid-template-columns:40px;grid-template-rows:12px 12px 12px;align-content:space-between;justify-items:stretch;align-items:stretch'><div id=r0></div><div id=r1></div><div id=r2></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("r0")->y);
    TEST_ASSERT_EQUAL(27, lot52_node("r1")->y);
    TEST_ASSERT_EQUAL(54, lot52_node("r2")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_ac_422(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:66px;grid-template-columns:40px;grid-template-rows:12px 12px 12px;align-content:space-around;justify-items:stretch;align-items:stretch'><div id=r0></div><div id=r1></div><div id=r2></div></div></body>", 480);
    TEST_ASSERT_EQUAL(5, lot52_node("r0")->y);
    TEST_ASSERT_EQUAL(27, lot52_node("r1")->y);
    TEST_ASSERT_EQUAL(49, lot52_node("r2")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_ac_423(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:66px;grid-template-columns:40px;grid-template-rows:12px 12px 12px;align-content:space-evenly;justify-items:stretch;align-items:stretch'><div id=r0></div><div id=r1></div><div id=r2></div></div></body>", 480);
    TEST_ASSERT_EQUAL(7, lot52_node("r0")->y);
    TEST_ASSERT_EQUAL(27, lot52_node("r1")->y);
    TEST_ASSERT_EQUAL(46, lot52_node("r2")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_ac_424(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:66px;grid-template-columns:40px;grid-template-rows:12px 12px 12px;align-content:start;justify-items:stretch;align-items:stretch'><div id=r0></div><div id=r1></div><div id=r2></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("r0")->y);
    TEST_ASSERT_EQUAL(12, lot52_node("r1")->y);
    TEST_ASSERT_EQUAL(24, lot52_node("r2")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_ac_425(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:66px;grid-template-columns:40px;grid-template-rows:12px 12px 12px;align-content:end;justify-items:stretch;align-items:stretch'><div id=r0></div><div id=r1></div><div id=r2></div></div></body>", 480);
    TEST_ASSERT_EQUAL(30, lot52_node("r0")->y);
    TEST_ASSERT_EQUAL(42, lot52_node("r1")->y);
    TEST_ASSERT_EQUAL(54, lot52_node("r2")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_ac_426(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:66px;grid-template-columns:40px;grid-template-rows:12px 12px 12px;align-content:center;justify-items:stretch;align-items:stretch'><div id=r0></div><div id=r1></div><div id=r2></div></div></body>", 480);
    TEST_ASSERT_EQUAL(15, lot52_node("r0")->y);
    TEST_ASSERT_EQUAL(27, lot52_node("r1")->y);
    TEST_ASSERT_EQUAL(39, lot52_node("r2")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_ac_427(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:76px;grid-template-columns:40px;grid-template-rows:12px 12px 12px;align-content:space-between;justify-items:stretch;align-items:stretch'><div id=r0></div><div id=r1></div><div id=r2></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("r0")->y);
    TEST_ASSERT_EQUAL(32, lot52_node("r1")->y);
    TEST_ASSERT_EQUAL(64, lot52_node("r2")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_ac_428(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:76px;grid-template-columns:40px;grid-template-rows:12px 12px 12px;align-content:space-around;justify-items:stretch;align-items:stretch'><div id=r0></div><div id=r1></div><div id=r2></div></div></body>", 480);
    TEST_ASSERT_EQUAL(6, lot52_node("r0")->y);
    TEST_ASSERT_EQUAL(32, lot52_node("r1")->y);
    TEST_ASSERT_EQUAL(57, lot52_node("r2")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_ac_429(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:76px;grid-template-columns:40px;grid-template-rows:12px 12px 12px;align-content:space-evenly;justify-items:stretch;align-items:stretch'><div id=r0></div><div id=r1></div><div id=r2></div></div></body>", 480);
    TEST_ASSERT_EQUAL(10, lot52_node("r0")->y);
    TEST_ASSERT_EQUAL(32, lot52_node("r1")->y);
    TEST_ASSERT_EQUAL(54, lot52_node("r2")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_ac_430(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:76px;grid-template-columns:40px;grid-template-rows:12px 12px 12px;align-content:start;justify-items:stretch;align-items:stretch'><div id=r0></div><div id=r1></div><div id=r2></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("r0")->y);
    TEST_ASSERT_EQUAL(12, lot52_node("r1")->y);
    TEST_ASSERT_EQUAL(24, lot52_node("r2")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_ac_431(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:76px;grid-template-columns:40px;grid-template-rows:12px 12px 12px;align-content:end;justify-items:stretch;align-items:stretch'><div id=r0></div><div id=r1></div><div id=r2></div></div></body>", 480);
    TEST_ASSERT_EQUAL(40, lot52_node("r0")->y);
    TEST_ASSERT_EQUAL(52, lot52_node("r1")->y);
    TEST_ASSERT_EQUAL(64, lot52_node("r2")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_ac_432(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:76px;grid-template-columns:40px;grid-template-rows:12px 12px 12px;align-content:center;justify-items:stretch;align-items:stretch'><div id=r0></div><div id=r1></div><div id=r2></div></div></body>", 480);
    TEST_ASSERT_EQUAL(20, lot52_node("r0")->y);
    TEST_ASSERT_EQUAL(32, lot52_node("r1")->y);
    TEST_ASSERT_EQUAL(44, lot52_node("r2")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_ac_433(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:32px;grid-template-columns:40px;grid-template-rows:16px 16px;align-content:space-between;justify-items:stretch;align-items:stretch'><div id=r0></div><div id=r1></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("r0")->y);
    TEST_ASSERT_EQUAL(16, lot52_node("r1")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_ac_434(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:32px;grid-template-columns:40px;grid-template-rows:16px 16px;align-content:space-around;justify-items:stretch;align-items:stretch'><div id=r0></div><div id=r1></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("r0")->y);
    TEST_ASSERT_EQUAL(16, lot52_node("r1")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_ac_435(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:32px;grid-template-columns:40px;grid-template-rows:16px 16px;align-content:space-evenly;justify-items:stretch;align-items:stretch'><div id=r0></div><div id=r1></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("r0")->y);
    TEST_ASSERT_EQUAL(16, lot52_node("r1")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_ac_436(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:32px;grid-template-columns:40px;grid-template-rows:16px 16px;align-content:start;justify-items:stretch;align-items:stretch'><div id=r0></div><div id=r1></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("r0")->y);
    TEST_ASSERT_EQUAL(16, lot52_node("r1")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_ac_437(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:32px;grid-template-columns:40px;grid-template-rows:16px 16px;align-content:end;justify-items:stretch;align-items:stretch'><div id=r0></div><div id=r1></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("r0")->y);
    TEST_ASSERT_EQUAL(16, lot52_node("r1")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_ac_438(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:32px;grid-template-columns:40px;grid-template-rows:16px 16px;align-content:center;justify-items:stretch;align-items:stretch'><div id=r0></div><div id=r1></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("r0")->y);
    TEST_ASSERT_EQUAL(16, lot52_node("r1")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_ac_439(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:52px;grid-template-columns:40px;grid-template-rows:16px 16px;align-content:space-between;justify-items:stretch;align-items:stretch'><div id=r0></div><div id=r1></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("r0")->y);
    TEST_ASSERT_EQUAL(36, lot52_node("r1")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_ac_440(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:52px;grid-template-columns:40px;grid-template-rows:16px 16px;align-content:space-around;justify-items:stretch;align-items:stretch'><div id=r0></div><div id=r1></div></div></body>", 480);
    TEST_ASSERT_EQUAL(5, lot52_node("r0")->y);
    TEST_ASSERT_EQUAL(31, lot52_node("r1")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_ac_441(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:52px;grid-template-columns:40px;grid-template-rows:16px 16px;align-content:space-evenly;justify-items:stretch;align-items:stretch'><div id=r0></div><div id=r1></div></div></body>", 480);
    TEST_ASSERT_EQUAL(6, lot52_node("r0")->y);
    TEST_ASSERT_EQUAL(29, lot52_node("r1")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_ac_442(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:52px;grid-template-columns:40px;grid-template-rows:16px 16px;align-content:start;justify-items:stretch;align-items:stretch'><div id=r0></div><div id=r1></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("r0")->y);
    TEST_ASSERT_EQUAL(16, lot52_node("r1")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_ac_443(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:52px;grid-template-columns:40px;grid-template-rows:16px 16px;align-content:end;justify-items:stretch;align-items:stretch'><div id=r0></div><div id=r1></div></div></body>", 480);
    TEST_ASSERT_EQUAL(20, lot52_node("r0")->y);
    TEST_ASSERT_EQUAL(36, lot52_node("r1")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_ac_444(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:52px;grid-template-columns:40px;grid-template-rows:16px 16px;align-content:center;justify-items:stretch;align-items:stretch'><div id=r0></div><div id=r1></div></div></body>", 480);
    TEST_ASSERT_EQUAL(10, lot52_node("r0")->y);
    TEST_ASSERT_EQUAL(26, lot52_node("r1")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_ac_445(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:62px;grid-template-columns:40px;grid-template-rows:16px 16px;align-content:space-between;justify-items:stretch;align-items:stretch'><div id=r0></div><div id=r1></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("r0")->y);
    TEST_ASSERT_EQUAL(46, lot52_node("r1")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_ac_446(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:62px;grid-template-columns:40px;grid-template-rows:16px 16px;align-content:space-around;justify-items:stretch;align-items:stretch'><div id=r0></div><div id=r1></div></div></body>", 480);
    TEST_ASSERT_EQUAL(7, lot52_node("r0")->y);
    TEST_ASSERT_EQUAL(38, lot52_node("r1")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_ac_447(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:62px;grid-template-columns:40px;grid-template-rows:16px 16px;align-content:space-evenly;justify-items:stretch;align-items:stretch'><div id=r0></div><div id=r1></div></div></body>", 480);
    TEST_ASSERT_EQUAL(10, lot52_node("r0")->y);
    TEST_ASSERT_EQUAL(36, lot52_node("r1")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_ac_448(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:62px;grid-template-columns:40px;grid-template-rows:16px 16px;align-content:start;justify-items:stretch;align-items:stretch'><div id=r0></div><div id=r1></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("r0")->y);
    TEST_ASSERT_EQUAL(16, lot52_node("r1")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_ac_449(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:62px;grid-template-columns:40px;grid-template-rows:16px 16px;align-content:end;justify-items:stretch;align-items:stretch'><div id=r0></div><div id=r1></div></div></body>", 480);
    TEST_ASSERT_EQUAL(30, lot52_node("r0")->y);
    TEST_ASSERT_EQUAL(46, lot52_node("r1")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_ac_450(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:62px;grid-template-columns:40px;grid-template-rows:16px 16px;align-content:center;justify-items:stretch;align-items:stretch'><div id=r0></div><div id=r1></div></div></body>", 480);
    TEST_ASSERT_EQUAL(15, lot52_node("r0")->y);
    TEST_ASSERT_EQUAL(31, lot52_node("r1")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_ac_451(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:72px;grid-template-columns:40px;grid-template-rows:16px 16px;align-content:space-between;justify-items:stretch;align-items:stretch'><div id=r0></div><div id=r1></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("r0")->y);
    TEST_ASSERT_EQUAL(56, lot52_node("r1")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_ac_452(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:72px;grid-template-columns:40px;grid-template-rows:16px 16px;align-content:space-around;justify-items:stretch;align-items:stretch'><div id=r0></div><div id=r1></div></div></body>", 480);
    TEST_ASSERT_EQUAL(10, lot52_node("r0")->y);
    TEST_ASSERT_EQUAL(46, lot52_node("r1")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_ac_453(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:72px;grid-template-columns:40px;grid-template-rows:16px 16px;align-content:space-evenly;justify-items:stretch;align-items:stretch'><div id=r0></div><div id=r1></div></div></body>", 480);
    TEST_ASSERT_EQUAL(13, lot52_node("r0")->y);
    TEST_ASSERT_EQUAL(42, lot52_node("r1")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_ac_454(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:72px;grid-template-columns:40px;grid-template-rows:16px 16px;align-content:start;justify-items:stretch;align-items:stretch'><div id=r0></div><div id=r1></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("r0")->y);
    TEST_ASSERT_EQUAL(16, lot52_node("r1")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_ac_455(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:72px;grid-template-columns:40px;grid-template-rows:16px 16px;align-content:end;justify-items:stretch;align-items:stretch'><div id=r0></div><div id=r1></div></div></body>", 480);
    TEST_ASSERT_EQUAL(40, lot52_node("r0")->y);
    TEST_ASSERT_EQUAL(56, lot52_node("r1")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_ac_456(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:72px;grid-template-columns:40px;grid-template-rows:16px 16px;align-content:center;justify-items:stretch;align-items:stretch'><div id=r0></div><div id=r1></div></div></body>", 480);
    TEST_ASSERT_EQUAL(20, lot52_node("r0")->y);
    TEST_ASSERT_EQUAL(36, lot52_node("r1")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_ac_457(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:48px;grid-template-columns:40px;grid-template-rows:16px 16px 16px;align-content:space-between;justify-items:stretch;align-items:stretch'><div id=r0></div><div id=r1></div><div id=r2></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("r0")->y);
    TEST_ASSERT_EQUAL(16, lot52_node("r1")->y);
    TEST_ASSERT_EQUAL(32, lot52_node("r2")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_ac_458(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:48px;grid-template-columns:40px;grid-template-rows:16px 16px 16px;align-content:space-around;justify-items:stretch;align-items:stretch'><div id=r0></div><div id=r1></div><div id=r2></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("r0")->y);
    TEST_ASSERT_EQUAL(16, lot52_node("r1")->y);
    TEST_ASSERT_EQUAL(32, lot52_node("r2")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_ac_459(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:48px;grid-template-columns:40px;grid-template-rows:16px 16px 16px;align-content:space-evenly;justify-items:stretch;align-items:stretch'><div id=r0></div><div id=r1></div><div id=r2></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("r0")->y);
    TEST_ASSERT_EQUAL(16, lot52_node("r1")->y);
    TEST_ASSERT_EQUAL(32, lot52_node("r2")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_ac_460(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:48px;grid-template-columns:40px;grid-template-rows:16px 16px 16px;align-content:start;justify-items:stretch;align-items:stretch'><div id=r0></div><div id=r1></div><div id=r2></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("r0")->y);
    TEST_ASSERT_EQUAL(16, lot52_node("r1")->y);
    TEST_ASSERT_EQUAL(32, lot52_node("r2")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_ac_461(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:48px;grid-template-columns:40px;grid-template-rows:16px 16px 16px;align-content:end;justify-items:stretch;align-items:stretch'><div id=r0></div><div id=r1></div><div id=r2></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("r0")->y);
    TEST_ASSERT_EQUAL(16, lot52_node("r1")->y);
    TEST_ASSERT_EQUAL(32, lot52_node("r2")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_ac_462(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:48px;grid-template-columns:40px;grid-template-rows:16px 16px 16px;align-content:center;justify-items:stretch;align-items:stretch'><div id=r0></div><div id=r1></div><div id=r2></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("r0")->y);
    TEST_ASSERT_EQUAL(16, lot52_node("r1")->y);
    TEST_ASSERT_EQUAL(32, lot52_node("r2")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_ac_463(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:68px;grid-template-columns:40px;grid-template-rows:16px 16px 16px;align-content:space-between;justify-items:stretch;align-items:stretch'><div id=r0></div><div id=r1></div><div id=r2></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("r0")->y);
    TEST_ASSERT_EQUAL(26, lot52_node("r1")->y);
    TEST_ASSERT_EQUAL(52, lot52_node("r2")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_ac_464(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:68px;grid-template-columns:40px;grid-template-rows:16px 16px 16px;align-content:space-around;justify-items:stretch;align-items:stretch'><div id=r0></div><div id=r1></div><div id=r2></div></div></body>", 480);
    TEST_ASSERT_EQUAL(3, lot52_node("r0")->y);
    TEST_ASSERT_EQUAL(26, lot52_node("r1")->y);
    TEST_ASSERT_EQUAL(48, lot52_node("r2")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_ac_465(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:68px;grid-template-columns:40px;grid-template-rows:16px 16px 16px;align-content:space-evenly;justify-items:stretch;align-items:stretch'><div id=r0></div><div id=r1></div><div id=r2></div></div></body>", 480);
    TEST_ASSERT_EQUAL(5, lot52_node("r0")->y);
    TEST_ASSERT_EQUAL(26, lot52_node("r1")->y);
    TEST_ASSERT_EQUAL(47, lot52_node("r2")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_ac_466(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:68px;grid-template-columns:40px;grid-template-rows:16px 16px 16px;align-content:start;justify-items:stretch;align-items:stretch'><div id=r0></div><div id=r1></div><div id=r2></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("r0")->y);
    TEST_ASSERT_EQUAL(16, lot52_node("r1")->y);
    TEST_ASSERT_EQUAL(32, lot52_node("r2")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_ac_467(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:68px;grid-template-columns:40px;grid-template-rows:16px 16px 16px;align-content:end;justify-items:stretch;align-items:stretch'><div id=r0></div><div id=r1></div><div id=r2></div></div></body>", 480);
    TEST_ASSERT_EQUAL(20, lot52_node("r0")->y);
    TEST_ASSERT_EQUAL(36, lot52_node("r1")->y);
    TEST_ASSERT_EQUAL(52, lot52_node("r2")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_ac_468(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:68px;grid-template-columns:40px;grid-template-rows:16px 16px 16px;align-content:center;justify-items:stretch;align-items:stretch'><div id=r0></div><div id=r1></div><div id=r2></div></div></body>", 480);
    TEST_ASSERT_EQUAL(10, lot52_node("r0")->y);
    TEST_ASSERT_EQUAL(26, lot52_node("r1")->y);
    TEST_ASSERT_EQUAL(42, lot52_node("r2")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_ac_469(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:78px;grid-template-columns:40px;grid-template-rows:16px 16px 16px;align-content:space-between;justify-items:stretch;align-items:stretch'><div id=r0></div><div id=r1></div><div id=r2></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("r0")->y);
    TEST_ASSERT_EQUAL(31, lot52_node("r1")->y);
    TEST_ASSERT_EQUAL(62, lot52_node("r2")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_ac_470(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:78px;grid-template-columns:40px;grid-template-rows:16px 16px 16px;align-content:space-around;justify-items:stretch;align-items:stretch'><div id=r0></div><div id=r1></div><div id=r2></div></div></body>", 480);
    TEST_ASSERT_EQUAL(5, lot52_node("r0")->y);
    TEST_ASSERT_EQUAL(31, lot52_node("r1")->y);
    TEST_ASSERT_EQUAL(57, lot52_node("r2")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_ac_471(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:78px;grid-template-columns:40px;grid-template-rows:16px 16px 16px;align-content:space-evenly;justify-items:stretch;align-items:stretch'><div id=r0></div><div id=r1></div><div id=r2></div></div></body>", 480);
    TEST_ASSERT_EQUAL(7, lot52_node("r0")->y);
    TEST_ASSERT_EQUAL(31, lot52_node("r1")->y);
    TEST_ASSERT_EQUAL(54, lot52_node("r2")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_ac_472(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:78px;grid-template-columns:40px;grid-template-rows:16px 16px 16px;align-content:start;justify-items:stretch;align-items:stretch'><div id=r0></div><div id=r1></div><div id=r2></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("r0")->y);
    TEST_ASSERT_EQUAL(16, lot52_node("r1")->y);
    TEST_ASSERT_EQUAL(32, lot52_node("r2")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_ac_473(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:78px;grid-template-columns:40px;grid-template-rows:16px 16px 16px;align-content:end;justify-items:stretch;align-items:stretch'><div id=r0></div><div id=r1></div><div id=r2></div></div></body>", 480);
    TEST_ASSERT_EQUAL(30, lot52_node("r0")->y);
    TEST_ASSERT_EQUAL(46, lot52_node("r1")->y);
    TEST_ASSERT_EQUAL(62, lot52_node("r2")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_ac_474(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:78px;grid-template-columns:40px;grid-template-rows:16px 16px 16px;align-content:center;justify-items:stretch;align-items:stretch'><div id=r0></div><div id=r1></div><div id=r2></div></div></body>", 480);
    TEST_ASSERT_EQUAL(15, lot52_node("r0")->y);
    TEST_ASSERT_EQUAL(31, lot52_node("r1")->y);
    TEST_ASSERT_EQUAL(47, lot52_node("r2")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_ac_475(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:88px;grid-template-columns:40px;grid-template-rows:16px 16px 16px;align-content:space-between;justify-items:stretch;align-items:stretch'><div id=r0></div><div id=r1></div><div id=r2></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("r0")->y);
    TEST_ASSERT_EQUAL(36, lot52_node("r1")->y);
    TEST_ASSERT_EQUAL(72, lot52_node("r2")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_ac_476(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:88px;grid-template-columns:40px;grid-template-rows:16px 16px 16px;align-content:space-around;justify-items:stretch;align-items:stretch'><div id=r0></div><div id=r1></div><div id=r2></div></div></body>", 480);
    TEST_ASSERT_EQUAL(6, lot52_node("r0")->y);
    TEST_ASSERT_EQUAL(36, lot52_node("r1")->y);
    TEST_ASSERT_EQUAL(65, lot52_node("r2")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_ac_477(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:88px;grid-template-columns:40px;grid-template-rows:16px 16px 16px;align-content:space-evenly;justify-items:stretch;align-items:stretch'><div id=r0></div><div id=r1></div><div id=r2></div></div></body>", 480);
    TEST_ASSERT_EQUAL(10, lot52_node("r0")->y);
    TEST_ASSERT_EQUAL(36, lot52_node("r1")->y);
    TEST_ASSERT_EQUAL(62, lot52_node("r2")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_ac_478(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:88px;grid-template-columns:40px;grid-template-rows:16px 16px 16px;align-content:start;justify-items:stretch;align-items:stretch'><div id=r0></div><div id=r1></div><div id=r2></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("r0")->y);
    TEST_ASSERT_EQUAL(16, lot52_node("r1")->y);
    TEST_ASSERT_EQUAL(32, lot52_node("r2")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_ac_479(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:88px;grid-template-columns:40px;grid-template-rows:16px 16px 16px;align-content:end;justify-items:stretch;align-items:stretch'><div id=r0></div><div id=r1></div><div id=r2></div></div></body>", 480);
    TEST_ASSERT_EQUAL(40, lot52_node("r0")->y);
    TEST_ASSERT_EQUAL(56, lot52_node("r1")->y);
    TEST_ASSERT_EQUAL(72, lot52_node("r2")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_ac_480(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:88px;grid-template-columns:40px;grid-template-rows:16px 16px 16px;align-content:center;justify-items:stretch;align-items:stretch'><div id=r0></div><div id=r1></div><div id=r2></div></div></body>", 480);
    TEST_ASSERT_EQUAL(20, lot52_node("r0")->y);
    TEST_ASSERT_EQUAL(36, lot52_node("r1")->y);
    TEST_ASSERT_EQUAL(52, lot52_node("r2")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_pc_1_20(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:60px;height:40px;grid-template-columns:20px 20px;grid-template-rows:10px 10px;place-content:space-between start;align-items:stretch'><div id=a></div><div id=b></div><div id=c></div><div id=d></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(30, lot52_node("c")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_pc_1_40(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:80px;height:60px;grid-template-columns:20px 20px;grid-template-rows:10px 10px;place-content:space-between start;align-items:stretch'><div id=a></div><div id=b></div><div id=c></div><div id=d></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(50, lot52_node("c")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_pc_1_60(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:100px;height:80px;grid-template-columns:20px 20px;grid-template-rows:10px 10px;place-content:space-between start;align-items:stretch'><div id=a></div><div id=b></div><div id=c></div><div id=d></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(70, lot52_node("c")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_pc_2_20(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:60px;height:40px;grid-template-columns:20px 20px;grid-template-rows:10px 10px;place-content:space-around center;align-items:stretch'><div id=a></div><div id=b></div><div id=c></div><div id=d></div></div></body>", 480);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(5, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(30, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(25, lot52_node("c")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_pc_2_40(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:80px;height:60px;grid-template-columns:20px 20px;grid-template-rows:10px 10px;place-content:space-around center;align-items:stretch'><div id=a></div><div id=b></div><div id=c></div><div id=d></div></div></body>", 480);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(40, lot52_node("c")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_pc_2_60(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:100px;height:80px;grid-template-columns:20px 20px;grid-template-rows:10px 10px;place-content:space-around center;align-items:stretch'><div id=a></div><div id=b></div><div id=c></div><div id=d></div></div></body>", 480);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(15, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(50, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(55, lot52_node("c")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_pc_3_20(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:60px;height:40px;grid-template-columns:20px 20px;grid-template-rows:10px 10px;place-content:space-evenly end;align-items:stretch'><div id=a></div><div id=b></div><div id=c></div><div id=d></div></div></body>", 480);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(6, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(23, lot52_node("c")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_pc_3_40(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:80px;height:60px;grid-template-columns:20px 20px;grid-template-rows:10px 10px;place-content:space-evenly end;align-items:stretch'><div id=a></div><div id=b></div><div id=c></div><div id=d></div></div></body>", 480);
    TEST_ASSERT_EQUAL(40, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(13, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(60, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(36, lot52_node("c")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_pc_3_60(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:100px;height:80px;grid-template-columns:20px 20px;grid-template-rows:10px 10px;place-content:space-evenly end;align-items:stretch'><div id=a></div><div id=b></div><div id=c></div><div id=d></div></div></body>", 480);
    TEST_ASSERT_EQUAL(60, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(80, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(50, lot52_node("c")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_pc_4_20(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:60px;height:40px;grid-template-columns:20px 20px;grid-template-rows:10px 10px;place-content:start space-between;align-items:stretch'><div id=a></div><div id=b></div><div id=c></div><div id=d></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(10, lot52_node("c")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_pc_4_40(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:80px;height:60px;grid-template-columns:20px 20px;grid-template-rows:10px 10px;place-content:start space-between;align-items:stretch'><div id=a></div><div id=b></div><div id=c></div><div id=d></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(60, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(10, lot52_node("c")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_pc_4_60(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:100px;height:80px;grid-template-columns:20px 20px;grid-template-rows:10px 10px;place-content:start space-between;align-items:stretch'><div id=a></div><div id=b></div><div id=c></div><div id=d></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(80, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(10, lot52_node("c")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_pc_5_20(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:60px;height:40px;grid-template-columns:20px 20px;grid-template-rows:10px 10px;place-content:center space-around;align-items:stretch'><div id=a></div><div id=b></div><div id=c></div><div id=d></div></div></body>", 480);
    TEST_ASSERT_EQUAL(5, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(35, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(20, lot52_node("c")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_pc_5_40(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:80px;height:60px;grid-template-columns:20px 20px;grid-template-rows:10px 10px;place-content:center space-around;align-items:stretch'><div id=a></div><div id=b></div><div id=c></div><div id=d></div></div></body>", 480);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(50, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(30, lot52_node("c")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_pc_5_60(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:100px;height:80px;grid-template-columns:20px 20px;grid-template-rows:10px 10px;place-content:center space-around;align-items:stretch'><div id=a></div><div id=b></div><div id=c></div><div id=d></div></div></body>", 480);
    TEST_ASSERT_EQUAL(15, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(65, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(40, lot52_node("c")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_pc_6_20(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:60px;height:40px;grid-template-columns:20px 20px;grid-template-rows:10px 10px;place-content:end space-evenly;align-items:stretch'><div id=a></div><div id=b></div><div id=c></div><div id=d></div></div></body>", 480);
    TEST_ASSERT_EQUAL(6, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(33, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(30, lot52_node("c")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_pc_6_40(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:80px;height:60px;grid-template-columns:20px 20px;grid-template-rows:10px 10px;place-content:end space-evenly;align-items:stretch'><div id=a></div><div id=b></div><div id=c></div><div id=d></div></div></body>", 480);
    TEST_ASSERT_EQUAL(13, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(40, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(46, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(50, lot52_node("c")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_pc_6_60(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:100px;height:80px;grid-template-columns:20px 20px;grid-template-rows:10px 10px;place-content:end space-evenly;align-items:stretch'><div id=a></div><div id=b></div><div id=c></div><div id=d></div></div></body>", 480);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(60, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(60, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(70, lot52_node("c")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_pc_7_20(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:60px;height:40px;grid-template-columns:20px 20px;grid-template-rows:10px 10px;place-content:space-between space-between;align-items:stretch'><div id=a></div><div id=b></div><div id=c></div><div id=d></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(30, lot52_node("c")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_pc_7_40(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:80px;height:60px;grid-template-columns:20px 20px;grid-template-rows:10px 10px;place-content:space-between space-between;align-items:stretch'><div id=a></div><div id=b></div><div id=c></div><div id=d></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(60, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(50, lot52_node("c")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_pc_7_60(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:100px;height:80px;grid-template-columns:20px 20px;grid-template-rows:10px 10px;place-content:space-between space-between;align-items:stretch'><div id=a></div><div id=b></div><div id=c></div><div id=d></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(80, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(70, lot52_node("c")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_pc_8_20(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:60px;height:40px;grid-template-columns:20px 20px;grid-template-rows:10px 10px;place-content:space-around space-around;align-items:stretch'><div id=a></div><div id=b></div><div id=c></div><div id=d></div></div></body>", 480);
    TEST_ASSERT_EQUAL(5, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(5, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(35, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(25, lot52_node("c")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_pc_8_40(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:80px;height:60px;grid-template-columns:20px 20px;grid-template-rows:10px 10px;place-content:space-around space-around;align-items:stretch'><div id=a></div><div id=b></div><div id=c></div><div id=d></div></div></body>", 480);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(50, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(40, lot52_node("c")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_pc_8_60(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:100px;height:80px;grid-template-columns:20px 20px;grid-template-rows:10px 10px;place-content:space-around space-around;align-items:stretch'><div id=a></div><div id=b></div><div id=c></div><div id=d></div></div></body>", 480);
    TEST_ASSERT_EQUAL(15, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(15, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(65, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(55, lot52_node("c")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_pc_9_20(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:60px;height:40px;grid-template-columns:20px 20px;grid-template-rows:10px 10px;place-content:space-evenly space-evenly;align-items:stretch'><div id=a></div><div id=b></div><div id=c></div><div id=d></div></div></body>", 480);
    TEST_ASSERT_EQUAL(6, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(6, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(33, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(23, lot52_node("c")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_pc_9_40(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:80px;height:60px;grid-template-columns:20px 20px;grid-template-rows:10px 10px;place-content:space-evenly space-evenly;align-items:stretch'><div id=a></div><div id=b></div><div id=c></div><div id=d></div></div></body>", 480);
    TEST_ASSERT_EQUAL(13, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(13, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(46, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(36, lot52_node("c")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_pc_9_60(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:100px;height:80px;grid-template-columns:20px 20px;grid-template-rows:10px 10px;place-content:space-evenly space-evenly;align-items:stretch'><div id=a></div><div id=b></div><div id=c></div><div id=d></div></div></body>", 480);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(60, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(50, lot52_node("c")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_pc_10_20(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:60px;height:40px;grid-template-columns:20px 20px;grid-template-rows:10px 10px;place-content:space-between center;align-items:stretch'><div id=a></div><div id=b></div><div id=c></div><div id=d></div></div></body>", 480);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(30, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(30, lot52_node("c")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_pc_10_40(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:80px;height:60px;grid-template-columns:20px 20px;grid-template-rows:10px 10px;place-content:space-between center;align-items:stretch'><div id=a></div><div id=b></div><div id=c></div><div id=d></div></div></body>", 480);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(50, lot52_node("c")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_pc_10_60(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:100px;height:80px;grid-template-columns:20px 20px;grid-template-rows:10px 10px;place-content:space-between center;align-items:stretch'><div id=a></div><div id=b></div><div id=c></div><div id=d></div></div></body>", 480);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(50, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(70, lot52_node("c")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void lot79_space_run(void) {
    RUN_TEST(test_l79_jc_1);
    RUN_TEST(test_l79_jc_2);
    RUN_TEST(test_l79_jc_3);
    RUN_TEST(test_l79_jc_4);
    RUN_TEST(test_l79_jc_5);
    RUN_TEST(test_l79_jc_6);
    RUN_TEST(test_l79_jc_7);
    RUN_TEST(test_l79_jc_8);
    RUN_TEST(test_l79_jc_9);
    RUN_TEST(test_l79_jc_10);
    RUN_TEST(test_l79_jc_11);
    RUN_TEST(test_l79_jc_12);
    RUN_TEST(test_l79_jc_13);
    RUN_TEST(test_l79_jc_14);
    RUN_TEST(test_l79_jc_15);
    RUN_TEST(test_l79_jc_16);
    RUN_TEST(test_l79_jc_17);
    RUN_TEST(test_l79_jc_18);
    RUN_TEST(test_l79_jc_19);
    RUN_TEST(test_l79_jc_20);
    RUN_TEST(test_l79_jc_21);
    RUN_TEST(test_l79_jc_22);
    RUN_TEST(test_l79_jc_23);
    RUN_TEST(test_l79_jc_24);
    RUN_TEST(test_l79_jc_25);
    RUN_TEST(test_l79_jc_26);
    RUN_TEST(test_l79_jc_27);
    RUN_TEST(test_l79_jc_28);
    RUN_TEST(test_l79_jc_29);
    RUN_TEST(test_l79_jc_30);
    RUN_TEST(test_l79_jc_31);
    RUN_TEST(test_l79_jc_32);
    RUN_TEST(test_l79_jc_33);
    RUN_TEST(test_l79_jc_34);
    RUN_TEST(test_l79_jc_35);
    RUN_TEST(test_l79_jc_36);
    RUN_TEST(test_l79_jc_37);
    RUN_TEST(test_l79_jc_38);
    RUN_TEST(test_l79_jc_39);
    RUN_TEST(test_l79_jc_40);
    RUN_TEST(test_l79_jc_41);
    RUN_TEST(test_l79_jc_42);
    RUN_TEST(test_l79_jc_43);
    RUN_TEST(test_l79_jc_44);
    RUN_TEST(test_l79_jc_45);
    RUN_TEST(test_l79_jc_46);
    RUN_TEST(test_l79_jc_47);
    RUN_TEST(test_l79_jc_48);
    RUN_TEST(test_l79_jc_49);
    RUN_TEST(test_l79_jc_50);
    RUN_TEST(test_l79_jc_51);
    RUN_TEST(test_l79_jc_52);
    RUN_TEST(test_l79_jc_53);
    RUN_TEST(test_l79_jc_54);
    RUN_TEST(test_l79_jc_55);
    RUN_TEST(test_l79_jc_56);
    RUN_TEST(test_l79_jc_57);
    RUN_TEST(test_l79_jc_58);
    RUN_TEST(test_l79_jc_59);
    RUN_TEST(test_l79_jc_60);
    RUN_TEST(test_l79_jc_61);
    RUN_TEST(test_l79_jc_62);
    RUN_TEST(test_l79_jc_63);
    RUN_TEST(test_l79_jc_64);
    RUN_TEST(test_l79_jc_65);
    RUN_TEST(test_l79_jc_66);
    RUN_TEST(test_l79_jc_67);
    RUN_TEST(test_l79_jc_68);
    RUN_TEST(test_l79_jc_69);
    RUN_TEST(test_l79_jc_70);
    RUN_TEST(test_l79_jc_71);
    RUN_TEST(test_l79_jc_72);
    RUN_TEST(test_l79_jc_73);
    RUN_TEST(test_l79_jc_74);
    RUN_TEST(test_l79_jc_75);
    RUN_TEST(test_l79_jc_76);
    RUN_TEST(test_l79_jc_77);
    RUN_TEST(test_l79_jc_78);
    RUN_TEST(test_l79_jc_79);
    RUN_TEST(test_l79_jc_80);
    RUN_TEST(test_l79_jc_81);
    RUN_TEST(test_l79_jc_82);
    RUN_TEST(test_l79_jc_83);
    RUN_TEST(test_l79_jc_84);
    RUN_TEST(test_l79_jc_85);
    RUN_TEST(test_l79_jc_86);
    RUN_TEST(test_l79_jc_87);
    RUN_TEST(test_l79_jc_88);
    RUN_TEST(test_l79_jc_89);
    RUN_TEST(test_l79_jc_90);
    RUN_TEST(test_l79_jc_91);
    RUN_TEST(test_l79_jc_92);
    RUN_TEST(test_l79_jc_93);
    RUN_TEST(test_l79_jc_94);
    RUN_TEST(test_l79_jc_95);
    RUN_TEST(test_l79_jc_96);
    RUN_TEST(test_l79_jc_97);
    RUN_TEST(test_l79_jc_98);
    RUN_TEST(test_l79_jc_99);
    RUN_TEST(test_l79_jc_100);
    RUN_TEST(test_l79_jc_101);
    RUN_TEST(test_l79_jc_102);
    RUN_TEST(test_l79_jc_103);
    RUN_TEST(test_l79_jc_104);
    RUN_TEST(test_l79_jc_105);
    RUN_TEST(test_l79_jc_106);
    RUN_TEST(test_l79_jc_107);
    RUN_TEST(test_l79_jc_108);
    RUN_TEST(test_l79_jc_109);
    RUN_TEST(test_l79_jc_110);
    RUN_TEST(test_l79_jc_111);
    RUN_TEST(test_l79_jc_112);
    RUN_TEST(test_l79_jc_113);
    RUN_TEST(test_l79_jc_114);
    RUN_TEST(test_l79_jc_115);
    RUN_TEST(test_l79_jc_116);
    RUN_TEST(test_l79_jc_117);
    RUN_TEST(test_l79_jc_118);
    RUN_TEST(test_l79_jc_119);
    RUN_TEST(test_l79_jc_120);
    RUN_TEST(test_l79_jc_121);
    RUN_TEST(test_l79_jc_122);
    RUN_TEST(test_l79_jc_123);
    RUN_TEST(test_l79_jc_124);
    RUN_TEST(test_l79_jc_125);
    RUN_TEST(test_l79_jc_126);
    RUN_TEST(test_l79_jc_127);
    RUN_TEST(test_l79_jc_128);
    RUN_TEST(test_l79_jc_129);
    RUN_TEST(test_l79_jc_130);
    RUN_TEST(test_l79_jc_131);
    RUN_TEST(test_l79_jc_132);
    RUN_TEST(test_l79_jc_133);
    RUN_TEST(test_l79_jc_134);
    RUN_TEST(test_l79_jc_135);
    RUN_TEST(test_l79_jc_136);
    RUN_TEST(test_l79_jc_137);
    RUN_TEST(test_l79_jc_138);
    RUN_TEST(test_l79_jc_139);
    RUN_TEST(test_l79_jc_140);
    RUN_TEST(test_l79_jc_141);
    RUN_TEST(test_l79_jc_142);
    RUN_TEST(test_l79_jc_143);
    RUN_TEST(test_l79_jc_144);
    RUN_TEST(test_l79_jc_145);
    RUN_TEST(test_l79_jc_146);
    RUN_TEST(test_l79_jc_147);
    RUN_TEST(test_l79_jc_148);
    RUN_TEST(test_l79_jc_149);
    RUN_TEST(test_l79_jc_150);
    RUN_TEST(test_l79_jc_151);
    RUN_TEST(test_l79_jc_152);
    RUN_TEST(test_l79_jc_153);
    RUN_TEST(test_l79_jc_154);
    RUN_TEST(test_l79_jc_155);
    RUN_TEST(test_l79_jc_156);
    RUN_TEST(test_l79_jc_157);
    RUN_TEST(test_l79_jc_158);
    RUN_TEST(test_l79_jc_159);
    RUN_TEST(test_l79_jc_160);
    RUN_TEST(test_l79_jc_161);
    RUN_TEST(test_l79_jc_162);
    RUN_TEST(test_l79_jc_163);
    RUN_TEST(test_l79_jc_164);
    RUN_TEST(test_l79_jc_165);
    RUN_TEST(test_l79_jc_166);
    RUN_TEST(test_l79_jc_167);
    RUN_TEST(test_l79_jc_168);
    RUN_TEST(test_l79_jc_169);
    RUN_TEST(test_l79_jc_170);
    RUN_TEST(test_l79_jc_171);
    RUN_TEST(test_l79_jc_172);
    RUN_TEST(test_l79_jc_173);
    RUN_TEST(test_l79_jc_174);
    RUN_TEST(test_l79_jc_175);
    RUN_TEST(test_l79_jc_176);
    RUN_TEST(test_l79_jc_177);
    RUN_TEST(test_l79_jc_178);
    RUN_TEST(test_l79_jc_179);
    RUN_TEST(test_l79_jc_180);
    RUN_TEST(test_l79_jc_181);
    RUN_TEST(test_l79_jc_182);
    RUN_TEST(test_l79_jc_183);
    RUN_TEST(test_l79_jc_184);
    RUN_TEST(test_l79_jc_185);
    RUN_TEST(test_l79_jc_186);
    RUN_TEST(test_l79_jc_187);
    RUN_TEST(test_l79_jc_188);
    RUN_TEST(test_l79_jc_189);
    RUN_TEST(test_l79_jc_190);
    RUN_TEST(test_l79_jc_191);
    RUN_TEST(test_l79_jc_192);
    RUN_TEST(test_l79_jc_193);
    RUN_TEST(test_l79_jc_194);
    RUN_TEST(test_l79_jc_195);
    RUN_TEST(test_l79_jc_196);
    RUN_TEST(test_l79_jc_197);
    RUN_TEST(test_l79_jc_198);
    RUN_TEST(test_l79_jc_199);
    RUN_TEST(test_l79_jc_200);
    RUN_TEST(test_l79_jc_201);
    RUN_TEST(test_l79_jc_202);
    RUN_TEST(test_l79_jc_203);
    RUN_TEST(test_l79_jc_204);
    RUN_TEST(test_l79_jc_205);
    RUN_TEST(test_l79_jc_206);
    RUN_TEST(test_l79_jc_207);
    RUN_TEST(test_l79_jc_208);
    RUN_TEST(test_l79_jc_209);
    RUN_TEST(test_l79_jc_210);
    RUN_TEST(test_l79_jc_211);
    RUN_TEST(test_l79_jc_212);
    RUN_TEST(test_l79_jc_213);
    RUN_TEST(test_l79_jc_214);
    RUN_TEST(test_l79_jc_215);
    RUN_TEST(test_l79_jc_216);
    RUN_TEST(test_l79_jc_217);
    RUN_TEST(test_l79_jc_218);
    RUN_TEST(test_l79_jc_219);
    RUN_TEST(test_l79_jc_220);
    RUN_TEST(test_l79_jc_221);
    RUN_TEST(test_l79_jc_222);
    RUN_TEST(test_l79_jc_223);
    RUN_TEST(test_l79_jc_224);
    RUN_TEST(test_l79_jc_225);
    RUN_TEST(test_l79_jc_226);
    RUN_TEST(test_l79_jc_227);
    RUN_TEST(test_l79_jc_228);
    RUN_TEST(test_l79_jc_229);
    RUN_TEST(test_l79_jc_230);
    RUN_TEST(test_l79_jc_231);
    RUN_TEST(test_l79_jc_232);
    RUN_TEST(test_l79_jc_233);
    RUN_TEST(test_l79_jc_234);
    RUN_TEST(test_l79_jc_235);
    RUN_TEST(test_l79_jc_236);
    RUN_TEST(test_l79_jc_237);
    RUN_TEST(test_l79_jc_238);
    RUN_TEST(test_l79_jc_239);
    RUN_TEST(test_l79_jc_240);
    RUN_TEST(test_l79_jc_241);
    RUN_TEST(test_l79_jc_242);
    RUN_TEST(test_l79_jc_243);
    RUN_TEST(test_l79_jc_244);
    RUN_TEST(test_l79_jc_245);
    RUN_TEST(test_l79_jc_246);
    RUN_TEST(test_l79_jc_247);
    RUN_TEST(test_l79_jc_248);
    RUN_TEST(test_l79_jc_249);
    RUN_TEST(test_l79_jc_250);
    RUN_TEST(test_l79_jc_251);
    RUN_TEST(test_l79_jc_252);
    RUN_TEST(test_l79_jc_253);
    RUN_TEST(test_l79_jc_254);
    RUN_TEST(test_l79_jc_255);
    RUN_TEST(test_l79_jc_256);
    RUN_TEST(test_l79_jc_257);
    RUN_TEST(test_l79_jc_258);
    RUN_TEST(test_l79_jc_259);
    RUN_TEST(test_l79_jc_260);
    RUN_TEST(test_l79_jc_261);
    RUN_TEST(test_l79_jc_262);
    RUN_TEST(test_l79_jc_263);
    RUN_TEST(test_l79_jc_264);
    RUN_TEST(test_l79_jc_265);
    RUN_TEST(test_l79_jc_266);
    RUN_TEST(test_l79_jc_267);
    RUN_TEST(test_l79_jc_268);
    RUN_TEST(test_l79_jc_269);
    RUN_TEST(test_l79_jc_270);
    RUN_TEST(test_l79_jc_271);
    RUN_TEST(test_l79_jc_272);
    RUN_TEST(test_l79_jc_273);
    RUN_TEST(test_l79_jc_274);
    RUN_TEST(test_l79_jc_275);
    RUN_TEST(test_l79_jc_276);
    RUN_TEST(test_l79_jc_277);
    RUN_TEST(test_l79_jc_278);
    RUN_TEST(test_l79_jc_279);
    RUN_TEST(test_l79_jc_280);
    RUN_TEST(test_l79_jc_281);
    RUN_TEST(test_l79_jc_282);
    RUN_TEST(test_l79_jc_283);
    RUN_TEST(test_l79_jc_284);
    RUN_TEST(test_l79_jc_285);
    RUN_TEST(test_l79_jc_286);
    RUN_TEST(test_l79_jc_287);
    RUN_TEST(test_l79_jc_288);
    RUN_TEST(test_l79_ac_289);
    RUN_TEST(test_l79_ac_290);
    RUN_TEST(test_l79_ac_291);
    RUN_TEST(test_l79_ac_292);
    RUN_TEST(test_l79_ac_293);
    RUN_TEST(test_l79_ac_294);
    RUN_TEST(test_l79_ac_295);
    RUN_TEST(test_l79_ac_296);
    RUN_TEST(test_l79_ac_297);
    RUN_TEST(test_l79_ac_298);
    RUN_TEST(test_l79_ac_299);
    RUN_TEST(test_l79_ac_300);
    RUN_TEST(test_l79_ac_301);
    RUN_TEST(test_l79_ac_302);
    RUN_TEST(test_l79_ac_303);
    RUN_TEST(test_l79_ac_304);
    RUN_TEST(test_l79_ac_305);
    RUN_TEST(test_l79_ac_306);
    RUN_TEST(test_l79_ac_307);
    RUN_TEST(test_l79_ac_308);
    RUN_TEST(test_l79_ac_309);
    RUN_TEST(test_l79_ac_310);
    RUN_TEST(test_l79_ac_311);
    RUN_TEST(test_l79_ac_312);
    RUN_TEST(test_l79_ac_313);
    RUN_TEST(test_l79_ac_314);
    RUN_TEST(test_l79_ac_315);
    RUN_TEST(test_l79_ac_316);
    RUN_TEST(test_l79_ac_317);
    RUN_TEST(test_l79_ac_318);
    RUN_TEST(test_l79_ac_319);
    RUN_TEST(test_l79_ac_320);
    RUN_TEST(test_l79_ac_321);
    RUN_TEST(test_l79_ac_322);
    RUN_TEST(test_l79_ac_323);
    RUN_TEST(test_l79_ac_324);
    RUN_TEST(test_l79_ac_325);
    RUN_TEST(test_l79_ac_326);
    RUN_TEST(test_l79_ac_327);
    RUN_TEST(test_l79_ac_328);
    RUN_TEST(test_l79_ac_329);
    RUN_TEST(test_l79_ac_330);
    RUN_TEST(test_l79_ac_331);
    RUN_TEST(test_l79_ac_332);
    RUN_TEST(test_l79_ac_333);
    RUN_TEST(test_l79_ac_334);
    RUN_TEST(test_l79_ac_335);
    RUN_TEST(test_l79_ac_336);
    RUN_TEST(test_l79_ac_337);
    RUN_TEST(test_l79_ac_338);
    RUN_TEST(test_l79_ac_339);
    RUN_TEST(test_l79_ac_340);
    RUN_TEST(test_l79_ac_341);
    RUN_TEST(test_l79_ac_342);
    RUN_TEST(test_l79_ac_343);
    RUN_TEST(test_l79_ac_344);
    RUN_TEST(test_l79_ac_345);
    RUN_TEST(test_l79_ac_346);
    RUN_TEST(test_l79_ac_347);
    RUN_TEST(test_l79_ac_348);
    RUN_TEST(test_l79_ac_349);
    RUN_TEST(test_l79_ac_350);
    RUN_TEST(test_l79_ac_351);
    RUN_TEST(test_l79_ac_352);
    RUN_TEST(test_l79_ac_353);
    RUN_TEST(test_l79_ac_354);
    RUN_TEST(test_l79_ac_355);
    RUN_TEST(test_l79_ac_356);
    RUN_TEST(test_l79_ac_357);
    RUN_TEST(test_l79_ac_358);
    RUN_TEST(test_l79_ac_359);
    RUN_TEST(test_l79_ac_360);
    RUN_TEST(test_l79_ac_361);
    RUN_TEST(test_l79_ac_362);
    RUN_TEST(test_l79_ac_363);
    RUN_TEST(test_l79_ac_364);
    RUN_TEST(test_l79_ac_365);
    RUN_TEST(test_l79_ac_366);
    RUN_TEST(test_l79_ac_367);
    RUN_TEST(test_l79_ac_368);
    RUN_TEST(test_l79_ac_369);
    RUN_TEST(test_l79_ac_370);
    RUN_TEST(test_l79_ac_371);
    RUN_TEST(test_l79_ac_372);
    RUN_TEST(test_l79_ac_373);
    RUN_TEST(test_l79_ac_374);
    RUN_TEST(test_l79_ac_375);
    RUN_TEST(test_l79_ac_376);
    RUN_TEST(test_l79_ac_377);
    RUN_TEST(test_l79_ac_378);
    RUN_TEST(test_l79_ac_379);
    RUN_TEST(test_l79_ac_380);
    RUN_TEST(test_l79_ac_381);
    RUN_TEST(test_l79_ac_382);
    RUN_TEST(test_l79_ac_383);
    RUN_TEST(test_l79_ac_384);
    RUN_TEST(test_l79_ac_385);
    RUN_TEST(test_l79_ac_386);
    RUN_TEST(test_l79_ac_387);
    RUN_TEST(test_l79_ac_388);
    RUN_TEST(test_l79_ac_389);
    RUN_TEST(test_l79_ac_390);
    RUN_TEST(test_l79_ac_391);
    RUN_TEST(test_l79_ac_392);
    RUN_TEST(test_l79_ac_393);
    RUN_TEST(test_l79_ac_394);
    RUN_TEST(test_l79_ac_395);
    RUN_TEST(test_l79_ac_396);
    RUN_TEST(test_l79_ac_397);
    RUN_TEST(test_l79_ac_398);
    RUN_TEST(test_l79_ac_399);
    RUN_TEST(test_l79_ac_400);
    RUN_TEST(test_l79_ac_401);
    RUN_TEST(test_l79_ac_402);
    RUN_TEST(test_l79_ac_403);
    RUN_TEST(test_l79_ac_404);
    RUN_TEST(test_l79_ac_405);
    RUN_TEST(test_l79_ac_406);
    RUN_TEST(test_l79_ac_407);
    RUN_TEST(test_l79_ac_408);
    RUN_TEST(test_l79_ac_409);
    RUN_TEST(test_l79_ac_410);
    RUN_TEST(test_l79_ac_411);
    RUN_TEST(test_l79_ac_412);
    RUN_TEST(test_l79_ac_413);
    RUN_TEST(test_l79_ac_414);
    RUN_TEST(test_l79_ac_415);
    RUN_TEST(test_l79_ac_416);
    RUN_TEST(test_l79_ac_417);
    RUN_TEST(test_l79_ac_418);
    RUN_TEST(test_l79_ac_419);
    RUN_TEST(test_l79_ac_420);
    RUN_TEST(test_l79_ac_421);
    RUN_TEST(test_l79_ac_422);
    RUN_TEST(test_l79_ac_423);
    RUN_TEST(test_l79_ac_424);
    RUN_TEST(test_l79_ac_425);
    RUN_TEST(test_l79_ac_426);
    RUN_TEST(test_l79_ac_427);
    RUN_TEST(test_l79_ac_428);
    RUN_TEST(test_l79_ac_429);
    RUN_TEST(test_l79_ac_430);
    RUN_TEST(test_l79_ac_431);
    RUN_TEST(test_l79_ac_432);
    RUN_TEST(test_l79_ac_433);
    RUN_TEST(test_l79_ac_434);
    RUN_TEST(test_l79_ac_435);
    RUN_TEST(test_l79_ac_436);
    RUN_TEST(test_l79_ac_437);
    RUN_TEST(test_l79_ac_438);
    RUN_TEST(test_l79_ac_439);
    RUN_TEST(test_l79_ac_440);
    RUN_TEST(test_l79_ac_441);
    RUN_TEST(test_l79_ac_442);
    RUN_TEST(test_l79_ac_443);
    RUN_TEST(test_l79_ac_444);
    RUN_TEST(test_l79_ac_445);
    RUN_TEST(test_l79_ac_446);
    RUN_TEST(test_l79_ac_447);
    RUN_TEST(test_l79_ac_448);
    RUN_TEST(test_l79_ac_449);
    RUN_TEST(test_l79_ac_450);
    RUN_TEST(test_l79_ac_451);
    RUN_TEST(test_l79_ac_452);
    RUN_TEST(test_l79_ac_453);
    RUN_TEST(test_l79_ac_454);
    RUN_TEST(test_l79_ac_455);
    RUN_TEST(test_l79_ac_456);
    RUN_TEST(test_l79_ac_457);
    RUN_TEST(test_l79_ac_458);
    RUN_TEST(test_l79_ac_459);
    RUN_TEST(test_l79_ac_460);
    RUN_TEST(test_l79_ac_461);
    RUN_TEST(test_l79_ac_462);
    RUN_TEST(test_l79_ac_463);
    RUN_TEST(test_l79_ac_464);
    RUN_TEST(test_l79_ac_465);
    RUN_TEST(test_l79_ac_466);
    RUN_TEST(test_l79_ac_467);
    RUN_TEST(test_l79_ac_468);
    RUN_TEST(test_l79_ac_469);
    RUN_TEST(test_l79_ac_470);
    RUN_TEST(test_l79_ac_471);
    RUN_TEST(test_l79_ac_472);
    RUN_TEST(test_l79_ac_473);
    RUN_TEST(test_l79_ac_474);
    RUN_TEST(test_l79_ac_475);
    RUN_TEST(test_l79_ac_476);
    RUN_TEST(test_l79_ac_477);
    RUN_TEST(test_l79_ac_478);
    RUN_TEST(test_l79_ac_479);
    RUN_TEST(test_l79_ac_480);
    RUN_TEST(test_l79_pc_1_20);
    RUN_TEST(test_l79_pc_1_40);
    RUN_TEST(test_l79_pc_1_60);
    RUN_TEST(test_l79_pc_2_20);
    RUN_TEST(test_l79_pc_2_40);
    RUN_TEST(test_l79_pc_2_60);
    RUN_TEST(test_l79_pc_3_20);
    RUN_TEST(test_l79_pc_3_40);
    RUN_TEST(test_l79_pc_3_60);
    RUN_TEST(test_l79_pc_4_20);
    RUN_TEST(test_l79_pc_4_40);
    RUN_TEST(test_l79_pc_4_60);
    RUN_TEST(test_l79_pc_5_20);
    RUN_TEST(test_l79_pc_5_40);
    RUN_TEST(test_l79_pc_5_60);
    RUN_TEST(test_l79_pc_6_20);
    RUN_TEST(test_l79_pc_6_40);
    RUN_TEST(test_l79_pc_6_60);
    RUN_TEST(test_l79_pc_7_20);
    RUN_TEST(test_l79_pc_7_40);
    RUN_TEST(test_l79_pc_7_60);
    RUN_TEST(test_l79_pc_8_20);
    RUN_TEST(test_l79_pc_8_40);
    RUN_TEST(test_l79_pc_8_60);
    RUN_TEST(test_l79_pc_9_20);
    RUN_TEST(test_l79_pc_9_40);
    RUN_TEST(test_l79_pc_9_60);
    RUN_TEST(test_l79_pc_10_20);
    RUN_TEST(test_l79_pc_10_40);
    RUN_TEST(test_l79_pc_10_60);
}
