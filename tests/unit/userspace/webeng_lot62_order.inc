/* Lot 62: CSS 2.1 appendix E inside the in-flow level: block backgrounds,
 * then floats, then inline content, then positioned boxes (z auto). */
static void test_l62_order_float_over_later_block(void) {
    lot52_render(L62_BODY "<div style='float:left;width:20px;height:20px;background:#ff0000'>"
                 "<p style='margin:0;background:#00ff00;width:10px'>F</p></div>"
                 "<div style='height:30px;background:#0000ff'>T</div>"
                 "<div style='margin-top:-30px;height:4px;background:#00ffff'></div></body>", 320);
    /* the later block goes under the whole float, the float keeps its order */
    TEST_ASSERT_TRUE(l61_index(0x00FFFF) < l61_index(0xFF0000));
    TEST_ASSERT_EQUAL(l61_index(0xFF0000) + 1, l61_index(0x00FF00));
    TEST_ASSERT_EQUAL(l61_index(0x00FF00) + 1, (int)(find_text("F") - g_doc.items));
    TEST_ASSERT_EQUAL(0xFF0000, (int)l61_paint(12, 2));  /* the float over the later block */
    TEST_ASSERT_EQUAL(0x00FFFF, (int)l61_paint(30, 2));
}

static void test_l62_order_text_over_a_later_float(void) {
    lot52_render(L62_BODY "<p style='margin:0'>AB</p>"
                 "<div style='float:left;margin-top:-10px;width:20px;height:20px;background:#ff0000'></div></body>", 320);
    TEST_ASSERT_EQUAL(0, l61_index(0xFF0000));
    TEST_ASSERT_EQUAL(1, (int)(find_text("AB") - g_doc.items));
}

static void test_l62_order_inline_content_over_later_blocks(void) {
    lot52_render(L62_BODY "<p style='margin:0'>AB</p>"
                 "<span style='display:inline-block;width:20px;height:20px;background:#ff0000'>C</span>"
                 "<div style='margin-top:-30px;height:40px;background:#0000ff'></div></body>", 320);
    TEST_ASSERT_EQUAL(0, l61_index(0x0000FF));
    TEST_ASSERT_EQUAL(1, (int)(find_text("AB") - g_doc.items));
    TEST_ASSERT_EQUAL(2, l61_index(0xFF0000));           /* the inline-block stays whole */
    TEST_ASSERT_EQUAL(3, (int)(find_text("C") - g_doc.items));
}

static void test_l62_order_keeps_emission_without_conflict(void) {
    lot52_render(L62_BODY "<div style='background:#ff0000'>A</div><div style='background:#0000ff'>B</div>"
                 "<div style='float:left;width:8px;height:4px;background:#00ff00'></div></body>", 320);
    TEST_ASSERT_EQUAL(0, l61_index(0xFF0000));
    TEST_ASSERT_EQUAL(1, (int)(find_text("A") - g_doc.items));
    TEST_ASSERT_EQUAL(2, l61_index(0x0000FF));
    TEST_ASSERT_EQUAL(3, (int)(find_text("B") - g_doc.items));
    TEST_ASSERT_EQUAL(4, l61_index(0x00FF00));
}

static void test_l62_order_inside_a_context_and_flex(void) {
    lot52_render(L62_BODY "<div style='position:relative;z-index:1'>"
                 "<div style='float:left;width:20px;height:20px;background:#ff0000'></div>"
                 "<div style='height:5px;background:#00ff00'></div></div></body>", 320);
    TEST_ASSERT_TRUE(l61_index(0x00FF00) < l61_index(0xFF0000));
    TEST_ASSERT_EQUAL(0xFF0000, (int)l61_paint(5, 2));
    /* a float declared on a flex item is not a float */
    lot52_render(L62_BODY "<div style='display:flex'><div style='float:left;width:20px;height:20px;background:#ff0000'></div></div>"
                 "<div style='margin-top:-10px;height:20px;background:#0000ff'></div></body>", 320);
    TEST_ASSERT_EQUAL(0x0000FF, (int)l61_paint(5, 15));
}

static void lot62_order_run(void) {
    RUN_TEST(test_l62_order_float_over_later_block);
    RUN_TEST(test_l62_order_text_over_a_later_float);
    RUN_TEST(test_l62_order_inline_content_over_later_blocks);
    RUN_TEST(test_l62_order_keeps_emission_without_conflict);
    RUN_TEST(test_l62_order_inside_a_context_and_flex);
}
