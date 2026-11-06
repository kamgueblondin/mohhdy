/* webeng_finput.inc - Lot 74: interactive form controls. Included by
 * webeng.c only, after webeng_forms.inc (the static painting of Lot 73).
 *
 * The live state of a page's controls is a we_forms_t the caller keeps
 * across renders (we_set_forms); we_parse keeps the pointer. Control k
 * (form_ord = k + 1) is the k-th input, select, textarea, button or
 * details in document order, at most WE_FSTATE_MAX. Before forms_build
 * paints the controls, finput_prepare writes the state of every entry
 * with set = 1 over the markup (value, checked, chosen option, open), so
 * the painting, :checked and :placeholder-shown follow the state with no
 * second code path. The focused control gets the WE_FF2_FOCUS flag (a
 * blue border, :focus, :focus-within) and a caret "_" after its text.
 *
 * we_form_click (a page point) and we_form_key (a key) change the state:
 * focus, typing (maxlength, readonly), backspace, Tab / Shift-Tab, ESC,
 * checkbox toggle, radio group (same name in the same form), select
 * (next option on a click, Up / Down keys, list box option click and
 * toggle when multiple), details open / close (a summary click), reset
 * (the form's entries go back to the markup), submit (a submit button or
 * Enter in a text field: implicit submission with the first submit button
 * of the form). A submission first checks the constraints (required,
 * type=email): an invalid form is not sent, its first invalid control
 * gets the focus (novalidate / formnovalidate skip the check).
 * we_form_request builds the request: application/x-www-form-urlencoded
 * (Lot 74) or multipart/form-data with a generated boundary and file
 * parts read through the local VFS reader (Lot 75).
 * pairs of the successful controls in tree order (HTML 4.10.21), GET
 * puts them as the query of the action (its old query and fragment
 * dropped), POST as the body; Lot 75: multipart/form-data builds a real
 * reported. Values are bytes as the page has them (no charset change). */

/* WE_FF2_FOCUS 4, STALE 32 (a textarea text replaced by the state), CARET
 * 64 (the caret text), EMAIL 256 (type=email): webeng_forms.inc */
#define WE_FF2_POST 1
#define WE_FF2_MULTIPART 2
#define WE_FF2_READONLY 8
#define WE_FF2_REQUIRED 16
#define WE_FF2_NOVALIDATE 128 /* form novalidate, submitter formnovalidate */
#define WE_FF2_FORM_ATTR 131072  /* Lot 75: form= id association */
#define WE_FF2_FORMACTION 262144 /* Lot 75: formaction= on a submitter */
#define WE_FF2_AC_OFF 524288       /* Lot 76: autocomplete=off */
#define WE_FF2_FORMTARGET 1048576  /* Lot 76: formtarget= present */
#define WE_FF2_TARGET_BLANK 2097152 /* Lot 76: formtarget=_blank */

/* attributes of Lot 74 on input, select, textarea, button, form, label:
 * 1 when taken */
static int finput_attr(we_doc_t *d, int idx, const char *an, const char *av, int vk) {
    we_node_t *n = &d->nodes[idx];
    if (w_eq(an, "name")) { if (pool_put(d, av, vk, &n->fname_off) == 0) n->fname_len = (uint16_t)vk; return 1; }
    if (w_eq(an, "readonly")) { n->form_flags2 |= WE_FF2_READONLY; return 1; }
    if (w_eq(an, "required")) { n->form_flags2 |= WE_FF2_REQUIRED; return 1; }
    if (w_eq(an, "pattern")) {
        if (pool_put(d, av, vk, &n->fpat_off) == 0) { n->fpat_len = (uint16_t)vk; n->form_flags2 |= WE_FF2_PATTERN; }
        return 1;
    }
    if (w_eq(an, "novalidate") || w_eq(an, "formnovalid")   /* formnovalidate, cut to 11 bytes */) { n->form_flags2 |= WE_FF2_NOVALIDATE; return 1; }
    if (w_eq(an, "autofocus")) { n->form_flags2 |= WE_FF2_AUTOFOCUS; return 1; }
    if (w_eq(an, "formmethod")) {
        if (value_is(av, vk, "dialog")) n->form_flags2 |= WE_FF2_DIALOG;
        else if (value_is(av, vk, "post")) n->form_flags2 |= WE_FF2_POST;
        return 1;
    }
    /* Lot 75: associate a control with a form by id (not a descendant) */
    if (w_eq(an, "form")) {
        if (vk > 0 && pool_put(d, av, vk, &n->fform_off) == 0) {
            n->fform_len = (uint16_t)vk;
            n->form_flags2 |= WE_FF2_FORM_ATTR;
        }
        return 1;
    }
    /* Lot 75: submitter overrides the form action URL */
    if (w_eq(an, "formaction")) {
        if (vk > 0 && pool_put(d, av, vk, &n->flist_off) == 0) {
            n->flist_len = (uint16_t)vk;
            n->form_flags2 |= WE_FF2_FORMACTION;
        } else diag(d, "formaction refused", -1);
        return 1;
    }
    /* Lot 76: formtarget=_self|_blank (recorded); other tokens refused */
    if (w_eq(an, "formtarget") || (w_eq(an, "target") && w_eq(n->tag, "form"))) {
        if (vk > 0 && pool_put(d, av, vk, &n->ftarget_off) == 0) {
            n->ftarget_len = (uint16_t)vk;
            n->form_flags2 |= WE_FF2_FORMTARGET;
            if (value_is(av, vk, "_blank")) n->form_flags2 |= WE_FF2_TARGET_BLANK;
            else if (!value_is(av, vk, "_self") && !value_is(av, vk, "_parent") &&
                     !value_is(av, vk, "_top") && av[0] != '_') {
                /* named browsing context: keep the token, not blank */
            } else if (value_is(av, vk, "_parent") || value_is(av, vk, "_top")) {
                /* treated as _self: no extra windows in the engine */
            }
        } else diag(d, "formtarget refused", -1);
        return 1;
    }
    /* Lot 76: autocomplete=; off sets WE_FF2_AC_OFF */
    if (w_eq(an, "autocomplete")) {
        if (vk > 0 && pool_put(d, av, vk, &n->fac_off) == 0) {
            n->fac_len = (uint16_t)vk;
            if (value_is(av, vk, "off") || value_is(av, vk, "false")) n->form_flags2 |= WE_FF2_AC_OFF;
        }
        return 1;
    }
    if (w_eq(an, "formenctype")) {
        if (value_is(av, vk, "multipart/form-data")) n->form_flags2 |= WE_FF2_MULTIPART;
        return 1;
    }
    if (w_eq(an, "tabindex")) {
        int k = 0, v = 0, neg = vk > 0 && av[0] == '-';
        for (k = neg; k < vk; k++) {
            if (av[k] < '0' || av[k] > '9') { diag(d, "form tabindex refused", idx); return 1; }
            if (v < 1000) v = v * 10 + av[k] - '0';
        }
        if (vk == neg) { diag(d, "form tabindex refused", idx); return 1; }
        n->tabindex = (int8_t)(neg ? -1 : v > 127 ? 127 : v);
        n->form_flags2 |= WE_FF2_TABINDEX;
        return 1;
    }
    if (w_eq(an, "step")) {
        int v;
        if (value_is(av, vk, "any")) { n->meter_set |= 32; return 1; }
        if (forms_fixed(av, vk, &v) == 0 && v > 0) { n->meter[3] = v; n->meter_set |= 8; }
        else diag(d, "form number refused", idx);
        return 1;
    }
    if (w_eq(an, "minlength")) {
        int k, v = 0;
        for (k = 0; k < vk; k++) {
            if (av[k] < '0' || av[k] > '9' || v > 255) { diag(d, "form size refused", idx); return 1; }
            v = v * 10 + av[k] - '0';
        }
        if (vk == 0 || v > 255) { diag(d, "form size refused", idx); return 1; }
        n->form_minlen = (uint8_t)v;
        return 1;
    }
    if (w_eq(an, "maxlength")) {
        int k, v = 0;
        for (k = 0; k < vk; k++) {
            if (av[k] < '0' || av[k] > '9' || v > 255) { diag(d, "form size refused", idx); return 1; }
            v = v * 10 + av[k] - '0';
        }
        if (vk == 0 || v > 255) { diag(d, "form size refused", idx); return 1; }
        n->form_maxlen = (uint8_t)(v ? v : 0);
        if (v == 0) n->form_flags2 |= WE_FF2_READONLY;   /* maxlength=0: nothing can be typed */
        return 1;
    }
    if (w_eq(n->tag, "form")) {
        if (w_eq(an, "action")) { if (pool_put(d, av, vk, &n->fval_off) == 0) n->fval_len = (uint16_t)vk; return 1; }
        if (w_eq(an, "method")) {
            if (value_is(av, vk, "post")) n->form_flags2 |= WE_FF2_POST;
            else if (value_is(av, vk, "dialog")) n->form_flags2 |= WE_FF2_DIALOG;
            return 1;
        }
        if (w_eq(an, "enctype")) { if (value_is(av, vk, "multipart/form-data")) n->form_flags2 |= WE_FF2_MULTIPART; return 1; }
        return 1;
    }
    if (w_eq(n->tag, "label")) {
        if (w_eq(an, "for")) { if (pool_put(d, av, vk, &n->fval_off) == 0) n->fval_len = (uint16_t)vk; }
        return 1;
    }
    return 0;
}

static int finput_kind(const we_node_t *n) {
    return w_eq(n->tag, "input") || w_eq(n->tag, "select") || w_eq(n->tag, "textarea") ||
           w_eq(n->tag, "button") || w_eq(n->tag, "details") || w_eq(n->tag, "dialog") ||
           w_eq(n->tag, "output");   /* Lot 76 */
}

static int finput_node(const we_doc_t *d, int ctl) {
    int i;
    if (ctl < 0) return -1;
    for (i = 1; i < (int)d->nnodes; i++) if (d->nodes[i].form_ord == ctl + 1) return i;
    return -1;
}

static int finput_textlike(const we_node_t *n) {
    return w_eq(n->tag, "textarea") ||
           (w_eq(n->tag, "input") && !(n->form_flags2 & WE_FF2_RANGE) &&
            (n->form_type == WE_FORM_TEXT || n->form_type == WE_FORM_PASSWORD ||
             n->form_type == WE_FORM_COLOR || n->form_type == WE_FORM_DATE || !n->form_type));
}

/* ---------- Lot 74: number and range (hundredths, forms_fixed) ---------- */

static int finput_is_range(const we_node_t *n) {
    return w_eq(n->tag, "input") && (n->form_flags2 & WE_FF2_RANGE);
}

/* range: min 0 and max 100 by default, a max under min is min */
static void finput_range_bounds(const we_node_t *n, int *min, int *max) {
    *min = (n->meter_set & 2) ? n->meter[1] : 0;
    *max = (n->meter_set & 4) ? n->meter[2] : 10000;
    if (*max < *min) *max = *min;
}

/* the step in hundredths, 0 for step=any */
static int finput_step(const we_node_t *n) {
    if (n->meter_set & 32) return 0;
    return (n->meter_set & 8) ? n->meter[3] : 100;
}

/* a value in hundredths as text: "12", "-3.5", "0.25" */
static int finput_fmt(int v, char *out, int max) {
    char t[16];
    int n = 0, k = 0, ip, fp, neg = v < 0;
    if (neg) v = -v;
    ip = v / 100; fp = v % 100;
    if (neg) t[n++] = '-';
    { char r[8]; int m = 0; do { r[m++] = (char)('0' + ip % 10); ip /= 10; } while (ip && m < 7); while (m) t[n++] = r[--m]; }
    if (fp) { t[n++] = '.'; t[n++] = (char)('0' + fp / 10); if (fp % 10) t[n++] = (char)('0' + fp % 10); }
    if (n >= max) return -1;
    for (k = 0; k < n; k++) out[k] = t[k];
    out[n] = 0;
    return n;
}

/* v clamped to [min, max] and on the step from min (the nearest, ties up;
 * a step past max goes one back) */
static int finput_snap(int v, int min, int max, int step) {
    if (v < min) v = min;
    if (v > max) v = max;
    if (step > 0) {
        int k = (v - min + step / 2) / step;
        v = min + k * step;
        if (v > max) v = min + (k - 1) * step;
    }
    return v;
}

/* the value of range i: its state or markup value when it is a number,
 * else the middle; always in bounds and on the step */
static int finput_range_value(const we_doc_t *d, int i) {
    const we_node_t *n = &d->nodes[i];
    const we_fstate_t *s = d->forms && n->form_ord ? &d->forms->c[n->form_ord - 1] : 0;
    int min, max, v;
    finput_range_bounds(n, &min, &max);
    if (s && s->set && s->len && forms_fixed(s->val, s->len, &v) == 0) {}
    else if ((!s || !s->set) && (n->form_flags & WE_FF_VALUE) && forms_fixed(d->pool + n->fval_off, n->fval_len, &v) == 0) {}
    else v = min + (max - min) / 2;
    return finput_snap(v, min, max, finput_step(n));
}

/* a number field: 1 when the text is not a number or is out of range
 * or off the step (from min, else 0) */
static int finput_number_bad(const we_node_t *n, const char *v, int len) {
    int x, step = finput_step(n), base = (n->meter_set & 2) ? n->meter[1] : 0;
    if (forms_fixed(v, len, &x) != 0) return 1;
    if ((n->meter_set & 2) && x < n->meter[1]) return 1;
    if ((n->meter_set & 4) && x > n->meter[2]) return 1;
    if (step > 0 && ((x - base) % step) != 0) return 1;
    return 0;
}

/* a url field: a scheme ("letter (letter | digit | + - .)* :") and more */
static int finput_url_bad(const char *v, int len) {
    int k = 1;
    if (len < 3 || !((v[0] >= 'a' && v[0] <= 'z') || (v[0] >= 'A' && v[0] <= 'Z'))) return 1;
    while (k < len && (w_namec(v[k]) || v[k] == '+' || v[k] == '.') && v[k] != '_') k++;
    return k >= len - 1 || v[k] != ':';
}

static int finput_submit_kind(const we_node_t *n) {
    return (w_eq(n->tag, "button") && (n->form_type == WE_FORM_SUBMIT || !n->form_type)) ||
           (w_eq(n->tag, "input") && n->form_type == WE_FORM_SUBMIT);
}

/* the form a node belongs to: form= id when set, else nearest form ancestor */
static int finput_owner(const we_doc_t *d, int i) {
    int p, j, k;
    const we_node_t *n = &d->nodes[i];
    if (n->form_flags2 & WE_FF2_FORM_ATTR) {
        for (j = 1; j < (int)d->nnodes; j++) {
            const we_node_t *f = &d->nodes[j];
            if (!w_eq(f->tag, "form") || !f->id[0]) continue;
            if ((int)w_len(f->id) != n->fform_len) continue;
            for (k = 0; k < n->fform_len && f->id[k] == d->pool[n->fform_off + k]; k++) {}
            if (k == n->fform_len) return j;
        }
        return -1;   /* form= names a missing id: the control is unbound */
    }
    p = n->parent;
    while (p >= 0) { if (w_eq(d->nodes[p].tag, "form")) return p; p = d->nodes[p].parent; }
    return -1;
}

static int finput_disabled(const we_doc_t *d, int i) {
    int p;
    if (d->nodes[i].form_flags & WE_FF_DISABLED) return 1;
    for (p = d->nodes[i].parent; p >= 0; p = d->nodes[p].parent)
        if (w_eq(d->nodes[p].tag, "fieldset") && (d->nodes[p].form_flags & WE_FF_DISABLED)) return 1;
    return 0;
}

/* option k of a select (tree order, optgroups included), its node */
static int finput_option(const we_doc_t *d, int sel, int want, int *count) {
    int stack[WE_DEPTH_MAX], sp = 0, c = d->nodes[sel].first, k = 0, hit = -1;
    while (c >= 0 || sp > 0) {
        if (c < 0) { c = d->nodes[stack[--sp]].next; continue; }
        if (w_eq(d->nodes[c].tag, "option")) { if (k == want) hit = c; k++; }
        if (w_eq(d->nodes[c].tag, "optgroup") && d->nodes[c].first >= 0 && sp < WE_DEPTH_MAX) { stack[sp++] = c; c = d->nodes[c].first; continue; }
        c = d->nodes[c].next;
    }
    if (count) *count = k;
    return hit;
}

/* the text of a node's text children (no generated, stale or caret text) */
static int finput_text_of(const we_doc_t *d, int i, char *out, int max) {
    int c, o = 0, k;
    for (c = d->nodes[i].first; c >= 0; c = d->nodes[c].next) {
        const we_node_t *t = &d->nodes[c];
        if (!t->is_text || (t->form_flags & WE_FF_PLACEHOLDER) || (t->form_flags2 & (WE_FF2_STALE | WE_FF2_CARET))) continue;
        for (k = 0; k < t->text_len && o < max - 1; k++) out[o++] = d->pool[t->text_off + k];
    }
    out[o] = 0;
    return o;
}

/* an option's value: its value attribute, else its text without the
 * leading and trailing spaces */
static int finput_option_value(const we_doc_t *d, int opt, char *out, int max) {
    const we_node_t *o = &d->nodes[opt];
    int n, a = 0, b;
    if (o->form_flags & WE_FF_VALUE) { n = o->fval_len < max - 1 ? o->fval_len : max - 1; w_cpy(out, max, d->pool + o->fval_off, n); return n; }
    n = finput_text_of(d, opt, out, max);
    while (a < n && w_space(out[a])) a++;
    b = n;
    while (b > a && w_space(out[b - 1])) b--;
    for (n = 0; a + n < b; n++) out[n] = out[a + n];
    out[n] = 0;
    return n;
}

/* the current state of control node i, read from the (state-applied) doc */
static void finput_capture(const we_doc_t *d, int i, we_fstate_t *s) {
    const we_node_t *n = &d->nodes[i];
    int k, count;
    w_zero(s, sizeof(*s));
    s->set = 1;
    if (w_eq(n->tag, "details") || w_eq(n->tag, "dialog")) { s->checked = (n->form_flags & WE_FF_OPEN) ? 1 : 0; return; }
    if (w_eq(n->tag, "select")) {
        (void)finput_option(d, i, -1, &count);
        for (k = 0; k < count && k < 32; k++) {
            int o = finput_option(d, i, k, 0);
            if (forms_listbox(n) ? (d->nodes[o].form_flags & WE_FF_SELECTED) : (d->nodes[o].form_flags & WE_FF_CHOSEN)) {
                if (!(s->selmask)) s->sel = (uint8_t)k;
                s->selmask |= 1U << k;
            }
        }
        return;
    }
    if (w_eq(n->tag, "textarea")) { s->len = (uint8_t)finput_text_of(d, i, s->val, WE_FSTATE_VAL); return; }
    s->checked = (n->form_flags & WE_FF_CHECKED) ? 1 : 0;
    if (n->form_flags & WE_FF_VALUE) {
        k = n->fval_len < WE_FSTATE_VAL - 1 ? n->fval_len : WE_FSTATE_VAL - 1;
        w_cpy(s->val, WE_FSTATE_VAL, d->pool + n->fval_off, k);
        s->len = (uint8_t)k;
    }
}

/* a text child with the state value (textarea), not cut at 48 bytes */
static void finput_textarea_text(we_doc_t *d, int i, const we_fstate_t *s) {
    int c, t, old_last = d->nodes[i].last;
    for (c = d->nodes[i].first; c >= 0; c = d->nodes[c].next)
        if (d->nodes[c].is_text) d->nodes[c].form_flags2 |= WE_FF2_STALE;
    d->nodes[i].has_text = s->len > 0;
    if (!s->len) return;
    t = new_node(d, i);
    if (t < 0) { diag(d, "form budget", i); return; }
    d->nodes[t].is_text = 1;
    if (pool_put(d, s->val, s->len, &d->nodes[t].text_off) != 0) { diag(d, "form budget", i); unlink_last(d, i, old_last); return; }
    d->nodes[t].text_len = s->len;
}

/* ordinals, then the state over the markup (before forms_build) */
static void finput_prepare(we_doc_t *d) {
    int i, ord = 0, count = (int)d->nnodes;
    for (i = 1; i < count; i++) {
        we_node_t *n = &d->nodes[i];
        if (n->is_text || !finput_kind(n)) continue;
        if (ord >= WE_FSTATE_MAX) { if (ord == WE_FSTATE_MAX) diag(d, "form state full", i); ord++; continue; }
        n->form_ord = (uint8_t)(++ord);
    }
    d->nform_ctl = (uint16_t)(ord < WE_FSTATE_MAX ? ord : WE_FSTATE_MAX);
    if (!d->forms) return;
    if (!d->forms->autofocused) {
        /* lot 74: the first autofocus control takes the focus once */
        d->forms->autofocused = 1;
        for (i = 1; i < count && d->forms->focus < 0; i++) {
            const we_node_t *n = &d->nodes[i];
            if (n->form_ord && (n->form_flags2 & WE_FF2_AUTOFOCUS) && !finput_disabled(d, i) &&
                !(w_eq(n->tag, "input") && n->form_type == WE_FORM_HIDDEN))
                d->forms->focus = (int16_t)(n->form_ord - 1);
        }
    }
    for (i = 1; i < count; i++) {
        we_node_t *n = &d->nodes[i];
        const we_fstate_t *s;
        if (!n->form_ord) continue;
        if (d->forms->focus == n->form_ord - 1) n->form_flags2 |= WE_FF2_FOCUS;
        s = &d->forms->c[n->form_ord - 1];
        if (!s->set) continue;
        if (w_eq(n->tag, "details") || w_eq(n->tag, "dialog")) {
            n->form_flags = (uint8_t)(s->checked ? n->form_flags | WE_FF_OPEN : n->form_flags & ~WE_FF_OPEN);
        } else if (w_eq(n->tag, "select")) {
            int k, cnt, o;
            (void)finput_option(d, i, -1, &cnt);
            for (k = 0; k < cnt; k++) {
                o = finput_option(d, i, k, 0);
                if (k < 32 && (forms_listbox(n) ? (s->selmask >> k) & 1U : k == s->sel)) d->nodes[o].form_flags |= WE_FF_SELECTED;
                else d->nodes[o].form_flags &= (uint8_t)~WE_FF_SELECTED;
            }
        } else if (w_eq(n->tag, "textarea")) {
            finput_textarea_text(d, i, s);
        } else {
            n->form_flags = (uint8_t)(s->checked ? n->form_flags | WE_FF_CHECKED : n->form_flags & ~WE_FF_CHECKED);
            if (finput_textlike(n) || n->form_type == WE_FORM_FILE) {
                if (s->len == 0) { n->fval_len = 0; n->form_flags &= (uint8_t)~WE_FF_VALUE; }
                else if (pool_put(d, s->val, s->len, &n->fval_off) == 0) { n->fval_len = s->len; n->form_flags |= WE_FF_VALUE; }
            }
        }
    }
}

/* the caret after the focused text field's text (after forms_build) */
static void finput_caret(we_doc_t *d) {
    int i, t;
    if (!d->forms || d->forms->focus < 0) return;
    i = finput_node(d, d->forms->focus);
    if (i < 0 || !finput_textlike(&d->nodes[i])) return;
    if (d->nodes[i].first >= 0 && (d->nodes[d->nodes[i].first].form_flags & WE_FF_PLACEHOLDER))
        d->nodes[d->nodes[i].first].form_flags2 |= WE_FF2_STALE;   /* typing replaces the placeholder */
    t = forms_text(d, i, "_", 1, 0, 0);
    if (t >= 0) d->nodes[t].form_flags2 |= WE_FF2_CARET;
}

/* ---------- state changes ---------- */

static we_fstate_t *finput_state(we_doc_t *d, int i) {
    we_fstate_t *s = &d->forms->c[d->nodes[i].form_ord - 1];
    if (!s->set) finput_capture(d, i, s);
    return s;
}

static int finput_focusable(const we_doc_t *d, int i) {
    const we_node_t *n = &d->nodes[i];
    if (!n->form_ord || finput_disabled(d, i)) return 0;
    if (w_eq(n->tag, "input") && n->form_type == WE_FORM_HIDDEN) return 0;
    if (w_eq(n->tag, "output")) return 0;   /* Lot 76: readonly display */
    if (!n->is_text && n->display == WE_DISP_NONE) return 0;
    return 1;
}

static int finput_set_focus(we_doc_t *d, int ctl) {
    if (d->forms->focus == ctl) return WE_FACT_NONE;
    d->forms->focus = (int16_t)ctl;
    return WE_FACT_CHANGED;
}

/* the value as submitted and checked (password bytes, not stars) */
static int finput_cur(const we_doc_t *d, int i, char *out, int max) {
    const we_node_t *n = &d->nodes[i];
    int k;
    if (w_eq(n->tag, "output")) {
        /* Lot 76: value= else concatenated text children */
        if (n->form_flags & WE_FF_VALUE) {
            k = n->fval_len < max - 1 ? n->fval_len : max - 1;
            w_cpy(out, max, d->pool + n->fval_off, k);
            return k;
        }
        return finput_text_of(d, i, out, max);
    }
    if (w_eq(n->tag, "textarea")) return finput_text_of(d, i, out, max);
    if (w_eq(n->tag, "select")) {
        int o = -1, cnt;
        (void)finput_option(d, i, -1, &cnt);
        for (k = 0; k < cnt; k++) {
            int c = finput_option(d, i, k, 0);
            if (d->nodes[c].form_flags & (forms_listbox(n) ? WE_FF_SELECTED : WE_FF_CHOSEN)) { o = c; break; }
        }
        if (o < 0) { out[0] = 0; return 0; }
        return finput_option_value(d, o, out, max);
    }
    if (finput_is_range(n)) { k = finput_fmt(finput_range_value(d, i), out, max); return k < 0 ? 0 : k; }
    if (n->form_type == WE_FORM_CHECKBOX || n->form_type == WE_FORM_RADIO) {
        if (!(n->form_flags & WE_FF_CHECKED)) { out[0] = 0; return 0; }
        if (!(n->form_flags & WE_FF_VALUE)) { w_cpy(out, max, "on", 2); return 2; }
    }
    if (!(n->form_flags & WE_FF_VALUE)) { out[0] = 0; return 0; }
    k = n->fval_len < max - 1 ? n->fval_len : max - 1;
    w_cpy(out, max, d->pool + n->fval_off, k);
    return k;
}

/* constraint validation of one control: 1 invalid (required and empty,
 * unchecked or no option chosen; type=email without '@' around text;
 * lot 74: shorter than minlength after an edit, a number that is not
 * one or is out of min / max / step, a url without a scheme) */

/* Lot 74: HTML pattern= as an anchored match; a small subset of JS regex:
 * literals, ., *, +, ?, character classes [abc] [^a], escapes \d \D \s \S
 * \w \W and \ of a meta, no groups and no | . Empty pattern matches "". */
static int finput_pat_escape(unsigned char c, const char **pp, const char *end) {
    if (*pp >= end) return -1;
    c = (unsigned char)**pp; (*pp)++;
    if (c == 'd') return 256;      /* digit */
    if (c == 'D') return 257;
    if (c == 's') return 258;      /* space */
    if (c == 'S') return 259;
    if (c == 'w') return 260;      /* word */
    if (c == 'W') return 261;
    return c;
}
static int finput_pat_class(unsigned char ch, const char *cls, int n, int neg) {
    int i, hit = 0;
    for (i = 0; i < n; i++) {
        if (cls[i] == '\\' && i + 1 < n) {
            const char *p = cls + i + 1; int k = finput_pat_escape(0, &p, cls + n);
            i = (int)(p - cls) - 1;
            if (k >= 256) {
                int ok = 0;
                if (k == 256) ok = ch >= '0' && ch <= '9';
                else if (k == 257) ok = !(ch >= '0' && ch <= '9');
                else if (k == 258) ok = ch == ' ' || ch == '\t' || ch == '\n' || ch == '\r';
                else if (k == 259) ok = !(ch == ' ' || ch == '\t' || ch == '\n' || ch == '\r');
                else if (k == 260) ok = (ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') || (ch >= '0' && ch <= '9') || ch == '_';
                else ok = !((ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') || (ch >= '0' && ch <= '9') || ch == '_');
                if (ok) hit = 1;
            } else if (ch == (unsigned char)k) hit = 1;
        } else if (i + 2 < n && cls[i + 1] == '-') {
            unsigned char a = (unsigned char)cls[i], b = (unsigned char)cls[i + 2];
            if (ch >= a && ch <= b) hit = 1;
            i += 2;
        } else if (ch == (unsigned char)cls[i]) hit = 1;
    }
    return neg ? !hit : hit;
}
static int finput_pat_atom(unsigned char ch, int kind, const char *cls, int clen) {
    if (kind == -1) return 1;                 /* . */
    if (kind >= 0 && kind < 256) return ch == (unsigned char)kind;
    if (kind == 256) return ch >= '0' && ch <= '9';
    if (kind == 257) return !(ch >= '0' && ch <= '9');
    if (kind == 258) return ch == ' ' || ch == '\t' || ch == '\n' || ch == '\r';
    if (kind == 259) return !(ch == ' ' || ch == '\t' || ch == '\n' || ch == '\r');
    if (kind == 260) return (ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') || (ch >= '0' && ch <= '9') || ch == '_';
    if (kind == 261) return !((ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') || (ch >= '0' && ch <= '9') || ch == '_');
    if (kind == 300) return finput_pat_class(ch, cls, clen, 0);
    if (kind == 301) return finput_pat_class(ch, cls, clen, 1);
    return 0;
}
/* recursive match of pattern[pi..] against text[ti..]; budget stops deep backtrack */
static int finput_pat_match(const char *pat, int plen, int pi, const char *text, int tlen, int ti, int *budget) {
    while (pi < plen) {
        int kind, clen = 0, quant = 0, next;
        const char *cls = 0;
        if (--*budget < 0) return 0;
        if (pat[pi] == '\\') {
            const char *p = pat + pi + 1; kind = finput_pat_escape(0, &p, pat + plen); pi = (int)(p - pat);
        } else if (pat[pi] == '.') { kind = -1; pi++; }
        else if (pat[pi] == '[') {
            int neg = 0, a = ++pi;
            if (pi < plen && pat[pi] == '^') { neg = 1; a = ++pi; }
            while (pi < plen && pat[pi] != ']') {
                if (pat[pi] == '\\' && pi + 1 < plen) pi += 2; else pi++;
            }
            cls = pat + a; clen = pi - a; kind = neg ? 301 : 300;
            if (pi < plen) pi++;
        } else { kind = (unsigned char)pat[pi++]; }
        if (pi < plen && (pat[pi] == '*' || pat[pi] == '+' || pat[pi] == '?')) quant = pat[pi++];
        else if (pi < plen && pat[pi] == '{') {
            int a = 0, b = -1, j = ++pi;
            while (j < plen && pat[j] >= '0' && pat[j] <= '9') a = a * 10 + pat[j++] - '0';
            if (j < plen && pat[j] == ',') {
                j++; b = 0;
                if (j < plen && pat[j] >= '0' && pat[j] <= '9') {
                    b = 0; while (j < plen && pat[j] >= '0' && pat[j] <= '9') b = b * 10 + pat[j++] - '0';
                } else b = 255;
            } else b = a;
            if (j < plen && pat[j] == '}') { pi = j + 1; quant = 4000 + a * 256 + (b & 255); }
        }
        if (quant == 0) {
            if (ti >= tlen || !finput_pat_atom((unsigned char)text[ti], kind, cls, clen)) return 0;
            ti++; continue;
        }
        if (quant == '?') {
            if (ti < tlen && finput_pat_atom((unsigned char)text[ti], kind, cls, clen) &&
                finput_pat_match(pat, plen, pi, text, tlen, ti + 1, budget)) return 1;
            continue;
        }
        /* *, +, and {n,m}: greedy with backtrack */
        {
            int n = 0, min, maxn = tlen - ti + 1, k, maxc;
            if (quant == '*') { min = 0; maxc = maxn; }
            else if (quant == '+') { min = 1; maxc = maxn; }
            else { min = (quant - 4000) / 256; maxc = (quant - 4000) & 255; if (maxc < min) maxc = min; }
            while (n < maxn - 1 && n < maxc && ti + n < tlen && finput_pat_atom((unsigned char)text[ti + n], kind, cls, clen)) n++;
            for (k = n; k >= min; k--)
                if (finput_pat_match(pat, plen, pi, text, tlen, ti + k, budget)) return 1;
            return 0;
        }
        (void)next;
    }
    return ti == tlen;
}
static int finput_pattern_bad(const we_doc_t *d, const we_node_t *n, const char *v, int len) {
    int budget = 4096;
    if (!(n->form_flags2 & WE_FF2_PATTERN) || !n->fpat_len) return 0;
    if (len == 0) return 0;   /* empty value: required alone refuses */
    return !finput_pat_match(d->pool + n->fpat_off, n->fpat_len, 0, v, len, 0, &budget);
}


static int finput_list_ok(const we_doc_t *d, int input, const char *v, int len);
/* Lot 76: #rrggbb only (exactly 7 bytes) */
static int finput_color_bad(const char *v, int len) {
    int k;
    if (len != 7 || v[0] != '#') return 1;
    for (k = 1; k < 7; k++) {
        char c = v[k];
        if (!((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F'))) return 1;
    }
    return 0;
}
/* Lot 76: YYYY-MM-DD with a real calendar day (no leap-second quirks) */
static int finput_date_bad(const char *v, int len) {
    int y, m, d, dim, k, leap;
    static const int mdays[] = { 0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
    if (len != 10 || v[4] != '-' || v[7] != '-') return 1;
    for (k = 0; k < 10; k++) {
        if (k == 4 || k == 7) continue;
        if (v[k] < '0' || v[k] > '9') return 1;
    }
    y = (v[0] - '0') * 1000 + (v[1] - '0') * 100 + (v[2] - '0') * 10 + (v[3] - '0');
    m = (v[5] - '0') * 10 + (v[6] - '0');
    d = (v[8] - '0') * 10 + (v[9] - '0');
    if (y < 1 || m < 1 || m > 12 || d < 1) return 1;
    leap = (y % 4 == 0 && (y % 100 != 0 || y % 400 == 0));
    dim = mdays[m] + (m == 2 && leap ? 1 : 0);
    return d > dim;
}
static int finput_invalid(const we_doc_t *d, int i) {
    const we_node_t *n = &d->nodes[i];
    char v[WE_FSTATE_VAL];
    int len, k, at = -1;
    if (!n->form_ord || finput_disabled(d, i) || (n->form_flags2 & WE_FF2_READONLY)) return 0;
    if (!(w_eq(n->tag, "input") || w_eq(n->tag, "textarea") || w_eq(n->tag, "select"))) return 0;
    if (w_eq(n->tag, "input") && (n->form_type == WE_FORM_HIDDEN || n->form_type == WE_FORM_SUBMIT ||
                                  n->form_type == WE_FORM_RESET || n->form_type == WE_FORM_BUTTON)) return 0;
    len = finput_cur(d, i, v, (int)sizeof(v));
    if ((n->form_flags2 & WE_FF2_REQUIRED) && len == 0 && !finput_is_range(n)) return 1;
    /* lot 74: tooShort after an edit, number, url */
    if (n->form_minlen && len > 0 && len < n->form_minlen && d->forms && d->forms->c[n->form_ord - 1].set) return 1;
    if ((n->form_flags2 & WE_FF2_NUMBER) && len > 0 && finput_number_bad(n, v, len)) return 1;
    if ((n->form_flags2 & WE_FF2_URL) && len > 0 && finput_url_bad(v, len)) return 1;
    if ((n->form_flags2 & WE_FF2_PATTERN) && len > 0 && finput_pattern_bad(d, n, v, len)) return 1;
    if ((n->form_flags2 & WE_FF2_LIST) && len > 0 && !finput_list_ok(d, i, v, len)) return 1;
    if ((n->form_flags2 & WE_FF2_EMAIL) && len > 0) {
        for (k = 0; k < len; k++) if (v[k] == '@') { if (at >= 0) return 1; at = k; }
        if (at <= 0 || at == len - 1) return 1;
    }
    /* Lot 76: color / date shape */
    if (n->form_type == WE_FORM_COLOR && len > 0 && finput_color_bad(v, len)) return 1;
    if (n->form_type == WE_FORM_DATE && len > 0 && finput_date_bad(v, len)) return 1;
    return 0;
}

/* the first invalid control of form f, or -1 */
static int finput_first_invalid(const we_doc_t *d, int f) {
    int i;
    for (i = 1; i < (int)d->nnodes; i++)
        if (d->nodes[i].form_ord && finput_owner(d, i) == f && finput_invalid(d, i)) return i;
    return -1;
}

/* a submission of form f by control sub (-1: implicit): checks, then
 * records it for we_form_request */
static int finput_submit(we_doc_t *d, int f, int sub) {
    int bad, p;
    if (f < 0) return WE_FACT_NONE;
    /* Lot 74: method=dialog on the form or the submitter closes the
     * enclosing dialog instead of building a request. */
    if ((d->nodes[f].form_flags2 & WE_FF2_DIALOG) ||
        (sub >= 0 && (d->nodes[sub].form_flags2 & WE_FF2_DIALOG))) {
        for (p = d->nodes[f].parent; p >= 0; p = d->nodes[p].parent)
            if (w_eq(d->nodes[p].tag, "dialog") && d->nodes[p].form_ord) {
                we_fstate_t *s = finput_state(d, p);
                s->checked = 0;
                d->forms->toggles++;
                return WE_FACT_CHANGED;
            }
        return WE_FACT_NONE;
    }
    if (!(d->nodes[f].form_flags2 & WE_FF2_NOVALIDATE) && !(sub >= 0 && (d->nodes[sub].form_flags2 & WE_FF2_NOVALIDATE))) {
        bad = finput_first_invalid(d, f);
        if (bad >= 0) {
            d->forms->refused++;
            d->forms->focus = (int16_t)(d->nodes[bad].form_ord - 1);
            return WE_FACT_CHANGED;
        }
    }
    d->forms->submit_form = (int16_t)f;
    d->forms->submitter = (int16_t)(sub >= 0 ? d->nodes[sub].form_ord - 1 : -1);
    d->forms->submits++;
    return WE_FACT_SUBMIT;
}

/* the first submit button of form f (implicit submission), or -1 */
static int finput_default_button(const we_doc_t *d, int f) {
    int i;
    for (i = 1; i < (int)d->nnodes; i++)
        if (d->nodes[i].form_ord && finput_submit_kind(&d->nodes[i]) && finput_owner(d, i) == f) return i;
    return -1;
}

static int finput_same(const char *a, const char *b, int n) {
    int k;
    for (k = 0; k < n; k++) if (a[k] != b[k]) return 0;
    return 1;
}

/* Lot 74: the datalist named by input list=, or -1 */
static int finput_datalist(const we_doc_t *d, int input) {
    const we_node_t *n = &d->nodes[input];
    char id[WE_TARGET_MAX];
    int dl, k;
    if (!(n->form_flags2 & WE_FF2_LIST) || !n->flist_len) return -1;
    k = n->flist_len < WE_TARGET_MAX - 1 ? n->flist_len : WE_TARGET_MAX - 1;
    w_cpy(id, WE_TARGET_MAX, d->pool + n->flist_off, k);
    dl = we_find_id(d, id);
    if (dl < 0 || !w_eq(d->nodes[dl].tag, "datalist")) return -1;
    return dl;
}

/* 1 when value matches an option of the list (or the list is empty/missing) */
static int finput_list_ok(const we_doc_t *d, int input, const char *v, int len) {
    int dl = finput_datalist(d, input), o, nopt = 0;
    if (dl < 0) return 1;
    for (o = d->nodes[dl].first; o >= 0; o = d->nodes[o].next) {
        const we_node_t *opt = &d->nodes[o];
        int ol;
        char ov[WE_FSTATE_VAL];
        if (!w_eq(opt->tag, "option")) continue;
        nopt++;
        ol = finput_option_value(d, o, ov, (int)sizeof(ov));
        if (ol == len && finput_same(ov, v, len)) return 1;
    }
    return nopt == 0;
}


static int finput_toggle_radio(we_doc_t *d, int i) {
    const we_node_t *n = &d->nodes[i];
    int j, f = finput_owner(d, i);
    we_fstate_t *s = finput_state(d, i);
    if (s->checked) return WE_FACT_NONE;
    for (j = 1; j < (int)d->nnodes; j++) {
        const we_node_t *m = &d->nodes[j];
        if (j == i || !m->form_ord || m->form_type != WE_FORM_RADIO || finput_owner(d, j) != f) continue;
        if (!n->fname_len || m->fname_len != n->fname_len || !finput_same(d->pool + m->fname_off, d->pool + n->fname_off, n->fname_len)) continue;
        finput_state(d, j)->checked = 0;
    }
    s->checked = 1;
    d->forms->toggles++;
    return WE_FACT_CHANGED;
}

/* activation of control node i (click, space): opt is the list box
 * option index under the pointer, or -1 */
static int finput_activate(we_doc_t *d, int i, int opt) {
    we_node_t *n = &d->nodes[i];
    we_fstate_t *s;
    int rc = finput_set_focus(d, n->form_ord - 1), cnt;
    if (w_eq(n->tag, "details") || w_eq(n->tag, "dialog")) {
        s = finput_state(d, i);
        s->checked = (uint8_t)!s->checked;
        d->forms->toggles++;
        return WE_FACT_CHANGED;
    }
    if (finput_textlike(n)) return rc;
    if (w_eq(n->tag, "select")) {
        s = finput_state(d, i);
        (void)finput_option(d, i, -1, &cnt);
        if (cnt == 0) return rc;
        if (forms_listbox(n)) {
            if (opt < 0 || opt >= cnt || opt >= 32) return rc;
            if (n->form_flags & WE_FF_MULTIPLE) s->selmask ^= 1U << opt;
            else s->selmask = 1U << opt;
            s->sel = (uint8_t)opt;
        } else {
            s->sel = (uint8_t)((s->sel + 1) % cnt);
            s->selmask = 1U << s->sel;
        }
        d->forms->toggles++;
        return WE_FACT_CHANGED;
    }
    switch (n->form_type) {
    case WE_FORM_CHECKBOX:
        s = finput_state(d, i);
        s->checked = (uint8_t)!s->checked;
        d->forms->toggles++;
        return WE_FACT_CHANGED;
    case WE_FORM_RADIO:
        return finput_toggle_radio(d, i) | rc;
    case WE_FORM_RESET: {
        int f = finput_owner(d, i), j;
        if (f < 0) return rc;
        for (j = 1; j < (int)d->nnodes; j++)
            if (d->nodes[j].form_ord && finput_owner(d, j) == f) d->forms->c[d->nodes[j].form_ord - 1].set = 0;
        d->forms->resets++;
        return WE_FACT_CHANGED;
    }
    case WE_FORM_BUTTON:
        return rc;
    default: break;
    }
    if (finput_submit_kind(n)) {
        int r = finput_submit(d, finput_owner(d, i), i);
        return r == WE_FACT_NONE ? rc : r;
    }
    return rc;
}

/* the control a label names: for=ID, else its first descendant control */
static int finput_label_target(const we_doc_t *d, int lab) {
    const we_node_t *n = &d->nodes[lab];
    int j, target = -1;
    if (n->fval_len) {
        char id[16];
        w_cpy(id, (int)sizeof(id), d->pool + n->fval_off, n->fval_len < 15 ? n->fval_len : 15);
        target = we_find_id(d, id);
        return target >= 0 && d->nodes[target].form_ord ? target : -1;
    }
    for (j = lab + 1; j < (int)d->nnodes; j++) {
        int p = j;
        while (p >= 0 && p != lab) p = d->nodes[p].parent;
        if (p == lab && d->nodes[j].form_ord) return j;
    }
    return -1;
}

/* the control node i belongs to: itself or an ancestor control, the
 * control of an enclosing label, the details of its summary; or -1 */
static int finput_target(const we_doc_t *d, int i) {
    int p;
    for (p = i; p >= 0; p = d->nodes[p].parent) {
        const we_node_t *n = &d->nodes[p];
        if (n->form_ord && !w_eq(n->tag, "details")) return p;
        if (w_eq(n->tag, "label")) return finput_label_target(d, p);
        if (w_eq(n->tag, "summary") && (n->form_flags & WE_FF_CHOSEN) && n->parent >= 0 && d->nodes[n->parent].form_ord)
            return n->parent;
    }
    return -1;
}

/* control under (x, y): the smallest laid-out box or text there that
 * belongs to a control; opt the list box option index there */
static int finput_hit(const we_doc_t *d, int x, int y, int *opt) {
    int i, best = -1, best_area = 0x7FFFFFFF;
    *opt = -1;
    for (i = 1; i < (int)d->nnodes; i++) {
        const we_node_t *n = &d->nodes[i];
        int area, target;
        if (n->w <= 0 || n->h <= 0 || n->display == WE_DISP_NONE || n->visibility) continue;
        if (x < n->x || y < n->y || x >= n->x + n->w || y >= n->y + n->h) continue;
        /* Lot 75: pointer-events:none skips this box and its targets */
        {
            int a = i;
            while (a >= 0 && !d->nodes[a].pointer_events) a = d->nodes[a].parent;
            if (a >= 0) continue;
        }
        target = finput_target(d, i);
        if (target < 0) continue;
        area = n->w * n->h;
        if (area < best_area) { best_area = area; best = target; }
    }
    if (best >= 0 && w_eq(d->nodes[best].tag, "select") && forms_listbox(&d->nodes[best])) {
        int k, cnt;
        (void)finput_option(d, best, -1, &cnt);
        for (k = 0; k < cnt; k++) {
            const we_node_t *o = &d->nodes[finput_option(d, best, k, 0)];
            if (o->h > 0 && y >= o->y && y < o->y + o->h) { *opt = k; break; }
        }
    }
    return best;
}

/* ---------- public ---------- */

void we_forms_init(we_forms_t *f) {
    if (!f) return;
    w_zero(f, sizeof(*f));
    f->focus = -1;
    f->submitter = -1;
    f->submit_form = -1;
}

void we_set_forms(we_doc_t *d, we_forms_t *f) { if (d) d->forms = f; }

int we_form_at(const we_doc_t *d, int x, int y) {
    int opt, i;
    if (!d) return -1;
    i = finput_hit(d, x, y, &opt);
    return i < 0 ? -1 : d->nodes[i].form_ord - 1;
}

int we_form_click(we_doc_t *d, int x, int y) {
    int opt, i;
    if (!d || !d->forms) return -1;
    i = finput_hit(d, x, y, &opt);
    if (i < 0) return -1;
    if (finput_disabled(d, i)) { d->forms->refused++; return WE_FACT_NONE; }
    if (finput_is_range(&d->nodes[i]) && !(d->nodes[i].form_flags2 & WE_FF2_READONLY)) {
        /* lot 74: the click position on the track sets the value */
        const we_node_t *n = &d->nodes[i];
        int min, max, w = n->w - 2 * n->border, at = x - n->x - n->border, v, rc;
        char t[16];
        we_fstate_t *s;
        finput_range_bounds(n, &min, &max);
        if (w < 1) w = 1;
        if (at < 0) at = 0;
        if (at > w) at = w;
        v = finput_snap(min + (int)(((long)(max - min) * at + w / 2) / w), min, max, finput_step(n));
        rc = finput_set_focus(d, n->form_ord - 1);
        if (v == finput_range_value(d, i)) return rc;
        s = finput_state(d, i);
        if (finput_fmt(v, t, (int)sizeof(t)) < 0) return rc;
        w_cpy(s->val, WE_FSTATE_VAL, t, w_len(t));
        s->len = (uint8_t)w_len(t);
        d->forms->edits++;
        return WE_FACT_CHANGED;
    }
    return finput_activate(d, i, opt);
}

/* next (dir 1) or previous (-1) focusable control after ctl */
/* lot 74: the sequential order: tabindex 1..127 first (ascending, then
 * tree order), then tabindex 0 or none; tabindex -1 is left out */
static int finput_tab_key(const we_doc_t *d, int ctl) {
    int i = finput_node(d, ctl), t;
    if (i < 0) return -1;
    t = (d->nodes[i].form_flags2 & WE_FF2_TABINDEX) ? d->nodes[i].tabindex : 0;
    if (t < 0) return -1;
    return (t > 0 ? t : 128) * WE_FSTATE_MAX + ctl;
}
static int finput_next(const we_doc_t *d, int ctl, int dir) {
    int c, best = -1, best_key = 0, wrap = -1, wrap_key = 0;
    int cur = ctl >= 0 ? finput_tab_key(d, ctl) : -1;
    if (ctl >= 0 && cur < 0) cur = 128 * WE_FSTATE_MAX + ctl;   /* a click on tabindex -1 */
    for (c = 0; c < d->nform_ctl; c++) {
        int i = finput_node(d, c), key = finput_tab_key(d, c);
        if (c == ctl || key < 0 || i < 0 || !finput_focusable(d, i)) continue;
        if (ctl < 0 || (dir > 0 ? key > cur : key < cur)) {
            if (best < 0 || (dir > 0 ? key < best_key : key > best_key)) { best = c; best_key = key; }
        }
        if (wrap < 0 || (dir > 0 ? key < wrap_key : key > wrap_key)) { wrap = c; wrap_key = key; }
    }
    return best >= 0 ? best : wrap;
}

int we_form_key(we_doc_t *d, int key) {
    int i, f;
    we_node_t *n;
    we_fstate_t *s;
    if (!d || !d->forms || d->nform_ctl == 0) return -1;
    if (key == WE_KEY_TAB || key == WE_KEY_SHIFT_TAB) {
        int c = finput_next(d, d->forms->focus, key == WE_KEY_TAB ? 1 : -1);
        return c < 0 ? -1 : finput_set_focus(d, c);
    }
    i = finput_node(d, d->forms->focus);
    if (i < 0) return -1;
    if (key == WE_KEY_ESC) return finput_set_focus(d, -1);
    n = &d->nodes[i];
    if (finput_disabled(d, i)) return WE_FACT_NONE;
    if (finput_is_range(n) || ((n->form_flags2 & WE_FF2_NUMBER) && (key == WE_KEY_UP || key == WE_KEY_DOWN))) {
        /* lot 74: a step up or down (range also left / right) */
        int up = key == WE_KEY_UP || key == WE_KEY_RIGHT, step = finput_step(n), v, nv;
        char t[16];
        if (!(key == WE_KEY_UP || key == WE_KEY_DOWN || key == WE_KEY_LEFT || key == WE_KEY_RIGHT)) {
            if (key == WE_KEY_ENTER) { f = finput_owner(d, i); return finput_submit(d, f, finput_default_button(d, f)); }
            return WE_FACT_NONE;
        }
        if (n->form_flags2 & WE_FF2_READONLY) { d->forms->refused++; return WE_FACT_NONE; }
        if (!step) step = 100;
        if (finput_is_range(n)) {
            int min, max;
            finput_range_bounds(n, &min, &max);
            v = finput_range_value(d, i);
            nv = finput_snap(v + (up ? step : -step), min, max, finput_step(n));
            if (up ? nv <= v : nv >= v) return WE_FACT_NONE;
        } else {
            char cur[WE_FSTATE_VAL];
            int len = finput_cur(d, i, cur, (int)sizeof(cur)), base = (n->meter_set & 2) ? n->meter[1] : 0;
            if (len == 0 || forms_fixed(cur, len, &v) != 0) v = (n->meter_set & 2) ? n->meter[1] : up ? step : -step;
            else {
                /* off the step: the next value on it in that direction */
                int off = finput_step(n) ? ((v - base) % step + step) % step : 0;   /* step=any: no grid */
                if (off) v -= off, v += up ? step : 0;
                else v += up ? step : -step;
            }
            if ((n->meter_set & 2) && v < n->meter[1]) v = n->meter[1];
            if ((n->meter_set & 4) && v > n->meter[2]) v = n->meter[2];
            if (v < -999999 || v > 999999) return WE_FACT_NONE;
            nv = v;
        }
        s = finput_state(d, i);
        if (finput_fmt(nv, t, (int)sizeof(t)) < 0) return WE_FACT_NONE;
        w_cpy(s->val, WE_FSTATE_VAL, t, w_len(t));
        s->len = (uint8_t)w_len(t);
        d->forms->edits++;
        return WE_FACT_CHANGED;
    }
    if (finput_textlike(n)) {
        int max = n->form_maxlen ? n->form_maxlen : WE_FSTATE_VAL - 1;
        int area = w_eq(n->tag, "textarea");
        if ((n->form_flags2 & WE_FF2_NUMBER) && key >= 32 && key <= 126 &&
            !((key >= '0' && key <= '9') || key == '-' || key == '+' || key == '.')) { d->forms->refused++; return WE_FACT_NONE; }
        /* Lot 76: color accepts # and hex; date accepts digits and '-' */
        if (n->form_type == WE_FORM_COLOR && key >= 32 && key <= 126 &&
            !((key >= '0' && key <= '9') || (key >= 'a' && key <= 'f') || (key >= 'A' && key <= 'F') || key == '#')) {
            d->forms->refused++; return WE_FACT_NONE;
        }
        if (n->form_type == WE_FORM_DATE && key >= 32 && key <= 126 &&
            !((key >= '0' && key <= '9') || key == '-')) {
            d->forms->refused++; return WE_FACT_NONE;
        }
        if (key == WE_KEY_ENTER && !area) return finput_submit(d, finput_owner(d, i), finput_default_button(d, finput_owner(d, i)));
        if (!(key == WE_KEY_BACKSPACE || key == WE_KEY_ENTER || (key >= 32 && key <= 126))) return WE_FACT_NONE;
        if (n->form_flags2 & WE_FF2_READONLY) { d->forms->refused++; return WE_FACT_NONE; }
        s = finput_state(d, i);
        if (key == WE_KEY_BACKSPACE) {
            if (!s->len) return WE_FACT_NONE;
            s->len--;
            s->val[s->len] = 0;
        } else {
            if (s->len >= max) { d->forms->refused++; return WE_FACT_NONE; }
            s->val[s->len++] = key == WE_KEY_ENTER ? '\n' : (char)key;
            s->val[s->len] = 0;
        }
        d->forms->edits++;
        return WE_FACT_CHANGED;
    }
    if (w_eq(n->tag, "select") && (key == WE_KEY_UP || key == WE_KEY_DOWN)) {
        int cnt;
        s = finput_state(d, i);
        (void)finput_option(d, i, -1, &cnt);
        if (key == WE_KEY_DOWN ? s->sel + 1 >= cnt : s->sel == 0) return WE_FACT_NONE;
        s->sel = (uint8_t)(s->sel + (key == WE_KEY_DOWN ? 1 : -1));
        s->selmask = 1U << s->sel;
        d->forms->toggles++;
        return WE_FACT_CHANGED;
    }
    if (key == ' ' || (key == WE_KEY_ENTER && (finput_submit_kind(n) || n->form_type == WE_FORM_RESET || w_eq(n->tag, "details"))))
        return finput_activate(d, i, -1);
    if (key == WE_KEY_ENTER && (n->form_type == WE_FORM_CHECKBOX || n->form_type == WE_FORM_RADIO)) {
        f = finput_owner(d, i);
        return finput_submit(d, f, finput_default_button(d, f));
    }
    return WE_FACT_NONE;
}

int we_form_set_text(we_doc_t *d, int ctl, const char *text) {
    int i, k, max;
    we_fstate_t *s;
    if (!d || !d->forms || !text) return -1;
    i = finput_node(d, ctl);
    if (i < 0 || !finput_textlike(&d->nodes[i]) || finput_disabled(d, i) || (d->nodes[i].form_flags2 & WE_FF2_READONLY)) return -1;
    max = d->nodes[i].form_maxlen ? d->nodes[i].form_maxlen : WE_FSTATE_VAL - 1;
    s = finput_state(d, i);
    for (k = 0; text[k] && k < max; k++) s->val[k] = text[k];
    if (text[k]) d->forms->refused++;
    s->len = (uint8_t)k;
    s->val[k] = 0;
    d->forms->edits++;
    return 0;
}

/* Lot 75: absolute local path for type=file; refuses URLs, ".." and
 * non-printable bytes. The path is kept in the control state; the bytes
 * are read at we_form_request time through the image/VFS reader. */
static int finput_path_ok(const char *path) {
    int i, start = 0, n = 0;
    if (!path || path[0] != '/' || path[1] == '/') return 0;
    for (i = 0; path[i]; i++) {
        char c = path[i];
        if ((unsigned char)c < 33 || (unsigned char)c > 126 || c == ':' || c == '\\' || c == '%' || c == '?' || c == '#') return 0;
        n++;
        if (n >= WE_FSTATE_VAL) return 0;
    }
    for (i = 1; i <= n; i++) {
        if (path[i] == '/' || path[i] == 0) {
            if (i - start == 3 && path[start + 1] == '.' && path[start + 2] == '.') return 0;
            start = i;
        }
    }
    return n > 0;
}

int we_form_set_file(we_doc_t *d, int ctl, const char *path) {
    int i, k;
    we_fstate_t *s;
    if (!d || !d->forms) return -1;
    i = finput_node(d, ctl);
    if (i < 0 || d->nodes[i].form_type != WE_FORM_FILE || finput_disabled(d, i)) return -1;
    if (!path || !path[0]) {
        s = finput_state(d, i);
        s->len = 0; s->val[0] = 0;
        d->forms->edits++;
        return 0;
    }
    if (!finput_path_ok(path)) { d->forms->refused++; return -1; }
    s = finput_state(d, i);
    for (k = 0; path[k] && k < WE_FSTATE_VAL - 1; k++) s->val[k] = path[k];
    s->val[k] = 0;
    s->len = (uint8_t)k;
    d->forms->edits++;
    return 0;
}

int we_form_value(const we_doc_t *d, int ctl, char *out, int max) {
    int i;
    if (!d || !out || max <= 0) return -1;
    out[0] = 0;
    i = finput_node(d, ctl);
    if (i < 0) return -1;
    return finput_cur(d, i, out, max);
}

/* x-www-form-urlencoded: unreserved bytes kept, space '+', the rest %XX;
 * 0, or -1 when the buffer is full */
static int finput_enc(char *out, int max, int *o, const char *s, int n, int crlf) {
    static const char hex[] = "0123456789ABCDEF";
    int k;
    for (k = 0; k < n; k++) {
        unsigned char c = (unsigned char)s[k];
        if (crlf && c == '\n' && (k == 0 || s[k - 1] != '\r')) {
            if (*o + 3 >= max) return -1;
            out[(*o)++] = '%'; out[(*o)++] = '0'; out[(*o)++] = 'D';
        }
        if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') ||
            c == '*' || c == '-' || c == '.' || c == '_') {
            if (*o + 1 >= max) return -1;
            out[(*o)++] = (char)c;
        } else if (c == ' ') {
            if (*o + 1 >= max) return -1;
            out[(*o)++] = '+';
        } else {
            if (*o + 3 >= max) return -1;
            out[(*o)++] = '%'; out[(*o)++] = hex[c >> 4]; out[(*o)++] = hex[c & 15];
        }
    }
    return 0;
}

static int finput_pair(char *out, int max, int *o, const we_doc_t *d, const we_node_t *n, const char *v, int vl, int crlf) {
    if (*o > 0) { if (*o + 1 >= max) return -1; out[(*o)++] = '&'; }
    if (finput_enc(out, max, o, d->pool + n->fname_off, n->fname_len, 0) != 0) return -1;
    if (*o + 1 >= max) return -1;
    out[(*o)++] = '=';
    return finput_enc(out, max, o, v, vl, crlf);
}

/* Lot 75: append raw bytes to the multipart body */
static int finput_mp_put(char *out, int max, int *o, const char *s, int n) {
    int k;
    if (*o + n >= max) return -1;
    for (k = 0; k < n; k++) out[(*o)++] = s[k];
    return 0;
}

static int finput_mp_str(char *out, int max, int *o, const char *s) {
    int n = 0;
    while (s[n]) n++;
    return finput_mp_put(out, max, o, s, n);
}

/* basename of a path kept in state */
static const char *finput_basename(const char *path) {
    const char *b = path;
    int i;
    for (i = 0; path[i]; i++) if (path[i] == '/') b = path + i + 1;
    return b[0] ? b : "file";
}

/* Lot 75: one multipart field (text or file). Returns 0, -1 refuse, -2 overflow */
static int finput_mp_field(const we_doc_t *d, we_form_req_t *req, int *o, const we_node_t *n,
                           const char *v, int vl, int is_file) {
    char line[160];
    int k = 0, name_len = n->fname_len;
    if (finput_mp_str(req->body, WE_FREQ_BODY, o, "--") != 0) return -2;
    if (finput_mp_str(req->body, WE_FREQ_BODY, o, req->boundary) != 0) return -2;
    if (finput_mp_str(req->body, WE_FREQ_BODY, o, "\r\n") != 0) return -2;
    /* Content-Disposition */
    {
        static const char pref[] = "Content-Disposition: form-data; name=\"";
        if (finput_mp_str(req->body, WE_FREQ_BODY, o, pref) != 0) return -2;
        if (finput_mp_put(req->body, WE_FREQ_BODY, o, d->pool + n->fname_off, name_len) != 0) return -2;
        if (finput_mp_str(req->body, WE_FREQ_BODY, o, "\"") != 0) return -2;
    }
    if (is_file) {
        const char *bn = (v && v[0]) ? finput_basename(v) : "";
        if (finput_mp_str(req->body, WE_FREQ_BODY, o, "; filename=\"") != 0) return -2;
        if (finput_mp_str(req->body, WE_FREQ_BODY, o, bn) != 0) return -2;
        if (finput_mp_str(req->body, WE_FREQ_BODY, o, "\"\r\nContent-Type: application/octet-stream\r\n\r\n") != 0) return -2;
        if (v && v[0]) {
            if (!image_read) return -1;
            {
                static unsigned char buf[WE_FFILE_MAX + 1];
                int got = image_read(image_ctx, v, buf, WE_FFILE_MAX + 1);
                if (got < 0) return -1;
                if (got > WE_FFILE_MAX) return -1;   /* too large: refused */
                if (finput_mp_put(req->body, WE_FREQ_BODY, o, (const char *)buf, got) != 0) return -2;
            }
        }
    } else {
        if (finput_mp_str(req->body, WE_FREQ_BODY, o, "\r\n\r\n") != 0) return -2;
        if (finput_mp_put(req->body, WE_FREQ_BODY, o, v, vl) != 0) return -2;
    }
    if (finput_mp_str(req->body, WE_FREQ_BODY, o, "\r\n") != 0) return -2;
    (void)line; (void)k;
    return 0;
}

/* Lot 75: a short boundary unique to this submission */
static void finput_boundary(char *out, int max, const we_doc_t *d) {
    static const char hex[] = "0123456789abcdef";
    unsigned h = 0x6f75u;
    int i, n = 0;
    for (i = 0; i < (int)d->nnodes && i < 64; i++) h = h * 131u + (unsigned)d->nodes[i].form_ord + (unsigned)d->nodes[i].fname_len;
    if (d->forms) h ^= ((unsigned)d->forms->edits << 8) ^ (unsigned)d->forms->submits;
    {
        const char *p = "----mohhdy";
        while (*p && n < max - 1) out[n++] = *p++;
    }
    for (i = 0; i < 16 && n < max - 1; i++) {
        out[n++] = hex[h & 15u];
        h = h * 1103515245u + 12345u;
    }
    out[n] = 0;
}

int we_form_request(const we_doc_t *d, we_form_req_t *req) {
    int f, i, o = 0, sub, a, k;
    char v[256];   /* Lot 75: attr values up to 255; was WE_FSTATE_VAL */
    if (!d || !req || !d->forms || d->forms->submit_form < 0 || d->forms->submit_form >= (int)d->nnodes) return -1;
    f = d->forms->submit_form;
    if (!w_eq(d->nodes[f].tag, "form")) return -1;
    w_zero(req, sizeof(*req));
    sub = finput_node(d, d->forms->submitter);
    req->form = (int16_t)f;
    req->post = (d->nodes[f].form_flags2 & WE_FF2_POST) ? 1 : 0;
    req->multipart = (d->nodes[f].form_flags2 & WE_FF2_MULTIPART) ? 1 : 0;
    if (sub >= 0) {
        if (d->nodes[sub].form_flags2 & WE_FF2_POST) req->post = 1;
        if (d->nodes[sub].form_flags2 & WE_FF2_DIALOG) { /* formmethod=dialog: no request */ }
        if (d->nodes[sub].form_flags2 & WE_FF2_MULTIPART) req->multipart = 1;
    }
    /* Lot 76: formtarget from submitter, else the form */
    {
        const we_node_t *tn = 0;
        if (sub >= 0 && (d->nodes[sub].form_flags2 & WE_FF2_FORMTARGET)) tn = &d->nodes[sub];
        else if (d->nodes[f].form_flags2 & WE_FF2_FORMTARGET) tn = &d->nodes[f];
        if (tn && tn->ftarget_len) {
            int tl = tn->ftarget_len < 15 ? tn->ftarget_len : 15;
            for (k = 0; k < tl; k++) req->target[k] = d->pool[tn->ftarget_off + k];
            req->target[tl] = 0;
            if (tn->form_flags2 & WE_FF2_TARGET_BLANK) req->target_blank = 1;
        } else {
            req->target[0] = '_'; req->target[1] = 's'; req->target[2] = 'e';
            req->target[3] = 'l'; req->target[4] = 'f'; req->target[5] = 0;
        }
    }
    /* GET never uses multipart bodies */
    if (!req->post) req->multipart = 0;
    if (req->multipart) {
        finput_boundary(req->boundary, WE_FREQ_BOUNDARY, d);
        {
            const char *p = "multipart/form-data; boundary=";
            int n = 0;
            while (*p && n < WE_FREQ_CTYPE - 1) req->ctype[n++] = *p++;
            for (k = 0; req->boundary[k] && n < WE_FREQ_CTYPE - 1; k++) req->ctype[n++] = req->boundary[k];
            req->ctype[n] = 0;
        }
    } else {
        const char *p = "application/x-www-form-urlencoded";
        for (k = 0; p[k] && k < WE_FREQ_CTYPE - 1; k++) req->ctype[k] = p[k];
        req->ctype[k] = 0;
    }
    for (i = 1; i < (int)d->nnodes; i++) {
        const we_node_t *n = &d->nodes[i];
        int vl, rc;
        if (!n->form_ord || !n->fname_len || finput_owner(d, i) != f || finput_disabled(d, i)) continue;
        if (w_eq(n->tag, "details") || w_eq(n->tag, "dialog")) continue;
        if (finput_submit_kind(n) || n->form_type == WE_FORM_RESET || n->form_type == WE_FORM_BUTTON) {
            if (i != sub) continue;
            vl = (n->form_flags & WE_FF_VALUE) ? (n->fval_len < (int)sizeof(v) - 1 ? n->fval_len : (int)sizeof(v) - 1) : 0;
            w_cpy(v, (int)sizeof(v), d->pool + n->fval_off, vl);
            if (req->multipart) {
                rc = finput_mp_field(d, req, &o, n, v, vl, 0);
                if (rc != 0) return rc == -1 ? -1 : -2;
            } else if (finput_pair(req->body, WE_FREQ_BODY, &o, d, n, v, vl, 0) != 0) return -2;
            continue;
        }
        if (n->form_type == WE_FORM_FILE) {
            we_fstate_t tmp;
            const we_fstate_t *s = &tmp;
            if (!req->multipart) continue;   /* file only in multipart */
            if (d->forms->c[n->form_ord - 1].set) s = &d->forms->c[n->form_ord - 1];
            else { finput_capture(d, i, &tmp); }
            if (!s->len) {
                /* empty file control: still send the field with empty filename */
                rc = finput_mp_field(d, req, &o, n, "", 0, 1);
                if (rc != 0) return rc == -1 ? -1 : -2;
                continue;
            }
            rc = finput_mp_field(d, req, &o, n, s->val, s->len, 1);
            if (rc == -1) { return -1; }   /* refused: no reader / too big / bad read */
            if (rc != 0) return -2;
            continue;
        }
        if (w_eq(n->tag, "select") && forms_listbox(n)) {
            int cnt;
            (void)finput_option(d, i, -1, &cnt);
            for (k = 0; k < cnt; k++) {
                int opt = finput_option(d, i, k, 0);
                if (!(d->nodes[opt].form_flags & WE_FF_SELECTED)) continue;
                vl = finput_option_value(d, opt, v, (int)sizeof(v));
                if (req->multipart) {
                    rc = finput_mp_field(d, req, &o, n, v, vl, 0);
                    if (rc != 0) return rc == -1 ? -1 : -2;
                } else if (finput_pair(req->body, WE_FREQ_BODY, &o, d, n, v, vl, 0) != 0) return -2;
            }
            continue;
        }
        if ((n->form_type == WE_FORM_CHECKBOX || n->form_type == WE_FORM_RADIO) && !(n->form_flags & WE_FF_CHECKED)) continue;
        vl = finput_cur(d, i, v, (int)sizeof(v));
        if (req->multipart) {
            rc = finput_mp_field(d, req, &o, n, v, vl, 0);
            if (rc != 0) return rc == -1 ? -1 : -2;
        } else if (finput_pair(req->body, WE_FREQ_BODY, &o, d, n, v, vl, w_eq(n->tag, "textarea")) != 0) return -2;
    }
    if (req->multipart) {
        if (finput_mp_str(req->body, WE_FREQ_BODY, &o, "--") != 0) return -2;
        if (finput_mp_str(req->body, WE_FREQ_BODY, &o, req->boundary) != 0) return -2;
        if (finput_mp_str(req->body, WE_FREQ_BODY, &o, "--\r\n") != 0) return -2;
    }
    req->body[o] = 0;
    req->body_len = o;
    /* the action: submitter formaction overrides; GET drops query/fragment */
    if (sub >= 0 && (d->nodes[sub].form_flags2 & WE_FF2_FORMACTION) && d->nodes[sub].flist_len) {
        a = d->nodes[sub].flist_len;
        for (k = 0; k < a && k < WE_FREQ_URL - 1; k++) {
            char ch = d->pool[d->nodes[sub].flist_off + k];
            if (ch == '#' || (!req->post && ch == '?')) break;
            req->url[k] = ch;
        }
    } else {
        a = d->nodes[f].fval_len;
        for (k = 0; k < a && k < WE_FREQ_URL - 1; k++) {
            char ch = d->pool[d->nodes[f].fval_off + k];
            if (ch == '#' || (!req->post && ch == '?')) break;
            req->url[k] = ch;
        }
    }
    if (!req->post) {
        if (k + 1 + o >= WE_FREQ_URL) return -2;
        req->url[k++] = '?';
        for (i = 0; i < o; i++) req->url[k++] = req->body[i];
    }
    req->url[k] = 0;
    return 0;
}

/* ---------- Lot 74: the URL fragment (:target) ---------- */
static int finput_hex(int c) {
    return c >= '0' && c <= '9' ? c - '0' : c >= 'a' && c <= 'f' ? c - 'a' + 10 : c >= 'A' && c <= 'F' ? c - 'A' + 10 : -1;
}

int we_set_target(we_doc_t *d, const char *frag) {
    char t[WE_TARGET_MAX];
    int i = 0, n = 0;
    if (!d || !d->forms) return -1;
    if (!frag) frag = "";
    if (frag[0] == '#') i = 1;
    for (; frag[i]; i++) {
        int c = (unsigned char)frag[i];
        if (c == '%' && finput_hex(frag[i + 1]) >= 0 && finput_hex(frag[i + 2]) >= 0) {
            c = finput_hex(frag[i + 1]) * 16 + finput_hex(frag[i + 2]);
            i += 2;
        }
        if (n + 1 >= WE_TARGET_MAX || c == 0) return -1;
        t[n++] = (char)c;
    }
    t[n] = 0;
    for (i = 0; i <= n; i++) d->forms->target[i] = t[i];
    return n ? 0 : 1;
}

int we_target_node(const we_doc_t *d) {
    uint32_t i;
    if (!d || !d->forms || !d->forms->target[0]) return -1;
    for (i = 1; i < d->nnodes; i++) {
        const we_node_t *n = &d->nodes[i];
        if (!n->is_text && !n->pseudo_kind && n->id[0] && w_eq(n->id, d->forms->target)) return (int)i;
    }
    return -1;
}
