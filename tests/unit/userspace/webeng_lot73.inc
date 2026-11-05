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
    /* the node of a fragmented block follows its first fragment when that
     * fragment moves (column 2 of 3 after the forced break) */
    lot52_render(L64_BODY "<div id=m style='width:100px;column-count:3;column-gap:5px'><p style='margin:0'>x1</p>"
                 "<p id=p style='margin:0;break-before:column'>b1 b2 b3 b4 b5 b6 b7 b8</p></div></body>", 320);
    l61_text_at("b1", 35, 1);
    l61_text_at("b5", 70, 1);
    l64_box("p", 35, 0, 30, 80);
    /* break-after goes with the last fragment: the next block opens column 3 */
    lot52_render(L64_BODY "<div id=m style='width:100px;column-count:3;column-gap:5px'>"
                 "<p id=p style='margin:0;break-after:column'>b1 b2 b3 b4 b5</p><p id=z style='margin:0'>z1</p></div></body>", 320);
    l61_text_at("b4", 35, 1);
    l61_text_at("b5", 35, 11);
    l61_text_at("z1", 70, 1);           /* without the break: 35, 21 under b5 */
    l64_box("z", 70, 0, 30, 10);
    l64_box("m", 0, 0, 100, 30);
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


/* Lot 73: line clamp. Lines past N are dropped, line N ends with "...",
 * the box ends at line N and the next block follows it. */
#define L73_CLAMP(n) "<p id=p style='margin:0;width:80px;display:-webkit-box;-webkit-box-orient:vertical;overflow:hidden;-webkit-line-clamp:" n "'>aa bb cc dd ee ff gg hh</p><div id=n>after</div></body>"

static void test_l73_line_clamp_two_lines(void) {
    lot52_render(L64_BODY L73_CLAMP("2"), 320);
    l61_text_at("aa bb cc", 0, 1);
    l61_text_at("dd ee...", 0, 11);   /* cut to the 80 px box: dd ee ff... does not fit */
    TEST_ASSERT_NULL(find_text("gg hh"));
    TEST_ASSERT_NULL(find_text("dd ee ff"));
    l64_box("p", 0, 0, 80, 20);
    l64_box("n", 0, 20, 320, 10);
    l61_text_at("after", 0, 21);
    TEST_ASSERT_TRUE(lot52_has_diag("line clamp"));
    TEST_ASSERT_FALSE(lot52_has_diag("display refused"));
}

static void test_l73_line_clamp_one_and_room(void) {
    lot52_render(L64_BODY L73_CLAMP("1"), 320);
    l61_text_at("aa bb...", 0, 1);
    TEST_ASSERT_NULL(find_text("dd ee ff"));
    l64_box("p", 0, 0, 80, 10);
    l61_text_at("after", 0, 11);
    /* room left on the last line: the dots are appended */
    lot52_render(L64_BODY "<p id=p style='margin:0;width:120px;line-clamp:1'>aa bb cc dd eeeeee ff</p></body>", 320);
    l61_text_at("aa bb cc dd...", 0, 1);   /* 11 chars, 15 fit */
    TEST_ASSERT_NULL(find_text("eeeeee ff"));
    /* exactly the room: 12 chars and the dots fill the 15 */
    lot52_render(L64_BODY "<p id=p style='margin:0;width:120px;line-clamp:1'>aa bb cc ddd eeeeee</p></body>", 320);
    l61_text_at("aa bb cc ddd...", 0, 1);
    l64_box("p", 0, 0, 120, 10);
}

static void test_l73_line_clamp_not_needed_and_nested(void) {
    /* enough lines: nothing changes, no diag */
    lot52_render(L64_BODY L73_CLAMP("3"), 320);
    l61_text_at("gg hh", 0, 21);
    l64_box("p", 0, 0, 80, 30);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
    lot52_render(L64_BODY L73_CLAMP("none"), 320);
    l64_box("p", 0, 0, 80, 30);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
    /* lines of nested blocks count in order */
    lot52_render(L64_BODY "<div id=p style='width:80px;line-clamp:2'><p style='margin:0'>one</p><p style='margin:0'>two</p>"
                 "<p style='margin:0'>three</p></div><div id=n>after</div></body>", 320);
    l61_text_at("one", 0, 1);
    l61_text_at("two...", 0, 11);
    TEST_ASSERT_NULL(find_text("three"));
    l64_box("p", 0, 0, 80, 20);
    l64_box("n", 0, 20, 320, 10);
}

static void test_l73_line_clamp_refusals_and_flow_root(void) {
    static const char *bad[] = { "0", "100", "2px", "-1", "x" };
    char page[512];
    int k;
    for (k = 0; k < 5; k++) {
        snprintf(page, sizeof(page), "%s<p id=p style='margin:0;width:80px;line-clamp:%s'>aa bb cc dd ee ff gg hh</p></body>", L64_BODY, bad[k]);
        lot52_render(page, 320);
        TEST_ASSERT_TRUE(lot52_has_diag("line clamp refused"));
        l64_box("p", 0, 0, 80, 30);
    }
    lot52_render(L64_BODY "<p style='-webkit-box-orient:diagonal'>x</p></body>", 320);
    TEST_ASSERT_TRUE(lot52_has_diag("box orient refused"));
    /* flow-root and -webkit-box are blocks, -webkit-inline-box an inline block */
    lot52_render(L64_BODY "<span id=p style='display:flow-root;width:40px'>a</span><span id=n style='display:-webkit-inline-box;width:40px'>b</span>"
                 "<span style='display:-webkit-inline-box;width:40px'>c</span></body>", 320);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
    l64_box("p", 0, 0, 40, 10);
    l61_text_at("b", 0, 11);
    l61_text_at("c", 40, 11);
}

/* Lot 73: list-style-position inside. The marker opens the first line in
 * the content box; the first line is indented by the marker and a space,
 * the next lines are not. */
static void test_l73_list_inside(void) {
    lot52_render(L64_BODY "<ul style='margin:0;padding:0 0 0 30px;width:110px;list-style-position:inside'><li id=a>aa bb cc dd ee</li>"
                 "<li id=b style='list-style:decimal inside'>x</li></ul></body>", 320);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
    l61_text_at("*", 30, 1);
    l61_text_at("aa bb cc dd", 46, 1);
    l61_text_at("ee", 30, 11);
    l61_text_at("2.", 30, 21);
    l61_text_at("x", 54, 21);
    l64_box("a", 30, 0, 110, 20);
    /* inherited by the li; outside again restores the gutter marker */
    lot52_render(L64_BODY "<ul style='margin:0;padding:0 0 0 30px;list-style:inside square'><li>p</li><li style='list-style-position:outside;list-style-type:disc'>q</li></ul></body>", 320);
    l61_text_at("p", 46, 1);
    l61_text_at("q", 30, 11);
    l61_text_at("*", 14, 11);
    l61_text_at("#", 30, 1);
}

static void test_l73_list_inside_rtl_and_refusals(void) {
    lot52_render(L64_BODY "<ol dir=rtl style='margin:0;padding:0 30px 0 0;width:100px;list-style-position:inside'><li id=a>ab</li></ol></body>", 320);
    l61_text_at(".1", 84, 1);           /* right of the content box: 100 - 16 */
    l61_text_at("ab", 60, 1);           /* the line ends before the marker room: 100 - 24 - 16 */
    lot52_render(L64_BODY "<ul style='list-style-position:middle'><li>a</li></ul></body>", 320);
    TEST_ASSERT_TRUE(lot52_has_diag("list marker refused"));
    lot52_render(L64_BODY "<ul style='list-style:inside outside disc'><li>a</li></ul></body>", 320);
    TEST_ASSERT_TRUE(lot52_has_diag("list marker refused"));
    /* none: no marker and no indent */
    lot52_render(L64_BODY "<ul style='margin:0;padding:0 0 0 30px;list-style:none inside'><li>a</li></ul></body>", 320);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
    l61_text_at("a", 30, 1);
}

/* Lot 73: font-variant small-caps (capitals of the same size, one diag)
 * and text-underline-offset (inherited, 0..16 px, auto). */
static const os_fb_web_item_t *l73_rect_at(int x, int y) {
    unsigned i;
    for (i = 0; i < g_doc.nitems; i++)
        if (g_doc.items[i].kind == OS_FB_WEB_RECT && g_doc.items[i].x == x && g_doc.items[i].y == y) return &g_doc.items[i];
    return 0;
}

static void test_l73_small_caps(void) {
    lot52_render(L64_BODY "<p style='margin:0;font-variant:small-caps'>Ab <span style='font-variant:normal'>cd</span> "
                 "<span style='text-transform:lowercase'>EF</span></p><p style='margin:0;font-variant-caps:all-small-caps'>gh</p></body>", 320);
    l61_text_at("AB", 0, 1);
    l61_text_at("cd", 24, 1);
    l61_text_at("ef", 48, 1);           /* an explicit transform wins */
    l61_text_at("GH", 0, 11);
    TEST_ASSERT_EQUAL(2, l61_diag_count("small-caps painted uppercase"));
    lot52_render(L64_BODY "<p style='font-variant:oldstyle-nums'>a</p></body>", 320);
    TEST_ASSERT_TRUE(lot52_has_diag("font-variant refused"));
    TEST_ASSERT_NOT_NULL(find_text("a"));
}

static void test_l73_underline_offset(void) {
    lot52_render(L64_BODY "<p style='margin:0'><u style='text-underline-offset:3px'>cd</u> <u>e</u>"
                 "<span style='text-underline-offset:16px'> <u>f</u> <u style='text-underline-offset:auto'>g</u></span></p></body>", 320);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
    TEST_ASSERT_NOT_NULL(l73_rect_at(0, 12));       /* 1 + 8 + 3 */
    TEST_ASSERT_NULL(l73_rect_at(0, 9));
    TEST_ASSERT_NOT_NULL(l73_rect_at(24, 9));       /* default */
    TEST_ASSERT_NOT_NULL(l73_rect_at(40, 25));      /* inherited 16 */
    TEST_ASSERT_NOT_NULL(l73_rect_at(56, 9));       /* auto resets */
    lot52_render(L64_BODY "<p style='text-underline-offset:17px'><u>a</u></p><p style='text-underline-offset:-1px'><u>b</u></p>"
                 "<p style='text-underline-offset:10%'><u>c</u></p></body>", 320);
    TEST_ASSERT_EQUAL(3, l61_diag_count("text-decoration refused"));
}

static void lot73_run(void) {
    RUN_TEST(test_l73_paragraph_split_between_columns);
    RUN_TEST(test_l73_orphans_and_widows);
    RUN_TEST(test_l73_kept_whole);
    RUN_TEST(test_l73_margins_and_inline_children);
    RUN_TEST(test_l73_forced_break_and_refusals);
    RUN_TEST(test_l73_line_clamp_two_lines);
    RUN_TEST(test_l73_line_clamp_one_and_room);
    RUN_TEST(test_l73_line_clamp_not_needed_and_nested);
    RUN_TEST(test_l73_line_clamp_refusals_and_flow_root);
    RUN_TEST(test_l73_list_inside);
    RUN_TEST(test_l73_list_inside_rtl_and_refusals);
    RUN_TEST(test_l73_small_caps);
    RUN_TEST(test_l73_underline_offset);
}
