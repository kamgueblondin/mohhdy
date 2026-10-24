/* userspace/webeng.c - Lot 6 (US-031, US-043, US-045). See webeng.h. */
#include "webeng.h"

/* ---------- small helpers (no libc) ---------- */
static int w_len(const char* s) { int n = 0; while (s && s[n]) n++; return n; }
static int w_eq(const char* a, const char* b) {
    int i = 0;
    while (a[i] && a[i] == b[i]) i++;
    return a[i] == b[i];
}
static char w_low(char c) { return (c >= 'A' && c <= 'Z') ? (char)(c + 32) : c; }
static int w_space(char c) { return c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == '\f'; }
static int w_namec(char c) {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '-' || c == '_';
}
static void w_cpy(char* d, int max, const char* s, int n) {
    int i = 0;
    if (max <= 0) return;
    while (i < n && i < max - 1 && s[i]) { d[i] = s[i]; i++; }
    d[i] = 0;
}
static void w_zero(void* p, uint32_t n) { uint8_t* b = (uint8_t*)p; while (n--) *b++ = 0; }
/* case-insensitive prefix match of word at s (length n available) */
static int w_ieq_n(const char* s, int n, const char* word) {
    int i = 0;
    while (word[i]) {
        if (i >= n || w_low(s[i]) != word[i]) return 0;
        i++;
    }
    return 1;
}
static void w_itoa(int v, char* out) {
    char t[12];
    int k = 0, j = 0;
    unsigned u = (unsigned)(v < 0 ? -v : v);
    if (v < 0) out[j++] = '-';
    do { t[k++] = (char)('0' + u % 10U); u /= 10U; } while (u && k < 11);
    while (k) out[j++] = t[--k];
    out[j] = 0;
}

static void diag(we_doc_t* d, const char* msg, int pos) {
    char num[12];
    int n;
    if (d->ndiag >= WE_MAX_DIAG) { d->diag_dropped++; return; }
    w_cpy(d->diag[d->ndiag], WE_DIAG_LEN - 12, msg, w_len(msg));
    n = w_len(d->diag[d->ndiag]);
    if (pos >= 0) {
        d->diag[d->ndiag][n++] = ' ';
        d->diag[d->ndiag][n++] = '@';
        d->diag[d->ndiag][n] = 0;
        w_itoa(pos, num);
        w_cpy(d->diag[d->ndiag] + n, WE_DIAG_LEN - n, num, w_len(num));
    }
    d->ndiag++;
}

static int pool_put(we_doc_t* d, const char* s, int n, uint16_t* off) {
    int i;
    if (n < 0 || d->pool_used + (uint32_t)n > WE_POOL) return -1;
    *off = (uint16_t)d->pool_used;
    for (i = 0; i < n; i++) d->pool[d->pool_used++] = s[i];
    return 0;
}

/* ---------- parse ---------- */
static const char* const k_void[] = { "br", "hr", "img", "input", "meta", "link", 0 };
static int is_void(const char* t) { int i; for (i = 0; k_void[i]; i++) if (w_eq(t, k_void[i])) return 1; return 0; }

static int new_node(we_doc_t* d, int parent) {
    we_node_t* n;
    int idx;
    if (d->nnodes >= WE_MAX_NODES) return -1;
    idx = (int)d->nnodes++;
    n = &d->nodes[idx];
    w_zero(n, sizeof(*n));
    n->parent = (int16_t)parent;
    n->first = n->last = n->next = -1;
    if (parent >= 0) {
        we_node_t* p = &d->nodes[parent];
        if (p->last >= 0) d->nodes[p->last].next = (int16_t)idx;
        else p->first = (int16_t)idx;
        p->last = (int16_t)idx;
    }
    return idx;
}

/* decode entities and collapse spaces of html[a..b) into the pool */
static int add_text(we_doc_t* d, int parent, const char* s, int n) {
    char buf[512];
    int i = 0, o = 0, space = 0, lead = 0, idx;
    while (i < n && o < (int)sizeof(buf) - 1) {
        char c = s[i];
        if (w_space(c)) { if (o == 0) lead = 1; space = 1; i++; continue; }
        if (space && o > 0) buf[o++] = ' ';
        space = 0;
        if (c == '&') {
            if (w_ieq_n(s + i, n - i, "&amp;")) { c = '&'; i += 5; }
            else if (w_ieq_n(s + i, n - i, "&lt;")) { c = '<'; i += 4; }
            else if (w_ieq_n(s + i, n - i, "&gt;")) { c = '>'; i += 4; }
            else if (w_ieq_n(s + i, n - i, "&quot;")) { c = '"'; i += 6; }
            else if (w_ieq_n(s + i, n - i, "&nbsp;")) { c = ' '; i += 6; }
            else i++;
            buf[o++] = c;
            continue;
        }
        buf[o++] = (c >= 32 && c < 127) ? c : '?';
        i++;
    }
    if (o == 0) return 0; /* whitespace only */
    idx = new_node(d, parent);
    if (idx < 0) return -1;
    d->nodes[idx].is_text = 1;
    d->nodes[idx].lead_space = (uint8_t)lead;
    d->nodes[idx].trail_space = (uint8_t)space;
    if (pool_put(d, buf, o, &d->nodes[idx].text_off) != 0) return -1;
    d->nodes[idx].text_len = (uint16_t)o;
    d->perf.text_nodes++;
    return 0;
}

static int find_ci(const char* s, int n, int from, const char* word) {
    int i;
    for (i = from; i < n; i++) if (w_ieq_n(s + i, n - i, word)) return i;
    return -1;
}

int we_parse(we_doc_t* d, const char* html, int len) {
    int stack[WE_DEPTH_MAX];
    int sp = 0, i = 0, text_start = 0, in_title = 0;
    if (!d || !html || len < 0) return -1;
    w_zero(d, sizeof(*d));
    d->root = (int16_t)new_node(d, -1);
    w_cpy(d->nodes[0].tag, 12, "#document", 9);
    stack[sp++] = 0;
    while (i <= len) {
        if (i == len || html[i] == '<') {
            if (i > text_start) {
                if (in_title) w_cpy(d->title, (int)sizeof(d->title), html + text_start, i - text_start);
                else if (add_text(d, stack[sp - 1], html + text_start, i - text_start) != 0) { diag(d, "too many nodes", i); return -2; }
            }
            if (i == len) break;
            if (w_ieq_n(html + i, len - i, "<!--")) {
                int e = find_ci(html, len, i + 4, "-->");
                i = e < 0 ? len : e + 3;
                text_start = i;
                continue;
            }
            if (i + 1 < len && html[i + 1] == '!') {
                while (i < len && html[i] != '>') i++;
                i++; text_start = i; continue;
            }
            if (i + 1 < len && html[i + 1] == '/') {
                char name[12];
                int k = 0, j = i + 2, s;
                while (j < len && w_namec(html[j]) && k < 11) name[k++] = w_low(html[j++]);
                name[k] = 0;
                while (j < len && html[j] != '>') j++;
                i = j + 1; text_start = i;
                if (w_eq(name, "title")) { in_title = 0; continue; }
                for (s = sp - 1; s > 0; s--) if (w_eq(d->nodes[stack[s]].tag, name)) break;
                if (s > 0) sp = s;
                else diag(d, "unmatched close tag", j);
                continue;
            }
            {
                char name[12];
                int k = 0, j = i + 1, idx, self = 0;
                we_node_t* n;
                while (j < len && w_namec(html[j]) && k < 11) name[k++] = w_low(html[j++]);
                name[k] = 0;
                if (k == 0) { i++; continue; } /* stray '<' becomes text */
                /* implied end tags */
                if ((w_eq(name, "p") || w_eq(name, "li")) && sp > 1 && w_eq(d->nodes[stack[sp - 1]].tag, name)) sp--;
                if (w_eq(name, "title")) {
                    while (j < len && html[j] != '>') j++;
                    in_title = 1; i = j + 1; text_start = i; continue;
                }
                idx = new_node(d, stack[sp - 1]);
                if (idx < 0) { diag(d, "too many nodes", i); return -3; }
                n = &d->nodes[idx];
                w_cpy(n->tag, 12, name, k);
                d->perf.elements++;
                /* attributes */
                while (j < len && html[j] != '>') {
                    char an[12], av[64];
                    int ak = 0, vk = 0;
                    if (w_space(html[j])) { j++; continue; }
                    if (html[j] == '/') { self = 1; j++; continue; }
                    while (j < len && w_namec(html[j]) && ak < 11) an[ak++] = w_low(html[j++]);
                    an[ak] = 0;
                    if (ak == 0) { j++; continue; }
                    while (j < len && w_space(html[j])) j++;
                    if (j < len && html[j] == '=') {
                        char q = 0;
                        j++;
                        while (j < len && w_space(html[j])) j++;
                        if (j < len && (html[j] == '"' || html[j] == '\'')) q = html[j++];
                        while (j < len && (q ? html[j] != q : (!w_space(html[j]) && html[j] != '>'))) {
                            if (vk < 63) av[vk++] = html[j];
                            j++;
                        }
                        if (q && j < len) j++;
                    }
                    av[vk] = 0;
                    if (w_eq(an, "id")) w_cpy(n->id, 16, av, vk);
                    else if (w_eq(an, "class")) w_cpy(n->cls, 32, av, vk);
                    else if (w_eq(an, "style")) { if (pool_put(d, av, vk, &n->style_off) == 0) n->style_len = (uint16_t)vk; }
                    else if (w_eq(an, "alt")) { if (pool_put(d, av, vk, &n->alt_off) == 0) n->alt_len = (uint16_t)vk; }
                    else if (w_eq(an, "href")) { if (pool_put(d, av, vk, &n->href_off) == 0) n->href_len = (uint16_t)vk; }
                }
                i = j + 1; text_start = i;
                if (w_eq(name, "img")) {
                    char alt[40];
                    int m, q = 0;
                    alt[q++] = '[';
                    for (m = 0; m < n->alt_len && q < 37; m++) { char ch = d->pool[n->alt_off + m]; alt[q++] = ch == ' ' ? '_' : ch; }
                    if (q == 1) { alt[q++] = 'i'; alt[q++] = 'm'; alt[q++] = 'g'; }
                    alt[q++] = ']';
                    if (pool_put(d, alt, q, &n->text_off) == 0) n->text_len = (uint16_t)q;
                }
                if (w_eq(name, "style") || w_eq(name, "script")) {
                    int e = find_ci(html, len, i, w_eq(name, "style") ? "</style" : "</script");
                    int end = e < 0 ? len : e;
                    if (w_eq(name, "style")) {
                        int c;
                        for (c = i; c < end && d->css_len < WE_CSS_MAX - 1; c++) d->css[d->css_len++] = html[c];
                        d->css[d->css_len++] = ' ';
                    } else {
                        d->perf.scripts_skipped++;
                        diag(d, "script ignored (no JavaScript)", i);
                    }
                    if (e < 0) { i = len; } else { while (e < len && html[e] != '>') e++; i = e + 1; }
                    text_start = i;
                    continue;
                }
                if (!self && !is_void(name)) {
                    if (sp >= WE_DEPTH_MAX) { diag(d, "nesting too deep", i); sp = WE_DEPTH_MAX - 1; }
                    stack[sp++] = idx;
                }
            }
            continue;
        }
        i++;
    }
    d->perf.nodes = d->nnodes;
    return 0;
}

/* ---------- CSS ---------- */
static int parse_sel(const char* s, int n, we_rule_t* r) {
    int i = 0, c = 0;
    w_zero(r->comp, sizeof(r->comp));
    r->ncomp = 0; r->spec = 0;
    while (i < n) {
        while (i < n && w_space(s[i])) i++;
        if (i >= n) break;
        if (c >= 2) return -1;
        if (s[i] == '*') i++;
        while (i < n && !w_space(s[i])) {
            int slot = 0, k = 0;
            char* dst;
            if (s[i] == '.') { slot = 1; i++; r->spec += 10; }
            else if (s[i] == '#') { slot = 2; i++; r->spec += 100; }
            else if (w_namec(s[i])) { slot = 0; r->spec += 1; }
            else return -1;
            dst = r->comp[c][slot];
            if (dst[0]) return -1; /* two classes in one compound: unsupported */
            while (i < n && w_namec(s[i]) && k < 31) dst[k++] = w_low(s[i++]);
            dst[k] = 0;
            if (k == 0) return -1;
        }
        c++;
    }
    r->ncomp = (uint8_t)c;
    return c > 0 ? 0 : -1;
}

static void parse_css(we_doc_t* d) {
    const char* s = d->css;
    int n = (int)d->css_len, i = 0;
    while (i < n) {
        int sel_a, sel_b, dec_a, dec_b, a;
        while (i < n && w_space(s[i])) i++;
        if (i >= n) break;
        if (s[i] == '@') { /* skip at-rules and their block */
            int depth = 0;
            while (i < n) { if (s[i] == '{') depth++; if (s[i] == '}' && --depth <= 0) { i++; break; } if (s[i] == ';' && depth == 0) { i++; break; } i++; }
            diag(d, "css at-rule skipped", i);
            continue;
        }
        sel_a = i;
        while (i < n && s[i] != '{') i++;
        sel_b = i;
        if (i >= n) break;
        dec_a = ++i;
        while (i < n && s[i] != '}') i++;
        dec_b = i;
        i++;
        a = sel_a;
        while (a < sel_b) {
            int b = a;
            we_rule_t* r;
            uint16_t off;
            while (b < sel_b && s[b] != ',') b++;
            if (d->nrules >= WE_MAX_RULES) { diag(d, "too many css rules", a); return; }
            r = &d->rules[d->nrules];
            if (parse_sel(s + a, b - a, r) == 0 && pool_put(d, s + dec_a, dec_b - dec_a, &off) == 0) {
                r->decl_off = off;
                r->decl_len = (uint16_t)(dec_b - dec_a);
                r->order = (uint16_t)d->nrules;
                d->nrules++;
            } else {
                diag(d, "css selector unsupported", a);
            }
            a = b + 1;
        }
    }
    d->perf.rules = d->nrules;
}

static int has_class(const char* list, const char* c) {
    int i = 0, n = w_len(c);
    while (list[i]) {
        int j = 0;
        while (list[i] == ' ') i++;
        while (list[i + j] && list[i + j] != ' ') j++;
        if (j == n && j > 0) {
            int k = 0;
            while (k < n && w_low(list[i + k]) == c[k]) k++;
            if (k == n) return 1;
        }
        i += j;
    }
    return 0;
}

static int match_comp(const we_node_t* n, char comp[3][32]) {
    char id[16];
    int i;
    if (n->is_text) return 0;
    if (comp[0][0] && !w_eq(n->tag, comp[0])) return 0;
    if (comp[1][0] && !has_class(n->cls, comp[1])) return 0;
    if (comp[2][0]) {
        for (i = 0; n->id[i] && i < 15; i++) id[i] = w_low(n->id[i]);
        id[i] = 0;
        if (!w_eq(id, comp[2])) return 0;
    }
    return 1;
}

static int match_rule(const we_doc_t* d, int idx, we_rule_t* r) {
    int p;
    if (!match_comp(&d->nodes[idx], r->comp[r->ncomp - 1])) return 0;
    if (r->ncomp == 1) return 1;
    for (p = d->nodes[idx].parent; p > 0; p = d->nodes[p].parent)
        if (match_comp(&d->nodes[p], r->comp[0])) return 1;
    return 0;
}

static int parse_color(const char* v, int n, uint32_t* out) {
    static const struct { const char* name; uint32_t rgb; } named[] = {
        {"black", 0x000000}, {"white", 0xFFFFFF}, {"red", 0xFF0000}, {"green", 0x008000},
        {"blue", 0x0000FF}, {"yellow", 0xFFFF00}, {"gray", 0x808080}, {"grey", 0x808080},
        {"navy", 0x000080}, {"teal", 0x008080}, {"orange", 0xFFA500}, {"purple", 0x800080},
        {"silver", 0xC0C0C0}, {"maroon", 0x800000}, {"lime", 0x00FF00}, {0, 0}};
    int i;
    if (n > 0 && v[0] == '#') {
        uint32_t x = 0;
        int k;
        for (k = 1; k < n; k++) {
            char c = w_low(v[k]);
            int h = (c >= '0' && c <= '9') ? c - '0' : (c >= 'a' && c <= 'f') ? c - 'a' + 10 : -1;
            if (h < 0) return -1;
            x = (x << 4) | (uint32_t)h;
        }
        if (n == 4) x = ((x & 0xF00) << 12) | ((x & 0xF00) << 8) | ((x & 0x0F0) << 8) | ((x & 0x0F0) << 4) | ((x & 0x00F) << 4) | (x & 0x00F);
        else if (n != 7) return -1;
        *out = x;
        return 0;
    }
    for (i = 0; named[i].name; i++)
        if (n == w_len(named[i].name) && w_ieq_n(v, n, named[i].name)) { *out = named[i].rgb; return 0; }
    return -1;
}

/* "12px", "12", "1.5em" (em = 10 px), "50%" (pct set) */
static int parse_len(const char* v, int n, int* px, int* pct) {
    int i = 0, val = 0, frac = 0, fd = 0, neg = 0;
    if (pct) *pct = 0;
    if (i < n && v[i] == '-') { neg = 1; i++; }
    if (i >= n || !((v[i] >= '0' && v[i] <= '9') || v[i] == '.')) return w_ieq_n(v, n, "auto") ? 1 : -1;
    while (i < n && v[i] >= '0' && v[i] <= '9') val = val * 10 + (v[i++] - '0');
    if (i < n && v[i] == '.') { i++; while (i < n && v[i] >= '0' && v[i] <= '9') { if (fd < 1) { frac = v[i] - '0'; fd++; } i++; } }
    if (i < n && v[i] == '%') { if (!pct) return -1; *pct = val; *px = 0; return 0; }
    if (n - i >= 2 && w_ieq_n(v + i, n - i, "em")) val = val * 10 + frac;
    if (val > 2000) val = 2000;
    *px = neg ? -val : val;
    return 0;
}

/* split value into up to 4 space separated tokens */
static int tokens(const char* v, int n, int a[4], int l[4]) {
    int i = 0, c = 0;
    while (i < n && c < 4) {
        while (i < n && w_space(v[i])) i++;
        if (i >= n) break;
        a[c] = i;
        while (i < n && !w_space(v[i])) i++;
        l[c] = i - a[c];
        c++;
    }
    return c;
}

static void box4(const char* v, int n, int16_t out[4]) {
    int a[4], l[4], px[4], c, k;
    c = tokens(v, n, a, l);
    for (k = 0; k < c; k++) if (parse_len(v + a[k], l[k], &px[k], 0) != 0) px[k] = 0;
    if (c == 1) { out[0] = out[1] = out[2] = out[3] = (int16_t)px[0]; }
    else if (c == 2) { out[0] = out[2] = (int16_t)px[0]; out[1] = out[3] = (int16_t)px[1]; }
    else if (c == 3) { out[0] = (int16_t)px[0]; out[1] = out[3] = (int16_t)px[1]; out[2] = (int16_t)px[2]; }
    else if (c == 4) { for (k = 0; k < 4; k++) out[k] = (int16_t)px[k]; }
}

static void apply_decl(we_doc_t* d, we_node_t* n, const char* p, int pl, const char* v, int vl) {
    char prop[24];
    int px, pct, k;
    uint32_t col;
    w_cpy(prop, 24, p, pl);
    for (k = 0; prop[k]; k++) prop[k] = w_low(prop[k]);
    /* trim !important (cascade order by specificity only) */
    for (k = 0; k + 9 <= vl; k++) if (w_ieq_n(v + k, vl - k, "!important")) { vl = k; break; }
    while (vl > 0 && w_space(v[vl - 1])) vl--;
    d->perf.decls_applied++;
    if (w_eq(prop, "display")) {
        if (w_ieq_n(v, vl, "none")) n->display = WE_DISP_NONE;
        else if (w_ieq_n(v, vl, "block")) n->display = WE_DISP_BLOCK;
        else if (w_ieq_n(v, vl, "inline")) n->display = WE_DISP_INLINE;
    } else if (w_eq(prop, "color")) { if (parse_color(v, vl, &col) == 0) n->color = col; }
    else if (w_eq(prop, "background") || w_eq(prop, "background-color")) {
        if (parse_color(v, vl, &col) == 0) { n->bg = col; n->has_bg = 1; }
        else if (w_ieq_n(v, vl, "none") || w_ieq_n(v, vl, "transparent")) n->has_bg = 0;
    }
    else if (w_eq(prop, "margin")) box4(v, vl, n->margin);
    else if (w_eq(prop, "padding")) box4(v, vl, n->padding);
    else if (w_eq(prop, "margin-top")) { if (parse_len(v, vl, &px, 0) == 0) n->margin[0] = (int16_t)px; }
    else if (w_eq(prop, "margin-bottom")) { if (parse_len(v, vl, &px, 0) == 0) n->margin[2] = (int16_t)px; }
    else if (w_eq(prop, "margin-left")) { if (parse_len(v, vl, &px, 0) == 0) n->margin[3] = (int16_t)px; }
    else if (w_eq(prop, "padding-left")) { if (parse_len(v, vl, &px, 0) == 0) n->padding[3] = (int16_t)px; }
    else if (w_eq(prop, "width")) {
        int r = parse_len(v, vl, &px, &pct);
        if (r == 1) { n->width = -1; n->width_pct = 0; }
        else if (r == 0) { if (pct) { n->width_pct = (int16_t)pct; n->width = -1; } else { n->width = (int16_t)px; n->width_pct = 0; } }
    } else if (w_eq(prop, "height")) {
        int r = parse_len(v, vl, &px, 0);
        if (r == 1) n->height = -1; else if (r == 0) n->height = (int16_t)px;
    } else if (w_eq(prop, "border")) {
        int a[4], l[4], c = tokens(v, vl, a, l);
        for (k = 0; k < c; k++) {
            if (parse_color(v + a[k], l[k], &col) == 0) n->border_color = col;
            else if (parse_len(v + a[k], l[k], &px, 0) == 0) n->border = (int16_t)px;
        }
        if (c == 1 && w_ieq_n(v, vl, "none")) n->border = 0;
    } else if (w_eq(prop, "font-weight")) {
        n->bold = (uint8_t)(w_ieq_n(v, vl, "bold") || w_ieq_n(v, vl, "700") || w_ieq_n(v, vl, "800") || w_ieq_n(v, vl, "900"));
    } else if (w_eq(prop, "text-align")) {
        n->align = w_ieq_n(v, vl, "center") ? WE_ALIGN_CENTER : w_ieq_n(v, vl, "right") ? WE_ALIGN_RIGHT : WE_ALIGN_LEFT;
    } else {
        d->perf.decls_applied--;
        d->perf.unknown_props++;
    }
}

static void apply_decls(we_doc_t* d, we_node_t* n, const char* s, int len) {
    int i = 0;
    while (i < len) {
        int pa, pb, va, vb;
        while (i < len && (w_space(s[i]) || s[i] == ';')) i++;
        pa = i;
        while (i < len && s[i] != ':' && s[i] != ';') i++;
        pb = i;
        while (pb > pa && w_space(s[pb - 1])) pb--;
        if (i >= len || s[i] != ':') { if (pb > pa) i++; continue; }
        i++;
        while (i < len && w_space(s[i])) i++;
        va = i;
        while (i < len && s[i] != ';') i++;
        vb = i;
        if (pb > pa && vb > va) apply_decl(d, n, s + pa, pb - pa, s + va, vb - va);
    }
}

static const char* const k_block[] = { "html", "body", "div", "p", "h1", "h2", "h3", "h4", "ul", "ol", "li",
    "section", "header", "footer", "nav", "article", "main", "pre", "blockquote", "hr", "form", "table", "tr", "aside", 0 };
static const char* const k_hidden[] = { "head", "style", "script", "meta", "link", "template", 0 };

static void ua_defaults(we_node_t* n) {
    int i;
    const char* t = n->tag;
    n->display = WE_DISP_INLINE;
    for (i = 0; k_block[i]; i++) if (w_eq(t, k_block[i])) n->display = WE_DISP_BLOCK;
    for (i = 0; k_hidden[i]; i++) if (w_eq(t, k_hidden[i])) n->display = WE_DISP_NONE;
    if (w_eq(t, "body")) { for (i = 0; i < 4; i++) n->margin[i] = 8; }
    if (w_eq(t, "p") || w_eq(t, "ul") || w_eq(t, "ol")) { n->margin[0] = 8; n->margin[2] = 8; }
    if (w_eq(t, "h1") || w_eq(t, "h2") || w_eq(t, "h3") || w_eq(t, "h4")) { n->bold = 1; n->margin[0] = 8; n->margin[2] = 8; }
    if (w_eq(t, "ul") || w_eq(t, "ol")) n->padding[3] = 16;
    if (w_eq(t, "b") || w_eq(t, "strong") || w_eq(t, "th")) n->bold = 1;
    if (w_eq(t, "a")) n->color = 0x0645AD;
    if (w_eq(t, "hr")) { n->height = 2; n->bg = 0x808080; n->has_bg = 1; n->margin[0] = 6; n->margin[2] = 6; }
    if (w_eq(t, "blockquote")) { n->margin[3] = 24; n->margin[1] = 24; }
}

int we_style(we_doc_t* d) {
    uint32_t i, r;
    if (!d || d->nnodes == 0) return -1;
    parse_css(d);
    for (i = 0; i < d->nnodes; i++) {
        we_node_t* n = &d->nodes[i];
        const we_node_t* p = n->parent >= 0 ? &d->nodes[n->parent] : 0;
        n->color = p ? p->color : 0x000000;
        n->bold = p ? p->bold : 0;
        n->align = p ? p->align : WE_ALIGN_LEFT;
        n->width = -1; n->height = -1; n->width_pct = 0;
        n->border_color = n->color;
        if (n->is_text) { n->display = WE_DISP_INLINE; continue; }
        if (i == 0) { n->display = WE_DISP_BLOCK; continue; }
        ua_defaults(n);
        /* cascade: rules sorted by (specificity, order), then inline style */
        {
            uint8_t used[WE_MAX_RULES];
            uint32_t k;
            for (k = 0; k < d->nrules; k++) used[k] = 0;
            for (;;) {
                int best = -1;
                for (r = 0; r < d->nrules; r++) {
                    if (used[r] || !match_rule(d, (int)i, &d->rules[r])) continue;
                    if (best < 0 || d->rules[r].spec < d->rules[best].spec ||
                        (d->rules[r].spec == d->rules[best].spec && d->rules[r].order < d->rules[best].order)) best = (int)r;
                }
                if (best < 0) break;
                used[best] = 1;
                apply_decls(d, n, d->pool + d->rules[best].decl_off, d->rules[best].decl_len);
            }
        }
        if (n->style_len) apply_decls(d, n, d->pool + n->style_off, n->style_len);
        if (n->border && n->border_color == (p ? p->color : 0)) n->border_color = n->color;
        if (p && p->display == WE_DISP_NONE) n->display = WE_DISP_NONE;
    }
    for (i = 0; i < d->nnodes; i++) {
        we_node_t* n = &d->nodes[i];
        if (n->parent >= 0 && d->nodes[n->parent].display == WE_DISP_NONE) n->display = WE_DISP_NONE;
    }
    return 0;
}

/* ---------- layout and paint ---------- */
static os_fb_web_item_t* emit(we_doc_t* d, uint8_t kind, int x, int y, int w, int h, uint32_t color) {
    os_fb_web_item_t* it;
    if (d->nitems >= OS_FB_WEB_ITEMS) { d->perf.items_dropped++; return 0; }
    it = &d->items[d->nitems++];
    w_zero(it, sizeof(*it));
    it->kind = kind;
    it->x = (int16_t)x; it->y = (int16_t)y;
    it->w = (uint16_t)(w < 0 ? 0 : w); it->h = (uint16_t)(h < 0 ? 0 : h);
    it->color = color;
    return it;
}

/* inline formatting context: words gathered from a run of inline nodes */
#define WE_LINE_WORDS 64
typedef struct {
    we_doc_t* d;
    int x0, width, y, align;
    int cur_w;
    int nwords;
    struct { const char* s; int len; int node; int space_before; } w[WE_LINE_WORDS];
    int lines;
    int pending_space;
} we_inline_t;

static void flush_line(we_inline_t* f, int force) {
    int i, x, total = 0;
    we_doc_t* d = f->d;
    if (f->nwords == 0 && !force) return;
    for (i = 0; i < f->nwords; i++) total += (f->w[i].len + (i && f->w[i].space_before ? 1 : 0)) * WE_CHAR_W;
    x = f->x0;
    if (f->align == WE_ALIGN_CENTER && total < f->width) x += (f->width - total) / 2;
    else if (f->align == WE_ALIGN_RIGHT && total < f->width) x += f->width - total;
    i = 0;
    while (i < f->nwords) {
        /* merge words of the same node into one TEXT item */
        int node = f->w[i].node, j = i, len = 0, xs;
        char buf[OS_FB_WEB_TEXT_LEN];
        os_fb_web_item_t* it;
        const we_node_t* n;
        if (i && f->w[i].space_before) x += WE_CHAR_W;
        xs = x;
        while (j < f->nwords && f->w[j].node == node) {
            int k, sp = (j > i && f->w[j].space_before) ? 1 : 0;
            if (len + sp + f->w[j].len > OS_FB_WEB_TEXT_LEN - 1) break;
            if (sp) buf[len++] = ' ';
            for (k = 0; k < f->w[j].len; k++) buf[len++] = f->w[j].s[k];
            j++;
        }
        if (j == i) { /* single overlong word: cut */
            int k;
            for (k = 0; k < f->w[i].len && k < OS_FB_WEB_TEXT_LEN - 1; k++) buf[k] = f->w[i].s[k];
            len = k; j = i + 1;
        }
        buf[len] = 0;
        n = &d->nodes[d->nodes[node].parent >= 0 ? d->nodes[node].parent : node];
        if (n->has_bg && n->display == WE_DISP_INLINE) emit(d, OS_FB_WEB_RECT, xs, f->y, len * WE_CHAR_W, WE_LINE_H, n->bg);
        it = emit(d, OS_FB_WEB_TEXT, xs, f->y + 1, len * WE_CHAR_W, 8, d->nodes[node].color);
        if (it) { it->bold = d->nodes[node].bold; w_cpy(it->text, OS_FB_WEB_TEXT_LEN, buf, len); }
        {   /* UX4: text nodes get the bounding box of their runs, so a
             * click on a link's words finds the <a> (we_link_at) */
            we_node_t* tn = &d->nodes[node];
            int x1 = xs + len * WE_CHAR_W, y1 = f->y + WE_LINE_H;
            if (tn->w <= 0) { tn->x = (int16_t)xs; tn->y = (int16_t)f->y; tn->w = (int16_t)(x1 - xs); tn->h = WE_LINE_H; }
            else {
                int ax = tn->x < xs ? tn->x : xs, ay = tn->y < f->y ? tn->y : f->y;
                int bx = tn->x + tn->w > x1 ? tn->x + tn->w : x1, by = tn->y + tn->h > y1 ? tn->y + tn->h : y1;
                tn->x = (int16_t)ax; tn->y = (int16_t)ay; tn->w = (int16_t)(bx - ax); tn->h = (int16_t)(by - ay);
            }
        }
        x = xs + len * WE_CHAR_W;
        i = j;
    }
    f->y += WE_LINE_H;
    f->lines++;
    d->perf.lines++;
    f->nwords = 0;
    f->cur_w = 0;
}

static void add_word(we_inline_t* f, const char* s, int len, int node, int space) {
    int ww = len * WE_CHAR_W + ((f->nwords && space) ? WE_CHAR_W : 0);
    if (f->nwords && f->cur_w + ww > f->width) { flush_line(f, 0); ww = len * WE_CHAR_W; }
    if (f->nwords >= WE_LINE_WORDS) { flush_line(f, 0); ww = len * WE_CHAR_W; }
    f->w[f->nwords].s = s; f->w[f->nwords].len = len; f->w[f->nwords].node = node;
    f->w[f->nwords].space_before = space;
    f->nwords++;
    f->cur_w += ww;
}

static void inline_node(we_inline_t* f, int idx, int depth) {
    we_doc_t* d = f->d;
    we_node_t* n = &d->nodes[idx];
    int c;
    if (n->display == WE_DISP_NONE || depth > WE_DEPTH_MAX) return;
    if (n->is_text) {
        const char* s = d->pool + n->text_off;
        int i = 0, len = n->text_len;
        if (n->lead_space) f->pending_space = 1;
        while (i < len) {
            int a;
            while (i < len && s[i] == ' ') { i++; f->pending_space = 1; }
            a = i;
            while (i < len && s[i] != ' ') i++;
            if (i > a) { add_word(f, s + a, i - a, idx, f->pending_space); f->pending_space = 0; }
        }
        if (n->trail_space) f->pending_space = 1;
        return;
    }
    if (w_eq(n->tag, "br")) { flush_line(f, 1); f->pending_space = 0; return; }
    if (w_eq(n->tag, "img")) {
        /* no image decoder: the parser stored "[alt]" in the pool */
        if (n->text_len) add_word(f, d->pool + n->text_off, n->text_len, idx, f->pending_space);
        f->pending_space = 0;
        return;
    }
    for (c = n->first; c >= 0; c = d->nodes[c].next) inline_node(f, c, depth + 1);
}

static int layout_block(we_doc_t* d, int idx, int x, int y, int cw, int depth);

static int layout_children(we_doc_t* d, int idx, int cx, int cy, int cwidth, int depth) {
    int c = d->nodes[idx].first, y = cy;
    while (c >= 0) {
        we_node_t* n = &d->nodes[c];
        if (n->display == WE_DISP_NONE) { c = n->next; continue; }
        if (n->display == WE_DISP_BLOCK) {
            y += layout_block(d, c, cx, y, cwidth, depth + 1);
            c = n->next;
            continue;
        }
        {   /* run of inline siblings; inline content never lays out a
             * block, so one static context is enough (no re-entry) */
            static we_inline_t f;
            f.d = d; f.x0 = cx; f.width = cwidth; f.y = y; f.align = d->nodes[idx].align;
            f.cur_w = 0; f.nwords = 0; f.lines = 0; f.pending_space = 0;
            while (c >= 0 && d->nodes[c].display != WE_DISP_BLOCK) {
                if (d->nodes[c].display != WE_DISP_NONE) inline_node(&f, c, depth + 1);
                c = d->nodes[c].next;
            }
            flush_line(&f, 0);
            y = f.y;
        }
    }
    return y - cy;
}

static int layout_block(we_doc_t* d, int idx, int x, int y, int cw, int depth) {
    we_node_t* n = &d->nodes[idx];
    int bx, by, bw, bh, content_x, content_w, content_h, slot, border_slot = -1;
    if (depth > WE_DEPTH_MAX) return 0;
    d->perf.boxes++;
    bx = x + n->margin[3];
    by = y + n->margin[0];
    if (n->width_pct > 0) bw = (cw * n->width_pct) / 100;
    else if (n->width >= 0) bw = n->width + 2 * n->border + n->padding[1] + n->padding[3];
    else bw = cw - n->margin[1] - n->margin[3];
    if (bw < 0) bw = 0;
    /* reserve the background item now so it paints under the children */
    slot = n->has_bg ? (int)d->nitems : -1;
    if (n->has_bg && !emit(d, OS_FB_WEB_RECT, 0, 0, 0, 0, n->bg)) slot = -1;
    content_x = bx + n->border + n->padding[3];
    content_w = bw - 2 * n->border - n->padding[1] - n->padding[3];
    if (content_w < WE_CHAR_W) content_w = WE_CHAR_W;
    content_h = layout_children(d, idx, content_x, by + n->border + n->padding[0], content_w, depth);
    if (n->height >= 0) content_h = n->height;
    bh = content_h + n->padding[0] + n->padding[2] + 2 * n->border;
    n->x = (int16_t)bx; n->y = (int16_t)by; n->w = (int16_t)bw; n->h = (int16_t)bh;
    if (slot >= 0) {
        os_fb_web_item_t* it = &d->items[slot];
        it->x = (int16_t)bx; it->y = (int16_t)by; it->w = (uint16_t)bw; it->h = (uint16_t)bh;
    }
    if (n->border > 0) {
        os_fb_web_item_t* it = emit(d, OS_FB_WEB_BORDER, bx, by, bw, bh, n->border_color);
        if (it) it->bold = (uint8_t)(n->border > 8 ? 8 : n->border);
        border_slot = 1;
    }
    (void)border_slot;
    return n->margin[0] + bh + n->margin[2];
}

int we_layout(we_doc_t* d, int viewport_w) {
    int h;
    if (!d || d->nnodes == 0 || viewport_w < 64) return -1;
    d->nitems = 0;
    d->perf.boxes = d->perf.lines = d->perf.items_dropped = 0;
    h = layout_block(d, 0, 0, 0, viewport_w, 0);
    d->perf.items = d->nitems;
    d->perf.page_w = (uint32_t)viewport_w;
    d->perf.page_h = (uint32_t)h;
    return 0;
}

int we_render(we_doc_t* d, const char* html, int len, int viewport_w) {
    int rc = we_parse(d, html, len);
    if (rc != 0) return rc;
    rc = we_style(d);
    if (rc != 0) return rc;
    return we_layout(d, viewport_w);
}

/* ---------- US-045: inspector ---------- */
int we_find_id(const we_doc_t* d, const char* id) {
    uint32_t i;
    for (i = 0; i < d->nnodes; i++) if (!d->nodes[i].is_text && d->nodes[i].id[0] && w_eq(d->nodes[i].id, id)) return (int)i;
    return -1;
}

static void out_num(we_out_fn out, void* ctx, const char* label, int v) {
    char num[12];
    out(ctx, label);
    w_itoa(v, num);
    out(ctx, num);
}

static void out_hex(we_out_fn out, void* ctx, const char* label, uint32_t v) {
    char h[8];
    int k;
    out(ctx, label);
    for (k = 0; k < 6; k++) h[k] = "0123456789abcdef"[(v >> (20 - 4 * k)) & 15U];
    h[6] = 0;
    out(ctx, "#");
    out(ctx, h);
}

void we_dump_dom(const we_doc_t* d, we_out_fn out, void* ctx) {
    uint32_t i;
    for (i = 1; i < d->nnodes; i++) {
        const we_node_t* n = &d->nodes[i];
        int depth = 0, p = n->parent;
        char line[64];
        int k;
        while (p > 0) { depth++; p = d->nodes[p].parent; }
        out_num(out, ctx, "dom ", (int)i);
        out(ctx, " ");
        for (k = 0; k < depth && k < 12; k++) out(ctx, ". ");
        if (n->is_text) {
            out(ctx, "\"");
            w_cpy(line, 40, d->pool + n->text_off, n->text_len);
            out(ctx, line);
            out(ctx, "\"\n");
            continue;
        }
        out(ctx, n->tag);
        if (n->id[0]) { out(ctx, "#"); out(ctx, n->id); }
        if (n->cls[0]) { out(ctx, " class="); out(ctx, n->cls); }
        out(ctx, n->display == WE_DISP_NONE ? " none" : n->display == WE_DISP_BLOCK ? " block" : " inline");
        out(ctx, "\n");
    }
}

void we_inspect(const we_doc_t* d, int node, we_out_fn out, void* ctx) {
    const we_node_t* n;
    if (node < 0 || (uint32_t)node >= d->nnodes) { out(ctx, "inspect none\n"); return; }
    n = &d->nodes[node];
    out(ctx, "inspect "); out(ctx, n->is_text ? "#text" : n->tag);
    if (n->id[0]) { out(ctx, "#"); out(ctx, n->id); }
    out(ctx, n->display == WE_DISP_NONE ? " display none" : n->display == WE_DISP_BLOCK ? " display block" : " display inline");
    out_num(out, ctx, " box ", n->x); out_num(out, ctx, " ", n->y);
    out_num(out, ctx, " ", n->w); out_num(out, ctx, "x", n->h);
    out_hex(out, ctx, " color ", n->color);
    if (n->has_bg) out_hex(out, ctx, " bg ", n->bg);
    out_num(out, ctx, " margin ", n->margin[0]); out_num(out, ctx, " ", n->margin[1]);
    out_num(out, ctx, " ", n->margin[2]); out_num(out, ctx, " ", n->margin[3]);
    out_num(out, ctx, " padding ", n->padding[0]); out_num(out, ctx, " ", n->padding[1]);
    out_num(out, ctx, " ", n->padding[2]); out_num(out, ctx, " ", n->padding[3]);
    out_num(out, ctx, " border ", n->border);
    out_num(out, ctx, " bold ", n->bold);
    out(ctx, n->align == WE_ALIGN_CENTER ? " align center" : n->align == WE_ALIGN_RIGHT ? " align right" : " align left");
    out(ctx, "\n");
}

/* UX4: hit test for the desktop browser links. Text nodes have no box of
 * their own here: the deepest element whose border box holds the point
 * wins, then its ancestors are searched for an <a href>. */
int we_link_at(const we_doc_t* d, int x, int y, char* href, int max) {
    int i, best = -1, best_area = 0x7FFFFFFF;
    if (!d || !href || max <= 0) return 0;
    href[0] = 0;
    for (i = 0; i < (int)d->nnodes; i++) {
        const we_node_t* n = &d->nodes[i];
        int area;
        if ((!n->is_text && n->display == WE_DISP_NONE) || n->w <= 0 || n->h <= 0) continue;
        if (x < n->x || y < n->y || x >= n->x + n->w || y >= n->y + n->h) continue;
        area = (int)n->w * (int)n->h;
        if (area <= best_area) { best_area = area; best = i; }
    }
    while (best >= 0) {
        const we_node_t* n = &d->nodes[best];
        if (n->href_len && w_eq(n->tag, "a")) {
            int k, len = n->href_len < max - 1 ? n->href_len : max - 1;
            for (k = 0; k < len; k++) href[k] = d->pool[n->href_off + k];
            href[len] = 0;
            return len;
        }
        best = n->parent;
    }
    return 0;
}
