/* Lot 76: systematic matrix of color/date validation and formtarget. */
#ifndef WE_FF2_AC_OFF
#define WE_FF2_AC_OFF 524288u
#define WE_FF2_FORMTARGET 1048576u
#define WE_FF2_TARGET_BLANK 2097152u
#endif

static we_forms_t g_l76s;
static const char *g_l76s_html;
static void l76s_render(const char *html) {
    we_forms_init(&g_l76s);
    we_set_forms(&g_doc, &g_l76s);
    g_l76s_html = html;
    TEST_ASSERT_EQUAL(0, we_render(&g_doc, html, (int)strlen(html), 480));
}
static void l76s_done(void) { we_set_forms(&g_doc, 0); g_l76s_html = 0; }
static int l76s_node(const char *id) { int i = we_find_id(&g_doc, id); return i > 0 ? i : 0; }
static int l76s_submit(void) {
    int k, x, y;
    for (k = 1; k < (int)g_doc.nnodes; k++)
        if (g_doc.nodes[k].form_type == WE_FORM_SUBMIT) break;
    x = g_doc.nodes[k].x + 2; y = g_doc.nodes[k].y + 2;
    return we_form_click(&g_doc, x, y);
}

static void test_l76_sys_1(void) {
    we_form_req_t q;
    char html[700];
    sprintf(html, "<form action=/sys1 method=GET>"
                  "<input id=sc1 type=color name=sc1 value=#0d1d29 autocomplete=off>"
                  "<input id=sd1 type=date name=sd1 value=1991-02-02>"
                  "<output id=so1 name=so1 value=R1>R1</output>"
                  "<input id=ss1 type=submit formtarget=_self value=Go></form>");
    l76s_render(html);
    TEST_ASSERT_EQUAL(WE_FORM_COLOR, g_doc.nodes[l76s_node("sc1")].form_type);
    TEST_ASSERT_EQUAL(WE_FORM_DATE, g_doc.nodes[l76s_node("sd1")].form_type);
    TEST_ASSERT_TRUE(g_doc.nodes[l76s_node("sc1")].form_flags2 & WE_FF2_AC_OFF);
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76s_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_EQUAL(0, q.target_blank);
    TEST_ASSERT_EQUAL_STRING("_self", q.target);
    TEST_ASSERT_NOT_NULL(strstr(q.url, "sc1=%230d1d29"));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "sd1=1991-02-02"));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "so1=R1"));
    l76s_done();
}

static void test_l76_sys_2(void) {
    we_form_req_t q;
    char html[700];
    sprintf(html, "<form action=/sys2 method=GET>"
                  "<input id=sc2 type=color name=sc2 value=#1a3a52 autocomplete=off>"
                  "<input id=sd2 type=date name=sd2 value=1992-03-03>"
                  "<output id=so2 name=so2 value=R2>R2</output>"
                  "<input id=ss2 type=submit formtarget=w2 value=Go></form>");
    l76s_render(html);
    TEST_ASSERT_EQUAL(WE_FORM_COLOR, g_doc.nodes[l76s_node("sc2")].form_type);
    TEST_ASSERT_EQUAL(WE_FORM_DATE, g_doc.nodes[l76s_node("sd2")].form_type);
    TEST_ASSERT_TRUE(g_doc.nodes[l76s_node("sc2")].form_flags2 & WE_FF2_AC_OFF);
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76s_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_EQUAL(0, q.target_blank);
    TEST_ASSERT_EQUAL_STRING("w2", q.target);
    TEST_ASSERT_NOT_NULL(strstr(q.url, "sc2=%231a3a52"));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "sd2=1992-03-03"));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "so2=R2"));
    l76s_done();
}

static void test_l76_sys_3(void) {
    we_form_req_t q;
    char html[700];
    sprintf(html, "<form action=/sys3 method=GET>"
                  "<input id=sc3 type=color name=sc3 value=#27577b autocomplete=off>"
                  "<input id=sd3 type=date name=sd3 value=1993-04-04>"
                  "<output id=so3 name=so3 value=R3>R3</output>"
                  "<input id=ss3 type=submit formtarget=_blank value=Go></form>");
    l76s_render(html);
    TEST_ASSERT_EQUAL(WE_FORM_COLOR, g_doc.nodes[l76s_node("sc3")].form_type);
    TEST_ASSERT_EQUAL(WE_FORM_DATE, g_doc.nodes[l76s_node("sd3")].form_type);
    TEST_ASSERT_TRUE(g_doc.nodes[l76s_node("sc3")].form_flags2 & WE_FF2_AC_OFF);
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76s_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_EQUAL(1, q.target_blank);
    TEST_ASSERT_EQUAL_STRING("_blank", q.target);
    TEST_ASSERT_NOT_NULL(strstr(q.url, "sc3=%2327577b"));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "sd3=1993-04-04"));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "so3=R3"));
    l76s_done();
}

static void test_l76_sys_4(void) {
    we_form_req_t q;
    char html[700];
    sprintf(html, "<form action=/sys4 method=GET>"
                  "<input id=sc4 type=color name=sc4 value=#3474a4 autocomplete=off>"
                  "<input id=sd4 type=date name=sd4 value=1994-05-05>"
                  "<output id=so4 name=so4 value=R4>R4</output>"
                  "<input id=ss4 type=submit formtarget=_self value=Go></form>");
    l76s_render(html);
    TEST_ASSERT_EQUAL(WE_FORM_COLOR, g_doc.nodes[l76s_node("sc4")].form_type);
    TEST_ASSERT_EQUAL(WE_FORM_DATE, g_doc.nodes[l76s_node("sd4")].form_type);
    TEST_ASSERT_TRUE(g_doc.nodes[l76s_node("sc4")].form_flags2 & WE_FF2_AC_OFF);
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76s_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_EQUAL(0, q.target_blank);
    TEST_ASSERT_EQUAL_STRING("_self", q.target);
    TEST_ASSERT_NOT_NULL(strstr(q.url, "sc4=%233474a4"));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "sd4=1994-05-05"));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "so4=R4"));
    l76s_done();
}

static void test_l76_sys_5(void) {
    we_form_req_t q;
    char html[700];
    sprintf(html, "<form action=/sys5 method=GET>"
                  "<input id=sc5 type=color name=sc5 value=#4191cd autocomplete=off>"
                  "<input id=sd5 type=date name=sd5 value=1995-06-06>"
                  "<output id=so5 name=so5 value=R5>R5</output>"
                  "<input id=ss5 type=submit formtarget=w5 value=Go></form>");
    l76s_render(html);
    TEST_ASSERT_EQUAL(WE_FORM_COLOR, g_doc.nodes[l76s_node("sc5")].form_type);
    TEST_ASSERT_EQUAL(WE_FORM_DATE, g_doc.nodes[l76s_node("sd5")].form_type);
    TEST_ASSERT_TRUE(g_doc.nodes[l76s_node("sc5")].form_flags2 & WE_FF2_AC_OFF);
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76s_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_EQUAL(0, q.target_blank);
    TEST_ASSERT_EQUAL_STRING("w5", q.target);
    TEST_ASSERT_NOT_NULL(strstr(q.url, "sc5=%234191cd"));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "sd5=1995-06-06"));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "so5=R5"));
    l76s_done();
}

static void test_l76_sys_6(void) {
    we_form_req_t q;
    char html[700];
    sprintf(html, "<form action=/sys6 method=GET>"
                  "<input id=sc6 type=color name=sc6 value=#4eaef6 autocomplete=off>"
                  "<input id=sd6 type=date name=sd6 value=1996-07-07>"
                  "<output id=so6 name=so6 value=R6>R6</output>"
                  "<input id=ss6 type=submit formtarget=_blank value=Go></form>");
    l76s_render(html);
    TEST_ASSERT_EQUAL(WE_FORM_COLOR, g_doc.nodes[l76s_node("sc6")].form_type);
    TEST_ASSERT_EQUAL(WE_FORM_DATE, g_doc.nodes[l76s_node("sd6")].form_type);
    TEST_ASSERT_TRUE(g_doc.nodes[l76s_node("sc6")].form_flags2 & WE_FF2_AC_OFF);
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76s_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_EQUAL(1, q.target_blank);
    TEST_ASSERT_EQUAL_STRING("_blank", q.target);
    TEST_ASSERT_NOT_NULL(strstr(q.url, "sc6=%234eaef6"));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "sd6=1996-07-07"));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "so6=R6"));
    l76s_done();
}

static void test_l76_sys_7(void) {
    we_form_req_t q;
    char html[700];
    sprintf(html, "<form action=/sys7 method=GET>"
                  "<input id=sc7 type=color name=sc7 value=#5bcb1f autocomplete=off>"
                  "<input id=sd7 type=date name=sd7 value=1997-08-08>"
                  "<output id=so7 name=so7 value=R7>R7</output>"
                  "<input id=ss7 type=submit formtarget=_self value=Go></form>");
    l76s_render(html);
    TEST_ASSERT_EQUAL(WE_FORM_COLOR, g_doc.nodes[l76s_node("sc7")].form_type);
    TEST_ASSERT_EQUAL(WE_FORM_DATE, g_doc.nodes[l76s_node("sd7")].form_type);
    TEST_ASSERT_TRUE(g_doc.nodes[l76s_node("sc7")].form_flags2 & WE_FF2_AC_OFF);
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76s_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_EQUAL(0, q.target_blank);
    TEST_ASSERT_EQUAL_STRING("_self", q.target);
    TEST_ASSERT_NOT_NULL(strstr(q.url, "sc7=%235bcb1f"));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "sd7=1997-08-08"));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "so7=R7"));
    l76s_done();
}

static void test_l76_sys_8(void) {
    we_form_req_t q;
    char html[700];
    sprintf(html, "<form action=/sys8 method=GET>"
                  "<input id=sc8 type=color name=sc8 value=#68e848 autocomplete=off>"
                  "<input id=sd8 type=date name=sd8 value=1998-09-09>"
                  "<output id=so8 name=so8 value=R8>R8</output>"
                  "<input id=ss8 type=submit formtarget=w8 value=Go></form>");
    l76s_render(html);
    TEST_ASSERT_EQUAL(WE_FORM_COLOR, g_doc.nodes[l76s_node("sc8")].form_type);
    TEST_ASSERT_EQUAL(WE_FORM_DATE, g_doc.nodes[l76s_node("sd8")].form_type);
    TEST_ASSERT_TRUE(g_doc.nodes[l76s_node("sc8")].form_flags2 & WE_FF2_AC_OFF);
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76s_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_EQUAL(0, q.target_blank);
    TEST_ASSERT_EQUAL_STRING("w8", q.target);
    TEST_ASSERT_NOT_NULL(strstr(q.url, "sc8=%2368e848"));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "sd8=1998-09-09"));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "so8=R8"));
    l76s_done();
}

static void test_l76_sys_9(void) {
    we_form_req_t q;
    char html[700];
    sprintf(html, "<form action=/sys9 method=GET>"
                  "<input id=sc9 type=color name=sc9 value=#750571 autocomplete=off>"
                  "<input id=sd9 type=date name=sd9 value=1999-10-10>"
                  "<output id=so9 name=so9 value=R9>R9</output>"
                  "<input id=ss9 type=submit formtarget=_blank value=Go></form>");
    l76s_render(html);
    TEST_ASSERT_EQUAL(WE_FORM_COLOR, g_doc.nodes[l76s_node("sc9")].form_type);
    TEST_ASSERT_EQUAL(WE_FORM_DATE, g_doc.nodes[l76s_node("sd9")].form_type);
    TEST_ASSERT_TRUE(g_doc.nodes[l76s_node("sc9")].form_flags2 & WE_FF2_AC_OFF);
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76s_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_EQUAL(1, q.target_blank);
    TEST_ASSERT_EQUAL_STRING("_blank", q.target);
    TEST_ASSERT_NOT_NULL(strstr(q.url, "sc9=%23750571"));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "sd9=1999-10-10"));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "so9=R9"));
    l76s_done();
}

static void test_l76_sys_10(void) {
    we_form_req_t q;
    char html[700];
    sprintf(html, "<form action=/sys10 method=GET>"
                  "<input id=sc10 type=color name=sc10 value=#82229a autocomplete=off>"
                  "<input id=sd10 type=date name=sd10 value=2000-11-11>"
                  "<output id=so10 name=so10 value=R10>R10</output>"
                  "<input id=ss10 type=submit formtarget=_self value=Go></form>");
    l76s_render(html);
    TEST_ASSERT_EQUAL(WE_FORM_COLOR, g_doc.nodes[l76s_node("sc10")].form_type);
    TEST_ASSERT_EQUAL(WE_FORM_DATE, g_doc.nodes[l76s_node("sd10")].form_type);
    TEST_ASSERT_TRUE(g_doc.nodes[l76s_node("sc10")].form_flags2 & WE_FF2_AC_OFF);
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76s_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_EQUAL(0, q.target_blank);
    TEST_ASSERT_EQUAL_STRING("_self", q.target);
    TEST_ASSERT_NOT_NULL(strstr(q.url, "sc10=%2382229a"));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "sd10=2000-11-11"));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "so10=R10"));
    l76s_done();
}

static void test_l76_sys_11(void) {
    we_form_req_t q;
    char html[700];
    sprintf(html, "<form action=/sys11 method=GET>"
                  "<input id=sc11 type=color name=sc11 value=#8f3fc3 autocomplete=off>"
                  "<input id=sd11 type=date name=sd11 value=2001-12-12>"
                  "<output id=so11 name=so11 value=R11>R11</output>"
                  "<input id=ss11 type=submit formtarget=w11 value=Go></form>");
    l76s_render(html);
    TEST_ASSERT_EQUAL(WE_FORM_COLOR, g_doc.nodes[l76s_node("sc11")].form_type);
    TEST_ASSERT_EQUAL(WE_FORM_DATE, g_doc.nodes[l76s_node("sd11")].form_type);
    TEST_ASSERT_TRUE(g_doc.nodes[l76s_node("sc11")].form_flags2 & WE_FF2_AC_OFF);
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76s_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_EQUAL(0, q.target_blank);
    TEST_ASSERT_EQUAL_STRING("w11", q.target);
    TEST_ASSERT_NOT_NULL(strstr(q.url, "sc11=%238f3fc3"));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "sd11=2001-12-12"));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "so11=R11"));
    l76s_done();
}

static void test_l76_sys_12(void) {
    we_form_req_t q;
    char html[700];
    sprintf(html, "<form action=/sys12 method=GET>"
                  "<input id=sc12 type=color name=sc12 value=#9c5cec autocomplete=off>"
                  "<input id=sd12 type=date name=sd12 value=2002-01-13>"
                  "<output id=so12 name=so12 value=R12>R12</output>"
                  "<input id=ss12 type=submit formtarget=_blank value=Go></form>");
    l76s_render(html);
    TEST_ASSERT_EQUAL(WE_FORM_COLOR, g_doc.nodes[l76s_node("sc12")].form_type);
    TEST_ASSERT_EQUAL(WE_FORM_DATE, g_doc.nodes[l76s_node("sd12")].form_type);
    TEST_ASSERT_TRUE(g_doc.nodes[l76s_node("sc12")].form_flags2 & WE_FF2_AC_OFF);
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76s_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_EQUAL(1, q.target_blank);
    TEST_ASSERT_EQUAL_STRING("_blank", q.target);
    TEST_ASSERT_NOT_NULL(strstr(q.url, "sc12=%239c5cec"));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "sd12=2002-01-13"));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "so12=R12"));
    l76s_done();
}

static void test_l76_sys_13(void) {
    we_form_req_t q;
    char html[700];
    sprintf(html, "<form action=/sys13 method=GET>"
                  "<input id=sc13 type=color name=sc13 value=#a97915 autocomplete=off>"
                  "<input id=sd13 type=date name=sd13 value=2003-02-14>"
                  "<output id=so13 name=so13 value=R13>R13</output>"
                  "<input id=ss13 type=submit formtarget=_self value=Go></form>");
    l76s_render(html);
    TEST_ASSERT_EQUAL(WE_FORM_COLOR, g_doc.nodes[l76s_node("sc13")].form_type);
    TEST_ASSERT_EQUAL(WE_FORM_DATE, g_doc.nodes[l76s_node("sd13")].form_type);
    TEST_ASSERT_TRUE(g_doc.nodes[l76s_node("sc13")].form_flags2 & WE_FF2_AC_OFF);
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76s_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_EQUAL(0, q.target_blank);
    TEST_ASSERT_EQUAL_STRING("_self", q.target);
    TEST_ASSERT_NOT_NULL(strstr(q.url, "sc13=%23a97915"));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "sd13=2003-02-14"));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "so13=R13"));
    l76s_done();
}

static void test_l76_sys_14(void) {
    we_form_req_t q;
    char html[700];
    sprintf(html, "<form action=/sys14 method=GET>"
                  "<input id=sc14 type=color name=sc14 value=#b6963e autocomplete=off>"
                  "<input id=sd14 type=date name=sd14 value=2004-03-15>"
                  "<output id=so14 name=so14 value=R14>R14</output>"
                  "<input id=ss14 type=submit formtarget=w14 value=Go></form>");
    l76s_render(html);
    TEST_ASSERT_EQUAL(WE_FORM_COLOR, g_doc.nodes[l76s_node("sc14")].form_type);
    TEST_ASSERT_EQUAL(WE_FORM_DATE, g_doc.nodes[l76s_node("sd14")].form_type);
    TEST_ASSERT_TRUE(g_doc.nodes[l76s_node("sc14")].form_flags2 & WE_FF2_AC_OFF);
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76s_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_EQUAL(0, q.target_blank);
    TEST_ASSERT_EQUAL_STRING("w14", q.target);
    TEST_ASSERT_NOT_NULL(strstr(q.url, "sc14=%23b6963e"));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "sd14=2004-03-15"));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "so14=R14"));
    l76s_done();
}

static void test_l76_sys_15(void) {
    we_form_req_t q;
    char html[700];
    sprintf(html, "<form action=/sys15 method=GET>"
                  "<input id=sc15 type=color name=sc15 value=#c3b367 autocomplete=off>"
                  "<input id=sd15 type=date name=sd15 value=2005-04-16>"
                  "<output id=so15 name=so15 value=R15>R15</output>"
                  "<input id=ss15 type=submit formtarget=_blank value=Go></form>");
    l76s_render(html);
    TEST_ASSERT_EQUAL(WE_FORM_COLOR, g_doc.nodes[l76s_node("sc15")].form_type);
    TEST_ASSERT_EQUAL(WE_FORM_DATE, g_doc.nodes[l76s_node("sd15")].form_type);
    TEST_ASSERT_TRUE(g_doc.nodes[l76s_node("sc15")].form_flags2 & WE_FF2_AC_OFF);
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76s_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_EQUAL(1, q.target_blank);
    TEST_ASSERT_EQUAL_STRING("_blank", q.target);
    TEST_ASSERT_NOT_NULL(strstr(q.url, "sc15=%23c3b367"));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "sd15=2005-04-16"));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "so15=R15"));
    l76s_done();
}

static void test_l76_sys_16(void) {
    we_form_req_t q;
    char html[700];
    sprintf(html, "<form action=/sys16 method=GET>"
                  "<input id=sc16 type=color name=sc16 value=#d0d090 autocomplete=off>"
                  "<input id=sd16 type=date name=sd16 value=2006-05-17>"
                  "<output id=so16 name=so16 value=R16>R16</output>"
                  "<input id=ss16 type=submit formtarget=_self value=Go></form>");
    l76s_render(html);
    TEST_ASSERT_EQUAL(WE_FORM_COLOR, g_doc.nodes[l76s_node("sc16")].form_type);
    TEST_ASSERT_EQUAL(WE_FORM_DATE, g_doc.nodes[l76s_node("sd16")].form_type);
    TEST_ASSERT_TRUE(g_doc.nodes[l76s_node("sc16")].form_flags2 & WE_FF2_AC_OFF);
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76s_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_EQUAL(0, q.target_blank);
    TEST_ASSERT_EQUAL_STRING("_self", q.target);
    TEST_ASSERT_NOT_NULL(strstr(q.url, "sc16=%23d0d090"));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "sd16=2006-05-17"));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "so16=R16"));
    l76s_done();
}

static void test_l76_sys_17(void) {
    we_form_req_t q;
    char html[700];
    sprintf(html, "<form action=/sys17 method=GET>"
                  "<input id=sc17 type=color name=sc17 value=#ddedb9 autocomplete=off>"
                  "<input id=sd17 type=date name=sd17 value=2007-06-18>"
                  "<output id=so17 name=so17 value=R17>R17</output>"
                  "<input id=ss17 type=submit formtarget=w17 value=Go></form>");
    l76s_render(html);
    TEST_ASSERT_EQUAL(WE_FORM_COLOR, g_doc.nodes[l76s_node("sc17")].form_type);
    TEST_ASSERT_EQUAL(WE_FORM_DATE, g_doc.nodes[l76s_node("sd17")].form_type);
    TEST_ASSERT_TRUE(g_doc.nodes[l76s_node("sc17")].form_flags2 & WE_FF2_AC_OFF);
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76s_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_EQUAL(0, q.target_blank);
    TEST_ASSERT_EQUAL_STRING("w17", q.target);
    TEST_ASSERT_NOT_NULL(strstr(q.url, "sc17=%23ddedb9"));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "sd17=2007-06-18"));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "so17=R17"));
    l76s_done();
}

static void test_l76_sys_18(void) {
    we_form_req_t q;
    char html[700];
    sprintf(html, "<form action=/sys18 method=GET>"
                  "<input id=sc18 type=color name=sc18 value=#ea0ae2 autocomplete=off>"
                  "<input id=sd18 type=date name=sd18 value=2008-07-19>"
                  "<output id=so18 name=so18 value=R18>R18</output>"
                  "<input id=ss18 type=submit formtarget=_blank value=Go></form>");
    l76s_render(html);
    TEST_ASSERT_EQUAL(WE_FORM_COLOR, g_doc.nodes[l76s_node("sc18")].form_type);
    TEST_ASSERT_EQUAL(WE_FORM_DATE, g_doc.nodes[l76s_node("sd18")].form_type);
    TEST_ASSERT_TRUE(g_doc.nodes[l76s_node("sc18")].form_flags2 & WE_FF2_AC_OFF);
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76s_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_EQUAL(1, q.target_blank);
    TEST_ASSERT_EQUAL_STRING("_blank", q.target);
    TEST_ASSERT_NOT_NULL(strstr(q.url, "sc18=%23ea0ae2"));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "sd18=2008-07-19"));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "so18=R18"));
    l76s_done();
}

static void test_l76_sys_19(void) {
    we_form_req_t q;
    char html[700];
    sprintf(html, "<form action=/sys19 method=GET>"
                  "<input id=sc19 type=color name=sc19 value=#f7270b autocomplete=off>"
                  "<input id=sd19 type=date name=sd19 value=2009-08-20>"
                  "<output id=so19 name=so19 value=R19>R19</output>"
                  "<input id=ss19 type=submit formtarget=_self value=Go></form>");
    l76s_render(html);
    TEST_ASSERT_EQUAL(WE_FORM_COLOR, g_doc.nodes[l76s_node("sc19")].form_type);
    TEST_ASSERT_EQUAL(WE_FORM_DATE, g_doc.nodes[l76s_node("sd19")].form_type);
    TEST_ASSERT_TRUE(g_doc.nodes[l76s_node("sc19")].form_flags2 & WE_FF2_AC_OFF);
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76s_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_EQUAL(0, q.target_blank);
    TEST_ASSERT_EQUAL_STRING("_self", q.target);
    TEST_ASSERT_NOT_NULL(strstr(q.url, "sc19=%23f7270b"));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "sd19=2009-08-20"));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "so19=R19"));
    l76s_done();
}

static void test_l76_sys_20(void) {
    we_form_req_t q;
    char html[700];
    sprintf(html, "<form action=/sys20 method=GET>"
                  "<input id=sc20 type=color name=sc20 value=#044434 autocomplete=off>"
                  "<input id=sd20 type=date name=sd20 value=2010-09-21>"
                  "<output id=so20 name=so20 value=R20>R20</output>"
                  "<input id=ss20 type=submit formtarget=w20 value=Go></form>");
    l76s_render(html);
    TEST_ASSERT_EQUAL(WE_FORM_COLOR, g_doc.nodes[l76s_node("sc20")].form_type);
    TEST_ASSERT_EQUAL(WE_FORM_DATE, g_doc.nodes[l76s_node("sd20")].form_type);
    TEST_ASSERT_TRUE(g_doc.nodes[l76s_node("sc20")].form_flags2 & WE_FF2_AC_OFF);
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76s_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_EQUAL(0, q.target_blank);
    TEST_ASSERT_EQUAL_STRING("w20", q.target);
    TEST_ASSERT_NOT_NULL(strstr(q.url, "sc20=%23044434"));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "sd20=2010-09-21"));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "so20=R20"));
    l76s_done();
}

static void test_l76_sys_21(void) {
    we_form_req_t q;
    char html[700];
    sprintf(html, "<form action=/sys21 method=GET>"
                  "<input id=sc21 type=color name=sc21 value=#11615d autocomplete=off>"
                  "<input id=sd21 type=date name=sd21 value=2011-10-22>"
                  "<output id=so21 name=so21 value=R21>R21</output>"
                  "<input id=ss21 type=submit formtarget=_blank value=Go></form>");
    l76s_render(html);
    TEST_ASSERT_EQUAL(WE_FORM_COLOR, g_doc.nodes[l76s_node("sc21")].form_type);
    TEST_ASSERT_EQUAL(WE_FORM_DATE, g_doc.nodes[l76s_node("sd21")].form_type);
    TEST_ASSERT_TRUE(g_doc.nodes[l76s_node("sc21")].form_flags2 & WE_FF2_AC_OFF);
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76s_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_EQUAL(1, q.target_blank);
    TEST_ASSERT_EQUAL_STRING("_blank", q.target);
    TEST_ASSERT_NOT_NULL(strstr(q.url, "sc21=%2311615d"));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "sd21=2011-10-22"));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "so21=R21"));
    l76s_done();
}

static void test_l76_sys_22(void) {
    we_form_req_t q;
    char html[700];
    sprintf(html, "<form action=/sys22 method=GET>"
                  "<input id=sc22 type=color name=sc22 value=#1e7e86 autocomplete=off>"
                  "<input id=sd22 type=date name=sd22 value=2012-11-23>"
                  "<output id=so22 name=so22 value=R22>R22</output>"
                  "<input id=ss22 type=submit formtarget=_self value=Go></form>");
    l76s_render(html);
    TEST_ASSERT_EQUAL(WE_FORM_COLOR, g_doc.nodes[l76s_node("sc22")].form_type);
    TEST_ASSERT_EQUAL(WE_FORM_DATE, g_doc.nodes[l76s_node("sd22")].form_type);
    TEST_ASSERT_TRUE(g_doc.nodes[l76s_node("sc22")].form_flags2 & WE_FF2_AC_OFF);
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76s_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_EQUAL(0, q.target_blank);
    TEST_ASSERT_EQUAL_STRING("_self", q.target);
    TEST_ASSERT_NOT_NULL(strstr(q.url, "sc22=%231e7e86"));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "sd22=2012-11-23"));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "so22=R22"));
    l76s_done();
}

static void test_l76_sys_23(void) {
    we_form_req_t q;
    char html[700];
    sprintf(html, "<form action=/sys23 method=GET>"
                  "<input id=sc23 type=color name=sc23 value=#2b9baf autocomplete=off>"
                  "<input id=sd23 type=date name=sd23 value=2013-12-24>"
                  "<output id=so23 name=so23 value=R23>R23</output>"
                  "<input id=ss23 type=submit formtarget=w23 value=Go></form>");
    l76s_render(html);
    TEST_ASSERT_EQUAL(WE_FORM_COLOR, g_doc.nodes[l76s_node("sc23")].form_type);
    TEST_ASSERT_EQUAL(WE_FORM_DATE, g_doc.nodes[l76s_node("sd23")].form_type);
    TEST_ASSERT_TRUE(g_doc.nodes[l76s_node("sc23")].form_flags2 & WE_FF2_AC_OFF);
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76s_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_EQUAL(0, q.target_blank);
    TEST_ASSERT_EQUAL_STRING("w23", q.target);
    TEST_ASSERT_NOT_NULL(strstr(q.url, "sc23=%232b9baf"));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "sd23=2013-12-24"));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "so23=R23"));
    l76s_done();
}

static void test_l76_sys_24(void) {
    we_form_req_t q;
    char html[700];
    sprintf(html, "<form action=/sys24 method=GET>"
                  "<input id=sc24 type=color name=sc24 value=#38b8d8 autocomplete=off>"
                  "<input id=sd24 type=date name=sd24 value=2014-01-25>"
                  "<output id=so24 name=so24 value=R24>R24</output>"
                  "<input id=ss24 type=submit formtarget=_blank value=Go></form>");
    l76s_render(html);
    TEST_ASSERT_EQUAL(WE_FORM_COLOR, g_doc.nodes[l76s_node("sc24")].form_type);
    TEST_ASSERT_EQUAL(WE_FORM_DATE, g_doc.nodes[l76s_node("sd24")].form_type);
    TEST_ASSERT_TRUE(g_doc.nodes[l76s_node("sc24")].form_flags2 & WE_FF2_AC_OFF);
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76s_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_EQUAL(1, q.target_blank);
    TEST_ASSERT_EQUAL_STRING("_blank", q.target);
    TEST_ASSERT_NOT_NULL(strstr(q.url, "sc24=%2338b8d8"));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "sd24=2014-01-25"));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "so24=R24"));
    l76s_done();
}

static void test_l76_sys_25(void) {
    we_form_req_t q;
    char html[700];
    sprintf(html, "<form action=/sys25 method=GET>"
                  "<input id=sc25 type=color name=sc25 value=#45d501 autocomplete=off>"
                  "<input id=sd25 type=date name=sd25 value=2015-02-26>"
                  "<output id=so25 name=so25 value=R25>R25</output>"
                  "<input id=ss25 type=submit formtarget=_self value=Go></form>");
    l76s_render(html);
    TEST_ASSERT_EQUAL(WE_FORM_COLOR, g_doc.nodes[l76s_node("sc25")].form_type);
    TEST_ASSERT_EQUAL(WE_FORM_DATE, g_doc.nodes[l76s_node("sd25")].form_type);
    TEST_ASSERT_TRUE(g_doc.nodes[l76s_node("sc25")].form_flags2 & WE_FF2_AC_OFF);
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76s_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_EQUAL(0, q.target_blank);
    TEST_ASSERT_EQUAL_STRING("_self", q.target);
    TEST_ASSERT_NOT_NULL(strstr(q.url, "sc25=%2345d501"));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "sd25=2015-02-26"));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "so25=R25"));
    l76s_done();
}

static void test_l76_sys_26(void) {
    we_form_req_t q;
    char html[700];
    sprintf(html, "<form action=/sys26 method=GET>"
                  "<input id=sc26 type=color name=sc26 value=#52f22a autocomplete=off>"
                  "<input id=sd26 type=date name=sd26 value=2016-03-27>"
                  "<output id=so26 name=so26 value=R26>R26</output>"
                  "<input id=ss26 type=submit formtarget=w26 value=Go></form>");
    l76s_render(html);
    TEST_ASSERT_EQUAL(WE_FORM_COLOR, g_doc.nodes[l76s_node("sc26")].form_type);
    TEST_ASSERT_EQUAL(WE_FORM_DATE, g_doc.nodes[l76s_node("sd26")].form_type);
    TEST_ASSERT_TRUE(g_doc.nodes[l76s_node("sc26")].form_flags2 & WE_FF2_AC_OFF);
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76s_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_EQUAL(0, q.target_blank);
    TEST_ASSERT_EQUAL_STRING("w26", q.target);
    TEST_ASSERT_NOT_NULL(strstr(q.url, "sc26=%2352f22a"));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "sd26=2016-03-27"));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "so26=R26"));
    l76s_done();
}

static void test_l76_sys_27(void) {
    we_form_req_t q;
    char html[700];
    sprintf(html, "<form action=/sys27 method=GET>"
                  "<input id=sc27 type=color name=sc27 value=#5f0f53 autocomplete=off>"
                  "<input id=sd27 type=date name=sd27 value=2017-04-01>"
                  "<output id=so27 name=so27 value=R27>R27</output>"
                  "<input id=ss27 type=submit formtarget=_blank value=Go></form>");
    l76s_render(html);
    TEST_ASSERT_EQUAL(WE_FORM_COLOR, g_doc.nodes[l76s_node("sc27")].form_type);
    TEST_ASSERT_EQUAL(WE_FORM_DATE, g_doc.nodes[l76s_node("sd27")].form_type);
    TEST_ASSERT_TRUE(g_doc.nodes[l76s_node("sc27")].form_flags2 & WE_FF2_AC_OFF);
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76s_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_EQUAL(1, q.target_blank);
    TEST_ASSERT_EQUAL_STRING("_blank", q.target);
    TEST_ASSERT_NOT_NULL(strstr(q.url, "sc27=%235f0f53"));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "sd27=2017-04-01"));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "so27=R27"));
    l76s_done();
}

static void test_l76_sys_28(void) {
    we_form_req_t q;
    char html[700];
    sprintf(html, "<form action=/sys28 method=GET>"
                  "<input id=sc28 type=color name=sc28 value=#6c2c7c autocomplete=off>"
                  "<input id=sd28 type=date name=sd28 value=2018-05-02>"
                  "<output id=so28 name=so28 value=R28>R28</output>"
                  "<input id=ss28 type=submit formtarget=_self value=Go></form>");
    l76s_render(html);
    TEST_ASSERT_EQUAL(WE_FORM_COLOR, g_doc.nodes[l76s_node("sc28")].form_type);
    TEST_ASSERT_EQUAL(WE_FORM_DATE, g_doc.nodes[l76s_node("sd28")].form_type);
    TEST_ASSERT_TRUE(g_doc.nodes[l76s_node("sc28")].form_flags2 & WE_FF2_AC_OFF);
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76s_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_EQUAL(0, q.target_blank);
    TEST_ASSERT_EQUAL_STRING("_self", q.target);
    TEST_ASSERT_NOT_NULL(strstr(q.url, "sc28=%236c2c7c"));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "sd28=2018-05-02"));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "so28=R28"));
    l76s_done();
}

static void test_l76_sys_29(void) {
    we_form_req_t q;
    char html[700];
    sprintf(html, "<form action=/sys29 method=GET>"
                  "<input id=sc29 type=color name=sc29 value=#7949a5 autocomplete=off>"
                  "<input id=sd29 type=date name=sd29 value=2019-06-03>"
                  "<output id=so29 name=so29 value=R29>R29</output>"
                  "<input id=ss29 type=submit formtarget=w29 value=Go></form>");
    l76s_render(html);
    TEST_ASSERT_EQUAL(WE_FORM_COLOR, g_doc.nodes[l76s_node("sc29")].form_type);
    TEST_ASSERT_EQUAL(WE_FORM_DATE, g_doc.nodes[l76s_node("sd29")].form_type);
    TEST_ASSERT_TRUE(g_doc.nodes[l76s_node("sc29")].form_flags2 & WE_FF2_AC_OFF);
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76s_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_EQUAL(0, q.target_blank);
    TEST_ASSERT_EQUAL_STRING("w29", q.target);
    TEST_ASSERT_NOT_NULL(strstr(q.url, "sc29=%237949a5"));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "sd29=2019-06-03"));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "so29=R29"));
    l76s_done();
}

static void test_l76_sys_30(void) {
    we_form_req_t q;
    char html[700];
    sprintf(html, "<form action=/sys30 method=GET>"
                  "<input id=sc30 type=color name=sc30 value=#8666ce autocomplete=off>"
                  "<input id=sd30 type=date name=sd30 value=2020-07-04>"
                  "<output id=so30 name=so30 value=R30>R30</output>"
                  "<input id=ss30 type=submit formtarget=_blank value=Go></form>");
    l76s_render(html);
    TEST_ASSERT_EQUAL(WE_FORM_COLOR, g_doc.nodes[l76s_node("sc30")].form_type);
    TEST_ASSERT_EQUAL(WE_FORM_DATE, g_doc.nodes[l76s_node("sd30")].form_type);
    TEST_ASSERT_TRUE(g_doc.nodes[l76s_node("sc30")].form_flags2 & WE_FF2_AC_OFF);
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76s_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_EQUAL(1, q.target_blank);
    TEST_ASSERT_EQUAL_STRING("_blank", q.target);
    TEST_ASSERT_NOT_NULL(strstr(q.url, "sc30=%238666ce"));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "sd30=2020-07-04"));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "so30=R30"));
    l76s_done();
}

static void test_l76_sys_31(void) {
    we_form_req_t q;
    char html[700];
    sprintf(html, "<form action=/sys31 method=GET>"
                  "<input id=sc31 type=color name=sc31 value=#9383f7 autocomplete=off>"
                  "<input id=sd31 type=date name=sd31 value=2021-08-05>"
                  "<output id=so31 name=so31 value=R31>R31</output>"
                  "<input id=ss31 type=submit formtarget=_self value=Go></form>");
    l76s_render(html);
    TEST_ASSERT_EQUAL(WE_FORM_COLOR, g_doc.nodes[l76s_node("sc31")].form_type);
    TEST_ASSERT_EQUAL(WE_FORM_DATE, g_doc.nodes[l76s_node("sd31")].form_type);
    TEST_ASSERT_TRUE(g_doc.nodes[l76s_node("sc31")].form_flags2 & WE_FF2_AC_OFF);
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76s_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_EQUAL(0, q.target_blank);
    TEST_ASSERT_EQUAL_STRING("_self", q.target);
    TEST_ASSERT_NOT_NULL(strstr(q.url, "sc31=%239383f7"));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "sd31=2021-08-05"));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "so31=R31"));
    l76s_done();
}

static void test_l76_sys_32(void) {
    we_form_req_t q;
    char html[700];
    sprintf(html, "<form action=/sys32 method=GET>"
                  "<input id=sc32 type=color name=sc32 value=#a0a020 autocomplete=off>"
                  "<input id=sd32 type=date name=sd32 value=2022-09-06>"
                  "<output id=so32 name=so32 value=R32>R32</output>"
                  "<input id=ss32 type=submit formtarget=w32 value=Go></form>");
    l76s_render(html);
    TEST_ASSERT_EQUAL(WE_FORM_COLOR, g_doc.nodes[l76s_node("sc32")].form_type);
    TEST_ASSERT_EQUAL(WE_FORM_DATE, g_doc.nodes[l76s_node("sd32")].form_type);
    TEST_ASSERT_TRUE(g_doc.nodes[l76s_node("sc32")].form_flags2 & WE_FF2_AC_OFF);
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76s_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_EQUAL(0, q.target_blank);
    TEST_ASSERT_EQUAL_STRING("w32", q.target);
    TEST_ASSERT_NOT_NULL(strstr(q.url, "sc32=%23a0a020"));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "sd32=2022-09-06"));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "so32=R32"));
    l76s_done();
}

static void test_l76_sys_33(void) {
    we_form_req_t q;
    char html[700];
    sprintf(html, "<form action=/sys33 method=GET>"
                  "<input id=sc33 type=color name=sc33 value=#adbd49 autocomplete=off>"
                  "<input id=sd33 type=date name=sd33 value=2023-10-07>"
                  "<output id=so33 name=so33 value=R33>R33</output>"
                  "<input id=ss33 type=submit formtarget=_blank value=Go></form>");
    l76s_render(html);
    TEST_ASSERT_EQUAL(WE_FORM_COLOR, g_doc.nodes[l76s_node("sc33")].form_type);
    TEST_ASSERT_EQUAL(WE_FORM_DATE, g_doc.nodes[l76s_node("sd33")].form_type);
    TEST_ASSERT_TRUE(g_doc.nodes[l76s_node("sc33")].form_flags2 & WE_FF2_AC_OFF);
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76s_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_EQUAL(1, q.target_blank);
    TEST_ASSERT_EQUAL_STRING("_blank", q.target);
    TEST_ASSERT_NOT_NULL(strstr(q.url, "sc33=%23adbd49"));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "sd33=2023-10-07"));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "so33=R33"));
    l76s_done();
}

static void test_l76_sys_34(void) {
    we_form_req_t q;
    char html[700];
    sprintf(html, "<form action=/sys34 method=GET>"
                  "<input id=sc34 type=color name=sc34 value=#bada72 autocomplete=off>"
                  "<input id=sd34 type=date name=sd34 value=2024-11-08>"
                  "<output id=so34 name=so34 value=R34>R34</output>"
                  "<input id=ss34 type=submit formtarget=_self value=Go></form>");
    l76s_render(html);
    TEST_ASSERT_EQUAL(WE_FORM_COLOR, g_doc.nodes[l76s_node("sc34")].form_type);
    TEST_ASSERT_EQUAL(WE_FORM_DATE, g_doc.nodes[l76s_node("sd34")].form_type);
    TEST_ASSERT_TRUE(g_doc.nodes[l76s_node("sc34")].form_flags2 & WE_FF2_AC_OFF);
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76s_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_EQUAL(0, q.target_blank);
    TEST_ASSERT_EQUAL_STRING("_self", q.target);
    TEST_ASSERT_NOT_NULL(strstr(q.url, "sc34=%23bada72"));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "sd34=2024-11-08"));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "so34=R34"));
    l76s_done();
}

static void test_l76_sys_35(void) {
    we_form_req_t q;
    char html[700];
    sprintf(html, "<form action=/sys35 method=GET>"
                  "<input id=sc35 type=color name=sc35 value=#c7f79b autocomplete=off>"
                  "<input id=sd35 type=date name=sd35 value=2025-12-09>"
                  "<output id=so35 name=so35 value=R35>R35</output>"
                  "<input id=ss35 type=submit formtarget=w35 value=Go></form>");
    l76s_render(html);
    TEST_ASSERT_EQUAL(WE_FORM_COLOR, g_doc.nodes[l76s_node("sc35")].form_type);
    TEST_ASSERT_EQUAL(WE_FORM_DATE, g_doc.nodes[l76s_node("sd35")].form_type);
    TEST_ASSERT_TRUE(g_doc.nodes[l76s_node("sc35")].form_flags2 & WE_FF2_AC_OFF);
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76s_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_EQUAL(0, q.target_blank);
    TEST_ASSERT_EQUAL_STRING("w35", q.target);
    TEST_ASSERT_NOT_NULL(strstr(q.url, "sc35=%23c7f79b"));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "sd35=2025-12-09"));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "so35=R35"));
    l76s_done();
}

static void test_l76_sys_36(void) {
    we_form_req_t q;
    char html[700];
    sprintf(html, "<form action=/sys36 method=GET>"
                  "<input id=sc36 type=color name=sc36 value=#d414c4 autocomplete=off>"
                  "<input id=sd36 type=date name=sd36 value=2026-01-10>"
                  "<output id=so36 name=so36 value=R36>R36</output>"
                  "<input id=ss36 type=submit formtarget=_blank value=Go></form>");
    l76s_render(html);
    TEST_ASSERT_EQUAL(WE_FORM_COLOR, g_doc.nodes[l76s_node("sc36")].form_type);
    TEST_ASSERT_EQUAL(WE_FORM_DATE, g_doc.nodes[l76s_node("sd36")].form_type);
    TEST_ASSERT_TRUE(g_doc.nodes[l76s_node("sc36")].form_flags2 & WE_FF2_AC_OFF);
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76s_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_EQUAL(1, q.target_blank);
    TEST_ASSERT_EQUAL_STRING("_blank", q.target);
    TEST_ASSERT_NOT_NULL(strstr(q.url, "sc36=%23d414c4"));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "sd36=2026-01-10"));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "so36=R36"));
    l76s_done();
}

static void test_l76_sys_37(void) {
    we_form_req_t q;
    char html[700];
    sprintf(html, "<form action=/sys37 method=GET>"
                  "<input id=sc37 type=color name=sc37 value=#e131ed autocomplete=off>"
                  "<input id=sd37 type=date name=sd37 value=2027-02-11>"
                  "<output id=so37 name=so37 value=R37>R37</output>"
                  "<input id=ss37 type=submit formtarget=_self value=Go></form>");
    l76s_render(html);
    TEST_ASSERT_EQUAL(WE_FORM_COLOR, g_doc.nodes[l76s_node("sc37")].form_type);
    TEST_ASSERT_EQUAL(WE_FORM_DATE, g_doc.nodes[l76s_node("sd37")].form_type);
    TEST_ASSERT_TRUE(g_doc.nodes[l76s_node("sc37")].form_flags2 & WE_FF2_AC_OFF);
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76s_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_EQUAL(0, q.target_blank);
    TEST_ASSERT_EQUAL_STRING("_self", q.target);
    TEST_ASSERT_NOT_NULL(strstr(q.url, "sc37=%23e131ed"));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "sd37=2027-02-11"));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "so37=R37"));
    l76s_done();
}

static void test_l76_sys_38(void) {
    we_form_req_t q;
    char html[700];
    sprintf(html, "<form action=/sys38 method=GET>"
                  "<input id=sc38 type=color name=sc38 value=#ee4e16 autocomplete=off>"
                  "<input id=sd38 type=date name=sd38 value=2028-03-12>"
                  "<output id=so38 name=so38 value=R38>R38</output>"
                  "<input id=ss38 type=submit formtarget=w38 value=Go></form>");
    l76s_render(html);
    TEST_ASSERT_EQUAL(WE_FORM_COLOR, g_doc.nodes[l76s_node("sc38")].form_type);
    TEST_ASSERT_EQUAL(WE_FORM_DATE, g_doc.nodes[l76s_node("sd38")].form_type);
    TEST_ASSERT_TRUE(g_doc.nodes[l76s_node("sc38")].form_flags2 & WE_FF2_AC_OFF);
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76s_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_EQUAL(0, q.target_blank);
    TEST_ASSERT_EQUAL_STRING("w38", q.target);
    TEST_ASSERT_NOT_NULL(strstr(q.url, "sc38=%23ee4e16"));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "sd38=2028-03-12"));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "so38=R38"));
    l76s_done();
}

static void test_l76_sys_39(void) {
    we_form_req_t q;
    char html[700];
    sprintf(html, "<form action=/sys39 method=GET>"
                  "<input id=sc39 type=color name=sc39 value=#fb6b3f autocomplete=off>"
                  "<input id=sd39 type=date name=sd39 value=2029-04-13>"
                  "<output id=so39 name=so39 value=R39>R39</output>"
                  "<input id=ss39 type=submit formtarget=_blank value=Go></form>");
    l76s_render(html);
    TEST_ASSERT_EQUAL(WE_FORM_COLOR, g_doc.nodes[l76s_node("sc39")].form_type);
    TEST_ASSERT_EQUAL(WE_FORM_DATE, g_doc.nodes[l76s_node("sd39")].form_type);
    TEST_ASSERT_TRUE(g_doc.nodes[l76s_node("sc39")].form_flags2 & WE_FF2_AC_OFF);
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76s_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_EQUAL(1, q.target_blank);
    TEST_ASSERT_EQUAL_STRING("_blank", q.target);
    TEST_ASSERT_NOT_NULL(strstr(q.url, "sc39=%23fb6b3f"));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "sd39=2029-04-13"));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "so39=R39"));
    l76s_done();
}

static void test_l76_sys_40(void) {
    we_form_req_t q;
    char html[700];
    sprintf(html, "<form action=/sys40 method=GET>"
                  "<input id=sc40 type=color name=sc40 value=#088868 autocomplete=off>"
                  "<input id=sd40 type=date name=sd40 value=1990-05-14>"
                  "<output id=so40 name=so40 value=R40>R40</output>"
                  "<input id=ss40 type=submit formtarget=_self value=Go></form>");
    l76s_render(html);
    TEST_ASSERT_EQUAL(WE_FORM_COLOR, g_doc.nodes[l76s_node("sc40")].form_type);
    TEST_ASSERT_EQUAL(WE_FORM_DATE, g_doc.nodes[l76s_node("sd40")].form_type);
    TEST_ASSERT_TRUE(g_doc.nodes[l76s_node("sc40")].form_flags2 & WE_FF2_AC_OFF);
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76s_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_EQUAL(0, q.target_blank);
    TEST_ASSERT_EQUAL_STRING("_self", q.target);
    TEST_ASSERT_NOT_NULL(strstr(q.url, "sc40=%23088868"));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "sd40=1990-05-14"));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "so40=R40"));
    l76s_done();
}

static void test_l76_sys_41(void) {
    we_form_req_t q;
    char html[700];
    sprintf(html, "<form action=/sys41 method=GET>"
                  "<input id=sc41 type=color name=sc41 value=#15a591 autocomplete=off>"
                  "<input id=sd41 type=date name=sd41 value=1991-06-15>"
                  "<output id=so41 name=so41 value=R41>R41</output>"
                  "<input id=ss41 type=submit formtarget=w41 value=Go></form>");
    l76s_render(html);
    TEST_ASSERT_EQUAL(WE_FORM_COLOR, g_doc.nodes[l76s_node("sc41")].form_type);
    TEST_ASSERT_EQUAL(WE_FORM_DATE, g_doc.nodes[l76s_node("sd41")].form_type);
    TEST_ASSERT_TRUE(g_doc.nodes[l76s_node("sc41")].form_flags2 & WE_FF2_AC_OFF);
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76s_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_EQUAL(0, q.target_blank);
    TEST_ASSERT_EQUAL_STRING("w41", q.target);
    TEST_ASSERT_NOT_NULL(strstr(q.url, "sc41=%2315a591"));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "sd41=1991-06-15"));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "so41=R41"));
    l76s_done();
}

static void test_l76_sys_42(void) {
    we_form_req_t q;
    char html[700];
    sprintf(html, "<form action=/sys42 method=GET>"
                  "<input id=sc42 type=color name=sc42 value=#22c2ba autocomplete=off>"
                  "<input id=sd42 type=date name=sd42 value=1992-07-16>"
                  "<output id=so42 name=so42 value=R42>R42</output>"
                  "<input id=ss42 type=submit formtarget=_blank value=Go></form>");
    l76s_render(html);
    TEST_ASSERT_EQUAL(WE_FORM_COLOR, g_doc.nodes[l76s_node("sc42")].form_type);
    TEST_ASSERT_EQUAL(WE_FORM_DATE, g_doc.nodes[l76s_node("sd42")].form_type);
    TEST_ASSERT_TRUE(g_doc.nodes[l76s_node("sc42")].form_flags2 & WE_FF2_AC_OFF);
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76s_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_EQUAL(1, q.target_blank);
    TEST_ASSERT_EQUAL_STRING("_blank", q.target);
    TEST_ASSERT_NOT_NULL(strstr(q.url, "sc42=%2322c2ba"));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "sd42=1992-07-16"));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "so42=R42"));
    l76s_done();
}

static void test_l76_sys_43(void) {
    we_form_req_t q;
    char html[700];
    sprintf(html, "<form action=/sys43 method=GET>"
                  "<input id=sc43 type=color name=sc43 value=#2fdfe3 autocomplete=off>"
                  "<input id=sd43 type=date name=sd43 value=1993-08-17>"
                  "<output id=so43 name=so43 value=R43>R43</output>"
                  "<input id=ss43 type=submit formtarget=_self value=Go></form>");
    l76s_render(html);
    TEST_ASSERT_EQUAL(WE_FORM_COLOR, g_doc.nodes[l76s_node("sc43")].form_type);
    TEST_ASSERT_EQUAL(WE_FORM_DATE, g_doc.nodes[l76s_node("sd43")].form_type);
    TEST_ASSERT_TRUE(g_doc.nodes[l76s_node("sc43")].form_flags2 & WE_FF2_AC_OFF);
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76s_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_EQUAL(0, q.target_blank);
    TEST_ASSERT_EQUAL_STRING("_self", q.target);
    TEST_ASSERT_NOT_NULL(strstr(q.url, "sc43=%232fdfe3"));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "sd43=1993-08-17"));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "so43=R43"));
    l76s_done();
}

static void test_l76_sys_44(void) {
    we_form_req_t q;
    char html[700];
    sprintf(html, "<form action=/sys44 method=GET>"
                  "<input id=sc44 type=color name=sc44 value=#3cfc0c autocomplete=off>"
                  "<input id=sd44 type=date name=sd44 value=1994-09-18>"
                  "<output id=so44 name=so44 value=R44>R44</output>"
                  "<input id=ss44 type=submit formtarget=w44 value=Go></form>");
    l76s_render(html);
    TEST_ASSERT_EQUAL(WE_FORM_COLOR, g_doc.nodes[l76s_node("sc44")].form_type);
    TEST_ASSERT_EQUAL(WE_FORM_DATE, g_doc.nodes[l76s_node("sd44")].form_type);
    TEST_ASSERT_TRUE(g_doc.nodes[l76s_node("sc44")].form_flags2 & WE_FF2_AC_OFF);
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76s_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_EQUAL(0, q.target_blank);
    TEST_ASSERT_EQUAL_STRING("w44", q.target);
    TEST_ASSERT_NOT_NULL(strstr(q.url, "sc44=%233cfc0c"));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "sd44=1994-09-18"));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "so44=R44"));
    l76s_done();
}

static void test_l76_sys_45(void) {
    we_form_req_t q;
    char html[700];
    sprintf(html, "<form action=/sys45 method=GET>"
                  "<input id=sc45 type=color name=sc45 value=#491935 autocomplete=off>"
                  "<input id=sd45 type=date name=sd45 value=1995-10-19>"
                  "<output id=so45 name=so45 value=R45>R45</output>"
                  "<input id=ss45 type=submit formtarget=_blank value=Go></form>");
    l76s_render(html);
    TEST_ASSERT_EQUAL(WE_FORM_COLOR, g_doc.nodes[l76s_node("sc45")].form_type);
    TEST_ASSERT_EQUAL(WE_FORM_DATE, g_doc.nodes[l76s_node("sd45")].form_type);
    TEST_ASSERT_TRUE(g_doc.nodes[l76s_node("sc45")].form_flags2 & WE_FF2_AC_OFF);
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76s_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_EQUAL(1, q.target_blank);
    TEST_ASSERT_EQUAL_STRING("_blank", q.target);
    TEST_ASSERT_NOT_NULL(strstr(q.url, "sc45=%23491935"));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "sd45=1995-10-19"));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "so45=R45"));
    l76s_done();
}

static void test_l76_sys_46(void) {
    we_form_req_t q;
    char html[700];
    sprintf(html, "<form action=/sys46 method=GET>"
                  "<input id=sc46 type=color name=sc46 value=#56365e autocomplete=off>"
                  "<input id=sd46 type=date name=sd46 value=1996-11-20>"
                  "<output id=so46 name=so46 value=R46>R46</output>"
                  "<input id=ss46 type=submit formtarget=_self value=Go></form>");
    l76s_render(html);
    TEST_ASSERT_EQUAL(WE_FORM_COLOR, g_doc.nodes[l76s_node("sc46")].form_type);
    TEST_ASSERT_EQUAL(WE_FORM_DATE, g_doc.nodes[l76s_node("sd46")].form_type);
    TEST_ASSERT_TRUE(g_doc.nodes[l76s_node("sc46")].form_flags2 & WE_FF2_AC_OFF);
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76s_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_EQUAL(0, q.target_blank);
    TEST_ASSERT_EQUAL_STRING("_self", q.target);
    TEST_ASSERT_NOT_NULL(strstr(q.url, "sc46=%2356365e"));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "sd46=1996-11-20"));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "so46=R46"));
    l76s_done();
}

static void test_l76_sys_47(void) {
    we_form_req_t q;
    char html[700];
    sprintf(html, "<form action=/sys47 method=GET>"
                  "<input id=sc47 type=color name=sc47 value=#635387 autocomplete=off>"
                  "<input id=sd47 type=date name=sd47 value=1997-12-21>"
                  "<output id=so47 name=so47 value=R47>R47</output>"
                  "<input id=ss47 type=submit formtarget=w47 value=Go></form>");
    l76s_render(html);
    TEST_ASSERT_EQUAL(WE_FORM_COLOR, g_doc.nodes[l76s_node("sc47")].form_type);
    TEST_ASSERT_EQUAL(WE_FORM_DATE, g_doc.nodes[l76s_node("sd47")].form_type);
    TEST_ASSERT_TRUE(g_doc.nodes[l76s_node("sc47")].form_flags2 & WE_FF2_AC_OFF);
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76s_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_EQUAL(0, q.target_blank);
    TEST_ASSERT_EQUAL_STRING("w47", q.target);
    TEST_ASSERT_NOT_NULL(strstr(q.url, "sc47=%23635387"));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "sd47=1997-12-21"));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "so47=R47"));
    l76s_done();
}

static void test_l76_sys_48(void) {
    we_form_req_t q;
    char html[700];
    sprintf(html, "<form action=/sys48 method=GET>"
                  "<input id=sc48 type=color name=sc48 value=#7070b0 autocomplete=off>"
                  "<input id=sd48 type=date name=sd48 value=1998-01-22>"
                  "<output id=so48 name=so48 value=R48>R48</output>"
                  "<input id=ss48 type=submit formtarget=_blank value=Go></form>");
    l76s_render(html);
    TEST_ASSERT_EQUAL(WE_FORM_COLOR, g_doc.nodes[l76s_node("sc48")].form_type);
    TEST_ASSERT_EQUAL(WE_FORM_DATE, g_doc.nodes[l76s_node("sd48")].form_type);
    TEST_ASSERT_TRUE(g_doc.nodes[l76s_node("sc48")].form_flags2 & WE_FF2_AC_OFF);
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76s_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_EQUAL(1, q.target_blank);
    TEST_ASSERT_EQUAL_STRING("_blank", q.target);
    TEST_ASSERT_NOT_NULL(strstr(q.url, "sc48=%237070b0"));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "sd48=1998-01-22"));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "so48=R48"));
    l76s_done();
}

static void test_l76_sys_49(void) {
    we_form_req_t q;
    char html[700];
    sprintf(html, "<form action=/sys49 method=GET>"
                  "<input id=sc49 type=color name=sc49 value=#7d8dd9 autocomplete=off>"
                  "<input id=sd49 type=date name=sd49 value=1999-02-23>"
                  "<output id=so49 name=so49 value=R49>R49</output>"
                  "<input id=ss49 type=submit formtarget=_self value=Go></form>");
    l76s_render(html);
    TEST_ASSERT_EQUAL(WE_FORM_COLOR, g_doc.nodes[l76s_node("sc49")].form_type);
    TEST_ASSERT_EQUAL(WE_FORM_DATE, g_doc.nodes[l76s_node("sd49")].form_type);
    TEST_ASSERT_TRUE(g_doc.nodes[l76s_node("sc49")].form_flags2 & WE_FF2_AC_OFF);
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76s_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_EQUAL(0, q.target_blank);
    TEST_ASSERT_EQUAL_STRING("_self", q.target);
    TEST_ASSERT_NOT_NULL(strstr(q.url, "sc49=%237d8dd9"));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "sd49=1999-02-23"));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "so49=R49"));
    l76s_done();
}

static void test_l76_sys_50(void) {
    we_form_req_t q;
    char html[700];
    sprintf(html, "<form action=/sys50 method=GET>"
                  "<input id=sc50 type=color name=sc50 value=#8aaa02 autocomplete=off>"
                  "<input id=sd50 type=date name=sd50 value=2000-03-24>"
                  "<output id=so50 name=so50 value=R50>R50</output>"
                  "<input id=ss50 type=submit formtarget=w50 value=Go></form>");
    l76s_render(html);
    TEST_ASSERT_EQUAL(WE_FORM_COLOR, g_doc.nodes[l76s_node("sc50")].form_type);
    TEST_ASSERT_EQUAL(WE_FORM_DATE, g_doc.nodes[l76s_node("sd50")].form_type);
    TEST_ASSERT_TRUE(g_doc.nodes[l76s_node("sc50")].form_flags2 & WE_FF2_AC_OFF);
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76s_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_EQUAL(0, q.target_blank);
    TEST_ASSERT_EQUAL_STRING("w50", q.target);
    TEST_ASSERT_NOT_NULL(strstr(q.url, "sc50=%238aaa02"));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "sd50=2000-03-24"));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "so50=R50"));
    l76s_done();
}

static void test_l76_sys_51(void) {
    we_form_req_t q;
    char html[700];
    sprintf(html, "<form action=/sys51 method=GET>"
                  "<input id=sc51 type=color name=sc51 value=#97c72b autocomplete=off>"
                  "<input id=sd51 type=date name=sd51 value=2001-04-25>"
                  "<output id=so51 name=so51 value=R51>R51</output>"
                  "<input id=ss51 type=submit formtarget=_blank value=Go></form>");
    l76s_render(html);
    TEST_ASSERT_EQUAL(WE_FORM_COLOR, g_doc.nodes[l76s_node("sc51")].form_type);
    TEST_ASSERT_EQUAL(WE_FORM_DATE, g_doc.nodes[l76s_node("sd51")].form_type);
    TEST_ASSERT_TRUE(g_doc.nodes[l76s_node("sc51")].form_flags2 & WE_FF2_AC_OFF);
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76s_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_EQUAL(1, q.target_blank);
    TEST_ASSERT_EQUAL_STRING("_blank", q.target);
    TEST_ASSERT_NOT_NULL(strstr(q.url, "sc51=%2397c72b"));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "sd51=2001-04-25"));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "so51=R51"));
    l76s_done();
}

static void test_l76_sys_52(void) {
    we_form_req_t q;
    char html[700];
    sprintf(html, "<form action=/sys52 method=GET>"
                  "<input id=sc52 type=color name=sc52 value=#a4e454 autocomplete=off>"
                  "<input id=sd52 type=date name=sd52 value=2002-05-26>"
                  "<output id=so52 name=so52 value=R52>R52</output>"
                  "<input id=ss52 type=submit formtarget=_self value=Go></form>");
    l76s_render(html);
    TEST_ASSERT_EQUAL(WE_FORM_COLOR, g_doc.nodes[l76s_node("sc52")].form_type);
    TEST_ASSERT_EQUAL(WE_FORM_DATE, g_doc.nodes[l76s_node("sd52")].form_type);
    TEST_ASSERT_TRUE(g_doc.nodes[l76s_node("sc52")].form_flags2 & WE_FF2_AC_OFF);
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76s_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_EQUAL(0, q.target_blank);
    TEST_ASSERT_EQUAL_STRING("_self", q.target);
    TEST_ASSERT_NOT_NULL(strstr(q.url, "sc52=%23a4e454"));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "sd52=2002-05-26"));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "so52=R52"));
    l76s_done();
}

static void test_l76_sys_53(void) {
    we_form_req_t q;
    char html[700];
    sprintf(html, "<form action=/sys53 method=GET>"
                  "<input id=sc53 type=color name=sc53 value=#b1017d autocomplete=off>"
                  "<input id=sd53 type=date name=sd53 value=2003-06-27>"
                  "<output id=so53 name=so53 value=R53>R53</output>"
                  "<input id=ss53 type=submit formtarget=w53 value=Go></form>");
    l76s_render(html);
    TEST_ASSERT_EQUAL(WE_FORM_COLOR, g_doc.nodes[l76s_node("sc53")].form_type);
    TEST_ASSERT_EQUAL(WE_FORM_DATE, g_doc.nodes[l76s_node("sd53")].form_type);
    TEST_ASSERT_TRUE(g_doc.nodes[l76s_node("sc53")].form_flags2 & WE_FF2_AC_OFF);
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76s_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_EQUAL(0, q.target_blank);
    TEST_ASSERT_EQUAL_STRING("w53", q.target);
    TEST_ASSERT_NOT_NULL(strstr(q.url, "sc53=%23b1017d"));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "sd53=2003-06-27"));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "so53=R53"));
    l76s_done();
}

static void test_l76_sys_54(void) {
    we_form_req_t q;
    char html[700];
    sprintf(html, "<form action=/sys54 method=GET>"
                  "<input id=sc54 type=color name=sc54 value=#be1ea6 autocomplete=off>"
                  "<input id=sd54 type=date name=sd54 value=2004-07-01>"
                  "<output id=so54 name=so54 value=R54>R54</output>"
                  "<input id=ss54 type=submit formtarget=_blank value=Go></form>");
    l76s_render(html);
    TEST_ASSERT_EQUAL(WE_FORM_COLOR, g_doc.nodes[l76s_node("sc54")].form_type);
    TEST_ASSERT_EQUAL(WE_FORM_DATE, g_doc.nodes[l76s_node("sd54")].form_type);
    TEST_ASSERT_TRUE(g_doc.nodes[l76s_node("sc54")].form_flags2 & WE_FF2_AC_OFF);
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76s_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_EQUAL(1, q.target_blank);
    TEST_ASSERT_EQUAL_STRING("_blank", q.target);
    TEST_ASSERT_NOT_NULL(strstr(q.url, "sc54=%23be1ea6"));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "sd54=2004-07-01"));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "so54=R54"));
    l76s_done();
}

static void test_l76_sys_55(void) {
    we_form_req_t q;
    char html[700];
    sprintf(html, "<form action=/sys55 method=GET>"
                  "<input id=sc55 type=color name=sc55 value=#cb3bcf autocomplete=off>"
                  "<input id=sd55 type=date name=sd55 value=2005-08-02>"
                  "<output id=so55 name=so55 value=R55>R55</output>"
                  "<input id=ss55 type=submit formtarget=_self value=Go></form>");
    l76s_render(html);
    TEST_ASSERT_EQUAL(WE_FORM_COLOR, g_doc.nodes[l76s_node("sc55")].form_type);
    TEST_ASSERT_EQUAL(WE_FORM_DATE, g_doc.nodes[l76s_node("sd55")].form_type);
    TEST_ASSERT_TRUE(g_doc.nodes[l76s_node("sc55")].form_flags2 & WE_FF2_AC_OFF);
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76s_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_EQUAL(0, q.target_blank);
    TEST_ASSERT_EQUAL_STRING("_self", q.target);
    TEST_ASSERT_NOT_NULL(strstr(q.url, "sc55=%23cb3bcf"));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "sd55=2005-08-02"));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "so55=R55"));
    l76s_done();
}

static void test_l76_sys_56(void) {
    we_form_req_t q;
    char html[700];
    sprintf(html, "<form action=/sys56 method=GET>"
                  "<input id=sc56 type=color name=sc56 value=#d858f8 autocomplete=off>"
                  "<input id=sd56 type=date name=sd56 value=2006-09-03>"
                  "<output id=so56 name=so56 value=R56>R56</output>"
                  "<input id=ss56 type=submit formtarget=w56 value=Go></form>");
    l76s_render(html);
    TEST_ASSERT_EQUAL(WE_FORM_COLOR, g_doc.nodes[l76s_node("sc56")].form_type);
    TEST_ASSERT_EQUAL(WE_FORM_DATE, g_doc.nodes[l76s_node("sd56")].form_type);
    TEST_ASSERT_TRUE(g_doc.nodes[l76s_node("sc56")].form_flags2 & WE_FF2_AC_OFF);
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76s_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_EQUAL(0, q.target_blank);
    TEST_ASSERT_EQUAL_STRING("w56", q.target);
    TEST_ASSERT_NOT_NULL(strstr(q.url, "sc56=%23d858f8"));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "sd56=2006-09-03"));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "so56=R56"));
    l76s_done();
}

static void test_l76_sys_57(void) {
    we_form_req_t q;
    char html[700];
    sprintf(html, "<form action=/sys57 method=GET>"
                  "<input id=sc57 type=color name=sc57 value=#e57521 autocomplete=off>"
                  "<input id=sd57 type=date name=sd57 value=2007-10-04>"
                  "<output id=so57 name=so57 value=R57>R57</output>"
                  "<input id=ss57 type=submit formtarget=_blank value=Go></form>");
    l76s_render(html);
    TEST_ASSERT_EQUAL(WE_FORM_COLOR, g_doc.nodes[l76s_node("sc57")].form_type);
    TEST_ASSERT_EQUAL(WE_FORM_DATE, g_doc.nodes[l76s_node("sd57")].form_type);
    TEST_ASSERT_TRUE(g_doc.nodes[l76s_node("sc57")].form_flags2 & WE_FF2_AC_OFF);
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76s_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_EQUAL(1, q.target_blank);
    TEST_ASSERT_EQUAL_STRING("_blank", q.target);
    TEST_ASSERT_NOT_NULL(strstr(q.url, "sc57=%23e57521"));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "sd57=2007-10-04"));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "so57=R57"));
    l76s_done();
}

static void test_l76_sys_58(void) {
    we_form_req_t q;
    char html[700];
    sprintf(html, "<form action=/sys58 method=GET>"
                  "<input id=sc58 type=color name=sc58 value=#f2924a autocomplete=off>"
                  "<input id=sd58 type=date name=sd58 value=2008-11-05>"
                  "<output id=so58 name=so58 value=R58>R58</output>"
                  "<input id=ss58 type=submit formtarget=_self value=Go></form>");
    l76s_render(html);
    TEST_ASSERT_EQUAL(WE_FORM_COLOR, g_doc.nodes[l76s_node("sc58")].form_type);
    TEST_ASSERT_EQUAL(WE_FORM_DATE, g_doc.nodes[l76s_node("sd58")].form_type);
    TEST_ASSERT_TRUE(g_doc.nodes[l76s_node("sc58")].form_flags2 & WE_FF2_AC_OFF);
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76s_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_EQUAL(0, q.target_blank);
    TEST_ASSERT_EQUAL_STRING("_self", q.target);
    TEST_ASSERT_NOT_NULL(strstr(q.url, "sc58=%23f2924a"));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "sd58=2008-11-05"));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "so58=R58"));
    l76s_done();
}

static void test_l76_sys_59(void) {
    we_form_req_t q;
    char html[700];
    sprintf(html, "<form action=/sys59 method=GET>"
                  "<input id=sc59 type=color name=sc59 value=#ffaf73 autocomplete=off>"
                  "<input id=sd59 type=date name=sd59 value=2009-12-06>"
                  "<output id=so59 name=so59 value=R59>R59</output>"
                  "<input id=ss59 type=submit formtarget=w59 value=Go></form>");
    l76s_render(html);
    TEST_ASSERT_EQUAL(WE_FORM_COLOR, g_doc.nodes[l76s_node("sc59")].form_type);
    TEST_ASSERT_EQUAL(WE_FORM_DATE, g_doc.nodes[l76s_node("sd59")].form_type);
    TEST_ASSERT_TRUE(g_doc.nodes[l76s_node("sc59")].form_flags2 & WE_FF2_AC_OFF);
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76s_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_EQUAL(0, q.target_blank);
    TEST_ASSERT_EQUAL_STRING("w59", q.target);
    TEST_ASSERT_NOT_NULL(strstr(q.url, "sc59=%23ffaf73"));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "sd59=2009-12-06"));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "so59=R59"));
    l76s_done();
}

static void test_l76_sys_60(void) {
    we_form_req_t q;
    char html[700];
    sprintf(html, "<form action=/sys60 method=GET>"
                  "<input id=sc60 type=color name=sc60 value=#0ccc9c autocomplete=off>"
                  "<input id=sd60 type=date name=sd60 value=2010-01-07>"
                  "<output id=so60 name=so60 value=R60>R60</output>"
                  "<input id=ss60 type=submit formtarget=_blank value=Go></form>");
    l76s_render(html);
    TEST_ASSERT_EQUAL(WE_FORM_COLOR, g_doc.nodes[l76s_node("sc60")].form_type);
    TEST_ASSERT_EQUAL(WE_FORM_DATE, g_doc.nodes[l76s_node("sd60")].form_type);
    TEST_ASSERT_TRUE(g_doc.nodes[l76s_node("sc60")].form_flags2 & WE_FF2_AC_OFF);
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76s_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_EQUAL(1, q.target_blank);
    TEST_ASSERT_EQUAL_STRING("_blank", q.target);
    TEST_ASSERT_NOT_NULL(strstr(q.url, "sc60=%230ccc9c"));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "sd60=2010-01-07"));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "so60=R60"));
    l76s_done();
}

static void test_l76_sys_61(void) {
    we_form_req_t q;
    char html[700];
    sprintf(html, "<form action=/sys61 method=GET>"
                  "<input id=sc61 type=color name=sc61 value=#19e9c5 autocomplete=off>"
                  "<input id=sd61 type=date name=sd61 value=2011-02-08>"
                  "<output id=so61 name=so61 value=R61>R61</output>"
                  "<input id=ss61 type=submit formtarget=_self value=Go></form>");
    l76s_render(html);
    TEST_ASSERT_EQUAL(WE_FORM_COLOR, g_doc.nodes[l76s_node("sc61")].form_type);
    TEST_ASSERT_EQUAL(WE_FORM_DATE, g_doc.nodes[l76s_node("sd61")].form_type);
    TEST_ASSERT_TRUE(g_doc.nodes[l76s_node("sc61")].form_flags2 & WE_FF2_AC_OFF);
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76s_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_EQUAL(0, q.target_blank);
    TEST_ASSERT_EQUAL_STRING("_self", q.target);
    TEST_ASSERT_NOT_NULL(strstr(q.url, "sc61=%2319e9c5"));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "sd61=2011-02-08"));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "so61=R61"));
    l76s_done();
}

static void test_l76_sys_62(void) {
    we_form_req_t q;
    char html[700];
    sprintf(html, "<form action=/sys62 method=GET>"
                  "<input id=sc62 type=color name=sc62 value=#2606ee autocomplete=off>"
                  "<input id=sd62 type=date name=sd62 value=2012-03-09>"
                  "<output id=so62 name=so62 value=R62>R62</output>"
                  "<input id=ss62 type=submit formtarget=w62 value=Go></form>");
    l76s_render(html);
    TEST_ASSERT_EQUAL(WE_FORM_COLOR, g_doc.nodes[l76s_node("sc62")].form_type);
    TEST_ASSERT_EQUAL(WE_FORM_DATE, g_doc.nodes[l76s_node("sd62")].form_type);
    TEST_ASSERT_TRUE(g_doc.nodes[l76s_node("sc62")].form_flags2 & WE_FF2_AC_OFF);
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76s_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_EQUAL(0, q.target_blank);
    TEST_ASSERT_EQUAL_STRING("w62", q.target);
    TEST_ASSERT_NOT_NULL(strstr(q.url, "sc62=%232606ee"));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "sd62=2012-03-09"));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "so62=R62"));
    l76s_done();
}

static void test_l76_sys_63(void) {
    we_form_req_t q;
    char html[700];
    sprintf(html, "<form action=/sys63 method=GET>"
                  "<input id=sc63 type=color name=sc63 value=#332317 autocomplete=off>"
                  "<input id=sd63 type=date name=sd63 value=2013-04-10>"
                  "<output id=so63 name=so63 value=R63>R63</output>"
                  "<input id=ss63 type=submit formtarget=_blank value=Go></form>");
    l76s_render(html);
    TEST_ASSERT_EQUAL(WE_FORM_COLOR, g_doc.nodes[l76s_node("sc63")].form_type);
    TEST_ASSERT_EQUAL(WE_FORM_DATE, g_doc.nodes[l76s_node("sd63")].form_type);
    TEST_ASSERT_TRUE(g_doc.nodes[l76s_node("sc63")].form_flags2 & WE_FF2_AC_OFF);
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76s_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_EQUAL(1, q.target_blank);
    TEST_ASSERT_EQUAL_STRING("_blank", q.target);
    TEST_ASSERT_NOT_NULL(strstr(q.url, "sc63=%23332317"));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "sd63=2013-04-10"));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "so63=R63"));
    l76s_done();
}

static void test_l76_sys_64(void) {
    we_form_req_t q;
    char html[700];
    sprintf(html, "<form action=/sys64 method=GET>"
                  "<input id=sc64 type=color name=sc64 value=#404040 autocomplete=off>"
                  "<input id=sd64 type=date name=sd64 value=2014-05-11>"
                  "<output id=so64 name=so64 value=R64>R64</output>"
                  "<input id=ss64 type=submit formtarget=_self value=Go></form>");
    l76s_render(html);
    TEST_ASSERT_EQUAL(WE_FORM_COLOR, g_doc.nodes[l76s_node("sc64")].form_type);
    TEST_ASSERT_EQUAL(WE_FORM_DATE, g_doc.nodes[l76s_node("sd64")].form_type);
    TEST_ASSERT_TRUE(g_doc.nodes[l76s_node("sc64")].form_flags2 & WE_FF2_AC_OFF);
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76s_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_EQUAL(0, q.target_blank);
    TEST_ASSERT_EQUAL_STRING("_self", q.target);
    TEST_ASSERT_NOT_NULL(strstr(q.url, "sc64=%23404040"));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "sd64=2014-05-11"));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "so64=R64"));
    l76s_done();
}

static void test_l76_sys_65(void) {
    we_form_req_t q;
    char html[700];
    sprintf(html, "<form action=/sys65 method=GET>"
                  "<input id=sc65 type=color name=sc65 value=#4d5d69 autocomplete=off>"
                  "<input id=sd65 type=date name=sd65 value=2015-06-12>"
                  "<output id=so65 name=so65 value=R65>R65</output>"
                  "<input id=ss65 type=submit formtarget=w65 value=Go></form>");
    l76s_render(html);
    TEST_ASSERT_EQUAL(WE_FORM_COLOR, g_doc.nodes[l76s_node("sc65")].form_type);
    TEST_ASSERT_EQUAL(WE_FORM_DATE, g_doc.nodes[l76s_node("sd65")].form_type);
    TEST_ASSERT_TRUE(g_doc.nodes[l76s_node("sc65")].form_flags2 & WE_FF2_AC_OFF);
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76s_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_EQUAL(0, q.target_blank);
    TEST_ASSERT_EQUAL_STRING("w65", q.target);
    TEST_ASSERT_NOT_NULL(strstr(q.url, "sc65=%234d5d69"));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "sd65=2015-06-12"));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "so65=R65"));
    l76s_done();
}

static void test_l76_sys_66(void) {
    we_form_req_t q;
    char html[700];
    sprintf(html, "<form action=/sys66 method=GET>"
                  "<input id=sc66 type=color name=sc66 value=#5a7a92 autocomplete=off>"
                  "<input id=sd66 type=date name=sd66 value=2016-07-13>"
                  "<output id=so66 name=so66 value=R66>R66</output>"
                  "<input id=ss66 type=submit formtarget=_blank value=Go></form>");
    l76s_render(html);
    TEST_ASSERT_EQUAL(WE_FORM_COLOR, g_doc.nodes[l76s_node("sc66")].form_type);
    TEST_ASSERT_EQUAL(WE_FORM_DATE, g_doc.nodes[l76s_node("sd66")].form_type);
    TEST_ASSERT_TRUE(g_doc.nodes[l76s_node("sc66")].form_flags2 & WE_FF2_AC_OFF);
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76s_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_EQUAL(1, q.target_blank);
    TEST_ASSERT_EQUAL_STRING("_blank", q.target);
    TEST_ASSERT_NOT_NULL(strstr(q.url, "sc66=%235a7a92"));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "sd66=2016-07-13"));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "so66=R66"));
    l76s_done();
}

static void test_l76_sys_67(void) {
    we_form_req_t q;
    char html[700];
    sprintf(html, "<form action=/sys67 method=GET>"
                  "<input id=sc67 type=color name=sc67 value=#6797bb autocomplete=off>"
                  "<input id=sd67 type=date name=sd67 value=2017-08-14>"
                  "<output id=so67 name=so67 value=R67>R67</output>"
                  "<input id=ss67 type=submit formtarget=_self value=Go></form>");
    l76s_render(html);
    TEST_ASSERT_EQUAL(WE_FORM_COLOR, g_doc.nodes[l76s_node("sc67")].form_type);
    TEST_ASSERT_EQUAL(WE_FORM_DATE, g_doc.nodes[l76s_node("sd67")].form_type);
    TEST_ASSERT_TRUE(g_doc.nodes[l76s_node("sc67")].form_flags2 & WE_FF2_AC_OFF);
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76s_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_EQUAL(0, q.target_blank);
    TEST_ASSERT_EQUAL_STRING("_self", q.target);
    TEST_ASSERT_NOT_NULL(strstr(q.url, "sc67=%236797bb"));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "sd67=2017-08-14"));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "so67=R67"));
    l76s_done();
}

static void test_l76_sys_68(void) {
    we_form_req_t q;
    char html[700];
    sprintf(html, "<form action=/sys68 method=GET>"
                  "<input id=sc68 type=color name=sc68 value=#74b4e4 autocomplete=off>"
                  "<input id=sd68 type=date name=sd68 value=2018-09-15>"
                  "<output id=so68 name=so68 value=R68>R68</output>"
                  "<input id=ss68 type=submit formtarget=w68 value=Go></form>");
    l76s_render(html);
    TEST_ASSERT_EQUAL(WE_FORM_COLOR, g_doc.nodes[l76s_node("sc68")].form_type);
    TEST_ASSERT_EQUAL(WE_FORM_DATE, g_doc.nodes[l76s_node("sd68")].form_type);
    TEST_ASSERT_TRUE(g_doc.nodes[l76s_node("sc68")].form_flags2 & WE_FF2_AC_OFF);
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76s_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_EQUAL(0, q.target_blank);
    TEST_ASSERT_EQUAL_STRING("w68", q.target);
    TEST_ASSERT_NOT_NULL(strstr(q.url, "sc68=%2374b4e4"));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "sd68=2018-09-15"));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "so68=R68"));
    l76s_done();
}

static void test_l76_sys_69(void) {
    we_form_req_t q;
    char html[700];
    sprintf(html, "<form action=/sys69 method=GET>"
                  "<input id=sc69 type=color name=sc69 value=#81d10d autocomplete=off>"
                  "<input id=sd69 type=date name=sd69 value=2019-10-16>"
                  "<output id=so69 name=so69 value=R69>R69</output>"
                  "<input id=ss69 type=submit formtarget=_blank value=Go></form>");
    l76s_render(html);
    TEST_ASSERT_EQUAL(WE_FORM_COLOR, g_doc.nodes[l76s_node("sc69")].form_type);
    TEST_ASSERT_EQUAL(WE_FORM_DATE, g_doc.nodes[l76s_node("sd69")].form_type);
    TEST_ASSERT_TRUE(g_doc.nodes[l76s_node("sc69")].form_flags2 & WE_FF2_AC_OFF);
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76s_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_EQUAL(1, q.target_blank);
    TEST_ASSERT_EQUAL_STRING("_blank", q.target);
    TEST_ASSERT_NOT_NULL(strstr(q.url, "sc69=%2381d10d"));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "sd69=2019-10-16"));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "so69=R69"));
    l76s_done();
}

static void test_l76_sys_70(void) {
    we_form_req_t q;
    char html[700];
    sprintf(html, "<form action=/sys70 method=GET>"
                  "<input id=sc70 type=color name=sc70 value=#8eee36 autocomplete=off>"
                  "<input id=sd70 type=date name=sd70 value=2020-11-17>"
                  "<output id=so70 name=so70 value=R70>R70</output>"
                  "<input id=ss70 type=submit formtarget=_self value=Go></form>");
    l76s_render(html);
    TEST_ASSERT_EQUAL(WE_FORM_COLOR, g_doc.nodes[l76s_node("sc70")].form_type);
    TEST_ASSERT_EQUAL(WE_FORM_DATE, g_doc.nodes[l76s_node("sd70")].form_type);
    TEST_ASSERT_TRUE(g_doc.nodes[l76s_node("sc70")].form_flags2 & WE_FF2_AC_OFF);
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76s_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_EQUAL(0, q.target_blank);
    TEST_ASSERT_EQUAL_STRING("_self", q.target);
    TEST_ASSERT_NOT_NULL(strstr(q.url, "sc70=%238eee36"));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "sd70=2020-11-17"));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "so70=R70"));
    l76s_done();
}

static void test_l76_sys_71(void) {
    we_form_req_t q;
    char html[700];
    sprintf(html, "<form action=/sys71 method=GET>"
                  "<input id=sc71 type=color name=sc71 value=#9b0b5f autocomplete=off>"
                  "<input id=sd71 type=date name=sd71 value=2021-12-18>"
                  "<output id=so71 name=so71 value=R71>R71</output>"
                  "<input id=ss71 type=submit formtarget=w71 value=Go></form>");
    l76s_render(html);
    TEST_ASSERT_EQUAL(WE_FORM_COLOR, g_doc.nodes[l76s_node("sc71")].form_type);
    TEST_ASSERT_EQUAL(WE_FORM_DATE, g_doc.nodes[l76s_node("sd71")].form_type);
    TEST_ASSERT_TRUE(g_doc.nodes[l76s_node("sc71")].form_flags2 & WE_FF2_AC_OFF);
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76s_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_EQUAL(0, q.target_blank);
    TEST_ASSERT_EQUAL_STRING("w71", q.target);
    TEST_ASSERT_NOT_NULL(strstr(q.url, "sc71=%239b0b5f"));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "sd71=2021-12-18"));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "so71=R71"));
    l76s_done();
}

static void test_l76_sys_72(void) {
    we_form_req_t q;
    char html[700];
    sprintf(html, "<form action=/sys72 method=GET>"
                  "<input id=sc72 type=color name=sc72 value=#a82888 autocomplete=off>"
                  "<input id=sd72 type=date name=sd72 value=2022-01-19>"
                  "<output id=so72 name=so72 value=R72>R72</output>"
                  "<input id=ss72 type=submit formtarget=_blank value=Go></form>");
    l76s_render(html);
    TEST_ASSERT_EQUAL(WE_FORM_COLOR, g_doc.nodes[l76s_node("sc72")].form_type);
    TEST_ASSERT_EQUAL(WE_FORM_DATE, g_doc.nodes[l76s_node("sd72")].form_type);
    TEST_ASSERT_TRUE(g_doc.nodes[l76s_node("sc72")].form_flags2 & WE_FF2_AC_OFF);
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76s_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_EQUAL(1, q.target_blank);
    TEST_ASSERT_EQUAL_STRING("_blank", q.target);
    TEST_ASSERT_NOT_NULL(strstr(q.url, "sc72=%23a82888"));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "sd72=2022-01-19"));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "so72=R72"));
    l76s_done();
}

static void test_l76_sys_73(void) {
    we_form_req_t q;
    char html[700];
    sprintf(html, "<form action=/sys73 method=GET>"
                  "<input id=sc73 type=color name=sc73 value=#b545b1 autocomplete=off>"
                  "<input id=sd73 type=date name=sd73 value=2023-02-20>"
                  "<output id=so73 name=so73 value=R73>R73</output>"
                  "<input id=ss73 type=submit formtarget=_self value=Go></form>");
    l76s_render(html);
    TEST_ASSERT_EQUAL(WE_FORM_COLOR, g_doc.nodes[l76s_node("sc73")].form_type);
    TEST_ASSERT_EQUAL(WE_FORM_DATE, g_doc.nodes[l76s_node("sd73")].form_type);
    TEST_ASSERT_TRUE(g_doc.nodes[l76s_node("sc73")].form_flags2 & WE_FF2_AC_OFF);
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76s_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_EQUAL(0, q.target_blank);
    TEST_ASSERT_EQUAL_STRING("_self", q.target);
    TEST_ASSERT_NOT_NULL(strstr(q.url, "sc73=%23b545b1"));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "sd73=2023-02-20"));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "so73=R73"));
    l76s_done();
}

static void test_l76_sys_74(void) {
    we_form_req_t q;
    char html[700];
    sprintf(html, "<form action=/sys74 method=GET>"
                  "<input id=sc74 type=color name=sc74 value=#c262da autocomplete=off>"
                  "<input id=sd74 type=date name=sd74 value=2024-03-21>"
                  "<output id=so74 name=so74 value=R74>R74</output>"
                  "<input id=ss74 type=submit formtarget=w74 value=Go></form>");
    l76s_render(html);
    TEST_ASSERT_EQUAL(WE_FORM_COLOR, g_doc.nodes[l76s_node("sc74")].form_type);
    TEST_ASSERT_EQUAL(WE_FORM_DATE, g_doc.nodes[l76s_node("sd74")].form_type);
    TEST_ASSERT_TRUE(g_doc.nodes[l76s_node("sc74")].form_flags2 & WE_FF2_AC_OFF);
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76s_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_EQUAL(0, q.target_blank);
    TEST_ASSERT_EQUAL_STRING("w74", q.target);
    TEST_ASSERT_NOT_NULL(strstr(q.url, "sc74=%23c262da"));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "sd74=2024-03-21"));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "so74=R74"));
    l76s_done();
}

static void test_l76_sys_75(void) {
    we_form_req_t q;
    char html[700];
    sprintf(html, "<form action=/sys75 method=GET>"
                  "<input id=sc75 type=color name=sc75 value=#cf7f03 autocomplete=off>"
                  "<input id=sd75 type=date name=sd75 value=2025-04-22>"
                  "<output id=so75 name=so75 value=R75>R75</output>"
                  "<input id=ss75 type=submit formtarget=_blank value=Go></form>");
    l76s_render(html);
    TEST_ASSERT_EQUAL(WE_FORM_COLOR, g_doc.nodes[l76s_node("sc75")].form_type);
    TEST_ASSERT_EQUAL(WE_FORM_DATE, g_doc.nodes[l76s_node("sd75")].form_type);
    TEST_ASSERT_TRUE(g_doc.nodes[l76s_node("sc75")].form_flags2 & WE_FF2_AC_OFF);
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76s_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_EQUAL(1, q.target_blank);
    TEST_ASSERT_EQUAL_STRING("_blank", q.target);
    TEST_ASSERT_NOT_NULL(strstr(q.url, "sc75=%23cf7f03"));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "sd75=2025-04-22"));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "so75=R75"));
    l76s_done();
}

static void test_l76_sys_76(void) {
    we_form_req_t q;
    char html[700];
    sprintf(html, "<form action=/sys76 method=GET>"
                  "<input id=sc76 type=color name=sc76 value=#dc9c2c autocomplete=off>"
                  "<input id=sd76 type=date name=sd76 value=2026-05-23>"
                  "<output id=so76 name=so76 value=R76>R76</output>"
                  "<input id=ss76 type=submit formtarget=_self value=Go></form>");
    l76s_render(html);
    TEST_ASSERT_EQUAL(WE_FORM_COLOR, g_doc.nodes[l76s_node("sc76")].form_type);
    TEST_ASSERT_EQUAL(WE_FORM_DATE, g_doc.nodes[l76s_node("sd76")].form_type);
    TEST_ASSERT_TRUE(g_doc.nodes[l76s_node("sc76")].form_flags2 & WE_FF2_AC_OFF);
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76s_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_EQUAL(0, q.target_blank);
    TEST_ASSERT_EQUAL_STRING("_self", q.target);
    TEST_ASSERT_NOT_NULL(strstr(q.url, "sc76=%23dc9c2c"));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "sd76=2026-05-23"));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "so76=R76"));
    l76s_done();
}

static void test_l76_sys_77(void) {
    we_form_req_t q;
    char html[700];
    sprintf(html, "<form action=/sys77 method=GET>"
                  "<input id=sc77 type=color name=sc77 value=#e9b955 autocomplete=off>"
                  "<input id=sd77 type=date name=sd77 value=2027-06-24>"
                  "<output id=so77 name=so77 value=R77>R77</output>"
                  "<input id=ss77 type=submit formtarget=w77 value=Go></form>");
    l76s_render(html);
    TEST_ASSERT_EQUAL(WE_FORM_COLOR, g_doc.nodes[l76s_node("sc77")].form_type);
    TEST_ASSERT_EQUAL(WE_FORM_DATE, g_doc.nodes[l76s_node("sd77")].form_type);
    TEST_ASSERT_TRUE(g_doc.nodes[l76s_node("sc77")].form_flags2 & WE_FF2_AC_OFF);
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76s_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_EQUAL(0, q.target_blank);
    TEST_ASSERT_EQUAL_STRING("w77", q.target);
    TEST_ASSERT_NOT_NULL(strstr(q.url, "sc77=%23e9b955"));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "sd77=2027-06-24"));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "so77=R77"));
    l76s_done();
}

static void test_l76_sys_78(void) {
    we_form_req_t q;
    char html[700];
    sprintf(html, "<form action=/sys78 method=GET>"
                  "<input id=sc78 type=color name=sc78 value=#f6d67e autocomplete=off>"
                  "<input id=sd78 type=date name=sd78 value=2028-07-25>"
                  "<output id=so78 name=so78 value=R78>R78</output>"
                  "<input id=ss78 type=submit formtarget=_blank value=Go></form>");
    l76s_render(html);
    TEST_ASSERT_EQUAL(WE_FORM_COLOR, g_doc.nodes[l76s_node("sc78")].form_type);
    TEST_ASSERT_EQUAL(WE_FORM_DATE, g_doc.nodes[l76s_node("sd78")].form_type);
    TEST_ASSERT_TRUE(g_doc.nodes[l76s_node("sc78")].form_flags2 & WE_FF2_AC_OFF);
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76s_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_EQUAL(1, q.target_blank);
    TEST_ASSERT_EQUAL_STRING("_blank", q.target);
    TEST_ASSERT_NOT_NULL(strstr(q.url, "sc78=%23f6d67e"));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "sd78=2028-07-25"));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "so78=R78"));
    l76s_done();
}

static void test_l76_sys_79(void) {
    we_form_req_t q;
    char html[700];
    sprintf(html, "<form action=/sys79 method=GET>"
                  "<input id=sc79 type=color name=sc79 value=#03f3a7 autocomplete=off>"
                  "<input id=sd79 type=date name=sd79 value=2029-08-26>"
                  "<output id=so79 name=so79 value=R79>R79</output>"
                  "<input id=ss79 type=submit formtarget=_self value=Go></form>");
    l76s_render(html);
    TEST_ASSERT_EQUAL(WE_FORM_COLOR, g_doc.nodes[l76s_node("sc79")].form_type);
    TEST_ASSERT_EQUAL(WE_FORM_DATE, g_doc.nodes[l76s_node("sd79")].form_type);
    TEST_ASSERT_TRUE(g_doc.nodes[l76s_node("sc79")].form_flags2 & WE_FF2_AC_OFF);
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76s_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_EQUAL(0, q.target_blank);
    TEST_ASSERT_EQUAL_STRING("_self", q.target);
    TEST_ASSERT_NOT_NULL(strstr(q.url, "sc79=%2303f3a7"));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "sd79=2029-08-26"));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "so79=R79"));
    l76s_done();
}

static void test_l76_sys_80(void) {
    we_form_req_t q;
    char html[700];
    sprintf(html, "<form action=/sys80 method=GET>"
                  "<input id=sc80 type=color name=sc80 value=#1010d0 autocomplete=off>"
                  "<input id=sd80 type=date name=sd80 value=1990-09-27>"
                  "<output id=so80 name=so80 value=R80>R80</output>"
                  "<input id=ss80 type=submit formtarget=w80 value=Go></form>");
    l76s_render(html);
    TEST_ASSERT_EQUAL(WE_FORM_COLOR, g_doc.nodes[l76s_node("sc80")].form_type);
    TEST_ASSERT_EQUAL(WE_FORM_DATE, g_doc.nodes[l76s_node("sd80")].form_type);
    TEST_ASSERT_TRUE(g_doc.nodes[l76s_node("sc80")].form_flags2 & WE_FF2_AC_OFF);
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76s_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_EQUAL(0, q.target_blank);
    TEST_ASSERT_EQUAL_STRING("w80", q.target);
    TEST_ASSERT_NOT_NULL(strstr(q.url, "sc80=%231010d0"));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "sd80=1990-09-27"));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "so80=R80"));
    l76s_done();
}

static void test_l76_sys_81(void) {
    we_form_req_t q;
    char html[700];
    sprintf(html, "<form action=/sys81 method=GET>"
                  "<input id=sc81 type=color name=sc81 value=#1d2df9 autocomplete=off>"
                  "<input id=sd81 type=date name=sd81 value=1991-10-01>"
                  "<output id=so81 name=so81 value=R81>R81</output>"
                  "<input id=ss81 type=submit formtarget=_blank value=Go></form>");
    l76s_render(html);
    TEST_ASSERT_EQUAL(WE_FORM_COLOR, g_doc.nodes[l76s_node("sc81")].form_type);
    TEST_ASSERT_EQUAL(WE_FORM_DATE, g_doc.nodes[l76s_node("sd81")].form_type);
    TEST_ASSERT_TRUE(g_doc.nodes[l76s_node("sc81")].form_flags2 & WE_FF2_AC_OFF);
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76s_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_EQUAL(1, q.target_blank);
    TEST_ASSERT_EQUAL_STRING("_blank", q.target);
    TEST_ASSERT_NOT_NULL(strstr(q.url, "sc81=%231d2df9"));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "sd81=1991-10-01"));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "so81=R81"));
    l76s_done();
}

static void test_l76_sys_82(void) {
    we_form_req_t q;
    char html[700];
    sprintf(html, "<form action=/sys82 method=GET>"
                  "<input id=sc82 type=color name=sc82 value=#2a4a22 autocomplete=off>"
                  "<input id=sd82 type=date name=sd82 value=1992-11-02>"
                  "<output id=so82 name=so82 value=R82>R82</output>"
                  "<input id=ss82 type=submit formtarget=_self value=Go></form>");
    l76s_render(html);
    TEST_ASSERT_EQUAL(WE_FORM_COLOR, g_doc.nodes[l76s_node("sc82")].form_type);
    TEST_ASSERT_EQUAL(WE_FORM_DATE, g_doc.nodes[l76s_node("sd82")].form_type);
    TEST_ASSERT_TRUE(g_doc.nodes[l76s_node("sc82")].form_flags2 & WE_FF2_AC_OFF);
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76s_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_EQUAL(0, q.target_blank);
    TEST_ASSERT_EQUAL_STRING("_self", q.target);
    TEST_ASSERT_NOT_NULL(strstr(q.url, "sc82=%232a4a22"));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "sd82=1992-11-02"));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "so82=R82"));
    l76s_done();
}

static void test_l76_sys_83(void) {
    we_form_req_t q;
    char html[700];
    sprintf(html, "<form action=/sys83 method=GET>"
                  "<input id=sc83 type=color name=sc83 value=#37674b autocomplete=off>"
                  "<input id=sd83 type=date name=sd83 value=1993-12-03>"
                  "<output id=so83 name=so83 value=R83>R83</output>"
                  "<input id=ss83 type=submit formtarget=w83 value=Go></form>");
    l76s_render(html);
    TEST_ASSERT_EQUAL(WE_FORM_COLOR, g_doc.nodes[l76s_node("sc83")].form_type);
    TEST_ASSERT_EQUAL(WE_FORM_DATE, g_doc.nodes[l76s_node("sd83")].form_type);
    TEST_ASSERT_TRUE(g_doc.nodes[l76s_node("sc83")].form_flags2 & WE_FF2_AC_OFF);
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76s_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_EQUAL(0, q.target_blank);
    TEST_ASSERT_EQUAL_STRING("w83", q.target);
    TEST_ASSERT_NOT_NULL(strstr(q.url, "sc83=%2337674b"));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "sd83=1993-12-03"));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "so83=R83"));
    l76s_done();
}

static void test_l76_sys_84(void) {
    we_form_req_t q;
    char html[700];
    sprintf(html, "<form action=/sys84 method=GET>"
                  "<input id=sc84 type=color name=sc84 value=#448474 autocomplete=off>"
                  "<input id=sd84 type=date name=sd84 value=1994-01-04>"
                  "<output id=so84 name=so84 value=R84>R84</output>"
                  "<input id=ss84 type=submit formtarget=_blank value=Go></form>");
    l76s_render(html);
    TEST_ASSERT_EQUAL(WE_FORM_COLOR, g_doc.nodes[l76s_node("sc84")].form_type);
    TEST_ASSERT_EQUAL(WE_FORM_DATE, g_doc.nodes[l76s_node("sd84")].form_type);
    TEST_ASSERT_TRUE(g_doc.nodes[l76s_node("sc84")].form_flags2 & WE_FF2_AC_OFF);
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76s_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_EQUAL(1, q.target_blank);
    TEST_ASSERT_EQUAL_STRING("_blank", q.target);
    TEST_ASSERT_NOT_NULL(strstr(q.url, "sc84=%23448474"));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "sd84=1994-01-04"));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "so84=R84"));
    l76s_done();
}

static void test_l76_sys_85(void) {
    we_form_req_t q;
    char html[700];
    sprintf(html, "<form action=/sys85 method=GET>"
                  "<input id=sc85 type=color name=sc85 value=#51a19d autocomplete=off>"
                  "<input id=sd85 type=date name=sd85 value=1995-02-05>"
                  "<output id=so85 name=so85 value=R85>R85</output>"
                  "<input id=ss85 type=submit formtarget=_self value=Go></form>");
    l76s_render(html);
    TEST_ASSERT_EQUAL(WE_FORM_COLOR, g_doc.nodes[l76s_node("sc85")].form_type);
    TEST_ASSERT_EQUAL(WE_FORM_DATE, g_doc.nodes[l76s_node("sd85")].form_type);
    TEST_ASSERT_TRUE(g_doc.nodes[l76s_node("sc85")].form_flags2 & WE_FF2_AC_OFF);
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76s_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_EQUAL(0, q.target_blank);
    TEST_ASSERT_EQUAL_STRING("_self", q.target);
    TEST_ASSERT_NOT_NULL(strstr(q.url, "sc85=%2351a19d"));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "sd85=1995-02-05"));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "so85=R85"));
    l76s_done();
}

static void test_l76_sys_86(void) {
    we_form_req_t q;
    char html[700];
    sprintf(html, "<form action=/sys86 method=GET>"
                  "<input id=sc86 type=color name=sc86 value=#5ebec6 autocomplete=off>"
                  "<input id=sd86 type=date name=sd86 value=1996-03-06>"
                  "<output id=so86 name=so86 value=R86>R86</output>"
                  "<input id=ss86 type=submit formtarget=w86 value=Go></form>");
    l76s_render(html);
    TEST_ASSERT_EQUAL(WE_FORM_COLOR, g_doc.nodes[l76s_node("sc86")].form_type);
    TEST_ASSERT_EQUAL(WE_FORM_DATE, g_doc.nodes[l76s_node("sd86")].form_type);
    TEST_ASSERT_TRUE(g_doc.nodes[l76s_node("sc86")].form_flags2 & WE_FF2_AC_OFF);
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76s_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_EQUAL(0, q.target_blank);
    TEST_ASSERT_EQUAL_STRING("w86", q.target);
    TEST_ASSERT_NOT_NULL(strstr(q.url, "sc86=%235ebec6"));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "sd86=1996-03-06"));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "so86=R86"));
    l76s_done();
}

static void test_l76_sys_87(void) {
    we_form_req_t q;
    char html[700];
    sprintf(html, "<form action=/sys87 method=GET>"
                  "<input id=sc87 type=color name=sc87 value=#6bdbef autocomplete=off>"
                  "<input id=sd87 type=date name=sd87 value=1997-04-07>"
                  "<output id=so87 name=so87 value=R87>R87</output>"
                  "<input id=ss87 type=submit formtarget=_blank value=Go></form>");
    l76s_render(html);
    TEST_ASSERT_EQUAL(WE_FORM_COLOR, g_doc.nodes[l76s_node("sc87")].form_type);
    TEST_ASSERT_EQUAL(WE_FORM_DATE, g_doc.nodes[l76s_node("sd87")].form_type);
    TEST_ASSERT_TRUE(g_doc.nodes[l76s_node("sc87")].form_flags2 & WE_FF2_AC_OFF);
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76s_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_EQUAL(1, q.target_blank);
    TEST_ASSERT_EQUAL_STRING("_blank", q.target);
    TEST_ASSERT_NOT_NULL(strstr(q.url, "sc87=%236bdbef"));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "sd87=1997-04-07"));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "so87=R87"));
    l76s_done();
}

static void test_l76_sys_88(void) {
    we_form_req_t q;
    char html[700];
    sprintf(html, "<form action=/sys88 method=GET>"
                  "<input id=sc88 type=color name=sc88 value=#78f818 autocomplete=off>"
                  "<input id=sd88 type=date name=sd88 value=1998-05-08>"
                  "<output id=so88 name=so88 value=R88>R88</output>"
                  "<input id=ss88 type=submit formtarget=_self value=Go></form>");
    l76s_render(html);
    TEST_ASSERT_EQUAL(WE_FORM_COLOR, g_doc.nodes[l76s_node("sc88")].form_type);
    TEST_ASSERT_EQUAL(WE_FORM_DATE, g_doc.nodes[l76s_node("sd88")].form_type);
    TEST_ASSERT_TRUE(g_doc.nodes[l76s_node("sc88")].form_flags2 & WE_FF2_AC_OFF);
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76s_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_EQUAL(0, q.target_blank);
    TEST_ASSERT_EQUAL_STRING("_self", q.target);
    TEST_ASSERT_NOT_NULL(strstr(q.url, "sc88=%2378f818"));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "sd88=1998-05-08"));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "so88=R88"));
    l76s_done();
}

static void test_l76_sys_89(void) {
    we_form_req_t q;
    char html[700];
    sprintf(html, "<form action=/sys89 method=GET>"
                  "<input id=sc89 type=color name=sc89 value=#851541 autocomplete=off>"
                  "<input id=sd89 type=date name=sd89 value=1999-06-09>"
                  "<output id=so89 name=so89 value=R89>R89</output>"
                  "<input id=ss89 type=submit formtarget=w89 value=Go></form>");
    l76s_render(html);
    TEST_ASSERT_EQUAL(WE_FORM_COLOR, g_doc.nodes[l76s_node("sc89")].form_type);
    TEST_ASSERT_EQUAL(WE_FORM_DATE, g_doc.nodes[l76s_node("sd89")].form_type);
    TEST_ASSERT_TRUE(g_doc.nodes[l76s_node("sc89")].form_flags2 & WE_FF2_AC_OFF);
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76s_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_EQUAL(0, q.target_blank);
    TEST_ASSERT_EQUAL_STRING("w89", q.target);
    TEST_ASSERT_NOT_NULL(strstr(q.url, "sc89=%23851541"));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "sd89=1999-06-09"));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "so89=R89"));
    l76s_done();
}

static void test_l76_sys_90(void) {
    we_form_req_t q;
    char html[700];
    sprintf(html, "<form action=/sys90 method=GET>"
                  "<input id=sc90 type=color name=sc90 value=#92326a autocomplete=off>"
                  "<input id=sd90 type=date name=sd90 value=2000-07-10>"
                  "<output id=so90 name=so90 value=R90>R90</output>"
                  "<input id=ss90 type=submit formtarget=_blank value=Go></form>");
    l76s_render(html);
    TEST_ASSERT_EQUAL(WE_FORM_COLOR, g_doc.nodes[l76s_node("sc90")].form_type);
    TEST_ASSERT_EQUAL(WE_FORM_DATE, g_doc.nodes[l76s_node("sd90")].form_type);
    TEST_ASSERT_TRUE(g_doc.nodes[l76s_node("sc90")].form_flags2 & WE_FF2_AC_OFF);
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76s_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_EQUAL(1, q.target_blank);
    TEST_ASSERT_EQUAL_STRING("_blank", q.target);
    TEST_ASSERT_NOT_NULL(strstr(q.url, "sc90=%2392326a"));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "sd90=2000-07-10"));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "so90=R90"));
    l76s_done();
}

static void test_l76_sys_91(void) {
    we_form_req_t q;
    char html[700];
    sprintf(html, "<form action=/sys91 method=GET>"
                  "<input id=sc91 type=color name=sc91 value=#9f4f93 autocomplete=off>"
                  "<input id=sd91 type=date name=sd91 value=2001-08-11>"
                  "<output id=so91 name=so91 value=R91>R91</output>"
                  "<input id=ss91 type=submit formtarget=_self value=Go></form>");
    l76s_render(html);
    TEST_ASSERT_EQUAL(WE_FORM_COLOR, g_doc.nodes[l76s_node("sc91")].form_type);
    TEST_ASSERT_EQUAL(WE_FORM_DATE, g_doc.nodes[l76s_node("sd91")].form_type);
    TEST_ASSERT_TRUE(g_doc.nodes[l76s_node("sc91")].form_flags2 & WE_FF2_AC_OFF);
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76s_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_EQUAL(0, q.target_blank);
    TEST_ASSERT_EQUAL_STRING("_self", q.target);
    TEST_ASSERT_NOT_NULL(strstr(q.url, "sc91=%239f4f93"));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "sd91=2001-08-11"));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "so91=R91"));
    l76s_done();
}

static void test_l76_sys_92(void) {
    we_form_req_t q;
    char html[700];
    sprintf(html, "<form action=/sys92 method=GET>"
                  "<input id=sc92 type=color name=sc92 value=#ac6cbc autocomplete=off>"
                  "<input id=sd92 type=date name=sd92 value=2002-09-12>"
                  "<output id=so92 name=so92 value=R92>R92</output>"
                  "<input id=ss92 type=submit formtarget=w92 value=Go></form>");
    l76s_render(html);
    TEST_ASSERT_EQUAL(WE_FORM_COLOR, g_doc.nodes[l76s_node("sc92")].form_type);
    TEST_ASSERT_EQUAL(WE_FORM_DATE, g_doc.nodes[l76s_node("sd92")].form_type);
    TEST_ASSERT_TRUE(g_doc.nodes[l76s_node("sc92")].form_flags2 & WE_FF2_AC_OFF);
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76s_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_EQUAL(0, q.target_blank);
    TEST_ASSERT_EQUAL_STRING("w92", q.target);
    TEST_ASSERT_NOT_NULL(strstr(q.url, "sc92=%23ac6cbc"));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "sd92=2002-09-12"));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "so92=R92"));
    l76s_done();
}

static void test_l76_sys_93(void) {
    we_form_req_t q;
    char html[700];
    sprintf(html, "<form action=/sys93 method=GET>"
                  "<input id=sc93 type=color name=sc93 value=#b989e5 autocomplete=off>"
                  "<input id=sd93 type=date name=sd93 value=2003-10-13>"
                  "<output id=so93 name=so93 value=R93>R93</output>"
                  "<input id=ss93 type=submit formtarget=_blank value=Go></form>");
    l76s_render(html);
    TEST_ASSERT_EQUAL(WE_FORM_COLOR, g_doc.nodes[l76s_node("sc93")].form_type);
    TEST_ASSERT_EQUAL(WE_FORM_DATE, g_doc.nodes[l76s_node("sd93")].form_type);
    TEST_ASSERT_TRUE(g_doc.nodes[l76s_node("sc93")].form_flags2 & WE_FF2_AC_OFF);
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76s_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_EQUAL(1, q.target_blank);
    TEST_ASSERT_EQUAL_STRING("_blank", q.target);
    TEST_ASSERT_NOT_NULL(strstr(q.url, "sc93=%23b989e5"));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "sd93=2003-10-13"));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "so93=R93"));
    l76s_done();
}

static void test_l76_sys_94(void) {
    we_form_req_t q;
    char html[700];
    sprintf(html, "<form action=/sys94 method=GET>"
                  "<input id=sc94 type=color name=sc94 value=#c6a60e autocomplete=off>"
                  "<input id=sd94 type=date name=sd94 value=2004-11-14>"
                  "<output id=so94 name=so94 value=R94>R94</output>"
                  "<input id=ss94 type=submit formtarget=_self value=Go></form>");
    l76s_render(html);
    TEST_ASSERT_EQUAL(WE_FORM_COLOR, g_doc.nodes[l76s_node("sc94")].form_type);
    TEST_ASSERT_EQUAL(WE_FORM_DATE, g_doc.nodes[l76s_node("sd94")].form_type);
    TEST_ASSERT_TRUE(g_doc.nodes[l76s_node("sc94")].form_flags2 & WE_FF2_AC_OFF);
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76s_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_EQUAL(0, q.target_blank);
    TEST_ASSERT_EQUAL_STRING("_self", q.target);
    TEST_ASSERT_NOT_NULL(strstr(q.url, "sc94=%23c6a60e"));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "sd94=2004-11-14"));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "so94=R94"));
    l76s_done();
}

static void test_l76_sys_95(void) {
    we_form_req_t q;
    char html[700];
    sprintf(html, "<form action=/sys95 method=GET>"
                  "<input id=sc95 type=color name=sc95 value=#d3c337 autocomplete=off>"
                  "<input id=sd95 type=date name=sd95 value=2005-12-15>"
                  "<output id=so95 name=so95 value=R95>R95</output>"
                  "<input id=ss95 type=submit formtarget=w95 value=Go></form>");
    l76s_render(html);
    TEST_ASSERT_EQUAL(WE_FORM_COLOR, g_doc.nodes[l76s_node("sc95")].form_type);
    TEST_ASSERT_EQUAL(WE_FORM_DATE, g_doc.nodes[l76s_node("sd95")].form_type);
    TEST_ASSERT_TRUE(g_doc.nodes[l76s_node("sc95")].form_flags2 & WE_FF2_AC_OFF);
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76s_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_EQUAL(0, q.target_blank);
    TEST_ASSERT_EQUAL_STRING("w95", q.target);
    TEST_ASSERT_NOT_NULL(strstr(q.url, "sc95=%23d3c337"));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "sd95=2005-12-15"));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "so95=R95"));
    l76s_done();
}

static void test_l76_sys_96(void) {
    we_form_req_t q;
    char html[700];
    sprintf(html, "<form action=/sys96 method=GET>"
                  "<input id=sc96 type=color name=sc96 value=#e0e060 autocomplete=off>"
                  "<input id=sd96 type=date name=sd96 value=2006-01-16>"
                  "<output id=so96 name=so96 value=R96>R96</output>"
                  "<input id=ss96 type=submit formtarget=_blank value=Go></form>");
    l76s_render(html);
    TEST_ASSERT_EQUAL(WE_FORM_COLOR, g_doc.nodes[l76s_node("sc96")].form_type);
    TEST_ASSERT_EQUAL(WE_FORM_DATE, g_doc.nodes[l76s_node("sd96")].form_type);
    TEST_ASSERT_TRUE(g_doc.nodes[l76s_node("sc96")].form_flags2 & WE_FF2_AC_OFF);
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76s_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_EQUAL(1, q.target_blank);
    TEST_ASSERT_EQUAL_STRING("_blank", q.target);
    TEST_ASSERT_NOT_NULL(strstr(q.url, "sc96=%23e0e060"));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "sd96=2006-01-16"));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "so96=R96"));
    l76s_done();
}

static void test_l76_sys_97(void) {
    we_form_req_t q;
    char html[700];
    sprintf(html, "<form action=/sys97 method=GET>"
                  "<input id=sc97 type=color name=sc97 value=#edfd89 autocomplete=off>"
                  "<input id=sd97 type=date name=sd97 value=2007-02-17>"
                  "<output id=so97 name=so97 value=R97>R97</output>"
                  "<input id=ss97 type=submit formtarget=_self value=Go></form>");
    l76s_render(html);
    TEST_ASSERT_EQUAL(WE_FORM_COLOR, g_doc.nodes[l76s_node("sc97")].form_type);
    TEST_ASSERT_EQUAL(WE_FORM_DATE, g_doc.nodes[l76s_node("sd97")].form_type);
    TEST_ASSERT_TRUE(g_doc.nodes[l76s_node("sc97")].form_flags2 & WE_FF2_AC_OFF);
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76s_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_EQUAL(0, q.target_blank);
    TEST_ASSERT_EQUAL_STRING("_self", q.target);
    TEST_ASSERT_NOT_NULL(strstr(q.url, "sc97=%23edfd89"));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "sd97=2007-02-17"));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "so97=R97"));
    l76s_done();
}

static void test_l76_sys_98(void) {
    we_form_req_t q;
    char html[700];
    sprintf(html, "<form action=/sys98 method=GET>"
                  "<input id=sc98 type=color name=sc98 value=#fa1ab2 autocomplete=off>"
                  "<input id=sd98 type=date name=sd98 value=2008-03-18>"
                  "<output id=so98 name=so98 value=R98>R98</output>"
                  "<input id=ss98 type=submit formtarget=w98 value=Go></form>");
    l76s_render(html);
    TEST_ASSERT_EQUAL(WE_FORM_COLOR, g_doc.nodes[l76s_node("sc98")].form_type);
    TEST_ASSERT_EQUAL(WE_FORM_DATE, g_doc.nodes[l76s_node("sd98")].form_type);
    TEST_ASSERT_TRUE(g_doc.nodes[l76s_node("sc98")].form_flags2 & WE_FF2_AC_OFF);
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76s_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_EQUAL(0, q.target_blank);
    TEST_ASSERT_EQUAL_STRING("w98", q.target);
    TEST_ASSERT_NOT_NULL(strstr(q.url, "sc98=%23fa1ab2"));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "sd98=2008-03-18"));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "so98=R98"));
    l76s_done();
}

static void test_l76_sys_99(void) {
    we_form_req_t q;
    char html[700];
    sprintf(html, "<form action=/sys99 method=GET>"
                  "<input id=sc99 type=color name=sc99 value=#0737db autocomplete=off>"
                  "<input id=sd99 type=date name=sd99 value=2009-04-19>"
                  "<output id=so99 name=so99 value=R99>R99</output>"
                  "<input id=ss99 type=submit formtarget=_blank value=Go></form>");
    l76s_render(html);
    TEST_ASSERT_EQUAL(WE_FORM_COLOR, g_doc.nodes[l76s_node("sc99")].form_type);
    TEST_ASSERT_EQUAL(WE_FORM_DATE, g_doc.nodes[l76s_node("sd99")].form_type);
    TEST_ASSERT_TRUE(g_doc.nodes[l76s_node("sc99")].form_flags2 & WE_FF2_AC_OFF);
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76s_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_EQUAL(1, q.target_blank);
    TEST_ASSERT_EQUAL_STRING("_blank", q.target);
    TEST_ASSERT_NOT_NULL(strstr(q.url, "sc99=%230737db"));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "sd99=2009-04-19"));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "so99=R99"));
    l76s_done();
}

static void test_l76_sys_100(void) {
    we_form_req_t q;
    char html[700];
    sprintf(html, "<form action=/sys100 method=GET>"
                  "<input id=sc100 type=color name=sc100 value=#145404 autocomplete=off>"
                  "<input id=sd100 type=date name=sd100 value=2010-05-20>"
                  "<output id=so100 name=so100 value=R100>R100</output>"
                  "<input id=ss100 type=submit formtarget=_self value=Go></form>");
    l76s_render(html);
    TEST_ASSERT_EQUAL(WE_FORM_COLOR, g_doc.nodes[l76s_node("sc100")].form_type);
    TEST_ASSERT_EQUAL(WE_FORM_DATE, g_doc.nodes[l76s_node("sd100")].form_type);
    TEST_ASSERT_TRUE(g_doc.nodes[l76s_node("sc100")].form_flags2 & WE_FF2_AC_OFF);
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76s_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_EQUAL(0, q.target_blank);
    TEST_ASSERT_EQUAL_STRING("_self", q.target);
    TEST_ASSERT_NOT_NULL(strstr(q.url, "sc100=%23145404"));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "sd100=2010-05-20"));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "so100=R100"));
    l76s_done();
}

static void test_l76_sys_101(void) {
    we_form_req_t q;
    char html[700];
    sprintf(html, "<form action=/sys101 method=GET>"
                  "<input id=sc101 type=color name=sc101 value=#21712d autocomplete=off>"
                  "<input id=sd101 type=date name=sd101 value=2011-06-21>"
                  "<output id=so101 name=so101 value=R101>R101</output>"
                  "<input id=ss101 type=submit formtarget=w101 value=Go></form>");
    l76s_render(html);
    TEST_ASSERT_EQUAL(WE_FORM_COLOR, g_doc.nodes[l76s_node("sc101")].form_type);
    TEST_ASSERT_EQUAL(WE_FORM_DATE, g_doc.nodes[l76s_node("sd101")].form_type);
    TEST_ASSERT_TRUE(g_doc.nodes[l76s_node("sc101")].form_flags2 & WE_FF2_AC_OFF);
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76s_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_EQUAL(0, q.target_blank);
    TEST_ASSERT_EQUAL_STRING("w101", q.target);
    TEST_ASSERT_NOT_NULL(strstr(q.url, "sc101=%2321712d"));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "sd101=2011-06-21"));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "so101=R101"));
    l76s_done();
}

static void test_l76_sys_102(void) {
    we_form_req_t q;
    char html[700];
    sprintf(html, "<form action=/sys102 method=GET>"
                  "<input id=sc102 type=color name=sc102 value=#2e8e56 autocomplete=off>"
                  "<input id=sd102 type=date name=sd102 value=2012-07-22>"
                  "<output id=so102 name=so102 value=R102>R102</output>"
                  "<input id=ss102 type=submit formtarget=_blank value=Go></form>");
    l76s_render(html);
    TEST_ASSERT_EQUAL(WE_FORM_COLOR, g_doc.nodes[l76s_node("sc102")].form_type);
    TEST_ASSERT_EQUAL(WE_FORM_DATE, g_doc.nodes[l76s_node("sd102")].form_type);
    TEST_ASSERT_TRUE(g_doc.nodes[l76s_node("sc102")].form_flags2 & WE_FF2_AC_OFF);
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76s_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_EQUAL(1, q.target_blank);
    TEST_ASSERT_EQUAL_STRING("_blank", q.target);
    TEST_ASSERT_NOT_NULL(strstr(q.url, "sc102=%232e8e56"));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "sd102=2012-07-22"));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "so102=R102"));
    l76s_done();
}

static void test_l76_sys_103(void) {
    we_form_req_t q;
    char html[700];
    sprintf(html, "<form action=/sys103 method=GET>"
                  "<input id=sc103 type=color name=sc103 value=#3bab7f autocomplete=off>"
                  "<input id=sd103 type=date name=sd103 value=2013-08-23>"
                  "<output id=so103 name=so103 value=R103>R103</output>"
                  "<input id=ss103 type=submit formtarget=_self value=Go></form>");
    l76s_render(html);
    TEST_ASSERT_EQUAL(WE_FORM_COLOR, g_doc.nodes[l76s_node("sc103")].form_type);
    TEST_ASSERT_EQUAL(WE_FORM_DATE, g_doc.nodes[l76s_node("sd103")].form_type);
    TEST_ASSERT_TRUE(g_doc.nodes[l76s_node("sc103")].form_flags2 & WE_FF2_AC_OFF);
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76s_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_EQUAL(0, q.target_blank);
    TEST_ASSERT_EQUAL_STRING("_self", q.target);
    TEST_ASSERT_NOT_NULL(strstr(q.url, "sc103=%233bab7f"));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "sd103=2013-08-23"));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "so103=R103"));
    l76s_done();
}

static void test_l76_sys_104(void) {
    we_form_req_t q;
    char html[700];
    sprintf(html, "<form action=/sys104 method=GET>"
                  "<input id=sc104 type=color name=sc104 value=#48c8a8 autocomplete=off>"
                  "<input id=sd104 type=date name=sd104 value=2014-09-24>"
                  "<output id=so104 name=so104 value=R104>R104</output>"
                  "<input id=ss104 type=submit formtarget=w104 value=Go></form>");
    l76s_render(html);
    TEST_ASSERT_EQUAL(WE_FORM_COLOR, g_doc.nodes[l76s_node("sc104")].form_type);
    TEST_ASSERT_EQUAL(WE_FORM_DATE, g_doc.nodes[l76s_node("sd104")].form_type);
    TEST_ASSERT_TRUE(g_doc.nodes[l76s_node("sc104")].form_flags2 & WE_FF2_AC_OFF);
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76s_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_EQUAL(0, q.target_blank);
    TEST_ASSERT_EQUAL_STRING("w104", q.target);
    TEST_ASSERT_NOT_NULL(strstr(q.url, "sc104=%2348c8a8"));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "sd104=2014-09-24"));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "so104=R104"));
    l76s_done();
}

static void test_l76_sys_105(void) {
    we_form_req_t q;
    char html[700];
    sprintf(html, "<form action=/sys105 method=GET>"
                  "<input id=sc105 type=color name=sc105 value=#55e5d1 autocomplete=off>"
                  "<input id=sd105 type=date name=sd105 value=2015-10-25>"
                  "<output id=so105 name=so105 value=R105>R105</output>"
                  "<input id=ss105 type=submit formtarget=_blank value=Go></form>");
    l76s_render(html);
    TEST_ASSERT_EQUAL(WE_FORM_COLOR, g_doc.nodes[l76s_node("sc105")].form_type);
    TEST_ASSERT_EQUAL(WE_FORM_DATE, g_doc.nodes[l76s_node("sd105")].form_type);
    TEST_ASSERT_TRUE(g_doc.nodes[l76s_node("sc105")].form_flags2 & WE_FF2_AC_OFF);
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76s_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_EQUAL(1, q.target_blank);
    TEST_ASSERT_EQUAL_STRING("_blank", q.target);
    TEST_ASSERT_NOT_NULL(strstr(q.url, "sc105=%2355e5d1"));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "sd105=2015-10-25"));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "so105=R105"));
    l76s_done();
}

static void test_l76_sys_106(void) {
    we_form_req_t q;
    char html[700];
    sprintf(html, "<form action=/sys106 method=GET>"
                  "<input id=sc106 type=color name=sc106 value=#6202fa autocomplete=off>"
                  "<input id=sd106 type=date name=sd106 value=2016-11-26>"
                  "<output id=so106 name=so106 value=R106>R106</output>"
                  "<input id=ss106 type=submit formtarget=_self value=Go></form>");
    l76s_render(html);
    TEST_ASSERT_EQUAL(WE_FORM_COLOR, g_doc.nodes[l76s_node("sc106")].form_type);
    TEST_ASSERT_EQUAL(WE_FORM_DATE, g_doc.nodes[l76s_node("sd106")].form_type);
    TEST_ASSERT_TRUE(g_doc.nodes[l76s_node("sc106")].form_flags2 & WE_FF2_AC_OFF);
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76s_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_EQUAL(0, q.target_blank);
    TEST_ASSERT_EQUAL_STRING("_self", q.target);
    TEST_ASSERT_NOT_NULL(strstr(q.url, "sc106=%236202fa"));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "sd106=2016-11-26"));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "so106=R106"));
    l76s_done();
}

static void test_l76_sys_107(void) {
    we_form_req_t q;
    char html[700];
    sprintf(html, "<form action=/sys107 method=GET>"
                  "<input id=sc107 type=color name=sc107 value=#6f1f23 autocomplete=off>"
                  "<input id=sd107 type=date name=sd107 value=2017-12-27>"
                  "<output id=so107 name=so107 value=R107>R107</output>"
                  "<input id=ss107 type=submit formtarget=w107 value=Go></form>");
    l76s_render(html);
    TEST_ASSERT_EQUAL(WE_FORM_COLOR, g_doc.nodes[l76s_node("sc107")].form_type);
    TEST_ASSERT_EQUAL(WE_FORM_DATE, g_doc.nodes[l76s_node("sd107")].form_type);
    TEST_ASSERT_TRUE(g_doc.nodes[l76s_node("sc107")].form_flags2 & WE_FF2_AC_OFF);
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76s_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_EQUAL(0, q.target_blank);
    TEST_ASSERT_EQUAL_STRING("w107", q.target);
    TEST_ASSERT_NOT_NULL(strstr(q.url, "sc107=%236f1f23"));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "sd107=2017-12-27"));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "so107=R107"));
    l76s_done();
}

static void test_l76_sys_108(void) {
    we_form_req_t q;
    char html[700];
    sprintf(html, "<form action=/sys108 method=GET>"
                  "<input id=sc108 type=color name=sc108 value=#7c3c4c autocomplete=off>"
                  "<input id=sd108 type=date name=sd108 value=2018-01-01>"
                  "<output id=so108 name=so108 value=R108>R108</output>"
                  "<input id=ss108 type=submit formtarget=_blank value=Go></form>");
    l76s_render(html);
    TEST_ASSERT_EQUAL(WE_FORM_COLOR, g_doc.nodes[l76s_node("sc108")].form_type);
    TEST_ASSERT_EQUAL(WE_FORM_DATE, g_doc.nodes[l76s_node("sd108")].form_type);
    TEST_ASSERT_TRUE(g_doc.nodes[l76s_node("sc108")].form_flags2 & WE_FF2_AC_OFF);
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76s_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_EQUAL(1, q.target_blank);
    TEST_ASSERT_EQUAL_STRING("_blank", q.target);
    TEST_ASSERT_NOT_NULL(strstr(q.url, "sc108=%237c3c4c"));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "sd108=2018-01-01"));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "so108=R108"));
    l76s_done();
}

static void test_l76_sys_109(void) {
    we_form_req_t q;
    char html[700];
    sprintf(html, "<form action=/sys109 method=GET>"
                  "<input id=sc109 type=color name=sc109 value=#895975 autocomplete=off>"
                  "<input id=sd109 type=date name=sd109 value=2019-02-02>"
                  "<output id=so109 name=so109 value=R109>R109</output>"
                  "<input id=ss109 type=submit formtarget=_self value=Go></form>");
    l76s_render(html);
    TEST_ASSERT_EQUAL(WE_FORM_COLOR, g_doc.nodes[l76s_node("sc109")].form_type);
    TEST_ASSERT_EQUAL(WE_FORM_DATE, g_doc.nodes[l76s_node("sd109")].form_type);
    TEST_ASSERT_TRUE(g_doc.nodes[l76s_node("sc109")].form_flags2 & WE_FF2_AC_OFF);
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76s_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_EQUAL(0, q.target_blank);
    TEST_ASSERT_EQUAL_STRING("_self", q.target);
    TEST_ASSERT_NOT_NULL(strstr(q.url, "sc109=%23895975"));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "sd109=2019-02-02"));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "so109=R109"));
    l76s_done();
}

static void test_l76_sys_110(void) {
    we_form_req_t q;
    char html[700];
    sprintf(html, "<form action=/sys110 method=GET>"
                  "<input id=sc110 type=color name=sc110 value=#96769e autocomplete=off>"
                  "<input id=sd110 type=date name=sd110 value=2020-03-03>"
                  "<output id=so110 name=so110 value=R110>R110</output>"
                  "<input id=ss110 type=submit formtarget=w110 value=Go></form>");
    l76s_render(html);
    TEST_ASSERT_EQUAL(WE_FORM_COLOR, g_doc.nodes[l76s_node("sc110")].form_type);
    TEST_ASSERT_EQUAL(WE_FORM_DATE, g_doc.nodes[l76s_node("sd110")].form_type);
    TEST_ASSERT_TRUE(g_doc.nodes[l76s_node("sc110")].form_flags2 & WE_FF2_AC_OFF);
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76s_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_EQUAL(0, q.target_blank);
    TEST_ASSERT_EQUAL_STRING("w110", q.target);
    TEST_ASSERT_NOT_NULL(strstr(q.url, "sc110=%2396769e"));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "sd110=2020-03-03"));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "so110=R110"));
    l76s_done();
}

static void test_l76_sys_111(void) {
    we_form_req_t q;
    char html[700];
    sprintf(html, "<form action=/sys111 method=GET>"
                  "<input id=sc111 type=color name=sc111 value=#a393c7 autocomplete=off>"
                  "<input id=sd111 type=date name=sd111 value=2021-04-04>"
                  "<output id=so111 name=so111 value=R111>R111</output>"
                  "<input id=ss111 type=submit formtarget=_blank value=Go></form>");
    l76s_render(html);
    TEST_ASSERT_EQUAL(WE_FORM_COLOR, g_doc.nodes[l76s_node("sc111")].form_type);
    TEST_ASSERT_EQUAL(WE_FORM_DATE, g_doc.nodes[l76s_node("sd111")].form_type);
    TEST_ASSERT_TRUE(g_doc.nodes[l76s_node("sc111")].form_flags2 & WE_FF2_AC_OFF);
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76s_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_EQUAL(1, q.target_blank);
    TEST_ASSERT_EQUAL_STRING("_blank", q.target);
    TEST_ASSERT_NOT_NULL(strstr(q.url, "sc111=%23a393c7"));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "sd111=2021-04-04"));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "so111=R111"));
    l76s_done();
}

static void test_l76_sys_112(void) {
    we_form_req_t q;
    char html[700];
    sprintf(html, "<form action=/sys112 method=GET>"
                  "<input id=sc112 type=color name=sc112 value=#b0b0f0 autocomplete=off>"
                  "<input id=sd112 type=date name=sd112 value=2022-05-05>"
                  "<output id=so112 name=so112 value=R112>R112</output>"
                  "<input id=ss112 type=submit formtarget=_self value=Go></form>");
    l76s_render(html);
    TEST_ASSERT_EQUAL(WE_FORM_COLOR, g_doc.nodes[l76s_node("sc112")].form_type);
    TEST_ASSERT_EQUAL(WE_FORM_DATE, g_doc.nodes[l76s_node("sd112")].form_type);
    TEST_ASSERT_TRUE(g_doc.nodes[l76s_node("sc112")].form_flags2 & WE_FF2_AC_OFF);
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76s_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_EQUAL(0, q.target_blank);
    TEST_ASSERT_EQUAL_STRING("_self", q.target);
    TEST_ASSERT_NOT_NULL(strstr(q.url, "sc112=%23b0b0f0"));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "sd112=2022-05-05"));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "so112=R112"));
    l76s_done();
}

static void test_l76_sys_113(void) {
    we_form_req_t q;
    char html[700];
    sprintf(html, "<form action=/sys113 method=GET>"
                  "<input id=sc113 type=color name=sc113 value=#bdcd19 autocomplete=off>"
                  "<input id=sd113 type=date name=sd113 value=2023-06-06>"
                  "<output id=so113 name=so113 value=R113>R113</output>"
                  "<input id=ss113 type=submit formtarget=w113 value=Go></form>");
    l76s_render(html);
    TEST_ASSERT_EQUAL(WE_FORM_COLOR, g_doc.nodes[l76s_node("sc113")].form_type);
    TEST_ASSERT_EQUAL(WE_FORM_DATE, g_doc.nodes[l76s_node("sd113")].form_type);
    TEST_ASSERT_TRUE(g_doc.nodes[l76s_node("sc113")].form_flags2 & WE_FF2_AC_OFF);
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76s_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_EQUAL(0, q.target_blank);
    TEST_ASSERT_EQUAL_STRING("w113", q.target);
    TEST_ASSERT_NOT_NULL(strstr(q.url, "sc113=%23bdcd19"));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "sd113=2023-06-06"));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "so113=R113"));
    l76s_done();
}

static void test_l76_sys_114(void) {
    we_form_req_t q;
    char html[700];
    sprintf(html, "<form action=/sys114 method=GET>"
                  "<input id=sc114 type=color name=sc114 value=#caea42 autocomplete=off>"
                  "<input id=sd114 type=date name=sd114 value=2024-07-07>"
                  "<output id=so114 name=so114 value=R114>R114</output>"
                  "<input id=ss114 type=submit formtarget=_blank value=Go></form>");
    l76s_render(html);
    TEST_ASSERT_EQUAL(WE_FORM_COLOR, g_doc.nodes[l76s_node("sc114")].form_type);
    TEST_ASSERT_EQUAL(WE_FORM_DATE, g_doc.nodes[l76s_node("sd114")].form_type);
    TEST_ASSERT_TRUE(g_doc.nodes[l76s_node("sc114")].form_flags2 & WE_FF2_AC_OFF);
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76s_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_EQUAL(1, q.target_blank);
    TEST_ASSERT_EQUAL_STRING("_blank", q.target);
    TEST_ASSERT_NOT_NULL(strstr(q.url, "sc114=%23caea42"));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "sd114=2024-07-07"));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "so114=R114"));
    l76s_done();
}

static void test_l76_sys_115(void) {
    we_form_req_t q;
    char html[700];
    sprintf(html, "<form action=/sys115 method=GET>"
                  "<input id=sc115 type=color name=sc115 value=#d7076b autocomplete=off>"
                  "<input id=sd115 type=date name=sd115 value=2025-08-08>"
                  "<output id=so115 name=so115 value=R115>R115</output>"
                  "<input id=ss115 type=submit formtarget=_self value=Go></form>");
    l76s_render(html);
    TEST_ASSERT_EQUAL(WE_FORM_COLOR, g_doc.nodes[l76s_node("sc115")].form_type);
    TEST_ASSERT_EQUAL(WE_FORM_DATE, g_doc.nodes[l76s_node("sd115")].form_type);
    TEST_ASSERT_TRUE(g_doc.nodes[l76s_node("sc115")].form_flags2 & WE_FF2_AC_OFF);
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76s_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_EQUAL(0, q.target_blank);
    TEST_ASSERT_EQUAL_STRING("_self", q.target);
    TEST_ASSERT_NOT_NULL(strstr(q.url, "sc115=%23d7076b"));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "sd115=2025-08-08"));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "so115=R115"));
    l76s_done();
}

static void test_l76_sys_116(void) {
    we_form_req_t q;
    char html[700];
    sprintf(html, "<form action=/sys116 method=GET>"
                  "<input id=sc116 type=color name=sc116 value=#e42494 autocomplete=off>"
                  "<input id=sd116 type=date name=sd116 value=2026-09-09>"
                  "<output id=so116 name=so116 value=R116>R116</output>"
                  "<input id=ss116 type=submit formtarget=w116 value=Go></form>");
    l76s_render(html);
    TEST_ASSERT_EQUAL(WE_FORM_COLOR, g_doc.nodes[l76s_node("sc116")].form_type);
    TEST_ASSERT_EQUAL(WE_FORM_DATE, g_doc.nodes[l76s_node("sd116")].form_type);
    TEST_ASSERT_TRUE(g_doc.nodes[l76s_node("sc116")].form_flags2 & WE_FF2_AC_OFF);
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76s_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_EQUAL(0, q.target_blank);
    TEST_ASSERT_EQUAL_STRING("w116", q.target);
    TEST_ASSERT_NOT_NULL(strstr(q.url, "sc116=%23e42494"));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "sd116=2026-09-09"));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "so116=R116"));
    l76s_done();
}

static void test_l76_sys_117(void) {
    we_form_req_t q;
    char html[700];
    sprintf(html, "<form action=/sys117 method=GET>"
                  "<input id=sc117 type=color name=sc117 value=#f141bd autocomplete=off>"
                  "<input id=sd117 type=date name=sd117 value=2027-10-10>"
                  "<output id=so117 name=so117 value=R117>R117</output>"
                  "<input id=ss117 type=submit formtarget=_blank value=Go></form>");
    l76s_render(html);
    TEST_ASSERT_EQUAL(WE_FORM_COLOR, g_doc.nodes[l76s_node("sc117")].form_type);
    TEST_ASSERT_EQUAL(WE_FORM_DATE, g_doc.nodes[l76s_node("sd117")].form_type);
    TEST_ASSERT_TRUE(g_doc.nodes[l76s_node("sc117")].form_flags2 & WE_FF2_AC_OFF);
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76s_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_EQUAL(1, q.target_blank);
    TEST_ASSERT_EQUAL_STRING("_blank", q.target);
    TEST_ASSERT_NOT_NULL(strstr(q.url, "sc117=%23f141bd"));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "sd117=2027-10-10"));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "so117=R117"));
    l76s_done();
}

static void test_l76_sys_118(void) {
    we_form_req_t q;
    char html[700];
    sprintf(html, "<form action=/sys118 method=GET>"
                  "<input id=sc118 type=color name=sc118 value=#fe5ee6 autocomplete=off>"
                  "<input id=sd118 type=date name=sd118 value=2028-11-11>"
                  "<output id=so118 name=so118 value=R118>R118</output>"
                  "<input id=ss118 type=submit formtarget=_self value=Go></form>");
    l76s_render(html);
    TEST_ASSERT_EQUAL(WE_FORM_COLOR, g_doc.nodes[l76s_node("sc118")].form_type);
    TEST_ASSERT_EQUAL(WE_FORM_DATE, g_doc.nodes[l76s_node("sd118")].form_type);
    TEST_ASSERT_TRUE(g_doc.nodes[l76s_node("sc118")].form_flags2 & WE_FF2_AC_OFF);
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76s_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_EQUAL(0, q.target_blank);
    TEST_ASSERT_EQUAL_STRING("_self", q.target);
    TEST_ASSERT_NOT_NULL(strstr(q.url, "sc118=%23fe5ee6"));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "sd118=2028-11-11"));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "so118=R118"));
    l76s_done();
}

static void test_l76_sys_119(void) {
    we_form_req_t q;
    char html[700];
    sprintf(html, "<form action=/sys119 method=GET>"
                  "<input id=sc119 type=color name=sc119 value=#0b7b0f autocomplete=off>"
                  "<input id=sd119 type=date name=sd119 value=2029-12-12>"
                  "<output id=so119 name=so119 value=R119>R119</output>"
                  "<input id=ss119 type=submit formtarget=w119 value=Go></form>");
    l76s_render(html);
    TEST_ASSERT_EQUAL(WE_FORM_COLOR, g_doc.nodes[l76s_node("sc119")].form_type);
    TEST_ASSERT_EQUAL(WE_FORM_DATE, g_doc.nodes[l76s_node("sd119")].form_type);
    TEST_ASSERT_TRUE(g_doc.nodes[l76s_node("sc119")].form_flags2 & WE_FF2_AC_OFF);
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76s_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_EQUAL(0, q.target_blank);
    TEST_ASSERT_EQUAL_STRING("w119", q.target);
    TEST_ASSERT_NOT_NULL(strstr(q.url, "sc119=%230b7b0f"));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "sd119=2029-12-12"));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "so119=R119"));
    l76s_done();
}

static void test_l76_sys_120(void) {
    we_form_req_t q;
    char html[700];
    sprintf(html, "<form action=/sys120 method=GET>"
                  "<input id=sc120 type=color name=sc120 value=#189838 autocomplete=off>"
                  "<input id=sd120 type=date name=sd120 value=1990-01-13>"
                  "<output id=so120 name=so120 value=R120>R120</output>"
                  "<input id=ss120 type=submit formtarget=_blank value=Go></form>");
    l76s_render(html);
    TEST_ASSERT_EQUAL(WE_FORM_COLOR, g_doc.nodes[l76s_node("sc120")].form_type);
    TEST_ASSERT_EQUAL(WE_FORM_DATE, g_doc.nodes[l76s_node("sd120")].form_type);
    TEST_ASSERT_TRUE(g_doc.nodes[l76s_node("sc120")].form_flags2 & WE_FF2_AC_OFF);
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76s_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_EQUAL(1, q.target_blank);
    TEST_ASSERT_EQUAL_STRING("_blank", q.target);
    TEST_ASSERT_NOT_NULL(strstr(q.url, "sc120=%23189838"));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "sd120=1990-01-13"));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "so120=R120"));
    l76s_done();
}

static void test_l76_sys_md_1(void) {
    we_form_req_t q;
    char html[400];
    sprintf(html, "<form action=/md1 method=GET>"
                  "<input id=md1 type=date name=md1 value=2021-01-31>"
                  "<input type=submit></form>");
    l76s_render(html);
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76s_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "md1=2021-01-31"));
    l76s_done();
}

static void test_l76_sys_md_2(void) {
    we_form_req_t q;
    char html[400];
    sprintf(html, "<form action=/md2 method=GET>"
                  "<input id=md2 type=date name=md2 value=2021-02-28>"
                  "<input type=submit></form>");
    l76s_render(html);
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76s_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "md2=2021-02-28"));
    l76s_done();
}

static void test_l76_sys_md_3(void) {
    we_form_req_t q;
    char html[400];
    sprintf(html, "<form action=/md3 method=GET>"
                  "<input id=md3 type=date name=md3 value=2021-03-31>"
                  "<input type=submit></form>");
    l76s_render(html);
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76s_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "md3=2021-03-31"));
    l76s_done();
}

static void test_l76_sys_md_4(void) {
    we_form_req_t q;
    char html[400];
    sprintf(html, "<form action=/md4 method=GET>"
                  "<input id=md4 type=date name=md4 value=2021-04-30>"
                  "<input type=submit></form>");
    l76s_render(html);
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76s_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "md4=2021-04-30"));
    l76s_done();
}

static void test_l76_sys_md_5(void) {
    we_form_req_t q;
    char html[400];
    sprintf(html, "<form action=/md5 method=GET>"
                  "<input id=md5 type=date name=md5 value=2021-05-31>"
                  "<input type=submit></form>");
    l76s_render(html);
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76s_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "md5=2021-05-31"));
    l76s_done();
}

static void test_l76_sys_md_6(void) {
    we_form_req_t q;
    char html[400];
    sprintf(html, "<form action=/md6 method=GET>"
                  "<input id=md6 type=date name=md6 value=2021-06-30>"
                  "<input type=submit></form>");
    l76s_render(html);
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76s_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "md6=2021-06-30"));
    l76s_done();
}

static void test_l76_sys_md_7(void) {
    we_form_req_t q;
    char html[400];
    sprintf(html, "<form action=/md7 method=GET>"
                  "<input id=md7 type=date name=md7 value=2021-07-31>"
                  "<input type=submit></form>");
    l76s_render(html);
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76s_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "md7=2021-07-31"));
    l76s_done();
}

static void test_l76_sys_md_8(void) {
    we_form_req_t q;
    char html[400];
    sprintf(html, "<form action=/md8 method=GET>"
                  "<input id=md8 type=date name=md8 value=2021-08-31>"
                  "<input type=submit></form>");
    l76s_render(html);
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76s_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "md8=2021-08-31"));
    l76s_done();
}

static void test_l76_sys_md_9(void) {
    we_form_req_t q;
    char html[400];
    sprintf(html, "<form action=/md9 method=GET>"
                  "<input id=md9 type=date name=md9 value=2021-09-30>"
                  "<input type=submit></form>");
    l76s_render(html);
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76s_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "md9=2021-09-30"));
    l76s_done();
}

static void test_l76_sys_md_10(void) {
    we_form_req_t q;
    char html[400];
    sprintf(html, "<form action=/md10 method=GET>"
                  "<input id=md10 type=date name=md10 value=2021-10-31>"
                  "<input type=submit></form>");
    l76s_render(html);
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76s_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "md10=2021-10-31"));
    l76s_done();
}

static void test_l76_sys_md_11(void) {
    we_form_req_t q;
    char html[400];
    sprintf(html, "<form action=/md11 method=GET>"
                  "<input id=md11 type=date name=md11 value=2021-11-30>"
                  "<input type=submit></form>");
    l76s_render(html);
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76s_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "md11=2021-11-30"));
    l76s_done();
}

static void test_l76_sys_md_12(void) {
    we_form_req_t q;
    char html[400];
    sprintf(html, "<form action=/md12 method=GET>"
                  "<input id=md12 type=date name=md12 value=2021-12-31>"
                  "<input type=submit></form>");
    l76s_render(html);
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l76s_submit());
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "md12=2021-12-31"));
    l76s_done();
}

static void test_l76_sys_badmd_1(void) {
    char html[400];
    sprintf(html, "<form action=/x method=GET>"
                  "<input id=bm1 type=date name=bm1 value=2021-02-29 required>"
                  "<input type=submit></form>");
    l76s_render(html);
    TEST_ASSERT_EQUAL(WE_FACT_CHANGED, l76s_submit());
    l76s_done();
}

static void test_l76_sys_badmd_2(void) {
    char html[400];
    sprintf(html, "<form action=/x method=GET>"
                  "<input id=bm2 type=date name=bm2 value=2024-02-30 required>"
                  "<input type=submit></form>");
    l76s_render(html);
    TEST_ASSERT_EQUAL(WE_FACT_CHANGED, l76s_submit());
    l76s_done();
}

static void test_l76_sys_badmd_3(void) {
    char html[400];
    sprintf(html, "<form action=/x method=GET>"
                  "<input id=bm3 type=date name=bm3 value=2020-04-31 required>"
                  "<input type=submit></form>");
    l76s_render(html);
    TEST_ASSERT_EQUAL(WE_FACT_CHANGED, l76s_submit());
    l76s_done();
}

static void test_l76_sys_badmd_4(void) {
    char html[400];
    sprintf(html, "<form action=/x method=GET>"
                  "<input id=bm4 type=date name=bm4 value=2020-06-31 required>"
                  "<input type=submit></form>");
    l76s_render(html);
    TEST_ASSERT_EQUAL(WE_FACT_CHANGED, l76s_submit());
    l76s_done();
}

static void test_l76_sys_badmd_5(void) {
    char html[400];
    sprintf(html, "<form action=/x method=GET>"
                  "<input id=bm5 type=date name=bm5 value=2020-09-31 required>"
                  "<input type=submit></form>");
    l76s_render(html);
    TEST_ASSERT_EQUAL(WE_FACT_CHANGED, l76s_submit());
    l76s_done();
}

static void test_l76_sys_badmd_6(void) {
    char html[400];
    sprintf(html, "<form action=/x method=GET>"
                  "<input id=bm6 type=date name=bm6 value=2020-11-31 required>"
                  "<input type=submit></form>");
    l76s_render(html);
    TEST_ASSERT_EQUAL(WE_FACT_CHANGED, l76s_submit());
    l76s_done();
}

static void test_l76_sys_badmd_7(void) {
    char html[400];
    sprintf(html, "<form action=/x method=GET>"
                  "<input id=bm7 type=date name=bm7 value=2100-02-29 required>"
                  "<input type=submit></form>");
    l76s_render(html);
    TEST_ASSERT_EQUAL(WE_FACT_CHANGED, l76s_submit());
    l76s_done();
}

static void test_l76_sys_badmd_8(void) {
    char html[400];
    sprintf(html, "<form action=/x method=GET>"
                  "<input id=bm8 type=date name=bm8 value=1900-02-29 required>"
                  "<input type=submit></form>");
    l76s_render(html);
    TEST_ASSERT_EQUAL(WE_FACT_CHANGED, l76s_submit());
    l76s_done();
}

static void test_l76_sys_badmd_9(void) {
    char html[400];
    sprintf(html, "<form action=/x method=GET>"
                  "<input id=bm9 type=date name=bm9 value=2020-01-32 required>"
                  "<input type=submit></form>");
    l76s_render(html);
    TEST_ASSERT_EQUAL(WE_FACT_CHANGED, l76s_submit());
    l76s_done();
}

static void test_l76_sys_badmd_10(void) {
    char html[400];
    sprintf(html, "<form action=/x method=GET>"
                  "<input id=bm10 type=date name=bm10 value=2020-00-01 required>"
                  "<input type=submit></form>");
    l76s_render(html);
    TEST_ASSERT_EQUAL(WE_FACT_CHANGED, l76s_submit());
    l76s_done();
}

static void test_l76_sys_badmd_11(void) {
    char html[400];
    sprintf(html, "<form action=/x method=GET>"
                  "<input id=bm11 type=date name=bm11 value=2020-13-01 required>"
                  "<input type=submit></form>");
    l76s_render(html);
    TEST_ASSERT_EQUAL(WE_FACT_CHANGED, l76s_submit());
    l76s_done();
}

static void test_l76_sys_badmd_12(void) {
    char html[400];
    sprintf(html, "<form action=/x method=GET>"
                  "<input id=bm12 type=date name=bm12 value=2020-12-32 required>"
                  "<input type=submit></form>");
    l76s_render(html);
    TEST_ASSERT_EQUAL(WE_FACT_CHANGED, l76s_submit());
    l76s_done();
}

static void lot76_sys_run(void) {
    RUN_TEST(test_l76_sys_1);
    RUN_TEST(test_l76_sys_2);
    RUN_TEST(test_l76_sys_3);
    RUN_TEST(test_l76_sys_4);
    RUN_TEST(test_l76_sys_5);
    RUN_TEST(test_l76_sys_6);
    RUN_TEST(test_l76_sys_7);
    RUN_TEST(test_l76_sys_8);
    RUN_TEST(test_l76_sys_9);
    RUN_TEST(test_l76_sys_10);
    RUN_TEST(test_l76_sys_11);
    RUN_TEST(test_l76_sys_12);
    RUN_TEST(test_l76_sys_13);
    RUN_TEST(test_l76_sys_14);
    RUN_TEST(test_l76_sys_15);
    RUN_TEST(test_l76_sys_16);
    RUN_TEST(test_l76_sys_17);
    RUN_TEST(test_l76_sys_18);
    RUN_TEST(test_l76_sys_19);
    RUN_TEST(test_l76_sys_20);
    RUN_TEST(test_l76_sys_21);
    RUN_TEST(test_l76_sys_22);
    RUN_TEST(test_l76_sys_23);
    RUN_TEST(test_l76_sys_24);
    RUN_TEST(test_l76_sys_25);
    RUN_TEST(test_l76_sys_26);
    RUN_TEST(test_l76_sys_27);
    RUN_TEST(test_l76_sys_28);
    RUN_TEST(test_l76_sys_29);
    RUN_TEST(test_l76_sys_30);
    RUN_TEST(test_l76_sys_31);
    RUN_TEST(test_l76_sys_32);
    RUN_TEST(test_l76_sys_33);
    RUN_TEST(test_l76_sys_34);
    RUN_TEST(test_l76_sys_35);
    RUN_TEST(test_l76_sys_36);
    RUN_TEST(test_l76_sys_37);
    RUN_TEST(test_l76_sys_38);
    RUN_TEST(test_l76_sys_39);
    RUN_TEST(test_l76_sys_40);
    RUN_TEST(test_l76_sys_41);
    RUN_TEST(test_l76_sys_42);
    RUN_TEST(test_l76_sys_43);
    RUN_TEST(test_l76_sys_44);
    RUN_TEST(test_l76_sys_45);
    RUN_TEST(test_l76_sys_46);
    RUN_TEST(test_l76_sys_47);
    RUN_TEST(test_l76_sys_48);
    RUN_TEST(test_l76_sys_49);
    RUN_TEST(test_l76_sys_50);
    RUN_TEST(test_l76_sys_51);
    RUN_TEST(test_l76_sys_52);
    RUN_TEST(test_l76_sys_53);
    RUN_TEST(test_l76_sys_54);
    RUN_TEST(test_l76_sys_55);
    RUN_TEST(test_l76_sys_56);
    RUN_TEST(test_l76_sys_57);
    RUN_TEST(test_l76_sys_58);
    RUN_TEST(test_l76_sys_59);
    RUN_TEST(test_l76_sys_60);
    RUN_TEST(test_l76_sys_61);
    RUN_TEST(test_l76_sys_62);
    RUN_TEST(test_l76_sys_63);
    RUN_TEST(test_l76_sys_64);
    RUN_TEST(test_l76_sys_65);
    RUN_TEST(test_l76_sys_66);
    RUN_TEST(test_l76_sys_67);
    RUN_TEST(test_l76_sys_68);
    RUN_TEST(test_l76_sys_69);
    RUN_TEST(test_l76_sys_70);
    RUN_TEST(test_l76_sys_71);
    RUN_TEST(test_l76_sys_72);
    RUN_TEST(test_l76_sys_73);
    RUN_TEST(test_l76_sys_74);
    RUN_TEST(test_l76_sys_75);
    RUN_TEST(test_l76_sys_76);
    RUN_TEST(test_l76_sys_77);
    RUN_TEST(test_l76_sys_78);
    RUN_TEST(test_l76_sys_79);
    RUN_TEST(test_l76_sys_80);
    RUN_TEST(test_l76_sys_81);
    RUN_TEST(test_l76_sys_82);
    RUN_TEST(test_l76_sys_83);
    RUN_TEST(test_l76_sys_84);
    RUN_TEST(test_l76_sys_85);
    RUN_TEST(test_l76_sys_86);
    RUN_TEST(test_l76_sys_87);
    RUN_TEST(test_l76_sys_88);
    RUN_TEST(test_l76_sys_89);
    RUN_TEST(test_l76_sys_90);
    RUN_TEST(test_l76_sys_91);
    RUN_TEST(test_l76_sys_92);
    RUN_TEST(test_l76_sys_93);
    RUN_TEST(test_l76_sys_94);
    RUN_TEST(test_l76_sys_95);
    RUN_TEST(test_l76_sys_96);
    RUN_TEST(test_l76_sys_97);
    RUN_TEST(test_l76_sys_98);
    RUN_TEST(test_l76_sys_99);
    RUN_TEST(test_l76_sys_100);
    RUN_TEST(test_l76_sys_101);
    RUN_TEST(test_l76_sys_102);
    RUN_TEST(test_l76_sys_103);
    RUN_TEST(test_l76_sys_104);
    RUN_TEST(test_l76_sys_105);
    RUN_TEST(test_l76_sys_106);
    RUN_TEST(test_l76_sys_107);
    RUN_TEST(test_l76_sys_108);
    RUN_TEST(test_l76_sys_109);
    RUN_TEST(test_l76_sys_110);
    RUN_TEST(test_l76_sys_111);
    RUN_TEST(test_l76_sys_112);
    RUN_TEST(test_l76_sys_113);
    RUN_TEST(test_l76_sys_114);
    RUN_TEST(test_l76_sys_115);
    RUN_TEST(test_l76_sys_116);
    RUN_TEST(test_l76_sys_117);
    RUN_TEST(test_l76_sys_118);
    RUN_TEST(test_l76_sys_119);
    RUN_TEST(test_l76_sys_120);
    RUN_TEST(test_l76_sys_md_1);
    RUN_TEST(test_l76_sys_md_2);
    RUN_TEST(test_l76_sys_md_3);
    RUN_TEST(test_l76_sys_md_4);
    RUN_TEST(test_l76_sys_md_5);
    RUN_TEST(test_l76_sys_md_6);
    RUN_TEST(test_l76_sys_md_7);
    RUN_TEST(test_l76_sys_md_8);
    RUN_TEST(test_l76_sys_md_9);
    RUN_TEST(test_l76_sys_md_10);
    RUN_TEST(test_l76_sys_md_11);
    RUN_TEST(test_l76_sys_md_12);
    RUN_TEST(test_l76_sys_badmd_1);
    RUN_TEST(test_l76_sys_badmd_2);
    RUN_TEST(test_l76_sys_badmd_3);
    RUN_TEST(test_l76_sys_badmd_4);
    RUN_TEST(test_l76_sys_badmd_5);
    RUN_TEST(test_l76_sys_badmd_6);
    RUN_TEST(test_l76_sys_badmd_7);
    RUN_TEST(test_l76_sys_badmd_8);
    RUN_TEST(test_l76_sys_badmd_9);
    RUN_TEST(test_l76_sys_badmd_10);
    RUN_TEST(test_l76_sys_badmd_11);
    RUN_TEST(test_l76_sys_badmd_12);
}
