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
    lot52_render("<body style='margin:0'><div id=p style='overflow:hidden;overflow:hidden visible clip;overflow-x:hidden auto;overflow-y:wavy;overflow:'></div></body>", 320);
    TEST_ASSERT_EQUAL(3, lot52_node("p")->overflow_axes);
    TEST_ASSERT_EQUAL(3, l61_diag_count("overflow refused"));
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
    lot52_render(L63_STYLE("") "<div id=a style='white-space:pre;white-space-collapse:break-spaces;text-wrap-mode:balance;text-wrap:wavy'>x</div></body>", 320);
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
}
