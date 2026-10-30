/* Lot 61: explicit viewport, position:fixed, static positions, scrolling. */
#define L61_FIXED "<body style='margin:0'><div id=o style='overflow:hidden;height:20px;width:100px'>" \
    "<div id=f style='position:fixed;right:10px;bottom:5px;width:30px;height:12px;background:#ff0000'>F</div></div>" \
    "<div id=g style='position:fixed;top:0;left:0;width:50%;height:10%;background:#00ff00'></div>" \
    "<div id=page style='height:900px;background:#0000ff'></div></body>"

static void l61_viewport(const char *html, int w, int h) {
    TEST_ASSERT_EQUAL(0, we_parse(&g_doc, html, (int)strlen(html)));
    TEST_ASSERT_EQUAL(0, we_style(&g_doc));
    TEST_ASSERT_EQUAL(0, we_layout_viewport(&g_doc, w, h));
}

static void test_l61_fixed_uses_the_default_viewport(void) {
    lot52_render(L61_FIXED, 320);
    TEST_ASSERT_EQUAL(1, lot52_node("f")->fixed);
    TEST_ASSERT_EQUAL(WE_VIEWPORT_H_DEFAULT, g_doc.viewport_h);
    l61_box("f", 280, 463, 30, 12);
    l61_box("g", 0, 0, 160, 48);
    l61_rect_is(0xFF0000, 280, 463, 30, 12);
    l61_text_at("F", 280, 464);
    TEST_ASSERT_EQUAL(920, (int)g_doc.perf.page_h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l61_fixed_follows_an_explicit_viewport(void) {
    l61_viewport(L61_FIXED, 400, 200);
    TEST_ASSERT_EQUAL(400, g_doc.viewport_w);
    TEST_ASSERT_EQUAL(200, g_doc.viewport_h);
    l61_box("f", 360, 183, 30, 12);
    l61_box("g", 0, 0, 200, 20);
    l61_rect_is(0x00FF00, 0, 0, 200, 20);
}

static void test_l61_fixed_escapes_overflow_clip_and_paints_last(void) {
    lot52_render(L61_FIXED, 320);
    TEST_ASSERT_TRUE(l61_index(0xFF0000) > l61_index(0x0000FF));
    TEST_ASSERT_TRUE(l61_index(0x00FF00) > l61_index(0x0000FF));
    TEST_ASSERT_EQUAL(30, l61_rect(0xFF0000)->w);
    TEST_ASSERT_EQUAL(12, l61_rect(0xFF0000)->h);
}

static void test_l61_scroll_moves_only_fixed_boxes(void) {
    l61_viewport(L61_FIXED, 320, 480);
    TEST_ASSERT_EQUAL(100, we_scroll(&g_doc, 100));
    TEST_ASSERT_EQUAL(100, g_doc.scroll_y);
    l61_rect_is(0xFF0000, 280, 563, 30, 12);
    l61_text_at("F", 280, 564);
    l61_rect_is(0x00FF00, 0, 100, 160, 48);
    l61_rect_is(0x0000FF, 0, 20, 320, 900);
    l61_box("f", 280, 563, 30, 12);
    l61_box("page", 0, 20, 320, 900);
    TEST_ASSERT_EQUAL(30, we_scroll(&g_doc, 30));
    l61_rect_is(0xFF0000, 280, 493, 30, 12);
}

static void test_l61_scroll_is_bounded(void) {
    l61_viewport(L61_FIXED, 320, 480);
    TEST_ASSERT_EQUAL(440, we_scroll(&g_doc, 5000));
    l61_rect_is(0x00FF00, 0, 440, 160, 48);
    TEST_ASSERT_EQUAL(0, we_scroll(&g_doc, -40));
    l61_rect_is(0x00FF00, 0, 0, 160, 48);
    l61_viewport("<body style='margin:0'><div style='height:50px'></div></body>", 320, 480);
    TEST_ASSERT_EQUAL(0, we_scroll(&g_doc, 20));
}

static void test_l61_relayout_resets_the_scroll(void) {
    l61_viewport(L61_FIXED, 320, 480);
    (void)we_scroll(&g_doc, 200);
    TEST_ASSERT_EQUAL(0, we_layout_viewport(&g_doc, 320, 480));
    TEST_ASSERT_EQUAL(0, g_doc.scroll_y);
    l61_rect_is(0x00FF00, 0, 0, 160, 48);
    l61_box("f", 280, 463, 30, 12);
}

static void test_l61_viewport_refuses_bad_sizes(void) {
    lot52_render(L61_FIXED, 320);
    TEST_ASSERT_EQUAL(-1, we_layout_viewport(&g_doc, 320, 0));
    TEST_ASSERT_EQUAL(-1, we_layout_viewport(&g_doc, 32, 200));
    TEST_ASSERT_EQUAL(0, we_scroll(0, 10));
}

static void test_l61_static_position_after_a_line(void) {
    lot52_render("<body style='margin:0'><div id=p style='position:relative;width:200px;padding:5px'>ab cd"
        "<div id=a style='position:absolute;width:10px;height:10px;background:#ff0000'></div><div id=b>B</div></div></body>", 320);
    l61_box("a", 5, 15, 10, 10);
    l61_box("b", 5, 15, 200, 10);
    l61_rect_is(0xFF0000, 5, 15, 10, 10);
    TEST_ASSERT_EQUAL(30, lot52_node("p")->h);
}

static void test_l61_static_position_between_blocks(void) {
    lot52_render("<body style='margin:0'><div id=p style='position:relative;width:200px'><div style='height:12px'></div>"
        "<div id=a style='position:absolute;left:7px;width:10px;height:10px'></div><div id=b style='height:8px'></div></div></body>", 320);
    l61_box("a", 7, 12, 10, 10);
    l61_box("b", 0, 12, 200, 8);
}

static void test_l61_static_position_inline_absolute_leaves_the_line(void) {
    lot52_render("<body style='margin:0'><div id=p style='position:relative;width:200px;padding:5px'><div>ab</div>"
        "<span>xx<span id=c style='position:absolute;left:3px'>C</span>yy</span></div></body>", 320);
    l61_box("c", 3, 15, 8, 10);
    l61_text_at("C", 3, 16);
    l61_text_at("xx", 5, 16);
    TEST_ASSERT_EQUAL(1, l61_count(OS_FB_WEB_TEXT) - 3);
}

static void test_l61_static_position_in_flex_is_content_origin(void) {
    lot52_render("<body style='margin:0'><div id=p style='position:relative;display:flex;width:200px;padding:6px 4px'>"
        "<div style='width:50px;height:10px'></div><div id=a style='position:absolute;width:5px;height:5px'></div></div></body>", 320);
    l61_box("a", 4, 6, 5, 5);
}

static void test_l61_static_position_keeps_insets_when_set(void) {
    lot52_render("<body style='margin:0'><div id=p style='position:relative;width:200px;height:40px;padding:5px'>ab cd"
        "<div id=a style='position:absolute;right:0;bottom:0;width:10px;height:10px'></div></div></body>", 320);
    l61_box("a", 200, 40, 10, 10);
}

static void test_l61_static_position_follows_a_baseline_shift(void) {
    lot52_render("<body style='margin:0'><div id=p style='position:relative;width:200px'>ab"
        "<div id=b style='display:inline-block;width:30px;vertical-align:baseline;padding-top:6px'>X<br>Y"
        "<div id=a style='position:absolute;width:4px;height:4px'></div></div></div></body>", 320);
    l61_box("b", 16, 0, 30, 26);
    l61_box("a", 16, 26, 4, 4);
}

static void lot61_viewport_run(void) {
    RUN_TEST(test_l61_fixed_uses_the_default_viewport);
    RUN_TEST(test_l61_fixed_follows_an_explicit_viewport);
    RUN_TEST(test_l61_fixed_escapes_overflow_clip_and_paints_last);
    RUN_TEST(test_l61_scroll_moves_only_fixed_boxes);
    RUN_TEST(test_l61_scroll_is_bounded);
    RUN_TEST(test_l61_relayout_resets_the_scroll);
    RUN_TEST(test_l61_viewport_refuses_bad_sizes);
    RUN_TEST(test_l61_static_position_after_a_line);
    RUN_TEST(test_l61_static_position_between_blocks);
    RUN_TEST(test_l61_static_position_inline_absolute_leaves_the_line);
    RUN_TEST(test_l61_static_position_in_flex_is_content_origin);
    RUN_TEST(test_l61_static_position_keeps_insets_when_set);
    RUN_TEST(test_l61_static_position_follows_a_baseline_shift);
}
