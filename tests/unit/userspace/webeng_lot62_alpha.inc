/* Lot 62: colours with alpha (CSS Color 4 rgb/rgba/hsl/hsla, #rgba,
 * #rrggbbaa, transparent) and source-over compositing of translucent
 * paint and opacity groups into opaque fragments. */
#define L62_BODY "<body style='margin:0'>"

static void test_l62_color_hex_alpha_forms(void) {
    lot52_render(L62_BODY "<div id=a style='background:#f008;height:2px'></div>"
                 "<div id=b style='background:#00ff0080;height:2px'></div>"
                 "<div id=c style='background:#123456;height:2px'></div></body>", 320);
    L62_HEX(0x77FF0000u, lot52_node("a")->bg);   /* 8 * 17 = 136, transparency 119 */
    L62_HEX(0x7F00FF00u, lot52_node("b")->bg);
    L62_HEX(0x00123456u, lot52_node("c")->bg);
    L62_HEX(0xFF7777u, l62_pixel(5, 0));          /* 136 of red over white */
    L62_HEX(0x7FFF7Fu, l62_pixel(5, 2));
    L62_HEX(0x123456u, l62_pixel(5, 4));
    l62_no_alpha_left();
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l62_color_rgb_functions(void) {
    lot52_render(L62_BODY "<div id=a style='background:rgb(10, 20, 30);height:2px'></div>"
                 "<div id=b style='background:rgba(255,0,0,0.5);height:2px'></div>"
                 "<div id=c style='background:rgb(0 0 255 / 25%);height:2px'></div>"
                 "<div id=d style='background:rgb(100%, 50%, 0%);height:2px'></div>"
                 "<div id=e style='background:rgba(300, -4, 12.6, 2);height:2px'></div>"
                 "<div id=f style='background:rgb(0 128 0 / .75);height:2px'></div></body>", 320);
    L62_HEX(0x000A141Eu, lot52_node("a")->bg);
    L62_HEX(0x7FFF0000u, lot52_node("b")->bg);   /* alpha 128 */
    L62_HEX(0xBF0000FFu, lot52_node("c")->bg);   /* alpha 64 */
    L62_HEX(0x00FF8000u, lot52_node("d")->bg);
    L62_HEX(0x00FF000Du, lot52_node("e")->bg);   /* clamped, rounded, alpha 1 */
    L62_HEX(0x40008000u, lot52_node("f")->bg);   /* alpha 191 */
    L62_HEX(0xFF7F7Fu, l62_pixel(1, 2));
    L62_HEX(0xBFBFFFu, l62_pixel(1, 4));
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l62_color_hsl_functions(void) {
    lot52_render(L62_BODY "<div id=a style='background:hsl(120, 100%, 50%);height:2px'></div>"
                 "<div id=b style='background:hsla(240deg 100% 25% / 0.5);height:2px'></div>"
                 "<div id=c style='background:hsl(0, 0%, 50%);height:2px'></div>"
                 "<div id=d style='background:hsl(-60, 100%, 50%);height:2px'></div>"
                 "<div id=e style='background:hsl(30, 100%, 75%);height:2px'></div></body>", 320);
    L62_HEX(0x0000FF00u, lot52_node("a")->bg);
    L62_HEX(0x7F000080u, lot52_node("b")->bg);
    L62_HEX(0x00808080u, lot52_node("c")->bg);
    L62_HEX(0x00FF00FFu, lot52_node("d")->bg);   /* hue wraps to 300 */
    L62_HEX(0x00FFBF80u, lot52_node("e")->bg);
}

static void test_l62_color_function_refusals(void) {
    lot52_render(L62_BODY "<div id=a style='background:#00f;background:rgb(1, 2);height:2px'></div>"
                 "<div id=b style='background:#00f;background:rgb(1 2, 3);height:2px'></div>"
                 "<div id=c style='background:#00f;background:hsl(120, 100, 50);height:2px'></div>"
                 "<div id=d style='background:#00f;background:rgba(1, 2, 3, 4, 5);height:2px'></div>"
                 "<div id=e style='background:#00f;background:rgb(10%, 2, 3);height:2px'></div>"
                 "<div id=f style='background:#00f;background:rgb(1 2 3 4);height:2px'></div>"
                 "<div id=g style='color:#00f;color:rgb(1, 2, 3, / 4)'>x</div>"
                 "<div id=h style='background:#00f;background:#12345'></div></body>", 320);
    L62_HEX(0x0000FFu, lot52_node("a")->bg);
    L62_HEX(0x0000FFu, lot52_node("b")->bg);
    L62_HEX(0x0000FFu, lot52_node("c")->bg);
    L62_HEX(0x0000FFu, lot52_node("d")->bg);
    L62_HEX(0x0000FFu, lot52_node("e")->bg);
    L62_HEX(0x0000FFu, lot52_node("f")->bg);
    L62_HEX(0x0000FFu, lot52_node("g")->color);
    L62_HEX(0x0000FFu, lot52_node("h")->bg);
    TEST_ASSERT_EQUAL(7, l61_diag_count("color refused"));
}

static void test_l62_color_function_in_border_shorthand(void) {
    lot52_render(L62_BODY "<div id=a style='border:2px solid rgb(0, 0, 255);width:6px;height:6px'></div>"
                 "<div id=b style='border:2px solid transparent;width:6px;height:6px;background:#00ff00'></div></body>", 320);
    L62_HEX(0x0000FFu, lot52_node("a")->border_color);
    TEST_ASSERT_EQUAL(2, lot52_node("a")->border);
    L62_HEX(0x0000FFu, l62_pixel(0, 0));
    /* A transparent border paints nothing: the green background shows. */
    L62_HEX(0xFF000000u, lot52_node("b")->border_color);
    L62_HEX(0x00FF00u, l62_pixel(0, 10));
    TEST_ASSERT_EQUAL(0, l61_count(OS_FB_WEB_BORDER) - 1);
    l62_no_alpha_left();
}

/* A translucent box half over a blue box: one piece per backdrop. */
static void test_l62_alpha_splits_along_the_backdrop(void) {
    lot52_render(L62_BODY "<div style='width:20px;height:10px;background:#0000ff'></div>"
                 "<div id=t style='margin-top:-5px;width:40px;height:10px;background:rgba(255,0,0,.5)'></div></body>", 320);
    l61_rect_at(0, 5, 20, 5, 0x80007F);                   /* over blue */
    l61_rect_at(20, 5, 20, 5, 0xFF7F7F);                  /* over the canvas */
    l61_rect_at(0, 10, 40, 5, 0xFF7F7F);
    L62_HEX(0x0000FFu, l62_pixel(5, 2));
    TEST_ASSERT_EQUAL(4, (int)g_doc.nitems);
    l62_no_alpha_left();
}

static void test_l62_alpha_over_border_sides(void) {
    lot52_render(L62_BODY "<div style='width:16px;height:16px;border:2px solid #000000'></div>"
                 "<div style='margin-top:-20px;width:20px;height:4px;background:rgba(255,255,255,0.5)'></div></body>", 320);
    L62_HEX(0x808080u, l62_pixel(0, 0));   /* white half over the black top */
    L62_HEX(0x808080u, l62_pixel(1, 3));   /* and over the left side */
    L62_HEX(0xFFFFFFu, l62_pixel(5, 3));   /* over the canvas inside */
    L62_HEX(0x000000u, l62_pixel(1, 5));   /* below the band */
    l62_no_alpha_left();
}

/* Text under a translucent box is repainted on top with its own blend. */
static void test_l62_alpha_over_text_repaints_the_glyphs(void) {
    const os_fb_web_item_t *h;
    int x, y;
    lot52_render(L62_BODY "<p style='margin:0'>HH</p>"
                 "<div style='margin-top:-10px;width:8px;height:10px;background:rgba(0,0,255,0.5)'></div></body>", 320);
    h = find_text("HH");
    TEST_ASSERT_NOT_NULL(h);
    if (!h) return;
    x = h->x; y = h->y;
    L62_HEX(0x7F7FFFu, l62_pixel(L62_H_GAP(x, y)));      /* box over canvas */
    L62_HEX(0x000080u, l62_pixel(L62_H_INK(x, y)));      /* box over black ink */
    L62_HEX(0x000000u, l62_pixel(L62_H_INK(x + 8, y)));  /* second H untouched */
    TEST_ASSERT_NOT_NULL(find_text("H"));                                /* cut to the box */
    TEST_ASSERT_EQUAL(1, l62_items_with(0x000080));
    l62_no_alpha_left();
}

static void test_l62_alpha_text_colour(void) {
    const os_fb_web_item_t *t;
    lot52_render(L62_BODY "<p style='margin:0;color:rgba(255,0,0,0.5);background:#0000ff'>HI</p></body>", 320);
    t = find_text("HI");
    TEST_ASSERT_NOT_NULL(t);
    if (!t) return;
    L62_HEX(0x80007Fu, t->color);           /* one item over a uniform backdrop */
    L62_HEX(0x80007Fu, l62_pixel(L62_H_INK(t->x, t->y)));
    /* Over two backdrops the text is cut into pieces of each colour. */
    lot52_render(L62_BODY "<div style='width:4px;height:10px;background:#0000ff'></div>"
                 "<p style='margin:-10px 0 0 0;color:rgba(255,0,0,0.5)'>H</p></body>", 320);
    L62_HEX(0x80007Fu, l62_pixel(1, 1));
    L62_HEX(0xFF7F7Fu, l62_pixel(5, 1));
    TEST_ASSERT_NULL(find_text("H"));
    l62_no_alpha_left();
}

/* Group opacity: the child covers its parent inside the group, so it
 * blends with the canvas, not with the parent's red. */
static void test_l62_opacity_group_is_one_layer(void) {
    lot52_render(L62_BODY "<div style='opacity:0.5;width:20px;height:10px;background:#ff0000'>"
                 "<div style='width:10px;height:10px;background:#0000ff'></div></div></body>", 320);
    L62_HEX(0x7F7FFFu, l62_pixel(5, 5));
    L62_HEX(0xFF7F7Fu, l62_pixel(15, 5));
    l62_no_alpha_left();
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l62_opacity_nested_groups(void) {
    /* Inner group over the outer group's red, then the outer over white. */
    lot52_render(L62_BODY "<div style='opacity:0.5;width:20px;height:10px;background:#ff0000'>"
                 "<div style='opacity:0.5;width:10px;height:10px;background:#0000ff'></div></div></body>", 320);
    L62_HEX(0xBF7FBFu, l62_pixel(5, 5));
    L62_HEX(0xFF7F7Fu, l62_pixel(15, 5));
    /* Without outer paint under it the inner alpha multiplies: 64. */
    lot52_render(L62_BODY "<div style='opacity:0.5;width:20px;height:10px'>"
                 "<div style='opacity:0.5;width:10px;height:10px;background:#0000ff'></div></div></body>", 320);
    L62_HEX(0xBFBFFFu, l62_pixel(5, 5));
    L62_HEX(0xFFFFFFu, l62_pixel(15, 5));
    l62_no_alpha_left();
}

static void test_l62_opacity_group_over_earlier_page_paint(void) {
    /* The group's items blend over the page under the group, the later
     * one over the earlier one inside the group. */
    lot52_render(L62_BODY "<div style='width:40px;height:20px;background:#00ff00'></div>"
                 "<div style='opacity:0.5;margin-top:-20px'><div style='width:20px;height:10px;background:#ff0000'></div>"
                 "<div style='margin-top:-5px;width:10px;height:10px;background:rgba(0,0,255,0.5)'></div></div></body>", 320);
    L62_HEX(0x807F00u, l62_pixel(15, 2));   /* red over green */
    L62_HEX(0x407F40u, l62_pixel(5, 7));    /* (blue over red) over green */
    L62_HEX(0x00BF40u, l62_pixel(5, 12));   /* blue alone, alpha 64 */
    L62_HEX(0x00FF00u, l62_pixel(30, 15));
    l62_no_alpha_left();
}

static void test_l62_alpha_zero_paint_is_dropped(void) {
    lot52_render(L62_BODY "<div style='height:4px;background:rgba(255,0,0,0)'></div>"
                 "<p style='margin:0;color:transparent'>X</p></body>", 320);
    TEST_ASSERT_EQUAL(0, (int)g_doc.nitems);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l62_alpha_budget_keeps_the_list_opaque(void) {
    static char html[4096];
    int k, at;
    at = sprintf(html, L62_BODY);
    for (k = 0; k < 12; k++) at += sprintf(html + at, "<div style='width:%dpx;height:6px;background:#%02x00ff'></div>", 60 - 4 * k, k * 16);
    for (k = 0; k < 6; k++) at += sprintf(html + at, "<div style='margin-top:%dpx;width:70px;height:72px;background:rgba(255,0,0,0.5)'></div>", k ? -71 : -72);
    at += sprintf(html + at, "<div style='height:2px;background:rgba(0,0,0,0)'></div></body>");
    lot52_render(html, 320);
    TEST_ASSERT_EQUAL(1, l61_diag_count("alpha display budget"));
    TEST_ASSERT_EQUAL(18, (int)g_doc.nitems);               /* transparent item dropped */
    TEST_ASSERT_EQUAL(6, l62_items_with(0xFF0000));
    l62_no_alpha_left();
}

static void lot62_alpha_run(void) {
    RUN_TEST(test_l62_color_hex_alpha_forms);
    RUN_TEST(test_l62_color_rgb_functions);
    RUN_TEST(test_l62_color_hsl_functions);
    RUN_TEST(test_l62_color_function_refusals);
    RUN_TEST(test_l62_color_function_in_border_shorthand);
    RUN_TEST(test_l62_alpha_splits_along_the_backdrop);
    RUN_TEST(test_l62_alpha_over_border_sides);
    RUN_TEST(test_l62_alpha_over_text_repaints_the_glyphs);
    RUN_TEST(test_l62_alpha_text_colour);
    RUN_TEST(test_l62_opacity_group_is_one_layer);
    RUN_TEST(test_l62_opacity_nested_groups);
    RUN_TEST(test_l62_opacity_group_over_earlier_page_paint);
    RUN_TEST(test_l62_alpha_zero_paint_is_dropped);
    RUN_TEST(test_l62_alpha_budget_keeps_the_list_opaque);
}
