/* Lot 62: linear-gradient backgrounds, axis-aligned, as at most 8 bands
 * taking the colour at their middle (premultiplied interpolation). */
static const uint32_t l62_grey_bands[8] = { 0x101010, 0x303030, 0x505050, 0x707070, 0x8F8F8F, 0xAFAFAF, 0xCFCFCF, 0xEFEFEF };

static void test_l62_gradient_vertical_bands(void) {
    int k;
    lot52_render(L62_BODY "<div id=a style='height:80px;background-image:linear-gradient(#000000, #ffffff);background-color:#ff0000'></div></body>", 320);
    TEST_ASSERT_EQUAL(9, (int)g_doc.nitems);
    l61_rect_at(0, 0, 320, 80, 0xFF0000);
    TEST_ASSERT_EQUAL(0, l61_index(0xFF0000));            /* the colour paints under the image */
    for (k = 0; k < 8; k++) {
        l61_rect_at(0, 10 * k, 320, 10, l62_grey_bands[k]);
        TEST_ASSERT_EQUAL(k + 1, l61_index(l62_grey_bands[k]));
    }
    TEST_ASSERT_EQUAL(1, l61_diag_count("gradient painted in bands"));
    lot52_render(L62_BODY "<div style='height:80px;background:linear-gradient(to top, #ffffff, #000000)'></div></body>", 320);
    for (k = 0; k < 8; k++) l61_rect_at(0, 10 * k, 320, 10, l62_grey_bands[k]);
    TEST_ASSERT_EQUAL(8, (int)g_doc.nitems);
}

static void test_l62_gradient_directions_and_stops(void) {
    static const char *const forms[3] = { "to left", "-90deg", "0.75turn" };
    int f;
    for (f = 0; f < 3; f++) {
        char html[256];
        snprintf(html, sizeof(html), L62_BODY "<div style='width:40px;height:4px;background:linear-gradient(%s, red 25%%, blue 75%%)'></div></body>", forms[f]);
        lot52_render(html, 320);
        l61_rect_at(0, 0, 5, 4, 0x0000FF);
        l61_rect_at(10, 0, 5, 4, 0x2000DF);
        l61_rect_at(15, 0, 5, 4, 0x60009F);
        l61_rect_at(20, 0, 5, 4, 0x9F0060);
        l61_rect_at(35, 0, 5, 4, 0xFF0000);
    }
    lot52_render(L62_BODY "<div style='width:40px;height:4px;background:linear-gradient(90deg, red 25%, blue 75%)'></div></body>", 320);
    l61_rect_at(0, 0, 5, 4, 0xFF0000);
    l61_rect_at(10, 0, 5, 4, 0xDF0020);
}

static void test_l62_gradient_positions(void) {
    /* the middle stop spreads to 50%; a smaller position clamps up (hard stop) */
    lot52_render(L62_BODY "<div style='height:8px;background:linear-gradient(#000000, #ffffff, #000000)'></div>"
                 "<div style='height:8px;background:linear-gradient(#ff0000 50%, #0000ff 20%)'></div></body>", 320);
    l61_rect_at(0, 0, 320, 1, 0x202020);
    l61_rect_at(0, 3, 320, 1, 0xDFDFDF);
    l61_rect_at(0, 4, 320, 1, 0xDFDFDF);
    l61_rect_at(0, 7, 320, 1, 0x202020);
    l61_rect_at(0, 11, 320, 1, 0xFF0000);
    l61_rect_at(0, 12, 320, 1, 0x0000FF);
    /* the clamped position also starts the next segment: blue at 50% */
    lot52_render(L62_BODY "<div style='height:8px;background:linear-gradient(#ff0000 50%, #0000ff 20%, #ffffff)'></div></body>", 320);
    l61_rect_at(0, 5, 320, 1, 0x6060FF);
    l61_rect_at(0, 7, 320, 1, 0xDFDFFF);
}

static void test_l62_gradient_alpha_and_small_boxes(void) {
    lot52_render(L62_BODY "<div style='height:3px;background:linear-gradient(180deg, rgba(255,0,0,0), #0000ff)'></div>"
                 "<div style='height:0;background:linear-gradient(red, blue)'></div>"
                 "<div style='height:2px;background:linear-gradient(red, red)'></div></body>", 320);
    l61_rect_at(0, 0, 320, 1, 0xD5D5FF);                  /* premultiplied: no red fringe */
    l61_rect_at(0, 1, 320, 1, 0x7F7FFF);
    l61_rect_at(0, 2, 320, 1, 0x2B2BFF);
    l61_rect_at(0, 3, 320, 1, 0xFF0000);
    TEST_ASSERT_EQUAL(5, (int)g_doc.nitems);
    l62_no_alpha_left();
}

static void test_l62_gradient_cascade(void) {
    lot52_render(L62_BODY "<div style='height:4px;background:linear-gradient(red, blue);background:#00ff00'></div>"
                 "<div style='height:4px;background-image:linear-gradient(red, blue);background-image:none'></div>"
                 "<div id=c style='height:4px;background:#00ff00;background:linear-gradient(red, red)'></div></body>", 320);
    TEST_ASSERT_EQUAL(1 + 4, (int)g_doc.nitems);
    l61_rect_at(0, 0, 320, 4, 0x00FF00);
    TEST_ASSERT_EQUAL(0, lot52_node("c")->has_bg);
    TEST_ASSERT_EQUAL(0, l61_diag_count("gradient painted in bands"));  /* one-stop-colour bands are exact */
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l62_gradient_refusals(void) {
    lot52_render(L62_BODY "<div id=a style='height:4px;background:linear-gradient(red, blue);"
                 "background:linear-gradient(red);background:linear-gradient(to top left, red, blue);"
                 "background:linear-gradient(45deg, red, blue);background:linear-gradient(red 10px, blue)'></div></body>", 320);
    TEST_ASSERT_EQUAL(4, l61_diag_count("gradient refused"));
    TEST_ASSERT_EQUAL(WE_GRAD_BOTTOM, lot52_node("a")->grad_dir);
    TEST_ASSERT_EQUAL(4, (int)g_doc.nitems);
    lot52_render(L62_BODY "<div id=a style='height:4px;background:linear-gradient(red, blue, red, blue, red);"
                 "background:linear-gradient(red, , blue);background:linear-gradient(red, blue 120%);"
                 "background-image:url(x.png)'></div></body>", 320);
    TEST_ASSERT_EQUAL(3, l61_diag_count("gradient refused"));
    TEST_ASSERT_EQUAL(1, l61_diag_count("background image refused"));
    TEST_ASSERT_EQUAL(0, lot52_node("a")->grad_dir);
    TEST_ASSERT_EQUAL(0, (int)g_doc.nitems);
}

static void lot62_gradient_run(void) {
    RUN_TEST(test_l62_gradient_vertical_bands);
    RUN_TEST(test_l62_gradient_directions_and_stops);
    RUN_TEST(test_l62_gradient_positions);
    RUN_TEST(test_l62_gradient_alpha_and_small_boxes);
    RUN_TEST(test_l62_gradient_cascade);
    RUN_TEST(test_l62_gradient_refusals);
}
