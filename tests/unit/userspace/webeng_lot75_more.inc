
#ifndef WE_FF2_FORM_ATTR
#define WE_FF2_FORM_ATTR 131072u
#define WE_FF2_FORMACTION 262144u
#endif

/* Lot 75: form= association, formaction/formenctype, CSS caret/user-select/
 * pointer-events/scroll-margin. */
static we_forms_t g_l75b;
static const char *g_l75b_html;
static void l75b_render(const char *html) {
    we_forms_init(&g_l75b);
    we_set_forms(&g_doc, &g_l75b);
    g_l75b_html = html;
    TEST_ASSERT_EQUAL(0, we_render(&g_doc, html, (int)strlen(html), 480));
}
static void l75b_done(void) { we_set_forms(&g_doc, 0); g_l75b_html = 0; }
static int l75b_node(const char *id) { int i = we_find_id(&g_doc, id); return i > 0 ? i : 0; }
static int l75b_ctl(const char *id) { return g_doc.nodes[l75b_node(id)].form_ord - 1; }
static void l75b_reflow(void) { TEST_ASSERT_EQUAL(0, we_render(&g_doc, g_l75b_html, (int)strlen(g_l75b_html), 480)); }
static int l75b_click(const char *id) {
    int i = l75b_node(id), x = g_doc.nodes[i].x + 2, y = g_doc.nodes[i].y + 2, rc;
    rc = we_form_click(&g_doc, x, y); l75b_reflow(); return rc;
}

static void test_l75_form_attr_association(void) {
    we_form_req_t q;
    int i, x, y;
    l75b_render("<form id=f1 action=/a method=GET></form>"
                "<form id=f2 action=/b method=POST></form>"
                "<input id=q name=q value=hi form=f1>"
                "<input id=s type=submit form=f1 value=Go>");
    TEST_ASSERT_TRUE(g_doc.nodes[l75b_node("q")].form_flags2 & WE_FF2_FORM_ATTR);
    /* submit belongs to f1 via form= */
    for (i = 1; i < (int)g_doc.nnodes; i++)
        if (g_doc.nodes[i].form_type == WE_FORM_SUBMIT) break;
    x = g_doc.nodes[i].x + 2; y = g_doc.nodes[i].y + 2;
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, we_form_click(&g_doc, x, y));
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_EQUAL(0, q.post);
    TEST_ASSERT_NOT_NULL(strstr(q.url, "/a?"));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "q=hi"));
    l75b_done();
}

static void test_l75_form_attr_missing_unbound(void) {
    we_form_req_t q;
    int i, x, y;
    /* form=missing: control unbound, submit of the page form should not include it */
    l75b_render("<form id=f action=/ok method=GET>"
                "<input name=in value=1><input id=s type=submit></form>"
                "<input id=out name=out value=2 form=nope>");
    for (i = 1; i < (int)g_doc.nnodes; i++)
        if (g_doc.nodes[i].form_type == WE_FORM_SUBMIT) break;
    x = g_doc.nodes[i].x + 2; y = g_doc.nodes[i].y + 2;
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, we_form_click(&g_doc, x, y));
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "in=1"));
    TEST_ASSERT_NULL(strstr(q.url, "out="));
    l75b_done();
}

static void test_l75_formaction_override(void) {
    we_form_req_t q;
    int i, x, y;
    l75b_render("<form action=/default method=GET>"
                "<input name=a value=1>"
                "<input id=s type=submit formaction=/other value=Go></form>");
    TEST_ASSERT_TRUE(g_doc.nodes[l75b_node("s")].form_flags2 & WE_FF2_FORMACTION);
    for (i = 1; i < (int)g_doc.nnodes; i++)
        if (g_doc.nodes[i].form_type == WE_FORM_SUBMIT) break;
    x = g_doc.nodes[i].x + 2; y = g_doc.nodes[i].y + 2;
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, we_form_click(&g_doc, x, y));
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "/other?"));
    TEST_ASSERT_NULL(strstr(q.url, "/default"));
    l75b_done();
}

static void test_l75_formenctype_multipart(void) {
    we_form_req_t q;
    int i, x, y;
    l75b_render("<form action=/p method=POST>"
                "<input name=a value=x>"
                "<input id=s type=submit formenctype=multipart/form-data formmethod=post></form>");
    for (i = 1; i < (int)g_doc.nnodes; i++)
        if (g_doc.nodes[i].form_type == WE_FORM_SUBMIT) break;
    x = g_doc.nodes[i].x + 2; y = g_doc.nodes[i].y + 2;
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, we_form_click(&g_doc, x, y));
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_EQUAL(1, q.post);
    TEST_ASSERT_EQUAL(1, q.multipart);
    TEST_ASSERT_NOT_NULL(strstr(q.body, "name=\"a\""));
    l75b_done();
}

static void test_l75_caret_color(void) {
    lot52_render("<style>input{caret-color:#ff0000}#b{caret-color:auto}</style>"
                 "<input id=a value=x><input id=b value=y>", 400);
    TEST_ASSERT_EQUAL(1, lot52_node("a")->caret_set);
    TEST_ASSERT_EQUAL(0xff0000u, lot52_node("a")->caret);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->caret_set);
}

static void test_l75_user_select(void) {
    lot52_render("<style>#a{user-select:none}#b{-webkit-user-select:text}#c{user-select:all}"
                 "#d{user-select:auto}#e{user-select:bogus}</style>"
                 "<p id=a>a</p><p id=b>b</p><p id=c>c</p><p id=d>d</p><p id=e>e</p>", 400);
    TEST_ASSERT_EQUAL(1, lot52_node("a")->user_select);
    TEST_ASSERT_EQUAL(2, lot52_node("b")->user_select);
    TEST_ASSERT_EQUAL(3, lot52_node("c")->user_select);
    TEST_ASSERT_EQUAL(0, lot52_node("d")->user_select);
    TEST_ASSERT_TRUE(l61_diag_count("user-select refused") >= 1);
}

static void test_l75_pointer_events_none(void) {
    int x, y;
    lot52_render("<style>a{pointer-events:none}</style><a id=a href=/x>link</a><p id=p>p</p>", 400);
    TEST_ASSERT_EQUAL(1, lot52_node("a")->pointer_events);
    x = lot52_node("a")->x + 2; y = lot52_node("a")->y + 2;
    {
        char href[64];
        TEST_ASSERT_EQUAL(0, we_link_at(&g_doc, x, y, href, (int)sizeof(href)));
    }
}

static void test_l75_scroll_margin_padding(void) {
    lot52_render("<style>#a{scroll-margin:10px}#b{scroll-margin-top:4px;scroll-margin-left:2px}"
                 "#c{scroll-padding:1px 2px 3px 4px}#d{scroll-padding-bottom:8px}"
                 "#e{scroll-margin:bogus}</style>"
                 "<div id=a></div><div id=b></div><div id=c></div><div id=d></div><div id=e></div>", 400);
    TEST_ASSERT_EQUAL(1, lot52_node("a")->scroll_m_set);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->scroll_m[0]);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->scroll_m[1]);
    TEST_ASSERT_EQUAL(4, lot52_node("b")->scroll_m[0]);
    TEST_ASSERT_EQUAL(2, lot52_node("b")->scroll_m[3]);
    TEST_ASSERT_EQUAL(1, lot52_node("c")->scroll_p_set);
    TEST_ASSERT_EQUAL(1, lot52_node("c")->scroll_p[0]);
    TEST_ASSERT_EQUAL(2, lot52_node("c")->scroll_p[1]);
    TEST_ASSERT_EQUAL(3, lot52_node("c")->scroll_p[2]);
    TEST_ASSERT_EQUAL(4, lot52_node("c")->scroll_p[3]);
    TEST_ASSERT_EQUAL(8, lot52_node("d")->scroll_p[2]);
    TEST_ASSERT_TRUE(l61_diag_count("scroll margin refused") >= 1);
}

static void test_l75_css_case_1(void) {
    char html[512];
    unsigned want = (unsigned)(0x100000u + 1 * 17u);
    sprintf(html, "<style>#t{caret-color:#%06x;user-select:%s;pointer-events:%s;"
                  "scroll-margin:%dpx;scroll-padding:%dpx %dpx}</style><p id=t>x</p>",
            (unsigned)(want & 0xffffffu),
            (1 % 4 == 0) ? "none" : (1 % 4 == 1) ? "text" : (1 % 4 == 2) ? "all" : "auto",
            (1 % 2) ? "none" : "auto",
            1 % 30, (1 * 3) % 20, (1 * 5) % 20);
    lot52_render(html, 400);
    TEST_ASSERT_EQUAL(1, lot52_node("t")->caret_set);
    TEST_ASSERT_EQUAL(want & 0xffffffu, lot52_node("t")->caret);
    TEST_ASSERT_EQUAL((1 % 4 == 0) ? 1 : (1 % 4 == 1) ? 2 : (1 % 4 == 2) ? 3 : 0,
                      lot52_node("t")->user_select);
    TEST_ASSERT_EQUAL((1 % 2) ? 1 : 0, lot52_node("t")->pointer_events);
    TEST_ASSERT_EQUAL(1, lot52_node("t")->scroll_m_set);
    TEST_ASSERT_EQUAL(1 % 30, lot52_node("t")->scroll_m[0]);
    TEST_ASSERT_EQUAL(1, lot52_node("t")->scroll_p_set);
}

static void test_l75_css_case_2(void) {
    char html[512];
    unsigned want = (unsigned)(0x100000u + 2 * 17u);
    sprintf(html, "<style>#t{caret-color:#%06x;user-select:%s;pointer-events:%s;"
                  "scroll-margin:%dpx;scroll-padding:%dpx %dpx}</style><p id=t>x</p>",
            (unsigned)(want & 0xffffffu),
            (2 % 4 == 0) ? "none" : (2 % 4 == 1) ? "text" : (2 % 4 == 2) ? "all" : "auto",
            (2 % 2) ? "none" : "auto",
            2 % 30, (2 * 3) % 20, (2 * 5) % 20);
    lot52_render(html, 400);
    TEST_ASSERT_EQUAL(1, lot52_node("t")->caret_set);
    TEST_ASSERT_EQUAL(want & 0xffffffu, lot52_node("t")->caret);
    TEST_ASSERT_EQUAL((2 % 4 == 0) ? 1 : (2 % 4 == 1) ? 2 : (2 % 4 == 2) ? 3 : 0,
                      lot52_node("t")->user_select);
    TEST_ASSERT_EQUAL((2 % 2) ? 1 : 0, lot52_node("t")->pointer_events);
    TEST_ASSERT_EQUAL(1, lot52_node("t")->scroll_m_set);
    TEST_ASSERT_EQUAL(2 % 30, lot52_node("t")->scroll_m[0]);
    TEST_ASSERT_EQUAL(1, lot52_node("t")->scroll_p_set);
}

static void test_l75_css_case_3(void) {
    char html[512];
    unsigned want = (unsigned)(0x100000u + 3 * 17u);
    sprintf(html, "<style>#t{caret-color:#%06x;user-select:%s;pointer-events:%s;"
                  "scroll-margin:%dpx;scroll-padding:%dpx %dpx}</style><p id=t>x</p>",
            (unsigned)(want & 0xffffffu),
            (3 % 4 == 0) ? "none" : (3 % 4 == 1) ? "text" : (3 % 4 == 2) ? "all" : "auto",
            (3 % 2) ? "none" : "auto",
            3 % 30, (3 * 3) % 20, (3 * 5) % 20);
    lot52_render(html, 400);
    TEST_ASSERT_EQUAL(1, lot52_node("t")->caret_set);
    TEST_ASSERT_EQUAL(want & 0xffffffu, lot52_node("t")->caret);
    TEST_ASSERT_EQUAL((3 % 4 == 0) ? 1 : (3 % 4 == 1) ? 2 : (3 % 4 == 2) ? 3 : 0,
                      lot52_node("t")->user_select);
    TEST_ASSERT_EQUAL((3 % 2) ? 1 : 0, lot52_node("t")->pointer_events);
    TEST_ASSERT_EQUAL(1, lot52_node("t")->scroll_m_set);
    TEST_ASSERT_EQUAL(3 % 30, lot52_node("t")->scroll_m[0]);
    TEST_ASSERT_EQUAL(1, lot52_node("t")->scroll_p_set);
}

static void test_l75_css_case_4(void) {
    char html[512];
    unsigned want = (unsigned)(0x100000u + 4 * 17u);
    sprintf(html, "<style>#t{caret-color:#%06x;user-select:%s;pointer-events:%s;"
                  "scroll-margin:%dpx;scroll-padding:%dpx %dpx}</style><p id=t>x</p>",
            (unsigned)(want & 0xffffffu),
            (4 % 4 == 0) ? "none" : (4 % 4 == 1) ? "text" : (4 % 4 == 2) ? "all" : "auto",
            (4 % 2) ? "none" : "auto",
            4 % 30, (4 * 3) % 20, (4 * 5) % 20);
    lot52_render(html, 400);
    TEST_ASSERT_EQUAL(1, lot52_node("t")->caret_set);
    TEST_ASSERT_EQUAL(want & 0xffffffu, lot52_node("t")->caret);
    TEST_ASSERT_EQUAL((4 % 4 == 0) ? 1 : (4 % 4 == 1) ? 2 : (4 % 4 == 2) ? 3 : 0,
                      lot52_node("t")->user_select);
    TEST_ASSERT_EQUAL((4 % 2) ? 1 : 0, lot52_node("t")->pointer_events);
    TEST_ASSERT_EQUAL(1, lot52_node("t")->scroll_m_set);
    TEST_ASSERT_EQUAL(4 % 30, lot52_node("t")->scroll_m[0]);
    TEST_ASSERT_EQUAL(1, lot52_node("t")->scroll_p_set);
}

static void test_l75_css_case_5(void) {
    char html[512];
    unsigned want = (unsigned)(0x100000u + 5 * 17u);
    sprintf(html, "<style>#t{caret-color:#%06x;user-select:%s;pointer-events:%s;"
                  "scroll-margin:%dpx;scroll-padding:%dpx %dpx}</style><p id=t>x</p>",
            (unsigned)(want & 0xffffffu),
            (5 % 4 == 0) ? "none" : (5 % 4 == 1) ? "text" : (5 % 4 == 2) ? "all" : "auto",
            (5 % 2) ? "none" : "auto",
            5 % 30, (5 * 3) % 20, (5 * 5) % 20);
    lot52_render(html, 400);
    TEST_ASSERT_EQUAL(1, lot52_node("t")->caret_set);
    TEST_ASSERT_EQUAL(want & 0xffffffu, lot52_node("t")->caret);
    TEST_ASSERT_EQUAL((5 % 4 == 0) ? 1 : (5 % 4 == 1) ? 2 : (5 % 4 == 2) ? 3 : 0,
                      lot52_node("t")->user_select);
    TEST_ASSERT_EQUAL((5 % 2) ? 1 : 0, lot52_node("t")->pointer_events);
    TEST_ASSERT_EQUAL(1, lot52_node("t")->scroll_m_set);
    TEST_ASSERT_EQUAL(5 % 30, lot52_node("t")->scroll_m[0]);
    TEST_ASSERT_EQUAL(1, lot52_node("t")->scroll_p_set);
}

static void test_l75_css_case_6(void) {
    char html[512];
    unsigned want = (unsigned)(0x100000u + 6 * 17u);
    sprintf(html, "<style>#t{caret-color:#%06x;user-select:%s;pointer-events:%s;"
                  "scroll-margin:%dpx;scroll-padding:%dpx %dpx}</style><p id=t>x</p>",
            (unsigned)(want & 0xffffffu),
            (6 % 4 == 0) ? "none" : (6 % 4 == 1) ? "text" : (6 % 4 == 2) ? "all" : "auto",
            (6 % 2) ? "none" : "auto",
            6 % 30, (6 * 3) % 20, (6 * 5) % 20);
    lot52_render(html, 400);
    TEST_ASSERT_EQUAL(1, lot52_node("t")->caret_set);
    TEST_ASSERT_EQUAL(want & 0xffffffu, lot52_node("t")->caret);
    TEST_ASSERT_EQUAL((6 % 4 == 0) ? 1 : (6 % 4 == 1) ? 2 : (6 % 4 == 2) ? 3 : 0,
                      lot52_node("t")->user_select);
    TEST_ASSERT_EQUAL((6 % 2) ? 1 : 0, lot52_node("t")->pointer_events);
    TEST_ASSERT_EQUAL(1, lot52_node("t")->scroll_m_set);
    TEST_ASSERT_EQUAL(6 % 30, lot52_node("t")->scroll_m[0]);
    TEST_ASSERT_EQUAL(1, lot52_node("t")->scroll_p_set);
}

static void test_l75_css_case_7(void) {
    char html[512];
    unsigned want = (unsigned)(0x100000u + 7 * 17u);
    sprintf(html, "<style>#t{caret-color:#%06x;user-select:%s;pointer-events:%s;"
                  "scroll-margin:%dpx;scroll-padding:%dpx %dpx}</style><p id=t>x</p>",
            (unsigned)(want & 0xffffffu),
            (7 % 4 == 0) ? "none" : (7 % 4 == 1) ? "text" : (7 % 4 == 2) ? "all" : "auto",
            (7 % 2) ? "none" : "auto",
            7 % 30, (7 * 3) % 20, (7 * 5) % 20);
    lot52_render(html, 400);
    TEST_ASSERT_EQUAL(1, lot52_node("t")->caret_set);
    TEST_ASSERT_EQUAL(want & 0xffffffu, lot52_node("t")->caret);
    TEST_ASSERT_EQUAL((7 % 4 == 0) ? 1 : (7 % 4 == 1) ? 2 : (7 % 4 == 2) ? 3 : 0,
                      lot52_node("t")->user_select);
    TEST_ASSERT_EQUAL((7 % 2) ? 1 : 0, lot52_node("t")->pointer_events);
    TEST_ASSERT_EQUAL(1, lot52_node("t")->scroll_m_set);
    TEST_ASSERT_EQUAL(7 % 30, lot52_node("t")->scroll_m[0]);
    TEST_ASSERT_EQUAL(1, lot52_node("t")->scroll_p_set);
}

static void test_l75_css_case_8(void) {
    char html[512];
    unsigned want = (unsigned)(0x100000u + 8 * 17u);
    sprintf(html, "<style>#t{caret-color:#%06x;user-select:%s;pointer-events:%s;"
                  "scroll-margin:%dpx;scroll-padding:%dpx %dpx}</style><p id=t>x</p>",
            (unsigned)(want & 0xffffffu),
            (8 % 4 == 0) ? "none" : (8 % 4 == 1) ? "text" : (8 % 4 == 2) ? "all" : "auto",
            (8 % 2) ? "none" : "auto",
            8 % 30, (8 * 3) % 20, (8 * 5) % 20);
    lot52_render(html, 400);
    TEST_ASSERT_EQUAL(1, lot52_node("t")->caret_set);
    TEST_ASSERT_EQUAL(want & 0xffffffu, lot52_node("t")->caret);
    TEST_ASSERT_EQUAL((8 % 4 == 0) ? 1 : (8 % 4 == 1) ? 2 : (8 % 4 == 2) ? 3 : 0,
                      lot52_node("t")->user_select);
    TEST_ASSERT_EQUAL((8 % 2) ? 1 : 0, lot52_node("t")->pointer_events);
    TEST_ASSERT_EQUAL(1, lot52_node("t")->scroll_m_set);
    TEST_ASSERT_EQUAL(8 % 30, lot52_node("t")->scroll_m[0]);
    TEST_ASSERT_EQUAL(1, lot52_node("t")->scroll_p_set);
}

static void test_l75_css_case_9(void) {
    char html[512];
    unsigned want = (unsigned)(0x100000u + 9 * 17u);
    sprintf(html, "<style>#t{caret-color:#%06x;user-select:%s;pointer-events:%s;"
                  "scroll-margin:%dpx;scroll-padding:%dpx %dpx}</style><p id=t>x</p>",
            (unsigned)(want & 0xffffffu),
            (9 % 4 == 0) ? "none" : (9 % 4 == 1) ? "text" : (9 % 4 == 2) ? "all" : "auto",
            (9 % 2) ? "none" : "auto",
            9 % 30, (9 * 3) % 20, (9 * 5) % 20);
    lot52_render(html, 400);
    TEST_ASSERT_EQUAL(1, lot52_node("t")->caret_set);
    TEST_ASSERT_EQUAL(want & 0xffffffu, lot52_node("t")->caret);
    TEST_ASSERT_EQUAL((9 % 4 == 0) ? 1 : (9 % 4 == 1) ? 2 : (9 % 4 == 2) ? 3 : 0,
                      lot52_node("t")->user_select);
    TEST_ASSERT_EQUAL((9 % 2) ? 1 : 0, lot52_node("t")->pointer_events);
    TEST_ASSERT_EQUAL(1, lot52_node("t")->scroll_m_set);
    TEST_ASSERT_EQUAL(9 % 30, lot52_node("t")->scroll_m[0]);
    TEST_ASSERT_EQUAL(1, lot52_node("t")->scroll_p_set);
}

static void test_l75_css_case_10(void) {
    char html[512];
    unsigned want = (unsigned)(0x100000u + 10 * 17u);
    sprintf(html, "<style>#t{caret-color:#%06x;user-select:%s;pointer-events:%s;"
                  "scroll-margin:%dpx;scroll-padding:%dpx %dpx}</style><p id=t>x</p>",
            (unsigned)(want & 0xffffffu),
            (10 % 4 == 0) ? "none" : (10 % 4 == 1) ? "text" : (10 % 4 == 2) ? "all" : "auto",
            (10 % 2) ? "none" : "auto",
            10 % 30, (10 * 3) % 20, (10 * 5) % 20);
    lot52_render(html, 400);
    TEST_ASSERT_EQUAL(1, lot52_node("t")->caret_set);
    TEST_ASSERT_EQUAL(want & 0xffffffu, lot52_node("t")->caret);
    TEST_ASSERT_EQUAL((10 % 4 == 0) ? 1 : (10 % 4 == 1) ? 2 : (10 % 4 == 2) ? 3 : 0,
                      lot52_node("t")->user_select);
    TEST_ASSERT_EQUAL((10 % 2) ? 1 : 0, lot52_node("t")->pointer_events);
    TEST_ASSERT_EQUAL(1, lot52_node("t")->scroll_m_set);
    TEST_ASSERT_EQUAL(10 % 30, lot52_node("t")->scroll_m[0]);
    TEST_ASSERT_EQUAL(1, lot52_node("t")->scroll_p_set);
}

static void test_l75_css_case_11(void) {
    char html[512];
    unsigned want = (unsigned)(0x100000u + 11 * 17u);
    sprintf(html, "<style>#t{caret-color:#%06x;user-select:%s;pointer-events:%s;"
                  "scroll-margin:%dpx;scroll-padding:%dpx %dpx}</style><p id=t>x</p>",
            (unsigned)(want & 0xffffffu),
            (11 % 4 == 0) ? "none" : (11 % 4 == 1) ? "text" : (11 % 4 == 2) ? "all" : "auto",
            (11 % 2) ? "none" : "auto",
            11 % 30, (11 * 3) % 20, (11 * 5) % 20);
    lot52_render(html, 400);
    TEST_ASSERT_EQUAL(1, lot52_node("t")->caret_set);
    TEST_ASSERT_EQUAL(want & 0xffffffu, lot52_node("t")->caret);
    TEST_ASSERT_EQUAL((11 % 4 == 0) ? 1 : (11 % 4 == 1) ? 2 : (11 % 4 == 2) ? 3 : 0,
                      lot52_node("t")->user_select);
    TEST_ASSERT_EQUAL((11 % 2) ? 1 : 0, lot52_node("t")->pointer_events);
    TEST_ASSERT_EQUAL(1, lot52_node("t")->scroll_m_set);
    TEST_ASSERT_EQUAL(11 % 30, lot52_node("t")->scroll_m[0]);
    TEST_ASSERT_EQUAL(1, lot52_node("t")->scroll_p_set);
}

static void test_l75_css_case_12(void) {
    char html[512];
    unsigned want = (unsigned)(0x100000u + 12 * 17u);
    sprintf(html, "<style>#t{caret-color:#%06x;user-select:%s;pointer-events:%s;"
                  "scroll-margin:%dpx;scroll-padding:%dpx %dpx}</style><p id=t>x</p>",
            (unsigned)(want & 0xffffffu),
            (12 % 4 == 0) ? "none" : (12 % 4 == 1) ? "text" : (12 % 4 == 2) ? "all" : "auto",
            (12 % 2) ? "none" : "auto",
            12 % 30, (12 * 3) % 20, (12 * 5) % 20);
    lot52_render(html, 400);
    TEST_ASSERT_EQUAL(1, lot52_node("t")->caret_set);
    TEST_ASSERT_EQUAL(want & 0xffffffu, lot52_node("t")->caret);
    TEST_ASSERT_EQUAL((12 % 4 == 0) ? 1 : (12 % 4 == 1) ? 2 : (12 % 4 == 2) ? 3 : 0,
                      lot52_node("t")->user_select);
    TEST_ASSERT_EQUAL((12 % 2) ? 1 : 0, lot52_node("t")->pointer_events);
    TEST_ASSERT_EQUAL(1, lot52_node("t")->scroll_m_set);
    TEST_ASSERT_EQUAL(12 % 30, lot52_node("t")->scroll_m[0]);
    TEST_ASSERT_EQUAL(1, lot52_node("t")->scroll_p_set);
}

static void test_l75_css_case_13(void) {
    char html[512];
    unsigned want = (unsigned)(0x100000u + 13 * 17u);
    sprintf(html, "<style>#t{caret-color:#%06x;user-select:%s;pointer-events:%s;"
                  "scroll-margin:%dpx;scroll-padding:%dpx %dpx}</style><p id=t>x</p>",
            (unsigned)(want & 0xffffffu),
            (13 % 4 == 0) ? "none" : (13 % 4 == 1) ? "text" : (13 % 4 == 2) ? "all" : "auto",
            (13 % 2) ? "none" : "auto",
            13 % 30, (13 * 3) % 20, (13 * 5) % 20);
    lot52_render(html, 400);
    TEST_ASSERT_EQUAL(1, lot52_node("t")->caret_set);
    TEST_ASSERT_EQUAL(want & 0xffffffu, lot52_node("t")->caret);
    TEST_ASSERT_EQUAL((13 % 4 == 0) ? 1 : (13 % 4 == 1) ? 2 : (13 % 4 == 2) ? 3 : 0,
                      lot52_node("t")->user_select);
    TEST_ASSERT_EQUAL((13 % 2) ? 1 : 0, lot52_node("t")->pointer_events);
    TEST_ASSERT_EQUAL(1, lot52_node("t")->scroll_m_set);
    TEST_ASSERT_EQUAL(13 % 30, lot52_node("t")->scroll_m[0]);
    TEST_ASSERT_EQUAL(1, lot52_node("t")->scroll_p_set);
}

static void test_l75_css_case_14(void) {
    char html[512];
    unsigned want = (unsigned)(0x100000u + 14 * 17u);
    sprintf(html, "<style>#t{caret-color:#%06x;user-select:%s;pointer-events:%s;"
                  "scroll-margin:%dpx;scroll-padding:%dpx %dpx}</style><p id=t>x</p>",
            (unsigned)(want & 0xffffffu),
            (14 % 4 == 0) ? "none" : (14 % 4 == 1) ? "text" : (14 % 4 == 2) ? "all" : "auto",
            (14 % 2) ? "none" : "auto",
            14 % 30, (14 * 3) % 20, (14 * 5) % 20);
    lot52_render(html, 400);
    TEST_ASSERT_EQUAL(1, lot52_node("t")->caret_set);
    TEST_ASSERT_EQUAL(want & 0xffffffu, lot52_node("t")->caret);
    TEST_ASSERT_EQUAL((14 % 4 == 0) ? 1 : (14 % 4 == 1) ? 2 : (14 % 4 == 2) ? 3 : 0,
                      lot52_node("t")->user_select);
    TEST_ASSERT_EQUAL((14 % 2) ? 1 : 0, lot52_node("t")->pointer_events);
    TEST_ASSERT_EQUAL(1, lot52_node("t")->scroll_m_set);
    TEST_ASSERT_EQUAL(14 % 30, lot52_node("t")->scroll_m[0]);
    TEST_ASSERT_EQUAL(1, lot52_node("t")->scroll_p_set);
}

static void test_l75_css_case_15(void) {
    char html[512];
    unsigned want = (unsigned)(0x100000u + 15 * 17u);
    sprintf(html, "<style>#t{caret-color:#%06x;user-select:%s;pointer-events:%s;"
                  "scroll-margin:%dpx;scroll-padding:%dpx %dpx}</style><p id=t>x</p>",
            (unsigned)(want & 0xffffffu),
            (15 % 4 == 0) ? "none" : (15 % 4 == 1) ? "text" : (15 % 4 == 2) ? "all" : "auto",
            (15 % 2) ? "none" : "auto",
            15 % 30, (15 * 3) % 20, (15 * 5) % 20);
    lot52_render(html, 400);
    TEST_ASSERT_EQUAL(1, lot52_node("t")->caret_set);
    TEST_ASSERT_EQUAL(want & 0xffffffu, lot52_node("t")->caret);
    TEST_ASSERT_EQUAL((15 % 4 == 0) ? 1 : (15 % 4 == 1) ? 2 : (15 % 4 == 2) ? 3 : 0,
                      lot52_node("t")->user_select);
    TEST_ASSERT_EQUAL((15 % 2) ? 1 : 0, lot52_node("t")->pointer_events);
    TEST_ASSERT_EQUAL(1, lot52_node("t")->scroll_m_set);
    TEST_ASSERT_EQUAL(15 % 30, lot52_node("t")->scroll_m[0]);
    TEST_ASSERT_EQUAL(1, lot52_node("t")->scroll_p_set);
}

static void test_l75_css_case_16(void) {
    char html[512];
    unsigned want = (unsigned)(0x100000u + 16 * 17u);
    sprintf(html, "<style>#t{caret-color:#%06x;user-select:%s;pointer-events:%s;"
                  "scroll-margin:%dpx;scroll-padding:%dpx %dpx}</style><p id=t>x</p>",
            (unsigned)(want & 0xffffffu),
            (16 % 4 == 0) ? "none" : (16 % 4 == 1) ? "text" : (16 % 4 == 2) ? "all" : "auto",
            (16 % 2) ? "none" : "auto",
            16 % 30, (16 * 3) % 20, (16 * 5) % 20);
    lot52_render(html, 400);
    TEST_ASSERT_EQUAL(1, lot52_node("t")->caret_set);
    TEST_ASSERT_EQUAL(want & 0xffffffu, lot52_node("t")->caret);
    TEST_ASSERT_EQUAL((16 % 4 == 0) ? 1 : (16 % 4 == 1) ? 2 : (16 % 4 == 2) ? 3 : 0,
                      lot52_node("t")->user_select);
    TEST_ASSERT_EQUAL((16 % 2) ? 1 : 0, lot52_node("t")->pointer_events);
    TEST_ASSERT_EQUAL(1, lot52_node("t")->scroll_m_set);
    TEST_ASSERT_EQUAL(16 % 30, lot52_node("t")->scroll_m[0]);
    TEST_ASSERT_EQUAL(1, lot52_node("t")->scroll_p_set);
}

static void test_l75_css_case_17(void) {
    char html[512];
    unsigned want = (unsigned)(0x100000u + 17 * 17u);
    sprintf(html, "<style>#t{caret-color:#%06x;user-select:%s;pointer-events:%s;"
                  "scroll-margin:%dpx;scroll-padding:%dpx %dpx}</style><p id=t>x</p>",
            (unsigned)(want & 0xffffffu),
            (17 % 4 == 0) ? "none" : (17 % 4 == 1) ? "text" : (17 % 4 == 2) ? "all" : "auto",
            (17 % 2) ? "none" : "auto",
            17 % 30, (17 * 3) % 20, (17 * 5) % 20);
    lot52_render(html, 400);
    TEST_ASSERT_EQUAL(1, lot52_node("t")->caret_set);
    TEST_ASSERT_EQUAL(want & 0xffffffu, lot52_node("t")->caret);
    TEST_ASSERT_EQUAL((17 % 4 == 0) ? 1 : (17 % 4 == 1) ? 2 : (17 % 4 == 2) ? 3 : 0,
                      lot52_node("t")->user_select);
    TEST_ASSERT_EQUAL((17 % 2) ? 1 : 0, lot52_node("t")->pointer_events);
    TEST_ASSERT_EQUAL(1, lot52_node("t")->scroll_m_set);
    TEST_ASSERT_EQUAL(17 % 30, lot52_node("t")->scroll_m[0]);
    TEST_ASSERT_EQUAL(1, lot52_node("t")->scroll_p_set);
}

static void test_l75_css_case_18(void) {
    char html[512];
    unsigned want = (unsigned)(0x100000u + 18 * 17u);
    sprintf(html, "<style>#t{caret-color:#%06x;user-select:%s;pointer-events:%s;"
                  "scroll-margin:%dpx;scroll-padding:%dpx %dpx}</style><p id=t>x</p>",
            (unsigned)(want & 0xffffffu),
            (18 % 4 == 0) ? "none" : (18 % 4 == 1) ? "text" : (18 % 4 == 2) ? "all" : "auto",
            (18 % 2) ? "none" : "auto",
            18 % 30, (18 * 3) % 20, (18 * 5) % 20);
    lot52_render(html, 400);
    TEST_ASSERT_EQUAL(1, lot52_node("t")->caret_set);
    TEST_ASSERT_EQUAL(want & 0xffffffu, lot52_node("t")->caret);
    TEST_ASSERT_EQUAL((18 % 4 == 0) ? 1 : (18 % 4 == 1) ? 2 : (18 % 4 == 2) ? 3 : 0,
                      lot52_node("t")->user_select);
    TEST_ASSERT_EQUAL((18 % 2) ? 1 : 0, lot52_node("t")->pointer_events);
    TEST_ASSERT_EQUAL(1, lot52_node("t")->scroll_m_set);
    TEST_ASSERT_EQUAL(18 % 30, lot52_node("t")->scroll_m[0]);
    TEST_ASSERT_EQUAL(1, lot52_node("t")->scroll_p_set);
}

static void test_l75_css_case_19(void) {
    char html[512];
    unsigned want = (unsigned)(0x100000u + 19 * 17u);
    sprintf(html, "<style>#t{caret-color:#%06x;user-select:%s;pointer-events:%s;"
                  "scroll-margin:%dpx;scroll-padding:%dpx %dpx}</style><p id=t>x</p>",
            (unsigned)(want & 0xffffffu),
            (19 % 4 == 0) ? "none" : (19 % 4 == 1) ? "text" : (19 % 4 == 2) ? "all" : "auto",
            (19 % 2) ? "none" : "auto",
            19 % 30, (19 * 3) % 20, (19 * 5) % 20);
    lot52_render(html, 400);
    TEST_ASSERT_EQUAL(1, lot52_node("t")->caret_set);
    TEST_ASSERT_EQUAL(want & 0xffffffu, lot52_node("t")->caret);
    TEST_ASSERT_EQUAL((19 % 4 == 0) ? 1 : (19 % 4 == 1) ? 2 : (19 % 4 == 2) ? 3 : 0,
                      lot52_node("t")->user_select);
    TEST_ASSERT_EQUAL((19 % 2) ? 1 : 0, lot52_node("t")->pointer_events);
    TEST_ASSERT_EQUAL(1, lot52_node("t")->scroll_m_set);
    TEST_ASSERT_EQUAL(19 % 30, lot52_node("t")->scroll_m[0]);
    TEST_ASSERT_EQUAL(1, lot52_node("t")->scroll_p_set);
}

static void test_l75_css_case_20(void) {
    char html[512];
    unsigned want = (unsigned)(0x100000u + 20 * 17u);
    sprintf(html, "<style>#t{caret-color:#%06x;user-select:%s;pointer-events:%s;"
                  "scroll-margin:%dpx;scroll-padding:%dpx %dpx}</style><p id=t>x</p>",
            (unsigned)(want & 0xffffffu),
            (20 % 4 == 0) ? "none" : (20 % 4 == 1) ? "text" : (20 % 4 == 2) ? "all" : "auto",
            (20 % 2) ? "none" : "auto",
            20 % 30, (20 * 3) % 20, (20 * 5) % 20);
    lot52_render(html, 400);
    TEST_ASSERT_EQUAL(1, lot52_node("t")->caret_set);
    TEST_ASSERT_EQUAL(want & 0xffffffu, lot52_node("t")->caret);
    TEST_ASSERT_EQUAL((20 % 4 == 0) ? 1 : (20 % 4 == 1) ? 2 : (20 % 4 == 2) ? 3 : 0,
                      lot52_node("t")->user_select);
    TEST_ASSERT_EQUAL((20 % 2) ? 1 : 0, lot52_node("t")->pointer_events);
    TEST_ASSERT_EQUAL(1, lot52_node("t")->scroll_m_set);
    TEST_ASSERT_EQUAL(20 % 30, lot52_node("t")->scroll_m[0]);
    TEST_ASSERT_EQUAL(1, lot52_node("t")->scroll_p_set);
}

static void test_l75_css_case_21(void) {
    char html[512];
    unsigned want = (unsigned)(0x100000u + 21 * 17u);
    sprintf(html, "<style>#t{caret-color:#%06x;user-select:%s;pointer-events:%s;"
                  "scroll-margin:%dpx;scroll-padding:%dpx %dpx}</style><p id=t>x</p>",
            (unsigned)(want & 0xffffffu),
            (21 % 4 == 0) ? "none" : (21 % 4 == 1) ? "text" : (21 % 4 == 2) ? "all" : "auto",
            (21 % 2) ? "none" : "auto",
            21 % 30, (21 * 3) % 20, (21 * 5) % 20);
    lot52_render(html, 400);
    TEST_ASSERT_EQUAL(1, lot52_node("t")->caret_set);
    TEST_ASSERT_EQUAL(want & 0xffffffu, lot52_node("t")->caret);
    TEST_ASSERT_EQUAL((21 % 4 == 0) ? 1 : (21 % 4 == 1) ? 2 : (21 % 4 == 2) ? 3 : 0,
                      lot52_node("t")->user_select);
    TEST_ASSERT_EQUAL((21 % 2) ? 1 : 0, lot52_node("t")->pointer_events);
    TEST_ASSERT_EQUAL(1, lot52_node("t")->scroll_m_set);
    TEST_ASSERT_EQUAL(21 % 30, lot52_node("t")->scroll_m[0]);
    TEST_ASSERT_EQUAL(1, lot52_node("t")->scroll_p_set);
}

static void test_l75_css_case_22(void) {
    char html[512];
    unsigned want = (unsigned)(0x100000u + 22 * 17u);
    sprintf(html, "<style>#t{caret-color:#%06x;user-select:%s;pointer-events:%s;"
                  "scroll-margin:%dpx;scroll-padding:%dpx %dpx}</style><p id=t>x</p>",
            (unsigned)(want & 0xffffffu),
            (22 % 4 == 0) ? "none" : (22 % 4 == 1) ? "text" : (22 % 4 == 2) ? "all" : "auto",
            (22 % 2) ? "none" : "auto",
            22 % 30, (22 * 3) % 20, (22 * 5) % 20);
    lot52_render(html, 400);
    TEST_ASSERT_EQUAL(1, lot52_node("t")->caret_set);
    TEST_ASSERT_EQUAL(want & 0xffffffu, lot52_node("t")->caret);
    TEST_ASSERT_EQUAL((22 % 4 == 0) ? 1 : (22 % 4 == 1) ? 2 : (22 % 4 == 2) ? 3 : 0,
                      lot52_node("t")->user_select);
    TEST_ASSERT_EQUAL((22 % 2) ? 1 : 0, lot52_node("t")->pointer_events);
    TEST_ASSERT_EQUAL(1, lot52_node("t")->scroll_m_set);
    TEST_ASSERT_EQUAL(22 % 30, lot52_node("t")->scroll_m[0]);
    TEST_ASSERT_EQUAL(1, lot52_node("t")->scroll_p_set);
}

static void test_l75_css_case_23(void) {
    char html[512];
    unsigned want = (unsigned)(0x100000u + 23 * 17u);
    sprintf(html, "<style>#t{caret-color:#%06x;user-select:%s;pointer-events:%s;"
                  "scroll-margin:%dpx;scroll-padding:%dpx %dpx}</style><p id=t>x</p>",
            (unsigned)(want & 0xffffffu),
            (23 % 4 == 0) ? "none" : (23 % 4 == 1) ? "text" : (23 % 4 == 2) ? "all" : "auto",
            (23 % 2) ? "none" : "auto",
            23 % 30, (23 * 3) % 20, (23 * 5) % 20);
    lot52_render(html, 400);
    TEST_ASSERT_EQUAL(1, lot52_node("t")->caret_set);
    TEST_ASSERT_EQUAL(want & 0xffffffu, lot52_node("t")->caret);
    TEST_ASSERT_EQUAL((23 % 4 == 0) ? 1 : (23 % 4 == 1) ? 2 : (23 % 4 == 2) ? 3 : 0,
                      lot52_node("t")->user_select);
    TEST_ASSERT_EQUAL((23 % 2) ? 1 : 0, lot52_node("t")->pointer_events);
    TEST_ASSERT_EQUAL(1, lot52_node("t")->scroll_m_set);
    TEST_ASSERT_EQUAL(23 % 30, lot52_node("t")->scroll_m[0]);
    TEST_ASSERT_EQUAL(1, lot52_node("t")->scroll_p_set);
}

static void test_l75_css_case_24(void) {
    char html[512];
    unsigned want = (unsigned)(0x100000u + 24 * 17u);
    sprintf(html, "<style>#t{caret-color:#%06x;user-select:%s;pointer-events:%s;"
                  "scroll-margin:%dpx;scroll-padding:%dpx %dpx}</style><p id=t>x</p>",
            (unsigned)(want & 0xffffffu),
            (24 % 4 == 0) ? "none" : (24 % 4 == 1) ? "text" : (24 % 4 == 2) ? "all" : "auto",
            (24 % 2) ? "none" : "auto",
            24 % 30, (24 * 3) % 20, (24 * 5) % 20);
    lot52_render(html, 400);
    TEST_ASSERT_EQUAL(1, lot52_node("t")->caret_set);
    TEST_ASSERT_EQUAL(want & 0xffffffu, lot52_node("t")->caret);
    TEST_ASSERT_EQUAL((24 % 4 == 0) ? 1 : (24 % 4 == 1) ? 2 : (24 % 4 == 2) ? 3 : 0,
                      lot52_node("t")->user_select);
    TEST_ASSERT_EQUAL((24 % 2) ? 1 : 0, lot52_node("t")->pointer_events);
    TEST_ASSERT_EQUAL(1, lot52_node("t")->scroll_m_set);
    TEST_ASSERT_EQUAL(24 % 30, lot52_node("t")->scroll_m[0]);
    TEST_ASSERT_EQUAL(1, lot52_node("t")->scroll_p_set);
}

static void test_l75_css_case_25(void) {
    char html[512];
    unsigned want = (unsigned)(0x100000u + 25 * 17u);
    sprintf(html, "<style>#t{caret-color:#%06x;user-select:%s;pointer-events:%s;"
                  "scroll-margin:%dpx;scroll-padding:%dpx %dpx}</style><p id=t>x</p>",
            (unsigned)(want & 0xffffffu),
            (25 % 4 == 0) ? "none" : (25 % 4 == 1) ? "text" : (25 % 4 == 2) ? "all" : "auto",
            (25 % 2) ? "none" : "auto",
            25 % 30, (25 * 3) % 20, (25 * 5) % 20);
    lot52_render(html, 400);
    TEST_ASSERT_EQUAL(1, lot52_node("t")->caret_set);
    TEST_ASSERT_EQUAL(want & 0xffffffu, lot52_node("t")->caret);
    TEST_ASSERT_EQUAL((25 % 4 == 0) ? 1 : (25 % 4 == 1) ? 2 : (25 % 4 == 2) ? 3 : 0,
                      lot52_node("t")->user_select);
    TEST_ASSERT_EQUAL((25 % 2) ? 1 : 0, lot52_node("t")->pointer_events);
    TEST_ASSERT_EQUAL(1, lot52_node("t")->scroll_m_set);
    TEST_ASSERT_EQUAL(25 % 30, lot52_node("t")->scroll_m[0]);
    TEST_ASSERT_EQUAL(1, lot52_node("t")->scroll_p_set);
}

static void test_l75_css_case_26(void) {
    char html[512];
    unsigned want = (unsigned)(0x100000u + 26 * 17u);
    sprintf(html, "<style>#t{caret-color:#%06x;user-select:%s;pointer-events:%s;"
                  "scroll-margin:%dpx;scroll-padding:%dpx %dpx}</style><p id=t>x</p>",
            (unsigned)(want & 0xffffffu),
            (26 % 4 == 0) ? "none" : (26 % 4 == 1) ? "text" : (26 % 4 == 2) ? "all" : "auto",
            (26 % 2) ? "none" : "auto",
            26 % 30, (26 * 3) % 20, (26 * 5) % 20);
    lot52_render(html, 400);
    TEST_ASSERT_EQUAL(1, lot52_node("t")->caret_set);
    TEST_ASSERT_EQUAL(want & 0xffffffu, lot52_node("t")->caret);
    TEST_ASSERT_EQUAL((26 % 4 == 0) ? 1 : (26 % 4 == 1) ? 2 : (26 % 4 == 2) ? 3 : 0,
                      lot52_node("t")->user_select);
    TEST_ASSERT_EQUAL((26 % 2) ? 1 : 0, lot52_node("t")->pointer_events);
    TEST_ASSERT_EQUAL(1, lot52_node("t")->scroll_m_set);
    TEST_ASSERT_EQUAL(26 % 30, lot52_node("t")->scroll_m[0]);
    TEST_ASSERT_EQUAL(1, lot52_node("t")->scroll_p_set);
}

static void test_l75_css_case_27(void) {
    char html[512];
    unsigned want = (unsigned)(0x100000u + 27 * 17u);
    sprintf(html, "<style>#t{caret-color:#%06x;user-select:%s;pointer-events:%s;"
                  "scroll-margin:%dpx;scroll-padding:%dpx %dpx}</style><p id=t>x</p>",
            (unsigned)(want & 0xffffffu),
            (27 % 4 == 0) ? "none" : (27 % 4 == 1) ? "text" : (27 % 4 == 2) ? "all" : "auto",
            (27 % 2) ? "none" : "auto",
            27 % 30, (27 * 3) % 20, (27 * 5) % 20);
    lot52_render(html, 400);
    TEST_ASSERT_EQUAL(1, lot52_node("t")->caret_set);
    TEST_ASSERT_EQUAL(want & 0xffffffu, lot52_node("t")->caret);
    TEST_ASSERT_EQUAL((27 % 4 == 0) ? 1 : (27 % 4 == 1) ? 2 : (27 % 4 == 2) ? 3 : 0,
                      lot52_node("t")->user_select);
    TEST_ASSERT_EQUAL((27 % 2) ? 1 : 0, lot52_node("t")->pointer_events);
    TEST_ASSERT_EQUAL(1, lot52_node("t")->scroll_m_set);
    TEST_ASSERT_EQUAL(27 % 30, lot52_node("t")->scroll_m[0]);
    TEST_ASSERT_EQUAL(1, lot52_node("t")->scroll_p_set);
}

static void test_l75_css_case_28(void) {
    char html[512];
    unsigned want = (unsigned)(0x100000u + 28 * 17u);
    sprintf(html, "<style>#t{caret-color:#%06x;user-select:%s;pointer-events:%s;"
                  "scroll-margin:%dpx;scroll-padding:%dpx %dpx}</style><p id=t>x</p>",
            (unsigned)(want & 0xffffffu),
            (28 % 4 == 0) ? "none" : (28 % 4 == 1) ? "text" : (28 % 4 == 2) ? "all" : "auto",
            (28 % 2) ? "none" : "auto",
            28 % 30, (28 * 3) % 20, (28 * 5) % 20);
    lot52_render(html, 400);
    TEST_ASSERT_EQUAL(1, lot52_node("t")->caret_set);
    TEST_ASSERT_EQUAL(want & 0xffffffu, lot52_node("t")->caret);
    TEST_ASSERT_EQUAL((28 % 4 == 0) ? 1 : (28 % 4 == 1) ? 2 : (28 % 4 == 2) ? 3 : 0,
                      lot52_node("t")->user_select);
    TEST_ASSERT_EQUAL((28 % 2) ? 1 : 0, lot52_node("t")->pointer_events);
    TEST_ASSERT_EQUAL(1, lot52_node("t")->scroll_m_set);
    TEST_ASSERT_EQUAL(28 % 30, lot52_node("t")->scroll_m[0]);
    TEST_ASSERT_EQUAL(1, lot52_node("t")->scroll_p_set);
}

static void test_l75_css_case_29(void) {
    char html[512];
    unsigned want = (unsigned)(0x100000u + 29 * 17u);
    sprintf(html, "<style>#t{caret-color:#%06x;user-select:%s;pointer-events:%s;"
                  "scroll-margin:%dpx;scroll-padding:%dpx %dpx}</style><p id=t>x</p>",
            (unsigned)(want & 0xffffffu),
            (29 % 4 == 0) ? "none" : (29 % 4 == 1) ? "text" : (29 % 4 == 2) ? "all" : "auto",
            (29 % 2) ? "none" : "auto",
            29 % 30, (29 * 3) % 20, (29 * 5) % 20);
    lot52_render(html, 400);
    TEST_ASSERT_EQUAL(1, lot52_node("t")->caret_set);
    TEST_ASSERT_EQUAL(want & 0xffffffu, lot52_node("t")->caret);
    TEST_ASSERT_EQUAL((29 % 4 == 0) ? 1 : (29 % 4 == 1) ? 2 : (29 % 4 == 2) ? 3 : 0,
                      lot52_node("t")->user_select);
    TEST_ASSERT_EQUAL((29 % 2) ? 1 : 0, lot52_node("t")->pointer_events);
    TEST_ASSERT_EQUAL(1, lot52_node("t")->scroll_m_set);
    TEST_ASSERT_EQUAL(29 % 30, lot52_node("t")->scroll_m[0]);
    TEST_ASSERT_EQUAL(1, lot52_node("t")->scroll_p_set);
}

static void test_l75_css_case_30(void) {
    char html[512];
    unsigned want = (unsigned)(0x100000u + 30 * 17u);
    sprintf(html, "<style>#t{caret-color:#%06x;user-select:%s;pointer-events:%s;"
                  "scroll-margin:%dpx;scroll-padding:%dpx %dpx}</style><p id=t>x</p>",
            (unsigned)(want & 0xffffffu),
            (30 % 4 == 0) ? "none" : (30 % 4 == 1) ? "text" : (30 % 4 == 2) ? "all" : "auto",
            (30 % 2) ? "none" : "auto",
            30 % 30, (30 * 3) % 20, (30 * 5) % 20);
    lot52_render(html, 400);
    TEST_ASSERT_EQUAL(1, lot52_node("t")->caret_set);
    TEST_ASSERT_EQUAL(want & 0xffffffu, lot52_node("t")->caret);
    TEST_ASSERT_EQUAL((30 % 4 == 0) ? 1 : (30 % 4 == 1) ? 2 : (30 % 4 == 2) ? 3 : 0,
                      lot52_node("t")->user_select);
    TEST_ASSERT_EQUAL((30 % 2) ? 1 : 0, lot52_node("t")->pointer_events);
    TEST_ASSERT_EQUAL(1, lot52_node("t")->scroll_m_set);
    TEST_ASSERT_EQUAL(30 % 30, lot52_node("t")->scroll_m[0]);
    TEST_ASSERT_EQUAL(1, lot52_node("t")->scroll_p_set);
}

static void test_l75_css_case_31(void) {
    char html[512];
    unsigned want = (unsigned)(0x100000u + 31 * 17u);
    sprintf(html, "<style>#t{caret-color:#%06x;user-select:%s;pointer-events:%s;"
                  "scroll-margin:%dpx;scroll-padding:%dpx %dpx}</style><p id=t>x</p>",
            (unsigned)(want & 0xffffffu),
            (31 % 4 == 0) ? "none" : (31 % 4 == 1) ? "text" : (31 % 4 == 2) ? "all" : "auto",
            (31 % 2) ? "none" : "auto",
            31 % 30, (31 * 3) % 20, (31 * 5) % 20);
    lot52_render(html, 400);
    TEST_ASSERT_EQUAL(1, lot52_node("t")->caret_set);
    TEST_ASSERT_EQUAL(want & 0xffffffu, lot52_node("t")->caret);
    TEST_ASSERT_EQUAL((31 % 4 == 0) ? 1 : (31 % 4 == 1) ? 2 : (31 % 4 == 2) ? 3 : 0,
                      lot52_node("t")->user_select);
    TEST_ASSERT_EQUAL((31 % 2) ? 1 : 0, lot52_node("t")->pointer_events);
    TEST_ASSERT_EQUAL(1, lot52_node("t")->scroll_m_set);
    TEST_ASSERT_EQUAL(31 % 30, lot52_node("t")->scroll_m[0]);
    TEST_ASSERT_EQUAL(1, lot52_node("t")->scroll_p_set);
}

static void test_l75_css_case_32(void) {
    char html[512];
    unsigned want = (unsigned)(0x100000u + 32 * 17u);
    sprintf(html, "<style>#t{caret-color:#%06x;user-select:%s;pointer-events:%s;"
                  "scroll-margin:%dpx;scroll-padding:%dpx %dpx}</style><p id=t>x</p>",
            (unsigned)(want & 0xffffffu),
            (32 % 4 == 0) ? "none" : (32 % 4 == 1) ? "text" : (32 % 4 == 2) ? "all" : "auto",
            (32 % 2) ? "none" : "auto",
            32 % 30, (32 * 3) % 20, (32 * 5) % 20);
    lot52_render(html, 400);
    TEST_ASSERT_EQUAL(1, lot52_node("t")->caret_set);
    TEST_ASSERT_EQUAL(want & 0xffffffu, lot52_node("t")->caret);
    TEST_ASSERT_EQUAL((32 % 4 == 0) ? 1 : (32 % 4 == 1) ? 2 : (32 % 4 == 2) ? 3 : 0,
                      lot52_node("t")->user_select);
    TEST_ASSERT_EQUAL((32 % 2) ? 1 : 0, lot52_node("t")->pointer_events);
    TEST_ASSERT_EQUAL(1, lot52_node("t")->scroll_m_set);
    TEST_ASSERT_EQUAL(32 % 30, lot52_node("t")->scroll_m[0]);
    TEST_ASSERT_EQUAL(1, lot52_node("t")->scroll_p_set);
}

static void test_l75_css_case_33(void) {
    char html[512];
    unsigned want = (unsigned)(0x100000u + 33 * 17u);
    sprintf(html, "<style>#t{caret-color:#%06x;user-select:%s;pointer-events:%s;"
                  "scroll-margin:%dpx;scroll-padding:%dpx %dpx}</style><p id=t>x</p>",
            (unsigned)(want & 0xffffffu),
            (33 % 4 == 0) ? "none" : (33 % 4 == 1) ? "text" : (33 % 4 == 2) ? "all" : "auto",
            (33 % 2) ? "none" : "auto",
            33 % 30, (33 * 3) % 20, (33 * 5) % 20);
    lot52_render(html, 400);
    TEST_ASSERT_EQUAL(1, lot52_node("t")->caret_set);
    TEST_ASSERT_EQUAL(want & 0xffffffu, lot52_node("t")->caret);
    TEST_ASSERT_EQUAL((33 % 4 == 0) ? 1 : (33 % 4 == 1) ? 2 : (33 % 4 == 2) ? 3 : 0,
                      lot52_node("t")->user_select);
    TEST_ASSERT_EQUAL((33 % 2) ? 1 : 0, lot52_node("t")->pointer_events);
    TEST_ASSERT_EQUAL(1, lot52_node("t")->scroll_m_set);
    TEST_ASSERT_EQUAL(33 % 30, lot52_node("t")->scroll_m[0]);
    TEST_ASSERT_EQUAL(1, lot52_node("t")->scroll_p_set);
}

static void test_l75_css_case_34(void) {
    char html[512];
    unsigned want = (unsigned)(0x100000u + 34 * 17u);
    sprintf(html, "<style>#t{caret-color:#%06x;user-select:%s;pointer-events:%s;"
                  "scroll-margin:%dpx;scroll-padding:%dpx %dpx}</style><p id=t>x</p>",
            (unsigned)(want & 0xffffffu),
            (34 % 4 == 0) ? "none" : (34 % 4 == 1) ? "text" : (34 % 4 == 2) ? "all" : "auto",
            (34 % 2) ? "none" : "auto",
            34 % 30, (34 * 3) % 20, (34 * 5) % 20);
    lot52_render(html, 400);
    TEST_ASSERT_EQUAL(1, lot52_node("t")->caret_set);
    TEST_ASSERT_EQUAL(want & 0xffffffu, lot52_node("t")->caret);
    TEST_ASSERT_EQUAL((34 % 4 == 0) ? 1 : (34 % 4 == 1) ? 2 : (34 % 4 == 2) ? 3 : 0,
                      lot52_node("t")->user_select);
    TEST_ASSERT_EQUAL((34 % 2) ? 1 : 0, lot52_node("t")->pointer_events);
    TEST_ASSERT_EQUAL(1, lot52_node("t")->scroll_m_set);
    TEST_ASSERT_EQUAL(34 % 30, lot52_node("t")->scroll_m[0]);
    TEST_ASSERT_EQUAL(1, lot52_node("t")->scroll_p_set);
}

static void test_l75_css_case_35(void) {
    char html[512];
    unsigned want = (unsigned)(0x100000u + 35 * 17u);
    sprintf(html, "<style>#t{caret-color:#%06x;user-select:%s;pointer-events:%s;"
                  "scroll-margin:%dpx;scroll-padding:%dpx %dpx}</style><p id=t>x</p>",
            (unsigned)(want & 0xffffffu),
            (35 % 4 == 0) ? "none" : (35 % 4 == 1) ? "text" : (35 % 4 == 2) ? "all" : "auto",
            (35 % 2) ? "none" : "auto",
            35 % 30, (35 * 3) % 20, (35 * 5) % 20);
    lot52_render(html, 400);
    TEST_ASSERT_EQUAL(1, lot52_node("t")->caret_set);
    TEST_ASSERT_EQUAL(want & 0xffffffu, lot52_node("t")->caret);
    TEST_ASSERT_EQUAL((35 % 4 == 0) ? 1 : (35 % 4 == 1) ? 2 : (35 % 4 == 2) ? 3 : 0,
                      lot52_node("t")->user_select);
    TEST_ASSERT_EQUAL((35 % 2) ? 1 : 0, lot52_node("t")->pointer_events);
    TEST_ASSERT_EQUAL(1, lot52_node("t")->scroll_m_set);
    TEST_ASSERT_EQUAL(35 % 30, lot52_node("t")->scroll_m[0]);
    TEST_ASSERT_EQUAL(1, lot52_node("t")->scroll_p_set);
}

static void test_l75_css_case_36(void) {
    char html[512];
    unsigned want = (unsigned)(0x100000u + 36 * 17u);
    sprintf(html, "<style>#t{caret-color:#%06x;user-select:%s;pointer-events:%s;"
                  "scroll-margin:%dpx;scroll-padding:%dpx %dpx}</style><p id=t>x</p>",
            (unsigned)(want & 0xffffffu),
            (36 % 4 == 0) ? "none" : (36 % 4 == 1) ? "text" : (36 % 4 == 2) ? "all" : "auto",
            (36 % 2) ? "none" : "auto",
            36 % 30, (36 * 3) % 20, (36 * 5) % 20);
    lot52_render(html, 400);
    TEST_ASSERT_EQUAL(1, lot52_node("t")->caret_set);
    TEST_ASSERT_EQUAL(want & 0xffffffu, lot52_node("t")->caret);
    TEST_ASSERT_EQUAL((36 % 4 == 0) ? 1 : (36 % 4 == 1) ? 2 : (36 % 4 == 2) ? 3 : 0,
                      lot52_node("t")->user_select);
    TEST_ASSERT_EQUAL((36 % 2) ? 1 : 0, lot52_node("t")->pointer_events);
    TEST_ASSERT_EQUAL(1, lot52_node("t")->scroll_m_set);
    TEST_ASSERT_EQUAL(36 % 30, lot52_node("t")->scroll_m[0]);
    TEST_ASSERT_EQUAL(1, lot52_node("t")->scroll_p_set);
}

static void test_l75_css_case_37(void) {
    char html[512];
    unsigned want = (unsigned)(0x100000u + 37 * 17u);
    sprintf(html, "<style>#t{caret-color:#%06x;user-select:%s;pointer-events:%s;"
                  "scroll-margin:%dpx;scroll-padding:%dpx %dpx}</style><p id=t>x</p>",
            (unsigned)(want & 0xffffffu),
            (37 % 4 == 0) ? "none" : (37 % 4 == 1) ? "text" : (37 % 4 == 2) ? "all" : "auto",
            (37 % 2) ? "none" : "auto",
            37 % 30, (37 * 3) % 20, (37 * 5) % 20);
    lot52_render(html, 400);
    TEST_ASSERT_EQUAL(1, lot52_node("t")->caret_set);
    TEST_ASSERT_EQUAL(want & 0xffffffu, lot52_node("t")->caret);
    TEST_ASSERT_EQUAL((37 % 4 == 0) ? 1 : (37 % 4 == 1) ? 2 : (37 % 4 == 2) ? 3 : 0,
                      lot52_node("t")->user_select);
    TEST_ASSERT_EQUAL((37 % 2) ? 1 : 0, lot52_node("t")->pointer_events);
    TEST_ASSERT_EQUAL(1, lot52_node("t")->scroll_m_set);
    TEST_ASSERT_EQUAL(37 % 30, lot52_node("t")->scroll_m[0]);
    TEST_ASSERT_EQUAL(1, lot52_node("t")->scroll_p_set);
}

static void test_l75_css_case_38(void) {
    char html[512];
    unsigned want = (unsigned)(0x100000u + 38 * 17u);
    sprintf(html, "<style>#t{caret-color:#%06x;user-select:%s;pointer-events:%s;"
                  "scroll-margin:%dpx;scroll-padding:%dpx %dpx}</style><p id=t>x</p>",
            (unsigned)(want & 0xffffffu),
            (38 % 4 == 0) ? "none" : (38 % 4 == 1) ? "text" : (38 % 4 == 2) ? "all" : "auto",
            (38 % 2) ? "none" : "auto",
            38 % 30, (38 * 3) % 20, (38 * 5) % 20);
    lot52_render(html, 400);
    TEST_ASSERT_EQUAL(1, lot52_node("t")->caret_set);
    TEST_ASSERT_EQUAL(want & 0xffffffu, lot52_node("t")->caret);
    TEST_ASSERT_EQUAL((38 % 4 == 0) ? 1 : (38 % 4 == 1) ? 2 : (38 % 4 == 2) ? 3 : 0,
                      lot52_node("t")->user_select);
    TEST_ASSERT_EQUAL((38 % 2) ? 1 : 0, lot52_node("t")->pointer_events);
    TEST_ASSERT_EQUAL(1, lot52_node("t")->scroll_m_set);
    TEST_ASSERT_EQUAL(38 % 30, lot52_node("t")->scroll_m[0]);
    TEST_ASSERT_EQUAL(1, lot52_node("t")->scroll_p_set);
}

static void test_l75_css_case_39(void) {
    char html[512];
    unsigned want = (unsigned)(0x100000u + 39 * 17u);
    sprintf(html, "<style>#t{caret-color:#%06x;user-select:%s;pointer-events:%s;"
                  "scroll-margin:%dpx;scroll-padding:%dpx %dpx}</style><p id=t>x</p>",
            (unsigned)(want & 0xffffffu),
            (39 % 4 == 0) ? "none" : (39 % 4 == 1) ? "text" : (39 % 4 == 2) ? "all" : "auto",
            (39 % 2) ? "none" : "auto",
            39 % 30, (39 * 3) % 20, (39 * 5) % 20);
    lot52_render(html, 400);
    TEST_ASSERT_EQUAL(1, lot52_node("t")->caret_set);
    TEST_ASSERT_EQUAL(want & 0xffffffu, lot52_node("t")->caret);
    TEST_ASSERT_EQUAL((39 % 4 == 0) ? 1 : (39 % 4 == 1) ? 2 : (39 % 4 == 2) ? 3 : 0,
                      lot52_node("t")->user_select);
    TEST_ASSERT_EQUAL((39 % 2) ? 1 : 0, lot52_node("t")->pointer_events);
    TEST_ASSERT_EQUAL(1, lot52_node("t")->scroll_m_set);
    TEST_ASSERT_EQUAL(39 % 30, lot52_node("t")->scroll_m[0]);
    TEST_ASSERT_EQUAL(1, lot52_node("t")->scroll_p_set);
}

static void test_l75_css_case_40(void) {
    char html[512];
    unsigned want = (unsigned)(0x100000u + 40 * 17u);
    sprintf(html, "<style>#t{caret-color:#%06x;user-select:%s;pointer-events:%s;"
                  "scroll-margin:%dpx;scroll-padding:%dpx %dpx}</style><p id=t>x</p>",
            (unsigned)(want & 0xffffffu),
            (40 % 4 == 0) ? "none" : (40 % 4 == 1) ? "text" : (40 % 4 == 2) ? "all" : "auto",
            (40 % 2) ? "none" : "auto",
            40 % 30, (40 * 3) % 20, (40 * 5) % 20);
    lot52_render(html, 400);
    TEST_ASSERT_EQUAL(1, lot52_node("t")->caret_set);
    TEST_ASSERT_EQUAL(want & 0xffffffu, lot52_node("t")->caret);
    TEST_ASSERT_EQUAL((40 % 4 == 0) ? 1 : (40 % 4 == 1) ? 2 : (40 % 4 == 2) ? 3 : 0,
                      lot52_node("t")->user_select);
    TEST_ASSERT_EQUAL((40 % 2) ? 1 : 0, lot52_node("t")->pointer_events);
    TEST_ASSERT_EQUAL(1, lot52_node("t")->scroll_m_set);
    TEST_ASSERT_EQUAL(40 % 30, lot52_node("t")->scroll_m[0]);
    TEST_ASSERT_EQUAL(1, lot52_node("t")->scroll_p_set);
}

static void test_l75_css_case_41(void) {
    char html[512];
    unsigned want = (unsigned)(0x100000u + 41 * 17u);
    sprintf(html, "<style>#t{caret-color:#%06x;user-select:%s;pointer-events:%s;"
                  "scroll-margin:%dpx;scroll-padding:%dpx %dpx}</style><p id=t>x</p>",
            (unsigned)(want & 0xffffffu),
            (41 % 4 == 0) ? "none" : (41 % 4 == 1) ? "text" : (41 % 4 == 2) ? "all" : "auto",
            (41 % 2) ? "none" : "auto",
            41 % 30, (41 * 3) % 20, (41 * 5) % 20);
    lot52_render(html, 400);
    TEST_ASSERT_EQUAL(1, lot52_node("t")->caret_set);
    TEST_ASSERT_EQUAL(want & 0xffffffu, lot52_node("t")->caret);
    TEST_ASSERT_EQUAL((41 % 4 == 0) ? 1 : (41 % 4 == 1) ? 2 : (41 % 4 == 2) ? 3 : 0,
                      lot52_node("t")->user_select);
    TEST_ASSERT_EQUAL((41 % 2) ? 1 : 0, lot52_node("t")->pointer_events);
    TEST_ASSERT_EQUAL(1, lot52_node("t")->scroll_m_set);
    TEST_ASSERT_EQUAL(41 % 30, lot52_node("t")->scroll_m[0]);
    TEST_ASSERT_EQUAL(1, lot52_node("t")->scroll_p_set);
}

static void test_l75_css_case_42(void) {
    char html[512];
    unsigned want = (unsigned)(0x100000u + 42 * 17u);
    sprintf(html, "<style>#t{caret-color:#%06x;user-select:%s;pointer-events:%s;"
                  "scroll-margin:%dpx;scroll-padding:%dpx %dpx}</style><p id=t>x</p>",
            (unsigned)(want & 0xffffffu),
            (42 % 4 == 0) ? "none" : (42 % 4 == 1) ? "text" : (42 % 4 == 2) ? "all" : "auto",
            (42 % 2) ? "none" : "auto",
            42 % 30, (42 * 3) % 20, (42 * 5) % 20);
    lot52_render(html, 400);
    TEST_ASSERT_EQUAL(1, lot52_node("t")->caret_set);
    TEST_ASSERT_EQUAL(want & 0xffffffu, lot52_node("t")->caret);
    TEST_ASSERT_EQUAL((42 % 4 == 0) ? 1 : (42 % 4 == 1) ? 2 : (42 % 4 == 2) ? 3 : 0,
                      lot52_node("t")->user_select);
    TEST_ASSERT_EQUAL((42 % 2) ? 1 : 0, lot52_node("t")->pointer_events);
    TEST_ASSERT_EQUAL(1, lot52_node("t")->scroll_m_set);
    TEST_ASSERT_EQUAL(42 % 30, lot52_node("t")->scroll_m[0]);
    TEST_ASSERT_EQUAL(1, lot52_node("t")->scroll_p_set);
}

static void test_l75_css_case_43(void) {
    char html[512];
    unsigned want = (unsigned)(0x100000u + 43 * 17u);
    sprintf(html, "<style>#t{caret-color:#%06x;user-select:%s;pointer-events:%s;"
                  "scroll-margin:%dpx;scroll-padding:%dpx %dpx}</style><p id=t>x</p>",
            (unsigned)(want & 0xffffffu),
            (43 % 4 == 0) ? "none" : (43 % 4 == 1) ? "text" : (43 % 4 == 2) ? "all" : "auto",
            (43 % 2) ? "none" : "auto",
            43 % 30, (43 * 3) % 20, (43 * 5) % 20);
    lot52_render(html, 400);
    TEST_ASSERT_EQUAL(1, lot52_node("t")->caret_set);
    TEST_ASSERT_EQUAL(want & 0xffffffu, lot52_node("t")->caret);
    TEST_ASSERT_EQUAL((43 % 4 == 0) ? 1 : (43 % 4 == 1) ? 2 : (43 % 4 == 2) ? 3 : 0,
                      lot52_node("t")->user_select);
    TEST_ASSERT_EQUAL((43 % 2) ? 1 : 0, lot52_node("t")->pointer_events);
    TEST_ASSERT_EQUAL(1, lot52_node("t")->scroll_m_set);
    TEST_ASSERT_EQUAL(43 % 30, lot52_node("t")->scroll_m[0]);
    TEST_ASSERT_EQUAL(1, lot52_node("t")->scroll_p_set);
}

static void test_l75_css_case_44(void) {
    char html[512];
    unsigned want = (unsigned)(0x100000u + 44 * 17u);
    sprintf(html, "<style>#t{caret-color:#%06x;user-select:%s;pointer-events:%s;"
                  "scroll-margin:%dpx;scroll-padding:%dpx %dpx}</style><p id=t>x</p>",
            (unsigned)(want & 0xffffffu),
            (44 % 4 == 0) ? "none" : (44 % 4 == 1) ? "text" : (44 % 4 == 2) ? "all" : "auto",
            (44 % 2) ? "none" : "auto",
            44 % 30, (44 * 3) % 20, (44 * 5) % 20);
    lot52_render(html, 400);
    TEST_ASSERT_EQUAL(1, lot52_node("t")->caret_set);
    TEST_ASSERT_EQUAL(want & 0xffffffu, lot52_node("t")->caret);
    TEST_ASSERT_EQUAL((44 % 4 == 0) ? 1 : (44 % 4 == 1) ? 2 : (44 % 4 == 2) ? 3 : 0,
                      lot52_node("t")->user_select);
    TEST_ASSERT_EQUAL((44 % 2) ? 1 : 0, lot52_node("t")->pointer_events);
    TEST_ASSERT_EQUAL(1, lot52_node("t")->scroll_m_set);
    TEST_ASSERT_EQUAL(44 % 30, lot52_node("t")->scroll_m[0]);
    TEST_ASSERT_EQUAL(1, lot52_node("t")->scroll_p_set);
}

static void test_l75_css_case_45(void) {
    char html[512];
    unsigned want = (unsigned)(0x100000u + 45 * 17u);
    sprintf(html, "<style>#t{caret-color:#%06x;user-select:%s;pointer-events:%s;"
                  "scroll-margin:%dpx;scroll-padding:%dpx %dpx}</style><p id=t>x</p>",
            (unsigned)(want & 0xffffffu),
            (45 % 4 == 0) ? "none" : (45 % 4 == 1) ? "text" : (45 % 4 == 2) ? "all" : "auto",
            (45 % 2) ? "none" : "auto",
            45 % 30, (45 * 3) % 20, (45 * 5) % 20);
    lot52_render(html, 400);
    TEST_ASSERT_EQUAL(1, lot52_node("t")->caret_set);
    TEST_ASSERT_EQUAL(want & 0xffffffu, lot52_node("t")->caret);
    TEST_ASSERT_EQUAL((45 % 4 == 0) ? 1 : (45 % 4 == 1) ? 2 : (45 % 4 == 2) ? 3 : 0,
                      lot52_node("t")->user_select);
    TEST_ASSERT_EQUAL((45 % 2) ? 1 : 0, lot52_node("t")->pointer_events);
    TEST_ASSERT_EQUAL(1, lot52_node("t")->scroll_m_set);
    TEST_ASSERT_EQUAL(45 % 30, lot52_node("t")->scroll_m[0]);
    TEST_ASSERT_EQUAL(1, lot52_node("t")->scroll_p_set);
}

static void test_l75_css_case_46(void) {
    char html[512];
    unsigned want = (unsigned)(0x100000u + 46 * 17u);
    sprintf(html, "<style>#t{caret-color:#%06x;user-select:%s;pointer-events:%s;"
                  "scroll-margin:%dpx;scroll-padding:%dpx %dpx}</style><p id=t>x</p>",
            (unsigned)(want & 0xffffffu),
            (46 % 4 == 0) ? "none" : (46 % 4 == 1) ? "text" : (46 % 4 == 2) ? "all" : "auto",
            (46 % 2) ? "none" : "auto",
            46 % 30, (46 * 3) % 20, (46 * 5) % 20);
    lot52_render(html, 400);
    TEST_ASSERT_EQUAL(1, lot52_node("t")->caret_set);
    TEST_ASSERT_EQUAL(want & 0xffffffu, lot52_node("t")->caret);
    TEST_ASSERT_EQUAL((46 % 4 == 0) ? 1 : (46 % 4 == 1) ? 2 : (46 % 4 == 2) ? 3 : 0,
                      lot52_node("t")->user_select);
    TEST_ASSERT_EQUAL((46 % 2) ? 1 : 0, lot52_node("t")->pointer_events);
    TEST_ASSERT_EQUAL(1, lot52_node("t")->scroll_m_set);
    TEST_ASSERT_EQUAL(46 % 30, lot52_node("t")->scroll_m[0]);
    TEST_ASSERT_EQUAL(1, lot52_node("t")->scroll_p_set);
}

static void test_l75_css_case_47(void) {
    char html[512];
    unsigned want = (unsigned)(0x100000u + 47 * 17u);
    sprintf(html, "<style>#t{caret-color:#%06x;user-select:%s;pointer-events:%s;"
                  "scroll-margin:%dpx;scroll-padding:%dpx %dpx}</style><p id=t>x</p>",
            (unsigned)(want & 0xffffffu),
            (47 % 4 == 0) ? "none" : (47 % 4 == 1) ? "text" : (47 % 4 == 2) ? "all" : "auto",
            (47 % 2) ? "none" : "auto",
            47 % 30, (47 * 3) % 20, (47 * 5) % 20);
    lot52_render(html, 400);
    TEST_ASSERT_EQUAL(1, lot52_node("t")->caret_set);
    TEST_ASSERT_EQUAL(want & 0xffffffu, lot52_node("t")->caret);
    TEST_ASSERT_EQUAL((47 % 4 == 0) ? 1 : (47 % 4 == 1) ? 2 : (47 % 4 == 2) ? 3 : 0,
                      lot52_node("t")->user_select);
    TEST_ASSERT_EQUAL((47 % 2) ? 1 : 0, lot52_node("t")->pointer_events);
    TEST_ASSERT_EQUAL(1, lot52_node("t")->scroll_m_set);
    TEST_ASSERT_EQUAL(47 % 30, lot52_node("t")->scroll_m[0]);
    TEST_ASSERT_EQUAL(1, lot52_node("t")->scroll_p_set);
}

static void test_l75_css_case_48(void) {
    char html[512];
    unsigned want = (unsigned)(0x100000u + 48 * 17u);
    sprintf(html, "<style>#t{caret-color:#%06x;user-select:%s;pointer-events:%s;"
                  "scroll-margin:%dpx;scroll-padding:%dpx %dpx}</style><p id=t>x</p>",
            (unsigned)(want & 0xffffffu),
            (48 % 4 == 0) ? "none" : (48 % 4 == 1) ? "text" : (48 % 4 == 2) ? "all" : "auto",
            (48 % 2) ? "none" : "auto",
            48 % 30, (48 * 3) % 20, (48 * 5) % 20);
    lot52_render(html, 400);
    TEST_ASSERT_EQUAL(1, lot52_node("t")->caret_set);
    TEST_ASSERT_EQUAL(want & 0xffffffu, lot52_node("t")->caret);
    TEST_ASSERT_EQUAL((48 % 4 == 0) ? 1 : (48 % 4 == 1) ? 2 : (48 % 4 == 2) ? 3 : 0,
                      lot52_node("t")->user_select);
    TEST_ASSERT_EQUAL((48 % 2) ? 1 : 0, lot52_node("t")->pointer_events);
    TEST_ASSERT_EQUAL(1, lot52_node("t")->scroll_m_set);
    TEST_ASSERT_EQUAL(48 % 30, lot52_node("t")->scroll_m[0]);
    TEST_ASSERT_EQUAL(1, lot52_node("t")->scroll_p_set);
}

static void test_l75_css_case_49(void) {
    char html[512];
    unsigned want = (unsigned)(0x100000u + 49 * 17u);
    sprintf(html, "<style>#t{caret-color:#%06x;user-select:%s;pointer-events:%s;"
                  "scroll-margin:%dpx;scroll-padding:%dpx %dpx}</style><p id=t>x</p>",
            (unsigned)(want & 0xffffffu),
            (49 % 4 == 0) ? "none" : (49 % 4 == 1) ? "text" : (49 % 4 == 2) ? "all" : "auto",
            (49 % 2) ? "none" : "auto",
            49 % 30, (49 * 3) % 20, (49 * 5) % 20);
    lot52_render(html, 400);
    TEST_ASSERT_EQUAL(1, lot52_node("t")->caret_set);
    TEST_ASSERT_EQUAL(want & 0xffffffu, lot52_node("t")->caret);
    TEST_ASSERT_EQUAL((49 % 4 == 0) ? 1 : (49 % 4 == 1) ? 2 : (49 % 4 == 2) ? 3 : 0,
                      lot52_node("t")->user_select);
    TEST_ASSERT_EQUAL((49 % 2) ? 1 : 0, lot52_node("t")->pointer_events);
    TEST_ASSERT_EQUAL(1, lot52_node("t")->scroll_m_set);
    TEST_ASSERT_EQUAL(49 % 30, lot52_node("t")->scroll_m[0]);
    TEST_ASSERT_EQUAL(1, lot52_node("t")->scroll_p_set);
}

static void test_l75_css_case_50(void) {
    char html[512];
    unsigned want = (unsigned)(0x100000u + 50 * 17u);
    sprintf(html, "<style>#t{caret-color:#%06x;user-select:%s;pointer-events:%s;"
                  "scroll-margin:%dpx;scroll-padding:%dpx %dpx}</style><p id=t>x</p>",
            (unsigned)(want & 0xffffffu),
            (50 % 4 == 0) ? "none" : (50 % 4 == 1) ? "text" : (50 % 4 == 2) ? "all" : "auto",
            (50 % 2) ? "none" : "auto",
            50 % 30, (50 * 3) % 20, (50 * 5) % 20);
    lot52_render(html, 400);
    TEST_ASSERT_EQUAL(1, lot52_node("t")->caret_set);
    TEST_ASSERT_EQUAL(want & 0xffffffu, lot52_node("t")->caret);
    TEST_ASSERT_EQUAL((50 % 4 == 0) ? 1 : (50 % 4 == 1) ? 2 : (50 % 4 == 2) ? 3 : 0,
                      lot52_node("t")->user_select);
    TEST_ASSERT_EQUAL((50 % 2) ? 1 : 0, lot52_node("t")->pointer_events);
    TEST_ASSERT_EQUAL(1, lot52_node("t")->scroll_m_set);
    TEST_ASSERT_EQUAL(50 % 30, lot52_node("t")->scroll_m[0]);
    TEST_ASSERT_EQUAL(1, lot52_node("t")->scroll_p_set);
}

static void test_l75_css_case_51(void) {
    char html[512];
    unsigned want = (unsigned)(0x100000u + 51 * 17u);
    sprintf(html, "<style>#t{caret-color:#%06x;user-select:%s;pointer-events:%s;"
                  "scroll-margin:%dpx;scroll-padding:%dpx %dpx}</style><p id=t>x</p>",
            (unsigned)(want & 0xffffffu),
            (51 % 4 == 0) ? "none" : (51 % 4 == 1) ? "text" : (51 % 4 == 2) ? "all" : "auto",
            (51 % 2) ? "none" : "auto",
            51 % 30, (51 * 3) % 20, (51 * 5) % 20);
    lot52_render(html, 400);
    TEST_ASSERT_EQUAL(1, lot52_node("t")->caret_set);
    TEST_ASSERT_EQUAL(want & 0xffffffu, lot52_node("t")->caret);
    TEST_ASSERT_EQUAL((51 % 4 == 0) ? 1 : (51 % 4 == 1) ? 2 : (51 % 4 == 2) ? 3 : 0,
                      lot52_node("t")->user_select);
    TEST_ASSERT_EQUAL((51 % 2) ? 1 : 0, lot52_node("t")->pointer_events);
    TEST_ASSERT_EQUAL(1, lot52_node("t")->scroll_m_set);
    TEST_ASSERT_EQUAL(51 % 30, lot52_node("t")->scroll_m[0]);
    TEST_ASSERT_EQUAL(1, lot52_node("t")->scroll_p_set);
}

static void test_l75_css_case_52(void) {
    char html[512];
    unsigned want = (unsigned)(0x100000u + 52 * 17u);
    sprintf(html, "<style>#t{caret-color:#%06x;user-select:%s;pointer-events:%s;"
                  "scroll-margin:%dpx;scroll-padding:%dpx %dpx}</style><p id=t>x</p>",
            (unsigned)(want & 0xffffffu),
            (52 % 4 == 0) ? "none" : (52 % 4 == 1) ? "text" : (52 % 4 == 2) ? "all" : "auto",
            (52 % 2) ? "none" : "auto",
            52 % 30, (52 * 3) % 20, (52 * 5) % 20);
    lot52_render(html, 400);
    TEST_ASSERT_EQUAL(1, lot52_node("t")->caret_set);
    TEST_ASSERT_EQUAL(want & 0xffffffu, lot52_node("t")->caret);
    TEST_ASSERT_EQUAL((52 % 4 == 0) ? 1 : (52 % 4 == 1) ? 2 : (52 % 4 == 2) ? 3 : 0,
                      lot52_node("t")->user_select);
    TEST_ASSERT_EQUAL((52 % 2) ? 1 : 0, lot52_node("t")->pointer_events);
    TEST_ASSERT_EQUAL(1, lot52_node("t")->scroll_m_set);
    TEST_ASSERT_EQUAL(52 % 30, lot52_node("t")->scroll_m[0]);
    TEST_ASSERT_EQUAL(1, lot52_node("t")->scroll_p_set);
}

static void test_l75_css_case_53(void) {
    char html[512];
    unsigned want = (unsigned)(0x100000u + 53 * 17u);
    sprintf(html, "<style>#t{caret-color:#%06x;user-select:%s;pointer-events:%s;"
                  "scroll-margin:%dpx;scroll-padding:%dpx %dpx}</style><p id=t>x</p>",
            (unsigned)(want & 0xffffffu),
            (53 % 4 == 0) ? "none" : (53 % 4 == 1) ? "text" : (53 % 4 == 2) ? "all" : "auto",
            (53 % 2) ? "none" : "auto",
            53 % 30, (53 * 3) % 20, (53 * 5) % 20);
    lot52_render(html, 400);
    TEST_ASSERT_EQUAL(1, lot52_node("t")->caret_set);
    TEST_ASSERT_EQUAL(want & 0xffffffu, lot52_node("t")->caret);
    TEST_ASSERT_EQUAL((53 % 4 == 0) ? 1 : (53 % 4 == 1) ? 2 : (53 % 4 == 2) ? 3 : 0,
                      lot52_node("t")->user_select);
    TEST_ASSERT_EQUAL((53 % 2) ? 1 : 0, lot52_node("t")->pointer_events);
    TEST_ASSERT_EQUAL(1, lot52_node("t")->scroll_m_set);
    TEST_ASSERT_EQUAL(53 % 30, lot52_node("t")->scroll_m[0]);
    TEST_ASSERT_EQUAL(1, lot52_node("t")->scroll_p_set);
}

static void test_l75_css_case_54(void) {
    char html[512];
    unsigned want = (unsigned)(0x100000u + 54 * 17u);
    sprintf(html, "<style>#t{caret-color:#%06x;user-select:%s;pointer-events:%s;"
                  "scroll-margin:%dpx;scroll-padding:%dpx %dpx}</style><p id=t>x</p>",
            (unsigned)(want & 0xffffffu),
            (54 % 4 == 0) ? "none" : (54 % 4 == 1) ? "text" : (54 % 4 == 2) ? "all" : "auto",
            (54 % 2) ? "none" : "auto",
            54 % 30, (54 * 3) % 20, (54 * 5) % 20);
    lot52_render(html, 400);
    TEST_ASSERT_EQUAL(1, lot52_node("t")->caret_set);
    TEST_ASSERT_EQUAL(want & 0xffffffu, lot52_node("t")->caret);
    TEST_ASSERT_EQUAL((54 % 4 == 0) ? 1 : (54 % 4 == 1) ? 2 : (54 % 4 == 2) ? 3 : 0,
                      lot52_node("t")->user_select);
    TEST_ASSERT_EQUAL((54 % 2) ? 1 : 0, lot52_node("t")->pointer_events);
    TEST_ASSERT_EQUAL(1, lot52_node("t")->scroll_m_set);
    TEST_ASSERT_EQUAL(54 % 30, lot52_node("t")->scroll_m[0]);
    TEST_ASSERT_EQUAL(1, lot52_node("t")->scroll_p_set);
}

static void test_l75_css_case_55(void) {
    char html[512];
    unsigned want = (unsigned)(0x100000u + 55 * 17u);
    sprintf(html, "<style>#t{caret-color:#%06x;user-select:%s;pointer-events:%s;"
                  "scroll-margin:%dpx;scroll-padding:%dpx %dpx}</style><p id=t>x</p>",
            (unsigned)(want & 0xffffffu),
            (55 % 4 == 0) ? "none" : (55 % 4 == 1) ? "text" : (55 % 4 == 2) ? "all" : "auto",
            (55 % 2) ? "none" : "auto",
            55 % 30, (55 * 3) % 20, (55 * 5) % 20);
    lot52_render(html, 400);
    TEST_ASSERT_EQUAL(1, lot52_node("t")->caret_set);
    TEST_ASSERT_EQUAL(want & 0xffffffu, lot52_node("t")->caret);
    TEST_ASSERT_EQUAL((55 % 4 == 0) ? 1 : (55 % 4 == 1) ? 2 : (55 % 4 == 2) ? 3 : 0,
                      lot52_node("t")->user_select);
    TEST_ASSERT_EQUAL((55 % 2) ? 1 : 0, lot52_node("t")->pointer_events);
    TEST_ASSERT_EQUAL(1, lot52_node("t")->scroll_m_set);
    TEST_ASSERT_EQUAL(55 % 30, lot52_node("t")->scroll_m[0]);
    TEST_ASSERT_EQUAL(1, lot52_node("t")->scroll_p_set);
}

static void test_l75_css_case_56(void) {
    char html[512];
    unsigned want = (unsigned)(0x100000u + 56 * 17u);
    sprintf(html, "<style>#t{caret-color:#%06x;user-select:%s;pointer-events:%s;"
                  "scroll-margin:%dpx;scroll-padding:%dpx %dpx}</style><p id=t>x</p>",
            (unsigned)(want & 0xffffffu),
            (56 % 4 == 0) ? "none" : (56 % 4 == 1) ? "text" : (56 % 4 == 2) ? "all" : "auto",
            (56 % 2) ? "none" : "auto",
            56 % 30, (56 * 3) % 20, (56 * 5) % 20);
    lot52_render(html, 400);
    TEST_ASSERT_EQUAL(1, lot52_node("t")->caret_set);
    TEST_ASSERT_EQUAL(want & 0xffffffu, lot52_node("t")->caret);
    TEST_ASSERT_EQUAL((56 % 4 == 0) ? 1 : (56 % 4 == 1) ? 2 : (56 % 4 == 2) ? 3 : 0,
                      lot52_node("t")->user_select);
    TEST_ASSERT_EQUAL((56 % 2) ? 1 : 0, lot52_node("t")->pointer_events);
    TEST_ASSERT_EQUAL(1, lot52_node("t")->scroll_m_set);
    TEST_ASSERT_EQUAL(56 % 30, lot52_node("t")->scroll_m[0]);
    TEST_ASSERT_EQUAL(1, lot52_node("t")->scroll_p_set);
}

static void test_l75_css_case_57(void) {
    char html[512];
    unsigned want = (unsigned)(0x100000u + 57 * 17u);
    sprintf(html, "<style>#t{caret-color:#%06x;user-select:%s;pointer-events:%s;"
                  "scroll-margin:%dpx;scroll-padding:%dpx %dpx}</style><p id=t>x</p>",
            (unsigned)(want & 0xffffffu),
            (57 % 4 == 0) ? "none" : (57 % 4 == 1) ? "text" : (57 % 4 == 2) ? "all" : "auto",
            (57 % 2) ? "none" : "auto",
            57 % 30, (57 * 3) % 20, (57 * 5) % 20);
    lot52_render(html, 400);
    TEST_ASSERT_EQUAL(1, lot52_node("t")->caret_set);
    TEST_ASSERT_EQUAL(want & 0xffffffu, lot52_node("t")->caret);
    TEST_ASSERT_EQUAL((57 % 4 == 0) ? 1 : (57 % 4 == 1) ? 2 : (57 % 4 == 2) ? 3 : 0,
                      lot52_node("t")->user_select);
    TEST_ASSERT_EQUAL((57 % 2) ? 1 : 0, lot52_node("t")->pointer_events);
    TEST_ASSERT_EQUAL(1, lot52_node("t")->scroll_m_set);
    TEST_ASSERT_EQUAL(57 % 30, lot52_node("t")->scroll_m[0]);
    TEST_ASSERT_EQUAL(1, lot52_node("t")->scroll_p_set);
}

static void test_l75_css_case_58(void) {
    char html[512];
    unsigned want = (unsigned)(0x100000u + 58 * 17u);
    sprintf(html, "<style>#t{caret-color:#%06x;user-select:%s;pointer-events:%s;"
                  "scroll-margin:%dpx;scroll-padding:%dpx %dpx}</style><p id=t>x</p>",
            (unsigned)(want & 0xffffffu),
            (58 % 4 == 0) ? "none" : (58 % 4 == 1) ? "text" : (58 % 4 == 2) ? "all" : "auto",
            (58 % 2) ? "none" : "auto",
            58 % 30, (58 * 3) % 20, (58 * 5) % 20);
    lot52_render(html, 400);
    TEST_ASSERT_EQUAL(1, lot52_node("t")->caret_set);
    TEST_ASSERT_EQUAL(want & 0xffffffu, lot52_node("t")->caret);
    TEST_ASSERT_EQUAL((58 % 4 == 0) ? 1 : (58 % 4 == 1) ? 2 : (58 % 4 == 2) ? 3 : 0,
                      lot52_node("t")->user_select);
    TEST_ASSERT_EQUAL((58 % 2) ? 1 : 0, lot52_node("t")->pointer_events);
    TEST_ASSERT_EQUAL(1, lot52_node("t")->scroll_m_set);
    TEST_ASSERT_EQUAL(58 % 30, lot52_node("t")->scroll_m[0]);
    TEST_ASSERT_EQUAL(1, lot52_node("t")->scroll_p_set);
}

static void test_l75_css_case_59(void) {
    char html[512];
    unsigned want = (unsigned)(0x100000u + 59 * 17u);
    sprintf(html, "<style>#t{caret-color:#%06x;user-select:%s;pointer-events:%s;"
                  "scroll-margin:%dpx;scroll-padding:%dpx %dpx}</style><p id=t>x</p>",
            (unsigned)(want & 0xffffffu),
            (59 % 4 == 0) ? "none" : (59 % 4 == 1) ? "text" : (59 % 4 == 2) ? "all" : "auto",
            (59 % 2) ? "none" : "auto",
            59 % 30, (59 * 3) % 20, (59 * 5) % 20);
    lot52_render(html, 400);
    TEST_ASSERT_EQUAL(1, lot52_node("t")->caret_set);
    TEST_ASSERT_EQUAL(want & 0xffffffu, lot52_node("t")->caret);
    TEST_ASSERT_EQUAL((59 % 4 == 0) ? 1 : (59 % 4 == 1) ? 2 : (59 % 4 == 2) ? 3 : 0,
                      lot52_node("t")->user_select);
    TEST_ASSERT_EQUAL((59 % 2) ? 1 : 0, lot52_node("t")->pointer_events);
    TEST_ASSERT_EQUAL(1, lot52_node("t")->scroll_m_set);
    TEST_ASSERT_EQUAL(59 % 30, lot52_node("t")->scroll_m[0]);
    TEST_ASSERT_EQUAL(1, lot52_node("t")->scroll_p_set);
}

static void test_l75_css_case_60(void) {
    char html[512];
    unsigned want = (unsigned)(0x100000u + 60 * 17u);
    sprintf(html, "<style>#t{caret-color:#%06x;user-select:%s;pointer-events:%s;"
                  "scroll-margin:%dpx;scroll-padding:%dpx %dpx}</style><p id=t>x</p>",
            (unsigned)(want & 0xffffffu),
            (60 % 4 == 0) ? "none" : (60 % 4 == 1) ? "text" : (60 % 4 == 2) ? "all" : "auto",
            (60 % 2) ? "none" : "auto",
            60 % 30, (60 * 3) % 20, (60 * 5) % 20);
    lot52_render(html, 400);
    TEST_ASSERT_EQUAL(1, lot52_node("t")->caret_set);
    TEST_ASSERT_EQUAL(want & 0xffffffu, lot52_node("t")->caret);
    TEST_ASSERT_EQUAL((60 % 4 == 0) ? 1 : (60 % 4 == 1) ? 2 : (60 % 4 == 2) ? 3 : 0,
                      lot52_node("t")->user_select);
    TEST_ASSERT_EQUAL((60 % 2) ? 1 : 0, lot52_node("t")->pointer_events);
    TEST_ASSERT_EQUAL(1, lot52_node("t")->scroll_m_set);
    TEST_ASSERT_EQUAL(60 % 30, lot52_node("t")->scroll_m[0]);
    TEST_ASSERT_EQUAL(1, lot52_node("t")->scroll_p_set);
}

static void test_l75_css_case_61(void) {
    char html[512];
    unsigned want = (unsigned)(0x100000u + 61 * 17u);
    sprintf(html, "<style>#t{caret-color:#%06x;user-select:%s;pointer-events:%s;"
                  "scroll-margin:%dpx;scroll-padding:%dpx %dpx}</style><p id=t>x</p>",
            (unsigned)(want & 0xffffffu),
            (61 % 4 == 0) ? "none" : (61 % 4 == 1) ? "text" : (61 % 4 == 2) ? "all" : "auto",
            (61 % 2) ? "none" : "auto",
            61 % 30, (61 * 3) % 20, (61 * 5) % 20);
    lot52_render(html, 400);
    TEST_ASSERT_EQUAL(1, lot52_node("t")->caret_set);
    TEST_ASSERT_EQUAL(want & 0xffffffu, lot52_node("t")->caret);
    TEST_ASSERT_EQUAL((61 % 4 == 0) ? 1 : (61 % 4 == 1) ? 2 : (61 % 4 == 2) ? 3 : 0,
                      lot52_node("t")->user_select);
    TEST_ASSERT_EQUAL((61 % 2) ? 1 : 0, lot52_node("t")->pointer_events);
    TEST_ASSERT_EQUAL(1, lot52_node("t")->scroll_m_set);
    TEST_ASSERT_EQUAL(61 % 30, lot52_node("t")->scroll_m[0]);
    TEST_ASSERT_EQUAL(1, lot52_node("t")->scroll_p_set);
}

static void test_l75_css_case_62(void) {
    char html[512];
    unsigned want = (unsigned)(0x100000u + 62 * 17u);
    sprintf(html, "<style>#t{caret-color:#%06x;user-select:%s;pointer-events:%s;"
                  "scroll-margin:%dpx;scroll-padding:%dpx %dpx}</style><p id=t>x</p>",
            (unsigned)(want & 0xffffffu),
            (62 % 4 == 0) ? "none" : (62 % 4 == 1) ? "text" : (62 % 4 == 2) ? "all" : "auto",
            (62 % 2) ? "none" : "auto",
            62 % 30, (62 * 3) % 20, (62 * 5) % 20);
    lot52_render(html, 400);
    TEST_ASSERT_EQUAL(1, lot52_node("t")->caret_set);
    TEST_ASSERT_EQUAL(want & 0xffffffu, lot52_node("t")->caret);
    TEST_ASSERT_EQUAL((62 % 4 == 0) ? 1 : (62 % 4 == 1) ? 2 : (62 % 4 == 2) ? 3 : 0,
                      lot52_node("t")->user_select);
    TEST_ASSERT_EQUAL((62 % 2) ? 1 : 0, lot52_node("t")->pointer_events);
    TEST_ASSERT_EQUAL(1, lot52_node("t")->scroll_m_set);
    TEST_ASSERT_EQUAL(62 % 30, lot52_node("t")->scroll_m[0]);
    TEST_ASSERT_EQUAL(1, lot52_node("t")->scroll_p_set);
}

static void test_l75_css_case_63(void) {
    char html[512];
    unsigned want = (unsigned)(0x100000u + 63 * 17u);
    sprintf(html, "<style>#t{caret-color:#%06x;user-select:%s;pointer-events:%s;"
                  "scroll-margin:%dpx;scroll-padding:%dpx %dpx}</style><p id=t>x</p>",
            (unsigned)(want & 0xffffffu),
            (63 % 4 == 0) ? "none" : (63 % 4 == 1) ? "text" : (63 % 4 == 2) ? "all" : "auto",
            (63 % 2) ? "none" : "auto",
            63 % 30, (63 * 3) % 20, (63 * 5) % 20);
    lot52_render(html, 400);
    TEST_ASSERT_EQUAL(1, lot52_node("t")->caret_set);
    TEST_ASSERT_EQUAL(want & 0xffffffu, lot52_node("t")->caret);
    TEST_ASSERT_EQUAL((63 % 4 == 0) ? 1 : (63 % 4 == 1) ? 2 : (63 % 4 == 2) ? 3 : 0,
                      lot52_node("t")->user_select);
    TEST_ASSERT_EQUAL((63 % 2) ? 1 : 0, lot52_node("t")->pointer_events);
    TEST_ASSERT_EQUAL(1, lot52_node("t")->scroll_m_set);
    TEST_ASSERT_EQUAL(63 % 30, lot52_node("t")->scroll_m[0]);
    TEST_ASSERT_EQUAL(1, lot52_node("t")->scroll_p_set);
}

static void test_l75_css_case_64(void) {
    char html[512];
    unsigned want = (unsigned)(0x100000u + 64 * 17u);
    sprintf(html, "<style>#t{caret-color:#%06x;user-select:%s;pointer-events:%s;"
                  "scroll-margin:%dpx;scroll-padding:%dpx %dpx}</style><p id=t>x</p>",
            (unsigned)(want & 0xffffffu),
            (64 % 4 == 0) ? "none" : (64 % 4 == 1) ? "text" : (64 % 4 == 2) ? "all" : "auto",
            (64 % 2) ? "none" : "auto",
            64 % 30, (64 * 3) % 20, (64 * 5) % 20);
    lot52_render(html, 400);
    TEST_ASSERT_EQUAL(1, lot52_node("t")->caret_set);
    TEST_ASSERT_EQUAL(want & 0xffffffu, lot52_node("t")->caret);
    TEST_ASSERT_EQUAL((64 % 4 == 0) ? 1 : (64 % 4 == 1) ? 2 : (64 % 4 == 2) ? 3 : 0,
                      lot52_node("t")->user_select);
    TEST_ASSERT_EQUAL((64 % 2) ? 1 : 0, lot52_node("t")->pointer_events);
    TEST_ASSERT_EQUAL(1, lot52_node("t")->scroll_m_set);
    TEST_ASSERT_EQUAL(64 % 30, lot52_node("t")->scroll_m[0]);
    TEST_ASSERT_EQUAL(1, lot52_node("t")->scroll_p_set);
}

static void test_l75_css_case_65(void) {
    char html[512];
    unsigned want = (unsigned)(0x100000u + 65 * 17u);
    sprintf(html, "<style>#t{caret-color:#%06x;user-select:%s;pointer-events:%s;"
                  "scroll-margin:%dpx;scroll-padding:%dpx %dpx}</style><p id=t>x</p>",
            (unsigned)(want & 0xffffffu),
            (65 % 4 == 0) ? "none" : (65 % 4 == 1) ? "text" : (65 % 4 == 2) ? "all" : "auto",
            (65 % 2) ? "none" : "auto",
            65 % 30, (65 * 3) % 20, (65 * 5) % 20);
    lot52_render(html, 400);
    TEST_ASSERT_EQUAL(1, lot52_node("t")->caret_set);
    TEST_ASSERT_EQUAL(want & 0xffffffu, lot52_node("t")->caret);
    TEST_ASSERT_EQUAL((65 % 4 == 0) ? 1 : (65 % 4 == 1) ? 2 : (65 % 4 == 2) ? 3 : 0,
                      lot52_node("t")->user_select);
    TEST_ASSERT_EQUAL((65 % 2) ? 1 : 0, lot52_node("t")->pointer_events);
    TEST_ASSERT_EQUAL(1, lot52_node("t")->scroll_m_set);
    TEST_ASSERT_EQUAL(65 % 30, lot52_node("t")->scroll_m[0]);
    TEST_ASSERT_EQUAL(1, lot52_node("t")->scroll_p_set);
}

static void test_l75_css_case_66(void) {
    char html[512];
    unsigned want = (unsigned)(0x100000u + 66 * 17u);
    sprintf(html, "<style>#t{caret-color:#%06x;user-select:%s;pointer-events:%s;"
                  "scroll-margin:%dpx;scroll-padding:%dpx %dpx}</style><p id=t>x</p>",
            (unsigned)(want & 0xffffffu),
            (66 % 4 == 0) ? "none" : (66 % 4 == 1) ? "text" : (66 % 4 == 2) ? "all" : "auto",
            (66 % 2) ? "none" : "auto",
            66 % 30, (66 * 3) % 20, (66 * 5) % 20);
    lot52_render(html, 400);
    TEST_ASSERT_EQUAL(1, lot52_node("t")->caret_set);
    TEST_ASSERT_EQUAL(want & 0xffffffu, lot52_node("t")->caret);
    TEST_ASSERT_EQUAL((66 % 4 == 0) ? 1 : (66 % 4 == 1) ? 2 : (66 % 4 == 2) ? 3 : 0,
                      lot52_node("t")->user_select);
    TEST_ASSERT_EQUAL((66 % 2) ? 1 : 0, lot52_node("t")->pointer_events);
    TEST_ASSERT_EQUAL(1, lot52_node("t")->scroll_m_set);
    TEST_ASSERT_EQUAL(66 % 30, lot52_node("t")->scroll_m[0]);
    TEST_ASSERT_EQUAL(1, lot52_node("t")->scroll_p_set);
}

static void test_l75_css_case_67(void) {
    char html[512];
    unsigned want = (unsigned)(0x100000u + 67 * 17u);
    sprintf(html, "<style>#t{caret-color:#%06x;user-select:%s;pointer-events:%s;"
                  "scroll-margin:%dpx;scroll-padding:%dpx %dpx}</style><p id=t>x</p>",
            (unsigned)(want & 0xffffffu),
            (67 % 4 == 0) ? "none" : (67 % 4 == 1) ? "text" : (67 % 4 == 2) ? "all" : "auto",
            (67 % 2) ? "none" : "auto",
            67 % 30, (67 * 3) % 20, (67 * 5) % 20);
    lot52_render(html, 400);
    TEST_ASSERT_EQUAL(1, lot52_node("t")->caret_set);
    TEST_ASSERT_EQUAL(want & 0xffffffu, lot52_node("t")->caret);
    TEST_ASSERT_EQUAL((67 % 4 == 0) ? 1 : (67 % 4 == 1) ? 2 : (67 % 4 == 2) ? 3 : 0,
                      lot52_node("t")->user_select);
    TEST_ASSERT_EQUAL((67 % 2) ? 1 : 0, lot52_node("t")->pointer_events);
    TEST_ASSERT_EQUAL(1, lot52_node("t")->scroll_m_set);
    TEST_ASSERT_EQUAL(67 % 30, lot52_node("t")->scroll_m[0]);
    TEST_ASSERT_EQUAL(1, lot52_node("t")->scroll_p_set);
}

static void test_l75_css_case_68(void) {
    char html[512];
    unsigned want = (unsigned)(0x100000u + 68 * 17u);
    sprintf(html, "<style>#t{caret-color:#%06x;user-select:%s;pointer-events:%s;"
                  "scroll-margin:%dpx;scroll-padding:%dpx %dpx}</style><p id=t>x</p>",
            (unsigned)(want & 0xffffffu),
            (68 % 4 == 0) ? "none" : (68 % 4 == 1) ? "text" : (68 % 4 == 2) ? "all" : "auto",
            (68 % 2) ? "none" : "auto",
            68 % 30, (68 * 3) % 20, (68 * 5) % 20);
    lot52_render(html, 400);
    TEST_ASSERT_EQUAL(1, lot52_node("t")->caret_set);
    TEST_ASSERT_EQUAL(want & 0xffffffu, lot52_node("t")->caret);
    TEST_ASSERT_EQUAL((68 % 4 == 0) ? 1 : (68 % 4 == 1) ? 2 : (68 % 4 == 2) ? 3 : 0,
                      lot52_node("t")->user_select);
    TEST_ASSERT_EQUAL((68 % 2) ? 1 : 0, lot52_node("t")->pointer_events);
    TEST_ASSERT_EQUAL(1, lot52_node("t")->scroll_m_set);
    TEST_ASSERT_EQUAL(68 % 30, lot52_node("t")->scroll_m[0]);
    TEST_ASSERT_EQUAL(1, lot52_node("t")->scroll_p_set);
}

static void test_l75_css_case_69(void) {
    char html[512];
    unsigned want = (unsigned)(0x100000u + 69 * 17u);
    sprintf(html, "<style>#t{caret-color:#%06x;user-select:%s;pointer-events:%s;"
                  "scroll-margin:%dpx;scroll-padding:%dpx %dpx}</style><p id=t>x</p>",
            (unsigned)(want & 0xffffffu),
            (69 % 4 == 0) ? "none" : (69 % 4 == 1) ? "text" : (69 % 4 == 2) ? "all" : "auto",
            (69 % 2) ? "none" : "auto",
            69 % 30, (69 * 3) % 20, (69 * 5) % 20);
    lot52_render(html, 400);
    TEST_ASSERT_EQUAL(1, lot52_node("t")->caret_set);
    TEST_ASSERT_EQUAL(want & 0xffffffu, lot52_node("t")->caret);
    TEST_ASSERT_EQUAL((69 % 4 == 0) ? 1 : (69 % 4 == 1) ? 2 : (69 % 4 == 2) ? 3 : 0,
                      lot52_node("t")->user_select);
    TEST_ASSERT_EQUAL((69 % 2) ? 1 : 0, lot52_node("t")->pointer_events);
    TEST_ASSERT_EQUAL(1, lot52_node("t")->scroll_m_set);
    TEST_ASSERT_EQUAL(69 % 30, lot52_node("t")->scroll_m[0]);
    TEST_ASSERT_EQUAL(1, lot52_node("t")->scroll_p_set);
}

static void test_l75_css_case_70(void) {
    char html[512];
    unsigned want = (unsigned)(0x100000u + 70 * 17u);
    sprintf(html, "<style>#t{caret-color:#%06x;user-select:%s;pointer-events:%s;"
                  "scroll-margin:%dpx;scroll-padding:%dpx %dpx}</style><p id=t>x</p>",
            (unsigned)(want & 0xffffffu),
            (70 % 4 == 0) ? "none" : (70 % 4 == 1) ? "text" : (70 % 4 == 2) ? "all" : "auto",
            (70 % 2) ? "none" : "auto",
            70 % 30, (70 * 3) % 20, (70 * 5) % 20);
    lot52_render(html, 400);
    TEST_ASSERT_EQUAL(1, lot52_node("t")->caret_set);
    TEST_ASSERT_EQUAL(want & 0xffffffu, lot52_node("t")->caret);
    TEST_ASSERT_EQUAL((70 % 4 == 0) ? 1 : (70 % 4 == 1) ? 2 : (70 % 4 == 2) ? 3 : 0,
                      lot52_node("t")->user_select);
    TEST_ASSERT_EQUAL((70 % 2) ? 1 : 0, lot52_node("t")->pointer_events);
    TEST_ASSERT_EQUAL(1, lot52_node("t")->scroll_m_set);
    TEST_ASSERT_EQUAL(70 % 30, lot52_node("t")->scroll_m[0]);
    TEST_ASSERT_EQUAL(1, lot52_node("t")->scroll_p_set);
}

static void test_l75_css_case_71(void) {
    char html[512];
    unsigned want = (unsigned)(0x100000u + 71 * 17u);
    sprintf(html, "<style>#t{caret-color:#%06x;user-select:%s;pointer-events:%s;"
                  "scroll-margin:%dpx;scroll-padding:%dpx %dpx}</style><p id=t>x</p>",
            (unsigned)(want & 0xffffffu),
            (71 % 4 == 0) ? "none" : (71 % 4 == 1) ? "text" : (71 % 4 == 2) ? "all" : "auto",
            (71 % 2) ? "none" : "auto",
            71 % 30, (71 * 3) % 20, (71 * 5) % 20);
    lot52_render(html, 400);
    TEST_ASSERT_EQUAL(1, lot52_node("t")->caret_set);
    TEST_ASSERT_EQUAL(want & 0xffffffu, lot52_node("t")->caret);
    TEST_ASSERT_EQUAL((71 % 4 == 0) ? 1 : (71 % 4 == 1) ? 2 : (71 % 4 == 2) ? 3 : 0,
                      lot52_node("t")->user_select);
    TEST_ASSERT_EQUAL((71 % 2) ? 1 : 0, lot52_node("t")->pointer_events);
    TEST_ASSERT_EQUAL(1, lot52_node("t")->scroll_m_set);
    TEST_ASSERT_EQUAL(71 % 30, lot52_node("t")->scroll_m[0]);
    TEST_ASSERT_EQUAL(1, lot52_node("t")->scroll_p_set);
}

static void test_l75_css_case_72(void) {
    char html[512];
    unsigned want = (unsigned)(0x100000u + 72 * 17u);
    sprintf(html, "<style>#t{caret-color:#%06x;user-select:%s;pointer-events:%s;"
                  "scroll-margin:%dpx;scroll-padding:%dpx %dpx}</style><p id=t>x</p>",
            (unsigned)(want & 0xffffffu),
            (72 % 4 == 0) ? "none" : (72 % 4 == 1) ? "text" : (72 % 4 == 2) ? "all" : "auto",
            (72 % 2) ? "none" : "auto",
            72 % 30, (72 * 3) % 20, (72 * 5) % 20);
    lot52_render(html, 400);
    TEST_ASSERT_EQUAL(1, lot52_node("t")->caret_set);
    TEST_ASSERT_EQUAL(want & 0xffffffu, lot52_node("t")->caret);
    TEST_ASSERT_EQUAL((72 % 4 == 0) ? 1 : (72 % 4 == 1) ? 2 : (72 % 4 == 2) ? 3 : 0,
                      lot52_node("t")->user_select);
    TEST_ASSERT_EQUAL((72 % 2) ? 1 : 0, lot52_node("t")->pointer_events);
    TEST_ASSERT_EQUAL(1, lot52_node("t")->scroll_m_set);
    TEST_ASSERT_EQUAL(72 % 30, lot52_node("t")->scroll_m[0]);
    TEST_ASSERT_EQUAL(1, lot52_node("t")->scroll_p_set);
}

static void test_l75_css_case_73(void) {
    char html[512];
    unsigned want = (unsigned)(0x100000u + 73 * 17u);
    sprintf(html, "<style>#t{caret-color:#%06x;user-select:%s;pointer-events:%s;"
                  "scroll-margin:%dpx;scroll-padding:%dpx %dpx}</style><p id=t>x</p>",
            (unsigned)(want & 0xffffffu),
            (73 % 4 == 0) ? "none" : (73 % 4 == 1) ? "text" : (73 % 4 == 2) ? "all" : "auto",
            (73 % 2) ? "none" : "auto",
            73 % 30, (73 * 3) % 20, (73 * 5) % 20);
    lot52_render(html, 400);
    TEST_ASSERT_EQUAL(1, lot52_node("t")->caret_set);
    TEST_ASSERT_EQUAL(want & 0xffffffu, lot52_node("t")->caret);
    TEST_ASSERT_EQUAL((73 % 4 == 0) ? 1 : (73 % 4 == 1) ? 2 : (73 % 4 == 2) ? 3 : 0,
                      lot52_node("t")->user_select);
    TEST_ASSERT_EQUAL((73 % 2) ? 1 : 0, lot52_node("t")->pointer_events);
    TEST_ASSERT_EQUAL(1, lot52_node("t")->scroll_m_set);
    TEST_ASSERT_EQUAL(73 % 30, lot52_node("t")->scroll_m[0]);
    TEST_ASSERT_EQUAL(1, lot52_node("t")->scroll_p_set);
}

static void test_l75_css_case_74(void) {
    char html[512];
    unsigned want = (unsigned)(0x100000u + 74 * 17u);
    sprintf(html, "<style>#t{caret-color:#%06x;user-select:%s;pointer-events:%s;"
                  "scroll-margin:%dpx;scroll-padding:%dpx %dpx}</style><p id=t>x</p>",
            (unsigned)(want & 0xffffffu),
            (74 % 4 == 0) ? "none" : (74 % 4 == 1) ? "text" : (74 % 4 == 2) ? "all" : "auto",
            (74 % 2) ? "none" : "auto",
            74 % 30, (74 * 3) % 20, (74 * 5) % 20);
    lot52_render(html, 400);
    TEST_ASSERT_EQUAL(1, lot52_node("t")->caret_set);
    TEST_ASSERT_EQUAL(want & 0xffffffu, lot52_node("t")->caret);
    TEST_ASSERT_EQUAL((74 % 4 == 0) ? 1 : (74 % 4 == 1) ? 2 : (74 % 4 == 2) ? 3 : 0,
                      lot52_node("t")->user_select);
    TEST_ASSERT_EQUAL((74 % 2) ? 1 : 0, lot52_node("t")->pointer_events);
    TEST_ASSERT_EQUAL(1, lot52_node("t")->scroll_m_set);
    TEST_ASSERT_EQUAL(74 % 30, lot52_node("t")->scroll_m[0]);
    TEST_ASSERT_EQUAL(1, lot52_node("t")->scroll_p_set);
}

static void test_l75_css_case_75(void) {
    char html[512];
    unsigned want = (unsigned)(0x100000u + 75 * 17u);
    sprintf(html, "<style>#t{caret-color:#%06x;user-select:%s;pointer-events:%s;"
                  "scroll-margin:%dpx;scroll-padding:%dpx %dpx}</style><p id=t>x</p>",
            (unsigned)(want & 0xffffffu),
            (75 % 4 == 0) ? "none" : (75 % 4 == 1) ? "text" : (75 % 4 == 2) ? "all" : "auto",
            (75 % 2) ? "none" : "auto",
            75 % 30, (75 * 3) % 20, (75 * 5) % 20);
    lot52_render(html, 400);
    TEST_ASSERT_EQUAL(1, lot52_node("t")->caret_set);
    TEST_ASSERT_EQUAL(want & 0xffffffu, lot52_node("t")->caret);
    TEST_ASSERT_EQUAL((75 % 4 == 0) ? 1 : (75 % 4 == 1) ? 2 : (75 % 4 == 2) ? 3 : 0,
                      lot52_node("t")->user_select);
    TEST_ASSERT_EQUAL((75 % 2) ? 1 : 0, lot52_node("t")->pointer_events);
    TEST_ASSERT_EQUAL(1, lot52_node("t")->scroll_m_set);
    TEST_ASSERT_EQUAL(75 % 30, lot52_node("t")->scroll_m[0]);
    TEST_ASSERT_EQUAL(1, lot52_node("t")->scroll_p_set);
}

static void test_l75_css_case_76(void) {
    char html[512];
    unsigned want = (unsigned)(0x100000u + 76 * 17u);
    sprintf(html, "<style>#t{caret-color:#%06x;user-select:%s;pointer-events:%s;"
                  "scroll-margin:%dpx;scroll-padding:%dpx %dpx}</style><p id=t>x</p>",
            (unsigned)(want & 0xffffffu),
            (76 % 4 == 0) ? "none" : (76 % 4 == 1) ? "text" : (76 % 4 == 2) ? "all" : "auto",
            (76 % 2) ? "none" : "auto",
            76 % 30, (76 * 3) % 20, (76 * 5) % 20);
    lot52_render(html, 400);
    TEST_ASSERT_EQUAL(1, lot52_node("t")->caret_set);
    TEST_ASSERT_EQUAL(want & 0xffffffu, lot52_node("t")->caret);
    TEST_ASSERT_EQUAL((76 % 4 == 0) ? 1 : (76 % 4 == 1) ? 2 : (76 % 4 == 2) ? 3 : 0,
                      lot52_node("t")->user_select);
    TEST_ASSERT_EQUAL((76 % 2) ? 1 : 0, lot52_node("t")->pointer_events);
    TEST_ASSERT_EQUAL(1, lot52_node("t")->scroll_m_set);
    TEST_ASSERT_EQUAL(76 % 30, lot52_node("t")->scroll_m[0]);
    TEST_ASSERT_EQUAL(1, lot52_node("t")->scroll_p_set);
}

static void test_l75_css_case_77(void) {
    char html[512];
    unsigned want = (unsigned)(0x100000u + 77 * 17u);
    sprintf(html, "<style>#t{caret-color:#%06x;user-select:%s;pointer-events:%s;"
                  "scroll-margin:%dpx;scroll-padding:%dpx %dpx}</style><p id=t>x</p>",
            (unsigned)(want & 0xffffffu),
            (77 % 4 == 0) ? "none" : (77 % 4 == 1) ? "text" : (77 % 4 == 2) ? "all" : "auto",
            (77 % 2) ? "none" : "auto",
            77 % 30, (77 * 3) % 20, (77 * 5) % 20);
    lot52_render(html, 400);
    TEST_ASSERT_EQUAL(1, lot52_node("t")->caret_set);
    TEST_ASSERT_EQUAL(want & 0xffffffu, lot52_node("t")->caret);
    TEST_ASSERT_EQUAL((77 % 4 == 0) ? 1 : (77 % 4 == 1) ? 2 : (77 % 4 == 2) ? 3 : 0,
                      lot52_node("t")->user_select);
    TEST_ASSERT_EQUAL((77 % 2) ? 1 : 0, lot52_node("t")->pointer_events);
    TEST_ASSERT_EQUAL(1, lot52_node("t")->scroll_m_set);
    TEST_ASSERT_EQUAL(77 % 30, lot52_node("t")->scroll_m[0]);
    TEST_ASSERT_EQUAL(1, lot52_node("t")->scroll_p_set);
}

static void test_l75_css_case_78(void) {
    char html[512];
    unsigned want = (unsigned)(0x100000u + 78 * 17u);
    sprintf(html, "<style>#t{caret-color:#%06x;user-select:%s;pointer-events:%s;"
                  "scroll-margin:%dpx;scroll-padding:%dpx %dpx}</style><p id=t>x</p>",
            (unsigned)(want & 0xffffffu),
            (78 % 4 == 0) ? "none" : (78 % 4 == 1) ? "text" : (78 % 4 == 2) ? "all" : "auto",
            (78 % 2) ? "none" : "auto",
            78 % 30, (78 * 3) % 20, (78 * 5) % 20);
    lot52_render(html, 400);
    TEST_ASSERT_EQUAL(1, lot52_node("t")->caret_set);
    TEST_ASSERT_EQUAL(want & 0xffffffu, lot52_node("t")->caret);
    TEST_ASSERT_EQUAL((78 % 4 == 0) ? 1 : (78 % 4 == 1) ? 2 : (78 % 4 == 2) ? 3 : 0,
                      lot52_node("t")->user_select);
    TEST_ASSERT_EQUAL((78 % 2) ? 1 : 0, lot52_node("t")->pointer_events);
    TEST_ASSERT_EQUAL(1, lot52_node("t")->scroll_m_set);
    TEST_ASSERT_EQUAL(78 % 30, lot52_node("t")->scroll_m[0]);
    TEST_ASSERT_EQUAL(1, lot52_node("t")->scroll_p_set);
}

static void test_l75_css_case_79(void) {
    char html[512];
    unsigned want = (unsigned)(0x100000u + 79 * 17u);
    sprintf(html, "<style>#t{caret-color:#%06x;user-select:%s;pointer-events:%s;"
                  "scroll-margin:%dpx;scroll-padding:%dpx %dpx}</style><p id=t>x</p>",
            (unsigned)(want & 0xffffffu),
            (79 % 4 == 0) ? "none" : (79 % 4 == 1) ? "text" : (79 % 4 == 2) ? "all" : "auto",
            (79 % 2) ? "none" : "auto",
            79 % 30, (79 * 3) % 20, (79 * 5) % 20);
    lot52_render(html, 400);
    TEST_ASSERT_EQUAL(1, lot52_node("t")->caret_set);
    TEST_ASSERT_EQUAL(want & 0xffffffu, lot52_node("t")->caret);
    TEST_ASSERT_EQUAL((79 % 4 == 0) ? 1 : (79 % 4 == 1) ? 2 : (79 % 4 == 2) ? 3 : 0,
                      lot52_node("t")->user_select);
    TEST_ASSERT_EQUAL((79 % 2) ? 1 : 0, lot52_node("t")->pointer_events);
    TEST_ASSERT_EQUAL(1, lot52_node("t")->scroll_m_set);
    TEST_ASSERT_EQUAL(79 % 30, lot52_node("t")->scroll_m[0]);
    TEST_ASSERT_EQUAL(1, lot52_node("t")->scroll_p_set);
}

static void test_l75_css_case_80(void) {
    char html[512];
    unsigned want = (unsigned)(0x100000u + 80 * 17u);
    sprintf(html, "<style>#t{caret-color:#%06x;user-select:%s;pointer-events:%s;"
                  "scroll-margin:%dpx;scroll-padding:%dpx %dpx}</style><p id=t>x</p>",
            (unsigned)(want & 0xffffffu),
            (80 % 4 == 0) ? "none" : (80 % 4 == 1) ? "text" : (80 % 4 == 2) ? "all" : "auto",
            (80 % 2) ? "none" : "auto",
            80 % 30, (80 * 3) % 20, (80 * 5) % 20);
    lot52_render(html, 400);
    TEST_ASSERT_EQUAL(1, lot52_node("t")->caret_set);
    TEST_ASSERT_EQUAL(want & 0xffffffu, lot52_node("t")->caret);
    TEST_ASSERT_EQUAL((80 % 4 == 0) ? 1 : (80 % 4 == 1) ? 2 : (80 % 4 == 2) ? 3 : 0,
                      lot52_node("t")->user_select);
    TEST_ASSERT_EQUAL((80 % 2) ? 1 : 0, lot52_node("t")->pointer_events);
    TEST_ASSERT_EQUAL(1, lot52_node("t")->scroll_m_set);
    TEST_ASSERT_EQUAL(80 % 30, lot52_node("t")->scroll_m[0]);
    TEST_ASSERT_EQUAL(1, lot52_node("t")->scroll_p_set);
}
static void lot75_more_run(void) {
    RUN_TEST(test_l75_form_attr_association);
    RUN_TEST(test_l75_form_attr_missing_unbound);
    RUN_TEST(test_l75_formaction_override);
    RUN_TEST(test_l75_formenctype_multipart);
    RUN_TEST(test_l75_caret_color);
    RUN_TEST(test_l75_user_select);
    RUN_TEST(test_l75_pointer_events_none);
    RUN_TEST(test_l75_scroll_margin_padding);
    RUN_TEST(test_l75_css_case_1);
    RUN_TEST(test_l75_css_case_2);
    RUN_TEST(test_l75_css_case_3);
    RUN_TEST(test_l75_css_case_4);
    RUN_TEST(test_l75_css_case_5);
    RUN_TEST(test_l75_css_case_6);
    RUN_TEST(test_l75_css_case_7);
    RUN_TEST(test_l75_css_case_8);
    RUN_TEST(test_l75_css_case_9);
    RUN_TEST(test_l75_css_case_10);
    RUN_TEST(test_l75_css_case_11);
    RUN_TEST(test_l75_css_case_12);
    RUN_TEST(test_l75_css_case_13);
    RUN_TEST(test_l75_css_case_14);
    RUN_TEST(test_l75_css_case_15);
    RUN_TEST(test_l75_css_case_16);
    RUN_TEST(test_l75_css_case_17);
    RUN_TEST(test_l75_css_case_18);
    RUN_TEST(test_l75_css_case_19);
    RUN_TEST(test_l75_css_case_20);
    RUN_TEST(test_l75_css_case_21);
    RUN_TEST(test_l75_css_case_22);
    RUN_TEST(test_l75_css_case_23);
    RUN_TEST(test_l75_css_case_24);
    RUN_TEST(test_l75_css_case_25);
    RUN_TEST(test_l75_css_case_26);
    RUN_TEST(test_l75_css_case_27);
    RUN_TEST(test_l75_css_case_28);
    RUN_TEST(test_l75_css_case_29);
    RUN_TEST(test_l75_css_case_30);
    RUN_TEST(test_l75_css_case_31);
    RUN_TEST(test_l75_css_case_32);
    RUN_TEST(test_l75_css_case_33);
    RUN_TEST(test_l75_css_case_34);
    RUN_TEST(test_l75_css_case_35);
    RUN_TEST(test_l75_css_case_36);
    RUN_TEST(test_l75_css_case_37);
    RUN_TEST(test_l75_css_case_38);
    RUN_TEST(test_l75_css_case_39);
    RUN_TEST(test_l75_css_case_40);
    RUN_TEST(test_l75_css_case_41);
    RUN_TEST(test_l75_css_case_42);
    RUN_TEST(test_l75_css_case_43);
    RUN_TEST(test_l75_css_case_44);
    RUN_TEST(test_l75_css_case_45);
    RUN_TEST(test_l75_css_case_46);
    RUN_TEST(test_l75_css_case_47);
    RUN_TEST(test_l75_css_case_48);
    RUN_TEST(test_l75_css_case_49);
    RUN_TEST(test_l75_css_case_50);
    RUN_TEST(test_l75_css_case_51);
    RUN_TEST(test_l75_css_case_52);
    RUN_TEST(test_l75_css_case_53);
    RUN_TEST(test_l75_css_case_54);
    RUN_TEST(test_l75_css_case_55);
    RUN_TEST(test_l75_css_case_56);
    RUN_TEST(test_l75_css_case_57);
    RUN_TEST(test_l75_css_case_58);
    RUN_TEST(test_l75_css_case_59);
    RUN_TEST(test_l75_css_case_60);
    RUN_TEST(test_l75_css_case_61);
    RUN_TEST(test_l75_css_case_62);
    RUN_TEST(test_l75_css_case_63);
    RUN_TEST(test_l75_css_case_64);
    RUN_TEST(test_l75_css_case_65);
    RUN_TEST(test_l75_css_case_66);
    RUN_TEST(test_l75_css_case_67);
    RUN_TEST(test_l75_css_case_68);
    RUN_TEST(test_l75_css_case_69);
    RUN_TEST(test_l75_css_case_70);
    RUN_TEST(test_l75_css_case_71);
    RUN_TEST(test_l75_css_case_72);
    RUN_TEST(test_l75_css_case_73);
    RUN_TEST(test_l75_css_case_74);
    RUN_TEST(test_l75_css_case_75);
    RUN_TEST(test_l75_css_case_76);
    RUN_TEST(test_l75_css_case_77);
    RUN_TEST(test_l75_css_case_78);
    RUN_TEST(test_l75_css_case_79);
    RUN_TEST(test_l75_css_case_80);
}
