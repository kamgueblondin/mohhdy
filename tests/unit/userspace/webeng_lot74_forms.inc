/* Lot 74: interactive forms (userspace/webeng_finput.inc). Every test
 * renders through l74_render, which keeps one we_forms_t across renders
 * like the browser; l74_done detaches it so other tests stay static. */
static we_forms_t g_l74f;
static const char *g_l74_html;

static void l74_render(void) {
    we_set_image_reader(0, 0);
    TEST_ASSERT_EQUAL(0, we_render(&g_doc, g_l74_html, (int)strlen(g_l74_html), 400));
}
static void l74_open(const char *html) {
    we_forms_init(&g_l74f);
    g_doc.forms = 0;
    we_set_forms(&g_doc, &g_l74f);
    g_l74_html = html;
    l74_render();
}
static void l74_done(void) { we_set_forms(&g_doc, 0); }
static int l74_node(const char *id) { int i = we_find_id(&g_doc, id); return i > 0 ? i : 0; }
/* a click 2 px inside the box of element id, then a render */
/* a point 2 px inside element id, or inside its first laid-out text
 * child (an inline element has no box of its own) */
static void l74_pt(const char *id, int *x, int *y) {
    int i = l74_node(id), c;
    for (c = g_doc.nodes[i].first; g_doc.nodes[i].w <= 0 && c >= 0; c = g_doc.nodes[c].next)
        if (g_doc.nodes[c].is_text && g_doc.nodes[c].w > 0) { i = c; break; }
    *x = g_doc.nodes[i].x + 2;
    *y = g_doc.nodes[i].y + 2;
}
static int l74_at(const char *id) { int x, y; l74_pt(id, &x, &y); return we_form_at(&g_doc, x, y); }
static int l74_click(const char *id) {
    int x, y, rc;
    l74_pt(id, &x, &y);
    rc = we_form_click(&g_doc, x, y);
    l74_render();
    return rc;
}
static int l74_key(int key) { int rc = we_form_key(&g_doc, key); l74_render(); return rc; }
static void l74_type(const char *s) { while (*s) { int c = *s++; TEST_ASSERT_EQUAL(WE_FACT_CHANGED, l74_key(c == '\r' ? WE_KEY_ENTER : c)); } }
static int l74_ctl(const char *id) { return g_doc.nodes[l74_node(id)].form_ord - 1; }
static const char *l74_val(const char *id) {
    static char v[80];
    if (we_form_value(&g_doc, l74_ctl(id), v, (int)sizeof(v)) < 0) return "<no control>";
    return v;
}
static int l74_text(const char *s) {
    uint32_t i;
    for (i = 0; i < g_doc.nitems; i++)
        if (g_doc.items[i].kind == OS_FB_WEB_TEXT && strstr(g_doc.items[i].text, s)) return 1;
    return 0;
}
static const char *l74_req(int expect_rc, we_form_req_t *q) {
    return we_form_request(&g_doc, q) == expect_rc ? q->url : "<request rc>";
}

#define L74_FORM "<form action='/find?old=1#top' id=f>" \
    "<input id=q name=q value='hi there'><input id=pw type=password name=pw>" \
    "<input id=cb type=checkbox name=cb><input id=cv type=checkbox name=cv value=yes checked>" \
    "<input id=r1 type=radio name=r value=one checked><input id=r2 type=radio name=r value=two>" \
    "<input id=r3 type=radio name=other value=three>" \
    "<select id=s name=s><option>a<option value=B>b<option>c</select>" \
    "<textarea id=ta name=ta>x</textarea><input type=hidden name=h value='1 2'>" \
    "<input id=go type=submit name=go value=Go><input id=go2 type=submit name=go2 value=Two>" \
    "<input id=rs type=reset><input id=bt type=button name=bt value=B>" \
    "<input id=nn value=noname><input id=dis name=dis value=d disabled></form>"

static void test_l74_ordinals_and_static_without_state(void) {
    lot52_render("<input id=a><select id=b><option>x</select><textarea id=c></textarea><button id=d>b</button>"
                 "<details id=e><summary>s</summary>t</details><p id=p>p</p><input id=h type=hidden>", 400);
    TEST_ASSERT_EQUAL(1, lot52_node("a")->form_ord);
    TEST_ASSERT_EQUAL(2, lot52_node("b")->form_ord);
    TEST_ASSERT_EQUAL(3, lot52_node("c")->form_ord);
    TEST_ASSERT_EQUAL(4, lot52_node("d")->form_ord);
    TEST_ASSERT_EQUAL(5, lot52_node("e")->form_ord);
    TEST_ASSERT_EQUAL(0, lot52_node("p")->form_ord);
    TEST_ASSERT_EQUAL(6, lot52_node("h")->form_ord);
    TEST_ASSERT_EQUAL(6, (int)g_doc.nform_ctl);
    /* no state: the calls refuse, nothing changes */
    TEST_ASSERT_EQUAL(-1, we_form_click(&g_doc, 2, 2));
    TEST_ASSERT_EQUAL(-1, we_form_key(&g_doc, 'a'));
    TEST_ASSERT_EQUAL(0, we_form_at(&g_doc, lot52_node("a")->x + 1, lot52_node("a")->y + 1));
    TEST_ASSERT_EQUAL(-1, we_form_at(&g_doc, lot52_node("p")->x + 1, lot52_node("p")->y + 1));
    TEST_ASSERT_EQUAL(-1, we_form_set_text(&g_doc, 0, "x"));
}

static void test_l74_state_full(void) {
    char html[1200];
    int k, o = 0;
    for (k = 0; k < WE_FSTATE_MAX + 2; k++) o += sprintf(html + o, "<input id=i%d>", k);
    lot52_render(html, 400);
    TEST_ASSERT_EQUAL(WE_FSTATE_MAX, lot52_node("i31")->form_ord);
    TEST_ASSERT_EQUAL(0, lot52_node("i32")->form_ord);
    TEST_ASSERT_EQUAL(0, lot52_node("i33")->form_ord);
    TEST_ASSERT_EQUAL(WE_FSTATE_MAX, (int)g_doc.nform_ctl);
    TEST_ASSERT_EQUAL(1, l61_diag_count("form state full"));
}

static void test_l74_text_typing_caret_and_request(void) {
    we_form_req_t q;
    l74_open(L74_FORM);
    TEST_ASSERT_FALSE(l74_text("_"));
    TEST_ASSERT_EQUAL(WE_FACT_CHANGED, l74_click("q"));
    TEST_ASSERT_EQUAL(l74_ctl("q"), g_l74f.focus);
    TEST_ASSERT_TRUE(l74_text("_"));                         /* caret */
    TEST_ASSERT_EQUAL(WE_FACT_NONE, l74_click("q"));         /* already focused */
    TEST_ASSERT_EQUAL(0x3875D7U, g_doc.nodes[l74_node("q")].border_color);
    l74_type("!&");
    TEST_ASSERT_EQUAL(0, strcmp(l74_val("q"), "hi there!&"));
    TEST_ASSERT_TRUE(l74_text("hi there!&"));
    TEST_ASSERT_EQUAL(WE_FACT_CHANGED, l74_key(WE_KEY_BACKSPACE));
    TEST_ASSERT_EQUAL(0, strcmp(l74_val("q"), "hi there!"));
    TEST_ASSERT_EQUAL(WE_FACT_NONE, l74_key(0x7F));          /* not a typed byte */
    TEST_ASSERT_EQUAL(WE_FACT_NONE, l74_key(31));
    TEST_ASSERT_EQUAL(3U, g_l74f.edits);
    /* password: stars painted, bytes sent */
    TEST_ASSERT_EQUAL(WE_FACT_CHANGED, l74_click("pw"));
    TEST_ASSERT_TRUE(g_doc.nodes[l74_node("q")].border_color != 0x3875D7U);
    l74_type("ab c");
    TEST_ASSERT_TRUE(l74_text("****"));
    TEST_ASSERT_FALSE(l74_text("ab c"));
    /* Enter: implicit submission with the first submit button */
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l74_key(WE_KEY_ENTER));
    TEST_ASSERT_EQUAL(l74_ctl("go"), g_l74f.submitter);
    TEST_ASSERT_EQUAL(l74_node("f"), g_l74f.submit_form);
    TEST_ASSERT_EQUAL(0, strcmp(l74_req(0, &q),
        "/find?q=hi+there%21&pw=ab+c&cv=yes&r=one&s=a&ta=x&h=1+2&go=Go"));
    TEST_ASSERT_EQUAL(0, q.post);
    TEST_ASSERT_EQUAL(0, q.multipart);
    TEST_ASSERT_EQUAL(l74_node("f"), q.form);
    TEST_ASSERT_EQUAL(0, strcmp(q.body, "q=hi+there%21&pw=ab+c&cv=yes&r=one&s=a&ta=x&h=1+2&go=Go"));
    TEST_ASSERT_EQUAL((int)strlen(q.body), q.body_len);
    TEST_ASSERT_EQUAL(1U, g_l74f.submits);
    l74_done();
}

static void test_l74_toggles_radio_select_textarea(void) {
    we_form_req_t q;
    l74_open(L74_FORM);
    TEST_ASSERT_EQUAL(WE_FACT_CHANGED, l74_click("cb"));
    TEST_ASSERT_TRUE(g_doc.nodes[l74_node("cb")].form_flags & 1);
    TEST_ASSERT_EQUAL(0, strcmp(l74_val("cb"), "on"));
    TEST_ASSERT_EQUAL(WE_FACT_CHANGED, l74_click("cv"));
    TEST_ASSERT_EQUAL(0, strcmp(l74_val("cv"), ""));
    TEST_ASSERT_EQUAL(WE_FACT_CHANGED, l74_click("r2"));
    TEST_ASSERT_EQUAL(0, strcmp(l74_val("r1"), ""));
    TEST_ASSERT_EQUAL(0, strcmp(l74_val("r2"), "two"));
    TEST_ASSERT_EQUAL(WE_FACT_NONE, l74_click("r2"));        /* checked, focused: nothing */
    TEST_ASSERT_EQUAL(WE_FACT_CHANGED, l74_click("r3"));     /* another group */
    TEST_ASSERT_EQUAL(0, strcmp(l74_val("r2"), "two"));
    TEST_ASSERT_EQUAL(0, strcmp(l74_val("r3"), "three"));
    TEST_ASSERT_EQUAL(WE_FACT_CHANGED, l74_click("r1"));
    TEST_ASSERT_EQUAL(0, strcmp(l74_val("r2"), ""));
    TEST_ASSERT_EQUAL(0, strcmp(l74_val("r3"), "three"));
    TEST_ASSERT_EQUAL(0, strcmp(l74_val("s"), "a"));
    TEST_ASSERT_EQUAL(WE_FACT_CHANGED, l74_click("s"));
    TEST_ASSERT_EQUAL(0, strcmp(l74_val("s"), "B"));
    TEST_ASSERT_TRUE(l74_text("b"));
    TEST_ASSERT_EQUAL(WE_FACT_CHANGED, l74_click("s"));
    TEST_ASSERT_EQUAL(WE_FACT_CHANGED, l74_click("s"));     /* wraps */
    TEST_ASSERT_EQUAL(0, strcmp(l74_val("s"), "a"));
    TEST_ASSERT_EQUAL(WE_FACT_NONE, l74_key(WE_KEY_UP));     /* first: no move */
    TEST_ASSERT_EQUAL(WE_FACT_CHANGED, l74_key(WE_KEY_DOWN));
    TEST_ASSERT_EQUAL(WE_FACT_CHANGED, l74_key(WE_KEY_DOWN));
    TEST_ASSERT_EQUAL(0, strcmp(l74_val("s"), "c"));
    TEST_ASSERT_EQUAL(WE_FACT_NONE, l74_key(WE_KEY_DOWN));   /* last */
    TEST_ASSERT_EQUAL(WE_FACT_CHANGED, l74_key(WE_KEY_UP));
    TEST_ASSERT_EQUAL(0, strcmp(l74_val("s"), "B"));
    TEST_ASSERT_EQUAL(WE_FACT_NONE, l74_key('z'));           /* select takes no text */
    TEST_ASSERT_EQUAL(WE_FACT_CHANGED, l74_click("ta"));
    l74_type("\ry");
    TEST_ASSERT_EQUAL(0, strcmp(l74_val("ta"), "x\ny"));
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l74_click("go2"));
    TEST_ASSERT_EQUAL(0, strcmp(l74_req(0, &q), "/find?q=hi+there&pw=&cb=on&r=one&other=three&s=B&ta=x%0D%0Ay&h=1+2&go2=Two"));
    TEST_ASSERT_EQUAL(11U, g_l74f.toggles);
    l74_done();
}

/* a reset button resets its own form only */
static void test_l74_reset_own_form(void) {
    l74_open("<form><input id=a value=1><input id=ra type=reset></form><form><input id=b value=2></form>");
    (void)l74_click("a"); l74_type("x");
    (void)l74_click("b"); l74_type("y");
    TEST_ASSERT_EQUAL(WE_FACT_CHANGED, l74_click("ra"));
    TEST_ASSERT_EQUAL_STRING("1", l74_val("a"));
    TEST_ASSERT_EQUAL_STRING("2y", l74_val("b"));
    l74_done();
}

static void test_l74_reset_disabled_readonly_maxlength(void) {
    l74_open(L74_FORM);
    (void)l74_click("q");
    l74_type("x");
    (void)l74_click("cb");
    (void)l74_click("s");
    TEST_ASSERT_EQUAL(0, strcmp(l74_val("q"), "hi therex"));
    TEST_ASSERT_EQUAL(WE_FACT_CHANGED, l74_click("rs"));
    TEST_ASSERT_EQUAL(0, strcmp(l74_val("q"), "hi there"));
    TEST_ASSERT_EQUAL(0, strcmp(l74_val("cb"), ""));
    TEST_ASSERT_EQUAL(0, strcmp(l74_val("s"), "a"));
    TEST_ASSERT_EQUAL(1U, g_l74f.resets);
    TEST_ASSERT_EQUAL(WE_FACT_NONE, l74_click("dis"));
    TEST_ASSERT_EQUAL(1U, g_l74f.refused);
    TEST_ASSERT_EQUAL(l74_ctl("rs"), g_l74f.focus);
    TEST_ASSERT_EQUAL(-1, we_form_set_text(&g_doc, l74_ctl("dis"), "x"));
    TEST_ASSERT_EQUAL(WE_FACT_CHANGED, l74_click("bt"));    /* a plain button only takes the focus */
    TEST_ASSERT_EQUAL(WE_FACT_NONE, l74_key(' '));
    l74_done();
    l74_open("<form><input id=ro readonly value=r><input id=m maxlength=3 value=ab><input id=z maxlength=0>"
             "<input id=e maxlength=x><input id=b maxlength=999></form>");
    TEST_ASSERT_EQUAL(2, l61_diag_count("form size refused"));
    (void)l74_click("ro");
    TEST_ASSERT_EQUAL(WE_FACT_NONE, l74_key('x'));
    TEST_ASSERT_EQUAL(WE_FACT_NONE, l74_key(WE_KEY_BACKSPACE));
    TEST_ASSERT_EQUAL(-1, we_form_set_text(&g_doc, l74_ctl("ro"), "x"));
    TEST_ASSERT_EQUAL(0, strcmp(l74_val("ro"), "r"));
    (void)l74_click("m");
    TEST_ASSERT_EQUAL(WE_FACT_CHANGED, l74_key('c'));
    TEST_ASSERT_EQUAL(WE_FACT_NONE, l74_key('d'));
    TEST_ASSERT_EQUAL(0, strcmp(l74_val("m"), "abc"));
    TEST_ASSERT_EQUAL(0, we_form_set_text(&g_doc, l74_ctl("m"), "wxyz"));
    l74_render();
    TEST_ASSERT_EQUAL(0, strcmp(l74_val("m"), "wxy"));
    (void)l74_click("z");
    TEST_ASSERT_EQUAL(WE_FACT_NONE, l74_key('a'));
    TEST_ASSERT_EQUAL(5U, g_l74f.refused);   /* ro x, ro backspace, d, wxyz cut, z a */
    (void)l74_click("e");
    TEST_ASSERT_EQUAL(WE_FACT_CHANGED, l74_key('a'));
    l74_done();
}

static void test_l74_tab_esc_and_space(void) {
    l74_open("<form><input id=a><input id=h type=hidden><input id=d disabled><input id=c type=checkbox>"
             "<p style='display:none'><input id=n></p><select id=s><option>1<option>2</select>"
             "<input id=sb type=submit></form>");
    TEST_ASSERT_EQUAL(-1, we_form_key(&g_doc, 'x'));         /* no focus */
    TEST_ASSERT_EQUAL(WE_FACT_CHANGED, l74_key(WE_KEY_TAB));
    TEST_ASSERT_EQUAL(l74_ctl("a"), g_l74f.focus);
    TEST_ASSERT_EQUAL(WE_FACT_CHANGED, l74_key(WE_KEY_TAB));
    TEST_ASSERT_EQUAL(l74_ctl("c"), g_l74f.focus);           /* hidden and disabled skipped */
    TEST_ASSERT_EQUAL(WE_FACT_CHANGED, l74_key(' '));
    TEST_ASSERT_EQUAL(0, strcmp(l74_val("c"), "on"));
    TEST_ASSERT_EQUAL(WE_FACT_CHANGED, l74_key(WE_KEY_TAB));
    TEST_ASSERT_EQUAL(l74_ctl("s"), g_l74f.focus);           /* the one in display:none skipped */
    TEST_ASSERT_EQUAL(WE_FACT_CHANGED, l74_key(WE_KEY_TAB));
    TEST_ASSERT_EQUAL(l74_ctl("sb"), g_l74f.focus);
    TEST_ASSERT_EQUAL(WE_FACT_CHANGED, l74_key(WE_KEY_TAB));
    TEST_ASSERT_EQUAL(l74_ctl("a"), g_l74f.focus);           /* wraps */
    TEST_ASSERT_EQUAL(WE_FACT_CHANGED, l74_key(WE_KEY_SHIFT_TAB));
    TEST_ASSERT_EQUAL(l74_ctl("sb"), g_l74f.focus);
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l74_key(WE_KEY_ENTER));
    TEST_ASSERT_EQUAL(WE_FACT_CHANGED, l74_key(WE_KEY_ESC));
    TEST_ASSERT_EQUAL(-1, g_l74f.focus);
    TEST_ASSERT_EQUAL(WE_FACT_CHANGED, l74_key(WE_KEY_SHIFT_TAB));
    TEST_ASSERT_EQUAL(l74_ctl("sb"), g_l74f.focus);           /* from no focus: the last */
    l74_done();
    l74_open("<form><input id=c type=checkbox><input id=x type=submit name=x value=X></form>");
    (void)l74_click("c");
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l74_key(WE_KEY_ENTER));  /* Enter on a box: implicit submission */
    TEST_ASSERT_EQUAL(l74_ctl("x"), g_l74f.submitter);
    l74_done();
    l74_open("<input id=c type=checkbox><input id=t>");
    (void)l74_click("t");
    TEST_ASSERT_EQUAL(WE_FACT_NONE, l74_key(WE_KEY_ENTER));    /* no form: nothing sent */
    TEST_ASSERT_EQUAL(-1, g_l74f.submit_form);
    (void)l74_click("c");
    TEST_ASSERT_EQUAL(WE_FACT_NONE, l74_key(WE_KEY_ENTER));
    TEST_ASSERT_EQUAL(WE_FACT_NONE, l74_key(WE_KEY_UP));
    l74_done();
}

static void test_l74_post_buttons_and_errors(void) {
    we_form_req_t q;
    char big[600];
    int k;
    l74_open("<form id=f action='/p?keep=1#frag' method=POST enctype='multipart/form-data'>"
             "<input name='a b' value='x=y&z'><button id=b name=btn value=v>Send</button>"
             "<button id=c type=button name=c>c</button><button id=r type=reset>r</button></form>"
             "<form id=g><input id=t name=t><button id=d>go</button></form>");
    TEST_ASSERT_EQUAL(-1, we_form_request(&g_doc, &q));      /* nothing asked yet */
    TEST_ASSERT_EQUAL(WE_FACT_CHANGED, l74_click("c"));
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l74_click("b"));
    TEST_ASSERT_EQUAL(0, strcmp(l74_req(0, &q), "/p?keep=1"));
    TEST_ASSERT_EQUAL(1, q.post);
    TEST_ASSERT_EQUAL(1, q.multipart);
    TEST_ASSERT_EQUAL(0, strcmp(q.body, "a+b=x%3Dy%26z&btn=v"));
    /* a button without type submits; without value it sends an empty value */
    (void)l74_click("t");
    l74_type("\x7e\x2e");
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l74_click("d"));
    TEST_ASSERT_EQUAL(0, strcmp(l74_req(0, &q), "?t=%7E."));
    TEST_ASSERT_EQUAL(0, q.post);
    /* too long for the url (GET) */
    for (k = 0; k < 60; k++) big[k] = '%';
    big[60] = 0;
    TEST_ASSERT_EQUAL(0, we_form_set_text(&g_doc, l74_ctl("t"), big));
    l74_render();
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l74_click("d"));
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));       /* 180 bytes fit */
    l74_done();
    /* a body that does not fit */
    {
        static char html[4000];
        int o = sprintf(html, "<form method=post>");
        for (k = 0; k < 12; k++) o += sprintf(html + o, "<input name=n%d value='%s'>", k, "%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%");
        o += sprintf(html + o, "<input id=s type=submit></form>");
        l74_open(html);
        TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l74_click("s"));
        TEST_ASSERT_EQUAL(-2, we_form_request(&g_doc, &q));
        l74_done();
    }
    {
        static char html[4000];
        int o = sprintf(html, "<form>");
        for (k = 0; k < 6; k++) o += sprintf(html + o, "<input name=n%d value='%s'>", k, "%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%");
        o += sprintf(html + o, "<input id=s type=submit></form>");
        l74_open(html);
        TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l74_click("s"));
        TEST_ASSERT_EQUAL(-2, we_form_request(&g_doc, &q));   /* 6 x 93 bytes: body fits, url not */
        l74_done();
    }
    (void)big;
}

static void test_l74_listbox_label_details_fieldset(void) {
    we_form_req_t q;
    l74_open("<form id=f><select id=m name=m multiple><option id=o1>one<option id=o2 value=2 selected>two"
             "<optgroup label=g><option id=o3>three</optgroup></select>"
             "<select id=l name=l size=3><option id=p1>x<option id=p2>y</select>"
             "<label id=lb for=cb>box</label><input id=cb type=checkbox name=cb>"
             "<label id=lw>wrap <input id=w name=w value=v></label><label id=lx for=nope>none</label>"
             "<details id=dt><summary id=su>more</summary>inside</details>"
             "<fieldset disabled><input id=fd name=fd value=1></fieldset><input id=go type=submit></form>");
    TEST_ASSERT_EQUAL(0, strcmp(l74_val("m"), "2"));
    TEST_ASSERT_EQUAL(WE_FACT_CHANGED, l74_click("o1"));
    TEST_ASSERT_EQUAL(WE_FACT_CHANGED, l74_click("o3"));
    TEST_ASSERT_EQUAL(WE_FACT_CHANGED, l74_click("o2"));     /* multiple: toggles off */
    TEST_ASSERT_EQUAL(WE_FACT_CHANGED, l74_click("p2"));
    TEST_ASSERT_EQUAL(WE_FACT_CHANGED, l74_click("p1"));     /* single: replaces */
    TEST_ASSERT_EQUAL(0x3875D7U, g_doc.nodes[l74_node("p1")].bg);
    TEST_ASSERT_TRUE(g_doc.nodes[l74_node("p2")].bg != 0x3875D7U);
    TEST_ASSERT_EQUAL(WE_FACT_CHANGED, l74_click("lb"));     /* label for=cb toggles it */
    TEST_ASSERT_EQUAL(0, strcmp(l74_val("cb"), "on"));
    TEST_ASSERT_EQUAL(l74_ctl("cb"), g_l74f.focus);
    TEST_ASSERT_EQUAL(l74_ctl("w"), l74_at("lw"));
    TEST_ASSERT_EQUAL(l74_ctl("w"), l74_at("w"));
    TEST_ASSERT_EQUAL(-1, l74_at("lx"));
    TEST_ASSERT_FALSE(l74_text("inside"));
    TEST_ASSERT_TRUE(l74_text(">"));
    TEST_ASSERT_EQUAL(WE_FACT_CHANGED, l74_click("su"));
    TEST_ASSERT_TRUE(l74_text("inside"));
    TEST_ASSERT_FALSE(l74_text(">"));
    TEST_ASSERT_EQUAL(WE_FACT_CHANGED, l74_key(WE_KEY_ENTER));
    TEST_ASSERT_FALSE(l74_text("inside"));
    TEST_ASSERT_EQUAL(WE_FACT_CHANGED, l74_key(' '));
    TEST_ASSERT_TRUE(l74_text("inside"));
    TEST_ASSERT_EQUAL(WE_FACT_NONE, l74_click("fd"));        /* disabled by its fieldset */
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l74_click("go"));
    TEST_ASSERT_EQUAL(0, strcmp(l74_req(0, &q), "?m=one&m=three&l=x&cb=on&w=v"));
    l74_done();
}

static void test_l74_validation(void) {
    we_form_req_t q;
    l74_open("<form id=f><input id=a name=a required><input id=e name=e type=email value=x>"
             "<select id=s name=s required><option value=''>pick<option>z</select>"
             "<input id=c type=checkbox name=c required><input id=go type=submit>"
             "<input id=nv type=submit formnovalidate name=nv value=1></form>"
             "<form id=g novalidate><input id=b name=b required><input id=go2 type=submit></form>");
    TEST_ASSERT_EQUAL(WE_FACT_CHANGED, l74_click("go"));     /* refused, focus on the first invalid */
    TEST_ASSERT_EQUAL(l74_ctl("a"), g_l74f.focus);
    TEST_ASSERT_EQUAL(1U, g_l74f.refused);
    TEST_ASSERT_EQUAL(0U, g_l74f.submits);
    l74_type("v");
    TEST_ASSERT_EQUAL(WE_FACT_CHANGED, l74_key(WE_KEY_ENTER));
    TEST_ASSERT_EQUAL(l74_ctl("e"), g_l74f.focus);           /* x is not an email */
    l74_type("@y");
    TEST_ASSERT_EQUAL(WE_FACT_CHANGED, l74_key(WE_KEY_ENTER));
    TEST_ASSERT_EQUAL(l74_ctl("s"), g_l74f.focus);           /* empty option value */
    TEST_ASSERT_EQUAL(WE_FACT_CHANGED, l74_key(WE_KEY_DOWN));
    TEST_ASSERT_EQUAL(WE_FACT_CHANGED, l74_click("go"));
    TEST_ASSERT_EQUAL(l74_ctl("c"), g_l74f.focus);           /* unchecked box */
    TEST_ASSERT_EQUAL(WE_FACT_CHANGED, l74_key(' '));
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l74_click("go"));
    TEST_ASSERT_EQUAL(0, strcmp(l74_req(0, &q), "?a=v&e=x%40y&s=z&c=on"));
    l74_done();
    l74_open("<form><input id=a name=a required><input id=e type=email name=e value='a@@b'>"
             "<input id=nv type=submit formnovalidate name=nv value=1></form>"
             "<form novalidate><input id=b name=b required><input id=go2 type=submit></form>");
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l74_click("nv"));
    TEST_ASSERT_EQUAL(0, strcmp(l74_req(0, &q), "?a=&e=a%40%40b&nv=1"));
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, l74_click("go2"));
    TEST_ASSERT_EQUAL(0, strcmp(l74_req(0, &q), "?b="));
    l74_done();
}

static void test_l74_live_selectors(void) {
    l74_open("<style>input:focus{color:#000001}form:focus-within{color:#000002}input:required{border-color:#000003}"
             "input:optional{border-color:#000004}input:read-only{background:#000005}input:read-write{background:#000006}"
             "input:invalid{outline-color:#000007}input:valid{outline-color:#000008}input:checked{color:#000009}"
             "textarea:focus-visible{color:#00000a}p:read-only{color:#00000b}div:focus-within{color:#00000c}</style>"
             "<form id=f><input id=a required><input id=b readonly value=1><input id=c type=checkbox>"
             "<input id=e type=email value=bad><textarea id=t></textarea></form><p id=p>p</p><div id=dv>d</div>");
    TEST_ASSERT_EQUAL(0, l73c("f"));
    TEST_ASSERT_EQUAL(3, (int)g_doc.nodes[l74_node("a")].border_color);
    TEST_ASSERT_EQUAL(4, (int)g_doc.nodes[l74_node("b")].border_color);
    TEST_ASSERT_EQUAL(6, (int)g_doc.nodes[l74_node("a")].bg);
    TEST_ASSERT_EQUAL(5, (int)g_doc.nodes[l74_node("b")].bg);
    TEST_ASSERT_EQUAL(5, (int)g_doc.nodes[l74_node("c")].bg);   /* a box is not editable */
    TEST_ASSERT_EQUAL(0xb, l73c("p"));
    (void)l74_click("a");
    TEST_ASSERT_EQUAL(1, l73c("a"));
    TEST_ASSERT_EQUAL(2, l73c("f"));
    TEST_ASSERT_EQUAL(0, l73c("dv"));
    (void)l74_click("c");
    TEST_ASSERT_EQUAL(9, l73c("c"));
    TEST_ASSERT_TRUE(l73c("a") != 1);
    (void)l74_click("t");
    TEST_ASSERT_EQUAL(0xa, l73c("t"));
    l74_done();
}

static void test_l74_validity_selectors(void) {
    l74_open("<style>.x:invalid{color:#000001}.x:valid{color:#000002}</style>"
             "<form><input class=x id=a required><input class=x id=b required value=v>"
             "<input class=x id=e type=email value='a@b'><input class=x id=e2 type=email value='@b'>"
             "<input class=x id=e3 type=email value='a@'><input class=x id=e4 type=email>"
             "<input class=x id=d required disabled><input class=x id=r required readonly>"
             "<input class=x id=h type=hidden required><select class=x id=s required><option>q</select>"
             "<textarea class=x id=t required></textarea><p class=x id=p>p</p></form>");
    TEST_ASSERT_EQUAL(1, l73c("a"));
    TEST_ASSERT_EQUAL(2, l73c("b"));
    TEST_ASSERT_EQUAL(2, l73c("e"));
    TEST_ASSERT_EQUAL(1, l73c("e2"));
    TEST_ASSERT_EQUAL(1, l73c("e3"));
    TEST_ASSERT_EQUAL(2, l73c("e4"));     /* empty and not required */
    TEST_ASSERT_EQUAL(2, l73c("d"));      /* barred from validation */
    TEST_ASSERT_EQUAL(2, l73c("r"));
    TEST_ASSERT_EQUAL(2, l73c("h"));
    TEST_ASSERT_EQUAL(2, l73c("s"));
    TEST_ASSERT_EQUAL(1, l73c("t"));
    TEST_ASSERT_EQUAL(0, l73c("p"));      /* not a control */
    (void)l74_click("t");
    l74_type("z");
    TEST_ASSERT_EQUAL(2, l73c("t"));
    l74_done();
}

static void test_l74_textarea_state_and_placeholder(void) {
    l74_open("<form><textarea id=t name=t placeholder=hint></textarea><input id=i placeholder=ph>"
             "<textarea id=u>keep me</textarea></form>");
    TEST_ASSERT_TRUE(l74_text("hint"));
    TEST_ASSERT_TRUE(l74_text("ph"));
    (void)l74_click("i");
    TEST_ASSERT_FALSE(l74_text("ph"));            /* the caret replaces the placeholder */
    TEST_ASSERT_TRUE(l74_text("_"));
    l74_type("q");
    TEST_ASSERT_TRUE(l74_text("q_") || (l74_text("q") && l74_text("_")));
    TEST_ASSERT_EQUAL(WE_FACT_CHANGED, l74_key(WE_KEY_BACKSPACE));
    TEST_ASSERT_EQUAL(WE_FACT_NONE, l74_key(WE_KEY_BACKSPACE));   /* empty */
    (void)l74_click("t");
    l74_type("abc");
    TEST_ASSERT_FALSE(l74_text("hint"));
    TEST_ASSERT_TRUE(l74_text("abc"));
    (void)l74_click("u");
    TEST_ASSERT_EQUAL(0, strcmp(l74_val("u"), "keep me"));
    l74_type("!");
    TEST_ASSERT_EQUAL(0, strcmp(l74_val("u"), "keep me!"));
    TEST_ASSERT_TRUE(l74_text("keep me!"));
    {
        int k;
        for (k = 0; k < 8; k++) TEST_ASSERT_EQUAL(WE_FACT_CHANGED, l74_key(WE_KEY_BACKSPACE));
    }
    TEST_ASSERT_EQUAL(WE_FACT_NONE, l74_key(WE_KEY_BACKSPACE));
    TEST_ASSERT_FALSE(l74_text("keep"));
    TEST_ASSERT_EQUAL(0, strcmp(l74_val("u"), ""));
    TEST_ASSERT_EQUAL(-1, we_form_value(&g_doc, 99, (char[4]){0}, 4));
    TEST_ASSERT_EQUAL(-1, we_form_value(&g_doc, 0, 0, 4));
    l74_done();
}

static void test_l74_option_end_tags(void) {
    lot52_render("<select id=s><option id=a>1<option id=b>2<optgroup id=g label=x><option id=c>3"
                 "<optgroup id=h label=y><option id=d>4</select>", 400);
    TEST_ASSERT_EQUAL(l74_node("s"), lot52_node("b")->parent);
    TEST_ASSERT_EQUAL(l74_node("s"), lot52_node("g")->parent);
    TEST_ASSERT_EQUAL(l74_node("g"), lot52_node("c")->parent);
    TEST_ASSERT_EQUAL(l74_node("s"), lot52_node("h")->parent);
    TEST_ASSERT_EQUAL(l74_node("h"), lot52_node("d")->parent);
}

static void lot74_forms_run(void) {
    RUN_TEST(test_l74_ordinals_and_static_without_state);
    RUN_TEST(test_l74_state_full);
    RUN_TEST(test_l74_text_typing_caret_and_request);
    RUN_TEST(test_l74_toggles_radio_select_textarea);
    RUN_TEST(test_l74_reset_disabled_readonly_maxlength);
    RUN_TEST(test_l74_reset_own_form);
    RUN_TEST(test_l74_tab_esc_and_space);
    RUN_TEST(test_l74_post_buttons_and_errors);
    RUN_TEST(test_l74_listbox_label_details_fieldset);
    RUN_TEST(test_l74_validation);
    RUN_TEST(test_l74_live_selectors);
    RUN_TEST(test_l74_validity_selectors);
    RUN_TEST(test_l74_textarea_state_and_placeholder);
    RUN_TEST(test_l74_option_end_tags);
}
