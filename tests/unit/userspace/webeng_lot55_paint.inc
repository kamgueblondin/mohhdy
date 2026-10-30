/* Clipping changes paint and link hits while retaining layout geometry. */
static void test_l55_clip_rectangle_right_and_bottom(void) {
    int painted;
    lot52_render("<body style='margin:0'><div id=p style='width:20px;height:10px;overflow:hidden'>"
        "<div id=a style='width:40px;height:20px;background:red'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(40, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(1, g_doc.nitems);
    TEST_ASSERT_EQUAL(20, g_doc.items[0].w);
    TEST_ASSERT_EQUAL(10, g_doc.items[0].h);
    TEST_ASSERT_EQUAL(0xff0000, lot52_pixel(19, 9, &painted));
    TEST_ASSERT_EQUAL(1, painted);
    lot52_pixel(20, 9, &painted); TEST_ASSERT_EQUAL(0, painted);
    lot52_pixel(19, 10, &painted); TEST_ASSERT_EQUAL(0, painted);
    TEST_ASSERT_EQUAL(0, g_doc.perf.items_dropped);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l55_clip_visible_default_retains_overflow(void) {
    int painted;
    lot52_render("<body style='margin:0'><div style='width:20px;height:10px;overflow:visible'>"
        "<div id=a style='width:40px;height:20px;background:red'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(40, g_doc.items[0].w);
    TEST_ASSERT_EQUAL(20, g_doc.items[0].h);
    TEST_ASSERT_EQUAL(0xff0000, lot52_pixel(39, 19, &painted));
    TEST_ASSERT_EQUAL(1, painted);
    TEST_ASSERT_EQUAL(40, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l55_clip_negative_margin_intersection(void) {
    int painted;
    lot52_render("<body style='margin:0'><div style='width:20px;height:10px;overflow:hidden'>"
        "<div id=a style='width:40px;height:20px;margin-left:-8px;margin-top:-4px;background:red'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(-8, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(-4, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(0, g_doc.items[0].x);
    TEST_ASSERT_EQUAL(0, g_doc.items[0].y);
    TEST_ASSERT_EQUAL(20, g_doc.items[0].w);
    TEST_ASSERT_EQUAL(10, g_doc.items[0].h);
    TEST_ASSERT_EQUAL(0xff0000, lot52_pixel(19, 9, &painted));
    TEST_ASSERT_EQUAL(1, painted);
    lot52_pixel(-1, 0, &painted); TEST_ASSERT_EQUAL(0, painted);
}

static void test_l55_clip_nested_ancestor_intersection(void) {
    int painted;
    lot52_render("<body style='margin:0'><div style='width:30px;height:20px;overflow:hidden'>"
        "<div style='width:20px;height:30px;margin-left:10px;overflow:hidden'>"
        "<div id=a style='width:40px;height:40px;background:red'></div></div></div></body>", 320);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(40, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, g_doc.items[0].x);
    TEST_ASSERT_EQUAL(20, g_doc.items[0].w);
    TEST_ASSERT_EQUAL(20, g_doc.items[0].h);
    TEST_ASSERT_EQUAL(0xff0000, lot52_pixel(29, 19, &painted));
    TEST_ASSERT_EQUAL(1, painted);
    lot52_pixel(30, 19, &painted); TEST_ASSERT_EQUAL(0, painted);
    lot52_pixel(10, 20, &painted); TEST_ASSERT_EQUAL(0, painted);
}

static void test_l55_clip_padding_box_preserves_parent_border(void) {
    uint32_t i;
    int borders = 0, painted;
    lot52_render("<body style='margin:0'><div style='width:20px;height:10px;border:2px solid blue;overflow:hidden'>"
        "<div id=a style='width:40px;height:20px;background:red'></div></div></body>", 320);
    for (i = 0; i < g_doc.nitems; i++) if (g_doc.items[i].kind == OS_FB_WEB_BORDER) {
        TEST_ASSERT_EQUAL(24, g_doc.items[i].w);
        TEST_ASSERT_EQUAL(14, g_doc.items[i].h);
        TEST_ASSERT_EQUAL(0x0000ff, g_doc.items[i].color);
        borders++;
    }
    TEST_ASSERT_EQUAL(1, borders);
    TEST_ASSERT_EQUAL(2, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(2, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(0xff0000, lot52_pixel(21, 11, &painted));
    TEST_ASSERT_EQUAL(1, painted);
    lot52_pixel(22, 11, &painted); TEST_ASSERT_EQUAL(0, painted);
}

static void test_l55_clip_zero_height_hides_descendants(void) {
    lot52_render("<body style='margin:0'><div style='width:20px;height:0;overflow:hidden'>"
        "<div id=a style='width:40px;height:20px;background:red'>A</div></div></body>", 320);
    TEST_ASSERT_EQUAL(40, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(0, g_doc.nitems);
    TEST_ASSERT_EQUAL(0, g_doc.perf.items_dropped);
    TEST_ASSERT_NULL(find_text("A"));
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l55_clip_fully_outside_child_is_omitted(void) {
    lot52_render("<body style='margin:0'><div style='width:20px;height:10px;overflow:hidden'>"
        "<div id=a style='width:10px;height:10px;margin-left:30px;background:red'>A</div></div></body>", 320);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(0, g_doc.nitems);
    TEST_ASSERT_NULL(find_text("A"));
    TEST_ASSERT_EQUAL(0, g_doc.perf.items_dropped);
}

static void test_l55_clip_text_keeps_only_whole_glyphs(void) {
    lot52_render("<body style='margin:0'><div style='width:20px;height:10px;overflow:hidden;white-space:nowrap'>"
        "<span>abcdef</span></div></body>", 320);
    TEST_ASSERT_NOT_NULL(find_text("ab"));
    TEST_ASSERT_NULL(find_text("abcdef"));
    TEST_ASSERT_EQUAL(0, find_text("ab")->x);
    TEST_ASSERT_EQUAL(1, find_text("ab")->y);
    TEST_ASSERT_EQUAL(16, find_text("ab")->w);
    TEST_ASSERT_EQUAL(8, find_text("ab")->h);
    TEST_ASSERT_EQUAL(0, g_doc.perf.items_dropped);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l55_clip_text_left_partial_glyph_is_omitted(void) {
    lot52_render("<body style='margin:0'><div style='width:24px;height:10px;overflow:hidden'>"
        "<div style='margin-left:-4px;width:48px;white-space:nowrap'>abcdef</div></div></body>", 320);
    TEST_ASSERT_NOT_NULL(find_text("bc"));
    TEST_ASSERT_EQUAL(4, find_text("bc")->x);
    TEST_ASSERT_EQUAL(16, find_text("bc")->w);
    TEST_ASSERT_EQUAL(1, find_text("bc")->y);
    TEST_ASSERT_NULL(find_text("abcdef"));
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

/* Lot 61: partial glyphs are painted from the kernel font. With 8px the
 * empty eighth glyph row is the only one cut, so lot 61 keeps the text
 * (test_l61_clip_empty_row_cut_keeps_text); 5px really cuts lit rows: the
 * text item is replaced by rectangles of the visible pixels only. */
static void test_l55_clip_text_vertical_partial_is_omitted(void) {
    uint32_t i;
    lot52_render("<body style='margin:0'><div style='width:40px;height:5px;overflow:hidden'>one</div></body>", 320);
    TEST_ASSERT_EQUAL(9, g_doc.nitems);
    for (i = 0; i < g_doc.nitems; i++) {
        TEST_ASSERT_EQUAL(OS_FB_WEB_RECT, g_doc.items[i].kind);
        TEST_ASSERT_TRUE(g_doc.items[i].y + g_doc.items[i].h <= 5);
    }
    TEST_ASSERT_NULL(find_text("one"));
    TEST_ASSERT_EQUAL(1, g_doc.perf.lines);
    TEST_ASSERT_EQUAL(0, g_doc.perf.items_dropped);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l55_clip_text_exact_vertical_fit(void) {
    lot52_render("<body style='margin:0'><div style='width:40px;height:9px;overflow:hidden'>one</div></body>", 320);
    TEST_ASSERT_NOT_NULL(find_text("one"));
    TEST_ASSERT_EQUAL(1, find_text("one")->y);
    TEST_ASSERT_EQUAL(8, find_text("one")->h);
    TEST_ASSERT_EQUAL(24, find_text("one")->w);
    TEST_ASSERT_EQUAL(1, g_doc.nitems);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l55_clip_multiple_lines_keeps_visible_line(void) {
    lot52_render("<body style='margin:0'><div style='width:24px;height:10px;overflow:hidden'>one two three</div></body>", 320);
    TEST_ASSERT_NOT_NULL(find_text("one"));
    TEST_ASSERT_NULL(find_text("two"));
    TEST_ASSERT_NULL(find_text("three"));
    TEST_ASSERT_EQUAL(3, g_doc.perf.lines);
    TEST_ASSERT_EQUAL(1, g_doc.nitems);
    TEST_ASSERT_EQUAL(0, g_doc.perf.items_dropped);
}

static void test_l55_clip_child_border_does_not_draw_false_edges(void) {
    int painted;
    lot52_render("<body style='margin:0'><div style='width:20px;height:10px;overflow:hidden'>"
        "<div id=a style='width:30px;height:20px;border:2px solid red'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(34, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(24, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(2, g_doc.nitems);
    TEST_ASSERT_EQUAL(OS_FB_WEB_RECT, g_doc.items[0].kind);
    TEST_ASSERT_EQUAL(OS_FB_WEB_RECT, g_doc.items[1].kind);
    TEST_ASSERT_EQUAL(0xff0000, lot52_pixel(19, 1, &painted));
    TEST_ASSERT_EQUAL(1, painted);
    TEST_ASSERT_EQUAL(0xff0000, lot52_pixel(1, 9, &painted));
    TEST_ASSERT_EQUAL(1, painted);
    lot52_pixel(19, 9, &painted); TEST_ASSERT_EQUAL(0, painted);
    lot52_pixel(10, 5, &painted); TEST_ASSERT_EQUAL(0, painted);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l55_clip_border_right_edge_after_negative_margin(void) {
    int painted;
    lot52_render("<body style='margin:0'><div style='width:20px;height:10px;overflow:hidden'>"
        "<div id=a style='width:20px;height:20px;border:2px solid red;margin-left:-10px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(-10, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(24, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(2, g_doc.nitems);
    TEST_ASSERT_EQUAL(0xff0000, lot52_pixel(13, 9, &painted));
    TEST_ASSERT_EQUAL(1, painted);
    TEST_ASSERT_EQUAL(0xff0000, lot52_pixel(0, 1, &painted));
    TEST_ASSERT_EQUAL(1, painted);
    lot52_pixel(0, 5, &painted); TEST_ASSERT_EQUAL(0, painted);
    lot52_pixel(14, 5, &painted); TEST_ASSERT_EQUAL(0, painted);
}

static void test_l55_clip_budget_is_bounded_and_diagnosed(void) {
    char html[4096];
    unsigned i;
    strcpy(html, "<style>.x{width:30px;height:20px;border:2px solid red;margin-bottom:-24px}</style>"
        "<body style='margin:0'><div style='width:20px;height:10px;overflow:hidden'>");
    for (i = 0; i < 90; i++) strcat(html, "<div class=x></div>");
    strcat(html, "</div></body>");
    lot52_render(html, 320);
    TEST_ASSERT_EQUAL(OS_FB_WEB_ITEMS, g_doc.nitems);
    TEST_ASSERT_TRUE(g_doc.perf.items_dropped > 0);
    TEST_ASSERT_TRUE(lot52_has_diag("clipping display budget"));
    TEST_ASSERT_TRUE(g_doc.ndiag <= WE_MAX_DIAG);
    for (i = 0; i < g_doc.nitems; i++) {
        TEST_ASSERT_TRUE(g_doc.items[i].x >= 0);
        TEST_ASSERT_TRUE(g_doc.items[i].y >= 0);
        TEST_ASSERT_TRUE(g_doc.items[i].x + g_doc.items[i].w <= 20);
        TEST_ASSERT_TRUE(g_doc.items[i].y + g_doc.items[i].h <= 10);
    }
}

static void test_l55_visibility_hidden_keeps_geometry(void) {
    lot52_render("<body style='margin:0'><div id=a style='height:20px;visibility:hidden;background:red'>hidden</div>"
        "<div id=b style='height:10px'>shown</div></body>", 320);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(20, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->h);
    TEST_ASSERT_NULL(find_text("hidden"));
    TEST_ASSERT_NOT_NULL(find_text("shown"));
    TEST_ASSERT_EQUAL(21, find_text("shown")->y);
    TEST_ASSERT_EQUAL(1, g_doc.nitems);
    TEST_ASSERT_EQUAL(0, g_doc.perf.items_dropped);
}

static void test_l55_visibility_inherits_to_children(void) {
    lot52_render("<body style='margin:0'><div id=p style='visibility:hidden'>"
        "<div id=a style='height:20px;background:red'>hidden</div></div></body>", 320);
    TEST_ASSERT_EQUAL(1, lot52_node("a")->visibility);
    TEST_ASSERT_EQUAL(20, lot52_node("p")->h);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->h);
    TEST_ASSERT_NULL(find_text("hidden"));
    TEST_ASSERT_EQUAL(0, g_doc.nitems);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l55_visibility_visible_child_overrides_hidden_parent(void) {
    lot52_render("<body style='margin:0'><div style='visibility:hidden;background:blue'>"
        "<div id=a style='visibility:visible;height:20px;background:red'>shown</div></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->visibility);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->h);
    TEST_ASSERT_NOT_NULL(find_text("shown"));
    TEST_ASSERT_EQUAL(2, g_doc.nitems);
    TEST_ASSERT_EQUAL(0xff0000, g_doc.items[0].color);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l55_visibility_invalid_preserves_hidden(void) {
    lot52_render("<body style='margin:0'><div id=a style='visibility:hidden;visibility:collapse;height:20px;background:red'>hidden</div></body>", 320);
    TEST_ASSERT_EQUAL(1, lot52_node("a")->visibility);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(0, g_doc.nitems);
    TEST_ASSERT_NULL(find_text("hidden"));
    TEST_ASSERT_TRUE(lot52_has_diag("visibility refused"));
}

static void test_l55_clip_invalid_overflow_preserves_hidden(void) {
    lot52_render("<body style='margin:0'><div id=p style='width:20px;height:10px;overflow:hidden;overflow:scroll'>"
        "<div style='width:40px;height:20px;background:red'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(1, lot52_node("p")->overflow_hidden);
    TEST_ASSERT_EQUAL(20, g_doc.items[0].w);
    TEST_ASSERT_EQUAL(10, g_doc.items[0].h);
    TEST_ASSERT_TRUE(lot52_has_diag("overflow refused"));
}

static void test_l55_clip_link_outside_hidden_area_is_not_hit(void) {
    char href[32];
    lot52_render("<body style='margin:0'><div style='width:24px;height:10px;overflow:hidden;white-space:nowrap'>"
        "<a href='/next'>abcdef</a></div></body>", 320);
    TEST_ASSERT_NOT_NULL(find_text("abc"));
    TEST_ASSERT_EQUAL(5, we_link_at(&g_doc, 8, 4, href, sizeof(href)));
    TEST_ASSERT_EQUAL(0, strcmp(href, "/next"));
    TEST_ASSERT_EQUAL(5, we_link_at(&g_doc, 23, 4, href, sizeof(href)));
    TEST_ASSERT_EQUAL(0, we_link_at(&g_doc, 24, 4, href, sizeof(href)));
    TEST_ASSERT_EQUAL(0, we_link_at(&g_doc, 40, 4, href, sizeof(href)));
    TEST_ASSERT_EQUAL(0, href[0]);
}

static void test_l55_clip_partial_glyph_is_not_a_link_hit(void) {
    char href[32];
    lot52_render("<body style='margin:0'><div style='width:20px;height:10px;overflow:hidden;white-space:nowrap'>"
        "<a href='/next'>abcdef</a></div></body>", 320);
    TEST_ASSERT_NOT_NULL(find_text("ab"));
    TEST_ASSERT_EQUAL(5, we_link_at(&g_doc, 15, 4, href, sizeof(href)));
    TEST_ASSERT_EQUAL(0, strcmp(href, "/next"));
    TEST_ASSERT_EQUAL(0, we_link_at(&g_doc, 16, 4, href, sizeof(href)));
    TEST_ASSERT_EQUAL(0, we_link_at(&g_doc, 19, 4, href, sizeof(href)));
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l55_clip_link_below_visible_line_is_not_hit(void) {
    char href[32];
    lot52_render("<body style='margin:0'><div style='width:24px;height:10px;overflow:hidden'>"
        "<a href='/next'>one two</a></div></body>", 320);
    TEST_ASSERT_NOT_NULL(find_text("one"));
    TEST_ASSERT_NULL(find_text("two"));
    TEST_ASSERT_EQUAL(5, we_link_at(&g_doc, 8, 4, href, sizeof(href)));
    TEST_ASSERT_EQUAL(0, we_link_at(&g_doc, 8, 14, href, sizeof(href)));
    TEST_ASSERT_EQUAL(0, href[0]);
}

static void test_l55_visibility_hidden_link_is_not_hit(void) {
    char href[32];
    lot52_render("<body style='margin:0'><div><a href='/next' style='visibility:hidden'>hidden</a></div></body>", 320);
    TEST_ASSERT_NULL(find_text("hidden"));
    TEST_ASSERT_EQUAL(0, we_link_at(&g_doc, 8, 4, href, sizeof(href)));
    TEST_ASSERT_EQUAL(0, href[0]);
    TEST_ASSERT_EQUAL(0, g_doc.nitems);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l55_visibility_visible_link_child_remains_hit(void) {
    char href[32];
    lot52_render("<body style='margin:0'><div style='visibility:hidden'>"
        "<a href='/next' style='visibility:visible'>shown</a></div></body>", 320);
    TEST_ASSERT_NOT_NULL(find_text("shown"));
    TEST_ASSERT_EQUAL(5, we_link_at(&g_doc, 8, 4, href, sizeof(href)));
    TEST_ASSERT_EQUAL(0, strcmp(href, "/next"));
    TEST_ASSERT_EQUAL(1, g_doc.nitems);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l55_clip_background_order_is_preserved(void) {
    int painted;
    lot52_render("<body style='margin:0'><div style='width:20px;height:10px;overflow:hidden;background:blue'>"
        "<div style='width:40px;height:5px;background:red'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(2, g_doc.nitems);
    TEST_ASSERT_EQUAL(0x0000ff, g_doc.items[0].color);
    TEST_ASSERT_EQUAL(0xff0000, g_doc.items[1].color);
    TEST_ASSERT_EQUAL(0xff0000, lot52_pixel(19, 4, &painted));
    TEST_ASSERT_EQUAL(1, painted);
    TEST_ASSERT_EQUAL(0x0000ff, lot52_pixel(19, 5, &painted));
    TEST_ASSERT_EQUAL(1, painted);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l55_clip_relative_descendants_use_moved_geometry(void) {
    int painted;
    lot52_render("<body style='margin:0'><div style='width:40px;height:30px;overflow:hidden'>"
        "<div id=a style='position:relative;left:10px;top:10px;width:20px;height:10px;background:red'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(10, g_doc.items[0].x);
    TEST_ASSERT_EQUAL(10, g_doc.items[0].y);
    TEST_ASSERT_EQUAL(0xff0000, lot52_pixel(29, 19, &painted));
    TEST_ASSERT_EQUAL(1, painted);
    lot52_pixel(9, 10, &painted); TEST_ASSERT_EQUAL(0, painted);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l55_clip_image_uses_same_decoder_and_bounds(void) {
    int painted;
    lot52_image("<body style='margin:0'><div style='width:20px;height:16px;overflow:hidden'>"
        "<img id=a src='/images/mire-g1.png' alt='fallback'></div></body>");
    TEST_ASSERT_EQUAL(1, g_doc.perf.images);
    TEST_ASSERT_EQUAL(0, g_doc.perf.image_refused);
    TEST_ASSERT_EQUAL(48, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(32, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(0xffffff, lot52_pixel(19, 15, &painted));
    TEST_ASSERT_EQUAL(1, painted);
    lot52_pixel(20, 15, &painted); TEST_ASSERT_EQUAL(0, painted);
    lot52_pixel(19, 16, &painted); TEST_ASSERT_EQUAL(0, painted);
    TEST_ASSERT_EQUAL(1, lot52_read_calls);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l55_clip_relayout_rebuilds_items_and_owners(void) {
    char href[32];
    lot52_render("<body style='margin:0'><div style='width:50%;height:10px;overflow:hidden;white-space:nowrap'>"
        "<a href='/next'>abcdefgh</a></div></body>", 64);
    TEST_ASSERT_NOT_NULL(find_text("abcd"));
    TEST_ASSERT_EQUAL(0, we_link_at(&g_doc, 40, 4, href, sizeof(href)));
    TEST_ASSERT_EQUAL(0, we_layout(&g_doc, 128));
    TEST_ASSERT_NOT_NULL(find_text("abcdefgh"));
    TEST_ASSERT_EQUAL(5, we_link_at(&g_doc, 40, 4, href, sizeof(href)));
    TEST_ASSERT_EQUAL(0, strcmp(href, "/next"));
    TEST_ASSERT_EQUAL(1, g_doc.nitems);
    TEST_ASSERT_EQUAL(0, g_doc.perf.items_dropped);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}
