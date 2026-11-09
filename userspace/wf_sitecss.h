/* wf_sitecss.h - Lot 86: a bounded stylesheet for the reader.
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

#define WF_SITE_NODES 40
#define WF_SITE_RULES 32
#define WF_SITE_PARTS 3
#define WF_SITE_CSS 3072
#define WF_SITE_CLS 2
#define WF_SITE_ATTR 2

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
    WF_SITE_PS_NTH = 5
};

typedef struct {
    char tag[12];
    char id[16];
    char cls[WF_SITE_CLS][12];
    int ncls;
    char aname[WF_SITE_ATTR][12];
    char aval[WF_SITE_ATTR][20];
    int nattr;
    char style[72];
    int parent, prev, next, first, last;
    int start;
    int child_i;     /* 1-based index among element siblings */
    int nsib;        /* element sibling count of the parent, copied onto each child */
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
    char cls[2][12];
    int ncls;
    char aname[12], aval[20];
    uint8_t attr_kind;            /* 0 none, 1 presence, 2 equal, 3 word */
    uint8_t pseudo;
    uint8_t nth_a, nth_b_neg;
    int nth_b;
    char not_tag[12];
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
    wf_site_node_t node[WF_SITE_NODES];
    int nnodes, dropped_nodes;
    wf_site_rule_t rule[WF_SITE_RULES];
    int nrules, dropped_rules, errors;
    char css[WF_SITE_CSS];
    int css_n;
    int ready;
    int vw;                       /* viewport width for @media, CSS px */
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
#endif
