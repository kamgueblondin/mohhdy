/* test_lot89_elemvar.c - Lot 89: custom properties per element. A --x set
 * by any selector, a style attribute or an @scope block cascades on the
 * elements it matches and is inherited; a declaration whose var() names
 * such a property is read again for each element. */
#include "../../framework/unity.h"
#include <string.h>

#include "wf_sitecss.h"

static wf_site_t g_d;

static const wf_site_node_t *by_id(const char *id) {
    int i;
    static wf_site_node_t none;
    for (i = 0; i < g_d.nnodes; i++) if (strcmp(g_d.node[i].id, id) == 0) return &g_d.node[i];
    return &none;
}
static int idx_of(const char *id) {
    int i;
    for (i = 0; i < g_d.nnodes; i++) if (strcmp(g_d.node[i].id, id) == 0) return i;
    return -1;
}
#define FS(id) (by_id(id)->tok[WF_SITE_K_FSIZE - WF_SITE_K_LIST])
#define IND(id) (by_id(id)->tok[WF_SITE_K_INDENT - WF_SITE_K_LIST])

static int prep(const char *html) { return wf_site_prepare_vw(&g_d, html, (int)strlen(html), 1000); }

static void test_redefined_lower_in_the_tree(void) {
    static const char html[] =
        "<html><head><style>"
        ":root { --fs: 14px }"
        ".card { --fs: 20px }"
        ".card .big { --fs: 24px }"
        "p { font-size: var(--fs) }"
        "</style></head><body><p id=a>a</p>"
        "<div class=card><p id=b>b</p><div><p id=c>c</p></div><p id=d class=big>d</p></div>"
        "<p id=e>e</p></body></html>";
    TEST_ASSERT_EQUAL(0, prep(html));
    TEST_ASSERT_EQUAL_STRING("14px", FS("a"));
    TEST_ASSERT_EQUAL_STRING("20px", FS("b"));
    TEST_ASSERT_EQUAL_STRING("20px", FS("c"));   /* inherited through a div */
    TEST_ASSERT_EQUAL_STRING("24px", FS("d"));
    TEST_ASSERT_EQUAL_STRING("14px", FS("e"));
    TEST_ASSERT_EQUAL_STRING("20px", wf_site_node_var(&g_d, idx_of("c"), "--fs"));
    TEST_ASSERT_EQUAL_STRING("14px", wf_site_node_var(&g_d, idx_of("a"), "--fs"));
}

static void test_cascade_order_specificity_important(void) {
    static const char html[] =
        "<html><head><style>"
        "#x { --w: 30px }"
        ".k { --w: 10px }"
        ".k { --w: 12px }"
        ".imp { --w: 40px !important }"
        "#y { --w: 50px }"
        "p { text-indent: var(--w, 1px) }"
        "</style></head><body>"
        "<p id=x class=k>x</p><p id=z class=k>z</p><p id=y class=imp>y</p><p id=n>n</p></body></html>";
    TEST_ASSERT_EQUAL(0, prep(html));
    TEST_ASSERT_EQUAL_STRING("30px", IND("x"));   /* id beats class */
    TEST_ASSERT_EQUAL_STRING("12px", IND("z"));   /* later class rule */
    TEST_ASSERT_EQUAL_STRING("40px", IND("y"));   /* !important beats the id */
    TEST_ASSERT_EQUAL_STRING("1px", IND("n"));    /* none set: the fallback */
}

static void test_style_attribute_and_calc(void) {
    static const char html[] =
        "<html><head><style>"
        ".row { --gap: 4px }"
        "span { letter-spacing: calc(var(--gap) * 2) }"
        "</style></head><body>"
        "<div class=row><span id=a>a</span></div>"
        "<div class=row style=\"--gap: 6px\"><span id=b>b</span>"
        "<span id=c style=\"--gap: 1px; letter-spacing: var(--gap)\">c</span></div>"
        "</body></html>";
    TEST_ASSERT_EQUAL(0, prep(html));
    TEST_ASSERT_EQUAL_STRING("8px", by_id("a")->tok[WF_SITE_K_LSPACE - WF_SITE_K_LIST]);
    TEST_ASSERT_EQUAL_STRING("12px", by_id("b")->tok[WF_SITE_K_LSPACE - WF_SITE_K_LIST]);
    TEST_ASSERT_EQUAL_STRING("1px", by_id("c")->tok[WF_SITE_K_LSPACE - WF_SITE_K_LIST]);
}

static void test_invalid_per_element_and_colour(void) {
    static const char html[] =
        "<html><head><style>"
        ".ok { --c: #102030 }"
        ".bad { --c: notacolour }"
        "p { color: #000001 }"
        "p { color: var(--c) }"
        "</style></head><body>"
        "<div class=ok><p id=a>a</p></div><div class=bad><p id=b>b</p></div><p id=c>c</p></body></html>";
    char text[96];
    TEST_ASSERT_EQUAL(0, prep(html));
    TEST_ASSERT_EQUAL((int)0x102030, (int)by_id("a")->color);
    /* invalid where it is used: dropped on that element, the earlier rule stays */
    TEST_ASSERT_EQUAL((int)0x000001, (int)by_id("b")->color);
    /* no value at all and no fallback: dropped as well */
    TEST_ASSERT_EQUAL((int)0x000001, (int)by_id("c")->color);
    TEST_ASSERT(wf_site_vars_text(&g_d, text, (int)sizeof(text)) > 0);
    TEST_ASSERT_NOT_NULL(strstr(text, "names 1 values 2"));
    TEST_ASSERT_NOT_NULL(strstr(text, "bound 2 reread 1"));
    TEST_ASSERT_NOT_NULL(strstr(text, "dropped 2"));
}

static void test_inside_scope(void) {
    static const char html[] =
        "<html><head><style>"
        ":root { --s: 15px }"
        "@scope (.panel) { --s: 22px; .note { --s: 9px } }"
        "p { font-size: var(--s) }"
        "</style></head><body><p id=a>a</p>"
        "<div class=panel><p id=b>b</p><p id=c class=note>c</p></div></body></html>";
    TEST_ASSERT_EQUAL(0, prep(html));
    TEST_ASSERT_EQUAL_STRING("15px", FS("a"));
    TEST_ASSERT_EQUAL_STRING("22px", FS("b"));
    TEST_ASSERT_EQUAL_STRING("9px", FS("c"));
}

static void test_root_only_unchanged_and_bounds(void) {
    static const char html[] =
        "<html><head><style>"
        ":root { --r: 11px }"
        "p { font-size: var(--r) }"
        ".a{--v1:1px}.a{--v2:1px}.a{--v3:1px}.a{--v4:1px}.a{--v5:1px}.a{--v6:1px}"
        ".a{--v7:1px}.a{--v8:1px}.a{--v9:1px}.a{--v10:1px}.a{--v11:1px}.a{--v12:1px}.a{--v13:1px}"
        ".a { --long: aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa }"
        "</style></head><body><p id=a class=a>a</p></body></html>";
    char text[96];
    TEST_ASSERT_EQUAL(0, prep(html));
    TEST_ASSERT_EQUAL_STRING("11px", FS("a"));
    TEST_ASSERT_EQUAL(WF_SITE_VNAMES, g_d.nvnames);
    TEST_ASSERT(g_d.dropped_vars >= 2);
    TEST_ASSERT(wf_site_vars_text(&g_d, text, (int)sizeof(text)) > 0);
    TEST_ASSERT_NOT_NULL(strstr(text, "names 12 "));
}

int main(void) {
    unity_init();
    RUN_TEST(test_redefined_lower_in_the_tree);
    RUN_TEST(test_cascade_order_specificity_important);
    RUN_TEST(test_style_attribute_and_calc);
    RUN_TEST(test_invalid_per_element_and_colour);
    RUN_TEST(test_inside_scope);
    RUN_TEST(test_root_only_unchanged_and_bounds);
    unity_print_results();
    unity_cleanup();
    return unity_stats.tests_failed == 0 ? 0 : 1;
}
