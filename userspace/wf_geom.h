/* wf_geom.h - Lot 104: the layout geometry the engine (webeng.c,
 * webeng_geom.inc) hands back to the reader style engine (wf_sitecss.c)
 * so view progress timelines can be computed. A plain record, no code:
 * the engine fills it from its layout boxes (read only, the layout is
 * not changed), the reader reads it on its next pass.
 *
 * One box per reader element index (the engine finds the element by
 * the id "wg<index>" the reader view writes on it, wf_html_set_geommarks),
 * in CSS px of the page: the border box along the block and inline axes
 * of its nearest scroller's writing mode (vertical-rl block offsets from
 * the right edge, the block start), from that scroller's content start, with that scroller's scrollport size, scroll offset and
 * scroll range (max scroll offset) along both axes.
 *
 * Lot 105: which box scroller that is (sm: its reader index + 1 when it
 * carries a mark, 0 for the root or an unmarked box), and, when the
 * element is itself a box scroller (sv: overflow auto or scroll along y),
 * its own scrollport height, scroll offset and scroll range along y (a
 * box scroller scrolls along y only); the root scroll offset (sc_y, the
 * GUI scroll position the layout had). */
#ifndef WF_GEOM_H
#define WF_GEOM_H

#define WF_GEOM_MAX 192          /* reader elements (WF_SITE_NODES) */

typedef struct {
    unsigned char ok;            /* 1: the element has a box */
    unsigned char root;          /* 1: its nearest scroller is the root (viewport) */
    unsigned char bvert;         /* 1: the block axis is vertical (a horizontal-tb scroller) */
    int b_off, b_size;           /* border box along the block axis */
    int i_off, i_size;           /* and along the inline axis */
    int vp_b, vp_i;              /* the scroller's scrollport size */
    int sc_b, sc_i;              /* its scroll offset */
    int rg_b, rg_i;              /* its scroll range, >= 0 */
    unsigned char sv;            /* Lot 105: 1 when the element is a box scroller itself */
    int sm;                      /* Lot 105: the nearest box scroller's reader index + 1, 0 none (root, unmarked) */
    int s_vp, s_sc, s_rg;        /* Lot 105: its own scrollport height, scroll offset and range along y (sv) */
} wf_geom_box_t;

typedef struct {
    int n;                       /* boxes filled (ok) */
    int vw, vh;                  /* the root scrollport (viewport) */
    int rg_x, rg_y;              /* the root scroll range, >= 0 */
    int sc_y;                    /* Lot 105: the root scroll offset along y (the GUI scroll position) */
    wf_geom_box_t box[WF_GEOM_MAX];
} wf_geom_t;

#endif
