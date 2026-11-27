/* Lot 130 (US-031): a style pass starts from the full initial style, and a
 * box narrower than its own padding no longer divides by zero.
 *
 * 1. Every pass after the first (a container query settling on another
 *    size, a viewport unit re-style, we_style called again by the page
 *    scripts) must give the nodes the bytes a first pass gives. style_node
 *    reset about half of the fields a declaration can write; the rest kept
 *    the value of the pass before when its rule no longer matched (an
 *    @container block after a resize, or after the container it queries
 *    changed size). The tests lay a document out wide and then narrow
 *    against a fresh document laid out narrow, and compare every node
 *    and every painted item byte for byte.
 * 2. scroll_paint_x divided by box_cw, 0 for a box that does not scroll
 *    along x, when the view width (the box width minus its padding) was
 *    negative: an img, a fixed 40 px placeholder, with padding: 0 25px
 *    stopped the guest with a divide error.
 * Values follow docs/lot130_plan.md. */
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
static char g_html[8192];

static void load(we_doc_t *d, we_forms_t *f, const char *html) {
    we_forms_init(f);
    we_set_forms(d, f);
    TEST_ASSERT_EQUAL(0, we_parse(d, html, (int)strlen(html)));
    TEST_ASSERT_EQUAL(0, we_style(d));
}

static void lays(const char *html) {
    load(&g_doc, &g_forms, html);
    TEST_ASSERT_EQUAL(0, we_layout_viewport(&g_doc, 640, 480));
}

static const we_node_t *el(const char *id) {
    int i = we_find_id(&g_doc, id);
    memset(&g_none, 0x5a, sizeof(g_none));
    g_none.h = -1; g_none.w = -1; g_none.x = -1; g_none.y = -1;
    return i >= 0 ? &g_doc.nodes[i] : &g_none;
}

#define XY(id, X, Y, W, H) do { EQ_MSG(X, el(id)->x, id " x"); EQ_MSG(Y, el(id)->y, id " y"); \
    EQ_MSG(W, el(id)->w, id " w"); EQ_MSG(H, el(id)->h, id " h"); } while (0)

static int diag_count(const we_doc_t *d, const char *t) {
    uint32_t i;
    int c = 0;
    for (i = 0; i < d->ndiag; i++) if (strstr(d->diag[i], t)) c++;
    return c;
}

/* 0 when g_doc and g_fresh hold the same nodes and the same painted items,
 * else 1 with the first difference printed */
static int same_doc(const char *what) {
    uint32_t i;
    if (g_doc.nnodes != g_fresh.nnodes) { printf("  %s: %d nodes against %d\n", what, (int)g_doc.nnodes, (int)g_fresh.nnodes); return 1; }
    for (i = 0; i < g_doc.nnodes; i++) {
        const unsigned char *a = (const unsigned char *)&g_doc.nodes[i], *b = (const unsigned char *)&g_fresh.nodes[i];
        uint32_t off;
        if (!memcmp(a, b, sizeof(we_node_t))) continue;
        for (off = 0; off < sizeof(we_node_t) && a[off] == b[off]; off++) {}
        printf("  %s: node %d <%s> differs from offset %d\n", what, (int)i, g_doc.nodes[i].tag, (int)off);
        return 1;
    }
    if (g_doc.nitems != g_fresh.nitems || memcmp(g_doc.items, g_fresh.items, g_doc.nitems * sizeof(g_doc.items[0]))) {
        printf("  %s: painted items differ (%d against %d)\n", what, (int)g_doc.nitems, (int)g_fresh.nitems);
        return 1;
    }
    return 0;
}

/* the document of every resize test: #t carries decl only inside an
 * @container block (min-width 500px), base is more author CSS for it */
static void build(const char *decl, const char *base) {
    snprintf(g_html, sizeof g_html,
        "<style>.c{container-type:inline-size}@container (min-width:500px){#t{%s}}"
        "#p{display:flex;flex-wrap:nowrap;width:100%%}%s#g{display:grid;grid-template-columns:30px 40px}</style>"
        "<div class=c><div id=p><div id=t><i id=x>ab</i><i id=y>c</i></div><b id=s>s</b></div>"
        "<div id=g><i>1</i><i>2</i><i>3</i></div></div>", decl, base);
}

/* g_doc: styled and laid out at wide, then at 284 (the container query
 * stops matching); g_fresh: laid out at 284 only */
static void resize(const char *decl, const char *base, int wide) {
    build(decl, base);
    load(&g_doc, &g_forms, g_html);
    if (wide) TEST_ASSERT_EQUAL(0, we_layout_viewport(&g_doc, wide, 480));
    TEST_ASSERT_EQUAL(0, we_layout_viewport(&g_doc, 284, 480));
    load(&g_fresh, &g_forms2, g_html);
    TEST_ASSERT_EQUAL(0, we_layout_viewport(&g_fresh, 284, 480));
}

/* the same with the rule on every element below .c, in a document with a
 * table, lists, a form and generated content */
static void resize_rich(const char *decl) {   /* 600: the widths where an option of a select kept own_edges */
    snprintf(g_html, sizeof g_html,
        "<style>.c{container-type:inline-size}@container (min-width:500px){.c *,.c *::before{%s}}"
        ".q::before{content:\"b\"}li::marker{color:red}</style>"
        "<div class=c><div id=p><div id=t><i id=x>ab</i><i id=y>c</i></div><b id=s>s</b></div>"
        "<div id=g style='display:grid'><i>1</i><i>2</i><i>3</i></div>"
        "<table border=1 cellpadding=3><caption>c</caption><tr><th>h<td bgcolor=red align=right>d<tr><td colspan=2>w</table>"
        "<ul><li>a<li value=5>b</ul><ol reversed type=a start=3><li>x</ol>"
        "<form><input type=text value=v><input type=checkbox checked><select><option>1<option selected>2</select>"
        "<textarea>t</textarea><button>b</button><progress value=3 max=9></progress><meter value=2></meter></form>"
        "<p class=q>q<q>z</q><img width=5 height=6 alt=a><hr><details open><summary>s</summary>x</details>"
        "<span style='display:flex'><i>1</i><i>2</i></span></div>", decl);
    load(&g_doc, &g_forms, g_html);
    TEST_ASSERT_EQUAL(0, we_layout_viewport(&g_doc, 600, 480));
    TEST_ASSERT_EQUAL(0, we_layout_viewport(&g_doc, 284, 480));
    load(&g_fresh, &g_forms2, g_html);
    TEST_ASSERT_EQUAL(0, we_layout_viewport(&g_fresh, 284, 480));
}

/* --- the box model: margin, padding and border stayed --- */
static void test_l130_resize_box(void) {
    resize("margin:10px;padding:7px;border:2px solid red", "", 624);
    XY("t", 0, 0, 24, 10); XY("s", 24, 0, 8, 10); XY("p", 0, 0, 284, 10); XY("g", 0, 10, 284, 20);
    EQ_MSG(0, el("t")->margin[0] | el("t")->margin[1] | el("t")->margin[2] | el("t")->margin[3], "margin");
    EQ_MSG(0, el("t")->padding[0] | el("t")->padding[1] | el("t")->padding[2] | el("t")->padding[3], "padding");
    EQ_MSG(0, el("t")->border, "border");
    EQ_MSG(0, same_doc("box"), "same as fresh");
    /* percentages: the resolved value and the percentage both start again */
    resize("margin-left:5%;padding:3%", "#t{width:200px;height:30px}", 624);
    XY("t", 0, 0, 200, 30); XY("s", 200, 0, 8, 30); XY("p", 0, 0, 284, 30);
    EQ_MSG(0, el("t")->spacing_pct, "spacing pct");
    EQ_MSG(0, same_doc("pct"), "pct same as fresh");
}

/* --- min-width and max-width in percent --- */
static void test_l130_resize_min_max_pct(void) {
    resize("max-width:30%", "#t{width:200px;height:30px}", 624);
    XY("t", 0, 0, 200, 30); XY("s", 200, 0, 8, 30);
    EQ_MSG(0, el("t")->max_pct, "max pct");
    EQ_MSG(0, same_doc("max"), "max same as fresh");
    resize("min-width:90%", "#t{width:200px;height:30px}", 624);
    XY("t", 0, 0, 200, 30); XY("s", 200, 0, 8, 30);
    EQ_MSG(0, el("t")->min_pct, "min pct");
    EQ_MSG(0, same_doc("min"), "min same as fresh");
}

/* --- flex: direction, wrap and gap stayed --- */
static void test_l130_resize_flex(void) {
    resize("flex-direction:column;flex-wrap:wrap;gap:20px", "#t{display:flex}", 624);
    XY("x", 0, 0, 16, 10); XY("y", 16, 0, 8, 10); XY("t", 0, 0, 24, 10); XY("p", 0, 0, 284, 10);
    EQ_MSG(0, el("t")->flex_dir, "dir"); EQ_MSG(0, el("t")->flex_wrap, "wrap"); EQ_MSG(0, el("t")->gap, "gap");
    EQ_MSG(0, diag_count(&g_doc, "column wrap requires height"), "no wrap diag");
    EQ_MSG(0, same_doc("flex"), "flex same as fresh");
}

/* --- paint: background, float, grid templates, shadows --- */
static void test_l130_resize_paint_and_grid(void) {
    resize("background:blue;float:left;box-shadow:2px 2px 3px red;outline:2px solid red", "", 624);
    EQ_MSG(0, el("t")->has_bg, "has_bg"); EQ_MSG(0, el("t")->bg, "bg"); EQ_MSG(0, el("t")->float_side, "float");
    EQ_MSG(0, el("t")->shadow_count, "shadow"); EQ_MSG(0, el("t")->outline_style, "outline");
    EQ_MSG(0, same_doc("paint"), "paint same as fresh");
    resize("grid-template-columns:20px 20px;grid-template-rows:20px 20px;grid-template-areas:\"a b\" \"c d\"", "#t{display:grid}", 624);
    EQ_MSG(0, el("t")->grid_cols, "grid cols"); EQ_MSG(0, el("t")->grid_rows, "grid rows");
    EQ_MSG(0, el("t")->tpl_count[0] | el("t")->tpl_count[1], "template tracks");
    EQ_MSG(0, same_doc("grid"), "grid same as fresh");
}

/* --- one declaration per field the reset covers --- */
static void test_l130_restyle_matches_fresh(void) {
    static const char *const decls[] = {
        "margin:10px", "padding:7px", "border:2px solid red", "border-top:3px solid red", "gap:20px",
        "background-color:blue", "flex-direction:column", "flex-wrap:wrap", "float:left",
        "min-width:50%", "max-width:50%", "margin-left:5%", "padding:3%",
        "grid-template-columns:20px 20px", "grid-template-rows:repeat(auto-fit,20px)",
        "grid-template-columns:30px repeat(auto-fill,20px)", "grid-template-columns:calc(10px + 5%) 20px",
        "grid-template-areas:\"a b\" \"c d\"", "box-decoration-break:clone",
        "user-select:none", "pointer-events:none", "accent-color:red", "caret-color:red",
        "scroll-margin:5px", "scroll-padding:4px", "transform:rotate(10deg)", "outline:2px solid red",
        "box-shadow:2px 2px 3px red", "text-decoration:underline red",
        "background-image:linear-gradient(red,blue)", "background-image:radial-gradient(circle closest-side,red,blue)",
        "background-image:linear-gradient(to top right,red,blue)", "background-image:linear-gradient(45.5deg,red,blue)",
        "background-image:repeating-linear-gradient(red 0%,blue 20%)", "background-image:conic-gradient(red,blue)",
        "content:\"x\"", "counter-reset:c 3", "counter-increment:c", "counter-set:c 2",
        /* the last element of each array the reset clears */
        "margin:1px 2px 3px 4px", "padding:1px 2px 3px 4px", "padding-left:5%", "border-left:3px solid red",
        "scroll-margin:1px 2px 3px 4px", "scroll-padding:1px 2px 3px 4px",
        "transform:rotate(1deg) rotate(2deg) rotate(3deg) rotate(4deg)", "box-shadow:1px 1px red,2px 2px blue",
        "background-image:linear-gradient(red,blue,green,yellow)",
        "background-image:radial-gradient(ellipse 10px 20px at 10px 20px,red,blue)",
        "grid-template-columns:10px 10px 10px 10px 10px 10px 10px 10px",
        "grid-template-rows:10px 10px 10px 10px 10px 10px 10px 10px",
        "grid-template-columns:calc(1px + 1%) calc(2px + 2%) calc(3px + 3%) calc(4px + 4%) calc(5px + 5%) calc(6px + 6%) calc(7px + 7%) calc(8px + 8%)",
        "grid-template-rows:calc(1px + 1%) calc(2px + 2%) calc(3px + 3%) calc(4px + 4%) calc(5px + 5%) calc(6px + 6%) calc(7px + 7%) calc(8px + 8%)",
        "grid-template-areas:\"a b c d\" \"e f g h\"", "grid-template-rows:30px repeat(auto-fit,20px)",
        "grid-auto-columns:minmax(calc(1px + 1%),calc(2px + 2%))", "grid-auto-rows:minmax(calc(1px + 1%),calc(2px + 2%))",
        0
    };
    int k;
    for (k = 0; decls[k]; k++) {
        resize(decls[k], "", 624);
        if (same_doc(decls[k])) printf("  declaration: %s\n", decls[k]);
        TEST_ASSERT_EQUAL(0, same_doc(decls[k]));
    }
}

/* --- the same on every element kind: table cells, form controls, lists --- */
static void test_l130_restyle_matches_fresh_rich(void) {
    static const char *const decls[] = {
        "margin:10px", "padding:7px", "border:2px solid red", "background-color:blue", "float:left",
        "gap:20px", "max-width:50%", "grid-template-columns:20px 20px", "box-shadow:2px 2px 3px red",
        "transform:rotate(10deg)", "background-image:linear-gradient(red,blue)", "outline:2px solid red",
        "border-collapse:collapse;border:1px solid red", "content:\"x\"", "counter-reset:c 3",
        "display:grid;border-width:1px 2px 3px 4px",
        0
    };
    int k;
    for (k = 0; decls[k]; k++) {
        resize_rich(decls[k]);
        if (same_doc(decls[k])) printf("  declaration: %s\n", decls[k]);
        TEST_ASSERT_EQUAL(0, same_doc(decls[k]));
    }
}

/* --- one layout call: the container the rule reads changes size under the
 * rule of the container above it --- */
static void test_l130_nested_container_settles(void) {
    lays("<style>.x{container-type:inline-size}.y{container-type:inline-size}.h{height:10px}#z3{display:flex}"
         "@container (min-width:300px){.y{width:100px}}"
         "@container (min-width:200px){#z1{margin:30px;padding:5px;background:red}"
         "#z2{border:4px solid blue;max-width:50%}#z3{flex-direction:column;gap:8px}}</style>"
         "<div class=x><div class=y><div id=z1 class=h></div><div id=z2 class=h></div>"
         "<div id=z3 class=h><i id=k1 style='width:20px;height:10px'></i><i id=k2 style='width:20px;height:10px'></i></div></div></div>");
    /* .y is 100 wide once the first rule applies, so the 200 px rule no longer matches it */
    XY("z1", 0, 0, 100, 10); XY("z2", 0, 10, 100, 10); XY("z3", 0, 20, 100, 10);
    XY("k1", 0, 20, 20, 10); XY("k2", 20, 20, 20, 10);
    EQ_MSG(0, el("z1")->margin[0] | el("z1")->margin[1] | el("z1")->margin[2] | el("z1")->margin[3], "z1 margin");
    EQ_MSG(0, el("z1")->padding[0] | el("z1")->padding[1], "z1 padding");
    EQ_MSG(0, el("z1")->has_bg, "z1 bg");
    EQ_MSG(0, el("z2")->border, "z2 border"); EQ_MSG(0, el("z2")->max_pct, "z2 max pct");
    EQ_MSG(0, el("z3")->flex_dir, "z3 dir"); EQ_MSG(0, el("z3")->gap, "z3 gap");
    EQ_MSG(1, g_doc.cq_restyles >= 2, "settled over several passes");
    /* the page with .y 100 px wide and no query at all lays out the same */
    {
        static const char *const ids[] = { "z1", "z2", "z3", "k1", "k2", 0 };
        int k;
        load(&g_fresh, &g_forms2,
             "<style>.x{container-type:inline-size}.y{container-type:inline-size;width:100px}.h{height:10px}#z3{display:flex}</style>"
             "<div class=x><div class=y><div id=z1 class=h></div><div id=z2 class=h></div>"
             "<div id=z3 class=h><i id=k1 style='width:20px;height:10px'></i><i id=k2 style='width:20px;height:10px'></i></div></div></div>");
        TEST_ASSERT_EQUAL(0, we_layout_viewport(&g_fresh, 640, 480));
        for (k = 0; ids[k]; k++) {
            const we_node_t *a = el(ids[k]), *b = &g_fresh.nodes[we_find_id(&g_fresh, ids[k])];
            EQ_MSG(b->x, a->x, ids[k]); EQ_MSG(b->y, a->y, ids[k]); EQ_MSG(b->w, a->w, ids[k]); EQ_MSG(b->h, a->h, ids[k]);
        }
        TEST_ASSERT_EQUAL(g_fresh.nitems, g_doc.nitems);
        TEST_ASSERT_EQUAL(0, memcmp(g_fresh.items, g_doc.items, g_doc.nitems * sizeof(g_doc.items[0])));
    }
}

/* --- we_style again on a document whose rules no longer match (the page
 * scripts' path) --- */
static void test_l130_restyle_after_class_change(void) {
    static const char *const plain = "<style>.b{margin:10px;padding:4px;background:#f00;float:left;border:2px solid red;"
        "box-shadow:1px 1px red;transform:rotate(5deg);background-image:linear-gradient(red,blue)}</style>"
        "<div id=t class=b>x</div><div id=u>y</div>";
    static const char *const bare = "<style>.b{margin:10px;padding:4px;background:#f00;float:left;border:2px solid red;"
        "box-shadow:1px 1px red;transform:rotate(5deg);background-image:linear-gradient(red,blue)}</style>"
        "<div id=t>x</div><div id=u>y</div>";
    int t;
    lays(plain);
    t = we_find_id(&g_doc, "t");
    EQ_MSG(1, t > 0, "t found");
    EQ_MSG(1, g_doc.nodes[t].margin[0] != 0 || g_doc.nodes[t].border != 0, "class b applies");
    memset(g_doc.nodes[t].cls, 0, sizeof g_doc.nodes[t].cls);
    TEST_ASSERT_EQUAL(0, we_style(&g_doc));
    TEST_ASSERT_EQUAL(0, we_layout_viewport(&g_doc, 640, 480));
    load(&g_fresh, &g_forms2, bare);
    TEST_ASSERT_EQUAL(0, we_layout_viewport(&g_fresh, 640, 480));
    EQ_MSG(0, g_doc.nodes[t].margin[0] | g_doc.nodes[t].border | g_doc.nodes[t].has_bg | g_doc.nodes[t].float_side, "class gone");
    EQ_MSG(g_fresh.nodes[t].x, g_doc.nodes[t].x, "x"); EQ_MSG(g_fresh.nodes[t].w, g_doc.nodes[t].w, "w");
    EQ_MSG(g_fresh.nodes[t].h, g_doc.nodes[t].h, "h");
}

/* --- an img (a 40 x 10 placeholder) with more padding than width --- */
static int scroll_items(void) {
    uint32_t i;
    int c = 0;
    for (i = 0; i < g_doc.nitems; i++)
        if (g_doc.items[i].kind == OS_FB_WEB_RECT && (g_doc.items[i].color == 0xDDDDDD || g_doc.items[i].color == 0x888888)) c++;
    return c;
}

static void test_l130_padding_wider_than_box(void) {
    lays("<style>body{margin:0}img{padding:0 25px}</style><img id=i src=x>");
    XY("i", 0, 0, 40, 10); EQ_MSG(0, scroll_items(), "no bar");
    lays("<style>body{margin:0}img{padding:0 25px;overflow:auto}</style><img id=i src=x>");
    XY("i", 0, 0, 40, 10); EQ_MSG(0, scroll_items(), "overflow auto, no bar");
    lays("<style>body{margin:0}img{padding:0 25px;overflow-x:scroll}</style><img id=i src=x>");
    XY("i", 0, 0, 40, 10); EQ_MSG(0, scroll_items(), "overflow-x scroll, no bar");
    lays("<style>img{padding:5%}</style><img id=i alt=a>");
    EQ_MSG(0, scroll_items(), "percent padding, no bar");
    lays("<style>body{margin:0}img{padding:0 21px}</style><img id=i src=x>");
    XY("i", 0, 0, 40, 10);   /* 42 px of padding on 40 px: the first value with a negative view */
    lays("<style>body{margin:0}img{padding:0 20px}</style><img id=i src=x>");
    XY("i", 0, 0, 40, 10);   /* 40 px: a view of 0 never reached the divide */
}

/* the guard keeps the bar of a box that does scroll along x */
static void test_l130_scrollbar_x_kept(void) {
    lays("<style>body{margin:0}#b{width:100px;height:30px;overflow-x:scroll}</style>"
         "<div id=b><div style='width:300px;height:10px'></div></div>");
    EQ_MSG(1, scroll_items() >= 2, "track and thumb painted");
    EQ_MSG(1, el("b")->box_cw > 0, "content width");
}

int main(void) {
    unity_init();
    RUN_TEST(test_l130_resize_box);
    RUN_TEST(test_l130_resize_min_max_pct);
    RUN_TEST(test_l130_resize_flex);
    RUN_TEST(test_l130_resize_paint_and_grid);
    RUN_TEST(test_l130_restyle_matches_fresh);
    RUN_TEST(test_l130_restyle_matches_fresh_rich);
    RUN_TEST(test_l130_nested_container_settles);
    RUN_TEST(test_l130_restyle_after_class_change);
    RUN_TEST(test_l130_padding_wider_than_box);
    RUN_TEST(test_l130_scrollbar_x_kept);
    unity_print_results();
    unity_cleanup();
    return unity_stats.tests_failed == 0 ? 0 : 1;
}
