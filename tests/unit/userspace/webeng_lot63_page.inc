/* Lot 63: the combined guest page also served to QEMU (/web/lot63.html). */
static void l63_load_page(void) {
    static char html[4096];
    FILE *file = fopen("initrd_content/web/lot63.html", "rb");
    int count;
    if (!file) file = fopen("../initrd_content/web/lot63.html", "rb");
    lot52_read_mode = 0;
    we_set_image_reader(lot52_reader, 0);
    TEST_ASSERT_NOT_NULL(file);
    if (!file) return;
    count = (int)fread(html, 1, sizeof(html) - 1, file); fclose(file); html[count] = 0;
    TEST_ASSERT_EQUAL(0, we_render(&g_doc, html, count, 640));
}

static void test_l63_page_combines_lot_behaviours(void) {
    l63_load_page();
    l61_rect_at(0, 0, 40, 4, 0xFF0000);                   /* a block ::before box */
    l61_text_at("-x", 0, 5);                              /* &mdash; as '-' */
    l61_text_at("[gc]", 16, 5);                           /* ::after with attr(id) */
    L62_HEX(0x0000FF, find_text("[gc]")->color);
    l61_text_at("VI.", 0, 15);                            /* counter-reset 4, increment 2 */
    l61_text_at("VIII.", 0, 25);
    l61_text_at("<", 0, 35);                              /* quotes */
    l61_text_at(">", 24, 35);
    l61_rect_at(0, 50, 20, 8, 0x2B00D4);                  /* radial closest-side at 10px 50% */
    l61_rect_at(22, 70, 14, 3, 0x008000);                 /* rotate(45deg) band */
    l61_rect_at(6, 95, 20, 1, 0x800080);                  /* skewX(45deg) last row */
    l61_rect_at(6, 98, 4, 2, 0x000080);                   /* dashed straight part */
    l61_rect_at(3, 98, 3, 2, 0x000080);                   /* its solid arc */
    TEST_ASSERT_EQUAL(0xFFFFFFFFu, l61_paint(0, 98));
    l61_rect_at(50, 116, 20, 8, 0xFFA500);                /* rounded clip, translated */
    l61_rect_at(55, 110, 10, 3, 0xFFA500);
    l61_text_at("ab", 0, 131);                            /* overflow-x: clip */
    l61_rect_at(10, 140, 4, 8, 0xFF00FF);                 /* collapsed column line band 4 */
    l61_rect_at(10, 140, 14, 2, 0x008080);                /* row line band 2 */
    l61_text_at("abc-", 0, 149);                          /* soft hyphen break */
    l61_text_at("defgh", 0, 159);
    l61_rect_at(30, 168, 10, 4, 0x00FFFF);                /* matrix(1,0,0,1,30,0) */
    l61_rect_at(0, 172, 1, 4, 0x000000);                  /* background-image tiles */
    l61_rect_at(5, 172, 1, 4, 0xC8C8C8);
    l61_rect_at(6, 172, 1, 4, 0x000000);
    l61_rect_at(11, 172, 1, 4, 0xC8C8C8);
    TEST_ASSERT_EQUAL(83, (int)g_doc.nitems);
    TEST_ASSERT_EQUAL(176, g_doc.perf.page_h);
    TEST_ASSERT_EQUAL(0, g_doc.perf.items_dropped);
    TEST_ASSERT_EQUAL(2, g_doc.ndiag);
    TEST_ASSERT_EQUAL(1, l61_diag_count("gradient painted in bands"));
    TEST_ASSERT_EQUAL(1, l61_diag_count("border-radius painted in steps"));
}

static void lot63_page_run(void) {
    RUN_TEST(test_l63_page_combines_lot_behaviours);
}
