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
 * pairs of the successful controls in tree order (HTML 4.10.21), GET
 * puts them as the query of the action (its old query and fragment
 * dropped), POST as the body; multipart/form-data is sent urlencoded and
 * reported. Values are bytes as the page has them (no charset change). */

/* WE_FF2_FOCUS 4, STALE 32 (a textarea text replaced by the state), CARET
 * 64 (the caret text), EMAIL 256 (type=email): webeng_forms.inc */
#define WE_FF2_POST 1
#define WE_FF2_MULTIPART 2
#define WE_FF2_READONLY 8
#define WE_FF2_REQUIRED 16
#define WE_FF2_NOVALIDATE 128 /* form novalidate, submitter formnovalidate */

/* attributes of Lot 74 on input, select, textarea, button, form, label:
 * 1 when taken */
static int finput_attr(we_doc_t *d, int idx, const char *an, const char *av, int vk) {
    we_node_t *n = &d->nodes[idx];
    if (w_eq(an, "name")) { if (pool_put(d, av, vk, &n->fname_off) == 0) n->fname_len = (uint16_t)vk; return 1; }
    if (w_eq(an, "readonly")) { n->form_flags2 |= WE_FF2_READONLY; return 1; }
    if (w_eq(an, "required")) { n->form_flags2 |= WE_FF2_REQUIRED; return 1; }
    if (w_eq(an, "novalidate") || w_eq(an, "formnovalid")   /* formnovalidate, cut to 11 bytes */) { n->form_flags2 |= WE_FF2_NOVALIDATE; return 1; }
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
        if (w_eq(an, "method")) { if (value_is(av, vk, "post")) n->form_flags2 |= WE_FF2_POST; return 1; }
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
           w_eq(n->tag, "button") || w_eq(n->tag, "details");
}

static int finput_node(const we_doc_t *d, int ctl) {
    int i;
    if (ctl < 0) return -1;
    for (i = 1; i < (int)d->nnodes; i++) if (d->nodes[i].form_ord == ctl + 1) return i;
    return -1;
}

static int finput_textlike(const we_node_t *n) {
    return w_eq(n->tag, "textarea") ||
           (w_eq(n->tag, "input") && (n->form_type == WE_FORM_TEXT || n->form_type == WE_FORM_PASSWORD || !n->form_type));
}

static int finput_submit_kind(const we_node_t *n) {
    return (w_eq(n->tag, "button") && (n->form_type == WE_FORM_SUBMIT || !n->form_type)) ||
           (w_eq(n->tag, "input") && n->form_type == WE_FORM_SUBMIT);
}

/* the form a node belongs to (nearest form ancestor), or -1 */
static int finput_owner(const we_doc_t *d, int i) {
    int p = d->nodes[i].parent;
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
    if (w_eq(n->tag, "details")) { s->checked = (n->form_flags & WE_FF_OPEN) ? 1 : 0; return; }
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
    for (i = 1; i < count; i++) {
        we_node_t *n = &d->nodes[i];
        const we_fstate_t *s;
        if (!n->form_ord) continue;
        if (d->forms->focus == n->form_ord - 1) n->form_flags2 |= WE_FF2_FOCUS;
        s = &d->forms->c[n->form_ord - 1];
        if (!s->set) continue;
        if (w_eq(n->tag, "details")) {
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
            if (finput_textlike(n)) {
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
 * unchecked or no option chosen; type=email without '@' around text) */
static int finput_invalid(const we_doc_t *d, int i) {
    const we_node_t *n = &d->nodes[i];
    char v[WE_FSTATE_VAL];
    int len, k, at = -1;
    if (!n->form_ord || finput_disabled(d, i) || (n->form_flags2 & WE_FF2_READONLY)) return 0;
    if (!(w_eq(n->tag, "input") || w_eq(n->tag, "textarea") || w_eq(n->tag, "select"))) return 0;
    if (w_eq(n->tag, "input") && (n->form_type == WE_FORM_HIDDEN || n->form_type == WE_FORM_SUBMIT ||
                                  n->form_type == WE_FORM_RESET || n->form_type == WE_FORM_BUTTON)) return 0;
    len = finput_cur(d, i, v, (int)sizeof(v));
    if ((n->form_flags2 & WE_FF2_REQUIRED) && len == 0) return 1;
    if ((n->form_flags2 & WE_FF2_EMAIL) && len > 0) {
        for (k = 0; k < len; k++) if (v[k] == '@') { if (at >= 0) return 1; at = k; }
        if (at <= 0 || at == len - 1) return 1;
    }
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
    int bad;
    if (f < 0) return WE_FACT_NONE;
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
    if (w_eq(n->tag, "details")) {
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
    return finput_activate(d, i, opt);
}

/* next (dir 1) or previous (-1) focusable control after ctl */
static int finput_next(const we_doc_t *d, int ctl, int dir) {
    int k, total = d->nform_ctl, base = ctl >= 0 ? ctl : (dir > 0 ? -1 : total);
    for (k = 1; k <= total; k++) {
        int c = ((base + dir * k) % total + total) % total, i = finput_node(d, c);
        if (i >= 0 && finput_focusable(d, i)) return c;
    }
    return -1;
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
    if (finput_textlike(n)) {
        int max = n->form_maxlen ? n->form_maxlen : WE_FSTATE_VAL - 1;
        int area = w_eq(n->tag, "textarea");
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

int we_form_request(const we_doc_t *d, we_form_req_t *req) {
    int f, i, o = 0, sub, a, k;
    char v[WE_FSTATE_VAL];
    if (!d || !req || !d->forms || d->forms->submit_form < 0 || d->forms->submit_form >= (int)d->nnodes) return -1;
    f = d->forms->submit_form;
    if (!w_eq(d->nodes[f].tag, "form")) return -1;
    w_zero(req, sizeof(*req));
    sub = finput_node(d, d->forms->submitter);
    req->form = (int16_t)f;
    req->post = (d->nodes[f].form_flags2 & WE_FF2_POST) ? 1 : 0;
    req->multipart = (d->nodes[f].form_flags2 & WE_FF2_MULTIPART) ? 1 : 0;
    for (i = 1; i < (int)d->nnodes; i++) {
        const we_node_t *n = &d->nodes[i];
        int vl;
        if (!n->form_ord || !n->fname_len || finput_owner(d, i) != f || finput_disabled(d, i)) continue;
        if (w_eq(n->tag, "details")) continue;
        if (finput_submit_kind(n) || n->form_type == WE_FORM_RESET || n->form_type == WE_FORM_BUTTON) {
            if (i != sub) continue;
            vl = (n->form_flags & WE_FF_VALUE) ? (n->fval_len < (int)sizeof(v) - 1 ? n->fval_len : (int)sizeof(v) - 1) : 0;
            w_cpy(v, (int)sizeof(v), d->pool + n->fval_off, vl);
            if (finput_pair(req->body, WE_FREQ_BODY, &o, d, n, v, vl, 0) != 0) return -2;
            continue;
        }
        if (w_eq(n->tag, "select") && forms_listbox(n)) {
            int cnt;
            (void)finput_option(d, i, -1, &cnt);
            for (k = 0; k < cnt; k++) {
                int opt = finput_option(d, i, k, 0);
                if (!(d->nodes[opt].form_flags & WE_FF_SELECTED)) continue;
                vl = finput_option_value(d, opt, v, (int)sizeof(v));
                if (finput_pair(req->body, WE_FREQ_BODY, &o, d, n, v, vl, 0) != 0) return -2;
            }
            continue;
        }
        if ((n->form_type == WE_FORM_CHECKBOX || n->form_type == WE_FORM_RADIO) && !(n->form_flags & WE_FF_CHECKED)) continue;
        vl = finput_cur(d, i, v, (int)sizeof(v));
        if (finput_pair(req->body, WE_FREQ_BODY, &o, d, n, v, vl, w_eq(n->tag, "textarea")) != 0) return -2;
    }
    req->body[o] = 0;
    req->body_len = o;
    /* the action: GET drops its query and fragment */
    a = d->nodes[f].fval_len;
    for (k = 0; k < a && k < WE_FREQ_URL - 1; k++) {
        char c = d->pool[d->nodes[f].fval_off + k];
        if (c == '#' || (!req->post && c == '?')) break;
        req->url[k] = c;
    }
    if (!req->post) {
        if (k + 1 + o >= WE_FREQ_URL) return -2;
        req->url[k++] = '?';
        for (i = 0; i < o; i++) req->url[k++] = req->body[i];
    }
    req->url[k] = 0;
    return 0;
}
