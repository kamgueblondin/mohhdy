/* wf_formfill.c - Lot 86: see wf_formfill.h. */
#include "wf_formfill.h"
#include "wf_qstring.h"

static char lc(char c) {
    return (c >= 'A' && c <= 'Z') ? (char)(c - 'A' + 'a') : c;
}
static int eq(const char *a, const char *b) {
    int i = 0;
    if (!a || !b) return 0;
    while (a[i] && a[i] == b[i]) i++;
    return a[i] == b[i];
}
static void scopy(char *d, int cap, const char *s) {
    int i = 0;
    if (!d || cap <= 0) return;
    if (!s) s = "";
    while (s[i] && i < cap - 1) { d[i] = s[i]; i++; }
    d[i] = 0;
}
static int app(char *o, int cap, int n, const char *s) {
    while (s && *s && n < cap - 1) o[n++] = *s++;
    if (cap > 0 && n < cap) o[n] = 0;
    return n;
}

int wf_formfill_host(const char *url, char *host, int cap) {
    int i = 0, a, b, k = 0;
    if (!host || cap <= 0) return -1;
    host[0] = 0;
    if (!url) return -1;
    while (url[i] && !(url[i] == ':' && url[i + 1] == '/' && url[i + 2] == '/')) i++;
    if (!url[i]) return -1;
    a = i + 3;
    while (url[a] && url[a] != '@') {
        if (url[a] == '/' || url[a] == '?' || url[a] == '#') break;
        a++;
    }
    if (url[a] == '@') a++;
    else a = i + 3;
    b = a;
    while (url[b] && url[b] != '/' && url[b] != '?' && url[b] != '#' && url[b] != ':') b++;
    if (b <= a) return -1;
    while (a < b && k < cap - 1) host[k++] = lc(url[a++]);
    host[k] = 0;
    return 0;
}

static int field_type_name(const wf_field_t *f, char *out, int cap) {
    if (f->itype[0]) { scopy(out, cap, f->itype); return 0; }
    if (f->type == WF_FIELD_PASSWORD) { scopy(out, cap, "password"); return 0; }
    if (f->type == WF_FIELD_HIDDEN) { scopy(out, cap, "hidden"); return 0; }
    if (f->type == WF_FIELD_TEXTAREA) { scopy(out, cap, "textarea"); return 0; }
    scopy(out, cap, "text");
    return 0;
}

static int remember_one(const wf_field_t *f) {
    char ty[12];
    if (!f->named || !f->name[0] || !f->value[0]) return 0;
    if (f->type == WF_FIELD_SUBMIT || f->type == WF_FIELD_FILE) return 0;
    if ((f->type == WF_FIELD_CHECKBOX || f->type == WF_FIELD_RADIO) && !f->checked) return 0;
    field_type_name(f, ty, (int)sizeof(ty));
    if (wf_autofill_secret(ty)) return 0;
    if (f->type == WF_FIELD_PASSWORD || f->type == WF_FIELD_HIDDEN) return 0;
    return 1;
}

int wf_formfill_offers(const wf_autofill_t *a, const wf_form_page_t *p, int field,
                       const char *host, wf_fill_offer_t *out, int cap) {
    int i, n = 0;
    char want[WF_AF_HOST];
    if (!a || !p || !out || cap <= 0 || field < 0 || field >= p->nfields) return 0;
    if (wf_formfill_host(host, want, (int)sizeof(want)) != 0 && host && host[0] && !eq(host, "")) {
        /* host may already be a bare hostname */
        scopy(want, (int)sizeof(want), host);
        for (i = 0; want[i]; i++) want[i] = lc(want[i]);
    }
    if (!want[0]) return 0;
    if (!p->field[field].name[0]) return 0;
    if (p->field[field].type == WF_FIELD_PASSWORD || p->field[field].type == WF_FIELD_HIDDEN ||
        p->field[field].type == WF_FIELD_SUBMIT || p->field[field].type == WF_FIELD_FILE)
        return 0;
    if (wf_autofill_secret(p->field[field].itype)) return 0;
    for (i = 0; i < a->n && n < cap && n < WF_FILL_OFFERS; i++) {
        if (!eq(a->e[i].host, want)) continue;
        if (!eq(a->e[i].name, p->field[field].name)) continue;
        out[n].ent = i;
        scopy(out[n].name, (int)sizeof(out[n].name), a->e[i].name);
        scopy(out[n].value, (int)sizeof(out[n].value), a->e[i].value);
        n++;
    }
    return n;
}

int wf_formfill_apply(wf_form_page_t *p, int field, const wf_fill_offer_t *o) {
    if (!p || !o || field < 0 || field >= p->nfields) return -1;
    if (p->field[field].type == WF_FIELD_PASSWORD || p->field[field].type == WF_FIELD_HIDDEN) return -1;
    if (wf_form_set(p, field, o->value) != 0) return -1;
    p->focus = field;
    return 0;
}

int wf_formfill_remember(wf_autofill_t *a, const wf_form_page_t *p, int form, const char *host) {
    int i, n = 0;
    char h[WF_AF_HOST], ty[12];
    if (!a || !p) return 0;
    if (wf_formfill_host(host, h, (int)sizeof(h)) != 0) {
        scopy(h, (int)sizeof(h), host ? host : "");
        for (i = 0; h[i]; i++) h[i] = lc(h[i]);
    }
    if (!h[0]) return 0;
    for (i = 0; i < p->nfields; i++) {
        const wf_field_t *f = &p->field[i];
        if (form >= 0 && f->form != form) continue;
        if (!remember_one(f)) continue;
        field_type_name(f, ty, (int)sizeof(ty));
        if (wf_autofill_put(a, h, f->name, f->value, ty) == 0) n++;
    }
    return n;
}

int wf_formfill_preview(const wf_form_page_t *p, int field, char *out, int cap) {
    static wf_form_submit_t sub;
    int f, submitter = -1, n = 0;
    if (!out || cap < 8) return -1;
    out[0] = 0;
    if (!p || field < 0 || field >= p->nfields) return -1;
    f = p->field[field].form;
    if (p->field[field].type == WF_FIELD_SUBMIT) submitter = field;
    if (wf_form_submit(p, f, submitter, &sub) != 0) return -1;
    n = app(out, cap, n, sub.method == WF_METHOD_POST ? "POST " : "GET ");
    n = app(out, cap, n, sub.url);
    n = app(out, cap, n, "\n");
    if (sub.method == WF_METHOD_POST && sub.body[0]) {
        static wf_qs_t qs;
        if (wf_qs_parse(sub.body, sub.body_len, &qs) != 0) return -1;
        n = app(out, cap, n, sub.body);
        n = app(out, cap, n, "\n");
    }
    return n;
}
