/* wf_form.c - Lot 71: forms of a fetched page (see wf_form.h). */
#include "wf_form.h"
#include "wf_entity.h"

static int f_len(const char *s) { int n = 0; while (s && s[n]) n++; return n; }
static void f_cpy(char *d, int cap, const char *s) {
    int i = 0;
    if (cap <= 0) return;
    while (s && s[i] && i < cap - 1) { d[i] = s[i]; i++; }
    d[i] = 0;
}
static int f_low(int c) { return c >= 'A' && c <= 'Z' ? c + 32 : c; }
static int f_ieq(const char *a, const char *b) {
    if (!a) a = "";
    while (*a && f_low((unsigned char)*a) == *b) { a++; b++; }
    return *a == 0 && *b == 0;
}
static int f_pre(const char *s, const char *p) { while (*p && *s == *p) { s++; p++; } return *p == 0; }
static int f_seq(const char *a, const char *b) { while (*a && *a == *b) { a++; b++; } return *a == *b; }

void wf_form_reset(wf_form_page_t *p) {
    int i;
    for (i = 0; i < (int)sizeof(*p); i++) ((char *)p)[i] = 0;
    p->focus = -1;
    p->current = -1;
}

static int resolve_action(const wf_url_t *base, const char *action, char *out, int cap) {
    static wf_url_t u;
    if (!base) { f_cpy(out, cap, action ? action : ""); return 0; }
    if (!action || !action[0]) {
        wf_url_format(base, out, cap);
        return 0;
    }
    /* only http(s) or relative actions (javascript:, mailto: refused) */
    {
        int k;
        for (k = 0; action[k] && action[k] != '/' && action[k] != '?' && action[k] != '#'; k++)
            if (action[k] == ':') {
                if (!(k == 4 && f_low(action[0]) == 'h' && f_low(action[1]) == 't' && f_low(action[2]) == 't' && f_low(action[3]) == 'p') &&
                    !(k == 5 && f_low(action[0]) == 'h' && f_low(action[1]) == 't' && f_low(action[2]) == 't' &&
                      f_low(action[3]) == 'p' && f_low(action[4]) == 's'))
                    return -1;
                break;
            }
    }
    if (wf_url_resolve(base, action, &u) != 0) return -1;
    wf_url_format(&u, out, cap);
    return 0;
}

int wf_form_begin(wf_form_page_t *p, const wf_url_t *base, const char *action, const char *method, const char *enctype) {
    wf_form_t *f;
    if (p->nforms >= WF_FORM_MAX) { p->current = -1; p->dropped++; return -1; }
    f = &p->form[p->nforms];
    if (resolve_action(base, action, f->action, (int)sizeof(f->action)) != 0) { p->current = -1; p->dropped++; return -1; }
    f->method = f_ieq(method, "post") ? WF_METHOD_POST : WF_METHOD_GET;
    f->multipart = (enctype && f_low((unsigned char)enctype[0]) == 'm') ? 1 : 0;
    f->nfields = 0;
    f->implicit = 0;
    p->current = p->nforms++;
    return p->current;
}

void wf_form_end(wf_form_page_t *p) { p->current = -1; }

static int implicit_form(wf_form_page_t *p, const wf_url_t *base) {
    int i;
    /* form-less controls share one implicit GET form on the page URL */
    for (i = 0; i < p->nforms; i++) if (p->form[i].implicit) return i;
    if (wf_form_begin(p, base, "", "get", "") < 0) return -1;
    i = p->current;
    p->form[i].implicit = 1;
    p->current = -1;
    return i;
}

int wf_form_add(wf_form_page_t *p, const wf_url_t *base, const char *type, const char *name, const char *value,
                const char *label, int checked, int maxlen) {
    wf_field_t *fl;
    int kind, form = p->current;
    if (!type || !type[0] || f_ieq(type, "text") || f_ieq(type, "search") || f_ieq(type, "email") || f_ieq(type, "url") ||
        f_ieq(type, "tel") || f_ieq(type, "number")) kind = WF_FIELD_TEXT;
    else if (f_ieq(type, "password")) kind = WF_FIELD_PASSWORD;
    else if (f_ieq(type, "textarea")) kind = WF_FIELD_TEXTAREA;
    else if (f_ieq(type, "hidden")) kind = WF_FIELD_HIDDEN;
    else if (f_ieq(type, "select")) kind = WF_FIELD_SELECT;
    else if (f_ieq(type, "checkbox")) kind = WF_FIELD_CHECKBOX;
    else if (f_ieq(type, "radio")) kind = WF_FIELD_RADIO;
    else if (f_ieq(type, "submit") || f_ieq(type, "image") || f_ieq(type, "button-submit")) kind = WF_FIELD_SUBMIT;
    else return -1;   /* file, reset, button (no submit), color, range, date...: not kept */
    if (p->nfields >= WF_FORM_FIELDS) { p->dropped++; return -1; }
    if (form < 0) form = implicit_form(p, base);
    if (form < 0) { p->dropped++; return -1; }
    fl = &p->field[p->nfields];
    fl->type = (uint8_t)kind;
    fl->form = (uint8_t)form;
    fl->named = (name && name[0]) ? 1 : 0;
    f_cpy(fl->name, (int)sizeof(fl->name), name);
    f_cpy(fl->value, (int)sizeof(fl->value), ((kind == WF_FIELD_CHECKBOX || kind == WF_FIELD_RADIO) && (!value || !value[0])) ? "on" : value);
    fl->checked = (uint8_t)((kind == WF_FIELD_CHECKBOX || kind == WF_FIELD_RADIO) && checked ? 1 : 0);
    fl->opt_start = (int16_t)p->opt_used;
    fl->nopt = 0;
    fl->sel = 0;
    if (kind == WF_FIELD_RADIO && fl->checked) {
        /* a later checked radio of the same group wins, as in browsers */
        int k;
        for (k = 0; k < p->nfields; k++)
            if (p->field[k].type == WF_FIELD_RADIO && p->field[k].form == (uint8_t)form && f_seq(p->field[k].name, fl->name))
                p->field[k].checked = 0;
    }
    f_cpy(fl->label, (int)sizeof(fl->label), label);
    if (kind == WF_FIELD_SUBMIT && !fl->label[0]) f_cpy(fl->label, (int)sizeof(fl->label), fl->value[0] ? fl->value : "Envoyer");
    fl->maxlen = (int16_t)((maxlen > 0 && maxlen < WF_FORM_VALUE) ? maxlen : WF_FORM_VALUE - 1);
    fl->cursor = (int16_t)f_len(fl->value);
    if (p->form[form].nfields < 254) p->form[form].nfields++;
    return p->nfields++;
}

int wf_form_add_option(wf_form_page_t *p, int field, const char *value, const char *label, int selected) {
    wf_field_t *f;
    int vl, ll, k;
    if (field < 0 || field >= p->nfields || field != p->nfields - 1) return -1;
    f = &p->field[field];
    if (f->type != WF_FIELD_SELECT) return -1;
    if (!label) label = "";
    if (!value) value = label;
    vl = f_len(value); ll = f_len(label);
    if (vl >= WF_FORM_VALUE) vl = WF_FORM_VALUE - 1;
    if (ll >= WF_FORM_LABEL) ll = WF_FORM_LABEL - 1;
    if (f->nopt >= WF_FORM_OPTIONS || p->opt_used + vl + ll + 2 > WF_FORM_OPT_POOL) { p->opt_dropped++; return -1; }
    for (k = 0; k < vl; k++) p->opt_pool[p->opt_used++] = value[k];
    p->opt_pool[p->opt_used++] = 0;
    for (k = 0; k < ll; k++) p->opt_pool[p->opt_used++] = label[k];
    p->opt_pool[p->opt_used++] = 0;
    if (selected || f->nopt == 0) {
        f->sel = f->nopt;
        f_cpy(f->value, (int)sizeof(f->value), value);
    }
    return f->nopt++;
}

static const char *opt_at(const wf_form_page_t *p, int field, int k, int label) {
    const wf_field_t *f;
    int o, i;
    if (field < 0 || field >= p->nfields) return 0;
    f = &p->field[field];
    if (f->type != WF_FIELD_SELECT || k < 0 || k >= f->nopt) return 0;
    o = f->opt_start;
    for (i = 0; i < k * 2 + label; i++) { while (o < p->opt_used && p->opt_pool[o]) o++; o++; }
    return o < p->opt_used ? p->opt_pool + o : 0;
}
const char *wf_form_option_value(const wf_form_page_t *p, int field, int k) { return opt_at(p, field, k, 0); }
const char *wf_form_option_label(const wf_form_page_t *p, int field, int k) { return opt_at(p, field, k, 1); }

int wf_form_is_toggle(const wf_form_page_t *p, int field) {
    int t;
    if (field < 0 || field >= p->nfields) return 0;
    t = p->field[field].type;
    return t == WF_FIELD_CHECKBOX || t == WF_FIELD_RADIO || t == WF_FIELD_SELECT;
}

static void radio_check(wf_form_page_t *p, int field) {
    int k;
    wf_field_t *f = &p->field[field];
    for (k = 0; k < p->nfields; k++)
        if (p->field[k].type == WF_FIELD_RADIO && p->field[k].form == f->form && f_seq(p->field[k].name, f->name))
            p->field[k].checked = 0;
    f->checked = 1;
}

static void select_to(wf_form_page_t *p, int field, int k) {
    wf_field_t *f = &p->field[field];
    const char *v = wf_form_option_value(p, field, k);
    if (!v) return;
    f->sel = (uint8_t)k;
    f_cpy(f->value, (int)sizeof(f->value), v);
}

int wf_form_activate(wf_form_page_t *p, int field) {
    wf_field_t *f;
    if (!wf_form_is_toggle(p, field)) return -1;
    f = &p->field[field];
    if (f->type == WF_FIELD_CHECKBOX) f->checked = (uint8_t)!f->checked;
    else if (f->type == WF_FIELD_RADIO) radio_check(p, field);
    else {
        if (!f->nopt) return -1;
        select_to(p, field, (f->sel + 1) % f->nopt);
    }
    return 0;
}

static int editable(const wf_field_t *f) {
    return f->type == WF_FIELD_TEXT || f->type == WF_FIELD_PASSWORD || f->type == WF_FIELD_TEXTAREA;
}

int wf_form_focus(wf_form_page_t *p, int field) {
    if (field < 0 || field >= p->nfields || !editable(&p->field[field])) { p->focus = -1; return -1; }
    p->focus = field;
    p->field[field].cursor = (int16_t)f_len(p->field[field].value);
    return 0;
}

int wf_form_focus_next(wf_form_page_t *p, int dir) {
    int i, k;
    if (p->nfields == 0) return -1;
    for (k = 1; k <= p->nfields; k++) {
        i = p->focus < 0 ? (dir > 0 ? k - 1 : p->nfields - k) : ((p->focus + dir * k) % p->nfields + p->nfields) % p->nfields;
        if (editable(&p->field[i])) return wf_form_focus(p, i);
    }
    return -1;
}

int wf_form_insert(wf_form_page_t *p, char c) {
    wf_field_t *f;
    int n, i;
    if (p->focus < 0) return -1;
    f = &p->field[p->focus];
    n = f_len(f->value);
    if ((unsigned char)c < 0x20u && c != '\n') return -1;
    if (c == '\n' && f->type != WF_FIELD_TEXTAREA) return -1;
    if (n >= f->maxlen || n >= WF_FORM_VALUE - 1) return -1;
    if (f->cursor < 0 || f->cursor > n) f->cursor = (int16_t)n;
    for (i = n; i > f->cursor; i--) f->value[i] = f->value[i - 1];
    f->value[f->cursor] = c;
    f->value[n + 1] = 0;
    f->cursor++;
    return 0;
}

int wf_form_backspace(wf_form_page_t *p) {
    wf_field_t *f;
    int n, i;
    if (p->focus < 0) return -1;
    f = &p->field[p->focus];
    n = f_len(f->value);
    if (f->cursor > n) f->cursor = (int16_t)n;
    if (f->cursor <= 0) return -1;
    for (i = f->cursor - 1; i < n; i++) f->value[i] = f->value[i + 1];
    f->cursor--;
    return 0;
}

int wf_form_delete(wf_form_page_t *p) {
    wf_field_t *f;
    int n, i;
    if (p->focus < 0) return -1;
    f = &p->field[p->focus];
    n = f_len(f->value);
    if (f->cursor >= n) return -1;
    for (i = f->cursor; i < n; i++) f->value[i] = f->value[i + 1];
    return 0;
}

int wf_form_move(wf_form_page_t *p, int delta) {
    wf_field_t *f;
    int n, c;
    if (p->focus < 0) return -1;
    f = &p->field[p->focus];
    n = f_len(f->value);
    c = f->cursor + delta;
    if (c < 0) c = 0;
    if (c > n) c = n;
    if (c == f->cursor) return -1;
    f->cursor = (int16_t)c;
    return 0;
}

int wf_form_set(wf_form_page_t *p, int field, const char *value) {
    wf_field_t *f;
    if (field < 0 || field >= p->nfields) return -1;
    f = &p->field[field];
    if (f->type == WF_FIELD_SUBMIT) return -1;
    if (f->type == WF_FIELD_CHECKBOX || f->type == WF_FIELD_RADIO) {
        int on = f_ieq(value, "on") || f_ieq(value, "1") || f_ieq(value, "yes") || f_ieq(value, "oui");
        int off = f_ieq(value, "off") || f_ieq(value, "0") || f_ieq(value, "no") || f_ieq(value, "non");
        if (!on && !off) return -1;
        if (f->type == WF_FIELD_RADIO) { if (!on) return -1; radio_check(p, field); }
        else f->checked = (uint8_t)on;
        return 0;
    }
    if (f->type == WF_FIELD_SELECT) {
        int k, n = 0;
        if (value[0] == '#') {
            for (k = 1; value[k] >= '0' && value[k] <= '9'; k++) n = n * 10 + (value[k] - '0');
            if (value[k] || n < 1 || n > f->nopt) return -1;
            select_to(p, field, n - 1);
            return 0;
        }
        for (k = 0; k < f->nopt; k++) if (f_seq(wf_form_option_value(p, field, k), value)) { select_to(p, field, k); return 0; }
        for (k = 0; k < f->nopt; k++) if (f_ieq(wf_form_option_label(p, field, k), value)) { select_to(p, field, k); return 0; }
        return -1;
    }
    f_cpy(f->value, f->maxlen + 1 < WF_FORM_VALUE ? f->maxlen + 1 : WF_FORM_VALUE, value);
    f->cursor = (int16_t)f_len(f->value);
    return 0;
}

int wf_form_display(const wf_form_page_t *p, int field, char *out, int cap) {
    const wf_field_t *f;
    int o = 0, i, n, focus, show_label, width = 24, start = 0;
    if (cap < 8 || field < 0 || field >= p->nfields) { if (cap > 0) out[0] = 0; return 0; }
    f = &p->field[field];
    if (f->type == WF_FIELD_CHECKBOX || f->type == WF_FIELD_RADIO) {
        const char *l = f->label[0] ? f->label : f->value;
        int radio = f->type == WF_FIELD_RADIO;
        out[o++] = radio ? '(' : '[';
        out[o++] = f->checked ? (radio ? '*' : 'x') : ' ';
        out[o++] = radio ? ')' : ']';
        out[o++] = ' ';
        for (i = 0; l[i] && o < cap - 1 && i < width; i++) out[o++] = (l[i] == '<' || l[i] == '>') ? ' ' : l[i];
        out[o] = 0;
        return o;
    }
    if (f->type == WF_FIELD_SELECT) {
        const char *l = wf_form_option_label(p, field, f->sel);
        if (!l || !l[0]) l = f->value;
        out[o++] = '[';
        out[o++] = ' ';
        for (i = 0; l && l[i] && o < cap - 5 && i < width; i++) out[o++] = (l[i] == '<' || l[i] == '>') ? ' ' : l[i];
        out[o++] = ' ';
        out[o++] = 'v';
        out[o++] = ' ';
        out[o++] = ']';
        out[o] = 0;
        return o;
    }
    if (f->type == WF_FIELD_SUBMIT) {
        out[o++] = '[';
        out[o++] = ' ';
        for (i = 0; f->label[i] && o < cap - 3; i++) out[o++] = (f->label[i] == '<' || f->label[i] == '>') ? ' ' : f->label[i];
        out[o++] = ' ';
        out[o++] = ']';
        out[o] = 0;
        return o;
    }
    focus = p->focus == field;
    n = f_len(f->value);
    show_label = n == 0 && !focus && f->label[0];
    /* a long value scrolls so the cursor stays visible */
    if (focus && f->cursor > width - 2) start = f->cursor - (width - 2);
    out[o++] = '[';
    out[o++] = ' ';
    if (show_label) {
        for (i = 0; f->label[i] && o < cap - 4 && i < width; i++) out[o++] = f->label[i] == '<' ? ' ' : f->label[i];
    } else {
        for (i = start; i <= n && o < cap - 4 && i - start < width; i++) {
            if (focus && i == f->cursor) out[o++] = '|';
            if (i == n) break;
            {
                char c = f->type == WF_FIELD_PASSWORD ? '*' : f->value[i];
                if (c == '\n') c = ' ';
                out[o++] = c;
            }
        }
    }
    while (o < cap - 3 && o < width + 2 + (focus ? 1 : 0)) out[o++] = '_';
    out[o++] = ' ';
    out[o++] = ']';
    out[o] = 0;
    return o;
}

static int hexd(int v) { return v < 10 ? '0' + v : 'A' + v - 10; }

int wf_form_urlencode_append(char *out, int cap, int len, const char *s) {
    return wf_form_urlencode_append_cs(out, cap, len, s, 0);
}
int wf_form_urlencode_append_cs(char *out, int cap, int len, const char *s, int latin1) {
    static char u8[WF_FORM_VALUE * 3 + 4];
    int i, n;
    if (latin1) {
        for (n = 0; s[n] && n < (int)sizeof(u8) - 1; n++) u8[n] = s[n];
        u8[n] = 0;
    } else n = wf_display_to_utf8(s, f_len(s), u8, (int)sizeof(u8));
    for (i = 0; i < n; i++) {
        unsigned char c = (unsigned char)u8[i];
        if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '-' || c == '_' || c == '.' || c == '*') {
            if (len + 1 >= cap) return -1;
            out[len++] = (char)c;
        } else if (c == ' ') {
            if (len + 1 >= cap) return -1;
            out[len++] = '+';
        } else if (c == '\n') {
            if (len + 6 >= cap) return -1;
            out[len++] = '%'; out[len++] = '0'; out[len++] = 'D'; out[len++] = '%'; out[len++] = '0'; out[len++] = 'A';
        } else {
            if (len + 3 >= cap) return -1;
            out[len++] = '%';
            out[len++] = (char)hexd(c >> 4);
            out[len++] = (char)hexd(c & 15);
        }
    }
    out[len] = 0;
    return len;
}

int wf_form_submit(const wf_form_page_t *p, int fi, int submitter, wf_form_submit_t *out) {
    static char q[WF_FORM_BODY_MAX];
    const wf_form_t *f;
    int i, len = 0, k;
    for (i = 0; i < (int)sizeof(*out); i++) ((char *)out)[i] = 0;
    if (fi < 0 || fi >= p->nforms) return -1;
    f = &p->form[fi];
    if (f->multipart) return -1;
    q[0] = 0;
    for (i = 0; i < p->nfields; i++) {
        const wf_field_t *fl = &p->field[i];
        if (fl->form != fi || !fl->named) continue;
        if (fl->type == WF_FIELD_SUBMIT && i != submitter) continue;
        if ((fl->type == WF_FIELD_CHECKBOX || fl->type == WF_FIELD_RADIO) && !fl->checked) continue;
        if (fl->type == WF_FIELD_SELECT && !fl->nopt) continue;
        if (len) { if (len + 1 >= (int)sizeof(q)) return -1; q[len++] = '&'; q[len] = 0; }
        len = wf_form_urlencode_append_cs(q, (int)sizeof(q), len, fl->name, p->latin1);
        if (len < 0 || len + 1 >= (int)sizeof(q)) return -1;
        q[len++] = '=';
        q[len] = 0;
        len = wf_form_urlencode_append_cs(q, (int)sizeof(q), len, fl->value, p->latin1);
        if (len < 0) return -1;
    }
    out->method = f->method;
    if (f->method == WF_METHOD_POST) {
        f_cpy(out->url, (int)sizeof(out->url), f->action);
        for (k = 0; k < len; k++) out->body[k] = q[k];
        out->body[len] = 0;
        out->body_len = len;
        out->content_type = "application/x-www-form-urlencoded";
        return 0;
    }
    /* GET: the query replaces the action's query; the fragment is dropped */
    k = 0;
    while (f->action[k] && f->action[k] != '?' && f->action[k] != '#' && k < (int)sizeof(out->url) - 1) {
        out->url[k] = f->action[k];
        k++;
    }
    if (k + 1 + len >= (int)sizeof(out->url)) return -1;
    out->url[k++] = '?';
    for (i = 0; i < len; i++) out->url[k++] = q[i];
    out->url[k] = 0;
    return 0;
}

int wf_form_first_text(const wf_form_page_t *p, int form) {
    int i;
    for (i = 0; i < p->nfields; i++)
        if ((form < 0 || p->field[i].form == form) && editable(&p->field[i])) return i;
    return -1;
}

int wf_form_href(const char *href, int *n) {
    int v = 0, d = 0;
    char kind;
    if (!href || !f_pre(href, "mohhdy-form:")) return 0;
    kind = href[12];
    if ((kind != 'f' && kind != 's') || href[13] != ':') return 0;
    href += 14;
    while (*href >= '0' && *href <= '9' && d < 4) { v = v * 10 + (*href++ - '0'); d++; }
    if (!d || *href) return 0;
    *n = v;
    return kind;
}
