/* userspace/webeng.c - Lot 6 (US-031, US-043, US-045). See webeng.h. */
#include "webeng.h"
#include "imgdec.h"

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
                if (w_eq(name, "td") || w_eq(name, "th") || w_eq(name, "tr")) {
                    int at;
                    for (at = sp - 1; at > 0; at--) {
                        const char *open = d->nodes[stack[at]].tag;
                        if (w_eq(open, "table")) break;
                        if (w_eq(name, "tr") ? w_eq(open, "tr") : (w_eq(open, "td") || w_eq(open, "th"))) {
                            sp = at;
                            break;
                        }
                        if (!w_eq(name, "tr") && w_eq(open, "tr")) break;
                    }
                }
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
                    char an[12], av[256];
                    int ak = 0, vk = 0, truncated = 0;
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
                            if (vk < (int)sizeof(av) - 1) av[vk++] = html[j];
                            else truncated = 1;
                            j++;
                        }
                        if (q && j < len) j++;
                    }
                    av[vk] = 0;
                    if (truncated) { diag(d, "attribute exceeds limit", idx); continue; }
                    if (w_eq(an, "id")) w_cpy(n->id, 16, av, vk);
                    else if (w_eq(an, "class")) w_cpy(n->cls, 32, av, vk);
                    else if (w_eq(an, "style")) { if (pool_put(d, av, vk, &n->style_off) == 0) n->style_len = (uint16_t)vk; }
                    else if (w_eq(an, "alt")) { if (pool_put(d, av, vk, &n->alt_off) == 0) n->alt_len = (uint16_t)vk; }
                    else if (w_eq(an, "colspan") || w_eq(an, "rowspan")) {
                        if (!(vk == 1 && av[0] == '1')) {
                            n->span_refused = 1;
                            diag(d, "table span refused", idx);
                        }
                    }
                    else if (w_eq(an, "src")) {
                        if (pool_put(d, av, vk, &n->src_off) == 0) n->src_len = (uint16_t)vk;
                    }
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
                        if (c < end) diag(d, "css source exceeds limit", idx);
                        if (d->css_len < WE_CSS_MAX) d->css[d->css_len++] = ' ';
                        else diag(d, "css source exceeds limit", idx);
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
    while (i < n && v[i] >= '0' && v[i] <= '9') {
        if (val < 2000) val = val * 10 + (v[i] - '0');
        if (val > 2000) val = 2000;
        i++;
    }
    if (i < n && v[i] == '.') { i++; while (i < n && v[i] >= '0' && v[i] <= '9') { if (fd < 1) { frac = v[i] - '0'; fd++; } i++; } }
    if (i < n && v[i] == '%') { if (!pct || neg) return -1; *pct = val; *px = 0; return 0; }
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

static int value_is(const char *v, int len, const char *s) {
    return len == w_len(s) && w_ieq_n(v, len, s);
}

static int bounded_uint(const char *v, int len, int max, int *out) {
    int i, n = 0;
    if (len == 0) return -1;
    for (i = 0; i < len; i++) {
        if (v[i] < '0' || v[i] > '9') return -1;
        if (n > (max - (v[i] - '0')) / 10) return -1;
        n = n * 10 + v[i] - '0';
        if (n > max) return -1;
    }
    *out = n;
    return 0;
}

/* Track syntax deliberately stops at pixels and equal fractions. */
static int parse_tracks(const char *v, int len, int16_t *sizes) {
    int i = 0, count = 0;
    if (w_ieq_n(v, len, "repeat(")) {
        int a = 7, b = a, n;
        while (b < len && v[b] >= '0' && v[b] <= '9') b++;
        if (bounded_uint(v + a, b - a, WE_TRACKS_MAX, &n) || n < 1) return -1;
        while (b < len && w_space(v[b])) b++;
        if (b >= len || v[b++] != ',') return -1;
        while (b < len && w_space(v[b])) b++;
        if (!w_ieq_n(v + b, len - b, "1fr")) return -1;
        b += 3;
        while (b < len && w_space(v[b])) b++;
        if (b >= len || v[b++] != ')') return -1;
        while (b < len && w_space(v[b])) b++;
        if (b != len) return -1;
        for (i = 0; i < n; i++) sizes[i] = -1;
        return n;
    }
    while (i < len) {
        int a, px;
        while (i < len && w_space(v[i])) i++;
        if (i == len) break;
        a = i;
        while (i < len && !w_space(v[i])) i++;
        if (count >= WE_TRACKS_MAX) return -1;
        if (value_is(v + a, i - a, "1fr")) sizes[count++] = -1;
        else {
            int end = i;
            if (end - a >= 2 && value_is(v + end - 2, 2, "px")) end -= 2;
            if (bounded_uint(v + a, end - a, 2000, &px)) return -1;
            sizes[count++] = (int16_t)px;
        }
    }
    return count ? count : -1;
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
        if (value_is(v, vl, "none")) n->display = WE_DISP_NONE;
        else if (value_is(v, vl, "block")) n->display = WE_DISP_BLOCK;
        else if (value_is(v, vl, "inline")) n->display = WE_DISP_INLINE;
        else if (value_is(v, vl, "flex")) n->display = WE_DISP_FLEX;
        else if (value_is(v, vl, "grid")) n->display = WE_DISP_GRID;
        else if (value_is(v, vl, "table")) n->display = WE_DISP_TABLE;
        else if (value_is(v, vl, "table-row")) n->display = WE_DISP_ROW;
        else if (value_is(v, vl, "table-cell")) n->display = WE_DISP_CELL;
        else diag(d, "display refused", -1);
    } else if (w_eq(prop, "flex-direction")) {
        if (value_is(v, vl, "row") || value_is(v, vl, "column"))
            n->flex_dir = (uint8_t)value_is(v, vl, "column");
        else diag(d, "flex-direction refused", -1);
    } else if (w_eq(prop, "flex-wrap")) {
        if (value_is(v, vl, "wrap") || value_is(v, vl, "nowrap"))
            n->flex_wrap = (uint8_t)value_is(v, vl, "wrap");
        else diag(d, "flex-wrap refused", -1);
    } else if (w_eq(prop, "flex-grow")) {
        if (bounded_uint(v, vl, 1000, &px) == 0) n->flex_grow = (int16_t)px;
        else diag(d, "flex-grow refused", -1);
    } else if (w_eq(prop, "flex-shrink")) {
        if (bounded_uint(v, vl, 1000, &px) == 0) n->flex_shrink = (int16_t)px;
        else diag(d, "flex-shrink refused", -1);
    } else if (w_eq(prop, "flex-basis")) {
        int end = vl, value;
        if (value_is(v, vl, "auto")) {
            n->flex_basis = -1; n->basis_pct = 0; n->basis_set = 1;
        } else {
            int percent = vl > 0 && v[vl - 1] == '%';
            if (percent) end--;
            else if (vl >= 2 && value_is(v + vl - 2, 2, "px")) end -= 2;
            if (bounded_uint(v, end, 2000, &value) == 0) {
                n->flex_basis = (int16_t)(percent ? -1 : value);
                n->basis_pct = (int16_t)(percent ? value : 0);
                /* Percent zero is a definite zero, not auto. */
                if (percent && value == 0) n->flex_basis = 0;
                n->basis_set = 1;
            } else diag(d, "flex-basis refused", -1);
        }
    } else if (w_eq(prop, "float")) {
        if (value_is(v, vl, "left")) n->float_side = WE_FLOAT_LEFT;
        else if (value_is(v, vl, "right")) n->float_side = WE_FLOAT_RIGHT;
        else if (value_is(v, vl, "none")) n->float_side = WE_FLOAT_NONE;
        else diag(d, "float refused", -1);
    } else if (w_eq(prop, "position")) {
        if (value_is(v, vl, "relative")) n->relative = 1;
        else if (value_is(v, vl, "static")) n->relative = 0;
        else diag(d, "position refused", -1);
    } else if (w_eq(prop, "left") || w_eq(prop, "top")) {
        if (parse_len(v, vl, &px, 0) == 0) {
            if (w_eq(prop, "left")) n->offset_x = (int16_t)px;
            else n->offset_y = (int16_t)px;
        } else diag(d, "position offset refused", -1);
    } else if (w_eq(prop, "list-style-type")) {
        if (value_is(v, vl, "none")) n->list_style = WE_LIST_NONE;
        else if (value_is(v, vl, "disc")) n->list_style = WE_LIST_DISC;
        else if (value_is(v, vl, "decimal")) n->list_style = WE_LIST_DECIMAL;
        else if (value_is(v, vl, "square")) n->list_style = WE_LIST_SQUARE;
        else diag(d, "list marker refused", -1);
    } else if (w_eq(prop, "min-width") || w_eq(prop, "max-width")) {
        int is_min = w_eq(prop, "min-width");
        int rc = parse_len(v, vl, &px, &pct);
        if (value_is(v, vl, "none") || rc == 1) {
            if (is_min) { n->min_width = -1; n->min_pct = 0; }
            else { n->max_width = -1; n->max_pct = 0; }
        } else if (rc == 0 && px >= 0) {
            if (is_min) { n->min_width = (int16_t)px; n->min_pct = (int16_t)pct; }
            else { n->max_width = (int16_t)px; n->max_pct = (int16_t)pct; }
        } else diag(d, "width constraint refused", -1);
    } else if (w_eq(prop, "gap")) {
        if (parse_len(v, vl, &px, 0) == 0 && px >= 0) n->gap = (int16_t)px;
    } else if (w_eq(prop, "grid-template-columns") || w_eq(prop, "grid-template-rows")) {
        int16_t sizes[WE_TRACKS_MAX];
        int count = parse_tracks(v, vl, sizes);
        if (count < 0) diag(d, "grid tracks refused", -1);
        else if (w_eq(prop, "grid-template-columns")) {
            int valid = 1;
            for (k = 0; k < count; k++) if (sizes[k] != -1) valid = 0;
            if (valid) n->grid_cols = (uint8_t)count;
            else diag(d, "grid columns require 1fr", -1);
        } else {
            n->grid_rows = (uint8_t)count;
            for (k = 0; k < count; k++) n->row_size[k] = sizes[k];
        }
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
    } else if (w_eq(prop, "min-height") || w_eq(prop, "max-height")) {
        int end = vl;
        if (vl >= 2 && value_is(v + vl - 2, 2, "px")) end -= 2;
        if (bounded_uint(v, end, 2000, &px)) diag(d, "height limit refused", -1);
        else if (w_eq(prop, "min-height")) n->min_height = (int16_t)px;
        else n->max_height = (int16_t)px;
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
    if (w_eq(t, "table")) n->display = WE_DISP_TABLE;
    if (w_eq(t, "tr")) n->display = WE_DISP_ROW;
    if (w_eq(t, "td") || w_eq(t, "th")) n->display = WE_DISP_CELL;
    if (w_eq(t, "ul")) n->list_style = WE_LIST_DISC;
    if (w_eq(t, "ol")) n->list_style = WE_LIST_DECIMAL;
    if (w_eq(t, "body")) { for (i = 0; i < 4; i++) n->margin[i] = 8; }
    if (w_eq(t, "p") || w_eq(t, "ul") || w_eq(t, "ol")) { n->margin[0] = 8; n->margin[2] = 8; }
    if (w_eq(t, "h1") || w_eq(t, "h2") || w_eq(t, "h3") || w_eq(t, "h4")) { n->bold = 1; n->margin[0] = 8; n->margin[2] = 8; }
    if (w_eq(t, "ul")) n->padding[3] = 16;
    if (w_eq(t, "ol")) n->padding[3] = 40;
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
        n->flex_grow = -1;
        n->flex_shrink = n->flex_basis = -1;
        n->basis_pct = n->basis_set = 0;
        n->min_width = n->max_width = -1;
        n->min_height = n->max_height = -1;
        n->list_style = p ? p->list_style : WE_LIST_NONE;
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
    if (x < -WE_LAYOUT_MAX) x = -WE_LAYOUT_MAX;
    if (x > WE_LAYOUT_MAX) x = WE_LAYOUT_MAX;
    if (y < -WE_LAYOUT_MAX) y = -WE_LAYOUT_MAX;
    if (y > WE_LAYOUT_MAX) y = WE_LAYOUT_MAX;
    if (w > WE_LAYOUT_MAX) w = WE_LAYOUT_MAX;
    if (h > WE_LAYOUT_MAX) h = WE_LAYOUT_MAX;
    it->x = (int16_t)x; it->y = (int16_t)y;
    it->w = (uint16_t)(w < 0 ? 0 : w); it->h = (uint16_t)(h < 0 ? 0 : h);
    it->color = color;
    return it;
}

/* Local images use the same decoder as img-info and the desktop viewer. */
static we_image_read_fn image_read;
static void *image_ctx;
static struct {
    unsigned char source[WE_IMAGE_SOURCE_MAX];
    unsigned char scratch[WE_IMAGE_SCRATCH_MAX];
    imgdec_t decoded;
    os_fb_web_item_t rect[OS_FB_WEB_ITEMS];
    char path[256];
} image_work;
typedef char image_workspace_is_bounded[
    sizeof(image_work) <= WE_IMAGE_WORKSPACE_MAX ? 1 : -1];

void we_set_image_reader(we_image_read_fn read, void *ctx) {
    image_read = read;
    image_ctx = ctx;
}

static int image_local_path(we_doc_t *d, const we_node_t *n) {
    int i, start = 0;
    if (!n->src_len || n->src_len >= sizeof(image_work.path)) return -1;
    for (i = 0; i < n->src_len; i++) {
        char c = d->pool[n->src_off + i];
        if (c < 33 || c > 126 || c == ':' || c == '\\' || c == '%' || c == '?' || c == '#') return -1;
        image_work.path[i] = c;
    }
    image_work.path[i] = 0;
    if (image_work.path[0] != '/' || image_work.path[1] == '/') return -1;
    for (i = 1; i <= n->src_len; i++) {
        if (image_work.path[i] == '/' || image_work.path[i] == 0) {
            if (i - start == 3 && image_work.path[start + 1] == '.' && image_work.path[start + 2] == '.') return -1;
            start = i;
        }
    }
    return 0;
}

static int image_fail(we_doc_t *d, const char *message, int idx) {
    diag(d, message, idx);
    d->perf.image_refused++;
    return -1;
}

static int image_open(we_doc_t *d, int idx) {
    const we_node_t *n = &d->nodes[idx];
    int bytes, rc;
    if (!n->src_len) return -1;
    if (image_local_path(d, n)) return image_fail(d, "image local path refused", idx);
    if (!image_read) return image_fail(d, "image reader unavailable", idx);
    bytes = image_read(image_ctx, image_work.path, image_work.source, WE_IMAGE_SOURCE_MAX);
    if (bytes < 0) return image_fail(d, "image read failed", idx);
    if (bytes > WE_IMAGE_SOURCE_MAX) return image_fail(d, "image source exceeds limit", idx);
    rc = imgdec_probe(image_work.source, bytes, &image_work.decoded);
    if (rc) return image_fail(d, "image format refused", idx);
    if (image_work.decoded.w > WE_IMAGE_MAX_W || image_work.decoded.h > WE_IMAGE_MAX_H)
        return image_fail(d, "image dimensions exceed limit", idx);
    if (imgdec_scratch_need(&image_work.decoded) > WE_IMAGE_SCRATCH_MAX)
        return image_fail(d, "image scratch exceeds limit", idx);
    rc = imgdec_open(image_work.source, bytes, image_work.scratch,
                     WE_IMAGE_SCRATCH_MAX, &image_work.decoded);
    if (rc) return image_fail(d, "image decode failed", idx);
    return 0;
}

/* Preflight the entire rectangle set. Failure never publishes half an
 * image and never consumes a display-list slot intended for the alt. */
static int image_paint(we_doc_t *d, int idx, int x, int y, int available) {
    we_node_t *n = &d->nodes[idx];
    int w, h, row, col, nr = 0, room = OS_FB_WEB_ITEMS - (int)d->nitems;
    if (image_open(d, idx)) return -1;
    w = n->width >= 0 ? n->width : image_work.decoded.w;
    if (n->width_pct > 0) w = available * n->width_pct / 100;
    h = n->height >= 0 ? n->height : image_work.decoded.h;
    if (w < 1 || h < 1 || w > WE_IMAGE_MAX_W || h > WE_IMAGE_MAX_H)
        return image_fail(d, "image output exceeds limit", idx);
    for (row = 0; row < h; row++) {
        col = 0;
        while (col < w) {
            int first = col, r, found = -1;
            unsigned char rgb[3];
            uint32_t color;
            imgdec_pixel(&image_work.decoded, col * image_work.decoded.w / w,
                         row * image_work.decoded.h / h, rgb);
            color = ((uint32_t)rgb[0] << 16) | ((uint32_t)rgb[1] << 8) | rgb[2];
            col++;
            while (col < w) {
                uint32_t next;
                imgdec_pixel(&image_work.decoded, col * image_work.decoded.w / w,
                             row * image_work.decoded.h / h, rgb);
                next = ((uint32_t)rgb[0] << 16) | ((uint32_t)rgb[1] << 8) | rgb[2];
                if (next != color) break;
                col++;
            }
            for (r = nr - 1; r >= 0; r--) {
                os_fb_web_item_t *it = &image_work.rect[r];
                if (it->x == first && it->w == col - first && it->color == color &&
                    it->y + it->h == row) { found = r; break; }
            }
            if (found >= 0) image_work.rect[found].h++;
            else {
                os_fb_web_item_t *it;
                if (nr >= room) return image_fail(d, "image display budget exceeded", idx);
                it = &image_work.rect[nr++];
                w_zero(it, sizeof(*it));
                it->kind = OS_FB_WEB_RECT;
                it->x = (int16_t)first;
                it->y = (int16_t)row;
                it->w = (uint16_t)(col - first);
                it->h = 1;
                it->color = color;
            }
        }
    }
    for (row = 0; row < nr; row++) {
        os_fb_web_item_t *it = &image_work.rect[row];
        emit(d, OS_FB_WEB_RECT, x + it->x, y + it->y, it->w, it->h, it->color);
    }
    n->x = (int16_t)x;
    n->y = (int16_t)y;
    n->w = (int16_t)w;
    n->h = (int16_t)h;
    d->perf.images++;
    return h;
}

/* inline formatting context: words gathered from a run of inline nodes */
#define WE_LINE_WORDS 64
typedef struct {
    we_doc_t* d;
    int x0, width, y, align;
    int base_x, base_width, nfloat;
    struct { int x, y, w, h; uint8_t side; } floats[WE_FLOATS_MAX];
    int cur_w;
    int nwords;
    struct { const char* s; int len; int node; int space_before; } w[WE_LINE_WORDS];
    int lines;
    int pending_space;
} we_inline_t;

static int limit_coord(int n) {
    if (n < -WE_LAYOUT_MAX) return -WE_LAYOUT_MAX;
    if (n > WE_LAYOUT_MAX) return WE_LAYOUT_MAX;
    return n;
}

static void flow_region(we_inline_t *f, int need) {
    int pass;
    for (pass = 0; pass <= WE_FLOATS_MAX; pass++) {
        int i, left = f->base_x, right = left + f->base_width;
        int next = WE_LAYOUT_MAX;
        for (i = 0; i < f->nfloat; i++) {
            int bottom = f->floats[i].y + f->floats[i].h;
            if (bottom <= f->y || f->floats[i].y >= f->y + WE_LINE_H) continue;
            if (bottom < next) next = bottom;
            if (f->floats[i].side == WE_FLOAT_LEFT && f->floats[i].x + f->floats[i].w > left)
                left = f->floats[i].x + f->floats[i].w;
            if (f->floats[i].side == WE_FLOAT_RIGHT && f->floats[i].x < right)
                right = f->floats[i].x;
        }
        f->x0 = left;
        f->width = right > left ? right - left : 0;
        if (f->width >= need || f->nwords || next == WE_LAYOUT_MAX ||
            (need > f->base_width && f->width >= f->base_width)) return;
        f->y = next;
    }
}

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
            if (tn->w <= 0) { tn->x = (int16_t)limit_coord(xs); tn->y = (int16_t)limit_coord(f->y); tn->w = (int16_t)(x1 - xs); tn->h = WE_LINE_H; }
            else {
                int ax = tn->x < xs ? tn->x : xs, ay = tn->y < f->y ? tn->y : f->y;
                int bx = tn->x + tn->w > x1 ? tn->x + tn->w : x1, by = tn->y + tn->h > y1 ? tn->y + tn->h : y1;
                tn->x = (int16_t)limit_coord(ax); tn->y = (int16_t)limit_coord(ay); tn->w = (int16_t)(bx - ax); tn->h = (int16_t)(by - ay);
            }
        }
        x = xs + len * WE_CHAR_W;
        i = j;
    }
    f->y = limit_coord(f->y + WE_LINE_H);
    f->lines++;
    d->perf.lines++;
    f->nwords = 0;
    f->cur_w = 0;
}

static void add_word(we_inline_t* f, const char* s, int len, int node, int space) {
    int ww;
    flow_region(f, len * WE_CHAR_W);
    ww = len * WE_CHAR_W + ((f->nwords && space) ? WE_CHAR_W : 0);
    if (f->nwords && f->cur_w + ww > f->width) { flush_line(f, 0); flow_region(f, len * WE_CHAR_W); ww = len * WE_CHAR_W; }
    if (f->nwords >= WE_LINE_WORDS) { flush_line(f, 0); ww = len * WE_CHAR_W; }
    f->w[f->nwords].s = s; f->w[f->nwords].len = len; f->w[f->nwords].node = node;
    f->w[f->nwords].space_before = space;
    f->nwords++;
    f->cur_w += ww;
}

static void place_float(we_inline_t *f, int idx, int depth);

static void inline_node(we_inline_t* f, int idx, int depth) {
    we_doc_t* d = f->d;
    we_node_t* n = &d->nodes[idx];
    int c;
    if (n->display == WE_DISP_NONE || depth > WE_DEPTH_MAX) return;
    if (n->float_side && !n->is_text) { place_float(f, idx, depth); return; }
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
        int h;
        flush_line(f, 0);
        flow_region(f, WE_CHAR_W);
        h = image_paint(d, idx, f->x0, f->y, f->width);
        if (h >= 0) { f->y = limit_coord(f->y + h); return; }
        if (n->text_len) add_word(f, d->pool + n->text_off, n->text_len, idx, f->pending_space);
        f->pending_space = 0;
        return;
    }
    for (c = n->first; c >= 0; c = d->nodes[c].next) inline_node(f, c, depth + 1);
}

static int layout_block(we_doc_t* d, int idx, int x, int y, int cw, int depth);
static int layout_flex(we_doc_t* d, int idx, int cx, int cy, int cwidth, int depth);
static int is_box(uint8_t display) {
    return display == WE_DISP_BLOCK || display == WE_DISP_FLEX || display == WE_DISP_GRID ||
           display == WE_DISP_TABLE || display == WE_DISP_ROW || display == WE_DISP_CELL;
}

static int node_border_extra(const we_node_t *n) {
    return 2 * n->border + n->padding[1] + n->padding[3];
}

static int constrained_width(const we_node_t *n, int width, int containing) {
    int low = n->min_pct > 0 ? containing * n->min_pct / 100 : n->min_width;
    int high = n->max_pct > 0 ? containing * n->max_pct / 100 : n->max_width;
    if (high >= 0 && width > high) width = high;
    if (low >= 0 && width < low) width = low;
    return width < 0 ? 0 : width > WE_LAYOUT_MAX ? WE_LAYOUT_MAX : width;
}

static int constrained_height(const we_node_t *n, int height) {
    if (n->max_height >= 0 && height > n->max_height) height = n->max_height;
    if (n->min_height >= 0 && height < n->min_height) height = n->min_height;
    return height < 0 ? 0 : height > WE_LAYOUT_MAX ? WE_LAYOUT_MAX : height;
}

static int preferred_outer(const we_node_t *n, int containing, int fallback) {
    int width = n->width_pct > 0 ? containing * n->width_pct / 100 :
                n->width >= 0 ? n->width : fallback;
    width = constrained_width(n, width, containing);
    return width + node_border_extra(n) + n->margin[1] + n->margin[3];
}

static int layout_assigned(we_doc_t *d, int idx, int x, int y, int outer, int depth);

static void place_float(we_inline_t *f, int idx, int depth) {
    we_doc_t *d = f->d;
    we_node_t *n = &d->nodes[idx];
    int width, x, height, slot;
    flush_line(f, 0);
    if (f->nfloat >= WE_FLOATS_MAX) {
        diag(d, "float capacity exceeded", idx);
        return;
    }
    width = preferred_outer(n, f->base_width, f->base_width / 2);
    if (width < WE_CHAR_W) width = WE_CHAR_W;
    if (width > f->base_width) width = f->base_width;
    flow_region(f, width);
    x = n->float_side == WE_FLOAT_RIGHT ? f->x0 + f->width - width : f->x0;
    if (x < f->base_x) x = f->base_x;
    height = layout_assigned(d, idx, x, f->y, width, depth);
    slot = f->nfloat++;
    f->floats[slot].x = x;
    f->floats[slot].y = f->y;
    f->floats[slot].w = width;
    f->floats[slot].h = height > 0 ? height : 0;
    f->floats[slot].side = n->float_side;
    d->perf.floats++;
}

static int layout_children(we_doc_t* d, int idx, int cx, int cy, int cwidth, int depth) {
    /* One context per bounded layout depth, never on the small guest stack. */
    static we_inline_t contexts[WE_DEPTH_MAX + 1];
    we_inline_t *f = &contexts[depth];
    int c, i, bottom;
    w_zero(f, sizeof(*f));
    f->d = d;
    f->base_x = f->x0 = cx;
    f->base_width = f->width = cwidth;
    f->y = limit_coord(cy);
    f->align = d->nodes[idx].align;
    for (c = d->nodes[idx].first; c >= 0; c = d->nodes[c].next) {
        we_node_t *n = &d->nodes[c];
        if (n->display == WE_DISP_NONE) continue;
        if (n->float_side && !n->is_text) {
            place_float(f, c, depth + 1);
        } else if (is_box(n->display)) {
            flush_line(f, 0);
            flow_region(f, WE_CHAR_W);
            f->y = limit_coord(f->y + layout_block(d, c, f->x0, f->y, f->width, depth + 1));
            f->pending_space = 0;
        } else inline_node(f, c, depth + 1);
    }
    flush_line(f, 0);
    bottom = f->y;
    for (i = 0; i < f->nfloat; i++) {
        int end = f->floats[i].y + f->floats[i].h;
        if (end > bottom) bottom = end;
    }
    return limit_coord(bottom - cy);
}

static int layout_table(we_doc_t *d, int idx, int x, int y, int width, int depth);

static void relative_shift(we_doc_t *d, int idx, int first_item, int containing_x, int containing_w) {
    we_node_t *n = &d->nodes[idx];
    const we_node_t *parent = n->parent >= 0 ? &d->nodes[n->parent] : 0;
    int dx = n->offset_x, dy = n->offset_y;
    int low = containing_x - n->x, high = containing_x + containing_w - n->x - n->w;
    uint32_t i;
    if (!n->relative) return;
    if (high < low) dx = 0;
    else { if (dx < low) dx = low; if (dx > high) dx = high; }
    if (parent && parent->height >= 0) {
        int top = parent->y + parent->border + parent->padding[0];
        low = top - n->y;
        high = top + parent->height - n->y - n->h;
        if (high < low) dy = 0;
        else { if (dy < low) dy = low; if (dy > high) dy = high; }
    } else dy = 0;
    for (i = (uint32_t)idx; i < d->nnodes; i++) {
        int ancestor = (int)i;
        while (ancestor >= 0 && ancestor != idx) ancestor = d->nodes[ancestor].parent;
        if (ancestor == idx) {
            d->nodes[i].x = (int16_t)limit_coord(d->nodes[i].x + dx);
            d->nodes[i].y = (int16_t)limit_coord(d->nodes[i].y + dy);
        }
    }
    for (i = (uint32_t)first_item; i < d->nitems; i++) {
        d->items[i].x = (int16_t)limit_coord(d->items[i].x + dx);
        d->items[i].y = (int16_t)limit_coord(d->items[i].y + dy);
    }
}

static int layout_block(we_doc_t* d, int idx, int x, int y, int cw, int depth) {
    we_node_t* n = &d->nodes[idx];
    int bx, by, bw, bh, content_x, content_w, content_h, slot, border_slot = -1;
    int first_item = (int)d->nitems;
    if (depth > WE_DEPTH_MAX) return 0;
    d->perf.boxes++;
    bx = x + n->margin[3];
    by = y + n->margin[0];
    if (n->width_pct > 0) bw = (cw * n->width_pct) / 100 + node_border_extra(n);
    else if (n->width >= 0) bw = n->width + 2 * n->border + n->padding[1] + n->padding[3];
    else bw = cw - n->margin[1] - n->margin[3];
    bw = constrained_width(n, bw - node_border_extra(n), cw) + node_border_extra(n);
    if (bw < 0) bw = 0;
    if (bw > WE_LAYOUT_MAX) bw = WE_LAYOUT_MAX;
    bx = limit_coord(bx);
    by = limit_coord(by);
    n->x = (int16_t)bx; n->y = (int16_t)by; n->w = (int16_t)bw;
    /* reserve the background item now so it paints under the children */
    slot = n->has_bg ? (int)d->nitems : -1;
    if (n->has_bg && !emit(d, OS_FB_WEB_RECT, 0, 0, 0, 0, n->bg)) slot = -1;
    content_x = limit_coord(bx + n->border + n->padding[3]);
    content_w = bw - 2 * n->border - n->padding[1] - n->padding[3];
    if (content_w < WE_CHAR_W) content_w = WE_CHAR_W;
    if (w_eq(n->tag, "img")) {
        int image_h = image_paint(d, idx, content_x, by + n->border + n->padding[0], content_w);
        if (image_h >= 0) {
            content_h = image_h;
            bw = n->w + node_border_extra(n);
        } else {
            os_fb_web_item_t *alt = emit(d, OS_FB_WEB_TEXT, content_x,
                by + n->border + n->padding[0] + 1, n->text_len * WE_CHAR_W, 8, n->color);
            if (alt) w_cpy(alt->text, OS_FB_WEB_TEXT_LEN, d->pool + n->text_off, n->text_len);
            content_h = WE_LINE_H;
        }
    } else if (n->display == WE_DISP_FLEX || n->display == WE_DISP_GRID)
        content_h = layout_flex(d, idx, content_x, by + n->border + n->padding[0], content_w, depth);
    else if (n->display == WE_DISP_TABLE)
        content_h = layout_table(d, idx, content_x, by + n->border + n->padding[0], content_w, depth);
    else
        content_h = layout_children(d, idx, content_x, by + n->border + n->padding[0], content_w, depth);
    if (n->height >= 0) content_h = n->height;
    content_h = constrained_height(n, content_h);
    bh = limit_coord(content_h + n->padding[0] + n->padding[2] + 2 * n->border);
    if (bh < 0) bh = 0;
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
    if (w_eq(n->tag, "li") && n->list_style != WE_LIST_NONE) {
        char marker[16];
        int number = 1, sibling, len;
        os_fb_web_item_t *it;
        if (n->parent >= 0) for (sibling = d->nodes[n->parent].first; sibling >= 0 && sibling != idx;
                                sibling = d->nodes[sibling].next)
            if (w_eq(d->nodes[sibling].tag, "li") && d->nodes[sibling].display != WE_DISP_NONE) number++;
        if (n->list_style == WE_LIST_DECIMAL) {
            w_itoa(number, marker);
            len = w_len(marker);
            marker[len++] = '.';
            marker[len] = 0;
        } else { marker[0] = n->list_style == WE_LIST_SQUARE ? '#' : '*'; marker[1] = 0; len = 1; }
        it = emit(d, OS_FB_WEB_TEXT, bx - (len + 1) * WE_CHAR_W, by + 1, len * WE_CHAR_W, 8, n->color);
        if (it) w_cpy(it->text, OS_FB_WEB_TEXT_LEN, marker, len);
    }
    relative_shift(d, idx, first_item, x, cw);
    return n->margin[0] + bh + n->margin[2];
}

static int flex_item(const we_node_t* n) { return !n->is_text && n->display != WE_DISP_NONE; }

static int layout_assigned(we_doc_t *d, int idx, int x, int y, int outer, int depth) {
    we_node_t *n = &d->nodes[idx];
    int16_t width = n->width, pct = n->width_pct;
    int16_t low = n->min_width, high = n->max_width;
    int16_t lp = n->min_pct, hp = n->max_pct;
    int h;
    n->width = (int16_t)(outer - node_border_extra(n) - n->margin[1] - n->margin[3]);
    if (n->width < 0) n->width = 0;
    n->width_pct = 0;
    n->min_width = n->max_width = -1;
    n->min_pct = n->max_pct = 0;
    h = layout_block(d, idx, x, y, outer, depth);
    n->width = width;
    n->width_pct = pct;
    n->min_width = low;
    n->max_width = high;
    n->min_pct = lp;
    n->max_pct = hp;
    return h;
}

typedef struct {
    int node[WE_MAX_NODES];
    int size[WE_MAX_NODES];
    uint32_t shrink_weight[WE_MAX_NODES];
} we_track_work_t;
static we_track_work_t track_work[WE_DEPTH_MAX + 1];

static int collect_items(we_doc_t *d, int idx, we_track_work_t *work) {
    int c, count = 0;
    for (c = d->nodes[idx].first; c >= 0; c = d->nodes[c].next)
        if (flex_item(&d->nodes[c])) work->node[count++] = c;
    return count;
}

static int grid_row_height(const we_node_t *n, int row, int rows, int natural, int gap) {
    int i, fixed = 0, fractions = 0, free;
    if (row >= n->grid_rows) return natural;
    if (n->row_size[row] >= 0) return n->row_size[row];
    if (n->height < 0) return natural;
    for (i = 0; i < n->grid_rows; i++) {
        if (n->row_size[i] >= 0) fixed += n->row_size[i];
        else fractions++;
    }
    free = n->height - fixed - (rows > 0 ? rows - 1 : 0) * gap;
    return free > 0 && fractions ? free / fractions : 0;
}

static int layout_grid(we_doc_t *d, int idx, int cx, int cy, int cw, int depth) {
    we_node_t *n = &d->nodes[idx];
    we_track_work_t *work = &track_work[depth];
    int count = collect_items(d, idx, work), cols = n->grid_cols ? n->grid_cols : 1;
    int gap = n->gap, row, rows, y = cy, k = 0, inner;
    if (cols > WE_TRACKS_MAX) cols = WE_TRACKS_MAX;
    rows = (count + cols - 1) / cols;
    if (n->grid_rows > rows) rows = n->grid_rows;
    inner = cw - (cols - 1) * gap;
    if (inner < 0) inner = 0;
    for (row = 0; row < rows; row++) {
        int col, natural = 0;
        for (col = 0; col < cols && k < count; col++, k++) {
            int width = inner / cols + (col < inner % cols);
            int x = cx + col * (inner / cols + gap) + (col < inner % cols ? col : inner % cols);
            int h = layout_block(d, work->node[k], x, y, width, depth + 1);
            if (h > natural) natural = h;
        }
        y = limit_coord(y + grid_row_height(n, row, rows, natural, gap));
        if (row + 1 < rows) y = limit_coord(y + gap);
        d->perf.grid_lines++;
    }
    return y - cy;
}

static int flex_extra(const we_node_t *n, int vertical) {
    return vertical ? 2 * n->border + n->padding[0] + n->padding[2] + n->margin[0] + n->margin[2] :
                      node_border_extra(n) + n->margin[1] + n->margin[3];
}

static int grow_limit(const we_node_t *n, int containing, int vertical) {
    int limit = vertical ? n->max_height : n->max_pct > 0 ? containing * n->max_pct / 100 : n->max_width;
    int low = vertical ? n->min_height : n->min_pct > 0 ? containing * n->min_pct / 100 : n->min_width;
    if (limit < 0) return WE_LAYOUT_MAX;
    if (low > limit) limit = low;
    return limit + flex_extra(n, vertical);
}

/* Iterative water filling: a capped grow item gives its unused share to
 * the other positive growers. Integer remainder goes in source order. */
static void grow_line(we_doc_t *d, we_track_work_t *work, int first, int end, int free, int containing, int vertical) {
    int pass;
    for (pass = 0; free > 0 && pass <= WE_MAX_NODES; pass++) {
        int k, weight = 0, given = 0;
        for (k = first; k < end; k++) {
            const we_node_t *n = &d->nodes[work->node[k]];
            if (n->flex_grow > 0 && work->size[k] < grow_limit(n, containing, vertical)) weight += n->flex_grow;
        }
        if (!weight) break;
        for (k = first; k < end; k++) {
            const we_node_t *n = &d->nodes[work->node[k]];
            int room = grow_limit(n, containing, vertical) - work->size[k], take;
            if (n->flex_grow <= 0 || room <= 0) continue;
            take = free * n->flex_grow / weight;
            if (take > room) take = room;
            work->size[k] += take;
            given += take;
        }
        free -= given;
        if (!given) {
            for (k = first; k < end && free; k++) {
                const we_node_t *n = &d->nodes[work->node[k]];
                if (n->flex_grow > 0 && work->size[k] < grow_limit(n, containing, vertical)) {
                    work->size[k]++;
                    free--;
                }
            }
        }
    }
}

/* Exact product/division without a libgcc 64-bit division dependency.
 * b <= denominator, so the quotient fits in a and therefore uint32_t. */
static uint32_t flex_share(uint32_t a, uint32_t b, uint32_t denominator) {
    uint64_t product = (uint64_t)a * b, remainder = 0;
    uint32_t result = 0;
    int bit;
    if (!denominator) return 0;
    for (bit = 63; bit >= 0; bit--) {
        remainder = (remainder << 1) | ((product >> bit) & 1U);
        if (remainder >= denominator) {
            remainder -= denominator;
            if (bit < 32) result |= 1U << bit;
        }
    }
    return result;
}

static int shrink_floor(const we_node_t *n, int containing, int vertical) {
    int minimum = vertical ? n->min_height : n->min_pct > 0 ? containing * n->min_pct / 100 : n->min_width;
    if (minimum < 0) minimum = 0;
    if (minimum > WE_LAYOUT_MAX) minimum = WE_LAYOUT_MAX;
    minimum += flex_extra(n, vertical);
    return minimum > 0 ? minimum : 0;
}

/* Freeze an item at its minimum, then redistribute the remaining deficit
 * using the original scaled bases. At most one pass per node plus rounding. */
static void shrink_line(we_doc_t *d, we_track_work_t *work, int first, int end,
                        int deficit, int containing, int vertical) {
    int pass;
    for (pass = 0; deficit > 0 && pass <= WE_MAX_NODES; pass++) {
        uint32_t weight = 0;
        int k, taken = 0;
        for (k = first; k < end; k++) {
            const we_node_t *n = &d->nodes[work->node[k]];
            if (work->size[k] > shrink_floor(n, containing, vertical)) weight += work->shrink_weight[k];
        }
        if (!weight) break;
        for (k = first; k < end; k++) {
            const we_node_t *n = &d->nodes[work->node[k]];
            int room = work->size[k] - shrink_floor(n, containing, vertical), take;
            if (room <= 0 || !work->shrink_weight[k]) continue;
            take = (int)flex_share((uint32_t)deficit, work->shrink_weight[k], weight);
            if (take > room) take = room;
            work->size[k] -= take;
            taken += take;
        }
        deficit -= taken;
        if (!taken) {
            for (k = first; k < end && deficit; k++) {
                const we_node_t *n = &d->nodes[work->node[k]];
                if (work->shrink_weight[k] && work->size[k] > shrink_floor(n, containing, vertical)) {
                    work->size[k]--;
                    deficit--;
                }
            }
        }
    }
    if (deficit) diag(d, "flex minimum prevents fit", -1);
}

static int flex_base(const we_node_t *n, int containing, int fallback, int vertical) {
    int basis;
    if (n->basis_pct > 0) basis = containing * n->basis_pct / 100;
    else if (n->flex_basis >= 0) basis = n->flex_basis;
    else if (vertical) basis = n->height >= 0 ? n->height : fallback;
    else if (n->width_pct > 0) basis = containing * n->width_pct / 100;
    else basis = n->width >= 0 ? n->width : fallback;
    if (basis < 0) basis = 0;
    if (basis > WE_LAYOUT_MAX) basis = WE_LAYOUT_MAX;
    return basis;
}

/* Sized columns use the same bounded tracks and grow/shrink solver as rows.
 * Auto content is not measured: an undeclared auto height starts at zero. */
static int layout_sized_column(we_doc_t *d, int idx, int cx, int cy, int cw, int depth, int count) {
    we_node_t *n = &d->nodes[idx];
    we_track_work_t *work = &track_work[depth];
    int main = n->height >= 0 ? constrained_height(n, n->height) : -1;
    int wrap = n->flex_wrap && main >= 0, gap = n->gap > 0 ? n->gap : 0;
    int k, x = cx, extent = 0;
    if (n->flex_wrap && main < 0) diag(d, "column wrap requires height", idx);
    for (k = 0; k < count; k++) {
        const we_node_t *child = &d->nodes[work->node[k]];
        int basis, factor = child->flex_shrink >= 0 ? child->flex_shrink : 1;
        if (child->basis_pct > 0 && main < 0) {
            diag(d, "column percent requires height", work->node[k]);
            basis = child->height >= 0 ? child->height : 0;
        } else basis = flex_base(child, main, 0, 1);
        work->size[k] = constrained_height(child, basis) + flex_extra(child, 1);
        if (work->size[k] < 0) work->size[k] = 0;
        work->shrink_weight[k] = (uint32_t)basis * (uint32_t)factor;
    }
    k = 0;
    while (k < count) {
        int first = k, used = 0, col_width = 0, y = cy, j;
        while (k < count) {
            int next = used + (k > first ? gap : 0) + work->size[k];
            if (wrap && k > first && next > main) break;
            used = next;
            k++;
        }
        if (main >= 0) {
            if (used > main) shrink_line(d, work, first, k, used - main, main, 1);
            else grow_line(d, work, first, k, main - used, main, 1);
        }
        for (j = first; j < k; j++) {
            we_node_t *child = &d->nodes[work->node[j]];
            int width = wrap ? preferred_outer(child, cw, WE_CHAR_W) : cw;
            int16_t saved = child->height, low = child->min_height, high = child->max_height;
            int height = work->size[j] - flex_extra(child, 1), actual;
            if (wrap && (x - cx + width > cw || width < 0)) {
                diag(d, "column wrap width exceeded", work->node[j]);
                return extent;
            }
            child->height = (int16_t)(height > 0 ? height : 0);
            child->min_height = child->max_height = -1;
            actual = wrap ? layout_assigned(d, work->node[j], x, y, width, depth + 1) :
                            layout_block(d, work->node[j], x, y, width, depth + 1);
            child->height = saved;
            child->min_height = low;
            child->max_height = high;
            if (width > col_width) col_width = width;
            y = limit_coord(y + actual);
            if (y - cy > extent) extent = y - cy;
            if (j + 1 < k) y = limit_coord(y + gap);
        }
        d->perf.flex_lines++;
        if (k < count) x = limit_coord(x + col_width + gap);
    }
    return extent;
}

static int layout_flex(we_doc_t* d, int idx, int cx, int cy, int cwidth, int depth) {
    we_node_t *n = &d->nodes[idx];
    we_track_work_t *work = &track_work[depth];
    int count, k, gap = n->gap > 0 ? n->gap : 0, y = cy, explicit_grow = 0, sizing = 0;
    if (depth > WE_DEPTH_MAX) return 0;
    if (n->display == WE_DISP_GRID) return layout_grid(d, idx, cx, cy, cwidth, depth);
    count = collect_items(d, idx, work);
    if (!count) return 0;
    if (n->flex_dir && (n->min_height >= 0 || n->max_height >= 0)) sizing = 1;
    for (k = 0; k < count; k++) {
        const we_node_t *child = &d->nodes[work->node[k]];
        if (child->basis_set || child->flex_shrink >= 0 ||
            (n->flex_dir && (child->min_height >= 0 || child->max_height >= 0))) sizing = 1;
    }
    if (n->flex_dir && sizing) return layout_sized_column(d, idx, cx, cy, cwidth, depth, count);
    if (n->flex_dir) {
        /* The original column path remains the vertical stacking model.
         * Grow distributes definite-height free space on its main axis. */
        if (n->flex_wrap && n->height >= 0) {
            int col_start = 0, col_y = cy, col_x = cx, col_w = 0, extent = 0;
            for (k = 0; k < count; k++) {
                const we_node_t *child = &d->nodes[work->node[k]];
                int h = child->height >= 0 ? child->height + child->margin[0] + child->margin[2] +
                        child->padding[0] + child->padding[2] + 2 * child->border : WE_LINE_H;
                int width = preferred_outer(child, cwidth, WE_CHAR_W), actual;
                if (k > col_start && col_y - cy + h > n->height) {
                    col_x += col_w + gap;
                    col_y = cy;
                    col_w = 0;
                    col_start = k;
                    d->perf.flex_lines++;
                }
                if (col_x + width > cx + cwidth) {
                    diag(d, "column wrap width exceeded", work->node[k]);
                    break;
                }
                actual = layout_assigned(d, work->node[k], col_x, col_y, width, depth + 1);
                if (width > col_w) col_w = width;
                if (col_y - cy + actual > extent) extent = col_y - cy + actual;
                col_y += actual + gap;
            }
            if (k) d->perf.flex_lines++;
            return extent;
        }
        if (n->flex_wrap) diag(d, "column wrap requires height", idx);
        int used = (count - 1) * gap, weight = 0, free;
        for (k = 0; k < count; k++) {
            we_node_t *child = &d->nodes[work->node[k]];
            used += child->height >= 0 ? child->height + child->margin[0] + child->margin[2] +
                    child->padding[0] + child->padding[2] + 2 * child->border : 0;
            if (child->flex_grow > 0) weight += child->flex_grow;
        }
        free = n->height > used ? n->height - used : 0;
        for (k = 0; k < count; k++) {
            we_node_t *child = &d->nodes[work->node[k]];
            int16_t saved = child->height;
            if (free && weight && child->flex_grow > 0)
                child->height = (int16_t)((saved >= 0 ? saved : 0) + free * child->flex_grow / weight);
            y += layout_block(d, work->node[k], cx, y, cwidth, depth + 1);
            child->height = saved;
            if (k + 1 < count) y += gap;
        }
        d->perf.flex_lines++;
        return y - cy;
    }
    for (k = 0; k < count; k++) if (d->nodes[work->node[k]].flex_grow >= 0) explicit_grow = 1;
    if (!explicit_grow && !n->flex_wrap && !sizing) {
        int available = cwidth - (count - 1) * gap;
        int each, x = cx, high = 0;
        if (available < 0) available = 0;
        each = available / count;
        if (each < WE_CHAR_W) each = WE_CHAR_W;
        for (k = 0; k < count; k++) {
            int h = layout_block(d, work->node[k], x, cy, each, depth + 1);
            if (h > high) high = h;
            x += each + gap;
        }
        d->perf.flex_lines++;
        return high;
    }
    for (k = 0; k < count; k++) {
        const we_node_t *child = &d->nodes[work->node[k]];
        int basis = flex_base(child, cwidth, n->flex_wrap ? WE_CHAR_W : 0, 0);
        int factor = child->flex_shrink >= 0 ? child->flex_shrink : sizing ? 1 : 0;
        work->size[k] = constrained_width(child, basis, cwidth) + node_border_extra(child) +
                         child->margin[1] + child->margin[3];
        if (work->size[k] < 0) work->size[k] = 0;
        work->shrink_weight[k] = (uint32_t)basis * (uint32_t)factor;
    }
    k = 0;
    while (k < count) {
        int first = k, used = 0, x = cx, high = 0, j;
        while (k < count) {
            int next = used + (k > first ? gap : 0) + work->size[k];
            if (n->flex_wrap && k > first && next > cwidth) break;
            used = next;
            k++;
        }
        if (used > cwidth && sizing) shrink_line(d, work, first, k, used - cwidth, cwidth, 0);
        else grow_line(d, work, first, k, cwidth > used ? cwidth - used : 0, cwidth, 0);
        for (j = first; j < k; j++) {
            int h = layout_assigned(d, work->node[j], x, y, work->size[j], depth + 1);
            if (h > high) high = h;
            x += work->size[j] + gap;
        }
        y = limit_coord(y + high);
        if (k < count) y = limit_coord(y + gap);
        d->perf.flex_lines++;
    }
    return y - cy;
}

/* Table rows share one bounded set of column widths. Spans other than 1
 * are refused by the parser and refuse the table as a whole. No silent
 * reinterpretation as a one-cell span and no independent table engine. */
static int layout_table(we_doc_t *d, int idx, int x, int y, int width, int depth) {
    int *rows = track_work[depth].node;
    int nr = 0, c, r, cols = 0, col, cy = y;
    int widths[WE_TRACKS_MAX], requested[WE_TRACKS_MAX], sum = 0, auto_cols = 0;
    /* Row indexes are small; all pixel and decoder buffers remain static. */
    for (c = d->nodes[idx].first; c >= 0; c = d->nodes[c].next) {
        if (d->nodes[c].display == WE_DISP_NONE) continue;
        if (d->nodes[c].display == WE_DISP_ROW) rows[nr++] = c;
        else if (w_eq(d->nodes[c].tag, "tbody") || w_eq(d->nodes[c].tag, "thead") || w_eq(d->nodes[c].tag, "tfoot")) {
            int row;
            for (row = d->nodes[c].first; row >= 0; row = d->nodes[row].next)
                if (d->nodes[row].display == WE_DISP_ROW) rows[nr++] = row;
        } else diag(d, "table child refused", c);
    }
    for (col = 0; col < WE_TRACKS_MAX; col++) requested[col] = 0;
    for (r = 0; r < nr; r++) {
        int cells = 0;
        for (c = d->nodes[rows[r]].first; c >= 0; c = d->nodes[c].next) {
            we_node_t *cell = &d->nodes[c];
            int want;
            if (cell->display == WE_DISP_NONE) continue;
            if (cell->display != WE_DISP_CELL) { diag(d, "table cell refused", c); continue; }
            if (cell->span_refused) { diag(d, "table refused after span", idx); return 0; }
            if (cells >= WE_TRACKS_MAX) { diag(d, "table columns exceed limit", idx); return 0; }
            want = preferred_outer(cell, width, 0);
            if (cell->width >= 0 || cell->width_pct > 0 || cell->min_width >= 0 || cell->min_pct > 0)
                if (want > requested[cells]) requested[cells] = want;
            cells++;
        }
        if (cells > cols) cols = cells;
    }
    if (!cols && !nr) return 0;
    if (!cols) cols = 1;
    for (col = 0; col < cols; col++) {
        sum += requested[col];
        if (!requested[col]) auto_cols++;
    }
    for (col = 0; col < cols; col++) {
        if (sum > width) widths[col] = width * requested[col] / sum;
        else if (!requested[col]) widths[col] = auto_cols ? (width - sum) / auto_cols : 0;
        else widths[col] = requested[col];
    }
    /* A definite table width is exhausted exactly. Extra width goes to
     * automatic columns, or equally to all columns when all are fixed. */
    {
        int assigned = 0, remaining, receivers;
        for (col = 0; col < cols; col++) assigned += widths[col];
        remaining = width - assigned;
        receivers = sum > width ? cols : auto_cols ? auto_cols : cols;
        for (col = 0; col < cols && remaining > 0; col++) {
            int take;
            if (sum <= width && auto_cols && requested[col]) continue;
            take = remaining / receivers + (remaining % receivers != 0);
            widths[col] += take;
            remaining -= take;
            receivers--;
        }
    }
    for (r = 0; r < nr; r++) {
        we_node_t *row = &d->nodes[rows[r]];
        int cx = x, high = 0, bg_slot = -1;
        if (row->has_bg) {
            bg_slot = (int)d->nitems;
            if (!emit(d, OS_FB_WEB_RECT, x, cy, width, 0, row->bg)) bg_slot = -1;
        }
        col = 0;
        row->x = (int16_t)x;
        row->y = (int16_t)cy;
        row->w = (int16_t)width;
        for (c = row->first; c >= 0; c = d->nodes[c].next) {
            int h;
            if (d->nodes[c].display != WE_DISP_CELL) continue;
            h = layout_assigned(d, c, cx, cy, widths[col], depth + 1);
            if (h > high) high = h;
            cx += widths[col++];
        }
        if (row->height > high) high = row->height;
        row->h = (int16_t)high;
        if (bg_slot >= 0) d->items[bg_slot].h = (uint16_t)high;
        cy = limit_coord(cy + high);
        d->perf.table_rows++;
    }
    return cy - y;
}

int we_layout(we_doc_t* d, int viewport_w) {
    int h;
    if (!d || d->nnodes == 0 || viewport_w < 64) return -1;
    d->nitems = 0;
    d->perf.boxes = d->perf.lines = d->perf.items_dropped = 0;
    d->perf.flex_lines = d->perf.grid_lines = d->perf.floats = 0;
    d->perf.table_rows = d->perf.images = d->perf.image_refused = 0;
    { uint32_t i; for (i = 0; i < d->nnodes; i++)
        d->nodes[i].x = d->nodes[i].y = d->nodes[i].w = d->nodes[i].h = 0; }
    if (viewport_w > WE_LAYOUT_MAX) viewport_w = WE_LAYOUT_MAX;
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
        out(ctx, n->display == WE_DISP_NONE ? " none" : n->display == WE_DISP_BLOCK ? " block"
            : n->display == WE_DISP_FLEX ? " flex" : n->display == WE_DISP_GRID ? " grid"
            : n->display == WE_DISP_TABLE ? " table" : n->display == WE_DISP_ROW ? " row"
            : n->display == WE_DISP_CELL ? " cell" : " inline");
        out(ctx, "\n");
    }
}

void we_inspect(const we_doc_t* d, int node, we_out_fn out, void* ctx) {
    const we_node_t* n;
    if (node < 0 || (uint32_t)node >= d->nnodes) { out(ctx, "inspect none\n"); return; }
    n = &d->nodes[node];
    out(ctx, "inspect "); out(ctx, n->is_text ? "#text" : n->tag);
    if (n->id[0]) { out(ctx, "#"); out(ctx, n->id); }
    out(ctx, n->display == WE_DISP_NONE ? " display none" : n->display == WE_DISP_BLOCK ? " display block"
        : n->display == WE_DISP_FLEX ? " display flex" : n->display == WE_DISP_GRID ? " display grid"
        : n->display == WE_DISP_TABLE ? " display table" : n->display == WE_DISP_ROW ? " display row"
        : n->display == WE_DISP_CELL ? " display cell" : " display inline");
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
