/* Lot 62 round 2: overflow:hidden with border-radius clips to the rounded
 * padding box; double, groove, ridge, inset and outset borders and the
 * shadows of an opaque rounded box follow the radii. */
static void test_l62_round_clip_child_background(void) {
    lot52_render(L62_BODY "<div style='width:20px;height:20px;overflow:hidden;border-radius:10px'>"
                 "<div style='height:20px;background:#ff0000'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(5, (int)g_doc.nitems);              /* the child takes the circle bands */
    l61_rect_at(5, 0, 10, 3, 0xFF0000);
    l61_rect_at(2, 3, 16, 3, 0xFF0000);
    l61_rect_at(0, 6, 20, 8, 0xFF0000);
    l61_rect_at(2, 14, 16, 3, 0xFF0000);
    l61_rect_at(5, 17, 10, 3, 0xFF0000);
    L62_HEX(0xFFFFFF, l62_pixel(0, 0));
    L62_HEX(0xFF0000, l62_pixel(10, 10));
    /* without overflow hidden the child stays square */
    lot52_render(L62_BODY "<div style='width:20px;height:20px;border-radius:10px'>"
                 "<div style='height:20px;background:#ff0000'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(1, (int)g_doc.nitems);
    l61_rect_at(0, 0, 20, 20, 0xFF0000);
}

static void test_l62_round_clip_text_glyphs(void) {
    lot52_render(L62_BODY "<div style='width:24px;height:16px;overflow:hidden;border-radius:8px'>HHH</div></body>", 320);
    TEST_ASSERT_NOT_NULL(find_text("H"));
    TEST_ASSERT_EQUAL(8, find_text("H")->x);              /* the middle glyph stays text */
    TEST_ASSERT_EQUAL(13, (int)g_doc.nitems);
    L62_HEX(0xFFFFFF, l62_pixel(1, 1));                   /* row 0 inset 4: cut */
    L62_HEX(0x000000, l62_pixel(5, 1));
    L62_HEX(0x000000, l62_pixel(1, 2));                   /* row 1 inset 1: kept */
    L62_HEX(0xFFFFFF, l62_pixel(21, 1));
    L62_HEX(0x000000, l62_pixel(18, 1));
}

static void test_l62_round_clip_padding_box_and_nesting(void) {
    lot52_render(L62_BODY "<div style='width:24px;height:12px;overflow:hidden;border-radius:6px;border:2px solid #000000'>"
                 "<div style='height:12px;background:#0000ff'></div></div></body>", 320);
    l61_rect_at(4, 2, 20, 1, 0x0000FF);                   /* radii 6 - 2 on the padding box */
    l61_rect_at(3, 3, 22, 1, 0x0000FF);
    l61_rect_at(2, 4, 24, 8, 0x0000FF);
    lot52_render(L62_BODY "<div style='width:40px;height:20px;overflow:hidden;border-radius:4px'>"
                 "<div style='margin-left:20px;width:20px;height:20px;overflow:hidden;border-radius:10px'>"
                 "<div style='height:20px;background:#00ff00'></div></div></div></body>", 320);
    l61_rect_at(25, 0, 10, 3, 0x00FF00);                  /* the inner circle wins inside the outer */
    l61_rect_at(20, 6, 20, 8, 0x00FF00);
    L62_HEX(0xFFFFFF, l62_pixel(39, 0));
}

static void test_l62_round_clip_escapes(void) {
    lot52_render(L62_BODY "<div style='width:20px;height:20px;overflow:hidden;border-radius:10px'>"
                 "<div style='position:fixed;top:0;left:0;width:4px;height:4px;background:#ff0000'></div></div></body>", 320);
    l61_rect_at(0, 0, 4, 4, 0xFF0000);                    /* fixed boxes escape the clip */
    lot52_render(L62_BODY "<div style='width:20px;height:20px;overflow:hidden;border-radius:10px;transform:translateX(1px)'>"
                 "<div style='height:20px;background:#ff0000'></div></div></body>", 320);
    l61_rect_at(1, 0, 20, 20, 0xFF0000);                  /* square under a transform */
}

static void test_l62_round_double_and_3d_borders(void) {
    lot52_render(L62_BODY "<div style='width:20px;height:12px;border:3px double #000000;border-radius:8px'></div></body>", 320);
    TEST_ASSERT_EQUAL(28, (int)g_doc.nitems);             /* two rings of one pixel */
    l61_rect_at(5, 0, 16, 1, 0x000000);
    l61_rect_at(6, 2, 14, 1, 0x000000);
    l61_rect_at(0, 5, 1, 8, 0x000000);
    l61_rect_at(2, 6, 1, 6, 0x000000);
    L62_HEX(0xFFFFFF, l62_pixel(1, 8));                   /* the gap */
    TEST_ASSERT_EQUAL(0, l61_diag_count("rounded effect painted square"));
    lot52_render(L62_BODY "<div style='width:20px;height:12px;border:2px outset #808080;border-radius:6px'></div></body>", 320);
    TEST_ASSERT_EQUAL(12, (int)g_doc.nitems);
    l61_rect_at(3, 0, 18, 2, 0xAAAAAA);                   /* upper and left parts light */
    l61_rect_at(0, 4, 2, 8, 0xAAAAAA);
    l61_rect_at(1, 13, 3, 1, 0xAAAAAA);
    l61_rect_at(22, 4, 2, 8, 0x555555);                   /* right and lower parts dark */
    l61_rect_at(20, 2, 3, 1, 0x555555);
    l61_rect_at(3, 14, 18, 2, 0x555555);
    lot52_render(L62_BODY "<div style='width:20px;height:12px;border:4px groove #808080;border-radius:8px'></div></body>", 320);
    L62_HEX(0x555555, l62_pixel(1, 8));                   /* outer ring: dark then light */
    L62_HEX(0xAAAAAA, l62_pixel(3, 8));
    L62_HEX(0xAAAAAA, l62_pixel(26, 8));
    L62_HEX(0x555555, l62_pixel(24, 8));
    L62_HEX(0xFFFFFF, l62_pixel(0, 0));
    lot52_render(L62_BODY "<div style='width:20px;height:12px;border:4px inset #808080;border-radius:8px'></div></body>", 320);
    L62_HEX(0x555555, l62_pixel(1, 8));
    L62_HEX(0xAAAAAA, l62_pixel(26, 8));
    L62_HEX(0xAAAAAA, l62_pixel(14, 18));
}

static void test_l62_round_dotted_stays_square(void) {
    /* lot 63: rounded dotted borders follow the corners */
    lot52_render(L62_BODY "<div style='width:20px;height:12px;border:2px dotted #808080;border-radius:6px'></div></body>", 320);
    TEST_ASSERT_EQUAL(0, l61_diag_count("rounded effect painted square"));
    L62_HEX(0xFFFFFF, l62_pixel(0, 0));
}

static void test_l62_round_outer_shadow(void) {
    lot52_render(L62_BODY "<div style='margin:4px;width:20px;height:20px;background:#ffffff;border-radius:10px;box-shadow:2px 2px #000000'></div></body>", 320);
    TEST_ASSERT_EQUAL(10, (int)g_doc.nitems);             /* the whole shape under the background */
    l61_rect_at(11, 6, 10, 3, 0x000000);
    l61_rect_at(6, 12, 20, 8, 0x000000);
    L62_HEX(0xFFFFFF, l62_pixel(25, 25));                 /* outside the shadow circle */
    L62_HEX(0x000000, l62_pixel(25, 14));
    L62_HEX(0xFFFFFF, l62_pixel(20, 14));                 /* the background on top */
    TEST_ASSERT_EQUAL(0, l61_diag_count("rounded effect painted square"));
    /* spread grows the radii, blur paints two rounded layers */
    lot52_render(L62_BODY "<div style='margin:8px;width:20px;height:20px;background:#ffffff;border-radius:10px;"
                 "box-shadow:0 0 2px 2px rgba(0,0,0,0.5)'></div></body>", 320);
    l61_rect_at(5, 13, 26, 10, 0xBFBFBF);                 /* wide layer: radii 10 + 3, half alpha */
    l61_rect_at(11, 5, 14, 4, 0xBFBFBF);
    l61_rect_at(7, 14, 22, 8, 0x7F7F7F);                  /* narrow layer on top: radii 10 + 1 */
    L62_HEX(0xFFFFFF, l62_pixel(6, 6));
    TEST_ASSERT_EQUAL(0, l61_diag_count("rounded effect painted square"));
    L62_HEX(0xFFFFFF, l62_pixel(5, 5));
}

static void test_l62_round_inset_shadow(void) {
    lot52_render(L62_BODY "<div style='width:20px;height:20px;background:#00ff00;border-radius:6px;box-shadow:inset 3px 0 #000000'></div></body>", 320);
    TEST_ASSERT_EQUAL(15, (int)g_doc.nitems);
    l61_rect_at(6, 0, 11, 2, 0x00FF00);                   /* background repainted inside the shape */
    l61_rect_at(3, 4, 17, 12, 0x00FF00);
    L62_HEX(0x000000, l62_pixel(4, 1));                   /* the shadow follows the curve */
    L62_HEX(0x000000, l62_pixel(1, 8));
    L62_HEX(0x00FF00, l62_pixel(4, 8));
    L62_HEX(0xFFFFFF, l62_pixel(0, 0));
    L62_HEX(0x00FF00, l62_pixel(19, 8));
    L62_HEX(0xFFFFFF, l62_pixel(21, 8));                  /* the repaint stays inside the box */
    L62_HEX(0xFFFFFF, l62_pixel(18, 1));
}

static void test_l62_round_square_shadow_fallbacks(void) {
    /* no background: the outer shadow cannot be covered, it stays square */
    lot52_render(L62_BODY "<div style='margin:4px;width:20px;height:20px;border-radius:10px;box-shadow:2px 2px #000000'></div></body>", 320);
    TEST_ASSERT_EQUAL(1, l61_diag_count("rounded effect painted square"));
    L62_HEX(0x000000, l62_pixel(25, 25));
    /* a translucent or padding-box background, an inset shadow with blur
     * or over a content-box background */
    lot52_render(L62_BODY "<div style='margin:4px;width:20px;height:20px;background:#ff000080;border-radius:10px;box-shadow:2px 2px #000000'></div></body>", 320);
    TEST_ASSERT_EQUAL(1, l61_diag_count("rounded effect painted square"));
    lot52_render(L62_BODY "<div style='margin:4px;width:20px;height:20px;background:#ff0000;background-clip:padding-box;border-radius:10px;box-shadow:2px 2px #000000'></div></body>", 320);
    TEST_ASSERT_EQUAL(1, l61_diag_count("rounded effect painted square"));
    lot52_render(L62_BODY "<div style='width:20px;height:20px;background:#ffffff;border-radius:6px;box-shadow:inset 3px 0 2px #000000'></div></body>", 320);
    TEST_ASSERT_EQUAL(1, l61_diag_count("rounded effect painted square"));
    lot52_render(L62_BODY "<div style='width:20px;height:20px;background:#ffffff;background-clip:content-box;border-radius:6px;box-shadow:inset 3px 0 #000000'></div></body>", 320);
    TEST_ASSERT_EQUAL(1, l61_diag_count("rounded effect painted square"));
    lot52_render(L62_BODY "<div style='width:20px;height:20px;background-color:#ffffff;background-image:linear-gradient(red, blue);border-radius:6px;box-shadow:inset 3px 0 #000000'></div></body>", 320);
    TEST_ASSERT_EQUAL(1, l61_diag_count("rounded effect painted square"));
    /* a padding-box background still takes a rounded inset shadow */
    lot52_render(L62_BODY "<div style='width:20px;height:20px;background:#ffffff;background-clip:padding-box;border-radius:6px;box-shadow:inset 3px 0 #000000'></div></body>", 320);
    TEST_ASSERT_EQUAL(0, l61_diag_count("rounded effect painted square"));
}

static void lot62_round_run(void) {
    RUN_TEST(test_l62_round_clip_child_background);
    RUN_TEST(test_l62_round_clip_text_glyphs);
    RUN_TEST(test_l62_round_clip_padding_box_and_nesting);
    RUN_TEST(test_l62_round_clip_escapes);
    RUN_TEST(test_l62_round_double_and_3d_borders);
    RUN_TEST(test_l62_round_dotted_stays_square);
    RUN_TEST(test_l62_round_outer_shadow);
    RUN_TEST(test_l62_round_inset_shadow);
    RUN_TEST(test_l62_round_square_shadow_fallbacks);
}
