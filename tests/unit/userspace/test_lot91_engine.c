/* Lot 91 (US-031): what the reader writes for backgrounds, the engine
 * draws. Each case runs a sheet through the reader, puts the style
 * attribute it gives on an element and hands that element to the engine
 * (webeng.c): the engine must take every declaration (no "refused"
 * diagnostic) and keep the radii, the shape, the clip and the origin. */
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

static void test_radial_circle_radius(void) {
    const we_node_t *n = engine_node("#t { background: radial-gradient(circle 40px, red, blue) }");
    TEST_ASSERT_EQUAL(WE_GRAD_RADIAL, n->grad_dir);
    TEST_ASSERT_EQUAL(1, n->grad_shape & 1);              /* circle */
    TEST_ASSERT_EQUAL(40, n->grad_size[0]);
    TEST_ASSERT_EQUAL(40, n->grad_size[1]);
}

static void test_radial_ellipse_radii(void) {
    const we_node_t *n = engine_node("#t { background-image: radial-gradient(30px 20% at left top, red, blue) }");
    TEST_ASSERT_EQUAL(WE_GRAD_RADIAL, n->grad_dir);
    TEST_ASSERT_EQUAL(0, n->grad_shape & 1);              /* ellipse */
    TEST_ASSERT_EQUAL(30, n->grad_size[0]);
    TEST_ASSERT(n->grad_at_pct & 8);                       /* ry in percent */
    TEST_ASSERT_EQUAL(0, n->grad_at[0]);
    TEST_ASSERT_EQUAL(0, n->grad_at[1]);
}

static void test_radial_implied_circle(void) {
    const we_node_t *n = engine_node("#t { background-image: radial-gradient(25px at center, red, blue) }");
    TEST_ASSERT_EQUAL(1, n->grad_shape & 1);
    TEST_ASSERT_EQUAL(25, n->grad_size[0]);
}

static void test_gradians_draw_quarter_turns(void) {
    const we_node_t *n = engine_node("#t { background-image: linear-gradient(100grad, red, blue) }");
    TEST_ASSERT_EQUAL(WE_GRAD_RIGHT, n->grad_dir);
    n = engine_node("#t { background-image: linear-gradient(300grad, red, blue) }");
    TEST_ASSERT_EQUAL(WE_GRAD_LEFT, n->grad_dir);
    n = engine_node("#t { background-image: linear-gradient(-100grad, red, blue) }");
    TEST_ASSERT_EQUAL(WE_GRAD_LEFT, n->grad_dir);
}

static void test_clip_and_origin(void) {
    const we_node_t *n = engine_node("#t { background-clip: content-box; background-origin: border-box }");
    TEST_ASSERT_EQUAL(WE_BOX_CONTENT, n->bg_clip);
    TEST_ASSERT_EQUAL(WE_BOX_BORDER, n->bg_origin);
    n = engine_node("#t { background: url(a.png) padding-box }");
    TEST_ASSERT_EQUAL(WE_BOX_PADDING, n->bg_clip);
    TEST_ASSERT_EQUAL(WE_BOX_PADDING, n->bg_origin);
    n = engine_node("#t { background: red content-box border-box }");
    TEST_ASSERT_EQUAL(WE_BOX_BORDER, n->bg_clip);
    TEST_ASSERT_EQUAL(WE_BOX_CONTENT, n->bg_origin);
    /* a list that agrees is one keyword for the engine */
    n = engine_node("#t { background-clip: padding-box, padding-box }");
    TEST_ASSERT_EQUAL(WE_BOX_PADDING, n->bg_clip);
}

static void test_shorthand_with_everything(void) {
    const we_node_t *n = engine_node("#t { background: url(a.png) no-repeat content-box, radial-gradient(ellipse 10px 30px, red, blue) "
                                     "content-box navy }");
    TEST_ASSERT(strstr(g_attr, "background-clip:content-box") != 0);
    TEST_ASSERT(strstr(g_attr, "background-origin:content-box") != 0);
    TEST_ASSERT_EQUAL(WE_BOX_CONTENT, n->bg_clip);
    TEST_ASSERT_EQUAL(WE_GRAD_RADIAL, n->grad_dir);
    TEST_ASSERT_EQUAL(10, n->grad_size[0]);
    TEST_ASSERT_EQUAL(30, n->grad_size[1]);
}

int main(void) {
    unity_init();
    RUN_TEST(test_radial_circle_radius);
    RUN_TEST(test_radial_ellipse_radii);
    RUN_TEST(test_radial_implied_circle);
    RUN_TEST(test_gradians_draw_quarter_turns);
    RUN_TEST(test_clip_and_origin);
    RUN_TEST(test_shorthand_with_everything);
    unity_print_results();
    unity_cleanup();
    return unity_stats.tests_failed == 0 ? 0 : 1;
}
