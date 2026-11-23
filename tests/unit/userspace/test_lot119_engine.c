/* Lot 119 (US-031): container queries (CSS Containment 3: @container,
 * container-type, container-name, container, the cqw/cqh/cqi/cqb/cqmin/
 * cqmax units, size and inline-size containment) and relative colors
 * (CSS Color 5 4: rgb/hsl/hwb(from <color> ...)). Values follow
 * docs/lot119_plan.md. */
#include "../../framework/unity.h"
#include <string.h>
#include <stdio.h>
#include "webeng.h"

/* an equality that names the failing case */
#define EQ_MSG(e, a, m) do { int e_ = (int)(e), a_ = (int)(a); \
    if (e_ != a_) printf("  %s: expected %d (0x%08x), got %d (0x%08x)\n", (m), e_, (unsigned)e_, a_, (unsigned)a_); \
    TEST_ASSERT_EQUAL(e_, a_); } while (0)
#define EQ_HEX(e, a) EQ_MSG(e, a, #a)

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
    g_none.h = -1;
    g_none.w = -1;
    return i >= 0 ? &g_doc.nodes[i] : &g_none;
}

static int diag_count(const char *t) {
    uint32_t i;
    int c = 0;
    for (i = 0; i < g_doc.ndiag; i++) if (strstr(g_doc.diag[i], t)) c++;
    return c;
}

static uint32_t color_of(const char *css) {
    char h[400];
    sprintf(h, "<body><div id=k style='color:#010203;color:%s'>k</div></body>", css);
    lays(h);
    return el("k")->color;
}

/* the width of #t, #u, ... after laying html out (-1: no such id) */
static int w_of(const char *id) { return el(id)->w; }

#define CQ_HEAD "<style>body{margin:0}.c{container-type:inline-size;width:300px}"
#define CQ_BODY "<body><div class=c><div id=t>t</div><div id=u>u</div></div></body>"

/* one condition on a 300 x auto inline-size container: whether #t's rule
 * applied (width 50 px; the content box is 300 px wide) */
static int cq_holds(const char *cond) {
    char h[600];
    sprintf(h, CQ_HEAD "@container %s{#t{width:50px}}</style>" CQ_BODY, cond);
    lays(h);
    return w_of("t") == 50 ? 1 : w_of("t") == 300 ? 0 : -1;
}

static void test_l119_cq_width(void) {
    TEST_ASSERT_EQUAL(1, cq_holds("(min-width: 200px)"));
    TEST_ASSERT_EQUAL(1, cq_holds("(min-width: 300px)"));
    TEST_ASSERT_EQUAL(0, cq_holds("(min-width: 300.5px)"));
    TEST_ASSERT_EQUAL(1, cq_holds("(max-width: 300px)"));
    TEST_ASSERT_EQUAL(0, cq_holds("(max-width: 299.99px)"));
    TEST_ASSERT_EQUAL(1, cq_holds("(min-width: 299.9px)"));
    TEST_ASSERT_EQUAL(1, cq_holds("(max-width: 300.09px)"));
    TEST_ASSERT_EQUAL(1, cq_holds("(width: 300px)"));
    TEST_ASSERT_EQUAL(0, cq_holds("(width: 301px)"));
    TEST_ASSERT_EQUAL(1, cq_holds("(inline-size: 300px)"));
    TEST_ASSERT_EQUAL(1, cq_holds("(min-inline-size: 300px)"));
    TEST_ASSERT_EQUAL(1, cq_holds("(width > 299px)"));
    TEST_ASSERT_EQUAL(0, cq_holds("(width > 300px)"));
    TEST_ASSERT_EQUAL(1, cq_holds("(width >= 300px)"));
    TEST_ASSERT_EQUAL(1, cq_holds("(width < 301px)"));
    TEST_ASSERT_EQUAL(0, cq_holds("(width < 300px)"));
    TEST_ASSERT_EQUAL(1, cq_holds("(width <= 300px)"));
    TEST_ASSERT_EQUAL(1, cq_holds("(width = 300px)"));
    TEST_ASSERT_EQUAL(1, cq_holds("(299px < width)"));
    TEST_ASSERT_EQUAL(0, cq_holds("(300px < width)"));
    TEST_ASSERT_EQUAL(1, cq_holds("(300px <= width)"));
    TEST_ASSERT_EQUAL(1, cq_holds("(301px > width)"));
    TEST_ASSERT_EQUAL(1, cq_holds("(300px = width)"));
    TEST_ASSERT_EQUAL(1, cq_holds("(250px <= width <= 300px)"));
    TEST_ASSERT_EQUAL(0, cq_holds("(250px <= width < 300px)"));
    TEST_ASSERT_EQUAL(0, cq_holds("(300px < width <= 400px)"));
    TEST_ASSERT_EQUAL(1, cq_holds("(400px > width >= 300px)"));
    TEST_ASSERT_EQUAL(0, cq_holds("(400px > width > 300px)"));
    /* em is the container's font (16 px), rem the root's (16 px) */
    TEST_ASSERT_EQUAL(1, cq_holds("(min-width: 18.75em)"));
    TEST_ASSERT_EQUAL(0, cq_holds("(min-width: 18.76em)"));
    TEST_ASSERT_EQUAL(1, cq_holds("(max-width: 18.75rem)"));
    TEST_ASSERT_EQUAL(0, cq_holds("(max-width: 18.74rem)"));
    TEST_ASSERT_EQUAL(1, cq_holds("(width)"));
    lays("<style>body{margin:0}.c{container-type:inline-size;width:0}@container (width){#t{height:5px}}"
         "@container (not (width)){#u{height:6px}}</style>" CQ_BODY);
    TEST_ASSERT_TRUE(el("t")->h != 5);
    TEST_ASSERT_EQUAL(6, el("u")->h);
    TEST_ASSERT_EQUAL(1, cq_holds("(min-width: 0)"));
    lays("<style>body{margin:0}.c{container-type:inline-size;width:300px;font-size:20px}"
         "@container (min-width: 15em){#t{width:50px}}@container (min-width: 15.01em){#u{width:50px}}</style>" CQ_BODY);
    TEST_ASSERT_EQUAL(50, w_of("t"));
    TEST_ASSERT_EQUAL(300, w_of("u"));
    /* the content box: padding and borders out */
    lays("<style>body{margin:0}.c{container-type:inline-size;width:300px;padding:0 7px;border:3px solid red}"
         "@container (width: 300px){#t{width:50px}}@container (width: 320px){#u{width:50px}}</style>" CQ_BODY);
    TEST_ASSERT_EQUAL(50, w_of("t"));
    TEST_ASSERT_EQUAL(300, w_of("u"));
    lays("<style>body{margin:0}.c{container-type:inline-size;width:300px;padding:0 7px;border:3px solid red;box-sizing:border-box}"
         "@container (width: 280px){#t{width:50px}}</style>" CQ_BODY);
    TEST_ASSERT_EQUAL(50, w_of("t"));
    /* the rules apply once (the sizes are new once) */
    lays(CQ_HEAD "@container (min-width: 200px){#t{width:50px}}</style>" CQ_BODY);
    TEST_ASSERT_EQUAL(1, g_doc.cq_restyles);
    TEST_ASSERT_EQUAL(1, g_doc.cq_used);
    TEST_ASSERT_EQUAL(0, diag_count("container"));
}

static void test_l119_cq_logic(void) {
    TEST_ASSERT_EQUAL(1, cq_holds("not (min-width: 400px)"));
    TEST_ASSERT_EQUAL(0, cq_holds("not (min-width: 200px)"));
    TEST_ASSERT_EQUAL(1, cq_holds("(min-width: 200px) and (max-width: 300px)"));
    TEST_ASSERT_EQUAL(0, cq_holds("(min-width: 200px) and (max-width: 299px)"));
    TEST_ASSERT_EQUAL(1, cq_holds("(min-width: 400px) or (max-width: 300px)"));
    TEST_ASSERT_EQUAL(0, cq_holds("(min-width: 400px) or (max-width: 299px)"));
    TEST_ASSERT_EQUAL(1, cq_holds("((min-width: 400px) or (width: 300px)) and (not (width < 10px))"));
    /* nested @container: both conditions; @media inside */
    lays(CQ_HEAD "@container (min-width: 200px){@container (max-width: 300px){#t{width:50px}}"
         "@container (max-width: 250px){#u{width:50px}}}</style>" CQ_BODY);
    TEST_ASSERT_EQUAL(50, w_of("t"));
    TEST_ASSERT_EQUAL(300, w_of("u"));
    lays(CQ_HEAD "@container (min-width: 400px){@container (max-width: 300px){#t{width:50px}}}"
         "@container (min-width: 200px){@media (min-width: 100px){#u{width:50px}}}</style>" CQ_BODY);
    TEST_ASSERT_EQUAL(300, w_of("t"));
    TEST_ASSERT_EQUAL(50, w_of("u"));
    lays(CQ_HEAD "@media (min-width: 100px){@container (min-width: 200px){#t{width:50px}}}"
         "@container (min-width: 200px){#u{width:40px}}#u{width:60px}</style>" CQ_BODY);
    TEST_ASSERT_EQUAL(50, w_of("t"));
    TEST_ASSERT_EQUAL(60, w_of("u"));     /* the cascade order holds */
    /* four nested groups (WE_CSS_NEST_MAX), not five */
    lays(CQ_HEAD "@container (min-width: 1px){@container (min-width: 2px){@container (min-width: 3px){"
         "@container (min-width: 4px){#t{width:50px}}}}}"
         "@container (min-width: 1px){@container (min-width: 2px){@container (min-width: 3px){"
         "@container (min-width: 4px){@container (min-width: 5px){#u{width:50px}}}}}}</style>" CQ_BODY);
    TEST_ASSERT_EQUAL(50, w_of("t"));
    TEST_ASSERT_EQUAL(300, w_of("u"));
    /* a rule after the block is not inside it */
    lays(CQ_HEAD "@container (min-width: 900px){#t{width:50px}}#u{width:70px}</style>" CQ_BODY);
    TEST_ASSERT_EQUAL(300, w_of("t"));
    TEST_ASSERT_EQUAL(70, w_of("u"));
}

static void test_l119_cq_container(void) {
    /* no container: unknown, the rules do not apply */
    lays("<style>body{margin:0}@container (min-width: 1px){#t{width:50px}}</style><body><div><div id=t>t</div></div></body>");
    TEST_ASSERT_EQUAL(640, w_of("t"));
    TEST_ASSERT_EQUAL(0, g_doc.cq_restyles);
    /* an element is not its own container */
    lays("<style>body{margin:0}#t{container-type:inline-size}@container (min-width: 1px){#t{width:50px}}</style>"
         "<body><div id=t>t</div></body>");
    TEST_ASSERT_EQUAL(640, w_of("t"));
    lays("<style>body{margin:0}#t{container-type:inline-size;width:300px}"
         "@container (min-width: 1px){#t{color:#00ff00 !important}}</style><body><div id=t>t</div></body>");
    TEST_ASSERT_EQUAL(0, (int)(el("t")->color & 0xffffff));
    /* the nearest container: inner 300, outer 500 */
    lays("<style>body{margin:0}.o{container-type:inline-size;width:500px}.c{container-type:inline-size;width:300px}"
         "@container (width: 300px){#t{width:50px}}</style><body><div class=o><div class=c><div id=t>t</div></div></div></body>");
    TEST_ASSERT_EQUAL(50, w_of("t"));
    /* a name: the nearest ancestor with it */
    lays("<style>body{margin:0}.o{container:card / inline-size;width:500px}.c{container-type:inline-size;width:300px}"
         "@container card (width: 500px){#t{width:50px}}@container side (min-width: 1px){#u{width:50px}}</style>"
         "<body><div class=o><div class=c><div id=t>t</div><div id=u>u</div></div></div></body>");
    TEST_ASSERT_EQUAL(50, w_of("t"));
    TEST_ASSERT_EQUAL(300, w_of("u"));
    /* a list of names; names are case-sensitive */
    lays("<style>body{margin:0}.c{container-type:inline-size;container-name:a  card b;width:300px}"
         "@container card (width: 300px){#t{width:50px}}@container Card (width: 300px){#u{width:50px}}</style>" CQ_BODY);
    TEST_ASSERT_EQUAL(50, w_of("t"));
    TEST_ASSERT_EQUAL(300, w_of("u"));
    /* a name alone: any type with the name */
    lays("<style>body{margin:0}.c{container-name:card;width:300px}.d{container:side;width:200px}"
         "@container card{#t{width:50px}}@container side{#u{width:50px}}</style>"
         "<body><div class=c><div id=t>t</div><div id=u>u</div></div></body>");
    TEST_ASSERT_EQUAL(50, w_of("t"));
    TEST_ASSERT_EQUAL(300, w_of("u"));
    /* a size feature needs a type: container-name alone is no container */
    lays("<style>body{margin:0}.c{container-name:card;width:300px}"
         "@container card (min-width: 1px){#t{width:50px}}@container (min-width: 1px){#u{width:50px}}</style>" CQ_BODY);
    TEST_ASSERT_EQUAL(300, w_of("t"));
    TEST_ASSERT_EQUAL(300, w_of("u"));
    /* container-type: normal, none name */
    lays("<style>body{margin:0}.c{container-type:inline-size;container-type:normal;width:300px}"
         "@container (min-width: 1px){#t{width:50px}}</style>" CQ_BODY);
    TEST_ASSERT_EQUAL(300, w_of("t"));
    lays("<style>body{margin:0}.c{container:card / inline-size;container-name:none;width:300px}"
         "@container card (min-width: 1px){#t{width:50px}}@container (min-width: 1px){#u{width:50px}}</style>" CQ_BODY);
    TEST_ASSERT_EQUAL(300, w_of("t"));
    TEST_ASSERT_EQUAL(50, w_of("u"));
    /* the shorthand without a type resets it */
    lays("<style>body{margin:0}.c{container-type:inline-size;container:card;width:300px}"
         "@container (min-width: 1px){#t{width:50px}}@container card{#u{width:50px}}</style>" CQ_BODY);
    TEST_ASSERT_EQUAL(300, w_of("t"));
    TEST_ASSERT_EQUAL(50, w_of("u"));
    lays("<style>body{margin:0}.c{container:none / inline-size;width:300px}"
         "@container (min-width: 1px){#t{width:50px}}</style>" CQ_BODY);
    TEST_ASSERT_EQUAL(50, w_of("t"));
    lays("<style>body{margin:0}.c{container-type:inline-size;container:none / normal;width:300px}"
         "@container (min-width: 1px){#t{width:50px}}</style>" CQ_BODY);
    TEST_ASSERT_EQUAL(300, w_of("t"));
    lays("<style>body{margin:0}.c{container-type:inline-size;container:none;width:300px}"
         "@container (min-width: 1px){#t{width:50px}}</style>" CQ_BODY);
    TEST_ASSERT_EQUAL(300, w_of("t"));
    /* a hidden container is none (its size is unknown, not 0) */
    lays("<style>body{margin:0}.c{container-type:inline-size;width:300px}.h{display:none;container-type:inline-size}"
         "@container (max-width: 10px){#t{color:#00ff00}}</style><body><div class=c><div class=h><div id=t>t</div></div></div></body>");
    TEST_ASSERT_EQUAL(0, el("t")->w);
    TEST_ASSERT_EQUAL(0, (int)(el("t")->color & 0xffffff));
    /* a named query skips a nearer element with the name but no type */
    lays("<style>body{margin:0}.o{container:card / inline-size;width:500px}.c{container-name:card;width:300px}"
         "@container card (width: 500px){#t{width:50px}}</style><body><div class=o><div class=c><div id=t>t</div></div></div></body>");
    TEST_ASSERT_EQUAL(50, w_of("t"));
    /* a name list: the second of two names of the same length */
    lays("<style>body{margin:0}.c{container:cart card / inline-size;width:300px}"
         "@container card (width: 300px){#t{width:50px}}</style>" CQ_BODY);
    TEST_ASSERT_EQUAL(50, w_of("t"));
    /* a pseudo-element queries its originating element */
    lays("<style>body{margin:0}#c{container-type:inline-size;width:300px}#c::before{content:'x';display:block;width:10cqw}"
         "@container (width: 300px){#c::before{color:#00ff00}}</style><body><div id=c>c</div></body>");
    {
        uint32_t i;
        int c = we_find_id(&g_doc, "c"), found = 0;
        for (i = g_doc.source_nodes; i < g_doc.nnodes; i++)
            if (g_doc.nodes[i].parent == c && !g_doc.nodes[i].is_text) {
                TEST_ASSERT_EQUAL(0x00ff00, (int)(g_doc.nodes[i].color & 0xffffff));
                TEST_ASSERT_EQUAL(30, g_doc.nodes[i].w);
                found = 1;
            }
        TEST_ASSERT_EQUAL(1, found);
    }
    /* the container is not inherited */
    lays("<style>body{margin:0}.o{container-type:inline-size;width:500px}.c{width:300px}"
         "@container (width: 500px){#t{width:50px}}</style><body><div class=o><div class=c><div id=t>t</div></div></div></body>");
    TEST_ASSERT_EQUAL(50, w_of("t"));
}

static void test_l119_cq_size(void) {
    /* block-axis features need container-type: size */
    lays("<style>body{margin:0}.c{container-type:inline-size;width:300px;height:100px}"
         "@container (min-height: 1px){#t{width:50px}}@container (orientation: landscape){#u{width:50px}}</style>" CQ_BODY);
    TEST_ASSERT_EQUAL(300, w_of("t"));
    TEST_ASSERT_EQUAL(300, w_of("u"));
    lays("<style>body{margin:0}.c{container-type:inline-size;width:300px;height:100px}"
         "@container (aspect-ratio > 1){#t{width:50px}}@container (orientation){#u{width:50px}}</style>" CQ_BODY);
    TEST_ASSERT_EQUAL(300, w_of("t"));
    TEST_ASSERT_EQUAL(300, w_of("u"));
    lays("<style>body{margin:0}.c{container:card / size;width:300px;height:100px}"
         "@container card (height: 100px){#t{width:50px}}@container (orientation: landscape){#u{width:50px}}</style>" CQ_BODY);
    TEST_ASSERT_EQUAL(50, w_of("t"));
    TEST_ASSERT_EQUAL(50, w_of("u"));
    lays("<style>body{margin:0}.c{container-type:size;width:300px;height:100px}"
         "@container (block-size > 100px){#t{width:50px}}@container (orientation: portrait){#u{width:50px}}</style>" CQ_BODY);
    TEST_ASSERT_EQUAL(300, w_of("t"));
    TEST_ASSERT_EQUAL(300, w_of("u"));
    lays("<style>body{margin:0}.c{container-type:size;width:100px;height:100px}"
         "@container (orientation: portrait){#t{width:50px}}@container (orientation){#u{width:40px}}</style>" CQ_BODY);
    TEST_ASSERT_EQUAL(50, w_of("t"));
    TEST_ASSERT_EQUAL(40, w_of("u"));
    lays("<style>body{margin:0}.c{container-type:size;width:300px;height:100px}"
         "@container (aspect-ratio: 3 / 1){#t{width:50px}}@container (min-aspect-ratio: 31/10){#u{width:50px}}</style>" CQ_BODY);
    TEST_ASSERT_EQUAL(50, w_of("t"));
    TEST_ASSERT_EQUAL(300, w_of("u"));
    lays("<style>body{margin:0}.c{container-type:size;width:300px;height:120px}"
         "@container (aspect-ratio > 2.4){#t{width:50px}}@container (aspect-ratio >= 5/2){#u{width:50px}}</style>" CQ_BODY);
    TEST_ASSERT_EQUAL(300, w_of("t"));
    TEST_ASSERT_EQUAL(50, w_of("u"));
    /* the height's content box (padding out) */
    lays("<style>body{margin:0}.c{container-type:size;width:300px;height:100px;padding:5px 0;border-top:2px solid red}"
         "@container (height: 100px){#t{width:50px}}</style>" CQ_BODY);
    TEST_ASSERT_EQUAL(50, w_of("t"));
    /* size containment: an auto height is that of an empty box */
    lays("<style>body{margin:0}.c{container-type:size;width:300px;padding:4px}</style>" CQ_BODY);
    TEST_ASSERT_EQUAL(8, el("t")->parent >= 0 ? g_doc.nodes[el("t")->parent].h : -1);
    lays("<style>body{margin:0}.c{container-type:inline-size;width:300px;padding:4px}</style>" CQ_BODY);
    TEST_ASSERT_TRUE(g_doc.nodes[el("t")->parent].h > 8);
    lays("<style>body{margin:0}.c{container-type:size;width:300px;height:30px}</style>" CQ_BODY);
    TEST_ASSERT_EQUAL(30, g_doc.nodes[el("t")->parent].h);
    /* inline-size containment: a shrink-to-fit box is as wide as an empty one */
    lays("<style>body{margin:0}.c{float:left;padding:0 5px}.k{container-type:inline-size}</style>"
         "<body><div class=c id=a>Hello world</div><div class='c k' id=b style='clear:left'>Hello world</div></body>");
    TEST_ASSERT_TRUE(el("a")->w > 40);
    TEST_ASSERT_EQUAL(10, el("b")->w);
    lays("<style>body{margin:0}.c{float:left;padding:0 5px;container-type:size;height:20px}</style>"
         "<body><div class=c id=b>Hello world</div></body>");
    TEST_ASSERT_EQUAL(10, el("b")->w);
}

static void test_l119_cq_units(void) {
    lays("<style>body{margin:0}.c{container-type:size;width:300px;height:200px}"
         "#t{width:10cqw;height:10cqh}#u{width:10cqi;height:10cqb}</style>" CQ_BODY);
    TEST_ASSERT_EQUAL(30, el("t")->w);
    TEST_ASSERT_EQUAL(20, el("t")->h);
    TEST_ASSERT_EQUAL(30, el("u")->w);
    TEST_ASSERT_EQUAL(20, el("u")->h);
    lays("<style>body{margin:0}.c{container-type:size;width:300px;height:200px}"
         "#t{width:10cqmin;height:10cqmax}#u{width:calc(50cqw - 10px);margin-left:1.5cqw}</style>" CQ_BODY);
    TEST_ASSERT_EQUAL(20, el("t")->w);
    TEST_ASSERT_EQUAL(30, el("t")->h);
    TEST_ASSERT_EQUAL(140, el("u")->w);
    TEST_ASSERT_EQUAL(5, el("u")->margin[3]);     /* 4.5 */
    /* no container: the viewport (640 x 480) */
    lays("<style>body{margin:0}#t{width:10cqw;height:10cqh}#u{width:10cqmax;height:10CQMIN}</style>"
         "<body><div id=t>t</div><div id=u>u</div></body>");
    TEST_ASSERT_EQUAL(64, el("t")->w);
    TEST_ASSERT_EQUAL(48, el("t")->h);
    TEST_ASSERT_EQUAL(64, el("u")->w);
    TEST_ASSERT_EQUAL(48, el("u")->h);
    TEST_ASSERT_EQUAL(0, g_doc.cq_restyles);
    lay_at("<style>body{margin:0}#t{width:10cqw;height:10cqh}</style><body><div id=t>t</div></body>", 800, 300);
    TEST_ASSERT_EQUAL(80, el("t")->w);
    TEST_ASSERT_EQUAL(30, el("t")->h);
    /* an inline-size container: cqh is the viewport's */
    lays("<style>body{margin:0}.c{container-type:inline-size;width:300px}#t{width:10cqw;height:10cqh}"
         "#u{width:10cqmin;height:10cqmax}</style>" CQ_BODY);
    TEST_ASSERT_EQUAL(30, el("t")->w);
    TEST_ASSERT_EQUAL(48, el("t")->h);
    TEST_ASSERT_EQUAL(30, el("u")->w);
    TEST_ASSERT_EQUAL(48, el("u")->h);
    /* the viewport stands in for cqh: a new viewport height restyles */
    lay_at("<style>body{margin:0}.c{container-type:inline-size;width:300px}#t{width:10cqw;height:10cqh}</style>" CQ_BODY, 640, 480);
    TEST_ASSERT_EQUAL(48, el("t")->h);
    TEST_ASSERT_EQUAL(0, we_layout_viewport(&g_doc, 640, 300));
    TEST_ASSERT_EQUAL(30, el("t")->h);
    TEST_ASSERT_EQUAL(30, el("t")->w);
    /* each axis from its nearest container */
    lays("<style>body{margin:0}.o{container-type:size;width:500px;height:150px}.c{container-type:inline-size;width:300px}"
         "#t{width:10cqw;height:10cqh}</style><body><div class=o><div class=c><div id=t>t</div></div></div></body>");
    TEST_ASSERT_EQUAL(30, el("t")->w);
    TEST_ASSERT_EQUAL(15, el("t")->h);
    /* a 0 px container is 0, not the viewport */
    lays("<style>body{margin:0}.c{container-type:size;width:0;height:0}#t{width:10cqw;height:calc(10cqh + 3px)}</style>"
         "<body><div class=c><div id=t>t</div></div></body>");
    TEST_ASSERT_EQUAL(0, el("t")->w);
    TEST_ASSERT_EQUAL(3, el("t")->h);
    /* the API: a context's container, else the viewport */
    {
        we_unit_ctx_t c;
        int type, milli, h100;
        memset(&c, 0, sizeof(c));
        c.font100 = 1600; c.root100 = 1600; c.vw = 1000; c.vh = 500;
        TEST_ASSERT_EQUAL(0, we_math_milli("calc(1cqw)", 10, &c, 0, &type, &milli)); TEST_ASSERT_EQUAL(10000, milli);
        TEST_ASSERT_EQUAL(0, we_math_milli("calc(1cqh)", 10, &c, 0, &type, &milli)); TEST_ASSERT_EQUAL(5000, milli);
        c.cqw = 200; c.cqh = 300;
        TEST_ASSERT_EQUAL(0, we_math_milli("calc(1cqw)", 10, &c, 0, &type, &milli)); TEST_ASSERT_EQUAL(2000, milli);
        TEST_ASSERT_EQUAL(0, we_math_milli("calc(1cqb)", 10, &c, 0, &type, &milli)); TEST_ASSERT_EQUAL(3000, milli);
        TEST_ASSERT_EQUAL(0, we_math_milli("calc(1cqmin)", 12, &c, 0, &type, &milli)); TEST_ASSERT_EQUAL(2000, milli);
        TEST_ASSERT_EQUAL(0, we_math_milli("calc(1cqmax)", 12, &c, 0, &type, &milli)); TEST_ASSERT_EQUAL(3000, milli);
        TEST_ASSERT_EQUAL(0, we_math_milli("calc(1cqi)", 10, &c, 0, &type, &milli)); TEST_ASSERT_EQUAL(2000, milli);
        TEST_ASSERT_EQUAL(0, we_unit_len100("2.5cqw", 6, &c, &h100)); TEST_ASSERT_EQUAL(500, h100);
        c.cqh = 0;
        TEST_ASSERT_EQUAL(0, we_math_milli("calc(1cqh)", 10, &c, 0, &type, &milli)); TEST_ASSERT_EQUAL(5000, milli);
        c.cqw = WE_LAYOUT_MAX; c.cqh = WE_LAYOUT_MAX;
        TEST_ASSERT_EQUAL(0, we_math_milli("calc(1cqw)", 10, &c, 0, &type, &milli)); TEST_ASSERT_EQUAL(WE_LAYOUT_MAX * 10, milli);
        TEST_ASSERT_EQUAL(0, we_math_milli("calc(1cqh)", 10, &c, 0, &type, &milli)); TEST_ASSERT_EQUAL(WE_LAYOUT_MAX * 10, milli);
        c.cqw = WE_LAYOUT_MAX + 1;
        TEST_ASSERT_EQUAL(0, we_math_milli("calc(1cqw)", 10, &c, 0, &type, &milli)); TEST_ASSERT_EQUAL(10000, milli);
    }
}

static void test_l119_cq_restyle(void) {
    /* a container sized in cqw of the one around it: one restyle per level */
    lays("<style>body{margin:0}.a{container-type:inline-size;width:400px}.b{container-type:inline-size;width:50cqw}"
         "#t{width:50cqw}</style><body><div class=a><div class=b id=b><div id=t>t</div></div></div></body>");
    TEST_ASSERT_EQUAL(200, el("b")->w);
    TEST_ASSERT_EQUAL(100, el("t")->w);
    TEST_ASSERT_EQUAL(2, g_doc.cq_restyles);
    lays("<style>body{margin:0}.a{container-type:inline-size;width:400px}.b{container-type:inline-size;width:50cqw}"
         "#t{width:50cqw}</style><body><div class=a><div class=b><div class=b id=c><div id=t>t</div></div></div></div></body>");
    TEST_ASSERT_EQUAL(100, el("c")->w);
    TEST_ASSERT_EQUAL(50, el("t")->w);
    TEST_ASSERT_EQUAL(3, g_doc.cq_restyles);
    TEST_ASSERT_EQUAL(0, diag_count("container query unsettled"));
    /* one level more than WE_CQ_PASSES restyles settle: the last sizes stay */
    lays("<style>body{margin:0}.a{container-type:inline-size;width:400px}.b{container-type:inline-size;width:50cqw}"
         "#t{width:50cqw}</style><body><div class=a><div class=b><div class=b><div class=b id=d><div id=t>t</div>"
         "</div></div></div></div></body>");
    TEST_ASSERT_EQUAL(3, g_doc.cq_restyles);
    TEST_ASSERT_EQUAL(1, diag_count("container query unsettled"));
    TEST_ASSERT_EQUAL(50, el("d")->w);
    TEST_ASSERT_EQUAL(40, el("t")->w);
    /* a query of a container sized by another query */
    lays("<style>body{margin:0}.a{container-type:inline-size;width:400px}.b{container-type:inline-size;width:300px}"
         "@container (min-width: 350px){.b{width:200px}}@container (max-width: 250px){#t{width:50px}}</style>"
         "<body><div class=a><div class=b id=b><div id=t>t</div></div></div></body>");
    TEST_ASSERT_EQUAL(200, el("b")->w);
    TEST_ASSERT_EQUAL(50, el("t")->w);
    /* a later layout at another viewport: the percentages follow */
    lay_at("<style>body{margin:0}.c{container-type:inline-size;width:50%}@container (min-width: 400px){#t{width:50px}}</style>"
           CQ_BODY, 640, 480);
    TEST_ASSERT_EQUAL(320, w_of("t"));
    TEST_ASSERT_EQUAL(0, we_layout_viewport(&g_doc, 900, 480));
    TEST_ASSERT_EQUAL(50, w_of("t"));
    TEST_ASSERT_EQUAL(0, we_layout_viewport(&g_doc, 640, 480));
    TEST_ASSERT_EQUAL(320, w_of("t"));
    /* a declaration a pass dropped is gone in the next one: #t is a
     * container named big only while #a is 400 px wide or more */
    lay_at("<style>body{margin:0}#a{container-type:inline-size;width:50%}#t{width:100px}"
           "@container (min-width: 400px){#t{container:big / inline-size}}"
           "@container (width: 100px){#u{width:50px}}@container big{#u{height:7px}}</style>"
           "<body><div id=a><div id=t><div id=u>u</div></div></div></body>", 900, 480);
    TEST_ASSERT_EQUAL(50, w_of("u"));
    TEST_ASSERT_EQUAL(7, el("u")->h);
    TEST_ASSERT_EQUAL(0, we_layout_viewport(&g_doc, 640, 480));
    TEST_ASSERT_EQUAL(100, w_of("u"));
    TEST_ASSERT_TRUE(el("u")->h != 7);
    TEST_ASSERT_EQUAL(0, diag_count("container query unsettled"));
    /* no container query, no unit: no restyle */
    lays("<style>body{margin:0}.c{container-type:inline-size;width:300px}</style>" CQ_BODY);
    TEST_ASSERT_EQUAL(0, g_doc.cq_used);
    TEST_ASSERT_EQUAL(0, g_doc.cq_restyles);
}

static void test_l119_cq_refused(void) {
    static const char *const bad[] = {
        "(min-width: 10%)", "(min-width: 10)", "(min-width: 10vw)", "(color)", "(width > )", "(width: 1px",
        "none (width: 1px)", "and (width: 1px)", "inherit (width: 1px)", "(width < 1px > 2px)",
        "(1px < width > 0px)", "(1px < height = 3px)", "(width = width)", "(1px < 2px)", "(min-orientation: portrait)",
        "(orientation: sideways)", "(width: 1px) and", "screen and (width: 1px)", "", "(width: 100001px)",
        "(width: 1.px)", "(width: 1px) , (height: 2px)", "(1px < width < 2px < 3px)", "(400px > width = 300px)", "(300px = width > 200px)", "(width < 1px < 2px)", "(max-aspect-ratio: 1px)", "(min-width: px)", "(width: .em)", "(1px < width < 2zz)", "(1zz < width < 2px)", "(width > 2zz)", "(2zz < width)", "(: 1px)", "(width: )", 0 };
    int k;
    for (k = 0; bad[k]; k++) {
        EQ_MSG(0, cq_holds(bad[k]), bad[k]);
        EQ_MSG(1, diag_count("container query refused"), bad[k]);
    }
    /* the table holds WE_CQ_MAX conditions */
    {
        char h[2400];
        int at = sprintf(h, CQ_HEAD);
        for (k = 0; k < WE_CQ_MAX; k++) at += sprintf(h + at, "@container (min-width: %dpx){#u{width:%dpx}}", k, 100 + k);
        sprintf(h + at, "@container (min-width: 1px){#t{width:50px}}</style>" CQ_BODY);
        lays(h);
        TEST_ASSERT_EQUAL(100 + WE_CQ_MAX - 1, w_of("u"));
        TEST_ASSERT_EQUAL(300, w_of("t"));
        TEST_ASSERT_EQUAL(1, diag_count("container query table full"));
        TEST_ASSERT_EQUAL(WE_CQ_MAX, g_doc.ncq);
    }
    /* declarations: a refused one keeps the earlier value */
    lays("<style>body{margin:0}.c{container-type:inline-size;container-type:block;width:300px}"
         "@container (min-width: 1px){#t{width:50px}}</style>" CQ_BODY);
    TEST_ASSERT_EQUAL(50, w_of("t"));
    TEST_ASSERT_EQUAL(1, diag_count("container type refused"));
    lays("<style>body{margin:0}.c{container:card / inline-size;container:side / bogus;width:300px}"
         "@container card (min-width: 1px){#t{width:50px}}</style>" CQ_BODY);
    TEST_ASSERT_EQUAL(50, w_of("t"));
    TEST_ASSERT_EQUAL(1, diag_count("container refused"));
    lays("<style>body{margin:0}.c{container:card / inline-size;container:and / size;width:300px}"
         "@container card (min-width: 1px){#t{width:50px}}@container (min-height: 0){#u{width:50px}}</style>" CQ_BODY);
    TEST_ASSERT_EQUAL(50, w_of("t"));
    TEST_ASSERT_EQUAL(300, w_of("u"));
    TEST_ASSERT_EQUAL(1, diag_count("container refused"));
    lays("<style>body{margin:0}.c{container:card / inline-size;container: / size;container:card /;width:300px}"
         "@container card (min-width: 1px){#t{width:50px}}</style>" CQ_BODY);
    TEST_ASSERT_EQUAL(50, w_of("t"));
    TEST_ASSERT_EQUAL(2, diag_count("container refused"));
    lays("<style>body{margin:0}.c{container:card / inline-size;container-name:none card;container-name:default;"
         "container-name:1x;width:300px}@container card (min-width: 1px){#t{width:50px}}</style>" CQ_BODY);
    TEST_ASSERT_EQUAL(50, w_of("t"));
    TEST_ASSERT_EQUAL(3, diag_count("container name refused"));
    lays("<style>body{margin:0}.c{container:card / inline-size;container:and;width:300px}"
         "@container card (min-width: 1px){#t{width:50px}}</style>" CQ_BODY);
    TEST_ASSERT_EQUAL(50, w_of("t"));
    TEST_ASSERT_EQUAL(1, diag_count("container refused"));
    TEST_ASSERT_EQUAL(0, diag_count("container name refused"));
    /* a name list of at most 255 characters */
    {
        char h[1400];
        int len;
        for (len = 255; len <= 256; len++) {
            int at = sprintf(h, "<style>body{margin:0}.c{container-type:inline-size;container-name:");
            for (k = 0; k < 36; k++) at += sprintf(h + at, "name%02d ", k);       /* 252 characters */
            at += sprintf(h + at, len == 255 ? "abc" : "abcd");
            sprintf(h + at, ";width:300px}@container abc (min-width: 1px){#t{width:50px}}"
                    "@container abcd (min-width: 1px){#t{width:50px}}</style>" CQ_BODY);
            lays(h);
            TEST_ASSERT_EQUAL(len == 255 ? 50 : 300, w_of("t"));
            TEST_ASSERT_EQUAL(len == 255 ? 0 : 1, diag_count("container name refused"));
        }
    }
    {
        char h[1400];
        int at = sprintf(h, "<style>body{margin:0}.c{container:card / inline-size;container-name:");
        for (k = 0; k < 43; k++) at += sprintf(h + at, "name%02d ", k);   /* 301 characters */
        sprintf(h + at, ";width:300px}@container card (min-width: 1px){#t{width:50px}}</style>" CQ_BODY);
        lays(h);
        TEST_ASSERT_EQUAL(50, w_of("t"));
        TEST_ASSERT_EQUAL(1, diag_count("container name refused"));
    }
    /* selector() belongs to @supports only */
    TEST_ASSERT_EQUAL(0, cq_holds("selector(div)"));
    TEST_ASSERT_EQUAL(1, diag_count("container query refused"));
    lays("<style>body{margin:0}@supports selector(div){#t{width:50px}}</style><body><div id=t>t</div></body>");
    TEST_ASSERT_EQUAL(50, w_of("t"));
    /* @supports knows the properties */
    lays("<style>body{margin:0}@supports (container-type: size){#t{width:50px}}"
         "@supports (container-type: bogus){#u{width:50px}}</style><body><div id=t>t</div><div id=u>u</div></body>");
    TEST_ASSERT_EQUAL(50, w_of("t"));
    TEST_ASSERT_EQUAL(640, w_of("u"));
}

/* ---------- relative colors ---------- */
static const struct { const char *origin, *hsl, *hwb; } conv[] = {
    { "#c06030", "hsl(20.000 60.000 47.058)", "hwb(20.000 18.823 24.705)" },
    { "#3060c0", "hsl(220.000 60.000 47.058)", "hwb(220.000 18.823 24.705)" },
    { "#20c040", "hsl(132.000 71.428 43.921)", "hwb(132.000 12.549 24.705)" },
    { "#808080", "hsl(0.000 0.000 50.196)", "hwb(0.000 50.196 49.803)" },
    { "#ffffff", "hsl(0.000 0.000 100.000)", "hwb(0.000 100.000 0.000)" },
    { "#000000", "hsl(0.000 0.000 0.000)", "hwb(0.000 0.000 100.000)" },
    { "#123456", "hsl(210.000 65.384 20.392)", "hwb(210.000 7.058 66.274)" },
    { "#f0e010", "hsl(55.714 88.188 50.196)", "hwb(55.714 6.274 5.882)" },
    { "#a0b0c0", "hsl(210.000 20.253 69.019)", "hwb(210.000 62.745 24.705)" },
    { "#c03060", "hsl(340.000 60.000 47.058)", "hwb(340.000 18.823 24.705)" },
    { "#ff0080", "hsl(329.883 100.000 50.000)", "hwb(329.883 0.000 0.000)" },
};

static void test_l119_relative_convert(void) {
    unsigned k;
    char a[120];
    for (k = 0; k < sizeof(conv) / sizeof(conv[0]); k++) {
        uint32_t want;
        want = color_of(conv[k].hsl);
        sprintf(a, "hsl(from %s h s l)", conv[k].origin);
        EQ_MSG(want, color_of(a), a);
        want = color_of(conv[k].hwb);
        sprintf(a, "hwb(from %s h w b)", conv[k].origin);
        EQ_MSG(want, color_of(a), a);
        sprintf(a, "rgb(%s)", conv[k].origin);
        sprintf(a, "rgb(from %s r g b)", conv[k].origin);
        EQ_MSG(color_of(conv[k].origin), color_of(a), a);
    }
    /* the channels as numbers: hsl(from c 0 0 calc(x)) is a gray of x% */
    EQ_HEX(color_of("hsl(0 0 60)"), color_of("hsl(from #c06030 0 0 s)"));
    EQ_HEX(0x00000000u, color_of("hsl(from red 0 0 calc(h))"));        /* the hue of red is 0, not 360 */
    EQ_HEX(0x00000000u, color_of("hsl(from #ff8080 0 0 calc(h))"));
    EQ_HEX(color_of("hsl(0 0 20)"), color_of("hsl(from #c06030 0 0 calc(h))"));
    EQ_HEX(color_of("hsl(0 0 40)"), color_of("hsl(from #3060c0 0 0 calc(h - 180))"));
    EQ_HEX(color_of("hsl(0 0 40)"), color_of("hsl(from #c03060 0 0 calc(h - 300))"));
    EQ_HEX(color_of("hsl(0 0 12)"), color_of("hsl(from #20c040 0 0 calc(h - 120))"));
    EQ_HEX(color_of("hwb(0 50 0)"), color_of("hwb(from #808080 0 calc(w - 0.196) 0)"));
    EQ_HEX(color_of("hwb(0 0 50)"), color_of("hwb(from #808080 0 0 calc(b + 0.197))"));
}

static void test_l119_relative_rgb(void) {
    EQ_HEX(0x00102030u, color_of("rgb(from #102030 r g b)"));
    EQ_HEX(0x00302010u, color_of("rgb(from #102030 b g r)"));
    EQ_HEX(0x00102030u, color_of("rgb(from rgb(16 32 48) r g b)"));
    EQ_HEX(0x0020ff80u, color_of("rgb(from #102030 calc(r * 2) 255 50%)"));
    EQ_HEX(0x00002030u, color_of("rgb(from #102030 none g b)"));
    EQ_HEX(0x00101010u, color_of("rgb(from #102030 r r r)"));
    EQ_HEX(0x00ff0000u, color_of("rgb( from  red  r  g  b )"));
    EQ_HEX(0x0000ff00u, color_of("RGB(FROM red G R B)"));
    EQ_HEX(0x0000ff00u, color_of("rgba(from red g r b)"));
    EQ_HEX(0x00ff0000u, color_of("rgb(from red calc(r + 10) g b)"));
    EQ_HEX(0x00000000u, color_of("rgb(from red calc(r - 300) g b)"));
    EQ_HEX(0x00808080u, color_of("rgb(from red calc(r / 2) calc(r / 2) calc(r / 2 + g))"));
    EQ_HEX(0x00ff0000u, color_of("rgb(from hsl(0 100 50) r g b)"));
    /* the alpha: the origin's unless given */
    EQ_HEX(color_of("rgb(16 32 48 / 0.5)"), color_of("rgb(from rgb(16 32 48 / 0.5) r g b)"));
    EQ_HEX(color_of("rgb(16 32 48 / 0.5)"), color_of("rgb(from rgb(16 32 48 / 0.5) r g b / alpha)"));
    EQ_HEX(color_of("rgb(16 32 48 / 0.25)"), color_of("rgb(from rgb(16 32 48 / 0.5) r g b / calc(alpha / 2))"));
    EQ_HEX(color_of("rgb(16 32 48 / 0.25)"), color_of("rgb(from #102030 r g b / 25%)"));
    EQ_HEX(color_of("rgb(16 32 48 / 0.5)"), color_of("rgb(from #102030 r g b/0.5)"));
    EQ_HEX(color_of("rgb(16 32 48)"), color_of("rgb(from rgb(16 32 48 / 0.5) r g b / 1)"));
    EQ_HEX(color_of("rgb(48 32 16 / 0.2)"), color_of("rgb(from rgb(16 32 48 / 0.2) b g r)"));
    EQ_HEX(color_of("rgb(0 0 0 / 0)"), color_of("rgb(from transparent r g b)"));
    EQ_HEX(color_of("rgb(51 0 0)"), color_of("rgb(from rgb(0 0 0 / 0.2) calc(alpha * 255) g b / 1)"));
    /* hsl and hwb: hue math, nesting */
    EQ_HEX(0x0000ff00u, color_of("hsl(from red calc(h + 120) s l)"));
    EQ_HEX(0x000000ffu, color_of("hsl(from red 240 s l)"));
    EQ_HEX(0x0000ffffu, color_of("hsl(from red 0.5turn s l)"));
    EQ_HEX(0x00ffffffu, color_of("hsl(from red h s 100%)"));
    EQ_HEX(0x000000ffu, color_of("rgb(from hsl(from red calc(h + 120) s l) b r g)"));
    EQ_HEX(0x00808080u, color_of("hwb(from red h 50 b)") == 0x00ff8080u ? 0x00808080u : 0);
    EQ_HEX(color_of("hwb(0 50 0)"), color_of("hwb(from red h 50 b)"));
    EQ_HEX(color_of("hsl(120 100 25 / 0.5)"), color_of("hsl(from rgb(0 128 0 / 0.5) h s 25)"));
    /* in a stylesheet and for the background */
    lays("<style>#k{background-color:rgb(from #102030 b g r)}</style><body><div id=k>k</div></body>");
    EQ_HEX(0x00302010u, el("k")->bg);
}

static void test_l119_relative_refused(void) {
    static const char *const bad[] = {
        "rgb(from red r g)", "rgb(from red r g b alpha)", "rgb(from red r, g, b)", "rgb(from nosuch r g b)",
        "rgb(from red r g x)", "rgb(from red r g b / alpha / 1)", "rgb(from red h s l)", "hsl(from red r s l)",
        "rgb(from red calc(h) g b)", "hsl(from red calc(h + 240deg) s l)", "hsl(from red 10% s l)", "rgb(from r g b)", "rgb(from  )", "rgb(fromred r g b)",
        "rgb(from red / 1)", "rgb(from red r g / b 1)", "rgb(from-red r g b)", "rgb(from red r g b / 1 2)", "rgb(from red r , g , b)", "hwb(from red h s l)", "rgb(from red r g b /)", "rgb(from currentcolor r g b)",
        "rgb(from red r g b c)", "rgb(from red r g calc(b + 1px))", "rgb(from red alpha(r) g b)", 0 };
    int k;
    for (k = 0; bad[k]; k++) EQ_MSG(0x00010203u, color_of(bad[k]), bad[k]);
    /* a refused color sets no background */
    lays("<style>#k{background-color:rgb(from red r g)}#j{background-color:hsl(from red 10% s l)}"
         "#i{background-color:hsl(from red h s l)}</style><body><div id=k>k</div><div id=j>j</div><div id=i>i</div></body>");
    TEST_ASSERT_EQUAL(0, el("k")->has_bg);
    TEST_ASSERT_EQUAL(0, el("j")->has_bg);
    TEST_ASSERT_EQUAL(1, el("i")->has_bg);
    /* the keywords are out of scope after the color */
    {
        we_unit_ctx_t c;
        int type, milli;
        memset(&c, 0, sizeof(c));
        EQ_HEX(0x00ff0000u, color_of("rgb(from red r g b)"));
        TEST_ASSERT_EQUAL(-1, we_math_milli("calc(r)", 7, &c, 0, &type, &milli));
        TEST_ASSERT_EQUAL(0x00010203u, color_of("rgb(from red r g x)"));
        TEST_ASSERT_EQUAL(-1, we_math_milli("calc(alpha)", 11, &c, 0, &type, &milli));
    }
}

int main(void) {
    unity_init();
    RUN_TEST(test_l119_cq_width);
    RUN_TEST(test_l119_cq_logic);
    RUN_TEST(test_l119_cq_container);
    RUN_TEST(test_l119_cq_size);
    RUN_TEST(test_l119_cq_units);
    RUN_TEST(test_l119_cq_restyle);
    RUN_TEST(test_l119_cq_refused);
    RUN_TEST(test_l119_relative_convert);
    RUN_TEST(test_l119_relative_rgb);
    RUN_TEST(test_l119_relative_refused);
    unity_print_results();
    unity_cleanup();
    return unity_stats.tests_failed == 0 ? 0 : 1;
}
