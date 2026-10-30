/* Lot 62: paint-time transform (translate, scale, quarter-turn rotate)
 * about transform-origin. Layout boxes stay where they are. */
#define L62_BOX "width:20px;height:10px;background:#ff0000"

static void l62_one_rect(int x, int y, int w, int h) {
    TEST_ASSERT_EQUAL(1, l61_count(OS_FB_WEB_RECT));
    l61_rect_is(0xFF0000, x, y, w, h);
}

static void test_l62_transform_translate_moves_paint_only(void) {
    lot52_render(L62_BODY "<div id=a style='" L62_BOX ";transform:translate(5px, 3px)'></div>"
                 "<div id=b style='height:4px'></div></body>", 320);
    l62_one_rect(5, 3, 20, 10);
    l61_box("a", 0, 0, 20, 10);                       /* layout unchanged */
    l61_box("b", 0, 10, 320, 4);
    lot52_render(L62_BODY "<div style='" L62_BOX ";transform:translate(50%, -100%)'></div></body>", 320);
    l62_one_rect(10, -10, 20, 10);
    lot52_render(L62_BODY "<div style='" L62_BOX ";transform:translateY(2px) translateX(-3px)'></div></body>", 320);
    l62_one_rect(-3, 2, 20, 10);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l62_transform_list_order(void) {
    /* About the centre (10, 5): T(10) S(2) maps x to 2x, S(2) T(10) to 2x + 10. */
    lot52_render(L62_BODY "<div style='" L62_BOX ";transform:translateX(10px) scale(2)'></div></body>", 320);
    l62_one_rect(0, -5, 40, 20);
    lot52_render(L62_BODY "<div style='" L62_BOX ";transform:scale(2) translateX(10px)'></div></body>", 320);
    l62_one_rect(10, -5, 40, 20);
    lot52_render(L62_BODY "<div style='" L62_BOX ";transform:scale(2, 50%)'></div></body>", 320);
    l62_one_rect(-10, 3, 40, 5);
    lot52_render(L62_BODY "<div style='" L62_BOX ";transform:scaleY(3) scaleX(.5)'></div></body>", 320);
    l62_one_rect(5, -10, 10, 30);
}

/* 7.5 px rounds half up to 8 */
static void test_l62_transform_rounds_half_up(void) {
    lot52_render(L62_BODY "<div style='width:5px;height:2px;background:#ff0000;transform-origin:0 0;transform:scale(1.5)'></div></body>", 320);
    l61_rect_at(0, 0, 8, 3, 0xFF0000);
}

static void test_l62_transform_origin(void) {
    lot52_render(L62_BODY "<div style='" L62_BOX ";transform:scale(.5);transform-origin:left top'></div></body>", 320);
    l62_one_rect(0, 0, 10, 5);
    lot52_render(L62_BODY "<div style='" L62_BOX ";transform:scale(.5);transform-origin:100% 0'></div></body>", 320);
    l62_one_rect(10, 0, 10, 5);
    lot52_render(L62_BODY "<div style='" L62_BOX ";transform:scale(.5);transform-origin:bottom'></div></body>", 320);
    l62_one_rect(5, 5, 10, 5);
    lot52_render(L62_BODY "<div style='" L62_BOX ";transform:scale(.5);transform-origin:top right'></div></body>", 320);
    l62_one_rect(10, 0, 10, 5);                       /* keywords swap: x right, y top */
    lot52_render(L62_BODY "<div style='" L62_BOX ";transform:scale(.5);transform-origin:4px 2px'></div></body>", 320);
    l62_one_rect(2, 1, 10, 5);
    lot52_render(L62_BODY "<div style='" L62_BOX ";transform:scale(.5);transform-origin:left left'></div>"
                 "<div style='transform-origin:10px top 3px'></div><div style='transform-origin:top 10%'></div></body>", 320);
    l62_one_rect(5, 3, 10, 5);                        /* refused: the centre stays */
    TEST_ASSERT_EQUAL(3, l61_diag_count("transform origin refused"));   /* a length after top is invalid */
}

static void test_l62_transform_quarter_turns(void) {
    lot52_render(L62_BODY "<div style='" L62_BOX ";transform:rotate(90deg)'></div></body>", 320);
    l62_one_rect(5, -5, 10, 20);
    lot52_render(L62_BODY "<div style='" L62_BOX ";transform:rotate(90deg);transform-origin:0 0'></div></body>", 320);
    l62_one_rect(-10, 0, 10, 20);
    lot52_render(L62_BODY "<div style='" L62_BOX ";transform:rotate(-90deg);transform-origin:0 0'></div></body>", 320);
    l62_one_rect(0, -20, 10, 20);
    lot52_render(L62_BODY "<div style='" L62_BOX ";transform:rotate(.75turn);transform-origin:0 0'></div></body>", 320);
    l62_one_rect(0, -20, 10, 20);
    lot52_render(L62_BODY "<div style='" L62_BOX ";transform:rotate(180deg);transform-origin:0 0'></div></body>", 320);
    l62_one_rect(-20, -10, 20, 10);
    lot52_render(L62_BODY "<div style='" L62_BOX ";transform:rotate(0)'></div></body>", 320);
    l62_one_rect(0, 0, 20, 10);
}

static void test_l62_transform_refusals_are_atomic(void) {
    lot52_render(L62_BODY "<div id=a style='" L62_BOX ";transform:translate(4px);transform:rotate(45dag)'></div>"
                 "<div id=b style='transform:skewX(90deg)'></div><div id=c style='transform:translate(1px, 2px, 3px)'></div>"
                 "<div id=d style='transform:scale(9)'></div><div id=e style='transform:scale(1) scale(1) scale(1) scale(1) scale(1)'></div>"
                 "<div id=f style='transform:translateX(1px, 2px)'></div><div id=g style='transform:rotate(90)'></div>"
                 "<div id=h style='transform:translate(auto)'></div><div id=i style='transform:scale(2deg)'></div></body>", 320);
    l62_one_rect(4, 0, 20, 10);                       /* the earlier translate stays */
    TEST_ASSERT_EQUAL(1, lot52_node("a")->tf_count);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->tf_count);
    TEST_ASSERT_EQUAL(0, lot52_node("e")->tf_count);
    TEST_ASSERT_EQUAL(8, (int)g_doc.ndiag);
    TEST_ASSERT_EQUAL(8, l61_diag_count("transform refused"));
    lot52_render(L62_BODY "<div id=a style='" L62_BOX ";transform:translate(4px);transform:none'></div></body>", 320);
    l62_one_rect(0, 0, 20, 10);
}

static void test_l62_transform_scaled_text_becomes_glyph_runs(void) {
    const os_fb_web_item_t *t;
    int x, y;
    lot52_render(L62_BODY "<p id=p style='margin:0;width:16px;transform:translate(3px, 1px)'>H</p></body>", 320);
    t = find_text("H");
    TEST_ASSERT_NOT_NULL(t);
    if (!t) return;
    x = t->x; y = t->y;                               /* translate keeps TEXT */
    TEST_ASSERT_EQUAL(3, x - lot52_node("p")->x);
    lot52_render(L62_BODY "<p id=p style='margin:0;width:16px;transform:scale(2);transform-origin:0 0'>H</p></body>", 320);
    TEST_ASSERT_NULL(find_text("H"));
    TEST_ASSERT_EQUAL(0, l61_count(OS_FB_WEB_TEXT));
    y -= 1;
    x -= 3;
    L62_HEX(0x000000, l62_pixel(2 * x + 2, 2 * y));   /* column 1, row 0 doubled */
    L62_HEX(0x000000, l62_pixel(2 * x + 3, 2 * y + 1));
    L62_HEX(0xFFFFFF, l62_pixel(2 * x + 6, 2 * y));   /* column 3 stays empty */
    L62_HEX(0x000000, l62_pixel(2 * x + 7, 2 * y + 7)); /* row 3 is the bar */
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l62_transform_mirrored_text(void) {
    const os_fb_web_item_t *t;
    int x, y;
    lot52_render(L62_BODY "<p style='margin:0;width:8px'>L</p></body>", 320);
    t = find_text("L");
    TEST_ASSERT_NOT_NULL(t);
    if (!t) return;
    x = t->x; y = t->y;
    L62_HEX(0x000000, l62_pixel(x + 1, y));           /* the stem of L is on the left */
    lot52_render(L62_BODY "<p style='margin:0;width:8px;transform:scaleX(-1)'>L</p></body>", 320);
    L62_HEX(0xFFFFFF, l62_pixel(x + 1, y));
    L62_HEX(0x000000, l62_pixel(x + 6, y));           /* mirrored about the centre */
}

static void test_l62_transform_border_and_zero_scale(void) {
    lot52_render(L62_BODY "<div style='border:1px solid #0000ff;width:8px;height:8px;transform:scale(2);transform-origin:0 0'></div></body>", 320);
    TEST_ASSERT_EQUAL(0, l61_count(OS_FB_WEB_BORDER));
    l61_rect_at(0, 0, 20, 2, 0x0000FF);
    l61_rect_at(18, 2, 2, 16, 0x0000FF);
    lot52_render(L62_BODY "<div style='" L62_BOX ";transform:scale(0)'></div></body>", 320);
    TEST_ASSERT_EQUAL(0, (int)g_doc.nitems);
}

static void test_l62_transform_nested_and_clipped(void) {
    lot52_render(L62_BODY "<div style='width:20px;transform:translateX(10px)'>"
                 "<div style='" L62_BOX ";transform:scale(2);transform-origin:0 0'></div></div></body>", 320);
    l62_one_rect(10, 0, 40, 20);
    /* A clip inside the transformed box moves with it. */
    lot52_render(L62_BODY "<div style='width:20px;height:10px;overflow:hidden;transform:translate(5px, 5px)'>"
                 "<div style='width:40px;height:20px;background:#ff0000'></div></div></body>", 320);
    l62_one_rect(5, 5, 20, 10);
    /* A clip outside it cuts the moved paint. */
    lot52_render(L62_BODY "<div style='width:20px;height:10px;overflow:hidden'>"
                 "<div style='" L62_BOX ";transform:translateX(15px)'></div></div></body>", 320);
    l62_one_rect(15, 0, 5, 10);
}

static void test_l62_transform_forms_context_and_containing_block(void) {
    lot52_render(L62_BODY "<div style='" L62_BOX ";transform:translate(0)'></div>"
                 "<div style='margin-top:-5px;width:20px;height:10px;background:#0000ff'></div></body>", 320);
    L62_HEX(0xFF0000, l62_pixel(5, 7));                /* z 0 context over later flow */
    lot52_render(L62_BODY "<div style='width:20px;height:10px;background:#ff0000'></div>"
                 "<div style='margin-top:-5px;width:20px;height:10px;background:#0000ff'></div></body>", 320);
    L62_HEX(0x0000FF, l62_pixel(5, 7));
    lot52_render(L62_BODY "<div style='margin-left:30px;height:20px;transform:translateX(0)'>"
                 "<div id=c style='position:absolute;left:5px;top:2px;width:4px;height:4px'></div></div></body>", 320);
    l61_box("c", 35, 2, 4, 4);
}

static void test_l62_transform_and_alpha(void) {
    lot52_render(L62_BODY "<div style='width:20px;height:10px;background:#0000ff'></div>"
                 "<div style='margin-top:-10px;width:20px;height:10px;background:rgba(255,0,0,.5);transform:translateX(10px)'></div></body>", 320);
    L62_HEX(0x80007F, l62_pixel(15, 5));               /* moved half over blue */
    L62_HEX(0xFF7F7F, l62_pixel(25, 5));
    L62_HEX(0x0000FF, l62_pixel(5, 5));
}

static void test_l62_transform_budget(void) {
    lot52_render(L62_BODY "<p style='margin:0;transform:scale(2)'>WWWWWWWWWWWWWWWW</p></body>", 640);
    TEST_ASSERT_EQUAL(1, l61_diag_count("transform display budget"));
    TEST_ASSERT_EQUAL(1, (int)g_doc.perf.items_dropped);
    TEST_ASSERT_EQUAL(0, (int)g_doc.nitems);
}

static void lot62_transform_run(void) {
    RUN_TEST(test_l62_transform_translate_moves_paint_only);
    RUN_TEST(test_l62_transform_list_order);
    RUN_TEST(test_l62_transform_origin);
    RUN_TEST(test_l62_transform_rounds_half_up);
    RUN_TEST(test_l62_transform_quarter_turns);
    RUN_TEST(test_l62_transform_refusals_are_atomic);
    RUN_TEST(test_l62_transform_scaled_text_becomes_glyph_runs);
    RUN_TEST(test_l62_transform_mirrored_text);
    RUN_TEST(test_l62_transform_border_and_zero_scale);
    RUN_TEST(test_l62_transform_nested_and_clipped);
    RUN_TEST(test_l62_transform_forms_context_and_containing_block);
    RUN_TEST(test_l62_transform_and_alpha);
    RUN_TEST(test_l62_transform_budget);
}
