/* Lot 74: @font-face refusal, table-layout:fixed, pattern=, dialog */

static void test_l74_font_face(void) {
    lot52_render("<style>@font-face{font-family:X;src:url(a.woff)}p{color:#000001}</style>"
                 L64_BODY "<p id=a>a</p></body>", 320);
    TEST_ASSERT_EQUAL(1, l61_diag_count("font face refused"));
    TEST_ASSERT_EQUAL(1, l73c("a"));
}

static void test_l74_table_fixed(void) {
    /* fixed: first row widths win; a later wide cell does not grow col 0 */
    lot52_render("<style>table{table-layout:fixed;width:200px;border-collapse:collapse}"
                 "td{border:1px solid #000}</style>" L64_BODY
                 "<table id=t><tr><td id=a style='width:40px'>a</td><td id=b>b</td></tr>"
                 "<tr><td id=c>wwwwwwwwwwwwwwwwwwww</td><td id=d>d</td></tr></table></body>", 400);
    TEST_ASSERT_EQUAL(1, g_doc.nodes[lot52_node("t") - g_doc.nodes].table_fixed);
    TEST_ASSERT_EQUAL(g_doc.nodes[lot52_node("a") - g_doc.nodes].w,
                      g_doc.nodes[lot52_node("c") - g_doc.nodes].w);
    TEST_ASSERT_TRUE(g_doc.nodes[lot52_node("a") - g_doc.nodes].w < 80);
    /* auto: the wide cell grows the column */
    lot52_render("<style>table{table-layout:auto;width:200px}"
                 "td{border:1px solid #000}</style>" L64_BODY
                 "<table id=t><tr><td id=a style='width:40px'>a</td><td id=b>b</td></tr>"
                 "<tr><td id=c>wwwwwwwwwwwwwwwwwwww</td><td id=d>d</td></tr></table></body>", 400);
    TEST_ASSERT_EQUAL(0, g_doc.nodes[lot52_node("t") - g_doc.nodes].table_fixed);
    TEST_ASSERT_TRUE(g_doc.nodes[lot52_node("c") - g_doc.nodes].w >
                     g_doc.nodes[lot52_node("a") - g_doc.nodes].w - 1);
}

static void test_l74_pattern(void) {
    l74_open(L74_BAD "<form id=f action=/p><input id=a name=a pattern='[A-Z][A-Z][A-Z]' value=AB>"
             "<input id=b name=b pattern='\\d+' value=12x><input id=c name=c pattern='a.*z' value=abcz>"
             "<input id=d name=d pattern='x?' value=>"
             "<input id=go type=submit></form>");
    TEST_ASSERT_EQUAL(1, l74_bad("a"));
    TEST_ASSERT_EQUAL(1, l74_bad("b"));
    TEST_ASSERT_EQUAL(0, l74_bad("c"));
    TEST_ASSERT_EQUAL(0, l74_bad("d"));          /* empty: required alone */
    (void)l74_click("a"); l74_type("C");
    TEST_ASSERT_EQUAL(0, l74_bad("a"));
    TEST_ASSERT_EQUAL(WE_FACT_CHANGED, l74_click("go"));  /* b still bad */
    TEST_ASSERT_EQUAL(l74_ctl("b"), g_l74f.focus);
    (void)l74_click("b");
    while (l74_val("b")[0]) l74_key(WE_KEY_BACKSPACE);
    l74_type("99");
    TEST_ASSERT_EQUAL(0, l74_bad("b"));
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l74_click("go"));
    l74_done();
}

static void test_l74_dialog(void) {
    l74_open("<dialog id=d><p>hi</p><form method=dialog><button id=ok>ok</button></form></dialog>"
             "<button id=open>open</button>");
    TEST_ASSERT_EQUAL(WE_DISP_NONE, g_doc.nodes[l74_node("d")].display);
    /* open attribute */
    l74_done();
    l74_open("<dialog id=d open><form method=dialog><button id=ok type=submit>ok</button></form></dialog>");
    TEST_ASSERT_EQUAL(WE_DISP_BLOCK, g_doc.nodes[l74_node("d")].display);
    TEST_ASSERT_EQUAL(WE_FACT_CHANGED, l74_click("ok"));
    l74_render();
    TEST_ASSERT_EQUAL(0, g_doc.nodes[l74_node("d")].display != WE_DISP_NONE);
    TEST_ASSERT_EQUAL(WE_DISP_NONE, g_doc.nodes[l74_node("d")].display);
    l74_done();
    /* click the dialog itself toggles */
    l74_open("<dialog id=d open>x</dialog>");
    TEST_ASSERT_EQUAL(WE_FACT_CHANGED, l74_click("d"));
    l74_render();
    TEST_ASSERT_EQUAL(WE_DISP_NONE, g_doc.nodes[l74_node("d")].display);
    l74_done();
}

static void lot74_more_tests(void) {
    RUN_TEST(test_l74_font_face);
    RUN_TEST(test_l74_table_fixed);
    RUN_TEST(test_l74_pattern);
    RUN_TEST(test_l74_dialog);
}
