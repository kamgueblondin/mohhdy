/* Lot 63 L: background-image url() of an initrd image, with
 * background-repeat, background-position and background-size. The test
 * image mire-ga16.png is 48x32; sampled at 6px wide its columns are the
 * grey ramp below (rows are uniform). */
static const unsigned l63_ramp[6] = {0x000000, 0x282828, 0x505050, 0x787878, 0xA0A0A0, 0xC8C8C8};

#define L63_BGI(box, extra) "<body style='margin:0'><div id=b style='" box ";background-image:url(/images/mire-ga16.png);" extra "'></div></body>"

static void l63_bgi(const char *html) {
    lot52_read_calls = 0;
    lot52_read_mode = 0;
    we_set_image_reader(lot52_reader, 0);
    TEST_ASSERT_EQUAL(0, we_render(&g_doc, html, (int)strlen(html), 320));
}

/* The colour at (x, y), or -1 when no rectangle covers it. */
static long l63_at(int x, int y) {
    int painted;
    unsigned color = lot52_pixel(x, y, &painted);
    return painted ? (long)color : -1;
}

static void test_l63_bgi_tiles_repeat_both_axes(void) {
    int i;
    l63_bgi(L63_BGI("width:14px;height:9px", "background-size:6px 4px"));
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
    TEST_ASSERT_EQUAL(1, lot52_read_calls);
    TEST_ASSERT_EQUAL_STRING("/images/mire-ga16.png", lot52_last_path);
    for (i = 0; i < 14; i++) TEST_ASSERT_EQUAL((long)l63_ramp[i % 6], l63_at(i, 0));
    TEST_ASSERT_EQUAL((long)l63_ramp[1], l63_at(13, 8));
    TEST_ASSERT_EQUAL(-1, l63_at(14, 0));
    TEST_ASSERT_EQUAL(-1, l63_at(0, 9));
    TEST_ASSERT_EQUAL(14, (int)g_doc.nitems);               /* one run per column, rows merged */
    for (i = 0; i < (int)g_doc.nitems; i++) {
        TEST_ASSERT_EQUAL(9, g_doc.items[i].h);
        TEST_ASSERT_EQUAL(lot52_node("b") - g_doc.nodes, g_doc.item_owner[i]);
    }
    /* natural size: mire-g1.png is 48x32 one-pixel stripes */
    l63_bgi("<body style='margin:0'><div id=b style='width:60px;height:40px;background-image:url(/images/mire-g1.png);background-repeat:no-repeat'></div></body>");
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
    TEST_ASSERT_EQUAL(48, (int)g_doc.nitems);
    TEST_ASSERT_EQUAL(32, g_doc.items[0].h);
    TEST_ASSERT_EQUAL(47, g_doc.items[47].x);
    TEST_ASSERT_TRUE(l63_at(0, 0) != l63_at(1, 0));
    /* the natural 48px ramp restarts at x 48 */
    l63_bgi(L63_BGI("width:60px;height:4px", ""));
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
    TEST_ASSERT_EQUAL(60, (int)g_doc.nitems);
    TEST_ASSERT_EQUAL(l63_at(0, 0), l63_at(48, 3));
    TEST_ASSERT_EQUAL(l63_at(11, 0), l63_at(59, 0));
    TEST_ASSERT_TRUE(l63_at(47, 0) != l63_at(48, 0));
}

static void test_l63_bgi_repeat_and_position(void) {
    l63_bgi(L63_BGI("width:12px;height:8px", "background-size:6px 4px;background-repeat:no-repeat;background-position:3px 2px"));
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
    TEST_ASSERT_EQUAL(6, (int)g_doc.nitems);
    TEST_ASSERT_EQUAL(-1, l63_at(2, 2));
    TEST_ASSERT_EQUAL((long)l63_ramp[0], l63_at(3, 2));
    TEST_ASSERT_EQUAL((long)l63_ramp[5], l63_at(8, 5));
    TEST_ASSERT_EQUAL(-1, l63_at(9, 2));
    TEST_ASSERT_EQUAL(-1, l63_at(3, 1));
    TEST_ASSERT_EQUAL(-1, l63_at(3, 6));
    /* percentages place the tile in the free space: (12-6)/2, 8-4 */
    l63_bgi(L63_BGI("width:12px;height:8px", "background-size:6px 4px;background-repeat:no-repeat;background-position:50% 100%"));
    TEST_ASSERT_EQUAL((long)l63_ramp[0], l63_at(3, 4));
    TEST_ASSERT_EQUAL(-1, l63_at(3, 3));
    TEST_ASSERT_EQUAL(-1, l63_at(2, 7));
    TEST_ASSERT_EQUAL((long)l63_ramp[5], l63_at(8, 7));
    /* keywords go through the position grammar: right bottom */
    l63_bgi(L63_BGI("width:12px;height:8px", "background-size:6px 4px;background-repeat:no-repeat;background-position:bottom right"));
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
    TEST_ASSERT_EQUAL((long)l63_ramp[0], l63_at(6, 4));
    TEST_ASSERT_EQUAL(-1, l63_at(5, 4));
    /* repeat-x: one band of rows; repeat-y: one band of columns */
    l63_bgi(L63_BGI("width:12px;height:8px", "background-size:6px 4px;background-repeat:repeat-x;background-position:0 2px"));
    TEST_ASSERT_EQUAL(-1, l63_at(0, 1));
    TEST_ASSERT_EQUAL((long)l63_ramp[5], l63_at(11, 5));
    TEST_ASSERT_EQUAL(-1, l63_at(0, 6));
    l63_bgi(L63_BGI("width:12px;height:8px", "background-size:6px 4px;background-repeat:repeat-y"));
    TEST_ASSERT_EQUAL((long)l63_ramp[5], l63_at(5, 7));
    TEST_ASSERT_EQUAL(-1, l63_at(6, 0));
    l63_bgi(L63_BGI("width:12px;height:8px", "background-size:6px 4px;background-repeat:no-repeat repeat"));
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
    TEST_ASSERT_EQUAL((long)l63_ramp[5], l63_at(5, 7));
    TEST_ASSERT_EQUAL(-1, l63_at(6, 0));
    l63_bgi(L63_BGI("width:12px;height:8px", "background-size:6px 4px;background-repeat:no-repeat;background-repeat:repeat"));
    TEST_ASSERT_EQUAL(3, lot52_node("b")->bgi_repeat);
    TEST_ASSERT_EQUAL((long)l63_ramp[5], l63_at(11, 7));
    l63_bgi(L63_BGI("width:12px;height:8px", "background-size:6px 4px;background-repeat:repeat no-repeat"));
    TEST_ASSERT_EQUAL((long)l63_ramp[5], l63_at(11, 3));
    TEST_ASSERT_EQUAL(-1, l63_at(0, 4));
    /* a negative offset wraps into the tile: column 0 shows tile column 2 */
    l63_bgi(L63_BGI("width:12px;height:8px", "background-size:6px 4px;background-position:-2px -1px"));
    TEST_ASSERT_EQUAL((long)l63_ramp[2], l63_at(0, 0));
    TEST_ASSERT_EQUAL((long)l63_ramp[1], l63_at(11, 7));
    TEST_ASSERT_EQUAL(12, (int)g_doc.nitems);
}

static void test_l63_bgi_sizes(void) {
    /* contain on 24x8: the 48x32 image fits by height, 12x8 */
    l63_bgi(L63_BGI("width:24px;height:8px", "background-size:contain;background-repeat:no-repeat"));
    TEST_ASSERT_TRUE(l63_at(11, 7) >= 0);
    TEST_ASSERT_EQUAL(-1, l63_at(12, 0));
    /* contain on 6x40 fits by width: 6x4 */
    l63_bgi(L63_BGI("width:6px;height:40px", "background-size:contain;background-repeat:no-repeat"));
    TEST_ASSERT_EQUAL((long)l63_ramp[5], l63_at(5, 3));
    TEST_ASSERT_EQUAL(-1, l63_at(0, 4));
    /* cover on 24x8 fills by width: 24x16, cut to the box */
    l63_bgi(L63_BGI("width:24px;height:8px", "background-size:cover;background-repeat:no-repeat"));
    TEST_ASSERT_TRUE(l63_at(23, 7) >= 0);
    TEST_ASSERT_EQUAL(-1, l63_at(0, 8));
    TEST_ASSERT_EQUAL(24, (int)g_doc.nitems);               /* a 24px ramp, cut to 8 rows */
    TEST_ASSERT_EQUAL(8, g_doc.items[23].h);
    TEST_ASSERT_EQUAL(0x0A0A0A, l63_at(1, 0));
    /* cover on 6x40 fills by height: 60x40 */
    l63_bgi(L63_BGI("width:6px;height:40px", "background-size:cover;background-repeat:no-repeat"));
    TEST_ASSERT_EQUAL(5, (int)g_doc.nitems);                /* 60x40: columns 0-1 share a sample */
    TEST_ASSERT_EQUAL(40, g_doc.items[4].h);
    TEST_ASSERT_EQUAL(0, l63_at(1, 0));
    TEST_ASSERT_EQUAL(0x050505, l63_at(2, 0));
    /* one explicit size keeps the ratio */
    l63_bgi(L63_BGI("width:24px;height:16px", "background-size:12px auto;background-repeat:no-repeat"));
    TEST_ASSERT_TRUE(l63_at(11, 7) >= 0);
    TEST_ASSERT_EQUAL(-1, l63_at(0, 8));
    TEST_ASSERT_EQUAL(-1, l63_at(12, 0));
    l63_bgi(L63_BGI("width:24px;height:16px", "background-size:12px;background-repeat:no-repeat"));
    TEST_ASSERT_EQUAL(-1, l63_at(0, 8));
    TEST_ASSERT_TRUE(l63_at(11, 7) >= 0);
    l63_bgi(L63_BGI("width:24px;height:16px", "background-size:auto 4px;background-repeat:no-repeat"));
    TEST_ASSERT_EQUAL((long)l63_ramp[5], l63_at(5, 3));
    TEST_ASSERT_EQUAL(-1, l63_at(6, 0));
    TEST_ASSERT_EQUAL(-1, l63_at(0, 4));
    /* percentages of the positioning area */
    l63_bgi(L63_BGI("width:24px;height:16px", "background-size:25% 25%;background-repeat:no-repeat"));
    TEST_ASSERT_EQUAL((long)l63_ramp[5], l63_at(5, 3));
    TEST_ASSERT_EQUAL(-1, l63_at(6, 0));
    TEST_ASSERT_EQUAL(-1, l63_at(0, 4));
    /* a later position keeps the size percentages */
    l63_bgi(L63_BGI("width:24px;height:16px", "background-size:25% 25%;background-position:0 0;background-repeat:no-repeat"));
    TEST_ASSERT_EQUAL(-1, l63_at(6, 0));
    l63_bgi(L63_BGI("width:24px;height:16px", "background-size:auto;background-repeat:no-repeat"));
    TEST_ASSERT_TRUE(l63_at(23, 15) >= 0);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->bgi_size_kind);
    /* a zero size paints nothing */
    l63_bgi(L63_BGI("width:24px;height:16px", "background-size:0 4px"));
    TEST_ASSERT_EQUAL(0, (int)g_doc.nitems);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l63_bgi_boxes_and_order(void) {
    /* positioned in the padding box, painted to the border box (wrapping) */
    l63_bgi(L63_BGI("width:6px;height:4px;padding:2px;border:1px solid #00ff00", "background-size:6px 4px"));
    TEST_ASSERT_EQUAL((long)l63_ramp[0], l63_at(1, 1));
    TEST_ASSERT_EQUAL((long)l63_ramp[5], l63_at(0, 1));
    l63_bgi(L63_BGI("width:6px;height:4px;padding:2px", "background-size:6px 4px;background-origin:content-box;background-clip:padding-box;background-repeat:no-repeat"));
    TEST_ASSERT_EQUAL(-1, l63_at(1, 2));
    TEST_ASSERT_EQUAL((long)l63_ramp[0], l63_at(2, 2));
    l63_bgi(L63_BGI("width:6px;height:4px;padding:2px", "background-size:6px 4px;background-clip:content-box"));
    TEST_ASSERT_EQUAL(-1, l63_at(1, 2));
    TEST_ASSERT_EQUAL((long)l63_ramp[2], l63_at(2, 2));
    TEST_ASSERT_EQUAL((long)l63_ramp[4], l63_at(4, 2));
    TEST_ASSERT_EQUAL(-1, l63_at(8, 2));
    /* over the colour, under the children; not inherited */
    l63_bgi("<body style='margin:0'><div id=b style='width:12px;background:#ff0000;background-image:url(/images/mire-ga16.png);background-size:6px 4px'>"
            "<div id=c style='height:8px;border:1px solid #0000ff'></div></div></body>");
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
    TEST_ASSERT_EQUAL(0xFF0000u, g_doc.items[0].color);
    TEST_ASSERT_EQUAL((long)l63_ramp[0], l63_at(0, 9));
    TEST_ASSERT_EQUAL(0, lot52_node("c")->bgi_len);
    TEST_ASSERT_EQUAL(1, lot52_read_calls);
    TEST_ASSERT_EQUAL(lot52_node("c") - g_doc.nodes, g_doc.item_owner[g_doc.nitems - 1]);
    /* under the inset shadow; nested images keep each box's order */
    l63_bgi(L63_BGI("width:8px;height:8px;box-shadow:inset 0 0 0 1px #ff0000", "background-size:4px 4px"));
    TEST_ASSERT_EQUAL(12, (int)g_doc.nitems);
    TEST_ASSERT_EQUAL((long)0xFF0000, l63_at(0, 0));
    TEST_ASSERT_EQUAL((long)0xFF0000, l63_at(7, 3));
    TEST_ASSERT_EQUAL(0, g_doc.items[4].color);
    TEST_ASSERT_EQUAL(0xFF0000u, g_doc.items[8].color);
    l63_bgi("<body style='margin:0'><div id=p style='width:12px;padding:2px;background-image:url(/images/mire-ga16.png);background-size:6px 4px;box-shadow:inset 0 0 0 1px #ff0000'>"
            "<div id=c style='height:4px;background-image:url(/images/mire-ga16.png);background-size:2px 2px;background-repeat:no-repeat'></div></div></body>");
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
    TEST_ASSERT_EQUAL(16 + 4 + 2, (int)g_doc.nitems);       /* 16 parent columns, its inset, 2 child columns */
    TEST_ASSERT_EQUAL(lot52_node("p") - g_doc.nodes, g_doc.item_owner[15]);
    TEST_ASSERT_EQUAL(0xFF0000u, g_doc.items[16].color);
    TEST_ASSERT_EQUAL(0xFF0000u, g_doc.items[19].color);
    TEST_ASSERT_EQUAL(lot52_node("c") - g_doc.nodes, g_doc.item_owner[20]);
    TEST_ASSERT_EQUAL(2, g_doc.items[20].x);
    TEST_ASSERT_EQUAL(2, g_doc.items[20].h);
    TEST_ASSERT_EQUAL(3, g_doc.items[21].x);
    /* quotes; a later background or gradient replaces the url */
    l63_bgi("<body style='margin:0'><div id=b style=\"width:6px;height:4px;background-image:url( '/images/mire-ga16.png' );background-size:6px 4px\"></div></body>");
    TEST_ASSERT_EQUAL(6, (int)g_doc.nitems);
    l63_bgi("<body style='margin:0'><div id=b style='width:6px;height:4px;background-image:url(\"/images/mire-ga16.png\");background-size:6px 4px'></div></body>");
    TEST_ASSERT_EQUAL(6, (int)g_doc.nitems);
    l63_bgi(L63_BGI("width:6px;height:4px", "background:#ff0000"));
    TEST_ASSERT_EQUAL(1, (int)g_doc.nitems);
    TEST_ASSERT_EQUAL(0, lot52_read_calls);
    l63_bgi(L63_BGI("width:6px;height:4px", "background-image:none"));
    TEST_ASSERT_EQUAL(0, (int)g_doc.nitems);
    l63_bgi(L63_BGI("width:6px;height:4px", "background-image:linear-gradient(#ff0000,#ff0000)"));
    TEST_ASSERT_EQUAL(0, lot52_read_calls);
    TEST_ASSERT_EQUAL(0xFF0000u, g_doc.items[0].color);
    l63_bgi("<body style='margin:0'><div id=b style='width:6px;height:4px;background-image:linear-gradient(#ff0000,#ff0000);background-image:url(/images/mire-ga16.png);background-size:6px 4px'></div></body>");
    TEST_ASSERT_EQUAL(0, lot52_node("b")->grad_dir);
    TEST_ASSERT_EQUAL(6, (int)g_doc.nitems);
    /* the url is read from a <style> rule as well */
    l63_bgi("<style>#b{background-image:url(/images/mire-ga16.png);background-size:6px 4px}</style><body style='margin:0'><div id=b style='width:6px;height:4px'></div></body>");
    TEST_ASSERT_EQUAL(6, (int)g_doc.nitems);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l63_bgi_refusals(void) {
    static const char *const bad_image[] = {"url()", "url(\"\")", "url(/images/mire-ga16.png", "uri(/images/mire-ga16.png)",
        "url(/images/mire-ga16.png) red", "image(/images/mire-ga16.png)", 0};
    static const char *const bad_repeat[] = {"spaces", "rounded", "repeat-x repeat", "repeat repeat repeat", "", 0};
    static const char *const bad_size[] = {"-1px", "contain cover", "1px 2px 3px", "auto contain", "4q", 0};
    char html[400];
    int k;
    for (k = 0; bad_image[k]; k++) {
        snprintf(html, sizeof(html), "<div id=b style='width:6px;height:4px;background-image:%s'></div>", bad_image[k]);
        l63_bgi(html);
        TEST_ASSERT_TRUE(lot52_has_diag("background image refused"));
        TEST_ASSERT_EQUAL(0, lot52_node("b")->bgi_len);
        TEST_ASSERT_EQUAL(0, lot52_read_calls);
    }
    for (k = 0; bad_repeat[k]; k++) {
        snprintf(html, sizeof(html), "<div id=b style='width:6px;height:4px;background-repeat:%s'></div>", bad_repeat[k]);
        l63_bgi(html);
        if (bad_repeat[k][0]) TEST_ASSERT_TRUE(lot52_has_diag("background repeat refused"));
        TEST_ASSERT_EQUAL(3, lot52_node("b")->bgi_repeat);
    }
    for (k = 0; bad_size[k]; k++) {
        snprintf(html, sizeof(html), "<div id=b style='width:6px;height:4px;background-size:%s'></div>", bad_size[k]);
        l63_bgi(html);
        TEST_ASSERT_TRUE(lot52_has_diag("background size refused"));
        TEST_ASSERT_EQUAL(WE_BGS_AUTO, lot52_node("b")->bgi_size_kind);
        TEST_ASSERT_EQUAL(-1, lot52_node("b")->bgi_size[0]);
    }
    l63_bgi("<div id=b style='background-position:left left'></div>");
    TEST_ASSERT_TRUE(lot52_has_diag("background position refused"));
    TEST_ASSERT_EQUAL(3, lot52_node("b")->bgi_pos_pct);
    /* an <img> keeps its source */
    l63_bgi("<img id=b src=/images/mire-g1.png style='background-image:url(/images/mire-ga16.png)'>");
    TEST_ASSERT_TRUE(lot52_has_diag("background image refused"));
    TEST_ASSERT_EQUAL_STRING("/images/mire-g1.png", lot52_last_path);
    /* the path rules of <img src>, and the reader failures */
    l63_bgi(L63_BGI("width:6px;height:4px", "background-image:url(/images/../etc/x.png)"));
    TEST_ASSERT_TRUE(lot52_has_diag("image local path refused"));
    TEST_ASSERT_EQUAL(0, lot52_read_calls);
    TEST_ASSERT_EQUAL(0, (int)g_doc.nitems);
    l63_bgi(L63_BGI("width:6px;height:4px", "background-image:url(http://x/a.png)"));
    TEST_ASSERT_TRUE(lot52_has_diag("image local path refused"));
    l63_bgi(L63_BGI("width:6px;height:4px", "background-image:url(/images/none.png)"));
    TEST_ASSERT_TRUE(lot52_has_diag("image read failed"));
    TEST_ASSERT_EQUAL(0, (int)g_doc.nitems);
    we_set_image_reader(0, 0);
    TEST_ASSERT_EQUAL(0, we_render(&g_doc, L63_BGI("width:6px;height:4px", ""), (int)strlen(L63_BGI("width:6px;height:4px", "")), 320));
    TEST_ASSERT_TRUE(lot52_has_diag("image reader unavailable"));
    /* more runs than reserved: refused whole, nothing painted */
    l63_bgi("<body style='margin:0'><div id=b style='width:12px;height:8px;background:#00ff00;background-image:url(/images/mire-rgb16.png);background-size:6px 4px'></div></body>");
    TEST_ASSERT_TRUE(lot52_has_diag("background image budget"));
    TEST_ASSERT_EQUAL(1, (int)g_doc.nitems);
    TEST_ASSERT_EQUAL(0x00FF00u, g_doc.items[0].color);
    /* the whole display list fits: 96 one-pixel columns, not 97 */
    l63_bgi("<body style='margin:0'><div id=b style='width:96px;height:2px;background-image:url(/images/mire-g1.png)'></div></body>");
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
    TEST_ASSERT_EQUAL(OS_FB_WEB_ITEMS, (int)g_doc.nitems);
    l63_bgi("<body style='margin:0'><div id=b style='width:97px;height:2px;background-image:url(/images/mire-g1.png)'></div></body>");
    TEST_ASSERT_TRUE(lot52_has_diag("background image budget"));
    TEST_ASSERT_EQUAL(0, (int)g_doc.nitems);
    TEST_ASSERT_EQUAL(0, g_doc.perf.items_dropped);
    l63_bgi("<body style='margin:0'><div id=b style='width:96px;height:2px;background:#ff0000;background-image:url(/images/mire-g1.png)'></div></body>");
    TEST_ASSERT_TRUE(lot52_has_diag("background image budget"));
    TEST_ASSERT_EQUAL(1, (int)g_doc.nitems);
    /* hidden boxes paint no image */
    l63_bgi(L63_BGI("width:6px;height:4px;visibility:hidden", "background-size:6px 4px"));
    TEST_ASSERT_EQUAL(0, (int)g_doc.nitems);
    TEST_ASSERT_EQUAL(0, lot52_read_calls);
    we_set_image_reader(0, 0);
}

static void lot63_bgimage_run(void) {
    RUN_TEST(test_l63_bgi_tiles_repeat_both_axes);
    RUN_TEST(test_l63_bgi_repeat_and_position);
    RUN_TEST(test_l63_bgi_sizes);
    RUN_TEST(test_l63_bgi_boxes_and_order);
    RUN_TEST(test_l63_bgi_refusals);
}
