/* Lot 85: the border cap is 64 px, not 8. Layout inset and the painted
 * thickness both follow the declared width. A length above 64 is still
 * refused and the previous width stays. */
static int lot85_border_bold(void) {
    int i, bold = -1;
    for (i = 0; i < (int)g_doc.nitems; i++)
        if (g_doc.items[i].kind == OS_FB_WEB_BORDER) bold = (int)g_doc.items[i].bold;
    return bold;
}

static void test_l85_border_9px_is_not_capped_at_8(void) {
    lot52_render("<body style='margin:0'><div style='width:300px'><div id=a style='border:9px solid #000'><div id=b style='height:4px'></div></div></div></body>", 480);
    TEST_ASSERT_EQUAL(9, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(9, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(282, lot52_node("b")->w);   /* 300 - 9 - 9 */
    TEST_ASSERT_EQUAL(9, lot85_border_bold());
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l85_border_widths_up_to_64(void) {
    static const int widths[] = {1, 8, 9, 12, 16, 20, 24, 32, 40, 48, 56, 64};
    int i;
    for (i = 0; i < (int)(sizeof(widths) / sizeof(widths[0])); i++) {
        char html[320];
        int w = widths[i];
        sprintf(html,
                "<body style='margin:0'><div style='width:400px'><div id=a style='border:%dpx solid #111'><div id=b style='height:2px'></div></div></div></body>",
                w);
        lot52_render(html, 480);
        TEST_ASSERT_EQUAL(w, lot52_node("b")->x);
        TEST_ASSERT_EQUAL(w, lot52_node("b")->y);
        TEST_ASSERT_EQUAL(400 - 2 * w, lot52_node("b")->w);
        TEST_ASSERT_EQUAL(w, lot85_border_bold());
        TEST_ASSERT_EQUAL(0, g_doc.ndiag);
    }
}

static void test_l85_border_65px_is_refused(void) {
    lot52_render("<body style='margin:0'><div style='width:200px'><div id=a style='border-left-width:12px;border-left-width:65px;border-style:solid'><div id=b style='height:2px'></div></div></div></body>", 480);
    TEST_ASSERT_EQUAL(12, lot52_node("b")->x);
    TEST_ASSERT_TRUE(lot52_has_diag("border edge refused"));
}

static void test_l85_border_per_side(void) {
    lot52_render("<body style='margin:0'><div style='width:300px'><div id=a style='border-left:32px solid #000;border-right:16px solid #000;border-top:20px solid #000;border-bottom:8px solid #000'><div id=b style='height:6px'></div></div></div></body>", 480);
    TEST_ASSERT_EQUAL(32, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(20, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(300 - 32 - 16, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l85_border_percent_min_is_not_8(void) {
    /* 50% of 200 is 100, min with 40px is 40. The old cap would store 8. */
    lot52_render("<body style='margin:0'><div style='width:200px'><div id=a style='border-left:min(50%, 40px) solid'><div id=b style='height:2px'></div></div></div></body>", 480);
    TEST_ASSERT_EQUAL(40, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(160, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l85_border_percent_caps_at_64(void) {
    /* 50% of 200 is 100, which is above 64, so the used width is 64. */
    lot52_render("<body style='margin:0'><div style='width:200px'><div id=a style='border-left:50% solid'><div id=b style='height:2px'></div></div></div></body>", 480);
    TEST_ASSERT_EQUAL(64, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(136, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l85_border_zero_stays_zero(void) {
    lot52_render("<body style='margin:0'><div style='width:120px'><div id=a style='border:0 solid #000'><div id=b style='height:3px'></div></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(120, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(-1, lot85_border_bold());
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l85_border_shorthand_four_sides(void) {
    lot52_render("<body style='margin:0'><div style='width:280px'><div id=a style='border-width:10px 24px 6px 18px;border-style:solid'><div id=b style='height:4px'></div></div></div></body>", 480);
    /* top right bottom left */
    TEST_ASSERT_EQUAL(18, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(280 - 18 - 24, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l85_outline_uses_the_same_cap(void) {
    const we_node_t *a;
    lot52_render("<body style='margin:0'><div id=a style='outline:30px solid #000;width:40px;height:10px'></div></body>", 480);
    a = lot52_node("a");
    TEST_ASSERT_TRUE(a != 0);
    TEST_ASSERT_EQUAL(30, a->outline_w);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void lot85_border_run(void) {
    RUN_TEST(test_l85_border_9px_is_not_capped_at_8);
    RUN_TEST(test_l85_border_widths_up_to_64);
    RUN_TEST(test_l85_border_65px_is_refused);
    RUN_TEST(test_l85_border_per_side);
    RUN_TEST(test_l85_border_percent_min_is_not_8);
    RUN_TEST(test_l85_border_percent_caps_at_64);
    RUN_TEST(test_l85_border_zero_stays_zero);
    RUN_TEST(test_l85_border_shorthand_four_sides);
    RUN_TEST(test_l85_outline_uses_the_same_cap);
}
