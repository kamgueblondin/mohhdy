/* Lot 64: the combined guest page also served to QEMU (/web/lot64.html). */
static void l64_load_page(void) {
    static char html[4096];
    FILE *file = fopen("initrd_content/web/lot64.html", "rb");
    int count;
    if (!file) file = fopen("../initrd_content/web/lot64.html", "rb");
    lot52_read_mode = 0;
    we_set_image_reader(lot52_reader, 0);
    TEST_ASSERT_NOT_NULL(file);
    if (!file) return;
    count = (int)fread(html, 1, sizeof(html) - 1, file); fclose(file); html[count] = 0;
    TEST_ASSERT_EQUAL(0, we_render(&g_doc, html, count, 640));
}

static void test_l64_page_combines_lot_behaviours(void) {
    l64_load_page();
    l64_glyph('a', 1, 0);                                 /* vertical-rl, upright glyphs */
    l64_glyph('b', 1, 8);
    l61_rect_at(10, 0, 2, 16, 0xFF0000);                  /* border-block-start: the right */
    l61_text_at("cd", 104, 17);                           /* dir=rtl: start at the right */
    l61_text_at("e", 0, 27);                              /* two columns, then a spanner */
    l61_text_at("f", 24, 27);
    l61_rect_at(20, 26, 1, 10, 0x008000);                 /* the rule of the first segment */
    l61_rect_at(0, 36, 40, 4, 0x0000FF);                  /* column-span: all */
    l61_text_at("g", 0, 41);
    l61_rect_at(0, 50, 28, 2, 0x800080);                  /* per-side styles and widths */
    l61_rect_at(0, 52, 4, 4, 0x008080);
    l61_rect_at(24, 52, 4, 4, 0x008080);
    l61_rect_at(12, 61, 1, 4, 0x000000);                  /* right 2px bottom 1px */
    l61_rect_at(17, 61, 1, 4, 0xC8C8C8);
    l61_rect_at(0, 66, 2, 2, 0x000000);                   /* the upper layer over the lower */
    l61_rect_at(11, 66, 1, 4, 0xC8C8C8);
    l61_text_at("h", 72, 71);                             /* rtl list */
    l61_text_at(".1", 88, 71);
    l61_rect_at(8, 80, 2, 2, 0x000080);                   /* dotted rounded border */
    l61_rect_at(1, 86, 2, 2, 0x000080);                   /* a dot on the arc */
    TEST_ASSERT_EQUAL(48, (int)g_doc.nitems);
    TEST_ASSERT_EQUAL(96, g_doc.perf.page_h);
    TEST_ASSERT_EQUAL(0, g_doc.perf.items_dropped);
    TEST_ASSERT_EQUAL(2, g_doc.ndiag);
    TEST_ASSERT_EQUAL(1, l61_diag_count("vertical text painted upright"));
    TEST_ASSERT_EQUAL(1, l61_diag_count("border-radius painted in steps"));
}

static void lot64_page_run(void) {
    RUN_TEST(test_l64_page_combines_lot_behaviours);
}
