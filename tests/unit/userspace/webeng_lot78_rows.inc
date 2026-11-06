/* Lot 78: systematic grid-template-rows, grid-row span/lines. */

static void test_l78_rows_px_1(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:44px;grid-template-rows:repeat(3,14px);row-gap:1px;align-items:stretch'><div id=r1_0></div><div id=r1_1></div><div id=r1_2></div></div></body>", 480);
    TEST_ASSERT_EQUAL(3, lot52_node("g")->grid_rows);
    TEST_ASSERT_EQUAL(14, lot52_node("r1_0")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("r1_0")->y);
    TEST_ASSERT_EQUAL(14, lot52_node("r1_1")->h);
    TEST_ASSERT_EQUAL(15, lot52_node("r1_1")->y);
    TEST_ASSERT_EQUAL(14, lot52_node("r1_2")->h);
    TEST_ASSERT_EQUAL(30, lot52_node("r1_2")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_rows_px_2(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:70px;grid-template-rows:repeat(4,16px);row-gap:2px;align-items:stretch'><div id=r2_0></div><div id=r2_1></div><div id=r2_2></div><div id=r2_3></div></div></body>", 480);
    TEST_ASSERT_EQUAL(4, lot52_node("g")->grid_rows);
    TEST_ASSERT_EQUAL(16, lot52_node("r2_0")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("r2_0")->y);
    TEST_ASSERT_EQUAL(16, lot52_node("r2_1")->h);
    TEST_ASSERT_EQUAL(18, lot52_node("r2_1")->y);
    TEST_ASSERT_EQUAL(16, lot52_node("r2_2")->h);
    TEST_ASSERT_EQUAL(36, lot52_node("r2_2")->y);
    TEST_ASSERT_EQUAL(16, lot52_node("r2_3")->h);
    TEST_ASSERT_EQUAL(54, lot52_node("r2_3")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_rows_px_3(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:39px;grid-template-rows:repeat(2,18px);row-gap:3px;align-items:stretch'><div id=r3_0></div><div id=r3_1></div></div></body>", 480);
    TEST_ASSERT_EQUAL(2, lot52_node("g")->grid_rows);
    TEST_ASSERT_EQUAL(18, lot52_node("r3_0")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("r3_0")->y);
    TEST_ASSERT_EQUAL(18, lot52_node("r3_1")->h);
    TEST_ASSERT_EQUAL(21, lot52_node("r3_1")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_rows_px_4(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:68px;grid-template-rows:repeat(3,20px);row-gap:4px;align-items:stretch'><div id=r4_0></div><div id=r4_1></div><div id=r4_2></div></div></body>", 480);
    TEST_ASSERT_EQUAL(3, lot52_node("g")->grid_rows);
    TEST_ASSERT_EQUAL(20, lot52_node("r4_0")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("r4_0")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("r4_1")->h);
    TEST_ASSERT_EQUAL(24, lot52_node("r4_1")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("r4_2")->h);
    TEST_ASSERT_EQUAL(48, lot52_node("r4_2")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_rows_px_5(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:88px;grid-template-rows:repeat(4,22px);row-gap:0px;align-items:stretch'><div id=r5_0></div><div id=r5_1></div><div id=r5_2></div><div id=r5_3></div></div></body>", 480);
    TEST_ASSERT_EQUAL(4, lot52_node("g")->grid_rows);
    TEST_ASSERT_EQUAL(22, lot52_node("r5_0")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("r5_0")->y);
    TEST_ASSERT_EQUAL(22, lot52_node("r5_1")->h);
    TEST_ASSERT_EQUAL(22, lot52_node("r5_1")->y);
    TEST_ASSERT_EQUAL(22, lot52_node("r5_2")->h);
    TEST_ASSERT_EQUAL(44, lot52_node("r5_2")->y);
    TEST_ASSERT_EQUAL(22, lot52_node("r5_3")->h);
    TEST_ASSERT_EQUAL(66, lot52_node("r5_3")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_rows_px_6(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:25px;grid-template-rows:repeat(2,12px);row-gap:1px;align-items:stretch'><div id=r6_0></div><div id=r6_1></div></div></body>", 480);
    TEST_ASSERT_EQUAL(2, lot52_node("g")->grid_rows);
    TEST_ASSERT_EQUAL(12, lot52_node("r6_0")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("r6_0")->y);
    TEST_ASSERT_EQUAL(12, lot52_node("r6_1")->h);
    TEST_ASSERT_EQUAL(13, lot52_node("r6_1")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_rows_px_7(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:46px;grid-template-rows:repeat(3,14px);row-gap:2px;align-items:stretch'><div id=r7_0></div><div id=r7_1></div><div id=r7_2></div></div></body>", 480);
    TEST_ASSERT_EQUAL(3, lot52_node("g")->grid_rows);
    TEST_ASSERT_EQUAL(14, lot52_node("r7_0")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("r7_0")->y);
    TEST_ASSERT_EQUAL(14, lot52_node("r7_1")->h);
    TEST_ASSERT_EQUAL(16, lot52_node("r7_1")->y);
    TEST_ASSERT_EQUAL(14, lot52_node("r7_2")->h);
    TEST_ASSERT_EQUAL(32, lot52_node("r7_2")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_rows_px_8(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:73px;grid-template-rows:repeat(4,16px);row-gap:3px;align-items:stretch'><div id=r8_0></div><div id=r8_1></div><div id=r8_2></div><div id=r8_3></div></div></body>", 480);
    TEST_ASSERT_EQUAL(4, lot52_node("g")->grid_rows);
    TEST_ASSERT_EQUAL(16, lot52_node("r8_0")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("r8_0")->y);
    TEST_ASSERT_EQUAL(16, lot52_node("r8_1")->h);
    TEST_ASSERT_EQUAL(19, lot52_node("r8_1")->y);
    TEST_ASSERT_EQUAL(16, lot52_node("r8_2")->h);
    TEST_ASSERT_EQUAL(38, lot52_node("r8_2")->y);
    TEST_ASSERT_EQUAL(16, lot52_node("r8_3")->h);
    TEST_ASSERT_EQUAL(57, lot52_node("r8_3")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_rows_px_9(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:40px;grid-template-rows:repeat(2,18px);row-gap:4px;align-items:stretch'><div id=r9_0></div><div id=r9_1></div></div></body>", 480);
    TEST_ASSERT_EQUAL(2, lot52_node("g")->grid_rows);
    TEST_ASSERT_EQUAL(18, lot52_node("r9_0")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("r9_0")->y);
    TEST_ASSERT_EQUAL(18, lot52_node("r9_1")->h);
    TEST_ASSERT_EQUAL(22, lot52_node("r9_1")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_rows_px_10(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:60px;grid-template-rows:repeat(3,20px);row-gap:0px;align-items:stretch'><div id=r10_0></div><div id=r10_1></div><div id=r10_2></div></div></body>", 480);
    TEST_ASSERT_EQUAL(3, lot52_node("g")->grid_rows);
    TEST_ASSERT_EQUAL(20, lot52_node("r10_0")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("r10_0")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("r10_1")->h);
    TEST_ASSERT_EQUAL(20, lot52_node("r10_1")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("r10_2")->h);
    TEST_ASSERT_EQUAL(40, lot52_node("r10_2")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_rows_px_11(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:91px;grid-template-rows:repeat(4,22px);row-gap:1px;align-items:stretch'><div id=r11_0></div><div id=r11_1></div><div id=r11_2></div><div id=r11_3></div></div></body>", 480);
    TEST_ASSERT_EQUAL(4, lot52_node("g")->grid_rows);
    TEST_ASSERT_EQUAL(22, lot52_node("r11_0")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("r11_0")->y);
    TEST_ASSERT_EQUAL(22, lot52_node("r11_1")->h);
    TEST_ASSERT_EQUAL(23, lot52_node("r11_1")->y);
    TEST_ASSERT_EQUAL(22, lot52_node("r11_2")->h);
    TEST_ASSERT_EQUAL(46, lot52_node("r11_2")->y);
    TEST_ASSERT_EQUAL(22, lot52_node("r11_3")->h);
    TEST_ASSERT_EQUAL(69, lot52_node("r11_3")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_rows_px_12(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:26px;grid-template-rows:repeat(2,12px);row-gap:2px;align-items:stretch'><div id=r12_0></div><div id=r12_1></div></div></body>", 480);
    TEST_ASSERT_EQUAL(2, lot52_node("g")->grid_rows);
    TEST_ASSERT_EQUAL(12, lot52_node("r12_0")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("r12_0")->y);
    TEST_ASSERT_EQUAL(12, lot52_node("r12_1")->h);
    TEST_ASSERT_EQUAL(14, lot52_node("r12_1")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_rows_px_13(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:48px;grid-template-rows:repeat(3,14px);row-gap:3px;align-items:stretch'><div id=r13_0></div><div id=r13_1></div><div id=r13_2></div></div></body>", 480);
    TEST_ASSERT_EQUAL(3, lot52_node("g")->grid_rows);
    TEST_ASSERT_EQUAL(14, lot52_node("r13_0")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("r13_0")->y);
    TEST_ASSERT_EQUAL(14, lot52_node("r13_1")->h);
    TEST_ASSERT_EQUAL(17, lot52_node("r13_1")->y);
    TEST_ASSERT_EQUAL(14, lot52_node("r13_2")->h);
    TEST_ASSERT_EQUAL(34, lot52_node("r13_2")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_rows_px_14(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:76px;grid-template-rows:repeat(4,16px);row-gap:4px;align-items:stretch'><div id=r14_0></div><div id=r14_1></div><div id=r14_2></div><div id=r14_3></div></div></body>", 480);
    TEST_ASSERT_EQUAL(4, lot52_node("g")->grid_rows);
    TEST_ASSERT_EQUAL(16, lot52_node("r14_0")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("r14_0")->y);
    TEST_ASSERT_EQUAL(16, lot52_node("r14_1")->h);
    TEST_ASSERT_EQUAL(20, lot52_node("r14_1")->y);
    TEST_ASSERT_EQUAL(16, lot52_node("r14_2")->h);
    TEST_ASSERT_EQUAL(40, lot52_node("r14_2")->y);
    TEST_ASSERT_EQUAL(16, lot52_node("r14_3")->h);
    TEST_ASSERT_EQUAL(60, lot52_node("r14_3")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_rows_px_15(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:36px;grid-template-rows:repeat(2,18px);row-gap:0px;align-items:stretch'><div id=r15_0></div><div id=r15_1></div></div></body>", 480);
    TEST_ASSERT_EQUAL(2, lot52_node("g")->grid_rows);
    TEST_ASSERT_EQUAL(18, lot52_node("r15_0")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("r15_0")->y);
    TEST_ASSERT_EQUAL(18, lot52_node("r15_1")->h);
    TEST_ASSERT_EQUAL(18, lot52_node("r15_1")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_rows_px_16(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:62px;grid-template-rows:repeat(3,20px);row-gap:1px;align-items:stretch'><div id=r16_0></div><div id=r16_1></div><div id=r16_2></div></div></body>", 480);
    TEST_ASSERT_EQUAL(3, lot52_node("g")->grid_rows);
    TEST_ASSERT_EQUAL(20, lot52_node("r16_0")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("r16_0")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("r16_1")->h);
    TEST_ASSERT_EQUAL(21, lot52_node("r16_1")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("r16_2")->h);
    TEST_ASSERT_EQUAL(42, lot52_node("r16_2")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_rows_px_17(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:94px;grid-template-rows:repeat(4,22px);row-gap:2px;align-items:stretch'><div id=r17_0></div><div id=r17_1></div><div id=r17_2></div><div id=r17_3></div></div></body>", 480);
    TEST_ASSERT_EQUAL(4, lot52_node("g")->grid_rows);
    TEST_ASSERT_EQUAL(22, lot52_node("r17_0")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("r17_0")->y);
    TEST_ASSERT_EQUAL(22, lot52_node("r17_1")->h);
    TEST_ASSERT_EQUAL(24, lot52_node("r17_1")->y);
    TEST_ASSERT_EQUAL(22, lot52_node("r17_2")->h);
    TEST_ASSERT_EQUAL(48, lot52_node("r17_2")->y);
    TEST_ASSERT_EQUAL(22, lot52_node("r17_3")->h);
    TEST_ASSERT_EQUAL(72, lot52_node("r17_3")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_rows_px_18(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:27px;grid-template-rows:repeat(2,12px);row-gap:3px;align-items:stretch'><div id=r18_0></div><div id=r18_1></div></div></body>", 480);
    TEST_ASSERT_EQUAL(2, lot52_node("g")->grid_rows);
    TEST_ASSERT_EQUAL(12, lot52_node("r18_0")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("r18_0")->y);
    TEST_ASSERT_EQUAL(12, lot52_node("r18_1")->h);
    TEST_ASSERT_EQUAL(15, lot52_node("r18_1")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_rows_px_19(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:50px;grid-template-rows:repeat(3,14px);row-gap:4px;align-items:stretch'><div id=r19_0></div><div id=r19_1></div><div id=r19_2></div></div></body>", 480);
    TEST_ASSERT_EQUAL(3, lot52_node("g")->grid_rows);
    TEST_ASSERT_EQUAL(14, lot52_node("r19_0")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("r19_0")->y);
    TEST_ASSERT_EQUAL(14, lot52_node("r19_1")->h);
    TEST_ASSERT_EQUAL(18, lot52_node("r19_1")->y);
    TEST_ASSERT_EQUAL(14, lot52_node("r19_2")->h);
    TEST_ASSERT_EQUAL(36, lot52_node("r19_2")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_rows_px_20(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:64px;grid-template-rows:repeat(4,16px);row-gap:0px;align-items:stretch'><div id=r20_0></div><div id=r20_1></div><div id=r20_2></div><div id=r20_3></div></div></body>", 480);
    TEST_ASSERT_EQUAL(4, lot52_node("g")->grid_rows);
    TEST_ASSERT_EQUAL(16, lot52_node("r20_0")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("r20_0")->y);
    TEST_ASSERT_EQUAL(16, lot52_node("r20_1")->h);
    TEST_ASSERT_EQUAL(16, lot52_node("r20_1")->y);
    TEST_ASSERT_EQUAL(16, lot52_node("r20_2")->h);
    TEST_ASSERT_EQUAL(32, lot52_node("r20_2")->y);
    TEST_ASSERT_EQUAL(16, lot52_node("r20_3")->h);
    TEST_ASSERT_EQUAL(48, lot52_node("r20_3")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_rows_px_21(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:37px;grid-template-rows:repeat(2,18px);row-gap:1px;align-items:stretch'><div id=r21_0></div><div id=r21_1></div></div></body>", 480);
    TEST_ASSERT_EQUAL(2, lot52_node("g")->grid_rows);
    TEST_ASSERT_EQUAL(18, lot52_node("r21_0")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("r21_0")->y);
    TEST_ASSERT_EQUAL(18, lot52_node("r21_1")->h);
    TEST_ASSERT_EQUAL(19, lot52_node("r21_1")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_rows_px_22(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:64px;grid-template-rows:repeat(3,20px);row-gap:2px;align-items:stretch'><div id=r22_0></div><div id=r22_1></div><div id=r22_2></div></div></body>", 480);
    TEST_ASSERT_EQUAL(3, lot52_node("g")->grid_rows);
    TEST_ASSERT_EQUAL(20, lot52_node("r22_0")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("r22_0")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("r22_1")->h);
    TEST_ASSERT_EQUAL(22, lot52_node("r22_1")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("r22_2")->h);
    TEST_ASSERT_EQUAL(44, lot52_node("r22_2")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_rows_px_23(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:97px;grid-template-rows:repeat(4,22px);row-gap:3px;align-items:stretch'><div id=r23_0></div><div id=r23_1></div><div id=r23_2></div><div id=r23_3></div></div></body>", 480);
    TEST_ASSERT_EQUAL(4, lot52_node("g")->grid_rows);
    TEST_ASSERT_EQUAL(22, lot52_node("r23_0")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("r23_0")->y);
    TEST_ASSERT_EQUAL(22, lot52_node("r23_1")->h);
    TEST_ASSERT_EQUAL(25, lot52_node("r23_1")->y);
    TEST_ASSERT_EQUAL(22, lot52_node("r23_2")->h);
    TEST_ASSERT_EQUAL(50, lot52_node("r23_2")->y);
    TEST_ASSERT_EQUAL(22, lot52_node("r23_3")->h);
    TEST_ASSERT_EQUAL(75, lot52_node("r23_3")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_rows_px_24(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:28px;grid-template-rows:repeat(2,12px);row-gap:4px;align-items:stretch'><div id=r24_0></div><div id=r24_1></div></div></body>", 480);
    TEST_ASSERT_EQUAL(2, lot52_node("g")->grid_rows);
    TEST_ASSERT_EQUAL(12, lot52_node("r24_0")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("r24_0")->y);
    TEST_ASSERT_EQUAL(12, lot52_node("r24_1")->h);
    TEST_ASSERT_EQUAL(16, lot52_node("r24_1")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_rows_px_25(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:42px;grid-template-rows:repeat(3,14px);row-gap:0px;align-items:stretch'><div id=r25_0></div><div id=r25_1></div><div id=r25_2></div></div></body>", 480);
    TEST_ASSERT_EQUAL(3, lot52_node("g")->grid_rows);
    TEST_ASSERT_EQUAL(14, lot52_node("r25_0")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("r25_0")->y);
    TEST_ASSERT_EQUAL(14, lot52_node("r25_1")->h);
    TEST_ASSERT_EQUAL(14, lot52_node("r25_1")->y);
    TEST_ASSERT_EQUAL(14, lot52_node("r25_2")->h);
    TEST_ASSERT_EQUAL(28, lot52_node("r25_2")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_rows_px_26(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:67px;grid-template-rows:repeat(4,16px);row-gap:1px;align-items:stretch'><div id=r26_0></div><div id=r26_1></div><div id=r26_2></div><div id=r26_3></div></div></body>", 480);
    TEST_ASSERT_EQUAL(4, lot52_node("g")->grid_rows);
    TEST_ASSERT_EQUAL(16, lot52_node("r26_0")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("r26_0")->y);
    TEST_ASSERT_EQUAL(16, lot52_node("r26_1")->h);
    TEST_ASSERT_EQUAL(17, lot52_node("r26_1")->y);
    TEST_ASSERT_EQUAL(16, lot52_node("r26_2")->h);
    TEST_ASSERT_EQUAL(34, lot52_node("r26_2")->y);
    TEST_ASSERT_EQUAL(16, lot52_node("r26_3")->h);
    TEST_ASSERT_EQUAL(51, lot52_node("r26_3")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_rows_px_27(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:38px;grid-template-rows:repeat(2,18px);row-gap:2px;align-items:stretch'><div id=r27_0></div><div id=r27_1></div></div></body>", 480);
    TEST_ASSERT_EQUAL(2, lot52_node("g")->grid_rows);
    TEST_ASSERT_EQUAL(18, lot52_node("r27_0")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("r27_0")->y);
    TEST_ASSERT_EQUAL(18, lot52_node("r27_1")->h);
    TEST_ASSERT_EQUAL(20, lot52_node("r27_1")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_rows_px_28(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:66px;grid-template-rows:repeat(3,20px);row-gap:3px;align-items:stretch'><div id=r28_0></div><div id=r28_1></div><div id=r28_2></div></div></body>", 480);
    TEST_ASSERT_EQUAL(3, lot52_node("g")->grid_rows);
    TEST_ASSERT_EQUAL(20, lot52_node("r28_0")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("r28_0")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("r28_1")->h);
    TEST_ASSERT_EQUAL(23, lot52_node("r28_1")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("r28_2")->h);
    TEST_ASSERT_EQUAL(46, lot52_node("r28_2")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_rows_px_29(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:100px;grid-template-rows:repeat(4,22px);row-gap:4px;align-items:stretch'><div id=r29_0></div><div id=r29_1></div><div id=r29_2></div><div id=r29_3></div></div></body>", 480);
    TEST_ASSERT_EQUAL(4, lot52_node("g")->grid_rows);
    TEST_ASSERT_EQUAL(22, lot52_node("r29_0")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("r29_0")->y);
    TEST_ASSERT_EQUAL(22, lot52_node("r29_1")->h);
    TEST_ASSERT_EQUAL(26, lot52_node("r29_1")->y);
    TEST_ASSERT_EQUAL(22, lot52_node("r29_2")->h);
    TEST_ASSERT_EQUAL(52, lot52_node("r29_2")->y);
    TEST_ASSERT_EQUAL(22, lot52_node("r29_3")->h);
    TEST_ASSERT_EQUAL(78, lot52_node("r29_3")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_rows_px_30(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:24px;grid-template-rows:repeat(2,12px);row-gap:0px;align-items:stretch'><div id=r30_0></div><div id=r30_1></div></div></body>", 480);
    TEST_ASSERT_EQUAL(2, lot52_node("g")->grid_rows);
    TEST_ASSERT_EQUAL(12, lot52_node("r30_0")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("r30_0")->y);
    TEST_ASSERT_EQUAL(12, lot52_node("r30_1")->h);
    TEST_ASSERT_EQUAL(12, lot52_node("r30_1")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_rows_px_31(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:44px;grid-template-rows:repeat(3,14px);row-gap:1px;align-items:stretch'><div id=r31_0></div><div id=r31_1></div><div id=r31_2></div></div></body>", 480);
    TEST_ASSERT_EQUAL(3, lot52_node("g")->grid_rows);
    TEST_ASSERT_EQUAL(14, lot52_node("r31_0")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("r31_0")->y);
    TEST_ASSERT_EQUAL(14, lot52_node("r31_1")->h);
    TEST_ASSERT_EQUAL(15, lot52_node("r31_1")->y);
    TEST_ASSERT_EQUAL(14, lot52_node("r31_2")->h);
    TEST_ASSERT_EQUAL(30, lot52_node("r31_2")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_rows_px_32(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:70px;grid-template-rows:repeat(4,16px);row-gap:2px;align-items:stretch'><div id=r32_0></div><div id=r32_1></div><div id=r32_2></div><div id=r32_3></div></div></body>", 480);
    TEST_ASSERT_EQUAL(4, lot52_node("g")->grid_rows);
    TEST_ASSERT_EQUAL(16, lot52_node("r32_0")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("r32_0")->y);
    TEST_ASSERT_EQUAL(16, lot52_node("r32_1")->h);
    TEST_ASSERT_EQUAL(18, lot52_node("r32_1")->y);
    TEST_ASSERT_EQUAL(16, lot52_node("r32_2")->h);
    TEST_ASSERT_EQUAL(36, lot52_node("r32_2")->y);
    TEST_ASSERT_EQUAL(16, lot52_node("r32_3")->h);
    TEST_ASSERT_EQUAL(54, lot52_node("r32_3")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_rows_px_33(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:39px;grid-template-rows:repeat(2,18px);row-gap:3px;align-items:stretch'><div id=r33_0></div><div id=r33_1></div></div></body>", 480);
    TEST_ASSERT_EQUAL(2, lot52_node("g")->grid_rows);
    TEST_ASSERT_EQUAL(18, lot52_node("r33_0")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("r33_0")->y);
    TEST_ASSERT_EQUAL(18, lot52_node("r33_1")->h);
    TEST_ASSERT_EQUAL(21, lot52_node("r33_1")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_rows_px_34(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:68px;grid-template-rows:repeat(3,20px);row-gap:4px;align-items:stretch'><div id=r34_0></div><div id=r34_1></div><div id=r34_2></div></div></body>", 480);
    TEST_ASSERT_EQUAL(3, lot52_node("g")->grid_rows);
    TEST_ASSERT_EQUAL(20, lot52_node("r34_0")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("r34_0")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("r34_1")->h);
    TEST_ASSERT_EQUAL(24, lot52_node("r34_1")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("r34_2")->h);
    TEST_ASSERT_EQUAL(48, lot52_node("r34_2")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_rows_px_35(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:88px;grid-template-rows:repeat(4,22px);row-gap:0px;align-items:stretch'><div id=r35_0></div><div id=r35_1></div><div id=r35_2></div><div id=r35_3></div></div></body>", 480);
    TEST_ASSERT_EQUAL(4, lot52_node("g")->grid_rows);
    TEST_ASSERT_EQUAL(22, lot52_node("r35_0")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("r35_0")->y);
    TEST_ASSERT_EQUAL(22, lot52_node("r35_1")->h);
    TEST_ASSERT_EQUAL(22, lot52_node("r35_1")->y);
    TEST_ASSERT_EQUAL(22, lot52_node("r35_2")->h);
    TEST_ASSERT_EQUAL(44, lot52_node("r35_2")->y);
    TEST_ASSERT_EQUAL(22, lot52_node("r35_3")->h);
    TEST_ASSERT_EQUAL(66, lot52_node("r35_3")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_rows_px_36(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:25px;grid-template-rows:repeat(2,12px);row-gap:1px;align-items:stretch'><div id=r36_0></div><div id=r36_1></div></div></body>", 480);
    TEST_ASSERT_EQUAL(2, lot52_node("g")->grid_rows);
    TEST_ASSERT_EQUAL(12, lot52_node("r36_0")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("r36_0")->y);
    TEST_ASSERT_EQUAL(12, lot52_node("r36_1")->h);
    TEST_ASSERT_EQUAL(13, lot52_node("r36_1")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_rows_px_37(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:46px;grid-template-rows:repeat(3,14px);row-gap:2px;align-items:stretch'><div id=r37_0></div><div id=r37_1></div><div id=r37_2></div></div></body>", 480);
    TEST_ASSERT_EQUAL(3, lot52_node("g")->grid_rows);
    TEST_ASSERT_EQUAL(14, lot52_node("r37_0")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("r37_0")->y);
    TEST_ASSERT_EQUAL(14, lot52_node("r37_1")->h);
    TEST_ASSERT_EQUAL(16, lot52_node("r37_1")->y);
    TEST_ASSERT_EQUAL(14, lot52_node("r37_2")->h);
    TEST_ASSERT_EQUAL(32, lot52_node("r37_2")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_rows_px_38(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:73px;grid-template-rows:repeat(4,16px);row-gap:3px;align-items:stretch'><div id=r38_0></div><div id=r38_1></div><div id=r38_2></div><div id=r38_3></div></div></body>", 480);
    TEST_ASSERT_EQUAL(4, lot52_node("g")->grid_rows);
    TEST_ASSERT_EQUAL(16, lot52_node("r38_0")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("r38_0")->y);
    TEST_ASSERT_EQUAL(16, lot52_node("r38_1")->h);
    TEST_ASSERT_EQUAL(19, lot52_node("r38_1")->y);
    TEST_ASSERT_EQUAL(16, lot52_node("r38_2")->h);
    TEST_ASSERT_EQUAL(38, lot52_node("r38_2")->y);
    TEST_ASSERT_EQUAL(16, lot52_node("r38_3")->h);
    TEST_ASSERT_EQUAL(57, lot52_node("r38_3")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_rows_px_39(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:40px;grid-template-rows:repeat(2,18px);row-gap:4px;align-items:stretch'><div id=r39_0></div><div id=r39_1></div></div></body>", 480);
    TEST_ASSERT_EQUAL(2, lot52_node("g")->grid_rows);
    TEST_ASSERT_EQUAL(18, lot52_node("r39_0")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("r39_0")->y);
    TEST_ASSERT_EQUAL(18, lot52_node("r39_1")->h);
    TEST_ASSERT_EQUAL(22, lot52_node("r39_1")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_rows_px_40(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:60px;grid-template-rows:repeat(3,20px);row-gap:0px;align-items:stretch'><div id=r40_0></div><div id=r40_1></div><div id=r40_2></div></div></body>", 480);
    TEST_ASSERT_EQUAL(3, lot52_node("g")->grid_rows);
    TEST_ASSERT_EQUAL(20, lot52_node("r40_0")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("r40_0")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("r40_1")->h);
    TEST_ASSERT_EQUAL(20, lot52_node("r40_1")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("r40_2")->h);
    TEST_ASSERT_EQUAL(40, lot52_node("r40_2")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_rows_px_41(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:91px;grid-template-rows:repeat(4,22px);row-gap:1px;align-items:stretch'><div id=r41_0></div><div id=r41_1></div><div id=r41_2></div><div id=r41_3></div></div></body>", 480);
    TEST_ASSERT_EQUAL(4, lot52_node("g")->grid_rows);
    TEST_ASSERT_EQUAL(22, lot52_node("r41_0")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("r41_0")->y);
    TEST_ASSERT_EQUAL(22, lot52_node("r41_1")->h);
    TEST_ASSERT_EQUAL(23, lot52_node("r41_1")->y);
    TEST_ASSERT_EQUAL(22, lot52_node("r41_2")->h);
    TEST_ASSERT_EQUAL(46, lot52_node("r41_2")->y);
    TEST_ASSERT_EQUAL(22, lot52_node("r41_3")->h);
    TEST_ASSERT_EQUAL(69, lot52_node("r41_3")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_rows_px_42(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:26px;grid-template-rows:repeat(2,12px);row-gap:2px;align-items:stretch'><div id=r42_0></div><div id=r42_1></div></div></body>", 480);
    TEST_ASSERT_EQUAL(2, lot52_node("g")->grid_rows);
    TEST_ASSERT_EQUAL(12, lot52_node("r42_0")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("r42_0")->y);
    TEST_ASSERT_EQUAL(12, lot52_node("r42_1")->h);
    TEST_ASSERT_EQUAL(14, lot52_node("r42_1")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_rows_px_43(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:48px;grid-template-rows:repeat(3,14px);row-gap:3px;align-items:stretch'><div id=r43_0></div><div id=r43_1></div><div id=r43_2></div></div></body>", 480);
    TEST_ASSERT_EQUAL(3, lot52_node("g")->grid_rows);
    TEST_ASSERT_EQUAL(14, lot52_node("r43_0")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("r43_0")->y);
    TEST_ASSERT_EQUAL(14, lot52_node("r43_1")->h);
    TEST_ASSERT_EQUAL(17, lot52_node("r43_1")->y);
    TEST_ASSERT_EQUAL(14, lot52_node("r43_2")->h);
    TEST_ASSERT_EQUAL(34, lot52_node("r43_2")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_rows_px_44(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:76px;grid-template-rows:repeat(4,16px);row-gap:4px;align-items:stretch'><div id=r44_0></div><div id=r44_1></div><div id=r44_2></div><div id=r44_3></div></div></body>", 480);
    TEST_ASSERT_EQUAL(4, lot52_node("g")->grid_rows);
    TEST_ASSERT_EQUAL(16, lot52_node("r44_0")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("r44_0")->y);
    TEST_ASSERT_EQUAL(16, lot52_node("r44_1")->h);
    TEST_ASSERT_EQUAL(20, lot52_node("r44_1")->y);
    TEST_ASSERT_EQUAL(16, lot52_node("r44_2")->h);
    TEST_ASSERT_EQUAL(40, lot52_node("r44_2")->y);
    TEST_ASSERT_EQUAL(16, lot52_node("r44_3")->h);
    TEST_ASSERT_EQUAL(60, lot52_node("r44_3")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_rows_px_45(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:36px;grid-template-rows:repeat(2,18px);row-gap:0px;align-items:stretch'><div id=r45_0></div><div id=r45_1></div></div></body>", 480);
    TEST_ASSERT_EQUAL(2, lot52_node("g")->grid_rows);
    TEST_ASSERT_EQUAL(18, lot52_node("r45_0")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("r45_0")->y);
    TEST_ASSERT_EQUAL(18, lot52_node("r45_1")->h);
    TEST_ASSERT_EQUAL(18, lot52_node("r45_1")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_rows_px_46(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:62px;grid-template-rows:repeat(3,20px);row-gap:1px;align-items:stretch'><div id=r46_0></div><div id=r46_1></div><div id=r46_2></div></div></body>", 480);
    TEST_ASSERT_EQUAL(3, lot52_node("g")->grid_rows);
    TEST_ASSERT_EQUAL(20, lot52_node("r46_0")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("r46_0")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("r46_1")->h);
    TEST_ASSERT_EQUAL(21, lot52_node("r46_1")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("r46_2")->h);
    TEST_ASSERT_EQUAL(42, lot52_node("r46_2")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_rows_px_47(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:94px;grid-template-rows:repeat(4,22px);row-gap:2px;align-items:stretch'><div id=r47_0></div><div id=r47_1></div><div id=r47_2></div><div id=r47_3></div></div></body>", 480);
    TEST_ASSERT_EQUAL(4, lot52_node("g")->grid_rows);
    TEST_ASSERT_EQUAL(22, lot52_node("r47_0")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("r47_0")->y);
    TEST_ASSERT_EQUAL(22, lot52_node("r47_1")->h);
    TEST_ASSERT_EQUAL(24, lot52_node("r47_1")->y);
    TEST_ASSERT_EQUAL(22, lot52_node("r47_2")->h);
    TEST_ASSERT_EQUAL(48, lot52_node("r47_2")->y);
    TEST_ASSERT_EQUAL(22, lot52_node("r47_3")->h);
    TEST_ASSERT_EQUAL(72, lot52_node("r47_3")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_rows_px_48(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:27px;grid-template-rows:repeat(2,12px);row-gap:3px;align-items:stretch'><div id=r48_0></div><div id=r48_1></div></div></body>", 480);
    TEST_ASSERT_EQUAL(2, lot52_node("g")->grid_rows);
    TEST_ASSERT_EQUAL(12, lot52_node("r48_0")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("r48_0")->y);
    TEST_ASSERT_EQUAL(12, lot52_node("r48_1")->h);
    TEST_ASSERT_EQUAL(15, lot52_node("r48_1")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_rows_px_49(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:50px;grid-template-rows:repeat(3,14px);row-gap:4px;align-items:stretch'><div id=r49_0></div><div id=r49_1></div><div id=r49_2></div></div></body>", 480);
    TEST_ASSERT_EQUAL(3, lot52_node("g")->grid_rows);
    TEST_ASSERT_EQUAL(14, lot52_node("r49_0")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("r49_0")->y);
    TEST_ASSERT_EQUAL(14, lot52_node("r49_1")->h);
    TEST_ASSERT_EQUAL(18, lot52_node("r49_1")->y);
    TEST_ASSERT_EQUAL(14, lot52_node("r49_2")->h);
    TEST_ASSERT_EQUAL(36, lot52_node("r49_2")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_rows_px_50(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:64px;grid-template-rows:repeat(4,16px);row-gap:0px;align-items:stretch'><div id=r50_0></div><div id=r50_1></div><div id=r50_2></div><div id=r50_3></div></div></body>", 480);
    TEST_ASSERT_EQUAL(4, lot52_node("g")->grid_rows);
    TEST_ASSERT_EQUAL(16, lot52_node("r50_0")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("r50_0")->y);
    TEST_ASSERT_EQUAL(16, lot52_node("r50_1")->h);
    TEST_ASSERT_EQUAL(16, lot52_node("r50_1")->y);
    TEST_ASSERT_EQUAL(16, lot52_node("r50_2")->h);
    TEST_ASSERT_EQUAL(32, lot52_node("r50_2")->y);
    TEST_ASSERT_EQUAL(16, lot52_node("r50_3")->h);
    TEST_ASSERT_EQUAL(48, lot52_node("r50_3")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_rows_px_51(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:37px;grid-template-rows:repeat(2,18px);row-gap:1px;align-items:stretch'><div id=r51_0></div><div id=r51_1></div></div></body>", 480);
    TEST_ASSERT_EQUAL(2, lot52_node("g")->grid_rows);
    TEST_ASSERT_EQUAL(18, lot52_node("r51_0")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("r51_0")->y);
    TEST_ASSERT_EQUAL(18, lot52_node("r51_1")->h);
    TEST_ASSERT_EQUAL(19, lot52_node("r51_1")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_rows_px_52(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:64px;grid-template-rows:repeat(3,20px);row-gap:2px;align-items:stretch'><div id=r52_0></div><div id=r52_1></div><div id=r52_2></div></div></body>", 480);
    TEST_ASSERT_EQUAL(3, lot52_node("g")->grid_rows);
    TEST_ASSERT_EQUAL(20, lot52_node("r52_0")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("r52_0")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("r52_1")->h);
    TEST_ASSERT_EQUAL(22, lot52_node("r52_1")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("r52_2")->h);
    TEST_ASSERT_EQUAL(44, lot52_node("r52_2")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_rows_px_53(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:97px;grid-template-rows:repeat(4,22px);row-gap:3px;align-items:stretch'><div id=r53_0></div><div id=r53_1></div><div id=r53_2></div><div id=r53_3></div></div></body>", 480);
    TEST_ASSERT_EQUAL(4, lot52_node("g")->grid_rows);
    TEST_ASSERT_EQUAL(22, lot52_node("r53_0")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("r53_0")->y);
    TEST_ASSERT_EQUAL(22, lot52_node("r53_1")->h);
    TEST_ASSERT_EQUAL(25, lot52_node("r53_1")->y);
    TEST_ASSERT_EQUAL(22, lot52_node("r53_2")->h);
    TEST_ASSERT_EQUAL(50, lot52_node("r53_2")->y);
    TEST_ASSERT_EQUAL(22, lot52_node("r53_3")->h);
    TEST_ASSERT_EQUAL(75, lot52_node("r53_3")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_rows_px_54(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:28px;grid-template-rows:repeat(2,12px);row-gap:4px;align-items:stretch'><div id=r54_0></div><div id=r54_1></div></div></body>", 480);
    TEST_ASSERT_EQUAL(2, lot52_node("g")->grid_rows);
    TEST_ASSERT_EQUAL(12, lot52_node("r54_0")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("r54_0")->y);
    TEST_ASSERT_EQUAL(12, lot52_node("r54_1")->h);
    TEST_ASSERT_EQUAL(16, lot52_node("r54_1")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_rows_px_55(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:42px;grid-template-rows:repeat(3,14px);row-gap:0px;align-items:stretch'><div id=r55_0></div><div id=r55_1></div><div id=r55_2></div></div></body>", 480);
    TEST_ASSERT_EQUAL(3, lot52_node("g")->grid_rows);
    TEST_ASSERT_EQUAL(14, lot52_node("r55_0")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("r55_0")->y);
    TEST_ASSERT_EQUAL(14, lot52_node("r55_1")->h);
    TEST_ASSERT_EQUAL(14, lot52_node("r55_1")->y);
    TEST_ASSERT_EQUAL(14, lot52_node("r55_2")->h);
    TEST_ASSERT_EQUAL(28, lot52_node("r55_2")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_rows_px_56(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:67px;grid-template-rows:repeat(4,16px);row-gap:1px;align-items:stretch'><div id=r56_0></div><div id=r56_1></div><div id=r56_2></div><div id=r56_3></div></div></body>", 480);
    TEST_ASSERT_EQUAL(4, lot52_node("g")->grid_rows);
    TEST_ASSERT_EQUAL(16, lot52_node("r56_0")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("r56_0")->y);
    TEST_ASSERT_EQUAL(16, lot52_node("r56_1")->h);
    TEST_ASSERT_EQUAL(17, lot52_node("r56_1")->y);
    TEST_ASSERT_EQUAL(16, lot52_node("r56_2")->h);
    TEST_ASSERT_EQUAL(34, lot52_node("r56_2")->y);
    TEST_ASSERT_EQUAL(16, lot52_node("r56_3")->h);
    TEST_ASSERT_EQUAL(51, lot52_node("r56_3")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_rows_px_57(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:38px;grid-template-rows:repeat(2,18px);row-gap:2px;align-items:stretch'><div id=r57_0></div><div id=r57_1></div></div></body>", 480);
    TEST_ASSERT_EQUAL(2, lot52_node("g")->grid_rows);
    TEST_ASSERT_EQUAL(18, lot52_node("r57_0")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("r57_0")->y);
    TEST_ASSERT_EQUAL(18, lot52_node("r57_1")->h);
    TEST_ASSERT_EQUAL(20, lot52_node("r57_1")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_rows_px_58(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:66px;grid-template-rows:repeat(3,20px);row-gap:3px;align-items:stretch'><div id=r58_0></div><div id=r58_1></div><div id=r58_2></div></div></body>", 480);
    TEST_ASSERT_EQUAL(3, lot52_node("g")->grid_rows);
    TEST_ASSERT_EQUAL(20, lot52_node("r58_0")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("r58_0")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("r58_1")->h);
    TEST_ASSERT_EQUAL(23, lot52_node("r58_1")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("r58_2")->h);
    TEST_ASSERT_EQUAL(46, lot52_node("r58_2")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_rows_px_59(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:100px;grid-template-rows:repeat(4,22px);row-gap:4px;align-items:stretch'><div id=r59_0></div><div id=r59_1></div><div id=r59_2></div><div id=r59_3></div></div></body>", 480);
    TEST_ASSERT_EQUAL(4, lot52_node("g")->grid_rows);
    TEST_ASSERT_EQUAL(22, lot52_node("r59_0")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("r59_0")->y);
    TEST_ASSERT_EQUAL(22, lot52_node("r59_1")->h);
    TEST_ASSERT_EQUAL(26, lot52_node("r59_1")->y);
    TEST_ASSERT_EQUAL(22, lot52_node("r59_2")->h);
    TEST_ASSERT_EQUAL(52, lot52_node("r59_2")->y);
    TEST_ASSERT_EQUAL(22, lot52_node("r59_3")->h);
    TEST_ASSERT_EQUAL(78, lot52_node("r59_3")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_rows_px_60(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:24px;grid-template-rows:repeat(2,12px);row-gap:0px;align-items:stretch'><div id=r60_0></div><div id=r60_1></div></div></body>", 480);
    TEST_ASSERT_EQUAL(2, lot52_node("g")->grid_rows);
    TEST_ASSERT_EQUAL(12, lot52_node("r60_0")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("r60_0")->y);
    TEST_ASSERT_EQUAL(12, lot52_node("r60_1")->h);
    TEST_ASSERT_EQUAL(12, lot52_node("r60_1")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_rows_px_61(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:44px;grid-template-rows:repeat(3,14px);row-gap:1px;align-items:stretch'><div id=r61_0></div><div id=r61_1></div><div id=r61_2></div></div></body>", 480);
    TEST_ASSERT_EQUAL(3, lot52_node("g")->grid_rows);
    TEST_ASSERT_EQUAL(14, lot52_node("r61_0")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("r61_0")->y);
    TEST_ASSERT_EQUAL(14, lot52_node("r61_1")->h);
    TEST_ASSERT_EQUAL(15, lot52_node("r61_1")->y);
    TEST_ASSERT_EQUAL(14, lot52_node("r61_2")->h);
    TEST_ASSERT_EQUAL(30, lot52_node("r61_2")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_rows_px_62(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:70px;grid-template-rows:repeat(4,16px);row-gap:2px;align-items:stretch'><div id=r62_0></div><div id=r62_1></div><div id=r62_2></div><div id=r62_3></div></div></body>", 480);
    TEST_ASSERT_EQUAL(4, lot52_node("g")->grid_rows);
    TEST_ASSERT_EQUAL(16, lot52_node("r62_0")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("r62_0")->y);
    TEST_ASSERT_EQUAL(16, lot52_node("r62_1")->h);
    TEST_ASSERT_EQUAL(18, lot52_node("r62_1")->y);
    TEST_ASSERT_EQUAL(16, lot52_node("r62_2")->h);
    TEST_ASSERT_EQUAL(36, lot52_node("r62_2")->y);
    TEST_ASSERT_EQUAL(16, lot52_node("r62_3")->h);
    TEST_ASSERT_EQUAL(54, lot52_node("r62_3")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_rows_px_63(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:39px;grid-template-rows:repeat(2,18px);row-gap:3px;align-items:stretch'><div id=r63_0></div><div id=r63_1></div></div></body>", 480);
    TEST_ASSERT_EQUAL(2, lot52_node("g")->grid_rows);
    TEST_ASSERT_EQUAL(18, lot52_node("r63_0")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("r63_0")->y);
    TEST_ASSERT_EQUAL(18, lot52_node("r63_1")->h);
    TEST_ASSERT_EQUAL(21, lot52_node("r63_1")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_rows_px_64(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:68px;grid-template-rows:repeat(3,20px);row-gap:4px;align-items:stretch'><div id=r64_0></div><div id=r64_1></div><div id=r64_2></div></div></body>", 480);
    TEST_ASSERT_EQUAL(3, lot52_node("g")->grid_rows);
    TEST_ASSERT_EQUAL(20, lot52_node("r64_0")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("r64_0")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("r64_1")->h);
    TEST_ASSERT_EQUAL(24, lot52_node("r64_1")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("r64_2")->h);
    TEST_ASSERT_EQUAL(48, lot52_node("r64_2")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_rows_px_65(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:88px;grid-template-rows:repeat(4,22px);row-gap:0px;align-items:stretch'><div id=r65_0></div><div id=r65_1></div><div id=r65_2></div><div id=r65_3></div></div></body>", 480);
    TEST_ASSERT_EQUAL(4, lot52_node("g")->grid_rows);
    TEST_ASSERT_EQUAL(22, lot52_node("r65_0")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("r65_0")->y);
    TEST_ASSERT_EQUAL(22, lot52_node("r65_1")->h);
    TEST_ASSERT_EQUAL(22, lot52_node("r65_1")->y);
    TEST_ASSERT_EQUAL(22, lot52_node("r65_2")->h);
    TEST_ASSERT_EQUAL(44, lot52_node("r65_2")->y);
    TEST_ASSERT_EQUAL(22, lot52_node("r65_3")->h);
    TEST_ASSERT_EQUAL(66, lot52_node("r65_3")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_rows_px_66(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:25px;grid-template-rows:repeat(2,12px);row-gap:1px;align-items:stretch'><div id=r66_0></div><div id=r66_1></div></div></body>", 480);
    TEST_ASSERT_EQUAL(2, lot52_node("g")->grid_rows);
    TEST_ASSERT_EQUAL(12, lot52_node("r66_0")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("r66_0")->y);
    TEST_ASSERT_EQUAL(12, lot52_node("r66_1")->h);
    TEST_ASSERT_EQUAL(13, lot52_node("r66_1")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_rows_px_67(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:46px;grid-template-rows:repeat(3,14px);row-gap:2px;align-items:stretch'><div id=r67_0></div><div id=r67_1></div><div id=r67_2></div></div></body>", 480);
    TEST_ASSERT_EQUAL(3, lot52_node("g")->grid_rows);
    TEST_ASSERT_EQUAL(14, lot52_node("r67_0")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("r67_0")->y);
    TEST_ASSERT_EQUAL(14, lot52_node("r67_1")->h);
    TEST_ASSERT_EQUAL(16, lot52_node("r67_1")->y);
    TEST_ASSERT_EQUAL(14, lot52_node("r67_2")->h);
    TEST_ASSERT_EQUAL(32, lot52_node("r67_2")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_rows_px_68(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:73px;grid-template-rows:repeat(4,16px);row-gap:3px;align-items:stretch'><div id=r68_0></div><div id=r68_1></div><div id=r68_2></div><div id=r68_3></div></div></body>", 480);
    TEST_ASSERT_EQUAL(4, lot52_node("g")->grid_rows);
    TEST_ASSERT_EQUAL(16, lot52_node("r68_0")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("r68_0")->y);
    TEST_ASSERT_EQUAL(16, lot52_node("r68_1")->h);
    TEST_ASSERT_EQUAL(19, lot52_node("r68_1")->y);
    TEST_ASSERT_EQUAL(16, lot52_node("r68_2")->h);
    TEST_ASSERT_EQUAL(38, lot52_node("r68_2")->y);
    TEST_ASSERT_EQUAL(16, lot52_node("r68_3")->h);
    TEST_ASSERT_EQUAL(57, lot52_node("r68_3")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_rows_px_69(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:40px;grid-template-rows:repeat(2,18px);row-gap:4px;align-items:stretch'><div id=r69_0></div><div id=r69_1></div></div></body>", 480);
    TEST_ASSERT_EQUAL(2, lot52_node("g")->grid_rows);
    TEST_ASSERT_EQUAL(18, lot52_node("r69_0")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("r69_0")->y);
    TEST_ASSERT_EQUAL(18, lot52_node("r69_1")->h);
    TEST_ASSERT_EQUAL(22, lot52_node("r69_1")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_rows_px_70(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:60px;grid-template-rows:repeat(3,20px);row-gap:0px;align-items:stretch'><div id=r70_0></div><div id=r70_1></div><div id=r70_2></div></div></body>", 480);
    TEST_ASSERT_EQUAL(3, lot52_node("g")->grid_rows);
    TEST_ASSERT_EQUAL(20, lot52_node("r70_0")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("r70_0")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("r70_1")->h);
    TEST_ASSERT_EQUAL(20, lot52_node("r70_1")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("r70_2")->h);
    TEST_ASSERT_EQUAL(40, lot52_node("r70_2")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_rows_px_71(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:91px;grid-template-rows:repeat(4,22px);row-gap:1px;align-items:stretch'><div id=r71_0></div><div id=r71_1></div><div id=r71_2></div><div id=r71_3></div></div></body>", 480);
    TEST_ASSERT_EQUAL(4, lot52_node("g")->grid_rows);
    TEST_ASSERT_EQUAL(22, lot52_node("r71_0")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("r71_0")->y);
    TEST_ASSERT_EQUAL(22, lot52_node("r71_1")->h);
    TEST_ASSERT_EQUAL(23, lot52_node("r71_1")->y);
    TEST_ASSERT_EQUAL(22, lot52_node("r71_2")->h);
    TEST_ASSERT_EQUAL(46, lot52_node("r71_2")->y);
    TEST_ASSERT_EQUAL(22, lot52_node("r71_3")->h);
    TEST_ASSERT_EQUAL(69, lot52_node("r71_3")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_rows_px_72(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:26px;grid-template-rows:repeat(2,12px);row-gap:2px;align-items:stretch'><div id=r72_0></div><div id=r72_1></div></div></body>", 480);
    TEST_ASSERT_EQUAL(2, lot52_node("g")->grid_rows);
    TEST_ASSERT_EQUAL(12, lot52_node("r72_0")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("r72_0")->y);
    TEST_ASSERT_EQUAL(12, lot52_node("r72_1")->h);
    TEST_ASSERT_EQUAL(14, lot52_node("r72_1")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_rows_px_73(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:48px;grid-template-rows:repeat(3,14px);row-gap:3px;align-items:stretch'><div id=r73_0></div><div id=r73_1></div><div id=r73_2></div></div></body>", 480);
    TEST_ASSERT_EQUAL(3, lot52_node("g")->grid_rows);
    TEST_ASSERT_EQUAL(14, lot52_node("r73_0")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("r73_0")->y);
    TEST_ASSERT_EQUAL(14, lot52_node("r73_1")->h);
    TEST_ASSERT_EQUAL(17, lot52_node("r73_1")->y);
    TEST_ASSERT_EQUAL(14, lot52_node("r73_2")->h);
    TEST_ASSERT_EQUAL(34, lot52_node("r73_2")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_rows_px_74(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:76px;grid-template-rows:repeat(4,16px);row-gap:4px;align-items:stretch'><div id=r74_0></div><div id=r74_1></div><div id=r74_2></div><div id=r74_3></div></div></body>", 480);
    TEST_ASSERT_EQUAL(4, lot52_node("g")->grid_rows);
    TEST_ASSERT_EQUAL(16, lot52_node("r74_0")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("r74_0")->y);
    TEST_ASSERT_EQUAL(16, lot52_node("r74_1")->h);
    TEST_ASSERT_EQUAL(20, lot52_node("r74_1")->y);
    TEST_ASSERT_EQUAL(16, lot52_node("r74_2")->h);
    TEST_ASSERT_EQUAL(40, lot52_node("r74_2")->y);
    TEST_ASSERT_EQUAL(16, lot52_node("r74_3")->h);
    TEST_ASSERT_EQUAL(60, lot52_node("r74_3")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_rows_px_75(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:36px;grid-template-rows:repeat(2,18px);row-gap:0px;align-items:stretch'><div id=r75_0></div><div id=r75_1></div></div></body>", 480);
    TEST_ASSERT_EQUAL(2, lot52_node("g")->grid_rows);
    TEST_ASSERT_EQUAL(18, lot52_node("r75_0")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("r75_0")->y);
    TEST_ASSERT_EQUAL(18, lot52_node("r75_1")->h);
    TEST_ASSERT_EQUAL(18, lot52_node("r75_1")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_rows_px_76(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:62px;grid-template-rows:repeat(3,20px);row-gap:1px;align-items:stretch'><div id=r76_0></div><div id=r76_1></div><div id=r76_2></div></div></body>", 480);
    TEST_ASSERT_EQUAL(3, lot52_node("g")->grid_rows);
    TEST_ASSERT_EQUAL(20, lot52_node("r76_0")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("r76_0")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("r76_1")->h);
    TEST_ASSERT_EQUAL(21, lot52_node("r76_1")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("r76_2")->h);
    TEST_ASSERT_EQUAL(42, lot52_node("r76_2")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_rows_px_77(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:94px;grid-template-rows:repeat(4,22px);row-gap:2px;align-items:stretch'><div id=r77_0></div><div id=r77_1></div><div id=r77_2></div><div id=r77_3></div></div></body>", 480);
    TEST_ASSERT_EQUAL(4, lot52_node("g")->grid_rows);
    TEST_ASSERT_EQUAL(22, lot52_node("r77_0")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("r77_0")->y);
    TEST_ASSERT_EQUAL(22, lot52_node("r77_1")->h);
    TEST_ASSERT_EQUAL(24, lot52_node("r77_1")->y);
    TEST_ASSERT_EQUAL(22, lot52_node("r77_2")->h);
    TEST_ASSERT_EQUAL(48, lot52_node("r77_2")->y);
    TEST_ASSERT_EQUAL(22, lot52_node("r77_3")->h);
    TEST_ASSERT_EQUAL(72, lot52_node("r77_3")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_rows_px_78(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:27px;grid-template-rows:repeat(2,12px);row-gap:3px;align-items:stretch'><div id=r78_0></div><div id=r78_1></div></div></body>", 480);
    TEST_ASSERT_EQUAL(2, lot52_node("g")->grid_rows);
    TEST_ASSERT_EQUAL(12, lot52_node("r78_0")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("r78_0")->y);
    TEST_ASSERT_EQUAL(12, lot52_node("r78_1")->h);
    TEST_ASSERT_EQUAL(15, lot52_node("r78_1")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_rows_px_79(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:50px;grid-template-rows:repeat(3,14px);row-gap:4px;align-items:stretch'><div id=r79_0></div><div id=r79_1></div><div id=r79_2></div></div></body>", 480);
    TEST_ASSERT_EQUAL(3, lot52_node("g")->grid_rows);
    TEST_ASSERT_EQUAL(14, lot52_node("r79_0")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("r79_0")->y);
    TEST_ASSERT_EQUAL(14, lot52_node("r79_1")->h);
    TEST_ASSERT_EQUAL(18, lot52_node("r79_1")->y);
    TEST_ASSERT_EQUAL(14, lot52_node("r79_2")->h);
    TEST_ASSERT_EQUAL(36, lot52_node("r79_2")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_rows_px_80(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:64px;grid-template-rows:repeat(4,16px);row-gap:0px;align-items:stretch'><div id=r80_0></div><div id=r80_1></div><div id=r80_2></div><div id=r80_3></div></div></body>", 480);
    TEST_ASSERT_EQUAL(4, lot52_node("g")->grid_rows);
    TEST_ASSERT_EQUAL(16, lot52_node("r80_0")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("r80_0")->y);
    TEST_ASSERT_EQUAL(16, lot52_node("r80_1")->h);
    TEST_ASSERT_EQUAL(16, lot52_node("r80_1")->y);
    TEST_ASSERT_EQUAL(16, lot52_node("r80_2")->h);
    TEST_ASSERT_EQUAL(32, lot52_node("r80_2")->y);
    TEST_ASSERT_EQUAL(16, lot52_node("r80_3")->h);
    TEST_ASSERT_EQUAL(48, lot52_node("r80_3")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_rows_px_81(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:37px;grid-template-rows:repeat(2,18px);row-gap:1px;align-items:stretch'><div id=r81_0></div><div id=r81_1></div></div></body>", 480);
    TEST_ASSERT_EQUAL(2, lot52_node("g")->grid_rows);
    TEST_ASSERT_EQUAL(18, lot52_node("r81_0")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("r81_0")->y);
    TEST_ASSERT_EQUAL(18, lot52_node("r81_1")->h);
    TEST_ASSERT_EQUAL(19, lot52_node("r81_1")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_rows_px_82(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:64px;grid-template-rows:repeat(3,20px);row-gap:2px;align-items:stretch'><div id=r82_0></div><div id=r82_1></div><div id=r82_2></div></div></body>", 480);
    TEST_ASSERT_EQUAL(3, lot52_node("g")->grid_rows);
    TEST_ASSERT_EQUAL(20, lot52_node("r82_0")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("r82_0")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("r82_1")->h);
    TEST_ASSERT_EQUAL(22, lot52_node("r82_1")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("r82_2")->h);
    TEST_ASSERT_EQUAL(44, lot52_node("r82_2")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_rows_px_83(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:97px;grid-template-rows:repeat(4,22px);row-gap:3px;align-items:stretch'><div id=r83_0></div><div id=r83_1></div><div id=r83_2></div><div id=r83_3></div></div></body>", 480);
    TEST_ASSERT_EQUAL(4, lot52_node("g")->grid_rows);
    TEST_ASSERT_EQUAL(22, lot52_node("r83_0")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("r83_0")->y);
    TEST_ASSERT_EQUAL(22, lot52_node("r83_1")->h);
    TEST_ASSERT_EQUAL(25, lot52_node("r83_1")->y);
    TEST_ASSERT_EQUAL(22, lot52_node("r83_2")->h);
    TEST_ASSERT_EQUAL(50, lot52_node("r83_2")->y);
    TEST_ASSERT_EQUAL(22, lot52_node("r83_3")->h);
    TEST_ASSERT_EQUAL(75, lot52_node("r83_3")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_rows_px_84(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:28px;grid-template-rows:repeat(2,12px);row-gap:4px;align-items:stretch'><div id=r84_0></div><div id=r84_1></div></div></body>", 480);
    TEST_ASSERT_EQUAL(2, lot52_node("g")->grid_rows);
    TEST_ASSERT_EQUAL(12, lot52_node("r84_0")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("r84_0")->y);
    TEST_ASSERT_EQUAL(12, lot52_node("r84_1")->h);
    TEST_ASSERT_EQUAL(16, lot52_node("r84_1")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_rows_px_85(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:42px;grid-template-rows:repeat(3,14px);row-gap:0px;align-items:stretch'><div id=r85_0></div><div id=r85_1></div><div id=r85_2></div></div></body>", 480);
    TEST_ASSERT_EQUAL(3, lot52_node("g")->grid_rows);
    TEST_ASSERT_EQUAL(14, lot52_node("r85_0")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("r85_0")->y);
    TEST_ASSERT_EQUAL(14, lot52_node("r85_1")->h);
    TEST_ASSERT_EQUAL(14, lot52_node("r85_1")->y);
    TEST_ASSERT_EQUAL(14, lot52_node("r85_2")->h);
    TEST_ASSERT_EQUAL(28, lot52_node("r85_2")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_rows_px_86(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:67px;grid-template-rows:repeat(4,16px);row-gap:1px;align-items:stretch'><div id=r86_0></div><div id=r86_1></div><div id=r86_2></div><div id=r86_3></div></div></body>", 480);
    TEST_ASSERT_EQUAL(4, lot52_node("g")->grid_rows);
    TEST_ASSERT_EQUAL(16, lot52_node("r86_0")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("r86_0")->y);
    TEST_ASSERT_EQUAL(16, lot52_node("r86_1")->h);
    TEST_ASSERT_EQUAL(17, lot52_node("r86_1")->y);
    TEST_ASSERT_EQUAL(16, lot52_node("r86_2")->h);
    TEST_ASSERT_EQUAL(34, lot52_node("r86_2")->y);
    TEST_ASSERT_EQUAL(16, lot52_node("r86_3")->h);
    TEST_ASSERT_EQUAL(51, lot52_node("r86_3")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_rows_px_87(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:38px;grid-template-rows:repeat(2,18px);row-gap:2px;align-items:stretch'><div id=r87_0></div><div id=r87_1></div></div></body>", 480);
    TEST_ASSERT_EQUAL(2, lot52_node("g")->grid_rows);
    TEST_ASSERT_EQUAL(18, lot52_node("r87_0")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("r87_0")->y);
    TEST_ASSERT_EQUAL(18, lot52_node("r87_1")->h);
    TEST_ASSERT_EQUAL(20, lot52_node("r87_1")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_rows_px_88(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:66px;grid-template-rows:repeat(3,20px);row-gap:3px;align-items:stretch'><div id=r88_0></div><div id=r88_1></div><div id=r88_2></div></div></body>", 480);
    TEST_ASSERT_EQUAL(3, lot52_node("g")->grid_rows);
    TEST_ASSERT_EQUAL(20, lot52_node("r88_0")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("r88_0")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("r88_1")->h);
    TEST_ASSERT_EQUAL(23, lot52_node("r88_1")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("r88_2")->h);
    TEST_ASSERT_EQUAL(46, lot52_node("r88_2")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_rows_px_89(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:100px;grid-template-rows:repeat(4,22px);row-gap:4px;align-items:stretch'><div id=r89_0></div><div id=r89_1></div><div id=r89_2></div><div id=r89_3></div></div></body>", 480);
    TEST_ASSERT_EQUAL(4, lot52_node("g")->grid_rows);
    TEST_ASSERT_EQUAL(22, lot52_node("r89_0")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("r89_0")->y);
    TEST_ASSERT_EQUAL(22, lot52_node("r89_1")->h);
    TEST_ASSERT_EQUAL(26, lot52_node("r89_1")->y);
    TEST_ASSERT_EQUAL(22, lot52_node("r89_2")->h);
    TEST_ASSERT_EQUAL(52, lot52_node("r89_2")->y);
    TEST_ASSERT_EQUAL(22, lot52_node("r89_3")->h);
    TEST_ASSERT_EQUAL(78, lot52_node("r89_3")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_rows_px_90(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:24px;grid-template-rows:repeat(2,12px);row-gap:0px;align-items:stretch'><div id=r90_0></div><div id=r90_1></div></div></body>", 480);
    TEST_ASSERT_EQUAL(2, lot52_node("g")->grid_rows);
    TEST_ASSERT_EQUAL(12, lot52_node("r90_0")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("r90_0")->y);
    TEST_ASSERT_EQUAL(12, lot52_node("r90_1")->h);
    TEST_ASSERT_EQUAL(12, lot52_node("r90_1")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_rows_px_91(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:44px;grid-template-rows:repeat(3,14px);row-gap:1px;align-items:stretch'><div id=r91_0></div><div id=r91_1></div><div id=r91_2></div></div></body>", 480);
    TEST_ASSERT_EQUAL(3, lot52_node("g")->grid_rows);
    TEST_ASSERT_EQUAL(14, lot52_node("r91_0")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("r91_0")->y);
    TEST_ASSERT_EQUAL(14, lot52_node("r91_1")->h);
    TEST_ASSERT_EQUAL(15, lot52_node("r91_1")->y);
    TEST_ASSERT_EQUAL(14, lot52_node("r91_2")->h);
    TEST_ASSERT_EQUAL(30, lot52_node("r91_2")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_rows_px_92(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:70px;grid-template-rows:repeat(4,16px);row-gap:2px;align-items:stretch'><div id=r92_0></div><div id=r92_1></div><div id=r92_2></div><div id=r92_3></div></div></body>", 480);
    TEST_ASSERT_EQUAL(4, lot52_node("g")->grid_rows);
    TEST_ASSERT_EQUAL(16, lot52_node("r92_0")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("r92_0")->y);
    TEST_ASSERT_EQUAL(16, lot52_node("r92_1")->h);
    TEST_ASSERT_EQUAL(18, lot52_node("r92_1")->y);
    TEST_ASSERT_EQUAL(16, lot52_node("r92_2")->h);
    TEST_ASSERT_EQUAL(36, lot52_node("r92_2")->y);
    TEST_ASSERT_EQUAL(16, lot52_node("r92_3")->h);
    TEST_ASSERT_EQUAL(54, lot52_node("r92_3")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_rows_px_93(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:39px;grid-template-rows:repeat(2,18px);row-gap:3px;align-items:stretch'><div id=r93_0></div><div id=r93_1></div></div></body>", 480);
    TEST_ASSERT_EQUAL(2, lot52_node("g")->grid_rows);
    TEST_ASSERT_EQUAL(18, lot52_node("r93_0")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("r93_0")->y);
    TEST_ASSERT_EQUAL(18, lot52_node("r93_1")->h);
    TEST_ASSERT_EQUAL(21, lot52_node("r93_1")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_rows_px_94(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:68px;grid-template-rows:repeat(3,20px);row-gap:4px;align-items:stretch'><div id=r94_0></div><div id=r94_1></div><div id=r94_2></div></div></body>", 480);
    TEST_ASSERT_EQUAL(3, lot52_node("g")->grid_rows);
    TEST_ASSERT_EQUAL(20, lot52_node("r94_0")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("r94_0")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("r94_1")->h);
    TEST_ASSERT_EQUAL(24, lot52_node("r94_1")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("r94_2")->h);
    TEST_ASSERT_EQUAL(48, lot52_node("r94_2")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_rows_px_95(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:88px;grid-template-rows:repeat(4,22px);row-gap:0px;align-items:stretch'><div id=r95_0></div><div id=r95_1></div><div id=r95_2></div><div id=r95_3></div></div></body>", 480);
    TEST_ASSERT_EQUAL(4, lot52_node("g")->grid_rows);
    TEST_ASSERT_EQUAL(22, lot52_node("r95_0")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("r95_0")->y);
    TEST_ASSERT_EQUAL(22, lot52_node("r95_1")->h);
    TEST_ASSERT_EQUAL(22, lot52_node("r95_1")->y);
    TEST_ASSERT_EQUAL(22, lot52_node("r95_2")->h);
    TEST_ASSERT_EQUAL(44, lot52_node("r95_2")->y);
    TEST_ASSERT_EQUAL(22, lot52_node("r95_3")->h);
    TEST_ASSERT_EQUAL(66, lot52_node("r95_3")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_rows_px_96(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:25px;grid-template-rows:repeat(2,12px);row-gap:1px;align-items:stretch'><div id=r96_0></div><div id=r96_1></div></div></body>", 480);
    TEST_ASSERT_EQUAL(2, lot52_node("g")->grid_rows);
    TEST_ASSERT_EQUAL(12, lot52_node("r96_0")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("r96_0")->y);
    TEST_ASSERT_EQUAL(12, lot52_node("r96_1")->h);
    TEST_ASSERT_EQUAL(13, lot52_node("r96_1")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_rows_px_97(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:46px;grid-template-rows:repeat(3,14px);row-gap:2px;align-items:stretch'><div id=r97_0></div><div id=r97_1></div><div id=r97_2></div></div></body>", 480);
    TEST_ASSERT_EQUAL(3, lot52_node("g")->grid_rows);
    TEST_ASSERT_EQUAL(14, lot52_node("r97_0")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("r97_0")->y);
    TEST_ASSERT_EQUAL(14, lot52_node("r97_1")->h);
    TEST_ASSERT_EQUAL(16, lot52_node("r97_1")->y);
    TEST_ASSERT_EQUAL(14, lot52_node("r97_2")->h);
    TEST_ASSERT_EQUAL(32, lot52_node("r97_2")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_rows_px_98(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:73px;grid-template-rows:repeat(4,16px);row-gap:3px;align-items:stretch'><div id=r98_0></div><div id=r98_1></div><div id=r98_2></div><div id=r98_3></div></div></body>", 480);
    TEST_ASSERT_EQUAL(4, lot52_node("g")->grid_rows);
    TEST_ASSERT_EQUAL(16, lot52_node("r98_0")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("r98_0")->y);
    TEST_ASSERT_EQUAL(16, lot52_node("r98_1")->h);
    TEST_ASSERT_EQUAL(19, lot52_node("r98_1")->y);
    TEST_ASSERT_EQUAL(16, lot52_node("r98_2")->h);
    TEST_ASSERT_EQUAL(38, lot52_node("r98_2")->y);
    TEST_ASSERT_EQUAL(16, lot52_node("r98_3")->h);
    TEST_ASSERT_EQUAL(57, lot52_node("r98_3")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_rows_px_99(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:40px;grid-template-rows:repeat(2,18px);row-gap:4px;align-items:stretch'><div id=r99_0></div><div id=r99_1></div></div></body>", 480);
    TEST_ASSERT_EQUAL(2, lot52_node("g")->grid_rows);
    TEST_ASSERT_EQUAL(18, lot52_node("r99_0")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("r99_0")->y);
    TEST_ASSERT_EQUAL(18, lot52_node("r99_1")->h);
    TEST_ASSERT_EQUAL(22, lot52_node("r99_1")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_rows_px_100(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:60px;grid-template-rows:repeat(3,20px);row-gap:0px;align-items:stretch'><div id=r100_0></div><div id=r100_1></div><div id=r100_2></div></div></body>", 480);
    TEST_ASSERT_EQUAL(3, lot52_node("g")->grid_rows);
    TEST_ASSERT_EQUAL(20, lot52_node("r100_0")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("r100_0")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("r100_1")->h);
    TEST_ASSERT_EQUAL(20, lot52_node("r100_1")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("r100_2")->h);
    TEST_ASSERT_EQUAL(40, lot52_node("r100_2")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_rows_fr_1(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:90px;grid-template-rows:repeat(3,1fr);row-gap:1px;align-items:stretch'><div id=f1_0></div><div id=f1_1></div><div id=f1_2></div></div></body>", 480);
    TEST_ASSERT_EQUAL(30, lot52_node("f1_0")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("f1_0")->y);
    TEST_ASSERT_EQUAL(29, lot52_node("f1_1")->h);
    TEST_ASSERT_EQUAL(31, lot52_node("f1_1")->y);
    TEST_ASSERT_EQUAL(29, lot52_node("f1_2")->h);
    TEST_ASSERT_EQUAL(61, lot52_node("f1_2")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_rows_fr_2(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:100px;grid-template-rows:repeat(4,1fr);row-gap:2px;align-items:stretch'><div id=f2_0></div><div id=f2_1></div><div id=f2_2></div><div id=f2_3></div></div></body>", 480);
    TEST_ASSERT_EQUAL(24, lot52_node("f2_0")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("f2_0")->y);
    TEST_ASSERT_EQUAL(24, lot52_node("f2_1")->h);
    TEST_ASSERT_EQUAL(26, lot52_node("f2_1")->y);
    TEST_ASSERT_EQUAL(23, lot52_node("f2_2")->h);
    TEST_ASSERT_EQUAL(52, lot52_node("f2_2")->y);
    TEST_ASSERT_EQUAL(23, lot52_node("f2_3")->h);
    TEST_ASSERT_EQUAL(77, lot52_node("f2_3")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_rows_fr_3(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:110px;grid-template-rows:repeat(2,1fr);row-gap:3px;align-items:stretch'><div id=f3_0></div><div id=f3_1></div></div></body>", 480);
    TEST_ASSERT_EQUAL(54, lot52_node("f3_0")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("f3_0")->y);
    TEST_ASSERT_EQUAL(53, lot52_node("f3_1")->h);
    TEST_ASSERT_EQUAL(57, lot52_node("f3_1")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_rows_fr_4(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:120px;grid-template-rows:repeat(3,1fr);row-gap:0px;align-items:stretch'><div id=f4_0></div><div id=f4_1></div><div id=f4_2></div></div></body>", 480);
    TEST_ASSERT_EQUAL(40, lot52_node("f4_0")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("f4_0")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("f4_1")->h);
    TEST_ASSERT_EQUAL(40, lot52_node("f4_1")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("f4_2")->h);
    TEST_ASSERT_EQUAL(80, lot52_node("f4_2")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_rows_fr_5(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:130px;grid-template-rows:repeat(4,1fr);row-gap:1px;align-items:stretch'><div id=f5_0></div><div id=f5_1></div><div id=f5_2></div><div id=f5_3></div></div></body>", 480);
    TEST_ASSERT_EQUAL(32, lot52_node("f5_0")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("f5_0")->y);
    TEST_ASSERT_EQUAL(32, lot52_node("f5_1")->h);
    TEST_ASSERT_EQUAL(33, lot52_node("f5_1")->y);
    TEST_ASSERT_EQUAL(32, lot52_node("f5_2")->h);
    TEST_ASSERT_EQUAL(66, lot52_node("f5_2")->y);
    TEST_ASSERT_EQUAL(31, lot52_node("f5_3")->h);
    TEST_ASSERT_EQUAL(99, lot52_node("f5_3")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_rows_fr_6(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:80px;grid-template-rows:repeat(2,1fr);row-gap:2px;align-items:stretch'><div id=f6_0></div><div id=f6_1></div></div></body>", 480);
    TEST_ASSERT_EQUAL(39, lot52_node("f6_0")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("f6_0")->y);
    TEST_ASSERT_EQUAL(39, lot52_node("f6_1")->h);
    TEST_ASSERT_EQUAL(41, lot52_node("f6_1")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_rows_fr_7(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:90px;grid-template-rows:repeat(3,1fr);row-gap:3px;align-items:stretch'><div id=f7_0></div><div id=f7_1></div><div id=f7_2></div></div></body>", 480);
    TEST_ASSERT_EQUAL(28, lot52_node("f7_0")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("f7_0")->y);
    TEST_ASSERT_EQUAL(28, lot52_node("f7_1")->h);
    TEST_ASSERT_EQUAL(31, lot52_node("f7_1")->y);
    TEST_ASSERT_EQUAL(28, lot52_node("f7_2")->h);
    TEST_ASSERT_EQUAL(62, lot52_node("f7_2")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_rows_fr_8(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:100px;grid-template-rows:repeat(4,1fr);row-gap:0px;align-items:stretch'><div id=f8_0></div><div id=f8_1></div><div id=f8_2></div><div id=f8_3></div></div></body>", 480);
    TEST_ASSERT_EQUAL(25, lot52_node("f8_0")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("f8_0")->y);
    TEST_ASSERT_EQUAL(25, lot52_node("f8_1")->h);
    TEST_ASSERT_EQUAL(25, lot52_node("f8_1")->y);
    TEST_ASSERT_EQUAL(25, lot52_node("f8_2")->h);
    TEST_ASSERT_EQUAL(50, lot52_node("f8_2")->y);
    TEST_ASSERT_EQUAL(25, lot52_node("f8_3")->h);
    TEST_ASSERT_EQUAL(75, lot52_node("f8_3")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_rows_fr_9(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:110px;grid-template-rows:repeat(2,1fr);row-gap:1px;align-items:stretch'><div id=f9_0></div><div id=f9_1></div></div></body>", 480);
    TEST_ASSERT_EQUAL(55, lot52_node("f9_0")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("f9_0")->y);
    TEST_ASSERT_EQUAL(54, lot52_node("f9_1")->h);
    TEST_ASSERT_EQUAL(56, lot52_node("f9_1")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_rows_fr_10(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:120px;grid-template-rows:repeat(3,1fr);row-gap:2px;align-items:stretch'><div id=f10_0></div><div id=f10_1></div><div id=f10_2></div></div></body>", 480);
    TEST_ASSERT_EQUAL(39, lot52_node("f10_0")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("f10_0")->y);
    TEST_ASSERT_EQUAL(39, lot52_node("f10_1")->h);
    TEST_ASSERT_EQUAL(41, lot52_node("f10_1")->y);
    TEST_ASSERT_EQUAL(38, lot52_node("f10_2")->h);
    TEST_ASSERT_EQUAL(82, lot52_node("f10_2")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_rows_fr_11(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:130px;grid-template-rows:repeat(4,1fr);row-gap:3px;align-items:stretch'><div id=f11_0></div><div id=f11_1></div><div id=f11_2></div><div id=f11_3></div></div></body>", 480);
    TEST_ASSERT_EQUAL(31, lot52_node("f11_0")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("f11_0")->y);
    TEST_ASSERT_EQUAL(30, lot52_node("f11_1")->h);
    TEST_ASSERT_EQUAL(34, lot52_node("f11_1")->y);
    TEST_ASSERT_EQUAL(30, lot52_node("f11_2")->h);
    TEST_ASSERT_EQUAL(67, lot52_node("f11_2")->y);
    TEST_ASSERT_EQUAL(30, lot52_node("f11_3")->h);
    TEST_ASSERT_EQUAL(100, lot52_node("f11_3")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_rows_fr_12(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:80px;grid-template-rows:repeat(2,1fr);row-gap:0px;align-items:stretch'><div id=f12_0></div><div id=f12_1></div></div></body>", 480);
    TEST_ASSERT_EQUAL(40, lot52_node("f12_0")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("f12_0")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("f12_1")->h);
    TEST_ASSERT_EQUAL(40, lot52_node("f12_1")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_rows_fr_13(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:90px;grid-template-rows:repeat(3,1fr);row-gap:1px;align-items:stretch'><div id=f13_0></div><div id=f13_1></div><div id=f13_2></div></div></body>", 480);
    TEST_ASSERT_EQUAL(30, lot52_node("f13_0")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("f13_0")->y);
    TEST_ASSERT_EQUAL(29, lot52_node("f13_1")->h);
    TEST_ASSERT_EQUAL(31, lot52_node("f13_1")->y);
    TEST_ASSERT_EQUAL(29, lot52_node("f13_2")->h);
    TEST_ASSERT_EQUAL(61, lot52_node("f13_2")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_rows_fr_14(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:100px;grid-template-rows:repeat(4,1fr);row-gap:2px;align-items:stretch'><div id=f14_0></div><div id=f14_1></div><div id=f14_2></div><div id=f14_3></div></div></body>", 480);
    TEST_ASSERT_EQUAL(24, lot52_node("f14_0")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("f14_0")->y);
    TEST_ASSERT_EQUAL(24, lot52_node("f14_1")->h);
    TEST_ASSERT_EQUAL(26, lot52_node("f14_1")->y);
    TEST_ASSERT_EQUAL(23, lot52_node("f14_2")->h);
    TEST_ASSERT_EQUAL(52, lot52_node("f14_2")->y);
    TEST_ASSERT_EQUAL(23, lot52_node("f14_3")->h);
    TEST_ASSERT_EQUAL(77, lot52_node("f14_3")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_rows_fr_15(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:110px;grid-template-rows:repeat(2,1fr);row-gap:3px;align-items:stretch'><div id=f15_0></div><div id=f15_1></div></div></body>", 480);
    TEST_ASSERT_EQUAL(54, lot52_node("f15_0")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("f15_0")->y);
    TEST_ASSERT_EQUAL(53, lot52_node("f15_1")->h);
    TEST_ASSERT_EQUAL(57, lot52_node("f15_1")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_rows_fr_16(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:120px;grid-template-rows:repeat(3,1fr);row-gap:0px;align-items:stretch'><div id=f16_0></div><div id=f16_1></div><div id=f16_2></div></div></body>", 480);
    TEST_ASSERT_EQUAL(40, lot52_node("f16_0")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("f16_0")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("f16_1")->h);
    TEST_ASSERT_EQUAL(40, lot52_node("f16_1")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("f16_2")->h);
    TEST_ASSERT_EQUAL(80, lot52_node("f16_2")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_rows_fr_17(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:130px;grid-template-rows:repeat(4,1fr);row-gap:1px;align-items:stretch'><div id=f17_0></div><div id=f17_1></div><div id=f17_2></div><div id=f17_3></div></div></body>", 480);
    TEST_ASSERT_EQUAL(32, lot52_node("f17_0")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("f17_0")->y);
    TEST_ASSERT_EQUAL(32, lot52_node("f17_1")->h);
    TEST_ASSERT_EQUAL(33, lot52_node("f17_1")->y);
    TEST_ASSERT_EQUAL(32, lot52_node("f17_2")->h);
    TEST_ASSERT_EQUAL(66, lot52_node("f17_2")->y);
    TEST_ASSERT_EQUAL(31, lot52_node("f17_3")->h);
    TEST_ASSERT_EQUAL(99, lot52_node("f17_3")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_rows_fr_18(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:80px;grid-template-rows:repeat(2,1fr);row-gap:2px;align-items:stretch'><div id=f18_0></div><div id=f18_1></div></div></body>", 480);
    TEST_ASSERT_EQUAL(39, lot52_node("f18_0")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("f18_0")->y);
    TEST_ASSERT_EQUAL(39, lot52_node("f18_1")->h);
    TEST_ASSERT_EQUAL(41, lot52_node("f18_1")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_rows_fr_19(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:90px;grid-template-rows:repeat(3,1fr);row-gap:3px;align-items:stretch'><div id=f19_0></div><div id=f19_1></div><div id=f19_2></div></div></body>", 480);
    TEST_ASSERT_EQUAL(28, lot52_node("f19_0")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("f19_0")->y);
    TEST_ASSERT_EQUAL(28, lot52_node("f19_1")->h);
    TEST_ASSERT_EQUAL(31, lot52_node("f19_1")->y);
    TEST_ASSERT_EQUAL(28, lot52_node("f19_2")->h);
    TEST_ASSERT_EQUAL(62, lot52_node("f19_2")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_rows_fr_20(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:100px;grid-template-rows:repeat(4,1fr);row-gap:0px;align-items:stretch'><div id=f20_0></div><div id=f20_1></div><div id=f20_2></div><div id=f20_3></div></div></body>", 480);
    TEST_ASSERT_EQUAL(25, lot52_node("f20_0")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("f20_0")->y);
    TEST_ASSERT_EQUAL(25, lot52_node("f20_1")->h);
    TEST_ASSERT_EQUAL(25, lot52_node("f20_1")->y);
    TEST_ASSERT_EQUAL(25, lot52_node("f20_2")->h);
    TEST_ASSERT_EQUAL(50, lot52_node("f20_2")->y);
    TEST_ASSERT_EQUAL(25, lot52_node("f20_3")->h);
    TEST_ASSERT_EQUAL(75, lot52_node("f20_3")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_rows_fr_21(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:110px;grid-template-rows:repeat(2,1fr);row-gap:1px;align-items:stretch'><div id=f21_0></div><div id=f21_1></div></div></body>", 480);
    TEST_ASSERT_EQUAL(55, lot52_node("f21_0")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("f21_0")->y);
    TEST_ASSERT_EQUAL(54, lot52_node("f21_1")->h);
    TEST_ASSERT_EQUAL(56, lot52_node("f21_1")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_rows_fr_22(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:120px;grid-template-rows:repeat(3,1fr);row-gap:2px;align-items:stretch'><div id=f22_0></div><div id=f22_1></div><div id=f22_2></div></div></body>", 480);
    TEST_ASSERT_EQUAL(39, lot52_node("f22_0")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("f22_0")->y);
    TEST_ASSERT_EQUAL(39, lot52_node("f22_1")->h);
    TEST_ASSERT_EQUAL(41, lot52_node("f22_1")->y);
    TEST_ASSERT_EQUAL(38, lot52_node("f22_2")->h);
    TEST_ASSERT_EQUAL(82, lot52_node("f22_2")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_rows_fr_23(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:130px;grid-template-rows:repeat(4,1fr);row-gap:3px;align-items:stretch'><div id=f23_0></div><div id=f23_1></div><div id=f23_2></div><div id=f23_3></div></div></body>", 480);
    TEST_ASSERT_EQUAL(31, lot52_node("f23_0")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("f23_0")->y);
    TEST_ASSERT_EQUAL(30, lot52_node("f23_1")->h);
    TEST_ASSERT_EQUAL(34, lot52_node("f23_1")->y);
    TEST_ASSERT_EQUAL(30, lot52_node("f23_2")->h);
    TEST_ASSERT_EQUAL(67, lot52_node("f23_2")->y);
    TEST_ASSERT_EQUAL(30, lot52_node("f23_3")->h);
    TEST_ASSERT_EQUAL(100, lot52_node("f23_3")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_rows_fr_24(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:80px;grid-template-rows:repeat(2,1fr);row-gap:0px;align-items:stretch'><div id=f24_0></div><div id=f24_1></div></div></body>", 480);
    TEST_ASSERT_EQUAL(40, lot52_node("f24_0")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("f24_0")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("f24_1")->h);
    TEST_ASSERT_EQUAL(40, lot52_node("f24_1")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_rows_fr_25(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:90px;grid-template-rows:repeat(3,1fr);row-gap:1px;align-items:stretch'><div id=f25_0></div><div id=f25_1></div><div id=f25_2></div></div></body>", 480);
    TEST_ASSERT_EQUAL(30, lot52_node("f25_0")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("f25_0")->y);
    TEST_ASSERT_EQUAL(29, lot52_node("f25_1")->h);
    TEST_ASSERT_EQUAL(31, lot52_node("f25_1")->y);
    TEST_ASSERT_EQUAL(29, lot52_node("f25_2")->h);
    TEST_ASSERT_EQUAL(61, lot52_node("f25_2")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_rows_fr_26(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:100px;grid-template-rows:repeat(4,1fr);row-gap:2px;align-items:stretch'><div id=f26_0></div><div id=f26_1></div><div id=f26_2></div><div id=f26_3></div></div></body>", 480);
    TEST_ASSERT_EQUAL(24, lot52_node("f26_0")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("f26_0")->y);
    TEST_ASSERT_EQUAL(24, lot52_node("f26_1")->h);
    TEST_ASSERT_EQUAL(26, lot52_node("f26_1")->y);
    TEST_ASSERT_EQUAL(23, lot52_node("f26_2")->h);
    TEST_ASSERT_EQUAL(52, lot52_node("f26_2")->y);
    TEST_ASSERT_EQUAL(23, lot52_node("f26_3")->h);
    TEST_ASSERT_EQUAL(77, lot52_node("f26_3")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_rows_fr_27(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:110px;grid-template-rows:repeat(2,1fr);row-gap:3px;align-items:stretch'><div id=f27_0></div><div id=f27_1></div></div></body>", 480);
    TEST_ASSERT_EQUAL(54, lot52_node("f27_0")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("f27_0")->y);
    TEST_ASSERT_EQUAL(53, lot52_node("f27_1")->h);
    TEST_ASSERT_EQUAL(57, lot52_node("f27_1")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_rows_fr_28(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:120px;grid-template-rows:repeat(3,1fr);row-gap:0px;align-items:stretch'><div id=f28_0></div><div id=f28_1></div><div id=f28_2></div></div></body>", 480);
    TEST_ASSERT_EQUAL(40, lot52_node("f28_0")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("f28_0")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("f28_1")->h);
    TEST_ASSERT_EQUAL(40, lot52_node("f28_1")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("f28_2")->h);
    TEST_ASSERT_EQUAL(80, lot52_node("f28_2")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_rows_fr_29(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:130px;grid-template-rows:repeat(4,1fr);row-gap:1px;align-items:stretch'><div id=f29_0></div><div id=f29_1></div><div id=f29_2></div><div id=f29_3></div></div></body>", 480);
    TEST_ASSERT_EQUAL(32, lot52_node("f29_0")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("f29_0")->y);
    TEST_ASSERT_EQUAL(32, lot52_node("f29_1")->h);
    TEST_ASSERT_EQUAL(33, lot52_node("f29_1")->y);
    TEST_ASSERT_EQUAL(32, lot52_node("f29_2")->h);
    TEST_ASSERT_EQUAL(66, lot52_node("f29_2")->y);
    TEST_ASSERT_EQUAL(31, lot52_node("f29_3")->h);
    TEST_ASSERT_EQUAL(99, lot52_node("f29_3")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_rows_fr_30(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:80px;grid-template-rows:repeat(2,1fr);row-gap:2px;align-items:stretch'><div id=f30_0></div><div id=f30_1></div></div></body>", 480);
    TEST_ASSERT_EQUAL(39, lot52_node("f30_0")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("f30_0")->y);
    TEST_ASSERT_EQUAL(39, lot52_node("f30_1")->h);
    TEST_ASSERT_EQUAL(41, lot52_node("f30_1")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_rows_fr_31(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:90px;grid-template-rows:repeat(3,1fr);row-gap:3px;align-items:stretch'><div id=f31_0></div><div id=f31_1></div><div id=f31_2></div></div></body>", 480);
    TEST_ASSERT_EQUAL(28, lot52_node("f31_0")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("f31_0")->y);
    TEST_ASSERT_EQUAL(28, lot52_node("f31_1")->h);
    TEST_ASSERT_EQUAL(31, lot52_node("f31_1")->y);
    TEST_ASSERT_EQUAL(28, lot52_node("f31_2")->h);
    TEST_ASSERT_EQUAL(62, lot52_node("f31_2")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_rows_fr_32(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:100px;grid-template-rows:repeat(4,1fr);row-gap:0px;align-items:stretch'><div id=f32_0></div><div id=f32_1></div><div id=f32_2></div><div id=f32_3></div></div></body>", 480);
    TEST_ASSERT_EQUAL(25, lot52_node("f32_0")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("f32_0")->y);
    TEST_ASSERT_EQUAL(25, lot52_node("f32_1")->h);
    TEST_ASSERT_EQUAL(25, lot52_node("f32_1")->y);
    TEST_ASSERT_EQUAL(25, lot52_node("f32_2")->h);
    TEST_ASSERT_EQUAL(50, lot52_node("f32_2")->y);
    TEST_ASSERT_EQUAL(25, lot52_node("f32_3")->h);
    TEST_ASSERT_EQUAL(75, lot52_node("f32_3")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_rows_fr_33(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:110px;grid-template-rows:repeat(2,1fr);row-gap:1px;align-items:stretch'><div id=f33_0></div><div id=f33_1></div></div></body>", 480);
    TEST_ASSERT_EQUAL(55, lot52_node("f33_0")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("f33_0")->y);
    TEST_ASSERT_EQUAL(54, lot52_node("f33_1")->h);
    TEST_ASSERT_EQUAL(56, lot52_node("f33_1")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_rows_fr_34(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:120px;grid-template-rows:repeat(3,1fr);row-gap:2px;align-items:stretch'><div id=f34_0></div><div id=f34_1></div><div id=f34_2></div></div></body>", 480);
    TEST_ASSERT_EQUAL(39, lot52_node("f34_0")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("f34_0")->y);
    TEST_ASSERT_EQUAL(39, lot52_node("f34_1")->h);
    TEST_ASSERT_EQUAL(41, lot52_node("f34_1")->y);
    TEST_ASSERT_EQUAL(38, lot52_node("f34_2")->h);
    TEST_ASSERT_EQUAL(82, lot52_node("f34_2")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_rows_fr_35(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:130px;grid-template-rows:repeat(4,1fr);row-gap:3px;align-items:stretch'><div id=f35_0></div><div id=f35_1></div><div id=f35_2></div><div id=f35_3></div></div></body>", 480);
    TEST_ASSERT_EQUAL(31, lot52_node("f35_0")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("f35_0")->y);
    TEST_ASSERT_EQUAL(30, lot52_node("f35_1")->h);
    TEST_ASSERT_EQUAL(34, lot52_node("f35_1")->y);
    TEST_ASSERT_EQUAL(30, lot52_node("f35_2")->h);
    TEST_ASSERT_EQUAL(67, lot52_node("f35_2")->y);
    TEST_ASSERT_EQUAL(30, lot52_node("f35_3")->h);
    TEST_ASSERT_EQUAL(100, lot52_node("f35_3")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_rows_fr_36(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:80px;grid-template-rows:repeat(2,1fr);row-gap:0px;align-items:stretch'><div id=f36_0></div><div id=f36_1></div></div></body>", 480);
    TEST_ASSERT_EQUAL(40, lot52_node("f36_0")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("f36_0")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("f36_1")->h);
    TEST_ASSERT_EQUAL(40, lot52_node("f36_1")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_rows_fr_37(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:90px;grid-template-rows:repeat(3,1fr);row-gap:1px;align-items:stretch'><div id=f37_0></div><div id=f37_1></div><div id=f37_2></div></div></body>", 480);
    TEST_ASSERT_EQUAL(30, lot52_node("f37_0")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("f37_0")->y);
    TEST_ASSERT_EQUAL(29, lot52_node("f37_1")->h);
    TEST_ASSERT_EQUAL(31, lot52_node("f37_1")->y);
    TEST_ASSERT_EQUAL(29, lot52_node("f37_2")->h);
    TEST_ASSERT_EQUAL(61, lot52_node("f37_2")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_rows_fr_38(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:100px;grid-template-rows:repeat(4,1fr);row-gap:2px;align-items:stretch'><div id=f38_0></div><div id=f38_1></div><div id=f38_2></div><div id=f38_3></div></div></body>", 480);
    TEST_ASSERT_EQUAL(24, lot52_node("f38_0")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("f38_0")->y);
    TEST_ASSERT_EQUAL(24, lot52_node("f38_1")->h);
    TEST_ASSERT_EQUAL(26, lot52_node("f38_1")->y);
    TEST_ASSERT_EQUAL(23, lot52_node("f38_2")->h);
    TEST_ASSERT_EQUAL(52, lot52_node("f38_2")->y);
    TEST_ASSERT_EQUAL(23, lot52_node("f38_3")->h);
    TEST_ASSERT_EQUAL(77, lot52_node("f38_3")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_rows_fr_39(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:110px;grid-template-rows:repeat(2,1fr);row-gap:3px;align-items:stretch'><div id=f39_0></div><div id=f39_1></div></div></body>", 480);
    TEST_ASSERT_EQUAL(54, lot52_node("f39_0")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("f39_0")->y);
    TEST_ASSERT_EQUAL(53, lot52_node("f39_1")->h);
    TEST_ASSERT_EQUAL(57, lot52_node("f39_1")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_rows_fr_40(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:120px;grid-template-rows:repeat(3,1fr);row-gap:0px;align-items:stretch'><div id=f40_0></div><div id=f40_1></div><div id=f40_2></div></div></body>", 480);
    TEST_ASSERT_EQUAL(40, lot52_node("f40_0")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("f40_0")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("f40_1")->h);
    TEST_ASSERT_EQUAL(40, lot52_node("f40_1")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("f40_2")->h);
    TEST_ASSERT_EQUAL(80, lot52_node("f40_2")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_rows_fr_41(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:130px;grid-template-rows:repeat(4,1fr);row-gap:1px;align-items:stretch'><div id=f41_0></div><div id=f41_1></div><div id=f41_2></div><div id=f41_3></div></div></body>", 480);
    TEST_ASSERT_EQUAL(32, lot52_node("f41_0")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("f41_0")->y);
    TEST_ASSERT_EQUAL(32, lot52_node("f41_1")->h);
    TEST_ASSERT_EQUAL(33, lot52_node("f41_1")->y);
    TEST_ASSERT_EQUAL(32, lot52_node("f41_2")->h);
    TEST_ASSERT_EQUAL(66, lot52_node("f41_2")->y);
    TEST_ASSERT_EQUAL(31, lot52_node("f41_3")->h);
    TEST_ASSERT_EQUAL(99, lot52_node("f41_3")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_rows_fr_42(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:80px;grid-template-rows:repeat(2,1fr);row-gap:2px;align-items:stretch'><div id=f42_0></div><div id=f42_1></div></div></body>", 480);
    TEST_ASSERT_EQUAL(39, lot52_node("f42_0")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("f42_0")->y);
    TEST_ASSERT_EQUAL(39, lot52_node("f42_1")->h);
    TEST_ASSERT_EQUAL(41, lot52_node("f42_1")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_rows_fr_43(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:90px;grid-template-rows:repeat(3,1fr);row-gap:3px;align-items:stretch'><div id=f43_0></div><div id=f43_1></div><div id=f43_2></div></div></body>", 480);
    TEST_ASSERT_EQUAL(28, lot52_node("f43_0")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("f43_0")->y);
    TEST_ASSERT_EQUAL(28, lot52_node("f43_1")->h);
    TEST_ASSERT_EQUAL(31, lot52_node("f43_1")->y);
    TEST_ASSERT_EQUAL(28, lot52_node("f43_2")->h);
    TEST_ASSERT_EQUAL(62, lot52_node("f43_2")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_rows_fr_44(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:100px;grid-template-rows:repeat(4,1fr);row-gap:0px;align-items:stretch'><div id=f44_0></div><div id=f44_1></div><div id=f44_2></div><div id=f44_3></div></div></body>", 480);
    TEST_ASSERT_EQUAL(25, lot52_node("f44_0")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("f44_0")->y);
    TEST_ASSERT_EQUAL(25, lot52_node("f44_1")->h);
    TEST_ASSERT_EQUAL(25, lot52_node("f44_1")->y);
    TEST_ASSERT_EQUAL(25, lot52_node("f44_2")->h);
    TEST_ASSERT_EQUAL(50, lot52_node("f44_2")->y);
    TEST_ASSERT_EQUAL(25, lot52_node("f44_3")->h);
    TEST_ASSERT_EQUAL(75, lot52_node("f44_3")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_rows_fr_45(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:110px;grid-template-rows:repeat(2,1fr);row-gap:1px;align-items:stretch'><div id=f45_0></div><div id=f45_1></div></div></body>", 480);
    TEST_ASSERT_EQUAL(55, lot52_node("f45_0")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("f45_0")->y);
    TEST_ASSERT_EQUAL(54, lot52_node("f45_1")->h);
    TEST_ASSERT_EQUAL(56, lot52_node("f45_1")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_rows_fr_46(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:120px;grid-template-rows:repeat(3,1fr);row-gap:2px;align-items:stretch'><div id=f46_0></div><div id=f46_1></div><div id=f46_2></div></div></body>", 480);
    TEST_ASSERT_EQUAL(39, lot52_node("f46_0")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("f46_0")->y);
    TEST_ASSERT_EQUAL(39, lot52_node("f46_1")->h);
    TEST_ASSERT_EQUAL(41, lot52_node("f46_1")->y);
    TEST_ASSERT_EQUAL(38, lot52_node("f46_2")->h);
    TEST_ASSERT_EQUAL(82, lot52_node("f46_2")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_rows_fr_47(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:130px;grid-template-rows:repeat(4,1fr);row-gap:3px;align-items:stretch'><div id=f47_0></div><div id=f47_1></div><div id=f47_2></div><div id=f47_3></div></div></body>", 480);
    TEST_ASSERT_EQUAL(31, lot52_node("f47_0")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("f47_0")->y);
    TEST_ASSERT_EQUAL(30, lot52_node("f47_1")->h);
    TEST_ASSERT_EQUAL(34, lot52_node("f47_1")->y);
    TEST_ASSERT_EQUAL(30, lot52_node("f47_2")->h);
    TEST_ASSERT_EQUAL(67, lot52_node("f47_2")->y);
    TEST_ASSERT_EQUAL(30, lot52_node("f47_3")->h);
    TEST_ASSERT_EQUAL(100, lot52_node("f47_3")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_rows_fr_48(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:80px;grid-template-rows:repeat(2,1fr);row-gap:0px;align-items:stretch'><div id=f48_0></div><div id=f48_1></div></div></body>", 480);
    TEST_ASSERT_EQUAL(40, lot52_node("f48_0")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("f48_0")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("f48_1")->h);
    TEST_ASSERT_EQUAL(40, lot52_node("f48_1")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_rows_fr_49(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:90px;grid-template-rows:repeat(3,1fr);row-gap:1px;align-items:stretch'><div id=f49_0></div><div id=f49_1></div><div id=f49_2></div></div></body>", 480);
    TEST_ASSERT_EQUAL(30, lot52_node("f49_0")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("f49_0")->y);
    TEST_ASSERT_EQUAL(29, lot52_node("f49_1")->h);
    TEST_ASSERT_EQUAL(31, lot52_node("f49_1")->y);
    TEST_ASSERT_EQUAL(29, lot52_node("f49_2")->h);
    TEST_ASSERT_EQUAL(61, lot52_node("f49_2")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_rows_fr_50(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:100px;grid-template-rows:repeat(4,1fr);row-gap:2px;align-items:stretch'><div id=f50_0></div><div id=f50_1></div><div id=f50_2></div><div id=f50_3></div></div></body>", 480);
    TEST_ASSERT_EQUAL(24, lot52_node("f50_0")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("f50_0")->y);
    TEST_ASSERT_EQUAL(24, lot52_node("f50_1")->h);
    TEST_ASSERT_EQUAL(26, lot52_node("f50_1")->y);
    TEST_ASSERT_EQUAL(23, lot52_node("f50_2")->h);
    TEST_ASSERT_EQUAL(52, lot52_node("f50_2")->y);
    TEST_ASSERT_EQUAL(23, lot52_node("f50_3")->h);
    TEST_ASSERT_EQUAL(77, lot52_node("f50_3")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_rows_fr_51(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:110px;grid-template-rows:repeat(2,1fr);row-gap:3px;align-items:stretch'><div id=f51_0></div><div id=f51_1></div></div></body>", 480);
    TEST_ASSERT_EQUAL(54, lot52_node("f51_0")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("f51_0")->y);
    TEST_ASSERT_EQUAL(53, lot52_node("f51_1")->h);
    TEST_ASSERT_EQUAL(57, lot52_node("f51_1")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_rows_fr_52(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:120px;grid-template-rows:repeat(3,1fr);row-gap:0px;align-items:stretch'><div id=f52_0></div><div id=f52_1></div><div id=f52_2></div></div></body>", 480);
    TEST_ASSERT_EQUAL(40, lot52_node("f52_0")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("f52_0")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("f52_1")->h);
    TEST_ASSERT_EQUAL(40, lot52_node("f52_1")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("f52_2")->h);
    TEST_ASSERT_EQUAL(80, lot52_node("f52_2")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_rows_fr_53(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:130px;grid-template-rows:repeat(4,1fr);row-gap:1px;align-items:stretch'><div id=f53_0></div><div id=f53_1></div><div id=f53_2></div><div id=f53_3></div></div></body>", 480);
    TEST_ASSERT_EQUAL(32, lot52_node("f53_0")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("f53_0")->y);
    TEST_ASSERT_EQUAL(32, lot52_node("f53_1")->h);
    TEST_ASSERT_EQUAL(33, lot52_node("f53_1")->y);
    TEST_ASSERT_EQUAL(32, lot52_node("f53_2")->h);
    TEST_ASSERT_EQUAL(66, lot52_node("f53_2")->y);
    TEST_ASSERT_EQUAL(31, lot52_node("f53_3")->h);
    TEST_ASSERT_EQUAL(99, lot52_node("f53_3")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_rows_fr_54(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:80px;grid-template-rows:repeat(2,1fr);row-gap:2px;align-items:stretch'><div id=f54_0></div><div id=f54_1></div></div></body>", 480);
    TEST_ASSERT_EQUAL(39, lot52_node("f54_0")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("f54_0")->y);
    TEST_ASSERT_EQUAL(39, lot52_node("f54_1")->h);
    TEST_ASSERT_EQUAL(41, lot52_node("f54_1")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_rows_fr_55(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:90px;grid-template-rows:repeat(3,1fr);row-gap:3px;align-items:stretch'><div id=f55_0></div><div id=f55_1></div><div id=f55_2></div></div></body>", 480);
    TEST_ASSERT_EQUAL(28, lot52_node("f55_0")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("f55_0")->y);
    TEST_ASSERT_EQUAL(28, lot52_node("f55_1")->h);
    TEST_ASSERT_EQUAL(31, lot52_node("f55_1")->y);
    TEST_ASSERT_EQUAL(28, lot52_node("f55_2")->h);
    TEST_ASSERT_EQUAL(62, lot52_node("f55_2")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_rows_fr_56(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:100px;grid-template-rows:repeat(4,1fr);row-gap:0px;align-items:stretch'><div id=f56_0></div><div id=f56_1></div><div id=f56_2></div><div id=f56_3></div></div></body>", 480);
    TEST_ASSERT_EQUAL(25, lot52_node("f56_0")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("f56_0")->y);
    TEST_ASSERT_EQUAL(25, lot52_node("f56_1")->h);
    TEST_ASSERT_EQUAL(25, lot52_node("f56_1")->y);
    TEST_ASSERT_EQUAL(25, lot52_node("f56_2")->h);
    TEST_ASSERT_EQUAL(50, lot52_node("f56_2")->y);
    TEST_ASSERT_EQUAL(25, lot52_node("f56_3")->h);
    TEST_ASSERT_EQUAL(75, lot52_node("f56_3")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_rows_fr_57(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:110px;grid-template-rows:repeat(2,1fr);row-gap:1px;align-items:stretch'><div id=f57_0></div><div id=f57_1></div></div></body>", 480);
    TEST_ASSERT_EQUAL(55, lot52_node("f57_0")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("f57_0")->y);
    TEST_ASSERT_EQUAL(54, lot52_node("f57_1")->h);
    TEST_ASSERT_EQUAL(56, lot52_node("f57_1")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_rows_fr_58(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:120px;grid-template-rows:repeat(3,1fr);row-gap:2px;align-items:stretch'><div id=f58_0></div><div id=f58_1></div><div id=f58_2></div></div></body>", 480);
    TEST_ASSERT_EQUAL(39, lot52_node("f58_0")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("f58_0")->y);
    TEST_ASSERT_EQUAL(39, lot52_node("f58_1")->h);
    TEST_ASSERT_EQUAL(41, lot52_node("f58_1")->y);
    TEST_ASSERT_EQUAL(38, lot52_node("f58_2")->h);
    TEST_ASSERT_EQUAL(82, lot52_node("f58_2")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_rows_fr_59(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:130px;grid-template-rows:repeat(4,1fr);row-gap:3px;align-items:stretch'><div id=f59_0></div><div id=f59_1></div><div id=f59_2></div><div id=f59_3></div></div></body>", 480);
    TEST_ASSERT_EQUAL(31, lot52_node("f59_0")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("f59_0")->y);
    TEST_ASSERT_EQUAL(30, lot52_node("f59_1")->h);
    TEST_ASSERT_EQUAL(34, lot52_node("f59_1")->y);
    TEST_ASSERT_EQUAL(30, lot52_node("f59_2")->h);
    TEST_ASSERT_EQUAL(67, lot52_node("f59_2")->y);
    TEST_ASSERT_EQUAL(30, lot52_node("f59_3")->h);
    TEST_ASSERT_EQUAL(100, lot52_node("f59_3")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_rows_fr_60(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:80px;grid-template-rows:repeat(2,1fr);row-gap:0px;align-items:stretch'><div id=f60_0></div><div id=f60_1></div></div></body>", 480);
    TEST_ASSERT_EQUAL(40, lot52_node("f60_0")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("f60_0")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("f60_1")->h);
    TEST_ASSERT_EQUAL(40, lot52_node("f60_1")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_rows_fr_61(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:90px;grid-template-rows:repeat(3,1fr);row-gap:1px;align-items:stretch'><div id=f61_0></div><div id=f61_1></div><div id=f61_2></div></div></body>", 480);
    TEST_ASSERT_EQUAL(30, lot52_node("f61_0")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("f61_0")->y);
    TEST_ASSERT_EQUAL(29, lot52_node("f61_1")->h);
    TEST_ASSERT_EQUAL(31, lot52_node("f61_1")->y);
    TEST_ASSERT_EQUAL(29, lot52_node("f61_2")->h);
    TEST_ASSERT_EQUAL(61, lot52_node("f61_2")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_rows_fr_62(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:100px;grid-template-rows:repeat(4,1fr);row-gap:2px;align-items:stretch'><div id=f62_0></div><div id=f62_1></div><div id=f62_2></div><div id=f62_3></div></div></body>", 480);
    TEST_ASSERT_EQUAL(24, lot52_node("f62_0")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("f62_0")->y);
    TEST_ASSERT_EQUAL(24, lot52_node("f62_1")->h);
    TEST_ASSERT_EQUAL(26, lot52_node("f62_1")->y);
    TEST_ASSERT_EQUAL(23, lot52_node("f62_2")->h);
    TEST_ASSERT_EQUAL(52, lot52_node("f62_2")->y);
    TEST_ASSERT_EQUAL(23, lot52_node("f62_3")->h);
    TEST_ASSERT_EQUAL(77, lot52_node("f62_3")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_rows_fr_63(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:110px;grid-template-rows:repeat(2,1fr);row-gap:3px;align-items:stretch'><div id=f63_0></div><div id=f63_1></div></div></body>", 480);
    TEST_ASSERT_EQUAL(54, lot52_node("f63_0")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("f63_0")->y);
    TEST_ASSERT_EQUAL(53, lot52_node("f63_1")->h);
    TEST_ASSERT_EQUAL(57, lot52_node("f63_1")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_rows_fr_64(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:120px;grid-template-rows:repeat(3,1fr);row-gap:0px;align-items:stretch'><div id=f64_0></div><div id=f64_1></div><div id=f64_2></div></div></body>", 480);
    TEST_ASSERT_EQUAL(40, lot52_node("f64_0")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("f64_0")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("f64_1")->h);
    TEST_ASSERT_EQUAL(40, lot52_node("f64_1")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("f64_2")->h);
    TEST_ASSERT_EQUAL(80, lot52_node("f64_2")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_rows_fr_65(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:130px;grid-template-rows:repeat(4,1fr);row-gap:1px;align-items:stretch'><div id=f65_0></div><div id=f65_1></div><div id=f65_2></div><div id=f65_3></div></div></body>", 480);
    TEST_ASSERT_EQUAL(32, lot52_node("f65_0")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("f65_0")->y);
    TEST_ASSERT_EQUAL(32, lot52_node("f65_1")->h);
    TEST_ASSERT_EQUAL(33, lot52_node("f65_1")->y);
    TEST_ASSERT_EQUAL(32, lot52_node("f65_2")->h);
    TEST_ASSERT_EQUAL(66, lot52_node("f65_2")->y);
    TEST_ASSERT_EQUAL(31, lot52_node("f65_3")->h);
    TEST_ASSERT_EQUAL(99, lot52_node("f65_3")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_rows_fr_66(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:80px;grid-template-rows:repeat(2,1fr);row-gap:2px;align-items:stretch'><div id=f66_0></div><div id=f66_1></div></div></body>", 480);
    TEST_ASSERT_EQUAL(39, lot52_node("f66_0")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("f66_0")->y);
    TEST_ASSERT_EQUAL(39, lot52_node("f66_1")->h);
    TEST_ASSERT_EQUAL(41, lot52_node("f66_1")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_rows_fr_67(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:90px;grid-template-rows:repeat(3,1fr);row-gap:3px;align-items:stretch'><div id=f67_0></div><div id=f67_1></div><div id=f67_2></div></div></body>", 480);
    TEST_ASSERT_EQUAL(28, lot52_node("f67_0")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("f67_0")->y);
    TEST_ASSERT_EQUAL(28, lot52_node("f67_1")->h);
    TEST_ASSERT_EQUAL(31, lot52_node("f67_1")->y);
    TEST_ASSERT_EQUAL(28, lot52_node("f67_2")->h);
    TEST_ASSERT_EQUAL(62, lot52_node("f67_2")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_rows_fr_68(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:100px;grid-template-rows:repeat(4,1fr);row-gap:0px;align-items:stretch'><div id=f68_0></div><div id=f68_1></div><div id=f68_2></div><div id=f68_3></div></div></body>", 480);
    TEST_ASSERT_EQUAL(25, lot52_node("f68_0")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("f68_0")->y);
    TEST_ASSERT_EQUAL(25, lot52_node("f68_1")->h);
    TEST_ASSERT_EQUAL(25, lot52_node("f68_1")->y);
    TEST_ASSERT_EQUAL(25, lot52_node("f68_2")->h);
    TEST_ASSERT_EQUAL(50, lot52_node("f68_2")->y);
    TEST_ASSERT_EQUAL(25, lot52_node("f68_3")->h);
    TEST_ASSERT_EQUAL(75, lot52_node("f68_3")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_rows_fr_69(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:110px;grid-template-rows:repeat(2,1fr);row-gap:1px;align-items:stretch'><div id=f69_0></div><div id=f69_1></div></div></body>", 480);
    TEST_ASSERT_EQUAL(55, lot52_node("f69_0")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("f69_0")->y);
    TEST_ASSERT_EQUAL(54, lot52_node("f69_1")->h);
    TEST_ASSERT_EQUAL(56, lot52_node("f69_1")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_rows_fr_70(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:120px;grid-template-rows:repeat(3,1fr);row-gap:2px;align-items:stretch'><div id=f70_0></div><div id=f70_1></div><div id=f70_2></div></div></body>", 480);
    TEST_ASSERT_EQUAL(39, lot52_node("f70_0")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("f70_0")->y);
    TEST_ASSERT_EQUAL(39, lot52_node("f70_1")->h);
    TEST_ASSERT_EQUAL(41, lot52_node("f70_1")->y);
    TEST_ASSERT_EQUAL(38, lot52_node("f70_2")->h);
    TEST_ASSERT_EQUAL(82, lot52_node("f70_2")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_rows_fr_71(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:130px;grid-template-rows:repeat(4,1fr);row-gap:3px;align-items:stretch'><div id=f71_0></div><div id=f71_1></div><div id=f71_2></div><div id=f71_3></div></div></body>", 480);
    TEST_ASSERT_EQUAL(31, lot52_node("f71_0")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("f71_0")->y);
    TEST_ASSERT_EQUAL(30, lot52_node("f71_1")->h);
    TEST_ASSERT_EQUAL(34, lot52_node("f71_1")->y);
    TEST_ASSERT_EQUAL(30, lot52_node("f71_2")->h);
    TEST_ASSERT_EQUAL(67, lot52_node("f71_2")->y);
    TEST_ASSERT_EQUAL(30, lot52_node("f71_3")->h);
    TEST_ASSERT_EQUAL(100, lot52_node("f71_3")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_rows_fr_72(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:80px;grid-template-rows:repeat(2,1fr);row-gap:0px;align-items:stretch'><div id=f72_0></div><div id=f72_1></div></div></body>", 480);
    TEST_ASSERT_EQUAL(40, lot52_node("f72_0")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("f72_0")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("f72_1")->h);
    TEST_ASSERT_EQUAL(40, lot52_node("f72_1")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_rows_fr_73(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:90px;grid-template-rows:repeat(3,1fr);row-gap:1px;align-items:stretch'><div id=f73_0></div><div id=f73_1></div><div id=f73_2></div></div></body>", 480);
    TEST_ASSERT_EQUAL(30, lot52_node("f73_0")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("f73_0")->y);
    TEST_ASSERT_EQUAL(29, lot52_node("f73_1")->h);
    TEST_ASSERT_EQUAL(31, lot52_node("f73_1")->y);
    TEST_ASSERT_EQUAL(29, lot52_node("f73_2")->h);
    TEST_ASSERT_EQUAL(61, lot52_node("f73_2")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_rows_fr_74(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:100px;grid-template-rows:repeat(4,1fr);row-gap:2px;align-items:stretch'><div id=f74_0></div><div id=f74_1></div><div id=f74_2></div><div id=f74_3></div></div></body>", 480);
    TEST_ASSERT_EQUAL(24, lot52_node("f74_0")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("f74_0")->y);
    TEST_ASSERT_EQUAL(24, lot52_node("f74_1")->h);
    TEST_ASSERT_EQUAL(26, lot52_node("f74_1")->y);
    TEST_ASSERT_EQUAL(23, lot52_node("f74_2")->h);
    TEST_ASSERT_EQUAL(52, lot52_node("f74_2")->y);
    TEST_ASSERT_EQUAL(23, lot52_node("f74_3")->h);
    TEST_ASSERT_EQUAL(77, lot52_node("f74_3")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_rows_fr_75(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:110px;grid-template-rows:repeat(2,1fr);row-gap:3px;align-items:stretch'><div id=f75_0></div><div id=f75_1></div></div></body>", 480);
    TEST_ASSERT_EQUAL(54, lot52_node("f75_0")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("f75_0")->y);
    TEST_ASSERT_EQUAL(53, lot52_node("f75_1")->h);
    TEST_ASSERT_EQUAL(57, lot52_node("f75_1")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_rows_fr_76(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:120px;grid-template-rows:repeat(3,1fr);row-gap:0px;align-items:stretch'><div id=f76_0></div><div id=f76_1></div><div id=f76_2></div></div></body>", 480);
    TEST_ASSERT_EQUAL(40, lot52_node("f76_0")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("f76_0")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("f76_1")->h);
    TEST_ASSERT_EQUAL(40, lot52_node("f76_1")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("f76_2")->h);
    TEST_ASSERT_EQUAL(80, lot52_node("f76_2")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_rows_fr_77(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:130px;grid-template-rows:repeat(4,1fr);row-gap:1px;align-items:stretch'><div id=f77_0></div><div id=f77_1></div><div id=f77_2></div><div id=f77_3></div></div></body>", 480);
    TEST_ASSERT_EQUAL(32, lot52_node("f77_0")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("f77_0")->y);
    TEST_ASSERT_EQUAL(32, lot52_node("f77_1")->h);
    TEST_ASSERT_EQUAL(33, lot52_node("f77_1")->y);
    TEST_ASSERT_EQUAL(32, lot52_node("f77_2")->h);
    TEST_ASSERT_EQUAL(66, lot52_node("f77_2")->y);
    TEST_ASSERT_EQUAL(31, lot52_node("f77_3")->h);
    TEST_ASSERT_EQUAL(99, lot52_node("f77_3")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_rows_fr_78(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:80px;grid-template-rows:repeat(2,1fr);row-gap:2px;align-items:stretch'><div id=f78_0></div><div id=f78_1></div></div></body>", 480);
    TEST_ASSERT_EQUAL(39, lot52_node("f78_0")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("f78_0")->y);
    TEST_ASSERT_EQUAL(39, lot52_node("f78_1")->h);
    TEST_ASSERT_EQUAL(41, lot52_node("f78_1")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_rows_fr_79(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:90px;grid-template-rows:repeat(3,1fr);row-gap:3px;align-items:stretch'><div id=f79_0></div><div id=f79_1></div><div id=f79_2></div></div></body>", 480);
    TEST_ASSERT_EQUAL(28, lot52_node("f79_0")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("f79_0")->y);
    TEST_ASSERT_EQUAL(28, lot52_node("f79_1")->h);
    TEST_ASSERT_EQUAL(31, lot52_node("f79_1")->y);
    TEST_ASSERT_EQUAL(28, lot52_node("f79_2")->h);
    TEST_ASSERT_EQUAL(62, lot52_node("f79_2")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_rows_fr_80(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:100px;grid-template-rows:repeat(4,1fr);row-gap:0px;align-items:stretch'><div id=f80_0></div><div id=f80_1></div><div id=f80_2></div><div id=f80_3></div></div></body>", 480);
    TEST_ASSERT_EQUAL(25, lot52_node("f80_0")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("f80_0")->y);
    TEST_ASSERT_EQUAL(25, lot52_node("f80_1")->h);
    TEST_ASSERT_EQUAL(25, lot52_node("f80_1")->y);
    TEST_ASSERT_EQUAL(25, lot52_node("f80_2")->h);
    TEST_ASSERT_EQUAL(50, lot52_node("f80_2")->y);
    TEST_ASSERT_EQUAL(25, lot52_node("f80_3")->h);
    TEST_ASSERT_EQUAL(75, lot52_node("f80_3")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

/* Lot 78: grid-row span with explicit template rows (mutation-proved). */
static void test_l78_row_span_1(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:50px;grid-template-columns:20px 30px;grid-template-rows:8px 8px;row-gap:0px;align-items:stretch'><div id=s1_0 style='grid-column:1;grid-row:1 / span 2'></div><div id=s1_1 style='grid-column:2;grid-row:1'></div><div id=s1_2 style='grid-column:2;grid-row:2'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(16, lot52_node("s1_0")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("s1_0")->y);
    TEST_ASSERT_EQUAL(0, lot52_node("s1_1")->y);
    TEST_ASSERT_EQUAL(8, lot52_node("s1_1")->h);
    TEST_ASSERT_EQUAL(8, lot52_node("s1_2")->y);
    TEST_ASSERT_EQUAL(8, lot52_node("s1_2")->h);
    TEST_ASSERT_EQUAL(16, lot52_node("g")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_row_span_2(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:50px;grid-template-columns:20px 30px;grid-template-rows:8px 8px;row-gap:2px;align-items:stretch'><div id=s2_0 style='grid-column:1;grid-row:1 / span 2'></div><div id=s2_1 style='grid-column:2;grid-row:1'></div><div id=s2_2 style='grid-column:2;grid-row:2'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(18, lot52_node("s2_0")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("s2_0")->y);
    TEST_ASSERT_EQUAL(0, lot52_node("s2_1")->y);
    TEST_ASSERT_EQUAL(8, lot52_node("s2_1")->h);
    TEST_ASSERT_EQUAL(10, lot52_node("s2_2")->y);
    TEST_ASSERT_EQUAL(8, lot52_node("s2_2")->h);
    TEST_ASSERT_EQUAL(18, lot52_node("g")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_row_span_3(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:50px;grid-template-columns:20px 30px;grid-template-rows:8px 8px;row-gap:3px;align-items:stretch'><div id=s3_0 style='grid-column:1;grid-row:1 / span 2'></div><div id=s3_1 style='grid-column:2;grid-row:1'></div><div id=s3_2 style='grid-column:2;grid-row:2'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(19, lot52_node("s3_0")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("s3_0")->y);
    TEST_ASSERT_EQUAL(0, lot52_node("s3_1")->y);
    TEST_ASSERT_EQUAL(8, lot52_node("s3_1")->h);
    TEST_ASSERT_EQUAL(11, lot52_node("s3_2")->y);
    TEST_ASSERT_EQUAL(8, lot52_node("s3_2")->h);
    TEST_ASSERT_EQUAL(19, lot52_node("g")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_row_span_4(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:50px;grid-template-columns:20px 30px;grid-template-rows:8px 8px;row-gap:4px;align-items:stretch'><div id=s4_0 style='grid-column:1;grid-row:1 / span 2'></div><div id=s4_1 style='grid-column:2;grid-row:1'></div><div id=s4_2 style='grid-column:2;grid-row:2'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(20, lot52_node("s4_0")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("s4_0")->y);
    TEST_ASSERT_EQUAL(0, lot52_node("s4_1")->y);
    TEST_ASSERT_EQUAL(8, lot52_node("s4_1")->h);
    TEST_ASSERT_EQUAL(12, lot52_node("s4_2")->y);
    TEST_ASSERT_EQUAL(8, lot52_node("s4_2")->h);
    TEST_ASSERT_EQUAL(20, lot52_node("g")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_row_span_5(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:50px;grid-template-columns:20px 30px;grid-template-rows:8px 8px;row-gap:5px;align-items:stretch'><div id=s5_0 style='grid-column:1;grid-row:1 / span 2'></div><div id=s5_1 style='grid-column:2;grid-row:1'></div><div id=s5_2 style='grid-column:2;grid-row:2'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(21, lot52_node("s5_0")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("s5_0")->y);
    TEST_ASSERT_EQUAL(0, lot52_node("s5_1")->y);
    TEST_ASSERT_EQUAL(8, lot52_node("s5_1")->h);
    TEST_ASSERT_EQUAL(13, lot52_node("s5_2")->y);
    TEST_ASSERT_EQUAL(8, lot52_node("s5_2")->h);
    TEST_ASSERT_EQUAL(21, lot52_node("g")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_row_span_6(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:50px;grid-template-columns:20px 30px;grid-template-rows:8px 8px;row-gap:6px;align-items:stretch'><div id=s6_0 style='grid-column:1;grid-row:1 / span 2'></div><div id=s6_1 style='grid-column:2;grid-row:1'></div><div id=s6_2 style='grid-column:2;grid-row:2'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(22, lot52_node("s6_0")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("s6_0")->y);
    TEST_ASSERT_EQUAL(0, lot52_node("s6_1")->y);
    TEST_ASSERT_EQUAL(8, lot52_node("s6_1")->h);
    TEST_ASSERT_EQUAL(14, lot52_node("s6_2")->y);
    TEST_ASSERT_EQUAL(8, lot52_node("s6_2")->h);
    TEST_ASSERT_EQUAL(22, lot52_node("g")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_row_span_7(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:50px;grid-template-columns:20px 30px;grid-template-rows:10px 10px;row-gap:0px;align-items:stretch'><div id=s7_0 style='grid-column:1;grid-row:1 / span 2'></div><div id=s7_1 style='grid-column:2;grid-row:1'></div><div id=s7_2 style='grid-column:2;grid-row:2'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(20, lot52_node("s7_0")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("s7_0")->y);
    TEST_ASSERT_EQUAL(0, lot52_node("s7_1")->y);
    TEST_ASSERT_EQUAL(10, lot52_node("s7_1")->h);
    TEST_ASSERT_EQUAL(10, lot52_node("s7_2")->y);
    TEST_ASSERT_EQUAL(10, lot52_node("s7_2")->h);
    TEST_ASSERT_EQUAL(20, lot52_node("g")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_row_span_8(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:50px;grid-template-columns:20px 30px;grid-template-rows:10px 10px;row-gap:2px;align-items:stretch'><div id=s8_0 style='grid-column:1;grid-row:1 / span 2'></div><div id=s8_1 style='grid-column:2;grid-row:1'></div><div id=s8_2 style='grid-column:2;grid-row:2'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(22, lot52_node("s8_0")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("s8_0")->y);
    TEST_ASSERT_EQUAL(0, lot52_node("s8_1")->y);
    TEST_ASSERT_EQUAL(10, lot52_node("s8_1")->h);
    TEST_ASSERT_EQUAL(12, lot52_node("s8_2")->y);
    TEST_ASSERT_EQUAL(10, lot52_node("s8_2")->h);
    TEST_ASSERT_EQUAL(22, lot52_node("g")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_row_span_9(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:50px;grid-template-columns:20px 30px;grid-template-rows:10px 10px;row-gap:3px;align-items:stretch'><div id=s9_0 style='grid-column:1;grid-row:1 / span 2'></div><div id=s9_1 style='grid-column:2;grid-row:1'></div><div id=s9_2 style='grid-column:2;grid-row:2'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(23, lot52_node("s9_0")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("s9_0")->y);
    TEST_ASSERT_EQUAL(0, lot52_node("s9_1")->y);
    TEST_ASSERT_EQUAL(10, lot52_node("s9_1")->h);
    TEST_ASSERT_EQUAL(13, lot52_node("s9_2")->y);
    TEST_ASSERT_EQUAL(10, lot52_node("s9_2")->h);
    TEST_ASSERT_EQUAL(23, lot52_node("g")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_row_span_10(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:50px;grid-template-columns:20px 30px;grid-template-rows:10px 10px;row-gap:4px;align-items:stretch'><div id=s10_0 style='grid-column:1;grid-row:1 / span 2'></div><div id=s10_1 style='grid-column:2;grid-row:1'></div><div id=s10_2 style='grid-column:2;grid-row:2'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(24, lot52_node("s10_0")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("s10_0")->y);
    TEST_ASSERT_EQUAL(0, lot52_node("s10_1")->y);
    TEST_ASSERT_EQUAL(10, lot52_node("s10_1")->h);
    TEST_ASSERT_EQUAL(14, lot52_node("s10_2")->y);
    TEST_ASSERT_EQUAL(10, lot52_node("s10_2")->h);
    TEST_ASSERT_EQUAL(24, lot52_node("g")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_row_span_11(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:50px;grid-template-columns:20px 30px;grid-template-rows:10px 10px;row-gap:5px;align-items:stretch'><div id=s11_0 style='grid-column:1;grid-row:1 / span 2'></div><div id=s11_1 style='grid-column:2;grid-row:1'></div><div id=s11_2 style='grid-column:2;grid-row:2'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(25, lot52_node("s11_0")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("s11_0")->y);
    TEST_ASSERT_EQUAL(0, lot52_node("s11_1")->y);
    TEST_ASSERT_EQUAL(10, lot52_node("s11_1")->h);
    TEST_ASSERT_EQUAL(15, lot52_node("s11_2")->y);
    TEST_ASSERT_EQUAL(10, lot52_node("s11_2")->h);
    TEST_ASSERT_EQUAL(25, lot52_node("g")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_row_span_12(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:50px;grid-template-columns:20px 30px;grid-template-rows:10px 10px;row-gap:6px;align-items:stretch'><div id=s12_0 style='grid-column:1;grid-row:1 / span 2'></div><div id=s12_1 style='grid-column:2;grid-row:1'></div><div id=s12_2 style='grid-column:2;grid-row:2'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(26, lot52_node("s12_0")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("s12_0")->y);
    TEST_ASSERT_EQUAL(0, lot52_node("s12_1")->y);
    TEST_ASSERT_EQUAL(10, lot52_node("s12_1")->h);
    TEST_ASSERT_EQUAL(16, lot52_node("s12_2")->y);
    TEST_ASSERT_EQUAL(10, lot52_node("s12_2")->h);
    TEST_ASSERT_EQUAL(26, lot52_node("g")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_row_span_13(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:50px;grid-template-columns:20px 30px;grid-template-rows:12px 12px;row-gap:0px;align-items:stretch'><div id=s13_0 style='grid-column:1;grid-row:1 / span 2'></div><div id=s13_1 style='grid-column:2;grid-row:1'></div><div id=s13_2 style='grid-column:2;grid-row:2'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(24, lot52_node("s13_0")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("s13_0")->y);
    TEST_ASSERT_EQUAL(0, lot52_node("s13_1")->y);
    TEST_ASSERT_EQUAL(12, lot52_node("s13_1")->h);
    TEST_ASSERT_EQUAL(12, lot52_node("s13_2")->y);
    TEST_ASSERT_EQUAL(12, lot52_node("s13_2")->h);
    TEST_ASSERT_EQUAL(24, lot52_node("g")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_row_span_14(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:50px;grid-template-columns:20px 30px;grid-template-rows:12px 12px;row-gap:2px;align-items:stretch'><div id=s14_0 style='grid-column:1;grid-row:1 / span 2'></div><div id=s14_1 style='grid-column:2;grid-row:1'></div><div id=s14_2 style='grid-column:2;grid-row:2'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(26, lot52_node("s14_0")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("s14_0")->y);
    TEST_ASSERT_EQUAL(0, lot52_node("s14_1")->y);
    TEST_ASSERT_EQUAL(12, lot52_node("s14_1")->h);
    TEST_ASSERT_EQUAL(14, lot52_node("s14_2")->y);
    TEST_ASSERT_EQUAL(12, lot52_node("s14_2")->h);
    TEST_ASSERT_EQUAL(26, lot52_node("g")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_row_span_15(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:50px;grid-template-columns:20px 30px;grid-template-rows:12px 12px;row-gap:3px;align-items:stretch'><div id=s15_0 style='grid-column:1;grid-row:1 / span 2'></div><div id=s15_1 style='grid-column:2;grid-row:1'></div><div id=s15_2 style='grid-column:2;grid-row:2'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(27, lot52_node("s15_0")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("s15_0")->y);
    TEST_ASSERT_EQUAL(0, lot52_node("s15_1")->y);
    TEST_ASSERT_EQUAL(12, lot52_node("s15_1")->h);
    TEST_ASSERT_EQUAL(15, lot52_node("s15_2")->y);
    TEST_ASSERT_EQUAL(12, lot52_node("s15_2")->h);
    TEST_ASSERT_EQUAL(27, lot52_node("g")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_row_span_16(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:50px;grid-template-columns:20px 30px;grid-template-rows:12px 12px;row-gap:4px;align-items:stretch'><div id=s16_0 style='grid-column:1;grid-row:1 / span 2'></div><div id=s16_1 style='grid-column:2;grid-row:1'></div><div id=s16_2 style='grid-column:2;grid-row:2'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(28, lot52_node("s16_0")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("s16_0")->y);
    TEST_ASSERT_EQUAL(0, lot52_node("s16_1")->y);
    TEST_ASSERT_EQUAL(12, lot52_node("s16_1")->h);
    TEST_ASSERT_EQUAL(16, lot52_node("s16_2")->y);
    TEST_ASSERT_EQUAL(12, lot52_node("s16_2")->h);
    TEST_ASSERT_EQUAL(28, lot52_node("g")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_row_span_17(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:50px;grid-template-columns:20px 30px;grid-template-rows:12px 12px;row-gap:5px;align-items:stretch'><div id=s17_0 style='grid-column:1;grid-row:1 / span 2'></div><div id=s17_1 style='grid-column:2;grid-row:1'></div><div id=s17_2 style='grid-column:2;grid-row:2'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(29, lot52_node("s17_0")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("s17_0")->y);
    TEST_ASSERT_EQUAL(0, lot52_node("s17_1")->y);
    TEST_ASSERT_EQUAL(12, lot52_node("s17_1")->h);
    TEST_ASSERT_EQUAL(17, lot52_node("s17_2")->y);
    TEST_ASSERT_EQUAL(12, lot52_node("s17_2")->h);
    TEST_ASSERT_EQUAL(29, lot52_node("g")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_row_span_18(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:50px;grid-template-columns:20px 30px;grid-template-rows:12px 12px;row-gap:6px;align-items:stretch'><div id=s18_0 style='grid-column:1;grid-row:1 / span 2'></div><div id=s18_1 style='grid-column:2;grid-row:1'></div><div id=s18_2 style='grid-column:2;grid-row:2'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(30, lot52_node("s18_0")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("s18_0")->y);
    TEST_ASSERT_EQUAL(0, lot52_node("s18_1")->y);
    TEST_ASSERT_EQUAL(12, lot52_node("s18_1")->h);
    TEST_ASSERT_EQUAL(18, lot52_node("s18_2")->y);
    TEST_ASSERT_EQUAL(12, lot52_node("s18_2")->h);
    TEST_ASSERT_EQUAL(30, lot52_node("g")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_row_span_19(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:50px;grid-template-columns:20px 30px;grid-template-rows:14px 14px;row-gap:0px;align-items:stretch'><div id=s19_0 style='grid-column:1;grid-row:1 / span 2'></div><div id=s19_1 style='grid-column:2;grid-row:1'></div><div id=s19_2 style='grid-column:2;grid-row:2'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(28, lot52_node("s19_0")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("s19_0")->y);
    TEST_ASSERT_EQUAL(0, lot52_node("s19_1")->y);
    TEST_ASSERT_EQUAL(14, lot52_node("s19_1")->h);
    TEST_ASSERT_EQUAL(14, lot52_node("s19_2")->y);
    TEST_ASSERT_EQUAL(14, lot52_node("s19_2")->h);
    TEST_ASSERT_EQUAL(28, lot52_node("g")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_row_span_20(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:50px;grid-template-columns:20px 30px;grid-template-rows:14px 14px;row-gap:2px;align-items:stretch'><div id=s20_0 style='grid-column:1;grid-row:1 / span 2'></div><div id=s20_1 style='grid-column:2;grid-row:1'></div><div id=s20_2 style='grid-column:2;grid-row:2'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(30, lot52_node("s20_0")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("s20_0")->y);
    TEST_ASSERT_EQUAL(0, lot52_node("s20_1")->y);
    TEST_ASSERT_EQUAL(14, lot52_node("s20_1")->h);
    TEST_ASSERT_EQUAL(16, lot52_node("s20_2")->y);
    TEST_ASSERT_EQUAL(14, lot52_node("s20_2")->h);
    TEST_ASSERT_EQUAL(30, lot52_node("g")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_row_span_21(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:50px;grid-template-columns:20px 30px;grid-template-rows:14px 14px;row-gap:3px;align-items:stretch'><div id=s21_0 style='grid-column:1;grid-row:1 / span 2'></div><div id=s21_1 style='grid-column:2;grid-row:1'></div><div id=s21_2 style='grid-column:2;grid-row:2'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(31, lot52_node("s21_0")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("s21_0")->y);
    TEST_ASSERT_EQUAL(0, lot52_node("s21_1")->y);
    TEST_ASSERT_EQUAL(14, lot52_node("s21_1")->h);
    TEST_ASSERT_EQUAL(17, lot52_node("s21_2")->y);
    TEST_ASSERT_EQUAL(14, lot52_node("s21_2")->h);
    TEST_ASSERT_EQUAL(31, lot52_node("g")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_row_span_22(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:50px;grid-template-columns:20px 30px;grid-template-rows:14px 14px;row-gap:4px;align-items:stretch'><div id=s22_0 style='grid-column:1;grid-row:1 / span 2'></div><div id=s22_1 style='grid-column:2;grid-row:1'></div><div id=s22_2 style='grid-column:2;grid-row:2'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(32, lot52_node("s22_0")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("s22_0")->y);
    TEST_ASSERT_EQUAL(0, lot52_node("s22_1")->y);
    TEST_ASSERT_EQUAL(14, lot52_node("s22_1")->h);
    TEST_ASSERT_EQUAL(18, lot52_node("s22_2")->y);
    TEST_ASSERT_EQUAL(14, lot52_node("s22_2")->h);
    TEST_ASSERT_EQUAL(32, lot52_node("g")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_row_span_23(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:50px;grid-template-columns:20px 30px;grid-template-rows:14px 14px;row-gap:5px;align-items:stretch'><div id=s23_0 style='grid-column:1;grid-row:1 / span 2'></div><div id=s23_1 style='grid-column:2;grid-row:1'></div><div id=s23_2 style='grid-column:2;grid-row:2'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(33, lot52_node("s23_0")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("s23_0")->y);
    TEST_ASSERT_EQUAL(0, lot52_node("s23_1")->y);
    TEST_ASSERT_EQUAL(14, lot52_node("s23_1")->h);
    TEST_ASSERT_EQUAL(19, lot52_node("s23_2")->y);
    TEST_ASSERT_EQUAL(14, lot52_node("s23_2")->h);
    TEST_ASSERT_EQUAL(33, lot52_node("g")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_row_span_24(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:50px;grid-template-columns:20px 30px;grid-template-rows:14px 14px;row-gap:6px;align-items:stretch'><div id=s24_0 style='grid-column:1;grid-row:1 / span 2'></div><div id=s24_1 style='grid-column:2;grid-row:1'></div><div id=s24_2 style='grid-column:2;grid-row:2'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(34, lot52_node("s24_0")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("s24_0")->y);
    TEST_ASSERT_EQUAL(0, lot52_node("s24_1")->y);
    TEST_ASSERT_EQUAL(14, lot52_node("s24_1")->h);
    TEST_ASSERT_EQUAL(20, lot52_node("s24_2")->y);
    TEST_ASSERT_EQUAL(14, lot52_node("s24_2")->h);
    TEST_ASSERT_EQUAL(34, lot52_node("g")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_row_span_25(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:50px;grid-template-columns:20px 30px;grid-template-rows:16px 16px;row-gap:0px;align-items:stretch'><div id=s25_0 style='grid-column:1;grid-row:1 / span 2'></div><div id=s25_1 style='grid-column:2;grid-row:1'></div><div id=s25_2 style='grid-column:2;grid-row:2'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(32, lot52_node("s25_0")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("s25_0")->y);
    TEST_ASSERT_EQUAL(0, lot52_node("s25_1")->y);
    TEST_ASSERT_EQUAL(16, lot52_node("s25_1")->h);
    TEST_ASSERT_EQUAL(16, lot52_node("s25_2")->y);
    TEST_ASSERT_EQUAL(16, lot52_node("s25_2")->h);
    TEST_ASSERT_EQUAL(32, lot52_node("g")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_row_span_26(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:50px;grid-template-columns:20px 30px;grid-template-rows:16px 16px;row-gap:2px;align-items:stretch'><div id=s26_0 style='grid-column:1;grid-row:1 / span 2'></div><div id=s26_1 style='grid-column:2;grid-row:1'></div><div id=s26_2 style='grid-column:2;grid-row:2'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(34, lot52_node("s26_0")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("s26_0")->y);
    TEST_ASSERT_EQUAL(0, lot52_node("s26_1")->y);
    TEST_ASSERT_EQUAL(16, lot52_node("s26_1")->h);
    TEST_ASSERT_EQUAL(18, lot52_node("s26_2")->y);
    TEST_ASSERT_EQUAL(16, lot52_node("s26_2")->h);
    TEST_ASSERT_EQUAL(34, lot52_node("g")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_row_span_27(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:50px;grid-template-columns:20px 30px;grid-template-rows:16px 16px;row-gap:3px;align-items:stretch'><div id=s27_0 style='grid-column:1;grid-row:1 / span 2'></div><div id=s27_1 style='grid-column:2;grid-row:1'></div><div id=s27_2 style='grid-column:2;grid-row:2'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(35, lot52_node("s27_0")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("s27_0")->y);
    TEST_ASSERT_EQUAL(0, lot52_node("s27_1")->y);
    TEST_ASSERT_EQUAL(16, lot52_node("s27_1")->h);
    TEST_ASSERT_EQUAL(19, lot52_node("s27_2")->y);
    TEST_ASSERT_EQUAL(16, lot52_node("s27_2")->h);
    TEST_ASSERT_EQUAL(35, lot52_node("g")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_row_span_28(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:50px;grid-template-columns:20px 30px;grid-template-rows:16px 16px;row-gap:4px;align-items:stretch'><div id=s28_0 style='grid-column:1;grid-row:1 / span 2'></div><div id=s28_1 style='grid-column:2;grid-row:1'></div><div id=s28_2 style='grid-column:2;grid-row:2'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(36, lot52_node("s28_0")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("s28_0")->y);
    TEST_ASSERT_EQUAL(0, lot52_node("s28_1")->y);
    TEST_ASSERT_EQUAL(16, lot52_node("s28_1")->h);
    TEST_ASSERT_EQUAL(20, lot52_node("s28_2")->y);
    TEST_ASSERT_EQUAL(16, lot52_node("s28_2")->h);
    TEST_ASSERT_EQUAL(36, lot52_node("g")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_row_span_29(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:50px;grid-template-columns:20px 30px;grid-template-rows:16px 16px;row-gap:5px;align-items:stretch'><div id=s29_0 style='grid-column:1;grid-row:1 / span 2'></div><div id=s29_1 style='grid-column:2;grid-row:1'></div><div id=s29_2 style='grid-column:2;grid-row:2'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(37, lot52_node("s29_0")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("s29_0")->y);
    TEST_ASSERT_EQUAL(0, lot52_node("s29_1")->y);
    TEST_ASSERT_EQUAL(16, lot52_node("s29_1")->h);
    TEST_ASSERT_EQUAL(21, lot52_node("s29_2")->y);
    TEST_ASSERT_EQUAL(16, lot52_node("s29_2")->h);
    TEST_ASSERT_EQUAL(37, lot52_node("g")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_row_span_30(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:50px;grid-template-columns:20px 30px;grid-template-rows:16px 16px;row-gap:6px;align-items:stretch'><div id=s30_0 style='grid-column:1;grid-row:1 / span 2'></div><div id=s30_1 style='grid-column:2;grid-row:1'></div><div id=s30_2 style='grid-column:2;grid-row:2'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(38, lot52_node("s30_0")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("s30_0")->y);
    TEST_ASSERT_EQUAL(0, lot52_node("s30_1")->y);
    TEST_ASSERT_EQUAL(16, lot52_node("s30_1")->h);
    TEST_ASSERT_EQUAL(22, lot52_node("s30_2")->y);
    TEST_ASSERT_EQUAL(16, lot52_node("s30_2")->h);
    TEST_ASSERT_EQUAL(38, lot52_node("g")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_row_span_31(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:50px;grid-template-columns:20px 30px;grid-template-rows:18px 18px;row-gap:0px;align-items:stretch'><div id=s31_0 style='grid-column:1;grid-row:1 / span 2'></div><div id=s31_1 style='grid-column:2;grid-row:1'></div><div id=s31_2 style='grid-column:2;grid-row:2'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(36, lot52_node("s31_0")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("s31_0")->y);
    TEST_ASSERT_EQUAL(0, lot52_node("s31_1")->y);
    TEST_ASSERT_EQUAL(18, lot52_node("s31_1")->h);
    TEST_ASSERT_EQUAL(18, lot52_node("s31_2")->y);
    TEST_ASSERT_EQUAL(18, lot52_node("s31_2")->h);
    TEST_ASSERT_EQUAL(36, lot52_node("g")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_row_span_32(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:50px;grid-template-columns:20px 30px;grid-template-rows:18px 18px;row-gap:2px;align-items:stretch'><div id=s32_0 style='grid-column:1;grid-row:1 / span 2'></div><div id=s32_1 style='grid-column:2;grid-row:1'></div><div id=s32_2 style='grid-column:2;grid-row:2'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(38, lot52_node("s32_0")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("s32_0")->y);
    TEST_ASSERT_EQUAL(0, lot52_node("s32_1")->y);
    TEST_ASSERT_EQUAL(18, lot52_node("s32_1")->h);
    TEST_ASSERT_EQUAL(20, lot52_node("s32_2")->y);
    TEST_ASSERT_EQUAL(18, lot52_node("s32_2")->h);
    TEST_ASSERT_EQUAL(38, lot52_node("g")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_row_span_33(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:50px;grid-template-columns:20px 30px;grid-template-rows:18px 18px;row-gap:3px;align-items:stretch'><div id=s33_0 style='grid-column:1;grid-row:1 / span 2'></div><div id=s33_1 style='grid-column:2;grid-row:1'></div><div id=s33_2 style='grid-column:2;grid-row:2'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(39, lot52_node("s33_0")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("s33_0")->y);
    TEST_ASSERT_EQUAL(0, lot52_node("s33_1")->y);
    TEST_ASSERT_EQUAL(18, lot52_node("s33_1")->h);
    TEST_ASSERT_EQUAL(21, lot52_node("s33_2")->y);
    TEST_ASSERT_EQUAL(18, lot52_node("s33_2")->h);
    TEST_ASSERT_EQUAL(39, lot52_node("g")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_row_span_34(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:50px;grid-template-columns:20px 30px;grid-template-rows:18px 18px;row-gap:4px;align-items:stretch'><div id=s34_0 style='grid-column:1;grid-row:1 / span 2'></div><div id=s34_1 style='grid-column:2;grid-row:1'></div><div id=s34_2 style='grid-column:2;grid-row:2'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(40, lot52_node("s34_0")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("s34_0")->y);
    TEST_ASSERT_EQUAL(0, lot52_node("s34_1")->y);
    TEST_ASSERT_EQUAL(18, lot52_node("s34_1")->h);
    TEST_ASSERT_EQUAL(22, lot52_node("s34_2")->y);
    TEST_ASSERT_EQUAL(18, lot52_node("s34_2")->h);
    TEST_ASSERT_EQUAL(40, lot52_node("g")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_row_span_35(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:50px;grid-template-columns:20px 30px;grid-template-rows:18px 18px;row-gap:5px;align-items:stretch'><div id=s35_0 style='grid-column:1;grid-row:1 / span 2'></div><div id=s35_1 style='grid-column:2;grid-row:1'></div><div id=s35_2 style='grid-column:2;grid-row:2'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(41, lot52_node("s35_0")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("s35_0")->y);
    TEST_ASSERT_EQUAL(0, lot52_node("s35_1")->y);
    TEST_ASSERT_EQUAL(18, lot52_node("s35_1")->h);
    TEST_ASSERT_EQUAL(23, lot52_node("s35_2")->y);
    TEST_ASSERT_EQUAL(18, lot52_node("s35_2")->h);
    TEST_ASSERT_EQUAL(41, lot52_node("g")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_row_span_36(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:50px;grid-template-columns:20px 30px;grid-template-rows:18px 18px;row-gap:6px;align-items:stretch'><div id=s36_0 style='grid-column:1;grid-row:1 / span 2'></div><div id=s36_1 style='grid-column:2;grid-row:1'></div><div id=s36_2 style='grid-column:2;grid-row:2'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(42, lot52_node("s36_0")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("s36_0")->y);
    TEST_ASSERT_EQUAL(0, lot52_node("s36_1")->y);
    TEST_ASSERT_EQUAL(18, lot52_node("s36_1")->h);
    TEST_ASSERT_EQUAL(24, lot52_node("s36_2")->y);
    TEST_ASSERT_EQUAL(18, lot52_node("s36_2")->h);
    TEST_ASSERT_EQUAL(42, lot52_node("g")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_row_span_37(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:50px;grid-template-columns:20px 30px;grid-template-rows:20px 20px;row-gap:0px;align-items:stretch'><div id=s37_0 style='grid-column:1;grid-row:1 / span 2'></div><div id=s37_1 style='grid-column:2;grid-row:1'></div><div id=s37_2 style='grid-column:2;grid-row:2'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(40, lot52_node("s37_0")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("s37_0")->y);
    TEST_ASSERT_EQUAL(0, lot52_node("s37_1")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("s37_1")->h);
    TEST_ASSERT_EQUAL(20, lot52_node("s37_2")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("s37_2")->h);
    TEST_ASSERT_EQUAL(40, lot52_node("g")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_row_span_38(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:50px;grid-template-columns:20px 30px;grid-template-rows:20px 20px;row-gap:2px;align-items:stretch'><div id=s38_0 style='grid-column:1;grid-row:1 / span 2'></div><div id=s38_1 style='grid-column:2;grid-row:1'></div><div id=s38_2 style='grid-column:2;grid-row:2'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(42, lot52_node("s38_0")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("s38_0")->y);
    TEST_ASSERT_EQUAL(0, lot52_node("s38_1")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("s38_1")->h);
    TEST_ASSERT_EQUAL(22, lot52_node("s38_2")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("s38_2")->h);
    TEST_ASSERT_EQUAL(42, lot52_node("g")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_row_span_39(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:50px;grid-template-columns:20px 30px;grid-template-rows:20px 20px;row-gap:3px;align-items:stretch'><div id=s39_0 style='grid-column:1;grid-row:1 / span 2'></div><div id=s39_1 style='grid-column:2;grid-row:1'></div><div id=s39_2 style='grid-column:2;grid-row:2'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(43, lot52_node("s39_0")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("s39_0")->y);
    TEST_ASSERT_EQUAL(0, lot52_node("s39_1")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("s39_1")->h);
    TEST_ASSERT_EQUAL(23, lot52_node("s39_2")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("s39_2")->h);
    TEST_ASSERT_EQUAL(43, lot52_node("g")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_row_span_40(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:50px;grid-template-columns:20px 30px;grid-template-rows:20px 20px;row-gap:4px;align-items:stretch'><div id=s40_0 style='grid-column:1;grid-row:1 / span 2'></div><div id=s40_1 style='grid-column:2;grid-row:1'></div><div id=s40_2 style='grid-column:2;grid-row:2'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(44, lot52_node("s40_0")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("s40_0")->y);
    TEST_ASSERT_EQUAL(0, lot52_node("s40_1")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("s40_1")->h);
    TEST_ASSERT_EQUAL(24, lot52_node("s40_2")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("s40_2")->h);
    TEST_ASSERT_EQUAL(44, lot52_node("g")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_row_span_41(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:50px;grid-template-columns:20px 30px;grid-template-rows:20px 20px;row-gap:5px;align-items:stretch'><div id=s41_0 style='grid-column:1;grid-row:1 / span 2'></div><div id=s41_1 style='grid-column:2;grid-row:1'></div><div id=s41_2 style='grid-column:2;grid-row:2'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(45, lot52_node("s41_0")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("s41_0")->y);
    TEST_ASSERT_EQUAL(0, lot52_node("s41_1")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("s41_1")->h);
    TEST_ASSERT_EQUAL(25, lot52_node("s41_2")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("s41_2")->h);
    TEST_ASSERT_EQUAL(45, lot52_node("g")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_row_span_42(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:50px;grid-template-columns:20px 30px;grid-template-rows:20px 20px;row-gap:6px;align-items:stretch'><div id=s42_0 style='grid-column:1;grid-row:1 / span 2'></div><div id=s42_1 style='grid-column:2;grid-row:1'></div><div id=s42_2 style='grid-column:2;grid-row:2'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(46, lot52_node("s42_0")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("s42_0")->y);
    TEST_ASSERT_EQUAL(0, lot52_node("s42_1")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("s42_1")->h);
    TEST_ASSERT_EQUAL(26, lot52_node("s42_2")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("s42_2")->h);
    TEST_ASSERT_EQUAL(46, lot52_node("g")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_row_span_43(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:50px;grid-template-columns:20px 30px;grid-template-rows:24px 24px;row-gap:0px;align-items:stretch'><div id=s43_0 style='grid-column:1;grid-row:1 / span 2'></div><div id=s43_1 style='grid-column:2;grid-row:1'></div><div id=s43_2 style='grid-column:2;grid-row:2'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(48, lot52_node("s43_0")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("s43_0")->y);
    TEST_ASSERT_EQUAL(0, lot52_node("s43_1")->y);
    TEST_ASSERT_EQUAL(24, lot52_node("s43_1")->h);
    TEST_ASSERT_EQUAL(24, lot52_node("s43_2")->y);
    TEST_ASSERT_EQUAL(24, lot52_node("s43_2")->h);
    TEST_ASSERT_EQUAL(48, lot52_node("g")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_row_span_44(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:50px;grid-template-columns:20px 30px;grid-template-rows:24px 24px;row-gap:2px;align-items:stretch'><div id=s44_0 style='grid-column:1;grid-row:1 / span 2'></div><div id=s44_1 style='grid-column:2;grid-row:1'></div><div id=s44_2 style='grid-column:2;grid-row:2'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(50, lot52_node("s44_0")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("s44_0")->y);
    TEST_ASSERT_EQUAL(0, lot52_node("s44_1")->y);
    TEST_ASSERT_EQUAL(24, lot52_node("s44_1")->h);
    TEST_ASSERT_EQUAL(26, lot52_node("s44_2")->y);
    TEST_ASSERT_EQUAL(24, lot52_node("s44_2")->h);
    TEST_ASSERT_EQUAL(50, lot52_node("g")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_row_span_45(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:50px;grid-template-columns:20px 30px;grid-template-rows:24px 24px;row-gap:3px;align-items:stretch'><div id=s45_0 style='grid-column:1;grid-row:1 / span 2'></div><div id=s45_1 style='grid-column:2;grid-row:1'></div><div id=s45_2 style='grid-column:2;grid-row:2'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(51, lot52_node("s45_0")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("s45_0")->y);
    TEST_ASSERT_EQUAL(0, lot52_node("s45_1")->y);
    TEST_ASSERT_EQUAL(24, lot52_node("s45_1")->h);
    TEST_ASSERT_EQUAL(27, lot52_node("s45_2")->y);
    TEST_ASSERT_EQUAL(24, lot52_node("s45_2")->h);
    TEST_ASSERT_EQUAL(51, lot52_node("g")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_row_span_46(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:50px;grid-template-columns:20px 30px;grid-template-rows:24px 24px;row-gap:4px;align-items:stretch'><div id=s46_0 style='grid-column:1;grid-row:1 / span 2'></div><div id=s46_1 style='grid-column:2;grid-row:1'></div><div id=s46_2 style='grid-column:2;grid-row:2'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(52, lot52_node("s46_0")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("s46_0")->y);
    TEST_ASSERT_EQUAL(0, lot52_node("s46_1")->y);
    TEST_ASSERT_EQUAL(24, lot52_node("s46_1")->h);
    TEST_ASSERT_EQUAL(28, lot52_node("s46_2")->y);
    TEST_ASSERT_EQUAL(24, lot52_node("s46_2")->h);
    TEST_ASSERT_EQUAL(52, lot52_node("g")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_row_span_47(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:50px;grid-template-columns:20px 30px;grid-template-rows:24px 24px;row-gap:5px;align-items:stretch'><div id=s47_0 style='grid-column:1;grid-row:1 / span 2'></div><div id=s47_1 style='grid-column:2;grid-row:1'></div><div id=s47_2 style='grid-column:2;grid-row:2'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(53, lot52_node("s47_0")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("s47_0")->y);
    TEST_ASSERT_EQUAL(0, lot52_node("s47_1")->y);
    TEST_ASSERT_EQUAL(24, lot52_node("s47_1")->h);
    TEST_ASSERT_EQUAL(29, lot52_node("s47_2")->y);
    TEST_ASSERT_EQUAL(24, lot52_node("s47_2")->h);
    TEST_ASSERT_EQUAL(53, lot52_node("g")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_row_span_48(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:50px;grid-template-columns:20px 30px;grid-template-rows:24px 24px;row-gap:6px;align-items:stretch'><div id=s48_0 style='grid-column:1;grid-row:1 / span 2'></div><div id=s48_1 style='grid-column:2;grid-row:1'></div><div id=s48_2 style='grid-column:2;grid-row:2'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(54, lot52_node("s48_0")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("s48_0")->y);
    TEST_ASSERT_EQUAL(0, lot52_node("s48_1")->y);
    TEST_ASSERT_EQUAL(24, lot52_node("s48_1")->h);
    TEST_ASSERT_EQUAL(30, lot52_node("s48_2")->y);
    TEST_ASSERT_EQUAL(24, lot52_node("s48_2")->h);
    TEST_ASSERT_EQUAL(54, lot52_node("g")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_row_span_49(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:50px;grid-template-columns:20px 30px;grid-template-rows:10px 20px;row-gap:4px;align-items:stretch'><div id=s49_0 style='grid-column:1;grid-row:1 / span 2'></div><div id=s49_1 style='grid-column:2;grid-row:1'></div><div id=s49_2 style='grid-column:2;grid-row:2'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(34, lot52_node("s49_0")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("s49_0")->y);
    TEST_ASSERT_EQUAL(0, lot52_node("s49_1")->y);
    TEST_ASSERT_EQUAL(10, lot52_node("s49_1")->h);
    TEST_ASSERT_EQUAL(14, lot52_node("s49_2")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("s49_2")->h);
    TEST_ASSERT_EQUAL(34, lot52_node("g")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_row_span_50(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:50px;grid-template-columns:20px 30px;grid-template-rows:20px 10px;row-gap:4px;align-items:stretch'><div id=s50_0 style='grid-column:1;grid-row:1 / span 2'></div><div id=s50_1 style='grid-column:2;grid-row:1'></div><div id=s50_2 style='grid-column:2;grid-row:2'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(34, lot52_node("s50_0")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("s50_0")->y);
    TEST_ASSERT_EQUAL(0, lot52_node("s50_1")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("s50_1")->h);
    TEST_ASSERT_EQUAL(24, lot52_node("s50_2")->y);
    TEST_ASSERT_EQUAL(10, lot52_node("s50_2")->h);
    TEST_ASSERT_EQUAL(34, lot52_node("g")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_row_span_51(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:50px;grid-template-columns:20px 30px;grid-template-rows:12px 18px;row-gap:3px;align-items:stretch'><div id=s51_0 style='grid-column:1;grid-row:1 / span 2'></div><div id=s51_1 style='grid-column:2;grid-row:1'></div><div id=s51_2 style='grid-column:2;grid-row:2'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(33, lot52_node("s51_0")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("s51_0")->y);
    TEST_ASSERT_EQUAL(0, lot52_node("s51_1")->y);
    TEST_ASSERT_EQUAL(12, lot52_node("s51_1")->h);
    TEST_ASSERT_EQUAL(15, lot52_node("s51_2")->y);
    TEST_ASSERT_EQUAL(18, lot52_node("s51_2")->h);
    TEST_ASSERT_EQUAL(33, lot52_node("g")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_row_span_52(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:50px;grid-template-columns:20px 30px;grid-template-rows:15px 25px;row-gap:5px;align-items:stretch'><div id=s52_0 style='grid-column:1;grid-row:1 / span 2'></div><div id=s52_1 style='grid-column:2;grid-row:1'></div><div id=s52_2 style='grid-column:2;grid-row:2'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(45, lot52_node("s52_0")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("s52_0")->y);
    TEST_ASSERT_EQUAL(0, lot52_node("s52_1")->y);
    TEST_ASSERT_EQUAL(15, lot52_node("s52_1")->h);
    TEST_ASSERT_EQUAL(20, lot52_node("s52_2")->y);
    TEST_ASSERT_EQUAL(25, lot52_node("s52_2")->h);
    TEST_ASSERT_EQUAL(45, lot52_node("g")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_row_span_53(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:50px;grid-template-columns:20px 30px;grid-template-rows:8px 16px;row-gap:2px;align-items:stretch'><div id=s53_0 style='grid-column:1;grid-row:1 / span 2'></div><div id=s53_1 style='grid-column:2;grid-row:1'></div><div id=s53_2 style='grid-column:2;grid-row:2'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(26, lot52_node("s53_0")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("s53_0")->y);
    TEST_ASSERT_EQUAL(0, lot52_node("s53_1")->y);
    TEST_ASSERT_EQUAL(8, lot52_node("s53_1")->h);
    TEST_ASSERT_EQUAL(10, lot52_node("s53_2")->y);
    TEST_ASSERT_EQUAL(16, lot52_node("s53_2")->h);
    TEST_ASSERT_EQUAL(26, lot52_node("g")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_row_span_54(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:50px;grid-template-columns:20px 30px;grid-template-rows:16px 8px;row-gap:2px;align-items:stretch'><div id=s54_0 style='grid-column:1;grid-row:1 / span 2'></div><div id=s54_1 style='grid-column:2;grid-row:1'></div><div id=s54_2 style='grid-column:2;grid-row:2'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(26, lot52_node("s54_0")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("s54_0")->y);
    TEST_ASSERT_EQUAL(0, lot52_node("s54_1")->y);
    TEST_ASSERT_EQUAL(16, lot52_node("s54_1")->h);
    TEST_ASSERT_EQUAL(18, lot52_node("s54_2")->y);
    TEST_ASSERT_EQUAL(8, lot52_node("s54_2")->h);
    TEST_ASSERT_EQUAL(26, lot52_node("g")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_row_span_55(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:50px;grid-template-columns:20px 30px;grid-template-rows:10px 30px;row-gap:0px;align-items:stretch'><div id=s55_0 style='grid-column:1;grid-row:1 / span 2'></div><div id=s55_1 style='grid-column:2;grid-row:1'></div><div id=s55_2 style='grid-column:2;grid-row:2'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(40, lot52_node("s55_0")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("s55_0")->y);
    TEST_ASSERT_EQUAL(0, lot52_node("s55_1")->y);
    TEST_ASSERT_EQUAL(10, lot52_node("s55_1")->h);
    TEST_ASSERT_EQUAL(10, lot52_node("s55_2")->y);
    TEST_ASSERT_EQUAL(30, lot52_node("s55_2")->h);
    TEST_ASSERT_EQUAL(40, lot52_node("g")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_row_span_56(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:50px;grid-template-columns:20px 30px;grid-template-rows:30px 10px;row-gap:0px;align-items:stretch'><div id=s56_0 style='grid-column:1;grid-row:1 / span 2'></div><div id=s56_1 style='grid-column:2;grid-row:1'></div><div id=s56_2 style='grid-column:2;grid-row:2'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(40, lot52_node("s56_0")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("s56_0")->y);
    TEST_ASSERT_EQUAL(0, lot52_node("s56_1")->y);
    TEST_ASSERT_EQUAL(30, lot52_node("s56_1")->h);
    TEST_ASSERT_EQUAL(30, lot52_node("s56_2")->y);
    TEST_ASSERT_EQUAL(10, lot52_node("s56_2")->h);
    TEST_ASSERT_EQUAL(40, lot52_node("g")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_row_span_57(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:50px;grid-template-columns:20px 30px;grid-template-rows:14px 14px;row-gap:8px;align-items:stretch'><div id=s57_0 style='grid-column:1;grid-row:1 / span 2'></div><div id=s57_1 style='grid-column:2;grid-row:1'></div><div id=s57_2 style='grid-column:2;grid-row:2'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(36, lot52_node("s57_0")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("s57_0")->y);
    TEST_ASSERT_EQUAL(0, lot52_node("s57_1")->y);
    TEST_ASSERT_EQUAL(14, lot52_node("s57_1")->h);
    TEST_ASSERT_EQUAL(22, lot52_node("s57_2")->y);
    TEST_ASSERT_EQUAL(14, lot52_node("s57_2")->h);
    TEST_ASSERT_EQUAL(36, lot52_node("g")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_row_span_58(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:50px;grid-template-columns:20px 30px;grid-template-rows:18px 18px;row-gap:1px;align-items:stretch'><div id=s58_0 style='grid-column:1;grid-row:1 / span 2'></div><div id=s58_1 style='grid-column:2;grid-row:1'></div><div id=s58_2 style='grid-column:2;grid-row:2'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(37, lot52_node("s58_0")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("s58_0")->y);
    TEST_ASSERT_EQUAL(0, lot52_node("s58_1")->y);
    TEST_ASSERT_EQUAL(18, lot52_node("s58_1")->h);
    TEST_ASSERT_EQUAL(19, lot52_node("s58_2")->y);
    TEST_ASSERT_EQUAL(18, lot52_node("s58_2")->h);
    TEST_ASSERT_EQUAL(37, lot52_node("g")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_row_span_59(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:50px;grid-template-columns:20px 30px;grid-template-rows:9px 11px;row-gap:3px;align-items:stretch'><div id=s59_0 style='grid-column:1;grid-row:1 / span 2'></div><div id=s59_1 style='grid-column:2;grid-row:1'></div><div id=s59_2 style='grid-column:2;grid-row:2'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(23, lot52_node("s59_0")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("s59_0")->y);
    TEST_ASSERT_EQUAL(0, lot52_node("s59_1")->y);
    TEST_ASSERT_EQUAL(9, lot52_node("s59_1")->h);
    TEST_ASSERT_EQUAL(12, lot52_node("s59_2")->y);
    TEST_ASSERT_EQUAL(11, lot52_node("s59_2")->h);
    TEST_ASSERT_EQUAL(23, lot52_node("g")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_row_span_60(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:50px;grid-template-columns:20px 30px;grid-template-rows:11px 9px;row-gap:3px;align-items:stretch'><div id=s60_0 style='grid-column:1;grid-row:1 / span 2'></div><div id=s60_1 style='grid-column:2;grid-row:1'></div><div id=s60_2 style='grid-column:2;grid-row:2'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(23, lot52_node("s60_0")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("s60_0")->y);
    TEST_ASSERT_EQUAL(0, lot52_node("s60_1")->y);
    TEST_ASSERT_EQUAL(11, lot52_node("s60_1")->h);
    TEST_ASSERT_EQUAL(14, lot52_node("s60_2")->y);
    TEST_ASSERT_EQUAL(9, lot52_node("s60_2")->h);
    TEST_ASSERT_EQUAL(23, lot52_node("g")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_row_lines_1(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:68px;grid-template-rows:20px 20px 20px;row-gap:4px;align-items:stretch'><div id=a1 style='grid-row:2 / 3'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(24, lot52_node("a1")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("a1")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_row_lines_2(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:68px;grid-template-rows:20px 20px 20px;row-gap:4px;align-items:stretch'><div id=a2 style='grid-row:2 / 3'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(24, lot52_node("a2")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("a2")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_row_lines_3(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:68px;grid-template-rows:20px 20px 20px;row-gap:4px;align-items:stretch'><div id=a3 style='grid-row:2 / 3'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(24, lot52_node("a3")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("a3")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_row_lines_4(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:68px;grid-template-rows:20px 20px 20px;row-gap:4px;align-items:stretch'><div id=a4 style='grid-row:2 / 3'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(24, lot52_node("a4")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("a4")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_row_lines_5(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:68px;grid-template-rows:20px 20px 20px;row-gap:4px;align-items:stretch'><div id=a5 style='grid-row:2 / 3'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(24, lot52_node("a5")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("a5")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_row_lines_6(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:68px;grid-template-rows:20px 20px 20px;row-gap:4px;align-items:stretch'><div id=a6 style='grid-row:2 / 3'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(24, lot52_node("a6")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("a6")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_row_lines_7(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:68px;grid-template-rows:20px 20px 20px;row-gap:4px;align-items:stretch'><div id=a7 style='grid-row:2 / 3'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(24, lot52_node("a7")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("a7")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_row_lines_8(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:68px;grid-template-rows:20px 20px 20px;row-gap:4px;align-items:stretch'><div id=a8 style='grid-row:2 / 3'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(24, lot52_node("a8")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("a8")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_row_lines_9(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:68px;grid-template-rows:20px 20px 20px;row-gap:4px;align-items:stretch'><div id=a9 style='grid-row:2 / 3'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(24, lot52_node("a9")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("a9")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_row_lines_10(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:68px;grid-template-rows:20px 20px 20px;row-gap:4px;align-items:stretch'><div id=a10 style='grid-row:2 / 3'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(24, lot52_node("a10")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("a10")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_row_lines_11(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:68px;grid-template-rows:20px 20px 20px;row-gap:4px;align-items:stretch'><div id=a11 style='grid-row:2 / 3'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(24, lot52_node("a11")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("a11")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_row_lines_12(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:68px;grid-template-rows:20px 20px 20px;row-gap:4px;align-items:stretch'><div id=a12 style='grid-row:2 / 3'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(24, lot52_node("a12")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("a12")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_row_lines_13(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:68px;grid-template-rows:20px 20px 20px;row-gap:4px;align-items:stretch'><div id=a13 style='grid-row:2 / 3'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(24, lot52_node("a13")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("a13")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_row_lines_14(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:68px;grid-template-rows:20px 20px 20px;row-gap:4px;align-items:stretch'><div id=a14 style='grid-row:2 / 3'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(24, lot52_node("a14")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("a14")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_row_lines_15(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:68px;grid-template-rows:20px 20px 20px;row-gap:4px;align-items:stretch'><div id=a15 style='grid-row:2 / 3'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(24, lot52_node("a15")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("a15")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_row_lines_16(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:68px;grid-template-rows:20px 20px 20px;row-gap:4px;align-items:stretch'><div id=a16 style='grid-row:2 / 3'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(24, lot52_node("a16")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("a16")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_row_lines_17(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:68px;grid-template-rows:20px 20px 20px;row-gap:4px;align-items:stretch'><div id=a17 style='grid-row:2 / 3'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(24, lot52_node("a17")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("a17")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_row_lines_18(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:68px;grid-template-rows:20px 20px 20px;row-gap:4px;align-items:stretch'><div id=a18 style='grid-row:2 / 3'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(24, lot52_node("a18")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("a18")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_row_lines_19(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:68px;grid-template-rows:20px 20px 20px;row-gap:4px;align-items:stretch'><div id=a19 style='grid-row:2 / 3'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(24, lot52_node("a19")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("a19")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_row_lines_20(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:68px;grid-template-rows:20px 20px 20px;row-gap:4px;align-items:stretch'><div id=a20 style='grid-row:2 / 3'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(24, lot52_node("a20")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("a20")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_row_lines_21(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:68px;grid-template-rows:20px 20px 20px;row-gap:4px;align-items:stretch'><div id=a21 style='grid-row:2 / 3'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(24, lot52_node("a21")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("a21")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_row_lines_22(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:68px;grid-template-rows:20px 20px 20px;row-gap:4px;align-items:stretch'><div id=a22 style='grid-row:2 / 3'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(24, lot52_node("a22")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("a22")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_row_lines_23(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:68px;grid-template-rows:20px 20px 20px;row-gap:4px;align-items:stretch'><div id=a23 style='grid-row:2 / 3'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(24, lot52_node("a23")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("a23")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_row_lines_24(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:68px;grid-template-rows:20px 20px 20px;row-gap:4px;align-items:stretch'><div id=a24 style='grid-row:2 / 3'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(24, lot52_node("a24")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("a24")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_row_lines_25(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:68px;grid-template-rows:20px 20px 20px;row-gap:4px;align-items:stretch'><div id=a25 style='grid-row:2 / 3'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(24, lot52_node("a25")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("a25")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_row_lines_26(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:68px;grid-template-rows:20px 20px 20px;row-gap:4px;align-items:stretch'><div id=a26 style='grid-row:2 / 3'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(24, lot52_node("a26")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("a26")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_row_lines_27(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:68px;grid-template-rows:20px 20px 20px;row-gap:4px;align-items:stretch'><div id=a27 style='grid-row:2 / 3'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(24, lot52_node("a27")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("a27")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_row_lines_28(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:68px;grid-template-rows:20px 20px 20px;row-gap:4px;align-items:stretch'><div id=a28 style='grid-row:2 / 3'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(24, lot52_node("a28")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("a28")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_row_lines_29(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:68px;grid-template-rows:20px 20px 20px;row-gap:4px;align-items:stretch'><div id=a29 style='grid-row:2 / 3'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(24, lot52_node("a29")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("a29")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_row_lines_30(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:68px;grid-template-rows:20px 20px 20px;row-gap:4px;align-items:stretch'><div id=a30 style='grid-row:2 / 3'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(24, lot52_node("a30")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("a30")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_row_lines_31(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:68px;grid-template-rows:20px 20px 20px;row-gap:4px;align-items:stretch'><div id=a31 style='grid-row:2 / 3'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(24, lot52_node("a31")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("a31")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_row_lines_32(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:68px;grid-template-rows:20px 20px 20px;row-gap:4px;align-items:stretch'><div id=a32 style='grid-row:2 / 3'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(24, lot52_node("a32")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("a32")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_row_lines_33(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:68px;grid-template-rows:20px 20px 20px;row-gap:4px;align-items:stretch'><div id=a33 style='grid-row:2 / 3'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(24, lot52_node("a33")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("a33")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_row_lines_34(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:68px;grid-template-rows:20px 20px 20px;row-gap:4px;align-items:stretch'><div id=a34 style='grid-row:2 / 3'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(24, lot52_node("a34")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("a34")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_row_lines_35(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:68px;grid-template-rows:20px 20px 20px;row-gap:4px;align-items:stretch'><div id=a35 style='grid-row:2 / 3'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(24, lot52_node("a35")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("a35")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_row_lines_36(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:68px;grid-template-rows:20px 20px 20px;row-gap:4px;align-items:stretch'><div id=a36 style='grid-row:2 / 3'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(24, lot52_node("a36")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("a36")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_row_lines_37(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:68px;grid-template-rows:20px 20px 20px;row-gap:4px;align-items:stretch'><div id=a37 style='grid-row:2 / 3'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(24, lot52_node("a37")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("a37")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_row_lines_38(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:68px;grid-template-rows:20px 20px 20px;row-gap:4px;align-items:stretch'><div id=a38 style='grid-row:2 / 3'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(24, lot52_node("a38")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("a38")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_row_lines_39(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:68px;grid-template-rows:20px 20px 20px;row-gap:4px;align-items:stretch'><div id=a39 style='grid-row:2 / 3'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(24, lot52_node("a39")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("a39")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_row_lines_40(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:68px;grid-template-rows:20px 20px 20px;row-gap:4px;align-items:stretch'><div id=a40 style='grid-row:2 / 3'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(24, lot52_node("a40")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("a40")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void lot78_rows_run(void) {
    RUN_TEST(test_l78_rows_px_1);
    RUN_TEST(test_l78_rows_px_2);
    RUN_TEST(test_l78_rows_px_3);
    RUN_TEST(test_l78_rows_px_4);
    RUN_TEST(test_l78_rows_px_5);
    RUN_TEST(test_l78_rows_px_6);
    RUN_TEST(test_l78_rows_px_7);
    RUN_TEST(test_l78_rows_px_8);
    RUN_TEST(test_l78_rows_px_9);
    RUN_TEST(test_l78_rows_px_10);
    RUN_TEST(test_l78_rows_px_11);
    RUN_TEST(test_l78_rows_px_12);
    RUN_TEST(test_l78_rows_px_13);
    RUN_TEST(test_l78_rows_px_14);
    RUN_TEST(test_l78_rows_px_15);
    RUN_TEST(test_l78_rows_px_16);
    RUN_TEST(test_l78_rows_px_17);
    RUN_TEST(test_l78_rows_px_18);
    RUN_TEST(test_l78_rows_px_19);
    RUN_TEST(test_l78_rows_px_20);
    RUN_TEST(test_l78_rows_px_21);
    RUN_TEST(test_l78_rows_px_22);
    RUN_TEST(test_l78_rows_px_23);
    RUN_TEST(test_l78_rows_px_24);
    RUN_TEST(test_l78_rows_px_25);
    RUN_TEST(test_l78_rows_px_26);
    RUN_TEST(test_l78_rows_px_27);
    RUN_TEST(test_l78_rows_px_28);
    RUN_TEST(test_l78_rows_px_29);
    RUN_TEST(test_l78_rows_px_30);
    RUN_TEST(test_l78_rows_px_31);
    RUN_TEST(test_l78_rows_px_32);
    RUN_TEST(test_l78_rows_px_33);
    RUN_TEST(test_l78_rows_px_34);
    RUN_TEST(test_l78_rows_px_35);
    RUN_TEST(test_l78_rows_px_36);
    RUN_TEST(test_l78_rows_px_37);
    RUN_TEST(test_l78_rows_px_38);
    RUN_TEST(test_l78_rows_px_39);
    RUN_TEST(test_l78_rows_px_40);
    RUN_TEST(test_l78_rows_px_41);
    RUN_TEST(test_l78_rows_px_42);
    RUN_TEST(test_l78_rows_px_43);
    RUN_TEST(test_l78_rows_px_44);
    RUN_TEST(test_l78_rows_px_45);
    RUN_TEST(test_l78_rows_px_46);
    RUN_TEST(test_l78_rows_px_47);
    RUN_TEST(test_l78_rows_px_48);
    RUN_TEST(test_l78_rows_px_49);
    RUN_TEST(test_l78_rows_px_50);
    RUN_TEST(test_l78_rows_px_51);
    RUN_TEST(test_l78_rows_px_52);
    RUN_TEST(test_l78_rows_px_53);
    RUN_TEST(test_l78_rows_px_54);
    RUN_TEST(test_l78_rows_px_55);
    RUN_TEST(test_l78_rows_px_56);
    RUN_TEST(test_l78_rows_px_57);
    RUN_TEST(test_l78_rows_px_58);
    RUN_TEST(test_l78_rows_px_59);
    RUN_TEST(test_l78_rows_px_60);
    RUN_TEST(test_l78_rows_px_61);
    RUN_TEST(test_l78_rows_px_62);
    RUN_TEST(test_l78_rows_px_63);
    RUN_TEST(test_l78_rows_px_64);
    RUN_TEST(test_l78_rows_px_65);
    RUN_TEST(test_l78_rows_px_66);
    RUN_TEST(test_l78_rows_px_67);
    RUN_TEST(test_l78_rows_px_68);
    RUN_TEST(test_l78_rows_px_69);
    RUN_TEST(test_l78_rows_px_70);
    RUN_TEST(test_l78_rows_px_71);
    RUN_TEST(test_l78_rows_px_72);
    RUN_TEST(test_l78_rows_px_73);
    RUN_TEST(test_l78_rows_px_74);
    RUN_TEST(test_l78_rows_px_75);
    RUN_TEST(test_l78_rows_px_76);
    RUN_TEST(test_l78_rows_px_77);
    RUN_TEST(test_l78_rows_px_78);
    RUN_TEST(test_l78_rows_px_79);
    RUN_TEST(test_l78_rows_px_80);
    RUN_TEST(test_l78_rows_px_81);
    RUN_TEST(test_l78_rows_px_82);
    RUN_TEST(test_l78_rows_px_83);
    RUN_TEST(test_l78_rows_px_84);
    RUN_TEST(test_l78_rows_px_85);
    RUN_TEST(test_l78_rows_px_86);
    RUN_TEST(test_l78_rows_px_87);
    RUN_TEST(test_l78_rows_px_88);
    RUN_TEST(test_l78_rows_px_89);
    RUN_TEST(test_l78_rows_px_90);
    RUN_TEST(test_l78_rows_px_91);
    RUN_TEST(test_l78_rows_px_92);
    RUN_TEST(test_l78_rows_px_93);
    RUN_TEST(test_l78_rows_px_94);
    RUN_TEST(test_l78_rows_px_95);
    RUN_TEST(test_l78_rows_px_96);
    RUN_TEST(test_l78_rows_px_97);
    RUN_TEST(test_l78_rows_px_98);
    RUN_TEST(test_l78_rows_px_99);
    RUN_TEST(test_l78_rows_px_100);
    RUN_TEST(test_l78_rows_fr_1);
    RUN_TEST(test_l78_rows_fr_2);
    RUN_TEST(test_l78_rows_fr_3);
    RUN_TEST(test_l78_rows_fr_4);
    RUN_TEST(test_l78_rows_fr_5);
    RUN_TEST(test_l78_rows_fr_6);
    RUN_TEST(test_l78_rows_fr_7);
    RUN_TEST(test_l78_rows_fr_8);
    RUN_TEST(test_l78_rows_fr_9);
    RUN_TEST(test_l78_rows_fr_10);
    RUN_TEST(test_l78_rows_fr_11);
    RUN_TEST(test_l78_rows_fr_12);
    RUN_TEST(test_l78_rows_fr_13);
    RUN_TEST(test_l78_rows_fr_14);
    RUN_TEST(test_l78_rows_fr_15);
    RUN_TEST(test_l78_rows_fr_16);
    RUN_TEST(test_l78_rows_fr_17);
    RUN_TEST(test_l78_rows_fr_18);
    RUN_TEST(test_l78_rows_fr_19);
    RUN_TEST(test_l78_rows_fr_20);
    RUN_TEST(test_l78_rows_fr_21);
    RUN_TEST(test_l78_rows_fr_22);
    RUN_TEST(test_l78_rows_fr_23);
    RUN_TEST(test_l78_rows_fr_24);
    RUN_TEST(test_l78_rows_fr_25);
    RUN_TEST(test_l78_rows_fr_26);
    RUN_TEST(test_l78_rows_fr_27);
    RUN_TEST(test_l78_rows_fr_28);
    RUN_TEST(test_l78_rows_fr_29);
    RUN_TEST(test_l78_rows_fr_30);
    RUN_TEST(test_l78_rows_fr_31);
    RUN_TEST(test_l78_rows_fr_32);
    RUN_TEST(test_l78_rows_fr_33);
    RUN_TEST(test_l78_rows_fr_34);
    RUN_TEST(test_l78_rows_fr_35);
    RUN_TEST(test_l78_rows_fr_36);
    RUN_TEST(test_l78_rows_fr_37);
    RUN_TEST(test_l78_rows_fr_38);
    RUN_TEST(test_l78_rows_fr_39);
    RUN_TEST(test_l78_rows_fr_40);
    RUN_TEST(test_l78_rows_fr_41);
    RUN_TEST(test_l78_rows_fr_42);
    RUN_TEST(test_l78_rows_fr_43);
    RUN_TEST(test_l78_rows_fr_44);
    RUN_TEST(test_l78_rows_fr_45);
    RUN_TEST(test_l78_rows_fr_46);
    RUN_TEST(test_l78_rows_fr_47);
    RUN_TEST(test_l78_rows_fr_48);
    RUN_TEST(test_l78_rows_fr_49);
    RUN_TEST(test_l78_rows_fr_50);
    RUN_TEST(test_l78_rows_fr_51);
    RUN_TEST(test_l78_rows_fr_52);
    RUN_TEST(test_l78_rows_fr_53);
    RUN_TEST(test_l78_rows_fr_54);
    RUN_TEST(test_l78_rows_fr_55);
    RUN_TEST(test_l78_rows_fr_56);
    RUN_TEST(test_l78_rows_fr_57);
    RUN_TEST(test_l78_rows_fr_58);
    RUN_TEST(test_l78_rows_fr_59);
    RUN_TEST(test_l78_rows_fr_60);
    RUN_TEST(test_l78_rows_fr_61);
    RUN_TEST(test_l78_rows_fr_62);
    RUN_TEST(test_l78_rows_fr_63);
    RUN_TEST(test_l78_rows_fr_64);
    RUN_TEST(test_l78_rows_fr_65);
    RUN_TEST(test_l78_rows_fr_66);
    RUN_TEST(test_l78_rows_fr_67);
    RUN_TEST(test_l78_rows_fr_68);
    RUN_TEST(test_l78_rows_fr_69);
    RUN_TEST(test_l78_rows_fr_70);
    RUN_TEST(test_l78_rows_fr_71);
    RUN_TEST(test_l78_rows_fr_72);
    RUN_TEST(test_l78_rows_fr_73);
    RUN_TEST(test_l78_rows_fr_74);
    RUN_TEST(test_l78_rows_fr_75);
    RUN_TEST(test_l78_rows_fr_76);
    RUN_TEST(test_l78_rows_fr_77);
    RUN_TEST(test_l78_rows_fr_78);
    RUN_TEST(test_l78_rows_fr_79);
    RUN_TEST(test_l78_rows_fr_80);
    RUN_TEST(test_l78_row_span_1);
    RUN_TEST(test_l78_row_span_2);
    RUN_TEST(test_l78_row_span_3);
    RUN_TEST(test_l78_row_span_4);
    RUN_TEST(test_l78_row_span_5);
    RUN_TEST(test_l78_row_span_6);
    RUN_TEST(test_l78_row_span_7);
    RUN_TEST(test_l78_row_span_8);
    RUN_TEST(test_l78_row_span_9);
    RUN_TEST(test_l78_row_span_10);
    RUN_TEST(test_l78_row_span_11);
    RUN_TEST(test_l78_row_span_12);
    RUN_TEST(test_l78_row_span_13);
    RUN_TEST(test_l78_row_span_14);
    RUN_TEST(test_l78_row_span_15);
    RUN_TEST(test_l78_row_span_16);
    RUN_TEST(test_l78_row_span_17);
    RUN_TEST(test_l78_row_span_18);
    RUN_TEST(test_l78_row_span_19);
    RUN_TEST(test_l78_row_span_20);
    RUN_TEST(test_l78_row_span_21);
    RUN_TEST(test_l78_row_span_22);
    RUN_TEST(test_l78_row_span_23);
    RUN_TEST(test_l78_row_span_24);
    RUN_TEST(test_l78_row_span_25);
    RUN_TEST(test_l78_row_span_26);
    RUN_TEST(test_l78_row_span_27);
    RUN_TEST(test_l78_row_span_28);
    RUN_TEST(test_l78_row_span_29);
    RUN_TEST(test_l78_row_span_30);
    RUN_TEST(test_l78_row_span_31);
    RUN_TEST(test_l78_row_span_32);
    RUN_TEST(test_l78_row_span_33);
    RUN_TEST(test_l78_row_span_34);
    RUN_TEST(test_l78_row_span_35);
    RUN_TEST(test_l78_row_span_36);
    RUN_TEST(test_l78_row_span_37);
    RUN_TEST(test_l78_row_span_38);
    RUN_TEST(test_l78_row_span_39);
    RUN_TEST(test_l78_row_span_40);
    RUN_TEST(test_l78_row_span_41);
    RUN_TEST(test_l78_row_span_42);
    RUN_TEST(test_l78_row_span_43);
    RUN_TEST(test_l78_row_span_44);
    RUN_TEST(test_l78_row_span_45);
    RUN_TEST(test_l78_row_span_46);
    RUN_TEST(test_l78_row_span_47);
    RUN_TEST(test_l78_row_span_48);
    RUN_TEST(test_l78_row_span_49);
    RUN_TEST(test_l78_row_span_50);
    RUN_TEST(test_l78_row_span_51);
    RUN_TEST(test_l78_row_span_52);
    RUN_TEST(test_l78_row_span_53);
    RUN_TEST(test_l78_row_span_54);
    RUN_TEST(test_l78_row_span_55);
    RUN_TEST(test_l78_row_span_56);
    RUN_TEST(test_l78_row_span_57);
    RUN_TEST(test_l78_row_span_58);
    RUN_TEST(test_l78_row_span_59);
    RUN_TEST(test_l78_row_span_60);
    RUN_TEST(test_l78_row_lines_1);
    RUN_TEST(test_l78_row_lines_2);
    RUN_TEST(test_l78_row_lines_3);
    RUN_TEST(test_l78_row_lines_4);
    RUN_TEST(test_l78_row_lines_5);
    RUN_TEST(test_l78_row_lines_6);
    RUN_TEST(test_l78_row_lines_7);
    RUN_TEST(test_l78_row_lines_8);
    RUN_TEST(test_l78_row_lines_9);
    RUN_TEST(test_l78_row_lines_10);
    RUN_TEST(test_l78_row_lines_11);
    RUN_TEST(test_l78_row_lines_12);
    RUN_TEST(test_l78_row_lines_13);
    RUN_TEST(test_l78_row_lines_14);
    RUN_TEST(test_l78_row_lines_15);
    RUN_TEST(test_l78_row_lines_16);
    RUN_TEST(test_l78_row_lines_17);
    RUN_TEST(test_l78_row_lines_18);
    RUN_TEST(test_l78_row_lines_19);
    RUN_TEST(test_l78_row_lines_20);
    RUN_TEST(test_l78_row_lines_21);
    RUN_TEST(test_l78_row_lines_22);
    RUN_TEST(test_l78_row_lines_23);
    RUN_TEST(test_l78_row_lines_24);
    RUN_TEST(test_l78_row_lines_25);
    RUN_TEST(test_l78_row_lines_26);
    RUN_TEST(test_l78_row_lines_27);
    RUN_TEST(test_l78_row_lines_28);
    RUN_TEST(test_l78_row_lines_29);
    RUN_TEST(test_l78_row_lines_30);
    RUN_TEST(test_l78_row_lines_31);
    RUN_TEST(test_l78_row_lines_32);
    RUN_TEST(test_l78_row_lines_33);
    RUN_TEST(test_l78_row_lines_34);
    RUN_TEST(test_l78_row_lines_35);
    RUN_TEST(test_l78_row_lines_36);
    RUN_TEST(test_l78_row_lines_37);
    RUN_TEST(test_l78_row_lines_38);
    RUN_TEST(test_l78_row_lines_39);
    RUN_TEST(test_l78_row_lines_40);
}
