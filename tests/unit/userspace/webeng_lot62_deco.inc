/* Lot 62: text-decoration lines under, over and through text runs. */
static const os_fb_web_item_t *l62_text(const char *s) {
    const os_fb_web_item_t *t = find_text(s);
    if (!t) unity_test_fail("missing text", __FILE__, __LINE__);
    return t ? t : &g_doc.items[0];
}

static void test_l62_deco_underline_basic(void) {
    const os_fb_web_item_t *t;
    lot52_render(L62_BODY "<p style='margin:0;text-decoration:underline'>AB</p><p id=n style='margin:0'>C</p></body>", 320);
    t = l62_text("AB");
    l61_rect_at(t->x, t->y + 8, 16, 1, 0x000000);
    l61_box("n", 0, 10, 320, 10);                      /* no layout change */
    TEST_ASSERT_EQUAL(3, (int)g_doc.nitems);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l62_deco_lines_colour_thickness_style(void) {
    const os_fb_web_item_t *t;
    lot52_render(L62_BODY "<p style='margin:4px 0 0 0;text-decoration:overline line-through #ff0000 2px'>AB</p></body>", 320);
    t = l62_text("AB");
    l61_rect_at(t->x, t->y - 2, 16, 2, 0xFF0000);
    l61_rect_at(t->x, t->y + 3, 16, 2, 0xFF0000);
    TEST_ASSERT_TRUE(l61_index(0xFF0000) > 0);          /* after the text item */
    lot52_render(L62_BODY "<p style='margin:0;text-decoration:underline double;text-decoration-color:#0000ff'>AB</p></body>", 320);
    t = l62_text("AB");
    l61_rect_at(t->x, t->y + 8, 16, 1, 0x0000FF);
    l61_rect_at(t->x, t->y + 10, 16, 1, 0x0000FF);
    lot52_render(L62_BODY "<p style='margin:0;text-decoration-line:underline;text-decoration-style:dashed;text-decoration-thickness:1px'>ABCD</p></body>", 320);
    t = l62_text("ABCD");
    l61_rect_at(t->x, t->y + 8, 3, 1, 0x000000);         /* dashes of max(2, 3) px */
    l61_rect_at(t->x + 6, t->y + 8, 3, 1, 0x000000);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
    /* a 25-character longhand name reaches its parser */
    lot52_render(L62_BODY "<p style='margin:0;text-decoration:underline;text-decoration-thickness:3px'>AB</p></body>", 320);
    t = l62_text("AB");
    l61_rect_at(t->x, t->y + 8, 16, 3, 0x000000);
}

static void test_l62_deco_propagates_with_the_declaring_colour(void) {
    const os_fb_web_item_t *t;
    lot52_render(L62_BODY "<div style='color:#0000ff;text-decoration:underline'>A<span style='color:#ff0000'>BC</span></div></body>", 320);
    t = l62_text("BC");
    L62_HEX(0xFF0000, t->color);
    l61_rect_at(t->x, t->y + 8, 16, 1, 0x0000FF);        /* the div's colour */
    /* none on a child does not remove the ancestor's line */
    lot52_render(L62_BODY "<div style='text-decoration:underline'><span style='text-decoration:none'>AB</span></div></body>", 320);
    t = l62_text("AB");
    l61_rect_at(t->x, t->y + 8, 16, 1, 0x000000);
    /* the nearest declaration of a line kind wins */
    lot52_render(L62_BODY "<div style='text-decoration:underline #0000ff'><span style='text-decoration:underline #ff0000'>AB</span></div></body>", 320);
    t = l62_text("AB");
    l61_rect_at(t->x, t->y + 8, 16, 1, 0xFF0000);
    TEST_ASSERT_EQUAL(-1, l61_index(0x0000FF));
}

static void test_l62_deco_stops_at_atomic_boxes(void) {
    lot52_render(L62_BODY "<div style='text-decoration:underline'><span style='display:inline-block'>AB</span>"
                 "<span style='display:inline-block;text-decoration:overline'>CD</span></div></body>", 320);
    TEST_ASSERT_EQUAL(3, (int)g_doc.nitems);             /* AB, CD and CD's overline */
    l61_rect_at(l62_text("CD")->x, l62_text("CD")->y - 1, 16, 1, 0x000000);
}

static void test_l62_deco_user_agent_tags(void) {
    const os_fb_web_item_t *t;
    lot52_render(L62_BODY "<p style='margin:0'><u>AB</u><s>CD</s><a href='/x'>EF</a></p></body>", 320);
    t = l62_text("AB");
    l61_rect_at(t->x, t->y + 8, 16, 1, 0x000000);
    t = l62_text("CD");
    l61_rect_at(t->x, t->y + 4, 16, 1, 0x000000);
    TEST_ASSERT_EQUAL(5, (int)g_doc.nitems);             /* links keep no UA underline */
}

static void test_l62_deco_refusals_and_wavy(void) {
    lot52_render(L62_BODY "<p id=a style='text-decoration:underline;text-decoration:underline underline'>A</p>"
                 "<p style='text-decoration:none underline'>B</p><p style='text-decoration-line:red'>C</p>"
                 "<p style='text-decoration-style:groove'>D</p><p style='text-decoration-thickness:10%'>E</p>"
                 "<p style='text-decoration:underline solid dashed'>F</p><p style='text-decoration-line:solid'>G</p></body>", 320);
    TEST_ASSERT_EQUAL(WE_DECO_UNDER, lot52_node("a")->deco_line);
    TEST_ASSERT_EQUAL(7, l61_diag_count("text-decoration refused"));
    lot52_render(L62_BODY "<p style='margin:0;text-decoration:underline wavy #ff0000'>AB</p></body>", 320);
    l61_rect_at(l62_text("AB")->x, l62_text("AB")->y + 8, 16, 1, 0xFF0000);
    TEST_ASSERT_EQUAL(1, l61_diag_count("wavy decoration painted solid"));
}

static void lot62_deco_run(void) {
    RUN_TEST(test_l62_deco_underline_basic);
    RUN_TEST(test_l62_deco_lines_colour_thickness_style);
    RUN_TEST(test_l62_deco_propagates_with_the_declaring_colour);
    RUN_TEST(test_l62_deco_stops_at_atomic_boxes);
    RUN_TEST(test_l62_deco_user_agent_tags);
    RUN_TEST(test_l62_deco_refusals_and_wavy);
}
