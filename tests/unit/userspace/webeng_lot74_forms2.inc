/* Lot 74: number, range, url, minlength, tabindex and autofocus
 * (webeng_finput.inc), through the helpers of webeng_lot74_forms.inc */

static int l74_fill_w(const char *id) {
    int i = l74_node(id), c;
    for (c = g_doc.nodes[i].first; c >= 0; c = g_doc.nodes[c].next)
        if (strcmp(g_doc.nodes[c].tag, "::fill") == 0) return g_doc.nodes[c].display == WE_DISP_NONE ? 0 : g_doc.nodes[c].w;
    return -1;
}

/* :invalid paints red (every page below has the rule) */
#define L74_BAD "<style>:invalid{color:#ff0000}</style>"
static int l74_bad(const char *id) { return (g_doc.nodes[l74_node(id)].color & 0xffffff) == 0xff0000; }

static void test_l74_number_keys_and_validity(void) {
    we_form_req_t q;
    l74_open(L74_BAD "<form action=/n id=f><input id=n type=number name=n min=2 max=10 step=2 value=4>"
             "<input id=m type=number name=m><input id=s type=number name=s step=any value=1.5>"
             "<input id=d type=number name=d step=0.25 min=-1></form>");
    TEST_ASSERT_EQUAL(0, l61_diag_count("input type taken as text"));
    l74_click("n");
    TEST_ASSERT_EQUAL(WE_FACT_CHANGED, l74_key(WE_KEY_UP));
    TEST_ASSERT_EQUAL_STRING("6", l74_val("n"));
    l74_key(WE_KEY_UP); l74_key(WE_KEY_UP); l74_key(WE_KEY_UP);
    TEST_ASSERT_EQUAL_STRING("10", l74_val("n"));          /* clamped to max */
    TEST_ASSERT_EQUAL(WE_FACT_CHANGED, l74_key(WE_KEY_DOWN));
    TEST_ASSERT_EQUAL_STRING("8", l74_val("n"));
    /* typing: digits, sign and point only */
    TEST_ASSERT_EQUAL(WE_FACT_NONE, l74_key('x'));
    TEST_ASSERT_EQUAL(1, g_l74f.refused);
    TEST_ASSERT_EQUAL(WE_FACT_CHANGED, l74_key(WE_KEY_BACKSPACE));
    l74_type("5");
    TEST_ASSERT_EQUAL_STRING("5", l74_val("n"));
    TEST_ASSERT_EQUAL(1, l74_bad("n"));   /* off the step from min 2 */
    TEST_ASSERT_EQUAL(WE_FACT_CHANGED, l74_key(WE_KEY_UP));
    TEST_ASSERT_EQUAL_STRING("6", l74_val("n"));          /* the next value on the step */
    l74_key(WE_KEY_BACKSPACE); l74_type("3");
    TEST_ASSERT_EQUAL(WE_FACT_CHANGED, l74_key(WE_KEY_DOWN));
    TEST_ASSERT_EQUAL_STRING("2", l74_val("n"));
    l74_key(WE_KEY_DOWN);
    TEST_ASSERT_EQUAL_STRING("2", l74_val("n"));          /* clamped to min */
    l74_key(WE_KEY_BACKSPACE); l74_type("12");
    TEST_ASSERT_EQUAL(1, l74_bad("n"));   /* over max */
    l74_key(WE_KEY_BACKSPACE); l74_key(WE_KEY_BACKSPACE); l74_type("-");
    TEST_ASSERT_EQUAL(1, l74_bad("n"));   /* not a number */
    l74_key(WE_KEY_BACKSPACE); l74_type("0");
    TEST_ASSERT_EQUAL(1, l74_bad("n"));   /* under min */
    TEST_ASSERT_EQUAL(WE_FACT_CHANGED, l74_key(WE_KEY_ENTER));     /* refused: focus stays on n */
    TEST_ASSERT_EQUAL(l74_ctl("n"), g_l74f.focus);
    l74_key(WE_KEY_BACKSPACE); l74_type("4");
    TEST_ASSERT_EQUAL(0, l74_bad("n"));
    /* an empty number: up starts at min, else one step from 0; step any
     * moves by 1 */
    l74_click("m");
    l74_key(WE_KEY_UP);
    TEST_ASSERT_EQUAL_STRING("1", l74_val("m"));
    l74_key(WE_KEY_DOWN); l74_key(WE_KEY_DOWN);
    TEST_ASSERT_EQUAL_STRING("-1", l74_val("m"));
    l74_click("s");
    TEST_ASSERT_EQUAL(0, l74_bad("s"));
    l74_key(WE_KEY_UP);
    TEST_ASSERT_EQUAL_STRING("2.5", l74_val("s"));
    l74_click("d");
    l74_key(WE_KEY_UP);
    TEST_ASSERT_EQUAL_STRING("-1", l74_val("d"));          /* empty: min */
    l74_key(WE_KEY_UP);
    TEST_ASSERT_EQUAL_STRING("-0.75", l74_val("d"));
    l74_key(WE_KEY_UP); l74_key(WE_KEY_UP); l74_key(WE_KEY_UP); l74_key(WE_KEY_UP); l74_key(WE_KEY_UP);
    TEST_ASSERT_EQUAL_STRING("0.5", l74_val("d"));
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l74_key(WE_KEY_ENTER));
    TEST_ASSERT_EQUAL_STRING("/n?n=4&m=-1&s=2.5&d=0.5", l74_req(0, &q));
    l74_done();
}

static void test_l74_number_attr_refusals(void) {
    l74_open("<form><input id=a type=number step=0 value=3><input id=b type=number step=x min=y max=1.5.5>"
             "<input id=c type=number readonly value=1></form>");
    TEST_ASSERT_EQUAL(4, l61_diag_count("form number refused"));
    l74_click("a");
    l74_key(WE_KEY_UP);
    TEST_ASSERT_EQUAL_STRING("4", l74_val("a"));          /* refused step: 1 */
    l74_click("c");
    TEST_ASSERT_EQUAL(WE_FACT_NONE, l74_key(WE_KEY_UP));
    TEST_ASSERT_EQUAL_STRING("1", l74_val("c"));
    TEST_ASSERT_EQUAL(1, g_l74f.refused);
    l74_done();
}

static void test_l74_range(void) {
    we_form_req_t q;
    int x, y;
    l74_open(L74_BAD "<body style='margin:0'><form action=/r id=f><input id=r type=range name=r>"
             "<input id=s type=range name=s min=10 max=20 step=5 value=13><input id=t type=range name=t min=5 max=1 value=9>"
             "<input id=u type=range name=u value=abc step=30></form></body>");
    TEST_ASSERT_EQUAL(0, l61_diag_count("input type taken as text"));
    TEST_ASSERT_EQUAL_STRING("50", l74_val("r"));          /* the middle */
    TEST_ASSERT_EQUAL_STRING("15", l74_val("s"));          /* 13 on the step: 15 */
    TEST_ASSERT_EQUAL_STRING("5", l74_val("t"));           /* max under min: min */
    TEST_ASSERT_EQUAL_STRING("60", l74_val("u"));          /* 50 on the step 30 from 0 */
    TEST_ASSERT_EQUAL(80, g_doc.nodes[l74_node("r")].w - 2);
    TEST_ASSERT_EQUAL(40, l74_fill_w("r"));
    TEST_ASSERT_EQUAL(40, l74_fill_w("s"));
    TEST_ASSERT_EQUAL(0, l74_fill_w("t"));
    /* keys: a step; left / right too; the ends stay (Tab: a click would
     * set the value) */
    l74_key(WE_KEY_TAB); l74_key(WE_KEY_TAB);
    TEST_ASSERT_EQUAL(l74_ctl("s"), g_l74f.focus);
    TEST_ASSERT_EQUAL(WE_FACT_CHANGED, l74_key(WE_KEY_RIGHT));
    TEST_ASSERT_EQUAL_STRING("20", l74_val("s"));
    TEST_ASSERT_EQUAL(80, l74_fill_w("s"));
    TEST_ASSERT_EQUAL(WE_FACT_NONE, l74_key(WE_KEY_UP));
    TEST_ASSERT_EQUAL(WE_FACT_CHANGED, l74_key(WE_KEY_LEFT));
    TEST_ASSERT_EQUAL(WE_FACT_CHANGED, l74_key(WE_KEY_DOWN));
    TEST_ASSERT_EQUAL_STRING("10", l74_val("s"));
    TEST_ASSERT_EQUAL(WE_FACT_NONE, l74_key(WE_KEY_DOWN));
    TEST_ASSERT_EQUAL(WE_FACT_NONE, l74_key('7'));        /* no typing */
    TEST_ASSERT_EQUAL(-1, we_form_set_text(&g_doc, l74_ctl("s"), "12"));
    /* a click sets the value from the position on the track */
    x = g_doc.nodes[l74_node("r")].x + 1 + 20;
    y = g_doc.nodes[l74_node("r")].y + 2;
    TEST_ASSERT_EQUAL(WE_FACT_CHANGED, we_form_click(&g_doc, x, y));
    l74_render();
    TEST_ASSERT_EQUAL_STRING("25", l74_val("r"));
    TEST_ASSERT_EQUAL(l74_ctl("r"), g_l74f.focus);
    TEST_ASSERT_EQUAL(20, l74_fill_w("r"));
    TEST_ASSERT_EQUAL(WE_FACT_NONE, we_form_click(&g_doc, x, y));   /* same value, same focus */
    x = g_doc.nodes[l74_node("r")].x + 81;
    we_form_click(&g_doc, x, y);
    l74_render();
    TEST_ASSERT_EQUAL_STRING("100", l74_val("r"));
    /* Enter submits; reset gives the markup value back */
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l74_key(WE_KEY_ENTER));
    TEST_ASSERT_EQUAL_STRING("/r?r=100&s=10&t=5&u=60", l74_req(0, &q));
    TEST_ASSERT_EQUAL(0, l74_bad("u"));
    l74_done();
}

static void test_l74_url_and_minlength(void) {
    l74_open(L74_BAD "<form id=f><input id=u type=url name=u><input id=m name=m minlength=3 value=ab>"
             "<textarea id=t name=t minlength=2></textarea><input id=z minlength=x></form>");
    TEST_ASSERT_EQUAL(1, l61_diag_count("form size refused"));
    TEST_ASSERT_EQUAL(0, l74_bad("m"));   /* not edited: valid */
    l74_click("u");
    l74_type("example.com");
    TEST_ASSERT_EQUAL(1, l74_bad("u"));
    TEST_ASSERT_EQUAL(1, we_form_set_text(&g_doc, l74_ctl("u"), "https:") == 0);
    l74_render();
    TEST_ASSERT_EQUAL(1, l74_bad("u"));   /* nothing after the scheme */
    we_form_set_text(&g_doc, l74_ctl("u"), "a+b.c-d:x");
    l74_render();
    TEST_ASSERT_EQUAL(0, l74_bad("u"));
    we_form_set_text(&g_doc, l74_ctl("u"), "1a:x");
    l74_render();
    TEST_ASSERT_EQUAL(1, l74_bad("u"));
    we_form_set_text(&g_doc, l74_ctl("u"), "a_b:x");
    l74_render();
    TEST_ASSERT_EQUAL(1, l74_bad("u"));
    we_form_set_text(&g_doc, l74_ctl("u"), "");
    l74_render();
    TEST_ASSERT_EQUAL(0, l74_bad("u"));   /* empty and not required */
    l74_click("m");
    l74_key(WE_KEY_BACKSPACE);
    TEST_ASSERT_EQUAL(1, l74_bad("m"));
    l74_type("bc");
    TEST_ASSERT_EQUAL(0, l74_bad("m"));
    l74_key(WE_KEY_BACKSPACE); l74_key(WE_KEY_BACKSPACE); l74_key(WE_KEY_BACKSPACE);
    TEST_ASSERT_EQUAL(0, l74_bad("m"));   /* empty is not too short */
    l74_click("t");
    l74_type("x");
    TEST_ASSERT_EQUAL(1, l74_bad("t"));
    l74_done();
}

static void test_l74_tabindex_and_autofocus(void) {
    l74_open("<form><input id=a><input id=b tabindex=2><input id=c tabindex=-1><input id=d tabindex=1>"
             "<input id=e tabindex=0 autofocus><input id=f autofocus><input id=g tabindex=x><input id=h tabindex=999>"
             "<input id=i disabled tabindex=1></form>");
    TEST_ASSERT_EQUAL(1, l61_diag_count("form tabindex refused"));
    TEST_ASSERT_EQUAL(l74_ctl("e"), g_l74f.focus);         /* the first autofocus */
    TEST_ASSERT_EQUAL(1, g_l74f.autofocused);
    /* order: d (1), b (2), h (127), then a, e, f, g in tree order; c out */
    l74_key(WE_KEY_ESC);
    TEST_ASSERT_EQUAL(-1, g_l74f.focus);
    l74_render();
    TEST_ASSERT_EQUAL(-1, g_l74f.focus);                   /* autofocus only once */
    l74_key(WE_KEY_TAB); TEST_ASSERT_EQUAL(l74_ctl("d"), g_l74f.focus);
    l74_key(WE_KEY_TAB); TEST_ASSERT_EQUAL(l74_ctl("b"), g_l74f.focus);
    l74_key(WE_KEY_TAB); TEST_ASSERT_EQUAL(l74_ctl("h"), g_l74f.focus);
    l74_key(WE_KEY_TAB); TEST_ASSERT_EQUAL(l74_ctl("a"), g_l74f.focus);
    l74_key(WE_KEY_TAB); TEST_ASSERT_EQUAL(l74_ctl("e"), g_l74f.focus);
    l74_key(WE_KEY_TAB); TEST_ASSERT_EQUAL(l74_ctl("f"), g_l74f.focus);
    l74_key(WE_KEY_TAB); TEST_ASSERT_EQUAL(l74_ctl("g"), g_l74f.focus);
    l74_key(WE_KEY_TAB); TEST_ASSERT_EQUAL(l74_ctl("d"), g_l74f.focus);   /* wraps */
    l74_key(WE_KEY_SHIFT_TAB); TEST_ASSERT_EQUAL(l74_ctl("g"), g_l74f.focus);
    l74_key(WE_KEY_SHIFT_TAB); TEST_ASSERT_EQUAL(l74_ctl("f"), g_l74f.focus);
    /* tabindex -1: focus by a click, Tab goes on among tabindex 0 after it */
    l74_click("c");
    TEST_ASSERT_EQUAL(l74_ctl("c"), g_l74f.focus);
    l74_key(WE_KEY_TAB); TEST_ASSERT_EQUAL(l74_ctl("e"), g_l74f.focus);   /* the next tabindex 0 after c */
    l74_key(WE_KEY_ESC);
    l74_key(WE_KEY_SHIFT_TAB); TEST_ASSERT_EQUAL(l74_ctl("g"), g_l74f.focus);
    l74_done();
    /* a disabled autofocus is skipped */
    l74_open("<input id=a disabled autofocus><input id=b type=hidden autofocus><input id=c autofocus>");
    TEST_ASSERT_EQUAL(l74_ctl("c"), g_l74f.focus);
    l74_done();
}

static void lot74_forms2_run(void) {
    RUN_TEST(test_l74_number_keys_and_validity);
    RUN_TEST(test_l74_number_attr_refusals);
    RUN_TEST(test_l74_range);
    RUN_TEST(test_l74_url_and_minlength);
    RUN_TEST(test_l74_tabindex_and_autofocus);
}
