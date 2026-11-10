/* wf_sitecss.h - Lot 86: a bounded stylesheet for the reader.
 * Lot 87: 192 nodes, 128 rules, 16 KiB of CSS. Attribute operators ^= $=
 * *= |= and the i flag. :first-of-type, :last-of-type, :only-child,
 * :only-of-type, :nth-last-child, :nth-of-type, :nth-last-of-type,
 * :empty, :root, :not(.c / #id / [a] / tag), :is() and :where() with up
 * to two simple alternatives. font-style, text-decoration, visibility.
 * Custom properties (--x) declared on :root, html or body, read by var()
 * with a fallback. Every declaration of a style attribute. @media lists
 * with and, width, height, orientation, prefers-color-scheme, print;
 * @supports; @import and <link rel=stylesheet> are listed for the caller,
 * which may fetch them and pass the text to wf_site_prepare_ext.
 * Selectors: type, .class, #id, [attr], [attr=value], [attr~=word],
 * descendant, child, adjacent and general sibling, :first-child,
 * :last-child, :link, :not(type), :nth-child(an+b) for a small an+b.
 * Lot 87: revert-layer and revert on any of these properties (the value
 * of the previous layer, or of the previous origin); initial, inherit and
 * unset (color, font-weight, text-align, font-style and visibility
 * inherit; display, margin, padding and text-decoration do not).
 * Cascade: user-agent, then author, then the style attribute. !important
 * beats normal. Specificity then source order. The reader uses display:none
 * to skip a subtree and copies color, font-weight and text-align onto the
 * tag it keeps. No @media, no external <link>, no JavaScript.
 * Lot 88: @scope (start) to (end) { ... } and @scope { ... } (the parent of
 * the <style> element is the root), nested up to four levels, with :scope
 * and &, relative rules (":scope p" implied, or a leading > + ~),
 * declarations right in the block (on the root, specificity 0) and scope
 * proximity in the cascade (between specificity and order). The all
 * shorthand with initial, inherit, unset, revert and revert-layer.
 * text-transform and white-space, which inherit, with every keyword.
 * Lot 88 (chunk 4): background-color (and a background that is one
 * colour), list-style-type (and the keyword of list-style), font-size,
 * text-indent and letter-spacing, with every keyword; the last four
 * inherit. Lengths are kept as a short checked token (12px, 1.5em, 120%)
 * and passed to the engine, which resolves em and % against the parent. */
#ifndef WF_SITECSS_H
#define WF_SITECSS_H
#include <stdint.h>

#define WF_SITE_NODES 192
#define WF_SITE_RULES 128
#define WF_SITE_PARTS 4
#define WF_SITE_CSS 16384
#define WF_SITE_CLS 4
#define WF_SITE_ATTR 4
#define WF_SITE_PCLS 3
#define WF_SITE_TOK 12            /* Lot 88: a checked value token (font-size: 1.25em) */
#define WF_SITE_TOKS 4            /* list, font-size, text-indent, letter-spacing */
#define WF_SITE_ALT 2
#define WF_SITE_VARS 16
#define WF_SITE_LINKS 6
#define WF_SITE_HREF 160
/* Lot 87: cascade layers (@layer). Names in order of first appearance;
 * a nested layer is "outer.inner"; an anonymous block gets its own. */
#define WF_SITE_LAYERS 12
#define WF_SITE_LAYER_NAME 32
/* Lot 88: @scope rules and the <style> blocks that own a part of css[] */
#define WF_SITE_SCOPES 8
#define WF_SITE_STYLES 8
#define WF_SITE_SCOPE_DEPTH 4
#define WF_SITE_SCOPE_ALTS 3        /* Lot 89: selectors in a start or end list */

enum {
    WF_SITE_DISP_NONE = 3,
    WF_SITE_DISP_BLOCK = 1,
    WF_SITE_DISP_INLINE = 2
};
enum {
    WF_SITE_REL_DESC = 0,
    WF_SITE_REL_CHILD = 1,
    WF_SITE_REL_ADJ = 2,
    WF_SITE_REL_SIB = 3
};
enum {
    WF_SITE_PS_NONE = 0,
    WF_SITE_PS_FIRST = 1,
    WF_SITE_PS_LINK = 2,
    WF_SITE_PS_NOT = 3,
    WF_SITE_PS_LAST = 4,
    WF_SITE_PS_NTH = 5,
    WF_SITE_PS_FIRST_TYPE = 6,
    WF_SITE_PS_LAST_TYPE = 7,
    WF_SITE_PS_ONLY = 8,
    WF_SITE_PS_ONLY_TYPE = 9,
    WF_SITE_PS_NTH_LAST = 10,
    WF_SITE_PS_NTH_TYPE = 11,
    WF_SITE_PS_NTH_LAST_TYPE = 12,
    WF_SITE_PS_EMPTY = 13,
    WF_SITE_PS_ROOT = 14,
    WF_SITE_PS_IS = 15,
    WF_SITE_PS_WHERE = 16,
    WF_SITE_PS_SCOPE = 17,    /* Lot 88: :scope, specificity of a class */
    WF_SITE_PS_AMP = 18       /* Lot 88: & in @scope, :where(:scope) (Lot 89: no longer set, see scope_ref) */
};
enum {
    WF_SITE_ATTR_NONE = 0,
    WF_SITE_ATTR_HAS = 1,
    WF_SITE_ATTR_EQ = 2,
    WF_SITE_ATTR_WORD = 3,
    WF_SITE_ATTR_PREFIX = 4,
    WF_SITE_ATTR_SUFFIX = 5,
    WF_SITE_ATTR_SUBSTR = 6,
    WF_SITE_ATTR_DASH = 7
};
/* rule kinds */
enum {
    WF_SITE_K_DISPLAY = 1,
    WF_SITE_K_WEIGHT = 2,
    WF_SITE_K_ALIGN = 3,
    WF_SITE_K_COLOR = 4,
    WF_SITE_K_MARGIN = 5,
    WF_SITE_K_PADDING = 6,
    WF_SITE_K_STYLE = 7,      /* font-style: 1 normal, 2 italic */
    WF_SITE_K_DECO = 8,       /* text-decoration: 1 none, 2 underline, 3 line-through */
    WF_SITE_K_VIS = 9,        /* visibility: 1 visible, 2 hidden */
    /* Lot 88 */
    WF_SITE_K_TRANSFORM = 10, /* text-transform: 1 none, 2 uppercase, 3 lowercase, 4 capitalize */
    WF_SITE_K_WS = 11,        /* white-space: 1 normal, 2 pre, 3 nowrap, 4 pre-wrap, 5 pre-line */
    WF_SITE_K_BG = 12,        /* background-color: 1 transparent, 2 colour */
    WF_SITE_K_LIST = 13,      /* list-style-type: token, inherits */
    WF_SITE_K_FSIZE = 14,     /* font-size: token, inherits */
    WF_SITE_K_INDENT = 15,    /* text-indent: token, inherits */
    WF_SITE_K_LSPACE = 16,    /* letter-spacing: token, inherits */
    WF_SITE_K_MAX = 16,
    WF_SITE_K_ALL = 17        /* the all shorthand: a CSS-wide keyword on every kind */
};
/* Lot 89: a compound that names the scoping root */
enum {
    WF_SITE_SREF_NONE = 0,
    WF_SITE_SREF_SCOPE = 1,   /* :scope, specificity of a class */
    WF_SITE_SREF_AMP = 2      /* &, :where(:scope), no specificity */
};
/* Lot 88: how a rule inside @scope relates to its scoping root */
enum {
    WF_SITE_SR_NONE = 0,      /* not relative: the selector has :scope or & */
    WF_SITE_SR_DESC = 1,      /* "p": ":scope p" */
    WF_SITE_SR_CHILD = 2,     /* "> p" */
    WF_SITE_SR_ADJ = 3,       /* "+ p" */
    WF_SITE_SR_SIB = 4        /* "~ p" */
};
/* Lot 87: CSS-wide keywords a declaration may carry instead of a value */
enum {
    WF_SITE_RV_NONE = 0,
    WF_SITE_RV_LAYER = 1,     /* revert-layer: as if this layer had no rule for it */
    WF_SITE_RV_ORIGIN = 2,    /* revert: as if this origin had no rule for it */
    WF_SITE_RV_INHERIT = 3    /* inherit (and unset on an inherited property):
                               * the parent's value after the cascade */
};

typedef struct {
    char tag[12];
    char id[16];
    char cls[WF_SITE_CLS][12];
    int ncls;
    char aname[WF_SITE_ATTR][12];
    char aval[WF_SITE_ATTR][64];
    int nattr;
    char style[128];
    int parent, prev, next, first, last;
    int start;
    int child_i;     /* 1-based index among element siblings */
    int nsib;        /* element sibling count of the parent, copied onto each child */
    int type_i;      /* 1-based index among siblings with the same tag */
    int ntype;       /* siblings with the same tag, this one included */
    int nkids;       /* element children */
    uint8_t has_text;   /* a non-space character directly inside */
    uint8_t italic, deco, vis;
    int italic_rank, deco_rank, vis_rank;
    uint8_t disp, disp_imp;
    uint8_t weight, weight_imp;   /* 1 normal, 2 bold */
    uint8_t align, align_imp;     /* 1 left, 2 center, 3 right */
    uint8_t color_set, color_imp;
    uint8_t hidden_attr;
    int disp_rank, weight_rank, align_rank, color_rank;
    int margin_rank, pad_rank;
    uint32_t color;               /* 0xRRGGBB */
    uint8_t margin[4], pad[4];    /* top right bottom left, px */
    uint8_t margin_set, pad_set;  /* bit i set when that side was assigned */
    /* Lot 87: the winning declaration of kind k was revert-layer (1) or
     * revert (2); wf_site_apply rolls it back before it returns */
    uint8_t rv[WF_SITE_K_MAX + 1];
    uint8_t transform, ws;        /* Lot 88: WF_SITE_K_TRANSFORM / WF_SITE_K_WS values */
    int transform_rank, ws_rank;
    /* Lot 88 (chunk 4): background-color, and the token kinds
     * (WF_SITE_K_LIST + i); tok_val[i] 1 = from initial (not passed on),
     * 2 = a value the page gave */
    uint32_t bg;
    uint8_t bg_set;
    int bg_rank;
    char tok[WF_SITE_TOKS][WF_SITE_TOK];
    uint8_t tok_val[WF_SITE_TOKS];
    int tok_rank[WF_SITE_TOKS];
} wf_site_node_t;

typedef struct {
    char tag[12];
    char id[16];
    char cls[WF_SITE_PCLS][12];
    int ncls;
    char aname[12], aval[40];
    uint8_t attr_kind;            /* WF_SITE_ATTR_* */
    uint8_t attr_ci;              /* [a=v i]: ASCII case-insensitive value */
    uint8_t pseudo;
    uint8_t nth_a, nth_b_neg;
    int nth_b;
    char not_tag[12];
    uint8_t not_kind;             /* 0 tag, 1 class, 2 id, 3 attribute present */
    /* :is() / :where(): up to WF_SITE_ALT alternatives (tag and/or class) */
    char alt_tag[WF_SITE_ALT][12];
    char alt_cls[WF_SITE_ALT][12];
    uint8_t nalt;
    uint8_t rel;                  /* combinator that leads to this compound */
    /* Lot 89: the compound names the scoping root, WF_SITE_SREF_*; kept
     * apart from pseudo so "&:first-child" and ":scope.wide:last-child"
     * keep both conditions */
    uint8_t scope_ref;
} wf_site_part_t;

typedef struct {
    wf_site_part_t part[WF_SITE_PARTS];
    int nparts;
    uint8_t imp, origin;          /* origin 1 user-agent, 2 author, 3 inline */
    uint8_t kind;                 /* 1 display 2 weight 3 align 4 color */
    uint8_t val;
    uint8_t box[4];               /* margin or padding sides */
    uint8_t box_n;
    uint32_t color;
    int spec, order;
    uint8_t layer;                /* Lot 87: 0 unlayered, else layer index + 1 */
    uint8_t revert;               /* Lot 87: WF_SITE_RV_* (kind set, no value) */
    uint8_t scope;                /* Lot 88: 0 unscoped, else scope index + 1 */
    uint8_t scope_rel;            /* Lot 88: WF_SITE_SR_* */
    char tok[WF_SITE_TOK];        /* Lot 88: the value of a token kind */
} wf_site_rule_t;

/* Lot 88: one @scope rule. A root is an element that matches start (or,
 * without start, the parent element of the <style> block, owner -1 for
 * the whole document); the scope is the root and its descendants, minus
 * every descendant that matches end and what is under it. */
typedef struct {
    /* Lot 89: start and end are selector lists of up to
     * WF_SITE_SCOPE_ALTS selectors; nstart / nend count the selectors,
     * start_len / end_len the compounds of each */
    wf_site_part_t start[WF_SITE_SCOPE_ALTS][WF_SITE_PARTS];
    uint8_t start_len[WF_SITE_SCOPE_ALTS];
    int nstart;
    wf_site_part_t end[WF_SITE_SCOPE_ALTS][WF_SITE_PARTS];
    uint8_t end_len[WF_SITE_SCOPE_ALTS];
    int nend;
    int owner;                    /* implicit root (no start): node index or -1 */
    int parent;                   /* enclosing scope index, or -1 */
} wf_site_scope_t;

typedef struct {
    char name[24];
    char value[32];
} wf_site_var_t;

typedef struct {
    char href[WF_SITE_HREF];
    char media[32];
    uint8_t from_import;
    uint8_t layered;              /* Lot 87: @import ... layer or layer(name) */
    char layer[WF_SITE_LAYER_NAME];
} wf_site_link_t;

typedef struct {
    wf_site_node_t node[WF_SITE_NODES];
    int nnodes, dropped_nodes;
    wf_site_rule_t rule[WF_SITE_RULES];
    int nrules, dropped_rules, errors;
    char css[WF_SITE_CSS];
    int css_n;
    int ready;
    int vw;                       /* viewport width for @media, CSS px */
    int vh;                       /* viewport height for @media, CSS px */
    wf_site_var_t var[WF_SITE_VARS];
    int nvars;
    wf_site_link_t link[WF_SITE_LINKS];
    int nlinks, dropped_links;
    int ext_n;                    /* bytes of css[] that came from outside */
    char layer[WF_SITE_LAYERS][WF_SITE_LAYER_NAME];   /* Lot 87 */
    int nlayers, dropped_layers, anon_layers;
    uint8_t layer_rank[WF_SITE_LAYERS];   /* cascade position, 1 = weakest */
    int reverts;                  /* Lot 87: revert-layer / revert rolled back */
    int inherits;                 /* Lot 87: inherit / unset taken from the parent */
    wf_site_scope_t scope[WF_SITE_SCOPES];   /* Lot 88 */
    int nscopes, dropped_scopes;
    int style_from[WF_SITE_STYLES], style_to[WF_SITE_STYLES], style_owner[WF_SITE_STYLES];
    int nstyles;
    int scoped_hits;              /* Lot 88: scoped rule matches painted */
} wf_site_t;

/* Build the tree and apply the <style> blocks plus the user-agent sheet. */
int wf_site_prepare(wf_site_t *d, const char *html, int n);
/* Same, with an explicit viewport. @media (min-width) and (max-width)
 * use it. Other media features are skipped. */
int wf_site_prepare_vw(wf_site_t *d, const char *html, int n, int vw);
/* 1 when this start-tag offset, or an ancestor, is display:none. */
int wf_site_hidden_at(const wf_site_t *d, int offset);
/* Writes ` style="..."` when the element has a computed color, weight or
 * align. Returns the length, or 0. */
int wf_site_style_attr(const wf_site_t *d, int offset, char *out, int cap);
/* One line per node: "node TAG id CLASS display D weight W align A color C". */
int wf_site_report(const wf_site_t *d, char *out, int cap);
const wf_site_node_t *wf_site_at(const wf_site_t *d, int offset);
/* Lot 87: ext (external sheets the caller fetched, already wrapped in
 * @media when the link had a media list) is parsed first, as author CSS,
 * then the <style> blocks. vh 0 means 480. */
int wf_site_prepare_ext(wf_site_t *d, const char *html, int n, int vw, int vh, const char *ext, int ext_n);
/* <link rel=stylesheet> and @import found while preparing. */
int wf_site_nlinks(const wf_site_t *d);
const wf_site_link_t *wf_site_link_at(const wf_site_t *d, int i);
/* 1 when this media query list applies at vw x vh (exposed for tests and
 * for the caller's <link media>). */
int wf_site_media_ok(const char *q, int n, int vw, int vh);
/* 1 when an @supports condition holds for the reader and the engine. */
int wf_site_supports(const char *cond, int n);
/* Lot 87: the conditions after the URL of an @import: "layer",
 * "layer(name)" and "supports(cond)", in that order, then the media list.
 * *media_at receives the offset of the media list in s. layer_out gets
 * the layer name ("" for an anonymous layer) and *layered 1 when there was
 * a layer. Returns 1 when supports() holds or is absent, 0 when not. */
int wf_site_import_conds(const char *s, int n, int *media_at, char *layer_out, int cap, int *layered);
/* Lot 87: the layer order of the last prepare, "a,b.c,#anon1"; length. */
int wf_site_layers_text(const wf_site_t *d, char *out, int cap);
/* Lot 88: "scopes N rules R hits H", the @scope summary of the last
 * prepare; length. */
int wf_site_scope_text(const wf_site_t *d, char *out, int cap);
/* Lot 88: the root a scoped rule would use for node idx (nearest first),
 * -1 when it is not in any scope of that rule, -2 for an unscoped rule. */
int wf_site_scope_root(const wf_site_t *d, int rule, int idx);
/* Custom property value after the cascade of :root/html/body, or 0. */
const char *wf_site_var(const wf_site_t *d, const char *name);
#endif
