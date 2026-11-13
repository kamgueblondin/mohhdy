/* Lot 88 (US-031): five more properties in the reader stylesheet:
 * background-color (and a background that is one colour), list-style-type
 * (and the keyword of list-style), font-size, text-indent and
 * letter-spacing. Lengths are kept as a checked token and passed to the
 * engine in the style attribute; every CSS-wide keyword works on them, and
 * the all shorthand resets them without writing initial values out. */
#include "../../framework/unity.h"
#include "../../../userspace/wf_sitecss.h"
#include <string.h>
#include <stdio.h>

static wf_site_t g_d;
static char g_o[400];

static const wf_site_node_t *by_id(const char *id) {
    int i;
    for (i = 0; i < g_d.nnodes; i++) if (strcmp(g_d.node[i].id, id) == 0) return &g_d.node[i];
    {
        static wf_site_node_t none;
        return &none;
    }
}

static void prep(const char *css, const char *body) {
    static char html[3072];
    int n = snprintf(html, sizeof(html), "<html><head><style>%s</style></head><body>%s</body></html>", css, body);
    TEST_ASSERT(n > 0 && n < (int)sizeof(html));
    TEST_ASSERT_EQUAL(0, wf_site_prepare(&g_d, html, n));
}

/* the style attribute the reader writes on the kept tag of node id */
static const char *style_of(const char *id) {
    const wf_site_node_t *nd = by_id(id);
    g_o[0] = 0;
    (void)wf_site_style_attr(&g_d, nd->start, g_o, (int)sizeof(g_o));
    return g_o;
}

#define LIST 0
#define SIZE 1
#define INDENT 2
#define LS 3

static void test_background_color(void) {
    prep(".a { background-color: #FF8800 } .b { background: navy } .c { background: url(x.png) red }"
         ".d { background-color: #123456 } .d.e { background-color: transparent }",
         "<p class=a id=a>a</p><p class=b id=b>b</p><p class=c id=c>c</p><p class='d e' id=d>d</p>");
    TEST_ASSERT_EQUAL(1, by_id("a")->bg_set);
    TEST_ASSERT_EQUAL(0xff8800, by_id("a")->bg);
    TEST_ASSERT(strstr(style_of("a"), "background-color:#ff8800") != 0);
    TEST_ASSERT_EQUAL(1, by_id("b")->bg_set);
    TEST_ASSERT_EQUAL(0x000080, by_id("b")->bg);
    /* an image layer and a colour: Lot 88 passed nothing on; Lot 90
     * passes both (the shorthand is split into its longhands) */
    TEST_ASSERT_EQUAL(1, by_id("c")->bg_set);
    TEST_ASSERT_EQUAL(0xff0000, by_id("c")->bg);
    TEST_ASSERT(strstr(style_of("c"), "background-color:#ff0000;background-image:url(x.png)") != 0);
    /* transparent, more specific, wins over the colour */
    TEST_ASSERT_EQUAL(0, by_id("d")->bg_set);
    /* not inherited */
    prep("div { background-color: #00ff00 }", "<div id=p><span id=k>x</span></div>");
    TEST_ASSERT_EQUAL(1, by_id("p")->bg_set);
    TEST_ASSERT_EQUAL(0, by_id("k")->bg_set);
}

static void test_list_style(void) {
    prep("ul.sq { list-style-type: square } ol.r { list-style: upper-roman inside }"
         "ol.bad { list-style: upper-roman sideways } ul.none { list-style: none }"
         "ul.img { list-style: url(dot.png) outside }",
         "<ul class=sq id=sq><li id=li>a</li></ul><ol class=r id=r><li>b</li></ol>"
         "<ol class=bad id=bad><li>c</li></ol><ul class=none id=none><li>d</li></ul><ul class=img id=img><li>e</li></ul>");
    TEST_ASSERT_EQUAL_STRING("square", by_id("sq")->tok[LIST]);
    TEST_ASSERT(strstr(style_of("sq"), "list-style-type:square") != 0);
    TEST_ASSERT_EQUAL_STRING("upper-roman", by_id("r")->tok[LIST]);
    /* a word that is no list-style part makes the value invalid */
    TEST_ASSERT_EQUAL(0, by_id("bad")->tok_val[LIST]);
    TEST_ASSERT_EQUAL_STRING("none", by_id("none")->tok[LIST]);
    /* an image alone gives no type: Lot 88 dropped the declaration;
     * Lot 90 keeps the image and the position, the type is initial (1,
     * not written out) */
    TEST_ASSERT_EQUAL(1, by_id("img")->tok_val[LIST]);
    TEST_ASSERT(strstr(style_of("img"), "list-style-image:url(dot.png);list-style-position:outside") != 0);
    TEST_ASSERT(strstr(g_o, "list-style-type") == 0);
    /* the li itself carries nothing: the engine inherits from the ul */
    TEST_ASSERT_EQUAL(0, by_id("li")->tok_val[LIST]);
    prep("ul { list-style-type: lower-alpha } li.x { list-style-type: inherit }",
         "<ul id=u><li class=x id=x>a</li></ul>");
    TEST_ASSERT_EQUAL_STRING("lower-alpha", by_id("x")->tok[LIST]);
    TEST_ASSERT_EQUAL(2, by_id("x")->tok_val[LIST]);
}

static void test_font_size(void) {
    prep("#a { font-size: 20px } #b { font-size: 1.5em } #c { font-size: 120% } #d { font-size: x-large }"
         "#e { font-size: 200px } #f { font-size: 1.125em } #g { font-size: -2px } #h { font-size: smaller }"
         "#i { font-size: 0 } #j { font-size: 12pt } #k { font-size: 7em }",
         "<p id=a>a</p><p id=b>b</p><p id=c>c</p><p id=d>d</p><p id=e>e</p><p id=f>f</p><p id=g>g</p>"
         "<p id=h>h</p><p id=i>i</p><p id=j>j</p><p id=k>k</p>");
    TEST_ASSERT_EQUAL_STRING("20px", by_id("a")->tok[SIZE]);
    TEST_ASSERT_EQUAL_STRING("1.5em", by_id("b")->tok[SIZE]);
    TEST_ASSERT_EQUAL_STRING("120%", by_id("c")->tok[SIZE]);
    TEST_ASSERT_EQUAL_STRING("x-large", by_id("d")->tok[SIZE]);
    TEST_ASSERT(strstr(style_of("b"), "font-size:1.5em") != 0);
    /* out of range, three decimals, negative, a bare 0, too large em */
    TEST_ASSERT_EQUAL(0, by_id("e")->tok_val[SIZE]);
    TEST_ASSERT_EQUAL(0, by_id("f")->tok_val[SIZE]);
    TEST_ASSERT_EQUAL(0, by_id("g")->tok_val[SIZE]);
    TEST_ASSERT_EQUAL(0, by_id("i")->tok_val[SIZE]);
    TEST_ASSERT_EQUAL(0, by_id("k")->tok_val[SIZE]);
    TEST_ASSERT_EQUAL_STRING("smaller", by_id("h")->tok[SIZE]);
    TEST_ASSERT_EQUAL_STRING("12pt", by_id("j")->tok[SIZE]);
}

static void test_indent_and_letter_spacing(void) {
    prep("#a { text-indent: 2em; letter-spacing: 2px } #b { text-indent: -5px; letter-spacing: 20px }"
         "#c { text-indent: 10%; letter-spacing: normal } #d { text-indent: 0; letter-spacing: -0.1em }",
         "<p id=a>a</p><p id=b>b</p><p id=c>c</p><p id=d>d</p>");
    TEST_ASSERT_EQUAL_STRING("2em", by_id("a")->tok[INDENT]);
    TEST_ASSERT_EQUAL_STRING("2px", by_id("a")->tok[LS]);
    TEST_ASSERT(strstr(style_of("a"), "text-indent:2em;letter-spacing:2px") != 0);
    /* the engine's bounds: no negative indent, no % indent, 16 px spacing */
    TEST_ASSERT_EQUAL(0, by_id("b")->tok_val[INDENT]);
    TEST_ASSERT_EQUAL(0, by_id("b")->tok_val[LS]);
    TEST_ASSERT_EQUAL(0, by_id("c")->tok_val[INDENT]);
    TEST_ASSERT_EQUAL_STRING("normal", by_id("c")->tok[LS]);
    TEST_ASSERT_EQUAL_STRING("0", by_id("d")->tok[INDENT]);
    TEST_ASSERT_EQUAL_STRING("-0.1em", by_id("d")->tok[LS]);
}

static void test_cascade(void) {
    /* specificity, then !important, then the style attribute */
    prep("p { font-size: 18px !important } #a { font-size: 30px } .x { letter-spacing: 1px } p.x { letter-spacing: 3px }",
         "<p id=a class=x>a</p><p id=b style='font-size: 40px; text-indent: 4px'>b</p>");
    TEST_ASSERT_EQUAL_STRING("18px", by_id("a")->tok[SIZE]);
    TEST_ASSERT_EQUAL_STRING("3px", by_id("a")->tok[LS]);
    /* the important author rule beats the normal inline one */
    TEST_ASSERT_EQUAL_STRING("18px", by_id("b")->tok[SIZE]);
    TEST_ASSERT_EQUAL_STRING("4px", by_id("b")->tok[INDENT]);
}

static void test_keywords(void) {
    /* initial resets and writes nothing out; unset on an inherited
     * property inherits; revert-layer goes back to the lower layer */
    prep("@layer base, top; @layer base { #r { font-size: 22px } } @layer top { #r { font-size: revert-layer } }"
         "#i { font-size: 30px } #i.k { font-size: initial } div { text-indent: 12px } #u { text-indent: unset }"
         "#t { background-color: red } #t.k { background-color: inherit }",
         "<p id=r>r</p><p id=i class=k>i</p><div id=dv><p id=u>u</p></div><div style='background-color: blue'><p id=t class=k>t</p></div>");
    TEST_ASSERT_EQUAL_STRING("22px", by_id("r")->tok[SIZE]);
    TEST_ASSERT_EQUAL_STRING("medium", by_id("i")->tok[SIZE]);
    TEST_ASSERT_EQUAL(1, by_id("i")->tok_val[SIZE]);
    TEST_ASSERT(strstr(style_of("i"), "font-size") == 0);
    TEST_ASSERT_EQUAL_STRING("12px", by_id("u")->tok[INDENT]);
    TEST_ASSERT_EQUAL(1, by_id("t")->bg_set);
    TEST_ASSERT_EQUAL(0x0000ff, by_id("t")->bg);
}

static void test_all_shorthand(void) {
    /* all: initial clears the five as well, without writing them out */
    prep("p { background-color: #ff0000; list-style-type: square; font-size: 2em; text-indent: 3px; letter-spacing: 1px }"
         "p.reset { all: initial }",
         "<p id=k>k</p><p id=z class=reset>z</p>");
    TEST_ASSERT_EQUAL(1, by_id("k")->bg_set);
    TEST_ASSERT_EQUAL(0, by_id("z")->bg_set);
    TEST_ASSERT_EQUAL(1, by_id("z")->tok_val[LIST]);
    TEST_ASSERT_EQUAL(1, by_id("z")->tok_val[SIZE]);
    /* (color: initial is black and is written, as in lot 87) */
    TEST_ASSERT(strstr(style_of("z"), "background") == 0);
    TEST_ASSERT(strstr(g_o, "list-style") == 0 && strstr(g_o, "font-size") == 0);
    TEST_ASSERT(strstr(g_o, "text-indent") == 0 && strstr(g_o, "letter-spacing") == 0);
    TEST_ASSERT(strstr(style_of("k"), "background-color:#ff0000;list-style-type:square;font-size:2em;"
                                      "text-indent:3px;letter-spacing:1px") != 0);
}

static void test_report_and_bounds(void) {
    char out[1200];
    prep("#a { font-size: 1.25em; background: #ABCDEF; list-style: decimal }",
         "<ol id=a><li>x</li></ol>");
    TEST_ASSERT(wf_site_report(&g_d, out, (int)sizeof(out)) > 0);
    TEST_ASSERT(strstr(out, "node ol #a display auto bg #abcdef list decimal size 1.25em") != 0);
    /* a long value cannot overrun the token */
    prep("#a { font-size: 123456789012345px; list-style: lower-alpha lower-alpha lower-alpha lower-alpha }",
         "<ol id=a><li>x</li></ol>");
    TEST_ASSERT_EQUAL(0, by_id("a")->tok_val[SIZE]);
    TEST_ASSERT(strlen(by_id("a")->tok[LIST]) < WF_SITE_TOK);
    /* every property at once fits the reader's attribute buffer */
    prep("#m { color: red; font-weight: bold; text-align: center; margin: 1px 2px 3px 4px; padding: 5px 6px 7px 8px;"
         " font-style: italic; text-decoration: underline; text-transform: uppercase; white-space: pre-wrap;"
         " background-color: #010203; list-style-type: upper-roman; font-size: 1.25em; text-indent: 10px;"
         " letter-spacing: -0.25em }",
         "<ul id=m><li>x</li></ul>");
    TEST_ASSERT(wf_site_style_attr(&g_d, by_id("m")->start, g_o, 320) < 320);
    TEST_ASSERT(strstr(g_o, "letter-spacing:-0.25em\"") != 0);
}

int main(void) {
    unity_init();
    RUN_TEST(test_background_color);
    RUN_TEST(test_list_style);
    RUN_TEST(test_font_size);
    RUN_TEST(test_indent_and_letter_spacing);
    RUN_TEST(test_cascade);
    RUN_TEST(test_keywords);
    RUN_TEST(test_all_shorthand);
    RUN_TEST(test_report_and_bounds);
    unity_print_results();
    unity_cleanup();
    return unity_stats.tests_failed == 0 ? 0 : 1;
}
