/* webeng_cq.inc - Lot 119: container queries (CSS Conditional 5).
 * Included by webeng.c only, the single engine.
 *
 * - container-type: normal | size | inline-size; container-name: none or
 *   identifiers (case-sensitive); the container shorthand "name / type".
 * - @container [name] condition: the condition of @media without media
 *   types (not, and, or, parentheses) over the size features width,
 *   height, inline-size, block-size (lengths in px, em and rem of the
 *   container, three decimals), aspect-ratio and orientation, in the
 *   name: value, min-/max- and range forms. A name alone queries the
 *   nearest ancestor with that name. Nested @container blocks all apply.
 * - The query container of an element is its nearest ancestor (for a
 *   pseudo-element, its originating element first) that has the name (when one is given) and a container-type for the axes the
 *   features read (inline: width, inline-size; block: height, block-size;
 *   both: aspect-ratio, orientation). Without one, or before a layout gave
 *   its size, the condition is unknown and the rules do not apply.
 * - Sizes are the content box of the last layout: we_layout_viewport
 *   styles and lays the document out again while they change (at most
 *   WE_CQ_PASSES times). size containment lays the container's height
 *   out as if it were empty; both types make its intrinsic inline size
 *   that of an empty box (CSS Contain 2, 3.1 and 3.2).
 * - The horizontal writing mode: the inline axis is the width. */

static int vertical_extra(const we_node_t *n);

/* the condition being evaluated: its container's content box in
 * hundredths of a px and its font for em; cq_axes collects the axes the
 * features read (cq_rule checks the syntax against a 0 x 0 box) */
static int cq_axes, cq_have_w, cq_have_h, cq_font100;

/* a size: a number (three decimals, at most 100000) in px, em or rem, or
 * 0 alone, in hundredths of a px */
static int cq_len(const char *v, int n, int *out) {
    int i = 0, whole = 0, frac = 0, scale = 1000, dig = 0, base;
    while (i < n && v[i] >= '0' && v[i] <= '9') {
        whole = whole * 10 + (v[i] - '0'); i++; dig++;
        if (whole > 100000) return -1;
    }
    if (i < n && v[i] == '.') {
        int fd = 0;
        i++;
        while (i < n && v[i] >= '0' && v[i] <= '9') { if (scale > 1) { scale /= 10; frac += (v[i] - '0') * scale; } i++; fd++; }
        if (!fd) return -1;
        dig += fd;
    }
    if (!dig) return -1;
    if (i == n) { if (whole || frac) return -1; *out = 0; return 0; }
    if (cssq_is(v + i, n - i, "px")) base = 100;
    else if (cssq_is(v + i, n - i, "em")) base = cq_font100;
    else if (cssq_is(v + i, n - i, "rem")) base = u_root100;
    else return -1;
    /* whole * base <= 1e5 * 6400; frac * base / 1000 */
    *out = whole * base + (frac * base + 500) / 1000;
    return 0;
}

/* a size feature: its axes and value (hundredths of a px, or the ratio x
 * 1000 for aspect-ratio: kind 3); 0 when it is not one */
static int cq_range_value(const char *name, int n, int *value) {
    if (cssq_is(name, n, "width") || cssq_is(name, n, "inline-size")) {
        cq_axes |= WE_CQ_AXIS_INLINE; *value = cq_have_w; return 1;
    }
    if (cssq_is(name, n, "height") || cssq_is(name, n, "block-size")) {
        cq_axes |= WE_CQ_AXIS_BLOCK; *value = cq_have_h; return 1;
    }
    if (cssq_is(name, n, "aspect-ratio")) {
        cq_axes |= WE_CQ_AXIS_INLINE | WE_CQ_AXIS_BLOCK;
        *value = cq_have_h > 0 ? cq_have_w / cq_have_h * 1000 + cq_have_w % cq_have_h * 1000 / cq_have_h : 0;
        return 3;
    }
    return 0;
}

static int cq_value(int kind, const char *v, int n, int *out) {
    return kind == 3 ? cssq_ratio(v, n, out) : cq_len(v, n, out);
}

/* "name op value", "value op name", "v1 op name op v2" */
static int cq_feature_range(const char *s, int n) {
    int i = 0, a = 0, count = 0, ops[2], pa[3], pb[3], name = -1, kind = 0, have = 0, k, want, lo, hi;
    while (i < n) {
        if (s[i] == '<' || s[i] == '>' || s[i] == '=') {
            if (count == 2) return -1;
            pa[count] = a; pb[count] = i;
            ops[count] = cssq_op(s, n, &i);
            a = i;
            count++;
        } else i++;
    }
    pa[count] = a; pb[count] = n;
    for (k = 0; k <= count; k++) {
        int x = pa[k], y = pb[k], kk;
        if (!cssq_trim(s, &x, &y)) return -1;
        pa[k] = x; pb[k] = y;
        kk = cq_range_value(s + x, y - x, &have);
        if (kk) { name = k; kind = kk; }   /* a second name fails as a value */
    }
    if (name < 0) return -1;
    if (count == 2) {
        if ((ops[0] == CSSQ_LT || ops[0] == CSSQ_LE) != (ops[1] == CSSQ_LT || ops[1] == CSSQ_LE) ||
            ops[0] == CSSQ_EQ || ops[1] == CSSQ_EQ) return -1;
        if (cq_value(kind, s + pa[0], pb[0] - pa[0], &lo) || cq_value(kind, s + pa[2], pb[2] - pa[2], &hi)) return -1;
        return cssq_compare(have, cssq_flip(ops[0]), lo) && cssq_compare(have, ops[1], hi);
    }
    if (cq_value(kind, s + pa[name ? 0 : 1], pb[name ? 0 : 1] - pa[name ? 0 : 1], &want)) return -1;
    return cssq_compare(have, name ? cssq_flip(ops[0]) : ops[0], want);
}

/* one container feature inside its parentheses: 1, 0, -1 bad */
static int cq_feature(const char *s, int n) {
    int colon = 0, a = 0, b = n, va, vb, kind, have, want, prefix = 0, i;
    while (colon < n && s[colon] != ':') colon++;
    if (colon == n) {
        for (i = 0; i < n; i++) if (s[i] == '<' || s[i] == '>' || s[i] == '=') return cq_feature_range(s, n);
        cssq_trim(s, &a, &b);
        /* boolean context: true unless the value is 0 */
        if (cq_range_value(s + a, b - a, &have)) return have != 0;
        if (cssq_is(s + a, b - a, "orientation")) { cq_axes |= WE_CQ_AXIS_INLINE | WE_CQ_AXIS_BLOCK; return 1; }
        return -1;
    }
    b = colon;
    if (!cssq_trim(s, &a, &b)) return -1;
    va = colon + 1; vb = n;
    if (!cssq_trim(s, &va, &vb)) return -1;
    if (b - a > 4 && w_ieq_n(s + a, 4, "min-")) { prefix = CSSQ_GE; a += 4; }
    else if (b - a > 4 && w_ieq_n(s + a, 4, "max-")) { prefix = CSSQ_LE; a += 4; }
    kind = cq_range_value(s + a, b - a, &have);
    if (kind) {
        if (cq_value(kind, s + va, vb - va, &want)) return -1;
        return cssq_compare(have, prefix ? prefix : CSSQ_EQ, want);
    }
    if (!prefix && cssq_is(s + a, b - a, "orientation")) {
        int portrait = cq_have_h >= cq_have_w;
        cq_axes |= WE_CQ_AXIS_INLINE | WE_CQ_AXIS_BLOCK;
        if (cssq_is(s + va, vb - va, "portrait")) return portrait;
        if (cssq_is(s + va, vb - va, "landscape")) return !portrait;
    }
    return -1;
}

/* a container name: an identifier other than the CSS-wide keywords, none,
 * and, or, not (case-insensitive) */
static int cq_name_ok(const char *v, int n) {
    static const char *const bad[] = { "none", "and", "or", "not", "initial", "inherit", "unset", "revert",
                                       "revert-layer", "default", 0 };
    int k;
    if (!prop_ident(v, n)) return 0;
    for (k = 0; bad[k]; k++) if (value_is(v, n, bad[k])) return 0;
    return 1;
}

/* @container prelude s[0..n): the condition index, -1 refused */
static int cq_rule(we_doc_t *d, const char *s, int n, int parent) {
    int a = 0, b = n, name_a = 0, name_b = 0, r;
    we_cq_t *q;
    cssq_trim(s, &a, &b);
    if (a < b && s[a] != '(') {
        int i = a;
        while (i < b && w_namec(s[i])) i++;
        /* "not (...)" is a condition, not a name */
        if (!(i - a == 3 && w_ieq_n(s + a, 3, "not"))) {
            if (!cq_name_ok(s + a, i - a)) return -1;
            name_a = a; name_b = i;
            a = i;
            cssq_trim(s, &a, &b);
        }
    }
    cq_axes = 0; cq_have_w = cq_have_h = 0; cq_font100 = WE_FONT_PX_DEFAULT * 100;
    r = a < b ? cssq_cond(d, s + a, b - a, 2, 0) : (name_b > name_a ? 0 : -1);
    if (r < 0) return -1;
    if (d->ncq >= WE_CQ_MAX) return -2;
    q = &d->cqs[d->ncq];
    q->name_off = (uint16_t)(s + name_a - d->css); q->name_len = (uint8_t)(name_b - name_a);
    q->cond_off = (uint16_t)(s + a - d->css); q->cond_len = (uint16_t)(b - a);
    q->axes = (uint8_t)cq_axes; q->parent = (int8_t)parent;
    return d->ncq++;
}

/* whether container c has the name s[0..n) in its container-name list */
static int cq_has_name(const we_doc_t *d, const we_node_t *c, const char *s, int n) {
    const char *v = d->pool + c->cq_name_off;
    int i = 0, len = c->cq_name_len;
    while (i < len) {
        int a, k;
        while (i < len && w_space(v[i])) i++;
        a = i;
        while (i < len && !w_space(v[i])) i++;
        if (i - a != n) continue;
        for (k = 0; k < n && v[a + k] == s[k]; k++) {}
        if (k == n) return 1;
    }
    return 0;
}

/* the query container of element idx for condition q, -1 when none: an
 * ancestor, or for a pseudo-element its originating element idx too */
static int cq_container(const we_doc_t *d, int idx, const we_cq_t *q, int pseudo) {
    int p = pseudo ? idx : d->nodes[idx].parent;
    for (; p > 0; p = d->nodes[p].parent) {
        const we_node_t *c = &d->nodes[p];
        if (q->name_len && !cq_has_name(d, c, d->css + q->name_off, q->name_len)) continue;
        if ((q->axes & WE_CQ_AXIS_INLINE) && c->cq_type == WE_CQ_NORMAL) continue;
        if ((q->axes & WE_CQ_AXIS_BLOCK) && c->cq_type != WE_CQ_SIZE) continue;
        return p;
    }
    return -1;
}

/* whether the rule's @container conditions (innermost k and those around
 * it) hold for element idx (or its pseudo-element) */
static int cq_match(we_doc_t *d, int idx, int k, int pseudo) {
    d->cq_used = 1;
    for (; k >= 0; k = d->cqs[k].parent) {
        const we_cq_t *q = &d->cqs[k];
        int c = cq_container(d, idx, q, pseudo), r;
        if (c < 0) return 0;
        if (!q->cond_len) continue;          /* a name alone: no size read */
        if (!d->nodes[c].cq_known) return 0;
        cq_have_w = d->nodes[c].cq_w * 100; cq_have_h = d->nodes[c].cq_h * 100;
        cq_font100 = d->nodes[c].font100 ? d->nodes[c].font100 : WE_FONT_PX_DEFAULT * 100;
        cq_axes = 0;
        r = cssq_cond(d, d->css + q->cond_off, q->cond_len, 2, 0);
        if (r != 1) return 0;
    }
    return 1;
}

/* the container units of element n: the content box of its nearest
 * ancestor container for each axis, once a layout gave it */
static void cq_units_set(const we_doc_t *d, const we_node_t *n) {
    int p = n->parent, have_w = 0, have_h = 0;
    u_cqw = u_cqh = -1;
    for (; p > 0 && !have_h; p = d->nodes[p].parent) {
        const we_node_t *c = &d->nodes[p];
        if (c->cq_type == WE_CQ_NORMAL) continue;
        if (!have_w) { have_w = 1; u_cqw = c->cq_known ? c->cq_w : -1; }
        if (c->cq_type == WE_CQ_SIZE) { have_h = 1; u_cqh = c->cq_known ? c->cq_h : -1; }
    }
}

/* a container-type value, -1 when refused */
static int cq_type_of(const char *v, int n) {
    if (value_is(v, n, "normal")) return WE_CQ_NORMAL;
    if (value_is(v, n, "size")) return WE_CQ_SIZE;
    if (value_is(v, n, "inline-size")) return WE_CQ_INLINE;
    return -1;
}

/* container-type, container-name, container on n; a refused value keeps
 * the earlier one */
static void cq_apply(we_doc_t *d, we_node_t *n, const char *prop, const char *v, int vl) {
    int a = 0, b = vl, slash = -1, k, i, set_type = -1;
    uint16_t off;
    if (w_eq(prop, "container-type")) {
        k = cq_type_of(v, vl);
        if (k < 0) diag(d, "container type refused", -1);
        else n->cq_type = (uint8_t)k;
        return;
    }
    if (w_eq(prop, "container")) {
        for (k = 0; k < vl; k++) if (v[k] == '/') { slash = k; break; }
        set_type = WE_CQ_NORMAL;
        if (slash >= 0) {
            int ta = slash + 1, tb = vl;
            cssq_trim(v, &ta, &tb);
            set_type = cq_type_of(v + ta, tb - ta);
            b = slash;
            cssq_trim(v, &a, &b);
            if (set_type < 0 || a == b) { diag(d, "container refused", -1); return; }
        }
    }
    cssq_trim(v, &a, &b);
    if (value_is(v + a, b - a, "none")) n->cq_name_len = 0;
    else {
        for (i = a; i < b;) {
            int s = i;
            while (i < b && !w_space(v[i])) i++;
            if (!cq_name_ok(v + s, i - s)) { diag(d, set_type >= 0 ? "container refused" : "container name refused", -1); return; }
            while (i < b && w_space(v[i])) i++;
        }
        if (b - a > 255 || cssq_pool_ref(d, v + a, b - a, &off)) { diag(d, "container name refused", -1); return; }
        n->cq_name_off = off; n->cq_name_len = (uint8_t)(b - a);
    }
    if (set_type >= 0) n->cq_type = (uint8_t)set_type;
}

/* 1 when prop is container-type, container-name or container (applied) */
static int cq_declaration(we_doc_t *d, we_node_t *n, const char *prop, const char *v, int vl) {
    if (!w_eq(prop, "container-type") && !w_eq(prop, "container-name") && !w_eq(prop, "container")) return 0;
    cq_apply(d, n, prop, v, vl);
    return 1;
}

/* after a layout: the content boxes of the containers; 1 when one is new
 * or changed (the styles that read them are stale) */
static int cq_record(we_doc_t *d) {
    int i, changed = 0;
    for (i = 1; i < (int)d->nnodes; i++) {
        we_node_t *n = &d->nodes[i];
        int w, h;
        if (n->cq_type == WE_CQ_NORMAL || n->display == WE_DISP_NONE) {
            if (n->cq_known) changed = 1;
            n->cq_known = 0;
            continue;
        }
        w = n->w - box_edge(n, 1) - box_edge(n, 3) - n->padding[1] - n->padding[3];
        h = n->h - vertical_extra(n);
        w = clamp_int(w, 0, WE_LAYOUT_MAX); h = clamp_int(h, 0, WE_LAYOUT_MAX);
        if (!n->cq_known || n->cq_w != w || n->cq_h != h) changed = 1;
        n->cq_w = (int16_t)w; n->cq_h = (int16_t)h; n->cq_known = 1;
    }
    return changed;
}
