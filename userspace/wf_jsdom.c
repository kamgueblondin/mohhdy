/* wf_jsdom.c - Lot 113: the bounded DOM of the page script pass, its
 * HTML parser and serializer, the document/element host objects and the
 * page runner (see wf_jsdom.h). Text and attribute values are kept as
 * they appear in HTML (entities encoded); the getters decode them and
 * the setters encode, so the serializer writes the pool as it is. */
#include "wf_jsdom.h"
#include "wf_jsi.h"

enum { DN_ELEM = 1, DN_TEXT = 3, DN_COMMENT = 8, DN_DOC = 9, DN_DOCTYPE = 10, DN_FRAG = 11 };

typedef struct {
    uint8_t type, raw, inserted, pad;
    int32_t parent, first, last, next, prev;
    uint32_t name, name_len;       /* lowercase tag (pool offset) */
    int32_t attr;                  /* first attribute */
    uint32_t text, text_len;       /* text, comment, doctype */
    uint32_t obj;                  /* the JS object of the node (arena), 0 before use */
    uint32_t listeners;            /* JS array of [type, fn] pairs, 0 before use */
} dnode_t;

typedef struct {
    uint32_t name, name_len, val, val_len;
    int32_t next;
} dattr_t;

static dnode_t g_n[WF_DOM_NODES];
static dattr_t g_a[WF_DOM_ATTRS];
static char g_b[WF_DOM_BYTES];
static int g_nn, g_na, g_trunc;
static uint32_t g_nb;

static int dlen(const char *s) { int n = 0; while (s[n]) n++; return n; }
static char lower(char c) { return (c >= 'A' && c <= 'Z') ? (char)(c + 32) : c; }

static uint32_t pool_put(const char *s, int n, int lowercase) {
    uint32_t at = g_nb;
    int i;
    if (n < 0) n = 0;
    if (g_nb + (uint32_t)n > WF_DOM_BYTES) { g_trunc = 1; return 0; }
    for (i = 0; i < n; i++) g_b[g_nb + (uint32_t)i] = lowercase ? lower(s[i]) : s[i];
    g_nb += (uint32_t)n;
    return at;
}

static int node_new(int type) {
    dnode_t *d;
    int i;
    if (g_nn >= WF_DOM_NODES) { g_trunc = 1; return -1; }
    d = &g_n[g_nn];
    for (i = 0; i < (int)sizeof(*d); i++) ((char *)d)[i] = 0;
    d->type = (uint8_t)type;
    d->parent = d->first = d->last = d->next = d->prev = d->attr = -1;
    return g_nn++;
}

static int name_is(int k, const char *s) {
    int l = dlen(s), i;
    if (g_n[k].type != DN_ELEM || (int)g_n[k].name_len != l) return 0;
    for (i = 0; i < l; i++) if (g_b[g_n[k].name + (uint32_t)i] != s[i]) return 0;
    return 1;
}

static void detach(int k) {
    dnode_t *d = &g_n[k];
    if (d->parent < 0) return;
    if (d->prev >= 0) g_n[d->prev].next = d->next; else g_n[d->parent].first = d->next;
    if (d->next >= 0) g_n[d->next].prev = d->prev; else g_n[d->parent].last = d->prev;
    d->parent = d->next = d->prev = -1;
}

static int is_ancestor(int a, int k) {      /* a is k or contains k */
    int d = 0;
    while (k >= 0 && d++ < WF_DOM_NODES) { if (k == a) return 1; k = g_n[k].parent; }
    return 0;
}

/* insert k under parent before ref (-1: at the end) */
static void insert_before(int parent, int k, int ref) {
    detach(k);
    g_n[k].parent = parent;
    if (ref < 0) {
        g_n[k].prev = g_n[parent].last;
        if (g_n[parent].last >= 0) g_n[g_n[parent].last].next = k; else g_n[parent].first = k;
        g_n[parent].last = k;
    } else {
        g_n[k].next = ref;
        g_n[k].prev = g_n[ref].prev;
        if (g_n[ref].prev >= 0) g_n[g_n[ref].prev].next = k; else g_n[parent].first = k;
        g_n[ref].prev = k;
    }
}

/* ---- attributes ---- */

static int attr_find(int k, const char *name, int nl) {
    int a, i;
    for (a = g_n[k].attr; a >= 0; a = g_a[a].next) {
        if ((int)g_a[a].name_len != nl) continue;
        for (i = 0; i < nl && g_b[g_a[a].name + (uint32_t)i] == lower(name[i]); i++) {}
        if (i == nl) return a;
    }
    return -1;
}

/* set an attribute; val is already HTML-encoded */
static void attr_set_raw(int k, const char *name, int nl, const char *val, int vl) {
    int a = attr_find(k, name, nl);
    if (a < 0) {
        int last;
        if (g_na >= WF_DOM_ATTRS) { g_trunc = 1; return; }
        a = g_na++;
        g_a[a].name = pool_put(name, nl, 1);
        g_a[a].name_len = (uint32_t)nl;
        g_a[a].next = -1;
        if (g_n[k].attr < 0) g_n[k].attr = a;
        else {
            for (last = g_n[k].attr; g_a[last].next >= 0; last = g_a[last].next) {}
            g_a[last].next = a;
        }
    }
    g_a[a].val = pool_put(val, vl, 0);
    g_a[a].val_len = (uint32_t)vl;
}

static void attr_remove(int k, const char *name, int nl) {
    int a = attr_find(k, name, nl), p;
    if (a < 0) return;
    if (g_n[k].attr == a) { g_n[k].attr = g_a[a].next; return; }
    for (p = g_n[k].attr; p >= 0 && g_a[p].next != a; p = g_a[p].next) {}
    if (p >= 0) g_a[p].next = g_a[a].next;
}

/* ---- entities ---- */

static int ent_decode(const char *s, int n, char *out, int max) {
    static const char *const names[] = { "amp", "&", "lt", "<", "gt", ">", "quot", "\"", "apos", "'", "nbsp", "\xC2\xA0",
                                         "copy", "\xC2\xA9", "reg", "\xC2\xAE", "hellip", "\xE2\x80\xA6", "mdash", "\xE2\x80\x94",
                                         "ndash", "\xE2\x80\x93", "laquo", "\xC2\xAB", "raquo", "\xC2\xBB", "eacute", "\xC3\xA9",
                                         "egrave", "\xC3\xA8", "agrave", "\xC3\xA0", "ccedil", "\xC3\xA7", "euro", "\xE2\x82\xAC", 0 };
    int i = 0, o = 0;
    while (i < n) {
        if (s[i] == '&') {
            int j = i + 1, k;
            if (j < n && s[j] == '#') {
                uint32_t v = 0;
                int hex = 0, any = 0;
                j++;
                if (j < n && (s[j] == 'x' || s[j] == 'X')) { hex = 1; j++; }
                while (j < n && v < 0x110000) {
                    char c = s[j];
                    int d = (c >= '0' && c <= '9') ? c - '0' : (hex && c >= 'a' && c <= 'f') ? c - 'a' + 10 : (hex && c >= 'A' && c <= 'F') ? c - 'A' + 10 : -1;
                    if (d < 0) break;
                    v = v * (hex ? 16u : 10u) + (uint32_t)d;
                    any = 1;
                    j++;
                }
                if (any && v > 0 && v < 0x110000) {
                    if (j < n && s[j] == ';') j++;
                    if (v < 0x80) { if (o < max) out[o++] = (char)v; }
                    else if (v < 0x800) { if (o + 2 <= max) { out[o++] = (char)(0xC0 | (v >> 6)); out[o++] = (char)(0x80 | (v & 63)); } }
                    else if (v < 0x10000) {
                        if (o + 3 <= max) { out[o++] = (char)(0xE0 | (v >> 12)); out[o++] = (char)(0x80 | ((v >> 6) & 63)); out[o++] = (char)(0x80 | (v & 63)); }
                    } else if (o + 4 <= max) {
                        out[o++] = (char)(0xF0 | (v >> 18)); out[o++] = (char)(0x80 | ((v >> 12) & 63));
                        out[o++] = (char)(0x80 | ((v >> 6) & 63)); out[o++] = (char)(0x80 | (v & 63));
                    }
                    i = j;
                    continue;
                }
            } else {
                for (k = 0; names[k]; k += 2) {
                    int l = dlen(names[k]), m;
                    if (j + l > n) continue;
                    for (m = 0; m < l && s[j + m] == names[k][m]; m++) {}
                    if (m == l && (j + l == n || s[j + l] == ';' || !((s[j + l] | 32) >= 'a' && (s[j + l] | 32) <= 'z'))) {
                        const char *r = names[k + 1];
                        int q;
                        for (q = 0; r[q] && o < max; q++) out[o++] = r[q];
                        i = j + l + (j + l < n && s[j + l] == ';');
                        break;
                    }
                }
                if (names[k]) continue;
            }
        }
        if (o < max) out[o++] = s[i];
        i++;
    }
    return o;
}

/* encode for text (attr 0) or a double-quoted attribute (attr 1) */
static void sb_encode(js_vm_t *vm, js_sb_t *b, const char *s, int n, int attr) {
    int i, run = 0;
    for (i = 0; i < n; i++) {
        const char *e = 0;
        char c = s[i];
        if (c == '&') e = "&amp;";
        else if (c == '<') e = "&lt;";
        else if (c == '>') e = attr ? 0 : "&gt;";
        else if (c == '"' && attr) e = "&quot;";
        if (!e) { run++; continue; }
        if (run) js_sb_put(vm, b, s + i - run, run);
        run = 0;
        js_sb_put(vm, b, e, dlen(e));
    }
    if (run) js_sb_put(vm, b, s + n - run, run);
}

/* ---- the HTML parser ---- */

static const char *const k_void[] = { "area", "base", "br", "col", "embed", "hr", "img", "input", "link", "meta",
                                      "param", "source", "track", "wbr", 0 };
static const char *const k_raw[] = { "script", "style", "textarea", "title", "xmp", 0 };
static const char *const k_pclose[] = { "address", "article", "aside", "blockquote", "div", "dl", "fieldset", "footer",
                                        "form", "h1", "h2", "h3", "h4", "h5", "h6", "header", "hr", "main", "nav",
                                        "ol", "p", "pre", "section", "table", "ul", "figure", "details", 0 };

static int in_list(const char *s, int n, const char *const *list) {
    int k, i;
    for (k = 0; list[k]; k++) {
        if (dlen(list[k]) != n) continue;
        for (i = 0; i < n && lower(s[i]) == list[k][i]; i++) {}
        if (i == n) return 1;
    }
    return 0;
}

static int node_in_list(int k, const char *const *list) {
    return g_n[k].type == DN_ELEM && in_list(g_b + g_n[k].name, (int)g_n[k].name_len, list);
}

static int is_space(char c) { return c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == '\f'; }

static void add_text(int parent, const char *s, int n, int raw) {
    int k;
    if (n <= 0) return;
    k = g_n[parent].last;
    if (k >= 0 && g_n[k].type == DN_TEXT && g_n[k].text + g_n[k].text_len == g_nb && g_n[k].raw == raw) {
        (void)pool_put(s, n, 0);       /* extend the last text node in place */
        if (!g_trunc) g_n[k].text_len += (uint32_t)n;
        return;
    }
    k = node_new(DN_TEXT);
    if (k < 0) return;
    g_n[k].text = pool_put(s, n, 0);
    g_n[k].text_len = g_trunc ? 0 : (uint32_t)n;
    g_n[k].raw = (uint8_t)raw;
    insert_before(parent, k, -1);
}

#define PSTACK 256

/* parse s[0..n) as children of base */
static void parse_into(int base, const char *s, int n) {
    int st[PSTACK], sp = 0, i = 0;
    st[sp++] = base;
    while (i < n && !g_trunc) {
        int top = st[sp - 1];
        if (s[i] != '<') {
            int j = i;
            while (j < n && s[j] != '<') j++;
            add_text(top, s + i, j - i, 0);
            i = j;
            continue;
        }
        if (i + 4 <= n && s[i + 1] == '!' && s[i + 2] == '-' && s[i + 3] == '-') {
            int j = i + 4, k;
            while (j + 3 <= n && !(s[j] == '-' && s[j + 1] == '-' && s[j + 2] == '>')) j++;
            k = node_new(DN_COMMENT);
            if (k >= 0) {
                int e = j + 3 <= n ? j : n;
                g_n[k].text = pool_put(s + i + 4, e - (i + 4), 0);
                g_n[k].text_len = (uint32_t)(e - (i + 4));
                insert_before(top, k, -1);
            }
            i = j + 3 <= n ? j + 3 : n;
            continue;
        }
        if (i + 1 < n && (s[i + 1] == '!' || s[i + 1] == '?')) {
            int j = i + 2, k;
            while (j < n && s[j] != '>') j++;
            k = node_new(s[i + 1] == '!' ? DN_DOCTYPE : DN_COMMENT);
            if (k >= 0) {
                g_n[k].text = pool_put(s + i + 2, j - (i + 2), 0);
                g_n[k].text_len = (uint32_t)(j - (i + 2));
                insert_before(top, k, -1);
            }
            i = j < n ? j + 1 : n;
            continue;
        }
        if (i + 1 < n && s[i + 1] == '/') {      /* end tag */
            int j = i + 2, ns, nl, d;
            ns = j;
            while (j < n && !is_space(s[j]) && s[j] != '>') j++;
            nl = j - ns;
            while (j < n && s[j] != '>') j++;
            i = j < n ? j + 1 : n;
            for (d = sp - 1; d > 0; d--) {
                int k = st[d], q;
                if ((int)g_n[k].name_len != nl) continue;
                for (q = 0; q < nl && g_b[g_n[k].name + (uint32_t)q] == lower(s[ns + q]); q++) {}
                if (q == nl) { sp = d; break; }
            }
            continue;
        }
        if (i + 1 < n && ((s[i + 1] | 32) >= 'a' && (s[i + 1] | 32) <= 'z')) {
            int j = i + 1, ns = j, nl, k, selfc = 0;
            while (j < n && !is_space(s[j]) && s[j] != '>' && s[j] != '/') j++;
            nl = j - ns;
            /* implied ends */
            if (in_list(s + ns, nl, k_pclose)) while (sp > 1 && name_is(st[sp - 1], "p")) sp--;
            if (in_list(s + ns, nl, (const char *const[]){ "li", 0 })) {
                while (sp > 1 && name_is(st[sp - 1], "p")) sp--;
                if (sp > 1 && name_is(st[sp - 1], "li")) sp--;
            }
            if (in_list(s + ns, nl, (const char *const[]){ "dt", "dd", 0 }) && sp > 1 &&
                (name_is(st[sp - 1], "dt") || name_is(st[sp - 1], "dd"))) sp--;
            if (in_list(s + ns, nl, (const char *const[]){ "option", 0 }) && sp > 1 && name_is(st[sp - 1], "option")) sp--;
            if (in_list(s + ns, nl, (const char *const[]){ "td", "th", "tr", 0 })) {
                if (sp > 1 && (name_is(st[sp - 1], "td") || name_is(st[sp - 1], "th"))) sp--;
                if (lower(s[ns + 1]) == 'r' && sp > 1 && name_is(st[sp - 1], "tr")) sp--;
            }
            k = node_new(DN_ELEM);
            if (k < 0) return;
            g_n[k].name = pool_put(s + ns, nl, 1);
            g_n[k].name_len = (uint32_t)nl;
            insert_before(st[sp - 1], k, -1);
            /* attributes */
            for (;;) {
                int as, al, vs = 0, vl = 0;
                char q;
                while (j < n && (is_space(s[j]) || s[j] == '/')) { if (s[j] == '/') selfc = 1; j++; }
                if (j >= n || s[j] == '>') break;
                selfc = 0;
                as = j;
                while (j < n && !is_space(s[j]) && s[j] != '>' && s[j] != '=' && s[j] != '/') j++;
                al = j - as;
                while (j < n && is_space(s[j])) j++;
                if (j < n && s[j] == '=') {
                    j++;
                    while (j < n && is_space(s[j])) j++;
                    if (j < n && (s[j] == '"' || s[j] == '\'')) {
                        q = s[j++];
                        vs = j;
                        while (j < n && s[j] != q) j++;
                        vl = j - vs;
                        if (j < n) j++;
                    } else {
                        vs = j;
                        while (j < n && !is_space(s[j]) && s[j] != '>') j++;
                        vl = j - vs;
                    }
                }
                if (al > 0 && attr_find(k, s + as, al) < 0) {
                    /* keep values double-quotable: a " inside becomes &quot; */
                    int qn = 0, z;
                    for (z = 0; z < vl; z++) if (s[vs + z] == '"') qn++;
                    if (!qn) attr_set_raw(k, s + as, al, s + vs, vl);
                    else {
                        char tmp[512];
                        int o = 0;
                        for (z = 0; z < vl && o < (int)sizeof(tmp) - 7; z++) {
                            if (s[vs + z] == '"') { tmp[o++] = '&'; tmp[o++] = 'q'; tmp[o++] = 'u'; tmp[o++] = 'o'; tmp[o++] = 't'; tmp[o++] = ';'; }
                            else tmp[o++] = s[vs + z];
                        }
                        attr_set_raw(k, s + as, al, tmp, o);
                    }
                }
            }
            i = j < n ? j + 1 : n;
            if (in_list(s + ns, nl, k_void) || selfc) continue;
            if (in_list(s + ns, nl, k_raw)) {
                /* raw text up to the matching end tag */
                int e = i, found = 0;
                while (e + 2 + nl <= n) {
                    if (s[e] == '<' && s[e + 1] == '/') {
                        int q;
                        for (q = 0; q < nl && lower(s[e + 2 + q]) == lower(s[ns + q]); q++) {}
                        if (q == nl) { found = 1; break; }
                    }
                    e++;
                }
                if (!found) e = n;
                add_text(k, s + i, e - i, 1);
                if (found) {
                    while (e < n && s[e] != '>') e++;
                    i = e < n ? e + 1 : n;
                } else {
                    i = n;
                }
                continue;
            }
            if (sp < PSTACK) st[sp++] = k;
            else g_trunc = 1;
            continue;
        }
        add_text(top, "<", 1, 0);      /* a lone < */
        i++;
    }
}

static int find_child(int parent, const char *name) {
    int k;
    for (k = g_n[parent].first; k >= 0; k = g_n[k].next) if (name_is(k, name)) return k;
    return -1;
}

static int elem_new(const char *name) {
    int k = node_new(DN_ELEM);
    if (k < 0) return -1;
    g_n[k].name = pool_put(name, dlen(name), 1);
    g_n[k].name_len = (uint32_t)dlen(name);
    return k;
}

/* make sure the document has html, head and body (scripts use document.body) */
static void fix_document(int doc) {
    int html = find_child(doc, "html"), head, body, k, nx;
    if (html < 0) {
        html = elem_new("html");
        if (html < 0) return;
        for (k = g_n[doc].first; k >= 0; k = nx) {
            nx = g_n[k].next;
            if (g_n[k].type != DN_DOCTYPE) insert_before(html, k, -1);
        }
        insert_before(doc, html, -1);
    }
    head = find_child(html, "head");
    body = find_child(html, "body");
    if (body < 0) {
        body = elem_new("body");
        if (body < 0) return;
        for (k = g_n[html].first; k >= 0; k = nx) {
            nx = g_n[k].next;
            if (k == head || name_is(k, "head")) continue;
            if (head < 0 && (name_is(k, "title") || name_is(k, "meta") || name_is(k, "link") || name_is(k, "style"))) continue;
            insert_before(body, k, -1);
        }
        insert_before(html, body, -1);
    }
    if (head < 0) {
        head = elem_new("head");
        if (head < 0) return;
        for (k = g_n[html].first; k >= 0; k = nx) {
            nx = g_n[k].next;
            if (k != body && g_n[k].type == DN_ELEM) insert_before(head, k, -1);
        }
        insert_before(html, head, g_n[html].first);
    }
}

static int dom_parse(const char *s, int n) {
    int doc;
    g_nn = g_na = 0;
    g_nb = 0;
    g_trunc = 0;
    doc = node_new(DN_DOC);
    parse_into(doc, s, n);
    fix_document(doc);
    return doc;
}

/* ---- the serializer ---- */

typedef struct {
    char *out;
    int n, max;
    js_vm_t *vm;
    js_sb_t *sb;
    int skip_scripts;
} dout_t;

static void dw(dout_t *o, const char *s, int n) {
    if (o->sb) { js_sb_put(o->vm, o->sb, s, n); return; }
    while (n-- > 0) {
        if (o->n < o->max - 1) o->out[o->n] = *s;
        else g_trunc = 1;
        o->n++;
        s++;
    }
}
static void dws(dout_t *o, const char *s) { dw(o, s, dlen(s)); }

static void ser(dout_t *o, int k, int depth) {
    dnode_t *d = &g_n[k];
    int c, a;
    if (depth > PSTACK) return;
    switch (d->type) {
    case DN_TEXT: dw(o, g_b + d->text, (int)d->text_len); return;
    case DN_COMMENT: dws(o, "<!--"); dw(o, g_b + d->text, (int)d->text_len); dws(o, "-->"); return;
    case DN_DOCTYPE: dws(o, "<!"); dw(o, g_b + d->text, (int)d->text_len); dws(o, ">"); return;
    case DN_DOC: case DN_FRAG:
        for (c = d->first; c >= 0; c = g_n[c].next) ser(o, c, depth + 1);
        return;
    default: break;
    }
    if (o->skip_scripts && (name_is(k, "script") || name_is(k, "noscript"))) return;
    dws(o, "<");
    dw(o, g_b + d->name, (int)d->name_len);
    for (a = d->attr; a >= 0; a = g_a[a].next) {
        dws(o, " ");
        dw(o, g_b + g_a[a].name, (int)g_a[a].name_len);
        dws(o, "=\"");
        dw(o, g_b + g_a[a].val, (int)g_a[a].val_len);
        dws(o, "\"");
    }
    dws(o, ">");
    if (node_in_list(k, k_void)) return;
    for (c = d->first; c >= 0; c = g_n[c].next) ser(o, c, depth + 1);
    dws(o, "</");
    dw(o, g_b + g_n[k].name, (int)g_n[k].name_len);
    dws(o, ">");
}

int wf_jsdom_roundtrip(const char *html, int n, char *out, int out_max) {
    dout_t o;
    int doc;
    if (out_max <= 0) return -1;
    doc = dom_parse(html, n);
    if (doc < 0) return -1;
    o.out = out;
    o.n = 0;
    o.max = out_max;
    o.vm = 0;
    o.sb = 0;
    o.skip_scripts = 0;
    ser(&o, doc, 0);
    if (o.n >= out_max) o.n = out_max - 1;
    out[o.n] = 0;
    return o.n;
}

/* ---- selectors: compounds of tag, #id, .class, [attr op value],
 * :first-child, :last-child, :not(simple), with the descendant, >, + and ~
 * combinators, and comma lists ---- */

static int attr_val(int k, const char *name, const char **v, int *vl) {
    int a = attr_find(k, name, dlen(name));
    if (a < 0) return 0;
    *v = g_b + g_a[a].val;
    *vl = (int)g_a[a].val_len;
    return 1;
}

static int has_token(const char *v, int vl, const char *t, int tl) {
    int i = 0;
    while (i < vl) {
        int j;
        while (i < vl && is_space(v[i])) i++;
        j = i;
        while (j < vl && !is_space(v[j])) j++;
        if (j - i == tl) {
            int q;
            for (q = 0; q < tl && v[i + q] == t[q]; q++) {}
            if (q == tl) return 1;
        }
        i = j;
    }
    return 0;
}

static int prev_elem(int k) { for (k = g_n[k].prev; k >= 0 && g_n[k].type != DN_ELEM; k = g_n[k].prev) {} return k; }
static int next_elem(int k) { for (k = g_n[k].next; k >= 0 && g_n[k].type != DN_ELEM; k = g_n[k].next) {} return k; }

static int ident_ch(char c) {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '-' || c == '_' || (uint8_t)c >= 0x80;
}

/* -1: the selector is not understood (no match) */
static int match_compound(int k, const char *s, int a, int b) {
    int i = a;
    if (g_n[k].type != DN_ELEM) return 0;
    if (i < b && s[i] == '*') i++;
    else if (i < b && ident_ch(s[i])) {
        int st = i, q;
        while (i < b && ident_ch(s[i])) i++;
        if ((int)g_n[k].name_len != i - st) return 0;
        for (q = 0; q < i - st; q++) if (g_b[g_n[k].name + (uint32_t)q] != lower(s[st + q])) return 0;
    }
    while (i < b) {
        char c = s[i++];
        if (c == '#' || c == '.') {
            int st = i;
            const char *v;
            int vl;
            while (i < b && ident_ch(s[i])) i++;
            if (c == '#') {
                if (!attr_val(k, "id", &v, &vl) || vl != i - st) return 0;
                { int q; for (q = 0; q < vl; q++) if (v[q] != s[st + q]) return 0; }
            } else {
                if (!attr_val(k, "class", &v, &vl) || !has_token(v, vl, s + st, i - st)) return 0;
            }
        } else if (c == '[') {
            int ns = i, nl, op = 0, vs = 0, vl2 = 0, q;
            const char *v;
            int vl;
            while (i < b && ident_ch(s[i])) i++;
            nl = i - ns;
            if (i < b && s[i] != ']' && s[i] != '=') { op = s[i]; i++; }
            if (i < b && s[i] == '=') {
                char qc = 0;
                if (!op) op = '=';
                i++;
                if (i < b && (s[i] == '"' || s[i] == '\'')) qc = s[i++];
                vs = i;
                while (i < b && (qc ? s[i] != qc : s[i] != ']')) i++;
                vl2 = i - vs;
                if (qc && i < b) i++;
            }
            while (i < b && s[i] != ']') i++;
            if (i < b) i++;
            {
                char nm[64];
                for (q = 0; q < nl && q < 63; q++) nm[q] = lower(s[ns + q]);
                nm[q] = 0;
                if (!attr_val(k, nm, &v, &vl)) return 0;
            }
            if (!op) continue;
            if (op == '=') { if (vl != vl2) return 0; for (q = 0; q < vl; q++) if (v[q] != s[vs + q]) return 0; }
            else if (op == '~') { if (!has_token(v, vl, s + vs, vl2)) return 0; }
            else if (op == '^') { if (vl < vl2) return 0; for (q = 0; q < vl2; q++) if (v[q] != s[vs + q]) return 0; }
            else if (op == '$') { if (vl < vl2) return 0; for (q = 0; q < vl2; q++) if (v[vl - vl2 + q] != s[vs + q]) return 0; }
            else if (op == '*') {
                int z, f = 0;
                for (z = 0; z + vl2 <= vl && !f; z++) { for (q = 0; q < vl2 && v[z + q] == s[vs + q]; q++) {} if (q == vl2) f = 1; }
                if (!f) return 0;
            } else if (op == '|') {
                if (vl < vl2) return 0;
                for (q = 0; q < vl2; q++) if (v[q] != s[vs + q]) return 0;
                if (vl > vl2 && v[vl2] != '-') return 0;
            } else return -1;
        } else if (c == ':') {
            int st = i, l;
            while (i < b && (ident_ch(s[i]))) i++;
            l = i - st;
            if (l == 11 && s[st] == 'f' && s[st + 6] == 'c') { if (prev_elem(k) >= 0) return 0; }          /* first-child */
            else if (l == 10 && s[st] == 'l' && s[st + 5] == 'c') { if (next_elem(k) >= 0) return 0; }     /* last-child */
            else if (l == 3 && s[st] == 'n' && i < b && s[i] == '(') {                                      /* not(...) */
                int e = i + 1, r;
                while (e < b && s[e] != ')') e++;
                r = match_compound(k, s, i + 1, e);
                if (r != 0) return r < 0 ? -1 : 0;
                i = e < b ? e + 1 : b;
            } else if (l == 4 && s[st] == 'r') {      /* root */
                if (g_n[k].parent < 0 || g_n[g_n[k].parent].type != DN_DOC) return 0;
            } else {
                return -1;
            }
        } else {
            return -1;
        }
    }
    return 1;
}

#define SEL_MAX 16

typedef struct { int a[SEL_MAX], b[SEL_MAX]; char comb[SEL_MAX]; int n; } sel_t;

static int sel_parse(const char *s, int a, int b, sel_t *t) {
    int i = a;
    char comb = 0;
    t->n = 0;
    while (i < b) {
        int st, depth = 0;
        while (i < b && is_space(s[i])) { if (t->n) comb = comb ? comb : ' '; i++; }
        if (i < b && (s[i] == '>' || s[i] == '+' || s[i] == '~')) { comb = s[i++]; continue; }
        if (i >= b) break;
        st = i;
        while (i < b && (depth || (!is_space(s[i]) && s[i] != '>' && s[i] != '+' && s[i] != '~'))) {
            if (s[i] == '[' || s[i] == '(') depth++;
            else if ((s[i] == ']' || s[i] == ')') && depth) depth--;
            i++;
        }
        if (t->n >= SEL_MAX) return 0;
        t->a[t->n] = st;
        t->b[t->n] = i;
        t->comb[t->n] = t->n ? (comb ? comb : ' ') : 0;
        t->n++;
        comb = 0;
    }
    return t->n > 0;
}

static int sel_match_at(int k, const char *s, const sel_t *t, int idx, int scope) {
    int r = match_compound(k, s, t->a[idx], t->b[idx]);
    if (r <= 0) return 0;
    if (idx == 0) return 1;
    switch (t->comb[idx]) {
    case '>': return g_n[k].parent >= 0 && g_n[k].parent != scope && sel_match_at(g_n[k].parent, s, t, idx - 1, scope);
    case '+': { int p = prev_elem(k); return p >= 0 && sel_match_at(p, s, t, idx - 1, scope); }
    case '~': { int p; for (p = prev_elem(k); p >= 0; p = prev_elem(p)) if (sel_match_at(p, s, t, idx - 1, scope)) return 1; return 0; }
    default: {
        int p;
        for (p = g_n[k].parent; p >= 0 && p != scope; p = g_n[p].parent) if (sel_match_at(p, s, t, idx - 1, scope)) return 1;
        return 0;
    }
    }
}

/* the whole comma list against k (scope -1: the document) */
static int sel_match(int k, const char *s, int n, int scope) {
    int i = 0;
    while (i < n) {
        int st = i, depth = 0;
        sel_t t;
        while (i < n && (depth || s[i] != ',')) {
            if (s[i] == '[' || s[i] == '(') depth++;
            else if ((s[i] == ']' || s[i] == ')') && depth) depth--;
            i++;
        }
        if (sel_parse(s, st, i, &t) && sel_match_at(k, s, &t, t.n - 1, scope)) return 1;
        i++;
    }
    return 0;
}

static int sel_valid(const char *s, int n) {
    int i, any = 0;
    for (i = 0; i < n; i++) if (!is_space(s[i])) any = 1;
    return any;
}

/* the next node in document order inside root */
static int walk_next(int k, int root) {
    if (g_n[k].first >= 0) return g_n[k].first;
    while (k >= 0 && k != root) {
        if (g_n[k].next >= 0) return g_n[k].next;
        k = g_n[k].parent;
    }
    return -1;
}

/* ---- the JavaScript side ---- */

enum { HK_NODE = 0, HK_STYLE = 1, HK_CLASSLIST = 2 };

enum {
    K_nodeType, K_nodeName, K_tagName, K_id, K_className, K_textContent, K_innerText, K_innerHTML, K_outerHTML,
    K_parentNode, K_parentElement, K_children, K_childNodes, K_firstChild, K_lastChild, K_firstElementChild,
    K_lastElementChild, K_nextSibling, K_previousSibling, K_nextElementSibling, K_previousElementSibling,
    K_childElementCount, K_style, K_classList, K_value, K_checked, K_disabled, K_hidden, K_selected, K_nodeValue,
    K_data, K_body, K_head, K_documentElement, K_title, K_readyState, K_cookie, K_ownerDocument, K_URL,
    K_defaultView, K_cssText, K_length, K_location, K_offsetWidth, K_offsetHeight, K_clientWidth, K_clientHeight,
    K_scrollTop, K_isConnected, K_dataset, K_COUNT
};
static const char *const k_names[K_COUNT] = {
    "nodeType", "nodeName", "tagName", "id", "className", "textContent", "innerText", "innerHTML", "outerHTML",
    "parentNode", "parentElement", "children", "childNodes", "firstChild", "lastChild", "firstElementChild",
    "lastElementChild", "nextSibling", "previousSibling", "nextElementSibling", "previousElementSibling",
    "childElementCount", "style", "classList", "value", "checked", "disabled", "hidden", "selected", "nodeValue",
    "data", "body", "head", "documentElement", "title", "readyState", "cookie", "ownerDocument", "URL",
    "defaultView", "cssText", "length", "location", "offsetWidth", "offsetHeight", "clientWidth", "clientHeight",
    "scrollTop", "isConnected", "dataset"
};
/* string attributes reflected as properties of the same name */
static const char *const k_reflect[] = { "href", "src", "alt", "type", "name", "placeholder", "rel", "lang", "dir",
                                         "action", "method", "target", "content", "htmlFor", "width", "height", "role", 0 };

typedef struct {
    js_vm_t *vm;
    uint32_t key[K_COUNT];
    uint32_t reflect[24];
    uint32_t p_node, p_style, p_classlist;
    int doc;
    char ready[16];
    uint32_t cookie;            /* the document.cookie string */
    uint32_t win_listeners;     /* JS array of [type, fn] for window */
    uint32_t location;
    js_sb_t write_buf;
    int writing;
    wf_jspage_stats_t *st;
} dom_rt_t;

static dom_rt_t g_rt;

static int key_index(uint32_t key) {
    int i;
    for (i = 0; i < K_COUNT; i++) if (g_rt.key[i] == key) return i;
    return -1;
}

static jv node_obj(js_vm_t *vm, int k) {
    uint32_t o;
    if (k < 0) return jv_null();
    if (g_n[k].obj) return jv_obj(g_n[k].obj);
    o = js_obj_new(vm, JS_CLASS_HOST, g_rt.p_node);
    if (!o) return jv_undef();
    JOBJ(vm, o)->x0 = (uint32_t)k;
    JOBJ(vm, o)->x1 = HK_NODE;
    g_n[k].obj = o;
    return jv_obj(o);
}

static int obj_node(js_vm_t *vm, jv v) {
    if (v.t != JT_OBJ || JOBJ(vm, v.a)->cls != JS_CLASS_HOST || JOBJ(vm, v.a)->x1 != HK_NODE) return -1;
    if (JOBJ(vm, v.a)->x0 >= (uint32_t)g_nn) return -1;
    return (int)JOBJ(vm, v.a)->x0;
}

static int this_node(js_vm_t *vm, jv thisv) {
    int k = obj_node(vm, thisv);
    if (k < 0) js_throw_err(vm, JE_TYPE, "Illegal invocation: not a DOM node");
    return k;
}

static uint32_t decoded(js_vm_t *vm, const char *s, int n) {
    js_sb_t b;
    char tmp[256];
    int i = 0;
    js_sb_init(vm, &b);
    while (i < n) {      /* slices of at most 200 bytes that never cut an entity */
        int e = i + 200 < n ? i + 200 : n, o;
        if (e < n) {
            int z;
            for (z = e - 1; z > i && z > e - 12; z--) if (s[z] == '&') { e = z; break; }
        }
        o = ent_decode(s + i, e - i, tmp, (int)sizeof(tmp));
        js_sb_put(vm, &b, tmp, o);
        i = e;
    }
    return js_sb_finish(vm, &b);
}

static void text_of(js_vm_t *vm, js_sb_t *b, int k, int depth) {
    int c;
    if (depth > PSTACK) return;
    if (g_n[k].type == DN_TEXT) {
        if (g_n[k].raw) js_sb_put(vm, b, g_b + g_n[k].text, (int)g_n[k].text_len);
        else js_sb_str(vm, b, decoded(vm, g_b + g_n[k].text, (int)g_n[k].text_len));
        return;
    }
    if (g_n[k].type == DN_COMMENT || g_n[k].type == DN_DOCTYPE) return;
    for (c = g_n[k].first; c >= 0; c = g_n[c].next) text_of(vm, b, c, depth + 1);
}

static uint32_t text_content(js_vm_t *vm, int k) {
    js_sb_t b;
    js_sb_init(vm, &b);
    text_of(vm, &b, k, 0);
    return js_sb_finish(vm, &b);
}

static void remove_children(int k) {
    while (g_n[k].first >= 0) detach(g_n[k].first);
}

/* a JS string as HTML-encoded pool text (raw: as it is) */
static int text_node_from(js_vm_t *vm, uint32_t s, int raw) {
    int k = node_new(DN_TEXT);
    if (k < 0) return -1;
    g_n[k].raw = (uint8_t)raw;
    if (raw) {
        g_n[k].text = pool_put(JSTR(vm, s)->s, (int)JSTR(vm, s)->len, 0);
        g_n[k].text_len = JSTR(vm, s)->len;
    } else {
        js_sb_t b;
        uint32_t e;
        js_sb_init(vm, &b);
        sb_encode(vm, &b, JSTR(vm, s)->s, (int)JSTR(vm, s)->len, 0);
        e = js_sb_finish(vm, &b);
        g_n[k].text = pool_put(JSTR(vm, e)->s, (int)JSTR(vm, e)->len, 0);
        g_n[k].text_len = JSTR(vm, e)->len;
    }
    if (g_trunc) g_n[k].text_len = 0;
    return k;
}

static int dom_bound(js_vm_t *vm) {
    if (!g_trunc) return 0;
    js_throw_err(vm, JE_RANGE, "page DOM bound reached (nodes, attributes or text)");
    return 1;
}

static void set_text(js_vm_t *vm, int k, uint32_t s) {
    int t;
    if (g_n[k].type == DN_TEXT || g_n[k].type == DN_COMMENT) {
        int raw = g_n[k].raw;
        t = text_node_from(vm, s, raw || g_n[k].type == DN_COMMENT);
        if (t < 0) { dom_bound(vm); return; }
        g_n[k].text = g_n[t].text;
        g_n[k].text_len = g_n[t].text_len;
        return;
    }
    remove_children(k);
    if (!JSTR(vm, s)->len) return;
    t = text_node_from(vm, s, node_in_list(k, k_raw));
    if (t < 0 || dom_bound(vm)) return;
    insert_before(k, t, -1);
}

static uint32_t serialize_js(js_vm_t *vm, int k, int outer) {
    js_sb_t b;
    dout_t o;
    int c;
    js_sb_init(vm, &b);
    o.out = 0; o.n = 0; o.max = 0; o.vm = vm; o.sb = &b; o.skip_scripts = 0;
    if (outer) ser(&o, k, 0);
    else for (c = g_n[k].first; c >= 0; c = g_n[c].next) ser(&o, c, 1);
    return js_sb_finish(vm, &b);
}

static void set_inner_html(js_vm_t *vm, int k, uint32_t s) {
    int c;
    remove_children(k);
    if (node_in_list(k, k_raw)) { set_text(vm, k, s); return; }
    c = g_nn;
    parse_into(k, JSTR(vm, s)->s, (int)JSTR(vm, s)->len);
    for (; c < g_nn; c++) g_n[c].inserted = 1;     /* scripts in innerHTML do not run */
    dom_bound(vm);
}

static uint32_t attr_get_js(js_vm_t *vm, int k, const char *name, int *found) {
    const char *v;
    int vl;
    *found = attr_val(k, name, &v, &vl);
    if (!*found) return vm->s_empty;
    return decoded(vm, v, vl);
}

static void attr_set_js(js_vm_t *vm, int k, const char *name, int nl, uint32_t val) {
    js_sb_t b;
    uint32_t e;
    js_sb_init(vm, &b);
    sb_encode(vm, &b, JSTR(vm, val)->s, (int)JSTR(vm, val)->len, 1);
    e = js_sb_finish(vm, &b);
    attr_set_raw(k, name, nl, JSTR(vm, e)->s, (int)JSTR(vm, e)->len);
    dom_bound(vm);
}

static uint32_t tag_upper(js_vm_t *vm, int k) {
    char b[64];
    int i, n = (int)g_n[k].name_len;
    if (n > 63) n = 63;
    for (i = 0; i < n; i++) {
        char c = g_b[g_n[k].name + (uint32_t)i];
        b[i] = (c >= 'a' && c <= 'z') ? (char)(c - 32) : c;
    }
    return js_str_new(vm, b, n);
}

static jv child_list(js_vm_t *vm, int k, int elems_only) {
    uint32_t a = js_array_new(vm, 8);
    int c;
    if (!a) return jv_undef();
    for (c = g_n[k].first; c >= 0; c = g_n[c].next) {
        if (elems_only && g_n[c].type != DN_ELEM) continue;
        js_array_push(vm, a, node_obj(vm, c));
    }
    return jv_obj(a);
}

static jv sub_obj(js_vm_t *vm, int k, int kind, uint32_t proto) {
    uint32_t o = js_obj_new(vm, JS_CLASS_HOST, proto);
    if (!o) return jv_undef();
    JOBJ(vm, o)->x0 = (uint32_t)k;
    JOBJ(vm, o)->x1 = (uint32_t)kind;
    return jv_obj(o);
}

static int bool_attr(int k, const char *name) { return attr_find(k, name, dlen(name)) >= 0; }

/* ---- inline style ---- */

static int camel_to_kebab(js_vm_t *vm, uint32_t key, char *out, int max) {
    const js_str_t *s = JSTR(vm, key);
    int i, o = 0;
    if (s->len == 8 && s->s[0] == 'c' && s->s[1] == 's' && s->s[3] == 'F') { /* cssFloat */
        out[0] = 'f'; out[1] = 'l'; out[2] = 'o'; out[3] = 'a'; out[4] = 't';
        return 5;
    }
    for (i = 0; i < (int)s->len && o < max - 2; i++) {
        char c = s->s[i];
        if (c >= 'A' && c <= 'Z') { out[o++] = '-'; out[o++] = (char)(c + 32); }
        else out[o++] = c;
    }
    return o;
}

static int trim_l(const char *s, int a, int b) { while (a < b && is_space(s[a])) a++; return a; }
static int trim_r(const char *s, int a, int b) { while (b > a && is_space(s[b - 1])) b--; return b; }

/* walk the declarations of a style value: calls back with name and value spans */
static int style_next(const char *v, int vl, int *pos, int *na, int *nb, int *va, int *vb) {
    while (*pos < vl) {
        int st = *pos, e = st, colon = -1, paren = 0;
        while (e < vl && (paren || v[e] != ';')) {
            if (v[e] == '(') paren++;
            else if (v[e] == ')' && paren) paren--;
            else if (v[e] == ':' && colon < 0) colon = e;
            e++;
        }
        *pos = e < vl ? e + 1 : vl;
        if (colon < 0) continue;
        *na = trim_l(v, st, colon);
        *nb = trim_r(v, *na, colon);
        *va = trim_l(v, colon + 1, e);
        *vb = trim_r(v, *va, e);
        if (*nb > *na) return 1;
    }
    return 0;
}

static int span_eq_ci(const char *a, int al, const char *b, int bl) {
    int i;
    if (al != bl) return 0;
    for (i = 0; i < al; i++) if (lower(a[i]) != lower(b[i])) return 0;
    return 1;
}

static uint32_t style_get(js_vm_t *vm, int k, const char *prop, int pl) {
    const char *v;
    int vl, pos = 0, na, nb, va, vb;
    if (!attr_val(k, "style", &v, &vl)) return vm->s_empty;
    while (style_next(v, vl, &pos, &na, &nb, &va, &vb))
        if (span_eq_ci(v + na, nb - na, prop, pl)) return decoded(vm, v + va, vb - va);
    return vm->s_empty;
}

static void style_set(js_vm_t *vm, int k, const char *prop, int pl, uint32_t val) {
    js_sb_t b;
    const char *v;
    int vl = 0, pos = 0, na, nb, va, vb, any = 0;
    uint32_t plain;
    const js_str_t *vs = JSTR(vm, val);
    js_sb_init(vm, &b);
    if (attr_val(k, "style", &v, &vl)) {
        uint32_t cur = decoded(vm, v, vl);    /* the arena copy stays put while we build */
        v = JSTR(vm, cur)->s;
        vl = (int)JSTR(vm, cur)->len;
        while (style_next(v, vl, &pos, &na, &nb, &va, &vb)) {
            if (span_eq_ci(v + na, nb - na, prop, pl)) continue;
            if (any) js_sb_ch(vm, &b, ' ');
            js_sb_put(vm, &b, v + na, nb - na);
            js_sb_put(vm, &b, ": ", 2);
            js_sb_put(vm, &b, v + va, vb - va);
            js_sb_ch(vm, &b, ';');
            any = 1;
        }
    }
    if (vs->len) {
        if (any) js_sb_ch(vm, &b, ' ');
        js_sb_put(vm, &b, prop, pl);
        js_sb_put(vm, &b, ": ", 2);
        js_sb_put(vm, &b, vs->s, (int)vs->len);
        js_sb_ch(vm, &b, ';');
    }
    plain = js_sb_finish(vm, &b);
    if (vm->ctl) return;
    attr_set_js(vm, k, "style", 5, plain);
}

/* data-foo-bar for dataset.fooBar */
static int dataset_attr(js_vm_t *vm, uint32_t key, char *out, int max) {
    int n;
    out[0] = 'd'; out[1] = 'a'; out[2] = 't'; out[3] = 'a'; out[4] = '-';
    n = camel_to_kebab(vm, key, out + 5, max - 5);
    return n + 5;
}

/* ---- classList ---- */

static uint32_t class_value(js_vm_t *vm, int k) {
    int found;
    return attr_get_js(vm, k, "class", &found);
}

static int class_has(js_vm_t *vm, int k, uint32_t tok) {
    uint32_t cv = class_value(vm, k);
    return has_token(JSTR(vm, cv)->s, (int)JSTR(vm, cv)->len, JSTR(vm, tok)->s, (int)JSTR(vm, tok)->len);
}

/* rewrite the class attribute without tok, and with it when add */
static void class_edit(js_vm_t *vm, int k, uint32_t tok, int add) {
    uint32_t cv = class_value(vm, k);
    const char *v = JSTR(vm, cv)->s;
    int vl = (int)JSTR(vm, cv)->len, i = 0, any = 0;
    const js_str_t *t = JSTR(vm, tok);
    js_sb_t b;
    js_sb_init(vm, &b);
    while (i < vl) {
        int j;
        while (i < vl && is_space(v[i])) i++;
        j = i;
        while (j < vl && !is_space(v[j])) j++;
        if (j > i) {
            int q, same = (j - i == (int)t->len);
            for (q = 0; same && q < j - i; q++) if (v[i + q] != t->s[q]) same = 0;
            if (!same) {
                if (any) js_sb_ch(vm, &b, ' ');
                js_sb_put(vm, &b, v + i, j - i);
                any = 1;
            }
        }
        i = j;
    }
    if (add) {
        if (any) js_sb_ch(vm, &b, ' ');
        js_sb_put(vm, &b, t->s, (int)t->len);
    }
    attr_set_js(vm, k, "class", 5, js_sb_finish(vm, &b));
}

static int class_token_ok(js_vm_t *vm, uint32_t tok) {
    const js_str_t *t = JSTR(vm, tok);
    uint32_t i;
    if (!t->len) { js_throw_err(vm, JE_SYNTAX, "The token provided must not be empty."); return 0; }
    for (i = 0; i < t->len; i++)
        if (is_space(t->s[i])) { js_throw_err(vm, JE_SYNTAX, "The token provided contains HTML space characters."); return 0; }
    return 1;
}

static jv class_list_fn(js_vm_t *vm, jv thisv, jv *argv, int argc, uint32_t magic) {
    js_obj_t *o;
    int k, i;
    if (thisv.t != JT_OBJ || JOBJ(vm, thisv.a)->cls != JS_CLASS_HOST || JOBJ(vm, thisv.a)->x1 != HK_CLASSLIST)
        return js_throw_err(vm, JE_TYPE, "Illegal invocation: not a DOMTokenList");
    o = JOBJ(vm, thisv.a);
    k = (int)o->x0;
    switch (magic) {
    case 0: case 1:          /* add, remove */
        for (i = 0; i < argc; i++) {
            uint32_t t = js_to_string(vm, argv[i]);
            JCK(vm);
            if (!class_token_ok(vm, t)) return jv_undef();
            if (magic == 0 ? !class_has(vm, k, t) : class_has(vm, k, t)) class_edit(vm, k, t, magic == 0);
        }
        return jv_undef();
    case 2: {                /* toggle(token, force) */
        uint32_t t = js_to_string(vm, JARG(0));
        int has, want;
        JCK(vm);
        if (!class_token_ok(vm, t)) return jv_undef();
        has = class_has(vm, k, t);
        want = argc > 1 && argv[1].t != JT_UNDEF ? js_truthy(vm, argv[1]) : !has;
        if (want != has) class_edit(vm, k, t, want);
        return jv_bool(want);
    }
    case 3: {                /* contains */
        uint32_t t = js_to_string(vm, JARG(0));
        JCK(vm);
        return jv_bool(class_has(vm, k, t));
    }
    case 4: {                /* replace(old, new) */
        uint32_t a = js_to_string(vm, JARG(0)), b2;
        JCK(vm);
        b2 = js_to_string(vm, JARG(1));
        JCK(vm);
        if (!class_token_ok(vm, a) || !class_token_ok(vm, b2)) return jv_undef();
        if (!class_has(vm, k, a)) return jv_bool(0);
        class_edit(vm, k, a, 0);
        if (!class_has(vm, k, b2)) class_edit(vm, k, b2, 1);
        return jv_bool(1);
    }
    case 5: {                /* item(i) */
        uint32_t cv = class_value(vm, k);
        const char *v = JSTR(vm, cv)->s;
        int vl = (int)JSTR(vm, cv)->len, p = 0, n = 0;
        {
            int idx = js_to_int32(vm, JARG(0));
            JCK(vm);
            while (p < vl) {
                int j;
                while (p < vl && is_space(v[p])) p++;
                j = p;
                while (j < vl && !is_space(v[j])) j++;
                if (j > p) { if (n == idx) return jv_str(js_str_new(vm, v + p, j - p)); n++; }
                p = j;
            }
        }
        return jv_null();
    }
    default:                 /* toString */
        return jv_str(class_value(vm, k));
    }
}

static int class_count(js_vm_t *vm, int k) {
    uint32_t cv = class_value(vm, k);
    const char *v = JSTR(vm, cv)->s;
    int vl = (int)JSTR(vm, cv)->len, p = 0, n = 0;
    while (p < vl) {
        int j;
        while (p < vl && is_space(v[p])) p++;
        j = p;
        while (j < vl && !is_space(v[j])) j++;
        if (j > p) n++;
        p = j;
    }
    return n;
}

/* ---- property hooks ---- */

static int doc_title(int doc) {
    int k;
    for (k = walk_next(doc, doc); k >= 0; k = walk_next(k, doc)) if (name_is(k, "title")) return k;
    return -1;
}

static int is_connected(int k) {
    while (k >= 0 && g_n[k].parent >= 0) k = g_n[k].parent;
    return k == g_rt.doc;
}

static jv str_or_empty(js_vm_t *vm, uint32_t s) { return jv_str(s ? s : vm->s_empty); }

static jv select_value(js_vm_t *vm, int k) {
    int c, first = -1, found;
    for (c = walk_next(k, k); c >= 0; c = walk_next(c, k)) {
        if (!name_is(c, "option")) continue;
        if (first < 0) first = c;
        if (bool_attr(c, "selected")) { first = c; break; }
    }
    if (first < 0) return jv_str(vm->s_empty);
    if (bool_attr(first, "value")) return jv_str(attr_get_js(vm, first, "value", &found));
    return jv_str(text_content(vm, first));
}

static int node_get(js_vm_t *vm, int k, uint32_t key, jv *out) {
    int idx = key_index(key), found, c;
    dnode_t *d = &g_n[k];
    if (idx < 0) {
        int i;
        if (d->type != DN_ELEM) return 0;
        for (i = 0; k_reflect[i]; i++) {
            if (g_rt.reflect[i] != key) continue;
            *out = jv_str(attr_get_js(vm, k, i == 13 ? "for" : k_reflect[i], &found));
            return 1;
        }
        return 0;
    }
    switch (idx) {
    case K_nodeType: *out = jv_int(d->type); return 1;
    case K_nodeName:
        if (d->type == DN_ELEM) *out = jv_str(tag_upper(vm, k));
        else *out = jv_str(js_str_c(vm, d->type == DN_TEXT ? "#text" : d->type == DN_COMMENT ? "#comment"
                                         : d->type == DN_DOC ? "#document" : d->type == DN_FRAG ? "#document-fragment" : "html"));
        return 1;
    case K_tagName: *out = d->type == DN_ELEM ? jv_str(tag_upper(vm, k)) : jv_undef(); return 1;
    case K_id: if (d->type != DN_ELEM) return 0; *out = jv_str(attr_get_js(vm, k, "id", &found)); return 1;
    case K_className: if (d->type != DN_ELEM) return 0; *out = jv_str(attr_get_js(vm, k, "class", &found)); return 1;
    case K_textContent: case K_innerText:
        *out = d->type == DN_DOC ? jv_null() : jv_str(text_content(vm, k));
        return 1;
    case K_nodeValue: case K_data:
        if (d->type == DN_TEXT || d->type == DN_COMMENT) *out = jv_str(text_content(vm, k));
        else *out = idx == K_data ? jv_undef() : jv_null();
        if (d->type == DN_COMMENT) *out = jv_str(js_str_new(vm, g_b + d->text, (int)d->text_len));
        return 1;
    case K_length:
        if (d->type != DN_TEXT && d->type != DN_COMMENT) return 0;
        *out = jv_int((int32_t)JSTR(vm, text_content(vm, k))->len);
        return 1;
    case K_innerHTML: if (d->type != DN_ELEM && d->type != DN_FRAG) return 0; *out = jv_str(serialize_js(vm, k, 0)); return 1;
    case K_outerHTML: if (d->type != DN_ELEM) return 0; *out = jv_str(serialize_js(vm, k, 1)); return 1;
    case K_parentNode: *out = node_obj(vm, d->parent); return 1;
    case K_parentElement: *out = node_obj(vm, d->parent >= 0 && g_n[d->parent].type == DN_ELEM ? d->parent : -1); return 1;
    case K_children: *out = child_list(vm, k, 1); return 1;
    case K_childNodes: *out = child_list(vm, k, 0); return 1;
    case K_firstChild: *out = node_obj(vm, d->first); return 1;
    case K_lastChild: *out = node_obj(vm, d->last); return 1;
    case K_firstElementChild:
        for (c = d->first; c >= 0 && g_n[c].type != DN_ELEM; c = g_n[c].next) {}
        *out = node_obj(vm, c);
        return 1;
    case K_lastElementChild:
        for (c = d->last; c >= 0 && g_n[c].type != DN_ELEM; c = g_n[c].prev) {}
        *out = node_obj(vm, c);
        return 1;
    case K_nextSibling: *out = node_obj(vm, d->next); return 1;
    case K_previousSibling: *out = node_obj(vm, d->prev); return 1;
    case K_nextElementSibling: *out = node_obj(vm, next_elem(k)); return 1;
    case K_previousElementSibling: *out = node_obj(vm, prev_elem(k)); return 1;
    case K_childElementCount: {
        int n = 0;
        for (c = d->first; c >= 0; c = g_n[c].next) if (g_n[c].type == DN_ELEM) n++;
        *out = jv_int(n);
        return 1;
    }
    case K_style: if (d->type != DN_ELEM) return 0; *out = sub_obj(vm, k, HK_STYLE, g_rt.p_style); return 1;
    case K_classList: if (d->type != DN_ELEM) return 0; *out = sub_obj(vm, k, HK_CLASSLIST, g_rt.p_classlist); return 1;
    case K_dataset: if (d->type != DN_ELEM) return 0; *out = sub_obj(vm, k, 3, 0); return 1;
    case K_value:
        if (d->type != DN_ELEM) return 0;
        if (name_is(k, "textarea")) *out = jv_str(text_content(vm, k));
        else if (name_is(k, "select")) *out = select_value(vm, k);
        else if (name_is(k, "option") && !bool_attr(k, "value")) *out = jv_str(text_content(vm, k));
        else *out = jv_str(attr_get_js(vm, k, "value", &found));
        return 1;
    case K_checked: case K_disabled: case K_hidden: case K_selected:
        if (d->type != DN_ELEM) return 0;
        *out = jv_bool(bool_attr(k, k_names[idx]));
        return 1;
    case K_title:
        if (d->type == DN_DOC) {
            int t = doc_title(k);
            *out = t < 0 ? jv_str(vm->s_empty) : jv_str(text_content(vm, t));
            return 1;
        }
        if (d->type != DN_ELEM) return 0;
        *out = jv_str(attr_get_js(vm, k, "title", &found));
        return 1;
    case K_body: case K_head: case K_documentElement: {
        int html;
        if (d->type != DN_DOC) return 0;
        html = find_child(k, "html");
        *out = node_obj(vm, idx == K_documentElement ? html : html < 0 ? -1 : find_child(html, idx == K_body ? "body" : "head"));
        return 1;
    }
    case K_readyState: if (d->type != DN_DOC) return 0; *out = jv_str(js_str_c(vm, g_rt.ready)); return 1;
    case K_cookie: if (d->type != DN_DOC) return 0; *out = str_or_empty(vm, g_rt.cookie); return 1;
    case K_URL: if (d->type != DN_DOC) return 0; *out = js_get_c(vm, jv_obj(g_rt.location), "href"); return 1;
    case K_location: if (d->type != DN_DOC) return 0; *out = jv_obj(g_rt.location); return 1;
    case K_defaultView: if (d->type != DN_DOC) return 0; *out = jv_obj(vm->global); return 1;
    case K_ownerDocument: *out = d->type == DN_DOC ? jv_null() : node_obj(vm, g_rt.doc); return 1;
    case K_isConnected: *out = jv_bool(is_connected(k)); return 1;
    case K_offsetWidth: case K_offsetHeight: case K_clientWidth: case K_clientHeight: case K_scrollTop:
        *out = jv_int(0);       /* no layout in the script pass */
        return 1;
    default: return 0;
    }
}

static int node_set(js_vm_t *vm, int k, uint32_t key, jv val) {
    int idx = key_index(key);
    dnode_t *d = &g_n[k];
    uint32_t s;
    if (idx < 0) {
        int i;
        if (d->type != DN_ELEM) return 0;
        for (i = 0; k_reflect[i]; i++) {
            if (g_rt.reflect[i] != key) continue;
            s = js_to_string(vm, val);
            if (vm->ctl) return 1;
            if (i == 13) attr_set_js(vm, k, "for", 3, s);
            else attr_set_js(vm, k, k_reflect[i], dlen(k_reflect[i]), s);
            return 1;
        }
        return 0;
    }
    switch (idx) {
    case K_textContent: case K_innerText: case K_nodeValue: case K_data:
        if (d->type == DN_DOC) return 1;
        if ((idx == K_nodeValue || idx == K_data) && d->type == DN_ELEM) return idx == K_nodeValue;
        s = val.t == JT_NULL ? vm->s_empty : js_to_string(vm, val);
        if (vm->ctl) return 1;
        set_text(vm, k, s);
        return 1;
    case K_innerHTML:
        if (d->type != DN_ELEM && d->type != DN_FRAG) return 0;
        s = val.t == JT_NULL ? vm->s_empty : js_to_string(vm, val);
        if (vm->ctl) return 1;
        set_inner_html(vm, k, s);
        return 1;
    case K_outerHTML: {
        int p = d->parent, frag, c, nx;
        if (d->type != DN_ELEM) return 0;
        if (p < 0) return 1;
        s = js_to_string(vm, val);
        if (vm->ctl) return 1;
        frag = node_new(DN_FRAG);
        if (frag < 0) { dom_bound(vm); return 1; }
        c = g_nn;
        parse_into(frag, JSTR(vm, s)->s, (int)JSTR(vm, s)->len);
        for (; c < g_nn; c++) g_n[c].inserted = 1;
        for (c = g_n[frag].first; c >= 0; c = nx) { nx = g_n[c].next; insert_before(p, c, k); }
        detach(k);
        dom_bound(vm);
        return 1;
    }
    case K_id: case K_className: case K_title: case K_value: {
        const char *an = idx == K_id ? "id" : idx == K_className ? "class" : idx == K_title ? "title" : "value";
        if (d->type == DN_DOC && idx == K_title) {
            int t = doc_title(k);
            s = js_to_string(vm, val);
            if (vm->ctl) return 1;
            if (t < 0) {
                int html = find_child(k, "html"), head = html < 0 ? -1 : find_child(html, "head");
                if (head < 0) return 1;
                t = elem_new("title");
                if (t < 0) { dom_bound(vm); return 1; }
                g_n[t].inserted = 1;
                insert_before(head, t, -1);
            }
            set_text(vm, t, s);
            return 1;
        }
        if (d->type != DN_ELEM) return 0;
        s = js_to_string(vm, val);
        if (vm->ctl) return 1;
        if (idx == K_value && name_is(k, "textarea")) set_text(vm, k, s);
        else attr_set_js(vm, k, an, dlen(an), s);
        return 1;
    }
    case K_checked: case K_disabled: case K_hidden: case K_selected:
        if (d->type != DN_ELEM) return 0;
        if (js_truthy(vm, val)) { if (!bool_attr(k, k_names[idx])) attr_set_raw(k, k_names[idx], dlen(k_names[idx]), "", 0); }
        else attr_remove(k, k_names[idx], dlen(k_names[idx]));
        dom_bound(vm);
        return 1;
    case K_cookie: {
        /* keep "name=value" pairs (attributes after ';' dropped), joined with "; " */
        uint32_t c2, cur = g_rt.cookie ? g_rt.cookie : vm->s_empty;
        const char *v;
        int e = 0, eq, n, pos = 0;
        js_sb_t b;
        if (d->type != DN_DOC) return 0;
        c2 = js_to_string(vm, val);
        if (vm->ctl) return 1;
        v = JSTR(vm, c2)->s;
        n = (int)JSTR(vm, c2)->len;
        while (e < n && v[e] != ';') e++;
        for (eq = 0; eq < e && v[eq] != '='; eq++) {}
        if (eq >= e) return 1;
        js_sb_init(vm, &b);
        {   /* existing pairs other than this name */
            const char *cv = JSTR(vm, cur)->s;
            int cl = (int)JSTR(vm, cur)->len, any = 0, na = trim_l(v, 0, eq), nb = trim_r(v, na, eq);
            while (pos < cl) {
                int st = trim_l(cv, pos, cl), en = st, ceq, ce;
                while (en < cl && cv[en] != ';') en++;
                for (ceq = st; ceq < en && cv[ceq] != '='; ceq++) {}
                ce = trim_r(cv, st, ceq);
                if (en > st && !span_eq_ci(cv + st, ce - st, v + na, nb - na)) {
                    if (any) js_sb_put(vm, &b, "; ", 2);
                    js_sb_put(vm, &b, cv + st, trim_r(cv, st, en) - st);
                    any = 1;
                }
                pos = en + 1;
            }
            if (any) js_sb_put(vm, &b, "; ", 2);
        }
        js_sb_put(vm, &b, v + trim_l(v, 0, e), trim_r(v, 0, e) - trim_l(v, 0, e));
        g_rt.cookie = js_sb_finish(vm, &b);
        return 1;
    }
    case K_style:
        if (d->type != DN_ELEM) return 0;
        s = js_to_string(vm, val);
        if (vm->ctl) return 1;
        attr_set_js(vm, k, "style", 5, s);
        return 1;
    case K_location:
        if (d->type != DN_DOC) return 0;
        js_log_put(vm, "warn: navigation is not supported in the script pass\n", 53);
        return 1;
    case K_nodeType: case K_nodeName: case K_tagName: case K_parentNode: case K_parentElement: case K_children:
    case K_childNodes: case K_firstChild: case K_lastChild: case K_firstElementChild: case K_lastElementChild:
    case K_nextSibling: case K_previousSibling: case K_nextElementSibling: case K_previousElementSibling:
    case K_childElementCount: case K_classList: case K_body: case K_head: case K_documentElement: case K_readyState:
    case K_ownerDocument: case K_URL: case K_defaultView: case K_isConnected: case K_dataset:
    case K_offsetWidth: case K_offsetHeight: case K_clientWidth: case K_clientHeight:
        return 1;     /* read-only: ignored as in sloppy-mode browsers */
    default: return 0;
    }
}

static int dom_get(js_vm_t *vm, jv obj, uint32_t key, jv *out) {
    js_obj_t *o = JOBJ(vm, obj.a);
    int k = (int)o->x0;
    if (k < 0 || k >= g_nn) return 0;
    switch (o->x1) {
    case HK_NODE: return node_get(vm, k, key, out);
    case HK_STYLE: {
        char b[64];
        int n;
        if (key == g_rt.key[K_cssText]) { int f; *out = jv_str(attr_get_js(vm, k, "style", &f)); return 1; }
        if (js_own_prop(vm, g_rt.p_style, key) || js_own_prop(vm, vm->p_object, key)) return 0;
        n = camel_to_kebab(vm, key, b, (int)sizeof(b));
        *out = jv_str(style_get(vm, k, b, n));
        return 1;
    }
    case HK_CLASSLIST: {
        uint32_t idx;
        if (key == g_rt.key[K_length]) { *out = jv_int(class_count(vm, k)); return 1; }
        if (key == g_rt.key[K_value]) { *out = jv_str(class_value(vm, k)); return 1; }
        if (js_str_index_key(vm, key, &idx)) {
            jv a = jv_int((int32_t)idx);
            *out = class_list_fn(vm, obj, &a, 1, 5);
            if (out->t == JT_NULL) *out = jv_undef();
            return 1;
        }
        return 0;
    }
    case 3: {      /* dataset */
        char b[72];
        int n, f;
        uint32_t v;
        if (js_own_prop(vm, vm->p_object, key)) return 0;
        n = dataset_attr(vm, key, b, (int)sizeof(b));
        b[n] = 0;
        v = attr_get_js(vm, k, b, &f);
        *out = f ? jv_str(v) : jv_undef();
        return 1;
    }
    default: return 0;
    }
}

static int dom_set(js_vm_t *vm, jv obj, uint32_t key, jv val) {
    js_obj_t *o = JOBJ(vm, obj.a);
    int k = (int)o->x0;
    uint32_t s;
    if (k < 0 || k >= g_nn) return 0;
    switch (o->x1) {
    case HK_NODE: return node_set(vm, k, key, val);
    case HK_STYLE: {
        char b[64];
        int n;
        s = val.t == JT_NULL ? vm->s_empty : js_to_string(vm, val);
        if (vm->ctl) return 1;
        if (key == g_rt.key[K_cssText]) { attr_set_js(vm, k, "style", 5, s); return 1; }
        n = camel_to_kebab(vm, key, b, (int)sizeof(b));
        style_set(vm, k, b, n, s);
        return 1;
    }
    case HK_CLASSLIST:
        if (key == g_rt.key[K_value]) {
            s = js_to_string(vm, val);
            if (!vm->ctl) attr_set_js(vm, k, "class", 5, s);
            return 1;
        }
        return 1;
    case 3: {
        char b[72];
        int n;
        s = js_to_string(vm, val);
        if (vm->ctl) return 1;
        n = dataset_attr(vm, key, b, (int)sizeof(b));
        attr_set_js(vm, k, b, n, s);
        return 1;
    }
    default: return 0;
    }
}

/* ---- node and document methods ---- */

enum {
    M_getAttribute, M_setAttribute, M_removeAttribute, M_hasAttribute, M_appendChild, M_removeChild, M_insertBefore,
    M_replaceChild, M_remove, M_append, M_prepend, M_before, M_after, M_cloneNode, M_contains, M_querySelector,
    M_querySelectorAll, M_getElementsByTagName, M_getElementsByClassName, M_getElementById, M_createElement,
    M_createTextNode, M_createComment, M_createDocumentFragment, M_matches, M_closest, M_insertAdjacentHTML,
    M_hasChildNodes, M_write, M_writeln, M_getElementsByName, M_toggleAttribute, M_getBoundingClientRect,
    M_insertAdjacentText, M_replaceWith, M_focus, M_blur, M_replaceChildren, M_hasAttributes, M_getAttributeNames,
    M_COUNT
};
static const char *const m_names[M_COUNT] = {
    "getAttribute", "setAttribute", "removeAttribute", "hasAttribute", "appendChild", "removeChild", "insertBefore",
    "replaceChild", "remove", "append", "prepend", "before", "after", "cloneNode", "contains", "querySelector",
    "querySelectorAll", "getElementsByTagName", "getElementsByClassName", "getElementById", "createElement",
    "createTextNode", "createComment", "createDocumentFragment", "matches", "closest", "insertAdjacentHTML",
    "hasChildNodes", "write", "writeln", "getElementsByName", "toggleAttribute", "getBoundingClientRect",
    "insertAdjacentText", "replaceWith", "focus", "blur", "replaceChildren", "hasAttributes", "getAttributeNames"
};
static const uint8_t m_nargs[M_COUNT] = { 1, 2, 1, 1, 1, 1, 2, 2, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 2,
                                          0, 0, 0, 1, 1, 0, 2, 0, 0, 0, 0, 0, 0 };

static int arg_node(js_vm_t *vm, jv v) {
    int k = obj_node(vm, v);
    if (k < 0) js_throw_err(vm, JE_TYPE, "parameter is not of type 'Node'");
    return k;
}

/* the node to insert for a value: a node, or a text node for anything else (append/prepend/before/after) */
static int node_or_text(js_vm_t *vm, jv v) {
    int k = obj_node(vm, v);
    uint32_t s;
    if (k >= 0) return k;
    s = js_to_string(vm, v);
    if (vm->ctl) return -1;
    k = text_node_from(vm, s, 0);
    if (k < 0) dom_bound(vm);
    return k;
}

/* insert k (a fragment moves its children) under parent before ref */
static int dom_insert(js_vm_t *vm, int parent, int k, int ref) {
    if (g_n[parent].type != DN_ELEM && g_n[parent].type != DN_DOC && g_n[parent].type != DN_FRAG) {
        js_throw_err(vm, JE_TYPE, "This node type does not support this method.");
        return -1;
    }
    if (is_ancestor(k, parent)) {
        js_throw_err(vm, JE_ERROR, "HierarchyRequestError: The new child element contains the parent.");
        return -1;
    }
    if (ref >= 0 && g_n[ref].parent != parent) {
        js_throw_err(vm, JE_ERROR, "NotFoundError: The node before which the new node is to be inserted is not a child of this node.");
        return -1;
    }
    if (ref == k) ref = g_n[k].next;
    if (g_n[k].type == DN_FRAG) {
        while (g_n[k].first >= 0) insert_before(parent, g_n[k].first, ref);
        return 0;
    }
    if (g_n[k].type == DN_DOC) { js_throw_err(vm, JE_ERROR, "HierarchyRequestError: a document cannot be inserted."); return -1; }
    insert_before(parent, k, ref);
    return 0;
}

static int clone_node(int k, int deep, int depth) {
    int c, a, nk;
    if (depth > PSTACK) return -1;
    nk = node_new(g_n[k].type == DN_DOC ? DN_FRAG : g_n[k].type);
    if (nk < 0) return -1;
    g_n[nk].raw = g_n[k].raw;
    g_n[nk].inserted = 1;
    g_n[nk].name = g_n[k].name;
    g_n[nk].name_len = g_n[k].name_len;
    g_n[nk].text = g_n[k].text;
    g_n[nk].text_len = g_n[k].text_len;
    for (a = g_n[k].attr; a >= 0; a = g_a[a].next) {
        int na, last;
        if (g_na >= WF_DOM_ATTRS) { g_trunc = 1; return nk; }
        na = g_na++;
        g_a[na].name = g_a[a].name;
        g_a[na].name_len = g_a[a].name_len;
        g_a[na].val = g_a[a].val;
        g_a[na].val_len = g_a[a].val_len;
        g_a[na].next = -1;
        if (g_n[nk].attr < 0) g_n[nk].attr = na;
        else { for (last = g_n[nk].attr; g_a[last].next >= 0; last = g_a[last].next) {} g_a[last].next = na; }
    }
    if (deep)
        for (c = g_n[k].first; c >= 0; c = g_n[c].next) {
            int cc = clone_node(c, 1, depth + 1);
            if (cc < 0) break;
            insert_before(nk, cc, -1);
        }
    return nk;
}

static jv query(js_vm_t *vm, int root, jv selv, int all) {
    uint32_t s = js_to_string(vm, selv), arr = 0;
    const char *sp;
    int n, k;
    if (vm->ctl) return jv_undef();
    sp = JSTR(vm, s)->s;
    n = (int)JSTR(vm, s)->len;
    if (!sel_valid(sp, n)) return js_throw_err2(vm, JE_SYNTAX, "Failed to execute 'querySelector': not a valid selector: ", s);
    if (all) { arr = js_array_new(vm, 8); if (!arr) return jv_undef(); }
    for (k = walk_next(root, root); k >= 0; k = walk_next(k, root)) {
        if (g_n[k].type != DN_ELEM) continue;
        js_step(vm);
        if (vm->ctl) return jv_undef();
        if (sel_match(k, sp, n, g_n[root].type == DN_DOC ? -1 : -1)) {
            if (!all) return node_obj(vm, k);
            js_array_push(vm, arr, node_obj(vm, k));
        }
    }
    return all ? jv_obj(arr) : jv_null();
}

/* collect elements under root by tag ("*" all), class tokens, or name attribute */
static jv collect(js_vm_t *vm, int root, uint32_t s, int how) {
    uint32_t arr = js_array_new(vm, 8);
    const js_str_t *t = JSTR(vm, s);
    int k;
    if (!arr) return jv_undef();
    for (k = walk_next(root, root); k >= 0; k = walk_next(k, root)) {
        int ok = 0;
        if (g_n[k].type != DN_ELEM) continue;
        if (how == 0) {
            ok = (t->len == 1 && t->s[0] == '*') || span_eq_ci(g_b + g_n[k].name, (int)g_n[k].name_len, t->s, (int)t->len);
        } else if (how == 1) {
            const char *v;
            int vl, p = 0;
            ok = attr_val(k, "class", &v, &vl);
            while (ok && p < (int)t->len) {     /* every token of s */
                int j;
                while (p < (int)t->len && is_space(t->s[p])) p++;
                j = p;
                while (j < (int)t->len && !is_space(t->s[j])) j++;
                if (j > p && !has_token(v, vl, t->s + p, j - p)) ok = 0;
                p = j;
            }
        } else {
            const char *v;
            int vl;
            ok = attr_val(k, "name", &v, &vl) && vl == (int)t->len && span_eq_ci(v, vl, t->s, vl);
        }
        if (ok) js_array_push(vm, arr, node_obj(vm, k));
    }
    return jv_obj(arr);
}

static int valid_name(js_vm_t *vm, uint32_t s) {
    const js_str_t *t = JSTR(vm, s);
    uint32_t i;
    if (!t->len) return 0;
    for (i = 0; i < t->len; i++) {
        char c = t->s[i];
        if (is_space(c) || c == '<' || c == '>' || c == '/' || c == '=' || c == '"' || c == '\'') return 0;
    }
    return 1;
}

static void doc_write(js_vm_t *vm, jv *argv, int argc, int nl) {
    int i;
    for (i = 0; i < argc; i++) {
        uint32_t s = js_to_string(vm, argv[i]);
        if (vm->ctl) return;
        if (g_rt.writing) js_sb_str(vm, &g_rt.write_buf, s);
    }
    if (nl && g_rt.writing) js_sb_ch(vm, &g_rt.write_buf, '\n');
    if (g_rt.st) g_rt.st->writes++;
    if (!g_rt.writing) js_log_put(vm, "warn: document.write after load is ignored\n", 43);
}

static jv insert_adjacent(js_vm_t *vm, int k, jv posv, jv what, int as_text) {
    uint32_t pos = js_to_string(vm, posv), s;
    const js_str_t *p;
    int frag, c, nx, parent, ref, mode;
    if (vm->ctl) return jv_undef();
    s = js_to_string(vm, what);
    if (vm->ctl) return jv_undef();
    p = JSTR(vm, pos);
    if (span_eq_ci(p->s, (int)p->len, "beforebegin", 11)) mode = 0;
    else if (span_eq_ci(p->s, (int)p->len, "afterbegin", 10)) mode = 1;
    else if (span_eq_ci(p->s, (int)p->len, "beforeend", 9)) mode = 2;
    else if (span_eq_ci(p->s, (int)p->len, "afterend", 8)) mode = 3;
    else return js_throw_err2(vm, JE_SYNTAX, "The value provided is not one of 'beforeBegin', 'afterBegin', 'beforeEnd', or 'afterEnd': ", pos);
    if (mode == 0 || mode == 3) { parent = g_n[k].parent; ref = mode == 0 ? k : g_n[k].next; if (parent < 0) return jv_undef(); }
    else { parent = k; ref = mode == 1 ? g_n[k].first : -1; }
    frag = node_new(DN_FRAG);
    if (frag < 0) { dom_bound(vm); return jv_undef(); }
    if (as_text) {
        c = text_node_from(vm, s, 0);
        if (c >= 0) insert_before(frag, c, -1);
    } else {
        c = g_nn;
        parse_into(frag, JSTR(vm, s)->s, (int)JSTR(vm, s)->len);
        for (; c < g_nn; c++) g_n[c].inserted = 1;
    }
    for (c = g_n[frag].first; c >= 0; c = nx) { nx = g_n[c].next; insert_before(parent, c, ref); }
    dom_bound(vm);
    return jv_undef();
}

static jv node_method(js_vm_t *vm, jv thisv, jv *argv, int argc, uint32_t magic) {
    int k = this_node(vm, thisv), c, i, found;
    uint32_t s;
    if (k < 0) return jv_undef();
    js_step(vm);
    JCK(vm);
    switch (magic) {
    case M_getAttribute: case M_hasAttribute: case M_removeAttribute: case M_toggleAttribute: {
        const js_str_t *t;
        char nm[64];
        int nl, has;
        s = js_to_string(vm, JARG(0));
        JCK(vm);
        if (g_n[k].type != DN_ELEM) return magic == M_getAttribute ? jv_null() : jv_bool(0);
        t = JSTR(vm, s);
        nl = t->len < 63 ? (int)t->len : 63;
        for (i = 0; i < nl; i++) nm[i] = lower(t->s[i]);
        nm[nl] = 0;
        has = attr_find(k, nm, nl) >= 0;
        if (magic == M_hasAttribute) return jv_bool(has);
        if (magic == M_getAttribute) return has ? jv_str(attr_get_js(vm, k, nm, &found)) : jv_null();
        if (magic == M_removeAttribute) { attr_remove(k, nm, nl); return jv_undef(); }
        {
            int want = argc > 1 && argv[1].t != JT_UNDEF ? js_truthy(vm, argv[1]) : !has;
            if (!valid_name(vm, s)) return js_throw_err2(vm, JE_ERROR, "InvalidCharacterError: not a valid attribute name: ", s);
            if (want && !has) attr_set_raw(k, nm, nl, "", 0);
            if (!want && has) attr_remove(k, nm, nl);
            dom_bound(vm);
            return jv_bool(want);
        }
    }
    case M_setAttribute: {
        uint32_t v;
        s = js_to_string(vm, JARG(0));
        JCK(vm);
        v = js_to_string(vm, JARG(1));
        JCK(vm);
        if (g_n[k].type != DN_ELEM) return jv_undef();
        if (!valid_name(vm, s)) return js_throw_err2(vm, JE_ERROR, "InvalidCharacterError: not a valid attribute name: ", s);
        attr_set_js(vm, k, JSTR(vm, s)->s, (int)JSTR(vm, s)->len, v);
        return jv_undef();
    }
    case M_hasAttributes: return jv_bool(g_n[k].type == DN_ELEM && g_n[k].attr >= 0);
    case M_getAttributeNames: {
        uint32_t arr = js_array_new(vm, 4);
        int a;
        if (!arr) return jv_undef();
        for (a = g_n[k].attr; a >= 0; a = g_a[a].next) js_array_push(vm, arr, jv_str(js_str_new(vm, g_b + g_a[a].name, (int)g_a[a].name_len)));
        return jv_obj(arr);
    }
    case M_appendChild: case M_insertBefore: {
        int ch = arg_node(vm, JARG(0)), ref = -1;
        if (ch < 0) return jv_undef();
        if (magic == M_insertBefore && argc > 1 && argv[1].t != JT_NULL && argv[1].t != JT_UNDEF) {
            ref = arg_node(vm, argv[1]);
            if (ref < 0) return jv_undef();
        }
        if (dom_insert(vm, k, ch, ref) < 0) return jv_undef();
        return JARG(0);
    }
    case M_removeChild: {
        int ch = arg_node(vm, JARG(0));
        if (ch < 0) return jv_undef();
        if (g_n[ch].parent != k) return js_throw_err(vm, JE_ERROR, "NotFoundError: The node to be removed is not a child of this node.");
        detach(ch);
        return JARG(0);
    }
    case M_replaceChild: {
        int nw = arg_node(vm, JARG(0)), old;
        if (nw < 0) return jv_undef();
        old = arg_node(vm, JARG(1));
        if (old < 0) return jv_undef();
        if (g_n[old].parent != k) return js_throw_err(vm, JE_ERROR, "NotFoundError: The node to be replaced is not a child of this node.");
        if (nw != old) {
            if (dom_insert(vm, k, nw, old) < 0) return jv_undef();
            detach(old);
        }
        return JARG(1);
    }
    case M_remove: detach(k); return jv_undef();
    case M_append: case M_prepend: case M_replaceChildren: {
        int ref = magic == M_prepend ? g_n[k].first : -1;
        if (magic == M_replaceChildren) remove_children(k);
        for (i = 0; i < argc; i++) {
            c = node_or_text(vm, argv[i]);
            if (c < 0 || dom_insert(vm, k, c, ref) < 0) return jv_undef();
        }
        return jv_undef();
    }
    case M_before: case M_after: case M_replaceWith: {
        int p = g_n[k].parent, ref = magic == M_before ? k : g_n[k].next;
        if (p < 0) return jv_undef();
        for (i = 0; i < argc; i++) {
            c = node_or_text(vm, argv[i]);
            if (c < 0) return jv_undef();
            if (c == ref) { ref = g_n[c].next; continue; }
            if (dom_insert(vm, p, c, ref) < 0) return jv_undef();
        }
        if (magic == M_replaceWith && g_n[k].parent == p) detach(k);
        return jv_undef();
    }
    case M_cloneNode: {
        int nk = clone_node(k, js_truthy(vm, JARG(0)), 0);
        if (nk < 0 || dom_bound(vm)) return jv_undef();
        return node_obj(vm, nk);
    }
    case M_contains: { int o = obj_node(vm, JARG(0)); return jv_bool(o >= 0 && is_ancestor(k, o)); }
    case M_hasChildNodes: return jv_bool(g_n[k].first >= 0);
    case M_querySelector: case M_querySelectorAll: return query(vm, k, JARG(0), magic == M_querySelectorAll);
    case M_getElementsByTagName: case M_getElementsByClassName: case M_getElementsByName:
        s = js_to_string(vm, JARG(0));
        JCK(vm);
        return collect(vm, magic == M_getElementsByName ? g_rt.doc : k, s,
                       magic == M_getElementsByTagName ? 0 : magic == M_getElementsByClassName ? 1 : 2);
    case M_getElementById: {
        const js_str_t *t;
        s = js_to_string(vm, JARG(0));
        JCK(vm);
        t = JSTR(vm, s);
        for (c = walk_next(k, k); c >= 0; c = walk_next(c, k)) {
            const char *v;
            int vl;
            if (g_n[c].type == DN_ELEM && attr_val(c, "id", &v, &vl)) {
                uint32_t dv = decoded(vm, v, vl);
                if (js_str_eq(vm, dv, s)) return node_obj(vm, c);
            }
        }
        (void)t;
        return jv_null();
    }
    case M_createElement: {
        char nm[64];
        const js_str_t *t;
        int nk;
        s = js_to_string(vm, JARG(0));
        JCK(vm);
        if (!valid_name(vm, s) || JSTR(vm, s)->len > 63)
            return js_throw_err2(vm, JE_ERROR, "InvalidCharacterError: The tag name provided is not a valid name: ", s);
        t = JSTR(vm, s);
        for (i = 0; i < (int)t->len; i++) nm[i] = lower(t->s[i]);
        nm[t->len] = 0;
        nk = elem_new(nm);
        if (nk < 0 || dom_bound(vm)) return jv_undef();
        g_n[nk].inserted = 1;          /* created scripts do not run */
        return node_obj(vm, nk);
    }
    case M_createTextNode: case M_createComment: {
        int nk;
        s = js_to_string(vm, JARG(0));
        JCK(vm);
        nk = text_node_from(vm, s, magic == M_createComment);
        if (nk < 0 || dom_bound(vm)) return jv_undef();
        if (magic == M_createComment) g_n[nk].type = DN_COMMENT;
        return node_obj(vm, nk);
    }
    case M_createDocumentFragment: {
        int nk = node_new(DN_FRAG);
        if (nk < 0 || dom_bound(vm)) return jv_undef();
        return node_obj(vm, nk);
    }
    case M_matches: case M_closest: {
        const char *sp;
        int n;
        s = js_to_string(vm, JARG(0));
        JCK(vm);
        sp = JSTR(vm, s)->s;
        n = (int)JSTR(vm, s)->len;
        if (!sel_valid(sp, n)) return js_throw_err2(vm, JE_SYNTAX, "not a valid selector: ", s);
        if (magic == M_matches) return jv_bool(sel_match(k, sp, n, -1));
        for (c = k; c >= 0; c = g_n[c].parent) if (g_n[c].type == DN_ELEM && sel_match(c, sp, n, -1)) return node_obj(vm, c);
        return jv_null();
    }
    case M_insertAdjacentHTML: case M_insertAdjacentText:
        return insert_adjacent(vm, k, JARG(0), JARG(1), magic == M_insertAdjacentText);
    case M_write: case M_writeln:
        if (g_n[k].type != DN_DOC) return js_throw_err(vm, JE_TYPE, "write is a document method");
        doc_write(vm, argv, argc, magic == M_writeln);
        return jv_undef();
    case M_getBoundingClientRect: {
        static const char *const f[] = { "x", "y", "width", "height", "top", "right", "bottom", "left", 0 };
        uint32_t o = js_obj_new(vm, JS_CLASS_OBJECT, vm->p_object);
        if (!o) return jv_undef();
        for (i = 0; f[i]; i++) js_define_c(vm, o, f[i], jv_int(0), P_DEFAULT);
        return jv_obj(o);
    }
    case M_focus: case M_blur: return jv_undef();
    default: return jv_undef();
    }
}

/* style methods: setProperty, getPropertyValue, removeProperty */
static jv style_method(js_vm_t *vm, jv thisv, jv *argv, int argc, uint32_t magic) {
    int k;
    uint32_t p, v;
    (void)argc;
    if (thisv.t != JT_OBJ || JOBJ(vm, thisv.a)->cls != JS_CLASS_HOST || JOBJ(vm, thisv.a)->x1 != HK_STYLE)
        return js_throw_err(vm, JE_TYPE, "Illegal invocation: not a CSSStyleDeclaration");
    k = (int)JOBJ(vm, thisv.a)->x0;
    p = js_to_string(vm, JARG(0));
    JCK(vm);
    if (magic == 1) return jv_str(style_get(vm, k, JSTR(vm, p)->s, (int)JSTR(vm, p)->len));
    if (magic == 2) {
        uint32_t old = style_get(vm, k, JSTR(vm, p)->s, (int)JSTR(vm, p)->len);
        style_set(vm, k, JSTR(vm, p)->s, (int)JSTR(vm, p)->len, vm->s_empty);
        return jv_str(old);
    }
    v = JARG(1).t == JT_NULL || JARG(1).t == JT_UNDEF ? vm->s_empty : js_to_string(vm, JARG(1));
    JCK(vm);
    style_set(vm, k, JSTR(vm, p)->s, (int)JSTR(vm, p)->len, v);
    return jv_undef();
}

/* ---- events ---- */

static uint32_t g_p_event;

static void logc(js_vm_t *vm, const char *s) { js_log_put(vm, s, dlen(s)); }

/* report an exception thrown by a listener or callback and go on (as browsers do) */
static void report_throw(js_vm_t *vm) {
    char b[200];
    int n;
    jv e;
    if (vm->ctl != CTL_THROW) return;
    e = vm->exc;
    vm->ctl = CTL_NONE;
    n = js_display(vm, e, b, (int)sizeof(b) - 1, 1);
    if (vm->ctl) { vm->ctl = CTL_NONE; n = 0; }
    logc(vm, "error: Uncaught ");
    js_log_put(vm, b, n);
    logc(vm, "\n");
    if (g_rt.st) {
        g_rt.st->failed++;
        if (!g_rt.st->first_err[0]) {
            const char *p = "Uncaught ";
            int i = 0, j;
            while (p[i] && i < 159) { g_rt.st->first_err[i] = p[i]; i++; }
            for (j = 0; j < n && i < 159; j++) g_rt.st->first_err[i++] = b[j];
            g_rt.st->first_err[i] = 0;
        }
    }
}

/* the listener array of a target (node object or the global object) */
static uint32_t *listeners_of(js_vm_t *vm, jv target) {
    int k = obj_node(vm, target);
    if (k >= 0) return &g_n[k].listeners;
    if ((target.t == JT_OBJ && target.a == vm->global) || target.t == JT_UNDEF) return &g_rt.win_listeners;
    return 0;
}

static jv listener_fn(js_vm_t *vm, jv thisv, jv *argv, int argc, uint32_t magic) {
    uint32_t *lp = listeners_of(vm, thisv), i, len;
    uint32_t type;
    jv fn = JARG(1);
    int once = 0;
    if (!lp) return js_throw_err(vm, JE_TYPE, "Illegal invocation: not an event target");
    type = js_to_key(vm, JARG(0));
    JCK(vm);
    if (!js_is_callable(vm, fn) && !jv_is_obj(fn)) return jv_undef();
    if (argc > 2 && jv_is_obj(argv[2])) {
        once = js_truthy(vm, js_get_c(vm, argv[2], "once"));
        JCK(vm);
    }
    if (!*lp) { *lp = js_array_new(vm, 4); if (!*lp) return jv_undef(); }
    len = JOBJ(vm, *lp)->alen;
    for (i = 0; i < len; i++) {
        jv e = js_array_get(vm, *lp, i);
        if (e.t != JT_OBJ) continue;
        if (js_array_get(vm, e.a, 0).a == type && js_strict_eq(vm, js_array_get(vm, e.a, 1), fn)) {
            if (magic) js_array_set(vm, *lp, i, jv_null());     /* removeEventListener */
            return jv_undef();
        }
    }
    if (!magic) {
        uint32_t e = js_array_new(vm, 3);
        if (!e) return jv_undef();
        js_array_push(vm, e, jv_str(type));
        js_array_push(vm, e, fn);
        js_array_push(vm, e, jv_bool(once));
        js_array_push(vm, *lp, jv_obj(e));
    }
    return jv_undef();
}

/* run the listeners of one target for ev; returns 1 when propagation stopped */
static int fire_on(js_vm_t *vm, jv target, jv ev, uint32_t type) {
    uint32_t *lp = listeners_of(vm, target), i, len;
    uint32_t on;
    jv h;
    char nm[48];
    const js_str_t *t = JSTR(vm, type);
    js_put(vm, ev, js_intern(vm, "currentTarget", 13), target);
    if (vm->ctl) return 1;
    if (lp && *lp) {
        len = JOBJ(vm, *lp)->alen;           /* listeners added while firing wait for the next event */
        for (i = 0; i < len && !vm->ctl; i++) {
            jv e = js_array_get(vm, *lp, i), fn;
            if (e.t != JT_OBJ || js_array_get(vm, e.a, 0).a != type) continue;
            fn = js_array_get(vm, e.a, 1);
            if (js_truthy(vm, js_array_get(vm, e.a, 2))) js_array_set(vm, *lp, i, jv_null());
            if (g_rt.st) g_rt.st->events++;
            if (js_is_callable(vm, fn)) js_call(vm, fn, target, &ev, 1);
            else {
                jv he = js_get_c(vm, fn, "handleEvent");
                if (!vm->ctl && js_is_callable(vm, he)) js_call(vm, he, fn, &ev, 1);
            }
            report_throw(vm);
            if (js_truthy(vm, js_get_c(vm, ev, "__imm"))) break;
        }
    }
    if (vm->ctl) return 1;
    /* the on<type> property */
    if (t->len < 40) {
        int q;
        nm[0] = 'o'; nm[1] = 'n';
        for (q = 0; q < (int)t->len; q++) nm[2 + q] = t->s[q];
        on = js_intern(vm, nm, (int)t->len + 2);
        h = js_get(vm, target, on);
        if (vm->ctl == CTL_THROW) report_throw(vm);
        if (!vm->ctl && js_is_callable(vm, h)) {
            jv r;
            if (g_rt.st) g_rt.st->events++;
            r = js_call(vm, h, target, &ev, 1);
            report_throw(vm);
            if (r.t == JT_BOOL && !r.a) js_put(vm, ev, js_intern(vm, "defaultPrevented", 16), jv_bool(1));
        }
    }
    if (vm->ctl) return 1;
    return js_truthy(vm, js_get_c(vm, ev, "cancelBubble"));
}

static jv dispatch_fn(js_vm_t *vm, jv thisv, jv *argv, int argc, uint32_t magic) {
    jv ev = JARG(0), tv;
    uint32_t type;
    int k, bubbles;
    (void)magic;
    if (!jv_is_obj(ev)) return js_throw_err(vm, JE_TYPE, "parameter 1 is not of type 'Event'");
    if (!listeners_of(vm, thisv)) return js_throw_err(vm, JE_TYPE, "Illegal invocation: not an event target");
    tv = js_get_c(vm, ev, "type");
    JCK(vm);
    type = js_to_key(vm, tv);
    JCK(vm);
    js_put(vm, ev, js_intern(vm, "target", 6), thisv);
    bubbles = js_truthy(vm, js_get_c(vm, ev, "bubbles"));
    JCK(vm);
    if (!fire_on(vm, thisv, ev, type) && bubbles) {
        k = obj_node(vm, thisv);
        while (k >= 0 && !vm->ctl) {
            k = g_n[k].parent;
            if (k < 0) {   /* the document bubbles to window */
                if (obj_node(vm, thisv) >= 0 && is_connected(obj_node(vm, thisv))) fire_on(vm, jv_obj(vm->global), ev, type);
                break;
            }
            if (fire_on(vm, node_obj(vm, k), ev, type)) break;
        }
    }
    JCK(vm);
    js_put(vm, ev, js_intern(vm, "currentTarget", 13), jv_null());
    return jv_bool(!js_truthy(vm, js_get_c(vm, ev, "defaultPrevented")));
}

static uint32_t event_new(js_vm_t *vm, uint32_t type, int bubbles, int cancelable) {
    uint32_t o = js_obj_new(vm, JS_CLASS_OBJECT, g_p_event);
    if (!o) return 0;
    js_define_c(vm, o, "type", jv_str(type), P_DEFAULT);
    js_define_c(vm, o, "bubbles", jv_bool(bubbles), P_DEFAULT);
    js_define_c(vm, o, "cancelable", jv_bool(cancelable), P_DEFAULT);
    js_define_c(vm, o, "defaultPrevented", jv_bool(0), P_DEFAULT);
    js_define_c(vm, o, "cancelBubble", jv_bool(0), P_DEFAULT);
    js_define_c(vm, o, "target", jv_null(), P_DEFAULT);
    js_define_c(vm, o, "currentTarget", jv_null(), P_DEFAULT);
    js_define_c(vm, o, "isTrusted", jv_bool(0), P_DEFAULT);
    js_define_c(vm, o, "timeStamp", jv_int(0), P_DEFAULT);
    return o;
}

static jv event_ctor(js_vm_t *vm, jv thisv, jv *argv, int argc, uint32_t magic) {
    uint32_t type, o;
    int b = 0, c = 0;
    (void)thisv;
    if (!vm->in_new && !vm->super_this) return js_throw_err(vm, JE_TYPE, "Failed to construct 'Event': Please use the 'new' operator");
    if (argc < 1) return js_throw_err(vm, JE_TYPE, "Failed to construct 'Event': 1 argument required, but only 0 present.");
    type = js_to_string(vm, argv[0]);
    JCK(vm);
    if (jv_is_obj(JARG(1))) {
        b = js_truthy(vm, js_get_c(vm, argv[1], "bubbles"));
        c = js_truthy(vm, js_get_c(vm, argv[1], "cancelable"));
        JCK(vm);
    }
    o = event_new(vm, type, b, c);
    if (!o) return jv_undef();
    if (magic) {     /* CustomEvent */
        jv d = jv_is_obj(JARG(1)) ? js_get_c(vm, argv[1], "detail") : jv_undef();
        JCK(vm);
        js_define_c(vm, o, "detail", d.t == JT_UNDEF ? jv_null() : d, P_DEFAULT);
    }
    if (vm->super_this) { JOBJ(vm, o)->proto = JOBJ(vm, vm->super_this)->proto; vm->super_this = 0; }
    return jv_obj(o);
}

static jv event_method(js_vm_t *vm, jv thisv, jv *argv, int argc, uint32_t magic) {
    (void)argv; (void)argc;
    if (!jv_is_obj(thisv)) return jv_undef();
    if (magic == 0) {
        if (js_truthy(vm, js_get_c(vm, thisv, "cancelable")))
            js_put(vm, thisv, js_intern(vm, "defaultPrevented", 16), jv_bool(1));
        return jv_undef();
    }
    js_put(vm, thisv, js_intern(vm, "cancelBubble", 12), jv_bool(1));
    if (magic == 2) js_put(vm, thisv, js_intern(vm, "__imm", 5), jv_bool(1));
    return jv_undef();
}

/* el.click(): a cancelable, bubbling click event */
static jv click_fn(js_vm_t *vm, jv thisv, jv *argv, int argc, uint32_t magic) {
    uint32_t ev;
    jv e;
    (void)argv; (void)argc; (void)magic;
    if (this_node(vm, thisv) < 0) return jv_undef();
    ev = event_new(vm, js_intern(vm, "click", 5), 1, 1);
    if (!ev) return jv_undef();
    e = jv_obj(ev);
    dispatch_fn(vm, thisv, &e, 1, 0);
    return jv_undef();
}

/* ---- timers (virtual clock: they run after load in due order) ---- */

typedef struct { jv fn; uint32_t due, delay, seq; int id, interval, runs, active; } dtimer_t;
static dtimer_t g_t[WF_DOM_TIMERS];
static int g_tid;
static uint32_t g_tseq, g_now;

static jv timer_fn(js_vm_t *vm, jv thisv, jv *argv, int argc, uint32_t magic) {
    int i, slot = -1;
    (void)thisv;
    if (magic == 2) {       /* clearTimeout / clearInterval / cancelAnimationFrame */
        int id = js_to_int32(vm, JARG(0));
        JCK(vm);
        for (i = 0; i < WF_DOM_TIMERS; i++) if (g_t[i].active && g_t[i].id == id) g_t[i].active = 0;
        return jv_undef();
    }
    if (!js_is_callable(vm, JARG(0))) {
        logc(vm, "warn: timers with a string body are not supported (no eval)\n");
        return jv_int(0);
    }
    for (i = 0; i < WF_DOM_TIMERS; i++) if (!g_t[i].active) { slot = i; break; }
    if (slot < 0) {
        logc(vm, "warn: timer bound reached, callback dropped\n");
        if (g_rt.st) g_rt.st->truncated = 1;
        return jv_int(0);
    }
    {
        int d = magic == 3 ? 16 : magic == 4 ? 0 : (argc > 1 ? js_to_int32(vm, argv[1]) : 0);
        JCK(vm);
        if (d < 0) d = 0;
        if (d > 600000) d = 600000;
        g_t[slot].fn = argv[0];
        g_t[slot].delay = (uint32_t)d;
        g_t[slot].due = g_now + (uint32_t)d;
        g_t[slot].seq = g_tseq++;
        g_t[slot].id = ++g_tid;
        g_t[slot].interval = magic == 1;
        g_t[slot].runs = 0;
        g_t[slot].active = 1;
        return jv_int(g_t[slot].id);
    }
}

/* ---- window extras ---- */

static uint32_t g_store[2];

static jv storage_fn(js_vm_t *vm, jv thisv, jv *argv, int argc, uint32_t magic) {
    uint32_t data, key;
    int which = (int)(magic >> 4);
    js_prop_t *p;
    (void)thisv; (void)argc;
    data = g_store[which & 1];
    switch (magic & 15) {
    case 0:   /* getItem */
        key = js_to_key(vm, JARG(0));
        JCK(vm);
        p = js_own_prop(vm, data, key);
        return p ? p->v : jv_null();
    case 1: { /* setItem */
        uint32_t v;
        key = js_to_key(vm, JARG(0));
        JCK(vm);
        v = js_to_string(vm, JARG(1));
        JCK(vm);
        js_put(vm, jv_obj(data), key, jv_str(v));
        return jv_undef();
    }
    case 2:   /* removeItem */
        key = js_to_key(vm, JARG(0));
        JCK(vm);
        js_delete_own(vm, data, key);
        return jv_undef();
    case 3:   /* clear */
        g_store[which & 1] = js_obj_new(vm, JS_CLASS_OBJECT, 0);
        return jv_undef();
    case 4: { /* key(i) */
        uint32_t ks = js_own_keys(vm, data, 1);
        int i = js_to_int32(vm, JARG(0));
        JCK(vm);
        if (!ks || i < 0 || (uint32_t)i >= JOBJ(vm, ks)->alen) return jv_null();
        return js_array_get(vm, ks, (uint32_t)i);
    }
    default: {/* length */
        uint32_t ks = js_own_keys(vm, data, 1);
        return jv_int(ks ? (int32_t)JOBJ(vm, ks)->alen : 0);
    }
    }
}

static jv window_fn(js_vm_t *vm, jv thisv, jv *argv, int argc, uint32_t magic) {
    (void)thisv;
    switch (magic) {
    case 0: case 1: case 2: {   /* alert, confirm, prompt: logged, no dialog */
        uint32_t s = argc ? js_to_string(vm, argv[0]) : vm->s_empty;
        JCK(vm);
        logc(vm, magic == 0 ? "alert: " : magic == 1 ? "confirm: " : "prompt: ");
        js_log_put(vm, JSTR(vm, s)->s, (int)JSTR(vm, s)->len);
        logc(vm, "\n");
        return magic == 0 ? jv_undef() : magic == 1 ? jv_bool(0) : jv_null();
    }
    case 3: {                   /* getComputedStyle: the inline style only */
        int k = obj_node(vm, JARG(0));
        if (k < 0) return js_throw_err(vm, JE_TYPE, "getComputedStyle: parameter 1 is not of type 'Element'");
        return sub_obj(vm, k, HK_STYLE, g_rt.p_style);
    }
    case 4: {                   /* matchMedia: nothing matches */
        uint32_t o = js_obj_new(vm, JS_CLASS_OBJECT, vm->p_object), q;
        if (!o) return jv_undef();
        q = js_to_string(vm, JARG(0));
        JCK(vm);
        js_define_c(vm, o, "matches", jv_bool(0), P_DEFAULT);
        js_define_c(vm, o, "media", jv_str(q), P_DEFAULT);
        js_def_native(vm, o, "addListener", window_fn, 1, 9);
        js_def_native(vm, o, "removeListener", window_fn, 1, 9);
        js_def_native(vm, o, "addEventListener", window_fn, 2, 9);
        js_def_native(vm, o, "removeEventListener", window_fn, 2, 9);
        return jv_obj(o);
    }
    case 5: return jv_int((int32_t)g_now);               /* performance.now */
    case 6: return jv_null();                               /* open */
    case 7: case 8: {           /* btoa, atob */
        static const char tb[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
        uint32_t s = js_to_string(vm, JARG(0));
        const js_str_t *t;
        js_sb_t b;
        uint32_t i, acc = 0;
        int bits = 0;
        JCK(vm);
        t = JSTR(vm, s);
        js_sb_init(vm, &b);
        if (magic == 7) {
            for (i = 0; i < t->len; i += 3) {
                uint32_t v = (uint32_t)(uint8_t)t->s[i] << 16, n = t->len - i;
                if (n > 1) v |= (uint32_t)(uint8_t)t->s[i + 1] << 8;
                if (n > 2) v |= (uint8_t)t->s[i + 2];
                js_sb_ch(vm, &b, tb[(v >> 18) & 63]);
                js_sb_ch(vm, &b, tb[(v >> 12) & 63]);
                js_sb_ch(vm, &b, n > 1 ? tb[(v >> 6) & 63] : '=');
                js_sb_ch(vm, &b, n > 2 ? tb[v & 63] : '=');
            }
        } else {
            for (i = 0; i < t->len; i++) {
                char c = t->s[i];
                int d;
                if (c == '=' || is_space(c)) continue;
                for (d = 0; d < 64 && tb[d] != c; d++) {}
                if (d == 64) return js_throw_err(vm, JE_ERROR, "InvalidCharacterError: The string to be decoded is not correctly encoded.");
                acc = (acc << 6) | (uint32_t)d;
                bits += 6;
                if (bits >= 8) { bits -= 8; js_sb_ch(vm, &b, (char)((acc >> bits) & 255)); }
            }
        }
        return jv_str(js_sb_finish(vm, &b));
    }
    case 10: {                  /* location.toString */
        return js_get_c(vm, jv_obj(g_rt.location), "href");
    }
    case 11:                    /* location.assign / replace / reload */
        logc(vm, "warn: navigation is not supported in the script pass\n");
        return jv_undef();
    default: return jv_undef(); /* scrollTo, focus, matchMedia listeners */
    }
}

static void url_parts(js_vm_t *vm, const char *url) {
    uint32_t o = js_obj_new(vm, JS_CLASS_OBJECT, vm->p_object);
    int n = url ? dlen(url) : 0, i = 0, ps, hs, he, pe, qe, port = -1;
    const char *u = url;
    if (!o) return;
    g_rt.location = o;
    if (!n) { u = "about:blank"; n = 11; }
    js_define_c(vm, o, "href", jv_str(js_str_new(vm, u, n)), P_DEFAULT);
    while (i < n && u[i] != ':' && u[i] != '/') i++;
    if (i < n && u[i] == ':') {
        js_define_c(vm, o, "protocol", jv_str(js_str_new(vm, u, i + 1)), P_DEFAULT);
        ps = i + 1;
    } else {
        js_define_c(vm, o, "protocol", jv_str(vm->s_empty), P_DEFAULT);
        ps = 0;
    }
    hs = ps;
    if (hs + 1 < n && u[hs] == '/' && u[hs + 1] == '/') hs += 2;
    else hs = ps;
    he = hs;
    if (hs != ps) while (he < n && u[he] != '/' && u[he] != '?' && u[he] != '#') he++;
    js_define_c(vm, o, "host", jv_str(js_str_new(vm, u + hs, he - hs)), P_DEFAULT);
    for (i = hs; i < he && u[i] != ':'; i++) {}
    js_define_c(vm, o, "hostname", jv_str(js_str_new(vm, u + hs, i - hs)), P_DEFAULT);
    js_define_c(vm, o, "port", jv_str(js_str_new(vm, u + (i < he ? i + 1 : he), i < he ? he - i - 1 : 0)), P_DEFAULT);
    (void)port;
    pe = he;
    while (pe < n && u[pe] != '?' && u[pe] != '#') pe++;
    js_define_c(vm, o, "pathname", jv_str(pe > he ? js_str_new(vm, u + he, pe - he) : js_str_c(vm, hs != ps ? "/" : "")), P_DEFAULT);
    qe = pe;
    if (qe < n && u[qe] == '?') while (qe < n && u[qe] != '#') qe++;
    js_define_c(vm, o, "search", jv_str(js_str_new(vm, u + pe, qe - pe)), P_DEFAULT);
    js_define_c(vm, o, "hash", jv_str(js_str_new(vm, u + qe, n - qe > 1 ? n - qe : 0)), P_DEFAULT);
    js_define_c(vm, o, "origin", jv_str(hs != ps ? js_str_new(vm, u, he) : js_str_c(vm, "null")), P_DEFAULT);
    js_def_native(vm, o, "toString", window_fn, 0, 10);
    js_def_native(vm, o, "assign", window_fn, 1, 11);
    js_def_native(vm, o, "replace", window_fn, 1, 11);
    js_def_native(vm, o, "reload", window_fn, 0, 11);
}

/* ---- realm setup ---- */

static uint32_t g_dispatch;
static js_vm_t g_vm;

static uint32_t plain_obj(js_vm_t *vm) { return js_obj_new(vm, JS_CLASS_OBJECT, vm->p_object); }

static int realm_setup(js_vm_t *vm, int doc, const char *url) {
    uint32_t g = vm->global, o, ctor;
    int i;
    for (i = 0; i < K_COUNT; i++) g_rt.key[i] = js_intern(vm, k_names[i], dlen(k_names[i]));
    for (i = 0; k_reflect[i]; i++) g_rt.reflect[i] = js_intern(vm, k_reflect[i], dlen(k_reflect[i]));
    g_rt.vm = vm;
    g_rt.doc = doc;
    g_rt.cookie = 0;
    g_rt.win_listeners = 0;
    g_rt.writing = 0;
    vm->host_get = dom_get;
    vm->host_set = dom_set;
    vm->host_ctx = &g_rt;

    g_rt.p_node = plain_obj(vm);
    g_rt.p_style = plain_obj(vm);
    g_rt.p_classlist = plain_obj(vm);
    g_p_event = plain_obj(vm);
    if (!g_rt.p_node || !g_rt.p_style || !g_rt.p_classlist || !g_p_event) return -1;
    for (i = 0; i < M_COUNT; i++) js_def_native(vm, g_rt.p_node, m_names[i], node_method, m_nargs[i], (uint32_t)i);
    js_def_native(vm, g_rt.p_node, "addEventListener", listener_fn, 2, 0);
    js_def_native(vm, g_rt.p_node, "removeEventListener", listener_fn, 2, 1);
    g_dispatch = js_def_native(vm, g_rt.p_node, "dispatchEvent", dispatch_fn, 1, 0);
    js_def_native(vm, g_rt.p_node, "click", click_fn, 0, 0);
    js_def_native(vm, g_rt.p_style, "setProperty", style_method, 2, 0);
    js_def_native(vm, g_rt.p_style, "getPropertyValue", style_method, 1, 1);
    js_def_native(vm, g_rt.p_style, "removeProperty", style_method, 1, 2);
    {
        static const char *const cl[] = { "add", "remove", "toggle", "contains", "replace", "item", "toString", 0 };
        for (i = 0; cl[i]; i++) js_def_native(vm, g_rt.p_classlist, cl[i], class_list_fn, 1, (uint32_t)i);
    }
    js_def_native(vm, g_p_event, "preventDefault", event_method, 0, 0);
    js_def_native(vm, g_p_event, "stopPropagation", event_method, 0, 1);
    js_def_native(vm, g_p_event, "stopImmediatePropagation", event_method, 0, 2);
    ctor = js_ctor_new(vm, "Event", event_ctor, 1, g_p_event);
    o = plain_obj(vm);
    if (!ctor || !o) return -1;
    JOBJ(vm, o)->proto = g_p_event;
    {   /* the CustomEvent native carries magic 1 */
        uint32_t c2 = js_native_new(vm, "CustomEvent", event_ctor, 1, 1);
        if (c2) {
            js_define(vm, c2, vm->s_prototype, jv_obj(o), 0);
            js_define(vm, o, vm->s_constructor, jv_obj(c2), P_HIDDEN);
            js_define_c(vm, g, "CustomEvent", jv_obj(c2), P_HIDDEN);
        }
    }

    url_parts(vm, url);
    js_define_c(vm, g, "document", node_obj(vm, doc), P_HIDDEN);
    js_define_c(vm, g, "self", jv_obj(g), P_HIDDEN);
    js_define_c(vm, g, "location", jv_obj(g_rt.location), P_HIDDEN);
    js_def_native(vm, g, "addEventListener", listener_fn, 2, 0);
    js_def_native(vm, g, "removeEventListener", listener_fn, 2, 1);
    js_def_native(vm, g, "dispatchEvent", dispatch_fn, 1, 0);
    js_def_native(vm, g, "setTimeout", timer_fn, 2, 0);
    js_def_native(vm, g, "setInterval", timer_fn, 2, 1);
    js_def_native(vm, g, "clearTimeout", timer_fn, 1, 2);
    js_def_native(vm, g, "clearInterval", timer_fn, 1, 2);
    js_def_native(vm, g, "requestAnimationFrame", timer_fn, 1, 3);
    js_def_native(vm, g, "cancelAnimationFrame", timer_fn, 1, 2);
    js_def_native(vm, g, "queueMicrotask", timer_fn, 1, 4);
    js_def_native(vm, g, "alert", window_fn, 1, 0);
    js_def_native(vm, g, "confirm", window_fn, 1, 1);
    js_def_native(vm, g, "prompt", window_fn, 2, 2);
    js_def_native(vm, g, "getComputedStyle", window_fn, 1, 3);
    js_def_native(vm, g, "matchMedia", window_fn, 1, 4);
    js_def_native(vm, g, "open", window_fn, 1, 6);
    js_def_native(vm, g, "btoa", window_fn, 1, 7);
    js_def_native(vm, g, "atob", window_fn, 1, 8);
    js_def_native(vm, g, "scrollTo", window_fn, 2, 9);
    js_def_native(vm, g, "scroll", window_fn, 2, 9);
    js_def_native(vm, g, "focus", window_fn, 0, 9);
    js_define_c(vm, g, "innerWidth", jv_int(1024), P_DEFAULT);
    js_define_c(vm, g, "innerHeight", jv_int(768), P_DEFAULT);
    js_define_c(vm, g, "devicePixelRatio", jv_int(1), P_DEFAULT);
    js_define_c(vm, g, "scrollX", jv_int(0), P_DEFAULT);
    js_define_c(vm, g, "scrollY", jv_int(0), P_DEFAULT);
    js_define_c(vm, g, "pageYOffset", jv_int(0), P_DEFAULT);
    js_define_c(vm, g, "name", jv_str(vm->s_empty), P_DEFAULT);

    o = plain_obj(vm);
    if (!o) return -1;
    js_define_c(vm, o, "userAgent", jv_str(js_str_c(vm, "Mozilla/5.0 (Mohhdy; i386) MohhdyWeb/113")), P_DEFAULT);
    js_define_c(vm, o, "appName", jv_str(js_str_c(vm, "Netscape")), P_DEFAULT);
    js_define_c(vm, o, "platform", jv_str(js_str_c(vm, "Mohhdy")), P_DEFAULT);
    js_define_c(vm, o, "language", jv_str(js_str_c(vm, "en-US")), P_DEFAULT);
    js_define_c(vm, o, "cookieEnabled", jv_bool(1), P_DEFAULT);
    js_define_c(vm, o, "onLine", jv_bool(1), P_DEFAULT);
    js_define_c(vm, o, "hardwareConcurrency", jv_int(1), P_DEFAULT);
    js_define_c(vm, g, "navigator", jv_obj(o), P_HIDDEN);
    o = plain_obj(vm);
    if (!o) return -1;
    js_define_c(vm, o, "width", jv_int(1024), P_DEFAULT);
    js_define_c(vm, o, "height", jv_int(768), P_DEFAULT);
    js_define_c(vm, o, "colorDepth", jv_int(32), P_DEFAULT);
    js_define_c(vm, g, "screen", jv_obj(o), P_HIDDEN);
    o = plain_obj(vm);
    if (!o) return -1;
    js_def_native(vm, o, "now", window_fn, 0, 5);
    js_define_c(vm, g, "performance", jv_obj(o), P_HIDDEN);
    o = plain_obj(vm);
    if (!o) return -1;
    js_define_c(vm, o, "ELEMENT_NODE", jv_int(1), P_DEFAULT);
    js_define_c(vm, o, "TEXT_NODE", jv_int(3), P_DEFAULT);
    js_define_c(vm, o, "COMMENT_NODE", jv_int(8), P_DEFAULT);
    js_define_c(vm, o, "DOCUMENT_NODE", jv_int(9), P_DEFAULT);
    js_define_c(vm, o, "DOCUMENT_FRAGMENT_NODE", jv_int(11), P_DEFAULT);
    js_define_c(vm, g, "Node", jv_obj(o), P_HIDDEN);
    for (i = 0; i < 2; i++) {
        static const char *const sn[] = { "getItem", "setItem", "removeItem", "clear", "key", 0 };
        uint32_t st = plain_obj(vm), getter;
        int q;
        g_store[i] = js_obj_new(vm, JS_CLASS_OBJECT, 0);
        if (!st || !g_store[i]) return -1;
        for (q = 0; sn[q]; q++) js_def_native(vm, st, sn[q], storage_fn, q == 1 ? 2 : 1, (uint32_t)((i << 4) | q));
        getter = js_native_new(vm, "length", storage_fn, 0, (uint32_t)((i << 4) | 5));
        if (getter) js_define_accessor(vm, st, vm->s_length, getter, 0, P_CONF);
        js_define_c(vm, g, i ? "sessionStorage" : "localStorage", jv_obj(st), P_HIDDEN);
    }
    for (i = 0; i < WF_DOM_TIMERS; i++) g_t[i].active = 0;
    g_tid = 0;
    g_tseq = 0;
    g_now = 0;
    return vm->ctl == CTL_ABORT ? -1 : 0;
}

/* ---- the page runner ---- */

static int g_calls;

static void copy_err(wf_jspage_stats_t *st, const char *e) {
    int i;
    if (st->first_err[0]) return;
    for (i = 0; e[i] && i < 159; i++) st->first_err[i] = e[i];
    st->first_err[i] = 0;
}

/* after one entry point: count it, log an uncaught error; 1 when the pass must stop */
static int after_run(js_vm_t *vm, int rc, wf_jspage_stats_t *st, int is_script) {
    st->steps += js_steps_used(vm);
    if (rc == JS_OK) { if (is_script) st->ran++; return 0; }
    st->failed++;
    copy_err(st, vm->err);
    logc(vm, "error: ");
    logc(vm, vm->err);
    logc(vm, "\n");
    /* an exhausted arena cannot run anything else */
    return rc == JS_ABORTED && js_mem_used(vm) + 65536u >= js_mem_size(vm);
}

static int is_js_type(int k) {
    const char *v;
    int vl, i;
    static const char *const ok[] = { "text/javascript", "application/javascript", "text/ecmascript",
                                      "application/ecmascript", "application/x-javascript", "text/jscript", 0 };
    if (!attr_val(k, "type", &v, &vl)) return 1;
    while (vl > 0 && is_space(v[vl - 1])) vl--;
    if (!vl) return 1;
    for (i = 0; ok[i]; i++) if (span_eq_ci(v, vl, ok[i], dlen(ok[i]))) return 1;
    return 0;
}

static int next_script(int doc) {
    int k;
    for (k = walk_next(doc, doc); k >= 0; k = walk_next(k, doc))
        if (g_n[k].inserted == 0 && name_is(k, "script")) return k;
    return -1;
}

static int has_script_tag(const char *s, int n) {
    int i;
    for (i = 0; i + 7 <= n; i++)
        if (s[i] == '<' && span_eq_ci(s + i + 1, 6, "script", 6)) return 1;
    return 0;
}

/* insert the HTML written by a script right after it */
static void flush_writes(js_vm_t *vm, int script) {
    uint32_t w = js_sb_finish(vm, &g_rt.write_buf);
    int frag, c, nx, parent, ref;
    g_rt.writing = 0;
    if (vm->ctl || !w || !JSTR(vm, w)->len) return;
    frag = node_new(DN_FRAG);
    if (frag < 0) return;
    parse_into(frag, JSTR(vm, w)->s, (int)JSTR(vm, w)->len);
    parent = g_n[script].parent;
    ref = g_n[script].next;
    if (parent < 0) {
        int html = find_child(g_rt.doc, "html");
        parent = html < 0 ? g_rt.doc : find_child(html, "body");
        if (parent < 0) parent = g_rt.doc;
        ref = -1;
    }
    for (c = g_n[frag].first; c >= 0; c = nx) { nx = g_n[c].next; insert_before(parent, c, ref); }
}

static int fire_event(js_vm_t *vm, jv target, const char *type, int bubbles, wf_jspage_stats_t *st) {
    uint32_t ev = event_new(vm, js_intern(vm, type, dlen(type)), bubbles, 0);
    jv e;
    int rc;
    if (!ev || vm->ctl) return 1;
    e = jv_obj(ev);
    if (++g_calls > WF_DOM_PAGE_CALLS) { st->truncated = 1; return 1; }
    js_set_budget(vm, WF_DOM_SCRIPT_BUDGET);
    rc = js_call_value(vm, jv_obj(g_dispatch), target, &e, 1, 0);
    return after_run(vm, rc, st, 0);
}

static void set_ready(const char *s) {
    int i;
    for (i = 0; s[i] && i < 15; i++) g_rt.ready[i] = s[i];
    g_rt.ready[i] = 0;
}

static int run_scripts(js_vm_t *vm, int doc, wf_jspage_stats_t *st) {
    int k;
    while ((k = next_script(doc)) >= 0) {
        uint32_t src;
        int rc;
        g_n[k].inserted = 2;
        if (bool_attr(k, "src")) { st->external++; continue; }
        if (!is_js_type(k)) { st->other_type++; continue; }
        st->scripts++;
        if (++g_calls > WF_DOM_PAGE_CALLS) { st->truncated = 1; return 1; }
        src = text_content(vm, k);
        if (vm->ctl) return 1;
        g_rt.writing = 1;
        js_sb_init(vm, &g_rt.write_buf);
        js_set_budget(vm, WF_DOM_SCRIPT_BUDGET);
        rc = js_eval(vm, JSTR(vm, src)->s, (int)JSTR(vm, src)->len, 0);
        flush_writes(vm, k);
        if (after_run(vm, rc, st, 1)) return 1;
    }
    return 0;
}

static int run_load(js_vm_t *vm, int doc, wf_jspage_stats_t *st) {
    int html = find_child(doc, "html"), body = html < 0 ? -1 : find_child(html, "body");
    set_ready("interactive");
    if (fire_event(vm, node_obj(vm, doc), "DOMContentLoaded", 1, st)) return 1;
    set_ready("complete");
    /* <body onload="..."> is window.onload unless a script set it */
    if (body >= 0 && bool_attr(body, "onload")) {
        jv cur = js_get_c(vm, jv_obj(vm->global), "onload");
        if (!js_is_callable(vm, cur)) {
            int f, rc;
            uint32_t code = attr_get_js(vm, body, "onload", &f);
            js_sb_t b;
            js_sb_init(vm, &b);
            js_sb_put(vm, &b, "window.onload=function(event){", 30);
            js_sb_str(vm, &b, code);
            js_sb_put(vm, &b, "\n};", 3);
            code = js_sb_finish(vm, &b);
            if (vm->ctl) return 1;
            js_set_budget(vm, WF_DOM_SCRIPT_BUDGET);
            rc = js_eval(vm, JSTR(vm, code)->s, (int)JSTR(vm, code)->len, 0);
            if (after_run(vm, rc, st, 0)) return 1;
        }
    }
    return fire_event(vm, jv_obj(vm->global), "load", 0, st);
}

static int run_timers(js_vm_t *vm, wf_jspage_stats_t *st) {
    for (;;) {
        int i, best = -1, rc;
        jv fn;
        for (i = 0; i < WF_DOM_TIMERS; i++) {
            if (!g_t[i].active) continue;
            if (best < 0 || g_t[i].due < g_t[best].due || (g_t[i].due == g_t[best].due && g_t[i].seq < g_t[best].seq)) best = i;
        }
        if (best < 0) return 0;
        if (++g_calls > WF_DOM_PAGE_CALLS) { st->truncated = 1; return 1; }
        fn = g_t[best].fn;
        g_now = g_t[best].due;
        g_t[best].runs++;
        if (g_t[best].interval && g_t[best].runs < WF_DOM_INTERVAL_RUNS) {
            g_t[best].due += g_t[best].delay ? g_t[best].delay : 1;
            g_t[best].seq = g_tseq++;
        } else {
            g_t[best].active = 0;
        }
        st->timers++;
        js_set_budget(vm, WF_DOM_SCRIPT_BUDGET);
        rc = js_call_value(vm, fn, jv_obj(vm->global), 0, 0, 0);
        if (after_run(vm, rc, st, 0)) return 1;
    }
}

int wf_jspage_run(void *arena, uint32_t arena_size, const char *html, int n, const char *url, char *out, int out_max,
                  wf_jspage_stats_t *st) {
    static wf_jspage_stats_t dummy;
    js_vm_t *vm = &g_vm;
    dout_t o;
    int doc, i;
    if (!st) st = &dummy;
    for (i = 0; i < (int)sizeof(*st); i++) ((char *)st)[i] = 0;
    g_vm.log_len = 0;
    g_vm.log[0] = 0;
    if (!html || n <= 0 || !out || out_max <= 0) return -1;
    if (!has_script_tag(html, n)) return 0;
    if (js_init(vm, arena, arena_size) != 0) return -1;
    doc = dom_parse(html, n);
    if (doc < 0) return -1;
    g_rt.st = st;
    g_calls = 0;
    if (realm_setup(vm, doc, url) != 0) { g_rt.st = 0; return -1; }
    set_ready("loading");
    if (!run_scripts(vm, doc, st) && !run_load(vm, doc, st)) run_timers(vm, st);
    if (!st->scripts) { g_rt.st = 0; return 0; }
    o.out = out;
    o.n = 0;
    o.max = out_max;
    o.vm = 0;
    o.sb = 0;
    o.skip_scripts = 1;
    ser(&o, doc, 0);
    if (o.n >= out_max) { o.n = out_max - 1; st->truncated = 1; }
    out[o.n] = 0;
    for (i = 0, st->nodes = 0; i < g_nn; i++) if (is_connected(i)) st->nodes++;
    if (g_trunc) st->truncated = 1;
    st->mem = js_mem_used(vm);
    g_rt.st = 0;
    vm->host_get = 0;
    vm->host_set = 0;
    return o.n;
}

const char *wf_jspage_log(int *len) {
    if (len) *len = g_vm.log_len;
    return g_vm.log;
}
