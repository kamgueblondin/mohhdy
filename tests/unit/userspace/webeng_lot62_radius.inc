/* Lot 62: border-radius as a staircase of row slices (3 rows per corner). */
static void test_l62_radius_background_slices(void) {
    lot52_render(L62_BODY "<div style='width:20px;height:20px;background:#ff0000;border-radius:6px'></div></body>", 320);
    TEST_ASSERT_EQUAL(5, (int)g_doc.nitems);
    l61_rect_at(3, 0, 14, 2, 0xFF0000);                   /* inset 6 - round(sqrt(11)) */
    l61_rect_at(1, 2, 18, 2, 0xFF0000);
    l61_rect_at(0, 4, 20, 12, 0xFF0000);                  /* the last corner row merges */
    l61_rect_at(1, 16, 18, 2, 0xFF0000);
    l61_rect_at(3, 18, 14, 2, 0xFF0000);
    TEST_ASSERT_EQUAL(1, l61_diag_count("border-radius painted in steps"));
}

static void test_l62_radius_percent_ellipse_and_scaling(void) {
    lot52_render(L62_BODY "<div style='width:20px;height:20px;background:#ff0000;border-radius:50%'></div>"
                 "<div style='width:20px;height:20px;background:#00ff00;border-radius:30px'></div>"
                 "<div style='width:20px;height:10px;background:#0000ff;border-radius:9px / 3px'></div>"
                 "<div style='width:20px;height:10px;background:#ff00ff;border-top-left-radius:6px 3px'></div></body>", 320);
    l61_rect_at(5, 0, 10, 3, 0xFF0000);
    l61_rect_at(2, 3, 16, 3, 0xFF0000);
    l61_rect_at(0, 6, 20, 8, 0xFF0000);
    l61_rect_at(5, 20, 10, 3, 0x00FF00);                  /* 30px scaled to 10 */
    l61_rect_at(0, 26, 20, 8, 0x00FF00);
    l61_rect_at(4, 40, 12, 1, 0x0000FF);
    l61_rect_at(1, 41, 18, 1, 0x0000FF);
    l61_rect_at(0, 42, 20, 6, 0x0000FF);
    l61_rect_at(3, 50, 17, 1, 0xFF00FF);                  /* one corner only */
    l61_rect_at(1, 51, 19, 1, 0xFF00FF);
    l61_rect_at(0, 52, 20, 8, 0xFF00FF);
    TEST_ASSERT_EQUAL(5 + 5 + 5 + 3, (int)g_doc.nitems);
}

static void test_l62_radius_solid_ring(void) {
    lot52_render(L62_BODY "<div style='width:20px;height:12px;border:2px solid #0000ff;border-radius:6px'></div></body>", 320);
    TEST_ASSERT_EQUAL(12, (int)g_doc.nitems);
    TEST_ASSERT_EQUAL(-1, l61_index(0xFFFFFF));
    l61_rect_at(3, 0, 18, 2, 0x0000FF);
    l61_rect_at(1, 2, 3, 1, 0x0000FF);
    l61_rect_at(20, 2, 3, 1, 0x0000FF);
    l61_rect_at(0, 4, 2, 8, 0x0000FF);
    l61_rect_at(22, 4, 2, 8, 0x0000FF);
    l61_rect_at(3, 14, 18, 2, 0x0000FF);
    L62_HEX(0xFFFFFF, l62_pixel(0, 0));                   /* outside the corner */
    L62_HEX(0xFFFFFF, l62_pixel(4, 2));                   /* inside the inner curve */
    L62_HEX(0x0000FF, l62_pixel(2, 3));
    /* lot 63: dashed borders follow the corners too */
    lot52_render(L62_BODY "<div style='width:20px;height:12px;border:2px dashed #0000ff;border-radius:6px'></div></body>", 320);
    TEST_ASSERT_EQUAL(0, l62_items_with(0xFFFFFF));
    TEST_ASSERT_EQUAL(0xFFFFFFFFu, l61_paint(0, 0));        /* nothing painted outside the corner */
}

static void test_l62_radius_clip_box_and_gradient(void) {
    lot52_render(L62_BODY "<div style='width:12px;height:12px;padding:2px;border:2px solid #000000;background:#00ffff;"
                 "background-clip:content-box;border-radius:8px'></div></body>", 320);
    l61_rect_at(6, 4, 8, 1, 0x00FFFF);                    /* radii 8 - 4 on the content box */
    l61_rect_at(5, 5, 10, 1, 0x00FFFF);
    l61_rect_at(4, 6, 12, 8, 0x00FFFF);
    l61_rect_at(6, 15, 8, 1, 0x00FFFF);
    lot52_render(L62_BODY "<div style='width:20px;height:8px;background:linear-gradient(red, blue);border-radius:4px'></div></body>", 320);
    TEST_ASSERT_EQUAL(1, l61_diag_count("gradient radius painted square"));
    TEST_ASSERT_EQUAL(8, (int)g_doc.nitems);
    TEST_ASSERT_EQUAL(0, g_doc.items[0].x);
}

static void test_l62_radius_refusals(void) {
    lot52_render(L62_BODY "<div id=a style='border-radius:4px;border-radius:-1px;border-radius:129px;border-radius:101%;"
                 "border-radius:1px 2px 3px 4px 5px;border-radius:1px/2px/3px;border-radius:4px /;"
                 "border-top-left-radius:1px 2px 3px'></div></body>", 320);
    TEST_ASSERT_EQUAL(7, l61_diag_count("border-radius refused"));
    TEST_ASSERT_EQUAL(4, lot52_node("a")->rad_x[2]);
    TEST_ASSERT_EQUAL(4, lot52_node("a")->rad_y[0]);
    lot52_render(L62_BODY "<div id=a style='border-radius:128px 100% 0 1px'></div></body>", 320);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
    TEST_ASSERT_EQUAL(128, lot52_node("a")->rad_x[0]);
    TEST_ASSERT_EQUAL(0x22, lot52_node("a")->rad_pct);
}

static void lot62_radius_run(void) {
    RUN_TEST(test_l62_radius_background_slices);
    RUN_TEST(test_l62_radius_percent_ellipse_and_scaling);
    RUN_TEST(test_l62_radius_solid_ring);
    RUN_TEST(test_l62_radius_clip_box_and_gradient);
    RUN_TEST(test_l62_radius_refusals);
}
