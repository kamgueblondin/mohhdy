/* Lot 64 B: direction rtl subset (alignment, block placement, flex, grid
 * and table mirroring). No bidi reordering: text stays left to right. */

static void test_l64_rtl_text_align_start_end(void) {
    lot52_render(L64_BODY "<div style='direction:rtl;width:100px'>"
                 "<p style='margin:0;text-align:justify;width:40px'>ab cd ef</p>"
                 "<p style='margin:0;text-align:start;text-align-last:end'>q</p>"
                 "<p style='margin:0;text-align:center'>c</p>"
                 "<p style='margin:0;text-align:left'>l</p>"
                 "<p style='margin:0;direction:ltr'>s</p>"
                 "<p style='margin:0;text-align:end'>e</p></div></body>", 320);
    l61_text_at("ab cd", 60, 1);
    l61_text_at("ef", 84, 11);                                /* the last line falls back to start: right */
    l61_text_at("q", 0, 21);                                  /* text-align-last:end is left */
    l61_text_at("c", 46, 31);
    l61_text_at("l", 0, 41);
    l61_text_at("s", 0, 51);                                  /* start resolved per element */
    l61_text_at("e", 0, 61);
    /* initial alignment in rtl is right; inherited start follows the child */
    lot52_render(L64_BODY "<div style='direction:rtl;width:100px'>ab<p id=p style='margin:0;direction:ltr'>cd</p></div></body>", 320);
    l61_text_at("ab", 84, 1);
    l61_text_at("cd", 0, 11);
    /* a physical value given to the parent is inherited as is */
    lot52_render(L64_BODY "<div style='text-align:right;width:100px'><p style='margin:0;direction:rtl'>ab</p>"
                 "<p style='margin:0;direction:rtl;text-align-last:start;text-align:left'>cd</p></div></body>", 320);
    l61_text_at("ab", 84, 1);
    l61_text_at("cd", 84, 11);                               /* one line: the last, start is right */
    /* justify-none falls back to start too */
    lot52_render(L64_BODY "<div style='direction:rtl;width:40px;text-align:justify;text-justify:none'>ab c de</div></body>", 320);
    l61_text_at("ab c", 8, 1);
}

static void test_l64_rtl_block_placement(void) {
    lot52_render(L64_BODY "<div style='direction:rtl;width:100px'>"
                 "<div id=c style='margin-inline-start:5px;width:20px;height:2px;background:#ff0000'></div>"
                 "<div id=m style='margin-left:auto;margin-right:auto;width:20px;height:2px'></div>"
                 "<div id=r style='margin-right:auto;width:20px;height:2px'></div>"
                 "<div id=l style='margin-left:auto;width:20px;height:2px'></div>"
                 "<div id=w style='width:120px;height:2px'></div></div>"
                 "<div style='width:100px'><div id=t style='width:20px;height:2px'></div></div></body>", 320);
    l64_box("c", 75, 0, 20, 2);                               /* margin-inline-start is the right margin */
    TEST_ASSERT_EQUAL(5, lot52_node("c")->margin[1]);
    l61_rect_at(75, 0, 20, 2, 0xFF0000);
    TEST_ASSERT_EQUAL(40, lot52_node("m")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("r")->x);
    TEST_ASSERT_EQUAL(80, lot52_node("l")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("w")->x);                 /* overflowing: kept at the left edge */
    TEST_ASSERT_EQUAL(0, lot52_node("t")->x);                 /* ltr unchanged */
    /* the containing block's direction decides, not the box's own */
    lot52_render(L64_BODY "<div style='width:100px'><div id=o style='direction:rtl;width:20px;height:2px'></div></div>"
                 "<div style='direction:rtl;width:100px'><div id=i style='direction:ltr;width:20px;height:2px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("o")->x);
    TEST_ASSERT_EQUAL(80, lot52_node("i")->x);
}

static void test_l64_rtl_flex_grid_table(void) {
    lot52_render(L64_BODY "<div id=f style='direction:rtl;display:flex;width:100px'><div id=a style='width:20px;height:4px;background:#ff0000'>a</div>"
                 "<div id=b style='width:30px;height:4px;background:#00ff00'></div></div></body>", 320);
    l64_box("a", 80, 0, 20, 4);
    l64_box("b", 20, 0, 30, 4);
    l61_rect_at(80, 0, 20, 4, 0xFF0000);
    l61_rect_at(20, 0, 30, 4, 0x00FF00);
    l61_text_at("a", 92, 1);                                  /* moved with its item, right-aligned inside */
    lot52_render(L64_BODY "<div style='direction:ltr;display:flex;width:100px'><div id=a style='width:20px;height:4px'></div>"
                 "<div id=b style='width:30px;height:4px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(50, lot52_node("b")->x);
    /* row-reverse in rtl runs left to right; an absolute child is not mirrored */
    lot52_render(L64_BODY "<div style='direction:rtl;display:flex;flex-direction:row-reverse;width:100px;position:relative'>"
                 "<div id=a style='width:20px;height:4px'></div><div id=b style='width:30px;height:4px'></div>"
                 "<div id=z style='position:absolute;left:3px;top:0;width:5px;height:5px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(3, lot52_node("z")->x);
    lot52_render(L64_BODY "<div style='direction:rtl;display:grid;grid-template-columns:30px 40px;width:100px'><div id=g1>1</div><div id=g2>2</div></div></body>", 320);
    l64_box("g1", 70, 0, 30, 10);
    l64_box("g2", 30, 0, 40, 10);
    l61_text_at("1", 92, 1);
    l61_text_at("2", 62, 1);
    lot52_render(L64_BODY "<table style='direction:rtl;width:100px;border-spacing:0'><tbody><tr><td id=c1 style='width:30px;border:1px solid #000'>1</td>"
                 "<td id=c2>2</td></tr></tbody></table></body>", 320);
    l64_box("c1", 68, 0, 32, 12);
    l64_box("c2", 0, 0, 68, 10);
    l61_text_at("1", 91, 2);
    l61_text_at("2", 60, 1);
    /* a nested table is mirrored once, by itself */
    lot52_render(L64_BODY "<table style='direction:rtl;width:100px;border-spacing:0'><tr><td style='width:60px'><table style='width:50px;border-spacing:0'>"
                 "<tr><td id=i1 style='width:20px'>1</td><td id=i2>2</td></tr></table></td><td>3</td></tr></table></body>", 320);
    l64_box("i1", 80, 0, 20, 10);
    l64_box("i2", 50, 0, 30, 10);
}

static void test_l64_rtl_vertical_inline_end(void) {
    /* in a vertical flow rtl lines start at the bottom (glyph order kept) */
    lot52_render(L64_BODY "<div style='direction:rtl;writing-mode:vertical-rl;height:40px'>ab</div></body>", 320);
    l64_glyph('a', 1, 24);
    l64_glyph('b', 1, 32);
    lot52_render(L64_BODY "<div style='writing-mode:vertical-rl;height:40px'>ab</div></body>", 320);
    l64_glyph('a', 1, 0);
}

static void lot64_rtl_run(void) {
    RUN_TEST(test_l64_rtl_text_align_start_end);
    RUN_TEST(test_l64_rtl_block_placement);
    RUN_TEST(test_l64_rtl_flex_grid_table);
    RUN_TEST(test_l64_rtl_vertical_inline_end);
}
