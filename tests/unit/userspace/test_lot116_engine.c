/* Lot 116 (US-031): the engine's line-height (a number inherited as a
 * number, px with hundredths carried from line to line, em and
 * percentages of the element's own font, 1..400 px), the engine's math
 * bounds (32 nodes, 8 deep, 8 arguments), the style attribute's length,
 * and the reader's Lot 116 values laid out by the engine (wf_html,
 * wf_sitecss, webeng). Hand cases next to the generated test_lot116_*.c;
 * expected values follow docs/lot116_plan.md. */
#include "../../framework/unity.h"
#include <string.h>
#include <stdio.h>
#include "webeng.h"
#include "wf_html.h"

static we_doc_t g_doc;
static we_forms_t g_forms;
static char g_html[8192], g_out[65536];
static we_node_t g_none;

static void lay(const char *html, int n) {
    we_forms_init(&g_forms);
    we_set_forms(&g_doc, &g_forms);
    TEST_ASSERT_EQUAL(0, we_parse(&g_doc, html, n));
    TEST_ASSERT_EQUAL(0, we_style(&g_doc));
    TEST_ASSERT_EQUAL(0, we_layout_viewport(&g_doc, 640, 480));
}

static void lays(const char *html) { lay(html, (int)strlen(html)); }

/* the element with this id (a blank node with h -1 when none) */
static const we_node_t *el(const char *id) {
    int i = we_find_id(&g_doc, id);
    g_none.h = -1;
    g_none.w = -1;
    return i >= 0 ? &g_doc.nodes[i] : &g_none;
}

/* the k-th element (from 0) with this tag */
static const we_node_t *nth(const char *t, int k) {
    int i;
    for (i = 0; i < (int)g_doc.nnodes; i++)
        if (!strcmp(g_doc.nodes[i].tag, t) && !k--) return &g_doc.nodes[i];
    g_none.h = -1;
    return &g_none;
}

static int has_diag(const char *t) {
    uint32_t i;
    for (i = 0; i < g_doc.ndiag; i++) if (strstr(g_doc.diag[i], t)) return 1;
    return 0;
}

static int text_y(const char *t) {
    uint32_t i;
    for (i = 0; i < g_doc.nitems; i++)
        if (g_doc.items[i].kind == OS_FB_WEB_TEXT && !strcmp(g_doc.items[i].text, t)) return g_doc.items[i].y;
    return -1000;
}

#define KF "@keyframes k { from { color: #000000 } to { color: #ffffff } } "
#define TL "#i { animation: k 1s linear; animation-timeline: scroll(nearest) } "

/* the reader's page laid out at 640 x 480 */
static void reduce(const char *css, const char *body) {
    wf_html_stats_t st;
    int n;
    snprintf(g_html, sizeof(g_html), "<html><head><style>" KF "%s</style></head><body>%s</body></html>", css, body);
    wf_html_set_sitecss(1);
    n = wf_html_reduce(g_html, (int)strlen(g_html), 0, g_out, 65536, &st);
    wf_html_set_sitecss(0);
    TEST_ASSERT_TRUE(n > 0);
    lay(g_out, n);
}

/* the height of the k-th box scroller along y */
static int sh(int k) {
    int i;
    for (i = 0; i < (int)g_doc.nnodes; i++)
        if ((g_doc.nodes[i].overflow_scroll & 2) && !k--) return g_doc.nodes[i].h;
    return -1;
}

#define L4 "a<br>b<br>c<br>d"

/* a number, px with hundredths, em and a percentage, as the engine keeps
 * them (line_height, lh_unitless, lh_frac) */
static void test_l116_lh_fields(void) {
    lays("<body style='margin:0'><div id=a style='line-height:1.5'>a</div><div id=b style='line-height:15.5px'>b</div>"
         "<div id=c style='font-size:20px;line-height:1.5em'>c</div><div id=d style='font-size:10px;line-height:150%'>d</div>"
         "<div id=e style='line-height:0px'>e</div><div id=f style='line-height:1.0049'>f</div></body>");
    TEST_ASSERT_EQUAL(150, el("a")->line_height);
    TEST_ASSERT_EQUAL(1, el("a")->lh_unitless);
    TEST_ASSERT_EQUAL(0, el("a")->lh_frac);
    TEST_ASSERT_EQUAL(15, el("b")->line_height);
    TEST_ASSERT_EQUAL(0, el("b")->lh_unitless);
    TEST_ASSERT_EQUAL(50, el("b")->lh_frac);
    /* em and % are px of the element's own font at the end of its cascade */
    TEST_ASSERT_EQUAL(30, el("c")->line_height);
    TEST_ASSERT_EQUAL(0, el("c")->lh_unitless);
    TEST_ASSERT_EQUAL(0, el("c")->lh_frac);
    TEST_ASSERT_EQUAL(15, el("d")->line_height);
    TEST_ASSERT_EQUAL(0, el("d")->lh_frac);
    /* 0px is kept as the smallest value (1 hundredth), used as 1 px */
    TEST_ASSERT_EQUAL(0, el("e")->line_height);
    TEST_ASSERT_EQUAL(1, el("e")->lh_frac);
    TEST_ASSERT_EQUAL(1, el("e")->h);
    /* three decimals read, the hundredths rounded half up: 1.004 is 100 */
    TEST_ASSERT_EQUAL(100, el("f")->line_height);
    TEST_ASSERT_EQUAL(16, el("f")->h);
    TEST_ASSERT_EQUAL(24, el("a")->h);      /* 1.5 x 16 px */
}

/* a number inherits as the number (each element's font), px and em
 * inherit as px (CSS Inline 3 4.2) */
static void test_l116_lh_inherit(void) {
    lays("<body style='margin:0'><div style='line-height:1.5'><div id=a style='font-size:32px'>a</div></div>"
         "<div style='line-height:24px'><div id=b style='font-size:32px'>b</div></div>"
         "<div style='line-height:1.5em'><div id=c style='font-size:32px'>c</div></div>"
         "<div style='font-size:10px;line-height:200%'><div id=d style='font-size:32px'>d</div></div></body>");
    TEST_ASSERT_EQUAL(48, el("a")->h);
    TEST_ASSERT_EQUAL(150, el("a")->line_height);
    TEST_ASSERT_EQUAL(1, el("a")->lh_unitless);
    TEST_ASSERT_EQUAL(24, el("b")->h);
    TEST_ASSERT_EQUAL(24, el("c")->h);
    TEST_ASSERT_EQUAL(0, el("c")->lh_unitless);
    TEST_ASSERT_EQUAL(20, el("d")->h);
    /* normal under a number is the engine's normal again */
    lays("<body style='margin:0'><div style='line-height:3'><div id=a style='line-height:normal'>a</div></div></body>");
    TEST_ASSERT_EQUAL(10, el("a")->h);
}

/* the hundredths add up from line to line */
static void test_l116_lh_carry(void) {
    lays("<body style='margin:0'><div id=a style='line-height:15.5px'>" L4 "</div></body>");
    TEST_ASSERT_EQUAL(62, el("a")->h);
    TEST_ASSERT_EQUAL(15, text_y("b") - text_y("a"));
    TEST_ASSERT_EQUAL(31, text_y("c") - text_y("a"));
    TEST_ASSERT_EQUAL(46, text_y("d") - text_y("a"));
    lays("<body style='margin:0'><div id=a style='line-height:10.25px'>" L4 "</div></body>");
    TEST_ASSERT_EQUAL(41, el("a")->h);
    TEST_ASSERT_EQUAL(20, text_y("c") - text_y("a"));
    /* a number times a font gives hundredths too: 1.33 x 16 = 21.28 */
    lays("<body style='margin:0'><div id=a style='line-height:1.33'>" L4 "</div></body>");
    TEST_ASSERT_EQUAL(85, el("a")->h);
    /* whole px: no carry */
    lays("<body style='margin:0'><div id=a style='line-height:21px'>" L4 "</div></body>");
    TEST_ASSERT_EQUAL(84, el("a")->h);
}

/* the carry only on line boxes of the plain line height: a taller line
 * (an inline box of 40 px) takes no hundredths; the whole px line height
 * is cut for line-clamp and columns (a rounded one would join the 20 px
 * steps of 20.6px into one line) */
static void test_l116_lh_carry_bounds(void) {
    lays("<body style='margin:0'><div id=a style='line-height:15.5px'>a<br>"
         "<span style='display:inline-block;width:10px;height:40px'></span>x<br>c<br>d</div></body>");
    TEST_ASSERT_EQUAL(55, text_y("c") - text_y("a"));     /* 15, then 40 (the 50 hundredths kept) */
    TEST_ASSERT_EQUAL(16, text_y("d") - text_y("c"));     /* c's line takes them: 100 */
    TEST_ASSERT_EQUAL(86, el("a")->h);
    /* 20.6px: steps 20, 21, 20 */
    lays("<body style='margin:0'><div id=a style='width:200px;line-height:20.6px'>a<br>b<br>c<br>d</div></body>");
    TEST_ASSERT_EQUAL(20, text_y("b") - text_y("a"));
    TEST_ASSERT_EQUAL(21, text_y("c") - text_y("b"));
    TEST_ASSERT_EQUAL(82, el("a")->h);
    /* line-clamp 2 keeps a and b (b ends with the dots), c and d go; the
     * clamped height counts whole px lines (20 + 20, the last fraction
     * not added: Not done) */
    lays("<body style='margin:0'><div id=a style='width:200px;line-height:20.6px;line-clamp:2'>"
         "a<br>b<br>c<br>d</div><div id=n>after</div></body>");
    TEST_ASSERT_EQUAL(1, text_y("a"));
    TEST_ASSERT_EQUAL(-1000, text_y("c"));
    TEST_ASSERT_EQUAL(-1000, text_y("d"));
    TEST_ASSERT_EQUAL(40, el("a")->h);
    TEST_ASSERT_EQUAL(41, text_y("after"));
}

/* an orthogonal (vertical) root contributes its lines times the line
 * height in hundredths, cut as the carried line boxes add up: "ab cd" in
 * 16 px is estimated at 3 lines (40 px of text), 3 x 20.6 = 61.8 gives
 * 61; the root itself lays out its 2 lines in 20 + 21 = 41 px */
static void test_l116_lh_vertical(void) {
    lays("<body style='margin:0'><div id=s style='display:inline-block'><div id=v style='writing-mode:vertical-rl;"
         "height:16px;line-height:20.6px'>ab cd</div></div></body>");
    TEST_ASSERT_EQUAL(61, el("s")->w);
    TEST_ASSERT_EQUAL(41, el("v")->w);
    lays("<body style='margin:0'><div id=s style='display:inline-block'><div id=v style='writing-mode:vertical-rl;"
         "height:16px;line-height:20.3px'>ab cd</div></div></body>");
    TEST_ASSERT_EQUAL(60, el("s")->w);       /* 60.9: cut, not rounded */
    TEST_ASSERT_EQUAL(40, el("v")->w);
    lays("<body style='margin:0'><div id=s style='display:inline-block'><div id=v style='writing-mode:vertical-rl;"
         "height:16px;line-height:21px'>ab cd</div></div></body>");
    TEST_ASSERT_EQUAL(63, el("s")->w);       /* whole px as before */
}

/* the bounds: 400 px, a number or em of 40, 4000 %; past them refused */
static void test_l116_lh_bounds(void) {
    lays("<body style='margin:0'><div id=a style='line-height:20px;line-height:400px'>a</div>"
         "<div id=b style='line-height:20px;line-height:401px'>b</div>"
         "<div id=c style='line-height:2;line-height:40'>c</div>"
         "<div id=d style='line-height:2;line-height:40.01'>d</div>"
         "<div id=e style='line-height:2;line-height:4001%'>e</div>"
         "<div id=f style='line-height:2;line-height:-1px'>f</div>"
         "<div id=g style='line-height:9px'>g</div>"
         "<div id=h style='font-size:20px;line-height:40em'>h</div></body>");
    TEST_ASSERT_EQUAL(400, el("a")->h);
    TEST_ASSERT_EQUAL(20, el("b")->h);
    TEST_ASSERT_EQUAL(400, el("c")->h);     /* 40 x 16 = 640 px, used 400 */
    TEST_ASSERT_EQUAL(32, el("d")->h);
    TEST_ASSERT_EQUAL(32, el("e")->h);
    TEST_ASSERT_EQUAL(32, el("f")->h);
    TEST_ASSERT_EQUAL(9, el("g")->h);       /* was refused below 10 px */
    TEST_ASSERT_EQUAL(400, el("h")->h);     /* 800 px kept as 400 */
    TEST_ASSERT_EQUAL(400, el("h")->line_height);
    TEST_ASSERT_TRUE(has_diag("line height refused"));
}

/* the math bounds: 8 arguments, 8 deep, 32 nodes; one past each refused */
static void test_l116_math_bounds(void) {
    lays("<body style='margin:0'><div style='width:200px'>"
         "<div id=a style='height:5px;width:50px;width:min(90px,80px,70px,60px,50px,40px,30px,20px)'></div>"
         "<div id=b style='height:5px;width:50px;width:min(90px,80px,70px,60px,50px,40px,30px,20px,10px)'></div>"
         "<div id=c style='height:5px;width:50px;width:calc(calc(calc(calc(calc(calc(calc(calc(7px))))))))'></div>"
         "<div id=d style='height:5px;width:50px;width:calc(calc(calc(calc(calc(calc(calc(calc(calc(7px)))))))))'></div>"
         "<div id=e style='height:5px;width:50px;width:clamp(1px + 1px + 1px + 1px + 1px + 1px + 1px + 1px + 1px + 1px"
         " + 1px + 1px + 1px + 1px + 1px,2px,30%)'></div>"
         "<div id=f style='height:5px;width:50px;width:clamp(1px + 1px + 1px + 1px + 1px + 1px + 1px + 1px + 1px + 1px"
         " + 1px + 1px + 1px + 1px + 1px + 1px,2px,30%)'></div>"
         "</div></body>");
    TEST_ASSERT_EQUAL(20, el("a")->w);
    TEST_ASSERT_EQUAL(15, el("a")->length[WE_LEN_WIDTH].count);     /* 8 literals, 7 min() steps */
    TEST_ASSERT_EQUAL(50, el("b")->w);
    TEST_ASSERT_EQUAL(7, el("c")->w);
    TEST_ASSERT_EQUAL(50, el("d")->w);
    TEST_ASSERT_EQUAL(15, el("e")->w);      /* max(15, min(2, 60)) */
    TEST_ASSERT_EQUAL(WE_MATH_NODES_MAX, el("e")->length[WE_LEN_WIDTH].count);
    TEST_ASSERT_EQUAL(50, el("f")->w);
    TEST_ASSERT_TRUE(has_diag("math length refused"));
    /* the packed node keeps its operator, dimension and operands */
    TEST_ASSERT_EQUAL(1, el("e")->length[WE_LEN_WIDTH].node[0].dimension);
    TEST_ASSERT_EQUAL(1, el("e")->length[WE_LEN_WIDTH].node[0].value);
    /* the 8th and 9th arguments of a percentage min() at a basis */
    lays("<body style='margin:0'><div style='width:200px'><div id=a style='height:5px;width:max(1%,2%,3%,4%,5%,6%,7%,8%)'>"
         "</div></div></body>");
    TEST_ASSERT_EQUAL(16, el("a")->w);
}

/* a style value up to 1279 bytes; other attributes still up to 255 */
static void test_l116_style_attr(void) {
    char h[3000];
    int k;
    strcpy(h, "<body style='margin:0'><div id=a style='width:30px;");
    for (k = 0; k < 900; k++) strcat(h, " ");
    strcat(h, "height:20px'>x</div><div id=b title='");
    for (k = 0; k < 256; k++) strcat(h, "t");
    strcat(h, "'>y</div></body>");
    lays(h);
    TEST_ASSERT_EQUAL(30, el("a")->w);
    TEST_ASSERT_EQUAL(20, el("a")->h);
    TEST_ASSERT_TRUE(has_diag("attribute exceeds limit"));      /* the title */
    TEST_ASSERT_EQUAL(0, el("b")->title_len);
}

/* the reader's line heights: a number passed as the number, fractions */
static void test_l116_reader_lh(void) {
    reduce("body { line-height: 1.25 } #b { font-size: 32px } #c { line-height: 15.5px }",
           "<p id=a>a</p><p id=b>b</p><p id=c>" L4 "</p>");
    TEST_ASSERT_NOT_NULL(strstr(g_out, "line-height:1.25"));
    TEST_ASSERT_EQUAL(20, nth("p", 0)->h);
    TEST_ASSERT_EQUAL(40, nth("p", 1)->h);
    TEST_ASSERT_NOT_NULL(strstr(g_out, "line-height:15.5px"));
    TEST_ASSERT_EQUAL(62, nth("p", 2)->h);
    /* past the old 40 px bound */
    reduce("#a { line-height: 100px } #b { line-height: 5 }", "<p id=a>a</p><p id=b>b</p>");
    TEST_ASSERT_EQUAL(100, nth("p", 0)->h);
    TEST_ASSERT_EQUAL(80, nth("p", 1)->h);
}

/* the reader's box scrollers with the raised math bounds */
static void test_l116_reader_math(void) {
    reduce("#p { overflow: auto; height: 300px } #o { overflow: auto; height: max(min(10%, 50px), min(20%, 40px), "
           "min(30%, 35px), min(5%, 60px), min(1%, 70px)) } " TL "#j { animation: k 1s linear; animation-timeline: scroll(nearest) }",
           "<main><div id=p><p id=j>j</p><div id=o><p id=i>i</p><p>a</p><p>b</p><p>c</p><p>d</p><p>e</p></div></div></main>");
    TEST_ASSERT_NOT_NULL(strstr(g_out, "height:max(min(10%, 50px), min(20%, 40px), min(30%, 35px), min(5%, 60px), "
                                       "min(1%, 70px))"));
    TEST_ASSERT_EQUAL(40, sh(1));       /* max(30, 40, 35, 15, 3) at 300 px */
}

/* html's margin and padding come off the body's percentage width; html's
 * height auto leaves the body's percentage height auto */
static void test_l116_body(void) {
    reduce("html { overflow: hidden; margin: 0 20px; padding: 0 10px } body { overflow: auto; width: 50%; height: 100px; "
           "margin: 0 } " TL, "<p id=i>x</p><p>a</p>");
    TEST_ASSERT_NOT_NULL(strstr(g_out, "width:290px"));
    TEST_ASSERT_EQUAL(290, nth("body", 0)->w);
    reduce("html { overflow: hidden } body { overflow: auto; height: 50%; margin: 0 } " TL, "<p id=i>x</p><p>a</p>");
    TEST_ASSERT_NOT_NULL(strstr(g_out, "height:50%"));
    TEST_ASSERT_TRUE(nth("body", 0)->h < 240);      /* auto: its content, not half the viewport */
    lays("<html><body style='margin:0;height:50%'><p>a</p></body></html>");
    TEST_ASSERT_EQUAL(nth("html", 0)->h, nth("body", 0)->h);
    lays("<html style='height:200px'><body style='margin:0;height:50%'><p>a</p></body></html>");
    TEST_ASSERT_EQUAL(100, nth("body", 0)->h);
}

/* var() invalid at computed-value time: height unset (auto); a var() of
 * the element's own custom properties read per element */
static void test_l116_var_box(void) {
    reduce("#o { overflow: auto; height: 50px; height: var(--nope) } " TL, "<div id=o><p id=i>i</p><p>a</p></div>");
    TEST_ASSERT_NULL(strstr(g_out, "height:50px"));
    TEST_ASSERT_EQUAL(-1, sh(0));
    reduce("#o { --h: 40px; overflow: auto; height: 50px; height: var(--h) } " TL, "<div id=o><p id=i>i</p><p>a</p></div>");
    TEST_ASSERT_NOT_NULL(strstr(g_out, "height:40px"));
    TEST_ASSERT_EQUAL(40, sh(0));
    reduce("#o { --h: red; overflow: auto; height: 50px; height: var(--h) } " TL, "<div id=o><p id=i>i</p><p>a</p></div>");
    TEST_ASSERT_EQUAL(-1, sh(0));
}

/* the QEMU page /lot116/lh.html (scripts/lot67_web_server.py, the same
 * bytes) through the guest's path: reduced with the geometry marks,
 * laid out, its geometry fed back, reduced again; then clicks on the
 * body's bar (tests/scripts/test_qemu_lot116.py checks the same lines) */
static const char k_page[] =
    "<!doctype html><html><head><title>Lot 116 lh</title><style>@keyframes k116 { from { color: #000000 } to { color: #ffffff } }\n"
    "html { overflow: hidden; line-height: 15px; height: calc(26rlh + 10px) }\n"
    "body { overflow: auto; height: 50%; margin: 0; scroll-timeline: --b116; line-height: 1.25 }\n"
    "#i116 { animation: k116 1000ms linear; animation-timeline: scroll(nearest) }\n"
    "#n116 { animation: k116 1000ms linear; animation-timeline: --b116 }\n"
    "#r116 { animation: k116 1000ms linear; animation-timeline: view() }\n"
    "#b116 { font-size: 24px }\n"
    "#q116 { line-height: 20.5px }\n"
    "#w116 { line-height: 45px }\n"
    "#c116 { line-height: 4cap }\n"
    "#e116 { font-size: 2rcap }\n"
    "#d116 { overflow: auto; height: max(5% + 10px, 10%, 2% + 30px, 1rlh + 25px, 25% - 5px) }\n"
    "#v116 { overflow: auto; height: 30px; height: var(--x116) }\n"
    "#dd116, #vv116 { animation: k116 1000ms linear; animation-timeline: scroll(nearest) }\n"
    "</style></head><body><h1>Body 116</h1><p id=\"i116\">ikeep116</p><p id=\"n116\">nkeep116</p>"
    "<p id=\"b116\">bkeep116</p><p id=\"q116\">qkeep116<br>qline116</p><p id=\"w116\">wkeep116</p>"
    "<p id=\"c116\">ckeep116</p><p id=\"e116\">ekeep116</p><div id=\"d116\"><p id=\"dd116\">dkeep116</p>"
    "</div><div id=\"v116\"><p id=\"vv116\">vkeep116</p></div><p>line116 0</p><p>line116 1</p>"
    "<p>line116 2</p><p>line116 3</p><p>line116 4</p><p>line116 5</p><p>line116 6</p><p>"
    "line116 7</p><p>line116 8</p><p>line116 9</p><p id=\"r116\">rkeep116</p><p>line116 0</p>"
    "<p>line116 1</p><p>line116 2</p><p>line116 3</p><p>line116 4</p><p>line116 5</p><p>"
    "fin116</p></body></html>";

static char g_rep[16384];

static void page_pass(int first) {
    wf_html_stats_t st;
    int n;
    wf_html_set_sitecss(1);
    wf_html_set_geommarks(1);
    n = wf_html_reduce(k_page, (int)strlen(k_page), 0, g_out, 65536, &st);
    wf_html_set_geommarks(0);
    TEST_ASSERT_TRUE(n > 0);
    g_out[n] = 0;
    if (first) we_forms_init(&g_forms);
    we_set_forms(&g_doc, &g_forms);
    TEST_ASSERT_EQUAL(0, we_parse(&g_doc, g_out, n));
    TEST_ASSERT_EQUAL(0, we_style(&g_doc));
    TEST_ASSERT_EQUAL(0, we_layout_viewport(&g_doc, 640, 480));
}

static void page_has(const char *want) {
    g_rep[0] = 0;
    TEST_ASSERT_TRUE(wf_html_sitecss_report(g_rep, (int)sizeof(g_rep)) > 0);
    if (!strstr(g_rep, want)) printf("missing: %s\n", want);
    TEST_ASSERT_NOT_NULL(strstr(g_rep, want));
}

static void test_l116_page(void) {
    static wf_geom_t g;
    static const int ys[4] = { 100, 150, 175, 190 }, sc[4] = { 445, 667, 778, 845 };
    static const char *const col[4] = { "#808080", "#bfbfbf", "#dfdfdf", "#f2f2f2" };
    static const char *const rv[4] = { 0, "#393939", "#b9b9b9", 0 };
    char want[96];
    int k;
    wf_html_set_csstime(500);
    wf_html_set_cssgeom(0);
    page_pass(1);
    (void)we_geom_feed(&g_doc, "wg", &g);
    TEST_ASSERT_EQUAL(200, g.body.s_vp);
    TEST_ASSERT_EQUAL(890, g.body.s_rg);
    wf_html_set_cssgeom(&g);
    page_pass(0);
    /* the line heights the engine laid out */
    TEST_ASSERT_EQUAL(40, nth("h1", 0)->h);
    TEST_ASSERT_EQUAL(30, nth("p", 2)->h);       /* #b116: 1.25 x 24 */
    TEST_ASSERT_EQUAL(41, nth("p", 3)->h);       /* #q116: 20 + 21 */
    TEST_ASSERT_EQUAL(45, nth("p", 4)->h);       /* #w116 */
    TEST_ASSERT_EQUAL(28, nth("p", 5)->h);       /* #c116: 4cap */
    TEST_ASSERT_EQUAL(17, nth("p", 6)->h);       /* #e116: 1.25 x 14 */
    TEST_ASSERT_EQUAL(45, nth("div", 0)->h);     /* #d116: 25% - 5px */
    TEST_ASSERT_EQUAL(36, nth("div", 1)->h);     /* #v116: auto */
    TEST_ASSERT_EQUAL(818, nth("p", 19)->y);     /* #r116 */
    page_has("node p #i116 display auto color #000000 lh num 1.25\n");
    page_has("node p #r116 display auto lh num 1.25\n");
    page_has("node p #q116 display auto lh 20.5\n");
    page_has("node p #e116 display auto size 14px lh num 1.25\n");
    for (k = 0; k < 4; k++) {
        TEST_ASSERT_EQUAL(2, we_box_scroll_click(&g_doc, 637, ys[k]));
        /* the GUI lays the page out again, then feeds its geometry */
        TEST_ASSERT_EQUAL(0, we_parse(&g_doc, g_out, (int)strlen(g_out)));
        TEST_ASSERT_EQUAL(0, we_style(&g_doc));
        TEST_ASSERT_EQUAL(0, we_layout_viewport(&g_doc, 640, 480));
        (void)we_geom_feed(&g_doc, "wg", &g);
        TEST_ASSERT_EQUAL(sc[k], g.body.s_sc);
        TEST_ASSERT_EQUAL(890, g.body.s_rg);
        wf_html_set_cssgeom(&g);
        page_pass(0);
        snprintf(want, sizeof(want), "node p #n116 display auto color %s lh num 1.25\n", col[k]);
        page_has(want);
        if (rv[k]) snprintf(want, sizeof(want), "node p #r116 display auto color %s lh num 1.25\n", rv[k]);
        else snprintf(want, sizeof(want), "node p #r116 display auto lh num 1.25\n");
        page_has(want);
    }
    wf_html_set_cssgeom(0);
    wf_html_set_sitecss(0);
    wf_html_set_csstime(0);
    we_set_forms(&g_doc, 0);
}

int main(void) {
    unity_init();
    RUN_TEST(test_l116_lh_fields);
    RUN_TEST(test_l116_lh_inherit);
    RUN_TEST(test_l116_lh_carry);
    RUN_TEST(test_l116_lh_carry_bounds);
    RUN_TEST(test_l116_lh_vertical);
    RUN_TEST(test_l116_lh_bounds);
    RUN_TEST(test_l116_math_bounds);
    RUN_TEST(test_l116_style_attr);
    RUN_TEST(test_l116_reader_lh);
    RUN_TEST(test_l116_reader_math);
    RUN_TEST(test_l116_body);
    RUN_TEST(test_l116_var_box);
    RUN_TEST(test_l116_page);
    unity_print_results();
    unity_cleanup();
    return unity_stats.tests_failed == 0 ? 0 : 1;
}
