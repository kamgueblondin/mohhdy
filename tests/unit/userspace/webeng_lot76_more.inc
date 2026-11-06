/* Lot 76: more color/date/autocomplete/formtarget/output/attr cases. */
#ifndef WE_FF2_AC_OFF
#define WE_FF2_AC_OFF 524288u
#define WE_FF2_FORMTARGET 1048576u
#define WE_FF2_TARGET_BLANK 2097152u
#endif

static we_forms_t g_l76m;
static const char *g_l76m_html;
static void l76m_render(const char *html) {
    we_forms_init(&g_l76m);
    we_set_forms(&g_doc, &g_l76m);
    g_l76m_html = html;
    TEST_ASSERT_EQUAL(0, we_render(&g_doc, html, (int)strlen(html), 480));
}
static void l76m_done(void) { we_set_forms(&g_doc, 0); g_l76m_html = 0; }
static int l76m_node(const char *id) { int i = we_find_id(&g_doc, id); return i > 0 ? i : 0; }
static int l76m_submit(void) {
    int k, x, y;
    for (k = 1; k < (int)g_doc.nnodes; k++)
        if (g_doc.nodes[k].form_type == WE_FORM_SUBMIT) break;
    x = g_doc.nodes[k].x + 2; y = g_doc.nodes[k].y + 2;
    return we_form_click(&g_doc, x, y);
}

static void test_l76m_color_1(void) {
    we_form_req_t q;
    char html[400];
    sprintf(html, "<form action=/c1 method=GET>"
                  "<input id=c1 type=color name=c1 value=#000000>"
                  "<input type=submit></form>");
    l76m_render(html);
    TEST_ASSERT_EQUAL(WE_FORM_COLOR, g_doc.nodes[l76m_node("c1")].form_type);
    TEST_ASSERT_EQUAL(0, l61_diag_count("input type taken as text"));
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76m_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "c1=%23000000"));
    l76m_done();
}

static void test_l76m_color_2(void) {
    we_form_req_t q;
    char html[400];
    sprintf(html, "<form action=/c2 method=GET>"
                  "<input id=c2 type=color name=c2 value=#ffffff>"
                  "<input type=submit></form>");
    l76m_render(html);
    TEST_ASSERT_EQUAL(WE_FORM_COLOR, g_doc.nodes[l76m_node("c2")].form_type);
    TEST_ASSERT_EQUAL(0, l61_diag_count("input type taken as text"));
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76m_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "c2=%23ffffff"));
    l76m_done();
}

static void test_l76m_color_3(void) {
    we_form_req_t q;
    char html[400];
    sprintf(html, "<form action=/c3 method=GET>"
                  "<input id=c3 type=color name=c3 value=#abcdef>"
                  "<input type=submit></form>");
    l76m_render(html);
    TEST_ASSERT_EQUAL(WE_FORM_COLOR, g_doc.nodes[l76m_node("c3")].form_type);
    TEST_ASSERT_EQUAL(0, l61_diag_count("input type taken as text"));
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76m_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "c3=%23abcdef"));
    l76m_done();
}

static void test_l76m_color_4(void) {
    we_form_req_t q;
    char html[400];
    sprintf(html, "<form action=/c4 method=GET>"
                  "<input id=c4 type=color name=c4 value=#ABCDEF>"
                  "<input type=submit></form>");
    l76m_render(html);
    TEST_ASSERT_EQUAL(WE_FORM_COLOR, g_doc.nodes[l76m_node("c4")].form_type);
    TEST_ASSERT_EQUAL(0, l61_diag_count("input type taken as text"));
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76m_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "c4=%23ABCDEF"));
    l76m_done();
}

static void test_l76m_color_5(void) {
    we_form_req_t q;
    char html[400];
    sprintf(html, "<form action=/c5 method=GET>"
                  "<input id=c5 type=color name=c5 value=#123456>"
                  "<input type=submit></form>");
    l76m_render(html);
    TEST_ASSERT_EQUAL(WE_FORM_COLOR, g_doc.nodes[l76m_node("c5")].form_type);
    TEST_ASSERT_EQUAL(0, l61_diag_count("input type taken as text"));
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76m_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "c5=%23123456"));
    l76m_done();
}

static void test_l76m_color_6(void) {
    we_form_req_t q;
    char html[400];
    sprintf(html, "<form action=/c6 method=GET>"
                  "<input id=c6 type=color name=c6 value=#789abc>"
                  "<input type=submit></form>");
    l76m_render(html);
    TEST_ASSERT_EQUAL(WE_FORM_COLOR, g_doc.nodes[l76m_node("c6")].form_type);
    TEST_ASSERT_EQUAL(0, l61_diag_count("input type taken as text"));
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76m_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "c6=%23789abc"));
    l76m_done();
}

static void test_l76m_color_7(void) {
    we_form_req_t q;
    char html[400];
    sprintf(html, "<form action=/c7 method=GET>"
                  "<input id=c7 type=color name=c7 value=#00ff00>"
                  "<input type=submit></form>");
    l76m_render(html);
    TEST_ASSERT_EQUAL(WE_FORM_COLOR, g_doc.nodes[l76m_node("c7")].form_type);
    TEST_ASSERT_EQUAL(0, l61_diag_count("input type taken as text"));
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76m_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "c7=%2300ff00"));
    l76m_done();
}

static void test_l76m_color_8(void) {
    we_form_req_t q;
    char html[400];
    sprintf(html, "<form action=/c8 method=GET>"
                  "<input id=c8 type=color name=c8 value=#ff0000>"
                  "<input type=submit></form>");
    l76m_render(html);
    TEST_ASSERT_EQUAL(WE_FORM_COLOR, g_doc.nodes[l76m_node("c8")].form_type);
    TEST_ASSERT_EQUAL(0, l61_diag_count("input type taken as text"));
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76m_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "c8=%23ff0000"));
    l76m_done();
}

static void test_l76m_color_9(void) {
    we_form_req_t q;
    char html[400];
    sprintf(html, "<form action=/c9 method=GET>"
                  "<input id=c9 type=color name=c9 value=#0000ff>"
                  "<input type=submit></form>");
    l76m_render(html);
    TEST_ASSERT_EQUAL(WE_FORM_COLOR, g_doc.nodes[l76m_node("c9")].form_type);
    TEST_ASSERT_EQUAL(0, l61_diag_count("input type taken as text"));
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76m_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "c9=%230000ff"));
    l76m_done();
}

static void test_l76m_color_10(void) {
    we_form_req_t q;
    char html[400];
    sprintf(html, "<form action=/c10 method=GET>"
                  "<input id=c10 type=color name=c10 value=#808080>"
                  "<input type=submit></form>");
    l76m_render(html);
    TEST_ASSERT_EQUAL(WE_FORM_COLOR, g_doc.nodes[l76m_node("c10")].form_type);
    TEST_ASSERT_EQUAL(0, l61_diag_count("input type taken as text"));
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76m_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "c10=%23808080"));
    l76m_done();
}

static void test_l76m_color_11(void) {
    we_form_req_t q;
    char html[400];
    sprintf(html, "<form action=/c11 method=GET>"
                  "<input id=c11 type=color name=c11 value=#010203>"
                  "<input type=submit></form>");
    l76m_render(html);
    TEST_ASSERT_EQUAL(WE_FORM_COLOR, g_doc.nodes[l76m_node("c11")].form_type);
    TEST_ASSERT_EQUAL(0, l61_diag_count("input type taken as text"));
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76m_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "c11=%23010203"));
    l76m_done();
}

static void test_l76m_color_12(void) {
    we_form_req_t q;
    char html[400];
    sprintf(html, "<form action=/c12 method=GET>"
                  "<input id=c12 type=color name=c12 value=#f0f0f0>"
                  "<input type=submit></form>");
    l76m_render(html);
    TEST_ASSERT_EQUAL(WE_FORM_COLOR, g_doc.nodes[l76m_node("c12")].form_type);
    TEST_ASSERT_EQUAL(0, l61_diag_count("input type taken as text"));
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76m_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "c12=%23f0f0f0"));
    l76m_done();
}

static void test_l76m_color_13(void) {
    we_form_req_t q;
    char html[400];
    sprintf(html, "<form action=/c13 method=GET>"
                  "<input id=c13 type=color name=c13 value=#0a0b0c>"
                  "<input type=submit></form>");
    l76m_render(html);
    TEST_ASSERT_EQUAL(WE_FORM_COLOR, g_doc.nodes[l76m_node("c13")].form_type);
    TEST_ASSERT_EQUAL(0, l61_diag_count("input type taken as text"));
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76m_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "c13=%230a0b0c"));
    l76m_done();
}

static void test_l76m_color_14(void) {
    we_form_req_t q;
    char html[400];
    sprintf(html, "<form action=/c14 method=GET>"
                  "<input id=c14 type=color name=c14 value=#dead00>"
                  "<input type=submit></form>");
    l76m_render(html);
    TEST_ASSERT_EQUAL(WE_FORM_COLOR, g_doc.nodes[l76m_node("c14")].form_type);
    TEST_ASSERT_EQUAL(0, l61_diag_count("input type taken as text"));
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76m_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "c14=%23dead00"));
    l76m_done();
}

static void test_l76m_color_15(void) {
    we_form_req_t q;
    char html[400];
    sprintf(html, "<form action=/c15 method=GET>"
                  "<input id=c15 type=color name=c15 value=#beef00>"
                  "<input type=submit></form>");
    l76m_render(html);
    TEST_ASSERT_EQUAL(WE_FORM_COLOR, g_doc.nodes[l76m_node("c15")].form_type);
    TEST_ASSERT_EQUAL(0, l61_diag_count("input type taken as text"));
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76m_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "c15=%23beef00"));
    l76m_done();
}

static void test_l76m_color_16(void) {
    we_form_req_t q;
    char html[400];
    sprintf(html, "<form action=/c16 method=GET>"
                  "<input id=c16 type=color name=c16 value=#c0ffee>"
                  "<input type=submit></form>");
    l76m_render(html);
    TEST_ASSERT_EQUAL(WE_FORM_COLOR, g_doc.nodes[l76m_node("c16")].form_type);
    TEST_ASSERT_EQUAL(0, l61_diag_count("input type taken as text"));
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76m_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "c16=%23c0ffee"));
    l76m_done();
}

static void test_l76m_color_17(void) {
    we_form_req_t q;
    char html[400];
    sprintf(html, "<form action=/c17 method=GET>"
                  "<input id=c17 type=color name=c17 value=#facade>"
                  "<input type=submit></form>");
    l76m_render(html);
    TEST_ASSERT_EQUAL(WE_FORM_COLOR, g_doc.nodes[l76m_node("c17")].form_type);
    TEST_ASSERT_EQUAL(0, l61_diag_count("input type taken as text"));
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76m_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "c17=%23facade"));
    l76m_done();
}

static void test_l76m_color_18(void) {
    we_form_req_t q;
    char html[400];
    sprintf(html, "<form action=/c18 method=GET>"
                  "<input id=c18 type=color name=c18 value=#bababa>"
                  "<input type=submit></form>");
    l76m_render(html);
    TEST_ASSERT_EQUAL(WE_FORM_COLOR, g_doc.nodes[l76m_node("c18")].form_type);
    TEST_ASSERT_EQUAL(0, l61_diag_count("input type taken as text"));
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76m_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "c18=%23bababa"));
    l76m_done();
}

static void test_l76m_color_19(void) {
    we_form_req_t q;
    char html[400];
    sprintf(html, "<form action=/c19 method=GET>"
                  "<input id=c19 type=color name=c19 value=#111111>"
                  "<input type=submit></form>");
    l76m_render(html);
    TEST_ASSERT_EQUAL(WE_FORM_COLOR, g_doc.nodes[l76m_node("c19")].form_type);
    TEST_ASSERT_EQUAL(0, l61_diag_count("input type taken as text"));
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76m_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "c19=%23111111"));
    l76m_done();
}

static void test_l76m_color_20(void) {
    we_form_req_t q;
    char html[400];
    sprintf(html, "<form action=/c20 method=GET>"
                  "<input id=c20 type=color name=c20 value=#eeeeee>"
                  "<input type=submit></form>");
    l76m_render(html);
    TEST_ASSERT_EQUAL(WE_FORM_COLOR, g_doc.nodes[l76m_node("c20")].form_type);
    TEST_ASSERT_EQUAL(0, l61_diag_count("input type taken as text"));
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76m_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "c20=%23eeeeee"));
    l76m_done();
}

static void test_l76m_color_21(void) {
    we_form_req_t q;
    char html[400];
    sprintf(html, "<form action=/c21 method=GET>"
                  "<input id=c21 type=color name=c21 value=#a1b2c3>"
                  "<input type=submit></form>");
    l76m_render(html);
    TEST_ASSERT_EQUAL(WE_FORM_COLOR, g_doc.nodes[l76m_node("c21")].form_type);
    TEST_ASSERT_EQUAL(0, l61_diag_count("input type taken as text"));
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76m_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "c21=%23a1b2c3"));
    l76m_done();
}

static void test_l76m_color_22(void) {
    we_form_req_t q;
    char html[400];
    sprintf(html, "<form action=/c22 method=GET>"
                  "<input id=c22 type=color name=c22 value=#d4e5f6>"
                  "<input type=submit></form>");
    l76m_render(html);
    TEST_ASSERT_EQUAL(WE_FORM_COLOR, g_doc.nodes[l76m_node("c22")].form_type);
    TEST_ASSERT_EQUAL(0, l61_diag_count("input type taken as text"));
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76m_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "c22=%23d4e5f6"));
    l76m_done();
}

static void test_l76m_color_23(void) {
    we_form_req_t q;
    char html[400];
    sprintf(html, "<form action=/c23 method=GET>"
                  "<input id=c23 type=color name=c23 value=#998877>"
                  "<input type=submit></form>");
    l76m_render(html);
    TEST_ASSERT_EQUAL(WE_FORM_COLOR, g_doc.nodes[l76m_node("c23")].form_type);
    TEST_ASSERT_EQUAL(0, l61_diag_count("input type taken as text"));
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76m_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "c23=%23998877"));
    l76m_done();
}

static void test_l76m_color_24(void) {
    we_form_req_t q;
    char html[400];
    sprintf(html, "<form action=/c24 method=GET>"
                  "<input id=c24 type=color name=c24 value=#665544>"
                  "<input type=submit></form>");
    l76m_render(html);
    TEST_ASSERT_EQUAL(WE_FORM_COLOR, g_doc.nodes[l76m_node("c24")].form_type);
    TEST_ASSERT_EQUAL(0, l61_diag_count("input type taken as text"));
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76m_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "c24=%23665544"));
    l76m_done();
}

static void test_l76m_color_25(void) {
    we_form_req_t q;
    char html[400];
    sprintf(html, "<form action=/c25 method=GET>"
                  "<input id=c25 type=color name=c25 value=#334455>"
                  "<input type=submit></form>");
    l76m_render(html);
    TEST_ASSERT_EQUAL(WE_FORM_COLOR, g_doc.nodes[l76m_node("c25")].form_type);
    TEST_ASSERT_EQUAL(0, l61_diag_count("input type taken as text"));
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76m_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "c25=%23334455"));
    l76m_done();
}

static void test_l76m_color_26(void) {
    we_form_req_t q;
    char html[400];
    sprintf(html, "<form action=/c26 method=GET>"
                  "<input id=c26 type=color name=c26 value=#aa55aa>"
                  "<input type=submit></form>");
    l76m_render(html);
    TEST_ASSERT_EQUAL(WE_FORM_COLOR, g_doc.nodes[l76m_node("c26")].form_type);
    TEST_ASSERT_EQUAL(0, l61_diag_count("input type taken as text"));
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76m_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "c26=%23aa55aa"));
    l76m_done();
}

static void test_l76m_color_27(void) {
    we_form_req_t q;
    char html[400];
    sprintf(html, "<form action=/c27 method=GET>"
                  "<input id=c27 type=color name=c27 value=#55aa55>"
                  "<input type=submit></form>");
    l76m_render(html);
    TEST_ASSERT_EQUAL(WE_FORM_COLOR, g_doc.nodes[l76m_node("c27")].form_type);
    TEST_ASSERT_EQUAL(0, l61_diag_count("input type taken as text"));
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76m_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "c27=%2355aa55"));
    l76m_done();
}

static void test_l76m_color_28(void) {
    we_form_req_t q;
    char html[400];
    sprintf(html, "<form action=/c28 method=GET>"
                  "<input id=c28 type=color name=c28 value=#5555aa>"
                  "<input type=submit></form>");
    l76m_render(html);
    TEST_ASSERT_EQUAL(WE_FORM_COLOR, g_doc.nodes[l76m_node("c28")].form_type);
    TEST_ASSERT_EQUAL(0, l61_diag_count("input type taken as text"));
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76m_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "c28=%235555aa"));
    l76m_done();
}

static void test_l76m_color_29(void) {
    we_form_req_t q;
    char html[400];
    sprintf(html, "<form action=/c29 method=GET>"
                  "<input id=c29 type=color name=c29 value=#cafeba>"
                  "<input type=submit></form>");
    l76m_render(html);
    TEST_ASSERT_EQUAL(WE_FORM_COLOR, g_doc.nodes[l76m_node("c29")].form_type);
    TEST_ASSERT_EQUAL(0, l61_diag_count("input type taken as text"));
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76m_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "c29=%23cafeba"));
    l76m_done();
}

static void test_l76m_color_30(void) {
    we_form_req_t q;
    char html[400];
    sprintf(html, "<form action=/c30 method=GET>"
                  "<input id=c30 type=color name=c30 value=#beaded>"
                  "<input type=submit></form>");
    l76m_render(html);
    TEST_ASSERT_EQUAL(WE_FORM_COLOR, g_doc.nodes[l76m_node("c30")].form_type);
    TEST_ASSERT_EQUAL(0, l61_diag_count("input type taken as text"));
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76m_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "c30=%23beaded"));
    l76m_done();
}

static void test_l76m_date_1(void) {
    we_form_req_t q;
    char html[400];
    sprintf(html, "<form action=/d1 method=GET>"
                  "<input id=d1 type=date name=d1 value=2020-01-01>"
                  "<input type=submit></form>");
    l76m_render(html);
    TEST_ASSERT_EQUAL(WE_FORM_DATE, g_doc.nodes[l76m_node("d1")].form_type);
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76m_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "d1=2020-01-01"));
    l76m_done();
}

static void test_l76m_date_2(void) {
    we_form_req_t q;
    char html[400];
    sprintf(html, "<form action=/d2 method=GET>"
                  "<input id=d2 type=date name=d2 value=2020-12-31>"
                  "<input type=submit></form>");
    l76m_render(html);
    TEST_ASSERT_EQUAL(WE_FORM_DATE, g_doc.nodes[l76m_node("d2")].form_type);
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76m_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "d2=2020-12-31"));
    l76m_done();
}

static void test_l76m_date_3(void) {
    we_form_req_t q;
    char html[400];
    sprintf(html, "<form action=/d3 method=GET>"
                  "<input id=d3 type=date name=d3 value=2021-02-28>"
                  "<input type=submit></form>");
    l76m_render(html);
    TEST_ASSERT_EQUAL(WE_FORM_DATE, g_doc.nodes[l76m_node("d3")].form_type);
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76m_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "d3=2021-02-28"));
    l76m_done();
}

static void test_l76m_date_4(void) {
    we_form_req_t q;
    char html[400];
    sprintf(html, "<form action=/d4 method=GET>"
                  "<input id=d4 type=date name=d4 value=2024-02-29>"
                  "<input type=submit></form>");
    l76m_render(html);
    TEST_ASSERT_EQUAL(WE_FORM_DATE, g_doc.nodes[l76m_node("d4")].form_type);
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76m_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "d4=2024-02-29"));
    l76m_done();
}

static void test_l76m_date_5(void) {
    we_form_req_t q;
    char html[400];
    sprintf(html, "<form action=/d5 method=GET>"
                  "<input id=d5 type=date name=d5 value=1999-06-15>"
                  "<input type=submit></form>");
    l76m_render(html);
    TEST_ASSERT_EQUAL(WE_FORM_DATE, g_doc.nodes[l76m_node("d5")].form_type);
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76m_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "d5=1999-06-15"));
    l76m_done();
}

static void test_l76m_date_6(void) {
    we_form_req_t q;
    char html[400];
    sprintf(html, "<form action=/d6 method=GET>"
                  "<input id=d6 type=date name=d6 value=2000-02-29>"
                  "<input type=submit></form>");
    l76m_render(html);
    TEST_ASSERT_EQUAL(WE_FORM_DATE, g_doc.nodes[l76m_node("d6")].form_type);
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76m_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "d6=2000-02-29"));
    l76m_done();
}

static void test_l76m_date_7(void) {
    we_form_req_t q;
    char html[400];
    sprintf(html, "<form action=/d7 method=GET>"
                  "<input id=d7 type=date name=d7 value=2016-02-29>"
                  "<input type=submit></form>");
    l76m_render(html);
    TEST_ASSERT_EQUAL(WE_FORM_DATE, g_doc.nodes[l76m_node("d7")].form_type);
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76m_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "d7=2016-02-29"));
    l76m_done();
}

static void test_l76m_date_8(void) {
    we_form_req_t q;
    char html[400];
    sprintf(html, "<form action=/d8 method=GET>"
                  "<input id=d8 type=date name=d8 value=2030-07-04>"
                  "<input type=submit></form>");
    l76m_render(html);
    TEST_ASSERT_EQUAL(WE_FORM_DATE, g_doc.nodes[l76m_node("d8")].form_type);
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76m_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "d8=2030-07-04"));
    l76m_done();
}

static void test_l76m_date_9(void) {
    we_form_req_t q;
    char html[400];
    sprintf(html, "<form action=/d9 method=GET>"
                  "<input id=d9 type=date name=d9 value=1970-01-01>"
                  "<input type=submit></form>");
    l76m_render(html);
    TEST_ASSERT_EQUAL(WE_FORM_DATE, g_doc.nodes[l76m_node("d9")].form_type);
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76m_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "d9=1970-01-01"));
    l76m_done();
}

static void test_l76m_date_10(void) {
    we_form_req_t q;
    char html[400];
    sprintf(html, "<form action=/d10 method=GET>"
                  "<input id=d10 type=date name=d10 value=2099-12-31>"
                  "<input type=submit></form>");
    l76m_render(html);
    TEST_ASSERT_EQUAL(WE_FORM_DATE, g_doc.nodes[l76m_node("d10")].form_type);
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76m_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "d10=2099-12-31"));
    l76m_done();
}

static void test_l76m_date_11(void) {
    we_form_req_t q;
    char html[400];
    sprintf(html, "<form action=/d11 method=GET>"
                  "<input id=d11 type=date name=d11 value=2022-03-01>"
                  "<input type=submit></form>");
    l76m_render(html);
    TEST_ASSERT_EQUAL(WE_FORM_DATE, g_doc.nodes[l76m_node("d11")].form_type);
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76m_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "d11=2022-03-01"));
    l76m_done();
}

static void test_l76m_date_12(void) {
    we_form_req_t q;
    char html[400];
    sprintf(html, "<form action=/d12 method=GET>"
                  "<input id=d12 type=date name=d12 value=2022-04-30>"
                  "<input type=submit></form>");
    l76m_render(html);
    TEST_ASSERT_EQUAL(WE_FORM_DATE, g_doc.nodes[l76m_node("d12")].form_type);
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76m_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "d12=2022-04-30"));
    l76m_done();
}

static void test_l76m_date_13(void) {
    we_form_req_t q;
    char html[400];
    sprintf(html, "<form action=/d13 method=GET>"
                  "<input id=d13 type=date name=d13 value=2022-05-31>"
                  "<input type=submit></form>");
    l76m_render(html);
    TEST_ASSERT_EQUAL(WE_FORM_DATE, g_doc.nodes[l76m_node("d13")].form_type);
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76m_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "d13=2022-05-31"));
    l76m_done();
}

static void test_l76m_date_14(void) {
    we_form_req_t q;
    char html[400];
    sprintf(html, "<form action=/d14 method=GET>"
                  "<input id=d14 type=date name=d14 value=2022-06-30>"
                  "<input type=submit></form>");
    l76m_render(html);
    TEST_ASSERT_EQUAL(WE_FORM_DATE, g_doc.nodes[l76m_node("d14")].form_type);
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76m_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "d14=2022-06-30"));
    l76m_done();
}

static void test_l76m_date_15(void) {
    we_form_req_t q;
    char html[400];
    sprintf(html, "<form action=/d15 method=GET>"
                  "<input id=d15 type=date name=d15 value=2022-07-31>"
                  "<input type=submit></form>");
    l76m_render(html);
    TEST_ASSERT_EQUAL(WE_FORM_DATE, g_doc.nodes[l76m_node("d15")].form_type);
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76m_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "d15=2022-07-31"));
    l76m_done();
}

static void test_l76m_date_16(void) {
    we_form_req_t q;
    char html[400];
    sprintf(html, "<form action=/d16 method=GET>"
                  "<input id=d16 type=date name=d16 value=2022-08-31>"
                  "<input type=submit></form>");
    l76m_render(html);
    TEST_ASSERT_EQUAL(WE_FORM_DATE, g_doc.nodes[l76m_node("d16")].form_type);
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76m_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "d16=2022-08-31"));
    l76m_done();
}

static void test_l76m_date_17(void) {
    we_form_req_t q;
    char html[400];
    sprintf(html, "<form action=/d17 method=GET>"
                  "<input id=d17 type=date name=d17 value=2022-09-30>"
                  "<input type=submit></form>");
    l76m_render(html);
    TEST_ASSERT_EQUAL(WE_FORM_DATE, g_doc.nodes[l76m_node("d17")].form_type);
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76m_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "d17=2022-09-30"));
    l76m_done();
}

static void test_l76m_date_18(void) {
    we_form_req_t q;
    char html[400];
    sprintf(html, "<form action=/d18 method=GET>"
                  "<input id=d18 type=date name=d18 value=2022-10-31>"
                  "<input type=submit></form>");
    l76m_render(html);
    TEST_ASSERT_EQUAL(WE_FORM_DATE, g_doc.nodes[l76m_node("d18")].form_type);
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76m_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "d18=2022-10-31"));
    l76m_done();
}

static void test_l76m_date_19(void) {
    we_form_req_t q;
    char html[400];
    sprintf(html, "<form action=/d19 method=GET>"
                  "<input id=d19 type=date name=d19 value=2022-11-30>"
                  "<input type=submit></form>");
    l76m_render(html);
    TEST_ASSERT_EQUAL(WE_FORM_DATE, g_doc.nodes[l76m_node("d19")].form_type);
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76m_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "d19=2022-11-30"));
    l76m_done();
}

static void test_l76m_date_20(void) {
    we_form_req_t q;
    char html[400];
    sprintf(html, "<form action=/d20 method=GET>"
                  "<input id=d20 type=date name=d20 value=2022-12-25>"
                  "<input type=submit></form>");
    l76m_render(html);
    TEST_ASSERT_EQUAL(WE_FORM_DATE, g_doc.nodes[l76m_node("d20")].form_type);
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76m_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "d20=2022-12-25"));
    l76m_done();
}

static void test_l76m_date_21(void) {
    we_form_req_t q;
    char html[400];
    sprintf(html, "<form action=/d21 method=GET>"
                  "<input id=d21 type=date name=d21 value=2015-01-15>"
                  "<input type=submit></form>");
    l76m_render(html);
    TEST_ASSERT_EQUAL(WE_FORM_DATE, g_doc.nodes[l76m_node("d21")].form_type);
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76m_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "d21=2015-01-15"));
    l76m_done();
}

static void test_l76m_date_22(void) {
    we_form_req_t q;
    char html[400];
    sprintf(html, "<form action=/d22 method=GET>"
                  "<input id=d22 type=date name=d22 value=2015-02-15>"
                  "<input type=submit></form>");
    l76m_render(html);
    TEST_ASSERT_EQUAL(WE_FORM_DATE, g_doc.nodes[l76m_node("d22")].form_type);
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76m_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "d22=2015-02-15"));
    l76m_done();
}

static void test_l76m_date_23(void) {
    we_form_req_t q;
    char html[400];
    sprintf(html, "<form action=/d23 method=GET>"
                  "<input id=d23 type=date name=d23 value=2015-03-15>"
                  "<input type=submit></form>");
    l76m_render(html);
    TEST_ASSERT_EQUAL(WE_FORM_DATE, g_doc.nodes[l76m_node("d23")].form_type);
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76m_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "d23=2015-03-15"));
    l76m_done();
}

static void test_l76m_date_24(void) {
    we_form_req_t q;
    char html[400];
    sprintf(html, "<form action=/d24 method=GET>"
                  "<input id=d24 type=date name=d24 value=2015-04-15>"
                  "<input type=submit></form>");
    l76m_render(html);
    TEST_ASSERT_EQUAL(WE_FORM_DATE, g_doc.nodes[l76m_node("d24")].form_type);
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76m_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "d24=2015-04-15"));
    l76m_done();
}

static void test_l76m_date_25(void) {
    we_form_req_t q;
    char html[400];
    sprintf(html, "<form action=/d25 method=GET>"
                  "<input id=d25 type=date name=d25 value=2015-05-15>"
                  "<input type=submit></form>");
    l76m_render(html);
    TEST_ASSERT_EQUAL(WE_FORM_DATE, g_doc.nodes[l76m_node("d25")].form_type);
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76m_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "d25=2015-05-15"));
    l76m_done();
}

static void test_l76m_date_26(void) {
    we_form_req_t q;
    char html[400];
    sprintf(html, "<form action=/d26 method=GET>"
                  "<input id=d26 type=date name=d26 value=2015-06-15>"
                  "<input type=submit></form>");
    l76m_render(html);
    TEST_ASSERT_EQUAL(WE_FORM_DATE, g_doc.nodes[l76m_node("d26")].form_type);
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76m_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "d26=2015-06-15"));
    l76m_done();
}

static void test_l76m_date_27(void) {
    we_form_req_t q;
    char html[400];
    sprintf(html, "<form action=/d27 method=GET>"
                  "<input id=d27 type=date name=d27 value=2015-07-15>"
                  "<input type=submit></form>");
    l76m_render(html);
    TEST_ASSERT_EQUAL(WE_FORM_DATE, g_doc.nodes[l76m_node("d27")].form_type);
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76m_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "d27=2015-07-15"));
    l76m_done();
}

static void test_l76m_date_28(void) {
    we_form_req_t q;
    char html[400];
    sprintf(html, "<form action=/d28 method=GET>"
                  "<input id=d28 type=date name=d28 value=2015-08-15>"
                  "<input type=submit></form>");
    l76m_render(html);
    TEST_ASSERT_EQUAL(WE_FORM_DATE, g_doc.nodes[l76m_node("d28")].form_type);
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76m_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "d28=2015-08-15"));
    l76m_done();
}

static void test_l76m_date_29(void) {
    we_form_req_t q;
    char html[400];
    sprintf(html, "<form action=/d29 method=GET>"
                  "<input id=d29 type=date name=d29 value=2015-09-15>"
                  "<input type=submit></form>");
    l76m_render(html);
    TEST_ASSERT_EQUAL(WE_FORM_DATE, g_doc.nodes[l76m_node("d29")].form_type);
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76m_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "d29=2015-09-15"));
    l76m_done();
}

static void test_l76m_date_30(void) {
    we_form_req_t q;
    char html[400];
    sprintf(html, "<form action=/d30 method=GET>"
                  "<input id=d30 type=date name=d30 value=2015-10-15>"
                  "<input type=submit></form>");
    l76m_render(html);
    TEST_ASSERT_EQUAL(WE_FORM_DATE, g_doc.nodes[l76m_node("d30")].form_type);
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76m_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "d30=2015-10-15"));
    l76m_done();
}

static void test_l76m_baddate_1(void) {
    char html[400];
    sprintf(html, "<form action=/x method=GET>"
                  "<input id=b1 type=date name=b1 value=2023-02-29 required>"
                  "<input type=submit></form>");
    l76m_render(html);
    TEST_ASSERT_EQUAL(WE_FACT_CHANGED, l76m_submit());
    l76m_done();
}

static void test_l76m_baddate_2(void) {
    char html[400];
    sprintf(html, "<form action=/x method=GET>"
                  "<input id=b2 type=date name=b2 value=2023-02-30 required>"
                  "<input type=submit></form>");
    l76m_render(html);
    TEST_ASSERT_EQUAL(WE_FACT_CHANGED, l76m_submit());
    l76m_done();
}

static void test_l76m_baddate_3(void) {
    char html[400];
    sprintf(html, "<form action=/x method=GET>"
                  "<input id=b3 type=date name=b3 value=2021-02-29 required>"
                  "<input type=submit></form>");
    l76m_render(html);
    TEST_ASSERT_EQUAL(WE_FACT_CHANGED, l76m_submit());
    l76m_done();
}

static void test_l76m_baddate_4(void) {
    char html[400];
    sprintf(html, "<form action=/x method=GET>"
                  "<input id=b4 type=date name=b4 value=2020-13-01 required>"
                  "<input type=submit></form>");
    l76m_render(html);
    TEST_ASSERT_EQUAL(WE_FACT_CHANGED, l76m_submit());
    l76m_done();
}

static void test_l76m_baddate_5(void) {
    char html[400];
    sprintf(html, "<form action=/x method=GET>"
                  "<input id=b5 type=date name=b5 value=2020-00-01 required>"
                  "<input type=submit></form>");
    l76m_render(html);
    TEST_ASSERT_EQUAL(WE_FACT_CHANGED, l76m_submit());
    l76m_done();
}

static void test_l76m_baddate_6(void) {
    char html[400];
    sprintf(html, "<form action=/x method=GET>"
                  "<input id=b6 type=date name=b6 value=2020-01-32 required>"
                  "<input type=submit></form>");
    l76m_render(html);
    TEST_ASSERT_EQUAL(WE_FACT_CHANGED, l76m_submit());
    l76m_done();
}

static void test_l76m_baddate_7(void) {
    char html[400];
    sprintf(html, "<form action=/x method=GET>"
                  "<input id=b7 type=date name=b7 value=#ff0000 required>"
                  "<input type=submit></form>");
    l76m_render(html);
    TEST_ASSERT_EQUAL(WE_FACT_CHANGED, l76m_submit());
    l76m_done();
}

static void test_l76m_baddate_8(void) {
    char html[400];
    sprintf(html, "<form action=/x method=GET>"
                  "<input id=b8 type=date name=b8 value=not-a-date required>"
                  "<input type=submit></form>");
    l76m_render(html);
    TEST_ASSERT_EQUAL(WE_FACT_CHANGED, l76m_submit());
    l76m_done();
}

static void test_l76m_baddate_9(void) {
    char html[400];
    sprintf(html, "<form action=/x method=GET>"
                  "<input id=b9 type=date name=b9 value=2020/01/01 required>"
                  "<input type=submit></form>");
    l76m_render(html);
    TEST_ASSERT_EQUAL(WE_FACT_CHANGED, l76m_submit());
    l76m_done();
}

static void test_l76m_baddate_10(void) {
    char html[400];
    sprintf(html, "<form action=/x method=GET>"
                  "<input id=b10 type=date name=b10 value=20-01-01 required>"
                  "<input type=submit></form>");
    l76m_render(html);
    TEST_ASSERT_EQUAL(WE_FACT_CHANGED, l76m_submit());
    l76m_done();
}

static void test_l76m_baddate_11(void) {
    char html[400];
    sprintf(html, "<form action=/x method=GET>"
                  "<input id=b11 type=date name=b11 value=2020-1-1 required>"
                  "<input type=submit></form>");
    l76m_render(html);
    TEST_ASSERT_EQUAL(WE_FACT_CHANGED, l76m_submit());
    l76m_done();
}

static void test_l76m_baddate_12(void) {
    char html[400];
    sprintf(html, "<form action=/x method=GET>"
                  "<input id=b12 type=date name=b12 value=20200201 required>"
                  "<input type=submit></form>");
    l76m_render(html);
    TEST_ASSERT_EQUAL(WE_FACT_CHANGED, l76m_submit());
    l76m_done();
}

static void test_l76m_baddate_13(void) {
    char html[400];
    sprintf(html, "<form action=/x method=GET>"
                  "<input id=b13 type=date name=b13 value=2020-04-31 required>"
                  "<input type=submit></form>");
    l76m_render(html);
    TEST_ASSERT_EQUAL(WE_FACT_CHANGED, l76m_submit());
    l76m_done();
}

static void test_l76m_baddate_14(void) {
    char html[400];
    sprintf(html, "<form action=/x method=GET>"
                  "<input id=b14 type=date name=b14 value=2020-06-31 required>"
                  "<input type=submit></form>");
    l76m_render(html);
    TEST_ASSERT_EQUAL(WE_FACT_CHANGED, l76m_submit());
    l76m_done();
}

static void test_l76m_baddate_15(void) {
    char html[400];
    sprintf(html, "<form action=/x method=GET>"
                  "<input id=b15 type=date name=b15 value=2020-09-31 required>"
                  "<input type=submit></form>");
    l76m_render(html);
    TEST_ASSERT_EQUAL(WE_FACT_CHANGED, l76m_submit());
    l76m_done();
}

static void test_l76m_baddate_16(void) {
    char html[400];
    sprintf(html, "<form action=/x method=GET>"
                  "<input id=b16 type=date name=b16 value=2020-11-31 required>"
                  "<input type=submit></form>");
    l76m_render(html);
    TEST_ASSERT_EQUAL(WE_FACT_CHANGED, l76m_submit());
    l76m_done();
}

static void test_l76m_baddate_17(void) {
    char html[400];
    sprintf(html, "<form action=/x method=GET>"
                  "<input id=b17 type=date name=b17 value=0000-01-01 required>"
                  "<input type=submit></form>");
    l76m_render(html);
    TEST_ASSERT_EQUAL(WE_FACT_CHANGED, l76m_submit());
    l76m_done();
}

static void test_l76m_baddate_18(void) {
    char html[400];
    sprintf(html, "<form action=/x method=GET>"
                  "<input id=b18 type=date name=b18 value=2020-02-00 required>"
                  "<input type=submit></form>");
    l76m_render(html);
    TEST_ASSERT_EQUAL(WE_FACT_CHANGED, l76m_submit());
    l76m_done();
}

static void test_l76m_badcolor_1(void) {
    char html[400];
    sprintf(html, "<form action=/x method=GET>"
                  "<input id=k1 type=color name=k1 value='ff0000' required>"
                  "<input type=submit></form>");
    l76m_render(html);
    TEST_ASSERT_EQUAL(WE_FACT_CHANGED, l76m_submit());
    l76m_done();
}

static void test_l76m_badcolor_2(void) {
    char html[400];
    sprintf(html, "<form action=/x method=GET>"
                  "<input id=k2 type=color name=k2 value='#ff00' required>"
                  "<input type=submit></form>");
    l76m_render(html);
    TEST_ASSERT_EQUAL(WE_FACT_CHANGED, l76m_submit());
    l76m_done();
}

static void test_l76m_badcolor_3(void) {
    char html[400];
    sprintf(html, "<form action=/x method=GET>"
                  "<input id=k3 type=color name=k3 value='#gg0000' required>"
                  "<input type=submit></form>");
    l76m_render(html);
    TEST_ASSERT_EQUAL(WE_FACT_CHANGED, l76m_submit());
    l76m_done();
}

static void test_l76m_badcolor_4(void) {
    char html[400];
    sprintf(html, "<form action=/x method=GET>"
                  "<input id=k4 type=color name=k4 value='#12345' required>"
                  "<input type=submit></form>");
    l76m_render(html);
    TEST_ASSERT_EQUAL(WE_FACT_CHANGED, l76m_submit());
    l76m_done();
}

static void test_l76m_badcolor_5(void) {
    char html[400];
    sprintf(html, "<form action=/x method=GET>"
                  "<input id=k5 type=color name=k5 value='#1234567' required>"
                  "<input type=submit></form>");
    l76m_render(html);
    TEST_ASSERT_EQUAL(WE_FACT_CHANGED, l76m_submit());
    l76m_done();
}

static void test_l76m_badcolor_6(void) {
    char html[400];
    sprintf(html, "<form action=/x method=GET>"
                  "<input id=k6 type=color name=k6 value='red' required>"
                  "<input type=submit></form>");
    l76m_render(html);
    TEST_ASSERT_EQUAL(WE_FACT_CHANGED, l76m_submit());
    l76m_done();
}

static void test_l76m_badcolor_7(void) {
    char html[400];
    sprintf(html, "<form action=/x method=GET>"
                  "<input id=k7 type=color name=k7 value='#FFFFFG' required>"
                  "<input type=submit></form>");
    l76m_render(html);
    TEST_ASSERT_EQUAL(WE_FACT_CHANGED, l76m_submit());
    l76m_done();
}

static void test_l76m_badcolor_8(void) {
    char html[400];
    sprintf(html, "<form action=/x method=GET>"
                  "<input id=k8 type=color name=k8 value='123456' required>"
                  "<input type=submit></form>");
    l76m_render(html);
    TEST_ASSERT_EQUAL(WE_FACT_CHANGED, l76m_submit());
    l76m_done();
}

static void test_l76m_badcolor_9(void) {
    char html[400];
    sprintf(html, "<form action=/x method=GET>"
                  "<input id=k9 type=color name=k9 value='#12 345' required>"
                  "<input type=submit></form>");
    l76m_render(html);
    TEST_ASSERT_EQUAL(WE_FACT_CHANGED, l76m_submit());
    l76m_done();
}

static void test_l76m_badcolor_10(void) {
    char html[400];
    sprintf(html, "<form action=/x method=GET>"
                  "<input id=k10 type=color name=k10 value='##00000' required>"
                  "<input type=submit></form>");
    l76m_render(html);
    TEST_ASSERT_EQUAL(WE_FACT_CHANGED, l76m_submit());
    l76m_done();
}

static void test_l76m_badcolor_11(void) {
    char html[400];
    sprintf(html, "<form action=/x method=GET>"
                  "<input id=k11 type=color name=k11 value='#00000g' required>"
                  "<input type=submit></form>");
    l76m_render(html);
    TEST_ASSERT_EQUAL(WE_FACT_CHANGED, l76m_submit());
    l76m_done();
}

static void test_l76m_badcolor_12(void) {
    char html[400];
    sprintf(html, "<form action=/x method=GET>"
                  "<input id=k12 type=color name=k12 value='notcolor' required>"
                  "<input type=submit></form>");
    l76m_render(html);
    TEST_ASSERT_EQUAL(WE_FACT_CHANGED, l76m_submit());
    l76m_done();
}

static void test_l76m_badcolor_13(void) {
    char html[400];
    sprintf(html, "<form action=/x method=GET>"
                  "<input id=k13 type=color name=k13 value='#' required>"
                  "<input type=submit></form>");
    l76m_render(html);
    TEST_ASSERT_EQUAL(WE_FACT_CHANGED, l76m_submit());
    l76m_done();
}

static void test_l76m_badcolor_14(void) {
    char html[400];
    sprintf(html, "<form action=/x method=GET>"
                  "<input id=k14 type=color name=k14 value='#0' required>"
                  "<input type=submit></form>");
    l76m_render(html);
    TEST_ASSERT_EQUAL(WE_FACT_CHANGED, l76m_submit());
    l76m_done();
}

static void test_l76m_badcolor_15(void) {
    char html[400];
    sprintf(html, "<form action=/x method=GET>"
                  "<input id=k15 type=color name=k15 value='#00' required>"
                  "<input type=submit></form>");
    l76m_render(html);
    TEST_ASSERT_EQUAL(WE_FACT_CHANGED, l76m_submit());
    l76m_done();
}

static void test_l76m_badcolor_16(void) {
    char html[400];
    sprintf(html, "<form action=/x method=GET>"
                  "<input id=k16 type=color name=k16 value='#000' required>"
                  "<input type=submit></form>");
    l76m_render(html);
    TEST_ASSERT_EQUAL(WE_FACT_CHANGED, l76m_submit());
    l76m_done();
}

static void test_l76m_badcolor_17(void) {
    char html[400];
    sprintf(html, "<form action=/x method=GET>"
                  "<input id=k17 type=color name=k17 value='#0000' required>"
                  "<input type=submit></form>");
    l76m_render(html);
    TEST_ASSERT_EQUAL(WE_FACT_CHANGED, l76m_submit());
    l76m_done();
}

static void test_l76m_badcolor_18(void) {
    char html[400];
    sprintf(html, "<form action=/x method=GET>"
                  "<input id=k18 type=color name=k18 value='#00000' required>"
                  "<input type=submit></form>");
    l76m_render(html);
    TEST_ASSERT_EQUAL(WE_FACT_CHANGED, l76m_submit());
    l76m_done();
}

static void test_l76m_ac_1(void) {
    char html[300];
    sprintf(html, "<input id=a1 name=a1 autocomplete=off>");
    l76m_render(html);
    TEST_ASSERT_TRUE(g_doc.nodes[l76m_node("a1")].fac_len > 0);
    TEST_ASSERT_EQUAL(1, !!(g_doc.nodes[l76m_node("a1")].form_flags2 & WE_FF2_AC_OFF));
    l76m_done();
}

static void test_l76m_ac_2(void) {
    char html[300];
    sprintf(html, "<input id=a2 name=a2 autocomplete=on>");
    l76m_render(html);
    TEST_ASSERT_TRUE(g_doc.nodes[l76m_node("a2")].fac_len > 0);
    TEST_ASSERT_EQUAL(0, !!(g_doc.nodes[l76m_node("a2")].form_flags2 & WE_FF2_AC_OFF));
    l76m_done();
}

static void test_l76m_ac_3(void) {
    char html[300];
    sprintf(html, "<input id=a3 name=a3 autocomplete=name>");
    l76m_render(html);
    TEST_ASSERT_TRUE(g_doc.nodes[l76m_node("a3")].fac_len > 0);
    TEST_ASSERT_EQUAL(0, !!(g_doc.nodes[l76m_node("a3")].form_flags2 & WE_FF2_AC_OFF));
    l76m_done();
}

static void test_l76m_ac_4(void) {
    char html[300];
    sprintf(html, "<input id=a4 name=a4 autocomplete=email>");
    l76m_render(html);
    TEST_ASSERT_TRUE(g_doc.nodes[l76m_node("a4")].fac_len > 0);
    TEST_ASSERT_EQUAL(0, !!(g_doc.nodes[l76m_node("a4")].form_flags2 & WE_FF2_AC_OFF));
    l76m_done();
}

static void test_l76m_ac_5(void) {
    char html[300];
    sprintf(html, "<input id=a5 name=a5 autocomplete=username>");
    l76m_render(html);
    TEST_ASSERT_TRUE(g_doc.nodes[l76m_node("a5")].fac_len > 0);
    TEST_ASSERT_EQUAL(0, !!(g_doc.nodes[l76m_node("a5")].form_flags2 & WE_FF2_AC_OFF));
    l76m_done();
}

static void test_l76m_ac_6(void) {
    char html[300];
    sprintf(html, "<input id=a6 name=a6 autocomplete=new-password>");
    l76m_render(html);
    TEST_ASSERT_TRUE(g_doc.nodes[l76m_node("a6")].fac_len > 0);
    TEST_ASSERT_EQUAL(0, !!(g_doc.nodes[l76m_node("a6")].form_flags2 & WE_FF2_AC_OFF));
    l76m_done();
}

static void test_l76m_ac_7(void) {
    char html[300];
    sprintf(html, "<input id=a7 name=a7 autocomplete=current-password>");
    l76m_render(html);
    TEST_ASSERT_TRUE(g_doc.nodes[l76m_node("a7")].fac_len > 0);
    TEST_ASSERT_EQUAL(0, !!(g_doc.nodes[l76m_node("a7")].form_flags2 & WE_FF2_AC_OFF));
    l76m_done();
}

static void test_l76m_ac_8(void) {
    char html[300];
    sprintf(html, "<input id=a8 name=a8 autocomplete=organization>");
    l76m_render(html);
    TEST_ASSERT_TRUE(g_doc.nodes[l76m_node("a8")].fac_len > 0);
    TEST_ASSERT_EQUAL(0, !!(g_doc.nodes[l76m_node("a8")].form_flags2 & WE_FF2_AC_OFF));
    l76m_done();
}

static void test_l76m_ac_9(void) {
    char html[300];
    sprintf(html, "<input id=a9 name=a9 autocomplete=street-address>");
    l76m_render(html);
    TEST_ASSERT_TRUE(g_doc.nodes[l76m_node("a9")].fac_len > 0);
    TEST_ASSERT_EQUAL(0, !!(g_doc.nodes[l76m_node("a9")].form_flags2 & WE_FF2_AC_OFF));
    l76m_done();
}

static void test_l76m_ac_10(void) {
    char html[300];
    sprintf(html, "<input id=a10 name=a10 autocomplete=country>");
    l76m_render(html);
    TEST_ASSERT_TRUE(g_doc.nodes[l76m_node("a10")].fac_len > 0);
    TEST_ASSERT_EQUAL(0, !!(g_doc.nodes[l76m_node("a10")].form_flags2 & WE_FF2_AC_OFF));
    l76m_done();
}

static void test_l76m_ac_11(void) {
    char html[300];
    sprintf(html, "<input id=a11 name=a11 autocomplete=postal-code>");
    l76m_render(html);
    TEST_ASSERT_TRUE(g_doc.nodes[l76m_node("a11")].fac_len > 0);
    TEST_ASSERT_EQUAL(0, !!(g_doc.nodes[l76m_node("a11")].form_flags2 & WE_FF2_AC_OFF));
    l76m_done();
}

static void test_l76m_ac_12(void) {
    char html[300];
    sprintf(html, "<input id=a12 name=a12 autocomplete=tel>");
    l76m_render(html);
    TEST_ASSERT_TRUE(g_doc.nodes[l76m_node("a12")].fac_len > 0);
    TEST_ASSERT_EQUAL(0, !!(g_doc.nodes[l76m_node("a12")].form_flags2 & WE_FF2_AC_OFF));
    l76m_done();
}

static void test_l76m_ac_13(void) {
    char html[300];
    sprintf(html, "<input id=a13 name=a13 autocomplete=url>");
    l76m_render(html);
    TEST_ASSERT_TRUE(g_doc.nodes[l76m_node("a13")].fac_len > 0);
    TEST_ASSERT_EQUAL(0, !!(g_doc.nodes[l76m_node("a13")].form_flags2 & WE_FF2_AC_OFF));
    l76m_done();
}

static void test_l76m_ac_14(void) {
    char html[300];
    sprintf(html, "<input id=a14 name=a14 autocomplete=one-time-code>");
    l76m_render(html);
    TEST_ASSERT_TRUE(g_doc.nodes[l76m_node("a14")].fac_len > 0);
    TEST_ASSERT_EQUAL(0, !!(g_doc.nodes[l76m_node("a14")].form_flags2 & WE_FF2_AC_OFF));
    l76m_done();
}

static void test_l76m_ac_15(void) {
    char html[300];
    sprintf(html, "<input id=a15 name=a15 autocomplete=cc-name>");
    l76m_render(html);
    TEST_ASSERT_TRUE(g_doc.nodes[l76m_node("a15")].fac_len > 0);
    TEST_ASSERT_EQUAL(0, !!(g_doc.nodes[l76m_node("a15")].form_flags2 & WE_FF2_AC_OFF));
    l76m_done();
}

static void test_l76m_ac_16(void) {
    char html[300];
    sprintf(html, "<input id=a16 name=a16 autocomplete=cc-number>");
    l76m_render(html);
    TEST_ASSERT_TRUE(g_doc.nodes[l76m_node("a16")].fac_len > 0);
    TEST_ASSERT_EQUAL(0, !!(g_doc.nodes[l76m_node("a16")].form_flags2 & WE_FF2_AC_OFF));
    l76m_done();
}

static void test_l76m_ac_17(void) {
    char html[300];
    sprintf(html, "<input id=a17 name=a17 autocomplete=cc-csc>");
    l76m_render(html);
    TEST_ASSERT_TRUE(g_doc.nodes[l76m_node("a17")].fac_len > 0);
    TEST_ASSERT_EQUAL(0, !!(g_doc.nodes[l76m_node("a17")].form_flags2 & WE_FF2_AC_OFF));
    l76m_done();
}

static void test_l76m_ac_18(void) {
    char html[300];
    sprintf(html, "<input id=a18 name=a18 autocomplete=bday>");
    l76m_render(html);
    TEST_ASSERT_TRUE(g_doc.nodes[l76m_node("a18")].fac_len > 0);
    TEST_ASSERT_EQUAL(0, !!(g_doc.nodes[l76m_node("a18")].form_flags2 & WE_FF2_AC_OFF));
    l76m_done();
}

static void test_l76m_ac_19(void) {
    char html[300];
    sprintf(html, "<input id=a19 name=a19 autocomplete=sex>");
    l76m_render(html);
    TEST_ASSERT_TRUE(g_doc.nodes[l76m_node("a19")].fac_len > 0);
    TEST_ASSERT_EQUAL(0, !!(g_doc.nodes[l76m_node("a19")].form_flags2 & WE_FF2_AC_OFF));
    l76m_done();
}

static void test_l76m_ac_20(void) {
    char html[300];
    sprintf(html, "<input id=a20 name=a20 autocomplete=false>");
    l76m_render(html);
    TEST_ASSERT_TRUE(g_doc.nodes[l76m_node("a20")].fac_len > 0);
    TEST_ASSERT_EQUAL(1, !!(g_doc.nodes[l76m_node("a20")].form_flags2 & WE_FF2_AC_OFF));
    l76m_done();
}

static void test_l76m_ac_21(void) {
    char html[300];
    sprintf(html, "<input id=a21 name=a21 autocomplete=OFF>");
    l76m_render(html);
    TEST_ASSERT_TRUE(g_doc.nodes[l76m_node("a21")].fac_len > 0);
    TEST_ASSERT_EQUAL(1, !!(g_doc.nodes[l76m_node("a21")].form_flags2 & WE_FF2_AC_OFF));
    l76m_done();
}

static void test_l76m_ac_22(void) {
    char html[300];
    sprintf(html, "<input id=a22 name=a22 autocomplete=On>");
    l76m_render(html);
    TEST_ASSERT_TRUE(g_doc.nodes[l76m_node("a22")].fac_len > 0);
    TEST_ASSERT_EQUAL(0, !!(g_doc.nodes[l76m_node("a22")].form_flags2 & WE_FF2_AC_OFF));
    l76m_done();
}

static void test_l76m_ft_1(void) {
    we_form_req_t q;
    char html[400];
    sprintf(html, "<form action=/t1 method=GET>"
                  "<input name=n value=1>"
                  "<input id=s1 type=submit formtarget=_blank value=Go></form>");
    l76m_render(html);
    TEST_ASSERT_TRUE(g_doc.nodes[l76m_node("s1")].form_flags2 & WE_FF2_FORMTARGET);
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76m_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_EQUAL(1, q.target_blank);
    TEST_ASSERT_EQUAL_STRING("_blank", q.target);
    l76m_done();
}

static void test_l76m_ft_2(void) {
    we_form_req_t q;
    char html[400];
    sprintf(html, "<form action=/t2 method=GET>"
                  "<input name=n value=2>"
                  "<input id=s2 type=submit formtarget=_self value=Go></form>");
    l76m_render(html);
    TEST_ASSERT_TRUE(g_doc.nodes[l76m_node("s2")].form_flags2 & WE_FF2_FORMTARGET);
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76m_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_EQUAL(0, q.target_blank);
    TEST_ASSERT_EQUAL_STRING("_self", q.target);
    l76m_done();
}

static void test_l76m_ft_3(void) {
    we_form_req_t q;
    char html[400];
    sprintf(html, "<form action=/t3 method=GET>"
                  "<input name=n value=3>"
                  "<input id=s3 type=submit formtarget=_parent value=Go></form>");
    l76m_render(html);
    TEST_ASSERT_TRUE(g_doc.nodes[l76m_node("s3")].form_flags2 & WE_FF2_FORMTARGET);
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76m_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_EQUAL(0, q.target_blank);
    TEST_ASSERT_EQUAL_STRING("_parent", q.target);
    l76m_done();
}

static void test_l76m_ft_4(void) {
    we_form_req_t q;
    char html[400];
    sprintf(html, "<form action=/t4 method=GET>"
                  "<input name=n value=4>"
                  "<input id=s4 type=submit formtarget=_top value=Go></form>");
    l76m_render(html);
    TEST_ASSERT_TRUE(g_doc.nodes[l76m_node("s4")].form_flags2 & WE_FF2_FORMTARGET);
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76m_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_EQUAL(0, q.target_blank);
    TEST_ASSERT_EQUAL_STRING("_top", q.target);
    l76m_done();
}

static void test_l76m_ft_5(void) {
    we_form_req_t q;
    char html[400];
    sprintf(html, "<form action=/t5 method=GET>"
                  "<input name=n value=5>"
                  "<input id=s5 type=submit formtarget=w1 value=Go></form>");
    l76m_render(html);
    TEST_ASSERT_TRUE(g_doc.nodes[l76m_node("s5")].form_flags2 & WE_FF2_FORMTARGET);
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76m_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_EQUAL(0, q.target_blank);
    TEST_ASSERT_EQUAL_STRING("w1", q.target);
    l76m_done();
}

static void test_l76m_ft_6(void) {
    we_form_req_t q;
    char html[400];
    sprintf(html, "<form action=/t6 method=GET>"
                  "<input name=n value=6>"
                  "<input id=s6 type=submit formtarget=w2 value=Go></form>");
    l76m_render(html);
    TEST_ASSERT_TRUE(g_doc.nodes[l76m_node("s6")].form_flags2 & WE_FF2_FORMTARGET);
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76m_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_EQUAL(0, q.target_blank);
    TEST_ASSERT_EQUAL_STRING("w2", q.target);
    l76m_done();
}

static void test_l76m_ft_7(void) {
    we_form_req_t q;
    char html[400];
    sprintf(html, "<form action=/t7 method=GET>"
                  "<input name=n value=7>"
                  "<input id=s7 type=submit formtarget=frameA value=Go></form>");
    l76m_render(html);
    TEST_ASSERT_TRUE(g_doc.nodes[l76m_node("s7")].form_flags2 & WE_FF2_FORMTARGET);
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76m_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_EQUAL(0, q.target_blank);
    TEST_ASSERT_EQUAL_STRING("frameA", q.target);
    l76m_done();
}

static void test_l76m_ft_8(void) {
    we_form_req_t q;
    char html[400];
    sprintf(html, "<form action=/t8 method=GET>"
                  "<input name=n value=8>"
                  "<input id=s8 type=submit formtarget=main value=Go></form>");
    l76m_render(html);
    TEST_ASSERT_TRUE(g_doc.nodes[l76m_node("s8")].form_flags2 & WE_FF2_FORMTARGET);
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76m_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_EQUAL(0, q.target_blank);
    TEST_ASSERT_EQUAL_STRING("main", q.target);
    l76m_done();
}

static void test_l76m_ft_9(void) {
    we_form_req_t q;
    char html[400];
    sprintf(html, "<form action=/t9 method=GET>"
                  "<input name=n value=9>"
                  "<input id=s9 type=submit formtarget=sidebar value=Go></form>");
    l76m_render(html);
    TEST_ASSERT_TRUE(g_doc.nodes[l76m_node("s9")].form_flags2 & WE_FF2_FORMTARGET);
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76m_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_EQUAL(0, q.target_blank);
    TEST_ASSERT_EQUAL_STRING("sidebar", q.target);
    l76m_done();
}

static void test_l76m_ft_10(void) {
    we_form_req_t q;
    char html[400];
    sprintf(html, "<form action=/t10 method=GET>"
                  "<input name=n value=10>"
                  "<input id=s10 type=submit formtarget=popup value=Go></form>");
    l76m_render(html);
    TEST_ASSERT_TRUE(g_doc.nodes[l76m_node("s10")].form_flags2 & WE_FF2_FORMTARGET);
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76m_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_EQUAL(0, q.target_blank);
    TEST_ASSERT_EQUAL_STRING("popup", q.target);
    l76m_done();
}

static void test_l76m_ft_11(void) {
    we_form_req_t q;
    char html[400];
    sprintf(html, "<form action=/t11 method=GET>"
                  "<input name=n value=11>"
                  "<input id=s11 type=submit formtarget=x value=Go></form>");
    l76m_render(html);
    TEST_ASSERT_TRUE(g_doc.nodes[l76m_node("s11")].form_flags2 & WE_FF2_FORMTARGET);
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76m_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_EQUAL(0, q.target_blank);
    TEST_ASSERT_EQUAL_STRING("x", q.target);
    l76m_done();
}

static void test_l76m_ft_12(void) {
    we_form_req_t q;
    char html[400];
    sprintf(html, "<form action=/t12 method=GET>"
                  "<input name=n value=12>"
                  "<input id=s12 type=submit formtarget=y value=Go></form>");
    l76m_render(html);
    TEST_ASSERT_TRUE(g_doc.nodes[l76m_node("s12")].form_flags2 & WE_FF2_FORMTARGET);
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76m_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_EQUAL(0, q.target_blank);
    TEST_ASSERT_EQUAL_STRING("y", q.target);
    l76m_done();
}

static void test_l76m_ft_13(void) {
    we_form_req_t q;
    char html[400];
    sprintf(html, "<form action=/t13 method=GET>"
                  "<input name=n value=13>"
                  "<input id=s13 type=submit formtarget=z value=Go></form>");
    l76m_render(html);
    TEST_ASSERT_TRUE(g_doc.nodes[l76m_node("s13")].form_flags2 & WE_FF2_FORMTARGET);
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76m_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_EQUAL(0, q.target_blank);
    TEST_ASSERT_EQUAL_STRING("z", q.target);
    l76m_done();
}

static void test_l76m_ft_14(void) {
    we_form_req_t q;
    char html[400];
    sprintf(html, "<form action=/t14 method=GET>"
                  "<input name=n value=14>"
                  "<input id=s14 type=submit formtarget=app value=Go></form>");
    l76m_render(html);
    TEST_ASSERT_TRUE(g_doc.nodes[l76m_node("s14")].form_flags2 & WE_FF2_FORMTARGET);
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76m_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_EQUAL(0, q.target_blank);
    TEST_ASSERT_EQUAL_STRING("app", q.target);
    l76m_done();
}

static void test_l76m_ft_15(void) {
    we_form_req_t q;
    char html[400];
    sprintf(html, "<form action=/t15 method=GET>"
                  "<input name=n value=15>"
                  "<input id=s15 type=submit formtarget=view value=Go></form>");
    l76m_render(html);
    TEST_ASSERT_TRUE(g_doc.nodes[l76m_node("s15")].form_flags2 & WE_FF2_FORMTARGET);
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76m_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_EQUAL(0, q.target_blank);
    TEST_ASSERT_EQUAL_STRING("view", q.target);
    l76m_done();
}

static void test_l76m_out_1(void) {
    we_form_req_t q;
    char html[400];
    sprintf(html, "<form action=/o1 method=GET>"
                  "<output id=o1 name=o1 value=V1>V1</output>"
                  "<input type=submit></form>");
    l76m_render(html);
    TEST_ASSERT_TRUE(g_doc.nodes[l76m_node("o1")].form_ord > 0);
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76m_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "o1=V1"));
    l76m_done();
}

static void test_l76m_out_2(void) {
    we_form_req_t q;
    char html[400];
    sprintf(html, "<form action=/o2 method=GET>"
                  "<output id=o2 name=o2 value=V2>V2</output>"
                  "<input type=submit></form>");
    l76m_render(html);
    TEST_ASSERT_TRUE(g_doc.nodes[l76m_node("o2")].form_ord > 0);
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76m_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "o2=V2"));
    l76m_done();
}

static void test_l76m_out_3(void) {
    we_form_req_t q;
    char html[400];
    sprintf(html, "<form action=/o3 method=GET>"
                  "<output id=o3 name=o3 value=V3>V3</output>"
                  "<input type=submit></form>");
    l76m_render(html);
    TEST_ASSERT_TRUE(g_doc.nodes[l76m_node("o3")].form_ord > 0);
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76m_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "o3=V3"));
    l76m_done();
}

static void test_l76m_out_4(void) {
    we_form_req_t q;
    char html[400];
    sprintf(html, "<form action=/o4 method=GET>"
                  "<output id=o4 name=o4 value=V4>V4</output>"
                  "<input type=submit></form>");
    l76m_render(html);
    TEST_ASSERT_TRUE(g_doc.nodes[l76m_node("o4")].form_ord > 0);
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76m_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "o4=V4"));
    l76m_done();
}

static void test_l76m_out_5(void) {
    we_form_req_t q;
    char html[400];
    sprintf(html, "<form action=/o5 method=GET>"
                  "<output id=o5 name=o5 value=V5>V5</output>"
                  "<input type=submit></form>");
    l76m_render(html);
    TEST_ASSERT_TRUE(g_doc.nodes[l76m_node("o5")].form_ord > 0);
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76m_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "o5=V5"));
    l76m_done();
}

static void test_l76m_out_6(void) {
    we_form_req_t q;
    char html[400];
    sprintf(html, "<form action=/o6 method=GET>"
                  "<output id=o6 name=o6 value=V6>V6</output>"
                  "<input type=submit></form>");
    l76m_render(html);
    TEST_ASSERT_TRUE(g_doc.nodes[l76m_node("o6")].form_ord > 0);
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76m_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "o6=V6"));
    l76m_done();
}

static void test_l76m_out_7(void) {
    we_form_req_t q;
    char html[400];
    sprintf(html, "<form action=/o7 method=GET>"
                  "<output id=o7 name=o7 value=V7>V7</output>"
                  "<input type=submit></form>");
    l76m_render(html);
    TEST_ASSERT_TRUE(g_doc.nodes[l76m_node("o7")].form_ord > 0);
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76m_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "o7=V7"));
    l76m_done();
}

static void test_l76m_out_8(void) {
    we_form_req_t q;
    char html[400];
    sprintf(html, "<form action=/o8 method=GET>"
                  "<output id=o8 name=o8 value=V8>V8</output>"
                  "<input type=submit></form>");
    l76m_render(html);
    TEST_ASSERT_TRUE(g_doc.nodes[l76m_node("o8")].form_ord > 0);
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76m_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "o8=V8"));
    l76m_done();
}

static void test_l76m_out_9(void) {
    we_form_req_t q;
    char html[400];
    sprintf(html, "<form action=/o9 method=GET>"
                  "<output id=o9 name=o9 value=V9>V9</output>"
                  "<input type=submit></form>");
    l76m_render(html);
    TEST_ASSERT_TRUE(g_doc.nodes[l76m_node("o9")].form_ord > 0);
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76m_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "o9=V9"));
    l76m_done();
}

static void test_l76m_out_10(void) {
    we_form_req_t q;
    char html[400];
    sprintf(html, "<form action=/o10 method=GET>"
                  "<output id=o10 name=o10 value=V10>V10</output>"
                  "<input type=submit></form>");
    l76m_render(html);
    TEST_ASSERT_TRUE(g_doc.nodes[l76m_node("o10")].form_ord > 0);
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76m_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "o10=V10"));
    l76m_done();
}

static void test_l76m_out_11(void) {
    we_form_req_t q;
    char html[400];
    sprintf(html, "<form action=/o11 method=GET>"
                  "<output id=o11 name=o11 value=V11>V11</output>"
                  "<input type=submit></form>");
    l76m_render(html);
    TEST_ASSERT_TRUE(g_doc.nodes[l76m_node("o11")].form_ord > 0);
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76m_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "o11=V11"));
    l76m_done();
}

static void test_l76m_out_12(void) {
    we_form_req_t q;
    char html[400];
    sprintf(html, "<form action=/o12 method=GET>"
                  "<output id=o12 name=o12 value=V12>V12</output>"
                  "<input type=submit></form>");
    l76m_render(html);
    TEST_ASSERT_TRUE(g_doc.nodes[l76m_node("o12")].form_ord > 0);
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76m_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "o12=V12"));
    l76m_done();
}

static void test_l76m_out_13(void) {
    we_form_req_t q;
    char html[400];
    sprintf(html, "<form action=/o13 method=GET>"
                  "<output id=o13 name=o13 value=V13>V13</output>"
                  "<input type=submit></form>");
    l76m_render(html);
    TEST_ASSERT_TRUE(g_doc.nodes[l76m_node("o13")].form_ord > 0);
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76m_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "o13=V13"));
    l76m_done();
}

static void test_l76m_out_14(void) {
    we_form_req_t q;
    char html[400];
    sprintf(html, "<form action=/o14 method=GET>"
                  "<output id=o14 name=o14 value=V14>V14</output>"
                  "<input type=submit></form>");
    l76m_render(html);
    TEST_ASSERT_TRUE(g_doc.nodes[l76m_node("o14")].form_ord > 0);
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76m_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "o14=V14"));
    l76m_done();
}

static void test_l76m_out_15(void) {
    we_form_req_t q;
    char html[400];
    sprintf(html, "<form action=/o15 method=GET>"
                  "<output id=o15 name=o15 value=V15>V15</output>"
                  "<input type=submit></form>");
    l76m_render(html);
    TEST_ASSERT_TRUE(g_doc.nodes[l76m_node("o15")].form_ord > 0);
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76m_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "o15=V15"));
    l76m_done();
}

static void test_l76m_out_16(void) {
    we_form_req_t q;
    char html[400];
    sprintf(html, "<form action=/o16 method=GET>"
                  "<output id=o16 name=o16 value=V16>V16</output>"
                  "<input type=submit></form>");
    l76m_render(html);
    TEST_ASSERT_TRUE(g_doc.nodes[l76m_node("o16")].form_ord > 0);
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76m_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "o16=V16"));
    l76m_done();
}

static void test_l76m_out_17(void) {
    we_form_req_t q;
    char html[400];
    sprintf(html, "<form action=/o17 method=GET>"
                  "<output id=o17 name=o17 value=V17>V17</output>"
                  "<input type=submit></form>");
    l76m_render(html);
    TEST_ASSERT_TRUE(g_doc.nodes[l76m_node("o17")].form_ord > 0);
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76m_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "o17=V17"));
    l76m_done();
}

static void test_l76m_out_18(void) {
    we_form_req_t q;
    char html[400];
    sprintf(html, "<form action=/o18 method=GET>"
                  "<output id=o18 name=o18 value=V18>V18</output>"
                  "<input type=submit></form>");
    l76m_render(html);
    TEST_ASSERT_TRUE(g_doc.nodes[l76m_node("o18")].form_ord > 0);
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76m_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "o18=V18"));
    l76m_done();
}

static void test_l76m_out_19(void) {
    we_form_req_t q;
    char html[400];
    sprintf(html, "<form action=/o19 method=GET>"
                  "<output id=o19 name=o19 value=V19>V19</output>"
                  "<input type=submit></form>");
    l76m_render(html);
    TEST_ASSERT_TRUE(g_doc.nodes[l76m_node("o19")].form_ord > 0);
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76m_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "o19=V19"));
    l76m_done();
}

static void test_l76m_out_20(void) {
    we_form_req_t q;
    char html[400];
    sprintf(html, "<form action=/o20 method=GET>"
                  "<output id=o20 name=o20 value=V20>V20</output>"
                  "<input type=submit></form>");
    l76m_render(html);
    TEST_ASSERT_TRUE(g_doc.nodes[l76m_node("o20")].form_ord > 0);
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76m_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "o20=V20"));
    l76m_done();
}











static void test_l76m_title_1(void) {
    char html[500];
    sprintf(html, "<style>span#t1::after{content:attr(title)}</style>"
                  "<span id=t1 title=T1>x</span>");
    l76m_render(html);
    TEST_ASSERT_TRUE(g_doc.nodes[l76m_node("t1")].title_len > 0);
    TEST_ASSERT_NOT_NULL(find_text("T1"));
    l76m_done();
}

static void test_l76m_title_2(void) {
    char html[500];
    sprintf(html, "<style>span#t2::after{content:attr(title)}</style>"
                  "<span id=t2 title=T2>x</span>");
    l76m_render(html);
    TEST_ASSERT_TRUE(g_doc.nodes[l76m_node("t2")].title_len > 0);
    TEST_ASSERT_NOT_NULL(find_text("T2"));
    l76m_done();
}

static void test_l76m_title_3(void) {
    char html[500];
    sprintf(html, "<style>span#t3::after{content:attr(title)}</style>"
                  "<span id=t3 title=T3>x</span>");
    l76m_render(html);
    TEST_ASSERT_TRUE(g_doc.nodes[l76m_node("t3")].title_len > 0);
    TEST_ASSERT_NOT_NULL(find_text("T3"));
    l76m_done();
}

static void test_l76m_title_4(void) {
    char html[500];
    sprintf(html, "<style>span#t4::after{content:attr(title)}</style>"
                  "<span id=t4 title=T4>x</span>");
    l76m_render(html);
    TEST_ASSERT_TRUE(g_doc.nodes[l76m_node("t4")].title_len > 0);
    TEST_ASSERT_NOT_NULL(find_text("T4"));
    l76m_done();
}

static void test_l76m_title_5(void) {
    char html[500];
    sprintf(html, "<style>span#t5::after{content:attr(title)}</style>"
                  "<span id=t5 title=T5>x</span>");
    l76m_render(html);
    TEST_ASSERT_TRUE(g_doc.nodes[l76m_node("t5")].title_len > 0);
    TEST_ASSERT_NOT_NULL(find_text("T5"));
    l76m_done();
}

static void test_l76m_title_6(void) {
    char html[500];
    sprintf(html, "<style>span#t6::after{content:attr(title)}</style>"
                  "<span id=t6 title=T6>x</span>");
    l76m_render(html);
    TEST_ASSERT_TRUE(g_doc.nodes[l76m_node("t6")].title_len > 0);
    TEST_ASSERT_NOT_NULL(find_text("T6"));
    l76m_done();
}

static void test_l76m_title_7(void) {
    char html[500];
    sprintf(html, "<style>span#t7::after{content:attr(title)}</style>"
                  "<span id=t7 title=T7>x</span>");
    l76m_render(html);
    TEST_ASSERT_TRUE(g_doc.nodes[l76m_node("t7")].title_len > 0);
    TEST_ASSERT_NOT_NULL(find_text("T7"));
    l76m_done();
}

static void test_l76m_title_8(void) {
    char html[500];
    sprintf(html, "<style>span#t8::after{content:attr(title)}</style>"
                  "<span id=t8 title=T8>x</span>");
    l76m_render(html);
    TEST_ASSERT_TRUE(g_doc.nodes[l76m_node("t8")].title_len > 0);
    TEST_ASSERT_NOT_NULL(find_text("T8"));
    l76m_done();
}

static void test_l76m_title_9(void) {
    char html[500];
    sprintf(html, "<style>span#t9::after{content:attr(title)}</style>"
                  "<span id=t9 title=T9>x</span>");
    l76m_render(html);
    TEST_ASSERT_TRUE(g_doc.nodes[l76m_node("t9")].title_len > 0);
    TEST_ASSERT_NOT_NULL(find_text("T9"));
    l76m_done();
}

static void test_l76m_title_10(void) {
    char html[500];
    sprintf(html, "<style>span#t10::after{content:attr(title)}</style>"
                  "<span id=t10 title=T10>x</span>");
    l76m_render(html);
    TEST_ASSERT_TRUE(g_doc.nodes[l76m_node("t10")].title_len > 0);
    TEST_ASSERT_NOT_NULL(find_text("T10"));
    l76m_done();
}

static void test_l76m_title_11(void) {
    char html[500];
    sprintf(html, "<style>span#t11::after{content:attr(title)}</style>"
                  "<span id=t11 title=T11>x</span>");
    l76m_render(html);
    TEST_ASSERT_TRUE(g_doc.nodes[l76m_node("t11")].title_len > 0);
    TEST_ASSERT_NOT_NULL(find_text("T11"));
    l76m_done();
}

static void test_l76m_title_12(void) {
    char html[500];
    sprintf(html, "<style>span#t12::after{content:attr(title)}</style>"
                  "<span id=t12 title=T12>x</span>");
    l76m_render(html);
    TEST_ASSERT_TRUE(g_doc.nodes[l76m_node("t12")].title_len > 0);
    TEST_ASSERT_NOT_NULL(find_text("T12"));
    l76m_done();
}

static void test_l76m_title_13(void) {
    char html[500];
    sprintf(html, "<style>span#t13::after{content:attr(title)}</style>"
                  "<span id=t13 title=T13>x</span>");
    l76m_render(html);
    TEST_ASSERT_TRUE(g_doc.nodes[l76m_node("t13")].title_len > 0);
    TEST_ASSERT_NOT_NULL(find_text("T13"));
    l76m_done();
}

static void test_l76m_title_14(void) {
    char html[500];
    sprintf(html, "<style>span#t14::after{content:attr(title)}</style>"
                  "<span id=t14 title=T14>x</span>");
    l76m_render(html);
    TEST_ASSERT_TRUE(g_doc.nodes[l76m_node("t14")].title_len > 0);
    TEST_ASSERT_NOT_NULL(find_text("T14"));
    l76m_done();
}

static void test_l76m_title_15(void) {
    char html[500];
    sprintf(html, "<style>span#t15::after{content:attr(title)}</style>"
                  "<span id=t15 title=T15>x</span>");
    l76m_render(html);
    TEST_ASSERT_TRUE(g_doc.nodes[l76m_node("t15")].title_len > 0);
    TEST_ASSERT_NOT_NULL(find_text("T15"));
    l76m_done();
}

static void test_l76m_title_16(void) {
    char html[500];
    sprintf(html, "<style>span#t16::after{content:attr(title)}</style>"
                  "<span id=t16 title=T16>x</span>");
    l76m_render(html);
    TEST_ASSERT_TRUE(g_doc.nodes[l76m_node("t16")].title_len > 0);
    TEST_ASSERT_NOT_NULL(find_text("T16"));
    l76m_done();
}

static void test_l76m_title_17(void) {
    char html[500];
    sprintf(html, "<style>span#t17::after{content:attr(title)}</style>"
                  "<span id=t17 title=T17>x</span>");
    l76m_render(html);
    TEST_ASSERT_TRUE(g_doc.nodes[l76m_node("t17")].title_len > 0);
    TEST_ASSERT_NOT_NULL(find_text("T17"));
    l76m_done();
}

static void test_l76m_title_18(void) {
    char html[500];
    sprintf(html, "<style>span#t18::after{content:attr(title)}</style>"
                  "<span id=t18 title=T18>x</span>");
    l76m_render(html);
    TEST_ASSERT_TRUE(g_doc.nodes[l76m_node("t18")].title_len > 0);
    TEST_ASSERT_NOT_NULL(find_text("T18"));
    l76m_done();
}

static void test_l76m_title_19(void) {
    char html[500];
    sprintf(html, "<style>span#t19::after{content:attr(title)}</style>"
                  "<span id=t19 title=T19>x</span>");
    l76m_render(html);
    TEST_ASSERT_TRUE(g_doc.nodes[l76m_node("t19")].title_len > 0);
    TEST_ASSERT_NOT_NULL(find_text("T19"));
    l76m_done();
}

static void test_l76m_title_20(void) {
    char html[500];
    sprintf(html, "<style>span#t20::after{content:attr(title)}</style>"
                  "<span id=t20 title=T20>x</span>");
    l76m_render(html);
    TEST_ASSERT_TRUE(g_doc.nodes[l76m_node("t20")].title_len > 0);
    TEST_ASSERT_NOT_NULL(find_text("T20"));
    l76m_done();
}





















static void lot76_more_run(void) {
    RUN_TEST(test_l76m_color_1);
    RUN_TEST(test_l76m_color_2);
    RUN_TEST(test_l76m_color_3);
    RUN_TEST(test_l76m_color_4);
    RUN_TEST(test_l76m_color_5);
    RUN_TEST(test_l76m_color_6);
    RUN_TEST(test_l76m_color_7);
    RUN_TEST(test_l76m_color_8);
    RUN_TEST(test_l76m_color_9);
    RUN_TEST(test_l76m_color_10);
    RUN_TEST(test_l76m_color_11);
    RUN_TEST(test_l76m_color_12);
    RUN_TEST(test_l76m_color_13);
    RUN_TEST(test_l76m_color_14);
    RUN_TEST(test_l76m_color_15);
    RUN_TEST(test_l76m_color_16);
    RUN_TEST(test_l76m_color_17);
    RUN_TEST(test_l76m_color_18);
    RUN_TEST(test_l76m_color_19);
    RUN_TEST(test_l76m_color_20);
    RUN_TEST(test_l76m_color_21);
    RUN_TEST(test_l76m_color_22);
    RUN_TEST(test_l76m_color_23);
    RUN_TEST(test_l76m_color_24);
    RUN_TEST(test_l76m_color_25);
    RUN_TEST(test_l76m_color_26);
    RUN_TEST(test_l76m_color_27);
    RUN_TEST(test_l76m_color_28);
    RUN_TEST(test_l76m_color_29);
    RUN_TEST(test_l76m_color_30);
    RUN_TEST(test_l76m_date_1);
    RUN_TEST(test_l76m_date_2);
    RUN_TEST(test_l76m_date_3);
    RUN_TEST(test_l76m_date_4);
    RUN_TEST(test_l76m_date_5);
    RUN_TEST(test_l76m_date_6);
    RUN_TEST(test_l76m_date_7);
    RUN_TEST(test_l76m_date_8);
    RUN_TEST(test_l76m_date_9);
    RUN_TEST(test_l76m_date_10);
    RUN_TEST(test_l76m_date_11);
    RUN_TEST(test_l76m_date_12);
    RUN_TEST(test_l76m_date_13);
    RUN_TEST(test_l76m_date_14);
    RUN_TEST(test_l76m_date_15);
    RUN_TEST(test_l76m_date_16);
    RUN_TEST(test_l76m_date_17);
    RUN_TEST(test_l76m_date_18);
    RUN_TEST(test_l76m_date_19);
    RUN_TEST(test_l76m_date_20);
    RUN_TEST(test_l76m_date_21);
    RUN_TEST(test_l76m_date_22);
    RUN_TEST(test_l76m_date_23);
    RUN_TEST(test_l76m_date_24);
    RUN_TEST(test_l76m_date_25);
    RUN_TEST(test_l76m_date_26);
    RUN_TEST(test_l76m_date_27);
    RUN_TEST(test_l76m_date_28);
    RUN_TEST(test_l76m_date_29);
    RUN_TEST(test_l76m_date_30);
    RUN_TEST(test_l76m_baddate_1);
    RUN_TEST(test_l76m_baddate_2);
    RUN_TEST(test_l76m_baddate_3);
    RUN_TEST(test_l76m_baddate_4);
    RUN_TEST(test_l76m_baddate_5);
    RUN_TEST(test_l76m_baddate_6);
    RUN_TEST(test_l76m_baddate_7);
    RUN_TEST(test_l76m_baddate_8);
    RUN_TEST(test_l76m_baddate_9);
    RUN_TEST(test_l76m_baddate_10);
    RUN_TEST(test_l76m_baddate_11);
    RUN_TEST(test_l76m_baddate_12);
    RUN_TEST(test_l76m_baddate_13);
    RUN_TEST(test_l76m_baddate_14);
    RUN_TEST(test_l76m_baddate_15);
    RUN_TEST(test_l76m_baddate_16);
    RUN_TEST(test_l76m_baddate_17);
    RUN_TEST(test_l76m_baddate_18);
    RUN_TEST(test_l76m_badcolor_1);
    RUN_TEST(test_l76m_badcolor_2);
    RUN_TEST(test_l76m_badcolor_3);
    RUN_TEST(test_l76m_badcolor_4);
    RUN_TEST(test_l76m_badcolor_5);
    RUN_TEST(test_l76m_badcolor_6);
    RUN_TEST(test_l76m_badcolor_7);
    RUN_TEST(test_l76m_badcolor_8);
    RUN_TEST(test_l76m_badcolor_9);
    RUN_TEST(test_l76m_badcolor_10);
    RUN_TEST(test_l76m_badcolor_11);
    RUN_TEST(test_l76m_badcolor_12);
    RUN_TEST(test_l76m_badcolor_13);
    RUN_TEST(test_l76m_badcolor_14);
    RUN_TEST(test_l76m_badcolor_15);
    RUN_TEST(test_l76m_badcolor_16);
    RUN_TEST(test_l76m_badcolor_17);
    RUN_TEST(test_l76m_badcolor_18);
    RUN_TEST(test_l76m_ac_1);
    RUN_TEST(test_l76m_ac_2);
    RUN_TEST(test_l76m_ac_3);
    RUN_TEST(test_l76m_ac_4);
    RUN_TEST(test_l76m_ac_5);
    RUN_TEST(test_l76m_ac_6);
    RUN_TEST(test_l76m_ac_7);
    RUN_TEST(test_l76m_ac_8);
    RUN_TEST(test_l76m_ac_9);
    RUN_TEST(test_l76m_ac_10);
    RUN_TEST(test_l76m_ac_11);
    RUN_TEST(test_l76m_ac_12);
    RUN_TEST(test_l76m_ac_13);
    RUN_TEST(test_l76m_ac_14);
    RUN_TEST(test_l76m_ac_15);
    RUN_TEST(test_l76m_ac_16);
    RUN_TEST(test_l76m_ac_17);
    RUN_TEST(test_l76m_ac_18);
    RUN_TEST(test_l76m_ac_19);
    RUN_TEST(test_l76m_ac_20);
    RUN_TEST(test_l76m_ac_21);
    RUN_TEST(test_l76m_ac_22);
    RUN_TEST(test_l76m_ft_1);
    RUN_TEST(test_l76m_ft_2);
    RUN_TEST(test_l76m_ft_3);
    RUN_TEST(test_l76m_ft_4);
    RUN_TEST(test_l76m_ft_5);
    RUN_TEST(test_l76m_ft_6);
    RUN_TEST(test_l76m_ft_7);
    RUN_TEST(test_l76m_ft_8);
    RUN_TEST(test_l76m_ft_9);
    RUN_TEST(test_l76m_ft_10);
    RUN_TEST(test_l76m_ft_11);
    RUN_TEST(test_l76m_ft_12);
    RUN_TEST(test_l76m_ft_13);
    RUN_TEST(test_l76m_ft_14);
    RUN_TEST(test_l76m_ft_15);
    RUN_TEST(test_l76m_out_1);
    RUN_TEST(test_l76m_out_2);
    RUN_TEST(test_l76m_out_3);
    RUN_TEST(test_l76m_out_4);
    RUN_TEST(test_l76m_out_5);
    RUN_TEST(test_l76m_out_6);
    RUN_TEST(test_l76m_out_7);
    RUN_TEST(test_l76m_out_8);
    RUN_TEST(test_l76m_out_9);
    RUN_TEST(test_l76m_out_10);
    RUN_TEST(test_l76m_out_11);
    RUN_TEST(test_l76m_out_12);
    RUN_TEST(test_l76m_out_13);
    RUN_TEST(test_l76m_out_14);
    RUN_TEST(test_l76m_out_15);
    RUN_TEST(test_l76m_out_16);
    RUN_TEST(test_l76m_out_17);
    RUN_TEST(test_l76m_out_18);
    RUN_TEST(test_l76m_out_19);
    RUN_TEST(test_l76m_out_20);
    RUN_TEST(test_l76m_title_1);
    RUN_TEST(test_l76m_title_2);
    RUN_TEST(test_l76m_title_3);
    RUN_TEST(test_l76m_title_4);
    RUN_TEST(test_l76m_title_5);
    RUN_TEST(test_l76m_title_6);
    RUN_TEST(test_l76m_title_7);
    RUN_TEST(test_l76m_title_8);
    RUN_TEST(test_l76m_title_9);
    RUN_TEST(test_l76m_title_10);
    RUN_TEST(test_l76m_title_11);
    RUN_TEST(test_l76m_title_12);
    RUN_TEST(test_l76m_title_13);
    RUN_TEST(test_l76m_title_14);
    RUN_TEST(test_l76m_title_15);
    RUN_TEST(test_l76m_title_16);
    RUN_TEST(test_l76m_title_17);
    RUN_TEST(test_l76m_title_18);
    RUN_TEST(test_l76m_title_19);
    RUN_TEST(test_l76m_title_20);
}
