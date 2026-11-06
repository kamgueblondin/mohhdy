/* Lot 74 extended proofs */


static void test_l74_pattern_braces(void) {
    l74_open(L74_BAD "<form><input id=a pattern='[A-Z]{2,4}' value=A>"
             "<input id=b pattern='\\d{3}' value=12>"
             "<input id=c pattern='x{2}' value=xx>"
             "<input id=d pattern='a{0,2}' value=>"
             "<input id=go type=submit></form>");
    TEST_ASSERT_EQUAL(1, l74_bad("a"));
    TEST_ASSERT_EQUAL(1, l74_bad("b"));
    TEST_ASSERT_EQUAL(0, l74_bad("c"));
    TEST_ASSERT_EQUAL(0, l74_bad("d"));
    (void)l74_click("a"); l74_type("BC");
    TEST_ASSERT_EQUAL(0, l74_bad("a"));
    l74_done();
}


static void test_l74_nest_0(void) {
    lot52_render("<style>a{color:#000001; &:hover{color:#000002}}</style>" L64_BODY "<a id=x>x</a></body>", 400);
    TEST_ASSERT_EQUAL(1, l73c("x"));
}


static void test_l74_nest_1(void) {
    lot52_render("<style>ul{color:#000001; & > li{color:#000002}}</style>" L64_BODY "<ul id=u><li id=a>a</li></ul></body>", 400);
    TEST_ASSERT_EQUAL(2, l73c("a"));
}


static void test_l74_nest_2(void) {
    lot52_render("<style>div{color:#000001; &.on{color:#000002}}</style>" L64_BODY "<div id=a class=on>a</div></body>", 400);
    TEST_ASSERT_EQUAL(2, l73c("a"));
}


static void test_l74_scrollbar_hidden_no_bar(void) {
    int i, bars = 0;
    lot52_render("<style>#b{overflow:hidden;height:30px;width:80px}</style>" L64_BODY
                 "<div id=b><p>1</p><p>2</p><p>3</p><p>4</p></div></body>", 200);
    for (i = 0; i < (int)g_doc.nitems; i++)
        if (g_doc.item_owner[i] == (int)(lot52_node("b") - g_doc.nodes) && g_doc.items[i].w == 6) bars++;
    TEST_ASSERT_EQUAL(0, bars);
}
static void test_l74_scrollbar_auto_overflow(void) {
    int i, bars = 0;
    l74_open("<style>#b{overflow:auto;height:30px;width:80px}#b p{height:20px;margin:0}</style>"
             "<div id=b><p>1</p><p>2</p><p>3</p></div>");
    for (i = 0; i < (int)g_doc.nitems; i++)
        if (g_doc.item_owner[i] == l74_node("b") && g_doc.items[i].kind == OS_FB_WEB_RECT && g_doc.items[i].w == 6) bars++;
    TEST_ASSERT_TRUE(bars >= 2);
    l74_done();
}


static void test_l74_iframe_style_in_srcdoc(void) {
    lot52_render("<body><iframe id=f srcdoc='<style>p{color:#000007}</style><p id=p>p</p>'></iframe></body>", 400);
    TEST_ASSERT_EQUAL(7, l73c("p"));
}
static void test_l74_iframe_form_in_srcdoc(void) {
    l74_open("<iframe id=f srcdoc='<form action=/x><input id=q name=q value=hi><input id=go type=submit></form>'></iframe>");
    TEST_ASSERT_TRUE(l74_node("q") > 0);
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l74_click("go"));
    l74_done();
}


static void test_l74_table_fixed_pct(void) {
    lot52_render("<style>table{table-layout:fixed;width:200px}td{border:0}</style>" L64_BODY
                 "<table id=t><tr><td id=a style='width:25%'>a</td><td id=b>b</td><td id=c>c</td><td id=d>d</td></tr>"
                 "<tr><td id=e>eeeeeeeeee</td><td>f</td><td>g</td><td>h</td></tr></table></body>", 400);
    TEST_ASSERT_EQUAL(g_doc.nodes[lot52_node("a") - g_doc.nodes].w,
                      g_doc.nodes[lot52_node("e") - g_doc.nodes].w);
}
static void test_l74_table_layout_refused(void) {
    lot52_render("<style>table{table-layout:weird}</style>" L64_BODY "<table id=t><tr><td>x</td></tr></table></body>", 200);
    TEST_ASSERT_EQUAL(1, l61_diag_count("table layout refused"));
}


static void test_l74_dialog_formmethod(void) {
    l74_open("<dialog id=d open><form><button id=ok type=submit formmethod=dialog>ok</button></form></dialog>");
    TEST_ASSERT_EQUAL(WE_FACT_CHANGED, l74_click("ok"));
    l74_render();
    TEST_ASSERT_EQUAL(WE_DISP_NONE, g_doc.nodes[l74_node("d")].display);
    l74_done();
}


static void test_l74_accent_range(void) {
    l74_open("<style>input{accent-color:#abcdef}</style>"
             "<form><input id=r type=range min=0 max=100 value=50></form>");
    TEST_ASSERT_EQUAL(1, g_doc.nodes[l74_node("r")].accent_set);
    l74_done();
}


static void test_l74_font_face_and_rules(void) {
    lot52_render("<style>@font-face{font-family:A;src:url(a)}@font-face{font-family:B;src:local(B)}"
                 "p{color:#000009}</style>" L64_BODY "<p id=a>a</p></body>", 200);
    TEST_ASSERT_EQUAL(2, l61_diag_count("font face refused"));
    TEST_ASSERT_EQUAL(9, l73c("a"));
}

static void lot74_ext_tests(void) {
    RUN_TEST(test_l74_pattern_braces);
    RUN_TEST(test_l74_nest_0);
    RUN_TEST(test_l74_nest_1);
    RUN_TEST(test_l74_nest_2);
    RUN_TEST(test_l74_scrollbar_hidden_no_bar);
    RUN_TEST(test_l74_scrollbar_auto_overflow);
    RUN_TEST(test_l74_iframe_style_in_srcdoc);
    RUN_TEST(test_l74_iframe_form_in_srcdoc);
    RUN_TEST(test_l74_table_fixed_pct);
    RUN_TEST(test_l74_table_layout_refused);
    RUN_TEST(test_l74_dialog_formmethod);
    RUN_TEST(test_l74_accent_range);
    RUN_TEST(test_l74_font_face_and_rules);
}
