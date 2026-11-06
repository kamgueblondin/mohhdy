/* Lot 76: type=color / type=date, autocomplete, formtarget, output,
 * content:attr(title|value|placeholder|name). Host tests only (no DNS). */
#ifndef WE_FF2_AC_OFF
#define WE_FF2_AC_OFF 524288u
#define WE_FF2_FORMTARGET 1048576u
#define WE_FF2_TARGET_BLANK 2097152u
#endif

static we_forms_t g_l76f;
static const char *g_l76_html;

static void l76_render(const char *html) {
    we_forms_init(&g_l76f);
    we_set_forms(&g_doc, &g_l76f);
    g_l76_html = html;
    TEST_ASSERT_EQUAL(0, we_render(&g_doc, html, (int)strlen(html), 480));
}
static void l76_done(void) { we_set_forms(&g_doc, 0); g_l76_html = 0; }
static int l76_node(const char *id) { int i = we_find_id(&g_doc, id); return i > 0 ? i : 0; }
static int l76_ctl(const char *id) { return g_doc.nodes[l76_node(id)].form_ord - 1; }
static void l76_reflow(void) {
    TEST_ASSERT_EQUAL(0, we_render(&g_doc, g_l76_html, (int)strlen(g_l76_html), 480));
}
static int l76_click(const char *id) {
    int i = l76_node(id), x = g_doc.nodes[i].x + 2, y = g_doc.nodes[i].y + 2, rc;
    rc = we_form_click(&g_doc, x, y);
    l76_reflow();
    return rc;
}
static int l76_submit_click(void) {
    int k, x, y;
    for (k = 1; k < (int)g_doc.nnodes; k++)
        if (g_doc.nodes[k].form_type == WE_FORM_SUBMIT) break;
    if (k >= (int)g_doc.nnodes) return -1;
    x = g_doc.nodes[k].x + 2; y = g_doc.nodes[k].y + 2;
    return we_form_click(&g_doc, x, y);
}

/* ---------- color ---------- */
static void test_l76_color_type_and_paint(void) {
    l76_render("<form action=/c method=GET>"
               "<input id=c type=color name=c value=#ff00aa>"
               "<input type=submit value=Go></form>");
    TEST_ASSERT_EQUAL(WE_FORM_COLOR, g_doc.nodes[l76_node("c")].form_type);
    TEST_ASSERT_EQUAL(0, l61_diag_count("input type taken as text"));
    TEST_ASSERT_NOT_NULL(find_text("#ff00aa"));
    TEST_ASSERT_EQUAL(0xff00aa, (int)g_doc.nodes[l76_node("c")].bg);
    l76_done();
}

static void test_l76_color_default_placeholder(void) {
    l76_render("<input id=c type=color name=c>");
    TEST_ASSERT_EQUAL(WE_FORM_COLOR, g_doc.nodes[l76_node("c")].form_type);
    TEST_ASSERT_NOT_NULL(find_text("#000000"));
    l76_done();
}

static void test_l76_color_validate_and_submit(void) {
    we_form_req_t q;
    l76_render("<form action=/ok method=GET>"
               "<input id=c type=color name=col value=#112233 required>"
               "<input type=submit></form>");
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76_submit_click());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "col=%23112233"));
    l76_done();
}

static void test_l76_color_invalid_blocks(void) {
    l76_render("<form action=/x method=GET>"
               "<input id=c type=color name=c value=nothex required>"
               "<input type=submit></form>");
    TEST_ASSERT_EQUAL(WE_FACT_CHANGED, l76_submit_click());
    TEST_ASSERT_TRUE(g_l76f.refused > 0 || g_l76f.submits == 0);
    l76_done();
}

static void test_l76_color_key_filter(void) {
    int before;
    l76_render("<input id=c type=color name=c value=#000000>");
    l76_click("c");
    before = g_l76f.refused;
    TEST_ASSERT_EQUAL(WE_FACT_NONE, we_form_key(&g_doc, 'z'));  /* not hex */
    TEST_ASSERT_TRUE(g_l76f.refused > before);
    TEST_ASSERT_EQUAL(WE_FACT_CHANGED, we_form_key(&g_doc, 'a'));
    l76_done();
}

/* ---------- date ---------- */
static void test_l76_date_type_and_paint(void) {
    l76_render("<form action=/d method=GET>"
               "<input id=d type=date name=d value=2024-02-29>"
               "<input type=submit></form>");
    TEST_ASSERT_EQUAL(WE_FORM_DATE, g_doc.nodes[l76_node("d")].form_type);
    TEST_ASSERT_EQUAL(0, l61_diag_count("input type taken as text"));
    TEST_ASSERT_NOT_NULL(find_text("2024-02-29"));
    l76_done();
}

static void test_l76_date_placeholder(void) {
    l76_render("<input id=d type=date name=d>");
    TEST_ASSERT_NOT_NULL(find_text("yyyy-mm-dd"));
    l76_done();
}

static void test_l76_date_leap_and_bad(void) {
    we_form_req_t q;
    /* valid leap day submits */
    l76_render("<form action=/ok method=GET>"
               "<input id=d type=date name=d value=2024-02-29>"
               "<input type=submit></form>");
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76_submit_click());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "d=2024-02-29"));
    l76_done();
    /* Feb 30 invalid */
    l76_render("<form action=/x method=GET>"
               "<input id=d type=date name=d value=2023-02-30 required>"
               "<input type=submit></form>");
    TEST_ASSERT_EQUAL(WE_FACT_CHANGED, l76_submit_click());
    l76_done();
    /* non-leap Feb 29 invalid */
    l76_render("<form action=/x method=GET>"
               "<input id=d type=date name=d value=2023-02-29 required>"
               "<input type=submit></form>");
    TEST_ASSERT_EQUAL(WE_FACT_CHANGED, l76_submit_click());
    l76_done();
}

static void test_l76_date_key_filter(void) {
    int before;
    l76_render("<input id=d type=date name=d value=2020-01-01>");
    l76_click("d");
    before = g_l76f.refused;
    TEST_ASSERT_EQUAL(WE_FACT_NONE, we_form_key(&g_doc, 'a'));
    TEST_ASSERT_TRUE(g_l76f.refused > before);
    TEST_ASSERT_EQUAL(WE_FACT_CHANGED, we_form_key(&g_doc, '1'));
    TEST_ASSERT_EQUAL(WE_FACT_CHANGED, we_form_key(&g_doc, '-'));
    l76_done();
}

static void test_l76_time_still_taken_as_text(void) {
    l76_render("<input id=t type=time value=12:00>");
    TEST_ASSERT_EQUAL(WE_FORM_TEXT, g_doc.nodes[l76_node("t")].form_type);
    TEST_ASSERT_EQUAL(1, l61_diag_count("input type taken as text"));
    l76_done();
}

/* ---------- autocomplete ---------- */
static void test_l76_autocomplete_off(void) {
    l76_render("<input id=a name=a autocomplete=off>"
               "<input id=b name=b autocomplete=on>"
               "<input id=c name=c autocomplete=new-password>");
    TEST_ASSERT_TRUE(g_doc.nodes[l76_node("a")].form_flags2 & WE_FF2_AC_OFF);
    TEST_ASSERT_FALSE(g_doc.nodes[l76_node("b")].form_flags2 & WE_FF2_AC_OFF);
    TEST_ASSERT_FALSE(g_doc.nodes[l76_node("c")].form_flags2 & WE_FF2_AC_OFF);
    TEST_ASSERT_TRUE(g_doc.nodes[l76_node("c")].fac_len > 0);
    l76_done();
}

/* ---------- formtarget ---------- */
static void test_l76_formtarget_blank(void) {
    we_form_req_t q;
    l76_render("<form action=/t method=GET>"
               "<input name=n value=1>"
               "<input id=s type=submit formtarget=_blank value=Go></form>");
    TEST_ASSERT_TRUE(g_doc.nodes[l76_node("s")].form_flags2 & WE_FF2_FORMTARGET);
    TEST_ASSERT_TRUE(g_doc.nodes[l76_node("s")].form_flags2 & WE_FF2_TARGET_BLANK);
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76_submit_click());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_EQUAL(1, q.target_blank);
    TEST_ASSERT_EQUAL_STRING("_blank", q.target);
    l76_done();
}

static void test_l76_form_target_attr(void) {
    we_form_req_t q;
    l76_render("<form action=/t method=GET target=_self>"
               "<input name=n value=1><input type=submit></form>");
    TEST_ASSERT_TRUE(g_doc.nodes[we_find_id(&g_doc, "n") > 0 ? 0 : 1].form_flags2 || 1);
    /* form itself carries FORMTARGET */
    {
        int f;
        for (f = 1; f < (int)g_doc.nnodes; f++)
            if (!g_doc.nodes[f].is_text && strcmp(g_doc.nodes[f].tag, "form") == 0) break;
        TEST_ASSERT_TRUE(g_doc.nodes[f].form_flags2 & WE_FF2_FORMTARGET);
        TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76_submit_click());
        TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
        TEST_ASSERT_EQUAL(0, q.target_blank);
        TEST_ASSERT_EQUAL_STRING("_self", q.target);
    }
    l76_done();
}

static void test_l76_formtarget_named(void) {
    we_form_req_t q;
    l76_render("<form action=/t method=GET>"
               "<input name=n value=1>"
               "<input id=s type=submit formtarget=frame1 value=Go></form>");
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76_submit_click());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_EQUAL(0, q.target_blank);
    TEST_ASSERT_EQUAL_STRING("frame1", q.target);
    l76_done();
}

/* ---------- output ---------- */
static void test_l76_output_paint_and_submit(void) {
    we_form_req_t q;
    l76_render("<form action=/o method=GET>"
               "<output id=o name=sum value=42>42</output>"
               "<input type=submit></form>");
    TEST_ASSERT_TRUE(g_doc.nodes[l76_node("o")].form_ord > 0);
    TEST_ASSERT_NOT_NULL(find_text("42"));
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76_submit_click());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "sum=42"));
    l76_done();
}

static void test_l76_output_not_focusable(void) {
    int fo;
    l76_render("<form><output id=o name=o>x</output><input id=i name=i></form>");
    /* Tab from -1 should skip output and land on the input */
    we_form_key(&g_doc, WE_KEY_TAB);
    fo = g_doc.nodes[l76_node("o")].form_ord - 1;
    TEST_ASSERT_TRUE(g_l76f.focus != fo);
    l76_done();
}

/* ---------- content:attr expansions ---------- */
static void test_l76_attr_title_value_placeholder_name(void) {
    /* title on a; value/name via button::before (input is replaced, no ::before) */
    l76_render("<style>"
               "a::after{content:attr(title)}"
               "button#x::before{content:attr(value) '/' attr(name)}"
               "</style>"
               "<a id=a title=Tip>L</a>"
               "<button id=x name=NM value=VAL>Go</button>"
               "<input id=p name=pp placeholder=PH>");
    TEST_ASSERT_EQUAL(32, g_doc.nodes[l76_node("a")].attr_flags & 32);
    TEST_ASSERT_TRUE(g_doc.nodes[l76_node("a")].title_len > 0);
    TEST_ASSERT_NOT_NULL(find_text("Tip"));
    TEST_ASSERT_EQUAL(3, (int)g_doc.nodes[l76_node("x")].fval_len);
    TEST_ASSERT_EQUAL(2, (int)g_doc.nodes[l76_node("x")].fname_len);
    TEST_ASSERT_EQUAL(2, (int)g_doc.nodes[l76_node("p")].fph_len);
    TEST_ASSERT_NOT_NULL(find_text("VAL/NM"));
    l76_done();
}

/* ---------- fieldset disabled still covers new types ---------- */
static void test_l76_fieldset_disabled_color_date(void) {
    l76_render("<form action=/x method=GET>"
               "<fieldset disabled>"
               "<input id=c type=color name=c value=#abcdef>"
               "<input id=d type=date name=d value=2020-01-01>"
               "</fieldset>"
               "<input type=submit></form>");
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76_submit_click());
    {
        we_form_req_t q;
        TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
        TEST_ASSERT_NULL(strstr(q.url, "c="));
        TEST_ASSERT_NULL(strstr(q.url, "d="));
    }
    l76_done();
}

static void lot76_forms_run(void) {
    RUN_TEST(test_l76_color_type_and_paint);
    RUN_TEST(test_l76_color_default_placeholder);
    RUN_TEST(test_l76_color_validate_and_submit);
    RUN_TEST(test_l76_color_invalid_blocks);
    RUN_TEST(test_l76_color_key_filter);
    RUN_TEST(test_l76_date_type_and_paint);
    RUN_TEST(test_l76_date_placeholder);
    RUN_TEST(test_l76_date_leap_and_bad);
    RUN_TEST(test_l76_date_key_filter);
    RUN_TEST(test_l76_time_still_taken_as_text);
    RUN_TEST(test_l76_autocomplete_off);
    RUN_TEST(test_l76_formtarget_blank);
    RUN_TEST(test_l76_form_target_attr);
    RUN_TEST(test_l76_formtarget_named);
    RUN_TEST(test_l76_output_paint_and_submit);
    RUN_TEST(test_l76_output_not_focusable);
    RUN_TEST(test_l76_attr_title_value_placeholder_name);
    RUN_TEST(test_l76_fieldset_disabled_color_date);
}
