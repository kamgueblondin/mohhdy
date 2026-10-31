/* Lot 64 E: background-repeat space / round, background layers (comma
 * lists) and the background shorthand with images. mire-ga16.png is the
 * 48x32 grey ramp of lot 63 (l63_ramp at 6 px). */
#define L64_GA "url(/images/mire-ga16.png)"
#define L64_G1 "url(/images/mire-g1.png)"
#define L64_BOX(style) "<body style='margin:0'><div id=b style='" style "'></div></body>"

static void test_l64_bg_repeat_space(void) {
    int i;
    /* 20 px hold 3 tiles of 6: 1 px gaps, the last tile at the right edge */
    l63_bgi(L64_BOX("width:20px;height:1px;background-image:" L64_GA ";background-size:6px 1px;background-repeat:space no-repeat"));
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
    TEST_ASSERT_EQUAL(WE_BGR_SPACE, lot52_node("b")->bgi_space);
    for (i = 0; i < 6; i++) {
        TEST_ASSERT_EQUAL((long)l63_ramp[i], l63_at(i, 0));
        TEST_ASSERT_EQUAL((long)l63_ramp[i], l63_at(7 + i, 0));
        TEST_ASSERT_EQUAL((long)l63_ramp[i], l63_at(14 + i, 0));
    }
    TEST_ASSERT_EQUAL(-1, l63_at(6, 0));
    TEST_ASSERT_EQUAL(-1, l63_at(13, 0));
    TEST_ASSERT_EQUAL(18, (int)g_doc.nitems);
    /* space ignores the position */
    l63_bgi(L64_BOX("width:20px;height:1px;background-image:" L64_GA ";background-size:6px 1px;background-repeat:space no-repeat;background-position:3px 0"));
    TEST_ASSERT_EQUAL((long)l63_ramp[0], l63_at(0, 0));
    /* one tile only: placed by the position, not repeated */
    l63_bgi(L64_BOX("width:10px;height:1px;background-image:" L64_GA ";background-size:6px 1px;background-repeat:space no-repeat;background-position:right top"));
    TEST_ASSERT_EQUAL(-1, l63_at(3, 0));
    TEST_ASSERT_EQUAL((long)l63_ramp[0], l63_at(4, 0));
    TEST_ASSERT_EQUAL((long)l63_ramp[5], l63_at(9, 0));
    TEST_ASSERT_EQUAL(6, (int)g_doc.nitems);
    /* both axes: 9 rows hold 2 tiles of 4 with a 1 px gap */
    l63_bgi(L64_BOX("width:6px;height:9px;background-image:" L64_GA ";background-size:6px 4px;background-repeat:space"));
    TEST_ASSERT_EQUAL(WE_BGR_SPACE | WE_BGR_SPACE << 2, lot52_node("b")->bgi_space);
    TEST_ASSERT_EQUAL((long)l63_ramp[2], l63_at(2, 3));
    TEST_ASSERT_EQUAL(-1, l63_at(2, 4));
    TEST_ASSERT_EQUAL((long)l63_ramp[2], l63_at(2, 5));
    TEST_ASSERT_EQUAL((long)l63_ramp[2], l63_at(2, 8));
}

static void test_l64_bg_repeat_round(void) {
    int i;
    /* 10 px: round(10 / 6) = 2 tiles of 5 */
    l63_bgi(L64_BOX("width:10px;height:4px;background-image:" L64_GA ";background-size:6px 4px;background-repeat:round no-repeat"));
    TEST_ASSERT_EQUAL(WE_BGR_ROUND, lot52_node("b")->bgi_space);
    TEST_ASSERT_EQUAL(10, (int)g_doc.nitems);
    for (i = 0; i < 5; i++) TEST_ASSERT_EQUAL(l63_at(i, 0), l63_at(5 + i, 0));
    TEST_ASSERT_EQUAL(0x2D2D2D, l63_at(1, 0));
    TEST_ASSERT_EQUAL(0xBEBEBE, l63_at(4, 3));                 /* the explicit height is kept */
    TEST_ASSERT_EQUAL(-1, l63_at(0, 4));
    /* an auto height follows the ratio: 4 x 5 / 6 = 3 */
    l63_bgi(L64_BOX("width:10px;height:6px;background-image:" L64_GA ";background-size:6px auto;background-repeat:round no-repeat"));
    TEST_ASSERT_EQUAL(0, l63_at(0, 2));
    TEST_ASSERT_EQUAL(-1, l63_at(0, 3));
    /* round on both axes keeps both fits: 13 / 6 -> 2 of 6, 9 / 4 -> 2 of 4 */
    l63_bgi(L64_BOX("width:13px;height:9px;background-image:" L64_GA ";background-size:6px 4px;background-repeat:round"));
    TEST_ASSERT_EQUAL(WE_BGR_ROUND | WE_BGR_ROUND << 2, lot52_node("b")->bgi_space);
    TEST_ASSERT_EQUAL((long)l63_ramp[0], l63_at(6, 0));
    TEST_ASSERT_EQUAL((long)l63_ramp[0], l63_at(12, 8));       /* the remainder starts a new tile */
    /* a tile larger than the area rounds to one tile of the area */
    l63_bgi(L64_BOX("width:4px;height:1px;background-image:" L64_GA ";background-size:6px 1px;background-repeat:round no-repeat"));
    TEST_ASSERT_EQUAL(4, (int)g_doc.nitems);
    TEST_ASSERT_EQUAL(0, l63_at(0, 0));
    /* the per-axis pairs, and repeat-x keeps the plain style */
    l63_bgi(L64_BOX("background-repeat:round space"));
    TEST_ASSERT_EQUAL(WE_BGR_ROUND | WE_BGR_SPACE << 2, lot52_node("b")->bgi_space);
    TEST_ASSERT_EQUAL(3, lot52_node("b")->bgi_repeat);
    l63_bgi(L64_BOX("background-repeat:no-repeat space"));
    TEST_ASSERT_EQUAL(WE_BGR_SPACE << 2, lot52_node("b")->bgi_space);
    TEST_ASSERT_EQUAL(2, lot52_node("b")->bgi_repeat);
    l63_bgi(L64_BOX("background-repeat:space;background-repeat:repeat-x"));
    TEST_ASSERT_EQUAL(0, lot52_node("b")->bgi_space);
    TEST_ASSERT_EQUAL(1, lot52_node("b")->bgi_repeat);
}

static void test_l64_bg_layers(void) {
    const we_node_t *b;
    /* the first layer paints on top of the second */
    l63_bgi(L64_BOX("width:12px;height:2px;background-image:" L64_GA ", " L64_G1 ";background-size:6px 1px, 12px 2px;"
                    "background-repeat:no-repeat, repeat;background-position:right top, 0 0"));
    b = lot52_node("b");
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
    TEST_ASSERT_EQUAL(2, lot52_read_calls);
    TEST_ASSERT_EQUAL(1, b->bgi_layers);
    TEST_ASSERT_EQUAL(1 << 0 | 1 << 1, b->bgi_more[0].repeat);
    TEST_ASSERT_EQUAL(0, b->bgi_repeat);
    TEST_ASSERT_EQUAL(12, b->bgi_more[0].size[0]);
    TEST_ASSERT_EQUAL((long)l63_ramp[0], l63_at(6, 0));
    TEST_ASSERT_EQUAL((long)l63_ramp[5], l63_at(11, 0));
    TEST_ASSERT_EQUAL(0, l63_at(0, 0) & 0xFF000000);
    TEST_ASSERT_TRUE(l63_at(0, 0) >= 0);
    TEST_ASSERT_TRUE(l63_at(6, 1) >= 0);                      /* the lower layer shows below the top tile */
    TEST_ASSERT_EQUAL(1, b->bgi_more[0].len > 0);
    /* a short list repeats; one value sets every layer */
    l63_bgi(L64_BOX("background-image:" L64_GA ", " L64_G1 ", " L64_GA ";background-size:1px 2px, 3px 4px;background-repeat:space"));
    b = lot52_node("b");
    TEST_ASSERT_EQUAL(2, b->bgi_layers);
    TEST_ASSERT_EQUAL(1, b->bgi_size[0]);
    TEST_ASSERT_EQUAL(3, b->bgi_more[0].size[0]);
    TEST_ASSERT_EQUAL(1, b->bgi_more[1].size[0]);
    TEST_ASSERT_EQUAL(WE_BGR_SPACE | WE_BGR_SPACE << 2, b->bgi_more[1].space);
    /* none layers keep their place in the lists; a gradient is the lowest */
    l63_bgi(L64_BOX("width:6px;height:2px;background-image:none, " L64_GA ", linear-gradient(to right, #ff0000, #0000ff);"
                    "background-size:auto, 6px 1px;background-repeat:repeat, no-repeat"));
    b = lot52_node("b");
    TEST_ASSERT_EQUAL(1, b->bgi_layers);
    TEST_ASSERT_EQUAL(0, b->bgi_len);
    TEST_ASSERT_EQUAL(WE_GRAD_RIGHT, b->grad_dir);
    TEST_ASSERT_EQUAL((long)l63_ramp[3], l63_at(3, 0));
    TEST_ASSERT_TRUE(l63_at(0, 1) > 0xC00000);                /* the gradient under row 1 */
    TEST_ASSERT_EQUAL(1, lot52_read_calls);
    /* one failing layer leaves the others */
    l63_bgi(L64_BOX("width:6px;height:1px;background-image:url(/images/missing.png), " L64_GA ";background-size:6px 1px"));
    TEST_ASSERT_TRUE(lot52_has_diag("image read failed"));
    TEST_ASSERT_EQUAL((long)l63_ramp[2], l63_at(2, 0));
    TEST_ASSERT_EQUAL(2, lot52_read_calls);
    l63_bgi(L64_BOX("width:6px;height:1px;background-image:" L64_GA ", url(/images/missing.png);background-size:6px 1px"));
    TEST_ASSERT_EQUAL((long)l63_ramp[2], l63_at(2, 0));
    TEST_ASSERT_EQUAL(1, l61_diag_count("image read failed"));
    /* a later background-image without a list drops the layers */
    l63_bgi(L64_BOX("background-image:" L64_GA ", " L64_G1 ";background-image:" L64_GA));
    TEST_ASSERT_EQUAL(0, lot52_node("b")->bgi_layers);
    l63_bgi(L64_BOX("background-image:" L64_GA ", " L64_G1 ";background-image:linear-gradient(red, blue)"));
    TEST_ASSERT_EQUAL(0, lot52_node("b")->bgi_layers);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->bgi_len);
    l63_bgi(L64_BOX("background-image:" L64_GA ", " L64_G1 ";background:#ff0000"));
    TEST_ASSERT_EQUAL(0, lot52_node("b")->bgi_layers);
}

static void test_l64_bg_layer_refusals(void) {
    static const char *const bad_image[] = {
        "linear-gradient(red, blue), " L64_GA, L64_GA ", " L64_GA ", " L64_GA ", " L64_GA ", " L64_GA,
        L64_GA ", red", L64_GA ",", ", " L64_GA, L64_GA ", url()", 0};
    static const char *const bad_list[] = {"repeat-x, bogus", "repeat,", "space space space", 0};
    char html[400];
    int k;
    for (k = 0; bad_image[k]; k++) {
        snprintf(html, sizeof(html), "<body style='margin:0'><div id=b style='background-image:" L64_G1 ", " L64_GA ";background-image:%s'></div></body>", bad_image[k]);
        l63_bgi(html);
        TEST_ASSERT_EQUAL(1, l61_diag_count("background image refused") + l61_diag_count("gradient refused"));
        TEST_ASSERT_EQUAL(1, lot52_node("b")->bgi_layers);    /* the earlier list is kept */
    }
    for (k = 0; bad_list[k]; k++) {
        snprintf(html, sizeof(html), "<body style='margin:0'><div id=b style='background-repeat:%s'></div></body>", bad_list[k]);
        l63_bgi(html);
        TEST_ASSERT_EQUAL(1, l61_diag_count("background repeat refused"));
        TEST_ASSERT_EQUAL(3, lot52_node("b")->bgi_repeat);
        TEST_ASSERT_EQUAL(3, lot52_node("b")->bgi_more[0].repeat);
    }
    l63_bgi(L64_BOX("background-position:left top, left left"));
    TEST_ASSERT_EQUAL(1, l61_diag_count("background position refused"));
    TEST_ASSERT_EQUAL(3, lot52_node("b")->bgi_pos_pct);
    l63_bgi(L64_BOX("background-size:4px, cover contain"));
    TEST_ASSERT_EQUAL(1, l61_diag_count("background size refused"));
    TEST_ASSERT_EQUAL(-1, lot52_node("b")->bgi_size[0]);
    /* a vertical flow drops every layer */
    l63_bgi("<body style='margin:0'><div style='writing-mode:vertical-rl;height:10px'><div id=b style='width:4px;background-image:"
            L64_GA ", " L64_G1 "'></div></div></body>");
    TEST_ASSERT_EQUAL(0, lot52_node("b")->bgi_layers);
    TEST_ASSERT_EQUAL(0, lot52_read_calls);
}

static void test_l64_bg_shorthand(void) {
    static const char *const bad[] = {L64_GA " border-box", L64_GA " / 4px", L64_GA " 0 0 / 4px 1px repeat 2px", "red blue",
        L64_GA " " L64_GA, L64_GA " 0 0 contain", "red, " L64_GA, L64_GA " 0 0 / / 4px", L64_GA " left repeat top",
        "linear-gradient(red, blue), " L64_GA, L64_GA " fixed", 0};
    const we_node_t *b;
    char html[400];
    int k;
    l63_bgi(L64_BOX("width:8px;height:3px;background:" L64_GA " no-repeat right top / 4px 1px #00ff00"));
    b = lot52_node("b");
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
    TEST_ASSERT_EQUAL(1, b->has_bg);
    TEST_ASSERT_EQUAL(0x00FF00, b->bg);
    TEST_ASSERT_EQUAL(0, b->bgi_repeat);
    TEST_ASSERT_EQUAL(4, b->bgi_size[0]);
    TEST_ASSERT_EQUAL(0x00FF00, l63_at(0, 0));
    TEST_ASSERT_EQUAL(0x00FF00, l63_at(4, 1));
    TEST_ASSERT_EQUAL(0, l63_at(4, 0));
    /* layers, the colour in the last one */
    l63_bgi(L64_BOX("width:8px;height:3px;background:" L64_GA " no-repeat 0 0 / 6px 1px, " L64_G1 " repeat-x 0 1px / 8px 1px, #ff0000"));
    b = lot52_node("b");
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
    TEST_ASSERT_EQUAL(2, b->bgi_layers);                      /* the colour layer is an image-less layer */
    TEST_ASSERT_EQUAL(0, b->bgi_more[1].len);
    TEST_ASSERT_EQUAL(1, b->bgi_more[0].repeat);
    TEST_ASSERT_EQUAL(0xFF0000, l63_at(0, 2));
    TEST_ASSERT_EQUAL((long)l63_ramp[1], l63_at(1, 0));
    TEST_ASSERT_EQUAL(0xFF0000, l63_at(7, 0));
    TEST_ASSERT_TRUE(l63_at(0, 1) != 0xFF0000);
    /* a gradient as the lowest layer, any order inside a layer */
    l63_bgi(L64_BOX("width:6px;height:2px;background:" L64_GA " 0 0 / 6px 1px no-repeat, linear-gradient(#ff0000, #0000ff)"));
    b = lot52_node("b");
    TEST_ASSERT_EQUAL(WE_GRAD_BOTTOM, b->grad_dir);
    TEST_ASSERT_EQUAL(0, b->bgi_layers);
    TEST_ASSERT_EQUAL((long)l63_ramp[4], l63_at(4, 0));
    TEST_ASSERT_TRUE(l63_at(4, 1) != (long)l63_ramp[4]);
    l63_bgi(L64_BOX("width:6px;height:1px;background:#0000ff " L64_GA " space"));
    TEST_ASSERT_EQUAL(0x0000FF, lot52_node("b")->bg);
    TEST_ASSERT_EQUAL(WE_BGR_SPACE | WE_BGR_SPACE << 2, lot52_node("b")->bgi_space);
    /* every property returns to its initial value */
    l63_bgi(L64_BOX("background-repeat:no-repeat;background-size:4px;background-color:#ff0000;background:" L64_GA));
    b = lot52_node("b");
    TEST_ASSERT_EQUAL(3, b->bgi_repeat);
    TEST_ASSERT_EQUAL(WE_BGS_AUTO, b->bgi_size_kind);
    TEST_ASSERT_EQUAL(0, b->has_bg);
    l63_bgi(L64_BOX("background:none transparent"));
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->has_bg);
    for (k = 0; bad[k]; k++) {
        snprintf(html, sizeof(html), "<body style='margin:0'><div id=b style='background:#00ff00;background:%s'></div></body>", bad[k]);
        l63_bgi(html);
        TEST_ASSERT_EQUAL(1, l61_diag_count("background refused"));
        TEST_ASSERT_EQUAL(0x00FF00, lot52_node("b")->bg);      /* the declaration is dropped whole */
        TEST_ASSERT_EQUAL(0, lot52_node("b")->bgi_len);
    }
    /* single-token values keep the lot 62 paths */
    l63_bgi(L64_BOX("background:rgb(1, 2, 3)"));
    TEST_ASSERT_EQUAL(0x010203, lot52_node("b")->bg);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void lot64_bglayers_run(void) {
    RUN_TEST(test_l64_bg_repeat_space);
    RUN_TEST(test_l64_bg_repeat_round);
    RUN_TEST(test_l64_bg_layers);
    RUN_TEST(test_l64_bg_layer_refusals);
    RUN_TEST(test_l64_bg_shorthand);
}
