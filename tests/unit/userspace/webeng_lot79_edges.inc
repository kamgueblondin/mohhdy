/* Lot 79: min/max sizes and flex/grid edge mixes. */
static void test_l79_minw_1(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:100px;grid-template-columns:30px 70px;align-items:stretch'><div id=a style='min-width:40px;height:10px'></div><div id=b style='height:10px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(40, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_minw_2(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:100px;grid-template-columns:40px 60px;align-items:stretch'><div id=a style='min-width:40px;height:10px'></div><div id=b style='height:10px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(40, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_minw_3(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:100px;grid-template-columns:50px 50px;align-items:stretch'><div id=a style='min-width:40px;height:10px'></div><div id=b style='height:10px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(50, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_minw_4(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:100px;grid-template-columns:30px 70px;align-items:stretch'><div id=a style='min-width:50px;height:10px'></div><div id=b style='height:10px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(50, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_minw_5(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:100px;grid-template-columns:40px 60px;align-items:stretch'><div id=a style='min-width:50px;height:10px'></div><div id=b style='height:10px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(50, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_minw_6(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:100px;grid-template-columns:50px 50px;align-items:stretch'><div id=a style='min-width:50px;height:10px'></div><div id=b style='height:10px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(50, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_minw_7(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:100px;grid-template-columns:30px 70px;align-items:stretch'><div id=a style='min-width:60px;height:10px'></div><div id=b style='height:10px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(60, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_minw_8(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:100px;grid-template-columns:40px 60px;align-items:stretch'><div id=a style='min-width:60px;height:10px'></div><div id=b style='height:10px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(60, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_minw_9(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:100px;grid-template-columns:50px 50px;align-items:stretch'><div id=a style='min-width:60px;height:10px'></div><div id=b style='height:10px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(60, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_minw_10(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:100px;grid-template-columns:30px 70px;align-items:stretch'><div id=a style='min-width:70px;height:10px'></div><div id=b style='height:10px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(70, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_minw_11(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:100px;grid-template-columns:40px 60px;align-items:stretch'><div id=a style='min-width:70px;height:10px'></div><div id=b style='height:10px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(70, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_minw_12(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:100px;grid-template-columns:50px 50px;align-items:stretch'><div id=a style='min-width:70px;height:10px'></div><div id=b style='height:10px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(70, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_minw_13(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:100px;grid-template-columns:30px 70px;align-items:stretch'><div id=a style='min-width:80px;height:10px'></div><div id=b style='height:10px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(80, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_minw_14(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:100px;grid-template-columns:40px 60px;align-items:stretch'><div id=a style='min-width:80px;height:10px'></div><div id=b style='height:10px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(80, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_minw_15(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:100px;grid-template-columns:50px 50px;align-items:stretch'><div id=a style='min-width:80px;height:10px'></div><div id=b style='height:10px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(80, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_maxw_16(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:120px;grid-template-columns:50px 1fr;justify-items:stretch;align-items:stretch'><div id=a style='max-width:20px;height:10px'></div><div id=b style='height:10px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_maxw_17(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:120px;grid-template-columns:60px 1fr;justify-items:stretch;align-items:stretch'><div id=a style='max-width:20px;height:10px'></div><div id=b style='height:10px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_maxw_18(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:120px;grid-template-columns:70px 1fr;justify-items:stretch;align-items:stretch'><div id=a style='max-width:20px;height:10px'></div><div id=b style='height:10px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_maxw_19(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:120px;grid-template-columns:50px 1fr;justify-items:stretch;align-items:stretch'><div id=a style='max-width:30px;height:10px'></div><div id=b style='height:10px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_maxw_20(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:120px;grid-template-columns:60px 1fr;justify-items:stretch;align-items:stretch'><div id=a style='max-width:30px;height:10px'></div><div id=b style='height:10px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_maxw_21(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:120px;grid-template-columns:70px 1fr;justify-items:stretch;align-items:stretch'><div id=a style='max-width:30px;height:10px'></div><div id=b style='height:10px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_maxw_22(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:120px;grid-template-columns:50px 1fr;justify-items:stretch;align-items:stretch'><div id=a style='max-width:40px;height:10px'></div><div id=b style='height:10px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(40, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_maxw_23(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:120px;grid-template-columns:60px 1fr;justify-items:stretch;align-items:stretch'><div id=a style='max-width:40px;height:10px'></div><div id=b style='height:10px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(40, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_maxw_24(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:120px;grid-template-columns:70px 1fr;justify-items:stretch;align-items:stretch'><div id=a style='max-width:40px;height:10px'></div><div id=b style='height:10px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(40, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_minh_25(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;grid-template-columns:40px;grid-template-rows:10px;align-items:start'><div id=a style='min-height:20px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_minh_26(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;grid-template-columns:40px;grid-template-rows:15px;align-items:start'><div id=a style='min-height:20px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_minh_27(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;grid-template-columns:40px;grid-template-rows:10px;align-items:start'><div id=a style='min-height:30px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_minh_28(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;grid-template-columns:40px;grid-template-rows:15px;align-items:start'><div id=a style='min-height:30px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_minh_29(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;grid-template-columns:40px;grid-template-rows:10px;align-items:start'><div id=a style='min-height:40px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(40, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_minh_30(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;grid-template-columns:40px;grid-template-rows:15px;align-items:start'><div id=a style='min-height:40px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(40, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_maxh_31(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:20px;grid-template-columns:40px;grid-template-rows:20px;align-items:stretch'><div id=a style='max-height:8px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(8, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_maxh_32(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:30px;grid-template-columns:40px;grid-template-rows:30px;align-items:stretch'><div id=a style='max-height:8px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(8, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_maxh_33(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:40px;grid-template-columns:40px;grid-template-rows:40px;align-items:stretch'><div id=a style='max-height:8px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(8, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_maxh_34(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:20px;grid-template-columns:40px;grid-template-rows:20px;align-items:stretch'><div id=a style='max-height:10px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_maxh_35(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:30px;grid-template-columns:40px;grid-template-rows:30px;align-items:stretch'><div id=a style='max-height:10px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_maxh_36(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:40px;grid-template-columns:40px;grid-template-rows:40px;align-items:stretch'><div id=a style='max-height:10px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_maxh_37(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:20px;grid-template-columns:40px;grid-template-rows:20px;align-items:stretch'><div id=a style='max-height:12px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(12, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_maxh_38(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:30px;grid-template-columns:40px;grid-template-rows:30px;align-items:stretch'><div id=a style='max-height:12px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(12, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_maxh_39(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;height:40px;grid-template-columns:40px;grid-template-rows:40px;align-items:stretch'><div id=a style='max-height:12px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(12, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_flex_mm_40(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:flex;width:100px;'><div id=a style='min-width:20px;max-width:20px;height:10px'></div><div id=b style='flex:1;height:10px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(100, lot52_node("g")->w);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_flex_mm_41(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:flex;width:100px;'><div id=a style='min-width:25px;max-width:25px;height:10px'></div><div id=b style='flex:1;height:10px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(25, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(100, lot52_node("g")->w);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_flex_mm_42(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:flex;width:100px;'><div id=a style='min-width:30px;max-width:30px;height:10px'></div><div id=b style='flex:1;height:10px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(100, lot52_node("g")->w);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_flex_mm_43(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:flex;width:100px;'><div id=a style='min-width:35px;max-width:35px;height:10px'></div><div id=b style='flex:1;height:10px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(35, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(100, lot52_node("g")->w);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_flex_mm_44(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:flex;width:100px;'><div id=a style='min-width:40px;max-width:40px;height:10px'></div><div id=b style='flex:1;height:10px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(40, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(100, lot52_node("g")->w);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_flex_mm_45(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:flex;width:100px;'><div id=a style='min-width:45px;max-width:45px;height:10px'></div><div id=b style='flex:1;height:10px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(45, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(100, lot52_node("g")->w);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_flex_mm_46(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:flex;width:100px;'><div id=a style='min-width:50px;max-width:50px;height:10px'></div><div id=b style='flex:1;height:10px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(50, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(100, lot52_node("g")->w);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_flex_mm_47(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:flex;width:100px;'><div id=a style='min-width:55px;max-width:55px;height:10px'></div><div id=b style='flex:1;height:10px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(55, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(100, lot52_node("g")->w);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_flex_mm_48(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:flex;width:100px;'><div id=a style='min-width:60px;max-width:60px;height:10px'></div><div id=b style='flex:1;height:10px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(60, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(100, lot52_node("g")->w);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_mix_1(void) {
    lot52_render("<body style='margin:0'><div id=f style='display:flex;width:100px;'><div id=fa style='min-width:35px;max-width:35px;height:10px'></div><div id=fb style='flex:1;height:10px'></div></div><div id=g style='display:grid;width:60px;grid-template-columns:repeat(3,20px);grid-auto-flow:dense;align-items:stretch'><div id=ga style='grid-column:span 2;height:12px'></div><div id=gb style='grid-column:span 2;height:12px'></div><div id=gc style='height:12px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(35, lot52_node("fa")->w);
    TEST_ASSERT_EQUAL(40, lot52_node("gc")->x);
    TEST_ASSERT_EQUAL(10, lot52_node("gc")->y);
    TEST_ASSERT_EQUAL(22, lot52_node("gb")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_mix_2(void) {
    lot52_render("<body style='margin:0'><div id=f style='display:flex;width:100px;'><div id=fa style='min-width:40px;max-width:40px;height:10px'></div><div id=fb style='flex:1;height:10px'></div></div><div id=g style='display:grid;width:60px;grid-template-columns:repeat(3,20px);grid-auto-flow:dense;align-items:stretch'><div id=ga style='grid-column:span 2;height:14px'></div><div id=gb style='grid-column:span 2;height:14px'></div><div id=gc style='height:14px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(40, lot52_node("fa")->w);
    TEST_ASSERT_EQUAL(40, lot52_node("gc")->x);
    TEST_ASSERT_EQUAL(10, lot52_node("gc")->y);
    TEST_ASSERT_EQUAL(24, lot52_node("gb")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_mix_3(void) {
    lot52_render("<body style='margin:0'><div id=f style='display:flex;width:100px;'><div id=fa style='min-width:45px;max-width:45px;height:10px'></div><div id=fb style='flex:1;height:10px'></div></div><div id=g style='display:grid;width:60px;grid-template-columns:repeat(3,20px);grid-auto-flow:dense;align-items:stretch'><div id=ga style='grid-column:span 2;height:16px'></div><div id=gb style='grid-column:span 2;height:16px'></div><div id=gc style='height:16px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(45, lot52_node("fa")->w);
    TEST_ASSERT_EQUAL(40, lot52_node("gc")->x);
    TEST_ASSERT_EQUAL(10, lot52_node("gc")->y);
    TEST_ASSERT_EQUAL(26, lot52_node("gb")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_mix_4(void) {
    lot52_render("<body style='margin:0'><div id=f style='display:flex;width:100px;'><div id=fa style='min-width:50px;max-width:50px;height:10px'></div><div id=fb style='flex:1;height:10px'></div></div><div id=g style='display:grid;width:60px;grid-template-columns:repeat(3,20px);grid-auto-flow:dense;align-items:stretch'><div id=ga style='grid-column:span 2;height:10px'></div><div id=gb style='grid-column:span 2;height:10px'></div><div id=gc style='height:10px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(50, lot52_node("fa")->w);
    TEST_ASSERT_EQUAL(40, lot52_node("gc")->x);
    TEST_ASSERT_EQUAL(10, lot52_node("gc")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("gb")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_mix_5(void) {
    lot52_render("<body style='margin:0'><div id=f style='display:flex;width:100px;'><div id=fa style='min-width:30px;max-width:30px;height:10px'></div><div id=fb style='flex:1;height:10px'></div></div><div id=g style='display:grid;width:60px;grid-template-columns:repeat(3,20px);grid-auto-flow:dense;align-items:stretch'><div id=ga style='grid-column:span 2;height:12px'></div><div id=gb style='grid-column:span 2;height:12px'></div><div id=gc style='height:12px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(30, lot52_node("fa")->w);
    TEST_ASSERT_EQUAL(40, lot52_node("gc")->x);
    TEST_ASSERT_EQUAL(10, lot52_node("gc")->y);
    TEST_ASSERT_EQUAL(22, lot52_node("gb")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_mix_6(void) {
    lot52_render("<body style='margin:0'><div id=f style='display:flex;width:100px;'><div id=fa style='min-width:35px;max-width:35px;height:10px'></div><div id=fb style='flex:1;height:10px'></div></div><div id=g style='display:grid;width:60px;grid-template-columns:repeat(3,20px);grid-auto-flow:dense;align-items:stretch'><div id=ga style='grid-column:span 2;height:14px'></div><div id=gb style='grid-column:span 2;height:14px'></div><div id=gc style='height:14px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(35, lot52_node("fa")->w);
    TEST_ASSERT_EQUAL(40, lot52_node("gc")->x);
    TEST_ASSERT_EQUAL(10, lot52_node("gc")->y);
    TEST_ASSERT_EQUAL(24, lot52_node("gb")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_mix_7(void) {
    lot52_render("<body style='margin:0'><div id=f style='display:flex;width:100px;'><div id=fa style='min-width:40px;max-width:40px;height:10px'></div><div id=fb style='flex:1;height:10px'></div></div><div id=g style='display:grid;width:60px;grid-template-columns:repeat(3,20px);grid-auto-flow:dense;align-items:stretch'><div id=ga style='grid-column:span 2;height:16px'></div><div id=gb style='grid-column:span 2;height:16px'></div><div id=gc style='height:16px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(40, lot52_node("fa")->w);
    TEST_ASSERT_EQUAL(40, lot52_node("gc")->x);
    TEST_ASSERT_EQUAL(10, lot52_node("gc")->y);
    TEST_ASSERT_EQUAL(26, lot52_node("gb")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_mix_8(void) {
    lot52_render("<body style='margin:0'><div id=f style='display:flex;width:100px;'><div id=fa style='min-width:45px;max-width:45px;height:10px'></div><div id=fb style='flex:1;height:10px'></div></div><div id=g style='display:grid;width:60px;grid-template-columns:repeat(3,20px);grid-auto-flow:dense;align-items:stretch'><div id=ga style='grid-column:span 2;height:10px'></div><div id=gb style='grid-column:span 2;height:10px'></div><div id=gc style='height:10px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(45, lot52_node("fa")->w);
    TEST_ASSERT_EQUAL(40, lot52_node("gc")->x);
    TEST_ASSERT_EQUAL(10, lot52_node("gc")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("gb")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_mix_9(void) {
    lot52_render("<body style='margin:0'><div id=f style='display:flex;width:100px;'><div id=fa style='min-width:50px;max-width:50px;height:10px'></div><div id=fb style='flex:1;height:10px'></div></div><div id=g style='display:grid;width:60px;grid-template-columns:repeat(3,20px);grid-auto-flow:dense;align-items:stretch'><div id=ga style='grid-column:span 2;height:12px'></div><div id=gb style='grid-column:span 2;height:12px'></div><div id=gc style='height:12px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(50, lot52_node("fa")->w);
    TEST_ASSERT_EQUAL(40, lot52_node("gc")->x);
    TEST_ASSERT_EQUAL(10, lot52_node("gc")->y);
    TEST_ASSERT_EQUAL(22, lot52_node("gb")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_mix_10(void) {
    lot52_render("<body style='margin:0'><div id=f style='display:flex;width:100px;'><div id=fa style='min-width:30px;max-width:30px;height:10px'></div><div id=fb style='flex:1;height:10px'></div></div><div id=g style='display:grid;width:60px;grid-template-columns:repeat(3,20px);grid-auto-flow:dense;align-items:stretch'><div id=ga style='grid-column:span 2;height:14px'></div><div id=gb style='grid-column:span 2;height:14px'></div><div id=gc style='height:14px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(30, lot52_node("fa")->w);
    TEST_ASSERT_EQUAL(40, lot52_node("gc")->x);
    TEST_ASSERT_EQUAL(10, lot52_node("gc")->y);
    TEST_ASSERT_EQUAL(24, lot52_node("gb")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_mix_11(void) {
    lot52_render("<body style='margin:0'><div id=f style='display:flex;width:100px;'><div id=fa style='min-width:35px;max-width:35px;height:10px'></div><div id=fb style='flex:1;height:10px'></div></div><div id=g style='display:grid;width:60px;grid-template-columns:repeat(3,20px);grid-auto-flow:dense;align-items:stretch'><div id=ga style='grid-column:span 2;height:16px'></div><div id=gb style='grid-column:span 2;height:16px'></div><div id=gc style='height:16px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(35, lot52_node("fa")->w);
    TEST_ASSERT_EQUAL(40, lot52_node("gc")->x);
    TEST_ASSERT_EQUAL(10, lot52_node("gc")->y);
    TEST_ASSERT_EQUAL(26, lot52_node("gb")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_mix_12(void) {
    lot52_render("<body style='margin:0'><div id=f style='display:flex;width:100px;'><div id=fa style='min-width:40px;max-width:40px;height:10px'></div><div id=fb style='flex:1;height:10px'></div></div><div id=g style='display:grid;width:60px;grid-template-columns:repeat(3,20px);grid-auto-flow:dense;align-items:stretch'><div id=ga style='grid-column:span 2;height:10px'></div><div id=gb style='grid-column:span 2;height:10px'></div><div id=gc style='height:10px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(40, lot52_node("fa")->w);
    TEST_ASSERT_EQUAL(40, lot52_node("gc")->x);
    TEST_ASSERT_EQUAL(10, lot52_node("gc")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("gb")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_mix_13(void) {
    lot52_render("<body style='margin:0'><div id=f style='display:flex;width:100px;'><div id=fa style='min-width:45px;max-width:45px;height:10px'></div><div id=fb style='flex:1;height:10px'></div></div><div id=g style='display:grid;width:60px;grid-template-columns:repeat(3,20px);grid-auto-flow:dense;align-items:stretch'><div id=ga style='grid-column:span 2;height:12px'></div><div id=gb style='grid-column:span 2;height:12px'></div><div id=gc style='height:12px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(45, lot52_node("fa")->w);
    TEST_ASSERT_EQUAL(40, lot52_node("gc")->x);
    TEST_ASSERT_EQUAL(10, lot52_node("gc")->y);
    TEST_ASSERT_EQUAL(22, lot52_node("gb")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_mix_14(void) {
    lot52_render("<body style='margin:0'><div id=f style='display:flex;width:100px;'><div id=fa style='min-width:50px;max-width:50px;height:10px'></div><div id=fb style='flex:1;height:10px'></div></div><div id=g style='display:grid;width:60px;grid-template-columns:repeat(3,20px);grid-auto-flow:dense;align-items:stretch'><div id=ga style='grid-column:span 2;height:14px'></div><div id=gb style='grid-column:span 2;height:14px'></div><div id=gc style='height:14px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(50, lot52_node("fa")->w);
    TEST_ASSERT_EQUAL(40, lot52_node("gc")->x);
    TEST_ASSERT_EQUAL(10, lot52_node("gc")->y);
    TEST_ASSERT_EQUAL(24, lot52_node("gb")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_mix_15(void) {
    lot52_render("<body style='margin:0'><div id=f style='display:flex;width:100px;'><div id=fa style='min-width:30px;max-width:30px;height:10px'></div><div id=fb style='flex:1;height:10px'></div></div><div id=g style='display:grid;width:60px;grid-template-columns:repeat(3,20px);grid-auto-flow:dense;align-items:stretch'><div id=ga style='grid-column:span 2;height:16px'></div><div id=gb style='grid-column:span 2;height:16px'></div><div id=gc style='height:16px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(30, lot52_node("fa")->w);
    TEST_ASSERT_EQUAL(40, lot52_node("gc")->x);
    TEST_ASSERT_EQUAL(10, lot52_node("gc")->y);
    TEST_ASSERT_EQUAL(26, lot52_node("gb")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_mix_16(void) {
    lot52_render("<body style='margin:0'><div id=f style='display:flex;width:100px;'><div id=fa style='min-width:35px;max-width:35px;height:10px'></div><div id=fb style='flex:1;height:10px'></div></div><div id=g style='display:grid;width:60px;grid-template-columns:repeat(3,20px);grid-auto-flow:dense;align-items:stretch'><div id=ga style='grid-column:span 2;height:10px'></div><div id=gb style='grid-column:span 2;height:10px'></div><div id=gc style='height:10px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(35, lot52_node("fa")->w);
    TEST_ASSERT_EQUAL(40, lot52_node("gc")->x);
    TEST_ASSERT_EQUAL(10, lot52_node("gc")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("gb")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_mix_17(void) {
    lot52_render("<body style='margin:0'><div id=f style='display:flex;width:100px;'><div id=fa style='min-width:40px;max-width:40px;height:10px'></div><div id=fb style='flex:1;height:10px'></div></div><div id=g style='display:grid;width:60px;grid-template-columns:repeat(3,20px);grid-auto-flow:dense;align-items:stretch'><div id=ga style='grid-column:span 2;height:12px'></div><div id=gb style='grid-column:span 2;height:12px'></div><div id=gc style='height:12px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(40, lot52_node("fa")->w);
    TEST_ASSERT_EQUAL(40, lot52_node("gc")->x);
    TEST_ASSERT_EQUAL(10, lot52_node("gc")->y);
    TEST_ASSERT_EQUAL(22, lot52_node("gb")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_mix_18(void) {
    lot52_render("<body style='margin:0'><div id=f style='display:flex;width:100px;'><div id=fa style='min-width:45px;max-width:45px;height:10px'></div><div id=fb style='flex:1;height:10px'></div></div><div id=g style='display:grid;width:60px;grid-template-columns:repeat(3,20px);grid-auto-flow:dense;align-items:stretch'><div id=ga style='grid-column:span 2;height:14px'></div><div id=gb style='grid-column:span 2;height:14px'></div><div id=gc style='height:14px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(45, lot52_node("fa")->w);
    TEST_ASSERT_EQUAL(40, lot52_node("gc")->x);
    TEST_ASSERT_EQUAL(10, lot52_node("gc")->y);
    TEST_ASSERT_EQUAL(24, lot52_node("gb")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_mix_19(void) {
    lot52_render("<body style='margin:0'><div id=f style='display:flex;width:100px;'><div id=fa style='min-width:50px;max-width:50px;height:10px'></div><div id=fb style='flex:1;height:10px'></div></div><div id=g style='display:grid;width:60px;grid-template-columns:repeat(3,20px);grid-auto-flow:dense;align-items:stretch'><div id=ga style='grid-column:span 2;height:16px'></div><div id=gb style='grid-column:span 2;height:16px'></div><div id=gc style='height:16px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(50, lot52_node("fa")->w);
    TEST_ASSERT_EQUAL(40, lot52_node("gc")->x);
    TEST_ASSERT_EQUAL(10, lot52_node("gc")->y);
    TEST_ASSERT_EQUAL(26, lot52_node("gb")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_mix_20(void) {
    lot52_render("<body style='margin:0'><div id=f style='display:flex;width:100px;'><div id=fa style='min-width:30px;max-width:30px;height:10px'></div><div id=fb style='flex:1;height:10px'></div></div><div id=g style='display:grid;width:60px;grid-template-columns:repeat(3,20px);grid-auto-flow:dense;align-items:stretch'><div id=ga style='grid-column:span 2;height:10px'></div><div id=gb style='grid-column:span 2;height:10px'></div><div id=gc style='height:10px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(30, lot52_node("fa")->w);
    TEST_ASSERT_EQUAL(40, lot52_node("gc")->x);
    TEST_ASSERT_EQUAL(10, lot52_node("gc")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("gb")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_mix_21(void) {
    lot52_render("<body style='margin:0'><div id=f style='display:flex;width:100px;'><div id=fa style='min-width:35px;max-width:35px;height:10px'></div><div id=fb style='flex:1;height:10px'></div></div><div id=g style='display:grid;width:60px;grid-template-columns:repeat(3,20px);grid-auto-flow:dense;align-items:stretch'><div id=ga style='grid-column:span 2;height:12px'></div><div id=gb style='grid-column:span 2;height:12px'></div><div id=gc style='height:12px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(35, lot52_node("fa")->w);
    TEST_ASSERT_EQUAL(40, lot52_node("gc")->x);
    TEST_ASSERT_EQUAL(10, lot52_node("gc")->y);
    TEST_ASSERT_EQUAL(22, lot52_node("gb")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_mix_22(void) {
    lot52_render("<body style='margin:0'><div id=f style='display:flex;width:100px;'><div id=fa style='min-width:40px;max-width:40px;height:10px'></div><div id=fb style='flex:1;height:10px'></div></div><div id=g style='display:grid;width:60px;grid-template-columns:repeat(3,20px);grid-auto-flow:dense;align-items:stretch'><div id=ga style='grid-column:span 2;height:14px'></div><div id=gb style='grid-column:span 2;height:14px'></div><div id=gc style='height:14px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(40, lot52_node("fa")->w);
    TEST_ASSERT_EQUAL(40, lot52_node("gc")->x);
    TEST_ASSERT_EQUAL(10, lot52_node("gc")->y);
    TEST_ASSERT_EQUAL(24, lot52_node("gb")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_mix_23(void) {
    lot52_render("<body style='margin:0'><div id=f style='display:flex;width:100px;'><div id=fa style='min-width:45px;max-width:45px;height:10px'></div><div id=fb style='flex:1;height:10px'></div></div><div id=g style='display:grid;width:60px;grid-template-columns:repeat(3,20px);grid-auto-flow:dense;align-items:stretch'><div id=ga style='grid-column:span 2;height:16px'></div><div id=gb style='grid-column:span 2;height:16px'></div><div id=gc style='height:16px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(45, lot52_node("fa")->w);
    TEST_ASSERT_EQUAL(40, lot52_node("gc")->x);
    TEST_ASSERT_EQUAL(10, lot52_node("gc")->y);
    TEST_ASSERT_EQUAL(26, lot52_node("gb")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_mix_24(void) {
    lot52_render("<body style='margin:0'><div id=f style='display:flex;width:100px;'><div id=fa style='min-width:50px;max-width:50px;height:10px'></div><div id=fb style='flex:1;height:10px'></div></div><div id=g style='display:grid;width:60px;grid-template-columns:repeat(3,20px);grid-auto-flow:dense;align-items:stretch'><div id=ga style='grid-column:span 2;height:10px'></div><div id=gb style='grid-column:span 2;height:10px'></div><div id=gc style='height:10px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(50, lot52_node("fa")->w);
    TEST_ASSERT_EQUAL(40, lot52_node("gc")->x);
    TEST_ASSERT_EQUAL(10, lot52_node("gc")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("gb")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_mix_25(void) {
    lot52_render("<body style='margin:0'><div id=f style='display:flex;width:100px;'><div id=fa style='min-width:30px;max-width:30px;height:10px'></div><div id=fb style='flex:1;height:10px'></div></div><div id=g style='display:grid;width:60px;grid-template-columns:repeat(3,20px);grid-auto-flow:dense;align-items:stretch'><div id=ga style='grid-column:span 2;height:12px'></div><div id=gb style='grid-column:span 2;height:12px'></div><div id=gc style='height:12px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(30, lot52_node("fa")->w);
    TEST_ASSERT_EQUAL(40, lot52_node("gc")->x);
    TEST_ASSERT_EQUAL(10, lot52_node("gc")->y);
    TEST_ASSERT_EQUAL(22, lot52_node("gb")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_mix_26(void) {
    lot52_render("<body style='margin:0'><div id=f style='display:flex;width:100px;'><div id=fa style='min-width:35px;max-width:35px;height:10px'></div><div id=fb style='flex:1;height:10px'></div></div><div id=g style='display:grid;width:60px;grid-template-columns:repeat(3,20px);grid-auto-flow:dense;align-items:stretch'><div id=ga style='grid-column:span 2;height:14px'></div><div id=gb style='grid-column:span 2;height:14px'></div><div id=gc style='height:14px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(35, lot52_node("fa")->w);
    TEST_ASSERT_EQUAL(40, lot52_node("gc")->x);
    TEST_ASSERT_EQUAL(10, lot52_node("gc")->y);
    TEST_ASSERT_EQUAL(24, lot52_node("gb")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_mix_27(void) {
    lot52_render("<body style='margin:0'><div id=f style='display:flex;width:100px;'><div id=fa style='min-width:40px;max-width:40px;height:10px'></div><div id=fb style='flex:1;height:10px'></div></div><div id=g style='display:grid;width:60px;grid-template-columns:repeat(3,20px);grid-auto-flow:dense;align-items:stretch'><div id=ga style='grid-column:span 2;height:16px'></div><div id=gb style='grid-column:span 2;height:16px'></div><div id=gc style='height:16px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(40, lot52_node("fa")->w);
    TEST_ASSERT_EQUAL(40, lot52_node("gc")->x);
    TEST_ASSERT_EQUAL(10, lot52_node("gc")->y);
    TEST_ASSERT_EQUAL(26, lot52_node("gb")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_mix_28(void) {
    lot52_render("<body style='margin:0'><div id=f style='display:flex;width:100px;'><div id=fa style='min-width:45px;max-width:45px;height:10px'></div><div id=fb style='flex:1;height:10px'></div></div><div id=g style='display:grid;width:60px;grid-template-columns:repeat(3,20px);grid-auto-flow:dense;align-items:stretch'><div id=ga style='grid-column:span 2;height:10px'></div><div id=gb style='grid-column:span 2;height:10px'></div><div id=gc style='height:10px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(45, lot52_node("fa")->w);
    TEST_ASSERT_EQUAL(40, lot52_node("gc")->x);
    TEST_ASSERT_EQUAL(10, lot52_node("gc")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("gb")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_mix_29(void) {
    lot52_render("<body style='margin:0'><div id=f style='display:flex;width:100px;'><div id=fa style='min-width:50px;max-width:50px;height:10px'></div><div id=fb style='flex:1;height:10px'></div></div><div id=g style='display:grid;width:60px;grid-template-columns:repeat(3,20px);grid-auto-flow:dense;align-items:stretch'><div id=ga style='grid-column:span 2;height:12px'></div><div id=gb style='grid-column:span 2;height:12px'></div><div id=gc style='height:12px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(50, lot52_node("fa")->w);
    TEST_ASSERT_EQUAL(40, lot52_node("gc")->x);
    TEST_ASSERT_EQUAL(10, lot52_node("gc")->y);
    TEST_ASSERT_EQUAL(22, lot52_node("gb")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_mix_30(void) {
    lot52_render("<body style='margin:0'><div id=f style='display:flex;width:100px;'><div id=fa style='min-width:30px;max-width:30px;height:10px'></div><div id=fb style='flex:1;height:10px'></div></div><div id=g style='display:grid;width:60px;grid-template-columns:repeat(3,20px);grid-auto-flow:dense;align-items:stretch'><div id=ga style='grid-column:span 2;height:14px'></div><div id=gb style='grid-column:span 2;height:14px'></div><div id=gc style='height:14px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(30, lot52_node("fa")->w);
    TEST_ASSERT_EQUAL(40, lot52_node("gc")->x);
    TEST_ASSERT_EQUAL(10, lot52_node("gc")->y);
    TEST_ASSERT_EQUAL(24, lot52_node("gb")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_mix_31(void) {
    lot52_render("<body style='margin:0'><div id=f style='display:flex;width:100px;'><div id=fa style='min-width:35px;max-width:35px;height:10px'></div><div id=fb style='flex:1;height:10px'></div></div><div id=g style='display:grid;width:60px;grid-template-columns:repeat(3,20px);grid-auto-flow:dense;align-items:stretch'><div id=ga style='grid-column:span 2;height:16px'></div><div id=gb style='grid-column:span 2;height:16px'></div><div id=gc style='height:16px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(35, lot52_node("fa")->w);
    TEST_ASSERT_EQUAL(40, lot52_node("gc")->x);
    TEST_ASSERT_EQUAL(10, lot52_node("gc")->y);
    TEST_ASSERT_EQUAL(26, lot52_node("gb")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_mix_32(void) {
    lot52_render("<body style='margin:0'><div id=f style='display:flex;width:100px;'><div id=fa style='min-width:40px;max-width:40px;height:10px'></div><div id=fb style='flex:1;height:10px'></div></div><div id=g style='display:grid;width:60px;grid-template-columns:repeat(3,20px);grid-auto-flow:dense;align-items:stretch'><div id=ga style='grid-column:span 2;height:10px'></div><div id=gb style='grid-column:span 2;height:10px'></div><div id=gc style='height:10px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(40, lot52_node("fa")->w);
    TEST_ASSERT_EQUAL(40, lot52_node("gc")->x);
    TEST_ASSERT_EQUAL(10, lot52_node("gc")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("gb")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_mix_33(void) {
    lot52_render("<body style='margin:0'><div id=f style='display:flex;width:100px;'><div id=fa style='min-width:45px;max-width:45px;height:10px'></div><div id=fb style='flex:1;height:10px'></div></div><div id=g style='display:grid;width:60px;grid-template-columns:repeat(3,20px);grid-auto-flow:dense;align-items:stretch'><div id=ga style='grid-column:span 2;height:12px'></div><div id=gb style='grid-column:span 2;height:12px'></div><div id=gc style='height:12px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(45, lot52_node("fa")->w);
    TEST_ASSERT_EQUAL(40, lot52_node("gc")->x);
    TEST_ASSERT_EQUAL(10, lot52_node("gc")->y);
    TEST_ASSERT_EQUAL(22, lot52_node("gb")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_mix_34(void) {
    lot52_render("<body style='margin:0'><div id=f style='display:flex;width:100px;'><div id=fa style='min-width:50px;max-width:50px;height:10px'></div><div id=fb style='flex:1;height:10px'></div></div><div id=g style='display:grid;width:60px;grid-template-columns:repeat(3,20px);grid-auto-flow:dense;align-items:stretch'><div id=ga style='grid-column:span 2;height:14px'></div><div id=gb style='grid-column:span 2;height:14px'></div><div id=gc style='height:14px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(50, lot52_node("fa")->w);
    TEST_ASSERT_EQUAL(40, lot52_node("gc")->x);
    TEST_ASSERT_EQUAL(10, lot52_node("gc")->y);
    TEST_ASSERT_EQUAL(24, lot52_node("gb")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_mix_35(void) {
    lot52_render("<body style='margin:0'><div id=f style='display:flex;width:100px;'><div id=fa style='min-width:30px;max-width:30px;height:10px'></div><div id=fb style='flex:1;height:10px'></div></div><div id=g style='display:grid;width:60px;grid-template-columns:repeat(3,20px);grid-auto-flow:dense;align-items:stretch'><div id=ga style='grid-column:span 2;height:16px'></div><div id=gb style='grid-column:span 2;height:16px'></div><div id=gc style='height:16px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(30, lot52_node("fa")->w);
    TEST_ASSERT_EQUAL(40, lot52_node("gc")->x);
    TEST_ASSERT_EQUAL(10, lot52_node("gc")->y);
    TEST_ASSERT_EQUAL(26, lot52_node("gb")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_mix_36(void) {
    lot52_render("<body style='margin:0'><div id=f style='display:flex;width:100px;'><div id=fa style='min-width:35px;max-width:35px;height:10px'></div><div id=fb style='flex:1;height:10px'></div></div><div id=g style='display:grid;width:60px;grid-template-columns:repeat(3,20px);grid-auto-flow:dense;align-items:stretch'><div id=ga style='grid-column:span 2;height:10px'></div><div id=gb style='grid-column:span 2;height:10px'></div><div id=gc style='height:10px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(35, lot52_node("fa")->w);
    TEST_ASSERT_EQUAL(40, lot52_node("gc")->x);
    TEST_ASSERT_EQUAL(10, lot52_node("gc")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("gb")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_mix_37(void) {
    lot52_render("<body style='margin:0'><div id=f style='display:flex;width:100px;'><div id=fa style='min-width:40px;max-width:40px;height:10px'></div><div id=fb style='flex:1;height:10px'></div></div><div id=g style='display:grid;width:60px;grid-template-columns:repeat(3,20px);grid-auto-flow:dense;align-items:stretch'><div id=ga style='grid-column:span 2;height:12px'></div><div id=gb style='grid-column:span 2;height:12px'></div><div id=gc style='height:12px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(40, lot52_node("fa")->w);
    TEST_ASSERT_EQUAL(40, lot52_node("gc")->x);
    TEST_ASSERT_EQUAL(10, lot52_node("gc")->y);
    TEST_ASSERT_EQUAL(22, lot52_node("gb")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_mix_38(void) {
    lot52_render("<body style='margin:0'><div id=f style='display:flex;width:100px;'><div id=fa style='min-width:45px;max-width:45px;height:10px'></div><div id=fb style='flex:1;height:10px'></div></div><div id=g style='display:grid;width:60px;grid-template-columns:repeat(3,20px);grid-auto-flow:dense;align-items:stretch'><div id=ga style='grid-column:span 2;height:14px'></div><div id=gb style='grid-column:span 2;height:14px'></div><div id=gc style='height:14px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(45, lot52_node("fa")->w);
    TEST_ASSERT_EQUAL(40, lot52_node("gc")->x);
    TEST_ASSERT_EQUAL(10, lot52_node("gc")->y);
    TEST_ASSERT_EQUAL(24, lot52_node("gb")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_mix_39(void) {
    lot52_render("<body style='margin:0'><div id=f style='display:flex;width:100px;'><div id=fa style='min-width:50px;max-width:50px;height:10px'></div><div id=fb style='flex:1;height:10px'></div></div><div id=g style='display:grid;width:60px;grid-template-columns:repeat(3,20px);grid-auto-flow:dense;align-items:stretch'><div id=ga style='grid-column:span 2;height:16px'></div><div id=gb style='grid-column:span 2;height:16px'></div><div id=gc style='height:16px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(50, lot52_node("fa")->w);
    TEST_ASSERT_EQUAL(40, lot52_node("gc")->x);
    TEST_ASSERT_EQUAL(10, lot52_node("gc")->y);
    TEST_ASSERT_EQUAL(26, lot52_node("gb")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_mix_40(void) {
    lot52_render("<body style='margin:0'><div id=f style='display:flex;width:100px;'><div id=fa style='min-width:30px;max-width:30px;height:10px'></div><div id=fb style='flex:1;height:10px'></div></div><div id=g style='display:grid;width:60px;grid-template-columns:repeat(3,20px);grid-auto-flow:dense;align-items:stretch'><div id=ga style='grid-column:span 2;height:10px'></div><div id=gb style='grid-column:span 2;height:10px'></div><div id=gc style='height:10px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(30, lot52_node("fa")->w);
    TEST_ASSERT_EQUAL(40, lot52_node("gc")->x);
    TEST_ASSERT_EQUAL(10, lot52_node("gc")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("gb")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_mix_41(void) {
    lot52_render("<body style='margin:0'><div id=f style='display:flex;width:100px;'><div id=fa style='min-width:35px;max-width:35px;height:10px'></div><div id=fb style='flex:1;height:10px'></div></div><div id=g style='display:grid;width:60px;grid-template-columns:repeat(3,20px);grid-auto-flow:dense;align-items:stretch'><div id=ga style='grid-column:span 2;height:12px'></div><div id=gb style='grid-column:span 2;height:12px'></div><div id=gc style='height:12px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(35, lot52_node("fa")->w);
    TEST_ASSERT_EQUAL(40, lot52_node("gc")->x);
    TEST_ASSERT_EQUAL(10, lot52_node("gc")->y);
    TEST_ASSERT_EQUAL(22, lot52_node("gb")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_mix_42(void) {
    lot52_render("<body style='margin:0'><div id=f style='display:flex;width:100px;'><div id=fa style='min-width:40px;max-width:40px;height:10px'></div><div id=fb style='flex:1;height:10px'></div></div><div id=g style='display:grid;width:60px;grid-template-columns:repeat(3,20px);grid-auto-flow:dense;align-items:stretch'><div id=ga style='grid-column:span 2;height:14px'></div><div id=gb style='grid-column:span 2;height:14px'></div><div id=gc style='height:14px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(40, lot52_node("fa")->w);
    TEST_ASSERT_EQUAL(40, lot52_node("gc")->x);
    TEST_ASSERT_EQUAL(10, lot52_node("gc")->y);
    TEST_ASSERT_EQUAL(24, lot52_node("gb")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_mix_43(void) {
    lot52_render("<body style='margin:0'><div id=f style='display:flex;width:100px;'><div id=fa style='min-width:45px;max-width:45px;height:10px'></div><div id=fb style='flex:1;height:10px'></div></div><div id=g style='display:grid;width:60px;grid-template-columns:repeat(3,20px);grid-auto-flow:dense;align-items:stretch'><div id=ga style='grid-column:span 2;height:16px'></div><div id=gb style='grid-column:span 2;height:16px'></div><div id=gc style='height:16px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(45, lot52_node("fa")->w);
    TEST_ASSERT_EQUAL(40, lot52_node("gc")->x);
    TEST_ASSERT_EQUAL(10, lot52_node("gc")->y);
    TEST_ASSERT_EQUAL(26, lot52_node("gb")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_mix_44(void) {
    lot52_render("<body style='margin:0'><div id=f style='display:flex;width:100px;'><div id=fa style='min-width:50px;max-width:50px;height:10px'></div><div id=fb style='flex:1;height:10px'></div></div><div id=g style='display:grid;width:60px;grid-template-columns:repeat(3,20px);grid-auto-flow:dense;align-items:stretch'><div id=ga style='grid-column:span 2;height:10px'></div><div id=gb style='grid-column:span 2;height:10px'></div><div id=gc style='height:10px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(50, lot52_node("fa")->w);
    TEST_ASSERT_EQUAL(40, lot52_node("gc")->x);
    TEST_ASSERT_EQUAL(10, lot52_node("gc")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("gb")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_mix_45(void) {
    lot52_render("<body style='margin:0'><div id=f style='display:flex;width:100px;'><div id=fa style='min-width:30px;max-width:30px;height:10px'></div><div id=fb style='flex:1;height:10px'></div></div><div id=g style='display:grid;width:60px;grid-template-columns:repeat(3,20px);grid-auto-flow:dense;align-items:stretch'><div id=ga style='grid-column:span 2;height:12px'></div><div id=gb style='grid-column:span 2;height:12px'></div><div id=gc style='height:12px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(30, lot52_node("fa")->w);
    TEST_ASSERT_EQUAL(40, lot52_node("gc")->x);
    TEST_ASSERT_EQUAL(10, lot52_node("gc")->y);
    TEST_ASSERT_EQUAL(22, lot52_node("gb")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_mix_46(void) {
    lot52_render("<body style='margin:0'><div id=f style='display:flex;width:100px;'><div id=fa style='min-width:35px;max-width:35px;height:10px'></div><div id=fb style='flex:1;height:10px'></div></div><div id=g style='display:grid;width:60px;grid-template-columns:repeat(3,20px);grid-auto-flow:dense;align-items:stretch'><div id=ga style='grid-column:span 2;height:14px'></div><div id=gb style='grid-column:span 2;height:14px'></div><div id=gc style='height:14px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(35, lot52_node("fa")->w);
    TEST_ASSERT_EQUAL(40, lot52_node("gc")->x);
    TEST_ASSERT_EQUAL(10, lot52_node("gc")->y);
    TEST_ASSERT_EQUAL(24, lot52_node("gb")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_mix_47(void) {
    lot52_render("<body style='margin:0'><div id=f style='display:flex;width:100px;'><div id=fa style='min-width:40px;max-width:40px;height:10px'></div><div id=fb style='flex:1;height:10px'></div></div><div id=g style='display:grid;width:60px;grid-template-columns:repeat(3,20px);grid-auto-flow:dense;align-items:stretch'><div id=ga style='grid-column:span 2;height:16px'></div><div id=gb style='grid-column:span 2;height:16px'></div><div id=gc style='height:16px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(40, lot52_node("fa")->w);
    TEST_ASSERT_EQUAL(40, lot52_node("gc")->x);
    TEST_ASSERT_EQUAL(10, lot52_node("gc")->y);
    TEST_ASSERT_EQUAL(26, lot52_node("gb")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_mix_48(void) {
    lot52_render("<body style='margin:0'><div id=f style='display:flex;width:100px;'><div id=fa style='min-width:45px;max-width:45px;height:10px'></div><div id=fb style='flex:1;height:10px'></div></div><div id=g style='display:grid;width:60px;grid-template-columns:repeat(3,20px);grid-auto-flow:dense;align-items:stretch'><div id=ga style='grid-column:span 2;height:10px'></div><div id=gb style='grid-column:span 2;height:10px'></div><div id=gc style='height:10px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(45, lot52_node("fa")->w);
    TEST_ASSERT_EQUAL(40, lot52_node("gc")->x);
    TEST_ASSERT_EQUAL(10, lot52_node("gc")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("gb")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_mix_49(void) {
    lot52_render("<body style='margin:0'><div id=f style='display:flex;width:100px;'><div id=fa style='min-width:50px;max-width:50px;height:10px'></div><div id=fb style='flex:1;height:10px'></div></div><div id=g style='display:grid;width:60px;grid-template-columns:repeat(3,20px);grid-auto-flow:dense;align-items:stretch'><div id=ga style='grid-column:span 2;height:12px'></div><div id=gb style='grid-column:span 2;height:12px'></div><div id=gc style='height:12px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(50, lot52_node("fa")->w);
    TEST_ASSERT_EQUAL(40, lot52_node("gc")->x);
    TEST_ASSERT_EQUAL(10, lot52_node("gc")->y);
    TEST_ASSERT_EQUAL(22, lot52_node("gb")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_mix_50(void) {
    lot52_render("<body style='margin:0'><div id=f style='display:flex;width:100px;'><div id=fa style='min-width:30px;max-width:30px;height:10px'></div><div id=fb style='flex:1;height:10px'></div></div><div id=g style='display:grid;width:60px;grid-template-columns:repeat(3,20px);grid-auto-flow:dense;align-items:stretch'><div id=ga style='grid-column:span 2;height:14px'></div><div id=gb style='grid-column:span 2;height:14px'></div><div id=gc style='height:14px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(30, lot52_node("fa")->w);
    TEST_ASSERT_EQUAL(40, lot52_node("gc")->x);
    TEST_ASSERT_EQUAL(10, lot52_node("gc")->y);
    TEST_ASSERT_EQUAL(24, lot52_node("gb")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_mix_51(void) {
    lot52_render("<body style='margin:0'><div id=f style='display:flex;width:100px;'><div id=fa style='min-width:35px;max-width:35px;height:10px'></div><div id=fb style='flex:1;height:10px'></div></div><div id=g style='display:grid;width:60px;grid-template-columns:repeat(3,20px);grid-auto-flow:dense;align-items:stretch'><div id=ga style='grid-column:span 2;height:16px'></div><div id=gb style='grid-column:span 2;height:16px'></div><div id=gc style='height:16px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(35, lot52_node("fa")->w);
    TEST_ASSERT_EQUAL(40, lot52_node("gc")->x);
    TEST_ASSERT_EQUAL(10, lot52_node("gc")->y);
    TEST_ASSERT_EQUAL(26, lot52_node("gb")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_mix_52(void) {
    lot52_render("<body style='margin:0'><div id=f style='display:flex;width:100px;'><div id=fa style='min-width:40px;max-width:40px;height:10px'></div><div id=fb style='flex:1;height:10px'></div></div><div id=g style='display:grid;width:60px;grid-template-columns:repeat(3,20px);grid-auto-flow:dense;align-items:stretch'><div id=ga style='grid-column:span 2;height:10px'></div><div id=gb style='grid-column:span 2;height:10px'></div><div id=gc style='height:10px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(40, lot52_node("fa")->w);
    TEST_ASSERT_EQUAL(40, lot52_node("gc")->x);
    TEST_ASSERT_EQUAL(10, lot52_node("gc")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("gb")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_mix_53(void) {
    lot52_render("<body style='margin:0'><div id=f style='display:flex;width:100px;'><div id=fa style='min-width:45px;max-width:45px;height:10px'></div><div id=fb style='flex:1;height:10px'></div></div><div id=g style='display:grid;width:60px;grid-template-columns:repeat(3,20px);grid-auto-flow:dense;align-items:stretch'><div id=ga style='grid-column:span 2;height:12px'></div><div id=gb style='grid-column:span 2;height:12px'></div><div id=gc style='height:12px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(45, lot52_node("fa")->w);
    TEST_ASSERT_EQUAL(40, lot52_node("gc")->x);
    TEST_ASSERT_EQUAL(10, lot52_node("gc")->y);
    TEST_ASSERT_EQUAL(22, lot52_node("gb")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_mix_54(void) {
    lot52_render("<body style='margin:0'><div id=f style='display:flex;width:100px;'><div id=fa style='min-width:50px;max-width:50px;height:10px'></div><div id=fb style='flex:1;height:10px'></div></div><div id=g style='display:grid;width:60px;grid-template-columns:repeat(3,20px);grid-auto-flow:dense;align-items:stretch'><div id=ga style='grid-column:span 2;height:14px'></div><div id=gb style='grid-column:span 2;height:14px'></div><div id=gc style='height:14px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(50, lot52_node("fa")->w);
    TEST_ASSERT_EQUAL(40, lot52_node("gc")->x);
    TEST_ASSERT_EQUAL(10, lot52_node("gc")->y);
    TEST_ASSERT_EQUAL(24, lot52_node("gb")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_mix_55(void) {
    lot52_render("<body style='margin:0'><div id=f style='display:flex;width:100px;'><div id=fa style='min-width:30px;max-width:30px;height:10px'></div><div id=fb style='flex:1;height:10px'></div></div><div id=g style='display:grid;width:60px;grid-template-columns:repeat(3,20px);grid-auto-flow:dense;align-items:stretch'><div id=ga style='grid-column:span 2;height:16px'></div><div id=gb style='grid-column:span 2;height:16px'></div><div id=gc style='height:16px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(30, lot52_node("fa")->w);
    TEST_ASSERT_EQUAL(40, lot52_node("gc")->x);
    TEST_ASSERT_EQUAL(10, lot52_node("gc")->y);
    TEST_ASSERT_EQUAL(26, lot52_node("gb")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_mix_56(void) {
    lot52_render("<body style='margin:0'><div id=f style='display:flex;width:100px;'><div id=fa style='min-width:35px;max-width:35px;height:10px'></div><div id=fb style='flex:1;height:10px'></div></div><div id=g style='display:grid;width:60px;grid-template-columns:repeat(3,20px);grid-auto-flow:dense;align-items:stretch'><div id=ga style='grid-column:span 2;height:10px'></div><div id=gb style='grid-column:span 2;height:10px'></div><div id=gc style='height:10px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(35, lot52_node("fa")->w);
    TEST_ASSERT_EQUAL(40, lot52_node("gc")->x);
    TEST_ASSERT_EQUAL(10, lot52_node("gc")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("gb")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_mix_57(void) {
    lot52_render("<body style='margin:0'><div id=f style='display:flex;width:100px;'><div id=fa style='min-width:40px;max-width:40px;height:10px'></div><div id=fb style='flex:1;height:10px'></div></div><div id=g style='display:grid;width:60px;grid-template-columns:repeat(3,20px);grid-auto-flow:dense;align-items:stretch'><div id=ga style='grid-column:span 2;height:12px'></div><div id=gb style='grid-column:span 2;height:12px'></div><div id=gc style='height:12px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(40, lot52_node("fa")->w);
    TEST_ASSERT_EQUAL(40, lot52_node("gc")->x);
    TEST_ASSERT_EQUAL(10, lot52_node("gc")->y);
    TEST_ASSERT_EQUAL(22, lot52_node("gb")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_mix_58(void) {
    lot52_render("<body style='margin:0'><div id=f style='display:flex;width:100px;'><div id=fa style='min-width:45px;max-width:45px;height:10px'></div><div id=fb style='flex:1;height:10px'></div></div><div id=g style='display:grid;width:60px;grid-template-columns:repeat(3,20px);grid-auto-flow:dense;align-items:stretch'><div id=ga style='grid-column:span 2;height:14px'></div><div id=gb style='grid-column:span 2;height:14px'></div><div id=gc style='height:14px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(45, lot52_node("fa")->w);
    TEST_ASSERT_EQUAL(40, lot52_node("gc")->x);
    TEST_ASSERT_EQUAL(10, lot52_node("gc")->y);
    TEST_ASSERT_EQUAL(24, lot52_node("gb")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_mix_59(void) {
    lot52_render("<body style='margin:0'><div id=f style='display:flex;width:100px;'><div id=fa style='min-width:50px;max-width:50px;height:10px'></div><div id=fb style='flex:1;height:10px'></div></div><div id=g style='display:grid;width:60px;grid-template-columns:repeat(3,20px);grid-auto-flow:dense;align-items:stretch'><div id=ga style='grid-column:span 2;height:16px'></div><div id=gb style='grid-column:span 2;height:16px'></div><div id=gc style='height:16px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(50, lot52_node("fa")->w);
    TEST_ASSERT_EQUAL(40, lot52_node("gc")->x);
    TEST_ASSERT_EQUAL(10, lot52_node("gc")->y);
    TEST_ASSERT_EQUAL(26, lot52_node("gb")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_mix_60(void) {
    lot52_render("<body style='margin:0'><div id=f style='display:flex;width:100px;'><div id=fa style='min-width:30px;max-width:30px;height:10px'></div><div id=fb style='flex:1;height:10px'></div></div><div id=g style='display:grid;width:60px;grid-template-columns:repeat(3,20px);grid-auto-flow:dense;align-items:stretch'><div id=ga style='grid-column:span 2;height:10px'></div><div id=gb style='grid-column:span 2;height:10px'></div><div id=gc style='height:10px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(30, lot52_node("fa")->w);
    TEST_ASSERT_EQUAL(40, lot52_node("gc")->x);
    TEST_ASSERT_EQUAL(10, lot52_node("gc")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("gb")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_mix_61(void) {
    lot52_render("<body style='margin:0'><div id=f style='display:flex;width:100px;'><div id=fa style='min-width:35px;max-width:35px;height:10px'></div><div id=fb style='flex:1;height:10px'></div></div><div id=g style='display:grid;width:60px;grid-template-columns:repeat(3,20px);grid-auto-flow:dense;align-items:stretch'><div id=ga style='grid-column:span 2;height:12px'></div><div id=gb style='grid-column:span 2;height:12px'></div><div id=gc style='height:12px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(35, lot52_node("fa")->w);
    TEST_ASSERT_EQUAL(40, lot52_node("gc")->x);
    TEST_ASSERT_EQUAL(10, lot52_node("gc")->y);
    TEST_ASSERT_EQUAL(22, lot52_node("gb")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_mix_62(void) {
    lot52_render("<body style='margin:0'><div id=f style='display:flex;width:100px;'><div id=fa style='min-width:40px;max-width:40px;height:10px'></div><div id=fb style='flex:1;height:10px'></div></div><div id=g style='display:grid;width:60px;grid-template-columns:repeat(3,20px);grid-auto-flow:dense;align-items:stretch'><div id=ga style='grid-column:span 2;height:14px'></div><div id=gb style='grid-column:span 2;height:14px'></div><div id=gc style='height:14px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(40, lot52_node("fa")->w);
    TEST_ASSERT_EQUAL(40, lot52_node("gc")->x);
    TEST_ASSERT_EQUAL(10, lot52_node("gc")->y);
    TEST_ASSERT_EQUAL(24, lot52_node("gb")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_mix_63(void) {
    lot52_render("<body style='margin:0'><div id=f style='display:flex;width:100px;'><div id=fa style='min-width:45px;max-width:45px;height:10px'></div><div id=fb style='flex:1;height:10px'></div></div><div id=g style='display:grid;width:60px;grid-template-columns:repeat(3,20px);grid-auto-flow:dense;align-items:stretch'><div id=ga style='grid-column:span 2;height:16px'></div><div id=gb style='grid-column:span 2;height:16px'></div><div id=gc style='height:16px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(45, lot52_node("fa")->w);
    TEST_ASSERT_EQUAL(40, lot52_node("gc")->x);
    TEST_ASSERT_EQUAL(10, lot52_node("gc")->y);
    TEST_ASSERT_EQUAL(26, lot52_node("gb")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_mix_64(void) {
    lot52_render("<body style='margin:0'><div id=f style='display:flex;width:100px;'><div id=fa style='min-width:50px;max-width:50px;height:10px'></div><div id=fb style='flex:1;height:10px'></div></div><div id=g style='display:grid;width:60px;grid-template-columns:repeat(3,20px);grid-auto-flow:dense;align-items:stretch'><div id=ga style='grid-column:span 2;height:10px'></div><div id=gb style='grid-column:span 2;height:10px'></div><div id=gc style='height:10px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(50, lot52_node("fa")->w);
    TEST_ASSERT_EQUAL(40, lot52_node("gc")->x);
    TEST_ASSERT_EQUAL(10, lot52_node("gc")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("gb")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_mix_65(void) {
    lot52_render("<body style='margin:0'><div id=f style='display:flex;width:100px;'><div id=fa style='min-width:30px;max-width:30px;height:10px'></div><div id=fb style='flex:1;height:10px'></div></div><div id=g style='display:grid;width:60px;grid-template-columns:repeat(3,20px);grid-auto-flow:dense;align-items:stretch'><div id=ga style='grid-column:span 2;height:12px'></div><div id=gb style='grid-column:span 2;height:12px'></div><div id=gc style='height:12px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(30, lot52_node("fa")->w);
    TEST_ASSERT_EQUAL(40, lot52_node("gc")->x);
    TEST_ASSERT_EQUAL(10, lot52_node("gc")->y);
    TEST_ASSERT_EQUAL(22, lot52_node("gb")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_mix_66(void) {
    lot52_render("<body style='margin:0'><div id=f style='display:flex;width:100px;'><div id=fa style='min-width:35px;max-width:35px;height:10px'></div><div id=fb style='flex:1;height:10px'></div></div><div id=g style='display:grid;width:60px;grid-template-columns:repeat(3,20px);grid-auto-flow:dense;align-items:stretch'><div id=ga style='grid-column:span 2;height:14px'></div><div id=gb style='grid-column:span 2;height:14px'></div><div id=gc style='height:14px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(35, lot52_node("fa")->w);
    TEST_ASSERT_EQUAL(40, lot52_node("gc")->x);
    TEST_ASSERT_EQUAL(10, lot52_node("gc")->y);
    TEST_ASSERT_EQUAL(24, lot52_node("gb")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_mix_67(void) {
    lot52_render("<body style='margin:0'><div id=f style='display:flex;width:100px;'><div id=fa style='min-width:40px;max-width:40px;height:10px'></div><div id=fb style='flex:1;height:10px'></div></div><div id=g style='display:grid;width:60px;grid-template-columns:repeat(3,20px);grid-auto-flow:dense;align-items:stretch'><div id=ga style='grid-column:span 2;height:16px'></div><div id=gb style='grid-column:span 2;height:16px'></div><div id=gc style='height:16px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(40, lot52_node("fa")->w);
    TEST_ASSERT_EQUAL(40, lot52_node("gc")->x);
    TEST_ASSERT_EQUAL(10, lot52_node("gc")->y);
    TEST_ASSERT_EQUAL(26, lot52_node("gb")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_mix_68(void) {
    lot52_render("<body style='margin:0'><div id=f style='display:flex;width:100px;'><div id=fa style='min-width:45px;max-width:45px;height:10px'></div><div id=fb style='flex:1;height:10px'></div></div><div id=g style='display:grid;width:60px;grid-template-columns:repeat(3,20px);grid-auto-flow:dense;align-items:stretch'><div id=ga style='grid-column:span 2;height:10px'></div><div id=gb style='grid-column:span 2;height:10px'></div><div id=gc style='height:10px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(45, lot52_node("fa")->w);
    TEST_ASSERT_EQUAL(40, lot52_node("gc")->x);
    TEST_ASSERT_EQUAL(10, lot52_node("gc")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("gb")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_mix_69(void) {
    lot52_render("<body style='margin:0'><div id=f style='display:flex;width:100px;'><div id=fa style='min-width:50px;max-width:50px;height:10px'></div><div id=fb style='flex:1;height:10px'></div></div><div id=g style='display:grid;width:60px;grid-template-columns:repeat(3,20px);grid-auto-flow:dense;align-items:stretch'><div id=ga style='grid-column:span 2;height:12px'></div><div id=gb style='grid-column:span 2;height:12px'></div><div id=gc style='height:12px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(50, lot52_node("fa")->w);
    TEST_ASSERT_EQUAL(40, lot52_node("gc")->x);
    TEST_ASSERT_EQUAL(10, lot52_node("gc")->y);
    TEST_ASSERT_EQUAL(22, lot52_node("gb")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_mix_70(void) {
    lot52_render("<body style='margin:0'><div id=f style='display:flex;width:100px;'><div id=fa style='min-width:30px;max-width:30px;height:10px'></div><div id=fb style='flex:1;height:10px'></div></div><div id=g style='display:grid;width:60px;grid-template-columns:repeat(3,20px);grid-auto-flow:dense;align-items:stretch'><div id=ga style='grid-column:span 2;height:14px'></div><div id=gb style='grid-column:span 2;height:14px'></div><div id=gc style='height:14px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(30, lot52_node("fa")->w);
    TEST_ASSERT_EQUAL(40, lot52_node("gc")->x);
    TEST_ASSERT_EQUAL(10, lot52_node("gc")->y);
    TEST_ASSERT_EQUAL(24, lot52_node("gb")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_mix_71(void) {
    lot52_render("<body style='margin:0'><div id=f style='display:flex;width:100px;'><div id=fa style='min-width:35px;max-width:35px;height:10px'></div><div id=fb style='flex:1;height:10px'></div></div><div id=g style='display:grid;width:60px;grid-template-columns:repeat(3,20px);grid-auto-flow:dense;align-items:stretch'><div id=ga style='grid-column:span 2;height:16px'></div><div id=gb style='grid-column:span 2;height:16px'></div><div id=gc style='height:16px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(35, lot52_node("fa")->w);
    TEST_ASSERT_EQUAL(40, lot52_node("gc")->x);
    TEST_ASSERT_EQUAL(10, lot52_node("gc")->y);
    TEST_ASSERT_EQUAL(26, lot52_node("gb")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_mix_72(void) {
    lot52_render("<body style='margin:0'><div id=f style='display:flex;width:100px;'><div id=fa style='min-width:40px;max-width:40px;height:10px'></div><div id=fb style='flex:1;height:10px'></div></div><div id=g style='display:grid;width:60px;grid-template-columns:repeat(3,20px);grid-auto-flow:dense;align-items:stretch'><div id=ga style='grid-column:span 2;height:10px'></div><div id=gb style='grid-column:span 2;height:10px'></div><div id=gc style='height:10px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(40, lot52_node("fa")->w);
    TEST_ASSERT_EQUAL(40, lot52_node("gc")->x);
    TEST_ASSERT_EQUAL(10, lot52_node("gc")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("gb")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_mix_73(void) {
    lot52_render("<body style='margin:0'><div id=f style='display:flex;width:100px;'><div id=fa style='min-width:45px;max-width:45px;height:10px'></div><div id=fb style='flex:1;height:10px'></div></div><div id=g style='display:grid;width:60px;grid-template-columns:repeat(3,20px);grid-auto-flow:dense;align-items:stretch'><div id=ga style='grid-column:span 2;height:12px'></div><div id=gb style='grid-column:span 2;height:12px'></div><div id=gc style='height:12px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(45, lot52_node("fa")->w);
    TEST_ASSERT_EQUAL(40, lot52_node("gc")->x);
    TEST_ASSERT_EQUAL(10, lot52_node("gc")->y);
    TEST_ASSERT_EQUAL(22, lot52_node("gb")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_mix_74(void) {
    lot52_render("<body style='margin:0'><div id=f style='display:flex;width:100px;'><div id=fa style='min-width:50px;max-width:50px;height:10px'></div><div id=fb style='flex:1;height:10px'></div></div><div id=g style='display:grid;width:60px;grid-template-columns:repeat(3,20px);grid-auto-flow:dense;align-items:stretch'><div id=ga style='grid-column:span 2;height:14px'></div><div id=gb style='grid-column:span 2;height:14px'></div><div id=gc style='height:14px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(50, lot52_node("fa")->w);
    TEST_ASSERT_EQUAL(40, lot52_node("gc")->x);
    TEST_ASSERT_EQUAL(10, lot52_node("gc")->y);
    TEST_ASSERT_EQUAL(24, lot52_node("gb")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_mix_75(void) {
    lot52_render("<body style='margin:0'><div id=f style='display:flex;width:100px;'><div id=fa style='min-width:30px;max-width:30px;height:10px'></div><div id=fb style='flex:1;height:10px'></div></div><div id=g style='display:grid;width:60px;grid-template-columns:repeat(3,20px);grid-auto-flow:dense;align-items:stretch'><div id=ga style='grid-column:span 2;height:16px'></div><div id=gb style='grid-column:span 2;height:16px'></div><div id=gc style='height:16px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(30, lot52_node("fa")->w);
    TEST_ASSERT_EQUAL(40, lot52_node("gc")->x);
    TEST_ASSERT_EQUAL(10, lot52_node("gc")->y);
    TEST_ASSERT_EQUAL(26, lot52_node("gb")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_mix_76(void) {
    lot52_render("<body style='margin:0'><div id=f style='display:flex;width:100px;'><div id=fa style='min-width:35px;max-width:35px;height:10px'></div><div id=fb style='flex:1;height:10px'></div></div><div id=g style='display:grid;width:60px;grid-template-columns:repeat(3,20px);grid-auto-flow:dense;align-items:stretch'><div id=ga style='grid-column:span 2;height:10px'></div><div id=gb style='grid-column:span 2;height:10px'></div><div id=gc style='height:10px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(35, lot52_node("fa")->w);
    TEST_ASSERT_EQUAL(40, lot52_node("gc")->x);
    TEST_ASSERT_EQUAL(10, lot52_node("gc")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("gb")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_mix_77(void) {
    lot52_render("<body style='margin:0'><div id=f style='display:flex;width:100px;'><div id=fa style='min-width:40px;max-width:40px;height:10px'></div><div id=fb style='flex:1;height:10px'></div></div><div id=g style='display:grid;width:60px;grid-template-columns:repeat(3,20px);grid-auto-flow:dense;align-items:stretch'><div id=ga style='grid-column:span 2;height:12px'></div><div id=gb style='grid-column:span 2;height:12px'></div><div id=gc style='height:12px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(40, lot52_node("fa")->w);
    TEST_ASSERT_EQUAL(40, lot52_node("gc")->x);
    TEST_ASSERT_EQUAL(10, lot52_node("gc")->y);
    TEST_ASSERT_EQUAL(22, lot52_node("gb")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_mix_78(void) {
    lot52_render("<body style='margin:0'><div id=f style='display:flex;width:100px;'><div id=fa style='min-width:45px;max-width:45px;height:10px'></div><div id=fb style='flex:1;height:10px'></div></div><div id=g style='display:grid;width:60px;grid-template-columns:repeat(3,20px);grid-auto-flow:dense;align-items:stretch'><div id=ga style='grid-column:span 2;height:14px'></div><div id=gb style='grid-column:span 2;height:14px'></div><div id=gc style='height:14px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(45, lot52_node("fa")->w);
    TEST_ASSERT_EQUAL(40, lot52_node("gc")->x);
    TEST_ASSERT_EQUAL(10, lot52_node("gc")->y);
    TEST_ASSERT_EQUAL(24, lot52_node("gb")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_mix_79(void) {
    lot52_render("<body style='margin:0'><div id=f style='display:flex;width:100px;'><div id=fa style='min-width:50px;max-width:50px;height:10px'></div><div id=fb style='flex:1;height:10px'></div></div><div id=g style='display:grid;width:60px;grid-template-columns:repeat(3,20px);grid-auto-flow:dense;align-items:stretch'><div id=ga style='grid-column:span 2;height:16px'></div><div id=gb style='grid-column:span 2;height:16px'></div><div id=gc style='height:16px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(50, lot52_node("fa")->w);
    TEST_ASSERT_EQUAL(40, lot52_node("gc")->x);
    TEST_ASSERT_EQUAL(10, lot52_node("gc")->y);
    TEST_ASSERT_EQUAL(26, lot52_node("gb")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_mix_80(void) {
    lot52_render("<body style='margin:0'><div id=f style='display:flex;width:100px;'><div id=fa style='min-width:30px;max-width:30px;height:10px'></div><div id=fb style='flex:1;height:10px'></div></div><div id=g style='display:grid;width:60px;grid-template-columns:repeat(3,20px);grid-auto-flow:dense;align-items:stretch'><div id=ga style='grid-column:span 2;height:10px'></div><div id=gb style='grid-column:span 2;height:10px'></div><div id=gc style='height:10px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(30, lot52_node("fa")->w);
    TEST_ASSERT_EQUAL(40, lot52_node("gc")->x);
    TEST_ASSERT_EQUAL(10, lot52_node("gc")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("gb")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_mix_81(void) {
    lot52_render("<body style='margin:0'><div id=f style='display:flex;width:100px;'><div id=fa style='min-width:35px;max-width:35px;height:10px'></div><div id=fb style='flex:1;height:10px'></div></div><div id=g style='display:grid;width:60px;grid-template-columns:repeat(3,20px);grid-auto-flow:dense;align-items:stretch'><div id=ga style='grid-column:span 2;height:12px'></div><div id=gb style='grid-column:span 2;height:12px'></div><div id=gc style='height:12px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(35, lot52_node("fa")->w);
    TEST_ASSERT_EQUAL(40, lot52_node("gc")->x);
    TEST_ASSERT_EQUAL(10, lot52_node("gc")->y);
    TEST_ASSERT_EQUAL(22, lot52_node("gb")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_mix_82(void) {
    lot52_render("<body style='margin:0'><div id=f style='display:flex;width:100px;'><div id=fa style='min-width:40px;max-width:40px;height:10px'></div><div id=fb style='flex:1;height:10px'></div></div><div id=g style='display:grid;width:60px;grid-template-columns:repeat(3,20px);grid-auto-flow:dense;align-items:stretch'><div id=ga style='grid-column:span 2;height:14px'></div><div id=gb style='grid-column:span 2;height:14px'></div><div id=gc style='height:14px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(40, lot52_node("fa")->w);
    TEST_ASSERT_EQUAL(40, lot52_node("gc")->x);
    TEST_ASSERT_EQUAL(10, lot52_node("gc")->y);
    TEST_ASSERT_EQUAL(24, lot52_node("gb")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_mix_83(void) {
    lot52_render("<body style='margin:0'><div id=f style='display:flex;width:100px;'><div id=fa style='min-width:45px;max-width:45px;height:10px'></div><div id=fb style='flex:1;height:10px'></div></div><div id=g style='display:grid;width:60px;grid-template-columns:repeat(3,20px);grid-auto-flow:dense;align-items:stretch'><div id=ga style='grid-column:span 2;height:16px'></div><div id=gb style='grid-column:span 2;height:16px'></div><div id=gc style='height:16px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(45, lot52_node("fa")->w);
    TEST_ASSERT_EQUAL(40, lot52_node("gc")->x);
    TEST_ASSERT_EQUAL(10, lot52_node("gc")->y);
    TEST_ASSERT_EQUAL(26, lot52_node("gb")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_mix_84(void) {
    lot52_render("<body style='margin:0'><div id=f style='display:flex;width:100px;'><div id=fa style='min-width:50px;max-width:50px;height:10px'></div><div id=fb style='flex:1;height:10px'></div></div><div id=g style='display:grid;width:60px;grid-template-columns:repeat(3,20px);grid-auto-flow:dense;align-items:stretch'><div id=ga style='grid-column:span 2;height:10px'></div><div id=gb style='grid-column:span 2;height:10px'></div><div id=gc style='height:10px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(50, lot52_node("fa")->w);
    TEST_ASSERT_EQUAL(40, lot52_node("gc")->x);
    TEST_ASSERT_EQUAL(10, lot52_node("gc")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("gb")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_mix_85(void) {
    lot52_render("<body style='margin:0'><div id=f style='display:flex;width:100px;'><div id=fa style='min-width:30px;max-width:30px;height:10px'></div><div id=fb style='flex:1;height:10px'></div></div><div id=g style='display:grid;width:60px;grid-template-columns:repeat(3,20px);grid-auto-flow:dense;align-items:stretch'><div id=ga style='grid-column:span 2;height:12px'></div><div id=gb style='grid-column:span 2;height:12px'></div><div id=gc style='height:12px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(30, lot52_node("fa")->w);
    TEST_ASSERT_EQUAL(40, lot52_node("gc")->x);
    TEST_ASSERT_EQUAL(10, lot52_node("gc")->y);
    TEST_ASSERT_EQUAL(22, lot52_node("gb")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_mix_86(void) {
    lot52_render("<body style='margin:0'><div id=f style='display:flex;width:100px;'><div id=fa style='min-width:35px;max-width:35px;height:10px'></div><div id=fb style='flex:1;height:10px'></div></div><div id=g style='display:grid;width:60px;grid-template-columns:repeat(3,20px);grid-auto-flow:dense;align-items:stretch'><div id=ga style='grid-column:span 2;height:14px'></div><div id=gb style='grid-column:span 2;height:14px'></div><div id=gc style='height:14px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(35, lot52_node("fa")->w);
    TEST_ASSERT_EQUAL(40, lot52_node("gc")->x);
    TEST_ASSERT_EQUAL(10, lot52_node("gc")->y);
    TEST_ASSERT_EQUAL(24, lot52_node("gb")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_mix_87(void) {
    lot52_render("<body style='margin:0'><div id=f style='display:flex;width:100px;'><div id=fa style='min-width:40px;max-width:40px;height:10px'></div><div id=fb style='flex:1;height:10px'></div></div><div id=g style='display:grid;width:60px;grid-template-columns:repeat(3,20px);grid-auto-flow:dense;align-items:stretch'><div id=ga style='grid-column:span 2;height:16px'></div><div id=gb style='grid-column:span 2;height:16px'></div><div id=gc style='height:16px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(40, lot52_node("fa")->w);
    TEST_ASSERT_EQUAL(40, lot52_node("gc")->x);
    TEST_ASSERT_EQUAL(10, lot52_node("gc")->y);
    TEST_ASSERT_EQUAL(26, lot52_node("gb")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_mix_88(void) {
    lot52_render("<body style='margin:0'><div id=f style='display:flex;width:100px;'><div id=fa style='min-width:45px;max-width:45px;height:10px'></div><div id=fb style='flex:1;height:10px'></div></div><div id=g style='display:grid;width:60px;grid-template-columns:repeat(3,20px);grid-auto-flow:dense;align-items:stretch'><div id=ga style='grid-column:span 2;height:10px'></div><div id=gb style='grid-column:span 2;height:10px'></div><div id=gc style='height:10px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(45, lot52_node("fa")->w);
    TEST_ASSERT_EQUAL(40, lot52_node("gc")->x);
    TEST_ASSERT_EQUAL(10, lot52_node("gc")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("gb")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_mix_89(void) {
    lot52_render("<body style='margin:0'><div id=f style='display:flex;width:100px;'><div id=fa style='min-width:50px;max-width:50px;height:10px'></div><div id=fb style='flex:1;height:10px'></div></div><div id=g style='display:grid;width:60px;grid-template-columns:repeat(3,20px);grid-auto-flow:dense;align-items:stretch'><div id=ga style='grid-column:span 2;height:12px'></div><div id=gb style='grid-column:span 2;height:12px'></div><div id=gc style='height:12px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(50, lot52_node("fa")->w);
    TEST_ASSERT_EQUAL(40, lot52_node("gc")->x);
    TEST_ASSERT_EQUAL(10, lot52_node("gc")->y);
    TEST_ASSERT_EQUAL(22, lot52_node("gb")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_mix_90(void) {
    lot52_render("<body style='margin:0'><div id=f style='display:flex;width:100px;'><div id=fa style='min-width:30px;max-width:30px;height:10px'></div><div id=fb style='flex:1;height:10px'></div></div><div id=g style='display:grid;width:60px;grid-template-columns:repeat(3,20px);grid-auto-flow:dense;align-items:stretch'><div id=ga style='grid-column:span 2;height:14px'></div><div id=gb style='grid-column:span 2;height:14px'></div><div id=gc style='height:14px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(30, lot52_node("fa")->w);
    TEST_ASSERT_EQUAL(40, lot52_node("gc")->x);
    TEST_ASSERT_EQUAL(10, lot52_node("gc")->y);
    TEST_ASSERT_EQUAL(24, lot52_node("gb")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_mix_91(void) {
    lot52_render("<body style='margin:0'><div id=f style='display:flex;width:100px;'><div id=fa style='min-width:35px;max-width:35px;height:10px'></div><div id=fb style='flex:1;height:10px'></div></div><div id=g style='display:grid;width:60px;grid-template-columns:repeat(3,20px);grid-auto-flow:dense;align-items:stretch'><div id=ga style='grid-column:span 2;height:16px'></div><div id=gb style='grid-column:span 2;height:16px'></div><div id=gc style='height:16px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(35, lot52_node("fa")->w);
    TEST_ASSERT_EQUAL(40, lot52_node("gc")->x);
    TEST_ASSERT_EQUAL(10, lot52_node("gc")->y);
    TEST_ASSERT_EQUAL(26, lot52_node("gb")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_mix_92(void) {
    lot52_render("<body style='margin:0'><div id=f style='display:flex;width:100px;'><div id=fa style='min-width:40px;max-width:40px;height:10px'></div><div id=fb style='flex:1;height:10px'></div></div><div id=g style='display:grid;width:60px;grid-template-columns:repeat(3,20px);grid-auto-flow:dense;align-items:stretch'><div id=ga style='grid-column:span 2;height:10px'></div><div id=gb style='grid-column:span 2;height:10px'></div><div id=gc style='height:10px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(40, lot52_node("fa")->w);
    TEST_ASSERT_EQUAL(40, lot52_node("gc")->x);
    TEST_ASSERT_EQUAL(10, lot52_node("gc")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("gb")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_mix_93(void) {
    lot52_render("<body style='margin:0'><div id=f style='display:flex;width:100px;'><div id=fa style='min-width:45px;max-width:45px;height:10px'></div><div id=fb style='flex:1;height:10px'></div></div><div id=g style='display:grid;width:60px;grid-template-columns:repeat(3,20px);grid-auto-flow:dense;align-items:stretch'><div id=ga style='grid-column:span 2;height:12px'></div><div id=gb style='grid-column:span 2;height:12px'></div><div id=gc style='height:12px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(45, lot52_node("fa")->w);
    TEST_ASSERT_EQUAL(40, lot52_node("gc")->x);
    TEST_ASSERT_EQUAL(10, lot52_node("gc")->y);
    TEST_ASSERT_EQUAL(22, lot52_node("gb")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_mix_94(void) {
    lot52_render("<body style='margin:0'><div id=f style='display:flex;width:100px;'><div id=fa style='min-width:50px;max-width:50px;height:10px'></div><div id=fb style='flex:1;height:10px'></div></div><div id=g style='display:grid;width:60px;grid-template-columns:repeat(3,20px);grid-auto-flow:dense;align-items:stretch'><div id=ga style='grid-column:span 2;height:14px'></div><div id=gb style='grid-column:span 2;height:14px'></div><div id=gc style='height:14px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(50, lot52_node("fa")->w);
    TEST_ASSERT_EQUAL(40, lot52_node("gc")->x);
    TEST_ASSERT_EQUAL(10, lot52_node("gc")->y);
    TEST_ASSERT_EQUAL(24, lot52_node("gb")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_mix_95(void) {
    lot52_render("<body style='margin:0'><div id=f style='display:flex;width:100px;'><div id=fa style='min-width:30px;max-width:30px;height:10px'></div><div id=fb style='flex:1;height:10px'></div></div><div id=g style='display:grid;width:60px;grid-template-columns:repeat(3,20px);grid-auto-flow:dense;align-items:stretch'><div id=ga style='grid-column:span 2;height:16px'></div><div id=gb style='grid-column:span 2;height:16px'></div><div id=gc style='height:16px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(30, lot52_node("fa")->w);
    TEST_ASSERT_EQUAL(40, lot52_node("gc")->x);
    TEST_ASSERT_EQUAL(10, lot52_node("gc")->y);
    TEST_ASSERT_EQUAL(26, lot52_node("gb")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_mix_96(void) {
    lot52_render("<body style='margin:0'><div id=f style='display:flex;width:100px;'><div id=fa style='min-width:35px;max-width:35px;height:10px'></div><div id=fb style='flex:1;height:10px'></div></div><div id=g style='display:grid;width:60px;grid-template-columns:repeat(3,20px);grid-auto-flow:dense;align-items:stretch'><div id=ga style='grid-column:span 2;height:10px'></div><div id=gb style='grid-column:span 2;height:10px'></div><div id=gc style='height:10px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(35, lot52_node("fa")->w);
    TEST_ASSERT_EQUAL(40, lot52_node("gc")->x);
    TEST_ASSERT_EQUAL(10, lot52_node("gc")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("gb")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_mix_97(void) {
    lot52_render("<body style='margin:0'><div id=f style='display:flex;width:100px;'><div id=fa style='min-width:40px;max-width:40px;height:10px'></div><div id=fb style='flex:1;height:10px'></div></div><div id=g style='display:grid;width:60px;grid-template-columns:repeat(3,20px);grid-auto-flow:dense;align-items:stretch'><div id=ga style='grid-column:span 2;height:12px'></div><div id=gb style='grid-column:span 2;height:12px'></div><div id=gc style='height:12px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(40, lot52_node("fa")->w);
    TEST_ASSERT_EQUAL(40, lot52_node("gc")->x);
    TEST_ASSERT_EQUAL(10, lot52_node("gc")->y);
    TEST_ASSERT_EQUAL(22, lot52_node("gb")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_mix_98(void) {
    lot52_render("<body style='margin:0'><div id=f style='display:flex;width:100px;'><div id=fa style='min-width:45px;max-width:45px;height:10px'></div><div id=fb style='flex:1;height:10px'></div></div><div id=g style='display:grid;width:60px;grid-template-columns:repeat(3,20px);grid-auto-flow:dense;align-items:stretch'><div id=ga style='grid-column:span 2;height:14px'></div><div id=gb style='grid-column:span 2;height:14px'></div><div id=gc style='height:14px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(45, lot52_node("fa")->w);
    TEST_ASSERT_EQUAL(40, lot52_node("gc")->x);
    TEST_ASSERT_EQUAL(10, lot52_node("gc")->y);
    TEST_ASSERT_EQUAL(24, lot52_node("gb")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_mix_99(void) {
    lot52_render("<body style='margin:0'><div id=f style='display:flex;width:100px;'><div id=fa style='min-width:50px;max-width:50px;height:10px'></div><div id=fb style='flex:1;height:10px'></div></div><div id=g style='display:grid;width:60px;grid-template-columns:repeat(3,20px);grid-auto-flow:dense;align-items:stretch'><div id=ga style='grid-column:span 2;height:16px'></div><div id=gb style='grid-column:span 2;height:16px'></div><div id=gc style='height:16px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(50, lot52_node("fa")->w);
    TEST_ASSERT_EQUAL(40, lot52_node("gc")->x);
    TEST_ASSERT_EQUAL(10, lot52_node("gc")->y);
    TEST_ASSERT_EQUAL(26, lot52_node("gb")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_mix_100(void) {
    lot52_render("<body style='margin:0'><div id=f style='display:flex;width:100px;'><div id=fa style='min-width:30px;max-width:30px;height:10px'></div><div id=fb style='flex:1;height:10px'></div></div><div id=g style='display:grid;width:60px;grid-template-columns:repeat(3,20px);grid-auto-flow:dense;align-items:stretch'><div id=ga style='grid-column:span 2;height:10px'></div><div id=gb style='grid-column:span 2;height:10px'></div><div id=gc style='height:10px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(30, lot52_node("fa")->w);
    TEST_ASSERT_EQUAL(40, lot52_node("gc")->x);
    TEST_ASSERT_EQUAL(10, lot52_node("gc")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("gb")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_mm_sys_49(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:120px;grid-template-columns:minmax(10px,1fr) 1fr;align-items:stretch'><div id=a style='height:8px'></div><div id=b style='height:8px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(60, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(60, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_mm_sys_50(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:120px;grid-template-columns:minmax(15px,1fr) 1fr;align-items:stretch'><div id=a style='height:8px'></div><div id=b style='height:8px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(60, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(60, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_mm_sys_51(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:120px;grid-template-columns:minmax(20px,1fr) 1fr;align-items:stretch'><div id=a style='height:8px'></div><div id=b style='height:8px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(60, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(60, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_mm_sys_52(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:120px;grid-template-columns:minmax(25px,1fr) 1fr;align-items:stretch'><div id=a style='height:8px'></div><div id=b style='height:8px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(60, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(60, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_mm_sys_53(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:120px;grid-template-columns:minmax(30px,1fr) 1fr;align-items:stretch'><div id=a style='height:8px'></div><div id=b style='height:8px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(60, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(60, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_mm_sys_54(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:120px;grid-template-columns:minmax(35px,1fr) 1fr;align-items:stretch'><div id=a style='height:8px'></div><div id=b style='height:8px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(60, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(60, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_mm_sys_55(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:120px;grid-template-columns:minmax(40px,1fr) 1fr;align-items:stretch'><div id=a style='height:8px'></div><div id=b style='height:8px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(60, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(60, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_mm_sys_56(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:120px;grid-template-columns:minmax(45px,1fr) 1fr;align-items:stretch'><div id=a style='height:8px'></div><div id=b style='height:8px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(60, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(60, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_mm_sys_57(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:120px;grid-template-columns:minmax(50px,1fr) 1fr;align-items:stretch'><div id=a style='height:8px'></div><div id=b style='height:8px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(60, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(60, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_mm_sys_58(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:120px;grid-template-columns:minmax(55px,1fr) 1fr;align-items:stretch'><div id=a style='height:8px'></div><div id=b style='height:8px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(60, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(60, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_mm_sys_59(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:120px;grid-template-columns:minmax(60px,1fr) 1fr;align-items:stretch'><div id=a style='height:8px'></div><div id=b style='height:8px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(60, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(60, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_mm_sys_60(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:120px;grid-template-columns:minmax(65px,1fr) 1fr;align-items:stretch'><div id=a style='height:8px'></div><div id=b style='height:8px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(65, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(55, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_mm_sys_61(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:120px;grid-template-columns:minmax(70px,1fr) 1fr;align-items:stretch'><div id=a style='height:8px'></div><div id=b style='height:8px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(70, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(50, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_mm_sys_62(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:120px;grid-template-columns:minmax(75px,1fr) 1fr;align-items:stretch'><div id=a style='height:8px'></div><div id=b style='height:8px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(75, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(45, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l79_mm_sys_63(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:120px;grid-template-columns:minmax(80px,1fr) 1fr;align-items:stretch'><div id=a style='height:8px'></div><div id=b style='height:8px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(80, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(40, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void lot79_edges_run(void) {
    RUN_TEST(test_l79_minw_1);
    RUN_TEST(test_l79_minw_2);
    RUN_TEST(test_l79_minw_3);
    RUN_TEST(test_l79_minw_4);
    RUN_TEST(test_l79_minw_5);
    RUN_TEST(test_l79_minw_6);
    RUN_TEST(test_l79_minw_7);
    RUN_TEST(test_l79_minw_8);
    RUN_TEST(test_l79_minw_9);
    RUN_TEST(test_l79_minw_10);
    RUN_TEST(test_l79_minw_11);
    RUN_TEST(test_l79_minw_12);
    RUN_TEST(test_l79_minw_13);
    RUN_TEST(test_l79_minw_14);
    RUN_TEST(test_l79_minw_15);
    RUN_TEST(test_l79_maxw_16);
    RUN_TEST(test_l79_maxw_17);
    RUN_TEST(test_l79_maxw_18);
    RUN_TEST(test_l79_maxw_19);
    RUN_TEST(test_l79_maxw_20);
    RUN_TEST(test_l79_maxw_21);
    RUN_TEST(test_l79_maxw_22);
    RUN_TEST(test_l79_maxw_23);
    RUN_TEST(test_l79_maxw_24);
    RUN_TEST(test_l79_minh_25);
    RUN_TEST(test_l79_minh_26);
    RUN_TEST(test_l79_minh_27);
    RUN_TEST(test_l79_minh_28);
    RUN_TEST(test_l79_minh_29);
    RUN_TEST(test_l79_minh_30);
    RUN_TEST(test_l79_maxh_31);
    RUN_TEST(test_l79_maxh_32);
    RUN_TEST(test_l79_maxh_33);
    RUN_TEST(test_l79_maxh_34);
    RUN_TEST(test_l79_maxh_35);
    RUN_TEST(test_l79_maxh_36);
    RUN_TEST(test_l79_maxh_37);
    RUN_TEST(test_l79_maxh_38);
    RUN_TEST(test_l79_maxh_39);
    RUN_TEST(test_l79_flex_mm_40);
    RUN_TEST(test_l79_flex_mm_41);
    RUN_TEST(test_l79_flex_mm_42);
    RUN_TEST(test_l79_flex_mm_43);
    RUN_TEST(test_l79_flex_mm_44);
    RUN_TEST(test_l79_flex_mm_45);
    RUN_TEST(test_l79_flex_mm_46);
    RUN_TEST(test_l79_flex_mm_47);
    RUN_TEST(test_l79_flex_mm_48);
    RUN_TEST(test_l79_mix_1);
    RUN_TEST(test_l79_mix_2);
    RUN_TEST(test_l79_mix_3);
    RUN_TEST(test_l79_mix_4);
    RUN_TEST(test_l79_mix_5);
    RUN_TEST(test_l79_mix_6);
    RUN_TEST(test_l79_mix_7);
    RUN_TEST(test_l79_mix_8);
    RUN_TEST(test_l79_mix_9);
    RUN_TEST(test_l79_mix_10);
    RUN_TEST(test_l79_mix_11);
    RUN_TEST(test_l79_mix_12);
    RUN_TEST(test_l79_mix_13);
    RUN_TEST(test_l79_mix_14);
    RUN_TEST(test_l79_mix_15);
    RUN_TEST(test_l79_mix_16);
    RUN_TEST(test_l79_mix_17);
    RUN_TEST(test_l79_mix_18);
    RUN_TEST(test_l79_mix_19);
    RUN_TEST(test_l79_mix_20);
    RUN_TEST(test_l79_mix_21);
    RUN_TEST(test_l79_mix_22);
    RUN_TEST(test_l79_mix_23);
    RUN_TEST(test_l79_mix_24);
    RUN_TEST(test_l79_mix_25);
    RUN_TEST(test_l79_mix_26);
    RUN_TEST(test_l79_mix_27);
    RUN_TEST(test_l79_mix_28);
    RUN_TEST(test_l79_mix_29);
    RUN_TEST(test_l79_mix_30);
    RUN_TEST(test_l79_mix_31);
    RUN_TEST(test_l79_mix_32);
    RUN_TEST(test_l79_mix_33);
    RUN_TEST(test_l79_mix_34);
    RUN_TEST(test_l79_mix_35);
    RUN_TEST(test_l79_mix_36);
    RUN_TEST(test_l79_mix_37);
    RUN_TEST(test_l79_mix_38);
    RUN_TEST(test_l79_mix_39);
    RUN_TEST(test_l79_mix_40);
    RUN_TEST(test_l79_mix_41);
    RUN_TEST(test_l79_mix_42);
    RUN_TEST(test_l79_mix_43);
    RUN_TEST(test_l79_mix_44);
    RUN_TEST(test_l79_mix_45);
    RUN_TEST(test_l79_mix_46);
    RUN_TEST(test_l79_mix_47);
    RUN_TEST(test_l79_mix_48);
    RUN_TEST(test_l79_mix_49);
    RUN_TEST(test_l79_mix_50);
    RUN_TEST(test_l79_mix_51);
    RUN_TEST(test_l79_mix_52);
    RUN_TEST(test_l79_mix_53);
    RUN_TEST(test_l79_mix_54);
    RUN_TEST(test_l79_mix_55);
    RUN_TEST(test_l79_mix_56);
    RUN_TEST(test_l79_mix_57);
    RUN_TEST(test_l79_mix_58);
    RUN_TEST(test_l79_mix_59);
    RUN_TEST(test_l79_mix_60);
    RUN_TEST(test_l79_mix_61);
    RUN_TEST(test_l79_mix_62);
    RUN_TEST(test_l79_mix_63);
    RUN_TEST(test_l79_mix_64);
    RUN_TEST(test_l79_mix_65);
    RUN_TEST(test_l79_mix_66);
    RUN_TEST(test_l79_mix_67);
    RUN_TEST(test_l79_mix_68);
    RUN_TEST(test_l79_mix_69);
    RUN_TEST(test_l79_mix_70);
    RUN_TEST(test_l79_mix_71);
    RUN_TEST(test_l79_mix_72);
    RUN_TEST(test_l79_mix_73);
    RUN_TEST(test_l79_mix_74);
    RUN_TEST(test_l79_mix_75);
    RUN_TEST(test_l79_mix_76);
    RUN_TEST(test_l79_mix_77);
    RUN_TEST(test_l79_mix_78);
    RUN_TEST(test_l79_mix_79);
    RUN_TEST(test_l79_mix_80);
    RUN_TEST(test_l79_mix_81);
    RUN_TEST(test_l79_mix_82);
    RUN_TEST(test_l79_mix_83);
    RUN_TEST(test_l79_mix_84);
    RUN_TEST(test_l79_mix_85);
    RUN_TEST(test_l79_mix_86);
    RUN_TEST(test_l79_mix_87);
    RUN_TEST(test_l79_mix_88);
    RUN_TEST(test_l79_mix_89);
    RUN_TEST(test_l79_mix_90);
    RUN_TEST(test_l79_mix_91);
    RUN_TEST(test_l79_mix_92);
    RUN_TEST(test_l79_mix_93);
    RUN_TEST(test_l79_mix_94);
    RUN_TEST(test_l79_mix_95);
    RUN_TEST(test_l79_mix_96);
    RUN_TEST(test_l79_mix_97);
    RUN_TEST(test_l79_mix_98);
    RUN_TEST(test_l79_mix_99);
    RUN_TEST(test_l79_mix_100);
    RUN_TEST(test_l79_mm_sys_49);
    RUN_TEST(test_l79_mm_sys_50);
    RUN_TEST(test_l79_mm_sys_51);
    RUN_TEST(test_l79_mm_sys_52);
    RUN_TEST(test_l79_mm_sys_53);
    RUN_TEST(test_l79_mm_sys_54);
    RUN_TEST(test_l79_mm_sys_55);
    RUN_TEST(test_l79_mm_sys_56);
    RUN_TEST(test_l79_mm_sys_57);
    RUN_TEST(test_l79_mm_sys_58);
    RUN_TEST(test_l79_mm_sys_59);
    RUN_TEST(test_l79_mm_sys_60);
    RUN_TEST(test_l79_mm_sys_61);
    RUN_TEST(test_l79_mm_sys_62);
    RUN_TEST(test_l79_mm_sys_63);
}
