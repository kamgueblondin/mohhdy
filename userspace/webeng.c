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
    n->colspan = 1;
    if (parent >= 0) {
        we_node_t* p = &d->nodes[parent];
        n->html_space = p->pending_html_space;
        p->pending_html_space = 0;
        if (p->last >= 0) d->nodes[p->last].next = (int16_t)idx;
        else p->first = (int16_t)idx;
        p->last = (int16_t)idx;
    }
    return idx;
}

/* decode entities and collapse spaces of html[a..b) into the pool */
static int add_text(we_doc_t* d, int parent, const char* s, int n) {
    char buf[WE_TEXT_SOURCE_MAX];
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
    if (i < n) diag(d, "text source exceeds limit", parent);
    if (o == 0) { d->nodes[parent].pending_html_space = 1; return 0; }
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
                        int span = 0, digit, valid = vk > 0;
                        for (digit = 0; digit < vk; digit++) {
                            if (av[digit] < '0' || av[digit] > '9' || span > WE_TRACKS_MAX) { valid = 0; break; }
                            span = span * 10 + av[digit] - '0';
                        }
                        if (!valid || span < 1 || span > WE_TRACKS_MAX || (w_eq(an, "rowspan") && span != 1)) {
                            n->span_refused = 1;
                            diag(d, "table span refused", idx);
                        } else if (w_eq(an, "colspan")) n->colspan = (uint8_t)span;
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

static int parse_pack(const char *v, int len, int distribution, int stretch) {
    if (value_is(v, len, "start") || value_is(v, len, "flex-start")) return WE_PACK_START;
    if (value_is(v, len, "end") || value_is(v, len, "flex-end")) return WE_PACK_END;
    if (value_is(v, len, "center")) return WE_PACK_CENTER;
    if (stretch && value_is(v, len, "stretch")) return WE_PACK_STRETCH;
    if (distribution && value_is(v, len, "space-between")) return WE_PACK_BETWEEN;
    if (distribution && value_is(v, len, "space-around")) return WE_PACK_AROUND;
    if (distribution && value_is(v, len, "space-evenly")) return WE_PACK_EVENLY;
    return -1;
}

static int parse_dimension(const char *v, int len, int *number, int *percent) {
    int end = len;
    *percent = len > 0 && v[len - 1] == '%';
    if (*percent) end--;
    else if (len >= 2 && value_is(v + len - 2, 2, "px")) end -= 2;
    return bounded_uint(v, end, 2000, number);
}

static int parse_placement(const char *v, int len, uint8_t *start, uint8_t *span) {
    int i = 0, a, number, position = 0, extent = 1;
    if (value_is(v, len, "auto")) { *start = 0; *span = 1; return 0; }
    if (w_ieq_n(v, len, "span ")) {
        i = 5;
        while (i < len && w_space(v[i])) i++;
        if (bounded_uint(v + i, len - i, WE_TRACKS_MAX, &extent) || extent < 1) return -1;
    } else {
        while (i < len && v[i] >= '0' && v[i] <= '9') i++;
        if (bounded_uint(v, i, WE_TRACKS_MAX, &position) || position < 1) return -1;
        while (i < len && w_space(v[i])) i++;
        if (i < len) {
            if (v[i++] != '/') return -1;
            while (i < len && w_space(v[i])) i++;
            if (w_ieq_n(v + i, len - i, "span ")) {
                i += 5;
                while (i < len && w_space(v[i])) i++;
                if (bounded_uint(v + i, len - i, WE_TRACKS_MAX, &extent) || extent < 1) return -1;
            } else {
                a = i;
                while (i < len && v[i] >= '0' && v[i] <= '9') i++;
                if (bounded_uint(v + a, i - a, WE_TRACKS_MAX + 1, &number) || number <= position) return -1;
                while (i < len && w_space(v[i])) i++;
                if (i != len) return -1;
                extent = number - position;
            }
        }
    }
    if (position && position + extent - 1 > WE_TRACKS_MAX) return -1;
    *start = (uint8_t)position;
    *span = (uint8_t)extent;
    return 0;
}

static int track_token(const char *v, int len, int16_t *size) {
    int n, percent;
    if (value_is(v, len, "auto")) { *size = WE_TRACK_AUTO; return 0; }
    if (len >= 2 && value_is(v + len - 2, 2, "fr")) {
        if (bounded_uint(v, len - 2, 8, &n) || n < 1) return -1;
        *size = (int16_t)-n;
        return 0;
    }
    if (parse_dimension(v, len, &n, &percent) || percent) return -1;
    *size = (int16_t)n;
    return 0;
}

static int parse_tracks(const char *v, int len, int16_t *sizes) {
    int i = 0, count = 0;
    if (w_ieq_n(v, len, "repeat(")) {
        int start = 7, end = start, number;
        int16_t size;
        while (end < len && v[end] >= '0' && v[end] <= '9') end++;
        if (bounded_uint(v + start, end - start, WE_TRACKS_MAX, &number) || number < 1) return -1;
        while (end < len && w_space(v[end])) end++;
        if (end >= len || v[end++] != ',') return -1;
        while (end < len && w_space(v[end])) end++;
        start = end;
        while (end < len && v[end] != ')' && !w_space(v[end])) end++;
        if (track_token(v + start, end - start, &size)) return -1;
        while (end < len && w_space(v[end])) end++;
        if (end >= len || v[end++] != ')') return -1;
        while (end < len && w_space(v[end])) end++;
        if (end != len) return -1;
        for (i = 0; i < number; i++) sizes[i] = size;
        return number;
    }
    while (i < len) {
        int first;
        while (i < len && w_space(v[i])) i++;
        if (i == len) break;
        first = i;
        while (i < len && !w_space(v[i])) i++;
        if (count >= WE_TRACKS_MAX || track_token(v + first, i - first, &sizes[count])) return -1;
        count++;
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
        if (value_is(v, vl, "content")) { n->basis_content = 1; n->basis_set = 1; n->flex_basis = -1; n->basis_pct = 0; }
        else if (value_is(v, vl, "auto")) {
            n->basis_content = 0; n->flex_basis = -1; n->basis_pct = 0; n->basis_set = 1;
        } else {
            int percent = vl > 0 && v[vl - 1] == '%';
            if (percent) end--;
            else if (vl >= 2 && value_is(v + vl - 2, 2, "px")) end -= 2;
            if (bounded_uint(v, end, 2000, &value) == 0) {
                n->basis_content = 0;
                n->flex_basis = (int16_t)(percent ? -1 : value);
                n->basis_pct = (int16_t)(percent ? value : 0);
                /* Percent zero is a definite zero, not auto. */
                if (percent && value == 0) n->flex_basis = 0;
                n->basis_set = 1;
            } else diag(d, "flex-basis refused", -1);
        }
    } else if (w_eq(prop, "justify-content") || w_eq(prop, "align-content") ||
               w_eq(prop, "align-items") || w_eq(prop, "align-self") ||
               w_eq(prop, "justify-items") || w_eq(prop, "justify-self")) {
        int distribution = w_eq(prop, "justify-content") || w_eq(prop, "align-content");
        int value = parse_pack(v, vl, distribution, !w_eq(prop, "justify-content"));
        if ((w_eq(prop, "align-self") || w_eq(prop, "justify-self")) && value_is(v, vl, "auto")) value = 0;
        if (value < 0) diag(d, "alignment refused", -1);
        else if (w_eq(prop, "justify-content")) n->justify_content = (uint8_t)value;
        else if (w_eq(prop, "align-content")) n->align_content = (uint8_t)value;
        else if (w_eq(prop, "align-items")) n->align_items = (uint8_t)value;
        else if (w_eq(prop, "align-self")) n->align_self = (uint8_t)value;
        else if (w_eq(prop, "justify-items")) n->justify_items = (uint8_t)value;
        else n->justify_self = (uint8_t)value;
    } else if (w_eq(prop, "row-gap") || w_eq(prop, "column-gap")) {
        int number, percent;
        if (parse_dimension(v, vl, &number, &percent) || percent) diag(d, "axis gap refused", -1);
        else if (w_eq(prop, "row-gap")) n->row_gap = (int16_t)number;
        else n->col_gap = (int16_t)number;
    } else if (w_eq(prop, "grid-column") || w_eq(prop, "grid-row")) {
        uint8_t start, span;
        if (parse_placement(v, vl, &start, &span)) diag(d, "grid placement refused", -1);
        else if (w_eq(prop, "grid-column")) { n->grid_col_start = start; n->grid_col_span = span; }
        else { n->grid_row_start = start; n->grid_row_span = span; }
    } else if (w_eq(prop, "box-sizing")) {
        if (value_is(v, vl, "border-box")) n->border_box = 1;
        else if (value_is(v, vl, "content-box")) n->border_box = 0;
        else diag(d, "box sizing refused", -1);
    } else if (w_eq(prop, "white-space")) {
        if (value_is(v, vl, "normal") || value_is(v, vl, "nowrap")) n->nowrap = (uint8_t)value_is(v, vl, "nowrap");
        else diag(d, "white space refused", -1);
    } else if (w_eq(prop, "overflow-wrap")) {
        if (value_is(v, vl, "normal") || value_is(v, vl, "break-word")) n->break_word = (uint8_t)value_is(v, vl, "break-word");
        else diag(d, "word wrapping refused", -1);
    } else if (w_eq(prop, "overflow")) {
        if (value_is(v, vl, "visible") || value_is(v, vl, "hidden")) n->overflow_hidden = (uint8_t)value_is(v, vl, "hidden");
        else diag(d, "overflow refused", -1);
    } else if (w_eq(prop, "visibility")) {
        if (value_is(v, vl, "visible") || value_is(v, vl, "hidden")) n->visibility = (uint8_t)value_is(v, vl, "hidden");
        else diag(d, "visibility refused", -1);
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
        int a[4], lengths[4], number[2], percent, count = tokens(v, vl, a, lengths), valid = count >= 1 && count <= 2;
        for (k = 0; k < count && k < 2; k++)
            if (parse_dimension(v + a[k], lengths[k], &number[k], &percent) || percent) valid = 0;
        if (!valid) diag(d, "gap refused", -1);
        else {
            n->gap = (int16_t)number[0];
            n->row_gap = count == 2 ? (int16_t)number[0] : -1;
            n->col_gap = count == 2 ? (int16_t)number[1] : -1;
        }
    } else if (w_eq(prop, "grid-template-columns") || w_eq(prop, "grid-template-rows")) {
        int16_t sizes[WE_TRACKS_MAX];
        int count = parse_tracks(v, vl, sizes);
        if (count < 0) diag(d, "grid tracks refused", -1);
        else if (w_eq(prop, "grid-template-columns")) {
            n->grid_cols = (uint8_t)count;
            for (k = 0; k < count; k++) n->col_size[k] = sizes[k];
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
        if (value_is(v, vl, "min-content") || value_is(v, vl, "max-content") || value_is(v, vl, "fit-content")) {
            n->width_kind = (uint8_t)(value_is(v, vl, "min-content") ? WE_SIZE_MIN_CONTENT :
                value_is(v, vl, "max-content") ? WE_SIZE_MAX_CONTENT : WE_SIZE_FIT_CONTENT);
            n->width = -1; n->width_pct = 0;
            return;
        }
        int r = parse_len(v, vl, &px, &pct);
        if (r >= 0) n->width_kind = WE_SIZE_AUTO;
        if (r == 1) { n->width = -1; n->width_pct = 0; }
        else if (r == 0) { if (pct) { n->width_pct = (int16_t)pct; n->width = -1; } else { n->width = (int16_t)px; n->width_pct = 0; } }
    } else if (w_eq(prop, "height")) {
        int number, percent;
        if (value_is(v, vl, "auto")) { n->height = -1; n->height_percent = 0; }
        else if (parse_dimension(v, vl, &number, &percent)) diag(d, "height refused", -1);
        else {
            n->height = percent ? -1 : (int16_t)number;
            n->height_pct = (int16_t)number;
            n->height_percent = (uint8_t)percent;
        }
    } else if (w_eq(prop, "min-height") || w_eq(prop, "max-height")) {
        int number, percent;
        if (parse_dimension(v, vl, &number, &percent)) diag(d, "height limit refused", -1);
        else if (w_eq(prop, "min-height")) {
            n->min_height = percent ? -1 : (int16_t)number;
            n->min_height_pct = (int16_t)number;
            n->min_height_percent = (uint8_t)percent;
        } else {
            n->max_height = percent ? -1 : (int16_t)number;
            n->max_height_pct = (int16_t)number;
            n->max_height_percent = (uint8_t)percent;
        }
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
    if (!d->css_parsed) { parse_css(d); d->css_parsed = 1; }
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
        n->row_gap = n->col_gap = -1;
        n->height_percent = n->min_height_percent = n->max_height_percent = 0;
        n->height_pct = n->min_height_pct = n->max_height_pct = 0;
        n->width_kind = n->basis_content = n->border_box = n->overflow_hidden = 0;
        n->justify_content = n->align_items = n->align_self = n->align_content = 0;
        n->justify_items = n->justify_self = 0;
        n->grid_col_start = n->grid_row_start = 0;
        n->nowrap = p ? p->nowrap : 0;
        n->break_word = p ? p->break_word : 0;
        n->visibility = p ? p->visibility : 0;
        n->grid_col_span = n->grid_row_span = 1;
        { int track; for (track = 0; track < WE_TRACKS_MAX; track++) {
            n->col_size[track] = -1;
            n->row_size[track] = WE_TRACK_AUTO;
        } }
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
static os_fb_web_item_t* emit(we_doc_t* d, int owner, uint8_t kind, int x, int y, int w, int h, uint32_t color) {
    os_fb_web_item_t* it;
    if (owner >= 0 && d->nodes[owner].visibility) return 0;
    if (d->nitems >= OS_FB_WEB_ITEMS) {
        if (!d->perf.items_dropped) diag(d, "display budget exceeded", owner);
        d->perf.items_dropped++;
        return 0;
    }
    d->item_owner[d->nitems] = (int16_t)owner;
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
    int16_t natural_w[WE_MAX_NODES], natural_h[WE_MAX_NODES];
    uint8_t failed[WE_MAX_NODES];
    int ready_idx;
} image_work;
typedef char image_workspace_is_bounded[
    sizeof(image_work) <= WE_IMAGE_WORKSPACE_MAX ? 1 : -1];

void we_set_image_reader(we_image_read_fn read, void *ctx) {
    image_read = read;
    image_ctx = ctx;
    image_work.ready_idx = -1;
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
    if (idx >= 0 && image_work.failed[idx]) return -1;
    if (idx >= 0) image_work.failed[idx] = 1;
    diag(d, message, idx);
    d->perf.image_refused++;
    return -1;
}

static int image_open(we_doc_t *d, int idx) {
    const we_node_t *n = &d->nodes[idx];
    int bytes, rc;
    if (image_work.failed[idx]) return -1;
    if (image_work.ready_idx == idx) return 0;
    image_work.ready_idx = -1;
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
    image_work.ready_idx = idx;
    image_work.natural_w[idx] = (int16_t)image_work.decoded.w;
    image_work.natural_h[idx] = (int16_t)image_work.decoded.h;
    return 0;
}

static int image_measure_height(we_doc_t *d, int idx) {
    const we_node_t *n = &d->nodes[idx];
    if (image_work.failed[idx]) return -1;
    if ((n->height >= 0 && (n->height < 1 || n->height > WE_IMAGE_MAX_H)) ||
        (n->width >= 0 && (n->width < 1 || n->width > WE_IMAGE_MAX_W)))
        return image_fail(d, "image output exceeds limit", idx);
    if (image_work.natural_h[idx] > 0) return image_work.natural_h[idx];
    return image_open(d, idx) ? -1 : image_work.natural_h[idx];
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
        emit(d, idx, OS_FB_WEB_RECT, x + it->x, y + it->y, it->w, it->h, it->color);
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
    int pending_space, measuring;
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
    if (f->measuring) {
        f->y = limit_coord(f->y + WE_LINE_H);
        f->lines++;
        f->nwords = f->cur_w = 0;
        return;
    }
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
        if (j == i) {
            int k;
            for (k = 0; k < f->w[i].len && k < OS_FB_WEB_TEXT_LEN - 1; k++) buf[k] = f->w[i].s[k];
            len = k;
            f->w[i].s += k;
            f->w[i].len -= k;
            f->w[i].space_before = 0;
            j = f->w[i].len ? i : i + 1;
        }
        buf[len] = 0;
        n = &d->nodes[d->nodes[node].parent >= 0 ? d->nodes[node].parent : node];
        if (n->has_bg && n->display == WE_DISP_INLINE) emit(d, node, OS_FB_WEB_RECT, xs, f->y, len * WE_CHAR_W, WE_LINE_H, n->bg);
        it = emit(d, node, OS_FB_WEB_TEXT, xs, f->y + 1, len * WE_CHAR_W, 8, d->nodes[node].color);
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

static void add_word_piece(we_inline_t* f, const char* s, int len, int node, int space) {
    int ww;
    flow_region(f, len * WE_CHAR_W);
    ww = len * WE_CHAR_W + ((f->nwords && space) ? WE_CHAR_W : 0);
    if (!f->d->nodes[node].nowrap && f->nwords && f->cur_w + ww > f->width) { flush_line(f, 0); flow_region(f, len * WE_CHAR_W); ww = len * WE_CHAR_W; }
    if (f->nwords >= WE_LINE_WORDS) {
        if (f->d->nodes[node].nowrap) diag(f->d, "inline word capacity", node);
        flush_line(f, 0);
        ww = len * WE_CHAR_W;
    }
    f->w[f->nwords].s = s; f->w[f->nwords].len = len; f->w[f->nwords].node = node;
    f->w[f->nwords].space_before = space;
    f->nwords++;
    f->cur_w += ww;
}

static void add_word(we_inline_t *f, const char *s, int len, int node, int space) {
    const we_node_t *n = &f->d->nodes[node];
    if (!n->break_word || n->nowrap || len * WE_CHAR_W <= f->base_width) {
        add_word_piece(f, s, len, node, space);
        return;
    }
    if (f->nwords) { flush_line(f, 0); space = 0; }
    while (len > 0) {
        int room, take;
        flow_region(f, WE_CHAR_W);
        room = f->width - f->cur_w - (f->nwords && space ? WE_CHAR_W : 0);
        if (room < WE_CHAR_W) {
            flush_line(f, 0);
            flow_region(f, WE_CHAR_W);
            room = f->width;
            space = 0;
        }
        take = room / WE_CHAR_W;
        if (take < 1) take = 1;
        if (take > len) take = len;
        add_word_piece(f, s, take, node, space);
        s += take;
        len -= take;
        space = 0;
        if (len) flush_line(f, 0);
    }
}

static void place_float(we_inline_t *f, int idx, int depth);

static void inline_node(we_inline_t* f, int idx, int depth) {
    we_doc_t* d = f->d;
    we_node_t* n = &d->nodes[idx];
    int c;
    if (n->html_space) f->pending_space = 1;
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
        if (f->measuring) {
            h = image_measure_height(d, idx);
            if (h >= 0 && n->height >= 0) h = n->height;
        } else h = image_paint(d, idx, f->x0, f->y, f->width);
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
    if (n->border_box) {
        if (low >= 0) { low -= node_border_extra(n); if (low < 0) low = 0; }
        if (high >= 0) { high -= node_border_extra(n); if (high < 0) high = 0; }
    }
    if (high >= 0 && width > high) width = high;
    if (low >= 0 && width < low) width = low;
    return width < 0 ? 0 : width > WE_LAYOUT_MAX ? WE_LAYOUT_MAX : width;
}

static int16_t forced_width[WE_MAX_NODES], forced_height[WE_MAX_NODES];
static int16_t resolved_height[WE_MAX_NODES], containing_height[WE_MAX_NODES];
static int measure_budget;
static uint8_t measure_content[WE_MAX_NODES];
typedef struct {
    int width, fixed, parent, result;
    uint8_t content, valid;
} we_measure_cache_t;
static we_measure_cache_t measure_cache[WE_MAX_NODES];

static int axis_gap(const we_node_t *n, int vertical) {
    int value = vertical ? n->row_gap : n->col_gap;
    return value >= 0 ? value : n->gap > 0 ? n->gap : 0;
}

static int vertical_extra(const we_node_t *n) {
    return 2 * n->border + n->padding[0] + n->padding[2];
}

static int content_value(int value, int extra, int border_box) {
    if (border_box) value -= extra;
    return value < 0 ? 0 : value > WE_LAYOUT_MAX ? WE_LAYOUT_MAX : value;
}

static int specified_width(const we_node_t *n, int containing, int fallback) {
    int value;
    if (n->width_kind == WE_SIZE_MIN_CONTENT) value = n->min_content;
    else if (n->width_kind == WE_SIZE_MAX_CONTENT) value = n->max_content;
    else if (n->width_kind == WE_SIZE_FIT_CONTENT) {
        value = containing - node_border_extra(n) - n->margin[1] - n->margin[3];
        if (value > n->max_content) value = n->max_content;
        if (value < n->min_content) value = n->min_content;
    } else {
        value = n->width_pct > 0 ? containing * n->width_pct / 100 :
                n->width >= 0 ? n->width : fallback;
        if (n->width >= 0 || n->width_pct > 0) value = content_value(value, node_border_extra(n), n->border_box);
    }
    return constrained_width(n, value, containing);
}

typedef struct {
    int line, word, minimum, maximum, pending;
} we_intrinsic_flow_t;

static void intrinsic_word(we_intrinsic_flow_t *flow) {
    if (flow->word > flow->minimum) flow->minimum = flow->word;
    flow->word = 0;
}

static void intrinsic_line(we_intrinsic_flow_t *flow) {
    intrinsic_word(flow);
    if (flow->line > flow->maximum) flow->maximum = flow->line;
    flow->line = 0;
    flow->pending = 0;
}

static void intrinsic_inline(we_doc_t *d, int idx, we_intrinsic_flow_t *flow, int depth) {
    const we_node_t *n = &d->nodes[idx];
    int c, i;
    if (n->html_space) { intrinsic_word(flow); flow->pending = 1; }
    if (depth > WE_DEPTH_MAX || n->display == WE_DISP_NONE) return;
    if (n->is_text) {
        if (n->lead_space) { intrinsic_word(flow); flow->pending = 1; }
        for (i = 0; i < n->text_len; i++) {
            if (d->pool[n->text_off + i] == ' ') {
                intrinsic_word(flow);
                flow->pending = 1;
            } else {
                if (flow->pending && flow->line) flow->line += WE_CHAR_W;
                flow->pending = 0;
                flow->line = limit_coord(flow->line + WE_CHAR_W);
                flow->word = limit_coord(flow->word + WE_CHAR_W);
            }
        }
        if (n->trail_space) { intrinsic_word(flow); flow->pending = 1; }
    } else if (w_eq(n->tag, "br")) intrinsic_line(flow);
    else if (w_eq(n->tag, "img")) {
        int width = n->width >= 0 ? n->width : n->text_len * WE_CHAR_W;
        intrinsic_word(flow);
        if (flow->pending && flow->line) flow->line += WE_CHAR_W;
        flow->pending = 0;
        flow->line = limit_coord(flow->line + width);
        if (width > flow->minimum) flow->minimum = width;
    } else if (is_box(n->display)) {
        int extra = node_border_extra(n) + n->margin[1] + n->margin[3];
        int low = n->min_content, high = n->max_content;
        intrinsic_line(flow);
        if (n->width >= 0) low = high = content_value(n->width, node_border_extra(n), n->border_box);
        low = constrained_width(n, low, 0) + extra;
        high = constrained_width(n, high, 0) + extra;
        if (low > flow->minimum) flow->minimum = low;
        if (high > flow->maximum) flow->maximum = high;
    } else {
        for (c = n->first; c >= 0; c = d->nodes[c].next) intrinsic_inline(d, c, flow, depth + 1);
    }
}

static void intrinsic_track_sizes(we_doc_t *d, int idx, int *minimum, int *maximum);

static void intrinsic_sizes(we_doc_t *d) {
    int i;
    for (i = (int)d->nnodes - 1; i >= 0; i--) {
        we_node_t *n = &d->nodes[i];
        we_intrinsic_flow_t flow;
        int c;
        w_zero(&flow, sizeof(flow));
        if (n->display == WE_DISP_NONE) { n->min_content = n->max_content = 0; continue; }
        if (n->is_text) intrinsic_inline(d, i, &flow, 0);
        else if (w_eq(n->tag, "img")) flow.minimum = flow.maximum = n->width >= 0 ? n->width : n->text_len * WE_CHAR_W;
        else if (n->display == WE_DISP_GRID || n->display == WE_DISP_TABLE)
            intrinsic_track_sizes(d, i, &flow.minimum, &flow.maximum);
        else if (n->display == WE_DISP_FLEX && !n->flex_dir) {
            int count = 0, gap = axis_gap(n, 0), low = 0, high = 0;
            for (c = n->first; c >= 0; c = d->nodes[c].next) {
                const we_node_t *child = &d->nodes[c];
                int extra, a, b;
                if (child->display == WE_DISP_NONE || child->is_text) continue;
                extra = node_border_extra(child) + child->margin[1] + child->margin[3];
                a = child->width >= 0 ? content_value(child->width, node_border_extra(child), child->border_box) : child->min_content;
                b = child->width >= 0 ? a : child->max_content;
                a = constrained_width(child, a, 0) + extra;
                b = constrained_width(child, b, 0) + extra;
                if (n->flex_wrap) { if (a > low) low = a; }
                else low = limit_coord(low + a + (count ? gap : 0));
                high = limit_coord(high + b + (count ? gap : 0));
                count++;
            }
            flow.minimum = low;
            flow.maximum = high;
        } else {
            for (c = n->first; c >= 0; c = d->nodes[c].next) intrinsic_inline(d, c, &flow, 0);
        }
        intrinsic_line(&flow);
        n->min_content = (int16_t)limit_coord(n->nowrap ? flow.maximum : flow.minimum);
        n->max_content = (int16_t)limit_coord(flow.maximum);
    }
}

static int definite_height(const we_doc_t *d, int idx, int depth) {
    const we_node_t *n;
    int value, parent, low, high;
    if (idx < 0 || depth > WE_DEPTH_MAX) return -1;
    n = &d->nodes[idx];
    if (forced_height[idx] >= 0)
        return content_value(forced_height[idx] - n->margin[0] - n->margin[2], vertical_extra(n), 1);
    if (resolved_height[idx] >= 0) return resolved_height[idx];
    parent = containing_height[idx] >= 0 ? containing_height[idx] :
             n->height_percent || n->min_height_percent || n->max_height_percent ?
             definite_height(d, n->parent, depth + 1) : -1;
    if (n->height_percent) {
        if (parent < 0) return -1;
        value = content_value(parent * n->height_pct / 100, vertical_extra(n), n->border_box);
    } else if (n->height >= 0) value = content_value(n->height, vertical_extra(n), n->border_box);
    else return -1;
    low = n->min_height_percent ? (parent >= 0 ? parent * n->min_height_pct / 100 : -1) : n->min_height;
    high = n->max_height_percent ? (parent >= 0 ? parent * n->max_height_pct / 100 : -1) : n->max_height;
    if (high >= 0 && value > content_value(high, vertical_extra(n), n->border_box))
        value = content_value(high, vertical_extra(n), n->border_box);
    if (low >= 0 && value < content_value(low, vertical_extra(n), n->border_box))
        value = content_value(low, vertical_extra(n), n->border_box);
    return value;
}

static int height_constraint(const we_doc_t *d, int idx, int value, int parent) {
    const we_node_t *n = &d->nodes[idx];
    int low = n->min_height_percent ? (parent >= 0 ? parent * n->min_height_pct / 100 : -1) : n->min_height;
    int high = n->max_height_percent ? (parent >= 0 ? parent * n->max_height_pct / 100 : -1) : n->max_height;
    if (high >= 0) {
        high = content_value(high, vertical_extra(n), n->border_box);
        if (value > high) value = high;
    }
    if (low >= 0) {
        low = content_value(low, vertical_extra(n), n->border_box);
        if (value < low) value = low;
    }
    return value < 0 ? 0 : value > WE_LAYOUT_MAX ? WE_LAYOUT_MAX : value;
}

static int measure_height(we_doc_t *d, int idx, int width, int depth);
static int measure_area_height(we_doc_t *d, int idx, int outer, int depth);
static int measure_grid(we_doc_t *d, int idx, int width, int depth);
static int measure_table(we_doc_t *d, int idx, int width, int depth);
static int measure_flex(we_doc_t *d, int idx, int width, int depth);
static int flow_children(we_doc_t *d, int idx, int x, int y, int width, int depth, int measuring);

static int measure_height(we_doc_t *d, int idx, int width, int depth) {
    const we_node_t *n = &d->nodes[idx];
    int fixed, content = 0, parent;
    if (depth > WE_DEPTH_MAX || n->display == WE_DISP_NONE) return 0;
    {
        const we_measure_cache_t *cache = &measure_cache[idx];
        int key_width = forced_width[idx] >= 0 ? forced_width[idx] : width;
        int key_fixed = measure_content[idx] ? -1 : definite_height(d, idx, 0);
        int key_parent = containing_height[idx] >= 0 ? containing_height[idx] : definite_height(d, n->parent, 0);
        if (cache->valid && cache->width == key_width && cache->fixed == key_fixed &&
            cache->parent == key_parent && cache->content == measure_content[idx]) return cache->result;
    }
    if (measure_budget-- <= 0) {
        if (measure_budget == -1) diag(d, "measurement budget exceeded", idx);
        return 0;
    }
    {
        we_measure_cache_t *cache = &measure_cache[idx];
        cache->width = forced_width[idx] >= 0 ? forced_width[idx] : width;
        cache->fixed = measure_content[idx] ? -1 : definite_height(d, idx, 0);
        cache->parent = containing_height[idx] >= 0 ? containing_height[idx] : definite_height(d, n->parent, 0);
        cache->content = measure_content[idx];
        cache->valid = 0;
    }
    fixed = measure_content[idx] ? -1 : definite_height(d, idx, 0);
    if (fixed >= 0) content = fixed;
    else {
        if (forced_width[idx] >= 0) width = forced_width[idx] - node_border_extra(n) - n->margin[1] - n->margin[3];
        else width = specified_width(n, width, width - node_border_extra(n) - n->margin[1] - n->margin[3]);
        if (width < WE_CHAR_W) width = WE_CHAR_W;
        if (w_eq(n->tag, "img")) {
            content = image_measure_height(d, idx);
            if (content < 0) content = WE_LINE_H;
        } else if (n->display == WE_DISP_GRID) content = measure_grid(d, idx, width, depth);
        else if (n->display == WE_DISP_TABLE) content = measure_table(d, idx, width, depth);
        else if (n->display == WE_DISP_FLEX) content = measure_flex(d, idx, width, depth);
        else content = flow_children(d, idx, 0, 0, width, depth, 1);
    }
    parent = containing_height[idx] >= 0 ? containing_height[idx] : definite_height(d, n->parent, 0);
    content = height_constraint(d, idx, content, parent);
    measure_cache[idx].result = limit_coord(content + vertical_extra(n) + n->margin[0] + n->margin[2]);
    measure_cache[idx].valid = 1;
    return measure_cache[idx].result;
}

static int preferred_outer(const we_node_t *n, int containing, int fallback) {
    int width = specified_width(n, containing, fallback);
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
    height = f->measuring ? measure_area_height(d, idx, width, depth) : layout_assigned(d, idx, x, f->y, width, depth);
    slot = f->nfloat++;
    f->floats[slot].x = x;
    f->floats[slot].y = f->y;
    f->floats[slot].w = width;
    f->floats[slot].h = height > 0 ? height : 0;
    f->floats[slot].side = n->float_side;
    if (!f->measuring) d->perf.floats++;
}

static int flow_children(we_doc_t* d, int idx, int cx, int cy, int cwidth, int depth, int measuring) {
    /* One context per bounded layout depth, never on the small guest stack. */
    static we_inline_t contexts[WE_DEPTH_MAX + 1];
    we_inline_t *f = &contexts[depth];
    int c, i, bottom;
    w_zero(f, sizeof(*f));
    f->d = d;
    f->measuring = measuring;
    f->base_x = f->x0 = cx;
    f->base_width = f->width = cwidth;
    f->y = limit_coord(cy);
    f->align = d->nodes[idx].align;
    for (c = d->nodes[idx].first; c >= 0; c = d->nodes[c].next) {
        we_node_t *n = &d->nodes[c];
        if (n->html_space) f->pending_space = 1;
        if (n->display == WE_DISP_NONE) continue;
        if (n->float_side && !n->is_text) {
            place_float(f, c, depth + 1);
        } else if (is_box(n->display)) {
            flush_line(f, 0);
            flow_region(f, WE_CHAR_W);
            f->y = limit_coord(f->y + (measuring ? measure_height(d, c, f->width, depth + 1) :
                layout_block(d, c, f->x0, f->y, f->width, depth + 1)));
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

static int layout_children(we_doc_t *d, int idx, int x, int y, int width, int depth) {
    return flow_children(d, idx, x, y, width, depth, 0);
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
    if (parent && definite_height(d, n->parent, 0) >= 0) {
        int top = parent->y + parent->border + parent->padding[0];
        low = top - n->y;
        high = top + definite_height(d, n->parent, 0) - n->y - n->h;
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
    if (forced_width[idx] >= 0) bw = forced_width[idx] - n->margin[1] - n->margin[3];
    else bw = specified_width(n, cw, cw - n->margin[1] - n->margin[3] - node_border_extra(n)) + node_border_extra(n);
    resolved_height[idx] = (int16_t)definite_height(d, idx, 0);
    if ((n->height_percent || n->min_height_percent || n->max_height_percent) && containing_height[idx] < 0 && definite_height(d, n->parent, 0) < 0)
        diag(d, "percent height indefinite", idx);
    if (bw < 0) bw = 0;
    if (bw > WE_LAYOUT_MAX) bw = WE_LAYOUT_MAX;
    bx = limit_coord(bx);
    by = limit_coord(by);
    n->x = (int16_t)bx; n->y = (int16_t)by; n->w = (int16_t)bw;
    /* reserve the background item now so it paints under the children */
    slot = n->has_bg ? (int)d->nitems : -1;
    if (n->has_bg && !emit(d, idx, OS_FB_WEB_RECT, 0, 0, 0, 0, n->bg)) slot = -1;
    content_x = limit_coord(bx + n->border + n->padding[3]);
    content_w = bw - 2 * n->border - n->padding[1] - n->padding[3];
    if (content_w < WE_CHAR_W) content_w = WE_CHAR_W;
    if (w_eq(n->tag, "img")) {
        int image_h = image_paint(d, idx, content_x, by + n->border + n->padding[0], content_w);
        if (image_h >= 0) {
            content_h = image_h;
            bw = n->w + node_border_extra(n);
        } else {
            os_fb_web_item_t *alt = emit(d, idx, OS_FB_WEB_TEXT, content_x,
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
    if (resolved_height[idx] >= 0) content_h = resolved_height[idx];
    if (forced_height[idx] < 0) content_h = height_constraint(d, idx, content_h, containing_height[idx] >= 0 ? containing_height[idx] : definite_height(d, n->parent, 0));
    bh = limit_coord(content_h + n->padding[0] + n->padding[2] + 2 * n->border);
    if (bh < 0) bh = 0;
    n->x = (int16_t)bx; n->y = (int16_t)by; n->w = (int16_t)bw; n->h = (int16_t)bh;
    if (slot >= 0) {
        os_fb_web_item_t* it = &d->items[slot];
        it->x = (int16_t)bx; it->y = (int16_t)by; it->w = (uint16_t)bw; it->h = (uint16_t)bh;
    }
    if (n->border > 0) {
        os_fb_web_item_t* it = emit(d, idx, OS_FB_WEB_BORDER, bx, by, bw, bh, n->border_color);
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
        it = emit(d, idx, OS_FB_WEB_TEXT, bx - (len + 1) * WE_CHAR_W, by + 1, len * WE_CHAR_W, 8, n->color);
        if (it) w_cpy(it->text, OS_FB_WEB_TEXT_LEN, marker, len);
    }
    relative_shift(d, idx, first_item, x, cw);
    return n->margin[0] + bh + n->margin[2];
}

static int flex_item(const we_node_t* n) { return !n->is_text && n->display != WE_DISP_NONE; }

static int layout_area(we_doc_t *d, int idx, int x, int y, int width, int height, int containing, int depth) {
    int16_t old_width = forced_width[idx], old_height = forced_height[idx];
    int result;
    if (width > WE_LAYOUT_MAX) width = WE_LAYOUT_MAX;
    if (height > WE_LAYOUT_MAX) height = WE_LAYOUT_MAX;
    forced_width[idx] = (int16_t)width;
    forced_height[idx] = (int16_t)height;
    resolved_height[idx] = -1;
    result = layout_block(d, idx, x, y, containing, depth);
    forced_width[idx] = old_width;
    forced_height[idx] = old_height;
    resolved_height[idx] = -1;
    return result;
}

static int layout_assigned(we_doc_t *d, int idx, int x, int y, int outer, int depth) {
    return layout_area(d, idx, x, y, outer, -1, outer, depth);
}

typedef struct { int first, end, used, cross; } we_flex_line_t;
typedef struct {
    int node[WE_MAX_NODES];
    int cross[WE_MAX_NODES];
    we_flex_line_t lines[WE_MAX_NODES];
    int main, cross_available, main_gap, cross_gap, line_count, cross_total;
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

static int pack_distance(int mode, int free, int index, int count);
static int measure_area_height(we_doc_t *d, int idx, int outer, int depth);

typedef struct {
    int row[WE_MAX_NODES], col[WE_MAX_NODES];
    uint8_t occupied[WE_GRID_ROWS_MAX];
    int column[WE_TRACKS_MAX], column_x[WE_TRACKS_MAX], minimum[WE_TRACKS_MAX];
    int height[WE_GRID_ROWS_MAX], row_y[WE_GRID_ROWS_MAX];
} we_grid_work_t;
static we_grid_work_t grid_work[WE_DEPTH_MAX + 1];
typedef char layout_workspace_is_bounded[
    sizeof(track_work) + sizeof(grid_work) +
    sizeof(we_inline_t) * (WE_DEPTH_MAX + 1) +
    sizeof(forced_width) + sizeof(forced_height) + sizeof(resolved_height) +
    sizeof(containing_height) + sizeof(measure_content) + sizeof(measure_cache) + sizeof(int16_t) * OS_FB_WEB_ITEMS +
    sizeof(os_fb_web_item_t) * OS_FB_WEB_ITEMS <= WE_LAYOUT_WORKSPACE_MAX ? 1 : -1];

static int grid_free(const we_grid_work_t *work, int row, int col, int rows, int cols, int count) {
    int r;
    unsigned mask;
    if (row < 0 || col < 0 || row + rows > WE_GRID_ROWS_MAX || col + cols > count) return 0;
    mask = ((1U << cols) - 1U) << col;
    for (r = row; r < row + rows; r++) if (work->occupied[r] & mask) return 0;
    return 1;
}

static void grid_claim(we_grid_work_t *work, int row, int col, int rows, int cols) {
    int r;
    unsigned mask = ((1U << cols) - 1U) << col;
    for (r = row; r < row + rows; r++) work->occupied[r] |= (uint8_t)mask;
}

static int grid_place(we_doc_t *d, we_track_work_t *items, we_grid_work_t *work, int count, int cols, int report) {
    int pass, k, rows = 0, cursor = 0;
    for (k = 0; k < count; k++) work->row[k] = work->col[k] = -1;
    for (pass = 0; pass < 2; pass++) {
        for (k = 0; k < count; k++) {
            const we_node_t *child = &d->nodes[items->node[k]];
            int explicit = child->grid_col_start || child->grid_row_start;
            int rs = child->grid_row_span, cs = child->grid_col_span;
            int at, first, placed = 0;
            if ((pass == 0) != explicit) continue;
            if (rs < 1) rs = 1;
            if (cs < 1) cs = 1;
            first = explicit ? 0 : cursor;
            for (at = first; at < WE_GRID_ROWS_MAX * cols; at++) {
                int r = at / cols, c = at % cols;
                if (child->grid_row_start && r != child->grid_row_start - 1) continue;
                if (child->grid_col_start && c != child->grid_col_start - 1) continue;
                if (!grid_free(work, r, c, rs, cs, cols)) continue;
                grid_claim(work, r, c, rs, cs);
                work->row[k] = r;
                work->col[k] = c;
                if (r + rs > rows) rows = r + rs;
                if (!explicit) cursor = at + cs;
                placed = 1;
                break;
            }
            if (!placed && report) diag(d, explicit ? "grid occupied or outside" : "grid placement capacity", items->node[k]);
        }
    }
    return rows;
}

static int grid_track_sizes(const we_node_t *n, int vertical, int count, int available,
                             int gap, int *sizes) {
    int k, fixed = 0, weight = 0, assigned = 0, free;
    int declared = vertical ? n->grid_rows : n->grid_cols;
    for (k = 0; k < count; k++) {
        int spec = k < declared ? (vertical ? n->row_size[k] : n->col_size[k]) :
                   vertical ? WE_TRACK_AUTO : -1;
        if (spec >= 0) sizes[k] = spec;
        else if (spec != WE_TRACK_AUTO && available >= 0) { weight -= spec; sizes[k] = 0; }
        fixed += sizes[k];
    }
    free = available >= 0 ? available - fixed - (count > 0 ? count - 1 : 0) * gap : 0;
    if (free < 0) free = 0;
    for (k = 0; k < count; k++) {
        int spec = k < declared ? (vertical ? n->row_size[k] : n->col_size[k]) : vertical ? WE_TRACK_AUTO : -1;
        if (spec < 0 && spec != WE_TRACK_AUTO && available >= 0 && weight) {
            sizes[k] = free * (-spec) / weight;
            assigned += sizes[k];
        }
    }
    if (weight) {
        int remainder = free - assigned;
        for (k = 0; k < count && remainder; k++) {
            int spec = k < declared ? (vertical ? n->row_size[k] : n->col_size[k]) : vertical ? WE_TRACK_AUTO : -1;
            if (spec < 0 && spec != WE_TRACK_AUTO) { sizes[k]++; remainder--; }
        }
    }
    assigned = count > 0 ? (count - 1) * gap : 0;
    for (k = 0; k < count; k++) assigned += sizes[k];
    return assigned;
}

static void grid_span_minimum(const we_node_t *n, int vertical, int start, int span,
                              int wanted, int gap, int definite, int *sizes) {
    int k, current = (span - 1) * gap, receivers = 0, declared = vertical ? n->grid_rows : n->grid_cols;
    for (k = start; k < start + span; k++) {
        int spec = k < declared ? (vertical ? n->row_size[k] : n->col_size[k]) : vertical ? WE_TRACK_AUTO : -1;
        current += sizes[k];
        if (spec == WE_TRACK_AUTO || (vertical && definite < 0 && spec < 0)) receivers++;
    }
    if (wanted <= current || !receivers) return;
    wanted -= current;
    for (k = start; k < start + span; k++) {
        int spec = k < declared ? (vertical ? n->row_size[k] : n->col_size[k]) : vertical ? WE_TRACK_AUTO : -1;
        int take;
        if (spec != WE_TRACK_AUTO && !(vertical && definite < 0 && spec < 0)) continue;
        take = wanted / receivers + (wanted % receivers != 0);
        sizes[k] += take;
        wanted -= take;
        receivers--;
    }
}

static int grid_span_extent(const int *sizes, int start, int span, int gap) {
    int k, total = (span - 1) * gap;
    for (k = start; k < start + span; k++) total += sizes[k];
    return total;
}

static uint32_t flex_share(uint32_t a, uint32_t b, uint32_t denominator);

static void fit_auto_columns(const we_node_t *n, int cols, int width, int gap,
                             const int *minimum, int *sizes) {
    int k, used = (cols - 1) * gap, room = 0, excess, removed = 0;
    for (k = 0; k < cols; k++) {
        int spec = k < n->grid_cols ? n->col_size[k] : -1;
        used += spec >= 0 ? spec : spec == WE_TRACK_AUTO ? sizes[k] : 0;
        if (spec == WE_TRACK_AUTO && sizes[k] > minimum[k]) room += sizes[k] - minimum[k];
    }
    excess = used > width ? used - width : 0;
    if (excess > room) excess = room;
    if (!room || !excess) return;
    for (k = 0; k < cols; k++) {
        int spec = k < n->grid_cols ? n->col_size[k] : -1;
        if (spec == WE_TRACK_AUTO && sizes[k] > minimum[k]) {
            int take = (int)flex_share((uint32_t)excess, (uint32_t)(sizes[k] - minimum[k]), (uint32_t)room);
            sizes[k] -= take;
            removed += take;
        }
    }
    for (k = 0; k < cols && removed < excess; k++) {
        int spec = k < n->grid_cols ? n->col_size[k] : -1;
        if (spec == WE_TRACK_AUTO && sizes[k] > minimum[k]) { sizes[k]--; removed++; }
    }
}

static int prepare_grid(we_doc_t *d, int idx, int cx, int cy, int cw, int depth) {
    we_node_t *n = &d->nodes[idx];
    we_track_work_t *items = &track_work[depth];
    we_grid_work_t *work = &grid_work[depth];
    int count = collect_items(d, idx, items), cols = n->grid_cols ? n->grid_cols : 1;
    int rows, k, used_width, used_height, fixed_height = definite_height(d, idx, 0);
    int gx = axis_gap(n, 0), gy = axis_gap(n, 1), position;
    w_zero(work, sizeof(*work));
    for (k = 0; k < count; k++) {
        const we_node_t *child = &d->nodes[items->node[k]];
        int end = child->grid_col_start ? child->grid_col_start + child->grid_col_span - 1 : child->grid_col_span;
        if (end > cols) cols = end;
    }
    if (cols > WE_TRACKS_MAX) cols = WE_TRACKS_MAX;
    rows = grid_place(d, items, work, count, cols, 1);
    if (n->grid_rows > rows) rows = n->grid_rows;
    for (k = 0; k < count; k++) {
        const we_node_t *child = &d->nodes[items->node[k]];
        if (work->row[k] < 0) continue;
        grid_span_minimum(n, 0, work->col[k], child->grid_col_span,
                         preferred_outer(child, cw, child->max_content), gx, cw, work->column);
        grid_span_minimum(n, 0, work->col[k], child->grid_col_span,
                         preferred_outer(child, cw, child->min_content), gx, cw, work->minimum);
    }
    fit_auto_columns(n, cols, cw, gx, work->minimum, work->column);
    used_width = grid_track_sizes(n, 0, cols, cw, gx, work->column);
    if (used_width > cw) diag(d, "grid width overflow", idx);
    position = cx;
    for (k = 0; k < cols; k++) {
        work->column_x[k] = limit_coord(position + pack_distance(n->justify_content, cw - used_width, k, cols));
        position += work->column[k] + gx;
    }
    for (k = 0; k < count; k++) {
        const we_node_t *child = &d->nodes[items->node[k]];
        int width, height;
        if (work->row[k] < 0) continue;
        width = work->column_x[work->col[k] + child->grid_col_span - 1] +
                work->column[work->col[k] + child->grid_col_span - 1] - work->column_x[work->col[k]];
        height = measure_area_height(d, items->node[k], width, depth + 1);
        grid_span_minimum(n, 1, work->row[k], child->grid_row_span, height, gy, fixed_height, work->height);
    }
    used_height = grid_track_sizes(n, 1, rows, fixed_height, gy, work->height);
    position = cy;
    {
        int receivers = 0, remaining = fixed_height > used_height ? fixed_height - used_height : 0;
        for (k = 0; k < rows; k++)
            if (k >= n->grid_rows || n->row_size[k] == WE_TRACK_AUTO) receivers++;
        for (k = 0; k < rows; k++) {
            int extra = 0;
            if (n->align_content == WE_PACK_STRETCH && receivers && (k >= n->grid_rows || n->row_size[k] == WE_TRACK_AUTO)) {
                extra = remaining / receivers + (remaining % receivers != 0);
                remaining -= extra;
                receivers--;
            }
            work->row_y[k] = limit_coord(position + (n->align_content == WE_PACK_STRETCH ? 0 :
                pack_distance(n->align_content, fixed_height - used_height, k, rows)));
            work->height[k] += extra;
            position += work->height[k] + gy;
        }
    }
    return rows;
}

static int measure_grid(we_doc_t *d, int idx, int width, int depth) {
    int rows = prepare_grid(d, idx, 0, 0, width, depth);
    const we_grid_work_t *work = &grid_work[depth];
    return rows ? limit_coord(work->row_y[rows - 1] + work->height[rows - 1]) : 0;
}

static int layout_grid(we_doc_t *d, int idx, int cx, int cy, int cw, int depth) {
    we_node_t *n = &d->nodes[idx];
    we_track_work_t *items = &track_work[depth];
    we_grid_work_t *work = &grid_work[depth];
    int rows = prepare_grid(d, idx, cx, cy, cw, depth);
    int count = collect_items(d, idx, items), k;
    int fixed_height = definite_height(d, idx, 0);
    for (k = 0; k < count; k++) {
        we_node_t *child = &d->nodes[items->node[k]];
        int r = work->row[k], c = work->col[k], width, height, child_width, child_height;
        int horizontal = child->justify_self ? child->justify_self : n->justify_items;
        int vertical = child->align_self ? child->align_self : n->align_items;
        int x, y, assigned_h = -1;
        if (r < 0) continue;
        width = work->column_x[c + child->grid_col_span - 1] + work->column[c + child->grid_col_span - 1] - work->column_x[c];
        height = work->row_y[r + child->grid_row_span - 1] + work->height[r + child->grid_row_span - 1] - work->row_y[r];
        child_width = horizontal == WE_PACK_END || horizontal == WE_PACK_CENTER || horizontal == WE_PACK_START ?
            preferred_outer(child, width, child->max_content) : preferred_outer(child, width, width - node_border_extra(child) - child->margin[1] - child->margin[3]);
        {
            int child_idx = items->node[k];
            int16_t saved = containing_height[child_idx];
            containing_height[child_idx] = (int16_t)height;
            child_height = measure_area_height(d, child_idx, child_width, depth + 1);
            containing_height[child_idx] = saved;
        }
        if (vertical == WE_PACK_STRETCH && child->height < 0 && !child->height_percent)
            child_height = height_constraint(d, items->node[k], height - vertical_extra(child) - child->margin[0] - child->margin[2], fixed_height) + vertical_extra(child) + child->margin[0] + child->margin[2];
        if (vertical == WE_PACK_STRETCH && child->height < 0 && !child->height_percent) assigned_h = child_height;
        x = work->column_x[c] + pack_distance(horizontal, width - child_width, 0, 1);
        y = work->row_y[r] + pack_distance(vertical, height - child_height, 0, 1);
        {
            int child_idx = items->node[k];
            int16_t saved = containing_height[child_idx];
            containing_height[child_idx] = (int16_t)height;
            layout_area(d, child_idx, limit_coord(x), limit_coord(y), child_width, assigned_h, width, depth + 1);
            containing_height[child_idx] = saved;
        }
    }
    d->perf.grid_lines += (uint32_t)rows;
    if (!rows) return 0;
    return limit_coord(work->row_y[rows - 1] - cy + work->height[rows - 1]);
}

static int flex_extra(const we_node_t *n, int vertical) {
    return vertical ? 2 * n->border + n->padding[0] + n->padding[2] + n->margin[0] + n->margin[2] :
                      node_border_extra(n) + n->margin[1] + n->margin[3];
}

static int axis_limit(const we_node_t *n, int containing, int vertical, int high) {
    int value;
    if (vertical) {
        int percent = high ? n->max_height_percent : n->min_height_percent;
        value = high ? n->max_height : n->min_height;
        if (percent) value = containing >= 0 ? containing * (high ? n->max_height_pct : n->min_height_pct) / 100 : -1;
    } else {
        int percent = high ? n->max_pct : n->min_pct;
        value = percent > 0 ? containing * percent / 100 : high ? n->max_width : n->min_width;
    }
    if (value >= 0) value = content_value(value, vertical ? vertical_extra(n) : node_border_extra(n), n->border_box);
    return value;
}

static int grow_limit(const we_node_t *n, int containing, int vertical) {
    int high = axis_limit(n, containing, vertical, 1);
    int low = axis_limit(n, containing, vertical, 0);
    if (high < 0) return WE_LAYOUT_MAX;
    if (low > high) high = low;
    return high + flex_extra(n, vertical);
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
    int minimum = axis_limit(n, containing, vertical, 0);
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

static int pack_distance(int mode, int free, int index, int count) {
    if (free <= 0 || count < 1) return 0;
    if (mode == WE_PACK_END) return free;
    if (mode == WE_PACK_CENTER) return free / 2;
    if (mode == WE_PACK_BETWEEN) return count > 1 ? free * index / (count - 1) : 0;
    if (mode == WE_PACK_AROUND) return free * (2 * index + 1) / (2 * count);
    if (mode == WE_PACK_EVENLY) return free * (index + 1) / (count + 1);
    return 0;
}

static int measure_area_height(we_doc_t *d, int idx, int outer, int depth) {
    int16_t saved = forced_width[idx];
    int height;
    forced_width[idx] = (int16_t)outer;
    height = measure_height(d, idx, outer, depth);
    forced_width[idx] = saved;
    return height;
}

static void prepare_flex(we_doc_t *d, int idx, int cw,
                              int depth, int count, int sizing, int modern) {
    we_node_t *n = &d->nodes[idx];
    we_track_work_t *work = &track_work[depth];
    int vertical = n->flex_dir != 0;
    int main = vertical ? definite_height(d, idx, 0) : cw;
    int cross_available = vertical ? cw : definite_height(d, idx, 0);
    int gap = axis_gap(n, vertical), cross_gap = axis_gap(n, !vertical);
    int wrap = n->flex_wrap && main >= 0;
    int k, lines = 0, cross_total = 0;
    if (n->flex_wrap && main < 0) diag(d, "column wrap requires height", idx);
    for (k = 0; k < count; k++) {
        we_node_t *child = &d->nodes[work->node[k]];
        int basis, intrinsic = child->basis_content ||
            (child->basis_set && child->flex_basis < 0 && !child->basis_pct);
        int factor = child->flex_shrink >= 0 ? child->flex_shrink : sizing || modern ? 1 : 0;
        if (child->basis_pct > 0 && main < 0) {
            diag(d, "column percent requires height", work->node[k]);
            basis = child->height >= 0 ? child->height : 0;
        } else if (child->basis_pct > 0) basis = content_value(main * child->basis_pct / 100,
            vertical ? vertical_extra(child) : node_border_extra(child), child->border_box);
        else if (child->flex_basis >= 0) basis = content_value(child->flex_basis,
            vertical ? vertical_extra(child) : node_border_extra(child), child->border_box);
        else if (vertical) {
            int fixed = child->basis_content ? -1 : definite_height(d, work->node[k], 0);
            if (fixed >= 0) basis = fixed;
            else if (intrinsic || modern || child->flex_shrink >= 0) {
                measure_content[work->node[k]] = child->basis_content;
                basis = measure_area_height(d, work->node[k], cw, depth + 1) - flex_extra(child, 1);
                measure_content[work->node[k]] = 0;
            }
            else basis = 0;
        } else if (child->basis_content) basis = child->max_content;
        else basis = specified_width(child, cw, intrinsic || modern ? child->max_content : wrap ? WE_CHAR_W : 0);
        basis = content_value(basis, 0, 0);
        work->size[k] = (vertical ? height_constraint(d, work->node[k], basis, main) :
                         constrained_width(child, basis, cw)) + flex_extra(child, vertical);
        if (work->size[k] < 0) work->size[k] = 0;
        work->shrink_weight[k] = (uint32_t)basis * (uint32_t)factor;
    }
    k = 0;
    while (k < count) {
        we_flex_line_t *line = &work->lines[lines];
        int used = 0, j;
        line->first = k;
        while (k < count) {
            int next = used + (k > line->first ? gap : 0) + work->size[k];
            if (wrap && k > line->first && next > main) break;
            used = next;
            k++;
        }
        line->end = k;
        if (main >= 0) {
            if (used > main && (sizing || modern)) shrink_line(d, work, line->first, k, used - main, main, vertical);
            else if (main > used) grow_line(d, work, line->first, k, main - used, main, vertical);
        }
        line->used = (k - line->first - 1) * gap;
        line->cross = 0;
        for (j = line->first; j < k; j++) {
            const we_node_t *child = &d->nodes[work->node[j]];
            int align = child->align_self ? child->align_self : n->align_items;
            line->used += work->size[j];
            if (vertical) {
                int fallback = wrap ? WE_CHAR_W : align == WE_PACK_END || align == WE_PACK_CENTER ? child->max_content : cw - flex_extra(child, 0);
                work->cross[j] = preferred_outer(child, cw, fallback);
            } else work->cross[j] = measure_area_height(d, work->node[j], work->size[j], depth + 1);
            if (work->cross[j] > line->cross) line->cross = work->cross[j];
        }
        if (!wrap && cross_available > line->cross) line->cross = cross_available;
        cross_total = limit_coord(cross_total + line->cross + (lines ? cross_gap : 0));
        lines++;
    }
    work->main = main;
    work->cross_available = cross_available;
    work->main_gap = gap;
    work->cross_gap = cross_gap;
    work->line_count = lines;
    work->cross_total = cross_total;
}

static void flex_modes(const we_doc_t *d, int idx, const we_track_work_t *work,
                       int count, int *sizing, int *modern) {
    const we_node_t *n = &d->nodes[idx];
    int k;
    *sizing = n->flex_dir && (n->min_height >= 0 || n->max_height >= 0 || n->min_height_percent || n->max_height_percent);
    *modern = n->justify_content || n->align_items || n->align_content || n->row_gap >= 0 || n->col_gap >= 0;
    for (k = 0; k < count; k++) {
        const we_node_t *child = &d->nodes[work->node[k]];
        if (child->align_self || child->basis_content || child->border_box || child->height_percent ||
            child->min_height_percent || child->max_height_percent || child->width_kind) *modern = 1;
        if (child->basis_set || child->flex_shrink >= 0 ||
            (n->flex_dir && (child->min_height >= 0 || child->max_height >= 0))) *sizing = 1;
    }
}

static int measure_flex(we_doc_t *d, int idx, int width, int depth) {
    const we_node_t *n = &d->nodes[idx];
    we_track_work_t *work = &track_work[depth];
    int count = collect_items(d, idx, work), sizing, modern, k, grow = 0;
    int height = 0, gap = axis_gap(n, n->flex_dir != 0);
    if (!count) return 0;
    flex_modes(d, idx, work, count, &sizing, &modern);
    for (k = 0; k < count; k++) if (d->nodes[work->node[k]].flex_grow >= 0) grow = 1;
    if (!sizing && !modern && (n->flex_dir || (!grow && !n->flex_wrap))) {
        int each = n->flex_dir ? width : (width - (count - 1) * gap) / count;
        if (each < WE_CHAR_W) each = WE_CHAR_W;
        for (k = 0; k < count; k++) {
            int value = measure_height(d, work->node[k], each, depth + 1);
            if (n->flex_dir) height = limit_coord(height + value + (k ? gap : 0));
            else if (value > height) height = value;
        }
        return height;
    }
    prepare_flex(d, idx, width, depth, count, sizing, modern);
    return n->flex_dir ? (work->main >= 0 ? work->main : work->lines[0].used) : work->cross_total;
}

static int layout_flex_tracks(we_doc_t *d, int idx, int cx, int cy, int cw,
                              int depth, int count, int sizing, int modern) {
    we_node_t *n = &d->nodes[idx];
    we_track_work_t *work = &track_work[depth];
    int vertical = n->flex_dir != 0, main, cross_available, gap, cross_gap, lines, cross_total;
    int k, extent = 0, wrap;
    prepare_flex(d, idx, cw, depth, count, sizing, modern);
    main = work->main;
    cross_available = work->cross_available;
    gap = work->main_gap;
    cross_gap = work->cross_gap;
    lines = work->line_count;
    cross_total = work->cross_total;
    wrap = n->flex_wrap && main >= 0;
    for (k = 0; k < lines; k++) {
        we_flex_line_t *line = &work->lines[k];
        int cross_free = cross_available > cross_total ? cross_available - cross_total : 0;
        int main_free = main > line->used ? main - line->used : 0;
        int extra = n->align_content == WE_PACK_STRETCH ? cross_free * (k + 1) / lines - cross_free * k / lines : 0;
        int cross_start = extent + (n->align_content == WE_PACK_STRETCH ? cross_free * k / lines :
            pack_distance(n->align_content, cross_free, k, lines));
        int used = 0, j, line_cross = line->cross + extra;
        for (j = line->first; j < line->end; j++) {
            we_node_t *child = &d->nodes[work->node[j]];
            int align = child->align_self ? child->align_self : n->align_items;
            int cross = work->cross[j], main_start, off, x, y, width, height;
            if (align == WE_PACK_STRETCH) {
                if (vertical && child->width < 0 && !child->width_pct && !child->width_kind)
                    cross = constrained_width(child, line_cross - flex_extra(child, 0), cw) + flex_extra(child, 0);
                if (!vertical && child->height < 0 && !child->height_percent)
                    cross = height_constraint(d, work->node[j], line_cross - flex_extra(child, 1), cross_available) + flex_extra(child, 1);
            }
            off = pack_distance(align, line_cross - cross, 0, 1);
            main_start = used + pack_distance(n->justify_content, main_free, j - line->first, line->end - line->first);
            x = cx + (vertical ? cross_start + off : main_start);
            y = cy + (vertical ? main_start : cross_start + off);
            width = vertical ? cross : work->size[j];
            height = vertical ? work->size[j] : align == WE_PACK_STRETCH && child->height < 0 && !child->height_percent ? cross : -1;
            if (vertical && wrap && (x - cx + width > cw || width < 0)) {
                diag(d, "column wrap width exceeded", work->node[j]);
                return main >= 0 ? main : 0;
            }
            layout_area(d, work->node[j], limit_coord(x), limit_coord(y), width, height, width, depth + 1);
            used += work->size[j] + gap;
        }
        extent = limit_coord(extent + line->cross + (k + 1 < lines ? cross_gap : 0));
        d->perf.flex_lines++;
    }
    if (!vertical) return cross_total + (cross_available > cross_total && n->align_content == WE_PACK_STRETCH ? cross_available - cross_total : 0);
    if (main >= 0) return main;
    return work->lines[0].used;
}

static int layout_flex(we_doc_t* d, int idx, int cx, int cy, int cwidth, int depth) {
    we_node_t *n = &d->nodes[idx];
    we_track_work_t *work = &track_work[depth];
    int count, k, gap = axis_gap(n, 0), y = cy, explicit_grow = 0, sizing = 0;
    int modern = n->justify_content || n->align_items || n->align_content || n->row_gap >= 0 || n->col_gap >= 0;
    if (depth > WE_DEPTH_MAX) return 0;
    if (n->display == WE_DISP_GRID) return layout_grid(d, idx, cx, cy, cwidth, depth);
    count = collect_items(d, idx, work);
    if (!count) return 0;
    flex_modes(d, idx, work, count, &sizing, &modern);
    if (modern || (n->flex_dir && sizing)) return layout_flex_tracks(d, idx, cx, cy, cwidth, depth, count, sizing, modern);
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
    return layout_flex_tracks(d, idx, cx, cy, cwidth, depth, count, sizing, modern);
}

static void intrinsic_track_request(const we_node_t *n, int cols, int start, int span,
                                    int wanted, int gap, int grid, int *sizes) {
    int k, current = (span - 1) * gap, receivers = 0;
    if (span < 1 || start < 0 || start + span > cols) return;
    for (k = start; k < start + span; k++) {
        current += sizes[k];
        if (!grid || k >= n->grid_cols || n->col_size[k] < 0) receivers++;
    }
    if (wanted <= current || !receivers) return;
    wanted -= current;
    for (k = start; k < start + span; k++) {
        int take;
        if (grid && k < n->grid_cols && n->col_size[k] >= 0) continue;
        take = wanted / receivers + (wanted % receivers != 0);
        sizes[k] += take;
        wanted -= take;
        receivers--;
    }
}

static int collect_table_rows(we_doc_t *d, int idx, int *rows, int report) {
    int c, count = 0;
    for (c = d->nodes[idx].first; c >= 0; c = d->nodes[c].next) {
        const we_node_t *n = &d->nodes[c];
        if (n->display == WE_DISP_NONE) continue;
        if (n->display == WE_DISP_ROW) rows[count++] = c;
        else if (w_eq(n->tag, "tbody") || w_eq(n->tag, "thead") || w_eq(n->tag, "tfoot")) {
            int r;
            for (r = n->first; r >= 0; r = d->nodes[r].next)
                if (d->nodes[r].display == WE_DISP_ROW) rows[count++] = r;
        } else if (report) diag(d, "table child refused", c);
    }
    return count;
}

static void intrinsic_track_sizes(we_doc_t *d, int idx, int *minimum, int *maximum) {
    const we_node_t *n = &d->nodes[idx];
    we_track_work_t *items = &track_work[0];
    we_grid_work_t *work = &grid_work[0];
    int grid = n->display == WE_DISP_GRID, count, cols = grid ? n->grid_cols : 0;
    int gap = grid ? axis_gap(n, 0) : 0, k, pass, rows = 0;
    w_zero(work, sizeof(*work));
    if (grid) {
        count = collect_items(d, idx, items);
        if (cols < 1) cols = 1;
        for (k = 0; k < count; k++) {
            const we_node_t *child = &d->nodes[items->node[k]];
            int end = child->grid_col_start ? child->grid_col_start + child->grid_col_span - 1 : child->grid_col_span;
            if (end > cols) cols = end;
        }
        if (cols > WE_TRACKS_MAX) cols = WE_TRACKS_MAX;
        grid_place(d, items, work, count, cols, 0);
    } else {
        int r;
        rows = collect_table_rows(d, idx, items->node, 0);
        count = 0;
        for (r = 0; r < rows; r++) {
            int c, column = 0;
            for (c = d->nodes[items->node[r]].first; c >= 0; c = d->nodes[c].next)
                if (d->nodes[c].display == WE_DISP_CELL) column += d->nodes[c].colspan;
            if (column > cols) cols = column;
        }
        if (cols > WE_TRACKS_MAX) cols = WE_TRACKS_MAX;
    }
    for (k = 0; k < cols; k++) {
        int fixed = grid && k < n->grid_cols && n->col_size[k] >= 0 ? n->col_size[k] : 0;
        work->column[k] = work->column_x[k] = fixed;
    }
    for (pass = 0; pass < 2; pass++) {
        int total = grid ? count : rows, r;
        for (r = 0; r < total; r++) {
            int c = grid ? items->node[r] : d->nodes[items->node[r]].first;
            int column = 0;
            while (c >= 0) {
                const we_node_t *child = &d->nodes[c];
                int span = grid ? child->grid_col_span : child->colspan;
                int start = grid ? work->col[r] : column;
                if (child->display != WE_DISP_NONE && (grid || child->display == WE_DISP_CELL)) {
                    if ((span == 1) == (pass == 0) && start >= 0 && start + span <= cols) {
                        int extra = node_border_extra(child) + child->margin[1] + child->margin[3];
                        int low = child->width >= 0 ? content_value(child->width, node_border_extra(child), child->border_box) : child->min_content;
                        int high = child->width >= 0 ? low : child->max_content;
                        intrinsic_track_request(n, cols, start, span, constrained_width(child, low, 0) + extra,
                                                gap, grid, work->column);
                        intrinsic_track_request(n, cols, start, span, constrained_width(child, high, 0) + extra,
                                                gap, grid, work->column_x);
                    }
                    column += span;
                }
                if (grid) break;
                c = child->next;
            }
        }
    }
    *minimum = *maximum = cols > 0 ? (cols - 1) * gap : 0;
    for (k = 0; k < cols; k++) {
        *minimum = limit_coord(*minimum + work->column[k]);
        *maximum = limit_coord(*maximum + work->column_x[k]);
    }
}

/* Table tracks are planned once for measurement and paint. Colspan is
 * bounded to eight columns; rowspan remains an explicit refusal. */
static int table_tracks(we_doc_t *d, int idx, int width, int depth, int *widths) {
    int *rows = track_work[depth].node;
    int nr = 0, c, r, cols = 0, col;
    int requested[WE_TRACKS_MAX], sum = 0, auto_cols = 0;
    nr = collect_table_rows(d, idx, rows, 1);
    for (col = 0; col < WE_TRACKS_MAX; col++) requested[col] = 0;
    for (r = 0; r < nr; r++) {
        int cells = 0;
        for (c = d->nodes[rows[r]].first; c >= 0; c = d->nodes[c].next) {
            we_node_t *cell = &d->nodes[c];
            int want;
            if (cell->display == WE_DISP_NONE) continue;
            if (cell->display != WE_DISP_CELL) { diag(d, "table cell refused", c); continue; }
            if (cell->span_refused) { diag(d, "table refused after span", idx); return 0; }
            if (cells + cell->colspan > WE_TRACKS_MAX) { diag(d, "table columns exceed limit", idx); return 0; }
            want = preferred_outer(cell, width, d->nodes[idx].width_kind == WE_SIZE_MIN_CONTENT ? cell->min_content :
                                    d->nodes[idx].width_kind ? cell->max_content : 0);
            if (cell->colspan == 1 && (d->nodes[idx].width_kind || cell->width >= 0 || cell->width_pct > 0 || cell->min_width >= 0 || cell->min_pct > 0))
                if (want > requested[cells]) requested[cells] = want;
            cells += cell->colspan;
        }
        if (cells > cols) cols = cells;
    }
    for (r = 0; r < nr; r++) {
        int column = 0;
        for (c = d->nodes[rows[r]].first; c >= 0; c = d->nodes[c].next) {
            const we_node_t *cell = &d->nodes[c];
            int span, current = 0, wanted, k, deficit;
            if (cell->display != WE_DISP_CELL) continue;
            span = cell->colspan;
            if (span < 1) span = 1;
            if (span > 1 && (d->nodes[idx].width_kind || cell->width >= 0 || cell->width_pct || cell->min_width >= 0 || cell->min_pct)) {
                wanted = preferred_outer(cell, width, d->nodes[idx].width_kind == WE_SIZE_MIN_CONTENT ? cell->min_content :
                                         d->nodes[idx].width_kind ? cell->max_content : 0);
                for (k = 0; k < span; k++) current += requested[column + k];
                deficit = wanted > current ? wanted - current : 0;
                for (k = 0; k < span; k++) {
                    int take = deficit / (span - k) + (deficit % (span - k) != 0);
                    requested[column + k] += take;
                    deficit -= take;
                }
            }
            column += span;
        }
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
    return nr;
}

static int measure_table(we_doc_t *d, int idx, int width, int depth) {
    int widths[WE_TRACKS_MAX], nr = table_tracks(d, idx, width, depth, widths);
    int *rows = track_work[depth].node;
    int r, total = 0;
    for (r = 0; r < nr; r++) {
        int c, col = 0, high = d->nodes[rows[r]].height > 0 ? d->nodes[rows[r]].height : 0;
        for (c = d->nodes[rows[r]].first; c >= 0; c = d->nodes[c].next) {
            const we_node_t *cell = &d->nodes[c];
            int span, size, h;
            if (cell->display != WE_DISP_CELL) continue;
            span = cell->colspan;
            size = grid_span_extent(widths, col, span, 0);
            h = measure_area_height(d, c, size, depth + 1);
            if (h > high) high = h;
            col += span;
        }
        total = limit_coord(total + high);
    }
    return total;
}

static int layout_table(we_doc_t *d, int idx, int x, int y, int width, int depth) {
    int widths[WE_TRACKS_MAX], nr = table_tracks(d, idx, width, depth, widths);
    int *rows = track_work[depth].node;
    int r, c, col, cy = y;
    for (r = 0; r < nr; r++) {
        we_node_t *row = &d->nodes[rows[r]];
        int cx = x, high = 0, bg_slot = -1;
        if (row->has_bg) {
            bg_slot = (int)d->nitems;
            if (!emit(d, rows[r], OS_FB_WEB_RECT, x, cy, width, 0, row->bg)) bg_slot = -1;
        }
        col = 0;
        row->x = (int16_t)x;
        row->y = (int16_t)cy;
        row->w = (int16_t)width;
        for (c = row->first; c >= 0; c = d->nodes[c].next) {
            int h;
            if (d->nodes[c].display != WE_DISP_CELL) continue;
            {
                int span = d->nodes[c].colspan;
                int cell_width = grid_span_extent(widths, col, span, 0);
                h = layout_assigned(d, c, cx, cy, cell_width, depth + 1);
                cx += cell_width;
                col += span;
            }
            if (h > high) high = h;
        }
        if (row->height > high) high = row->height;
        row->h = (int16_t)high;
        if (bg_slot >= 0) d->items[bg_slot].h = (uint16_t)high;
        cy = limit_coord(cy + high);
        d->perf.table_rows++;
    }
    return cy - y;
}

static int clip_bounds(const we_doc_t *d, int owner, int *left, int *top, int *right, int *bottom) {
    int p = owner >= 0 ? d->nodes[owner].parent : -1;
    *left = *top = -WE_LAYOUT_MAX;
    *right = *bottom = WE_LAYOUT_MAX;
    for (; p >= 0; p = d->nodes[p].parent) {
        const we_node_t *n = &d->nodes[p];
        int x, y, r, b;
        if (!n->overflow_hidden) continue;
        x = n->x + n->border;
        y = n->y + n->border;
        r = n->x + n->w - n->border;
        b = n->y + n->h - n->border;
        if (x > *left) *left = x;
        if (y > *top) *top = y;
        if (r < *right) *right = r;
        if (b < *bottom) *bottom = b;
    }
    return *right > *left && *bottom > *top;
}

static int clip_rectangle(os_fb_web_item_t *item, int left, int top, int right, int bottom) {
    int x = item->x, y = item->y, r = x + item->w, b = y + item->h;
    if (x < left) x = left;
    if (y < top) y = top;
    if (r > right) r = right;
    if (b > bottom) b = bottom;
    if (r <= x || b <= y) return 0;
    item->x = (int16_t)x;
    item->y = (int16_t)y;
    item->w = (uint16_t)(r - x);
    item->h = (uint16_t)(b - y);
    return 1;
}

static int clip_text(os_fb_web_item_t *item, int left, int top, int right, int bottom) {
    int first, last, len = w_len(item->text), k;
    if (item->y < top || item->y + item->h > bottom) return 0;
    first = left > item->x ? (left - item->x + WE_CHAR_W - 1) / WE_CHAR_W : 0;
    last = right > item->x ? (right - item->x) / WE_CHAR_W : 0;
    if (last > len) last = len;
    if (first >= last) return 0;
    for (k = first; k < last; k++) item->text[k - first] = item->text[k];
    item->text[last - first] = 0;
    item->x = (int16_t)(item->x + first * WE_CHAR_W);
    item->w = (uint16_t)((last - first) * WE_CHAR_W);
    return 1;
}

static void clip_display_list(we_doc_t *d) {
    static os_fb_web_item_t output[OS_FB_WEB_ITEMS];
    static int16_t owners[OS_FB_WEB_ITEMS];
    int i, count = 0;
    for (i = 0; i < (int)d->nitems; i++) {
        os_fb_web_item_t item = d->items[i];
        int left, top, right, bottom, k, fragments = 1;
        os_fb_web_item_t edges[4];
        if (!clip_bounds(d, d->item_owner[i], &left, &top, &right, &bottom)) continue;
        if (item.x >= left && item.y >= top && item.x + item.w <= right && item.y + item.h <= bottom) {
            edges[0] = item;
        } else if (item.kind == OS_FB_WEB_TEXT) {
            if (!clip_text(&item, left, top, right, bottom)) continue;
            edges[0] = item;
        } else if (item.kind == OS_FB_WEB_BORDER) {
            int thickness = item.bold ? item.bold : 1;
            int w = item.w, h = item.h;
            if (thickness > w) thickness = w;
            if (thickness > h) thickness = h;
            fragments = 0;
            for (k = 0; k < 4; k++) {
                os_fb_web_item_t edge = item;
                edge.kind = OS_FB_WEB_RECT;
                if (k < 2) {
                    edge.h = (uint16_t)thickness;
                    if (k) edge.y = (int16_t)(item.y + h - thickness);
                } else {
                    edge.w = (uint16_t)thickness;
                    edge.y = (int16_t)(item.y + thickness);
                    edge.h = (uint16_t)(h > 2 * thickness ? h - 2 * thickness : 0);
                    if (k == 3) edge.x = (int16_t)(item.x + w - thickness);
                }
                if (clip_rectangle(&edge, left, top, right, bottom)) edges[fragments++] = edge;
            }
        } else {
            if (!clip_rectangle(&item, left, top, right, bottom)) continue;
            edges[0] = item;
        }
        if (count + fragments > OS_FB_WEB_ITEMS) {
            d->perf.items_dropped += (uint32_t)fragments;
            diag(d, "clipping display budget", d->item_owner[i]);
            continue;
        }
        for (k = 0; k < fragments; k++) { owners[count] = d->item_owner[i]; output[count++] = edges[k]; }
    }
    for (i = 0; i < count; i++) { d->items[i] = output[i]; d->item_owner[i] = owners[i]; }
    d->nitems = (uint32_t)count;
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
    { uint32_t i; for (i = 0; i < d->nnodes; i++) {
        forced_width[i] = forced_height[i] = resolved_height[i] = containing_height[i] = -1;
        measure_content[i] = 0;
    } }
    image_work.ready_idx = -1;
    w_zero(image_work.failed, sizeof(image_work.failed));
    w_zero(image_work.natural_w, sizeof(image_work.natural_w));
    w_zero(image_work.natural_h, sizeof(image_work.natural_h));
    w_zero(measure_cache, sizeof(measure_cache));
    measure_budget = WE_MEASURE_BUDGET;
    intrinsic_sizes(d);
    h = layout_block(d, 0, 0, 0, viewport_w, 0);
    clip_display_list(d);
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
    int i, best = -1, best_area = 0x7FFFFFFF, best_depth = -1;
    if (!d || !href || max <= 0) return 0;
    href[0] = 0;
    for (i = 0; i < (int)d->nnodes; i++) {
        const we_node_t* n = &d->nodes[i];
        int area, left, top, right, bottom, depth = 0, ancestor;
        if (n->visibility) continue;
        if (!clip_bounds(d, i, &left, &top, &right, &bottom) || x < left || y < top || x >= right || y >= bottom) continue;
        if ((!n->is_text && n->display == WE_DISP_NONE) || n->w <= 0 || n->h <= 0) continue;
        if (x < n->x || y < n->y || x >= n->x + n->w || y >= n->y + n->h) continue;
        if (n->is_text) {
            uint32_t item;
            int painted = 0;
            for (item = 0; item < d->nitems; item++) {
                const os_fb_web_item_t *it = &d->items[item];
                if (d->item_owner[item] == i && it->kind == OS_FB_WEB_TEXT &&
                    x >= it->x && x < it->x + it->w && y >= it->y - 1 && y < it->y - 1 + WE_LINE_H) {
                    painted = 1;
                    break;
                }
            }
            if (!painted) continue;
        }
        if (left < n->x) left = n->x;
        if (top < n->y) top = n->y;
        if (right > n->x + n->w) right = n->x + n->w;
        if (bottom > n->y + n->h) bottom = n->y + n->h;
        area = (right - left) * (bottom - top);
        for (ancestor = i; ancestor >= 0; ancestor = d->nodes[ancestor].parent) depth++;
        if (depth > best_depth || (depth == best_depth && area <= best_area)) {
            best_depth = depth;
            best_area = area;
            best = i;
        }
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
