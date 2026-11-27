/* Lot 131 (US-031): display: contents.
 *
 * The engine refused it ("display refused"): an element meant to vanish
 * stayed a box, with its own margin, padding, border and background, and
 * its children stayed inside it: a wrapper in a flex row was one flex item,
 * in a grid one cell, in a paragraph an inline box with edges.
 *
 * Now a contents element generates no box and its children are the box
 * children of its parent (CSS Display 3, 2.5). The engine keeps the
 * element as a display none node flagged contents and, at the end of
 * we_style, hangs its children from the nearest ancestor that has a box
 * (splice_contents); the next we_style restores the DOM links first. The
 * oracle of most tests is the same page without the wrapper element (or
 * with a plain span in inline flow): same boxes, same painted items.
 * Values follow docs/lot131_plan.md. */
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

/* a wrapper's own box properties: none of them applies */
#define JUNK "background:red;padding:20px;margin:9px;border:3px solid blue;width:5px;height:99px"

static void test_l131_flex_wrapper(void) {
    lays("<div id=f style='display:flex;width:300px'><div id=w style='display:contents;" JUNK "'>"
         "<div id=a style='width:50px;height:10px'>x</div><div id=b style='width:60px;height:10px'>y</div></div>"
         "<div id=c style='width:30px;height:10px'>z</div></div>");
    XY("f", 0, 0, 300, 10);
    XY("a", 0, 0, 50, 10);
    XY("b", 50, 0, 60, 10);
    XY("c", 110, 0, 30, 10);   /* three flex items, not two */
    XY("w", 0, 0, 0, 0);
    EQ_MSG(WE_DISP_NONE, el("w")->display, "wrapper display");
    EQ_MSG(1, el("w")->contents, "wrapper flag");
    EQ_MSG(0, diag_count(&g_doc, "display refused"), "no refusal");
    same_boxes("<div id=f style='display:flex;width:300px'><div id=w style='display:contents;" JUNK "'>"
               "<div id=a style='width:50px;height:10px'>x</div><div id=b style='width:60px;height:10px'>y</div></div>"
               "<div id=c style='width:30px;height:10px'>z</div></div>",
               "<div id=f style='display:flex;width:300px'><div id=a style='width:50px;height:10px'>x</div>"
               "<div id=b style='width:60px;height:10px'>y</div><div id=c style='width:30px;height:10px'>z</div></div>", "f a b c");
    /* flex properties of the wrapper do not make it an item: flex:1 and order */
    same_boxes("<div id=f style='display:flex;width:300px'><div style='display:contents;flex:1;order:5'>"
               "<div id=a style='width:50px;height:10px'>x</div></div><div id=c style='width:30px;height:10px'>z</div></div>",
               "<div id=f style='display:flex;width:300px'><div id=a style='width:50px;height:10px'>x</div>"
               "<div id=c style='width:30px;height:10px'>z</div></div>", "f a c");
    /* a column wrap: the children are the items of the container's lines */
    same_boxes("<div id=f style='display:flex;flex-direction:column;flex-wrap:wrap;height:25px;gap:3px'>"
               "<div style='display:contents'><div id=a style='width:20px;height:10px'></div><div id=b style='width:20px;height:10px'></div></div>"
               "<div id=c style='width:20px;height:10px'></div></div>",
               "<div id=f style='display:flex;flex-direction:column;flex-wrap:wrap;height:25px;gap:3px'>"
               "<div id=a style='width:20px;height:10px'></div><div id=b style='width:20px;height:10px'></div>"
               "<div id=c style='width:20px;height:10px'></div></div>", "f a b c");
}

static void test_l131_grid_wrapper(void) {
    lays("<div id=g style='display:grid;grid-template-columns:100px 100px;width:300px'>"
         "<div id=w style='display:contents;padding:9px;border:2px solid red'><div id=a>1</div><div id=b>2</div></div><div id=c>3</div></div>");
    XY("g", 0, 0, 300, 20);
    XY("a", 0, 0, 100, 10);
    XY("b", 100, 0, 100, 10);
    XY("c", 0, 10, 100, 10);   /* the wrapper took one cell before */
    same_boxes("<div id=g style='display:grid;grid-template-columns:50px 1fr 30px;gap:4px;width:200px'>"
               "<div style='display:contents;grid-column:3;" JUNK "'><div id=a style='height:10px'>1</div><div id=b style='height:12px'>2</div></div>"
               "<div id=c style='grid-column:1 / span 2;height:10px'>3</div></div>",
               "<div id=g style='display:grid;grid-template-columns:50px 1fr 30px;gap:4px;width:200px'>"
               "<div id=a style='height:10px'>1</div><div id=b style='height:12px'>2</div>"
               "<div id=c style='grid-column:1 / span 2;height:10px'>3</div></div>", "g a b c");
}

static void test_l131_inline_flow(void) {
    lays("<div id=p style='width:200px'><b id=b0>x y</b> <span id=s style='display:contents;padding:9px;border:1px solid red'>ab <i id=i>cd</i></span> ef</div>");
    XY("p", 0, 0, 200, 10);
    EQ_MSG(0, text_x(&g_doc, "x y"), "x y");
    EQ_MSG(32, text_x(&g_doc, "ab"), "ab: no padding or border of the span");
    EQ_MSG(56, text_x(&g_doc, "cd"), "cd");
    EQ_MSG(80, text_x(&g_doc, "ef"), "ef");
    /* the plain span is the oracle, white space around it included */
    same_boxes("<div id=p style='width:200px'><b id=b0>x y</b> <span style='display:contents;margin:9px;padding:9px;border:1px solid red'>ab <i>cd</i></span> ef</div>",
               "<div id=p style='width:200px'><b id=b0>x y</b> <span>ab <i>cd</i></span> ef</div>", "p");
    same_boxes("<div id=p style='width:90px'>aa bb <span style='display:contents'><span style='display:contents'>cc dd </span>ee ff </span>gg</div>",
               "<div id=p style='width:90px'>aa bb <span><span>cc dd </span>ee ff </span>gg</div>", "p");
    /* no space around the wrapper stays no space */
    same_boxes("<div id=p style='width:200px'>a<span style='display:contents'>b</span>c</div>",
               "<div id=p style='width:200px'>a<span>b</span>c</div>", "p");
}

static void test_l131_containing_blocks(void) {
    /* percentages resolve against the grandparent, a relative wrapper is not a containing block */
    lays("<div id=p style='width:200px;height:40px'><div id=w style='display:contents;height:10px'><div id=a style='height:50%;width:25%'>x</div></div></div>");
    XY("a", 0, 0, 50, 20);
    lays("<div id=r style='position:relative;width:200px;height:60px;padding:5px'><div id=w style='display:contents;position:relative;left:30px;top:7px'>"
         "<div id=a style='position:absolute;right:0;bottom:0;width:20px;height:10px'></div></div></div>");
    XY("r", 0, 0, 210, 70);
    XY("a", 190, 60, 20, 10);
    /* nested wrappers and a following sibling */
    lays("<div id=o style='width:200px'><div id=w1 style='display:contents'><div id=w2 style='display:contents;border:1px solid red'>"
         "<div id=a style='height:10px'>x</div></div><div id=b style='height:10px'>y</div></div></div>");
    XY("o", 0, 0, 200, 20);
    XY("a", 0, 0, 200, 10);
    XY("b", 0, 10, 200, 10);
    /* block flow: the wrapper's own margin, padding and size do not apply */
    same_boxes("<div id=o style='width:200px'><div style='display:contents;" JUNK "'><div id=a style='height:10px'>x</div><div id=b style='height:10px'>y</div></div></div>",
               "<div id=o style='width:200px'><div id=a style='height:10px'>x</div><div id=b style='height:10px'>y</div></div>", "o a b");
    /* a contents element at the top of a fragment: its children hang from the document */
    same_boxes("<div style='display:contents;padding:30px'><div id=a style='height:10px'>x</div></div><div id=b style='height:10px'>y</div>",
               "<div id=a style='height:10px'>x</div><div id=b style='height:10px'>y</div>", "a b");
}

static void test_l131_hidden_and_boxes_back(void) {
    /* inside display none the children stay hidden, and what follows is laid out */
    lays("<div id=n style='display:none'><div id=w style='display:contents'><div id=a>a</div></div></div><div id=z style='height:10px'>z</div>");
    EQ_MSG(WE_DISP_NONE, el("a")->display, "a hidden");
    XY("z", 0, 0, 640, 10);
    lays("<div id=n style='display:none'><div id=w1 style='display:contents'><div id=w2 style='display:contents'><div id=a>a</div></div></div></div><div id=z style='height:10px'>z</div>");
    EQ_MSG(WE_DISP_NONE, el("a")->display, "a hidden two levels down");
    /* a display none child of a contents element stays hidden */
    lays("<div id=o style='width:100px'><div style='display:contents;padding:5px'><div id=h style='display:none;height:30px'>h</div><div id=a style='height:10px'>a</div></div></div>");
    XY("a", 0, 0, 100, 10);
    XY("o", 0, 0, 100, 10);
    /* positive control: display:block with the same properties keeps its box */
    lays("<div id=o style='width:200px'><div id=w style='display:block;padding:20px;border:3px solid blue'><div id=a style='height:10px'>x</div></div></div>");
    XY("w", 0, 0, 200, 56);
    XY("a", 23, 23, 154, 10);
    EQ_MSG(0, el("w")->contents, "block has no flag");
}

static void test_l131_replaced_and_root(void) {
    /* replaced elements and controls have nothing to hand over: none */
    lays("<div id=o style='width:200px'><img id=i style='display:contents' width=20 height=10><input id=n style='display:contents'>"
         "<br id=br style='display:contents'><div id=a style='height:10px'>a</div></div>");
    EQ_MSG(WE_DISP_NONE, el("i")->display, "img");
    EQ_MSG(0, el("i")->contents, "img flag");
    EQ_MSG(WE_DISP_NONE, el("n")->display, "input");
    EQ_MSG(0, el("n")->contents, "input flag");
    EQ_MSG(0, el("br")->contents, "br flag");
    XY("a", 0, 0, 200, 10);
    EQ_MSG(0, diag_count(&g_doc, "display"), "no diagnostic");
    /* the root element keeps refusing it, with its own diagnostic */
    lays("<html style='display:contents'><body style='margin:0'><div id=a style='height:10px'>a</div></body></html>");
    EQ_MSG(1, diag_count(&g_doc, "contents refused on the root"), "root refused");
    EQ_MSG(0, diag_count(&g_doc, "display refused"), "not the old text");
    XY("a", 0, 0, 640, 10);
    /* only the root: an html element met inside the body is an ordinary element */
    lays("<div id=o style='width:100px'><html id=h style='display:contents'><div id=a style='height:10px'>a</div></html></div>");
    EQ_MSG(1, el("h")->contents, "nested html");
    EQ_MSG(0, diag_count(&g_doc, "contents refused"), "nested html not refused");
    XY("a", 0, 0, 100, 10);
    /* a value that is not a display keeps contents (the refusal keeps the previous value) */
    lays("<div id=o style='width:100px'><div id=w style='display:contents;display:bogus'><div id=a style='height:10px'>a</div></div></div>");
    EQ_MSG(1, el("w")->contents, "kept");
    EQ_MSG(1, diag_count(&g_doc, "display refused"), "bogus refused");
    XY("o", 0, 0, 100, 10);
}

static void test_l131_cascade_and_restyle(void) {
    /* a later display wins: a box again, flag cleared */
    lays("<div id=o style='width:200px'><div id=w style='display:contents;display:block;padding:20px'><div id=a style='height:10px'>a</div></div></div>");
    EQ_MSG(0, el("w")->contents, "block after contents");
    XY("w", 0, 0, 200, 50);
    lays("<div id=o style='width:200px'><div id=w style='display:contents;display:none;padding:20px'><div id=a style='height:10px'>a</div></div></div>");
    EQ_MSG(0, el("w")->contents, "none after contents");
    EQ_MSG(WE_DISP_NONE, el("a")->display, "children hidden");
    lays("<div id=o style='width:200px'><div id=w style='display:block;display:contents;padding:20px'><div id=a style='height:10px'>a</div></div></div>");
    EQ_MSG(1, el("w")->contents, "contents after block");
    XY("o", 0, 0, 200, 10);
    lays("<style>.c{display:contents}#w{display:block;padding:20px}</style><div id=o style='width:200px'><div id=w class=c><div id=a style='height:10px'>a</div></div></div>");
    EQ_MSG(0, el("w")->contents, "id rule wins over the class");
    lays("<style>.c{display:contents}#w{display:block;padding:20px}</style><div id=o style='width:200px'><div id=w class=c style='display:contents'><div id=a style='height:10px'>a</div></div></div>");
    EQ_MSG(1, el("w")->contents, "style attribute wins");
    lays("<style>#w{display:contents !important}</style><div id=o style='width:200px'><div id=w style='display:block;padding:20px'><div id=a style='height:10px'>a</div></div></div>");
    EQ_MSG(1, el("w")->contents, "important");
    XY("o", 0, 0, 200, 10);
}

/* we_style again (the page scripts, a container query pass): the DOM links come
 * back first, so the second pass and the second splice see the DOM */
static void test_l131_links_restored(void) {
    const char *html = "<style>.c{display:contents}</style><div id=o style='width:200px'><div id=w class=c><div id=a>a</div><div id=b>b</div></div><div id=z>z</div></div>";
    load(&g_doc, &g_forms, html);
    EQ_MSG(1, el("w")->contents, "contents");
    EQ_MSG(1, g_doc.spliced, "spliced after the style pass");
    EQ_MSG(el("o") - g_doc.nodes, g_doc.nodes[el("a") - g_doc.nodes].parent, "a hangs from o in the box tree");
    EQ_MSG(el("w") - g_doc.nodes, g_doc.dom_link[el("a") - g_doc.nodes][0], "the DOM parent is kept");
    EQ_MSG(-1, el("w")->first, "a contents element is a leaf of the box tree: first");
    EQ_MSG(-1, el("w")->last, "last");
    EQ_MSG(el("w") - g_doc.nodes, g_doc.nodes[el("o") - g_doc.nodes].first, "o starts with the contents element");
    EQ_MSG(el("z") - g_doc.nodes, g_doc.nodes[el("o") - g_doc.nodes].last, "o ends with z");
    TEST_ASSERT_EQUAL(0, we_style(&g_doc));
    EQ_MSG(1, g_doc.spliced, "spliced again");
    EQ_MSG(el("o") - g_doc.nodes, g_doc.nodes[el("a") - g_doc.nodes].parent, "same box tree after a second pass");
    EQ_MSG(el("b") - g_doc.nodes, g_doc.nodes[el("a") - g_doc.nodes].next, "b follows a");
    TEST_ASSERT_EQUAL(0, we_layout_viewport(&g_doc, 640, 480));
    XY("a", 0, 0, 200, 10);
    XY("b", 0, 10, 200, 10);
    XY("z", 0, 20, 200, 10);
    /* the same with and without a second pass: same bytes and items */
    lay(&g_fresh, &g_forms2, html);
    TEST_ASSERT_EQUAL(g_fresh.nitems, g_doc.nitems);
    TEST_ASSERT_EQUAL(0, memcmp(g_doc.items, g_fresh.items, g_doc.nitems * sizeof(g_doc.items[0])));
    /* a page with no contents element is never spliced */
    lays("<div id=o style='width:200px'><div id=w><div id=a>a</div></div></div>");
    EQ_MSG(0, g_doc.spliced, "no contents, no splice");
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

static void test_l131_box_tree_consistent(void) {
    static const char *pages[] = {
        "<style>.c{display:contents}</style><div id=o><div id=z>z</div><div id=w class=c><div id=a>a</div><div id=b>b</div></div></div>",
        "<style>.c{display:contents}</style><div id=o><div id=w class=c><div id=a>a</div><div class=c id=v><div id=b>b</div></div></div></div>",
        "<style>.c{display:contents}</style><div id=o><div id=z>z</div><div id=w class=c></div></div>",
        "<style>.c{display:contents}</style><div id=o><div id=w class=c><div id=v class=c><div id=a>a</div></div></div><div id=z>z</div></div>",
        "<style>.c{display:contents}</style><p id=o>a <span class=c id=w>b <i class=c id=v>c</i></span></p>",
        "<style>.c{display:contents}.c::after{content:'Y'}</style><div id=o><div id=w class=c><div id=a>a</div></div></div>",
    };
    int k;
    for (k = 0; k < (int)(sizeof pages / sizeof pages[0]); k++) {
        char what[24];
        snprintf(what, sizeof what, "page %d", k);
        lays(pages[k]);
        TEST_ASSERT_EQUAL(1, g_doc.spliced);
        check_box_tree(&g_doc, what);
        TEST_ASSERT_EQUAL(0, we_style(&g_doc));
        check_box_tree(&g_doc, what);
    }
    /* o ends with the last child of the contents element that ends it */
    lays(pages[0]);
    EQ_MSG(el("b") - g_doc.nodes, el("o")->last, "o ends with b");
    EQ_MSG(el("w") - g_doc.nodes, el("z")->next, "w follows z");
    lays(pages[2]);
    EQ_MSG(el("w") - g_doc.nodes, el("o")->last, "an empty contents element is the last child");
    /* before the splice, and with no contents element, the DOM lists are the box lists */
    lays("<div id=o><div id=a>a</div><div id=b>b</div></div>");
    check_box_tree(&g_doc, "no contents");
}

static int text_index(const we_doc_t *d, const char *t) {
    uint32_t i;
    for (i = 0; i < d->nitems; i++)
        if (d->items[i].kind == OS_FB_WEB_TEXT && !strcmp(d->items[i].text, t)) return (int)i;
    return -1;
}

/* paint order: an absolute box with z-index 0 is not a reason to sort the
 * display list by stacking key (the float still paints before the inline text) */
static void test_l131_stacking_zero_absolute(void) {
    lays("<div style='position:absolute;z-index:0'>x</div><div style='float:left'>f</div>y");
    EQ_MSG(1, text_index(&g_doc, "f") >= 0 && text_index(&g_doc, "f") < text_index(&g_doc, "y"), "float before the inline text");
    EQ_MSG(1, text_index(&g_doc, "y") < text_index(&g_doc, "x"), "positioned after the inline text");
}

/* a full node table: the splice reads and writes the DOM links of the nodes
 * that exist and nothing past them (the bytes after the document stay) */
static struct { we_doc_t d; unsigned char canary[32]; } g_edge;

static void test_l131_full_node_table(void) {
    static char html[16384];
    int i, k, len = snprintf(html, sizeof html, "<style>.c{display:contents}</style><div id=o style='width:100px'><div id=w class=c><div id=a style='height:10px'>a</div></div>");
    /* ten texts of the longest size (511) fill the pool past the byte that the
     * contents flag of a node past the table would be read from */
    for (k = 0; k < 10; k++) {
        len += snprintf(html + len, sizeof html - len, "<p>");
        for (i = 0; i < 511; i++) html[len++] = 'y';
        len += snprintf(html + len, sizeof html - len, "</p>");
    }
    for (i = 0; i < 67; i++) len += snprintf(html + len, sizeof html - len, "<b>x</b>");
    memset(g_edge.canary, 0xa5, sizeof g_edge.canary);
    lay(&g_edge.d, &g_forms, html);
    EQ_MSG(WE_MAX_NODES, g_edge.d.nnodes, "the node table is full");
    EQ_MSG(1, g_edge.d.spliced, "spliced");
    EQ_MSG(1, g_edge.d.nodes[we_find_id(&g_edge.d, "w")].contents, "contents");
    check_box_tree(&g_edge.d, "full table");
    {
        int texts = 0;
        for (k = 0; k < (int)g_edge.d.nnodes; k++) {
            const we_node_t *tx = &g_edge.d.nodes[k];
            if (!tx->is_text || tx->text_len != 511) continue;
            texts++;
            for (i = 0; i < tx->text_len; i++) if (g_edge.d.pool[tx->text_off + i] != 'y') break;
            EQ_MSG(511, i, "the text is intact in the pool");
        }
        EQ_MSG(10, texts, "ten long texts");
    }
    for (i = 0; i < (int)sizeof g_edge.canary; i++) EQ_MSG(0xa5, g_edge.canary[i], "nothing written past the document");
    /* a second pass over a full table leaves the document as the first pass made it */
    load(&g_doc, &g_forms, html);
    load(&g_edge.d, &g_forms, html);
    TEST_ASSERT_EQUAL(0, we_style(&g_edge.d));
    TEST_ASSERT_EQUAL(0, memcmp(&g_doc.nodes, &g_edge.d.nodes, sizeof g_doc.nodes));
    TEST_ASSERT_EQUAL(0, memcmp(&g_doc.dom_link, &g_edge.d.dom_link, sizeof g_doc.dom_link));
    EQ_MSG(g_doc.spliced, g_edge.d.spliced, "spliced");
    TEST_ASSERT_EQUAL(0, memcmp(g_doc.pool, g_edge.d.pool, sizeof g_doc.pool));   /* what follows the node table */
    for (i = 0; i < (int)sizeof g_edge.canary; i++) EQ_MSG(0xa5, g_edge.canary[i], "nothing written past the document (second pass)");
}

static void test_l131_generated_and_counters(void) {
    /* ::before and ::after of a contents element are boxes of its parent */
    lays("<style>.w::before{content:'X'}.w::after{content:'Y'}</style><div id=p style='display:flex;width:100px'>"
         "<div class=w style='display:contents;padding:4px'><i id=a>a</i></div></div>");
    EQ_MSG(0, text_x(&g_doc, "X"), "before");
    EQ_MSG(8, text_x(&g_doc, "a"), "a");
    EQ_MSG(16, text_x(&g_doc, "Y"), "after");
    same_boxes("<style>.w::before{content:'X'}.w::after{content:'Y'}</style><div id=p style='display:flex;width:100px'><div class=w style='display:contents;padding:4px'><i id=a>a</i></div></div>",
               "<style>.w::before{content:'X'}.w::after{content:'Y'}</style><div id=p style='display:flex;width:100px'><i class=w id=a>a</i></div>", "p");
    /* a second style pass drops the generated boxes of the first (they were hung from the box parent) */
    {
        const char *gp = "<style>.w::before{content:'X'}.w::after{content:'Y'}</style><div id=p style='display:flex;width:100px'>"
                         "<div class=w style='display:contents'><i id=a>a</i></div></div>";
        int nodes;
        load(&g_doc, &g_forms, gp);
        nodes = (int)g_doc.nnodes;
        TEST_ASSERT_EQUAL(0, we_style(&g_doc));
        EQ_MSG(nodes, g_doc.nnodes, "same node count after a second pass");
        TEST_ASSERT_EQUAL(0, we_style(&g_doc));
        EQ_MSG(nodes, g_doc.nnodes, "and a third");
        TEST_ASSERT_EQUAL(0, we_layout_viewport(&g_doc, 640, 480));
        lay(&g_fresh, &g_forms2, gp);
        TEST_ASSERT_EQUAL(g_fresh.nitems, g_doc.nitems);
        TEST_ASSERT_EQUAL(0, memcmp(g_doc.items, g_fresh.items, g_doc.nitems * sizeof(g_doc.items[0])));
    }
    /* a counter set on a contents element counts */
    lays("<style>.k{counter-increment:n 3}.s::before{content:counter(n)}</style><div style='width:100px'>"
         "<div class=k style='display:contents'></div><div id=a class=s style='height:10px'>a</div></div>");
    EQ_MSG(1, text_x(&g_doc, "3") >= 0, "counter incremented by the contents element");
}

static void test_l131_list_numbering(void) {
    /* the list owner is the DOM parent: start and reversed still apply */
    lays("<div style='width:100px'><ol start=3 style='display:contents'><li id=a>a<li id=b>b</ol></div>");
    XY("a", 0, 0, 100, 10);
    XY("b", 0, 10, 100, 10);
    EQ_MSG(1, text_x(&g_doc, "3.") != -1000, "3.");
    EQ_MSG(1, text_x(&g_doc, "4.") != -1000, "4.");
    lays("<div style='width:100px'><ol reversed style='display:contents'><li>a<li>b<li>c</ol></div>");
    EQ_MSG(1, text_x(&g_doc, "3.") != -1000, "reversed 3.");
    EQ_MSG(1, text_x(&g_doc, "1.") != -1000, "reversed 1.");
    EQ_MSG(1, text_x(&g_doc, "4.") == -1000, "not 4.");
    /* an li of another list in the same box parent does not count */
    lays("<div style='width:100px'><ul><li>q</ul><ol style='display:contents'><li>a</ol></div>");
    EQ_MSG(1, text_x(&g_doc, "1.") != -1000, "1.");
    /* the owner may be the document node itself (items at the top level) */
    lays("<li style='list-style-type:decimal'>a</li><li style='list-style-type:decimal'>b</li>");
    EQ_MSG(1, text_x(&g_doc, "2.") != -1000, "second top-level li is 2.");
    /* an li inside a contents wrapper belongs to the wrapper, as it does to a plain div:
     * the li after it is the second item of the ol, not the third */
    lays("<div style='width:100px'><ol><li>a</li><div style='display:contents'><li>b</li></div><li>c</li></ol></div>");
    EQ_MSG(1, text_x(&g_doc, "2.") != -1000, "c is 2.");
    EQ_MSG(1, text_x(&g_doc, "3.") == -1000, "no 3.");
    lays("<div style='width:100px'><ol><li>a</li><div style='display:block'><li>b</li></div><li>c</li></ol></div>");
    EQ_MSG(1, text_x(&g_doc, "2.") != -1000, "plain div: c is 2.");
    EQ_MSG(1, text_x(&g_doc, "3.") == -1000, "plain div: no 3.");
}

static void test_l131_table_sections(void) {
    same_boxes("<table id=t border=1><tbody style='display:contents;background:red'><tr><td id=a>a<td id=b>b<tr><td id=c>c<td id=d>d</tbody></table>",
               "<table id=t border=1><tr><td id=a>a<td id=b>b<tr><td id=c>c<td id=d>d</table>", "t a b c d");
    same_boxes("<table id=t border=1><thead style='display:contents;background:red'><tr><th id=a>h</thead><tbody><tr><td id=b>b</tbody></table>",
               "<table id=t border=1><thead><tr><th id=a>h</thead><tbody><tr><td id=b>b</tbody></table>", "t a b");
}

/* z-index, transform, opacity, outline, overflow, shadow: no box, no effect */
static void test_l131_no_box_effects(void) {
    same_boxes("<div id=o style='width:200px'><div style='display:contents;transform:translate(7px,7px);opacity:.5;z-index:3;position:relative;"
               "outline:2px solid blue;overflow:hidden;box-shadow:3px 3px red;background:red'>"
               "<div id=a style='height:10px;background:green'>a</div><div id=b style='height:10px;float:right;width:20px'>b</div></div><div id=c style='height:10px;background:blue'>c</div></div>",
               "<div id=o style='width:200px'><div id=a style='height:10px;background:green'>a</div><div id=b style='height:10px;float:right;width:20px'>b</div>"
               "<div id=c style='height:10px;background:blue'>c</div></div>", "o a b c");
    same_boxes("<div id=o style='width:200px'><div style='display:contents;opacity:0'><div id=a style='height:10px;background:green'>a</div></div></div>",
               "<div id=o style='width:200px'><div id=a style='height:10px;background:green'>a</div></div>", "o a");
    /* visibility is inherited: the children see it */
    lays("<div id=o style='width:200px'><div style='display:contents;visibility:hidden'><div id=a style='height:10px;background:green'>a</div></div></div>");
    EQ_MSG(0, (int)g_doc.nitems, "hidden children paint nothing");
}

static void cat(void *ctx, const char *s) { strncat((char *)ctx, s, 2047 - strlen((char *)ctx)); }

static void test_l131_inspect_and_dump(void) {
    static char out[2048];
    lays("<div id=o><div id=w style='display:contents'><i id=a>a</i></div></div>");
    out[0] = 0;
    we_inspect(&g_doc, we_find_id(&g_doc, "w"), cat, out);
    EQ_MSG(1, strstr(out, " display contents box 0 0 0x0") != 0, "inspect");
    out[0] = 0;
    we_dump_dom(&g_doc, cat, out);
    EQ_MSG(1, strstr(out, "div#w contents") != 0, "dump");
    out[0] = 0;
    we_inspect(&g_doc, we_find_id(&g_doc, "o"), cat, out);
    EQ_MSG(1, strstr(out, " display block") != 0, "block unchanged");
}

/* the passes after the first: a container query that stops (or starts) matching */
static void test_l131_container_query_restyle(void) {
    const char *css = "<style>.c{container-type:inline-size}@container (min-width:500px){#w{display:contents}}</style>";
    char html[1024];
    /* wide: contents, 3 items; then narrow: the wrapper is a box again */
    snprintf(html, sizeof(html), "%s<div class=c><div id=f style='display:flex'><div id=w style='padding:5px'><div id=a style='width:20px;height:10px'></div>"
             "<div id=b style='width:20px;height:10px'></div></div><div id=z style='width:20px;height:10px'></div></div></div>", css);
    load(&g_doc, &g_forms, html);
    TEST_ASSERT_EQUAL(0, we_layout_viewport(&g_doc, 600, 480));
    XY("a", 0, 0, 20, 10);
    XY("b", 20, 0, 20, 10);
    XY("z", 40, 0, 20, 10);
    EQ_MSG(1, el("w")->contents, "contents at 600");
    TEST_ASSERT_EQUAL(0, we_layout_viewport(&g_doc, 284, 480));
    EQ_MSG(0, el("w")->contents, "a box at 284");
    EQ_MSG(0, g_doc.spliced, "nothing left to splice");
    XY("w", 0, 0, 30, 30);
    XY("a", 5, 5, 20, 10);
    XY("b", 5, 15, 20, 10);
    XY("z", 30, 0, 20, 10);
    load(&g_fresh, &g_forms2, html);   /* a document laid out narrow only */
    TEST_ASSERT_EQUAL(0, we_layout_viewport(&g_fresh, 284, 480));
    TEST_ASSERT_EQUAL(g_fresh.nnodes, g_doc.nnodes);
    TEST_ASSERT_EQUAL(0, memcmp(g_doc.nodes, g_fresh.nodes, g_doc.nnodes * sizeof(we_node_t)));
    TEST_ASSERT_EQUAL(0, memcmp(g_doc.items, g_fresh.items, g_doc.nitems * sizeof(g_doc.items[0])));
    /* and back: narrow, then wide */
    TEST_ASSERT_EQUAL(0, we_layout_viewport(&g_doc, 600, 480));
    EQ_MSG(1, el("w")->contents, "contents again at 600");
    XY("z", 40, 0, 20, 10);
}

/* a viewport unit re-style (layout_once) splices again */
static void test_l131_viewport_restyle(void) {
    const char *html = "<div id=f style='display:flex;width:50vw'><div style='display:contents;padding:10vw'><div id=a style='width:10vw;height:10px'></div></div><div id=z style='width:5vw;height:10px'></div></div>";
    load(&g_doc, &g_forms, html);
    TEST_ASSERT_EQUAL(0, we_layout_viewport(&g_doc, 800, 480));
    XY("a", 0, 0, 80, 10);
    XY("z", 80, 0, 40, 10);
    TEST_ASSERT_EQUAL(0, we_layout_viewport(&g_doc, 200, 480));
    XY("a", 0, 0, 20, 10);
    XY("z", 20, 0, 10, 10);
    EQ_MSG(1, g_doc.vp_restyles >= 1, "re-styled");
}

int main(void) {
    unity_init();
    RUN_TEST(test_l131_flex_wrapper);
    RUN_TEST(test_l131_grid_wrapper);
    RUN_TEST(test_l131_inline_flow);
    RUN_TEST(test_l131_containing_blocks);
    RUN_TEST(test_l131_hidden_and_boxes_back);
    RUN_TEST(test_l131_replaced_and_root);
    RUN_TEST(test_l131_cascade_and_restyle);
    RUN_TEST(test_l131_links_restored);
    RUN_TEST(test_l131_box_tree_consistent);
    RUN_TEST(test_l131_stacking_zero_absolute);
    RUN_TEST(test_l131_full_node_table);
    RUN_TEST(test_l131_generated_and_counters);
    RUN_TEST(test_l131_list_numbering);
    RUN_TEST(test_l131_table_sections);
    RUN_TEST(test_l131_no_box_effects);
    RUN_TEST(test_l131_inspect_and_dump);
    RUN_TEST(test_l131_container_query_restyle);
    RUN_TEST(test_l131_viewport_restyle);
    unity_print_results();
    unity_cleanup();
    return unity_stats.tests_failed == 0 ? 0 : 1;
}
