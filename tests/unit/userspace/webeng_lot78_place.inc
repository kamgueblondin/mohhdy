/* Lot 78 contract: we_doc_t stays under 1 MiB; place-* is CSS only. */
static void test_l78_contract_doc_size(void) {
    TEST_ASSERT_TRUE(sizeof(we_doc_t) <= WE_DOCUMENT_MAX);
    TEST_ASSERT_TRUE(sizeof(we_doc_t) < 1048576u);
}

/* Lot 78: place-items / place-content / place-self + hand checks. */
/* Uses lot52_render / lot52_node from webeng_lot52.inc. */

static void test_l78_place_items_one_keyword(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:80px;height:40px;"
        "grid-template-columns:80px;grid-template-rows:40px;place-items:end'>"
        "<div id=a style='width:20px;height:10px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(WE_PACK_END, lot52_node("g")->align_items);
    TEST_ASSERT_EQUAL(WE_PACK_END, lot52_node("g")->justify_items);
    TEST_ASSERT_EQUAL(60, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_place_items_two_keywords(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:80px;height:40px;"
        "grid-template-columns:80px;grid-template-rows:40px;place-items:start end'>"
        "<div id=a style='width:20px;height:10px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(WE_PACK_START, lot52_node("g")->align_items);
    TEST_ASSERT_EQUAL(WE_PACK_END, lot52_node("g")->justify_items);
    TEST_ASSERT_EQUAL(60, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_place_content_center(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:100px;height:60px;"
        "grid-template-columns:40px;grid-template-rows:20px;place-content:center'>"
        "<div id=a style='height:20px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(WE_PACK_CENTER, lot52_node("g")->align_content);
    TEST_ASSERT_EQUAL(WE_PACK_CENTER, lot52_node("g")->justify_content);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_place_self_overrides(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:80px;height:40px;"
        "grid-template-columns:80px;grid-template-rows:40px;place-items:start'>"
        "<div id=a style='width:20px;height:10px;place-self:end'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(WE_PACK_END, lot52_node("a")->align_self);
    TEST_ASSERT_EQUAL(WE_PACK_END, lot52_node("a")->justify_self);
    TEST_ASSERT_EQUAL(60, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_place_items_refuses_bogus(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;place-items:bogus'>"
        "<div id=a>A</div></div></body>", 320);
    TEST_ASSERT_TRUE(g_doc.ndiag > 0);
}

static void test_l78_place_sys_1(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:70px;height:48px;grid-template-columns:70px;grid-template-rows:48px;place-items:center'><div id=a style='width:12px;height:8px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(29, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_place_sys_2(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:80px;height:56px;grid-template-columns:80px;grid-template-rows:56px;place-items:start'><div id=a style='width:14px;height:10px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_place_sys_3(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:90px;height:64px;grid-template-columns:90px;grid-template-rows:64px;place-items:end'><div id=a style='width:16px;height:12px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(74, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(52, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_place_sys_4(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:100px;height:40px;grid-template-columns:100px;grid-template-rows:40px;place-items:center'><div id=a style='width:18px;height:14px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(41, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(13, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_place_sys_5(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:60px;height:48px;grid-template-columns:60px;grid-template-rows:48px;place-items:start'><div id=a style='width:20px;height:6px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_place_sys_6(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:70px;height:56px;grid-template-columns:70px;grid-template-rows:56px;place-items:end'><div id=a style='width:22px;height:8px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(48, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(48, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_place_sys_7(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:80px;height:64px;grid-template-columns:80px;grid-template-rows:64px;place-items:center'><div id=a style='width:10px;height:10px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(35, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(27, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_place_sys_8(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:90px;height:40px;grid-template-columns:90px;grid-template-rows:40px;place-items:start'><div id=a style='width:12px;height:12px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_place_sys_9(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:100px;height:48px;grid-template-columns:100px;grid-template-rows:48px;place-items:end'><div id=a style='width:14px;height:14px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(86, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(34, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_place_sys_10(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:60px;height:56px;grid-template-columns:60px;grid-template-rows:56px;place-items:center'><div id=a style='width:16px;height:6px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(22, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(25, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_place_sys_11(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:70px;height:64px;grid-template-columns:70px;grid-template-rows:64px;place-items:start'><div id=a style='width:18px;height:8px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_place_sys_12(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:80px;height:40px;grid-template-columns:80px;grid-template-rows:40px;place-items:end'><div id=a style='width:20px;height:10px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(60, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_place_sys_13(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:90px;height:48px;grid-template-columns:90px;grid-template-rows:48px;place-items:center'><div id=a style='width:22px;height:12px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(34, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(18, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_place_sys_14(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:100px;height:56px;grid-template-columns:100px;grid-template-rows:56px;place-items:start'><div id=a style='width:10px;height:14px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_place_sys_15(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:60px;height:64px;grid-template-columns:60px;grid-template-rows:64px;place-items:end'><div id=a style='width:12px;height:6px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(48, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(58, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_place_sys_16(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:70px;height:40px;grid-template-columns:70px;grid-template-rows:40px;place-items:center'><div id=a style='width:14px;height:8px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(28, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(16, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_place_sys_17(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:80px;height:48px;grid-template-columns:80px;grid-template-rows:48px;place-items:start'><div id=a style='width:16px;height:10px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_place_sys_18(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:90px;height:56px;grid-template-columns:90px;grid-template-rows:56px;place-items:end'><div id=a style='width:18px;height:12px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(72, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(44, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_place_sys_19(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:100px;height:64px;grid-template-columns:100px;grid-template-rows:64px;place-items:center'><div id=a style='width:20px;height:14px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(40, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(25, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_place_sys_20(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:60px;height:40px;grid-template-columns:60px;grid-template-rows:40px;place-items:start'><div id=a style='width:22px;height:6px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_place_sys_21(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:70px;height:48px;grid-template-columns:70px;grid-template-rows:48px;place-items:end'><div id=a style='width:10px;height:8px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(60, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(40, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_place_sys_22(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:80px;height:56px;grid-template-columns:80px;grid-template-rows:56px;place-items:center'><div id=a style='width:12px;height:10px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(34, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(23, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_place_sys_23(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:90px;height:64px;grid-template-columns:90px;grid-template-rows:64px;place-items:start'><div id=a style='width:14px;height:12px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_place_sys_24(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:100px;height:40px;grid-template-columns:100px;grid-template-rows:40px;place-items:end'><div id=a style='width:16px;height:14px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(84, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(26, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_place_sys_25(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:60px;height:48px;grid-template-columns:60px;grid-template-rows:48px;place-items:center'><div id=a style='width:18px;height:6px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(21, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(21, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_place_sys_26(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:70px;height:56px;grid-template-columns:70px;grid-template-rows:56px;place-items:start'><div id=a style='width:20px;height:8px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_place_sys_27(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:80px;height:64px;grid-template-columns:80px;grid-template-rows:64px;place-items:end'><div id=a style='width:22px;height:10px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(58, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(54, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_place_sys_28(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:90px;height:40px;grid-template-columns:90px;grid-template-rows:40px;place-items:center'><div id=a style='width:10px;height:12px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(40, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(14, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_place_sys_29(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:100px;height:48px;grid-template-columns:100px;grid-template-rows:48px;place-items:start'><div id=a style='width:12px;height:14px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_place_sys_30(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:60px;height:56px;grid-template-columns:60px;grid-template-rows:56px;place-items:end'><div id=a style='width:14px;height:6px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(46, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(50, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_place_sys_31(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:70px;height:64px;grid-template-columns:70px;grid-template-rows:64px;place-items:center'><div id=a style='width:16px;height:8px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(27, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(28, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_place_sys_32(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:80px;height:40px;grid-template-columns:80px;grid-template-rows:40px;place-items:start'><div id=a style='width:18px;height:10px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_place_sys_33(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:90px;height:48px;grid-template-columns:90px;grid-template-rows:48px;place-items:end'><div id=a style='width:20px;height:12px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(70, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(36, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_place_sys_34(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:100px;height:56px;grid-template-columns:100px;grid-template-rows:56px;place-items:center'><div id=a style='width:22px;height:14px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(39, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(21, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_place_sys_35(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:60px;height:64px;grid-template-columns:60px;grid-template-rows:64px;place-items:start'><div id=a style='width:10px;height:6px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_place_sys_36(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:70px;height:40px;grid-template-columns:70px;grid-template-rows:40px;place-items:end'><div id=a style='width:12px;height:8px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(58, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(32, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_place_sys_37(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:80px;height:48px;grid-template-columns:80px;grid-template-rows:48px;place-items:center'><div id=a style='width:14px;height:10px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(33, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(19, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_place_sys_38(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:90px;height:56px;grid-template-columns:90px;grid-template-rows:56px;place-items:start'><div id=a style='width:16px;height:12px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_place_sys_39(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:100px;height:64px;grid-template-columns:100px;grid-template-rows:64px;place-items:end'><div id=a style='width:18px;height:14px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(82, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(50, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_place_sys_40(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:60px;height:40px;grid-template-columns:60px;grid-template-rows:40px;place-items:center'><div id=a style='width:20px;height:6px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(17, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_place_sys_41(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:70px;height:48px;grid-template-columns:70px;grid-template-rows:48px;place-items:start'><div id=a style='width:22px;height:8px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_place_sys_42(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:80px;height:56px;grid-template-columns:80px;grid-template-rows:56px;place-items:end'><div id=a style='width:10px;height:10px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(70, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(46, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_place_sys_43(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:90px;height:64px;grid-template-columns:90px;grid-template-rows:64px;place-items:center'><div id=a style='width:12px;height:12px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(39, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(26, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_place_sys_44(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:100px;height:40px;grid-template-columns:100px;grid-template-rows:40px;place-items:start'><div id=a style='width:14px;height:14px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_place_sys_45(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:60px;height:48px;grid-template-columns:60px;grid-template-rows:48px;place-items:end'><div id=a style='width:16px;height:6px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(44, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(42, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_place_sys_46(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:70px;height:56px;grid-template-columns:70px;grid-template-rows:56px;place-items:center'><div id=a style='width:18px;height:8px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(26, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(24, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_place_sys_47(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:80px;height:64px;grid-template-columns:80px;grid-template-rows:64px;place-items:start'><div id=a style='width:20px;height:10px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_place_sys_48(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:90px;height:40px;grid-template-columns:90px;grid-template-rows:40px;place-items:end'><div id=a style='width:22px;height:12px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(68, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(28, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_place_sys_49(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:100px;height:48px;grid-template-columns:100px;grid-template-rows:48px;place-items:center'><div id=a style='width:10px;height:14px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(45, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(17, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_place_sys_50(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:60px;height:56px;grid-template-columns:60px;grid-template-rows:56px;place-items:start'><div id=a style='width:12px;height:6px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_place_sys_51(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:70px;height:64px;grid-template-columns:70px;grid-template-rows:64px;place-items:end'><div id=a style='width:14px;height:8px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(56, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(56, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_place_sys_52(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:80px;height:40px;grid-template-columns:80px;grid-template-rows:40px;place-items:center'><div id=a style='width:16px;height:10px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(32, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(15, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_place_sys_53(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:90px;height:48px;grid-template-columns:90px;grid-template-rows:48px;place-items:start'><div id=a style='width:18px;height:12px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_place_sys_54(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:100px;height:56px;grid-template-columns:100px;grid-template-rows:56px;place-items:end'><div id=a style='width:20px;height:14px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(80, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(42, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_place_sys_55(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:60px;height:64px;grid-template-columns:60px;grid-template-rows:64px;place-items:center'><div id=a style='width:22px;height:6px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(19, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(29, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_place_sys_56(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:70px;height:40px;grid-template-columns:70px;grid-template-rows:40px;place-items:start'><div id=a style='width:10px;height:8px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_place_sys_57(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:80px;height:48px;grid-template-columns:80px;grid-template-rows:48px;place-items:end'><div id=a style='width:12px;height:10px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(68, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(38, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_place_sys_58(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:90px;height:56px;grid-template-columns:90px;grid-template-rows:56px;place-items:center'><div id=a style='width:14px;height:12px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(38, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(22, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_place_sys_59(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:100px;height:64px;grid-template-columns:100px;grid-template-rows:64px;place-items:start'><div id=a style='width:16px;height:14px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_place_sys_60(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:60px;height:40px;grid-template-columns:60px;grid-template-rows:40px;place-items:end'><div id=a style='width:18px;height:6px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(42, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(34, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_place_sys_61(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:70px;height:48px;grid-template-columns:70px;grid-template-rows:48px;place-items:center'><div id=a style='width:20px;height:8px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(25, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_place_sys_62(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:80px;height:56px;grid-template-columns:80px;grid-template-rows:56px;place-items:start'><div id=a style='width:22px;height:10px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_place_sys_63(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:90px;height:64px;grid-template-columns:90px;grid-template-rows:64px;place-items:end'><div id=a style='width:10px;height:12px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(80, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(52, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_place_sys_64(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:100px;height:40px;grid-template-columns:100px;grid-template-rows:40px;place-items:center'><div id=a style='width:12px;height:14px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(44, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(13, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_place_sys_65(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:60px;height:48px;grid-template-columns:60px;grid-template-rows:48px;place-items:start'><div id=a style='width:14px;height:6px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_place_sys_66(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:70px;height:56px;grid-template-columns:70px;grid-template-rows:56px;place-items:end'><div id=a style='width:16px;height:8px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(54, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(48, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_place_sys_67(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:80px;height:64px;grid-template-columns:80px;grid-template-rows:64px;place-items:center'><div id=a style='width:18px;height:10px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(31, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(27, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_place_sys_68(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:90px;height:40px;grid-template-columns:90px;grid-template-rows:40px;place-items:start'><div id=a style='width:20px;height:12px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_place_sys_69(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:100px;height:48px;grid-template-columns:100px;grid-template-rows:48px;place-items:end'><div id=a style='width:22px;height:14px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(78, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(34, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_place_sys_70(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:60px;height:56px;grid-template-columns:60px;grid-template-rows:56px;place-items:center'><div id=a style='width:10px;height:6px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(25, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(25, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_place_sys_71(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:70px;height:64px;grid-template-columns:70px;grid-template-rows:64px;place-items:start'><div id=a style='width:12px;height:8px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_place_sys_72(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:80px;height:40px;grid-template-columns:80px;grid-template-rows:40px;place-items:end'><div id=a style='width:14px;height:10px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(66, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_place_sys_73(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:90px;height:48px;grid-template-columns:90px;grid-template-rows:48px;place-items:center'><div id=a style='width:16px;height:12px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(37, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(18, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_place_sys_74(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:100px;height:56px;grid-template-columns:100px;grid-template-rows:56px;place-items:start'><div id=a style='width:18px;height:14px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_place_sys_75(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:60px;height:64px;grid-template-columns:60px;grid-template-rows:64px;place-items:end'><div id=a style='width:20px;height:6px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(40, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(58, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_place_sys_76(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:70px;height:40px;grid-template-columns:70px;grid-template-rows:40px;place-items:center'><div id=a style='width:22px;height:8px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(24, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(16, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_place_sys_77(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:80px;height:48px;grid-template-columns:80px;grid-template-rows:48px;place-items:start'><div id=a style='width:10px;height:10px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_place_sys_78(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:90px;height:56px;grid-template-columns:90px;grid-template-rows:56px;place-items:end'><div id=a style='width:12px;height:12px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(78, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(44, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_place_sys_79(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:100px;height:64px;grid-template-columns:100px;grid-template-rows:64px;place-items:center'><div id=a style='width:14px;height:14px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(43, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(25, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l78_place_sys_80(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:60px;height:40px;grid-template-columns:60px;grid-template-rows:40px;place-items:start'><div id=a style='width:16px;height:6px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void lot78_place_run(void) {
    RUN_TEST(test_l78_contract_doc_size);
    RUN_TEST(test_l78_place_items_one_keyword);
    RUN_TEST(test_l78_place_items_two_keywords);
    RUN_TEST(test_l78_place_content_center);
    RUN_TEST(test_l78_place_self_overrides);
    RUN_TEST(test_l78_place_items_refuses_bogus);
    RUN_TEST(test_l78_place_sys_1);
    RUN_TEST(test_l78_place_sys_2);
    RUN_TEST(test_l78_place_sys_3);
    RUN_TEST(test_l78_place_sys_4);
    RUN_TEST(test_l78_place_sys_5);
    RUN_TEST(test_l78_place_sys_6);
    RUN_TEST(test_l78_place_sys_7);
    RUN_TEST(test_l78_place_sys_8);
    RUN_TEST(test_l78_place_sys_9);
    RUN_TEST(test_l78_place_sys_10);
    RUN_TEST(test_l78_place_sys_11);
    RUN_TEST(test_l78_place_sys_12);
    RUN_TEST(test_l78_place_sys_13);
    RUN_TEST(test_l78_place_sys_14);
    RUN_TEST(test_l78_place_sys_15);
    RUN_TEST(test_l78_place_sys_16);
    RUN_TEST(test_l78_place_sys_17);
    RUN_TEST(test_l78_place_sys_18);
    RUN_TEST(test_l78_place_sys_19);
    RUN_TEST(test_l78_place_sys_20);
    RUN_TEST(test_l78_place_sys_21);
    RUN_TEST(test_l78_place_sys_22);
    RUN_TEST(test_l78_place_sys_23);
    RUN_TEST(test_l78_place_sys_24);
    RUN_TEST(test_l78_place_sys_25);
    RUN_TEST(test_l78_place_sys_26);
    RUN_TEST(test_l78_place_sys_27);
    RUN_TEST(test_l78_place_sys_28);
    RUN_TEST(test_l78_place_sys_29);
    RUN_TEST(test_l78_place_sys_30);
    RUN_TEST(test_l78_place_sys_31);
    RUN_TEST(test_l78_place_sys_32);
    RUN_TEST(test_l78_place_sys_33);
    RUN_TEST(test_l78_place_sys_34);
    RUN_TEST(test_l78_place_sys_35);
    RUN_TEST(test_l78_place_sys_36);
    RUN_TEST(test_l78_place_sys_37);
    RUN_TEST(test_l78_place_sys_38);
    RUN_TEST(test_l78_place_sys_39);
    RUN_TEST(test_l78_place_sys_40);
    RUN_TEST(test_l78_place_sys_41);
    RUN_TEST(test_l78_place_sys_42);
    RUN_TEST(test_l78_place_sys_43);
    RUN_TEST(test_l78_place_sys_44);
    RUN_TEST(test_l78_place_sys_45);
    RUN_TEST(test_l78_place_sys_46);
    RUN_TEST(test_l78_place_sys_47);
    RUN_TEST(test_l78_place_sys_48);
    RUN_TEST(test_l78_place_sys_49);
    RUN_TEST(test_l78_place_sys_50);
    RUN_TEST(test_l78_place_sys_51);
    RUN_TEST(test_l78_place_sys_52);
    RUN_TEST(test_l78_place_sys_53);
    RUN_TEST(test_l78_place_sys_54);
    RUN_TEST(test_l78_place_sys_55);
    RUN_TEST(test_l78_place_sys_56);
    RUN_TEST(test_l78_place_sys_57);
    RUN_TEST(test_l78_place_sys_58);
    RUN_TEST(test_l78_place_sys_59);
    RUN_TEST(test_l78_place_sys_60);
    RUN_TEST(test_l78_place_sys_61);
    RUN_TEST(test_l78_place_sys_62);
    RUN_TEST(test_l78_place_sys_63);
    RUN_TEST(test_l78_place_sys_64);
    RUN_TEST(test_l78_place_sys_65);
    RUN_TEST(test_l78_place_sys_66);
    RUN_TEST(test_l78_place_sys_67);
    RUN_TEST(test_l78_place_sys_68);
    RUN_TEST(test_l78_place_sys_69);
    RUN_TEST(test_l78_place_sys_70);
    RUN_TEST(test_l78_place_sys_71);
    RUN_TEST(test_l78_place_sys_72);
    RUN_TEST(test_l78_place_sys_73);
    RUN_TEST(test_l78_place_sys_74);
    RUN_TEST(test_l78_place_sys_75);
    RUN_TEST(test_l78_place_sys_76);
    RUN_TEST(test_l78_place_sys_77);
    RUN_TEST(test_l78_place_sys_78);
    RUN_TEST(test_l78_place_sys_79);
    RUN_TEST(test_l78_place_sys_80);
}
