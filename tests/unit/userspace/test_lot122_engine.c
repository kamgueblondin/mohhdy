/* Lot 122 (US-031): row flex items. On the flex algorithm (any row with a
 * flex property, wrap or a modern alignment) an auto basis is the item's
 * max-content width, every item shrinks by the default factor 1, and a row
 * item with min-width auto keeps its min-content width as minimum (CSS
 * Flexbox 4.5, 7.2.3, 9.7). Plain rows keep their equal slots. Values
 * follow docs/lot122_plan.md (8 px glyphs, 10 px lines). */
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

/* --- auto bases are the content width --- */
static void test_l122_content_bases(void) {
    /* growers start from their content: 32 + 130, 8 + 130 */
    lays("<div style='display:flex;width:300px'><div id=a style='flex-grow:1'>aaaa</div><div id=b style='flex-grow:1'>b</div></div>");
    BOX("a", 0, 0, 162, 10); BOX("b", 162, 0, 138, 10);
    /* next to a flex-basis item, an auto item keeps its content (was 0) */
    lays("<div style='display:flex'><div id=a style='flex-basis:50px'>x</div><div id=b>y</div></div>");
    BOX("a", 0, 0, 50, 10); BOX("b", 50, 0, 8, 10);
    lays("<div style='display:flex'><div id=a style='flex-shrink:0'>x</div><div id=b>yy</div></div>");
    BOX("a", 0, 0, 8, 10); BOX("b", 8, 0, 16, 10);
    /* a declared width is still the basis */
    lays("<div style='display:flex'><div id=a style='flex-shrink:0;width:30px'>x</div><div id=b>y</div></div>");
    BOX("a", 0, 0, 30, 10); BOX("b", 30, 0, 8, 10);
    /* wrapping breaks on the content bases, then each line grows */
    lays("<div style='display:flex;flex-wrap:wrap;width:100px'><div id=a style='flex-grow:1'>aaaaaaaa</div><div id=b style='flex-grow:1'>bbbbbbbb</div><div id=c style='flex-grow:1'>c</div></div>");
    BOX("a", 0, 0, 100, 10); BOX("b", 0, 10, 78, 10); BOX("c", 78, 10, 22, 10);
    EQ_MSG(0, (int)g_doc.ndiag, "no diag");
}

/* --- every item shrinks by the default factor 1 --- */
static void test_l122_default_shrink(void) {
    /* growers only: 88 + 88 in 100, both shrink to 50 on two lines */
    lays("<div style='display:flex;width:100px'><div id=a style='flex-grow:1'>aa aa aa aa</div><div id=b style='flex-grow:1'>bb bb bb bb</div></div>");
    BOX("a", 0, 0, 50, 20); BOX("b", 50, 0, 50, 20);
    /* shares follow the bases: 64 and 40 in 100 lose 2 + the rounding px, and 1 */
    lays("<div style='display:flex;width:100px'><div id=a style='flex-grow:1'>aa aa aa</div><div id=b style='flex-grow:1'>bb bb</div></div>");
    BOX("a", 0, 0, 61, 20); BOX("b", 61, 0, 39, 20);
    /* flex-shrink 0 holds */
    lays("<div style='display:flex;width:100px'><div id=a style='flex-grow:1;flex-shrink:0'>aa aa aa aa</div><div id=b style='flex-grow:1'>bb bb bb bb</div></div>");
    BOX("a", 0, 0, 88, 10); BOX("b", 88, 0, 16, 40);
    /* a wrapped line alone shrinks too */
    lays("<div style='display:flex;flex-wrap:wrap;width:100px'><div id=a style='width:150px'>x</div><div id=b style='width:20px'>y</div></div>");
    BOX("a", 0, 0, 100, 10); BOX("b", 0, 10, 20, 10);
}

/* --- automatic minimum of row items --- */
static void test_l122_auto_minimum(void) {
    /* unbreakable words stop the shrink: overflow, one diagnostic */
    lays("<div style='display:flex;width:100px'><div id=a style='flex-shrink:1'>aaaaaaaa</div><div id=b>bbbbbbbb</div></div>");
    BOX("a", 0, 0, 64, 10); BOX("b", 64, 0, 64, 10);
    EQ_MSG(1, diag_count("flex minimum prevents fit"), "overflow diag");
    /* the shrink goes on with the other item: 64 stays, 88 -> 36 */
    lays("<div style='display:flex;width:100px'><div id=a>aaaaaaaa</div><div id=b style='flex-shrink:1'>bb bb bb bb</div></div>");
    BOX("a", 0, 0, 64, 10); BOX("b", 64, 0, 36, 40);
    EQ_MSG(0, (int)g_doc.ndiag, "fits");
    /* zero bases: a grower held at its min-content width is frozen */
    lays("<div style='display:flex;width:100px'><div id=a style='flex:1 1 0'>aaaaaaaa</div><div id=b style='flex:1 1 0'>b</div><div id=c style='flex:1 1 0'>c</div></div>");
    BOX("a", 0, 0, 64, 10); BOX("b", 64, 0, 18, 10); BOX("c", 82, 0, 18, 10);
    lays("<div style='display:flex;width:100px'><div id=a style='flex:1 1 0'>aaaaaaaaaaaa</div><div id=b style='flex:1 1 0'>b</div></div>");
    BOX("a", 0, 0, 96, 10); BOX("b", 96, 0, 8, 10);
    /* a definite width below the content caps the minimum; so does max-width */
    lays("<div style='display:flex;width:100px'><div id=a style='flex:1 1 0'>a</div><div id=b style='flex:1 1 0;width:30px'>bbbbbbbbbbbbbbbbbb</div></div>");
    BOX("a", 0, 0, 50, 10); BOX("b", 50, 0, 50, 10);
    lays("<div style='display:flex;width:100px'><div id=a style='width:150px;flex-grow:0'>x</div></div>");
    BOX("a", 0, 0, 100, 10);
    lays("<div style='display:flex;width:100px'><div id=a style='max-width:20px'>aaaaaaaa</div><div id=b style='flex-shrink:1'>bbbbbbbbbb</div></div>");
    BOX("a", 0, 0, 20, 10); BOX("b", 20, 0, 80, 10);
    lays("<div style='display:flex;width:100px'><div id=a style='flex:1 1 0;width:20%'>aaaaaaaa</div><div id=b style='flex:1 1 0'>b</div></div>");
    BOX("a", 0, 0, 50, 10); BOX("b", 50, 0, 50, 10);
    lays("<div style='display:flex;width:100px'><div id=a style='flex:1 1 0;width:min-content'>aaa aaa</div><div id=b style='flex:3 1 0'>b</div></div>");
    BOX("a", 0, 0, 25, 20); BOX("b", 25, 0, 75, 10);
    lays("<div style='display:flex;width:100px'><div id=a style='flex:1 1 0;width:calc(10px + 5px)'>aaaaaaaa</div><div id=b style='flex:5 1 0'>b</div></div>");
    BOX("a", 0, 0, 17, 10); BOX("b", 17, 0, 83, 10);
    /* min-width 0, a length, a percentage, math, scroll containers: no automatic minimum */
    lays("<div style='display:flex;width:100px'><div id=a style='flex:1 1 0;min-width:0'>aaaaaaaa</div><div id=b style='flex:1 1 0;overflow:hidden'>bbbbbbbb</div></div>");
    BOX("a", 0, 0, 50, 10); BOX("b", 50, 0, 50, 10);
    /* a scroll container keeps its specified min-width in the shrink: 80 -> 60, 80 -> 40 */
    lays("<div style='display:flex;width:100px'><div id=a style='overflow:hidden;min-width:60px;width:80px;flex-shrink:1'>a</div><div id=b style='width:80px;min-width:0'>b</div></div>");
    BOX("a", 0, 0, 60, 10); BOX("b", 60, 0, 40, 10);
    /* width 0 is definite: no minimum left from the content */
    lays("<div style='display:flex;width:100px'><div id=a style='flex:1 1 0;width:0'>aaaaaaaa</div><div id=b style='flex:1 1 0'>b</div></div>");
    BOX("a", 0, 0, 50, 10); BOX("b", 50, 0, 50, 10);
    lays("<div style='display:flex;width:100px'><div id=a style='flex:1 1 0;min-width:10px'>aaaaaaaa</div><div id=b style='flex:1 1 0;min-width:10%'>bbbbbbbb</div><div id=c style='flex:1 1 0;min-width:calc(5px + 5px)'>cccccccc</div></div>");
    BOX("a", 0, 0, 34, 10); BOX("b", 34, 0, 33, 10); BOX("c", 67, 0, 33, 10);
    /* min-width auto after a length is the automatic minimum again */
    lays("<div style='display:flex;width:100px'><div id=a style='flex:1 1 0;min-width:0;min-width:auto'>aaaaaaaa</div><div id=b style='flex:1 1 0'>b</div></div>");
    BOX("a", 0, 0, 64, 10); BOX("b", 64, 0, 36, 10);
    /* an intrinsic min-width keeps its own value and the shrink honours it */
    lays("<div style='display:flex;width:100px'><div id=a style='width:30px;min-width:min-content'>aaaaaaaaaaaa</div><div id=b style='min-width:0'>bbbb</div></div>");
    BOX("a", 0, 0, 96, 10); BOX("b", 96, 0, 4, 10);
    /* padding, borders and margins count in the minimum */
    lays("<div style='display:flex;width:100px'><div id=a style='flex:1 1 0;padding:0 5px'>aaaaaaaa</div><div id=b style='flex:1 1 0'>b</div></div>");
    BOX("a", 0, 0, 74, 10); BOX("b", 74, 0, 26, 10);
    lays("<div style='display:flex;width:100px'><div id=a style='flex:1 1 0;margin:0 3px;border:2px solid red'>aaaaaaaa</div><div id=b style='flex:1 1 0'>b</div></div>");
    BOX("a", 3, 0, 68, 14); BOX("b", 74, 0, 26, 10);
}

/* --- rows measured inside a column take the same sizes --- */
static void test_l122_measured_rows(void) {
    /* a modern row: 92 + 8 (bases 152 and 8, minimum 8), two lines */
    lays("<div style='display:flex;flex-direction:column;width:100px'><div id=r style='display:flex;justify-content:flex-start'><div id=a>aaaa aaaa aaaa aaaa</div><div id=b>b</div></div><div id=z>z</div></div>");
    BOX("r", 0, 0, 100, 20); BOX("a", 0, 0, 92, 20); BOX("b", 92, 0, 8, 10); BOX("z", 0, 20, 100, 10);
    /* a plain row keeps its equal slots, measured the same way: 50 + 50, four lines */
    lays("<div style='display:flex;flex-direction:column;width:100px'><div id=r style='display:flex'><div id=a>aaaa aaaa aaaa aaaa</div><div id=b>b</div></div><div id=z>z</div></div>");
    BOX("r", 0, 0, 100, 40); BOX("a", 0, 0, 50, 40); BOX("b", 50, 0, 50, 10); BOX("z", 0, 40, 100, 10);
    lays("<div style='display:flex;flex-direction:column;width:96px'><div id=r style='display:flex'><div id=a>aaa aaa</div><div id=b>b</div><div id=c>c</div></div><div id=z>z</div></div>");
    BOX("a", 0, 0, 32, 20); BOX("b", 32, 0, 32, 10); BOX("c", 64, 0, 32, 10); BOX("z", 0, 20, 96, 10);
    /* a grower row measured: 88 + 8 shrink to fit 64, the text wraps */
    lays("<div style='display:flex;flex-direction:column;width:64px'><div id=r style='display:flex'><div id=a style='flex-grow:1'>aa aa aa aa</div><div id=b style='flex-grow:1'>b</div></div><div id=z>z</div></div>");
    BOX("r", 0, 0, 64, 20); BOX("z", 0, 20, 64, 10);
}

int main(void) {
    unity_init();
    RUN_TEST(test_l122_content_bases);
    RUN_TEST(test_l122_default_shrink);
    RUN_TEST(test_l122_auto_minimum);
    RUN_TEST(test_l122_measured_rows);
    unity_print_results();
    unity_cleanup();
    return unity_stats.tests_failed == 0 ? 0 : 1;
}
