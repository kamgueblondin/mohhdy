/* Lot 129 (US-031): the CSS-wide keywords inherit, initial, unset and
 * revert (CSS Cascade 4, 7.3) on the layout properties: flex, flex-grow,
 * flex-shrink, flex-basis, flex-flow, flex-direction, flex-wrap, order,
 * the alignment longhands and place-*, gap, row-gap, column-gap, width,
 * height, min-width, max-width, min-height, max-height and box-sizing.
 * inherit takes the parent's computed value (the initial value on the
 * root); none of these properties is inherited, so unset and revert are
 * the initial value. revert-layer stays refused. Values follow
 * docs/lot129_plan.md. */
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
    memset(&g_none, 0x5a, sizeof(g_none));
    g_none.h = -1; g_none.w = -1; g_none.x = -1; g_none.y = -1;
    return i >= 0 ? &g_doc.nodes[i] : &g_none;
}

static int diag_count(const char *t) {
    uint32_t i;
    int c = 0;
    for (i = 0; i < g_doc.ndiag; i++) if (strstr(g_doc.diag[i], t)) c++;
    return c;
}

static int ovf(const we_node_t *n, int prop) { return (n->align_overflow >> (2 * prop)) & 3; }

static int slot_same(const we_node_t *a, const we_node_t *b, int slot) {
    return memcmp(&a->length[slot], &b->length[slot], sizeof(a->length[slot])) == 0;
}

static int slot_empty(const we_node_t *a, int slot) {
    we_length_t zero;
    memset(&zero, 0, sizeof(zero));
    return memcmp(&a->length[slot], &zero, sizeof(zero)) == 0;
}

#define XY(id, X, Y, W, H) do { EQ_MSG(X, el(id)->x, id " x"); EQ_MSG(Y, el(id)->y, id " y"); \
    EQ_MSG(W, el(id)->w, id " w"); EQ_MSG(H, el(id)->h, id " h"); } while (0)

/* the fields of each group, child c against source s */
#define SAME_GROW(c, s) do { EQ_MSG(el(s)->flex_grow, el(c)->flex_grow, c " grow"); EQ_MSG(el(s)->grow_frac, el(c)->grow_frac, c " grow frac"); } while (0)
#define SAME_SHRINK(c, s) do { EQ_MSG(el(s)->flex_shrink, el(c)->flex_shrink, c " shrink"); EQ_MSG(el(s)->shrink_frac, el(c)->shrink_frac, c " shrink frac"); } while (0)
#define SAME_BASIS(c, s) do { EQ_MSG(el(s)->flex_basis, el(c)->flex_basis, c " basis"); EQ_MSG(el(s)->basis_pct, el(c)->basis_pct, c " basis pct"); \
    EQ_MSG(el(s)->basis_set, el(c)->basis_set, c " basis set"); EQ_MSG(el(s)->basis_content, el(c)->basis_content, c " basis content"); \
    EQ_MSG(el(s)->basis_kind, el(c)->basis_kind, c " basis kind"); EQ_MSG(1, slot_same(el(c), el(s), WE_LEN_BASIS), c " basis math"); } while (0)

/* --- inherit: each longhand copies its own fields, nothing else --- */
static void test_l129_inherit_flex(void) {
    lays("<div id=p style='display:flex;flex:2.5 3.25 calc(10px + 5%);flex-flow:column-reverse wrap-reverse;order:3'>"
         "<p id=a style='flex-grow:7;flex-grow:inherit'>a</p>"
         "<p id=b style='flex-shrink:7;flex-shrink:inherit'>b</p>"
         "<p id=c style='flex-basis:9px;flex-basis:inherit'>c</p>"
         "<p id=d style='flex:7 8 9px;flex:inherit'>d</p>"
         "<p id=e style='flex-direction:row;flex-direction:inherit'>e</p>"
         "<p id=f style='flex-wrap:wrap;flex-wrap:inherit'>f</p>"
         "<p id=g style='flex-flow:row nowrap;flex-flow:inherit'>g</p>"
         "<p id=h style='order:-2;order:inherit'>h</p>"
         "<p id=i style='flex:7 8 9px;flex-grow:inherit;flex-direction:inherit'>i</p>"
         "</div>");
    EQ_MSG(2, el("p")->flex_grow, "p grow"); EQ_MSG(50, el("p")->grow_frac, "p frac");
    EQ_MSG(1, el("p")->length[WE_LEN_BASIS].count > 0, "p basis math");
    SAME_GROW("a", "p"); SAME_GROW("d", "p"); SAME_GROW("i", "p");
    SAME_SHRINK("b", "p"); SAME_SHRINK("d", "p");
    SAME_BASIS("c", "p"); SAME_BASIS("d", "p");
    /* a longhand leaves the other groups */
    EQ_MSG(-1, el("a")->flex_shrink, "a shrink"); EQ_MSG(-1, el("a")->flex_basis, "a basis");
    EQ_MSG(0, el("a")->basis_set, "a basis set"); EQ_MSG(1, slot_empty(el("a"), WE_LEN_BASIS), "a basis math");
    EQ_MSG(-1, el("b")->flex_grow, "b grow"); EQ_MSG(0, el("b")->basis_set, "b basis set");
    EQ_MSG(-1, el("c")->flex_grow, "c grow"); EQ_MSG(-1, el("c")->flex_shrink, "c shrink");
    EQ_MSG(8, el("i")->flex_shrink, "i shrink"); EQ_MSG(9, el("i")->flex_basis, "i basis");
    EQ_MSG(1, el("i")->flex_reverse, "i reverse"); EQ_MSG(1, el("i")->flex_dir, "i dir");
    EQ_MSG(1, el("e")->flex_dir, "e dir"); EQ_MSG(1, el("e")->flex_reverse, "e reverse"); EQ_MSG(0, el("e")->flex_wrap, "e wrap");
    EQ_MSG(0, el("f")->flex_dir, "f dir"); EQ_MSG(0, el("f")->flex_reverse, "f reverse"); EQ_MSG(2, el("f")->flex_wrap, "f wrap");
    EQ_MSG(1, el("g")->flex_dir, "g dir"); EQ_MSG(1, el("g")->flex_reverse, "g reverse"); EQ_MSG(2, el("g")->flex_wrap, "g wrap");
    EQ_MSG(3, el("h")->item_order, "h order"); EQ_MSG(0, el("a")->item_order, "a order");
    EQ_MSG(0, diag_count("refused"), "accepted");
    /* the other basis forms */
    lays("<div id=p style='display:flex;flex-basis:min-content'><p id=a style='flex-basis:inherit'>a</p></div>"
         "<div id=q style='display:flex;flex-basis:content'><p id=b style='flex-basis:inherit'>b</p></div>"
         "<div id=r style='display:flex;flex-basis:30%'><p id=c style='flex-basis:inherit'>c</p></div>"
         "<div id=s style='display:flex;flex-basis:fit-content(20px)'><p id=d style='flex-basis:inherit'>d</p></div>");
    EQ_MSG(1, el("p")->basis_kind != 0, "p kind"); EQ_MSG(1, el("q")->basis_content, "q content"); EQ_MSG(30, el("r")->basis_pct, "r pct");
    SAME_BASIS("a", "p"); SAME_BASIS("b", "q"); SAME_BASIS("c", "r"); SAME_BASIS("d", "s");
    EQ_MSG(1, el("s")->length[WE_LEN_BASIS].count > 0, "s math");
}

static void test_l129_inherit_align(void) {
    lays("<div id=p style='display:flex;justify-content:safe center;align-content:unsafe end;align-items:safe flex-end;"
         "align-self:unsafe center;justify-items:safe end;justify-self:unsafe start'>"
         "<p id=a style='justify-content:start;justify-content:inherit'>a</p>"
         "<p id=b style='align-content:start;align-content:inherit'>b</p>"
         "<p id=c style='align-items:start;align-items:inherit'>c</p>"
         "<p id=d style='align-self:start;align-self:inherit'>d</p>"
         "<p id=e style='justify-items:start;justify-items:inherit'>e</p>"
         "<p id=f style='justify-self:start;justify-self:inherit'>f</p>"
         "<p id=g style='place-content:start;place-content:inherit'>g</p>"
         "<p id=h style='place-items:start;place-items:inherit'>h</p>"
         "<p id=i style='place-self:start;place-self:inherit'>i</p>"
         "<p id=j style='place-content:safe start;place-items:unsafe start;place-self:safe end;justify-content:inherit'>j</p>"
         "</div>");
    {
        const we_node_t *p = el("p");
        EQ_MSG(1, p->justify_content && p->align_content && p->align_items && p->align_self && p->justify_items && p->justify_self, "p set");
        EQ_MSG(1, ovf(p, WE_OVF_JC) && ovf(p, WE_OVF_AC) && ovf(p, WE_OVF_AI) && ovf(p, WE_OVF_AS) && ovf(p, WE_OVF_JI) && ovf(p, WE_OVF_JS), "p overflow");
    }
    EQ_MSG(el("p")->justify_content, el("a")->justify_content, "a jc"); EQ_MSG(ovf(el("p"), WE_OVF_JC), ovf(el("a"), WE_OVF_JC), "a jc ovf");
    EQ_MSG(el("p")->align_content, el("b")->align_content, "b ac"); EQ_MSG(ovf(el("p"), WE_OVF_AC), ovf(el("b"), WE_OVF_AC), "b ac ovf");
    EQ_MSG(el("p")->align_items, el("c")->align_items, "c ai"); EQ_MSG(ovf(el("p"), WE_OVF_AI), ovf(el("c"), WE_OVF_AI), "c ai ovf");
    EQ_MSG(el("p")->align_self, el("d")->align_self, "d as"); EQ_MSG(ovf(el("p"), WE_OVF_AS), ovf(el("d"), WE_OVF_AS), "d as ovf");
    EQ_MSG(el("p")->justify_items, el("e")->justify_items, "e ji"); EQ_MSG(ovf(el("p"), WE_OVF_JI), ovf(el("e"), WE_OVF_JI), "e ji ovf");
    EQ_MSG(el("p")->justify_self, el("f")->justify_self, "f js"); EQ_MSG(ovf(el("p"), WE_OVF_JS), ovf(el("f"), WE_OVF_JS), "f js ovf");
    /* each longhand leaves the others at their initial value */
    EQ_MSG(0, el("a")->align_content, "a ac"); EQ_MSG(0, el("a")->align_overflow >> 2, "a other ovf");
    EQ_MSG(0, el("f")->justify_items, "f ji"); EQ_MSG(0, el("f")->align_overflow & 0x3ff, "f other ovf");
    EQ_MSG(el("p")->align_content, el("g")->align_content, "g ac"); EQ_MSG(el("p")->justify_content, el("g")->justify_content, "g jc");
    EQ_MSG(el("p")->align_overflow & 0xf, el("g")->align_overflow, "g ovf");
    EQ_MSG(el("p")->align_items, el("h")->align_items, "h ai"); EQ_MSG(el("p")->justify_items, el("h")->justify_items, "h ji");
    EQ_MSG(el("p")->align_overflow & 0x330, el("h")->align_overflow, "h ovf");
    EQ_MSG(el("p")->align_self, el("i")->align_self, "i as"); EQ_MSG(el("p")->justify_self, el("i")->justify_self, "i js");
    EQ_MSG(el("p")->align_overflow & 0xcc0, el("i")->align_overflow, "i ovf");
    /* j: only the justify-content bits come from the parent */
    EQ_MSG(el("p")->justify_content, el("j")->justify_content, "j jc");
    EQ_MSG(ovf(el("p"), WE_OVF_JC), ovf(el("j"), WE_OVF_JC), "j jc ovf");
    EQ_MSG(1, ovf(el("j"), WE_OVF_AC) != 0 && ovf(el("j"), WE_OVF_AI) != 0 && ovf(el("j"), WE_OVF_JS) != 0, "j kept ovf");
    EQ_MSG(0, diag_count("refused"), "accepted");
}

static void test_l129_inherit_gap(void) {
    lays("<div id=p style='display:flex;row-gap:6px;column-gap:7%'>"
         "<p id=a style='row-gap:1px;row-gap:inherit'>a</p>"
         "<p id=b style='column-gap:1px;column-gap:inherit'>b</p>"
         "<p id=c style='gap:1px 2px;gap:inherit'>c</p></div>"
         "<div id=q style='display:flex;gap:calc(3px + 2px) 9px'>"
         "<p id=d style='gap:inherit'>d</p><p id=e style='row-gap:inherit'>e</p></div>");
    EQ_MSG(6, el("a")->row_gap, "a row"); EQ_MSG(2, el("a")->gap_set & 2, "a set"); EQ_MSG(0, el("a")->gap_set & 1, "a col unset");
    EQ_MSG(-1, el("a")->col_gap, "a col");
    EQ_MSG(el("p")->col_gap, el("b")->col_gap, "b col"); EQ_MSG(el("p")->gap_pct[0], el("b")->gap_pct[0], "b pct");
    EQ_MSG(0x8007, el("b")->gap_pct[0], "b pct value"); EQ_MSG(1, el("b")->gap_set, "b set"); EQ_MSG(-1, el("b")->row_gap, "b row");
    EQ_MSG(6, el("c")->row_gap, "c row"); EQ_MSG(0x8007, el("c")->gap_pct[0], "c col pct"); EQ_MSG(0, el("c")->gap_pct[1], "c row pct");
    EQ_MSG(3, el("c")->gap_set, "c set");
    EQ_MSG(1, slot_same(el("d"), el("q"), WE_LEN_ROW_GAP) && slot_same(el("d"), el("q"), WE_LEN_COL_GAP), "d math");
    EQ_MSG(1, el("d")->length[WE_LEN_ROW_GAP].count > 0, "d row math");
    EQ_MSG(el("q")->row_gap, el("d")->row_gap, "d row"); EQ_MSG(9, el("d")->col_gap, "d col");
    EQ_MSG(1, slot_same(el("e"), el("q"), WE_LEN_ROW_GAP), "e math"); EQ_MSG(1, slot_empty(el("e"), WE_LEN_COL_GAP), "e col math");
    /* a row percentage and a column math expression */
    lays("<div id=p style='display:flex;row-gap:10%;column-gap:calc(2px + 4px)'>"
         "<p id=a style='row-gap:1px;row-gap:inherit'>a</p><p id=b style='column-gap:1px;column-gap:inherit'>b</p></div>");
    EQ_MSG(0x800a, el("a")->gap_pct[1], "a row pct"); EQ_MSG(0, el("a")->row_gap, "a row");
    EQ_MSG(1, el("p")->length[WE_LEN_COL_GAP].count > 0, "p col math");
    EQ_MSG(1, slot_same(el("b"), el("p"), WE_LEN_COL_GAP), "b col math"); EQ_MSG(6, el("b")->col_gap, "b col");
    EQ_MSG(0, el("b")->gap_pct[0], "b col pct");
    /* an explicit 0 axis next to a shorthand value stays 0 */
    lays("<div id=p style='display:flex;flex-direction:column;gap:7px;row-gap:0px'>"
         "<div id=a style='display:flex;flex-direction:column;row-gap:inherit'><i id=a1>x</i><i id=a2>y</i></div></div>");
    EQ_MSG(7, el("p")->gap, "p gap"); EQ_MSG(0, el("a")->row_gap, "a row"); EQ_MSG(10, el("a2")->y - el("a1")->y, "a no gap");
    /* the shorthand's single value: gap copies it, an axis resolves it */
    lays("<div id=r style='display:flex;width:300px;gap:7px'>"
         "<div id=a style='display:flex;gap:inherit'><i id=a1>x</i><i id=a2>y</i></div>"
         "<div id=b style='display:flex;column-gap:inherit'><i id=b1>x</i><i id=b2>y</i></div>"
         "<div id=c style='display:flex;gap:3px;column-gap:inherit'><i id=c1>x</i><i id=c2>y</i></div>"
         "<div id=d style='display:flex;flex-direction:column;gap:3px;row-gap:inherit'><i id=d1>x</i><i id=d2>y</i></div></div>"
         "<div id=s style='display:flex;width:300px'>"
         "<div id=e style='display:flex;gap:9px;column-gap:inherit'><i id=e1>x</i><i id=e2>y</i></div>"
         "<div id=f style='display:flex;gap:9px;gap:inherit'><i id=f1>x</i><i id=f2>y</i></div></div>");
    EQ_MSG(7, el("a")->gap, "a gap"); EQ_MSG(-1, el("a")->col_gap, "a col"); EQ_MSG(-1, el("a")->row_gap, "a row");
    EQ_MSG(15, el("a2")->x - el("a")->x, "a gap used");
    EQ_MSG(7, el("b")->col_gap, "b col"); EQ_MSG(15, el("b2")->x - el("b")->x, "b gap used");
    EQ_MSG(7, el("c")->col_gap, "c col"); EQ_MSG(-1, el("c")->row_gap, "c row");
    EQ_MSG(15, el("c2")->x - el("c")->x, "c gap used");
    EQ_MSG(7, el("d")->row_gap, "d row"); EQ_MSG(17, el("d2")->y - el("d")->y, "d gap used");
    EQ_MSG(0, el("e")->col_gap, "e col"); EQ_MSG(8, el("e2")->x - el("e")->x, "e gap used");
    EQ_MSG(0, el("f")->gap, "f gap"); EQ_MSG(-1, el("f")->col_gap, "f col"); EQ_MSG(8, el("f2")->x - el("f")->x, "f gap used");
    EQ_MSG(0, diag_count("refused"), "accepted");
}

static void test_l129_inherit_size(void) {
    lays("<div style='height:200px'><div id=p style='width:50%;height:calc(20% + 3px);min-width:min-content;max-width:40%;min-height:5%;max-height:70px;box-sizing:border-box'>"
         "<p id=a style='width:9px;width:inherit'>a</p>"
         "<p id=b style='height:9px;height:inherit'>b</p>"
         "<p id=c style='min-width:9px;min-width:inherit'>c</p>"
         "<p id=d style='max-width:9px;max-width:inherit'>d</p>"
         "<p id=e style='min-height:9px;min-height:inherit'>e</p>"
         "<p id=f style='max-height:9px;max-height:inherit'>f</p>"
         "<p id=g style='box-sizing:content-box;box-sizing:inherit'>g</p></div>"
         "<div id=q style='width:fit-content;height:min-content;min-width:calc(10% + 1px);max-width:max-content;min-height:calc(4px + 1%);max-height:30%'>"
         "<p id=h style='width:inherit;height:inherit;min-width:inherit;max-width:inherit;min-height:inherit;max-height:inherit'>h</p></div></div>");
    EQ_MSG(el("p")->width, el("a")->width, "a width"); EQ_MSG(50, el("a")->width_pct, "a pct"); EQ_MSG(el("p")->width_kind, el("a")->width_kind, "a kind");
    EQ_MSG(1, slot_same(el("a"), el("p"), WE_LEN_WIDTH), "a math");
    EQ_MSG(-1, el("a")->height, "a height"); EQ_MSG(0, el("a")->border_box, "a box");
    EQ_MSG(el("p")->height, el("b")->height, "b height"); EQ_MSG(el("p")->height_pct, el("b")->height_pct, "b pct");
    EQ_MSG(1, el("b")->height_percent, "b percent"); EQ_MSG(el("p")->height_intrinsic, el("b")->height_intrinsic, "b intrinsic");
    EQ_MSG(1, el("b")->length[WE_LEN_HEIGHT].count > 0 && slot_same(el("b"), el("p"), WE_LEN_HEIGHT), "b math");
    EQ_MSG(-1, el("b")->width, "b width"); EQ_MSG(0, el("b")->width_pct, "b width pct");
    EQ_MSG(el("p")->min_width, el("c")->min_width, "c min"); EQ_MSG(1, el("c")->min_width_kind != 0, "c kind");
    EQ_MSG(el("p")->min_width_kind, el("c")->min_width_kind, "c same kind"); EQ_MSG(-1, el("c")->max_width, "c max");
    EQ_MSG(1, slot_same(el("c"), el("p"), WE_LEN_MIN_W), "c math");
    EQ_MSG(el("p")->max_width, el("d")->max_width, "d max"); EQ_MSG(40, el("d")->max_pct, "d pct"); EQ_MSG(0, el("d")->max_width_kind, "d kind");
    EQ_MSG(-1, el("d")->min_width, "d min"); EQ_MSG(0, el("d")->min_width_kind, "d min kind");
    EQ_MSG(el("p")->min_height, el("e")->min_height, "e min"); EQ_MSG(5, el("e")->min_height_pct, "e pct"); EQ_MSG(1, el("e")->min_height_percent, "e percent");
    EQ_MSG(-1, el("e")->max_height, "e max");
    EQ_MSG(70, el("f")->max_height, "f max"); EQ_MSG(70, el("f")->max_height_pct, "f pct"); EQ_MSG(0, el("f")->max_height_percent, "f percent");
    EQ_MSG(-1, el("f")->min_height, "f min");
    EQ_MSG(1, el("g")->border_box, "g box");
    {
        const we_node_t *q = el("q"), *h = el("h");
        int k;
        EQ_MSG(q->width_kind, h->width_kind, "h width kind"); EQ_MSG(1, h->width_kind != 0, "h kind set");
        EQ_MSG(q->width, h->width, "h width"); EQ_MSG(q->width_pct, h->width_pct, "h width pct");
        EQ_MSG(1, h->height_intrinsic, "h intrinsic"); EQ_MSG(-1, h->height, "h height");
        EQ_MSG(q->min_width, h->min_width, "h min"); EQ_MSG(q->min_pct, h->min_pct, "h min pct"); EQ_MSG(q->min_width_kind, h->min_width_kind, "h min kind");
        EQ_MSG(q->max_width_kind, h->max_width_kind, "h max kind"); EQ_MSG(1, h->max_width_kind != 0, "h max kind set");
        EQ_MSG(q->min_height, h->min_height, "h min h"); EQ_MSG(q->min_height_percent, h->min_height_percent, "h min h percent");
        EQ_MSG(q->max_height, h->max_height, "h max h"); EQ_MSG(30, h->max_height_pct, "h max h pct"); EQ_MSG(1, h->max_height_percent, "h max h percent");
        for (k = WE_LEN_WIDTH; k <= WE_LEN_MAX_H; k++) EQ_MSG(1, slot_same(h, q, k), "h slot");
        EQ_MSG(1, q->length[WE_LEN_MIN_W].count > 0 && q->length[WE_LEN_MIN_H].count > 0, "q math");
    }
    /* h's inherited min-height calc(4px + 1%) meets q's content height */
    EQ_MSG(1, diag_count("math evaluation refused"), "indefinite percent");
    EQ_MSG(1, diag_count("refused"), "accepted");
}

/* --- initial, unset, revert: the initial values, other groups kept --- */
static void test_l129_initial(void) {
    static const char *const words[3] = { "initial", "unset", "revert" };
    static char html[4096];
    int w;
    for (w = 0; w < 3; w++) {
        const char *k = words[w];
        snprintf(html, sizeof(html),
            "<div style='display:flex;flex:2 3 4px;flex-flow:column wrap;order:2;gap:5px;width:50px;height:20px;align-items:center;box-sizing:border-box'>"
            "<p id=a style='flex:2.5 3.5 calc(5px + 5%%);flex-grow:%s'>a</p>"
            "<p id=b style='flex:2.5 3.5 calc(5px + 5%%);flex-shrink:%s'>b</p>"
            "<p id=c style='flex:2.5 3.5 calc(5px + 5%%);flex-basis:%s'>c</p>"
            "<p id=d style='flex:2.5 3.5 content;flex:%s'>d</p>"
            "<p id=e style='flex-flow:column-reverse wrap-reverse;flex-direction:%s'>e</p>"
            "<p id=f style='flex-flow:column-reverse wrap-reverse;flex-wrap:%s'>f</p>"
            "<p id=g style='flex-flow:column-reverse wrap-reverse;flex-flow:%s'>g</p>"
            "<p id=h style='order:4;order:%s'>h</p>"
            "<p id=i style='flex-basis:min-content;flex-basis:%s'>i</p></div>",
            k, k, k, k, k, k, k, k, k);
        lays(html);
        EQ_MSG(0, el("a")->flex_grow, "a grow"); EQ_MSG(0, el("a")->grow_frac, "a frac");
        EQ_MSG(3, el("a")->flex_shrink, "a shrink kept"); EQ_MSG(1, el("a")->basis_set, "a basis kept");
        EQ_MSG(1, el("a")->length[WE_LEN_BASIS].count > 0, "a math kept");
        EQ_MSG(1, el("b")->flex_shrink, "b shrink"); EQ_MSG(0, el("b")->shrink_frac, "b frac"); EQ_MSG(2, el("b")->flex_grow, "b grow kept");
        EQ_MSG(-1, el("c")->flex_basis, "c basis"); EQ_MSG(1, el("c")->basis_set, "c set"); EQ_MSG(0, el("c")->basis_pct, "c pct");
        EQ_MSG(1, slot_empty(el("c"), WE_LEN_BASIS), "c math"); EQ_MSG(50, el("c")->grow_frac, "c frac kept");
        EQ_MSG(0, el("d")->flex_grow, "d grow"); EQ_MSG(1, el("d")->flex_shrink, "d shrink"); EQ_MSG(0, el("d")->shrink_frac, "d frac");
        EQ_MSG(0, el("d")->basis_content, "d content"); EQ_MSG(1, el("d")->basis_set, "d set"); EQ_MSG(-1, el("d")->flex_basis, "d basis");
        EQ_MSG(0, el("e")->flex_dir, "e dir"); EQ_MSG(0, el("e")->flex_reverse, "e reverse"); EQ_MSG(2, el("e")->flex_wrap, "e wrap kept");
        EQ_MSG(1, el("f")->flex_dir, "f dir kept"); EQ_MSG(1, el("f")->flex_reverse, "f reverse kept"); EQ_MSG(0, el("f")->flex_wrap, "f wrap");
        EQ_MSG(0, el("g")->flex_dir, "g dir"); EQ_MSG(0, el("g")->flex_reverse, "g reverse"); EQ_MSG(0, el("g")->flex_wrap, "g wrap");
        EQ_MSG(0, el("h")->item_order, "h order");
        EQ_MSG(0, el("i")->basis_kind, "i kind"); EQ_MSG(1, el("i")->basis_set, "i set"); EQ_MSG(-1, el("i")->flex_basis, "i basis");
        EQ_MSG(0, diag_count("refused"), "accepted");
    }
}

static void test_l129_initial_box(void) {
    static const char *const words[3] = { "initial", "unset", "revert" };
    static char html[4096];
    int w;
    for (w = 0; w < 3; w++) {
        const char *k = words[w];
        snprintf(html, sizeof(html),
            "<div style='display:flex;width:300px;height:40px;gap:5px;align-items:center;justify-items:end;box-sizing:border-box'>"
            "<p id=a style='width:calc(10px + 5%%);width:%s'>a</p>"
            "<p id=b style='width:fit-content;height:min-content;width:%s;height:%s'>b</p>"
            "<p id=c style='height:calc(5px + 10%%);height:%s'>c</p>"
            "<p id=d style='min-width:20%%;max-width:min-content;min-height:7%%;max-height:calc(3px + 9%%);min-width:%s;max-width:%s;min-height:%s;max-height:%s'>d</p>"
            "<p id=e style='min-width:min-content;max-width:calc(30px + 1%%);min-height:7px;max-height:9px;min-width:%s;max-width:%s;min-height:%s;max-height:%s'>e</p>"
            "<p id=f style='box-sizing:border-box;box-sizing:%s'>f</p>"
            "<p id=g style='justify-content:safe center;align-content:unsafe end;align-items:safe end;align-self:unsafe end;justify-items:safe end;justify-self:unsafe end;"
            "justify-content:%s;align-items:%s;justify-self:%s'>g</p>"
            "<p id=h style='place-content:safe center;place-items:unsafe end;place-self:safe end;place-content:%s;place-items:%s;place-self:%s'>h</p>"
            "<p id=i style='display:flex;gap:calc(1px + 2px) 4%%;row-gap:%s'>i</p>"
            "<p id=j style='display:flex;gap:6px;column-gap:%s'>j</p>"
            "<p id=l style='display:flex;gap:calc(1px + 2px) 4%%;gap:%s'>l</p>"
            "<p id=m style='display:flex;align-content:center;align-self:center;justify-items:end;align-content:%s;align-self:%s;justify-items:%s'>m</p></div>",
            k, k, k, k, k, k, k, k, k, k, k, k, k, k, k, k, k, k, k, k, k, k, k, k, k);
        lays(html);
        EQ_MSG(-1, el("a")->width, "a width"); EQ_MSG(0, el("a")->width_pct, "a pct"); EQ_MSG(1, slot_empty(el("a"), WE_LEN_WIDTH), "a math");
        EQ_MSG(0, el("b")->width_kind, "b kind"); EQ_MSG(0, el("b")->height_intrinsic, "b intrinsic");
        EQ_MSG(-1, el("c")->height, "c height"); EQ_MSG(0, el("c")->height_percent, "c percent"); EQ_MSG(0, el("c")->height_pct, "c pct");
        EQ_MSG(1, slot_empty(el("c"), WE_LEN_HEIGHT), "c math");
        EQ_MSG(-1, el("d")->min_width, "d min"); EQ_MSG(0, el("d")->min_pct, "d min pct");
        EQ_MSG(-1, el("d")->max_width, "d max"); EQ_MSG(0, el("d")->max_width_kind, "d max kind");
        EQ_MSG(-1, el("d")->min_height, "d min h"); EQ_MSG(0, el("d")->min_height_pct, "d min h pct"); EQ_MSG(0, el("d")->min_height_percent, "d min h percent");
        EQ_MSG(-1, el("d")->max_height, "d max h"); EQ_MSG(0, el("d")->max_height_percent, "d max h percent"); EQ_MSG(0, el("d")->max_height_pct, "d max h pct");
        EQ_MSG(1, slot_empty(el("d"), WE_LEN_MAX_H), "d max h math");
        EQ_MSG(0, el("e")->min_width_kind, "e min kind"); EQ_MSG(-1, el("e")->max_width, "e max"); EQ_MSG(0, el("e")->max_pct, "e max pct");
        EQ_MSG(1, slot_empty(el("e"), WE_LEN_MAX_W), "e max math");
        EQ_MSG(-1, el("e")->min_height, "e min h"); EQ_MSG(-1, el("e")->max_height, "e max h");
        EQ_MSG(0, el("f")->border_box, "f box");
        EQ_MSG(0, el("g")->justify_content, "g jc"); EQ_MSG(0, el("g")->align_items, "g ai"); EQ_MSG(0, el("g")->justify_self, "g js");
        EQ_MSG(0, ovf(el("g"), WE_OVF_JC), "g jc ovf"); EQ_MSG(0, ovf(el("g"), WE_OVF_AI), "g ai ovf"); EQ_MSG(0, ovf(el("g"), WE_OVF_JS), "g js ovf");
        EQ_MSG(1, el("g")->align_content != 0 && el("g")->align_self != 0 && el("g")->justify_items != 0, "g kept");
        EQ_MSG(1, ovf(el("g"), WE_OVF_AC) != 0 && ovf(el("g"), WE_OVF_AS) != 0 && ovf(el("g"), WE_OVF_JI) != 0, "g kept ovf");
        EQ_MSG(0, el("h")->align_overflow, "h ovf");
        EQ_MSG(0, el("h")->align_content | el("h")->justify_content | el("h")->align_items | el("h")->justify_items | el("h")->align_self | el("h")->justify_self, "h values");
        EQ_MSG(0, el("m")->align_content | el("m")->align_self | el("m")->justify_items, "m values");
        EQ_MSG(0, el("i")->row_gap, "i row"); EQ_MSG(1, slot_empty(el("i"), WE_LEN_ROW_GAP), "i row math"); EQ_MSG(1, el("i")->gap_set, "i set");
        EQ_MSG(0x8004, el("i")->gap_pct[0], "i col kept");
        EQ_MSG(0, el("j")->col_gap, "j col"); EQ_MSG(-1, el("j")->row_gap, "j row kept"); EQ_MSG(6, el("j")->gap, "j gap kept");
        EQ_MSG(2, el("j")->gap_set, "j set");
        EQ_MSG(0, el("l")->gap, "l gap"); EQ_MSG(-1, el("l")->col_gap, "l col"); EQ_MSG(0, el("l")->gap_pct[0], "l pct");
        EQ_MSG(0, el("l")->gap_pct[1], "l row pct"); EQ_MSG(0, el("l")->gap_set, "l set");
        EQ_MSG(1, slot_empty(el("l"), WE_LEN_ROW_GAP) && slot_empty(el("l"), WE_LEN_COL_GAP), "l math");
        EQ_MSG(0, diag_count("refused"), "accepted");
    }
}

/* a top-level element inherits from the document node, which keeps the
 * style reset (no factor, no basis); revert-layer and other words stay
 * refused, the earlier value kept */
static void test_l129_root_refused(void) {
    lays("<p id=a style='width:50px;flex:2 3 4px;order:2;gap:5px;justify-content:center;box-sizing:border-box;"
         "width:inherit;flex:inherit;order:inherit;gap:inherit;justify-content:inherit;box-sizing:inherit'>a</p>");
    EQ_MSG(-1, el("a")->width, "a width"); EQ_MSG(-1, el("a")->flex_grow, "a grow"); EQ_MSG(-1, el("a")->flex_shrink, "a shrink");
    EQ_MSG(0, el("a")->basis_set, "a basis"); EQ_MSG(-1, el("a")->flex_basis, "a basis auto"); EQ_MSG(0, el("a")->item_order, "a order"); EQ_MSG(0, el("a")->gap, "a gap");
    EQ_MSG(0, el("a")->justify_content, "a jc"); EQ_MSG(0, el("a")->border_box, "a box");
    EQ_MSG(0, diag_count("refused"), "accepted");
    lays("<div style='display:flex;flex:5;order:5'><p id=a style='flex:2 3 4px;order:2;flex:revert-layer;order:revert-layer;flex-grow:inherits;order:Inherit'>a</p>"
         "<p id=b style='FLEX-GROW:INITIAL;flex-shrink:Unset;flex-basis:REVERT;flex-grow:4;flex-grow:default'>b</p></div>");
    EQ_MSG(2, el("a")->flex_grow, "a grow kept"); EQ_MSG(5, el("a")->item_order, "a order (Inherit)");
    EQ_MSG(4, el("b")->flex_grow, "b grow"); EQ_MSG(1, el("b")->flex_shrink, "b shrink"); EQ_MSG(1, el("b")->basis_set, "b basis");
    EQ_MSG(1, diag_count("flex refused"), "revert-layer flex");
    EQ_MSG(1, diag_count("order refused"), "revert-layer order");
    EQ_MSG(2, diag_count("flex-grow refused"), "inherits, default");
    /* a property outside the list is unchanged: a keyword stays its own */
    lays("<div style='display:grid;grid-template-columns:10px 20px'><p id=a style='grid-template-columns:inherit'>a</p></div>");
    EQ_MSG(1, diag_count("grid tracks refused"), "grid tracks");
}

/* --- layout --- */
static void test_l129_layout(void) {
    /* flex:inherit gives 2 3 4px to a; b has the same, so they share */
    lays("<div id=r style='display:flex;width:300px'><div id=p style='display:flex;width:300px;flex:2 3 4px'>"
         "<i id=a style='flex:inherit'>a</i><i id=b style='flex:2 3 4px'>b</i></div></div>");
    XY("a", 0, 0, 150, 10); XY("b", 150, 0, 150, 10);
    /* flex-flow:inherit turns the inner box into a column */
    lays("<div style='display:flex;flex-flow:column;width:300px'><div id=a style='display:flex;flex-flow:inherit'><span id=x>x</span><span id=y>y</span></div></div>");
    XY("a", 0, 0, 300, 20); XY("x", 0, 0, 300, 10); XY("y", 0, 10, 300, 10);
    /* width, height, box-sizing and min-width */
    lays("<div style='display:flex;width:300px'><div style='width:100px;height:30px;box-sizing:border-box;min-width:120px'>"
         "<div id=a style='display:inline-block;width:inherit;height:inherit;padding:5px;box-sizing:inherit'>a</div>"
         "<div id=b style='display:inline-block;width:20px;min-width:inherit'>b</div></div></div>");
    XY("a", 0, 0, 100, 30); EQ_MSG(120, el("b")->w, "b min");
    /* initial undoes an earlier width; unset an earlier order */
    lays("<div id=r style='display:flex;width:100px'><i id=a style='width:50px;width:initial'>a</i><i id=b style='order:-1;order:unset'>b</i></div>");
    XY("a", 0, 0, 8, 10); XY("b", 8, 0, 8, 10);
    /* justify-content:inherit centres the inner items */
    lays("<div style='display:flex;justify-content:center;width:200px'><div id=a style='display:flex;width:40px;justify-content:inherit'><i id=x>x</i></div></div>");
    XY("x", 96, 0, 8, 10);
    /* height:inherit of a percentage resolves against the child's own containing block */
    lays("<div style='height:200px'><div style='height:50%'><div id=a style='height:inherit'>a</div></div></div>");
    EQ_MSG(50, el("a")->h, "a half of 100");
}

/* a second style pass (a container query read another size) restarts from the reset */
static void test_l129_restyle(void) {
    lays("<style>#a{width:40px;order:1}.c{container-type:inline-size}@container (min-width: 500px){#a{width:inherit;order:inherit}}</style>"
         "<div class=c><div style='width:120px;order:3;display:flex'><p id=a>a</p></div></div>");
    EQ_MSG(120, el("a")->width, "wide width"); EQ_MSG(3, el("a")->item_order, "wide order");
    TEST_ASSERT_EQUAL(0, we_layout_viewport(&g_doc, 300, 480));
    EQ_MSG(40, el("a")->width, "narrow width"); EQ_MSG(1, el("a")->item_order, "narrow order");
}

int main(void) {
    unity_init();
    RUN_TEST(test_l129_inherit_flex);
    RUN_TEST(test_l129_inherit_align);
    RUN_TEST(test_l129_inherit_gap);
    RUN_TEST(test_l129_inherit_size);
    RUN_TEST(test_l129_initial);
    RUN_TEST(test_l129_initial_box);
    RUN_TEST(test_l129_root_refused);
    RUN_TEST(test_l129_layout);
    RUN_TEST(test_l129_restyle);
    unity_print_results();
    unity_cleanup();
    return unity_stats.tests_failed == 0 ? 0 : 1;
}
