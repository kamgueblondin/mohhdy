/* Lot 64 E: list-style-image markers and inline background images. */

static int l64_text_count_at(const char *text, int y) {
    uint32_t i;
    int count = 0;
    for (i = 0; i < g_doc.nitems; i++)
        if (g_doc.items[i].kind == OS_FB_WEB_TEXT && g_doc.items[i].y == y && !strcmp(g_doc.items[i].text, text)) count++;
    return count;
}

static void test_l64_marker_image(void) {
    int i;
    l63_bgi("<body style='margin:0'><ul style='margin:0;padding-left:40px;list-style-image:url(/images/mire-ga16.png)'>"
            "<li id=a>a</li><li id=b style='list-style-image:url(/images/missing.png)'>b</li>"
            "<li id=c style='list-style-image:none;list-style-type:square'>c</li></ul></body>");
    /* 48 x 32 scaled to 12 x 8, its right edge 8 px before the li */
    for (i = 0; i < 12; i++) TEST_ASSERT_EQUAL((long)(i * 0x141414), l63_at(20 + i, 1));
    TEST_ASSERT_EQUAL(-1, l63_at(19, 1));
    TEST_ASSERT_EQUAL(-1, l63_at(32, 1));
    TEST_ASSERT_EQUAL(-1, l63_at(20, 0));
    TEST_ASSERT_EQUAL((long)0xDCDCDC, l63_at(31, 8));
    TEST_ASSERT_EQUAL(-1, l63_at(31, 9));
    TEST_ASSERT_EQUAL(0, l64_text_count_at("*", 1));          /* no glyph marker beside the image */
    TEST_ASSERT_EQUAL(1, l61_diag_count("list marker image scaled"));
    /* a failing image falls back to the type */
    TEST_ASSERT_EQUAL(1, l61_diag_count("image read failed"));
    l61_text_at("*", 24, 11);
    l61_text_at("#", 24, 21);
    TEST_ASSERT_EQUAL(2, lot52_read_calls);
    /* inherited, and the shorthand sets it */
    TEST_ASSERT_EQUAL(lot52_node("a")->lsi_len, (int)strlen("/images/mire-ga16.png"));
    TEST_ASSERT_EQUAL(0, lot52_node("c")->lsi_len);
    l63_bgi("<body style='margin:0'><ul id=u style='list-style:square url(/images/mire-ga16.png) outside'><li>a</li></ul>"
            "<ul id=v style='list-style-image:url(/images/mire-ga16.png);list-style:circle'><li>b</li></ul></body>");
    TEST_ASSERT_EQUAL(WE_LIST_SQUARE, lot52_node("u")->list_style);
    TEST_ASSERT_EQUAL(21, lot52_node("u")->lsi_len);
    TEST_ASSERT_EQUAL(0, lot52_node("v")->lsi_len);          /* the shorthand resets the image */
    l63_bgi("<body style='margin:0'><ul id=u style='list-style-image:url(/images/mire-ga16.png);list-style-image:url();"
            "list-style:url(/images/a.png) url(/images/b.png)'><li>a</li></ul></body>");
    TEST_ASSERT_EQUAL(2, l61_diag_count("list marker refused"));
    TEST_ASSERT_EQUAL(21, lot52_node("u")->lsi_len);
}

static void test_l64_marker_image_budget_and_li_background(void) {
    char html[1400];
    int k, at;
    /* an li with its own background image keeps it under its marker */
    l63_bgi("<body style='margin:0'><ul style='margin:0;padding-left:40px'><li id=a style='list-style-image:url(/images/mire-ga16.png);"
            "background:url(/images/mire-ga16.png) no-repeat 0 0 / 6px 1px'>a</li></ul></body>");
    TEST_ASSERT_EQUAL((long)l63_ramp[1], l63_at(41, 0));
    TEST_ASSERT_EQUAL(0, l63_at(20, 1));
    TEST_ASSERT_EQUAL(0, lot52_node("a")->bgi_pos[0]);
    TEST_ASSERT_EQUAL(6, lot52_node("a")->bgi_size[0]);
    TEST_ASSERT_EQUAL(WE_BOX_BORDER, lot52_node("a")->bg_clip);  /* restored after the marker */
    /* hidden: no marker image either */
    l63_bgi("<body style='margin:0'><ul style='margin:0;padding-left:40px;visibility:hidden'><li style='list-style-image:url(/images/mire-ga16.png)'>a</li></ul></body>");
    TEST_ASSERT_EQUAL(0, (int)g_doc.nitems);
    /* no room left: the type marker */
    at = snprintf(html, sizeof(html), "<body style='margin:0'><div style='width:8px'>");
    for (k = 0; k < 90; k++) at += snprintf(html + at, sizeof(html) - (size_t)at, "a ");
    snprintf(html + at, sizeof(html) - (size_t)at, "</div><ul style='margin:0;padding-left:40px'><li style='list-style-image:url(/images/mire-ga16.png)'>b</li></ul></body>");
    l63_bgi(html);
    TEST_ASSERT_EQUAL(1, l61_diag_count("background image budget"));
    TEST_ASSERT_NOT_NULL(find_text("*"));
}

static void test_l64_inline_background_image(void) {
    int i;
    l63_bgi("<body style='margin:0'><div style='width:64px'>ab <span id=s style='background:#ff0000 url(/images/mire-ga16.png) no-repeat 0 0 / 6px 2px'>"
            "cd ef gh</span> ij</div></body>");
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
    /* each line fragment gets its own tile at its top left */
    for (i = 0; i < 6; i++) {
        TEST_ASSERT_EQUAL((long)l63_ramp[i], l63_at(24 + i, 0));
        TEST_ASSERT_EQUAL((long)l63_ramp[i], l63_at(24 + i, 1));
        TEST_ASSERT_EQUAL((long)l63_ramp[i], l63_at(i, 10));
    }
    TEST_ASSERT_EQUAL(0xFF0000, l63_at(30, 0));
    TEST_ASSERT_EQUAL(0xFF0000, l63_at(24, 2));
    TEST_ASSERT_EQUAL(-1, l63_at(16, 0));
    l61_text_at("cd ef", 24, 1);                               /* the text over the tiles */
    TEST_ASSERT_EQUAL(1, lot52_read_calls);
    /* repeating along the fragment, cut to it */
    l63_bgi("<body style='margin:0'>a<span style='background-image:url(/images/mire-ga16.png);background-size:6px 10px'>bcd</span></body>");
    TEST_ASSERT_EQUAL((long)l63_ramp[0], l63_at(8, 0));
    TEST_ASSERT_EQUAL((long)l63_ramp[0], l63_at(14, 9));
    TEST_ASSERT_EQUAL((long)l63_ramp[3], l63_at(29, 5));
    TEST_ASSERT_EQUAL(-1, l63_at(32, 5));
    TEST_ASSERT_EQUAL(-1, l63_at(7, 5));
    /* layers in an inline box */
    l63_bgi("<body style='margin:0'><span style='background-image:url(/images/mire-ga16.png), url(/images/mire-g1.png);"
            "background-size:6px 1px, 16px 10px;background-repeat:no-repeat, repeat'>xy</span></body>");
    TEST_ASSERT_EQUAL((long)l63_ramp[2], l63_at(2, 0));
    TEST_ASSERT_TRUE(l63_at(2, 1) >= 0);
    TEST_ASSERT_EQUAL(2, lot52_read_calls);
}

static void lot64_marker_run(void) {
    RUN_TEST(test_l64_marker_image);
    RUN_TEST(test_l64_marker_image_budget_and_li_background);
    RUN_TEST(test_l64_inline_background_image);
}
