/* Lot 78: more grid placement / place-content / 2d track systematics. */

static void test_l78_mix_1(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:68px;height:29px;grid-template-columns:repeat(3,22px);grid-template-rows:repeat(2,14px);gap:1px;align-items:stretch;justify-items:stretch'><div id=m1_0></div><div id=m1_1></div><div id=m1_2></div><div id=m1_3></div><div id=m1_4></div><div id=m1_5></div></div></body>", 480);
    TEST_ASSERT_EQUAL(3, lot52_node("g")->grid_cols);
    TEST_ASSERT_EQUAL(2, lot52_node("g")->grid_rows);
    TEST_ASSERT_EQUAL(0, lot52_node("m1_0")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("m1_0")->y);
    TEST_ASSERT_EQUAL(22, lot52_node("m1_0")->w);
    TEST_ASSERT_EQUAL(14, lot52_node("m1_0")->h);
    TEST_ASSERT_EQUAL(23, lot52_node("m1_1")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("m1_1")->y);
    TEST_ASSERT_EQUAL(22, lot52_node("m1_1")->w);
    TEST_ASSERT_EQUAL(14, lot52_node("m1_1")->h);
    TEST_ASSERT_EQUAL(46, lot52_node("m1_2")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("m1_2")->y);
    TEST_ASSERT_EQUAL(22, lot52_node("m1_2")->w);
    TEST_ASSERT_EQUAL(14, lot52_node("m1_2")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("m1_3")->x);
    TEST_ASSERT_EQUAL(15, lot52_node("m1_3")->y);
    TEST_ASSERT_EQUAL(22, lot52_node("m1_3")->w);
    TEST_ASSERT_EQUAL(14, lot52_node("m1_3")->h);
    TEST_ASSERT_EQUAL(23, lot52_node("m1_4")->x);
    TEST_ASSERT_EQUAL(15, lot52_node("m1_4")->y);
    TEST_ASSERT_EQUAL(22, lot52_node("m1_4")->w);
    TEST_ASSERT_EQUAL(14, lot52_node("m1_4")->h);
    TEST_ASSERT_EQUAL(46, lot52_node("m1_5")->x);
    TEST_ASSERT_EQUAL(15, lot52_node("m1_5")->y);
    TEST_ASSERT_EQUAL(22, lot52_node("m1_5")->w);
    TEST_ASSERT_EQUAL(14, lot52_node("m1_5")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_mix_2(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:50px;height:52px;grid-template-columns:repeat(2,24px);grid-template-rows:repeat(3,16px);gap:2px;align-items:stretch;justify-items:stretch'><div id=m2_0></div><div id=m2_1></div><div id=m2_2></div><div id=m2_3></div><div id=m2_4></div><div id=m2_5></div></div></body>", 480);
    TEST_ASSERT_EQUAL(2, lot52_node("g")->grid_cols);
    TEST_ASSERT_EQUAL(3, lot52_node("g")->grid_rows);
    TEST_ASSERT_EQUAL(0, lot52_node("m2_0")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("m2_0")->y);
    TEST_ASSERT_EQUAL(24, lot52_node("m2_0")->w);
    TEST_ASSERT_EQUAL(16, lot52_node("m2_0")->h);
    TEST_ASSERT_EQUAL(26, lot52_node("m2_1")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("m2_1")->y);
    TEST_ASSERT_EQUAL(24, lot52_node("m2_1")->w);
    TEST_ASSERT_EQUAL(16, lot52_node("m2_1")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("m2_2")->x);
    TEST_ASSERT_EQUAL(18, lot52_node("m2_2")->y);
    TEST_ASSERT_EQUAL(24, lot52_node("m2_2")->w);
    TEST_ASSERT_EQUAL(16, lot52_node("m2_2")->h);
    TEST_ASSERT_EQUAL(26, lot52_node("m2_3")->x);
    TEST_ASSERT_EQUAL(18, lot52_node("m2_3")->y);
    TEST_ASSERT_EQUAL(24, lot52_node("m2_3")->w);
    TEST_ASSERT_EQUAL(16, lot52_node("m2_3")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("m2_4")->x);
    TEST_ASSERT_EQUAL(36, lot52_node("m2_4")->y);
    TEST_ASSERT_EQUAL(24, lot52_node("m2_4")->w);
    TEST_ASSERT_EQUAL(16, lot52_node("m2_4")->h);
    TEST_ASSERT_EQUAL(26, lot52_node("m2_5")->x);
    TEST_ASSERT_EQUAL(36, lot52_node("m2_5")->y);
    TEST_ASSERT_EQUAL(24, lot52_node("m2_5")->w);
    TEST_ASSERT_EQUAL(16, lot52_node("m2_5")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_mix_3(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:84px;height:42px;grid-template-columns:repeat(3,26px);grid-template-rows:repeat(3,12px);gap:3px;align-items:stretch;justify-items:stretch'><div id=m3_0></div><div id=m3_1></div><div id=m3_2></div><div id=m3_3></div><div id=m3_4></div><div id=m3_5></div><div id=m3_6></div><div id=m3_7></div><div id=m3_8></div></div></body>", 480);
    TEST_ASSERT_EQUAL(3, lot52_node("g")->grid_cols);
    TEST_ASSERT_EQUAL(3, lot52_node("g")->grid_rows);
    TEST_ASSERT_EQUAL(0, lot52_node("m3_0")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("m3_0")->y);
    TEST_ASSERT_EQUAL(26, lot52_node("m3_0")->w);
    TEST_ASSERT_EQUAL(12, lot52_node("m3_0")->h);
    TEST_ASSERT_EQUAL(29, lot52_node("m3_1")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("m3_1")->y);
    TEST_ASSERT_EQUAL(26, lot52_node("m3_1")->w);
    TEST_ASSERT_EQUAL(12, lot52_node("m3_1")->h);
    TEST_ASSERT_EQUAL(58, lot52_node("m3_2")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("m3_2")->y);
    TEST_ASSERT_EQUAL(26, lot52_node("m3_2")->w);
    TEST_ASSERT_EQUAL(12, lot52_node("m3_2")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("m3_3")->x);
    TEST_ASSERT_EQUAL(15, lot52_node("m3_3")->y);
    TEST_ASSERT_EQUAL(26, lot52_node("m3_3")->w);
    TEST_ASSERT_EQUAL(12, lot52_node("m3_3")->h);
    TEST_ASSERT_EQUAL(29, lot52_node("m3_4")->x);
    TEST_ASSERT_EQUAL(15, lot52_node("m3_4")->y);
    TEST_ASSERT_EQUAL(26, lot52_node("m3_4")->w);
    TEST_ASSERT_EQUAL(12, lot52_node("m3_4")->h);
    TEST_ASSERT_EQUAL(58, lot52_node("m3_5")->x);
    TEST_ASSERT_EQUAL(15, lot52_node("m3_5")->y);
    TEST_ASSERT_EQUAL(26, lot52_node("m3_5")->w);
    TEST_ASSERT_EQUAL(12, lot52_node("m3_5")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("m3_6")->x);
    TEST_ASSERT_EQUAL(30, lot52_node("m3_6")->y);
    TEST_ASSERT_EQUAL(26, lot52_node("m3_6")->w);
    TEST_ASSERT_EQUAL(12, lot52_node("m3_6")->h);
    TEST_ASSERT_EQUAL(29, lot52_node("m3_7")->x);
    TEST_ASSERT_EQUAL(30, lot52_node("m3_7")->y);
    TEST_ASSERT_EQUAL(26, lot52_node("m3_7")->w);
    TEST_ASSERT_EQUAL(12, lot52_node("m3_7")->h);
    TEST_ASSERT_EQUAL(58, lot52_node("m3_8")->x);
    TEST_ASSERT_EQUAL(30, lot52_node("m3_8")->y);
    TEST_ASSERT_EQUAL(26, lot52_node("m3_8")->w);
    TEST_ASSERT_EQUAL(12, lot52_node("m3_8")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_mix_4(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:28px;grid-template-columns:repeat(2,20px);grid-template-rows:repeat(2,14px);gap:0px;align-items:stretch;justify-items:stretch'><div id=m4_0></div><div id=m4_1></div><div id=m4_2></div><div id=m4_3></div></div></body>", 480);
    TEST_ASSERT_EQUAL(2, lot52_node("g")->grid_cols);
    TEST_ASSERT_EQUAL(2, lot52_node("g")->grid_rows);
    TEST_ASSERT_EQUAL(0, lot52_node("m4_0")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("m4_0")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("m4_0")->w);
    TEST_ASSERT_EQUAL(14, lot52_node("m4_0")->h);
    TEST_ASSERT_EQUAL(20, lot52_node("m4_1")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("m4_1")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("m4_1")->w);
    TEST_ASSERT_EQUAL(14, lot52_node("m4_1")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("m4_2")->x);
    TEST_ASSERT_EQUAL(14, lot52_node("m4_2")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("m4_2")->w);
    TEST_ASSERT_EQUAL(14, lot52_node("m4_2")->h);
    TEST_ASSERT_EQUAL(20, lot52_node("m4_3")->x);
    TEST_ASSERT_EQUAL(14, lot52_node("m4_3")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("m4_3")->w);
    TEST_ASSERT_EQUAL(14, lot52_node("m4_3")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_mix_5(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:68px;height:33px;grid-template-columns:repeat(3,22px);grid-template-rows:repeat(2,16px);gap:1px;align-items:stretch;justify-items:stretch'><div id=m5_0></div><div id=m5_1></div><div id=m5_2></div><div id=m5_3></div><div id=m5_4></div><div id=m5_5></div></div></body>", 480);
    TEST_ASSERT_EQUAL(3, lot52_node("g")->grid_cols);
    TEST_ASSERT_EQUAL(2, lot52_node("g")->grid_rows);
    TEST_ASSERT_EQUAL(0, lot52_node("m5_0")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("m5_0")->y);
    TEST_ASSERT_EQUAL(22, lot52_node("m5_0")->w);
    TEST_ASSERT_EQUAL(16, lot52_node("m5_0")->h);
    TEST_ASSERT_EQUAL(23, lot52_node("m5_1")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("m5_1")->y);
    TEST_ASSERT_EQUAL(22, lot52_node("m5_1")->w);
    TEST_ASSERT_EQUAL(16, lot52_node("m5_1")->h);
    TEST_ASSERT_EQUAL(46, lot52_node("m5_2")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("m5_2")->y);
    TEST_ASSERT_EQUAL(22, lot52_node("m5_2")->w);
    TEST_ASSERT_EQUAL(16, lot52_node("m5_2")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("m5_3")->x);
    TEST_ASSERT_EQUAL(17, lot52_node("m5_3")->y);
    TEST_ASSERT_EQUAL(22, lot52_node("m5_3")->w);
    TEST_ASSERT_EQUAL(16, lot52_node("m5_3")->h);
    TEST_ASSERT_EQUAL(23, lot52_node("m5_4")->x);
    TEST_ASSERT_EQUAL(17, lot52_node("m5_4")->y);
    TEST_ASSERT_EQUAL(22, lot52_node("m5_4")->w);
    TEST_ASSERT_EQUAL(16, lot52_node("m5_4")->h);
    TEST_ASSERT_EQUAL(46, lot52_node("m5_5")->x);
    TEST_ASSERT_EQUAL(17, lot52_node("m5_5")->y);
    TEST_ASSERT_EQUAL(22, lot52_node("m5_5")->w);
    TEST_ASSERT_EQUAL(16, lot52_node("m5_5")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_mix_6(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:50px;height:40px;grid-template-columns:repeat(2,24px);grid-template-rows:repeat(3,12px);gap:2px;align-items:stretch;justify-items:stretch'><div id=m6_0></div><div id=m6_1></div><div id=m6_2></div><div id=m6_3></div><div id=m6_4></div><div id=m6_5></div></div></body>", 480);
    TEST_ASSERT_EQUAL(2, lot52_node("g")->grid_cols);
    TEST_ASSERT_EQUAL(3, lot52_node("g")->grid_rows);
    TEST_ASSERT_EQUAL(0, lot52_node("m6_0")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("m6_0")->y);
    TEST_ASSERT_EQUAL(24, lot52_node("m6_0")->w);
    TEST_ASSERT_EQUAL(12, lot52_node("m6_0")->h);
    TEST_ASSERT_EQUAL(26, lot52_node("m6_1")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("m6_1")->y);
    TEST_ASSERT_EQUAL(24, lot52_node("m6_1")->w);
    TEST_ASSERT_EQUAL(12, lot52_node("m6_1")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("m6_2")->x);
    TEST_ASSERT_EQUAL(14, lot52_node("m6_2")->y);
    TEST_ASSERT_EQUAL(24, lot52_node("m6_2")->w);
    TEST_ASSERT_EQUAL(12, lot52_node("m6_2")->h);
    TEST_ASSERT_EQUAL(26, lot52_node("m6_3")->x);
    TEST_ASSERT_EQUAL(14, lot52_node("m6_3")->y);
    TEST_ASSERT_EQUAL(24, lot52_node("m6_3")->w);
    TEST_ASSERT_EQUAL(12, lot52_node("m6_3")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("m6_4")->x);
    TEST_ASSERT_EQUAL(28, lot52_node("m6_4")->y);
    TEST_ASSERT_EQUAL(24, lot52_node("m6_4")->w);
    TEST_ASSERT_EQUAL(12, lot52_node("m6_4")->h);
    TEST_ASSERT_EQUAL(26, lot52_node("m6_5")->x);
    TEST_ASSERT_EQUAL(28, lot52_node("m6_5")->y);
    TEST_ASSERT_EQUAL(24, lot52_node("m6_5")->w);
    TEST_ASSERT_EQUAL(12, lot52_node("m6_5")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_mix_7(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:84px;height:48px;grid-template-columns:repeat(3,26px);grid-template-rows:repeat(3,14px);gap:3px;align-items:stretch;justify-items:stretch'><div id=m7_0></div><div id=m7_1></div><div id=m7_2></div><div id=m7_3></div><div id=m7_4></div><div id=m7_5></div><div id=m7_6></div><div id=m7_7></div><div id=m7_8></div></div></body>", 480);
    TEST_ASSERT_EQUAL(3, lot52_node("g")->grid_cols);
    TEST_ASSERT_EQUAL(3, lot52_node("g")->grid_rows);
    TEST_ASSERT_EQUAL(0, lot52_node("m7_0")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("m7_0")->y);
    TEST_ASSERT_EQUAL(26, lot52_node("m7_0")->w);
    TEST_ASSERT_EQUAL(14, lot52_node("m7_0")->h);
    TEST_ASSERT_EQUAL(29, lot52_node("m7_1")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("m7_1")->y);
    TEST_ASSERT_EQUAL(26, lot52_node("m7_1")->w);
    TEST_ASSERT_EQUAL(14, lot52_node("m7_1")->h);
    TEST_ASSERT_EQUAL(58, lot52_node("m7_2")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("m7_2")->y);
    TEST_ASSERT_EQUAL(26, lot52_node("m7_2")->w);
    TEST_ASSERT_EQUAL(14, lot52_node("m7_2")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("m7_3")->x);
    TEST_ASSERT_EQUAL(17, lot52_node("m7_3")->y);
    TEST_ASSERT_EQUAL(26, lot52_node("m7_3")->w);
    TEST_ASSERT_EQUAL(14, lot52_node("m7_3")->h);
    TEST_ASSERT_EQUAL(29, lot52_node("m7_4")->x);
    TEST_ASSERT_EQUAL(17, lot52_node("m7_4")->y);
    TEST_ASSERT_EQUAL(26, lot52_node("m7_4")->w);
    TEST_ASSERT_EQUAL(14, lot52_node("m7_4")->h);
    TEST_ASSERT_EQUAL(58, lot52_node("m7_5")->x);
    TEST_ASSERT_EQUAL(17, lot52_node("m7_5")->y);
    TEST_ASSERT_EQUAL(26, lot52_node("m7_5")->w);
    TEST_ASSERT_EQUAL(14, lot52_node("m7_5")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("m7_6")->x);
    TEST_ASSERT_EQUAL(34, lot52_node("m7_6")->y);
    TEST_ASSERT_EQUAL(26, lot52_node("m7_6")->w);
    TEST_ASSERT_EQUAL(14, lot52_node("m7_6")->h);
    TEST_ASSERT_EQUAL(29, lot52_node("m7_7")->x);
    TEST_ASSERT_EQUAL(34, lot52_node("m7_7")->y);
    TEST_ASSERT_EQUAL(26, lot52_node("m7_7")->w);
    TEST_ASSERT_EQUAL(14, lot52_node("m7_7")->h);
    TEST_ASSERT_EQUAL(58, lot52_node("m7_8")->x);
    TEST_ASSERT_EQUAL(34, lot52_node("m7_8")->y);
    TEST_ASSERT_EQUAL(26, lot52_node("m7_8")->w);
    TEST_ASSERT_EQUAL(14, lot52_node("m7_8")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_mix_8(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:32px;grid-template-columns:repeat(2,20px);grid-template-rows:repeat(2,16px);gap:0px;align-items:stretch;justify-items:stretch'><div id=m8_0></div><div id=m8_1></div><div id=m8_2></div><div id=m8_3></div></div></body>", 480);
    TEST_ASSERT_EQUAL(2, lot52_node("g")->grid_cols);
    TEST_ASSERT_EQUAL(2, lot52_node("g")->grid_rows);
    TEST_ASSERT_EQUAL(0, lot52_node("m8_0")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("m8_0")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("m8_0")->w);
    TEST_ASSERT_EQUAL(16, lot52_node("m8_0")->h);
    TEST_ASSERT_EQUAL(20, lot52_node("m8_1")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("m8_1")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("m8_1")->w);
    TEST_ASSERT_EQUAL(16, lot52_node("m8_1")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("m8_2")->x);
    TEST_ASSERT_EQUAL(16, lot52_node("m8_2")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("m8_2")->w);
    TEST_ASSERT_EQUAL(16, lot52_node("m8_2")->h);
    TEST_ASSERT_EQUAL(20, lot52_node("m8_3")->x);
    TEST_ASSERT_EQUAL(16, lot52_node("m8_3")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("m8_3")->w);
    TEST_ASSERT_EQUAL(16, lot52_node("m8_3")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_mix_9(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:68px;height:25px;grid-template-columns:repeat(3,22px);grid-template-rows:repeat(2,12px);gap:1px;align-items:stretch;justify-items:stretch'><div id=m9_0></div><div id=m9_1></div><div id=m9_2></div><div id=m9_3></div><div id=m9_4></div><div id=m9_5></div></div></body>", 480);
    TEST_ASSERT_EQUAL(3, lot52_node("g")->grid_cols);
    TEST_ASSERT_EQUAL(2, lot52_node("g")->grid_rows);
    TEST_ASSERT_EQUAL(0, lot52_node("m9_0")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("m9_0")->y);
    TEST_ASSERT_EQUAL(22, lot52_node("m9_0")->w);
    TEST_ASSERT_EQUAL(12, lot52_node("m9_0")->h);
    TEST_ASSERT_EQUAL(23, lot52_node("m9_1")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("m9_1")->y);
    TEST_ASSERT_EQUAL(22, lot52_node("m9_1")->w);
    TEST_ASSERT_EQUAL(12, lot52_node("m9_1")->h);
    TEST_ASSERT_EQUAL(46, lot52_node("m9_2")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("m9_2")->y);
    TEST_ASSERT_EQUAL(22, lot52_node("m9_2")->w);
    TEST_ASSERT_EQUAL(12, lot52_node("m9_2")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("m9_3")->x);
    TEST_ASSERT_EQUAL(13, lot52_node("m9_3")->y);
    TEST_ASSERT_EQUAL(22, lot52_node("m9_3")->w);
    TEST_ASSERT_EQUAL(12, lot52_node("m9_3")->h);
    TEST_ASSERT_EQUAL(23, lot52_node("m9_4")->x);
    TEST_ASSERT_EQUAL(13, lot52_node("m9_4")->y);
    TEST_ASSERT_EQUAL(22, lot52_node("m9_4")->w);
    TEST_ASSERT_EQUAL(12, lot52_node("m9_4")->h);
    TEST_ASSERT_EQUAL(46, lot52_node("m9_5")->x);
    TEST_ASSERT_EQUAL(13, lot52_node("m9_5")->y);
    TEST_ASSERT_EQUAL(22, lot52_node("m9_5")->w);
    TEST_ASSERT_EQUAL(12, lot52_node("m9_5")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_mix_10(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:50px;height:46px;grid-template-columns:repeat(2,24px);grid-template-rows:repeat(3,14px);gap:2px;align-items:stretch;justify-items:stretch'><div id=m10_0></div><div id=m10_1></div><div id=m10_2></div><div id=m10_3></div><div id=m10_4></div><div id=m10_5></div></div></body>", 480);
    TEST_ASSERT_EQUAL(2, lot52_node("g")->grid_cols);
    TEST_ASSERT_EQUAL(3, lot52_node("g")->grid_rows);
    TEST_ASSERT_EQUAL(0, lot52_node("m10_0")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("m10_0")->y);
    TEST_ASSERT_EQUAL(24, lot52_node("m10_0")->w);
    TEST_ASSERT_EQUAL(14, lot52_node("m10_0")->h);
    TEST_ASSERT_EQUAL(26, lot52_node("m10_1")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("m10_1")->y);
    TEST_ASSERT_EQUAL(24, lot52_node("m10_1")->w);
    TEST_ASSERT_EQUAL(14, lot52_node("m10_1")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("m10_2")->x);
    TEST_ASSERT_EQUAL(16, lot52_node("m10_2")->y);
    TEST_ASSERT_EQUAL(24, lot52_node("m10_2")->w);
    TEST_ASSERT_EQUAL(14, lot52_node("m10_2")->h);
    TEST_ASSERT_EQUAL(26, lot52_node("m10_3")->x);
    TEST_ASSERT_EQUAL(16, lot52_node("m10_3")->y);
    TEST_ASSERT_EQUAL(24, lot52_node("m10_3")->w);
    TEST_ASSERT_EQUAL(14, lot52_node("m10_3")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("m10_4")->x);
    TEST_ASSERT_EQUAL(32, lot52_node("m10_4")->y);
    TEST_ASSERT_EQUAL(24, lot52_node("m10_4")->w);
    TEST_ASSERT_EQUAL(14, lot52_node("m10_4")->h);
    TEST_ASSERT_EQUAL(26, lot52_node("m10_5")->x);
    TEST_ASSERT_EQUAL(32, lot52_node("m10_5")->y);
    TEST_ASSERT_EQUAL(24, lot52_node("m10_5")->w);
    TEST_ASSERT_EQUAL(14, lot52_node("m10_5")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_mix_11(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:84px;height:54px;grid-template-columns:repeat(3,26px);grid-template-rows:repeat(3,16px);gap:3px;align-items:stretch;justify-items:stretch'><div id=m11_0></div><div id=m11_1></div><div id=m11_2></div><div id=m11_3></div><div id=m11_4></div><div id=m11_5></div><div id=m11_6></div><div id=m11_7></div><div id=m11_8></div></div></body>", 480);
    TEST_ASSERT_EQUAL(3, lot52_node("g")->grid_cols);
    TEST_ASSERT_EQUAL(3, lot52_node("g")->grid_rows);
    TEST_ASSERT_EQUAL(0, lot52_node("m11_0")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("m11_0")->y);
    TEST_ASSERT_EQUAL(26, lot52_node("m11_0")->w);
    TEST_ASSERT_EQUAL(16, lot52_node("m11_0")->h);
    TEST_ASSERT_EQUAL(29, lot52_node("m11_1")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("m11_1")->y);
    TEST_ASSERT_EQUAL(26, lot52_node("m11_1")->w);
    TEST_ASSERT_EQUAL(16, lot52_node("m11_1")->h);
    TEST_ASSERT_EQUAL(58, lot52_node("m11_2")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("m11_2")->y);
    TEST_ASSERT_EQUAL(26, lot52_node("m11_2")->w);
    TEST_ASSERT_EQUAL(16, lot52_node("m11_2")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("m11_3")->x);
    TEST_ASSERT_EQUAL(19, lot52_node("m11_3")->y);
    TEST_ASSERT_EQUAL(26, lot52_node("m11_3")->w);
    TEST_ASSERT_EQUAL(16, lot52_node("m11_3")->h);
    TEST_ASSERT_EQUAL(29, lot52_node("m11_4")->x);
    TEST_ASSERT_EQUAL(19, lot52_node("m11_4")->y);
    TEST_ASSERT_EQUAL(26, lot52_node("m11_4")->w);
    TEST_ASSERT_EQUAL(16, lot52_node("m11_4")->h);
    TEST_ASSERT_EQUAL(58, lot52_node("m11_5")->x);
    TEST_ASSERT_EQUAL(19, lot52_node("m11_5")->y);
    TEST_ASSERT_EQUAL(26, lot52_node("m11_5")->w);
    TEST_ASSERT_EQUAL(16, lot52_node("m11_5")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("m11_6")->x);
    TEST_ASSERT_EQUAL(38, lot52_node("m11_6")->y);
    TEST_ASSERT_EQUAL(26, lot52_node("m11_6")->w);
    TEST_ASSERT_EQUAL(16, lot52_node("m11_6")->h);
    TEST_ASSERT_EQUAL(29, lot52_node("m11_7")->x);
    TEST_ASSERT_EQUAL(38, lot52_node("m11_7")->y);
    TEST_ASSERT_EQUAL(26, lot52_node("m11_7")->w);
    TEST_ASSERT_EQUAL(16, lot52_node("m11_7")->h);
    TEST_ASSERT_EQUAL(58, lot52_node("m11_8")->x);
    TEST_ASSERT_EQUAL(38, lot52_node("m11_8")->y);
    TEST_ASSERT_EQUAL(26, lot52_node("m11_8")->w);
    TEST_ASSERT_EQUAL(16, lot52_node("m11_8")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_mix_12(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:24px;grid-template-columns:repeat(2,20px);grid-template-rows:repeat(2,12px);gap:0px;align-items:stretch;justify-items:stretch'><div id=m12_0></div><div id=m12_1></div><div id=m12_2></div><div id=m12_3></div></div></body>", 480);
    TEST_ASSERT_EQUAL(2, lot52_node("g")->grid_cols);
    TEST_ASSERT_EQUAL(2, lot52_node("g")->grid_rows);
    TEST_ASSERT_EQUAL(0, lot52_node("m12_0")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("m12_0")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("m12_0")->w);
    TEST_ASSERT_EQUAL(12, lot52_node("m12_0")->h);
    TEST_ASSERT_EQUAL(20, lot52_node("m12_1")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("m12_1")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("m12_1")->w);
    TEST_ASSERT_EQUAL(12, lot52_node("m12_1")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("m12_2")->x);
    TEST_ASSERT_EQUAL(12, lot52_node("m12_2")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("m12_2")->w);
    TEST_ASSERT_EQUAL(12, lot52_node("m12_2")->h);
    TEST_ASSERT_EQUAL(20, lot52_node("m12_3")->x);
    TEST_ASSERT_EQUAL(12, lot52_node("m12_3")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("m12_3")->w);
    TEST_ASSERT_EQUAL(12, lot52_node("m12_3")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_mix_13(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:68px;height:29px;grid-template-columns:repeat(3,22px);grid-template-rows:repeat(2,14px);gap:1px;align-items:stretch;justify-items:stretch'><div id=m13_0></div><div id=m13_1></div><div id=m13_2></div><div id=m13_3></div><div id=m13_4></div><div id=m13_5></div></div></body>", 480);
    TEST_ASSERT_EQUAL(3, lot52_node("g")->grid_cols);
    TEST_ASSERT_EQUAL(2, lot52_node("g")->grid_rows);
    TEST_ASSERT_EQUAL(0, lot52_node("m13_0")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("m13_0")->y);
    TEST_ASSERT_EQUAL(22, lot52_node("m13_0")->w);
    TEST_ASSERT_EQUAL(14, lot52_node("m13_0")->h);
    TEST_ASSERT_EQUAL(23, lot52_node("m13_1")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("m13_1")->y);
    TEST_ASSERT_EQUAL(22, lot52_node("m13_1")->w);
    TEST_ASSERT_EQUAL(14, lot52_node("m13_1")->h);
    TEST_ASSERT_EQUAL(46, lot52_node("m13_2")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("m13_2")->y);
    TEST_ASSERT_EQUAL(22, lot52_node("m13_2")->w);
    TEST_ASSERT_EQUAL(14, lot52_node("m13_2")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("m13_3")->x);
    TEST_ASSERT_EQUAL(15, lot52_node("m13_3")->y);
    TEST_ASSERT_EQUAL(22, lot52_node("m13_3")->w);
    TEST_ASSERT_EQUAL(14, lot52_node("m13_3")->h);
    TEST_ASSERT_EQUAL(23, lot52_node("m13_4")->x);
    TEST_ASSERT_EQUAL(15, lot52_node("m13_4")->y);
    TEST_ASSERT_EQUAL(22, lot52_node("m13_4")->w);
    TEST_ASSERT_EQUAL(14, lot52_node("m13_4")->h);
    TEST_ASSERT_EQUAL(46, lot52_node("m13_5")->x);
    TEST_ASSERT_EQUAL(15, lot52_node("m13_5")->y);
    TEST_ASSERT_EQUAL(22, lot52_node("m13_5")->w);
    TEST_ASSERT_EQUAL(14, lot52_node("m13_5")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_mix_14(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:50px;height:52px;grid-template-columns:repeat(2,24px);grid-template-rows:repeat(3,16px);gap:2px;align-items:stretch;justify-items:stretch'><div id=m14_0></div><div id=m14_1></div><div id=m14_2></div><div id=m14_3></div><div id=m14_4></div><div id=m14_5></div></div></body>", 480);
    TEST_ASSERT_EQUAL(2, lot52_node("g")->grid_cols);
    TEST_ASSERT_EQUAL(3, lot52_node("g")->grid_rows);
    TEST_ASSERT_EQUAL(0, lot52_node("m14_0")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("m14_0")->y);
    TEST_ASSERT_EQUAL(24, lot52_node("m14_0")->w);
    TEST_ASSERT_EQUAL(16, lot52_node("m14_0")->h);
    TEST_ASSERT_EQUAL(26, lot52_node("m14_1")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("m14_1")->y);
    TEST_ASSERT_EQUAL(24, lot52_node("m14_1")->w);
    TEST_ASSERT_EQUAL(16, lot52_node("m14_1")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("m14_2")->x);
    TEST_ASSERT_EQUAL(18, lot52_node("m14_2")->y);
    TEST_ASSERT_EQUAL(24, lot52_node("m14_2")->w);
    TEST_ASSERT_EQUAL(16, lot52_node("m14_2")->h);
    TEST_ASSERT_EQUAL(26, lot52_node("m14_3")->x);
    TEST_ASSERT_EQUAL(18, lot52_node("m14_3")->y);
    TEST_ASSERT_EQUAL(24, lot52_node("m14_3")->w);
    TEST_ASSERT_EQUAL(16, lot52_node("m14_3")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("m14_4")->x);
    TEST_ASSERT_EQUAL(36, lot52_node("m14_4")->y);
    TEST_ASSERT_EQUAL(24, lot52_node("m14_4")->w);
    TEST_ASSERT_EQUAL(16, lot52_node("m14_4")->h);
    TEST_ASSERT_EQUAL(26, lot52_node("m14_5")->x);
    TEST_ASSERT_EQUAL(36, lot52_node("m14_5")->y);
    TEST_ASSERT_EQUAL(24, lot52_node("m14_5")->w);
    TEST_ASSERT_EQUAL(16, lot52_node("m14_5")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_mix_15(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:84px;height:42px;grid-template-columns:repeat(3,26px);grid-template-rows:repeat(3,12px);gap:3px;align-items:stretch;justify-items:stretch'><div id=m15_0></div><div id=m15_1></div><div id=m15_2></div><div id=m15_3></div><div id=m15_4></div><div id=m15_5></div><div id=m15_6></div><div id=m15_7></div><div id=m15_8></div></div></body>", 480);
    TEST_ASSERT_EQUAL(3, lot52_node("g")->grid_cols);
    TEST_ASSERT_EQUAL(3, lot52_node("g")->grid_rows);
    TEST_ASSERT_EQUAL(0, lot52_node("m15_0")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("m15_0")->y);
    TEST_ASSERT_EQUAL(26, lot52_node("m15_0")->w);
    TEST_ASSERT_EQUAL(12, lot52_node("m15_0")->h);
    TEST_ASSERT_EQUAL(29, lot52_node("m15_1")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("m15_1")->y);
    TEST_ASSERT_EQUAL(26, lot52_node("m15_1")->w);
    TEST_ASSERT_EQUAL(12, lot52_node("m15_1")->h);
    TEST_ASSERT_EQUAL(58, lot52_node("m15_2")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("m15_2")->y);
    TEST_ASSERT_EQUAL(26, lot52_node("m15_2")->w);
    TEST_ASSERT_EQUAL(12, lot52_node("m15_2")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("m15_3")->x);
    TEST_ASSERT_EQUAL(15, lot52_node("m15_3")->y);
    TEST_ASSERT_EQUAL(26, lot52_node("m15_3")->w);
    TEST_ASSERT_EQUAL(12, lot52_node("m15_3")->h);
    TEST_ASSERT_EQUAL(29, lot52_node("m15_4")->x);
    TEST_ASSERT_EQUAL(15, lot52_node("m15_4")->y);
    TEST_ASSERT_EQUAL(26, lot52_node("m15_4")->w);
    TEST_ASSERT_EQUAL(12, lot52_node("m15_4")->h);
    TEST_ASSERT_EQUAL(58, lot52_node("m15_5")->x);
    TEST_ASSERT_EQUAL(15, lot52_node("m15_5")->y);
    TEST_ASSERT_EQUAL(26, lot52_node("m15_5")->w);
    TEST_ASSERT_EQUAL(12, lot52_node("m15_5")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("m15_6")->x);
    TEST_ASSERT_EQUAL(30, lot52_node("m15_6")->y);
    TEST_ASSERT_EQUAL(26, lot52_node("m15_6")->w);
    TEST_ASSERT_EQUAL(12, lot52_node("m15_6")->h);
    TEST_ASSERT_EQUAL(29, lot52_node("m15_7")->x);
    TEST_ASSERT_EQUAL(30, lot52_node("m15_7")->y);
    TEST_ASSERT_EQUAL(26, lot52_node("m15_7")->w);
    TEST_ASSERT_EQUAL(12, lot52_node("m15_7")->h);
    TEST_ASSERT_EQUAL(58, lot52_node("m15_8")->x);
    TEST_ASSERT_EQUAL(30, lot52_node("m15_8")->y);
    TEST_ASSERT_EQUAL(26, lot52_node("m15_8")->w);
    TEST_ASSERT_EQUAL(12, lot52_node("m15_8")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_mix_16(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:28px;grid-template-columns:repeat(2,20px);grid-template-rows:repeat(2,14px);gap:0px;align-items:stretch;justify-items:stretch'><div id=m16_0></div><div id=m16_1></div><div id=m16_2></div><div id=m16_3></div></div></body>", 480);
    TEST_ASSERT_EQUAL(2, lot52_node("g")->grid_cols);
    TEST_ASSERT_EQUAL(2, lot52_node("g")->grid_rows);
    TEST_ASSERT_EQUAL(0, lot52_node("m16_0")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("m16_0")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("m16_0")->w);
    TEST_ASSERT_EQUAL(14, lot52_node("m16_0")->h);
    TEST_ASSERT_EQUAL(20, lot52_node("m16_1")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("m16_1")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("m16_1")->w);
    TEST_ASSERT_EQUAL(14, lot52_node("m16_1")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("m16_2")->x);
    TEST_ASSERT_EQUAL(14, lot52_node("m16_2")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("m16_2")->w);
    TEST_ASSERT_EQUAL(14, lot52_node("m16_2")->h);
    TEST_ASSERT_EQUAL(20, lot52_node("m16_3")->x);
    TEST_ASSERT_EQUAL(14, lot52_node("m16_3")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("m16_3")->w);
    TEST_ASSERT_EQUAL(14, lot52_node("m16_3")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_mix_17(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:68px;height:33px;grid-template-columns:repeat(3,22px);grid-template-rows:repeat(2,16px);gap:1px;align-items:stretch;justify-items:stretch'><div id=m17_0></div><div id=m17_1></div><div id=m17_2></div><div id=m17_3></div><div id=m17_4></div><div id=m17_5></div></div></body>", 480);
    TEST_ASSERT_EQUAL(3, lot52_node("g")->grid_cols);
    TEST_ASSERT_EQUAL(2, lot52_node("g")->grid_rows);
    TEST_ASSERT_EQUAL(0, lot52_node("m17_0")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("m17_0")->y);
    TEST_ASSERT_EQUAL(22, lot52_node("m17_0")->w);
    TEST_ASSERT_EQUAL(16, lot52_node("m17_0")->h);
    TEST_ASSERT_EQUAL(23, lot52_node("m17_1")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("m17_1")->y);
    TEST_ASSERT_EQUAL(22, lot52_node("m17_1")->w);
    TEST_ASSERT_EQUAL(16, lot52_node("m17_1")->h);
    TEST_ASSERT_EQUAL(46, lot52_node("m17_2")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("m17_2")->y);
    TEST_ASSERT_EQUAL(22, lot52_node("m17_2")->w);
    TEST_ASSERT_EQUAL(16, lot52_node("m17_2")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("m17_3")->x);
    TEST_ASSERT_EQUAL(17, lot52_node("m17_3")->y);
    TEST_ASSERT_EQUAL(22, lot52_node("m17_3")->w);
    TEST_ASSERT_EQUAL(16, lot52_node("m17_3")->h);
    TEST_ASSERT_EQUAL(23, lot52_node("m17_4")->x);
    TEST_ASSERT_EQUAL(17, lot52_node("m17_4")->y);
    TEST_ASSERT_EQUAL(22, lot52_node("m17_4")->w);
    TEST_ASSERT_EQUAL(16, lot52_node("m17_4")->h);
    TEST_ASSERT_EQUAL(46, lot52_node("m17_5")->x);
    TEST_ASSERT_EQUAL(17, lot52_node("m17_5")->y);
    TEST_ASSERT_EQUAL(22, lot52_node("m17_5")->w);
    TEST_ASSERT_EQUAL(16, lot52_node("m17_5")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_mix_18(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:50px;height:40px;grid-template-columns:repeat(2,24px);grid-template-rows:repeat(3,12px);gap:2px;align-items:stretch;justify-items:stretch'><div id=m18_0></div><div id=m18_1></div><div id=m18_2></div><div id=m18_3></div><div id=m18_4></div><div id=m18_5></div></div></body>", 480);
    TEST_ASSERT_EQUAL(2, lot52_node("g")->grid_cols);
    TEST_ASSERT_EQUAL(3, lot52_node("g")->grid_rows);
    TEST_ASSERT_EQUAL(0, lot52_node("m18_0")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("m18_0")->y);
    TEST_ASSERT_EQUAL(24, lot52_node("m18_0")->w);
    TEST_ASSERT_EQUAL(12, lot52_node("m18_0")->h);
    TEST_ASSERT_EQUAL(26, lot52_node("m18_1")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("m18_1")->y);
    TEST_ASSERT_EQUAL(24, lot52_node("m18_1")->w);
    TEST_ASSERT_EQUAL(12, lot52_node("m18_1")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("m18_2")->x);
    TEST_ASSERT_EQUAL(14, lot52_node("m18_2")->y);
    TEST_ASSERT_EQUAL(24, lot52_node("m18_2")->w);
    TEST_ASSERT_EQUAL(12, lot52_node("m18_2")->h);
    TEST_ASSERT_EQUAL(26, lot52_node("m18_3")->x);
    TEST_ASSERT_EQUAL(14, lot52_node("m18_3")->y);
    TEST_ASSERT_EQUAL(24, lot52_node("m18_3")->w);
    TEST_ASSERT_EQUAL(12, lot52_node("m18_3")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("m18_4")->x);
    TEST_ASSERT_EQUAL(28, lot52_node("m18_4")->y);
    TEST_ASSERT_EQUAL(24, lot52_node("m18_4")->w);
    TEST_ASSERT_EQUAL(12, lot52_node("m18_4")->h);
    TEST_ASSERT_EQUAL(26, lot52_node("m18_5")->x);
    TEST_ASSERT_EQUAL(28, lot52_node("m18_5")->y);
    TEST_ASSERT_EQUAL(24, lot52_node("m18_5")->w);
    TEST_ASSERT_EQUAL(12, lot52_node("m18_5")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_mix_19(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:84px;height:48px;grid-template-columns:repeat(3,26px);grid-template-rows:repeat(3,14px);gap:3px;align-items:stretch;justify-items:stretch'><div id=m19_0></div><div id=m19_1></div><div id=m19_2></div><div id=m19_3></div><div id=m19_4></div><div id=m19_5></div><div id=m19_6></div><div id=m19_7></div><div id=m19_8></div></div></body>", 480);
    TEST_ASSERT_EQUAL(3, lot52_node("g")->grid_cols);
    TEST_ASSERT_EQUAL(3, lot52_node("g")->grid_rows);
    TEST_ASSERT_EQUAL(0, lot52_node("m19_0")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("m19_0")->y);
    TEST_ASSERT_EQUAL(26, lot52_node("m19_0")->w);
    TEST_ASSERT_EQUAL(14, lot52_node("m19_0")->h);
    TEST_ASSERT_EQUAL(29, lot52_node("m19_1")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("m19_1")->y);
    TEST_ASSERT_EQUAL(26, lot52_node("m19_1")->w);
    TEST_ASSERT_EQUAL(14, lot52_node("m19_1")->h);
    TEST_ASSERT_EQUAL(58, lot52_node("m19_2")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("m19_2")->y);
    TEST_ASSERT_EQUAL(26, lot52_node("m19_2")->w);
    TEST_ASSERT_EQUAL(14, lot52_node("m19_2")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("m19_3")->x);
    TEST_ASSERT_EQUAL(17, lot52_node("m19_3")->y);
    TEST_ASSERT_EQUAL(26, lot52_node("m19_3")->w);
    TEST_ASSERT_EQUAL(14, lot52_node("m19_3")->h);
    TEST_ASSERT_EQUAL(29, lot52_node("m19_4")->x);
    TEST_ASSERT_EQUAL(17, lot52_node("m19_4")->y);
    TEST_ASSERT_EQUAL(26, lot52_node("m19_4")->w);
    TEST_ASSERT_EQUAL(14, lot52_node("m19_4")->h);
    TEST_ASSERT_EQUAL(58, lot52_node("m19_5")->x);
    TEST_ASSERT_EQUAL(17, lot52_node("m19_5")->y);
    TEST_ASSERT_EQUAL(26, lot52_node("m19_5")->w);
    TEST_ASSERT_EQUAL(14, lot52_node("m19_5")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("m19_6")->x);
    TEST_ASSERT_EQUAL(34, lot52_node("m19_6")->y);
    TEST_ASSERT_EQUAL(26, lot52_node("m19_6")->w);
    TEST_ASSERT_EQUAL(14, lot52_node("m19_6")->h);
    TEST_ASSERT_EQUAL(29, lot52_node("m19_7")->x);
    TEST_ASSERT_EQUAL(34, lot52_node("m19_7")->y);
    TEST_ASSERT_EQUAL(26, lot52_node("m19_7")->w);
    TEST_ASSERT_EQUAL(14, lot52_node("m19_7")->h);
    TEST_ASSERT_EQUAL(58, lot52_node("m19_8")->x);
    TEST_ASSERT_EQUAL(34, lot52_node("m19_8")->y);
    TEST_ASSERT_EQUAL(26, lot52_node("m19_8")->w);
    TEST_ASSERT_EQUAL(14, lot52_node("m19_8")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_mix_20(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:32px;grid-template-columns:repeat(2,20px);grid-template-rows:repeat(2,16px);gap:0px;align-items:stretch;justify-items:stretch'><div id=m20_0></div><div id=m20_1></div><div id=m20_2></div><div id=m20_3></div></div></body>", 480);
    TEST_ASSERT_EQUAL(2, lot52_node("g")->grid_cols);
    TEST_ASSERT_EQUAL(2, lot52_node("g")->grid_rows);
    TEST_ASSERT_EQUAL(0, lot52_node("m20_0")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("m20_0")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("m20_0")->w);
    TEST_ASSERT_EQUAL(16, lot52_node("m20_0")->h);
    TEST_ASSERT_EQUAL(20, lot52_node("m20_1")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("m20_1")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("m20_1")->w);
    TEST_ASSERT_EQUAL(16, lot52_node("m20_1")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("m20_2")->x);
    TEST_ASSERT_EQUAL(16, lot52_node("m20_2")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("m20_2")->w);
    TEST_ASSERT_EQUAL(16, lot52_node("m20_2")->h);
    TEST_ASSERT_EQUAL(20, lot52_node("m20_3")->x);
    TEST_ASSERT_EQUAL(16, lot52_node("m20_3")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("m20_3")->w);
    TEST_ASSERT_EQUAL(16, lot52_node("m20_3")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_mix_21(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:68px;height:25px;grid-template-columns:repeat(3,22px);grid-template-rows:repeat(2,12px);gap:1px;align-items:stretch;justify-items:stretch'><div id=m21_0></div><div id=m21_1></div><div id=m21_2></div><div id=m21_3></div><div id=m21_4></div><div id=m21_5></div></div></body>", 480);
    TEST_ASSERT_EQUAL(3, lot52_node("g")->grid_cols);
    TEST_ASSERT_EQUAL(2, lot52_node("g")->grid_rows);
    TEST_ASSERT_EQUAL(0, lot52_node("m21_0")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("m21_0")->y);
    TEST_ASSERT_EQUAL(22, lot52_node("m21_0")->w);
    TEST_ASSERT_EQUAL(12, lot52_node("m21_0")->h);
    TEST_ASSERT_EQUAL(23, lot52_node("m21_1")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("m21_1")->y);
    TEST_ASSERT_EQUAL(22, lot52_node("m21_1")->w);
    TEST_ASSERT_EQUAL(12, lot52_node("m21_1")->h);
    TEST_ASSERT_EQUAL(46, lot52_node("m21_2")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("m21_2")->y);
    TEST_ASSERT_EQUAL(22, lot52_node("m21_2")->w);
    TEST_ASSERT_EQUAL(12, lot52_node("m21_2")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("m21_3")->x);
    TEST_ASSERT_EQUAL(13, lot52_node("m21_3")->y);
    TEST_ASSERT_EQUAL(22, lot52_node("m21_3")->w);
    TEST_ASSERT_EQUAL(12, lot52_node("m21_3")->h);
    TEST_ASSERT_EQUAL(23, lot52_node("m21_4")->x);
    TEST_ASSERT_EQUAL(13, lot52_node("m21_4")->y);
    TEST_ASSERT_EQUAL(22, lot52_node("m21_4")->w);
    TEST_ASSERT_EQUAL(12, lot52_node("m21_4")->h);
    TEST_ASSERT_EQUAL(46, lot52_node("m21_5")->x);
    TEST_ASSERT_EQUAL(13, lot52_node("m21_5")->y);
    TEST_ASSERT_EQUAL(22, lot52_node("m21_5")->w);
    TEST_ASSERT_EQUAL(12, lot52_node("m21_5")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_mix_22(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:50px;height:46px;grid-template-columns:repeat(2,24px);grid-template-rows:repeat(3,14px);gap:2px;align-items:stretch;justify-items:stretch'><div id=m22_0></div><div id=m22_1></div><div id=m22_2></div><div id=m22_3></div><div id=m22_4></div><div id=m22_5></div></div></body>", 480);
    TEST_ASSERT_EQUAL(2, lot52_node("g")->grid_cols);
    TEST_ASSERT_EQUAL(3, lot52_node("g")->grid_rows);
    TEST_ASSERT_EQUAL(0, lot52_node("m22_0")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("m22_0")->y);
    TEST_ASSERT_EQUAL(24, lot52_node("m22_0")->w);
    TEST_ASSERT_EQUAL(14, lot52_node("m22_0")->h);
    TEST_ASSERT_EQUAL(26, lot52_node("m22_1")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("m22_1")->y);
    TEST_ASSERT_EQUAL(24, lot52_node("m22_1")->w);
    TEST_ASSERT_EQUAL(14, lot52_node("m22_1")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("m22_2")->x);
    TEST_ASSERT_EQUAL(16, lot52_node("m22_2")->y);
    TEST_ASSERT_EQUAL(24, lot52_node("m22_2")->w);
    TEST_ASSERT_EQUAL(14, lot52_node("m22_2")->h);
    TEST_ASSERT_EQUAL(26, lot52_node("m22_3")->x);
    TEST_ASSERT_EQUAL(16, lot52_node("m22_3")->y);
    TEST_ASSERT_EQUAL(24, lot52_node("m22_3")->w);
    TEST_ASSERT_EQUAL(14, lot52_node("m22_3")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("m22_4")->x);
    TEST_ASSERT_EQUAL(32, lot52_node("m22_4")->y);
    TEST_ASSERT_EQUAL(24, lot52_node("m22_4")->w);
    TEST_ASSERT_EQUAL(14, lot52_node("m22_4")->h);
    TEST_ASSERT_EQUAL(26, lot52_node("m22_5")->x);
    TEST_ASSERT_EQUAL(32, lot52_node("m22_5")->y);
    TEST_ASSERT_EQUAL(24, lot52_node("m22_5")->w);
    TEST_ASSERT_EQUAL(14, lot52_node("m22_5")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_mix_23(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:84px;height:54px;grid-template-columns:repeat(3,26px);grid-template-rows:repeat(3,16px);gap:3px;align-items:stretch;justify-items:stretch'><div id=m23_0></div><div id=m23_1></div><div id=m23_2></div><div id=m23_3></div><div id=m23_4></div><div id=m23_5></div><div id=m23_6></div><div id=m23_7></div><div id=m23_8></div></div></body>", 480);
    TEST_ASSERT_EQUAL(3, lot52_node("g")->grid_cols);
    TEST_ASSERT_EQUAL(3, lot52_node("g")->grid_rows);
    TEST_ASSERT_EQUAL(0, lot52_node("m23_0")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("m23_0")->y);
    TEST_ASSERT_EQUAL(26, lot52_node("m23_0")->w);
    TEST_ASSERT_EQUAL(16, lot52_node("m23_0")->h);
    TEST_ASSERT_EQUAL(29, lot52_node("m23_1")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("m23_1")->y);
    TEST_ASSERT_EQUAL(26, lot52_node("m23_1")->w);
    TEST_ASSERT_EQUAL(16, lot52_node("m23_1")->h);
    TEST_ASSERT_EQUAL(58, lot52_node("m23_2")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("m23_2")->y);
    TEST_ASSERT_EQUAL(26, lot52_node("m23_2")->w);
    TEST_ASSERT_EQUAL(16, lot52_node("m23_2")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("m23_3")->x);
    TEST_ASSERT_EQUAL(19, lot52_node("m23_3")->y);
    TEST_ASSERT_EQUAL(26, lot52_node("m23_3")->w);
    TEST_ASSERT_EQUAL(16, lot52_node("m23_3")->h);
    TEST_ASSERT_EQUAL(29, lot52_node("m23_4")->x);
    TEST_ASSERT_EQUAL(19, lot52_node("m23_4")->y);
    TEST_ASSERT_EQUAL(26, lot52_node("m23_4")->w);
    TEST_ASSERT_EQUAL(16, lot52_node("m23_4")->h);
    TEST_ASSERT_EQUAL(58, lot52_node("m23_5")->x);
    TEST_ASSERT_EQUAL(19, lot52_node("m23_5")->y);
    TEST_ASSERT_EQUAL(26, lot52_node("m23_5")->w);
    TEST_ASSERT_EQUAL(16, lot52_node("m23_5")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("m23_6")->x);
    TEST_ASSERT_EQUAL(38, lot52_node("m23_6")->y);
    TEST_ASSERT_EQUAL(26, lot52_node("m23_6")->w);
    TEST_ASSERT_EQUAL(16, lot52_node("m23_6")->h);
    TEST_ASSERT_EQUAL(29, lot52_node("m23_7")->x);
    TEST_ASSERT_EQUAL(38, lot52_node("m23_7")->y);
    TEST_ASSERT_EQUAL(26, lot52_node("m23_7")->w);
    TEST_ASSERT_EQUAL(16, lot52_node("m23_7")->h);
    TEST_ASSERT_EQUAL(58, lot52_node("m23_8")->x);
    TEST_ASSERT_EQUAL(38, lot52_node("m23_8")->y);
    TEST_ASSERT_EQUAL(26, lot52_node("m23_8")->w);
    TEST_ASSERT_EQUAL(16, lot52_node("m23_8")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_mix_24(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:24px;grid-template-columns:repeat(2,20px);grid-template-rows:repeat(2,12px);gap:0px;align-items:stretch;justify-items:stretch'><div id=m24_0></div><div id=m24_1></div><div id=m24_2></div><div id=m24_3></div></div></body>", 480);
    TEST_ASSERT_EQUAL(2, lot52_node("g")->grid_cols);
    TEST_ASSERT_EQUAL(2, lot52_node("g")->grid_rows);
    TEST_ASSERT_EQUAL(0, lot52_node("m24_0")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("m24_0")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("m24_0")->w);
    TEST_ASSERT_EQUAL(12, lot52_node("m24_0")->h);
    TEST_ASSERT_EQUAL(20, lot52_node("m24_1")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("m24_1")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("m24_1")->w);
    TEST_ASSERT_EQUAL(12, lot52_node("m24_1")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("m24_2")->x);
    TEST_ASSERT_EQUAL(12, lot52_node("m24_2")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("m24_2")->w);
    TEST_ASSERT_EQUAL(12, lot52_node("m24_2")->h);
    TEST_ASSERT_EQUAL(20, lot52_node("m24_3")->x);
    TEST_ASSERT_EQUAL(12, lot52_node("m24_3")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("m24_3")->w);
    TEST_ASSERT_EQUAL(12, lot52_node("m24_3")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_mix_25(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:68px;height:29px;grid-template-columns:repeat(3,22px);grid-template-rows:repeat(2,14px);gap:1px;align-items:stretch;justify-items:stretch'><div id=m25_0></div><div id=m25_1></div><div id=m25_2></div><div id=m25_3></div><div id=m25_4></div><div id=m25_5></div></div></body>", 480);
    TEST_ASSERT_EQUAL(3, lot52_node("g")->grid_cols);
    TEST_ASSERT_EQUAL(2, lot52_node("g")->grid_rows);
    TEST_ASSERT_EQUAL(0, lot52_node("m25_0")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("m25_0")->y);
    TEST_ASSERT_EQUAL(22, lot52_node("m25_0")->w);
    TEST_ASSERT_EQUAL(14, lot52_node("m25_0")->h);
    TEST_ASSERT_EQUAL(23, lot52_node("m25_1")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("m25_1")->y);
    TEST_ASSERT_EQUAL(22, lot52_node("m25_1")->w);
    TEST_ASSERT_EQUAL(14, lot52_node("m25_1")->h);
    TEST_ASSERT_EQUAL(46, lot52_node("m25_2")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("m25_2")->y);
    TEST_ASSERT_EQUAL(22, lot52_node("m25_2")->w);
    TEST_ASSERT_EQUAL(14, lot52_node("m25_2")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("m25_3")->x);
    TEST_ASSERT_EQUAL(15, lot52_node("m25_3")->y);
    TEST_ASSERT_EQUAL(22, lot52_node("m25_3")->w);
    TEST_ASSERT_EQUAL(14, lot52_node("m25_3")->h);
    TEST_ASSERT_EQUAL(23, lot52_node("m25_4")->x);
    TEST_ASSERT_EQUAL(15, lot52_node("m25_4")->y);
    TEST_ASSERT_EQUAL(22, lot52_node("m25_4")->w);
    TEST_ASSERT_EQUAL(14, lot52_node("m25_4")->h);
    TEST_ASSERT_EQUAL(46, lot52_node("m25_5")->x);
    TEST_ASSERT_EQUAL(15, lot52_node("m25_5")->y);
    TEST_ASSERT_EQUAL(22, lot52_node("m25_5")->w);
    TEST_ASSERT_EQUAL(14, lot52_node("m25_5")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_mix_26(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:50px;height:52px;grid-template-columns:repeat(2,24px);grid-template-rows:repeat(3,16px);gap:2px;align-items:stretch;justify-items:stretch'><div id=m26_0></div><div id=m26_1></div><div id=m26_2></div><div id=m26_3></div><div id=m26_4></div><div id=m26_5></div></div></body>", 480);
    TEST_ASSERT_EQUAL(2, lot52_node("g")->grid_cols);
    TEST_ASSERT_EQUAL(3, lot52_node("g")->grid_rows);
    TEST_ASSERT_EQUAL(0, lot52_node("m26_0")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("m26_0")->y);
    TEST_ASSERT_EQUAL(24, lot52_node("m26_0")->w);
    TEST_ASSERT_EQUAL(16, lot52_node("m26_0")->h);
    TEST_ASSERT_EQUAL(26, lot52_node("m26_1")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("m26_1")->y);
    TEST_ASSERT_EQUAL(24, lot52_node("m26_1")->w);
    TEST_ASSERT_EQUAL(16, lot52_node("m26_1")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("m26_2")->x);
    TEST_ASSERT_EQUAL(18, lot52_node("m26_2")->y);
    TEST_ASSERT_EQUAL(24, lot52_node("m26_2")->w);
    TEST_ASSERT_EQUAL(16, lot52_node("m26_2")->h);
    TEST_ASSERT_EQUAL(26, lot52_node("m26_3")->x);
    TEST_ASSERT_EQUAL(18, lot52_node("m26_3")->y);
    TEST_ASSERT_EQUAL(24, lot52_node("m26_3")->w);
    TEST_ASSERT_EQUAL(16, lot52_node("m26_3")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("m26_4")->x);
    TEST_ASSERT_EQUAL(36, lot52_node("m26_4")->y);
    TEST_ASSERT_EQUAL(24, lot52_node("m26_4")->w);
    TEST_ASSERT_EQUAL(16, lot52_node("m26_4")->h);
    TEST_ASSERT_EQUAL(26, lot52_node("m26_5")->x);
    TEST_ASSERT_EQUAL(36, lot52_node("m26_5")->y);
    TEST_ASSERT_EQUAL(24, lot52_node("m26_5")->w);
    TEST_ASSERT_EQUAL(16, lot52_node("m26_5")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_mix_27(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:84px;height:42px;grid-template-columns:repeat(3,26px);grid-template-rows:repeat(3,12px);gap:3px;align-items:stretch;justify-items:stretch'><div id=m27_0></div><div id=m27_1></div><div id=m27_2></div><div id=m27_3></div><div id=m27_4></div><div id=m27_5></div><div id=m27_6></div><div id=m27_7></div><div id=m27_8></div></div></body>", 480);
    TEST_ASSERT_EQUAL(3, lot52_node("g")->grid_cols);
    TEST_ASSERT_EQUAL(3, lot52_node("g")->grid_rows);
    TEST_ASSERT_EQUAL(0, lot52_node("m27_0")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("m27_0")->y);
    TEST_ASSERT_EQUAL(26, lot52_node("m27_0")->w);
    TEST_ASSERT_EQUAL(12, lot52_node("m27_0")->h);
    TEST_ASSERT_EQUAL(29, lot52_node("m27_1")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("m27_1")->y);
    TEST_ASSERT_EQUAL(26, lot52_node("m27_1")->w);
    TEST_ASSERT_EQUAL(12, lot52_node("m27_1")->h);
    TEST_ASSERT_EQUAL(58, lot52_node("m27_2")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("m27_2")->y);
    TEST_ASSERT_EQUAL(26, lot52_node("m27_2")->w);
    TEST_ASSERT_EQUAL(12, lot52_node("m27_2")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("m27_3")->x);
    TEST_ASSERT_EQUAL(15, lot52_node("m27_3")->y);
    TEST_ASSERT_EQUAL(26, lot52_node("m27_3")->w);
    TEST_ASSERT_EQUAL(12, lot52_node("m27_3")->h);
    TEST_ASSERT_EQUAL(29, lot52_node("m27_4")->x);
    TEST_ASSERT_EQUAL(15, lot52_node("m27_4")->y);
    TEST_ASSERT_EQUAL(26, lot52_node("m27_4")->w);
    TEST_ASSERT_EQUAL(12, lot52_node("m27_4")->h);
    TEST_ASSERT_EQUAL(58, lot52_node("m27_5")->x);
    TEST_ASSERT_EQUAL(15, lot52_node("m27_5")->y);
    TEST_ASSERT_EQUAL(26, lot52_node("m27_5")->w);
    TEST_ASSERT_EQUAL(12, lot52_node("m27_5")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("m27_6")->x);
    TEST_ASSERT_EQUAL(30, lot52_node("m27_6")->y);
    TEST_ASSERT_EQUAL(26, lot52_node("m27_6")->w);
    TEST_ASSERT_EQUAL(12, lot52_node("m27_6")->h);
    TEST_ASSERT_EQUAL(29, lot52_node("m27_7")->x);
    TEST_ASSERT_EQUAL(30, lot52_node("m27_7")->y);
    TEST_ASSERT_EQUAL(26, lot52_node("m27_7")->w);
    TEST_ASSERT_EQUAL(12, lot52_node("m27_7")->h);
    TEST_ASSERT_EQUAL(58, lot52_node("m27_8")->x);
    TEST_ASSERT_EQUAL(30, lot52_node("m27_8")->y);
    TEST_ASSERT_EQUAL(26, lot52_node("m27_8")->w);
    TEST_ASSERT_EQUAL(12, lot52_node("m27_8")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_mix_28(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:28px;grid-template-columns:repeat(2,20px);grid-template-rows:repeat(2,14px);gap:0px;align-items:stretch;justify-items:stretch'><div id=m28_0></div><div id=m28_1></div><div id=m28_2></div><div id=m28_3></div></div></body>", 480);
    TEST_ASSERT_EQUAL(2, lot52_node("g")->grid_cols);
    TEST_ASSERT_EQUAL(2, lot52_node("g")->grid_rows);
    TEST_ASSERT_EQUAL(0, lot52_node("m28_0")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("m28_0")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("m28_0")->w);
    TEST_ASSERT_EQUAL(14, lot52_node("m28_0")->h);
    TEST_ASSERT_EQUAL(20, lot52_node("m28_1")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("m28_1")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("m28_1")->w);
    TEST_ASSERT_EQUAL(14, lot52_node("m28_1")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("m28_2")->x);
    TEST_ASSERT_EQUAL(14, lot52_node("m28_2")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("m28_2")->w);
    TEST_ASSERT_EQUAL(14, lot52_node("m28_2")->h);
    TEST_ASSERT_EQUAL(20, lot52_node("m28_3")->x);
    TEST_ASSERT_EQUAL(14, lot52_node("m28_3")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("m28_3")->w);
    TEST_ASSERT_EQUAL(14, lot52_node("m28_3")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_mix_29(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:68px;height:33px;grid-template-columns:repeat(3,22px);grid-template-rows:repeat(2,16px);gap:1px;align-items:stretch;justify-items:stretch'><div id=m29_0></div><div id=m29_1></div><div id=m29_2></div><div id=m29_3></div><div id=m29_4></div><div id=m29_5></div></div></body>", 480);
    TEST_ASSERT_EQUAL(3, lot52_node("g")->grid_cols);
    TEST_ASSERT_EQUAL(2, lot52_node("g")->grid_rows);
    TEST_ASSERT_EQUAL(0, lot52_node("m29_0")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("m29_0")->y);
    TEST_ASSERT_EQUAL(22, lot52_node("m29_0")->w);
    TEST_ASSERT_EQUAL(16, lot52_node("m29_0")->h);
    TEST_ASSERT_EQUAL(23, lot52_node("m29_1")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("m29_1")->y);
    TEST_ASSERT_EQUAL(22, lot52_node("m29_1")->w);
    TEST_ASSERT_EQUAL(16, lot52_node("m29_1")->h);
    TEST_ASSERT_EQUAL(46, lot52_node("m29_2")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("m29_2")->y);
    TEST_ASSERT_EQUAL(22, lot52_node("m29_2")->w);
    TEST_ASSERT_EQUAL(16, lot52_node("m29_2")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("m29_3")->x);
    TEST_ASSERT_EQUAL(17, lot52_node("m29_3")->y);
    TEST_ASSERT_EQUAL(22, lot52_node("m29_3")->w);
    TEST_ASSERT_EQUAL(16, lot52_node("m29_3")->h);
    TEST_ASSERT_EQUAL(23, lot52_node("m29_4")->x);
    TEST_ASSERT_EQUAL(17, lot52_node("m29_4")->y);
    TEST_ASSERT_EQUAL(22, lot52_node("m29_4")->w);
    TEST_ASSERT_EQUAL(16, lot52_node("m29_4")->h);
    TEST_ASSERT_EQUAL(46, lot52_node("m29_5")->x);
    TEST_ASSERT_EQUAL(17, lot52_node("m29_5")->y);
    TEST_ASSERT_EQUAL(22, lot52_node("m29_5")->w);
    TEST_ASSERT_EQUAL(16, lot52_node("m29_5")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_mix_30(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:50px;height:40px;grid-template-columns:repeat(2,24px);grid-template-rows:repeat(3,12px);gap:2px;align-items:stretch;justify-items:stretch'><div id=m30_0></div><div id=m30_1></div><div id=m30_2></div><div id=m30_3></div><div id=m30_4></div><div id=m30_5></div></div></body>", 480);
    TEST_ASSERT_EQUAL(2, lot52_node("g")->grid_cols);
    TEST_ASSERT_EQUAL(3, lot52_node("g")->grid_rows);
    TEST_ASSERT_EQUAL(0, lot52_node("m30_0")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("m30_0")->y);
    TEST_ASSERT_EQUAL(24, lot52_node("m30_0")->w);
    TEST_ASSERT_EQUAL(12, lot52_node("m30_0")->h);
    TEST_ASSERT_EQUAL(26, lot52_node("m30_1")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("m30_1")->y);
    TEST_ASSERT_EQUAL(24, lot52_node("m30_1")->w);
    TEST_ASSERT_EQUAL(12, lot52_node("m30_1")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("m30_2")->x);
    TEST_ASSERT_EQUAL(14, lot52_node("m30_2")->y);
    TEST_ASSERT_EQUAL(24, lot52_node("m30_2")->w);
    TEST_ASSERT_EQUAL(12, lot52_node("m30_2")->h);
    TEST_ASSERT_EQUAL(26, lot52_node("m30_3")->x);
    TEST_ASSERT_EQUAL(14, lot52_node("m30_3")->y);
    TEST_ASSERT_EQUAL(24, lot52_node("m30_3")->w);
    TEST_ASSERT_EQUAL(12, lot52_node("m30_3")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("m30_4")->x);
    TEST_ASSERT_EQUAL(28, lot52_node("m30_4")->y);
    TEST_ASSERT_EQUAL(24, lot52_node("m30_4")->w);
    TEST_ASSERT_EQUAL(12, lot52_node("m30_4")->h);
    TEST_ASSERT_EQUAL(26, lot52_node("m30_5")->x);
    TEST_ASSERT_EQUAL(28, lot52_node("m30_5")->y);
    TEST_ASSERT_EQUAL(24, lot52_node("m30_5")->w);
    TEST_ASSERT_EQUAL(12, lot52_node("m30_5")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_mix_31(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:84px;height:48px;grid-template-columns:repeat(3,26px);grid-template-rows:repeat(3,14px);gap:3px;align-items:stretch;justify-items:stretch'><div id=m31_0></div><div id=m31_1></div><div id=m31_2></div><div id=m31_3></div><div id=m31_4></div><div id=m31_5></div><div id=m31_6></div><div id=m31_7></div><div id=m31_8></div></div></body>", 480);
    TEST_ASSERT_EQUAL(3, lot52_node("g")->grid_cols);
    TEST_ASSERT_EQUAL(3, lot52_node("g")->grid_rows);
    TEST_ASSERT_EQUAL(0, lot52_node("m31_0")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("m31_0")->y);
    TEST_ASSERT_EQUAL(26, lot52_node("m31_0")->w);
    TEST_ASSERT_EQUAL(14, lot52_node("m31_0")->h);
    TEST_ASSERT_EQUAL(29, lot52_node("m31_1")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("m31_1")->y);
    TEST_ASSERT_EQUAL(26, lot52_node("m31_1")->w);
    TEST_ASSERT_EQUAL(14, lot52_node("m31_1")->h);
    TEST_ASSERT_EQUAL(58, lot52_node("m31_2")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("m31_2")->y);
    TEST_ASSERT_EQUAL(26, lot52_node("m31_2")->w);
    TEST_ASSERT_EQUAL(14, lot52_node("m31_2")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("m31_3")->x);
    TEST_ASSERT_EQUAL(17, lot52_node("m31_3")->y);
    TEST_ASSERT_EQUAL(26, lot52_node("m31_3")->w);
    TEST_ASSERT_EQUAL(14, lot52_node("m31_3")->h);
    TEST_ASSERT_EQUAL(29, lot52_node("m31_4")->x);
    TEST_ASSERT_EQUAL(17, lot52_node("m31_4")->y);
    TEST_ASSERT_EQUAL(26, lot52_node("m31_4")->w);
    TEST_ASSERT_EQUAL(14, lot52_node("m31_4")->h);
    TEST_ASSERT_EQUAL(58, lot52_node("m31_5")->x);
    TEST_ASSERT_EQUAL(17, lot52_node("m31_5")->y);
    TEST_ASSERT_EQUAL(26, lot52_node("m31_5")->w);
    TEST_ASSERT_EQUAL(14, lot52_node("m31_5")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("m31_6")->x);
    TEST_ASSERT_EQUAL(34, lot52_node("m31_6")->y);
    TEST_ASSERT_EQUAL(26, lot52_node("m31_6")->w);
    TEST_ASSERT_EQUAL(14, lot52_node("m31_6")->h);
    TEST_ASSERT_EQUAL(29, lot52_node("m31_7")->x);
    TEST_ASSERT_EQUAL(34, lot52_node("m31_7")->y);
    TEST_ASSERT_EQUAL(26, lot52_node("m31_7")->w);
    TEST_ASSERT_EQUAL(14, lot52_node("m31_7")->h);
    TEST_ASSERT_EQUAL(58, lot52_node("m31_8")->x);
    TEST_ASSERT_EQUAL(34, lot52_node("m31_8")->y);
    TEST_ASSERT_EQUAL(26, lot52_node("m31_8")->w);
    TEST_ASSERT_EQUAL(14, lot52_node("m31_8")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_mix_32(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:32px;grid-template-columns:repeat(2,20px);grid-template-rows:repeat(2,16px);gap:0px;align-items:stretch;justify-items:stretch'><div id=m32_0></div><div id=m32_1></div><div id=m32_2></div><div id=m32_3></div></div></body>", 480);
    TEST_ASSERT_EQUAL(2, lot52_node("g")->grid_cols);
    TEST_ASSERT_EQUAL(2, lot52_node("g")->grid_rows);
    TEST_ASSERT_EQUAL(0, lot52_node("m32_0")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("m32_0")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("m32_0")->w);
    TEST_ASSERT_EQUAL(16, lot52_node("m32_0")->h);
    TEST_ASSERT_EQUAL(20, lot52_node("m32_1")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("m32_1")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("m32_1")->w);
    TEST_ASSERT_EQUAL(16, lot52_node("m32_1")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("m32_2")->x);
    TEST_ASSERT_EQUAL(16, lot52_node("m32_2")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("m32_2")->w);
    TEST_ASSERT_EQUAL(16, lot52_node("m32_2")->h);
    TEST_ASSERT_EQUAL(20, lot52_node("m32_3")->x);
    TEST_ASSERT_EQUAL(16, lot52_node("m32_3")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("m32_3")->w);
    TEST_ASSERT_EQUAL(16, lot52_node("m32_3")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_mix_33(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:68px;height:25px;grid-template-columns:repeat(3,22px);grid-template-rows:repeat(2,12px);gap:1px;align-items:stretch;justify-items:stretch'><div id=m33_0></div><div id=m33_1></div><div id=m33_2></div><div id=m33_3></div><div id=m33_4></div><div id=m33_5></div></div></body>", 480);
    TEST_ASSERT_EQUAL(3, lot52_node("g")->grid_cols);
    TEST_ASSERT_EQUAL(2, lot52_node("g")->grid_rows);
    TEST_ASSERT_EQUAL(0, lot52_node("m33_0")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("m33_0")->y);
    TEST_ASSERT_EQUAL(22, lot52_node("m33_0")->w);
    TEST_ASSERT_EQUAL(12, lot52_node("m33_0")->h);
    TEST_ASSERT_EQUAL(23, lot52_node("m33_1")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("m33_1")->y);
    TEST_ASSERT_EQUAL(22, lot52_node("m33_1")->w);
    TEST_ASSERT_EQUAL(12, lot52_node("m33_1")->h);
    TEST_ASSERT_EQUAL(46, lot52_node("m33_2")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("m33_2")->y);
    TEST_ASSERT_EQUAL(22, lot52_node("m33_2")->w);
    TEST_ASSERT_EQUAL(12, lot52_node("m33_2")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("m33_3")->x);
    TEST_ASSERT_EQUAL(13, lot52_node("m33_3")->y);
    TEST_ASSERT_EQUAL(22, lot52_node("m33_3")->w);
    TEST_ASSERT_EQUAL(12, lot52_node("m33_3")->h);
    TEST_ASSERT_EQUAL(23, lot52_node("m33_4")->x);
    TEST_ASSERT_EQUAL(13, lot52_node("m33_4")->y);
    TEST_ASSERT_EQUAL(22, lot52_node("m33_4")->w);
    TEST_ASSERT_EQUAL(12, lot52_node("m33_4")->h);
    TEST_ASSERT_EQUAL(46, lot52_node("m33_5")->x);
    TEST_ASSERT_EQUAL(13, lot52_node("m33_5")->y);
    TEST_ASSERT_EQUAL(22, lot52_node("m33_5")->w);
    TEST_ASSERT_EQUAL(12, lot52_node("m33_5")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_mix_34(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:50px;height:46px;grid-template-columns:repeat(2,24px);grid-template-rows:repeat(3,14px);gap:2px;align-items:stretch;justify-items:stretch'><div id=m34_0></div><div id=m34_1></div><div id=m34_2></div><div id=m34_3></div><div id=m34_4></div><div id=m34_5></div></div></body>", 480);
    TEST_ASSERT_EQUAL(2, lot52_node("g")->grid_cols);
    TEST_ASSERT_EQUAL(3, lot52_node("g")->grid_rows);
    TEST_ASSERT_EQUAL(0, lot52_node("m34_0")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("m34_0")->y);
    TEST_ASSERT_EQUAL(24, lot52_node("m34_0")->w);
    TEST_ASSERT_EQUAL(14, lot52_node("m34_0")->h);
    TEST_ASSERT_EQUAL(26, lot52_node("m34_1")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("m34_1")->y);
    TEST_ASSERT_EQUAL(24, lot52_node("m34_1")->w);
    TEST_ASSERT_EQUAL(14, lot52_node("m34_1")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("m34_2")->x);
    TEST_ASSERT_EQUAL(16, lot52_node("m34_2")->y);
    TEST_ASSERT_EQUAL(24, lot52_node("m34_2")->w);
    TEST_ASSERT_EQUAL(14, lot52_node("m34_2")->h);
    TEST_ASSERT_EQUAL(26, lot52_node("m34_3")->x);
    TEST_ASSERT_EQUAL(16, lot52_node("m34_3")->y);
    TEST_ASSERT_EQUAL(24, lot52_node("m34_3")->w);
    TEST_ASSERT_EQUAL(14, lot52_node("m34_3")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("m34_4")->x);
    TEST_ASSERT_EQUAL(32, lot52_node("m34_4")->y);
    TEST_ASSERT_EQUAL(24, lot52_node("m34_4")->w);
    TEST_ASSERT_EQUAL(14, lot52_node("m34_4")->h);
    TEST_ASSERT_EQUAL(26, lot52_node("m34_5")->x);
    TEST_ASSERT_EQUAL(32, lot52_node("m34_5")->y);
    TEST_ASSERT_EQUAL(24, lot52_node("m34_5")->w);
    TEST_ASSERT_EQUAL(14, lot52_node("m34_5")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_mix_35(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:84px;height:54px;grid-template-columns:repeat(3,26px);grid-template-rows:repeat(3,16px);gap:3px;align-items:stretch;justify-items:stretch'><div id=m35_0></div><div id=m35_1></div><div id=m35_2></div><div id=m35_3></div><div id=m35_4></div><div id=m35_5></div><div id=m35_6></div><div id=m35_7></div><div id=m35_8></div></div></body>", 480);
    TEST_ASSERT_EQUAL(3, lot52_node("g")->grid_cols);
    TEST_ASSERT_EQUAL(3, lot52_node("g")->grid_rows);
    TEST_ASSERT_EQUAL(0, lot52_node("m35_0")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("m35_0")->y);
    TEST_ASSERT_EQUAL(26, lot52_node("m35_0")->w);
    TEST_ASSERT_EQUAL(16, lot52_node("m35_0")->h);
    TEST_ASSERT_EQUAL(29, lot52_node("m35_1")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("m35_1")->y);
    TEST_ASSERT_EQUAL(26, lot52_node("m35_1")->w);
    TEST_ASSERT_EQUAL(16, lot52_node("m35_1")->h);
    TEST_ASSERT_EQUAL(58, lot52_node("m35_2")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("m35_2")->y);
    TEST_ASSERT_EQUAL(26, lot52_node("m35_2")->w);
    TEST_ASSERT_EQUAL(16, lot52_node("m35_2")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("m35_3")->x);
    TEST_ASSERT_EQUAL(19, lot52_node("m35_3")->y);
    TEST_ASSERT_EQUAL(26, lot52_node("m35_3")->w);
    TEST_ASSERT_EQUAL(16, lot52_node("m35_3")->h);
    TEST_ASSERT_EQUAL(29, lot52_node("m35_4")->x);
    TEST_ASSERT_EQUAL(19, lot52_node("m35_4")->y);
    TEST_ASSERT_EQUAL(26, lot52_node("m35_4")->w);
    TEST_ASSERT_EQUAL(16, lot52_node("m35_4")->h);
    TEST_ASSERT_EQUAL(58, lot52_node("m35_5")->x);
    TEST_ASSERT_EQUAL(19, lot52_node("m35_5")->y);
    TEST_ASSERT_EQUAL(26, lot52_node("m35_5")->w);
    TEST_ASSERT_EQUAL(16, lot52_node("m35_5")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("m35_6")->x);
    TEST_ASSERT_EQUAL(38, lot52_node("m35_6")->y);
    TEST_ASSERT_EQUAL(26, lot52_node("m35_6")->w);
    TEST_ASSERT_EQUAL(16, lot52_node("m35_6")->h);
    TEST_ASSERT_EQUAL(29, lot52_node("m35_7")->x);
    TEST_ASSERT_EQUAL(38, lot52_node("m35_7")->y);
    TEST_ASSERT_EQUAL(26, lot52_node("m35_7")->w);
    TEST_ASSERT_EQUAL(16, lot52_node("m35_7")->h);
    TEST_ASSERT_EQUAL(58, lot52_node("m35_8")->x);
    TEST_ASSERT_EQUAL(38, lot52_node("m35_8")->y);
    TEST_ASSERT_EQUAL(26, lot52_node("m35_8")->w);
    TEST_ASSERT_EQUAL(16, lot52_node("m35_8")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_mix_36(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:24px;grid-template-columns:repeat(2,20px);grid-template-rows:repeat(2,12px);gap:0px;align-items:stretch;justify-items:stretch'><div id=m36_0></div><div id=m36_1></div><div id=m36_2></div><div id=m36_3></div></div></body>", 480);
    TEST_ASSERT_EQUAL(2, lot52_node("g")->grid_cols);
    TEST_ASSERT_EQUAL(2, lot52_node("g")->grid_rows);
    TEST_ASSERT_EQUAL(0, lot52_node("m36_0")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("m36_0")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("m36_0")->w);
    TEST_ASSERT_EQUAL(12, lot52_node("m36_0")->h);
    TEST_ASSERT_EQUAL(20, lot52_node("m36_1")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("m36_1")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("m36_1")->w);
    TEST_ASSERT_EQUAL(12, lot52_node("m36_1")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("m36_2")->x);
    TEST_ASSERT_EQUAL(12, lot52_node("m36_2")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("m36_2")->w);
    TEST_ASSERT_EQUAL(12, lot52_node("m36_2")->h);
    TEST_ASSERT_EQUAL(20, lot52_node("m36_3")->x);
    TEST_ASSERT_EQUAL(12, lot52_node("m36_3")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("m36_3")->w);
    TEST_ASSERT_EQUAL(12, lot52_node("m36_3")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_mix_37(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:68px;height:29px;grid-template-columns:repeat(3,22px);grid-template-rows:repeat(2,14px);gap:1px;align-items:stretch;justify-items:stretch'><div id=m37_0></div><div id=m37_1></div><div id=m37_2></div><div id=m37_3></div><div id=m37_4></div><div id=m37_5></div></div></body>", 480);
    TEST_ASSERT_EQUAL(3, lot52_node("g")->grid_cols);
    TEST_ASSERT_EQUAL(2, lot52_node("g")->grid_rows);
    TEST_ASSERT_EQUAL(0, lot52_node("m37_0")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("m37_0")->y);
    TEST_ASSERT_EQUAL(22, lot52_node("m37_0")->w);
    TEST_ASSERT_EQUAL(14, lot52_node("m37_0")->h);
    TEST_ASSERT_EQUAL(23, lot52_node("m37_1")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("m37_1")->y);
    TEST_ASSERT_EQUAL(22, lot52_node("m37_1")->w);
    TEST_ASSERT_EQUAL(14, lot52_node("m37_1")->h);
    TEST_ASSERT_EQUAL(46, lot52_node("m37_2")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("m37_2")->y);
    TEST_ASSERT_EQUAL(22, lot52_node("m37_2")->w);
    TEST_ASSERT_EQUAL(14, lot52_node("m37_2")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("m37_3")->x);
    TEST_ASSERT_EQUAL(15, lot52_node("m37_3")->y);
    TEST_ASSERT_EQUAL(22, lot52_node("m37_3")->w);
    TEST_ASSERT_EQUAL(14, lot52_node("m37_3")->h);
    TEST_ASSERT_EQUAL(23, lot52_node("m37_4")->x);
    TEST_ASSERT_EQUAL(15, lot52_node("m37_4")->y);
    TEST_ASSERT_EQUAL(22, lot52_node("m37_4")->w);
    TEST_ASSERT_EQUAL(14, lot52_node("m37_4")->h);
    TEST_ASSERT_EQUAL(46, lot52_node("m37_5")->x);
    TEST_ASSERT_EQUAL(15, lot52_node("m37_5")->y);
    TEST_ASSERT_EQUAL(22, lot52_node("m37_5")->w);
    TEST_ASSERT_EQUAL(14, lot52_node("m37_5")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_mix_38(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:50px;height:52px;grid-template-columns:repeat(2,24px);grid-template-rows:repeat(3,16px);gap:2px;align-items:stretch;justify-items:stretch'><div id=m38_0></div><div id=m38_1></div><div id=m38_2></div><div id=m38_3></div><div id=m38_4></div><div id=m38_5></div></div></body>", 480);
    TEST_ASSERT_EQUAL(2, lot52_node("g")->grid_cols);
    TEST_ASSERT_EQUAL(3, lot52_node("g")->grid_rows);
    TEST_ASSERT_EQUAL(0, lot52_node("m38_0")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("m38_0")->y);
    TEST_ASSERT_EQUAL(24, lot52_node("m38_0")->w);
    TEST_ASSERT_EQUAL(16, lot52_node("m38_0")->h);
    TEST_ASSERT_EQUAL(26, lot52_node("m38_1")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("m38_1")->y);
    TEST_ASSERT_EQUAL(24, lot52_node("m38_1")->w);
    TEST_ASSERT_EQUAL(16, lot52_node("m38_1")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("m38_2")->x);
    TEST_ASSERT_EQUAL(18, lot52_node("m38_2")->y);
    TEST_ASSERT_EQUAL(24, lot52_node("m38_2")->w);
    TEST_ASSERT_EQUAL(16, lot52_node("m38_2")->h);
    TEST_ASSERT_EQUAL(26, lot52_node("m38_3")->x);
    TEST_ASSERT_EQUAL(18, lot52_node("m38_3")->y);
    TEST_ASSERT_EQUAL(24, lot52_node("m38_3")->w);
    TEST_ASSERT_EQUAL(16, lot52_node("m38_3")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("m38_4")->x);
    TEST_ASSERT_EQUAL(36, lot52_node("m38_4")->y);
    TEST_ASSERT_EQUAL(24, lot52_node("m38_4")->w);
    TEST_ASSERT_EQUAL(16, lot52_node("m38_4")->h);
    TEST_ASSERT_EQUAL(26, lot52_node("m38_5")->x);
    TEST_ASSERT_EQUAL(36, lot52_node("m38_5")->y);
    TEST_ASSERT_EQUAL(24, lot52_node("m38_5")->w);
    TEST_ASSERT_EQUAL(16, lot52_node("m38_5")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_mix_39(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:84px;height:42px;grid-template-columns:repeat(3,26px);grid-template-rows:repeat(3,12px);gap:3px;align-items:stretch;justify-items:stretch'><div id=m39_0></div><div id=m39_1></div><div id=m39_2></div><div id=m39_3></div><div id=m39_4></div><div id=m39_5></div><div id=m39_6></div><div id=m39_7></div><div id=m39_8></div></div></body>", 480);
    TEST_ASSERT_EQUAL(3, lot52_node("g")->grid_cols);
    TEST_ASSERT_EQUAL(3, lot52_node("g")->grid_rows);
    TEST_ASSERT_EQUAL(0, lot52_node("m39_0")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("m39_0")->y);
    TEST_ASSERT_EQUAL(26, lot52_node("m39_0")->w);
    TEST_ASSERT_EQUAL(12, lot52_node("m39_0")->h);
    TEST_ASSERT_EQUAL(29, lot52_node("m39_1")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("m39_1")->y);
    TEST_ASSERT_EQUAL(26, lot52_node("m39_1")->w);
    TEST_ASSERT_EQUAL(12, lot52_node("m39_1")->h);
    TEST_ASSERT_EQUAL(58, lot52_node("m39_2")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("m39_2")->y);
    TEST_ASSERT_EQUAL(26, lot52_node("m39_2")->w);
    TEST_ASSERT_EQUAL(12, lot52_node("m39_2")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("m39_3")->x);
    TEST_ASSERT_EQUAL(15, lot52_node("m39_3")->y);
    TEST_ASSERT_EQUAL(26, lot52_node("m39_3")->w);
    TEST_ASSERT_EQUAL(12, lot52_node("m39_3")->h);
    TEST_ASSERT_EQUAL(29, lot52_node("m39_4")->x);
    TEST_ASSERT_EQUAL(15, lot52_node("m39_4")->y);
    TEST_ASSERT_EQUAL(26, lot52_node("m39_4")->w);
    TEST_ASSERT_EQUAL(12, lot52_node("m39_4")->h);
    TEST_ASSERT_EQUAL(58, lot52_node("m39_5")->x);
    TEST_ASSERT_EQUAL(15, lot52_node("m39_5")->y);
    TEST_ASSERT_EQUAL(26, lot52_node("m39_5")->w);
    TEST_ASSERT_EQUAL(12, lot52_node("m39_5")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("m39_6")->x);
    TEST_ASSERT_EQUAL(30, lot52_node("m39_6")->y);
    TEST_ASSERT_EQUAL(26, lot52_node("m39_6")->w);
    TEST_ASSERT_EQUAL(12, lot52_node("m39_6")->h);
    TEST_ASSERT_EQUAL(29, lot52_node("m39_7")->x);
    TEST_ASSERT_EQUAL(30, lot52_node("m39_7")->y);
    TEST_ASSERT_EQUAL(26, lot52_node("m39_7")->w);
    TEST_ASSERT_EQUAL(12, lot52_node("m39_7")->h);
    TEST_ASSERT_EQUAL(58, lot52_node("m39_8")->x);
    TEST_ASSERT_EQUAL(30, lot52_node("m39_8")->y);
    TEST_ASSERT_EQUAL(26, lot52_node("m39_8")->w);
    TEST_ASSERT_EQUAL(12, lot52_node("m39_8")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_mix_40(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:28px;grid-template-columns:repeat(2,20px);grid-template-rows:repeat(2,14px);gap:0px;align-items:stretch;justify-items:stretch'><div id=m40_0></div><div id=m40_1></div><div id=m40_2></div><div id=m40_3></div></div></body>", 480);
    TEST_ASSERT_EQUAL(2, lot52_node("g")->grid_cols);
    TEST_ASSERT_EQUAL(2, lot52_node("g")->grid_rows);
    TEST_ASSERT_EQUAL(0, lot52_node("m40_0")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("m40_0")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("m40_0")->w);
    TEST_ASSERT_EQUAL(14, lot52_node("m40_0")->h);
    TEST_ASSERT_EQUAL(20, lot52_node("m40_1")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("m40_1")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("m40_1")->w);
    TEST_ASSERT_EQUAL(14, lot52_node("m40_1")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("m40_2")->x);
    TEST_ASSERT_EQUAL(14, lot52_node("m40_2")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("m40_2")->w);
    TEST_ASSERT_EQUAL(14, lot52_node("m40_2")->h);
    TEST_ASSERT_EQUAL(20, lot52_node("m40_3")->x);
    TEST_ASSERT_EQUAL(14, lot52_node("m40_3")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("m40_3")->w);
    TEST_ASSERT_EQUAL(14, lot52_node("m40_3")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_mix_41(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:68px;height:33px;grid-template-columns:repeat(3,22px);grid-template-rows:repeat(2,16px);gap:1px;align-items:stretch;justify-items:stretch'><div id=m41_0></div><div id=m41_1></div><div id=m41_2></div><div id=m41_3></div><div id=m41_4></div><div id=m41_5></div></div></body>", 480);
    TEST_ASSERT_EQUAL(3, lot52_node("g")->grid_cols);
    TEST_ASSERT_EQUAL(2, lot52_node("g")->grid_rows);
    TEST_ASSERT_EQUAL(0, lot52_node("m41_0")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("m41_0")->y);
    TEST_ASSERT_EQUAL(22, lot52_node("m41_0")->w);
    TEST_ASSERT_EQUAL(16, lot52_node("m41_0")->h);
    TEST_ASSERT_EQUAL(23, lot52_node("m41_1")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("m41_1")->y);
    TEST_ASSERT_EQUAL(22, lot52_node("m41_1")->w);
    TEST_ASSERT_EQUAL(16, lot52_node("m41_1")->h);
    TEST_ASSERT_EQUAL(46, lot52_node("m41_2")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("m41_2")->y);
    TEST_ASSERT_EQUAL(22, lot52_node("m41_2")->w);
    TEST_ASSERT_EQUAL(16, lot52_node("m41_2")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("m41_3")->x);
    TEST_ASSERT_EQUAL(17, lot52_node("m41_3")->y);
    TEST_ASSERT_EQUAL(22, lot52_node("m41_3")->w);
    TEST_ASSERT_EQUAL(16, lot52_node("m41_3")->h);
    TEST_ASSERT_EQUAL(23, lot52_node("m41_4")->x);
    TEST_ASSERT_EQUAL(17, lot52_node("m41_4")->y);
    TEST_ASSERT_EQUAL(22, lot52_node("m41_4")->w);
    TEST_ASSERT_EQUAL(16, lot52_node("m41_4")->h);
    TEST_ASSERT_EQUAL(46, lot52_node("m41_5")->x);
    TEST_ASSERT_EQUAL(17, lot52_node("m41_5")->y);
    TEST_ASSERT_EQUAL(22, lot52_node("m41_5")->w);
    TEST_ASSERT_EQUAL(16, lot52_node("m41_5")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_mix_42(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:50px;height:40px;grid-template-columns:repeat(2,24px);grid-template-rows:repeat(3,12px);gap:2px;align-items:stretch;justify-items:stretch'><div id=m42_0></div><div id=m42_1></div><div id=m42_2></div><div id=m42_3></div><div id=m42_4></div><div id=m42_5></div></div></body>", 480);
    TEST_ASSERT_EQUAL(2, lot52_node("g")->grid_cols);
    TEST_ASSERT_EQUAL(3, lot52_node("g")->grid_rows);
    TEST_ASSERT_EQUAL(0, lot52_node("m42_0")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("m42_0")->y);
    TEST_ASSERT_EQUAL(24, lot52_node("m42_0")->w);
    TEST_ASSERT_EQUAL(12, lot52_node("m42_0")->h);
    TEST_ASSERT_EQUAL(26, lot52_node("m42_1")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("m42_1")->y);
    TEST_ASSERT_EQUAL(24, lot52_node("m42_1")->w);
    TEST_ASSERT_EQUAL(12, lot52_node("m42_1")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("m42_2")->x);
    TEST_ASSERT_EQUAL(14, lot52_node("m42_2")->y);
    TEST_ASSERT_EQUAL(24, lot52_node("m42_2")->w);
    TEST_ASSERT_EQUAL(12, lot52_node("m42_2")->h);
    TEST_ASSERT_EQUAL(26, lot52_node("m42_3")->x);
    TEST_ASSERT_EQUAL(14, lot52_node("m42_3")->y);
    TEST_ASSERT_EQUAL(24, lot52_node("m42_3")->w);
    TEST_ASSERT_EQUAL(12, lot52_node("m42_3")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("m42_4")->x);
    TEST_ASSERT_EQUAL(28, lot52_node("m42_4")->y);
    TEST_ASSERT_EQUAL(24, lot52_node("m42_4")->w);
    TEST_ASSERT_EQUAL(12, lot52_node("m42_4")->h);
    TEST_ASSERT_EQUAL(26, lot52_node("m42_5")->x);
    TEST_ASSERT_EQUAL(28, lot52_node("m42_5")->y);
    TEST_ASSERT_EQUAL(24, lot52_node("m42_5")->w);
    TEST_ASSERT_EQUAL(12, lot52_node("m42_5")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_mix_43(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:84px;height:48px;grid-template-columns:repeat(3,26px);grid-template-rows:repeat(3,14px);gap:3px;align-items:stretch;justify-items:stretch'><div id=m43_0></div><div id=m43_1></div><div id=m43_2></div><div id=m43_3></div><div id=m43_4></div><div id=m43_5></div><div id=m43_6></div><div id=m43_7></div><div id=m43_8></div></div></body>", 480);
    TEST_ASSERT_EQUAL(3, lot52_node("g")->grid_cols);
    TEST_ASSERT_EQUAL(3, lot52_node("g")->grid_rows);
    TEST_ASSERT_EQUAL(0, lot52_node("m43_0")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("m43_0")->y);
    TEST_ASSERT_EQUAL(26, lot52_node("m43_0")->w);
    TEST_ASSERT_EQUAL(14, lot52_node("m43_0")->h);
    TEST_ASSERT_EQUAL(29, lot52_node("m43_1")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("m43_1")->y);
    TEST_ASSERT_EQUAL(26, lot52_node("m43_1")->w);
    TEST_ASSERT_EQUAL(14, lot52_node("m43_1")->h);
    TEST_ASSERT_EQUAL(58, lot52_node("m43_2")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("m43_2")->y);
    TEST_ASSERT_EQUAL(26, lot52_node("m43_2")->w);
    TEST_ASSERT_EQUAL(14, lot52_node("m43_2")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("m43_3")->x);
    TEST_ASSERT_EQUAL(17, lot52_node("m43_3")->y);
    TEST_ASSERT_EQUAL(26, lot52_node("m43_3")->w);
    TEST_ASSERT_EQUAL(14, lot52_node("m43_3")->h);
    TEST_ASSERT_EQUAL(29, lot52_node("m43_4")->x);
    TEST_ASSERT_EQUAL(17, lot52_node("m43_4")->y);
    TEST_ASSERT_EQUAL(26, lot52_node("m43_4")->w);
    TEST_ASSERT_EQUAL(14, lot52_node("m43_4")->h);
    TEST_ASSERT_EQUAL(58, lot52_node("m43_5")->x);
    TEST_ASSERT_EQUAL(17, lot52_node("m43_5")->y);
    TEST_ASSERT_EQUAL(26, lot52_node("m43_5")->w);
    TEST_ASSERT_EQUAL(14, lot52_node("m43_5")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("m43_6")->x);
    TEST_ASSERT_EQUAL(34, lot52_node("m43_6")->y);
    TEST_ASSERT_EQUAL(26, lot52_node("m43_6")->w);
    TEST_ASSERT_EQUAL(14, lot52_node("m43_6")->h);
    TEST_ASSERT_EQUAL(29, lot52_node("m43_7")->x);
    TEST_ASSERT_EQUAL(34, lot52_node("m43_7")->y);
    TEST_ASSERT_EQUAL(26, lot52_node("m43_7")->w);
    TEST_ASSERT_EQUAL(14, lot52_node("m43_7")->h);
    TEST_ASSERT_EQUAL(58, lot52_node("m43_8")->x);
    TEST_ASSERT_EQUAL(34, lot52_node("m43_8")->y);
    TEST_ASSERT_EQUAL(26, lot52_node("m43_8")->w);
    TEST_ASSERT_EQUAL(14, lot52_node("m43_8")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_mix_44(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:32px;grid-template-columns:repeat(2,20px);grid-template-rows:repeat(2,16px);gap:0px;align-items:stretch;justify-items:stretch'><div id=m44_0></div><div id=m44_1></div><div id=m44_2></div><div id=m44_3></div></div></body>", 480);
    TEST_ASSERT_EQUAL(2, lot52_node("g")->grid_cols);
    TEST_ASSERT_EQUAL(2, lot52_node("g")->grid_rows);
    TEST_ASSERT_EQUAL(0, lot52_node("m44_0")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("m44_0")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("m44_0")->w);
    TEST_ASSERT_EQUAL(16, lot52_node("m44_0")->h);
    TEST_ASSERT_EQUAL(20, lot52_node("m44_1")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("m44_1")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("m44_1")->w);
    TEST_ASSERT_EQUAL(16, lot52_node("m44_1")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("m44_2")->x);
    TEST_ASSERT_EQUAL(16, lot52_node("m44_2")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("m44_2")->w);
    TEST_ASSERT_EQUAL(16, lot52_node("m44_2")->h);
    TEST_ASSERT_EQUAL(20, lot52_node("m44_3")->x);
    TEST_ASSERT_EQUAL(16, lot52_node("m44_3")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("m44_3")->w);
    TEST_ASSERT_EQUAL(16, lot52_node("m44_3")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_mix_45(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:68px;height:25px;grid-template-columns:repeat(3,22px);grid-template-rows:repeat(2,12px);gap:1px;align-items:stretch;justify-items:stretch'><div id=m45_0></div><div id=m45_1></div><div id=m45_2></div><div id=m45_3></div><div id=m45_4></div><div id=m45_5></div></div></body>", 480);
    TEST_ASSERT_EQUAL(3, lot52_node("g")->grid_cols);
    TEST_ASSERT_EQUAL(2, lot52_node("g")->grid_rows);
    TEST_ASSERT_EQUAL(0, lot52_node("m45_0")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("m45_0")->y);
    TEST_ASSERT_EQUAL(22, lot52_node("m45_0")->w);
    TEST_ASSERT_EQUAL(12, lot52_node("m45_0")->h);
    TEST_ASSERT_EQUAL(23, lot52_node("m45_1")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("m45_1")->y);
    TEST_ASSERT_EQUAL(22, lot52_node("m45_1")->w);
    TEST_ASSERT_EQUAL(12, lot52_node("m45_1")->h);
    TEST_ASSERT_EQUAL(46, lot52_node("m45_2")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("m45_2")->y);
    TEST_ASSERT_EQUAL(22, lot52_node("m45_2")->w);
    TEST_ASSERT_EQUAL(12, lot52_node("m45_2")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("m45_3")->x);
    TEST_ASSERT_EQUAL(13, lot52_node("m45_3")->y);
    TEST_ASSERT_EQUAL(22, lot52_node("m45_3")->w);
    TEST_ASSERT_EQUAL(12, lot52_node("m45_3")->h);
    TEST_ASSERT_EQUAL(23, lot52_node("m45_4")->x);
    TEST_ASSERT_EQUAL(13, lot52_node("m45_4")->y);
    TEST_ASSERT_EQUAL(22, lot52_node("m45_4")->w);
    TEST_ASSERT_EQUAL(12, lot52_node("m45_4")->h);
    TEST_ASSERT_EQUAL(46, lot52_node("m45_5")->x);
    TEST_ASSERT_EQUAL(13, lot52_node("m45_5")->y);
    TEST_ASSERT_EQUAL(22, lot52_node("m45_5")->w);
    TEST_ASSERT_EQUAL(12, lot52_node("m45_5")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_mix_46(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:50px;height:46px;grid-template-columns:repeat(2,24px);grid-template-rows:repeat(3,14px);gap:2px;align-items:stretch;justify-items:stretch'><div id=m46_0></div><div id=m46_1></div><div id=m46_2></div><div id=m46_3></div><div id=m46_4></div><div id=m46_5></div></div></body>", 480);
    TEST_ASSERT_EQUAL(2, lot52_node("g")->grid_cols);
    TEST_ASSERT_EQUAL(3, lot52_node("g")->grid_rows);
    TEST_ASSERT_EQUAL(0, lot52_node("m46_0")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("m46_0")->y);
    TEST_ASSERT_EQUAL(24, lot52_node("m46_0")->w);
    TEST_ASSERT_EQUAL(14, lot52_node("m46_0")->h);
    TEST_ASSERT_EQUAL(26, lot52_node("m46_1")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("m46_1")->y);
    TEST_ASSERT_EQUAL(24, lot52_node("m46_1")->w);
    TEST_ASSERT_EQUAL(14, lot52_node("m46_1")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("m46_2")->x);
    TEST_ASSERT_EQUAL(16, lot52_node("m46_2")->y);
    TEST_ASSERT_EQUAL(24, lot52_node("m46_2")->w);
    TEST_ASSERT_EQUAL(14, lot52_node("m46_2")->h);
    TEST_ASSERT_EQUAL(26, lot52_node("m46_3")->x);
    TEST_ASSERT_EQUAL(16, lot52_node("m46_3")->y);
    TEST_ASSERT_EQUAL(24, lot52_node("m46_3")->w);
    TEST_ASSERT_EQUAL(14, lot52_node("m46_3")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("m46_4")->x);
    TEST_ASSERT_EQUAL(32, lot52_node("m46_4")->y);
    TEST_ASSERT_EQUAL(24, lot52_node("m46_4")->w);
    TEST_ASSERT_EQUAL(14, lot52_node("m46_4")->h);
    TEST_ASSERT_EQUAL(26, lot52_node("m46_5")->x);
    TEST_ASSERT_EQUAL(32, lot52_node("m46_5")->y);
    TEST_ASSERT_EQUAL(24, lot52_node("m46_5")->w);
    TEST_ASSERT_EQUAL(14, lot52_node("m46_5")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_mix_47(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:84px;height:54px;grid-template-columns:repeat(3,26px);grid-template-rows:repeat(3,16px);gap:3px;align-items:stretch;justify-items:stretch'><div id=m47_0></div><div id=m47_1></div><div id=m47_2></div><div id=m47_3></div><div id=m47_4></div><div id=m47_5></div><div id=m47_6></div><div id=m47_7></div><div id=m47_8></div></div></body>", 480);
    TEST_ASSERT_EQUAL(3, lot52_node("g")->grid_cols);
    TEST_ASSERT_EQUAL(3, lot52_node("g")->grid_rows);
    TEST_ASSERT_EQUAL(0, lot52_node("m47_0")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("m47_0")->y);
    TEST_ASSERT_EQUAL(26, lot52_node("m47_0")->w);
    TEST_ASSERT_EQUAL(16, lot52_node("m47_0")->h);
    TEST_ASSERT_EQUAL(29, lot52_node("m47_1")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("m47_1")->y);
    TEST_ASSERT_EQUAL(26, lot52_node("m47_1")->w);
    TEST_ASSERT_EQUAL(16, lot52_node("m47_1")->h);
    TEST_ASSERT_EQUAL(58, lot52_node("m47_2")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("m47_2")->y);
    TEST_ASSERT_EQUAL(26, lot52_node("m47_2")->w);
    TEST_ASSERT_EQUAL(16, lot52_node("m47_2")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("m47_3")->x);
    TEST_ASSERT_EQUAL(19, lot52_node("m47_3")->y);
    TEST_ASSERT_EQUAL(26, lot52_node("m47_3")->w);
    TEST_ASSERT_EQUAL(16, lot52_node("m47_3")->h);
    TEST_ASSERT_EQUAL(29, lot52_node("m47_4")->x);
    TEST_ASSERT_EQUAL(19, lot52_node("m47_4")->y);
    TEST_ASSERT_EQUAL(26, lot52_node("m47_4")->w);
    TEST_ASSERT_EQUAL(16, lot52_node("m47_4")->h);
    TEST_ASSERT_EQUAL(58, lot52_node("m47_5")->x);
    TEST_ASSERT_EQUAL(19, lot52_node("m47_5")->y);
    TEST_ASSERT_EQUAL(26, lot52_node("m47_5")->w);
    TEST_ASSERT_EQUAL(16, lot52_node("m47_5")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("m47_6")->x);
    TEST_ASSERT_EQUAL(38, lot52_node("m47_6")->y);
    TEST_ASSERT_EQUAL(26, lot52_node("m47_6")->w);
    TEST_ASSERT_EQUAL(16, lot52_node("m47_6")->h);
    TEST_ASSERT_EQUAL(29, lot52_node("m47_7")->x);
    TEST_ASSERT_EQUAL(38, lot52_node("m47_7")->y);
    TEST_ASSERT_EQUAL(26, lot52_node("m47_7")->w);
    TEST_ASSERT_EQUAL(16, lot52_node("m47_7")->h);
    TEST_ASSERT_EQUAL(58, lot52_node("m47_8")->x);
    TEST_ASSERT_EQUAL(38, lot52_node("m47_8")->y);
    TEST_ASSERT_EQUAL(26, lot52_node("m47_8")->w);
    TEST_ASSERT_EQUAL(16, lot52_node("m47_8")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_mix_48(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:24px;grid-template-columns:repeat(2,20px);grid-template-rows:repeat(2,12px);gap:0px;align-items:stretch;justify-items:stretch'><div id=m48_0></div><div id=m48_1></div><div id=m48_2></div><div id=m48_3></div></div></body>", 480);
    TEST_ASSERT_EQUAL(2, lot52_node("g")->grid_cols);
    TEST_ASSERT_EQUAL(2, lot52_node("g")->grid_rows);
    TEST_ASSERT_EQUAL(0, lot52_node("m48_0")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("m48_0")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("m48_0")->w);
    TEST_ASSERT_EQUAL(12, lot52_node("m48_0")->h);
    TEST_ASSERT_EQUAL(20, lot52_node("m48_1")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("m48_1")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("m48_1")->w);
    TEST_ASSERT_EQUAL(12, lot52_node("m48_1")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("m48_2")->x);
    TEST_ASSERT_EQUAL(12, lot52_node("m48_2")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("m48_2")->w);
    TEST_ASSERT_EQUAL(12, lot52_node("m48_2")->h);
    TEST_ASSERT_EQUAL(20, lot52_node("m48_3")->x);
    TEST_ASSERT_EQUAL(12, lot52_node("m48_3")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("m48_3")->w);
    TEST_ASSERT_EQUAL(12, lot52_node("m48_3")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_mix_49(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:68px;height:29px;grid-template-columns:repeat(3,22px);grid-template-rows:repeat(2,14px);gap:1px;align-items:stretch;justify-items:stretch'><div id=m49_0></div><div id=m49_1></div><div id=m49_2></div><div id=m49_3></div><div id=m49_4></div><div id=m49_5></div></div></body>", 480);
    TEST_ASSERT_EQUAL(3, lot52_node("g")->grid_cols);
    TEST_ASSERT_EQUAL(2, lot52_node("g")->grid_rows);
    TEST_ASSERT_EQUAL(0, lot52_node("m49_0")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("m49_0")->y);
    TEST_ASSERT_EQUAL(22, lot52_node("m49_0")->w);
    TEST_ASSERT_EQUAL(14, lot52_node("m49_0")->h);
    TEST_ASSERT_EQUAL(23, lot52_node("m49_1")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("m49_1")->y);
    TEST_ASSERT_EQUAL(22, lot52_node("m49_1")->w);
    TEST_ASSERT_EQUAL(14, lot52_node("m49_1")->h);
    TEST_ASSERT_EQUAL(46, lot52_node("m49_2")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("m49_2")->y);
    TEST_ASSERT_EQUAL(22, lot52_node("m49_2")->w);
    TEST_ASSERT_EQUAL(14, lot52_node("m49_2")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("m49_3")->x);
    TEST_ASSERT_EQUAL(15, lot52_node("m49_3")->y);
    TEST_ASSERT_EQUAL(22, lot52_node("m49_3")->w);
    TEST_ASSERT_EQUAL(14, lot52_node("m49_3")->h);
    TEST_ASSERT_EQUAL(23, lot52_node("m49_4")->x);
    TEST_ASSERT_EQUAL(15, lot52_node("m49_4")->y);
    TEST_ASSERT_EQUAL(22, lot52_node("m49_4")->w);
    TEST_ASSERT_EQUAL(14, lot52_node("m49_4")->h);
    TEST_ASSERT_EQUAL(46, lot52_node("m49_5")->x);
    TEST_ASSERT_EQUAL(15, lot52_node("m49_5")->y);
    TEST_ASSERT_EQUAL(22, lot52_node("m49_5")->w);
    TEST_ASSERT_EQUAL(14, lot52_node("m49_5")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_mix_50(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:50px;height:52px;grid-template-columns:repeat(2,24px);grid-template-rows:repeat(3,16px);gap:2px;align-items:stretch;justify-items:stretch'><div id=m50_0></div><div id=m50_1></div><div id=m50_2></div><div id=m50_3></div><div id=m50_4></div><div id=m50_5></div></div></body>", 480);
    TEST_ASSERT_EQUAL(2, lot52_node("g")->grid_cols);
    TEST_ASSERT_EQUAL(3, lot52_node("g")->grid_rows);
    TEST_ASSERT_EQUAL(0, lot52_node("m50_0")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("m50_0")->y);
    TEST_ASSERT_EQUAL(24, lot52_node("m50_0")->w);
    TEST_ASSERT_EQUAL(16, lot52_node("m50_0")->h);
    TEST_ASSERT_EQUAL(26, lot52_node("m50_1")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("m50_1")->y);
    TEST_ASSERT_EQUAL(24, lot52_node("m50_1")->w);
    TEST_ASSERT_EQUAL(16, lot52_node("m50_1")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("m50_2")->x);
    TEST_ASSERT_EQUAL(18, lot52_node("m50_2")->y);
    TEST_ASSERT_EQUAL(24, lot52_node("m50_2")->w);
    TEST_ASSERT_EQUAL(16, lot52_node("m50_2")->h);
    TEST_ASSERT_EQUAL(26, lot52_node("m50_3")->x);
    TEST_ASSERT_EQUAL(18, lot52_node("m50_3")->y);
    TEST_ASSERT_EQUAL(24, lot52_node("m50_3")->w);
    TEST_ASSERT_EQUAL(16, lot52_node("m50_3")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("m50_4")->x);
    TEST_ASSERT_EQUAL(36, lot52_node("m50_4")->y);
    TEST_ASSERT_EQUAL(24, lot52_node("m50_4")->w);
    TEST_ASSERT_EQUAL(16, lot52_node("m50_4")->h);
    TEST_ASSERT_EQUAL(26, lot52_node("m50_5")->x);
    TEST_ASSERT_EQUAL(36, lot52_node("m50_5")->y);
    TEST_ASSERT_EQUAL(24, lot52_node("m50_5")->w);
    TEST_ASSERT_EQUAL(16, lot52_node("m50_5")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_mix_51(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:84px;height:42px;grid-template-columns:repeat(3,26px);grid-template-rows:repeat(3,12px);gap:3px;align-items:stretch;justify-items:stretch'><div id=m51_0></div><div id=m51_1></div><div id=m51_2></div><div id=m51_3></div><div id=m51_4></div><div id=m51_5></div><div id=m51_6></div><div id=m51_7></div><div id=m51_8></div></div></body>", 480);
    TEST_ASSERT_EQUAL(3, lot52_node("g")->grid_cols);
    TEST_ASSERT_EQUAL(3, lot52_node("g")->grid_rows);
    TEST_ASSERT_EQUAL(0, lot52_node("m51_0")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("m51_0")->y);
    TEST_ASSERT_EQUAL(26, lot52_node("m51_0")->w);
    TEST_ASSERT_EQUAL(12, lot52_node("m51_0")->h);
    TEST_ASSERT_EQUAL(29, lot52_node("m51_1")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("m51_1")->y);
    TEST_ASSERT_EQUAL(26, lot52_node("m51_1")->w);
    TEST_ASSERT_EQUAL(12, lot52_node("m51_1")->h);
    TEST_ASSERT_EQUAL(58, lot52_node("m51_2")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("m51_2")->y);
    TEST_ASSERT_EQUAL(26, lot52_node("m51_2")->w);
    TEST_ASSERT_EQUAL(12, lot52_node("m51_2")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("m51_3")->x);
    TEST_ASSERT_EQUAL(15, lot52_node("m51_3")->y);
    TEST_ASSERT_EQUAL(26, lot52_node("m51_3")->w);
    TEST_ASSERT_EQUAL(12, lot52_node("m51_3")->h);
    TEST_ASSERT_EQUAL(29, lot52_node("m51_4")->x);
    TEST_ASSERT_EQUAL(15, lot52_node("m51_4")->y);
    TEST_ASSERT_EQUAL(26, lot52_node("m51_4")->w);
    TEST_ASSERT_EQUAL(12, lot52_node("m51_4")->h);
    TEST_ASSERT_EQUAL(58, lot52_node("m51_5")->x);
    TEST_ASSERT_EQUAL(15, lot52_node("m51_5")->y);
    TEST_ASSERT_EQUAL(26, lot52_node("m51_5")->w);
    TEST_ASSERT_EQUAL(12, lot52_node("m51_5")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("m51_6")->x);
    TEST_ASSERT_EQUAL(30, lot52_node("m51_6")->y);
    TEST_ASSERT_EQUAL(26, lot52_node("m51_6")->w);
    TEST_ASSERT_EQUAL(12, lot52_node("m51_6")->h);
    TEST_ASSERT_EQUAL(29, lot52_node("m51_7")->x);
    TEST_ASSERT_EQUAL(30, lot52_node("m51_7")->y);
    TEST_ASSERT_EQUAL(26, lot52_node("m51_7")->w);
    TEST_ASSERT_EQUAL(12, lot52_node("m51_7")->h);
    TEST_ASSERT_EQUAL(58, lot52_node("m51_8")->x);
    TEST_ASSERT_EQUAL(30, lot52_node("m51_8")->y);
    TEST_ASSERT_EQUAL(26, lot52_node("m51_8")->w);
    TEST_ASSERT_EQUAL(12, lot52_node("m51_8")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_mix_52(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:28px;grid-template-columns:repeat(2,20px);grid-template-rows:repeat(2,14px);gap:0px;align-items:stretch;justify-items:stretch'><div id=m52_0></div><div id=m52_1></div><div id=m52_2></div><div id=m52_3></div></div></body>", 480);
    TEST_ASSERT_EQUAL(2, lot52_node("g")->grid_cols);
    TEST_ASSERT_EQUAL(2, lot52_node("g")->grid_rows);
    TEST_ASSERT_EQUAL(0, lot52_node("m52_0")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("m52_0")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("m52_0")->w);
    TEST_ASSERT_EQUAL(14, lot52_node("m52_0")->h);
    TEST_ASSERT_EQUAL(20, lot52_node("m52_1")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("m52_1")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("m52_1")->w);
    TEST_ASSERT_EQUAL(14, lot52_node("m52_1")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("m52_2")->x);
    TEST_ASSERT_EQUAL(14, lot52_node("m52_2")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("m52_2")->w);
    TEST_ASSERT_EQUAL(14, lot52_node("m52_2")->h);
    TEST_ASSERT_EQUAL(20, lot52_node("m52_3")->x);
    TEST_ASSERT_EQUAL(14, lot52_node("m52_3")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("m52_3")->w);
    TEST_ASSERT_EQUAL(14, lot52_node("m52_3")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_mix_53(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:68px;height:33px;grid-template-columns:repeat(3,22px);grid-template-rows:repeat(2,16px);gap:1px;align-items:stretch;justify-items:stretch'><div id=m53_0></div><div id=m53_1></div><div id=m53_2></div><div id=m53_3></div><div id=m53_4></div><div id=m53_5></div></div></body>", 480);
    TEST_ASSERT_EQUAL(3, lot52_node("g")->grid_cols);
    TEST_ASSERT_EQUAL(2, lot52_node("g")->grid_rows);
    TEST_ASSERT_EQUAL(0, lot52_node("m53_0")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("m53_0")->y);
    TEST_ASSERT_EQUAL(22, lot52_node("m53_0")->w);
    TEST_ASSERT_EQUAL(16, lot52_node("m53_0")->h);
    TEST_ASSERT_EQUAL(23, lot52_node("m53_1")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("m53_1")->y);
    TEST_ASSERT_EQUAL(22, lot52_node("m53_1")->w);
    TEST_ASSERT_EQUAL(16, lot52_node("m53_1")->h);
    TEST_ASSERT_EQUAL(46, lot52_node("m53_2")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("m53_2")->y);
    TEST_ASSERT_EQUAL(22, lot52_node("m53_2")->w);
    TEST_ASSERT_EQUAL(16, lot52_node("m53_2")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("m53_3")->x);
    TEST_ASSERT_EQUAL(17, lot52_node("m53_3")->y);
    TEST_ASSERT_EQUAL(22, lot52_node("m53_3")->w);
    TEST_ASSERT_EQUAL(16, lot52_node("m53_3")->h);
    TEST_ASSERT_EQUAL(23, lot52_node("m53_4")->x);
    TEST_ASSERT_EQUAL(17, lot52_node("m53_4")->y);
    TEST_ASSERT_EQUAL(22, lot52_node("m53_4")->w);
    TEST_ASSERT_EQUAL(16, lot52_node("m53_4")->h);
    TEST_ASSERT_EQUAL(46, lot52_node("m53_5")->x);
    TEST_ASSERT_EQUAL(17, lot52_node("m53_5")->y);
    TEST_ASSERT_EQUAL(22, lot52_node("m53_5")->w);
    TEST_ASSERT_EQUAL(16, lot52_node("m53_5")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_mix_54(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:50px;height:40px;grid-template-columns:repeat(2,24px);grid-template-rows:repeat(3,12px);gap:2px;align-items:stretch;justify-items:stretch'><div id=m54_0></div><div id=m54_1></div><div id=m54_2></div><div id=m54_3></div><div id=m54_4></div><div id=m54_5></div></div></body>", 480);
    TEST_ASSERT_EQUAL(2, lot52_node("g")->grid_cols);
    TEST_ASSERT_EQUAL(3, lot52_node("g")->grid_rows);
    TEST_ASSERT_EQUAL(0, lot52_node("m54_0")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("m54_0")->y);
    TEST_ASSERT_EQUAL(24, lot52_node("m54_0")->w);
    TEST_ASSERT_EQUAL(12, lot52_node("m54_0")->h);
    TEST_ASSERT_EQUAL(26, lot52_node("m54_1")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("m54_1")->y);
    TEST_ASSERT_EQUAL(24, lot52_node("m54_1")->w);
    TEST_ASSERT_EQUAL(12, lot52_node("m54_1")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("m54_2")->x);
    TEST_ASSERT_EQUAL(14, lot52_node("m54_2")->y);
    TEST_ASSERT_EQUAL(24, lot52_node("m54_2")->w);
    TEST_ASSERT_EQUAL(12, lot52_node("m54_2")->h);
    TEST_ASSERT_EQUAL(26, lot52_node("m54_3")->x);
    TEST_ASSERT_EQUAL(14, lot52_node("m54_3")->y);
    TEST_ASSERT_EQUAL(24, lot52_node("m54_3")->w);
    TEST_ASSERT_EQUAL(12, lot52_node("m54_3")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("m54_4")->x);
    TEST_ASSERT_EQUAL(28, lot52_node("m54_4")->y);
    TEST_ASSERT_EQUAL(24, lot52_node("m54_4")->w);
    TEST_ASSERT_EQUAL(12, lot52_node("m54_4")->h);
    TEST_ASSERT_EQUAL(26, lot52_node("m54_5")->x);
    TEST_ASSERT_EQUAL(28, lot52_node("m54_5")->y);
    TEST_ASSERT_EQUAL(24, lot52_node("m54_5")->w);
    TEST_ASSERT_EQUAL(12, lot52_node("m54_5")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_mix_55(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:84px;height:48px;grid-template-columns:repeat(3,26px);grid-template-rows:repeat(3,14px);gap:3px;align-items:stretch;justify-items:stretch'><div id=m55_0></div><div id=m55_1></div><div id=m55_2></div><div id=m55_3></div><div id=m55_4></div><div id=m55_5></div><div id=m55_6></div><div id=m55_7></div><div id=m55_8></div></div></body>", 480);
    TEST_ASSERT_EQUAL(3, lot52_node("g")->grid_cols);
    TEST_ASSERT_EQUAL(3, lot52_node("g")->grid_rows);
    TEST_ASSERT_EQUAL(0, lot52_node("m55_0")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("m55_0")->y);
    TEST_ASSERT_EQUAL(26, lot52_node("m55_0")->w);
    TEST_ASSERT_EQUAL(14, lot52_node("m55_0")->h);
    TEST_ASSERT_EQUAL(29, lot52_node("m55_1")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("m55_1")->y);
    TEST_ASSERT_EQUAL(26, lot52_node("m55_1")->w);
    TEST_ASSERT_EQUAL(14, lot52_node("m55_1")->h);
    TEST_ASSERT_EQUAL(58, lot52_node("m55_2")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("m55_2")->y);
    TEST_ASSERT_EQUAL(26, lot52_node("m55_2")->w);
    TEST_ASSERT_EQUAL(14, lot52_node("m55_2")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("m55_3")->x);
    TEST_ASSERT_EQUAL(17, lot52_node("m55_3")->y);
    TEST_ASSERT_EQUAL(26, lot52_node("m55_3")->w);
    TEST_ASSERT_EQUAL(14, lot52_node("m55_3")->h);
    TEST_ASSERT_EQUAL(29, lot52_node("m55_4")->x);
    TEST_ASSERT_EQUAL(17, lot52_node("m55_4")->y);
    TEST_ASSERT_EQUAL(26, lot52_node("m55_4")->w);
    TEST_ASSERT_EQUAL(14, lot52_node("m55_4")->h);
    TEST_ASSERT_EQUAL(58, lot52_node("m55_5")->x);
    TEST_ASSERT_EQUAL(17, lot52_node("m55_5")->y);
    TEST_ASSERT_EQUAL(26, lot52_node("m55_5")->w);
    TEST_ASSERT_EQUAL(14, lot52_node("m55_5")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("m55_6")->x);
    TEST_ASSERT_EQUAL(34, lot52_node("m55_6")->y);
    TEST_ASSERT_EQUAL(26, lot52_node("m55_6")->w);
    TEST_ASSERT_EQUAL(14, lot52_node("m55_6")->h);
    TEST_ASSERT_EQUAL(29, lot52_node("m55_7")->x);
    TEST_ASSERT_EQUAL(34, lot52_node("m55_7")->y);
    TEST_ASSERT_EQUAL(26, lot52_node("m55_7")->w);
    TEST_ASSERT_EQUAL(14, lot52_node("m55_7")->h);
    TEST_ASSERT_EQUAL(58, lot52_node("m55_8")->x);
    TEST_ASSERT_EQUAL(34, lot52_node("m55_8")->y);
    TEST_ASSERT_EQUAL(26, lot52_node("m55_8")->w);
    TEST_ASSERT_EQUAL(14, lot52_node("m55_8")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_mix_56(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:32px;grid-template-columns:repeat(2,20px);grid-template-rows:repeat(2,16px);gap:0px;align-items:stretch;justify-items:stretch'><div id=m56_0></div><div id=m56_1></div><div id=m56_2></div><div id=m56_3></div></div></body>", 480);
    TEST_ASSERT_EQUAL(2, lot52_node("g")->grid_cols);
    TEST_ASSERT_EQUAL(2, lot52_node("g")->grid_rows);
    TEST_ASSERT_EQUAL(0, lot52_node("m56_0")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("m56_0")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("m56_0")->w);
    TEST_ASSERT_EQUAL(16, lot52_node("m56_0")->h);
    TEST_ASSERT_EQUAL(20, lot52_node("m56_1")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("m56_1")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("m56_1")->w);
    TEST_ASSERT_EQUAL(16, lot52_node("m56_1")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("m56_2")->x);
    TEST_ASSERT_EQUAL(16, lot52_node("m56_2")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("m56_2")->w);
    TEST_ASSERT_EQUAL(16, lot52_node("m56_2")->h);
    TEST_ASSERT_EQUAL(20, lot52_node("m56_3")->x);
    TEST_ASSERT_EQUAL(16, lot52_node("m56_3")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("m56_3")->w);
    TEST_ASSERT_EQUAL(16, lot52_node("m56_3")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_mix_57(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:68px;height:25px;grid-template-columns:repeat(3,22px);grid-template-rows:repeat(2,12px);gap:1px;align-items:stretch;justify-items:stretch'><div id=m57_0></div><div id=m57_1></div><div id=m57_2></div><div id=m57_3></div><div id=m57_4></div><div id=m57_5></div></div></body>", 480);
    TEST_ASSERT_EQUAL(3, lot52_node("g")->grid_cols);
    TEST_ASSERT_EQUAL(2, lot52_node("g")->grid_rows);
    TEST_ASSERT_EQUAL(0, lot52_node("m57_0")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("m57_0")->y);
    TEST_ASSERT_EQUAL(22, lot52_node("m57_0")->w);
    TEST_ASSERT_EQUAL(12, lot52_node("m57_0")->h);
    TEST_ASSERT_EQUAL(23, lot52_node("m57_1")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("m57_1")->y);
    TEST_ASSERT_EQUAL(22, lot52_node("m57_1")->w);
    TEST_ASSERT_EQUAL(12, lot52_node("m57_1")->h);
    TEST_ASSERT_EQUAL(46, lot52_node("m57_2")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("m57_2")->y);
    TEST_ASSERT_EQUAL(22, lot52_node("m57_2")->w);
    TEST_ASSERT_EQUAL(12, lot52_node("m57_2")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("m57_3")->x);
    TEST_ASSERT_EQUAL(13, lot52_node("m57_3")->y);
    TEST_ASSERT_EQUAL(22, lot52_node("m57_3")->w);
    TEST_ASSERT_EQUAL(12, lot52_node("m57_3")->h);
    TEST_ASSERT_EQUAL(23, lot52_node("m57_4")->x);
    TEST_ASSERT_EQUAL(13, lot52_node("m57_4")->y);
    TEST_ASSERT_EQUAL(22, lot52_node("m57_4")->w);
    TEST_ASSERT_EQUAL(12, lot52_node("m57_4")->h);
    TEST_ASSERT_EQUAL(46, lot52_node("m57_5")->x);
    TEST_ASSERT_EQUAL(13, lot52_node("m57_5")->y);
    TEST_ASSERT_EQUAL(22, lot52_node("m57_5")->w);
    TEST_ASSERT_EQUAL(12, lot52_node("m57_5")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_mix_58(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:50px;height:46px;grid-template-columns:repeat(2,24px);grid-template-rows:repeat(3,14px);gap:2px;align-items:stretch;justify-items:stretch'><div id=m58_0></div><div id=m58_1></div><div id=m58_2></div><div id=m58_3></div><div id=m58_4></div><div id=m58_5></div></div></body>", 480);
    TEST_ASSERT_EQUAL(2, lot52_node("g")->grid_cols);
    TEST_ASSERT_EQUAL(3, lot52_node("g")->grid_rows);
    TEST_ASSERT_EQUAL(0, lot52_node("m58_0")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("m58_0")->y);
    TEST_ASSERT_EQUAL(24, lot52_node("m58_0")->w);
    TEST_ASSERT_EQUAL(14, lot52_node("m58_0")->h);
    TEST_ASSERT_EQUAL(26, lot52_node("m58_1")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("m58_1")->y);
    TEST_ASSERT_EQUAL(24, lot52_node("m58_1")->w);
    TEST_ASSERT_EQUAL(14, lot52_node("m58_1")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("m58_2")->x);
    TEST_ASSERT_EQUAL(16, lot52_node("m58_2")->y);
    TEST_ASSERT_EQUAL(24, lot52_node("m58_2")->w);
    TEST_ASSERT_EQUAL(14, lot52_node("m58_2")->h);
    TEST_ASSERT_EQUAL(26, lot52_node("m58_3")->x);
    TEST_ASSERT_EQUAL(16, lot52_node("m58_3")->y);
    TEST_ASSERT_EQUAL(24, lot52_node("m58_3")->w);
    TEST_ASSERT_EQUAL(14, lot52_node("m58_3")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("m58_4")->x);
    TEST_ASSERT_EQUAL(32, lot52_node("m58_4")->y);
    TEST_ASSERT_EQUAL(24, lot52_node("m58_4")->w);
    TEST_ASSERT_EQUAL(14, lot52_node("m58_4")->h);
    TEST_ASSERT_EQUAL(26, lot52_node("m58_5")->x);
    TEST_ASSERT_EQUAL(32, lot52_node("m58_5")->y);
    TEST_ASSERT_EQUAL(24, lot52_node("m58_5")->w);
    TEST_ASSERT_EQUAL(14, lot52_node("m58_5")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_mix_59(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:84px;height:54px;grid-template-columns:repeat(3,26px);grid-template-rows:repeat(3,16px);gap:3px;align-items:stretch;justify-items:stretch'><div id=m59_0></div><div id=m59_1></div><div id=m59_2></div><div id=m59_3></div><div id=m59_4></div><div id=m59_5></div><div id=m59_6></div><div id=m59_7></div><div id=m59_8></div></div></body>", 480);
    TEST_ASSERT_EQUAL(3, lot52_node("g")->grid_cols);
    TEST_ASSERT_EQUAL(3, lot52_node("g")->grid_rows);
    TEST_ASSERT_EQUAL(0, lot52_node("m59_0")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("m59_0")->y);
    TEST_ASSERT_EQUAL(26, lot52_node("m59_0")->w);
    TEST_ASSERT_EQUAL(16, lot52_node("m59_0")->h);
    TEST_ASSERT_EQUAL(29, lot52_node("m59_1")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("m59_1")->y);
    TEST_ASSERT_EQUAL(26, lot52_node("m59_1")->w);
    TEST_ASSERT_EQUAL(16, lot52_node("m59_1")->h);
    TEST_ASSERT_EQUAL(58, lot52_node("m59_2")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("m59_2")->y);
    TEST_ASSERT_EQUAL(26, lot52_node("m59_2")->w);
    TEST_ASSERT_EQUAL(16, lot52_node("m59_2")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("m59_3")->x);
    TEST_ASSERT_EQUAL(19, lot52_node("m59_3")->y);
    TEST_ASSERT_EQUAL(26, lot52_node("m59_3")->w);
    TEST_ASSERT_EQUAL(16, lot52_node("m59_3")->h);
    TEST_ASSERT_EQUAL(29, lot52_node("m59_4")->x);
    TEST_ASSERT_EQUAL(19, lot52_node("m59_4")->y);
    TEST_ASSERT_EQUAL(26, lot52_node("m59_4")->w);
    TEST_ASSERT_EQUAL(16, lot52_node("m59_4")->h);
    TEST_ASSERT_EQUAL(58, lot52_node("m59_5")->x);
    TEST_ASSERT_EQUAL(19, lot52_node("m59_5")->y);
    TEST_ASSERT_EQUAL(26, lot52_node("m59_5")->w);
    TEST_ASSERT_EQUAL(16, lot52_node("m59_5")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("m59_6")->x);
    TEST_ASSERT_EQUAL(38, lot52_node("m59_6")->y);
    TEST_ASSERT_EQUAL(26, lot52_node("m59_6")->w);
    TEST_ASSERT_EQUAL(16, lot52_node("m59_6")->h);
    TEST_ASSERT_EQUAL(29, lot52_node("m59_7")->x);
    TEST_ASSERT_EQUAL(38, lot52_node("m59_7")->y);
    TEST_ASSERT_EQUAL(26, lot52_node("m59_7")->w);
    TEST_ASSERT_EQUAL(16, lot52_node("m59_7")->h);
    TEST_ASSERT_EQUAL(58, lot52_node("m59_8")->x);
    TEST_ASSERT_EQUAL(38, lot52_node("m59_8")->y);
    TEST_ASSERT_EQUAL(26, lot52_node("m59_8")->w);
    TEST_ASSERT_EQUAL(16, lot52_node("m59_8")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_mix_60(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:24px;grid-template-columns:repeat(2,20px);grid-template-rows:repeat(2,12px);gap:0px;align-items:stretch;justify-items:stretch'><div id=m60_0></div><div id=m60_1></div><div id=m60_2></div><div id=m60_3></div></div></body>", 480);
    TEST_ASSERT_EQUAL(2, lot52_node("g")->grid_cols);
    TEST_ASSERT_EQUAL(2, lot52_node("g")->grid_rows);
    TEST_ASSERT_EQUAL(0, lot52_node("m60_0")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("m60_0")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("m60_0")->w);
    TEST_ASSERT_EQUAL(12, lot52_node("m60_0")->h);
    TEST_ASSERT_EQUAL(20, lot52_node("m60_1")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("m60_1")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("m60_1")->w);
    TEST_ASSERT_EQUAL(12, lot52_node("m60_1")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("m60_2")->x);
    TEST_ASSERT_EQUAL(12, lot52_node("m60_2")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("m60_2")->w);
    TEST_ASSERT_EQUAL(12, lot52_node("m60_2")->h);
    TEST_ASSERT_EQUAL(20, lot52_node("m60_3")->x);
    TEST_ASSERT_EQUAL(12, lot52_node("m60_3")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("m60_3")->w);
    TEST_ASSERT_EQUAL(12, lot52_node("m60_3")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_mix_61(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:68px;height:29px;grid-template-columns:repeat(3,22px);grid-template-rows:repeat(2,14px);gap:1px;align-items:stretch;justify-items:stretch'><div id=m61_0></div><div id=m61_1></div><div id=m61_2></div><div id=m61_3></div><div id=m61_4></div><div id=m61_5></div></div></body>", 480);
    TEST_ASSERT_EQUAL(3, lot52_node("g")->grid_cols);
    TEST_ASSERT_EQUAL(2, lot52_node("g")->grid_rows);
    TEST_ASSERT_EQUAL(0, lot52_node("m61_0")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("m61_0")->y);
    TEST_ASSERT_EQUAL(22, lot52_node("m61_0")->w);
    TEST_ASSERT_EQUAL(14, lot52_node("m61_0")->h);
    TEST_ASSERT_EQUAL(23, lot52_node("m61_1")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("m61_1")->y);
    TEST_ASSERT_EQUAL(22, lot52_node("m61_1")->w);
    TEST_ASSERT_EQUAL(14, lot52_node("m61_1")->h);
    TEST_ASSERT_EQUAL(46, lot52_node("m61_2")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("m61_2")->y);
    TEST_ASSERT_EQUAL(22, lot52_node("m61_2")->w);
    TEST_ASSERT_EQUAL(14, lot52_node("m61_2")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("m61_3")->x);
    TEST_ASSERT_EQUAL(15, lot52_node("m61_3")->y);
    TEST_ASSERT_EQUAL(22, lot52_node("m61_3")->w);
    TEST_ASSERT_EQUAL(14, lot52_node("m61_3")->h);
    TEST_ASSERT_EQUAL(23, lot52_node("m61_4")->x);
    TEST_ASSERT_EQUAL(15, lot52_node("m61_4")->y);
    TEST_ASSERT_EQUAL(22, lot52_node("m61_4")->w);
    TEST_ASSERT_EQUAL(14, lot52_node("m61_4")->h);
    TEST_ASSERT_EQUAL(46, lot52_node("m61_5")->x);
    TEST_ASSERT_EQUAL(15, lot52_node("m61_5")->y);
    TEST_ASSERT_EQUAL(22, lot52_node("m61_5")->w);
    TEST_ASSERT_EQUAL(14, lot52_node("m61_5")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_mix_62(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:50px;height:52px;grid-template-columns:repeat(2,24px);grid-template-rows:repeat(3,16px);gap:2px;align-items:stretch;justify-items:stretch'><div id=m62_0></div><div id=m62_1></div><div id=m62_2></div><div id=m62_3></div><div id=m62_4></div><div id=m62_5></div></div></body>", 480);
    TEST_ASSERT_EQUAL(2, lot52_node("g")->grid_cols);
    TEST_ASSERT_EQUAL(3, lot52_node("g")->grid_rows);
    TEST_ASSERT_EQUAL(0, lot52_node("m62_0")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("m62_0")->y);
    TEST_ASSERT_EQUAL(24, lot52_node("m62_0")->w);
    TEST_ASSERT_EQUAL(16, lot52_node("m62_0")->h);
    TEST_ASSERT_EQUAL(26, lot52_node("m62_1")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("m62_1")->y);
    TEST_ASSERT_EQUAL(24, lot52_node("m62_1")->w);
    TEST_ASSERT_EQUAL(16, lot52_node("m62_1")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("m62_2")->x);
    TEST_ASSERT_EQUAL(18, lot52_node("m62_2")->y);
    TEST_ASSERT_EQUAL(24, lot52_node("m62_2")->w);
    TEST_ASSERT_EQUAL(16, lot52_node("m62_2")->h);
    TEST_ASSERT_EQUAL(26, lot52_node("m62_3")->x);
    TEST_ASSERT_EQUAL(18, lot52_node("m62_3")->y);
    TEST_ASSERT_EQUAL(24, lot52_node("m62_3")->w);
    TEST_ASSERT_EQUAL(16, lot52_node("m62_3")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("m62_4")->x);
    TEST_ASSERT_EQUAL(36, lot52_node("m62_4")->y);
    TEST_ASSERT_EQUAL(24, lot52_node("m62_4")->w);
    TEST_ASSERT_EQUAL(16, lot52_node("m62_4")->h);
    TEST_ASSERT_EQUAL(26, lot52_node("m62_5")->x);
    TEST_ASSERT_EQUAL(36, lot52_node("m62_5")->y);
    TEST_ASSERT_EQUAL(24, lot52_node("m62_5")->w);
    TEST_ASSERT_EQUAL(16, lot52_node("m62_5")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_mix_63(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:84px;height:42px;grid-template-columns:repeat(3,26px);grid-template-rows:repeat(3,12px);gap:3px;align-items:stretch;justify-items:stretch'><div id=m63_0></div><div id=m63_1></div><div id=m63_2></div><div id=m63_3></div><div id=m63_4></div><div id=m63_5></div><div id=m63_6></div><div id=m63_7></div><div id=m63_8></div></div></body>", 480);
    TEST_ASSERT_EQUAL(3, lot52_node("g")->grid_cols);
    TEST_ASSERT_EQUAL(3, lot52_node("g")->grid_rows);
    TEST_ASSERT_EQUAL(0, lot52_node("m63_0")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("m63_0")->y);
    TEST_ASSERT_EQUAL(26, lot52_node("m63_0")->w);
    TEST_ASSERT_EQUAL(12, lot52_node("m63_0")->h);
    TEST_ASSERT_EQUAL(29, lot52_node("m63_1")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("m63_1")->y);
    TEST_ASSERT_EQUAL(26, lot52_node("m63_1")->w);
    TEST_ASSERT_EQUAL(12, lot52_node("m63_1")->h);
    TEST_ASSERT_EQUAL(58, lot52_node("m63_2")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("m63_2")->y);
    TEST_ASSERT_EQUAL(26, lot52_node("m63_2")->w);
    TEST_ASSERT_EQUAL(12, lot52_node("m63_2")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("m63_3")->x);
    TEST_ASSERT_EQUAL(15, lot52_node("m63_3")->y);
    TEST_ASSERT_EQUAL(26, lot52_node("m63_3")->w);
    TEST_ASSERT_EQUAL(12, lot52_node("m63_3")->h);
    TEST_ASSERT_EQUAL(29, lot52_node("m63_4")->x);
    TEST_ASSERT_EQUAL(15, lot52_node("m63_4")->y);
    TEST_ASSERT_EQUAL(26, lot52_node("m63_4")->w);
    TEST_ASSERT_EQUAL(12, lot52_node("m63_4")->h);
    TEST_ASSERT_EQUAL(58, lot52_node("m63_5")->x);
    TEST_ASSERT_EQUAL(15, lot52_node("m63_5")->y);
    TEST_ASSERT_EQUAL(26, lot52_node("m63_5")->w);
    TEST_ASSERT_EQUAL(12, lot52_node("m63_5")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("m63_6")->x);
    TEST_ASSERT_EQUAL(30, lot52_node("m63_6")->y);
    TEST_ASSERT_EQUAL(26, lot52_node("m63_6")->w);
    TEST_ASSERT_EQUAL(12, lot52_node("m63_6")->h);
    TEST_ASSERT_EQUAL(29, lot52_node("m63_7")->x);
    TEST_ASSERT_EQUAL(30, lot52_node("m63_7")->y);
    TEST_ASSERT_EQUAL(26, lot52_node("m63_7")->w);
    TEST_ASSERT_EQUAL(12, lot52_node("m63_7")->h);
    TEST_ASSERT_EQUAL(58, lot52_node("m63_8")->x);
    TEST_ASSERT_EQUAL(30, lot52_node("m63_8")->y);
    TEST_ASSERT_EQUAL(26, lot52_node("m63_8")->w);
    TEST_ASSERT_EQUAL(12, lot52_node("m63_8")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_mix_64(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:28px;grid-template-columns:repeat(2,20px);grid-template-rows:repeat(2,14px);gap:0px;align-items:stretch;justify-items:stretch'><div id=m64_0></div><div id=m64_1></div><div id=m64_2></div><div id=m64_3></div></div></body>", 480);
    TEST_ASSERT_EQUAL(2, lot52_node("g")->grid_cols);
    TEST_ASSERT_EQUAL(2, lot52_node("g")->grid_rows);
    TEST_ASSERT_EQUAL(0, lot52_node("m64_0")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("m64_0")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("m64_0")->w);
    TEST_ASSERT_EQUAL(14, lot52_node("m64_0")->h);
    TEST_ASSERT_EQUAL(20, lot52_node("m64_1")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("m64_1")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("m64_1")->w);
    TEST_ASSERT_EQUAL(14, lot52_node("m64_1")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("m64_2")->x);
    TEST_ASSERT_EQUAL(14, lot52_node("m64_2")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("m64_2")->w);
    TEST_ASSERT_EQUAL(14, lot52_node("m64_2")->h);
    TEST_ASSERT_EQUAL(20, lot52_node("m64_3")->x);
    TEST_ASSERT_EQUAL(14, lot52_node("m64_3")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("m64_3")->w);
    TEST_ASSERT_EQUAL(14, lot52_node("m64_3")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_mix_65(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:68px;height:33px;grid-template-columns:repeat(3,22px);grid-template-rows:repeat(2,16px);gap:1px;align-items:stretch;justify-items:stretch'><div id=m65_0></div><div id=m65_1></div><div id=m65_2></div><div id=m65_3></div><div id=m65_4></div><div id=m65_5></div></div></body>", 480);
    TEST_ASSERT_EQUAL(3, lot52_node("g")->grid_cols);
    TEST_ASSERT_EQUAL(2, lot52_node("g")->grid_rows);
    TEST_ASSERT_EQUAL(0, lot52_node("m65_0")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("m65_0")->y);
    TEST_ASSERT_EQUAL(22, lot52_node("m65_0")->w);
    TEST_ASSERT_EQUAL(16, lot52_node("m65_0")->h);
    TEST_ASSERT_EQUAL(23, lot52_node("m65_1")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("m65_1")->y);
    TEST_ASSERT_EQUAL(22, lot52_node("m65_1")->w);
    TEST_ASSERT_EQUAL(16, lot52_node("m65_1")->h);
    TEST_ASSERT_EQUAL(46, lot52_node("m65_2")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("m65_2")->y);
    TEST_ASSERT_EQUAL(22, lot52_node("m65_2")->w);
    TEST_ASSERT_EQUAL(16, lot52_node("m65_2")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("m65_3")->x);
    TEST_ASSERT_EQUAL(17, lot52_node("m65_3")->y);
    TEST_ASSERT_EQUAL(22, lot52_node("m65_3")->w);
    TEST_ASSERT_EQUAL(16, lot52_node("m65_3")->h);
    TEST_ASSERT_EQUAL(23, lot52_node("m65_4")->x);
    TEST_ASSERT_EQUAL(17, lot52_node("m65_4")->y);
    TEST_ASSERT_EQUAL(22, lot52_node("m65_4")->w);
    TEST_ASSERT_EQUAL(16, lot52_node("m65_4")->h);
    TEST_ASSERT_EQUAL(46, lot52_node("m65_5")->x);
    TEST_ASSERT_EQUAL(17, lot52_node("m65_5")->y);
    TEST_ASSERT_EQUAL(22, lot52_node("m65_5")->w);
    TEST_ASSERT_EQUAL(16, lot52_node("m65_5")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_mix_66(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:50px;height:40px;grid-template-columns:repeat(2,24px);grid-template-rows:repeat(3,12px);gap:2px;align-items:stretch;justify-items:stretch'><div id=m66_0></div><div id=m66_1></div><div id=m66_2></div><div id=m66_3></div><div id=m66_4></div><div id=m66_5></div></div></body>", 480);
    TEST_ASSERT_EQUAL(2, lot52_node("g")->grid_cols);
    TEST_ASSERT_EQUAL(3, lot52_node("g")->grid_rows);
    TEST_ASSERT_EQUAL(0, lot52_node("m66_0")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("m66_0")->y);
    TEST_ASSERT_EQUAL(24, lot52_node("m66_0")->w);
    TEST_ASSERT_EQUAL(12, lot52_node("m66_0")->h);
    TEST_ASSERT_EQUAL(26, lot52_node("m66_1")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("m66_1")->y);
    TEST_ASSERT_EQUAL(24, lot52_node("m66_1")->w);
    TEST_ASSERT_EQUAL(12, lot52_node("m66_1")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("m66_2")->x);
    TEST_ASSERT_EQUAL(14, lot52_node("m66_2")->y);
    TEST_ASSERT_EQUAL(24, lot52_node("m66_2")->w);
    TEST_ASSERT_EQUAL(12, lot52_node("m66_2")->h);
    TEST_ASSERT_EQUAL(26, lot52_node("m66_3")->x);
    TEST_ASSERT_EQUAL(14, lot52_node("m66_3")->y);
    TEST_ASSERT_EQUAL(24, lot52_node("m66_3")->w);
    TEST_ASSERT_EQUAL(12, lot52_node("m66_3")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("m66_4")->x);
    TEST_ASSERT_EQUAL(28, lot52_node("m66_4")->y);
    TEST_ASSERT_EQUAL(24, lot52_node("m66_4")->w);
    TEST_ASSERT_EQUAL(12, lot52_node("m66_4")->h);
    TEST_ASSERT_EQUAL(26, lot52_node("m66_5")->x);
    TEST_ASSERT_EQUAL(28, lot52_node("m66_5")->y);
    TEST_ASSERT_EQUAL(24, lot52_node("m66_5")->w);
    TEST_ASSERT_EQUAL(12, lot52_node("m66_5")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_mix_67(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:84px;height:48px;grid-template-columns:repeat(3,26px);grid-template-rows:repeat(3,14px);gap:3px;align-items:stretch;justify-items:stretch'><div id=m67_0></div><div id=m67_1></div><div id=m67_2></div><div id=m67_3></div><div id=m67_4></div><div id=m67_5></div><div id=m67_6></div><div id=m67_7></div><div id=m67_8></div></div></body>", 480);
    TEST_ASSERT_EQUAL(3, lot52_node("g")->grid_cols);
    TEST_ASSERT_EQUAL(3, lot52_node("g")->grid_rows);
    TEST_ASSERT_EQUAL(0, lot52_node("m67_0")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("m67_0")->y);
    TEST_ASSERT_EQUAL(26, lot52_node("m67_0")->w);
    TEST_ASSERT_EQUAL(14, lot52_node("m67_0")->h);
    TEST_ASSERT_EQUAL(29, lot52_node("m67_1")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("m67_1")->y);
    TEST_ASSERT_EQUAL(26, lot52_node("m67_1")->w);
    TEST_ASSERT_EQUAL(14, lot52_node("m67_1")->h);
    TEST_ASSERT_EQUAL(58, lot52_node("m67_2")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("m67_2")->y);
    TEST_ASSERT_EQUAL(26, lot52_node("m67_2")->w);
    TEST_ASSERT_EQUAL(14, lot52_node("m67_2")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("m67_3")->x);
    TEST_ASSERT_EQUAL(17, lot52_node("m67_3")->y);
    TEST_ASSERT_EQUAL(26, lot52_node("m67_3")->w);
    TEST_ASSERT_EQUAL(14, lot52_node("m67_3")->h);
    TEST_ASSERT_EQUAL(29, lot52_node("m67_4")->x);
    TEST_ASSERT_EQUAL(17, lot52_node("m67_4")->y);
    TEST_ASSERT_EQUAL(26, lot52_node("m67_4")->w);
    TEST_ASSERT_EQUAL(14, lot52_node("m67_4")->h);
    TEST_ASSERT_EQUAL(58, lot52_node("m67_5")->x);
    TEST_ASSERT_EQUAL(17, lot52_node("m67_5")->y);
    TEST_ASSERT_EQUAL(26, lot52_node("m67_5")->w);
    TEST_ASSERT_EQUAL(14, lot52_node("m67_5")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("m67_6")->x);
    TEST_ASSERT_EQUAL(34, lot52_node("m67_6")->y);
    TEST_ASSERT_EQUAL(26, lot52_node("m67_6")->w);
    TEST_ASSERT_EQUAL(14, lot52_node("m67_6")->h);
    TEST_ASSERT_EQUAL(29, lot52_node("m67_7")->x);
    TEST_ASSERT_EQUAL(34, lot52_node("m67_7")->y);
    TEST_ASSERT_EQUAL(26, lot52_node("m67_7")->w);
    TEST_ASSERT_EQUAL(14, lot52_node("m67_7")->h);
    TEST_ASSERT_EQUAL(58, lot52_node("m67_8")->x);
    TEST_ASSERT_EQUAL(34, lot52_node("m67_8")->y);
    TEST_ASSERT_EQUAL(26, lot52_node("m67_8")->w);
    TEST_ASSERT_EQUAL(14, lot52_node("m67_8")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_mix_68(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:32px;grid-template-columns:repeat(2,20px);grid-template-rows:repeat(2,16px);gap:0px;align-items:stretch;justify-items:stretch'><div id=m68_0></div><div id=m68_1></div><div id=m68_2></div><div id=m68_3></div></div></body>", 480);
    TEST_ASSERT_EQUAL(2, lot52_node("g")->grid_cols);
    TEST_ASSERT_EQUAL(2, lot52_node("g")->grid_rows);
    TEST_ASSERT_EQUAL(0, lot52_node("m68_0")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("m68_0")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("m68_0")->w);
    TEST_ASSERT_EQUAL(16, lot52_node("m68_0")->h);
    TEST_ASSERT_EQUAL(20, lot52_node("m68_1")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("m68_1")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("m68_1")->w);
    TEST_ASSERT_EQUAL(16, lot52_node("m68_1")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("m68_2")->x);
    TEST_ASSERT_EQUAL(16, lot52_node("m68_2")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("m68_2")->w);
    TEST_ASSERT_EQUAL(16, lot52_node("m68_2")->h);
    TEST_ASSERT_EQUAL(20, lot52_node("m68_3")->x);
    TEST_ASSERT_EQUAL(16, lot52_node("m68_3")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("m68_3")->w);
    TEST_ASSERT_EQUAL(16, lot52_node("m68_3")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_mix_69(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:68px;height:25px;grid-template-columns:repeat(3,22px);grid-template-rows:repeat(2,12px);gap:1px;align-items:stretch;justify-items:stretch'><div id=m69_0></div><div id=m69_1></div><div id=m69_2></div><div id=m69_3></div><div id=m69_4></div><div id=m69_5></div></div></body>", 480);
    TEST_ASSERT_EQUAL(3, lot52_node("g")->grid_cols);
    TEST_ASSERT_EQUAL(2, lot52_node("g")->grid_rows);
    TEST_ASSERT_EQUAL(0, lot52_node("m69_0")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("m69_0")->y);
    TEST_ASSERT_EQUAL(22, lot52_node("m69_0")->w);
    TEST_ASSERT_EQUAL(12, lot52_node("m69_0")->h);
    TEST_ASSERT_EQUAL(23, lot52_node("m69_1")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("m69_1")->y);
    TEST_ASSERT_EQUAL(22, lot52_node("m69_1")->w);
    TEST_ASSERT_EQUAL(12, lot52_node("m69_1")->h);
    TEST_ASSERT_EQUAL(46, lot52_node("m69_2")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("m69_2")->y);
    TEST_ASSERT_EQUAL(22, lot52_node("m69_2")->w);
    TEST_ASSERT_EQUAL(12, lot52_node("m69_2")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("m69_3")->x);
    TEST_ASSERT_EQUAL(13, lot52_node("m69_3")->y);
    TEST_ASSERT_EQUAL(22, lot52_node("m69_3")->w);
    TEST_ASSERT_EQUAL(12, lot52_node("m69_3")->h);
    TEST_ASSERT_EQUAL(23, lot52_node("m69_4")->x);
    TEST_ASSERT_EQUAL(13, lot52_node("m69_4")->y);
    TEST_ASSERT_EQUAL(22, lot52_node("m69_4")->w);
    TEST_ASSERT_EQUAL(12, lot52_node("m69_4")->h);
    TEST_ASSERT_EQUAL(46, lot52_node("m69_5")->x);
    TEST_ASSERT_EQUAL(13, lot52_node("m69_5")->y);
    TEST_ASSERT_EQUAL(22, lot52_node("m69_5")->w);
    TEST_ASSERT_EQUAL(12, lot52_node("m69_5")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_mix_70(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:50px;height:46px;grid-template-columns:repeat(2,24px);grid-template-rows:repeat(3,14px);gap:2px;align-items:stretch;justify-items:stretch'><div id=m70_0></div><div id=m70_1></div><div id=m70_2></div><div id=m70_3></div><div id=m70_4></div><div id=m70_5></div></div></body>", 480);
    TEST_ASSERT_EQUAL(2, lot52_node("g")->grid_cols);
    TEST_ASSERT_EQUAL(3, lot52_node("g")->grid_rows);
    TEST_ASSERT_EQUAL(0, lot52_node("m70_0")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("m70_0")->y);
    TEST_ASSERT_EQUAL(24, lot52_node("m70_0")->w);
    TEST_ASSERT_EQUAL(14, lot52_node("m70_0")->h);
    TEST_ASSERT_EQUAL(26, lot52_node("m70_1")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("m70_1")->y);
    TEST_ASSERT_EQUAL(24, lot52_node("m70_1")->w);
    TEST_ASSERT_EQUAL(14, lot52_node("m70_1")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("m70_2")->x);
    TEST_ASSERT_EQUAL(16, lot52_node("m70_2")->y);
    TEST_ASSERT_EQUAL(24, lot52_node("m70_2")->w);
    TEST_ASSERT_EQUAL(14, lot52_node("m70_2")->h);
    TEST_ASSERT_EQUAL(26, lot52_node("m70_3")->x);
    TEST_ASSERT_EQUAL(16, lot52_node("m70_3")->y);
    TEST_ASSERT_EQUAL(24, lot52_node("m70_3")->w);
    TEST_ASSERT_EQUAL(14, lot52_node("m70_3")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("m70_4")->x);
    TEST_ASSERT_EQUAL(32, lot52_node("m70_4")->y);
    TEST_ASSERT_EQUAL(24, lot52_node("m70_4")->w);
    TEST_ASSERT_EQUAL(14, lot52_node("m70_4")->h);
    TEST_ASSERT_EQUAL(26, lot52_node("m70_5")->x);
    TEST_ASSERT_EQUAL(32, lot52_node("m70_5")->y);
    TEST_ASSERT_EQUAL(24, lot52_node("m70_5")->w);
    TEST_ASSERT_EQUAL(14, lot52_node("m70_5")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_mix_71(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:84px;height:54px;grid-template-columns:repeat(3,26px);grid-template-rows:repeat(3,16px);gap:3px;align-items:stretch;justify-items:stretch'><div id=m71_0></div><div id=m71_1></div><div id=m71_2></div><div id=m71_3></div><div id=m71_4></div><div id=m71_5></div><div id=m71_6></div><div id=m71_7></div><div id=m71_8></div></div></body>", 480);
    TEST_ASSERT_EQUAL(3, lot52_node("g")->grid_cols);
    TEST_ASSERT_EQUAL(3, lot52_node("g")->grid_rows);
    TEST_ASSERT_EQUAL(0, lot52_node("m71_0")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("m71_0")->y);
    TEST_ASSERT_EQUAL(26, lot52_node("m71_0")->w);
    TEST_ASSERT_EQUAL(16, lot52_node("m71_0")->h);
    TEST_ASSERT_EQUAL(29, lot52_node("m71_1")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("m71_1")->y);
    TEST_ASSERT_EQUAL(26, lot52_node("m71_1")->w);
    TEST_ASSERT_EQUAL(16, lot52_node("m71_1")->h);
    TEST_ASSERT_EQUAL(58, lot52_node("m71_2")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("m71_2")->y);
    TEST_ASSERT_EQUAL(26, lot52_node("m71_2")->w);
    TEST_ASSERT_EQUAL(16, lot52_node("m71_2")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("m71_3")->x);
    TEST_ASSERT_EQUAL(19, lot52_node("m71_3")->y);
    TEST_ASSERT_EQUAL(26, lot52_node("m71_3")->w);
    TEST_ASSERT_EQUAL(16, lot52_node("m71_3")->h);
    TEST_ASSERT_EQUAL(29, lot52_node("m71_4")->x);
    TEST_ASSERT_EQUAL(19, lot52_node("m71_4")->y);
    TEST_ASSERT_EQUAL(26, lot52_node("m71_4")->w);
    TEST_ASSERT_EQUAL(16, lot52_node("m71_4")->h);
    TEST_ASSERT_EQUAL(58, lot52_node("m71_5")->x);
    TEST_ASSERT_EQUAL(19, lot52_node("m71_5")->y);
    TEST_ASSERT_EQUAL(26, lot52_node("m71_5")->w);
    TEST_ASSERT_EQUAL(16, lot52_node("m71_5")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("m71_6")->x);
    TEST_ASSERT_EQUAL(38, lot52_node("m71_6")->y);
    TEST_ASSERT_EQUAL(26, lot52_node("m71_6")->w);
    TEST_ASSERT_EQUAL(16, lot52_node("m71_6")->h);
    TEST_ASSERT_EQUAL(29, lot52_node("m71_7")->x);
    TEST_ASSERT_EQUAL(38, lot52_node("m71_7")->y);
    TEST_ASSERT_EQUAL(26, lot52_node("m71_7")->w);
    TEST_ASSERT_EQUAL(16, lot52_node("m71_7")->h);
    TEST_ASSERT_EQUAL(58, lot52_node("m71_8")->x);
    TEST_ASSERT_EQUAL(38, lot52_node("m71_8")->y);
    TEST_ASSERT_EQUAL(26, lot52_node("m71_8")->w);
    TEST_ASSERT_EQUAL(16, lot52_node("m71_8")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_mix_72(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:24px;grid-template-columns:repeat(2,20px);grid-template-rows:repeat(2,12px);gap:0px;align-items:stretch;justify-items:stretch'><div id=m72_0></div><div id=m72_1></div><div id=m72_2></div><div id=m72_3></div></div></body>", 480);
    TEST_ASSERT_EQUAL(2, lot52_node("g")->grid_cols);
    TEST_ASSERT_EQUAL(2, lot52_node("g")->grid_rows);
    TEST_ASSERT_EQUAL(0, lot52_node("m72_0")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("m72_0")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("m72_0")->w);
    TEST_ASSERT_EQUAL(12, lot52_node("m72_0")->h);
    TEST_ASSERT_EQUAL(20, lot52_node("m72_1")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("m72_1")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("m72_1")->w);
    TEST_ASSERT_EQUAL(12, lot52_node("m72_1")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("m72_2")->x);
    TEST_ASSERT_EQUAL(12, lot52_node("m72_2")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("m72_2")->w);
    TEST_ASSERT_EQUAL(12, lot52_node("m72_2")->h);
    TEST_ASSERT_EQUAL(20, lot52_node("m72_3")->x);
    TEST_ASSERT_EQUAL(12, lot52_node("m72_3")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("m72_3")->w);
    TEST_ASSERT_EQUAL(12, lot52_node("m72_3")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_mix_73(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:68px;height:29px;grid-template-columns:repeat(3,22px);grid-template-rows:repeat(2,14px);gap:1px;align-items:stretch;justify-items:stretch'><div id=m73_0></div><div id=m73_1></div><div id=m73_2></div><div id=m73_3></div><div id=m73_4></div><div id=m73_5></div></div></body>", 480);
    TEST_ASSERT_EQUAL(3, lot52_node("g")->grid_cols);
    TEST_ASSERT_EQUAL(2, lot52_node("g")->grid_rows);
    TEST_ASSERT_EQUAL(0, lot52_node("m73_0")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("m73_0")->y);
    TEST_ASSERT_EQUAL(22, lot52_node("m73_0")->w);
    TEST_ASSERT_EQUAL(14, lot52_node("m73_0")->h);
    TEST_ASSERT_EQUAL(23, lot52_node("m73_1")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("m73_1")->y);
    TEST_ASSERT_EQUAL(22, lot52_node("m73_1")->w);
    TEST_ASSERT_EQUAL(14, lot52_node("m73_1")->h);
    TEST_ASSERT_EQUAL(46, lot52_node("m73_2")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("m73_2")->y);
    TEST_ASSERT_EQUAL(22, lot52_node("m73_2")->w);
    TEST_ASSERT_EQUAL(14, lot52_node("m73_2")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("m73_3")->x);
    TEST_ASSERT_EQUAL(15, lot52_node("m73_3")->y);
    TEST_ASSERT_EQUAL(22, lot52_node("m73_3")->w);
    TEST_ASSERT_EQUAL(14, lot52_node("m73_3")->h);
    TEST_ASSERT_EQUAL(23, lot52_node("m73_4")->x);
    TEST_ASSERT_EQUAL(15, lot52_node("m73_4")->y);
    TEST_ASSERT_EQUAL(22, lot52_node("m73_4")->w);
    TEST_ASSERT_EQUAL(14, lot52_node("m73_4")->h);
    TEST_ASSERT_EQUAL(46, lot52_node("m73_5")->x);
    TEST_ASSERT_EQUAL(15, lot52_node("m73_5")->y);
    TEST_ASSERT_EQUAL(22, lot52_node("m73_5")->w);
    TEST_ASSERT_EQUAL(14, lot52_node("m73_5")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_mix_74(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:50px;height:52px;grid-template-columns:repeat(2,24px);grid-template-rows:repeat(3,16px);gap:2px;align-items:stretch;justify-items:stretch'><div id=m74_0></div><div id=m74_1></div><div id=m74_2></div><div id=m74_3></div><div id=m74_4></div><div id=m74_5></div></div></body>", 480);
    TEST_ASSERT_EQUAL(2, lot52_node("g")->grid_cols);
    TEST_ASSERT_EQUAL(3, lot52_node("g")->grid_rows);
    TEST_ASSERT_EQUAL(0, lot52_node("m74_0")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("m74_0")->y);
    TEST_ASSERT_EQUAL(24, lot52_node("m74_0")->w);
    TEST_ASSERT_EQUAL(16, lot52_node("m74_0")->h);
    TEST_ASSERT_EQUAL(26, lot52_node("m74_1")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("m74_1")->y);
    TEST_ASSERT_EQUAL(24, lot52_node("m74_1")->w);
    TEST_ASSERT_EQUAL(16, lot52_node("m74_1")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("m74_2")->x);
    TEST_ASSERT_EQUAL(18, lot52_node("m74_2")->y);
    TEST_ASSERT_EQUAL(24, lot52_node("m74_2")->w);
    TEST_ASSERT_EQUAL(16, lot52_node("m74_2")->h);
    TEST_ASSERT_EQUAL(26, lot52_node("m74_3")->x);
    TEST_ASSERT_EQUAL(18, lot52_node("m74_3")->y);
    TEST_ASSERT_EQUAL(24, lot52_node("m74_3")->w);
    TEST_ASSERT_EQUAL(16, lot52_node("m74_3")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("m74_4")->x);
    TEST_ASSERT_EQUAL(36, lot52_node("m74_4")->y);
    TEST_ASSERT_EQUAL(24, lot52_node("m74_4")->w);
    TEST_ASSERT_EQUAL(16, lot52_node("m74_4")->h);
    TEST_ASSERT_EQUAL(26, lot52_node("m74_5")->x);
    TEST_ASSERT_EQUAL(36, lot52_node("m74_5")->y);
    TEST_ASSERT_EQUAL(24, lot52_node("m74_5")->w);
    TEST_ASSERT_EQUAL(16, lot52_node("m74_5")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_mix_75(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:84px;height:42px;grid-template-columns:repeat(3,26px);grid-template-rows:repeat(3,12px);gap:3px;align-items:stretch;justify-items:stretch'><div id=m75_0></div><div id=m75_1></div><div id=m75_2></div><div id=m75_3></div><div id=m75_4></div><div id=m75_5></div><div id=m75_6></div><div id=m75_7></div><div id=m75_8></div></div></body>", 480);
    TEST_ASSERT_EQUAL(3, lot52_node("g")->grid_cols);
    TEST_ASSERT_EQUAL(3, lot52_node("g")->grid_rows);
    TEST_ASSERT_EQUAL(0, lot52_node("m75_0")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("m75_0")->y);
    TEST_ASSERT_EQUAL(26, lot52_node("m75_0")->w);
    TEST_ASSERT_EQUAL(12, lot52_node("m75_0")->h);
    TEST_ASSERT_EQUAL(29, lot52_node("m75_1")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("m75_1")->y);
    TEST_ASSERT_EQUAL(26, lot52_node("m75_1")->w);
    TEST_ASSERT_EQUAL(12, lot52_node("m75_1")->h);
    TEST_ASSERT_EQUAL(58, lot52_node("m75_2")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("m75_2")->y);
    TEST_ASSERT_EQUAL(26, lot52_node("m75_2")->w);
    TEST_ASSERT_EQUAL(12, lot52_node("m75_2")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("m75_3")->x);
    TEST_ASSERT_EQUAL(15, lot52_node("m75_3")->y);
    TEST_ASSERT_EQUAL(26, lot52_node("m75_3")->w);
    TEST_ASSERT_EQUAL(12, lot52_node("m75_3")->h);
    TEST_ASSERT_EQUAL(29, lot52_node("m75_4")->x);
    TEST_ASSERT_EQUAL(15, lot52_node("m75_4")->y);
    TEST_ASSERT_EQUAL(26, lot52_node("m75_4")->w);
    TEST_ASSERT_EQUAL(12, lot52_node("m75_4")->h);
    TEST_ASSERT_EQUAL(58, lot52_node("m75_5")->x);
    TEST_ASSERT_EQUAL(15, lot52_node("m75_5")->y);
    TEST_ASSERT_EQUAL(26, lot52_node("m75_5")->w);
    TEST_ASSERT_EQUAL(12, lot52_node("m75_5")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("m75_6")->x);
    TEST_ASSERT_EQUAL(30, lot52_node("m75_6")->y);
    TEST_ASSERT_EQUAL(26, lot52_node("m75_6")->w);
    TEST_ASSERT_EQUAL(12, lot52_node("m75_6")->h);
    TEST_ASSERT_EQUAL(29, lot52_node("m75_7")->x);
    TEST_ASSERT_EQUAL(30, lot52_node("m75_7")->y);
    TEST_ASSERT_EQUAL(26, lot52_node("m75_7")->w);
    TEST_ASSERT_EQUAL(12, lot52_node("m75_7")->h);
    TEST_ASSERT_EQUAL(58, lot52_node("m75_8")->x);
    TEST_ASSERT_EQUAL(30, lot52_node("m75_8")->y);
    TEST_ASSERT_EQUAL(26, lot52_node("m75_8")->w);
    TEST_ASSERT_EQUAL(12, lot52_node("m75_8")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_mix_76(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:28px;grid-template-columns:repeat(2,20px);grid-template-rows:repeat(2,14px);gap:0px;align-items:stretch;justify-items:stretch'><div id=m76_0></div><div id=m76_1></div><div id=m76_2></div><div id=m76_3></div></div></body>", 480);
    TEST_ASSERT_EQUAL(2, lot52_node("g")->grid_cols);
    TEST_ASSERT_EQUAL(2, lot52_node("g")->grid_rows);
    TEST_ASSERT_EQUAL(0, lot52_node("m76_0")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("m76_0")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("m76_0")->w);
    TEST_ASSERT_EQUAL(14, lot52_node("m76_0")->h);
    TEST_ASSERT_EQUAL(20, lot52_node("m76_1")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("m76_1")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("m76_1")->w);
    TEST_ASSERT_EQUAL(14, lot52_node("m76_1")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("m76_2")->x);
    TEST_ASSERT_EQUAL(14, lot52_node("m76_2")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("m76_2")->w);
    TEST_ASSERT_EQUAL(14, lot52_node("m76_2")->h);
    TEST_ASSERT_EQUAL(20, lot52_node("m76_3")->x);
    TEST_ASSERT_EQUAL(14, lot52_node("m76_3")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("m76_3")->w);
    TEST_ASSERT_EQUAL(14, lot52_node("m76_3")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_mix_77(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:68px;height:33px;grid-template-columns:repeat(3,22px);grid-template-rows:repeat(2,16px);gap:1px;align-items:stretch;justify-items:stretch'><div id=m77_0></div><div id=m77_1></div><div id=m77_2></div><div id=m77_3></div><div id=m77_4></div><div id=m77_5></div></div></body>", 480);
    TEST_ASSERT_EQUAL(3, lot52_node("g")->grid_cols);
    TEST_ASSERT_EQUAL(2, lot52_node("g")->grid_rows);
    TEST_ASSERT_EQUAL(0, lot52_node("m77_0")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("m77_0")->y);
    TEST_ASSERT_EQUAL(22, lot52_node("m77_0")->w);
    TEST_ASSERT_EQUAL(16, lot52_node("m77_0")->h);
    TEST_ASSERT_EQUAL(23, lot52_node("m77_1")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("m77_1")->y);
    TEST_ASSERT_EQUAL(22, lot52_node("m77_1")->w);
    TEST_ASSERT_EQUAL(16, lot52_node("m77_1")->h);
    TEST_ASSERT_EQUAL(46, lot52_node("m77_2")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("m77_2")->y);
    TEST_ASSERT_EQUAL(22, lot52_node("m77_2")->w);
    TEST_ASSERT_EQUAL(16, lot52_node("m77_2")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("m77_3")->x);
    TEST_ASSERT_EQUAL(17, lot52_node("m77_3")->y);
    TEST_ASSERT_EQUAL(22, lot52_node("m77_3")->w);
    TEST_ASSERT_EQUAL(16, lot52_node("m77_3")->h);
    TEST_ASSERT_EQUAL(23, lot52_node("m77_4")->x);
    TEST_ASSERT_EQUAL(17, lot52_node("m77_4")->y);
    TEST_ASSERT_EQUAL(22, lot52_node("m77_4")->w);
    TEST_ASSERT_EQUAL(16, lot52_node("m77_4")->h);
    TEST_ASSERT_EQUAL(46, lot52_node("m77_5")->x);
    TEST_ASSERT_EQUAL(17, lot52_node("m77_5")->y);
    TEST_ASSERT_EQUAL(22, lot52_node("m77_5")->w);
    TEST_ASSERT_EQUAL(16, lot52_node("m77_5")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_mix_78(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:50px;height:40px;grid-template-columns:repeat(2,24px);grid-template-rows:repeat(3,12px);gap:2px;align-items:stretch;justify-items:stretch'><div id=m78_0></div><div id=m78_1></div><div id=m78_2></div><div id=m78_3></div><div id=m78_4></div><div id=m78_5></div></div></body>", 480);
    TEST_ASSERT_EQUAL(2, lot52_node("g")->grid_cols);
    TEST_ASSERT_EQUAL(3, lot52_node("g")->grid_rows);
    TEST_ASSERT_EQUAL(0, lot52_node("m78_0")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("m78_0")->y);
    TEST_ASSERT_EQUAL(24, lot52_node("m78_0")->w);
    TEST_ASSERT_EQUAL(12, lot52_node("m78_0")->h);
    TEST_ASSERT_EQUAL(26, lot52_node("m78_1")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("m78_1")->y);
    TEST_ASSERT_EQUAL(24, lot52_node("m78_1")->w);
    TEST_ASSERT_EQUAL(12, lot52_node("m78_1")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("m78_2")->x);
    TEST_ASSERT_EQUAL(14, lot52_node("m78_2")->y);
    TEST_ASSERT_EQUAL(24, lot52_node("m78_2")->w);
    TEST_ASSERT_EQUAL(12, lot52_node("m78_2")->h);
    TEST_ASSERT_EQUAL(26, lot52_node("m78_3")->x);
    TEST_ASSERT_EQUAL(14, lot52_node("m78_3")->y);
    TEST_ASSERT_EQUAL(24, lot52_node("m78_3")->w);
    TEST_ASSERT_EQUAL(12, lot52_node("m78_3")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("m78_4")->x);
    TEST_ASSERT_EQUAL(28, lot52_node("m78_4")->y);
    TEST_ASSERT_EQUAL(24, lot52_node("m78_4")->w);
    TEST_ASSERT_EQUAL(12, lot52_node("m78_4")->h);
    TEST_ASSERT_EQUAL(26, lot52_node("m78_5")->x);
    TEST_ASSERT_EQUAL(28, lot52_node("m78_5")->y);
    TEST_ASSERT_EQUAL(24, lot52_node("m78_5")->w);
    TEST_ASSERT_EQUAL(12, lot52_node("m78_5")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_mix_79(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:84px;height:48px;grid-template-columns:repeat(3,26px);grid-template-rows:repeat(3,14px);gap:3px;align-items:stretch;justify-items:stretch'><div id=m79_0></div><div id=m79_1></div><div id=m79_2></div><div id=m79_3></div><div id=m79_4></div><div id=m79_5></div><div id=m79_6></div><div id=m79_7></div><div id=m79_8></div></div></body>", 480);
    TEST_ASSERT_EQUAL(3, lot52_node("g")->grid_cols);
    TEST_ASSERT_EQUAL(3, lot52_node("g")->grid_rows);
    TEST_ASSERT_EQUAL(0, lot52_node("m79_0")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("m79_0")->y);
    TEST_ASSERT_EQUAL(26, lot52_node("m79_0")->w);
    TEST_ASSERT_EQUAL(14, lot52_node("m79_0")->h);
    TEST_ASSERT_EQUAL(29, lot52_node("m79_1")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("m79_1")->y);
    TEST_ASSERT_EQUAL(26, lot52_node("m79_1")->w);
    TEST_ASSERT_EQUAL(14, lot52_node("m79_1")->h);
    TEST_ASSERT_EQUAL(58, lot52_node("m79_2")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("m79_2")->y);
    TEST_ASSERT_EQUAL(26, lot52_node("m79_2")->w);
    TEST_ASSERT_EQUAL(14, lot52_node("m79_2")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("m79_3")->x);
    TEST_ASSERT_EQUAL(17, lot52_node("m79_3")->y);
    TEST_ASSERT_EQUAL(26, lot52_node("m79_3")->w);
    TEST_ASSERT_EQUAL(14, lot52_node("m79_3")->h);
    TEST_ASSERT_EQUAL(29, lot52_node("m79_4")->x);
    TEST_ASSERT_EQUAL(17, lot52_node("m79_4")->y);
    TEST_ASSERT_EQUAL(26, lot52_node("m79_4")->w);
    TEST_ASSERT_EQUAL(14, lot52_node("m79_4")->h);
    TEST_ASSERT_EQUAL(58, lot52_node("m79_5")->x);
    TEST_ASSERT_EQUAL(17, lot52_node("m79_5")->y);
    TEST_ASSERT_EQUAL(26, lot52_node("m79_5")->w);
    TEST_ASSERT_EQUAL(14, lot52_node("m79_5")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("m79_6")->x);
    TEST_ASSERT_EQUAL(34, lot52_node("m79_6")->y);
    TEST_ASSERT_EQUAL(26, lot52_node("m79_6")->w);
    TEST_ASSERT_EQUAL(14, lot52_node("m79_6")->h);
    TEST_ASSERT_EQUAL(29, lot52_node("m79_7")->x);
    TEST_ASSERT_EQUAL(34, lot52_node("m79_7")->y);
    TEST_ASSERT_EQUAL(26, lot52_node("m79_7")->w);
    TEST_ASSERT_EQUAL(14, lot52_node("m79_7")->h);
    TEST_ASSERT_EQUAL(58, lot52_node("m79_8")->x);
    TEST_ASSERT_EQUAL(34, lot52_node("m79_8")->y);
    TEST_ASSERT_EQUAL(26, lot52_node("m79_8")->w);
    TEST_ASSERT_EQUAL(14, lot52_node("m79_8")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_mix_80(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:32px;grid-template-columns:repeat(2,20px);grid-template-rows:repeat(2,16px);gap:0px;align-items:stretch;justify-items:stretch'><div id=m80_0></div><div id=m80_1></div><div id=m80_2></div><div id=m80_3></div></div></body>", 480);
    TEST_ASSERT_EQUAL(2, lot52_node("g")->grid_cols);
    TEST_ASSERT_EQUAL(2, lot52_node("g")->grid_rows);
    TEST_ASSERT_EQUAL(0, lot52_node("m80_0")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("m80_0")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("m80_0")->w);
    TEST_ASSERT_EQUAL(16, lot52_node("m80_0")->h);
    TEST_ASSERT_EQUAL(20, lot52_node("m80_1")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("m80_1")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("m80_1")->w);
    TEST_ASSERT_EQUAL(16, lot52_node("m80_1")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("m80_2")->x);
    TEST_ASSERT_EQUAL(16, lot52_node("m80_2")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("m80_2")->w);
    TEST_ASSERT_EQUAL(16, lot52_node("m80_2")->h);
    TEST_ASSERT_EQUAL(20, lot52_node("m80_3")->x);
    TEST_ASSERT_EQUAL(16, lot52_node("m80_3")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("m80_3")->w);
    TEST_ASSERT_EQUAL(16, lot52_node("m80_3")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_mix_81(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:68px;height:25px;grid-template-columns:repeat(3,22px);grid-template-rows:repeat(2,12px);gap:1px;align-items:stretch;justify-items:stretch'><div id=m81_0></div><div id=m81_1></div><div id=m81_2></div><div id=m81_3></div><div id=m81_4></div><div id=m81_5></div></div></body>", 480);
    TEST_ASSERT_EQUAL(3, lot52_node("g")->grid_cols);
    TEST_ASSERT_EQUAL(2, lot52_node("g")->grid_rows);
    TEST_ASSERT_EQUAL(0, lot52_node("m81_0")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("m81_0")->y);
    TEST_ASSERT_EQUAL(22, lot52_node("m81_0")->w);
    TEST_ASSERT_EQUAL(12, lot52_node("m81_0")->h);
    TEST_ASSERT_EQUAL(23, lot52_node("m81_1")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("m81_1")->y);
    TEST_ASSERT_EQUAL(22, lot52_node("m81_1")->w);
    TEST_ASSERT_EQUAL(12, lot52_node("m81_1")->h);
    TEST_ASSERT_EQUAL(46, lot52_node("m81_2")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("m81_2")->y);
    TEST_ASSERT_EQUAL(22, lot52_node("m81_2")->w);
    TEST_ASSERT_EQUAL(12, lot52_node("m81_2")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("m81_3")->x);
    TEST_ASSERT_EQUAL(13, lot52_node("m81_3")->y);
    TEST_ASSERT_EQUAL(22, lot52_node("m81_3")->w);
    TEST_ASSERT_EQUAL(12, lot52_node("m81_3")->h);
    TEST_ASSERT_EQUAL(23, lot52_node("m81_4")->x);
    TEST_ASSERT_EQUAL(13, lot52_node("m81_4")->y);
    TEST_ASSERT_EQUAL(22, lot52_node("m81_4")->w);
    TEST_ASSERT_EQUAL(12, lot52_node("m81_4")->h);
    TEST_ASSERT_EQUAL(46, lot52_node("m81_5")->x);
    TEST_ASSERT_EQUAL(13, lot52_node("m81_5")->y);
    TEST_ASSERT_EQUAL(22, lot52_node("m81_5")->w);
    TEST_ASSERT_EQUAL(12, lot52_node("m81_5")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_mix_82(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:50px;height:46px;grid-template-columns:repeat(2,24px);grid-template-rows:repeat(3,14px);gap:2px;align-items:stretch;justify-items:stretch'><div id=m82_0></div><div id=m82_1></div><div id=m82_2></div><div id=m82_3></div><div id=m82_4></div><div id=m82_5></div></div></body>", 480);
    TEST_ASSERT_EQUAL(2, lot52_node("g")->grid_cols);
    TEST_ASSERT_EQUAL(3, lot52_node("g")->grid_rows);
    TEST_ASSERT_EQUAL(0, lot52_node("m82_0")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("m82_0")->y);
    TEST_ASSERT_EQUAL(24, lot52_node("m82_0")->w);
    TEST_ASSERT_EQUAL(14, lot52_node("m82_0")->h);
    TEST_ASSERT_EQUAL(26, lot52_node("m82_1")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("m82_1")->y);
    TEST_ASSERT_EQUAL(24, lot52_node("m82_1")->w);
    TEST_ASSERT_EQUAL(14, lot52_node("m82_1")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("m82_2")->x);
    TEST_ASSERT_EQUAL(16, lot52_node("m82_2")->y);
    TEST_ASSERT_EQUAL(24, lot52_node("m82_2")->w);
    TEST_ASSERT_EQUAL(14, lot52_node("m82_2")->h);
    TEST_ASSERT_EQUAL(26, lot52_node("m82_3")->x);
    TEST_ASSERT_EQUAL(16, lot52_node("m82_3")->y);
    TEST_ASSERT_EQUAL(24, lot52_node("m82_3")->w);
    TEST_ASSERT_EQUAL(14, lot52_node("m82_3")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("m82_4")->x);
    TEST_ASSERT_EQUAL(32, lot52_node("m82_4")->y);
    TEST_ASSERT_EQUAL(24, lot52_node("m82_4")->w);
    TEST_ASSERT_EQUAL(14, lot52_node("m82_4")->h);
    TEST_ASSERT_EQUAL(26, lot52_node("m82_5")->x);
    TEST_ASSERT_EQUAL(32, lot52_node("m82_5")->y);
    TEST_ASSERT_EQUAL(24, lot52_node("m82_5")->w);
    TEST_ASSERT_EQUAL(14, lot52_node("m82_5")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_mix_83(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:84px;height:54px;grid-template-columns:repeat(3,26px);grid-template-rows:repeat(3,16px);gap:3px;align-items:stretch;justify-items:stretch'><div id=m83_0></div><div id=m83_1></div><div id=m83_2></div><div id=m83_3></div><div id=m83_4></div><div id=m83_5></div><div id=m83_6></div><div id=m83_7></div><div id=m83_8></div></div></body>", 480);
    TEST_ASSERT_EQUAL(3, lot52_node("g")->grid_cols);
    TEST_ASSERT_EQUAL(3, lot52_node("g")->grid_rows);
    TEST_ASSERT_EQUAL(0, lot52_node("m83_0")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("m83_0")->y);
    TEST_ASSERT_EQUAL(26, lot52_node("m83_0")->w);
    TEST_ASSERT_EQUAL(16, lot52_node("m83_0")->h);
    TEST_ASSERT_EQUAL(29, lot52_node("m83_1")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("m83_1")->y);
    TEST_ASSERT_EQUAL(26, lot52_node("m83_1")->w);
    TEST_ASSERT_EQUAL(16, lot52_node("m83_1")->h);
    TEST_ASSERT_EQUAL(58, lot52_node("m83_2")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("m83_2")->y);
    TEST_ASSERT_EQUAL(26, lot52_node("m83_2")->w);
    TEST_ASSERT_EQUAL(16, lot52_node("m83_2")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("m83_3")->x);
    TEST_ASSERT_EQUAL(19, lot52_node("m83_3")->y);
    TEST_ASSERT_EQUAL(26, lot52_node("m83_3")->w);
    TEST_ASSERT_EQUAL(16, lot52_node("m83_3")->h);
    TEST_ASSERT_EQUAL(29, lot52_node("m83_4")->x);
    TEST_ASSERT_EQUAL(19, lot52_node("m83_4")->y);
    TEST_ASSERT_EQUAL(26, lot52_node("m83_4")->w);
    TEST_ASSERT_EQUAL(16, lot52_node("m83_4")->h);
    TEST_ASSERT_EQUAL(58, lot52_node("m83_5")->x);
    TEST_ASSERT_EQUAL(19, lot52_node("m83_5")->y);
    TEST_ASSERT_EQUAL(26, lot52_node("m83_5")->w);
    TEST_ASSERT_EQUAL(16, lot52_node("m83_5")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("m83_6")->x);
    TEST_ASSERT_EQUAL(38, lot52_node("m83_6")->y);
    TEST_ASSERT_EQUAL(26, lot52_node("m83_6")->w);
    TEST_ASSERT_EQUAL(16, lot52_node("m83_6")->h);
    TEST_ASSERT_EQUAL(29, lot52_node("m83_7")->x);
    TEST_ASSERT_EQUAL(38, lot52_node("m83_7")->y);
    TEST_ASSERT_EQUAL(26, lot52_node("m83_7")->w);
    TEST_ASSERT_EQUAL(16, lot52_node("m83_7")->h);
    TEST_ASSERT_EQUAL(58, lot52_node("m83_8")->x);
    TEST_ASSERT_EQUAL(38, lot52_node("m83_8")->y);
    TEST_ASSERT_EQUAL(26, lot52_node("m83_8")->w);
    TEST_ASSERT_EQUAL(16, lot52_node("m83_8")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_mix_84(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:24px;grid-template-columns:repeat(2,20px);grid-template-rows:repeat(2,12px);gap:0px;align-items:stretch;justify-items:stretch'><div id=m84_0></div><div id=m84_1></div><div id=m84_2></div><div id=m84_3></div></div></body>", 480);
    TEST_ASSERT_EQUAL(2, lot52_node("g")->grid_cols);
    TEST_ASSERT_EQUAL(2, lot52_node("g")->grid_rows);
    TEST_ASSERT_EQUAL(0, lot52_node("m84_0")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("m84_0")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("m84_0")->w);
    TEST_ASSERT_EQUAL(12, lot52_node("m84_0")->h);
    TEST_ASSERT_EQUAL(20, lot52_node("m84_1")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("m84_1")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("m84_1")->w);
    TEST_ASSERT_EQUAL(12, lot52_node("m84_1")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("m84_2")->x);
    TEST_ASSERT_EQUAL(12, lot52_node("m84_2")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("m84_2")->w);
    TEST_ASSERT_EQUAL(12, lot52_node("m84_2")->h);
    TEST_ASSERT_EQUAL(20, lot52_node("m84_3")->x);
    TEST_ASSERT_EQUAL(12, lot52_node("m84_3")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("m84_3")->w);
    TEST_ASSERT_EQUAL(12, lot52_node("m84_3")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_mix_85(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:68px;height:29px;grid-template-columns:repeat(3,22px);grid-template-rows:repeat(2,14px);gap:1px;align-items:stretch;justify-items:stretch'><div id=m85_0></div><div id=m85_1></div><div id=m85_2></div><div id=m85_3></div><div id=m85_4></div><div id=m85_5></div></div></body>", 480);
    TEST_ASSERT_EQUAL(3, lot52_node("g")->grid_cols);
    TEST_ASSERT_EQUAL(2, lot52_node("g")->grid_rows);
    TEST_ASSERT_EQUAL(0, lot52_node("m85_0")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("m85_0")->y);
    TEST_ASSERT_EQUAL(22, lot52_node("m85_0")->w);
    TEST_ASSERT_EQUAL(14, lot52_node("m85_0")->h);
    TEST_ASSERT_EQUAL(23, lot52_node("m85_1")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("m85_1")->y);
    TEST_ASSERT_EQUAL(22, lot52_node("m85_1")->w);
    TEST_ASSERT_EQUAL(14, lot52_node("m85_1")->h);
    TEST_ASSERT_EQUAL(46, lot52_node("m85_2")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("m85_2")->y);
    TEST_ASSERT_EQUAL(22, lot52_node("m85_2")->w);
    TEST_ASSERT_EQUAL(14, lot52_node("m85_2")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("m85_3")->x);
    TEST_ASSERT_EQUAL(15, lot52_node("m85_3")->y);
    TEST_ASSERT_EQUAL(22, lot52_node("m85_3")->w);
    TEST_ASSERT_EQUAL(14, lot52_node("m85_3")->h);
    TEST_ASSERT_EQUAL(23, lot52_node("m85_4")->x);
    TEST_ASSERT_EQUAL(15, lot52_node("m85_4")->y);
    TEST_ASSERT_EQUAL(22, lot52_node("m85_4")->w);
    TEST_ASSERT_EQUAL(14, lot52_node("m85_4")->h);
    TEST_ASSERT_EQUAL(46, lot52_node("m85_5")->x);
    TEST_ASSERT_EQUAL(15, lot52_node("m85_5")->y);
    TEST_ASSERT_EQUAL(22, lot52_node("m85_5")->w);
    TEST_ASSERT_EQUAL(14, lot52_node("m85_5")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_mix_86(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:50px;height:52px;grid-template-columns:repeat(2,24px);grid-template-rows:repeat(3,16px);gap:2px;align-items:stretch;justify-items:stretch'><div id=m86_0></div><div id=m86_1></div><div id=m86_2></div><div id=m86_3></div><div id=m86_4></div><div id=m86_5></div></div></body>", 480);
    TEST_ASSERT_EQUAL(2, lot52_node("g")->grid_cols);
    TEST_ASSERT_EQUAL(3, lot52_node("g")->grid_rows);
    TEST_ASSERT_EQUAL(0, lot52_node("m86_0")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("m86_0")->y);
    TEST_ASSERT_EQUAL(24, lot52_node("m86_0")->w);
    TEST_ASSERT_EQUAL(16, lot52_node("m86_0")->h);
    TEST_ASSERT_EQUAL(26, lot52_node("m86_1")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("m86_1")->y);
    TEST_ASSERT_EQUAL(24, lot52_node("m86_1")->w);
    TEST_ASSERT_EQUAL(16, lot52_node("m86_1")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("m86_2")->x);
    TEST_ASSERT_EQUAL(18, lot52_node("m86_2")->y);
    TEST_ASSERT_EQUAL(24, lot52_node("m86_2")->w);
    TEST_ASSERT_EQUAL(16, lot52_node("m86_2")->h);
    TEST_ASSERT_EQUAL(26, lot52_node("m86_3")->x);
    TEST_ASSERT_EQUAL(18, lot52_node("m86_3")->y);
    TEST_ASSERT_EQUAL(24, lot52_node("m86_3")->w);
    TEST_ASSERT_EQUAL(16, lot52_node("m86_3")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("m86_4")->x);
    TEST_ASSERT_EQUAL(36, lot52_node("m86_4")->y);
    TEST_ASSERT_EQUAL(24, lot52_node("m86_4")->w);
    TEST_ASSERT_EQUAL(16, lot52_node("m86_4")->h);
    TEST_ASSERT_EQUAL(26, lot52_node("m86_5")->x);
    TEST_ASSERT_EQUAL(36, lot52_node("m86_5")->y);
    TEST_ASSERT_EQUAL(24, lot52_node("m86_5")->w);
    TEST_ASSERT_EQUAL(16, lot52_node("m86_5")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_mix_87(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:84px;height:42px;grid-template-columns:repeat(3,26px);grid-template-rows:repeat(3,12px);gap:3px;align-items:stretch;justify-items:stretch'><div id=m87_0></div><div id=m87_1></div><div id=m87_2></div><div id=m87_3></div><div id=m87_4></div><div id=m87_5></div><div id=m87_6></div><div id=m87_7></div><div id=m87_8></div></div></body>", 480);
    TEST_ASSERT_EQUAL(3, lot52_node("g")->grid_cols);
    TEST_ASSERT_EQUAL(3, lot52_node("g")->grid_rows);
    TEST_ASSERT_EQUAL(0, lot52_node("m87_0")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("m87_0")->y);
    TEST_ASSERT_EQUAL(26, lot52_node("m87_0")->w);
    TEST_ASSERT_EQUAL(12, lot52_node("m87_0")->h);
    TEST_ASSERT_EQUAL(29, lot52_node("m87_1")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("m87_1")->y);
    TEST_ASSERT_EQUAL(26, lot52_node("m87_1")->w);
    TEST_ASSERT_EQUAL(12, lot52_node("m87_1")->h);
    TEST_ASSERT_EQUAL(58, lot52_node("m87_2")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("m87_2")->y);
    TEST_ASSERT_EQUAL(26, lot52_node("m87_2")->w);
    TEST_ASSERT_EQUAL(12, lot52_node("m87_2")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("m87_3")->x);
    TEST_ASSERT_EQUAL(15, lot52_node("m87_3")->y);
    TEST_ASSERT_EQUAL(26, lot52_node("m87_3")->w);
    TEST_ASSERT_EQUAL(12, lot52_node("m87_3")->h);
    TEST_ASSERT_EQUAL(29, lot52_node("m87_4")->x);
    TEST_ASSERT_EQUAL(15, lot52_node("m87_4")->y);
    TEST_ASSERT_EQUAL(26, lot52_node("m87_4")->w);
    TEST_ASSERT_EQUAL(12, lot52_node("m87_4")->h);
    TEST_ASSERT_EQUAL(58, lot52_node("m87_5")->x);
    TEST_ASSERT_EQUAL(15, lot52_node("m87_5")->y);
    TEST_ASSERT_EQUAL(26, lot52_node("m87_5")->w);
    TEST_ASSERT_EQUAL(12, lot52_node("m87_5")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("m87_6")->x);
    TEST_ASSERT_EQUAL(30, lot52_node("m87_6")->y);
    TEST_ASSERT_EQUAL(26, lot52_node("m87_6")->w);
    TEST_ASSERT_EQUAL(12, lot52_node("m87_6")->h);
    TEST_ASSERT_EQUAL(29, lot52_node("m87_7")->x);
    TEST_ASSERT_EQUAL(30, lot52_node("m87_7")->y);
    TEST_ASSERT_EQUAL(26, lot52_node("m87_7")->w);
    TEST_ASSERT_EQUAL(12, lot52_node("m87_7")->h);
    TEST_ASSERT_EQUAL(58, lot52_node("m87_8")->x);
    TEST_ASSERT_EQUAL(30, lot52_node("m87_8")->y);
    TEST_ASSERT_EQUAL(26, lot52_node("m87_8")->w);
    TEST_ASSERT_EQUAL(12, lot52_node("m87_8")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_mix_88(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:28px;grid-template-columns:repeat(2,20px);grid-template-rows:repeat(2,14px);gap:0px;align-items:stretch;justify-items:stretch'><div id=m88_0></div><div id=m88_1></div><div id=m88_2></div><div id=m88_3></div></div></body>", 480);
    TEST_ASSERT_EQUAL(2, lot52_node("g")->grid_cols);
    TEST_ASSERT_EQUAL(2, lot52_node("g")->grid_rows);
    TEST_ASSERT_EQUAL(0, lot52_node("m88_0")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("m88_0")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("m88_0")->w);
    TEST_ASSERT_EQUAL(14, lot52_node("m88_0")->h);
    TEST_ASSERT_EQUAL(20, lot52_node("m88_1")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("m88_1")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("m88_1")->w);
    TEST_ASSERT_EQUAL(14, lot52_node("m88_1")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("m88_2")->x);
    TEST_ASSERT_EQUAL(14, lot52_node("m88_2")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("m88_2")->w);
    TEST_ASSERT_EQUAL(14, lot52_node("m88_2")->h);
    TEST_ASSERT_EQUAL(20, lot52_node("m88_3")->x);
    TEST_ASSERT_EQUAL(14, lot52_node("m88_3")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("m88_3")->w);
    TEST_ASSERT_EQUAL(14, lot52_node("m88_3")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_mix_89(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:68px;height:33px;grid-template-columns:repeat(3,22px);grid-template-rows:repeat(2,16px);gap:1px;align-items:stretch;justify-items:stretch'><div id=m89_0></div><div id=m89_1></div><div id=m89_2></div><div id=m89_3></div><div id=m89_4></div><div id=m89_5></div></div></body>", 480);
    TEST_ASSERT_EQUAL(3, lot52_node("g")->grid_cols);
    TEST_ASSERT_EQUAL(2, lot52_node("g")->grid_rows);
    TEST_ASSERT_EQUAL(0, lot52_node("m89_0")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("m89_0")->y);
    TEST_ASSERT_EQUAL(22, lot52_node("m89_0")->w);
    TEST_ASSERT_EQUAL(16, lot52_node("m89_0")->h);
    TEST_ASSERT_EQUAL(23, lot52_node("m89_1")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("m89_1")->y);
    TEST_ASSERT_EQUAL(22, lot52_node("m89_1")->w);
    TEST_ASSERT_EQUAL(16, lot52_node("m89_1")->h);
    TEST_ASSERT_EQUAL(46, lot52_node("m89_2")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("m89_2")->y);
    TEST_ASSERT_EQUAL(22, lot52_node("m89_2")->w);
    TEST_ASSERT_EQUAL(16, lot52_node("m89_2")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("m89_3")->x);
    TEST_ASSERT_EQUAL(17, lot52_node("m89_3")->y);
    TEST_ASSERT_EQUAL(22, lot52_node("m89_3")->w);
    TEST_ASSERT_EQUAL(16, lot52_node("m89_3")->h);
    TEST_ASSERT_EQUAL(23, lot52_node("m89_4")->x);
    TEST_ASSERT_EQUAL(17, lot52_node("m89_4")->y);
    TEST_ASSERT_EQUAL(22, lot52_node("m89_4")->w);
    TEST_ASSERT_EQUAL(16, lot52_node("m89_4")->h);
    TEST_ASSERT_EQUAL(46, lot52_node("m89_5")->x);
    TEST_ASSERT_EQUAL(17, lot52_node("m89_5")->y);
    TEST_ASSERT_EQUAL(22, lot52_node("m89_5")->w);
    TEST_ASSERT_EQUAL(16, lot52_node("m89_5")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_mix_90(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:50px;height:40px;grid-template-columns:repeat(2,24px);grid-template-rows:repeat(3,12px);gap:2px;align-items:stretch;justify-items:stretch'><div id=m90_0></div><div id=m90_1></div><div id=m90_2></div><div id=m90_3></div><div id=m90_4></div><div id=m90_5></div></div></body>", 480);
    TEST_ASSERT_EQUAL(2, lot52_node("g")->grid_cols);
    TEST_ASSERT_EQUAL(3, lot52_node("g")->grid_rows);
    TEST_ASSERT_EQUAL(0, lot52_node("m90_0")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("m90_0")->y);
    TEST_ASSERT_EQUAL(24, lot52_node("m90_0")->w);
    TEST_ASSERT_EQUAL(12, lot52_node("m90_0")->h);
    TEST_ASSERT_EQUAL(26, lot52_node("m90_1")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("m90_1")->y);
    TEST_ASSERT_EQUAL(24, lot52_node("m90_1")->w);
    TEST_ASSERT_EQUAL(12, lot52_node("m90_1")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("m90_2")->x);
    TEST_ASSERT_EQUAL(14, lot52_node("m90_2")->y);
    TEST_ASSERT_EQUAL(24, lot52_node("m90_2")->w);
    TEST_ASSERT_EQUAL(12, lot52_node("m90_2")->h);
    TEST_ASSERT_EQUAL(26, lot52_node("m90_3")->x);
    TEST_ASSERT_EQUAL(14, lot52_node("m90_3")->y);
    TEST_ASSERT_EQUAL(24, lot52_node("m90_3")->w);
    TEST_ASSERT_EQUAL(12, lot52_node("m90_3")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("m90_4")->x);
    TEST_ASSERT_EQUAL(28, lot52_node("m90_4")->y);
    TEST_ASSERT_EQUAL(24, lot52_node("m90_4")->w);
    TEST_ASSERT_EQUAL(12, lot52_node("m90_4")->h);
    TEST_ASSERT_EQUAL(26, lot52_node("m90_5")->x);
    TEST_ASSERT_EQUAL(28, lot52_node("m90_5")->y);
    TEST_ASSERT_EQUAL(24, lot52_node("m90_5")->w);
    TEST_ASSERT_EQUAL(12, lot52_node("m90_5")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_mix_91(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:84px;height:48px;grid-template-columns:repeat(3,26px);grid-template-rows:repeat(3,14px);gap:3px;align-items:stretch;justify-items:stretch'><div id=m91_0></div><div id=m91_1></div><div id=m91_2></div><div id=m91_3></div><div id=m91_4></div><div id=m91_5></div><div id=m91_6></div><div id=m91_7></div><div id=m91_8></div></div></body>", 480);
    TEST_ASSERT_EQUAL(3, lot52_node("g")->grid_cols);
    TEST_ASSERT_EQUAL(3, lot52_node("g")->grid_rows);
    TEST_ASSERT_EQUAL(0, lot52_node("m91_0")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("m91_0")->y);
    TEST_ASSERT_EQUAL(26, lot52_node("m91_0")->w);
    TEST_ASSERT_EQUAL(14, lot52_node("m91_0")->h);
    TEST_ASSERT_EQUAL(29, lot52_node("m91_1")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("m91_1")->y);
    TEST_ASSERT_EQUAL(26, lot52_node("m91_1")->w);
    TEST_ASSERT_EQUAL(14, lot52_node("m91_1")->h);
    TEST_ASSERT_EQUAL(58, lot52_node("m91_2")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("m91_2")->y);
    TEST_ASSERT_EQUAL(26, lot52_node("m91_2")->w);
    TEST_ASSERT_EQUAL(14, lot52_node("m91_2")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("m91_3")->x);
    TEST_ASSERT_EQUAL(17, lot52_node("m91_3")->y);
    TEST_ASSERT_EQUAL(26, lot52_node("m91_3")->w);
    TEST_ASSERT_EQUAL(14, lot52_node("m91_3")->h);
    TEST_ASSERT_EQUAL(29, lot52_node("m91_4")->x);
    TEST_ASSERT_EQUAL(17, lot52_node("m91_4")->y);
    TEST_ASSERT_EQUAL(26, lot52_node("m91_4")->w);
    TEST_ASSERT_EQUAL(14, lot52_node("m91_4")->h);
    TEST_ASSERT_EQUAL(58, lot52_node("m91_5")->x);
    TEST_ASSERT_EQUAL(17, lot52_node("m91_5")->y);
    TEST_ASSERT_EQUAL(26, lot52_node("m91_5")->w);
    TEST_ASSERT_EQUAL(14, lot52_node("m91_5")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("m91_6")->x);
    TEST_ASSERT_EQUAL(34, lot52_node("m91_6")->y);
    TEST_ASSERT_EQUAL(26, lot52_node("m91_6")->w);
    TEST_ASSERT_EQUAL(14, lot52_node("m91_6")->h);
    TEST_ASSERT_EQUAL(29, lot52_node("m91_7")->x);
    TEST_ASSERT_EQUAL(34, lot52_node("m91_7")->y);
    TEST_ASSERT_EQUAL(26, lot52_node("m91_7")->w);
    TEST_ASSERT_EQUAL(14, lot52_node("m91_7")->h);
    TEST_ASSERT_EQUAL(58, lot52_node("m91_8")->x);
    TEST_ASSERT_EQUAL(34, lot52_node("m91_8")->y);
    TEST_ASSERT_EQUAL(26, lot52_node("m91_8")->w);
    TEST_ASSERT_EQUAL(14, lot52_node("m91_8")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_mix_92(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:32px;grid-template-columns:repeat(2,20px);grid-template-rows:repeat(2,16px);gap:0px;align-items:stretch;justify-items:stretch'><div id=m92_0></div><div id=m92_1></div><div id=m92_2></div><div id=m92_3></div></div></body>", 480);
    TEST_ASSERT_EQUAL(2, lot52_node("g")->grid_cols);
    TEST_ASSERT_EQUAL(2, lot52_node("g")->grid_rows);
    TEST_ASSERT_EQUAL(0, lot52_node("m92_0")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("m92_0")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("m92_0")->w);
    TEST_ASSERT_EQUAL(16, lot52_node("m92_0")->h);
    TEST_ASSERT_EQUAL(20, lot52_node("m92_1")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("m92_1")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("m92_1")->w);
    TEST_ASSERT_EQUAL(16, lot52_node("m92_1")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("m92_2")->x);
    TEST_ASSERT_EQUAL(16, lot52_node("m92_2")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("m92_2")->w);
    TEST_ASSERT_EQUAL(16, lot52_node("m92_2")->h);
    TEST_ASSERT_EQUAL(20, lot52_node("m92_3")->x);
    TEST_ASSERT_EQUAL(16, lot52_node("m92_3")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("m92_3")->w);
    TEST_ASSERT_EQUAL(16, lot52_node("m92_3")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_mix_93(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:68px;height:25px;grid-template-columns:repeat(3,22px);grid-template-rows:repeat(2,12px);gap:1px;align-items:stretch;justify-items:stretch'><div id=m93_0></div><div id=m93_1></div><div id=m93_2></div><div id=m93_3></div><div id=m93_4></div><div id=m93_5></div></div></body>", 480);
    TEST_ASSERT_EQUAL(3, lot52_node("g")->grid_cols);
    TEST_ASSERT_EQUAL(2, lot52_node("g")->grid_rows);
    TEST_ASSERT_EQUAL(0, lot52_node("m93_0")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("m93_0")->y);
    TEST_ASSERT_EQUAL(22, lot52_node("m93_0")->w);
    TEST_ASSERT_EQUAL(12, lot52_node("m93_0")->h);
    TEST_ASSERT_EQUAL(23, lot52_node("m93_1")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("m93_1")->y);
    TEST_ASSERT_EQUAL(22, lot52_node("m93_1")->w);
    TEST_ASSERT_EQUAL(12, lot52_node("m93_1")->h);
    TEST_ASSERT_EQUAL(46, lot52_node("m93_2")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("m93_2")->y);
    TEST_ASSERT_EQUAL(22, lot52_node("m93_2")->w);
    TEST_ASSERT_EQUAL(12, lot52_node("m93_2")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("m93_3")->x);
    TEST_ASSERT_EQUAL(13, lot52_node("m93_3")->y);
    TEST_ASSERT_EQUAL(22, lot52_node("m93_3")->w);
    TEST_ASSERT_EQUAL(12, lot52_node("m93_3")->h);
    TEST_ASSERT_EQUAL(23, lot52_node("m93_4")->x);
    TEST_ASSERT_EQUAL(13, lot52_node("m93_4")->y);
    TEST_ASSERT_EQUAL(22, lot52_node("m93_4")->w);
    TEST_ASSERT_EQUAL(12, lot52_node("m93_4")->h);
    TEST_ASSERT_EQUAL(46, lot52_node("m93_5")->x);
    TEST_ASSERT_EQUAL(13, lot52_node("m93_5")->y);
    TEST_ASSERT_EQUAL(22, lot52_node("m93_5")->w);
    TEST_ASSERT_EQUAL(12, lot52_node("m93_5")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_mix_94(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:50px;height:46px;grid-template-columns:repeat(2,24px);grid-template-rows:repeat(3,14px);gap:2px;align-items:stretch;justify-items:stretch'><div id=m94_0></div><div id=m94_1></div><div id=m94_2></div><div id=m94_3></div><div id=m94_4></div><div id=m94_5></div></div></body>", 480);
    TEST_ASSERT_EQUAL(2, lot52_node("g")->grid_cols);
    TEST_ASSERT_EQUAL(3, lot52_node("g")->grid_rows);
    TEST_ASSERT_EQUAL(0, lot52_node("m94_0")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("m94_0")->y);
    TEST_ASSERT_EQUAL(24, lot52_node("m94_0")->w);
    TEST_ASSERT_EQUAL(14, lot52_node("m94_0")->h);
    TEST_ASSERT_EQUAL(26, lot52_node("m94_1")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("m94_1")->y);
    TEST_ASSERT_EQUAL(24, lot52_node("m94_1")->w);
    TEST_ASSERT_EQUAL(14, lot52_node("m94_1")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("m94_2")->x);
    TEST_ASSERT_EQUAL(16, lot52_node("m94_2")->y);
    TEST_ASSERT_EQUAL(24, lot52_node("m94_2")->w);
    TEST_ASSERT_EQUAL(14, lot52_node("m94_2")->h);
    TEST_ASSERT_EQUAL(26, lot52_node("m94_3")->x);
    TEST_ASSERT_EQUAL(16, lot52_node("m94_3")->y);
    TEST_ASSERT_EQUAL(24, lot52_node("m94_3")->w);
    TEST_ASSERT_EQUAL(14, lot52_node("m94_3")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("m94_4")->x);
    TEST_ASSERT_EQUAL(32, lot52_node("m94_4")->y);
    TEST_ASSERT_EQUAL(24, lot52_node("m94_4")->w);
    TEST_ASSERT_EQUAL(14, lot52_node("m94_4")->h);
    TEST_ASSERT_EQUAL(26, lot52_node("m94_5")->x);
    TEST_ASSERT_EQUAL(32, lot52_node("m94_5")->y);
    TEST_ASSERT_EQUAL(24, lot52_node("m94_5")->w);
    TEST_ASSERT_EQUAL(14, lot52_node("m94_5")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_mix_95(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:84px;height:54px;grid-template-columns:repeat(3,26px);grid-template-rows:repeat(3,16px);gap:3px;align-items:stretch;justify-items:stretch'><div id=m95_0></div><div id=m95_1></div><div id=m95_2></div><div id=m95_3></div><div id=m95_4></div><div id=m95_5></div><div id=m95_6></div><div id=m95_7></div><div id=m95_8></div></div></body>", 480);
    TEST_ASSERT_EQUAL(3, lot52_node("g")->grid_cols);
    TEST_ASSERT_EQUAL(3, lot52_node("g")->grid_rows);
    TEST_ASSERT_EQUAL(0, lot52_node("m95_0")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("m95_0")->y);
    TEST_ASSERT_EQUAL(26, lot52_node("m95_0")->w);
    TEST_ASSERT_EQUAL(16, lot52_node("m95_0")->h);
    TEST_ASSERT_EQUAL(29, lot52_node("m95_1")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("m95_1")->y);
    TEST_ASSERT_EQUAL(26, lot52_node("m95_1")->w);
    TEST_ASSERT_EQUAL(16, lot52_node("m95_1")->h);
    TEST_ASSERT_EQUAL(58, lot52_node("m95_2")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("m95_2")->y);
    TEST_ASSERT_EQUAL(26, lot52_node("m95_2")->w);
    TEST_ASSERT_EQUAL(16, lot52_node("m95_2")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("m95_3")->x);
    TEST_ASSERT_EQUAL(19, lot52_node("m95_3")->y);
    TEST_ASSERT_EQUAL(26, lot52_node("m95_3")->w);
    TEST_ASSERT_EQUAL(16, lot52_node("m95_3")->h);
    TEST_ASSERT_EQUAL(29, lot52_node("m95_4")->x);
    TEST_ASSERT_EQUAL(19, lot52_node("m95_4")->y);
    TEST_ASSERT_EQUAL(26, lot52_node("m95_4")->w);
    TEST_ASSERT_EQUAL(16, lot52_node("m95_4")->h);
    TEST_ASSERT_EQUAL(58, lot52_node("m95_5")->x);
    TEST_ASSERT_EQUAL(19, lot52_node("m95_5")->y);
    TEST_ASSERT_EQUAL(26, lot52_node("m95_5")->w);
    TEST_ASSERT_EQUAL(16, lot52_node("m95_5")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("m95_6")->x);
    TEST_ASSERT_EQUAL(38, lot52_node("m95_6")->y);
    TEST_ASSERT_EQUAL(26, lot52_node("m95_6")->w);
    TEST_ASSERT_EQUAL(16, lot52_node("m95_6")->h);
    TEST_ASSERT_EQUAL(29, lot52_node("m95_7")->x);
    TEST_ASSERT_EQUAL(38, lot52_node("m95_7")->y);
    TEST_ASSERT_EQUAL(26, lot52_node("m95_7")->w);
    TEST_ASSERT_EQUAL(16, lot52_node("m95_7")->h);
    TEST_ASSERT_EQUAL(58, lot52_node("m95_8")->x);
    TEST_ASSERT_EQUAL(38, lot52_node("m95_8")->y);
    TEST_ASSERT_EQUAL(26, lot52_node("m95_8")->w);
    TEST_ASSERT_EQUAL(16, lot52_node("m95_8")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_mix_96(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:24px;grid-template-columns:repeat(2,20px);grid-template-rows:repeat(2,12px);gap:0px;align-items:stretch;justify-items:stretch'><div id=m96_0></div><div id=m96_1></div><div id=m96_2></div><div id=m96_3></div></div></body>", 480);
    TEST_ASSERT_EQUAL(2, lot52_node("g")->grid_cols);
    TEST_ASSERT_EQUAL(2, lot52_node("g")->grid_rows);
    TEST_ASSERT_EQUAL(0, lot52_node("m96_0")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("m96_0")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("m96_0")->w);
    TEST_ASSERT_EQUAL(12, lot52_node("m96_0")->h);
    TEST_ASSERT_EQUAL(20, lot52_node("m96_1")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("m96_1")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("m96_1")->w);
    TEST_ASSERT_EQUAL(12, lot52_node("m96_1")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("m96_2")->x);
    TEST_ASSERT_EQUAL(12, lot52_node("m96_2")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("m96_2")->w);
    TEST_ASSERT_EQUAL(12, lot52_node("m96_2")->h);
    TEST_ASSERT_EQUAL(20, lot52_node("m96_3")->x);
    TEST_ASSERT_EQUAL(12, lot52_node("m96_3")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("m96_3")->w);
    TEST_ASSERT_EQUAL(12, lot52_node("m96_3")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_mix_97(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:68px;height:29px;grid-template-columns:repeat(3,22px);grid-template-rows:repeat(2,14px);gap:1px;align-items:stretch;justify-items:stretch'><div id=m97_0></div><div id=m97_1></div><div id=m97_2></div><div id=m97_3></div><div id=m97_4></div><div id=m97_5></div></div></body>", 480);
    TEST_ASSERT_EQUAL(3, lot52_node("g")->grid_cols);
    TEST_ASSERT_EQUAL(2, lot52_node("g")->grid_rows);
    TEST_ASSERT_EQUAL(0, lot52_node("m97_0")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("m97_0")->y);
    TEST_ASSERT_EQUAL(22, lot52_node("m97_0")->w);
    TEST_ASSERT_EQUAL(14, lot52_node("m97_0")->h);
    TEST_ASSERT_EQUAL(23, lot52_node("m97_1")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("m97_1")->y);
    TEST_ASSERT_EQUAL(22, lot52_node("m97_1")->w);
    TEST_ASSERT_EQUAL(14, lot52_node("m97_1")->h);
    TEST_ASSERT_EQUAL(46, lot52_node("m97_2")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("m97_2")->y);
    TEST_ASSERT_EQUAL(22, lot52_node("m97_2")->w);
    TEST_ASSERT_EQUAL(14, lot52_node("m97_2")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("m97_3")->x);
    TEST_ASSERT_EQUAL(15, lot52_node("m97_3")->y);
    TEST_ASSERT_EQUAL(22, lot52_node("m97_3")->w);
    TEST_ASSERT_EQUAL(14, lot52_node("m97_3")->h);
    TEST_ASSERT_EQUAL(23, lot52_node("m97_4")->x);
    TEST_ASSERT_EQUAL(15, lot52_node("m97_4")->y);
    TEST_ASSERT_EQUAL(22, lot52_node("m97_4")->w);
    TEST_ASSERT_EQUAL(14, lot52_node("m97_4")->h);
    TEST_ASSERT_EQUAL(46, lot52_node("m97_5")->x);
    TEST_ASSERT_EQUAL(15, lot52_node("m97_5")->y);
    TEST_ASSERT_EQUAL(22, lot52_node("m97_5")->w);
    TEST_ASSERT_EQUAL(14, lot52_node("m97_5")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_mix_98(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:50px;height:52px;grid-template-columns:repeat(2,24px);grid-template-rows:repeat(3,16px);gap:2px;align-items:stretch;justify-items:stretch'><div id=m98_0></div><div id=m98_1></div><div id=m98_2></div><div id=m98_3></div><div id=m98_4></div><div id=m98_5></div></div></body>", 480);
    TEST_ASSERT_EQUAL(2, lot52_node("g")->grid_cols);
    TEST_ASSERT_EQUAL(3, lot52_node("g")->grid_rows);
    TEST_ASSERT_EQUAL(0, lot52_node("m98_0")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("m98_0")->y);
    TEST_ASSERT_EQUAL(24, lot52_node("m98_0")->w);
    TEST_ASSERT_EQUAL(16, lot52_node("m98_0")->h);
    TEST_ASSERT_EQUAL(26, lot52_node("m98_1")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("m98_1")->y);
    TEST_ASSERT_EQUAL(24, lot52_node("m98_1")->w);
    TEST_ASSERT_EQUAL(16, lot52_node("m98_1")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("m98_2")->x);
    TEST_ASSERT_EQUAL(18, lot52_node("m98_2")->y);
    TEST_ASSERT_EQUAL(24, lot52_node("m98_2")->w);
    TEST_ASSERT_EQUAL(16, lot52_node("m98_2")->h);
    TEST_ASSERT_EQUAL(26, lot52_node("m98_3")->x);
    TEST_ASSERT_EQUAL(18, lot52_node("m98_3")->y);
    TEST_ASSERT_EQUAL(24, lot52_node("m98_3")->w);
    TEST_ASSERT_EQUAL(16, lot52_node("m98_3")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("m98_4")->x);
    TEST_ASSERT_EQUAL(36, lot52_node("m98_4")->y);
    TEST_ASSERT_EQUAL(24, lot52_node("m98_4")->w);
    TEST_ASSERT_EQUAL(16, lot52_node("m98_4")->h);
    TEST_ASSERT_EQUAL(26, lot52_node("m98_5")->x);
    TEST_ASSERT_EQUAL(36, lot52_node("m98_5")->y);
    TEST_ASSERT_EQUAL(24, lot52_node("m98_5")->w);
    TEST_ASSERT_EQUAL(16, lot52_node("m98_5")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_mix_99(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:84px;height:42px;grid-template-columns:repeat(3,26px);grid-template-rows:repeat(3,12px);gap:3px;align-items:stretch;justify-items:stretch'><div id=m99_0></div><div id=m99_1></div><div id=m99_2></div><div id=m99_3></div><div id=m99_4></div><div id=m99_5></div><div id=m99_6></div><div id=m99_7></div><div id=m99_8></div></div></body>", 480);
    TEST_ASSERT_EQUAL(3, lot52_node("g")->grid_cols);
    TEST_ASSERT_EQUAL(3, lot52_node("g")->grid_rows);
    TEST_ASSERT_EQUAL(0, lot52_node("m99_0")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("m99_0")->y);
    TEST_ASSERT_EQUAL(26, lot52_node("m99_0")->w);
    TEST_ASSERT_EQUAL(12, lot52_node("m99_0")->h);
    TEST_ASSERT_EQUAL(29, lot52_node("m99_1")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("m99_1")->y);
    TEST_ASSERT_EQUAL(26, lot52_node("m99_1")->w);
    TEST_ASSERT_EQUAL(12, lot52_node("m99_1")->h);
    TEST_ASSERT_EQUAL(58, lot52_node("m99_2")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("m99_2")->y);
    TEST_ASSERT_EQUAL(26, lot52_node("m99_2")->w);
    TEST_ASSERT_EQUAL(12, lot52_node("m99_2")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("m99_3")->x);
    TEST_ASSERT_EQUAL(15, lot52_node("m99_3")->y);
    TEST_ASSERT_EQUAL(26, lot52_node("m99_3")->w);
    TEST_ASSERT_EQUAL(12, lot52_node("m99_3")->h);
    TEST_ASSERT_EQUAL(29, lot52_node("m99_4")->x);
    TEST_ASSERT_EQUAL(15, lot52_node("m99_4")->y);
    TEST_ASSERT_EQUAL(26, lot52_node("m99_4")->w);
    TEST_ASSERT_EQUAL(12, lot52_node("m99_4")->h);
    TEST_ASSERT_EQUAL(58, lot52_node("m99_5")->x);
    TEST_ASSERT_EQUAL(15, lot52_node("m99_5")->y);
    TEST_ASSERT_EQUAL(26, lot52_node("m99_5")->w);
    TEST_ASSERT_EQUAL(12, lot52_node("m99_5")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("m99_6")->x);
    TEST_ASSERT_EQUAL(30, lot52_node("m99_6")->y);
    TEST_ASSERT_EQUAL(26, lot52_node("m99_6")->w);
    TEST_ASSERT_EQUAL(12, lot52_node("m99_6")->h);
    TEST_ASSERT_EQUAL(29, lot52_node("m99_7")->x);
    TEST_ASSERT_EQUAL(30, lot52_node("m99_7")->y);
    TEST_ASSERT_EQUAL(26, lot52_node("m99_7")->w);
    TEST_ASSERT_EQUAL(12, lot52_node("m99_7")->h);
    TEST_ASSERT_EQUAL(58, lot52_node("m99_8")->x);
    TEST_ASSERT_EQUAL(30, lot52_node("m99_8")->y);
    TEST_ASSERT_EQUAL(26, lot52_node("m99_8")->w);
    TEST_ASSERT_EQUAL(12, lot52_node("m99_8")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_mix_100(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:28px;grid-template-columns:repeat(2,20px);grid-template-rows:repeat(2,14px);gap:0px;align-items:stretch;justify-items:stretch'><div id=m100_0></div><div id=m100_1></div><div id=m100_2></div><div id=m100_3></div></div></body>", 480);
    TEST_ASSERT_EQUAL(2, lot52_node("g")->grid_cols);
    TEST_ASSERT_EQUAL(2, lot52_node("g")->grid_rows);
    TEST_ASSERT_EQUAL(0, lot52_node("m100_0")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("m100_0")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("m100_0")->w);
    TEST_ASSERT_EQUAL(14, lot52_node("m100_0")->h);
    TEST_ASSERT_EQUAL(20, lot52_node("m100_1")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("m100_1")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("m100_1")->w);
    TEST_ASSERT_EQUAL(14, lot52_node("m100_1")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("m100_2")->x);
    TEST_ASSERT_EQUAL(14, lot52_node("m100_2")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("m100_2")->w);
    TEST_ASSERT_EQUAL(14, lot52_node("m100_2")->h);
    TEST_ASSERT_EQUAL(20, lot52_node("m100_3")->x);
    TEST_ASSERT_EQUAL(14, lot52_node("m100_3")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("m100_3")->w);
    TEST_ASSERT_EQUAL(14, lot52_node("m100_3")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_place_content_sys_1(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:50px;grid-template-columns:40px;grid-template-rows:10px 10px;place-content:space-between start'><div id=a1></div><div id=b1></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("a1")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("b1")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_place_content_sys_2(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:50px;grid-template-columns:40px;grid-template-rows:10px 10px;place-content:space-between start'><div id=a2></div><div id=b2></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("a2")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("b2")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_place_content_sys_3(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:50px;grid-template-columns:40px;grid-template-rows:10px 10px;place-content:space-between start'><div id=a3></div><div id=b3></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("a3")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("b3")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_place_content_sys_4(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:50px;grid-template-columns:40px;grid-template-rows:10px 10px;place-content:space-between start'><div id=a4></div><div id=b4></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("a4")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("b4")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_place_content_sys_5(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:50px;grid-template-columns:40px;grid-template-rows:10px 10px;place-content:space-between start'><div id=a5></div><div id=b5></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("a5")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("b5")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_place_content_sys_6(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:50px;grid-template-columns:40px;grid-template-rows:10px 10px;place-content:space-between start'><div id=a6></div><div id=b6></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("a6")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("b6")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_place_content_sys_7(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:50px;grid-template-columns:40px;grid-template-rows:10px 10px;place-content:space-between start'><div id=a7></div><div id=b7></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("a7")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("b7")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_place_content_sys_8(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:50px;grid-template-columns:40px;grid-template-rows:10px 10px;place-content:space-between start'><div id=a8></div><div id=b8></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("a8")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("b8")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_place_content_sys_9(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:50px;grid-template-columns:40px;grid-template-rows:10px 10px;place-content:space-between start'><div id=a9></div><div id=b9></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("a9")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("b9")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_place_content_sys_10(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:50px;grid-template-columns:40px;grid-template-rows:10px 10px;place-content:space-between start'><div id=a10></div><div id=b10></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("a10")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("b10")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_place_content_sys_11(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:50px;grid-template-columns:40px;grid-template-rows:10px 10px;place-content:space-between start'><div id=a11></div><div id=b11></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("a11")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("b11")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_place_content_sys_12(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:50px;grid-template-columns:40px;grid-template-rows:10px 10px;place-content:space-between start'><div id=a12></div><div id=b12></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("a12")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("b12")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_place_content_sys_13(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:50px;grid-template-columns:40px;grid-template-rows:10px 10px;place-content:space-between start'><div id=a13></div><div id=b13></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("a13")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("b13")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_place_content_sys_14(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:50px;grid-template-columns:40px;grid-template-rows:10px 10px;place-content:space-between start'><div id=a14></div><div id=b14></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("a14")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("b14")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_place_content_sys_15(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:50px;grid-template-columns:40px;grid-template-rows:10px 10px;place-content:space-between start'><div id=a15></div><div id=b15></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("a15")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("b15")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_place_content_sys_16(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:50px;grid-template-columns:40px;grid-template-rows:10px 10px;place-content:space-between start'><div id=a16></div><div id=b16></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("a16")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("b16")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_place_content_sys_17(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:50px;grid-template-columns:40px;grid-template-rows:10px 10px;place-content:space-between start'><div id=a17></div><div id=b17></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("a17")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("b17")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_place_content_sys_18(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:50px;grid-template-columns:40px;grid-template-rows:10px 10px;place-content:space-between start'><div id=a18></div><div id=b18></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("a18")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("b18")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_place_content_sys_19(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:50px;grid-template-columns:40px;grid-template-rows:10px 10px;place-content:space-between start'><div id=a19></div><div id=b19></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("a19")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("b19")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_place_content_sys_20(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:50px;grid-template-columns:40px;grid-template-rows:10px 10px;place-content:space-between start'><div id=a20></div><div id=b20></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("a20")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("b20")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_place_content_sys_21(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:50px;grid-template-columns:40px;grid-template-rows:10px 10px;place-content:space-between start'><div id=a21></div><div id=b21></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("a21")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("b21")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_place_content_sys_22(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:50px;grid-template-columns:40px;grid-template-rows:10px 10px;place-content:space-between start'><div id=a22></div><div id=b22></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("a22")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("b22")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_place_content_sys_23(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:50px;grid-template-columns:40px;grid-template-rows:10px 10px;place-content:space-between start'><div id=a23></div><div id=b23></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("a23")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("b23")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_place_content_sys_24(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:50px;grid-template-columns:40px;grid-template-rows:10px 10px;place-content:space-between start'><div id=a24></div><div id=b24></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("a24")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("b24")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_place_content_sys_25(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:50px;grid-template-columns:40px;grid-template-rows:10px 10px;place-content:space-between start'><div id=a25></div><div id=b25></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("a25")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("b25")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_place_content_sys_26(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:50px;grid-template-columns:40px;grid-template-rows:10px 10px;place-content:space-between start'><div id=a26></div><div id=b26></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("a26")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("b26")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_place_content_sys_27(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:50px;grid-template-columns:40px;grid-template-rows:10px 10px;place-content:space-between start'><div id=a27></div><div id=b27></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("a27")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("b27")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_place_content_sys_28(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:50px;grid-template-columns:40px;grid-template-rows:10px 10px;place-content:space-between start'><div id=a28></div><div id=b28></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("a28")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("b28")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_place_content_sys_29(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:50px;grid-template-columns:40px;grid-template-rows:10px 10px;place-content:space-between start'><div id=a29></div><div id=b29></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("a29")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("b29")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_place_content_sys_30(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:50px;grid-template-columns:40px;grid-template-rows:10px 10px;place-content:space-between start'><div id=a30></div><div id=b30></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("a30")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("b30")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_place_content_sys_31(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:50px;grid-template-columns:40px;grid-template-rows:10px 10px;place-content:space-between start'><div id=a31></div><div id=b31></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("a31")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("b31")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_place_content_sys_32(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:50px;grid-template-columns:40px;grid-template-rows:10px 10px;place-content:space-between start'><div id=a32></div><div id=b32></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("a32")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("b32")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_place_content_sys_33(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:50px;grid-template-columns:40px;grid-template-rows:10px 10px;place-content:space-between start'><div id=a33></div><div id=b33></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("a33")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("b33")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_place_content_sys_34(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:50px;grid-template-columns:40px;grid-template-rows:10px 10px;place-content:space-between start'><div id=a34></div><div id=b34></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("a34")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("b34")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_place_content_sys_35(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:50px;grid-template-columns:40px;grid-template-rows:10px 10px;place-content:space-between start'><div id=a35></div><div id=b35></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("a35")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("b35")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_place_content_sys_36(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:50px;grid-template-columns:40px;grid-template-rows:10px 10px;place-content:space-between start'><div id=a36></div><div id=b36></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("a36")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("b36")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_place_content_sys_37(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:50px;grid-template-columns:40px;grid-template-rows:10px 10px;place-content:space-between start'><div id=a37></div><div id=b37></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("a37")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("b37")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_place_content_sys_38(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:50px;grid-template-columns:40px;grid-template-rows:10px 10px;place-content:space-between start'><div id=a38></div><div id=b38></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("a38")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("b38")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_place_content_sys_39(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:50px;grid-template-columns:40px;grid-template-rows:10px 10px;place-content:space-between start'><div id=a39></div><div id=b39></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("a39")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("b39")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_place_content_sys_40(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:50px;grid-template-columns:40px;grid-template-rows:10px 10px;place-content:space-between start'><div id=a40></div><div id=b40></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("a40")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("b40")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_place_content_sys_41(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:50px;grid-template-columns:40px;grid-template-rows:10px 10px;place-content:space-between start'><div id=a41></div><div id=b41></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("a41")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("b41")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_place_content_sys_42(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:50px;grid-template-columns:40px;grid-template-rows:10px 10px;place-content:space-between start'><div id=a42></div><div id=b42></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("a42")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("b42")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_place_content_sys_43(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:50px;grid-template-columns:40px;grid-template-rows:10px 10px;place-content:space-between start'><div id=a43></div><div id=b43></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("a43")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("b43")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_place_content_sys_44(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:50px;grid-template-columns:40px;grid-template-rows:10px 10px;place-content:space-between start'><div id=a44></div><div id=b44></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("a44")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("b44")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_place_content_sys_45(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:50px;grid-template-columns:40px;grid-template-rows:10px 10px;place-content:space-between start'><div id=a45></div><div id=b45></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("a45")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("b45")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_place_content_sys_46(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:50px;grid-template-columns:40px;grid-template-rows:10px 10px;place-content:space-between start'><div id=a46></div><div id=b46></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("a46")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("b46")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_place_content_sys_47(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:50px;grid-template-columns:40px;grid-template-rows:10px 10px;place-content:space-between start'><div id=a47></div><div id=b47></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("a47")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("b47")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_place_content_sys_48(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:50px;grid-template-columns:40px;grid-template-rows:10px 10px;place-content:space-between start'><div id=a48></div><div id=b48></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("a48")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("b48")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_place_content_sys_49(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:50px;grid-template-columns:40px;grid-template-rows:10px 10px;place-content:space-between start'><div id=a49></div><div id=b49></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("a49")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("b49")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_place_content_sys_50(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:50px;grid-template-columns:40px;grid-template-rows:10px 10px;place-content:space-between start'><div id=a50></div><div id=b50></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("a50")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("b50")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

/* Lot 78: explicit grid-column/grid-row line placement. */
static void test_l78_xy_place_1(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:98px;height:68px;grid-template-columns:30px 30px 30px;grid-template-rows:20px 20px 20px;gap:4px;align-items:stretch'><div id=c1 style='grid-column:1 / 2;grid-row:1 / 2'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("c1")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("c1")->y);
    TEST_ASSERT_EQUAL(30, lot52_node("c1")->w);
    TEST_ASSERT_EQUAL(20, lot52_node("c1")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_xy_place_2(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:98px;height:68px;grid-template-columns:30px 30px 30px;grid-template-rows:20px 20px 20px;gap:4px;align-items:stretch'><div id=c2 style='grid-column:1 / 2;grid-row:2 / 3'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("c2")->x);
    TEST_ASSERT_EQUAL(24, lot52_node("c2")->y);
    TEST_ASSERT_EQUAL(30, lot52_node("c2")->w);
    TEST_ASSERT_EQUAL(20, lot52_node("c2")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_xy_place_3(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:98px;height:68px;grid-template-columns:30px 30px 30px;grid-template-rows:20px 20px 20px;gap:4px;align-items:stretch'><div id=c3 style='grid-column:1 / 2;grid-row:3 / 4'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("c3")->x);
    TEST_ASSERT_EQUAL(48, lot52_node("c3")->y);
    TEST_ASSERT_EQUAL(30, lot52_node("c3")->w);
    TEST_ASSERT_EQUAL(20, lot52_node("c3")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_xy_place_4(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:98px;height:68px;grid-template-columns:30px 30px 30px;grid-template-rows:20px 20px 20px;gap:4px;align-items:stretch'><div id=c4 style='grid-column:2 / 3;grid-row:1 / 2'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(34, lot52_node("c4")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("c4")->y);
    TEST_ASSERT_EQUAL(30, lot52_node("c4")->w);
    TEST_ASSERT_EQUAL(20, lot52_node("c4")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_xy_place_5(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:98px;height:68px;grid-template-columns:30px 30px 30px;grid-template-rows:20px 20px 20px;gap:4px;align-items:stretch'><div id=c5 style='grid-column:2 / 3;grid-row:2 / 3'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(34, lot52_node("c5")->x);
    TEST_ASSERT_EQUAL(24, lot52_node("c5")->y);
    TEST_ASSERT_EQUAL(30, lot52_node("c5")->w);
    TEST_ASSERT_EQUAL(20, lot52_node("c5")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_xy_place_6(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:98px;height:68px;grid-template-columns:30px 30px 30px;grid-template-rows:20px 20px 20px;gap:4px;align-items:stretch'><div id=c6 style='grid-column:2 / 3;grid-row:3 / 4'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(34, lot52_node("c6")->x);
    TEST_ASSERT_EQUAL(48, lot52_node("c6")->y);
    TEST_ASSERT_EQUAL(30, lot52_node("c6")->w);
    TEST_ASSERT_EQUAL(20, lot52_node("c6")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_xy_place_7(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:98px;height:68px;grid-template-columns:30px 30px 30px;grid-template-rows:20px 20px 20px;gap:4px;align-items:stretch'><div id=c7 style='grid-column:3 / 4;grid-row:1 / 2'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(68, lot52_node("c7")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("c7")->y);
    TEST_ASSERT_EQUAL(30, lot52_node("c7")->w);
    TEST_ASSERT_EQUAL(20, lot52_node("c7")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_xy_place_8(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:98px;height:68px;grid-template-columns:30px 30px 30px;grid-template-rows:20px 20px 20px;gap:4px;align-items:stretch'><div id=c8 style='grid-column:3 / 4;grid-row:2 / 3'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(68, lot52_node("c8")->x);
    TEST_ASSERT_EQUAL(24, lot52_node("c8")->y);
    TEST_ASSERT_EQUAL(30, lot52_node("c8")->w);
    TEST_ASSERT_EQUAL(20, lot52_node("c8")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_xy_place_9(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:98px;height:68px;grid-template-columns:30px 30px 30px;grid-template-rows:20px 20px 20px;gap:4px;align-items:stretch'><div id=c9 style='grid-column:3 / 4;grid-row:3 / 4'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(68, lot52_node("c9")->x);
    TEST_ASSERT_EQUAL(48, lot52_node("c9")->y);
    TEST_ASSERT_EQUAL(30, lot52_node("c9")->w);
    TEST_ASSERT_EQUAL(20, lot52_node("c9")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_xy_place_10(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:94px;height:64px;grid-template-columns:30px 30px 30px;grid-template-rows:20px 20px 20px;gap:2px;align-items:stretch'><div id=c10 style='grid-column:1 / 2;grid-row:1 / 2'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("c10")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("c10")->y);
    TEST_ASSERT_EQUAL(30, lot52_node("c10")->w);
    TEST_ASSERT_EQUAL(20, lot52_node("c10")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_xy_place_11(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:94px;height:64px;grid-template-columns:30px 30px 30px;grid-template-rows:20px 20px 20px;gap:2px;align-items:stretch'><div id=c11 style='grid-column:1 / 2;grid-row:2 / 3'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("c11")->x);
    TEST_ASSERT_EQUAL(22, lot52_node("c11")->y);
    TEST_ASSERT_EQUAL(30, lot52_node("c11")->w);
    TEST_ASSERT_EQUAL(20, lot52_node("c11")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_xy_place_12(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:94px;height:64px;grid-template-columns:30px 30px 30px;grid-template-rows:20px 20px 20px;gap:2px;align-items:stretch'><div id=c12 style='grid-column:1 / 2;grid-row:3 / 4'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("c12")->x);
    TEST_ASSERT_EQUAL(44, lot52_node("c12")->y);
    TEST_ASSERT_EQUAL(30, lot52_node("c12")->w);
    TEST_ASSERT_EQUAL(20, lot52_node("c12")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_xy_place_13(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:94px;height:64px;grid-template-columns:30px 30px 30px;grid-template-rows:20px 20px 20px;gap:2px;align-items:stretch'><div id=c13 style='grid-column:2 / 3;grid-row:1 / 2'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(32, lot52_node("c13")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("c13")->y);
    TEST_ASSERT_EQUAL(30, lot52_node("c13")->w);
    TEST_ASSERT_EQUAL(20, lot52_node("c13")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_xy_place_14(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:94px;height:64px;grid-template-columns:30px 30px 30px;grid-template-rows:20px 20px 20px;gap:2px;align-items:stretch'><div id=c14 style='grid-column:2 / 3;grid-row:2 / 3'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(32, lot52_node("c14")->x);
    TEST_ASSERT_EQUAL(22, lot52_node("c14")->y);
    TEST_ASSERT_EQUAL(30, lot52_node("c14")->w);
    TEST_ASSERT_EQUAL(20, lot52_node("c14")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_xy_place_15(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:94px;height:64px;grid-template-columns:30px 30px 30px;grid-template-rows:20px 20px 20px;gap:2px;align-items:stretch'><div id=c15 style='grid-column:2 / 3;grid-row:3 / 4'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(32, lot52_node("c15")->x);
    TEST_ASSERT_EQUAL(44, lot52_node("c15")->y);
    TEST_ASSERT_EQUAL(30, lot52_node("c15")->w);
    TEST_ASSERT_EQUAL(20, lot52_node("c15")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_xy_place_16(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:94px;height:64px;grid-template-columns:30px 30px 30px;grid-template-rows:20px 20px 20px;gap:2px;align-items:stretch'><div id=c16 style='grid-column:3 / 4;grid-row:1 / 2'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(64, lot52_node("c16")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("c16")->y);
    TEST_ASSERT_EQUAL(30, lot52_node("c16")->w);
    TEST_ASSERT_EQUAL(20, lot52_node("c16")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_xy_place_17(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:94px;height:64px;grid-template-columns:30px 30px 30px;grid-template-rows:20px 20px 20px;gap:2px;align-items:stretch'><div id=c17 style='grid-column:3 / 4;grid-row:2 / 3'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(64, lot52_node("c17")->x);
    TEST_ASSERT_EQUAL(22, lot52_node("c17")->y);
    TEST_ASSERT_EQUAL(30, lot52_node("c17")->w);
    TEST_ASSERT_EQUAL(20, lot52_node("c17")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_xy_place_18(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:94px;height:64px;grid-template-columns:30px 30px 30px;grid-template-rows:20px 20px 20px;gap:2px;align-items:stretch'><div id=c18 style='grid-column:3 / 4;grid-row:3 / 4'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(64, lot52_node("c18")->x);
    TEST_ASSERT_EQUAL(44, lot52_node("c18")->y);
    TEST_ASSERT_EQUAL(30, lot52_node("c18")->w);
    TEST_ASSERT_EQUAL(20, lot52_node("c18")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_xy_place_19(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:87px;height:66px;grid-template-columns:25px 25px 25px;grid-template-rows:18px 18px 18px;gap:6px;align-items:stretch'><div id=c19 style='grid-column:1 / 2;grid-row:1 / 2'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("c19")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("c19")->y);
    TEST_ASSERT_EQUAL(25, lot52_node("c19")->w);
    TEST_ASSERT_EQUAL(18, lot52_node("c19")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_xy_place_20(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:87px;height:66px;grid-template-columns:25px 25px 25px;grid-template-rows:18px 18px 18px;gap:6px;align-items:stretch'><div id=c20 style='grid-column:1 / 2;grid-row:2 / 3'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("c20")->x);
    TEST_ASSERT_EQUAL(24, lot52_node("c20")->y);
    TEST_ASSERT_EQUAL(25, lot52_node("c20")->w);
    TEST_ASSERT_EQUAL(18, lot52_node("c20")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_xy_place_21(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:87px;height:66px;grid-template-columns:25px 25px 25px;grid-template-rows:18px 18px 18px;gap:6px;align-items:stretch'><div id=c21 style='grid-column:1 / 2;grid-row:3 / 4'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("c21")->x);
    TEST_ASSERT_EQUAL(48, lot52_node("c21")->y);
    TEST_ASSERT_EQUAL(25, lot52_node("c21")->w);
    TEST_ASSERT_EQUAL(18, lot52_node("c21")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_xy_place_22(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:87px;height:66px;grid-template-columns:25px 25px 25px;grid-template-rows:18px 18px 18px;gap:6px;align-items:stretch'><div id=c22 style='grid-column:2 / 3;grid-row:1 / 2'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(31, lot52_node("c22")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("c22")->y);
    TEST_ASSERT_EQUAL(25, lot52_node("c22")->w);
    TEST_ASSERT_EQUAL(18, lot52_node("c22")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_xy_place_23(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:87px;height:66px;grid-template-columns:25px 25px 25px;grid-template-rows:18px 18px 18px;gap:6px;align-items:stretch'><div id=c23 style='grid-column:2 / 3;grid-row:2 / 3'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(31, lot52_node("c23")->x);
    TEST_ASSERT_EQUAL(24, lot52_node("c23")->y);
    TEST_ASSERT_EQUAL(25, lot52_node("c23")->w);
    TEST_ASSERT_EQUAL(18, lot52_node("c23")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_xy_place_24(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:87px;height:66px;grid-template-columns:25px 25px 25px;grid-template-rows:18px 18px 18px;gap:6px;align-items:stretch'><div id=c24 style='grid-column:2 / 3;grid-row:3 / 4'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(31, lot52_node("c24")->x);
    TEST_ASSERT_EQUAL(48, lot52_node("c24")->y);
    TEST_ASSERT_EQUAL(25, lot52_node("c24")->w);
    TEST_ASSERT_EQUAL(18, lot52_node("c24")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_xy_place_25(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:98px;height:68px;grid-template-columns:30px 30px 30px;grid-template-rows:20px 20px 20px;gap:4px;align-items:stretch'><div id=c25 style='grid-column:3 / 4;grid-row:1 / 2'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(68, lot52_node("c25")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("c25")->y);
    TEST_ASSERT_EQUAL(30, lot52_node("c25")->w);
    TEST_ASSERT_EQUAL(20, lot52_node("c25")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_xy_place_26(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:98px;height:68px;grid-template-columns:30px 30px 30px;grid-template-rows:20px 20px 20px;gap:4px;align-items:stretch'><div id=c26 style='grid-column:3 / 4;grid-row:2 / 3'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(68, lot52_node("c26")->x);
    TEST_ASSERT_EQUAL(24, lot52_node("c26")->y);
    TEST_ASSERT_EQUAL(30, lot52_node("c26")->w);
    TEST_ASSERT_EQUAL(20, lot52_node("c26")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_xy_place_27(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:98px;height:68px;grid-template-columns:30px 30px 30px;grid-template-rows:20px 20px 20px;gap:4px;align-items:stretch'><div id=c27 style='grid-column:3 / 4;grid-row:3 / 4'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(68, lot52_node("c27")->x);
    TEST_ASSERT_EQUAL(48, lot52_node("c27")->y);
    TEST_ASSERT_EQUAL(30, lot52_node("c27")->w);
    TEST_ASSERT_EQUAL(20, lot52_node("c27")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_xy_place_28(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:98px;height:68px;grid-template-columns:30px 30px 30px;grid-template-rows:20px 20px 20px;gap:4px;align-items:stretch'><div id=c28 style='grid-column:1 / 2;grid-row:1 / 2'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("c28")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("c28")->y);
    TEST_ASSERT_EQUAL(30, lot52_node("c28")->w);
    TEST_ASSERT_EQUAL(20, lot52_node("c28")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_xy_place_29(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:98px;height:68px;grid-template-columns:30px 30px 30px;grid-template-rows:20px 20px 20px;gap:4px;align-items:stretch'><div id=c29 style='grid-column:1 / 2;grid-row:2 / 3'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("c29")->x);
    TEST_ASSERT_EQUAL(24, lot52_node("c29")->y);
    TEST_ASSERT_EQUAL(30, lot52_node("c29")->w);
    TEST_ASSERT_EQUAL(20, lot52_node("c29")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_xy_place_30(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:98px;height:68px;grid-template-columns:30px 30px 30px;grid-template-rows:20px 20px 20px;gap:4px;align-items:stretch'><div id=c30 style='grid-column:1 / 2;grid-row:3 / 4'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("c30")->x);
    TEST_ASSERT_EQUAL(48, lot52_node("c30")->y);
    TEST_ASSERT_EQUAL(30, lot52_node("c30")->w);
    TEST_ASSERT_EQUAL(20, lot52_node("c30")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_xy_place_31(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:98px;height:68px;grid-template-columns:30px 30px 30px;grid-template-rows:20px 20px 20px;gap:4px;align-items:stretch'><div id=c31 style='grid-column:2 / 3;grid-row:1 / 2'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(34, lot52_node("c31")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("c31")->y);
    TEST_ASSERT_EQUAL(30, lot52_node("c31")->w);
    TEST_ASSERT_EQUAL(20, lot52_node("c31")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_xy_place_32(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:98px;height:68px;grid-template-columns:30px 30px 30px;grid-template-rows:20px 20px 20px;gap:4px;align-items:stretch'><div id=c32 style='grid-column:2 / 3;grid-row:2 / 3'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(34, lot52_node("c32")->x);
    TEST_ASSERT_EQUAL(24, lot52_node("c32")->y);
    TEST_ASSERT_EQUAL(30, lot52_node("c32")->w);
    TEST_ASSERT_EQUAL(20, lot52_node("c32")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_xy_place_33(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:98px;height:68px;grid-template-columns:30px 30px 30px;grid-template-rows:20px 20px 20px;gap:4px;align-items:stretch'><div id=c33 style='grid-column:2 / 3;grid-row:3 / 4'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(34, lot52_node("c33")->x);
    TEST_ASSERT_EQUAL(48, lot52_node("c33")->y);
    TEST_ASSERT_EQUAL(30, lot52_node("c33")->w);
    TEST_ASSERT_EQUAL(20, lot52_node("c33")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_xy_place_34(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:98px;height:68px;grid-template-columns:30px 30px 30px;grid-template-rows:20px 20px 20px;gap:4px;align-items:stretch'><div id=c34 style='grid-column:3 / 4;grid-row:1 / 2'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(68, lot52_node("c34")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("c34")->y);
    TEST_ASSERT_EQUAL(30, lot52_node("c34")->w);
    TEST_ASSERT_EQUAL(20, lot52_node("c34")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_xy_place_35(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:98px;height:68px;grid-template-columns:30px 30px 30px;grid-template-rows:20px 20px 20px;gap:4px;align-items:stretch'><div id=c35 style='grid-column:3 / 4;grid-row:2 / 3'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(68, lot52_node("c35")->x);
    TEST_ASSERT_EQUAL(24, lot52_node("c35")->y);
    TEST_ASSERT_EQUAL(30, lot52_node("c35")->w);
    TEST_ASSERT_EQUAL(20, lot52_node("c35")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_xy_place_36(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:98px;height:68px;grid-template-columns:30px 30px 30px;grid-template-rows:20px 20px 20px;gap:4px;align-items:stretch'><div id=c36 style='grid-column:3 / 4;grid-row:3 / 4'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(68, lot52_node("c36")->x);
    TEST_ASSERT_EQUAL(48, lot52_node("c36")->y);
    TEST_ASSERT_EQUAL(30, lot52_node("c36")->w);
    TEST_ASSERT_EQUAL(20, lot52_node("c36")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_xy_place_37(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:98px;height:68px;grid-template-columns:30px 30px 30px;grid-template-rows:20px 20px 20px;gap:4px;align-items:stretch'><div id=c37 style='grid-column:1 / 2;grid-row:1 / 2'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("c37")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("c37")->y);
    TEST_ASSERT_EQUAL(30, lot52_node("c37")->w);
    TEST_ASSERT_EQUAL(20, lot52_node("c37")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_xy_place_38(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:98px;height:68px;grid-template-columns:30px 30px 30px;grid-template-rows:20px 20px 20px;gap:4px;align-items:stretch'><div id=c38 style='grid-column:1 / 2;grid-row:2 / 3'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("c38")->x);
    TEST_ASSERT_EQUAL(24, lot52_node("c38")->y);
    TEST_ASSERT_EQUAL(30, lot52_node("c38")->w);
    TEST_ASSERT_EQUAL(20, lot52_node("c38")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_xy_place_39(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:98px;height:68px;grid-template-columns:30px 30px 30px;grid-template-rows:20px 20px 20px;gap:4px;align-items:stretch'><div id=c39 style='grid-column:1 / 2;grid-row:3 / 4'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("c39")->x);
    TEST_ASSERT_EQUAL(48, lot52_node("c39")->y);
    TEST_ASSERT_EQUAL(30, lot52_node("c39")->w);
    TEST_ASSERT_EQUAL(20, lot52_node("c39")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_xy_place_40(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:98px;height:68px;grid-template-columns:30px 30px 30px;grid-template-rows:20px 20px 20px;gap:4px;align-items:stretch'><div id=c40 style='grid-column:2 / 3;grid-row:1 / 2'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(34, lot52_node("c40")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("c40")->y);
    TEST_ASSERT_EQUAL(30, lot52_node("c40")->w);
    TEST_ASSERT_EQUAL(20, lot52_node("c40")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void lot78_more_run(void) {
    RUN_TEST(test_l78_mix_1);
    RUN_TEST(test_l78_mix_2);
    RUN_TEST(test_l78_mix_3);
    RUN_TEST(test_l78_mix_4);
    RUN_TEST(test_l78_mix_5);
    RUN_TEST(test_l78_mix_6);
    RUN_TEST(test_l78_mix_7);
    RUN_TEST(test_l78_mix_8);
    RUN_TEST(test_l78_mix_9);
    RUN_TEST(test_l78_mix_10);
    RUN_TEST(test_l78_mix_11);
    RUN_TEST(test_l78_mix_12);
    RUN_TEST(test_l78_mix_13);
    RUN_TEST(test_l78_mix_14);
    RUN_TEST(test_l78_mix_15);
    RUN_TEST(test_l78_mix_16);
    RUN_TEST(test_l78_mix_17);
    RUN_TEST(test_l78_mix_18);
    RUN_TEST(test_l78_mix_19);
    RUN_TEST(test_l78_mix_20);
    RUN_TEST(test_l78_mix_21);
    RUN_TEST(test_l78_mix_22);
    RUN_TEST(test_l78_mix_23);
    RUN_TEST(test_l78_mix_24);
    RUN_TEST(test_l78_mix_25);
    RUN_TEST(test_l78_mix_26);
    RUN_TEST(test_l78_mix_27);
    RUN_TEST(test_l78_mix_28);
    RUN_TEST(test_l78_mix_29);
    RUN_TEST(test_l78_mix_30);
    RUN_TEST(test_l78_mix_31);
    RUN_TEST(test_l78_mix_32);
    RUN_TEST(test_l78_mix_33);
    RUN_TEST(test_l78_mix_34);
    RUN_TEST(test_l78_mix_35);
    RUN_TEST(test_l78_mix_36);
    RUN_TEST(test_l78_mix_37);
    RUN_TEST(test_l78_mix_38);
    RUN_TEST(test_l78_mix_39);
    RUN_TEST(test_l78_mix_40);
    RUN_TEST(test_l78_mix_41);
    RUN_TEST(test_l78_mix_42);
    RUN_TEST(test_l78_mix_43);
    RUN_TEST(test_l78_mix_44);
    RUN_TEST(test_l78_mix_45);
    RUN_TEST(test_l78_mix_46);
    RUN_TEST(test_l78_mix_47);
    RUN_TEST(test_l78_mix_48);
    RUN_TEST(test_l78_mix_49);
    RUN_TEST(test_l78_mix_50);
    RUN_TEST(test_l78_mix_51);
    RUN_TEST(test_l78_mix_52);
    RUN_TEST(test_l78_mix_53);
    RUN_TEST(test_l78_mix_54);
    RUN_TEST(test_l78_mix_55);
    RUN_TEST(test_l78_mix_56);
    RUN_TEST(test_l78_mix_57);
    RUN_TEST(test_l78_mix_58);
    RUN_TEST(test_l78_mix_59);
    RUN_TEST(test_l78_mix_60);
    RUN_TEST(test_l78_mix_61);
    RUN_TEST(test_l78_mix_62);
    RUN_TEST(test_l78_mix_63);
    RUN_TEST(test_l78_mix_64);
    RUN_TEST(test_l78_mix_65);
    RUN_TEST(test_l78_mix_66);
    RUN_TEST(test_l78_mix_67);
    RUN_TEST(test_l78_mix_68);
    RUN_TEST(test_l78_mix_69);
    RUN_TEST(test_l78_mix_70);
    RUN_TEST(test_l78_mix_71);
    RUN_TEST(test_l78_mix_72);
    RUN_TEST(test_l78_mix_73);
    RUN_TEST(test_l78_mix_74);
    RUN_TEST(test_l78_mix_75);
    RUN_TEST(test_l78_mix_76);
    RUN_TEST(test_l78_mix_77);
    RUN_TEST(test_l78_mix_78);
    RUN_TEST(test_l78_mix_79);
    RUN_TEST(test_l78_mix_80);
    RUN_TEST(test_l78_mix_81);
    RUN_TEST(test_l78_mix_82);
    RUN_TEST(test_l78_mix_83);
    RUN_TEST(test_l78_mix_84);
    RUN_TEST(test_l78_mix_85);
    RUN_TEST(test_l78_mix_86);
    RUN_TEST(test_l78_mix_87);
    RUN_TEST(test_l78_mix_88);
    RUN_TEST(test_l78_mix_89);
    RUN_TEST(test_l78_mix_90);
    RUN_TEST(test_l78_mix_91);
    RUN_TEST(test_l78_mix_92);
    RUN_TEST(test_l78_mix_93);
    RUN_TEST(test_l78_mix_94);
    RUN_TEST(test_l78_mix_95);
    RUN_TEST(test_l78_mix_96);
    RUN_TEST(test_l78_mix_97);
    RUN_TEST(test_l78_mix_98);
    RUN_TEST(test_l78_mix_99);
    RUN_TEST(test_l78_mix_100);
    RUN_TEST(test_l78_place_content_sys_1);
    RUN_TEST(test_l78_place_content_sys_2);
    RUN_TEST(test_l78_place_content_sys_3);
    RUN_TEST(test_l78_place_content_sys_4);
    RUN_TEST(test_l78_place_content_sys_5);
    RUN_TEST(test_l78_place_content_sys_6);
    RUN_TEST(test_l78_place_content_sys_7);
    RUN_TEST(test_l78_place_content_sys_8);
    RUN_TEST(test_l78_place_content_sys_9);
    RUN_TEST(test_l78_place_content_sys_10);
    RUN_TEST(test_l78_place_content_sys_11);
    RUN_TEST(test_l78_place_content_sys_12);
    RUN_TEST(test_l78_place_content_sys_13);
    RUN_TEST(test_l78_place_content_sys_14);
    RUN_TEST(test_l78_place_content_sys_15);
    RUN_TEST(test_l78_place_content_sys_16);
    RUN_TEST(test_l78_place_content_sys_17);
    RUN_TEST(test_l78_place_content_sys_18);
    RUN_TEST(test_l78_place_content_sys_19);
    RUN_TEST(test_l78_place_content_sys_20);
    RUN_TEST(test_l78_place_content_sys_21);
    RUN_TEST(test_l78_place_content_sys_22);
    RUN_TEST(test_l78_place_content_sys_23);
    RUN_TEST(test_l78_place_content_sys_24);
    RUN_TEST(test_l78_place_content_sys_25);
    RUN_TEST(test_l78_place_content_sys_26);
    RUN_TEST(test_l78_place_content_sys_27);
    RUN_TEST(test_l78_place_content_sys_28);
    RUN_TEST(test_l78_place_content_sys_29);
    RUN_TEST(test_l78_place_content_sys_30);
    RUN_TEST(test_l78_place_content_sys_31);
    RUN_TEST(test_l78_place_content_sys_32);
    RUN_TEST(test_l78_place_content_sys_33);
    RUN_TEST(test_l78_place_content_sys_34);
    RUN_TEST(test_l78_place_content_sys_35);
    RUN_TEST(test_l78_place_content_sys_36);
    RUN_TEST(test_l78_place_content_sys_37);
    RUN_TEST(test_l78_place_content_sys_38);
    RUN_TEST(test_l78_place_content_sys_39);
    RUN_TEST(test_l78_place_content_sys_40);
    RUN_TEST(test_l78_place_content_sys_41);
    RUN_TEST(test_l78_place_content_sys_42);
    RUN_TEST(test_l78_place_content_sys_43);
    RUN_TEST(test_l78_place_content_sys_44);
    RUN_TEST(test_l78_place_content_sys_45);
    RUN_TEST(test_l78_place_content_sys_46);
    RUN_TEST(test_l78_place_content_sys_47);
    RUN_TEST(test_l78_place_content_sys_48);
    RUN_TEST(test_l78_place_content_sys_49);
    RUN_TEST(test_l78_place_content_sys_50);
    RUN_TEST(test_l78_xy_place_1);
    RUN_TEST(test_l78_xy_place_2);
    RUN_TEST(test_l78_xy_place_3);
    RUN_TEST(test_l78_xy_place_4);
    RUN_TEST(test_l78_xy_place_5);
    RUN_TEST(test_l78_xy_place_6);
    RUN_TEST(test_l78_xy_place_7);
    RUN_TEST(test_l78_xy_place_8);
    RUN_TEST(test_l78_xy_place_9);
    RUN_TEST(test_l78_xy_place_10);
    RUN_TEST(test_l78_xy_place_11);
    RUN_TEST(test_l78_xy_place_12);
    RUN_TEST(test_l78_xy_place_13);
    RUN_TEST(test_l78_xy_place_14);
    RUN_TEST(test_l78_xy_place_15);
    RUN_TEST(test_l78_xy_place_16);
    RUN_TEST(test_l78_xy_place_17);
    RUN_TEST(test_l78_xy_place_18);
    RUN_TEST(test_l78_xy_place_19);
    RUN_TEST(test_l78_xy_place_20);
    RUN_TEST(test_l78_xy_place_21);
    RUN_TEST(test_l78_xy_place_22);
    RUN_TEST(test_l78_xy_place_23);
    RUN_TEST(test_l78_xy_place_24);
    RUN_TEST(test_l78_xy_place_25);
    RUN_TEST(test_l78_xy_place_26);
    RUN_TEST(test_l78_xy_place_27);
    RUN_TEST(test_l78_xy_place_28);
    RUN_TEST(test_l78_xy_place_29);
    RUN_TEST(test_l78_xy_place_30);
    RUN_TEST(test_l78_xy_place_31);
    RUN_TEST(test_l78_xy_place_32);
    RUN_TEST(test_l78_xy_place_33);
    RUN_TEST(test_l78_xy_place_34);
    RUN_TEST(test_l78_xy_place_35);
    RUN_TEST(test_l78_xy_place_36);
    RUN_TEST(test_l78_xy_place_37);
    RUN_TEST(test_l78_xy_place_38);
    RUN_TEST(test_l78_xy_place_39);
    RUN_TEST(test_l78_xy_place_40);
}
