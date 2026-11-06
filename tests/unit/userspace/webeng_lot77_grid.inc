/* Lot 77: systematic CSS grid subset proofs (px/fr/repeat/gap/span/auto). */
/* Uses lot52_render / lot52_node from webeng_lot52.inc. */

static void test_l77_grid_px_1(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:74px;grid-template-columns:repeat(3,24px);column-gap:1px'><div id=a1_0 style='height:8px'></div><div id=a1_1 style='height:8px'></div><div id=a1_2 style='height:8px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(3, lot52_node("g")->grid_cols);
    TEST_ASSERT_EQUAL(24, lot52_node("a1_0")->w);
    TEST_ASSERT_EQUAL(0, lot52_node("a1_0")->x);
    TEST_ASSERT_EQUAL(24, lot52_node("a1_1")->w);
    TEST_ASSERT_EQUAL(25, lot52_node("a1_1")->x);
    TEST_ASSERT_EQUAL(24, lot52_node("a1_2")->w);
    TEST_ASSERT_EQUAL(50, lot52_node("a1_2")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l77_grid_px_2(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:118px;grid-template-columns:repeat(4,28px);column-gap:2px'><div id=a2_0 style='height:8px'></div><div id=a2_1 style='height:8px'></div><div id=a2_2 style='height:8px'></div><div id=a2_3 style='height:8px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(4, lot52_node("g")->grid_cols);
    TEST_ASSERT_EQUAL(28, lot52_node("a2_0")->w);
    TEST_ASSERT_EQUAL(0, lot52_node("a2_0")->x);
    TEST_ASSERT_EQUAL(28, lot52_node("a2_1")->w);
    TEST_ASSERT_EQUAL(30, lot52_node("a2_1")->x);
    TEST_ASSERT_EQUAL(28, lot52_node("a2_2")->w);
    TEST_ASSERT_EQUAL(60, lot52_node("a2_2")->x);
    TEST_ASSERT_EQUAL(28, lot52_node("a2_3")->w);
    TEST_ASSERT_EQUAL(90, lot52_node("a2_3")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l77_grid_px_3(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:67px;grid-template-columns:repeat(2,32px);column-gap:3px'><div id=a3_0 style='height:8px'></div><div id=a3_1 style='height:8px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(2, lot52_node("g")->grid_cols);
    TEST_ASSERT_EQUAL(32, lot52_node("a3_0")->w);
    TEST_ASSERT_EQUAL(0, lot52_node("a3_0")->x);
    TEST_ASSERT_EQUAL(32, lot52_node("a3_1")->w);
    TEST_ASSERT_EQUAL(35, lot52_node("a3_1")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l77_grid_px_4(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:116px;grid-template-columns:repeat(3,36px);column-gap:4px'><div id=a4_0 style='height:8px'></div><div id=a4_1 style='height:8px'></div><div id=a4_2 style='height:8px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(3, lot52_node("g")->grid_cols);
    TEST_ASSERT_EQUAL(36, lot52_node("a4_0")->w);
    TEST_ASSERT_EQUAL(0, lot52_node("a4_0")->x);
    TEST_ASSERT_EQUAL(36, lot52_node("a4_1")->w);
    TEST_ASSERT_EQUAL(40, lot52_node("a4_1")->x);
    TEST_ASSERT_EQUAL(36, lot52_node("a4_2")->w);
    TEST_ASSERT_EQUAL(80, lot52_node("a4_2")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l77_grid_px_5(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:95px;grid-template-columns:repeat(4,20px);column-gap:5px'><div id=a5_0 style='height:8px'></div><div id=a5_1 style='height:8px'></div><div id=a5_2 style='height:8px'></div><div id=a5_3 style='height:8px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(4, lot52_node("g")->grid_cols);
    TEST_ASSERT_EQUAL(20, lot52_node("a5_0")->w);
    TEST_ASSERT_EQUAL(0, lot52_node("a5_0")->x);
    TEST_ASSERT_EQUAL(20, lot52_node("a5_1")->w);
    TEST_ASSERT_EQUAL(25, lot52_node("a5_1")->x);
    TEST_ASSERT_EQUAL(20, lot52_node("a5_2")->w);
    TEST_ASSERT_EQUAL(50, lot52_node("a5_2")->x);
    TEST_ASSERT_EQUAL(20, lot52_node("a5_3")->w);
    TEST_ASSERT_EQUAL(75, lot52_node("a5_3")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l77_grid_px_6(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:48px;grid-template-columns:repeat(2,24px);column-gap:0px'><div id=a6_0 style='height:8px'></div><div id=a6_1 style='height:8px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(2, lot52_node("g")->grid_cols);
    TEST_ASSERT_EQUAL(24, lot52_node("a6_0")->w);
    TEST_ASSERT_EQUAL(0, lot52_node("a6_0")->x);
    TEST_ASSERT_EQUAL(24, lot52_node("a6_1")->w);
    TEST_ASSERT_EQUAL(24, lot52_node("a6_1")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l77_grid_px_7(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:86px;grid-template-columns:repeat(3,28px);column-gap:1px'><div id=a7_0 style='height:8px'></div><div id=a7_1 style='height:8px'></div><div id=a7_2 style='height:8px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(3, lot52_node("g")->grid_cols);
    TEST_ASSERT_EQUAL(28, lot52_node("a7_0")->w);
    TEST_ASSERT_EQUAL(0, lot52_node("a7_0")->x);
    TEST_ASSERT_EQUAL(28, lot52_node("a7_1")->w);
    TEST_ASSERT_EQUAL(29, lot52_node("a7_1")->x);
    TEST_ASSERT_EQUAL(28, lot52_node("a7_2")->w);
    TEST_ASSERT_EQUAL(58, lot52_node("a7_2")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l77_grid_px_8(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:134px;grid-template-columns:repeat(4,32px);column-gap:2px'><div id=a8_0 style='height:8px'></div><div id=a8_1 style='height:8px'></div><div id=a8_2 style='height:8px'></div><div id=a8_3 style='height:8px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(4, lot52_node("g")->grid_cols);
    TEST_ASSERT_EQUAL(32, lot52_node("a8_0")->w);
    TEST_ASSERT_EQUAL(0, lot52_node("a8_0")->x);
    TEST_ASSERT_EQUAL(32, lot52_node("a8_1")->w);
    TEST_ASSERT_EQUAL(34, lot52_node("a8_1")->x);
    TEST_ASSERT_EQUAL(32, lot52_node("a8_2")->w);
    TEST_ASSERT_EQUAL(68, lot52_node("a8_2")->x);
    TEST_ASSERT_EQUAL(32, lot52_node("a8_3")->w);
    TEST_ASSERT_EQUAL(102, lot52_node("a8_3")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l77_grid_px_9(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:75px;grid-template-columns:repeat(2,36px);column-gap:3px'><div id=a9_0 style='height:8px'></div><div id=a9_1 style='height:8px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(2, lot52_node("g")->grid_cols);
    TEST_ASSERT_EQUAL(36, lot52_node("a9_0")->w);
    TEST_ASSERT_EQUAL(0, lot52_node("a9_0")->x);
    TEST_ASSERT_EQUAL(36, lot52_node("a9_1")->w);
    TEST_ASSERT_EQUAL(39, lot52_node("a9_1")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l77_grid_px_10(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:68px;grid-template-columns:repeat(3,20px);column-gap:4px'><div id=a10_0 style='height:8px'></div><div id=a10_1 style='height:8px'></div><div id=a10_2 style='height:8px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(3, lot52_node("g")->grid_cols);
    TEST_ASSERT_EQUAL(20, lot52_node("a10_0")->w);
    TEST_ASSERT_EQUAL(0, lot52_node("a10_0")->x);
    TEST_ASSERT_EQUAL(20, lot52_node("a10_1")->w);
    TEST_ASSERT_EQUAL(24, lot52_node("a10_1")->x);
    TEST_ASSERT_EQUAL(20, lot52_node("a10_2")->w);
    TEST_ASSERT_EQUAL(48, lot52_node("a10_2")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l77_grid_px_11(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:111px;grid-template-columns:repeat(4,24px);column-gap:5px'><div id=a11_0 style='height:8px'></div><div id=a11_1 style='height:8px'></div><div id=a11_2 style='height:8px'></div><div id=a11_3 style='height:8px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(4, lot52_node("g")->grid_cols);
    TEST_ASSERT_EQUAL(24, lot52_node("a11_0")->w);
    TEST_ASSERT_EQUAL(0, lot52_node("a11_0")->x);
    TEST_ASSERT_EQUAL(24, lot52_node("a11_1")->w);
    TEST_ASSERT_EQUAL(29, lot52_node("a11_1")->x);
    TEST_ASSERT_EQUAL(24, lot52_node("a11_2")->w);
    TEST_ASSERT_EQUAL(58, lot52_node("a11_2")->x);
    TEST_ASSERT_EQUAL(24, lot52_node("a11_3")->w);
    TEST_ASSERT_EQUAL(87, lot52_node("a11_3")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l77_grid_px_12(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:56px;grid-template-columns:repeat(2,28px);column-gap:0px'><div id=a12_0 style='height:8px'></div><div id=a12_1 style='height:8px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(2, lot52_node("g")->grid_cols);
    TEST_ASSERT_EQUAL(28, lot52_node("a12_0")->w);
    TEST_ASSERT_EQUAL(0, lot52_node("a12_0")->x);
    TEST_ASSERT_EQUAL(28, lot52_node("a12_1")->w);
    TEST_ASSERT_EQUAL(28, lot52_node("a12_1")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l77_grid_px_13(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:98px;grid-template-columns:repeat(3,32px);column-gap:1px'><div id=a13_0 style='height:8px'></div><div id=a13_1 style='height:8px'></div><div id=a13_2 style='height:8px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(3, lot52_node("g")->grid_cols);
    TEST_ASSERT_EQUAL(32, lot52_node("a13_0")->w);
    TEST_ASSERT_EQUAL(0, lot52_node("a13_0")->x);
    TEST_ASSERT_EQUAL(32, lot52_node("a13_1")->w);
    TEST_ASSERT_EQUAL(33, lot52_node("a13_1")->x);
    TEST_ASSERT_EQUAL(32, lot52_node("a13_2")->w);
    TEST_ASSERT_EQUAL(66, lot52_node("a13_2")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l77_grid_px_14(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:150px;grid-template-columns:repeat(4,36px);column-gap:2px'><div id=a14_0 style='height:8px'></div><div id=a14_1 style='height:8px'></div><div id=a14_2 style='height:8px'></div><div id=a14_3 style='height:8px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(4, lot52_node("g")->grid_cols);
    TEST_ASSERT_EQUAL(36, lot52_node("a14_0")->w);
    TEST_ASSERT_EQUAL(0, lot52_node("a14_0")->x);
    TEST_ASSERT_EQUAL(36, lot52_node("a14_1")->w);
    TEST_ASSERT_EQUAL(38, lot52_node("a14_1")->x);
    TEST_ASSERT_EQUAL(36, lot52_node("a14_2")->w);
    TEST_ASSERT_EQUAL(76, lot52_node("a14_2")->x);
    TEST_ASSERT_EQUAL(36, lot52_node("a14_3")->w);
    TEST_ASSERT_EQUAL(114, lot52_node("a14_3")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l77_grid_px_15(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:43px;grid-template-columns:repeat(2,20px);column-gap:3px'><div id=a15_0 style='height:8px'></div><div id=a15_1 style='height:8px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(2, lot52_node("g")->grid_cols);
    TEST_ASSERT_EQUAL(20, lot52_node("a15_0")->w);
    TEST_ASSERT_EQUAL(0, lot52_node("a15_0")->x);
    TEST_ASSERT_EQUAL(20, lot52_node("a15_1")->w);
    TEST_ASSERT_EQUAL(23, lot52_node("a15_1")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l77_grid_px_16(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:80px;grid-template-columns:repeat(3,24px);column-gap:4px'><div id=a16_0 style='height:8px'></div><div id=a16_1 style='height:8px'></div><div id=a16_2 style='height:8px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(3, lot52_node("g")->grid_cols);
    TEST_ASSERT_EQUAL(24, lot52_node("a16_0")->w);
    TEST_ASSERT_EQUAL(0, lot52_node("a16_0")->x);
    TEST_ASSERT_EQUAL(24, lot52_node("a16_1")->w);
    TEST_ASSERT_EQUAL(28, lot52_node("a16_1")->x);
    TEST_ASSERT_EQUAL(24, lot52_node("a16_2")->w);
    TEST_ASSERT_EQUAL(56, lot52_node("a16_2")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l77_grid_px_17(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:127px;grid-template-columns:repeat(4,28px);column-gap:5px'><div id=a17_0 style='height:8px'></div><div id=a17_1 style='height:8px'></div><div id=a17_2 style='height:8px'></div><div id=a17_3 style='height:8px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(4, lot52_node("g")->grid_cols);
    TEST_ASSERT_EQUAL(28, lot52_node("a17_0")->w);
    TEST_ASSERT_EQUAL(0, lot52_node("a17_0")->x);
    TEST_ASSERT_EQUAL(28, lot52_node("a17_1")->w);
    TEST_ASSERT_EQUAL(33, lot52_node("a17_1")->x);
    TEST_ASSERT_EQUAL(28, lot52_node("a17_2")->w);
    TEST_ASSERT_EQUAL(66, lot52_node("a17_2")->x);
    TEST_ASSERT_EQUAL(28, lot52_node("a17_3")->w);
    TEST_ASSERT_EQUAL(99, lot52_node("a17_3")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l77_grid_px_18(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:64px;grid-template-columns:repeat(2,32px);column-gap:0px'><div id=a18_0 style='height:8px'></div><div id=a18_1 style='height:8px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(2, lot52_node("g")->grid_cols);
    TEST_ASSERT_EQUAL(32, lot52_node("a18_0")->w);
    TEST_ASSERT_EQUAL(0, lot52_node("a18_0")->x);
    TEST_ASSERT_EQUAL(32, lot52_node("a18_1")->w);
    TEST_ASSERT_EQUAL(32, lot52_node("a18_1")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l77_grid_px_19(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:110px;grid-template-columns:repeat(3,36px);column-gap:1px'><div id=a19_0 style='height:8px'></div><div id=a19_1 style='height:8px'></div><div id=a19_2 style='height:8px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(3, lot52_node("g")->grid_cols);
    TEST_ASSERT_EQUAL(36, lot52_node("a19_0")->w);
    TEST_ASSERT_EQUAL(0, lot52_node("a19_0")->x);
    TEST_ASSERT_EQUAL(36, lot52_node("a19_1")->w);
    TEST_ASSERT_EQUAL(37, lot52_node("a19_1")->x);
    TEST_ASSERT_EQUAL(36, lot52_node("a19_2")->w);
    TEST_ASSERT_EQUAL(74, lot52_node("a19_2")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l77_grid_px_20(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:86px;grid-template-columns:repeat(4,20px);column-gap:2px'><div id=a20_0 style='height:8px'></div><div id=a20_1 style='height:8px'></div><div id=a20_2 style='height:8px'></div><div id=a20_3 style='height:8px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(4, lot52_node("g")->grid_cols);
    TEST_ASSERT_EQUAL(20, lot52_node("a20_0")->w);
    TEST_ASSERT_EQUAL(0, lot52_node("a20_0")->x);
    TEST_ASSERT_EQUAL(20, lot52_node("a20_1")->w);
    TEST_ASSERT_EQUAL(22, lot52_node("a20_1")->x);
    TEST_ASSERT_EQUAL(20, lot52_node("a20_2")->w);
    TEST_ASSERT_EQUAL(44, lot52_node("a20_2")->x);
    TEST_ASSERT_EQUAL(20, lot52_node("a20_3")->w);
    TEST_ASSERT_EQUAL(66, lot52_node("a20_3")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l77_grid_px_21(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:51px;grid-template-columns:repeat(2,24px);column-gap:3px'><div id=a21_0 style='height:8px'></div><div id=a21_1 style='height:8px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(2, lot52_node("g")->grid_cols);
    TEST_ASSERT_EQUAL(24, lot52_node("a21_0")->w);
    TEST_ASSERT_EQUAL(0, lot52_node("a21_0")->x);
    TEST_ASSERT_EQUAL(24, lot52_node("a21_1")->w);
    TEST_ASSERT_EQUAL(27, lot52_node("a21_1")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l77_grid_px_22(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:92px;grid-template-columns:repeat(3,28px);column-gap:4px'><div id=a22_0 style='height:8px'></div><div id=a22_1 style='height:8px'></div><div id=a22_2 style='height:8px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(3, lot52_node("g")->grid_cols);
    TEST_ASSERT_EQUAL(28, lot52_node("a22_0")->w);
    TEST_ASSERT_EQUAL(0, lot52_node("a22_0")->x);
    TEST_ASSERT_EQUAL(28, lot52_node("a22_1")->w);
    TEST_ASSERT_EQUAL(32, lot52_node("a22_1")->x);
    TEST_ASSERT_EQUAL(28, lot52_node("a22_2")->w);
    TEST_ASSERT_EQUAL(64, lot52_node("a22_2")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l77_grid_px_23(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:143px;grid-template-columns:repeat(4,32px);column-gap:5px'><div id=a23_0 style='height:8px'></div><div id=a23_1 style='height:8px'></div><div id=a23_2 style='height:8px'></div><div id=a23_3 style='height:8px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(4, lot52_node("g")->grid_cols);
    TEST_ASSERT_EQUAL(32, lot52_node("a23_0")->w);
    TEST_ASSERT_EQUAL(0, lot52_node("a23_0")->x);
    TEST_ASSERT_EQUAL(32, lot52_node("a23_1")->w);
    TEST_ASSERT_EQUAL(37, lot52_node("a23_1")->x);
    TEST_ASSERT_EQUAL(32, lot52_node("a23_2")->w);
    TEST_ASSERT_EQUAL(74, lot52_node("a23_2")->x);
    TEST_ASSERT_EQUAL(32, lot52_node("a23_3")->w);
    TEST_ASSERT_EQUAL(111, lot52_node("a23_3")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l77_grid_px_24(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:72px;grid-template-columns:repeat(2,36px);column-gap:0px'><div id=a24_0 style='height:8px'></div><div id=a24_1 style='height:8px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(2, lot52_node("g")->grid_cols);
    TEST_ASSERT_EQUAL(36, lot52_node("a24_0")->w);
    TEST_ASSERT_EQUAL(0, lot52_node("a24_0")->x);
    TEST_ASSERT_EQUAL(36, lot52_node("a24_1")->w);
    TEST_ASSERT_EQUAL(36, lot52_node("a24_1")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l77_grid_px_25(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:62px;grid-template-columns:repeat(3,20px);column-gap:1px'><div id=a25_0 style='height:8px'></div><div id=a25_1 style='height:8px'></div><div id=a25_2 style='height:8px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(3, lot52_node("g")->grid_cols);
    TEST_ASSERT_EQUAL(20, lot52_node("a25_0")->w);
    TEST_ASSERT_EQUAL(0, lot52_node("a25_0")->x);
    TEST_ASSERT_EQUAL(20, lot52_node("a25_1")->w);
    TEST_ASSERT_EQUAL(21, lot52_node("a25_1")->x);
    TEST_ASSERT_EQUAL(20, lot52_node("a25_2")->w);
    TEST_ASSERT_EQUAL(42, lot52_node("a25_2")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l77_grid_px_26(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:102px;grid-template-columns:repeat(4,24px);column-gap:2px'><div id=a26_0 style='height:8px'></div><div id=a26_1 style='height:8px'></div><div id=a26_2 style='height:8px'></div><div id=a26_3 style='height:8px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(4, lot52_node("g")->grid_cols);
    TEST_ASSERT_EQUAL(24, lot52_node("a26_0")->w);
    TEST_ASSERT_EQUAL(0, lot52_node("a26_0")->x);
    TEST_ASSERT_EQUAL(24, lot52_node("a26_1")->w);
    TEST_ASSERT_EQUAL(26, lot52_node("a26_1")->x);
    TEST_ASSERT_EQUAL(24, lot52_node("a26_2")->w);
    TEST_ASSERT_EQUAL(52, lot52_node("a26_2")->x);
    TEST_ASSERT_EQUAL(24, lot52_node("a26_3")->w);
    TEST_ASSERT_EQUAL(78, lot52_node("a26_3")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l77_grid_px_27(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:59px;grid-template-columns:repeat(2,28px);column-gap:3px'><div id=a27_0 style='height:8px'></div><div id=a27_1 style='height:8px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(2, lot52_node("g")->grid_cols);
    TEST_ASSERT_EQUAL(28, lot52_node("a27_0")->w);
    TEST_ASSERT_EQUAL(0, lot52_node("a27_0")->x);
    TEST_ASSERT_EQUAL(28, lot52_node("a27_1")->w);
    TEST_ASSERT_EQUAL(31, lot52_node("a27_1")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l77_grid_px_28(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:104px;grid-template-columns:repeat(3,32px);column-gap:4px'><div id=a28_0 style='height:8px'></div><div id=a28_1 style='height:8px'></div><div id=a28_2 style='height:8px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(3, lot52_node("g")->grid_cols);
    TEST_ASSERT_EQUAL(32, lot52_node("a28_0")->w);
    TEST_ASSERT_EQUAL(0, lot52_node("a28_0")->x);
    TEST_ASSERT_EQUAL(32, lot52_node("a28_1")->w);
    TEST_ASSERT_EQUAL(36, lot52_node("a28_1")->x);
    TEST_ASSERT_EQUAL(32, lot52_node("a28_2")->w);
    TEST_ASSERT_EQUAL(72, lot52_node("a28_2")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l77_grid_px_29(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:159px;grid-template-columns:repeat(4,36px);column-gap:5px'><div id=a29_0 style='height:8px'></div><div id=a29_1 style='height:8px'></div><div id=a29_2 style='height:8px'></div><div id=a29_3 style='height:8px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(4, lot52_node("g")->grid_cols);
    TEST_ASSERT_EQUAL(36, lot52_node("a29_0")->w);
    TEST_ASSERT_EQUAL(0, lot52_node("a29_0")->x);
    TEST_ASSERT_EQUAL(36, lot52_node("a29_1")->w);
    TEST_ASSERT_EQUAL(41, lot52_node("a29_1")->x);
    TEST_ASSERT_EQUAL(36, lot52_node("a29_2")->w);
    TEST_ASSERT_EQUAL(82, lot52_node("a29_2")->x);
    TEST_ASSERT_EQUAL(36, lot52_node("a29_3")->w);
    TEST_ASSERT_EQUAL(123, lot52_node("a29_3")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l77_grid_px_30(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;grid-template-columns:repeat(2,20px);column-gap:0px'><div id=a30_0 style='height:8px'></div><div id=a30_1 style='height:8px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(2, lot52_node("g")->grid_cols);
    TEST_ASSERT_EQUAL(20, lot52_node("a30_0")->w);
    TEST_ASSERT_EQUAL(0, lot52_node("a30_0")->x);
    TEST_ASSERT_EQUAL(20, lot52_node("a30_1")->w);
    TEST_ASSERT_EQUAL(20, lot52_node("a30_1")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l77_grid_px_31(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:74px;grid-template-columns:repeat(3,24px);column-gap:1px'><div id=a31_0 style='height:8px'></div><div id=a31_1 style='height:8px'></div><div id=a31_2 style='height:8px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(3, lot52_node("g")->grid_cols);
    TEST_ASSERT_EQUAL(24, lot52_node("a31_0")->w);
    TEST_ASSERT_EQUAL(0, lot52_node("a31_0")->x);
    TEST_ASSERT_EQUAL(24, lot52_node("a31_1")->w);
    TEST_ASSERT_EQUAL(25, lot52_node("a31_1")->x);
    TEST_ASSERT_EQUAL(24, lot52_node("a31_2")->w);
    TEST_ASSERT_EQUAL(50, lot52_node("a31_2")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l77_grid_px_32(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:118px;grid-template-columns:repeat(4,28px);column-gap:2px'><div id=a32_0 style='height:8px'></div><div id=a32_1 style='height:8px'></div><div id=a32_2 style='height:8px'></div><div id=a32_3 style='height:8px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(4, lot52_node("g")->grid_cols);
    TEST_ASSERT_EQUAL(28, lot52_node("a32_0")->w);
    TEST_ASSERT_EQUAL(0, lot52_node("a32_0")->x);
    TEST_ASSERT_EQUAL(28, lot52_node("a32_1")->w);
    TEST_ASSERT_EQUAL(30, lot52_node("a32_1")->x);
    TEST_ASSERT_EQUAL(28, lot52_node("a32_2")->w);
    TEST_ASSERT_EQUAL(60, lot52_node("a32_2")->x);
    TEST_ASSERT_EQUAL(28, lot52_node("a32_3")->w);
    TEST_ASSERT_EQUAL(90, lot52_node("a32_3")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l77_grid_px_33(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:67px;grid-template-columns:repeat(2,32px);column-gap:3px'><div id=a33_0 style='height:8px'></div><div id=a33_1 style='height:8px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(2, lot52_node("g")->grid_cols);
    TEST_ASSERT_EQUAL(32, lot52_node("a33_0")->w);
    TEST_ASSERT_EQUAL(0, lot52_node("a33_0")->x);
    TEST_ASSERT_EQUAL(32, lot52_node("a33_1")->w);
    TEST_ASSERT_EQUAL(35, lot52_node("a33_1")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l77_grid_px_34(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:116px;grid-template-columns:repeat(3,36px);column-gap:4px'><div id=a34_0 style='height:8px'></div><div id=a34_1 style='height:8px'></div><div id=a34_2 style='height:8px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(3, lot52_node("g")->grid_cols);
    TEST_ASSERT_EQUAL(36, lot52_node("a34_0")->w);
    TEST_ASSERT_EQUAL(0, lot52_node("a34_0")->x);
    TEST_ASSERT_EQUAL(36, lot52_node("a34_1")->w);
    TEST_ASSERT_EQUAL(40, lot52_node("a34_1")->x);
    TEST_ASSERT_EQUAL(36, lot52_node("a34_2")->w);
    TEST_ASSERT_EQUAL(80, lot52_node("a34_2")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l77_grid_px_35(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:95px;grid-template-columns:repeat(4,20px);column-gap:5px'><div id=a35_0 style='height:8px'></div><div id=a35_1 style='height:8px'></div><div id=a35_2 style='height:8px'></div><div id=a35_3 style='height:8px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(4, lot52_node("g")->grid_cols);
    TEST_ASSERT_EQUAL(20, lot52_node("a35_0")->w);
    TEST_ASSERT_EQUAL(0, lot52_node("a35_0")->x);
    TEST_ASSERT_EQUAL(20, lot52_node("a35_1")->w);
    TEST_ASSERT_EQUAL(25, lot52_node("a35_1")->x);
    TEST_ASSERT_EQUAL(20, lot52_node("a35_2")->w);
    TEST_ASSERT_EQUAL(50, lot52_node("a35_2")->x);
    TEST_ASSERT_EQUAL(20, lot52_node("a35_3")->w);
    TEST_ASSERT_EQUAL(75, lot52_node("a35_3")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l77_grid_px_36(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:48px;grid-template-columns:repeat(2,24px);column-gap:0px'><div id=a36_0 style='height:8px'></div><div id=a36_1 style='height:8px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(2, lot52_node("g")->grid_cols);
    TEST_ASSERT_EQUAL(24, lot52_node("a36_0")->w);
    TEST_ASSERT_EQUAL(0, lot52_node("a36_0")->x);
    TEST_ASSERT_EQUAL(24, lot52_node("a36_1")->w);
    TEST_ASSERT_EQUAL(24, lot52_node("a36_1")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l77_grid_px_37(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:86px;grid-template-columns:repeat(3,28px);column-gap:1px'><div id=a37_0 style='height:8px'></div><div id=a37_1 style='height:8px'></div><div id=a37_2 style='height:8px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(3, lot52_node("g")->grid_cols);
    TEST_ASSERT_EQUAL(28, lot52_node("a37_0")->w);
    TEST_ASSERT_EQUAL(0, lot52_node("a37_0")->x);
    TEST_ASSERT_EQUAL(28, lot52_node("a37_1")->w);
    TEST_ASSERT_EQUAL(29, lot52_node("a37_1")->x);
    TEST_ASSERT_EQUAL(28, lot52_node("a37_2")->w);
    TEST_ASSERT_EQUAL(58, lot52_node("a37_2")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l77_grid_px_38(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:134px;grid-template-columns:repeat(4,32px);column-gap:2px'><div id=a38_0 style='height:8px'></div><div id=a38_1 style='height:8px'></div><div id=a38_2 style='height:8px'></div><div id=a38_3 style='height:8px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(4, lot52_node("g")->grid_cols);
    TEST_ASSERT_EQUAL(32, lot52_node("a38_0")->w);
    TEST_ASSERT_EQUAL(0, lot52_node("a38_0")->x);
    TEST_ASSERT_EQUAL(32, lot52_node("a38_1")->w);
    TEST_ASSERT_EQUAL(34, lot52_node("a38_1")->x);
    TEST_ASSERT_EQUAL(32, lot52_node("a38_2")->w);
    TEST_ASSERT_EQUAL(68, lot52_node("a38_2")->x);
    TEST_ASSERT_EQUAL(32, lot52_node("a38_3")->w);
    TEST_ASSERT_EQUAL(102, lot52_node("a38_3")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l77_grid_px_39(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:75px;grid-template-columns:repeat(2,36px);column-gap:3px'><div id=a39_0 style='height:8px'></div><div id=a39_1 style='height:8px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(2, lot52_node("g")->grid_cols);
    TEST_ASSERT_EQUAL(36, lot52_node("a39_0")->w);
    TEST_ASSERT_EQUAL(0, lot52_node("a39_0")->x);
    TEST_ASSERT_EQUAL(36, lot52_node("a39_1")->w);
    TEST_ASSERT_EQUAL(39, lot52_node("a39_1")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l77_grid_px_40(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:68px;grid-template-columns:repeat(3,20px);column-gap:4px'><div id=a40_0 style='height:8px'></div><div id=a40_1 style='height:8px'></div><div id=a40_2 style='height:8px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(3, lot52_node("g")->grid_cols);
    TEST_ASSERT_EQUAL(20, lot52_node("a40_0")->w);
    TEST_ASSERT_EQUAL(0, lot52_node("a40_0")->x);
    TEST_ASSERT_EQUAL(20, lot52_node("a40_1")->w);
    TEST_ASSERT_EQUAL(24, lot52_node("a40_1")->x);
    TEST_ASSERT_EQUAL(20, lot52_node("a40_2")->w);
    TEST_ASSERT_EQUAL(48, lot52_node("a40_2")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l77_grid_px_41(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:111px;grid-template-columns:repeat(4,24px);column-gap:5px'><div id=a41_0 style='height:8px'></div><div id=a41_1 style='height:8px'></div><div id=a41_2 style='height:8px'></div><div id=a41_3 style='height:8px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(4, lot52_node("g")->grid_cols);
    TEST_ASSERT_EQUAL(24, lot52_node("a41_0")->w);
    TEST_ASSERT_EQUAL(0, lot52_node("a41_0")->x);
    TEST_ASSERT_EQUAL(24, lot52_node("a41_1")->w);
    TEST_ASSERT_EQUAL(29, lot52_node("a41_1")->x);
    TEST_ASSERT_EQUAL(24, lot52_node("a41_2")->w);
    TEST_ASSERT_EQUAL(58, lot52_node("a41_2")->x);
    TEST_ASSERT_EQUAL(24, lot52_node("a41_3")->w);
    TEST_ASSERT_EQUAL(87, lot52_node("a41_3")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l77_grid_px_42(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:56px;grid-template-columns:repeat(2,28px);column-gap:0px'><div id=a42_0 style='height:8px'></div><div id=a42_1 style='height:8px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(2, lot52_node("g")->grid_cols);
    TEST_ASSERT_EQUAL(28, lot52_node("a42_0")->w);
    TEST_ASSERT_EQUAL(0, lot52_node("a42_0")->x);
    TEST_ASSERT_EQUAL(28, lot52_node("a42_1")->w);
    TEST_ASSERT_EQUAL(28, lot52_node("a42_1")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l77_grid_px_43(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:98px;grid-template-columns:repeat(3,32px);column-gap:1px'><div id=a43_0 style='height:8px'></div><div id=a43_1 style='height:8px'></div><div id=a43_2 style='height:8px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(3, lot52_node("g")->grid_cols);
    TEST_ASSERT_EQUAL(32, lot52_node("a43_0")->w);
    TEST_ASSERT_EQUAL(0, lot52_node("a43_0")->x);
    TEST_ASSERT_EQUAL(32, lot52_node("a43_1")->w);
    TEST_ASSERT_EQUAL(33, lot52_node("a43_1")->x);
    TEST_ASSERT_EQUAL(32, lot52_node("a43_2")->w);
    TEST_ASSERT_EQUAL(66, lot52_node("a43_2")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l77_grid_px_44(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:150px;grid-template-columns:repeat(4,36px);column-gap:2px'><div id=a44_0 style='height:8px'></div><div id=a44_1 style='height:8px'></div><div id=a44_2 style='height:8px'></div><div id=a44_3 style='height:8px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(4, lot52_node("g")->grid_cols);
    TEST_ASSERT_EQUAL(36, lot52_node("a44_0")->w);
    TEST_ASSERT_EQUAL(0, lot52_node("a44_0")->x);
    TEST_ASSERT_EQUAL(36, lot52_node("a44_1")->w);
    TEST_ASSERT_EQUAL(38, lot52_node("a44_1")->x);
    TEST_ASSERT_EQUAL(36, lot52_node("a44_2")->w);
    TEST_ASSERT_EQUAL(76, lot52_node("a44_2")->x);
    TEST_ASSERT_EQUAL(36, lot52_node("a44_3")->w);
    TEST_ASSERT_EQUAL(114, lot52_node("a44_3")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l77_grid_px_45(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:43px;grid-template-columns:repeat(2,20px);column-gap:3px'><div id=a45_0 style='height:8px'></div><div id=a45_1 style='height:8px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(2, lot52_node("g")->grid_cols);
    TEST_ASSERT_EQUAL(20, lot52_node("a45_0")->w);
    TEST_ASSERT_EQUAL(0, lot52_node("a45_0")->x);
    TEST_ASSERT_EQUAL(20, lot52_node("a45_1")->w);
    TEST_ASSERT_EQUAL(23, lot52_node("a45_1")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l77_grid_px_46(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:80px;grid-template-columns:repeat(3,24px);column-gap:4px'><div id=a46_0 style='height:8px'></div><div id=a46_1 style='height:8px'></div><div id=a46_2 style='height:8px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(3, lot52_node("g")->grid_cols);
    TEST_ASSERT_EQUAL(24, lot52_node("a46_0")->w);
    TEST_ASSERT_EQUAL(0, lot52_node("a46_0")->x);
    TEST_ASSERT_EQUAL(24, lot52_node("a46_1")->w);
    TEST_ASSERT_EQUAL(28, lot52_node("a46_1")->x);
    TEST_ASSERT_EQUAL(24, lot52_node("a46_2")->w);
    TEST_ASSERT_EQUAL(56, lot52_node("a46_2")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l77_grid_px_47(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:127px;grid-template-columns:repeat(4,28px);column-gap:5px'><div id=a47_0 style='height:8px'></div><div id=a47_1 style='height:8px'></div><div id=a47_2 style='height:8px'></div><div id=a47_3 style='height:8px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(4, lot52_node("g")->grid_cols);
    TEST_ASSERT_EQUAL(28, lot52_node("a47_0")->w);
    TEST_ASSERT_EQUAL(0, lot52_node("a47_0")->x);
    TEST_ASSERT_EQUAL(28, lot52_node("a47_1")->w);
    TEST_ASSERT_EQUAL(33, lot52_node("a47_1")->x);
    TEST_ASSERT_EQUAL(28, lot52_node("a47_2")->w);
    TEST_ASSERT_EQUAL(66, lot52_node("a47_2")->x);
    TEST_ASSERT_EQUAL(28, lot52_node("a47_3")->w);
    TEST_ASSERT_EQUAL(99, lot52_node("a47_3")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l77_grid_px_48(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:64px;grid-template-columns:repeat(2,32px);column-gap:0px'><div id=a48_0 style='height:8px'></div><div id=a48_1 style='height:8px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(2, lot52_node("g")->grid_cols);
    TEST_ASSERT_EQUAL(32, lot52_node("a48_0")->w);
    TEST_ASSERT_EQUAL(0, lot52_node("a48_0")->x);
    TEST_ASSERT_EQUAL(32, lot52_node("a48_1")->w);
    TEST_ASSERT_EQUAL(32, lot52_node("a48_1")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l77_grid_px_49(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:110px;grid-template-columns:repeat(3,36px);column-gap:1px'><div id=a49_0 style='height:8px'></div><div id=a49_1 style='height:8px'></div><div id=a49_2 style='height:8px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(3, lot52_node("g")->grid_cols);
    TEST_ASSERT_EQUAL(36, lot52_node("a49_0")->w);
    TEST_ASSERT_EQUAL(0, lot52_node("a49_0")->x);
    TEST_ASSERT_EQUAL(36, lot52_node("a49_1")->w);
    TEST_ASSERT_EQUAL(37, lot52_node("a49_1")->x);
    TEST_ASSERT_EQUAL(36, lot52_node("a49_2")->w);
    TEST_ASSERT_EQUAL(74, lot52_node("a49_2")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l77_grid_px_50(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:86px;grid-template-columns:repeat(4,20px);column-gap:2px'><div id=a50_0 style='height:8px'></div><div id=a50_1 style='height:8px'></div><div id=a50_2 style='height:8px'></div><div id=a50_3 style='height:8px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(4, lot52_node("g")->grid_cols);
    TEST_ASSERT_EQUAL(20, lot52_node("a50_0")->w);
    TEST_ASSERT_EQUAL(0, lot52_node("a50_0")->x);
    TEST_ASSERT_EQUAL(20, lot52_node("a50_1")->w);
    TEST_ASSERT_EQUAL(22, lot52_node("a50_1")->x);
    TEST_ASSERT_EQUAL(20, lot52_node("a50_2")->w);
    TEST_ASSERT_EQUAL(44, lot52_node("a50_2")->x);
    TEST_ASSERT_EQUAL(20, lot52_node("a50_3")->w);
    TEST_ASSERT_EQUAL(66, lot52_node("a50_3")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l77_grid_px_51(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:51px;grid-template-columns:repeat(2,24px);column-gap:3px'><div id=a51_0 style='height:8px'></div><div id=a51_1 style='height:8px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(2, lot52_node("g")->grid_cols);
    TEST_ASSERT_EQUAL(24, lot52_node("a51_0")->w);
    TEST_ASSERT_EQUAL(0, lot52_node("a51_0")->x);
    TEST_ASSERT_EQUAL(24, lot52_node("a51_1")->w);
    TEST_ASSERT_EQUAL(27, lot52_node("a51_1")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l77_grid_px_52(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:92px;grid-template-columns:repeat(3,28px);column-gap:4px'><div id=a52_0 style='height:8px'></div><div id=a52_1 style='height:8px'></div><div id=a52_2 style='height:8px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(3, lot52_node("g")->grid_cols);
    TEST_ASSERT_EQUAL(28, lot52_node("a52_0")->w);
    TEST_ASSERT_EQUAL(0, lot52_node("a52_0")->x);
    TEST_ASSERT_EQUAL(28, lot52_node("a52_1")->w);
    TEST_ASSERT_EQUAL(32, lot52_node("a52_1")->x);
    TEST_ASSERT_EQUAL(28, lot52_node("a52_2")->w);
    TEST_ASSERT_EQUAL(64, lot52_node("a52_2")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l77_grid_px_53(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:143px;grid-template-columns:repeat(4,32px);column-gap:5px'><div id=a53_0 style='height:8px'></div><div id=a53_1 style='height:8px'></div><div id=a53_2 style='height:8px'></div><div id=a53_3 style='height:8px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(4, lot52_node("g")->grid_cols);
    TEST_ASSERT_EQUAL(32, lot52_node("a53_0")->w);
    TEST_ASSERT_EQUAL(0, lot52_node("a53_0")->x);
    TEST_ASSERT_EQUAL(32, lot52_node("a53_1")->w);
    TEST_ASSERT_EQUAL(37, lot52_node("a53_1")->x);
    TEST_ASSERT_EQUAL(32, lot52_node("a53_2")->w);
    TEST_ASSERT_EQUAL(74, lot52_node("a53_2")->x);
    TEST_ASSERT_EQUAL(32, lot52_node("a53_3")->w);
    TEST_ASSERT_EQUAL(111, lot52_node("a53_3")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l77_grid_px_54(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:72px;grid-template-columns:repeat(2,36px);column-gap:0px'><div id=a54_0 style='height:8px'></div><div id=a54_1 style='height:8px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(2, lot52_node("g")->grid_cols);
    TEST_ASSERT_EQUAL(36, lot52_node("a54_0")->w);
    TEST_ASSERT_EQUAL(0, lot52_node("a54_0")->x);
    TEST_ASSERT_EQUAL(36, lot52_node("a54_1")->w);
    TEST_ASSERT_EQUAL(36, lot52_node("a54_1")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l77_grid_px_55(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:62px;grid-template-columns:repeat(3,20px);column-gap:1px'><div id=a55_0 style='height:8px'></div><div id=a55_1 style='height:8px'></div><div id=a55_2 style='height:8px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(3, lot52_node("g")->grid_cols);
    TEST_ASSERT_EQUAL(20, lot52_node("a55_0")->w);
    TEST_ASSERT_EQUAL(0, lot52_node("a55_0")->x);
    TEST_ASSERT_EQUAL(20, lot52_node("a55_1")->w);
    TEST_ASSERT_EQUAL(21, lot52_node("a55_1")->x);
    TEST_ASSERT_EQUAL(20, lot52_node("a55_2")->w);
    TEST_ASSERT_EQUAL(42, lot52_node("a55_2")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l77_grid_px_56(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:102px;grid-template-columns:repeat(4,24px);column-gap:2px'><div id=a56_0 style='height:8px'></div><div id=a56_1 style='height:8px'></div><div id=a56_2 style='height:8px'></div><div id=a56_3 style='height:8px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(4, lot52_node("g")->grid_cols);
    TEST_ASSERT_EQUAL(24, lot52_node("a56_0")->w);
    TEST_ASSERT_EQUAL(0, lot52_node("a56_0")->x);
    TEST_ASSERT_EQUAL(24, lot52_node("a56_1")->w);
    TEST_ASSERT_EQUAL(26, lot52_node("a56_1")->x);
    TEST_ASSERT_EQUAL(24, lot52_node("a56_2")->w);
    TEST_ASSERT_EQUAL(52, lot52_node("a56_2")->x);
    TEST_ASSERT_EQUAL(24, lot52_node("a56_3")->w);
    TEST_ASSERT_EQUAL(78, lot52_node("a56_3")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l77_grid_px_57(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:59px;grid-template-columns:repeat(2,28px);column-gap:3px'><div id=a57_0 style='height:8px'></div><div id=a57_1 style='height:8px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(2, lot52_node("g")->grid_cols);
    TEST_ASSERT_EQUAL(28, lot52_node("a57_0")->w);
    TEST_ASSERT_EQUAL(0, lot52_node("a57_0")->x);
    TEST_ASSERT_EQUAL(28, lot52_node("a57_1")->w);
    TEST_ASSERT_EQUAL(31, lot52_node("a57_1")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l77_grid_px_58(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:104px;grid-template-columns:repeat(3,32px);column-gap:4px'><div id=a58_0 style='height:8px'></div><div id=a58_1 style='height:8px'></div><div id=a58_2 style='height:8px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(3, lot52_node("g")->grid_cols);
    TEST_ASSERT_EQUAL(32, lot52_node("a58_0")->w);
    TEST_ASSERT_EQUAL(0, lot52_node("a58_0")->x);
    TEST_ASSERT_EQUAL(32, lot52_node("a58_1")->w);
    TEST_ASSERT_EQUAL(36, lot52_node("a58_1")->x);
    TEST_ASSERT_EQUAL(32, lot52_node("a58_2")->w);
    TEST_ASSERT_EQUAL(72, lot52_node("a58_2")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l77_grid_px_59(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:159px;grid-template-columns:repeat(4,36px);column-gap:5px'><div id=a59_0 style='height:8px'></div><div id=a59_1 style='height:8px'></div><div id=a59_2 style='height:8px'></div><div id=a59_3 style='height:8px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(4, lot52_node("g")->grid_cols);
    TEST_ASSERT_EQUAL(36, lot52_node("a59_0")->w);
    TEST_ASSERT_EQUAL(0, lot52_node("a59_0")->x);
    TEST_ASSERT_EQUAL(36, lot52_node("a59_1")->w);
    TEST_ASSERT_EQUAL(41, lot52_node("a59_1")->x);
    TEST_ASSERT_EQUAL(36, lot52_node("a59_2")->w);
    TEST_ASSERT_EQUAL(82, lot52_node("a59_2")->x);
    TEST_ASSERT_EQUAL(36, lot52_node("a59_3")->w);
    TEST_ASSERT_EQUAL(123, lot52_node("a59_3")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l77_grid_px_60(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;grid-template-columns:repeat(2,20px);column-gap:0px'><div id=a60_0 style='height:8px'></div><div id=a60_1 style='height:8px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(2, lot52_node("g")->grid_cols);
    TEST_ASSERT_EQUAL(20, lot52_node("a60_0")->w);
    TEST_ASSERT_EQUAL(0, lot52_node("a60_0")->x);
    TEST_ASSERT_EQUAL(20, lot52_node("a60_1")->w);
    TEST_ASSERT_EQUAL(20, lot52_node("a60_1")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l77_grid_px_61(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:74px;grid-template-columns:repeat(3,24px);column-gap:1px'><div id=a61_0 style='height:8px'></div><div id=a61_1 style='height:8px'></div><div id=a61_2 style='height:8px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(3, lot52_node("g")->grid_cols);
    TEST_ASSERT_EQUAL(24, lot52_node("a61_0")->w);
    TEST_ASSERT_EQUAL(0, lot52_node("a61_0")->x);
    TEST_ASSERT_EQUAL(24, lot52_node("a61_1")->w);
    TEST_ASSERT_EQUAL(25, lot52_node("a61_1")->x);
    TEST_ASSERT_EQUAL(24, lot52_node("a61_2")->w);
    TEST_ASSERT_EQUAL(50, lot52_node("a61_2")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l77_grid_px_62(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:118px;grid-template-columns:repeat(4,28px);column-gap:2px'><div id=a62_0 style='height:8px'></div><div id=a62_1 style='height:8px'></div><div id=a62_2 style='height:8px'></div><div id=a62_3 style='height:8px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(4, lot52_node("g")->grid_cols);
    TEST_ASSERT_EQUAL(28, lot52_node("a62_0")->w);
    TEST_ASSERT_EQUAL(0, lot52_node("a62_0")->x);
    TEST_ASSERT_EQUAL(28, lot52_node("a62_1")->w);
    TEST_ASSERT_EQUAL(30, lot52_node("a62_1")->x);
    TEST_ASSERT_EQUAL(28, lot52_node("a62_2")->w);
    TEST_ASSERT_EQUAL(60, lot52_node("a62_2")->x);
    TEST_ASSERT_EQUAL(28, lot52_node("a62_3")->w);
    TEST_ASSERT_EQUAL(90, lot52_node("a62_3")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l77_grid_px_63(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:67px;grid-template-columns:repeat(2,32px);column-gap:3px'><div id=a63_0 style='height:8px'></div><div id=a63_1 style='height:8px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(2, lot52_node("g")->grid_cols);
    TEST_ASSERT_EQUAL(32, lot52_node("a63_0")->w);
    TEST_ASSERT_EQUAL(0, lot52_node("a63_0")->x);
    TEST_ASSERT_EQUAL(32, lot52_node("a63_1")->w);
    TEST_ASSERT_EQUAL(35, lot52_node("a63_1")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l77_grid_px_64(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:116px;grid-template-columns:repeat(3,36px);column-gap:4px'><div id=a64_0 style='height:8px'></div><div id=a64_1 style='height:8px'></div><div id=a64_2 style='height:8px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(3, lot52_node("g")->grid_cols);
    TEST_ASSERT_EQUAL(36, lot52_node("a64_0")->w);
    TEST_ASSERT_EQUAL(0, lot52_node("a64_0")->x);
    TEST_ASSERT_EQUAL(36, lot52_node("a64_1")->w);
    TEST_ASSERT_EQUAL(40, lot52_node("a64_1")->x);
    TEST_ASSERT_EQUAL(36, lot52_node("a64_2")->w);
    TEST_ASSERT_EQUAL(80, lot52_node("a64_2")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l77_grid_px_65(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:95px;grid-template-columns:repeat(4,20px);column-gap:5px'><div id=a65_0 style='height:8px'></div><div id=a65_1 style='height:8px'></div><div id=a65_2 style='height:8px'></div><div id=a65_3 style='height:8px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(4, lot52_node("g")->grid_cols);
    TEST_ASSERT_EQUAL(20, lot52_node("a65_0")->w);
    TEST_ASSERT_EQUAL(0, lot52_node("a65_0")->x);
    TEST_ASSERT_EQUAL(20, lot52_node("a65_1")->w);
    TEST_ASSERT_EQUAL(25, lot52_node("a65_1")->x);
    TEST_ASSERT_EQUAL(20, lot52_node("a65_2")->w);
    TEST_ASSERT_EQUAL(50, lot52_node("a65_2")->x);
    TEST_ASSERT_EQUAL(20, lot52_node("a65_3")->w);
    TEST_ASSERT_EQUAL(75, lot52_node("a65_3")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l77_grid_px_66(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:48px;grid-template-columns:repeat(2,24px);column-gap:0px'><div id=a66_0 style='height:8px'></div><div id=a66_1 style='height:8px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(2, lot52_node("g")->grid_cols);
    TEST_ASSERT_EQUAL(24, lot52_node("a66_0")->w);
    TEST_ASSERT_EQUAL(0, lot52_node("a66_0")->x);
    TEST_ASSERT_EQUAL(24, lot52_node("a66_1")->w);
    TEST_ASSERT_EQUAL(24, lot52_node("a66_1")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l77_grid_px_67(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:86px;grid-template-columns:repeat(3,28px);column-gap:1px'><div id=a67_0 style='height:8px'></div><div id=a67_1 style='height:8px'></div><div id=a67_2 style='height:8px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(3, lot52_node("g")->grid_cols);
    TEST_ASSERT_EQUAL(28, lot52_node("a67_0")->w);
    TEST_ASSERT_EQUAL(0, lot52_node("a67_0")->x);
    TEST_ASSERT_EQUAL(28, lot52_node("a67_1")->w);
    TEST_ASSERT_EQUAL(29, lot52_node("a67_1")->x);
    TEST_ASSERT_EQUAL(28, lot52_node("a67_2")->w);
    TEST_ASSERT_EQUAL(58, lot52_node("a67_2")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l77_grid_px_68(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:134px;grid-template-columns:repeat(4,32px);column-gap:2px'><div id=a68_0 style='height:8px'></div><div id=a68_1 style='height:8px'></div><div id=a68_2 style='height:8px'></div><div id=a68_3 style='height:8px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(4, lot52_node("g")->grid_cols);
    TEST_ASSERT_EQUAL(32, lot52_node("a68_0")->w);
    TEST_ASSERT_EQUAL(0, lot52_node("a68_0")->x);
    TEST_ASSERT_EQUAL(32, lot52_node("a68_1")->w);
    TEST_ASSERT_EQUAL(34, lot52_node("a68_1")->x);
    TEST_ASSERT_EQUAL(32, lot52_node("a68_2")->w);
    TEST_ASSERT_EQUAL(68, lot52_node("a68_2")->x);
    TEST_ASSERT_EQUAL(32, lot52_node("a68_3")->w);
    TEST_ASSERT_EQUAL(102, lot52_node("a68_3")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l77_grid_px_69(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:75px;grid-template-columns:repeat(2,36px);column-gap:3px'><div id=a69_0 style='height:8px'></div><div id=a69_1 style='height:8px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(2, lot52_node("g")->grid_cols);
    TEST_ASSERT_EQUAL(36, lot52_node("a69_0")->w);
    TEST_ASSERT_EQUAL(0, lot52_node("a69_0")->x);
    TEST_ASSERT_EQUAL(36, lot52_node("a69_1")->w);
    TEST_ASSERT_EQUAL(39, lot52_node("a69_1")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l77_grid_px_70(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:68px;grid-template-columns:repeat(3,20px);column-gap:4px'><div id=a70_0 style='height:8px'></div><div id=a70_1 style='height:8px'></div><div id=a70_2 style='height:8px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(3, lot52_node("g")->grid_cols);
    TEST_ASSERT_EQUAL(20, lot52_node("a70_0")->w);
    TEST_ASSERT_EQUAL(0, lot52_node("a70_0")->x);
    TEST_ASSERT_EQUAL(20, lot52_node("a70_1")->w);
    TEST_ASSERT_EQUAL(24, lot52_node("a70_1")->x);
    TEST_ASSERT_EQUAL(20, lot52_node("a70_2")->w);
    TEST_ASSERT_EQUAL(48, lot52_node("a70_2")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l77_grid_px_71(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:111px;grid-template-columns:repeat(4,24px);column-gap:5px'><div id=a71_0 style='height:8px'></div><div id=a71_1 style='height:8px'></div><div id=a71_2 style='height:8px'></div><div id=a71_3 style='height:8px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(4, lot52_node("g")->grid_cols);
    TEST_ASSERT_EQUAL(24, lot52_node("a71_0")->w);
    TEST_ASSERT_EQUAL(0, lot52_node("a71_0")->x);
    TEST_ASSERT_EQUAL(24, lot52_node("a71_1")->w);
    TEST_ASSERT_EQUAL(29, lot52_node("a71_1")->x);
    TEST_ASSERT_EQUAL(24, lot52_node("a71_2")->w);
    TEST_ASSERT_EQUAL(58, lot52_node("a71_2")->x);
    TEST_ASSERT_EQUAL(24, lot52_node("a71_3")->w);
    TEST_ASSERT_EQUAL(87, lot52_node("a71_3")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l77_grid_px_72(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:56px;grid-template-columns:repeat(2,28px);column-gap:0px'><div id=a72_0 style='height:8px'></div><div id=a72_1 style='height:8px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(2, lot52_node("g")->grid_cols);
    TEST_ASSERT_EQUAL(28, lot52_node("a72_0")->w);
    TEST_ASSERT_EQUAL(0, lot52_node("a72_0")->x);
    TEST_ASSERT_EQUAL(28, lot52_node("a72_1")->w);
    TEST_ASSERT_EQUAL(28, lot52_node("a72_1")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l77_grid_px_73(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:98px;grid-template-columns:repeat(3,32px);column-gap:1px'><div id=a73_0 style='height:8px'></div><div id=a73_1 style='height:8px'></div><div id=a73_2 style='height:8px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(3, lot52_node("g")->grid_cols);
    TEST_ASSERT_EQUAL(32, lot52_node("a73_0")->w);
    TEST_ASSERT_EQUAL(0, lot52_node("a73_0")->x);
    TEST_ASSERT_EQUAL(32, lot52_node("a73_1")->w);
    TEST_ASSERT_EQUAL(33, lot52_node("a73_1")->x);
    TEST_ASSERT_EQUAL(32, lot52_node("a73_2")->w);
    TEST_ASSERT_EQUAL(66, lot52_node("a73_2")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l77_grid_px_74(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:150px;grid-template-columns:repeat(4,36px);column-gap:2px'><div id=a74_0 style='height:8px'></div><div id=a74_1 style='height:8px'></div><div id=a74_2 style='height:8px'></div><div id=a74_3 style='height:8px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(4, lot52_node("g")->grid_cols);
    TEST_ASSERT_EQUAL(36, lot52_node("a74_0")->w);
    TEST_ASSERT_EQUAL(0, lot52_node("a74_0")->x);
    TEST_ASSERT_EQUAL(36, lot52_node("a74_1")->w);
    TEST_ASSERT_EQUAL(38, lot52_node("a74_1")->x);
    TEST_ASSERT_EQUAL(36, lot52_node("a74_2")->w);
    TEST_ASSERT_EQUAL(76, lot52_node("a74_2")->x);
    TEST_ASSERT_EQUAL(36, lot52_node("a74_3")->w);
    TEST_ASSERT_EQUAL(114, lot52_node("a74_3")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l77_grid_px_75(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:43px;grid-template-columns:repeat(2,20px);column-gap:3px'><div id=a75_0 style='height:8px'></div><div id=a75_1 style='height:8px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(2, lot52_node("g")->grid_cols);
    TEST_ASSERT_EQUAL(20, lot52_node("a75_0")->w);
    TEST_ASSERT_EQUAL(0, lot52_node("a75_0")->x);
    TEST_ASSERT_EQUAL(20, lot52_node("a75_1")->w);
    TEST_ASSERT_EQUAL(23, lot52_node("a75_1")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l77_grid_px_76(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:80px;grid-template-columns:repeat(3,24px);column-gap:4px'><div id=a76_0 style='height:8px'></div><div id=a76_1 style='height:8px'></div><div id=a76_2 style='height:8px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(3, lot52_node("g")->grid_cols);
    TEST_ASSERT_EQUAL(24, lot52_node("a76_0")->w);
    TEST_ASSERT_EQUAL(0, lot52_node("a76_0")->x);
    TEST_ASSERT_EQUAL(24, lot52_node("a76_1")->w);
    TEST_ASSERT_EQUAL(28, lot52_node("a76_1")->x);
    TEST_ASSERT_EQUAL(24, lot52_node("a76_2")->w);
    TEST_ASSERT_EQUAL(56, lot52_node("a76_2")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l77_grid_px_77(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:127px;grid-template-columns:repeat(4,28px);column-gap:5px'><div id=a77_0 style='height:8px'></div><div id=a77_1 style='height:8px'></div><div id=a77_2 style='height:8px'></div><div id=a77_3 style='height:8px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(4, lot52_node("g")->grid_cols);
    TEST_ASSERT_EQUAL(28, lot52_node("a77_0")->w);
    TEST_ASSERT_EQUAL(0, lot52_node("a77_0")->x);
    TEST_ASSERT_EQUAL(28, lot52_node("a77_1")->w);
    TEST_ASSERT_EQUAL(33, lot52_node("a77_1")->x);
    TEST_ASSERT_EQUAL(28, lot52_node("a77_2")->w);
    TEST_ASSERT_EQUAL(66, lot52_node("a77_2")->x);
    TEST_ASSERT_EQUAL(28, lot52_node("a77_3")->w);
    TEST_ASSERT_EQUAL(99, lot52_node("a77_3")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l77_grid_px_78(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:64px;grid-template-columns:repeat(2,32px);column-gap:0px'><div id=a78_0 style='height:8px'></div><div id=a78_1 style='height:8px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(2, lot52_node("g")->grid_cols);
    TEST_ASSERT_EQUAL(32, lot52_node("a78_0")->w);
    TEST_ASSERT_EQUAL(0, lot52_node("a78_0")->x);
    TEST_ASSERT_EQUAL(32, lot52_node("a78_1")->w);
    TEST_ASSERT_EQUAL(32, lot52_node("a78_1")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l77_grid_px_79(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:110px;grid-template-columns:repeat(3,36px);column-gap:1px'><div id=a79_0 style='height:8px'></div><div id=a79_1 style='height:8px'></div><div id=a79_2 style='height:8px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(3, lot52_node("g")->grid_cols);
    TEST_ASSERT_EQUAL(36, lot52_node("a79_0")->w);
    TEST_ASSERT_EQUAL(0, lot52_node("a79_0")->x);
    TEST_ASSERT_EQUAL(36, lot52_node("a79_1")->w);
    TEST_ASSERT_EQUAL(37, lot52_node("a79_1")->x);
    TEST_ASSERT_EQUAL(36, lot52_node("a79_2")->w);
    TEST_ASSERT_EQUAL(74, lot52_node("a79_2")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l77_grid_px_80(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:86px;grid-template-columns:repeat(4,20px);column-gap:2px'><div id=a80_0 style='height:8px'></div><div id=a80_1 style='height:8px'></div><div id=a80_2 style='height:8px'></div><div id=a80_3 style='height:8px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(4, lot52_node("g")->grid_cols);
    TEST_ASSERT_EQUAL(20, lot52_node("a80_0")->w);
    TEST_ASSERT_EQUAL(0, lot52_node("a80_0")->x);
    TEST_ASSERT_EQUAL(20, lot52_node("a80_1")->w);
    TEST_ASSERT_EQUAL(22, lot52_node("a80_1")->x);
    TEST_ASSERT_EQUAL(20, lot52_node("a80_2")->w);
    TEST_ASSERT_EQUAL(44, lot52_node("a80_2")->x);
    TEST_ASSERT_EQUAL(20, lot52_node("a80_3")->w);
    TEST_ASSERT_EQUAL(66, lot52_node("a80_3")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l77_grid_fr_1(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:100px;grid-template-columns:repeat(3,1fr)'><div id=b1_0 style='height:8px'></div><div id=b1_1 style='height:8px'></div><div id=b1_2 style='height:8px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(3, lot52_node("g")->grid_cols);
    TEST_ASSERT_EQUAL(34, lot52_node("b1_0")->w);
    TEST_ASSERT_EQUAL(0, lot52_node("b1_0")->x);
    TEST_ASSERT_EQUAL(33, lot52_node("b1_1")->w);
    TEST_ASSERT_EQUAL(34, lot52_node("b1_1")->x);
    TEST_ASSERT_EQUAL(33, lot52_node("b1_2")->w);
    TEST_ASSERT_EQUAL(67, lot52_node("b1_2")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l77_grid_fr_2(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:110px;grid-template-columns:repeat(4,1fr)'><div id=b2_0 style='height:8px'></div><div id=b2_1 style='height:8px'></div><div id=b2_2 style='height:8px'></div><div id=b2_3 style='height:8px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(4, lot52_node("g")->grid_cols);
    TEST_ASSERT_EQUAL(28, lot52_node("b2_0")->w);
    TEST_ASSERT_EQUAL(0, lot52_node("b2_0")->x);
    TEST_ASSERT_EQUAL(28, lot52_node("b2_1")->w);
    TEST_ASSERT_EQUAL(28, lot52_node("b2_1")->x);
    TEST_ASSERT_EQUAL(27, lot52_node("b2_2")->w);
    TEST_ASSERT_EQUAL(56, lot52_node("b2_2")->x);
    TEST_ASSERT_EQUAL(27, lot52_node("b2_3")->w);
    TEST_ASSERT_EQUAL(83, lot52_node("b2_3")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l77_grid_fr_3(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:120px;grid-template-columns:repeat(2,1fr)'><div id=b3_0 style='height:8px'></div><div id=b3_1 style='height:8px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(2, lot52_node("g")->grid_cols);
    TEST_ASSERT_EQUAL(60, lot52_node("b3_0")->w);
    TEST_ASSERT_EQUAL(0, lot52_node("b3_0")->x);
    TEST_ASSERT_EQUAL(60, lot52_node("b3_1")->w);
    TEST_ASSERT_EQUAL(60, lot52_node("b3_1")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l77_grid_fr_4(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:130px;grid-template-columns:repeat(3,1fr)'><div id=b4_0 style='height:8px'></div><div id=b4_1 style='height:8px'></div><div id=b4_2 style='height:8px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(3, lot52_node("g")->grid_cols);
    TEST_ASSERT_EQUAL(44, lot52_node("b4_0")->w);
    TEST_ASSERT_EQUAL(0, lot52_node("b4_0")->x);
    TEST_ASSERT_EQUAL(43, lot52_node("b4_1")->w);
    TEST_ASSERT_EQUAL(44, lot52_node("b4_1")->x);
    TEST_ASSERT_EQUAL(43, lot52_node("b4_2")->w);
    TEST_ASSERT_EQUAL(87, lot52_node("b4_2")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l77_grid_fr_5(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:140px;grid-template-columns:repeat(4,1fr)'><div id=b5_0 style='height:8px'></div><div id=b5_1 style='height:8px'></div><div id=b5_2 style='height:8px'></div><div id=b5_3 style='height:8px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(4, lot52_node("g")->grid_cols);
    TEST_ASSERT_EQUAL(35, lot52_node("b5_0")->w);
    TEST_ASSERT_EQUAL(0, lot52_node("b5_0")->x);
    TEST_ASSERT_EQUAL(35, lot52_node("b5_1")->w);
    TEST_ASSERT_EQUAL(35, lot52_node("b5_1")->x);
    TEST_ASSERT_EQUAL(35, lot52_node("b5_2")->w);
    TEST_ASSERT_EQUAL(70, lot52_node("b5_2")->x);
    TEST_ASSERT_EQUAL(35, lot52_node("b5_3")->w);
    TEST_ASSERT_EQUAL(105, lot52_node("b5_3")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l77_grid_fr_6(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:150px;grid-template-columns:repeat(2,1fr)'><div id=b6_0 style='height:8px'></div><div id=b6_1 style='height:8px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(2, lot52_node("g")->grid_cols);
    TEST_ASSERT_EQUAL(75, lot52_node("b6_0")->w);
    TEST_ASSERT_EQUAL(0, lot52_node("b6_0")->x);
    TEST_ASSERT_EQUAL(75, lot52_node("b6_1")->w);
    TEST_ASSERT_EQUAL(75, lot52_node("b6_1")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l77_grid_fr_7(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:90px;grid-template-columns:repeat(3,1fr)'><div id=b7_0 style='height:8px'></div><div id=b7_1 style='height:8px'></div><div id=b7_2 style='height:8px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(3, lot52_node("g")->grid_cols);
    TEST_ASSERT_EQUAL(30, lot52_node("b7_0")->w);
    TEST_ASSERT_EQUAL(0, lot52_node("b7_0")->x);
    TEST_ASSERT_EQUAL(30, lot52_node("b7_1")->w);
    TEST_ASSERT_EQUAL(30, lot52_node("b7_1")->x);
    TEST_ASSERT_EQUAL(30, lot52_node("b7_2")->w);
    TEST_ASSERT_EQUAL(60, lot52_node("b7_2")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l77_grid_fr_8(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:100px;grid-template-columns:repeat(4,1fr)'><div id=b8_0 style='height:8px'></div><div id=b8_1 style='height:8px'></div><div id=b8_2 style='height:8px'></div><div id=b8_3 style='height:8px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(4, lot52_node("g")->grid_cols);
    TEST_ASSERT_EQUAL(25, lot52_node("b8_0")->w);
    TEST_ASSERT_EQUAL(0, lot52_node("b8_0")->x);
    TEST_ASSERT_EQUAL(25, lot52_node("b8_1")->w);
    TEST_ASSERT_EQUAL(25, lot52_node("b8_1")->x);
    TEST_ASSERT_EQUAL(25, lot52_node("b8_2")->w);
    TEST_ASSERT_EQUAL(50, lot52_node("b8_2")->x);
    TEST_ASSERT_EQUAL(25, lot52_node("b8_3")->w);
    TEST_ASSERT_EQUAL(75, lot52_node("b8_3")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l77_grid_fr_9(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:110px;grid-template-columns:repeat(2,1fr)'><div id=b9_0 style='height:8px'></div><div id=b9_1 style='height:8px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(2, lot52_node("g")->grid_cols);
    TEST_ASSERT_EQUAL(55, lot52_node("b9_0")->w);
    TEST_ASSERT_EQUAL(0, lot52_node("b9_0")->x);
    TEST_ASSERT_EQUAL(55, lot52_node("b9_1")->w);
    TEST_ASSERT_EQUAL(55, lot52_node("b9_1")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l77_grid_fr_10(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:120px;grid-template-columns:repeat(3,1fr)'><div id=b10_0 style='height:8px'></div><div id=b10_1 style='height:8px'></div><div id=b10_2 style='height:8px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(3, lot52_node("g")->grid_cols);
    TEST_ASSERT_EQUAL(40, lot52_node("b10_0")->w);
    TEST_ASSERT_EQUAL(0, lot52_node("b10_0")->x);
    TEST_ASSERT_EQUAL(40, lot52_node("b10_1")->w);
    TEST_ASSERT_EQUAL(40, lot52_node("b10_1")->x);
    TEST_ASSERT_EQUAL(40, lot52_node("b10_2")->w);
    TEST_ASSERT_EQUAL(80, lot52_node("b10_2")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l77_grid_fr_11(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:130px;grid-template-columns:repeat(4,1fr)'><div id=b11_0 style='height:8px'></div><div id=b11_1 style='height:8px'></div><div id=b11_2 style='height:8px'></div><div id=b11_3 style='height:8px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(4, lot52_node("g")->grid_cols);
    TEST_ASSERT_EQUAL(33, lot52_node("b11_0")->w);
    TEST_ASSERT_EQUAL(0, lot52_node("b11_0")->x);
    TEST_ASSERT_EQUAL(33, lot52_node("b11_1")->w);
    TEST_ASSERT_EQUAL(33, lot52_node("b11_1")->x);
    TEST_ASSERT_EQUAL(32, lot52_node("b11_2")->w);
    TEST_ASSERT_EQUAL(66, lot52_node("b11_2")->x);
    TEST_ASSERT_EQUAL(32, lot52_node("b11_3")->w);
    TEST_ASSERT_EQUAL(98, lot52_node("b11_3")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l77_grid_fr_12(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:140px;grid-template-columns:repeat(2,1fr)'><div id=b12_0 style='height:8px'></div><div id=b12_1 style='height:8px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(2, lot52_node("g")->grid_cols);
    TEST_ASSERT_EQUAL(70, lot52_node("b12_0")->w);
    TEST_ASSERT_EQUAL(0, lot52_node("b12_0")->x);
    TEST_ASSERT_EQUAL(70, lot52_node("b12_1")->w);
    TEST_ASSERT_EQUAL(70, lot52_node("b12_1")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l77_grid_fr_13(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:150px;grid-template-columns:repeat(3,1fr)'><div id=b13_0 style='height:8px'></div><div id=b13_1 style='height:8px'></div><div id=b13_2 style='height:8px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(3, lot52_node("g")->grid_cols);
    TEST_ASSERT_EQUAL(50, lot52_node("b13_0")->w);
    TEST_ASSERT_EQUAL(0, lot52_node("b13_0")->x);
    TEST_ASSERT_EQUAL(50, lot52_node("b13_1")->w);
    TEST_ASSERT_EQUAL(50, lot52_node("b13_1")->x);
    TEST_ASSERT_EQUAL(50, lot52_node("b13_2")->w);
    TEST_ASSERT_EQUAL(100, lot52_node("b13_2")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l77_grid_fr_14(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:90px;grid-template-columns:repeat(4,1fr)'><div id=b14_0 style='height:8px'></div><div id=b14_1 style='height:8px'></div><div id=b14_2 style='height:8px'></div><div id=b14_3 style='height:8px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(4, lot52_node("g")->grid_cols);
    TEST_ASSERT_EQUAL(23, lot52_node("b14_0")->w);
    TEST_ASSERT_EQUAL(0, lot52_node("b14_0")->x);
    TEST_ASSERT_EQUAL(23, lot52_node("b14_1")->w);
    TEST_ASSERT_EQUAL(23, lot52_node("b14_1")->x);
    TEST_ASSERT_EQUAL(22, lot52_node("b14_2")->w);
    TEST_ASSERT_EQUAL(46, lot52_node("b14_2")->x);
    TEST_ASSERT_EQUAL(22, lot52_node("b14_3")->w);
    TEST_ASSERT_EQUAL(68, lot52_node("b14_3")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l77_grid_fr_15(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:100px;grid-template-columns:repeat(2,1fr)'><div id=b15_0 style='height:8px'></div><div id=b15_1 style='height:8px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(2, lot52_node("g")->grid_cols);
    TEST_ASSERT_EQUAL(50, lot52_node("b15_0")->w);
    TEST_ASSERT_EQUAL(0, lot52_node("b15_0")->x);
    TEST_ASSERT_EQUAL(50, lot52_node("b15_1")->w);
    TEST_ASSERT_EQUAL(50, lot52_node("b15_1")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l77_grid_fr_16(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:110px;grid-template-columns:repeat(3,1fr)'><div id=b16_0 style='height:8px'></div><div id=b16_1 style='height:8px'></div><div id=b16_2 style='height:8px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(3, lot52_node("g")->grid_cols);
    TEST_ASSERT_EQUAL(37, lot52_node("b16_0")->w);
    TEST_ASSERT_EQUAL(0, lot52_node("b16_0")->x);
    TEST_ASSERT_EQUAL(37, lot52_node("b16_1")->w);
    TEST_ASSERT_EQUAL(37, lot52_node("b16_1")->x);
    TEST_ASSERT_EQUAL(36, lot52_node("b16_2")->w);
    TEST_ASSERT_EQUAL(74, lot52_node("b16_2")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l77_grid_fr_17(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:120px;grid-template-columns:repeat(4,1fr)'><div id=b17_0 style='height:8px'></div><div id=b17_1 style='height:8px'></div><div id=b17_2 style='height:8px'></div><div id=b17_3 style='height:8px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(4, lot52_node("g")->grid_cols);
    TEST_ASSERT_EQUAL(30, lot52_node("b17_0")->w);
    TEST_ASSERT_EQUAL(0, lot52_node("b17_0")->x);
    TEST_ASSERT_EQUAL(30, lot52_node("b17_1")->w);
    TEST_ASSERT_EQUAL(30, lot52_node("b17_1")->x);
    TEST_ASSERT_EQUAL(30, lot52_node("b17_2")->w);
    TEST_ASSERT_EQUAL(60, lot52_node("b17_2")->x);
    TEST_ASSERT_EQUAL(30, lot52_node("b17_3")->w);
    TEST_ASSERT_EQUAL(90, lot52_node("b17_3")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l77_grid_fr_18(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:130px;grid-template-columns:repeat(2,1fr)'><div id=b18_0 style='height:8px'></div><div id=b18_1 style='height:8px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(2, lot52_node("g")->grid_cols);
    TEST_ASSERT_EQUAL(65, lot52_node("b18_0")->w);
    TEST_ASSERT_EQUAL(0, lot52_node("b18_0")->x);
    TEST_ASSERT_EQUAL(65, lot52_node("b18_1")->w);
    TEST_ASSERT_EQUAL(65, lot52_node("b18_1")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l77_grid_fr_19(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:140px;grid-template-columns:repeat(3,1fr)'><div id=b19_0 style='height:8px'></div><div id=b19_1 style='height:8px'></div><div id=b19_2 style='height:8px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(3, lot52_node("g")->grid_cols);
    TEST_ASSERT_EQUAL(47, lot52_node("b19_0")->w);
    TEST_ASSERT_EQUAL(0, lot52_node("b19_0")->x);
    TEST_ASSERT_EQUAL(47, lot52_node("b19_1")->w);
    TEST_ASSERT_EQUAL(47, lot52_node("b19_1")->x);
    TEST_ASSERT_EQUAL(46, lot52_node("b19_2")->w);
    TEST_ASSERT_EQUAL(94, lot52_node("b19_2")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l77_grid_fr_20(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:150px;grid-template-columns:repeat(4,1fr)'><div id=b20_0 style='height:8px'></div><div id=b20_1 style='height:8px'></div><div id=b20_2 style='height:8px'></div><div id=b20_3 style='height:8px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(4, lot52_node("g")->grid_cols);
    TEST_ASSERT_EQUAL(38, lot52_node("b20_0")->w);
    TEST_ASSERT_EQUAL(0, lot52_node("b20_0")->x);
    TEST_ASSERT_EQUAL(38, lot52_node("b20_1")->w);
    TEST_ASSERT_EQUAL(38, lot52_node("b20_1")->x);
    TEST_ASSERT_EQUAL(37, lot52_node("b20_2")->w);
    TEST_ASSERT_EQUAL(76, lot52_node("b20_2")->x);
    TEST_ASSERT_EQUAL(37, lot52_node("b20_3")->w);
    TEST_ASSERT_EQUAL(113, lot52_node("b20_3")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l77_grid_fr_21(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:90px;grid-template-columns:repeat(2,1fr)'><div id=b21_0 style='height:8px'></div><div id=b21_1 style='height:8px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(2, lot52_node("g")->grid_cols);
    TEST_ASSERT_EQUAL(45, lot52_node("b21_0")->w);
    TEST_ASSERT_EQUAL(0, lot52_node("b21_0")->x);
    TEST_ASSERT_EQUAL(45, lot52_node("b21_1")->w);
    TEST_ASSERT_EQUAL(45, lot52_node("b21_1")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l77_grid_fr_22(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:100px;grid-template-columns:repeat(3,1fr)'><div id=b22_0 style='height:8px'></div><div id=b22_1 style='height:8px'></div><div id=b22_2 style='height:8px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(3, lot52_node("g")->grid_cols);
    TEST_ASSERT_EQUAL(34, lot52_node("b22_0")->w);
    TEST_ASSERT_EQUAL(0, lot52_node("b22_0")->x);
    TEST_ASSERT_EQUAL(33, lot52_node("b22_1")->w);
    TEST_ASSERT_EQUAL(34, lot52_node("b22_1")->x);
    TEST_ASSERT_EQUAL(33, lot52_node("b22_2")->w);
    TEST_ASSERT_EQUAL(67, lot52_node("b22_2")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l77_grid_fr_23(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:110px;grid-template-columns:repeat(4,1fr)'><div id=b23_0 style='height:8px'></div><div id=b23_1 style='height:8px'></div><div id=b23_2 style='height:8px'></div><div id=b23_3 style='height:8px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(4, lot52_node("g")->grid_cols);
    TEST_ASSERT_EQUAL(28, lot52_node("b23_0")->w);
    TEST_ASSERT_EQUAL(0, lot52_node("b23_0")->x);
    TEST_ASSERT_EQUAL(28, lot52_node("b23_1")->w);
    TEST_ASSERT_EQUAL(28, lot52_node("b23_1")->x);
    TEST_ASSERT_EQUAL(27, lot52_node("b23_2")->w);
    TEST_ASSERT_EQUAL(56, lot52_node("b23_2")->x);
    TEST_ASSERT_EQUAL(27, lot52_node("b23_3")->w);
    TEST_ASSERT_EQUAL(83, lot52_node("b23_3")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l77_grid_fr_24(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:120px;grid-template-columns:repeat(2,1fr)'><div id=b24_0 style='height:8px'></div><div id=b24_1 style='height:8px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(2, lot52_node("g")->grid_cols);
    TEST_ASSERT_EQUAL(60, lot52_node("b24_0")->w);
    TEST_ASSERT_EQUAL(0, lot52_node("b24_0")->x);
    TEST_ASSERT_EQUAL(60, lot52_node("b24_1")->w);
    TEST_ASSERT_EQUAL(60, lot52_node("b24_1")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l77_grid_fr_25(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:130px;grid-template-columns:repeat(3,1fr)'><div id=b25_0 style='height:8px'></div><div id=b25_1 style='height:8px'></div><div id=b25_2 style='height:8px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(3, lot52_node("g")->grid_cols);
    TEST_ASSERT_EQUAL(44, lot52_node("b25_0")->w);
    TEST_ASSERT_EQUAL(0, lot52_node("b25_0")->x);
    TEST_ASSERT_EQUAL(43, lot52_node("b25_1")->w);
    TEST_ASSERT_EQUAL(44, lot52_node("b25_1")->x);
    TEST_ASSERT_EQUAL(43, lot52_node("b25_2")->w);
    TEST_ASSERT_EQUAL(87, lot52_node("b25_2")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l77_grid_fr_26(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:140px;grid-template-columns:repeat(4,1fr)'><div id=b26_0 style='height:8px'></div><div id=b26_1 style='height:8px'></div><div id=b26_2 style='height:8px'></div><div id=b26_3 style='height:8px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(4, lot52_node("g")->grid_cols);
    TEST_ASSERT_EQUAL(35, lot52_node("b26_0")->w);
    TEST_ASSERT_EQUAL(0, lot52_node("b26_0")->x);
    TEST_ASSERT_EQUAL(35, lot52_node("b26_1")->w);
    TEST_ASSERT_EQUAL(35, lot52_node("b26_1")->x);
    TEST_ASSERT_EQUAL(35, lot52_node("b26_2")->w);
    TEST_ASSERT_EQUAL(70, lot52_node("b26_2")->x);
    TEST_ASSERT_EQUAL(35, lot52_node("b26_3")->w);
    TEST_ASSERT_EQUAL(105, lot52_node("b26_3")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l77_grid_fr_27(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:150px;grid-template-columns:repeat(2,1fr)'><div id=b27_0 style='height:8px'></div><div id=b27_1 style='height:8px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(2, lot52_node("g")->grid_cols);
    TEST_ASSERT_EQUAL(75, lot52_node("b27_0")->w);
    TEST_ASSERT_EQUAL(0, lot52_node("b27_0")->x);
    TEST_ASSERT_EQUAL(75, lot52_node("b27_1")->w);
    TEST_ASSERT_EQUAL(75, lot52_node("b27_1")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l77_grid_fr_28(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:90px;grid-template-columns:repeat(3,1fr)'><div id=b28_0 style='height:8px'></div><div id=b28_1 style='height:8px'></div><div id=b28_2 style='height:8px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(3, lot52_node("g")->grid_cols);
    TEST_ASSERT_EQUAL(30, lot52_node("b28_0")->w);
    TEST_ASSERT_EQUAL(0, lot52_node("b28_0")->x);
    TEST_ASSERT_EQUAL(30, lot52_node("b28_1")->w);
    TEST_ASSERT_EQUAL(30, lot52_node("b28_1")->x);
    TEST_ASSERT_EQUAL(30, lot52_node("b28_2")->w);
    TEST_ASSERT_EQUAL(60, lot52_node("b28_2")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l77_grid_fr_29(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:100px;grid-template-columns:repeat(4,1fr)'><div id=b29_0 style='height:8px'></div><div id=b29_1 style='height:8px'></div><div id=b29_2 style='height:8px'></div><div id=b29_3 style='height:8px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(4, lot52_node("g")->grid_cols);
    TEST_ASSERT_EQUAL(25, lot52_node("b29_0")->w);
    TEST_ASSERT_EQUAL(0, lot52_node("b29_0")->x);
    TEST_ASSERT_EQUAL(25, lot52_node("b29_1")->w);
    TEST_ASSERT_EQUAL(25, lot52_node("b29_1")->x);
    TEST_ASSERT_EQUAL(25, lot52_node("b29_2")->w);
    TEST_ASSERT_EQUAL(50, lot52_node("b29_2")->x);
    TEST_ASSERT_EQUAL(25, lot52_node("b29_3")->w);
    TEST_ASSERT_EQUAL(75, lot52_node("b29_3")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l77_grid_fr_30(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:110px;grid-template-columns:repeat(2,1fr)'><div id=b30_0 style='height:8px'></div><div id=b30_1 style='height:8px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(2, lot52_node("g")->grid_cols);
    TEST_ASSERT_EQUAL(55, lot52_node("b30_0")->w);
    TEST_ASSERT_EQUAL(0, lot52_node("b30_0")->x);
    TEST_ASSERT_EQUAL(55, lot52_node("b30_1")->w);
    TEST_ASSERT_EQUAL(55, lot52_node("b30_1")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l77_grid_fr_31(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:120px;grid-template-columns:repeat(3,1fr)'><div id=b31_0 style='height:8px'></div><div id=b31_1 style='height:8px'></div><div id=b31_2 style='height:8px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(3, lot52_node("g")->grid_cols);
    TEST_ASSERT_EQUAL(40, lot52_node("b31_0")->w);
    TEST_ASSERT_EQUAL(0, lot52_node("b31_0")->x);
    TEST_ASSERT_EQUAL(40, lot52_node("b31_1")->w);
    TEST_ASSERT_EQUAL(40, lot52_node("b31_1")->x);
    TEST_ASSERT_EQUAL(40, lot52_node("b31_2")->w);
    TEST_ASSERT_EQUAL(80, lot52_node("b31_2")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l77_grid_fr_32(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:130px;grid-template-columns:repeat(4,1fr)'><div id=b32_0 style='height:8px'></div><div id=b32_1 style='height:8px'></div><div id=b32_2 style='height:8px'></div><div id=b32_3 style='height:8px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(4, lot52_node("g")->grid_cols);
    TEST_ASSERT_EQUAL(33, lot52_node("b32_0")->w);
    TEST_ASSERT_EQUAL(0, lot52_node("b32_0")->x);
    TEST_ASSERT_EQUAL(33, lot52_node("b32_1")->w);
    TEST_ASSERT_EQUAL(33, lot52_node("b32_1")->x);
    TEST_ASSERT_EQUAL(32, lot52_node("b32_2")->w);
    TEST_ASSERT_EQUAL(66, lot52_node("b32_2")->x);
    TEST_ASSERT_EQUAL(32, lot52_node("b32_3")->w);
    TEST_ASSERT_EQUAL(98, lot52_node("b32_3")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l77_grid_fr_33(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:140px;grid-template-columns:repeat(2,1fr)'><div id=b33_0 style='height:8px'></div><div id=b33_1 style='height:8px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(2, lot52_node("g")->grid_cols);
    TEST_ASSERT_EQUAL(70, lot52_node("b33_0")->w);
    TEST_ASSERT_EQUAL(0, lot52_node("b33_0")->x);
    TEST_ASSERT_EQUAL(70, lot52_node("b33_1")->w);
    TEST_ASSERT_EQUAL(70, lot52_node("b33_1")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l77_grid_fr_34(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:150px;grid-template-columns:repeat(3,1fr)'><div id=b34_0 style='height:8px'></div><div id=b34_1 style='height:8px'></div><div id=b34_2 style='height:8px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(3, lot52_node("g")->grid_cols);
    TEST_ASSERT_EQUAL(50, lot52_node("b34_0")->w);
    TEST_ASSERT_EQUAL(0, lot52_node("b34_0")->x);
    TEST_ASSERT_EQUAL(50, lot52_node("b34_1")->w);
    TEST_ASSERT_EQUAL(50, lot52_node("b34_1")->x);
    TEST_ASSERT_EQUAL(50, lot52_node("b34_2")->w);
    TEST_ASSERT_EQUAL(100, lot52_node("b34_2")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l77_grid_fr_35(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:90px;grid-template-columns:repeat(4,1fr)'><div id=b35_0 style='height:8px'></div><div id=b35_1 style='height:8px'></div><div id=b35_2 style='height:8px'></div><div id=b35_3 style='height:8px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(4, lot52_node("g")->grid_cols);
    TEST_ASSERT_EQUAL(23, lot52_node("b35_0")->w);
    TEST_ASSERT_EQUAL(0, lot52_node("b35_0")->x);
    TEST_ASSERT_EQUAL(23, lot52_node("b35_1")->w);
    TEST_ASSERT_EQUAL(23, lot52_node("b35_1")->x);
    TEST_ASSERT_EQUAL(22, lot52_node("b35_2")->w);
    TEST_ASSERT_EQUAL(46, lot52_node("b35_2")->x);
    TEST_ASSERT_EQUAL(22, lot52_node("b35_3")->w);
    TEST_ASSERT_EQUAL(68, lot52_node("b35_3")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l77_grid_fr_36(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:100px;grid-template-columns:repeat(2,1fr)'><div id=b36_0 style='height:8px'></div><div id=b36_1 style='height:8px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(2, lot52_node("g")->grid_cols);
    TEST_ASSERT_EQUAL(50, lot52_node("b36_0")->w);
    TEST_ASSERT_EQUAL(0, lot52_node("b36_0")->x);
    TEST_ASSERT_EQUAL(50, lot52_node("b36_1")->w);
    TEST_ASSERT_EQUAL(50, lot52_node("b36_1")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l77_grid_fr_37(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:110px;grid-template-columns:repeat(3,1fr)'><div id=b37_0 style='height:8px'></div><div id=b37_1 style='height:8px'></div><div id=b37_2 style='height:8px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(3, lot52_node("g")->grid_cols);
    TEST_ASSERT_EQUAL(37, lot52_node("b37_0")->w);
    TEST_ASSERT_EQUAL(0, lot52_node("b37_0")->x);
    TEST_ASSERT_EQUAL(37, lot52_node("b37_1")->w);
    TEST_ASSERT_EQUAL(37, lot52_node("b37_1")->x);
    TEST_ASSERT_EQUAL(36, lot52_node("b37_2")->w);
    TEST_ASSERT_EQUAL(74, lot52_node("b37_2")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l77_grid_fr_38(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:120px;grid-template-columns:repeat(4,1fr)'><div id=b38_0 style='height:8px'></div><div id=b38_1 style='height:8px'></div><div id=b38_2 style='height:8px'></div><div id=b38_3 style='height:8px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(4, lot52_node("g")->grid_cols);
    TEST_ASSERT_EQUAL(30, lot52_node("b38_0")->w);
    TEST_ASSERT_EQUAL(0, lot52_node("b38_0")->x);
    TEST_ASSERT_EQUAL(30, lot52_node("b38_1")->w);
    TEST_ASSERT_EQUAL(30, lot52_node("b38_1")->x);
    TEST_ASSERT_EQUAL(30, lot52_node("b38_2")->w);
    TEST_ASSERT_EQUAL(60, lot52_node("b38_2")->x);
    TEST_ASSERT_EQUAL(30, lot52_node("b38_3")->w);
    TEST_ASSERT_EQUAL(90, lot52_node("b38_3")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l77_grid_fr_39(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:130px;grid-template-columns:repeat(2,1fr)'><div id=b39_0 style='height:8px'></div><div id=b39_1 style='height:8px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(2, lot52_node("g")->grid_cols);
    TEST_ASSERT_EQUAL(65, lot52_node("b39_0")->w);
    TEST_ASSERT_EQUAL(0, lot52_node("b39_0")->x);
    TEST_ASSERT_EQUAL(65, lot52_node("b39_1")->w);
    TEST_ASSERT_EQUAL(65, lot52_node("b39_1")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l77_grid_fr_40(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:140px;grid-template-columns:repeat(3,1fr)'><div id=b40_0 style='height:8px'></div><div id=b40_1 style='height:8px'></div><div id=b40_2 style='height:8px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(3, lot52_node("g")->grid_cols);
    TEST_ASSERT_EQUAL(47, lot52_node("b40_0")->w);
    TEST_ASSERT_EQUAL(0, lot52_node("b40_0")->x);
    TEST_ASSERT_EQUAL(47, lot52_node("b40_1")->w);
    TEST_ASSERT_EQUAL(47, lot52_node("b40_1")->x);
    TEST_ASSERT_EQUAL(46, lot52_node("b40_2")->w);
    TEST_ASSERT_EQUAL(94, lot52_node("b40_2")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l77_grid_fr_41(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:150px;grid-template-columns:repeat(4,1fr)'><div id=b41_0 style='height:8px'></div><div id=b41_1 style='height:8px'></div><div id=b41_2 style='height:8px'></div><div id=b41_3 style='height:8px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(4, lot52_node("g")->grid_cols);
    TEST_ASSERT_EQUAL(38, lot52_node("b41_0")->w);
    TEST_ASSERT_EQUAL(0, lot52_node("b41_0")->x);
    TEST_ASSERT_EQUAL(38, lot52_node("b41_1")->w);
    TEST_ASSERT_EQUAL(38, lot52_node("b41_1")->x);
    TEST_ASSERT_EQUAL(37, lot52_node("b41_2")->w);
    TEST_ASSERT_EQUAL(76, lot52_node("b41_2")->x);
    TEST_ASSERT_EQUAL(37, lot52_node("b41_3")->w);
    TEST_ASSERT_EQUAL(113, lot52_node("b41_3")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l77_grid_fr_42(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:90px;grid-template-columns:repeat(2,1fr)'><div id=b42_0 style='height:8px'></div><div id=b42_1 style='height:8px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(2, lot52_node("g")->grid_cols);
    TEST_ASSERT_EQUAL(45, lot52_node("b42_0")->w);
    TEST_ASSERT_EQUAL(0, lot52_node("b42_0")->x);
    TEST_ASSERT_EQUAL(45, lot52_node("b42_1")->w);
    TEST_ASSERT_EQUAL(45, lot52_node("b42_1")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l77_grid_fr_43(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:100px;grid-template-columns:repeat(3,1fr)'><div id=b43_0 style='height:8px'></div><div id=b43_1 style='height:8px'></div><div id=b43_2 style='height:8px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(3, lot52_node("g")->grid_cols);
    TEST_ASSERT_EQUAL(34, lot52_node("b43_0")->w);
    TEST_ASSERT_EQUAL(0, lot52_node("b43_0")->x);
    TEST_ASSERT_EQUAL(33, lot52_node("b43_1")->w);
    TEST_ASSERT_EQUAL(34, lot52_node("b43_1")->x);
    TEST_ASSERT_EQUAL(33, lot52_node("b43_2")->w);
    TEST_ASSERT_EQUAL(67, lot52_node("b43_2")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l77_grid_fr_44(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:110px;grid-template-columns:repeat(4,1fr)'><div id=b44_0 style='height:8px'></div><div id=b44_1 style='height:8px'></div><div id=b44_2 style='height:8px'></div><div id=b44_3 style='height:8px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(4, lot52_node("g")->grid_cols);
    TEST_ASSERT_EQUAL(28, lot52_node("b44_0")->w);
    TEST_ASSERT_EQUAL(0, lot52_node("b44_0")->x);
    TEST_ASSERT_EQUAL(28, lot52_node("b44_1")->w);
    TEST_ASSERT_EQUAL(28, lot52_node("b44_1")->x);
    TEST_ASSERT_EQUAL(27, lot52_node("b44_2")->w);
    TEST_ASSERT_EQUAL(56, lot52_node("b44_2")->x);
    TEST_ASSERT_EQUAL(27, lot52_node("b44_3")->w);
    TEST_ASSERT_EQUAL(83, lot52_node("b44_3")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l77_grid_fr_45(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:120px;grid-template-columns:repeat(2,1fr)'><div id=b45_0 style='height:8px'></div><div id=b45_1 style='height:8px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(2, lot52_node("g")->grid_cols);
    TEST_ASSERT_EQUAL(60, lot52_node("b45_0")->w);
    TEST_ASSERT_EQUAL(0, lot52_node("b45_0")->x);
    TEST_ASSERT_EQUAL(60, lot52_node("b45_1")->w);
    TEST_ASSERT_EQUAL(60, lot52_node("b45_1")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l77_grid_fr_46(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:130px;grid-template-columns:repeat(3,1fr)'><div id=b46_0 style='height:8px'></div><div id=b46_1 style='height:8px'></div><div id=b46_2 style='height:8px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(3, lot52_node("g")->grid_cols);
    TEST_ASSERT_EQUAL(44, lot52_node("b46_0")->w);
    TEST_ASSERT_EQUAL(0, lot52_node("b46_0")->x);
    TEST_ASSERT_EQUAL(43, lot52_node("b46_1")->w);
    TEST_ASSERT_EQUAL(44, lot52_node("b46_1")->x);
    TEST_ASSERT_EQUAL(43, lot52_node("b46_2")->w);
    TEST_ASSERT_EQUAL(87, lot52_node("b46_2")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l77_grid_fr_47(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:140px;grid-template-columns:repeat(4,1fr)'><div id=b47_0 style='height:8px'></div><div id=b47_1 style='height:8px'></div><div id=b47_2 style='height:8px'></div><div id=b47_3 style='height:8px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(4, lot52_node("g")->grid_cols);
    TEST_ASSERT_EQUAL(35, lot52_node("b47_0")->w);
    TEST_ASSERT_EQUAL(0, lot52_node("b47_0")->x);
    TEST_ASSERT_EQUAL(35, lot52_node("b47_1")->w);
    TEST_ASSERT_EQUAL(35, lot52_node("b47_1")->x);
    TEST_ASSERT_EQUAL(35, lot52_node("b47_2")->w);
    TEST_ASSERT_EQUAL(70, lot52_node("b47_2")->x);
    TEST_ASSERT_EQUAL(35, lot52_node("b47_3")->w);
    TEST_ASSERT_EQUAL(105, lot52_node("b47_3")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l77_grid_fr_48(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:150px;grid-template-columns:repeat(2,1fr)'><div id=b48_0 style='height:8px'></div><div id=b48_1 style='height:8px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(2, lot52_node("g")->grid_cols);
    TEST_ASSERT_EQUAL(75, lot52_node("b48_0")->w);
    TEST_ASSERT_EQUAL(0, lot52_node("b48_0")->x);
    TEST_ASSERT_EQUAL(75, lot52_node("b48_1")->w);
    TEST_ASSERT_EQUAL(75, lot52_node("b48_1")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l77_grid_fr_49(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:90px;grid-template-columns:repeat(3,1fr)'><div id=b49_0 style='height:8px'></div><div id=b49_1 style='height:8px'></div><div id=b49_2 style='height:8px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(3, lot52_node("g")->grid_cols);
    TEST_ASSERT_EQUAL(30, lot52_node("b49_0")->w);
    TEST_ASSERT_EQUAL(0, lot52_node("b49_0")->x);
    TEST_ASSERT_EQUAL(30, lot52_node("b49_1")->w);
    TEST_ASSERT_EQUAL(30, lot52_node("b49_1")->x);
    TEST_ASSERT_EQUAL(30, lot52_node("b49_2")->w);
    TEST_ASSERT_EQUAL(60, lot52_node("b49_2")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l77_grid_fr_50(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:100px;grid-template-columns:repeat(4,1fr)'><div id=b50_0 style='height:8px'></div><div id=b50_1 style='height:8px'></div><div id=b50_2 style='height:8px'></div><div id=b50_3 style='height:8px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(4, lot52_node("g")->grid_cols);
    TEST_ASSERT_EQUAL(25, lot52_node("b50_0")->w);
    TEST_ASSERT_EQUAL(0, lot52_node("b50_0")->x);
    TEST_ASSERT_EQUAL(25, lot52_node("b50_1")->w);
    TEST_ASSERT_EQUAL(25, lot52_node("b50_1")->x);
    TEST_ASSERT_EQUAL(25, lot52_node("b50_2")->w);
    TEST_ASSERT_EQUAL(50, lot52_node("b50_2")->x);
    TEST_ASSERT_EQUAL(25, lot52_node("b50_3")->w);
    TEST_ASSERT_EQUAL(75, lot52_node("b50_3")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l77_grid_fr_51(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:110px;grid-template-columns:repeat(2,1fr)'><div id=b51_0 style='height:8px'></div><div id=b51_1 style='height:8px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(2, lot52_node("g")->grid_cols);
    TEST_ASSERT_EQUAL(55, lot52_node("b51_0")->w);
    TEST_ASSERT_EQUAL(0, lot52_node("b51_0")->x);
    TEST_ASSERT_EQUAL(55, lot52_node("b51_1")->w);
    TEST_ASSERT_EQUAL(55, lot52_node("b51_1")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l77_grid_fr_52(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:120px;grid-template-columns:repeat(3,1fr)'><div id=b52_0 style='height:8px'></div><div id=b52_1 style='height:8px'></div><div id=b52_2 style='height:8px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(3, lot52_node("g")->grid_cols);
    TEST_ASSERT_EQUAL(40, lot52_node("b52_0")->w);
    TEST_ASSERT_EQUAL(0, lot52_node("b52_0")->x);
    TEST_ASSERT_EQUAL(40, lot52_node("b52_1")->w);
    TEST_ASSERT_EQUAL(40, lot52_node("b52_1")->x);
    TEST_ASSERT_EQUAL(40, lot52_node("b52_2")->w);
    TEST_ASSERT_EQUAL(80, lot52_node("b52_2")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l77_grid_fr_53(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:130px;grid-template-columns:repeat(4,1fr)'><div id=b53_0 style='height:8px'></div><div id=b53_1 style='height:8px'></div><div id=b53_2 style='height:8px'></div><div id=b53_3 style='height:8px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(4, lot52_node("g")->grid_cols);
    TEST_ASSERT_EQUAL(33, lot52_node("b53_0")->w);
    TEST_ASSERT_EQUAL(0, lot52_node("b53_0")->x);
    TEST_ASSERT_EQUAL(33, lot52_node("b53_1")->w);
    TEST_ASSERT_EQUAL(33, lot52_node("b53_1")->x);
    TEST_ASSERT_EQUAL(32, lot52_node("b53_2")->w);
    TEST_ASSERT_EQUAL(66, lot52_node("b53_2")->x);
    TEST_ASSERT_EQUAL(32, lot52_node("b53_3")->w);
    TEST_ASSERT_EQUAL(98, lot52_node("b53_3")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l77_grid_fr_54(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:140px;grid-template-columns:repeat(2,1fr)'><div id=b54_0 style='height:8px'></div><div id=b54_1 style='height:8px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(2, lot52_node("g")->grid_cols);
    TEST_ASSERT_EQUAL(70, lot52_node("b54_0")->w);
    TEST_ASSERT_EQUAL(0, lot52_node("b54_0")->x);
    TEST_ASSERT_EQUAL(70, lot52_node("b54_1")->w);
    TEST_ASSERT_EQUAL(70, lot52_node("b54_1")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l77_grid_fr_55(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:150px;grid-template-columns:repeat(3,1fr)'><div id=b55_0 style='height:8px'></div><div id=b55_1 style='height:8px'></div><div id=b55_2 style='height:8px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(3, lot52_node("g")->grid_cols);
    TEST_ASSERT_EQUAL(50, lot52_node("b55_0")->w);
    TEST_ASSERT_EQUAL(0, lot52_node("b55_0")->x);
    TEST_ASSERT_EQUAL(50, lot52_node("b55_1")->w);
    TEST_ASSERT_EQUAL(50, lot52_node("b55_1")->x);
    TEST_ASSERT_EQUAL(50, lot52_node("b55_2")->w);
    TEST_ASSERT_EQUAL(100, lot52_node("b55_2")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l77_grid_fr_56(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:90px;grid-template-columns:repeat(4,1fr)'><div id=b56_0 style='height:8px'></div><div id=b56_1 style='height:8px'></div><div id=b56_2 style='height:8px'></div><div id=b56_3 style='height:8px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(4, lot52_node("g")->grid_cols);
    TEST_ASSERT_EQUAL(23, lot52_node("b56_0")->w);
    TEST_ASSERT_EQUAL(0, lot52_node("b56_0")->x);
    TEST_ASSERT_EQUAL(23, lot52_node("b56_1")->w);
    TEST_ASSERT_EQUAL(23, lot52_node("b56_1")->x);
    TEST_ASSERT_EQUAL(22, lot52_node("b56_2")->w);
    TEST_ASSERT_EQUAL(46, lot52_node("b56_2")->x);
    TEST_ASSERT_EQUAL(22, lot52_node("b56_3")->w);
    TEST_ASSERT_EQUAL(68, lot52_node("b56_3")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l77_grid_fr_57(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:100px;grid-template-columns:repeat(2,1fr)'><div id=b57_0 style='height:8px'></div><div id=b57_1 style='height:8px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(2, lot52_node("g")->grid_cols);
    TEST_ASSERT_EQUAL(50, lot52_node("b57_0")->w);
    TEST_ASSERT_EQUAL(0, lot52_node("b57_0")->x);
    TEST_ASSERT_EQUAL(50, lot52_node("b57_1")->w);
    TEST_ASSERT_EQUAL(50, lot52_node("b57_1")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l77_grid_fr_58(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:110px;grid-template-columns:repeat(3,1fr)'><div id=b58_0 style='height:8px'></div><div id=b58_1 style='height:8px'></div><div id=b58_2 style='height:8px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(3, lot52_node("g")->grid_cols);
    TEST_ASSERT_EQUAL(37, lot52_node("b58_0")->w);
    TEST_ASSERT_EQUAL(0, lot52_node("b58_0")->x);
    TEST_ASSERT_EQUAL(37, lot52_node("b58_1")->w);
    TEST_ASSERT_EQUAL(37, lot52_node("b58_1")->x);
    TEST_ASSERT_EQUAL(36, lot52_node("b58_2")->w);
    TEST_ASSERT_EQUAL(74, lot52_node("b58_2")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l77_grid_fr_59(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:120px;grid-template-columns:repeat(4,1fr)'><div id=b59_0 style='height:8px'></div><div id=b59_1 style='height:8px'></div><div id=b59_2 style='height:8px'></div><div id=b59_3 style='height:8px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(4, lot52_node("g")->grid_cols);
    TEST_ASSERT_EQUAL(30, lot52_node("b59_0")->w);
    TEST_ASSERT_EQUAL(0, lot52_node("b59_0")->x);
    TEST_ASSERT_EQUAL(30, lot52_node("b59_1")->w);
    TEST_ASSERT_EQUAL(30, lot52_node("b59_1")->x);
    TEST_ASSERT_EQUAL(30, lot52_node("b59_2")->w);
    TEST_ASSERT_EQUAL(60, lot52_node("b59_2")->x);
    TEST_ASSERT_EQUAL(30, lot52_node("b59_3")->w);
    TEST_ASSERT_EQUAL(90, lot52_node("b59_3")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l77_grid_fr_60(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:130px;grid-template-columns:repeat(2,1fr)'><div id=b60_0 style='height:8px'></div><div id=b60_1 style='height:8px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(2, lot52_node("g")->grid_cols);
    TEST_ASSERT_EQUAL(65, lot52_node("b60_0")->w);
    TEST_ASSERT_EQUAL(0, lot52_node("b60_0")->x);
    TEST_ASSERT_EQUAL(65, lot52_node("b60_1")->w);
    TEST_ASSERT_EQUAL(65, lot52_node("b60_1")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l77_grid_fr_61(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:140px;grid-template-columns:repeat(3,1fr)'><div id=b61_0 style='height:8px'></div><div id=b61_1 style='height:8px'></div><div id=b61_2 style='height:8px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(3, lot52_node("g")->grid_cols);
    TEST_ASSERT_EQUAL(47, lot52_node("b61_0")->w);
    TEST_ASSERT_EQUAL(0, lot52_node("b61_0")->x);
    TEST_ASSERT_EQUAL(47, lot52_node("b61_1")->w);
    TEST_ASSERT_EQUAL(47, lot52_node("b61_1")->x);
    TEST_ASSERT_EQUAL(46, lot52_node("b61_2")->w);
    TEST_ASSERT_EQUAL(94, lot52_node("b61_2")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l77_grid_fr_62(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:150px;grid-template-columns:repeat(4,1fr)'><div id=b62_0 style='height:8px'></div><div id=b62_1 style='height:8px'></div><div id=b62_2 style='height:8px'></div><div id=b62_3 style='height:8px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(4, lot52_node("g")->grid_cols);
    TEST_ASSERT_EQUAL(38, lot52_node("b62_0")->w);
    TEST_ASSERT_EQUAL(0, lot52_node("b62_0")->x);
    TEST_ASSERT_EQUAL(38, lot52_node("b62_1")->w);
    TEST_ASSERT_EQUAL(38, lot52_node("b62_1")->x);
    TEST_ASSERT_EQUAL(37, lot52_node("b62_2")->w);
    TEST_ASSERT_EQUAL(76, lot52_node("b62_2")->x);
    TEST_ASSERT_EQUAL(37, lot52_node("b62_3")->w);
    TEST_ASSERT_EQUAL(113, lot52_node("b62_3")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l77_grid_fr_63(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:90px;grid-template-columns:repeat(2,1fr)'><div id=b63_0 style='height:8px'></div><div id=b63_1 style='height:8px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(2, lot52_node("g")->grid_cols);
    TEST_ASSERT_EQUAL(45, lot52_node("b63_0")->w);
    TEST_ASSERT_EQUAL(0, lot52_node("b63_0")->x);
    TEST_ASSERT_EQUAL(45, lot52_node("b63_1")->w);
    TEST_ASSERT_EQUAL(45, lot52_node("b63_1")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l77_grid_fr_64(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:100px;grid-template-columns:repeat(3,1fr)'><div id=b64_0 style='height:8px'></div><div id=b64_1 style='height:8px'></div><div id=b64_2 style='height:8px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(3, lot52_node("g")->grid_cols);
    TEST_ASSERT_EQUAL(34, lot52_node("b64_0")->w);
    TEST_ASSERT_EQUAL(0, lot52_node("b64_0")->x);
    TEST_ASSERT_EQUAL(33, lot52_node("b64_1")->w);
    TEST_ASSERT_EQUAL(34, lot52_node("b64_1")->x);
    TEST_ASSERT_EQUAL(33, lot52_node("b64_2")->w);
    TEST_ASSERT_EQUAL(67, lot52_node("b64_2")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l77_grid_fr_65(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:110px;grid-template-columns:repeat(4,1fr)'><div id=b65_0 style='height:8px'></div><div id=b65_1 style='height:8px'></div><div id=b65_2 style='height:8px'></div><div id=b65_3 style='height:8px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(4, lot52_node("g")->grid_cols);
    TEST_ASSERT_EQUAL(28, lot52_node("b65_0")->w);
    TEST_ASSERT_EQUAL(0, lot52_node("b65_0")->x);
    TEST_ASSERT_EQUAL(28, lot52_node("b65_1")->w);
    TEST_ASSERT_EQUAL(28, lot52_node("b65_1")->x);
    TEST_ASSERT_EQUAL(27, lot52_node("b65_2")->w);
    TEST_ASSERT_EQUAL(56, lot52_node("b65_2")->x);
    TEST_ASSERT_EQUAL(27, lot52_node("b65_3")->w);
    TEST_ASSERT_EQUAL(83, lot52_node("b65_3")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l77_grid_fr_66(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:120px;grid-template-columns:repeat(2,1fr)'><div id=b66_0 style='height:8px'></div><div id=b66_1 style='height:8px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(2, lot52_node("g")->grid_cols);
    TEST_ASSERT_EQUAL(60, lot52_node("b66_0")->w);
    TEST_ASSERT_EQUAL(0, lot52_node("b66_0")->x);
    TEST_ASSERT_EQUAL(60, lot52_node("b66_1")->w);
    TEST_ASSERT_EQUAL(60, lot52_node("b66_1")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l77_grid_fr_67(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:130px;grid-template-columns:repeat(3,1fr)'><div id=b67_0 style='height:8px'></div><div id=b67_1 style='height:8px'></div><div id=b67_2 style='height:8px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(3, lot52_node("g")->grid_cols);
    TEST_ASSERT_EQUAL(44, lot52_node("b67_0")->w);
    TEST_ASSERT_EQUAL(0, lot52_node("b67_0")->x);
    TEST_ASSERT_EQUAL(43, lot52_node("b67_1")->w);
    TEST_ASSERT_EQUAL(44, lot52_node("b67_1")->x);
    TEST_ASSERT_EQUAL(43, lot52_node("b67_2")->w);
    TEST_ASSERT_EQUAL(87, lot52_node("b67_2")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l77_grid_fr_68(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:140px;grid-template-columns:repeat(4,1fr)'><div id=b68_0 style='height:8px'></div><div id=b68_1 style='height:8px'></div><div id=b68_2 style='height:8px'></div><div id=b68_3 style='height:8px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(4, lot52_node("g")->grid_cols);
    TEST_ASSERT_EQUAL(35, lot52_node("b68_0")->w);
    TEST_ASSERT_EQUAL(0, lot52_node("b68_0")->x);
    TEST_ASSERT_EQUAL(35, lot52_node("b68_1")->w);
    TEST_ASSERT_EQUAL(35, lot52_node("b68_1")->x);
    TEST_ASSERT_EQUAL(35, lot52_node("b68_2")->w);
    TEST_ASSERT_EQUAL(70, lot52_node("b68_2")->x);
    TEST_ASSERT_EQUAL(35, lot52_node("b68_3")->w);
    TEST_ASSERT_EQUAL(105, lot52_node("b68_3")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l77_grid_fr_69(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:150px;grid-template-columns:repeat(2,1fr)'><div id=b69_0 style='height:8px'></div><div id=b69_1 style='height:8px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(2, lot52_node("g")->grid_cols);
    TEST_ASSERT_EQUAL(75, lot52_node("b69_0")->w);
    TEST_ASSERT_EQUAL(0, lot52_node("b69_0")->x);
    TEST_ASSERT_EQUAL(75, lot52_node("b69_1")->w);
    TEST_ASSERT_EQUAL(75, lot52_node("b69_1")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l77_grid_fr_70(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:90px;grid-template-columns:repeat(3,1fr)'><div id=b70_0 style='height:8px'></div><div id=b70_1 style='height:8px'></div><div id=b70_2 style='height:8px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(3, lot52_node("g")->grid_cols);
    TEST_ASSERT_EQUAL(30, lot52_node("b70_0")->w);
    TEST_ASSERT_EQUAL(0, lot52_node("b70_0")->x);
    TEST_ASSERT_EQUAL(30, lot52_node("b70_1")->w);
    TEST_ASSERT_EQUAL(30, lot52_node("b70_1")->x);
    TEST_ASSERT_EQUAL(30, lot52_node("b70_2")->w);
    TEST_ASSERT_EQUAL(60, lot52_node("b70_2")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l77_grid_fr_71(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:100px;grid-template-columns:repeat(4,1fr)'><div id=b71_0 style='height:8px'></div><div id=b71_1 style='height:8px'></div><div id=b71_2 style='height:8px'></div><div id=b71_3 style='height:8px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(4, lot52_node("g")->grid_cols);
    TEST_ASSERT_EQUAL(25, lot52_node("b71_0")->w);
    TEST_ASSERT_EQUAL(0, lot52_node("b71_0")->x);
    TEST_ASSERT_EQUAL(25, lot52_node("b71_1")->w);
    TEST_ASSERT_EQUAL(25, lot52_node("b71_1")->x);
    TEST_ASSERT_EQUAL(25, lot52_node("b71_2")->w);
    TEST_ASSERT_EQUAL(50, lot52_node("b71_2")->x);
    TEST_ASSERT_EQUAL(25, lot52_node("b71_3")->w);
    TEST_ASSERT_EQUAL(75, lot52_node("b71_3")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l77_grid_fr_72(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:110px;grid-template-columns:repeat(2,1fr)'><div id=b72_0 style='height:8px'></div><div id=b72_1 style='height:8px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(2, lot52_node("g")->grid_cols);
    TEST_ASSERT_EQUAL(55, lot52_node("b72_0")->w);
    TEST_ASSERT_EQUAL(0, lot52_node("b72_0")->x);
    TEST_ASSERT_EQUAL(55, lot52_node("b72_1")->w);
    TEST_ASSERT_EQUAL(55, lot52_node("b72_1")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l77_grid_fr_73(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:120px;grid-template-columns:repeat(3,1fr)'><div id=b73_0 style='height:8px'></div><div id=b73_1 style='height:8px'></div><div id=b73_2 style='height:8px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(3, lot52_node("g")->grid_cols);
    TEST_ASSERT_EQUAL(40, lot52_node("b73_0")->w);
    TEST_ASSERT_EQUAL(0, lot52_node("b73_0")->x);
    TEST_ASSERT_EQUAL(40, lot52_node("b73_1")->w);
    TEST_ASSERT_EQUAL(40, lot52_node("b73_1")->x);
    TEST_ASSERT_EQUAL(40, lot52_node("b73_2")->w);
    TEST_ASSERT_EQUAL(80, lot52_node("b73_2")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l77_grid_fr_74(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:130px;grid-template-columns:repeat(4,1fr)'><div id=b74_0 style='height:8px'></div><div id=b74_1 style='height:8px'></div><div id=b74_2 style='height:8px'></div><div id=b74_3 style='height:8px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(4, lot52_node("g")->grid_cols);
    TEST_ASSERT_EQUAL(33, lot52_node("b74_0")->w);
    TEST_ASSERT_EQUAL(0, lot52_node("b74_0")->x);
    TEST_ASSERT_EQUAL(33, lot52_node("b74_1")->w);
    TEST_ASSERT_EQUAL(33, lot52_node("b74_1")->x);
    TEST_ASSERT_EQUAL(32, lot52_node("b74_2")->w);
    TEST_ASSERT_EQUAL(66, lot52_node("b74_2")->x);
    TEST_ASSERT_EQUAL(32, lot52_node("b74_3")->w);
    TEST_ASSERT_EQUAL(98, lot52_node("b74_3")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l77_grid_fr_75(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:140px;grid-template-columns:repeat(2,1fr)'><div id=b75_0 style='height:8px'></div><div id=b75_1 style='height:8px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(2, lot52_node("g")->grid_cols);
    TEST_ASSERT_EQUAL(70, lot52_node("b75_0")->w);
    TEST_ASSERT_EQUAL(0, lot52_node("b75_0")->x);
    TEST_ASSERT_EQUAL(70, lot52_node("b75_1")->w);
    TEST_ASSERT_EQUAL(70, lot52_node("b75_1")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l77_grid_fr_76(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:150px;grid-template-columns:repeat(3,1fr)'><div id=b76_0 style='height:8px'></div><div id=b76_1 style='height:8px'></div><div id=b76_2 style='height:8px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(3, lot52_node("g")->grid_cols);
    TEST_ASSERT_EQUAL(50, lot52_node("b76_0")->w);
    TEST_ASSERT_EQUAL(0, lot52_node("b76_0")->x);
    TEST_ASSERT_EQUAL(50, lot52_node("b76_1")->w);
    TEST_ASSERT_EQUAL(50, lot52_node("b76_1")->x);
    TEST_ASSERT_EQUAL(50, lot52_node("b76_2")->w);
    TEST_ASSERT_EQUAL(100, lot52_node("b76_2")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l77_grid_fr_77(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:90px;grid-template-columns:repeat(4,1fr)'><div id=b77_0 style='height:8px'></div><div id=b77_1 style='height:8px'></div><div id=b77_2 style='height:8px'></div><div id=b77_3 style='height:8px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(4, lot52_node("g")->grid_cols);
    TEST_ASSERT_EQUAL(23, lot52_node("b77_0")->w);
    TEST_ASSERT_EQUAL(0, lot52_node("b77_0")->x);
    TEST_ASSERT_EQUAL(23, lot52_node("b77_1")->w);
    TEST_ASSERT_EQUAL(23, lot52_node("b77_1")->x);
    TEST_ASSERT_EQUAL(22, lot52_node("b77_2")->w);
    TEST_ASSERT_EQUAL(46, lot52_node("b77_2")->x);
    TEST_ASSERT_EQUAL(22, lot52_node("b77_3")->w);
    TEST_ASSERT_EQUAL(68, lot52_node("b77_3")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l77_grid_fr_78(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:100px;grid-template-columns:repeat(2,1fr)'><div id=b78_0 style='height:8px'></div><div id=b78_1 style='height:8px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(2, lot52_node("g")->grid_cols);
    TEST_ASSERT_EQUAL(50, lot52_node("b78_0")->w);
    TEST_ASSERT_EQUAL(0, lot52_node("b78_0")->x);
    TEST_ASSERT_EQUAL(50, lot52_node("b78_1")->w);
    TEST_ASSERT_EQUAL(50, lot52_node("b78_1")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l77_grid_fr_79(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:110px;grid-template-columns:repeat(3,1fr)'><div id=b79_0 style='height:8px'></div><div id=b79_1 style='height:8px'></div><div id=b79_2 style='height:8px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(3, lot52_node("g")->grid_cols);
    TEST_ASSERT_EQUAL(37, lot52_node("b79_0")->w);
    TEST_ASSERT_EQUAL(0, lot52_node("b79_0")->x);
    TEST_ASSERT_EQUAL(37, lot52_node("b79_1")->w);
    TEST_ASSERT_EQUAL(37, lot52_node("b79_1")->x);
    TEST_ASSERT_EQUAL(36, lot52_node("b79_2")->w);
    TEST_ASSERT_EQUAL(74, lot52_node("b79_2")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l77_grid_fr_80(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:120px;grid-template-columns:repeat(4,1fr)'><div id=b80_0 style='height:8px'></div><div id=b80_1 style='height:8px'></div><div id=b80_2 style='height:8px'></div><div id=b80_3 style='height:8px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(4, lot52_node("g")->grid_cols);
    TEST_ASSERT_EQUAL(30, lot52_node("b80_0")->w);
    TEST_ASSERT_EQUAL(0, lot52_node("b80_0")->x);
    TEST_ASSERT_EQUAL(30, lot52_node("b80_1")->w);
    TEST_ASSERT_EQUAL(30, lot52_node("b80_1")->x);
    TEST_ASSERT_EQUAL(30, lot52_node("b80_2")->w);
    TEST_ASSERT_EQUAL(60, lot52_node("b80_2")->x);
    TEST_ASSERT_EQUAL(30, lot52_node("b80_3")->w);
    TEST_ASSERT_EQUAL(90, lot52_node("b80_3")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l77_grid_mix_1(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:120px;grid-template-columns:18px 1fr'><div id=c1_0 style='height:8px'></div><div id=c1_1 style='height:8px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(2, lot52_node("g")->grid_cols);
    TEST_ASSERT_EQUAL(18, lot52_node("c1_0")->w);
    TEST_ASSERT_EQUAL(102, lot52_node("c1_1")->w);
    TEST_ASSERT_EQUAL(18, lot52_node("c1_1")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l77_grid_mix_2(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:140px;grid-template-columns:20px 1fr'><div id=c2_0 style='height:8px'></div><div id=c2_1 style='height:8px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(2, lot52_node("g")->grid_cols);
    TEST_ASSERT_EQUAL(20, lot52_node("c2_0")->w);
    TEST_ASSERT_EQUAL(120, lot52_node("c2_1")->w);
    TEST_ASSERT_EQUAL(20, lot52_node("c2_1")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l77_grid_mix_3(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:160px;grid-template-columns:22px 1fr'><div id=c3_0 style='height:8px'></div><div id=c3_1 style='height:8px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(2, lot52_node("g")->grid_cols);
    TEST_ASSERT_EQUAL(22, lot52_node("c3_0")->w);
    TEST_ASSERT_EQUAL(138, lot52_node("c3_1")->w);
    TEST_ASSERT_EQUAL(22, lot52_node("c3_1")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l77_grid_mix_4(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:180px;grid-template-columns:24px 1fr'><div id=c4_0 style='height:8px'></div><div id=c4_1 style='height:8px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(2, lot52_node("g")->grid_cols);
    TEST_ASSERT_EQUAL(24, lot52_node("c4_0")->w);
    TEST_ASSERT_EQUAL(156, lot52_node("c4_1")->w);
    TEST_ASSERT_EQUAL(24, lot52_node("c4_1")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l77_grid_mix_5(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:100px;grid-template-columns:26px 1fr'><div id=c5_0 style='height:8px'></div><div id=c5_1 style='height:8px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(2, lot52_node("g")->grid_cols);
    TEST_ASSERT_EQUAL(26, lot52_node("c5_0")->w);
    TEST_ASSERT_EQUAL(74, lot52_node("c5_1")->w);
    TEST_ASSERT_EQUAL(26, lot52_node("c5_1")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l77_grid_mix_6(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:120px;grid-template-columns:28px 1fr'><div id=c6_0 style='height:8px'></div><div id=c6_1 style='height:8px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(2, lot52_node("g")->grid_cols);
    TEST_ASSERT_EQUAL(28, lot52_node("c6_0")->w);
    TEST_ASSERT_EQUAL(92, lot52_node("c6_1")->w);
    TEST_ASSERT_EQUAL(28, lot52_node("c6_1")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l77_grid_mix_7(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:140px;grid-template-columns:30px 1fr'><div id=c7_0 style='height:8px'></div><div id=c7_1 style='height:8px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(2, lot52_node("g")->grid_cols);
    TEST_ASSERT_EQUAL(30, lot52_node("c7_0")->w);
    TEST_ASSERT_EQUAL(110, lot52_node("c7_1")->w);
    TEST_ASSERT_EQUAL(30, lot52_node("c7_1")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l77_grid_mix_8(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:160px;grid-template-columns:16px 1fr'><div id=c8_0 style='height:8px'></div><div id=c8_1 style='height:8px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(2, lot52_node("g")->grid_cols);
    TEST_ASSERT_EQUAL(16, lot52_node("c8_0")->w);
    TEST_ASSERT_EQUAL(144, lot52_node("c8_1")->w);
    TEST_ASSERT_EQUAL(16, lot52_node("c8_1")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l77_grid_mix_9(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:180px;grid-template-columns:18px 1fr'><div id=c9_0 style='height:8px'></div><div id=c9_1 style='height:8px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(2, lot52_node("g")->grid_cols);
    TEST_ASSERT_EQUAL(18, lot52_node("c9_0")->w);
    TEST_ASSERT_EQUAL(162, lot52_node("c9_1")->w);
    TEST_ASSERT_EQUAL(18, lot52_node("c9_1")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l77_grid_mix_10(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:100px;grid-template-columns:20px 1fr'><div id=c10_0 style='height:8px'></div><div id=c10_1 style='height:8px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(2, lot52_node("g")->grid_cols);
    TEST_ASSERT_EQUAL(20, lot52_node("c10_0")->w);
    TEST_ASSERT_EQUAL(80, lot52_node("c10_1")->w);
    TEST_ASSERT_EQUAL(20, lot52_node("c10_1")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l77_grid_mix_11(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:120px;grid-template-columns:22px 1fr'><div id=c11_0 style='height:8px'></div><div id=c11_1 style='height:8px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(2, lot52_node("g")->grid_cols);
    TEST_ASSERT_EQUAL(22, lot52_node("c11_0")->w);
    TEST_ASSERT_EQUAL(98, lot52_node("c11_1")->w);
    TEST_ASSERT_EQUAL(22, lot52_node("c11_1")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l77_grid_mix_12(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:140px;grid-template-columns:24px 1fr'><div id=c12_0 style='height:8px'></div><div id=c12_1 style='height:8px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(2, lot52_node("g")->grid_cols);
    TEST_ASSERT_EQUAL(24, lot52_node("c12_0")->w);
    TEST_ASSERT_EQUAL(116, lot52_node("c12_1")->w);
    TEST_ASSERT_EQUAL(24, lot52_node("c12_1")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l77_grid_mix_13(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:160px;grid-template-columns:26px 1fr'><div id=c13_0 style='height:8px'></div><div id=c13_1 style='height:8px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(2, lot52_node("g")->grid_cols);
    TEST_ASSERT_EQUAL(26, lot52_node("c13_0")->w);
    TEST_ASSERT_EQUAL(134, lot52_node("c13_1")->w);
    TEST_ASSERT_EQUAL(26, lot52_node("c13_1")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l77_grid_mix_14(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:180px;grid-template-columns:28px 1fr'><div id=c14_0 style='height:8px'></div><div id=c14_1 style='height:8px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(2, lot52_node("g")->grid_cols);
    TEST_ASSERT_EQUAL(28, lot52_node("c14_0")->w);
    TEST_ASSERT_EQUAL(152, lot52_node("c14_1")->w);
    TEST_ASSERT_EQUAL(28, lot52_node("c14_1")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l77_grid_mix_15(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:100px;grid-template-columns:30px 1fr'><div id=c15_0 style='height:8px'></div><div id=c15_1 style='height:8px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(2, lot52_node("g")->grid_cols);
    TEST_ASSERT_EQUAL(30, lot52_node("c15_0")->w);
    TEST_ASSERT_EQUAL(70, lot52_node("c15_1")->w);
    TEST_ASSERT_EQUAL(30, lot52_node("c15_1")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l77_grid_mix_16(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:120px;grid-template-columns:16px 1fr'><div id=c16_0 style='height:8px'></div><div id=c16_1 style='height:8px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(2, lot52_node("g")->grid_cols);
    TEST_ASSERT_EQUAL(16, lot52_node("c16_0")->w);
    TEST_ASSERT_EQUAL(104, lot52_node("c16_1")->w);
    TEST_ASSERT_EQUAL(16, lot52_node("c16_1")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l77_grid_mix_17(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:140px;grid-template-columns:18px 1fr'><div id=c17_0 style='height:8px'></div><div id=c17_1 style='height:8px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(2, lot52_node("g")->grid_cols);
    TEST_ASSERT_EQUAL(18, lot52_node("c17_0")->w);
    TEST_ASSERT_EQUAL(122, lot52_node("c17_1")->w);
    TEST_ASSERT_EQUAL(18, lot52_node("c17_1")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l77_grid_mix_18(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:160px;grid-template-columns:20px 1fr'><div id=c18_0 style='height:8px'></div><div id=c18_1 style='height:8px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(2, lot52_node("g")->grid_cols);
    TEST_ASSERT_EQUAL(20, lot52_node("c18_0")->w);
    TEST_ASSERT_EQUAL(140, lot52_node("c18_1")->w);
    TEST_ASSERT_EQUAL(20, lot52_node("c18_1")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l77_grid_mix_19(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:180px;grid-template-columns:22px 1fr'><div id=c19_0 style='height:8px'></div><div id=c19_1 style='height:8px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(2, lot52_node("g")->grid_cols);
    TEST_ASSERT_EQUAL(22, lot52_node("c19_0")->w);
    TEST_ASSERT_EQUAL(158, lot52_node("c19_1")->w);
    TEST_ASSERT_EQUAL(22, lot52_node("c19_1")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l77_grid_mix_20(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:100px;grid-template-columns:24px 1fr'><div id=c20_0 style='height:8px'></div><div id=c20_1 style='height:8px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(2, lot52_node("g")->grid_cols);
    TEST_ASSERT_EQUAL(24, lot52_node("c20_0")->w);
    TEST_ASSERT_EQUAL(76, lot52_node("c20_1")->w);
    TEST_ASSERT_EQUAL(24, lot52_node("c20_1")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l77_grid_mix_21(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:120px;grid-template-columns:26px 1fr'><div id=c21_0 style='height:8px'></div><div id=c21_1 style='height:8px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(2, lot52_node("g")->grid_cols);
    TEST_ASSERT_EQUAL(26, lot52_node("c21_0")->w);
    TEST_ASSERT_EQUAL(94, lot52_node("c21_1")->w);
    TEST_ASSERT_EQUAL(26, lot52_node("c21_1")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l77_grid_mix_22(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:140px;grid-template-columns:28px 1fr'><div id=c22_0 style='height:8px'></div><div id=c22_1 style='height:8px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(2, lot52_node("g")->grid_cols);
    TEST_ASSERT_EQUAL(28, lot52_node("c22_0")->w);
    TEST_ASSERT_EQUAL(112, lot52_node("c22_1")->w);
    TEST_ASSERT_EQUAL(28, lot52_node("c22_1")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l77_grid_mix_23(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:160px;grid-template-columns:30px 1fr'><div id=c23_0 style='height:8px'></div><div id=c23_1 style='height:8px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(2, lot52_node("g")->grid_cols);
    TEST_ASSERT_EQUAL(30, lot52_node("c23_0")->w);
    TEST_ASSERT_EQUAL(130, lot52_node("c23_1")->w);
    TEST_ASSERT_EQUAL(30, lot52_node("c23_1")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l77_grid_mix_24(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:180px;grid-template-columns:16px 1fr'><div id=c24_0 style='height:8px'></div><div id=c24_1 style='height:8px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(2, lot52_node("g")->grid_cols);
    TEST_ASSERT_EQUAL(16, lot52_node("c24_0")->w);
    TEST_ASSERT_EQUAL(164, lot52_node("c24_1")->w);
    TEST_ASSERT_EQUAL(16, lot52_node("c24_1")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l77_grid_mix_25(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:100px;grid-template-columns:18px 1fr'><div id=c25_0 style='height:8px'></div><div id=c25_1 style='height:8px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(2, lot52_node("g")->grid_cols);
    TEST_ASSERT_EQUAL(18, lot52_node("c25_0")->w);
    TEST_ASSERT_EQUAL(82, lot52_node("c25_1")->w);
    TEST_ASSERT_EQUAL(18, lot52_node("c25_1")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l77_grid_mix_26(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:120px;grid-template-columns:20px 1fr'><div id=c26_0 style='height:8px'></div><div id=c26_1 style='height:8px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(2, lot52_node("g")->grid_cols);
    TEST_ASSERT_EQUAL(20, lot52_node("c26_0")->w);
    TEST_ASSERT_EQUAL(100, lot52_node("c26_1")->w);
    TEST_ASSERT_EQUAL(20, lot52_node("c26_1")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l77_grid_mix_27(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:140px;grid-template-columns:22px 1fr'><div id=c27_0 style='height:8px'></div><div id=c27_1 style='height:8px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(2, lot52_node("g")->grid_cols);
    TEST_ASSERT_EQUAL(22, lot52_node("c27_0")->w);
    TEST_ASSERT_EQUAL(118, lot52_node("c27_1")->w);
    TEST_ASSERT_EQUAL(22, lot52_node("c27_1")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l77_grid_mix_28(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:160px;grid-template-columns:24px 1fr'><div id=c28_0 style='height:8px'></div><div id=c28_1 style='height:8px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(2, lot52_node("g")->grid_cols);
    TEST_ASSERT_EQUAL(24, lot52_node("c28_0")->w);
    TEST_ASSERT_EQUAL(136, lot52_node("c28_1")->w);
    TEST_ASSERT_EQUAL(24, lot52_node("c28_1")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l77_grid_mix_29(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:180px;grid-template-columns:26px 1fr'><div id=c29_0 style='height:8px'></div><div id=c29_1 style='height:8px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(2, lot52_node("g")->grid_cols);
    TEST_ASSERT_EQUAL(26, lot52_node("c29_0")->w);
    TEST_ASSERT_EQUAL(154, lot52_node("c29_1")->w);
    TEST_ASSERT_EQUAL(26, lot52_node("c29_1")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l77_grid_mix_30(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:100px;grid-template-columns:28px 1fr'><div id=c30_0 style='height:8px'></div><div id=c30_1 style='height:8px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(2, lot52_node("g")->grid_cols);
    TEST_ASSERT_EQUAL(28, lot52_node("c30_0")->w);
    TEST_ASSERT_EQUAL(72, lot52_node("c30_1")->w);
    TEST_ASSERT_EQUAL(28, lot52_node("c30_1")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l77_grid_mix_31(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:120px;grid-template-columns:30px 1fr'><div id=c31_0 style='height:8px'></div><div id=c31_1 style='height:8px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(2, lot52_node("g")->grid_cols);
    TEST_ASSERT_EQUAL(30, lot52_node("c31_0")->w);
    TEST_ASSERT_EQUAL(90, lot52_node("c31_1")->w);
    TEST_ASSERT_EQUAL(30, lot52_node("c31_1")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l77_grid_mix_32(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:140px;grid-template-columns:16px 1fr'><div id=c32_0 style='height:8px'></div><div id=c32_1 style='height:8px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(2, lot52_node("g")->grid_cols);
    TEST_ASSERT_EQUAL(16, lot52_node("c32_0")->w);
    TEST_ASSERT_EQUAL(124, lot52_node("c32_1")->w);
    TEST_ASSERT_EQUAL(16, lot52_node("c32_1")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l77_grid_mix_33(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:160px;grid-template-columns:18px 1fr'><div id=c33_0 style='height:8px'></div><div id=c33_1 style='height:8px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(2, lot52_node("g")->grid_cols);
    TEST_ASSERT_EQUAL(18, lot52_node("c33_0")->w);
    TEST_ASSERT_EQUAL(142, lot52_node("c33_1")->w);
    TEST_ASSERT_EQUAL(18, lot52_node("c33_1")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l77_grid_mix_34(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:180px;grid-template-columns:20px 1fr'><div id=c34_0 style='height:8px'></div><div id=c34_1 style='height:8px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(2, lot52_node("g")->grid_cols);
    TEST_ASSERT_EQUAL(20, lot52_node("c34_0")->w);
    TEST_ASSERT_EQUAL(160, lot52_node("c34_1")->w);
    TEST_ASSERT_EQUAL(20, lot52_node("c34_1")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l77_grid_mix_35(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:100px;grid-template-columns:22px 1fr'><div id=c35_0 style='height:8px'></div><div id=c35_1 style='height:8px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(2, lot52_node("g")->grid_cols);
    TEST_ASSERT_EQUAL(22, lot52_node("c35_0")->w);
    TEST_ASSERT_EQUAL(78, lot52_node("c35_1")->w);
    TEST_ASSERT_EQUAL(22, lot52_node("c35_1")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l77_grid_mix_36(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:120px;grid-template-columns:24px 1fr'><div id=c36_0 style='height:8px'></div><div id=c36_1 style='height:8px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(2, lot52_node("g")->grid_cols);
    TEST_ASSERT_EQUAL(24, lot52_node("c36_0")->w);
    TEST_ASSERT_EQUAL(96, lot52_node("c36_1")->w);
    TEST_ASSERT_EQUAL(24, lot52_node("c36_1")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l77_grid_mix_37(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:140px;grid-template-columns:26px 1fr'><div id=c37_0 style='height:8px'></div><div id=c37_1 style='height:8px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(2, lot52_node("g")->grid_cols);
    TEST_ASSERT_EQUAL(26, lot52_node("c37_0")->w);
    TEST_ASSERT_EQUAL(114, lot52_node("c37_1")->w);
    TEST_ASSERT_EQUAL(26, lot52_node("c37_1")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l77_grid_mix_38(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:160px;grid-template-columns:28px 1fr'><div id=c38_0 style='height:8px'></div><div id=c38_1 style='height:8px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(2, lot52_node("g")->grid_cols);
    TEST_ASSERT_EQUAL(28, lot52_node("c38_0")->w);
    TEST_ASSERT_EQUAL(132, lot52_node("c38_1")->w);
    TEST_ASSERT_EQUAL(28, lot52_node("c38_1")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l77_grid_mix_39(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:180px;grid-template-columns:30px 1fr'><div id=c39_0 style='height:8px'></div><div id=c39_1 style='height:8px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(2, lot52_node("g")->grid_cols);
    TEST_ASSERT_EQUAL(30, lot52_node("c39_0")->w);
    TEST_ASSERT_EQUAL(150, lot52_node("c39_1")->w);
    TEST_ASSERT_EQUAL(30, lot52_node("c39_1")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l77_grid_mix_40(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:100px;grid-template-columns:16px 1fr'><div id=c40_0 style='height:8px'></div><div id=c40_1 style='height:8px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(2, lot52_node("g")->grid_cols);
    TEST_ASSERT_EQUAL(16, lot52_node("c40_0")->w);
    TEST_ASSERT_EQUAL(84, lot52_node("c40_1")->w);
    TEST_ASSERT_EQUAL(16, lot52_node("c40_1")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l77_grid_mix_41(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:120px;grid-template-columns:18px 1fr'><div id=c41_0 style='height:8px'></div><div id=c41_1 style='height:8px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(2, lot52_node("g")->grid_cols);
    TEST_ASSERT_EQUAL(18, lot52_node("c41_0")->w);
    TEST_ASSERT_EQUAL(102, lot52_node("c41_1")->w);
    TEST_ASSERT_EQUAL(18, lot52_node("c41_1")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l77_grid_mix_42(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:140px;grid-template-columns:20px 1fr'><div id=c42_0 style='height:8px'></div><div id=c42_1 style='height:8px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(2, lot52_node("g")->grid_cols);
    TEST_ASSERT_EQUAL(20, lot52_node("c42_0")->w);
    TEST_ASSERT_EQUAL(120, lot52_node("c42_1")->w);
    TEST_ASSERT_EQUAL(20, lot52_node("c42_1")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l77_grid_mix_43(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:160px;grid-template-columns:22px 1fr'><div id=c43_0 style='height:8px'></div><div id=c43_1 style='height:8px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(2, lot52_node("g")->grid_cols);
    TEST_ASSERT_EQUAL(22, lot52_node("c43_0")->w);
    TEST_ASSERT_EQUAL(138, lot52_node("c43_1")->w);
    TEST_ASSERT_EQUAL(22, lot52_node("c43_1")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l77_grid_mix_44(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:180px;grid-template-columns:24px 1fr'><div id=c44_0 style='height:8px'></div><div id=c44_1 style='height:8px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(2, lot52_node("g")->grid_cols);
    TEST_ASSERT_EQUAL(24, lot52_node("c44_0")->w);
    TEST_ASSERT_EQUAL(156, lot52_node("c44_1")->w);
    TEST_ASSERT_EQUAL(24, lot52_node("c44_1")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l77_grid_mix_45(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:100px;grid-template-columns:26px 1fr'><div id=c45_0 style='height:8px'></div><div id=c45_1 style='height:8px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(2, lot52_node("g")->grid_cols);
    TEST_ASSERT_EQUAL(26, lot52_node("c45_0")->w);
    TEST_ASSERT_EQUAL(74, lot52_node("c45_1")->w);
    TEST_ASSERT_EQUAL(26, lot52_node("c45_1")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l77_grid_mix_46(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:120px;grid-template-columns:28px 1fr'><div id=c46_0 style='height:8px'></div><div id=c46_1 style='height:8px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(2, lot52_node("g")->grid_cols);
    TEST_ASSERT_EQUAL(28, lot52_node("c46_0")->w);
    TEST_ASSERT_EQUAL(92, lot52_node("c46_1")->w);
    TEST_ASSERT_EQUAL(28, lot52_node("c46_1")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l77_grid_mix_47(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:140px;grid-template-columns:30px 1fr'><div id=c47_0 style='height:8px'></div><div id=c47_1 style='height:8px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(2, lot52_node("g")->grid_cols);
    TEST_ASSERT_EQUAL(30, lot52_node("c47_0")->w);
    TEST_ASSERT_EQUAL(110, lot52_node("c47_1")->w);
    TEST_ASSERT_EQUAL(30, lot52_node("c47_1")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l77_grid_mix_48(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:160px;grid-template-columns:16px 1fr'><div id=c48_0 style='height:8px'></div><div id=c48_1 style='height:8px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(2, lot52_node("g")->grid_cols);
    TEST_ASSERT_EQUAL(16, lot52_node("c48_0")->w);
    TEST_ASSERT_EQUAL(144, lot52_node("c48_1")->w);
    TEST_ASSERT_EQUAL(16, lot52_node("c48_1")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l77_grid_mix_49(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:180px;grid-template-columns:18px 1fr'><div id=c49_0 style='height:8px'></div><div id=c49_1 style='height:8px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(2, lot52_node("g")->grid_cols);
    TEST_ASSERT_EQUAL(18, lot52_node("c49_0")->w);
    TEST_ASSERT_EQUAL(162, lot52_node("c49_1")->w);
    TEST_ASSERT_EQUAL(18, lot52_node("c49_1")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l77_grid_mix_50(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:100px;grid-template-columns:20px 1fr'><div id=c50_0 style='height:8px'></div><div id=c50_1 style='height:8px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(2, lot52_node("g")->grid_cols);
    TEST_ASSERT_EQUAL(20, lot52_node("c50_0")->w);
    TEST_ASSERT_EQUAL(80, lot52_node("c50_1")->w);
    TEST_ASSERT_EQUAL(20, lot52_node("c50_1")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l77_grid_mix_51(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:120px;grid-template-columns:22px 1fr'><div id=c51_0 style='height:8px'></div><div id=c51_1 style='height:8px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(2, lot52_node("g")->grid_cols);
    TEST_ASSERT_EQUAL(22, lot52_node("c51_0")->w);
    TEST_ASSERT_EQUAL(98, lot52_node("c51_1")->w);
    TEST_ASSERT_EQUAL(22, lot52_node("c51_1")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l77_grid_mix_52(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:140px;grid-template-columns:24px 1fr'><div id=c52_0 style='height:8px'></div><div id=c52_1 style='height:8px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(2, lot52_node("g")->grid_cols);
    TEST_ASSERT_EQUAL(24, lot52_node("c52_0")->w);
    TEST_ASSERT_EQUAL(116, lot52_node("c52_1")->w);
    TEST_ASSERT_EQUAL(24, lot52_node("c52_1")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l77_grid_mix_53(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:160px;grid-template-columns:26px 1fr'><div id=c53_0 style='height:8px'></div><div id=c53_1 style='height:8px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(2, lot52_node("g")->grid_cols);
    TEST_ASSERT_EQUAL(26, lot52_node("c53_0")->w);
    TEST_ASSERT_EQUAL(134, lot52_node("c53_1")->w);
    TEST_ASSERT_EQUAL(26, lot52_node("c53_1")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l77_grid_mix_54(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:180px;grid-template-columns:28px 1fr'><div id=c54_0 style='height:8px'></div><div id=c54_1 style='height:8px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(2, lot52_node("g")->grid_cols);
    TEST_ASSERT_EQUAL(28, lot52_node("c54_0")->w);
    TEST_ASSERT_EQUAL(152, lot52_node("c54_1")->w);
    TEST_ASSERT_EQUAL(28, lot52_node("c54_1")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l77_grid_mix_55(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:100px;grid-template-columns:30px 1fr'><div id=c55_0 style='height:8px'></div><div id=c55_1 style='height:8px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(2, lot52_node("g")->grid_cols);
    TEST_ASSERT_EQUAL(30, lot52_node("c55_0")->w);
    TEST_ASSERT_EQUAL(70, lot52_node("c55_1")->w);
    TEST_ASSERT_EQUAL(30, lot52_node("c55_1")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l77_grid_mix_56(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:120px;grid-template-columns:16px 1fr'><div id=c56_0 style='height:8px'></div><div id=c56_1 style='height:8px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(2, lot52_node("g")->grid_cols);
    TEST_ASSERT_EQUAL(16, lot52_node("c56_0")->w);
    TEST_ASSERT_EQUAL(104, lot52_node("c56_1")->w);
    TEST_ASSERT_EQUAL(16, lot52_node("c56_1")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l77_grid_mix_57(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:140px;grid-template-columns:18px 1fr'><div id=c57_0 style='height:8px'></div><div id=c57_1 style='height:8px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(2, lot52_node("g")->grid_cols);
    TEST_ASSERT_EQUAL(18, lot52_node("c57_0")->w);
    TEST_ASSERT_EQUAL(122, lot52_node("c57_1")->w);
    TEST_ASSERT_EQUAL(18, lot52_node("c57_1")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l77_grid_mix_58(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:160px;grid-template-columns:20px 1fr'><div id=c58_0 style='height:8px'></div><div id=c58_1 style='height:8px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(2, lot52_node("g")->grid_cols);
    TEST_ASSERT_EQUAL(20, lot52_node("c58_0")->w);
    TEST_ASSERT_EQUAL(140, lot52_node("c58_1")->w);
    TEST_ASSERT_EQUAL(20, lot52_node("c58_1")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l77_grid_mix_59(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:180px;grid-template-columns:22px 1fr'><div id=c59_0 style='height:8px'></div><div id=c59_1 style='height:8px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(2, lot52_node("g")->grid_cols);
    TEST_ASSERT_EQUAL(22, lot52_node("c59_0")->w);
    TEST_ASSERT_EQUAL(158, lot52_node("c59_1")->w);
    TEST_ASSERT_EQUAL(22, lot52_node("c59_1")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l77_grid_mix_60(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:100px;grid-template-columns:24px 1fr'><div id=c60_0 style='height:8px'></div><div id=c60_1 style='height:8px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(2, lot52_node("g")->grid_cols);
    TEST_ASSERT_EQUAL(24, lot52_node("c60_0")->w);
    TEST_ASSERT_EQUAL(76, lot52_node("c60_1")->w);
    TEST_ASSERT_EQUAL(24, lot52_node("c60_1")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l77_grid_span_1(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:78px;grid-template-columns:repeat(3,24px);column-gap:3px;row-gap:3px'><div id=d1_0 style='height:8px;grid-column:span 2'></div><div id=d1_1 style='height:8px'></div><div id=d1_2 style='height:8px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(51, lot52_node("d1_0")->w);
    TEST_ASSERT_EQUAL(0, lot52_node("d1_0")->x);
    TEST_ASSERT_EQUAL(24, lot52_node("d1_1")->w);
    TEST_ASSERT_EQUAL(54, lot52_node("d1_1")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("d1_2")->x);
    TEST_ASSERT_TRUE(lot52_node("d1_2")->y > lot52_node("d1_0")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l77_grid_span_2(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:80px;grid-template-columns:repeat(3,24px);column-gap:4px;row-gap:4px'><div id=d2_0 style='height:8px;grid-column:span 2'></div><div id=d2_1 style='height:8px'></div><div id=d2_2 style='height:8px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(52, lot52_node("d2_0")->w);
    TEST_ASSERT_EQUAL(0, lot52_node("d2_0")->x);
    TEST_ASSERT_EQUAL(24, lot52_node("d2_1")->w);
    TEST_ASSERT_EQUAL(56, lot52_node("d2_1")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("d2_2")->x);
    TEST_ASSERT_TRUE(lot52_node("d2_2")->y > lot52_node("d2_0")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l77_grid_span_3(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:82px;grid-template-columns:repeat(3,24px);column-gap:5px;row-gap:5px'><div id=d3_0 style='height:8px;grid-column:span 2'></div><div id=d3_1 style='height:8px'></div><div id=d3_2 style='height:8px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(53, lot52_node("d3_0")->w);
    TEST_ASSERT_EQUAL(0, lot52_node("d3_0")->x);
    TEST_ASSERT_EQUAL(24, lot52_node("d3_1")->w);
    TEST_ASSERT_EQUAL(58, lot52_node("d3_1")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("d3_2")->x);
    TEST_ASSERT_TRUE(lot52_node("d3_2")->y > lot52_node("d3_0")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l77_grid_span_4(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:76px;grid-template-columns:repeat(3,24px);column-gap:2px;row-gap:2px'><div id=d4_0 style='height:8px;grid-column:span 2'></div><div id=d4_1 style='height:8px'></div><div id=d4_2 style='height:8px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(50, lot52_node("d4_0")->w);
    TEST_ASSERT_EQUAL(0, lot52_node("d4_0")->x);
    TEST_ASSERT_EQUAL(24, lot52_node("d4_1")->w);
    TEST_ASSERT_EQUAL(52, lot52_node("d4_1")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("d4_2")->x);
    TEST_ASSERT_TRUE(lot52_node("d4_2")->y > lot52_node("d4_0")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l77_grid_span_5(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:78px;grid-template-columns:repeat(3,24px);column-gap:3px;row-gap:3px'><div id=d5_0 style='height:8px;grid-column:span 2'></div><div id=d5_1 style='height:8px'></div><div id=d5_2 style='height:8px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(51, lot52_node("d5_0")->w);
    TEST_ASSERT_EQUAL(0, lot52_node("d5_0")->x);
    TEST_ASSERT_EQUAL(24, lot52_node("d5_1")->w);
    TEST_ASSERT_EQUAL(54, lot52_node("d5_1")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("d5_2")->x);
    TEST_ASSERT_TRUE(lot52_node("d5_2")->y > lot52_node("d5_0")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l77_grid_span_6(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:80px;grid-template-columns:repeat(3,24px);column-gap:4px;row-gap:4px'><div id=d6_0 style='height:8px;grid-column:span 2'></div><div id=d6_1 style='height:8px'></div><div id=d6_2 style='height:8px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(52, lot52_node("d6_0")->w);
    TEST_ASSERT_EQUAL(0, lot52_node("d6_0")->x);
    TEST_ASSERT_EQUAL(24, lot52_node("d6_1")->w);
    TEST_ASSERT_EQUAL(56, lot52_node("d6_1")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("d6_2")->x);
    TEST_ASSERT_TRUE(lot52_node("d6_2")->y > lot52_node("d6_0")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l77_grid_span_7(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:82px;grid-template-columns:repeat(3,24px);column-gap:5px;row-gap:5px'><div id=d7_0 style='height:8px;grid-column:span 2'></div><div id=d7_1 style='height:8px'></div><div id=d7_2 style='height:8px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(53, lot52_node("d7_0")->w);
    TEST_ASSERT_EQUAL(0, lot52_node("d7_0")->x);
    TEST_ASSERT_EQUAL(24, lot52_node("d7_1")->w);
    TEST_ASSERT_EQUAL(58, lot52_node("d7_1")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("d7_2")->x);
    TEST_ASSERT_TRUE(lot52_node("d7_2")->y > lot52_node("d7_0")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l77_grid_span_8(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:76px;grid-template-columns:repeat(3,24px);column-gap:2px;row-gap:2px'><div id=d8_0 style='height:8px;grid-column:span 2'></div><div id=d8_1 style='height:8px'></div><div id=d8_2 style='height:8px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(50, lot52_node("d8_0")->w);
    TEST_ASSERT_EQUAL(0, lot52_node("d8_0")->x);
    TEST_ASSERT_EQUAL(24, lot52_node("d8_1")->w);
    TEST_ASSERT_EQUAL(52, lot52_node("d8_1")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("d8_2")->x);
    TEST_ASSERT_TRUE(lot52_node("d8_2")->y > lot52_node("d8_0")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l77_grid_span_9(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:78px;grid-template-columns:repeat(3,24px);column-gap:3px;row-gap:3px'><div id=d9_0 style='height:8px;grid-column:span 2'></div><div id=d9_1 style='height:8px'></div><div id=d9_2 style='height:8px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(51, lot52_node("d9_0")->w);
    TEST_ASSERT_EQUAL(0, lot52_node("d9_0")->x);
    TEST_ASSERT_EQUAL(24, lot52_node("d9_1")->w);
    TEST_ASSERT_EQUAL(54, lot52_node("d9_1")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("d9_2")->x);
    TEST_ASSERT_TRUE(lot52_node("d9_2")->y > lot52_node("d9_0")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l77_grid_span_10(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:80px;grid-template-columns:repeat(3,24px);column-gap:4px;row-gap:4px'><div id=d10_0 style='height:8px;grid-column:span 2'></div><div id=d10_1 style='height:8px'></div><div id=d10_2 style='height:8px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(52, lot52_node("d10_0")->w);
    TEST_ASSERT_EQUAL(0, lot52_node("d10_0")->x);
    TEST_ASSERT_EQUAL(24, lot52_node("d10_1")->w);
    TEST_ASSERT_EQUAL(56, lot52_node("d10_1")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("d10_2")->x);
    TEST_ASSERT_TRUE(lot52_node("d10_2")->y > lot52_node("d10_0")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l77_grid_span_11(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:82px;grid-template-columns:repeat(3,24px);column-gap:5px;row-gap:5px'><div id=d11_0 style='height:8px;grid-column:span 2'></div><div id=d11_1 style='height:8px'></div><div id=d11_2 style='height:8px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(53, lot52_node("d11_0")->w);
    TEST_ASSERT_EQUAL(0, lot52_node("d11_0")->x);
    TEST_ASSERT_EQUAL(24, lot52_node("d11_1")->w);
    TEST_ASSERT_EQUAL(58, lot52_node("d11_1")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("d11_2")->x);
    TEST_ASSERT_TRUE(lot52_node("d11_2")->y > lot52_node("d11_0")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l77_grid_span_12(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:76px;grid-template-columns:repeat(3,24px);column-gap:2px;row-gap:2px'><div id=d12_0 style='height:8px;grid-column:span 2'></div><div id=d12_1 style='height:8px'></div><div id=d12_2 style='height:8px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(50, lot52_node("d12_0")->w);
    TEST_ASSERT_EQUAL(0, lot52_node("d12_0")->x);
    TEST_ASSERT_EQUAL(24, lot52_node("d12_1")->w);
    TEST_ASSERT_EQUAL(52, lot52_node("d12_1")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("d12_2")->x);
    TEST_ASSERT_TRUE(lot52_node("d12_2")->y > lot52_node("d12_0")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l77_grid_span_13(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:78px;grid-template-columns:repeat(3,24px);column-gap:3px;row-gap:3px'><div id=d13_0 style='height:8px;grid-column:span 2'></div><div id=d13_1 style='height:8px'></div><div id=d13_2 style='height:8px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(51, lot52_node("d13_0")->w);
    TEST_ASSERT_EQUAL(0, lot52_node("d13_0")->x);
    TEST_ASSERT_EQUAL(24, lot52_node("d13_1")->w);
    TEST_ASSERT_EQUAL(54, lot52_node("d13_1")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("d13_2")->x);
    TEST_ASSERT_TRUE(lot52_node("d13_2")->y > lot52_node("d13_0")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l77_grid_span_14(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:80px;grid-template-columns:repeat(3,24px);column-gap:4px;row-gap:4px'><div id=d14_0 style='height:8px;grid-column:span 2'></div><div id=d14_1 style='height:8px'></div><div id=d14_2 style='height:8px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(52, lot52_node("d14_0")->w);
    TEST_ASSERT_EQUAL(0, lot52_node("d14_0")->x);
    TEST_ASSERT_EQUAL(24, lot52_node("d14_1")->w);
    TEST_ASSERT_EQUAL(56, lot52_node("d14_1")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("d14_2")->x);
    TEST_ASSERT_TRUE(lot52_node("d14_2")->y > lot52_node("d14_0")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l77_grid_span_15(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:82px;grid-template-columns:repeat(3,24px);column-gap:5px;row-gap:5px'><div id=d15_0 style='height:8px;grid-column:span 2'></div><div id=d15_1 style='height:8px'></div><div id=d15_2 style='height:8px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(53, lot52_node("d15_0")->w);
    TEST_ASSERT_EQUAL(0, lot52_node("d15_0")->x);
    TEST_ASSERT_EQUAL(24, lot52_node("d15_1")->w);
    TEST_ASSERT_EQUAL(58, lot52_node("d15_1")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("d15_2")->x);
    TEST_ASSERT_TRUE(lot52_node("d15_2")->y > lot52_node("d15_0")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l77_grid_span_16(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:76px;grid-template-columns:repeat(3,24px);column-gap:2px;row-gap:2px'><div id=d16_0 style='height:8px;grid-column:span 2'></div><div id=d16_1 style='height:8px'></div><div id=d16_2 style='height:8px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(50, lot52_node("d16_0")->w);
    TEST_ASSERT_EQUAL(0, lot52_node("d16_0")->x);
    TEST_ASSERT_EQUAL(24, lot52_node("d16_1")->w);
    TEST_ASSERT_EQUAL(52, lot52_node("d16_1")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("d16_2")->x);
    TEST_ASSERT_TRUE(lot52_node("d16_2")->y > lot52_node("d16_0")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l77_grid_span_17(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:78px;grid-template-columns:repeat(3,24px);column-gap:3px;row-gap:3px'><div id=d17_0 style='height:8px;grid-column:span 2'></div><div id=d17_1 style='height:8px'></div><div id=d17_2 style='height:8px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(51, lot52_node("d17_0")->w);
    TEST_ASSERT_EQUAL(0, lot52_node("d17_0")->x);
    TEST_ASSERT_EQUAL(24, lot52_node("d17_1")->w);
    TEST_ASSERT_EQUAL(54, lot52_node("d17_1")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("d17_2")->x);
    TEST_ASSERT_TRUE(lot52_node("d17_2")->y > lot52_node("d17_0")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l77_grid_span_18(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:80px;grid-template-columns:repeat(3,24px);column-gap:4px;row-gap:4px'><div id=d18_0 style='height:8px;grid-column:span 2'></div><div id=d18_1 style='height:8px'></div><div id=d18_2 style='height:8px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(52, lot52_node("d18_0")->w);
    TEST_ASSERT_EQUAL(0, lot52_node("d18_0")->x);
    TEST_ASSERT_EQUAL(24, lot52_node("d18_1")->w);
    TEST_ASSERT_EQUAL(56, lot52_node("d18_1")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("d18_2")->x);
    TEST_ASSERT_TRUE(lot52_node("d18_2")->y > lot52_node("d18_0")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l77_grid_span_19(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:82px;grid-template-columns:repeat(3,24px);column-gap:5px;row-gap:5px'><div id=d19_0 style='height:8px;grid-column:span 2'></div><div id=d19_1 style='height:8px'></div><div id=d19_2 style='height:8px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(53, lot52_node("d19_0")->w);
    TEST_ASSERT_EQUAL(0, lot52_node("d19_0")->x);
    TEST_ASSERT_EQUAL(24, lot52_node("d19_1")->w);
    TEST_ASSERT_EQUAL(58, lot52_node("d19_1")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("d19_2")->x);
    TEST_ASSERT_TRUE(lot52_node("d19_2")->y > lot52_node("d19_0")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l77_grid_span_20(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:76px;grid-template-columns:repeat(3,24px);column-gap:2px;row-gap:2px'><div id=d20_0 style='height:8px;grid-column:span 2'></div><div id=d20_1 style='height:8px'></div><div id=d20_2 style='height:8px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(50, lot52_node("d20_0")->w);
    TEST_ASSERT_EQUAL(0, lot52_node("d20_0")->x);
    TEST_ASSERT_EQUAL(24, lot52_node("d20_1")->w);
    TEST_ASSERT_EQUAL(52, lot52_node("d20_1")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("d20_2")->x);
    TEST_ASSERT_TRUE(lot52_node("d20_2")->y > lot52_node("d20_0")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l77_grid_span_21(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:78px;grid-template-columns:repeat(3,24px);column-gap:3px;row-gap:3px'><div id=d21_0 style='height:8px;grid-column:span 2'></div><div id=d21_1 style='height:8px'></div><div id=d21_2 style='height:8px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(51, lot52_node("d21_0")->w);
    TEST_ASSERT_EQUAL(0, lot52_node("d21_0")->x);
    TEST_ASSERT_EQUAL(24, lot52_node("d21_1")->w);
    TEST_ASSERT_EQUAL(54, lot52_node("d21_1")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("d21_2")->x);
    TEST_ASSERT_TRUE(lot52_node("d21_2")->y > lot52_node("d21_0")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l77_grid_span_22(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:80px;grid-template-columns:repeat(3,24px);column-gap:4px;row-gap:4px'><div id=d22_0 style='height:8px;grid-column:span 2'></div><div id=d22_1 style='height:8px'></div><div id=d22_2 style='height:8px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(52, lot52_node("d22_0")->w);
    TEST_ASSERT_EQUAL(0, lot52_node("d22_0")->x);
    TEST_ASSERT_EQUAL(24, lot52_node("d22_1")->w);
    TEST_ASSERT_EQUAL(56, lot52_node("d22_1")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("d22_2")->x);
    TEST_ASSERT_TRUE(lot52_node("d22_2")->y > lot52_node("d22_0")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l77_grid_span_23(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:82px;grid-template-columns:repeat(3,24px);column-gap:5px;row-gap:5px'><div id=d23_0 style='height:8px;grid-column:span 2'></div><div id=d23_1 style='height:8px'></div><div id=d23_2 style='height:8px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(53, lot52_node("d23_0")->w);
    TEST_ASSERT_EQUAL(0, lot52_node("d23_0")->x);
    TEST_ASSERT_EQUAL(24, lot52_node("d23_1")->w);
    TEST_ASSERT_EQUAL(58, lot52_node("d23_1")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("d23_2")->x);
    TEST_ASSERT_TRUE(lot52_node("d23_2")->y > lot52_node("d23_0")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l77_grid_span_24(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:76px;grid-template-columns:repeat(3,24px);column-gap:2px;row-gap:2px'><div id=d24_0 style='height:8px;grid-column:span 2'></div><div id=d24_1 style='height:8px'></div><div id=d24_2 style='height:8px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(50, lot52_node("d24_0")->w);
    TEST_ASSERT_EQUAL(0, lot52_node("d24_0")->x);
    TEST_ASSERT_EQUAL(24, lot52_node("d24_1")->w);
    TEST_ASSERT_EQUAL(52, lot52_node("d24_1")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("d24_2")->x);
    TEST_ASSERT_TRUE(lot52_node("d24_2")->y > lot52_node("d24_0")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l77_grid_span_25(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:78px;grid-template-columns:repeat(3,24px);column-gap:3px;row-gap:3px'><div id=d25_0 style='height:8px;grid-column:span 2'></div><div id=d25_1 style='height:8px'></div><div id=d25_2 style='height:8px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(51, lot52_node("d25_0")->w);
    TEST_ASSERT_EQUAL(0, lot52_node("d25_0")->x);
    TEST_ASSERT_EQUAL(24, lot52_node("d25_1")->w);
    TEST_ASSERT_EQUAL(54, lot52_node("d25_1")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("d25_2")->x);
    TEST_ASSERT_TRUE(lot52_node("d25_2")->y > lot52_node("d25_0")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l77_grid_span_26(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:80px;grid-template-columns:repeat(3,24px);column-gap:4px;row-gap:4px'><div id=d26_0 style='height:8px;grid-column:span 2'></div><div id=d26_1 style='height:8px'></div><div id=d26_2 style='height:8px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(52, lot52_node("d26_0")->w);
    TEST_ASSERT_EQUAL(0, lot52_node("d26_0")->x);
    TEST_ASSERT_EQUAL(24, lot52_node("d26_1")->w);
    TEST_ASSERT_EQUAL(56, lot52_node("d26_1")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("d26_2")->x);
    TEST_ASSERT_TRUE(lot52_node("d26_2")->y > lot52_node("d26_0")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l77_grid_span_27(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:82px;grid-template-columns:repeat(3,24px);column-gap:5px;row-gap:5px'><div id=d27_0 style='height:8px;grid-column:span 2'></div><div id=d27_1 style='height:8px'></div><div id=d27_2 style='height:8px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(53, lot52_node("d27_0")->w);
    TEST_ASSERT_EQUAL(0, lot52_node("d27_0")->x);
    TEST_ASSERT_EQUAL(24, lot52_node("d27_1")->w);
    TEST_ASSERT_EQUAL(58, lot52_node("d27_1")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("d27_2")->x);
    TEST_ASSERT_TRUE(lot52_node("d27_2")->y > lot52_node("d27_0")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l77_grid_span_28(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:76px;grid-template-columns:repeat(3,24px);column-gap:2px;row-gap:2px'><div id=d28_0 style='height:8px;grid-column:span 2'></div><div id=d28_1 style='height:8px'></div><div id=d28_2 style='height:8px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(50, lot52_node("d28_0")->w);
    TEST_ASSERT_EQUAL(0, lot52_node("d28_0")->x);
    TEST_ASSERT_EQUAL(24, lot52_node("d28_1")->w);
    TEST_ASSERT_EQUAL(52, lot52_node("d28_1")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("d28_2")->x);
    TEST_ASSERT_TRUE(lot52_node("d28_2")->y > lot52_node("d28_0")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l77_grid_span_29(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:78px;grid-template-columns:repeat(3,24px);column-gap:3px;row-gap:3px'><div id=d29_0 style='height:8px;grid-column:span 2'></div><div id=d29_1 style='height:8px'></div><div id=d29_2 style='height:8px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(51, lot52_node("d29_0")->w);
    TEST_ASSERT_EQUAL(0, lot52_node("d29_0")->x);
    TEST_ASSERT_EQUAL(24, lot52_node("d29_1")->w);
    TEST_ASSERT_EQUAL(54, lot52_node("d29_1")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("d29_2")->x);
    TEST_ASSERT_TRUE(lot52_node("d29_2")->y > lot52_node("d29_0")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l77_grid_span_30(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:80px;grid-template-columns:repeat(3,24px);column-gap:4px;row-gap:4px'><div id=d30_0 style='height:8px;grid-column:span 2'></div><div id=d30_1 style='height:8px'></div><div id=d30_2 style='height:8px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(52, lot52_node("d30_0")->w);
    TEST_ASSERT_EQUAL(0, lot52_node("d30_0")->x);
    TEST_ASSERT_EQUAL(24, lot52_node("d30_1")->w);
    TEST_ASSERT_EQUAL(56, lot52_node("d30_1")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("d30_2")->x);
    TEST_ASSERT_TRUE(lot52_node("d30_2")->y > lot52_node("d30_0")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l77_grid_span_31(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:82px;grid-template-columns:repeat(3,24px);column-gap:5px;row-gap:5px'><div id=d31_0 style='height:8px;grid-column:span 2'></div><div id=d31_1 style='height:8px'></div><div id=d31_2 style='height:8px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(53, lot52_node("d31_0")->w);
    TEST_ASSERT_EQUAL(0, lot52_node("d31_0")->x);
    TEST_ASSERT_EQUAL(24, lot52_node("d31_1")->w);
    TEST_ASSERT_EQUAL(58, lot52_node("d31_1")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("d31_2")->x);
    TEST_ASSERT_TRUE(lot52_node("d31_2")->y > lot52_node("d31_0")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l77_grid_span_32(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:76px;grid-template-columns:repeat(3,24px);column-gap:2px;row-gap:2px'><div id=d32_0 style='height:8px;grid-column:span 2'></div><div id=d32_1 style='height:8px'></div><div id=d32_2 style='height:8px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(50, lot52_node("d32_0")->w);
    TEST_ASSERT_EQUAL(0, lot52_node("d32_0")->x);
    TEST_ASSERT_EQUAL(24, lot52_node("d32_1")->w);
    TEST_ASSERT_EQUAL(52, lot52_node("d32_1")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("d32_2")->x);
    TEST_ASSERT_TRUE(lot52_node("d32_2")->y > lot52_node("d32_0")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l77_grid_span_33(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:78px;grid-template-columns:repeat(3,24px);column-gap:3px;row-gap:3px'><div id=d33_0 style='height:8px;grid-column:span 2'></div><div id=d33_1 style='height:8px'></div><div id=d33_2 style='height:8px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(51, lot52_node("d33_0")->w);
    TEST_ASSERT_EQUAL(0, lot52_node("d33_0")->x);
    TEST_ASSERT_EQUAL(24, lot52_node("d33_1")->w);
    TEST_ASSERT_EQUAL(54, lot52_node("d33_1")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("d33_2")->x);
    TEST_ASSERT_TRUE(lot52_node("d33_2")->y > lot52_node("d33_0")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l77_grid_span_34(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:80px;grid-template-columns:repeat(3,24px);column-gap:4px;row-gap:4px'><div id=d34_0 style='height:8px;grid-column:span 2'></div><div id=d34_1 style='height:8px'></div><div id=d34_2 style='height:8px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(52, lot52_node("d34_0")->w);
    TEST_ASSERT_EQUAL(0, lot52_node("d34_0")->x);
    TEST_ASSERT_EQUAL(24, lot52_node("d34_1")->w);
    TEST_ASSERT_EQUAL(56, lot52_node("d34_1")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("d34_2")->x);
    TEST_ASSERT_TRUE(lot52_node("d34_2")->y > lot52_node("d34_0")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l77_grid_span_35(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:82px;grid-template-columns:repeat(3,24px);column-gap:5px;row-gap:5px'><div id=d35_0 style='height:8px;grid-column:span 2'></div><div id=d35_1 style='height:8px'></div><div id=d35_2 style='height:8px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(53, lot52_node("d35_0")->w);
    TEST_ASSERT_EQUAL(0, lot52_node("d35_0")->x);
    TEST_ASSERT_EQUAL(24, lot52_node("d35_1")->w);
    TEST_ASSERT_EQUAL(58, lot52_node("d35_1")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("d35_2")->x);
    TEST_ASSERT_TRUE(lot52_node("d35_2")->y > lot52_node("d35_0")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l77_grid_span_36(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:76px;grid-template-columns:repeat(3,24px);column-gap:2px;row-gap:2px'><div id=d36_0 style='height:8px;grid-column:span 2'></div><div id=d36_1 style='height:8px'></div><div id=d36_2 style='height:8px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(50, lot52_node("d36_0")->w);
    TEST_ASSERT_EQUAL(0, lot52_node("d36_0")->x);
    TEST_ASSERT_EQUAL(24, lot52_node("d36_1")->w);
    TEST_ASSERT_EQUAL(52, lot52_node("d36_1")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("d36_2")->x);
    TEST_ASSERT_TRUE(lot52_node("d36_2")->y > lot52_node("d36_0")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l77_grid_span_37(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:78px;grid-template-columns:repeat(3,24px);column-gap:3px;row-gap:3px'><div id=d37_0 style='height:8px;grid-column:span 2'></div><div id=d37_1 style='height:8px'></div><div id=d37_2 style='height:8px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(51, lot52_node("d37_0")->w);
    TEST_ASSERT_EQUAL(0, lot52_node("d37_0")->x);
    TEST_ASSERT_EQUAL(24, lot52_node("d37_1")->w);
    TEST_ASSERT_EQUAL(54, lot52_node("d37_1")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("d37_2")->x);
    TEST_ASSERT_TRUE(lot52_node("d37_2")->y > lot52_node("d37_0")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l77_grid_span_38(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:80px;grid-template-columns:repeat(3,24px);column-gap:4px;row-gap:4px'><div id=d38_0 style='height:8px;grid-column:span 2'></div><div id=d38_1 style='height:8px'></div><div id=d38_2 style='height:8px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(52, lot52_node("d38_0")->w);
    TEST_ASSERT_EQUAL(0, lot52_node("d38_0")->x);
    TEST_ASSERT_EQUAL(24, lot52_node("d38_1")->w);
    TEST_ASSERT_EQUAL(56, lot52_node("d38_1")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("d38_2")->x);
    TEST_ASSERT_TRUE(lot52_node("d38_2")->y > lot52_node("d38_0")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l77_grid_span_39(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:82px;grid-template-columns:repeat(3,24px);column-gap:5px;row-gap:5px'><div id=d39_0 style='height:8px;grid-column:span 2'></div><div id=d39_1 style='height:8px'></div><div id=d39_2 style='height:8px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(53, lot52_node("d39_0")->w);
    TEST_ASSERT_EQUAL(0, lot52_node("d39_0")->x);
    TEST_ASSERT_EQUAL(24, lot52_node("d39_1")->w);
    TEST_ASSERT_EQUAL(58, lot52_node("d39_1")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("d39_2")->x);
    TEST_ASSERT_TRUE(lot52_node("d39_2")->y > lot52_node("d39_0")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l77_grid_span_40(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:76px;grid-template-columns:repeat(3,24px);column-gap:2px;row-gap:2px'><div id=d40_0 style='height:8px;grid-column:span 2'></div><div id=d40_1 style='height:8px'></div><div id=d40_2 style='height:8px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(50, lot52_node("d40_0")->w);
    TEST_ASSERT_EQUAL(0, lot52_node("d40_0")->x);
    TEST_ASSERT_EQUAL(24, lot52_node("d40_1")->w);
    TEST_ASSERT_EQUAL(52, lot52_node("d40_1")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("d40_2")->x);
    TEST_ASSERT_TRUE(lot52_node("d40_2")->y > lot52_node("d40_0")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l77_grid_span_41(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:78px;grid-template-columns:repeat(3,24px);column-gap:3px;row-gap:3px'><div id=d41_0 style='height:8px;grid-column:span 2'></div><div id=d41_1 style='height:8px'></div><div id=d41_2 style='height:8px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(51, lot52_node("d41_0")->w);
    TEST_ASSERT_EQUAL(0, lot52_node("d41_0")->x);
    TEST_ASSERT_EQUAL(24, lot52_node("d41_1")->w);
    TEST_ASSERT_EQUAL(54, lot52_node("d41_1")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("d41_2")->x);
    TEST_ASSERT_TRUE(lot52_node("d41_2")->y > lot52_node("d41_0")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l77_grid_span_42(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:80px;grid-template-columns:repeat(3,24px);column-gap:4px;row-gap:4px'><div id=d42_0 style='height:8px;grid-column:span 2'></div><div id=d42_1 style='height:8px'></div><div id=d42_2 style='height:8px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(52, lot52_node("d42_0")->w);
    TEST_ASSERT_EQUAL(0, lot52_node("d42_0")->x);
    TEST_ASSERT_EQUAL(24, lot52_node("d42_1")->w);
    TEST_ASSERT_EQUAL(56, lot52_node("d42_1")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("d42_2")->x);
    TEST_ASSERT_TRUE(lot52_node("d42_2")->y > lot52_node("d42_0")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l77_grid_span_43(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:82px;grid-template-columns:repeat(3,24px);column-gap:5px;row-gap:5px'><div id=d43_0 style='height:8px;grid-column:span 2'></div><div id=d43_1 style='height:8px'></div><div id=d43_2 style='height:8px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(53, lot52_node("d43_0")->w);
    TEST_ASSERT_EQUAL(0, lot52_node("d43_0")->x);
    TEST_ASSERT_EQUAL(24, lot52_node("d43_1")->w);
    TEST_ASSERT_EQUAL(58, lot52_node("d43_1")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("d43_2")->x);
    TEST_ASSERT_TRUE(lot52_node("d43_2")->y > lot52_node("d43_0")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l77_grid_span_44(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:76px;grid-template-columns:repeat(3,24px);column-gap:2px;row-gap:2px'><div id=d44_0 style='height:8px;grid-column:span 2'></div><div id=d44_1 style='height:8px'></div><div id=d44_2 style='height:8px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(50, lot52_node("d44_0")->w);
    TEST_ASSERT_EQUAL(0, lot52_node("d44_0")->x);
    TEST_ASSERT_EQUAL(24, lot52_node("d44_1")->w);
    TEST_ASSERT_EQUAL(52, lot52_node("d44_1")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("d44_2")->x);
    TEST_ASSERT_TRUE(lot52_node("d44_2")->y > lot52_node("d44_0")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l77_grid_span_45(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:78px;grid-template-columns:repeat(3,24px);column-gap:3px;row-gap:3px'><div id=d45_0 style='height:8px;grid-column:span 2'></div><div id=d45_1 style='height:8px'></div><div id=d45_2 style='height:8px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(51, lot52_node("d45_0")->w);
    TEST_ASSERT_EQUAL(0, lot52_node("d45_0")->x);
    TEST_ASSERT_EQUAL(24, lot52_node("d45_1")->w);
    TEST_ASSERT_EQUAL(54, lot52_node("d45_1")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("d45_2")->x);
    TEST_ASSERT_TRUE(lot52_node("d45_2")->y > lot52_node("d45_0")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l77_grid_span_46(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:80px;grid-template-columns:repeat(3,24px);column-gap:4px;row-gap:4px'><div id=d46_0 style='height:8px;grid-column:span 2'></div><div id=d46_1 style='height:8px'></div><div id=d46_2 style='height:8px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(52, lot52_node("d46_0")->w);
    TEST_ASSERT_EQUAL(0, lot52_node("d46_0")->x);
    TEST_ASSERT_EQUAL(24, lot52_node("d46_1")->w);
    TEST_ASSERT_EQUAL(56, lot52_node("d46_1")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("d46_2")->x);
    TEST_ASSERT_TRUE(lot52_node("d46_2")->y > lot52_node("d46_0")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l77_grid_span_47(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:82px;grid-template-columns:repeat(3,24px);column-gap:5px;row-gap:5px'><div id=d47_0 style='height:8px;grid-column:span 2'></div><div id=d47_1 style='height:8px'></div><div id=d47_2 style='height:8px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(53, lot52_node("d47_0")->w);
    TEST_ASSERT_EQUAL(0, lot52_node("d47_0")->x);
    TEST_ASSERT_EQUAL(24, lot52_node("d47_1")->w);
    TEST_ASSERT_EQUAL(58, lot52_node("d47_1")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("d47_2")->x);
    TEST_ASSERT_TRUE(lot52_node("d47_2")->y > lot52_node("d47_0")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l77_grid_span_48(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:76px;grid-template-columns:repeat(3,24px);column-gap:2px;row-gap:2px'><div id=d48_0 style='height:8px;grid-column:span 2'></div><div id=d48_1 style='height:8px'></div><div id=d48_2 style='height:8px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(50, lot52_node("d48_0")->w);
    TEST_ASSERT_EQUAL(0, lot52_node("d48_0")->x);
    TEST_ASSERT_EQUAL(24, lot52_node("d48_1")->w);
    TEST_ASSERT_EQUAL(52, lot52_node("d48_1")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("d48_2")->x);
    TEST_ASSERT_TRUE(lot52_node("d48_2")->y > lot52_node("d48_0")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l77_grid_span_49(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:78px;grid-template-columns:repeat(3,24px);column-gap:3px;row-gap:3px'><div id=d49_0 style='height:8px;grid-column:span 2'></div><div id=d49_1 style='height:8px'></div><div id=d49_2 style='height:8px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(51, lot52_node("d49_0")->w);
    TEST_ASSERT_EQUAL(0, lot52_node("d49_0")->x);
    TEST_ASSERT_EQUAL(24, lot52_node("d49_1")->w);
    TEST_ASSERT_EQUAL(54, lot52_node("d49_1")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("d49_2")->x);
    TEST_ASSERT_TRUE(lot52_node("d49_2")->y > lot52_node("d49_0")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l77_grid_span_50(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:80px;grid-template-columns:repeat(3,24px);column-gap:4px;row-gap:4px'><div id=d50_0 style='height:8px;grid-column:span 2'></div><div id=d50_1 style='height:8px'></div><div id=d50_2 style='height:8px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(52, lot52_node("d50_0")->w);
    TEST_ASSERT_EQUAL(0, lot52_node("d50_0")->x);
    TEST_ASSERT_EQUAL(24, lot52_node("d50_1")->w);
    TEST_ASSERT_EQUAL(56, lot52_node("d50_1")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("d50_2")->x);
    TEST_ASSERT_TRUE(lot52_node("d50_2")->y > lot52_node("d50_0")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l77_grid_auto_1(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:61px;grid-template-columns:repeat(2,30px);gap:1px'><div id=e1_0 style='height:8px'></div><div id=e1_1 style='height:8px'></div><div id=e1_2 style='height:8px'></div><div id=e1_3 style='height:8px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("e1_0")->x);
    TEST_ASSERT_EQUAL(31, lot52_node("e1_1")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("e1_2")->x);
    TEST_ASSERT_EQUAL(31, lot52_node("e1_3")->x);
    TEST_ASSERT_EQUAL(lot52_node("e1_0")->y, lot52_node("e1_1")->y);
    TEST_ASSERT_EQUAL(lot52_node("e1_2")->y, lot52_node("e1_3")->y);
    TEST_ASSERT_TRUE(lot52_node("e1_2")->y > lot52_node("e1_0")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l77_grid_auto_2(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:62px;grid-template-columns:repeat(2,30px);gap:2px'><div id=e2_0 style='height:8px'></div><div id=e2_1 style='height:8px'></div><div id=e2_2 style='height:8px'></div><div id=e2_3 style='height:8px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("e2_0")->x);
    TEST_ASSERT_EQUAL(32, lot52_node("e2_1")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("e2_2")->x);
    TEST_ASSERT_EQUAL(32, lot52_node("e2_3")->x);
    TEST_ASSERT_EQUAL(lot52_node("e2_0")->y, lot52_node("e2_1")->y);
    TEST_ASSERT_EQUAL(lot52_node("e2_2")->y, lot52_node("e2_3")->y);
    TEST_ASSERT_TRUE(lot52_node("e2_2")->y > lot52_node("e2_0")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l77_grid_auto_3(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:63px;grid-template-columns:repeat(2,30px);gap:3px'><div id=e3_0 style='height:8px'></div><div id=e3_1 style='height:8px'></div><div id=e3_2 style='height:8px'></div><div id=e3_3 style='height:8px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("e3_0")->x);
    TEST_ASSERT_EQUAL(33, lot52_node("e3_1")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("e3_2")->x);
    TEST_ASSERT_EQUAL(33, lot52_node("e3_3")->x);
    TEST_ASSERT_EQUAL(lot52_node("e3_0")->y, lot52_node("e3_1")->y);
    TEST_ASSERT_EQUAL(lot52_node("e3_2")->y, lot52_node("e3_3")->y);
    TEST_ASSERT_TRUE(lot52_node("e3_2")->y > lot52_node("e3_0")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l77_grid_auto_4(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:64px;grid-template-columns:repeat(2,30px);gap:4px'><div id=e4_0 style='height:8px'></div><div id=e4_1 style='height:8px'></div><div id=e4_2 style='height:8px'></div><div id=e4_3 style='height:8px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("e4_0")->x);
    TEST_ASSERT_EQUAL(34, lot52_node("e4_1")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("e4_2")->x);
    TEST_ASSERT_EQUAL(34, lot52_node("e4_3")->x);
    TEST_ASSERT_EQUAL(lot52_node("e4_0")->y, lot52_node("e4_1")->y);
    TEST_ASSERT_EQUAL(lot52_node("e4_2")->y, lot52_node("e4_3")->y);
    TEST_ASSERT_TRUE(lot52_node("e4_2")->y > lot52_node("e4_0")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l77_grid_auto_5(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:60px;grid-template-columns:repeat(2,30px);gap:0px'><div id=e5_0 style='height:8px'></div><div id=e5_1 style='height:8px'></div><div id=e5_2 style='height:8px'></div><div id=e5_3 style='height:8px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("e5_0")->x);
    TEST_ASSERT_EQUAL(30, lot52_node("e5_1")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("e5_2")->x);
    TEST_ASSERT_EQUAL(30, lot52_node("e5_3")->x);
    TEST_ASSERT_EQUAL(lot52_node("e5_0")->y, lot52_node("e5_1")->y);
    TEST_ASSERT_EQUAL(lot52_node("e5_2")->y, lot52_node("e5_3")->y);
    TEST_ASSERT_TRUE(lot52_node("e5_2")->y > lot52_node("e5_0")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l77_grid_auto_6(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:61px;grid-template-columns:repeat(2,30px);gap:1px'><div id=e6_0 style='height:8px'></div><div id=e6_1 style='height:8px'></div><div id=e6_2 style='height:8px'></div><div id=e6_3 style='height:8px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("e6_0")->x);
    TEST_ASSERT_EQUAL(31, lot52_node("e6_1")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("e6_2")->x);
    TEST_ASSERT_EQUAL(31, lot52_node("e6_3")->x);
    TEST_ASSERT_EQUAL(lot52_node("e6_0")->y, lot52_node("e6_1")->y);
    TEST_ASSERT_EQUAL(lot52_node("e6_2")->y, lot52_node("e6_3")->y);
    TEST_ASSERT_TRUE(lot52_node("e6_2")->y > lot52_node("e6_0")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l77_grid_auto_7(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:62px;grid-template-columns:repeat(2,30px);gap:2px'><div id=e7_0 style='height:8px'></div><div id=e7_1 style='height:8px'></div><div id=e7_2 style='height:8px'></div><div id=e7_3 style='height:8px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("e7_0")->x);
    TEST_ASSERT_EQUAL(32, lot52_node("e7_1")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("e7_2")->x);
    TEST_ASSERT_EQUAL(32, lot52_node("e7_3")->x);
    TEST_ASSERT_EQUAL(lot52_node("e7_0")->y, lot52_node("e7_1")->y);
    TEST_ASSERT_EQUAL(lot52_node("e7_2")->y, lot52_node("e7_3")->y);
    TEST_ASSERT_TRUE(lot52_node("e7_2")->y > lot52_node("e7_0")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l77_grid_auto_8(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:63px;grid-template-columns:repeat(2,30px);gap:3px'><div id=e8_0 style='height:8px'></div><div id=e8_1 style='height:8px'></div><div id=e8_2 style='height:8px'></div><div id=e8_3 style='height:8px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("e8_0")->x);
    TEST_ASSERT_EQUAL(33, lot52_node("e8_1")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("e8_2")->x);
    TEST_ASSERT_EQUAL(33, lot52_node("e8_3")->x);
    TEST_ASSERT_EQUAL(lot52_node("e8_0")->y, lot52_node("e8_1")->y);
    TEST_ASSERT_EQUAL(lot52_node("e8_2")->y, lot52_node("e8_3")->y);
    TEST_ASSERT_TRUE(lot52_node("e8_2")->y > lot52_node("e8_0")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l77_grid_auto_9(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:64px;grid-template-columns:repeat(2,30px);gap:4px'><div id=e9_0 style='height:8px'></div><div id=e9_1 style='height:8px'></div><div id=e9_2 style='height:8px'></div><div id=e9_3 style='height:8px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("e9_0")->x);
    TEST_ASSERT_EQUAL(34, lot52_node("e9_1")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("e9_2")->x);
    TEST_ASSERT_EQUAL(34, lot52_node("e9_3")->x);
    TEST_ASSERT_EQUAL(lot52_node("e9_0")->y, lot52_node("e9_1")->y);
    TEST_ASSERT_EQUAL(lot52_node("e9_2")->y, lot52_node("e9_3")->y);
    TEST_ASSERT_TRUE(lot52_node("e9_2")->y > lot52_node("e9_0")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l77_grid_auto_10(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:60px;grid-template-columns:repeat(2,30px);gap:0px'><div id=e10_0 style='height:8px'></div><div id=e10_1 style='height:8px'></div><div id=e10_2 style='height:8px'></div><div id=e10_3 style='height:8px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("e10_0")->x);
    TEST_ASSERT_EQUAL(30, lot52_node("e10_1")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("e10_2")->x);
    TEST_ASSERT_EQUAL(30, lot52_node("e10_3")->x);
    TEST_ASSERT_EQUAL(lot52_node("e10_0")->y, lot52_node("e10_1")->y);
    TEST_ASSERT_EQUAL(lot52_node("e10_2")->y, lot52_node("e10_3")->y);
    TEST_ASSERT_TRUE(lot52_node("e10_2")->y > lot52_node("e10_0")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l77_grid_auto_11(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:61px;grid-template-columns:repeat(2,30px);gap:1px'><div id=e11_0 style='height:8px'></div><div id=e11_1 style='height:8px'></div><div id=e11_2 style='height:8px'></div><div id=e11_3 style='height:8px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("e11_0")->x);
    TEST_ASSERT_EQUAL(31, lot52_node("e11_1")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("e11_2")->x);
    TEST_ASSERT_EQUAL(31, lot52_node("e11_3")->x);
    TEST_ASSERT_EQUAL(lot52_node("e11_0")->y, lot52_node("e11_1")->y);
    TEST_ASSERT_EQUAL(lot52_node("e11_2")->y, lot52_node("e11_3")->y);
    TEST_ASSERT_TRUE(lot52_node("e11_2")->y > lot52_node("e11_0")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l77_grid_auto_12(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:62px;grid-template-columns:repeat(2,30px);gap:2px'><div id=e12_0 style='height:8px'></div><div id=e12_1 style='height:8px'></div><div id=e12_2 style='height:8px'></div><div id=e12_3 style='height:8px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("e12_0")->x);
    TEST_ASSERT_EQUAL(32, lot52_node("e12_1")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("e12_2")->x);
    TEST_ASSERT_EQUAL(32, lot52_node("e12_3")->x);
    TEST_ASSERT_EQUAL(lot52_node("e12_0")->y, lot52_node("e12_1")->y);
    TEST_ASSERT_EQUAL(lot52_node("e12_2")->y, lot52_node("e12_3")->y);
    TEST_ASSERT_TRUE(lot52_node("e12_2")->y > lot52_node("e12_0")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l77_grid_auto_13(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:63px;grid-template-columns:repeat(2,30px);gap:3px'><div id=e13_0 style='height:8px'></div><div id=e13_1 style='height:8px'></div><div id=e13_2 style='height:8px'></div><div id=e13_3 style='height:8px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("e13_0")->x);
    TEST_ASSERT_EQUAL(33, lot52_node("e13_1")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("e13_2")->x);
    TEST_ASSERT_EQUAL(33, lot52_node("e13_3")->x);
    TEST_ASSERT_EQUAL(lot52_node("e13_0")->y, lot52_node("e13_1")->y);
    TEST_ASSERT_EQUAL(lot52_node("e13_2")->y, lot52_node("e13_3")->y);
    TEST_ASSERT_TRUE(lot52_node("e13_2")->y > lot52_node("e13_0")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l77_grid_auto_14(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:64px;grid-template-columns:repeat(2,30px);gap:4px'><div id=e14_0 style='height:8px'></div><div id=e14_1 style='height:8px'></div><div id=e14_2 style='height:8px'></div><div id=e14_3 style='height:8px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("e14_0")->x);
    TEST_ASSERT_EQUAL(34, lot52_node("e14_1")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("e14_2")->x);
    TEST_ASSERT_EQUAL(34, lot52_node("e14_3")->x);
    TEST_ASSERT_EQUAL(lot52_node("e14_0")->y, lot52_node("e14_1")->y);
    TEST_ASSERT_EQUAL(lot52_node("e14_2")->y, lot52_node("e14_3")->y);
    TEST_ASSERT_TRUE(lot52_node("e14_2")->y > lot52_node("e14_0")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l77_grid_auto_15(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:60px;grid-template-columns:repeat(2,30px);gap:0px'><div id=e15_0 style='height:8px'></div><div id=e15_1 style='height:8px'></div><div id=e15_2 style='height:8px'></div><div id=e15_3 style='height:8px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("e15_0")->x);
    TEST_ASSERT_EQUAL(30, lot52_node("e15_1")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("e15_2")->x);
    TEST_ASSERT_EQUAL(30, lot52_node("e15_3")->x);
    TEST_ASSERT_EQUAL(lot52_node("e15_0")->y, lot52_node("e15_1")->y);
    TEST_ASSERT_EQUAL(lot52_node("e15_2")->y, lot52_node("e15_3")->y);
    TEST_ASSERT_TRUE(lot52_node("e15_2")->y > lot52_node("e15_0")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l77_grid_auto_16(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:61px;grid-template-columns:repeat(2,30px);gap:1px'><div id=e16_0 style='height:8px'></div><div id=e16_1 style='height:8px'></div><div id=e16_2 style='height:8px'></div><div id=e16_3 style='height:8px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("e16_0")->x);
    TEST_ASSERT_EQUAL(31, lot52_node("e16_1")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("e16_2")->x);
    TEST_ASSERT_EQUAL(31, lot52_node("e16_3")->x);
    TEST_ASSERT_EQUAL(lot52_node("e16_0")->y, lot52_node("e16_1")->y);
    TEST_ASSERT_EQUAL(lot52_node("e16_2")->y, lot52_node("e16_3")->y);
    TEST_ASSERT_TRUE(lot52_node("e16_2")->y > lot52_node("e16_0")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l77_grid_auto_17(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:62px;grid-template-columns:repeat(2,30px);gap:2px'><div id=e17_0 style='height:8px'></div><div id=e17_1 style='height:8px'></div><div id=e17_2 style='height:8px'></div><div id=e17_3 style='height:8px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("e17_0")->x);
    TEST_ASSERT_EQUAL(32, lot52_node("e17_1")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("e17_2")->x);
    TEST_ASSERT_EQUAL(32, lot52_node("e17_3")->x);
    TEST_ASSERT_EQUAL(lot52_node("e17_0")->y, lot52_node("e17_1")->y);
    TEST_ASSERT_EQUAL(lot52_node("e17_2")->y, lot52_node("e17_3")->y);
    TEST_ASSERT_TRUE(lot52_node("e17_2")->y > lot52_node("e17_0")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l77_grid_auto_18(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:63px;grid-template-columns:repeat(2,30px);gap:3px'><div id=e18_0 style='height:8px'></div><div id=e18_1 style='height:8px'></div><div id=e18_2 style='height:8px'></div><div id=e18_3 style='height:8px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("e18_0")->x);
    TEST_ASSERT_EQUAL(33, lot52_node("e18_1")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("e18_2")->x);
    TEST_ASSERT_EQUAL(33, lot52_node("e18_3")->x);
    TEST_ASSERT_EQUAL(lot52_node("e18_0")->y, lot52_node("e18_1")->y);
    TEST_ASSERT_EQUAL(lot52_node("e18_2")->y, lot52_node("e18_3")->y);
    TEST_ASSERT_TRUE(lot52_node("e18_2")->y > lot52_node("e18_0")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l77_grid_auto_19(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:64px;grid-template-columns:repeat(2,30px);gap:4px'><div id=e19_0 style='height:8px'></div><div id=e19_1 style='height:8px'></div><div id=e19_2 style='height:8px'></div><div id=e19_3 style='height:8px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("e19_0")->x);
    TEST_ASSERT_EQUAL(34, lot52_node("e19_1")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("e19_2")->x);
    TEST_ASSERT_EQUAL(34, lot52_node("e19_3")->x);
    TEST_ASSERT_EQUAL(lot52_node("e19_0")->y, lot52_node("e19_1")->y);
    TEST_ASSERT_EQUAL(lot52_node("e19_2")->y, lot52_node("e19_3")->y);
    TEST_ASSERT_TRUE(lot52_node("e19_2")->y > lot52_node("e19_0")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l77_grid_auto_20(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:60px;grid-template-columns:repeat(2,30px);gap:0px'><div id=e20_0 style='height:8px'></div><div id=e20_1 style='height:8px'></div><div id=e20_2 style='height:8px'></div><div id=e20_3 style='height:8px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("e20_0")->x);
    TEST_ASSERT_EQUAL(30, lot52_node("e20_1")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("e20_2")->x);
    TEST_ASSERT_EQUAL(30, lot52_node("e20_3")->x);
    TEST_ASSERT_EQUAL(lot52_node("e20_0")->y, lot52_node("e20_1")->y);
    TEST_ASSERT_EQUAL(lot52_node("e20_2")->y, lot52_node("e20_3")->y);
    TEST_ASSERT_TRUE(lot52_node("e20_2")->y > lot52_node("e20_0")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l77_grid_auto_21(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:61px;grid-template-columns:repeat(2,30px);gap:1px'><div id=e21_0 style='height:8px'></div><div id=e21_1 style='height:8px'></div><div id=e21_2 style='height:8px'></div><div id=e21_3 style='height:8px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("e21_0")->x);
    TEST_ASSERT_EQUAL(31, lot52_node("e21_1")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("e21_2")->x);
    TEST_ASSERT_EQUAL(31, lot52_node("e21_3")->x);
    TEST_ASSERT_EQUAL(lot52_node("e21_0")->y, lot52_node("e21_1")->y);
    TEST_ASSERT_EQUAL(lot52_node("e21_2")->y, lot52_node("e21_3")->y);
    TEST_ASSERT_TRUE(lot52_node("e21_2")->y > lot52_node("e21_0")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l77_grid_auto_22(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:62px;grid-template-columns:repeat(2,30px);gap:2px'><div id=e22_0 style='height:8px'></div><div id=e22_1 style='height:8px'></div><div id=e22_2 style='height:8px'></div><div id=e22_3 style='height:8px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("e22_0")->x);
    TEST_ASSERT_EQUAL(32, lot52_node("e22_1")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("e22_2")->x);
    TEST_ASSERT_EQUAL(32, lot52_node("e22_3")->x);
    TEST_ASSERT_EQUAL(lot52_node("e22_0")->y, lot52_node("e22_1")->y);
    TEST_ASSERT_EQUAL(lot52_node("e22_2")->y, lot52_node("e22_3")->y);
    TEST_ASSERT_TRUE(lot52_node("e22_2")->y > lot52_node("e22_0")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l77_grid_auto_23(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:63px;grid-template-columns:repeat(2,30px);gap:3px'><div id=e23_0 style='height:8px'></div><div id=e23_1 style='height:8px'></div><div id=e23_2 style='height:8px'></div><div id=e23_3 style='height:8px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("e23_0")->x);
    TEST_ASSERT_EQUAL(33, lot52_node("e23_1")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("e23_2")->x);
    TEST_ASSERT_EQUAL(33, lot52_node("e23_3")->x);
    TEST_ASSERT_EQUAL(lot52_node("e23_0")->y, lot52_node("e23_1")->y);
    TEST_ASSERT_EQUAL(lot52_node("e23_2")->y, lot52_node("e23_3")->y);
    TEST_ASSERT_TRUE(lot52_node("e23_2")->y > lot52_node("e23_0")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l77_grid_auto_24(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:64px;grid-template-columns:repeat(2,30px);gap:4px'><div id=e24_0 style='height:8px'></div><div id=e24_1 style='height:8px'></div><div id=e24_2 style='height:8px'></div><div id=e24_3 style='height:8px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("e24_0")->x);
    TEST_ASSERT_EQUAL(34, lot52_node("e24_1")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("e24_2")->x);
    TEST_ASSERT_EQUAL(34, lot52_node("e24_3")->x);
    TEST_ASSERT_EQUAL(lot52_node("e24_0")->y, lot52_node("e24_1")->y);
    TEST_ASSERT_EQUAL(lot52_node("e24_2")->y, lot52_node("e24_3")->y);
    TEST_ASSERT_TRUE(lot52_node("e24_2")->y > lot52_node("e24_0")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l77_grid_auto_25(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:60px;grid-template-columns:repeat(2,30px);gap:0px'><div id=e25_0 style='height:8px'></div><div id=e25_1 style='height:8px'></div><div id=e25_2 style='height:8px'></div><div id=e25_3 style='height:8px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("e25_0")->x);
    TEST_ASSERT_EQUAL(30, lot52_node("e25_1")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("e25_2")->x);
    TEST_ASSERT_EQUAL(30, lot52_node("e25_3")->x);
    TEST_ASSERT_EQUAL(lot52_node("e25_0")->y, lot52_node("e25_1")->y);
    TEST_ASSERT_EQUAL(lot52_node("e25_2")->y, lot52_node("e25_3")->y);
    TEST_ASSERT_TRUE(lot52_node("e25_2")->y > lot52_node("e25_0")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l77_grid_auto_26(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:61px;grid-template-columns:repeat(2,30px);gap:1px'><div id=e26_0 style='height:8px'></div><div id=e26_1 style='height:8px'></div><div id=e26_2 style='height:8px'></div><div id=e26_3 style='height:8px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("e26_0")->x);
    TEST_ASSERT_EQUAL(31, lot52_node("e26_1")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("e26_2")->x);
    TEST_ASSERT_EQUAL(31, lot52_node("e26_3")->x);
    TEST_ASSERT_EQUAL(lot52_node("e26_0")->y, lot52_node("e26_1")->y);
    TEST_ASSERT_EQUAL(lot52_node("e26_2")->y, lot52_node("e26_3")->y);
    TEST_ASSERT_TRUE(lot52_node("e26_2")->y > lot52_node("e26_0")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l77_grid_auto_27(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:62px;grid-template-columns:repeat(2,30px);gap:2px'><div id=e27_0 style='height:8px'></div><div id=e27_1 style='height:8px'></div><div id=e27_2 style='height:8px'></div><div id=e27_3 style='height:8px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("e27_0")->x);
    TEST_ASSERT_EQUAL(32, lot52_node("e27_1")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("e27_2")->x);
    TEST_ASSERT_EQUAL(32, lot52_node("e27_3")->x);
    TEST_ASSERT_EQUAL(lot52_node("e27_0")->y, lot52_node("e27_1")->y);
    TEST_ASSERT_EQUAL(lot52_node("e27_2")->y, lot52_node("e27_3")->y);
    TEST_ASSERT_TRUE(lot52_node("e27_2")->y > lot52_node("e27_0")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l77_grid_auto_28(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:63px;grid-template-columns:repeat(2,30px);gap:3px'><div id=e28_0 style='height:8px'></div><div id=e28_1 style='height:8px'></div><div id=e28_2 style='height:8px'></div><div id=e28_3 style='height:8px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("e28_0")->x);
    TEST_ASSERT_EQUAL(33, lot52_node("e28_1")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("e28_2")->x);
    TEST_ASSERT_EQUAL(33, lot52_node("e28_3")->x);
    TEST_ASSERT_EQUAL(lot52_node("e28_0")->y, lot52_node("e28_1")->y);
    TEST_ASSERT_EQUAL(lot52_node("e28_2")->y, lot52_node("e28_3")->y);
    TEST_ASSERT_TRUE(lot52_node("e28_2")->y > lot52_node("e28_0")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l77_grid_auto_29(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:64px;grid-template-columns:repeat(2,30px);gap:4px'><div id=e29_0 style='height:8px'></div><div id=e29_1 style='height:8px'></div><div id=e29_2 style='height:8px'></div><div id=e29_3 style='height:8px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("e29_0")->x);
    TEST_ASSERT_EQUAL(34, lot52_node("e29_1")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("e29_2")->x);
    TEST_ASSERT_EQUAL(34, lot52_node("e29_3")->x);
    TEST_ASSERT_EQUAL(lot52_node("e29_0")->y, lot52_node("e29_1")->y);
    TEST_ASSERT_EQUAL(lot52_node("e29_2")->y, lot52_node("e29_3")->y);
    TEST_ASSERT_TRUE(lot52_node("e29_2")->y > lot52_node("e29_0")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l77_grid_auto_30(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:60px;grid-template-columns:repeat(2,30px);gap:0px'><div id=e30_0 style='height:8px'></div><div id=e30_1 style='height:8px'></div><div id=e30_2 style='height:8px'></div><div id=e30_3 style='height:8px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("e30_0")->x);
    TEST_ASSERT_EQUAL(30, lot52_node("e30_1")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("e30_2")->x);
    TEST_ASSERT_EQUAL(30, lot52_node("e30_3")->x);
    TEST_ASSERT_EQUAL(lot52_node("e30_0")->y, lot52_node("e30_1")->y);
    TEST_ASSERT_EQUAL(lot52_node("e30_2")->y, lot52_node("e30_3")->y);
    TEST_ASSERT_TRUE(lot52_node("e30_2")->y > lot52_node("e30_0")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l77_grid_auto_31(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:61px;grid-template-columns:repeat(2,30px);gap:1px'><div id=e31_0 style='height:8px'></div><div id=e31_1 style='height:8px'></div><div id=e31_2 style='height:8px'></div><div id=e31_3 style='height:8px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("e31_0")->x);
    TEST_ASSERT_EQUAL(31, lot52_node("e31_1")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("e31_2")->x);
    TEST_ASSERT_EQUAL(31, lot52_node("e31_3")->x);
    TEST_ASSERT_EQUAL(lot52_node("e31_0")->y, lot52_node("e31_1")->y);
    TEST_ASSERT_EQUAL(lot52_node("e31_2")->y, lot52_node("e31_3")->y);
    TEST_ASSERT_TRUE(lot52_node("e31_2")->y > lot52_node("e31_0")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l77_grid_auto_32(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:62px;grid-template-columns:repeat(2,30px);gap:2px'><div id=e32_0 style='height:8px'></div><div id=e32_1 style='height:8px'></div><div id=e32_2 style='height:8px'></div><div id=e32_3 style='height:8px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("e32_0")->x);
    TEST_ASSERT_EQUAL(32, lot52_node("e32_1")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("e32_2")->x);
    TEST_ASSERT_EQUAL(32, lot52_node("e32_3")->x);
    TEST_ASSERT_EQUAL(lot52_node("e32_0")->y, lot52_node("e32_1")->y);
    TEST_ASSERT_EQUAL(lot52_node("e32_2")->y, lot52_node("e32_3")->y);
    TEST_ASSERT_TRUE(lot52_node("e32_2")->y > lot52_node("e32_0")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l77_grid_auto_33(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:63px;grid-template-columns:repeat(2,30px);gap:3px'><div id=e33_0 style='height:8px'></div><div id=e33_1 style='height:8px'></div><div id=e33_2 style='height:8px'></div><div id=e33_3 style='height:8px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("e33_0")->x);
    TEST_ASSERT_EQUAL(33, lot52_node("e33_1")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("e33_2")->x);
    TEST_ASSERT_EQUAL(33, lot52_node("e33_3")->x);
    TEST_ASSERT_EQUAL(lot52_node("e33_0")->y, lot52_node("e33_1")->y);
    TEST_ASSERT_EQUAL(lot52_node("e33_2")->y, lot52_node("e33_3")->y);
    TEST_ASSERT_TRUE(lot52_node("e33_2")->y > lot52_node("e33_0")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l77_grid_auto_34(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:64px;grid-template-columns:repeat(2,30px);gap:4px'><div id=e34_0 style='height:8px'></div><div id=e34_1 style='height:8px'></div><div id=e34_2 style='height:8px'></div><div id=e34_3 style='height:8px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("e34_0")->x);
    TEST_ASSERT_EQUAL(34, lot52_node("e34_1")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("e34_2")->x);
    TEST_ASSERT_EQUAL(34, lot52_node("e34_3")->x);
    TEST_ASSERT_EQUAL(lot52_node("e34_0")->y, lot52_node("e34_1")->y);
    TEST_ASSERT_EQUAL(lot52_node("e34_2")->y, lot52_node("e34_3")->y);
    TEST_ASSERT_TRUE(lot52_node("e34_2")->y > lot52_node("e34_0")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l77_grid_auto_35(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:60px;grid-template-columns:repeat(2,30px);gap:0px'><div id=e35_0 style='height:8px'></div><div id=e35_1 style='height:8px'></div><div id=e35_2 style='height:8px'></div><div id=e35_3 style='height:8px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("e35_0")->x);
    TEST_ASSERT_EQUAL(30, lot52_node("e35_1")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("e35_2")->x);
    TEST_ASSERT_EQUAL(30, lot52_node("e35_3")->x);
    TEST_ASSERT_EQUAL(lot52_node("e35_0")->y, lot52_node("e35_1")->y);
    TEST_ASSERT_EQUAL(lot52_node("e35_2")->y, lot52_node("e35_3")->y);
    TEST_ASSERT_TRUE(lot52_node("e35_2")->y > lot52_node("e35_0")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l77_grid_auto_36(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:61px;grid-template-columns:repeat(2,30px);gap:1px'><div id=e36_0 style='height:8px'></div><div id=e36_1 style='height:8px'></div><div id=e36_2 style='height:8px'></div><div id=e36_3 style='height:8px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("e36_0")->x);
    TEST_ASSERT_EQUAL(31, lot52_node("e36_1")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("e36_2")->x);
    TEST_ASSERT_EQUAL(31, lot52_node("e36_3")->x);
    TEST_ASSERT_EQUAL(lot52_node("e36_0")->y, lot52_node("e36_1")->y);
    TEST_ASSERT_EQUAL(lot52_node("e36_2")->y, lot52_node("e36_3")->y);
    TEST_ASSERT_TRUE(lot52_node("e36_2")->y > lot52_node("e36_0")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l77_grid_auto_37(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:62px;grid-template-columns:repeat(2,30px);gap:2px'><div id=e37_0 style='height:8px'></div><div id=e37_1 style='height:8px'></div><div id=e37_2 style='height:8px'></div><div id=e37_3 style='height:8px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("e37_0")->x);
    TEST_ASSERT_EQUAL(32, lot52_node("e37_1")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("e37_2")->x);
    TEST_ASSERT_EQUAL(32, lot52_node("e37_3")->x);
    TEST_ASSERT_EQUAL(lot52_node("e37_0")->y, lot52_node("e37_1")->y);
    TEST_ASSERT_EQUAL(lot52_node("e37_2")->y, lot52_node("e37_3")->y);
    TEST_ASSERT_TRUE(lot52_node("e37_2")->y > lot52_node("e37_0")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l77_grid_auto_38(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:63px;grid-template-columns:repeat(2,30px);gap:3px'><div id=e38_0 style='height:8px'></div><div id=e38_1 style='height:8px'></div><div id=e38_2 style='height:8px'></div><div id=e38_3 style='height:8px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("e38_0")->x);
    TEST_ASSERT_EQUAL(33, lot52_node("e38_1")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("e38_2")->x);
    TEST_ASSERT_EQUAL(33, lot52_node("e38_3")->x);
    TEST_ASSERT_EQUAL(lot52_node("e38_0")->y, lot52_node("e38_1")->y);
    TEST_ASSERT_EQUAL(lot52_node("e38_2")->y, lot52_node("e38_3")->y);
    TEST_ASSERT_TRUE(lot52_node("e38_2")->y > lot52_node("e38_0")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l77_grid_auto_39(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:64px;grid-template-columns:repeat(2,30px);gap:4px'><div id=e39_0 style='height:8px'></div><div id=e39_1 style='height:8px'></div><div id=e39_2 style='height:8px'></div><div id=e39_3 style='height:8px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("e39_0")->x);
    TEST_ASSERT_EQUAL(34, lot52_node("e39_1")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("e39_2")->x);
    TEST_ASSERT_EQUAL(34, lot52_node("e39_3")->x);
    TEST_ASSERT_EQUAL(lot52_node("e39_0")->y, lot52_node("e39_1")->y);
    TEST_ASSERT_EQUAL(lot52_node("e39_2")->y, lot52_node("e39_3")->y);
    TEST_ASSERT_TRUE(lot52_node("e39_2")->y > lot52_node("e39_0")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l77_grid_auto_40(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:60px;grid-template-columns:repeat(2,30px);gap:0px'><div id=e40_0 style='height:8px'></div><div id=e40_1 style='height:8px'></div><div id=e40_2 style='height:8px'></div><div id=e40_3 style='height:8px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("e40_0")->x);
    TEST_ASSERT_EQUAL(30, lot52_node("e40_1")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("e40_2")->x);
    TEST_ASSERT_EQUAL(30, lot52_node("e40_3")->x);
    TEST_ASSERT_EQUAL(lot52_node("e40_0")->y, lot52_node("e40_1")->y);
    TEST_ASSERT_EQUAL(lot52_node("e40_2")->y, lot52_node("e40_3")->y);
    TEST_ASSERT_TRUE(lot52_node("e40_2")->y > lot52_node("e40_0")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void lot77_grid_run(void) {
    RUN_TEST(test_l77_grid_px_1);
    RUN_TEST(test_l77_grid_px_2);
    RUN_TEST(test_l77_grid_px_3);
    RUN_TEST(test_l77_grid_px_4);
    RUN_TEST(test_l77_grid_px_5);
    RUN_TEST(test_l77_grid_px_6);
    RUN_TEST(test_l77_grid_px_7);
    RUN_TEST(test_l77_grid_px_8);
    RUN_TEST(test_l77_grid_px_9);
    RUN_TEST(test_l77_grid_px_10);
    RUN_TEST(test_l77_grid_px_11);
    RUN_TEST(test_l77_grid_px_12);
    RUN_TEST(test_l77_grid_px_13);
    RUN_TEST(test_l77_grid_px_14);
    RUN_TEST(test_l77_grid_px_15);
    RUN_TEST(test_l77_grid_px_16);
    RUN_TEST(test_l77_grid_px_17);
    RUN_TEST(test_l77_grid_px_18);
    RUN_TEST(test_l77_grid_px_19);
    RUN_TEST(test_l77_grid_px_20);
    RUN_TEST(test_l77_grid_px_21);
    RUN_TEST(test_l77_grid_px_22);
    RUN_TEST(test_l77_grid_px_23);
    RUN_TEST(test_l77_grid_px_24);
    RUN_TEST(test_l77_grid_px_25);
    RUN_TEST(test_l77_grid_px_26);
    RUN_TEST(test_l77_grid_px_27);
    RUN_TEST(test_l77_grid_px_28);
    RUN_TEST(test_l77_grid_px_29);
    RUN_TEST(test_l77_grid_px_30);
    RUN_TEST(test_l77_grid_px_31);
    RUN_TEST(test_l77_grid_px_32);
    RUN_TEST(test_l77_grid_px_33);
    RUN_TEST(test_l77_grid_px_34);
    RUN_TEST(test_l77_grid_px_35);
    RUN_TEST(test_l77_grid_px_36);
    RUN_TEST(test_l77_grid_px_37);
    RUN_TEST(test_l77_grid_px_38);
    RUN_TEST(test_l77_grid_px_39);
    RUN_TEST(test_l77_grid_px_40);
    RUN_TEST(test_l77_grid_px_41);
    RUN_TEST(test_l77_grid_px_42);
    RUN_TEST(test_l77_grid_px_43);
    RUN_TEST(test_l77_grid_px_44);
    RUN_TEST(test_l77_grid_px_45);
    RUN_TEST(test_l77_grid_px_46);
    RUN_TEST(test_l77_grid_px_47);
    RUN_TEST(test_l77_grid_px_48);
    RUN_TEST(test_l77_grid_px_49);
    RUN_TEST(test_l77_grid_px_50);
    RUN_TEST(test_l77_grid_px_51);
    RUN_TEST(test_l77_grid_px_52);
    RUN_TEST(test_l77_grid_px_53);
    RUN_TEST(test_l77_grid_px_54);
    RUN_TEST(test_l77_grid_px_55);
    RUN_TEST(test_l77_grid_px_56);
    RUN_TEST(test_l77_grid_px_57);
    RUN_TEST(test_l77_grid_px_58);
    RUN_TEST(test_l77_grid_px_59);
    RUN_TEST(test_l77_grid_px_60);
    RUN_TEST(test_l77_grid_px_61);
    RUN_TEST(test_l77_grid_px_62);
    RUN_TEST(test_l77_grid_px_63);
    RUN_TEST(test_l77_grid_px_64);
    RUN_TEST(test_l77_grid_px_65);
    RUN_TEST(test_l77_grid_px_66);
    RUN_TEST(test_l77_grid_px_67);
    RUN_TEST(test_l77_grid_px_68);
    RUN_TEST(test_l77_grid_px_69);
    RUN_TEST(test_l77_grid_px_70);
    RUN_TEST(test_l77_grid_px_71);
    RUN_TEST(test_l77_grid_px_72);
    RUN_TEST(test_l77_grid_px_73);
    RUN_TEST(test_l77_grid_px_74);
    RUN_TEST(test_l77_grid_px_75);
    RUN_TEST(test_l77_grid_px_76);
    RUN_TEST(test_l77_grid_px_77);
    RUN_TEST(test_l77_grid_px_78);
    RUN_TEST(test_l77_grid_px_79);
    RUN_TEST(test_l77_grid_px_80);
    RUN_TEST(test_l77_grid_fr_1);
    RUN_TEST(test_l77_grid_fr_2);
    RUN_TEST(test_l77_grid_fr_3);
    RUN_TEST(test_l77_grid_fr_4);
    RUN_TEST(test_l77_grid_fr_5);
    RUN_TEST(test_l77_grid_fr_6);
    RUN_TEST(test_l77_grid_fr_7);
    RUN_TEST(test_l77_grid_fr_8);
    RUN_TEST(test_l77_grid_fr_9);
    RUN_TEST(test_l77_grid_fr_10);
    RUN_TEST(test_l77_grid_fr_11);
    RUN_TEST(test_l77_grid_fr_12);
    RUN_TEST(test_l77_grid_fr_13);
    RUN_TEST(test_l77_grid_fr_14);
    RUN_TEST(test_l77_grid_fr_15);
    RUN_TEST(test_l77_grid_fr_16);
    RUN_TEST(test_l77_grid_fr_17);
    RUN_TEST(test_l77_grid_fr_18);
    RUN_TEST(test_l77_grid_fr_19);
    RUN_TEST(test_l77_grid_fr_20);
    RUN_TEST(test_l77_grid_fr_21);
    RUN_TEST(test_l77_grid_fr_22);
    RUN_TEST(test_l77_grid_fr_23);
    RUN_TEST(test_l77_grid_fr_24);
    RUN_TEST(test_l77_grid_fr_25);
    RUN_TEST(test_l77_grid_fr_26);
    RUN_TEST(test_l77_grid_fr_27);
    RUN_TEST(test_l77_grid_fr_28);
    RUN_TEST(test_l77_grid_fr_29);
    RUN_TEST(test_l77_grid_fr_30);
    RUN_TEST(test_l77_grid_fr_31);
    RUN_TEST(test_l77_grid_fr_32);
    RUN_TEST(test_l77_grid_fr_33);
    RUN_TEST(test_l77_grid_fr_34);
    RUN_TEST(test_l77_grid_fr_35);
    RUN_TEST(test_l77_grid_fr_36);
    RUN_TEST(test_l77_grid_fr_37);
    RUN_TEST(test_l77_grid_fr_38);
    RUN_TEST(test_l77_grid_fr_39);
    RUN_TEST(test_l77_grid_fr_40);
    RUN_TEST(test_l77_grid_fr_41);
    RUN_TEST(test_l77_grid_fr_42);
    RUN_TEST(test_l77_grid_fr_43);
    RUN_TEST(test_l77_grid_fr_44);
    RUN_TEST(test_l77_grid_fr_45);
    RUN_TEST(test_l77_grid_fr_46);
    RUN_TEST(test_l77_grid_fr_47);
    RUN_TEST(test_l77_grid_fr_48);
    RUN_TEST(test_l77_grid_fr_49);
    RUN_TEST(test_l77_grid_fr_50);
    RUN_TEST(test_l77_grid_fr_51);
    RUN_TEST(test_l77_grid_fr_52);
    RUN_TEST(test_l77_grid_fr_53);
    RUN_TEST(test_l77_grid_fr_54);
    RUN_TEST(test_l77_grid_fr_55);
    RUN_TEST(test_l77_grid_fr_56);
    RUN_TEST(test_l77_grid_fr_57);
    RUN_TEST(test_l77_grid_fr_58);
    RUN_TEST(test_l77_grid_fr_59);
    RUN_TEST(test_l77_grid_fr_60);
    RUN_TEST(test_l77_grid_fr_61);
    RUN_TEST(test_l77_grid_fr_62);
    RUN_TEST(test_l77_grid_fr_63);
    RUN_TEST(test_l77_grid_fr_64);
    RUN_TEST(test_l77_grid_fr_65);
    RUN_TEST(test_l77_grid_fr_66);
    RUN_TEST(test_l77_grid_fr_67);
    RUN_TEST(test_l77_grid_fr_68);
    RUN_TEST(test_l77_grid_fr_69);
    RUN_TEST(test_l77_grid_fr_70);
    RUN_TEST(test_l77_grid_fr_71);
    RUN_TEST(test_l77_grid_fr_72);
    RUN_TEST(test_l77_grid_fr_73);
    RUN_TEST(test_l77_grid_fr_74);
    RUN_TEST(test_l77_grid_fr_75);
    RUN_TEST(test_l77_grid_fr_76);
    RUN_TEST(test_l77_grid_fr_77);
    RUN_TEST(test_l77_grid_fr_78);
    RUN_TEST(test_l77_grid_fr_79);
    RUN_TEST(test_l77_grid_fr_80);
    RUN_TEST(test_l77_grid_mix_1);
    RUN_TEST(test_l77_grid_mix_2);
    RUN_TEST(test_l77_grid_mix_3);
    RUN_TEST(test_l77_grid_mix_4);
    RUN_TEST(test_l77_grid_mix_5);
    RUN_TEST(test_l77_grid_mix_6);
    RUN_TEST(test_l77_grid_mix_7);
    RUN_TEST(test_l77_grid_mix_8);
    RUN_TEST(test_l77_grid_mix_9);
    RUN_TEST(test_l77_grid_mix_10);
    RUN_TEST(test_l77_grid_mix_11);
    RUN_TEST(test_l77_grid_mix_12);
    RUN_TEST(test_l77_grid_mix_13);
    RUN_TEST(test_l77_grid_mix_14);
    RUN_TEST(test_l77_grid_mix_15);
    RUN_TEST(test_l77_grid_mix_16);
    RUN_TEST(test_l77_grid_mix_17);
    RUN_TEST(test_l77_grid_mix_18);
    RUN_TEST(test_l77_grid_mix_19);
    RUN_TEST(test_l77_grid_mix_20);
    RUN_TEST(test_l77_grid_mix_21);
    RUN_TEST(test_l77_grid_mix_22);
    RUN_TEST(test_l77_grid_mix_23);
    RUN_TEST(test_l77_grid_mix_24);
    RUN_TEST(test_l77_grid_mix_25);
    RUN_TEST(test_l77_grid_mix_26);
    RUN_TEST(test_l77_grid_mix_27);
    RUN_TEST(test_l77_grid_mix_28);
    RUN_TEST(test_l77_grid_mix_29);
    RUN_TEST(test_l77_grid_mix_30);
    RUN_TEST(test_l77_grid_mix_31);
    RUN_TEST(test_l77_grid_mix_32);
    RUN_TEST(test_l77_grid_mix_33);
    RUN_TEST(test_l77_grid_mix_34);
    RUN_TEST(test_l77_grid_mix_35);
    RUN_TEST(test_l77_grid_mix_36);
    RUN_TEST(test_l77_grid_mix_37);
    RUN_TEST(test_l77_grid_mix_38);
    RUN_TEST(test_l77_grid_mix_39);
    RUN_TEST(test_l77_grid_mix_40);
    RUN_TEST(test_l77_grid_mix_41);
    RUN_TEST(test_l77_grid_mix_42);
    RUN_TEST(test_l77_grid_mix_43);
    RUN_TEST(test_l77_grid_mix_44);
    RUN_TEST(test_l77_grid_mix_45);
    RUN_TEST(test_l77_grid_mix_46);
    RUN_TEST(test_l77_grid_mix_47);
    RUN_TEST(test_l77_grid_mix_48);
    RUN_TEST(test_l77_grid_mix_49);
    RUN_TEST(test_l77_grid_mix_50);
    RUN_TEST(test_l77_grid_mix_51);
    RUN_TEST(test_l77_grid_mix_52);
    RUN_TEST(test_l77_grid_mix_53);
    RUN_TEST(test_l77_grid_mix_54);
    RUN_TEST(test_l77_grid_mix_55);
    RUN_TEST(test_l77_grid_mix_56);
    RUN_TEST(test_l77_grid_mix_57);
    RUN_TEST(test_l77_grid_mix_58);
    RUN_TEST(test_l77_grid_mix_59);
    RUN_TEST(test_l77_grid_mix_60);
    RUN_TEST(test_l77_grid_span_1);
    RUN_TEST(test_l77_grid_span_2);
    RUN_TEST(test_l77_grid_span_3);
    RUN_TEST(test_l77_grid_span_4);
    RUN_TEST(test_l77_grid_span_5);
    RUN_TEST(test_l77_grid_span_6);
    RUN_TEST(test_l77_grid_span_7);
    RUN_TEST(test_l77_grid_span_8);
    RUN_TEST(test_l77_grid_span_9);
    RUN_TEST(test_l77_grid_span_10);
    RUN_TEST(test_l77_grid_span_11);
    RUN_TEST(test_l77_grid_span_12);
    RUN_TEST(test_l77_grid_span_13);
    RUN_TEST(test_l77_grid_span_14);
    RUN_TEST(test_l77_grid_span_15);
    RUN_TEST(test_l77_grid_span_16);
    RUN_TEST(test_l77_grid_span_17);
    RUN_TEST(test_l77_grid_span_18);
    RUN_TEST(test_l77_grid_span_19);
    RUN_TEST(test_l77_grid_span_20);
    RUN_TEST(test_l77_grid_span_21);
    RUN_TEST(test_l77_grid_span_22);
    RUN_TEST(test_l77_grid_span_23);
    RUN_TEST(test_l77_grid_span_24);
    RUN_TEST(test_l77_grid_span_25);
    RUN_TEST(test_l77_grid_span_26);
    RUN_TEST(test_l77_grid_span_27);
    RUN_TEST(test_l77_grid_span_28);
    RUN_TEST(test_l77_grid_span_29);
    RUN_TEST(test_l77_grid_span_30);
    RUN_TEST(test_l77_grid_span_31);
    RUN_TEST(test_l77_grid_span_32);
    RUN_TEST(test_l77_grid_span_33);
    RUN_TEST(test_l77_grid_span_34);
    RUN_TEST(test_l77_grid_span_35);
    RUN_TEST(test_l77_grid_span_36);
    RUN_TEST(test_l77_grid_span_37);
    RUN_TEST(test_l77_grid_span_38);
    RUN_TEST(test_l77_grid_span_39);
    RUN_TEST(test_l77_grid_span_40);
    RUN_TEST(test_l77_grid_span_41);
    RUN_TEST(test_l77_grid_span_42);
    RUN_TEST(test_l77_grid_span_43);
    RUN_TEST(test_l77_grid_span_44);
    RUN_TEST(test_l77_grid_span_45);
    RUN_TEST(test_l77_grid_span_46);
    RUN_TEST(test_l77_grid_span_47);
    RUN_TEST(test_l77_grid_span_48);
    RUN_TEST(test_l77_grid_span_49);
    RUN_TEST(test_l77_grid_span_50);
    RUN_TEST(test_l77_grid_auto_1);
    RUN_TEST(test_l77_grid_auto_2);
    RUN_TEST(test_l77_grid_auto_3);
    RUN_TEST(test_l77_grid_auto_4);
    RUN_TEST(test_l77_grid_auto_5);
    RUN_TEST(test_l77_grid_auto_6);
    RUN_TEST(test_l77_grid_auto_7);
    RUN_TEST(test_l77_grid_auto_8);
    RUN_TEST(test_l77_grid_auto_9);
    RUN_TEST(test_l77_grid_auto_10);
    RUN_TEST(test_l77_grid_auto_11);
    RUN_TEST(test_l77_grid_auto_12);
    RUN_TEST(test_l77_grid_auto_13);
    RUN_TEST(test_l77_grid_auto_14);
    RUN_TEST(test_l77_grid_auto_15);
    RUN_TEST(test_l77_grid_auto_16);
    RUN_TEST(test_l77_grid_auto_17);
    RUN_TEST(test_l77_grid_auto_18);
    RUN_TEST(test_l77_grid_auto_19);
    RUN_TEST(test_l77_grid_auto_20);
    RUN_TEST(test_l77_grid_auto_21);
    RUN_TEST(test_l77_grid_auto_22);
    RUN_TEST(test_l77_grid_auto_23);
    RUN_TEST(test_l77_grid_auto_24);
    RUN_TEST(test_l77_grid_auto_25);
    RUN_TEST(test_l77_grid_auto_26);
    RUN_TEST(test_l77_grid_auto_27);
    RUN_TEST(test_l77_grid_auto_28);
    RUN_TEST(test_l77_grid_auto_29);
    RUN_TEST(test_l77_grid_auto_30);
    RUN_TEST(test_l77_grid_auto_31);
    RUN_TEST(test_l77_grid_auto_32);
    RUN_TEST(test_l77_grid_auto_33);
    RUN_TEST(test_l77_grid_auto_34);
    RUN_TEST(test_l77_grid_auto_35);
    RUN_TEST(test_l77_grid_auto_36);
    RUN_TEST(test_l77_grid_auto_37);
    RUN_TEST(test_l77_grid_auto_38);
    RUN_TEST(test_l77_grid_auto_39);
    RUN_TEST(test_l77_grid_auto_40);
}
