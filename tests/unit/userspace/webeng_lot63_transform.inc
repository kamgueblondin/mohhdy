/* Lot 63: rotate by any whole degree, skew, matrices composed across
 * nested transforms, parallelograms painted as row bands. */
#define L63_BOX "width:20px;height:10px;background:#ff0000"

static void l63_item_is(int i, int x, int y, int w, int h) {
    TEST_ASSERT_TRUE(i < (int)g_doc.nitems);
    if (i >= (int)g_doc.nitems) return;
    TEST_ASSERT_EQUAL(OS_FB_WEB_RECT, g_doc.items[i].kind);
    TEST_ASSERT_EQUAL(x, g_doc.items[i].x);
    TEST_ASSERT_EQUAL(y, g_doc.items[i].y);
    TEST_ASSERT_EQUAL(w, g_doc.items[i].w);
    TEST_ASSERT_EQUAL(h, g_doc.items[i].h);
}

static const we_transform_t *l63_tf(const char *value) {
    static char html[160];
    snprintf(html, sizeof(html), "<body><div id=a style='transform:%s'></div></body>", value);
    lot52_render(html, 320);
    return lot52_node("a")->tf_count == 1 ? &lot52_node("a")->tf[0] : 0;
}

static void test_l63_transform_angle_units(void) {
    static const struct { const char *v; int kind, x, y; } ok[] = {
        {"rotate(30deg)", WE_TF_ROTATE, 30, 0}, {"rotate(-30deg)", WE_TF_ROTATE, 330, 0}, {"rotate(.25turn)", WE_TF_ROTATE, 90, 0},
        {"rotate(100grad)", WE_TF_ROTATE, 90, 0}, {"rotate(1rad)", WE_TF_ROTATE, 57, 0}, {"rotate(3.1416rad)", WE_TF_ROTATE, 180, 0},
        {"rotate(30.5deg)", WE_TF_ROTATE, 31, 0}, {"rotate(-30.5deg)", WE_TF_ROTATE, 329, 0}, {"rotate(30.4deg)", WE_TF_ROTATE, 30, 0},
        {"rotate(720deg)", WE_TF_ROTATE, 0, 0}, {"rotate(-450deg)", WE_TF_ROTATE, 270, 0}, {"rotate(5000turn)", WE_TF_ROTATE, 0, 0},
        {"rotate(0)", WE_TF_ROTATE, 0, 0}, {"ROTATE(45DEG)", WE_TF_ROTATE, 45, 0},
        {"skewX(10deg)", WE_TF_SKEW, 10, 0}, {"skewY(-20deg)", WE_TF_SKEW, 0, -20}, {"skew(10deg, 20deg)", WE_TF_SKEW, 10, 20},
        {"skew(15deg)", WE_TF_SKEW, 15, 0}, {"skewX(170deg)", WE_TF_SKEW, -10, 0}, {"skewX(80deg)", WE_TF_SKEW, 80, 0},
        {"skewX(-80deg)", WE_TF_SKEW, -80, 0}, {"skewX(100deg)", WE_TF_SKEW, -80, 0}, {"skewX(-100deg)", WE_TF_SKEW, 80, 0},
        {"skewY(-190deg)", WE_TF_SKEW, 0, -10}, {"skewX(0)", WE_TF_SKEW, 0, 0}, {"skewX(370deg)", WE_TF_SKEW, 10, 0},
        {"skewY(-370deg)", WE_TF_SKEW, 0, -10}};
    unsigned k;
    for (k = 0; k < sizeof(ok) / sizeof(ok[0]); k++) {
        const we_transform_t *t = l63_tf(ok[k].v);
        L63_CASE(t && t->kind == ok[k].kind && t->x == ok[k].x && t->y == ok[k].y, ok[k].v);
        L63_CASE(0 == g_doc.ndiag, ok[k].v);
    }
}

static void test_l63_transform_angle_refusals(void) {
    static const char *const bad[] = { "rotate(30)", "rotate(10deg, 20deg)", "skewX(1deg, 2deg)", "skewY(1deg, 2deg)", "rotate(10%)",
        "rotate(1degdeg)", "rotate(deg)", "rotate(1.deg)", "rotate(100000turn)", "rotate(1e2deg)", "skewX(90deg)", "skewX(81deg)",
        "skewX(-81deg)", "skewY(270deg)", "skew(10deg, 90deg)", "skew(1px)", "skew()", "skew(1deg, 2deg, 3deg)", "rotate(1 deg)" };
    unsigned k;
    for (k = 0; k < sizeof(bad) / sizeof(bad[0]); k++) {
        L63_CASE(0 == l63_tf(bad[k]), bad[k]);
        L63_CASE(1 == l61_diag_count("transform refused"), bad[k]);
    }
}

/* linear-gradient angles share the parser: quarter turns only */
static void test_l63_transform_gradient_angles(void) {
    lot52_render("<body><div id=a style='height:4px;background:linear-gradient(100grad, #ff0000, #0000ff)'></div>"
                 "<div id=b style='height:4px;background:linear-gradient(.5turn, #ff0000, #0000ff)'></div>"
                 "<div id=c style='height:4px;background:linear-gradient(-90deg, #ff0000, #0000ff)'></div>"
                 "<div id=d style='height:4px;background:linear-gradient(45deg, #ff0000, #0000ff)'></div></body>", 320);
    TEST_ASSERT_EQUAL(WE_GRAD_RIGHT, lot52_node("a")->grad_dir);
    TEST_ASSERT_EQUAL(WE_GRAD_BOTTOM, lot52_node("b")->grad_dir);
    TEST_ASSERT_EQUAL(WE_GRAD_LEFT, lot52_node("c")->grad_dir);
    TEST_ASSERT_EQUAL(0, lot52_node("d")->grad_dir);
    TEST_ASSERT_EQUAL(1, l61_diag_count("gradient refused"));
}

/* rotate(45deg) about 0 0: corners (0,0) (14.1,14.1) (7.1,21.2) (-7.1,7.1),
 * 21 rows in 8 bands, each spanning the shape at its middle row; corners
 * in half pixels keep a span within one pixel of the exact one. */
static void test_l63_transform_rotate_bands(void) {
    lot52_render(L62_BODY "<div style='" L63_BOX ";transform:rotate(45deg);transform-origin:0 0'></div></body>", 320);
    TEST_ASSERT_EQUAL(8, (int)g_doc.nitems);
    l63_item_is(0, -1, 0, 2, 2);                       /* rows 0-1 at y 1: x -1..1 */
    l63_item_is(1, -3, 2, 7, 3);                       /* rows 2-4 at y 3.5: x -3.5..3.5 */
    l63_item_is(3, -5, 7, 14, 3);                      /* rows 7-9 at y 8.5: x -5.6..8.5, corner (-7,7) */
    l63_item_is(7, 6, 18, 3, 3);                       /* rows 18-20 at y 19.5: x 5.4..8.8, corner (7,21) */
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
    lot52_render(L62_BODY "<div style='" L63_BOX ";transform:rotate(45deg)'></div></body>", 320);
    l63_item_is(0, 5, -5, 3, 2);                       /* about the centre (10,5) */
    l63_item_is(7, 13, 13, 2, 3);
    lot52_render(L62_BODY "<div style='" L63_BOX ";transform:rotate(90deg)'></div></body>", 320);
    TEST_ASSERT_EQUAL(1, (int)g_doc.nitems);           /* quarter turns stay one rectangle */
    l63_item_is(0, 5, -5, 10, 20);
    lot52_render(L62_BODY "<div style='width:4px;height:4px;background:#ff0000;transform:rotate(30deg)'></div></body>", 320);
    TEST_ASSERT_EQUAL(4, (int)g_doc.nitems);           /* y -0.7..4.7: rows 0-3, one band each */
    l63_item_is(0, 1, 0, 2, 1);                        /* x 0.6..3.4 at y 0.5 */
    l63_item_is(1, 0, 1, 5, 1);
}

/* skewX(45deg) about 0 0 shifts row y by y: 10 rows in 8 bands. */
static void test_l63_transform_skew_bands(void) {
    lot52_render(L62_BODY "<div style='" L63_BOX ";transform:skewX(45deg);transform-origin:0 0'></div></body>", 320);
    TEST_ASSERT_EQUAL(8, (int)g_doc.nitems);
    l63_item_is(0, 1, 0, 20, 1);
    l63_item_is(3, 4, 3, 20, 2);                       /* rows 3-4 merge in one band */
    l63_item_is(7, 9, 8, 20, 2);
    lot52_render(L62_BODY "<div style='" L63_BOX ";transform:skewY(45deg);transform-origin:0 0'></div></body>", 320);
    l63_item_is(0, 0, 0, 2, 3);                        /* 30 rows; the left edge stays at x 0 */
    l63_item_is(7, 18, 26, 2, 4);
    lot52_render(L62_BODY "<div style='" L63_BOX ";transform:skewX(-45deg);transform-origin:0 0'></div></body>", 320);
    l63_item_is(0, 0, 0, 20, 1);                       /* x 0.5 less: -0.5 rounds up to 0 */
    l63_item_is(7, -9, 8, 20, 2);
}

/* the first edge met is not always the right side; a horizontal edge at
 * a band middle is skipped (a band of half a row stays unpainted) */
static void test_l63_transform_band_edges(void) {
    lot52_render(L62_BODY "<div style='" L63_BOX ";transform:rotate(-45deg);transform-origin:0 0'></div></body>", 320);
    TEST_ASSERT_EQUAL(8, (int)g_doc.nitems);
    l63_item_is(0, 13, -14, 2, 2);                     /* y -13: x 13..15.2 */
    l63_item_is(3, 6, -7, 14, 3);
    l63_item_is(7, 6, 4, 3, 3);
    lot52_render(L62_BODY "<div style='width:20px;height:5px;background:#ff0000;transform:skewX(45deg) scaleY(1.5);transform-origin:0 0'></div></body>", 320);
    TEST_ASSERT_EQUAL(7, (int)g_doc.nitems);           /* rows 0-6; row 7 holds half a pixel */
    l63_item_is(6, 7, 6, 20, 1);
    /* equal neighbouring bands merge */
    lot52_render(L62_BODY "<div style='width:60px;height:4px;background:#ff0000;transform:rotate(1deg);transform-origin:0 0'></div></body>", 320);
    TEST_ASSERT_EQUAL(3, (int)g_doc.nitems);
    l63_item_is(1, 0, 1, 60, 3);
}

/* nested transforms with no clip between them compose before banding */
static void test_l63_transform_composed(void) {
    lot52_render(L62_BODY "<div style='width:30px;height:30px;transform:rotate(45deg)'>"
                 "<div style='width:30px;height:30px;transform:rotate(-45deg);background:#ff0000'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(1, (int)g_doc.nitems);
    l63_item_is(0, 0, 0, 30, 30);
    lot52_render(L62_BODY "<div style='width:30px;height:30px;transform:skewX(30deg)'>"
                 "<div style='width:30px;height:30px;transform:skewX(-30deg);background:#ff0000'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(1, (int)g_doc.nitems);
    l63_item_is(0, 0, 0, 30, 30);
    /* the order matters: rotate(90deg) after skewX(45deg) maps (x, y) to (-y, x + y) */
    lot52_render(L62_BODY "<div style='width:20px;height:10px;transform:rotate(90deg);transform-origin:0 0'>"
                 "<div style='" L63_BOX ";transform:skewX(45deg);transform-origin:0 0'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(6, (int)g_doc.nitems);
    l63_item_is(0, -1, 0, 1, 3);                       /* y 1.5: x -1.5..0 */
    l63_item_is(3, -10, 11, 10, 11);                   /* the full width between y 10 and 20 */
    lot52_render(L62_BODY "<div style='width:20px;transform:scale(.5);transform-origin:0 0'>"
                 "<div style='" L63_BOX ";transform:translateX(10px) scale(2);transform-origin:0 0'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(1, (int)g_doc.nitems);           /* (x/2) o (2x + 10) = x + 5 */
    l63_item_is(0, 5, 0, 20, 10);
}

/* a clip between two transforms splits them: inner bands, clip, outer bands */
static void test_l63_transform_clip_between(void) {
    lot52_render(L62_BODY "<div style='width:20px;height:20px;overflow:hidden'>"
                 "<div style='width:20px;height:20px;background:#ff0000;transform:rotate(45deg)'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(6, (int)g_doc.nitems);
    l63_item_is(0, 5, 0, 10, 3);                       /* the diamond's top cut at y 0 */
    l63_item_is(2, 0, 6, 20, 4);                       /* and its sides at x 0 and 20 */
    l63_item_is(5, 5, 17, 11, 3);
    lot52_render(L62_BODY "<div style='width:100px;height:100px;overflow:hidden;transform:rotate(30deg)'>"
                 "<div style='width:60px;height:60px;border:10px solid #0000ff;transform:rotate(30deg)'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(0, (int)g_doc.nitems);           /* 4 sides, 8 bands each, banded again */
    TEST_ASSERT_EQUAL(1, l61_diag_count("transform display budget"));
}

static void test_l63_transform_text_upright(void) {
    lot52_render(L62_BODY "<p style='margin:0;width:16px;transform:rotate(30deg)'>Hi</p></body>", 320);
    l61_text_at("Hi", 0, 1);                           /* the centre maps to itself */
    TEST_ASSERT_EQUAL(1, l61_diag_count("transform text upright"));
    lot52_render(L62_BODY "<p style='margin:0;width:16px;transform:rotate(30deg);transform-origin:0 0'>Hi</p></body>", 320);
    l61_text_at("Hi", -4, 4);                          /* centre (8,5) to (4.4,8.3) */
    lot52_render(L62_BODY "<p style='margin:0;width:16px;transform:skewY(45deg);transform-origin:0 0'>Hi</p></body>", 320);
    l61_text_at("Hi", 0, 9);                           /* centre (8,5) to (8,13) */
    lot52_render(L62_BODY "<p style='margin:0;width:16px;transform:rotate(90deg)'>Hi</p></body>", 320);
    TEST_ASSERT_NULL(find_text("Hi"));                 /* a quarter turn still maps the glyphs */
    TEST_ASSERT_EQUAL(0, l61_diag_count("transform text upright"));
}

static void lot63_transform_run(void) {
    RUN_TEST(test_l63_transform_angle_units);
    RUN_TEST(test_l63_transform_angle_refusals);
    RUN_TEST(test_l63_transform_gradient_angles);
    RUN_TEST(test_l63_transform_rotate_bands);
    RUN_TEST(test_l63_transform_skew_bands);
    RUN_TEST(test_l63_transform_band_edges);
    RUN_TEST(test_l63_transform_composed);
    RUN_TEST(test_l63_transform_clip_between);
    RUN_TEST(test_l63_transform_text_upright);
}
