/* Lot 124 (US-031): align-content normal behaves as stretch in a flex
 * container (CSS Box Alignment 5.1, Flexbox 9.4 step 15): the free cross
 * space goes to the lines of a wrapping row, or of a wrapping column whose
 * width is definite. wrap-reverse swaps cross-start and cross-end for the
 * items inside a line too (5.2, 8.3). A stretched item's height is
 * definite for its children's percentages (9.8). place-items and
 * place-content accept normal. Values follow docs/lot124_plan.md (8 px
 * glyphs, 10 px lines). */
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

/* --- align-content normal stretches the lines of a wrapping row --- */
static void test_l124_stretch_rows(void) {
    /* two lines share the 80 px left in a 100 px container (were 10 + 10 at the top) */
    lays("<div style='display:flex;flex-wrap:wrap;width:100px;height:100px'><div id=a style='width:60px'>a</div><div id=b style='width:60px'>b</div></div>");
    BOX("a", 0, 0, 60, 50); BOX("b", 0, 50, 60, 50);
    EQ_MSG(0, (int)g_doc.ndiag, "no diag");
    /* the keyword says the same; an odd remainder goes to the last line */
    lays("<div style='display:flex;flex-wrap:wrap;width:100px;height:101px;align-content:normal'><div id=a style='width:60px'>a</div><div id=b style='width:60px'>b</div></div>");
    BOX("a", 0, 0, 60, 50); BOX("b", 0, 50, 60, 51);
    EQ_MSG(0, diag_count("alignment refused"), "align-content normal");
    /* lines of 55 and 45: a height, a max-height and flex-end inside them */
    lays("<div style='display:flex;flex-wrap:wrap;width:100px;height:100px'><div id=a style='width:60px;height:20px'>a</div><div id=b style='width:60px;max-height:30px'>b</div><div id=c style='width:30px;align-self:flex-end'>c</div></div>");
    BOX("a", 0, 0, 60, 20); BOX("b", 0, 55, 60, 30); BOX("c", 60, 90, 30, 10);
    /* flex-start items keep their height in the stretched lines */
    lays("<div style='display:flex;flex-wrap:wrap;width:100px;height:100px;align-items:flex-start'><div id=a style='width:60px'>a</div><div id=b style='width:60px'>b</div></div>");
    BOX("a", 0, 0, 60, 10); BOX("b", 0, 50, 60, 10);
    /* the row gap stays between the lines */
    lays("<div style='display:flex;flex-wrap:wrap;width:100px;height:100px;row-gap:10px'><div id=a style='width:60px'>a</div><div id=b style='width:60px'>b</div></div>");
    BOX("a", 0, 0, 60, 45); BOX("b", 0, 55, 60, 45);
    /* one line of a wrapping container takes the whole height (was 10) */
    lays("<div style='display:flex;flex-wrap:wrap;width:100px;height:40px'><div id=a>a</div><div id=b>b</div></div>");
    BOX("a", 0, 0, 8, 40); BOX("b", 8, 0, 8, 40);
    /* an indefinite height leaves nothing to share */
    lays("<div style='display:flex;flex-wrap:wrap;width:100px'><div id=a style='width:60px'>a</div><div id=b style='width:60px'>b</div></div>");
    BOX("a", 0, 0, 60, 10); BOX("b", 0, 10, 60, 10);
    /* an explicit flex-start packs the lines as before */
    lays("<div style='display:flex;flex-wrap:wrap;width:100px;height:100px;align-content:flex-start'><div id=a style='width:60px'>a</div><div id=b style='width:60px'>b</div></div>");
    BOX("a", 0, 0, 60, 10); BOX("b", 0, 10, 60, 10);
}

/* --- and the lines of a wrapping column (its width is definite) --- */
static void test_l124_stretch_columns(void) {
    /* lines of 8 + 8 share 84 px: 50 each (items were 8 wide) */
    lays("<div style='display:flex;flex-direction:column;flex-wrap:wrap;width:100px;height:20px'><div id=a>a</div><div id=b>b</div><div id=c>c</div></div>");
    BOX("a", 0, 0, 50, 10); BOX("b", 0, 10, 50, 10); BOX("c", 50, 0, 50, 10);
    /* a width stays, the column gap stays between the lines, margins count */
    lays("<div style='display:flex;flex-direction:column;flex-wrap:wrap;width:100px;height:20px;column-gap:10px'><div id=a style='width:20px'>a</div><div id=b>b</div><div id=c style='margin-left:5px'>c</div></div>");
    BOX("a", 0, 0, 20, 10); BOX("b", 0, 10, 48, 10); BOX("c", 63, 0, 37, 10);
}

/* --- wrap-reverse: cross-start is the far edge, also inside a line --- */
static void test_l124_wrap_reverse(void) {
    /* the stretched single line: a 10 px item sits at its bottom, an auto one fills it */
    lays("<div style='display:flex;flex-wrap:wrap-reverse;width:40px;height:30px'><div id=a style='width:20px;height:10px'></div><div id=b style='width:20px'>b</div></div>");
    BOX("a", 0, 20, 20, 10); BOX("b", 20, 0, 20, 30);
    /* two stretched lines (22 at the bottom, 18 at the top), items at their bottoms */
    lays("<div style='display:flex;flex-wrap:wrap-reverse;width:40px;height:40px'><div id=a style='width:30px;height:10px'></div><div id=b style='width:30px;height:5px'></div></div>");
    BOX("a", 0, 30, 30, 10); BOX("b", 0, 13, 30, 5);
    /* flex-end goes to the top; center does not move */
    lays("<div style='display:flex;flex-wrap:wrap-reverse;width:40px;height:30px;align-items:flex-end'><div id=a style='width:20px;height:10px'></div><div id=b style='width:20px;height:10px;align-self:center'></div></div>");
    BOX("a", 0, 0, 20, 10); BOX("b", 20, 10, 20, 10);
    /* auto margins are physical: not mirrored */
    lays("<div style='display:flex;flex-wrap:wrap-reverse;width:40px;height:30px'><div id=a style='width:20px;height:10px;margin-top:auto'></div><div id=b style='width:20px;height:10px;margin-bottom:auto'></div></div>");
    BOX("a", 0, 20, 20, 10); BOX("b", 20, 0, 20, 10);
    /* an item taller than its line overflows toward the top */
    lays("<div style='display:flex;flex-wrap:wrap-reverse;width:40px;height:30px;align-items:flex-start'><div id=a style='width:20px;height:40px'></div><div id=b style='width:20px;height:10px'></div></div>");
    BOX("a", 0, 0, 20, 40); BOX("b", 20, 30, 20, 10);
    /* a baseline group sits at the bottom of the line, a last-baseline group at its top */
    lays("<div style='display:flex;flex-wrap:wrap-reverse;width:60px;height:30px;align-items:baseline'><div id=a style='width:20px'>a</div><div id=b style='width:20px;font-size:20px'>b</div><div id=c style='width:20px;height:5px;align-self:flex-start'></div></div>");
    BOX("a", 0, 17, 20, 10); BOX("b", 20, 17, 20, 13); BOX("c", 40, 25, 20, 5);
    lays("<div style='display:flex;flex-wrap:wrap-reverse;width:60px;height:30px;align-items:last baseline'><div id=d style='width:20px'>d</div><div id=e style='width:20px;font-size:20px'>e</div></div>");
    BOX("d", 0, 0, 20, 10); BOX("e", 20, 0, 20, 13);
    /* column baselines keep their start fallback: at the left of their line */
    lays("<div style='display:flex;flex-direction:column;flex-wrap:wrap-reverse;width:100px;height:20px;align-items:baseline'><div id=a style='width:20px'>a</div><div id=b style='width:10px'>b</div><div id=c style='width:30px'>c</div></div>");
    BOX("a", 55, 0, 20, 10); BOX("b", 55, 10, 10, 10); BOX("c", 0, 0, 30, 10);
    /* a column: the first line on the right, a 20 px item at its right edge */
    lays("<div style='display:flex;flex-direction:column;flex-wrap:wrap-reverse;width:100px;height:20px'><div id=a style='width:20px'>a</div><div id=b>b</div><div id=c>c</div></div>");
    BOX("a", 80, 0, 20, 10); BOX("b", 44, 10, 56, 10); BOX("c", 0, 0, 44, 10);
}

/* --- percentages inside a stretched item (CSS Flexbox 9.8) --- */
static void test_l124_stretched_percent(void) {
    /* single line, indefinite container height: 50% of the stretched 40 */
    lays("<div style='display:flex;width:300px'><div id=a><div id=c style='height:50%'>c</div></div><div id=b>b<br>b<br>b<br>b</div></div>");
    BOX("a", 0, 0, 8, 40); BOX("c", 0, 0, 8, 20);
    /* in a line stretched by align-content normal: 50% of 50 */
    lays("<div style='display:flex;flex-wrap:wrap;width:100px;height:100px'><div id=a style='width:60px'><div id=c style='height:50%'>c</div></div><div id=b style='width:60px'>b</div></div>");
    BOX("a", 0, 0, 60, 50); BOX("c", 0, 0, 60, 25);
    /* a flex-start item is not stretched: the percentage stays auto */
    lays("<div style='display:flex;flex-wrap:wrap;width:100px;height:100px;align-items:flex-start'><div id=a style='width:60px'><div id=c style='height:50%'>c</div></div><div id=b style='width:60px'>b</div></div>");
    BOX("a", 0, 0, 60, 10); BOX("c", 0, 0, 60, 10);
}

/* --- normal on place-items and place-content --- */
static void test_l124_place_normal(void) {
    lays("<div id=r style='display:flex;width:300px;place-items:center;place-items:normal'><div id=a>a</div><div id=b>b<br>b</div></div>");
    BOX("a", 0, 0, 8, 20);
    EQ_MSG(0, diag_count("place alignment refused"), "place-items normal");
    EQ_MSG(WE_PACK_UNSET, el("r")->align_items, "align-items");
    EQ_MSG(WE_PACK_UNSET, el("r")->justify_items, "justify-items");
    /* normal center: align normal (stretch), justify center */
    lays("<div id=r style='display:flex;width:300px;place-items:normal center'><div id=a>a</div><div id=b>b<br>b</div></div>");
    BOX("a", 0, 0, 8, 20);
    EQ_MSG(WE_PACK_CENTER, el("r")->justify_items, "justify-items center");
    /* center normal: align center, justify normal */
    lays("<div id=r style='display:flex;width:300px;justify-items:end;place-items:center normal'><div id=a>a</div><div id=b>b<br>b</div></div>");
    BOX("a", 0, 5, 8, 10);
    EQ_MSG(WE_PACK_UNSET, el("r")->justify_items, "justify-items normal");
    EQ_MSG(0, diag_count("place alignment refused"), "two values");
    /* place-content normal: stretched lines */
    lays("<div style='display:flex;flex-wrap:wrap;width:100px;height:100px;place-content:center;place-content:normal'><div id=a style='width:60px'>a</div><div id=b style='width:60px'>b</div></div>");
    BOX("a", 0, 0, 60, 50); BOX("b", 0, 50, 60, 50);
    lays("<div style='display:flex;flex-wrap:wrap;width:100px;height:100px;place-content:normal center'><div id=a style='width:60px'>a</div><div id=b style='width:60px'>b</div></div>");
    BOX("a", 20, 0, 60, 50); BOX("b", 20, 50, 60, 50);
    lays("<div style='display:flex;flex-wrap:wrap;width:100px;height:100px;justify-content:flex-end;place-content:center normal'><div id=a style='width:60px'>a</div><div id=b style='width:20px'>b</div></div>");
    BOX("a", 0, 45, 60, 10); BOX("b", 60, 45, 20, 10);
    EQ_MSG(0, diag_count("place alignment refused"), "place-content normal");
    /* place-self normal would mean stretch, not auto: still refused */
    lays("<div style='display:flex;width:300px;align-items:flex-start'><div id=a style='place-self:normal'>a</div><div id=b style='place-self:auto normal'>b<br>b</div><div id=c style='place-self:normal auto'>c</div></div>");
    BOX("a", 0, 0, 8, 10);
    EQ_MSG(3, diag_count("place alignment refused"), "place-self normal");
}

int main(void) {
    unity_init();
    RUN_TEST(test_l124_stretch_rows);
    RUN_TEST(test_l124_stretch_columns);
    RUN_TEST(test_l124_wrap_reverse);
    RUN_TEST(test_l124_stretched_percent);
    RUN_TEST(test_l124_place_normal);
    unity_print_results();
    unity_cleanup();
    return unity_stats.tests_failed == 0 ? 0 : 1;
}
