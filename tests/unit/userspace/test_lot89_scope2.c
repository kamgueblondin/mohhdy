/* Lot 89: @scope fixes found after chunk 1.
 * 1. "&:first-child" and ":scope:first-child": the scoping root mark was
 *    the compound's pseudo-class, so the second pseudo-class replaced it;
 *    the rule then had no :scope, became relative (":scope :first-child")
 *    and painted the first child of every root instead of the root that is
 *    a first child. The root mark is now its own field.
 * 2. "@scope (.a, .b)": the start (and end) took one selector; the rest of
 *    the list was dropped with an error. Lists of up to three selectors
 *    are kept now; a longer list keeps three and counts an error; an empty
 *    selector in the list drops the block, like an invalid selector list. */
#include "../../framework/unity.h"
#include "../../../userspace/wf_sitecss.h"
#include <string.h>
#include <stdio.h>

static wf_site_t g_d;

static const wf_site_node_t *by_id(const char *id) {
    int i;
    static wf_site_node_t none;
    for (i = 0; i < g_d.nnodes; i++) if (strcmp(g_d.node[i].id, id) == 0) return &g_d.node[i];
    return &none;
}

static void prep(const char *css, const char *body) {
    static char html[3072];
    int n = snprintf(html, sizeof(html), "<html><head><style>%s</style></head><body>%s</body></html>", css, body);
    TEST_ASSERT(n > 0 && n < (int)sizeof(html));
    TEST_ASSERT_EQUAL(0, wf_site_prepare(&g_d, html, n));
}

#define CARDS "<div><div class=card id=c1><p id=p1>a</p></div><div class=card id=c2><p id=p2>b</p></div></div>"

static void test_amp_with_pseudo_class(void) {
    prep("@scope (.card) { &:first-child { color: #ff0000 } }", CARDS);
    TEST_ASSERT_EQUAL(0xff0000, by_id("c1")->color);
    TEST_ASSERT_EQUAL(0, by_id("c2")->color_set);
    TEST_ASSERT_EQUAL(0, by_id("p1")->color_set);
    TEST_ASSERT_EQUAL(0, by_id("p2")->color_set);
}

static void test_scope_with_pseudo_class(void) {
    prep("@scope (.card) { :scope:last-child { font-weight: bold } }", CARDS);
    TEST_ASSERT_EQUAL(2, by_id("c2")->weight);
    TEST_ASSERT_EQUAL(0, by_id("c1")->weight);
    TEST_ASSERT_EQUAL(0, by_id("p2")->weight);
}

static void test_scope_pseudo_order_and_class(void) {
    /* the pseudo-class first, then :scope, then a class on the compound */
    prep("@scope (div) { :first-child:scope.card { color: #00ff00 } }", CARDS);
    TEST_ASSERT_EQUAL(0x00ff00, by_id("c1")->color);
    TEST_ASSERT_EQUAL(0, by_id("c2")->color_set);
    /* & then a descendant with its own pseudo-class */
    prep("@scope (.card) { & p:only-child { color: #0000ff } }", CARDS);
    TEST_ASSERT_EQUAL(0x0000ff, by_id("p1")->color);
    TEST_ASSERT_EQUAL(0x0000ff, by_id("p2")->color);
    TEST_ASSERT_EQUAL(0, by_id("c1")->color_set);
}

static void test_scope_specificity_with_pseudo(void) {
    /* :scope:first-child is (0,2,0) and beats div.card (0,1,1);
     * &:first-child is (0,1,0) (& adds nothing) and loses to div.card.
     * At equal specificity the scoped rule would win on proximity. */
    prep("@scope (.card) { :scope:first-child { color: #ff0000 } } div.card { color: #00ff00 }", CARDS);
    TEST_ASSERT_EQUAL(0xff0000, by_id("c1")->color);
    TEST_ASSERT_EQUAL(0x00ff00, by_id("c2")->color);
    prep("@scope (.card) { &:first-child { color: #ff0000 } } div.card { color: #00ff00 }", CARDS);
    TEST_ASSERT_EQUAL(0x00ff00, by_id("c1")->color);
    prep("@scope (.card) { &:first-child { color: #ff0000 } } .card { color: #00ff00 }", CARDS);
    TEST_ASSERT_EQUAL(0xff0000, by_id("c1")->color);
}

static void test_start_list(void) {
    prep("@scope (.a, .b) { p { color: #00ff00 } }",
         "<div class=a><p id=pa>a</p></div><div class=b><p id=pb>b</p></div><div class=c><p id=pc>c</p></div>");
    TEST_ASSERT_EQUAL(0x00ff00, by_id("pa")->color);
    TEST_ASSERT_EQUAL(0x00ff00, by_id("pb")->color);
    TEST_ASSERT_EQUAL(0, by_id("pc")->color_set);
    TEST_ASSERT_EQUAL(2, g_d.scope[0].nstart);
    TEST_ASSERT_EQUAL(0, g_d.errors);
}

static void test_start_list_with_combinators(void) {
    prep("@scope (main > .a, aside .b) { p { color: #123456 } }",
         "<main><div class=a><p id=x>x</p></div><div><div class=a><p id=y>y</p></div></div></main>"
         "<aside><div><div class=b><p id=z>z</p></div></div></aside><div class=b><p id=w>w</p></div>");
    TEST_ASSERT_EQUAL(0x123456, by_id("x")->color);
    TEST_ASSERT_EQUAL(0, by_id("y")->color_set);     /* .a not a child of main */
    TEST_ASSERT_EQUAL(0x123456, by_id("z")->color);
    TEST_ASSERT_EQUAL(0, by_id("w")->color_set);     /* .b not inside aside */
    TEST_ASSERT_EQUAL(2, g_d.scope[0].start_len[0]);
    TEST_ASSERT_EQUAL(2, g_d.scope[0].start_len[1]);
}

static void test_end_list(void) {
    prep("@scope (.box) to (.ad, figure) { p { color: #ff00ff } }",
         "<div class=box><p id=in>in</p><div class=ad><p id=ad>ad</p></div><figure><p id=fig>f</p></figure></div>");
    TEST_ASSERT_EQUAL(0xff00ff, by_id("in")->color);
    TEST_ASSERT_EQUAL(0, by_id("ad")->color_set);
    TEST_ASSERT_EQUAL(0, by_id("fig")->color_set);
    TEST_ASSERT_EQUAL(2, g_d.scope[0].nend);
}

static void test_long_list_keeps_three(void) {
    /* Lot 90 (converted, review point): the cap rose from three to six
     * selectors (WF_SITE_SCOPE_ALTS); the four-selector list of Lot 89 is
     * now whole, so the test uses seven: six are kept, the seventh is
     * dropped with an error. */
    prep("@scope (.a, .b, .c, .d, .e, .f, .g) { p { color: #0a0b0c } }",
         "<div class=a><p id=pa>a</p></div><div class=c><p id=pc>c</p></div><div class=d><p id=pd>d</p></div>"
         "<div class=f><p id=pf>f</p></div><div class=g><p id=pg>g</p></div>");
    TEST_ASSERT_EQUAL(1, g_d.nscopes);
    TEST_ASSERT_EQUAL(WF_SITE_SCOPE_ALTS, g_d.scope[0].nstart);
    TEST_ASSERT_EQUAL(6, WF_SITE_SCOPE_ALTS);
    TEST_ASSERT_EQUAL(0x0a0b0c, by_id("pa")->color);
    TEST_ASSERT_EQUAL(0x0a0b0c, by_id("pc")->color);
    TEST_ASSERT_EQUAL(0x0a0b0c, by_id("pd")->color);
    TEST_ASSERT_EQUAL(0x0a0b0c, by_id("pf")->color);
    TEST_ASSERT_EQUAL(0, by_id("pg")->color_set);
    TEST_ASSERT_EQUAL(1, g_d.errors);
}

static void test_empty_selector_in_list_drops_block(void) {
    prep("@scope (.a, ) { p { color: #ff0000 } } p { color: #00ff00 }",
         "<div class=a><p id=p>p</p></div>");
    TEST_ASSERT_EQUAL(0x00ff00, by_id("p")->color);
    TEST_ASSERT_EQUAL(0, g_d.nscopes);
    prep("@scope (, .a) { p { color: #ff0000 } } p { color: #00ff00 }",
         "<div class=a><p id=p>p</p></div>");
    TEST_ASSERT_EQUAL(0x00ff00, by_id("p")->color);
    TEST_ASSERT_EQUAL(0, g_d.nscopes);
    TEST_ASSERT(g_d.errors > 0);
}

static void test_nested_scope_with_list(void) {
    /* the inner start list is matched inside the outer scope only */
    prep("@scope (.outer) { @scope (.x, .y) { p { color: #abcdef } } }",
         "<div class=outer><div class=x><p id=ox>1</p></div><div class=y><p id=oy>2</p></div></div>"
         "<div class=x><p id=lx>3</p></div>");
    TEST_ASSERT_EQUAL(0xabcdef, by_id("ox")->color);
    TEST_ASSERT_EQUAL(0xabcdef, by_id("oy")->color);
    TEST_ASSERT_EQUAL(0, by_id("lx")->color_set);
    TEST_ASSERT_EQUAL(2, g_d.nscopes);
}

static void test_amp_pseudo_outside_scope(void) {
    /* outside @scope, :scope is :root; :scope:first-child is the html
     * element (the only top element) */
    prep(":scope:first-child { color: #111111 } :scope:last-child p { color: #222222 }",
         "<p id=p>p</p>");
    TEST_ASSERT_EQUAL(0x111111, g_d.node[0].color);
    TEST_ASSERT_EQUAL(0x222222, by_id("p")->color);
}

int main(void) {
    unity_init();
    RUN_TEST(test_amp_with_pseudo_class);
    RUN_TEST(test_scope_with_pseudo_class);
    RUN_TEST(test_scope_pseudo_order_and_class);
    RUN_TEST(test_scope_specificity_with_pseudo);
    RUN_TEST(test_start_list);
    RUN_TEST(test_start_list_with_combinators);
    RUN_TEST(test_end_list);
    RUN_TEST(test_long_list_keeps_three);
    RUN_TEST(test_empty_selector_in_list_drops_block);
    RUN_TEST(test_nested_scope_with_list);
    RUN_TEST(test_amp_pseudo_outside_scope);
    unity_print_results();
    unity_cleanup();
    return unity_stats.tests_failed == 0 ? 0 : 1;
}
