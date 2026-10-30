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
 *          intrinsic text/box sizes; flex packing and alignment on both
 *          axes; bounded grid tracks/placement, table colspan and clipping.
 *          Lot 56: position absolute against the nearest positioned box,
 *          flex order, reverse and wrap-reverse, table rowspan and
 *          two-axis border-spacing, pre and pre-line whitespace, auto
 *          margins on the flex main axis, clear, line-height, text-indent,
 *          inline-block, table vertical-align and absolute z-index.
 *          No margin collapsing,
 *          bounded floats, tables, local images; no font loading or JavaScript.
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
#define WE_TEXT_SOURCE_MAX 512
#define WE_CSS_MAX 2048
#define WE_MAX_DIAG 8
#define WE_DIAG_LEN 48
#define WE_DEPTH_MAX 24
#define WE_CHAR_W 8
#define WE_LINE_H 10
#define WE_TEXT_BASELINE 9
#define WE_TRACKS_MAX 8
#define WE_GRID_ROWS_MAX WE_MAX_NODES
/* Static layout/measure/clip scratch; document storage is caller-owned. */
#define WE_LAYOUT_WORKSPACE_MAX (512 * 1024)
#define WE_MEASURE_BUDGET (WE_MAX_NODES * WE_MAX_NODES)
#define WE_TRACK_AUTO (-32768)
#define WE_GRID_AREAS_MAX 8
#define WE_GRID_NAME_MAX 16
#define WE_GRID_LINE_ALIASES 2
typedef struct { char name[WE_TRACKS_MAX + 1][WE_GRID_LINE_ALIASES][WE_GRID_NAME_MAX]; } we_grid_names_t;
/* Lot 61: an edge is auto, a line number, a name with an occurrence
 * (nth, negative counts from the end) or a span. A named span counts
 * lines carrying edge[] from the opposite resolved edge. */
typedef struct {
    char edge[2][WE_GRID_NAME_MAX];
    uint8_t span[2], active;
    int8_t nth[2];
    uint8_t named_span[2], bare[2];
    /* Plain numeric placement kept so subgrid clamping can be undone. */
    uint8_t plain_start, plain_span;
} we_grid_request_t;
#define WE_TRACK_LIMIT 2000
enum { WE_AUTO_REPEAT_NONE, WE_AUTO_FILL, WE_AUTO_FIT };
enum { WE_FIT_FILL, WE_FIT_CONTAIN, WE_FIT_COVER, WE_FIT_NONE, WE_FIT_SCALE_DOWN };
typedef struct {
    char name[WE_GRID_NAME_MAX];
    uint8_t row, col, rows, cols;
} we_grid_area_t;
#define WE_FLOATS_MAX 16
#define WE_ORDER_MAX 8
#define WE_SPACING_MAX 64
enum { WE_WS_NORMAL = 0, WE_WS_PRE = 1, WE_WS_PRE_WRAP = 2, WE_WS_PRE_LINE = 3 };
enum { WE_INSET_LEFT = 1, WE_INSET_TOP = 2, WE_INSET_RIGHT = 4, WE_INSET_BOTTOM = 8 };
enum { WE_AUTO_LEFT = 1, WE_AUTO_RIGHT = 2, WE_AUTO_TOP = 4, WE_AUTO_BOTTOM = 8 };
enum { WE_CLEAR_NONE = 0, WE_CLEAR_LEFT = 1, WE_CLEAR_RIGHT = 2, WE_CLEAR_BOTH = 3 };
enum { WE_VALIGN_TOP = 0, WE_VALIGN_MIDDLE = 1, WE_VALIGN_BOTTOM = 2, WE_VALIGN_BASELINE = 3 };
#define WE_Z_MAX 8
/* Lot 61 border styles; 0 = unset, painted and ranked as solid. */
#define WE_BS_NONE 1
#define WE_BS_HIDDEN 2
#define WE_BS_SOLID 3
#define WE_LINE_H_MAX 40
#define WE_INDENT_MAX 400
#define WE_LAYOUT_MAX 16000
/* One shared, non-reentrant image workspace in webeng.c. No heap.
 * Source and decoder scratch each occupy 64 KiB. Decoded dimensions and
 * output dimensions must fit 64x64. Pixel rectangles share the existing
 * 96-item display list; an image that cannot fit is refused atomically. */
#define WE_IMAGE_SOURCE_MAX 65536
#define WE_IMAGE_SCRATCH_MAX 65536
#define WE_IMAGE_MAX_W 64
#define WE_IMAGE_MAX_H 64
#define WE_IMAGE_WORKSPACE_MAX (WE_IMAGE_SOURCE_MAX + WE_IMAGE_SCRATCH_MAX + 8192)

enum { WE_FLOAT_NONE, WE_FLOAT_LEFT, WE_FLOAT_RIGHT };
enum { WE_LIST_NONE, WE_LIST_DISC, WE_LIST_DECIMAL, WE_LIST_SQUARE };

enum { WE_DISP_NONE = 0, WE_DISP_BLOCK = 1, WE_DISP_INLINE = 2, WE_DISP_FLEX = 3, WE_DISP_GRID = 4, WE_DISP_TABLE = 5, WE_DISP_ROW = 6, WE_DISP_CELL = 7, WE_DISP_INLINE_BLOCK = 8 };
enum { WE_ALIGN_LEFT = 0, WE_ALIGN_CENTER = 1, WE_ALIGN_RIGHT = 2 };
enum { WE_PACK_UNSET, WE_PACK_START, WE_PACK_END, WE_PACK_CENTER,
       WE_PACK_STRETCH, WE_PACK_BETWEEN, WE_PACK_AROUND, WE_PACK_EVENLY, WE_PACK_BASELINE,
       WE_PACK_LAST_BASELINE };
enum { WE_SIZE_AUTO, WE_SIZE_MIN_CONTENT, WE_SIZE_MAX_CONTENT, WE_SIZE_FIT_CONTENT, WE_SIZE_FIT_LIMIT };
enum { WE_WRAP_NORMAL, WE_WRAP_BREAK_WORD, WE_WRAP_ANYWHERE };

#define WE_MATH_NODES_MAX 24
#define WE_MATH_DEPTH_MAX 4
#define WE_MATH_LITERAL_MAX 2000
#define WE_MATH_VALUE_MAX 1000000
#define WE_DOCUMENT_MAX (1024 * 1024)
enum { WE_LEN_WIDTH, WE_LEN_HEIGHT, WE_LEN_MIN_W, WE_LEN_MAX_W, WE_LEN_MIN_H, WE_LEN_MAX_H,
       WE_LEN_BASIS, WE_LEN_LEFT, WE_LEN_TOP, WE_LEN_RIGHT, WE_LEN_BOTTOM, WE_LEN_ROW_GAP, WE_LEN_COL_GAP,
       WE_LENGTH_PROPS };
typedef struct {
    int16_t value;
    uint8_t op, a, b, c, dimension;
} we_math_node_t;
typedef struct {
    we_math_node_t node[WE_MATH_NODES_MAX];
    uint8_t count, percent;
} we_length_t;

/* Lot 62: paint-time transform functions. translate keeps px or percent
 * of the border box (pct bit 0 x, bit 1 y), scale keeps 1/1000 factors in
 * -8..8, rotate keeps quarter turns (0..3) in x. */
#define WE_TF_MAX 4
enum { WE_TF_TRANSLATE = 1, WE_TF_SCALE, WE_TF_ROTATE };
typedef struct { uint8_t kind, pct; int16_t x, y; } we_transform_t;

/* Lot 62: box-shadow entries (offsets and spread in px, blur 0..32). */
#define WE_SHADOWS_MAX 2
typedef struct { int8_t x, y, spread; uint8_t blur, inset, has_color; uint32_t color; } we_shadow_t;

typedef struct {
    uint8_t is_text, attr_flags, has_text;
    we_length_t length[WE_LENGTH_PROPS];
    int16_t aspect_num, aspect_den;
    uint8_t lead_space, trail_space, html_space, pending_html_space;
    char tag[12];
    char id[16];
    char cls[32];
    int16_t parent, first, last, next;
    uint16_t text_off, text_len;   /* text node content in pool */
    uint16_t style_off, style_len; /* inline style attribute in pool */
    uint16_t alt_off, alt_len;     /* img alt */
    uint16_t href_off, href_len;   /* a href (UX4: desktop browser links) */
    uint16_t src_off, src_len;
    uint8_t span_refused, colspan;
    /* computed style */
    uint8_t display, bold, align, has_bg;
    uint8_t flex_dir;          /* 0 row, 1 column (display:flex) */
    uint8_t grid_cols;         /* 1..8 when display:grid, else 0 */
    uint8_t flex_wrap, float_side, grid_rows, relative;
    uint8_t list_style;
    uint8_t justify_content, align_items, align_self, align_content;
    uint8_t justify_items, justify_self;
    uint8_t width_kind, basis_content, border_box;
    uint8_t min_width_kind, max_width_kind, basis_kind, break_all;
    uint8_t nowrap, break_word, visibility, overflow_hidden;
    uint8_t grid_col_start, grid_row_start, grid_col_span, grid_row_span;
    we_grid_names_t line_names[2];
    /* Lot 61: one repeat(auto-fill|auto-fit, track) per axis. The template
     * keeps one repetition; layout expands it to 1..8 tracks in place. */
    uint8_t auto_mode[2], auto_at[2], auto_count[2], tpl_count[2];
    int16_t tpl_size[2][WE_TRACKS_MAX], tpl_floor[2][WE_TRACKS_MAX], tpl_cap[2][WE_TRACKS_MAX];
    /* Lot 61: grid-template-*: subgrid per axis. Layout copies the parent
     * tracks, gap (sub_gap - 1, 0 when none) and line names in place. */
    uint8_t subgrid[2];
    int16_t sub_gap[2];
    uint8_t gap_set;           /* lot 61: bit 0 column, bit 1 row gap declared */
    we_grid_request_t grid_request[2];
    int16_t col_size[WE_TRACKS_MAX];
    int16_t col_floor[WE_TRACKS_MAX], col_cap[WE_TRACKS_MAX];
    int16_t row_floor[WE_TRACKS_MAX], row_cap[WE_TRACKS_MAX];
    int16_t auto_col, auto_row, auto_col_floor, auto_col_cap, auto_row_floor, auto_row_cap;
    uint8_t grid_dense, grid_column_flow, area_count, area_rows, area_cols;
    we_grid_area_t areas[WE_GRID_AREAS_MAX];
    char grid_area[WE_GRID_NAME_MAX];
    uint8_t object_fit, object_fit_set;
    int16_t object_x, object_y;
    int16_t row_gap, col_gap;
    int16_t height_pct, min_height_pct, max_height_pct;
    uint8_t height_percent, min_height_percent, max_height_percent;
    int16_t min_content, max_content;
    int16_t flex_basis, basis_pct, flex_shrink;
    uint8_t basis_set;
    int16_t flex_grow;         /* -1: legacy equal flex slots, 0..1000 explicit */
    int16_t row_size[WE_TRACKS_MAX]; /* -1: 1fr, otherwise pixels */
    int16_t min_width, max_width;   /* -1: unset, content-box px */
    int16_t min_height, max_height; /* -1: unset; percent values use flags */
    int16_t min_pct, max_pct;
    int16_t offset_x, offset_y;
    int16_t offset_r, offset_b; /* right / bottom, valid when inset_flags says so */
    int8_t item_order;         /* -8..8, source order when equal */
    uint8_t flex_reverse, absolute, rowspan, ws_mode, inset_flags, margin_auto, border_collapse;
    int16_t border_spacing;    /* horizontal; separate borders; collapse forces 0 */
    int16_t border_spacing_y;  /* vertical; one value sets both */
    int16_t line_height;       /* 0 uses WE_LINE_H; glyphs stay at the top */
    int16_t text_indent;       /* first line only, after float shortening */
    int8_t z_index;            /* absolute paint order, -8..8 */
    uint8_t z_set, clear_side, valign;
    uint8_t fixed;             /* lot 61: position:fixed (absolute + viewport) */
    uint8_t collapse_w;        /* lot 61: shared cell border width, layout time */
    /* Lot 61 border edges, top right bottom left. edge_w -1 and style 0
     * mean "use border / border_style". Only collapsed tables read them. */
    int8_t edge_w[4];
    uint8_t edge_style[4], border_style, edge_color_set, collapse_cell;
    uint8_t transparency;       /* lot 61: 100 - opacity%, non-zero forms a stacking context */
    uint8_t cin[4];             /* lot 61: collapsed cell content insets, layout time */
    /* Lot 62: transform list and transform-origin (px, or percent when the
     * tf_origin_pct bit of the axis is set; default 50% 50%). */
    we_transform_t tf[WE_TF_MAX];
    uint8_t tf_count, tf_origin_pct;
    int16_t tf_origin[2];
    /* Lot 62: outline (style 0 none, width 0..8, offset -32..32, colour
     * currentColor unless set) and up to WE_SHADOWS_MAX box shadows. */
    uint8_t outline_style, outline_w, outline_color_set;
    int8_t outline_offset;
    uint32_t outline_color;
    we_shadow_t shadow[WE_SHADOWS_MAX];
    uint8_t shadow_count;
    uint32_t edge_color[4];
    int16_t gap;               /* px between flex/grid items */
    uint32_t color, bg, border_color;
    int16_t margin[4], padding[4]; /* top right bottom left */
    int16_t border;
    int16_t width, height;         /* -1 auto */
    int16_t width_pct;             /* >0: percent of the containing block */
    /* layout box (border box, page pixels) */
    int16_t x, y, w, h;
} we_node_t;

#define WE_SELECTOR_PARTS_MAX 4
#define WE_SELECTOR_CLASSES_MAX 3
#define WE_SELECTOR_ATTRS_MAX 2
#define WE_SELECTOR_WORK_MAX 4096
enum { WE_SEL_DESC, WE_SEL_CHILD, WE_SEL_ADJACENT, WE_SEL_SIBLING };
enum { WE_PSEUDO_FIRST = 1, WE_PSEUDO_LAST, WE_PSEUDO_ONLY, WE_PSEUDO_EMPTY,
       WE_PSEUDO_ROOT, WE_PSEUDO_NTH, WE_PSEUDO_NTH_LAST, WE_PSEUDO_TYPE, WE_PSEUDO_TYPE_LAST };
enum { WE_ATTR_PRESENT, WE_ATTR_EQUAL, WE_ATTR_TOKEN, WE_ATTR_PREFIX, WE_ATTR_SUFFIX, WE_ATTR_CONTAINS };
typedef struct {
    char name[8], value[32];
    uint8_t op, insensitive;
} we_selector_attr_t;
typedef struct {
    char tag[32], id[32], cls[WE_SELECTOR_CLASSES_MAX][32];
    we_selector_attr_t attr[WE_SELECTOR_ATTRS_MAX];
    uint8_t classes, attrs, pseudo, combinator;
    int16_t nth_a, nth_b;
} we_selector_part_t;
typedef struct {
    we_selector_part_t part[WE_SELECTOR_PARTS_MAX];
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
    uint32_t flex_lines, grid_lines, floats, table_rows;
    uint32_t images, image_refused;
} we_perf_t;

typedef struct {
    we_node_t nodes[WE_MAX_NODES];
    uint32_t nnodes;
    char pool[WE_POOL];
    uint32_t pool_used;
    char css[WE_CSS_MAX];
    uint32_t css_len;
    uint8_t css_parsed;
    we_rule_t rules[WE_MAX_RULES];
    uint32_t nrules;
    os_fb_web_item_t items[OS_FB_WEB_ITEMS];
    int16_t item_owner[OS_FB_WEB_ITEMS];
    uint32_t nitems;
    char title[48];
    char diag[WE_MAX_DIAG][WE_DIAG_LEN];
    uint32_t ndiag, diag_dropped;
    we_perf_t perf;
    int16_t root;
    /* Lot 61: explicit viewport and the scroll offset fixed boxes follow. */
    int16_t viewport_w, viewport_h, scroll_y;
} we_doc_t;
typedef char we_document_is_bounded[sizeof(we_doc_t) <= WE_DOCUMENT_MAX ? 1 : -1];

/* The reader returns the whole file size, or <0. Returning cap + 1
 * reports a larger/truncated file. Only absolute local paths are passed;
 * URLs and parent traversal are refused by the engine before the reader.
 * The callback and its context outlive a render. Passing 0 disables it.
 * Rendering is synchronous and non-reentrant, like the shared workspace. */
typedef int (*we_image_read_fn)(void *ctx, const char *path, unsigned char *dst, int cap);
void we_set_image_reader(we_image_read_fn read, void *ctx);

/* Each phase returns 0 or a negative error. we_render runs all three. */
int we_parse(we_doc_t* d, const char* html, int len);
int we_style(we_doc_t* d);
int we_layout(we_doc_t* d, int viewport_w);
/* Lot 61: layout against a viewport of viewport_w x viewport_h (fixed boxes,
 * scroll bound). we_layout uses WE_VIEWPORT_H_DEFAULT. */
#define WE_VIEWPORT_H_DEFAULT 480
int we_layout_viewport(we_doc_t* d, int viewport_w, int viewport_h);
/* Clamp y to [0, page_h - viewport_h], move fixed boxes, return the offset. */
int we_scroll(we_doc_t* d, int y);
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
