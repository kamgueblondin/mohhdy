/* Lot 63: character references, overflow values and white-space longhands. */
static void test_l63_text_character_references(void) {
    lot52_render(L63_STYLE("p{margin:0}") "<p>a&#39;b&#x41;&#65;c&mdash;&hellip;&copy;&#8217;&#xe9;&shy;x&#173;y&euro;&bogus;&#;&#12a&#10;z&AMP;&nbsp;q</p>"
                 "<p>&lsquo;&rsquo;&ldquo;&rdquo;&laquo;&raquo;&reg;&trade;&times;&middot;&bull;&ndash;&apos;&lt;&gt;&quot;</p>"
                 "<p>&#8211;&#8212;&#8216;&#8220;&#8221;&#8230;&#9;&#160;&#8194;&#8202;&#8203;&#XA0;&#1234567;&#12345678;</p>"
                 "<p>&ensp;&emsp;&thinsp;.&#x2a;&#X2B;&#xg;&#x;&amp</p></body>", 640);
    TEST_ASSERT_NOT_NULL(find_text("a'bAAc-...(c)'?xyEUR&bogus;&#;&#12a z& q"));   /* numeric, named, soft hyphen, unknown */
    TEST_ASSERT_NOT_NULL(find_text("''\"\"<<>>(R)TMx.*-'<>\""));
    TEST_ASSERT_NOT_NULL(find_text("--'\"\"... ? ?&#12345678;"));   /* spaces collapse, 7 digits at most */
    TEST_ASSERT_NOT_NULL(find_text(".*+&#xg;&#x;&amp"));             /* leading spaces collapse */
}

static void test_l63_text_reference_spaces_and_newline(void) {
    lot52_render(L63_STYLE("p{margin:0}") "<p id=a>&nbsp;</p><p id=b style='white-space:pre'>x&#10;y</p><p id=c>&#32;</p></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->h);              /* a no-break space alone collapses, as before */
    TEST_ASSERT_EQUAL(find_text("x")->y + 10, find_text("y")->y);   /* &#10; is a newline */
    TEST_ASSERT_EQUAL(0, lot52_node("c")->h);
}

static void test_l63_text_overflow_values(void) {
    static const char *const clips[] = { "clip", "auto", "scroll", "hidden visible", "visible scroll", "auto auto" };
    unsigned k;
    for (k = 0; k < sizeof(clips) / sizeof(clips[0]); k++) {
        char html[256];
        snprintf(html, sizeof(html), "<body style='margin:0'><div id=p style='width:20px;height:10px;overflow:%s'>"
                 "<div style='width:40px;height:20px;background:#ff0000'></div></div></body>", clips[k]);
        lot52_render(html, 320);
        L63_CASE(lot52_node("p")->overflow_hidden == 1, clips[k]);
        L63_CASE(g_doc.items[0].w == 20 && g_doc.items[0].h == 10, clips[k]);   /* clipped to the box */
        L63_CASE(g_doc.ndiag == 0, clips[k]);
    }
    lot52_render("<body style='margin:0'><div id=p style='width:20px;height:10px;overflow-x:hidden'><div style='width:40px;height:20px;background:#ff0000'></div></div>"
                 "<div id=q style='overflow:hidden;overflow-x:visible'></div><div id=r style='overflow-y:scroll;overflow-y:visible'></div>"
                 "<div id=s style='overflow:hidden auto;overflow:visible'></div><div id=t style='overflow-y:clip'></div></body>", 320);
    TEST_ASSERT_EQUAL(1, lot52_node("p")->overflow_axes);
    TEST_ASSERT_EQUAL(20, g_doc.items[0].w);
    TEST_ASSERT_EQUAL(10, g_doc.items[0].h);               /* the visible y axis clips too */
    TEST_ASSERT_EQUAL(2, lot52_node("q")->overflow_axes);   /* x cleared, y kept */
    TEST_ASSERT_EQUAL(1, lot52_node("q")->overflow_hidden);
    TEST_ASSERT_EQUAL(0, lot52_node("r")->overflow_hidden);
    TEST_ASSERT_EQUAL(0, lot52_node("s")->overflow_axes);
    TEST_ASSERT_EQUAL(2, lot52_node("t")->overflow_axes);
    lot52_render("<body style='margin:0'><div id=p style='overflow:hidden;overflow:hidden visible clip;overflow-x:hidden auto;overflow-y:wavy;overflow:;overflow:visible wavy'></div>"
                 "<div id=q style='overflow-x:clip;overflow-y:clip'></div></body>", 320);
    TEST_ASSERT_EQUAL(3, lot52_node("p")->overflow_axes);   /* a bad y value refuses the pair */
    TEST_ASSERT_EQUAL(4, l61_diag_count("overflow refused"));
    TEST_ASSERT_EQUAL(3, lot52_node("q")->overflow_axes);   /* the y longhand keeps x */
}

/* a second style pass starts each box from visible on both axes */
static void test_l63_text_overflow_restyle(void) {
    uint32_t k;
    lot52_render(L63_STYLE("div{overflow-x:clip}") "<div id=p style='overflow-y:clip'></div></body>", 320);
    TEST_ASSERT_EQUAL(3, lot52_node("p")->overflow_axes);
    for (k = 0; k < g_doc.nrules; k++) g_doc.rules[k].pseudo_el = WE_PSEUDO_AFTER;   /* the rule no longer applies */
    TEST_ASSERT_EQUAL(0, we_style(&g_doc));
    TEST_ASSERT_EQUAL(2, lot52_node("p")->overflow_axes);
}

/* an nbsp-only chunk between elements acts as one collapsible space */
static void test_l63_text_nbsp_chunk(void) {
    lot52_render(L63_STYLE("") "<p id=p style='margin:0'><b>a</b>&nbsp;<b>c</b></p></body>", 320);
    TEST_ASSERT_EQUAL(-1, l63_child(we_find_id(&g_doc, "p"), 2));   /* no text node of its own */
    TEST_ASSERT_EQUAL(16, l63_tx("c"));
}

/* tabs: kept under pre, pre-wrap and break-spaces up to the next stop */
static void test_l63_text_tabs(void) {
    lot52_render(L63_STYLE("pre{margin:0}") "<pre>a&#9;b\n\tc</pre><pre style='tab-size:2'>a\tb</pre><pre style='tab-size:20px'>a\tb</pre>"
                 "<pre style='tab-size:0'>a\tb</pre><p style='margin:0'>x\t\ty</p></body>", 320);
    l61_text_at("a       b", 0, 1);                        /* &#9; is a tab: column 8 */
    l61_text_at("        c", 0, 11);                       /* from the line start */
    l61_text_at("a b", 0, 21);                             /* stops every 2 spaces */
    l61_text_at("a  b", 0, 31);                            /* 20 px: two 8 px advances reach it */
    l61_text_at("ab", 0, 41);                              /* tab-size 0 removes tabs */
    l61_text_at("x y", 0, 51);                             /* collapsed like spaces */
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
    /* pre-wrap: a tab that does not fit starts the next line */
    lot52_render(L63_STYLE("pre{margin:0;width:24px;white-space:pre-wrap}") "<pre style='tab-size:1'>ab\tc\td</pre><pre>ab\tc</pre></body>", 320);
    l61_text_at("ab ", 0, 1);
    l61_text_at("c d", 0, 11);
    l61_text_at("        ", 0, 31);                        /* 8 spaces from x 0, not 6 */
    l61_text_at("c", 0, 41);
    /* intrinsic widths count tab stops too */
    lot52_render(L63_STYLE("p{margin:0}div{display:inline-block}") "<div id=a><p style='white-space:pre-wrap;tab-size:3'>\tx</p></div>"
                 "<div id=b><p style='white-space:pre'>ab\tc</p></div></body>", 320);
    TEST_ASSERT_EQUAL(32, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(72, lot52_node("b")->w);
    /* letter-spacing -4px: a 255 px tab needs 63 advances, painted as 32 */
    lot52_render(L63_STYLE("p{margin:0;white-space:pre;letter-spacing:-4px;tab-size:255px}") "<p>a\tb</p></body>", 320);
    TEST_ASSERT_EQUAL(4 + 32 * 4, l63_tx("b") - l63_tx("a"));
    /* a collapsed tab is a break opportunity in min-content too */
    lot52_render(L63_STYLE("p{margin:0}div{display:inline-block;width:min-content}") "<div id=a><p>aa\tbb</p></div></body>", 320);
    TEST_ASSERT_EQUAL(16, lot52_node("a")->w);
}

static void test_l63_text_tab_size_values(void) {
    static const char *const bad[] = { "-1", "32", "2.5", "10%", "300px", "4pt", "abc", "+4px", "1 2", "100", "031", "-4px", "10%px", "256px" };  /* an empty value never reaches the property */
    unsigned k;
    lot52_render(L63_STYLE("") "<div id=a style='tab-size:4'><p id=b>x</p></div><div id=c style='tab-size:1em'></div><div id=d style='tab-size:31'></div>"
                 "<div id=e style='tab-size:255px'></div><div id=f></div></body>", 320);
    TEST_ASSERT_EQUAL(32, lot52_node("a")->tab_px);
    TEST_ASSERT_EQUAL(32, lot52_node("b")->tab_px);        /* inherited */
    TEST_ASSERT_EQUAL(10, lot52_node("c")->tab_px);        /* parse_len reads 1em as 10px */
    TEST_ASSERT_EQUAL(248, lot52_node("d")->tab_px);
    TEST_ASSERT_EQUAL(255, lot52_node("e")->tab_px);
    TEST_ASSERT_EQUAL(64, lot52_node("f")->tab_px);        /* 8 spaces by default */
    for (k = 0; k < sizeof(bad) / sizeof(bad[0]); k++) {
        char html[160];
        snprintf(html, sizeof(html), "<body><div id=a style='tab-size:3;tab-size:%s'></div></body>", bad[k]);
        lot52_render(html, 320);
        L63_CASE(24 == lot52_node("a")->tab_px, bad[k]);
        L63_CASE(1 == l61_diag_count("tab size refused"), bad[k]);
    }
}

/* break-spaces: preserved spaces fill the line and continue on the next */
static void test_l63_text_break_spaces(void) {
    lot52_render(L63_STYLE("p{margin:0;width:40px}") "<p id=a style='white-space:break-spaces'>ab      cd</p>"
                 "<p id=b style='white-space:pre-wrap'>ab      cd</p><p id=c style='white-space-collapse:break-spaces'>a  b</p>"
                 "<p id=d style='white-space:break-spaces;text-wrap-mode:nowrap'>a</p><p id=e style='white-space:break-spaces;text-wrap:wrap'>a</p>"
                 "<p id=f style='text-wrap-mode:nowrap;white-space-collapse:break-spaces'>a</p></body>", 320);
    TEST_ASSERT_EQUAL(WE_WS_BREAK_SPACES, lot52_node("a")->ws_mode);
    l61_text_at("ab   ", 0, 1);                             /* 3 spaces fill the 5 columns */
    l61_text_at("   cd", 0, 11);
    l61_text_at("ab", 0, 21);                              /* pre-wrap moves the run whole */
    l61_text_at("      ", 0, 31);
    l61_text_at("cd", 0, 41);
    TEST_ASSERT_EQUAL(WE_WS_BREAK_SPACES, lot52_node("c")->ws_mode);
    l61_text_at("a  b", 0, 51);
    TEST_ASSERT_EQUAL(WE_WS_PRE, lot52_node("d")->ws_mode);
    TEST_ASSERT_EQUAL(WE_WS_BREAK_SPACES, lot52_node("e")->ws_mode);
    TEST_ASSERT_EQUAL(WE_WS_PRE, lot52_node("f")->ws_mode);  /* the collapse longhand keeps nowrap */
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
    lot52_render(L63_STYLE("p{margin:0}div{display:inline-block}") "<div id=a><p style='white-space:break-spaces'>a   b</p></div>"
                 "<div id=b style='width:min-content'><p style='white-space:break-spaces'>ab   cd</p></div></body>", 320);
    TEST_ASSERT_EQUAL(40, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(16, lot52_node("b")->w);            /* every space is a break */
    /* the line room is read after the float band is entered, as for words */
    lot52_render(L63_STYLE("p{margin:0;width:40px}") "<p style='white-space:break-spaces'>"
                 "<span style='float:left;width:16px;height:10px'></span>          x</p></body>", 320);
    l61_text_at("   ", 16, 1);                             /* 24 px beside the float */
    l61_text_at("     ", 0, 11);
    l61_text_at("  x", 0, 21);
}

static void test_l63_text_white_space_longhands(void) {
    lot52_render(L63_STYLE("div{width:40px}") "<div id=a style='white-space-collapse:preserve'>a  b c d e f</div>"
                 "<div id=b style='text-wrap:nowrap'>a b c d e f g</div><div id=c style='white-space:pre;text-wrap-mode:wrap'>x</div>"
                 "<div id=d style='white-space:pre-wrap;text-wrap-mode:nowrap'>y</div><div id=e style='text-wrap-mode:nowrap;white-space-collapse:preserve'>z</div>"
                 "<div id=f style='white-space:pre;white-space-collapse:collapse'>w</div><div id=g style='white-space-collapse:preserve-breaks'>v</div>"
                 "<div id=h style='white-space:nowrap;text-wrap:balance'>u</div><div id=i style='white-space:nowrap;text-wrap:pretty'>t</div>"
                 "<div id=j style='white-space:nowrap;text-wrap:stable'>s</div></body>", 320);
    TEST_ASSERT_EQUAL(WE_WS_PRE_WRAP, lot52_node("a")->ws_mode);
    TEST_ASSERT_NOT_NULL(find_text("a  b "));             /* spaces kept, lines still wrap */
    TEST_ASSERT_NOT_NULL(find_text("a b c d e f g"));     /* nowrap */
    TEST_ASSERT_EQUAL(1, lot52_node("b")->nowrap);
    TEST_ASSERT_EQUAL(WE_WS_PRE_WRAP, lot52_node("c")->ws_mode);
    TEST_ASSERT_EQUAL(0, lot52_node("c")->nowrap);
    TEST_ASSERT_EQUAL(WE_WS_PRE, lot52_node("d")->ws_mode);
    TEST_ASSERT_EQUAL(1, lot52_node("d")->nowrap);
    TEST_ASSERT_EQUAL(WE_WS_PRE, lot52_node("e")->ws_mode);
    TEST_ASSERT_EQUAL(WE_WS_NORMAL, lot52_node("f")->ws_mode);
    TEST_ASSERT_EQUAL(1, lot52_node("f")->nowrap);
    TEST_ASSERT_EQUAL(WE_WS_PRE_LINE, lot52_node("g")->ws_mode);
    TEST_ASSERT_EQUAL(0, lot52_node("h")->nowrap);
    TEST_ASSERT_EQUAL(0, lot52_node("i")->nowrap);
    TEST_ASSERT_EQUAL(0, lot52_node("j")->nowrap);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
    lot52_render(L63_STYLE("") "<div id=a style='white-space:pre;white-space-collapse:discard;text-wrap-mode:balance;text-wrap:wavy'>x</div></body>", 320);
    TEST_ASSERT_EQUAL(WE_WS_PRE, lot52_node("a")->ws_mode);
    TEST_ASSERT_EQUAL(1, lot52_node("a")->nowrap);
    TEST_ASSERT_EQUAL(3, l61_diag_count("white space refused"));   /* balance is text-wrap only */
}

static void test_l63_text_word_wrap_and_break(void) {
    lot52_render(L63_STYLE("div{width:24px}") "<div id=a style='word-wrap:break-word'>abcdefgh</div>"
                 "<div id=b style='word-break:break-all;word-break:keep-all'>abc defgh</div><div id=c style='word-break:break-all;word-break:break-word'>abcdefgh</div></body>", 320);
    TEST_ASSERT_EQUAL(WE_WRAP_BREAK_WORD, lot52_node("a")->break_word);   /* the legacy alias */
    TEST_ASSERT_EQUAL(30, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->break_all);
    TEST_ASSERT_EQUAL(WE_WRAP_ANYWHERE, lot52_node("c")->break_word);
    TEST_ASSERT_EQUAL(0, lot52_node("c")->break_all);
    TEST_ASSERT_EQUAL(8, lot52_node("c")->min_content);   /* anywhere: breaks count in min-content */
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
    lot52_render(L63_STYLE("") "<div id=a style='word-wrap:break-word;word-wrap:loose'>x</div></body>", 320);
    TEST_ASSERT_EQUAL(1, l61_diag_count("word wrapping refused"));
}

static void lot63_text_run(void) {
    RUN_TEST(test_l63_text_character_references);
    RUN_TEST(test_l63_text_reference_spaces_and_newline);
    RUN_TEST(test_l63_text_overflow_values);
    RUN_TEST(test_l63_text_white_space_longhands);
    RUN_TEST(test_l63_text_word_wrap_and_break);
    RUN_TEST(test_l63_text_overflow_restyle);
    RUN_TEST(test_l63_text_nbsp_chunk);
    RUN_TEST(test_l63_text_tabs);
    RUN_TEST(test_l63_text_tab_size_values);
    RUN_TEST(test_l63_text_break_spaces);
}
