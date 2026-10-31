/* Lot 64 F: multi-column layout subset. */

static void test_l64_columns_balance_lines_and_boxes(void) {
    lot52_render(L64_BODY "<div id=m style='width:100px;column-count:2;column-gap:10px;column-rule:2px solid #ff0000'>aa bb cc dd ee ff gg"
                 "<p id=p style='margin:0;height:12px;background:#0000ff'></p>hh</div><div id=n>after</div></body>", 320);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
    /* columns of 45: three lines in the first, the rest (32 px) in the second */
    l61_text_at("aa bb", 0, 1);
    l61_text_at("ee ff", 0, 21);
    l61_text_at("gg", 55, 1);
    l64_box("p", 55, 10, 45, 12);
    l61_rect_at(55, 10, 45, 12, 0x0000FF);
    l61_text_at("hh", 55, 23);
    l64_box("m", 0, 0, 100, 32);
    l61_rect_at(49, 0, 2, 32, 0xFF0000);                      /* the rule in the middle of the gap */
    l61_text_at("after", 0, 33);
    TEST_ASSERT_EQUAL(8, (int)g_doc.nitems);
    /* lines of an inline element split like the text around them */
    lot52_render(L64_BODY "<div style='width:100px;column-count:2'>aa <a id=l href=/x>bb cc</a> dd</div></body>", 320);
    l61_text_at("aa", 0, 1);
    l61_text_at("bb", 24, 1);
    l61_text_at("cc", 54, 1);
    l61_text_at("dd", 78, 1);
    TEST_ASSERT_EQUAL(10, g_doc.perf.page_h);
}

static void test_l64_columns_boxes_whole(void) {
    lot52_render(L64_BODY "<div id=m style='width:90px;column-count:3;column-gap:0;column-rule:4px dashed'><div id=a style='height:20px;background:#ff0000'></div>"
                 "<div id=b style='height:20px;margin-top:4px;background:#00ff00'></div><div id=c style='height:20px;background:#0000ff'></div></div></body>", 320);
    l64_box("a", 0, 0, 30, 20);
    l64_box("b", 30, 4, 30, 20);                              /* its margin moves with it */
    l64_box("c", 60, 0, 30, 20);
    l64_box("m", 0, 0, 90, 24);
    l61_rect_at(28, 0, 4, 8, 0x000000);                       /* a dashed rule in currentColor, gap 0 */
    l61_rect_at(28, 16, 4, 8, 0x000000);
    l61_rect_at(58, 0, 4, 8, 0x000000);
    TEST_ASSERT_EQUAL(7, (int)g_doc.nitems);
    /* a box taller than the balance keeps the others after it */
    lot52_render(L64_BODY "<div style='width:90px;column-count:3;column-gap:0'><div id=a style='height:50px'></div>"
                 "<div id=b style='height:10px'></div><div id=c style='height:10px'></div></div></body>", 320);
    l64_box("a", 0, 0, 30, 50);
    l64_box("b", 30, 0, 30, 10);
    l64_box("c", 30, 10, 30, 10);
    /* nested content moves with its box */
    lot52_render(L64_BODY "<div style='width:100px;column-count:2;column-gap:0'><div id=a style='height:10px'></div>"
                 "<div id=b><p id=q style='margin:0'>xy</p></div></div></body>", 320);
    l64_box("b", 50, 0, 50, 10);
    l64_box("q", 50, 0, 50, 10);
    l61_text_at("xy", 50, 1);
}

static void test_l64_columns_width_fill_and_overflow(void) {
    /* column-width: as many as fit, then wider */
    lot52_render(L64_BODY "<div style='width:100px;columns:30px;height:20px;column-fill:auto'>aa bb cc dd ee ff gg hh ii jj</div></body>", 320);
    l61_text_at("aa bb", 0, 1);
    l61_text_at("cc dd", 0, 11);
    l61_text_at("ee ff", 54, 1);
    l61_text_at("ii jj", 108, 1);                             /* an overflow column on the right */
    TEST_ASSERT_EQUAL(1, l61_diag_count("columns overflow"));
    /* balance with a definite height balances below it */
    lot52_render(L64_BODY "<div id=m style='width:100px;column-count:2;height:40px'>aa bb cc dd</div></body>", 320);
    l61_text_at("aa bb", 0, 1);
    l61_text_at("cc dd", 54, 1);
    l64_box("m", 0, 0, 100, 40);
    /* fill auto fills the height first */
    lot52_render(L64_BODY "<div style='width:100px;column-count:2;height:40px;column-fill:auto'>aa bb cc dd</div></body>", 320);
    l61_text_at("cc dd", 0, 11);
    /* count and width: the smaller count wins */
    lot52_render(L64_BODY "<div style='width:100px;columns:3 10px;column-gap:2px'>a b c d e f</div></body>", 320);
    l61_text_at("a b", 0, 1);
    l61_text_at("c d", 34, 1);
    l61_text_at("e f", 68, 1);
    /* too narrow for the count: fewer columns of at least one glyph */
    lot52_render(L64_BODY "<div style='width:20px;column-count:8;column-gap:4px'>a b</div></body>", 320);
    l61_text_at("a", 0, 1);
    l61_text_at("b", 12, 1);
    /* no content, and the count of one */
    lot52_render(L64_BODY "<div id=m style='width:20px;column-count:2;column-rule:1px solid red'></div>"
                 "<div id=k style='width:40px;column-count:1'>a b</div></body>", 320);
    l64_box("m", 0, 0, 20, 0);
    TEST_ASSERT_EQUAL(1, l64_text_items());
    l61_text_at("a b", 0, 1);
}

static void test_l64_columns_parse(void) {
    static const char *const bad[] = {"column-count:0", "column-count:9", "column-count:2.5", "column-width:-3px", "column-width:10%",
        "columns:2 3", "columns:10px 20px", "columns:2 10px 3", "column-fill:balance-all", "column-rule:2px solid solid",
        "column-rule-style:wavy", "column-rule-width:huge", "column-rule-color:nope", 0};
    char html[200];
    int k;
    for (k = 0; bad[k]; k++) {
        snprintf(html, sizeof(html), L64_BODY "<div id=m style='%s'>a</div></body>", bad[k]);
        lot52_render(html, 320);
        TEST_ASSERT_EQUAL(1, l61_diag_count("columns refused") + l61_diag_count("column rule refused"));
        TEST_ASSERT_EQUAL(0, lot52_node("m")->col_count);
        TEST_ASSERT_EQUAL(-1, lot52_node("m")->col_width);
        TEST_ASSERT_EQUAL(0, lot52_node("m")->rule_style);
    }
    lot52_render(L64_BODY "<div id=m style='columns:auto 12px;column-rule:dotted #00ff00 thin;column-fill:auto'>a</div>"
                 "<div id=n style='columns:4;column-count:auto;column-rule-style:double;column-rule-width:thick;column-rule-color:#0000ff'>a</div>"
                 "<div id=o style='columns:auto auto;column-span:none'>a</div><div id=p style='column-span:all;column-span:2'>a</div></body>", 320);
    TEST_ASSERT_EQUAL(1, lot52_node("p")->col_span_all);
    TEST_ASSERT_EQUAL(0, lot52_node("m")->col_count);
    TEST_ASSERT_EQUAL(12, lot52_node("m")->col_width);
    TEST_ASSERT_EQUAL(1, lot52_node("m")->col_fill_auto);
    TEST_ASSERT_EQUAL(6, lot52_node("m")->rule_style);
    TEST_ASSERT_EQUAL(1, lot52_node("m")->rule_w);
    TEST_ASSERT_EQUAL(0x00FF00, lot52_node("m")->rule_color);
    TEST_ASSERT_EQUAL(1, lot52_node("m")->rule_color_set);
    TEST_ASSERT_EQUAL(0, lot52_node("n")->col_count);
    TEST_ASSERT_EQUAL(4, lot52_node("n")->rule_style);
    TEST_ASSERT_EQUAL(5, lot52_node("n")->rule_w);
    TEST_ASSERT_EQUAL(0x0000FF, lot52_node("n")->rule_color);
    TEST_ASSERT_EQUAL(-1, lot52_node("o")->col_width);
    TEST_ASSERT_EQUAL(1, l61_diag_count("column span refused"));
    TEST_ASSERT_EQUAL(1, (int)g_doc.ndiag);
    /* not inherited; flex, grid and vertical flows ignore the columns */
    lot52_render(L64_BODY "<div style='column-count:2;width:100px'><div id=c style='width:100px'>a b</div></div>"
                 "<div id=f style='display:flex;column-count:2;width:100px'><div>x y</div></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("c")->col_count);
    l61_text_at("a b", 0, 1);
    l61_text_at("x y", 0, 11);
    lot52_render(L64_BODY "<div style='writing-mode:vertical-rl;height:16px;column-count:2'>ab</div></body>", 320);
    l64_glyph('a', 1, 0);
    l64_glyph('b', 1, 8);
}

static void lot64_columns_run(void) {
    RUN_TEST(test_l64_columns_balance_lines_and_boxes);
    RUN_TEST(test_l64_columns_boxes_whole);
    RUN_TEST(test_l64_columns_width_fill_and_overflow);
    RUN_TEST(test_l64_columns_parse);
}
