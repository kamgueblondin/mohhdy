/* userspace/webeng.h - Lot 6 (US-031, US-043, US-045): minimal guest web
 * engine. Pure C, no libc, no JavaScript, no Chromium/WebKit.
 *   parse: HTML tokenizer and DOM tree (nesting, void elements, implied
 *          </p> and </li>, comments, entities, <style> collected, <script>
 *          skipped and counted).
 *   style: CSS rules from <style> and style="" (type, .class, #id,
 *          compounds, one descendant combinator, comma groups), cascade by
 *          specificity then source order, inherited color/bold/align, a
 *          user-agent sheet.
 *   layout: block boxes (margin, border, padding, width/height px or %)
 *          stacked vertically, inline text flowed in line boxes of the
 *          8x8 font (8 px per char, 10 px per line), text-align, <br>.
 *          No margin collapsing, no floats, no tables, no fonts.
 *   paint: display list (os_fb_web_item_t) drawn by the OS-UI web pane.
 * US-043: phase counters (we_perf_t). US-045: DOM dump, box inspector,
 * diagnostics console. */
#ifndef MOHHDY_WEBENG_H
#define MOHHDY_WEBENG_H
#include <stdint.h>
#include "os_syscalls.h"

#define WE_MAX_NODES 160
#define WE_MAX_RULES 48
#define WE_POOL 6144
#define WE_CSS_MAX 2048
#define WE_MAX_DIAG 8
#define WE_DIAG_LEN 48
#define WE_DEPTH_MAX 24
#define WE_CHAR_W 8
#define WE_LINE_H 10

enum { WE_DISP_NONE = 0, WE_DISP_BLOCK = 1, WE_DISP_INLINE = 2 };
enum { WE_ALIGN_LEFT = 0, WE_ALIGN_CENTER = 1, WE_ALIGN_RIGHT = 2 };

typedef struct {
    uint8_t is_text;
    uint8_t lead_space, trail_space;
    char tag[12];
    char id[16];
    char cls[32];
    int16_t parent, first, last, next;
    uint16_t text_off, text_len;   /* text node content in pool */
    uint16_t style_off, style_len; /* inline style attribute in pool */
    uint16_t alt_off, alt_len;     /* img alt */
    uint16_t href_off, href_len;   /* a href (UX4: desktop browser links) */
    /* computed style */
    uint8_t display, bold, align, has_bg;
    uint32_t color, bg, border_color;
    int16_t margin[4], padding[4]; /* top right bottom left */
    int16_t border;
    int16_t width, height;         /* -1 auto */
    int16_t width_pct;             /* >0: percent of the containing block */
    /* layout box (border box, page pixels) */
    int16_t x, y, w, h;
} we_node_t;

typedef struct {
    char comp[2][3][32];           /* [compound][tag, class, id] */
    uint8_t ncomp;
    uint16_t spec;
    uint16_t order;
    uint16_t decl_off, decl_len;
} we_rule_t;

typedef struct {
    uint32_t nodes, elements, text_nodes, scripts_skipped;
    uint32_t rules, decls_applied, unknown_props;
    uint32_t boxes, lines, items, items_dropped;
    uint32_t page_w, page_h;
} we_perf_t;

typedef struct {
    we_node_t nodes[WE_MAX_NODES];
    uint32_t nnodes;
    char pool[WE_POOL];
    uint32_t pool_used;
    char css[WE_CSS_MAX];
    uint32_t css_len;
    we_rule_t rules[WE_MAX_RULES];
    uint32_t nrules;
    os_fb_web_item_t items[OS_FB_WEB_ITEMS];
    uint32_t nitems;
    char title[48];
    char diag[WE_MAX_DIAG][WE_DIAG_LEN];
    uint32_t ndiag, diag_dropped;
    we_perf_t perf;
    int16_t root;
} we_doc_t;

/* Each phase returns 0 or a negative error. we_render runs all three. */
int we_parse(we_doc_t* d, const char* html, int len);
int we_style(we_doc_t* d);
int we_layout(we_doc_t* d, int viewport_w);
int we_render(we_doc_t* d, const char* html, int len, int viewport_w);
/* US-045 */
int we_find_id(const we_doc_t* d, const char* id);
typedef void (*we_out_fn)(void* ctx, const char* s);
void we_dump_dom(const we_doc_t* d, we_out_fn out, void* ctx);
void we_inspect(const we_doc_t* d, int node, we_out_fn out, void* ctx);
/* UX4: link under page point (x, y): copies the href of the nearest <a>
 * ancestor of the deepest laid-out node there; returns its length, 0 when
 * no link is there. */
int we_link_at(const we_doc_t* d, int x, int y, char* href, int max);
#endif
