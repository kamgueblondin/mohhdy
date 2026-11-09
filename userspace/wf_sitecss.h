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
 * Cascade: user-agent, then author, then the style attribute. !important
 * beats normal. Specificity then source order. The reader uses display:none
 * to skip a subtree and copies color, font-weight and text-align onto the
 * tag it keeps. No @media, no external <link>, no JavaScript. */
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
#define WF_SITE_ALT 2
#define WF_SITE_VARS 16
#define WF_SITE_LINKS 6
#define WF_SITE_HREF 160

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
    WF_SITE_PS_WHERE = 16
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
    WF_SITE_K_VIS = 9         /* visibility: 1 visible, 2 hidden */
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
} wf_site_rule_t;

typedef struct {
    char name[24];
    char value[32];
} wf_site_var_t;

typedef struct {
    char href[WF_SITE_HREF];
    char media[32];
    uint8_t from_import;
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
/* Custom property value after the cascade of :root/html/body, or 0. */
const char *wf_site_var(const wf_site_t *d, const char *name);
#endif
