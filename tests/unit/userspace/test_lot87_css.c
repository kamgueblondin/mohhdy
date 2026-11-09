/* Lot 87: reader stylesheet - larger tables, attribute operators,
 * structural pseudo-classes, :is/:where/:not, custom properties, every
 * inline declaration, media query lists, @supports, @import and
 * <link rel=stylesheet>, external sheets passed by the caller. */
#include "../../framework/unity.h"
#include "../../../userspace/wf_sitecss.h"
#include <string.h>
#include <stdio.h>

static wf_site_t g_d;

static const wf_site_node_t *by_id(const char *id) {
    int i;
    for (i = 0; i < g_d.nnodes; i++) if (strcmp(g_d.node[i].id, id) == 0) return &g_d.node[i];
    return 0;
}

static int prep(const char *html) {
    return wf_site_prepare(&g_d, html, (int)strlen(html));
}

static void test_attribute_operators(void) {
    const wf_site_node_t *n;
    TEST_ASSERT_EQUAL(0, prep(
        "<html><head><style>"
        "a[href^=\"https\"] { font-weight: bold }"
        "a[href$=\".pdf\"] { color: #ff0000 }"
        "a[href*=\"ads\"] { display: none }"
        "p[lang|=en] { text-align: center }"
        "p[title=\"HELLO\" i] { font-style: italic }"
        "p[title=\"HELLO\"] { text-align: right }"
        "div[class^=\"col-\"] { color: #00ff00 }"
        "span[data-role=note] { text-decoration: underline }"
        "</style></head><body>"
        "<a id=a1 href=\"https://x.test/doc.pdf\">a</a>"
        "<a id=a2 href=\"http://x.test/ads/1\">b</a>"
        "<p id=p1 lang=en-GB>c</p><p id=p2 lang=english>d</p>"
        "<p id=p3 title=hello>e</p>"
        "<div id=d1 class=\"col-4 main\">f</div><div id=d2 class=\"main col-4\">g</div>"
        "<span id=s1 data-role=note>h</span>"
        "</body></html>"));
    n = by_id("a1");
    TEST_ASSERT_NOT_NULL(n);
    TEST_ASSERT_EQUAL(2, n->weight);
    TEST_ASSERT_EQUAL(1, n->color_set);
    TEST_ASSERT_EQUAL(0xff0000, n->color);
    n = by_id("a2");
    TEST_ASSERT_EQUAL(WF_SITE_DISP_NONE, n->disp);
    TEST_ASSERT(n->weight != 2);
    TEST_ASSERT_EQUAL(2, by_id("p1")->align);          /* en-GB starts with en- */
    TEST_ASSERT(by_id("p2")->align != 2);              /* english is not en */
    TEST_ASSERT_EQUAL(2, by_id("p3")->italic);          /* i flag */
    TEST_ASSERT(by_id("p3")->align != 3);               /* case kept without i */
    TEST_ASSERT_EQUAL(1, by_id("d1")->color_set);       /* class string starts with col- */
    TEST_ASSERT_EQUAL(0, by_id("d2")->color_set);
    TEST_ASSERT_EQUAL(2, by_id("s1")->deco);
}

static void test_structural(void) {
    TEST_ASSERT_EQUAL(0, prep(
        "<html><head><style>"
        "p:first-of-type { font-weight: bold }"
        "p:last-of-type { text-align: right }"
        "em:only-child { font-style: italic }"
        "b:only-of-type { color: #010203 }"
        "li:nth-last-child(1) { text-align: center }"
        "li:nth-of-type(2) { font-weight: bold }"
        "li:nth-last-of-type(2) { color: #0000ff }"
        "div:empty { display: none }"
        ":root { text-align: center }"
        "</style></head><body><section>"
        "<h2 id=h>t</h2><p id=p1>a</p><p id=p2>b<em id=e1>x</em></p><p id=p3>c<em>y</em><em>z</em><b id=b1>q</b></p>"
        "</section><ul><li id=l1>1</li><li id=l2>2</li><li id=l3>3</li></ul>"
        "<div id=empty></div><div id=text>t</div><div id=kid><i></i></div>"
        "</body></html>"));
    TEST_ASSERT_EQUAL(2, by_id("p1")->weight);
    TEST_ASSERT(by_id("p2")->weight != 2);
    TEST_ASSERT_EQUAL(3, by_id("p3")->align);
    TEST_ASSERT(by_id("p2")->align != 3);
    TEST_ASSERT_EQUAL(2, by_id("e1")->italic);           /* the only element child */
    TEST_ASSERT_EQUAL(1, by_id("b1")->color_set);        /* only b among em, em, b */
    TEST_ASSERT_EQUAL(2, by_id("l3")->align);
    TEST_ASSERT(by_id("l2")->align != 2);
    TEST_ASSERT_EQUAL(2, by_id("l2")->weight);
    TEST_ASSERT_EQUAL(1, by_id("l2")->color_set);
    TEST_ASSERT_EQUAL(WF_SITE_DISP_NONE, by_id("empty")->disp);
    TEST_ASSERT(by_id("text")->disp != WF_SITE_DISP_NONE);   /* text is not empty */
    TEST_ASSERT(by_id("kid")->disp != WF_SITE_DISP_NONE);    /* an element child */
    TEST_ASSERT_EQUAL(2, g_d.node[0].align);                 /* :root is html */
}

static void test_is_where_not(void) {
    TEST_ASSERT_EQUAL(0, prep(
        "<html><head><style>"
        ":is(h1, .title) { font-weight: bold }"
        ":where(p, .lead) { color: #111111 }"
        "p { color: #222222 }"
        "li:not(.done) { text-decoration: line-through }"
        "li:not(#keep) { font-style: italic }"
        "a:not([href]) { display: none }"
        "a:hover { display: none }"
        "p::first-line { display: none }"
        "nav, p:focus, h3 { text-align: center }"
        "</style></head><body>"
        "<h1 id=h1>a</h1><div id=t class=title>b</div><p id=p>c</p>"
        "<ul><li id=done class=done>x</li><li id=keep>y</li></ul>"
        "<a id=nohref>z</a><a id=link href=/x>w</a><nav id=nav>n</nav><h3 id=h3>m</h3>"
        "</body></html>"));
    TEST_ASSERT_EQUAL(2, by_id("h1")->weight);
    TEST_ASSERT_EQUAL(2, by_id("t")->weight);
    /* :where() has no specificity: the later p wins and also the earlier
     * one would lose on order; p beats :where(p) */
    TEST_ASSERT_EQUAL(0x222222, by_id("p")->color);
    TEST_ASSERT(by_id("done")->deco != 3);
    TEST_ASSERT_EQUAL(3, by_id("keep")->deco);
    TEST_ASSERT_EQUAL(2, by_id("done")->italic);
    TEST_ASSERT(by_id("keep")->italic != 2);
    TEST_ASSERT_EQUAL(WF_SITE_DISP_NONE, by_id("nohref")->disp);
    TEST_ASSERT(by_id("link")->disp != WF_SITE_DISP_NONE);   /* :hover never matches */
    TEST_ASSERT(by_id("p")->disp != WF_SITE_DISP_NONE);      /* ::first-line is not the p */
    TEST_ASSERT_EQUAL(2, by_id("nav")->align);               /* the list keeps nav and h3 */
    TEST_ASSERT_EQUAL(2, by_id("h3")->align);
    TEST_ASSERT(g_d.errors >= 3);
}

static void test_is_specificity(void) {
    TEST_ASSERT_EQUAL(0, prep(
        "<html><head><style>"
        ":is(.a, p) { color: #000001 }"
        "p.b { color: #000002 }"
        "#x :where(p) { text-align: right }"
        "p { text-align: center }"
        "</style></head><body><div id=x><p id=q class=\"a b\">t</p></div></body></html>"));
    /* :is(.a, p) counts as one class; p.b is a class and a type: p.b wins */
    TEST_ASSERT_EQUAL(0x000002, by_id("q")->color);
    /* #x :where(p) is 1,0,0 and beats p */
    TEST_ASSERT_EQUAL(3, by_id("q")->align);
}

static void test_custom_properties(void) {
    TEST_ASSERT_EQUAL(0, prep(
        "<html><head><style>"
        ":root { --brand: #123456; --Weight: bold; --gap: 4px }"
        "body { --accent: #abcdef }"
        "h1 { color: var(--brand) }"
        "h2 { color: var(--missing, #654321) }"
        "h3 { color: var(--missing) }"
        "h3 { font-weight: var(--weight) }"
        "p { color: var(--accent); padding: var(--gap) }"
        "</style></head><body><h1 id=a>a</h1><h2 id=b>b</h2><h3 id=c>c</h3><p id=d>d</p></body></html>"));
    TEST_ASSERT_EQUAL(0x123456, by_id("a")->color);
    TEST_ASSERT_EQUAL(0x654321, by_id("b")->color);
    TEST_ASSERT_EQUAL(0, by_id("c")->color_set);       /* unknown, no fallback: ignored */
    TEST_ASSERT_EQUAL(2, by_id("c")->weight);
    TEST_ASSERT_EQUAL(0xabcdef, by_id("d")->color);
    TEST_ASSERT_EQUAL(0x0F, by_id("d")->pad_set);
    TEST_ASSERT_EQUAL(4, by_id("d")->pad[0]);
    TEST_ASSERT_EQUAL_STRING("#123456", wf_site_var(&g_d, "--brand"));
    TEST_ASSERT_NULL(wf_site_var(&g_d, "--none"));
}

static void test_inline_all_declarations(void) {
    char attr[160];
    TEST_ASSERT_EQUAL(0, prep(
        "<html><body><p id=p style=\"color:#00ff00; font-weight:bold; text-align:center; font-style:italic\">x</p>"
        "<span id=s style=\"text-decoration: underline; visibility: hidden\">y</span></body></html>"));
    TEST_ASSERT_EQUAL(0x00ff00, by_id("p")->color);
    TEST_ASSERT_EQUAL(2, by_id("p")->weight);
    TEST_ASSERT_EQUAL(2, by_id("p")->align);
    TEST_ASSERT_EQUAL(2, by_id("p")->italic);
    TEST_ASSERT(wf_site_style_attr(&g_d, by_id("p")->start, attr, (int)sizeof(attr)) > 0);
    TEST_ASSERT_NOT_NULL(strstr(attr, "font-style:italic"));
    TEST_ASSERT_NOT_NULL(strstr(attr, "color:#00ff00"));
    TEST_ASSERT(wf_site_style_attr(&g_d, by_id("s")->start, attr, (int)sizeof(attr)) > 0);
    TEST_ASSERT_NOT_NULL(strstr(attr, "text-decoration:underline"));
    TEST_ASSERT_EQUAL(1, wf_site_hidden_at(&g_d, by_id("s")->start));
}

static void test_visibility_inherits(void) {
    TEST_ASSERT_EQUAL(0, prep(
        "<html><head><style>.v { visibility: hidden } .v .back { visibility: visible }</style></head><body>"
        "<div id=box class=v><p id=in>x</p><p id=back class=back>y</p></div></body></html>"));
    TEST_ASSERT_EQUAL(1, wf_site_hidden_at(&g_d, by_id("box")->start));
    TEST_ASSERT_EQUAL(1, wf_site_hidden_at(&g_d, by_id("in")->start));
    TEST_ASSERT_EQUAL(0, wf_site_hidden_at(&g_d, by_id("back")->start));
}

static void test_media_lists(void) {
    TEST_ASSERT_EQUAL(1, wf_site_media_ok("screen", 6, 800, 600));
    TEST_ASSERT_EQUAL(0, wf_site_media_ok("print", 5, 800, 600));
    TEST_ASSERT_EQUAL(1, wf_site_media_ok("print, screen", 13, 800, 600));
    TEST_ASSERT_EQUAL(1, wf_site_media_ok("all", 3, 800, 600));
    TEST_ASSERT_EQUAL(0, wf_site_media_ok("not screen", 10, 800, 600));
    TEST_ASSERT_EQUAL(1, wf_site_media_ok("not print", 9, 800, 600));
    TEST_ASSERT_EQUAL(1, wf_site_media_ok("only screen and (min-width: 40em)", 33, 800, 600));
    TEST_ASSERT_EQUAL(0, wf_site_media_ok("only screen and (min-width: 60em)", 33, 800, 600));
    TEST_ASSERT_EQUAL(1, wf_site_media_ok("(max-width:800px)", 17, 800, 600));
    TEST_ASSERT_EQUAL(0, wf_site_media_ok("(width: 801px)", 14, 800, 600));
    TEST_ASSERT_EQUAL(1, wf_site_media_ok("(min-height: 500px)", 19, 800, 600));
    TEST_ASSERT_EQUAL(0, wf_site_media_ok("(max-height: 500px)", 19, 800, 600));
    TEST_ASSERT_EQUAL(1, wf_site_media_ok("(orientation: landscape)", 24, 800, 600));
    TEST_ASSERT_EQUAL(0, wf_site_media_ok("(orientation: portrait)", 23, 800, 600));
    TEST_ASSERT_EQUAL(1, wf_site_media_ok("(orientation: portrait)", 23, 400, 600));
    TEST_ASSERT_EQUAL(1, wf_site_media_ok("(prefers-color-scheme: light)", 29, 800, 600));
    TEST_ASSERT_EQUAL(0, wf_site_media_ok("(prefers-color-scheme: dark)", 28, 800, 600));
    TEST_ASSERT_EQUAL(0, wf_site_media_ok("(monochrome-ish: 3)", 19, 800, 600));
    TEST_ASSERT_EQUAL(1, wf_site_media_ok("(hover: hover) and (pointer: fine)", 34, 800, 600));
    TEST_ASSERT_EQUAL(1, wf_site_media_ok("", 0, 800, 600));
}

static void test_media_in_sheet(void) {
    static wf_site_t d;
    const char *h =
        "<html><head><style>"
        "@media print { .nav { display: none } }"
        "@media screen and (max-width: 500px) { .side { display: none } }"
        "@media (prefers-color-scheme: dark) { p { color: #ffffff } }"
        "</style></head><body><div id=nav class=nav>n</div><div id=side class=side>s</div><p id=p>t</p></body></html>";
    int i, nav = -1, side = -1;
    TEST_ASSERT_EQUAL(0, wf_site_prepare_ext(&d, h, (int)strlen(h), 400, 700, 0, 0));
    for (i = 0; i < d.nnodes; i++) {
        if (strcmp(d.node[i].id, "nav") == 0) nav = i;
        if (strcmp(d.node[i].id, "side") == 0) side = i;
    }
    TEST_ASSERT(nav >= 0 && side >= 0);
    TEST_ASSERT(d.node[nav].disp != WF_SITE_DISP_NONE);   /* print only (Lot 86 applied it) */
    TEST_ASSERT_EQUAL(WF_SITE_DISP_NONE, d.node[side].disp);
    TEST_ASSERT_EQUAL(0, wf_site_prepare_ext(&d, h, (int)strlen(h), 900, 700, 0, 0));
    TEST_ASSERT(d.node[side].disp != WF_SITE_DISP_NONE);
}

static void test_supports(void) {
    const char *c;
    c = "(display: grid)";
    TEST_ASSERT_EQUAL(1, wf_site_supports(c, (int)strlen(c)));
    c = "(display: ruby)";
    TEST_ASSERT_EQUAL(0, wf_site_supports(c, (int)strlen(c)));
    c = "not (display: ruby)";
    TEST_ASSERT_EQUAL(1, wf_site_supports(c, (int)strlen(c)));
    c = "(display: flex) and (gap: 1rem)";
    TEST_ASSERT_EQUAL(1, wf_site_supports(c, (int)strlen(c)));
    c = "(display: flex) and (backdrop-filter: blur(2px))";
    TEST_ASSERT_EQUAL(0, wf_site_supports(c, (int)strlen(c)));
    c = "(backdrop-filter: blur(2px)) or (color: red)";
    TEST_ASSERT_EQUAL(1, wf_site_supports(c, (int)strlen(c)));
    c = "selector(:has(a))";
    TEST_ASSERT_EQUAL(0, wf_site_supports(c, (int)strlen(c)));
    c = "((display: grid) and (color: red))";
    TEST_ASSERT_EQUAL(1, wf_site_supports(c, (int)strlen(c)));
    TEST_ASSERT_EQUAL(0, prep(
        "<html><head><style>"
        "@supports (display: grid) { #a { font-weight: bold } }"
        "@supports (display: ruby) { #a { display: none } }"
        "@supports not (display: ruby) { #b { color: #0a0b0c } }"
        "</style></head><body><p id=a>a</p><p id=b>b</p></body></html>"));
    TEST_ASSERT_EQUAL(2, by_id("a")->weight);
    TEST_ASSERT(by_id("a")->disp != WF_SITE_DISP_NONE);
    TEST_ASSERT_EQUAL(0x0a0b0c, by_id("b")->color);
}

static void test_links_and_imports(void) {
    const wf_site_link_t *l;
    TEST_ASSERT_EQUAL(0, prep(
        "<html><head>"
        "<link rel=\"stylesheet\" href=\"/css/main.css\">"
        "<link rel=\"preload stylesheet\" href=\"site.css\" media=\"screen\">"
        "<link rel=\"alternate stylesheet\" href=\"alt.css\">"
        "<link rel=\"icon\" href=\"/favicon.ico\">"
        "<link rel=stylesheet href=/css/main.css>"
        "<style>@import url(\"print.css\") print; @import 'more.css'; @import url(x.css);"
        "p { color: red }</style>"
        "</head><body><p>x</p></body></html>"));
    TEST_ASSERT_EQUAL(5, wf_site_nlinks(&g_d));
    l = wf_site_link_at(&g_d, 0);
    TEST_ASSERT_EQUAL_STRING("/css/main.css", l->href);
    TEST_ASSERT_EQUAL(0, l->from_import);
    l = wf_site_link_at(&g_d, 1);
    TEST_ASSERT_EQUAL_STRING("site.css", l->href);
    TEST_ASSERT_EQUAL_STRING("screen", l->media);
    l = wf_site_link_at(&g_d, 2);
    TEST_ASSERT_EQUAL_STRING("print.css", l->href);
    TEST_ASSERT_EQUAL_STRING("print", l->media);
    TEST_ASSERT_EQUAL(1, l->from_import);
    TEST_ASSERT_EQUAL_STRING("more.css", wf_site_link_at(&g_d, 3)->href);
    TEST_ASSERT_EQUAL_STRING("x.css", wf_site_link_at(&g_d, 4)->href);
    TEST_ASSERT_NULL(wf_site_link_at(&g_d, 5));
}

static void test_external_sheet_order(void) {
    static wf_site_t d;
    const char *ext = "p { color: #000001; font-weight: bold } .x { display: none }";
    const char *h = "<html><head><style>p { color: #000002 }</style></head><body><p id=p>a</p><div id=x class=x>b</div></body></html>";
    int i, p = -1, x = -1;
    TEST_ASSERT_EQUAL(0, wf_site_prepare_ext(&d, h, (int)strlen(h), 640, 480, ext, (int)strlen(ext)));
    for (i = 0; i < d.nnodes; i++) {
        if (strcmp(d.node[i].id, "p") == 0) p = i;
        if (strcmp(d.node[i].id, "x") == 0) x = i;
    }
    TEST_ASSERT_EQUAL(0x000002, d.node[p].color);   /* the page <style> comes later */
    TEST_ASSERT_EQUAL(2, d.node[p].weight);               /* the external sheet still applies */
    TEST_ASSERT_EQUAL(WF_SITE_DISP_NONE, d.node[x].disp);
    TEST_ASSERT(d.ext_n > 0);
}

static void test_bigger_tables(void) {
    static char page[24000];
    int n = 0, i;
    n += snprintf(page + n, sizeof(page) - (size_t)n, "<html><head><style>");
    for (i = 0; i < 100; i++) n += snprintf(page + n, sizeof(page) - (size_t)n, ".c%d{font-weight:bold}", i);
    n += snprintf(page + n, sizeof(page) - (size_t)n, "</style></head><body>");
    for (i = 0; i < 180; i++) n += snprintf(page + n, sizeof(page) - (size_t)n, "<p class=c%d>t</p>", i % 120);
    n += snprintf(page + n, sizeof(page) - (size_t)n, "</body></html>");
    TEST_ASSERT_EQUAL(0, wf_site_prepare(&g_d, page, n));
    TEST_ASSERT(g_d.nnodes >= 183);            /* html, head, body and 180 p (Lot 86 kept 40) */
    TEST_ASSERT(g_d.nrules >= 100);
    {
        int bold = 0;
        for (i = 0; i < g_d.nnodes; i++) if (g_d.node[i].weight == 2) bold++;
        TEST_ASSERT(bold >= 150);
    }
}

int main(void) {
    unity_init();
    RUN_TEST(test_attribute_operators);
    RUN_TEST(test_structural);
    RUN_TEST(test_is_where_not);
    RUN_TEST(test_is_specificity);
    RUN_TEST(test_custom_properties);
    RUN_TEST(test_inline_all_declarations);
    RUN_TEST(test_visibility_inherits);
    RUN_TEST(test_media_lists);
    RUN_TEST(test_media_in_sheet);
    RUN_TEST(test_supports);
    RUN_TEST(test_links_and_imports);
    RUN_TEST(test_external_sheet_order);
    RUN_TEST(test_bigger_tables);
    unity_print_results();
    unity_cleanup();
    return unity_stats.tests_failed == 0 ? 0 : 1;
}
