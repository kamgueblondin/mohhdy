/* Lot 126 (US-031): the safe and unsafe overflow positions (CSS Box
 * Alignment 4.4) on align-items, align-self, align-content,
 * justify-content, justify-items, justify-self and place-*: safe falls
 * back to start when the subject overflows, unsafe aligns it truly; with
 * neither keyword flex items and lines keep the Lot 125 true alignment.
 * align-content of flex lines and the content alignment of grid tracks
 * align truly on overflow too, and grid items stretch by default
 * (align-self normal, not replaced, no aspect ratio). Values follow
 * docs/lot126_plan.md (8 px glyphs, 10 px lines). */
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

static int ovf(const char *id, int prop) { return (el(id)->align_overflow >> (2 * prop)) & 3; }

/* --- parsing: safe / unsafe go with a position only --- */
static void test_l126_parse(void) {
    lays("<div id=r style='display:flex;align-items:safe center;justify-content:unsafe end;align-content:safe flex-start;justify-items:unsafe start'><div id=a style='align-self:unsafe flex-end;justify-self:safe center'>a</div><div id=b style='align-self:safe end;align-self:auto'>b</div></div>");
    EQ_MSG(WE_PACK_CENTER, el("r")->align_items, "align-items value");
    EQ_MSG(WE_OVF_SAFE, ovf("r", WE_OVF_AI), "align-items safe");
    EQ_MSG(WE_PACK_END, el("r")->justify_content, "justify-content value");
    EQ_MSG(WE_OVF_UNSAFE, ovf("r", WE_OVF_JC), "justify-content unsafe");
    EQ_MSG(WE_PACK_FLEX_START, el("r")->align_content, "align-content value");
    EQ_MSG(WE_OVF_SAFE, ovf("r", WE_OVF_AC), "align-content safe");
    EQ_MSG(WE_PACK_START, el("r")->justify_items, "justify-items value");
    EQ_MSG(WE_OVF_UNSAFE, ovf("r", WE_OVF_JI), "justify-items unsafe");
    EQ_MSG(WE_PACK_FLEX_END, el("a")->align_self, "align-self value");
    EQ_MSG(WE_OVF_UNSAFE, ovf("a", WE_OVF_AS), "align-self unsafe");
    EQ_MSG(WE_PACK_CENTER, el("a")->justify_self, "justify-self value");
    EQ_MSG(WE_OVF_SAFE, ovf("a", WE_OVF_JS), "justify-self safe");
    /* auto clears the keyword with the value */
    EQ_MSG(0, el("b")->align_self, "align-self auto");
    EQ_MSG(WE_OVF_NONE, ovf("b", WE_OVF_AS), "auto clears safe");
    EQ_MSG(0, diag_count("alignment refused"), "accepted");
    /* a later value without the keyword clears it */
    lays("<div id=r style='display:flex;align-items:safe center;align-items:end'>a</div>");
    EQ_MSG(WE_PACK_END, el("r")->align_items, "plain value");
    EQ_MSG(WE_OVF_NONE, ovf("r", WE_OVF_AI), "keyword cleared");
    /* refused: no position after the keyword, a non-position, extra tokens */
    lays("<div id=r style='display:flex;align-items:center;align-items:safe stretch;align-items:unsafe;align-items:safe baseline;align-content:safe space-between;justify-content:safe normal;align-items:safe center end;align-self:unsafe auto'>a</div>");
    EQ_MSG(7, diag_count("alignment refused"), "refused");
    EQ_MSG(WE_PACK_CENTER, el("r")->align_items, "value kept");
    EQ_MSG(WE_OVF_NONE, ovf("r", WE_OVF_AI), "keyword not stored");
    /* place-*: one keyword per value; one value copies to both */
    lays("<div id=r style='display:grid;place-items:safe center unsafe end;place-content:unsafe start'><div id=a style='place-self:end safe start'>a</div><div id=b style='place-self:unsafe center'>b</div></div>");
    EQ_MSG(WE_PACK_CENTER, el("r")->align_items, "place-items first");
    EQ_MSG(WE_OVF_SAFE, ovf("r", WE_OVF_AI), "place-items first safe");
    EQ_MSG(WE_PACK_END, el("r")->justify_items, "place-items second");
    EQ_MSG(WE_OVF_UNSAFE, ovf("r", WE_OVF_JI), "place-items second unsafe");
    EQ_MSG(WE_PACK_START, el("r")->align_content, "place-content first");
    EQ_MSG(WE_PACK_START, el("r")->justify_content, "place-content copy");
    EQ_MSG(WE_OVF_UNSAFE, ovf("r", WE_OVF_AC), "place-content first unsafe");
    EQ_MSG(WE_OVF_UNSAFE, ovf("r", WE_OVF_JC), "place-content copy unsafe");
    EQ_MSG(WE_PACK_END, el("a")->align_self, "place-self first");
    EQ_MSG(WE_OVF_NONE, ovf("a", WE_OVF_AS), "place-self first plain");
    EQ_MSG(WE_PACK_START, el("a")->justify_self, "place-self second");
    EQ_MSG(WE_OVF_SAFE, ovf("a", WE_OVF_JS), "place-self second safe");
    EQ_MSG(WE_OVF_UNSAFE, ovf("b", WE_OVF_AS), "place-self one value");
    EQ_MSG(WE_OVF_UNSAFE, ovf("b", WE_OVF_JS), "place-self copied");
    EQ_MSG(0, diag_count("place alignment refused"), "place accepted");
    lays("<div id=r style='display:grid;place-items:center;place-items:center safe;place-items:safe center end start;place-items:center end start;place-items:safe stretch;place-content:center unsafe space-around;place-items:a b c d e;place-items:safe'>a</div>");
    EQ_MSG(7, diag_count("place alignment refused"), "place refused");
    EQ_MSG(WE_PACK_CENTER, el("r")->align_items, "place value kept");
    lays("<div id=r style='display:grid;place-items:safe center unsafe end start;place-items:center safe end start;place-items:center end'>a</div>");
    EQ_MSG(2, diag_count("place alignment refused"), "place refused, extra tokens");
    EQ_MSG(WE_PACK_CENTER, el("r")->align_items, "place two values kept");
    EQ_MSG(WE_PACK_END, el("r")->justify_items, "place two values kept, second");
    /* a container query restyle (Lot 119) clears a keyword its query no
     * longer sets */
    lays("<style>@container (min-width: 250px) { #r { align-items: safe center } }</style><div style='container-type:inline-size;width:50vw'><div id=r style='display:flex'>a</div></div>");
    EQ_MSG(WE_OVF_SAFE, ovf("r", WE_OVF_AI), "query keyword");
    EQ_MSG(WE_PACK_CENTER, el("r")->align_items, "query value");
    TEST_ASSERT_EQUAL(0, we_layout_viewport(&g_doc, 400, 480));
    EQ_MSG(0, el("r")->align_overflow, "restyle clears the keyword");
    EQ_MSG(0, el("r")->align_items, "restyle clears the value");
}

/* --- flex: safe starts an overflowing subject, unsafe and the default align truly --- */
static void test_l126_flex(void) {
    /* items: safe center at the start, unsafe end past the start edge, default center both sides */
    lays("<div style='display:flex;width:100px;height:40px;align-items:safe center'><div id=a style='width:20px;height:60px'></div><div id=b style='width:20px;height:60px;align-self:unsafe end'></div><div id=c style='width:20px;height:10px'></div><div id=d style='width:20px;height:60px;align-self:center'></div></div>");
    BOX("a", 0, 0, 20, 60); BOX("b", 20, -20, 20, 60); BOX("c", 40, 15, 20, 10); BOX("d", 60, -10, 20, 60);
    /* safe is inherited from align-items by align-self: auto */
    lays("<div style='display:flex;width:100px;height:40px;align-items:safe end'><div id=a style='width:20px;height:60px;align-self:auto'></div></div>");
    BOX("a", 0, 0, 20, 60);
    /* a column: the cross axis is horizontal */
    lays("<div style='display:flex;flex-direction:column;width:40px;height:100px;align-items:safe center'><div id=a style='width:60px;height:20px'></div><div id=b style='width:60px;height:20px;align-self:unsafe center'></div></div>");
    BOX("a", 0, 0, 60, 20); BOX("b", -10, 20, 60, 20);
    /* main axis */
    lays("<div style='display:flex;width:100px;justify-content:safe center'><div id=a style='flex:0 0 80px;height:10px'></div><div id=b style='flex:0 0 80px;height:10px'></div></div>");
    BOX("a", 0, 0, 80, 10); BOX("b", 80, 0, 80, 10);
    lays("<div style='display:flex;width:100px;justify-content:unsafe center'><div id=a style='flex:0 0 80px;height:10px'></div><div id=b style='flex:0 0 80px;height:10px'></div></div>");
    BOX("a", -30, 0, 80, 10); BOX("b", 50, 0, 80, 10);
    /* a line that fits keeps its packing under safe */
    lays("<div style='display:flex;width:100px;justify-content:safe center'><div id=a style='flex:0 0 30px;height:10px'></div><div id=b style='flex:0 0 30px;height:10px'></div></div>");
    BOX("a", 20, 0, 30, 10); BOX("b", 50, 0, 30, 10);
    /* row-reverse: safe starts at the writing-mode start (the left) */
    lays("<div style='display:flex;flex-direction:row-reverse;width:100px;justify-content:safe center'><div id=a style='flex:0 0 80px;height:10px'></div><div id=b style='flex:0 0 80px;height:10px'></div></div>");
    BOX("a", 80, 0, 80, 10); BOX("b", 0, 0, 80, 10);
    lays("<div style='display:flex;flex-direction:row-reverse;width:100px;justify-content:safe flex-end'><div id=a style='flex:0 0 80px;height:10px'></div><div id=b style='flex:0 0 80px;height:10px'></div></div>");
    BOX("a", 80, 0, 80, 10); BOX("b", 0, 0, 80, 10);
    /* rtl: the start is the right edge */
    lays("<div style='direction:rtl;display:flex;width:100px;justify-content:safe end'><div id=a style='flex:0 0 80px;height:10px'></div><div id=b style='flex:0 0 80px;height:10px'></div></div>");
    BOX("a", 20, 0, 80, 10); BOX("b", -60, 0, 80, 10);
    /* height: 0px column flex container (main == 0) overflows non-shrinkable items under center */
    lays("<div style='display:flex;flex-direction:column;width:20px;height:0px;justify-content:center'><div id=a style='flex:0 0 20px;width:10px'></div></div>");
    BOX("a", 0, -10, 10, 20);
}

/* --- flex lines: align-content aligns overflowing lines truly --- */
static void test_l126_flex_lines(void) {
    /* two 20 px lines in 20 px: center overflows both sides (was 0 / 20) */
    lays("<div style='display:flex;flex-wrap:wrap;width:50px;height:20px;align-content:center'><div id=a style='width:30px;height:20px'></div><div id=b style='width:30px;height:20px'></div></div>");
    BOX("a", 0, -10, 30, 20); BOX("b", 0, 10, 30, 20);
    /* a single-line container: align-content does not move its line, the
     * taller item overflows its own line (align-items normal) */
    lays("<div style='display:flex;width:50px;height:20px;align-content:center'><div id=a style='width:20px;height:40px'></div><div id=b style='width:20px'>b</div></div>");
    BOX("a", 0, 0, 20, 40); BOX("b", 20, 0, 20, 20);
    lays("<div style='display:flex;flex-wrap:wrap;width:50px;height:20px;align-content:flex-end'><div id=a style='width:30px;height:20px'></div><div id=b style='width:30px;height:20px'></div></div>");
    BOX("a", 0, -20, 30, 20); BOX("b", 0, 0, 30, 20);
    lays("<div style='display:flex;flex-wrap:wrap;width:50px;height:20px;align-content:safe center'><div id=a style='width:30px;height:20px'></div><div id=b style='width:30px;height:20px'></div></div>");
    BOX("a", 0, 0, 30, 20); BOX("b", 0, 20, 30, 20);
    lays("<div style='display:flex;flex-wrap:wrap;width:50px;height:20px;align-content:unsafe center'><div id=a style='width:30px;height:20px'></div><div id=b style='width:30px;height:20px'></div></div>");
    BOX("a", 0, -10, 30, 20); BOX("b", 0, 10, 30, 20);
    /* normal (stretch) and the distributed values fall back to the start */
    lays("<div style='display:flex;flex-wrap:wrap;width:50px;height:20px'><div id=a style='width:30px;height:20px'></div><div id=b style='width:30px;height:20px'></div></div>");
    BOX("a", 0, 0, 30, 20); BOX("b", 0, 20, 30, 20);
    lays("<div style='display:flex;flex-wrap:wrap;width:50px;height:20px;align-content:space-around'><div id=a style='width:30px;height:20px'></div><div id=b style='width:30px;height:20px'></div></div>");
    BOX("a", 0, 0, 30, 20); BOX("b", 0, 20, 30, 20);
    /* wrap-reverse: flex-start packs from the bottom edge and overflows the top */
    lays("<div style='display:flex;flex-wrap:wrap-reverse;width:50px;height:20px;align-content:flex-start'><div id=a style='width:30px;height:20px'></div><div id=b style='width:30px;height:20px'></div></div>");
    BOX("a", 0, 0, 30, 20); BOX("b", 0, -20, 30, 20);
    lays("<div style='display:flex;flex-wrap:wrap-reverse;width:50px;height:20px;align-content:center'><div id=a style='width:30px;height:20px'></div><div id=b style='width:30px;height:20px'></div></div>");
    BOX("a", 0, 10, 30, 20); BOX("b", 0, -10, 30, 20);
    /* safe in wrap-reverse: the writing-mode start, the top */
    lays("<div style='display:flex;flex-wrap:wrap-reverse;width:50px;height:20px;align-content:safe end'><div id=a style='width:30px;height:20px'></div><div id=b style='width:30px;height:20px'></div></div>");
    BOX("a", 0, 20, 30, 20); BOX("b", 0, 0, 30, 20);
    /* a wrapping column overflows its width */
    lays("<div style='display:flex;flex-direction:column;flex-wrap:wrap;width:20px;height:20px;align-content:end'><div id=a style='width:20px;height:20px'></div><div id=b style='width:20px;height:20px'></div></div>");
    BOX("a", -20, 0, 20, 20); BOX("b", 0, 0, 20, 20);
    /* lines that fit keep their packing; an indefinite height has nothing to overflow */
    lays("<div style='display:flex;flex-wrap:wrap;width:50px;height:60px;align-content:center'><div id=a style='width:30px;height:20px'></div><div id=b style='width:30px;height:20px'></div></div>");
    BOX("a", 0, 10, 30, 20); BOX("b", 0, 30, 30, 20);
    lays("<div style='display:flex;flex-wrap:wrap;width:50px;height:60px;align-content:safe center'><div id=a style='width:30px;height:20px'></div><div id=b style='width:30px;height:20px'></div></div>");
    BOX("a", 0, 10, 30, 20); BOX("b", 0, 30, 30, 20);
    lays("<div style='display:flex;flex-wrap:wrap;width:50px;align-content:end'><div id=a style='width:30px;height:20px'></div><div id=b style='width:30px;height:20px'></div></div>");
    BOX("a", 0, 0, 30, 20); BOX("b", 0, 20, 30, 20);
}

/* --- grid: tracks align truly, items stay safe unless unsafe --- */
static void test_l126_grid_alignment(void) {
    /* columns: center overflows both sides (were 0 / 80) */
    lays("<div style='display:grid;width:100px;grid-template-columns:80px 80px;justify-content:center'><div id=a>a</div><div id=b>b</div></div>");
    BOX("a", -30, 0, 80, 10); BOX("b", 50, 0, 80, 10);
    lays("<div style='display:grid;width:100px;grid-template-columns:80px 80px;justify-content:end'><div id=a>a</div><div id=b>b</div></div>");
    BOX("a", -60, 0, 80, 10); BOX("b", 20, 0, 80, 10);
    lays("<div style='display:grid;width:100px;grid-template-columns:80px 80px;justify-content:safe center'><div id=a>a</div><div id=b>b</div></div>");
    BOX("a", 0, 0, 80, 10); BOX("b", 80, 0, 80, 10);
    lays("<div style='direction:rtl;display:grid;width:100px;grid-template-columns:80px 80px;justify-content:center'><div id=a>a</div><div id=b>b</div></div>");
    BOX("a", 50, 0, 80, 10); BOX("b", -30, 0, 80, 10);
    /* rows of a definite height */
    lays("<div style='display:grid;width:50px;height:20px;grid-template-rows:20px 20px;align-content:end'><div id=a>a</div><div id=b>b</div></div>");
    BOX("a", 0, -20, 50, 20); BOX("b", 0, 0, 50, 20);
    lays("<div style='display:grid;width:50px;height:20px;grid-template-rows:20px 20px;align-content:center'><div id=a>a</div><div id=b>b</div></div>");
    BOX("a", 0, -10, 50, 20); BOX("b", 0, 10, 50, 20);
    lays("<div style='display:grid;width:50px;height:0px;grid-template-rows:20px 20px;align-content:center'><div id=a>a</div><div id=b>b</div></div>");
    BOX("a", 0, -20, 50, 20); BOX("b", 0, 0, 50, 20);
    lays("<div style='display:grid;width:50px;height:20px;grid-template-rows:20px 20px;align-content:safe end'><div id=a>a</div><div id=b>b</div></div>");
    BOX("a", 0, 0, 50, 20); BOX("b", 0, 20, 50, 20);
    /* an indefinite height has nothing to overflow */
    lays("<div style='display:grid;width:50px;grid-template-rows:20px 20px;align-content:end'><div id=a>a</div><div id=b>b</div></div>");
    BOX("a", 0, 0, 50, 20); BOX("b", 0, 20, 50, 20);
    /* items: an overflowing item stays at the start of its area unless unsafe */
    lays("<div style='display:grid;width:50px;grid-template-columns:50px;justify-items:center'><div id=a style='width:70px'>a</div><div id=b style='width:70px;justify-self:unsafe center'>b</div><div id=c style='width:70px;justify-self:unsafe end'>c</div><div id=d style='width:30px;justify-self:safe end'>d</div></div>");
    BOX("a", 0, 0, 70, 10); BOX("b", -10, 10, 70, 10); BOX("c", -20, 20, 70, 10); BOX("d", 20, 30, 30, 10);
    lays("<div style='display:grid;width:100px;grid-template-columns:50px 50px;grid-template-rows:10px;align-items:center'><div id=a style='height:30px'>a</div><div id=b style='height:30px;align-self:unsafe center'>b</div></div>");
    BOX("a", 0, 0, 50, 30); BOX("b", 50, -10, 50, 30);
    lays("<div style='display:grid;width:100px;grid-template-columns:50px 50px;grid-template-rows:10px;place-items:unsafe end'><div id=a style='width:70px;height:30px'>a</div></div>");
    BOX("a", -20, -20, 70, 30);
}

/* --- grid items stretch by default --- */
static void test_l126_grid_stretch(void) {
    /* the 10 px item of a 20 px row is 20 high (was 10) */
    lays("<div style='display:grid;width:60px;grid-template-columns:30px 30px'><div id=a>a</div><div id=b>b<br>b</div></div>");
    BOX("a", 0, 0, 30, 20); BOX("b", 30, 0, 30, 20);
    EQ_MSG(0, (int)g_doc.ndiag, "no diag");
    /* a child percentage sees the stretched item; max-height clamps it */
    lays("<div style='display:grid;width:60px;grid-template-columns:30px 30px'><div id=a><div id=c style='height:50%'></div></div><div id=b style='height:40px'>b</div><div id=d style='max-height:15px'></div><div id=e style='height:30px'></div></div>");
    BOX("a", 0, 0, 30, 40); BOX("c", 0, 0, 30, 20); BOX("d", 0, 40, 30, 15);
    /* start, an explicit height, auto margins, align-items: start keep the content height */
    lays("<div style='display:grid;width:120px;grid-template-columns:30px 30px 30px 30px'><div id=a style='align-self:start'>a</div><div id=b style='height:5px'></div><div id=c style='margin-top:auto'>c</div><div id=d style='height:40px'></div></div>");
    BOX("a", 0, 0, 30, 10); BOX("b", 30, 0, 30, 5); BOX("c", 60, 30, 30, 10);
    lays("<div style='display:grid;width:60px;grid-template-columns:30px 30px;align-items:start'><div id=a>a</div><div id=b>b<br>b</div></div>");
    BOX("a", 0, 0, 30, 10);
    /* a replaced element and an item with an aspect ratio keep start */
    lays("<div style='display:grid;width:60px;grid-template-columns:30px 30px'><img id=i width=10 height=5><div id=b style='height:40px'>b</div><div id=a style='aspect-ratio:3/1'></div><div id=e style='height:40px'>e</div></div>");
    BOX("i", 0, 0, 10, 5); BOX("a", 0, 40, 30, 10);
    lays("<div style='display:grid;width:60px;grid-template-columns:30px 30px'><img id=i alt=x><div id=b style='height:60px'>b</div><input id=t><div id=e style='height:60px'>e</div></div>");
    /* an img (alt text) and an input in 60 px rows keep their own height */
    EQ_MSG(10, el("i")->h, "img h"); EQ_MSG(14, el("t")->h, "input h");
    /* a grid stretched by a flex line: its rows (Lot 125) and now its items */
    lays("<div style='display:flex;width:100px;height:40px'><div id=g style='display:grid;width:50px'><div id=c>c</div><div id=d>d</div></div></div>");
    BOX("c", 0, 0, 50, 20); BOX("d", 0, 20, 50, 20);
    /* items of a row subgrid stretch in its rows */
    lays("<div style='display:grid;width:60px;grid-template-columns:30px 30px;grid-template-rows:auto'><div id=s style='display:grid;grid-template-rows:subgrid'><div id=a>a</div></div><div id=b>b<br>b</div></div>");
    BOX("s", 0, 0, 30, 20); BOX("a", 0, 0, 30, 20);
}

int main(void) {
    unity_init();
    RUN_TEST(test_l126_parse);
    RUN_TEST(test_l126_flex);
    RUN_TEST(test_l126_flex_lines);
    RUN_TEST(test_l126_grid_alignment);
    RUN_TEST(test_l126_grid_stretch);
    unity_print_results();
    unity_cleanup();
    return unity_stats.tests_failed == 0 ? 0 : 1;
}
