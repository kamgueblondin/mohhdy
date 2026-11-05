/* Lot 73: HTML presentational hints (webeng_hints.inc). */

static void test_l73_hints_table(void) {
    lot52_render("<body id=bd text=ff0000 bgcolor='#00ff00' style='margin:0'><table id=t border=2 cellpadding=3 cellspacing=4 width=200 "
                 "align=center bgcolor=0000ff><tr id=r valign=bottom><td id=a width=50 nowrap align=right>A</td>"
                 "<td id=b height=40 bgcolor=abc>B</td></tr></table></body>", 320);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
    TEST_ASSERT_EQUAL(0xFF0000, l73c("t"));             /* body text */
    TEST_ASSERT_EQUAL(204, lot52_node("t")->w);          /* width 200 + border 2 */
    TEST_ASSERT_EQUAL(58, lot52_node("t")->x);           /* align=center: auto margins */
    TEST_ASSERT_EQUAL(2, lot52_node("t")->border);
    TEST_ASSERT_EQUAL(8 /* outset */, lot52_node("t")->border_style);
    TEST_ASSERT_EQUAL(0x808080, (int)lot52_node("t")->border_color);
    TEST_ASSERT_EQUAL(0x0000FF, (int)lot52_node("t")->bg);
    TEST_ASSERT_EQUAL(0x00FF00, (int)lot52_node("bd")->bg);
    TEST_ASSERT_EQUAL(58, lot52_node("a")->w);           /* width 50 + cellpadding 3 + cell border 1 */
    TEST_ASSERT_EQUAL(1, lot52_node("a")->border);
    TEST_ASSERT_EQUAL(10 /* inset */, lot52_node("a")->border_style);
    TEST_ASSERT_EQUAL(3, lot52_node("a")->padding[0]);
    TEST_ASSERT_EQUAL(122, lot52_node("a")->x + lot52_node("a")->w + 4);   /* cellspacing 4 */
    TEST_ASSERT_EQUAL(122, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(1, lot52_node("a")->nowrap);
    TEST_ASSERT_EQUAL(WE_ALIGN_RIGHT, lot52_node("a")->align);
    TEST_ASSERT_EQUAL(0xAABBCC, (int)lot52_node("b")->bg);   /* legacy 3-digit colour */
    l61_text_at("A", 106, 37);                           /* right, and the row's valign bottom */
    l61_text_at("B", 126, 37);
    TEST_ASSERT_EQUAL(48, lot52_node("b")->h);           /* height 40 + padding 6 + border 2 */
}

static void test_l73_hints_refused_and_css_wins(void) {
    lot52_render("<body style='margin:0'><table id=t width=50%><tr><td id=a width=10x>a</td><td id=b width=0>b</td><td id=c width=-3>c</td>"
                 "<td id=d height=2.5>d</td></tr></table><table id=u border=0><tr><td id=e>e</td></tr></table>"
                 "<table id=v border=x cellpadding=2% cellspacing=1%><tr><td id=f>f</td></tr></table><p id=p align=bogus>p</p></body>", 320);
    TEST_ASSERT_EQUAL(160, lot52_node("t")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->w);           /* digits first, the rest ignored */
    TEST_ASSERT_EQUAL(50, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(0, lot52_node("u")->border);       /* border=0: none */
    TEST_ASSERT_EQUAL(0, lot52_node("e")->border);
    TEST_ASSERT_EQUAL(0, lot52_node("f")->padding[0]);
    TEST_ASSERT_EQUAL(6, l61_diag_count("hint refused"));
    lot52_render("<style>td{padding:7px}#q{text-align:left}</style><body style='margin:0'><table cellpadding=3 border=1>"
                 "<tbody valign=middle><tr><td id=a style='height:30px'>a</td><td id=b valign=top style='height:30px'>b</td></tr></tbody>"
                 "</table><p id=q align=right>q</p><div id=h hidden style='display:block'>h</div><div id=g hidden>g</div>"
                 "<span id=s align=center>s</span></body>", 320);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
    TEST_ASSERT_EQUAL(7, lot52_node("a")->padding[0]);   /* author CSS beats cellpadding */
    l61_text_at("a", 9, 20);                             /* valign of the row group: middle */
    l61_text_at("b", 168, 10);                           /* its own valign: top */
    TEST_ASSERT_EQUAL(WE_ALIGN_LEFT, lot52_node("q")->align);
    TEST_ASSERT_EQUAL(WE_DISP_BLOCK, lot52_node("h")->display);   /* hidden is UA level */
    TEST_ASSERT_EQUAL(WE_DISP_NONE, lot52_node("g")->display);
    TEST_ASSERT_NULL(find_text("g"));
    TEST_ASSERT_EQUAL(WE_ALIGN_LEFT, lot52_node("s")->align);     /* no align hint on span */
}

static void test_l73_hints_inline_and_images(void) {
    lot52_render("<body style='margin:0'><font id=f color=00ff00 size=5>F</font><font id=g size=-1>s</font><font id=h size=+9>L</font>"
                 "<font id=i size=x color=nope>i</font><center id=c>x</center><p id=p align=center>c</p><div id=d align=justify>d</div>"
                 "<div id=e align=MIDDLE>e</div></body>", 320);
    TEST_ASSERT_EQUAL(0x00FF00, l73c("f"));
    TEST_ASSERT_EQUAL(24, lot52_node("f")->font_px);
    TEST_ASSERT_EQUAL(13, lot52_node("g")->font_px);
    TEST_ASSERT_EQUAL(48, lot52_node("h")->font_px);
    TEST_ASSERT_EQUAL(16, lot52_node("i")->font_px);
    TEST_ASSERT_EQUAL(1, l61_diag_count("hint refused"));            /* size=x; a colour name is CSS's job */
    TEST_ASSERT_EQUAL(0, l73c("i"));
    TEST_ASSERT_EQUAL(WE_DISP_BLOCK, lot52_node("c")->display);
    TEST_ASSERT_EQUAL(WE_ALIGN_CENTER, lot52_node("c")->align);
    TEST_ASSERT_EQUAL(WE_ALIGN_CENTER, lot52_node("p")->align);
    TEST_ASSERT_EQUAL(WE_ALIGN_JUSTIFY, lot52_node("d")->align);
    TEST_ASSERT_EQUAL(WE_ALIGN_CENTER, lot52_node("e")->align);
    lot52_render("<body style='margin:0'><img id=i width=30 height=20 hspace=5 vspace=2 border=1 align=right>"
                 "<img id=j align=middle width=4 height=4><img id=k align=absmiddle><img id=l align=top><img id=m align=bogus>"
                 "<table id=t align=right width=50><tr><td>R</td></tr></table><table id=u align=left><tr><td>L</td></tr></table>"
                 "<table id=v align=top><tr><td>v</td></tr></table>"
                 "<hr id=a width=50% align=left><hr id=b width=40 align=right><hr id=c width=40 align=center></body>", 320);
    TEST_ASSERT_EQUAL(5, lot52_node("i")->margin[1]);
    TEST_ASSERT_EQUAL(5, lot52_node("i")->margin[3]);
    TEST_ASSERT_EQUAL(2, lot52_node("i")->margin[0]);
    TEST_ASSERT_EQUAL(2, lot52_node("i")->margin[2]);
    TEST_ASSERT_EQUAL(1, lot52_node("i")->border);
    TEST_ASSERT_EQUAL(WE_BS_SOLID, lot52_node("i")->border_style);
    TEST_ASSERT_EQUAL(30, lot52_node("i")->width);
    TEST_ASSERT_EQUAL(20, lot52_node("i")->height);
    TEST_ASSERT_EQUAL(WE_FLOAT_RIGHT, lot52_node("i")->float_side);
    TEST_ASSERT_EQUAL(WE_VALIGN_MIDDLE, lot52_node("j")->valign);
    TEST_ASSERT_EQUAL(WE_VALIGN_MIDDLE, lot52_node("k")->valign);
    TEST_ASSERT_EQUAL(WE_VALIGN_TOP, lot52_node("l")->valign);
    TEST_ASSERT_EQUAL(WE_FLOAT_RIGHT, lot52_node("t")->float_side);
    TEST_ASSERT_EQUAL(WE_FLOAT_LEFT, lot52_node("u")->float_side);
    TEST_ASSERT_EQUAL(2, l61_diag_count("hint refused"));            /* img bogus, table top */
    TEST_ASSERT_EQUAL(WE_AUTO_RIGHT, lot52_node("a")->margin_auto);
    TEST_ASSERT_EQUAL(WE_AUTO_LEFT, lot52_node("b")->margin_auto);
    TEST_ASSERT_EQUAL(WE_AUTO_LEFT | WE_AUTO_RIGHT, lot52_node("c")->margin_auto);
}

static void test_l73_hints_limits(void) {
    lot52_render("<body style='margin:0'><table id=t width=100 width=200 height=1 cellpadding=1 cellspacing=1 border=1 bgcolor=fff "
                 "align=left hidden valign=top><tr><td>x</td></tr></table><p id=p align=right title=x foo=1 align=left>p</p></body>", 320);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);                 /* valign is no table hint */
    TEST_ASSERT_EQUAL(WE_DISP_NONE, lot52_node("t")->display);
    TEST_ASSERT_EQUAL(WE_HINTS_MAX, lot52_node("t")->nhints);   /* every table hint, the second width dropped */
    TEST_ASSERT_EQUAL(WE_ALIGN_RIGHT, lot52_node("p")->align);    /* the first one wins */
    TEST_ASSERT_EQUAL(1, lot52_node("p")->nhints);
}


/* Lot 73: captions, row group order, col widths */
static void test_l73_table_caption_and_groups(void) {
    lot52_render("<body style='margin:0'><table id=t style='width:200px'><caption id=c>Cap</caption><tfoot><tr><td>F</td></tr></tfoot>"
                 "<tbody><tr><td>B</td></tr></tbody><thead><tr><td>H</td></tr></thead>"
                 "<caption id=l style='caption-side:bottom'>Low</caption></table></body>", 320);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
    l61_text_at("Cap", 88, 1);                           /* centred above */
    l61_text_at("H", 0, 11);                             /* thead first */
    l61_text_at("B", 0, 21);
    l61_text_at("F", 0, 31);                             /* tfoot last */
    l61_text_at("Low", 88, 41);
    TEST_ASSERT_EQUAL(50, lot52_node("t")->h);
    TEST_ASSERT_EQUAL(2, (int)g_doc.perf.table_captions);
    TEST_ASSERT_EQUAL(1, lot52_node("l")->caption_bottom);
    /* only the first thead / tfoot move; caption-side inherits; align */
    lot52_render("<body style='margin:0'><table style='width:100px;caption-side:bottom'><tfoot><tr><td>F1</td></tr></tfoot>"
                 "<tfoot><tr><td>F2</td></tr></tfoot><thead><tr><td>H1</td></tr></thead><thead><tr><td>H2</td></tr></thead>"
                 "<caption>Bot</caption><caption align=top style='text-align:left'>Top</caption><caption style='caption-side:x'>X</caption>"
                 "<caption align=left>Y</caption></table></body>", 320);
    l61_text_at("Top", 0, 1);
    l61_text_at("H1", 0, 11);
    l61_text_at("F2", 0, 21);
    l61_text_at("H2", 0, 31);
    l61_text_at("F1", 0, 41);
    l61_text_at("Bot", 38, 51);
    l61_text_at("X", 46, 61);
    l61_text_at("Y", 0, 71);
    TEST_ASSERT_EQUAL(1, l61_diag_count("caption side refused"));
    TEST_ASSERT_EQUAL(0, l61_diag_count("table child refused"));
}

static void test_l73_table_col_widths(void) {
    lot52_render("<body style='margin:0'><table style='width:300px'><col width=40><colgroup span=2 style='width:100px'></colgroup>"
                 "<tr><td>a</td><td>b</td><td>c</td><td>d</td></tr></table></body>", 320);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
    l61_text_at("a", 0, 1);
    l61_text_at("b", 40, 1);
    l61_text_at("c", 140, 1);
    l61_text_at("d", 240, 1);
    lot52_render("<body style='margin:0'><table style='width:300px'><colgroup style='width:90px'><col style='width:20%'>"
                 "<col style='display:none;width:200px'><col span=2></colgroup><col style='width:30px'>"
                 "<tr><td>a</td><td>b</td><td>c</td><td>d</td></tr></table></body>", 320);
    l61_text_at("a", 0, 1);                              /* 20% of 300 */
    l61_text_at("b", 60, 1);                             /* the colgroup width is not its cols' */
    l61_text_at("d", 270, 1);                            /* the col after the group: column 4 */
}

static void lot73_hints_run(void) {
    RUN_TEST(test_l73_hints_table);
    RUN_TEST(test_l73_hints_refused_and_css_wins);
    RUN_TEST(test_l73_hints_inline_and_images);
    RUN_TEST(test_l73_hints_limits);
    RUN_TEST(test_l73_table_caption_and_groups);
    RUN_TEST(test_l73_table_col_widths);
}
