/* Lot 62: 3D border styles. Shades: dark keeps 2/3 of each channel, light
 * moves 1/3 of the way to white (#3090f0: dark #2060a0, light #75b5f5). */
#define L62_C 0x3090F0
#define L62_DARK 0x2060A0
#define L62_LIGHT 0x75B5F5

static void test_l62_border_inset_and_ridge_frames(void) {
    lot52_render(L62_BODY "<div style='border:3px inset #3090f0;width:4px;height:4px'></div>"
                 "<div style='border:4px ridge #3090f0;width:4px;height:4px'></div></body>", 320);
    L62_HEX(L62_DARK, l62_pixel(0, 0));         /* inset: top and left dark */
    L62_HEX(L62_DARK, l62_pixel(2, 2));
    L62_HEX(L62_DARK, l62_pixel(0, 5));
    L62_HEX(L62_LIGHT, l62_pixel(8, 5));        /* right and bottom light */
    L62_HEX(L62_LIGHT, l62_pixel(5, 8));
    L62_HEX(0xFFFFFF, l62_pixel(4, 4));         /* content untouched */
    /* ridge 4px at y 10: outer 2px light then 2px dark on the top-left */
    L62_HEX(L62_LIGHT, l62_pixel(1, 11));
    L62_HEX(L62_DARK, l62_pixel(3, 13));
    L62_HEX(L62_DARK, l62_pixel(11, 15));       /* outer right dark */
    L62_HEX(L62_LIGHT, l62_pixel(9, 15));       /* inner right light */
    L62_HEX(L62_LIGHT, l62_pixel(6, 18));       /* inner bottom light */
    L62_HEX(L62_DARK, l62_pixel(6, 21));        /* outer bottom dark */
    TEST_ASSERT_EQUAL(12, l61_count(OS_FB_WEB_RECT));
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l62_border_corners_follow_the_pinwheel(void) {
    lot52_render(L62_BODY "<div style='border:2px outset #3090f0;width:6px;height:6px'></div></body>", 320);
    L62_HEX(L62_LIGHT, l62_pixel(0, 0));        /* top-left corner: top side */
    L62_HEX(L62_DARK, l62_pixel(9, 0));         /* top-right corner: right side */
    L62_HEX(L62_DARK, l62_pixel(0, 9));         /* bottom-left corner: bottom side */
    L62_HEX(L62_DARK, l62_pixel(9, 9));
    l61_rect_at(0, 0, 8, 2, L62_LIGHT);
    l61_rect_at(0, 2, 2, 6, L62_LIGHT);
    l61_rect_at(8, 0, 2, 8, L62_DARK);
    l61_rect_at(0, 8, 10, 2, L62_DARK);
}

static void test_l62_border_thin_groove_and_small_box(void) {
    /* 1px groove: no outer half, the inner frame is the outset shade. */
    lot52_render(L62_BODY "<div style='border:1px groove #3090f0;width:4px;height:4px'></div>"
                 "<div style='border:8px inset #3090f0;width:0;height:0'></div></body>", 320);
    L62_HEX(L62_LIGHT, l62_pixel(0, 0));
    L62_HEX(L62_DARK, l62_pixel(5, 3));
    /* A 16x16 box of 8px sides: two halves, no overlap. */
    l61_rect_at(0, 6, 8, 8, L62_DARK);
    l61_rect_at(8, 6, 8, 8, L62_LIGHT);
    l61_rect_at(0, 14, 16, 8, L62_LIGHT);
    L62_HEX(L62_DARK, l62_pixel(3, 10));
}

static void test_l62_border_collapsed_3d_lines(void) {
    /* Collapsed: outset draws as groove, inset as ridge (CSS 2.1 17.6.2). */
    lot52_render(L62_BODY "<table style='width:40px;border-collapse:collapse'><tr>"
                 "<td style='border:2px outset #3090f0;height:6px'></td>"
                 "<td style='border:2px outset #3090f0;height:6px'></td></tr></table>"
                 "<table style='width:40px;border-collapse:collapse'><tr>"
                 "<td style='border:4px inset #3090f0;height:6px'></td></tr></table></body>", 320);
    L62_HEX(L62_DARK, l62_pixel(5, 0));         /* groove: top half dark */
    L62_HEX(L62_LIGHT, l62_pixel(5, 1));
    L62_HEX(L62_DARK, l62_pixel(19, 5));        /* shared line: left half dark */
    L62_HEX(L62_LIGHT, l62_pixel(20, 5));
    /* ridge 4px at y 10: the first two rows light, then dark */
    L62_HEX(L62_LIGHT, l62_pixel(10, 11));
    L62_HEX(L62_DARK, l62_pixel(10, 12));
    L62_HEX(L62_LIGHT, l62_pixel(1, 18));
    L62_HEX(L62_DARK, l62_pixel(2, 18));
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l62_border_3d_keeps_alpha(void) {
    lot52_render(L62_BODY "<div style='border:2px outset rgba(48,144,240,0.5);width:4px;height:4px'></div></body>", 320);
    L62_HEX(0xBADAFA, l62_pixel(0, 0));         /* light at alpha 128 over white */
    L62_HEX(0x8FAFCF, l62_pixel(7, 7));         /* dark at alpha 128 */
    l62_no_alpha_left();
}

static void lot62_border_run(void) {
    RUN_TEST(test_l62_border_inset_and_ridge_frames);
    RUN_TEST(test_l62_border_corners_follow_the_pinwheel);
    RUN_TEST(test_l62_border_thin_groove_and_small_box);
    RUN_TEST(test_l62_border_collapsed_3d_lines);
    RUN_TEST(test_l62_border_3d_keeps_alpha);
}
