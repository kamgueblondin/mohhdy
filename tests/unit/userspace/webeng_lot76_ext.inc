/* Lot 76: extended parameterized color/date/formtarget/attr coverage. */
#ifndef WE_FF2_AC_OFF
#define WE_FF2_AC_OFF 524288u
#define WE_FF2_FORMTARGET 1048576u
#define WE_FF2_TARGET_BLANK 2097152u
#endif

static we_forms_t g_l76e;
static const char *g_l76e_html;
static void l76e_render(const char *html) {
    we_forms_init(&g_l76e);
    we_set_forms(&g_doc, &g_l76e);
    g_l76e_html = html;
    TEST_ASSERT_EQUAL(0, we_render(&g_doc, html, (int)strlen(html), 480));
}
static void l76e_done(void) { we_set_forms(&g_doc, 0); g_l76e_html = 0; }
static int l76e_node(const char *id) { int i = we_find_id(&g_doc, id); return i > 0 ? i : 0; }
static int l76e_submit(void) {
    int k, x, y;
    for (k = 1; k < (int)g_doc.nnodes; k++)
        if (g_doc.nodes[k].form_type == WE_FORM_SUBMIT) break;
    x = g_doc.nodes[k].x + 2; y = g_doc.nodes[k].y + 2;
    return we_form_click(&g_doc, x, y);
}

static void test_l76_ext_color_1(void) {
    we_form_req_t q;
    char html[500];
    sprintf(html, "<form action=/ec1 method=POST>"
                  "<input id=ec1 type=color name=ec1 value=#112535 autocomplete=off>"
                  "<input type=submit formtarget=_blank></form>");
    l76e_render(html);
    TEST_ASSERT_EQUAL(WE_FORM_COLOR, g_doc.nodes[l76e_node("ec1")].form_type);
    TEST_ASSERT_TRUE(g_doc.nodes[l76e_node("ec1")].form_flags2 & WE_FF2_AC_OFF);
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76e_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_EQUAL(1, q.target_blank);
    TEST_ASSERT_EQUAL(1, q.post);
    if (1) { TEST_ASSERT_NOT_NULL(strstr(q.body, "ec1=%23112535")); }
    else { TEST_ASSERT_NOT_NULL(strstr(q.url, "ec1=%23112535")); }
    l76e_done();
}

static void test_l76_ext_color_2(void) {
    we_form_req_t q;
    char html[500];
    sprintf(html, "<form action=/ec2 method=GET>"
                  "<input id=ec2 type=color name=ec2 value=#224a6a autocomplete=off>"
                  "<input type=submit formtarget=_blank></form>");
    l76e_render(html);
    TEST_ASSERT_EQUAL(WE_FORM_COLOR, g_doc.nodes[l76e_node("ec2")].form_type);
    TEST_ASSERT_TRUE(g_doc.nodes[l76e_node("ec2")].form_flags2 & WE_FF2_AC_OFF);
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76e_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_EQUAL(1, q.target_blank);
    TEST_ASSERT_EQUAL(0, q.post);
    if (0) { TEST_ASSERT_NOT_NULL(strstr(q.body, "ec2=%23224a6a")); }
    else { TEST_ASSERT_NOT_NULL(strstr(q.url, "ec2=%23224a6a")); }
    l76e_done();
}

static void test_l76_ext_color_3(void) {
    we_form_req_t q;
    char html[500];
    sprintf(html, "<form action=/ec3 method=POST>"
                  "<input id=ec3 type=color name=ec3 value=#336f9f autocomplete=off>"
                  "<input type=submit formtarget=_blank></form>");
    l76e_render(html);
    TEST_ASSERT_EQUAL(WE_FORM_COLOR, g_doc.nodes[l76e_node("ec3")].form_type);
    TEST_ASSERT_TRUE(g_doc.nodes[l76e_node("ec3")].form_flags2 & WE_FF2_AC_OFF);
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76e_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_EQUAL(1, q.target_blank);
    TEST_ASSERT_EQUAL(1, q.post);
    if (1) { TEST_ASSERT_NOT_NULL(strstr(q.body, "ec3=%23336f9f")); }
    else { TEST_ASSERT_NOT_NULL(strstr(q.url, "ec3=%23336f9f")); }
    l76e_done();
}

static void test_l76_ext_color_4(void) {
    we_form_req_t q;
    char html[500];
    sprintf(html, "<form action=/ec4 method=GET>"
                  "<input id=ec4 type=color name=ec4 value=#4494d4 autocomplete=off>"
                  "<input type=submit formtarget=_blank></form>");
    l76e_render(html);
    TEST_ASSERT_EQUAL(WE_FORM_COLOR, g_doc.nodes[l76e_node("ec4")].form_type);
    TEST_ASSERT_TRUE(g_doc.nodes[l76e_node("ec4")].form_flags2 & WE_FF2_AC_OFF);
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76e_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_EQUAL(1, q.target_blank);
    TEST_ASSERT_EQUAL(0, q.post);
    if (0) { TEST_ASSERT_NOT_NULL(strstr(q.body, "ec4=%234494d4")); }
    else { TEST_ASSERT_NOT_NULL(strstr(q.url, "ec4=%234494d4")); }
    l76e_done();
}

static void test_l76_ext_color_5(void) {
    we_form_req_t q;
    char html[500];
    sprintf(html, "<form action=/ec5 method=POST>"
                  "<input id=ec5 type=color name=ec5 value=#55b909 autocomplete=off>"
                  "<input type=submit formtarget=_blank></form>");
    l76e_render(html);
    TEST_ASSERT_EQUAL(WE_FORM_COLOR, g_doc.nodes[l76e_node("ec5")].form_type);
    TEST_ASSERT_TRUE(g_doc.nodes[l76e_node("ec5")].form_flags2 & WE_FF2_AC_OFF);
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76e_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_EQUAL(1, q.target_blank);
    TEST_ASSERT_EQUAL(1, q.post);
    if (1) { TEST_ASSERT_NOT_NULL(strstr(q.body, "ec5=%2355b909")); }
    else { TEST_ASSERT_NOT_NULL(strstr(q.url, "ec5=%2355b909")); }
    l76e_done();
}

static void test_l76_ext_color_6(void) {
    we_form_req_t q;
    char html[500];
    sprintf(html, "<form action=/ec6 method=GET>"
                  "<input id=ec6 type=color name=ec6 value=#66de3e autocomplete=off>"
                  "<input type=submit formtarget=_blank></form>");
    l76e_render(html);
    TEST_ASSERT_EQUAL(WE_FORM_COLOR, g_doc.nodes[l76e_node("ec6")].form_type);
    TEST_ASSERT_TRUE(g_doc.nodes[l76e_node("ec6")].form_flags2 & WE_FF2_AC_OFF);
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76e_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_EQUAL(1, q.target_blank);
    TEST_ASSERT_EQUAL(0, q.post);
    if (0) { TEST_ASSERT_NOT_NULL(strstr(q.body, "ec6=%2366de3e")); }
    else { TEST_ASSERT_NOT_NULL(strstr(q.url, "ec6=%2366de3e")); }
    l76e_done();
}

static void test_l76_ext_color_7(void) {
    we_form_req_t q;
    char html[500];
    sprintf(html, "<form action=/ec7 method=POST>"
                  "<input id=ec7 type=color name=ec7 value=#770373 autocomplete=off>"
                  "<input type=submit formtarget=_blank></form>");
    l76e_render(html);
    TEST_ASSERT_EQUAL(WE_FORM_COLOR, g_doc.nodes[l76e_node("ec7")].form_type);
    TEST_ASSERT_TRUE(g_doc.nodes[l76e_node("ec7")].form_flags2 & WE_FF2_AC_OFF);
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76e_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_EQUAL(1, q.target_blank);
    TEST_ASSERT_EQUAL(1, q.post);
    if (1) { TEST_ASSERT_NOT_NULL(strstr(q.body, "ec7=%23770373")); }
    else { TEST_ASSERT_NOT_NULL(strstr(q.url, "ec7=%23770373")); }
    l76e_done();
}

static void test_l76_ext_color_8(void) {
    we_form_req_t q;
    char html[500];
    sprintf(html, "<form action=/ec8 method=GET>"
                  "<input id=ec8 type=color name=ec8 value=#8828a8 autocomplete=off>"
                  "<input type=submit formtarget=_blank></form>");
    l76e_render(html);
    TEST_ASSERT_EQUAL(WE_FORM_COLOR, g_doc.nodes[l76e_node("ec8")].form_type);
    TEST_ASSERT_TRUE(g_doc.nodes[l76e_node("ec8")].form_flags2 & WE_FF2_AC_OFF);
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76e_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_EQUAL(1, q.target_blank);
    TEST_ASSERT_EQUAL(0, q.post);
    if (0) { TEST_ASSERT_NOT_NULL(strstr(q.body, "ec8=%238828a8")); }
    else { TEST_ASSERT_NOT_NULL(strstr(q.url, "ec8=%238828a8")); }
    l76e_done();
}

static void test_l76_ext_color_9(void) {
    we_form_req_t q;
    char html[500];
    sprintf(html, "<form action=/ec9 method=POST>"
                  "<input id=ec9 type=color name=ec9 value=#994ddd autocomplete=off>"
                  "<input type=submit formtarget=_blank></form>");
    l76e_render(html);
    TEST_ASSERT_EQUAL(WE_FORM_COLOR, g_doc.nodes[l76e_node("ec9")].form_type);
    TEST_ASSERT_TRUE(g_doc.nodes[l76e_node("ec9")].form_flags2 & WE_FF2_AC_OFF);
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76e_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_EQUAL(1, q.target_blank);
    TEST_ASSERT_EQUAL(1, q.post);
    if (1) { TEST_ASSERT_NOT_NULL(strstr(q.body, "ec9=%23994ddd")); }
    else { TEST_ASSERT_NOT_NULL(strstr(q.url, "ec9=%23994ddd")); }
    l76e_done();
}

static void test_l76_ext_color_10(void) {
    we_form_req_t q;
    char html[500];
    sprintf(html, "<form action=/ec10 method=GET>"
                  "<input id=ec10 type=color name=ec10 value=#aa7212 autocomplete=off>"
                  "<input type=submit formtarget=_blank></form>");
    l76e_render(html);
    TEST_ASSERT_EQUAL(WE_FORM_COLOR, g_doc.nodes[l76e_node("ec10")].form_type);
    TEST_ASSERT_TRUE(g_doc.nodes[l76e_node("ec10")].form_flags2 & WE_FF2_AC_OFF);
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76e_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_EQUAL(1, q.target_blank);
    TEST_ASSERT_EQUAL(0, q.post);
    if (0) { TEST_ASSERT_NOT_NULL(strstr(q.body, "ec10=%23aa7212")); }
    else { TEST_ASSERT_NOT_NULL(strstr(q.url, "ec10=%23aa7212")); }
    l76e_done();
}

static void test_l76_ext_color_11(void) {
    we_form_req_t q;
    char html[500];
    sprintf(html, "<form action=/ec11 method=POST>"
                  "<input id=ec11 type=color name=ec11 value=#bb9747 autocomplete=off>"
                  "<input type=submit formtarget=_blank></form>");
    l76e_render(html);
    TEST_ASSERT_EQUAL(WE_FORM_COLOR, g_doc.nodes[l76e_node("ec11")].form_type);
    TEST_ASSERT_TRUE(g_doc.nodes[l76e_node("ec11")].form_flags2 & WE_FF2_AC_OFF);
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76e_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_EQUAL(1, q.target_blank);
    TEST_ASSERT_EQUAL(1, q.post);
    if (1) { TEST_ASSERT_NOT_NULL(strstr(q.body, "ec11=%23bb9747")); }
    else { TEST_ASSERT_NOT_NULL(strstr(q.url, "ec11=%23bb9747")); }
    l76e_done();
}

static void test_l76_ext_color_12(void) {
    we_form_req_t q;
    char html[500];
    sprintf(html, "<form action=/ec12 method=GET>"
                  "<input id=ec12 type=color name=ec12 value=#ccbc7c autocomplete=off>"
                  "<input type=submit formtarget=_blank></form>");
    l76e_render(html);
    TEST_ASSERT_EQUAL(WE_FORM_COLOR, g_doc.nodes[l76e_node("ec12")].form_type);
    TEST_ASSERT_TRUE(g_doc.nodes[l76e_node("ec12")].form_flags2 & WE_FF2_AC_OFF);
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76e_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_EQUAL(1, q.target_blank);
    TEST_ASSERT_EQUAL(0, q.post);
    if (0) { TEST_ASSERT_NOT_NULL(strstr(q.body, "ec12=%23ccbc7c")); }
    else { TEST_ASSERT_NOT_NULL(strstr(q.url, "ec12=%23ccbc7c")); }
    l76e_done();
}

static void test_l76_ext_color_13(void) {
    we_form_req_t q;
    char html[500];
    sprintf(html, "<form action=/ec13 method=POST>"
                  "<input id=ec13 type=color name=ec13 value=#dde1b1 autocomplete=off>"
                  "<input type=submit formtarget=_blank></form>");
    l76e_render(html);
    TEST_ASSERT_EQUAL(WE_FORM_COLOR, g_doc.nodes[l76e_node("ec13")].form_type);
    TEST_ASSERT_TRUE(g_doc.nodes[l76e_node("ec13")].form_flags2 & WE_FF2_AC_OFF);
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76e_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_EQUAL(1, q.target_blank);
    TEST_ASSERT_EQUAL(1, q.post);
    if (1) { TEST_ASSERT_NOT_NULL(strstr(q.body, "ec13=%23dde1b1")); }
    else { TEST_ASSERT_NOT_NULL(strstr(q.url, "ec13=%23dde1b1")); }
    l76e_done();
}

static void test_l76_ext_color_14(void) {
    we_form_req_t q;
    char html[500];
    sprintf(html, "<form action=/ec14 method=GET>"
                  "<input id=ec14 type=color name=ec14 value=#ee06e6 autocomplete=off>"
                  "<input type=submit formtarget=_blank></form>");
    l76e_render(html);
    TEST_ASSERT_EQUAL(WE_FORM_COLOR, g_doc.nodes[l76e_node("ec14")].form_type);
    TEST_ASSERT_TRUE(g_doc.nodes[l76e_node("ec14")].form_flags2 & WE_FF2_AC_OFF);
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76e_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_EQUAL(1, q.target_blank);
    TEST_ASSERT_EQUAL(0, q.post);
    if (0) { TEST_ASSERT_NOT_NULL(strstr(q.body, "ec14=%23ee06e6")); }
    else { TEST_ASSERT_NOT_NULL(strstr(q.url, "ec14=%23ee06e6")); }
    l76e_done();
}

static void test_l76_ext_color_15(void) {
    we_form_req_t q;
    char html[500];
    sprintf(html, "<form action=/ec15 method=POST>"
                  "<input id=ec15 type=color name=ec15 value=#ff2b1b autocomplete=off>"
                  "<input type=submit formtarget=_blank></form>");
    l76e_render(html);
    TEST_ASSERT_EQUAL(WE_FORM_COLOR, g_doc.nodes[l76e_node("ec15")].form_type);
    TEST_ASSERT_TRUE(g_doc.nodes[l76e_node("ec15")].form_flags2 & WE_FF2_AC_OFF);
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76e_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_EQUAL(1, q.target_blank);
    TEST_ASSERT_EQUAL(1, q.post);
    if (1) { TEST_ASSERT_NOT_NULL(strstr(q.body, "ec15=%23ff2b1b")); }
    else { TEST_ASSERT_NOT_NULL(strstr(q.url, "ec15=%23ff2b1b")); }
    l76e_done();
}

static void test_l76_ext_color_16(void) {
    we_form_req_t q;
    char html[500];
    sprintf(html, "<form action=/ec16 method=GET>"
                  "<input id=ec16 type=color name=ec16 value=#105050 autocomplete=off>"
                  "<input type=submit formtarget=_blank></form>");
    l76e_render(html);
    TEST_ASSERT_EQUAL(WE_FORM_COLOR, g_doc.nodes[l76e_node("ec16")].form_type);
    TEST_ASSERT_TRUE(g_doc.nodes[l76e_node("ec16")].form_flags2 & WE_FF2_AC_OFF);
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76e_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_EQUAL(1, q.target_blank);
    TEST_ASSERT_EQUAL(0, q.post);
    if (0) { TEST_ASSERT_NOT_NULL(strstr(q.body, "ec16=%23105050")); }
    else { TEST_ASSERT_NOT_NULL(strstr(q.url, "ec16=%23105050")); }
    l76e_done();
}

static void test_l76_ext_color_17(void) {
    we_form_req_t q;
    char html[500];
    sprintf(html, "<form action=/ec17 method=POST>"
                  "<input id=ec17 type=color name=ec17 value=#217585 autocomplete=off>"
                  "<input type=submit formtarget=_blank></form>");
    l76e_render(html);
    TEST_ASSERT_EQUAL(WE_FORM_COLOR, g_doc.nodes[l76e_node("ec17")].form_type);
    TEST_ASSERT_TRUE(g_doc.nodes[l76e_node("ec17")].form_flags2 & WE_FF2_AC_OFF);
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76e_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_EQUAL(1, q.target_blank);
    TEST_ASSERT_EQUAL(1, q.post);
    if (1) { TEST_ASSERT_NOT_NULL(strstr(q.body, "ec17=%23217585")); }
    else { TEST_ASSERT_NOT_NULL(strstr(q.url, "ec17=%23217585")); }
    l76e_done();
}

static void test_l76_ext_color_18(void) {
    we_form_req_t q;
    char html[500];
    sprintf(html, "<form action=/ec18 method=GET>"
                  "<input id=ec18 type=color name=ec18 value=#329aba autocomplete=off>"
                  "<input type=submit formtarget=_blank></form>");
    l76e_render(html);
    TEST_ASSERT_EQUAL(WE_FORM_COLOR, g_doc.nodes[l76e_node("ec18")].form_type);
    TEST_ASSERT_TRUE(g_doc.nodes[l76e_node("ec18")].form_flags2 & WE_FF2_AC_OFF);
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76e_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_EQUAL(1, q.target_blank);
    TEST_ASSERT_EQUAL(0, q.post);
    if (0) { TEST_ASSERT_NOT_NULL(strstr(q.body, "ec18=%23329aba")); }
    else { TEST_ASSERT_NOT_NULL(strstr(q.url, "ec18=%23329aba")); }
    l76e_done();
}

static void test_l76_ext_color_19(void) {
    we_form_req_t q;
    char html[500];
    sprintf(html, "<form action=/ec19 method=POST>"
                  "<input id=ec19 type=color name=ec19 value=#43bfef autocomplete=off>"
                  "<input type=submit formtarget=_blank></form>");
    l76e_render(html);
    TEST_ASSERT_EQUAL(WE_FORM_COLOR, g_doc.nodes[l76e_node("ec19")].form_type);
    TEST_ASSERT_TRUE(g_doc.nodes[l76e_node("ec19")].form_flags2 & WE_FF2_AC_OFF);
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76e_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_EQUAL(1, q.target_blank);
    TEST_ASSERT_EQUAL(1, q.post);
    if (1) { TEST_ASSERT_NOT_NULL(strstr(q.body, "ec19=%2343bfef")); }
    else { TEST_ASSERT_NOT_NULL(strstr(q.url, "ec19=%2343bfef")); }
    l76e_done();
}

static void test_l76_ext_color_20(void) {
    we_form_req_t q;
    char html[500];
    sprintf(html, "<form action=/ec20 method=GET>"
                  "<input id=ec20 type=color name=ec20 value=#54e424 autocomplete=off>"
                  "<input type=submit formtarget=_blank></form>");
    l76e_render(html);
    TEST_ASSERT_EQUAL(WE_FORM_COLOR, g_doc.nodes[l76e_node("ec20")].form_type);
    TEST_ASSERT_TRUE(g_doc.nodes[l76e_node("ec20")].form_flags2 & WE_FF2_AC_OFF);
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76e_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_EQUAL(1, q.target_blank);
    TEST_ASSERT_EQUAL(0, q.post);
    if (0) { TEST_ASSERT_NOT_NULL(strstr(q.body, "ec20=%2354e424")); }
    else { TEST_ASSERT_NOT_NULL(strstr(q.url, "ec20=%2354e424")); }
    l76e_done();
}

static void test_l76_ext_color_21(void) {
    we_form_req_t q;
    char html[500];
    sprintf(html, "<form action=/ec21 method=POST>"
                  "<input id=ec21 type=color name=ec21 value=#650959 autocomplete=off>"
                  "<input type=submit formtarget=_blank></form>");
    l76e_render(html);
    TEST_ASSERT_EQUAL(WE_FORM_COLOR, g_doc.nodes[l76e_node("ec21")].form_type);
    TEST_ASSERT_TRUE(g_doc.nodes[l76e_node("ec21")].form_flags2 & WE_FF2_AC_OFF);
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76e_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_EQUAL(1, q.target_blank);
    TEST_ASSERT_EQUAL(1, q.post);
    if (1) { TEST_ASSERT_NOT_NULL(strstr(q.body, "ec21=%23650959")); }
    else { TEST_ASSERT_NOT_NULL(strstr(q.url, "ec21=%23650959")); }
    l76e_done();
}

static void test_l76_ext_color_22(void) {
    we_form_req_t q;
    char html[500];
    sprintf(html, "<form action=/ec22 method=GET>"
                  "<input id=ec22 type=color name=ec22 value=#762e8e autocomplete=off>"
                  "<input type=submit formtarget=_blank></form>");
    l76e_render(html);
    TEST_ASSERT_EQUAL(WE_FORM_COLOR, g_doc.nodes[l76e_node("ec22")].form_type);
    TEST_ASSERT_TRUE(g_doc.nodes[l76e_node("ec22")].form_flags2 & WE_FF2_AC_OFF);
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76e_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_EQUAL(1, q.target_blank);
    TEST_ASSERT_EQUAL(0, q.post);
    if (0) { TEST_ASSERT_NOT_NULL(strstr(q.body, "ec22=%23762e8e")); }
    else { TEST_ASSERT_NOT_NULL(strstr(q.url, "ec22=%23762e8e")); }
    l76e_done();
}

static void test_l76_ext_color_23(void) {
    we_form_req_t q;
    char html[500];
    sprintf(html, "<form action=/ec23 method=POST>"
                  "<input id=ec23 type=color name=ec23 value=#8753c3 autocomplete=off>"
                  "<input type=submit formtarget=_blank></form>");
    l76e_render(html);
    TEST_ASSERT_EQUAL(WE_FORM_COLOR, g_doc.nodes[l76e_node("ec23")].form_type);
    TEST_ASSERT_TRUE(g_doc.nodes[l76e_node("ec23")].form_flags2 & WE_FF2_AC_OFF);
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76e_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_EQUAL(1, q.target_blank);
    TEST_ASSERT_EQUAL(1, q.post);
    if (1) { TEST_ASSERT_NOT_NULL(strstr(q.body, "ec23=%238753c3")); }
    else { TEST_ASSERT_NOT_NULL(strstr(q.url, "ec23=%238753c3")); }
    l76e_done();
}

static void test_l76_ext_color_24(void) {
    we_form_req_t q;
    char html[500];
    sprintf(html, "<form action=/ec24 method=GET>"
                  "<input id=ec24 type=color name=ec24 value=#9878f8 autocomplete=off>"
                  "<input type=submit formtarget=_blank></form>");
    l76e_render(html);
    TEST_ASSERT_EQUAL(WE_FORM_COLOR, g_doc.nodes[l76e_node("ec24")].form_type);
    TEST_ASSERT_TRUE(g_doc.nodes[l76e_node("ec24")].form_flags2 & WE_FF2_AC_OFF);
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76e_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_EQUAL(1, q.target_blank);
    TEST_ASSERT_EQUAL(0, q.post);
    if (0) { TEST_ASSERT_NOT_NULL(strstr(q.body, "ec24=%239878f8")); }
    else { TEST_ASSERT_NOT_NULL(strstr(q.url, "ec24=%239878f8")); }
    l76e_done();
}

static void test_l76_ext_color_25(void) {
    we_form_req_t q;
    char html[500];
    sprintf(html, "<form action=/ec25 method=POST>"
                  "<input id=ec25 type=color name=ec25 value=#a99d2d autocomplete=off>"
                  "<input type=submit formtarget=_blank></form>");
    l76e_render(html);
    TEST_ASSERT_EQUAL(WE_FORM_COLOR, g_doc.nodes[l76e_node("ec25")].form_type);
    TEST_ASSERT_TRUE(g_doc.nodes[l76e_node("ec25")].form_flags2 & WE_FF2_AC_OFF);
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76e_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_EQUAL(1, q.target_blank);
    TEST_ASSERT_EQUAL(1, q.post);
    if (1) { TEST_ASSERT_NOT_NULL(strstr(q.body, "ec25=%23a99d2d")); }
    else { TEST_ASSERT_NOT_NULL(strstr(q.url, "ec25=%23a99d2d")); }
    l76e_done();
}

static void test_l76_ext_color_26(void) {
    we_form_req_t q;
    char html[500];
    sprintf(html, "<form action=/ec26 method=GET>"
                  "<input id=ec26 type=color name=ec26 value=#bac262 autocomplete=off>"
                  "<input type=submit formtarget=_blank></form>");
    l76e_render(html);
    TEST_ASSERT_EQUAL(WE_FORM_COLOR, g_doc.nodes[l76e_node("ec26")].form_type);
    TEST_ASSERT_TRUE(g_doc.nodes[l76e_node("ec26")].form_flags2 & WE_FF2_AC_OFF);
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76e_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_EQUAL(1, q.target_blank);
    TEST_ASSERT_EQUAL(0, q.post);
    if (0) { TEST_ASSERT_NOT_NULL(strstr(q.body, "ec26=%23bac262")); }
    else { TEST_ASSERT_NOT_NULL(strstr(q.url, "ec26=%23bac262")); }
    l76e_done();
}

static void test_l76_ext_color_27(void) {
    we_form_req_t q;
    char html[500];
    sprintf(html, "<form action=/ec27 method=POST>"
                  "<input id=ec27 type=color name=ec27 value=#cbe797 autocomplete=off>"
                  "<input type=submit formtarget=_blank></form>");
    l76e_render(html);
    TEST_ASSERT_EQUAL(WE_FORM_COLOR, g_doc.nodes[l76e_node("ec27")].form_type);
    TEST_ASSERT_TRUE(g_doc.nodes[l76e_node("ec27")].form_flags2 & WE_FF2_AC_OFF);
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76e_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_EQUAL(1, q.target_blank);
    TEST_ASSERT_EQUAL(1, q.post);
    if (1) { TEST_ASSERT_NOT_NULL(strstr(q.body, "ec27=%23cbe797")); }
    else { TEST_ASSERT_NOT_NULL(strstr(q.url, "ec27=%23cbe797")); }
    l76e_done();
}

static void test_l76_ext_color_28(void) {
    we_form_req_t q;
    char html[500];
    sprintf(html, "<form action=/ec28 method=GET>"
                  "<input id=ec28 type=color name=ec28 value=#dc0ccc autocomplete=off>"
                  "<input type=submit formtarget=_blank></form>");
    l76e_render(html);
    TEST_ASSERT_EQUAL(WE_FORM_COLOR, g_doc.nodes[l76e_node("ec28")].form_type);
    TEST_ASSERT_TRUE(g_doc.nodes[l76e_node("ec28")].form_flags2 & WE_FF2_AC_OFF);
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76e_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_EQUAL(1, q.target_blank);
    TEST_ASSERT_EQUAL(0, q.post);
    if (0) { TEST_ASSERT_NOT_NULL(strstr(q.body, "ec28=%23dc0ccc")); }
    else { TEST_ASSERT_NOT_NULL(strstr(q.url, "ec28=%23dc0ccc")); }
    l76e_done();
}

static void test_l76_ext_color_29(void) {
    we_form_req_t q;
    char html[500];
    sprintf(html, "<form action=/ec29 method=POST>"
                  "<input id=ec29 type=color name=ec29 value=#ed3101 autocomplete=off>"
                  "<input type=submit formtarget=_blank></form>");
    l76e_render(html);
    TEST_ASSERT_EQUAL(WE_FORM_COLOR, g_doc.nodes[l76e_node("ec29")].form_type);
    TEST_ASSERT_TRUE(g_doc.nodes[l76e_node("ec29")].form_flags2 & WE_FF2_AC_OFF);
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76e_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_EQUAL(1, q.target_blank);
    TEST_ASSERT_EQUAL(1, q.post);
    if (1) { TEST_ASSERT_NOT_NULL(strstr(q.body, "ec29=%23ed3101")); }
    else { TEST_ASSERT_NOT_NULL(strstr(q.url, "ec29=%23ed3101")); }
    l76e_done();
}

static void test_l76_ext_color_30(void) {
    we_form_req_t q;
    char html[500];
    sprintf(html, "<form action=/ec30 method=GET>"
                  "<input id=ec30 type=color name=ec30 value=#fe5636 autocomplete=off>"
                  "<input type=submit formtarget=_blank></form>");
    l76e_render(html);
    TEST_ASSERT_EQUAL(WE_FORM_COLOR, g_doc.nodes[l76e_node("ec30")].form_type);
    TEST_ASSERT_TRUE(g_doc.nodes[l76e_node("ec30")].form_flags2 & WE_FF2_AC_OFF);
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76e_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_EQUAL(1, q.target_blank);
    TEST_ASSERT_EQUAL(0, q.post);
    if (0) { TEST_ASSERT_NOT_NULL(strstr(q.body, "ec30=%23fe5636")); }
    else { TEST_ASSERT_NOT_NULL(strstr(q.url, "ec30=%23fe5636")); }
    l76e_done();
}

static void test_l76_ext_color_31(void) {
    we_form_req_t q;
    char html[500];
    sprintf(html, "<form action=/ec31 method=POST>"
                  "<input id=ec31 type=color name=ec31 value=#0f7b6b autocomplete=off>"
                  "<input type=submit formtarget=_blank></form>");
    l76e_render(html);
    TEST_ASSERT_EQUAL(WE_FORM_COLOR, g_doc.nodes[l76e_node("ec31")].form_type);
    TEST_ASSERT_TRUE(g_doc.nodes[l76e_node("ec31")].form_flags2 & WE_FF2_AC_OFF);
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76e_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_EQUAL(1, q.target_blank);
    TEST_ASSERT_EQUAL(1, q.post);
    if (1) { TEST_ASSERT_NOT_NULL(strstr(q.body, "ec31=%230f7b6b")); }
    else { TEST_ASSERT_NOT_NULL(strstr(q.url, "ec31=%230f7b6b")); }
    l76e_done();
}

static void test_l76_ext_color_32(void) {
    we_form_req_t q;
    char html[500];
    sprintf(html, "<form action=/ec32 method=GET>"
                  "<input id=ec32 type=color name=ec32 value=#20a0a0 autocomplete=off>"
                  "<input type=submit formtarget=_blank></form>");
    l76e_render(html);
    TEST_ASSERT_EQUAL(WE_FORM_COLOR, g_doc.nodes[l76e_node("ec32")].form_type);
    TEST_ASSERT_TRUE(g_doc.nodes[l76e_node("ec32")].form_flags2 & WE_FF2_AC_OFF);
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76e_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_EQUAL(1, q.target_blank);
    TEST_ASSERT_EQUAL(0, q.post);
    if (0) { TEST_ASSERT_NOT_NULL(strstr(q.body, "ec32=%2320a0a0")); }
    else { TEST_ASSERT_NOT_NULL(strstr(q.url, "ec32=%2320a0a0")); }
    l76e_done();
}

static void test_l76_ext_color_33(void) {
    we_form_req_t q;
    char html[500];
    sprintf(html, "<form action=/ec33 method=POST>"
                  "<input id=ec33 type=color name=ec33 value=#31c5d5 autocomplete=off>"
                  "<input type=submit formtarget=_blank></form>");
    l76e_render(html);
    TEST_ASSERT_EQUAL(WE_FORM_COLOR, g_doc.nodes[l76e_node("ec33")].form_type);
    TEST_ASSERT_TRUE(g_doc.nodes[l76e_node("ec33")].form_flags2 & WE_FF2_AC_OFF);
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76e_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_EQUAL(1, q.target_blank);
    TEST_ASSERT_EQUAL(1, q.post);
    if (1) { TEST_ASSERT_NOT_NULL(strstr(q.body, "ec33=%2331c5d5")); }
    else { TEST_ASSERT_NOT_NULL(strstr(q.url, "ec33=%2331c5d5")); }
    l76e_done();
}

static void test_l76_ext_color_34(void) {
    we_form_req_t q;
    char html[500];
    sprintf(html, "<form action=/ec34 method=GET>"
                  "<input id=ec34 type=color name=ec34 value=#42ea0a autocomplete=off>"
                  "<input type=submit formtarget=_blank></form>");
    l76e_render(html);
    TEST_ASSERT_EQUAL(WE_FORM_COLOR, g_doc.nodes[l76e_node("ec34")].form_type);
    TEST_ASSERT_TRUE(g_doc.nodes[l76e_node("ec34")].form_flags2 & WE_FF2_AC_OFF);
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76e_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_EQUAL(1, q.target_blank);
    TEST_ASSERT_EQUAL(0, q.post);
    if (0) { TEST_ASSERT_NOT_NULL(strstr(q.body, "ec34=%2342ea0a")); }
    else { TEST_ASSERT_NOT_NULL(strstr(q.url, "ec34=%2342ea0a")); }
    l76e_done();
}

static void test_l76_ext_color_35(void) {
    we_form_req_t q;
    char html[500];
    sprintf(html, "<form action=/ec35 method=POST>"
                  "<input id=ec35 type=color name=ec35 value=#530f3f autocomplete=off>"
                  "<input type=submit formtarget=_blank></form>");
    l76e_render(html);
    TEST_ASSERT_EQUAL(WE_FORM_COLOR, g_doc.nodes[l76e_node("ec35")].form_type);
    TEST_ASSERT_TRUE(g_doc.nodes[l76e_node("ec35")].form_flags2 & WE_FF2_AC_OFF);
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76e_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_EQUAL(1, q.target_blank);
    TEST_ASSERT_EQUAL(1, q.post);
    if (1) { TEST_ASSERT_NOT_NULL(strstr(q.body, "ec35=%23530f3f")); }
    else { TEST_ASSERT_NOT_NULL(strstr(q.url, "ec35=%23530f3f")); }
    l76e_done();
}

static void test_l76_ext_color_36(void) {
    we_form_req_t q;
    char html[500];
    sprintf(html, "<form action=/ec36 method=GET>"
                  "<input id=ec36 type=color name=ec36 value=#643474 autocomplete=off>"
                  "<input type=submit formtarget=_blank></form>");
    l76e_render(html);
    TEST_ASSERT_EQUAL(WE_FORM_COLOR, g_doc.nodes[l76e_node("ec36")].form_type);
    TEST_ASSERT_TRUE(g_doc.nodes[l76e_node("ec36")].form_flags2 & WE_FF2_AC_OFF);
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76e_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_EQUAL(1, q.target_blank);
    TEST_ASSERT_EQUAL(0, q.post);
    if (0) { TEST_ASSERT_NOT_NULL(strstr(q.body, "ec36=%23643474")); }
    else { TEST_ASSERT_NOT_NULL(strstr(q.url, "ec36=%23643474")); }
    l76e_done();
}

static void test_l76_ext_color_37(void) {
    we_form_req_t q;
    char html[500];
    sprintf(html, "<form action=/ec37 method=POST>"
                  "<input id=ec37 type=color name=ec37 value=#7559a9 autocomplete=off>"
                  "<input type=submit formtarget=_blank></form>");
    l76e_render(html);
    TEST_ASSERT_EQUAL(WE_FORM_COLOR, g_doc.nodes[l76e_node("ec37")].form_type);
    TEST_ASSERT_TRUE(g_doc.nodes[l76e_node("ec37")].form_flags2 & WE_FF2_AC_OFF);
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76e_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_EQUAL(1, q.target_blank);
    TEST_ASSERT_EQUAL(1, q.post);
    if (1) { TEST_ASSERT_NOT_NULL(strstr(q.body, "ec37=%237559a9")); }
    else { TEST_ASSERT_NOT_NULL(strstr(q.url, "ec37=%237559a9")); }
    l76e_done();
}

static void test_l76_ext_color_38(void) {
    we_form_req_t q;
    char html[500];
    sprintf(html, "<form action=/ec38 method=GET>"
                  "<input id=ec38 type=color name=ec38 value=#867ede autocomplete=off>"
                  "<input type=submit formtarget=_blank></form>");
    l76e_render(html);
    TEST_ASSERT_EQUAL(WE_FORM_COLOR, g_doc.nodes[l76e_node("ec38")].form_type);
    TEST_ASSERT_TRUE(g_doc.nodes[l76e_node("ec38")].form_flags2 & WE_FF2_AC_OFF);
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76e_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_EQUAL(1, q.target_blank);
    TEST_ASSERT_EQUAL(0, q.post);
    if (0) { TEST_ASSERT_NOT_NULL(strstr(q.body, "ec38=%23867ede")); }
    else { TEST_ASSERT_NOT_NULL(strstr(q.url, "ec38=%23867ede")); }
    l76e_done();
}

static void test_l76_ext_color_39(void) {
    we_form_req_t q;
    char html[500];
    sprintf(html, "<form action=/ec39 method=POST>"
                  "<input id=ec39 type=color name=ec39 value=#97a313 autocomplete=off>"
                  "<input type=submit formtarget=_blank></form>");
    l76e_render(html);
    TEST_ASSERT_EQUAL(WE_FORM_COLOR, g_doc.nodes[l76e_node("ec39")].form_type);
    TEST_ASSERT_TRUE(g_doc.nodes[l76e_node("ec39")].form_flags2 & WE_FF2_AC_OFF);
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76e_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_EQUAL(1, q.target_blank);
    TEST_ASSERT_EQUAL(1, q.post);
    if (1) { TEST_ASSERT_NOT_NULL(strstr(q.body, "ec39=%2397a313")); }
    else { TEST_ASSERT_NOT_NULL(strstr(q.url, "ec39=%2397a313")); }
    l76e_done();
}

static void test_l76_ext_color_40(void) {
    we_form_req_t q;
    char html[500];
    sprintf(html, "<form action=/ec40 method=GET>"
                  "<input id=ec40 type=color name=ec40 value=#a8c848 autocomplete=off>"
                  "<input type=submit formtarget=_blank></form>");
    l76e_render(html);
    TEST_ASSERT_EQUAL(WE_FORM_COLOR, g_doc.nodes[l76e_node("ec40")].form_type);
    TEST_ASSERT_TRUE(g_doc.nodes[l76e_node("ec40")].form_flags2 & WE_FF2_AC_OFF);
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76e_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_EQUAL(1, q.target_blank);
    TEST_ASSERT_EQUAL(0, q.post);
    if (0) { TEST_ASSERT_NOT_NULL(strstr(q.body, "ec40=%23a8c848")); }
    else { TEST_ASSERT_NOT_NULL(strstr(q.url, "ec40=%23a8c848")); }
    l76e_done();
}

static void test_l76_ext_color_41(void) {
    we_form_req_t q;
    char html[500];
    sprintf(html, "<form action=/ec41 method=POST>"
                  "<input id=ec41 type=color name=ec41 value=#b9ed7d autocomplete=off>"
                  "<input type=submit formtarget=_blank></form>");
    l76e_render(html);
    TEST_ASSERT_EQUAL(WE_FORM_COLOR, g_doc.nodes[l76e_node("ec41")].form_type);
    TEST_ASSERT_TRUE(g_doc.nodes[l76e_node("ec41")].form_flags2 & WE_FF2_AC_OFF);
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76e_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_EQUAL(1, q.target_blank);
    TEST_ASSERT_EQUAL(1, q.post);
    if (1) { TEST_ASSERT_NOT_NULL(strstr(q.body, "ec41=%23b9ed7d")); }
    else { TEST_ASSERT_NOT_NULL(strstr(q.url, "ec41=%23b9ed7d")); }
    l76e_done();
}

static void test_l76_ext_color_42(void) {
    we_form_req_t q;
    char html[500];
    sprintf(html, "<form action=/ec42 method=GET>"
                  "<input id=ec42 type=color name=ec42 value=#ca12b2 autocomplete=off>"
                  "<input type=submit formtarget=_blank></form>");
    l76e_render(html);
    TEST_ASSERT_EQUAL(WE_FORM_COLOR, g_doc.nodes[l76e_node("ec42")].form_type);
    TEST_ASSERT_TRUE(g_doc.nodes[l76e_node("ec42")].form_flags2 & WE_FF2_AC_OFF);
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76e_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_EQUAL(1, q.target_blank);
    TEST_ASSERT_EQUAL(0, q.post);
    if (0) { TEST_ASSERT_NOT_NULL(strstr(q.body, "ec42=%23ca12b2")); }
    else { TEST_ASSERT_NOT_NULL(strstr(q.url, "ec42=%23ca12b2")); }
    l76e_done();
}

static void test_l76_ext_color_43(void) {
    we_form_req_t q;
    char html[500];
    sprintf(html, "<form action=/ec43 method=POST>"
                  "<input id=ec43 type=color name=ec43 value=#db37e7 autocomplete=off>"
                  "<input type=submit formtarget=_blank></form>");
    l76e_render(html);
    TEST_ASSERT_EQUAL(WE_FORM_COLOR, g_doc.nodes[l76e_node("ec43")].form_type);
    TEST_ASSERT_TRUE(g_doc.nodes[l76e_node("ec43")].form_flags2 & WE_FF2_AC_OFF);
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76e_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_EQUAL(1, q.target_blank);
    TEST_ASSERT_EQUAL(1, q.post);
    if (1) { TEST_ASSERT_NOT_NULL(strstr(q.body, "ec43=%23db37e7")); }
    else { TEST_ASSERT_NOT_NULL(strstr(q.url, "ec43=%23db37e7")); }
    l76e_done();
}

static void test_l76_ext_color_44(void) {
    we_form_req_t q;
    char html[500];
    sprintf(html, "<form action=/ec44 method=GET>"
                  "<input id=ec44 type=color name=ec44 value=#ec5c1c autocomplete=off>"
                  "<input type=submit formtarget=_blank></form>");
    l76e_render(html);
    TEST_ASSERT_EQUAL(WE_FORM_COLOR, g_doc.nodes[l76e_node("ec44")].form_type);
    TEST_ASSERT_TRUE(g_doc.nodes[l76e_node("ec44")].form_flags2 & WE_FF2_AC_OFF);
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76e_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_EQUAL(1, q.target_blank);
    TEST_ASSERT_EQUAL(0, q.post);
    if (0) { TEST_ASSERT_NOT_NULL(strstr(q.body, "ec44=%23ec5c1c")); }
    else { TEST_ASSERT_NOT_NULL(strstr(q.url, "ec44=%23ec5c1c")); }
    l76e_done();
}

static void test_l76_ext_color_45(void) {
    we_form_req_t q;
    char html[500];
    sprintf(html, "<form action=/ec45 method=POST>"
                  "<input id=ec45 type=color name=ec45 value=#fd8151 autocomplete=off>"
                  "<input type=submit formtarget=_blank></form>");
    l76e_render(html);
    TEST_ASSERT_EQUAL(WE_FORM_COLOR, g_doc.nodes[l76e_node("ec45")].form_type);
    TEST_ASSERT_TRUE(g_doc.nodes[l76e_node("ec45")].form_flags2 & WE_FF2_AC_OFF);
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76e_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_EQUAL(1, q.target_blank);
    TEST_ASSERT_EQUAL(1, q.post);
    if (1) { TEST_ASSERT_NOT_NULL(strstr(q.body, "ec45=%23fd8151")); }
    else { TEST_ASSERT_NOT_NULL(strstr(q.url, "ec45=%23fd8151")); }
    l76e_done();
}

static void test_l76_ext_color_46(void) {
    we_form_req_t q;
    char html[500];
    sprintf(html, "<form action=/ec46 method=GET>"
                  "<input id=ec46 type=color name=ec46 value=#0ea686 autocomplete=off>"
                  "<input type=submit formtarget=_blank></form>");
    l76e_render(html);
    TEST_ASSERT_EQUAL(WE_FORM_COLOR, g_doc.nodes[l76e_node("ec46")].form_type);
    TEST_ASSERT_TRUE(g_doc.nodes[l76e_node("ec46")].form_flags2 & WE_FF2_AC_OFF);
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76e_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_EQUAL(1, q.target_blank);
    TEST_ASSERT_EQUAL(0, q.post);
    if (0) { TEST_ASSERT_NOT_NULL(strstr(q.body, "ec46=%230ea686")); }
    else { TEST_ASSERT_NOT_NULL(strstr(q.url, "ec46=%230ea686")); }
    l76e_done();
}

static void test_l76_ext_color_47(void) {
    we_form_req_t q;
    char html[500];
    sprintf(html, "<form action=/ec47 method=POST>"
                  "<input id=ec47 type=color name=ec47 value=#1fcbbb autocomplete=off>"
                  "<input type=submit formtarget=_blank></form>");
    l76e_render(html);
    TEST_ASSERT_EQUAL(WE_FORM_COLOR, g_doc.nodes[l76e_node("ec47")].form_type);
    TEST_ASSERT_TRUE(g_doc.nodes[l76e_node("ec47")].form_flags2 & WE_FF2_AC_OFF);
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76e_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_EQUAL(1, q.target_blank);
    TEST_ASSERT_EQUAL(1, q.post);
    if (1) { TEST_ASSERT_NOT_NULL(strstr(q.body, "ec47=%231fcbbb")); }
    else { TEST_ASSERT_NOT_NULL(strstr(q.url, "ec47=%231fcbbb")); }
    l76e_done();
}

static void test_l76_ext_color_48(void) {
    we_form_req_t q;
    char html[500];
    sprintf(html, "<form action=/ec48 method=GET>"
                  "<input id=ec48 type=color name=ec48 value=#30f0f0 autocomplete=off>"
                  "<input type=submit formtarget=_blank></form>");
    l76e_render(html);
    TEST_ASSERT_EQUAL(WE_FORM_COLOR, g_doc.nodes[l76e_node("ec48")].form_type);
    TEST_ASSERT_TRUE(g_doc.nodes[l76e_node("ec48")].form_flags2 & WE_FF2_AC_OFF);
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76e_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_EQUAL(1, q.target_blank);
    TEST_ASSERT_EQUAL(0, q.post);
    if (0) { TEST_ASSERT_NOT_NULL(strstr(q.body, "ec48=%2330f0f0")); }
    else { TEST_ASSERT_NOT_NULL(strstr(q.url, "ec48=%2330f0f0")); }
    l76e_done();
}

static void test_l76_ext_color_49(void) {
    we_form_req_t q;
    char html[500];
    sprintf(html, "<form action=/ec49 method=POST>"
                  "<input id=ec49 type=color name=ec49 value=#411525 autocomplete=off>"
                  "<input type=submit formtarget=_blank></form>");
    l76e_render(html);
    TEST_ASSERT_EQUAL(WE_FORM_COLOR, g_doc.nodes[l76e_node("ec49")].form_type);
    TEST_ASSERT_TRUE(g_doc.nodes[l76e_node("ec49")].form_flags2 & WE_FF2_AC_OFF);
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76e_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_EQUAL(1, q.target_blank);
    TEST_ASSERT_EQUAL(1, q.post);
    if (1) { TEST_ASSERT_NOT_NULL(strstr(q.body, "ec49=%23411525")); }
    else { TEST_ASSERT_NOT_NULL(strstr(q.url, "ec49=%23411525")); }
    l76e_done();
}

static void test_l76_ext_color_50(void) {
    we_form_req_t q;
    char html[500];
    sprintf(html, "<form action=/ec50 method=GET>"
                  "<input id=ec50 type=color name=ec50 value=#523a5a autocomplete=off>"
                  "<input type=submit formtarget=_blank></form>");
    l76e_render(html);
    TEST_ASSERT_EQUAL(WE_FORM_COLOR, g_doc.nodes[l76e_node("ec50")].form_type);
    TEST_ASSERT_TRUE(g_doc.nodes[l76e_node("ec50")].form_flags2 & WE_FF2_AC_OFF);
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76e_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_EQUAL(1, q.target_blank);
    TEST_ASSERT_EQUAL(0, q.post);
    if (0) { TEST_ASSERT_NOT_NULL(strstr(q.body, "ec50=%23523a5a")); }
    else { TEST_ASSERT_NOT_NULL(strstr(q.url, "ec50=%23523a5a")); }
    l76e_done();
}

static void test_l76_ext_color_51(void) {
    we_form_req_t q;
    char html[500];
    sprintf(html, "<form action=/ec51 method=POST>"
                  "<input id=ec51 type=color name=ec51 value=#635f8f autocomplete=off>"
                  "<input type=submit formtarget=_blank></form>");
    l76e_render(html);
    TEST_ASSERT_EQUAL(WE_FORM_COLOR, g_doc.nodes[l76e_node("ec51")].form_type);
    TEST_ASSERT_TRUE(g_doc.nodes[l76e_node("ec51")].form_flags2 & WE_FF2_AC_OFF);
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76e_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_EQUAL(1, q.target_blank);
    TEST_ASSERT_EQUAL(1, q.post);
    if (1) { TEST_ASSERT_NOT_NULL(strstr(q.body, "ec51=%23635f8f")); }
    else { TEST_ASSERT_NOT_NULL(strstr(q.url, "ec51=%23635f8f")); }
    l76e_done();
}

static void test_l76_ext_color_52(void) {
    we_form_req_t q;
    char html[500];
    sprintf(html, "<form action=/ec52 method=GET>"
                  "<input id=ec52 type=color name=ec52 value=#7484c4 autocomplete=off>"
                  "<input type=submit formtarget=_blank></form>");
    l76e_render(html);
    TEST_ASSERT_EQUAL(WE_FORM_COLOR, g_doc.nodes[l76e_node("ec52")].form_type);
    TEST_ASSERT_TRUE(g_doc.nodes[l76e_node("ec52")].form_flags2 & WE_FF2_AC_OFF);
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76e_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_EQUAL(1, q.target_blank);
    TEST_ASSERT_EQUAL(0, q.post);
    if (0) { TEST_ASSERT_NOT_NULL(strstr(q.body, "ec52=%237484c4")); }
    else { TEST_ASSERT_NOT_NULL(strstr(q.url, "ec52=%237484c4")); }
    l76e_done();
}

static void test_l76_ext_color_53(void) {
    we_form_req_t q;
    char html[500];
    sprintf(html, "<form action=/ec53 method=POST>"
                  "<input id=ec53 type=color name=ec53 value=#85a9f9 autocomplete=off>"
                  "<input type=submit formtarget=_blank></form>");
    l76e_render(html);
    TEST_ASSERT_EQUAL(WE_FORM_COLOR, g_doc.nodes[l76e_node("ec53")].form_type);
    TEST_ASSERT_TRUE(g_doc.nodes[l76e_node("ec53")].form_flags2 & WE_FF2_AC_OFF);
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76e_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_EQUAL(1, q.target_blank);
    TEST_ASSERT_EQUAL(1, q.post);
    if (1) { TEST_ASSERT_NOT_NULL(strstr(q.body, "ec53=%2385a9f9")); }
    else { TEST_ASSERT_NOT_NULL(strstr(q.url, "ec53=%2385a9f9")); }
    l76e_done();
}

static void test_l76_ext_color_54(void) {
    we_form_req_t q;
    char html[500];
    sprintf(html, "<form action=/ec54 method=GET>"
                  "<input id=ec54 type=color name=ec54 value=#96ce2e autocomplete=off>"
                  "<input type=submit formtarget=_blank></form>");
    l76e_render(html);
    TEST_ASSERT_EQUAL(WE_FORM_COLOR, g_doc.nodes[l76e_node("ec54")].form_type);
    TEST_ASSERT_TRUE(g_doc.nodes[l76e_node("ec54")].form_flags2 & WE_FF2_AC_OFF);
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76e_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_EQUAL(1, q.target_blank);
    TEST_ASSERT_EQUAL(0, q.post);
    if (0) { TEST_ASSERT_NOT_NULL(strstr(q.body, "ec54=%2396ce2e")); }
    else { TEST_ASSERT_NOT_NULL(strstr(q.url, "ec54=%2396ce2e")); }
    l76e_done();
}

static void test_l76_ext_color_55(void) {
    we_form_req_t q;
    char html[500];
    sprintf(html, "<form action=/ec55 method=POST>"
                  "<input id=ec55 type=color name=ec55 value=#a7f363 autocomplete=off>"
                  "<input type=submit formtarget=_blank></form>");
    l76e_render(html);
    TEST_ASSERT_EQUAL(WE_FORM_COLOR, g_doc.nodes[l76e_node("ec55")].form_type);
    TEST_ASSERT_TRUE(g_doc.nodes[l76e_node("ec55")].form_flags2 & WE_FF2_AC_OFF);
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76e_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_EQUAL(1, q.target_blank);
    TEST_ASSERT_EQUAL(1, q.post);
    if (1) { TEST_ASSERT_NOT_NULL(strstr(q.body, "ec55=%23a7f363")); }
    else { TEST_ASSERT_NOT_NULL(strstr(q.url, "ec55=%23a7f363")); }
    l76e_done();
}

static void test_l76_ext_color_56(void) {
    we_form_req_t q;
    char html[500];
    sprintf(html, "<form action=/ec56 method=GET>"
                  "<input id=ec56 type=color name=ec56 value=#b81898 autocomplete=off>"
                  "<input type=submit formtarget=_blank></form>");
    l76e_render(html);
    TEST_ASSERT_EQUAL(WE_FORM_COLOR, g_doc.nodes[l76e_node("ec56")].form_type);
    TEST_ASSERT_TRUE(g_doc.nodes[l76e_node("ec56")].form_flags2 & WE_FF2_AC_OFF);
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76e_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_EQUAL(1, q.target_blank);
    TEST_ASSERT_EQUAL(0, q.post);
    if (0) { TEST_ASSERT_NOT_NULL(strstr(q.body, "ec56=%23b81898")); }
    else { TEST_ASSERT_NOT_NULL(strstr(q.url, "ec56=%23b81898")); }
    l76e_done();
}

static void test_l76_ext_color_57(void) {
    we_form_req_t q;
    char html[500];
    sprintf(html, "<form action=/ec57 method=POST>"
                  "<input id=ec57 type=color name=ec57 value=#c93dcd autocomplete=off>"
                  "<input type=submit formtarget=_blank></form>");
    l76e_render(html);
    TEST_ASSERT_EQUAL(WE_FORM_COLOR, g_doc.nodes[l76e_node("ec57")].form_type);
    TEST_ASSERT_TRUE(g_doc.nodes[l76e_node("ec57")].form_flags2 & WE_FF2_AC_OFF);
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76e_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_EQUAL(1, q.target_blank);
    TEST_ASSERT_EQUAL(1, q.post);
    if (1) { TEST_ASSERT_NOT_NULL(strstr(q.body, "ec57=%23c93dcd")); }
    else { TEST_ASSERT_NOT_NULL(strstr(q.url, "ec57=%23c93dcd")); }
    l76e_done();
}

static void test_l76_ext_color_58(void) {
    we_form_req_t q;
    char html[500];
    sprintf(html, "<form action=/ec58 method=GET>"
                  "<input id=ec58 type=color name=ec58 value=#da6202 autocomplete=off>"
                  "<input type=submit formtarget=_blank></form>");
    l76e_render(html);
    TEST_ASSERT_EQUAL(WE_FORM_COLOR, g_doc.nodes[l76e_node("ec58")].form_type);
    TEST_ASSERT_TRUE(g_doc.nodes[l76e_node("ec58")].form_flags2 & WE_FF2_AC_OFF);
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76e_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_EQUAL(1, q.target_blank);
    TEST_ASSERT_EQUAL(0, q.post);
    if (0) { TEST_ASSERT_NOT_NULL(strstr(q.body, "ec58=%23da6202")); }
    else { TEST_ASSERT_NOT_NULL(strstr(q.url, "ec58=%23da6202")); }
    l76e_done();
}

static void test_l76_ext_color_59(void) {
    we_form_req_t q;
    char html[500];
    sprintf(html, "<form action=/ec59 method=POST>"
                  "<input id=ec59 type=color name=ec59 value=#eb8737 autocomplete=off>"
                  "<input type=submit formtarget=_blank></form>");
    l76e_render(html);
    TEST_ASSERT_EQUAL(WE_FORM_COLOR, g_doc.nodes[l76e_node("ec59")].form_type);
    TEST_ASSERT_TRUE(g_doc.nodes[l76e_node("ec59")].form_flags2 & WE_FF2_AC_OFF);
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76e_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_EQUAL(1, q.target_blank);
    TEST_ASSERT_EQUAL(1, q.post);
    if (1) { TEST_ASSERT_NOT_NULL(strstr(q.body, "ec59=%23eb8737")); }
    else { TEST_ASSERT_NOT_NULL(strstr(q.url, "ec59=%23eb8737")); }
    l76e_done();
}

static void test_l76_ext_color_60(void) {
    we_form_req_t q;
    char html[500];
    sprintf(html, "<form action=/ec60 method=GET>"
                  "<input id=ec60 type=color name=ec60 value=#fcac6c autocomplete=off>"
                  "<input type=submit formtarget=_blank></form>");
    l76e_render(html);
    TEST_ASSERT_EQUAL(WE_FORM_COLOR, g_doc.nodes[l76e_node("ec60")].form_type);
    TEST_ASSERT_TRUE(g_doc.nodes[l76e_node("ec60")].form_flags2 & WE_FF2_AC_OFF);
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76e_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_EQUAL(1, q.target_blank);
    TEST_ASSERT_EQUAL(0, q.post);
    if (0) { TEST_ASSERT_NOT_NULL(strstr(q.body, "ec60=%23fcac6c")); }
    else { TEST_ASSERT_NOT_NULL(strstr(q.url, "ec60=%23fcac6c")); }
    l76e_done();
}

static void test_l76_ext_color_61(void) {
    we_form_req_t q;
    char html[500];
    sprintf(html, "<form action=/ec61 method=POST>"
                  "<input id=ec61 type=color name=ec61 value=#0dd1a1 autocomplete=off>"
                  "<input type=submit formtarget=_blank></form>");
    l76e_render(html);
    TEST_ASSERT_EQUAL(WE_FORM_COLOR, g_doc.nodes[l76e_node("ec61")].form_type);
    TEST_ASSERT_TRUE(g_doc.nodes[l76e_node("ec61")].form_flags2 & WE_FF2_AC_OFF);
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76e_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_EQUAL(1, q.target_blank);
    TEST_ASSERT_EQUAL(1, q.post);
    if (1) { TEST_ASSERT_NOT_NULL(strstr(q.body, "ec61=%230dd1a1")); }
    else { TEST_ASSERT_NOT_NULL(strstr(q.url, "ec61=%230dd1a1")); }
    l76e_done();
}

static void test_l76_ext_color_62(void) {
    we_form_req_t q;
    char html[500];
    sprintf(html, "<form action=/ec62 method=GET>"
                  "<input id=ec62 type=color name=ec62 value=#1ef6d6 autocomplete=off>"
                  "<input type=submit formtarget=_blank></form>");
    l76e_render(html);
    TEST_ASSERT_EQUAL(WE_FORM_COLOR, g_doc.nodes[l76e_node("ec62")].form_type);
    TEST_ASSERT_TRUE(g_doc.nodes[l76e_node("ec62")].form_flags2 & WE_FF2_AC_OFF);
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76e_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_EQUAL(1, q.target_blank);
    TEST_ASSERT_EQUAL(0, q.post);
    if (0) { TEST_ASSERT_NOT_NULL(strstr(q.body, "ec62=%231ef6d6")); }
    else { TEST_ASSERT_NOT_NULL(strstr(q.url, "ec62=%231ef6d6")); }
    l76e_done();
}

static void test_l76_ext_color_63(void) {
    we_form_req_t q;
    char html[500];
    sprintf(html, "<form action=/ec63 method=POST>"
                  "<input id=ec63 type=color name=ec63 value=#2f1b0b autocomplete=off>"
                  "<input type=submit formtarget=_blank></form>");
    l76e_render(html);
    TEST_ASSERT_EQUAL(WE_FORM_COLOR, g_doc.nodes[l76e_node("ec63")].form_type);
    TEST_ASSERT_TRUE(g_doc.nodes[l76e_node("ec63")].form_flags2 & WE_FF2_AC_OFF);
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76e_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_EQUAL(1, q.target_blank);
    TEST_ASSERT_EQUAL(1, q.post);
    if (1) { TEST_ASSERT_NOT_NULL(strstr(q.body, "ec63=%232f1b0b")); }
    else { TEST_ASSERT_NOT_NULL(strstr(q.url, "ec63=%232f1b0b")); }
    l76e_done();
}

static void test_l76_ext_color_64(void) {
    we_form_req_t q;
    char html[500];
    sprintf(html, "<form action=/ec64 method=GET>"
                  "<input id=ec64 type=color name=ec64 value=#404040 autocomplete=off>"
                  "<input type=submit formtarget=_blank></form>");
    l76e_render(html);
    TEST_ASSERT_EQUAL(WE_FORM_COLOR, g_doc.nodes[l76e_node("ec64")].form_type);
    TEST_ASSERT_TRUE(g_doc.nodes[l76e_node("ec64")].form_flags2 & WE_FF2_AC_OFF);
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76e_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_EQUAL(1, q.target_blank);
    TEST_ASSERT_EQUAL(0, q.post);
    if (0) { TEST_ASSERT_NOT_NULL(strstr(q.body, "ec64=%23404040")); }
    else { TEST_ASSERT_NOT_NULL(strstr(q.url, "ec64=%23404040")); }
    l76e_done();
}

static void test_l76_ext_color_65(void) {
    we_form_req_t q;
    char html[500];
    sprintf(html, "<form action=/ec65 method=POST>"
                  "<input id=ec65 type=color name=ec65 value=#516575 autocomplete=off>"
                  "<input type=submit formtarget=_blank></form>");
    l76e_render(html);
    TEST_ASSERT_EQUAL(WE_FORM_COLOR, g_doc.nodes[l76e_node("ec65")].form_type);
    TEST_ASSERT_TRUE(g_doc.nodes[l76e_node("ec65")].form_flags2 & WE_FF2_AC_OFF);
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76e_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_EQUAL(1, q.target_blank);
    TEST_ASSERT_EQUAL(1, q.post);
    if (1) { TEST_ASSERT_NOT_NULL(strstr(q.body, "ec65=%23516575")); }
    else { TEST_ASSERT_NOT_NULL(strstr(q.url, "ec65=%23516575")); }
    l76e_done();
}

static void test_l76_ext_color_66(void) {
    we_form_req_t q;
    char html[500];
    sprintf(html, "<form action=/ec66 method=GET>"
                  "<input id=ec66 type=color name=ec66 value=#628aaa autocomplete=off>"
                  "<input type=submit formtarget=_blank></form>");
    l76e_render(html);
    TEST_ASSERT_EQUAL(WE_FORM_COLOR, g_doc.nodes[l76e_node("ec66")].form_type);
    TEST_ASSERT_TRUE(g_doc.nodes[l76e_node("ec66")].form_flags2 & WE_FF2_AC_OFF);
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76e_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_EQUAL(1, q.target_blank);
    TEST_ASSERT_EQUAL(0, q.post);
    if (0) { TEST_ASSERT_NOT_NULL(strstr(q.body, "ec66=%23628aaa")); }
    else { TEST_ASSERT_NOT_NULL(strstr(q.url, "ec66=%23628aaa")); }
    l76e_done();
}

static void test_l76_ext_color_67(void) {
    we_form_req_t q;
    char html[500];
    sprintf(html, "<form action=/ec67 method=POST>"
                  "<input id=ec67 type=color name=ec67 value=#73afdf autocomplete=off>"
                  "<input type=submit formtarget=_blank></form>");
    l76e_render(html);
    TEST_ASSERT_EQUAL(WE_FORM_COLOR, g_doc.nodes[l76e_node("ec67")].form_type);
    TEST_ASSERT_TRUE(g_doc.nodes[l76e_node("ec67")].form_flags2 & WE_FF2_AC_OFF);
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76e_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_EQUAL(1, q.target_blank);
    TEST_ASSERT_EQUAL(1, q.post);
    if (1) { TEST_ASSERT_NOT_NULL(strstr(q.body, "ec67=%2373afdf")); }
    else { TEST_ASSERT_NOT_NULL(strstr(q.url, "ec67=%2373afdf")); }
    l76e_done();
}

static void test_l76_ext_color_68(void) {
    we_form_req_t q;
    char html[500];
    sprintf(html, "<form action=/ec68 method=GET>"
                  "<input id=ec68 type=color name=ec68 value=#84d414 autocomplete=off>"
                  "<input type=submit formtarget=_blank></form>");
    l76e_render(html);
    TEST_ASSERT_EQUAL(WE_FORM_COLOR, g_doc.nodes[l76e_node("ec68")].form_type);
    TEST_ASSERT_TRUE(g_doc.nodes[l76e_node("ec68")].form_flags2 & WE_FF2_AC_OFF);
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76e_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_EQUAL(1, q.target_blank);
    TEST_ASSERT_EQUAL(0, q.post);
    if (0) { TEST_ASSERT_NOT_NULL(strstr(q.body, "ec68=%2384d414")); }
    else { TEST_ASSERT_NOT_NULL(strstr(q.url, "ec68=%2384d414")); }
    l76e_done();
}

static void test_l76_ext_color_69(void) {
    we_form_req_t q;
    char html[500];
    sprintf(html, "<form action=/ec69 method=POST>"
                  "<input id=ec69 type=color name=ec69 value=#95f949 autocomplete=off>"
                  "<input type=submit formtarget=_blank></form>");
    l76e_render(html);
    TEST_ASSERT_EQUAL(WE_FORM_COLOR, g_doc.nodes[l76e_node("ec69")].form_type);
    TEST_ASSERT_TRUE(g_doc.nodes[l76e_node("ec69")].form_flags2 & WE_FF2_AC_OFF);
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76e_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_EQUAL(1, q.target_blank);
    TEST_ASSERT_EQUAL(1, q.post);
    if (1) { TEST_ASSERT_NOT_NULL(strstr(q.body, "ec69=%2395f949")); }
    else { TEST_ASSERT_NOT_NULL(strstr(q.url, "ec69=%2395f949")); }
    l76e_done();
}

static void test_l76_ext_color_70(void) {
    we_form_req_t q;
    char html[500];
    sprintf(html, "<form action=/ec70 method=GET>"
                  "<input id=ec70 type=color name=ec70 value=#a61e7e autocomplete=off>"
                  "<input type=submit formtarget=_blank></form>");
    l76e_render(html);
    TEST_ASSERT_EQUAL(WE_FORM_COLOR, g_doc.nodes[l76e_node("ec70")].form_type);
    TEST_ASSERT_TRUE(g_doc.nodes[l76e_node("ec70")].form_flags2 & WE_FF2_AC_OFF);
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76e_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_EQUAL(1, q.target_blank);
    TEST_ASSERT_EQUAL(0, q.post);
    if (0) { TEST_ASSERT_NOT_NULL(strstr(q.body, "ec70=%23a61e7e")); }
    else { TEST_ASSERT_NOT_NULL(strstr(q.url, "ec70=%23a61e7e")); }
    l76e_done();
}

static void test_l76_ext_color_71(void) {
    we_form_req_t q;
    char html[500];
    sprintf(html, "<form action=/ec71 method=POST>"
                  "<input id=ec71 type=color name=ec71 value=#b743b3 autocomplete=off>"
                  "<input type=submit formtarget=_blank></form>");
    l76e_render(html);
    TEST_ASSERT_EQUAL(WE_FORM_COLOR, g_doc.nodes[l76e_node("ec71")].form_type);
    TEST_ASSERT_TRUE(g_doc.nodes[l76e_node("ec71")].form_flags2 & WE_FF2_AC_OFF);
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76e_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_EQUAL(1, q.target_blank);
    TEST_ASSERT_EQUAL(1, q.post);
    if (1) { TEST_ASSERT_NOT_NULL(strstr(q.body, "ec71=%23b743b3")); }
    else { TEST_ASSERT_NOT_NULL(strstr(q.url, "ec71=%23b743b3")); }
    l76e_done();
}

static void test_l76_ext_color_72(void) {
    we_form_req_t q;
    char html[500];
    sprintf(html, "<form action=/ec72 method=GET>"
                  "<input id=ec72 type=color name=ec72 value=#c868e8 autocomplete=off>"
                  "<input type=submit formtarget=_blank></form>");
    l76e_render(html);
    TEST_ASSERT_EQUAL(WE_FORM_COLOR, g_doc.nodes[l76e_node("ec72")].form_type);
    TEST_ASSERT_TRUE(g_doc.nodes[l76e_node("ec72")].form_flags2 & WE_FF2_AC_OFF);
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76e_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_EQUAL(1, q.target_blank);
    TEST_ASSERT_EQUAL(0, q.post);
    if (0) { TEST_ASSERT_NOT_NULL(strstr(q.body, "ec72=%23c868e8")); }
    else { TEST_ASSERT_NOT_NULL(strstr(q.url, "ec72=%23c868e8")); }
    l76e_done();
}

static void test_l76_ext_color_73(void) {
    we_form_req_t q;
    char html[500];
    sprintf(html, "<form action=/ec73 method=POST>"
                  "<input id=ec73 type=color name=ec73 value=#d98d1d autocomplete=off>"
                  "<input type=submit formtarget=_blank></form>");
    l76e_render(html);
    TEST_ASSERT_EQUAL(WE_FORM_COLOR, g_doc.nodes[l76e_node("ec73")].form_type);
    TEST_ASSERT_TRUE(g_doc.nodes[l76e_node("ec73")].form_flags2 & WE_FF2_AC_OFF);
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76e_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_EQUAL(1, q.target_blank);
    TEST_ASSERT_EQUAL(1, q.post);
    if (1) { TEST_ASSERT_NOT_NULL(strstr(q.body, "ec73=%23d98d1d")); }
    else { TEST_ASSERT_NOT_NULL(strstr(q.url, "ec73=%23d98d1d")); }
    l76e_done();
}

static void test_l76_ext_color_74(void) {
    we_form_req_t q;
    char html[500];
    sprintf(html, "<form action=/ec74 method=GET>"
                  "<input id=ec74 type=color name=ec74 value=#eab252 autocomplete=off>"
                  "<input type=submit formtarget=_blank></form>");
    l76e_render(html);
    TEST_ASSERT_EQUAL(WE_FORM_COLOR, g_doc.nodes[l76e_node("ec74")].form_type);
    TEST_ASSERT_TRUE(g_doc.nodes[l76e_node("ec74")].form_flags2 & WE_FF2_AC_OFF);
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76e_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_EQUAL(1, q.target_blank);
    TEST_ASSERT_EQUAL(0, q.post);
    if (0) { TEST_ASSERT_NOT_NULL(strstr(q.body, "ec74=%23eab252")); }
    else { TEST_ASSERT_NOT_NULL(strstr(q.url, "ec74=%23eab252")); }
    l76e_done();
}

static void test_l76_ext_color_75(void) {
    we_form_req_t q;
    char html[500];
    sprintf(html, "<form action=/ec75 method=POST>"
                  "<input id=ec75 type=color name=ec75 value=#fbd787 autocomplete=off>"
                  "<input type=submit formtarget=_blank></form>");
    l76e_render(html);
    TEST_ASSERT_EQUAL(WE_FORM_COLOR, g_doc.nodes[l76e_node("ec75")].form_type);
    TEST_ASSERT_TRUE(g_doc.nodes[l76e_node("ec75")].form_flags2 & WE_FF2_AC_OFF);
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76e_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_EQUAL(1, q.target_blank);
    TEST_ASSERT_EQUAL(1, q.post);
    if (1) { TEST_ASSERT_NOT_NULL(strstr(q.body, "ec75=%23fbd787")); }
    else { TEST_ASSERT_NOT_NULL(strstr(q.url, "ec75=%23fbd787")); }
    l76e_done();
}

static void test_l76_ext_color_76(void) {
    we_form_req_t q;
    char html[500];
    sprintf(html, "<form action=/ec76 method=GET>"
                  "<input id=ec76 type=color name=ec76 value=#0cfcbc autocomplete=off>"
                  "<input type=submit formtarget=_blank></form>");
    l76e_render(html);
    TEST_ASSERT_EQUAL(WE_FORM_COLOR, g_doc.nodes[l76e_node("ec76")].form_type);
    TEST_ASSERT_TRUE(g_doc.nodes[l76e_node("ec76")].form_flags2 & WE_FF2_AC_OFF);
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76e_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_EQUAL(1, q.target_blank);
    TEST_ASSERT_EQUAL(0, q.post);
    if (0) { TEST_ASSERT_NOT_NULL(strstr(q.body, "ec76=%230cfcbc")); }
    else { TEST_ASSERT_NOT_NULL(strstr(q.url, "ec76=%230cfcbc")); }
    l76e_done();
}

static void test_l76_ext_color_77(void) {
    we_form_req_t q;
    char html[500];
    sprintf(html, "<form action=/ec77 method=POST>"
                  "<input id=ec77 type=color name=ec77 value=#1d21f1 autocomplete=off>"
                  "<input type=submit formtarget=_blank></form>");
    l76e_render(html);
    TEST_ASSERT_EQUAL(WE_FORM_COLOR, g_doc.nodes[l76e_node("ec77")].form_type);
    TEST_ASSERT_TRUE(g_doc.nodes[l76e_node("ec77")].form_flags2 & WE_FF2_AC_OFF);
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76e_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_EQUAL(1, q.target_blank);
    TEST_ASSERT_EQUAL(1, q.post);
    if (1) { TEST_ASSERT_NOT_NULL(strstr(q.body, "ec77=%231d21f1")); }
    else { TEST_ASSERT_NOT_NULL(strstr(q.url, "ec77=%231d21f1")); }
    l76e_done();
}

static void test_l76_ext_color_78(void) {
    we_form_req_t q;
    char html[500];
    sprintf(html, "<form action=/ec78 method=GET>"
                  "<input id=ec78 type=color name=ec78 value=#2e4626 autocomplete=off>"
                  "<input type=submit formtarget=_blank></form>");
    l76e_render(html);
    TEST_ASSERT_EQUAL(WE_FORM_COLOR, g_doc.nodes[l76e_node("ec78")].form_type);
    TEST_ASSERT_TRUE(g_doc.nodes[l76e_node("ec78")].form_flags2 & WE_FF2_AC_OFF);
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76e_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_EQUAL(1, q.target_blank);
    TEST_ASSERT_EQUAL(0, q.post);
    if (0) { TEST_ASSERT_NOT_NULL(strstr(q.body, "ec78=%232e4626")); }
    else { TEST_ASSERT_NOT_NULL(strstr(q.url, "ec78=%232e4626")); }
    l76e_done();
}

static void test_l76_ext_color_79(void) {
    we_form_req_t q;
    char html[500];
    sprintf(html, "<form action=/ec79 method=POST>"
                  "<input id=ec79 type=color name=ec79 value=#3f6b5b autocomplete=off>"
                  "<input type=submit formtarget=_blank></form>");
    l76e_render(html);
    TEST_ASSERT_EQUAL(WE_FORM_COLOR, g_doc.nodes[l76e_node("ec79")].form_type);
    TEST_ASSERT_TRUE(g_doc.nodes[l76e_node("ec79")].form_flags2 & WE_FF2_AC_OFF);
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76e_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_EQUAL(1, q.target_blank);
    TEST_ASSERT_EQUAL(1, q.post);
    if (1) { TEST_ASSERT_NOT_NULL(strstr(q.body, "ec79=%233f6b5b")); }
    else { TEST_ASSERT_NOT_NULL(strstr(q.url, "ec79=%233f6b5b")); }
    l76e_done();
}

static void test_l76_ext_color_80(void) {
    we_form_req_t q;
    char html[500];
    sprintf(html, "<form action=/ec80 method=GET>"
                  "<input id=ec80 type=color name=ec80 value=#509090 autocomplete=off>"
                  "<input type=submit formtarget=_blank></form>");
    l76e_render(html);
    TEST_ASSERT_EQUAL(WE_FORM_COLOR, g_doc.nodes[l76e_node("ec80")].form_type);
    TEST_ASSERT_TRUE(g_doc.nodes[l76e_node("ec80")].form_flags2 & WE_FF2_AC_OFF);
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76e_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_EQUAL(1, q.target_blank);
    TEST_ASSERT_EQUAL(0, q.post);
    if (0) { TEST_ASSERT_NOT_NULL(strstr(q.body, "ec80=%23509090")); }
    else { TEST_ASSERT_NOT_NULL(strstr(q.url, "ec80=%23509090")); }
    l76e_done();
}

static void test_l76_ext_date_1(void) {
    we_form_req_t q;
    char html[500];
    sprintf(html, "<form action=/ed1 method=GET target=_self>"
                  "<input id=ed1 type=date name=ed1 value=2001-02-02 required>"
                  "<input type=submit></form>");
    l76e_render(html);
    TEST_ASSERT_EQUAL(WE_FORM_DATE, g_doc.nodes[l76e_node("ed1")].form_type);
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76e_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_EQUAL_STRING("_self", q.target);
    TEST_ASSERT_NOT_NULL(strstr(q.url, "ed1=2001-02-02"));
    l76e_done();
}

static void test_l76_ext_date_2(void) {
    we_form_req_t q;
    char html[500];
    sprintf(html, "<form action=/ed2 method=GET target=_self>"
                  "<input id=ed2 type=date name=ed2 value=2002-03-03 required>"
                  "<input type=submit></form>");
    l76e_render(html);
    TEST_ASSERT_EQUAL(WE_FORM_DATE, g_doc.nodes[l76e_node("ed2")].form_type);
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76e_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_EQUAL_STRING("_self", q.target);
    TEST_ASSERT_NOT_NULL(strstr(q.url, "ed2=2002-03-03"));
    l76e_done();
}

static void test_l76_ext_date_3(void) {
    we_form_req_t q;
    char html[500];
    sprintf(html, "<form action=/ed3 method=GET target=_self>"
                  "<input id=ed3 type=date name=ed3 value=2003-04-04 required>"
                  "<input type=submit></form>");
    l76e_render(html);
    TEST_ASSERT_EQUAL(WE_FORM_DATE, g_doc.nodes[l76e_node("ed3")].form_type);
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76e_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_EQUAL_STRING("_self", q.target);
    TEST_ASSERT_NOT_NULL(strstr(q.url, "ed3=2003-04-04"));
    l76e_done();
}

static void test_l76_ext_date_4(void) {
    we_form_req_t q;
    char html[500];
    sprintf(html, "<form action=/ed4 method=GET target=_self>"
                  "<input id=ed4 type=date name=ed4 value=2004-05-05 required>"
                  "<input type=submit></form>");
    l76e_render(html);
    TEST_ASSERT_EQUAL(WE_FORM_DATE, g_doc.nodes[l76e_node("ed4")].form_type);
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76e_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_EQUAL_STRING("_self", q.target);
    TEST_ASSERT_NOT_NULL(strstr(q.url, "ed4=2004-05-05"));
    l76e_done();
}

static void test_l76_ext_date_5(void) {
    we_form_req_t q;
    char html[500];
    sprintf(html, "<form action=/ed5 method=GET target=_self>"
                  "<input id=ed5 type=date name=ed5 value=2005-06-06 required>"
                  "<input type=submit></form>");
    l76e_render(html);
    TEST_ASSERT_EQUAL(WE_FORM_DATE, g_doc.nodes[l76e_node("ed5")].form_type);
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76e_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_EQUAL_STRING("_self", q.target);
    TEST_ASSERT_NOT_NULL(strstr(q.url, "ed5=2005-06-06"));
    l76e_done();
}

static void test_l76_ext_date_6(void) {
    we_form_req_t q;
    char html[500];
    sprintf(html, "<form action=/ed6 method=GET target=_self>"
                  "<input id=ed6 type=date name=ed6 value=2006-07-07 required>"
                  "<input type=submit></form>");
    l76e_render(html);
    TEST_ASSERT_EQUAL(WE_FORM_DATE, g_doc.nodes[l76e_node("ed6")].form_type);
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76e_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_EQUAL_STRING("_self", q.target);
    TEST_ASSERT_NOT_NULL(strstr(q.url, "ed6=2006-07-07"));
    l76e_done();
}

static void test_l76_ext_date_7(void) {
    we_form_req_t q;
    char html[500];
    sprintf(html, "<form action=/ed7 method=GET target=_self>"
                  "<input id=ed7 type=date name=ed7 value=2007-08-08 required>"
                  "<input type=submit></form>");
    l76e_render(html);
    TEST_ASSERT_EQUAL(WE_FORM_DATE, g_doc.nodes[l76e_node("ed7")].form_type);
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76e_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_EQUAL_STRING("_self", q.target);
    TEST_ASSERT_NOT_NULL(strstr(q.url, "ed7=2007-08-08"));
    l76e_done();
}

static void test_l76_ext_date_8(void) {
    we_form_req_t q;
    char html[500];
    sprintf(html, "<form action=/ed8 method=GET target=_self>"
                  "<input id=ed8 type=date name=ed8 value=2008-09-09 required>"
                  "<input type=submit></form>");
    l76e_render(html);
    TEST_ASSERT_EQUAL(WE_FORM_DATE, g_doc.nodes[l76e_node("ed8")].form_type);
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76e_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_EQUAL_STRING("_self", q.target);
    TEST_ASSERT_NOT_NULL(strstr(q.url, "ed8=2008-09-09"));
    l76e_done();
}

static void test_l76_ext_date_9(void) {
    we_form_req_t q;
    char html[500];
    sprintf(html, "<form action=/ed9 method=GET target=_self>"
                  "<input id=ed9 type=date name=ed9 value=2009-10-10 required>"
                  "<input type=submit></form>");
    l76e_render(html);
    TEST_ASSERT_EQUAL(WE_FORM_DATE, g_doc.nodes[l76e_node("ed9")].form_type);
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76e_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_EQUAL_STRING("_self", q.target);
    TEST_ASSERT_NOT_NULL(strstr(q.url, "ed9=2009-10-10"));
    l76e_done();
}

static void test_l76_ext_date_10(void) {
    we_form_req_t q;
    char html[500];
    sprintf(html, "<form action=/ed10 method=GET target=_self>"
                  "<input id=ed10 type=date name=ed10 value=2010-11-11 required>"
                  "<input type=submit></form>");
    l76e_render(html);
    TEST_ASSERT_EQUAL(WE_FORM_DATE, g_doc.nodes[l76e_node("ed10")].form_type);
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76e_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_EQUAL_STRING("_self", q.target);
    TEST_ASSERT_NOT_NULL(strstr(q.url, "ed10=2010-11-11"));
    l76e_done();
}

static void test_l76_ext_date_11(void) {
    we_form_req_t q;
    char html[500];
    sprintf(html, "<form action=/ed11 method=GET target=_self>"
                  "<input id=ed11 type=date name=ed11 value=2011-12-12 required>"
                  "<input type=submit></form>");
    l76e_render(html);
    TEST_ASSERT_EQUAL(WE_FORM_DATE, g_doc.nodes[l76e_node("ed11")].form_type);
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76e_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_EQUAL_STRING("_self", q.target);
    TEST_ASSERT_NOT_NULL(strstr(q.url, "ed11=2011-12-12"));
    l76e_done();
}

static void test_l76_ext_date_12(void) {
    we_form_req_t q;
    char html[500];
    sprintf(html, "<form action=/ed12 method=GET target=_self>"
                  "<input id=ed12 type=date name=ed12 value=2012-01-13 required>"
                  "<input type=submit></form>");
    l76e_render(html);
    TEST_ASSERT_EQUAL(WE_FORM_DATE, g_doc.nodes[l76e_node("ed12")].form_type);
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76e_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_EQUAL_STRING("_self", q.target);
    TEST_ASSERT_NOT_NULL(strstr(q.url, "ed12=2012-01-13"));
    l76e_done();
}

static void test_l76_ext_date_13(void) {
    we_form_req_t q;
    char html[500];
    sprintf(html, "<form action=/ed13 method=GET target=_self>"
                  "<input id=ed13 type=date name=ed13 value=2013-02-14 required>"
                  "<input type=submit></form>");
    l76e_render(html);
    TEST_ASSERT_EQUAL(WE_FORM_DATE, g_doc.nodes[l76e_node("ed13")].form_type);
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76e_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_EQUAL_STRING("_self", q.target);
    TEST_ASSERT_NOT_NULL(strstr(q.url, "ed13=2013-02-14"));
    l76e_done();
}

static void test_l76_ext_date_14(void) {
    we_form_req_t q;
    char html[500];
    sprintf(html, "<form action=/ed14 method=GET target=_self>"
                  "<input id=ed14 type=date name=ed14 value=2014-03-15 required>"
                  "<input type=submit></form>");
    l76e_render(html);
    TEST_ASSERT_EQUAL(WE_FORM_DATE, g_doc.nodes[l76e_node("ed14")].form_type);
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76e_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_EQUAL_STRING("_self", q.target);
    TEST_ASSERT_NOT_NULL(strstr(q.url, "ed14=2014-03-15"));
    l76e_done();
}

static void test_l76_ext_date_15(void) {
    we_form_req_t q;
    char html[500];
    sprintf(html, "<form action=/ed15 method=GET target=_self>"
                  "<input id=ed15 type=date name=ed15 value=2015-04-16 required>"
                  "<input type=submit></form>");
    l76e_render(html);
    TEST_ASSERT_EQUAL(WE_FORM_DATE, g_doc.nodes[l76e_node("ed15")].form_type);
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76e_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_EQUAL_STRING("_self", q.target);
    TEST_ASSERT_NOT_NULL(strstr(q.url, "ed15=2015-04-16"));
    l76e_done();
}

static void test_l76_ext_date_16(void) {
    we_form_req_t q;
    char html[500];
    sprintf(html, "<form action=/ed16 method=GET target=_self>"
                  "<input id=ed16 type=date name=ed16 value=2016-05-17 required>"
                  "<input type=submit></form>");
    l76e_render(html);
    TEST_ASSERT_EQUAL(WE_FORM_DATE, g_doc.nodes[l76e_node("ed16")].form_type);
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76e_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_EQUAL_STRING("_self", q.target);
    TEST_ASSERT_NOT_NULL(strstr(q.url, "ed16=2016-05-17"));
    l76e_done();
}

static void test_l76_ext_date_17(void) {
    we_form_req_t q;
    char html[500];
    sprintf(html, "<form action=/ed17 method=GET target=_self>"
                  "<input id=ed17 type=date name=ed17 value=2017-06-18 required>"
                  "<input type=submit></form>");
    l76e_render(html);
    TEST_ASSERT_EQUAL(WE_FORM_DATE, g_doc.nodes[l76e_node("ed17")].form_type);
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76e_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_EQUAL_STRING("_self", q.target);
    TEST_ASSERT_NOT_NULL(strstr(q.url, "ed17=2017-06-18"));
    l76e_done();
}

static void test_l76_ext_date_18(void) {
    we_form_req_t q;
    char html[500];
    sprintf(html, "<form action=/ed18 method=GET target=_self>"
                  "<input id=ed18 type=date name=ed18 value=2018-07-19 required>"
                  "<input type=submit></form>");
    l76e_render(html);
    TEST_ASSERT_EQUAL(WE_FORM_DATE, g_doc.nodes[l76e_node("ed18")].form_type);
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76e_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_EQUAL_STRING("_self", q.target);
    TEST_ASSERT_NOT_NULL(strstr(q.url, "ed18=2018-07-19"));
    l76e_done();
}

static void test_l76_ext_date_19(void) {
    we_form_req_t q;
    char html[500];
    sprintf(html, "<form action=/ed19 method=GET target=_self>"
                  "<input id=ed19 type=date name=ed19 value=2019-08-20 required>"
                  "<input type=submit></form>");
    l76e_render(html);
    TEST_ASSERT_EQUAL(WE_FORM_DATE, g_doc.nodes[l76e_node("ed19")].form_type);
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76e_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_EQUAL_STRING("_self", q.target);
    TEST_ASSERT_NOT_NULL(strstr(q.url, "ed19=2019-08-20"));
    l76e_done();
}

static void test_l76_ext_date_20(void) {
    we_form_req_t q;
    char html[500];
    sprintf(html, "<form action=/ed20 method=GET target=_self>"
                  "<input id=ed20 type=date name=ed20 value=2020-09-21 required>"
                  "<input type=submit></form>");
    l76e_render(html);
    TEST_ASSERT_EQUAL(WE_FORM_DATE, g_doc.nodes[l76e_node("ed20")].form_type);
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76e_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_EQUAL_STRING("_self", q.target);
    TEST_ASSERT_NOT_NULL(strstr(q.url, "ed20=2020-09-21"));
    l76e_done();
}

static void test_l76_ext_date_21(void) {
    we_form_req_t q;
    char html[500];
    sprintf(html, "<form action=/ed21 method=GET target=_self>"
                  "<input id=ed21 type=date name=ed21 value=2021-10-22 required>"
                  "<input type=submit></form>");
    l76e_render(html);
    TEST_ASSERT_EQUAL(WE_FORM_DATE, g_doc.nodes[l76e_node("ed21")].form_type);
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76e_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_EQUAL_STRING("_self", q.target);
    TEST_ASSERT_NOT_NULL(strstr(q.url, "ed21=2021-10-22"));
    l76e_done();
}

static void test_l76_ext_date_22(void) {
    we_form_req_t q;
    char html[500];
    sprintf(html, "<form action=/ed22 method=GET target=_self>"
                  "<input id=ed22 type=date name=ed22 value=2022-11-23 required>"
                  "<input type=submit></form>");
    l76e_render(html);
    TEST_ASSERT_EQUAL(WE_FORM_DATE, g_doc.nodes[l76e_node("ed22")].form_type);
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76e_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_EQUAL_STRING("_self", q.target);
    TEST_ASSERT_NOT_NULL(strstr(q.url, "ed22=2022-11-23"));
    l76e_done();
}

static void test_l76_ext_date_23(void) {
    we_form_req_t q;
    char html[500];
    sprintf(html, "<form action=/ed23 method=GET target=_self>"
                  "<input id=ed23 type=date name=ed23 value=2023-12-24 required>"
                  "<input type=submit></form>");
    l76e_render(html);
    TEST_ASSERT_EQUAL(WE_FORM_DATE, g_doc.nodes[l76e_node("ed23")].form_type);
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76e_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_EQUAL_STRING("_self", q.target);
    TEST_ASSERT_NOT_NULL(strstr(q.url, "ed23=2023-12-24"));
    l76e_done();
}

static void test_l76_ext_date_24(void) {
    we_form_req_t q;
    char html[500];
    sprintf(html, "<form action=/ed24 method=GET target=_self>"
                  "<input id=ed24 type=date name=ed24 value=2024-01-25 required>"
                  "<input type=submit></form>");
    l76e_render(html);
    TEST_ASSERT_EQUAL(WE_FORM_DATE, g_doc.nodes[l76e_node("ed24")].form_type);
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76e_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_EQUAL_STRING("_self", q.target);
    TEST_ASSERT_NOT_NULL(strstr(q.url, "ed24=2024-01-25"));
    l76e_done();
}

static void test_l76_ext_date_25(void) {
    we_form_req_t q;
    char html[500];
    sprintf(html, "<form action=/ed25 method=GET target=_self>"
                  "<input id=ed25 type=date name=ed25 value=2000-02-26 required>"
                  "<input type=submit></form>");
    l76e_render(html);
    TEST_ASSERT_EQUAL(WE_FORM_DATE, g_doc.nodes[l76e_node("ed25")].form_type);
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76e_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_EQUAL_STRING("_self", q.target);
    TEST_ASSERT_NOT_NULL(strstr(q.url, "ed25=2000-02-26"));
    l76e_done();
}

static void test_l76_ext_date_26(void) {
    we_form_req_t q;
    char html[500];
    sprintf(html, "<form action=/ed26 method=GET target=_self>"
                  "<input id=ed26 type=date name=ed26 value=2001-03-27 required>"
                  "<input type=submit></form>");
    l76e_render(html);
    TEST_ASSERT_EQUAL(WE_FORM_DATE, g_doc.nodes[l76e_node("ed26")].form_type);
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76e_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_EQUAL_STRING("_self", q.target);
    TEST_ASSERT_NOT_NULL(strstr(q.url, "ed26=2001-03-27"));
    l76e_done();
}

static void test_l76_ext_date_27(void) {
    we_form_req_t q;
    char html[500];
    sprintf(html, "<form action=/ed27 method=GET target=_self>"
                  "<input id=ed27 type=date name=ed27 value=2002-04-28 required>"
                  "<input type=submit></form>");
    l76e_render(html);
    TEST_ASSERT_EQUAL(WE_FORM_DATE, g_doc.nodes[l76e_node("ed27")].form_type);
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76e_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_EQUAL_STRING("_self", q.target);
    TEST_ASSERT_NOT_NULL(strstr(q.url, "ed27=2002-04-28"));
    l76e_done();
}

static void test_l76_ext_date_28(void) {
    we_form_req_t q;
    char html[500];
    sprintf(html, "<form action=/ed28 method=GET target=_self>"
                  "<input id=ed28 type=date name=ed28 value=2003-05-01 required>"
                  "<input type=submit></form>");
    l76e_render(html);
    TEST_ASSERT_EQUAL(WE_FORM_DATE, g_doc.nodes[l76e_node("ed28")].form_type);
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76e_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_EQUAL_STRING("_self", q.target);
    TEST_ASSERT_NOT_NULL(strstr(q.url, "ed28=2003-05-01"));
    l76e_done();
}

static void test_l76_ext_date_29(void) {
    we_form_req_t q;
    char html[500];
    sprintf(html, "<form action=/ed29 method=GET target=_self>"
                  "<input id=ed29 type=date name=ed29 value=2004-06-02 required>"
                  "<input type=submit></form>");
    l76e_render(html);
    TEST_ASSERT_EQUAL(WE_FORM_DATE, g_doc.nodes[l76e_node("ed29")].form_type);
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76e_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_EQUAL_STRING("_self", q.target);
    TEST_ASSERT_NOT_NULL(strstr(q.url, "ed29=2004-06-02"));
    l76e_done();
}

static void test_l76_ext_date_30(void) {
    we_form_req_t q;
    char html[500];
    sprintf(html, "<form action=/ed30 method=GET target=_self>"
                  "<input id=ed30 type=date name=ed30 value=2005-07-03 required>"
                  "<input type=submit></form>");
    l76e_render(html);
    TEST_ASSERT_EQUAL(WE_FORM_DATE, g_doc.nodes[l76e_node("ed30")].form_type);
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76e_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_EQUAL_STRING("_self", q.target);
    TEST_ASSERT_NOT_NULL(strstr(q.url, "ed30=2005-07-03"));
    l76e_done();
}

static void test_l76_ext_date_31(void) {
    we_form_req_t q;
    char html[500];
    sprintf(html, "<form action=/ed31 method=GET target=_self>"
                  "<input id=ed31 type=date name=ed31 value=2006-08-04 required>"
                  "<input type=submit></form>");
    l76e_render(html);
    TEST_ASSERT_EQUAL(WE_FORM_DATE, g_doc.nodes[l76e_node("ed31")].form_type);
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76e_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_EQUAL_STRING("_self", q.target);
    TEST_ASSERT_NOT_NULL(strstr(q.url, "ed31=2006-08-04"));
    l76e_done();
}

static void test_l76_ext_date_32(void) {
    we_form_req_t q;
    char html[500];
    sprintf(html, "<form action=/ed32 method=GET target=_self>"
                  "<input id=ed32 type=date name=ed32 value=2007-09-05 required>"
                  "<input type=submit></form>");
    l76e_render(html);
    TEST_ASSERT_EQUAL(WE_FORM_DATE, g_doc.nodes[l76e_node("ed32")].form_type);
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76e_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_EQUAL_STRING("_self", q.target);
    TEST_ASSERT_NOT_NULL(strstr(q.url, "ed32=2007-09-05"));
    l76e_done();
}

static void test_l76_ext_date_33(void) {
    we_form_req_t q;
    char html[500];
    sprintf(html, "<form action=/ed33 method=GET target=_self>"
                  "<input id=ed33 type=date name=ed33 value=2008-10-06 required>"
                  "<input type=submit></form>");
    l76e_render(html);
    TEST_ASSERT_EQUAL(WE_FORM_DATE, g_doc.nodes[l76e_node("ed33")].form_type);
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76e_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_EQUAL_STRING("_self", q.target);
    TEST_ASSERT_NOT_NULL(strstr(q.url, "ed33=2008-10-06"));
    l76e_done();
}

static void test_l76_ext_date_34(void) {
    we_form_req_t q;
    char html[500];
    sprintf(html, "<form action=/ed34 method=GET target=_self>"
                  "<input id=ed34 type=date name=ed34 value=2009-11-07 required>"
                  "<input type=submit></form>");
    l76e_render(html);
    TEST_ASSERT_EQUAL(WE_FORM_DATE, g_doc.nodes[l76e_node("ed34")].form_type);
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76e_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_EQUAL_STRING("_self", q.target);
    TEST_ASSERT_NOT_NULL(strstr(q.url, "ed34=2009-11-07"));
    l76e_done();
}

static void test_l76_ext_date_35(void) {
    we_form_req_t q;
    char html[500];
    sprintf(html, "<form action=/ed35 method=GET target=_self>"
                  "<input id=ed35 type=date name=ed35 value=2010-12-08 required>"
                  "<input type=submit></form>");
    l76e_render(html);
    TEST_ASSERT_EQUAL(WE_FORM_DATE, g_doc.nodes[l76e_node("ed35")].form_type);
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76e_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_EQUAL_STRING("_self", q.target);
    TEST_ASSERT_NOT_NULL(strstr(q.url, "ed35=2010-12-08"));
    l76e_done();
}

static void test_l76_ext_date_36(void) {
    we_form_req_t q;
    char html[500];
    sprintf(html, "<form action=/ed36 method=GET target=_self>"
                  "<input id=ed36 type=date name=ed36 value=2011-01-09 required>"
                  "<input type=submit></form>");
    l76e_render(html);
    TEST_ASSERT_EQUAL(WE_FORM_DATE, g_doc.nodes[l76e_node("ed36")].form_type);
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76e_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_EQUAL_STRING("_self", q.target);
    TEST_ASSERT_NOT_NULL(strstr(q.url, "ed36=2011-01-09"));
    l76e_done();
}

static void test_l76_ext_date_37(void) {
    we_form_req_t q;
    char html[500];
    sprintf(html, "<form action=/ed37 method=GET target=_self>"
                  "<input id=ed37 type=date name=ed37 value=2012-02-10 required>"
                  "<input type=submit></form>");
    l76e_render(html);
    TEST_ASSERT_EQUAL(WE_FORM_DATE, g_doc.nodes[l76e_node("ed37")].form_type);
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76e_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_EQUAL_STRING("_self", q.target);
    TEST_ASSERT_NOT_NULL(strstr(q.url, "ed37=2012-02-10"));
    l76e_done();
}

static void test_l76_ext_date_38(void) {
    we_form_req_t q;
    char html[500];
    sprintf(html, "<form action=/ed38 method=GET target=_self>"
                  "<input id=ed38 type=date name=ed38 value=2013-03-11 required>"
                  "<input type=submit></form>");
    l76e_render(html);
    TEST_ASSERT_EQUAL(WE_FORM_DATE, g_doc.nodes[l76e_node("ed38")].form_type);
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76e_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_EQUAL_STRING("_self", q.target);
    TEST_ASSERT_NOT_NULL(strstr(q.url, "ed38=2013-03-11"));
    l76e_done();
}

static void test_l76_ext_date_39(void) {
    we_form_req_t q;
    char html[500];
    sprintf(html, "<form action=/ed39 method=GET target=_self>"
                  "<input id=ed39 type=date name=ed39 value=2014-04-12 required>"
                  "<input type=submit></form>");
    l76e_render(html);
    TEST_ASSERT_EQUAL(WE_FORM_DATE, g_doc.nodes[l76e_node("ed39")].form_type);
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76e_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_EQUAL_STRING("_self", q.target);
    TEST_ASSERT_NOT_NULL(strstr(q.url, "ed39=2014-04-12"));
    l76e_done();
}

static void test_l76_ext_date_40(void) {
    we_form_req_t q;
    char html[500];
    sprintf(html, "<form action=/ed40 method=GET target=_self>"
                  "<input id=ed40 type=date name=ed40 value=2015-05-13 required>"
                  "<input type=submit></form>");
    l76e_render(html);
    TEST_ASSERT_EQUAL(WE_FORM_DATE, g_doc.nodes[l76e_node("ed40")].form_type);
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76e_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_EQUAL_STRING("_self", q.target);
    TEST_ASSERT_NOT_NULL(strstr(q.url, "ed40=2015-05-13"));
    l76e_done();
}

static void test_l76_ext_date_41(void) {
    we_form_req_t q;
    char html[500];
    sprintf(html, "<form action=/ed41 method=GET target=_self>"
                  "<input id=ed41 type=date name=ed41 value=2016-06-14 required>"
                  "<input type=submit></form>");
    l76e_render(html);
    TEST_ASSERT_EQUAL(WE_FORM_DATE, g_doc.nodes[l76e_node("ed41")].form_type);
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76e_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_EQUAL_STRING("_self", q.target);
    TEST_ASSERT_NOT_NULL(strstr(q.url, "ed41=2016-06-14"));
    l76e_done();
}

static void test_l76_ext_date_42(void) {
    we_form_req_t q;
    char html[500];
    sprintf(html, "<form action=/ed42 method=GET target=_self>"
                  "<input id=ed42 type=date name=ed42 value=2017-07-15 required>"
                  "<input type=submit></form>");
    l76e_render(html);
    TEST_ASSERT_EQUAL(WE_FORM_DATE, g_doc.nodes[l76e_node("ed42")].form_type);
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76e_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_EQUAL_STRING("_self", q.target);
    TEST_ASSERT_NOT_NULL(strstr(q.url, "ed42=2017-07-15"));
    l76e_done();
}

static void test_l76_ext_date_43(void) {
    we_form_req_t q;
    char html[500];
    sprintf(html, "<form action=/ed43 method=GET target=_self>"
                  "<input id=ed43 type=date name=ed43 value=2018-08-16 required>"
                  "<input type=submit></form>");
    l76e_render(html);
    TEST_ASSERT_EQUAL(WE_FORM_DATE, g_doc.nodes[l76e_node("ed43")].form_type);
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76e_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_EQUAL_STRING("_self", q.target);
    TEST_ASSERT_NOT_NULL(strstr(q.url, "ed43=2018-08-16"));
    l76e_done();
}

static void test_l76_ext_date_44(void) {
    we_form_req_t q;
    char html[500];
    sprintf(html, "<form action=/ed44 method=GET target=_self>"
                  "<input id=ed44 type=date name=ed44 value=2019-09-17 required>"
                  "<input type=submit></form>");
    l76e_render(html);
    TEST_ASSERT_EQUAL(WE_FORM_DATE, g_doc.nodes[l76e_node("ed44")].form_type);
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76e_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_EQUAL_STRING("_self", q.target);
    TEST_ASSERT_NOT_NULL(strstr(q.url, "ed44=2019-09-17"));
    l76e_done();
}

static void test_l76_ext_date_45(void) {
    we_form_req_t q;
    char html[500];
    sprintf(html, "<form action=/ed45 method=GET target=_self>"
                  "<input id=ed45 type=date name=ed45 value=2020-10-18 required>"
                  "<input type=submit></form>");
    l76e_render(html);
    TEST_ASSERT_EQUAL(WE_FORM_DATE, g_doc.nodes[l76e_node("ed45")].form_type);
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76e_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_EQUAL_STRING("_self", q.target);
    TEST_ASSERT_NOT_NULL(strstr(q.url, "ed45=2020-10-18"));
    l76e_done();
}

static void test_l76_ext_date_46(void) {
    we_form_req_t q;
    char html[500];
    sprintf(html, "<form action=/ed46 method=GET target=_self>"
                  "<input id=ed46 type=date name=ed46 value=2021-11-19 required>"
                  "<input type=submit></form>");
    l76e_render(html);
    TEST_ASSERT_EQUAL(WE_FORM_DATE, g_doc.nodes[l76e_node("ed46")].form_type);
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76e_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_EQUAL_STRING("_self", q.target);
    TEST_ASSERT_NOT_NULL(strstr(q.url, "ed46=2021-11-19"));
    l76e_done();
}

static void test_l76_ext_date_47(void) {
    we_form_req_t q;
    char html[500];
    sprintf(html, "<form action=/ed47 method=GET target=_self>"
                  "<input id=ed47 type=date name=ed47 value=2022-12-20 required>"
                  "<input type=submit></form>");
    l76e_render(html);
    TEST_ASSERT_EQUAL(WE_FORM_DATE, g_doc.nodes[l76e_node("ed47")].form_type);
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76e_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_EQUAL_STRING("_self", q.target);
    TEST_ASSERT_NOT_NULL(strstr(q.url, "ed47=2022-12-20"));
    l76e_done();
}

static void test_l76_ext_date_48(void) {
    we_form_req_t q;
    char html[500];
    sprintf(html, "<form action=/ed48 method=GET target=_self>"
                  "<input id=ed48 type=date name=ed48 value=2023-01-21 required>"
                  "<input type=submit></form>");
    l76e_render(html);
    TEST_ASSERT_EQUAL(WE_FORM_DATE, g_doc.nodes[l76e_node("ed48")].form_type);
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76e_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_EQUAL_STRING("_self", q.target);
    TEST_ASSERT_NOT_NULL(strstr(q.url, "ed48=2023-01-21"));
    l76e_done();
}

static void test_l76_ext_date_49(void) {
    we_form_req_t q;
    char html[500];
    sprintf(html, "<form action=/ed49 method=GET target=_self>"
                  "<input id=ed49 type=date name=ed49 value=2024-02-22 required>"
                  "<input type=submit></form>");
    l76e_render(html);
    TEST_ASSERT_EQUAL(WE_FORM_DATE, g_doc.nodes[l76e_node("ed49")].form_type);
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76e_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_EQUAL_STRING("_self", q.target);
    TEST_ASSERT_NOT_NULL(strstr(q.url, "ed49=2024-02-22"));
    l76e_done();
}

static void test_l76_ext_date_50(void) {
    we_form_req_t q;
    char html[500];
    sprintf(html, "<form action=/ed50 method=GET target=_self>"
                  "<input id=ed50 type=date name=ed50 value=2000-03-23 required>"
                  "<input type=submit></form>");
    l76e_render(html);
    TEST_ASSERT_EQUAL(WE_FORM_DATE, g_doc.nodes[l76e_node("ed50")].form_type);
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76e_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_EQUAL_STRING("_self", q.target);
    TEST_ASSERT_NOT_NULL(strstr(q.url, "ed50=2000-03-23"));
    l76e_done();
}

static void test_l76_ext_date_51(void) {
    we_form_req_t q;
    char html[500];
    sprintf(html, "<form action=/ed51 method=GET target=_self>"
                  "<input id=ed51 type=date name=ed51 value=2001-04-24 required>"
                  "<input type=submit></form>");
    l76e_render(html);
    TEST_ASSERT_EQUAL(WE_FORM_DATE, g_doc.nodes[l76e_node("ed51")].form_type);
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76e_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_EQUAL_STRING("_self", q.target);
    TEST_ASSERT_NOT_NULL(strstr(q.url, "ed51=2001-04-24"));
    l76e_done();
}

static void test_l76_ext_date_52(void) {
    we_form_req_t q;
    char html[500];
    sprintf(html, "<form action=/ed52 method=GET target=_self>"
                  "<input id=ed52 type=date name=ed52 value=2002-05-25 required>"
                  "<input type=submit></form>");
    l76e_render(html);
    TEST_ASSERT_EQUAL(WE_FORM_DATE, g_doc.nodes[l76e_node("ed52")].form_type);
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76e_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_EQUAL_STRING("_self", q.target);
    TEST_ASSERT_NOT_NULL(strstr(q.url, "ed52=2002-05-25"));
    l76e_done();
}

static void test_l76_ext_date_53(void) {
    we_form_req_t q;
    char html[500];
    sprintf(html, "<form action=/ed53 method=GET target=_self>"
                  "<input id=ed53 type=date name=ed53 value=2003-06-26 required>"
                  "<input type=submit></form>");
    l76e_render(html);
    TEST_ASSERT_EQUAL(WE_FORM_DATE, g_doc.nodes[l76e_node("ed53")].form_type);
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76e_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_EQUAL_STRING("_self", q.target);
    TEST_ASSERT_NOT_NULL(strstr(q.url, "ed53=2003-06-26"));
    l76e_done();
}

static void test_l76_ext_date_54(void) {
    we_form_req_t q;
    char html[500];
    sprintf(html, "<form action=/ed54 method=GET target=_self>"
                  "<input id=ed54 type=date name=ed54 value=2004-07-27 required>"
                  "<input type=submit></form>");
    l76e_render(html);
    TEST_ASSERT_EQUAL(WE_FORM_DATE, g_doc.nodes[l76e_node("ed54")].form_type);
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76e_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_EQUAL_STRING("_self", q.target);
    TEST_ASSERT_NOT_NULL(strstr(q.url, "ed54=2004-07-27"));
    l76e_done();
}

static void test_l76_ext_date_55(void) {
    we_form_req_t q;
    char html[500];
    sprintf(html, "<form action=/ed55 method=GET target=_self>"
                  "<input id=ed55 type=date name=ed55 value=2005-08-28 required>"
                  "<input type=submit></form>");
    l76e_render(html);
    TEST_ASSERT_EQUAL(WE_FORM_DATE, g_doc.nodes[l76e_node("ed55")].form_type);
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76e_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_EQUAL_STRING("_self", q.target);
    TEST_ASSERT_NOT_NULL(strstr(q.url, "ed55=2005-08-28"));
    l76e_done();
}

static void test_l76_ext_date_56(void) {
    we_form_req_t q;
    char html[500];
    sprintf(html, "<form action=/ed56 method=GET target=_self>"
                  "<input id=ed56 type=date name=ed56 value=2006-09-01 required>"
                  "<input type=submit></form>");
    l76e_render(html);
    TEST_ASSERT_EQUAL(WE_FORM_DATE, g_doc.nodes[l76e_node("ed56")].form_type);
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76e_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_EQUAL_STRING("_self", q.target);
    TEST_ASSERT_NOT_NULL(strstr(q.url, "ed56=2006-09-01"));
    l76e_done();
}

static void test_l76_ext_date_57(void) {
    we_form_req_t q;
    char html[500];
    sprintf(html, "<form action=/ed57 method=GET target=_self>"
                  "<input id=ed57 type=date name=ed57 value=2007-10-02 required>"
                  "<input type=submit></form>");
    l76e_render(html);
    TEST_ASSERT_EQUAL(WE_FORM_DATE, g_doc.nodes[l76e_node("ed57")].form_type);
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76e_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_EQUAL_STRING("_self", q.target);
    TEST_ASSERT_NOT_NULL(strstr(q.url, "ed57=2007-10-02"));
    l76e_done();
}

static void test_l76_ext_date_58(void) {
    we_form_req_t q;
    char html[500];
    sprintf(html, "<form action=/ed58 method=GET target=_self>"
                  "<input id=ed58 type=date name=ed58 value=2008-11-03 required>"
                  "<input type=submit></form>");
    l76e_render(html);
    TEST_ASSERT_EQUAL(WE_FORM_DATE, g_doc.nodes[l76e_node("ed58")].form_type);
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76e_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_EQUAL_STRING("_self", q.target);
    TEST_ASSERT_NOT_NULL(strstr(q.url, "ed58=2008-11-03"));
    l76e_done();
}

static void test_l76_ext_date_59(void) {
    we_form_req_t q;
    char html[500];
    sprintf(html, "<form action=/ed59 method=GET target=_self>"
                  "<input id=ed59 type=date name=ed59 value=2009-12-04 required>"
                  "<input type=submit></form>");
    l76e_render(html);
    TEST_ASSERT_EQUAL(WE_FORM_DATE, g_doc.nodes[l76e_node("ed59")].form_type);
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76e_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_EQUAL_STRING("_self", q.target);
    TEST_ASSERT_NOT_NULL(strstr(q.url, "ed59=2009-12-04"));
    l76e_done();
}

static void test_l76_ext_date_60(void) {
    we_form_req_t q;
    char html[500];
    sprintf(html, "<form action=/ed60 method=GET target=_self>"
                  "<input id=ed60 type=date name=ed60 value=2010-01-05 required>"
                  "<input type=submit></form>");
    l76e_render(html);
    TEST_ASSERT_EQUAL(WE_FORM_DATE, g_doc.nodes[l76e_node("ed60")].form_type);
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76e_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_EQUAL_STRING("_self", q.target);
    TEST_ASSERT_NOT_NULL(strstr(q.url, "ed60=2010-01-05"));
    l76e_done();
}

static void test_l76_ext_date_61(void) {
    we_form_req_t q;
    char html[500];
    sprintf(html, "<form action=/ed61 method=GET target=_self>"
                  "<input id=ed61 type=date name=ed61 value=2011-02-06 required>"
                  "<input type=submit></form>");
    l76e_render(html);
    TEST_ASSERT_EQUAL(WE_FORM_DATE, g_doc.nodes[l76e_node("ed61")].form_type);
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76e_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_EQUAL_STRING("_self", q.target);
    TEST_ASSERT_NOT_NULL(strstr(q.url, "ed61=2011-02-06"));
    l76e_done();
}

static void test_l76_ext_date_62(void) {
    we_form_req_t q;
    char html[500];
    sprintf(html, "<form action=/ed62 method=GET target=_self>"
                  "<input id=ed62 type=date name=ed62 value=2012-03-07 required>"
                  "<input type=submit></form>");
    l76e_render(html);
    TEST_ASSERT_EQUAL(WE_FORM_DATE, g_doc.nodes[l76e_node("ed62")].form_type);
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76e_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_EQUAL_STRING("_self", q.target);
    TEST_ASSERT_NOT_NULL(strstr(q.url, "ed62=2012-03-07"));
    l76e_done();
}

static void test_l76_ext_date_63(void) {
    we_form_req_t q;
    char html[500];
    sprintf(html, "<form action=/ed63 method=GET target=_self>"
                  "<input id=ed63 type=date name=ed63 value=2013-04-08 required>"
                  "<input type=submit></form>");
    l76e_render(html);
    TEST_ASSERT_EQUAL(WE_FORM_DATE, g_doc.nodes[l76e_node("ed63")].form_type);
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76e_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_EQUAL_STRING("_self", q.target);
    TEST_ASSERT_NOT_NULL(strstr(q.url, "ed63=2013-04-08"));
    l76e_done();
}

static void test_l76_ext_date_64(void) {
    we_form_req_t q;
    char html[500];
    sprintf(html, "<form action=/ed64 method=GET target=_self>"
                  "<input id=ed64 type=date name=ed64 value=2014-05-09 required>"
                  "<input type=submit></form>");
    l76e_render(html);
    TEST_ASSERT_EQUAL(WE_FORM_DATE, g_doc.nodes[l76e_node("ed64")].form_type);
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76e_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_EQUAL_STRING("_self", q.target);
    TEST_ASSERT_NOT_NULL(strstr(q.url, "ed64=2014-05-09"));
    l76e_done();
}

static void test_l76_ext_date_65(void) {
    we_form_req_t q;
    char html[500];
    sprintf(html, "<form action=/ed65 method=GET target=_self>"
                  "<input id=ed65 type=date name=ed65 value=2015-06-10 required>"
                  "<input type=submit></form>");
    l76e_render(html);
    TEST_ASSERT_EQUAL(WE_FORM_DATE, g_doc.nodes[l76e_node("ed65")].form_type);
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76e_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_EQUAL_STRING("_self", q.target);
    TEST_ASSERT_NOT_NULL(strstr(q.url, "ed65=2015-06-10"));
    l76e_done();
}

static void test_l76_ext_date_66(void) {
    we_form_req_t q;
    char html[500];
    sprintf(html, "<form action=/ed66 method=GET target=_self>"
                  "<input id=ed66 type=date name=ed66 value=2016-07-11 required>"
                  "<input type=submit></form>");
    l76e_render(html);
    TEST_ASSERT_EQUAL(WE_FORM_DATE, g_doc.nodes[l76e_node("ed66")].form_type);
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76e_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_EQUAL_STRING("_self", q.target);
    TEST_ASSERT_NOT_NULL(strstr(q.url, "ed66=2016-07-11"));
    l76e_done();
}

static void test_l76_ext_date_67(void) {
    we_form_req_t q;
    char html[500];
    sprintf(html, "<form action=/ed67 method=GET target=_self>"
                  "<input id=ed67 type=date name=ed67 value=2017-08-12 required>"
                  "<input type=submit></form>");
    l76e_render(html);
    TEST_ASSERT_EQUAL(WE_FORM_DATE, g_doc.nodes[l76e_node("ed67")].form_type);
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76e_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_EQUAL_STRING("_self", q.target);
    TEST_ASSERT_NOT_NULL(strstr(q.url, "ed67=2017-08-12"));
    l76e_done();
}

static void test_l76_ext_date_68(void) {
    we_form_req_t q;
    char html[500];
    sprintf(html, "<form action=/ed68 method=GET target=_self>"
                  "<input id=ed68 type=date name=ed68 value=2018-09-13 required>"
                  "<input type=submit></form>");
    l76e_render(html);
    TEST_ASSERT_EQUAL(WE_FORM_DATE, g_doc.nodes[l76e_node("ed68")].form_type);
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76e_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_EQUAL_STRING("_self", q.target);
    TEST_ASSERT_NOT_NULL(strstr(q.url, "ed68=2018-09-13"));
    l76e_done();
}

static void test_l76_ext_date_69(void) {
    we_form_req_t q;
    char html[500];
    sprintf(html, "<form action=/ed69 method=GET target=_self>"
                  "<input id=ed69 type=date name=ed69 value=2019-10-14 required>"
                  "<input type=submit></form>");
    l76e_render(html);
    TEST_ASSERT_EQUAL(WE_FORM_DATE, g_doc.nodes[l76e_node("ed69")].form_type);
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76e_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_EQUAL_STRING("_self", q.target);
    TEST_ASSERT_NOT_NULL(strstr(q.url, "ed69=2019-10-14"));
    l76e_done();
}

static void test_l76_ext_date_70(void) {
    we_form_req_t q;
    char html[500];
    sprintf(html, "<form action=/ed70 method=GET target=_self>"
                  "<input id=ed70 type=date name=ed70 value=2020-11-15 required>"
                  "<input type=submit></form>");
    l76e_render(html);
    TEST_ASSERT_EQUAL(WE_FORM_DATE, g_doc.nodes[l76e_node("ed70")].form_type);
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76e_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_EQUAL_STRING("_self", q.target);
    TEST_ASSERT_NOT_NULL(strstr(q.url, "ed70=2020-11-15"));
    l76e_done();
}

static void test_l76_ext_date_71(void) {
    we_form_req_t q;
    char html[500];
    sprintf(html, "<form action=/ed71 method=GET target=_self>"
                  "<input id=ed71 type=date name=ed71 value=2021-12-16 required>"
                  "<input type=submit></form>");
    l76e_render(html);
    TEST_ASSERT_EQUAL(WE_FORM_DATE, g_doc.nodes[l76e_node("ed71")].form_type);
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76e_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_EQUAL_STRING("_self", q.target);
    TEST_ASSERT_NOT_NULL(strstr(q.url, "ed71=2021-12-16"));
    l76e_done();
}

static void test_l76_ext_date_72(void) {
    we_form_req_t q;
    char html[500];
    sprintf(html, "<form action=/ed72 method=GET target=_self>"
                  "<input id=ed72 type=date name=ed72 value=2022-01-17 required>"
                  "<input type=submit></form>");
    l76e_render(html);
    TEST_ASSERT_EQUAL(WE_FORM_DATE, g_doc.nodes[l76e_node("ed72")].form_type);
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76e_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_EQUAL_STRING("_self", q.target);
    TEST_ASSERT_NOT_NULL(strstr(q.url, "ed72=2022-01-17"));
    l76e_done();
}

static void test_l76_ext_date_73(void) {
    we_form_req_t q;
    char html[500];
    sprintf(html, "<form action=/ed73 method=GET target=_self>"
                  "<input id=ed73 type=date name=ed73 value=2023-02-18 required>"
                  "<input type=submit></form>");
    l76e_render(html);
    TEST_ASSERT_EQUAL(WE_FORM_DATE, g_doc.nodes[l76e_node("ed73")].form_type);
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76e_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_EQUAL_STRING("_self", q.target);
    TEST_ASSERT_NOT_NULL(strstr(q.url, "ed73=2023-02-18"));
    l76e_done();
}

static void test_l76_ext_date_74(void) {
    we_form_req_t q;
    char html[500];
    sprintf(html, "<form action=/ed74 method=GET target=_self>"
                  "<input id=ed74 type=date name=ed74 value=2024-03-19 required>"
                  "<input type=submit></form>");
    l76e_render(html);
    TEST_ASSERT_EQUAL(WE_FORM_DATE, g_doc.nodes[l76e_node("ed74")].form_type);
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76e_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_EQUAL_STRING("_self", q.target);
    TEST_ASSERT_NOT_NULL(strstr(q.url, "ed74=2024-03-19"));
    l76e_done();
}

static void test_l76_ext_date_75(void) {
    we_form_req_t q;
    char html[500];
    sprintf(html, "<form action=/ed75 method=GET target=_self>"
                  "<input id=ed75 type=date name=ed75 value=2000-04-20 required>"
                  "<input type=submit></form>");
    l76e_render(html);
    TEST_ASSERT_EQUAL(WE_FORM_DATE, g_doc.nodes[l76e_node("ed75")].form_type);
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76e_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_EQUAL_STRING("_self", q.target);
    TEST_ASSERT_NOT_NULL(strstr(q.url, "ed75=2000-04-20"));
    l76e_done();
}

static void test_l76_ext_date_76(void) {
    we_form_req_t q;
    char html[500];
    sprintf(html, "<form action=/ed76 method=GET target=_self>"
                  "<input id=ed76 type=date name=ed76 value=2001-05-21 required>"
                  "<input type=submit></form>");
    l76e_render(html);
    TEST_ASSERT_EQUAL(WE_FORM_DATE, g_doc.nodes[l76e_node("ed76")].form_type);
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76e_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_EQUAL_STRING("_self", q.target);
    TEST_ASSERT_NOT_NULL(strstr(q.url, "ed76=2001-05-21"));
    l76e_done();
}

static void test_l76_ext_date_77(void) {
    we_form_req_t q;
    char html[500];
    sprintf(html, "<form action=/ed77 method=GET target=_self>"
                  "<input id=ed77 type=date name=ed77 value=2002-06-22 required>"
                  "<input type=submit></form>");
    l76e_render(html);
    TEST_ASSERT_EQUAL(WE_FORM_DATE, g_doc.nodes[l76e_node("ed77")].form_type);
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76e_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_EQUAL_STRING("_self", q.target);
    TEST_ASSERT_NOT_NULL(strstr(q.url, "ed77=2002-06-22"));
    l76e_done();
}

static void test_l76_ext_date_78(void) {
    we_form_req_t q;
    char html[500];
    sprintf(html, "<form action=/ed78 method=GET target=_self>"
                  "<input id=ed78 type=date name=ed78 value=2003-07-23 required>"
                  "<input type=submit></form>");
    l76e_render(html);
    TEST_ASSERT_EQUAL(WE_FORM_DATE, g_doc.nodes[l76e_node("ed78")].form_type);
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76e_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_EQUAL_STRING("_self", q.target);
    TEST_ASSERT_NOT_NULL(strstr(q.url, "ed78=2003-07-23"));
    l76e_done();
}

static void test_l76_ext_date_79(void) {
    we_form_req_t q;
    char html[500];
    sprintf(html, "<form action=/ed79 method=GET target=_self>"
                  "<input id=ed79 type=date name=ed79 value=2004-08-24 required>"
                  "<input type=submit></form>");
    l76e_render(html);
    TEST_ASSERT_EQUAL(WE_FORM_DATE, g_doc.nodes[l76e_node("ed79")].form_type);
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76e_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_EQUAL_STRING("_self", q.target);
    TEST_ASSERT_NOT_NULL(strstr(q.url, "ed79=2004-08-24"));
    l76e_done();
}

static void test_l76_ext_date_80(void) {
    we_form_req_t q;
    char html[500];
    sprintf(html, "<form action=/ed80 method=GET target=_self>"
                  "<input id=ed80 type=date name=ed80 value=2005-09-25 required>"
                  "<input type=submit></form>");
    l76e_render(html);
    TEST_ASSERT_EQUAL(WE_FORM_DATE, g_doc.nodes[l76e_node("ed80")].form_type);
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76e_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_EQUAL_STRING("_self", q.target);
    TEST_ASSERT_NOT_NULL(strstr(q.url, "ed80=2005-09-25"));
    l76e_done();
}

static void test_l76_ext_attr_1(void) {
    char html[700];
    sprintf(html, "<style>"
                  "a#t1::after{content:attr(title)}"
                  "button#b1::before{content:attr(value) '/' attr(name)}"
                  "</style>"
                  "<a id=t1 title=T1>x</a>"
                  "<button id=b1 name=M1 value=V1>Go</button>"
                  "<input id=p1 name=P1 placeholder=PH1>");
    l76e_render(html);
    TEST_ASSERT_NOT_NULL(find_text("T1"));
    TEST_ASSERT_NOT_NULL(find_text("V1/M1"));
    TEST_ASSERT_TRUE(g_doc.nodes[l76e_node("p1")].fph_len > 0);
    l76e_done();
}

static void test_l76_ext_attr_2(void) {
    char html[700];
    sprintf(html, "<style>"
                  "a#t2::after{content:attr(title)}"
                  "button#b2::before{content:attr(value) '/' attr(name)}"
                  "</style>"
                  "<a id=t2 title=T2>x</a>"
                  "<button id=b2 name=M2 value=V2>Go</button>"
                  "<input id=p2 name=P2 placeholder=PH2>");
    l76e_render(html);
    TEST_ASSERT_NOT_NULL(find_text("T2"));
    TEST_ASSERT_NOT_NULL(find_text("V2/M2"));
    TEST_ASSERT_TRUE(g_doc.nodes[l76e_node("p2")].fph_len > 0);
    l76e_done();
}

static void test_l76_ext_attr_3(void) {
    char html[700];
    sprintf(html, "<style>"
                  "a#t3::after{content:attr(title)}"
                  "button#b3::before{content:attr(value) '/' attr(name)}"
                  "</style>"
                  "<a id=t3 title=T3>x</a>"
                  "<button id=b3 name=M3 value=V3>Go</button>"
                  "<input id=p3 name=P3 placeholder=PH3>");
    l76e_render(html);
    TEST_ASSERT_NOT_NULL(find_text("T3"));
    TEST_ASSERT_NOT_NULL(find_text("V3/M3"));
    TEST_ASSERT_TRUE(g_doc.nodes[l76e_node("p3")].fph_len > 0);
    l76e_done();
}

static void test_l76_ext_attr_4(void) {
    char html[700];
    sprintf(html, "<style>"
                  "a#t4::after{content:attr(title)}"
                  "button#b4::before{content:attr(value) '/' attr(name)}"
                  "</style>"
                  "<a id=t4 title=T4>x</a>"
                  "<button id=b4 name=M4 value=V4>Go</button>"
                  "<input id=p4 name=P4 placeholder=PH4>");
    l76e_render(html);
    TEST_ASSERT_NOT_NULL(find_text("T4"));
    TEST_ASSERT_NOT_NULL(find_text("V4/M4"));
    TEST_ASSERT_TRUE(g_doc.nodes[l76e_node("p4")].fph_len > 0);
    l76e_done();
}

static void test_l76_ext_attr_5(void) {
    char html[700];
    sprintf(html, "<style>"
                  "a#t5::after{content:attr(title)}"
                  "button#b5::before{content:attr(value) '/' attr(name)}"
                  "</style>"
                  "<a id=t5 title=T5>x</a>"
                  "<button id=b5 name=M5 value=V5>Go</button>"
                  "<input id=p5 name=P5 placeholder=PH5>");
    l76e_render(html);
    TEST_ASSERT_NOT_NULL(find_text("T5"));
    TEST_ASSERT_NOT_NULL(find_text("V5/M5"));
    TEST_ASSERT_TRUE(g_doc.nodes[l76e_node("p5")].fph_len > 0);
    l76e_done();
}

static void test_l76_ext_attr_6(void) {
    char html[700];
    sprintf(html, "<style>"
                  "a#t6::after{content:attr(title)}"
                  "button#b6::before{content:attr(value) '/' attr(name)}"
                  "</style>"
                  "<a id=t6 title=T6>x</a>"
                  "<button id=b6 name=M6 value=V6>Go</button>"
                  "<input id=p6 name=P6 placeholder=PH6>");
    l76e_render(html);
    TEST_ASSERT_NOT_NULL(find_text("T6"));
    TEST_ASSERT_NOT_NULL(find_text("V6/M6"));
    TEST_ASSERT_TRUE(g_doc.nodes[l76e_node("p6")].fph_len > 0);
    l76e_done();
}

static void test_l76_ext_attr_7(void) {
    char html[700];
    sprintf(html, "<style>"
                  "a#t7::after{content:attr(title)}"
                  "button#b7::before{content:attr(value) '/' attr(name)}"
                  "</style>"
                  "<a id=t7 title=T7>x</a>"
                  "<button id=b7 name=M7 value=V7>Go</button>"
                  "<input id=p7 name=P7 placeholder=PH7>");
    l76e_render(html);
    TEST_ASSERT_NOT_NULL(find_text("T7"));
    TEST_ASSERT_NOT_NULL(find_text("V7/M7"));
    TEST_ASSERT_TRUE(g_doc.nodes[l76e_node("p7")].fph_len > 0);
    l76e_done();
}

static void test_l76_ext_attr_8(void) {
    char html[700];
    sprintf(html, "<style>"
                  "a#t8::after{content:attr(title)}"
                  "button#b8::before{content:attr(value) '/' attr(name)}"
                  "</style>"
                  "<a id=t8 title=T8>x</a>"
                  "<button id=b8 name=M8 value=V8>Go</button>"
                  "<input id=p8 name=P8 placeholder=PH8>");
    l76e_render(html);
    TEST_ASSERT_NOT_NULL(find_text("T8"));
    TEST_ASSERT_NOT_NULL(find_text("V8/M8"));
    TEST_ASSERT_TRUE(g_doc.nodes[l76e_node("p8")].fph_len > 0);
    l76e_done();
}

static void test_l76_ext_attr_9(void) {
    char html[700];
    sprintf(html, "<style>"
                  "a#t9::after{content:attr(title)}"
                  "button#b9::before{content:attr(value) '/' attr(name)}"
                  "</style>"
                  "<a id=t9 title=T9>x</a>"
                  "<button id=b9 name=M9 value=V9>Go</button>"
                  "<input id=p9 name=P9 placeholder=PH9>");
    l76e_render(html);
    TEST_ASSERT_NOT_NULL(find_text("T9"));
    TEST_ASSERT_NOT_NULL(find_text("V9/M9"));
    TEST_ASSERT_TRUE(g_doc.nodes[l76e_node("p9")].fph_len > 0);
    l76e_done();
}

static void test_l76_ext_attr_10(void) {
    char html[700];
    sprintf(html, "<style>"
                  "a#t10::after{content:attr(title)}"
                  "button#b10::before{content:attr(value) '/' attr(name)}"
                  "</style>"
                  "<a id=t10 title=T10>x</a>"
                  "<button id=b10 name=M10 value=V10>Go</button>"
                  "<input id=p10 name=P10 placeholder=PH10>");
    l76e_render(html);
    TEST_ASSERT_NOT_NULL(find_text("T10"));
    TEST_ASSERT_NOT_NULL(find_text("V10/M10"));
    TEST_ASSERT_TRUE(g_doc.nodes[l76e_node("p10")].fph_len > 0);
    l76e_done();
}

static void test_l76_ext_attr_11(void) {
    char html[700];
    sprintf(html, "<style>"
                  "a#t11::after{content:attr(title)}"
                  "button#b11::before{content:attr(value) '/' attr(name)}"
                  "</style>"
                  "<a id=t11 title=T11>x</a>"
                  "<button id=b11 name=M11 value=V11>Go</button>"
                  "<input id=p11 name=P11 placeholder=PH11>");
    l76e_render(html);
    TEST_ASSERT_NOT_NULL(find_text("T11"));
    TEST_ASSERT_NOT_NULL(find_text("V11/M11"));
    TEST_ASSERT_TRUE(g_doc.nodes[l76e_node("p11")].fph_len > 0);
    l76e_done();
}

static void test_l76_ext_attr_12(void) {
    char html[700];
    sprintf(html, "<style>"
                  "a#t12::after{content:attr(title)}"
                  "button#b12::before{content:attr(value) '/' attr(name)}"
                  "</style>"
                  "<a id=t12 title=T12>x</a>"
                  "<button id=b12 name=M12 value=V12>Go</button>"
                  "<input id=p12 name=P12 placeholder=PH12>");
    l76e_render(html);
    TEST_ASSERT_NOT_NULL(find_text("T12"));
    TEST_ASSERT_NOT_NULL(find_text("V12/M12"));
    TEST_ASSERT_TRUE(g_doc.nodes[l76e_node("p12")].fph_len > 0);
    l76e_done();
}

static void test_l76_ext_attr_13(void) {
    char html[700];
    sprintf(html, "<style>"
                  "a#t13::after{content:attr(title)}"
                  "button#b13::before{content:attr(value) '/' attr(name)}"
                  "</style>"
                  "<a id=t13 title=T13>x</a>"
                  "<button id=b13 name=M13 value=V13>Go</button>"
                  "<input id=p13 name=P13 placeholder=PH13>");
    l76e_render(html);
    TEST_ASSERT_NOT_NULL(find_text("T13"));
    TEST_ASSERT_NOT_NULL(find_text("V13/M13"));
    TEST_ASSERT_TRUE(g_doc.nodes[l76e_node("p13")].fph_len > 0);
    l76e_done();
}

static void test_l76_ext_attr_14(void) {
    char html[700];
    sprintf(html, "<style>"
                  "a#t14::after{content:attr(title)}"
                  "button#b14::before{content:attr(value) '/' attr(name)}"
                  "</style>"
                  "<a id=t14 title=T14>x</a>"
                  "<button id=b14 name=M14 value=V14>Go</button>"
                  "<input id=p14 name=P14 placeholder=PH14>");
    l76e_render(html);
    TEST_ASSERT_NOT_NULL(find_text("T14"));
    TEST_ASSERT_NOT_NULL(find_text("V14/M14"));
    TEST_ASSERT_TRUE(g_doc.nodes[l76e_node("p14")].fph_len > 0);
    l76e_done();
}

static void test_l76_ext_attr_15(void) {
    char html[700];
    sprintf(html, "<style>"
                  "a#t15::after{content:attr(title)}"
                  "button#b15::before{content:attr(value) '/' attr(name)}"
                  "</style>"
                  "<a id=t15 title=T15>x</a>"
                  "<button id=b15 name=M15 value=V15>Go</button>"
                  "<input id=p15 name=P15 placeholder=PH15>");
    l76e_render(html);
    TEST_ASSERT_NOT_NULL(find_text("T15"));
    TEST_ASSERT_NOT_NULL(find_text("V15/M15"));
    TEST_ASSERT_TRUE(g_doc.nodes[l76e_node("p15")].fph_len > 0);
    l76e_done();
}

static void test_l76_ext_attr_16(void) {
    char html[700];
    sprintf(html, "<style>"
                  "a#t16::after{content:attr(title)}"
                  "button#b16::before{content:attr(value) '/' attr(name)}"
                  "</style>"
                  "<a id=t16 title=T16>x</a>"
                  "<button id=b16 name=M16 value=V16>Go</button>"
                  "<input id=p16 name=P16 placeholder=PH16>");
    l76e_render(html);
    TEST_ASSERT_NOT_NULL(find_text("T16"));
    TEST_ASSERT_NOT_NULL(find_text("V16/M16"));
    TEST_ASSERT_TRUE(g_doc.nodes[l76e_node("p16")].fph_len > 0);
    l76e_done();
}

static void test_l76_ext_attr_17(void) {
    char html[700];
    sprintf(html, "<style>"
                  "a#t17::after{content:attr(title)}"
                  "button#b17::before{content:attr(value) '/' attr(name)}"
                  "</style>"
                  "<a id=t17 title=T17>x</a>"
                  "<button id=b17 name=M17 value=V17>Go</button>"
                  "<input id=p17 name=P17 placeholder=PH17>");
    l76e_render(html);
    TEST_ASSERT_NOT_NULL(find_text("T17"));
    TEST_ASSERT_NOT_NULL(find_text("V17/M17"));
    TEST_ASSERT_TRUE(g_doc.nodes[l76e_node("p17")].fph_len > 0);
    l76e_done();
}

static void test_l76_ext_attr_18(void) {
    char html[700];
    sprintf(html, "<style>"
                  "a#t18::after{content:attr(title)}"
                  "button#b18::before{content:attr(value) '/' attr(name)}"
                  "</style>"
                  "<a id=t18 title=T18>x</a>"
                  "<button id=b18 name=M18 value=V18>Go</button>"
                  "<input id=p18 name=P18 placeholder=PH18>");
    l76e_render(html);
    TEST_ASSERT_NOT_NULL(find_text("T18"));
    TEST_ASSERT_NOT_NULL(find_text("V18/M18"));
    TEST_ASSERT_TRUE(g_doc.nodes[l76e_node("p18")].fph_len > 0);
    l76e_done();
}

static void test_l76_ext_attr_19(void) {
    char html[700];
    sprintf(html, "<style>"
                  "a#t19::after{content:attr(title)}"
                  "button#b19::before{content:attr(value) '/' attr(name)}"
                  "</style>"
                  "<a id=t19 title=T19>x</a>"
                  "<button id=b19 name=M19 value=V19>Go</button>"
                  "<input id=p19 name=P19 placeholder=PH19>");
    l76e_render(html);
    TEST_ASSERT_NOT_NULL(find_text("T19"));
    TEST_ASSERT_NOT_NULL(find_text("V19/M19"));
    TEST_ASSERT_TRUE(g_doc.nodes[l76e_node("p19")].fph_len > 0);
    l76e_done();
}

static void test_l76_ext_attr_20(void) {
    char html[700];
    sprintf(html, "<style>"
                  "a#t20::after{content:attr(title)}"
                  "button#b20::before{content:attr(value) '/' attr(name)}"
                  "</style>"
                  "<a id=t20 title=T20>x</a>"
                  "<button id=b20 name=M20 value=V20>Go</button>"
                  "<input id=p20 name=P20 placeholder=PH20>");
    l76e_render(html);
    TEST_ASSERT_NOT_NULL(find_text("T20"));
    TEST_ASSERT_NOT_NULL(find_text("V20/M20"));
    TEST_ASSERT_TRUE(g_doc.nodes[l76e_node("p20")].fph_len > 0);
    l76e_done();
}

static void test_l76_ext_attr_21(void) {
    char html[700];
    sprintf(html, "<style>"
                  "a#t21::after{content:attr(title)}"
                  "button#b21::before{content:attr(value) '/' attr(name)}"
                  "</style>"
                  "<a id=t21 title=T21>x</a>"
                  "<button id=b21 name=M21 value=V21>Go</button>"
                  "<input id=p21 name=P21 placeholder=PH21>");
    l76e_render(html);
    TEST_ASSERT_NOT_NULL(find_text("T21"));
    TEST_ASSERT_NOT_NULL(find_text("V21/M21"));
    TEST_ASSERT_TRUE(g_doc.nodes[l76e_node("p21")].fph_len > 0);
    l76e_done();
}

static void test_l76_ext_attr_22(void) {
    char html[700];
    sprintf(html, "<style>"
                  "a#t22::after{content:attr(title)}"
                  "button#b22::before{content:attr(value) '/' attr(name)}"
                  "</style>"
                  "<a id=t22 title=T22>x</a>"
                  "<button id=b22 name=M22 value=V22>Go</button>"
                  "<input id=p22 name=P22 placeholder=PH22>");
    l76e_render(html);
    TEST_ASSERT_NOT_NULL(find_text("T22"));
    TEST_ASSERT_NOT_NULL(find_text("V22/M22"));
    TEST_ASSERT_TRUE(g_doc.nodes[l76e_node("p22")].fph_len > 0);
    l76e_done();
}

static void test_l76_ext_attr_23(void) {
    char html[700];
    sprintf(html, "<style>"
                  "a#t23::after{content:attr(title)}"
                  "button#b23::before{content:attr(value) '/' attr(name)}"
                  "</style>"
                  "<a id=t23 title=T23>x</a>"
                  "<button id=b23 name=M23 value=V23>Go</button>"
                  "<input id=p23 name=P23 placeholder=PH23>");
    l76e_render(html);
    TEST_ASSERT_NOT_NULL(find_text("T23"));
    TEST_ASSERT_NOT_NULL(find_text("V23/M23"));
    TEST_ASSERT_TRUE(g_doc.nodes[l76e_node("p23")].fph_len > 0);
    l76e_done();
}

static void test_l76_ext_attr_24(void) {
    char html[700];
    sprintf(html, "<style>"
                  "a#t24::after{content:attr(title)}"
                  "button#b24::before{content:attr(value) '/' attr(name)}"
                  "</style>"
                  "<a id=t24 title=T24>x</a>"
                  "<button id=b24 name=M24 value=V24>Go</button>"
                  "<input id=p24 name=P24 placeholder=PH24>");
    l76e_render(html);
    TEST_ASSERT_NOT_NULL(find_text("T24"));
    TEST_ASSERT_NOT_NULL(find_text("V24/M24"));
    TEST_ASSERT_TRUE(g_doc.nodes[l76e_node("p24")].fph_len > 0);
    l76e_done();
}

static void test_l76_ext_attr_25(void) {
    char html[700];
    sprintf(html, "<style>"
                  "a#t25::after{content:attr(title)}"
                  "button#b25::before{content:attr(value) '/' attr(name)}"
                  "</style>"
                  "<a id=t25 title=T25>x</a>"
                  "<button id=b25 name=M25 value=V25>Go</button>"
                  "<input id=p25 name=P25 placeholder=PH25>");
    l76e_render(html);
    TEST_ASSERT_NOT_NULL(find_text("T25"));
    TEST_ASSERT_NOT_NULL(find_text("V25/M25"));
    TEST_ASSERT_TRUE(g_doc.nodes[l76e_node("p25")].fph_len > 0);
    l76e_done();
}

static void test_l76_ext_attr_26(void) {
    char html[700];
    sprintf(html, "<style>"
                  "a#t26::after{content:attr(title)}"
                  "button#b26::before{content:attr(value) '/' attr(name)}"
                  "</style>"
                  "<a id=t26 title=T26>x</a>"
                  "<button id=b26 name=M26 value=V26>Go</button>"
                  "<input id=p26 name=P26 placeholder=PH26>");
    l76e_render(html);
    TEST_ASSERT_NOT_NULL(find_text("T26"));
    TEST_ASSERT_NOT_NULL(find_text("V26/M26"));
    TEST_ASSERT_TRUE(g_doc.nodes[l76e_node("p26")].fph_len > 0);
    l76e_done();
}

static void test_l76_ext_attr_27(void) {
    char html[700];
    sprintf(html, "<style>"
                  "a#t27::after{content:attr(title)}"
                  "button#b27::before{content:attr(value) '/' attr(name)}"
                  "</style>"
                  "<a id=t27 title=T27>x</a>"
                  "<button id=b27 name=M27 value=V27>Go</button>"
                  "<input id=p27 name=P27 placeholder=PH27>");
    l76e_render(html);
    TEST_ASSERT_NOT_NULL(find_text("T27"));
    TEST_ASSERT_NOT_NULL(find_text("V27/M27"));
    TEST_ASSERT_TRUE(g_doc.nodes[l76e_node("p27")].fph_len > 0);
    l76e_done();
}

static void test_l76_ext_attr_28(void) {
    char html[700];
    sprintf(html, "<style>"
                  "a#t28::after{content:attr(title)}"
                  "button#b28::before{content:attr(value) '/' attr(name)}"
                  "</style>"
                  "<a id=t28 title=T28>x</a>"
                  "<button id=b28 name=M28 value=V28>Go</button>"
                  "<input id=p28 name=P28 placeholder=PH28>");
    l76e_render(html);
    TEST_ASSERT_NOT_NULL(find_text("T28"));
    TEST_ASSERT_NOT_NULL(find_text("V28/M28"));
    TEST_ASSERT_TRUE(g_doc.nodes[l76e_node("p28")].fph_len > 0);
    l76e_done();
}

static void test_l76_ext_attr_29(void) {
    char html[700];
    sprintf(html, "<style>"
                  "a#t29::after{content:attr(title)}"
                  "button#b29::before{content:attr(value) '/' attr(name)}"
                  "</style>"
                  "<a id=t29 title=T29>x</a>"
                  "<button id=b29 name=M29 value=V29>Go</button>"
                  "<input id=p29 name=P29 placeholder=PH29>");
    l76e_render(html);
    TEST_ASSERT_NOT_NULL(find_text("T29"));
    TEST_ASSERT_NOT_NULL(find_text("V29/M29"));
    TEST_ASSERT_TRUE(g_doc.nodes[l76e_node("p29")].fph_len > 0);
    l76e_done();
}

static void test_l76_ext_attr_30(void) {
    char html[700];
    sprintf(html, "<style>"
                  "a#t30::after{content:attr(title)}"
                  "button#b30::before{content:attr(value) '/' attr(name)}"
                  "</style>"
                  "<a id=t30 title=T30>x</a>"
                  "<button id=b30 name=M30 value=V30>Go</button>"
                  "<input id=p30 name=P30 placeholder=PH30>");
    l76e_render(html);
    TEST_ASSERT_NOT_NULL(find_text("T30"));
    TEST_ASSERT_NOT_NULL(find_text("V30/M30"));
    TEST_ASSERT_TRUE(g_doc.nodes[l76e_node("p30")].fph_len > 0);
    l76e_done();
}

static void test_l76_ext_attr_31(void) {
    char html[700];
    sprintf(html, "<style>"
                  "a#t31::after{content:attr(title)}"
                  "button#b31::before{content:attr(value) '/' attr(name)}"
                  "</style>"
                  "<a id=t31 title=T31>x</a>"
                  "<button id=b31 name=M31 value=V31>Go</button>"
                  "<input id=p31 name=P31 placeholder=PH31>");
    l76e_render(html);
    TEST_ASSERT_NOT_NULL(find_text("T31"));
    TEST_ASSERT_NOT_NULL(find_text("V31/M31"));
    TEST_ASSERT_TRUE(g_doc.nodes[l76e_node("p31")].fph_len > 0);
    l76e_done();
}

static void test_l76_ext_attr_32(void) {
    char html[700];
    sprintf(html, "<style>"
                  "a#t32::after{content:attr(title)}"
                  "button#b32::before{content:attr(value) '/' attr(name)}"
                  "</style>"
                  "<a id=t32 title=T32>x</a>"
                  "<button id=b32 name=M32 value=V32>Go</button>"
                  "<input id=p32 name=P32 placeholder=PH32>");
    l76e_render(html);
    TEST_ASSERT_NOT_NULL(find_text("T32"));
    TEST_ASSERT_NOT_NULL(find_text("V32/M32"));
    TEST_ASSERT_TRUE(g_doc.nodes[l76e_node("p32")].fph_len > 0);
    l76e_done();
}

static void test_l76_ext_attr_33(void) {
    char html[700];
    sprintf(html, "<style>"
                  "a#t33::after{content:attr(title)}"
                  "button#b33::before{content:attr(value) '/' attr(name)}"
                  "</style>"
                  "<a id=t33 title=T33>x</a>"
                  "<button id=b33 name=M33 value=V33>Go</button>"
                  "<input id=p33 name=P33 placeholder=PH33>");
    l76e_render(html);
    TEST_ASSERT_NOT_NULL(find_text("T33"));
    TEST_ASSERT_NOT_NULL(find_text("V33/M33"));
    TEST_ASSERT_TRUE(g_doc.nodes[l76e_node("p33")].fph_len > 0);
    l76e_done();
}

static void test_l76_ext_attr_34(void) {
    char html[700];
    sprintf(html, "<style>"
                  "a#t34::after{content:attr(title)}"
                  "button#b34::before{content:attr(value) '/' attr(name)}"
                  "</style>"
                  "<a id=t34 title=T34>x</a>"
                  "<button id=b34 name=M34 value=V34>Go</button>"
                  "<input id=p34 name=P34 placeholder=PH34>");
    l76e_render(html);
    TEST_ASSERT_NOT_NULL(find_text("T34"));
    TEST_ASSERT_NOT_NULL(find_text("V34/M34"));
    TEST_ASSERT_TRUE(g_doc.nodes[l76e_node("p34")].fph_len > 0);
    l76e_done();
}

static void test_l76_ext_attr_35(void) {
    char html[700];
    sprintf(html, "<style>"
                  "a#t35::after{content:attr(title)}"
                  "button#b35::before{content:attr(value) '/' attr(name)}"
                  "</style>"
                  "<a id=t35 title=T35>x</a>"
                  "<button id=b35 name=M35 value=V35>Go</button>"
                  "<input id=p35 name=P35 placeholder=PH35>");
    l76e_render(html);
    TEST_ASSERT_NOT_NULL(find_text("T35"));
    TEST_ASSERT_NOT_NULL(find_text("V35/M35"));
    TEST_ASSERT_TRUE(g_doc.nodes[l76e_node("p35")].fph_len > 0);
    l76e_done();
}

static void test_l76_ext_attr_36(void) {
    char html[700];
    sprintf(html, "<style>"
                  "a#t36::after{content:attr(title)}"
                  "button#b36::before{content:attr(value) '/' attr(name)}"
                  "</style>"
                  "<a id=t36 title=T36>x</a>"
                  "<button id=b36 name=M36 value=V36>Go</button>"
                  "<input id=p36 name=P36 placeholder=PH36>");
    l76e_render(html);
    TEST_ASSERT_NOT_NULL(find_text("T36"));
    TEST_ASSERT_NOT_NULL(find_text("V36/M36"));
    TEST_ASSERT_TRUE(g_doc.nodes[l76e_node("p36")].fph_len > 0);
    l76e_done();
}

static void test_l76_ext_attr_37(void) {
    char html[700];
    sprintf(html, "<style>"
                  "a#t37::after{content:attr(title)}"
                  "button#b37::before{content:attr(value) '/' attr(name)}"
                  "</style>"
                  "<a id=t37 title=T37>x</a>"
                  "<button id=b37 name=M37 value=V37>Go</button>"
                  "<input id=p37 name=P37 placeholder=PH37>");
    l76e_render(html);
    TEST_ASSERT_NOT_NULL(find_text("T37"));
    TEST_ASSERT_NOT_NULL(find_text("V37/M37"));
    TEST_ASSERT_TRUE(g_doc.nodes[l76e_node("p37")].fph_len > 0);
    l76e_done();
}

static void test_l76_ext_attr_38(void) {
    char html[700];
    sprintf(html, "<style>"
                  "a#t38::after{content:attr(title)}"
                  "button#b38::before{content:attr(value) '/' attr(name)}"
                  "</style>"
                  "<a id=t38 title=T38>x</a>"
                  "<button id=b38 name=M38 value=V38>Go</button>"
                  "<input id=p38 name=P38 placeholder=PH38>");
    l76e_render(html);
    TEST_ASSERT_NOT_NULL(find_text("T38"));
    TEST_ASSERT_NOT_NULL(find_text("V38/M38"));
    TEST_ASSERT_TRUE(g_doc.nodes[l76e_node("p38")].fph_len > 0);
    l76e_done();
}

static void test_l76_ext_attr_39(void) {
    char html[700];
    sprintf(html, "<style>"
                  "a#t39::after{content:attr(title)}"
                  "button#b39::before{content:attr(value) '/' attr(name)}"
                  "</style>"
                  "<a id=t39 title=T39>x</a>"
                  "<button id=b39 name=M39 value=V39>Go</button>"
                  "<input id=p39 name=P39 placeholder=PH39>");
    l76e_render(html);
    TEST_ASSERT_NOT_NULL(find_text("T39"));
    TEST_ASSERT_NOT_NULL(find_text("V39/M39"));
    TEST_ASSERT_TRUE(g_doc.nodes[l76e_node("p39")].fph_len > 0);
    l76e_done();
}

static void test_l76_ext_attr_40(void) {
    char html[700];
    sprintf(html, "<style>"
                  "a#t40::after{content:attr(title)}"
                  "button#b40::before{content:attr(value) '/' attr(name)}"
                  "</style>"
                  "<a id=t40 title=T40>x</a>"
                  "<button id=b40 name=M40 value=V40>Go</button>"
                  "<input id=p40 name=P40 placeholder=PH40>");
    l76e_render(html);
    TEST_ASSERT_NOT_NULL(find_text("T40"));
    TEST_ASSERT_NOT_NULL(find_text("V40/M40"));
    TEST_ASSERT_TRUE(g_doc.nodes[l76e_node("p40")].fph_len > 0);
    l76e_done();
}

static void test_l76_ext_attr_41(void) {
    char html[700];
    sprintf(html, "<style>"
                  "a#t41::after{content:attr(title)}"
                  "button#b41::before{content:attr(value) '/' attr(name)}"
                  "</style>"
                  "<a id=t41 title=T41>x</a>"
                  "<button id=b41 name=M41 value=V41>Go</button>"
                  "<input id=p41 name=P41 placeholder=PH41>");
    l76e_render(html);
    TEST_ASSERT_NOT_NULL(find_text("T41"));
    TEST_ASSERT_NOT_NULL(find_text("V41/M41"));
    TEST_ASSERT_TRUE(g_doc.nodes[l76e_node("p41")].fph_len > 0);
    l76e_done();
}

static void test_l76_ext_attr_42(void) {
    char html[700];
    sprintf(html, "<style>"
                  "a#t42::after{content:attr(title)}"
                  "button#b42::before{content:attr(value) '/' attr(name)}"
                  "</style>"
                  "<a id=t42 title=T42>x</a>"
                  "<button id=b42 name=M42 value=V42>Go</button>"
                  "<input id=p42 name=P42 placeholder=PH42>");
    l76e_render(html);
    TEST_ASSERT_NOT_NULL(find_text("T42"));
    TEST_ASSERT_NOT_NULL(find_text("V42/M42"));
    TEST_ASSERT_TRUE(g_doc.nodes[l76e_node("p42")].fph_len > 0);
    l76e_done();
}

static void test_l76_ext_attr_43(void) {
    char html[700];
    sprintf(html, "<style>"
                  "a#t43::after{content:attr(title)}"
                  "button#b43::before{content:attr(value) '/' attr(name)}"
                  "</style>"
                  "<a id=t43 title=T43>x</a>"
                  "<button id=b43 name=M43 value=V43>Go</button>"
                  "<input id=p43 name=P43 placeholder=PH43>");
    l76e_render(html);
    TEST_ASSERT_NOT_NULL(find_text("T43"));
    TEST_ASSERT_NOT_NULL(find_text("V43/M43"));
    TEST_ASSERT_TRUE(g_doc.nodes[l76e_node("p43")].fph_len > 0);
    l76e_done();
}

static void test_l76_ext_attr_44(void) {
    char html[700];
    sprintf(html, "<style>"
                  "a#t44::after{content:attr(title)}"
                  "button#b44::before{content:attr(value) '/' attr(name)}"
                  "</style>"
                  "<a id=t44 title=T44>x</a>"
                  "<button id=b44 name=M44 value=V44>Go</button>"
                  "<input id=p44 name=P44 placeholder=PH44>");
    l76e_render(html);
    TEST_ASSERT_NOT_NULL(find_text("T44"));
    TEST_ASSERT_NOT_NULL(find_text("V44/M44"));
    TEST_ASSERT_TRUE(g_doc.nodes[l76e_node("p44")].fph_len > 0);
    l76e_done();
}

static void test_l76_ext_attr_45(void) {
    char html[700];
    sprintf(html, "<style>"
                  "a#t45::after{content:attr(title)}"
                  "button#b45::before{content:attr(value) '/' attr(name)}"
                  "</style>"
                  "<a id=t45 title=T45>x</a>"
                  "<button id=b45 name=M45 value=V45>Go</button>"
                  "<input id=p45 name=P45 placeholder=PH45>");
    l76e_render(html);
    TEST_ASSERT_NOT_NULL(find_text("T45"));
    TEST_ASSERT_NOT_NULL(find_text("V45/M45"));
    TEST_ASSERT_TRUE(g_doc.nodes[l76e_node("p45")].fph_len > 0);
    l76e_done();
}

static void test_l76_ext_attr_46(void) {
    char html[700];
    sprintf(html, "<style>"
                  "a#t46::after{content:attr(title)}"
                  "button#b46::before{content:attr(value) '/' attr(name)}"
                  "</style>"
                  "<a id=t46 title=T46>x</a>"
                  "<button id=b46 name=M46 value=V46>Go</button>"
                  "<input id=p46 name=P46 placeholder=PH46>");
    l76e_render(html);
    TEST_ASSERT_NOT_NULL(find_text("T46"));
    TEST_ASSERT_NOT_NULL(find_text("V46/M46"));
    TEST_ASSERT_TRUE(g_doc.nodes[l76e_node("p46")].fph_len > 0);
    l76e_done();
}

static void test_l76_ext_attr_47(void) {
    char html[700];
    sprintf(html, "<style>"
                  "a#t47::after{content:attr(title)}"
                  "button#b47::before{content:attr(value) '/' attr(name)}"
                  "</style>"
                  "<a id=t47 title=T47>x</a>"
                  "<button id=b47 name=M47 value=V47>Go</button>"
                  "<input id=p47 name=P47 placeholder=PH47>");
    l76e_render(html);
    TEST_ASSERT_NOT_NULL(find_text("T47"));
    TEST_ASSERT_NOT_NULL(find_text("V47/M47"));
    TEST_ASSERT_TRUE(g_doc.nodes[l76e_node("p47")].fph_len > 0);
    l76e_done();
}

static void test_l76_ext_attr_48(void) {
    char html[700];
    sprintf(html, "<style>"
                  "a#t48::after{content:attr(title)}"
                  "button#b48::before{content:attr(value) '/' attr(name)}"
                  "</style>"
                  "<a id=t48 title=T48>x</a>"
                  "<button id=b48 name=M48 value=V48>Go</button>"
                  "<input id=p48 name=P48 placeholder=PH48>");
    l76e_render(html);
    TEST_ASSERT_NOT_NULL(find_text("T48"));
    TEST_ASSERT_NOT_NULL(find_text("V48/M48"));
    TEST_ASSERT_TRUE(g_doc.nodes[l76e_node("p48")].fph_len > 0);
    l76e_done();
}

static void test_l76_ext_attr_49(void) {
    char html[700];
    sprintf(html, "<style>"
                  "a#t49::after{content:attr(title)}"
                  "button#b49::before{content:attr(value) '/' attr(name)}"
                  "</style>"
                  "<a id=t49 title=T49>x</a>"
                  "<button id=b49 name=M49 value=V49>Go</button>"
                  "<input id=p49 name=P49 placeholder=PH49>");
    l76e_render(html);
    TEST_ASSERT_NOT_NULL(find_text("T49"));
    TEST_ASSERT_NOT_NULL(find_text("V49/M49"));
    TEST_ASSERT_TRUE(g_doc.nodes[l76e_node("p49")].fph_len > 0);
    l76e_done();
}

static void test_l76_ext_attr_50(void) {
    char html[700];
    sprintf(html, "<style>"
                  "a#t50::after{content:attr(title)}"
                  "button#b50::before{content:attr(value) '/' attr(name)}"
                  "</style>"
                  "<a id=t50 title=T50>x</a>"
                  "<button id=b50 name=M50 value=V50>Go</button>"
                  "<input id=p50 name=P50 placeholder=PH50>");
    l76e_render(html);
    TEST_ASSERT_NOT_NULL(find_text("T50"));
    TEST_ASSERT_NOT_NULL(find_text("V50/M50"));
    TEST_ASSERT_TRUE(g_doc.nodes[l76e_node("p50")].fph_len > 0);
    l76e_done();
}

static void test_l76_ext_attr_51(void) {
    char html[700];
    sprintf(html, "<style>"
                  "a#t51::after{content:attr(title)}"
                  "button#b51::before{content:attr(value) '/' attr(name)}"
                  "</style>"
                  "<a id=t51 title=T51>x</a>"
                  "<button id=b51 name=M51 value=V51>Go</button>"
                  "<input id=p51 name=P51 placeholder=PH51>");
    l76e_render(html);
    TEST_ASSERT_NOT_NULL(find_text("T51"));
    TEST_ASSERT_NOT_NULL(find_text("V51/M51"));
    TEST_ASSERT_TRUE(g_doc.nodes[l76e_node("p51")].fph_len > 0);
    l76e_done();
}

static void test_l76_ext_attr_52(void) {
    char html[700];
    sprintf(html, "<style>"
                  "a#t52::after{content:attr(title)}"
                  "button#b52::before{content:attr(value) '/' attr(name)}"
                  "</style>"
                  "<a id=t52 title=T52>x</a>"
                  "<button id=b52 name=M52 value=V52>Go</button>"
                  "<input id=p52 name=P52 placeholder=PH52>");
    l76e_render(html);
    TEST_ASSERT_NOT_NULL(find_text("T52"));
    TEST_ASSERT_NOT_NULL(find_text("V52/M52"));
    TEST_ASSERT_TRUE(g_doc.nodes[l76e_node("p52")].fph_len > 0);
    l76e_done();
}

static void test_l76_ext_attr_53(void) {
    char html[700];
    sprintf(html, "<style>"
                  "a#t53::after{content:attr(title)}"
                  "button#b53::before{content:attr(value) '/' attr(name)}"
                  "</style>"
                  "<a id=t53 title=T53>x</a>"
                  "<button id=b53 name=M53 value=V53>Go</button>"
                  "<input id=p53 name=P53 placeholder=PH53>");
    l76e_render(html);
    TEST_ASSERT_NOT_NULL(find_text("T53"));
    TEST_ASSERT_NOT_NULL(find_text("V53/M53"));
    TEST_ASSERT_TRUE(g_doc.nodes[l76e_node("p53")].fph_len > 0);
    l76e_done();
}

static void test_l76_ext_attr_54(void) {
    char html[700];
    sprintf(html, "<style>"
                  "a#t54::after{content:attr(title)}"
                  "button#b54::before{content:attr(value) '/' attr(name)}"
                  "</style>"
                  "<a id=t54 title=T54>x</a>"
                  "<button id=b54 name=M54 value=V54>Go</button>"
                  "<input id=p54 name=P54 placeholder=PH54>");
    l76e_render(html);
    TEST_ASSERT_NOT_NULL(find_text("T54"));
    TEST_ASSERT_NOT_NULL(find_text("V54/M54"));
    TEST_ASSERT_TRUE(g_doc.nodes[l76e_node("p54")].fph_len > 0);
    l76e_done();
}

static void test_l76_ext_attr_55(void) {
    char html[700];
    sprintf(html, "<style>"
                  "a#t55::after{content:attr(title)}"
                  "button#b55::before{content:attr(value) '/' attr(name)}"
                  "</style>"
                  "<a id=t55 title=T55>x</a>"
                  "<button id=b55 name=M55 value=V55>Go</button>"
                  "<input id=p55 name=P55 placeholder=PH55>");
    l76e_render(html);
    TEST_ASSERT_NOT_NULL(find_text("T55"));
    TEST_ASSERT_NOT_NULL(find_text("V55/M55"));
    TEST_ASSERT_TRUE(g_doc.nodes[l76e_node("p55")].fph_len > 0);
    l76e_done();
}

static void test_l76_ext_attr_56(void) {
    char html[700];
    sprintf(html, "<style>"
                  "a#t56::after{content:attr(title)}"
                  "button#b56::before{content:attr(value) '/' attr(name)}"
                  "</style>"
                  "<a id=t56 title=T56>x</a>"
                  "<button id=b56 name=M56 value=V56>Go</button>"
                  "<input id=p56 name=P56 placeholder=PH56>");
    l76e_render(html);
    TEST_ASSERT_NOT_NULL(find_text("T56"));
    TEST_ASSERT_NOT_NULL(find_text("V56/M56"));
    TEST_ASSERT_TRUE(g_doc.nodes[l76e_node("p56")].fph_len > 0);
    l76e_done();
}

static void test_l76_ext_attr_57(void) {
    char html[700];
    sprintf(html, "<style>"
                  "a#t57::after{content:attr(title)}"
                  "button#b57::before{content:attr(value) '/' attr(name)}"
                  "</style>"
                  "<a id=t57 title=T57>x</a>"
                  "<button id=b57 name=M57 value=V57>Go</button>"
                  "<input id=p57 name=P57 placeholder=PH57>");
    l76e_render(html);
    TEST_ASSERT_NOT_NULL(find_text("T57"));
    TEST_ASSERT_NOT_NULL(find_text("V57/M57"));
    TEST_ASSERT_TRUE(g_doc.nodes[l76e_node("p57")].fph_len > 0);
    l76e_done();
}

static void test_l76_ext_attr_58(void) {
    char html[700];
    sprintf(html, "<style>"
                  "a#t58::after{content:attr(title)}"
                  "button#b58::before{content:attr(value) '/' attr(name)}"
                  "</style>"
                  "<a id=t58 title=T58>x</a>"
                  "<button id=b58 name=M58 value=V58>Go</button>"
                  "<input id=p58 name=P58 placeholder=PH58>");
    l76e_render(html);
    TEST_ASSERT_NOT_NULL(find_text("T58"));
    TEST_ASSERT_NOT_NULL(find_text("V58/M58"));
    TEST_ASSERT_TRUE(g_doc.nodes[l76e_node("p58")].fph_len > 0);
    l76e_done();
}

static void test_l76_ext_attr_59(void) {
    char html[700];
    sprintf(html, "<style>"
                  "a#t59::after{content:attr(title)}"
                  "button#b59::before{content:attr(value) '/' attr(name)}"
                  "</style>"
                  "<a id=t59 title=T59>x</a>"
                  "<button id=b59 name=M59 value=V59>Go</button>"
                  "<input id=p59 name=P59 placeholder=PH59>");
    l76e_render(html);
    TEST_ASSERT_NOT_NULL(find_text("T59"));
    TEST_ASSERT_NOT_NULL(find_text("V59/M59"));
    TEST_ASSERT_TRUE(g_doc.nodes[l76e_node("p59")].fph_len > 0);
    l76e_done();
}

static void test_l76_ext_attr_60(void) {
    char html[700];
    sprintf(html, "<style>"
                  "a#t60::after{content:attr(title)}"
                  "button#b60::before{content:attr(value) '/' attr(name)}"
                  "</style>"
                  "<a id=t60 title=T60>x</a>"
                  "<button id=b60 name=M60 value=V60>Go</button>"
                  "<input id=p60 name=P60 placeholder=PH60>");
    l76e_render(html);
    TEST_ASSERT_NOT_NULL(find_text("T60"));
    TEST_ASSERT_NOT_NULL(find_text("V60/M60"));
    TEST_ASSERT_TRUE(g_doc.nodes[l76e_node("p60")].fph_len > 0);
    l76e_done();
}

static void test_l76_ext_outfs_1(void) {
    we_form_req_t q;
    char html[500];
    sprintf(html, "<form action=/ofs1 method=GET>"
                  "<fieldset disabled><output id=xo1 name=xo1 value=Z1>Z1</output></fieldset>"
                  "<output id=yo1 name=yo1 value=Y1>Y1</output>"
                  "<input type=submit></form>");
    l76e_render(html);
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76e_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_NULL(strstr(q.url, "xo1="));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "yo1=Y1"));
    l76e_done();
}

static void test_l76_ext_outfs_2(void) {
    we_form_req_t q;
    char html[500];
    sprintf(html, "<form action=/ofs2 method=GET>"
                  "<fieldset disabled><output id=xo2 name=xo2 value=Z2>Z2</output></fieldset>"
                  "<output id=yo2 name=yo2 value=Y2>Y2</output>"
                  "<input type=submit></form>");
    l76e_render(html);
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76e_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_NULL(strstr(q.url, "xo2="));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "yo2=Y2"));
    l76e_done();
}

static void test_l76_ext_outfs_3(void) {
    we_form_req_t q;
    char html[500];
    sprintf(html, "<form action=/ofs3 method=GET>"
                  "<fieldset disabled><output id=xo3 name=xo3 value=Z3>Z3</output></fieldset>"
                  "<output id=yo3 name=yo3 value=Y3>Y3</output>"
                  "<input type=submit></form>");
    l76e_render(html);
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76e_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_NULL(strstr(q.url, "xo3="));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "yo3=Y3"));
    l76e_done();
}

static void test_l76_ext_outfs_4(void) {
    we_form_req_t q;
    char html[500];
    sprintf(html, "<form action=/ofs4 method=GET>"
                  "<fieldset disabled><output id=xo4 name=xo4 value=Z4>Z4</output></fieldset>"
                  "<output id=yo4 name=yo4 value=Y4>Y4</output>"
                  "<input type=submit></form>");
    l76e_render(html);
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76e_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_NULL(strstr(q.url, "xo4="));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "yo4=Y4"));
    l76e_done();
}

static void test_l76_ext_outfs_5(void) {
    we_form_req_t q;
    char html[500];
    sprintf(html, "<form action=/ofs5 method=GET>"
                  "<fieldset disabled><output id=xo5 name=xo5 value=Z5>Z5</output></fieldset>"
                  "<output id=yo5 name=yo5 value=Y5>Y5</output>"
                  "<input type=submit></form>");
    l76e_render(html);
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76e_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_NULL(strstr(q.url, "xo5="));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "yo5=Y5"));
    l76e_done();
}

static void test_l76_ext_outfs_6(void) {
    we_form_req_t q;
    char html[500];
    sprintf(html, "<form action=/ofs6 method=GET>"
                  "<fieldset disabled><output id=xo6 name=xo6 value=Z6>Z6</output></fieldset>"
                  "<output id=yo6 name=yo6 value=Y6>Y6</output>"
                  "<input type=submit></form>");
    l76e_render(html);
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76e_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_NULL(strstr(q.url, "xo6="));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "yo6=Y6"));
    l76e_done();
}

static void test_l76_ext_outfs_7(void) {
    we_form_req_t q;
    char html[500];
    sprintf(html, "<form action=/ofs7 method=GET>"
                  "<fieldset disabled><output id=xo7 name=xo7 value=Z7>Z7</output></fieldset>"
                  "<output id=yo7 name=yo7 value=Y7>Y7</output>"
                  "<input type=submit></form>");
    l76e_render(html);
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76e_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_NULL(strstr(q.url, "xo7="));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "yo7=Y7"));
    l76e_done();
}

static void test_l76_ext_outfs_8(void) {
    we_form_req_t q;
    char html[500];
    sprintf(html, "<form action=/ofs8 method=GET>"
                  "<fieldset disabled><output id=xo8 name=xo8 value=Z8>Z8</output></fieldset>"
                  "<output id=yo8 name=yo8 value=Y8>Y8</output>"
                  "<input type=submit></form>");
    l76e_render(html);
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76e_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_NULL(strstr(q.url, "xo8="));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "yo8=Y8"));
    l76e_done();
}

static void test_l76_ext_outfs_9(void) {
    we_form_req_t q;
    char html[500];
    sprintf(html, "<form action=/ofs9 method=GET>"
                  "<fieldset disabled><output id=xo9 name=xo9 value=Z9>Z9</output></fieldset>"
                  "<output id=yo9 name=yo9 value=Y9>Y9</output>"
                  "<input type=submit></form>");
    l76e_render(html);
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76e_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_NULL(strstr(q.url, "xo9="));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "yo9=Y9"));
    l76e_done();
}

static void test_l76_ext_outfs_10(void) {
    we_form_req_t q;
    char html[500];
    sprintf(html, "<form action=/ofs10 method=GET>"
                  "<fieldset disabled><output id=xo10 name=xo10 value=Z10>Z10</output></fieldset>"
                  "<output id=yo10 name=yo10 value=Y10>Y10</output>"
                  "<input type=submit></form>");
    l76e_render(html);
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76e_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_NULL(strstr(q.url, "xo10="));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "yo10=Y10"));
    l76e_done();
}

static void test_l76_ext_outfs_11(void) {
    we_form_req_t q;
    char html[500];
    sprintf(html, "<form action=/ofs11 method=GET>"
                  "<fieldset disabled><output id=xo11 name=xo11 value=Z11>Z11</output></fieldset>"
                  "<output id=yo11 name=yo11 value=Y11>Y11</output>"
                  "<input type=submit></form>");
    l76e_render(html);
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76e_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_NULL(strstr(q.url, "xo11="));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "yo11=Y11"));
    l76e_done();
}

static void test_l76_ext_outfs_12(void) {
    we_form_req_t q;
    char html[500];
    sprintf(html, "<form action=/ofs12 method=GET>"
                  "<fieldset disabled><output id=xo12 name=xo12 value=Z12>Z12</output></fieldset>"
                  "<output id=yo12 name=yo12 value=Y12>Y12</output>"
                  "<input type=submit></form>");
    l76e_render(html);
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76e_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_NULL(strstr(q.url, "xo12="));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "yo12=Y12"));
    l76e_done();
}

static void test_l76_ext_outfs_13(void) {
    we_form_req_t q;
    char html[500];
    sprintf(html, "<form action=/ofs13 method=GET>"
                  "<fieldset disabled><output id=xo13 name=xo13 value=Z13>Z13</output></fieldset>"
                  "<output id=yo13 name=yo13 value=Y13>Y13</output>"
                  "<input type=submit></form>");
    l76e_render(html);
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76e_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_NULL(strstr(q.url, "xo13="));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "yo13=Y13"));
    l76e_done();
}

static void test_l76_ext_outfs_14(void) {
    we_form_req_t q;
    char html[500];
    sprintf(html, "<form action=/ofs14 method=GET>"
                  "<fieldset disabled><output id=xo14 name=xo14 value=Z14>Z14</output></fieldset>"
                  "<output id=yo14 name=yo14 value=Y14>Y14</output>"
                  "<input type=submit></form>");
    l76e_render(html);
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76e_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_NULL(strstr(q.url, "xo14="));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "yo14=Y14"));
    l76e_done();
}

static void test_l76_ext_outfs_15(void) {
    we_form_req_t q;
    char html[500];
    sprintf(html, "<form action=/ofs15 method=GET>"
                  "<fieldset disabled><output id=xo15 name=xo15 value=Z15>Z15</output></fieldset>"
                  "<output id=yo15 name=yo15 value=Y15>Y15</output>"
                  "<input type=submit></form>");
    l76e_render(html);
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76e_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_NULL(strstr(q.url, "xo15="));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "yo15=Y15"));
    l76e_done();
}

static void test_l76_ext_outfs_16(void) {
    we_form_req_t q;
    char html[500];
    sprintf(html, "<form action=/ofs16 method=GET>"
                  "<fieldset disabled><output id=xo16 name=xo16 value=Z16>Z16</output></fieldset>"
                  "<output id=yo16 name=yo16 value=Y16>Y16</output>"
                  "<input type=submit></form>");
    l76e_render(html);
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76e_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_NULL(strstr(q.url, "xo16="));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "yo16=Y16"));
    l76e_done();
}

static void test_l76_ext_outfs_17(void) {
    we_form_req_t q;
    char html[500];
    sprintf(html, "<form action=/ofs17 method=GET>"
                  "<fieldset disabled><output id=xo17 name=xo17 value=Z17>Z17</output></fieldset>"
                  "<output id=yo17 name=yo17 value=Y17>Y17</output>"
                  "<input type=submit></form>");
    l76e_render(html);
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76e_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_NULL(strstr(q.url, "xo17="));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "yo17=Y17"));
    l76e_done();
}

static void test_l76_ext_outfs_18(void) {
    we_form_req_t q;
    char html[500];
    sprintf(html, "<form action=/ofs18 method=GET>"
                  "<fieldset disabled><output id=xo18 name=xo18 value=Z18>Z18</output></fieldset>"
                  "<output id=yo18 name=yo18 value=Y18>Y18</output>"
                  "<input type=submit></form>");
    l76e_render(html);
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76e_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_NULL(strstr(q.url, "xo18="));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "yo18=Y18"));
    l76e_done();
}

static void test_l76_ext_outfs_19(void) {
    we_form_req_t q;
    char html[500];
    sprintf(html, "<form action=/ofs19 method=GET>"
                  "<fieldset disabled><output id=xo19 name=xo19 value=Z19>Z19</output></fieldset>"
                  "<output id=yo19 name=yo19 value=Y19>Y19</output>"
                  "<input type=submit></form>");
    l76e_render(html);
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76e_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_NULL(strstr(q.url, "xo19="));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "yo19=Y19"));
    l76e_done();
}

static void test_l76_ext_outfs_20(void) {
    we_form_req_t q;
    char html[500];
    sprintf(html, "<form action=/ofs20 method=GET>"
                  "<fieldset disabled><output id=xo20 name=xo20 value=Z20>Z20</output></fieldset>"
                  "<output id=yo20 name=yo20 value=Y20>Y20</output>"
                  "<input type=submit></form>");
    l76e_render(html);
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76e_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_NULL(strstr(q.url, "xo20="));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "yo20=Y20"));
    l76e_done();
}

static void test_l76_ext_outfs_21(void) {
    we_form_req_t q;
    char html[500];
    sprintf(html, "<form action=/ofs21 method=GET>"
                  "<fieldset disabled><output id=xo21 name=xo21 value=Z21>Z21</output></fieldset>"
                  "<output id=yo21 name=yo21 value=Y21>Y21</output>"
                  "<input type=submit></form>");
    l76e_render(html);
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76e_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_NULL(strstr(q.url, "xo21="));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "yo21=Y21"));
    l76e_done();
}

static void test_l76_ext_outfs_22(void) {
    we_form_req_t q;
    char html[500];
    sprintf(html, "<form action=/ofs22 method=GET>"
                  "<fieldset disabled><output id=xo22 name=xo22 value=Z22>Z22</output></fieldset>"
                  "<output id=yo22 name=yo22 value=Y22>Y22</output>"
                  "<input type=submit></form>");
    l76e_render(html);
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76e_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_NULL(strstr(q.url, "xo22="));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "yo22=Y22"));
    l76e_done();
}

static void test_l76_ext_outfs_23(void) {
    we_form_req_t q;
    char html[500];
    sprintf(html, "<form action=/ofs23 method=GET>"
                  "<fieldset disabled><output id=xo23 name=xo23 value=Z23>Z23</output></fieldset>"
                  "<output id=yo23 name=yo23 value=Y23>Y23</output>"
                  "<input type=submit></form>");
    l76e_render(html);
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76e_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_NULL(strstr(q.url, "xo23="));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "yo23=Y23"));
    l76e_done();
}

static void test_l76_ext_outfs_24(void) {
    we_form_req_t q;
    char html[500];
    sprintf(html, "<form action=/ofs24 method=GET>"
                  "<fieldset disabled><output id=xo24 name=xo24 value=Z24>Z24</output></fieldset>"
                  "<output id=yo24 name=yo24 value=Y24>Y24</output>"
                  "<input type=submit></form>");
    l76e_render(html);
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76e_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_NULL(strstr(q.url, "xo24="));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "yo24=Y24"));
    l76e_done();
}

static void test_l76_ext_outfs_25(void) {
    we_form_req_t q;
    char html[500];
    sprintf(html, "<form action=/ofs25 method=GET>"
                  "<fieldset disabled><output id=xo25 name=xo25 value=Z25>Z25</output></fieldset>"
                  "<output id=yo25 name=yo25 value=Y25>Y25</output>"
                  "<input type=submit></form>");
    l76e_render(html);
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76e_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_NULL(strstr(q.url, "xo25="));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "yo25=Y25"));
    l76e_done();
}

static void test_l76_ext_outfs_26(void) {
    we_form_req_t q;
    char html[500];
    sprintf(html, "<form action=/ofs26 method=GET>"
                  "<fieldset disabled><output id=xo26 name=xo26 value=Z26>Z26</output></fieldset>"
                  "<output id=yo26 name=yo26 value=Y26>Y26</output>"
                  "<input type=submit></form>");
    l76e_render(html);
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76e_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_NULL(strstr(q.url, "xo26="));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "yo26=Y26"));
    l76e_done();
}

static void test_l76_ext_outfs_27(void) {
    we_form_req_t q;
    char html[500];
    sprintf(html, "<form action=/ofs27 method=GET>"
                  "<fieldset disabled><output id=xo27 name=xo27 value=Z27>Z27</output></fieldset>"
                  "<output id=yo27 name=yo27 value=Y27>Y27</output>"
                  "<input type=submit></form>");
    l76e_render(html);
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76e_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_NULL(strstr(q.url, "xo27="));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "yo27=Y27"));
    l76e_done();
}

static void test_l76_ext_outfs_28(void) {
    we_form_req_t q;
    char html[500];
    sprintf(html, "<form action=/ofs28 method=GET>"
                  "<fieldset disabled><output id=xo28 name=xo28 value=Z28>Z28</output></fieldset>"
                  "<output id=yo28 name=yo28 value=Y28>Y28</output>"
                  "<input type=submit></form>");
    l76e_render(html);
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76e_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_NULL(strstr(q.url, "xo28="));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "yo28=Y28"));
    l76e_done();
}

static void test_l76_ext_outfs_29(void) {
    we_form_req_t q;
    char html[500];
    sprintf(html, "<form action=/ofs29 method=GET>"
                  "<fieldset disabled><output id=xo29 name=xo29 value=Z29>Z29</output></fieldset>"
                  "<output id=yo29 name=yo29 value=Y29>Y29</output>"
                  "<input type=submit></form>");
    l76e_render(html);
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76e_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_NULL(strstr(q.url, "xo29="));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "yo29=Y29"));
    l76e_done();
}

static void test_l76_ext_outfs_30(void) {
    we_form_req_t q;
    char html[500];
    sprintf(html, "<form action=/ofs30 method=GET>"
                  "<fieldset disabled><output id=xo30 name=xo30 value=Z30>Z30</output></fieldset>"
                  "<output id=yo30 name=yo30 value=Y30>Y30</output>"
                  "<input type=submit></form>");
    l76e_render(html);
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76e_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_NULL(strstr(q.url, "xo30="));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "yo30=Y30"));
    l76e_done();
}

static void test_l76_ext_outfs_31(void) {
    we_form_req_t q;
    char html[500];
    sprintf(html, "<form action=/ofs31 method=GET>"
                  "<fieldset disabled><output id=xo31 name=xo31 value=Z31>Z31</output></fieldset>"
                  "<output id=yo31 name=yo31 value=Y31>Y31</output>"
                  "<input type=submit></form>");
    l76e_render(html);
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76e_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_NULL(strstr(q.url, "xo31="));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "yo31=Y31"));
    l76e_done();
}

static void test_l76_ext_outfs_32(void) {
    we_form_req_t q;
    char html[500];
    sprintf(html, "<form action=/ofs32 method=GET>"
                  "<fieldset disabled><output id=xo32 name=xo32 value=Z32>Z32</output></fieldset>"
                  "<output id=yo32 name=yo32 value=Y32>Y32</output>"
                  "<input type=submit></form>");
    l76e_render(html);
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76e_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_NULL(strstr(q.url, "xo32="));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "yo32=Y32"));
    l76e_done();
}

static void test_l76_ext_outfs_33(void) {
    we_form_req_t q;
    char html[500];
    sprintf(html, "<form action=/ofs33 method=GET>"
                  "<fieldset disabled><output id=xo33 name=xo33 value=Z33>Z33</output></fieldset>"
                  "<output id=yo33 name=yo33 value=Y33>Y33</output>"
                  "<input type=submit></form>");
    l76e_render(html);
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76e_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_NULL(strstr(q.url, "xo33="));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "yo33=Y33"));
    l76e_done();
}

static void test_l76_ext_outfs_34(void) {
    we_form_req_t q;
    char html[500];
    sprintf(html, "<form action=/ofs34 method=GET>"
                  "<fieldset disabled><output id=xo34 name=xo34 value=Z34>Z34</output></fieldset>"
                  "<output id=yo34 name=yo34 value=Y34>Y34</output>"
                  "<input type=submit></form>");
    l76e_render(html);
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76e_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_NULL(strstr(q.url, "xo34="));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "yo34=Y34"));
    l76e_done();
}

static void test_l76_ext_outfs_35(void) {
    we_form_req_t q;
    char html[500];
    sprintf(html, "<form action=/ofs35 method=GET>"
                  "<fieldset disabled><output id=xo35 name=xo35 value=Z35>Z35</output></fieldset>"
                  "<output id=yo35 name=yo35 value=Y35>Y35</output>"
                  "<input type=submit></form>");
    l76e_render(html);
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76e_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_NULL(strstr(q.url, "xo35="));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "yo35=Y35"));
    l76e_done();
}

static void test_l76_ext_outfs_36(void) {
    we_form_req_t q;
    char html[500];
    sprintf(html, "<form action=/ofs36 method=GET>"
                  "<fieldset disabled><output id=xo36 name=xo36 value=Z36>Z36</output></fieldset>"
                  "<output id=yo36 name=yo36 value=Y36>Y36</output>"
                  "<input type=submit></form>");
    l76e_render(html);
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76e_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_NULL(strstr(q.url, "xo36="));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "yo36=Y36"));
    l76e_done();
}

static void test_l76_ext_outfs_37(void) {
    we_form_req_t q;
    char html[500];
    sprintf(html, "<form action=/ofs37 method=GET>"
                  "<fieldset disabled><output id=xo37 name=xo37 value=Z37>Z37</output></fieldset>"
                  "<output id=yo37 name=yo37 value=Y37>Y37</output>"
                  "<input type=submit></form>");
    l76e_render(html);
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76e_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_NULL(strstr(q.url, "xo37="));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "yo37=Y37"));
    l76e_done();
}

static void test_l76_ext_outfs_38(void) {
    we_form_req_t q;
    char html[500];
    sprintf(html, "<form action=/ofs38 method=GET>"
                  "<fieldset disabled><output id=xo38 name=xo38 value=Z38>Z38</output></fieldset>"
                  "<output id=yo38 name=yo38 value=Y38>Y38</output>"
                  "<input type=submit></form>");
    l76e_render(html);
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76e_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_NULL(strstr(q.url, "xo38="));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "yo38=Y38"));
    l76e_done();
}

static void test_l76_ext_outfs_39(void) {
    we_form_req_t q;
    char html[500];
    sprintf(html, "<form action=/ofs39 method=GET>"
                  "<fieldset disabled><output id=xo39 name=xo39 value=Z39>Z39</output></fieldset>"
                  "<output id=yo39 name=yo39 value=Y39>Y39</output>"
                  "<input type=submit></form>");
    l76e_render(html);
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76e_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_NULL(strstr(q.url, "xo39="));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "yo39=Y39"));
    l76e_done();
}

static void test_l76_ext_outfs_40(void) {
    we_form_req_t q;
    char html[500];
    sprintf(html, "<form action=/ofs40 method=GET>"
                  "<fieldset disabled><output id=xo40 name=xo40 value=Z40>Z40</output></fieldset>"
                  "<output id=yo40 name=yo40 value=Y40>Y40</output>"
                  "<input type=submit></form>");
    l76e_render(html);
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76e_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_NULL(strstr(q.url, "xo40="));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "yo40=Y40"));
    l76e_done();
}

static void lot76_ext_run(void) {
    RUN_TEST(test_l76_ext_color_1);
    RUN_TEST(test_l76_ext_color_2);
    RUN_TEST(test_l76_ext_color_3);
    RUN_TEST(test_l76_ext_color_4);
    RUN_TEST(test_l76_ext_color_5);
    RUN_TEST(test_l76_ext_color_6);
    RUN_TEST(test_l76_ext_color_7);
    RUN_TEST(test_l76_ext_color_8);
    RUN_TEST(test_l76_ext_color_9);
    RUN_TEST(test_l76_ext_color_10);
    RUN_TEST(test_l76_ext_color_11);
    RUN_TEST(test_l76_ext_color_12);
    RUN_TEST(test_l76_ext_color_13);
    RUN_TEST(test_l76_ext_color_14);
    RUN_TEST(test_l76_ext_color_15);
    RUN_TEST(test_l76_ext_color_16);
    RUN_TEST(test_l76_ext_color_17);
    RUN_TEST(test_l76_ext_color_18);
    RUN_TEST(test_l76_ext_color_19);
    RUN_TEST(test_l76_ext_color_20);
    RUN_TEST(test_l76_ext_color_21);
    RUN_TEST(test_l76_ext_color_22);
    RUN_TEST(test_l76_ext_color_23);
    RUN_TEST(test_l76_ext_color_24);
    RUN_TEST(test_l76_ext_color_25);
    RUN_TEST(test_l76_ext_color_26);
    RUN_TEST(test_l76_ext_color_27);
    RUN_TEST(test_l76_ext_color_28);
    RUN_TEST(test_l76_ext_color_29);
    RUN_TEST(test_l76_ext_color_30);
    RUN_TEST(test_l76_ext_color_31);
    RUN_TEST(test_l76_ext_color_32);
    RUN_TEST(test_l76_ext_color_33);
    RUN_TEST(test_l76_ext_color_34);
    RUN_TEST(test_l76_ext_color_35);
    RUN_TEST(test_l76_ext_color_36);
    RUN_TEST(test_l76_ext_color_37);
    RUN_TEST(test_l76_ext_color_38);
    RUN_TEST(test_l76_ext_color_39);
    RUN_TEST(test_l76_ext_color_40);
    RUN_TEST(test_l76_ext_color_41);
    RUN_TEST(test_l76_ext_color_42);
    RUN_TEST(test_l76_ext_color_43);
    RUN_TEST(test_l76_ext_color_44);
    RUN_TEST(test_l76_ext_color_45);
    RUN_TEST(test_l76_ext_color_46);
    RUN_TEST(test_l76_ext_color_47);
    RUN_TEST(test_l76_ext_color_48);
    RUN_TEST(test_l76_ext_color_49);
    RUN_TEST(test_l76_ext_color_50);
    RUN_TEST(test_l76_ext_color_51);
    RUN_TEST(test_l76_ext_color_52);
    RUN_TEST(test_l76_ext_color_53);
    RUN_TEST(test_l76_ext_color_54);
    RUN_TEST(test_l76_ext_color_55);
    RUN_TEST(test_l76_ext_color_56);
    RUN_TEST(test_l76_ext_color_57);
    RUN_TEST(test_l76_ext_color_58);
    RUN_TEST(test_l76_ext_color_59);
    RUN_TEST(test_l76_ext_color_60);
    RUN_TEST(test_l76_ext_color_61);
    RUN_TEST(test_l76_ext_color_62);
    RUN_TEST(test_l76_ext_color_63);
    RUN_TEST(test_l76_ext_color_64);
    RUN_TEST(test_l76_ext_color_65);
    RUN_TEST(test_l76_ext_color_66);
    RUN_TEST(test_l76_ext_color_67);
    RUN_TEST(test_l76_ext_color_68);
    RUN_TEST(test_l76_ext_color_69);
    RUN_TEST(test_l76_ext_color_70);
    RUN_TEST(test_l76_ext_color_71);
    RUN_TEST(test_l76_ext_color_72);
    RUN_TEST(test_l76_ext_color_73);
    RUN_TEST(test_l76_ext_color_74);
    RUN_TEST(test_l76_ext_color_75);
    RUN_TEST(test_l76_ext_color_76);
    RUN_TEST(test_l76_ext_color_77);
    RUN_TEST(test_l76_ext_color_78);
    RUN_TEST(test_l76_ext_color_79);
    RUN_TEST(test_l76_ext_color_80);
    RUN_TEST(test_l76_ext_date_1);
    RUN_TEST(test_l76_ext_date_2);
    RUN_TEST(test_l76_ext_date_3);
    RUN_TEST(test_l76_ext_date_4);
    RUN_TEST(test_l76_ext_date_5);
    RUN_TEST(test_l76_ext_date_6);
    RUN_TEST(test_l76_ext_date_7);
    RUN_TEST(test_l76_ext_date_8);
    RUN_TEST(test_l76_ext_date_9);
    RUN_TEST(test_l76_ext_date_10);
    RUN_TEST(test_l76_ext_date_11);
    RUN_TEST(test_l76_ext_date_12);
    RUN_TEST(test_l76_ext_date_13);
    RUN_TEST(test_l76_ext_date_14);
    RUN_TEST(test_l76_ext_date_15);
    RUN_TEST(test_l76_ext_date_16);
    RUN_TEST(test_l76_ext_date_17);
    RUN_TEST(test_l76_ext_date_18);
    RUN_TEST(test_l76_ext_date_19);
    RUN_TEST(test_l76_ext_date_20);
    RUN_TEST(test_l76_ext_date_21);
    RUN_TEST(test_l76_ext_date_22);
    RUN_TEST(test_l76_ext_date_23);
    RUN_TEST(test_l76_ext_date_24);
    RUN_TEST(test_l76_ext_date_25);
    RUN_TEST(test_l76_ext_date_26);
    RUN_TEST(test_l76_ext_date_27);
    RUN_TEST(test_l76_ext_date_28);
    RUN_TEST(test_l76_ext_date_29);
    RUN_TEST(test_l76_ext_date_30);
    RUN_TEST(test_l76_ext_date_31);
    RUN_TEST(test_l76_ext_date_32);
    RUN_TEST(test_l76_ext_date_33);
    RUN_TEST(test_l76_ext_date_34);
    RUN_TEST(test_l76_ext_date_35);
    RUN_TEST(test_l76_ext_date_36);
    RUN_TEST(test_l76_ext_date_37);
    RUN_TEST(test_l76_ext_date_38);
    RUN_TEST(test_l76_ext_date_39);
    RUN_TEST(test_l76_ext_date_40);
    RUN_TEST(test_l76_ext_date_41);
    RUN_TEST(test_l76_ext_date_42);
    RUN_TEST(test_l76_ext_date_43);
    RUN_TEST(test_l76_ext_date_44);
    RUN_TEST(test_l76_ext_date_45);
    RUN_TEST(test_l76_ext_date_46);
    RUN_TEST(test_l76_ext_date_47);
    RUN_TEST(test_l76_ext_date_48);
    RUN_TEST(test_l76_ext_date_49);
    RUN_TEST(test_l76_ext_date_50);
    RUN_TEST(test_l76_ext_date_51);
    RUN_TEST(test_l76_ext_date_52);
    RUN_TEST(test_l76_ext_date_53);
    RUN_TEST(test_l76_ext_date_54);
    RUN_TEST(test_l76_ext_date_55);
    RUN_TEST(test_l76_ext_date_56);
    RUN_TEST(test_l76_ext_date_57);
    RUN_TEST(test_l76_ext_date_58);
    RUN_TEST(test_l76_ext_date_59);
    RUN_TEST(test_l76_ext_date_60);
    RUN_TEST(test_l76_ext_date_61);
    RUN_TEST(test_l76_ext_date_62);
    RUN_TEST(test_l76_ext_date_63);
    RUN_TEST(test_l76_ext_date_64);
    RUN_TEST(test_l76_ext_date_65);
    RUN_TEST(test_l76_ext_date_66);
    RUN_TEST(test_l76_ext_date_67);
    RUN_TEST(test_l76_ext_date_68);
    RUN_TEST(test_l76_ext_date_69);
    RUN_TEST(test_l76_ext_date_70);
    RUN_TEST(test_l76_ext_date_71);
    RUN_TEST(test_l76_ext_date_72);
    RUN_TEST(test_l76_ext_date_73);
    RUN_TEST(test_l76_ext_date_74);
    RUN_TEST(test_l76_ext_date_75);
    RUN_TEST(test_l76_ext_date_76);
    RUN_TEST(test_l76_ext_date_77);
    RUN_TEST(test_l76_ext_date_78);
    RUN_TEST(test_l76_ext_date_79);
    RUN_TEST(test_l76_ext_date_80);
    RUN_TEST(test_l76_ext_attr_1);
    RUN_TEST(test_l76_ext_attr_2);
    RUN_TEST(test_l76_ext_attr_3);
    RUN_TEST(test_l76_ext_attr_4);
    RUN_TEST(test_l76_ext_attr_5);
    RUN_TEST(test_l76_ext_attr_6);
    RUN_TEST(test_l76_ext_attr_7);
    RUN_TEST(test_l76_ext_attr_8);
    RUN_TEST(test_l76_ext_attr_9);
    RUN_TEST(test_l76_ext_attr_10);
    RUN_TEST(test_l76_ext_attr_11);
    RUN_TEST(test_l76_ext_attr_12);
    RUN_TEST(test_l76_ext_attr_13);
    RUN_TEST(test_l76_ext_attr_14);
    RUN_TEST(test_l76_ext_attr_15);
    RUN_TEST(test_l76_ext_attr_16);
    RUN_TEST(test_l76_ext_attr_17);
    RUN_TEST(test_l76_ext_attr_18);
    RUN_TEST(test_l76_ext_attr_19);
    RUN_TEST(test_l76_ext_attr_20);
    RUN_TEST(test_l76_ext_attr_21);
    RUN_TEST(test_l76_ext_attr_22);
    RUN_TEST(test_l76_ext_attr_23);
    RUN_TEST(test_l76_ext_attr_24);
    RUN_TEST(test_l76_ext_attr_25);
    RUN_TEST(test_l76_ext_attr_26);
    RUN_TEST(test_l76_ext_attr_27);
    RUN_TEST(test_l76_ext_attr_28);
    RUN_TEST(test_l76_ext_attr_29);
    RUN_TEST(test_l76_ext_attr_30);
    RUN_TEST(test_l76_ext_attr_31);
    RUN_TEST(test_l76_ext_attr_32);
    RUN_TEST(test_l76_ext_attr_33);
    RUN_TEST(test_l76_ext_attr_34);
    RUN_TEST(test_l76_ext_attr_35);
    RUN_TEST(test_l76_ext_attr_36);
    RUN_TEST(test_l76_ext_attr_37);
    RUN_TEST(test_l76_ext_attr_38);
    RUN_TEST(test_l76_ext_attr_39);
    RUN_TEST(test_l76_ext_attr_40);
    RUN_TEST(test_l76_ext_attr_41);
    RUN_TEST(test_l76_ext_attr_42);
    RUN_TEST(test_l76_ext_attr_43);
    RUN_TEST(test_l76_ext_attr_44);
    RUN_TEST(test_l76_ext_attr_45);
    RUN_TEST(test_l76_ext_attr_46);
    RUN_TEST(test_l76_ext_attr_47);
    RUN_TEST(test_l76_ext_attr_48);
    RUN_TEST(test_l76_ext_attr_49);
    RUN_TEST(test_l76_ext_attr_50);
    RUN_TEST(test_l76_ext_attr_51);
    RUN_TEST(test_l76_ext_attr_52);
    RUN_TEST(test_l76_ext_attr_53);
    RUN_TEST(test_l76_ext_attr_54);
    RUN_TEST(test_l76_ext_attr_55);
    RUN_TEST(test_l76_ext_attr_56);
    RUN_TEST(test_l76_ext_attr_57);
    RUN_TEST(test_l76_ext_attr_58);
    RUN_TEST(test_l76_ext_attr_59);
    RUN_TEST(test_l76_ext_attr_60);
    RUN_TEST(test_l76_ext_outfs_1);
    RUN_TEST(test_l76_ext_outfs_2);
    RUN_TEST(test_l76_ext_outfs_3);
    RUN_TEST(test_l76_ext_outfs_4);
    RUN_TEST(test_l76_ext_outfs_5);
    RUN_TEST(test_l76_ext_outfs_6);
    RUN_TEST(test_l76_ext_outfs_7);
    RUN_TEST(test_l76_ext_outfs_8);
    RUN_TEST(test_l76_ext_outfs_9);
    RUN_TEST(test_l76_ext_outfs_10);
    RUN_TEST(test_l76_ext_outfs_11);
    RUN_TEST(test_l76_ext_outfs_12);
    RUN_TEST(test_l76_ext_outfs_13);
    RUN_TEST(test_l76_ext_outfs_14);
    RUN_TEST(test_l76_ext_outfs_15);
    RUN_TEST(test_l76_ext_outfs_16);
    RUN_TEST(test_l76_ext_outfs_17);
    RUN_TEST(test_l76_ext_outfs_18);
    RUN_TEST(test_l76_ext_outfs_19);
    RUN_TEST(test_l76_ext_outfs_20);
    RUN_TEST(test_l76_ext_outfs_21);
    RUN_TEST(test_l76_ext_outfs_22);
    RUN_TEST(test_l76_ext_outfs_23);
    RUN_TEST(test_l76_ext_outfs_24);
    RUN_TEST(test_l76_ext_outfs_25);
    RUN_TEST(test_l76_ext_outfs_26);
    RUN_TEST(test_l76_ext_outfs_27);
    RUN_TEST(test_l76_ext_outfs_28);
    RUN_TEST(test_l76_ext_outfs_29);
    RUN_TEST(test_l76_ext_outfs_30);
    RUN_TEST(test_l76_ext_outfs_31);
    RUN_TEST(test_l76_ext_outfs_32);
    RUN_TEST(test_l76_ext_outfs_33);
    RUN_TEST(test_l76_ext_outfs_34);
    RUN_TEST(test_l76_ext_outfs_35);
    RUN_TEST(test_l76_ext_outfs_36);
    RUN_TEST(test_l76_ext_outfs_37);
    RUN_TEST(test_l76_ext_outfs_38);
    RUN_TEST(test_l76_ext_outfs_39);
    RUN_TEST(test_l76_ext_outfs_40);
}
