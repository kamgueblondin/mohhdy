/* Lot 125 (US-031): true alignment, the default when no safe keyword is
 * given (CSS Box Alignment 4.4): a flex item larger than its line overflows
 * both sides under center and the start side under end / flex-end, and
 * justify-content does the same on the main axis. start / end follow the
 * writing mode while flex-start / flex-end follow the flex axes (5.1, 5.2):
 * they differ in row-reverse, column-reverse and wrap-reverse. A grid's
 * align-content normal behaves as stretch, so a grid stretched as a flex
 * item stretches its auto rows and their children's percentages see the
 * stretched row. Values follow docs/lot125_plan.md (8 px glyphs, 10 px
 * lines). */
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

/* --- cross axis: an item larger than its line overflows both sides --- */
static void test_l125_true_cross(void) {
    /* center: 60 in a 40 px line, 10 above and 10 below (was y 0) */
    lays("<div style='display:flex;width:100px;height:40px;align-items:center'><div id=a style='width:20px;height:60px'></div><div id=b style='width:20px;height:10px'></div></div>");
    BOX("a", 0, -10, 20, 60); BOX("b", 20, 15, 20, 10);
    EQ_MSG(0, (int)g_doc.ndiag, "no diag");
    /* an odd overflow: the extra pixel goes past the end edge */
    lays("<div style='display:flex;width:100px;height:40px'><div id=a style='width:20px;height:61px;align-self:center'></div></div>");
    BOX("a", 0, -10, 20, 61);
    /* end and flex-end: the whole overflow goes past the start edge */
    lays("<div style='display:flex;width:100px;height:40px;align-items:end'><div id=a style='width:20px;height:60px'></div></div>");
    BOX("a", 0, -20, 20, 60);
    lays("<div style='display:flex;width:100px;height:40px;align-items:flex-end'><div id=a style='width:20px;height:60px'></div></div>");
    BOX("a", 0, -20, 20, 60);
    /* start and flex-start stay on the start edge */
    lays("<div style='display:flex;width:100px;height:40px;align-items:start'><div id=a style='width:20px;height:60px'></div><div id=b style='width:20px;height:60px;align-self:flex-start'></div></div>");
    BOX("a", 0, 0, 20, 60); BOX("b", 20, 0, 20, 60);
    /* a column: the cross axis is horizontal */
    lays("<div style='display:flex;flex-direction:column;width:40px;height:100px;align-items:center'><div id=a style='width:60px;height:20px'></div><div id=b style='width:60px;height:20px;align-self:end'></div></div>");
    BOX("a", -10, 0, 60, 20); BOX("b", -20, 20, 60, 20);
    /* a column item under flex-end or end takes its fit-content width */
    lays("<div style='display:flex;flex-direction:column;width:100px'><div id=a style='align-self:flex-end'>ab</div><div id=b style='align-self:end'>ab</div></div>");
    BOX("a", 84, 0, 16, 10); BOX("b", 84, 10, 16, 10);
    /* auto margins keep the overflow at the start (they absorb no negative space) */
    lays("<div style='display:flex;width:100px;height:40px'><div id=a style='width:20px;height:60px;margin:auto 0'></div></div>");
    BOX("a", 0, 0, 20, 60);
    /* nested: the overflowing item inside a padded container */
    lays("<div style='display:flex;width:100px;height:40px;padding:5px;align-items:center'><div id=a style='width:20px;height:50px'></div></div>");
    BOX("a", 5, 0, 20, 50);
}

/* --- main axis: justify-content overflows the same way --- */
static void test_l125_true_main(void) {
    /* center: 160 px of items in 100 px, 30 past each side (were 0 / 80) */
    lays("<div style='display:flex;width:100px;justify-content:center'><div id=a style='flex:0 0 80px;height:10px'></div><div id=b style='flex:0 0 80px;height:10px'></div></div>");
    BOX("a", -30, 0, 80, 10); BOX("b", 50, 0, 80, 10);
    EQ_MSG(1, diag_count("flex minimum prevents fit"), "overflow diag");
    /* end and flex-end: the line ends on the end edge */
    lays("<div style='display:flex;width:100px;justify-content:flex-end'><div id=a style='flex:0 0 80px;height:10px'></div><div id=b style='flex:0 0 80px;height:10px'></div></div>");
    BOX("a", -60, 0, 80, 10); BOX("b", 20, 0, 80, 10);
    /* the distributed values fall back to the start (their own fallback is out of theme) */
    lays("<div style='display:flex;width:100px;justify-content:space-around'><div id=a style='flex:0 0 80px;height:10px'></div><div id=b style='flex:0 0 80px;height:10px'></div></div>");
    BOX("a", 0, 0, 80, 10); BOX("b", 80, 0, 80, 10);
    /* a column of a definite height */
    lays("<div style='display:flex;flex-direction:column;width:40px;height:40px;justify-content:center'><div id=a style='flex:0 0 30px;width:20px'></div><div id=b style='flex:0 0 30px;width:20px'></div></div>");
    BOX("a", 0, -10, 20, 30); BOX("b", 0, 20, 20, 30);
    /* rtl mirrors the overflow: end is the left edge */
    lays("<div style='direction:rtl;display:flex;width:100px;justify-content:end'><div id=a style='flex:0 0 80px;height:10px'></div><div id=b style='flex:0 0 80px;height:10px'></div></div>");
    BOX("a", 80, 0, 80, 10); BOX("b", 0, 0, 80, 10);
    /* a line that fits keeps its packing */
    lays("<div style='display:flex;width:100px;justify-content:center'><div id=a style='flex:0 0 30px;height:10px'></div><div id=b style='flex:0 0 30px;height:10px'></div></div>");
    BOX("a", 20, 0, 30, 10); BOX("b", 50, 0, 30, 10);
}

/* --- start / end follow the writing mode, flex-start / flex-end the axes --- */
static void test_l125_start_vs_flex_start(void) {
    /* row-reverse: start packs to the left, flex-start to the right */
    lays("<div style='display:flex;flex-direction:row-reverse;width:100px;justify-content:start'><div id=a style='width:20px;height:10px'></div><div id=b style='width:20px;height:10px'></div></div>");
    BOX("a", 20, 0, 20, 10); BOX("b", 0, 0, 20, 10);
    lays("<div style='display:flex;flex-direction:row-reverse;width:100px;justify-content:flex-start'><div id=a style='width:20px;height:10px'></div><div id=b style='width:20px;height:10px'></div></div>");
    BOX("a", 80, 0, 20, 10); BOX("b", 60, 0, 20, 10);
    lays("<div style='display:flex;flex-direction:row-reverse;width:100px;justify-content:end'><div id=a style='width:20px;height:10px'></div><div id=b style='width:20px;height:10px'></div></div>");
    BOX("a", 80, 0, 20, 10); BOX("b", 60, 0, 20, 10);
    lays("<div style='display:flex;flex-direction:row-reverse;width:100px;justify-content:flex-end'><div id=a style='width:20px;height:10px'></div><div id=b style='width:20px;height:10px'></div></div>");
    BOX("a", 20, 0, 20, 10); BOX("b", 0, 0, 20, 10);
    /* rtl row-reverse: start is the right edge, flex-start the left one */
    lays("<div style='direction:rtl;display:flex;flex-direction:row-reverse;width:120px;justify-content:start'><div id=a style='width:20px;height:10px'></div><div id=b style='width:20px;height:10px'></div></div>");
    BOX("a", 80, 0, 20, 10); BOX("b", 100, 0, 20, 10);
    lays("<div style='direction:rtl;display:flex;flex-direction:row-reverse;width:120px;justify-content:flex-start'><div id=a style='width:20px;height:10px'></div><div id=b style='width:20px;height:10px'></div></div>");
    BOX("a", 0, 0, 20, 10); BOX("b", 20, 0, 20, 10);
    /* column-reverse: start is the top */
    lays("<div style='display:flex;flex-direction:column-reverse;width:40px;height:100px;justify-content:start'><div id=a style='width:20px;height:20px'></div><div id=b style='width:20px;height:20px'></div></div>");
    BOX("a", 0, 20, 20, 20); BOX("b", 0, 0, 20, 20);
    lays("<div style='display:flex;flex-direction:column-reverse;width:40px;height:100px;justify-content:flex-start'><div id=a style='width:20px;height:20px'></div><div id=b style='width:20px;height:20px'></div></div>");
    BOX("a", 0, 80, 20, 20); BOX("b", 0, 60, 20, 20);
    /* overflow in row-reverse under end: past the left (start) edge */
    lays("<div style='display:flex;flex-direction:row-reverse;width:100px;justify-content:end'><div id=a style='flex:0 0 80px;height:10px'></div><div id=b style='flex:0 0 80px;height:10px'></div></div>");
    BOX("a", 20, 0, 80, 10); BOX("b", -60, 0, 80, 10);
}

/* --- wrap-reverse: the cross axis flips for flex-start, not for start --- */
static void test_l125_wrap_reverse(void) {
    /* items in a single stretched line */
    lays("<div style='display:flex;flex-wrap:wrap-reverse;width:100px;height:40px;align-items:start'><div id=a style='width:20px;height:10px'></div><div id=b style='width:20px;height:20px;align-self:end'></div></div>");
    BOX("a", 0, 0, 20, 10); BOX("b", 20, 20, 20, 20);
    lays("<div style='display:flex;flex-wrap:wrap-reverse;width:100px;height:40px;align-items:flex-start'><div id=a style='width:20px;height:10px'></div><div id=b style='width:20px;height:20px;align-self:flex-end'></div></div>");
    BOX("a", 0, 30, 20, 10); BOX("b", 20, 0, 20, 20);
    /* lines: align-content start packs them at the top, flex-start at the bottom */
    lays("<div style='display:flex;flex-wrap:wrap-reverse;width:50px;height:100px;align-content:start'><div id=a style='width:30px;height:10px'></div><div id=b style='width:30px;height:20px'></div></div>");
    BOX("a", 0, 20, 30, 10); BOX("b", 0, 0, 30, 20);
    lays("<div style='display:flex;flex-wrap:wrap-reverse;width:50px;height:100px;align-content:flex-start'><div id=a style='width:30px;height:10px'></div><div id=b style='width:30px;height:20px'></div></div>");
    BOX("a", 0, 90, 30, 10); BOX("b", 0, 70, 30, 20);
    lays("<div style='display:flex;flex-wrap:wrap-reverse;width:50px;height:100px;align-content:end'><div id=a style='width:30px;height:10px'></div><div id=b style='width:30px;height:20px'></div></div>");
    BOX("a", 0, 90, 30, 10); BOX("b", 0, 70, 30, 20);
    lays("<div style='display:flex;flex-wrap:wrap-reverse;width:50px;height:100px;align-content:flex-end'><div id=a style='width:30px;height:10px'></div><div id=b style='width:30px;height:20px'></div></div>");
    BOX("a", 0, 20, 30, 10); BOX("b", 0, 0, 30, 20);
    /* plain wrap: start and flex-start agree */
    lays("<div style='display:flex;flex-wrap:wrap;width:50px;height:100px;align-content:start'><div id=a style='width:30px;height:10px'></div><div id=b style='width:30px;height:20px'></div></div>");
    BOX("a", 0, 0, 30, 10); BOX("b", 0, 10, 30, 20);
}

/* --- a grid stretches its auto rows (align-content normal) --- */
static void test_l125_grid_rows(void) {
    /* a grid stretched by a flex line: its row is 40, the child's 50% is 20 (was 10) */
    lays("<div style='display:flex;width:100px;height:40px'><div id=g style='display:grid;width:50px'><div id=c style='height:50%'></div></div></div>");
    BOX("g", 0, 0, 50, 40); BOX("c", 0, 0, 50, 20);
    /* two auto rows share a definite height (b was at 10) */
    lays("<div id=g style='display:grid;width:50px;height:100px'><div id=a style='height:10px'></div><div id=b style='height:10px'></div></div>");
    BOX("a", 0, 0, 50, 10); BOX("b", 0, 50, 50, 10);
    /* fixed rows keep their size; only the auto one takes the rest */
    lays("<div style='display:grid;width:50px;height:100px;grid-template-rows:20px auto'><div id=a style='height:10px'></div><div id=b style='height:10px'></div><div id=c style='height:50%'></div></div>");
    BOX("b", 0, 20, 50, 10);
    /* align-content start keeps the packing */
    lays("<div style='display:grid;width:50px;height:100px;align-content:start'><div id=a style='height:10px'></div><div id=b style='height:10px'></div></div>");
    BOX("b", 0, 10, 50, 10);
    /* flex-start / flex-end in a grid behave as start / end */
    lays("<div style='display:grid;width:50px;height:100px;align-content:flex-end'><div id=a style='height:10px'></div><div id=b style='height:10px'></div></div>");
    BOX("a", 0, 80, 50, 10); BOX("b", 0, 90, 50, 10);
    lays("<div style='display:grid;width:100px;grid-template-columns:50px;justify-items:flex-end'><div id=a>a</div></div>");
    BOX("a", 42, 0, 8, 10);
    /* an indefinite height has nothing to share */
    lays("<div style='display:grid;width:50px'><div id=a style='height:10px'></div><div id=b style='height:10px'></div></div>");
    BOX("b", 0, 10, 50, 10);
    /* masonry keeps its packing: the grid rows of masonry columns are not stretched */
    lays("<div style='display:grid;width:60px;height:100px;grid-template-columns:masonry;grid-template-rows:auto auto'><div id=a style='width:10px;height:10px'></div><div id=b style='width:10px;height:20px'></div><div id=c style='width:10px;height:10px'></div></div>");
    BOX("a", 0, 0, 10, 10); BOX("b", 0, 10, 10, 20); BOX("c", 10, 0, 10, 10);
    /* a row subgrid follows the stretched rows of its parent */
    lays("<div style='display:grid;height:100px;width:60px;grid-template-rows:auto auto auto'><div id=s style='display:grid;grid-row:1/3;grid-template-rows:subgrid'><div id=a style='height:10px'></div><div id=b style='height:10px'></div></div><div id=c style='height:10px'></div></div>");
    BOX("s", 0, 0, 60, 67); BOX("b", 0, 34, 60, 10); BOX("c", 0, 67, 60, 10);
}

int main(void) {
    unity_init();
    RUN_TEST(test_l125_true_cross);
    RUN_TEST(test_l125_true_main);
    RUN_TEST(test_l125_start_vs_flex_start);
    RUN_TEST(test_l125_wrap_reverse);
    RUN_TEST(test_l125_grid_rows);
    unity_print_results();
    unity_cleanup();
    return unity_stats.tests_failed == 0 ? 0 : 1;
}
