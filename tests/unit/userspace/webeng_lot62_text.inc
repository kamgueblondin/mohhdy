/* Lot 62: text-transform, text-shadow and background-clip / -origin. */
static void test_l62_text_transform_modes(void) {
    lot52_render(L62_BODY "<p style='margin:0;text-transform:uppercase'>ab <b>cd</b></p>"
                 "<p style='margin:0;text-transform:capitalize'>hello wo<i>rld</i> x</p>"
                 "<div style='text-transform:lowercase'>MiXeD <span style='text-transform:none'>KeEp</span></div></body>", 320);
    TEST_ASSERT_NOT_NULL(find_text("AB"));
    TEST_ASSERT_NOT_NULL(find_text("CD"));                /* inherited */
    TEST_ASSERT_NOT_NULL(find_text("Hello Wo"));
    TEST_ASSERT_NOT_NULL(find_text("rld"));               /* same word, not a start */
    TEST_ASSERT_NOT_NULL(find_text("X"));
    TEST_ASSERT_NOT_NULL(find_text("mixed"));
    TEST_ASSERT_NOT_NULL(find_text("KeEp"));
    TEST_ASSERT_EQUAL(24, find_text("CD")->x);            /* widths unchanged */
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l62_text_shadow_paints_under_the_run(void) {
    const os_fb_web_item_t *t;
    lot52_render(L62_BODY "<p style='margin:0;color:#0000ff;text-shadow:1px 2px #ff0000, -1px 0'>AB</p></body>", 320);
    TEST_ASSERT_EQUAL(3, (int)g_doc.nitems);
    t = &g_doc.items[0];                                  /* second shadow, text colour */
    TEST_ASSERT_EQUAL(-1, t->x); TEST_ASSERT_EQUAL(1, t->y);
    L62_HEX(0x0000FF, t->color);
    t = &g_doc.items[1];                                  /* first shadow on top of it */
    TEST_ASSERT_EQUAL(1, t->x); TEST_ASSERT_EQUAL(3, t->y);
    L62_HEX(0xFF0000, t->color);
    TEST_ASSERT_EQUAL_STRING("AB", t->text);
    TEST_ASSERT_EQUAL(0, g_doc.items[2].x);
    L62_HEX(0x0000FF, g_doc.items[2].color);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
    /* inherited, removed by none, blur painted sharp */
    lot52_render(L62_BODY "<div style='text-shadow:0 1px 2px #00ff00'>A<span style='text-shadow:none'>B</span></div></body>", 320);
    TEST_ASSERT_EQUAL(3, (int)g_doc.nitems);
    TEST_ASSERT_EQUAL(1, l62_items_with(0x00FF00));
    TEST_ASSERT_EQUAL(1, l61_diag_count("text-shadow blur painted sharp"));
}

static void test_l62_text_shadow_and_transform_refusals(void) {
    lot52_render(L62_BODY "<p id=a style='text-shadow:1px 1px;text-shadow:1px 1px 1px 1px;text-shadow:inset 1px 1px;"
                 "text-shadow:1px;text-shadow:1px 1px, 1px 1px, 1px 1px;text-transform:uppercase;text-transform:full-width'>a</p></body>", 320);
    TEST_ASSERT_EQUAL(4, l61_diag_count("text-shadow refused"));
    TEST_ASSERT_EQUAL(1, l61_diag_count("text-transform refused"));
    TEST_ASSERT_EQUAL(1, lot52_node("a")->tshadow_count);
    TEST_ASSERT_NOT_NULL(find_text("A"));
}

static void test_l62_background_clip_and_origin(void) {
    lot52_render(L62_BODY "<div style='border:2px solid #000000;padding:3px;width:10px;height:10px;background:#00ff00;background-clip:content-box'></div>"
                 "<div style='border:2px solid #000000;padding:3px;width:10px;height:10px;background:#00ff00;background-clip:padding-box'></div>"
                 "<div style='border:2px solid #000000;padding:2px;width:4px;height:8px;background:linear-gradient(#ff0000,#0000ff);"
                 "background-clip:padding-box;background-origin:content-box'></div>"
                 "<div style='border:2px solid #000000;width:4px;height:8px;background:linear-gradient(#ff0000,#0000ff);background-origin:border-box;background-clip:content-box'></div></body>", 320);
    l61_rect_at(5, 5, 10, 10, 0x00FF00);
    l61_rect_at(2, 22, 16, 16, 0x00FF00);
    l61_rect_at(4, 44, 4, 1, 0xEF0010);                  /* 8 bands over the content box */
    l61_rect_at(4, 51, 4, 1, 0x1000EF);
    /* bands over the 12 px border box, cut to the content box: the first and
     * last band fall in the border and are dropped */
    l61_rect_at(2, 58, 4, 1, 0xCF0030);
    l61_rect_at(2, 65, 4, 1, 0x3000CF);
    TEST_ASSERT_EQUAL(2 + 2 + 9 + 7, (int)g_doc.nitems);
    lot52_render(L62_BODY "<div style='background-clip:margin-box;background-origin:text'></div></body>", 320);
    TEST_ASSERT_EQUAL(2, l61_diag_count("background box refused"));
}

static void lot62_text_run(void) {
    RUN_TEST(test_l62_text_transform_modes);
    RUN_TEST(test_l62_text_shadow_paints_under_the_run);
    RUN_TEST(test_l62_text_shadow_and_transform_refusals);
    RUN_TEST(test_l62_background_clip_and_origin);
}
