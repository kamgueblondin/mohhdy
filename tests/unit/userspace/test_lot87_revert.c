/* Lot 87: revert-layer and revert in the reader stylesheet. A winning
 * revert-layer gives the property the value it would have without its
 * layer (unlayered rules and the style attribute count as the last
 * layers); revert drops the whole author origin, leaving the user-agent
 * sheet. */
#include "../../framework/unity.h"
#include "../../../userspace/wf_sitecss.h"
#include <string.h>
#include <stdio.h>

static wf_site_t g_d;

static const wf_site_node_t *by_id(const char *id) {
    int i;
    for (i = 0; i < g_d.nnodes; i++) if (strcmp(g_d.node[i].id, id) == 0) return &g_d.node[i];
    {
        /* no such id: a blank node, so the checks on it fail */
        static wf_site_node_t none;
        return &none;
    }
}

static void prep(const char *css, const char *body) {
    static char html[2048];
    int n = snprintf(html, sizeof(html), "<html><head><style>%s</style></head><body>%s</body></html>", css, body);
    TEST_ASSERT(n > 0 && n < (int)sizeof(html));
    TEST_ASSERT_EQUAL(0, wf_site_prepare(&g_d, html, n));
}

static void test_layer_back_to_earlier_layer(void) {
    prep("@layer base, theme;"
         "@layer base { p { color: #ff0000; } }"
         "@layer theme { p { color: #0000ff } p.r { color: revert-layer } }",
         "<p id=a>a</p><p id=b class=r>b</p>");
    TEST_ASSERT_EQUAL(0x0000ff, by_id("a")->color);
    /* theme's p.r wins in theme and reverts: base gives red, and theme's
     * own weaker p rule is not used either */
    TEST_ASSERT_EQUAL(0xff0000, by_id("b")->color);
    TEST_ASSERT_EQUAL(1, by_id("b")->color_set);
    TEST_ASSERT_EQUAL(1, g_d.reverts);
}

static void test_unlayered_is_the_last_layer(void) {
    prep("@layer base { p { color: #111111 } }"
         "p { color: #222222 } p.u { color: revert-layer }",
         "<p id=u class=u>u</p>");
    /* every unlayered rule goes, the layer is left */
    TEST_ASSERT_EQUAL(0x111111, by_id("u")->color);
}

static void test_nothing_left_unsets(void) {
    prep("@layer only { p { color: revert-layer; } }"
         "p.k { font-weight: bold }",
         "<p id=n>n</p><p id=k class=k>k</p>");
    TEST_ASSERT_EQUAL(0, by_id("n")->color_set);
    TEST_ASSERT_EQUAL(0, by_id("n")->color_rank);
    /* other properties are not touched */
    TEST_ASSERT_EQUAL(2, by_id("k")->weight);
    TEST_ASSERT_EQUAL(0, by_id("k")->color_set);
}

static void test_reverts_chain_down(void) {
    prep("@layer a, b, c;"
         "@layer a { p { text-align: center } }"
         "@layer b { p { text-align: revert-layer } }"
         "@layer c { p { text-align: revert-layer } }",
         "<p id=c>c</p>");
    TEST_ASSERT_EQUAL(2, by_id("c")->align);
    TEST_ASSERT_EQUAL(2, g_d.reverts);
}

static void test_losing_revert_does_nothing(void) {
    prep("@layer base { p { color: #ff0000 } }"
         "@layer theme { p { color: revert-layer } #g { color: #00ff00 } }",
         "<p id=g>g</p><p id=h>h</p>");
    TEST_ASSERT_EQUAL(0x00ff00, by_id("g")->color);
    TEST_ASSERT_EQUAL(0xff0000, by_id("h")->color);
}

static void test_important_order(void) {
    /* important: the earlier layer wins, so b's revert-layer loses */
    prep("@layer a { p { color: #0a0a0a !important } }"
         "@layer b { p { color: revert-layer !important } }",
         "<p id=i>i</p>");
    TEST_ASSERT_EQUAL(0x0a0a0a, by_id("i")->color);
    TEST_ASSERT_EQUAL(0, g_d.reverts);
    /* a's important revert-layer wins and goes back to b's important */
    prep("@layer a { p { color: revert-layer !important } }"
         "@layer b { p { color: #0b0b0b !important } }"
         "p { color: #0c0c0c }",
         "<p id=j>j</p>");
    TEST_ASSERT_EQUAL(0x0b0b0b, by_id("j")->color);
}

static void test_revert_goes_to_user_agent(void) {
    /* the user-agent sheet hides [hidden]; an author display:block shows
     * it; revert drops every author rule and the UA rule is back */
    prep("p { display: block } #rv { display: revert }",
         "<p hidden id=sh>s</p><p hidden id=rv>r</p>");
    TEST_ASSERT_EQUAL(WF_SITE_DISP_BLOCK, by_id("sh")->disp);
    TEST_ASSERT_EQUAL(WF_SITE_DISP_NONE, by_id("rv")->disp);
    TEST_ASSERT(wf_site_hidden_at(&g_d, by_id("rv")->start));
    TEST_ASSERT(!wf_site_hidden_at(&g_d, by_id("sh")->start));
    /* revert of a property the UA sheet does not set: unset */
    prep("@layer l { p { color: #333333 } } p { color: revert }", "<p id=x>x</p>");
    TEST_ASSERT_EQUAL(0, by_id("x")->color_set);
}

static void test_style_attribute(void) {
    /* the style attribute is the last author step: revert-layer there
     * gives the sheet's value, revert gives nothing */
    prep("p { color: #444444; font-style: italic }",
         "<p id=s1 style=\"color: revert-layer\">a</p><p id=s2 style=\"color: revert; font-style: normal\">b</p>");
    TEST_ASSERT_EQUAL(0x444444, by_id("s1")->color);
    TEST_ASSERT_EQUAL(0, by_id("s2")->color_set);
    TEST_ASSERT_EQUAL(1, by_id("s2")->italic);
    /* a sheet revert-layer under a plain style attribute changes nothing */
    prep("@layer z { p { color: #555555 } } p { color: revert-layer }",
         "<p id=s3 style=\"color: #666666\">c</p><p id=s4>d</p>");
    TEST_ASSERT_EQUAL(0x666666, by_id("s3")->color);
    TEST_ASSERT_EQUAL(0x555555, by_id("s4")->color);
}

static void test_box_and_unknown(void) {
    prep("@layer base { div { margin: 4px 8px } }"
         "@layer site { div { margin: revert-layer; width: revert-layer } div { padding: 2px } }"
         "div.p { padding: revert-layer }",
         "<div id=m class=p>m</div>");
    TEST_ASSERT_EQUAL(0x0F, by_id("m")->margin_set);
    TEST_ASSERT_EQUAL(4, by_id("m")->margin[0]);
    TEST_ASSERT_EQUAL(8, by_id("m")->margin[1]);
    /* the unlayered padding revert goes back to site's 2px */
    TEST_ASSERT_EQUAL(2, by_id("m")->pad[0]);
    TEST_ASSERT_EQUAL(0, g_d.errors);
}

static void test_report_shows_the_result(void) {
    static char out[2048];
    prep("@layer base { p { font-weight: bold } } @layer top { p { font-weight: revert-layer } }",
         "<p id=w>w</p>");
    TEST_ASSERT_EQUAL(2, by_id("w")->weight);
    TEST_ASSERT(wf_site_report(&g_d, out, (int)sizeof(out)) > 0);
    TEST_ASSERT_NOT_NULL(strstr(out, "weight bold"));
}

int main(void) {
    unity_init();
    RUN_TEST(test_layer_back_to_earlier_layer);
    RUN_TEST(test_unlayered_is_the_last_layer);
    RUN_TEST(test_nothing_left_unsets);
    RUN_TEST(test_reverts_chain_down);
    RUN_TEST(test_losing_revert_does_nothing);
    RUN_TEST(test_important_order);
    RUN_TEST(test_revert_goes_to_user_agent);
    RUN_TEST(test_style_attribute);
    RUN_TEST(test_box_and_unknown);
    RUN_TEST(test_report_shows_the_result);
    unity_print_results();
    unity_cleanup();
    return unity_stats.tests_failed == 0 ? 0 : 1;
}
