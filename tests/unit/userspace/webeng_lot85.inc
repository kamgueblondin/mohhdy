/* Lot 85 hand-written layout checks. Auto columns of a masonry-rows
 * grid are sized from the column masonry placed the item in. */
static void test_l85_auto_column_follows_masonry(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:300px;column-gap:0;row-gap:0;grid-template-columns:auto auto;grid-template-rows:masonry'><div id=a style='width:10px;height:80px'></div><div id=b style='width:10px;height:10px'></div><div id=c style='width:70px;height:10px'></div></div></body>", 480);
    /* a is the tall first item, so b and the wide c pack into column 1.
     * Ordinary placement would have put c back in column 0 and made that
     * column 70px. */
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(10, lot52_node("c")->x);
    TEST_ASSERT_EQUAL(10, lot52_node("c")->y);
    TEST_ASSERT_EQUAL(70, lot52_node("c")->w);
    TEST_ASSERT_EQUAL(80, lot52_node("g")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l85_auto_column_keeps_the_gap(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:300px;column-gap:8px;row-gap:4px;grid-template-columns:auto auto;grid-template-rows:masonry'><div id=a style='width:10px;height:40px'></div><div id=b style='width:20px;height:10px'></div><div id=c style='width:50px;height:10px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(18, lot52_node("b")->x);   /* 10 + gap 8 */
    TEST_ASSERT_EQUAL(18, lot52_node("c")->x);
    TEST_ASSERT_EQUAL(14, lot52_node("c")->y);   /* b is 10px, then the 4px row-gap */
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l85_explicit_start_sizes_that_column(void) {
    /* c asks for column 1. It must widen column 1, not column 0. */
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:300px;column-gap:0;grid-template-columns:auto auto;grid-template-rows:masonry'><div id=a style='width:12px;height:20px'></div><div id=c style='width:40px;height:8px;grid-column:1'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("c")->x);
    TEST_ASSERT_EQUAL(12, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(40, lot52_node("c")->w);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l85_fixed_columns_are_unchanged(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:200px;grid-template-columns:40px 60px;grid-template-rows:masonry'><div id=a style='height:30px'></div><div id=b style='height:10px'></div><div id=c style='height:10px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(40, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->y);
    /* b's column is shorter, so c packs under b, not under a. */
    TEST_ASSERT_EQUAL(40, lot52_node("c")->x);
    TEST_ASSERT_EQUAL(10, lot52_node("c")->y);
    TEST_ASSERT_EQUAL(30, lot52_node("g")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l85_percent_top_uses_the_packed_height(void) {
    /* 50% of the packed 80px is 40. The raw percentage token is 50, so a
     * fallback that kept the token would land at 50. */
    lot52_render("<body style='margin:0'><div id=g style='position:relative;display:grid;width:200px;grid-template-columns:100px 100px;grid-template-rows:masonry'><div id=c style='height:80px'></div><div id=a style='position:absolute;top:50%;width:8px;height:8px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(40, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(80, lot52_node("g")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l85_percent_bottom_uses_the_packed_height(void) {
    lot52_render("<body style='margin:0'><div id=g style='position:relative;display:grid;width:200px;grid-template-columns:100px 100px;grid-template-rows:masonry'><div id=c style='height:80px'></div><div id=a style='position:absolute;bottom:25%;width:8px;height:8px'></div></div></body>", 480);
    /* 25% of 80 is 20; the box sits 20px above the bottom: 80 - 20 - 8. */
    TEST_ASSERT_EQUAL(52, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l85_document_still_fits(void) {
    TEST_ASSERT_TRUE(sizeof(we_doc_t) <= WE_DOCUMENT_MAX);
}

static void lot85_run(void) {
    RUN_TEST(test_l85_auto_column_follows_masonry);
    RUN_TEST(test_l85_auto_column_keeps_the_gap);
    RUN_TEST(test_l85_explicit_start_sizes_that_column);
    RUN_TEST(test_l85_fixed_columns_are_unchanged);
    RUN_TEST(test_l85_percent_top_uses_the_packed_height);
    RUN_TEST(test_l85_percent_bottom_uses_the_packed_height);
    RUN_TEST(test_l85_document_still_fits);
}
