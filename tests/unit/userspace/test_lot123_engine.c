/* Lot 123 (US-031): every flex row on the flex algorithm. Plain rows (no
 * flex property, no modern alignment) lose their equal slots: auto bases
 * are the content width, items shrink by 1 down to their automatic
 * minimum. Items stretch to their line by default (align-items normal,
 * CSS Flexbox 8.3, 9.4 step 11): row items to its height, an indefinite
 * percentage height counting as auto, column items to its width; a single
 * line takes a definite container cross size (9.4 step 15). normal is
 * accepted on the container alignment properties. Values follow
 * docs/lot123_plan.md (8 px glyphs, 10 px lines). */
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

/* --- plain rows take the flex algorithm --- */
static void test_l123_plain_bases(void) {
    /* content widths, not equal slots (were 150 / 150) */
    lays("<div style='display:flex;width:300px'><div id=a>aaaa</div><div id=b>b</div></div>");
    BOX("a", 0, 0, 32, 10); BOX("b", 32, 0, 8, 10);
    EQ_MSG(0, (int)g_doc.ndiag, "no diag");
    /* empty items are 0 wide (were 320 each) */
    lays("<div style='display:flex'><div id=a style='height:10px'></div><div id=b style='height:10px'></div></div>");
    BOX("a", 0, 0, 0, 10); BOX("b", 0, 0, 0, 10);
    /* gaps sit between content widths */
    lays("<div style='display:flex;width:100px;gap:10px'><div id=a>aa</div><div id=b>bb</div></div>");
    BOX("a", 0, 0, 16, 10); BOX("b", 26, 0, 16, 10);
    /* a nested plain row is as wide as its items */
    lays("<div style='display:flex;width:200px'><div id=a style='display:flex'><div id=c>c</div><div id=d>dd</div></div><div id=b>b</div></div>");
    BOX("a", 0, 0, 24, 10); BOX("c", 0, 0, 8, 10); BOX("d", 8, 0, 16, 10); BOX("b", 24, 0, 8, 10);
    EQ_MSG(2, (int)g_doc.perf.flex_lines, "two lines");
}

static void test_l123_plain_shrink(void) {
    /* a wide item shrinks to its row (stayed 150) */
    lays("<div style='display:flex;width:100px'><div id=a style='width:150px'>x</div></div>");
    BOX("a", 0, 0, 100, 10);
    /* scaled shares: 80 and 60 lose 23 and 17 */
    lays("<div style='display:flex;width:100px'><div id=a style='width:80px'>x</div><div id=b style='width:60px'>y</div></div>");
    BOX("a", 0, 0, 57, 10); BOX("b", 57, 0, 43, 10);
    /* words wrap down to the automatic minimum */
    lays("<div style='display:flex;width:100px'><div id=a>aa aa aa aa</div><div id=b>bb bb bb bb</div></div>");
    BOX("a", 0, 0, 50, 20); BOX("b", 50, 0, 50, 20);
    /* unbreakable words overflow, diagnosed */
    lays("<div style='display:flex;width:100px'><div id=a>aaaaaaaa</div><div id=b>bbbbbbbb</div></div>");
    BOX("a", 0, 0, 64, 10); BOX("b", 64, 0, 64, 10);
    EQ_MSG(1, diag_count("flex minimum prevents fit"), "overflow diag");
}

/* --- align-items normal stretches row items --- */
static void test_l123_default_stretch(void) {
    /* auto heights take the line (40); a height, a max-height, align-self,
     * auto margins and an explicit min-height keep theirs */
    lays("<div style='display:flex;width:300px'><div id=a>a</div><div id=b>b<br>b<br>b</div><div id=c style='height:12px'>c</div><div id=d style='max-height:15px'>d</div><div id=e style='align-self:flex-start'>e</div><div id=f style='margin:auto 0'>f</div><div id=h style='min-height:40px'>h</div></div>");
    BOX("a", 0, 0, 8, 40); BOX("b", 8, 0, 8, 40); BOX("c", 16, 0, 8, 12); BOX("d", 24, 0, 8, 15);
    BOX("e", 32, 0, 8, 10); BOX("f", 40, 15, 8, 10); BOX("h", 48, 0, 8, 40);
    /* a definite container height is the line; edges count in the outer size */
    lays("<div id=r style='display:flex;width:300px;height:50px'><div id=a>a</div><div id=b style='padding:2px;border:1px solid red;margin:3px'>b</div></div>");
    BOX("r", 0, 0, 300, 50); BOX("a", 0, 0, 8, 50); BOX("b", 11, 3, 14, 44);
    /* every line of a wrapping row stretches its items */
    lays("<div style='display:flex;flex-wrap:wrap;width:40px'><div id=a>aaaa</div><div id=b>b<br>b</div><div id=c>c</div></div>");
    BOX("a", 0, 0, 32, 20); BOX("b", 32, 0, 8, 20); BOX("c", 0, 20, 8, 10);
    /* a percentage height of an indefinite container is auto: stretched,
     * explicit stretch too; against a definite height it resolves */
    lays("<div style='display:flex;width:300px'><div id=a>a<br>a</div><div id=g style='height:50%'>g</div><div id=k style='height:50%;align-self:stretch'>k</div></div>");
    BOX("a", 0, 0, 8, 20); BOX("g", 8, 0, 8, 20); BOX("k", 16, 0, 8, 20);
    lays("<div style='display:flex;width:300px;height:60px'><div id=a>a</div><div id=g style='height:50%'>g</div></div>");
    BOX("a", 0, 0, 8, 60); BOX("g", 8, 0, 8, 30);
    /* flex-start keeps the content heights */
    lays("<div style='display:flex;width:300px;align-items:flex-start'><div id=a>a</div><div id=b>b<br>b</div></div>");
    BOX("a", 0, 0, 8, 10); BOX("b", 8, 0, 8, 20);
    /* a column item fills the width as before */
    lays("<div style='display:flex;flex-direction:column;width:100px'><div id=a>a</div><div id=b style='width:20px'>b</div></div>");
    BOX("a", 0, 0, 100, 10); BOX("b", 0, 10, 20, 10);
    /* a wrapping column stretches its items to their line (was 8 wide);
     * Lot 124: align-content: flex-start keeps the lines at their content
     * width (normal now stretches the lines to the container) */
    lays("<div style='display:flex;flex-direction:column;flex-wrap:wrap;align-content:flex-start;height:20px;width:100px'><div id=a>a</div><div id=b style='width:30px'>b</div><div id=c>c</div><div id=e style='align-self:flex-start'>e</div></div>");
    BOX("a", 0, 0, 30, 10); BOX("b", 0, 10, 30, 10); BOX("c", 30, 0, 8, 10); BOX("e", 30, 10, 8, 10);
}

/* --- a single line is as tall (wide) as a definite container (9.4 step 15) --- */
static void test_l123_single_line(void) {
    /* a taller item overflows the 20 px line, the others stretch to 20 (not 40) */
    lays("<div id=r style='display:flex;width:300px;height:20px'><div id=a>a</div><div id=b style='height:40px'>b</div></div><div id=z>z</div>");
    BOX("r", 0, 0, 300, 20); BOX("a", 0, 0, 8, 20); BOX("b", 8, 0, 8, 40); BOX("z", 0, 20, 640, 10);
    /* a wider column item overflows the 100 px line, the others stretch to 100 */
    lays("<div style='display:flex;flex-direction:column;width:100px'><div id=a style='max-width:30px'>a</div><div id=b style='min-width:150px'>b</div><div id=c style='padding:0 5px;margin:0 7px'>c</div></div>");
    BOX("a", 0, 0, 30, 10); BOX("b", 0, 10, 150, 10); BOX("c", 7, 20, 86, 10);
    /* a definite height of 0 is a 0 high line */
    lays("<div id=r style='display:flex;width:100px;height:0'><div id=a>a</div></div><div id=z>z</div>");
    BOX("r", 0, 0, 100, 0); BOX("a", 0, 0, 8, 0); BOX("z", 0, 0, 640, 10);
}

/* --- normal on the container alignment properties --- */
static void test_l123_normal_keyword(void) {
    lays("<div style='display:flex;width:300px;align-items:flex-start;align-items:normal'><div id=a>a</div><div id=b>b<br>b</div></div>");
    BOX("a", 0, 0, 8, 20); BOX("b", 8, 0, 8, 20);
    EQ_MSG(0, diag_count("alignment refused"), "align-items normal");
    lays("<div style='display:flex;width:300px;justify-content:center;justify-content:normal;align-content:normal;justify-items:normal'><div id=a>a</div></div>");
    BOX("a", 0, 0, 8, 10);
    EQ_MSG(0, diag_count("alignment refused"), "container normal");
    /* Lot 124: align-content normal now stretches the lines (Lot 123 pinned
     * the initial packing here, 16x10 at y 0 and 10: the gap this lot fills) */
    lays("<div style='display:flex;flex-wrap:wrap;width:20px;height:40px;align-content:normal'><div id=a>aa</div><div id=b>bb</div></div>");
    BOX("a", 0, 0, 16, 20); BOX("b", 0, 20, 16, 20);
    /* align-self normal would mean stretch, not auto: still refused */
    lays("<div style='display:flex;width:300px;align-items:flex-start'><div id=a style='align-self:normal'>a</div><div id=b>b<br>b</div><div id=c style='justify-self:normal'>c</div></div>");
    BOX("a", 0, 0, 8, 10);
    EQ_MSG(2, diag_count("alignment refused"), "self normal");
}

/* --- measured plain rows --- */
static void test_l123_measured_rows(void) {
    /* inside a column: 92 + 8 (bases 152 and 8, minimum 8), stretched to 20 */
    lays("<div style='display:flex;flex-direction:column;width:100px'><div id=r style='display:flex'><div id=a>aaaa aaaa aaaa aaaa</div><div id=b>b</div></div><div id=z>z</div></div>");
    BOX("r", 0, 0, 100, 20); BOX("a", 0, 0, 92, 20); BOX("b", 92, 0, 8, 20); BOX("z", 0, 20, 100, 10);
    /* in a float: the row is measured on its content */
    lays("<div style='width:200px'><div id=f style='float:left'><div id=r style='display:flex'><div id=a>aa</div><div id=b>b<br>b</div></div></div></div>");
    BOX("r", 0, 0, 24, 20); BOX("a", 0, 0, 16, 20); BOX("b", 16, 0, 8, 20);
}

int main(void) {
    unity_init();
    RUN_TEST(test_l123_plain_bases);
    RUN_TEST(test_l123_plain_shrink);
    RUN_TEST(test_l123_default_stretch);
    RUN_TEST(test_l123_single_line);
    RUN_TEST(test_l123_normal_keyword);
    RUN_TEST(test_l123_measured_rows);
    unity_print_results();
    unity_cleanup();
    return unity_stats.tests_failed == 0 ? 0 : 1;
}
