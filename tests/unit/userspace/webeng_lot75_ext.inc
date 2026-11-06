#ifndef WE_FF2_FORM_ATTR
#define WE_FF2_FORM_ATTR 131072u
#define WE_FF2_FORMACTION 262144u
#endif

static void test_l75_ext_form_1(void) {
    we_form_req_t q;
    char html[700], sid[16];
    int k, x, y;
    sprintf(sid, "s1");
    sprintf(html,
        "<form id=f1 action=/e1 method=%s>"
        "<input name=n1 value=v1 form=f1>"
        "<input id=s1 type=submit formaction=/alt1 value=Go></form>",
        (1 % 2) ? "POST" : "GET");
    l75b_render(html);
    TEST_ASSERT_TRUE(g_doc.nodes[l75b_node(sid)].form_flags2 & WE_FF2_FORMACTION);
    for (k = 1; k < (int)g_doc.nnodes; k++)
        if (g_doc.nodes[k].form_type == WE_FORM_SUBMIT) break;
    x = g_doc.nodes[k].x + 2; y = g_doc.nodes[k].y + 2;
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, we_form_click(&g_doc, x, y));
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "/alt1"));
    if (1 % 2) { TEST_ASSERT_EQUAL(1, q.post); }
    else { TEST_ASSERT_EQUAL(0, q.post); }
    l75b_done();
}

static void test_l75_ext_form_2(void) {
    we_form_req_t q;
    char html[700], sid[16];
    int k, x, y;
    sprintf(sid, "s2");
    sprintf(html,
        "<form id=f2 action=/e2 method=%s>"
        "<input name=n2 value=v2 form=f2>"
        "<input id=s2 type=submit formaction=/alt2 value=Go></form>",
        (2 % 2) ? "POST" : "GET");
    l75b_render(html);
    TEST_ASSERT_TRUE(g_doc.nodes[l75b_node(sid)].form_flags2 & WE_FF2_FORMACTION);
    for (k = 1; k < (int)g_doc.nnodes; k++)
        if (g_doc.nodes[k].form_type == WE_FORM_SUBMIT) break;
    x = g_doc.nodes[k].x + 2; y = g_doc.nodes[k].y + 2;
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, we_form_click(&g_doc, x, y));
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "/alt2"));
    if (2 % 2) { TEST_ASSERT_EQUAL(1, q.post); }
    else { TEST_ASSERT_EQUAL(0, q.post); }
    l75b_done();
}

static void test_l75_ext_form_3(void) {
    we_form_req_t q;
    char html[700], sid[16];
    int k, x, y;
    sprintf(sid, "s3");
    sprintf(html,
        "<form id=f3 action=/e3 method=%s>"
        "<input name=n3 value=v3 form=f3>"
        "<input id=s3 type=submit formaction=/alt3 value=Go></form>",
        (3 % 2) ? "POST" : "GET");
    l75b_render(html);
    TEST_ASSERT_TRUE(g_doc.nodes[l75b_node(sid)].form_flags2 & WE_FF2_FORMACTION);
    for (k = 1; k < (int)g_doc.nnodes; k++)
        if (g_doc.nodes[k].form_type == WE_FORM_SUBMIT) break;
    x = g_doc.nodes[k].x + 2; y = g_doc.nodes[k].y + 2;
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, we_form_click(&g_doc, x, y));
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "/alt3"));
    if (3 % 2) { TEST_ASSERT_EQUAL(1, q.post); }
    else { TEST_ASSERT_EQUAL(0, q.post); }
    l75b_done();
}

static void test_l75_ext_form_4(void) {
    we_form_req_t q;
    char html[700], sid[16];
    int k, x, y;
    sprintf(sid, "s4");
    sprintf(html,
        "<form id=f4 action=/e4 method=%s>"
        "<input name=n4 value=v4 form=f4>"
        "<input id=s4 type=submit formaction=/alt4 value=Go></form>",
        (4 % 2) ? "POST" : "GET");
    l75b_render(html);
    TEST_ASSERT_TRUE(g_doc.nodes[l75b_node(sid)].form_flags2 & WE_FF2_FORMACTION);
    for (k = 1; k < (int)g_doc.nnodes; k++)
        if (g_doc.nodes[k].form_type == WE_FORM_SUBMIT) break;
    x = g_doc.nodes[k].x + 2; y = g_doc.nodes[k].y + 2;
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, we_form_click(&g_doc, x, y));
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "/alt4"));
    if (4 % 2) { TEST_ASSERT_EQUAL(1, q.post); }
    else { TEST_ASSERT_EQUAL(0, q.post); }
    l75b_done();
}

static void test_l75_ext_form_5(void) {
    we_form_req_t q;
    char html[700], sid[16];
    int k, x, y;
    sprintf(sid, "s5");
    sprintf(html,
        "<form id=f5 action=/e5 method=%s>"
        "<input name=n5 value=v5 form=f5>"
        "<input id=s5 type=submit formaction=/alt5 value=Go></form>",
        (5 % 2) ? "POST" : "GET");
    l75b_render(html);
    TEST_ASSERT_TRUE(g_doc.nodes[l75b_node(sid)].form_flags2 & WE_FF2_FORMACTION);
    for (k = 1; k < (int)g_doc.nnodes; k++)
        if (g_doc.nodes[k].form_type == WE_FORM_SUBMIT) break;
    x = g_doc.nodes[k].x + 2; y = g_doc.nodes[k].y + 2;
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, we_form_click(&g_doc, x, y));
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "/alt5"));
    if (5 % 2) { TEST_ASSERT_EQUAL(1, q.post); }
    else { TEST_ASSERT_EQUAL(0, q.post); }
    l75b_done();
}

static void test_l75_ext_form_6(void) {
    we_form_req_t q;
    char html[700], sid[16];
    int k, x, y;
    sprintf(sid, "s6");
    sprintf(html,
        "<form id=f6 action=/e6 method=%s>"
        "<input name=n6 value=v6 form=f6>"
        "<input id=s6 type=submit formaction=/alt6 value=Go></form>",
        (6 % 2) ? "POST" : "GET");
    l75b_render(html);
    TEST_ASSERT_TRUE(g_doc.nodes[l75b_node(sid)].form_flags2 & WE_FF2_FORMACTION);
    for (k = 1; k < (int)g_doc.nnodes; k++)
        if (g_doc.nodes[k].form_type == WE_FORM_SUBMIT) break;
    x = g_doc.nodes[k].x + 2; y = g_doc.nodes[k].y + 2;
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, we_form_click(&g_doc, x, y));
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "/alt6"));
    if (6 % 2) { TEST_ASSERT_EQUAL(1, q.post); }
    else { TEST_ASSERT_EQUAL(0, q.post); }
    l75b_done();
}

static void test_l75_ext_form_7(void) {
    we_form_req_t q;
    char html[700], sid[16];
    int k, x, y;
    sprintf(sid, "s7");
    sprintf(html,
        "<form id=f7 action=/e7 method=%s>"
        "<input name=n7 value=v7 form=f7>"
        "<input id=s7 type=submit formaction=/alt7 value=Go></form>",
        (7 % 2) ? "POST" : "GET");
    l75b_render(html);
    TEST_ASSERT_TRUE(g_doc.nodes[l75b_node(sid)].form_flags2 & WE_FF2_FORMACTION);
    for (k = 1; k < (int)g_doc.nnodes; k++)
        if (g_doc.nodes[k].form_type == WE_FORM_SUBMIT) break;
    x = g_doc.nodes[k].x + 2; y = g_doc.nodes[k].y + 2;
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, we_form_click(&g_doc, x, y));
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "/alt7"));
    if (7 % 2) { TEST_ASSERT_EQUAL(1, q.post); }
    else { TEST_ASSERT_EQUAL(0, q.post); }
    l75b_done();
}

static void test_l75_ext_form_8(void) {
    we_form_req_t q;
    char html[700], sid[16];
    int k, x, y;
    sprintf(sid, "s8");
    sprintf(html,
        "<form id=f8 action=/e8 method=%s>"
        "<input name=n8 value=v8 form=f8>"
        "<input id=s8 type=submit formaction=/alt8 value=Go></form>",
        (8 % 2) ? "POST" : "GET");
    l75b_render(html);
    TEST_ASSERT_TRUE(g_doc.nodes[l75b_node(sid)].form_flags2 & WE_FF2_FORMACTION);
    for (k = 1; k < (int)g_doc.nnodes; k++)
        if (g_doc.nodes[k].form_type == WE_FORM_SUBMIT) break;
    x = g_doc.nodes[k].x + 2; y = g_doc.nodes[k].y + 2;
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, we_form_click(&g_doc, x, y));
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "/alt8"));
    if (8 % 2) { TEST_ASSERT_EQUAL(1, q.post); }
    else { TEST_ASSERT_EQUAL(0, q.post); }
    l75b_done();
}

static void test_l75_ext_form_9(void) {
    we_form_req_t q;
    char html[700], sid[16];
    int k, x, y;
    sprintf(sid, "s9");
    sprintf(html,
        "<form id=f9 action=/e9 method=%s>"
        "<input name=n9 value=v9 form=f9>"
        "<input id=s9 type=submit formaction=/alt9 value=Go></form>",
        (9 % 2) ? "POST" : "GET");
    l75b_render(html);
    TEST_ASSERT_TRUE(g_doc.nodes[l75b_node(sid)].form_flags2 & WE_FF2_FORMACTION);
    for (k = 1; k < (int)g_doc.nnodes; k++)
        if (g_doc.nodes[k].form_type == WE_FORM_SUBMIT) break;
    x = g_doc.nodes[k].x + 2; y = g_doc.nodes[k].y + 2;
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, we_form_click(&g_doc, x, y));
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "/alt9"));
    if (9 % 2) { TEST_ASSERT_EQUAL(1, q.post); }
    else { TEST_ASSERT_EQUAL(0, q.post); }
    l75b_done();
}

static void test_l75_ext_form_10(void) {
    we_form_req_t q;
    char html[700], sid[16];
    int k, x, y;
    sprintf(sid, "s10");
    sprintf(html,
        "<form id=f10 action=/e10 method=%s>"
        "<input name=n10 value=v10 form=f10>"
        "<input id=s10 type=submit formaction=/alt10 value=Go></form>",
        (10 % 2) ? "POST" : "GET");
    l75b_render(html);
    TEST_ASSERT_TRUE(g_doc.nodes[l75b_node(sid)].form_flags2 & WE_FF2_FORMACTION);
    for (k = 1; k < (int)g_doc.nnodes; k++)
        if (g_doc.nodes[k].form_type == WE_FORM_SUBMIT) break;
    x = g_doc.nodes[k].x + 2; y = g_doc.nodes[k].y + 2;
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, we_form_click(&g_doc, x, y));
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "/alt10"));
    if (10 % 2) { TEST_ASSERT_EQUAL(1, q.post); }
    else { TEST_ASSERT_EQUAL(0, q.post); }
    l75b_done();
}

static void test_l75_ext_form_11(void) {
    we_form_req_t q;
    char html[700], sid[16];
    int k, x, y;
    sprintf(sid, "s11");
    sprintf(html,
        "<form id=f11 action=/e11 method=%s>"
        "<input name=n11 value=v11 form=f11>"
        "<input id=s11 type=submit formaction=/alt11 value=Go></form>",
        (11 % 2) ? "POST" : "GET");
    l75b_render(html);
    TEST_ASSERT_TRUE(g_doc.nodes[l75b_node(sid)].form_flags2 & WE_FF2_FORMACTION);
    for (k = 1; k < (int)g_doc.nnodes; k++)
        if (g_doc.nodes[k].form_type == WE_FORM_SUBMIT) break;
    x = g_doc.nodes[k].x + 2; y = g_doc.nodes[k].y + 2;
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, we_form_click(&g_doc, x, y));
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "/alt11"));
    if (11 % 2) { TEST_ASSERT_EQUAL(1, q.post); }
    else { TEST_ASSERT_EQUAL(0, q.post); }
    l75b_done();
}

static void test_l75_ext_form_12(void) {
    we_form_req_t q;
    char html[700], sid[16];
    int k, x, y;
    sprintf(sid, "s12");
    sprintf(html,
        "<form id=f12 action=/e12 method=%s>"
        "<input name=n12 value=v12 form=f12>"
        "<input id=s12 type=submit formaction=/alt12 value=Go></form>",
        (12 % 2) ? "POST" : "GET");
    l75b_render(html);
    TEST_ASSERT_TRUE(g_doc.nodes[l75b_node(sid)].form_flags2 & WE_FF2_FORMACTION);
    for (k = 1; k < (int)g_doc.nnodes; k++)
        if (g_doc.nodes[k].form_type == WE_FORM_SUBMIT) break;
    x = g_doc.nodes[k].x + 2; y = g_doc.nodes[k].y + 2;
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, we_form_click(&g_doc, x, y));
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "/alt12"));
    if (12 % 2) { TEST_ASSERT_EQUAL(1, q.post); }
    else { TEST_ASSERT_EQUAL(0, q.post); }
    l75b_done();
}

static void test_l75_ext_form_13(void) {
    we_form_req_t q;
    char html[700], sid[16];
    int k, x, y;
    sprintf(sid, "s13");
    sprintf(html,
        "<form id=f13 action=/e13 method=%s>"
        "<input name=n13 value=v13 form=f13>"
        "<input id=s13 type=submit formaction=/alt13 value=Go></form>",
        (13 % 2) ? "POST" : "GET");
    l75b_render(html);
    TEST_ASSERT_TRUE(g_doc.nodes[l75b_node(sid)].form_flags2 & WE_FF2_FORMACTION);
    for (k = 1; k < (int)g_doc.nnodes; k++)
        if (g_doc.nodes[k].form_type == WE_FORM_SUBMIT) break;
    x = g_doc.nodes[k].x + 2; y = g_doc.nodes[k].y + 2;
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, we_form_click(&g_doc, x, y));
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "/alt13"));
    if (13 % 2) { TEST_ASSERT_EQUAL(1, q.post); }
    else { TEST_ASSERT_EQUAL(0, q.post); }
    l75b_done();
}

static void test_l75_ext_form_14(void) {
    we_form_req_t q;
    char html[700], sid[16];
    int k, x, y;
    sprintf(sid, "s14");
    sprintf(html,
        "<form id=f14 action=/e14 method=%s>"
        "<input name=n14 value=v14 form=f14>"
        "<input id=s14 type=submit formaction=/alt14 value=Go></form>",
        (14 % 2) ? "POST" : "GET");
    l75b_render(html);
    TEST_ASSERT_TRUE(g_doc.nodes[l75b_node(sid)].form_flags2 & WE_FF2_FORMACTION);
    for (k = 1; k < (int)g_doc.nnodes; k++)
        if (g_doc.nodes[k].form_type == WE_FORM_SUBMIT) break;
    x = g_doc.nodes[k].x + 2; y = g_doc.nodes[k].y + 2;
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, we_form_click(&g_doc, x, y));
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "/alt14"));
    if (14 % 2) { TEST_ASSERT_EQUAL(1, q.post); }
    else { TEST_ASSERT_EQUAL(0, q.post); }
    l75b_done();
}

static void test_l75_ext_form_15(void) {
    we_form_req_t q;
    char html[700], sid[16];
    int k, x, y;
    sprintf(sid, "s15");
    sprintf(html,
        "<form id=f15 action=/e15 method=%s>"
        "<input name=n15 value=v15 form=f15>"
        "<input id=s15 type=submit formaction=/alt15 value=Go></form>",
        (15 % 2) ? "POST" : "GET");
    l75b_render(html);
    TEST_ASSERT_TRUE(g_doc.nodes[l75b_node(sid)].form_flags2 & WE_FF2_FORMACTION);
    for (k = 1; k < (int)g_doc.nnodes; k++)
        if (g_doc.nodes[k].form_type == WE_FORM_SUBMIT) break;
    x = g_doc.nodes[k].x + 2; y = g_doc.nodes[k].y + 2;
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, we_form_click(&g_doc, x, y));
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "/alt15"));
    if (15 % 2) { TEST_ASSERT_EQUAL(1, q.post); }
    else { TEST_ASSERT_EQUAL(0, q.post); }
    l75b_done();
}

static void test_l75_ext_form_16(void) {
    we_form_req_t q;
    char html[700], sid[16];
    int k, x, y;
    sprintf(sid, "s16");
    sprintf(html,
        "<form id=f16 action=/e16 method=%s>"
        "<input name=n16 value=v16 form=f16>"
        "<input id=s16 type=submit formaction=/alt16 value=Go></form>",
        (16 % 2) ? "POST" : "GET");
    l75b_render(html);
    TEST_ASSERT_TRUE(g_doc.nodes[l75b_node(sid)].form_flags2 & WE_FF2_FORMACTION);
    for (k = 1; k < (int)g_doc.nnodes; k++)
        if (g_doc.nodes[k].form_type == WE_FORM_SUBMIT) break;
    x = g_doc.nodes[k].x + 2; y = g_doc.nodes[k].y + 2;
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, we_form_click(&g_doc, x, y));
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "/alt16"));
    if (16 % 2) { TEST_ASSERT_EQUAL(1, q.post); }
    else { TEST_ASSERT_EQUAL(0, q.post); }
    l75b_done();
}

static void test_l75_ext_form_17(void) {
    we_form_req_t q;
    char html[700], sid[16];
    int k, x, y;
    sprintf(sid, "s17");
    sprintf(html,
        "<form id=f17 action=/e17 method=%s>"
        "<input name=n17 value=v17 form=f17>"
        "<input id=s17 type=submit formaction=/alt17 value=Go></form>",
        (17 % 2) ? "POST" : "GET");
    l75b_render(html);
    TEST_ASSERT_TRUE(g_doc.nodes[l75b_node(sid)].form_flags2 & WE_FF2_FORMACTION);
    for (k = 1; k < (int)g_doc.nnodes; k++)
        if (g_doc.nodes[k].form_type == WE_FORM_SUBMIT) break;
    x = g_doc.nodes[k].x + 2; y = g_doc.nodes[k].y + 2;
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, we_form_click(&g_doc, x, y));
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "/alt17"));
    if (17 % 2) { TEST_ASSERT_EQUAL(1, q.post); }
    else { TEST_ASSERT_EQUAL(0, q.post); }
    l75b_done();
}

static void test_l75_ext_form_18(void) {
    we_form_req_t q;
    char html[700], sid[16];
    int k, x, y;
    sprintf(sid, "s18");
    sprintf(html,
        "<form id=f18 action=/e18 method=%s>"
        "<input name=n18 value=v18 form=f18>"
        "<input id=s18 type=submit formaction=/alt18 value=Go></form>",
        (18 % 2) ? "POST" : "GET");
    l75b_render(html);
    TEST_ASSERT_TRUE(g_doc.nodes[l75b_node(sid)].form_flags2 & WE_FF2_FORMACTION);
    for (k = 1; k < (int)g_doc.nnodes; k++)
        if (g_doc.nodes[k].form_type == WE_FORM_SUBMIT) break;
    x = g_doc.nodes[k].x + 2; y = g_doc.nodes[k].y + 2;
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, we_form_click(&g_doc, x, y));
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "/alt18"));
    if (18 % 2) { TEST_ASSERT_EQUAL(1, q.post); }
    else { TEST_ASSERT_EQUAL(0, q.post); }
    l75b_done();
}

static void test_l75_ext_form_19(void) {
    we_form_req_t q;
    char html[700], sid[16];
    int k, x, y;
    sprintf(sid, "s19");
    sprintf(html,
        "<form id=f19 action=/e19 method=%s>"
        "<input name=n19 value=v19 form=f19>"
        "<input id=s19 type=submit formaction=/alt19 value=Go></form>",
        (19 % 2) ? "POST" : "GET");
    l75b_render(html);
    TEST_ASSERT_TRUE(g_doc.nodes[l75b_node(sid)].form_flags2 & WE_FF2_FORMACTION);
    for (k = 1; k < (int)g_doc.nnodes; k++)
        if (g_doc.nodes[k].form_type == WE_FORM_SUBMIT) break;
    x = g_doc.nodes[k].x + 2; y = g_doc.nodes[k].y + 2;
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, we_form_click(&g_doc, x, y));
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "/alt19"));
    if (19 % 2) { TEST_ASSERT_EQUAL(1, q.post); }
    else { TEST_ASSERT_EQUAL(0, q.post); }
    l75b_done();
}

static void test_l75_ext_form_20(void) {
    we_form_req_t q;
    char html[700], sid[16];
    int k, x, y;
    sprintf(sid, "s20");
    sprintf(html,
        "<form id=f20 action=/e20 method=%s>"
        "<input name=n20 value=v20 form=f20>"
        "<input id=s20 type=submit formaction=/alt20 value=Go></form>",
        (20 % 2) ? "POST" : "GET");
    l75b_render(html);
    TEST_ASSERT_TRUE(g_doc.nodes[l75b_node(sid)].form_flags2 & WE_FF2_FORMACTION);
    for (k = 1; k < (int)g_doc.nnodes; k++)
        if (g_doc.nodes[k].form_type == WE_FORM_SUBMIT) break;
    x = g_doc.nodes[k].x + 2; y = g_doc.nodes[k].y + 2;
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, we_form_click(&g_doc, x, y));
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "/alt20"));
    if (20 % 2) { TEST_ASSERT_EQUAL(1, q.post); }
    else { TEST_ASSERT_EQUAL(0, q.post); }
    l75b_done();
}

static void test_l75_ext_form_21(void) {
    we_form_req_t q;
    char html[700], sid[16];
    int k, x, y;
    sprintf(sid, "s21");
    sprintf(html,
        "<form id=f21 action=/e21 method=%s>"
        "<input name=n21 value=v21 form=f21>"
        "<input id=s21 type=submit formaction=/alt21 value=Go></form>",
        (21 % 2) ? "POST" : "GET");
    l75b_render(html);
    TEST_ASSERT_TRUE(g_doc.nodes[l75b_node(sid)].form_flags2 & WE_FF2_FORMACTION);
    for (k = 1; k < (int)g_doc.nnodes; k++)
        if (g_doc.nodes[k].form_type == WE_FORM_SUBMIT) break;
    x = g_doc.nodes[k].x + 2; y = g_doc.nodes[k].y + 2;
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, we_form_click(&g_doc, x, y));
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "/alt21"));
    if (21 % 2) { TEST_ASSERT_EQUAL(1, q.post); }
    else { TEST_ASSERT_EQUAL(0, q.post); }
    l75b_done();
}

static void test_l75_ext_form_22(void) {
    we_form_req_t q;
    char html[700], sid[16];
    int k, x, y;
    sprintf(sid, "s22");
    sprintf(html,
        "<form id=f22 action=/e22 method=%s>"
        "<input name=n22 value=v22 form=f22>"
        "<input id=s22 type=submit formaction=/alt22 value=Go></form>",
        (22 % 2) ? "POST" : "GET");
    l75b_render(html);
    TEST_ASSERT_TRUE(g_doc.nodes[l75b_node(sid)].form_flags2 & WE_FF2_FORMACTION);
    for (k = 1; k < (int)g_doc.nnodes; k++)
        if (g_doc.nodes[k].form_type == WE_FORM_SUBMIT) break;
    x = g_doc.nodes[k].x + 2; y = g_doc.nodes[k].y + 2;
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, we_form_click(&g_doc, x, y));
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "/alt22"));
    if (22 % 2) { TEST_ASSERT_EQUAL(1, q.post); }
    else { TEST_ASSERT_EQUAL(0, q.post); }
    l75b_done();
}

static void test_l75_ext_form_23(void) {
    we_form_req_t q;
    char html[700], sid[16];
    int k, x, y;
    sprintf(sid, "s23");
    sprintf(html,
        "<form id=f23 action=/e23 method=%s>"
        "<input name=n23 value=v23 form=f23>"
        "<input id=s23 type=submit formaction=/alt23 value=Go></form>",
        (23 % 2) ? "POST" : "GET");
    l75b_render(html);
    TEST_ASSERT_TRUE(g_doc.nodes[l75b_node(sid)].form_flags2 & WE_FF2_FORMACTION);
    for (k = 1; k < (int)g_doc.nnodes; k++)
        if (g_doc.nodes[k].form_type == WE_FORM_SUBMIT) break;
    x = g_doc.nodes[k].x + 2; y = g_doc.nodes[k].y + 2;
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, we_form_click(&g_doc, x, y));
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "/alt23"));
    if (23 % 2) { TEST_ASSERT_EQUAL(1, q.post); }
    else { TEST_ASSERT_EQUAL(0, q.post); }
    l75b_done();
}

static void test_l75_ext_form_24(void) {
    we_form_req_t q;
    char html[700], sid[16];
    int k, x, y;
    sprintf(sid, "s24");
    sprintf(html,
        "<form id=f24 action=/e24 method=%s>"
        "<input name=n24 value=v24 form=f24>"
        "<input id=s24 type=submit formaction=/alt24 value=Go></form>",
        (24 % 2) ? "POST" : "GET");
    l75b_render(html);
    TEST_ASSERT_TRUE(g_doc.nodes[l75b_node(sid)].form_flags2 & WE_FF2_FORMACTION);
    for (k = 1; k < (int)g_doc.nnodes; k++)
        if (g_doc.nodes[k].form_type == WE_FORM_SUBMIT) break;
    x = g_doc.nodes[k].x + 2; y = g_doc.nodes[k].y + 2;
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, we_form_click(&g_doc, x, y));
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "/alt24"));
    if (24 % 2) { TEST_ASSERT_EQUAL(1, q.post); }
    else { TEST_ASSERT_EQUAL(0, q.post); }
    l75b_done();
}

static void test_l75_ext_form_25(void) {
    we_form_req_t q;
    char html[700], sid[16];
    int k, x, y;
    sprintf(sid, "s25");
    sprintf(html,
        "<form id=f25 action=/e25 method=%s>"
        "<input name=n25 value=v25 form=f25>"
        "<input id=s25 type=submit formaction=/alt25 value=Go></form>",
        (25 % 2) ? "POST" : "GET");
    l75b_render(html);
    TEST_ASSERT_TRUE(g_doc.nodes[l75b_node(sid)].form_flags2 & WE_FF2_FORMACTION);
    for (k = 1; k < (int)g_doc.nnodes; k++)
        if (g_doc.nodes[k].form_type == WE_FORM_SUBMIT) break;
    x = g_doc.nodes[k].x + 2; y = g_doc.nodes[k].y + 2;
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, we_form_click(&g_doc, x, y));
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "/alt25"));
    if (25 % 2) { TEST_ASSERT_EQUAL(1, q.post); }
    else { TEST_ASSERT_EQUAL(0, q.post); }
    l75b_done();
}

static void test_l75_ext_form_26(void) {
    we_form_req_t q;
    char html[700], sid[16];
    int k, x, y;
    sprintf(sid, "s26");
    sprintf(html,
        "<form id=f26 action=/e26 method=%s>"
        "<input name=n26 value=v26 form=f26>"
        "<input id=s26 type=submit formaction=/alt26 value=Go></form>",
        (26 % 2) ? "POST" : "GET");
    l75b_render(html);
    TEST_ASSERT_TRUE(g_doc.nodes[l75b_node(sid)].form_flags2 & WE_FF2_FORMACTION);
    for (k = 1; k < (int)g_doc.nnodes; k++)
        if (g_doc.nodes[k].form_type == WE_FORM_SUBMIT) break;
    x = g_doc.nodes[k].x + 2; y = g_doc.nodes[k].y + 2;
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, we_form_click(&g_doc, x, y));
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "/alt26"));
    if (26 % 2) { TEST_ASSERT_EQUAL(1, q.post); }
    else { TEST_ASSERT_EQUAL(0, q.post); }
    l75b_done();
}

static void test_l75_ext_form_27(void) {
    we_form_req_t q;
    char html[700], sid[16];
    int k, x, y;
    sprintf(sid, "s27");
    sprintf(html,
        "<form id=f27 action=/e27 method=%s>"
        "<input name=n27 value=v27 form=f27>"
        "<input id=s27 type=submit formaction=/alt27 value=Go></form>",
        (27 % 2) ? "POST" : "GET");
    l75b_render(html);
    TEST_ASSERT_TRUE(g_doc.nodes[l75b_node(sid)].form_flags2 & WE_FF2_FORMACTION);
    for (k = 1; k < (int)g_doc.nnodes; k++)
        if (g_doc.nodes[k].form_type == WE_FORM_SUBMIT) break;
    x = g_doc.nodes[k].x + 2; y = g_doc.nodes[k].y + 2;
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, we_form_click(&g_doc, x, y));
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "/alt27"));
    if (27 % 2) { TEST_ASSERT_EQUAL(1, q.post); }
    else { TEST_ASSERT_EQUAL(0, q.post); }
    l75b_done();
}

static void test_l75_ext_form_28(void) {
    we_form_req_t q;
    char html[700], sid[16];
    int k, x, y;
    sprintf(sid, "s28");
    sprintf(html,
        "<form id=f28 action=/e28 method=%s>"
        "<input name=n28 value=v28 form=f28>"
        "<input id=s28 type=submit formaction=/alt28 value=Go></form>",
        (28 % 2) ? "POST" : "GET");
    l75b_render(html);
    TEST_ASSERT_TRUE(g_doc.nodes[l75b_node(sid)].form_flags2 & WE_FF2_FORMACTION);
    for (k = 1; k < (int)g_doc.nnodes; k++)
        if (g_doc.nodes[k].form_type == WE_FORM_SUBMIT) break;
    x = g_doc.nodes[k].x + 2; y = g_doc.nodes[k].y + 2;
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, we_form_click(&g_doc, x, y));
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "/alt28"));
    if (28 % 2) { TEST_ASSERT_EQUAL(1, q.post); }
    else { TEST_ASSERT_EQUAL(0, q.post); }
    l75b_done();
}

static void test_l75_ext_form_29(void) {
    we_form_req_t q;
    char html[700], sid[16];
    int k, x, y;
    sprintf(sid, "s29");
    sprintf(html,
        "<form id=f29 action=/e29 method=%s>"
        "<input name=n29 value=v29 form=f29>"
        "<input id=s29 type=submit formaction=/alt29 value=Go></form>",
        (29 % 2) ? "POST" : "GET");
    l75b_render(html);
    TEST_ASSERT_TRUE(g_doc.nodes[l75b_node(sid)].form_flags2 & WE_FF2_FORMACTION);
    for (k = 1; k < (int)g_doc.nnodes; k++)
        if (g_doc.nodes[k].form_type == WE_FORM_SUBMIT) break;
    x = g_doc.nodes[k].x + 2; y = g_doc.nodes[k].y + 2;
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, we_form_click(&g_doc, x, y));
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "/alt29"));
    if (29 % 2) { TEST_ASSERT_EQUAL(1, q.post); }
    else { TEST_ASSERT_EQUAL(0, q.post); }
    l75b_done();
}

static void test_l75_ext_form_30(void) {
    we_form_req_t q;
    char html[700], sid[16];
    int k, x, y;
    sprintf(sid, "s30");
    sprintf(html,
        "<form id=f30 action=/e30 method=%s>"
        "<input name=n30 value=v30 form=f30>"
        "<input id=s30 type=submit formaction=/alt30 value=Go></form>",
        (30 % 2) ? "POST" : "GET");
    l75b_render(html);
    TEST_ASSERT_TRUE(g_doc.nodes[l75b_node(sid)].form_flags2 & WE_FF2_FORMACTION);
    for (k = 1; k < (int)g_doc.nnodes; k++)
        if (g_doc.nodes[k].form_type == WE_FORM_SUBMIT) break;
    x = g_doc.nodes[k].x + 2; y = g_doc.nodes[k].y + 2;
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, we_form_click(&g_doc, x, y));
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "/alt30"));
    if (30 % 2) { TEST_ASSERT_EQUAL(1, q.post); }
    else { TEST_ASSERT_EQUAL(0, q.post); }
    l75b_done();
}

static void test_l75_ext_form_31(void) {
    we_form_req_t q;
    char html[700], sid[16];
    int k, x, y;
    sprintf(sid, "s31");
    sprintf(html,
        "<form id=f31 action=/e31 method=%s>"
        "<input name=n31 value=v31 form=f31>"
        "<input id=s31 type=submit formaction=/alt31 value=Go></form>",
        (31 % 2) ? "POST" : "GET");
    l75b_render(html);
    TEST_ASSERT_TRUE(g_doc.nodes[l75b_node(sid)].form_flags2 & WE_FF2_FORMACTION);
    for (k = 1; k < (int)g_doc.nnodes; k++)
        if (g_doc.nodes[k].form_type == WE_FORM_SUBMIT) break;
    x = g_doc.nodes[k].x + 2; y = g_doc.nodes[k].y + 2;
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, we_form_click(&g_doc, x, y));
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "/alt31"));
    if (31 % 2) { TEST_ASSERT_EQUAL(1, q.post); }
    else { TEST_ASSERT_EQUAL(0, q.post); }
    l75b_done();
}

static void test_l75_ext_form_32(void) {
    we_form_req_t q;
    char html[700], sid[16];
    int k, x, y;
    sprintf(sid, "s32");
    sprintf(html,
        "<form id=f32 action=/e32 method=%s>"
        "<input name=n32 value=v32 form=f32>"
        "<input id=s32 type=submit formaction=/alt32 value=Go></form>",
        (32 % 2) ? "POST" : "GET");
    l75b_render(html);
    TEST_ASSERT_TRUE(g_doc.nodes[l75b_node(sid)].form_flags2 & WE_FF2_FORMACTION);
    for (k = 1; k < (int)g_doc.nnodes; k++)
        if (g_doc.nodes[k].form_type == WE_FORM_SUBMIT) break;
    x = g_doc.nodes[k].x + 2; y = g_doc.nodes[k].y + 2;
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, we_form_click(&g_doc, x, y));
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "/alt32"));
    if (32 % 2) { TEST_ASSERT_EQUAL(1, q.post); }
    else { TEST_ASSERT_EQUAL(0, q.post); }
    l75b_done();
}

static void test_l75_ext_form_33(void) {
    we_form_req_t q;
    char html[700], sid[16];
    int k, x, y;
    sprintf(sid, "s33");
    sprintf(html,
        "<form id=f33 action=/e33 method=%s>"
        "<input name=n33 value=v33 form=f33>"
        "<input id=s33 type=submit formaction=/alt33 value=Go></form>",
        (33 % 2) ? "POST" : "GET");
    l75b_render(html);
    TEST_ASSERT_TRUE(g_doc.nodes[l75b_node(sid)].form_flags2 & WE_FF2_FORMACTION);
    for (k = 1; k < (int)g_doc.nnodes; k++)
        if (g_doc.nodes[k].form_type == WE_FORM_SUBMIT) break;
    x = g_doc.nodes[k].x + 2; y = g_doc.nodes[k].y + 2;
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, we_form_click(&g_doc, x, y));
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "/alt33"));
    if (33 % 2) { TEST_ASSERT_EQUAL(1, q.post); }
    else { TEST_ASSERT_EQUAL(0, q.post); }
    l75b_done();
}

static void test_l75_ext_form_34(void) {
    we_form_req_t q;
    char html[700], sid[16];
    int k, x, y;
    sprintf(sid, "s34");
    sprintf(html,
        "<form id=f34 action=/e34 method=%s>"
        "<input name=n34 value=v34 form=f34>"
        "<input id=s34 type=submit formaction=/alt34 value=Go></form>",
        (34 % 2) ? "POST" : "GET");
    l75b_render(html);
    TEST_ASSERT_TRUE(g_doc.nodes[l75b_node(sid)].form_flags2 & WE_FF2_FORMACTION);
    for (k = 1; k < (int)g_doc.nnodes; k++)
        if (g_doc.nodes[k].form_type == WE_FORM_SUBMIT) break;
    x = g_doc.nodes[k].x + 2; y = g_doc.nodes[k].y + 2;
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, we_form_click(&g_doc, x, y));
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "/alt34"));
    if (34 % 2) { TEST_ASSERT_EQUAL(1, q.post); }
    else { TEST_ASSERT_EQUAL(0, q.post); }
    l75b_done();
}

static void test_l75_ext_form_35(void) {
    we_form_req_t q;
    char html[700], sid[16];
    int k, x, y;
    sprintf(sid, "s35");
    sprintf(html,
        "<form id=f35 action=/e35 method=%s>"
        "<input name=n35 value=v35 form=f35>"
        "<input id=s35 type=submit formaction=/alt35 value=Go></form>",
        (35 % 2) ? "POST" : "GET");
    l75b_render(html);
    TEST_ASSERT_TRUE(g_doc.nodes[l75b_node(sid)].form_flags2 & WE_FF2_FORMACTION);
    for (k = 1; k < (int)g_doc.nnodes; k++)
        if (g_doc.nodes[k].form_type == WE_FORM_SUBMIT) break;
    x = g_doc.nodes[k].x + 2; y = g_doc.nodes[k].y + 2;
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, we_form_click(&g_doc, x, y));
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "/alt35"));
    if (35 % 2) { TEST_ASSERT_EQUAL(1, q.post); }
    else { TEST_ASSERT_EQUAL(0, q.post); }
    l75b_done();
}

static void test_l75_ext_form_36(void) {
    we_form_req_t q;
    char html[700], sid[16];
    int k, x, y;
    sprintf(sid, "s36");
    sprintf(html,
        "<form id=f36 action=/e36 method=%s>"
        "<input name=n36 value=v36 form=f36>"
        "<input id=s36 type=submit formaction=/alt36 value=Go></form>",
        (36 % 2) ? "POST" : "GET");
    l75b_render(html);
    TEST_ASSERT_TRUE(g_doc.nodes[l75b_node(sid)].form_flags2 & WE_FF2_FORMACTION);
    for (k = 1; k < (int)g_doc.nnodes; k++)
        if (g_doc.nodes[k].form_type == WE_FORM_SUBMIT) break;
    x = g_doc.nodes[k].x + 2; y = g_doc.nodes[k].y + 2;
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, we_form_click(&g_doc, x, y));
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "/alt36"));
    if (36 % 2) { TEST_ASSERT_EQUAL(1, q.post); }
    else { TEST_ASSERT_EQUAL(0, q.post); }
    l75b_done();
}

static void test_l75_ext_form_37(void) {
    we_form_req_t q;
    char html[700], sid[16];
    int k, x, y;
    sprintf(sid, "s37");
    sprintf(html,
        "<form id=f37 action=/e37 method=%s>"
        "<input name=n37 value=v37 form=f37>"
        "<input id=s37 type=submit formaction=/alt37 value=Go></form>",
        (37 % 2) ? "POST" : "GET");
    l75b_render(html);
    TEST_ASSERT_TRUE(g_doc.nodes[l75b_node(sid)].form_flags2 & WE_FF2_FORMACTION);
    for (k = 1; k < (int)g_doc.nnodes; k++)
        if (g_doc.nodes[k].form_type == WE_FORM_SUBMIT) break;
    x = g_doc.nodes[k].x + 2; y = g_doc.nodes[k].y + 2;
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, we_form_click(&g_doc, x, y));
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "/alt37"));
    if (37 % 2) { TEST_ASSERT_EQUAL(1, q.post); }
    else { TEST_ASSERT_EQUAL(0, q.post); }
    l75b_done();
}

static void test_l75_ext_form_38(void) {
    we_form_req_t q;
    char html[700], sid[16];
    int k, x, y;
    sprintf(sid, "s38");
    sprintf(html,
        "<form id=f38 action=/e38 method=%s>"
        "<input name=n38 value=v38 form=f38>"
        "<input id=s38 type=submit formaction=/alt38 value=Go></form>",
        (38 % 2) ? "POST" : "GET");
    l75b_render(html);
    TEST_ASSERT_TRUE(g_doc.nodes[l75b_node(sid)].form_flags2 & WE_FF2_FORMACTION);
    for (k = 1; k < (int)g_doc.nnodes; k++)
        if (g_doc.nodes[k].form_type == WE_FORM_SUBMIT) break;
    x = g_doc.nodes[k].x + 2; y = g_doc.nodes[k].y + 2;
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, we_form_click(&g_doc, x, y));
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "/alt38"));
    if (38 % 2) { TEST_ASSERT_EQUAL(1, q.post); }
    else { TEST_ASSERT_EQUAL(0, q.post); }
    l75b_done();
}

static void test_l75_ext_form_39(void) {
    we_form_req_t q;
    char html[700], sid[16];
    int k, x, y;
    sprintf(sid, "s39");
    sprintf(html,
        "<form id=f39 action=/e39 method=%s>"
        "<input name=n39 value=v39 form=f39>"
        "<input id=s39 type=submit formaction=/alt39 value=Go></form>",
        (39 % 2) ? "POST" : "GET");
    l75b_render(html);
    TEST_ASSERT_TRUE(g_doc.nodes[l75b_node(sid)].form_flags2 & WE_FF2_FORMACTION);
    for (k = 1; k < (int)g_doc.nnodes; k++)
        if (g_doc.nodes[k].form_type == WE_FORM_SUBMIT) break;
    x = g_doc.nodes[k].x + 2; y = g_doc.nodes[k].y + 2;
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, we_form_click(&g_doc, x, y));
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "/alt39"));
    if (39 % 2) { TEST_ASSERT_EQUAL(1, q.post); }
    else { TEST_ASSERT_EQUAL(0, q.post); }
    l75b_done();
}

static void test_l75_ext_form_40(void) {
    we_form_req_t q;
    char html[700], sid[16];
    int k, x, y;
    sprintf(sid, "s40");
    sprintf(html,
        "<form id=f40 action=/e40 method=%s>"
        "<input name=n40 value=v40 form=f40>"
        "<input id=s40 type=submit formaction=/alt40 value=Go></form>",
        (40 % 2) ? "POST" : "GET");
    l75b_render(html);
    TEST_ASSERT_TRUE(g_doc.nodes[l75b_node(sid)].form_flags2 & WE_FF2_FORMACTION);
    for (k = 1; k < (int)g_doc.nnodes; k++)
        if (g_doc.nodes[k].form_type == WE_FORM_SUBMIT) break;
    x = g_doc.nodes[k].x + 2; y = g_doc.nodes[k].y + 2;
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, we_form_click(&g_doc, x, y));
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "/alt40"));
    if (40 % 2) { TEST_ASSERT_EQUAL(1, q.post); }
    else { TEST_ASSERT_EQUAL(0, q.post); }
    l75b_done();
}

static void test_l75_ext_form_41(void) {
    we_form_req_t q;
    char html[700], sid[16];
    int k, x, y;
    sprintf(sid, "s41");
    sprintf(html,
        "<form id=f41 action=/e41 method=%s>"
        "<input name=n41 value=v41 form=f41>"
        "<input id=s41 type=submit formaction=/alt41 value=Go></form>",
        (41 % 2) ? "POST" : "GET");
    l75b_render(html);
    TEST_ASSERT_TRUE(g_doc.nodes[l75b_node(sid)].form_flags2 & WE_FF2_FORMACTION);
    for (k = 1; k < (int)g_doc.nnodes; k++)
        if (g_doc.nodes[k].form_type == WE_FORM_SUBMIT) break;
    x = g_doc.nodes[k].x + 2; y = g_doc.nodes[k].y + 2;
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, we_form_click(&g_doc, x, y));
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "/alt41"));
    if (41 % 2) { TEST_ASSERT_EQUAL(1, q.post); }
    else { TEST_ASSERT_EQUAL(0, q.post); }
    l75b_done();
}

static void test_l75_ext_form_42(void) {
    we_form_req_t q;
    char html[700], sid[16];
    int k, x, y;
    sprintf(sid, "s42");
    sprintf(html,
        "<form id=f42 action=/e42 method=%s>"
        "<input name=n42 value=v42 form=f42>"
        "<input id=s42 type=submit formaction=/alt42 value=Go></form>",
        (42 % 2) ? "POST" : "GET");
    l75b_render(html);
    TEST_ASSERT_TRUE(g_doc.nodes[l75b_node(sid)].form_flags2 & WE_FF2_FORMACTION);
    for (k = 1; k < (int)g_doc.nnodes; k++)
        if (g_doc.nodes[k].form_type == WE_FORM_SUBMIT) break;
    x = g_doc.nodes[k].x + 2; y = g_doc.nodes[k].y + 2;
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, we_form_click(&g_doc, x, y));
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "/alt42"));
    if (42 % 2) { TEST_ASSERT_EQUAL(1, q.post); }
    else { TEST_ASSERT_EQUAL(0, q.post); }
    l75b_done();
}

static void test_l75_ext_form_43(void) {
    we_form_req_t q;
    char html[700], sid[16];
    int k, x, y;
    sprintf(sid, "s43");
    sprintf(html,
        "<form id=f43 action=/e43 method=%s>"
        "<input name=n43 value=v43 form=f43>"
        "<input id=s43 type=submit formaction=/alt43 value=Go></form>",
        (43 % 2) ? "POST" : "GET");
    l75b_render(html);
    TEST_ASSERT_TRUE(g_doc.nodes[l75b_node(sid)].form_flags2 & WE_FF2_FORMACTION);
    for (k = 1; k < (int)g_doc.nnodes; k++)
        if (g_doc.nodes[k].form_type == WE_FORM_SUBMIT) break;
    x = g_doc.nodes[k].x + 2; y = g_doc.nodes[k].y + 2;
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, we_form_click(&g_doc, x, y));
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "/alt43"));
    if (43 % 2) { TEST_ASSERT_EQUAL(1, q.post); }
    else { TEST_ASSERT_EQUAL(0, q.post); }
    l75b_done();
}

static void test_l75_ext_form_44(void) {
    we_form_req_t q;
    char html[700], sid[16];
    int k, x, y;
    sprintf(sid, "s44");
    sprintf(html,
        "<form id=f44 action=/e44 method=%s>"
        "<input name=n44 value=v44 form=f44>"
        "<input id=s44 type=submit formaction=/alt44 value=Go></form>",
        (44 % 2) ? "POST" : "GET");
    l75b_render(html);
    TEST_ASSERT_TRUE(g_doc.nodes[l75b_node(sid)].form_flags2 & WE_FF2_FORMACTION);
    for (k = 1; k < (int)g_doc.nnodes; k++)
        if (g_doc.nodes[k].form_type == WE_FORM_SUBMIT) break;
    x = g_doc.nodes[k].x + 2; y = g_doc.nodes[k].y + 2;
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, we_form_click(&g_doc, x, y));
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "/alt44"));
    if (44 % 2) { TEST_ASSERT_EQUAL(1, q.post); }
    else { TEST_ASSERT_EQUAL(0, q.post); }
    l75b_done();
}

static void test_l75_ext_form_45(void) {
    we_form_req_t q;
    char html[700], sid[16];
    int k, x, y;
    sprintf(sid, "s45");
    sprintf(html,
        "<form id=f45 action=/e45 method=%s>"
        "<input name=n45 value=v45 form=f45>"
        "<input id=s45 type=submit formaction=/alt45 value=Go></form>",
        (45 % 2) ? "POST" : "GET");
    l75b_render(html);
    TEST_ASSERT_TRUE(g_doc.nodes[l75b_node(sid)].form_flags2 & WE_FF2_FORMACTION);
    for (k = 1; k < (int)g_doc.nnodes; k++)
        if (g_doc.nodes[k].form_type == WE_FORM_SUBMIT) break;
    x = g_doc.nodes[k].x + 2; y = g_doc.nodes[k].y + 2;
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, we_form_click(&g_doc, x, y));
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "/alt45"));
    if (45 % 2) { TEST_ASSERT_EQUAL(1, q.post); }
    else { TEST_ASSERT_EQUAL(0, q.post); }
    l75b_done();
}

static void test_l75_ext_form_46(void) {
    we_form_req_t q;
    char html[700], sid[16];
    int k, x, y;
    sprintf(sid, "s46");
    sprintf(html,
        "<form id=f46 action=/e46 method=%s>"
        "<input name=n46 value=v46 form=f46>"
        "<input id=s46 type=submit formaction=/alt46 value=Go></form>",
        (46 % 2) ? "POST" : "GET");
    l75b_render(html);
    TEST_ASSERT_TRUE(g_doc.nodes[l75b_node(sid)].form_flags2 & WE_FF2_FORMACTION);
    for (k = 1; k < (int)g_doc.nnodes; k++)
        if (g_doc.nodes[k].form_type == WE_FORM_SUBMIT) break;
    x = g_doc.nodes[k].x + 2; y = g_doc.nodes[k].y + 2;
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, we_form_click(&g_doc, x, y));
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "/alt46"));
    if (46 % 2) { TEST_ASSERT_EQUAL(1, q.post); }
    else { TEST_ASSERT_EQUAL(0, q.post); }
    l75b_done();
}

static void test_l75_ext_form_47(void) {
    we_form_req_t q;
    char html[700], sid[16];
    int k, x, y;
    sprintf(sid, "s47");
    sprintf(html,
        "<form id=f47 action=/e47 method=%s>"
        "<input name=n47 value=v47 form=f47>"
        "<input id=s47 type=submit formaction=/alt47 value=Go></form>",
        (47 % 2) ? "POST" : "GET");
    l75b_render(html);
    TEST_ASSERT_TRUE(g_doc.nodes[l75b_node(sid)].form_flags2 & WE_FF2_FORMACTION);
    for (k = 1; k < (int)g_doc.nnodes; k++)
        if (g_doc.nodes[k].form_type == WE_FORM_SUBMIT) break;
    x = g_doc.nodes[k].x + 2; y = g_doc.nodes[k].y + 2;
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, we_form_click(&g_doc, x, y));
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "/alt47"));
    if (47 % 2) { TEST_ASSERT_EQUAL(1, q.post); }
    else { TEST_ASSERT_EQUAL(0, q.post); }
    l75b_done();
}

static void test_l75_ext_form_48(void) {
    we_form_req_t q;
    char html[700], sid[16];
    int k, x, y;
    sprintf(sid, "s48");
    sprintf(html,
        "<form id=f48 action=/e48 method=%s>"
        "<input name=n48 value=v48 form=f48>"
        "<input id=s48 type=submit formaction=/alt48 value=Go></form>",
        (48 % 2) ? "POST" : "GET");
    l75b_render(html);
    TEST_ASSERT_TRUE(g_doc.nodes[l75b_node(sid)].form_flags2 & WE_FF2_FORMACTION);
    for (k = 1; k < (int)g_doc.nnodes; k++)
        if (g_doc.nodes[k].form_type == WE_FORM_SUBMIT) break;
    x = g_doc.nodes[k].x + 2; y = g_doc.nodes[k].y + 2;
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, we_form_click(&g_doc, x, y));
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "/alt48"));
    if (48 % 2) { TEST_ASSERT_EQUAL(1, q.post); }
    else { TEST_ASSERT_EQUAL(0, q.post); }
    l75b_done();
}

static void test_l75_ext_form_49(void) {
    we_form_req_t q;
    char html[700], sid[16];
    int k, x, y;
    sprintf(sid, "s49");
    sprintf(html,
        "<form id=f49 action=/e49 method=%s>"
        "<input name=n49 value=v49 form=f49>"
        "<input id=s49 type=submit formaction=/alt49 value=Go></form>",
        (49 % 2) ? "POST" : "GET");
    l75b_render(html);
    TEST_ASSERT_TRUE(g_doc.nodes[l75b_node(sid)].form_flags2 & WE_FF2_FORMACTION);
    for (k = 1; k < (int)g_doc.nnodes; k++)
        if (g_doc.nodes[k].form_type == WE_FORM_SUBMIT) break;
    x = g_doc.nodes[k].x + 2; y = g_doc.nodes[k].y + 2;
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, we_form_click(&g_doc, x, y));
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "/alt49"));
    if (49 % 2) { TEST_ASSERT_EQUAL(1, q.post); }
    else { TEST_ASSERT_EQUAL(0, q.post); }
    l75b_done();
}

static void test_l75_ext_form_50(void) {
    we_form_req_t q;
    char html[700], sid[16];
    int k, x, y;
    sprintf(sid, "s50");
    sprintf(html,
        "<form id=f50 action=/e50 method=%s>"
        "<input name=n50 value=v50 form=f50>"
        "<input id=s50 type=submit formaction=/alt50 value=Go></form>",
        (50 % 2) ? "POST" : "GET");
    l75b_render(html);
    TEST_ASSERT_TRUE(g_doc.nodes[l75b_node(sid)].form_flags2 & WE_FF2_FORMACTION);
    for (k = 1; k < (int)g_doc.nnodes; k++)
        if (g_doc.nodes[k].form_type == WE_FORM_SUBMIT) break;
    x = g_doc.nodes[k].x + 2; y = g_doc.nodes[k].y + 2;
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, we_form_click(&g_doc, x, y));
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "/alt50"));
    if (50 % 2) { TEST_ASSERT_EQUAL(1, q.post); }
    else { TEST_ASSERT_EQUAL(0, q.post); }
    l75b_done();
}

static void test_l75_ext_form_51(void) {
    we_form_req_t q;
    char html[700], sid[16];
    int k, x, y;
    sprintf(sid, "s51");
    sprintf(html,
        "<form id=f51 action=/e51 method=%s>"
        "<input name=n51 value=v51 form=f51>"
        "<input id=s51 type=submit formaction=/alt51 value=Go></form>",
        (51 % 2) ? "POST" : "GET");
    l75b_render(html);
    TEST_ASSERT_TRUE(g_doc.nodes[l75b_node(sid)].form_flags2 & WE_FF2_FORMACTION);
    for (k = 1; k < (int)g_doc.nnodes; k++)
        if (g_doc.nodes[k].form_type == WE_FORM_SUBMIT) break;
    x = g_doc.nodes[k].x + 2; y = g_doc.nodes[k].y + 2;
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, we_form_click(&g_doc, x, y));
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "/alt51"));
    if (51 % 2) { TEST_ASSERT_EQUAL(1, q.post); }
    else { TEST_ASSERT_EQUAL(0, q.post); }
    l75b_done();
}

static void test_l75_ext_form_52(void) {
    we_form_req_t q;
    char html[700], sid[16];
    int k, x, y;
    sprintf(sid, "s52");
    sprintf(html,
        "<form id=f52 action=/e52 method=%s>"
        "<input name=n52 value=v52 form=f52>"
        "<input id=s52 type=submit formaction=/alt52 value=Go></form>",
        (52 % 2) ? "POST" : "GET");
    l75b_render(html);
    TEST_ASSERT_TRUE(g_doc.nodes[l75b_node(sid)].form_flags2 & WE_FF2_FORMACTION);
    for (k = 1; k < (int)g_doc.nnodes; k++)
        if (g_doc.nodes[k].form_type == WE_FORM_SUBMIT) break;
    x = g_doc.nodes[k].x + 2; y = g_doc.nodes[k].y + 2;
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, we_form_click(&g_doc, x, y));
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "/alt52"));
    if (52 % 2) { TEST_ASSERT_EQUAL(1, q.post); }
    else { TEST_ASSERT_EQUAL(0, q.post); }
    l75b_done();
}

static void test_l75_ext_form_53(void) {
    we_form_req_t q;
    char html[700], sid[16];
    int k, x, y;
    sprintf(sid, "s53");
    sprintf(html,
        "<form id=f53 action=/e53 method=%s>"
        "<input name=n53 value=v53 form=f53>"
        "<input id=s53 type=submit formaction=/alt53 value=Go></form>",
        (53 % 2) ? "POST" : "GET");
    l75b_render(html);
    TEST_ASSERT_TRUE(g_doc.nodes[l75b_node(sid)].form_flags2 & WE_FF2_FORMACTION);
    for (k = 1; k < (int)g_doc.nnodes; k++)
        if (g_doc.nodes[k].form_type == WE_FORM_SUBMIT) break;
    x = g_doc.nodes[k].x + 2; y = g_doc.nodes[k].y + 2;
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, we_form_click(&g_doc, x, y));
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "/alt53"));
    if (53 % 2) { TEST_ASSERT_EQUAL(1, q.post); }
    else { TEST_ASSERT_EQUAL(0, q.post); }
    l75b_done();
}

static void test_l75_ext_form_54(void) {
    we_form_req_t q;
    char html[700], sid[16];
    int k, x, y;
    sprintf(sid, "s54");
    sprintf(html,
        "<form id=f54 action=/e54 method=%s>"
        "<input name=n54 value=v54 form=f54>"
        "<input id=s54 type=submit formaction=/alt54 value=Go></form>",
        (54 % 2) ? "POST" : "GET");
    l75b_render(html);
    TEST_ASSERT_TRUE(g_doc.nodes[l75b_node(sid)].form_flags2 & WE_FF2_FORMACTION);
    for (k = 1; k < (int)g_doc.nnodes; k++)
        if (g_doc.nodes[k].form_type == WE_FORM_SUBMIT) break;
    x = g_doc.nodes[k].x + 2; y = g_doc.nodes[k].y + 2;
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, we_form_click(&g_doc, x, y));
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "/alt54"));
    if (54 % 2) { TEST_ASSERT_EQUAL(1, q.post); }
    else { TEST_ASSERT_EQUAL(0, q.post); }
    l75b_done();
}

static void test_l75_ext_form_55(void) {
    we_form_req_t q;
    char html[700], sid[16];
    int k, x, y;
    sprintf(sid, "s55");
    sprintf(html,
        "<form id=f55 action=/e55 method=%s>"
        "<input name=n55 value=v55 form=f55>"
        "<input id=s55 type=submit formaction=/alt55 value=Go></form>",
        (55 % 2) ? "POST" : "GET");
    l75b_render(html);
    TEST_ASSERT_TRUE(g_doc.nodes[l75b_node(sid)].form_flags2 & WE_FF2_FORMACTION);
    for (k = 1; k < (int)g_doc.nnodes; k++)
        if (g_doc.nodes[k].form_type == WE_FORM_SUBMIT) break;
    x = g_doc.nodes[k].x + 2; y = g_doc.nodes[k].y + 2;
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, we_form_click(&g_doc, x, y));
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "/alt55"));
    if (55 % 2) { TEST_ASSERT_EQUAL(1, q.post); }
    else { TEST_ASSERT_EQUAL(0, q.post); }
    l75b_done();
}

static void test_l75_ext_form_56(void) {
    we_form_req_t q;
    char html[700], sid[16];
    int k, x, y;
    sprintf(sid, "s56");
    sprintf(html,
        "<form id=f56 action=/e56 method=%s>"
        "<input name=n56 value=v56 form=f56>"
        "<input id=s56 type=submit formaction=/alt56 value=Go></form>",
        (56 % 2) ? "POST" : "GET");
    l75b_render(html);
    TEST_ASSERT_TRUE(g_doc.nodes[l75b_node(sid)].form_flags2 & WE_FF2_FORMACTION);
    for (k = 1; k < (int)g_doc.nnodes; k++)
        if (g_doc.nodes[k].form_type == WE_FORM_SUBMIT) break;
    x = g_doc.nodes[k].x + 2; y = g_doc.nodes[k].y + 2;
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, we_form_click(&g_doc, x, y));
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "/alt56"));
    if (56 % 2) { TEST_ASSERT_EQUAL(1, q.post); }
    else { TEST_ASSERT_EQUAL(0, q.post); }
    l75b_done();
}

static void test_l75_ext_form_57(void) {
    we_form_req_t q;
    char html[700], sid[16];
    int k, x, y;
    sprintf(sid, "s57");
    sprintf(html,
        "<form id=f57 action=/e57 method=%s>"
        "<input name=n57 value=v57 form=f57>"
        "<input id=s57 type=submit formaction=/alt57 value=Go></form>",
        (57 % 2) ? "POST" : "GET");
    l75b_render(html);
    TEST_ASSERT_TRUE(g_doc.nodes[l75b_node(sid)].form_flags2 & WE_FF2_FORMACTION);
    for (k = 1; k < (int)g_doc.nnodes; k++)
        if (g_doc.nodes[k].form_type == WE_FORM_SUBMIT) break;
    x = g_doc.nodes[k].x + 2; y = g_doc.nodes[k].y + 2;
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, we_form_click(&g_doc, x, y));
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "/alt57"));
    if (57 % 2) { TEST_ASSERT_EQUAL(1, q.post); }
    else { TEST_ASSERT_EQUAL(0, q.post); }
    l75b_done();
}

static void test_l75_ext_form_58(void) {
    we_form_req_t q;
    char html[700], sid[16];
    int k, x, y;
    sprintf(sid, "s58");
    sprintf(html,
        "<form id=f58 action=/e58 method=%s>"
        "<input name=n58 value=v58 form=f58>"
        "<input id=s58 type=submit formaction=/alt58 value=Go></form>",
        (58 % 2) ? "POST" : "GET");
    l75b_render(html);
    TEST_ASSERT_TRUE(g_doc.nodes[l75b_node(sid)].form_flags2 & WE_FF2_FORMACTION);
    for (k = 1; k < (int)g_doc.nnodes; k++)
        if (g_doc.nodes[k].form_type == WE_FORM_SUBMIT) break;
    x = g_doc.nodes[k].x + 2; y = g_doc.nodes[k].y + 2;
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, we_form_click(&g_doc, x, y));
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "/alt58"));
    if (58 % 2) { TEST_ASSERT_EQUAL(1, q.post); }
    else { TEST_ASSERT_EQUAL(0, q.post); }
    l75b_done();
}

static void test_l75_ext_form_59(void) {
    we_form_req_t q;
    char html[700], sid[16];
    int k, x, y;
    sprintf(sid, "s59");
    sprintf(html,
        "<form id=f59 action=/e59 method=%s>"
        "<input name=n59 value=v59 form=f59>"
        "<input id=s59 type=submit formaction=/alt59 value=Go></form>",
        (59 % 2) ? "POST" : "GET");
    l75b_render(html);
    TEST_ASSERT_TRUE(g_doc.nodes[l75b_node(sid)].form_flags2 & WE_FF2_FORMACTION);
    for (k = 1; k < (int)g_doc.nnodes; k++)
        if (g_doc.nodes[k].form_type == WE_FORM_SUBMIT) break;
    x = g_doc.nodes[k].x + 2; y = g_doc.nodes[k].y + 2;
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, we_form_click(&g_doc, x, y));
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "/alt59"));
    if (59 % 2) { TEST_ASSERT_EQUAL(1, q.post); }
    else { TEST_ASSERT_EQUAL(0, q.post); }
    l75b_done();
}

static void test_l75_ext_form_60(void) {
    we_form_req_t q;
    char html[700], sid[16];
    int k, x, y;
    sprintf(sid, "s60");
    sprintf(html,
        "<form id=f60 action=/e60 method=%s>"
        "<input name=n60 value=v60 form=f60>"
        "<input id=s60 type=submit formaction=/alt60 value=Go></form>",
        (60 % 2) ? "POST" : "GET");
    l75b_render(html);
    TEST_ASSERT_TRUE(g_doc.nodes[l75b_node(sid)].form_flags2 & WE_FF2_FORMACTION);
    for (k = 1; k < (int)g_doc.nnodes; k++)
        if (g_doc.nodes[k].form_type == WE_FORM_SUBMIT) break;
    x = g_doc.nodes[k].x + 2; y = g_doc.nodes[k].y + 2;
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, we_form_click(&g_doc, x, y));
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "/alt60"));
    if (60 % 2) { TEST_ASSERT_EQUAL(1, q.post); }
    else { TEST_ASSERT_EQUAL(0, q.post); }
    l75b_done();
}

static void test_l75_ext_form_61(void) {
    we_form_req_t q;
    char html[700], sid[16];
    int k, x, y;
    sprintf(sid, "s61");
    sprintf(html,
        "<form id=f61 action=/e61 method=%s>"
        "<input name=n61 value=v61 form=f61>"
        "<input id=s61 type=submit formaction=/alt61 value=Go></form>",
        (61 % 2) ? "POST" : "GET");
    l75b_render(html);
    TEST_ASSERT_TRUE(g_doc.nodes[l75b_node(sid)].form_flags2 & WE_FF2_FORMACTION);
    for (k = 1; k < (int)g_doc.nnodes; k++)
        if (g_doc.nodes[k].form_type == WE_FORM_SUBMIT) break;
    x = g_doc.nodes[k].x + 2; y = g_doc.nodes[k].y + 2;
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, we_form_click(&g_doc, x, y));
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "/alt61"));
    if (61 % 2) { TEST_ASSERT_EQUAL(1, q.post); }
    else { TEST_ASSERT_EQUAL(0, q.post); }
    l75b_done();
}

static void test_l75_ext_form_62(void) {
    we_form_req_t q;
    char html[700], sid[16];
    int k, x, y;
    sprintf(sid, "s62");
    sprintf(html,
        "<form id=f62 action=/e62 method=%s>"
        "<input name=n62 value=v62 form=f62>"
        "<input id=s62 type=submit formaction=/alt62 value=Go></form>",
        (62 % 2) ? "POST" : "GET");
    l75b_render(html);
    TEST_ASSERT_TRUE(g_doc.nodes[l75b_node(sid)].form_flags2 & WE_FF2_FORMACTION);
    for (k = 1; k < (int)g_doc.nnodes; k++)
        if (g_doc.nodes[k].form_type == WE_FORM_SUBMIT) break;
    x = g_doc.nodes[k].x + 2; y = g_doc.nodes[k].y + 2;
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, we_form_click(&g_doc, x, y));
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "/alt62"));
    if (62 % 2) { TEST_ASSERT_EQUAL(1, q.post); }
    else { TEST_ASSERT_EQUAL(0, q.post); }
    l75b_done();
}

static void test_l75_ext_form_63(void) {
    we_form_req_t q;
    char html[700], sid[16];
    int k, x, y;
    sprintf(sid, "s63");
    sprintf(html,
        "<form id=f63 action=/e63 method=%s>"
        "<input name=n63 value=v63 form=f63>"
        "<input id=s63 type=submit formaction=/alt63 value=Go></form>",
        (63 % 2) ? "POST" : "GET");
    l75b_render(html);
    TEST_ASSERT_TRUE(g_doc.nodes[l75b_node(sid)].form_flags2 & WE_FF2_FORMACTION);
    for (k = 1; k < (int)g_doc.nnodes; k++)
        if (g_doc.nodes[k].form_type == WE_FORM_SUBMIT) break;
    x = g_doc.nodes[k].x + 2; y = g_doc.nodes[k].y + 2;
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, we_form_click(&g_doc, x, y));
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "/alt63"));
    if (63 % 2) { TEST_ASSERT_EQUAL(1, q.post); }
    else { TEST_ASSERT_EQUAL(0, q.post); }
    l75b_done();
}

static void test_l75_ext_form_64(void) {
    we_form_req_t q;
    char html[700], sid[16];
    int k, x, y;
    sprintf(sid, "s64");
    sprintf(html,
        "<form id=f64 action=/e64 method=%s>"
        "<input name=n64 value=v64 form=f64>"
        "<input id=s64 type=submit formaction=/alt64 value=Go></form>",
        (64 % 2) ? "POST" : "GET");
    l75b_render(html);
    TEST_ASSERT_TRUE(g_doc.nodes[l75b_node(sid)].form_flags2 & WE_FF2_FORMACTION);
    for (k = 1; k < (int)g_doc.nnodes; k++)
        if (g_doc.nodes[k].form_type == WE_FORM_SUBMIT) break;
    x = g_doc.nodes[k].x + 2; y = g_doc.nodes[k].y + 2;
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, we_form_click(&g_doc, x, y));
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "/alt64"));
    if (64 % 2) { TEST_ASSERT_EQUAL(1, q.post); }
    else { TEST_ASSERT_EQUAL(0, q.post); }
    l75b_done();
}

static void test_l75_ext_form_65(void) {
    we_form_req_t q;
    char html[700], sid[16];
    int k, x, y;
    sprintf(sid, "s65");
    sprintf(html,
        "<form id=f65 action=/e65 method=%s>"
        "<input name=n65 value=v65 form=f65>"
        "<input id=s65 type=submit formaction=/alt65 value=Go></form>",
        (65 % 2) ? "POST" : "GET");
    l75b_render(html);
    TEST_ASSERT_TRUE(g_doc.nodes[l75b_node(sid)].form_flags2 & WE_FF2_FORMACTION);
    for (k = 1; k < (int)g_doc.nnodes; k++)
        if (g_doc.nodes[k].form_type == WE_FORM_SUBMIT) break;
    x = g_doc.nodes[k].x + 2; y = g_doc.nodes[k].y + 2;
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, we_form_click(&g_doc, x, y));
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "/alt65"));
    if (65 % 2) { TEST_ASSERT_EQUAL(1, q.post); }
    else { TEST_ASSERT_EQUAL(0, q.post); }
    l75b_done();
}

static void test_l75_ext_form_66(void) {
    we_form_req_t q;
    char html[700], sid[16];
    int k, x, y;
    sprintf(sid, "s66");
    sprintf(html,
        "<form id=f66 action=/e66 method=%s>"
        "<input name=n66 value=v66 form=f66>"
        "<input id=s66 type=submit formaction=/alt66 value=Go></form>",
        (66 % 2) ? "POST" : "GET");
    l75b_render(html);
    TEST_ASSERT_TRUE(g_doc.nodes[l75b_node(sid)].form_flags2 & WE_FF2_FORMACTION);
    for (k = 1; k < (int)g_doc.nnodes; k++)
        if (g_doc.nodes[k].form_type == WE_FORM_SUBMIT) break;
    x = g_doc.nodes[k].x + 2; y = g_doc.nodes[k].y + 2;
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, we_form_click(&g_doc, x, y));
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "/alt66"));
    if (66 % 2) { TEST_ASSERT_EQUAL(1, q.post); }
    else { TEST_ASSERT_EQUAL(0, q.post); }
    l75b_done();
}

static void test_l75_ext_form_67(void) {
    we_form_req_t q;
    char html[700], sid[16];
    int k, x, y;
    sprintf(sid, "s67");
    sprintf(html,
        "<form id=f67 action=/e67 method=%s>"
        "<input name=n67 value=v67 form=f67>"
        "<input id=s67 type=submit formaction=/alt67 value=Go></form>",
        (67 % 2) ? "POST" : "GET");
    l75b_render(html);
    TEST_ASSERT_TRUE(g_doc.nodes[l75b_node(sid)].form_flags2 & WE_FF2_FORMACTION);
    for (k = 1; k < (int)g_doc.nnodes; k++)
        if (g_doc.nodes[k].form_type == WE_FORM_SUBMIT) break;
    x = g_doc.nodes[k].x + 2; y = g_doc.nodes[k].y + 2;
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, we_form_click(&g_doc, x, y));
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "/alt67"));
    if (67 % 2) { TEST_ASSERT_EQUAL(1, q.post); }
    else { TEST_ASSERT_EQUAL(0, q.post); }
    l75b_done();
}

static void test_l75_ext_form_68(void) {
    we_form_req_t q;
    char html[700], sid[16];
    int k, x, y;
    sprintf(sid, "s68");
    sprintf(html,
        "<form id=f68 action=/e68 method=%s>"
        "<input name=n68 value=v68 form=f68>"
        "<input id=s68 type=submit formaction=/alt68 value=Go></form>",
        (68 % 2) ? "POST" : "GET");
    l75b_render(html);
    TEST_ASSERT_TRUE(g_doc.nodes[l75b_node(sid)].form_flags2 & WE_FF2_FORMACTION);
    for (k = 1; k < (int)g_doc.nnodes; k++)
        if (g_doc.nodes[k].form_type == WE_FORM_SUBMIT) break;
    x = g_doc.nodes[k].x + 2; y = g_doc.nodes[k].y + 2;
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, we_form_click(&g_doc, x, y));
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "/alt68"));
    if (68 % 2) { TEST_ASSERT_EQUAL(1, q.post); }
    else { TEST_ASSERT_EQUAL(0, q.post); }
    l75b_done();
}

static void test_l75_ext_form_69(void) {
    we_form_req_t q;
    char html[700], sid[16];
    int k, x, y;
    sprintf(sid, "s69");
    sprintf(html,
        "<form id=f69 action=/e69 method=%s>"
        "<input name=n69 value=v69 form=f69>"
        "<input id=s69 type=submit formaction=/alt69 value=Go></form>",
        (69 % 2) ? "POST" : "GET");
    l75b_render(html);
    TEST_ASSERT_TRUE(g_doc.nodes[l75b_node(sid)].form_flags2 & WE_FF2_FORMACTION);
    for (k = 1; k < (int)g_doc.nnodes; k++)
        if (g_doc.nodes[k].form_type == WE_FORM_SUBMIT) break;
    x = g_doc.nodes[k].x + 2; y = g_doc.nodes[k].y + 2;
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, we_form_click(&g_doc, x, y));
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "/alt69"));
    if (69 % 2) { TEST_ASSERT_EQUAL(1, q.post); }
    else { TEST_ASSERT_EQUAL(0, q.post); }
    l75b_done();
}

static void test_l75_ext_form_70(void) {
    we_form_req_t q;
    char html[700], sid[16];
    int k, x, y;
    sprintf(sid, "s70");
    sprintf(html,
        "<form id=f70 action=/e70 method=%s>"
        "<input name=n70 value=v70 form=f70>"
        "<input id=s70 type=submit formaction=/alt70 value=Go></form>",
        (70 % 2) ? "POST" : "GET");
    l75b_render(html);
    TEST_ASSERT_TRUE(g_doc.nodes[l75b_node(sid)].form_flags2 & WE_FF2_FORMACTION);
    for (k = 1; k < (int)g_doc.nnodes; k++)
        if (g_doc.nodes[k].form_type == WE_FORM_SUBMIT) break;
    x = g_doc.nodes[k].x + 2; y = g_doc.nodes[k].y + 2;
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, we_form_click(&g_doc, x, y));
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "/alt70"));
    if (70 % 2) { TEST_ASSERT_EQUAL(1, q.post); }
    else { TEST_ASSERT_EQUAL(0, q.post); }
    l75b_done();
}

static void test_l75_ext_form_71(void) {
    we_form_req_t q;
    char html[700], sid[16];
    int k, x, y;
    sprintf(sid, "s71");
    sprintf(html,
        "<form id=f71 action=/e71 method=%s>"
        "<input name=n71 value=v71 form=f71>"
        "<input id=s71 type=submit formaction=/alt71 value=Go></form>",
        (71 % 2) ? "POST" : "GET");
    l75b_render(html);
    TEST_ASSERT_TRUE(g_doc.nodes[l75b_node(sid)].form_flags2 & WE_FF2_FORMACTION);
    for (k = 1; k < (int)g_doc.nnodes; k++)
        if (g_doc.nodes[k].form_type == WE_FORM_SUBMIT) break;
    x = g_doc.nodes[k].x + 2; y = g_doc.nodes[k].y + 2;
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, we_form_click(&g_doc, x, y));
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "/alt71"));
    if (71 % 2) { TEST_ASSERT_EQUAL(1, q.post); }
    else { TEST_ASSERT_EQUAL(0, q.post); }
    l75b_done();
}

static void test_l75_ext_form_72(void) {
    we_form_req_t q;
    char html[700], sid[16];
    int k, x, y;
    sprintf(sid, "s72");
    sprintf(html,
        "<form id=f72 action=/e72 method=%s>"
        "<input name=n72 value=v72 form=f72>"
        "<input id=s72 type=submit formaction=/alt72 value=Go></form>",
        (72 % 2) ? "POST" : "GET");
    l75b_render(html);
    TEST_ASSERT_TRUE(g_doc.nodes[l75b_node(sid)].form_flags2 & WE_FF2_FORMACTION);
    for (k = 1; k < (int)g_doc.nnodes; k++)
        if (g_doc.nodes[k].form_type == WE_FORM_SUBMIT) break;
    x = g_doc.nodes[k].x + 2; y = g_doc.nodes[k].y + 2;
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, we_form_click(&g_doc, x, y));
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "/alt72"));
    if (72 % 2) { TEST_ASSERT_EQUAL(1, q.post); }
    else { TEST_ASSERT_EQUAL(0, q.post); }
    l75b_done();
}

static void test_l75_ext_form_73(void) {
    we_form_req_t q;
    char html[700], sid[16];
    int k, x, y;
    sprintf(sid, "s73");
    sprintf(html,
        "<form id=f73 action=/e73 method=%s>"
        "<input name=n73 value=v73 form=f73>"
        "<input id=s73 type=submit formaction=/alt73 value=Go></form>",
        (73 % 2) ? "POST" : "GET");
    l75b_render(html);
    TEST_ASSERT_TRUE(g_doc.nodes[l75b_node(sid)].form_flags2 & WE_FF2_FORMACTION);
    for (k = 1; k < (int)g_doc.nnodes; k++)
        if (g_doc.nodes[k].form_type == WE_FORM_SUBMIT) break;
    x = g_doc.nodes[k].x + 2; y = g_doc.nodes[k].y + 2;
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, we_form_click(&g_doc, x, y));
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "/alt73"));
    if (73 % 2) { TEST_ASSERT_EQUAL(1, q.post); }
    else { TEST_ASSERT_EQUAL(0, q.post); }
    l75b_done();
}

static void test_l75_ext_form_74(void) {
    we_form_req_t q;
    char html[700], sid[16];
    int k, x, y;
    sprintf(sid, "s74");
    sprintf(html,
        "<form id=f74 action=/e74 method=%s>"
        "<input name=n74 value=v74 form=f74>"
        "<input id=s74 type=submit formaction=/alt74 value=Go></form>",
        (74 % 2) ? "POST" : "GET");
    l75b_render(html);
    TEST_ASSERT_TRUE(g_doc.nodes[l75b_node(sid)].form_flags2 & WE_FF2_FORMACTION);
    for (k = 1; k < (int)g_doc.nnodes; k++)
        if (g_doc.nodes[k].form_type == WE_FORM_SUBMIT) break;
    x = g_doc.nodes[k].x + 2; y = g_doc.nodes[k].y + 2;
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, we_form_click(&g_doc, x, y));
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "/alt74"));
    if (74 % 2) { TEST_ASSERT_EQUAL(1, q.post); }
    else { TEST_ASSERT_EQUAL(0, q.post); }
    l75b_done();
}

static void test_l75_ext_form_75(void) {
    we_form_req_t q;
    char html[700], sid[16];
    int k, x, y;
    sprintf(sid, "s75");
    sprintf(html,
        "<form id=f75 action=/e75 method=%s>"
        "<input name=n75 value=v75 form=f75>"
        "<input id=s75 type=submit formaction=/alt75 value=Go></form>",
        (75 % 2) ? "POST" : "GET");
    l75b_render(html);
    TEST_ASSERT_TRUE(g_doc.nodes[l75b_node(sid)].form_flags2 & WE_FF2_FORMACTION);
    for (k = 1; k < (int)g_doc.nnodes; k++)
        if (g_doc.nodes[k].form_type == WE_FORM_SUBMIT) break;
    x = g_doc.nodes[k].x + 2; y = g_doc.nodes[k].y + 2;
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, we_form_click(&g_doc, x, y));
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "/alt75"));
    if (75 % 2) { TEST_ASSERT_EQUAL(1, q.post); }
    else { TEST_ASSERT_EQUAL(0, q.post); }
    l75b_done();
}

static void test_l75_ext_form_76(void) {
    we_form_req_t q;
    char html[700], sid[16];
    int k, x, y;
    sprintf(sid, "s76");
    sprintf(html,
        "<form id=f76 action=/e76 method=%s>"
        "<input name=n76 value=v76 form=f76>"
        "<input id=s76 type=submit formaction=/alt76 value=Go></form>",
        (76 % 2) ? "POST" : "GET");
    l75b_render(html);
    TEST_ASSERT_TRUE(g_doc.nodes[l75b_node(sid)].form_flags2 & WE_FF2_FORMACTION);
    for (k = 1; k < (int)g_doc.nnodes; k++)
        if (g_doc.nodes[k].form_type == WE_FORM_SUBMIT) break;
    x = g_doc.nodes[k].x + 2; y = g_doc.nodes[k].y + 2;
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, we_form_click(&g_doc, x, y));
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "/alt76"));
    if (76 % 2) { TEST_ASSERT_EQUAL(1, q.post); }
    else { TEST_ASSERT_EQUAL(0, q.post); }
    l75b_done();
}

static void test_l75_ext_form_77(void) {
    we_form_req_t q;
    char html[700], sid[16];
    int k, x, y;
    sprintf(sid, "s77");
    sprintf(html,
        "<form id=f77 action=/e77 method=%s>"
        "<input name=n77 value=v77 form=f77>"
        "<input id=s77 type=submit formaction=/alt77 value=Go></form>",
        (77 % 2) ? "POST" : "GET");
    l75b_render(html);
    TEST_ASSERT_TRUE(g_doc.nodes[l75b_node(sid)].form_flags2 & WE_FF2_FORMACTION);
    for (k = 1; k < (int)g_doc.nnodes; k++)
        if (g_doc.nodes[k].form_type == WE_FORM_SUBMIT) break;
    x = g_doc.nodes[k].x + 2; y = g_doc.nodes[k].y + 2;
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, we_form_click(&g_doc, x, y));
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "/alt77"));
    if (77 % 2) { TEST_ASSERT_EQUAL(1, q.post); }
    else { TEST_ASSERT_EQUAL(0, q.post); }
    l75b_done();
}

static void test_l75_ext_form_78(void) {
    we_form_req_t q;
    char html[700], sid[16];
    int k, x, y;
    sprintf(sid, "s78");
    sprintf(html,
        "<form id=f78 action=/e78 method=%s>"
        "<input name=n78 value=v78 form=f78>"
        "<input id=s78 type=submit formaction=/alt78 value=Go></form>",
        (78 % 2) ? "POST" : "GET");
    l75b_render(html);
    TEST_ASSERT_TRUE(g_doc.nodes[l75b_node(sid)].form_flags2 & WE_FF2_FORMACTION);
    for (k = 1; k < (int)g_doc.nnodes; k++)
        if (g_doc.nodes[k].form_type == WE_FORM_SUBMIT) break;
    x = g_doc.nodes[k].x + 2; y = g_doc.nodes[k].y + 2;
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, we_form_click(&g_doc, x, y));
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "/alt78"));
    if (78 % 2) { TEST_ASSERT_EQUAL(1, q.post); }
    else { TEST_ASSERT_EQUAL(0, q.post); }
    l75b_done();
}

static void test_l75_ext_form_79(void) {
    we_form_req_t q;
    char html[700], sid[16];
    int k, x, y;
    sprintf(sid, "s79");
    sprintf(html,
        "<form id=f79 action=/e79 method=%s>"
        "<input name=n79 value=v79 form=f79>"
        "<input id=s79 type=submit formaction=/alt79 value=Go></form>",
        (79 % 2) ? "POST" : "GET");
    l75b_render(html);
    TEST_ASSERT_TRUE(g_doc.nodes[l75b_node(sid)].form_flags2 & WE_FF2_FORMACTION);
    for (k = 1; k < (int)g_doc.nnodes; k++)
        if (g_doc.nodes[k].form_type == WE_FORM_SUBMIT) break;
    x = g_doc.nodes[k].x + 2; y = g_doc.nodes[k].y + 2;
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, we_form_click(&g_doc, x, y));
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "/alt79"));
    if (79 % 2) { TEST_ASSERT_EQUAL(1, q.post); }
    else { TEST_ASSERT_EQUAL(0, q.post); }
    l75b_done();
}

static void test_l75_ext_form_80(void) {
    we_form_req_t q;
    char html[700], sid[16];
    int k, x, y;
    sprintf(sid, "s80");
    sprintf(html,
        "<form id=f80 action=/e80 method=%s>"
        "<input name=n80 value=v80 form=f80>"
        "<input id=s80 type=submit formaction=/alt80 value=Go></form>",
        (80 % 2) ? "POST" : "GET");
    l75b_render(html);
    TEST_ASSERT_TRUE(g_doc.nodes[l75b_node(sid)].form_flags2 & WE_FF2_FORMACTION);
    for (k = 1; k < (int)g_doc.nnodes; k++)
        if (g_doc.nodes[k].form_type == WE_FORM_SUBMIT) break;
    x = g_doc.nodes[k].x + 2; y = g_doc.nodes[k].y + 2;
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, we_form_click(&g_doc, x, y));
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "/alt80"));
    if (80 % 2) { TEST_ASSERT_EQUAL(1, q.post); }
    else { TEST_ASSERT_EQUAL(0, q.post); }
    l75b_done();
}

static void test_l75_ext_form_81(void) {
    we_form_req_t q;
    char html[700], sid[16];
    int k, x, y;
    sprintf(sid, "s81");
    sprintf(html,
        "<form id=f81 action=/e81 method=%s>"
        "<input name=n81 value=v81 form=f81>"
        "<input id=s81 type=submit formaction=/alt81 value=Go></form>",
        (81 % 2) ? "POST" : "GET");
    l75b_render(html);
    TEST_ASSERT_TRUE(g_doc.nodes[l75b_node(sid)].form_flags2 & WE_FF2_FORMACTION);
    for (k = 1; k < (int)g_doc.nnodes; k++)
        if (g_doc.nodes[k].form_type == WE_FORM_SUBMIT) break;
    x = g_doc.nodes[k].x + 2; y = g_doc.nodes[k].y + 2;
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, we_form_click(&g_doc, x, y));
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "/alt81"));
    if (81 % 2) { TEST_ASSERT_EQUAL(1, q.post); }
    else { TEST_ASSERT_EQUAL(0, q.post); }
    l75b_done();
}

static void test_l75_ext_form_82(void) {
    we_form_req_t q;
    char html[700], sid[16];
    int k, x, y;
    sprintf(sid, "s82");
    sprintf(html,
        "<form id=f82 action=/e82 method=%s>"
        "<input name=n82 value=v82 form=f82>"
        "<input id=s82 type=submit formaction=/alt82 value=Go></form>",
        (82 % 2) ? "POST" : "GET");
    l75b_render(html);
    TEST_ASSERT_TRUE(g_doc.nodes[l75b_node(sid)].form_flags2 & WE_FF2_FORMACTION);
    for (k = 1; k < (int)g_doc.nnodes; k++)
        if (g_doc.nodes[k].form_type == WE_FORM_SUBMIT) break;
    x = g_doc.nodes[k].x + 2; y = g_doc.nodes[k].y + 2;
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, we_form_click(&g_doc, x, y));
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "/alt82"));
    if (82 % 2) { TEST_ASSERT_EQUAL(1, q.post); }
    else { TEST_ASSERT_EQUAL(0, q.post); }
    l75b_done();
}

static void test_l75_ext_form_83(void) {
    we_form_req_t q;
    char html[700], sid[16];
    int k, x, y;
    sprintf(sid, "s83");
    sprintf(html,
        "<form id=f83 action=/e83 method=%s>"
        "<input name=n83 value=v83 form=f83>"
        "<input id=s83 type=submit formaction=/alt83 value=Go></form>",
        (83 % 2) ? "POST" : "GET");
    l75b_render(html);
    TEST_ASSERT_TRUE(g_doc.nodes[l75b_node(sid)].form_flags2 & WE_FF2_FORMACTION);
    for (k = 1; k < (int)g_doc.nnodes; k++)
        if (g_doc.nodes[k].form_type == WE_FORM_SUBMIT) break;
    x = g_doc.nodes[k].x + 2; y = g_doc.nodes[k].y + 2;
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, we_form_click(&g_doc, x, y));
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "/alt83"));
    if (83 % 2) { TEST_ASSERT_EQUAL(1, q.post); }
    else { TEST_ASSERT_EQUAL(0, q.post); }
    l75b_done();
}

static void test_l75_ext_form_84(void) {
    we_form_req_t q;
    char html[700], sid[16];
    int k, x, y;
    sprintf(sid, "s84");
    sprintf(html,
        "<form id=f84 action=/e84 method=%s>"
        "<input name=n84 value=v84 form=f84>"
        "<input id=s84 type=submit formaction=/alt84 value=Go></form>",
        (84 % 2) ? "POST" : "GET");
    l75b_render(html);
    TEST_ASSERT_TRUE(g_doc.nodes[l75b_node(sid)].form_flags2 & WE_FF2_FORMACTION);
    for (k = 1; k < (int)g_doc.nnodes; k++)
        if (g_doc.nodes[k].form_type == WE_FORM_SUBMIT) break;
    x = g_doc.nodes[k].x + 2; y = g_doc.nodes[k].y + 2;
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, we_form_click(&g_doc, x, y));
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "/alt84"));
    if (84 % 2) { TEST_ASSERT_EQUAL(1, q.post); }
    else { TEST_ASSERT_EQUAL(0, q.post); }
    l75b_done();
}

static void test_l75_ext_form_85(void) {
    we_form_req_t q;
    char html[700], sid[16];
    int k, x, y;
    sprintf(sid, "s85");
    sprintf(html,
        "<form id=f85 action=/e85 method=%s>"
        "<input name=n85 value=v85 form=f85>"
        "<input id=s85 type=submit formaction=/alt85 value=Go></form>",
        (85 % 2) ? "POST" : "GET");
    l75b_render(html);
    TEST_ASSERT_TRUE(g_doc.nodes[l75b_node(sid)].form_flags2 & WE_FF2_FORMACTION);
    for (k = 1; k < (int)g_doc.nnodes; k++)
        if (g_doc.nodes[k].form_type == WE_FORM_SUBMIT) break;
    x = g_doc.nodes[k].x + 2; y = g_doc.nodes[k].y + 2;
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, we_form_click(&g_doc, x, y));
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "/alt85"));
    if (85 % 2) { TEST_ASSERT_EQUAL(1, q.post); }
    else { TEST_ASSERT_EQUAL(0, q.post); }
    l75b_done();
}

static void test_l75_ext_form_86(void) {
    we_form_req_t q;
    char html[700], sid[16];
    int k, x, y;
    sprintf(sid, "s86");
    sprintf(html,
        "<form id=f86 action=/e86 method=%s>"
        "<input name=n86 value=v86 form=f86>"
        "<input id=s86 type=submit formaction=/alt86 value=Go></form>",
        (86 % 2) ? "POST" : "GET");
    l75b_render(html);
    TEST_ASSERT_TRUE(g_doc.nodes[l75b_node(sid)].form_flags2 & WE_FF2_FORMACTION);
    for (k = 1; k < (int)g_doc.nnodes; k++)
        if (g_doc.nodes[k].form_type == WE_FORM_SUBMIT) break;
    x = g_doc.nodes[k].x + 2; y = g_doc.nodes[k].y + 2;
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, we_form_click(&g_doc, x, y));
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "/alt86"));
    if (86 % 2) { TEST_ASSERT_EQUAL(1, q.post); }
    else { TEST_ASSERT_EQUAL(0, q.post); }
    l75b_done();
}

static void test_l75_ext_form_87(void) {
    we_form_req_t q;
    char html[700], sid[16];
    int k, x, y;
    sprintf(sid, "s87");
    sprintf(html,
        "<form id=f87 action=/e87 method=%s>"
        "<input name=n87 value=v87 form=f87>"
        "<input id=s87 type=submit formaction=/alt87 value=Go></form>",
        (87 % 2) ? "POST" : "GET");
    l75b_render(html);
    TEST_ASSERT_TRUE(g_doc.nodes[l75b_node(sid)].form_flags2 & WE_FF2_FORMACTION);
    for (k = 1; k < (int)g_doc.nnodes; k++)
        if (g_doc.nodes[k].form_type == WE_FORM_SUBMIT) break;
    x = g_doc.nodes[k].x + 2; y = g_doc.nodes[k].y + 2;
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, we_form_click(&g_doc, x, y));
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "/alt87"));
    if (87 % 2) { TEST_ASSERT_EQUAL(1, q.post); }
    else { TEST_ASSERT_EQUAL(0, q.post); }
    l75b_done();
}

static void test_l75_ext_form_88(void) {
    we_form_req_t q;
    char html[700], sid[16];
    int k, x, y;
    sprintf(sid, "s88");
    sprintf(html,
        "<form id=f88 action=/e88 method=%s>"
        "<input name=n88 value=v88 form=f88>"
        "<input id=s88 type=submit formaction=/alt88 value=Go></form>",
        (88 % 2) ? "POST" : "GET");
    l75b_render(html);
    TEST_ASSERT_TRUE(g_doc.nodes[l75b_node(sid)].form_flags2 & WE_FF2_FORMACTION);
    for (k = 1; k < (int)g_doc.nnodes; k++)
        if (g_doc.nodes[k].form_type == WE_FORM_SUBMIT) break;
    x = g_doc.nodes[k].x + 2; y = g_doc.nodes[k].y + 2;
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, we_form_click(&g_doc, x, y));
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "/alt88"));
    if (88 % 2) { TEST_ASSERT_EQUAL(1, q.post); }
    else { TEST_ASSERT_EQUAL(0, q.post); }
    l75b_done();
}

static void test_l75_ext_form_89(void) {
    we_form_req_t q;
    char html[700], sid[16];
    int k, x, y;
    sprintf(sid, "s89");
    sprintf(html,
        "<form id=f89 action=/e89 method=%s>"
        "<input name=n89 value=v89 form=f89>"
        "<input id=s89 type=submit formaction=/alt89 value=Go></form>",
        (89 % 2) ? "POST" : "GET");
    l75b_render(html);
    TEST_ASSERT_TRUE(g_doc.nodes[l75b_node(sid)].form_flags2 & WE_FF2_FORMACTION);
    for (k = 1; k < (int)g_doc.nnodes; k++)
        if (g_doc.nodes[k].form_type == WE_FORM_SUBMIT) break;
    x = g_doc.nodes[k].x + 2; y = g_doc.nodes[k].y + 2;
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, we_form_click(&g_doc, x, y));
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "/alt89"));
    if (89 % 2) { TEST_ASSERT_EQUAL(1, q.post); }
    else { TEST_ASSERT_EQUAL(0, q.post); }
    l75b_done();
}

static void test_l75_ext_form_90(void) {
    we_form_req_t q;
    char html[700], sid[16];
    int k, x, y;
    sprintf(sid, "s90");
    sprintf(html,
        "<form id=f90 action=/e90 method=%s>"
        "<input name=n90 value=v90 form=f90>"
        "<input id=s90 type=submit formaction=/alt90 value=Go></form>",
        (90 % 2) ? "POST" : "GET");
    l75b_render(html);
    TEST_ASSERT_TRUE(g_doc.nodes[l75b_node(sid)].form_flags2 & WE_FF2_FORMACTION);
    for (k = 1; k < (int)g_doc.nnodes; k++)
        if (g_doc.nodes[k].form_type == WE_FORM_SUBMIT) break;
    x = g_doc.nodes[k].x + 2; y = g_doc.nodes[k].y + 2;
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, we_form_click(&g_doc, x, y));
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "/alt90"));
    if (90 % 2) { TEST_ASSERT_EQUAL(1, q.post); }
    else { TEST_ASSERT_EQUAL(0, q.post); }
    l75b_done();
}

static void test_l75_ext_form_91(void) {
    we_form_req_t q;
    char html[700], sid[16];
    int k, x, y;
    sprintf(sid, "s91");
    sprintf(html,
        "<form id=f91 action=/e91 method=%s>"
        "<input name=n91 value=v91 form=f91>"
        "<input id=s91 type=submit formaction=/alt91 value=Go></form>",
        (91 % 2) ? "POST" : "GET");
    l75b_render(html);
    TEST_ASSERT_TRUE(g_doc.nodes[l75b_node(sid)].form_flags2 & WE_FF2_FORMACTION);
    for (k = 1; k < (int)g_doc.nnodes; k++)
        if (g_doc.nodes[k].form_type == WE_FORM_SUBMIT) break;
    x = g_doc.nodes[k].x + 2; y = g_doc.nodes[k].y + 2;
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, we_form_click(&g_doc, x, y));
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "/alt91"));
    if (91 % 2) { TEST_ASSERT_EQUAL(1, q.post); }
    else { TEST_ASSERT_EQUAL(0, q.post); }
    l75b_done();
}

static void test_l75_ext_form_92(void) {
    we_form_req_t q;
    char html[700], sid[16];
    int k, x, y;
    sprintf(sid, "s92");
    sprintf(html,
        "<form id=f92 action=/e92 method=%s>"
        "<input name=n92 value=v92 form=f92>"
        "<input id=s92 type=submit formaction=/alt92 value=Go></form>",
        (92 % 2) ? "POST" : "GET");
    l75b_render(html);
    TEST_ASSERT_TRUE(g_doc.nodes[l75b_node(sid)].form_flags2 & WE_FF2_FORMACTION);
    for (k = 1; k < (int)g_doc.nnodes; k++)
        if (g_doc.nodes[k].form_type == WE_FORM_SUBMIT) break;
    x = g_doc.nodes[k].x + 2; y = g_doc.nodes[k].y + 2;
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, we_form_click(&g_doc, x, y));
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "/alt92"));
    if (92 % 2) { TEST_ASSERT_EQUAL(1, q.post); }
    else { TEST_ASSERT_EQUAL(0, q.post); }
    l75b_done();
}

static void test_l75_ext_form_93(void) {
    we_form_req_t q;
    char html[700], sid[16];
    int k, x, y;
    sprintf(sid, "s93");
    sprintf(html,
        "<form id=f93 action=/e93 method=%s>"
        "<input name=n93 value=v93 form=f93>"
        "<input id=s93 type=submit formaction=/alt93 value=Go></form>",
        (93 % 2) ? "POST" : "GET");
    l75b_render(html);
    TEST_ASSERT_TRUE(g_doc.nodes[l75b_node(sid)].form_flags2 & WE_FF2_FORMACTION);
    for (k = 1; k < (int)g_doc.nnodes; k++)
        if (g_doc.nodes[k].form_type == WE_FORM_SUBMIT) break;
    x = g_doc.nodes[k].x + 2; y = g_doc.nodes[k].y + 2;
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, we_form_click(&g_doc, x, y));
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "/alt93"));
    if (93 % 2) { TEST_ASSERT_EQUAL(1, q.post); }
    else { TEST_ASSERT_EQUAL(0, q.post); }
    l75b_done();
}

static void test_l75_ext_form_94(void) {
    we_form_req_t q;
    char html[700], sid[16];
    int k, x, y;
    sprintf(sid, "s94");
    sprintf(html,
        "<form id=f94 action=/e94 method=%s>"
        "<input name=n94 value=v94 form=f94>"
        "<input id=s94 type=submit formaction=/alt94 value=Go></form>",
        (94 % 2) ? "POST" : "GET");
    l75b_render(html);
    TEST_ASSERT_TRUE(g_doc.nodes[l75b_node(sid)].form_flags2 & WE_FF2_FORMACTION);
    for (k = 1; k < (int)g_doc.nnodes; k++)
        if (g_doc.nodes[k].form_type == WE_FORM_SUBMIT) break;
    x = g_doc.nodes[k].x + 2; y = g_doc.nodes[k].y + 2;
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, we_form_click(&g_doc, x, y));
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "/alt94"));
    if (94 % 2) { TEST_ASSERT_EQUAL(1, q.post); }
    else { TEST_ASSERT_EQUAL(0, q.post); }
    l75b_done();
}

static void test_l75_ext_form_95(void) {
    we_form_req_t q;
    char html[700], sid[16];
    int k, x, y;
    sprintf(sid, "s95");
    sprintf(html,
        "<form id=f95 action=/e95 method=%s>"
        "<input name=n95 value=v95 form=f95>"
        "<input id=s95 type=submit formaction=/alt95 value=Go></form>",
        (95 % 2) ? "POST" : "GET");
    l75b_render(html);
    TEST_ASSERT_TRUE(g_doc.nodes[l75b_node(sid)].form_flags2 & WE_FF2_FORMACTION);
    for (k = 1; k < (int)g_doc.nnodes; k++)
        if (g_doc.nodes[k].form_type == WE_FORM_SUBMIT) break;
    x = g_doc.nodes[k].x + 2; y = g_doc.nodes[k].y + 2;
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, we_form_click(&g_doc, x, y));
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "/alt95"));
    if (95 % 2) { TEST_ASSERT_EQUAL(1, q.post); }
    else { TEST_ASSERT_EQUAL(0, q.post); }
    l75b_done();
}

static void test_l75_ext_form_96(void) {
    we_form_req_t q;
    char html[700], sid[16];
    int k, x, y;
    sprintf(sid, "s96");
    sprintf(html,
        "<form id=f96 action=/e96 method=%s>"
        "<input name=n96 value=v96 form=f96>"
        "<input id=s96 type=submit formaction=/alt96 value=Go></form>",
        (96 % 2) ? "POST" : "GET");
    l75b_render(html);
    TEST_ASSERT_TRUE(g_doc.nodes[l75b_node(sid)].form_flags2 & WE_FF2_FORMACTION);
    for (k = 1; k < (int)g_doc.nnodes; k++)
        if (g_doc.nodes[k].form_type == WE_FORM_SUBMIT) break;
    x = g_doc.nodes[k].x + 2; y = g_doc.nodes[k].y + 2;
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, we_form_click(&g_doc, x, y));
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "/alt96"));
    if (96 % 2) { TEST_ASSERT_EQUAL(1, q.post); }
    else { TEST_ASSERT_EQUAL(0, q.post); }
    l75b_done();
}

static void test_l75_ext_form_97(void) {
    we_form_req_t q;
    char html[700], sid[16];
    int k, x, y;
    sprintf(sid, "s97");
    sprintf(html,
        "<form id=f97 action=/e97 method=%s>"
        "<input name=n97 value=v97 form=f97>"
        "<input id=s97 type=submit formaction=/alt97 value=Go></form>",
        (97 % 2) ? "POST" : "GET");
    l75b_render(html);
    TEST_ASSERT_TRUE(g_doc.nodes[l75b_node(sid)].form_flags2 & WE_FF2_FORMACTION);
    for (k = 1; k < (int)g_doc.nnodes; k++)
        if (g_doc.nodes[k].form_type == WE_FORM_SUBMIT) break;
    x = g_doc.nodes[k].x + 2; y = g_doc.nodes[k].y + 2;
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, we_form_click(&g_doc, x, y));
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "/alt97"));
    if (97 % 2) { TEST_ASSERT_EQUAL(1, q.post); }
    else { TEST_ASSERT_EQUAL(0, q.post); }
    l75b_done();
}

static void test_l75_ext_form_98(void) {
    we_form_req_t q;
    char html[700], sid[16];
    int k, x, y;
    sprintf(sid, "s98");
    sprintf(html,
        "<form id=f98 action=/e98 method=%s>"
        "<input name=n98 value=v98 form=f98>"
        "<input id=s98 type=submit formaction=/alt98 value=Go></form>",
        (98 % 2) ? "POST" : "GET");
    l75b_render(html);
    TEST_ASSERT_TRUE(g_doc.nodes[l75b_node(sid)].form_flags2 & WE_FF2_FORMACTION);
    for (k = 1; k < (int)g_doc.nnodes; k++)
        if (g_doc.nodes[k].form_type == WE_FORM_SUBMIT) break;
    x = g_doc.nodes[k].x + 2; y = g_doc.nodes[k].y + 2;
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, we_form_click(&g_doc, x, y));
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "/alt98"));
    if (98 % 2) { TEST_ASSERT_EQUAL(1, q.post); }
    else { TEST_ASSERT_EQUAL(0, q.post); }
    l75b_done();
}

static void test_l75_ext_form_99(void) {
    we_form_req_t q;
    char html[700], sid[16];
    int k, x, y;
    sprintf(sid, "s99");
    sprintf(html,
        "<form id=f99 action=/e99 method=%s>"
        "<input name=n99 value=v99 form=f99>"
        "<input id=s99 type=submit formaction=/alt99 value=Go></form>",
        (99 % 2) ? "POST" : "GET");
    l75b_render(html);
    TEST_ASSERT_TRUE(g_doc.nodes[l75b_node(sid)].form_flags2 & WE_FF2_FORMACTION);
    for (k = 1; k < (int)g_doc.nnodes; k++)
        if (g_doc.nodes[k].form_type == WE_FORM_SUBMIT) break;
    x = g_doc.nodes[k].x + 2; y = g_doc.nodes[k].y + 2;
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, we_form_click(&g_doc, x, y));
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "/alt99"));
    if (99 % 2) { TEST_ASSERT_EQUAL(1, q.post); }
    else { TEST_ASSERT_EQUAL(0, q.post); }
    l75b_done();
}

static void test_l75_ext_form_100(void) {
    we_form_req_t q;
    char html[700], sid[16];
    int k, x, y;
    sprintf(sid, "s100");
    sprintf(html,
        "<form id=f100 action=/e100 method=%s>"
        "<input name=n100 value=v100 form=f100>"
        "<input id=s100 type=submit formaction=/alt100 value=Go></form>",
        (100 % 2) ? "POST" : "GET");
    l75b_render(html);
    TEST_ASSERT_TRUE(g_doc.nodes[l75b_node(sid)].form_flags2 & WE_FF2_FORMACTION);
    for (k = 1; k < (int)g_doc.nnodes; k++)
        if (g_doc.nodes[k].form_type == WE_FORM_SUBMIT) break;
    x = g_doc.nodes[k].x + 2; y = g_doc.nodes[k].y + 2;
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, we_form_click(&g_doc, x, y));
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_NOT_NULL(strstr(q.url, "/alt100"));
    if (100 % 2) { TEST_ASSERT_EQUAL(1, q.post); }
    else { TEST_ASSERT_EQUAL(0, q.post); }
    l75b_done();
}

static void test_l75_ext_css_1(void) {
    char html[400];
    sprintf(html, "<style>#x{scroll-margin-top:%dpx;scroll-margin-right:%dpx;"
                  "scroll-padding-left:%dpx;caret-color:#%02x%02x%02x;"
                  "user-select:%s;pointer-events:%s}</style><div id=x></div>",
            1, 1+1, 1+2, (1*3)&255, (1*5)&255, (1*7)&255,
            (1%3==0)?"none":(1%3==1)?"text":"all",
            (1%2)?"none":"auto");
    lot52_render(html, 320);
    TEST_ASSERT_EQUAL(1, lot52_node("x")->scroll_m_set);
    TEST_ASSERT_EQUAL(1, lot52_node("x")->scroll_m[0]);
    TEST_ASSERT_EQUAL(1+1, lot52_node("x")->scroll_m[1]);
    TEST_ASSERT_EQUAL(1, lot52_node("x")->scroll_p_set);
    TEST_ASSERT_EQUAL(1+2, lot52_node("x")->scroll_p[3]);
    TEST_ASSERT_EQUAL(1, lot52_node("x")->caret_set);
}

static void test_l75_ext_css_2(void) {
    char html[400];
    sprintf(html, "<style>#x{scroll-margin-top:%dpx;scroll-margin-right:%dpx;"
                  "scroll-padding-left:%dpx;caret-color:#%02x%02x%02x;"
                  "user-select:%s;pointer-events:%s}</style><div id=x></div>",
            2, 2+1, 2+2, (2*3)&255, (2*5)&255, (2*7)&255,
            (2%3==0)?"none":(2%3==1)?"text":"all",
            (2%2)?"none":"auto");
    lot52_render(html, 320);
    TEST_ASSERT_EQUAL(1, lot52_node("x")->scroll_m_set);
    TEST_ASSERT_EQUAL(2, lot52_node("x")->scroll_m[0]);
    TEST_ASSERT_EQUAL(2+1, lot52_node("x")->scroll_m[1]);
    TEST_ASSERT_EQUAL(1, lot52_node("x")->scroll_p_set);
    TEST_ASSERT_EQUAL(2+2, lot52_node("x")->scroll_p[3]);
    TEST_ASSERT_EQUAL(1, lot52_node("x")->caret_set);
}

static void test_l75_ext_css_3(void) {
    char html[400];
    sprintf(html, "<style>#x{scroll-margin-top:%dpx;scroll-margin-right:%dpx;"
                  "scroll-padding-left:%dpx;caret-color:#%02x%02x%02x;"
                  "user-select:%s;pointer-events:%s}</style><div id=x></div>",
            3, 3+1, 3+2, (3*3)&255, (3*5)&255, (3*7)&255,
            (3%3==0)?"none":(3%3==1)?"text":"all",
            (3%2)?"none":"auto");
    lot52_render(html, 320);
    TEST_ASSERT_EQUAL(1, lot52_node("x")->scroll_m_set);
    TEST_ASSERT_EQUAL(3, lot52_node("x")->scroll_m[0]);
    TEST_ASSERT_EQUAL(3+1, lot52_node("x")->scroll_m[1]);
    TEST_ASSERT_EQUAL(1, lot52_node("x")->scroll_p_set);
    TEST_ASSERT_EQUAL(3+2, lot52_node("x")->scroll_p[3]);
    TEST_ASSERT_EQUAL(1, lot52_node("x")->caret_set);
}

static void test_l75_ext_css_4(void) {
    char html[400];
    sprintf(html, "<style>#x{scroll-margin-top:%dpx;scroll-margin-right:%dpx;"
                  "scroll-padding-left:%dpx;caret-color:#%02x%02x%02x;"
                  "user-select:%s;pointer-events:%s}</style><div id=x></div>",
            4, 4+1, 4+2, (4*3)&255, (4*5)&255, (4*7)&255,
            (4%3==0)?"none":(4%3==1)?"text":"all",
            (4%2)?"none":"auto");
    lot52_render(html, 320);
    TEST_ASSERT_EQUAL(1, lot52_node("x")->scroll_m_set);
    TEST_ASSERT_EQUAL(4, lot52_node("x")->scroll_m[0]);
    TEST_ASSERT_EQUAL(4+1, lot52_node("x")->scroll_m[1]);
    TEST_ASSERT_EQUAL(1, lot52_node("x")->scroll_p_set);
    TEST_ASSERT_EQUAL(4+2, lot52_node("x")->scroll_p[3]);
    TEST_ASSERT_EQUAL(1, lot52_node("x")->caret_set);
}

static void test_l75_ext_css_5(void) {
    char html[400];
    sprintf(html, "<style>#x{scroll-margin-top:%dpx;scroll-margin-right:%dpx;"
                  "scroll-padding-left:%dpx;caret-color:#%02x%02x%02x;"
                  "user-select:%s;pointer-events:%s}</style><div id=x></div>",
            5, 5+1, 5+2, (5*3)&255, (5*5)&255, (5*7)&255,
            (5%3==0)?"none":(5%3==1)?"text":"all",
            (5%2)?"none":"auto");
    lot52_render(html, 320);
    TEST_ASSERT_EQUAL(1, lot52_node("x")->scroll_m_set);
    TEST_ASSERT_EQUAL(5, lot52_node("x")->scroll_m[0]);
    TEST_ASSERT_EQUAL(5+1, lot52_node("x")->scroll_m[1]);
    TEST_ASSERT_EQUAL(1, lot52_node("x")->scroll_p_set);
    TEST_ASSERT_EQUAL(5+2, lot52_node("x")->scroll_p[3]);
    TEST_ASSERT_EQUAL(1, lot52_node("x")->caret_set);
}

static void test_l75_ext_css_6(void) {
    char html[400];
    sprintf(html, "<style>#x{scroll-margin-top:%dpx;scroll-margin-right:%dpx;"
                  "scroll-padding-left:%dpx;caret-color:#%02x%02x%02x;"
                  "user-select:%s;pointer-events:%s}</style><div id=x></div>",
            6, 6+1, 6+2, (6*3)&255, (6*5)&255, (6*7)&255,
            (6%3==0)?"none":(6%3==1)?"text":"all",
            (6%2)?"none":"auto");
    lot52_render(html, 320);
    TEST_ASSERT_EQUAL(1, lot52_node("x")->scroll_m_set);
    TEST_ASSERT_EQUAL(6, lot52_node("x")->scroll_m[0]);
    TEST_ASSERT_EQUAL(6+1, lot52_node("x")->scroll_m[1]);
    TEST_ASSERT_EQUAL(1, lot52_node("x")->scroll_p_set);
    TEST_ASSERT_EQUAL(6+2, lot52_node("x")->scroll_p[3]);
    TEST_ASSERT_EQUAL(1, lot52_node("x")->caret_set);
}

static void test_l75_ext_css_7(void) {
    char html[400];
    sprintf(html, "<style>#x{scroll-margin-top:%dpx;scroll-margin-right:%dpx;"
                  "scroll-padding-left:%dpx;caret-color:#%02x%02x%02x;"
                  "user-select:%s;pointer-events:%s}</style><div id=x></div>",
            7, 7+1, 7+2, (7*3)&255, (7*5)&255, (7*7)&255,
            (7%3==0)?"none":(7%3==1)?"text":"all",
            (7%2)?"none":"auto");
    lot52_render(html, 320);
    TEST_ASSERT_EQUAL(1, lot52_node("x")->scroll_m_set);
    TEST_ASSERT_EQUAL(7, lot52_node("x")->scroll_m[0]);
    TEST_ASSERT_EQUAL(7+1, lot52_node("x")->scroll_m[1]);
    TEST_ASSERT_EQUAL(1, lot52_node("x")->scroll_p_set);
    TEST_ASSERT_EQUAL(7+2, lot52_node("x")->scroll_p[3]);
    TEST_ASSERT_EQUAL(1, lot52_node("x")->caret_set);
}

static void test_l75_ext_css_8(void) {
    char html[400];
    sprintf(html, "<style>#x{scroll-margin-top:%dpx;scroll-margin-right:%dpx;"
                  "scroll-padding-left:%dpx;caret-color:#%02x%02x%02x;"
                  "user-select:%s;pointer-events:%s}</style><div id=x></div>",
            8, 8+1, 8+2, (8*3)&255, (8*5)&255, (8*7)&255,
            (8%3==0)?"none":(8%3==1)?"text":"all",
            (8%2)?"none":"auto");
    lot52_render(html, 320);
    TEST_ASSERT_EQUAL(1, lot52_node("x")->scroll_m_set);
    TEST_ASSERT_EQUAL(8, lot52_node("x")->scroll_m[0]);
    TEST_ASSERT_EQUAL(8+1, lot52_node("x")->scroll_m[1]);
    TEST_ASSERT_EQUAL(1, lot52_node("x")->scroll_p_set);
    TEST_ASSERT_EQUAL(8+2, lot52_node("x")->scroll_p[3]);
    TEST_ASSERT_EQUAL(1, lot52_node("x")->caret_set);
}

static void test_l75_ext_css_9(void) {
    char html[400];
    sprintf(html, "<style>#x{scroll-margin-top:%dpx;scroll-margin-right:%dpx;"
                  "scroll-padding-left:%dpx;caret-color:#%02x%02x%02x;"
                  "user-select:%s;pointer-events:%s}</style><div id=x></div>",
            9, 9+1, 9+2, (9*3)&255, (9*5)&255, (9*7)&255,
            (9%3==0)?"none":(9%3==1)?"text":"all",
            (9%2)?"none":"auto");
    lot52_render(html, 320);
    TEST_ASSERT_EQUAL(1, lot52_node("x")->scroll_m_set);
    TEST_ASSERT_EQUAL(9, lot52_node("x")->scroll_m[0]);
    TEST_ASSERT_EQUAL(9+1, lot52_node("x")->scroll_m[1]);
    TEST_ASSERT_EQUAL(1, lot52_node("x")->scroll_p_set);
    TEST_ASSERT_EQUAL(9+2, lot52_node("x")->scroll_p[3]);
    TEST_ASSERT_EQUAL(1, lot52_node("x")->caret_set);
}

static void test_l75_ext_css_10(void) {
    char html[400];
    sprintf(html, "<style>#x{scroll-margin-top:%dpx;scroll-margin-right:%dpx;"
                  "scroll-padding-left:%dpx;caret-color:#%02x%02x%02x;"
                  "user-select:%s;pointer-events:%s}</style><div id=x></div>",
            10, 10+1, 10+2, (10*3)&255, (10*5)&255, (10*7)&255,
            (10%3==0)?"none":(10%3==1)?"text":"all",
            (10%2)?"none":"auto");
    lot52_render(html, 320);
    TEST_ASSERT_EQUAL(1, lot52_node("x")->scroll_m_set);
    TEST_ASSERT_EQUAL(10, lot52_node("x")->scroll_m[0]);
    TEST_ASSERT_EQUAL(10+1, lot52_node("x")->scroll_m[1]);
    TEST_ASSERT_EQUAL(1, lot52_node("x")->scroll_p_set);
    TEST_ASSERT_EQUAL(10+2, lot52_node("x")->scroll_p[3]);
    TEST_ASSERT_EQUAL(1, lot52_node("x")->caret_set);
}

static void test_l75_ext_css_11(void) {
    char html[400];
    sprintf(html, "<style>#x{scroll-margin-top:%dpx;scroll-margin-right:%dpx;"
                  "scroll-padding-left:%dpx;caret-color:#%02x%02x%02x;"
                  "user-select:%s;pointer-events:%s}</style><div id=x></div>",
            11, 11+1, 11+2, (11*3)&255, (11*5)&255, (11*7)&255,
            (11%3==0)?"none":(11%3==1)?"text":"all",
            (11%2)?"none":"auto");
    lot52_render(html, 320);
    TEST_ASSERT_EQUAL(1, lot52_node("x")->scroll_m_set);
    TEST_ASSERT_EQUAL(11, lot52_node("x")->scroll_m[0]);
    TEST_ASSERT_EQUAL(11+1, lot52_node("x")->scroll_m[1]);
    TEST_ASSERT_EQUAL(1, lot52_node("x")->scroll_p_set);
    TEST_ASSERT_EQUAL(11+2, lot52_node("x")->scroll_p[3]);
    TEST_ASSERT_EQUAL(1, lot52_node("x")->caret_set);
}

static void test_l75_ext_css_12(void) {
    char html[400];
    sprintf(html, "<style>#x{scroll-margin-top:%dpx;scroll-margin-right:%dpx;"
                  "scroll-padding-left:%dpx;caret-color:#%02x%02x%02x;"
                  "user-select:%s;pointer-events:%s}</style><div id=x></div>",
            12, 12+1, 12+2, (12*3)&255, (12*5)&255, (12*7)&255,
            (12%3==0)?"none":(12%3==1)?"text":"all",
            (12%2)?"none":"auto");
    lot52_render(html, 320);
    TEST_ASSERT_EQUAL(1, lot52_node("x")->scroll_m_set);
    TEST_ASSERT_EQUAL(12, lot52_node("x")->scroll_m[0]);
    TEST_ASSERT_EQUAL(12+1, lot52_node("x")->scroll_m[1]);
    TEST_ASSERT_EQUAL(1, lot52_node("x")->scroll_p_set);
    TEST_ASSERT_EQUAL(12+2, lot52_node("x")->scroll_p[3]);
    TEST_ASSERT_EQUAL(1, lot52_node("x")->caret_set);
}

static void test_l75_ext_css_13(void) {
    char html[400];
    sprintf(html, "<style>#x{scroll-margin-top:%dpx;scroll-margin-right:%dpx;"
                  "scroll-padding-left:%dpx;caret-color:#%02x%02x%02x;"
                  "user-select:%s;pointer-events:%s}</style><div id=x></div>",
            13, 13+1, 13+2, (13*3)&255, (13*5)&255, (13*7)&255,
            (13%3==0)?"none":(13%3==1)?"text":"all",
            (13%2)?"none":"auto");
    lot52_render(html, 320);
    TEST_ASSERT_EQUAL(1, lot52_node("x")->scroll_m_set);
    TEST_ASSERT_EQUAL(13, lot52_node("x")->scroll_m[0]);
    TEST_ASSERT_EQUAL(13+1, lot52_node("x")->scroll_m[1]);
    TEST_ASSERT_EQUAL(1, lot52_node("x")->scroll_p_set);
    TEST_ASSERT_EQUAL(13+2, lot52_node("x")->scroll_p[3]);
    TEST_ASSERT_EQUAL(1, lot52_node("x")->caret_set);
}

static void test_l75_ext_css_14(void) {
    char html[400];
    sprintf(html, "<style>#x{scroll-margin-top:%dpx;scroll-margin-right:%dpx;"
                  "scroll-padding-left:%dpx;caret-color:#%02x%02x%02x;"
                  "user-select:%s;pointer-events:%s}</style><div id=x></div>",
            14, 14+1, 14+2, (14*3)&255, (14*5)&255, (14*7)&255,
            (14%3==0)?"none":(14%3==1)?"text":"all",
            (14%2)?"none":"auto");
    lot52_render(html, 320);
    TEST_ASSERT_EQUAL(1, lot52_node("x")->scroll_m_set);
    TEST_ASSERT_EQUAL(14, lot52_node("x")->scroll_m[0]);
    TEST_ASSERT_EQUAL(14+1, lot52_node("x")->scroll_m[1]);
    TEST_ASSERT_EQUAL(1, lot52_node("x")->scroll_p_set);
    TEST_ASSERT_EQUAL(14+2, lot52_node("x")->scroll_p[3]);
    TEST_ASSERT_EQUAL(1, lot52_node("x")->caret_set);
}

static void test_l75_ext_css_15(void) {
    char html[400];
    sprintf(html, "<style>#x{scroll-margin-top:%dpx;scroll-margin-right:%dpx;"
                  "scroll-padding-left:%dpx;caret-color:#%02x%02x%02x;"
                  "user-select:%s;pointer-events:%s}</style><div id=x></div>",
            15, 15+1, 15+2, (15*3)&255, (15*5)&255, (15*7)&255,
            (15%3==0)?"none":(15%3==1)?"text":"all",
            (15%2)?"none":"auto");
    lot52_render(html, 320);
    TEST_ASSERT_EQUAL(1, lot52_node("x")->scroll_m_set);
    TEST_ASSERT_EQUAL(15, lot52_node("x")->scroll_m[0]);
    TEST_ASSERT_EQUAL(15+1, lot52_node("x")->scroll_m[1]);
    TEST_ASSERT_EQUAL(1, lot52_node("x")->scroll_p_set);
    TEST_ASSERT_EQUAL(15+2, lot52_node("x")->scroll_p[3]);
    TEST_ASSERT_EQUAL(1, lot52_node("x")->caret_set);
}

static void test_l75_ext_css_16(void) {
    char html[400];
    sprintf(html, "<style>#x{scroll-margin-top:%dpx;scroll-margin-right:%dpx;"
                  "scroll-padding-left:%dpx;caret-color:#%02x%02x%02x;"
                  "user-select:%s;pointer-events:%s}</style><div id=x></div>",
            16, 16+1, 16+2, (16*3)&255, (16*5)&255, (16*7)&255,
            (16%3==0)?"none":(16%3==1)?"text":"all",
            (16%2)?"none":"auto");
    lot52_render(html, 320);
    TEST_ASSERT_EQUAL(1, lot52_node("x")->scroll_m_set);
    TEST_ASSERT_EQUAL(16, lot52_node("x")->scroll_m[0]);
    TEST_ASSERT_EQUAL(16+1, lot52_node("x")->scroll_m[1]);
    TEST_ASSERT_EQUAL(1, lot52_node("x")->scroll_p_set);
    TEST_ASSERT_EQUAL(16+2, lot52_node("x")->scroll_p[3]);
    TEST_ASSERT_EQUAL(1, lot52_node("x")->caret_set);
}

static void test_l75_ext_css_17(void) {
    char html[400];
    sprintf(html, "<style>#x{scroll-margin-top:%dpx;scroll-margin-right:%dpx;"
                  "scroll-padding-left:%dpx;caret-color:#%02x%02x%02x;"
                  "user-select:%s;pointer-events:%s}</style><div id=x></div>",
            17, 17+1, 17+2, (17*3)&255, (17*5)&255, (17*7)&255,
            (17%3==0)?"none":(17%3==1)?"text":"all",
            (17%2)?"none":"auto");
    lot52_render(html, 320);
    TEST_ASSERT_EQUAL(1, lot52_node("x")->scroll_m_set);
    TEST_ASSERT_EQUAL(17, lot52_node("x")->scroll_m[0]);
    TEST_ASSERT_EQUAL(17+1, lot52_node("x")->scroll_m[1]);
    TEST_ASSERT_EQUAL(1, lot52_node("x")->scroll_p_set);
    TEST_ASSERT_EQUAL(17+2, lot52_node("x")->scroll_p[3]);
    TEST_ASSERT_EQUAL(1, lot52_node("x")->caret_set);
}

static void test_l75_ext_css_18(void) {
    char html[400];
    sprintf(html, "<style>#x{scroll-margin-top:%dpx;scroll-margin-right:%dpx;"
                  "scroll-padding-left:%dpx;caret-color:#%02x%02x%02x;"
                  "user-select:%s;pointer-events:%s}</style><div id=x></div>",
            18, 18+1, 18+2, (18*3)&255, (18*5)&255, (18*7)&255,
            (18%3==0)?"none":(18%3==1)?"text":"all",
            (18%2)?"none":"auto");
    lot52_render(html, 320);
    TEST_ASSERT_EQUAL(1, lot52_node("x")->scroll_m_set);
    TEST_ASSERT_EQUAL(18, lot52_node("x")->scroll_m[0]);
    TEST_ASSERT_EQUAL(18+1, lot52_node("x")->scroll_m[1]);
    TEST_ASSERT_EQUAL(1, lot52_node("x")->scroll_p_set);
    TEST_ASSERT_EQUAL(18+2, lot52_node("x")->scroll_p[3]);
    TEST_ASSERT_EQUAL(1, lot52_node("x")->caret_set);
}

static void test_l75_ext_css_19(void) {
    char html[400];
    sprintf(html, "<style>#x{scroll-margin-top:%dpx;scroll-margin-right:%dpx;"
                  "scroll-padding-left:%dpx;caret-color:#%02x%02x%02x;"
                  "user-select:%s;pointer-events:%s}</style><div id=x></div>",
            19, 19+1, 19+2, (19*3)&255, (19*5)&255, (19*7)&255,
            (19%3==0)?"none":(19%3==1)?"text":"all",
            (19%2)?"none":"auto");
    lot52_render(html, 320);
    TEST_ASSERT_EQUAL(1, lot52_node("x")->scroll_m_set);
    TEST_ASSERT_EQUAL(19, lot52_node("x")->scroll_m[0]);
    TEST_ASSERT_EQUAL(19+1, lot52_node("x")->scroll_m[1]);
    TEST_ASSERT_EQUAL(1, lot52_node("x")->scroll_p_set);
    TEST_ASSERT_EQUAL(19+2, lot52_node("x")->scroll_p[3]);
    TEST_ASSERT_EQUAL(1, lot52_node("x")->caret_set);
}

static void test_l75_ext_css_20(void) {
    char html[400];
    sprintf(html, "<style>#x{scroll-margin-top:%dpx;scroll-margin-right:%dpx;"
                  "scroll-padding-left:%dpx;caret-color:#%02x%02x%02x;"
                  "user-select:%s;pointer-events:%s}</style><div id=x></div>",
            20, 20+1, 20+2, (20*3)&255, (20*5)&255, (20*7)&255,
            (20%3==0)?"none":(20%3==1)?"text":"all",
            (20%2)?"none":"auto");
    lot52_render(html, 320);
    TEST_ASSERT_EQUAL(1, lot52_node("x")->scroll_m_set);
    TEST_ASSERT_EQUAL(20, lot52_node("x")->scroll_m[0]);
    TEST_ASSERT_EQUAL(20+1, lot52_node("x")->scroll_m[1]);
    TEST_ASSERT_EQUAL(1, lot52_node("x")->scroll_p_set);
    TEST_ASSERT_EQUAL(20+2, lot52_node("x")->scroll_p[3]);
    TEST_ASSERT_EQUAL(1, lot52_node("x")->caret_set);
}

static void test_l75_ext_css_21(void) {
    char html[400];
    sprintf(html, "<style>#x{scroll-margin-top:%dpx;scroll-margin-right:%dpx;"
                  "scroll-padding-left:%dpx;caret-color:#%02x%02x%02x;"
                  "user-select:%s;pointer-events:%s}</style><div id=x></div>",
            21, 21+1, 21+2, (21*3)&255, (21*5)&255, (21*7)&255,
            (21%3==0)?"none":(21%3==1)?"text":"all",
            (21%2)?"none":"auto");
    lot52_render(html, 320);
    TEST_ASSERT_EQUAL(1, lot52_node("x")->scroll_m_set);
    TEST_ASSERT_EQUAL(21, lot52_node("x")->scroll_m[0]);
    TEST_ASSERT_EQUAL(21+1, lot52_node("x")->scroll_m[1]);
    TEST_ASSERT_EQUAL(1, lot52_node("x")->scroll_p_set);
    TEST_ASSERT_EQUAL(21+2, lot52_node("x")->scroll_p[3]);
    TEST_ASSERT_EQUAL(1, lot52_node("x")->caret_set);
}

static void test_l75_ext_css_22(void) {
    char html[400];
    sprintf(html, "<style>#x{scroll-margin-top:%dpx;scroll-margin-right:%dpx;"
                  "scroll-padding-left:%dpx;caret-color:#%02x%02x%02x;"
                  "user-select:%s;pointer-events:%s}</style><div id=x></div>",
            22, 22+1, 22+2, (22*3)&255, (22*5)&255, (22*7)&255,
            (22%3==0)?"none":(22%3==1)?"text":"all",
            (22%2)?"none":"auto");
    lot52_render(html, 320);
    TEST_ASSERT_EQUAL(1, lot52_node("x")->scroll_m_set);
    TEST_ASSERT_EQUAL(22, lot52_node("x")->scroll_m[0]);
    TEST_ASSERT_EQUAL(22+1, lot52_node("x")->scroll_m[1]);
    TEST_ASSERT_EQUAL(1, lot52_node("x")->scroll_p_set);
    TEST_ASSERT_EQUAL(22+2, lot52_node("x")->scroll_p[3]);
    TEST_ASSERT_EQUAL(1, lot52_node("x")->caret_set);
}

static void test_l75_ext_css_23(void) {
    char html[400];
    sprintf(html, "<style>#x{scroll-margin-top:%dpx;scroll-margin-right:%dpx;"
                  "scroll-padding-left:%dpx;caret-color:#%02x%02x%02x;"
                  "user-select:%s;pointer-events:%s}</style><div id=x></div>",
            23, 23+1, 23+2, (23*3)&255, (23*5)&255, (23*7)&255,
            (23%3==0)?"none":(23%3==1)?"text":"all",
            (23%2)?"none":"auto");
    lot52_render(html, 320);
    TEST_ASSERT_EQUAL(1, lot52_node("x")->scroll_m_set);
    TEST_ASSERT_EQUAL(23, lot52_node("x")->scroll_m[0]);
    TEST_ASSERT_EQUAL(23+1, lot52_node("x")->scroll_m[1]);
    TEST_ASSERT_EQUAL(1, lot52_node("x")->scroll_p_set);
    TEST_ASSERT_EQUAL(23+2, lot52_node("x")->scroll_p[3]);
    TEST_ASSERT_EQUAL(1, lot52_node("x")->caret_set);
}

static void test_l75_ext_css_24(void) {
    char html[400];
    sprintf(html, "<style>#x{scroll-margin-top:%dpx;scroll-margin-right:%dpx;"
                  "scroll-padding-left:%dpx;caret-color:#%02x%02x%02x;"
                  "user-select:%s;pointer-events:%s}</style><div id=x></div>",
            24, 24+1, 24+2, (24*3)&255, (24*5)&255, (24*7)&255,
            (24%3==0)?"none":(24%3==1)?"text":"all",
            (24%2)?"none":"auto");
    lot52_render(html, 320);
    TEST_ASSERT_EQUAL(1, lot52_node("x")->scroll_m_set);
    TEST_ASSERT_EQUAL(24, lot52_node("x")->scroll_m[0]);
    TEST_ASSERT_EQUAL(24+1, lot52_node("x")->scroll_m[1]);
    TEST_ASSERT_EQUAL(1, lot52_node("x")->scroll_p_set);
    TEST_ASSERT_EQUAL(24+2, lot52_node("x")->scroll_p[3]);
    TEST_ASSERT_EQUAL(1, lot52_node("x")->caret_set);
}

static void test_l75_ext_css_25(void) {
    char html[400];
    sprintf(html, "<style>#x{scroll-margin-top:%dpx;scroll-margin-right:%dpx;"
                  "scroll-padding-left:%dpx;caret-color:#%02x%02x%02x;"
                  "user-select:%s;pointer-events:%s}</style><div id=x></div>",
            25, 25+1, 25+2, (25*3)&255, (25*5)&255, (25*7)&255,
            (25%3==0)?"none":(25%3==1)?"text":"all",
            (25%2)?"none":"auto");
    lot52_render(html, 320);
    TEST_ASSERT_EQUAL(1, lot52_node("x")->scroll_m_set);
    TEST_ASSERT_EQUAL(25, lot52_node("x")->scroll_m[0]);
    TEST_ASSERT_EQUAL(25+1, lot52_node("x")->scroll_m[1]);
    TEST_ASSERT_EQUAL(1, lot52_node("x")->scroll_p_set);
    TEST_ASSERT_EQUAL(25+2, lot52_node("x")->scroll_p[3]);
    TEST_ASSERT_EQUAL(1, lot52_node("x")->caret_set);
}

static void test_l75_ext_css_26(void) {
    char html[400];
    sprintf(html, "<style>#x{scroll-margin-top:%dpx;scroll-margin-right:%dpx;"
                  "scroll-padding-left:%dpx;caret-color:#%02x%02x%02x;"
                  "user-select:%s;pointer-events:%s}</style><div id=x></div>",
            26, 26+1, 26+2, (26*3)&255, (26*5)&255, (26*7)&255,
            (26%3==0)?"none":(26%3==1)?"text":"all",
            (26%2)?"none":"auto");
    lot52_render(html, 320);
    TEST_ASSERT_EQUAL(1, lot52_node("x")->scroll_m_set);
    TEST_ASSERT_EQUAL(26, lot52_node("x")->scroll_m[0]);
    TEST_ASSERT_EQUAL(26+1, lot52_node("x")->scroll_m[1]);
    TEST_ASSERT_EQUAL(1, lot52_node("x")->scroll_p_set);
    TEST_ASSERT_EQUAL(26+2, lot52_node("x")->scroll_p[3]);
    TEST_ASSERT_EQUAL(1, lot52_node("x")->caret_set);
}

static void test_l75_ext_css_27(void) {
    char html[400];
    sprintf(html, "<style>#x{scroll-margin-top:%dpx;scroll-margin-right:%dpx;"
                  "scroll-padding-left:%dpx;caret-color:#%02x%02x%02x;"
                  "user-select:%s;pointer-events:%s}</style><div id=x></div>",
            27, 27+1, 27+2, (27*3)&255, (27*5)&255, (27*7)&255,
            (27%3==0)?"none":(27%3==1)?"text":"all",
            (27%2)?"none":"auto");
    lot52_render(html, 320);
    TEST_ASSERT_EQUAL(1, lot52_node("x")->scroll_m_set);
    TEST_ASSERT_EQUAL(27, lot52_node("x")->scroll_m[0]);
    TEST_ASSERT_EQUAL(27+1, lot52_node("x")->scroll_m[1]);
    TEST_ASSERT_EQUAL(1, lot52_node("x")->scroll_p_set);
    TEST_ASSERT_EQUAL(27+2, lot52_node("x")->scroll_p[3]);
    TEST_ASSERT_EQUAL(1, lot52_node("x")->caret_set);
}

static void test_l75_ext_css_28(void) {
    char html[400];
    sprintf(html, "<style>#x{scroll-margin-top:%dpx;scroll-margin-right:%dpx;"
                  "scroll-padding-left:%dpx;caret-color:#%02x%02x%02x;"
                  "user-select:%s;pointer-events:%s}</style><div id=x></div>",
            28, 28+1, 28+2, (28*3)&255, (28*5)&255, (28*7)&255,
            (28%3==0)?"none":(28%3==1)?"text":"all",
            (28%2)?"none":"auto");
    lot52_render(html, 320);
    TEST_ASSERT_EQUAL(1, lot52_node("x")->scroll_m_set);
    TEST_ASSERT_EQUAL(28, lot52_node("x")->scroll_m[0]);
    TEST_ASSERT_EQUAL(28+1, lot52_node("x")->scroll_m[1]);
    TEST_ASSERT_EQUAL(1, lot52_node("x")->scroll_p_set);
    TEST_ASSERT_EQUAL(28+2, lot52_node("x")->scroll_p[3]);
    TEST_ASSERT_EQUAL(1, lot52_node("x")->caret_set);
}

static void test_l75_ext_css_29(void) {
    char html[400];
    sprintf(html, "<style>#x{scroll-margin-top:%dpx;scroll-margin-right:%dpx;"
                  "scroll-padding-left:%dpx;caret-color:#%02x%02x%02x;"
                  "user-select:%s;pointer-events:%s}</style><div id=x></div>",
            29, 29+1, 29+2, (29*3)&255, (29*5)&255, (29*7)&255,
            (29%3==0)?"none":(29%3==1)?"text":"all",
            (29%2)?"none":"auto");
    lot52_render(html, 320);
    TEST_ASSERT_EQUAL(1, lot52_node("x")->scroll_m_set);
    TEST_ASSERT_EQUAL(29, lot52_node("x")->scroll_m[0]);
    TEST_ASSERT_EQUAL(29+1, lot52_node("x")->scroll_m[1]);
    TEST_ASSERT_EQUAL(1, lot52_node("x")->scroll_p_set);
    TEST_ASSERT_EQUAL(29+2, lot52_node("x")->scroll_p[3]);
    TEST_ASSERT_EQUAL(1, lot52_node("x")->caret_set);
}

static void test_l75_ext_css_30(void) {
    char html[400];
    sprintf(html, "<style>#x{scroll-margin-top:%dpx;scroll-margin-right:%dpx;"
                  "scroll-padding-left:%dpx;caret-color:#%02x%02x%02x;"
                  "user-select:%s;pointer-events:%s}</style><div id=x></div>",
            30, 30+1, 30+2, (30*3)&255, (30*5)&255, (30*7)&255,
            (30%3==0)?"none":(30%3==1)?"text":"all",
            (30%2)?"none":"auto");
    lot52_render(html, 320);
    TEST_ASSERT_EQUAL(1, lot52_node("x")->scroll_m_set);
    TEST_ASSERT_EQUAL(30, lot52_node("x")->scroll_m[0]);
    TEST_ASSERT_EQUAL(30+1, lot52_node("x")->scroll_m[1]);
    TEST_ASSERT_EQUAL(1, lot52_node("x")->scroll_p_set);
    TEST_ASSERT_EQUAL(30+2, lot52_node("x")->scroll_p[3]);
    TEST_ASSERT_EQUAL(1, lot52_node("x")->caret_set);
}

static void test_l75_ext_css_31(void) {
    char html[400];
    sprintf(html, "<style>#x{scroll-margin-top:%dpx;scroll-margin-right:%dpx;"
                  "scroll-padding-left:%dpx;caret-color:#%02x%02x%02x;"
                  "user-select:%s;pointer-events:%s}</style><div id=x></div>",
            31, 31+1, 31+2, (31*3)&255, (31*5)&255, (31*7)&255,
            (31%3==0)?"none":(31%3==1)?"text":"all",
            (31%2)?"none":"auto");
    lot52_render(html, 320);
    TEST_ASSERT_EQUAL(1, lot52_node("x")->scroll_m_set);
    TEST_ASSERT_EQUAL(31, lot52_node("x")->scroll_m[0]);
    TEST_ASSERT_EQUAL(31+1, lot52_node("x")->scroll_m[1]);
    TEST_ASSERT_EQUAL(1, lot52_node("x")->scroll_p_set);
    TEST_ASSERT_EQUAL(31+2, lot52_node("x")->scroll_p[3]);
    TEST_ASSERT_EQUAL(1, lot52_node("x")->caret_set);
}

static void test_l75_ext_css_32(void) {
    char html[400];
    sprintf(html, "<style>#x{scroll-margin-top:%dpx;scroll-margin-right:%dpx;"
                  "scroll-padding-left:%dpx;caret-color:#%02x%02x%02x;"
                  "user-select:%s;pointer-events:%s}</style><div id=x></div>",
            32, 32+1, 32+2, (32*3)&255, (32*5)&255, (32*7)&255,
            (32%3==0)?"none":(32%3==1)?"text":"all",
            (32%2)?"none":"auto");
    lot52_render(html, 320);
    TEST_ASSERT_EQUAL(1, lot52_node("x")->scroll_m_set);
    TEST_ASSERT_EQUAL(32, lot52_node("x")->scroll_m[0]);
    TEST_ASSERT_EQUAL(32+1, lot52_node("x")->scroll_m[1]);
    TEST_ASSERT_EQUAL(1, lot52_node("x")->scroll_p_set);
    TEST_ASSERT_EQUAL(32+2, lot52_node("x")->scroll_p[3]);
    TEST_ASSERT_EQUAL(1, lot52_node("x")->caret_set);
}

static void test_l75_ext_css_33(void) {
    char html[400];
    sprintf(html, "<style>#x{scroll-margin-top:%dpx;scroll-margin-right:%dpx;"
                  "scroll-padding-left:%dpx;caret-color:#%02x%02x%02x;"
                  "user-select:%s;pointer-events:%s}</style><div id=x></div>",
            33, 33+1, 33+2, (33*3)&255, (33*5)&255, (33*7)&255,
            (33%3==0)?"none":(33%3==1)?"text":"all",
            (33%2)?"none":"auto");
    lot52_render(html, 320);
    TEST_ASSERT_EQUAL(1, lot52_node("x")->scroll_m_set);
    TEST_ASSERT_EQUAL(33, lot52_node("x")->scroll_m[0]);
    TEST_ASSERT_EQUAL(33+1, lot52_node("x")->scroll_m[1]);
    TEST_ASSERT_EQUAL(1, lot52_node("x")->scroll_p_set);
    TEST_ASSERT_EQUAL(33+2, lot52_node("x")->scroll_p[3]);
    TEST_ASSERT_EQUAL(1, lot52_node("x")->caret_set);
}

static void test_l75_ext_css_34(void) {
    char html[400];
    sprintf(html, "<style>#x{scroll-margin-top:%dpx;scroll-margin-right:%dpx;"
                  "scroll-padding-left:%dpx;caret-color:#%02x%02x%02x;"
                  "user-select:%s;pointer-events:%s}</style><div id=x></div>",
            34, 34+1, 34+2, (34*3)&255, (34*5)&255, (34*7)&255,
            (34%3==0)?"none":(34%3==1)?"text":"all",
            (34%2)?"none":"auto");
    lot52_render(html, 320);
    TEST_ASSERT_EQUAL(1, lot52_node("x")->scroll_m_set);
    TEST_ASSERT_EQUAL(34, lot52_node("x")->scroll_m[0]);
    TEST_ASSERT_EQUAL(34+1, lot52_node("x")->scroll_m[1]);
    TEST_ASSERT_EQUAL(1, lot52_node("x")->scroll_p_set);
    TEST_ASSERT_EQUAL(34+2, lot52_node("x")->scroll_p[3]);
    TEST_ASSERT_EQUAL(1, lot52_node("x")->caret_set);
}

static void test_l75_ext_css_35(void) {
    char html[400];
    sprintf(html, "<style>#x{scroll-margin-top:%dpx;scroll-margin-right:%dpx;"
                  "scroll-padding-left:%dpx;caret-color:#%02x%02x%02x;"
                  "user-select:%s;pointer-events:%s}</style><div id=x></div>",
            35, 35+1, 35+2, (35*3)&255, (35*5)&255, (35*7)&255,
            (35%3==0)?"none":(35%3==1)?"text":"all",
            (35%2)?"none":"auto");
    lot52_render(html, 320);
    TEST_ASSERT_EQUAL(1, lot52_node("x")->scroll_m_set);
    TEST_ASSERT_EQUAL(35, lot52_node("x")->scroll_m[0]);
    TEST_ASSERT_EQUAL(35+1, lot52_node("x")->scroll_m[1]);
    TEST_ASSERT_EQUAL(1, lot52_node("x")->scroll_p_set);
    TEST_ASSERT_EQUAL(35+2, lot52_node("x")->scroll_p[3]);
    TEST_ASSERT_EQUAL(1, lot52_node("x")->caret_set);
}

static void test_l75_ext_css_36(void) {
    char html[400];
    sprintf(html, "<style>#x{scroll-margin-top:%dpx;scroll-margin-right:%dpx;"
                  "scroll-padding-left:%dpx;caret-color:#%02x%02x%02x;"
                  "user-select:%s;pointer-events:%s}</style><div id=x></div>",
            36, 36+1, 36+2, (36*3)&255, (36*5)&255, (36*7)&255,
            (36%3==0)?"none":(36%3==1)?"text":"all",
            (36%2)?"none":"auto");
    lot52_render(html, 320);
    TEST_ASSERT_EQUAL(1, lot52_node("x")->scroll_m_set);
    TEST_ASSERT_EQUAL(36, lot52_node("x")->scroll_m[0]);
    TEST_ASSERT_EQUAL(36+1, lot52_node("x")->scroll_m[1]);
    TEST_ASSERT_EQUAL(1, lot52_node("x")->scroll_p_set);
    TEST_ASSERT_EQUAL(36+2, lot52_node("x")->scroll_p[3]);
    TEST_ASSERT_EQUAL(1, lot52_node("x")->caret_set);
}

static void test_l75_ext_css_37(void) {
    char html[400];
    sprintf(html, "<style>#x{scroll-margin-top:%dpx;scroll-margin-right:%dpx;"
                  "scroll-padding-left:%dpx;caret-color:#%02x%02x%02x;"
                  "user-select:%s;pointer-events:%s}</style><div id=x></div>",
            37, 37+1, 37+2, (37*3)&255, (37*5)&255, (37*7)&255,
            (37%3==0)?"none":(37%3==1)?"text":"all",
            (37%2)?"none":"auto");
    lot52_render(html, 320);
    TEST_ASSERT_EQUAL(1, lot52_node("x")->scroll_m_set);
    TEST_ASSERT_EQUAL(37, lot52_node("x")->scroll_m[0]);
    TEST_ASSERT_EQUAL(37+1, lot52_node("x")->scroll_m[1]);
    TEST_ASSERT_EQUAL(1, lot52_node("x")->scroll_p_set);
    TEST_ASSERT_EQUAL(37+2, lot52_node("x")->scroll_p[3]);
    TEST_ASSERT_EQUAL(1, lot52_node("x")->caret_set);
}

static void test_l75_ext_css_38(void) {
    char html[400];
    sprintf(html, "<style>#x{scroll-margin-top:%dpx;scroll-margin-right:%dpx;"
                  "scroll-padding-left:%dpx;caret-color:#%02x%02x%02x;"
                  "user-select:%s;pointer-events:%s}</style><div id=x></div>",
            38, 38+1, 38+2, (38*3)&255, (38*5)&255, (38*7)&255,
            (38%3==0)?"none":(38%3==1)?"text":"all",
            (38%2)?"none":"auto");
    lot52_render(html, 320);
    TEST_ASSERT_EQUAL(1, lot52_node("x")->scroll_m_set);
    TEST_ASSERT_EQUAL(38, lot52_node("x")->scroll_m[0]);
    TEST_ASSERT_EQUAL(38+1, lot52_node("x")->scroll_m[1]);
    TEST_ASSERT_EQUAL(1, lot52_node("x")->scroll_p_set);
    TEST_ASSERT_EQUAL(38+2, lot52_node("x")->scroll_p[3]);
    TEST_ASSERT_EQUAL(1, lot52_node("x")->caret_set);
}

static void test_l75_ext_css_39(void) {
    char html[400];
    sprintf(html, "<style>#x{scroll-margin-top:%dpx;scroll-margin-right:%dpx;"
                  "scroll-padding-left:%dpx;caret-color:#%02x%02x%02x;"
                  "user-select:%s;pointer-events:%s}</style><div id=x></div>",
            39, 39+1, 39+2, (39*3)&255, (39*5)&255, (39*7)&255,
            (39%3==0)?"none":(39%3==1)?"text":"all",
            (39%2)?"none":"auto");
    lot52_render(html, 320);
    TEST_ASSERT_EQUAL(1, lot52_node("x")->scroll_m_set);
    TEST_ASSERT_EQUAL(39, lot52_node("x")->scroll_m[0]);
    TEST_ASSERT_EQUAL(39+1, lot52_node("x")->scroll_m[1]);
    TEST_ASSERT_EQUAL(1, lot52_node("x")->scroll_p_set);
    TEST_ASSERT_EQUAL(39+2, lot52_node("x")->scroll_p[3]);
    TEST_ASSERT_EQUAL(1, lot52_node("x")->caret_set);
}

static void test_l75_ext_css_40(void) {
    char html[400];
    sprintf(html, "<style>#x{scroll-margin-top:%dpx;scroll-margin-right:%dpx;"
                  "scroll-padding-left:%dpx;caret-color:#%02x%02x%02x;"
                  "user-select:%s;pointer-events:%s}</style><div id=x></div>",
            40, 40+1, 40+2, (40*3)&255, (40*5)&255, (40*7)&255,
            (40%3==0)?"none":(40%3==1)?"text":"all",
            (40%2)?"none":"auto");
    lot52_render(html, 320);
    TEST_ASSERT_EQUAL(1, lot52_node("x")->scroll_m_set);
    TEST_ASSERT_EQUAL(40, lot52_node("x")->scroll_m[0]);
    TEST_ASSERT_EQUAL(40+1, lot52_node("x")->scroll_m[1]);
    TEST_ASSERT_EQUAL(1, lot52_node("x")->scroll_p_set);
    TEST_ASSERT_EQUAL(40+2, lot52_node("x")->scroll_p[3]);
    TEST_ASSERT_EQUAL(1, lot52_node("x")->caret_set);
}

static void test_l75_ext_css_41(void) {
    char html[400];
    sprintf(html, "<style>#x{scroll-margin-top:%dpx;scroll-margin-right:%dpx;"
                  "scroll-padding-left:%dpx;caret-color:#%02x%02x%02x;"
                  "user-select:%s;pointer-events:%s}</style><div id=x></div>",
            41, 41+1, 41+2, (41*3)&255, (41*5)&255, (41*7)&255,
            (41%3==0)?"none":(41%3==1)?"text":"all",
            (41%2)?"none":"auto");
    lot52_render(html, 320);
    TEST_ASSERT_EQUAL(1, lot52_node("x")->scroll_m_set);
    TEST_ASSERT_EQUAL(41, lot52_node("x")->scroll_m[0]);
    TEST_ASSERT_EQUAL(41+1, lot52_node("x")->scroll_m[1]);
    TEST_ASSERT_EQUAL(1, lot52_node("x")->scroll_p_set);
    TEST_ASSERT_EQUAL(41+2, lot52_node("x")->scroll_p[3]);
    TEST_ASSERT_EQUAL(1, lot52_node("x")->caret_set);
}

static void test_l75_ext_css_42(void) {
    char html[400];
    sprintf(html, "<style>#x{scroll-margin-top:%dpx;scroll-margin-right:%dpx;"
                  "scroll-padding-left:%dpx;caret-color:#%02x%02x%02x;"
                  "user-select:%s;pointer-events:%s}</style><div id=x></div>",
            42, 42+1, 42+2, (42*3)&255, (42*5)&255, (42*7)&255,
            (42%3==0)?"none":(42%3==1)?"text":"all",
            (42%2)?"none":"auto");
    lot52_render(html, 320);
    TEST_ASSERT_EQUAL(1, lot52_node("x")->scroll_m_set);
    TEST_ASSERT_EQUAL(42, lot52_node("x")->scroll_m[0]);
    TEST_ASSERT_EQUAL(42+1, lot52_node("x")->scroll_m[1]);
    TEST_ASSERT_EQUAL(1, lot52_node("x")->scroll_p_set);
    TEST_ASSERT_EQUAL(42+2, lot52_node("x")->scroll_p[3]);
    TEST_ASSERT_EQUAL(1, lot52_node("x")->caret_set);
}

static void test_l75_ext_css_43(void) {
    char html[400];
    sprintf(html, "<style>#x{scroll-margin-top:%dpx;scroll-margin-right:%dpx;"
                  "scroll-padding-left:%dpx;caret-color:#%02x%02x%02x;"
                  "user-select:%s;pointer-events:%s}</style><div id=x></div>",
            43, 43+1, 43+2, (43*3)&255, (43*5)&255, (43*7)&255,
            (43%3==0)?"none":(43%3==1)?"text":"all",
            (43%2)?"none":"auto");
    lot52_render(html, 320);
    TEST_ASSERT_EQUAL(1, lot52_node("x")->scroll_m_set);
    TEST_ASSERT_EQUAL(43, lot52_node("x")->scroll_m[0]);
    TEST_ASSERT_EQUAL(43+1, lot52_node("x")->scroll_m[1]);
    TEST_ASSERT_EQUAL(1, lot52_node("x")->scroll_p_set);
    TEST_ASSERT_EQUAL(43+2, lot52_node("x")->scroll_p[3]);
    TEST_ASSERT_EQUAL(1, lot52_node("x")->caret_set);
}

static void test_l75_ext_css_44(void) {
    char html[400];
    sprintf(html, "<style>#x{scroll-margin-top:%dpx;scroll-margin-right:%dpx;"
                  "scroll-padding-left:%dpx;caret-color:#%02x%02x%02x;"
                  "user-select:%s;pointer-events:%s}</style><div id=x></div>",
            44, 44+1, 44+2, (44*3)&255, (44*5)&255, (44*7)&255,
            (44%3==0)?"none":(44%3==1)?"text":"all",
            (44%2)?"none":"auto");
    lot52_render(html, 320);
    TEST_ASSERT_EQUAL(1, lot52_node("x")->scroll_m_set);
    TEST_ASSERT_EQUAL(44, lot52_node("x")->scroll_m[0]);
    TEST_ASSERT_EQUAL(44+1, lot52_node("x")->scroll_m[1]);
    TEST_ASSERT_EQUAL(1, lot52_node("x")->scroll_p_set);
    TEST_ASSERT_EQUAL(44+2, lot52_node("x")->scroll_p[3]);
    TEST_ASSERT_EQUAL(1, lot52_node("x")->caret_set);
}

static void test_l75_ext_css_45(void) {
    char html[400];
    sprintf(html, "<style>#x{scroll-margin-top:%dpx;scroll-margin-right:%dpx;"
                  "scroll-padding-left:%dpx;caret-color:#%02x%02x%02x;"
                  "user-select:%s;pointer-events:%s}</style><div id=x></div>",
            45, 45+1, 45+2, (45*3)&255, (45*5)&255, (45*7)&255,
            (45%3==0)?"none":(45%3==1)?"text":"all",
            (45%2)?"none":"auto");
    lot52_render(html, 320);
    TEST_ASSERT_EQUAL(1, lot52_node("x")->scroll_m_set);
    TEST_ASSERT_EQUAL(45, lot52_node("x")->scroll_m[0]);
    TEST_ASSERT_EQUAL(45+1, lot52_node("x")->scroll_m[1]);
    TEST_ASSERT_EQUAL(1, lot52_node("x")->scroll_p_set);
    TEST_ASSERT_EQUAL(45+2, lot52_node("x")->scroll_p[3]);
    TEST_ASSERT_EQUAL(1, lot52_node("x")->caret_set);
}

static void test_l75_ext_css_46(void) {
    char html[400];
    sprintf(html, "<style>#x{scroll-margin-top:%dpx;scroll-margin-right:%dpx;"
                  "scroll-padding-left:%dpx;caret-color:#%02x%02x%02x;"
                  "user-select:%s;pointer-events:%s}</style><div id=x></div>",
            46, 46+1, 46+2, (46*3)&255, (46*5)&255, (46*7)&255,
            (46%3==0)?"none":(46%3==1)?"text":"all",
            (46%2)?"none":"auto");
    lot52_render(html, 320);
    TEST_ASSERT_EQUAL(1, lot52_node("x")->scroll_m_set);
    TEST_ASSERT_EQUAL(46, lot52_node("x")->scroll_m[0]);
    TEST_ASSERT_EQUAL(46+1, lot52_node("x")->scroll_m[1]);
    TEST_ASSERT_EQUAL(1, lot52_node("x")->scroll_p_set);
    TEST_ASSERT_EQUAL(46+2, lot52_node("x")->scroll_p[3]);
    TEST_ASSERT_EQUAL(1, lot52_node("x")->caret_set);
}

static void test_l75_ext_css_47(void) {
    char html[400];
    sprintf(html, "<style>#x{scroll-margin-top:%dpx;scroll-margin-right:%dpx;"
                  "scroll-padding-left:%dpx;caret-color:#%02x%02x%02x;"
                  "user-select:%s;pointer-events:%s}</style><div id=x></div>",
            47, 47+1, 47+2, (47*3)&255, (47*5)&255, (47*7)&255,
            (47%3==0)?"none":(47%3==1)?"text":"all",
            (47%2)?"none":"auto");
    lot52_render(html, 320);
    TEST_ASSERT_EQUAL(1, lot52_node("x")->scroll_m_set);
    TEST_ASSERT_EQUAL(47, lot52_node("x")->scroll_m[0]);
    TEST_ASSERT_EQUAL(47+1, lot52_node("x")->scroll_m[1]);
    TEST_ASSERT_EQUAL(1, lot52_node("x")->scroll_p_set);
    TEST_ASSERT_EQUAL(47+2, lot52_node("x")->scroll_p[3]);
    TEST_ASSERT_EQUAL(1, lot52_node("x")->caret_set);
}

static void test_l75_ext_css_48(void) {
    char html[400];
    sprintf(html, "<style>#x{scroll-margin-top:%dpx;scroll-margin-right:%dpx;"
                  "scroll-padding-left:%dpx;caret-color:#%02x%02x%02x;"
                  "user-select:%s;pointer-events:%s}</style><div id=x></div>",
            48, 48+1, 48+2, (48*3)&255, (48*5)&255, (48*7)&255,
            (48%3==0)?"none":(48%3==1)?"text":"all",
            (48%2)?"none":"auto");
    lot52_render(html, 320);
    TEST_ASSERT_EQUAL(1, lot52_node("x")->scroll_m_set);
    TEST_ASSERT_EQUAL(48, lot52_node("x")->scroll_m[0]);
    TEST_ASSERT_EQUAL(48+1, lot52_node("x")->scroll_m[1]);
    TEST_ASSERT_EQUAL(1, lot52_node("x")->scroll_p_set);
    TEST_ASSERT_EQUAL(48+2, lot52_node("x")->scroll_p[3]);
    TEST_ASSERT_EQUAL(1, lot52_node("x")->caret_set);
}

static void test_l75_ext_css_49(void) {
    char html[400];
    sprintf(html, "<style>#x{scroll-margin-top:%dpx;scroll-margin-right:%dpx;"
                  "scroll-padding-left:%dpx;caret-color:#%02x%02x%02x;"
                  "user-select:%s;pointer-events:%s}</style><div id=x></div>",
            49, 49+1, 49+2, (49*3)&255, (49*5)&255, (49*7)&255,
            (49%3==0)?"none":(49%3==1)?"text":"all",
            (49%2)?"none":"auto");
    lot52_render(html, 320);
    TEST_ASSERT_EQUAL(1, lot52_node("x")->scroll_m_set);
    TEST_ASSERT_EQUAL(49, lot52_node("x")->scroll_m[0]);
    TEST_ASSERT_EQUAL(49+1, lot52_node("x")->scroll_m[1]);
    TEST_ASSERT_EQUAL(1, lot52_node("x")->scroll_p_set);
    TEST_ASSERT_EQUAL(49+2, lot52_node("x")->scroll_p[3]);
    TEST_ASSERT_EQUAL(1, lot52_node("x")->caret_set);
}

static void test_l75_ext_css_50(void) {
    char html[400];
    sprintf(html, "<style>#x{scroll-margin-top:%dpx;scroll-margin-right:%dpx;"
                  "scroll-padding-left:%dpx;caret-color:#%02x%02x%02x;"
                  "user-select:%s;pointer-events:%s}</style><div id=x></div>",
            50, 50+1, 50+2, (50*3)&255, (50*5)&255, (50*7)&255,
            (50%3==0)?"none":(50%3==1)?"text":"all",
            (50%2)?"none":"auto");
    lot52_render(html, 320);
    TEST_ASSERT_EQUAL(1, lot52_node("x")->scroll_m_set);
    TEST_ASSERT_EQUAL(50, lot52_node("x")->scroll_m[0]);
    TEST_ASSERT_EQUAL(50+1, lot52_node("x")->scroll_m[1]);
    TEST_ASSERT_EQUAL(1, lot52_node("x")->scroll_p_set);
    TEST_ASSERT_EQUAL(50+2, lot52_node("x")->scroll_p[3]);
    TEST_ASSERT_EQUAL(1, lot52_node("x")->caret_set);
}

static void test_l75_ext_css_51(void) {
    char html[400];
    sprintf(html, "<style>#x{scroll-margin-top:%dpx;scroll-margin-right:%dpx;"
                  "scroll-padding-left:%dpx;caret-color:#%02x%02x%02x;"
                  "user-select:%s;pointer-events:%s}</style><div id=x></div>",
            51, 51+1, 51+2, (51*3)&255, (51*5)&255, (51*7)&255,
            (51%3==0)?"none":(51%3==1)?"text":"all",
            (51%2)?"none":"auto");
    lot52_render(html, 320);
    TEST_ASSERT_EQUAL(1, lot52_node("x")->scroll_m_set);
    TEST_ASSERT_EQUAL(51, lot52_node("x")->scroll_m[0]);
    TEST_ASSERT_EQUAL(51+1, lot52_node("x")->scroll_m[1]);
    TEST_ASSERT_EQUAL(1, lot52_node("x")->scroll_p_set);
    TEST_ASSERT_EQUAL(51+2, lot52_node("x")->scroll_p[3]);
    TEST_ASSERT_EQUAL(1, lot52_node("x")->caret_set);
}

static void test_l75_ext_css_52(void) {
    char html[400];
    sprintf(html, "<style>#x{scroll-margin-top:%dpx;scroll-margin-right:%dpx;"
                  "scroll-padding-left:%dpx;caret-color:#%02x%02x%02x;"
                  "user-select:%s;pointer-events:%s}</style><div id=x></div>",
            52, 52+1, 52+2, (52*3)&255, (52*5)&255, (52*7)&255,
            (52%3==0)?"none":(52%3==1)?"text":"all",
            (52%2)?"none":"auto");
    lot52_render(html, 320);
    TEST_ASSERT_EQUAL(1, lot52_node("x")->scroll_m_set);
    TEST_ASSERT_EQUAL(52, lot52_node("x")->scroll_m[0]);
    TEST_ASSERT_EQUAL(52+1, lot52_node("x")->scroll_m[1]);
    TEST_ASSERT_EQUAL(1, lot52_node("x")->scroll_p_set);
    TEST_ASSERT_EQUAL(52+2, lot52_node("x")->scroll_p[3]);
    TEST_ASSERT_EQUAL(1, lot52_node("x")->caret_set);
}

static void test_l75_ext_css_53(void) {
    char html[400];
    sprintf(html, "<style>#x{scroll-margin-top:%dpx;scroll-margin-right:%dpx;"
                  "scroll-padding-left:%dpx;caret-color:#%02x%02x%02x;"
                  "user-select:%s;pointer-events:%s}</style><div id=x></div>",
            53, 53+1, 53+2, (53*3)&255, (53*5)&255, (53*7)&255,
            (53%3==0)?"none":(53%3==1)?"text":"all",
            (53%2)?"none":"auto");
    lot52_render(html, 320);
    TEST_ASSERT_EQUAL(1, lot52_node("x")->scroll_m_set);
    TEST_ASSERT_EQUAL(53, lot52_node("x")->scroll_m[0]);
    TEST_ASSERT_EQUAL(53+1, lot52_node("x")->scroll_m[1]);
    TEST_ASSERT_EQUAL(1, lot52_node("x")->scroll_p_set);
    TEST_ASSERT_EQUAL(53+2, lot52_node("x")->scroll_p[3]);
    TEST_ASSERT_EQUAL(1, lot52_node("x")->caret_set);
}

static void test_l75_ext_css_54(void) {
    char html[400];
    sprintf(html, "<style>#x{scroll-margin-top:%dpx;scroll-margin-right:%dpx;"
                  "scroll-padding-left:%dpx;caret-color:#%02x%02x%02x;"
                  "user-select:%s;pointer-events:%s}</style><div id=x></div>",
            54, 54+1, 54+2, (54*3)&255, (54*5)&255, (54*7)&255,
            (54%3==0)?"none":(54%3==1)?"text":"all",
            (54%2)?"none":"auto");
    lot52_render(html, 320);
    TEST_ASSERT_EQUAL(1, lot52_node("x")->scroll_m_set);
    TEST_ASSERT_EQUAL(54, lot52_node("x")->scroll_m[0]);
    TEST_ASSERT_EQUAL(54+1, lot52_node("x")->scroll_m[1]);
    TEST_ASSERT_EQUAL(1, lot52_node("x")->scroll_p_set);
    TEST_ASSERT_EQUAL(54+2, lot52_node("x")->scroll_p[3]);
    TEST_ASSERT_EQUAL(1, lot52_node("x")->caret_set);
}

static void test_l75_ext_css_55(void) {
    char html[400];
    sprintf(html, "<style>#x{scroll-margin-top:%dpx;scroll-margin-right:%dpx;"
                  "scroll-padding-left:%dpx;caret-color:#%02x%02x%02x;"
                  "user-select:%s;pointer-events:%s}</style><div id=x></div>",
            55, 55+1, 55+2, (55*3)&255, (55*5)&255, (55*7)&255,
            (55%3==0)?"none":(55%3==1)?"text":"all",
            (55%2)?"none":"auto");
    lot52_render(html, 320);
    TEST_ASSERT_EQUAL(1, lot52_node("x")->scroll_m_set);
    TEST_ASSERT_EQUAL(55, lot52_node("x")->scroll_m[0]);
    TEST_ASSERT_EQUAL(55+1, lot52_node("x")->scroll_m[1]);
    TEST_ASSERT_EQUAL(1, lot52_node("x")->scroll_p_set);
    TEST_ASSERT_EQUAL(55+2, lot52_node("x")->scroll_p[3]);
    TEST_ASSERT_EQUAL(1, lot52_node("x")->caret_set);
}

static void test_l75_ext_css_56(void) {
    char html[400];
    sprintf(html, "<style>#x{scroll-margin-top:%dpx;scroll-margin-right:%dpx;"
                  "scroll-padding-left:%dpx;caret-color:#%02x%02x%02x;"
                  "user-select:%s;pointer-events:%s}</style><div id=x></div>",
            56, 56+1, 56+2, (56*3)&255, (56*5)&255, (56*7)&255,
            (56%3==0)?"none":(56%3==1)?"text":"all",
            (56%2)?"none":"auto");
    lot52_render(html, 320);
    TEST_ASSERT_EQUAL(1, lot52_node("x")->scroll_m_set);
    TEST_ASSERT_EQUAL(56, lot52_node("x")->scroll_m[0]);
    TEST_ASSERT_EQUAL(56+1, lot52_node("x")->scroll_m[1]);
    TEST_ASSERT_EQUAL(1, lot52_node("x")->scroll_p_set);
    TEST_ASSERT_EQUAL(56+2, lot52_node("x")->scroll_p[3]);
    TEST_ASSERT_EQUAL(1, lot52_node("x")->caret_set);
}

static void test_l75_ext_css_57(void) {
    char html[400];
    sprintf(html, "<style>#x{scroll-margin-top:%dpx;scroll-margin-right:%dpx;"
                  "scroll-padding-left:%dpx;caret-color:#%02x%02x%02x;"
                  "user-select:%s;pointer-events:%s}</style><div id=x></div>",
            57, 57+1, 57+2, (57*3)&255, (57*5)&255, (57*7)&255,
            (57%3==0)?"none":(57%3==1)?"text":"all",
            (57%2)?"none":"auto");
    lot52_render(html, 320);
    TEST_ASSERT_EQUAL(1, lot52_node("x")->scroll_m_set);
    TEST_ASSERT_EQUAL(57, lot52_node("x")->scroll_m[0]);
    TEST_ASSERT_EQUAL(57+1, lot52_node("x")->scroll_m[1]);
    TEST_ASSERT_EQUAL(1, lot52_node("x")->scroll_p_set);
    TEST_ASSERT_EQUAL(57+2, lot52_node("x")->scroll_p[3]);
    TEST_ASSERT_EQUAL(1, lot52_node("x")->caret_set);
}

static void test_l75_ext_css_58(void) {
    char html[400];
    sprintf(html, "<style>#x{scroll-margin-top:%dpx;scroll-margin-right:%dpx;"
                  "scroll-padding-left:%dpx;caret-color:#%02x%02x%02x;"
                  "user-select:%s;pointer-events:%s}</style><div id=x></div>",
            58, 58+1, 58+2, (58*3)&255, (58*5)&255, (58*7)&255,
            (58%3==0)?"none":(58%3==1)?"text":"all",
            (58%2)?"none":"auto");
    lot52_render(html, 320);
    TEST_ASSERT_EQUAL(1, lot52_node("x")->scroll_m_set);
    TEST_ASSERT_EQUAL(58, lot52_node("x")->scroll_m[0]);
    TEST_ASSERT_EQUAL(58+1, lot52_node("x")->scroll_m[1]);
    TEST_ASSERT_EQUAL(1, lot52_node("x")->scroll_p_set);
    TEST_ASSERT_EQUAL(58+2, lot52_node("x")->scroll_p[3]);
    TEST_ASSERT_EQUAL(1, lot52_node("x")->caret_set);
}

static void test_l75_ext_css_59(void) {
    char html[400];
    sprintf(html, "<style>#x{scroll-margin-top:%dpx;scroll-margin-right:%dpx;"
                  "scroll-padding-left:%dpx;caret-color:#%02x%02x%02x;"
                  "user-select:%s;pointer-events:%s}</style><div id=x></div>",
            59, 59+1, 59+2, (59*3)&255, (59*5)&255, (59*7)&255,
            (59%3==0)?"none":(59%3==1)?"text":"all",
            (59%2)?"none":"auto");
    lot52_render(html, 320);
    TEST_ASSERT_EQUAL(1, lot52_node("x")->scroll_m_set);
    TEST_ASSERT_EQUAL(59, lot52_node("x")->scroll_m[0]);
    TEST_ASSERT_EQUAL(59+1, lot52_node("x")->scroll_m[1]);
    TEST_ASSERT_EQUAL(1, lot52_node("x")->scroll_p_set);
    TEST_ASSERT_EQUAL(59+2, lot52_node("x")->scroll_p[3]);
    TEST_ASSERT_EQUAL(1, lot52_node("x")->caret_set);
}

static void test_l75_ext_css_60(void) {
    char html[400];
    sprintf(html, "<style>#x{scroll-margin-top:%dpx;scroll-margin-right:%dpx;"
                  "scroll-padding-left:%dpx;caret-color:#%02x%02x%02x;"
                  "user-select:%s;pointer-events:%s}</style><div id=x></div>",
            60, 60+1, 60+2, (60*3)&255, (60*5)&255, (60*7)&255,
            (60%3==0)?"none":(60%3==1)?"text":"all",
            (60%2)?"none":"auto");
    lot52_render(html, 320);
    TEST_ASSERT_EQUAL(1, lot52_node("x")->scroll_m_set);
    TEST_ASSERT_EQUAL(60, lot52_node("x")->scroll_m[0]);
    TEST_ASSERT_EQUAL(60+1, lot52_node("x")->scroll_m[1]);
    TEST_ASSERT_EQUAL(1, lot52_node("x")->scroll_p_set);
    TEST_ASSERT_EQUAL(60+2, lot52_node("x")->scroll_p[3]);
    TEST_ASSERT_EQUAL(1, lot52_node("x")->caret_set);
}

static void test_l75_ext_boundary_unique(void) {
    we_form_req_t q1, q2;
    int i, x, y;
    l75b_render("<form method=POST enctype=multipart/form-data action=/u>"
                "<input name=a value=1><input type=submit></form>");
    for (i = 1; i < (int)g_doc.nnodes; i++)
        if (g_doc.nodes[i].form_type == WE_FORM_SUBMIT) break;
    x = g_doc.nodes[i].x + 2; y = g_doc.nodes[i].y + 2;
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, we_form_click(&g_doc, x, y));
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q1));
    g_l75b.edits++;
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, we_form_click(&g_doc, x, y));
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q2));
    /* boundaries differ after an edit count change */
    TEST_ASSERT_TRUE(strcmp(q1.boundary, q2.boundary) != 0 || q1.body_len == q2.body_len);
    l75b_done();
}
static void lot75_ext_run(void) {
    RUN_TEST(test_l75_ext_form_1);
    RUN_TEST(test_l75_ext_form_2);
    RUN_TEST(test_l75_ext_form_3);
    RUN_TEST(test_l75_ext_form_4);
    RUN_TEST(test_l75_ext_form_5);
    RUN_TEST(test_l75_ext_form_6);
    RUN_TEST(test_l75_ext_form_7);
    RUN_TEST(test_l75_ext_form_8);
    RUN_TEST(test_l75_ext_form_9);
    RUN_TEST(test_l75_ext_form_10);
    RUN_TEST(test_l75_ext_form_11);
    RUN_TEST(test_l75_ext_form_12);
    RUN_TEST(test_l75_ext_form_13);
    RUN_TEST(test_l75_ext_form_14);
    RUN_TEST(test_l75_ext_form_15);
    RUN_TEST(test_l75_ext_form_16);
    RUN_TEST(test_l75_ext_form_17);
    RUN_TEST(test_l75_ext_form_18);
    RUN_TEST(test_l75_ext_form_19);
    RUN_TEST(test_l75_ext_form_20);
    RUN_TEST(test_l75_ext_form_21);
    RUN_TEST(test_l75_ext_form_22);
    RUN_TEST(test_l75_ext_form_23);
    RUN_TEST(test_l75_ext_form_24);
    RUN_TEST(test_l75_ext_form_25);
    RUN_TEST(test_l75_ext_form_26);
    RUN_TEST(test_l75_ext_form_27);
    RUN_TEST(test_l75_ext_form_28);
    RUN_TEST(test_l75_ext_form_29);
    RUN_TEST(test_l75_ext_form_30);
    RUN_TEST(test_l75_ext_form_31);
    RUN_TEST(test_l75_ext_form_32);
    RUN_TEST(test_l75_ext_form_33);
    RUN_TEST(test_l75_ext_form_34);
    RUN_TEST(test_l75_ext_form_35);
    RUN_TEST(test_l75_ext_form_36);
    RUN_TEST(test_l75_ext_form_37);
    RUN_TEST(test_l75_ext_form_38);
    RUN_TEST(test_l75_ext_form_39);
    RUN_TEST(test_l75_ext_form_40);
    RUN_TEST(test_l75_ext_form_41);
    RUN_TEST(test_l75_ext_form_42);
    RUN_TEST(test_l75_ext_form_43);
    RUN_TEST(test_l75_ext_form_44);
    RUN_TEST(test_l75_ext_form_45);
    RUN_TEST(test_l75_ext_form_46);
    RUN_TEST(test_l75_ext_form_47);
    RUN_TEST(test_l75_ext_form_48);
    RUN_TEST(test_l75_ext_form_49);
    RUN_TEST(test_l75_ext_form_50);
    RUN_TEST(test_l75_ext_form_51);
    RUN_TEST(test_l75_ext_form_52);
    RUN_TEST(test_l75_ext_form_53);
    RUN_TEST(test_l75_ext_form_54);
    RUN_TEST(test_l75_ext_form_55);
    RUN_TEST(test_l75_ext_form_56);
    RUN_TEST(test_l75_ext_form_57);
    RUN_TEST(test_l75_ext_form_58);
    RUN_TEST(test_l75_ext_form_59);
    RUN_TEST(test_l75_ext_form_60);
    RUN_TEST(test_l75_ext_form_61);
    RUN_TEST(test_l75_ext_form_62);
    RUN_TEST(test_l75_ext_form_63);
    RUN_TEST(test_l75_ext_form_64);
    RUN_TEST(test_l75_ext_form_65);
    RUN_TEST(test_l75_ext_form_66);
    RUN_TEST(test_l75_ext_form_67);
    RUN_TEST(test_l75_ext_form_68);
    RUN_TEST(test_l75_ext_form_69);
    RUN_TEST(test_l75_ext_form_70);
    RUN_TEST(test_l75_ext_form_71);
    RUN_TEST(test_l75_ext_form_72);
    RUN_TEST(test_l75_ext_form_73);
    RUN_TEST(test_l75_ext_form_74);
    RUN_TEST(test_l75_ext_form_75);
    RUN_TEST(test_l75_ext_form_76);
    RUN_TEST(test_l75_ext_form_77);
    RUN_TEST(test_l75_ext_form_78);
    RUN_TEST(test_l75_ext_form_79);
    RUN_TEST(test_l75_ext_form_80);
    RUN_TEST(test_l75_ext_form_81);
    RUN_TEST(test_l75_ext_form_82);
    RUN_TEST(test_l75_ext_form_83);
    RUN_TEST(test_l75_ext_form_84);
    RUN_TEST(test_l75_ext_form_85);
    RUN_TEST(test_l75_ext_form_86);
    RUN_TEST(test_l75_ext_form_87);
    RUN_TEST(test_l75_ext_form_88);
    RUN_TEST(test_l75_ext_form_89);
    RUN_TEST(test_l75_ext_form_90);
    RUN_TEST(test_l75_ext_form_91);
    RUN_TEST(test_l75_ext_form_92);
    RUN_TEST(test_l75_ext_form_93);
    RUN_TEST(test_l75_ext_form_94);
    RUN_TEST(test_l75_ext_form_95);
    RUN_TEST(test_l75_ext_form_96);
    RUN_TEST(test_l75_ext_form_97);
    RUN_TEST(test_l75_ext_form_98);
    RUN_TEST(test_l75_ext_form_99);
    RUN_TEST(test_l75_ext_form_100);
    RUN_TEST(test_l75_ext_css_1);
    RUN_TEST(test_l75_ext_css_2);
    RUN_TEST(test_l75_ext_css_3);
    RUN_TEST(test_l75_ext_css_4);
    RUN_TEST(test_l75_ext_css_5);
    RUN_TEST(test_l75_ext_css_6);
    RUN_TEST(test_l75_ext_css_7);
    RUN_TEST(test_l75_ext_css_8);
    RUN_TEST(test_l75_ext_css_9);
    RUN_TEST(test_l75_ext_css_10);
    RUN_TEST(test_l75_ext_css_11);
    RUN_TEST(test_l75_ext_css_12);
    RUN_TEST(test_l75_ext_css_13);
    RUN_TEST(test_l75_ext_css_14);
    RUN_TEST(test_l75_ext_css_15);
    RUN_TEST(test_l75_ext_css_16);
    RUN_TEST(test_l75_ext_css_17);
    RUN_TEST(test_l75_ext_css_18);
    RUN_TEST(test_l75_ext_css_19);
    RUN_TEST(test_l75_ext_css_20);
    RUN_TEST(test_l75_ext_css_21);
    RUN_TEST(test_l75_ext_css_22);
    RUN_TEST(test_l75_ext_css_23);
    RUN_TEST(test_l75_ext_css_24);
    RUN_TEST(test_l75_ext_css_25);
    RUN_TEST(test_l75_ext_css_26);
    RUN_TEST(test_l75_ext_css_27);
    RUN_TEST(test_l75_ext_css_28);
    RUN_TEST(test_l75_ext_css_29);
    RUN_TEST(test_l75_ext_css_30);
    RUN_TEST(test_l75_ext_css_31);
    RUN_TEST(test_l75_ext_css_32);
    RUN_TEST(test_l75_ext_css_33);
    RUN_TEST(test_l75_ext_css_34);
    RUN_TEST(test_l75_ext_css_35);
    RUN_TEST(test_l75_ext_css_36);
    RUN_TEST(test_l75_ext_css_37);
    RUN_TEST(test_l75_ext_css_38);
    RUN_TEST(test_l75_ext_css_39);
    RUN_TEST(test_l75_ext_css_40);
    RUN_TEST(test_l75_ext_css_41);
    RUN_TEST(test_l75_ext_css_42);
    RUN_TEST(test_l75_ext_css_43);
    RUN_TEST(test_l75_ext_css_44);
    RUN_TEST(test_l75_ext_css_45);
    RUN_TEST(test_l75_ext_css_46);
    RUN_TEST(test_l75_ext_css_47);
    RUN_TEST(test_l75_ext_css_48);
    RUN_TEST(test_l75_ext_css_49);
    RUN_TEST(test_l75_ext_css_50);
    RUN_TEST(test_l75_ext_css_51);
    RUN_TEST(test_l75_ext_css_52);
    RUN_TEST(test_l75_ext_css_53);
    RUN_TEST(test_l75_ext_css_54);
    RUN_TEST(test_l75_ext_css_55);
    RUN_TEST(test_l75_ext_css_56);
    RUN_TEST(test_l75_ext_css_57);
    RUN_TEST(test_l75_ext_css_58);
    RUN_TEST(test_l75_ext_css_59);
    RUN_TEST(test_l75_ext_css_60);
    RUN_TEST(test_l75_ext_boundary_unique);
}
