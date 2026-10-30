/* Lot 62: the combined guest page also served to QEMU (/web/lot62.html). */
static void l62_load_page(void) {
    static char html[4096];
    FILE *file = fopen("initrd_content/web/lot62.html", "rb");
    int count;
    if (!file) file = fopen("../initrd_content/web/lot62.html", "rb");
    TEST_ASSERT_NOT_NULL(file);
    if (!file) return;
    count = (int)fread(html, 1, sizeof(html) - 1, file); fclose(file); html[count] = 0;
    TEST_ASSERT_EQUAL(0, we_render(&g_doc, html, count, 640));
}

static void test_l62_page_combines_lot_behaviours(void) {
    l62_load_page();
    l61_rect_at(0, 0, 60, 10, 0x80007F);                  /* rgba(255,0,0,.5) over blue */
    l61_rect_at(0, 0, 120, 10, 0x0000FF);
    l61_rect_at(0, 10, 23, 1, 0x75B5F5);                  /* ridge: light outer top */
    l61_rect_at(1, 11, 21, 1, 0x2060A0);                  /* dark inner top */
    l61_rect_at(25, 18, 20, 10, 0x008000);                /* translate(30px) scale(2,1) */
    l61_box("ol", 2, 30, 20, 6);
    TEST_ASSERT_EQUAL(OS_FB_WEB_BORDER, g_doc.items[g_doc.nitems - 1].kind);  /* outline last */
    L62_HEX(0xFF00FF, g_doc.items[g_doc.nitems - 1].color);
    TEST_ASSERT_EQUAL(0, g_doc.items[g_doc.nitems - 1].x);
    l61_rect_at(4, 44, 20, 2, 0x808080);                  /* shadow under the box */
    TEST_ASSERT_TRUE(l61_index(0x808080) < l61_index(0x000080));
    l61_rect_at(0, 53, 16, 1, 0xFF0000);                  /* underline */
    l61_box("pc", 30, 54, 90, 4);                         /* 25% of 120 */
    TEST_ASSERT_TRUE(l61_index(0xFFA500) < (int)(find_text("UP") - g_doc.items));
    TEST_ASSERT_EQUAL(1, find_text("UP")->x);                /* the shadow first, then the run */
    L62_HEX(0xC0C0C0, find_text("UP")->color);
    l61_rect_at(0, 68, 120, 1, 0x101010);                 /* gradient bands */
    l61_rect_at(0, 75, 120, 1, 0xEFEFEF);
    l61_rect_at(3, 76, 14, 2, 0x800000);                  /* rounded corner */
    TEST_ASSERT_NOT_NULL(find_text("ab..."));
    TEST_ASSERT_NOT_NULL(find_text("iv."));
    TEST_ASSERT_EQUAL(108, g_doc.perf.page_h);
    TEST_ASSERT_EQUAL(0, g_doc.perf.items_dropped);
    TEST_ASSERT_EQUAL(2, g_doc.ndiag);
    TEST_ASSERT_EQUAL(1, l61_diag_count("gradient painted in bands"));
    TEST_ASSERT_EQUAL(1, l61_diag_count("border-radius painted in steps"));
    l62_no_alpha_left();
}

static void lot62_page_run(void) {
    RUN_TEST(test_l62_page_combines_lot_behaviours);
}
