/* Lot 127 (US-031): self-start / self-end (align-items, align-self,
 * justify-items, justify-self) follow the item's direction; left / right
 * (justify-content, justify-items, justify-self) are physical on the
 * inline axis and start on the other one (CSS Box Alignment 4.2), safe /
 * unsafe and place-* included. An auto-width column flex item or grid
 * item that does not stretch takes fit-content (Flexbox 9.4 step 7, Grid
 * 6.2): min(max-content, max(min-content, available)), not the full width
 * (start) nor an uncapped max-content; a wrapping column sizes its lines
 * from that (not one character). Values follow docs/lot127_plan.md (8 px
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

static int ovf(const char *id, int prop) { return (el(id)->align_overflow >> (2 * prop)) & 3; }

/* --- parsing: which property takes which keyword --- */
static void test_l127_parse(void) {
    lays("<div id=r style='display:flex;align-items:self-end;justify-items:left;justify-content:right'><div id=a style='align-self:self-start;justify-self:self-end'>a</div><div id=b style='justify-self:right;align-self:unsafe self-end'>b</div><div id=c style='justify-self:safe left;justify-self:self-start'>c</div></div>");
    EQ_MSG(WE_PACK_SELF_END, el("r")->align_items, "align-items self-end");
    EQ_MSG(WE_PACK_LEFT, el("r")->justify_items, "justify-items left");
    EQ_MSG(WE_PACK_RIGHT, el("r")->justify_content, "justify-content right");
    EQ_MSG(WE_PACK_SELF_START, el("a")->align_self, "align-self self-start");
    EQ_MSG(WE_PACK_SELF_END, el("a")->justify_self, "justify-self self-end");
    EQ_MSG(WE_PACK_RIGHT, el("b")->justify_self, "justify-self right");
    EQ_MSG(WE_PACK_SELF_END, el("b")->align_self, "align-self unsafe self-end");
    EQ_MSG(WE_OVF_UNSAFE, ovf("b", WE_OVF_AS), "unsafe kept");
    EQ_MSG(WE_PACK_SELF_START, el("c")->justify_self, "justify-self self-start");
    EQ_MSG(WE_OVF_NONE, ovf("c", WE_OVF_JS), "later value clears safe");
    EQ_MSG(0, diag_count("alignment refused"), "accepted");
    lays("<div id=r style='display:flex;justify-items:safe right;justify-content:unsafe left'>a</div>");
    EQ_MSG(WE_PACK_RIGHT, el("r")->justify_items, "safe right");
    EQ_MSG(WE_OVF_SAFE, ovf("r", WE_OVF_JI), "safe right keyword");
    EQ_MSG(WE_PACK_LEFT, el("r")->justify_content, "unsafe left");
    EQ_MSG(WE_OVF_UNSAFE, ovf("r", WE_OVF_JC), "unsafe left keyword");
    /* refused: left / right off the justify-* properties, self-* on the
     * content properties */
    lays("<div id=r style='display:flex;align-items:center;justify-content:center;align-content:center;align-items:left;align-items:right;align-content:left;align-content:self-start;justify-content:self-start;justify-content:self-end;align-items:safe right;justify-content:safe self-end'><div id=a style='align-self:left'>a</div></div>");
    EQ_MSG(8, diag_count("alignment refused"), "refused");
    EQ_MSG(WE_PACK_CENTER, el("r")->align_items, "align-items kept");
    EQ_MSG(WE_PACK_CENTER, el("r")->justify_content, "justify-content kept");
    EQ_MSG(WE_PACK_CENTER, el("r")->align_content, "align-content kept");
    EQ_MSG(0, el("a")->align_self, "align-self left refused");
    lays("<div id=r style='display:flex;align-content:safe self-end;align-self:right;align-content:unsafe right'>a</div>");
    EQ_MSG(3, diag_count("alignment refused"), "refused 2");
    EQ_MSG(0, el("r")->align_content, "align-content not stored");
    /* place-*: the block axis value takes self-*, the inline one left /
     * right too; place-content no self-* */
    lays("<div id=r style='display:grid;place-items:self-end left;place-content:center right'><div id=a style='place-self:self-start safe right'>a</div><div id=b style='place-self:unsafe self-end self-start'>b</div><div id=c style='place-self:self-end'>c</div></div>");
    EQ_MSG(WE_PACK_SELF_END, el("r")->align_items, "place-items first");
    EQ_MSG(WE_PACK_LEFT, el("r")->justify_items, "place-items second");
    EQ_MSG(WE_PACK_CENTER, el("r")->align_content, "place-content first");
    EQ_MSG(WE_PACK_RIGHT, el("r")->justify_content, "place-content second");
    EQ_MSG(WE_PACK_SELF_START, el("a")->align_self, "place-self first");
    EQ_MSG(WE_PACK_RIGHT, el("a")->justify_self, "place-self safe right");
    EQ_MSG(WE_OVF_SAFE, ovf("a", WE_OVF_JS), "place-self safe");
    EQ_MSG(WE_PACK_SELF_END, el("b")->align_self, "place-self unsafe first");
    EQ_MSG(WE_OVF_UNSAFE, ovf("b", WE_OVF_AS), "place-self unsafe");
    EQ_MSG(WE_PACK_SELF_START, el("b")->justify_self, "place-self second");
    EQ_MSG(WE_PACK_SELF_END, el("c")->align_self, "one value copies");
    EQ_MSG(WE_PACK_SELF_END, el("c")->justify_self, "one value copies inline");
    EQ_MSG(0, diag_count("refused"), "place accepted");
    lays("<div id=r style='display:grid;place-content:start safe left;place-items:safe self-start'>a</div>");
    EQ_MSG(WE_PACK_START, el("r")->align_content, "place-content start");
    EQ_MSG(WE_PACK_LEFT, el("r")->justify_content, "place-content safe left");
    EQ_MSG(WE_OVF_SAFE, ovf("r", WE_OVF_JC), "place-content safe");
    EQ_MSG(WE_PACK_SELF_START, el("r")->align_items, "place-items safe self-start");
    EQ_MSG(WE_PACK_SELF_START, el("r")->justify_items, "copied");
    EQ_MSG(WE_OVF_SAFE, ovf("r", WE_OVF_JI), "copied safe");
    lays("<div id=r style='display:grid;place-items:center;place-content:center;place-items:left;place-items:left start;place-content:left;place-content:start self-end;place-content:safe self-start start;place-content:center safe self-end;place-items:start safe self;place-content:safe left start'>a</div>");
    EQ_MSG(8, diag_count("place alignment refused"), "place refused");
    EQ_MSG(WE_PACK_CENTER, el("r")->align_items, "place-items kept");
    EQ_MSG(WE_PACK_CENTER, el("r")->justify_content, "place-content kept");
    lays("<div id=r style='display:grid'><div id=a style='place-self:right;place-self:right left'>a</div></div>");
    EQ_MSG(2, diag_count("place alignment refused"), "place-self refused");
}

/* --- self-start / self-end --- */
static void test_l127_self(void) {
    /* flex row: the cross axis is the block axis, the direction does not count */
    lays("<div style='display:flex;width:100px;height:40px;align-items:self-end'><div id=a style='width:20px'>a</div><div id=b style='width:20px;direction:rtl'>b</div><div id=c style='width:20px;align-self:self-start;direction:rtl'>c</div></div>");
    BOX("a", 0, 30, 20, 10); BOX("b", 20, 30, 20, 10); BOX("c", 40, 0, 20, 10);
    EQ_MSG(0, (int)g_doc.ndiag, "no diag");
    /* flex column: the inline axis; an rtl item starts on the right */
    lays("<div style='display:flex;flex-direction:column;width:100px'><div id=a style='align-self:self-start'>ab</div><div id=b style='align-self:self-end'>ab</div><div id=c style='align-self:self-start;direction:rtl'>ab</div><div id=d style='align-self:self-end;direction:rtl'>ab</div></div>");
    BOX("a", 0, 0, 16, 10); BOX("b", 84, 10, 16, 10); BOX("c", 84, 20, 16, 10); BOX("d", 0, 30, 16, 10);
    /* an rtl container: an ltr item starts on the left */
    lays("<div style='display:flex;flex-direction:column;width:100px;direction:rtl;align-items:self-start'><div id=a>ab</div><div id=b style='direction:ltr'>ab</div><div id=c style='direction:ltr;align-self:self-end'>ab</div><div id=d style='align-self:self-end'>ab</div></div>");
    BOX("a", 84, 0, 16, 10); BOX("b", 0, 10, 16, 10); BOX("c", 84, 20, 16, 10); BOX("d", 0, 30, 16, 10);
    /* wrap-reverse keeps self-start where it is, like start (Lot 125) */
    lays("<div style='display:flex;flex-direction:column;flex-wrap:wrap-reverse;width:100px;height:20px;align-content:start'><div id=a style='align-self:self-start'>ab</div><div id=b style='align-self:flex-start'>ab</div><div id=c>abcdef</div><div id=d>x</div></div>");
    BOX("a", 48, 0, 16, 10); BOX("b", 48, 10, 16, 10); BOX("c", 0, 0, 48, 10);
    lays("<div style='display:flex;flex-direction:column;flex-wrap:wrap-reverse;width:100px;height:20px;align-content:start'><div id=a style='align-self:self-start'>ab</div><div id=b style='align-self:flex-start'>abcd</div><div id=c>abcdef</div><div id=d>x</div></div>");
    BOX("a", 48, 0, 16, 10); BOX("b", 48, 10, 32, 10);
    lays("<div style='display:flex;flex-direction:column;flex-wrap:wrap-reverse;width:100px;height:20px;align-content:start'><div id=a style='align-self:flex-start'>ab</div><div id=b>abcd</div></div>");
    BOX("a", 16, 0, 16, 10); BOX("b", 0, 10, 32, 10);
    /* grid: inline axis per item direction, block axis plain */
    lays("<div style='display:grid;width:100px;grid-template-columns:50px 50px;grid-template-rows:30px;justify-items:self-end'><div id=a>ab</div><div id=b style='direction:rtl'>ab</div></div>");
    BOX("a", 34, 0, 16, 30); BOX("b", 50, 0, 16, 30);
    lays("<div style='display:grid;width:100px;grid-template-columns:50px 50px;grid-template-rows:30px;direction:rtl'><div id=a style='justify-self:self-start;align-self:self-end'>ab</div><div id=b style='direction:ltr;justify-self:self-start;align-self:self-start'>ab</div></div>");
    BOX("a", 84, 20, 16, 10); BOX("b", 0, 0, 16, 10);
    lays("<div style='display:grid;width:100px;grid-template-columns:50px 50px;grid-template-rows:30px'><div id=a style='direction:rtl;align-self:self-end;justify-self:self-start'>ab</div><div id=b style='direction:rtl;justify-self:self-end'>ab</div></div>");
    BOX("a", 34, 20, 16, 10); BOX("b", 50, 0, 16, 30);
}

/* --- left / right --- */
static void test_l127_left_right(void) {
    lays("<div style='display:flex;width:100px;justify-content:right'><div id=a style='width:20px'>a</div><div id=b style='width:20px'>b</div></div>");
    BOX("a", 60, 0, 20, 10); BOX("b", 80, 0, 20, 10);
    lays("<div style='display:flex;width:100px;justify-content:left;direction:rtl'><div id=a style='width:20px'>a</div><div id=b style='width:20px'>b</div></div>");
    BOX("a", 20, 0, 20, 10); BOX("b", 0, 0, 20, 10);
    lays("<div style='display:flex;width:100px;justify-content:right;direction:rtl'><div id=a style='width:20px'>a</div></div>");
    BOX("a", 80, 0, 20, 10);
    lays("<div style='display:flex;width:100px;justify-content:left'><div id=a style='width:20px'>a</div></div>");
    BOX("a", 0, 0, 20, 10);
    /* row-reverse: still physical */
    lays("<div style='display:flex;flex-direction:row-reverse;width:100px;justify-content:left'><div id=a style='width:20px'>a</div><div id=b style='width:20px'>b</div></div>");
    BOX("a", 20, 0, 20, 10); BOX("b", 0, 0, 20, 10);
    lays("<div style='display:flex;flex-direction:row-reverse;width:100px;justify-content:right;direction:rtl'><div id=a style='width:20px'>a</div><div id=b style='width:20px'>b</div></div>");
    BOX("a", 60, 0, 20, 10); BOX("b", 80, 0, 20, 10);
    /* a column: the main axis is the block axis, left and right are start */
    lays("<div style='display:flex;flex-direction:column;width:100px;height:40px;justify-content:right'><div id=a>a</div></div>");
    BOX("a", 0, 0, 100, 10);
    lays("<div style='display:flex;flex-direction:column;width:100px;height:40px;justify-content:left;direction:rtl'><div id=a>a</div></div>");
    BOX("a", 0, 0, 100, 10);
    /* start follows the writing mode: the top, column-reverse too (Lot 125) */
    lays("<div style='display:flex;flex-direction:column-reverse;width:100px;height:40px;justify-content:right'><div id=a>a</div></div>");
    BOX("a", 0, 0, 100, 10);
    /* grid tracks and items */
    lays("<div style='display:grid;width:100px;grid-template-columns:30px 30px;justify-content:right;justify-items:right'><div id=a>ab</div><div id=b style='justify-self:left'>ab</div></div>");
    BOX("a", 54, 0, 16, 10); BOX("b", 70, 0, 16, 10);
    lays("<div style='display:grid;width:100px;grid-template-columns:30px 30px;justify-content:left;justify-items:left;direction:rtl'><div id=a>ab</div><div id=b style='justify-self:right'>ab</div></div>");
    BOX("a", 30, 0, 16, 10); BOX("b", 14, 0, 16, 10);
    lays("<div style='display:grid;width:100px;grid-template-columns:30px 30px;justify-content:right;direction:rtl'><div id=a>ab</div></div>");
    BOX("a", 70, 0, 30, 10);
    /* safe / unsafe: an overflowing item takes the start (the right in rtl) */
    lays("<div style='display:grid;width:50px;grid-template-columns:50px'><div id=a style='width:70px;justify-self:safe right'>a</div><div id=b style='width:70px;justify-self:unsafe right'>b</div><div id=c style='width:70px;justify-self:right'>c</div></div>");
    BOX("a", 0, 0, 70, 10); BOX("b", -20, 10, 70, 10); BOX("c", 0, 20, 70, 10);
    lays("<div style='display:grid;width:50px;grid-template-columns:50px;direction:rtl'><div id=a style='width:70px;justify-self:safe left'>a</div><div id=b style='width:70px;justify-self:unsafe left'>b</div></div>");
    BOX("a", -20, 0, 70, 10); BOX("b", 0, 10, 70, 10);
    /* align-self of a flex row item stays a block axis value: left refused */
    lays("<div style='display:flex;width:100px;height:40px;align-items:end'><div id=a style='align-self:left;width:20px'>a</div></div>");
    BOX("a", 0, 30, 20, 10);
}

/* --- fit-content --- */
static void test_l127_fit_content(void) {
    /* column flex: start and flex-start no longer take the full width */
    lays("<div style='display:flex;flex-direction:column;width:100px'><div id=a style='align-self:start'>ab</div><div id=b style='align-self:flex-start'>ab</div><div id=c style='align-self:baseline'>ab</div><div id=d>ab</div><div id=e style='align-self:stretch'>ab</div><div id=f style='align-self:last baseline'>ab</div></div>");
    BOX("a", 0, 0, 16, 10); BOX("b", 0, 10, 16, 10); BOX("c", 0, 20, 16, 10); BOX("d", 0, 30, 100, 10);
    BOX("e", 0, 40, 100, 10); BOX("f", 84, 50, 16, 10);   /* last baseline: safe end */
    /* capped by the container: wraps instead of overflowing (end, center,
     * auto margin, start) */
    lays("<div style='display:flex;flex-direction:column;width:40px'><div id=a style='align-self:end'>aaaa bbbb cccc</div><div id=b style='align-self:center'>aaaa bbbb cccc</div><div id=c style='margin-left:auto'>aaaa bbbb cccc</div><div id=d style='align-self:start'>aaaa bbbb</div></div>");
    BOX("a", 0, 0, 40, 30); BOX("b", 0, 30, 40, 30); BOX("c", 0, 60, 40, 30); BOX("d", 0, 90, 40, 20);
    /* min-content wins over the container; edges come off the space */
    lays("<div style='display:flex;flex-direction:column;width:40px'><div id=a style='align-self:center'>aaaaaaaaaaaa</div><div id=b style='align-self:start;padding-left:5px'>aaaa bbbb cccc</div><div id=c style='align-self:end;margin-right:4px'>aaaa bbb</div><div id=d style='align-self:start;width:30px'>ab</div></div>");
    BOX("a", -28, 0, 96, 10); BOX("b", 0, 10, 40, 30); BOX("c", 0, 40, 36, 20); BOX("d", 0, 60, 30, 10);
    lays("<div style='display:flex;flex-direction:column;width:40px'><div id=a style='align-self:end'>aaaa bb</div></div>");
    BOX("a", 0, 0, 40, 20);
    /* an auto margin: content width, pushed to the end */
    lays("<div style='display:flex;flex-direction:column;width:100px'><div id=a style='margin-left:auto'>ab</div><div id=b style='margin-right:auto;align-self:stretch'>ab</div></div>");
    BOX("a", 84, 0, 16, 10); BOX("b", 0, 10, 16, 10);
    /* the height is measured at the width the item is laid out at: its
     * fit-content, its own width, its max-width (was the container's) */
    lays("<div style='display:flex;flex-direction:column;width:100px'><div id=a><div id=c style='aspect-ratio:2'></div></div><div id=e style='align-self:start'><div style='width:60px'></div><div id=f style='aspect-ratio:2'></div></div><div id=g style='align-self:end;width:50%'><div id=h style='aspect-ratio:2'></div></div><div id=i style='width:50px;padding:0 5px;margin-left:10px'><div id=j style='aspect-ratio:2'></div></div></div>");
    BOX("a", 0, 0, 100, 50); BOX("c", 0, 0, 100, 50); BOX("e", 0, 50, 60, 30); BOX("f", 0, 50, 60, 30);
    BOX("g", 50, 80, 50, 25); BOX("h", 50, 80, 50, 25); BOX("i", 10, 105, 60, 25); BOX("j", 15, 105, 50, 25);
    lays("<div style='display:flex;flex-direction:column;width:40px'><div id=x style='align-self:end;max-width:20px'>ab cd</div><div id=y style='max-width:20px'>ab cd</div><div id=z style='min-width:60px;align-self:start'>ab</div><div id=m style='width:30px'>aaaa bbbb</div></div>");
    BOX("x", 20, 0, 20, 20); BOX("y", 0, 20, 20, 20); BOX("z", 0, 40, 60, 10); BOX("m", 0, 50, 30, 20);
    /* a content basis and the automatic minimum use that width too */
    lays("<div style='display:flex;flex-direction:column;width:100px'><div id=a style='flex-basis:content;align-self:start'><div style='width:60px'></div><div style='aspect-ratio:2'></div></div><div id=b style='align-self:start;flex-basis:0px'><div style='width:40px'></div><div style='aspect-ratio:2'></div></div></div>");
    BOX("a", 0, 0, 60, 30); BOX("b", 0, 30, 40, 20);
    lays("<div style='display:flex;flex-direction:column;width:100px'><div id=a style='flex-basis:max-content;align-self:start'><div style='width:60px'></div><div style='aspect-ratio:2'></div></div></div>");
    BOX("a", 0, 0, 60, 30);
    /* a content-keyword basis of a nested flex column is measured one level down */
    lays("<div style='display:flex;flex-direction:column;width:100px'><div id=z style='height:5px'></div><div id=a style='flex-basis:max-content;display:flex;flex-direction:column'><div id=c>ab</div><div id=d>cd</div></div><div id=e>ef</div></div>");
    BOX("z", 0, 0, 100, 5); BOX("a", 0, 5, 100, 20); BOX("c", 0, 5, 100, 10); BOX("d", 0, 15, 100, 10); BOX("e", 0, 25, 100, 10);
    /* a wrapping column: lines as wide as their widest item */
    lays("<div style='display:flex;flex-direction:column;flex-wrap:wrap;width:100px;height:20px;align-content:start'><div id=a>ab</div><div id=b>abcd</div><div id=c>abcdef</div><div id=d style='align-self:end'>x</div></div>");
    BOX("a", 0, 0, 32, 10); BOX("b", 0, 10, 32, 10); BOX("c", 32, 0, 48, 10); BOX("d", 72, 10, 8, 10);
    lays("<div style='display:flex;flex-direction:column;flex-wrap:wrap;width:100px;height:20px'><div id=a>ab</div><div id=b>abcd</div><div id=c>abcdef</div><div id=d>x</div></div>");
    BOX("a", 0, 0, 42, 10); BOX("b", 0, 10, 42, 10); BOX("c", 42, 0, 58, 10); BOX("d", 42, 10, 58, 10);
    lays("<div style='display:flex;flex-direction:column;flex-wrap:wrap;width:40px;height:40px;align-content:start'><div id=a>aaaa bbbb cccc</div></div>");
    BOX("a", 0, 0, 40, 30);
    /* rows are unchanged */
    lays("<div style='display:flex;width:100px;height:30px;align-items:start'><div id=a>ab</div></div>");
    BOX("a", 0, 0, 16, 10);
    /* a row item's automatic minimum still resolves its 50% against the row */
    lays("<div style='display:flex;width:100px'><div id=a style='width:50%'>aaaaaaaa</div><div id=b style='flex:0 0 100px'></div></div>");
    BOX("a", 0, 0, 50, 10); BOX("b", 50, 0, 100, 10);
    /* grid: capped by the area */
    lays("<div style='display:grid;width:80px;grid-template-columns:40px 40px'><div id=a style='justify-self:start'>aaaa bbbb cccc</div><div id=b style='justify-self:end;padding-right:4px'>aaaa bbbb cccc</div><div id=c style='margin-right:auto'>aaaa bbbb cccc</div><div id=d style='justify-self:center'>aaaaaaaaaaaa</div><div id=e style='justify-self:self-end'>ab</div><div id=f>ab</div></div>");
    BOX("a", 0, 0, 40, 30); BOX("b", 40, 0, 40, 30); BOX("c", 0, 30, 40, 30); BOX("d", 40, 30, 96, 30);
    BOX("e", 24, 60, 16, 10); BOX("f", 40, 60, 40, 10);
    lays("<div style='display:grid;width:80px;grid-template-columns:40px 40px'><div id=a style='justify-self:flex-end'>aaaa bb</div><div id=b style='justify-self:left'>aaaa bbbb cccc</div></div>");
    BOX("a", 0, 0, 40, 30); BOX("b", 40, 0, 40, 30);
    lays("<div style='display:grid;width:40px;grid-template-columns:40px'><div id=a style='justify-self:center;margin-left:4px;border-left:2px solid red'>aaaa bbbb cccc</div></div>");
    BOX("a", 4, 0, 36, 30);
    /* stretch and normal fill the area; every position keeps its width */
    lays("<div style='display:grid;width:100px;grid-template-columns:100px'><div id=a style='justify-self:stretch'>ab</div><div id=b style='justify-self:normal'>ab</div><div id=c style='justify-self:self-start'>ab</div><div id=d style='justify-self:right'>ab</div><div id=e style='justify-self:left'>ab</div><div id=f style='justify-self:end'>ab</div><div id=g style='justify-self:flex-start'>ab</div></div>");
    BOX("a", 0, 0, 100, 10); BOX("b", 0, 10, 100, 10); BOX("c", 0, 20, 16, 10); BOX("d", 84, 30, 16, 10);
    BOX("e", 0, 40, 16, 10); BOX("f", 84, 50, 16, 10); BOX("g", 0, 60, 16, 10);
    lays("<div style='display:grid;width:100px;grid-template-columns:100px;justify-items:stretch'><div id=a>ab</div><div id=b style='justify-self:self-end'>ab</div><div id=c style='justify-self:auto'>ab</div></div>");
    BOX("a", 0, 0, 100, 10); BOX("b", 84, 10, 16, 10); BOX("c", 0, 20, 100, 10);
    /* place-items: baseline copies baseline to justify-items: start, own width (was the area) */
    lays("<div style='display:grid;width:100px;grid-template-columns:100px;place-items:baseline'><div id=a>ab</div></div>");
    BOX("a", 0, 0, 16, 10);
    /* an auto margin keeps the item's width, also under stretch */
    lays("<div style='display:grid;width:100px;grid-template-columns:100px'><div id=a style='margin-left:auto'>ab</div><div id=b style='margin-right:auto;justify-self:stretch'>ab</div></div>");
    BOX("a", 84, 0, 16, 10); BOX("b", 0, 10, 16, 10);
}

int main(void) {
    unity_init();
    RUN_TEST(test_l127_parse);
    RUN_TEST(test_l127_self);
    RUN_TEST(test_l127_left_right);
    RUN_TEST(test_l127_fit_content);
    unity_print_results();
    unity_cleanup();
    return unity_stats.tests_failed == 0 ? 0 : 1;
}
