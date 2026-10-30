/* Lot 61: the combined guest page also served to QEMU (/web/lot61.html). */
static void l61_load_page(void) {
    static char html[4096];
    FILE *file = fopen("initrd_content/web/lot61.html", "rb");
    int count;
    if (!file) file = fopen("../initrd_content/web/lot61.html", "rb");
    TEST_ASSERT_NOT_NULL(file);
    if (!file) return;
    count = (int)fread(html, 1, sizeof(html) - 1, file); fclose(file); html[count] = 0;
    /* QEMU lays the page out 640 wide; render at the same width. */
    we_set_image_reader(lot52_reader, 0);
    TEST_ASSERT_EQUAL(0, we_render(&g_doc, html, count, 640));
    we_set_image_reader(0, 0);
}

static void test_l61_page_combines_lot_behaviours(void) {
    int k, fragments = 0;
    l61_load_page();
    l61_box("fa", 0, 10, 50, 10);      /* last baseline aligned with fb's second line */
    l61_box("fb", 50, 0, 50, 24);
    l61_box("sub", 0, 24, 120, 10);    /* subgrid over three auto-fill tracks */
    l61_box("sa", 40, 24, 80, 10);
    l61_box("sb", 0, 24, 40, 10);
    l61_box("ib", 16, 34, 20, 16);     /* inline-block on the text baseline */
    l61_box("clip", 0, 64, 12, 10);
    l61_box("image", 0, 74, 32, 21);
    l61_box("fix", 610, 470, 30, 10);  /* fixed to the 640x480 viewport */
    l61_rect_is(0x800080, 610, 470, 30, 10);
    /* Collapsed table: the wider 4px table border wins the outer edges,
     * the 2px cell border keeps the interior edge. */
    l61_rect_at(0, 95, 120, 4, 0x0000FF);
    l61_rect_at(0, 101, 120, 4, 0x0000FF);
    l61_rect_at(0, 95, 4, 10, 0x0000FF);
    l61_rect_at(116, 95, 4, 10, 0x0000FF);
    l61_rect_at(59, 95, 2, 10, 0xFF0000);
    TEST_ASSERT_EQUAL(0xFF0000, (int)l61_paint(60, 96));   /* vertical over horizontal */
    TEST_ASSERT_EQUAL(0x0000FF, (int)l61_paint(2, 100));
    /* Subgrid column-gap 10px over 40px parent tracks. */
    l61_box("s2a", 0, 105, 35, 6);
    l61_box("s2b", 45, 105, 30, 6);
    /* z-index:-1 paints first, below the overlapping in-flow block. */
    TEST_ASSERT_EQUAL(0, l61_index(0x00FF00));
    TEST_ASSERT_EQUAL(0x00FF00, (int)l61_paint(5, 112));
    TEST_ASSERT_EQUAL(0x000080, (int)l61_paint(5, 115));
    for (k = 0; k < (int)g_doc.nitems; k++)
        if (g_doc.items[k].kind == OS_FB_WEB_RECT && g_doc.items[k].color == 0 &&
            g_doc.items[k].x >= 8 && g_doc.items[k].x + g_doc.items[k].w <= 12 &&
            g_doc.items[k].y >= 65 && g_doc.items[k].y + g_doc.items[k].h <= 73) fragments++;
    TEST_ASSERT_EQUAL(3, fragments);   /* the second W is cut at x 12 */
    TEST_ASSERT_EQUAL(120, g_doc.perf.page_h);
    TEST_ASSERT_EQUAL(1, g_doc.perf.images);
    TEST_ASSERT_EQUAL(0, g_doc.perf.items_dropped);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l61_page_scroll_moves_only_fixed(void) {
    l61_load_page();
    /* Relayout re-reads the image, so the reader stays installed. */
    we_set_image_reader(lot52_reader, 0);
    TEST_ASSERT_EQUAL(0, we_layout_viewport(&g_doc, 640, 40));
    we_set_image_reader(0, 0);
    TEST_ASSERT_EQUAL(120, g_doc.perf.page_h);
    l61_box("fix", 610, 30, 30, 10);
    TEST_ASSERT_EQUAL(80, we_scroll(&g_doc, 1000));   /* clamped to 120 - 40 */
    l61_box("fix", 610, 110, 30, 10);
    l61_box("sa", 40, 24, 80, 10);
    l61_rect_is(0x800080, 610, 110, 30, 10);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void lot61_page_run(void) {
    RUN_TEST(test_l61_page_combines_lot_behaviours);
    RUN_TEST(test_l61_page_scroll_moves_only_fixed);
}
