/* Lot 88: @scope in the reader stylesheet (CSS Cascade 6). A scope is a
 * root (an element matching the start selector, or the parent of the
 * <style> block) and its descendants, minus the limits (descendants
 * matching the end selector) and what is under them. Rules inside are
 * relative to the root unless they say :scope or &. A nearer root beats
 * a farther one after specificity, before order. Also the all shorthand
 * and text-transform / white-space with every keyword. */
#include "../../framework/unity.h"
#include "../../../userspace/wf_sitecss.h"
#include <string.h>
#include <stdio.h>

static wf_site_t g_d;

static const wf_site_node_t *by_id(const char *id) {
    int i;
    for (i = 0; i < g_d.nnodes; i++) if (strcmp(g_d.node[i].id, id) == 0) return &g_d.node[i];
    {
        static wf_site_node_t none;
        return &none;
    }
}

static int idx_of(const char *id) {
    int i;
    for (i = 0; i < g_d.nnodes; i++) if (strcmp(g_d.node[i].id, id) == 0) return i;
    return -9;
}

static void prep_raw(const char *html) {
    int n = (int)strlen(html);
    TEST_ASSERT_EQUAL(0, wf_site_prepare(&g_d, html, n));
}

static void prep(const char *css, const char *body) {
    static char html[3072];
    int n = snprintf(html, sizeof(html), "<html><head><style>%s</style></head><body>%s</body></html>", css, body);
    TEST_ASSERT(n > 0 && n < (int)sizeof(html));
    prep_raw(html);
}

static void test_scope_start_only(void) {
    prep("@scope (.card) { p { color: #ff0000 } }",
         "<div class=card id=c><p id=in>a</p><div><p id=deep>b</p></div></div><p id=out>c</p>");
    TEST_ASSERT_EQUAL(0xff0000, by_id("in")->color);
    TEST_ASSERT_EQUAL(0xff0000, by_id("deep")->color);
    TEST_ASSERT_EQUAL(0, by_id("out")->color_set);
    TEST_ASSERT_EQUAL(1, g_d.nscopes);
    TEST_ASSERT_EQUAL(2, g_d.scoped_hits);
}

static void test_scope_limit(void) {
    /* .content and everything under it leave the scope */
    prep("@scope (.media) to (.content) { p { font-weight: bold } }",
         "<div class=media><p id=a>a</p><div class=content id=lim><p id=b>b</p></div></div>");
    TEST_ASSERT_EQUAL(2, by_id("a")->weight);
    TEST_ASSERT_EQUAL(0, by_id("b")->weight);
}

static void test_limit_with_child_combinator_keeps_the_limit_parent(void) {
    /* "to (:scope > .x)": only a direct .x child is a limit */
    prep("@scope (.m) to (:scope > .x) { em { font-style: italic } }",
         "<div class=m><div class=x><em id=e1>a</em></div>"
         "<div><div class=x><em id=e2>b</em></div></div></div>");
    TEST_ASSERT_EQUAL(0, by_id("e1")->italic);
    TEST_ASSERT_EQUAL(2, by_id("e2")->italic);
}

static void test_scope_and_amp_match_the_root(void) {
    prep("@scope (.box) { :scope { color: #00ff00 } & > p { text-align: center } }",
         "<div class=box id=bx><p id=p1>a</p><div><p id=p2>b</p></div></div>");
    TEST_ASSERT_EQUAL(0x00ff00, by_id("bx")->color);
    TEST_ASSERT_EQUAL(0, by_id("p1")->color_set);
    TEST_ASSERT_EQUAL(2, by_id("p1")->align);
    TEST_ASSERT_EQUAL(0, by_id("p2")->align);
}

static void test_leading_child_combinator(void) {
    prep("@scope (#r) { > span { text-decoration: underline } }",
         "<div id=r><span id=s1>a</span><b><span id=s2>b</span></b></div>");
    TEST_ASSERT_EQUAL(2, by_id("s1")->deco);
    TEST_ASSERT_EQUAL(0, by_id("s2")->deco);
}

static void test_relative_ancestor_must_be_inside(void) {
    /* "@scope (.in) { .a p }" is ":scope .a p": an .a outside the root
     * does not count */
    prep("@scope (.in) { .a p { color: #0000ff } }",
         "<div class=a><div class=in><p id=no>x</p><div class=a><p id=yes>y</p></div></div></div>");
    TEST_ASSERT_EQUAL(0, by_id("no")->color_set);
    TEST_ASSERT_EQUAL(0x0000ff, by_id("yes")->color);
}

static void test_explicit_scope_selector_may_reach_outside(void) {
    /* with :scope written, the rest of the selector is unrestricted */
    prep("@scope (.in) { .a :scope p { color: #0000ff } }",
         "<div class=a><div class=in><p id=p>x</p></div></div>");
    TEST_ASSERT_EQUAL(0x0000ff, by_id("p")->color);
}

static void test_proximity_beats_order(void) {
    /* same specificity: the nearer root wins even though the farther
     * one comes later */
    prep("@scope (.light) { p { color: #eeeeee } }"
         "@scope (.dark) { p { color: #111111 } }",
         "<div class=light><div class=dark><p id=d>d</p></div></div>"
         "<div class=dark><div class=light><p id=l>l</p></div></div>");
    TEST_ASSERT_EQUAL(0x111111, by_id("d")->color);
    TEST_ASSERT_EQUAL(0xeeeeee, by_id("l")->color);
}

static void test_specificity_beats_proximity(void) {
    prep("p.k { color: #222222 } @scope (.near) { p { color: #333333 } }",
         "<div class=near><p id=k class=k>k</p><p id=j>j</p></div>");
    TEST_ASSERT_EQUAL(0x222222, by_id("k")->color);
    TEST_ASSERT_EQUAL(0x333333, by_id("j")->color);
}

static void test_scoped_beats_unscoped_same_specificity(void) {
    /* an unscoped rule has infinite hops: the scoped one wins though it
     * comes first */
    prep("@scope (.s) { p { color: #444444 } } p { color: #555555 }",
         "<div class=s><p id=a>a</p></div><p id=b>b</p>");
    TEST_ASSERT_EQUAL(0x444444, by_id("a")->color);
    TEST_ASSERT_EQUAL(0x555555, by_id("b")->color);
}

static void test_prelude_specificity_not_added(void) {
    /* "@scope (#hero) { img }" has the specificity of img: a .x rule
     * outside beats it */
    prep(".x { font-weight: normal } @scope (#hero) { b { font-weight: bold } }",
         "<div id=hero><b id=b1 class=x>a</b><b id=b2>b</b></div>");
    TEST_ASSERT_EQUAL(1, by_id("b1")->weight);
    TEST_ASSERT_EQUAL(2, by_id("b2")->weight);
}

static void test_implicit_root_is_style_parent(void) {
    prep_raw("<html><body><div id=w1><style>@scope { p { color: #aa0000 } }</style><p id=a>a</p></div>"
             "<div id=w2><p id=b>b</p></div></body></html>");
    TEST_ASSERT_EQUAL(0xaa0000, by_id("a")->color);
    TEST_ASSERT_EQUAL(0, by_id("b")->color_set);
    TEST_ASSERT_EQUAL(idx_of("w1"), g_d.scope[0].owner);
}

static void test_head_style_scope_reaches_nothing_in_body(void) {
    /* a <style> in <head>: its parent is head, nothing of body is in it */
    prep("@scope { p { color: #aa0000 } }", "<p id=a>a</p>");
    TEST_ASSERT_EQUAL(0, by_id("a")->color_set);
}

static void test_declarations_in_scope_block(void) {
    prep("@scope (.note) { color: #123456; font-weight: bold; em { font-style: italic } text-align: right }",
         "<div class=note id=n><em id=e>e</em></div><div class=x id=x></div>");
    TEST_ASSERT_EQUAL(0x123456, by_id("n")->color);
    TEST_ASSERT_EQUAL(2, by_id("n")->weight);
    TEST_ASSERT_EQUAL(3, by_id("n")->align);
    TEST_ASSERT_EQUAL(2, by_id("e")->italic);
    TEST_ASSERT_EQUAL(0, by_id("x")->color_set);
}

static void test_nested_scope(void) {
    prep("@scope (.outer) { @scope (:scope > .inner) { p { color: #0a0a0a } } }",
         "<div class=outer><div class=inner><p id=a>a</p></div><div><div class=inner><p id=b>b</p></div></div></div>"
         "<div class=inner><p id=c>c</p></div>");
    TEST_ASSERT_EQUAL(0x0a0a0a, by_id("a")->color);
    TEST_ASSERT_EQUAL(0, by_id("b")->color_set);
    TEST_ASSERT_EQUAL(0, by_id("c")->color_set);
    TEST_ASSERT_EQUAL(2, g_d.nscopes);
    TEST_ASSERT_EQUAL(0, g_d.scope[1].parent);
}

static void test_bad_prelude_drops_block(void) {
    prep("@scope (.a) junk { p { color: #ff0000 } } p.k { color: #00ff00 }",
         "<div class=a><p id=p class=k>p</p></div>");
    TEST_ASSERT_EQUAL(0x00ff00, by_id("p")->color);
    TEST_ASSERT_EQUAL(0, g_d.nscopes);
    TEST_ASSERT(g_d.errors > 0);
}

static void test_scope_in_media_and_layer(void) {
    prep("@media (min-width: 100px) { @scope (.m) { p { color: #010203 } } }"
         "@layer l { @scope (.m) { span { color: #040506 } } } span { color: #070809 }",
         "<div class=m><p id=p>p</p><span id=s>s</span></div>");
    TEST_ASSERT_EQUAL(0x010203, by_id("p")->color);
    /* unlayered beats a layer whatever the proximity */
    TEST_ASSERT_EQUAL(0x070809, by_id("s")->color);
}

static void test_scope_table_full(void) {
    char css[1200];
    int k, n = 0;
    for (k = 0; k < WF_SITE_SCOPES + 2; k++)
        n += snprintf(css + n, sizeof(css) - (size_t)n, "@scope (.c%d) { p { color: #0000%02x } }", k, k + 1);
    prep(css, "<div class=c9><p id=p>p</p></div><div class=c0><p id=q>q</p></div>");
    TEST_ASSERT_EQUAL(WF_SITE_SCOPES, g_d.nscopes);
    TEST_ASSERT_EQUAL(2, g_d.dropped_scopes);
    /* the dropped blocks apply nothing (not widened to the document) */
    TEST_ASSERT_EQUAL(0, by_id("p")->color_set);
    TEST_ASSERT_EQUAL(0x000001, by_id("q")->color);
}

static void test_scope_root_api_and_text(void) {
    char t[64];
    int sr;
    prep("@scope (.r) { p { color: #ff0000 } } i { color: #00ff00 }",
         "<div class=r id=r><div><p id=p>p</p></div></div><p id=o>o</p>");
    /* the user-agent rules come first: the scoped one is the one with
     * a scope, the i rule the last */
    for (sr = 0; sr < g_d.nrules && !g_d.rule[sr].scope; sr++) {}
    TEST_ASSERT(sr < g_d.nrules);
    TEST_ASSERT_EQUAL(idx_of("r"), wf_site_scope_root(&g_d, sr, idx_of("p")));
    TEST_ASSERT_EQUAL(-1, wf_site_scope_root(&g_d, sr, idx_of("o")));
    TEST_ASSERT_EQUAL(-2, wf_site_scope_root(&g_d, g_d.nrules - 1, idx_of("o")));
    TEST_ASSERT(wf_site_scope_text(&g_d, t, (int)sizeof(t)) > 0);
    TEST_ASSERT_EQUAL_STRING("scopes 1 rules 1 hits 1", t);
}

static void test_all_initial_and_unset(void) {
    prep("div { color: #ff0000; font-weight: bold; text-transform: uppercase; display: block }"
         "p.i { all: initial } p.u { all: unset }",
         "<div><p id=i class=i>i</p><p id=u class=u>u</p></div>");
    /* initial: inline, black, normal, none */
    TEST_ASSERT_EQUAL(WF_SITE_DISP_INLINE, by_id("i")->disp);
    TEST_ASSERT_EQUAL(0x000000, by_id("i")->color);
    TEST_ASSERT_EQUAL(1, by_id("i")->color_set);
    TEST_ASSERT_EQUAL(1, by_id("i")->weight);
    TEST_ASSERT_EQUAL(1, by_id("i")->transform);
    /* unset: the inherited ones from the div, display initial */
    TEST_ASSERT_EQUAL(0xff0000, by_id("u")->color);
    TEST_ASSERT_EQUAL(2, by_id("u")->weight);
    TEST_ASSERT_EQUAL(2, by_id("u")->transform);
    TEST_ASSERT_EQUAL(WF_SITE_DISP_INLINE, by_id("u")->disp);
}

static void test_all_revert_layer_and_later_longhand(void) {
    prep("@layer a, b; @layer a { p { color: #0000aa; font-style: italic } }"
         "@layer b { p { color: #00aa00 } p.r { all: revert-layer } }"
         "p.r { font-weight: bold }",
         "<p id=r class=r>r</p>");
    TEST_ASSERT_EQUAL(0x0000aa, by_id("r")->color);
    TEST_ASSERT_EQUAL(2, by_id("r")->italic);
    /* the unlayered longhand beats the layered all */
    TEST_ASSERT_EQUAL(2, by_id("r")->weight);
}

static void test_all_bad_value_ignored(void) {
    prep("p { color: #abcdef } p { all: red }", "<p id=p>p</p>");
    TEST_ASSERT_EQUAL(0xabcdef, by_id("p")->color);
}

static void test_transform_and_white_space(void) {
    char st[200];
    const wf_site_node_t *nd;
    prep("article { text-transform: capitalize; white-space: pre-line }"
         "h1 { text-transform: uppercase } code { white-space: inherit } q { white-space: initial }"
         "em { text-transform: unset }",
         "<article id=a><h1 id=h>t</h1><code id=c>x</code><q id=q>y</q><em id=e>z</em></article>");
    TEST_ASSERT_EQUAL(4, by_id("a")->transform);
    TEST_ASSERT_EQUAL(5, by_id("a")->ws);
    TEST_ASSERT_EQUAL(2, by_id("h")->transform);
    TEST_ASSERT_EQUAL(5, by_id("c")->ws);
    TEST_ASSERT_EQUAL(1, by_id("q")->ws);
    TEST_ASSERT_EQUAL(4, by_id("e")->transform);
    nd = by_id("h");
    TEST_ASSERT(wf_site_style_attr(&g_d, nd->start, st, (int)sizeof(st)) > 0);
    TEST_ASSERT(strstr(st, "text-transform:uppercase") != 0);
    nd = by_id("a");
    TEST_ASSERT(wf_site_style_attr(&g_d, nd->start, st, (int)sizeof(st)) > 0);
    TEST_ASSERT(strstr(st, "white-space:pre-line") != 0);
    TEST_ASSERT(strstr(st, "text-transform:capitalize") != 0);
}

static void test_supports_new_properties(void) {
    TEST_ASSERT_EQUAL(1, wf_site_supports("(white-space: pre)", 18));
    TEST_ASSERT_EQUAL(1, wf_site_supports("(all: unset)", 12));
}

static void test_style_attribute_all(void) {
    prep("p { color: #ff0000 }", "<p id=p style=\"all: revert\">p</p>");
    /* revert in the style attribute drops the author origin */
    TEST_ASSERT_EQUAL(0, by_id("p")->color_set);
}

int main(void) {
    unity_init();
    RUN_TEST(test_scope_start_only);
    RUN_TEST(test_scope_limit);
    RUN_TEST(test_limit_with_child_combinator_keeps_the_limit_parent);
    RUN_TEST(test_scope_and_amp_match_the_root);
    RUN_TEST(test_leading_child_combinator);
    RUN_TEST(test_relative_ancestor_must_be_inside);
    RUN_TEST(test_explicit_scope_selector_may_reach_outside);
    RUN_TEST(test_proximity_beats_order);
    RUN_TEST(test_specificity_beats_proximity);
    RUN_TEST(test_scoped_beats_unscoped_same_specificity);
    RUN_TEST(test_prelude_specificity_not_added);
    RUN_TEST(test_implicit_root_is_style_parent);
    RUN_TEST(test_head_style_scope_reaches_nothing_in_body);
    RUN_TEST(test_declarations_in_scope_block);
    RUN_TEST(test_nested_scope);
    RUN_TEST(test_bad_prelude_drops_block);
    RUN_TEST(test_scope_in_media_and_layer);
    RUN_TEST(test_scope_table_full);
    RUN_TEST(test_scope_root_api_and_text);
    RUN_TEST(test_all_initial_and_unset);
    RUN_TEST(test_all_revert_layer_and_later_longhand);
    RUN_TEST(test_all_bad_value_ignored);
    RUN_TEST(test_transform_and_white_space);
    RUN_TEST(test_supports_new_properties);
    RUN_TEST(test_style_attribute_all);
    unity_print_results();
    unity_cleanup();
    return unity_stats.tests_failed == 0 ? 0 : 1;
}
