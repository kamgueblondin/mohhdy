/* Lot 74: @font-face, table-layout:fixed, pattern=, dialog, scrollbars, iframe srcdoc */

static void test_l74_font_face(void) {
    lot52_render("<style>@font-face{font-family:X;src:url(a.woff)}p{color:#000001}</style>"
                 L64_BODY "<p id=a>a</p></body>", 320);
    TEST_ASSERT_EQUAL(1, l61_diag_count("font face refused"));
    TEST_ASSERT_EQUAL(1, l73c("a"));
}

static void test_l74_table_fixed(void) {
    lot52_render("<style>table{table-layout:fixed;width:200px;border-collapse:collapse}"
                 "td{border:1px solid #000}</style>" L64_BODY
                 "<table id=t><tr><td id=a style='width:40px'>a</td><td id=b>b</td></tr>"
                 "<tr><td id=c>wwwwwwwwwwwwwwwwwwww</td><td id=d>d</td></tr></table></body>", 400);
    TEST_ASSERT_EQUAL(1, g_doc.nodes[lot52_node("t") - g_doc.nodes].table_fixed);
    TEST_ASSERT_EQUAL(g_doc.nodes[lot52_node("a") - g_doc.nodes].w,
                      g_doc.nodes[lot52_node("c") - g_doc.nodes].w);
    TEST_ASSERT_TRUE(g_doc.nodes[lot52_node("a") - g_doc.nodes].w < 80);
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
    TEST_ASSERT_EQUAL(0, l74_bad("d"));
    (void)l74_click("a"); l74_type("C");
    TEST_ASSERT_EQUAL(0, l74_bad("a"));
    TEST_ASSERT_EQUAL(WE_FACT_CHANGED, l74_click("go"));
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
    l74_done();
    l74_open("<dialog id=d open><form method=dialog><button id=ok type=submit>ok</button></form></dialog>");
    TEST_ASSERT_EQUAL(WE_DISP_BLOCK, g_doc.nodes[l74_node("d")].display);
    TEST_ASSERT_EQUAL(WE_FACT_CHANGED, l74_click("ok"));
    l74_render();
    TEST_ASSERT_EQUAL(WE_DISP_NONE, g_doc.nodes[l74_node("d")].display);
    l74_done();
    l74_open("<dialog id=d open>x</dialog>");
    TEST_ASSERT_EQUAL(WE_FACT_CHANGED, l74_click("d"));
    l74_render();
    TEST_ASSERT_EQUAL(WE_DISP_NONE, g_doc.nodes[l74_node("d")].display);
    l74_done();
}

static void test_l74_scrollbar(void) {
    int x, y, i, bars = 0;
    l74_open("<style>#box{overflow:scroll;height:40px;width:120px}"
             "#box p{margin:0;height:20px}</style>"
             "<div id=box><p id=a>a</p><p id=b>b</p><p id=c>c</p><p id=d>d</p></div>");
    TEST_ASSERT_TRUE(g_doc.nodes[l74_node("box")].overflow_scroll & 2);
    for (i = 0; i < (int)g_doc.nitems; i++)
        if (g_doc.item_owner[i] == l74_node("box") && g_doc.items[i].kind == OS_FB_WEB_RECT &&
            g_doc.items[i].w == 6) bars++;
    TEST_ASSERT_TRUE(bars >= 2);
    x = g_doc.nodes[l74_node("box")].x + g_doc.nodes[l74_node("box")].w - 3;
    y = g_doc.nodes[l74_node("box")].y + g_doc.nodes[l74_node("box")].h - 4;
    TEST_ASSERT_EQUAL(2, we_box_scroll_click(&g_doc, x, y));
    TEST_ASSERT_TRUE(g_l74f.nscroll >= 1);
    l74_render();
    TEST_ASSERT_TRUE(g_doc.nodes[l74_node("box")].box_scroll_y > 0);
    TEST_ASSERT_TRUE(g_doc.nodes[l74_node("a")].y < g_doc.nodes[l74_node("box")].y +
                    g_doc.nodes[l74_node("box")].border + g_doc.nodes[l74_node("box")].padding[0]);
    l74_done();
    lot52_render("<style>#box{overflow:hidden;height:40px;width:120px}</style>"
                 L64_BODY "<div id=box><p>a</p><p>b</p><p>c</p><p>d</p></div></body>", 200);
    TEST_ASSERT_EQUAL(0, g_doc.nodes[lot52_node("box") - g_doc.nodes].overflow_scroll);
}

static void test_l74_iframe_srcdoc(void) {
    lot52_render("<body style='margin:0'><iframe id=f srcdoc='<p id=in style=\"color:#000001\">hi</p>'></iframe>"
                 "<iframe id=g src='http://x/'></iframe></body>", 400);
    TEST_ASSERT_TRUE(g_doc.nodes[lot52_node("f") - g_doc.nodes].attr_flags & 64);
    TEST_ASSERT_EQUAL(1, l73c("in"));
    TEST_ASSERT_EQUAL(1, l61_diag_count("iframe src refused"));
    /* nested srcdoc refused */
    lot52_render("<body><iframe id=f srcdoc='<iframe srcdoc=\"<p id=x>x</p>\"></iframe><p id=y>y</p>'></iframe></body>", 400);
    TEST_ASSERT_EQUAL(1, l61_diag_count("iframe nested srcdoc refused"));
    TEST_ASSERT_EQUAL(-1, we_find_id(&g_doc, "x"));
    TEST_ASSERT_TRUE(we_find_id(&g_doc, "y") > 0);
}


static void test_l74_css_nesting(void) {
    lot52_render("<style>p{color:#000001; & span{color:#000002} &.x{color:#000003} q{color:#000004}}"
                 "div{color:#000005; & p{color:#000006}}</style>"
                 L64_BODY "<p id=a>a<span id=b>b</span></p><p id=c class=x>c</p><div id=d><p id=e>e</p></div>"
                 "<p id=f><q id=g>g</q></p></body>", 400);
    TEST_ASSERT_EQUAL(1, l73c("a"));
    TEST_ASSERT_EQUAL(2, l73c("b"));
    TEST_ASSERT_EQUAL(3, l73c("c"));
    TEST_ASSERT_EQUAL(4, l73c("g"));
    TEST_ASSERT_EQUAL(6, l73c("e"));
}

static void test_l74_accent_appearance(void) {
    l74_open("<style>input{accent-color:#112233}#n{appearance:none; -webkit-appearance:none}</style>"
             "<form><input id=c type=checkbox checked><input id=n type=text value=x></form>");
    TEST_ASSERT_EQUAL(1, g_doc.nodes[l74_node("c")].accent_set);
    TEST_ASSERT_EQUAL(0x112233, g_doc.nodes[l74_node("c")].accent & 0xffffff);
    TEST_ASSERT_EQUAL(1, g_doc.nodes[l74_node("n")].appearance);
    TEST_ASSERT_EQUAL(0, g_doc.nodes[l74_node("n")].border);
    l74_done();
}

static void lot74_more_tests(void) {
    RUN_TEST(test_l74_font_face);
    RUN_TEST(test_l74_table_fixed);
    RUN_TEST(test_l74_pattern);
    RUN_TEST(test_l74_dialog);
    RUN_TEST(test_l74_scrollbar);
    RUN_TEST(test_l74_iframe_srcdoc);
    RUN_TEST(test_l74_css_nesting);
    RUN_TEST(test_l74_accent_appearance);
}
