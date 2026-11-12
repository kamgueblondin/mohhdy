/* Lot 90 (US-031): the reader sheet passes background images, gradients,
 * positions, sizes and repeats, list marker images and positions; values
 * up to 127 bytes; font sizes resolved in px when they mix em with px or
 * use rem. Hand-written cases next to the generated suites
 * (test_lot90_bgval.c, test_lot90_bgsheet.c, test_lot90_fontpx.c). */
#include "../../framework/unity.h"
#include <string.h>
#include <stdio.h>
#include "wf_sitecss.h"
#include "wf_cssbg.h"

static wf_site_t g_d;
static char g_o[1400];
static char g_html[8192];

static void prep(const char *css, const char *body) {
    snprintf(g_html, sizeof(g_html), "<html><head><style>%s</style></head><body>%s</body></html>", css, body);
    TEST_ASSERT_EQUAL(0, wf_site_prepare(&g_d, g_html, (int)strlen(g_html)));
}

static const wf_site_node_t *by_id(const char *id) {
    int i;
    for (i = 0; i < g_d.nnodes; i++) if (!strcmp(g_d.node[i].id, id)) return &g_d.node[i];
    return &g_d.node[0];
}

static int idx_of(const char *id) {
    int i;
    for (i = 0; i < g_d.nnodes; i++) if (!strcmp(g_d.node[i].id, id)) return i;
    return -1;
}

static const char *style_of(const char *id) {
    g_o[0] = 0;
    wf_site_style_attr(&g_d, by_id(id)->start, g_o, (int)sizeof(g_o));
    return g_o;
}

#define L(k) (WF_SITE_K_##k - WF_SITE_K_BGIMG)

static void test_url_keeps_case(void) {
    prep(".a { background-image: URL(\"Img/Hero.PNG\") } .b { list-style-image: url( 'Dots/Red.GIF' ) }",
         "<p id=a class=a>a</p><ul id=b class=b><li>x</li></ul>");
    TEST_ASSERT_EQUAL_STRING(" style=\"background-image:url(Img/Hero.PNG)\"", style_of("a"));
    TEST_ASSERT_EQUAL_STRING(" style=\"list-style-image:url(Dots/Red.GIF)\"", style_of("b"));
    /* the rest of a value is still lower case */
    prep(".a { background: URL(A.png) NO-REPEAT CENTER }", "<p id=a class=a>a</p>");
    TEST_ASSERT_EQUAL_STRING(" style=\"background-image:url(A.png);background-position:center center;"
                             "background-repeat:no-repeat\"", style_of("a"));
}

static void test_shorthand_resets_longhands(void) {
    /* a later shorthand without an image puts the image back to none
     * (initial: not written); a later longhand changes one part only */
    prep("p { background-image: url(a.png); background-repeat: no-repeat } p { background: #00ff00 }"
         "#k { background-position: right bottom }",
         "<p id=t>t</p><p id=k>k</p>");
    TEST_ASSERT_EQUAL_STRING(" style=\"background-color:#00ff00\"", style_of("t"));
    TEST_ASSERT_EQUAL(1, by_id("t")->lv_val[L(BGIMG)]);
    TEST_ASSERT_EQUAL(1, by_id("t")->lv_val[L(BGREP)]);
    TEST_ASSERT_EQUAL_STRING(" style=\"background-color:#00ff00;background-position:right bottom\"", style_of("k"));
}

static void test_keywords_on_shorthands(void) {
    /* inherit on the shorthand: every background longhand from the
     * parent (CSS-wide keywords reach the longhands) */
    prep("div { background: url(p.png) repeat-x 10px 20px } div p { background: inherit }"
         "ul { list-style: square inside url(m.png) } li.u { list-style: unset } li.i { list-style: initial }",
         "<div id=d><p id=p>p</p></div><ul id=u><li id=lu class=u>a</li><li id=li class=i>b</li></ul>");
    TEST_ASSERT_EQUAL_STRING(" style=\"background-image:url(p.png);background-position:10px 20px;"
                             "background-repeat:repeat-x\"", style_of("p"));
    /* unset inherits the list kinds (they inherit) */
    TEST_ASSERT_EQUAL_STRING(" style=\"list-style-type:square;list-style-image:url(m.png);list-style-position:inside\"",
                             style_of("lu"));
    /* initial: nothing written */
    TEST_ASSERT_EQUAL_STRING("", style_of("li"));
    TEST_ASSERT_EQUAL(1, by_id("li")->lv_val[L(LIMG)]);
    /* unset on background: initial (the background kinds do not inherit) */
    prep("div { background: url(p.png) red } div p { background: unset }", "<div id=d><p id=p>p</p></div>");
    TEST_ASSERT_EQUAL_STRING("", style_of("p"));
}

static void test_revert_layer_on_shorthand(void) {
    prep("@layer a, b; @layer a { p { background: url(a.png) no-repeat } } @layer b { p { background: revert-layer } }",
         "<p id=p>p</p>");
    TEST_ASSERT_EQUAL_STRING(" style=\"background-image:url(a.png);background-repeat:no-repeat\"", style_of("p"));
}

static void test_all_initial_clears_new_kinds(void) {
    prep("p { background: url(a.png) center / cover; list-style-image: url(m.png); list-style-position: inside }"
         "p.r { all: initial }",
         "<p id=k>k</p><p id=r class=r>r</p>");
    TEST_ASSERT(strstr(style_of("k"), "background-size:cover") != 0);
    TEST_ASSERT(strstr(g_o, "list-style-position:inside") != 0);
    TEST_ASSERT(strstr(style_of("r"), "background-image") == 0);
    TEST_ASSERT(strstr(g_o, "list-style-image") == 0);
}

static void test_list_style_shorthand(void) {
    prep("ul.a { list-style: none } ul.b { list-style: inside } ul.c { list-style: none url(x.png) }"
         "ul.d { list-style: square none url(x.png) } ul.e { list-style: none none } ul.f { list-style: url(x.png) url(y.png) }"
         "ul.g { list-style: inside outside }",
         "<ul id=a class=a></ul><ul id=b class=b></ul><ul id=c class=c></ul><ul id=d class=d></ul>"
         "<ul id=e class=e></ul><ul id=f class=f></ul><ul id=g class=g></ul>");
    TEST_ASSERT_EQUAL_STRING(" style=\"list-style-type:none\"", style_of("a"));
    /* Lot 88 dropped a list-style without a type keyword */
    TEST_ASSERT_EQUAL_STRING(" style=\"list-style-position:inside\"", style_of("b"));
    /* none with an image: the type is none */
    TEST_ASSERT_EQUAL_STRING(" style=\"list-style-type:none;list-style-image:url(x.png)\"", style_of("c"));
    /* none with a type and an image has nothing left to set: invalid */
    TEST_ASSERT_EQUAL_STRING("", style_of("d"));
    TEST_ASSERT_EQUAL_STRING(" style=\"list-style-type:none\"", style_of("e"));
    TEST_ASSERT_EQUAL_STRING("", style_of("f"));
    TEST_ASSERT_EQUAL_STRING("", style_of("g"));
}

static void test_long_values(void) {
    /* 47 bytes was the Lot 89 limit; a 100-byte clamp() is kept now */
    char css[600];
    snprintf(css, sizeof(css), "#a { font-size: 12px } #a { font-size: clamp(1rem, calc(0.5rem + 1vw + 1px + 1px + 1px + 1px), 2rem) }"
                               "#b { font-size: 14px } #b { background-image: url(%s) }", "x");
    prep(css, "<p id=a>a</p><p id=b>b</p>");
    TEST_ASSERT(strlen("clamp(1rem, calc(0.5rem + 1vw + 1px + 1px + 1px + 1px), 2rem)") > 47);
    TEST_ASSERT_EQUAL_STRING(" style=\"font-size:18.4px\"", style_of("a"));
    TEST_ASSERT_EQUAL(0, g_d.long_values);
    /* a value of 128 bytes or more is dropped, never cut: the earlier
     * declaration stays and the drop is counted */
    {
        char longurl[200];
        int k;
        for (k = 0; k < 130; k++) longurl[k] = (char)('a' + k % 26);
        longurl[130] = 0;
        snprintf(css, sizeof(css), "#a { background-image: url(ok.png) } #a { background-image: url(%s) }", longurl);
        prep(css, "<p id=a>a</p>");
        TEST_ASSERT_EQUAL_STRING(" style=\"background-image:url(ok.png)\"", style_of("a"));
        TEST_ASSERT_EQUAL(1, g_d.long_values);
    }
    /* exactly 127 bytes of value fits */
    {
        char v[140];
        int k;
        strcpy(v, "url(");
        for (k = 4; k < 126; k++) v[k] = 'b';
        v[126] = ')';
        v[127] = 0;
        TEST_ASSERT_EQUAL(127, (int)strlen(v));
        snprintf(css, sizeof(css), "#a { list-style-image: %s }", v);
        prep(css, "<ul id=a></ul>");
        /* the url itself is longer than 100 bytes: refused by the value
         * check, not by the length bound */
        TEST_ASSERT_EQUAL_STRING("", style_of("a"));
        TEST_ASSERT_EQUAL(0, g_d.long_values);
    }
}

static void test_pool_bound(void) {
    /* 32 distinct long values fit the pool; the 33rd is left out and
     * counted (its declaration does not apply) */
    char css[4096];
    char body[2048];
    int k, n = 0, b = 0;
    for (k = 0; k < 33; k++) {
        n += snprintf(css + n, sizeof(css) - n, "#e%d { background-image: url(i%d.png) }", k, k);
        b += snprintf(body + b, sizeof(body) - b, "<p id=e%d>x</p>", k);
    }
    prep(css, body);
    TEST_ASSERT_EQUAL(32, g_d.nlongs);
    TEST_ASSERT_EQUAL(1, g_d.dropped_longs);
    TEST_ASSERT_EQUAL_STRING(" style=\"background-image:url(i31.png)\"", style_of("e31"));
    TEST_ASSERT_EQUAL_STRING("", style_of("e32"));
    /* the same value twice takes one entry */
    prep("p { background-image: url(s.png) } div { background-image: url(s.png) }", "<p id=a>a</p><div id=b>b</div>");
    TEST_ASSERT_EQUAL(1, g_d.nlongs);
}

static void test_report_lines(void) {
    char out[2000];
    prep("#a { background: url(a.png) no-repeat left / contain; list-style: url(m.png) inside }", "<ul id=a><li>x</li></ul>");
    TEST_ASSERT(wf_site_report(&g_d, out, (int)sizeof(out)) > 0);
    TEST_ASSERT_NOT_NULL(strstr(out, "node ul #a display auto bgimg url(a.png) bgpos left center bgsize contain bgrep no-repeat"
                                     " limg url(m.png) lpos inside"));
}

static void test_var_in_shorthand_per_element(void) {
    /* a background shorthand with var() of a per-element property is read
     * again for each element, each longhand of it included */
    prep(".a { --img: url(a.png) } .b { --img: url(b.png) } p { background: var(--img) no-repeat }",
         "<div class=a><p id=x>x</p></div><div class=b><p id=y>y</p></div>");
    TEST_ASSERT_EQUAL_STRING(" style=\"background-image:url(a.png);background-repeat:no-repeat\"", style_of("x"));
    TEST_ASSERT_EQUAL_STRING(" style=\"background-image:url(b.png);background-repeat:no-repeat\"", style_of("y"));
}

static void test_style_attribute(void) {
    prep("", "<p id=a style=\"background: linear-gradient(to right, red, blue) ; list-style-position: inside\">a</p>");
    TEST_ASSERT_EQUAL_STRING(" style=\"background-image:linear-gradient(to right, red, blue);list-style-position:inside\"",
                             style_of("a"));
}

static void test_unknown_colour_drops_shorthand(void) {
    /* the reader keeps eight colour names; a shorthand whose colour it
     * does not keep is dropped whole, as a lone colour was in Lot 88 */
    prep("#a { background: url(a.png) orange } #b { background: url(b.png) #abc }", "<p id=a>a</p><p id=b>b</p>");
    TEST_ASSERT_EQUAL_STRING("", style_of("a"));
    TEST_ASSERT_EQUAL_STRING(" style=\"background-color:#aabbcc;background-image:url(b.png)\"", style_of("b"));
}

static void test_font_root_and_rem(void) {
    /* rem follows the root font size; with a 16 px root a plain rem token
     * is passed on as written */
    prep("html { font-size: 20px } #a { font-size: 1.5rem } #b { text-indent: calc(1rem + 2px) }",
         "<p id=a>a</p><p id=b>b</p>");
    TEST_ASSERT_EQUAL(20000, g_d.root_fs);
    TEST_ASSERT_EQUAL_STRING(" style=\"font-size:30px\"", style_of("a"));
    TEST_ASSERT_EQUAL_STRING(" style=\"text-indent:22px\"", style_of("b"));
    prep("#a { font-size: 1.5rem }", "<p id=a>a</p>");
    TEST_ASSERT_EQUAL_STRING(" style=\"font-size:1.5rem\"", style_of("a"));
    TEST_ASSERT_EQUAL(16000, g_d.root_fs);
    TEST_ASSERT_EQUAL(24000, g_d.fs[idx_of("a")]);
}

static void test_font_mixed_against_parent(void) {
    prep("#p { font-size: 10px } #c { font-size: calc(2em + 3px) } #g { font-size: calc(50% - 1px) }"
         "#l { letter-spacing: calc(0.5em - 2px) }",
         "<div id=p><div id=c><span id=g>g</span><span id=l>l</span></div></div>");
    TEST_ASSERT_EQUAL_STRING(" style=\"font-size:23px\"", style_of("c"));
    TEST_ASSERT_EQUAL_STRING(" style=\"font-size:10.5px\"", style_of("g"));
    /* letter-spacing: em of the element itself (it inherits 23 px) */
    TEST_ASSERT_EQUAL_STRING(" style=\"letter-spacing:9.5px\"", style_of("l"));
    TEST_ASSERT_EQUAL(3, g_d.len_resolved);
    TEST_ASSERT_EQUAL(0, g_d.len_dropped);
}

static void test_font_resolved_out_of_bounds(void) {
    /* resolved but outside the engine's bounds: not written, counted; the
     * element keeps its parent's size */
    prep("#p { font-size: 60px } #c { font-size: calc(2em + 1px) } #l { letter-spacing: calc(1em - 1px) }",
         "<div id=p><p id=c>c</p><p id=l>l</p></div>");
    TEST_ASSERT_EQUAL_STRING("", style_of("c"));
    TEST_ASSERT_EQUAL(60000, g_d.fs[idx_of("c")]);
    TEST_ASSERT_EQUAL_STRING("", style_of("l"));
    TEST_ASSERT_EQUAL(2, g_d.len_dropped);
}

static void test_font_keywords(void) {
    prep("#p { font-size: 20px } #s { font-size: smaller } #l { font-size: larger } #x { font-size: x-large }"
         "#m { font-size: calc(1em + 1px) }",
         "<div id=p><p id=s>s<span id=m>m</span></p><p id=l>l</p><p id=x>x</p></div>");
    TEST_ASSERT_EQUAL(16666, g_d.fs[idx_of("s")]);
    TEST_ASSERT_EQUAL(24000, g_d.fs[idx_of("l")]);
    TEST_ASSERT_EQUAL(24000, g_d.fs[idx_of("x")]);
    TEST_ASSERT_EQUAL_STRING(" style=\"font-size:17.67px\"", style_of("m"));
}

int main(void) {
    unity_init();
    RUN_TEST(test_url_keeps_case);
    RUN_TEST(test_shorthand_resets_longhands);
    RUN_TEST(test_keywords_on_shorthands);
    RUN_TEST(test_revert_layer_on_shorthand);
    RUN_TEST(test_all_initial_clears_new_kinds);
    RUN_TEST(test_list_style_shorthand);
    RUN_TEST(test_long_values);
    RUN_TEST(test_pool_bound);
    RUN_TEST(test_report_lines);
    RUN_TEST(test_var_in_shorthand_per_element);
    RUN_TEST(test_style_attribute);
    RUN_TEST(test_unknown_colour_drops_shorthand);
    RUN_TEST(test_font_root_and_rem);
    RUN_TEST(test_font_mixed_against_parent);
    RUN_TEST(test_font_resolved_out_of_bounds);
    RUN_TEST(test_font_keywords);
    unity_print_results();
    unity_cleanup();
    return unity_stats.tests_failed == 0 ? 0 : 1;
}
