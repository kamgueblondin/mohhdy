/* wf_jslayout.c - Lot 115: the renderer's boxes for the page scripts
 * (see wf_jslayout.h). */
#include "wf_jslayout.h"
#include "webeng.h"
#include "umalloc.h"

static we_doc_t *g_doc;
static int g_made, g_refused;

static char low(char c) { return (c >= 'A' && c <= 'Z') ? (char)(c + 32) : c; }

static int under(const we_doc_t *d, int k, int anc) {
    int guard = 0;
    for (; k >= 0 && guard < WE_MAX_NODES; k = d->nodes[k].parent, guard++) if (k == anc) return 1;
    return 0;
}

/* the engine boxes block-level elements; an inline element is painted as
 * the text items of its descendants: its box is their union (1), or 0
 * when none was painted (items past the engine's bound) */
static int inline_box(const we_doc_t *d, int node, wf_jslay_box_t *b) {
    uint32_t j;
    int x0 = 0, y0 = 0, x1 = 0, y1 = 0, any = 0;
    for (j = 0; j < d->nitems; j++) {
        const os_fb_web_item_t *it = &d->items[j];
        if (d->item_owner[j] < 0 || !under(d, d->item_owner[j], node)) continue;
        if (!any || it->x < x0) x0 = it->x;
        if (!any || it->y < y0) y0 = it->y;
        if (!any || it->x + (int)it->w > x1) x1 = it->x + (int)it->w;
        if (!any || it->y + (int)it->h > y1) y1 = it->y + (int)it->h;
        any = 1;
    }
    if (!any) return 0;
    b->x = x0; b->y = y0; b->w = x1 - x0; b->h = y1 - y0;
    b->cw = b->ch = 0;          /* clientWidth of an inline box is 0 */
    return 1;
}

int wf_jslayout_engine(void *ctx, const char *html, int n, int vw, int vh, wf_jslay_box_t *out, int max) {
    uint32_t i, last;
    int nb = 0;
    (void)ctx;
    if (!html || n < 0 || !out || max <= 0 || vw <= 0 || vh <= 0) return -1;
    if (!g_doc) {
        g_doc = (we_doc_t *)umalloc((unsigned)sizeof(we_doc_t));
        if (!g_doc) { g_refused++; return -1; }
        g_doc->forms = 0;       /* we_parse keeps forms: none here (controls are painted static) */
    }
    if (we_parse(g_doc, html, n) != 0) { g_refused++; return -1; }
    we_set_media(g_doc, vw, vh);
    if (we_style(g_doc) != 0 || we_layout_viewport(g_doc, vw, vh) != 0) { g_refused++; return -1; }
    g_made++;
    /* the source elements in document order (generated boxes come after) */
    last = g_doc->source_nodes ? g_doc->source_nodes : g_doc->nnodes;
    if (last > g_doc->nnodes) last = g_doc->nnodes;
    for (i = 1; i < last && nb < max; i++) {
        const we_node_t *d = &g_doc->nodes[i];
        wf_jslay_box_t *b;
        wf_geom_box_t g;
        int k;
        if (d->is_text || d->tag[0] == '#' || d->tag[0] == 0) continue;
        b = &out[nb++];
        for (k = 0; k < 11 && d->tag[k]; k++) b->tag[k] = low(d->tag[k]);
        b->tag[k] = 0;
        b->ok = (unsigned char)(we_geom(g_doc, (int)i, &g) == 0 && g.ok);
        b->x = d->x; b->y = d->y; b->w = d->w; b->h = d->h;
        b->cw = b->w; b->ch = b->h;
        we_client_box(g_doc, (int)i, &b->cw, &b->ch);
        if (!b->ok) b->x = b->y = b->w = b->h = b->cw = b->ch = 0;
        else if (d->display == WE_DISP_INLINE && d->w == 0 && d->h == 0 && !inline_box(g_doc, (int)i, b))
            b->ok = 2;
        else if (d->display == WE_DISP_INLINE) b->cw = b->ch = 0;
    }
    return nb;
}

void wf_jslayout_release(void) {
    if (g_doc) ufree(g_doc);
    g_doc = 0;
}

void wf_jslayout_counts(int *made, int *refused) {
    if (made) *made = g_made;
    if (refused) *refused = g_refused;
}
