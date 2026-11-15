/* Lot 104: the layout geometry of an element for the reader style engine
 * (wf_geom.h), read from the boxes the layout left in the document; the
 * layout itself is not changed. The nearest scroller is the nearest
 * ancestor that scrolls vertically (overflow auto or scroll, Lot 74),
 * else the root (the viewport of we_layout_viewport). Positions are taken
 * from the scroller's content start: the page origin for the root, the
 * padding edge of a box scroller moved back by its scroll offset (its
 * children were shifted by it, scroll_apply). A box scroller scrolls
 * along y only. The block and inline axes are those of the scroller's
 * writing mode (the root element's for the root); in vertical-rl the
 * block offset is taken from the block start (the right edge of the
 * scroller's content box, of the root element for the root), the inline
 * offsets stay physical (direction is not read). */

/* 1 when node i or an ancestor is display:none (no box) */
static int geom_hidden(const we_doc_t *d, int i) {
    int guard = 0;
    for (; i >= 0 && guard < WE_MAX_NODES; i = d->nodes[i].parent, guard++)
        if (d->nodes[i].display == WE_DISP_NONE) return 1;
    return 0;
}

/* the root element (the html element under the document node), or -1 */
static int geom_root_el(const we_doc_t *d) {
    int i;
    for (i = 0; i < (int)d->nnodes; i++)
        if (d->nodes[i].parent == d->root && !d->nodes[i].is_text) return i;
    return -1;
}

/* the physical geometry of node i: its border box (x, y, w, h) from the
 * scroller's content start, the scrollport (vw, vh), scroll offset (sx,
 * sy) and range (rx, ry); 1 when the scroller is the root; its writing
 * mode in *wm */
static int geom_phys(const we_doc_t *d, int i, int *r, int *wm) {
    const we_node_t *n = &d->nodes[i];
    int s, vw, vh;
    for (s = n->parent; s >= 0 && !overflow_scrollable(&d->nodes[s]); s = d->nodes[s].parent) {}
    if (s >= 0) *wm = d->nodes[s].wmode;
    else {
        int h = geom_root_el(d);
        *wm = h >= 0 ? d->nodes[h].wmode : WE_WM_HORIZONTAL;
    }
    r[0] = n->x; r[1] = n->y; r[2] = n->w; r[3] = n->h;
    if (s < 0) {
        /* the viewport of we_layout_viewport (0 before a layout); the
         * engine's page width is the viewport width (it does not track
         * a wider overflow), so the root scrolls along y only */
        vw = d->viewport_w;
        vh = d->viewport_h;
        r[4] = vw; r[5] = vh;
        r[6] = 0; r[7] = d->scroll_y;
        r[8] = 0;
        r[9] = (int)d->perf.page_h > vh ? (int)d->perf.page_h - vh : 0;
        return 1;
    }
    {
        const we_node_t *c = &d->nodes[s];
        int left = c->x + box_edge(c, 3) + c->padding[3], top = c->y + box_edge(c, 0) + c->padding[0];
        int view = scroll_view_h(c), content = scroll_content_bottom(d, s) + c->box_scroll_y;
        int w = c->w - box_edge(c, 1) - box_edge(c, 3) - c->padding[1] - c->padding[3];
        r[0] = n->x - left;
        r[1] = n->y - (top - c->box_scroll_y);
        r[4] = w > 0 ? w : 0; r[5] = view;
        r[6] = 0; r[7] = c->box_scroll_y;
        r[8] = 0; r[9] = content > view ? content - view : 0;
    }
    return 0;
}

int we_geom(const we_doc_t *d, int node, wf_geom_box_t *g) {
    int r[10], root, v, wm;
    if (!g) return -1;
    g->ok = 0;
    if (!d || node < 0 || node >= (int)d->nnodes || d->nodes[node].is_text || geom_hidden(d, node)) return -1;
    root = geom_phys(d, node, r, &wm);
    v = wm == WE_WM_HORIZONTAL;
    if (wm == WE_WM_VERTICAL_RL) {
        /* from the block start: the right edge of the scroller's content
         * box, for the root the right edge of the root element (the
         * engine lays a vertical-rl page out from the left) */
        int h = root ? geom_root_el(d) : -1, right = h >= 0 ? d->nodes[h].x + d->nodes[h].w : r[4];
        r[0] = right - (r[0] + r[2]);
    }
    g->ok = 1;
    g->root = (unsigned char)root;
    g->bvert = (unsigned char)v;
    /* the block axis is y in horizontal-tb, x in the vertical modes */
    g->b_off = r[v ? 1 : 0]; g->b_size = r[v ? 3 : 2];
    g->i_off = r[v ? 0 : 1]; g->i_size = r[v ? 2 : 3];
    g->vp_b = r[v ? 5 : 4]; g->vp_i = r[v ? 4 : 5];
    g->sc_b = r[v ? 7 : 6]; g->sc_i = r[v ? 6 : 7];
    g->rg_b = r[v ? 9 : 8]; g->rg_i = r[v ? 8 : 9];
    return 0;
}

/* the reader index of an id "<prefix><digits>" (no leading zero, below
 * WF_GEOM_MAX), else -1 */
static int geom_mark(const char *id, const char *prefix) {
    int k = 0, v = 0, dg = 0;
    while (prefix[k] && id[k] == prefix[k]) k++;
    if (prefix[k]) return -1;
    for (; id[k] >= '0' && id[k] <= '9'; k++, dg++) {
        v = v * 10 + (id[k] - '0');
        if (v >= WF_GEOM_MAX) return -1;     /* this also keeps v small */
    }
    if (!dg || id[k] || (dg > 1 && id[k - dg] == '0')) return -1;
    return v;
}

int we_geom_feed(const we_doc_t *d, const char *prefix, wf_geom_t *g) {
    int i, k, r[10], wm;
    if (!g) return -1;
    for (k = 0; k < (int)sizeof(*g); k++) ((unsigned char *)g)[k] = 0;
    if (!d || !prefix || !prefix[0] || d->root < 0 || !d->nnodes) return 0;
    (void)geom_phys(d, d->root, r, &wm);
    g->vw = r[4]; g->vh = r[5]; g->rg_x = r[8]; g->rg_y = r[9];
    for (i = 0; i < (int)d->nnodes; i++) {
        int m = geom_mark(d->nodes[i].id, prefix);     /* a text node has no id */
        if (m < 0 || g->box[m].ok) continue;
        if (we_geom(d, i, &g->box[m]) == 0) g->n++;
    }
    return g->n;
}
