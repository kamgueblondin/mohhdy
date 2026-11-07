/* Lot 81: painted object-position offsets, aspect-ratio auto on images,
 * floated/absolute inline-level grids and flexes. */
static void lot81_img(const char *style) {
    char html[400];
    snprintf(html, sizeof html, "<body style='margin:0'><img id=im src='/images/mire-g1.png' alt='f' style='%s'></body>", style);
    lot57_image(html);
}

static void test_l81_paint_aspect_auto_height(void) {
    lot81_img("height:20px;aspect-ratio:auto 1/1");
    TEST_ASSERT_EQUAL(30, lot52_node("im")->w);
    TEST_ASSERT_EQUAL(20, lot52_node("im")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l81_paint_aspect_auto_width(void) {
    lot81_img("width:30px;aspect-ratio:auto 1/1");
    TEST_ASSERT_EQUAL(30, lot52_node("im")->w);
    TEST_ASSERT_EQUAL(20, lot52_node("im")->h);
}

static void test_l81_paint_aspect_plain_height(void) {
    lot81_img("height:20px;aspect-ratio:1/1");
    TEST_ASSERT_EQUAL(20, lot52_node("im")->w);
    TEST_ASSERT_EQUAL(20, lot52_node("im")->h);
}

static void lot81_painted(const char *style, int x, int y, int want) {
    int painted = -1;
    lot81_img(style);
    (void)lot52_pixel(x, y, &painted);
    TEST_ASSERT_EQUAL(want, painted);
}

#define L81_NONE "width:60px;height:40px;object-fit:none;object-position:"
static void test_l81_paint_offset_left_edge(void) {
    lot81_painted(L81_NONE "left 5px top 3px", 4, 10, 0);
    lot81_painted(L81_NONE "left 5px top 3px", 5, 10, 1);
}
static void test_l81_paint_offset_top_edge(void) {
    lot81_painted(L81_NONE "left 5px top 3px", 10, 2, 0);
    lot81_painted(L81_NONE "left 5px top 3px", 10, 3, 1);
}
static void test_l81_paint_offset_right_edge(void) {
    /* 60 - 48 = 12 free columns: right 5px puts the image at x 7 */
    lot81_painted(L81_NONE "right 5px bottom 3px", 6, 20, 0);
    lot81_painted(L81_NONE "right 5px bottom 3px", 7, 20, 1);
    lot81_painted(L81_NONE "right 5px bottom 3px", 54, 20, 1);
    lot81_painted(L81_NONE "right 5px bottom 3px", 55, 20, 0);
}
static void test_l81_paint_offset_bottom_edge(void) {
    /* 40 - 32 = 8 free rows: bottom 3px puts the image at y 5 */
    lot81_painted(L81_NONE "right 5px bottom 3px", 20, 4, 0);
    lot81_painted(L81_NONE "right 5px bottom 3px", 20, 5, 1);
    lot81_painted(L81_NONE "right 5px bottom 3px", 20, 36, 1);
    lot81_painted(L81_NONE "right 5px bottom 3px", 20, 37, 0);
}
static void test_l81_paint_offset_lengths(void) {
    lot81_painted(L81_NONE "3px 4px", 2, 10, 0);
    lot81_painted(L81_NONE "3px 4px", 3, 10, 1);
    lot81_painted(L81_NONE "3px 4px", 10, 3, 0);
    lot81_painted(L81_NONE "3px 4px", 10, 4, 1);
}
static void test_l81_paint_offset_zero(void) {
    lot81_painted(L81_NONE "0 0", 0, 0, 1);
    lot81_painted(L81_NONE "0 0", 48, 10, 0);
}

static void test_l81_paint_abs_inline_grid(void) {
    lot52_render("<body style='margin:0'><div style='position:relative;width:200px;height:50px'>x<div id=a style='position:absolute;left:10px;top:20px;display:inline-grid;grid-template-columns:30px'><div style='height:5px'></div></div></div></body>", 480);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->w);
}
static void test_l81_paint_float_inline_flex(void) {
    lot52_render("<body style='margin:0'><div style='width:200px'><div id=a style='float:right;display:inline-flex'><div style='width:30px;height:5px'></div></div></div></body>", 480);
    TEST_ASSERT_EQUAL(170, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->w);
}

static void lot81_paint_run(void) {
    RUN_TEST(test_l81_paint_aspect_auto_height);
    RUN_TEST(test_l81_paint_aspect_auto_width);
    RUN_TEST(test_l81_paint_aspect_plain_height);
    RUN_TEST(test_l81_paint_offset_left_edge);
    RUN_TEST(test_l81_paint_offset_top_edge);
    RUN_TEST(test_l81_paint_offset_right_edge);
    RUN_TEST(test_l81_paint_offset_bottom_edge);
    RUN_TEST(test_l81_paint_offset_lengths);
    RUN_TEST(test_l81_paint_offset_zero);
    RUN_TEST(test_l81_paint_abs_inline_grid);
    RUN_TEST(test_l81_paint_float_inline_flex);
}
