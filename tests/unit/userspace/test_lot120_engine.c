/* Lot 120 (US-031): the root box and the initial containing block (the
 * percentage heights of the html element, absolutely positioned boxes
 * without a positioned ancestor, the canvas background propagated from
 * the root or its body), container style queries (@container
 * style(--x: v), CSS Conditional 5 7.2) and the angle units on the hue
 * channel only. Values follow docs/lot120_plan.md. */
#include "../../framework/unity.h"
#include <string.h>
#include <stdio.h>
#include "webeng.h"

#define EQ_MSG(e, a, m) do { int e_ = (int)(e), a_ = (int)(a); \
    if (e_ != a_) printf("  %s: expected %d (0x%08x), got %d (0x%08x)\n", (m), e_, (unsigned)e_, a_, (unsigned)a_); \
    TEST_ASSERT_EQUAL(e_, a_); } while (0)

static we_doc_t g_doc;
static we_forms_t g_forms;
static we_node_t g_none;

static void lay_at(const char *html, int w, int h) {
    we_forms_init(&g_forms);
    we_set_forms(&g_doc, &g_forms);
    TEST_ASSERT_EQUAL(0, we_parse(&g_doc, html, (int)strlen(html)));
    TEST_ASSERT_EQUAL(0, we_style(&g_doc));
    TEST_ASSERT_EQUAL(0, we_layout_viewport(&g_doc, w, h));
}

static void lays(const char *html) { lay_at(html, 640, 480); }

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

/* the display items of that color (alpha byte included) */
static int items_of(uint32_t color) {
    uint32_t i;
    int c = 0;
    for (i = 0; i < g_doc.nitems; i++) if (g_doc.items[i].kind == OS_FB_WEB_RECT && g_doc.items[i].color == color) c++;
    return c;
}

#define BOX(id, X, Y, W, H) do { const we_node_t *n_ = el(id); \
    EQ_MSG(X, n_->x, id " x"); EQ_MSG(Y, n_->y, id " y"); EQ_MSG(W, n_->w, id " w"); EQ_MSG(H, n_->h, id " h"); } while (0)

/* --- the initial containing block: percentage heights of the root --- */
static void test_l120_root_height(void) {
    lays("<html id=r style='height:100%'><body id=b style='height:100%;margin:0'><div id=a style='height:50%'></div></body></html>");
    BOX("r", 0, 0, 640, 480);
    BOX("b", 0, 0, 640, 480);
    BOX("a", 0, 0, 640, 240);
    EQ_MSG(0, diag_count("percent height indefinite"), "no indefinite height");
    EQ_MSG(we_find_id(&g_doc, "r"), g_doc.root_html, "root_html");
    lay_at("<html id=r style='height:25%'><body style='margin:0'></body></html>", 640, 200);
    BOX("r", 0, 0, 640, 50);
    lays("<html id=r style='height:50%;min-height:300px'><body id=b style='margin:0;min-height:100%'><div style='height:10px'></div></body></html>");
    BOX("r", 0, 0, 640, 300);
    BOX("b", 0, 0, 640, 300);
    lays("<html id=r style='height:400px;max-height:50%'><body style='margin:0'></body></html>");
    BOX("r", 0, 0, 640, 240);
    lays("<html id=r style='min-height:100%'><body style='margin:0'><div style='height:10px'></div></body></html>");
    BOX("r", 0, 0, 640, 480);
    /* padding and borders of the root: the content box takes the percentage */
    lays("<html id=r style='height:50%;padding:5px;border:2px solid red;margin:10px'><body id=b style='margin:0;height:100%'></body></html>");
    BOX("r", 10, 10, 620, 254);
    BOX("b", 17, 17, 606, 240);
    lays("<html id=r style='height:50%;box-sizing:border-box;padding:5px'><body id=b style='margin:0;height:100%'></body></html>");
    BOX("r", 0, 0, 640, 240);
    BOX("b", 5, 5, 630, 230);
    /* body in html without a height: still indefinite */
    lays("<html id=r><body id=b style='height:50%;margin:0'><div style='height:10px'></div></body></html>");
    BOX("b", 0, 0, 640, 10);
    EQ_MSG(1, diag_count("percent height indefinite"), "body in an auto html");
    /* a fragment has no root element: its boxes keep the document block */
    lays("<div id=a style='height:50%'>a</div>");
    EQ_MSG(0, g_doc.root_html, "fragment root");
    EQ_MSG(1, diag_count("percent height indefinite"), "fragment");
    EQ_MSG(10, el("a")->h, "fragment height");
    lays("<body id=b style='height:100%;margin:0'>b</body>");
    EQ_MSG(0, g_doc.root_html, "body root");
    EQ_MSG(10, el("b")->h, "body as root");
    /* the html element after a text node, and a later html ignored */
    lays(" <html id=r style='height:10%'><body style='margin:0'></body></html>");
    BOX("r", 0, 0, 640, 48);
    lays("<html id=r style='display:none;height:100%'><body></body></html>");
    EQ_MSG(0, el("r")->h, "hidden root");
    /* a vh height and a percentage width are unchanged */
    lays("<html id=r style='width:50%;height:25vh'><body id=b style='margin:0;height:50%'></body></html>");
    BOX("r", 0, 0, 320, 120);
    BOX("b", 0, 0, 320, 60);
    lays("<html id=r style='margin:10%'><body style='margin:0;height:10px'></body></html>");
    BOX("r", 64, 64, 512, 10);
}

/* --- the initial containing block of absolute boxes --- */
static void test_l120_icb_absolute(void) {
    lays("<html><body style='margin:0'><div id=a style='position:absolute;bottom:0;right:0;width:20px;height:10px'></div></body></html>");
    BOX("a", 620, 470, 20, 10);
    /* a page taller than the viewport: the block is still the first screen */
    lays("<html><body style='margin:0'><div id=a style='position:absolute;bottom:0;right:0;width:20px;height:10px'></div><div style='height:1000px'></div></body></html>");
    BOX("a", 620, 470, 20, 10);
    lay_at("<html><body style='margin:0'><div id=a style='position:absolute;bottom:0;width:20px;height:10px'></div></body></html>", 640, 300);
    BOX("a", 0, 290, 20, 10);
    lays("<div id=a style='position:absolute;top:0;bottom:0;left:0;width:20px'></div>");
    BOX("a", 0, 0, 20, 480);
    lays("<html><body><div id=a style='position:absolute;top:0;left:0;height:100%;width:50%'></div></body></html>");
    BOX("a", 0, 0, 320, 480);
    lays("<html><body><div id=a style='position:absolute;top:50%;left:25%;width:10px;height:10px'></div></body></html>");
    BOX("a", 160, 240, 10, 10);
    /* the origin of the canvas, not the margin box of html or body */
    lays("<html style='margin:20px;padding:5px'><body style='margin:7px'><div id=a style='position:absolute;top:0;left:0;width:10px;height:10px'></div></body></html>");
    BOX("a", 0, 0, 10, 10);
    lays("<html style='margin:20px'><body><div id=a style='position:absolute;bottom:10px;right:10px;width:10px;height:10px'></div></body></html>");
    BOX("a", 620, 460, 10, 10);
    /* a positioned ancestor is the block, as before */
    lays("<html><body style='margin:0'><div style='position:relative;height:100px;margin-top:30px'><div id=a style='position:absolute;bottom:0;width:10px;height:10px'></div></div></body></html>");
    BOX("a", 0, 120, 10, 10);
    lays("<html style='position:relative;height:200px'><body style='margin:0'><div id=a style='position:absolute;bottom:0;width:10px;height:10px'></div></body></html>");
    BOX("a", 0, 190, 10, 10);
    /* auto insets keep the static position */
    lays("<html><body style='margin:0'><div style='height:30px'></div><div id=a style='position:absolute;width:10px;height:10px'></div></body></html>");
    BOX("a", 0, 30, 10, 10);
}

/* --- the canvas background (CSS Backgrounds 3, 2.11.2) --- */
static void test_l120_canvas(void) {
    const os_fb_web_item_t *it;
    lays("<html id=r style='background:#123456'><body id=b style='margin:0;background:#00ff00'><div style='height:10px'></div></body></html>");
    EQ_MSG(we_find_id(&g_doc, "r"), g_doc.canvas_from, "html paints the canvas");
    EQ_MSG(0x123456, g_doc.canvas_bg, "canvas color");
    it = &g_doc.items[0];
    EQ_MSG(OS_FB_WEB_RECT, it->kind, "canvas kind");
    EQ_MSG(0, it->x, "canvas x"); EQ_MSG(0, it->y, "canvas y");
    EQ_MSG(640, it->w, "canvas w"); EQ_MSG(480, it->h, "canvas h");
    EQ_MSG(0x123456, it->color, "canvas item");
    EQ_MSG(1, items_of(0x123456), "the html box does not paint it again");
    EQ_MSG(1, items_of(0x00ff00), "body keeps its own");
    EQ_MSG(-1, g_doc.item_owner[0] - 1, "owner: the document");
    /* html without a background: body's color goes to the canvas */
    lays("<html id=r><body id=b style='margin:8px;background:#00ff00'><div style='height:1000px'></div></body></html>");
    EQ_MSG(we_find_id(&g_doc, "b"), g_doc.canvas_from, "body paints the canvas");
    EQ_MSG(0x00ff00, g_doc.items[0].color, "body color");
    EQ_MSG(640, g_doc.items[0].w, "tall w");
    EQ_MSG(1016, g_doc.items[0].h, "the whole page");
    EQ_MSG(1, items_of(0x00ff00), "body box not painted");
    lays("<html style='background:transparent'><head></head><body id=b style='background:#0000ff'></body></html>");
    EQ_MSG(we_find_id(&g_doc, "b"), g_doc.canvas_from, "transparent html");
    EQ_MSG(0x0000ff, g_doc.canvas_bg, "after head");
    /* a root gradient or image: nothing propagates from body */
    lays("<html style='background-image:linear-gradient(red,blue)'><body style='background:#00ff00'></body></html>");
    EQ_MSG(0, g_doc.canvas_from, "gradient root");
    EQ_MSG(1, items_of(0x00ff00), "body paints its own");
    lays("<html style='background-image:url(none.png)'><body style='background:#00ff00'></body></html>");
    EQ_MSG(0, g_doc.canvas_from, "image root");
    lays("<html style='background:#ff0000 linear-gradient(red,blue)'><body></body></html>");
    EQ_MSG(1, g_doc.canvas_from, "color with a gradient");
    EQ_MSG(0xff0000, g_doc.canvas_bg, "its color");
    /* boxes that are not rendered */
    lays("<html><body style='display:none;background:#00ff00'></body></html>");
    EQ_MSG(0, g_doc.canvas_from, "hidden body");
    EQ_MSG(0, items_of(0x00ff00), "hidden body item");
    lays("<html style='display:none;background:#00ff00'><body></body></html>");
    EQ_MSG(0, g_doc.canvas_from, "hidden html");
    lays("<html><body></body></html>");
    EQ_MSG(0, g_doc.canvas_from, "no color anywhere");
    EQ_MSG(0, g_doc.nitems, "no canvas item");
    lays("<html><div style='background:#00ff00'></div></html>");
    EQ_MSG(0, g_doc.canvas_from, "no body");
    lays("<html><body><div style='background:#00ff00;height:5px'></div></body></html>");
    EQ_MSG(0, g_doc.canvas_from, "a div is not body");
    lays("<body style='background:#00ff00'></body>");
    EQ_MSG(0, g_doc.canvas_from, "fragment body");
    lays("<div style='background:#00ff00;height:5px'></div>");
    EQ_MSG(0, g_doc.canvas_from, "fragment");
    EQ_MSG(1, items_of(0x00ff00), "the first item of a fragment is its own");
    /* a translucent root color keeps its alpha */
    lays("<html style='background:rgb(0 0 255 / 0.5)'><body></body></html>");
    EQ_MSG(0x7f0000ff, g_doc.canvas_bg, "alpha");
}

static char g_out[1024];
static void out_cat(void *ctx, const char *s) { (void)ctx; if (strlen(g_out) + strlen(s) < sizeof(g_out)) strcat(g_out, s); }

static void test_l120_inspect(void) {
    lays("<html id=r><body id=b style='background:#00ff00'></body></html>");
    g_out[0] = 0; we_inspect(&g_doc, we_find_id(&g_doc, "b"), out_cat, 0);
    TEST_ASSERT_NOT_NULL(strstr(g_out, " canvas #00ff00\n"));
    g_out[0] = 0; we_inspect(&g_doc, we_find_id(&g_doc, "r"), out_cat, 0);
    TEST_ASSERT_NULL(strstr(g_out, " canvas "));
    lays("<div id=a>a</div>");
    g_out[0] = 0; we_inspect(&g_doc, 0, out_cat, 0);
    TEST_ASSERT_NULL(strstr(g_out, " canvas "));
}

/* --- container style queries --- */
#define SQ_HEAD "<style>body{margin:0}.c{--t:dark;--n: 1   2}"
#define SQ_BODY "<body><div class=c id=c><div id=t>t</div></div></body>"

/* whether #t's rule under @container cond applied (50 px wide), extra
 * CSS before it */
static int sq_holds_css(const char *extra, const char *cond) {
    char h[900];
    sprintf(h, SQ_HEAD "%s@container %s{#t{width:50px}}</style>" SQ_BODY, extra, cond);
    lays(h);
    return el("t")->w == 50 ? 1 : el("t")->w == 640 || el("t")->w == 300 ? 0 : -1;   /* 300: a sized .c */
}
static int sq_holds(const char *cond) { return sq_holds_css("", cond); }

static void test_l120_style_query(void) {
    EQ_MSG(1, sq_holds("style(--t: dark)"), "value");
    EQ_MSG(1, sq_holds("style(--t:dark)"), "no space");
    EQ_MSG(1, sq_holds("style( --t : dark )"), "spaces");
    EQ_MSG(0, sq_holds("style(--t: light)"), "other value");
    EQ_MSG(0, sq_holds("style(--t: Dark)"), "case");
    EQ_MSG(0, sq_holds("style(--t: dar)"), "prefix");
    EQ_MSG(0, sq_holds("style(--t: darker)"), "longer");
    EQ_MSG(1, sq_holds("style(--n: 1 2)"), "white space runs");
    EQ_MSG(1, sq_holds("style(--n:1  2 )"), "white space runs 2");
    EQ_MSG(0, sq_holds("style(--n: 12)"), "no space is another value");
    EQ_MSG(0, sq_holds("style(--n: 1 2 3)"), "longer list");
    EQ_MSG(0, sq_holds("style(--n: 1)"), "shorter list");
    EQ_MSG(1, sq_holds("style(--t)"), "defined");
    EQ_MSG(0, sq_holds("style(--z)"), "undefined");
    EQ_MSG(0, sq_holds("style(--z: dark)"), "undefined value");
    EQ_MSG(1, sq_holds("style(--z: initial)"), "undefined is initial");
    EQ_MSG(0, sq_holds("style(--t: initial)"), "defined is not initial");
    EQ_MSG(1, sq_holds("not style(--t: light)"), "not");
    EQ_MSG(1, sq_holds("style(not (--t: light))"), "not inside");
    EQ_MSG(0, sq_holds("style(not (--t: dark))"), "not inside false");
    EQ_MSG(1, sq_holds("style((--t: dark) and (--n: 1 2))"), "and");
    EQ_MSG(0, sq_holds("style((--t: dark) and (--z: 1))"), "and false");
    EQ_MSG(1, sq_holds("style((--t: light) or (--t: dark))"), "or");
    EQ_MSG(0, sq_holds("style((--t: light) or (--z))"), "or false");
    EQ_MSG(1, sq_holds("style(((--t: dark)))"), "nested parentheses");
    EQ_MSG(1, sq_holds("style(--t: dark) and style(--n: 1 2)"), "two functions");
    EQ_MSG(1, sq_holds("style(--t: light) or style(--t)"), "two functions or");
    EQ_MSG(1, sq_holds("(style(--t: dark))"), "in parentheses");
    /* WE_CSS_NEST_MAX groups inside style() too */
    EQ_MSG(1, sq_holds("style((((--t: dark))))"), "three groups");
    /* var() in the value: the container's */
    EQ_MSG(1, sq_holds_css(".c{--d:dark}", "style(--t: var(--d))"), "var");
    EQ_MSG(0, sq_holds_css(".c{--d:light}", "style(--t: var(--d))"), "var other");
    EQ_MSG(1, sq_holds("style(--t: var(--q, dark))"), "var fallback");
    EQ_MSG(0, sq_holds("style(--t: var(--q))"), "var undefined");
    EQ_MSG(1, sq_holds("not style(--t: var(--q))"), "var undefined is false, not unknown");
    EQ_MSG(1, sq_holds_css("#t{--d:light}.c{--d:dark}", "style(--t: var(--d))"), "var of the container, not the element");
    /* registered properties: their initial value */
    EQ_MSG(1, sq_holds_css("@property --r{syntax:'<number>';inherits:true;initial-value:5}", "style(--r: 5)"), "registered initial");
    EQ_MSG(0, sq_holds_css("@property --r{syntax:'<number>';inherits:true;initial-value:5}", "style(--r)"), "registered not set");
    EQ_MSG(1, sq_holds_css("@property --r{syntax:'<number>';inherits:true;initial-value:5}", "style(--r: initial)"), "registered initial kw");
    EQ_MSG(1, sq_holds_css("@property --r{syntax:'<number>';inherits:true;initial-value:5}.c{--r:6}", "style(--r)"), "registered set");
    EQ_MSG(1, sq_holds_css("@property --r{syntax:'<number>';inherits:true;initial-value:5}.c{--r:6}", "style(--r: 6)"), "registered value");
    EQ_MSG(0, sq_holds_css("@property --r{syntax:'<number>';inherits:true;initial-value:5}.c{--r:6}", "style(--r: initial)"), "registered not initial");
    EQ_MSG(0, sq_holds_css("@property --r{syntax:'<number>';inherits:true;initial-value:5}.c{--r:5}", "style(--r)"), "registered set to initial");
    EQ_MSG(1, sq_holds_css("@property --r{syntax:'<number>';inherits:true;initial-value:5}.c{--r:5}", "style(--r: initial)"), "set to initial is initial");
    EQ_MSG(1, sq_holds_css("@property --r{syntax:'*';inherits:true;initial-value:  a   b  }", "style(--r: a b)"), "spaced initial value");
    EQ_MSG(1, sq_holds_css("@property --r{syntax:'*';inherits:true;initial-value:  a   b  }", "style(--r: initial)"), "spaced initial keyword");
    /* an empty fallback substituted: white space at the ends */
    EQ_MSG(1, sq_holds("style(--t: var(--q,) dark)"), "empty var first");
    EQ_MSG(1, sq_holds("style(--t: dark var(--q,))"), "empty var last");
    EQ_MSG(1, sq_holds("style(--n: 1 var(--q,) 2)"), "empty var inside");
    /* no initial-value: the registration is invalid, --r undefined */
    EQ_MSG(1, sq_holds_css("@property --r{syntax:'<number>';inherits:true}", "style(--r: initial)"), "invalid registration");
    EQ_MSG(1, sq_holds_css("@property --r{syntax:'*';inherits:true}", "style(--r: initial)"), "universal without initial");
    /* inherited values and the nearest ancestor */
    lays("<style>html{--t:dark}@container style(--t: dark){#t{width:50px}}</style><html><body style='margin:0'><div><div id=t>t</div></div></body></html>");
    EQ_MSG(50, el("t")->w, "inherited");
    lays("<style>.o{--t:dark}.i{--t:light}@container style(--t: dark){#t{width:50px}}</style><body style='margin:0'><div class=o><div class=i><div id=t>t</div></div></div></body>");
    EQ_MSG(640, el("t")->w, "nearest ancestor");
    lays("<style>.o{--t:dark;container-name:o}.i{--t:light}@container o style(--t: dark){#t{width:50px}}</style><body style='margin:0'><div class=o><div class=i><div id=t>t</div></div></div></body>");
    EQ_MSG(50, el("t")->w, "named");
    lays("<style>.o{--t:dark;container-name:o}.i{--t:light}@container x style(--t: dark){#t{width:50px}}</style><body style='margin:0'><div class=o><div class=i><div id=t>t</div></div></div></body>");
    EQ_MSG(640, el("t")->w, "no such name");
    lays("<style>#t{--t:dark}@container style(--t: dark){#t{width:50px}}</style><body style='margin:0'><div><div id=t>t</div></div></body>");
    EQ_MSG(640, el("t")->w, "the element itself is not its container");
    /* a pseudo-element queries its originating element */
    lays("<style>#t{--t:dark;float:left}#t::before{content:'ab'}@container style(--t: dark){#t::before{content:'abcd'}}</style><body style='margin:0'><div id=t></div></body>");
    EQ_MSG(32, el("t")->w, "pseudo-element");
    /* with size features: the one container needs a type */
    EQ_MSG(1, sq_holds_css(".c{container-type:inline-size;width:300px}", "style(--t: dark) and (min-width: 100px)"), "mixed");
    EQ_MSG(0, sq_holds_css(".c{container-type:inline-size;width:300px}", "style(--t: dark) and (min-width: 400px)"), "mixed false");
    EQ_MSG(0, sq_holds("style(--t: dark) and (min-width: 100px)"), "mixed without a type");
    /* style features alone need no layout of the container */
    lays("<style>.c{--t:dark}@container style(--t: dark){#t{width:50px}}</style><body style='margin:0'><div class=c><div id=t>t</div></div></body>");
    EQ_MSG(0, g_doc.cq_restyles, "no restyle pass");
    EQ_MSG(50, el("t")->w, "pure style");
    /* rules inside set custom properties used by the element */
    lays("<style>.c{--t:dark}@container style(--t: dark){#t{--w:70px}}#t{width:var(--w,20px)}</style><body style='margin:0'><div class=c><div id=t>t</div></div></body>");
    EQ_MSG(70, el("t")->w, "custom property from the rule");
    EQ_MSG(0, diag_count("container query refused"), "no refusal");
}

static void test_l120_style_refused(void) {
    static const char *const bad[] = {
        "style()", "style(color: red)", "style(--t:)", "style(--)", "style(-t: dark)", "style(t: dark)",
        "style(--t dark)", "style(style(--t: dark))", "style((--t: dark) and (--n: 1 2) or (--t))",
        "style((width > 1px))", "style(--t: dark", "style (--t: dark)", "style(--t: dark) and",
        "style(--t$: dark)", "style((--t: dark) and)", "style(-tt: dark)", "style(t-t: dark)", "style((((((--t: dark))))))", "style(--t: dark) style(--t)", 0 };
    int k;
    for (k = 0; bad[k]; k++) {
        int r = sq_holds(bad[k]);
        if (r != 0 || diag_count("container query refused") != 1) printf("  refused: %s (%d, %d)\n", bad[k], r, diag_count("container query refused"));
        EQ_MSG(0, r, bad[k]);
        EQ_MSG(1, diag_count("container query refused"), bad[k]);
    }
    /* a name and a style query, and a name called style */
    EQ_MSG(1, sq_holds_css(".c{container-name:card}", "card style(--t: dark)"), "card style");
    EQ_MSG(1, sq_holds_css(".c{container-name:style}", "style"), "a name");
    EQ_MSG(0, diag_count("container query refused"), "named ok");
}

/* --- the angle units are the hue's only --- */
static uint32_t color_of(const char *css) {
    char h[400];
    sprintf(h, "<body><div id=k style='color:#010203;color:%s'>k</div></body>", css);
    lays(h);
    return el("k")->color;
}

static void test_l120_hue_units(void) {
    static const struct { const char *css; uint32_t want; } cases[] = {
        { "rgb(10deg 20 30)", 0x010203u },
        { "rgb(10deg, 20, 30)", 0x010203u },
        { "rgb(10 20deg 30)", 0x010203u },
        { "rgb(10 20 30deg)", 0x010203u },
        { "rgba(10, 20, 30, 1deg)", 0x010203u },
        { "rgb(0 0 0 / 1deg)", 0x010203u },
        { "rgb(10 20 30)", 0x0a141eu },
        { "rgb(10DEG 20 30)", 0x010203u },
        { "hsl(120deg 100% 50%)", 0x00ff00u },
        { "hsl(120DEG, 100%, 50%)", 0x00ff00u },
        { "hsl(120 100deg 50%)", 0x010203u },
        { "hsl(120 100% 50deg)", 0x010203u },
        { "hsl(0.5turn 100% 50%)", 0x00ffffu },
        { "hwb(120deg 0% 0%)", 0x00ff00u },
        { "hwb(120 0deg 0%)", 0x010203u },
        { "rgb(from red 10deg g b)", 0x010203u },
        { "rgb(from red r g 10deg)", 0x010203u },
        { "hsl(from red 120deg s l)", 0x00ff00u },
        { "hsl(from red h 100deg l)", 0x010203u },
    };
    unsigned k;
    for (k = 0; k < sizeof(cases) / sizeof(cases[0]); k++) EQ_MSG(cases[k].want, color_of(cases[k].css), cases[k].css);
    /* the transform numbers are unitless too */
    lays("<div id=a style='transform:matrix(1,0,0,1,10deg,0)'>a</div><div id=b style='transform:matrix(1,0,0,1,10,0)'>b</div>"
         "<div id=c style='transform:scale(1deg)'>c</div><div id=d style='transform:scale(2)'>d</div>");
    EQ_MSG(0, el("a")->tf_count, "matrix deg");
    TEST_ASSERT_TRUE(el("b")->tf_count > 0);
    EQ_MSG(0, el("c")->tf_count, "scale deg");
    TEST_ASSERT_TRUE(el("d")->tf_count > 0);
}

int main(void) {
    unity_init();
    RUN_TEST(test_l120_root_height);
    RUN_TEST(test_l120_icb_absolute);
    RUN_TEST(test_l120_canvas);
    RUN_TEST(test_l120_inspect);
    RUN_TEST(test_l120_style_query);
    RUN_TEST(test_l120_style_refused);
    RUN_TEST(test_l120_hue_units);
    unity_print_results();
    unity_cleanup();
    return unity_stats.tests_failed == 0 ? 0 : 1;
}
