/* Lot 87: cascade layers in the reader stylesheet (@layer statements and
 * blocks, nested and anonymous layers, !important reversed), @import with
 * layer() and supports(), and the specificity/order packing fix. */
#include "../../framework/unity.h"
#include "../../../userspace/wf_sitecss.h"
#include "../../../userspace/wf_csslink.h"
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

static void test_statement_sets_order(void) {
    char t[160];
    TEST_ASSERT_EQUAL(0, prep(
        "<html><head><style>"
        "@layer base, theme;"
        "@layer theme { p { color: #0000ff } }"
        "@layer base { p { color: #ff0000 } #x { font-weight: bold } }"
        "</style></head><body><p id=x>a</p></body></html>"));
    /* theme is declared after base: it wins even though its block is first,
     * and even against the id selector of base for another property only */
    TEST_ASSERT_EQUAL(0x0000ff, by_id("x")->color);
    TEST_ASSERT_EQUAL(2, by_id("x")->weight);
    TEST_ASSERT(wf_site_layers_text(&g_d, t, (int)sizeof(t)) > 0);
    TEST_ASSERT_EQUAL_STRING("base,theme", t);
}

static void test_block_order_without_statement(void) {
    TEST_ASSERT_EQUAL(0, prep(
        "<html><head><style>"
        "@layer one { #y { color: #111111 } }"
        "@layer two { p { color: #222222 } }"
        "</style></head><body><p id=y>a</p></body></html>"));
    /* a later layer beats an earlier one whatever the specificity */
    TEST_ASSERT_EQUAL(0x222222, by_id("y")->color);
}

static void test_unlayered_wins(void) {
    TEST_ASSERT_EQUAL(0, prep(
        "<html><head><style>"
        "p { color: #00ff00 }"
        "@layer x { #z { color: #ff0000 } }"
        "@layer x { #z { text-align: center } }"
        "</style></head><body><p id=z>a</p></body></html>"));
    TEST_ASSERT_EQUAL(0x00ff00, by_id("z")->color);
    /* a property nobody else sets still comes from the layer, and a block
     * reopening the same name is the same layer */
    TEST_ASSERT_EQUAL(2, by_id("z")->align);
    TEST_ASSERT_EQUAL(1, g_d.nlayers);
}

static void test_important_reversed(void) {
    TEST_ASSERT_EQUAL(0, prep(
        "<html><head><style>"
        "@layer a, b;"
        "@layer a { p { color: #aa0000 !important } }"
        "@layer b { p { color: #0000bb !important } }"
        "p { color: #00cc00 }"
        "#w2 { color: #333333 !important }"
        "</style></head><body><p id=w1>a</p><p id=w2>b</p></body></html>"));
    /* important: the first layer wins, and it beats unlayered normal */
    TEST_ASSERT_EQUAL(0xaa0000, by_id("w1")->color);
    /* layered important beats unlayered important, even with an id */
    TEST_ASSERT_EQUAL(0xaa0000, by_id("w2")->color);
}

static void test_inline_still_wins(void) {
    TEST_ASSERT_EQUAL(0, prep(
        "<html><head><style>"
        "@layer a { p { color: #aa0000 } }"
        "</style></head><body><p id=i1 style=\"color: #010203\">a</p></body></html>"));
    TEST_ASSERT_EQUAL(0x010203, by_id("i1")->color);
}

static void test_nested_and_anonymous(void) {
    char t[200];
    TEST_ASSERT_EQUAL(0, prep(
        "<html><head><style>"
        "@layer outer { @layer inner { p { color: #101010 } p { text-decoration: underline } } "
        "p { font-weight: bold } p { text-decoration: none } }"
        "@layer { p { color: #202020 } }"
        "@layer { p { text-align: right } }"
        "@layer outer.inner { #n { text-align: center } }"
        "</style></head><body><p id=n>a</p></body></html>"));
    TEST_ASSERT(wf_site_layers_text(&g_d, t, (int)sizeof(t)) > 0);
    TEST_ASSERT_EQUAL_STRING("outer,outer.inner,#anon1,#anon2", t);
    /* the first anonymous layer comes after outer.inner: it wins color */
    TEST_ASSERT_EQUAL(0x202020, by_id("n")->color);
    TEST_ASSERT_EQUAL(2, by_id("n")->weight);
    /* inside outer, the rules written in outer beat its sublayer */
    TEST_ASSERT_EQUAL(1, by_id("n")->deco);
    /* positions: outer.inner 1, outer 2, #anon1 3, #anon2 4 */
    TEST_ASSERT_EQUAL(2, g_d.layer_rank[0]);
    TEST_ASSERT_EQUAL(1, g_d.layer_rank[1]);
    TEST_ASSERT_EQUAL(3, g_d.layer_rank[2]);
    /* the second anonymous layer is later than outer.inner: right wins */
    TEST_ASSERT_EQUAL(3, by_id("n")->align);
}

static void test_parent_made_by_child(void) {
    char t[120];
    TEST_ASSERT_EQUAL(0, prep(
        "<html><head><style>"
        "@layer a.b, c;"
        "@layer a { p { color: #0a0a0a } }"
        "@layer a.b { p { color: #0b0b0b } }"
        "@layer c { #m { font-weight: bold } }"
        "</style></head><body><p id=m>x</p></body></html>"));
    TEST_ASSERT(wf_site_layers_text(&g_d, t, (int)sizeof(t)) > 0);
    TEST_ASSERT_EQUAL_STRING("a,a.b,c", t);
    TEST_ASSERT_EQUAL(0x0a0a0a, by_id("m")->color);   /* a's own rules beat a.b */
    TEST_ASSERT_EQUAL(2, by_id("m")->weight);
}

static void test_layer_table_full(void) {
    static char html[2048];
    int k, n = 0;
    n += snprintf(html + n, sizeof(html) - (size_t)n, "<html><head><style>");
    for (k = 0; k < WF_SITE_LAYERS + 1; k++)
        n += snprintf(html + n, sizeof(html) - (size_t)n, "@layer l%d { #f%d { font-weight: bold } }", k, k);
    n += snprintf(html + n, sizeof(html) - (size_t)n, "</style></head><body>");
    for (k = 0; k < WF_SITE_LAYERS + 1; k++) n += snprintf(html + n, sizeof(html) - (size_t)n, "<p id=f%d>x</p>", k);
    snprintf(html + n, sizeof(html) - (size_t)n, "</body></html>");
    TEST_ASSERT_EQUAL(0, prep(html));
    TEST_ASSERT_EQUAL(WF_SITE_LAYERS, g_d.nlayers);
    TEST_ASSERT_EQUAL(1, g_d.dropped_layers);
    /* the rules of the dropped layer still apply (unlayered) */
    TEST_ASSERT_EQUAL(2, by_id("f12")->weight);
    TEST_ASSERT_EQUAL(2, by_id("f0")->weight);
}

static void test_specificity_beats_order(void) {
    static char html[2048];
    int k, n = 0;
    n += snprintf(html + n, sizeof(html) - (size_t)n, "<html><head><style>body p { font-weight: bold }");
    /* enough rules in between that order & 1023 is far above 10 */
    for (k = 0; k < 20; k++) n += snprintf(html + n, sizeof(html) - (size_t)n, ".c%d { color: #000001 }", k);
    n += snprintf(html + n, sizeof(html) - (size_t)n, "p { font-weight: normal }");
    snprintf(html + n, sizeof(html) - (size_t)n, "</style></head><body><p id=s>x</p></body></html>");
    TEST_ASSERT_EQUAL(0, prep(html));
    /* "body p" (two types) beats a later "p" */
    TEST_ASSERT_EQUAL(2, by_id("s")->weight);
}

static void test_import_conds(void) {
    char layer[32];
    int at = -1, layered = -1;
    const char *a = "layer(Base) supports(display: grid) screen and (min-width: 10px)";
    TEST_ASSERT_EQUAL(1, wf_site_import_conds(a, (int)strlen(a), &at, layer, (int)sizeof(layer), &layered));
    TEST_ASSERT_EQUAL(1, layered);
    TEST_ASSERT_EQUAL_STRING("base", layer);
    TEST_ASSERT_EQUAL_STRING("screen and (min-width: 10px)", a + at);
    a = "layer print";
    TEST_ASSERT_EQUAL(1, wf_site_import_conds(a, (int)strlen(a), &at, layer, (int)sizeof(layer), &layered));
    TEST_ASSERT_EQUAL(1, layered);
    TEST_ASSERT_EQUAL_STRING("", layer);
    TEST_ASSERT_EQUAL_STRING("print", a + at);
    a = "layers-not-a-layer";
    TEST_ASSERT_EQUAL(1, wf_site_import_conds(a, (int)strlen(a), &at, layer, (int)sizeof(layer), &layered));
    TEST_ASSERT_EQUAL(0, layered);
    TEST_ASSERT_EQUAL(0, at);
    a = "supports(not (display: grid))";
    TEST_ASSERT_EQUAL(0, wf_site_import_conds(a, (int)strlen(a), &at, layer, (int)sizeof(layer), &layered));
    a = "";
    TEST_ASSERT_EQUAL(1, wf_site_import_conds(a, 0, &at, layer, (int)sizeof(layer), &layered));
    TEST_ASSERT_EQUAL(0, layered);
}

static void test_import_layer_in_page(void) {
    TEST_ASSERT_EQUAL(0, prep(
        "<html><head><style>"
        "@import url(a.css) layer(site) screen;"
        "@import url(b.css) supports(not (display: grid));"
        "@import \"c.css\" layer;"
        "</style></head><body><p>x</p></body></html>"));
    TEST_ASSERT_EQUAL(2, wf_site_nlinks(&g_d));
    TEST_ASSERT_EQUAL_STRING("a.css", wf_site_link_at(&g_d, 0)->href);
    TEST_ASSERT_EQUAL_STRING("screen", wf_site_link_at(&g_d, 0)->media);
    TEST_ASSERT_EQUAL(1, wf_site_link_at(&g_d, 0)->layered);
    TEST_ASSERT_EQUAL_STRING("site", wf_site_link_at(&g_d, 0)->layer);
    TEST_ASSERT_EQUAL_STRING("c.css", wf_site_link_at(&g_d, 1)->href);
    TEST_ASSERT_EQUAL(1, wf_site_link_at(&g_d, 1)->layered);
    TEST_ASSERT_EQUAL_STRING("", wf_site_link_at(&g_d, 1)->layer);
}

/* ---- external sheets with layers ---- */
static const struct { const char *url; const char *body; } g_site[] = {
    { "http://h.test/site.css", "@layer reset, site;\n@import url(reset.css) layer(reset);\np { color: #00aa00 }" },
    { "http://h.test/reset.css", "p { color: #ff0000; font-weight: bold } #k { text-align: center }" },
    { "http://h.test/util.css", "#k { color: #0000ff }" },
    { "http://h.test/nested.css", "@import url(gone.css) supports(not (display: block));\n"
                                  "@import url(/../bad) ;\n#k { color: #123456 }" },
    { 0, 0 }
};
static int g_gets;
static int fake_get(void *ctx, const char *url, char *buf, int cap) {
    int i;
    (void)ctx;
    g_gets++;
    for (i = 0; g_site[i].url; i++) {
        if (strcmp(g_site[i].url, url) == 0) {
            int n = (int)strlen(g_site[i].body);
            if (n > cap) n = cap;
            memcpy(buf, g_site[i].body, (size_t)n);
            return n;
        }
    }
    return -1;
}

/* a supports() import inside a fetched sheet is counted, not fetched (QEMU
 * found that this path had no stats) */
static void test_nested_unsupported_counted(void) {
    static char ext[4096], line[200];
    static const char page[] = "<html><head><link rel=stylesheet href=\"nested.css\"></head><body></body></html>";
    wf_cssl_stats_t st;
    int n;
    g_gets = 0;
    n = wf_csslink_collect(page, (int)strlen(page), "http://h.test/", 640, 480, fake_get, 0,
                           ext, (int)sizeof(ext), &st);
    TEST_ASSERT(n > 0);
    TEST_ASSERT_EQUAL(1, st.unsupported);
    TEST_ASSERT_NULL(strstr(ext, "gone"));
    TEST_ASSERT(strstr(ext, "#123456") != 0);
    TEST_ASSERT(wf_csslink_stats_text(&st, line, (int)sizeof(line)) > 0);
    TEST_ASSERT(strstr(line, " unsupported 1\n") != 0);
}

static void test_collect_layers(void) {
    static char ext[8192], line[200];
    static const char page[] =
        "<html><head>"
        "<style>@layer util;\n@import url(util.css) layer(util);\n"
        "@import url(never.css) supports(not (display: block));</style>"
        "<link rel=stylesheet href=\"site.css\" >"
        "</head><body><p id=k>x</p></body></html>";
    wf_cssl_stats_t st;
    int n;
    g_gets = 0;
    n = wf_csslink_collect(page, (int)strlen(page), "http://h.test/index.html", 640, 480, fake_get, 0,
                           ext, (int)sizeof(ext), &st);
    TEST_ASSERT(n > 0);
    /* never.css is not fetched: supports() is false */
    TEST_ASSERT_EQUAL(1, st.unsupported);
    TEST_ASSERT_EQUAL(3, st.fetched);
    TEST_ASSERT_EQUAL(3, g_gets);
    TEST_ASSERT_EQUAL(2, st.layered);
    TEST_ASSERT(strstr(ext, "@layer util {\n#k { color: #0000ff }") != 0);
    /* the statement of site.css is written before the imported reset.css */
    {
        const char *stmt = strstr(ext, "@layer reset, site;");
        const char *reset = strstr(ext, "@layer reset {");
        TEST_ASSERT_NOT_NULL(stmt);
        TEST_ASSERT_NOT_NULL(reset);
        TEST_ASSERT(stmt < reset);
    }
    TEST_ASSERT(wf_csslink_stats_text(&st, line, (int)sizeof(line)) > 0);
    TEST_ASSERT(strstr(line, " layered 2 unsupported 1\n") != 0);
    /* the cascade: util and reset are layers, site.css itself is unlayered */
    TEST_ASSERT_EQUAL(0, wf_site_prepare_ext(&g_d, page, (int)strlen(page), 640, 480, ext, n));
    TEST_ASSERT_EQUAL(0x00aa00, by_id("k")->color);      /* unlayered p beats layered #k */
    TEST_ASSERT_EQUAL(2, by_id("k")->weight);            /* from reset, nobody else */
    TEST_ASSERT_EQUAL(2, by_id("k")->align);
}

int main(void) {
    unity_init();
    RUN_TEST(test_statement_sets_order);
    RUN_TEST(test_block_order_without_statement);
    RUN_TEST(test_unlayered_wins);
    RUN_TEST(test_important_reversed);
    RUN_TEST(test_inline_still_wins);
    RUN_TEST(test_nested_and_anonymous);
    RUN_TEST(test_parent_made_by_child);
    RUN_TEST(test_layer_table_full);
    RUN_TEST(test_specificity_beats_order);
    RUN_TEST(test_import_conds);
    RUN_TEST(test_import_layer_in_page);
    RUN_TEST(test_collect_layers);
    RUN_TEST(test_nested_unsupported_counted);
    unity_print_results();
    unity_cleanup();
    return unity_stats.tests_failed == 0 ? 0 : 1;
}
