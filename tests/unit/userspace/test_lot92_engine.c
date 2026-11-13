/* Lot 92 (US-031): what the reader writes for radian angles and for
 * registered values it computed, the engine draws. Each case runs a
 * sheet through the reader, puts the style attribute it gives on an
 * element and hands that element to the engine (webeng.c, unchanged):
 * the engine must take every declaration (no "refused" diagnostic). */
#include "../../framework/unity.h"
#include <string.h>
#include <stdio.h>
#include "wf_sitecss.h"
#include "webeng.h"

static wf_site_t g_d;
static we_doc_t g_doc;
static char g_attr[1400];
static char g_html[4096];

static we_node_t g_none;
static const we_node_t *g_n;

/* sets g_n to the engine's node (a zero node when a step failed) */
static void run_engine(const char *css) {
    int i, idx;
    g_n = &g_none;
    snprintf(g_html, sizeof(g_html), "<html><head><style>%s</style></head><body><div id=t>t</div></body></html>", css);
    TEST_ASSERT_EQUAL(0, wf_site_prepare(&g_d, g_html, (int)strlen(g_html)));
    g_attr[0] = 0;
    for (i = 0; i < g_d.nnodes; i++)
        if (!strcmp(g_d.node[i].id, "t")) wf_site_style_attr(&g_d, g_d.node[i].start, g_attr, (int)sizeof(g_attr));
    TEST_ASSERT(g_attr[0] != 0);
    snprintf(g_html, sizeof(g_html), "<html><body><div id=t%s>t</div></body></html>", g_attr);
    memset(&g_doc, 0, sizeof(g_doc));
    TEST_ASSERT_EQUAL(0, we_parse(&g_doc, g_html, (int)strlen(g_html)));
    TEST_ASSERT_EQUAL(0, we_style(&g_doc));
    for (i = 0; i < (int)g_doc.ndiag; i++) TEST_ASSERT(strstr(g_doc.diag[i], "refused") == 0);
    idx = we_find_id(&g_doc, "t");
    TEST_ASSERT(idx >= 0);
    g_n = &g_doc.nodes[idx];
}

static const we_node_t *engine_node(const char *css) {
    run_engine(css);
    return g_n;
}

static void test_radians_draw_quarter_turns(void) {
    const we_node_t *n = engine_node("#t { background-image: linear-gradient(1.5708rad, red, blue) }");
    TEST_ASSERT(strstr(g_attr, "linear-gradient(90deg, red, blue)") != 0);
    TEST_ASSERT_EQUAL(WE_GRAD_RIGHT, n->grad_dir);
    n = engine_node("#t { background-image: linear-gradient(3.1416RAD, red, blue) }");
    TEST_ASSERT_EQUAL(WE_GRAD_BOTTOM, n->grad_dir);
    n = engine_node("#t { background: linear-gradient(-1.5708rad, red, blue) no-repeat }");
    TEST_ASSERT_EQUAL(WE_GRAD_LEFT, n->grad_dir);
    n = engine_node("#t { background-image: linear-gradient(4.7124rad, red, blue) }");
    TEST_ASSERT_EQUAL(WE_GRAD_LEFT, n->grad_dir);
    n = engine_node("#t { background-image: linear-gradient(0rad, red, blue) }");
    TEST_ASSERT_EQUAL(WE_GRAD_TOP, n->grad_dir);
}

static void test_computed_radius_drawn(void) {
    const we_node_t *n = engine_node("@property --r { syntax: '<length>'; inherits: false; initial-value: 0px }"
                                     "#t { --r: calc(20px * 2); background-image: radial-gradient(circle var(--r), red, blue) }");
    TEST_ASSERT_EQUAL(WE_GRAD_RADIAL, n->grad_dir);
    TEST_ASSERT_EQUAL(40, n->grad_size[0]);
    n = engine_node("@property --r { syntax: '<length>'; inherits: false; initial-value: 0px }"
                    "#t { --r: round(up, 33px, 10px); background-image: radial-gradient(circle var(--r), red, blue) }");
    TEST_ASSERT_EQUAL(40, n->grad_size[0]);
}

static void test_computed_angle_drawn(void) {
    const we_node_t *n = engine_node("@property --a { syntax: '<angle>'; inherits: false; initial-value: 0deg }"
                                     "#t { --a: calc(0.25turn * 2); background-image: linear-gradient(var(--a), red, blue) }");
    TEST_ASSERT(strstr(g_attr, "linear-gradient(180deg, red, blue)") != 0);
    TEST_ASSERT_EQUAL(WE_GRAD_BOTTOM, n->grad_dir);
    n = engine_node("@property --a { syntax: '<angle>'; inherits: false; initial-value: 0deg }"
                    "#t { --a: mod(450deg, 360deg); background-image: linear-gradient(var(--a), red, blue) }");
    TEST_ASSERT_EQUAL(WE_GRAD_RIGHT, n->grad_dir);
}

int main(void) {
    unity_init();
    RUN_TEST(test_radians_draw_quarter_turns);
    RUN_TEST(test_computed_radius_drawn);
    RUN_TEST(test_computed_angle_drawn);
    unity_print_results();
    unity_cleanup();
    return unity_stats.tests_failed == 0 ? 0 : 1;
}
