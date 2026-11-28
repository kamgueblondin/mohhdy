/* Lot 132 (US-031): display: contents on table parts.
 *
 * Lot 131 gave display: contents to blocks, flex and grid items, inline runs
 * and table sections, and left two holes: a tr with it handed its cells to
 * the table ("table child refused": the cells were not laid out and not
 * painted), and a td with it handed its content to the row ("table cell
 * refused": the content vanished).
 *
 * CSS Tables 3 9.1.2: what a contents row or cell hands over is wrapped in
 * anonymous boxes: consecutive cells of a table or a section in one
 * anonymous row, consecutive content of a row in one anonymous cell. The
 * engine builds them at the end of we_style (anon_table_parts), as nodes
 * past the generated ones, and only for what came through a contents
 * element: the stray children of a plain table stay refused. The oracle of
 * most tests is the same table with plain rows (or cells with no padding and
 * no border, which is what an anonymous cell has). Values follow
 * docs/lot132_plan.md. */
#include "../../framework/unity.h"
#include <string.h>
#include <stdio.h>
#include "webeng.h"

#define EQ_MSG(e, a, m) do { int e_ = (int)(e), a_ = (int)(a); \
    if (e_ != a_) printf("  %s: expected %d, got %d\n", (m), e_, a_); \
    TEST_ASSERT_EQUAL(e_, a_); } while (0)

static we_doc_t g_doc, g_fresh;
static we_forms_t g_forms, g_forms2;
static we_node_t g_none;

static void load(we_doc_t *d, we_forms_t *f, const char *html) {
    we_forms_init(f);
    we_set_forms(d, f);
    TEST_ASSERT_EQUAL(0, we_parse(d, html, (int)strlen(html)));
    TEST_ASSERT_EQUAL(0, we_style(d));
}

static void lay(we_doc_t *d, we_forms_t *f, const char *html) {
    load(d, f, html);
    TEST_ASSERT_EQUAL(0, we_layout_viewport(d, 640, 480));
}

static void lays(const char *html) { lay(&g_doc, &g_forms, html); }

static const we_node_t *elo(const we_doc_t *d, const char *id) {
    int i = we_find_id(d, id);
    memset(&g_none, 0x5a, sizeof(g_none));
    g_none.h = -1; g_none.w = -1; g_none.x = -1; g_none.y = -1;
    return i >= 0 ? &d->nodes[i] : &g_none;
}
static const we_node_t *el(const char *id) { return elo(&g_doc, id); }

#define XY(id, X, Y, W, H) do { EQ_MSG(X, el(id)->x, id " x"); EQ_MSG(Y, el(id)->y, id " y"); \
    EQ_MSG(W, el(id)->w, id " w"); EQ_MSG(H, el(id)->h, id " h"); } while (0)

static int diag_count(const we_doc_t *d, const char *t) {
    uint32_t i;
    int c = 0;
    for (i = 0; i < d->ndiag; i++) if (strstr(d->diag[i], t)) c++;
    return c;
}

/* x of the painted text item with this text, -1000 when there is none */
static int text_x(const we_doc_t *d, const char *t) {
    uint32_t i;
    for (i = 0; i < d->nitems; i++)
        if (d->items[i].kind == OS_FB_WEB_TEXT && !strcmp(d->items[i].text, t)) return d->items[i].x;
    return -1000;
}

/* page a (with wrappers) against page b (the oracle): every id in the list
 * has the same box, and the display list is the same */
static void same_boxes(const char *a, const char *b, const char *ids) {
    char list[256], *id;
    lay(&g_doc, &g_forms, a);
    lay(&g_fresh, &g_forms2, b);
    strncpy(list, ids, sizeof(list) - 1); list[sizeof(list) - 1] = 0;
    for (id = strtok(list, " "); id; id = strtok(0, " ")) {
        const we_node_t *x = elo(&g_doc, id), *y = elo(&g_fresh, id);
        if (x->x != y->x || x->y != y->y || x->w != y->w || x->h != y->h)
            printf("  %s: %d %d %d %d against %d %d %d %d\n", id, x->x, x->y, x->w, x->h, y->x, y->y, y->w, y->h);
        TEST_ASSERT_TRUE(x->x == y->x && x->y == y->y && x->w == y->w && x->h == y->h);
    }
    if (g_doc.nitems != g_fresh.nitems || memcmp(g_doc.items, g_fresh.items, g_doc.nitems * sizeof(g_doc.items[0])))
        printf("  painted items differ: %d against %d\n", (int)g_doc.nitems, (int)g_fresh.nitems);
    TEST_ASSERT_EQUAL(g_fresh.nitems, g_doc.nitems);
    TEST_ASSERT_EQUAL(0, memcmp(g_doc.items, g_fresh.items, g_doc.nitems * sizeof(g_doc.items[0])));
    EQ_MSG(g_fresh.perf.page_h, g_doc.perf.page_h, "page height");
}

/* the box tree after the splice: every child hangs from its parent, the
 * list of children ends at last, a contents element is a leaf */
static void check_box_tree(const we_doc_t *d, const char *what) {
    int i, c, end;
    for (i = 0; i < (int)d->nnodes; i++) {
        const we_node_t *n = &d->nodes[i];
        end = -1;
        for (c = n->first; c >= 0; c = d->nodes[c].next) {
            if (d->nodes[c].parent != i) printf("  %s: node %d hangs from %d, listed under %d\n", what, c, d->nodes[c].parent, i);
            TEST_ASSERT_EQUAL(i, d->nodes[c].parent);
            end = c;
        }
        if (end != n->last) printf("  %s: node %d list ends at %d, last is %d\n", what, i, end, n->last);
        TEST_ASSERT_EQUAL(end, n->last);
        if (n->contents) { TEST_ASSERT_EQUAL(-1, n->first); }
    }
}

/* a contents row or cell: none of its own box properties applies */
#define RJUNK "background:red;height:50px;border:3px solid blue;margin:9px;padding:20px;width:5px;position:relative;transform:translate(7px,7px)"

static int anon_count(const we_doc_t *d, int display) {
    int i, c = 0;
    for (i = 0; i < (int)d->nnodes; i++)
        if (!d->nodes[i].is_text && !d->nodes[i].tag[0] && !d->nodes[i].id[0] && d->nodes[i].display == display) c++;
    return c;
}

static void test_l132_row_contents(void) {
    /* the cells of a contents row stay laid out and painted, as a row of their own */
    lays("<table id=t border=1><tr id=r style='display:contents;" RJUNK "'><td id=a>a<td id=b>bb</tr><tr><td id=c>c<td id=d>d</tr></table>");
    XY("t", 0, 0, 640, 26);
    XY("a", 1, 1, 319, 12);
    XY("b", 320, 1, 319, 12);
    XY("c", 1, 13, 319, 12);
    XY("d", 320, 13, 319, 12);
    XY("r", 0, 0, 0, 0);
    EQ_MSG(1, el("r")->contents, "row flag");
    EQ_MSG(0, diag_count(&g_doc, "table child refused"), "no refusal");
    EQ_MSG(1, anon_count(&g_doc, WE_DISP_ROW), "one anonymous row");
    EQ_MSG(0, anon_count(&g_doc, WE_DISP_CELL), "no anonymous cell");
    EQ_MSG(1, text_x(&g_doc, "a") != -1000 && text_x(&g_doc, "bb") != -1000, "cells painted");
    same_boxes("<table id=t border=1><tr id=r style='display:contents;" RJUNK "'><td id=a>a<td id=b>bb</tr><tr><td id=c>c<td id=d>d</tr></table>",
               "<table id=t border=1><tr><td id=a>a<td id=b>bb</tr><tr><td id=c>c<td id=d>d</tr></table>", "t a b c d");
    /* in a section, with colspan and rowspan */
    same_boxes("<table id=t border=1><tbody id=s><tr style='display:contents;" RJUNK "'><td id=a rowspan=2>a<td id=b colspan=2>bb</tr>"
               "<tr><td id=c>c<td id=d>d</tr></tbody></table>",
               "<table id=t border=1><tbody id=s><tr><td id=a rowspan=2>a<td id=b colspan=2>bb</tr>"
               "<tr><td id=c>c<td id=d>d</tr></tbody></table>", "t a b c d");
    /* a head, a body and a foot: the anonymous rows keep their section */
    same_boxes("<table id=t border=1><tfoot><tr style='display:contents'><td id=f>f</tr></tfoot><thead><tr style='display:contents;" RJUNK "'><td id=h>h</tr></thead>"
               "<tbody><tr style='display:contents'><td id=a>a<td id=b>b</tr></tbody></table>",
               "<table id=t border=1><tfoot><tr><td id=f>f</tr></tfoot><thead><tr><td id=h>h</tr></thead>"
               "<tbody><tr><td id=a>a<td id=b>b</tr></tbody></table>", "t f h a b");
    /* a caption and a column group are still what they were */
    same_boxes("<table id=t border=1><caption id=k>cap</caption><colgroup><col style='width:100px'></colgroup><tr style='display:contents'><td id=a>a<td id=b>b</tr></table>",
               "<table id=t border=1><caption id=k>cap</caption><colgroup><col style='width:100px'></colgroup><tr><td id=a>a<td id=b>b</tr></table>", "t k a b");
}

static void test_l132_rows_merge(void) {
    /* consecutive contents rows hand their cells to one anonymous row (CSS Tables 3 9.1.2) */
    lays("<table id=t border=1><tr style='display:contents'><td id=a>a<td id=b>b</tr><tr style='display:contents'><td id=c>c<td id=d>d</tr></table>");
    XY("t", 0, 0, 640, 14);
    XY("a", 1, 1, 160, 12);
    XY("b", 161, 1, 160, 12);
    XY("c", 321, 1, 159, 12);
    XY("d", 480, 1, 159, 12);
    EQ_MSG(1, anon_count(&g_doc, WE_DISP_ROW), "one row for both");
    same_boxes("<table id=t border=1><tr style='display:contents'><td id=a>a<td id=b>b</tr><tr style='display:contents'><td id=c>c<td id=d>d</tr></table>",
               "<table id=t border=1><tr><td id=a>a<td id=b>b<td id=c>c<td id=d>d</tr></table>", "t a b c d");
    /* a plain row between them keeps them apart */
    lays("<table id=t border=1><tr style='display:contents'><td id=a>a</tr><tr><td id=b>b</tr><tr style='display:contents'><td id=c>c</tr></table>");
    EQ_MSG(2, anon_count(&g_doc, WE_DISP_ROW), "two anonymous rows");
    same_boxes("<table id=t border=1><tr style='display:contents'><td id=a>a</tr><tr><td id=b>b</tr><tr style='display:contents'><td id=c>c</tr></table>",
               "<table id=t border=1><tr><td id=a>a</tr><tr><td id=b>b</tr><tr><td id=c>c</tr></table>", "t a b c");
    /* a contents row after a plain one is a row of its own */
    same_boxes("<table id=t border=1><tr><td id=a>a<td id=b>b</tr><tr style='display:contents'><td id=c>c</tr></table>",
               "<table id=t border=1><tr><td id=a>a<td id=b>b</tr><tr><td id=c>c</tr></table>", "t a b c");
    /* an empty contents row makes no row */
    same_boxes("<table id=t border=1><tr style='display:contents'></tr><tr><td id=a>a</tr></table>",
               "<table id=t border=1><tr><td id=a>a</tr></table>", "t a");
}

static void test_l132_cell_contents(void) {
    /* the content of a contents cell is an anonymous cell: no padding, no border, no size of its own */
    lays("<table id=t border=1><tr><td id=a>a<td id=x style='display:contents;" RJUNK "'>xy<td id=b>b</tr></table>");
    XY("t", 0, 0, 640, 14);
    XY("a", 1, 1, 213, 12);
    XY("x", 0, 0, 0, 0);
    XY("b", 427, 1, 212, 12);
    EQ_MSG(1, anon_count(&g_doc, WE_DISP_CELL), "one anonymous cell");
    EQ_MSG(0, diag_count(&g_doc, "table cell refused"), "no refusal");
    EQ_MSG(1, text_x(&g_doc, "xy") != -1000, "the content is painted");
    same_boxes("<table id=t border=1><tr><td id=a>a<td id=x style='display:contents;" RJUNK "'>xy<td id=b>b</tr></table>",
               "<table id=t border=1><tr><td id=a>a<td style='padding:0;border:0'>xy<td id=b>b</tr></table>", "t a b");
    /* blocks, inline boxes and text are one cell */
    same_boxes("<table id=t border=1><tr><td style='display:contents'><div id=k style='height:20px'>k</div><i id=m>m</i>n<td id=b>b</tr></table>",
               "<table id=t border=1><tr><td style='padding:0;border:0'><div id=k style='height:20px'>k</div><i id=m>m</i>n<td id=b>b</tr></table>", "t k m b");
    /* a real cell inside the contents cell stays a cell, the text around it makes cells */
    same_boxes("<table id=t border=1><tr><td style='display:contents'>p<div id=a style='display:table-cell'>a</div>q</td><td id=b>b</tr></table>",
               "<table id=t border=1><tr><td style='padding:0;border:0'>p</td><div id=a style='display:table-cell'>a</div><td style='padding:0;border:0'>q<td id=b>b</tr></table>", "t a b");
    /* th keeps being bold through the anonymous cell: it inherits from its DOM parent */
    lays("<table id=t><tr><th style='display:contents'>H</th></tr></table>");
    EQ_MSG(1, text_x(&g_doc, "H") != -1000, "th content painted");
    EQ_MSG(1, el("t")->h > 0, "the table has a row");
    /* an empty contents cell makes no cell */
    same_boxes("<table id=t border=1><tr><td id=a>a<td style='display:contents'></td><td id=b>b</tr></table>",
               "<table id=t border=1><tr><td id=a>a<td id=b>b</tr></table>", "t a b");
}

static void test_l132_row_and_cell_contents(void) {
    /* a contents row holding a contents cell: an anonymous row holding an anonymous cell */
    lays("<table id=t border=1><tr style='display:contents'><td style='display:contents'>x<td id=b>b</tr></table>");
    EQ_MSG(1, anon_count(&g_doc, WE_DISP_ROW), "anonymous row");
    EQ_MSG(1, anon_count(&g_doc, WE_DISP_CELL), "anonymous cell");
    same_boxes("<table id=t border=1><tr style='display:contents'><td style='display:contents'>x<td id=b>b</tr></table>",
               "<table id=t border=1><tr><td style='padding:0;border:0'>x<td id=b>b</tr></table>", "t b");
    /* content of a contents element straight in a table: a row of an anonymous cell */
    same_boxes("<table id=t border=1><div style='display:contents'>x</div><tr><td id=a>a</tr></table>",
               "<table id=t border=1><tr><td style='padding:0;border:0'>x</tr><tr><td id=a>a</tr></table>", "t a");
    /* a contents wrapper holding a contents row, in a contents section */
    same_boxes("<table id=t border=1><tbody style='display:contents;" RJUNK "'><tr style='display:contents'><td id=a>a<td id=b>b</tr></tbody></table>",
               "<table id=t border=1><tr><td id=a>a<td id=b>b</tr></table>", "t a b");
    /* a contents body keeps the list of its parent going: what comes after it is not its content */
    same_boxes("<table id=t border=1><tbody style='display:contents'><tr><td id=a>a</td></tr></tbody><tr style='display:contents'><td id=b>b</td></tr></table>",
               "<table id=t border=1><tr><td id=a>a</td></tr><tr><td id=b>b</td></tr></table>", "t a b");
    same_boxes("<table id=t><tr style='display:contents'><td style='display:contents'>u</td><td id=e0>r</td></tr>"
               "<tbody style='display:contents'><tr style='display:contents'><td id=e1>r0</td></tr></tbody></table>",
               "<table id=t><tr><td style='padding:0;border:0'>u</td><td id=e0>r</td><td id=e1>r0</td></tr></table>", "t e0 e1");
    EQ_MSG(0, diag_count(&g_doc, "table cell refused") + diag_count(&g_doc, "table child refused"), "nothing refused");
    same_boxes("<table id=t border=1><thead style='display:contents'><tr><td id=a>a</td></tr></thead><tr style='display:contents'><td id=b>b</td></tr>"
               "<tfoot style='display:contents'><tr><td id=c>c</td></tr></tfoot><tr style='display:contents'><td id=d>d</td></tr></table>",
               "<table id=t border=1><tr><td id=a>a</td></tr><tr><td id=b>b</td></tr><tr><td id=c>c</td></tr><tr><td id=d>d</td></tr></table>", "t a b c d");
    /* a table inside a contents cell is inside an anonymous cell */
    same_boxes("<table id=o border=1><tr><td style='display:contents'><table id=i border=1><tr><td id=a>a</tr></table><td id=b>b</tr></table>",
               "<table id=o border=1><tr><td style='padding:0;border:0'><table id=i border=1><tr><td id=a>a</tr></table><td id=b>b</tr></table>", "o i a b");
}

static void test_l132_only_what_came_through_contents(void) {
    /* the stray children of a plain table keep being refused, with or without a contents element on the page */
    lays("<table id=t border=1><div id=x>stray</div><tr><td id=a>a</tr></table>");
    EQ_MSG(1, diag_count(&g_doc, "table child refused"), "plain table: refused");
    lays("<div style='display:contents'><i>k</i></div><table id=t border=1><div id=x>stray</div><tr><td id=a>a</tr></table>");
    EQ_MSG(1, diag_count(&g_doc, "table child refused"), "contents elsewhere: still refused");
    EQ_MSG(0, anon_count(&g_doc, WE_DISP_CELL) + anon_count(&g_doc, WE_DISP_ROW), "no anonymous box");
    lays("<div style='display:contents'><i>k</i></div><table id=t border=1><tr><span id=y>stray</span><td id=a>a</tr></table>");
    EQ_MSG(1, diag_count(&g_doc, "table cell refused"), "a plain row: refused");
    EQ_MSG(0, anon_count(&g_doc, WE_DISP_CELL), "no anonymous cell");
}

static void test_l132_valid_children_handed_over(void) {
    /* (no border=1: the cell edges of that hint come from the DOM table parent, not from the box tree)
     * what a contents element hands to a table and is already a table child is not wrapped:
     * a caption, a column group, a head, a body, a foot, a row */
    same_boxes("<table id=t><div style='display:contents'><caption id=k>cap</caption><colgroup><col style='width:60px'></colgroup>"
               "<thead><tr><td id=h>h</td></tr></thead><tbody><tr><td id=a>a<td id=b>b</tr></tbody><tfoot><tr><td id=f>f</td></tr></tfoot></div></table>",
               "<table id=t><caption id=k>cap</caption><colgroup><col style='width:60px'></colgroup>"
               "<thead><tr><td id=h>h</td></tr></thead><tbody><tr><td id=a>a<td id=b>b</tr></tbody><tfoot><tr><td id=f>f</td></tr></tfoot></table>", "t k h a b f");
    EQ_MSG(0, anon_count(&g_doc, WE_DISP_ROW) + anon_count(&g_doc, WE_DISP_CELL), "nothing wrapped");
    /* each one alone, so that each tag test of the valid children counts */
    same_boxes("<table id=t><div style='display:contents'><caption id=k>cap</caption><tr><td id=a>a</td></tr></div></table>",
               "<table id=t><caption id=k>cap</caption><tr><td id=a>a</td></tr></table>", "t k a");
    same_boxes("<table id=t><div style='display:contents'><colgroup><col style='width:90px'></colgroup><tr><td id=a>a</td><td id=b>b</td></tr></div></table>",
               "<table id=t><colgroup><col style='width:90px'></colgroup><tr><td id=a>a</td><td id=b>b</td></tr></table>", "t a b");
    same_boxes("<table id=t><div style='display:contents'><col style='width:90px'><tr><td id=a>a</td><td id=b>b</td></tr></div></table>",
               "<table id=t><col style='width:90px'><tr><td id=a>a</td><td id=b>b</td></tr></table>", "t a b");
    same_boxes("<table id=t><div style='display:contents'><thead><tr><td id=h>h</td></tr></thead><tr><td id=a>a</td></tr></div></table>",
               "<table id=t><thead><tr><td id=h>h</td></tr></thead><tr><td id=a>a</td></tr></table>", "t h a");
    same_boxes("<table id=t><div style='display:contents'><tbody><tr><td id=a>a</td></tr></tbody><tr><td id=b>b</td></tr></div></table>",
               "<table id=t><tbody><tr><td id=a>a</td></tr></tbody><tr><td id=b>b</td></tr></table>", "t a b");
    same_boxes("<table id=t><div style='display:contents'><tfoot><tr><td id=f>f</td></tr></tfoot><tr><td id=a>a</td></tr></div></table>",
               "<table id=t><tfoot><tr><td id=f>f</td></tr></tfoot><tr><td id=a>a</td></tr></table>", "t f a");
    /* plain rows of a contents body stay rows */
    same_boxes("<table id=t><tbody style='display:contents'><tr><td id=a>a</td></tr><tr><td id=b>b</td></tr></tbody></table>",
               "<table id=t><tr><td id=a>a</td></tr><tr><td id=b>b</td></tr></table>", "t a b");
    /* in a row, whatever is not a cell is content, a table part included: it goes into an anonymous cell */
    same_boxes("<table id=t><tr><td style='display:contents'><div id=r style='display:table-row'>r</div></td><td id=b>b</tr></table>",
               "<table id=t><tr><td style='padding:0;border:0'><div id=r style='display:table-row'>r</div></td><td id=b>b</tr></table>", "t b");
    EQ_MSG(1, anon_count(&g_doc, WE_DISP_CELL), "an anonymous cell for the row");
    same_boxes("<table id=t><tr><td style='display:contents'><caption id=k>cap</caption></td><td id=b>b</tr></table>",
               "<table id=t><tr><td style='padding:0;border:0'><caption id=k>cap</caption></td><td id=b>b</tr></table>", "t k b");
}

static void test_l132_skipped_between_cells(void) {
    /* an absolute box or a display none box between two cells does not cut the anonymous row */
    same_boxes("<table id=t border=1><tr style='display:contents'><td id=a>a</td><span id=s style='position:absolute;left:5px;top:5px'>s</span>"
               "<td id=c>c</td></tr></table>",
               "<table id=t border=1><tr><td id=a>a</td><span id=s style='position:absolute;left:5px;top:5px'>s</span><td id=c>c</td></tr></table>", "t a c s");
    lays("<table id=t border=1><tr style='display:contents'><td id=a>a</td><span id=s style='position:absolute;left:5px;top:5px'>s</span>"
         "<td id=c>c</td></tr></table>");
    EQ_MSG(1, anon_count(&g_doc, WE_DISP_ROW), "one row around the absolute box");
    EQ_MSG(el("a")->y, el("c")->y, "same line");
    lays("<table id=t border=1><tr style='display:contents'><td id=a>a</td><span id=s style='display:none'>s</span><td id=c>c</td></tr></table>");
    EQ_MSG(1, anon_count(&g_doc, WE_DISP_ROW), "one row around the hidden box");
    EQ_MSG(el("a")->y, el("c")->y, "same line again");
    /* and content around them is one anonymous cell */
    lays("<table id=t border=1><tr><td style='display:contents'>p<span id=s style='display:none'>s</span><i id=m>q</i></td></tr></table>");
    EQ_MSG(1, anon_count(&g_doc, WE_DISP_CELL), "one cell around the hidden box");
}

static void test_l132_anonymous_style(void) {
    /* an anonymous box matches no rule (the universal one included) and inherits from its box parent */
    const char *css = "<style>*{padding:4px;margin:3px;border:2px solid red;background:#00ff00}"
                      "table{font-size:20px;color:#0000ff;text-align:right;line-height:30px;border:0;padding:0;margin:0;background:none}"
                      "td{border:0;padding:0;margin:0;background:none}tr{padding:6px;border:1px solid #ff0000;margin:2px;background:#ffff00}</style>";
    char a[900], b[900];
    snprintf(a, sizeof a, "%s<table id=t border=1 style='width:300px'><tr><td id=p>p</td><td style='display:contents'>xy</td></tr></table>", css);
    snprintf(b, sizeof b, "%s<table id=t border=1 style='width:300px'><tr><td id=p>p</td><td style='padding:0;border:0;margin:0;background:none'>xy</td></tr></table>", css);
    same_boxes(a, b, "t p");
    EQ_MSG(1, anon_count(&g_doc, WE_DISP_CELL), "anonymous cell");
    {
        int i;
        for (i = 0; i < (int)g_doc.nnodes; i++)
            if (!g_doc.nodes[i].is_text && !g_doc.nodes[i].tag[0] && g_doc.nodes[i].display == WE_DISP_CELL) {
                EQ_MSG(0, g_doc.nodes[i].padding[0] + g_doc.nodes[i].border + g_doc.nodes[i].margin[0], "no edges from the universal rule");
                EQ_MSG(0, g_doc.nodes[i].has_bg, "no background");
                EQ_MSG(WE_ALIGN_RIGHT, g_doc.nodes[i].align, "text-align inherited from the table");
                EQ_MSG(0x0000ff, g_doc.nodes[i].color & 0xffffff, "color inherited from the table");
            }
    }
}

static void test_l132_stale_links(void) {
    /* a first pass that generated a box leaves its DOM link in the slot the anonymous box takes in the next pass:
     * the anonymous cell is new, whatever the stale link says */
    const char *css = "<style>.c{container-type:inline-size}@container (min-width:500px){#t::before{content:'G'}}</style>";
    char html[900];
    snprintf(html, sizeof html, "%s<div class=c><table id=t border=1><div style='display:contents'>x</div></table></div>", css);
    load(&g_doc, &g_forms, html);
    TEST_ASSERT_EQUAL(0, we_layout_viewport(&g_doc, 600, 480));
    TEST_ASSERT_EQUAL(0, we_layout_viewport(&g_doc, 284, 480));
    load(&g_fresh, &g_forms2, html);
    TEST_ASSERT_EQUAL(0, we_layout_viewport(&g_fresh, 284, 480));
    EQ_MSG(1, anon_count(&g_fresh, WE_DISP_ROW), "fresh: one anonymous row");
    EQ_MSG(1, anon_count(&g_doc, WE_DISP_ROW), "after a wide pass: one anonymous row");
    EQ_MSG(1, anon_count(&g_doc, WE_DISP_CELL), "after a wide pass: one anonymous cell");
    TEST_ASSERT_EQUAL(g_fresh.nnodes, g_doc.nnodes);
    TEST_ASSERT_EQUAL(0, memcmp(g_doc.items, g_fresh.items, g_doc.nitems * sizeof(g_doc.items[0])));
    check_box_tree(&g_doc, "stale links");
}

static void test_l132_second_pass(void) {
    const char *html = "<table id=t border=1><tr style='display:contents'><td id=a>a<td style='display:contents'>x<td id=b>b</tr>"
                       "<tr style='display:contents'><td id=c>c</tr></table>";
    int nodes, rows, cells;
    load(&g_doc, &g_forms, html);
    nodes = (int)g_doc.nnodes;
    rows = anon_count(&g_doc, WE_DISP_ROW); cells = anon_count(&g_doc, WE_DISP_CELL);
    EQ_MSG(1, rows, "one row");
    EQ_MSG(1, cells, "one cell");
    check_box_tree(&g_doc, "first pass");
    TEST_ASSERT_EQUAL(0, we_style(&g_doc));
    EQ_MSG(nodes, g_doc.nnodes, "same node count after a second pass");
    EQ_MSG(rows, anon_count(&g_doc, WE_DISP_ROW), "same anonymous rows");
    EQ_MSG(cells, anon_count(&g_doc, WE_DISP_CELL), "same anonymous cells");
    check_box_tree(&g_doc, "second pass");
    TEST_ASSERT_EQUAL(0, we_style(&g_doc));
    EQ_MSG(nodes, g_doc.nnodes, "and a third");
    TEST_ASSERT_EQUAL(0, we_layout_viewport(&g_doc, 640, 480));
    lay(&g_fresh, &g_forms2, html);
    TEST_ASSERT_EQUAL(g_fresh.nitems, g_doc.nitems);
    TEST_ASSERT_EQUAL(0, memcmp(g_doc.items, g_fresh.items, g_doc.nitems * sizeof(g_doc.items[0])));
    /* a layout, then a style pass, then a layout: the first layout leaves nothing behind */
    lays(html);
    TEST_ASSERT_EQUAL(0, we_style(&g_doc));
    TEST_ASSERT_EQUAL(0, we_layout_viewport(&g_doc, 640, 480));
    TEST_ASSERT_EQUAL(g_fresh.nitems, g_doc.nitems);
    TEST_ASSERT_EQUAL(0, memcmp(g_doc.items, g_fresh.items, g_doc.nitems * sizeof(g_doc.items[0])));
}

static void test_l132_node_budget(void) {
    static char html[4096];
    int k, rows, full = -1, last_two_nodes = 0;
    /* two runs of strays (two contents rows apart by a plain row); one more empty paragraph each time, until
     * the table of nodes has room for no anonymous row, then for one, then for none */
    for (k = 1; k < WE_MAX_NODES; k++) {
        int len = snprintf(html, sizeof html, "<table><tr style='display:contents'><td id=a>a</td><td id=b>b</td></tr><tr><td id=c>c</td></tr>"
                           "<tr style='display:contents'><td id=d>d</td></tr></table>"), j;
        for (j = 0; j < k; j++) len += snprintf(html + len, sizeof html - len, "<p></p>");
        load(&g_doc, &g_forms, html);
        rows = anon_count(&g_doc, WE_DISP_ROW);
        EQ_MSG(1, (int)g_doc.nnodes <= WE_MAX_NODES, "never past the table");
        if (rows == 2) { EQ_MSG(0, diag_count(&g_doc, "table anonymous box budget"), "room: no diagnostic"); last_two_nodes = (int)g_doc.nnodes; continue; }
        if (rows == 1) {
            EQ_MSG(WE_MAX_NODES, g_doc.nnodes, "room for one row: the table of nodes is full");
            EQ_MSG(1, diag_count(&g_doc, "table anonymous box budget"), "room for one row: one diagnostic");
            continue;
        }
        full = k;
        break;
    }
    TEST_ASSERT_TRUE(full > 0);
    EQ_MSG(WE_MAX_NODES, last_two_nodes, "the last page with two rows filled the table of nodes");
    EQ_MSG(2, diag_count(&g_doc, "table anonymous box budget"), "one diagnostic for each run");
    TEST_ASSERT_EQUAL(0, we_layout_viewport(&g_doc, 640, 480));
    EQ_MSG(3, diag_count(&g_doc, "table child refused"), "the three cells stay refused, as before the lot");
    XY("a", 0, 0, 0, 0);
    XY("d", 0, 0, 0, 0);
    EQ_MSG(1, el("c")->w > 0, "the plain row is laid out");
    check_box_tree(&g_doc, "budget");
}

static void test_l132_restyle(void) {
    /* a restyle (container query) goes through we_style again: the same boxes as a fresh load */
    const char *css = "<style>.c{container-type:inline-size}@container (min-width:500px){#w{display:contents}}</style>";
    char html[1024];
    snprintf(html, sizeof(html), "%s<div class=c><table id=t border=1><tr id=w><td id=a>a<td id=b>b</tr><tr><td id=c>c</tr></table></div>", css);
    load(&g_fresh, &g_forms2, html);
    TEST_ASSERT_EQUAL(0, we_layout_viewport(&g_fresh, 284, 480));
    load(&g_doc, &g_forms, html);
    TEST_ASSERT_EQUAL(0, we_layout_viewport(&g_doc, 600, 480));
    EQ_MSG(1, anon_count(&g_doc, WE_DISP_ROW), "wide: the row is contents, one anonymous row");
    EQ_MSG(1, el("w")->contents, "wide: flag");
    TEST_ASSERT_EQUAL(0, we_layout_viewport(&g_doc, 284, 480));
    EQ_MSG(0, anon_count(&g_doc, WE_DISP_ROW), "narrow: plain rows, the anonymous row is gone");
    EQ_MSG(0, el("w")->contents, "narrow: a row again");
    XY("w", 1, 1, 282, 12);
    TEST_ASSERT_EQUAL(g_fresh.nnodes, g_doc.nnodes);
    TEST_ASSERT_EQUAL(0, memcmp(g_doc.nodes, g_fresh.nodes, g_doc.nnodes * sizeof(we_node_t)));
    TEST_ASSERT_EQUAL(0, memcmp(g_doc.items, g_fresh.items, g_doc.nitems * sizeof(g_doc.items[0])));
    TEST_ASSERT_EQUAL(0, we_layout_viewport(&g_doc, 600, 480));
    EQ_MSG(1, anon_count(&g_doc, WE_DISP_ROW), "wide again: one anonymous row");
}

static void test_l132_display_table_parts(void) {
    /* the same on elements that are table parts by display only */
    same_boxes("<div id=t style='display:table;width:300px'><div style='display:contents'><div id=a style='display:table-cell'>a</div>"
               "<div id=b style='display:table-cell'>b</div></div></div>",
               "<div id=t style='display:table;width:300px'><div style='display:table-row'><div id=a style='display:table-cell'>a</div>"
               "<div id=b style='display:table-cell'>b</div></div></div>", "t a b");
}

static void test_l132_box_tree(void) {
    lays("<table id=t border=1><thead style='display:contents'><tr style='display:contents'><th>H</th></tr></thead>"
         "<tbody><tr style='display:contents'><td>1<td style='display:contents'>2<td>3</tr><tr style='display:contents'><td>4</tr></tbody>"
         "<tfoot><tr><td style='display:contents'>f</td></tr></tfoot></table>");
    check_box_tree(&g_doc, "mixed table");
    EQ_MSG(0, diag_count(&g_doc, "table child refused") + diag_count(&g_doc, "table cell refused"), "nothing refused");
}

int main(void) {
    unity_init();
    RUN_TEST(test_l132_row_contents);
    RUN_TEST(test_l132_rows_merge);
    RUN_TEST(test_l132_cell_contents);
    RUN_TEST(test_l132_row_and_cell_contents);
    RUN_TEST(test_l132_only_what_came_through_contents);
    RUN_TEST(test_l132_valid_children_handed_over);
    RUN_TEST(test_l132_skipped_between_cells);
    RUN_TEST(test_l132_anonymous_style);
    RUN_TEST(test_l132_stale_links);
    RUN_TEST(test_l132_second_pass);
    RUN_TEST(test_l132_node_budget);
    RUN_TEST(test_l132_restyle);
    RUN_TEST(test_l132_display_table_parts);
    RUN_TEST(test_l132_box_tree);
    unity_print_results();
    unity_cleanup();
    return unity_stats.tests_failed == 0 ? 0 : 1;
}
