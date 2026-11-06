/* Lot 74: overflow:auto/scroll paint a vertical scrollbar and scroll the
 * box contents by box_scroll_y. Hidden/clip keep clipping without a bar.
 * The bar is WE_SCROLL_W px inside the padding edge on the right. Offsets
 * persist in we_forms_t.scroll_* across renders when forms are attached. */
#define WE_SCROLL_W 6

static int overflow_scrollable(const we_node_t *n) {
    return (n->overflow_scroll & 2) != 0;
}

static int scroll_content_bottom(const we_doc_t *d, int idx) {
    const we_node_t *n = &d->nodes[idx];
    int pad_top = n->y + box_edge(n, 0) + n->padding[0];
    int bottom = pad_top, c;
    for (c = n->first; c >= 0; c = d->nodes[c].next) {
        const we_node_t *ch = &d->nodes[c];
        int b;
        if (ch->display == WE_DISP_NONE || ch->absolute || ch->fixed) continue;
        b = ch->y + ch->h;
        if (b > bottom) bottom = b;
    }
    return bottom - pad_top;
}

static int scroll_view_h(const we_node_t *n) {
    int h = n->h - box_edge(n, 0) - box_edge(n, 2) - n->padding[0] - n->padding[2];
    return h > 0 ? h : 0;
}

static void scroll_shift(we_doc_t *d, int idx, int dy) {
    int c, i;
    if (!dy) return;
    for (c = d->nodes[idx].first; c >= 0; c = d->nodes[c].next) {
        we_node_t *ch = &d->nodes[c];
        int stack[WE_MAX_NODES], sp = 0, k;
        if (ch->display == WE_DISP_NONE || ch->absolute || ch->fixed) continue;
        ch->y = (int16_t)(ch->y + dy);
        stack[sp++] = c;
        while (sp) {
            int cur = stack[--sp];
            for (k = d->nodes[cur].first; k >= 0; k = d->nodes[k].next) {
                if (d->nodes[k].absolute || d->nodes[k].fixed) continue;
                d->nodes[k].y = (int16_t)(d->nodes[k].y + dy);
                if (d->nodes[k].first >= 0 && sp < WE_MAX_NODES) stack[sp++] = k;
            }
        }
    }
    for (i = 0; i < (int)d->nitems; i++) {
        int own = d->item_owner[i], p;
        if (own < 0) continue;
        p = own;
        while (p >= 0 && p != idx) p = d->nodes[p].parent;
        if (p == idx && own != idx) d->items[i].y = (int16_t)(d->items[i].y + dy);
    }
}

static int scroll_node_id(const we_doc_t *d, int idx, char *out, int max) {
    const we_node_t *n = &d->nodes[idx];
    if (!n->id[0]) { out[0] = 0; return -1; }
    w_cpy(out, max, n->id, w_len(n->id));
    return 0;
}

static void scroll_restore(we_doc_t *d, int idx) {
    int k;
    if (!d->forms || !d->forms->nscroll || !d->nodes[idx].id[0]) return;
    for (k = 0; k < d->forms->nscroll; k++)
        if (w_eq(d->forms->scroll_id[k], d->nodes[idx].id)) {
            d->nodes[idx].box_scroll_y = d->forms->scroll_y[k];
            return;
        }
}

static void scroll_apply(we_doc_t *d, int idx) {
    we_node_t *n = &d->nodes[idx];
    int content, view, max_scroll, want;
    if (!overflow_scrollable(n)) return;
    scroll_restore(d, idx);
    content = scroll_content_bottom(d, idx);
    view = scroll_view_h(n);
    max_scroll = content > view ? content - view : 0;
    want = n->box_scroll_y;
    if (want < 0) want = 0;
    if (want > max_scroll) want = max_scroll;
    n->box_scroll_y = (int16_t)want;
    if (want) scroll_shift(d, idx, -want);
}

static int scroll_paint(we_doc_t *d, int idx) {
    we_node_t *n = &d->nodes[idx];
    int view, content, max_scroll, track_x, track_y, track_h, thumb_h, thumb_y, ratio;
    if (!overflow_scrollable(n) || d->nitems + 2 > WE_MAX_ITEMS) return 0;
    view = scroll_view_h(n);
    content = scroll_content_bottom(d, idx) + n->box_scroll_y;
    if (content <= view) return 0;
    max_scroll = content - view;
    track_x = n->x + n->w - box_edge(n, 1) - WE_SCROLL_W;
    track_y = n->y + box_edge(n, 0);
    track_h = n->h - box_edge(n, 0) - box_edge(n, 2);
    if (track_h < 4 || track_x < n->x) return 0;
    thumb_h = view * track_h / content;
    if (thumb_h < 4) thumb_h = 4;
    if (thumb_h > track_h) thumb_h = track_h;
    ratio = max_scroll > 0 ? (int)(((long)(track_h - thumb_h) * n->box_scroll_y) / max_scroll) : 0;
    thumb_y = track_y + ratio;
    {
        os_fb_web_item_t *it = &d->items[d->nitems];
        it->kind = OS_FB_WEB_RECT; it->x = (int16_t)track_x; it->y = (int16_t)track_y;
        it->w = WE_SCROLL_W; it->h = (uint16_t)track_h; it->color = 0xDDDDDD;
        d->item_owner[d->nitems++] = (int16_t)idx;
        it = &d->items[d->nitems];
        it->kind = OS_FB_WEB_RECT; it->x = (int16_t)track_x; it->y = (int16_t)thumb_y;
        it->w = WE_SCROLL_W; it->h = (uint16_t)thumb_h; it->color = 0x888888;
        d->item_owner[d->nitems++] = (int16_t)idx;
    }
    return 2;
}

static void scroll_persist(we_doc_t *d, int idx, int want) {
    char id[WE_TARGET_MAX];
    int k;
    if (!d->forms || scroll_node_id(d, idx, id, WE_TARGET_MAX) != 0 || !id[0]) {
        d->nodes[idx].box_scroll_y = (int16_t)want;
        return;
    }
    for (k = 0; k < d->forms->nscroll; k++)
        if (w_eq(d->forms->scroll_id[k], id)) { d->forms->scroll_y[k] = (int16_t)want; d->nodes[idx].box_scroll_y = (int16_t)want; return; }
    if (d->forms->nscroll < WE_BOXSCROLL_MAX) {
        k = d->forms->nscroll++;
        w_cpy(d->forms->scroll_id[k], WE_TARGET_MAX, id, w_len(id));
        d->forms->scroll_y[k] = (int16_t)want;
    }
    d->nodes[idx].box_scroll_y = (int16_t)want;
}

int we_box_scroll_click(we_doc_t *d, int x, int y) {
    int i;
    if (!d) return -1;
    for (i = (int)d->nnodes - 1; i >= 1; i--) {
        we_node_t *n = &d->nodes[i];
        int track_x, track_y, track_h, content, view, max_scroll, rel, want;
        if (!overflow_scrollable(n) || n->display == WE_DISP_NONE) continue;
        track_x = n->x + n->w - box_edge(n, 1) - WE_SCROLL_W;
        track_y = n->y + box_edge(n, 0);
        track_h = n->h - box_edge(n, 0) - box_edge(n, 2);
        if (x < track_x || x >= track_x + WE_SCROLL_W || y < track_y || y >= track_y + track_h) continue;
        view = scroll_view_h(n);
        content = scroll_content_bottom(d, i) + n->box_scroll_y;
        max_scroll = content > view ? content - view : 0;
        if (max_scroll <= 0 || track_h < 1) return 0;
        rel = y - track_y;
        want = (int)(((long)max_scroll * rel) / track_h);
        if (want < 0) want = 0;
        if (want > max_scroll) want = max_scroll;
        if (want == n->box_scroll_y) return 1;
        scroll_persist(d, i, want);
        return 2;
    }
    return 0;
}
