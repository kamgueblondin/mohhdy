/* Lot 64 A: writing-mode vertical-rl / vertical-lr orthogonal roots. */
#define L64_BODY "<body style='margin:0'>"

/* The TEXT item holding exactly the glyph c at (x, y), 8x8. */
static void l64_glyph(char c, int x, int y) {
    uint32_t i;
    for (i = 0; i < g_doc.nitems; i++) {
        const os_fb_web_item_t *it = &g_doc.items[i];
        if (it->kind == OS_FB_WEB_TEXT && it->text[0] == c && !it->text[1] && it->x == x && it->y == y &&
            it->w == 8 && it->h == 8) return;
    }
    unity_test_fail("missing stacked glyph", __FILE__, __LINE__);
}

static int l64_text_items(void) {
    uint32_t i;
    int count = 0;
    for (i = 0; i < g_doc.nitems; i++) if (g_doc.items[i].kind == OS_FB_WEB_TEXT) count++;
    return count;
}

static void l64_box(const char *id, int x, int y, int w, int h) {
    const we_node_t *n = lot52_node(id);
    TEST_ASSERT_EQUAL(x, n->x);
    TEST_ASSERT_EQUAL(y, n->y);
    TEST_ASSERT_EQUAL(w, n->w);
    TEST_ASSERT_EQUAL(h, n->h);
}

static void test_l64_vertical_rl_lines_from_the_right(void) {
    lot52_render(L64_BODY "<div id=v style='writing-mode:vertical-rl;height:40px;background:#ffff00'>ab cd"
                 "<div id=c style='background:#ff0000;height:6px;width:12px;margin-left:3px'>x</div></div>"
                 "<div id=n>after</div></body>", 320);
    /* extent: one line (10) + c's logical height 12 + its logical bottom margin 3 */
    l64_box("v", 0, 0, 25, 40);
    l61_rect_at(0, 0, 25, 40, 0xFFFF00);
    l64_glyph('a', 16, 0);
    l64_glyph('b', 16, 8);
    l64_glyph('c', 16, 24);                                  /* the space paints nothing */
    l64_glyph('d', 16, 32);
    l64_box("c", 3, 0, 12, 6);
    l61_rect_at(3, 0, 12, 6, 0xFF0000);
    l64_glyph('x', 6, 0);                                     /* c's first line at its right */
    l61_text_at("after", 0, 41);
    TEST_ASSERT_EQUAL(8, (int)g_doc.nitems);
    TEST_ASSERT_EQUAL(1, l61_diag_count("vertical text painted upright"));
    /* the declared physical values are back after the layout */
    TEST_ASSERT_EQUAL(12, lot52_node("c")->width);
    TEST_ASSERT_EQUAL(6, lot52_node("c")->height);
    TEST_ASSERT_EQUAL(3, lot52_node("c")->margin[3]);
    TEST_ASSERT_EQUAL(0, lot52_node("c")->margin[1]);
}

static void test_l64_vertical_lr_lines_from_the_left(void) {
    lot52_render(L64_BODY "<div id=v style='writing-mode:vertical-lr;height:40px'>ab cd"
                 "<div id=c style='height:6px;width:12px;margin-left:3px'>x</div></div></body>", 320);
    l64_box("v", 0, 0, 25, 40);
    l64_glyph('a', 1, 0);
    l64_glyph('d', 1, 32);
    l64_box("c", 13, 0, 12, 6);                               /* margin-left is block-start */
    l64_glyph('x', 14, 0);
    /* a definite width keeps the box; rl lines start at its right edge */
    lot52_render(L64_BODY "<div id=v style='writing-mode:vertical-rl;width:50px'>abc</div></body>", 320);
    l64_box("v", 0, 0, 50, 24);                               /* auto height: max-content */
    l64_glyph('a', 41, 0);
    l64_glyph('c', 41, 16);
    /* a definite height wraps the inline axis */
    lot52_render(L64_BODY "<div id=v style='writing-mode:vertical-rl;height:24px'>abcd ef</div></body>", 320);
    l64_box("v", 0, 0, 20, 24);
    l64_glyph('d', 11, 24);                                   /* the word overflows, as across */
    l64_glyph('e', 1, 0);
    l64_glyph('f', 1, 8);
    TEST_ASSERT_EQUAL(6, l64_text_items());
}

static void test_l64_vertical_upright_and_refusals(void) {
    static const char *const bad_mode[] = {"vertical-up", "sideways-rl", "sideways-lr", "tb-rl", "", 0};
    static const char *const bad_orientation[] = {"sideways", "sideways-right", "use-glyph-orientation", "upright mixed", 0};
    char html[200];
    int k;
    lot52_render(L64_BODY "<div style='writing-mode:vertical-rl;text-orientation:upright;height:20px'>ab</div></body>", 320);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
    TEST_ASSERT_EQUAL(1, g_doc.nodes[2].upright);
    lot52_render(L64_BODY "<div style='writing-mode:vertical-rl;text-orientation:upright;text-orientation:mixed;height:20px'>ab</div></body>", 320);
    TEST_ASSERT_EQUAL(1, l61_diag_count("vertical text painted upright"));
    /* a box without text paints no diagnostic */
    lot52_render(L64_BODY "<div style='writing-mode:vertical-rl;height:20px;width:4px;background:#ff0000'></div></body>", 320);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
    for (k = 0; bad_mode[k]; k++) {
        snprintf(html, sizeof(html), L64_BODY "<div id=v style='writing-mode:vertical-rl;writing-mode:%s'>ab</div></body>", bad_mode[k]);
        lot52_render(html, 320);
        if (bad_mode[k][0]) TEST_ASSERT_EQUAL(1, l61_diag_count("writing mode refused"));
        TEST_ASSERT_EQUAL(WE_WM_VERTICAL_RL, lot52_node("v")->wmode);
    }
    for (k = 0; bad_orientation[k]; k++) {
        snprintf(html, sizeof(html), L64_BODY "<div id=v style='text-orientation:upright;text-orientation:%s'>ab</div></body>", bad_orientation[k]);
        lot52_render(html, 320);
        TEST_ASSERT_EQUAL(1, l61_diag_count("text orientation refused"));
        TEST_ASSERT_EQUAL(1, lot52_node("v")->upright);
    }
    lot52_render(L64_BODY "<div id=v style='direction:rtl;direction:sideways'>ab</div></body>", 320);
    TEST_ASSERT_EQUAL(1, l61_diag_count("direction refused"));
    TEST_ASSERT_EQUAL(1, lot52_node("v")->dir_rtl);
    /* inherited */
    lot52_render(L64_BODY "<div style='writing-mode:vertical-lr;direction:rtl;text-orientation:upright;height:20px'><p id=p style='margin:0'>a</p></div></body>", 320);
    TEST_ASSERT_EQUAL(WE_WM_VERTICAL_LR, lot52_node("p")->wmode);
    TEST_ASSERT_EQUAL(1, lot52_node("p")->dir_rtl);
    TEST_ASSERT_EQUAL(1, lot52_node("p")->upright);
}

static void test_l64_vertical_physical_properties_rotate(void) {
    /* padding-inline-start is the physical top, margin-block-start the right */
    lot52_render(L64_BODY "<div id=v style='writing-mode:vertical-rl;height:30px'><div id=c style='padding-inline-start:4px;"
                 "margin-block-start:2px;width:10px;border-radius:5px 0 0 0;box-shadow:3px 1px #0000ff;background:#ff0000'>x</div></div></body>", 320);
    l64_box("v", 0, 0, 12, 30);
    l64_box("c", 0, 0, 10, 30);
    TEST_ASSERT_EQUAL(4, lot52_node("c")->padding[0]);
    TEST_ASSERT_EQUAL(2, lot52_node("c")->margin[1]);
    l64_glyph('x', 1, 4);
    l61_rect_at(3, 0, 7, 30, 0xFF0000);                       /* rounded top-left: the left column starts lower */
    l61_rect_at(0, 3, 1, 27, 0xFF0000);
    l61_rect_at(6, 1, 7, 30, 0x0000FF);                       /* shadow 3px right, 1px down */
    TEST_ASSERT_EQUAL(5, lot52_node("c")->rad_x[0]);
    TEST_ASSERT_EQUAL(0, lot52_node("c")->rad_x[1]);
    TEST_ASSERT_EQUAL(3, lot52_node("c")->shadow[0].x);
    TEST_ASSERT_EQUAL(1, lot52_node("c")->shadow[0].y);
    /* the same in vertical-lr: block-start is the left */
    lot52_render(L64_BODY "<div id=v style='writing-mode:vertical-lr;height:30px'><div id=c style='margin-block-start:2px;"
                 "width:10px;box-shadow:3px 1px #0000ff;background:#ff0000'></div></div></body>", 320);
    l64_box("c", 2, 0, 10, 30);
    l61_rect_at(2, 0, 10, 30, 0xFF0000);
    l61_rect_at(12, 1, 3, 30, 0x0000FF);                      /* the visible part of the shadow */
    /* a physical linear gradient keeps its direction: bands across, top to bottom */
    lot52_render(L64_BODY "<div style='writing-mode:vertical-lr;height:20px'><div id=g style='width:8px;"
                 "background:linear-gradient(to bottom,#ff0000,#0000ff)'></div></div></body>", 320);
    l61_rect_at(0, 0, 8, 2, 0xEF0010);
    l61_rect_at(0, 10, 8, 2, 0x70008F);
    TEST_ASSERT_EQUAL(WE_GRAD_BOTTOM, lot52_node("g")->grad_dir);
    lot52_render(L64_BODY "<div style='writing-mode:vertical-rl;height:20px'><div id=g style='width:8px;"
                 "background:linear-gradient(to right,#ff0000,#0000ff)'></div></div></body>", 320);
    l61_rect_at(0, 0, 1, 20, 0xEF0010);                       /* vertical bands, red at the left */
    l61_rect_at(7, 0, 1, 20, 0x1000EF);
    TEST_ASSERT_EQUAL(WE_GRAD_RIGHT, lot52_node("g")->grad_dir);
}

static void test_l64_vertical_flex_and_nesting(void) {
    /* a flex row runs along the inline axis: down */
    lot52_render(L64_BODY "<div id=v style='writing-mode:vertical-rl;height:30px;display:flex'><div id=a style='width:6px;height:8px;background:#ff0000'></div>"
                 "<div id=b style='width:6px;height:8px;background:#00ff00'></div></div></body>", 320);
    l64_box("v", 0, 0, 6, 30);
    l64_box("a", 0, 0, 6, 8);
    l64_box("b", 0, 8, 6, 8);
    /* nested modes take the root's; images and background images are refused */
    lot52_render(L64_BODY "<div style='writing-mode:vertical-rl;height:30px'><img id=i src=/images/mire-g1.png alt=no>"
                 "<div id=g style='width:4px;background-image:url(/images/mire-ga16.png)'></div>"
                 "<p id=h style='writing-mode:horizontal-tb;margin:0;width:50%'>q</p></div></body>", 320);
    TEST_ASSERT_EQUAL(1, l61_diag_count("nested writing mode inherited"));
    TEST_ASSERT_EQUAL(WE_WM_VERTICAL_RL, lot52_node("h")->wmode);
    TEST_ASSERT_EQUAL(1, l61_diag_count("background image refused"));
    TEST_ASSERT_EQUAL(0, lot52_node("g")->bgi_len);
    TEST_ASSERT_EQUAL(1, l61_diag_count("vertical flow size kept physical"));
    TEST_ASSERT_EQUAL(1, l61_diag_count("image in vertical flow refused"));
    l64_glyph('[', 15, 0);                                    /* the alt text, stacked */
    l64_glyph('n', 15, 8);
    l64_glyph('o', 15, 16);
    l64_glyph('q', 1, 0);
    /* a vertical root as a flex item: its definite height is the inline size; the
     * intrinsic estimate is the max-content cut in inline sizes (64 px at 16: 4 lines) */
    lot52_render(L64_BODY "<div style='display:flex;width:100px'><div id=v style='writing-mode:vertical-rl;height:16px;flex:none'>ab cd ef</div>"
                 "<div id=n style='flex:none'>z</div></div></body>", 320);
    l64_box("v", 0, 0, 30, 16);
    l64_glyph('a', 21, 0);
    l64_glyph('e', 1, 0);
    TEST_ASSERT_EQUAL(40, lot52_node("v")->max_content);
    TEST_ASSERT_EQUAL(40, lot52_node("v")->min_content);
}

static void test_l64_vertical_budget(void) {
    /* every glyph is an item: past the list the rest is dropped */
    lot52_render(L64_BODY "<div style='writing-mode:vertical-rl;height:800px'>"
                 "abcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyz</div></body>", 320);
    TEST_ASSERT_EQUAL(OS_FB_WEB_ITEMS, (int)g_doc.nitems);
    TEST_ASSERT_EQUAL(8, (int)g_doc.perf.items_dropped);
    TEST_ASSERT_EQUAL(1, l61_diag_count("display budget exceeded"));
    l64_glyph('r', 1, 95 * 8);
}

static void lot64_vertical_run(void) {
    RUN_TEST(test_l64_vertical_rl_lines_from_the_right);
    RUN_TEST(test_l64_vertical_lr_lines_from_the_left);
    RUN_TEST(test_l64_vertical_upright_and_refusals);
    RUN_TEST(test_l64_vertical_physical_properties_rotate);
    RUN_TEST(test_l64_vertical_flex_and_nesting);
    RUN_TEST(test_l64_vertical_budget);
}
