/* webeng_hints.inc - Lot 73: HTML presentational hints (HTML Living
 * Standard 15.3, the "rendering" section). Included by webeng.c only.
 *
 * The parser keeps the presentational attributes of an element (up to
 * WE_HINTS_MAX, value text in the pool). The style pass turns them into
 * declarations applied after the UA styles and before every author rule
 * (author level, specificity zero), so CSS still wins:
 *   width / height (img, table, td, th, col, colgroup, hr, iframe, video,
 *     canvas, object, embed; height also tr): "N" px, "N%" percent, digits
 *     first, the rest ignored; 0 and junk refused ("hint refused");
 *   align: text-align on p, div, h1-h6, caption, td, th, tr, thead,
 *     tbody, tfoot, legend (left, center, right, justify; "middle" is
 *     center); table and hr: center gives auto inline margins, left /
 *     right float the table; img, iframe, object, embed: left / right
 *     float, top / middle / bottom / baseline vertical-align;
 *   valign (td, th, tr, thead, tbody, tfoot, col): vertical-align, a cell
 *     without its own takes the one of its row, then of its row group;
 *   bgcolor (body, table, tr, td, th, thead, tbody, tfoot): background;
 *     text (body), color (font): color; a value of 3 or 6 hex digits
 *     without "#" is read as hex (legacy colour);
 *   size (font): 1..7 or +N / -N from 3 (10, 13, 16, 18, 24, 32, 48 px);
 *   border (table): N px outset grey on the table and 1px inset grey on
 *     its cells when N > 0; border (img): N px solid;
 *   cellpadding (table): padding of its cells; cellspacing (table):
 *     border-spacing;
 *   nowrap (td, th): white-space nowrap;
 *   hspace / vspace (img, object, embed): margins;
 *   hidden (any element): display none.
 * <center> is a block with text-align center, <font> an inline; the UA
 * style of <caption> (block, centred) is set here too, align=top|bottom
 * on a caption is caption-side. */

enum { H_WIDTH = 1, H_HEIGHT, H_ALIGN, H_VALIGN, H_BGCOLOR, H_COLOR, H_SIZE, H_BORDER, H_CELLPADDING,
       H_CELLSPACING, H_NOWRAP, H_HSPACE, H_VSPACE, H_HIDDEN };

static void apply_decl(we_doc_t *d, we_node_t *n, const char *p, int pl, const char *v, int vl);

static int table_group_tag(const char *tag) {
    return w_eq(tag, "thead") || w_eq(tag, "tbody") || w_eq(tag, "tfoot");
}

static int hints_in(const char *tag, const char *const *list) {
    int k;
    for (k = 0; list[k]; k++) if (w_eq(tag, list[k])) return 1;
    return 0;
}

static const char *const hk_dims[] = { "img", "table", "td", "th", "col", "colgroup", "hr", "iframe", "video", "canvas", "object", "embed", 0 };
static const char *const hk_text_align[] = { "p", "div", "h1", "h2", "h3", "h4", "h5", "h6", "caption", "td", "th", "tr",
                                             "thead", "tbody", "tfoot", "legend", 0 };
static const char *const hk_valign[] = { "td", "th", "tr", "thead", "tbody", "tfoot", "col", 0 };
static const char *const hk_bgcolor[] = { "body", "table", "tr", "td", "th", "thead", "tbody", "tfoot", 0 };
static const char *const hk_replaced[] = { "img", "iframe", "object", "embed", 0 };

/* the attribute's hint kind for this element, 0 when it has none */
static int hints_kind(const char *tag, const char *an) {
    if (w_eq(an, "hidden")) return H_HIDDEN;
    if (w_eq(an, "width")) return hints_in(tag, hk_dims) ? H_WIDTH : 0;
    if (w_eq(an, "height")) return hints_in(tag, hk_dims) || w_eq(tag, "tr") ? H_HEIGHT : 0;
    if (w_eq(an, "align"))
        return hints_in(tag, hk_text_align) || w_eq(tag, "table") || w_eq(tag, "hr") || hints_in(tag, hk_replaced) ? H_ALIGN : 0;
    if (w_eq(an, "valign")) return hints_in(tag, hk_valign) ? H_VALIGN : 0;
    if (w_eq(an, "bgcolor")) return hints_in(tag, hk_bgcolor) ? H_BGCOLOR : 0;
    if (w_eq(an, "text")) return w_eq(tag, "body") ? H_COLOR : 0;
    if (w_eq(an, "color")) return w_eq(tag, "font") ? H_COLOR : 0;
    if (w_eq(an, "size")) return w_eq(tag, "font") ? H_SIZE : 0;
    if (w_eq(an, "border")) return w_eq(tag, "table") || w_eq(tag, "img") ? H_BORDER : 0;
    if (w_eq(an, "cellpadding")) return w_eq(tag, "table") ? H_CELLPADDING : 0;
    if (w_eq(an, "cellspacing")) return w_eq(tag, "table") ? H_CELLSPACING : 0;
    if (w_eq(an, "nowrap")) return w_eq(tag, "td") || w_eq(tag, "th") ? H_NOWRAP : 0;
    if (w_eq(an, "hspace")) return hints_in(tag, hk_replaced) ? H_HSPACE : 0;
    if (w_eq(an, "vspace")) return hints_in(tag, hk_replaced) ? H_VSPACE : 0;
    return 0;
}

/* parser: keep one presentational attribute */
static void hints_attr(we_doc_t *d, int idx, const char *tag, const char *an, const char *av, int vk) {
    we_node_t *n = &d->nodes[idx];
    int kind = hints_kind(tag, an), k;
    if (!kind) return;
    if (vk > 255) vk = 255;
    for (k = 0; k < n->nhints; k++) if (n->hint_kind[k] == kind) return;   /* the first one wins */
    if (n->nhints >= WE_HINTS_MAX) return;   /* not reached: a table has 8 hint kinds, the most */
    if (pool_put(d, av, vk, &n->hint_off[n->nhints])) return;
    n->hint_len[n->nhints] = (uint8_t)vk;
    n->hint_kind[n->nhints++] = (uint8_t)kind;
}

/* the value of a hint of n, or 0 */
static const char *hints_get(const we_doc_t *d, const we_node_t *n, int kind, int *len) {
    int k;
    for (k = 0; k < n->nhints; k++)
        if (n->hint_kind[k] == kind) { *len = n->hint_len[k]; return d->pool + n->hint_off[k]; }
    return 0;
}

/* HTML dimension: digits [. digits] [%], the rest ignored. Writes "Npx"
 * or "N%" into out; returns the integer part or -1. zero_ok allows 0. */
static int hints_dim(const char *v, int n, char *out, int zero_ok) {
    int i = 0, val = 0, dig = 0, at;
    while (i < n && w_space(v[i])) i++;
    while (i < n && v[i] >= '0' && v[i] <= '9') { if (val < 100000) val = val * 10 + v[i] - '0'; i++; dig++; }
    if (!dig || (!val && !zero_ok) || val > WE_LAYOUT_MAX) return -1;
    while (i < n && v[i] == '.') { i++; while (i < n && v[i] >= '0' && v[i] <= '9') i++; }
    w_itoa(val, out);
    at = w_len(out);
    if (i < n && v[i] == '%') { if (val > 1000) return -1; out[at++] = '%'; }
    else { out[at++] = 'p'; out[at++] = 'x'; }
    out[at] = 0;
    return val;
}

/* legacy colour: 3 or 6 hex digits without '#' get one */
static int hints_color(const char *v, int n, char *out, int cap) {
    int a = 0, b = n, k, hex = 1;
    while (a < b && w_space(v[a])) a++;
    while (b > a && w_space(v[b - 1])) b--;
    if (b - a < 1 || b - a + 2 > cap) return -1;
    for (k = a; k < b; k++)
        if (!((v[k] >= '0' && v[k] <= '9') || (w_low(v[k]) >= 'a' && w_low(v[k]) <= 'f'))) { hex = 0; break; }
    k = 0;
    if (hex && (b - a == 3 || b - a == 6)) out[k++] = '#';
    while (a < b) out[k++] = v[a++];
    out[k] = 0;
    return k;
}

static void hints_decl(we_doc_t *d, we_node_t *n, const char *prop, const char *value) {
    apply_decl(d, n, prop, w_len(prop), value, w_len(value));
}

static int hints_table_of(const we_doc_t *d, const we_node_t *cell) {
    int p = cell->parent, steps = 0;
    while (p > 0 && steps++ < 3) {
        if (w_eq(d->nodes[p].tag, "table")) return p;
        p = d->nodes[p].parent;
    }
    return -1;
}

static void hints_valign(we_doc_t *d, we_node_t *n, const char *v, int len) {
    static const char *const keys[] = { "top", "middle", "bottom", "baseline", 0 };
    int k;
    for (k = 0; keys[k]; k++) if (value_is(v, len, keys[k])) { hints_decl(d, n, "vertical-align", keys[k]); return; }
    diag(d, "hint refused", -1);
}

/* font size="N": 1..7, or +N / -N relative to 3 */
static int hints_font_px(const char *v, int len) {
    static const int px[] = { 10, 13, 16, 18, 24, 32, 48 };
    int i = 0, sign = 0, val = 0, dig = 0;
    while (i < len && w_space(v[i])) i++;
    if (i < len && (v[i] == '+' || v[i] == '-')) sign = v[i++] == '-' ? -1 : 1;
    while (i < len && v[i] >= '0' && v[i] <= '9') { if (val < 100) val = val * 10 + v[i] - '0'; i++; dig++; }
    if (!dig) return -1;
    if (sign) val = 3 + sign * val;
    if (val < 1) val = 1;
    if (val > 7) val = 7;
    return px[val - 1];
}

/* More of the HTML UA style sheet (HTML 15.3): lists of definitions,
 * figures, address, menu / dir, mark, sub / sup, abbr with a title. */
static void hints_ua(we_doc_t *d, we_node_t *n) {
    static const char *const blocks[] = { "dl", "dt", "dd", "figure", "figcaption", "address", "menu", "dir", "hgroup",
                                          "search", "listing", "xmp", "plaintext", 0 };
    const char *t = n->tag;
    if (!hints_in(t, blocks) && !w_eq(t, "mark") && !w_eq(t, "sub") && !w_eq(t, "sup") && !w_eq(t, "abbr")) return;
    if (hints_in(t, blocks)) n->display = WE_DISP_BLOCK;
    if (w_eq(t, "dl")) hints_decl(d, n, "margin", "8px 0");
    else if (w_eq(t, "dd")) hints_decl(d, n, "margin-left", "40px");
    else if (w_eq(t, "figure")) hints_decl(d, n, "margin", "8px 40px");
    else if (w_eq(t, "menu") || w_eq(t, "dir")) { n->list_style = WE_LIST_DISC; hints_decl(d, n, "margin", "8px 0"); hints_decl(d, n, "padding-left", "16px"); }
    else if (w_eq(t, "listing") || w_eq(t, "xmp") || w_eq(t, "plaintext")) hints_decl(d, n, "white-space", "pre");
    else if (w_eq(t, "mark")) { hints_decl(d, n, "background-color", "#ffff00"); hints_decl(d, n, "color", "#000000"); }
    else if (w_eq(t, "sub") || w_eq(t, "sup")) {
        hints_decl(d, n, "font-size", "smaller");
        hints_decl(d, n, "vertical-align", w_eq(t, "sub") ? "sub" : "super");
    } else if (w_eq(t, "abbr") && (n->attr_flags & 32)) hints_decl(d, n, "text-decoration", "underline dotted");
}

/* style pass: the hints of element n (after the UA styles) */
static void hints_apply(we_doc_t *d, we_node_t *n) {
    char buf[96];
    int k, len, table;
    const char *v;
    if (w_eq(n->tag, "center")) { n->display = WE_DISP_BLOCK; hints_decl(d, n, "text-align", "center"); }
    if (w_eq(n->tag, "caption")) { n->display = WE_DISP_BLOCK; hints_decl(d, n, "text-align", "center"); }   /* UA */
    hints_ua(d, n);
    for (k = 0; k < n->nhints; k++) {
        v = d->pool + n->hint_off[k];
        len = n->hint_len[k];
        switch (n->hint_kind[k]) {
        case H_HIDDEN: n->display = WE_DISP_NONE; break;
        case H_WIDTH: case H_HEIGHT:
            if (hints_dim(v, len, buf, 0) < 0) diag(d, "hint refused", -1);
            else hints_decl(d, n, n->hint_kind[k] == H_WIDTH ? "width" : "height", buf);
            break;
        case H_ALIGN:
            if (w_eq(n->tag, "table") || w_eq(n->tag, "hr")) {
                if (value_is(v, len, "center") || value_is(v, len, "middle")) {
                    hints_decl(d, n, "margin-left", "auto"); hints_decl(d, n, "margin-right", "auto");
                } else if (w_eq(n->tag, "table") && (value_is(v, len, "left") || value_is(v, len, "right"))) {
                    hints_decl(d, n, "float", value_is(v, len, "left") ? "left" : "right");
                } else if (w_eq(n->tag, "hr") && value_is(v, len, "left")) hints_decl(d, n, "margin-right", "auto");
                else if (w_eq(n->tag, "hr") && value_is(v, len, "right")) hints_decl(d, n, "margin-left", "auto");
                else diag(d, "hint refused", -1);
            } else if (hints_in(n->tag, hk_replaced)) {
                if (value_is(v, len, "left") || value_is(v, len, "right")) hints_decl(d, n, "float", value_is(v, len, "left") ? "left" : "right");
                else if (value_is(v, len, "absmiddle") || value_is(v, len, "center")) hints_decl(d, n, "vertical-align", "middle");
                else hints_valign(d, n, v, len);
            } else if (w_eq(n->tag, "caption") && (value_is(v, len, "top") || value_is(v, len, "bottom"))) {
                hints_decl(d, n, "caption-side", value_is(v, len, "top") ? "top" : "bottom");
            } else {
                if (value_is(v, len, "middle") || value_is(v, len, "center")) hints_decl(d, n, "text-align", "center");
                else if (value_is(v, len, "left") || value_is(v, len, "right") || value_is(v, len, "justify")) {
                    w_cpy(buf, sizeof(buf), v, len);
                    for (table = 0; buf[table]; table++) buf[table] = w_low(buf[table]);
                    hints_decl(d, n, "text-align", buf);
                } else diag(d, "hint refused", -1);
            }
            break;
        case H_VALIGN:
            if (!w_eq(n->tag, "tr") && !w_eq(n->tag, "col") && !table_group_tag(n->tag)) hints_valign(d, n, v, len);
            break;
        case H_BGCOLOR: case H_COLOR:
            if (hints_color(v, len, buf, (int)sizeof(buf)) < 0) diag(d, "hint refused", -1);
            else hints_decl(d, n, n->hint_kind[k] == H_BGCOLOR ? "background-color" : "color", buf);
            break;
        case H_SIZE: {
            int px = hints_font_px(v, len);
            if (px < 0) { diag(d, "hint refused", -1); break; }
            w_itoa(px, buf); w_cpy(buf + w_len(buf), 4, "px", 2);
            hints_decl(d, n, "font-size", buf);
            break;
        }
        case H_BORDER:
            if (hints_dim(v, len, buf, 1) < 0 || buf[w_len(buf) - 1] == '%') { diag(d, "hint refused", -1); break; }
            if (w_eq(n->tag, "table") && w_eq(buf, "0px")) break;
            hints_decl(d, n, "border-width", buf);
            hints_decl(d, n, "border-style", w_eq(n->tag, "table") ? "outset" : "solid");
            if (w_eq(n->tag, "table")) hints_decl(d, n, "border-color", "#808080");
            break;
        case H_CELLSPACING:
            if (hints_dim(v, len, buf, 1) < 0 || buf[w_len(buf) - 1] == '%') diag(d, "hint refused", -1);
            else hints_decl(d, n, "border-spacing", buf);
            break;
        case H_NOWRAP: hints_decl(d, n, "white-space", "nowrap"); break;
        case H_HSPACE: case H_VSPACE:
            if (hints_dim(v, len, buf, 1) < 0 || buf[w_len(buf) - 1] == '%') { diag(d, "hint refused", -1); break; }
            hints_decl(d, n, n->hint_kind[k] == H_HSPACE ? "margin-left" : "margin-top", buf);
            hints_decl(d, n, n->hint_kind[k] == H_HSPACE ? "margin-right" : "margin-bottom", buf);
            break;
        default: break;   /* cellpadding: read by the cells */
        }
    }
    /* cells: the table's border and cellpadding, the row / group valign */
    if (!w_eq(n->tag, "td") && !w_eq(n->tag, "th")) return;
    table = hints_table_of(d, n);
    if (table < 0) return;
    v = hints_get(d, &d->nodes[table], H_BORDER, &len);
    if (v && hints_dim(v, len, buf, 1) > 0) {
        hints_decl(d, n, "border-width", "1px");
        hints_decl(d, n, "border-style", "inset");
        hints_decl(d, n, "border-color", "#808080");
    }
    v = hints_get(d, &d->nodes[table], H_CELLPADDING, &len);
    if (v) {
        if (hints_dim(v, len, buf, 1) < 0 || buf[w_len(buf) - 1] == '%') diag(d, "hint refused", -1);
        else hints_decl(d, n, "padding", buf);
    }
    if (!hints_get(d, n, H_VALIGN, &len)) {
        int p = n->parent, steps;
        for (steps = 0; p > 0 && p != table && steps < 2; steps++, p = d->nodes[p].parent) {
            v = hints_get(d, &d->nodes[p], H_VALIGN, &len);
            if (v) { hints_valign(d, n, v, len); break; }
        }
    }
}
