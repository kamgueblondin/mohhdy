/* Lot 73: multi-column fragmentation. A block child made of lines only
 * (nothing painted for the box) is cut between its lines; orphans and
 * widows (inherited, 2) keep lines together at a column bottom and top;
 * break-inside: avoid keeps the block whole; the box node follows its
 * first fragment. Columns of 45 px (width 100, count 2, gap 10). */
#define L73_COLS "<div id=m style='width:100px;column-count:2;column-gap:10px'>"

static void test_l73_paragraph_split_between_columns(void) {
    lot52_render(L64_BODY L73_COLS "<p id=p style='margin:0'>aa bb cc dd ee ff gg hh</p></div>"
                 "<div id=n>after</div></body>", 320);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
    l61_text_at("aa bb", 0, 1);
    l61_text_at("cc dd", 0, 11);
    l61_text_at("ee ff", 55, 1);
    l61_text_at("gg hh", 55, 11);
    l64_box("m", 0, 0, 100, 20);
    l61_text_at("after", 0, 21);
    l64_box("p", 0, 0, 45, 40);          /* the node keeps its first fragment place */
    /* six lines: 1-3 then 4-6 */
    lot52_render(L64_BODY L73_COLS "<p style='margin:0'>a1 a2 a3 a4 a5 a6 a7 a8 a9 b1 b2 b3</p></div></body>", 320);
    l61_text_at("a1 a2", 0, 1);
    l61_text_at("a5 a6", 0, 21);
    l61_text_at("a7 a8", 55, 1);
    l61_text_at("b2 b3", 55, 21);
    l64_box("m", 0, 0, 100, 30);
}

static void test_l73_orphans_and_widows(void) {
    static const char *five = "<p style='margin:0'>a1 a2 a3 a4 a5 a6 a7 a8 a9 b1</p></div></body>";
    char page[512];
    /* defaults (2 / 2): units 1-2, 3, 4-5: lines 1-3 then 4-5 */
    snprintf(page, sizeof(page), "%s%s%s", L64_BODY, L73_COLS, five);
    lot52_render(page, 320);
    l61_text_at("a5 a6", 0, 21);
    l61_text_at("a7 a8", 55, 1);
    l64_box("m", 0, 0, 100, 30);
    /* widows 3 on the container (inherited): lines 1-2 then 3-5 */
    snprintf(page, sizeof(page), "%s%s%s", L64_BODY, "<div id=m style='width:100px;column-count:2;column-gap:10px;widows:3'>", five);
    lot52_render(page, 320);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
    l61_text_at("a3 a4", 0, 11);
    l61_text_at("a5 a6", 55, 1);
    l61_text_at("a9 b1", 55, 21);
    l64_box("m", 0, 0, 100, 30);
    /* orphans 4, widows 1: lines 1-4 then 5 */
    snprintf(page, sizeof(page), "%s%s%s", L64_BODY, "<div id=m style='width:100px;column-count:2;column-gap:10px;orphans:4;widows:1'>", five);
    lot52_render(page, 320);
    l61_text_at("a7 a8", 0, 31);
    l61_text_at("a9 b1", 55, 1);
    l64_box("m", 0, 0, 100, 40);
    /* fewer lines than orphans + widows: the block stays whole */
    lot52_render(L64_BODY L73_COLS "<p style='margin:0'>a1 a2 a3 a4 a5 a6</p></div></body>", 320);
    l61_text_at("a1 a2", 0, 1);
    l61_text_at("a5 a6", 0, 21);
    l64_box("m", 0, 0, 100, 30);
    /* orphans 1 widows 1 cut three lines 2 + 1 */
    lot52_render(L64_BODY "<div id=m style='width:100px;column-count:2;column-gap:10px;orphans:1;widows:1'>"
                 "<p style='margin:0'>a1 a2 a3 a4 a5 a6</p></div></body>", 320);
    l61_text_at("a5 a6", 55, 1);
    l64_box("m", 0, 0, 100, 20);
}

static void test_l73_kept_whole(void) {
    /* break-inside avoid */
    lot52_render(L64_BODY L73_COLS "<p style='margin:0;break-inside:avoid'>aa bb cc dd ee ff gg hh</p></div></body>", 320);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
    l61_text_at("gg hh", 0, 31);
    l64_box("m", 0, 0, 100, 40);
    lot52_render(L64_BODY L73_COLS "<p style='margin:0;break-inside:avoid-column'>aa bb cc dd ee ff gg hh</p></div></body>", 320);
    l61_text_at("gg hh", 0, 31);
    /* a painted box (background) is not fragmented */
    lot52_render(L64_BODY L73_COLS "<p style='margin:0;background:#00ff00'>aa bb cc dd ee ff gg hh</p></div></body>", 320);
    l61_text_at("gg hh", 0, 31);
    /* nor a border */
    lot52_render(L64_BODY L73_COLS "<p style='margin:0;border-left:2px solid red'>aa bb cc dd ee ff gg hh</p></div></body>", 320);
    l61_text_at("aa bb", 2, 1);
    TEST_ASSERT_NOT_NULL(find_text("gg hh"));
    TEST_ASSERT_EQUAL(31, find_text("gg hh")->y);
    /* nor a block with a block inside */
    lot52_render(L64_BODY L73_COLS "<div style='margin:0'>aa bb cc dd<p style='margin:0'>ee ff gg hh</p></div></div></body>", 320);
    l61_text_at("gg hh", 0, 31);
    /* auto after avoid: cut again */
    lot52_render(L64_BODY L73_COLS "<p style='margin:0;break-inside:avoid;break-inside:auto'>aa bb cc dd ee ff gg hh</p></div></body>", 320);
    l61_text_at("ee ff", 55, 1);
}

static void test_l73_margins_and_inline_children(void) {
    /* the top margin goes with the first fragment, the bottom one with the last */
    lot52_render(L64_BODY L73_COLS "<p id=p style='margin:6px 0 4px 0'>aa bb cc dd ee ff gg hh</p></div></body>", 320);
    l61_text_at("aa bb", 0, 7);
    l61_text_at("cc dd", 0, 17);
    l61_text_at("ee ff", 55, 1);
    l61_text_at("gg hh", 55, 11);
    l64_box("m", 0, 0, 100, 26);
    /* inline elements inside the block move with their lines */
    lot52_render(L64_BODY L73_COLS "<p style='margin:0'>aa bb cc dd <b>ee ff</b> <a href=/x>gg hh</a></p></div></body>", 320);
    l61_text_at("ee ff", 55, 1);
    l61_text_at("gg hh", 55, 11);
    /* two paragraphs of one word per line (columns of 30): 4 + 8 lines in
     * three columns of 4: the second paragraph is cut in two */
    lot52_render(L64_BODY "<div id=m style='width:100px;column-count:3;column-gap:5px'><p style='margin:0'>a1 a2 a3 a4</p>"
                 "<p style='margin:0'>b1 b2 b3 b4 b5 b6 b7 b8</p></div></body>", 320);
    l61_text_at("a1", 0, 1);
    l61_text_at("a4", 0, 31);
    l61_text_at("b1", 35, 1);
    l61_text_at("b4", 35, 31);
    l61_text_at("b5", 70, 1);
    l61_text_at("b8", 70, 31);
    l64_box("m", 0, 0, 100, 40);
}

static void test_l73_forced_break_and_refusals(void) {
    /* break-before: column on a fragmented block opens a column */
    lot52_render(L64_BODY L73_COLS "<p style='margin:0'>x1</p><p style='margin:0;break-before:column'>aa bb cc dd ee ff gg hh</p></div></body>", 320);
    l61_text_at("x1", 0, 1);
    l61_text_at("aa bb", 55, 1);
    /* bad values are refused and the defaults stay */
    lot52_render(L64_BODY "<div id=m style='width:100px;column-count:2;column-gap:10px;orphans:0;widows:x;break-inside:page'>"
                 "<p style='margin:0'>a1 a2 a3 a4 a5 a6 a7 a8 a9 b1</p></div></body>", 320);
    TEST_ASSERT_EQUAL(3, l61_diag_count("break refused"));
    l61_text_at("a7 a8", 55, 1);
    lot52_render(L64_BODY "<div id=o style='orphans:100;widows:-1;orphans:3x'>a</div></body>", 320);
    TEST_ASSERT_EQUAL(3, l61_diag_count("break refused"));
    TEST_ASSERT_EQUAL(2, lot52_node("o")->orphans);
    lot52_render(L64_BODY "<div style='orphans:7;widows:5'><p id=q>a</p></div></body>", 320);
    TEST_ASSERT_EQUAL(7, lot52_node("q")->orphans);
    TEST_ASSERT_EQUAL(5, lot52_node("q")->widows);
}

static void lot73_run(void) {
    RUN_TEST(test_l73_paragraph_split_between_columns);
    RUN_TEST(test_l73_orphans_and_widows);
    RUN_TEST(test_l73_kept_whole);
    RUN_TEST(test_l73_margins_and_inline_children);
    RUN_TEST(test_l73_forced_break_and_refusals);
}
