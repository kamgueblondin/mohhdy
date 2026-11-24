/* Lot 121 (US-031): flex items. The flex shorthand and flex-flow (CSS
 * Flexbox 7.1.1, 5.3), the automatic minimum size of column flex items
 * (4.5), min-height auto and max-height none, and column containers always
 * on the flex algorithm (auto bases from the content, percentage and
 * definite heights, default shrink). Values follow docs/lot121_plan.md. */
#include "../../framework/unity.h"
#include <string.h>
#include <stdio.h>
#include "webeng.h"

#define EQ_MSG(e, a, m) do { int e_ = (int)(e), a_ = (int)(a); \
    if (e_ != a_) printf("  %s: expected %d, got %d\n", (m), e_, a_); \
    TEST_ASSERT_EQUAL(e_, a_); } while (0)

static we_doc_t g_doc;
static we_forms_t g_forms;
static we_node_t g_none;

static void lays(const char *html) {
    we_forms_init(&g_forms);
    we_set_forms(&g_doc, &g_forms);
    TEST_ASSERT_EQUAL(0, we_parse(&g_doc, html, (int)strlen(html)));
    TEST_ASSERT_EQUAL(0, we_style(&g_doc));
    TEST_ASSERT_EQUAL(0, we_layout_viewport(&g_doc, 640, 480));
}

static const we_node_t *el(const char *id) {
    int i = we_find_id(&g_doc, id);
    g_none.h = -1; g_none.w = -1; g_none.x = -1; g_none.y = -1;
    return i >= 0 ? &g_doc.nodes[i] : &g_none;
}

static int diag_count(const char *t) {
    uint32_t i;
    int c = 0;
    for (i = 0; i < g_doc.ndiag; i++) if (strstr(g_doc.diag[i], t)) c++;
    return c;
}

#define BOX(id, X, Y, W, H) do { const we_node_t *n_ = el(id); \
    EQ_MSG(X, n_->x, id " x"); EQ_MSG(Y, n_->y, id " y"); EQ_MSG(W, n_->w, id " w"); EQ_MSG(H, n_->h, id " h"); } while (0)

/* the flex fields of #i after "pre; flex: css" */
static const we_node_t *flex_item(const char *pre, const char *css) {
    static char html[512];
    snprintf(html, sizeof(html), "<div style='display:flex'><div id=i style='%s;flex:%s'>x</div></div>", pre, css);
    lays(html);
    return el("i");
}

/* grow, shrink, basis px (-1 auto), basis percent, content basis */
static void flex_is(const char *css, int grow, int shrink, int basis, int pct, int content) {
    const we_node_t *n = flex_item("order:0", css);
    EQ_MSG(grow, n->flex_grow, css);
    EQ_MSG(shrink, n->flex_shrink, css);
    EQ_MSG(basis, n->flex_basis, css);
    EQ_MSG(pct, n->basis_pct, css);
    EQ_MSG(content, n->basis_content, css);
    EQ_MSG(1, n->basis_set, css);
    EQ_MSG(0, diag_count("refused"), css);
}

/* --- the flex shorthand --- */
static void test_l121_flex_shorthand(void) {
    flex_is("1", 1, 1, 0, 0, 0);
    flex_is("0", 0, 1, 0, 0, 0);
    flex_is("2 3", 2, 3, 0, 0, 0);
    flex_is("0 0", 0, 0, 0, 0, 0);
    flex_is("1 1 0", 1, 1, 0, 0, 0);
    flex_is("1 2 30px", 1, 2, 30, 0, 0);
    flex_is("30px 1 2", 1, 2, 30, 0, 0);
    flex_is("3 30px", 3, 1, 30, 0, 0);
    flex_is("30px 3", 3, 1, 30, 0, 0);
    flex_is("10px", 1, 1, 10, 0, 0);
    flex_is("auto", 1, 1, -1, 0, 0);
    flex_is("none", 0, 0, -1, 0, 0);
    flex_is("initial", 0, 1, -1, 0, 0);
    flex_is("2 auto", 2, 1, -1, 0, 0);
    flex_is("50%", 1, 1, -1, 50, 0);
    flex_is("1 0%", 1, 1, 0, 0, 0);
    flex_is("content", 1, 1, -1, 0, 1);
    flex_is("2 0 content", 2, 0, -1, 0, 1);
    flex_is("calc(1 + 2) 4", 3, 4, 0, 0, 0);
    flex_is("1000 1000", 1000, 1000, 0, 0, 0);
    /* a math basis goes to the flex-basis slot */
    {
        const we_node_t *n = flex_item("order:0", "1 calc(10px + 5px)");
        EQ_MSG(1, n->flex_grow, "math basis grow");
        TEST_ASSERT_TRUE(n->length[WE_LEN_BASIS].count > 0);
        EQ_MSG(0, diag_count("refused"), "math basis");
    }
    /* the last declaration wins, the shorthand resets an earlier longhand */
    {
        const we_node_t *n = flex_item("flex-grow:5;flex-shrink:6;flex-basis:7px", "auto");
        EQ_MSG(1, n->flex_grow, "reset grow"); EQ_MSG(1, n->flex_shrink, "reset shrink"); EQ_MSG(-1, n->flex_basis, "reset basis");
    }
}

/* refused: one "flex refused", no longhand diagnostic, the earlier values kept */
static void test_l121_flex_refused(void) {
    static const char *const bad[] = {
        "1 10px 2", "a", "1 2 3", "1 2 3px 4", "10px 20px", "none 1", "auto auto", "-1", "1001",
        "1 1 1 1", "1 2 0 0", "1.5", "red", "1 calc(1px + )", "0 0 0 0", "inherit", "unset"
    };
    unsigned k;
    for (k = 0; k < sizeof(bad) / sizeof(bad[0]); k++) {
        const we_node_t *n = flex_item("flex:2 3 4px", bad[k]);
        EQ_MSG(2, n->flex_grow, bad[k]);
        EQ_MSG(3, n->flex_shrink, bad[k]);
        EQ_MSG(4, n->flex_basis, bad[k]);
        EQ_MSG(1, diag_count("flex refused"), bad[k]);
        EQ_MSG(1, (int)g_doc.ndiag, bad[k]);
    }
    /* a refused shorthand drops a refused basis' own diagnostic even when the list is full */
    {
        static char html[4096];
        int i, p = 0;
        p += snprintf(html + p, sizeof(html) - p, "<div style='display:flex'>");
        for (i = 0; i < WE_MAX_DIAG; i++) p += snprintf(html + p, sizeof(html) - p, "<b style='flex-wrap:x'></b>");
        snprintf(html + p, sizeof(html) - p, "<div id=i style='flex:2 3 4px;flex:1 -5px'>x</div></div>");
        lays(html);
        EQ_MSG(WE_MAX_DIAG, (int)g_doc.ndiag, "full list");
        EQ_MSG(2, el("i")->flex_grow, "full list grow");
        EQ_MSG(1, (int)g_doc.diag_dropped, "dropped: the basis one taken back, flex refused counted");
    }
}

/* --- flex-flow --- */
static void test_l121_flex_flow(void) {
    lays("<div id=a style='display:flex;flex-flow:column wrap'></div><div id=b style='display:flex;flex-flow:wrap-reverse column-reverse'></div>"
         "<div id=c style='display:flex;flex-wrap:wrap;flex-direction:column;flex-flow:row'></div><div id=d style='display:flex;flex-flow:nowrap'></div>"
         "<div id=e style='display:flex;flex-flow:row-reverse'></div><div id=f style='display:flex;flex-flow:wrap'></div>");
    EQ_MSG(1, el("a")->flex_dir, "a dir"); EQ_MSG(0, el("a")->flex_reverse, "a rev"); EQ_MSG(1, el("a")->flex_wrap, "a wrap");
    EQ_MSG(1, el("b")->flex_dir, "b dir"); EQ_MSG(1, el("b")->flex_reverse, "b rev"); EQ_MSG(2, el("b")->flex_wrap, "b wrap");
    EQ_MSG(0, el("c")->flex_dir, "c dir"); EQ_MSG(0, el("c")->flex_wrap, "c wrap");
    EQ_MSG(0, el("d")->flex_dir, "d dir"); EQ_MSG(0, el("d")->flex_wrap, "d wrap");
    EQ_MSG(0, el("e")->flex_dir, "e dir"); EQ_MSG(1, el("e")->flex_reverse, "e rev");
    EQ_MSG(0, el("f")->flex_dir, "f dir"); EQ_MSG(1, el("f")->flex_wrap, "f wrap");
    EQ_MSG(0, (int)g_doc.ndiag, "no diag");
    lays("<div id=a style='display:flex;flex-flow:column wrap;flex-flow:row column'></div><div id=b style='display:flex;flex-flow:column wrap;flex-flow:wrap nowrap'></div>"
         "<div id=c style='display:flex;flex-flow:column wrap;flex-flow:diagonal'></div><div id=d style='display:flex;flex-flow:column wrap;flex-flow:row wrap row'></div>");
    EQ_MSG(4, diag_count("flex-flow refused"), "refused");
    EQ_MSG(1, el("a")->flex_dir, "kept dir"); EQ_MSG(1, el("a")->flex_wrap, "kept wrap");
    EQ_MSG(1, el("b")->flex_dir, "b kept"); EQ_MSG(1, el("c")->flex_wrap, "c kept"); EQ_MSG(1, el("d")->flex_dir, "d kept");
    /* each shorthand counts as one declaration, refused or not (its flex-basis
     * pass is not counted again) */
    lays("<div style='display:flex'><div id=a style='flex:1 2 30px;flex:red;flex-flow:column wrap'>x</div></div>");
    EQ_MSG(4, (int)g_doc.perf.decls_applied, "decls applied");
    EQ_MSG(0, (int)g_doc.perf.unknown_props, "no unknown property");
}

/* --- column growth in definite heights --- */
static void test_l121_column_grow(void) {
    /* the Lot 120 finding: body flex:1 column in an html of height 100% */
    lays("<html style='height:100%'><body style='margin:0;height:100%;display:flex;flex-direction:column'>"
         "<div id=h style='height:20px'></div><div id=m style='flex:1'></div><div id=f style='height:30px'></div></body></html>");
    BOX("h", 0, 0, 640, 20); BOX("m", 0, 20, 640, 430); BOX("f", 0, 450, 640, 30);
    /* the longhand alone, same chain */
    lays("<html style='height:100%'><body style='margin:0;height:100%;display:flex;flex-direction:column'>"
         "<div id=h style='height:20px'>a</div><div id=m style='flex-grow:1'>b</div><div id=f style='height:30px'>c</div></body></html>");
    BOX("m", 0, 20, 640, 430); BOX("f", 0, 450, 640, 30);
    /* proportional shares of the free space */
    lays("<div id=c style='display:flex;flex-direction:column;height:300px'><div id=a style='flex:1'></div><div id=b style='flex:2'></div><div id=x style='height:30px'></div></div>");
    BOX("a", 0, 0, 640, 90); BOX("b", 0, 90, 640, 180); BOX("x", 0, 270, 640, 30);
    /* an auto basis is the content height; a 0 basis is 0 */
    lays("<div style='display:flex;flex-direction:column;height:300px'><div id=a style='flex-grow:1'>t</div><div id=b style='flex:1 1 0'>u</div></div>");
    BOX("a", 0, 0, 640, 155); BOX("b", 0, 155, 640, 145);
    lays("<div style='display:flex;flex-direction:column;height:100px'><div id=a style='flex-grow:1'>t<br>t<br>t</div><div id=b style='flex-grow:1'>u</div></div>");
    BOX("a", 0, 0, 640, 60); BOX("b", 0, 60, 640, 40);
    /* percent height of the container itself */
    lays("<div style='height:200px'><div style='display:flex;flex-direction:column;height:50%'><div id=a style='flex:1'></div><div id=b style='flex:3'></div></div></div>");
    BOX("a", 0, 0, 640, 25); BOX("b", 0, 25, 640, 75);
    /* max-height caps a grower; the rest goes to the other one */
    lays("<div style='display:flex;flex-direction:column;height:100px'><div id=a style='flex:1;max-height:20px'></div><div id=b style='flex:1'></div></div>");
    BOX("a", 0, 0, 640, 20); BOX("b", 0, 20, 640, 80);
    /* gaps, padding and margins of items */
    lays("<div style='display:flex;flex-direction:column;height:100px;row-gap:10px'><div id=a style='flex:1;padding:5px'></div><div id=b style='flex:1;margin-top:10px'></div></div>");
    BOX("a", 0, 0, 640, 45); BOX("b", 0, 65, 640, 35);   /* outer bases 10 and 10: 35 each more */
    /* the row direction is unchanged: flex:1 shares the width */
    lays("<div style='display:flex;width:300px'><div id=a style='flex:1'></div><div id=b style='flex:2'></div><div id=x style='width:30px'></div></div>");
    BOX("a", 0, 0, 90, 0); BOX("b", 90, 0, 180, 0); BOX("x", 270, 0, 30, 0);
}

/* --- the automatic minimum size of column items --- */
static void test_l121_auto_minimum(void) {
    /* auto-height column, 0 bases: the items keep their content height */
    lays("<div id=c style='display:flex;flex-direction:column'><div id=a style='flex:1'>hello</div><div id=b style='flex-basis:0'>world</div></div>");
    BOX("c", 0, 0, 640, 20); BOX("a", 0, 0, 640, 10); BOX("b", 0, 10, 640, 10);
    /* min-height: 0 and scroll containers have no automatic minimum */
    lays("<div id=c style='display:flex;flex-direction:column'><div id=a style='flex:1;min-height:0'>hello</div><div id=b style='flex:1;overflow:hidden'>world</div><div id=d style='flex:1;min-height:5px'>x</div></div>");
    BOX("c", 0, 0, 640, 5); BOX("a", 0, 0, 640, 0); BOX("b", 0, 0, 640, 0); BOX("d", 0, 0, 640, 5);
    lays("<div id=c style='display:flex;flex-direction:column'><div id=a style='flex:1;min-height:20%'>hello</div><div id=b style='flex:1;min-height:calc(2px + 1px)'>world</div></div>");
    BOX("a", 0, 0, 640, 0); BOX("b", 0, 0, 640, 3);
    /* min-height auto after a length is the automatic minimum again */
    lays("<div id=c style='display:flex;flex-direction:column'><div id=a style='flex:1;min-height:0;min-height:auto'>hello</div><div id=b style='flex:1;min-height:calc(1px + 1px);min-height:auto'>x</div></div>");
    BOX("a", 0, 0, 640, 10); BOX("b", 0, 10, 640, 10);
    EQ_MSG(0, (int)g_doc.ndiag, "min-height auto accepted");
    /* and after a percentage: the 30% (30 px) minimum is gone */
    lays("<div style='display:flex;flex-direction:column;height:100px'><div id=a style='flex:0 1 0;min-height:30%;min-height:auto'>x<br>y</div><div id=b style='flex:0 1 0'>z</div></div>");
    BOX("a", 0, 0, 640, 20); BOX("b", 0, 20, 640, 10);
    /* growing: a grower held at its content is frozen, the other takes the rest */
    lays("<div style='display:flex;flex-direction:column;height:300px'><div id=a style='flex:1'><div style='height:200px'></div></div><div id=b style='flex:1'>u</div></div>");
    BOX("a", 0, 0, 640, 200); BOX("b", 0, 200, 640, 100);
    lays("<div style='display:flex;flex-direction:column;height:300px'><div id=a style='flex:1'>t</div><div id=b style='flex:1'>u<br>u<br>u</div></div>");
    BOX("a", 0, 0, 640, 150); BOX("b", 0, 150, 640, 150);
    /* two frozen in turn */
    lays("<div style='display:flex;flex-direction:column;height:300px'><div id=a style='flex:1'><div style='height:140px'></div></div>"
         "<div id=b style='flex:2'><div style='height:150px'></div></div><div id=e style='flex:7'></div></div>");
    BOX("a", 0, 0, 640, 140); BOX("b", 0, 140, 640, 150); BOX("e", 0, 290, 640, 10);
    /* a frozen item leaves the weights: the 100 px left go 1:2 (33 + the rounding px, 66) */
    lays("<div style='display:flex;flex-direction:column;height:300px'><div id=f style='flex:1'><div style='height:200px'></div></div>"
         "<div id=a style='flex:1'></div><div id=b style='flex:2'></div></div>");
    BOX("f", 0, 0, 640, 200); BOX("a", 0, 200, 640, 34); BOX("b", 0, 234, 640, 66);
    /* zero bases take the rounding px too */
    lays("<div style='display:flex;flex-direction:column;height:100px'><div id=a style='flex:1'></div><div id=b style='flex:1'></div><div id=d style='flex:1'></div></div>");
    BOX("a", 0, 0, 640, 34); BOX("b", 0, 34, 640, 33); BOX("d", 0, 67, 640, 33);
    /* but not an item already at its max-height */
    lays("<div style='display:flex;flex-direction:column;height:101px'><div id=a style='flex:1;max-height:10px'></div><div id=b style='flex:1'></div><div id=d style='flex:1'></div></div>");
    BOX("a", 0, 0, 640, 10); BOX("b", 0, 10, 640, 46); BOX("d", 0, 56, 640, 45);
    /* padding and borders count in the minimum */
    lays("<div id=c style='display:flex;flex-direction:column'><div id=a style='flex:1;padding:3px;border:2px solid red'>x</div></div>");
    BOX("c", 0, 0, 640, 20); BOX("a", 0, 0, 640, 20);
    /* shrinking stops at the minimum: min(height, content) */
    lays("<div style='display:flex;flex-direction:column;height:50px'><div id=a style='height:40px'>t<br>t<br>t<br>t<br>t<br>t</div><div id=b style='height:40px'>u</div></div>");
    BOX("a", 0, 0, 640, 40); BOX("b", 0, 40, 640, 10);
    /* a specified minimum still holds in the shrink: 35 px, the other takes the rest */
    lays("<div style='display:flex;flex-direction:column;height:50px'><div id=a style='height:40px;min-height:35px'>t</div><div id=b style='height:40px;min-height:0'>u</div></div>");
    BOX("a", 0, 0, 640, 35); BOX("b", 0, 35, 640, 15);
    /* the last shrink px skips an item already at its minimum */
    lays("<div style='display:flex;flex-direction:column;height:79px'><div id=a style='height:40px;min-height:40px'></div><div id=b style='height:20px'></div><div id=d style='height:20px'></div></div>");
    BOX("a", 0, 0, 640, 40); BOX("b", 0, 40, 640, 19); BOX("d", 0, 59, 640, 20);
    /* a definite height below the content caps the minimum; so does max-height */
    lays("<div style='display:flex;flex-direction:column;height:300px'><div id=a style='flex-grow:1;flex-basis:0'>x</div><div id=b style='flex-grow:1;flex-basis:0;height:200px'>y</div><div id=d style='flex-grow:1;flex-basis:0'>z</div></div>");
    BOX("a", 0, 0, 640, 100); BOX("b", 0, 100, 640, 100); BOX("d", 0, 200, 640, 100);
    lays("<div id=c style='display:flex;flex-direction:column'><div id=a style='flex:1;max-height:4px'>hello</div><div id=b style='flex:1;height:6px'>x<br>y</div></div>");
    BOX("a", 0, 0, 640, 4); BOX("b", 0, 4, 640, 6);
    /* the capped minimum does not freeze a max-height item above its maximum */
    lays("<div style='display:flex;flex-direction:column;height:100px'><div id=a style='flex:1;max-height:4px'>hello</div><div id=b style='flex:1'>x</div></div>");
    BOX("a", 0, 0, 640, 4); BOX("b", 0, 4, 640, 96);
    /* the minimum stops shrinking past the container: overflow, one diagnostic */
    lays("<div style='display:flex;flex-direction:column;height:15px'><div id=a>t</div><div id=b>u</div></div>");
    BOX("a", 0, 0, 640, 10); BOX("b", 0, 10, 640, 10);
    EQ_MSG(1, diag_count("flex minimum prevents fit"), "overflow diag");
    /* rows: min-width 0 turns the automatic minimum off (Lot 122 gives rows one) */
    lays("<div style='display:flex;width:100px'><div id=a style='flex-grow:1;flex-basis:0;min-width:0'>aaaaaaaaaaaaaaaaaaaa</div><div id=b style='flex-grow:1;flex-basis:0;min-width:0'>b</div></div>");
    BOX("a", 0, 0, 50, 10); BOX("b", 50, 0, 50, 10);
    /* a row with flex bases keeps the sizing path */
    lays("<div style='display:flex'><div id=a style='flex-basis:50px'>x</div><div id=b style='flex-basis:60px'>y</div></div>");
    BOX("a", 0, 0, 50, 10); BOX("b", 50, 0, 60, 10);
}

/* --- max-height none --- */
static void test_l121_height_keywords(void) {
    lays("<div id=a style='max-height:5px;max-height:none'>x<br>y</div><div id=b style='max-height:calc(1px + 2px);max-height:none'>x</div>"
         "<div id=c style='min-height:30px;min-height:auto'>x</div><div id=d style='max-height:auto'>x</div><div id=e style='min-height:none'>x</div>");
    BOX("a", 0, 0, 640, 20); BOX("b", 0, 20, 640, 10); BOX("c", 0, 30, 640, 10);
    EQ_MSG(2, diag_count("height limit refused"), "max-height auto and min-height none refused");
    /* a refused min-height keeps an earlier math one, percentage included */
    lays("<div style='height:100px'><div id=a style='min-height:calc(10% + 0px);min-height:bogus'></div></div>");
    BOX("a", 0, 0, 640, 10);
    EQ_MSG(1, diag_count("height limit refused"), "bogus refused");
}

int main(void) {
    unity_init();
    RUN_TEST(test_l121_flex_shorthand);
    RUN_TEST(test_l121_flex_refused);
    RUN_TEST(test_l121_flex_flow);
    RUN_TEST(test_l121_column_grow);
    RUN_TEST(test_l121_auto_minimum);
    RUN_TEST(test_l121_height_keywords);
    unity_print_results();
    unity_cleanup();
    return unity_stats.tests_failed == 0 ? 0 : 1;
}
