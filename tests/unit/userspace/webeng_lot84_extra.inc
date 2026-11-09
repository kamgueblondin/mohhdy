/* Lot 84 hand-written corners: the shared pool, the 0..8 cap, a refused
 * negative width, ordered clearing definite-first, and the document cap. */
static void test_l84_border_shorthand_and_cap(void) {
    lot52_render("<body style='margin:0'><div style='width:200px'><div id=a style='border-left:min(50%, 20px) solid'><div id=b style='height:2px'></div></div></div></body>", 480);
    /* 50% of 200 is 100, min with 20px is 20, the paint cap is 8 */
    TEST_ASSERT_EQUAL(8, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(192, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l84_border_math_shares_one_pool_slot(void) {
    lot52_render("<body style='margin:0'><div style='width:200px'><div id=a style='border-left-width:min(2%, 6px);border-style:solid'><div id=b style='height:1px'></div></div><div id=c style='border-right-width:min(2%, 6px);border-style:solid;height:1px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(1, g_doc.ntrack_pool);
    TEST_ASSERT_EQUAL(4, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l84_border_negative_keeps_the_previous_width(void) {
    lot52_render("<body style='margin:0'><div style='width:100px'><div id=a style='border-left-width:4px;border-left-width:-3px;border-style:solid'><div id=b style='height:1px'></div></div></div></body>", 480);
    TEST_ASSERT_EQUAL(4, lot52_node("b")->x);
    TEST_ASSERT_TRUE(lot52_has_diag("border edge refused"));
}

static void test_l84_border_fixed_math_uses_no_pool_slot(void) {
    lot52_render("<body style='margin:0'><div style='width:100px'><div id=a style='border-left-width:min(3px, 9px);border-style:solid'><div id=b style='height:1px'></div></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, g_doc.ntrack_pool);
    TEST_ASSERT_EQUAL(3, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l84_ordered_clears_definite_first(void) {
    lot52_render("<style>#g{masonry-auto-flow:definite-first next}#g{masonry-auto-flow:ordered pack}</style><body style='margin:0'><div id=g style='display:grid;grid-template-rows:masonry;grid-template-columns:30px 30px'></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("g")->masonry_next);
    TEST_ASSERT_EQUAL(0, lot52_node("g")->masonry_definite);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l84_column_flow_does_not_flatten_masonry_rows(void) {
    /* four 10px items, two declared columns: packed height 20, not 10 */
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:60px;grid-auto-flow:column;grid-template-columns:30px 30px;grid-template-rows:masonry'><div id=c0 style='height:10px'></div><div id=c1 style='height:10px'></div><div id=c2 style='height:10px'></div><div id=c3 style='height:10px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("c0")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("c0")->y);
    TEST_ASSERT_EQUAL(30, lot52_node("c1")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("c1")->y);
    TEST_ASSERT_EQUAL(0, lot52_node("c2")->x);
    TEST_ASSERT_EQUAL(10, lot52_node("c2")->y);
    TEST_ASSERT_EQUAL(30, lot52_node("c3")->x);
    TEST_ASSERT_EQUAL(10, lot52_node("c3")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("g")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l84_height_keyword_on_a_block_is_content_height(void) {
    lot52_render("<body style='margin:0'><div id=a style='height:min-content;width:40px'>ab</div></body>", 480);
    TEST_ASSERT_EQUAL(1, lot52_node("a")->height_intrinsic);
    TEST_ASSERT_TRUE(lot52_node("a")->h > 0);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l84_document_still_fits(void) {
    TEST_ASSERT_TRUE(sizeof(we_doc_t) <= WE_DOCUMENT_MAX);
    TEST_ASSERT_TRUE(sizeof(we_doc_t) < 1024u * 1024u);
}

static void lot84_extra_run(void) {
    RUN_TEST(test_l84_border_shorthand_and_cap);
    RUN_TEST(test_l84_border_math_shares_one_pool_slot);
    RUN_TEST(test_l84_border_negative_keeps_the_previous_width);
    RUN_TEST(test_l84_border_fixed_math_uses_no_pool_slot);
    RUN_TEST(test_l84_ordered_clears_definite_first);
    RUN_TEST(test_l84_column_flow_does_not_flatten_masonry_rows);
    RUN_TEST(test_l84_height_keyword_on_a_block_is_content_height);
    RUN_TEST(test_l84_document_still_fits);
}
