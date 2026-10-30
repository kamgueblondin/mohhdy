/* Lot 62: outline and box-shadow. Neither changes layout. */
static void test_l62_outline_solid_and_offset(void) {
    lot52_render(L62_BODY "<div id=a style='margin:10px;width:10px;height:10px;outline:2px solid #ff0000'></div>"
                 "<div id=b style='height:4px'></div></body>", 320);
    l61_box("a", 10, 10, 10, 10);
    l61_box("b", 0, 30, 320, 4);
    {
        const os_fb_web_item_t *it = &g_doc.items[g_doc.nitems - 1];
        TEST_ASSERT_EQUAL(OS_FB_WEB_BORDER, it->kind);
        TEST_ASSERT_EQUAL(8, it->x); TEST_ASSERT_EQUAL(8, it->y);
        TEST_ASSERT_EQUAL(14, it->w); TEST_ASSERT_EQUAL(14, it->h);
        TEST_ASSERT_EQUAL(2, it->bold);
        TEST_ASSERT_EQUAL(0, it->pad);
    }
    lot52_render(L62_BODY "<div style='margin:10px;width:10px;height:10px;outline:thin solid;outline-offset:3px;color:#0000ff'></div></body>", 320);
    L62_HEX(0x0000FF, l62_pixel(6, 6));                  /* 10 - 3 - 1 */
    L62_HEX(0xFFFFFF, l62_pixel(7, 7));
    lot52_render(L62_BODY "<div style='margin:10px;width:10px;height:10px;outline:solid thick #00ff00;outline-offset:-3px'></div></body>", 320);
    L62_HEX(0x00FF00, l62_pixel(12, 12));                /* 10 + 3 - 5 = 8 .. 12 */
    L62_HEX(0x00FF00, l62_pixel(8, 8));
    L62_HEX(0xFFFFFF, l62_pixel(15, 15));
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l62_outline_longhands_and_styles(void) {
    lot52_render(L62_BODY "<div style='margin:10px;width:20px;height:10px;outline-style:dashed;outline-width:1px;outline-color:#ff0000'></div></body>", 320);
    TEST_ASSERT_EQUAL(0, l61_count(OS_FB_WEB_BORDER));
    l61_rect_at(9, 9, 3, 1, 0xFF0000);                    /* dashes of 3px */
    L62_HEX(0xFFFFFF, l62_pixel(12, 9));
    lot52_render(L62_BODY "<div style='margin:10px;width:10px;height:10px;outline:2px outset #3090f0'></div></body>", 320);
    L62_HEX(L62_LIGHT, l62_pixel(8, 8));
    L62_HEX(L62_DARK, l62_pixel(21, 21));
    lot52_render(L62_BODY "<div style='margin:10px;width:10px;height:10px;outline:2px solid red;outline-style:none'></div>"
                 "<div style='height:2px;outline:auto 1px'></div></body>", 320);
    TEST_ASSERT_EQUAL(1, (int)g_doc.nitems);             /* only the auto (solid) one */
    TEST_ASSERT_EQUAL(OS_FB_WEB_BORDER, g_doc.items[0].kind);
    TEST_ASSERT_EQUAL(29, g_doc.items[0].y);
}

static void test_l62_outline_refusals(void) {
    lot52_render(L62_BODY "<div id=a style='outline:2px hidden'></div><div style='outline:1px solid red blue'></div>"
                 "<div style='outline-width:5%'></div><div style='outline-offset:10%'></div>"
                 "<div style='outline-style:hidden'></div><div style='outline:1px solid red 2px'></div>"
                 "<div style='outline-width:4'></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->outline_style);
    TEST_ASSERT_EQUAL(7, l61_diag_count("outline refused"));
}

static void test_l62_outline_paints_last_in_its_context(void) {
    /* Appendix E step 10: above a z-index 1 box of the same context. */
    lot52_render(L62_BODY "<div style='margin:4px;width:10px;height:10px;outline:2px solid #ff0000'></div>"
                 "<div style='position:relative;z-index:1;margin-top:-18px;width:20px;height:20px;background:#0000ff'></div></body>", 320);
    L62_HEX(0xFF0000, l62_pixel(3, 3));
    L62_HEX(0x0000FF, l62_pixel(8, 8));
    /* Inside a context, it stays inside: a later z 2 box covers it. */
    lot52_render(L62_BODY "<div style='position:relative;z-index:1'><div style='margin:4px;width:10px;height:10px;outline:2px solid #ff0000'></div></div>"
                 "<div style='position:relative;z-index:2;margin-top:-18px;width:20px;height:20px;background:#0000ff'></div></body>", 320);
    L62_HEX(0x0000FF, l62_pixel(3, 3));
}

static void test_l62_shadow_outer_offset_and_spread(void) {
    lot52_render(L62_BODY "<div id=a style='width:10px;height:10px;background:#ffff00;box-shadow:4px 4px #000000'></div>"
                 "<div id=b style='height:2px'></div></body>", 320);
    l61_box("b", 0, 10, 320, 2);
    l61_rect_at(4, 10, 10, 4, 0x000000);
    l61_rect_at(10, 4, 4, 6, 0x000000);
    TEST_ASSERT_TRUE(l61_index(0x000000) < l61_index(0xFFFF00));   /* under the background */
    L62_HEX(0xFFFF00, l62_pixel(8, 8));
    lot52_render(L62_BODY "<div style='margin:4px;width:10px;height:10px;box-shadow:0 0 0 2px #ff0000'></div></body>", 320);
    L62_HEX(0xFF0000, l62_pixel(2, 2));
    L62_HEX(0xFF0000, l62_pixel(15, 15));
    L62_HEX(0xFFFFFF, l62_pixel(8, 8));                  /* no shadow under the box */
    TEST_ASSERT_EQUAL(4, (int)g_doc.nitems);
    lot52_render(L62_BODY "<div style='margin:4px;width:10px;height:10px;box-shadow:0 0 0 -5px red'></div></body>", 320);
    TEST_ASSERT_EQUAL(0, (int)g_doc.nitems);             /* unused reserved items removed */
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l62_shadow_blur_layers(void) {
    lot52_render(L62_BODY "<div style='margin:8px;width:10px;height:10px;box-shadow:0 0 4px #ff0000'></div></body>", 320);
    L62_HEX(0xFF8080, l62_pixel(7, 12));                 /* 2px band: half alpha */
    L62_HEX(0xFF8080, l62_pixel(6, 12));
    L62_HEX(0xFFFFFF, l62_pixel(5, 12));
    lot52_render(L62_BODY "<div style='margin:8px;width:10px;height:10px;box-shadow:0 0 4px 3px #ff0000'></div></body>", 320);
    L62_HEX(0xFF0000, l62_pixel(7, 12));                 /* narrow layer (spread - 2): full alpha */
    L62_HEX(0xFF8080, l62_pixel(6, 12));                 /* wide layer (spread + 2) only */
    L62_HEX(0xFF8080, l62_pixel(3, 12));
    L62_HEX(0xFFFFFF, l62_pixel(2, 12));
    l62_no_alpha_left();
}

static void test_l62_shadow_inset_over_background(void) {
    lot52_render(L62_BODY "<div style='width:10px;height:10px;background:#ffff00;box-shadow:inset 2px 2px #0000ff'>"
                 "<div style='width:4px;height:4px;background:#00ff00'></div></div></body>", 320);
    l61_rect_at(0, 0, 10, 2, 0x0000FF);
    l61_rect_at(0, 2, 2, 8, 0x0000FF);
    TEST_ASSERT_TRUE(l61_index(0xFFFF00) < l61_index(0x0000FF));
    TEST_ASSERT_TRUE(l61_index(0x0000FF) < l61_index(0x00FF00));   /* under the content */
    L62_HEX(0x00FF00, l62_pixel(1, 1));
    L62_HEX(0xFFFF00, l62_pixel(5, 5));
    /* inset with a border: the padding box is shadowed, the border is not */
    lot52_render(L62_BODY "<div style='width:10px;height:10px;border:2px solid #000000;box-shadow:inset 0 0 0 1px #ff0000'></div></body>", 320);
    L62_HEX(0x000000, l62_pixel(1, 1));
    L62_HEX(0xFF0000, l62_pixel(2, 2));
    L62_HEX(0xFF0000, l62_pixel(11, 6));
    L62_HEX(0xFFFFFF, l62_pixel(3, 3));
}

static void test_l62_shadow_inset_blur(void) {
    lot52_render(L62_BODY "<div style='width:20px;height:20px;box-shadow:inset 0 0 4px 4px #0000ff'></div></body>", 320);
    L62_HEX(0x0000FF, l62_pixel(0, 10));                 /* both layers (shrunk 4 - 2) */
    L62_HEX(0x0000FF, l62_pixel(1, 10));
    L62_HEX(0x8080FF, l62_pixel(2, 10));                 /* the wide layer only (4 + 2) */
    L62_HEX(0x8080FF, l62_pixel(5, 10));
    L62_HEX(0xFFFFFF, l62_pixel(6, 10));
    L62_HEX(0x8080FF, l62_pixel(10, 16));
    L62_HEX(0x0000FF, l62_pixel(10, 18));
}

static void test_l62_shadow_list_order_and_colour(void) {
    lot52_render(L62_BODY "<div style='width:10px;height:10px;box-shadow:2px 0 #ff0000, 4px 0 #0000ff'></div></body>", 320);
    L62_HEX(0xFF0000, l62_pixel(11, 5));                 /* the first shadow is on top */
    L62_HEX(0x0000FF, l62_pixel(13, 5));
    lot52_render(L62_BODY "<div style='width:10px;height:10px;color:#00ff00;box-shadow:inset 1px 1px'></div>"
                 "<div style='width:10px;height:10px;box-shadow:#ff0000 2px 2px'></div>"
                 "<div style='width:10px;height:10px;box-shadow:2px 2px rgba(0,0,255,.5)'></div></body>", 320);
    L62_HEX(0x00FF00, l62_pixel(0, 5));                  /* currentColor */
    L62_HEX(0xFF0000, l62_pixel(11, 15));                /* colour first */
    L62_HEX(0x7F7FFF, l62_pixel(11, 31));
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l62_shadow_refusals(void) {
    lot52_render(L62_BODY "<div id=a style='box-shadow:1px 1px red;box-shadow:1px'></div>"
                 "<div style='box-shadow:1px 2px -3px'></div><div style='box-shadow:1px 2px 3px 4px 5px'></div>"
                 "<div style='box-shadow:1px 1px, 2px 2px, 3px 3px'></div><div style='box-shadow:red 1px 2px red'></div>"
                 "<div style='box-shadow:1px red 2px'></div><div style='box-shadow:inset 1px 1px inset'></div>"
                 "<div style='box-shadow:1px inset 2px'></div><div style='box-shadow:1% 2px'></div></body>", 320);
    TEST_ASSERT_EQUAL(1, lot52_node("a")->shadow_count);
    TEST_ASSERT_EQUAL(1, lot52_node("a")->shadow[0].x);
    TEST_ASSERT_EQUAL(8, l61_diag_count("box-shadow refused"));
    lot52_render(L62_BODY "<div id=a style='box-shadow:1px 1px red;box-shadow:none'></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->shadow_count);
}

static void lot62_decor_run(void) {
    RUN_TEST(test_l62_outline_solid_and_offset);
    RUN_TEST(test_l62_outline_longhands_and_styles);
    RUN_TEST(test_l62_outline_refusals);
    RUN_TEST(test_l62_outline_paints_last_in_its_context);
    RUN_TEST(test_l62_shadow_outer_offset_and_spread);
    RUN_TEST(test_l62_shadow_blur_layers);
    RUN_TEST(test_l62_shadow_inset_over_background);
    RUN_TEST(test_l62_shadow_inset_blur);
    RUN_TEST(test_l62_shadow_list_order_and_colour);
    RUN_TEST(test_l62_shadow_refusals);
}
