/* Lot 63 I: soft hyphens and the hyphens property. */
#define L63_H_P(style, text) "<p style='margin:0;" style "'>" text "</p>"

static void test_l63_hyphen_breaks_with_a_hyphen(void) {
    lot52_render("<body style='margin:0'>" L63_H_P("width:56px", "xx hyph&shy;en&shy;ation ok") "</body>", 320);
    TEST_ASSERT_EQUAL(4, (int)g_doc.nitems);
    l61_text_at("xx", 0, 1);                                 /* "xx hyph-" needs 8 columns: break at the space */
    l61_text_at("hyphen-", 0, 11);
    l61_text_at("ation", 0, 21);
    l61_text_at("ok", 0, 31);
    /* a piece must fit with its hyphen; the last one without */
    lot52_render("<body style='margin:0'>" L63_H_P("width:40px", "abc&shy;de&shy;f") "</body>", 320);
    l61_text_at("abc-", 0, 1);
    l61_text_at("def", 0, 11);
    /* leading, doubled and trailing soft hyphens give one break */
    lot52_render("<body style='margin:0'>" L63_H_P("width:24px", "&shy;ab&shy;&shy;cd&shy;") "</body>", 320);
    TEST_ASSERT_EQUAL(2, (int)g_doc.nitems);
    l61_text_at("ab-", 0, 1);
    l61_text_at("cd", 0, 11);
    /* a word that fits keeps no hyphen */
    lot52_render("<body style='margin:0'>" L63_H_P("width:320px", "a&#173;b") "</body>", 320);
    TEST_ASSERT_EQUAL(1, (int)g_doc.nitems);
    l61_text_at("ab", 0, 1);
    lot52_render("<body style='margin:0'>" L63_H_P("width:320px", "x ab&shy;cd") "</body>", 320);
    l61_text_at("x abcd", 0, 1);                             /* no space between pieces */
    /* trailing soft hyphens reserve no hyphen for the last piece */
    lot52_render("<body style='margin:0'>" L63_H_P("width:32px", "ab&shy;cd&shy;&shy;") "</body>", 320);
    l61_text_at("abcd", 0, 1);
    /* a first piece wider than the line still gets its hyphen on that line */
    lot52_render("<body style='margin:0'>" L63_H_P("width:16px", "abcd&shy;ef") "</body>", 320);
    l61_text_at("abcd-", 0, 1);
    l61_text_at("ef", 0, 11);
}

static void test_l63_hyphen_letter_spacing(void) {
    lot52_render("<body style='margin:0'>" L63_H_P("width:32px;letter-spacing:2px", "ab&shy;cd") "</body>", 320);
    l61_text_at("-", 20, 1);                                 /* 10px advances: "ab-" is 30px */
    l61_text_at("c", 0, 11);
    lot52_render("<body style='margin:0'>" L63_H_P("width:40px;letter-spacing:2px", "ab&shy;cd") "</body>", 320);
    TEST_ASSERT_NULL(find_text("-"));                         /* 40px: the word fits */
    l61_text_at("d", 30, 1);
}

static void test_l63_hyphen_glued_forms(void) {
    lot52_render("<body style='margin:0'>" L63_H_P("width:40px;hyphens:none", "x abc&shy;def") "</body>", 320);
    l61_text_at("x", 0, 1);
    l61_text_at("abcdef", 0, 11);                            /* moved whole, then overflowing */
    lot52_render("<body style='margin:0'>" L63_H_P("width:56px;hyphens:none", "x abc&shy;def") "</body>", 320);
    l61_text_at("abcdef", 0, 11);                            /* 8 + 8 + 48 > 56: the space counts */
    lot52_render("<body style='margin:0'>" L63_H_P("width:320px;hyphens:none", "x abc&shy;def") "</body>", 320);
    l61_text_at("x abcdef", 0, 1);
    lot52_render("<body style='margin:0'>" L63_H_P("width:40px;white-space:nowrap", "x abc&shy;def") "</body>", 320);
    l61_text_at("x abcdef", 0, 1);
    lot52_render("<body style='margin:0'>" L63_H_P("width:56px;word-break:break-all", "xx hyph&shy;enation ok") "</body>", 320);
    l61_text_at("xx hyph", 0, 1);                            /* break-all: no hyphen */
    l61_text_at("enation", 0, 11);
    lot52_render("<body style='margin:0'>" L63_H_P("width:56px;overflow-wrap:break-word;hyphens:none", "xx hyph&shy;enation ok") "</body>", 320);
    l61_text_at("xx hyph", 0, 1);                            /* pieces, no hyphen */
    l61_text_at("enation", 0, 11);
    lot52_render("<body style='margin:0'>" L63_H_P("width:56px;overflow-wrap:break-word", "xx hyph&shy;en&shy;ation ok") "</body>", 320);
    l61_text_at("hyphen-", 0, 11);
}

static void test_l63_hyphen_min_content(void) {
    lot52_render(L63_STYLE("div{display:inline-block;width:min-content}") "<div id=a>hyph&shy;enation</div><br>"
                 "<div id=b style='hyphens:none'>hyph&shy;enation</div><br><div id=c>&shy;ab&shy;&shy;cd&shy;</div><br>"
                 "<div id=d style='letter-spacing:2px'>abcd&shy;e</div><br><div id=e>ab&shy; cd ab&shy;&shy; c</div></body>", 320);
    TEST_ASSERT_EQUAL(56, lot52_node("a")->w);               /* "enation" */
    TEST_ASSERT_EQUAL(88, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(24, lot52_node("c")->w);               /* "ab-" */
    TEST_ASSERT_EQUAL(50, lot52_node("d")->w);               /* "abcd-" at 10px */
    TEST_ASSERT_EQUAL(16, lot52_node("e")->w);               /* no break before a space or at the end */
}

static void test_l63_hyphen_values_and_refs(void) {
    lot52_render(L63_STYLE("") "<div id=a style='hyphens:none'><p id=b>x</p></div><div id=c style='hyphens:none;hyphens:auto'></div>"
                 "<div id=d style='hyphens:none;hyphens:manual'></div><div id=e style='hyphens:none;hyphens:all'></div>"
                 "<p id=f>&shy;</p><p id=g>a&shy;</p></body>", 320);
    TEST_ASSERT_EQUAL(WE_HYPHENS_NONE, lot52_node("a")->hyphens);
    TEST_ASSERT_EQUAL(WE_HYPHENS_NONE, lot52_node("b")->hyphens);    /* inherited */
    TEST_ASSERT_EQUAL(WE_HYPHENS_MANUAL, lot52_node("c")->hyphens);
    TEST_ASSERT_EQUAL(WE_HYPHENS_MANUAL, lot52_node("d")->hyphens);
    TEST_ASSERT_EQUAL(WE_HYPHENS_NONE, lot52_node("e")->hyphens);
    TEST_ASSERT_EQUAL(1, l61_diag_count("hyphens refused"));
    TEST_ASSERT_EQUAL(0, lot52_node("f")->h);                        /* a soft hyphen alone is no glyph */
    TEST_ASSERT_EQUAL(-1, lot52_node("f")->first);                   /* and makes no text node */
    TEST_ASSERT_EQUAL(2, (int)g_doc.nitems);                         /* "x" and "a" */
    l61_text_at("a", 0, 51);
    TEST_ASSERT_EQUAL(2, g_doc.nodes[lot52_node("g")->first].text_len);
    TEST_ASSERT_EQUAL(WE_SHY, g_doc.pool[g_doc.nodes[lot52_node("g")->first].text_off + 1]);
}

static void lot63_hyphen_run(void) {
    RUN_TEST(test_l63_hyphen_breaks_with_a_hyphen);
    RUN_TEST(test_l63_hyphen_letter_spacing);
    RUN_TEST(test_l63_hyphen_glued_forms);
    RUN_TEST(test_l63_hyphen_min_content);
    RUN_TEST(test_l63_hyphen_values_and_refs);
}
