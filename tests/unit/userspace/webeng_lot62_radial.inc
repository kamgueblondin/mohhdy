/* Lot 62 round 2: radial-gradient as nested ellipse rings. */
static void test_l62_radial_circle_rings(void) {
    lot52_render(L62_BODY "<div style='width:30px;height:30px;background:radial-gradient(circle closest-side, #ff0000, #0000ff)'></div></body>", 320);
    TEST_ASSERT_EQUAL(16, (int)g_doc.nitems);
    l61_rect_at(0, 0, 30, 30, 0x0000FF);                  /* the last stop outside the shape */
    l61_rect_at(0, 10, 30, 10, 0x2B00D4);                 /* r 15: colour at 5/6 */
    l61_rect_at(5, 11, 20, 8, 0x800080);                  /* r 10: colour at 1/2 */
    l61_rect_at(10, 13, 10, 4, 0xD5002A);                 /* r 5: colour at 1/6 */
    L62_HEX(0x0000FF, l62_pixel(0, 0));
    L62_HEX(0xD5002A, l62_pixel(15, 15));
    TEST_ASSERT_EQUAL(1, l61_diag_count("gradient painted in bands"));
}

static void test_l62_radial_extents(void) {
    lot52_render(L62_BODY "<div style='width:40px;height:20px;background:radial-gradient(ellipse closest-side at center, #ffffff, #000000)'></div></body>", 320);
    l61_rect_at(0, 6, 40, 8, 0x2B2B2B);                   /* rx 20, ry 10 */
    l61_rect_at(9, 0, 22, 3, 0x2B2B2B);
    l61_rect_at(7, 8, 26, 4, 0x808080);
    L62_HEX(0x000000, l62_pixel(0, 0));
    lot52_render(L62_BODY "<div style='width:40px;height:20px;background:radial-gradient(circle farthest-side, #ffffff, #000000)'></div></body>", 320);
    l61_rect_at(2, 0, 36, 3, 0x2B2B2B);                   /* r 20, cut to the box */
    l61_rect_at(0, 3, 40, 14, 0x2B2B2B);
    lot52_render(L62_BODY "<div style='width:40px;height:20px;background:radial-gradient(circle, #ffffff, #000000)'></div></body>", 320);
    l61_rect_at(0, 0, 40, 20, 0x2B2B2B);                  /* farthest-corner: r 22 covers the box */
    l61_rect_at(8, 0, 24, 5, 0x808080);
    lot52_render(L62_BODY "<div style='width:40px;height:20px;background:radial-gradient(circle closest-corner, #ffffff, #000000)'></div></body>", 320);
    l61_rect_at(8, 0, 24, 5, 0x808080);
    lot52_render(L62_BODY "<div style='width:40px;height:20px;background:radial-gradient(#ffffff, #000000)'></div></body>", 320);
    l61_rect_at(0, 0, 40, 20, 0x2B2B2B);                  /* ellipse farthest-corner: rx 28, ry 14 */
    l61_rect_at(10, 1, 20, 3, 0x808080);
    lot52_render(L62_BODY "<div style='width:40px;height:20px;background:radial-gradient(circle closest-side, #ffffff, #000000)'></div></body>", 320);
    l61_rect_at(10, 6, 20, 8, 0x2B2B2B);                  /* r 10 from the short side */
    L62_HEX(0x000000, l62_pixel(5, 10));
}

static void test_l62_radial_origin_and_clip(void) {
    lot52_render(L62_BODY "<div style='width:20px;height:20px;padding:4px;background:radial-gradient(circle closest-side, #ff0000, #0000ff);"
                 "background-clip:content-box;background-origin:content-box'></div></body>", 320);
    l61_rect_at(4, 4, 20, 20, 0x0000FF);
    l61_rect_at(9, 4, 10, 3, 0x2B00D4);
    L62_HEX(0xFFFFFF, l62_pixel(2, 2));
    lot52_render(L62_BODY "<div style='width:20px;height:20px;background-color:#00ff00;background-image:radial-gradient(#ff0000, #0000ff)'></div></body>", 320);
    l61_rect_at(0, 0, 20, 20, 0x00FF00);                  /* the colour stays under the image */
    TEST_ASSERT_TRUE(l61_index(0x0000FF) > l61_index(0x00FF00));
    lot52_render(L62_BODY "<div style='width:20px;height:20px;background:radial-gradient(#ff0000, #ff0000)'></div></body>", 320);
    TEST_ASSERT_EQUAL(0, l61_diag_count("gradient painted in bands"));
}

static void test_l62_radial_refusals(void) {
    static const char *const bad[] = {
        "<div id=a style='background:radial-gradient(circle at middle, red, blue)'></div>",
        "<div id=a style='background:radial-gradient(circle red, blue)'></div>",
        "<div id=a style='background:radial-gradient(red)'></div>",
        "<div id=a style='background:radial-gradient(#ff000080, blue)'></div>",
        "<div id=a style='background:radial-gradient(circle at center center center, red, blue)'></div>",
        "<div id=a style='background:radial-gradient(ellipse at, red, blue)'></div>" };
    char page[256];
    int k;
    for (k = 0; k < 6; k++) {
        snprintf(page, sizeof(page), "%s%s</body>", L62_BODY, bad[k]);
        lot52_render(page, 320);
        TEST_ASSERT_EQUAL(1, l61_diag_count("gradient refused"));
        TEST_ASSERT_EQUAL(0, lot52_node("a")->grad_dir);
    }
    lot52_render(L62_BODY "<div id=a style='background:radial-gradient(farthest-side circle, red, blue 40%)'></div>"
                 "<div id=b style='background:radial-gradient(at center, red, blue)'></div></body>", 320);
    TEST_ASSERT_EQUAL(0, l61_diag_count("gradient refused"));
    TEST_ASSERT_EQUAL(WE_GRAD_RADIAL, lot52_node("a")->grad_dir);
    TEST_ASSERT_EQUAL(1 | (WE_EXT_FARTHEST_SIDE << 1), lot52_node("a")->grad_shape);
    TEST_ASSERT_EQUAL(4000, lot52_node("a")->grad_pos[1]);
    TEST_ASSERT_EQUAL(WE_EXT_FARTHEST_CORNER << 1, lot52_node("b")->grad_shape);
}

static void lot62_radial_run(void) {
    RUN_TEST(test_l62_radial_circle_rings);
    RUN_TEST(test_l62_radial_extents);
    RUN_TEST(test_l62_radial_origin_and_clip);
    RUN_TEST(test_l62_radial_refusals);
}
