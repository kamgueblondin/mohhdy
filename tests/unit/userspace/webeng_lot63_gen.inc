/* Lot 63: ::before/::after boxes, content, counters and quotes. */
#define L63_STYLE(css) "<style>body{margin:0}" css "</style><body>"

/* a failing table case names its input */
#define L63_CASE(cond, what) do { if (!(cond)) printf("  lot63 case: %s\n", what); TEST_ASSERT_TRUE(cond); } while (0)

static int l63_tx(const char *text) {
    const os_fb_web_item_t *it = find_text(text);
    return it ? it->x : -9999;                            /* a missing run fails, not crashes */
}

static const char *l63_text_of(int box);
/* the text of the ::before (kind 1) or ::after (kind 2) box of element id */
static const char *l63_gen(const char *id, int kind) {
    int e = we_find_id(&g_doc, id), c = e >= 0 ? g_doc.nodes[e].first : -1, box = -1;
    for (; c >= 0; c = g_doc.nodes[c].next) if (g_doc.nodes[c].pseudo_kind == kind) box = c;
    return box >= 0 ? l63_text_of(box) : "(none)";
}

static int l63_child(int parent, int nth) {
    int c = g_doc.nodes[parent].first;
    while (c >= 0 && nth-- > 0) c = g_doc.nodes[c].next;
    return c;
}

static const char *l63_text_of(int box) {
    static char buf[WE_CONTENT_TEXT_MAX + 1];
    int t = box >= 0 ? g_doc.nodes[box].first : -1;
    buf[0] = 0;
    if (t >= 0 && g_doc.nodes[t].is_text) {
        memcpy(buf, g_doc.pool + g_doc.nodes[t].text_off, g_doc.nodes[t].text_len);
        buf[g_doc.nodes[t].text_len] = 0;
    }
    return buf;
}

static void test_l63_gen_before_after_boxes(void) {
    int a;
    lot52_render(L63_STYLE("p{margin:0}p::before{content:\"<\"}p:after{content:'>'}") "<p id=a>x<b>y</b></p></body>", 320);
    a = we_find_id(&g_doc, "a");
    TEST_ASSERT_EQUAL_STRING("::before", g_doc.nodes[l63_child(a, 0)].tag);   /* first child */
    TEST_ASSERT_EQUAL(WE_PSEUDO_BEFORE, g_doc.nodes[l63_child(a, 0)].pseudo_kind);
    TEST_ASSERT_EQUAL_STRING("::after", g_doc.nodes[g_doc.nodes[a].last].tag); /* last child */
    TEST_ASSERT_EQUAL(-1, g_doc.nodes[g_doc.nodes[a].last].next);
    TEST_ASSERT_EQUAL_STRING("<", l63_text_of(l63_child(a, 0)));
    TEST_ASSERT_EQUAL_STRING(">", l63_text_of(g_doc.nodes[a].last));
    TEST_ASSERT_EQUAL(0, strcmp(g_doc.nodes[l63_child(a, 2)].tag, "b"));   /* the source children keep their order */
    TEST_ASSERT_EQUAL(0, l63_tx("<"));                    /* inline boxes in the line */
    TEST_ASSERT_EQUAL(8, l63_tx("x"));
    TEST_ASSERT_EQUAL(24, l63_tx(">"));
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
    lot52_render(L63_STYLE("p{margin:0}p::after{content:'z'}") "<p id=a></p></body>", 320);
    a = we_find_id(&g_doc, "a");
    TEST_ASSERT_EQUAL(g_doc.nodes[a].first, g_doc.nodes[a].last);   /* empty element: only child */
    TEST_ASSERT_NOT_NULL(find_text("z"));
    lot52_render(L63_STYLE("p{margin:0}p::before{content:'z'}#b::before{content:none}") "<p id=a></p><p id=c>q</p><p id=b>r</p></body>", 320);
    TEST_ASSERT_EQUAL(g_doc.nodes[we_find_id(&g_doc, "a")].first, g_doc.nodes[we_find_id(&g_doc, "a")].last);
    TEST_ASSERT_EQUAL(1, g_doc.nodes[g_doc.nodes[we_find_id(&g_doc, "c")].last].is_text);
    TEST_ASSERT_EQUAL(8, l63_tx("q"));                    /* before an existing child */
    TEST_ASSERT_EQUAL(0, l63_tx("r"));
    TEST_ASSERT_EQUAL_STRING("(none)", l63_gen("b", WE_PSEUDO_BEFORE));
}

static void test_l63_gen_cascade(void) {
    lot52_render(L63_STYLE("#a::before{content:'B';color:#ff0000}p::before{content:'A';color:#0000ff}p{color:#00ff00}")
                 "<p id=a style='margin:0;color:#00ff00'>x</p><p id=c style='margin:0'>y</p></body>", 320);
    TEST_ASSERT_NOT_NULL(find_text("B"));                 /* the id rule wins over the later type rule */
    TEST_ASSERT_EQUAL(0xFF0000, find_text("B")->color);
    TEST_ASSERT_EQUAL(0x0000FF, find_text("A")->color);
    TEST_ASSERT_EQUAL(0x00FF00, find_text("x")->color);   /* element rules do not reach the box */
    lot52_render(L63_STYLE("p::before{content:'k'}p{font-weight:bold}") "<p style='margin:0;content:\"no\";color:#ff0000'>x</p></body>", 320);
    TEST_ASSERT_NOT_NULL(find_text("k"));                 /* inline style stays on the element */
    TEST_ASSERT_EQUAL(0xFF0000, find_text("k")->color);   /* inherited from the element */
    TEST_ASSERT_EQUAL(1, find_text("k")->bold);
    lot52_render(L63_STYLE("p::before{content:'1' !important}p::before{content:'2'}") "<p id=p style='margin:0'>x</p></body>", 320);
    TEST_ASSERT_EQUAL_STRING("1", l63_gen("p", WE_PSEUDO_BEFORE));   /* important pass */
}

static void test_l63_gen_none_and_display(void) {
    uint32_t base;
    lot52_render(L63_STYLE("p{margin:0}") "<p>x</p><p>y</p></body>", 320);
    base = g_doc.nnodes;
    lot52_render(L63_STYLE("p{margin:0}p::before{content:'a'}p::before{content:none}p::after{content:'b';content:normal}")
                 "<p>x</p><p>y</p></body>", 320);
    TEST_ASSERT_EQUAL(base, g_doc.nnodes);                /* none and normal make no box */
    lot52_render(L63_STYLE("p{margin:0}p::before{content:'a';display:none}#h{display:none}#h::after{content:'c'}"
                           "img::before{content:'i'}#v::after{content:'s';display:block}")
                 "<p>x</p><div id=h>h</div><img id=i src=/no.png alt=q><div id=v>t</div></body>", 320);
    TEST_ASSERT_NOT_NULL(find_text("x"));
    TEST_ASSERT_NULL(find_text("a"));                     /* display none box */
    TEST_ASSERT_NULL(find_text("c"));                     /* inside a display none element */
    TEST_ASSERT_EQUAL_STRING("(none)", l63_gen("h", WE_PSEUDO_AFTER));
    TEST_ASSERT_EQUAL_STRING("(none)", l63_gen("i", WE_PSEUDO_BEFORE));   /* replaced elements get none */
    TEST_ASSERT_NOT_NULL(find_text("s"));
    TEST_ASSERT_EQUAL(0, l63_tx("s"));                    /* a block box takes its own line */
    TEST_ASSERT_EQUAL(find_text("t")->y + 10, find_text("s")->y);
}

static void test_l63_gen_clearfix_and_flex(void) {
    lot52_render(L63_STYLE(".c::before{content:'';display:block;clear:both;height:4px}.c::after{content:'z';display:block;clear:both}")
                 "<div class=c id=a>q<div style='float:left;width:20px;height:30px;background:#ff0000'></div>w</div></body>", 320);
    TEST_ASSERT_EQUAL(4, find_text("q")->y - 1);          /* the empty block takes its height */
    TEST_ASSERT_EQUAL(44, find_text("z")->y - 1);         /* the cleared block sits below the float */
    TEST_ASSERT_EQUAL(-1, g_doc.nodes[g_doc.nodes[we_find_id(&g_doc, "a")].first].first);   /* "" adds no text */
    lot52_render(L63_STYLE(".c::after{content:'z';display:block}")
                 "<div class=c id=a><div style='float:left;width:20px;height:30px;background:#ff0000'></div>w</div></body>", 320);
    TEST_ASSERT_EQUAL(20, l63_tx("z"));                   /* without clear it flows beside the float */
    lot52_render(L63_STYLE("#f{display:flex}#f::before{content:'L'}#f::after{content:'R'}")
                 "<div id=f><span>m</span></div></body>", 320);
    TEST_ASSERT_EQUAL(0, l63_tx("L"));                    /* flex items like the span between them */
    TEST_ASSERT_EQUAL(106, l63_tx("m"));
    TEST_ASSERT_EQUAL(212, l63_tx("R"));
}

static void test_l63_gen_strings_and_escapes(void) {
    int a;
    lot52_render(L63_STYLE("p{margin:0}#a::before{content:\"\\41 B\\\"\\\\\" '\\'' \"\\2014\"}"
                           "#b::before{content:\"x\\Ay\"}#c::before{content:\"a;b}c\"}#d::before{content:\"l\\\nm\" \"\\4a\"}")
                 "<p id=a>-</p><p id=b style='white-space:pre'>-</p><p id=c>-</p><p id=d>-</p></body>", 320);
    a = we_find_id(&g_doc, "a");
    TEST_ASSERT_EQUAL_STRING("AB\"\\'?", l63_text_of(g_doc.nodes[a].first));   /* hex, quote, backslash, non-ASCII */
    TEST_ASSERT_EQUAL_STRING("x\ny", l63_text_of(g_doc.nodes[we_find_id(&g_doc, "b")].first));
    TEST_ASSERT_EQUAL(find_text("x")->y + 10, find_text("y")->y);             /* \A breaks a pre line */
    TEST_ASSERT_EQUAL_STRING("a;b}c", l63_text_of(g_doc.nodes[we_find_id(&g_doc, "c")].first));
    TEST_ASSERT_EQUAL_STRING("lmJ", l63_text_of(g_doc.nodes[we_find_id(&g_doc, "d")].first));
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l63_gen_attr(void) {
    lot52_render(L63_STYLE("a::after{content:' (' attr(href) ')'}div::before{content:attr(id) '/' attr(class) '/' attr(src) '/' attr(alt) '/' attr(title) '.'}")
                 "<a id=a href=u.html>x</a><div id=d class='k m' src=s.png alt=t title=no>y</div><p id=p><div>z</div></p></body>", 320);
    TEST_ASSERT_EQUAL_STRING(" (u.html)", l63_gen("a", WE_PSEUDO_AFTER));
    TEST_ASSERT_EQUAL_STRING("d/k m/s.png/t/.", l63_gen("d", WE_PSEUDO_BEFORE));   /* a missing attribute gives "" */
    TEST_ASSERT_NOT_NULL(find_text("////."));
}

static void test_l63_gen_refusals(void) {
    static const char *const bad[] = { "url(x)", "\"x", "counter(a,bogus)", "counter(a", "attr(id", "attr(id x)", "foo",
        "\"a\"x", "counter(a b)", "counters(a)", "counters(a,b)", "counters(a,\"x)", "open-quote(", "counter(1)",
        "counter()", "rgb(a)", "counter(a,)", "\"a\nb\"", "\"\"open-quote2" };
    unsigned k;
    for (k = 0; k < sizeof(bad) / sizeof(bad[0]); k++) {
        char html[256];
        snprintf(html, sizeof(html), L63_STYLE("p{margin:0}p::before{content:'ok'}p::before{content:%s}") "<p id=p>x</p></body>", bad[k]);
        lot52_render(html, 320);
        L63_CASE(1 == l61_diag_count("content refused"), bad[k]);
        L63_CASE(strcmp(l63_gen("p", WE_PSEUDO_BEFORE), "ok") == 0, bad[k]);   /* the earlier value stays */
    }
}

static void test_l63_gen_selectors(void) {
    lot52_render(L63_STYLE("p{margin:0}p::before span{content:'a'}p::first-line{color:red}p::before::after{content:'b'}"
                           "p:first-child::before{content:'1'}p:hover::before{content:'h'}p::before{content:'c'}")
                 "<p id=x>x</p><p id=y>y</p></body>", 320);
    TEST_ASSERT_EQUAL(4, l61_diag_count("css selector unsupported"));
    TEST_ASSERT_EQUAL_STRING("1", l63_gen("x", WE_PSEUDO_BEFORE));   /* structural pseudo-class before ::before */
    TEST_ASSERT_EQUAL_STRING("c", l63_gen("y", WE_PSEUDO_BEFORE));
    TEST_ASSERT_EQUAL(4, (int)g_doc.nrules);
    lot52_render(L63_STYLE("p{margin:0}p::before {content:'+'}::before{content:'-'}") "<p id=p>x</p></body>", 320);
    TEST_ASSERT_EQUAL_STRING("+", l63_gen("p", WE_PSEUDO_BEFORE));   /* trailing space allowed, the type adds specificity */
}

static void test_l63_gen_counters(void) {
    lot52_render(L63_STYLE("body{counter-reset:s}h2{margin:0;font-weight:normal}h2::before{counter-increment:s;content:counter(s) '. '}")
                 "<h2 id=a>A</h2><h2 id=b>B</h2><h2 id=c>C</h2></body>", 320);
    TEST_ASSERT_EQUAL_STRING("1. ", l63_gen("a", WE_PSEUDO_BEFORE));
    TEST_ASSERT_EQUAL_STRING("2. ", l63_gen("b", WE_PSEUDO_BEFORE));
    TEST_ASSERT_EQUAL_STRING("3. ", l63_gen("c", WE_PSEUDO_BEFORE));
    TEST_ASSERT_EQUAL(11, find_text("2.")->y);
    lot52_render(L63_STYLE("p{margin:0}#r{counter-reset:a 3 b -2}#r p::before{counter-increment:a 2 b;content:counter(a) ',' counter(b)}"
                           "#s{counter-reset:c 3;counter-increment:c 4;counter-set:c 9 d}#s::before{content:counter(c) counter(d)}"
                           "#t::before{counter-increment:z;counter-set:z 6;content:counter(z) counter(u)}")
                 "<div id=r><p id=x>x</p><p id=y>y</p></div><div id=s>s</div><div id=t>t</div></body>", 320);
    TEST_ASSERT_EQUAL_STRING("5,-1", l63_gen("x", WE_PSEUDO_BEFORE));   /* reset values, increment 2 and default 1 */
    TEST_ASSERT_EQUAL_STRING("7,0", l63_gen("y", WE_PSEUDO_BEFORE));
    TEST_ASSERT_EQUAL_STRING("90", l63_gen("s", WE_PSEUDO_BEFORE));    /* reset, increment, then set (default 0) */
    TEST_ASSERT_EQUAL_STRING("60", l63_gen("t", WE_PSEUDO_BEFORE));    /* missing counters start at 0 */
}

static void test_l63_gen_counter_styles(void) {
    lot52_render(L63_STYLE("p{margin:0;counter-reset:n 4}p::before{content:counter(n,upper-roman) counter(n,lower-alpha) counter(n,decimal-leading-zero) "
                           "counter(n, disc) counter(n,circle) counter(n,square) counter(n,none) counter(n,decimal) counter(n,upper-latin)}")
                 "<p id=p>x</p></body>", 320);
    TEST_ASSERT_EQUAL_STRING("IVd04*o#4D", l63_gen("p", WE_PSEUDO_BEFORE));
}

static void test_l63_gen_counters_nested(void) {
    lot52_render(L63_STYLE("ol{counter-reset:i;margin:0;padding:0;list-style:none}li{display:block}li::before{counter-increment:i;content:counters(i,'.') ' '}"
                           "ul{margin:0;padding:0;list-style:none}ul li::before{content:counters(i,'-',lower-alpha) ':'}")
                 "<ol><li id=a>a</li><li id=b>b<ol><li id=c>c</li><li id=d>d</li></ol></li><li id=e>e</li></ol><ul><li id=u>u</li></ul></body>", 320);
    TEST_ASSERT_EQUAL_STRING("1 ", l63_gen("a", WE_PSEUDO_BEFORE));
    TEST_ASSERT_EQUAL_STRING("2 ", l63_gen("b", WE_PSEUDO_BEFORE));
    TEST_ASSERT_EQUAL_STRING("2.1 ", l63_gen("c", WE_PSEUDO_BEFORE));   /* the nested reset nests */
    TEST_ASSERT_EQUAL_STRING("2.2 ", l63_gen("d", WE_PSEUDO_BEFORE));
    TEST_ASSERT_EQUAL_STRING("3 ", l63_gen("e", WE_PSEUDO_BEFORE));     /* the inner scope ended */
    TEST_ASSERT_EQUAL_STRING("d:", l63_gen("u", WE_PSEUDO_BEFORE));     /* the ol's reset reaches its next sibling */
    lot52_render(L63_STYLE("ul{margin:0;padding:0;list-style:none}li::before{counter-increment:i;content:counter(i)}")
                 "<ul><li id=a>a</li><li id=b>b</li></ul></body>", 320);
    TEST_ASSERT_EQUAL_STRING("1", l63_gen("a", WE_PSEUDO_BEFORE));     /* no reset: each box makes its own */
    TEST_ASSERT_EQUAL_STRING("1", l63_gen("b", WE_PSEUDO_BEFORE));
    lot52_render(L63_STYLE("p{margin:0;counter-reset:v 2}#q{counter-reset:v 7}p::after,#q::after,#s::after{content:counters(v,'<>')}")
                 "<p id=p><span id=s><b id=q>x</b></span></p></body>", 320);
    TEST_ASSERT_EQUAL_STRING("2<>7", l63_gen("q", WE_PSEUDO_AFTER));
    TEST_ASSERT_EQUAL_STRING("2<>7", l63_gen("s", WE_PSEUDO_AFTER));    /* a following sibling of b is in scope */
    TEST_ASSERT_EQUAL_STRING("2", l63_gen("p", WE_PSEUDO_AFTER));       /* the scope ended with the span */
}

static void test_l63_gen_counter_scope(void) {
    lot52_render(L63_STYLE("p{margin:0}.r{counter-reset:x 5}.u::before{counter-increment:x;content:counter(x) counters(x,'.')}")
                 "<div><p class=r>a</p><p class=u id=b>b</p><p class=r>c</p><p class=u id=d>d</p></div><p class=u id=e>e</p></body>", 320);
    TEST_ASSERT_EQUAL_STRING("66", l63_gen("b", WE_PSEUDO_BEFORE));    /* a sibling's reset is in scope */
    TEST_ASSERT_EQUAL_STRING("66", l63_gen("d", WE_PSEUDO_BEFORE));    /* a sibling reset replaces, it does not nest */
    TEST_ASSERT_EQUAL_STRING("11", l63_gen("e", WE_PSEUDO_BEFORE));    /* the scope ends with the parent */
    lot52_render(L63_STYLE("p{margin:0}p::before{content:counter(list-item) ') '}")
                 "<ol start=3><li><p id=a>a</p></li><li><p id=b>b</p></li></ol><p id=c>c</p></body>", 320);
    TEST_ASSERT_EQUAL_STRING("3) ", l63_gen("a", WE_PSEUDO_BEFORE));   /* the implicit list-item counter */
    TEST_ASSERT_EQUAL_STRING("4) ", l63_gen("b", WE_PSEUDO_BEFORE));
    TEST_ASSERT_EQUAL_STRING("0) ", l63_gen("c", WE_PSEUDO_BEFORE));
    lot52_render(L63_STYLE("body{counter-reset:k}p{margin:0}#h{display:none;counter-increment:k 5}p::before{counter-increment:k;content:counter(k)}")
                 "<div id=h>h</div><p id=a>a</p><p id=b style='visibility:hidden'>b</p><p id=c>c</p></body>", 320);
    TEST_ASSERT_EQUAL_STRING("1", l63_gen("a", WE_PSEUDO_BEFORE));     /* display none does not count */
    TEST_ASSERT_EQUAL_STRING("3", l63_gen("c", WE_PSEUDO_BEFORE));     /* visibility hidden does */
    lot52_render(L63_STYLE("p{margin:0;counter-increment:k 999990}p::before{content:counter(k)}")
                 "<p>a</p><p id=b>b</p></body>", 320);
    TEST_ASSERT_EQUAL_STRING("999999", l63_gen("b", WE_PSEUDO_BEFORE));   /* values saturate */
}

static void test_l63_gen_counter_refusals_and_budget(void) {
    static const char *const bad[] = { "none 1", "a b c d e", "a 1.5", "inherit", "a,b", "a -", "a 12345678", "1", "initial", "unset", "a 2x" };
    unsigned k;
    for (k = 0; k < sizeof(bad) / sizeof(bad[0]); k++) {
        char html[256];
        snprintf(html, sizeof(html), L63_STYLE("p{margin:0;counter-reset:q 7;counter-reset:%s}p::before{content:counter(q)}") "<p id=p>x</p></body>", bad[k]);
        lot52_render(html, 320);
        L63_CASE(1 == l61_diag_count("counter refused"), bad[k]);
        L63_CASE(strcmp(l63_gen("p", WE_PSEUDO_BEFORE), "7") == 0, bad[k]);
    }
    lot52_render(L63_STYLE("p{margin:0;counter-reset:q 7;counter-reset:none}p::before{content:counter(q) counter(w)}p{counter-increment:w +3 a1 1234567}")
                 "<p id=p>x</p></body>", 320);
    TEST_ASSERT_EQUAL_STRING("03", l63_gen("p", WE_PSEUDO_BEFORE));   /* none clears, +3 and 7 digits are valid */
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
    lot52_render(L63_STYLE("div{counter-reset:a b c d}p{margin:0}p::before{content:counters(d,'.')}")
                 "<div><div><div><div><div><p id=p>x</p></div></div></div></div></div></body>", 320);
    TEST_ASSERT_EQUAL(4, l61_diag_count("counter budget"));   /* 16 instances, the fifth level overflows */
    TEST_ASSERT_EQUAL_STRING("0.0.0.0", l63_gen("p", WE_PSEUDO_BEFORE));
}

static void test_l63_gen_quotes(void) {
    lot52_render(L63_STYLE("p{margin:0}") "<p><q id=a>a <q id=b>b</q> c</q></p></body>", 320);
    TEST_ASSERT_EQUAL_STRING("\"", l63_gen("a", WE_PSEUDO_BEFORE));    /* UA quotes, nested level */
    TEST_ASSERT_EQUAL_STRING("'", l63_gen("b", WE_PSEUDO_BEFORE));
    TEST_ASSERT_EQUAL_STRING("'", l63_gen("b", WE_PSEUDO_AFTER));
    TEST_ASSERT_EQUAL_STRING("\"", l63_gen("a", WE_PSEUDO_AFTER));
    TEST_ASSERT_EQUAL(24, l63_tx("'"));
    lot52_render(L63_STYLE("p{margin:0}#l{quotes:'<' '>' '[' ']'}#n{quotes:none}#m{quotes:'(' ')'}#m q{quotes:auto}q.x::before{content:none}")
                 "<p id=l><q id=a>a<q id=b>b<q id=c>c</q></q></q></p><p id=n><q id=d>d</q></p><p id=m><q id=e>e</q></p><p><q id=f class=x>f</q></p></body>", 320);
    TEST_ASSERT_EQUAL_STRING("<", l63_gen("a", WE_PSEUDO_BEFORE));
    TEST_ASSERT_EQUAL_STRING("[", l63_gen("b", WE_PSEUDO_BEFORE));
    TEST_ASSERT_EQUAL_STRING("[", l63_gen("c", WE_PSEUDO_BEFORE));     /* the last pair past the list */
    TEST_ASSERT_EQUAL_STRING("]", l63_gen("c", WE_PSEUDO_AFTER));
    TEST_ASSERT_EQUAL_STRING(">", l63_gen("a", WE_PSEUDO_AFTER));
    TEST_ASSERT_EQUAL_STRING("", l63_gen("d", WE_PSEUDO_BEFORE));      /* none: a box without text */
    TEST_ASSERT_EQUAL_STRING("\"", l63_gen("e", WE_PSEUDO_BEFORE));    /* auto resets to the UA pairs */
    TEST_ASSERT_EQUAL_STRING("(none)", l63_gen("f", WE_PSEUDO_BEFORE));
    TEST_ASSERT_EQUAL_STRING("", l63_gen("f", WE_PSEUDO_AFTER));       /* no open quote, so no close quote */
    lot52_render(L63_STYLE("p{margin:0}#a::before{content:close-quote 'x' open-quote no-open-quote}#a::after{content:no-close-quote close-quote close-quote}"
                           "#b::before{content:open-quote}")
                 "<p id=a>-</p><p id=b>-</p></body>", 320);
    TEST_ASSERT_EQUAL_STRING("x\"", l63_gen("a", WE_PSEUDO_BEFORE));   /* close at depth 0 paints nothing */
    TEST_ASSERT_EQUAL_STRING("\"", l63_gen("a", WE_PSEUDO_AFTER));     /* no- forms move the depth only */
    TEST_ASSERT_EQUAL_STRING("\"", l63_gen("b", WE_PSEUDO_BEFORE));    /* the depth carries on through the document */
}

static void test_l63_gen_quotes_refusals(void) {
    static const char *const bad[] = { "'a'", "'a' 'b' 'c'", "x y", "'abcd' 'b'", "'1' '2' '3' '4' '5' '6'", "'a' \"b" };
    unsigned k;
    for (k = 0; k < sizeof(bad) / sizeof(bad[0]); k++) {
        char html[256];
        snprintf(html, sizeof(html), L63_STYLE("p{margin:0;quotes:'{' '}';quotes:%s}") "<p><q id=q>x</q></p></body>", bad[k]);
        lot52_render(html, 320);
        L63_CASE(1 == l61_diag_count("quotes refused"), bad[k]);
        L63_CASE(strcmp(l63_gen("q", WE_PSEUDO_BEFORE), "{") == 0, bad[k]);
    }
}

static void test_l63_gen_budgets(void) {
    static char html[1600];
    int k, at;
    at = snprintf(html, sizeof(html), L63_STYLE("b::before{content:'x'}"));
    for (k = 0; k < 90; k++) at += snprintf(html + at, sizeof(html) - (size_t)at, "<b></b>");
    snprintf(html + at, sizeof(html) - (size_t)at, "</body>");
    lot52_render(html, 640);
    TEST_ASSERT_EQUAL(WE_MAX_NODES, (int)g_doc.nnodes);
    TEST_ASSERT_TRUE(l61_diag_count("generated content budget") >= 1);
    lot52_render(L63_STYLE("p{margin:0}p::before{content:'0123456789012345678901234567890123456789012345678901234567890123456789"
                           "01234567890123456789012345678'}") "<p id=a>x</p></body>", 640);
    TEST_ASSERT_EQUAL(1, l61_diag_count("generated content truncated"));
    TEST_ASSERT_EQUAL(WE_CONTENT_TEXT_MAX, (int)g_doc.nodes[g_doc.nodes[g_doc.nodes[we_find_id(&g_doc, "a")].first].first].text_len);
}

static void test_l63_gen_restyle(void) {
    uint32_t nodes, items;
    lot52_render(L63_STYLE("p{margin:0}p::before{content:'b'}p::after{content:'a'}") "<p id=p>x</p><p>y</p></body>", 320);
    nodes = g_doc.nnodes; items = g_doc.nitems;
    TEST_ASSERT_EQUAL(0, we_style(&g_doc));               /* a second pass drops and rebuilds the boxes */
    TEST_ASSERT_EQUAL(0, we_layout(&g_doc, 320));
    TEST_ASSERT_EQUAL(nodes, g_doc.nnodes);
    TEST_ASSERT_EQUAL(items, g_doc.nitems);
    TEST_ASSERT_EQUAL_STRING("b", l63_gen("p", WE_PSEUDO_BEFORE));
    TEST_ASSERT_EQUAL_STRING("a", l63_gen("p", WE_PSEUDO_AFTER));
    TEST_ASSERT_EQUAL(1, g_doc.nodes[l63_child(we_find_id(&g_doc, "p"), 1)].is_text);
    TEST_ASSERT_EQUAL(-1, l63_child(we_find_id(&g_doc, "p"), 3));   /* before, text, after: no duplicate */
}

static void lot63_gen_run(void) {
    RUN_TEST(test_l63_gen_before_after_boxes);
    RUN_TEST(test_l63_gen_cascade);
    RUN_TEST(test_l63_gen_none_and_display);
    RUN_TEST(test_l63_gen_clearfix_and_flex);
    RUN_TEST(test_l63_gen_strings_and_escapes);
    RUN_TEST(test_l63_gen_attr);
    RUN_TEST(test_l63_gen_refusals);
    RUN_TEST(test_l63_gen_selectors);
    RUN_TEST(test_l63_gen_counters);
    RUN_TEST(test_l63_gen_counter_styles);
    RUN_TEST(test_l63_gen_counters_nested);
    RUN_TEST(test_l63_gen_counter_scope);
    RUN_TEST(test_l63_gen_counter_refusals_and_budget);
    RUN_TEST(test_l63_gen_quotes);
    RUN_TEST(test_l63_gen_quotes_refusals);
    RUN_TEST(test_l63_gen_budgets);
    RUN_TEST(test_l63_gen_restyle);
}
