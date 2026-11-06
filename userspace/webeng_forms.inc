/* webeng_forms.inc - Lot 73: HTML form controls, painted static (no
 * input, no submission: the engine has no event loop for pages). Included
 * by webeng.c only, the single engine.
 *
 * The parser keeps the attributes of input, button, select, option,
 * textarea, progress, meter and details (forms_attr). After the parse,
 * forms_build gives each control the text it shows as a text child:
 * an input its value (password: one '*' per byte) or its placeholder
 * (grey), a checked checkbox "x" and a checked radio "*", a submit or
 * reset input its value or "Submit" / "Reset"; a one-line select gets
 * " v" after its chosen option (the first selected, else the first); a
 * progress or meter bar gets a fill box; a details element without a
 * summary gets one ("Details") and every summary a "> " (closed) or
 * "v " (open) marker. The UA styles (forms_ua) come before the author
 * rules, so CSS can change them. Unknown input types show as text with
 * one diag "input type taken as text". */

/* WE_FORM_* control types: webeng.h */
/* lot 74: webeng_finput.inc */
static int finput_attr(we_doc_t *d, int idx, const char *an, const char *av, int vk);
static void finput_prepare(we_doc_t *d);
static void finput_caret(we_doc_t *d);
static int finput_range_value(const we_doc_t *d, int i);
static void finput_range_bounds(const we_node_t *n, int *min, int *max);
#define WE_FF2_FOCUS 4
#define WE_FF2_STALE 32
#define WE_FF2_CARET 64
#define WE_FF2_EMAIL 256
#define WE_FF2_NUMBER 512     /* lot 74: type=number */
#define WE_FF2_RANGE 1024     /* lot 74: type=range */
#define WE_FF2_URL 2048       /* lot 74: type=url */
#define WE_FF2_AUTOFOCUS 4096
#define WE_FF2_TABINDEX 8192
#define WE_FF2_PATTERN 16384  /* lot 74: pattern= present */
#define WE_FF2_DIALOG 32768  /* lot 74: dialog open toggle */
#define WE_FF2_LIST 65536    /* lot 74: input list= (datalist id) */
#define WE_FF_CHECKED 1
#define WE_FF_SELECTED 2
#define WE_FF_DISABLED 4
#define WE_FF_OPEN 8
#define WE_FF_MULTIPLE 16
#define WE_FF_CHOSEN 32
#define WE_FF_PLACEHOLDER 64
#define WE_FF_VALUE 128
#define WE_FORM_BORDER 0x767676
#define WE_FORM_FILL 0x3875D7
#define WE_FORM_TEXT_MAX 48

static int forms_tag(const char *t) {
    static const char *const tags[] = { "input", "button", "select", "option", "textarea", "progress", "meter", "details", "dialog", "datalist", "output",
                                         "form", "label", "fieldset", 0 };   /* lot 74: form, label, fieldset */
    int k;
    for (k = 0; tags[k]; k++) if (w_eq(t, tags[k])) return 1;
    return 0;
}

/* a decimal number to hundredths (-9999.99 .. 9999.99): 0, else -1 */
static int forms_fixed(const char *v, int len, int *out) {
    int i = 0, neg = 0, ip = 0, fp = 0, fd = 0, digits = 0;
    while (i < len && w_space(v[i])) i++;
    if (i < len && (v[i] == '-' || v[i] == '+')) neg = v[i++] == '-';
    while (i < len && v[i] >= '0' && v[i] <= '9') { ip = ip * 10 + v[i++] - '0'; digits++; if (ip > 9999) return -1; }
    if (i < len && v[i] == '.') {
        i++;
        while (i < len && v[i] >= '0' && v[i] <= '9') { if (fd < 2) { fp = fp * 10 + v[i] - '0'; fd++; } i++; digits++; }
    }
    while (i < len && w_space(v[i])) i++;
    if (!digits || i != len) return -1;
    if (fd == 1) fp *= 10;
    *out = (ip * 100 + fp) * (neg ? -1 : 1);
    return 0;
}

static int forms_input_type(we_doc_t *d, const char *v, int len, int idx) {
    static const char *const text[] = { "text", "search", "email", "url", "tel", "number", 0 };
    int k;
    for (k = 0; text[k]; k++) if (value_is(v, len, text[k])) return WE_FORM_TEXT;
    if (value_is(v, len, "range")) return WE_FORM_TEXT;   /* lot 74: WE_FF2_RANGE */
    if (value_is(v, len, "file")) return WE_FORM_FILE;   /* Lot 75 */
    if (value_is(v, len, "color")) return WE_FORM_COLOR; /* Lot 76 */
    if (value_is(v, len, "date")) return WE_FORM_DATE;   /* Lot 76 */
    if (value_is(v, len, "time") || value_is(v, len, "week") || value_is(v, len, "month") ||
        value_is(v, len, "datetime-local")) {
        diag(d, "input type taken as text", idx);
        return WE_FORM_TEXT;
    }
    if (value_is(v, len, "password")) return WE_FORM_PASSWORD;
    if (value_is(v, len, "checkbox")) return WE_FORM_CHECKBOX;
    if (value_is(v, len, "radio")) return WE_FORM_RADIO;
    if (value_is(v, len, "submit")) return WE_FORM_SUBMIT;
    if (value_is(v, len, "reset")) return WE_FORM_RESET;
    if (value_is(v, len, "button")) return WE_FORM_BUTTON;
    if (value_is(v, len, "hidden")) return WE_FORM_HIDDEN;
    diag(d, "input type taken as text", idx);
    return WE_FORM_TEXT;
}

/* one attribute of a form element (name an, value av of vk bytes) */
static void forms_attr(we_doc_t *d, int idx, const char *an, const char *av, int vk) {
    we_node_t *n = &d->nodes[idx];
    int v;
    if (!forms_tag(n->tag)) return;
    if (finput_attr(d, idx, an, av, vk)) return;   /* lot 74 */
    if (w_eq(an, "type") && w_eq(n->tag, "input") && value_is(av, vk, "email")) n->form_flags2 |= WE_FF2_EMAIL;
    if (w_eq(an, "type") && w_eq(n->tag, "input") && value_is(av, vk, "number")) n->form_flags2 |= WE_FF2_NUMBER;
    if (w_eq(an, "type") && w_eq(n->tag, "input") && value_is(av, vk, "range")) n->form_flags2 |= WE_FF2_RANGE;
    if (w_eq(an, "type") && w_eq(n->tag, "input") && value_is(av, vk, "url")) n->form_flags2 |= WE_FF2_URL;
    if (w_eq(an, "list") && w_eq(n->tag, "input")) {
        /* Lot 74: list= names a datalist; id in fpat_* so placeholder keeps fph */
        if (pool_put(d, av, vk, &n->flist_off) == 0) {
            n->flist_len = (uint16_t)vk;
            n->form_flags2 |= WE_FF2_LIST;
        }
        return;
    }
    if (w_eq(an, "type") && w_eq(n->tag, "input")) n->form_type = (uint8_t)forms_input_type(d, av, vk, idx);
    else if (w_eq(an, "type") && w_eq(n->tag, "button")) {
        if (value_is(av, vk, "reset")) n->form_type = WE_FORM_RESET;
        else if (value_is(av, vk, "button")) n->form_type = WE_FORM_BUTTON;
        else n->form_type = WE_FORM_SUBMIT;
    }
    else if (w_eq(an, "value")) {
        if (w_eq(n->tag, "progress") || w_eq(n->tag, "meter")) {
            if (forms_fixed(av, vk, &v) == 0) { n->meter[0] = v; n->form_flags |= WE_FF_VALUE; }
            else diag(d, "form number refused", idx);
        } else if (pool_put(d, av, vk, &n->fval_off) == 0) { n->fval_len = (uint16_t)vk; n->form_flags |= WE_FF_VALUE; }
    }
    else if (w_eq(an, "placeholder")) { if (pool_put(d, av, vk, &n->fph_off) == 0) n->fph_len = (uint16_t)vk; }
    else if (w_eq(an, "checked")) n->form_flags |= WE_FF_CHECKED;
    else if (w_eq(an, "selected")) n->form_flags |= WE_FF_SELECTED;
    else if (w_eq(an, "disabled")) n->form_flags |= WE_FF_DISABLED;
    else if (w_eq(an, "open")) n->form_flags |= WE_FF_OPEN;
    else if (w_eq(an, "multiple")) n->form_flags |= WE_FF_MULTIPLE;
    else if (w_eq(an, "size") || w_eq(an, "rows") || w_eq(an, "cols")) {
        int k = 0, ok = vk > 0;
        v = 0;
        for (k = 0; k < vk; k++) { if (av[k] < '0' || av[k] > '9' || v > 255) { ok = 0; break; } v = v * 10 + av[k] - '0'; }
        if (!ok || v < 1 || v > 255) { diag(d, "form size refused", idx); return; }
        if (w_eq(an, "rows")) n->form_rows = (uint8_t)v;
        else if (w_eq(an, "cols")) n->form_cols = (uint8_t)v;
        else n->form_size = (uint8_t)v;
    }
    else if (w_eq(an, "min") || w_eq(an, "max") || w_eq(an, "low") || w_eq(an, "high")) {
        int slot = w_eq(an, "min") ? 1 : w_eq(an, "max") ? 2 : w_eq(an, "low") ? 3 : 4;
        if (forms_fixed(av, vk, &v) == 0) { n->meter[slot] = v; n->meter_set |= (uint8_t)(1 << slot); }
        else diag(d, "form number refused", idx);
    }
}

/* a text child (first: linked before the other children) */
static int forms_text(we_doc_t *d, int parent, const char *s, int len, int first, int flags) {
    int t, old_first = d->nodes[parent].first, old_last = d->nodes[parent].last;
    if (len <= 0) return -1;
    if (len > WE_FORM_TEXT_MAX) len = WE_FORM_TEXT_MAX;
    t = new_node(d, parent);
    if (t < 0) { diag(d, "form budget", parent); return -1; }
    d->nodes[t].is_text = 1;
    d->nodes[t].form_flags = (uint8_t)flags;
    if (pool_put(d, s, len, &d->nodes[t].text_off) != 0) { diag(d, "form budget", parent); unlink_last(d, parent, old_last); return -1; }
    d->nodes[t].text_len = (uint16_t)len;
    if (first && old_first >= 0) {
        d->nodes[old_last].next = -1;
        d->nodes[parent].last = (int16_t)old_last;
        d->nodes[t].next = (int16_t)old_first;
        d->nodes[parent].first = (int16_t)t;
    }
    return t;
}

static int forms_listbox(const we_node_t *sel) {
    return (sel->form_flags & WE_FF_MULTIPLE) || sel->form_size > 1;
}

/* the option a one-line select shows: the first selected, else the first */
static void forms_choose(we_doc_t *d, int sel) {
    int c, first = -1, chosen = -1;
    int stack[WE_DEPTH_MAX], sp = 0;
    c = d->nodes[sel].first;
    while (c >= 0 || sp > 0) {
        if (c < 0) { c = d->nodes[stack[--sp]].next; continue; }
        if (w_eq(d->nodes[c].tag, "option")) {
            if (first < 0) first = c;
            if (chosen < 0 && (d->nodes[c].form_flags & WE_FF_SELECTED)) chosen = c;
        }
        if (w_eq(d->nodes[c].tag, "optgroup") && d->nodes[c].first >= 0 && sp < WE_DEPTH_MAX) { stack[sp++] = c; c = d->nodes[c].first; continue; }
        c = d->nodes[c].next;
    }
    if (chosen < 0) chosen = first;
    if (chosen >= 0) d->nodes[chosen].form_flags |= WE_FF_CHOSEN;
}

static void forms_bar(we_doc_t *d, int idx) {
    int f = new_node(d, idx);
    if (f < 0) { diag(d, "form budget", idx); return; }
    w_cpy(d->nodes[f].tag, 12, "::fill", 6);
}

static void forms_build(we_doc_t *d) {
    int i, count;
    finput_prepare(d);   /* lot 74: ordinals and the live state */
    count = (int)d->nnodes;
    for (i = 1; i < count; i++) {
        we_node_t *n = &d->nodes[i];
        if (n->is_text) continue;
        if (w_eq(n->tag, "input")) {
            char buf[WE_FORM_TEXT_MAX];
            int k, len;
            if (!n->form_type) n->form_type = WE_FORM_TEXT;
            switch (n->form_type) {
            case WE_FORM_TEXT:
            case WE_FORM_PASSWORD:
                if (n->form_flags2 & WE_FF2_RANGE) {   /* lot 74: a track and its fill */
                    n->meter[0] = finput_range_value(d, i);
                    forms_bar(d, i);
                    break;
                }
                if (n->fval_len) {
                    len = n->fval_len < WE_FORM_TEXT_MAX ? n->fval_len : WE_FORM_TEXT_MAX;
                    for (k = 0; k < len; k++) buf[k] = n->form_type == WE_FORM_PASSWORD ? '*' : d->pool[n->fval_off + k];
                    (void)forms_text(d, i, buf, len, 0, 0);
                } else if (n->fph_len) (void)forms_text(d, i, d->pool + n->fph_off, n->fph_len, 0, WE_FF_PLACEHOLDER);
                break;
            case WE_FORM_CHECKBOX:
                if (n->form_flags & WE_FF_CHECKED) (void)forms_text(d, i, "x", 1, 0, 0);
                break;
            case WE_FORM_RADIO:
                if (n->form_flags & WE_FF_CHECKED) (void)forms_text(d, i, "*", 1, 0, 0);
                break;
            case WE_FORM_SUBMIT:
            case WE_FORM_RESET:
            case WE_FORM_BUTTON:
                if (n->form_flags & WE_FF_VALUE) (void)forms_text(d, i, d->pool + n->fval_off, n->fval_len, 0, 0);
                else if (n->form_type == WE_FORM_SUBMIT) (void)forms_text(d, i, "Submit", 6, 0, 0);
                else if (n->form_type == WE_FORM_RESET) (void)forms_text(d, i, "Reset", 5, 0, 0);
                break;
            case WE_FORM_COLOR:
                /* Lot 76: show #rrggbb (default #000000) as the field text */
                if (n->fval_len) {
                    len = n->fval_len < WE_FORM_TEXT_MAX ? n->fval_len : WE_FORM_TEXT_MAX;
                    for (k = 0; k < len; k++) buf[k] = d->pool[n->fval_off + k];
                    (void)forms_text(d, i, buf, len, 0, 0);
                } else (void)forms_text(d, i, "#000000", 7, 0, WE_FF_PLACEHOLDER);
                break;
            case WE_FORM_DATE:
                /* Lot 76: show YYYY-MM-DD or a placeholder */
                if (n->fval_len) {
                    len = n->fval_len < WE_FORM_TEXT_MAX ? n->fval_len : WE_FORM_TEXT_MAX;
                    for (k = 0; k < len; k++) buf[k] = d->pool[n->fval_off + k];
                    (void)forms_text(d, i, buf, len, 0, 0);
                } else if (n->fph_len) (void)forms_text(d, i, d->pool + n->fph_off, n->fph_len, 0, WE_FF_PLACEHOLDER);
                else (void)forms_text(d, i, "yyyy-mm-dd", 10, 0, WE_FF_PLACEHOLDER);
                break;
            case WE_FORM_FILE:
                if (n->form_flags & WE_FF_VALUE) {
                    /* show the basename of the bound path */
                    int a = 0, b = n->fval_len;
                    while (a < b && d->pool[n->fval_off + a]) a++;
                    a = 0;
                    for (k = 0; k < n->fval_len; k++) if (d->pool[n->fval_off + k] == '/') a = k + 1;
                    len = n->fval_len - a;
                    if (len > WE_FORM_TEXT_MAX) len = WE_FORM_TEXT_MAX;
                    for (k = 0; k < len; k++) buf[k] = d->pool[n->fval_off + a + k];
                    (void)forms_text(d, i, buf, len, 0, 0);
                } else if (n->fph_len) (void)forms_text(d, i, d->pool + n->fph_off, n->fph_len, 0, WE_FF_PLACEHOLDER);
                else (void)forms_text(d, i, "Browse...", 9, 0, WE_FF_PLACEHOLDER);
                break;
            default: break;
            }
        } else if (w_eq(n->tag, "select")) {
            if (!forms_listbox(n)) { forms_choose(d, i); (void)forms_text(d, i, " v", 2, 0, 0); }
        } else if (w_eq(n->tag, "textarea")) {
            if (!d->nodes[i].has_text && n->fph_len) (void)forms_text(d, i, d->pool + n->fph_off, n->fph_len, 0, WE_FF_PLACEHOLDER);
        } else if (w_eq(n->tag, "progress") || w_eq(n->tag, "meter")) {
            forms_bar(d, i);
        } else if (w_eq(n->tag, "details")) {
            int first = n->first, s;
            while (first >= 0 && d->nodes[first].is_text) first = d->nodes[first].next;
            if (first < 0 || !w_eq(d->nodes[first].tag, "summary")) {
                int old_first = d->nodes[i].first, old_last = d->nodes[i].last;
                s = new_node(d, i);
                if (s < 0) { diag(d, "form budget", i); continue; }
                w_cpy(d->nodes[s].tag, 12, "summary", 7);
                if (old_first >= 0) {
                    d->nodes[old_last].next = -1;
                    d->nodes[i].last = (int16_t)old_last;
                    d->nodes[s].next = (int16_t)old_first;
                    d->nodes[i].first = (int16_t)s;
                }
                (void)forms_text(d, s, "Details", 7, 0, 0);
            } else s = first;
            d->nodes[s].form_flags |= WE_FF_CHOSEN;    /* the summary that shows */
            (void)forms_text(d, s, (d->nodes[i].form_flags & WE_FF_OPEN) ? "v " : "> ", 2, 1, 0);
        } else if (w_eq(n->tag, "output")) {
            /* Lot 76: form-associated readonly display; show value= or keep children */
            if ((n->form_flags & WE_FF_VALUE) && !n->has_text)
                (void)forms_text(d, i, d->pool + n->fval_off, n->fval_len, 0, 0);
        }
    }
    finput_caret(d);   /* lot 74 */
}

/* the bar fill in percent: value / max for progress (max 1 by default),
 * (value - min) / (max - min) for meter (0 and 1 by default; a max under
 * min is min); -1 for an indeterminate progress (no value) */
static int forms_fill_pct(const we_node_t *bar) {
    int min = 0, max = 100, v = bar->meter[0];
    if (w_eq(bar->tag, "input")) finput_range_bounds(bar, &min, &max);   /* lot 74: range */
    else if (w_eq(bar->tag, "progress")) {
        if (!(bar->form_flags & WE_FF_VALUE)) return -1;
        if ((bar->meter_set & 4) && bar->meter[2] > 0) max = bar->meter[2];
    } else {
        if (bar->meter_set & 2) min = bar->meter[1];
        if (bar->meter_set & 4) max = bar->meter[2];
        if (max < min) max = min;
    }
    if (max == min) return 0;
    if (v < min) v = min;
    if (v > max) v = max;
    return (int)(((long)(v - min) * 100 + (max - min) / 2) / (max - min));
}

/* meter colour: green inside [low, high], else yellow */
static uint32_t forms_meter_color(const we_node_t *m) {
    int v = m->meter[0];
    if ((m->meter_set & 8) && v < m->meter[3]) return 0xE6C200;
    if ((m->meter_set & 16) && v > m->meter[4]) return 0xE6C200;
    return 0x4CAF50;
}

/* Lot 74: re-apply accent-color / appearance after the cascade */
static void forms_accent_apply(we_doc_t *d) {
    int i;
    for (i = 1; i < (int)d->nnodes; i++) {
        we_node_t *n = &d->nodes[i];
        if (n->appearance) { n->border = 0; n->has_bg = 0; n->bg = 0; continue; }
        if (n->accent_set && n->border) n->border_color = n->accent;
        if (n->accent_set && w_eq(n->tag, "::fill")) n->bg = n->accent;
    }
}

static void forms_box(we_node_t *n, uint32_t bg) {
    /* Lot 74: appearance:none drops the UA chrome (caller still sizes). */
    n->display = WE_DISP_INLINE_BLOCK;
    n->border = 1;
    n->border_style = WE_BS_SOLID;
    n->border_color = WE_FORM_BORDER;
    n->bg = bg;
    n->has_bg = 1;
    n->nowrap = 1;
    n->overflow_hidden = 1;
    n->min_height = 10;   /* one line, even when empty */
}

static void forms_ua(we_doc_t *d, we_node_t *n);
/* lot 74: the focused control: its usual UA style, a blue border */
static void forms_ua_focus(we_doc_t *d, we_node_t *n) {
    n->form_flags2 &= ~WE_FF2_FOCUS;
    forms_ua(d, n);
    n->form_flags2 |= WE_FF2_FOCUS;
    if (n->border) n->border_color = (n->accent_set) ? n->accent : WE_FORM_FILL;
    if (n->appearance) { n->border = 0; n->bg = 0; n->has_bg = 0; }
}

/* UA styles of the form elements (after ua_defaults, before the rules) */
static void forms_ua(we_doc_t *d, we_node_t *n) {
    const we_node_t *p = n->parent >= 0 ? &d->nodes[n->parent] : 0;
    if (n->form_flags & WE_FF_DISABLED) n->color = 0x6D6D6D;
    if (n->form_flags2 & WE_FF2_FOCUS) { forms_ua_focus(d, n); return; }   /* lot 74 */
    if (w_eq(n->tag, "input")) {
        switch (n->form_type) {
        case WE_FORM_HIDDEN: n->display = WE_DISP_NONE; return;
        case WE_FORM_CHECKBOX: case WE_FORM_RADIO:
            forms_box(n, 0xFFFFFF);
            n->width = WE_CHAR_W; n->height = WE_CHAR_W + 2;
            n->margin[1] = n->margin[3] = 3;
            return;
        case WE_FORM_SUBMIT: case WE_FORM_RESET: case WE_FORM_BUTTON:
            forms_box(n, 0xEFEFEF);
            n->padding[0] = n->padding[2] = 1; n->padding[1] = n->padding[3] = 6;
            return;
        case WE_FORM_COLOR: {
            /* Lot 76: a swatch-sized field; bg from a valid #rrggbb value */
            uint32_t sw = 0x000000;
            int ok = 0;
            if (n->fval_len == 7 && d->pool[n->fval_off] == '#') {
                int h, v = 0;
                for (h = 1; h < 7; h++) {
                    char c = d->pool[n->fval_off + h];
                    int dgt = (c >= '0' && c <= '9') ? c - '0' : (c >= 'a' && c <= 'f') ? c - 'a' + 10 :
                              (c >= 'A' && c <= 'F') ? c - 'A' + 10 : -1;
                    if (dgt < 0) { v = -1; break; }
                    v = (v << 4) | dgt;
                }
                if (v >= 0) { sw = (uint32_t)v; ok = 1; }
            }
            forms_box(n, ok ? sw : 0xFFFFFF);
            n->padding[0] = n->padding[2] = 1; n->padding[1] = n->padding[3] = 2;
            n->width = (int16_t)(8 * WE_CHAR_W);
            if (ok && ((sw >> 16) + ((sw >> 8) & 0xFF) + (sw & 0xFF)) < 300) n->color = 0xFFFFFF;
            return;
        }
        case WE_FORM_DATE:
            forms_box(n, 0xFFFFFF);
            n->padding[0] = n->padding[2] = 1; n->padding[1] = n->padding[3] = 2;
            n->width = (int16_t)(12 * WE_CHAR_W);
            return;
        default:
            if (n->form_flags2 & WE_FF2_RANGE) {   /* lot 74: the track */
                forms_box(n, 0xE6E6E6);
                n->width = 80; n->height = 8; n->min_height = -1;
                return;
            }
            forms_box(n, 0xFFFFFF);
            n->padding[0] = n->padding[2] = 1; n->padding[1] = n->padding[3] = 2;
            n->width = (int16_t)((n->form_size ? n->form_size : 20) * WE_CHAR_W);
            return;
        }
    }
    if (w_eq(n->tag, "button")) {
        forms_box(n, 0xEFEFEF);
        n->padding[0] = n->padding[2] = 1; n->padding[1] = n->padding[3] = 6;
        return;
    }
    if (w_eq(n->tag, "select")) {
        forms_box(n, 0xFFFFFF);
        n->padding[0] = n->padding[2] = 1; n->padding[1] = n->padding[3] = 2;
        if (forms_listbox(n)) {
            int rows = n->form_size > 1 ? n->form_size : 4;
            n->height = (int16_t)(rows * 10);
            n->nowrap = 1;
        }
        return;
    }
    if (w_eq(n->tag, "option") || w_eq(n->tag, "optgroup")) {
        const we_node_t *sel = p;
        while (sel && !w_eq(sel->tag, "select") && sel->parent >= 0) sel = &d->nodes[sel->parent];
        if (!sel || !w_eq(sel->tag, "select")) return;
        if (!forms_listbox(sel)) {
            if (w_eq(n->tag, "option") && !(n->form_flags & WE_FF_CHOSEN)) n->display = WE_DISP_NONE;
            return;
        }
        n->display = WE_DISP_BLOCK;
        if (w_eq(n->tag, "option") && (n->form_flags & WE_FF_SELECTED)) { n->bg = WE_FORM_FILL; n->has_bg = 1; n->color = 0xFFFFFF; }
        return;
    }
    if (w_eq(n->tag, "textarea")) {
        forms_box(n, 0xFFFFFF);
        n->nowrap = 0;
        n->padding[0] = n->padding[1] = n->padding[2] = n->padding[3] = 2;
        n->width = (int16_t)((n->form_cols ? n->form_cols : 20) * WE_CHAR_W);
        n->height = (int16_t)((n->form_rows ? n->form_rows : 2) * 10);
        n->ws_mode = WE_WS_PRE_WRAP;
        return;
    }
    if (w_eq(n->tag, "progress") || w_eq(n->tag, "meter")) {
        forms_box(n, 0xE6E6E6);
        n->width = 80; n->height = 8; n->min_height = -1;
        return;
    }
    if (w_eq(n->tag, "::fill") && p) {
        int pct = forms_fill_pct(p);
        n->display = pct > 0 ? WE_DISP_BLOCK : WE_DISP_NONE;
        n->width_pct = (int16_t)pct;
        n->height = 8;
        n->bg = w_eq(p->tag, "meter") ? forms_meter_color(p) : (p->accent_set ? p->accent : WE_FORM_FILL);
        n->has_bg = 1;
        return;
    }
    if (p && (w_eq(p->tag, "progress") || w_eq(p->tag, "meter"))) { n->display = WE_DISP_NONE; return; }   /* fallback content */
    if (w_eq(n->tag, "dialog")) {
        /* Lot 74: a dialog without open is not shown; open makes a block. */
        n->display = (n->form_flags & WE_FF_OPEN) ? WE_DISP_BLOCK : WE_DISP_NONE;
        return;
    }
    if (w_eq(n->tag, "details") || w_eq(n->tag, "summary")) { n->display = WE_DISP_BLOCK; return; }
    if (w_eq(n->tag, "fieldset")) {
        n->display = WE_DISP_BLOCK;
        n->border = 2; n->border_style = 9; n->border_color = 0xC0C0C0;   /* groove */
        n->margin[1] = n->margin[3] = 2;
        n->padding[0] = 4; n->padding[2] = 8; n->padding[1] = n->padding[3] = 10;
        return;
    }
    if (w_eq(n->tag, "legend")) { n->display = WE_DISP_BLOCK; n->padding[1] = n->padding[3] = 2; return; }
    /* the children of a closed details, but its summary */
    if (p && w_eq(p->tag, "details") && !(p->form_flags & WE_FF_OPEN) && !(n->form_flags & WE_FF_CHOSEN))
        n->display = WE_DISP_NONE;
}

/* a text node: placeholder grey; hidden directly inside a closed details */
static void forms_text_style(we_doc_t *d, we_node_t *n) {
    const we_node_t *p = n->parent >= 0 ? &d->nodes[n->parent] : 0;
    if (n->form_flags & WE_FF_PLACEHOLDER) n->color = 0x757575;
    if (n->form_flags2 & WE_FF2_STALE) { n->display = WE_DISP_NONE; return; }   /* lot 74 */
    if (n->form_flags2 & WE_FF2_CARET) {
        int own = n->parent >= 0 ? n->parent : -1;
        if (own >= 0 && d->nodes[own].caret_set) n->color = d->nodes[own].caret;
        else n->color = WE_FORM_FILL;
    }
    if (p && w_eq(p->tag, "details") && !(p->form_flags & WE_FF_OPEN)) n->display = WE_DISP_NONE;
    if (p && (w_eq(p->tag, "progress") || w_eq(p->tag, "meter"))) n->display = WE_DISP_NONE;
}
