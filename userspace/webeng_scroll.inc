/* Lot 74: overflow:auto/scroll paint a vertical scrollbar and scroll the
 * box contents by box_scroll_y. Hidden/clip keep clipping without a bar.
 * The bar is WE_SCROLL_W px inside the padding edge on the right. Offsets
 * persist in we_forms_t.scroll_* across renders when forms are attached.
 * Lot 108: overflow-x auto/scroll scroll the contents along x by
 * box_scroll_x, with a horizontal bar WE_SCROLL_W px high inside the
 * padding edge at the bottom (left of the vertical bar when both are
 * painted). The content width is the right edge of the in-flow
 * descendant boxes and text runs (a nowrap line is wider than its
 * block), from the content edge; out-of-flow and hidden
 * subtrees and the insides of a box clipping along x do not count. It is
 * measured by scroll_apply before the display list is clipped (box_cw). */
#define WE_SCROLL_W 6

static int overflow_scrollable(const we_node_t *n) {
    return (n->overflow_scroll & 2) != 0;
}

/* Lot 108: overflow-x auto or scroll */
static int overflow_scrollable_x(const we_node_t *n) {
    return (n->overflow_scroll & 1) != 0;
}

/* Lot 108: 1 when node c counts for the content width of box idx: an
 * in-flow descendant (no node from c up to idx, idx excluded, is hidden
 * or out of flow) not clipped along x by a box between them */
static int scroll_in_flow(const we_doc_t *d, int c, int idx) {
    int guard = 0, p;
    for (p = c; p >= 0 && guard < WE_MAX_NODES; p = d->nodes[p].parent, guard++) {
        const we_node_t *q = &d->nodes[p];
        if (p == idx) return 1;
        if (q->display == WE_DISP_NONE || q->absolute || q->fixed) return 0;
        if (p != c && (q->overflow_axes & 1)) return 0;
    }
    return 0;
}

/* Lot 108: the content width of box idx from its left content edge: the
 * right edges of the boxes that count, text runs included (a child
 * narrower than its own child does not hide it; a box shadow or an
 * outline painted past a box does not count) */
static int scroll_content_right(const we_doc_t *d, int idx) {
    const we_node_t *n = &d->nodes[idx];
    int pad_left = n->x + box_edge(n, 3) + n->padding[3];
    int right = pad_left, c;
    for (c = idx + 1; c < (int)d->nnodes; c++) {
        const we_node_t *ch = &d->nodes[c];
        if (!scroll_in_flow(d, c, idx)) continue;
        if (ch->x + ch->w > right) right = ch->x + ch->w;
    }
    return right - pad_left;
}

/* Lot 108: the scrollport width (the content box, as scroll_view_h; the
 * layout keeps a border box at least as wide as its borders and padding) */
static int scroll_view_w(const we_node_t *n) {
    return n->w - box_edge(n, 1) - box_edge(n, 3) - n->padding[1] - n->padding[3];
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

/* moves the in-flow contents of box idx by dx, dy (Lot 108: dx too) */
static void scroll_shift(we_doc_t *d, int idx, int dx, int dy) {
    int c, i;
    if (!dx && !dy) return;
    for (c = d->nodes[idx].first; c >= 0; c = d->nodes[c].next) {
        we_node_t *ch = &d->nodes[c];
        int stack[WE_MAX_NODES], sp = 0, k;
        if (ch->display == WE_DISP_NONE || ch->absolute || ch->fixed) continue;
        ch->x = (int16_t)(ch->x + dx);
        ch->y = (int16_t)(ch->y + dy);
        stack[sp++] = c;
        while (sp) {
            int cur = stack[--sp];
            for (k = d->nodes[cur].first; k >= 0; k = d->nodes[k].next) {
                if (d->nodes[k].absolute || d->nodes[k].fixed) continue;
                d->nodes[k].x = (int16_t)(d->nodes[k].x + dx);
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
        if (p == idx && own != idx) {
            d->items[i].x = (int16_t)(d->items[i].x + dx);
            d->items[i].y = (int16_t)(d->items[i].y + dy);
        }
    }
}

/* the key of a box's offset in the forms: its id; Lot 110: " body" for
 * the body element without one (the body box scroller the reader
 * passes; an id never holds a space) */
static int scroll_node_id(const we_doc_t *d, int idx, char *out, int max) {
    const we_node_t *n = &d->nodes[idx];
    if (!n->id[0] && w_eq(n->tag, "body")) {
        w_cpy(out, max, " body", 5);
        return 0;
    }
    if (!n->id[0]) { out[0] = 0; return -1; }
    w_cpy(out, max, n->id, w_len(n->id));
    return 0;
}

static void scroll_restore(we_doc_t *d, int idx) {
    char id[WE_TARGET_MAX];
    int k;
    if (!d->forms || !d->forms->nscroll || scroll_node_id(d, idx, id, WE_TARGET_MAX) != 0) return;
    for (k = 0; k < d->forms->nscroll; k++)
        if (w_eq(d->forms->scroll_id[k], id)) {
            d->nodes[idx].box_scroll_y = d->forms->scroll_y[k];
            d->nodes[idx].box_scroll_x = d->forms->scroll_x[k];     /* Lot 108 */
            return;
        }
}

static void scroll_apply(we_doc_t *d, int idx) {
    we_node_t *n = &d->nodes[idx];
    int content, view, max_scroll, want, wx = 0;
    if (!overflow_scrollable(n) && !overflow_scrollable_x(n)) return;
    scroll_restore(d, idx);
    if (overflow_scrollable_x(n)) {
        /* Lot 108: along x, before the y shift (it does not move x) */
        content = scroll_content_right(d, idx);
        n->box_cw = (int16_t)content;
        view = scroll_view_w(n);
        max_scroll = content > view ? content - view : 0;
        wx = n->box_scroll_x;
        if (wx < 0) wx = 0;
        if (wx > max_scroll) wx = max_scroll;
    }
    n->box_scroll_x = (int16_t)wx;
    want = 0;
    if (overflow_scrollable(n)) {
        content = scroll_content_bottom(d, idx);
        view = scroll_view_h(n);
        max_scroll = content > view ? content - view : 0;
        want = n->box_scroll_y;
        if (want < 0) want = 0;
        if (want > max_scroll) want = max_scroll;
    }
    n->box_scroll_y = (int16_t)want;
    scroll_shift(d, idx, -wx, -want);
}

/* Lot 108: the horizontal bar of box idx (ybar: the vertical one is
 * painted, the track stops left of it); its track in *tx, *ty, *tw */
static int scroll_xtrack(const we_doc_t *d, int idx, int ybar, int *tx, int *ty, int *tw) {
    const we_node_t *n = &d->nodes[idx];
    (void)d;
    *tx = n->x + box_edge(n, 3);
    *ty = n->y + n->h - box_edge(n, 2) - WE_SCROLL_W;
    *tw = n->w - box_edge(n, 1) - box_edge(n, 3) - (ybar ? WE_SCROLL_W : 0);
    return *tw >= 4 && *ty >= n->y;
}

/* Lot 108: 1 when box idx paints its vertical bar */
static int scroll_ybar(const we_doc_t *d, int idx) {
    const we_node_t *n = &d->nodes[idx];
    int track_h = n->h - box_edge(n, 0) - box_edge(n, 2);
    if (!overflow_scrollable(n) || track_h < 4 || n->x + n->w - box_edge(n, 1) - WE_SCROLL_W < n->x) return 0;
    return scroll_content_bottom(d, idx) + n->box_scroll_y > scroll_view_h(n);
}

static void scroll_paint_x(we_doc_t *d, int idx, int ybar) {
    we_node_t *n = &d->nodes[idx];
    int view, content, max_scroll, tx, ty, tw, thumb_w, ratio;
    /* box_cw is 0 for a box not scrolling along x (style_node) */
    if (d->nitems + 2 > WE_MAX_ITEMS) return;
    view = scroll_view_w(n);
    content = n->box_cw;
    /* Lot 130: content 0 is a box that does not scroll along x; a box narrower
     * than its own padding (an img, a fixed 40 px placeholder with padding:0 25px)
     * has a negative view, so content <= view alone let 0 through to the divide */
    if (content <= 0 || content <= view || !scroll_xtrack(d, idx, ybar, &tx, &ty, &tw)) return;
    max_scroll = content - view;
    thumb_w = view * tw / content;
    if (thumb_w < 4) thumb_w = 4;
    if (thumb_w > tw) thumb_w = tw;
    ratio = (int)(((long)(tw - thumb_w) * n->box_scroll_x) / max_scroll);
    {
        os_fb_web_item_t *it = &d->items[d->nitems];
        it->kind = OS_FB_WEB_RECT; it->x = (int16_t)tx; it->y = (int16_t)ty;
        it->w = (uint16_t)tw; it->h = WE_SCROLL_W; it->color = 0xDDDDDD;
        d->item_owner[d->nitems++] = (int16_t)idx;
        it = &d->items[d->nitems];
        it->kind = OS_FB_WEB_RECT; it->x = (int16_t)(tx + ratio); it->y = (int16_t)ty;
        it->w = (uint16_t)thumb_w; it->h = WE_SCROLL_W; it->color = 0x888888;
        d->item_owner[d->nitems++] = (int16_t)idx;
    }
}

static int scroll_paint(we_doc_t *d, int idx) {
    we_node_t *n = &d->nodes[idx];
    int view, content, max_scroll, track_x, track_y, track_h, thumb_h, thumb_y, ratio;
    /* Lot 108: the horizontal bar first (it reads the content before
     * the vertical bar's items exist) */
    scroll_paint_x(d, idx, scroll_ybar(d, idx));
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

/* the offset of box idx along y (Lot 108: x when ax), kept in the forms
 * by the box id when forms are attached */
static void scroll_persist(we_doc_t *d, int idx, int want, int ax) {
    char id[WE_TARGET_MAX];
    int k;
    int16_t *own = ax ? &d->nodes[idx].box_scroll_x : &d->nodes[idx].box_scroll_y;
    *own = (int16_t)want;
    if (!d->forms || scroll_node_id(d, idx, id, WE_TARGET_MAX) != 0 || !id[0]) return;
    for (k = 0; k < d->forms->nscroll; k++)
        if (w_eq(d->forms->scroll_id[k], id)) { (ax ? d->forms->scroll_x : d->forms->scroll_y)[k] = (int16_t)want; return; }
    if (d->forms->nscroll < WE_BOXSCROLL_MAX) {
        k = d->forms->nscroll++;
        w_cpy(d->forms->scroll_id[k], WE_TARGET_MAX, id, w_len(id));
        d->forms->scroll_y[k] = d->nodes[idx].box_scroll_y;
        d->forms->scroll_x[k] = d->nodes[idx].box_scroll_x;
    }
}

/* Lot 108: a click at x, y on the horizontal track of box i: 2 moved, 1
 * already there, 0 nothing to scroll, -1 not on the track */
static int scroll_click_x(we_doc_t *d, int i, int x, int y) {
    we_node_t *n = &d->nodes[i];
    int tx, ty, tw, content, view, max_scroll, want;
    if (!overflow_scrollable_x(n) || !scroll_xtrack(d, i, scroll_ybar(d, i), &tx, &ty, &tw)) return -1;
    if (x < tx || x >= tx + tw || y < ty || y >= ty + WE_SCROLL_W) return -1;
    view = scroll_view_w(n);
    content = n->box_cw;
    max_scroll = content > view ? content - view : 0;
    if (max_scroll <= 0) return 0;
    /* below max_scroll: x - tx < tw */
    want = (int)(((long)max_scroll * (x - tx)) / tw);
    if (want == n->box_scroll_x) return 1;
    scroll_persist(d, i, want, 1);
    return 2;
}

int we_box_scroll_click(we_doc_t *d, int x, int y) {
    int i;
    if (!d) return -1;
    for (i = (int)d->nnodes - 1; i >= 1; i--) {
        we_node_t *n = &d->nodes[i];
        int track_x, track_y, track_h, content, view, max_scroll, rel, want, r;
        if (n->display == WE_DISP_NONE) continue;
        /* Lot 108: the horizontal track first (it stops left of a
         * painted vertical bar, so the corner stays the vertical one's) */
        if ((r = scroll_click_x(d, i, x, y)) >= 0) return r;
        if (!overflow_scrollable(n)) continue;
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
        scroll_persist(d, i, want, 0);
        return 2;
    }
    return 0;
}
