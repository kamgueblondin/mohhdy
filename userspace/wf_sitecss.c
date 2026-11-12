/* wf_sitecss.c - Lot 86: see wf_sitecss.h. Freestanding, no heap. */
#include "wf_sitecss.h"
#include "wf_cssmath.h"
#include "wf_cssvar.h"
#include "wf_cssbg.h"

int wf_site_apply(wf_site_t *d);

static int slen(const char *s) {
    int n = 0;
    while (s && s[n]) n++;
    return n;
}
static int eq(const char *a, const char *b) {
    int i = 0;
    if (!a || !b) return 0;
    while (a[i] && a[i] == b[i]) i++;
    return a[i] == b[i];
}
static char lc(char c) {
    return (c >= 'A' && c <= 'Z') ? (char)(c - 'A' + 'a') : c;
}
static int is_sp(char c) {
    return c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == '\f';
}
static void clear(void *p, int n) {
    unsigned char *b = (unsigned char *)p;
    int i;
    for (i = 0; i < n; i++) b[i] = 0;
}
static void scopy(char *d, int cap, const char *s, int n) {
    int i = 0;
    if (cap <= 0) return;
    while (i < n && s[i] && i < cap - 1) { d[i] = s[i]; i++; }
    d[i] = 0;
}
static void slower(char *d, int cap, const char *s, int n) {
    int i = 0;
    if (cap <= 0) return;
    while (i < n && s[i] && i < cap - 1) { d[i] = lc(s[i]); i++; }
    d[i] = 0;
}

/* ---- Lot 89: custom properties per element -------------------------- */

/* node whose properties var() reads while one declaration is parsed for
 * that element (-1: the sheet-wide values) */
static int g_vnode = -1;

static int vname_index(const wf_site_t *d, const char *name) {
    int k;
    if (!d || !name) return -1;
    for (k = 0; k < d->nvnames; k++) if (eq(d->vname[k], name)) return k;
    return -1;
}

/* registers --name (n bytes at s, lower case); full table: counted */
static void vname_add(wf_site_t *d, const char *s, int n) {
    char nm[24];
    if (n < 3 || s[0] != '-' || s[1] != '-') return;
    if (n >= (int)sizeof(nm)) { d->dropped_vars++; return; }
    slower(nm, (int)sizeof(nm), s, n);
    if (vname_index(d, nm) >= 0) return;
    if (d->nvnames >= WF_SITE_VNAMES) { d->dropped_vars++; return; }
    scopy(d->vname[d->nvnames], 24, nm, slen(nm));
    d->nvnames++;
}

/* index of (name k, value) in evar, added when new; -1 when full */
static int evar_intern(wf_site_t *d, int k, const char *val) {
    int e;
    for (e = 0; e < d->nevars; e++)
        if (eq(d->evar[e].name, d->vname[k]) && eq(d->evar[e].value, val)) return e;
    if (d->nevars >= WF_SITE_EVARS) { d->dropped_vars++; return -1; }
    scopy(d->evar[d->nevars].name, 24, d->vname[k], slen(d->vname[k]));
    scopy(d->evar[d->nevars].value, WF_SITE_VVAL, val, slen(val));
    return d->nevars++;
}

/* Lot 90: index + 1 of a long value in the pool, added when new; 0 when
 * the pool is full (counted, the declaration is left out) */
static int long_intern(wf_site_t *d, const char *val) {
    int e;
    for (e = 0; e < d->nlongs; e++) if (eq(d->lpool[e], val)) return e + 1;
    if (d->nlongs >= WF_SITE_LONGS) { d->dropped_longs++; return 0; }
    scopy(d->lpool[d->nlongs], WF_SITE_LONG, val, slen(val));
    d->nlongs++;
    return d->nlongs;
}

/* the element's value of name, 0 when it has none; *inv 1 when it is
 * guaranteed-invalid there (Lot 90: no fallback to the sheet-wide value) */
static const char *node_var_inv(const wf_site_t *d, int idx, const char *name, int *inv) {
    int k = vname_index(d, name), b;
    *inv = 0;
    if (k < 0 || idx < 0 || idx >= d->nnodes) return 0;
    b = d->node[idx].vbind[k];
    if (!b) return 0;
    if (b == WF_SITE_VINVALID || b > d->nevars) { *inv = 1; return 0; }
    return d->evar[b - 1].value;
}

/* Lot 90: the registration of a name, or 0 */
const wf_cssprop_t *wf_site_prop(const wf_site_t *d, const char *name) {
    int k;
    if (!d || !name) return 0;
    for (k = 0; k < d->nprops; k++) if (eq(d->prop[k].name, name)) return &d->prop[k];
    return 0;
}

/* the declarations "--x: v" of one block (n bytes at s), names only */
static void vnames_in(wf_site_t *d, const char *s, int n) {
    int i = 0;
    while (i < n) {
        int a, b;
        while (i < n && (is_sp(s[i]) || s[i] == ';')) i++;
        a = i;
        while (i < n && s[i] != ':' && s[i] != ';') i++;
        if (i >= n || s[i] != ':') { while (i < n && s[i] != ';') i++; continue; }
        b = i;
        while (b > a && is_sp(s[b - 1])) b--;
        if (b - a >= 3 && s[a] == '-' && s[a + 1] == '-') vname_add(d, s + a, b - a);
        while (i < n && s[i] != ';') i++;
    }
}

static int void_tag(const char *t) {
    static const char *const v[] = {
        "area", "base", "br", "col", "embed", "hr", "img", "input", "link",
        "meta", "param", "source", "track", "wbr", 0
    };
    int i;
    for (i = 0; v[i]; i++) if (eq(t, v[i])) return 1;
    return 0;
}

static int skip_comment_html(const char *s, int n, int i) {
    if (i + 3 < n && s[i + 1] == '!' && s[i + 2] == '-' && s[i + 3] == '-') {
        i += 4;
        while (i + 2 < n && !(s[i] == '-' && s[i + 1] == '-' && s[i + 2] == '>')) i++;
        return i + 3 <= n ? i + 3 : n;
    }
    return i;
}

static int tag_end(const char *s, int n, int i) {
    int q = 0;
    while (i < n && (q || s[i] != '>')) {
        if (s[i] == '"' || s[i] == '\'') q = (q == s[i]) ? 0 : (q ? q : s[i]);
        i++;
    }
    return i;
}

static int name_at(const char *s, int n, int i, char *out, int cap) {
    int k = 0;
    while (i < n && ((s[i] >= 'A' && s[i] <= 'Z') || (s[i] >= 'a' && s[i] <= 'z') ||
                     (s[i] >= '0' && s[i] <= '9') || s[i] == '-' || s[i] == ':')) {
        if (k < cap - 1) out[k++] = lc(s[i]);
        i++;
    }
    if (cap > 0) out[k] = 0;
    return i;
}

/* Attribute value starting at s[i] (the first value byte, quote already
 * consumed). Writes into out and returns the index after the value. */
static int attr_value(const char *s, int n, int i, int end, char *out, int cap) {
    (void)n;
    int q = 0, k = 0;
    if (i < end && (s[i] == '"' || s[i] == '\'')) { q = s[i]; i++; }
    while (i < end) {
        if (q) { if (s[i] == q) { i++; break; } }
        else if (is_sp(s[i]) || s[i] == '>' || s[i] == '/') break;
        if (k < cap - 1) out[k++] = s[i];
        i++;
    }
    if (cap > 0) out[k] = 0;
    return i;
}

static void take_attr(wf_site_node_t *nd, const char *name, const char *val) {
    if (eq(name, "id") && !nd->id[0]) { scopy(nd->id, (int)sizeof(nd->id), val, slen(val)); return; }
    if (eq(name, "class")) {
        int i = 0;
        while (val[i] && nd->ncls < WF_SITE_CLS) {
            int a;
            while (val[i] && is_sp(val[i])) i++;
            a = i;
            while (val[i] && !is_sp(val[i])) i++;
            if (i > a && nd->ncls < WF_SITE_CLS) {
                scopy(nd->cls[nd->ncls], (int)sizeof(nd->cls[0]), val + a, i - a);
                nd->ncls++;
            }
        }
        return;
    }
    if (eq(name, "style") && !nd->style[0]) { scopy(nd->style, (int)sizeof(nd->style), val, slen(val)); return; }
    if (eq(name, "hidden")) { nd->hidden_attr = 1; return; }
    if (nd->nattr < WF_SITE_ATTR && (eq(name, "name") || eq(name, "type") || eq(name, "href") || eq(name, "role") ||
                                     eq(name, "lang") || eq(name, "rel") || eq(name, "title") || eq(name, "dir") ||
                                     eq(name, "aria-hidden") || eq(name, "media") ||
                                     (name[0] == 'd' && name[1] == 'a' && name[2] == 't' && name[3] == 'a' && name[4] == '-'))) {
        scopy(nd->aname[nd->nattr], (int)sizeof(nd->aname[0]), name, slen(name));
        scopy(nd->aval[nd->nattr], (int)sizeof(nd->aval[0]), val, slen(val));
        nd->nattr++;
    }
}

static void read_attrs(wf_site_node_t *nd, const char *s, int from, int end) {
    int i = from;
    while (i < end) {
        char name[16], val[128];
        while (i < end && (is_sp(s[i]) || s[i] == '/')) i++;
        if (i >= end || s[i] == '>') break;
        i = name_at(s, end, i, name, (int)sizeof(name));
        if (!name[0]) { i++; continue; }
        while (i < end && is_sp(s[i])) i++;
        val[0] = 0;
        if (i < end && s[i] == '=') {
            i++;
            while (i < end && is_sp(s[i])) i++;
            i = attr_value(s, end, i, end, val, (int)sizeof(val));
        }
        take_attr(nd, name, val);
    }
}

static int find_end_tag(const char *s, int n, int i, const char *tag) {
    int tl = slen(tag);
    while (i < n) {
        if (s[i] == '<' && i + 1 < n && s[i + 1] == '/') {
            int k = i + 2, j = 0;
            while (k < n && j < tl && lc(s[k]) == tag[j]) { k++; j++; }
            if (j == tl && (k >= n || s[k] == '>' || is_sp(s[k]))) {
                while (k < n && s[k] != '>') k++;
                return k < n ? k + 1 : n;
            }
        }
        i++;
    }
    return n;
}

static int css_append(wf_site_t *d, const char *s, int a, int b) {
    while (a < b) {
        if (d->css_n + 1 >= WF_SITE_CSS) return -1;
        d->css[d->css_n++] = s[a++];
    }
    if (d->css_n + 1 < WF_SITE_CSS) d->css[d->css_n++] = '\n';
    d->css[d->css_n] = 0;
    return 0;
}

static void copy_part(wf_site_part_t *d, const wf_site_part_t *s) {
    const unsigned char *a = (const unsigned char *)s;
    unsigned char *b = (unsigned char *)d;
    int i;
    for (i = 0; i < (int)sizeof(*d); i++) b[i] = a[i];
}

static void link_child(wf_site_t *d, int parent, int idx) {
    wf_site_node_t *nd = &d->node[idx];
    nd->parent = parent;
    nd->first = nd->last = nd->next = nd->prev = -1;
    if (parent < 0) {
        int k, prev = -1, count = 0;
        for (k = 0; k < idx; k++) if (d->node[k].parent < 0) { prev = k; count++; }
        nd->child_i = count + 1;
        nd->prev = prev;
        if (prev >= 0) d->node[prev].next = idx;
        nd->nsib = count + 1;
        return;
    }
    {
        wf_site_node_t *p = &d->node[parent];
        if (p->first < 0) p->first = idx;
        if (p->last >= 0) {
            d->node[p->last].next = idx;
            nd->prev = p->last;
        }
        p->last = idx;
        nd->child_i = (p->first == idx) ? 1 : d->node[nd->prev].child_i + 1;
    }
}

static void finish_counts(wf_site_t *d) {
    int i;
    for (i = 0; i < d->nnodes; i++) {
        wf_site_node_t *nd = &d->node[i];
        int k, ti = 0, nt = 0;
        /* siblings share the parent; a root node's siblings are the other roots */
        for (k = 0; k < d->nnodes; k++) {
            if (d->node[k].parent != nd->parent) continue;
            if (!eq(d->node[k].tag, nd->tag)) continue;
            nt++;
            if (k <= i) ti++;
        }
        nd->type_i = ti;
        nd->ntype = nt;
        if (nd->parent >= 0) d->node[nd->parent].nkids++;
    }
    for (i = 0; i < d->nnodes; i++) {
        int p = d->node[i].parent;
        int count = 0, c;
        if (p < 0) {
            int k;
            for (k = 0; k < d->nnodes; k++) if (d->node[k].parent < 0) count++;
            d->node[i].nsib = count;
        } else {
            for (c = d->node[p].first; c >= 0; c = d->node[c].next) count++;
            d->node[i].nsib = count;
        }
    }
}

static void add_link(wf_site_t *d, const char *href, int hn, const char *media, int mn, int from_import) {
    wf_site_link_t *l;
    int k;
    if (!href || hn <= 0) return;
    for (k = 0; k < d->nlinks; k++) {
        int j = 0;
        while (j < hn && d->link[k].href[j] == href[j]) j++;
        if (j == hn && !d->link[k].href[j]) return;   /* same sheet twice */
    }
    if (d->nlinks >= WF_SITE_LINKS || hn >= WF_SITE_HREF) { d->dropped_links++; return; }
    l = &d->link[d->nlinks++];
    scopy(l->href, (int)sizeof(l->href), href, hn);
    scopy(l->media, (int)sizeof(l->media), media ? media : "", media ? mn : 0);
    l->from_import = (uint8_t)(from_import ? 1 : 0);
}

/* <link rel=stylesheet href=... media=...>: rel is a word list. */
static void scan_link(wf_site_t *d, const char *s, int from, int end) {
    int i = from, sheet = 0, alt = 0;
    char href[WF_SITE_HREF], media[32];
    href[0] = media[0] = 0;
    while (i < end) {
        char name[16], val[WF_SITE_HREF];
        while (i < end && (is_sp(s[i]) || s[i] == '/')) i++;
        if (i >= end || s[i] == '>') break;
        i = name_at(s, end, i, name, (int)sizeof(name));
        if (!name[0]) { i++; continue; }
        while (i < end && is_sp(s[i])) i++;
        val[0] = 0;
        if (i < end && s[i] == '=') {
            i++;
            while (i < end && is_sp(s[i])) i++;
            i = attr_value(s, end, i, end, val, (int)sizeof(val));
        }
        if (eq(name, "rel")) {
            int p = 0;
            while (val[p]) {
                char w[16];
                int a;
                while (val[p] && is_sp(val[p])) p++;
                a = p;
                while (val[p] && !is_sp(val[p])) p++;
                slower(w, (int)sizeof(w), val + a, p - a);
                if (eq(w, "stylesheet")) sheet = 1;
                if (eq(w, "alternate")) alt = 1;
            }
        } else if (eq(name, "href")) scopy(href, (int)sizeof(href), val, slen(val));
        else if (eq(name, "media")) scopy(media, (int)sizeof(media), val, slen(val));
    }
    /* an alternate stylesheet is not applied by default (HTML) */
    if (sheet && !alt && href[0]) add_link(d, href, slen(href), media, slen(media), 0);
}

int wf_site_prepare_vw(wf_site_t *d, const char *html, int n, int vw) {
    return wf_site_prepare_ext(d, html, n, vw, 0, 0, 0);
}

int wf_site_prepare_ext(wf_site_t *d, const char *html, int n, int vw, int vh, const char *ext, int ext_n) {
    int i = 0, parent = -1;
    if (!d) return -1;
    clear(d, (int)sizeof(*d));
    d->vw = vw > 0 ? vw : 640;
    d->vh = vh > 0 ? vh : 480;
    if (!html || n < 0) n = 0;
    if (ext && ext_n > 0) {
        if (css_append(d, ext, 0, ext_n) != 0) d->errors++;
        d->ext_n = d->css_n;
    }
    while (i < n) {
        char tag[12];
        int end, close, self;
        if (html[i] != '<') {
            if (parent >= 0 && !is_sp(html[i])) d->node[parent].has_text = 1;
            i++;
            continue;
        }
        if (i + 3 < n && html[i + 1] == '!' && html[i + 2] == '-' ) {
            i = skip_comment_html(html, n, i);
            continue;
        }
        if (i + 1 < n && (html[i + 1] == '!' || html[i + 1] == '?')) {
            end = tag_end(html, n, i + 2);
            i = end < n ? end + 1 : n;
            continue;
        }
        close = (i + 1 < n && html[i + 1] == '/');
        end = tag_end(html, n, i + 1 + close);
        name_at(html, n, i + 1 + close, tag, (int)sizeof(tag));
        self = (end > i && html[end - 1] == '/');
        if (!tag[0]) { i++; continue; }
        if (close) {
            while (parent >= 0 && !eq(d->node[parent].tag, tag)) parent = d->node[parent].parent;
            if (parent >= 0) parent = d->node[parent].parent;
            i = end < n ? end + 1 : n;
            continue;
        }
        if (eq(tag, "style")) {
            int body = end < n ? end + 1 : n;
            int after = find_end_tag(html, n, body, "style");
            int stop = after;
            /* drop the closing tag from the copied text */
            if (after > body) {
                int k = after - 1;
                while (k > body && html[k - 1] != '<') k--;
                if (k > body && html[k - 1] == '<') stop = k - 1;
            }
            {
                int from = d->css_n;
                if (css_append(d, html, body, stop) != 0) d->errors++;
                /* Lot 88: @scope without a prelude uses the parent element
                 * of the <style> block as its root */
                if (d->nstyles < WF_SITE_STYLES) {
                    d->style_from[d->nstyles] = from;
                    d->style_to[d->nstyles] = d->css_n;
                    d->style_owner[d->nstyles] = parent;
                    d->nstyles++;
                }
            }
            i = after;
            continue;
        }
        if (eq(tag, "link")) scan_link(d, html, i + 1, end);
        if (eq(tag, "script") || eq(tag, "noscript")) {
            int body = end < n ? end + 1 : n;
            i = find_end_tag(html, n, body, tag);
            continue;
        }
        if (d->nnodes >= WF_SITE_NODES) {
            d->dropped_nodes++;
            i = end < n ? end + 1 : n;
            if (!void_tag(tag) && !self) i = find_end_tag(html, n, i, tag);
            continue;
        }
        {
            int idx = d->nnodes++;
            wf_site_node_t *nd = &d->node[idx];
            clear(nd, (int)sizeof(*nd));
            scopy(nd->tag, (int)sizeof(nd->tag), tag, slen(tag));
            nd->start = i;
            read_attrs(nd, html, i + 1, end);
            link_child(d, parent, idx);
            if (!void_tag(tag) && !self) parent = idx;
        }
        i = end < n ? end + 1 : n;
    }
    finish_counts(d);
    return wf_site_apply(d);
}

int wf_site_nlinks(const wf_site_t *d) { return d ? d->nlinks : 0; }
const wf_site_link_t *wf_site_link_at(const wf_site_t *d, int i) {
    if (!d || i < 0 || i >= d->nlinks) return 0;
    return &d->link[i];
}

int wf_site_prepare(wf_site_t *d, const char *html, int n) {
    return wf_site_prepare_vw(d, html, n, 640);
}

/* The apply step is below the parser. A forward declaration keeps the
 * builder above the cascade. */
int wf_site_apply(wf_site_t *d);

static int ident(const char *s, int n, int i, char *out, int cap) {
    int k = 0;
    if (i < n && s[i] == '*') {
        if (cap > 0) out[0] = 0;
        return i + 1;
    }
    while (i < n && ((s[i] >= 'A' && s[i] <= 'Z') || (s[i] >= 'a' && s[i] <= 'z') ||
                     (s[i] >= '0' && s[i] <= '9') || s[i] == '-' || s[i] == '_')) {
        if (k < cap - 1) out[k++] = lc(s[i]);
        i++;
    }
    if (cap > 0) out[k] = 0;
    return i;
}

static int hexdig(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
}

static int parse_color(const char *s, int n, uint32_t *out) {
    int i = 0;
    while (i < n && is_sp(s[i])) i++;
    if (i < n && s[i] == '#') {
        int k = 0, v[8];
        i++;
        while (i < n && k < 8 && hexdig(s[i]) >= 0) v[k++] = hexdig(s[i++]);
        if (k == 3) {
            *out = (uint32_t)((v[0] * 17 << 16) | (v[1] * 17 << 8) | (v[2] * 17));
            return 1;
        }
        if (k == 6) {
            *out = (uint32_t)((v[0] << 20) | (v[1] << 16) | (v[2] << 12) | (v[3] << 8) | (v[4] << 4) | v[5]);
            return 1;
        }
        return 0;
    }
    {
        char name[16];
        int k = 0;
        static const char *const nm[] = { "black", "white", "red", "green", "blue", "gray", "grey", "navy", 0 };
        static const uint32_t cv[] = { 0x000000u, 0xfffffeu, 0xff0000u, 0x008000u, 0x0000ffu, 0x808080u, 0x808080u, 0x000080u };
        while (i < n && k < 15 && !is_sp(s[i]) && s[i] != ';' && s[i] != '!') name[k++] = lc(s[i++]);
        name[k] = 0;
        /* white is 0xfffffe so a stored 0 is still "unset" only via color_set */
        for (k = 0; nm[k]; k++) if (eq(name, nm[k])) { *out = cv[k] == 0xfffffeu ? 0xffffffu : cv[k]; return 1; }
    }
    return 0;
}

static int nth_match(const wf_site_part_t *pt, int index) {
    int a = (int)pt->nth_a, b = pt->nth_b;
    if (pt->nth_b_neg) b = -b;
    if (index <= 0) return 0;
    if (a == 0) return index == b;
    if ((index - b) % a) return 0;
    return (index - b) / a >= 0;
}

static int has_class(const wf_site_node_t *nd, const char *c) {
    int i;
    for (i = 0; i < nd->ncls; i++) if (eq(nd->cls[i], c)) return 1;
    return 0;
}

static char fold(char c, int ci) { return ci ? lc(c) : c; }

/* Value tests of [a=v], [a^=v], [a$=v], [a*=v], [a|=v]. */
static int attr_value_match(const char *have, const char *want, int kind, int ci) {
    int hl = slen(have), wl = slen(want), i, k;
    if (kind == WF_SITE_ATTR_EQ) {
        if (hl != wl) return 0;
        for (i = 0; i < hl; i++) if (fold(have[i], ci) != fold(want[i], ci)) return 0;
        return 1;
    }
    if (kind == WF_SITE_ATTR_PREFIX || kind == WF_SITE_ATTR_DASH) {
        if (!wl || hl < wl) return 0;
        for (i = 0; i < wl; i++) if (fold(have[i], ci) != fold(want[i], ci)) return 0;
        if (kind == WF_SITE_ATTR_DASH) return hl == wl || have[wl] == '-';
        return 1;
    }
    if (kind == WF_SITE_ATTR_SUFFIX) {
        if (!wl || hl < wl) return 0;
        for (i = 0; i < wl; i++) if (fold(have[hl - wl + i], ci) != fold(want[i], ci)) return 0;
        return 1;
    }
    if (kind == WF_SITE_ATTR_SUBSTR) {
        if (!wl) return 0;
        for (i = 0; i + wl <= hl; i++) {
            for (k = 0; k < wl && fold(have[i + k], ci) == fold(want[k], ci); k++) {}
            if (k == wl) return 1;
        }
        return 0;
    }
    if (kind == WF_SITE_ATTR_WORD) {
        int p = 0;
        if (!wl) return 0;
        while (have[p]) {
            int a = p;
            while (have[p] && !is_sp(have[p])) p++;
            if (p - a == wl) {
                for (k = 0; k < wl && fold(have[a + k], ci) == fold(want[k], ci); k++) {}
                if (k == wl) return 1;
            }
            while (have[p] && is_sp(have[p])) p++;
        }
        return 0;
    }
    return 0;
}

/* The class list as one space-separated string (for [class^=..] etc.). */
static void class_string(const wf_site_node_t *nd, char *out, int cap) {
    int i, n = 0, k;
    out[0] = 0;
    for (i = 0; i < nd->ncls; i++) {
        if (i && n < cap - 1) out[n++] = ' ';
        for (k = 0; nd->cls[i][k] && n < cap - 1; k++) out[n++] = nd->cls[i][k];
    }
    out[n] = 0;
}

static int has_attr(const wf_site_node_t *nd, const wf_site_part_t *pt) {
    int i;
    if (!pt->attr_kind) return 1;
    if (pt->attr_kind >= WF_SITE_ATTR_PREFIX || pt->attr_ci) {
        /* the newer operators share one value test for every attribute */
        char cs[WF_SITE_CLS * 13];
        const char *have = 0;
        if (eq(pt->aname, "id")) have = nd->id[0] ? nd->id : 0;
        else if (eq(pt->aname, "class")) { class_string(nd, cs, (int)sizeof(cs)); have = nd->ncls ? cs : 0; }
        else for (i = 0; i < nd->nattr; i++) if (eq(nd->aname[i], pt->aname)) { have = nd->aval[i]; break; }
        if (!have) return 0;
        if (pt->attr_kind == WF_SITE_ATTR_HAS) return 1;
        return attr_value_match(have, pt->aval, pt->attr_kind, pt->attr_ci);
    }
    if (eq(pt->aname, "id")) {
        if (!nd->id[0]) return 0;
        if (pt->attr_kind == 1) return 1;
        return eq(nd->id, pt->aval);
    }
    if (eq(pt->aname, "class")) {
        if (pt->attr_kind == 1) return nd->ncls > 0;
        if (pt->attr_kind == 3) return has_class(nd, pt->aval);
        return has_class(nd, pt->aval);
    }
    if (eq(pt->aname, "hidden")) return nd->hidden_attr;
    for (i = 0; i < nd->nattr; i++) {
        if (!eq(nd->aname[i], pt->aname)) continue;
        if (pt->attr_kind == 1) return 1;
        if (pt->attr_kind == 2) return eq(nd->aval[i], pt->aval);
        if (pt->attr_kind == 3) {
            int p = 0;
            const char *v = nd->aval[i];
            int wl = slen(pt->aval);
            while (v[p]) {
                int a = p;
                while (v[p] && !is_sp(v[p])) p++;
                if (p - a == wl) {
                    int k = 0;
                    while (k < wl && v[a + k] == pt->aval[k]) k++;
                    if (k == wl) return 1;
                }
                while (v[p] && is_sp(v[p])) p++;
            }
        }
    }
    return 0;
}

static int nth_last_index(const wf_site_node_t *nd) { return nd->nsib - nd->child_i + 1; }

static int not_match(const wf_site_node_t *nd, const wf_site_part_t *pt) {
    /* 1 when the :not() argument matches this node (so the part fails) */
    if (!pt->not_tag[0]) return 0;
    if (pt->not_kind == 1) return has_class(nd, pt->not_tag);
    if (pt->not_kind == 2) return eq(nd->id, pt->not_tag);
    if (pt->not_kind == 3) {
        int i;
        if (eq(pt->not_tag, "hidden")) return nd->hidden_attr;
        if (eq(pt->not_tag, "id")) return nd->id[0] != 0;
        if (eq(pt->not_tag, "class")) return nd->ncls > 0;
        for (i = 0; i < nd->nattr; i++) if (eq(nd->aname[i], pt->not_tag)) return 1;
        return 0;
    }
    return eq(nd->tag, pt->not_tag);
}

static int alt_match(const wf_site_node_t *nd, const wf_site_part_t *pt) {
    int k;
    for (k = 0; k < pt->nalt && k < WF_SITE_ALT; k++) {
        if (pt->alt_tag[k][0] && !eq(pt->alt_tag[k], nd->tag)) continue;
        if (pt->alt_cls[k][0] && !has_class(nd, pt->alt_cls[k])) continue;
        return 1;
    }
    return 0;
}

/* sroot: the scoping root :scope and & stand for (Lot 88); -2 outside
 * any @scope, where :scope is :root as in Selectors 4. */
static int part_match(const wf_site_t *d, int idx, const wf_site_part_t *pt, int sroot) {
    const wf_site_node_t *nd;
    int i;
    if (idx < 0 || idx >= d->nnodes) return 0;
    nd = &d->node[idx];
    if (pt->scope_ref) {
        if (sroot >= 0 && idx != sroot) return 0;
        if (sroot < 0 && (nd->parent >= 0 || !eq(nd->tag, "html"))) return 0;
    }
    if (pt->tag[0] && !eq(pt->tag, nd->tag)) return 0;
    if (pt->id[0] && !eq(pt->id, nd->id)) return 0;
    for (i = 0; i < pt->ncls; i++) if (!has_class(nd, pt->cls[i])) return 0;
    if (!has_attr(nd, pt)) return 0;
    if (pt->pseudo == WF_SITE_PS_FIRST && nd->child_i != 1) return 0;
    if (pt->pseudo == WF_SITE_PS_LAST && nd->child_i != nd->nsib) return 0;
    if (pt->pseudo == WF_SITE_PS_LINK && !eq(nd->tag, "a")) return 0;
    if (pt->pseudo == WF_SITE_PS_NOT && not_match(nd, pt)) return 0;
    if (pt->pseudo == WF_SITE_PS_NTH && !nth_match(pt, nd->child_i)) return 0;
    if (pt->pseudo == WF_SITE_PS_FIRST_TYPE && nd->type_i != 1) return 0;
    if (pt->pseudo == WF_SITE_PS_LAST_TYPE && nd->type_i != nd->ntype) return 0;
    if (pt->pseudo == WF_SITE_PS_ONLY && nd->nsib != 1) return 0;
    if (pt->pseudo == WF_SITE_PS_ONLY_TYPE && nd->ntype != 1) return 0;
    if (pt->pseudo == WF_SITE_PS_NTH_LAST && !nth_match(pt, nth_last_index(nd))) return 0;
    if (pt->pseudo == WF_SITE_PS_NTH_TYPE && !nth_match(pt, nd->type_i)) return 0;
    if (pt->pseudo == WF_SITE_PS_NTH_LAST_TYPE && !nth_match(pt, nd->ntype - nd->type_i + 1)) return 0;
    if (pt->pseudo == WF_SITE_PS_EMPTY && (nd->nkids || nd->has_text)) return 0;
    if (pt->pseudo == WF_SITE_PS_ROOT && (nd->parent >= 0 || !eq(nd->tag, "html"))) return 0;
    if ((pt->pseudo == WF_SITE_PS_IS || pt->pseudo == WF_SITE_PS_WHERE) && !alt_match(nd, pt)) return 0;
    return 1;
}

/* A compound chain (subject last). *left gets the node the first
 * compound matched, for the relative rules of @scope. */
static int chain_match(const wf_site_t *d, int idx, const wf_site_part_t *part, int nparts, int sroot, int *left) {
    int k, cur;
    if (nparts <= 0) return 0;
    if (!part_match(d, idx, &part[nparts - 1], sroot)) return 0;
    cur = idx;
    for (k = nparts - 1; k > 0; k--) {
        uint8_t rel = part[k].rel;
        if (rel == WF_SITE_REL_CHILD) {
            cur = cur >= 0 ? d->node[cur].parent : -1;
            if (!part_match(d, cur, &part[k - 1], sroot)) return 0;
        } else if (rel == WF_SITE_REL_DESC) {
            cur = cur >= 0 ? d->node[cur].parent : -1;
            while (cur >= 0 && !part_match(d, cur, &part[k - 1], sroot)) cur = d->node[cur].parent;
            if (cur < 0) return 0;
        } else if (rel == WF_SITE_REL_ADJ) {
            cur = cur >= 0 ? d->node[cur].prev : -1;
            if (!part_match(d, cur, &part[k - 1], sroot)) return 0;
        } else {
            cur = cur >= 0 ? d->node[cur].prev : -1;
            while (cur >= 0 && !part_match(d, cur, &part[k - 1], sroot)) cur = d->node[cur].prev;
            if (cur < 0) return 0;
        }
    }
    if (left) *left = cur;
    return 1;
}

/* ---- @scope (Lot 88) ------------------------------------------------- */

static int is_ancestor(const wf_site_t *d, int anc, int idx) {
    int guard = 0;
    if (anc < 0 || idx < 0) return 0;
    idx = d->node[idx].parent;
    while (idx >= 0 && guard++ < WF_SITE_NODES) {
        if (idx == anc) return 1;
        idx = d->node[idx].parent;
    }
    return 0;
}

static int scope_in(const wf_site_t *d, int sc, int root, int idx, int depth);

/* Lot 89: a start or end list matches when one of its selectors does */
static int list_match(const wf_site_t *d, int idx, const wf_site_part_t list[][WF_SITE_PARTS],
                      const uint8_t *len, int nsel, int sroot) {
    int k;
    for (k = 0; k < nsel && k < WF_SITE_SCOPE_ALTS; k++)
        if (chain_match(d, idx, list[k], len[k], sroot, 0)) return 1;
    return 0;
}

/* root is a scoping root of scope sc: it matches start (or is the owner
 * of an implicit scope) and, for a nested @scope, sits in a scope of the
 * enclosing one, whose root is the :scope of start. */
static int scope_root_ok(const wf_site_t *d, int sc, int root, int depth) {
    const wf_site_scope_t *s = &d->scope[sc];
    if (root < 0 || root >= d->nnodes || depth > WF_SITE_SCOPE_DEPTH) return 0;
    if (s->parent >= 0) {
        int outer = root, guard = 0;
        /* the nearest enclosing root that holds this one */
        while (outer >= 0 && guard++ < WF_SITE_NODES) {
            if (scope_root_ok(d, s->parent, outer, depth + 1) && scope_in(d, s->parent, outer, root, depth + 1)) {
                if (s->nstart) {
                    if (list_match(d, root, s->start, s->start_len, s->nstart, outer)) return 1;
                } else if (root == outer) return 1;
            }
            outer = d->node[outer].parent;
        }
        return 0;
    }
    if (s->nstart) return list_match(d, root, s->start, s->start_len, s->nstart, -2);
    if (s->owner >= 0) return root == s->owner;
    return d->node[root].parent < 0;     /* the document: its top elements */
}

/* idx is in the scope that root opens: root itself or a descendant, and
 * neither a limit (a descendant of root matching end) nor under one. */
static int scope_in(const wf_site_t *d, int sc, int root, int idx, int depth) {
    const wf_site_scope_t *s = &d->scope[sc];
    int cur = idx, guard = 0;
    if (depth > WF_SITE_SCOPE_DEPTH) return 0;
    if (idx != root && !is_ancestor(d, root, idx)) return 0;
    if (!s->nend) return 1;
    while (cur >= 0 && cur != root && guard++ < WF_SITE_NODES) {
        if (list_match(d, cur, s->end, s->end_len, s->nend, root)) return 0;
        cur = d->node[cur].parent;
    }
    return 1;
}

/* the leftmost compound of a relative rule against its root */
static int rel_ok(const wf_site_t *d, int rel, int left, int root) {
    int cur, guard = 0;
    switch (rel) {
    case WF_SITE_SR_NONE: return 1;
    case WF_SITE_SR_DESC: return is_ancestor(d, root, left);
    case WF_SITE_SR_CHILD: return left >= 0 && d->node[left].parent == root;
    case WF_SITE_SR_ADJ: return left >= 0 && d->node[left].prev == root;
    case WF_SITE_SR_SIB:
        cur = left >= 0 ? d->node[left].prev : -1;
        while (cur >= 0 && guard++ < WF_SITE_NODES) { if (cur == root) return 1; cur = d->node[cur].prev; }
        return 0;
    default: return 0;
    }
}

/* Scoped rule: the nearest root (walking up from the subject) that
 * opens a scope holding the subject and makes the selector match.
 * Returns the root, -1 when none; *hops gets the generations between. */
static int scoped_root(const wf_site_t *d, int idx, const wf_site_rule_t *r, int *hops) {
    int sc = (int)r->scope - 1, root = idx, h = 0;
    if (sc < 0 || sc >= d->nscopes) return -1;
    while (root >= 0 && h < WF_SITE_NODES) {
        if (scope_root_ok(d, sc, root, 0) && scope_in(d, sc, root, idx, 0)) {
            int left = -1;
            if (chain_match(d, idx, r->part, r->nparts, root, &left) && rel_ok(d, r->scope_rel, left, root)) {
                if (hops) *hops = h;
                return root;
            }
        }
        root = d->node[root].parent;
        h++;
    }
    return -1;
}

/* 0 no match, else 1 + proximity: 0 for an unscoped rule (infinite
 * hops, the weakest), 15 - hops (at least 1) for a scoped one (Lot 90:
 * 15 - hops, hops capped at 14; Lot 88: 7 - hops, capped at 6). */
static int rule_match(const wf_site_t *d, int idx, const wf_site_rule_t *r) {
    int hops = 0;
    if (!r->scope) return chain_match(d, idx, r->part, r->nparts, -2, 0) ? 1 : 0;
    if (scoped_root(d, idx, r, &hops) < 0) return 0;
    if (hops > WF_SITE_SCOPE_HOPS) hops = WF_SITE_SCOPE_HOPS;
    return 1 + 15 - hops;
}

int wf_site_scope_root(const wf_site_t *d, int rule, int idx) {
    if (!d || rule < 0 || rule >= d->nrules || idx < 0 || idx >= d->nnodes) return -1;
    if (!d->rule[rule].scope) return -2;
    return scoped_root(d, idx, &d->rule[rule], 0);
}

static int specificity(const wf_site_rule_t *r) {
    int s = 0, p;
    for (p = 0; p < r->nparts; p++) {
        const wf_site_part_t *pt = &r->part[p];
        if (pt->id[0]) s += 10000;
        s += pt->ncls * 100;
        if (pt->attr_kind) s += 100;
        if (pt->pseudo == WF_SITE_PS_NOT) {
            /* :not() takes the specificity of its argument */
            if (pt->not_kind == 2) s += 10000;
            else if (pt->not_kind == 1 || pt->not_kind == 3) s += 100;
            else s += pt->not_tag[0] ? 1 : 0;
        } else if (pt->pseudo == WF_SITE_PS_WHERE) {
            /* :where() adds nothing */
        } else if (pt->pseudo == WF_SITE_PS_IS) {
            /* the most specific alternative */
            int k, best = 0;
            for (k = 0; k < pt->nalt && k < WF_SITE_ALT; k++) {
                int v = (pt->alt_cls[k][0] ? 100 : 0) + (pt->alt_tag[k][0] ? 1 : 0);
                if (v > best) best = v;
            }
            s += best;
        } else if (pt->pseudo) s += 100;
        if (pt->tag[0]) s += 1;
        /* Lot 89: :scope counts as a pseudo-class, & (:where(:scope)) as nothing */
        if (pt->scope_ref == WF_SITE_SREF_SCOPE) s += 100;
    }
    return s;
}

/* Lot 87: the specificity packed so that order never outweighs it (Lot 86
 * added order & 1023 to spec * 10, so a later rule beat one more type
 * selector): ids 0..7, classes 0..31, types 0..31 -> 0..8191. */
static int spec_packed(int spec) {
    int ids = spec / 10000, cls = (spec / 100) % 100, tags = spec % 100;
    if (ids > 7) ids = 7;
    if (cls > 31) cls = 31;
    if (tags > 31) tags = 31;
    return ids * 1024 + cls * 32 + tags;
}

/* Lot 87: the layer step of the cascade (CSS Cascade 5). Normal
 * declarations: later layers beat earlier ones and unlayered beats every
 * layer. Important ones: the reverse, earlier layers win and every layer
 * beats unlayered. 0..15. */
static int layer_step(const wf_site_rule_t *r) {
    int k = r->layer;                  /* 0 unlayered, else 1..WF_SITE_LAYERS */
    if (k > 14) k = 14;
    if (!r->imp) return k ? k : 15;
    return k ? 15 - k : 0;
}

static int rank_of(const wf_site_rule_t *r, int prox) {
    /* important, then origin (inline > author > ua), then layer, then
     * specificity, then (Lot 88) scope proximity 0..15 (Lot 90; 0..7
     * before), then order 0..127 (WF_SITE_RULES); fits in 31 bits: the
     * largest value, (127 * 8192 + 8191) * 2048 + 15 * 128 + 127, is
     * exactly 2^31 - 1 */
    int top = ((int)r->imp * 4 + (int)r->origin) * 16 + layer_step(r);
    if (prox < 0) prox = 0;
    if (prox > 15) prox = 15;
    return (top * 8192 + spec_packed(r->spec)) * WF_SITE_RANK_LOW + prox * 128 + (r->order & 127);
}

/* the cascade step of a rule without its specificity and order:
 * importance, origin and layer */
static int top_of(const wf_site_rule_t *r) {
    return ((int)r->imp * 4 + (int)r->origin) * 16 + layer_step(r);
}

static int *kind_rank(wf_site_node_t *nd, int kind) {
    switch (kind) {
    case 1: return &nd->disp_rank;
    case 2: return &nd->weight_rank;
    case 3: return &nd->align_rank;
    case 4: return &nd->color_rank;
    case 5: return &nd->margin_rank;
    case 6: return &nd->pad_rank;
    case WF_SITE_K_STYLE: return &nd->italic_rank;
    case WF_SITE_K_DECO: return &nd->deco_rank;
    case WF_SITE_K_VIS: return &nd->vis_rank;
    case WF_SITE_K_TRANSFORM: return &nd->transform_rank;
    case WF_SITE_K_WS: return &nd->ws_rank;
    case WF_SITE_K_BG: return &nd->bg_rank;
    case WF_SITE_K_LIST: case WF_SITE_K_FSIZE: case WF_SITE_K_INDENT: case WF_SITE_K_LSPACE:
        return &nd->tok_rank[kind - WF_SITE_K_LIST];
    case WF_SITE_K_BGIMG: case WF_SITE_K_BGPOS: case WF_SITE_K_BGSIZE: case WF_SITE_K_BGREP:
    case WF_SITE_K_LIMG: case WF_SITE_K_LPOS: case WF_SITE_K_BGCLIP: case WF_SITE_K_BGORIGIN:
        return &nd->lv_rank[kind - WF_SITE_K_BGIMG];
    default: return 0;
    }
}

/* Lot 87: forget what the cascade gave one property of a node */
static void reset_kind(wf_site_node_t *nd, int kind) {
    int k;
    switch (kind) {
    case 1: nd->disp = 0; nd->disp_imp = 0; break;
    case 2: nd->weight = 0; nd->weight_imp = 0; break;
    case 3: nd->align = 0; nd->align_imp = 0; break;
    case 4: nd->color = 0; nd->color_set = 0; nd->color_imp = 0; break;
    case 5: for (k = 0; k < 4; k++) nd->margin[k] = 0; nd->margin_set = 0; break;
    case 6: for (k = 0; k < 4; k++) nd->pad[k] = 0; nd->pad_set = 0; break;
    case WF_SITE_K_STYLE: nd->italic = 0; break;
    case WF_SITE_K_DECO: nd->deco = 0; break;
    case WF_SITE_K_VIS: nd->vis = 0; break;
    case WF_SITE_K_TRANSFORM: nd->transform = 0; break;
    case WF_SITE_K_WS: nd->ws = 0; break;
    case WF_SITE_K_BG: nd->bg = 0; nd->bg_set = 0; break;
    case WF_SITE_K_LIST: case WF_SITE_K_FSIZE: case WF_SITE_K_INDENT: case WF_SITE_K_LSPACE:
        nd->tok[kind - WF_SITE_K_LIST][0] = 0;
        nd->tok_val[kind - WF_SITE_K_LIST] = 0;
        break;
    case WF_SITE_K_BGIMG: case WF_SITE_K_BGPOS: case WF_SITE_K_BGSIZE: case WF_SITE_K_BGREP:
    case WF_SITE_K_LIMG: case WF_SITE_K_LPOS: case WF_SITE_K_BGCLIP: case WF_SITE_K_BGORIGIN:
        nd->lv[kind - WF_SITE_K_BGIMG] = 0;
        nd->lv_val[kind - WF_SITE_K_BGIMG] = 0;
        break;
    default: return;
    }
    if (kind_rank(nd, kind)) *kind_rank(nd, kind) = 0;
    nd->rv[kind] = 0;
}

static void paint_one(wf_site_t *d, int idx, const wf_site_rule_t *r, int prox) {
    wf_site_node_t *nd = &d->node[idx];
    int rank = rank_of(r, prox);
    if (r->kind == WF_SITE_K_VAR) {
        /* Lot 89: the strongest --x on this element wins, like any
         * property; inheritance is done once every element has its own */
        int k = (int)r->vname - 1, e;
        if (k < 0 || k >= d->nvnames || rank < nd->vrank[k]) return;
        e = evar_intern(d, k, r->vval);
        if (e < 0) return;
        nd->vbind[k] = (uint8_t)(e + 1);
        nd->vrank[k] = rank > 0 ? rank : 1;
        return;
    }
    if (r->revert && r->kind >= 1 && r->kind <= WF_SITE_K_MAX) {
        /* Lot 87: a winning revert-layer / revert keeps its rank (later
         * weaker rules still lose to it) and is rolled back at the end */
        int *rr = kind_rank(nd, r->kind);
        if (rr && rank >= *rr) { *rr = rank; nd->rv[r->kind] = r->revert; }
        return;
    }
    if (r->kind >= 1 && r->kind <= WF_SITE_K_MAX && kind_rank(nd, r->kind) && rank >= *kind_rank(nd, r->kind))
        nd->rv[r->kind] = 0;
    if (r->kind == 1) {
        if (rank >= nd->disp_rank) { nd->disp = r->val; nd->disp_imp = r->imp; nd->disp_rank = rank; }
    } else if (r->kind == 2) {
        if (rank >= nd->weight_rank) { nd->weight = r->val; nd->weight_imp = r->imp; nd->weight_rank = rank; }
    } else if (r->kind == 3) {
        if (rank >= nd->align_rank) { nd->align = r->val; nd->align_imp = r->imp; nd->align_rank = rank; }
    } else if (r->kind == 4) {
        if (rank >= nd->color_rank) { nd->color = r->color; nd->color_set = 1; nd->color_imp = r->imp; nd->color_rank = rank; }
    } else if (r->kind == 5 || r->kind == 6) {
        uint8_t *side = r->kind == 5 ? nd->margin : nd->pad;
        uint8_t *flag = r->kind == 5 ? &nd->margin_set : &nd->pad_set;
        int *rr = r->kind == 5 ? &nd->margin_rank : &nd->pad_rank;
        int t, ri, bo, le;
        if (rank < *rr || r->box_n < 1) return;
        if (r->box_n == 1) { t = ri = bo = le = r->box[0]; }
        else if (r->box_n == 2) { t = bo = r->box[0]; ri = le = r->box[1]; }
        else if (r->box_n == 3) { t = r->box[0]; ri = le = r->box[1]; bo = r->box[2]; }
        else { t = r->box[0]; ri = r->box[1]; bo = r->box[2]; le = r->box[3]; }
        side[0] = (uint8_t)t; side[1] = (uint8_t)ri; side[2] = (uint8_t)bo; side[3] = (uint8_t)le;
        *flag = 0x0F;
        *rr = rank;
    } else if (r->kind == WF_SITE_K_STYLE) {
        if (rank >= nd->italic_rank) { nd->italic = r->val; nd->italic_rank = rank; }
    } else if (r->kind == WF_SITE_K_DECO) {
        if (rank >= nd->deco_rank) { nd->deco = r->val; nd->deco_rank = rank; }
    } else if (r->kind == WF_SITE_K_VIS) {
        if (rank >= nd->vis_rank) { nd->vis = r->val; nd->vis_rank = rank; }
    } else if (r->kind == WF_SITE_K_TRANSFORM) {
        if (rank >= nd->transform_rank) { nd->transform = r->val; nd->transform_rank = rank; }
    } else if (r->kind == WF_SITE_K_WS) {
        if (rank >= nd->ws_rank) { nd->ws = r->val; nd->ws_rank = rank; }
    } else if (r->kind == WF_SITE_K_BG) {
        if (rank >= nd->bg_rank) { nd->bg = r->color; nd->bg_set = (uint8_t)(r->val == 2); nd->bg_rank = rank; }
    } else if (r->kind >= WF_SITE_K_LIST && r->kind <= WF_SITE_K_LSPACE) {
        int t = r->kind - WF_SITE_K_LIST;
        if (rank >= nd->tok_rank[t]) {
            scopy(nd->tok[t], WF_SITE_TOK, r->tok, slen(r->tok));
            nd->tok_val[t] = r->val;
            nd->tok_rank[t] = rank;
        }
    } else if (r->kind >= WF_SITE_K_BGIMG && r->kind <= WF_SITE_K_BGORIGIN) {
        int t = r->kind - WF_SITE_K_BGIMG;
        if (rank >= nd->lv_rank[t]) {
            nd->lv[t] = r->lidx;
            nd->lv_val[t] = r->val;
            nd->lv_rank[t] = rank;
        }
    }
}

static int kind_inherits(int kind);
static void initial_value(int kind, wf_site_rule_t *proto);

/* Lot 88: the all shorthand keeps its keyword in val (1 initial,
 * 2 inherit, 3 unset, 4 revert, 5 revert-layer); one kind of it: */
static void all_kind(const wf_site_rule_t *r, int kind, wf_site_rule_t *out) {
    int kw = r->val;
    out->imp = r->imp;
    out->origin = r->origin;
    out->layer = r->layer;
    out->spec = r->spec;
    out->order = r->order;
    out->nparts = 0;
    out->scope = r->scope;
    out->scope_rel = r->scope_rel;
    out->tok[0] = 0;
    out->lidx = 0;
    if (kw == 1 || (kw == 3 && !kind_inherits(kind))) { initial_value(kind, out); return; }
    out->kind = (uint8_t)kind;
    out->val = 0;
    out->box_n = 0;
    out->color = 0;
    out->revert = (uint8_t)(kw == 4 ? WF_SITE_RV_ORIGIN : kw == 5 ? WF_SITE_RV_LAYER : WF_SITE_RV_INHERIT);
}

/* only: 0 every kind the rule sets, else that kind alone */
static void paint_rule(wf_site_t *d, int idx, const wf_site_rule_t *r, int prox, int only) {
    if (r->kind == WF_SITE_K_ALL) {
        int k;
        for (k = 1; k <= WF_SITE_K_MAX; k++) {
            wf_site_rule_t one;
            if (only && only != k) continue;
            all_kind(r, k, &one);
            paint_one(d, idx, &one, prox);
        }
        return;
    }
    if (only && r->kind != only) return;
    paint_one(d, idx, r, prox);
}

static const wf_site_node_t *node_by_start(const wf_site_t *d, int offset) {
    int i;
    if (!d || !d->ready) return 0;
    for (i = 0; i < d->nnodes; i++) if (d->node[i].start == offset) return &d->node[i];
    return 0;
}

int wf_site_hidden_at(const wf_site_t *d, int offset) {
    const wf_site_node_t *nd = node_by_start(d, offset);
    int guard = 0, vis_seen = 0;
    while (nd && guard++ < WF_SITE_NODES) {
        if (nd->disp == WF_SITE_DISP_NONE) return 1;
        /* visibility inherits: the nearest element that sets it decides */
        if (!vis_seen && nd->vis) {
            vis_seen = 1;
            if (nd->vis == 2) return 1;
        }
        if (nd->parent < 0) break;
        nd = &d->node[nd->parent];
    }
    return 0;
}

const wf_site_node_t *wf_site_at(const wf_site_t *d, int offset) {
    return node_by_start(d, offset);
}

static int app(char *o, int cap, int n, const char *s) {
    while (*s && n < cap - 1) o[n++] = *s++;
    if (n < cap) o[n] = 0;
    return n;
}

static int app_u(char *o, int cap, int n, unsigned v) {
    char t[8];
    int k = 0;
    if (!v) return app(o, cap, n, "0");
    while (v && k < 7) { t[k++] = (char)('0' + v % 10u); v /= 10u; }
    while (k) { char c[2]; c[0] = t[--k]; c[1] = 0; n = app(o, cap, n, c); }
    return n;
}

static int app_box(char *o, int cap, int n, const uint8_t side[4]) {
    int i;
    for (i = 0; i < 4; i++) {
        if (i) n = app(o, cap, n, " ");
        n = app_u(o, cap, n, side[i]);
        n = app(o, cap, n, "px");
    }
    return n;
}

static int app_hex(char *o, int cap, int n, uint32_t c) {
    char t[8];
    int i;
    const char *h = "0123456789abcdef";
    for (i = 5; i >= 0; i--) { t[5 - i] = h[(c >> (i * 4)) & 15]; }
    t[6] = 0;
    n = app(o, cap, n, "#");
    return app(o, cap, n, t);
}

/* Lot 90: a pool kind with a value the page gave */
static int long_given(const wf_site_node_t *nd) {
    int t;
    for (t = 0; t < WF_SITE_LKINDS; t++) if (nd->lv_val[t] == 2 && nd->lv[t]) return 1;
    return 0;
}

int wf_site_style_attr(const wf_site_t *d, int offset, char *out, int cap) {
    const wf_site_node_t *nd = node_by_start(d, offset);
    int n = 0, any = 0;
    if (!out || cap < 8) return 0;
    out[0] = 0;
    if (!nd) return 0;
    if (!nd->color_set && nd->weight != 2 && nd->align < 2 && !nd->margin_set && !nd->pad_set &&
        nd->italic != 2 && nd->deco < 2 && nd->transform < 2 && nd->ws < 2 && !nd->bg_set &&
        nd->tok_val[0] != 2 && nd->tok_val[1] != 2 && nd->tok_val[2] != 2 && nd->tok_val[3] != 2 &&
        !long_given(nd)) return 0;
    n = app(out, cap, n, " style=\"");
    if (nd->color_set) { n = app(out, cap, n, "color:"); n = app_hex(out, cap, n, nd->color); any = 1; }
    if (nd->weight == 2) { if (any) n = app(out, cap, n, ";"); n = app(out, cap, n, "font-weight:bold"); any = 1; }
    if (nd->align >= 2) {
        if (any) n = app(out, cap, n, ";");
        n = app(out, cap, n, "text-align:");
        n = app(out, cap, n, nd->align == 2 ? "center" : "right");
        any = 1;
    }
    if (nd->margin_set) {
        if (any) n = app(out, cap, n, ";");
        n = app(out, cap, n, "margin:");
        n = app_box(out, cap, n, nd->margin);
        any = 1;
    }
    if (nd->pad_set) {
        if (any) n = app(out, cap, n, ";");
        n = app(out, cap, n, "padding:");
        n = app_box(out, cap, n, nd->pad);
        any = 1;
    }
    if (nd->italic == 2) {
        if (any) n = app(out, cap, n, ";");
        n = app(out, cap, n, "font-style:italic");
        any = 1;
    }
    if (nd->deco >= 2) {
        if (any) n = app(out, cap, n, ";");
        n = app(out, cap, n, nd->deco == 2 ? "text-decoration:underline" : "text-decoration:line-through");
        any = 1;
    }
    if (nd->transform >= 2 && nd->transform <= 4) {
        static const char *const tt[5] = {"", "", "uppercase", "lowercase", "capitalize"};
        if (any) n = app(out, cap, n, ";");
        n = app(out, cap, n, "text-transform:");
        n = app(out, cap, n, tt[nd->transform]);
        any = 1;
    }
    if (nd->ws >= 2 && nd->ws <= 5) {
        static const char *const wt[6] = {"", "", "pre", "nowrap", "pre-wrap", "pre-line"};
        if (any) n = app(out, cap, n, ";");
        n = app(out, cap, n, "white-space:");
        n = app(out, cap, n, wt[nd->ws]);
        any = 1;
    }
    if (nd->bg_set) {
        if (any) n = app(out, cap, n, ";");
        n = app(out, cap, n, "background-color:");
        n = app_hex(out, cap, n, nd->bg);
        any = 1;
    }
    {
        static const char *const tp[WF_SITE_TOKS] = { "list-style-type:", "font-size:", "text-indent:", "letter-spacing:" };
        int t;
        for (t = 0; t < WF_SITE_TOKS; t++) {
            if (nd->tok_val[t] != 2 || !nd->tok[t][0]) continue;
            if (any) n = app(out, cap, n, ";");
            n = app(out, cap, n, tp[t]);
            n = app(out, cap, n, nd->tok[t]);
            any = 1;
        }
    }
    {
        /* Lot 90: the pool kinds, in the order the engine reads them
         * (the image list before the per-layer lists) */
        static const char *const lp[WF_SITE_LKINDS] = { "background-image:", "background-position:", "background-size:",
                                                      "background-repeat:", "list-style-image:", "list-style-position:",
                                                      "background-clip:", "background-origin:" };
        int t;
        for (t = 0; t < WF_SITE_LKINDS; t++) {
            if (nd->lv_val[t] != 2 || !nd->lv[t] || nd->lv[t] > d->nlongs) continue;
            if (any) n = app(out, cap, n, ";");
            n = app(out, cap, n, lp[t]);
            n = app(out, cap, n, d->lpool[nd->lv[t] - 1]);
            any = 1;
        }
    }
    n = app(out, cap, n, "\"");
    return n;
}

int wf_site_report(const wf_site_t *d, char *out, int cap) {
    int n = 0, i;
    if (!out || cap < 8) return 0;
    out[0] = 0;
    if (!d || !d->ready) return app(out, cap, 0, "css none\n");
    n = app(out, cap, n, "css nodes ");
    {
        char t[12];
        int k = 0, v = d->nnodes;
        if (!v) t[k++] = '0';
        while (v && k < 11) { t[k++] = (char)('0' + v % 10); v /= 10; }
        while (k) { char c[2]; c[0] = t[--k]; c[1] = 0; n = app(out, cap, n, c); }
    }
    n = app(out, cap, n, " rules ");
    {
        char t[12];
        int k = 0, v = d->nrules;
        if (!v) t[k++] = '0';
        while (v && k < 11) { t[k++] = (char)('0' + v % 10); v /= 10; }
        while (k) { char c[2]; c[0] = t[--k]; c[1] = 0; n = app(out, cap, n, c); }
    }
    n = app(out, cap, n, "\n");
    for (i = 0; i < d->nnodes && n < cap - 48; i++) {
        const wf_site_node_t *nd = &d->node[i];
        n = app(out, cap, n, "node ");
        n = app(out, cap, n, nd->tag);
        if (nd->id[0]) { n = app(out, cap, n, " #"); n = app(out, cap, n, nd->id); }
        if (nd->ncls) { n = app(out, cap, n, " ."); n = app(out, cap, n, nd->cls[0]); }
        n = app(out, cap, n, nd->disp == WF_SITE_DISP_NONE ? " display none" :
                              nd->disp == WF_SITE_DISP_INLINE ? " display inline" :
                              nd->disp == WF_SITE_DISP_BLOCK ? " display block" : " display auto");
        if (nd->weight == 2) n = app(out, cap, n, " weight bold");
        if (nd->align == 2) n = app(out, cap, n, " align center");
        else if (nd->align == 3) n = app(out, cap, n, " align right");
        if (nd->color_set) { n = app(out, cap, n, " color "); n = app_hex(out, cap, n, nd->color); }
        if (nd->italic == 2) n = app(out, cap, n, " italic");
        if (nd->deco == 2) n = app(out, cap, n, " underline");
        else if (nd->deco == 3) n = app(out, cap, n, " line-through");
        if (nd->vis == 2) n = app(out, cap, n, " hidden");
        if (nd->transform == 2) n = app(out, cap, n, " uppercase");
        else if (nd->transform == 3) n = app(out, cap, n, " lowercase");
        else if (nd->transform == 4) n = app(out, cap, n, " capitalize");
        if (nd->ws == 2) n = app(out, cap, n, " ws-pre");
        else if (nd->ws == 3) n = app(out, cap, n, " ws-nowrap");
        else if (nd->ws == 4) n = app(out, cap, n, " ws-pre-wrap");
        else if (nd->ws == 5) n = app(out, cap, n, " ws-pre-line");
        if (nd->bg_set) { n = app(out, cap, n, " bg "); n = app_hex(out, cap, n, nd->bg); }
        {
            static const char *const tn[WF_SITE_TOKS] = { " list ", " size ", " indent ", " ls " };
            int t;
            for (t = 0; t < WF_SITE_TOKS; t++) {
                if (nd->tok_val[t] != 2) continue;
                n = app(out, cap, n, tn[t]);
                n = app(out, cap, n, nd->tok[t]);
            }
        }
        {
            static const char *const ln[WF_SITE_LKINDS] = { " bgimg ", " bgpos ", " bgsize ", " bgrep ", " limg ", " lpos ",
                                                          " bgclip ", " bgorigin " };
            int t;
            for (t = 0; t < WF_SITE_LKINDS; t++) {
                if (nd->lv_val[t] != 2 || !nd->lv[t] || nd->lv[t] > d->nlongs) continue;
                n = app(out, cap, n, ln[t]);
                n = app(out, cap, n, d->lpool[nd->lv[t] - 1]);
            }
        }
        n = app(out, cap, n, "\n");
    }
    return n;
}

/* Lot 88: "scopes N rules R hits H" */
int wf_site_vars_text(const wf_site_t *d, char *out, int cap) {
    int n = 0;
    if (!out || cap < 8) return 0;
    out[0] = 0;
    if (!d) return 0;
    n = app(out, cap, n, "names ");
    n = app_u(out, cap, n, (unsigned)d->nvnames);
    n = app(out, cap, n, " values ");
    n = app_u(out, cap, n, (unsigned)d->nevars);
    n = app(out, cap, n, " bound ");
    n = app_u(out, cap, n, (unsigned)d->var_bound);
    n = app(out, cap, n, " reread ");
    n = app_u(out, cap, n, (unsigned)d->deferred_hits);
    n = app(out, cap, n, " dropped ");
    n = app_u(out, cap, n, (unsigned)(d->dropped_vars + d->deferred_dropped));
    return n;
}

int wf_site_scope_text(const wf_site_t *d, char *out, int cap) {
    int n = 0, r, nr = 0;
    if (!out || cap < 8) return 0;
    out[0] = 0;
    if (!d) return 0;
    for (r = 0; r < d->nrules; r++) if (d->rule[r].scope) nr++;
    n = app(out, cap, n, "scopes ");
    n = app_u(out, cap, n, (unsigned)d->nscopes);
    n = app(out, cap, n, " rules ");
    n = app_u(out, cap, n, (unsigned)nr);
    n = app(out, cap, n, " hits ");
    n = app_u(out, cap, n, (unsigned)d->scoped_hits);
    if (d->dropped_scopes) {
        n = app(out, cap, n, " dropped ");
        n = app_u(out, cap, n, (unsigned)d->dropped_scopes);
    }
    return n;
}

/* ---- declaration and selector parsers --------------------------------- */

static int skip_noise(const char *s, int n, int i) {
    for (;;) {
        int p = i;
        while (i < n && is_sp(s[i])) i++;
        if (i + 1 < n && s[i] == '/' && s[i + 1] == '*') {
            i += 2;
            while (i + 1 < n && !(s[i] == '*' && s[i + 1] == '/')) i++;
            i = i + 2 <= n ? i + 2 : n;
        }
        if (i == p) return i;
    }
}

static int skip_block(const char *s, int n, int i) {
    int depth = 0;
    if (i < n && s[i] == '{') { depth = 1; i++; }
    while (i < n && depth) {
        if (s[i] == '{') depth++;
        else if (s[i] == '}') depth--;
        else if (s[i] == '"' || s[i] == '\'') {
            char q = s[i++];
            while (i < n && s[i] != q) i++;
        }
        i++;
    }
    return i;
}

static int add_rule(wf_site_t *d, const wf_site_rule_t *src) {
    wf_site_rule_t *r;
    int p, b;
    if (d->nrules >= WF_SITE_RULES) { d->dropped_rules++; return -1; }
    r = &d->rule[d->nrules];
    clear(r, (int)sizeof(*r));
    r->nparts = src->nparts;
    r->imp = src->imp;
    r->origin = src->origin;
    r->kind = src->kind;
    r->val = src->val;
    r->box_n = src->box_n;
    r->box[0] = src->box[0];
    r->box[1] = src->box[1];
    r->box[2] = src->box[2];
    r->box[3] = src->box[3];
    r->color = src->color;
    r->layer = src->layer;
    r->revert = src->revert;
    r->scope = src->scope;
    r->scope_rel = src->scope_rel;
    scopy(r->tok, WF_SITE_TOK, src->tok, slen(src->tok));
    r->vname = src->vname;
    scopy(r->vval, (int)sizeof(r->vval), src->vval, slen(src->vval));
    r->deferred = src->deferred;
    scopy(r->dprop, (int)sizeof(r->dprop), src->dprop, slen(src->dprop));
    scopy(r->draw, (int)sizeof(r->draw), src->draw, slen(src->draw));
    r->lidx = src->lidx;
    r->order = d->nrules;
    for (p = 0; p < src->nparts && p < WF_SITE_PARTS; p++) {
        const unsigned char *a = (const unsigned char *)&src->part[p];
        unsigned char *dst = (unsigned char *)&r->part[p];
        for (b = 0; b < (int)sizeof(wf_site_part_t); b++) dst[b] = a[b];
    }
    r->spec = specificity(r);
    d->nrules++;
    return 0;
}

static int display_val(const char *s) {
    if (eq(s, "none")) return WF_SITE_DISP_NONE;
    if (eq(s, "inline")) return WF_SITE_DISP_INLINE;
    if (eq(s, "block") || eq(s, "flex") || eq(s, "list-item") || eq(s, "table") || eq(s, "flow-root"))
        return WF_SITE_DISP_BLOCK;
    return 0;
}

static int weight_val(const char *s) {
    int n = 0, i = 0;
    if (eq(s, "bold") || eq(s, "bolder")) return 2;
    if (eq(s, "normal") || eq(s, "lighter")) return 1;
    while (s[i] >= '0' && s[i] <= '9') { n = n * 10 + s[i] - '0'; i++; }
    if (i && n >= 600) return 2;
    if (i && n > 0) return 1;
    return 0;
}

static int align_val(const char *s) {
    if (eq(s, "left") || eq(s, "start")) return 1;
    if (eq(s, "center")) return 2;
    if (eq(s, "right") || eq(s, "end")) return 3;
    return 0;
}

/* Lot 90: returns the length, or -1 when the value does not fit out (it
 * used to be cut); url(...) keeps its case (a path is case-sensitive) */
static int trim_word(const char *s, int n, char *out, int cap, int *imp) {
    int a = 0, b = n, k = 0, url = 0;
    *imp = 0;
    while (a < b && is_sp(s[a])) a++;
    while (b > a && is_sp(s[b - 1])) b--;
    if (b - a >= 10) {
        int p = a;
        while (p + 10 <= b) {
            if (s[p] == '!' && lc(s[p + 1]) == 'i') {
                *imp = 1;
                b = p;
                while (b > a && is_sp(s[b - 1])) b--;
                break;
            }
            p++;
        }
    }
    if (b - a > cap - 1) { out[0] = 0; return -1; }
    while (a < b) {
        if (!url && a + 4 <= b && lc(s[a]) == 'u' && lc(s[a + 1]) == 'r' && lc(s[a + 2]) == 'l' && s[a + 3] == '(' &&
            (a == 0 || !((lc(s[a - 1]) >= 'a' && lc(s[a - 1]) <= 'z') || s[a - 1] == '-'))) {
            int k2;
            for (k2 = 0; k2 < 4; k2++) out[k++] = lc(s[a++]);
            url = 1;
            continue;
        }
        if (url) { if (s[a] == ')') url = 0; out[k++] = s[a++]; continue; }
        out[k++] = lc(s[a++]);
    }
    out[k] = 0;
    return k;
}

static int parse_box(const char *s, wf_site_rule_t *proto) {
    int i = 0, n = 0;
    while (s[i] && n < 4) {
        int v = 0, any = 0;
        while (is_sp(s[i])) i++;
        if (!s[i]) break;
        while (s[i] >= '0' && s[i] <= '9') { v = v * 10 + (s[i] - '0'); i++; any = 1; }
        if (!any) return 0;
        if (v > 255) v = 255;
        if (s[i] == 'p' && s[i + 1] == 'x') i += 2;
        proto->box[n++] = (uint8_t)v;
    }
    proto->box_n = (uint8_t)n;
    return n > 0;
}

static int style_val(const char *s) {
    if (eq(s, "italic") || eq(s, "oblique")) return 2;
    if (eq(s, "normal")) return 1;
    return 0;
}

/* text-decoration and text-decoration-line: the first line keyword */
static int deco_val(const char *s) {
    int i = 0;
    while (s[i]) {
        char w[16];
        int a, k = 0;
        while (s[i] && is_sp(s[i])) i++;
        a = i;
        while (s[i] && !is_sp(s[i])) i++;
        while (a < i && k < 15) w[k++] = s[a++];
        w[k] = 0;
        if (eq(w, "none")) return 1;
        if (eq(w, "underline")) return 2;
        if (eq(w, "line-through")) return 3;
    }
    return 0;
}

static int vis_val(const char *s) {
    if (eq(s, "hidden") || eq(s, "collapse")) return 2;
    if (eq(s, "visible")) return 1;
    return 0;
}

/* Lot 88 */
static int transform_val(const char *s) {
    if (eq(s, "none")) return 1;
    if (eq(s, "uppercase")) return 2;
    if (eq(s, "lowercase")) return 3;
    if (eq(s, "capitalize")) return 4;
    return 0;
}

static int ws_val(const char *s) {
    if (eq(s, "normal")) return 1;
    if (eq(s, "pre")) return 2;
    if (eq(s, "nowrap")) return 3;
    if (eq(s, "pre-wrap") || eq(s, "break-spaces")) return 4;
    if (eq(s, "pre-line")) return 5;
    return 0;
}

/* the all shorthand takes a CSS-wide keyword only */
static int all_val(const char *s) {
    if (eq(s, "initial")) return 1;
    if (eq(s, "inherit")) return 2;
    if (eq(s, "unset")) return 3;
    if (eq(s, "revert")) return 4;
    if (eq(s, "revert-layer")) return 5;
    return 0;
}

const char *wf_site_var(const wf_site_t *d, const char *name) {
    int i;
    if (!d || !name) return 0;
    for (i = d->nvars - 1; i >= 0; i--)
        if (eq(d->var[i].name, name)) return d->var[i].invalid ? 0 : d->var[i].value;
    return 0;
}

/* Lot 89: every var() of a value is substituted (wf_cssvar.c):
 * var() inside calc() or another value, fallbacks that hold var(), and
 * custom properties defined from other ones, 8 levels at most (a cycle is
 * invalid). 0 when the value holds no var() (val unchanged), 1 replaced,
 * -1 invalid at computed-value time (an unknown property with no
 * fallback, a cycle, a malformed var() or a result longer than val):
 * the declaration is dropped. */
static const char *var_get(const void *ctx, const char *name) {
    const wf_site_t *d = (const wf_site_t *)ctx;
    /* Lot 89: the element's own or inherited value first */
    if (g_vnode >= 0) {
        int inv;
        const char *v = node_var_inv(d, g_vnode, name, &inv);
        if (v) return v;
        if (inv) return 0;   /* Lot 90: guaranteed-invalid on this element */
    }
    return wf_site_var(d, name);
}

const char *wf_site_node_var(const wf_site_t *d, int idx, const char *name) {
    const char *v;
    int inv;
    if (!d || !name) return 0;
    v = node_var_inv(d, idx, name, &inv);
    if (inv) return 0;
    return v ? v : wf_site_var(d, name);
}

/* Lot 89: 1 when val names a per-element custom property in a var() */
static int names_elem_var(const wf_site_t *d, const char *val) {
    int i, k;
    if (!d || !d->nvnames) return 0;
    for (i = 0; val[i]; i++) {
        char nm[24];
        if (!(val[i] == 'v' && val[i + 1] == 'a' && val[i + 2] == 'r' && val[i + 3] == '(')) continue;
        i += 4;
        while (val[i] == ' ') i++;
        k = 0;
        while (val[i] && val[i] != ',' && val[i] != ')' && val[i] != ' ' && k < 23) nm[k++] = val[i++];
        nm[k] = 0;
        if (vname_index(d, nm) >= 0) return 1;
        if (!val[i]) break;
    }
    return 0;
}
static int expand_var(const wf_site_t *d, char *val, int cap) {
    char tmp[WF_SITE_VAL];
    int r;
    if (cap > (int)sizeof(tmp)) cap = (int)sizeof(tmp);
    r = wf_cssvar_subst(val, tmp, cap, var_get, d);
    if (r <= 0) return r < 0 ? (wf_cssvar_count(val) == 0 ? 0 : -1) : 0;
    scopy(val, cap, tmp, slen(tmp));
    return 1;
}

/* Lot 87: the property names the reader keeps, by rule kind */
static int prop_kind(const char *prop) {
    if (eq(prop, "display")) return 1;
    if (eq(prop, "font-weight")) return 2;
    if (eq(prop, "text-align")) return 3;
    if (eq(prop, "color")) return 4;
    if (eq(prop, "margin")) return 5;
    if (eq(prop, "padding")) return 6;
    if (eq(prop, "font-style")) return WF_SITE_K_STYLE;
    if (eq(prop, "text-decoration") || eq(prop, "text-decoration-line")) return WF_SITE_K_DECO;
    if (eq(prop, "visibility")) return WF_SITE_K_VIS;
    if (eq(prop, "text-transform")) return WF_SITE_K_TRANSFORM;
    if (eq(prop, "white-space")) return WF_SITE_K_WS;
    if (eq(prop, "background-color") || eq(prop, "background")) return WF_SITE_K_BG;
    if (eq(prop, "list-style-type") || eq(prop, "list-style")) return WF_SITE_K_LIST;
    if (eq(prop, "font-size")) return WF_SITE_K_FSIZE;
    if (eq(prop, "text-indent")) return WF_SITE_K_INDENT;
    if (eq(prop, "letter-spacing")) return WF_SITE_K_LSPACE;
    /* Lot 90 */
    if (eq(prop, "background-image")) return WF_SITE_K_BGIMG;
    if (eq(prop, "background-position")) return WF_SITE_K_BGPOS;
    if (eq(prop, "background-size")) return WF_SITE_K_BGSIZE;
    if (eq(prop, "background-repeat")) return WF_SITE_K_BGREP;
    if (eq(prop, "list-style-image")) return WF_SITE_K_LIMG;
    if (eq(prop, "list-style-position")) return WF_SITE_K_LPOS;
    /* Lot 91 */
    if (eq(prop, "background-clip")) return WF_SITE_K_BGCLIP;
    if (eq(prop, "background-origin")) return WF_SITE_K_BGORIGIN;
    return 0;
}

/* Lot 87: the properties that inherit in CSS */
static int kind_inherits(int kind) {
    return kind == 2 || kind == 3 || kind == 4 || kind == WF_SITE_K_STYLE || kind == WF_SITE_K_VIS ||
           kind == WF_SITE_K_TRANSFORM || kind == WF_SITE_K_WS || kind == WF_SITE_K_LIST ||
           kind == WF_SITE_K_FSIZE || kind == WF_SITE_K_INDENT || kind == WF_SITE_K_LSPACE ||
           kind == WF_SITE_K_LIMG || kind == WF_SITE_K_LPOS;
}

/* Lot 87: initial is a value like any other: inline, normal, start
 * (left), canvas text (black), 0, normal, none, visible */
static void initial_value(int kind, wf_site_rule_t *proto) {
    proto->kind = (uint8_t)kind;
    proto->revert = WF_SITE_RV_NONE;
    proto->color = 0;
    proto->box_n = 0;
    proto->tok[0] = 0;
    proto->lidx = 0;    /* Lot 90: none, 0% 0%, auto, repeat, none, outside */
    switch (kind) {
    case 1: proto->val = WF_SITE_DISP_INLINE; break;
    /* Lot 88: transparent, disc, medium, 0, normal; val 1 marks a value
     * that came from initial and is not passed to the engine */
    case WF_SITE_K_BG: proto->val = 1; break;
    case WF_SITE_K_LIST: proto->val = 1; scopy(proto->tok, WF_SITE_TOK, "disc", 4); break;
    case WF_SITE_K_FSIZE: proto->val = 1; scopy(proto->tok, WF_SITE_TOK, "medium", 6); break;
    case WF_SITE_K_INDENT: proto->val = 1; scopy(proto->tok, WF_SITE_TOK, "0", 1); break;
    case WF_SITE_K_LSPACE: proto->val = 1; scopy(proto->tok, WF_SITE_TOK, "normal", 6); break;
    case 4: proto->val = 1; proto->color = 0x000000; break;
    case 5: case 6: proto->val = 0; proto->box_n = 1; proto->box[0] = 0; break;
    default: proto->val = 1; break;     /* normal, left, normal, none, visible, none, normal */
    }
}

/* ---- Lot 88 (chunk 4): token properties ---------------------------- */

/* list-style-type keywords the engine draws (webeng list_keyword) */
static int list_name(const char *s) {
    static const char *const names[] = { "none", "disc", "circle", "square", "decimal", "decimal-leading-zero",
                                         "lower-alpha", "upper-alpha", "lower-latin", "upper-latin",
                                         "lower-roman", "upper-roman", 0 };
    int i;
    for (i = 0; names[i]; i++) if (eq(s, names[i])) return 1;
    return 0;
}

/* A number with at most two decimals and a unit from units[] (or no unit
 * for 0); milli gets the value x 1000, signed. Returns the unit index + 1,
 * 0 for a bare 0, -1 when invalid. */
static int length_tok(const char *s, const char *const *units, int *milli) {
    int i = 0, neg = 0, v = 0, frac = 0, fd = 0, digits = 0, u;
    if (s[i] == '-' || s[i] == '+') { neg = s[i] == '-'; i++; }
    while (s[i] >= '0' && s[i] <= '9') { if (v < 100000) v = v * 10 + (s[i] - '0'); i++; digits++; }
    if (s[i] == '.') {
        i++;
        while (s[i] >= '0' && s[i] <= '9') { if (fd < 3) { frac = frac * 10 + (s[i] - '0'); fd++; } else return -1; i++; digits++; }
        if (fd > 2) return -1;
    }
    if (!digits) return -1;
    while (fd < 3) { frac *= 10; fd++; }
    *milli = (neg ? -1 : 1) * (v * 1000 + frac);
    if (!s[i]) return *milli == 0 ? 0 : -1;
    for (u = 0; units[u]; u++) if (eq(s + i, units[u])) return u + 1;
    return -1;
}

/* font-size: an absolute or relative keyword, or a positive length in px
 * (up to 96), pt (72), em or rem (6) or % (600) */
static int fsize_tok(const char *s) {
    static const char *const kw[] = { "xx-small", "x-small", "small", "medium", "large", "x-large", "xx-large",
                                      "xxx-large", "smaller", "larger", 0 };
    static const char *const units[] = { "px", "pt", "em", "rem", "%", 0 };
    static const int maxm[] = { 96000, 72000, 6000, 6000, 600000 };
    int i, m = 0, u;
    for (i = 0; kw[i]; i++) if (eq(s, kw[i])) return 1;
    u = length_tok(s, units, &m);
    return u > 0 && m > 0 && m <= maxm[u - 1];
}

/* text-indent: 0, px 0..255 or em 0..10 (the engine refuses % here) */
static int indent_tok(const char *s) {
    static const char *const units[] = { "px", "em", 0 };
    int m = 0, u = length_tok(s, units, &m);
    if (u == 0) return 1;
    if (u == 1) return m >= 0 && m <= 255000;
    return u == 2 && m >= 0 && m <= 10000;
}

/* letter-spacing: normal, 0, px -4..16 (the engine's bounds) or em
 * -0.25..1 */
static int lspace_tok(const char *s) {
    static const char *const units[] = { "px", "em", 0 };
    int m = 0, u;
    if (eq(s, "normal")) return 1;
    u = length_tok(s, units, &m);
    if (u == 0) return 1;
    if (u == 1) return m >= -4000 && m <= 16000;
    return u == 2 && m >= -250 && m <= 1000;
}

/* Lot 90: the value names rem (resolved against the root font size) */
static int has_rem(const char *v) {
    int i;
    for (i = 0; v[i]; i++)
        if (v[i] == 'r' && v[i + 1] == 'e' && v[i + 2] == 'm' && i > 0 && ((v[i - 1] >= '0' && v[i - 1] <= '9') || v[i - 1] == '.'))
            return 1;
    return 0;
}

/* one token kind: 1 and proto filled, 0 invalid */
static int tok_decl(const wf_site_t *d, const char *prop, const char *val, wf_site_rule_t *proto) {
    char t[WF_SITE_TOK], m[WF_SITE_TOK];
    int kind = 0;
    t[0] = 0;
    /* Lot 89: calc(), min(), max() and clamp() resolve to one length
     * token first (wf_cssmath.c), then go through the same bounds as a
     * written length; an expression that does not resolve is invalid */
    if (wf_css_is_math(val) && (eq(prop, "font-size") || eq(prop, "text-indent") || eq(prop, "letter-spacing"))) {
        int px = 0, r = wf_css_math(val, d ? d->vw : 0, d ? d->vh : 0, m, (int)sizeof(m));
        if ((r != 1 || has_rem(val)) && d && wf_css_math_px(val, d->vw, d->vh, 16000, 16000, &px) == 1) {
            /* Lot 90: em mixed with px, or rem: kept as written ("=N",
             * entry N - 1 of the pool) and resolved once the parent and
             * root font sizes are known (resolve_lengths) */
            int e = long_intern((wf_site_t *)d, val);
            if (!e) return 0;
            proto->kind = (uint8_t)(eq(prop, "font-size") ? WF_SITE_K_FSIZE : eq(prop, "text-indent") ? WF_SITE_K_INDENT :
                                    WF_SITE_K_LSPACE);
            proto->val = 2;
            proto->box_n = 0;
            proto->tok[0] = '=';
            proto->tok[1] = 0;
            {
                char t[8];
                int k = 0, n = 1;
                while (e && k < 7) { t[k++] = (char)('0' + e % 10); e /= 10; }
                while (k && n < WF_SITE_TOK - 1) proto->tok[n++] = t[--k];
                proto->tok[n] = 0;
            }
            return 1;
        }
        if (r != 1) return 0;
        if (eq(prop, "font-size") && fsize_tok(m)) kind = WF_SITE_K_FSIZE;
        else if (eq(prop, "text-indent") && indent_tok(m)) kind = WF_SITE_K_INDENT;
        else if (eq(prop, "letter-spacing") && lspace_tok(m)) kind = WF_SITE_K_LSPACE;
        if (!kind) return 0;
        proto->kind = (uint8_t)kind;
        proto->val = 2;
        proto->box_n = 0;
        scopy(proto->tok, WF_SITE_TOK, m, slen(m));
        return 1;
    }
    if (slen(val) >= WF_SITE_TOK + 12) return 0;
    if (eq(prop, "list-style-type")) {
        if (list_name(val)) { scopy(t, WF_SITE_TOK, val, slen(val)); kind = WF_SITE_K_LIST; }
    } else if (slen(val) < WF_SITE_TOK) {
        if (eq(prop, "font-size") && fsize_tok(val)) kind = WF_SITE_K_FSIZE;
        else if (eq(prop, "text-indent") && indent_tok(val)) kind = WF_SITE_K_INDENT;
        else if (eq(prop, "letter-spacing") && lspace_tok(val)) kind = WF_SITE_K_LSPACE;
        if (kind) scopy(t, WF_SITE_TOK, val, slen(val));
    }
    if (!kind) return 0;
    proto->kind = (uint8_t)kind;
    proto->val = 2;
    proto->box_n = 0;
    scopy(proto->tok, WF_SITE_TOK, t, slen(t));
    return 1;
}

/* ---- Lot 90: shorthands that set several kinds ------------------------
 * parse_decl fills proto with the first longhand; the others of a
 * shorthand (background, list-style) go to g_more, copies of proto with
 * their own kind and value. The callers add one rule per entry. */
static wf_site_rule_t g_more[6];
static int g_nmore;

static int short_kinds(const char *prop, int *kinds) {
    if (eq(prop, "background")) {
        kinds[0] = WF_SITE_K_BG; kinds[1] = WF_SITE_K_BGIMG; kinds[2] = WF_SITE_K_BGPOS;
        kinds[3] = WF_SITE_K_BGSIZE; kinds[4] = WF_SITE_K_BGREP;
        kinds[5] = WF_SITE_K_BGCLIP; kinds[6] = WF_SITE_K_BGORIGIN;   /* Lot 91 */
        return 7;
    }
    if (eq(prop, "list-style")) {
        kinds[0] = WF_SITE_K_LIST; kinds[1] = WF_SITE_K_LIMG; kinds[2] = WF_SITE_K_LPOS;
        return 3;
    }
    return 0;
}

/* the other longhands of a shorthand, each a copy of proto with the
 * same keyword (CSS-wide keywords and the per-element kind-only form) */
static void more_like(const char *prop, const wf_site_rule_t *proto, int mode) {
    int kinds[7], nk = short_kinds(prop, kinds), k;
    g_nmore = 0;
    for (k = 1; k < nk; k++) {
        wf_site_rule_t *m = &g_more[g_nmore++];
        *m = *proto;
        if (mode == 1) initial_value(kinds[k], m);
        else m->kind = (uint8_t)kinds[k];
    }
}

/* a pool kind with its normal form, or initial when val is 1 */
static int long_proto(const wf_site_t *d, int kind, const char *norm, int given, wf_site_rule_t *out) {
    out->kind = (uint8_t)kind;
    out->revert = WF_SITE_RV_NONE;
    out->box_n = 0;
    out->tok[0] = 0;
    out->lidx = 0;
    out->val = 1;
    if (!given) return 1;
    out->lidx = (uint8_t)long_intern((wf_site_t *)d, norm);   /* d is never a const object */
    if (!out->lidx) return 0;
    out->val = 2;
    return 1;
}

static int list_name(const char *s);

/* Lot 90: the background shorthand: the colour (one colour as in lot
 * 88, or the last layer's) and the four layer lists */
static int bg_short(const wf_site_t *d, const char *val, wf_site_rule_t *proto) {
    static wf_cssbg_t bg;
    static const int kinds[6] = { WF_SITE_K_BGIMG, WF_SITE_K_BGPOS, WF_SITE_K_BGSIZE, WF_SITE_K_BGREP,
                                  WF_SITE_K_BGCLIP, WF_SITE_K_BGORIGIN };
    uint32_t c = 0;
    int k;
    if (!wf_cssbg_shorthand(val, &bg)) return 0;
    proto->kind = WF_SITE_K_BG;
    proto->box_n = 0;
    proto->lidx = 0;
    if (!bg.color[0] || eq(bg.color, "transparent")) { proto->val = 1; proto->color = 0; }
    else if (parse_color(bg.color, slen(bg.color), &c)) { proto->val = 2; proto->color = c; }
    else return 0;                    /* a colour the reader does not keep: dropped as in lot 88 */
    g_nmore = 0;
    for (k = 0; k < 6; k++) {
        const char *norm = k == 0 ? bg.image : k == 1 ? bg.position : k == 2 ? bg.size : k == 3 ? bg.repeat :
                           k == 4 ? bg.clip : bg.origin;
        int given = k == 0 ? bg.has_image : k == 1 ? bg.has_position : k == 2 ? bg.has_size : k == 3 ? bg.has_repeat :
                    k == 4 ? bg.has_clip : bg.has_origin;
        wf_site_rule_t *m = &g_more[g_nmore];
        *m = *proto;
        if (!long_proto(d, kinds[k], norm, given, m)) { g_nmore = 0; return 0; }
        g_nmore++;
    }
    return 1;
}

/* Lot 90: list-style: a type, a position and an image in any order, each
 * at most once; none is the type when no type is given (and the image is
 * none anyway). What is not given returns to its initial value. */
static int list_short3(const wf_site_t *d, const char *val, wf_site_rule_t *proto) {
    char type[WF_SITE_TOK], img[WF_SITE_LONG], pos[8];
    int i = 0, nones = 0, l;
    type[0] = img[0] = pos[0] = 0;
    while (val[i]) {
        char w[WF_SITE_VAL];
        int k = 0;
        while (val[i] == ' ') i++;
        while (val[i] && val[i] != ' ' && k < (int)sizeof(w) - 1) w[k++] = val[i++];
        w[k] = 0;
        if (!k) break;
        if (eq(w, "none")) { if (++nones > 2) return 0; continue; }
        if (list_name(w)) { if (type[0]) return 0; scopy(type, WF_SITE_TOK, w, k); continue; }
        if (eq(w, "inside") || eq(w, "outside")) { if (pos[0]) return 0; scopy(pos, 8, w, k); continue; }
        if (k > 4 && w[0] == 'u' && w[1] == 'r' && w[2] == 'l' && w[3] == '(') {
            if (img[0]) return 0;
            l = wf_cssbg_list_image(w, img, (int)sizeof(img));
            if (!l) return 0;
            continue;
        }
        return 0;
    }
    if (!type[0] && !img[0] && !pos[0] && !nones) return 0;
    if (nones && type[0] && img[0]) return 0;      /* none has nothing left to set */
    if (nones == 2 && (type[0] || img[0])) return 0;
    if (!type[0] && nones) scopy(type, WF_SITE_TOK, "none", 4);
    if (type[0]) {
        proto->kind = WF_SITE_K_LIST;
        proto->val = 2;
        proto->box_n = 0;
        scopy(proto->tok, WF_SITE_TOK, type, slen(type));
    } else initial_value(WF_SITE_K_LIST, proto);
    g_nmore = 0;
    g_more[0] = *proto;
    g_more[1] = *proto;
    if (!long_proto(d, WF_SITE_K_LIMG, img, img[0] != 0, &g_more[0]) ||
        !long_proto(d, WF_SITE_K_LPOS, pos, pos[0] != 0, &g_more[1])) return 0;
    g_nmore = 2;
    return 1;
}

/* Lot 90: one pool longhand (background-image ... list-style-position) */
static int long_decl(const wf_site_t *d, const char *prop, const char *val, wf_site_rule_t *proto) {
    char norm[WF_SITE_LONG];
    int kind = 0, l = 0;
    if (eq(prop, "background-image")) { kind = WF_SITE_K_BGIMG; l = wf_cssbg_image(val, norm, (int)sizeof(norm)); }
    else if (eq(prop, "background-position")) { kind = WF_SITE_K_BGPOS; l = wf_cssbg_position(val, norm, (int)sizeof(norm)); }
    else if (eq(prop, "background-size")) { kind = WF_SITE_K_BGSIZE; l = wf_cssbg_size(val, norm, (int)sizeof(norm)); }
    else if (eq(prop, "background-repeat")) { kind = WF_SITE_K_BGREP; l = wf_cssbg_repeat(val, norm, (int)sizeof(norm)); }
    else if (eq(prop, "list-style-image")) { kind = WF_SITE_K_LIMG; l = wf_cssbg_list_image(val, norm, (int)sizeof(norm)); }
    else if (eq(prop, "background-clip")) { kind = WF_SITE_K_BGCLIP; l = wf_cssbg_box(val, norm, (int)sizeof(norm)); }
    else if (eq(prop, "background-origin")) { kind = WF_SITE_K_BGORIGIN; l = wf_cssbg_box(val, norm, (int)sizeof(norm)); }
    else if (eq(prop, "list-style-position")) {
        kind = WF_SITE_K_LPOS;
        if (eq(val, "inside") || eq(val, "outside")) { scopy(norm, WF_SITE_LONG, val, slen(val)); l = slen(norm); }
    }
    if (!kind || l <= 0) return 0;
    return long_proto(d, kind, norm, 1, proto);
}

static int parse_decl(const wf_site_t *d, const char *body, int n, wf_site_rule_t *proto) {
    int i = 0;
    proto->kind = 0;
    proto->revert = WF_SITE_RV_NONE;
    g_nmore = 0;
    while (i < n) {
        char prop[24], val[WF_SITE_VAL];
        int a, b, imp = 0, vs, ve;
        i = skip_noise(body, n, i);
        a = i;
        while (i < n && body[i] != ':' && body[i] != ';' && body[i] != '}') i++;
        if (i >= n || body[i] != ':') { while (i < n && body[i] != ';') i++; if (i < n) i++; continue; }
        b = i;
        while (b > a && is_sp(body[b - 1])) b--;
        slower(prop, (int)sizeof(prop), body + a, b - a);
        i++;
        vs = i;
        while (i < n && body[i] != ';') i++;
        ve = i;
        if (trim_word(body + vs, ve - vs, val, (int)sizeof(val), &imp) < 0) {
            /* Lot 90: too long to keep: dropped, never cut */
            if (d) ((wf_site_t *)d)->long_values++;
            if (i < n && body[i] == ';') i++;
            continue;
        }
        proto->imp = (uint8_t)imp;
        proto->deferred = 0;
        proto->lidx = 0;
        if (prop[0] == '-' && prop[1] == '-') {
            /* Lot 89: a custom property set on the elements this rule
             * matches; only names found while scanning the sheet (a full
             * table or a value over 63 bytes leaves the declaration out) */
            int k = vname_index(d, prop), vl = slen(val);
            if (k >= 0 && vl > 0 && vl < (int)sizeof(proto->vval)) {
                proto->kind = WF_SITE_K_VAR;
                proto->vname = (uint8_t)(k + 1);
                scopy(proto->vval, (int)sizeof(proto->vval), val, vl);
                return 1;
            }
            /* Lot 90: a value over 63 bytes is counted once (the sheet
             * pass and the style attribute pass both run with g_vnode -1) */
            if (k >= 0 && vl >= (int)sizeof(proto->vval) && d && g_vnode < 0) ((wf_site_t *)d)->dropped_vars++;
            if (i < n && body[i] == ';') i++;
            continue;
        }
        if (d && g_vnode < 0 && prop_kind(prop) && names_elem_var(d, val)) {
            /* Lot 89: read again for each element (wf_site_apply) */
            proto->deferred = 1;
            scopy(proto->dprop, (int)sizeof(proto->dprop), prop, slen(prop));
            scopy(proto->draw, (int)sizeof(proto->draw), val, slen(val));
        }
        if (d && expand_var(d, val, (int)sizeof(val)) < 0) {
            if (proto->deferred) {
                /* no sheet-wide value: the kind alone, the value comes per
                 * element */
                proto->kind = (uint8_t)prop_kind(prop);
                proto->val = 0;
                proto->box_n = 0;
                proto->tok[0] = 0;
                more_like(prop, proto, 0);
                return 1;
            }
            if (i < n && body[i] == ';') i++;
            continue;
        }
        if (eq(prop, "all")) {
            /* Lot 88: every reader property at once; any other value is
             * invalid for all */
            int kw = all_val(val);
            if (kw) {
                proto->kind = WF_SITE_K_ALL;
                proto->val = (uint8_t)kw;
                proto->revert = WF_SITE_RV_NONE;
                proto->box_n = 0;
                return 1;
            }
            if (i < n && body[i] == ';') i++;
            continue;
        }
        if (prop_kind(prop) && (eq(val, "initial") || (eq(val, "unset") && !kind_inherits(prop_kind(prop))))) {
            initial_value(prop_kind(prop), proto);
            more_like(prop, proto, 1);
            return 1;
        }
        if (prop_kind(prop) && (eq(val, "inherit") || eq(val, "unset"))) {
            proto->kind = (uint8_t)prop_kind(prop);
            proto->revert = WF_SITE_RV_INHERIT;
            proto->val = 0;
            proto->box_n = 0;
            more_like(prop, proto, 0);
            if (eq(val, "unset")) {
                /* unset: inherit on the list kinds (they inherit), initial
                 * on the background ones */
                int k;
                for (k = 0; k < g_nmore; k++)
                    if (!kind_inherits(g_more[k].kind)) initial_value(g_more[k].kind, &g_more[k]);
            }
            return 1;
        }
        if ((eq(val, "revert-layer") || eq(val, "revert")) && prop_kind(prop)) {
            proto->kind = (uint8_t)prop_kind(prop);
            proto->revert = (uint8_t)(eq(val, "revert") ? WF_SITE_RV_ORIGIN : WF_SITE_RV_LAYER);
            proto->val = 0;
            proto->box_n = 0;
            more_like(prop, proto, 0);
            return 1;
        }
        if (eq(prop, "display")) {
            int v = display_val(val);
            if (v) { proto->kind = 1; proto->val = (uint8_t)v; return 1; }
        } else if (eq(prop, "font-weight")) {
            int v = weight_val(val);
            if (v) { proto->kind = 2; proto->val = (uint8_t)v; return 1; }
        } else if (eq(prop, "text-align")) {
            int v = align_val(val);
            if (v) { proto->kind = 3; proto->val = (uint8_t)v; return 1; }
        } else if (eq(prop, "color")) {
            uint32_t c = 0;
            if (parse_color(val, slen(val), &c)) { proto->kind = 4; proto->color = c; proto->val = 1; return 1; }
        } else if (eq(prop, "margin") || eq(prop, "padding")) {
            if (parse_box(val, proto)) { proto->kind = (uint8_t)(eq(prop, "margin") ? 5 : 6); return 1; }
        } else if (eq(prop, "font-style")) {
            int v = style_val(val);
            if (v) { proto->kind = WF_SITE_K_STYLE; proto->val = (uint8_t)v; return 1; }
        } else if (eq(prop, "text-decoration") || eq(prop, "text-decoration-line")) {
            int v = deco_val(val);
            if (v) { proto->kind = WF_SITE_K_DECO; proto->val = (uint8_t)v; return 1; }
        } else if (eq(prop, "visibility")) {
            int v = vis_val(val);
            if (v) { proto->kind = WF_SITE_K_VIS; proto->val = (uint8_t)v; return 1; }
        } else if (eq(prop, "text-transform")) {
            int v = transform_val(val);
            if (v) { proto->kind = WF_SITE_K_TRANSFORM; proto->val = (uint8_t)v; return 1; }
        } else if (eq(prop, "white-space")) {
            int v = ws_val(val);
            if (v) { proto->kind = WF_SITE_K_WS; proto->val = (uint8_t)v; return 1; }
        } else if (eq(prop, "background-color")) {
            /* Lot 88: a colour, or transparent */
            uint32_t c = 0;
            if (eq(val, "transparent")) { proto->kind = WF_SITE_K_BG; proto->val = 1; proto->color = 0; return 1; }
            if (parse_color(val, slen(val), &c)) { proto->kind = WF_SITE_K_BG; proto->val = 2; proto->color = c; return 1; }
        } else if (eq(prop, "background")) {
            /* Lot 90: the colour and the four layer lists (wf_cssbg.c);
             * Lot 88 kept the colour only when it was the whole value */
            if (bg_short(d, val, proto)) return 1;
        } else if (eq(prop, "list-style")) {
            /* Lot 90: type, image and position */
            if (list_short3(d, val, proto)) return 1;
        } else if (long_decl(d, prop, val, proto)) {
            return 1;
        } else if (tok_decl(d, prop, val, proto)) {
            return 1;
        }
        if (i < n && body[i] == ';') i++;
    }
    return 0;
}

/* Lot 90: the rules one declaration gives (a shorthand gives several):
 * parse_decl, then the longhands it left in g_more; *nout entries copied
 * to out (at most 7, Lot 91: background gives seven) */
static int decl_rules(const wf_site_t *d, const char *body, int n, const wf_site_rule_t *base, wf_site_rule_t *out, int *nout) {
    int k;
    *nout = 0;
    out[0] = *base;
    if (!parse_decl(d, body, n, &out[0]) || !out[0].kind) { g_nmore = 0; return 0; }
    *nout = 1;
    for (k = 0; k < g_nmore && *nout < 7; k++) out[(*nout)++] = g_more[k];
    g_nmore = 0;
    return 1;
}

static int parse_nth(const char *s, int n, wf_site_part_t *pt) {
    int i = 0, a = 0, b = 0, bneg = 0, sign = 1;
    while (i < n && is_sp(s[i])) i++;
    if (i + 3 <= n && lc(s[i]) == 'o' && lc(s[i + 1]) == 'd' && lc(s[i + 2]) == 'd') { a = 2; b = 1; }
    else if (i + 4 <= n && lc(s[i]) == 'e') { a = 2; b = 0; }
    else {
        if (s[i] == '+' || s[i] == '-') { sign = s[i] == '-' ? -1 : 1; i++; }
        if (i < n && lc(s[i]) == 'n') { a = sign; }
        else {
            int v = 0, any = 0;
            while (i < n && s[i] >= '0' && s[i] <= '9') { v = v * 10 + s[i] - '0'; i++; any = 1; }
            if (!any) return 0;
            if (i < n && lc(s[i]) == 'n') { a = sign * v; i++; }
            else { a = 0; b = sign < 0 ? v : v; bneg = sign < 0; pt->nth_a = 0; pt->nth_b = b; pt->nth_b_neg = (uint8_t)bneg; return 1; }
        }
        if (i < n && lc(s[i]) == 'n') i++;
        while (i < n && is_sp(s[i])) i++;
        if (i < n && (s[i] == '+' || s[i] == '-')) {
            int v = 0;
            bneg = s[i] == '-';
            i++;
            while (i < n && is_sp(s[i])) i++;
            while (i < n && s[i] >= '0' && s[i] <= '9') { v = v * 10 + s[i] - '0'; i++; }
            b = v;
        }
    }
    if (a < 0) a = 0;
    if (a > 255) a = 255;
    if (b > 255) b = 255;
    pt->nth_a = (uint8_t)a;
    pt->nth_b = b;
    pt->nth_b_neg = (uint8_t)bneg;
    return 1;
}

static int parse_compound(const char *s, int n, int i, wf_site_part_t *pt) {
    clear(pt, (int)sizeof(*pt));
    i = skip_noise(s, n, i);
    if (i < n && s[i] == '&') {
        /* Lot 88: & in @scope is :where(:scope); a compound may go on
         * after it ("&.wide") */
        pt->scope_ref = WF_SITE_SREF_AMP;
        i++;
    } else if (i < n && s[i] != '.' && s[i] != '#' && s[i] != '[' && s[i] != ':' && s[i] != '*')
        i = ident(s, n, i, pt->tag, (int)sizeof(pt->tag));
    else if (i < n && s[i] == '*') i++;
    while (i < n) {
        if (s[i] == '#') {
            i = ident(s, n, i + 1, pt->id, (int)sizeof(pt->id));
        } else if (s[i] == '.') {
            if (pt->ncls < WF_SITE_PCLS) i = ident(s, n, i + 1, pt->cls[pt->ncls], (int)sizeof(pt->cls[0]));
            else { i++; while (i < n && s[i] != '.' && s[i] != '#' && s[i] != '[' && s[i] != ':' && !is_sp(s[i]) && s[i] != '>' && s[i] != '+' && s[i] != '~' && s[i] != ',' && s[i] != '{') i++; }
            if (pt->ncls < WF_SITE_PCLS && pt->cls[pt->ncls][0]) pt->ncls++;
        } else if (s[i] == '[') {
            char name[12], val[40];
            int kind = 1;
            i++;
            i = skip_noise(s, n, i);
            i = ident(s, n, i, name, (int)sizeof(name));
            i = skip_noise(s, n, i);
            val[0] = 0;
            if (i + 1 < n && s[i + 1] == '=' && (s[i] == '~' || s[i] == '^' || s[i] == '$' || s[i] == '*' || s[i] == '|')) {
                kind = s[i] == '~' ? WF_SITE_ATTR_WORD : s[i] == '^' ? WF_SITE_ATTR_PREFIX :
                       s[i] == '$' ? WF_SITE_ATTR_SUFFIX : s[i] == '*' ? WF_SITE_ATTR_SUBSTR : WF_SITE_ATTR_DASH;
                i += 2;
            }
            else if (i < n && s[i] == '=') { kind = 2; i++; }
            if (kind != 1) {
                i = skip_noise(s, n, i);
                if (i < n && (s[i] == '"' || s[i] == '\'')) i = attr_value(s, n, i, n, val, (int)sizeof(val));
                else {
                    /* an unquoted value ends at ] or a space (Lot 86 read
                     * past the bracket) */
                    int k = 0;
                    while (i < n && s[i] != ']' && !is_sp(s[i])) { if (k < (int)sizeof(val) - 1) val[k++] = s[i]; i++; }
                    val[k] = 0;
                }
                i = skip_noise(s, n, i);
                /* [a=v i]: ASCII case-insensitive; [a=v s] keeps the case */
                if (i < n && (s[i] == 'i' || s[i] == 'I') && i + 1 < n && (s[i + 1] == ']' || is_sp(s[i + 1])))
                    pt->attr_ci = 1;
            }
            while (i < n && s[i] != ']') i++;
            if (i < n) i++;
            scopy(pt->aname, (int)sizeof(pt->aname), name, slen(name));
            scopy(pt->aval, (int)sizeof(pt->aval), val, slen(val));
            pt->attr_kind = (uint8_t)kind;
        } else if (s[i] == ':') {
            char ps[24];
            i++;
            if (i < n && s[i] == ':') i++;   /* ::pseudo-element is ignored */
            i = ident(s, n, i, ps, (int)sizeof(ps));
            if (eq(ps, "first-child")) pt->pseudo = WF_SITE_PS_FIRST;
            else if (eq(ps, "last-child")) pt->pseudo = WF_SITE_PS_LAST;
            else if (eq(ps, "link") || eq(ps, "visited") || eq(ps, "any-link")) pt->pseudo = WF_SITE_PS_LINK;
            else if (eq(ps, "first-of-type")) pt->pseudo = WF_SITE_PS_FIRST_TYPE;
            else if (eq(ps, "last-of-type")) pt->pseudo = WF_SITE_PS_LAST_TYPE;
            else if (eq(ps, "only-child")) pt->pseudo = WF_SITE_PS_ONLY;
            else if (eq(ps, "only-of-type")) pt->pseudo = WF_SITE_PS_ONLY_TYPE;
            else if (eq(ps, "empty")) pt->pseudo = WF_SITE_PS_EMPTY;
            else if (eq(ps, "root")) pt->pseudo = WF_SITE_PS_ROOT;
            else if (eq(ps, "scope")) {
                /* Lot 89: kept apart from pseudo, so ":scope:first-child"
                 * keeps both conditions */
                if (!pt->scope_ref) pt->scope_ref = WF_SITE_SREF_SCOPE;
            }
            else if (eq(ps, "not") && i < n && s[i] == '(') {
                i++;
                i = skip_noise(s, n, i);
                if (i < n && s[i] == '.') { pt->not_kind = 1; i = ident(s, n, i + 1, pt->not_tag, (int)sizeof(pt->not_tag)); }
                else if (i < n && s[i] == '#') { pt->not_kind = 2; i = ident(s, n, i + 1, pt->not_tag, (int)sizeof(pt->not_tag)); }
                else if (i < n && s[i] == '[') {
                    pt->not_kind = 3;
                    i = skip_noise(s, n, i + 1);
                    i = ident(s, n, i, pt->not_tag, (int)sizeof(pt->not_tag));
                } else i = ident(s, n, i, pt->not_tag, (int)sizeof(pt->not_tag));
                while (i < n && s[i] != ')') i++;
                if (i < n) i++;
                pt->pseudo = WF_SITE_PS_NOT;
            } else if ((eq(ps, "is") || eq(ps, "where") || eq(ps, "matches")) && i < n && s[i] == '(') {
                /* up to WF_SITE_ALT alternatives, each "tag", ".cls" or "tag.cls" */
                int close = i + 1, depth = 1;
                while (close < n && depth) { if (s[close] == '(') depth++; else if (s[close] == ')') depth--; if (depth) close++; }
                i++;
                pt->nalt = 0;
                while (i < close) {
                    char at[12], ac[12];
                    at[0] = ac[0] = 0;
                    i = skip_noise(s, close, i);
                    if (i < close && s[i] != '.' && s[i] != ',') i = ident(s, close, i, at, (int)sizeof(at));
                    if (i < close && s[i] == '.') i = ident(s, close, i + 1, ac, (int)sizeof(ac));
                    /* anything else inside the alternative is not supported:
                     * the whole selector is dropped rather than widened */
                    i = skip_noise(s, close, i);
                    if (i < close && s[i] != ',') return -1;
                    if (pt->nalt >= WF_SITE_ALT) return -1;
                    if (!at[0] && !ac[0]) return -1;
                    scopy(pt->alt_tag[pt->nalt], 12, at, slen(at));
                    scopy(pt->alt_cls[pt->nalt], 12, ac, slen(ac));
                    pt->nalt++;
                    if (i < close && s[i] == ',') i++;
                }
                i = close < n ? close + 1 : n;
                pt->pseudo = (uint8_t)(eq(ps, "where") ? WF_SITE_PS_WHERE : WF_SITE_PS_IS);
            } else if ((eq(ps, "nth-child") || eq(ps, "nth-last-child") || eq(ps, "nth-of-type") ||
                        eq(ps, "nth-last-of-type")) && i < n && s[i] == '(') {
                int a = ++i;
                while (i < n && s[i] != ')') i++;
                if (!parse_nth(s + a, i - a, pt)) return -1;
                if (i < n) i++;
                pt->pseudo = eq(ps, "nth-child") ? WF_SITE_PS_NTH : eq(ps, "nth-last-child") ? WF_SITE_PS_NTH_LAST :
                             eq(ps, "nth-of-type") ? WF_SITE_PS_NTH_TYPE : WF_SITE_PS_NTH_LAST_TYPE;
            } else if (i < n && s[i] == '(') {
                /* an unknown functional pseudo-class never matches */
                return -1;
            } else if (ps[0]) {
                /* :hover, :focus, :checked...: a state the reader does not
                 * have. The rule cannot match a static page. */
                return -1;
            }
        } else break;
    }
    return i;
}

static int parse_selector(wf_site_t *d, const char *s, int n, const wf_site_rule_t *proto) {
    int i = 0;
    while (i < n) {
        wf_site_rule_t rule;
        int parts = 0;
        uint8_t rel = WF_SITE_REL_DESC;
        clear(&rule, (int)sizeof(rule));
        rule.kind = proto->kind;
        rule.val = proto->val;
        rule.box_n = proto->box_n;
        rule.box[0] = proto->box[0];
        rule.box[1] = proto->box[1];
        rule.box[2] = proto->box[2];
        rule.box[3] = proto->box[3];
        rule.color = proto->color;
        rule.imp = proto->imp;
        rule.origin = proto->origin;
        rule.layer = proto->layer;
        rule.revert = proto->revert;
        rule.scope = proto->scope;
        scopy(rule.tok, WF_SITE_TOK, proto->tok, slen(proto->tok));
        rule.vname = proto->vname;
        scopy(rule.vval, (int)sizeof(rule.vval), proto->vval, slen(proto->vval));
        rule.deferred = proto->deferred;
        scopy(rule.dprop, (int)sizeof(rule.dprop), proto->dprop, slen(proto->dprop));
        scopy(rule.draw, (int)sizeof(rule.draw), proto->draw, slen(proto->draw));
        rule.lidx = proto->lidx;
        {
            uint8_t lead = WF_SITE_REL_DESC;
            int has_scope = 0, q;
        i = skip_noise(s, n, i);
        while (i < n && s[i] != ',') {
            wf_site_part_t part;
            int before;
            i = skip_noise(s, n, i);
            if (i >= n || s[i] == ',') break;
            if (s[i] == '>') { rel = WF_SITE_REL_CHILD; i++; continue; }
            if (s[i] == '+') { rel = WF_SITE_REL_ADJ; i++; continue; }
            if (s[i] == '~') { rel = WF_SITE_REL_SIB; i++; continue; }
            before = i;
            i = parse_compound(s, n, i, &part);
            if (i < 0) {
                /* this selector of the list cannot match: skip to the next
                 * comma at depth 0 and keep the others */
                int depth = 0;
                i = before;
                while (i < n && (depth || s[i] != ',')) {
                    if (s[i] == '(') depth++;
                    else if (s[i] == ')' && depth) depth--;
                    i++;
                }
                parts = 0;
                d->errors++;
                break;
            }
            if (i == before) { i++; continue; }
            if (!parts) lead = rel;
            part.rel = parts ? rel : WF_SITE_REL_DESC;
            rel = WF_SITE_REL_DESC;
            if (parts < WF_SITE_PARTS) copy_part(&rule.part[parts++], &part);
            else d->errors++;
        }
        rule.nparts = parts;
        /* Lot 88: in @scope a selector without :scope or & is relative
         * to the root: ":scope p", or the leading combinator it has */
        for (q = 0; q < parts; q++)
            if (rule.part[q].scope_ref) has_scope = 1;
        if (!rule.scope || has_scope) rule.scope_rel = WF_SITE_SR_NONE;
        else rule.scope_rel = (uint8_t)(lead == WF_SITE_REL_CHILD ? WF_SITE_SR_CHILD :
                                        lead == WF_SITE_REL_ADJ ? WF_SITE_SR_ADJ :
                                        lead == WF_SITE_REL_SIB ? WF_SITE_SR_SIB : WF_SITE_SR_DESC);
        if (parts) add_rule(d, &rule);
        }
        if (i < n && s[i] == ',') i++;
    }
    return 0;
}

/* ---- media queries ------------------------------------------------- */

/* A length in CSS px: N, Npx, Nem / Nrem (16 px). -1 when not a length. */
static int media_len(const char *s, int n, int *i) {
    int v = 0, any = 0, frac = 0, fd = 0;
    while (*i < n && is_sp(s[*i])) (*i)++;
    while (*i < n && s[*i] >= '0' && s[*i] <= '9') { if (v < 100000) v = v * 10 + s[*i] - '0'; (*i)++; any = 1; }
    if (*i < n && s[*i] == '.') {
        (*i)++;
        while (*i < n && s[*i] >= '0' && s[*i] <= '9') { if (fd < 2) { frac = frac * 10 + s[*i] - '0'; fd++; } (*i)++; any = 1; }
        while (fd < 2) { frac *= 10; fd++; }
    }
    if (!any) return -1;
    if (*i + 1 < n && lc(s[*i]) == 'p' && lc(s[*i + 1]) == 'x') { *i += 2; return v; }
    if (*i + 2 < n && lc(s[*i]) == 'r' && lc(s[*i + 1]) == 'e' && lc(s[*i + 2]) == 'm') { *i += 3; return v * 16 + frac * 16 / 100; }
    if (*i + 1 < n && lc(s[*i]) == 'e' && lc(s[*i + 1]) == 'm') { *i += 2; return v * 16 + frac * 16 / 100; }
    if (v == 0) return 0;
    return v;   /* a bare number: treated as px (quirk) */
}

static int word_at(const char *s, int n, int i, char *out, int cap) {
    int k = 0;
    while (i < n && is_sp(s[i])) i++;
    while (i < n && ((s[i] >= 'a' && s[i] <= 'z') || (s[i] >= 'A' && s[i] <= 'Z') || s[i] == '-' ||
                     (s[i] >= '0' && s[i] <= '9'))) {
        if (k < cap - 1) out[k++] = lc(s[i]);
        i++;
    }
    out[k] = 0;
    return i;
}

/* One "(feature: value)" test. Unknown features are false (Media Queries 4). */
static int media_feature(const char *s, int n, int vw, int vh) {
    char f[32], v[24];
    int i = 0, len;
    i = word_at(s, n, i, f, (int)sizeof(f));
    while (i < n && is_sp(s[i])) i++;
    if (i >= n) {
        /* boolean form: (color), (hover), (orientation) */
        if (eq(f, "color") || eq(f, "hover") || eq(f, "pointer") || eq(f, "any-hover") || eq(f, "any-pointer")) return 1;
        if (eq(f, "width") || eq(f, "height") || eq(f, "orientation")) return 1;
        return 0;
    }
    if (s[i] != ':') return 0;
    i++;
    if (eq(f, "min-width") || eq(f, "max-width") || eq(f, "width") ||
        eq(f, "min-height") || eq(f, "max-height") || eq(f, "height")) {
        int h = eq(f, "height") || eq(f, "min-height") || eq(f, "max-height");
        int have = h ? vh : vw;
        len = media_len(s, n, &i);
        if (len < 0) return 0;
        if (f[0] == 'm' && f[1] == 'i') return have >= len;
        if (f[0] == 'm' && f[1] == 'a') return have <= len;
        return have == len;
    }
    i = word_at(s, n, i, v, (int)sizeof(v));
    if (eq(f, "orientation")) return eq(v, "portrait") ? vh >= vw : eq(v, "landscape") ? vw > vh : 0;
    if (eq(f, "prefers-color-scheme")) return eq(v, "light");
    if (eq(f, "prefers-reduced-motion")) return eq(v, "reduce");   /* no animation in the reader */
    if (eq(f, "hover") || eq(f, "any-hover")) return eq(v, "hover");
    if (eq(f, "pointer") || eq(f, "any-pointer")) return eq(v, "fine");
    if (eq(f, "display-mode")) return eq(v, "browser");
    return 0;
}

/* One media query: [only|not] [type] [and (feature)]... */
static int media_query(const char *s, int n, int vw, int vh) {
    int i = 0, neg = 0, ok = 1, first = 1;
    char w[24];
    while (i < n) {
        int at;
        while (i < n && is_sp(s[i])) i++;
        if (i >= n) break;
        if (s[i] == '(') {
            int depth = 1, a = ++i;
            while (i < n && depth) { if (s[i] == '(') depth++; else if (s[i] == ')') depth--; if (depth) i++; }
            if (!media_feature(s + a, i - a, vw, vh)) ok = 0;
            if (i < n) i++;
            first = 0;
            continue;
        }
        at = i;
        i = word_at(s, n, i, w, (int)sizeof(w));
        if (i == at) { i++; continue; }
        if (eq(w, "and")) continue;
        if (first && eq(w, "not")) { neg = 1; continue; }
        if (first && eq(w, "only")) continue;
        if (eq(w, "print") || eq(w, "speech") || eq(w, "tv") || eq(w, "handheld") || eq(w, "aural")) ok = 0;
        else if (!eq(w, "screen") && !eq(w, "all")) ok = 0;
        first = 0;
    }
    return neg ? !ok : ok;
}

int wf_site_media_ok(const char *s, int n, int vw, int vh) {
    int i = 0, depth = 0, a = 0, any = 0;
    if (!s || n <= 0) return 1;
    if (vw <= 0) vw = 640;
    if (vh <= 0) vh = 480;
    /* comma separated list: true when one query is true */
    for (i = 0; i <= n; i++) {
        if (i < n && s[i] == '(') depth++;
        else if (i < n && s[i] == ')' && depth) depth--;
        if (i == n || (s[i] == ',' && !depth)) {
            int k = a;
            while (k < i && is_sp(s[k])) k++;
            if (k < i) {
                any = 1;
                if (media_query(s + k, i - k, vw, vh)) return 1;
            }
            a = i + 1;
        }
    }
    return any ? 0 : 1;
}

/* Lot 86 name kept for the @media prelude ("@media" itself skipped). */
static int media_ok(const char *s, int n, int vw, int vh) {
    int i = 0;
    while (i < n && s[i] != ' ' && s[i] != '\t' && s[i] != '(') i++;   /* @media */
    return wf_site_media_ok(s + i, n - i, vw, vh);
}

/* ---- @supports ------------------------------------------------------ */

static int known_prop(const char *p, const char *v) {
    static const char *const props[] = {
        "display", "color", "font-weight", "text-align", "margin", "padding", "font-style",
        "text-decoration", "text-decoration-line", "visibility", "background-color", "border",
        "border-radius", "width", "height", "max-width", "min-width", "gap", "grid-template-columns",
        "flex-wrap", "position", "opacity", "box-shadow", "transform", "line-height",
        "letter-spacing", "column-count", "aspect-ratio", "object-fit", "text-transform", "white-space", "all",
        "background", "list-style-type", "list-style", "font-size", "text-indent", 0
    };
    int i;
    for (i = 0; props[i]; i++) if (eq(props[i], p)) break;
    if (!props[i]) return 0;
    if (eq(p, "display")) {
        return eq(v, "block") || eq(v, "inline") || eq(v, "none") || eq(v, "flex") || eq(v, "grid") ||
               eq(v, "inline-block") || eq(v, "table") || eq(v, "list-item") || eq(v, "flow-root") ||
               eq(v, "inline-flex") || eq(v, "inline-grid");
    }
    return v[0] != 0;
}

/* (prop: value), not (...), (...) and (...), (...) or (...),
 * selector(...) is false. */
static int supports_expr(const char *s, int n, int *i);

static int supports_term(const char *s, int n, int *i) {
    char w[16];
    int at;
    while (*i < n && is_sp(s[*i])) (*i)++;
    if (*i >= n) return 0;
    if (s[*i] == '(') {
        int depth = 1, a = ++(*i), k, colon = -1, r;
        while (*i < n && depth) { if (s[*i] == '(') depth++; else if (s[*i] == ')') depth--; if (depth) (*i)++; }
        for (k = a; k < *i; k++) if (s[k] == ':' ) { colon = k; break; }
        {
            int j = a;
            while (j < *i && is_sp(s[j])) j++;
            if (j < *i && s[j] == '(' ) {
                int sub = j;
                r = supports_expr(s, *i, &sub);
                if (*i < n) (*i)++;
                return r;
            }
            if (j + 3 < *i && lc(s[j]) == 'n' && lc(s[j + 1]) == 'o' && lc(s[j + 2]) == 't' && is_sp(s[j + 3])) {
                int sub = j;
                r = supports_expr(s, *i, &sub);
                if (*i < n) (*i)++;
                return r;
            }
        }
        if (colon < 0) { if (*i < n) (*i)++; return 0; }
        {
            char p[32], v[32];
            int b = colon, e = *i;
            while (a < b && is_sp(s[a])) a++;
            while (b > a && is_sp(s[b - 1])) b--;
            slower(p, (int)sizeof(p), s + a, b - a);
            b = colon + 1;
            while (b < e && is_sp(s[b])) b++;
            while (e > b && is_sp(s[e - 1])) e--;
            slower(v, (int)sizeof(v), s + b, e - b);
            r = known_prop(p, v);
        }
        if (*i < n) (*i)++;
        return r;
    }
    at = *i;
    *i = word_at(s, n, *i, w, (int)sizeof(w));
    if (*i == at) { (*i)++; return 0; }
    if (eq(w, "not")) return !supports_term(s, n, i);
    if (eq(w, "selector") || eq(w, "font-tech") || eq(w, "font-format")) {
        int depth = 0;
        while (*i < n) { if (s[*i] == '(') depth++; else if (s[*i] == ')') { depth--; if (depth <= 0) { (*i)++; break; } } (*i)++; }
        return 0;
    }
    return 0;
}

static int supports_expr(const char *s, int n, int *i) {
    int r = supports_term(s, n, i);
    for (;;) {
        char w[8];
        int at;
        while (*i < n && is_sp(s[*i])) (*i)++;
        if (*i >= n || s[*i] == ')') return r;
        at = *i;
        *i = word_at(s, n, *i, w, (int)sizeof(w));
        if (*i == at) { (*i)++; continue; }
        if (eq(w, "and")) { int t = supports_term(s, n, i); r = r && t; }
        else if (eq(w, "or")) { int t = supports_term(s, n, i); r = r || t; }
        else return 0;
    }
}

int wf_site_supports(const char *cond, int n) {
    int i = 0;
    if (!cond || n <= 0) return 0;
    return supports_expr(cond, n, &i);
}

/* ---- @import, custom properties -------------------------------------- */

/* @import url(x) media; / @import "x" media; */
static void take_import(wf_site_t *d, const char *s, int n) {
    int i = 7, a, b;   /* after "@import" */
    while (i < n && is_sp(s[i])) i++;
    if (i + 4 <= n && lc(s[i]) == 'u' && lc(s[i + 1]) == 'r' && lc(s[i + 2]) == 'l' && s[i + 3] == '(') {
        i += 4;
        while (i < n && is_sp(s[i])) i++;
        if (i < n && (s[i] == '"' || s[i] == '\'')) {
            char q = s[i++];
            a = i;
            while (i < n && s[i] != q) i++;
            b = i;
            while (i < n && s[i] != ')') i++;
        } else {
            a = i;
            while (i < n && s[i] != ')' && !is_sp(s[i])) i++;
            b = i;
            while (i < n && s[i] != ')') i++;
        }
        if (i < n) i++;
    } else if (i < n && (s[i] == '"' || s[i] == '\'')) {
        char q = s[i++];
        a = i;
        while (i < n && s[i] != q) i++;
        b = i;
        if (i < n) i++;
    } else return;
    while (i < n && is_sp(s[i])) i++;
    {
        int me = n, mat = 0, layered = 0, before;
        char layer[WF_SITE_LAYER_NAME];
        while (me > i && (is_sp(s[me - 1]) || s[me - 1] == ';')) me--;
        /* Lot 87: layer / layer(name) / supports() come before the media */
        if (!wf_site_import_conds(s + i, me - i, &mat, layer, (int)sizeof(layer), &layered)) return;
        before = d->nlinks;
        add_link(d, s + a, b - a, s + i + mat, me - i - mat, 1);
        if (layered && d->nlinks > before) {
            wf_site_link_t *l = &d->link[d->nlinks - 1];
            int k = 0;
            l->layered = 1;
            while (layer[k] && k < WF_SITE_LAYER_NAME - 1) { l->layer[k] = layer[k]; k++; }
            l->layer[k] = 0;
        }
    }
}

/* --name: value in a rule whose selector is :root, html or body. The last
 * one wins (source order); names and values are kept in lower case. */
static void take_vars(wf_site_t *d, const char *sel, int sn, const char *body, int bn) {
    int i = 0, ok = 0;
    char w[16];
    while (i < sn) {
        int at;
        while (i < sn && (is_sp(sel[i]) || sel[i] == ',')) i++;
        if (i >= sn) break;
        if (sel[i] == ':') {
            at = i + 1;
            i = word_at(sel, sn, at, w, (int)sizeof(w));
            if (eq(w, "root")) ok = 1;
            continue;
        }
        at = i;
        i = word_at(sel, sn, i, w, (int)sizeof(w));
        if (i == at) { i++; continue; }
        if (eq(w, "html") || eq(w, "body")) ok = 1;
    }
    if (!ok) {
        /* Lot 89: any other selector sets them per element */
        vnames_in(d, body, bn);
        return;
    }
    i = 0;
    while (i < bn) {
        int a, b, vs, ve, k;
        while (i < bn && (is_sp(body[i]) || body[i] == ';')) i++;
        a = i;
        while (i < bn && body[i] != ':' && body[i] != ';') i++;
        if (i >= bn || body[i] != ':') { while (i < bn && body[i] != ';') i++; continue; }
        b = i;
        while (b > a && is_sp(body[b - 1])) b--;
        i++;
        vs = i;
        while (i < bn && body[i] != ';') i++;
        ve = i;
        if (b - a < 3 || body[a] != '-' || body[a + 1] != '-') continue;
        while (vs < ve && is_sp(body[vs])) vs++;
        while (ve > vs && is_sp(body[ve - 1])) ve--;
        {
            /* Lot 90: a registered name (@property) is always per element */
            char nm[24];
            if (b - a >= (int)sizeof(nm)) { d->errors++; continue; }
            slower(nm, (int)sizeof(nm), body + a, b - a);
            if (wf_site_prop(d, nm)) { vname_add(d, body + a, b - a); continue; }
        }
        for (k = 0; k < d->nvars; k++) {
            char nm[24];
            slower(nm, (int)sizeof(nm), body + a, b - a);
            if (eq(d->var[k].name, nm)) break;
        }
        /* Lot 90: a value that does not fit is left out, never cut */
        if (ve - vs >= (int)sizeof(d->var[0].value)) { d->dropped_vars++; continue; }
        if (k == d->nvars) {
            if (d->nvars >= WF_SITE_VARS) { d->errors++; continue; }
            d->nvars++;
        }
        slower(d->var[k].name, (int)sizeof(d->var[k].name), body + a, b - a);
        slower(d->var[k].value, (int)sizeof(d->var[k].value), body + vs, ve - vs);
        d->var[k].invalid = 0;
    }
}

/* ---- Lot 90: @property and custom properties computed where declared -- */

static int at_is(const char *s, int n, int at, const char *w);

/* one @property rule; the last valid one for a name wins */
static void take_prop(wf_site_t *d, const char *pre, int pn, const char *body, int bn) {
    wf_cssprop_t p;
    int k;
    if (!wf_cssprop_parse(pre, pn, body, bn, &p)) { d->dropped_props++; return; }
    for (k = 0; k < d->nprops; k++) if (eq(d->prop[k].name, p.name)) break;
    if (k == d->nprops) {
        if (d->nprops >= WF_SITE_PROPS) { d->dropped_props++; return; }
        d->nprops++;
    }
    d->prop[k] = p;
    vname_add(d, p.name, slen(p.name));
}

/* Pass 0: @property at the top level and in applying @media, @supports
 * and @layer blocks, before any custom property is read */
static void scan_props(wf_site_t *d, const char *s, int n, int depth) {
    int i = 0;
    while (i < n) {
        i = skip_noise(s, n, i);
        if (i >= n) break;
        if (s[i] == '@') {
            int prelude = i;
            while (i < n && s[i] != '{' && s[i] != ';') i++;
            if (i < n && s[i] == ';') { i++; continue; }
            if (i < n && s[i] == '{') {
                int body = i + 1, after = skip_block(s, n, i), blen = after - 1 - body;
                if (blen < 0) blen = 0;
                if (at_is(s, n, prelude, "property")) take_prop(d, s + prelude + 9, i - prelude - 9, s + body, blen);
                else if (depth < 4 && blen > 0 &&
                         ((prelude + 6 <= n && lc(s[prelude + 1]) == 'm' && lc(s[prelude + 2]) == 'e' &&
                           media_ok(s + prelude, body - 1 - prelude, d->vw, d->vh)) ||
                          (prelude + 9 <= n && lc(s[prelude + 1]) == 's' && lc(s[prelude + 2]) == 'u' &&
                           wf_site_supports(s + prelude + 9, body - 1 - prelude - 9)) ||
                          at_is(s, n, prelude, "layer")))
                    scan_props(d, s + body, blen, depth + 1);
                i = after;
            }
            continue;
        }
        while (i < n && s[i] != '{') i++;
        if (i >= n) break;
        i = skip_block(s, n, i);
    }
}

/* the sheet-wide values (:root, html, body) computed on the root: their
 * var() read the other sheet-wide values only; one that does not resolve
 * makes the value guaranteed-invalid (wf_site_var then returns 0) */
static void resolve_sheet_vars(wf_site_t *d) {
    static char res[WF_SITE_VARS][WF_SITE_VVAL];
    static int8_t st[WF_SITE_VARS];
    int k;
    for (k = 0; k < d->nvars; k++) {
        const char *v = d->var[k].value;
        st[k] = 0;
        if (eq(v, "initial") || eq(v, "inherit") || eq(v, "unset") || eq(v, "revert") || eq(v, "revert-layer")) {
            /* nothing above the root: the initial value, guaranteed-invalid */
            st[k] = -1;
            continue;
        }
        if (wf_cssvar_count(v) == 0) continue;
        g_vnode = -1;
        if (wf_cssvar_subst(v, res[k], WF_SITE_VVAL, var_get, d) < 0) st[k] = -1;
        else st[k] = 1;
    }
    for (k = 0; k < d->nvars; k++) {
        if (st[k] < 0) { d->var[k].invalid = 1; d->var_invalid++; }
        else if (st[k] > 0) { scopy(d->var[k].value, WF_SITE_VVAL, res[k], slen(res[k])); d->var_computed++; }
    }
}

/* the registration of name k, or 0 */
static const wf_cssprop_t *kprop(const wf_site_t *d, int k) { return wf_site_prop(d, d->vname[k]); }

/* the initial value of name k as a binding: the registered one, else
 * guaranteed-invalid for a registered name without one, 0 (sheet-wide)
 * when the name is not registered */
static uint8_t initial_b(wf_site_t *d, int k) {
    const wf_cssprop_t *p = kprop(d, k);
    int e;
    if (!p) return 0;
    if (!p->has_initial) return WF_SITE_VINVALID;
    e = evar_intern(d, k, p->initial);
    if (e < 0) return WF_SITE_VINVALID;
    d->prop_initial++;
    return (uint8_t)(e + 1);
}

/* what name k inherits on node i (unset on an inherited property) */
static uint8_t inherited_b(wf_site_t *d, int i, int k) {
    const wf_cssprop_t *p = kprop(d, k);
    int par = d->node[i].parent;
    if (p && !p->inherits) return initial_b(d, k);
    if (par >= 0 && par < i && d->node[par].vbind[k]) return d->node[par].vbind[k];
    return p ? initial_b(d, k) : 0;
}

/* Lot 89 copied the declared text down the tree and substituted var() on
 * each element that used it. Lot 90: each element's own values are
 * computed on that element (CSS-wide keywords, then var(), then the
 * syntax of a registration), and that computed value is what descendants
 * inherit. Nodes are in document order: the parent is done first. */
static void compute_vars(wf_site_t *d) {
    static uint8_t own[WF_SITE_VNAMES], nb[WF_SITE_VNAMES];
    int i, k;
    d->var_bound = 0;
    for (i = 0; i < d->nnodes; i++) {
        wf_site_node_t *nd = &d->node[i];
        int any = 0;
        for (k = 0; k < d->nvnames; k++) {
            own[k] = nd->vbind[k];
            if (own[k]) any = 1;
            else nd->vbind[k] = inherited_b(d, i, k);
        }
        if (any) d->var_bound++;
        /* CSS-wide keywords first: other values may read them */
        for (k = 0; k < d->nvnames; k++) {
            const char *v;
            if (!own[k] || own[k] > d->nevars) continue;
            v = d->evar[own[k] - 1].value;
            if (eq(v, "initial")) {
                const wf_cssprop_t *p = kprop(d, k);
                nd->vbind[k] = p ? initial_b(d, k) : WF_SITE_VINVALID;
            } else if (eq(v, "inherit")) {
                int par = nd->parent;
                nd->vbind[k] = (par >= 0 && par < i && d->node[par].vbind[k]) ? d->node[par].vbind[k] :
                               (kprop(d, k) ? initial_b(d, k) : 0);
            } else if (eq(v, "unset") || eq(v, "revert") || eq(v, "revert-layer")) {
                /* revert and revert-layer act as unset on a custom property
                 * (the reader does not keep earlier origins for them) */
                nd->vbind[k] = inherited_b(d, i, k);
            } else continue;
            own[k] = 0;
        }
        /* var() of the own values, every one read against the same state */
        for (k = 0; k < d->nvnames; k++) {
            static char tmp[WF_SITE_VVAL];
            const char *v;
            int r, e;
            nb[k] = nd->vbind[k];
            if (!own[k]) continue;
            v = d->evar[own[k] - 1].value;
            if (wf_cssvar_count(v) == 0) continue;
            g_vnode = i;
            r = wf_cssvar_subst(v, tmp, WF_SITE_VVAL, var_get, d);
            g_vnode = -1;
            if (r < 0) {
                d->var_invalid++;
                nb[k] = kprop(d, k) ? inherited_b(d, i, k) : WF_SITE_VINVALID;
                continue;
            }
            e = evar_intern(d, k, tmp);
            if (e < 0) { d->var_invalid++; nb[k] = WF_SITE_VINVALID; continue; }
            nb[k] = (uint8_t)(e + 1);
            d->var_computed++;
        }
        for (k = 0; k < d->nvnames; k++) {
            const wf_cssprop_t *p;
            nd->vbind[k] = nb[k];
            if (!own[k] || !(p = kprop(d, k))) continue;
            /* a registered value of the wrong type acts as unset */
            if (nb[k] != WF_SITE_VINVALID && nb[k] <= d->nevars && wf_cssprop_match(p->syntax, d->evar[nb[k] - 1].value))
                continue;
            if (nb[k] != WF_SITE_VINVALID) d->prop_unset++;
            nd->vbind[k] = inherited_b(d, i, k);
        }
    }
}

int wf_site_props_text(const wf_site_t *d, char *out, int cap) {
    int n = 0;
    if (!out || cap <= 0) return 0;
    out[0] = 0;
    if (!d) return 0;
    n = app(out, cap, n, "registered ");
    n = app_u(out, cap, n, (unsigned)d->nprops);
    n = app(out, cap, n, " rejected ");
    n = app_u(out, cap, n, (unsigned)d->dropped_props);
    n = app(out, cap, n, " computed ");
    n = app_u(out, cap, n, (unsigned)d->var_computed);
    n = app(out, cap, n, " invalid ");
    n = app_u(out, cap, n, (unsigned)d->var_invalid);
    n = app(out, cap, n, " unset ");
    n = app_u(out, cap, n, (unsigned)d->prop_unset);
    n = app(out, cap, n, " initial ");
    n = app_u(out, cap, n, (unsigned)d->prop_initial);
    return n;
}

/* ---- @layer ----------------------------------------------------------- */

/* index + 1 of a layer name, registering it at the end of the order when
 * it is new; 0 when the table is full (its rules then count as unlayered,
 * and dropped_layers says so) */
static int layer_id(wf_site_t *d, const char *name, int n) {
    int k;
    char low[WF_SITE_LAYER_NAME];
    if (n <= 0) return 0;
    if (n >= WF_SITE_LAYER_NAME) { d->dropped_layers++; return 0; }
    /* "a.b" first makes "a" exist (its parent is placed where the child
     * first appears) */
    for (k = n - 1; k > 0; k--) if (name[k] == '.') { (void)layer_id(d, name, k); break; }
    slower(low, (int)sizeof(low), name, n);
    for (k = 0; k < d->nlayers; k++) if (eq(d->layer[k], low)) return k + 1;
    if (d->nlayers >= WF_SITE_LAYERS) { d->dropped_layers++; return 0; }
    scopy(d->layer[d->nlayers], WF_SITE_LAYER_NAME, low, slen(low));
    d->nlayers++;
    return d->nlayers;
}

/* "outer.inner" for a layer named inside another one */
static int layer_join(const wf_site_t *d, int outer, const char *name, int n, char *out, int cap) {
    int m = 0, k;
    out[0] = 0;
    if (outer > 0 && outer <= d->nlayers) {
        const char *o = d->layer[outer - 1];
        for (k = 0; o[k] && m < cap - 1; k++) out[m++] = o[k];
        if (m < cap - 1) out[m++] = '.';
    }
    for (k = 0; k < n && m < cap - 1; k++) out[m++] = name[k];
    out[m] = 0;
    return (outer > 0 ? slen(d->layer[outer - 1]) + 1 : 0) + n < cap ? m : -1;
}

static int layer_name_char(char c) {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') ||
           c == '-' || c == '_' || c == '.';
}

/* "@layer a, b.c;" declares the order; returns how many names */
static int layer_statement(wf_site_t *d, int outer, const char *s, int n) {
    int i = 6, got = 0;   /* after "@layer" */
    while (i < n) {
        int a;
        char full[WF_SITE_LAYER_NAME];
        while (i < n && (is_sp(s[i]) || s[i] == ',')) i++;
        a = i;
        while (i < n && layer_name_char(s[i])) i++;
        if (i == a) { if (i < n) i++; continue; }
        if (layer_join(d, outer, s + a, i - a, full, (int)sizeof(full)) < 0) { d->dropped_layers++; continue; }
        if (layer_id(d, full, slen(full))) got++;
    }
    return got;
}

/* "@layer name {" or "@layer {": the layer of the block (0 when the
 * table is full) */
static int layer_block(wf_site_t *d, int outer, const char *s, int n) {
    int i = 6, a;
    char full[WF_SITE_LAYER_NAME];
    while (i < n && is_sp(s[i])) i++;
    a = i;
    while (i < n && layer_name_char(s[i])) i++;
    if (i == a) {
        /* anonymous: a layer nobody else can name */
        char anon[12];
        int k = 0, v = ++d->anon_layers;
        char t[8];
        int tn = 0;
        anon[k++] = '#'; anon[k++] = 'a'; anon[k++] = 'n'; anon[k++] = 'o'; anon[k++] = 'n';
        while (v && tn < 7) { t[tn++] = (char)('0' + v % 10); v /= 10; }
        while (tn && k < 11) anon[k++] = t[--tn];
        anon[k] = 0;
        if (layer_join(d, outer, anon, k, full, (int)sizeof(full)) < 0) { d->dropped_layers++; return 0; }
        return layer_id(d, full, slen(full));
    }
    if (layer_join(d, outer, s + a, i - a, full, (int)sizeof(full)) < 0) { d->dropped_layers++; return 0; }
    return layer_id(d, full, slen(full));
}

static int at_is(const char *s, int n, int at, const char *w) {
    int k = 0;
    while (w[k]) { if (at + 1 + k >= n || lc(s[at + 1 + k]) != w[k]) return 0; k++; }
    return at + 1 + k >= n || !layer_name_char(s[at + 1 + k]);
}

int wf_site_import_conds(const char *s, int n, int *media_at, char *layer_out, int cap, int *layered) {
    int i = 0, ok = 1;
    if (layer_out && cap > 0) layer_out[0] = 0;
    if (layered) *layered = 0;
    if (media_at) *media_at = 0;
    if (!s || n <= 0) return 1;
    while (i < n && is_sp(s[i])) i++;
    if (i + 5 <= n && lc(s[i]) == 'l' && lc(s[i + 1]) == 'a' && lc(s[i + 2]) == 'y' && lc(s[i + 3]) == 'e' &&
        lc(s[i + 4]) == 'r' && (i + 5 == n || s[i + 5] == '(' || is_sp(s[i + 5]) || s[i + 5] == ';')) {
        i += 5;
        if (layered) *layered = 1;
        if (i < n && s[i] == '(') {
            int a = ++i, k = 0;
            while (i < n && s[i] != ')') i++;
            if (layer_out) {
                int b = i;
                while (a < b && is_sp(s[a])) a++;
                while (b > a && is_sp(s[b - 1])) b--;
                while (a < b && k < cap - 1) layer_out[k++] = lc(s[a++]);
                layer_out[k] = 0;
            }
            if (i < n) i++;
        }
        while (i < n && is_sp(s[i])) i++;
    }
    if (i + 9 <= n && lc(s[i]) == 's' && lc(s[i + 1]) == 'u' && lc(s[i + 2]) == 'p' && lc(s[i + 3]) == 'p' &&
        lc(s[i + 4]) == 'o' && lc(s[i + 5]) == 'r' && lc(s[i + 6]) == 't' && lc(s[i + 7]) == 's' && s[i + 8] == '(') {
        int a = i + 8, depth = 0;
        i = a;
        do {
            if (s[i] == '(') depth++;
            else if (s[i] == ')') depth--;
            i++;
        } while (i < n && depth > 0);
        /* the condition is given with its parentheses, as after @supports */
        ok = wf_site_supports(s + a, i - a);
        while (i < n && is_sp(s[i])) i++;
    }
    if (media_at) *media_at = i;
    return ok;
}

/* parent index of layer k ("a" for "a.b"), -1 for a top-level layer */
static int layer_parent(const wf_site_t *d, int k) {
    const char *nm = d->layer[k];
    int n = slen(nm), cut = -1, j;
    for (j = n - 1; j > 0; j--) if (nm[j] == '.') { cut = j; break; }
    if (cut < 0) return -1;
    for (j = 0; j < d->nlayers; j++) {
        int i = 0;
        while (i < cut && d->layer[j][i] == nm[i]) i++;
        if (i == cut && d->layer[j][i] == 0) return j;
    }
    return -1;
}

static void layer_visit(wf_site_t *d, int k, int *next, int depth) {
    int j;
    if (depth > WF_SITE_LAYERS) return;
    /* the sublayers first, in order of appearance, then the layer's own
     * rules: inside "a", "a.b" is weaker than the rules written in "a" */
    for (j = 0; j < d->nlayers; j++) if (j != k && layer_parent(d, j) == k) layer_visit(d, j, next, depth + 1);
    d->layer_rank[k] = (uint8_t)(++*next);
}

/* Lot 87: cascade position of every layer (CSS Cascade 5, 6.4.3) */
static void layer_ranks(wf_site_t *d) {
    int k, next = 0;
    for (k = 0; k < d->nlayers; k++) d->layer_rank[k] = 0;
    for (k = 0; k < d->nlayers; k++) if (layer_parent(d, k) < 0) layer_visit(d, k, &next, 0);
    for (k = 0; k < d->nlayers; k++) if (!d->layer_rank[k]) d->layer_rank[k] = (uint8_t)(++next);
}

int wf_site_layers_text(const wf_site_t *d, char *out, int cap) {
    int k, m = 0, j;
    if (!out || cap <= 0) return 0;
    out[0] = 0;
    if (!d) return 0;
    for (k = 0; k < d->nlayers; k++) {
        if (k && m < cap - 1) out[m++] = ',';
        for (j = 0; d->layer[k][j] && m < cap - 1; j++) out[m++] = d->layer[k][j];
    }
    out[m] = 0;
    return m;
}

/* Pass 1 over a sheet: custom properties and @import, including those in
 * applying @media / @supports blocks. */
static void scan_scope_names(wf_site_t *d, const char *s, int n, int depth);

static void scan_sheet(wf_site_t *d, const char *s, int n, int depth) {
    int i = 0;
    while (i < n) {
        int sel, brace;
        i = skip_noise(s, n, i);
        if (i >= n) break;
        if (s[i] == '@') {
            int prelude = i;
            while (i < n && s[i] != '{' && s[i] != ';') i++;
            if (i - prelude >= 7 && lc(s[prelude + 1]) == 'i' && lc(s[prelude + 2]) == 'm' && lc(s[prelude + 3]) == 'p')
                take_import(d, s + prelude, i - prelude);
            if (i < n && s[i] == ';') { i++; continue; }
            if (i < n && s[i] == '{') {
                int body = i + 1, after = skip_block(s, n, i), keep = 0;
                if (prelude + 6 <= n && lc(s[prelude + 1]) == 'm' && lc(s[prelude + 2]) == 'e')
                    keep = media_ok(s + prelude, body - 1 - prelude, d->vw, d->vh);
                else if (prelude + 9 <= n && lc(s[prelude + 1]) == 's' && lc(s[prelude + 2]) == 'u')
                    keep = wf_site_supports(s + prelude + 9, body - 1 - prelude - 9);
                else if (at_is(s, n, prelude, "layer"))
                    keep = 1;   /* Lot 87: variables and imports inside a layer count */
                else if (at_is(s, n, prelude, "scope")) {
                    /* Lot 89: every --x in @scope is per element */
                    if (depth < 4 && after - 1 > body) scan_scope_names(d, s + body, after - 1 - body, depth + 1);
                    i = after;
                    continue;
                }
                if (keep && depth < 4 && after - 1 > body) scan_sheet(d, s + body, after - 1 - body, depth + 1);
                i = after;
            }
            continue;
        }
        sel = i;
        while (i < n && s[i] != '{') i++;
        if (i >= n) break;
        brace = i;
        i = skip_block(s, n, i);
        take_vars(d, s + sel, brace - sel, s + brace + 1, i - 1 - (brace + 1));
    }
}

/* Lot 89: names in an @scope block: declarations right in the block and
 * every rule, nested blocks included (never sheet-wide) */
static void scan_scope_names(wf_site_t *d, const char *s, int n, int depth) {
    int i = 0;
    while (i < n) {
        int k = i, brace;
        while (k < n && s[k] != '{' && s[k] != '}') k++;
        if (k >= n) { vnames_in(d, s + i, n - i); break; }
        /* "--x: v; p" before "{": the declarations end at the last ';' */
        brace = k;
        while (k > i && s[k - 1] != ';') k--;
        if (k > i) vnames_in(d, s + i, k - i);
        {
            int body = brace + 1, after = skip_block(s, n, brace), at = k;
            while (at < brace && is_sp(s[at])) at++;
            if (at < brace && s[at] == '@') {
                if (depth < 4 && after - 1 > body) scan_scope_names(d, s + body, after - 1 - body, depth + 1);
            } else if (after - 1 > body) vnames_in(d, s + body, after - 1 - body);
            i = after;
        }
    }
}

/* ---- @scope prelude (Lot 88) ------------------------------------------ */

/* "(selector, ...)" at i: up to WF_SITE_SCOPE_ALTS compound chains (a
 * longer list keeps the first ones and counts an error; an empty or
 * invalid selector in the list drops the prelude, as a browser drops an
 * invalid selector list). Returns the index after ")" or -1. */
static int scope_sel(wf_site_t *d, const char *s, int n, int i, wf_site_part_t out[][WF_SITE_PARTS],
                     uint8_t *len, int *nout) {
    int close, depth = 1, k, parts = 0, nsel = 0, over = 0;
    uint8_t rel = WF_SITE_REL_DESC;
    *nout = 0;
    if (i >= n || s[i] != '(') return -1;
    close = i + 1;
    while (close < n && depth) {
        if (s[close] == '(') depth++;
        else if (s[close] == ')') depth--;
        if (depth) close++;
    }
    if (close >= n) return -1;
    k = i + 1;
    while (k <= close) {
        wf_site_part_t part;
        int before;
        k = skip_noise(s, close, k);
        if (k >= close || s[k] == ',') {
            /* end of one selector */
            if (!parts) return -1;
            if (!over) len[nsel++] = (uint8_t)parts;
            parts = 0;
            rel = WF_SITE_REL_DESC;
            if (k >= close) break;
            k++;
            if (nsel >= WF_SITE_SCOPE_ALTS && !over) { over = 1; d->errors++; }
            continue;
        }
        if (s[k] == '>') { rel = WF_SITE_REL_CHILD; k++; continue; }
        if (s[k] == '+') { rel = WF_SITE_REL_ADJ; k++; continue; }
        if (s[k] == '~') { rel = WF_SITE_REL_SIB; k++; continue; }
        before = k;
        k = parse_compound(s, close, k, &part);
        if (k < 0) return -1;
        if (k == before) { k++; continue; }
        part.rel = parts ? rel : WF_SITE_REL_DESC;
        rel = WF_SITE_REL_DESC;
        if (parts >= WF_SITE_PARTS) return -1;
        if (!over) copy_part(&out[nsel][parts], &part);
        parts++;
    }
    if (!nsel) return -1;
    *nout = nsel;
    return close + 1;
}

/* the parent element of the <style> block whose text holds css offset
 * at, or -1 (an external sheet, or a <style> at the top) */
static int style_owner_at(const wf_site_t *d, int at) {
    int k;
    for (k = 0; k < d->nstyles; k++)
        if (at >= d->style_from[k] && at < d->style_to[k]) return d->style_owner[k];
    return -1;
}

/* "@scope (start) to (end)", "@scope to (end)", "@scope (start)" or
 * "@scope": a new scope inside outer (-1 none). Returns its index + 1,
 * 0 when the prelude is not valid (the block is then dropped, as a
 * browser does) or the table is full (counted). */
static int scope_block(wf_site_t *d, int outer, const char *s, int n, int css_at) {
    wf_site_scope_t *sc;
    int i = 6;   /* after "@scope" */
    if (d->nscopes >= WF_SITE_SCOPES) { d->dropped_scopes++; return 0; }
    sc = &d->scope[d->nscopes];
    clear(sc, (int)sizeof(*sc));
    sc->parent = outer;
    sc->owner = -1;
    i = skip_noise(s, n, i);
    if (i < n && s[i] == '(') {
        i = scope_sel(d, s, n, i, sc->start, sc->start_len, &sc->nstart);
        if (i < 0) { d->errors++; return 0; }
        i = skip_noise(s, n, i);
    }
    if (i + 1 < n && lc(s[i]) == 't' && lc(s[i + 1]) == 'o' && (i + 2 >= n || is_sp(s[i + 2]) || s[i + 2] == '(')) {
        i = skip_noise(s, n, i + 2);
        i = scope_sel(d, s, n, i, sc->end, sc->end_len, &sc->nend);
        if (i < 0) { d->errors++; return 0; }
        i = skip_noise(s, n, i);
    }
    if (i < n) { d->errors++; return 0; }
    if (!sc->nstart && outer < 0) sc->owner = style_owner_at(d, css_at);
    d->nscopes++;
    return d->nscopes;
}

/* a run of declarations right in an @scope block: they apply to the
 * root, as ":where(:scope)" */
static void scope_decls(wf_site_t *d, const char *s, int n, uint8_t origin, int layer, int scope) {
    int p = 0;
    while (p < n) {
        wf_site_rule_t proto;
        int semi = p;
        while (semi < n && s[semi] != ';') semi++;
        static wf_site_rule_t got[7];
        int ng = 0, k;
        clear(&proto, (int)sizeof(proto));
        proto.origin = origin;
        proto.layer = (uint8_t)layer;
        proto.scope = (uint8_t)scope;
        if (decl_rules(d, s + p, semi - p, &proto, got, &ng))
            for (k = 0; k < ng; k++) parse_selector(d, "&", 1, &got[k]);
        p = semi < n ? semi + 1 : n;
    }
}

static int parse_sheet_in(wf_site_t *d, const char *s, int n, uint8_t origin, int layer, int depth, int scope) {
    int i = 0;
    while (i < n) {
        int sel, brace;
        i = skip_noise(s, n, i);
        if (i >= n) break;
        if (scope && s[i] != '@') {
            /* Lot 88: "color: red;" before "{" in an @scope block is a
             * declaration of the root, not a selector */
            int k = i, colon = 0;
            while (k < n && s[k] != '{' && s[k] != ';' && s[k] != '}') { if (s[k] == ':' && !colon) colon = k; k++; }
            if (k < n && s[k] == ';' && colon) {
                scope_decls(d, s + i, k - i, origin, layer, scope);
                i = k + 1;
                continue;
            }
            if (k >= n && colon) {
                /* the last declaration may go without ";" */
                scope_decls(d, s + i, n - i, origin, layer, scope);
                break;
            }
        }
        if (s[i] == '@') {
            int prelude = i;
            while (i < n && s[i] != '{' && s[i] != ';') i++;
            if (i < n && s[i] == ';') {
                /* Lot 87: "@layer a, b;" fixes the order before any rule */
                if (at_is(s, n, prelude, "layer")) (void)layer_statement(d, layer, s + prelude, i - prelude);
                i++;
                continue;
            }
            if (i < n && s[i] == '{' && at_is(s, n, prelude, "layer")) {
                int body = i + 1, after = skip_block(s, n, i), blen = after - 1 - body;
                int inner = layer_block(d, layer, s + prelude, i - prelude);
                /* a full table: the block still applies, unlayered (counted) */
                if (blen > 0 && depth < 6) parse_sheet_in(d, s + body, blen, origin, inner ? inner : layer, depth + 1, scope);
                i = after;
                continue;
            }
            if (i < n && s[i] == '{' &&
                ((prelude + 6 <= n && lc(s[prelude + 1]) == 'm' && lc(s[prelude + 2]) == 'e' &&
                  media_ok(s + prelude, i - prelude, d->vw, d->vh)) ||
                 (prelude + 9 <= n && lc(s[prelude + 1]) == 's' && lc(s[prelude + 2]) == 'u' &&
                  wf_site_supports(s + prelude + 9, i - prelude - 9)))) {
                int body = i + 1;
                int after = skip_block(s, n, i);
                int blen = after - 1 - body;
                if (blen > 0 && depth < 6) parse_sheet_in(d, s + body, blen, origin, layer, depth + 1, scope);
                i = after;
            } else if (i < n && s[i] == '{' && at_is(s, n, prelude, "scope")) {
                /* Lot 88: @scope, nested up to WF_SITE_SCOPE_DEPTH */
                int body = i + 1, after = skip_block(s, n, i), blen = after - 1 - body;
                int inner = 0, lvl = 0, k = scope - 1;
                while (k >= 0 && lvl < 8) { lvl++; k = d->scope[k].parent; }
                if (lvl < WF_SITE_SCOPE_DEPTH)
                    inner = scope_block(d, scope - 1, s + prelude, i - prelude, (int)(s + prelude - d->css));
                else d->errors++;
                if (inner && blen > 0 && depth < 6) parse_sheet_in(d, s + body, blen, origin, layer, depth + 1, inner);
                i = after;
            } else i = skip_block(s, n, i);
            continue;
        }
        sel = i;
        while (i < n && s[i] != '{') i++;
        if (i >= n) break;
        brace = i;
        i = skip_block(s, n, i);
        {
            wf_site_rule_t proto;
            int body = brace + 1;
            int body_n = i - 1 - body;
            if (body_n < 0) body_n = 0;
            /* one declaration per rule keeps the cascade table small; a
             * block with several properties becomes several selectors */
            {
                int p = 0;
                while (p < body_n) {
                    static wf_site_rule_t got[7];
                    int semi = p, ng = 0, k;
                    while (semi < body_n && s[body + semi] != ';') semi++;
                    clear(&proto, (int)sizeof(proto));
                    proto.origin = origin;
                    proto.layer = (uint8_t)layer;
                    proto.scope = (uint8_t)scope;
                    if (decl_rules(d, s + body + p, semi - p, &proto, got, &ng))
                        for (k = 0; k < ng; k++) parse_selector(d, s + sel, brace - sel, &got[k]);
                    p = semi < body_n ? semi + 1 : body_n;
                }
            }
        }
    }
    return 0;
}

static int parse_sheet(wf_site_t *d, const char *s, int n, uint8_t origin) {
    return parse_sheet_in(d, s, n, origin, 0, 0, 0);
}

static void add_ua(wf_site_t *d, const char *sel, int disp) {
    wf_site_rule_t proto;
    clear(&proto, (int)sizeof(proto));
    proto.origin = 1;
    proto.kind = 1;
    proto.val = (uint8_t)disp;
    parse_selector(d, sel, slen(sel), &proto);
}

/* Every declaration of a style attribute (Lot 86 took only the first). */
/* the style attribute of node i, one declaration at a time; kind 0 every
 * kind, ceiling: only declarations whose cascade step is below it */
static void inline_node(wf_site_t *d, int i, int kind, int ceiling) {
    const char *st = d->node[i].style;
    int n = slen(st), p = 0;
    while (p < n) {
        static wf_site_rule_t got[7];
        wf_site_rule_t proto;
        int semi = p, ng = 0, k;
        while (semi < n && st[semi] != ';') semi++;
        clear(&proto, (int)sizeof(proto));
        proto.origin = 3;
        g_vnode = i;   /* Lot 89: var() in a style attribute reads this element */
        if (decl_rules(d, st + p, semi - p, &proto, got, &ng)) {
            for (k = 0; k < ng; k++) {
                wf_site_rule_t *r = &got[k];
                if (r->kind == WF_SITE_K_VAR || (kind && r->kind != kind && r->kind != WF_SITE_K_ALL) ||
                    top_of(r) >= ceiling) continue;
                r->spec = 100000;
                r->order = 1000 + i;
                r->nparts = 0;
                paint_rule(d, i, r, 0, kind);
            }
        }
        g_vnode = -1;
        p = semi < n ? semi + 1 : n;
    }
}

/* Lot 89: the --x of the style attributes (they beat every sheet rule) */
static void inline_vars(wf_site_t *d) {
    int i;
    for (i = 0; i < d->nnodes; i++) {
        const char *st = d->node[i].style;
        int n = slen(st), p = 0;
        while (p < n) {
            wf_site_rule_t proto;
            int semi = p;
            while (semi < n && st[semi] != ';') semi++;
            clear(&proto, (int)sizeof(proto));
            proto.origin = 3;
            if (parse_decl(d, st + p, semi - p, &proto) && proto.kind == WF_SITE_K_VAR) {
                proto.spec = 100000;
                proto.order = 1000 + i;
                proto.nparts = 0;
                paint_rule(d, i, &proto, 0, 0);
            }
            p = semi < n ? semi + 1 : n;
        }
    }
}

/* Lot 89: the declaration of a deferred rule read for node idx with that
 * element's custom properties; 0 when it is invalid there (dropped on
 * this element, as a sheet-wide invalid var() drops it everywhere) */
static int defer_rule(wf_site_t *d, int idx, const wf_site_rule_t *r, wf_site_rule_t *out) {
    static wf_site_rule_t got[7];
    char text[20 + WF_SITE_VAL + 4];
    wf_site_rule_t p;
    int n = 0, ok, k, ng = 0, j;
    const unsigned char *a = (const unsigned char *)r;
    unsigned char *b = (unsigned char *)out;
    for (k = 0; k < (int)sizeof(*r); k++) b[k] = a[k];
    for (k = 0; r->dprop[k] && n < (int)sizeof(text) - 2; k++) text[n++] = r->dprop[k];
    text[n++] = ':';
    for (k = 0; r->draw[k] && n < (int)sizeof(text) - 1; k++) text[n++] = r->draw[k];
    text[n] = 0;
    clear(&p, (int)sizeof(p));
    g_vnode = idx;
    ok = decl_rules(d, text, n, &p, got, &ng);
    g_vnode = -1;
    /* Lot 90: a shorthand gives several kinds: the one of this rule */
    for (j = 0; ok && j < ng && got[j].kind != r->kind; j++) {}
    if (!ok || j >= ng) { d->deferred_dropped++; return 0; }
    p = got[j];
    out->val = p.val;
    out->color = p.color;
    out->box_n = p.box_n;
    for (k = 0; k < 4; k++) out->box[k] = p.box[k];
    scopy(out->tok, WF_SITE_TOK, p.tok, slen(p.tok));
    out->lidx = p.lidx;
    out->revert = p.revert;
    out->deferred = 0;
    d->deferred_hits++;
    return 1;
}

/* one rule on one node, a deferred rule read for that node first */
static void paint_matched(wf_site_t *d, int i, const wf_site_rule_t *r, int prox, int only) {
    if (r->deferred) {
        static wf_site_rule_t tmp;
        if (!defer_rule(d, i, r, &tmp)) return;
        paint_rule(d, i, &tmp, prox, only);
        return;
    }
    paint_rule(d, i, r, prox, only);
}

static void apply_inline(wf_site_t *d) {
    int i;
    for (i = 0; i < d->nnodes; i++) inline_node(d, i, 0, 1 << 30);
}

/* Lot 87: revert-layer and revert. The property of the node gets the
 * value it would have without the declarations of that layer (revert-
 * layer: every cascade step from that one up goes) or of that origin
 * (revert: an author revert, the style attribute included, leaves the
 * user-agent sheet only; a user-agent revert leaves nothing). The cascade
 * runs again for that property alone with what is left; a revert-layer
 * found there rolls back again (each round goes strictly lower, at most
 * WF_SITE_LAYERS + 3 rounds). */
static int keep_rule(int top, int origin, int ceiling, int max_origin) {
    if (max_origin) return origin < max_origin;
    return top < ceiling;
}

/* Lot 87: the parent's value of one property (inherit) */
static void copy_kind(wf_site_node_t *nd, const wf_site_node_t *par, int kind) {
    int k;
    switch (kind) {
    case 1: nd->disp = par->disp; nd->disp_imp = par->disp_imp; break;
    case 2: nd->weight = par->weight; nd->weight_imp = par->weight_imp; break;
    case 3: nd->align = par->align; nd->align_imp = par->align_imp; break;
    case 4: nd->color = par->color; nd->color_set = par->color_set; nd->color_imp = par->color_imp; break;
    case 5: for (k = 0; k < 4; k++) nd->margin[k] = par->margin[k]; nd->margin_set = par->margin_set; break;
    case 6: for (k = 0; k < 4; k++) nd->pad[k] = par->pad[k]; nd->pad_set = par->pad_set; break;
    case WF_SITE_K_STYLE: nd->italic = par->italic; break;
    case WF_SITE_K_DECO: nd->deco = par->deco; break;
    case WF_SITE_K_VIS: nd->vis = par->vis; break;
    case WF_SITE_K_TRANSFORM: nd->transform = par->transform; break;
    case WF_SITE_K_WS: nd->ws = par->ws; break;
    case WF_SITE_K_BG: nd->bg = par->bg; nd->bg_set = par->bg_set; break;
    case WF_SITE_K_LIST: case WF_SITE_K_FSIZE: case WF_SITE_K_INDENT: case WF_SITE_K_LSPACE:
        scopy(nd->tok[kind - WF_SITE_K_LIST], WF_SITE_TOK, par->tok[kind - WF_SITE_K_LIST],
              slen(par->tok[kind - WF_SITE_K_LIST]));
        nd->tok_val[kind - WF_SITE_K_LIST] = par->tok_val[kind - WF_SITE_K_LIST];
        break;
    case WF_SITE_K_BGIMG: case WF_SITE_K_BGPOS: case WF_SITE_K_BGSIZE: case WF_SITE_K_BGREP:
    case WF_SITE_K_LIMG: case WF_SITE_K_LPOS: case WF_SITE_K_BGCLIP: case WF_SITE_K_BGORIGIN:
        nd->lv[kind - WF_SITE_K_BGIMG] = par->lv[kind - WF_SITE_K_BGIMG];
        nd->lv_val[kind - WF_SITE_K_BGIMG] = par->lv_val[kind - WF_SITE_K_BGIMG];
        break;
    default: break;
    }
}

static void resolve_reverts(wf_site_t *d) {
    int i, k, r, round;
    for (i = 0; i < d->nnodes; i++) {
        wf_site_node_t *nd = &d->node[i];
        for (k = 1; k <= WF_SITE_K_MAX; k++) {
            for (round = 0; (nd->rv[k] == WF_SITE_RV_LAYER || nd->rv[k] == WF_SITE_RV_ORIGIN) &&
                            round < WF_SITE_LAYERS + 3; round++) {
                int top = *kind_rank(nd, k) / (8192 * WF_SITE_RANK_LOW);
                int origin = (top / 16) % 4, ceiling = top, max_origin = 0;
                if (nd->rv[k] == WF_SITE_RV_ORIGIN) max_origin = origin >= 2 ? 2 : 1;
                reset_kind(nd, k);
                d->reverts++;
                for (r = 0; r < d->nrules; r++) {
                    const wf_site_rule_t *ru = &d->rule[r];
                    int m;
                    if ((ru->kind != k && ru->kind != WF_SITE_K_ALL) ||
                        !keep_rule(top_of(ru), ru->origin, ceiling, max_origin)) continue;
                    m = rule_match(d, i, ru);
                    if (m) paint_matched(d, i, ru, m - 1, k);
                }
                /* the style attribute is author origin: an author revert
                 * drops it as well */
                if (!max_origin) inline_node(d, i, k, ceiling);
            }
            if (nd->rv[k] == WF_SITE_RV_INHERIT) {
                /* nodes come in document order: the parent is final */
                int rank = *kind_rank(nd, k);
                if (nd->parent >= 0 && nd->parent < i) copy_kind(nd, &d->node[nd->parent], k);
                else reset_kind(nd, k);
                *kind_rank(nd, k) = rank;
                nd->rv[k] = 0;
                d->inherits++;
            }
            if (nd->rv[k]) reset_kind(nd, k);
        }
    }
}

/* ---- Lot 90: font sizes in px ---------------------------------------
 * The engine resolves em and % against the parent itself; the reader
 * resolves what the engine cannot: an expression kept as "=N" (em mixed
 * with px, or rem) and a rem length when the root font size is not
 * 16 px. fs[i] is each element's font size in px thousandths (keywords as
 * CSS absolute sizes, smaller / larger by 1.2), the root's from 16 px. */
static int fs_keyword(const char *t, int parent) {
    static const char *const kw[] = { "xx-small", "x-small", "small", "medium", "large", "x-large", "xx-large",
                                      "xxx-large", 0 };
    static const int px[] = { 9000, 10000, 13000, 16000, 18000, 24000, 32000, 48000 };
    int k;
    for (k = 0; kw[k]; k++) if (eq(t, kw[k])) return px[k];
    if (eq(t, "smaller")) return parent * 5 / 6;
    if (eq(t, "larger")) return parent * 6 / 5;
    return 0;
}

static int ends_rem(const char *t) {
    int n = slen(t);
    return n > 3 && t[n - 3] == 'r' && t[n - 2] == 'e' && t[n - 1] == 'm';
}

/* the px value of token t against em and the root size; 0 when it does
 * not resolve (*ok 0) */
static int tok_px(const wf_site_t *d, const char *t, int em, int root, int *ok) {
    int v = 0;
    *ok = 0;
    if (t[0] == '=') {
        int e = 0, k = 1;
        while (t[k] >= '0' && t[k] <= '9') e = e * 10 + (t[k++] - '0');
        if (e < 1 || e > d->nlongs) return 0;
        if (wf_css_math_px(d->lpool[e - 1], d->vw, d->vh, em, root, &v) != 1) return 0;
    } else if (wf_css_math_px(t, d->vw, d->vh, em, root, &v) != 1) return 0;
    *ok = 1;
    return v;
}

static void resolve_lengths(wf_site_t *d) {
    static const int lo[3] = { 1, 0, -4000 }, hi[3] = { 96000, 255000, 16000 };
    int i, root = 16000;
    d->root_fs = 16000;
    for (i = 0; i < d->nnodes; i++) {
        wf_site_node_t *nd = &d->node[i];
        int parent = (nd->parent >= 0 && nd->parent < i) ? d->fs[nd->parent] : 16000;
        int t, f = parent, ok;
        const char *ft = nd->tok[WF_SITE_K_FSIZE - WF_SITE_K_LIST];
        int is_root = nd->parent < 0 && eq(nd->tag, "html");
        int rem = is_root ? 16000 : root;
        if (nd->tok_val[WF_SITE_K_FSIZE - WF_SITE_K_LIST] == 1) f = 16000;
        else if (nd->tok_val[WF_SITE_K_FSIZE - WF_SITE_K_LIST] == 2) {
            int k = fs_keyword(ft, parent);
            if (k) f = k;
            else {
                int v = tok_px(d, ft, parent, rem, &ok);
                if (ok && v >= lo[0] && v <= hi[0]) f = v;
            }
        }
        d->fs[i] = f;
        if (is_root) { root = f; d->root_fs = f; }
        for (t = 0; t < 3; t++) {
            int slot = (WF_SITE_K_FSIZE - WF_SITE_K_LIST) + t, v;
            char *tk = nd->tok[slot];
            if (nd->tok_val[slot] != 2) continue;
            if (tk[0] != '=' && !(t == 0 && ends_rem(tk) && rem != 16000)) continue;
            /* font-size against the parent, the other two against the
             * element's own size */
            v = tok_px(d, tk, t == 0 ? parent : f, rem, &ok);
            if (!ok || v < lo[t] || v > hi[t] || wf_css_token(v, "px", tk, WF_SITE_TOK) < 0) {
                nd->tok_val[slot] = 0;
                tk[0] = 0;
                d->len_dropped++;
                continue;
            }
            d->len_resolved++;
        }
    }
}

int wf_site_apply(wf_site_t *d) {
    int i, r;
    if (!d) return -1;
    add_ua(d, "[hidden]", WF_SITE_DISP_NONE);
    add_ua(d, "script", WF_SITE_DISP_NONE);
    add_ua(d, "style", WF_SITE_DISP_NONE);
    for (i = 0; i < d->nnodes; i++) vnames_in(d, d->node[i].style, slen(d->node[i].style));
    if (d->css_n) {
        /* Lot 90: registrations before any custom property is read */
        scan_props(d, d->css, d->css_n, 0);
        scan_sheet(d, d->css, d->css_n, 0);
        resolve_sheet_vars(d);
        parse_sheet(d, d->css, d->css_n, 2);
    }
    /* Lot 87: the rules carry the layer index until every layer is known;
     * then the position in the cascade */
    layer_ranks(d);
    for (r = 0; r < d->nrules; r++)
        if (d->rule[r].layer > 0 && d->rule[r].layer <= d->nlayers) d->rule[r].layer = d->layer_rank[d->rule[r].layer - 1];
    d->scoped_hits = 0;
    d->deferred_hits = 0;
    d->deferred_dropped = 0;
    /* Lot 89: custom properties first (every element, then inherited),
     * so a deferred declaration reads the final values */
    for (r = 0; r < d->nrules; r++) {
        if (d->rule[r].kind != WF_SITE_K_VAR) continue;
        for (i = 0; i < d->nnodes; i++) {
            int m = rule_match(d, i, &d->rule[r]);
            if (m) paint_rule(d, i, &d->rule[r], m - 1, 0);
        }
    }
    inline_vars(d);
    compute_vars(d);
    for (r = 0; r < d->nrules; r++) {
        if (d->rule[r].kind == WF_SITE_K_VAR) continue;
        for (i = 0; i < d->nnodes; i++) {
            int m = rule_match(d, i, &d->rule[r]);
            if (!m) continue;
            if (d->rule[r].scope) d->scoped_hits++;
            paint_matched(d, i, &d->rule[r], m - 1, 0);
        }
    }
    apply_inline(d);
    d->reverts = 0;
    d->inherits = 0;
    resolve_reverts(d);
    d->len_resolved = 0;
    d->len_dropped = 0;
    resolve_lengths(d);
    d->ready = 1;
    return 0;
}
