/* Lot 117 (US-031): font-relative lengths in the engine. em is the
 * element's own font size wherever the engine reads a length (Lot 116 and
 * before: 10 px), rem the root element's, ex, ch, cap and ic the 8x8 glyph
 * metrics at the element's scale, lh its line height and rlh the root's;
 * font-size keeps hundredths of a px and reads those units (against the
 * parent) and math; line-height reads them (lh of the parent) and math;
 * line-clamp counts the carried fraction; the reader passes a number line
 * height for a font size in hundredths. Hand cases next to the generated
 * test_lot117_*.c; expected values follow docs/lot117_plan.md. */
#include "../../framework/unity.h"
#include <string.h>
#include <stdio.h>
#include "webeng.h"
#include "wf_html.h"

static we_doc_t g_doc;
static we_forms_t g_forms;
static char g_html[8192], g_out[65536];
static we_node_t g_none;

static void lay(const char *html, int n) {
    we_forms_init(&g_forms);
    we_set_forms(&g_doc, &g_forms);
    TEST_ASSERT_EQUAL(0, we_parse(&g_doc, html, n));
    TEST_ASSERT_EQUAL(0, we_style(&g_doc));
    TEST_ASSERT_EQUAL(0, we_layout_viewport(&g_doc, 640, 480));
}

static void lays(const char *html) { lay(html, (int)strlen(html)); }

static const we_node_t *el(const char *id) {
    int i = we_find_id(&g_doc, id);
    g_none.h = -1;
    g_none.w = -1;
    return i >= 0 ? &g_doc.nodes[i] : &g_none;
}

static const we_node_t *nth(const char *t, int k) {
    int i;
    for (i = 0; i < (int)g_doc.nnodes; i++)
        if (!strcmp(g_doc.nodes[i].tag, t) && !k--) return &g_doc.nodes[i];
    g_none.h = -1;
    return &g_none;
}

static int diag_count(const char *t) {
    uint32_t i;
    int c = 0;
    for (i = 0; i < g_doc.ndiag; i++) if (strstr(g_doc.diag[i], t)) c++;
    return c;
}

static int text_y(const char *t) {
    uint32_t i;
    for (i = 0; i < g_doc.nitems; i++)
        if (g_doc.items[i].kind == OS_FB_WEB_TEXT && !strcmp(g_doc.items[i].text, t)) return g_doc.items[i].y;
    return -1000;
}

#define KF "@keyframes k { from { color: #000000 } to { color: #ffffff } } "

/* the reader's page laid out at 640 x 480 */
static void out_cat(void *ctx, const char *t) {
    (void)ctx;
    if (strlen(g_out) + strlen(t) < sizeof(g_out)) strcat(g_out, t);
}

static void reduce(const char *css, const char *body) {
    wf_html_stats_t st;
    int n;
    snprintf(g_html, sizeof(g_html), "<html><head><style>" KF "%s</style></head><body>%s</body></html>", css, body);
    wf_html_set_sitecss(1);
    n = wf_html_reduce(g_html, (int)strlen(g_html), 0, g_out, 65536, &st);
    wf_html_set_sitecss(0);
    TEST_ASSERT_TRUE(n > 0);
    lay(g_out, n);
}

#define L4 "a<br>b<br>c<br>d"

/* em is the element's own font, whatever the order of its declarations
 * (the font pass applies font-size, then line-height, first) */
static void test_l117_em_own_font(void) {
    lays("<body style='margin:0'><div id=a style='margin-left:2em;padding-left:1.5em;font-size:20px'>a</div>"
         "<div id=b style='font-size:20px;line-height:1.5;padding-top:1lh;line-height:2'>b</div>"
         "<div style='font-size:20px'><div id=c style='font-size:1.5em;margin-left:1em'>c</div></div>"
         "<div id=d style='margin:1.5em'>d</div></body>");
    TEST_ASSERT_EQUAL(40, el("a")->margin[3]);       /* Lot 116: 20 (em = 10 px) */
    TEST_ASSERT_EQUAL(30, el("a")->padding[3]);
    TEST_ASSERT_EQUAL(40, el("b")->padding[0]);      /* lh: 2 x 20 */
    TEST_ASSERT_EQUAL(30, el("c")->font_px);         /* em in font-size: the parent's */
    TEST_ASSERT_EQUAL(30, el("c")->margin[3]);       /* em elsewhere: its own */
    TEST_ASSERT_EQUAL(24, el("d")->margin[0]);       /* 1.5 x 16 */
    TEST_ASSERT_EQUAL(24, el("d")->margin[1]);
}

/* rem and rlh: the root element's, their initial values in its own
 * font-size and line-height (CSS Values 4 6.1.1) */
static void test_l117_root_units(void) {
    lays("<html style='font-size:2rem;line-height:2rlh;padding-left:1rem'><body style='margin:0'>"
         "<div id=d style='width:2rem;height:1rlh'>d</div><div id=e style='font-size:0.5rem;line-height:0.5rlh'>e</div>"
         "</body></html>");
    TEST_ASSERT_EQUAL(32, nth("html", 0)->font_px);   /* 2 x the initial 16 */
    TEST_ASSERT_EQUAL(20, nth("html", 0)->line_height);   /* 2 x the initial normal 10 */
    TEST_ASSERT_EQUAL(32, nth("html", 0)->padding[3]);    /* rem after the root's font: 32 */
    TEST_ASSERT_EQUAL(64, el("d")->w);
    TEST_ASSERT_EQUAL(20, el("d")->h);
    TEST_ASSERT_EQUAL(16, el("e")->font_px);
    TEST_ASSERT_EQUAL(10, el("e")->line_height);
    TEST_ASSERT_EQUAL(0, el("e")->lh_frac);
    /* the root's line-height set by its own rem: the initial 16 px */
    lays("<html style='font-size:30px;line-height:1rem'><body style='margin:0'><div id=d style='height:1rem'>d</div></body></html>");
    TEST_ASSERT_EQUAL(16, nth("html", 0)->line_height);
    TEST_ASSERT_EQUAL(30, el("d")->h);
}

/* lh in line-height is the parent's line height; lh elsewhere its own */
static void test_l117_lh_unit(void) {
    lays("<body style='margin:0'><div style='line-height:20px'><div id=a style='line-height:1.5lh;padding-left:1lh'>a</div>"
         "<div id=b style='padding-left:1.25lh'>b</div><div id=c style='line-height:calc(1lh + 0.5em)'>c</div></div></body>");
    TEST_ASSERT_EQUAL(30, el("a")->line_height);
    TEST_ASSERT_EQUAL(30, el("a")->padding[3]);
    TEST_ASSERT_EQUAL(25, el("b")->padding[3]);
    TEST_ASSERT_EQUAL(28, el("c")->line_height);     /* 20 + 8 */
    TEST_ASSERT_EQUAL(30, el("a")->h);
}

/* the font pass is quiet: a refused font-size or line-height is reported
 * once, each declaration counted once */
static void test_l117_quiet_pass(void) {
    lays("<body style='margin:0'><div id=a style='font-size:bogus;line-height:x;margin:1em;font-size:20px'>a</div></body>");
    TEST_ASSERT_EQUAL(1, diag_count("font size refused"));
    TEST_ASSERT_EQUAL(1, diag_count("line height refused"));
    TEST_ASSERT_EQUAL(2, (int)g_doc.ndiag);
    TEST_ASSERT_EQUAL(5, (int)g_doc.perf.decls_applied);
    TEST_ASSERT_EQUAL(20, el("a")->margin[0]);
    TEST_ASSERT_EQUAL(13, el("a")->h);               /* normal at 20 px: q 5 */
}

/* the cascade order holds in the font pass: importance and specificity */
static void test_l117_pass_cascade(void) {
    lays("<html><head><style>#e { font-size: 10px !important } div#e { font-size: 30px; margin-left: 2em } "
         "#f { line-height: 3 !important } #f { line-height: 1; padding-left: 1lh } "
         "#g { --f: 22.5px } #h { font-size: var(--f); margin-left: 2em }</style></head>"
         "<body style='margin:0'><div id=e style='font-size:40px'>e</div><div id=f style='line-height:2'>f</div>"
         "<div id=g><div id=h>h</div></div></body></html>");
    TEST_ASSERT_EQUAL(10, el("e")->font_px);
    TEST_ASSERT_EQUAL(20, el("e")->margin[3]);
    TEST_ASSERT_EQUAL(300, el("f")->line_height);
    TEST_ASSERT_EQUAL(48, el("f")->padding[3]);       /* 3 x 16 */
    TEST_ASSERT_EQUAL(2250, el("h")->font100);
    TEST_ASSERT_EQUAL(45, el("h")->margin[3]);
}

/* the font size keeps hundredths: UA heading sizes, small and big, a
 * number line height and em of a fractional font */
static void test_l117_font_hundredths(void) {
    lays("<body style='margin:0'><div id=p style='font-size:13.5px;line-height:1.5'><h1 id=h1>a</h1><h5 id=h5>b</h5>"
         "<small id=s>c</small><big id=g>d</big><div id=q style='margin-left:3em'>" L4 "</div></div></body>");
    TEST_ASSERT_EQUAL(1350, el("p")->font100);
    TEST_ASSERT_EQUAL(14, el("p")->font_px);         /* 13.5 rounds half up */
    TEST_ASSERT_EQUAL(2700, el("h1")->font100);
    TEST_ASSERT_EQUAL(1121, el("h5")->font100);      /* 0.83 x 13.5 = 11.205 */
    TEST_ASSERT_EQUAL(11, el("h5")->font_px);
    TEST_ASSERT_EQUAL(1125, el("s")->font100);       /* 13.5 / 1.2 */
    TEST_ASSERT_EQUAL(1620, el("g")->font100);       /* 13.5 x 1.2 */
    TEST_ASSERT_EQUAL(41, el("q")->margin[3]);       /* 40.5 */
    TEST_ASSERT_EQUAL(81, el("q")->h);               /* 4 x 20.25 carried */
}

/* font-relative units and math in font-size, against the parent */
static void test_l117_font_units(void) {
    lays("<body style='margin:0'><div style='font-size:20px'><div id=a style='font-size:2ex'>a</div>"
         "<div id=b style='font-size:2ch'>b</div><div id=c style='font-size:2cap'>c</div><div id=d style='font-size:2ic'>d</div>"
         "<div id=e style='font-size:1lh'>e</div><div id=f style='font-size:1rlh'>f</div>"
         "<div id=g style='font-size:calc(50% + 0.5em)'>g</div><div id=h style='font-size:clamp(10px, 3em, 30px)'>h</div>"
         "<div id=i style='font-size:max(1px, -5px)'>i</div><div id=j style='font-size:0.2px'>j</div>"
         "<div id=k style='font-size:0.5px'>k</div><div id=l style='font-size:100px'>l</div>"
         "<div id=m style='font-size:10.555px'>m</div><div id=n style='font-size:9pt'>n</div>"
         "<div id=o style='font-size:62.5%'>o</div><div id=q style='font-size:calc(2)'>q</div></div></body>");
    TEST_ASSERT_EQUAL(1200, el("a")->font100);       /* ex of 20 px: 6 */
    TEST_ASSERT_EQUAL(2000, el("b")->font100);       /* ch: 10 */
    TEST_ASSERT_EQUAL(1800, el("c")->font100);       /* cap: 9 */
    TEST_ASSERT_EQUAL(4000, el("d")->font100);       /* ic: 1em */
    TEST_ASSERT_EQUAL(1300, el("e")->font100);       /* the parent's normal line: 13 */
    TEST_ASSERT_EQUAL(1000, el("f")->font100);       /* the root's: 10 */
    TEST_ASSERT_EQUAL(2000, el("g")->font100);
    TEST_ASSERT_EQUAL(3000, el("h")->font100);
    TEST_ASSERT_EQUAL(400, el("i")->font100);        /* 1 px, at least 4 */
    TEST_ASSERT_EQUAL(2000, el("j")->font100);       /* under half a px: refused */
    TEST_ASSERT_EQUAL(400, el("k")->font100);
    TEST_ASSERT_EQUAL(6400, el("l")->font100);
    TEST_ASSERT_EQUAL(1056, el("m")->font100);
    TEST_ASSERT_EQUAL(1200, el("n")->font100);
    TEST_ASSERT_EQUAL(1250, el("o")->font100);
    TEST_ASSERT_EQUAL(2000, el("q")->font100);
    TEST_ASSERT_EQUAL(2, diag_count("font size refused"));    /* j and q */
}

/* line-height math and units */
static void test_l117_lh_math(void) {
    lays("<body style='margin:0'><div style='font-size:20px'><div id=a style='line-height:calc(1em + 0.5ex)'>a</div>"
         "<div id=b style='line-height:calc(150%)'>b</div><div id=c style='line-height:min(2lh, 30px)'>c</div>"
         "<div id=d style='line-height:2.5rem'>d</div><div id=e style='line-height:1.333ch'>e</div>"
         "<div id=f style='line-height:15px;line-height:calc(1px - 2px)'>f</div><div id=g style='line-height:20px;line-height:-1rem'>g</div>"
         "<div id=h style='line-height:0rem'>h</div></div></body>");
    TEST_ASSERT_EQUAL(23, el("a")->line_height);     /* 20 + 3 */
    TEST_ASSERT_EQUAL(30, el("b")->line_height);
    TEST_ASSERT_EQUAL(26, el("c")->line_height);     /* 2 x the parent's 13 */
    TEST_ASSERT_EQUAL(40, el("d")->line_height);
    TEST_ASSERT_EQUAL(13, el("e")->line_height);     /* 1.333 x 10 */
    TEST_ASSERT_EQUAL(33, el("e")->lh_frac);
    TEST_ASSERT_EQUAL(0, el("e")->lh_unitless);
    TEST_ASSERT_EQUAL(15, el("f")->line_height);
    TEST_ASSERT_EQUAL(20, el("g")->line_height);
    TEST_ASSERT_EQUAL(0, el("h")->line_height);
    TEST_ASSERT_EQUAL(1, el("h")->lh_frac);
    TEST_ASSERT_EQUAL(2, diag_count("line height refused"));
}

/* the parsers that took px or em take the other units */
static void test_l117_parsers(void) {
    lays("<body style='margin:0'><div id=a style='font-size:20px;tab-size:2cap;outline-width:1rlh;column-width:3ch;"
         "text-underline-offset:1ex;letter-spacing:0.25em;word-spacing:1ic;border-width:0.5ex'>a</div>"
         "<div id=b style='height:20px;height:2.5lh;flex-basis:2em;margin-inline:1rem'>b</div>"
         "<div id=c style='position:relative;left:-1.5em;top:1rlh;tab-size:3zz;outline-width:2zz;height:-1em'>c</div>"
         "<div id=d style='transform:translateX(2em)'>d</div><div id=e style='font-size:20px;border-radius:0.5em'>e</div>"
         "<div id=f style='padding-block:1ch;min-height:3cap'>f</div></body>");
    TEST_ASSERT_EQUAL(18, el("a")->tab_px);
    TEST_ASSERT_EQUAL(10, el("a")->outline_w);
    TEST_ASSERT_EQUAL(30, el("a")->col_width);
    TEST_ASSERT_EQUAL(6, el("a")->underline_off);
    TEST_ASSERT_EQUAL(5, el("a")->letter_spacing);
    TEST_ASSERT_EQUAL(20, el("a")->word_spacing);
    TEST_ASSERT_EQUAL(3, el("a")->border);           /* 0.5 x 6 */
    TEST_ASSERT_EQUAL(25, el("b")->h);               /* 2.5 x the normal 10 */
    TEST_ASSERT_EQUAL(32, el("b")->flex_basis);
    TEST_ASSERT_EQUAL(16, el("b")->margin[3]);
    TEST_ASSERT_EQUAL(16, el("b")->margin[1]);
    TEST_ASSERT_EQUAL(-24, el("c")->offset_x);
    TEST_ASSERT_EQUAL(10, el("c")->offset_y);
    TEST_ASSERT_EQUAL(64, el("c")->tab_px);
    TEST_ASSERT_EQUAL(3, el("c")->outline_w);
    TEST_ASSERT_EQUAL(1, diag_count("tab size refused"));
    TEST_ASSERT_EQUAL(1, diag_count("outline refused"));
    TEST_ASSERT_EQUAL(10, el("c")->h);
    TEST_ASSERT_EQUAL(10, el("e")->rad_x[0]);
    TEST_ASSERT_EQUAL(8, el("f")->padding[0]);
    TEST_ASSERT_EQUAL(8, el("f")->padding[2]);
    TEST_ASSERT_EQUAL(37, el("f")->h);               /* 8 + min-height 3 x 7 (content box) + 8 */
}

/* inset, inset-inline and border-spacing read the units too (Lot 116: em
 * refused as a position offset, border-spacing em = 10 px) */
static void test_l117_insets(void) {
    lays("<body style='margin:0'><div id=a style='position:relative;font-size:20px;inset:1em'>a</div>"
         "<div id=b style='position:relative;font-size:20px;inset-inline:0.5em;top:1ex'>b</div>"
         "<table id=t style='font-size:20px;border-spacing:0.5em 2ch'><tr><td>x</td></tr></table></body>");
    TEST_ASSERT_EQUAL(20, el("a")->offset_x);
    TEST_ASSERT_EQUAL(20, el("a")->offset_y);
    TEST_ASSERT_EQUAL(10, el("b")->offset_x);
    TEST_ASSERT_EQUAL(6, el("b")->offset_y);
    TEST_ASSERT_EQUAL(10, el("t")->border_spacing);
    TEST_ASSERT_EQUAL(20, el("t")->border_spacing_y);
    TEST_ASSERT_EQUAL(0, diag_count("refused"));
}

/* the px bound of a unit length: +-2000 px (70em is 1120, 130em 2080) */
static void test_l117_bounds(void) {
    lays("<body style='margin:0'><div id=a style='margin-left:70em;margin-right:-130em'>a</div>"
         "<div id=b style='margin-left:125em;margin-right:-125.03em'>b</div></body>");
    TEST_ASSERT_EQUAL(1120, el("a")->margin[3]);
    TEST_ASSERT_EQUAL(-2000, el("a")->margin[1]);
    TEST_ASSERT_EQUAL(2000, el("b")->margin[3]);
    TEST_ASSERT_EQUAL(-2000, el("b")->margin[1]);
}

/* the refusals of the unit paths: negative or too large lengths, a
 * negative padding pair, flex-basis and border-spacing; a negative leaf
 * in math; object-position offsets; a 1200 % font of a 4 px parent; a
 * unit line height past 400 px */
static void test_l117_refusals(void) {
    lays("<body style='margin:0'><div id=a style='height:-1em'>a</div><div id=b style='height:1001em'>b</div>"
         "<div id=c style='height:1000em'>c</div></body>");
    TEST_ASSERT_EQUAL(10, el("a")->h);
    TEST_ASSERT_EQUAL(10, el("b")->h);
    TEST_ASSERT_EQUAL(16000, el("c")->h);
    TEST_ASSERT_EQUAL(2, diag_count("height refused"));
    lays("<body style='margin:0'><div id=a style='padding-block:-1em;flex-basis:-1em'>a</div>"
         "<table id=t style='border-spacing:-1em'><tr><td>x</td></tr></table></body>");
    TEST_ASSERT_EQUAL(0, el("a")->padding[0]);
    TEST_ASSERT_EQUAL(-1, el("a")->flex_basis);
    TEST_ASSERT_EQUAL(1, diag_count("logical spacing refused"));
    TEST_ASSERT_EQUAL(1, diag_count("flex-basis refused"));
    TEST_ASSERT_EQUAL(1, diag_count("border spacing refused"));
    lays("<body style='margin:0'><div id=a style='font-size:20px;width:calc(-1em + 50px)'>a</div>"
         "<img id=i src=x style='font-size:20px;width:40px;height:40px;object-position:1em 2em'>"
         "<div style='font-size:4px'><div id=f style='font-size:1200%'>f</div></div>"
         "<div id=l style='line-height:41lh'>l</div><div id=m style='line-height:40lh'>m</div></body>");
    TEST_ASSERT_EQUAL(30, el("a")->w);
    TEST_ASSERT_EQUAL(20, el("i")->object_dx);
    TEST_ASSERT_EQUAL(40, el("i")->object_dy);
    TEST_ASSERT_EQUAL(4800, el("f")->font100);
    TEST_ASSERT_EQUAL(0, el("l")->line_height);
    TEST_ASSERT_EQUAL(400, el("m")->line_height);
    TEST_ASSERT_EQUAL(1, diag_count("line height refused"));
}

/* without html the first top element gives rem; we_inspect prints font100
 * and lh100 */
static void test_l117_root_inspect(void) {
    lays("<div style='font-size:30px'>a</div><div id=y style='width:2rem'>y</div>");
    TEST_ASSERT_EQUAL(60, el("y")->w);
    lays("<body style='margin:0'><div id=a style='font-size:13.5px;line-height:1.5'>a</div></body>");
    g_out[0] = 0;
    we_inspect(&g_doc, we_find_id(&g_doc, "a"), out_cat, 0);
    TEST_ASSERT_NOT_NULL(strstr(g_out, " font100 1350 lh100 2025\n"));
}

/* line-clamp with a fractional number line height: the carried sum */
static void test_l117_clamp(void) {
    lays("<body style='margin:0'><div id=a style='font-size:13.5px;line-height:1.5;line-clamp:3'>" L4 "</div>"
         "<div id=n>after</div></body>");
    TEST_ASSERT_EQUAL(60, el("a")->h);               /* 3 x 20.25 = 60.75 */
    TEST_ASSERT_EQUAL(-1000, text_y("d"));
    TEST_ASSERT_EQUAL(61, text_y("after"));
    /* whole px line heights: unchanged */
    lays("<body style='margin:0'><div id=a style='line-height:20px;line-clamp:2'>" L4 "</div></body>");
    TEST_ASSERT_EQUAL(40, el("a")->h);
    /* one line of 20.6 px clamped: 20 (the next line starts at 20) */
    lays("<body style='margin:0'><div id=a style='line-height:20.6px;line-clamp:1'>a<br>b<br>c</div></body>");
    TEST_ASSERT_EQUAL(20, el("a")->h);
    /* the next line one px lower without a fraction is a gap: kept out */
    lays("<body style='margin:0'><div id=a style='line-height:20px;line-clamp:1'><div>a</div>"
         "<div style='margin-top:1px'>b</div></div></body>");
    TEST_ASSERT_EQUAL(20, el("a")->h);
    /* with a fractional line height a 2 px gap stays out too */
    lays("<body style='margin:0'><div id=a style='line-height:20.6px;line-clamp:1'><div>a</div>"
         "<div style='margin-top:2px'>b</div></div></body>");
    TEST_ASSERT_EQUAL(20, el("a")->h);
}

/* the public API: we_unit_len100 and we_font_size100 */
static void test_l117_api(void) {
    we_unit_ctx_t c = { 2000, 1600, 2400, 1000 };
    int h = 0;
    TEST_ASSERT_EQUAL(0, we_unit_len100("1.5em", 5, &c, &h)); TEST_ASSERT_EQUAL(3000, h);
    TEST_ASSERT_EQUAL(0, we_unit_len100("2rem", 4, &c, &h)); TEST_ASSERT_EQUAL(3200, h);
    TEST_ASSERT_EQUAL(0, we_unit_len100("1ex", 3, &c, &h)); TEST_ASSERT_EQUAL(600, h);
    TEST_ASSERT_EQUAL(0, we_unit_len100("1ch", 3, &c, &h)); TEST_ASSERT_EQUAL(1000, h);
    TEST_ASSERT_EQUAL(0, we_unit_len100("1cap", 4, &c, &h)); TEST_ASSERT_EQUAL(900, h);
    TEST_ASSERT_EQUAL(0, we_unit_len100("1ic", 3, &c, &h)); TEST_ASSERT_EQUAL(2000, h);
    TEST_ASSERT_EQUAL(0, we_unit_len100("0.5lh", 5, &c, &h)); TEST_ASSERT_EQUAL(1200, h);
    TEST_ASSERT_EQUAL(0, we_unit_len100("3rlh", 4, &c, &h)); TEST_ASSERT_EQUAL(3000, h);
    TEST_ASSERT_EQUAL(0, we_unit_len100("-.25EM", 6, &c, &h)); TEST_ASSERT_EQUAL(-500, h);
    TEST_ASSERT_EQUAL(0, we_unit_len100("+1.0009em", 9, &c, &h)); TEST_ASSERT_EQUAL(2000, h);   /* 3 decimals */
    TEST_ASSERT_EQUAL(0, we_unit_len100("0.0005em", 8, &c, &h)); TEST_ASSERT_EQUAL(0, h);      /* the 4th decimal is not read */
    TEST_ASSERT_EQUAL(0, we_unit_len100("3000em", 6, &c, &h)); TEST_ASSERT_EQUAL(4000000, h);  /* whole part 2000 */
    TEST_ASSERT_EQUAL(-1, we_unit_len100("1.5", 3, &c, &h));
    TEST_ASSERT_EQUAL(-1, we_unit_len100("em", 2, &c, &h));
    TEST_ASSERT_EQUAL(-1, we_unit_len100("1.5px", 5, &c, &h));
    TEST_ASSERT_EQUAL(-1, we_unit_len100("2emx", 4, &c, &h));
    TEST_ASSERT_EQUAL(-1, we_unit_len100("2e", 2, &c, &h));
    TEST_ASSERT_EQUAL(-1, we_unit_len100(".em", 3, &c, &h));
    TEST_ASSERT_EQUAL(-1, we_unit_len100(0, 0, &c, &h));
    TEST_ASSERT_EQUAL(-1, we_unit_len100("1em", 3, 0, &h));
    TEST_ASSERT_EQUAL(-1, we_unit_len100("1em", 3, &c, 0));
    TEST_ASSERT_EQUAL(-1, we_unit_len100("1em", -1, &c, &h));
    TEST_ASSERT_EQUAL(0, we_font_size100("1.5em", 5, &c, &h)); TEST_ASSERT_EQUAL(3000, h);
    TEST_ASSERT_EQUAL(0, we_font_size100(" larger ", 8, &c, &h)); TEST_ASSERT_EQUAL(2400, h);
    TEST_ASSERT_EQUAL(0, we_font_size100("smaller", 7, &c, &h)); TEST_ASSERT_EQUAL(1667, h);
    TEST_ASSERT_EQUAL(0, we_font_size100("x-large", 7, &c, &h)); TEST_ASSERT_EQUAL(2400, h);
    TEST_ASSERT_EQUAL(-1, we_font_size100("12", 2, &c, &h));
    TEST_ASSERT_EQUAL(-1, we_font_size100(0, 0, &c, &h));
    TEST_ASSERT_EQUAL(-1, we_font_size100("1em", 3, 0, &h));
    TEST_ASSERT_EQUAL(-1, we_font_size100("1em", 3, &c, 0));
    /* a context of zeros is the initial one */
    { we_unit_ctx_t z = { 0, 0, 0, 0 };
      TEST_ASSERT_EQUAL(0, we_unit_len100("1em", 3, &z, &h)); TEST_ASSERT_EQUAL(1600, h);
      TEST_ASSERT_EQUAL(0, we_unit_len100("1rem", 4, &z, &h)); TEST_ASSERT_EQUAL(1600, h);
      TEST_ASSERT_EQUAL(0, we_unit_len100("1lh", 3, &z, &h)); TEST_ASSERT_EQUAL(1000, h);
      TEST_ASSERT_EQUAL(0, we_unit_len100("1rlh", 4, &z, &h)); TEST_ASSERT_EQUAL(1000, h); }
    /* the API leaves the style context alone */
    lays("<body style='margin:0'><div style='font-size:30px'><div id=a style='margin-left:1em'>a</div></div></body>");
    TEST_ASSERT_EQUAL(0, we_unit_len100("1em", 3, &c, &h));
    TEST_ASSERT_EQUAL(30, el("a")->margin[3]);
}

/* math leaves: decimals only with a font-relative unit (Lot 118: px and
 * numbers take decimals too) */
static void test_l117_math_leaves(void) {
    lays("<body style='margin:0'><div style='width:600px;font-size:20px'><div id=a style='width:calc(2.5em + 10px)'>a</div>"
         "<div id=b style='width:50px;width:calc(1.5px + 2px)'>b</div><div id=c style='width:50px;width:calc(1.5 * 10px)'>c</div>"
         "<div id=d style='width:min(10em, 50%)'>d</div><div id=e style='width:calc(.5em + 1rem)'>e</div>"
         "<div id=f style='width:50px;width:calc(101em)'>f</div><div id=g style='width:calc(100em)'>g</div></div></body>");
    TEST_ASSERT_EQUAL(60, el("a")->w);
    TEST_ASSERT_EQUAL(3, el("b")->w);               /* Lot 118: 3.5px (was refused: 50) */
    TEST_ASSERT_EQUAL(15, el("c")->w);              /* Lot 118: 15px (was refused: 50) */
    TEST_ASSERT_EQUAL(200, el("d")->w);
    TEST_ASSERT_EQUAL(26, el("e")->w);
    TEST_ASSERT_EQUAL(50, el("f")->w);              /* 2020 px: past the 2000 literal */
    TEST_ASSERT_EQUAL(2000, el("g")->w);
    TEST_ASSERT_EQUAL(1, diag_count("math length refused"));    /* Lot 118: was 3 */
}

/* ::before boxes have the font pass too */
static void test_l117_pseudo(void) {
    lays("<html><head><style>#q::before { content: 'ab'; margin-left: 1em; font-size: 2em }</style></head>"
         "<body style='margin:0'><div id=q style='font-size:15px'>x</div></body></html>");
    {
        int i, found = 0;
        for (i = 0; i < (int)g_doc.nnodes; i++)
            if (g_doc.nodes[i].pseudo_kind) {
                TEST_ASSERT_EQUAL(3000, g_doc.nodes[i].font100);
                TEST_ASSERT_EQUAL(30, g_doc.nodes[i].margin[3]);
                found = 1;
            }
        TEST_ASSERT_TRUE(found);
    }
}

/* the reader passes a number line height for a font in hundredths */
static void test_l117_reader(void) {
    reduce("#b { font-size: 10.5px; line-height: 2 } #c { font-size: 13.25px; line-height: 1.5 }",
           "<p id=b>" L4 "</p><p id=c>" L4 "</p>");
    TEST_ASSERT_NOT_NULL(strstr(g_out, "font-size:10.5px;line-height:2"));
    TEST_ASSERT_EQUAL(1050, nth("p", 0)->font100);
    TEST_ASSERT_EQUAL(200, nth("p", 0)->line_height);
    TEST_ASSERT_EQUAL(1, nth("p", 0)->lh_unitless);
    TEST_ASSERT_EQUAL(84, nth("p", 0)->h);
    TEST_ASSERT_EQUAL(79, nth("p", 1)->h);          /* 4 x 19.875 -> 19.88 carried */
    /* outside the engine's 4..64 px font sizes the number goes as px */
    reduce("#d { font-size: 3.5px; line-height: 2 } #e { font-size: 70.5px; line-height: 2 }",
           "<p id=d>d</p><p id=e>e</p>");
    TEST_ASSERT_NOT_NULL(strstr(g_out, "font-size:3.5px;line-height:7px\""));
    TEST_ASSERT_NOT_NULL(strstr(g_out, "font-size:70.5px;line-height:141px\""));
}

int main(void) {
    unity_init();
    RUN_TEST(test_l117_em_own_font);
    RUN_TEST(test_l117_root_units);
    RUN_TEST(test_l117_lh_unit);
    RUN_TEST(test_l117_quiet_pass);
    RUN_TEST(test_l117_pass_cascade);
    RUN_TEST(test_l117_font_hundredths);
    RUN_TEST(test_l117_font_units);
    RUN_TEST(test_l117_lh_math);
    RUN_TEST(test_l117_parsers);
    RUN_TEST(test_l117_insets);
    RUN_TEST(test_l117_bounds);
    RUN_TEST(test_l117_refusals);
    RUN_TEST(test_l117_root_inspect);
    RUN_TEST(test_l117_clamp);
    RUN_TEST(test_l117_api);
    RUN_TEST(test_l117_math_leaves);
    RUN_TEST(test_l117_pseudo);
    RUN_TEST(test_l117_reader);
    unity_print_results();
    unity_cleanup();
    return unity_stats.tests_failed == 0 ? 0 : 1;
}
