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

/* Lot 73: inline box edges and box-decoration-break. An inline element's
 * horizontal margin, border and padding take room before its first word
 * and after its last one; its borders and padded background paint per
 * line fragment. slice (default) paints the start edge on the first
 * fragment and the end edge on the last; clone gives every fragment both
 * edges and the room for them. */
#define L73_SPAN "border:2px solid #ff0000;padding:0 4px;background:#ffff00;margin:0 3px"
static void test_l73_inline_edges_slice(void) {
    lot52_render(L64_BODY "<p style='margin:0;width:120px'>x <span style='" L73_SPAN "'>ab cd ef gh ij</span> z</p></body>", 320);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
    l61_text_at("ab cd ef gh", 25, 1);           /* 8 + 8 + margin 3 + border 2 + padding 4 */
    l61_rect_at(21, 0, 92, 10, 0xFFFF00);        /* background over the left padding only */
    l61_rect_at(19, -2, 94, 2, 0xFF0000);        /* top border */
    l61_rect_at(19, 10, 94, 2, 0xFF0000);        /* bottom border */
    l61_rect_at(19, 0, 2, 10, 0xFF0000);         /* left border: first fragment */
    TEST_ASSERT_NULL(l73_rect_at(113, 0));       /* no right border on it */
    l61_text_at("ij", 0, 11);                    /* the second fragment has no start edge */
    l61_rect_at(0, 10, 20, 10, 0xFFFF00);
    l61_rect_at(20, 10, 2, 10, 0xFF0000);        /* right border: last fragment */
    TEST_ASSERT_NULL(l73_rect_at(-2, 10));
    l61_text_at("z", 33, 11);                    /* 16 + 4 + 2 + 3 + space */
}

static void test_l73_inline_edges_clone(void) {
    lot52_render(L64_BODY "<p style='margin:0;width:120px'>x <span style='" L73_SPAN ";-webkit-box-decoration-break:clone'>ab cd ef gh ij</span> z</p></body>", 320);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
    l61_text_at("ab cd ef", 25, 1);              /* gh would leave no room for the end edge */
    l61_rect_at(93, 0, 2, 10, 0xFF0000);         /* first fragment closes */
    l61_rect_at(21, 0, 72, 10, 0xFFFF00);
    l61_text_at("gh ij", 9, 11);                 /* the second fragment opens again */
    l61_rect_at(3, 10, 2, 10, 0xFF0000);
    l61_rect_at(5, 10, 48, 10, 0xFFFF00);
    l61_rect_at(53, 10, 2, 10, 0xFF0000);
    l61_text_at("z", 66, 11);
    /* a box that fits on one line paints the same */
    lot52_render(L64_BODY "<p style='margin:0'><span style='" L73_SPAN ";box-decoration-break:clone'>ab</span>c</p></body>", 320);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
    l61_text_at("ab", 9, 1);
    l61_rect_at(3, 0, 2, 10, 0xFF0000);
    l61_rect_at(29, 0, 2, 10, 0xFF0000);
    l61_text_at("c", 34, 1);
}

static void test_l73_inline_edges_nesting(void) {
    /* an empty box takes no room; edges of outer boxes add up; the end
     * edge of an outer box follows the last word inside it */
    lot52_render(L64_BODY "<p style='margin:0'>a<span style='padding:0 5px'></span>b <b style='margin-left:6px'>"
                 "<i style='padding-right:2px'>c</i></b>d <span style='margin-right:4px'><u style='padding-right:1px'>e</u></span>f"
                 " <span style='padding-left:3px'><span style='padding-left:2px'>g</span></span></p></body>", 320);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
    l61_text_at("b", 8, 1);
    l61_text_at("c", 30, 1);
    l61_text_at("d", 40, 1);
    l61_text_at("f", 69, 1);                     /* 56 + 8 + 1 + 4 */
    l61_text_at("g", 90, 1);                     /* 77 + 8 + 3 + 2 */
    /* rtl lays words left to right too: the start edge is the left one */
    lot52_render(L64_BODY "<p style='margin:0;width:120px;direction:rtl'>x <span style='border-left:2px solid #ff0000;"
                 "border-right:1px solid #0000ff;padding:0 4px'>ab cd ef gh ij</span> z</p></body>", 320);
    l61_text_at("ab cd ef gh", 32, 1);
    l61_rect_at(26, 0, 2, 10, 0xFF0000);
    l61_rect_at(103, 10, 1, 10, 0x0000FF);
    l61_text_at("z", 112, 11);
}

static void test_l73_inline_edges_long_word_and_wrap(void) {
    /* a word longer than one text item paints in pieces: the start edge
     * before the first piece only, the end edge after the last only */
    lot52_render(L64_BODY "<p style='margin:0'><span style='padding:0 4px;border-left:2px solid #ff0000;border-right:2px solid #0000ff'>"
                 "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa</span></p></body>", 640);
    l61_rect_at(0, 0, 2, 10, 0xFF0000);
    l61_text_at("aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa", 6, 1);
    l61_text_at("aaa", 382, 1);
    l61_rect_at(410, 0, 2, 10, 0x0000FF);
    TEST_ASSERT_NULL(l73_rect_at(382, 0));
    TEST_ASSERT_NULL(l73_rect_at(376, 0));
    TEST_ASSERT_EQUAL(4, (int)g_doc.nitems);   /* two borders, two pieces */
    /* the end edge counts toward the line width */
    lot52_render(L64_BODY "<p style='margin:0;width:40px'><span style='padding-right:8px'>ab</span> cd</p>"
                 "<p style='margin:0;width:60px;padding:0 10px;box-decoration-break:clone'>ab cd ef gh</p></body>", 320);
    l61_text_at("cd", 0, 11);
    l61_text_at("ef gh", 10, 31);          /* a block's padding is not an inline edge */
}

static void test_l73_decoration_break_refusals(void) {
    lot52_render(L64_BODY "<p style='margin:0'><span style='box-decoration-break:clone;box-decoration-break:auto;" L73_SPAN "'>ab cd</span></p>"
                 "<p style='margin:0;width:40px'><span style='box-decoration-break:clone;box-decoration-break:slice;" L73_SPAN "'>ab cd</span></p></body>", 320);
    TEST_ASSERT_EQUAL(1, l61_diag_count("box-decoration-break refused"));
    l61_rect_at(53, 0, 2, 10, 0xFF0000);         /* auto refused: clone stays (one line) */
    l61_text_at("ab", 9, 11);
    l61_text_at("cd", 0, 21);                    /* slice: no start edge on the second line */
    l61_rect_at(20, 20, 2, 10, 0xFF0000);
    TEST_ASSERT_NULL(l73_rect_at(25, 10));       /* nor an end edge on the first */
}

/* Lot 73: form controls painted static (webeng_forms.inc). */
static const os_fb_web_item_t *l73_item(int kind, int x, int y) {
    unsigned i;
    for (i = 0; i < g_doc.nitems; i++)
        if (g_doc.items[i].kind == kind && g_doc.items[i].x == x && g_doc.items[i].y == y) return &g_doc.items[i];
    return 0;
}

static const we_node_t *l73_node(const char *tag, int nth) {
    unsigned i;
    for (i = 0; i < g_doc.nnodes; i++)
        if (strcmp(g_doc.nodes[i].tag, tag) == 0 && nth-- == 0) return &g_doc.nodes[i];
    return 0;
}

static int l73_fill_pct(int nth) {
    const we_node_t *n = l73_node("::fill", nth);
    return n ? n->width_pct : -1;
}

static void test_l73_form_inputs(void) {
    lot52_render(L64_BODY "<input value='abc' size=5><input placeholder='nom'><input type=password value=abc>"
                 "<input type=checkbox checked><input type=radio checked><input type=checkbox><button>Go</button></body>", 320);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
    l61_rect_at(0, 0, 46, 14, 0xFFFFFF);            /* 5 glyphs + padding 2 + border 1 */
    l61_text_at("abc", 3, 3);
    l61_rect_at(46, 0, 166, 14, 0xFFFFFF);          /* size 20 by default */
    l61_text_at("nom", 49, 3);
    TEST_ASSERT_EQUAL(0x757575, (int)find_text("nom")->color);   /* placeholder */
    TEST_ASSERT_EQUAL(0x000000, (int)find_text("abc")->color);
    l61_text_at("***", 3, 17);                      /* password: one star a byte */
    l61_rect_at(169, 14, 10, 12, 0xFFFFFF);
    l61_text_at("x", 170, 16);
    l61_text_at("*", 186, 16);
    l61_rect_at(201, 14, 10, 12, 0xFFFFFF);         /* unchecked: an empty box */
    TEST_ASSERT_NULL(l73_item(OS_FB_WEB_TEXT, 202, 16));
    l61_rect_at(214, 14, 30, 14, 0xEFEFEF);
    l61_text_at("Go", 221, 17);
    TEST_ASSERT_NOT_NULL(l73_item(OS_FB_WEB_BORDER, 214, 14));
}

static void test_l73_form_buttons_and_types(void) {
    /* Lot 76: type=date is WE_FORM_DATE (no longer "taken as text"). value=d still paints. */
    lot52_render(L64_BODY "<input type=submit><input type=reset value=RAZ><input type=hidden value=h><input type=button>"
                 "<input type=date value=d><input type=text size=0><button type=reset disabled>B</button></body>", 320);
    l61_text_at("Submit", 7, 3);
    l61_text_at("RAZ", 69, 3);
    TEST_ASSERT_NULL(find_text("h"));
    l61_rect_at(100, 0, 14, 14, 0xEFEFEF);          /* button without value: padding only */
    TEST_ASSERT_EQUAL(WE_FORM_DATE, l73_node("input", 4)->form_type);
    TEST_ASSERT_NOT_NULL(find_text("d"));           /* date value text */
    l61_text_at("B", 173, 17);
    TEST_ASSERT_EQUAL(0, l61_diag_count("input type taken as text"));
    TEST_ASSERT_EQUAL(1, l61_diag_count("form size refused"));
    TEST_ASSERT_EQUAL(0x6D6D6D, (int)find_text("B")->color);   /* disabled */
    l61_rect_at(0, 14, 166, 14, 0xFFFFFF);          /* size refused: 20 glyphs, one empty line */
    l61_rect_at(166, 14, 22, 14, 0xEFEFEF);
    TEST_ASSERT_EQUAL(WE_FORM_RESET, l73_node("button", 0)->form_type);
    TEST_ASSERT_EQUAL(WE_FORM_RESET, l73_node("input", 1)->form_type);
    TEST_ASSERT_EQUAL(WE_DISP_NONE, l73_node("input", 2)->display);   /* hidden */
}

static void test_l73_form_select(void) {
    lot52_render(L64_BODY "<select><option>aa</option><option selected>bb</option></select>"
                 "<select size=2><option>x</option><option selected>y</option><option>z</option></select>"
                 "<select><optgroup><option>a</option><option>c</option></optgroup></select></body>", 320);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
    l61_text_at("bb", 3, 3);
    TEST_ASSERT_NULL(find_text("aa"));
    l61_text_at("v", 27, 3);
    l61_rect_at(38, 0, 14, 24, 0xFFFFFF);           /* list box: two rows */
    l61_text_at("x", 41, 3);
    l61_rect_at(41, 12, 8, 10, 0x3875D7);           /* the selected row */
    TEST_ASSERT_EQUAL(0xFFFFFF, (int)find_text("y")->color);
    l61_text_at("a", 55, 3);                         /* inside an optgroup: the first */
    TEST_ASSERT_NULL(find_text("c"));
    lot52_render(L64_BODY "<select><option selected>p1</option><option selected>p2</option></select>"
                 "<textarea placeholder=ph>t</textarea></body>", 320);
    TEST_ASSERT_NOT_NULL(find_text("p1"));          /* one row: the first selected */
    TEST_ASSERT_NULL(find_text("p2"));
    TEST_ASSERT_NULL(find_text("ph"));              /* text wins over the placeholder */
}

static void test_l73_form_textarea_and_bars(void) {
    lot52_render(L64_BODY "<textarea rows=2 cols=6>hello world</textarea><textarea placeholder=pp></textarea>"
                 "<progress value=30 max=60></progress><meter value=0.2 low=0.3>m<b>q</b></meter>"
                 "<meter min=10 max=20 value=15 high=12></meter><progress>x</progress>"
                 "<meter value=5 min=8 max=2></meter></body>", 320);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
    l61_rect_at(0, 0, 54, 26, 0xFFFFFF);
    l61_text_at("hello ", 3, 4);
    l61_text_at("world", 3, 14);                     /* wraps inside its columns */
    l61_text_at("pp", 57, 4);
    TEST_ASSERT_EQUAL(0x757575, (int)find_text("pp")->color);
    l61_rect_at(220, 0, 82, 10, 0xE6E6E6);
    l61_rect_at(221, 1, 40, 8, 0x3875D7);            /* 30 / 60 */
    l61_rect_at(1, 27, 16, 8, 0xE6C200);             /* 0.2 of 1, under low */
    TEST_ASSERT_NULL(find_text("m"));
    TEST_ASSERT_NULL(find_text("q"));
    l61_rect_at(83, 27, 40, 8, 0xE6C200);            /* over high */
    TEST_ASSERT_NULL(find_text("x"));
    l61_rect_at(164, 26, 82, 10, 0xE6E6E6);
    TEST_ASSERT_NULL(l73_rect_at(165, 27));          /* indeterminate */
    l61_rect_at(0, 36, 82, 10, 0xE6E6E6);
    TEST_ASSERT_NULL(l73_rect_at(1, 37));            /* max under min: empty */
    lot52_render(L64_BODY "<meter value=0.5></meter><progress value=2 max=-1></progress><meter value=x></meter></body>", 320);
    l61_rect_at(1, 1, 40, 8, 0x4CAF50);
    l61_rect_at(83, 1, 80, 8, 0x3875D7);             /* a bad max is 1: full */
    TEST_ASSERT_EQUAL(1, l61_diag_count("form number refused"));
    lot52_render(L64_BODY "<meter value=3x></meter><progress value=0.5 max=0></progress>"
                 "<progress value=3 max=2></progress></body>", 320);
    TEST_ASSERT_EQUAL(1, l61_diag_count("form number refused"));   /* trailing junk */
    l61_rect_at(83, 1, 40, 8, 0x3875D7);             /* max 0 is ignored: 0.5 of 1 */
    TEST_ASSERT_EQUAL(100, l73_fill_pct(2));         /* a value over max is max */
}

static void test_l73_form_details_fieldset(void) {
    lot52_render(L64_BODY "<details><summary>S</summary>hidden<p>gone</p></details><details open>shown</details>"
                 "<fieldset><legend>L</legend>f</fieldset></body>", 320);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
    l61_text_at(">", 0, 1);
    l61_text_at("S", 16, 1);
    TEST_ASSERT_NULL(find_text("hidden"));
    TEST_ASSERT_NULL(find_text("gone"));
    l61_text_at("v", 0, 11);
    l61_text_at("Details", 16, 11);                  /* a summary is made */
    l61_text_at("shown", 0, 21);
    l61_text_at("L", 16, 37);
    l61_text_at("f", 14, 47);
    TEST_ASSERT_NOT_NULL(l73_item(OS_FB_WEB_RECT, 2, 30));   /* groove: two tones */
    TEST_ASSERT_NOT_NULL(l73_item(OS_FB_WEB_RECT, 3, 31));
    /* author rules come after the UA styles */
    lot52_render(L64_BODY "<style>input{width:24px;background:#ffff00;border:0}</style><input value=q></body>", 320);
    l61_rect_at(0, 0, 28, 12, 0xFFFF00);
    l61_text_at("q", 2, 2);
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
    RUN_TEST(test_l73_inline_edges_slice);
    RUN_TEST(test_l73_inline_edges_clone);
    RUN_TEST(test_l73_inline_edges_nesting);
    RUN_TEST(test_l73_inline_edges_long_word_and_wrap);
    RUN_TEST(test_l73_decoration_break_refusals);
    RUN_TEST(test_l73_form_inputs);
    RUN_TEST(test_l73_form_buttons_and_types);
    RUN_TEST(test_l73_form_select);
    RUN_TEST(test_l73_form_textarea_and_bars);
    RUN_TEST(test_l73_form_details_fieldset);
}
