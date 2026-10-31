/* userspace/webeng.c - Lot 6 (US-031, US-043, US-045). See webeng.h. */
#include "webeng.h"
#include "imgdec.h"
/* Lot 61: the kernel 8x8 font, shared read-only for partial glyph clipping. */
#include "../kernel/gfx_font8.h"

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

static int value_is(const char *v, int len, const char *s);
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
static const char* const k_void[] = { "br", "hr", "img", "input", "meta", "link", "col", 0 };
static int is_void(const char* t) { int i; for (i = 0; k_void[i]; i++) if (w_eq(t, k_void[i])) return 1; return 0; }

/* Lot 62: an ol start or li value, -9999..9999. */
static int list_number(const char *v, int len, int *out) {
    int k = 0, neg = 0, n = 0;
    if (len > 0 && (v[0] == '-' || v[0] == '+')) { neg = v[0] == '-'; k = 1; }
    if (k >= len) return -1;
    for (; k < len; k++) {
        if (v[k] < '0' || v[k] > '9') return -1;
        n = n * 10 + v[k] - '0';
        if (n > 9999) return -1;
    }
    *out = neg ? -n : n;
    return 0;
}

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
    n->rowspan = 1;
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

static int ancestor_pre(const we_doc_t *d, int parent) {
    while (parent >= 0) {
        if (w_eq(d->nodes[parent].tag, "pre")) return 1;
        parent = d->nodes[parent].parent;
    }
    return 0;
}

static int css_hex(char c) {
    return c >= '0' && c <= '9' ? c - '0' : c >= 'a' && c <= 'f' ? c - 'a' + 10 : c >= 'A' && c <= 'F' ? c - 'A' + 10 : -1;
}

/* Lot 63: character references. Numeric references and a set of named
 * ones map to printable ASCII for the kernel font (typographic quotes and
 * dashes to their ASCII forms, no-break and other spaces to a space, the
 * soft hyphen to WE_SHY); other code points become '?'. Unknown names
 * keep the '&'. Returns the bytes used, 0 for a plain '&'. */
static int char_ref(const char *s, int n, char out[4]) {
    static const char *const names[] = { "amp", "lt", "gt", "quot", "apos", "nbsp", "shy", "mdash", "ndash", "hellip",
        "lsquo", "rsquo", "ldquo", "rdquo", "laquo", "raquo", "copy", "reg", "trade", "times", "middot", "bull",
        "ensp", "emsp", "thinsp", "euro" };
    static const char *const text[] = { "&", "<", ">", "\"", "'", " ", "\x1f", "-", "-", "...",
        "'", "'", "\"", "\"", "<<", ">>", "(c)", "(R)", "TM", "x", ".", "*", " ", " ", " ", "EUR" };
    int i = 1, k, code = 0, digits = 0, hex;
    out[0] = 0;
    if (n > 1 && s[1] == '#') {
        hex = n > 2 && (s[2] == 'x' || s[2] == 'X');
        i = hex ? 3 : 2;
        while (i < n && digits < 7 && (hex ? css_hex(s[i]) >= 0 : s[i] >= '0' && s[i] <= '9')) {
            code = code * (hex ? 16 : 10) + css_hex(s[i++]);
            digits++;
        }
        if (!digits || i >= n || s[i] != ';') return 0;
        out[1] = 0;
        if (code == 9 || code == 10 || code == 13 || code == 160 || (code >= 8192 && code <= 8202)) out[0] = code == 10 ? '\n' : code == 9 ? '\t' : ' ';
        else if (code >= 32 && code < 127) out[0] = (char)code;
        else if (code == 8211 || code == 8212) out[0] = '-';
        else if (code == 8216 || code == 8217) out[0] = '\'';
        else if (code == 8220 || code == 8221) out[0] = '"';
        else if (code == 8230) { out[0] = out[1] = out[2] = '.'; out[3] = 0; }
        else out[0] = code == 173 ? WE_SHY : '?';
        return i + 1;
    }
    for (k = 0; k < (int)(sizeof(names) / sizeof(names[0])); k++) {
        int l = w_len(names[k]);
        if (n > l + 1 && w_ieq_n(s + 1, l, names[k]) && s[l + 1] == ';') {
            w_cpy(out, 4, text[k], w_len(text[k]));
            return l + 2;
        }
    }
    return 0;
}

/* Decode entities. Spaces stay in the buffer when the chunk has a glyph or
 * a pre ancestor needs its newlines. A whitespace-only chunk between
 * ordinary elements still collapses to one html space, as before. */
static int add_text(we_doc_t* d, int parent, const char* s, int n) {
    char buf[WE_TEXT_SOURCE_MAX];
    if (n > 0) d->nodes[parent].has_text = 1;
    int i = 0, o = 0, graphic = 0, newline = 0, idx;
    while (i < n && o < (int)sizeof(buf) - 1) {
        char c = s[i];
        if (c == '\r') { i++; continue; }
        if (c == '&') {
            char rep[4];
            int used = char_ref(s + i, n - i, rep), k;
            if (!used) { rep[0] = '&'; rep[1] = 0; used = 1; }
            for (k = 0; rep[k] && o < (int)sizeof(buf) - 1; k++) { if (rep[k] != ' ' && rep[k] != WE_SHY) graphic = 1; buf[o++] = rep[k]; }
            i += used;
            continue;
        }
        if (w_space(c)) {
            buf[o++] = c == '\n' ? '\n' : c == '\t' ? '\t' : ' ';   /* lot 63: tabs kept for tab-size */
            if (c == '\n') newline = 1;
            i++;
            continue;
        }
        graphic = 1;
        buf[o++] = (c >= 32 && c < 127) ? c : '?';
        i++;
    }
    if (i < n) diag(d, "text source exceeds limit", parent);
    if (!graphic && !(ancestor_pre(d, parent) && newline)) {
        if (o > 0) d->nodes[parent].pending_html_space = 1;
        return 0;
    }
    idx = new_node(d, parent);
    if (idx < 0) return -1;
    d->nodes[idx].is_text = 1;
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
                    if (w_eq(an, "id")) n->attr_flags |= 1;
                    else if (w_eq(an, "class")) n->attr_flags |= 2;
                    else if (w_eq(an, "href")) n->attr_flags |= 4;
                    else if (w_eq(an, "src")) n->attr_flags |= 8;
                    else if (w_eq(an, "alt")) n->attr_flags |= 16;
                    if (w_eq(an, "id")) w_cpy(n->id, 16, av, vk);
                    else if (w_eq(an, "class")) w_cpy(n->cls, 32, av, vk);
                    else if (w_eq(an, "style")) { if (pool_put(d, av, vk, &n->style_off) == 0) n->style_len = (uint16_t)vk; }
                    else if (w_eq(an, "alt")) { if (pool_put(d, av, vk, &n->alt_off) == 0) n->alt_len = (uint16_t)vk; }
                    else if (w_eq(an, "colspan") || w_eq(an, "rowspan") ||
                             (w_eq(an, "span") && (w_eq(name, "col") || w_eq(name, "colgroup")))) {
                        int span = 0, digit, valid = vk > 0;
                        for (digit = 0; digit < vk; digit++) {
                            if (av[digit] < '0' || av[digit] > '9' || span > WE_TRACKS_MAX) { valid = 0; break; }
                            span = span * 10 + av[digit] - '0';
                        }
                        if (!valid || span < 1 || span > WE_TRACKS_MAX) {
                            n->span_refused = 1;
                            diag(d, "table span refused", idx);
                        } else if (!w_eq(an, "rowspan")) n->colspan = (uint8_t)span;
                        else n->rowspan = (uint8_t)span;
                    }
                    else if (w_eq(an, "src")) {
                        if (pool_put(d, av, vk, &n->src_off) == 0) n->src_len = (uint16_t)vk;
                    }
                    else if (w_eq(an, "href")) { if (pool_put(d, av, vk, &n->href_off) == 0) n->href_len = (uint16_t)vk; }
                    else if ((w_eq(an, "start") && w_eq(name, "ol")) || (w_eq(an, "value") && w_eq(name, "li"))) {
                        int number;
                        if (list_number(av, vk, &number)) diag(d, "list number refused", idx);
                        else { n->list_value = (int16_t)number; n->list_value_set = 1; }
                    }
                    else if (w_eq(an, "reversed") && w_eq(name, "ol")) n->list_reversed = 1;
                    else if (w_eq(an, "dir")) {
                        /* lot 64: no bidi, so auto finds no strong rtl text */
                        if (value_is(av, vk, "rtl")) n->dir_attr = 2;
                        else if (value_is(av, vk, "ltr") || value_is(av, vk, "auto")) n->dir_attr = 1;
                        else diag(d, "dir attribute refused", idx);
                        if (value_is(av, vk, "auto")) diag(d, "dir auto taken as ltr", idx);
                    }
                    else if (w_eq(an, "type") && (w_eq(name, "ol") || w_eq(name, "ul"))) {
                        static const char types[] = "1aAiI";
                        static const uint8_t styles[] = { WE_LIST_DECIMAL, WE_LIST_LOWER_ALPHA, WE_LIST_UPPER_ALPHA,
                                                          WE_LIST_LOWER_ROMAN, WE_LIST_UPPER_ROMAN };
                        int t;
                        for (t = 0; t < 5; t++) if (vk == 1 && av[0] == types[t]) n->list_type_attr = styles[t];
                        if (w_eq(av, "disc")) n->list_type_attr = WE_LIST_DISC;
                        else if (w_eq(av, "circle")) n->list_type_attr = WE_LIST_CIRCLE;
                        else if (w_eq(av, "square")) n->list_type_attr = WE_LIST_SQUARE;
                        if (!n->list_type_attr) diag(d, "list type refused", idx);
                    }
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
static int sel_ident(const char *s, int len, int *at, char *out, int cap, int lower) {
    int start = *at, n = 0;
    while (*at < len && w_namec(s[*at])) {
        if (n + 1 >= cap) return -1;
        out[n++] = lower ? w_low(s[*at]) : s[*at]; (*at)++;
    }
    out[n] = 0;
    return *at == start ? -1 : 0;
}

static int sel_integer(const char *s, int len, int *out) {
    int sign = 1, i = 0, number = 0;
    if (len && (s[0] == '+' || s[0] == '-')) { sign = s[0] == '-' ? -1 : 1; i++; }
    if (i == len) return -1;
    for (; i < len; i++) {
        if (s[i] < '0' || s[i] > '9' || number > WE_MAX_NODES) return -1;
        number = number * 10 + s[i] - '0';
        if (number > WE_MAX_NODES) return -1;
    }
    *out = number * sign;
    return 0;
}

static int sel_nth(const char *s, int len, int16_t *a, int16_t *b) {
    int at, first = 0, end = len, coefficient = 0, constant = 0, npos = -1;
    while (first < end && w_space(s[first])) first++;
    while (end > first && w_space(s[end - 1])) end--;
    s += first; len = end - first;
    if (w_ieq_n(s, len, "odd") && len == 3) { *a = 2; *b = 1; return 0; }
    if (w_ieq_n(s, len, "even") && len == 4) { *a = 2; *b = 0; return 0; }
    for (at = 0; at < len; at++) if (w_low(s[at]) == 'n') { if (npos >= 0) return -1; npos = at; }
    if (npos < 0) { if (sel_integer(s, len, &constant)) return -1; }
    else {
        if (npos == 0 || (npos == 1 && s[0] == '+')) coefficient = 1;
        else if (npos == 1 && s[0] == '-') coefficient = -1;
        else if (sel_integer(s, npos, &coefficient)) return -1;
        at = npos + 1;
        while (at < len && w_space(s[at])) at++;
        if (at < len) {
            int sign, begin;
            if (s[at] != '+' && s[at] != '-') return -1;
            sign = s[at++] == '-' ? -1 : 1;
            while (at < len && w_space(s[at])) at++;
            begin = at;
            while (at < len && s[at] >= '0' && s[at] <= '9') at++;
            if (at != len || sel_integer(s + begin, at - begin, &constant)) return -1;
            constant *= sign;
        }
    }
    *a = (int16_t)coefficient; *b = (int16_t)constant;
    return 0;
}

static int sel_attribute(const char *s, int len, int *at, we_selector_attr_t *attr) {
    int i = *at + 1, start, quote = 0, length;
    while (i < len && w_space(s[i])) i++;
    if (sel_ident(s, len, &i, attr->name, sizeof(attr->name), 1)) return -1;
    if (!w_eq(attr->name, "id") && !w_eq(attr->name, "class") && !w_eq(attr->name, "href") &&
        !w_eq(attr->name, "src") && !w_eq(attr->name, "alt")) return -1;
    while (i < len && w_space(s[i])) i++;
    if (i < len && s[i] == ']') { *at = i + 1; attr->op = WE_ATTR_PRESENT; return 0; }
    if (i >= len) return -1;
    if (s[i] == '=') { attr->op = WE_ATTR_EQUAL; i++; }
    else {
        char op = s[i++];
        if (i == len || s[i++] != '=') return -1;
        attr->op = (uint8_t)(op == '~' ? WE_ATTR_TOKEN : op == '^' ? WE_ATTR_PREFIX : op == '$' ? WE_ATTR_SUFFIX : op == '*' ? WE_ATTR_CONTAINS : 255);
        if (attr->op == 255) return -1;
    }
    while (i < len && w_space(s[i])) i++;
    if (i < len && (s[i] == '\'' || s[i] == '"')) quote = s[i++];
    start = i;
    while (i < len && (quote ? s[i] != quote : !w_space(s[i]) && s[i] != ']')) {
        if (s[i] == '\\') return -1;
        i++;
    }
    length = i - start;
    if (length >= (int)sizeof(attr->value) || (quote && (i == len || s[i++] != quote))) return -1;
    w_cpy(attr->value, sizeof(attr->value), s + start, length);
    while (i < len && w_space(s[i])) i++;
    if (i < len && w_low(s[i]) == 'i') { attr->insensitive = 1; i++; while (i < len && w_space(s[i])) i++; }
    if (i == len || s[i++] != ']') return -1;
    *at = i;
    return 0;
}

static int parse_sel(const char* s, int len, we_rule_t* r) {
    int i = 0, count = 0;
    w_zero(r->part, sizeof(r->part)); r->ncomp = 0; r->spec = 0;
    while (i < len && w_space(s[i])) i++;
    while (i < len) {
        we_selector_part_t *part;
        int simple = 0, space = 0;
        if (count == WE_SELECTOR_PARTS_MAX) return -1;
        part = &r->part[count];
        if (s[i] == '*') { i++; simple = 1; }
        else if (w_namec(s[i])) {
            if (sel_ident(s, len, &i, part->tag, sizeof(part->tag), 1)) return -1;
            r->spec++; simple = 1;
        }
        while (i < len && !w_space(s[i]) && s[i] != '>' && s[i] != '+' && s[i] != '~') {
            char marker = s[i++];
            if (marker == '.') {
                if (part->classes == WE_SELECTOR_CLASSES_MAX || sel_ident(s, len, &i, part->cls[part->classes], 32, 1)) return -1;
                part->classes++; r->spec += 64;
            } else if (marker == '#') {
                if (part->id[0] || sel_ident(s, len, &i, part->id, sizeof(part->id), 1)) return -1;
                r->spec += 4096;
            } else if (marker == '[') {
                i--;
                if (part->attrs == WE_SELECTOR_ATTRS_MAX || sel_attribute(s, len, &i, &part->attr[part->attrs])) return -1;
                part->attrs++; r->spec += 64;
            } else if (marker == ':') {
                char pseudo[24];
                int element = i < len && s[i] == ':';
                if (element) i++;
                if (sel_ident(s, len, &i, pseudo, sizeof(pseudo), 1)) return -1;
                /* Lot 63: ::before/::after (and the CSS 2 one-colon form) end the selector */
                if (w_eq(pseudo, "before") || w_eq(pseudo, "after")) {
                    int rest = i;
                    while (rest < len && w_space(s[rest])) rest++;
                    if (rest < len) return -1;
                    r->pseudo_el = (uint8_t)(w_eq(pseudo, "before") ? WE_PSEUDO_BEFORE : WE_PSEUDO_AFTER);
                    simple = 1;
                    continue;
                }
                if (element || part->pseudo) return -1;
                part->pseudo = (uint8_t)(w_eq(pseudo, "first-child") ? WE_PSEUDO_FIRST : w_eq(pseudo, "last-child") ? WE_PSEUDO_LAST :
                    w_eq(pseudo, "only-child") ? WE_PSEUDO_ONLY : w_eq(pseudo, "empty") ? WE_PSEUDO_EMPTY :
                    w_eq(pseudo, "root") ? WE_PSEUDO_ROOT : w_eq(pseudo, "nth-child") ? WE_PSEUDO_NTH :
                    w_eq(pseudo, "nth-last-child") ? WE_PSEUDO_NTH_LAST : w_eq(pseudo, "nth-of-type") ? WE_PSEUDO_TYPE :
                    w_eq(pseudo, "nth-last-of-type") ? WE_PSEUDO_TYPE_LAST : 0);
                if (!part->pseudo) return -1;
                if (part->pseudo >= WE_PSEUDO_NTH) {
                    int begin;
                    if (i == len || s[i++] != '(') return -1;
                    begin = i; while (i < len && s[i] != ')') i++;
                    if (i == len || sel_nth(s + begin, i - begin, &part->nth_a, &part->nth_b)) return -1;
                    i++;
                }
                r->spec += 64;
            } else return -1;
            simple = 1;
        }
        if (!simple) return -1;
        count++;
        while (i < len && w_space(s[i])) { space = 1; i++; }
        if (i == len) break;
        if (count == WE_SELECTOR_PARTS_MAX) return -1;
        if (s[i] == '>' || s[i] == '+' || s[i] == '~') {
            r->part[count].combinator = (uint8_t)(s[i] == '>' ? WE_SEL_CHILD : s[i] == '+' ? WE_SEL_ADJACENT : WE_SEL_SIBLING);
            i++; while (i < len && w_space(s[i])) i++;
            if (i == len) return -1;
        } else if (!space) return -1;
    }
    r->ncomp = (uint8_t)count;
    return count ? 0 : -1;
}

/* Lot 63: the end of a block or value skips quoted strings (content). */
static int decl_end(const char *s, int n, int i, char stop) {
    int quote = 0;
    for (; i < n; i++) {
        if (quote) { if (s[i] == '\\' && i + 1 < n) i++; else if (s[i] == quote) quote = 0; }
        else if (s[i] == '"' || s[i] == '\'') quote = s[i];
        else if (s[i] == stop) break;
    }
    return i;
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
        i = decl_end(s, n, i, '}');
        dec_b = i;
        i++;
        a = sel_a;
        while (a < sel_b) {
            int b = a;
            we_rule_t* r;
            uint16_t off;
            { int quote = 0, brackets = 0, parens = 0;
              while (b < sel_b) {
                  int ch = s[b];
                  if (quote) { if (ch == quote) quote = 0; }
                  else if (ch == '\'' || ch == '"') quote = ch;
                  else if (ch == '[') brackets++;
                  else if (ch == ']') brackets--;
                  else if (ch == '(') parens++;
                  else if (ch == ')') parens--;
                  else if (ch == ',' && !brackets && !parens) break;
                  b++;
              }
            }
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

static int sel_equal(const char *a, const char *b, int len, int insensitive) {
    int i;
    for (i = 0; i < len; i++) if ((insensitive ? w_low(a[i]) : a[i]) != (insensitive ? w_low(b[i]) : b[i])) return 0;
    return 1;
}

static int match_attribute(const we_doc_t *d, const we_node_t *n, const we_selector_attr_t *attr) {
    const char *value;
    int len, wanted = w_len(attr->value), i;
    if (w_eq(attr->name, "id")) { value = n->id; len = w_len(value); }
    else if (w_eq(attr->name, "class")) { value = n->cls; len = w_len(value); }
    else if (w_eq(attr->name, "href")) { value = d->pool + n->href_off; len = n->href_len; }
    else if (w_eq(attr->name, "src")) { value = d->pool + n->src_off; len = n->src_len; }
    else { value = d->pool + n->alt_off; len = n->alt_len; }
    {
        int flag = w_eq(attr->name, "id") ? 1 : w_eq(attr->name, "class") ? 2 :
                   w_eq(attr->name, "href") ? 4 : w_eq(attr->name, "src") ? 8 : 16;
        if (!(n->attr_flags & flag)) return 0;
    }
    if (attr->op == WE_ATTR_PRESENT) return 1;
    if (attr->op == WE_ATTR_EQUAL) return len == wanted && sel_equal(value, attr->value, len, attr->insensitive);
    if (!wanted || wanted > len) return 0;
    if (attr->op == WE_ATTR_PREFIX) return sel_equal(value, attr->value, wanted, attr->insensitive);
    if (attr->op == WE_ATTR_SUFFIX) return sel_equal(value + len - wanted, attr->value, wanted, attr->insensitive);
    for (i = 0; i + wanted <= len; i++) {
        if (attr->op == WE_ATTR_TOKEN && ((i && !w_space(value[i - 1])) || (i + wanted < len && !w_space(value[i + wanted])))) continue;
        if (sel_equal(value + i, attr->value, wanted, attr->insensitive)) return 1;
    }
    return 0;
}

static int match_pseudo(const we_doc_t *d, int idx, const we_selector_part_t *part) {
    const we_node_t *n = &d->nodes[idx];
    int c, ordinal = 0, total = 0, typed = part->pseudo == WE_PSEUDO_TYPE || part->pseudo == WE_PSEUDO_TYPE_LAST;
    if (!part->pseudo) return 1;
    if (part->pseudo == WE_PSEUDO_ROOT) return n->parent == 0;
    if (part->pseudo == WE_PSEUDO_EMPTY) {
        if (n->has_text) return 0;
        for (c = n->first; c >= 0; c = d->nodes[c].next)
            if (!d->nodes[c].is_text || d->nodes[c].text_len) return 0;
        return 1;
    }
    if (n->parent < 0) return 0;
    for (c = d->nodes[n->parent].first; c >= 0; c = d->nodes[c].next) {
        const we_node_t *child = &d->nodes[c];
        if (child->is_text || (typed && !w_eq(child->tag, n->tag))) continue;
        total++;
        if (c == idx) ordinal = total;
    }
    if (part->pseudo == WE_PSEUDO_FIRST) return ordinal == 1;
    if (part->pseudo == WE_PSEUDO_LAST) return ordinal == total;
    if (part->pseudo == WE_PSEUDO_ONLY) return total == 1;
    if (part->pseudo == WE_PSEUDO_NTH_LAST || part->pseudo == WE_PSEUDO_TYPE_LAST) ordinal = total - ordinal + 1;
    ordinal -= part->nth_b;
    if (!part->nth_a) return ordinal == 0;
    return ordinal % part->nth_a == 0 && ordinal / part->nth_a >= 0;
}

static int match_comp(const we_doc_t *d, int idx, const we_selector_part_t *part) {
    const we_node_t *n = &d->nodes[idx]; int i;
    if (n->is_text) return 0;
    if (part->tag[0] && !w_eq(n->tag, part->tag)) return 0;
    if (part->id[0] && (w_len(n->id) != w_len(part->id) || !sel_equal(n->id, part->id, w_len(n->id), 1))) return 0;
    for (i = 0; i < part->classes; i++) if (!has_class(n->cls, part->cls[i])) return 0;
    for (i = 0; i < part->attrs; i++) if (!match_attribute(d, n, &part->attr[i])) return 0;
    return match_pseudo(d, idx, part);
}

static int previous_element(const we_doc_t *d, int idx) {
    int c, previous = -1, parent = d->nodes[idx].parent;
    if (parent < 0) return -1;
    for (c = d->nodes[parent].first; c >= 0 && c != idx; c = d->nodes[c].next)
        if (!d->nodes[c].is_text) previous = c;
    return previous;
}

static int match_chain(const we_doc_t *d, int idx, const we_rule_t *r, int part, int *budget) {
    int candidate, relation;
    if (idx <= 0 || (*budget)-- <= 0 || !match_comp(d, idx, &r->part[part])) return 0;
    if (!part) return 1;
    relation = r->part[part].combinator;
    candidate = relation == WE_SEL_ADJACENT || relation == WE_SEL_SIBLING ? previous_element(d, idx) : d->nodes[idx].parent;
    while (candidate > 0 && *budget > 0) {
        if (match_chain(d, candidate, r, part - 1, budget)) return 1;
        if (relation == WE_SEL_CHILD || relation == WE_SEL_ADJACENT) break;
        candidate = relation == WE_SEL_SIBLING ? previous_element(d, candidate) : d->nodes[candidate].parent;
    }
    return 0;
}

static int match_rule(we_doc_t* d, int idx, const we_rule_t* r) {
    int budget = WE_SELECTOR_WORK_MAX;
    int result = r->ncomp ? match_chain(d, idx, r, r->ncomp - 1, &budget) : 0;
    if (budget <= 0) diag(d, "selector work budget", idx);
    return result;
}

/* Lot 62: colours carry a transparency byte in bits 24..31 (0 opaque,
 * 255 fully transparent). Layout and stacking keep it; the compositing
 * pass resolves it against the backdrop and clears it, since the kernel
 * painter masks the byte. */
#define WE_ALPHA_SHIFT 24
static int color_alpha_of(uint32_t c);
static uint32_t with_alpha(uint32_t rgb, int alpha) {
    if (alpha < 0) alpha = 0;
    if (alpha > 255) alpha = 255;
    return (rgb & 0xFFFFFFu) | ((uint32_t)(255 - alpha) << WE_ALPHA_SHIFT);
}

/* One colour argument: an integer or decimal number with an optional %
 * (CSS Color 4 legacy and space syntax). Values are returned in 1/1000
 * units, the percent flag tells which form was used. */
static int color_number(const char *v, int n, int *milli, int *percent) {
    int at = 0, whole = 0, frac = 0, scale = 100, digits = 0, negative = 0;
    *percent = n > 0 && v[n - 1] == '%';
    if (*percent) n--;
    if (n > 3 && w_ieq_n(v + n - 3, 3, "deg")) n -= 3;
    if (at < n && (v[at] == '-' || v[at] == '+')) { negative = v[at] == '-'; at++; }
    while (at < n && v[at] >= '0' && v[at] <= '9') { if (whole < 100000) whole = whole * 10 + v[at] - '0'; at++; digits++; }
    if (at < n && v[at] == '.') {
        at++;
        while (at < n && v[at] >= '0' && v[at] <= '9') { frac += (v[at] - '0') * scale; scale /= 10; at++; digits++; }
    }
    if (!digits || at != n) return -1;
    *milli = whole * 1000 + frac;
    if (negative) *milli = -*milli;
    return 0;
}

static int clamp_int(int v, int lo, int hi) {
    return v < lo ? lo : v > hi ? hi : v;
}

/* Alpha argument: a number 0..1 or a percentage, clamped, as 0..255. */
static int color_alpha(int milli, int percent) {
    int a = percent ? milli / 100 : milli;
    a = clamp_int(a, 0, 1000);
    return (a * 255 + 500) / 1000;
}

/* hsl() to rgb in 32-bit integer arithmetic (no 64-bit division on i386):
 * hue in 1/1000 degrees, s and l in 1/1000 percent, reduced to 1/10
 * degree and per-mille. */
static uint32_t hsl_rgb(int hue, int sat, int light) {
    int c, x, m, h6, part, r = 0, g = 0, b = 0;
    hue = (hue / 100) % 3600;
    if (hue < 0) hue += 3600;
    sat = clamp_int(sat, 0, 100000) / 100;
    light = clamp_int(light, 0, 100000) / 100;
    c = (1000 - (2 * light > 1000 ? 2 * light - 1000 : 1000 - 2 * light)) * sat / 1000;
    h6 = hue / 600;
    part = hue % 1200;
    x = c * (600 - (part > 600 ? part - 600 : 600 - part)) / 600;
    m = light - c / 2;
    if (h6 == 0) { r = c; g = x; } else if (h6 == 1) { r = x; g = c; } else if (h6 == 2) { g = c; b = x; }
    else if (h6 == 3) { g = x; b = c; } else if (h6 == 4) { r = x; b = c; } else { r = c; b = x; }
    r = ((r + m) * 255 + 500) / 1000; g = ((g + m) * 255 + 500) / 1000; b = ((b + m) * 255 + 500) / 1000;
    return ((uint32_t)clamp_int(r, 0, 255) << 16) | ((uint32_t)clamp_int(g, 0, 255) << 8) | (uint32_t)clamp_int(b, 0, 255);
}

/* rgb()/rgba()/hsl()/hsla(): three channels then an optional alpha,
 * separated by commas, or by spaces with "/ alpha". */
static int parse_color_function(const char *v, int n, uint32_t *out) {
    int hsl, open, at, count = 0, slash = 0, commas = 0, value[4], percent[4], k, alpha = 255;
    if (n > 5 && (w_ieq_n(v, 5, "rgba(") || w_ieq_n(v, 5, "hsla("))) open = 5;
    else if (n > 4 && (w_ieq_n(v, 4, "rgb(") || w_ieq_n(v, 4, "hsl("))) open = 4;
    else return -1;
    hsl = w_low(v[0]) == 'h';
    if (v[n - 1] != ')') return -1;
    at = open;
    while (at < n - 1) {
        int start;
        while (at < n - 1 && w_space(v[at])) at++;
        if (at >= n - 1) break;
        if (v[at] == ',' || v[at] == '/') {
            if (v[at] == ',') commas++; else { if (slash || count != 3) return -1; slash = 1; }
            if (count == 0) return -1;
            at++;
            continue;
        }
        if (count == 4) return -1;
        start = at;
        while (at < n - 1 && !w_space(v[at]) && v[at] != ',' && v[at] != '/') at++;
        if (color_number(v + start, at - start, &value[count], &percent[count])) return -1;
        count++;
    }
    if (count < 3 || (commas && slash) || (commas && commas != count - 1) || (!commas && count == 4 && !slash))
        return -1;
    if (count == 4) alpha = color_alpha(value[3], percent[3]);
    if (hsl) {
        if (percent[0] || !percent[1] || !percent[2]) return -1;
        *out = with_alpha(hsl_rgb(value[0], value[1], value[2]), alpha);
        return 0;
    }
    *out = 0;
    for (k = 0; k < 3; k++) {
        int c = percent[k] ? (value[k] * 255 + 50000) / 100000 : (value[k] + 500) / 1000;
        if (percent[k] != percent[0]) return -1;
        *out = (*out << 8) | (uint32_t)clamp_int(c, 0, 255);
    }
    *out = with_alpha(*out, alpha);
    return 0;
}

static int parse_color(const char* v, int n, uint32_t* out) {
    static const struct { const char* name; uint32_t rgb; } named[] = {
        {"black", 0x000000}, {"white", 0xFFFFFF}, {"red", 0xFF0000}, {"green", 0x008000},
        {"blue", 0x0000FF}, {"yellow", 0xFFFF00}, {"gray", 0x808080}, {"grey", 0x808080},
        {"navy", 0x000080}, {"teal", 0x008080}, {"orange", 0xFFA500}, {"purple", 0x800080},
        {"silver", 0xC0C0C0}, {"maroon", 0x800000}, {"lime", 0x00FF00}, {0, 0}};
    int i, alpha = 255;
    if (n > 0 && v[0] == '#') {
        uint32_t x = 0;
        int k;
        for (k = 1; k < n; k++) {
            char c = w_low(v[k]);
            int h = (c >= '0' && c <= '9') ? c - '0' : (c >= 'a' && c <= 'f') ? c - 'a' + 10 : -1;
            if (h < 0) return -1;
            x = (x << 4) | (uint32_t)h;
        }
        if (n == 5) { alpha = (int)(x & 0xF) * 17; x >>= 4; }
        if (n == 9) { alpha = (int)(x & 0xFF); x >>= 8; }
        if (n == 4 || n == 5) x = ((x & 0xF00) << 12) | ((x & 0xF00) << 8) | ((x & 0x0F0) << 8) | ((x & 0x0F0) << 4) | ((x & 0x00F) << 4) | (x & 0x00F);
        else if (n != 7 && n != 9) return -1;
        *out = with_alpha(x, alpha);
        return 0;
    }
    if (value_is(v, n, "transparent")) { *out = with_alpha(0, 0); return 0; }
    if (n > 4 && v[n - 1] == ')') return parse_color_function(v, n, out);
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
        /* Lot 62: spaces inside a functional value (rgb(1 2 3)) do not split. */
        { int depth = 0;
          while (i < n && (depth || !w_space(v[i]))) {
              if (v[i] == '(') depth++;
              else if (v[i] == ')' && depth) depth--;
              i++;
          } }
        l[c] = i - a[c];
        c++;
    }
    return c;
}

/* Lot 62: "12.5%" or "-10%" (sign only when allowed), percent kept in
 * tenths so 12.5% resolves exactly; at most 1000%. */
static int spacing_percent(const char *v, int len, int negative_ok, int *tenths) {
    int i = 0, whole = 0, frac = 0, digits = 0, neg = 0;
    if (len > 0 && v[0] == '-') { if (!negative_ok) return -1; neg = 1; i = 1; }
    while (i < len && v[i] >= '0' && v[i] <= '9') { if (whole <= 1000) whole = whole * 10 + v[i] - '0'; i++; digits++; }
    if (i < len && v[i] == '.') {
        i++;
        if (i < len && v[i] >= '0' && v[i] <= '9') { frac = v[i] - '0'; digits++; }
        while (i < len && v[i] >= '0' && v[i] <= '9') i++;
    }
    if (!digits || i != len - 1 || v[i] != '%' || whole > 1000) return -1;
    *tenths = (neg ? -1 : 1) * (whole * 10 + frac);
    return 0;
}

/* Lot 62: record (or clear when tenths is WE_NO_PCT) the percentage of one
 * side; box 0 margin, 1 padding. */
#define WE_NO_PCT 0x7fff
static void spacing_set(we_node_t *n, int box, int side, int tenths) {
    int bit = box * 4 + side;
    if (tenths == WE_NO_PCT) { n->spacing_pct &= (uint8_t)~(1u << bit); return; }
    n->spacing_pct |= (uint8_t)(1u << bit);
    n->spacing_value[bit] = (int16_t)tenths;
    if (box) n->padding[side] = 0; else n->margin[side] = 0;
}

static void box4(we_node_t *node, int box, const char* v, int n, int16_t out[4]) {
    int a[4], l[4], px[4], pct[4], c, k;
    static const int8_t from[4][4] = { {0, 0, 0, 0}, {0, 1, 0, 1}, {0, 1, 2, 1}, {0, 1, 2, 3} };
    c = tokens(v, n, a, l);
    if (c < 1) return;
    for (k = 0; k < c; k++) {
        pct[k] = WE_NO_PCT;
        if (spacing_percent(v + a[k], l[k], !box, &pct[k]) == 0) px[k] = 0;
        else { pct[k] = WE_NO_PCT; if (parse_len(v + a[k], l[k], &px[k], 0) != 0) px[k] = 0; }
    }
    for (k = 0; k < 4; k++) {
        out[k] = (int16_t)px[from[c - 1][k]];
        spacing_set(node, box, k, pct[from[c - 1][k]]);
    }
}

/* Lot 62: percentages of the boxes laid out in a containing block of width
 * cw: in-flow and floated children and, through inline boxes and table
 * rows, their descendants. Absolute children also use cw (the parent
 * content box, not their positioned containing block). */
static void spacing_node(we_node_t *n, int cw) {
    int bit;
    if (cw < 0) cw = 0;
    for (bit = 0; bit < 8; bit++) if (n->spacing_pct & (1u << bit)) {
        int t = n->spacing_value[bit], v = t < 0 ? -((-t * cw) / 1000) : (t * cw) / 1000;
        if (v > 2000) v = 2000;
        if (v < -2000) v = -2000;
        if (bit < 4) n->margin[bit] = (int16_t)v; else n->padding[bit - 4] = (int16_t)v;
    }
}

/* Lot 62 round 2: grid items resolve against their grid area width
 * (prepare_grid, 0 for track sizing) and absolute boxes against the
 * padding box of their containing block (place_absolutes). */
static void spacing_resolve(we_doc_t *d, int parent, int cw, int depth) {
    int c, grid = d->nodes[parent].display == WE_DISP_GRID;
    if (depth > WE_DEPTH_MAX) return;
    for (c = d->nodes[parent].first; c >= 0; c = d->nodes[c].next) {
        we_node_t *n = &d->nodes[c];
        if (n->is_text || n->absolute) continue;
        spacing_node(n, grid ? 0 : cw);
        if (n->display == WE_DISP_INLINE || n->display == WE_DISP_ROW) spacing_resolve(d, c, cw, depth + 1);
    }
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

static int math_refused;

enum { M_NUMBER, M_PX, M_PERCENT, M_ADD, M_SUB, M_MUL, M_DIV, M_MIN, M_MAX, M_CLAMP };
typedef struct {
    const char *text;
    int length, at, depth;
    we_length_t *out;
} we_math_parser_t;

static int math_checked(int a, int b, int op, int *value) {
    int result, aa = a < 0 ? -a : a, bb = b < 0 ? -b : b;
    if (aa > WE_MATH_VALUE_MAX || bb > WE_MATH_VALUE_MAX) return -1;
    if (op == M_MUL) {
        if (bb && aa > WE_MATH_VALUE_MAX / bb) return -1;
        result = a * b;
    } else if (op == M_DIV) {
        if (!b) return -1;
        result = a / b;
    } else if (op == M_ADD) result = a + b;
    else result = a - b;
    if (result < -WE_MATH_VALUE_MAX || result > WE_MATH_VALUE_MAX) return -1;
    *value = result;
    return 0;
}

static int math_eval(const we_length_t *expr, int basis, int *out) {
    int values[WE_MATH_NODES_MAX], i;
    if (!expr->count || expr->count > WE_MATH_NODES_MAX) return -2;
    if (basis < 0 && expr->percent) return -1;
    for (i = 0; i < expr->count; i++) {
        const we_math_node_t *node = &expr->node[i];
        int a, b;
        if (node->op <= M_PERCENT) {
            values[i] = node->op == M_PERCENT ? basis * node->value / 100 : node->value;
            continue;
        }
        if (node->a >= i || node->b >= i) return -2;
        a = values[node->a]; b = values[node->b];
        if (node->op == M_MIN) values[i] = a < b ? a : b;
        else if (node->op == M_MAX) values[i] = a > b ? a : b;
        else if (node->op == M_CLAMP) {
            int c;
            if (node->c >= i) return -2;
            c = values[node->c];
            values[i] = b < c ? b : c;
            if (values[i] < a) values[i] = a;
        } else if (math_checked(a, b, node->op, &values[i])) return -2;
    }
    *out = values[expr->count - 1];
    return 0;
}

static int math_used(const we_node_t *n, int slot, int basis, int fallback, int nonnegative) {
    int result;
    if (!n->length[slot].count) return fallback;
    if (math_eval(&n->length[slot], basis, &result)) { math_refused = 1; return fallback; }
    if (nonnegative && result < 0) return 0;
    if (result > WE_LAYOUT_MAX) result = WE_LAYOUT_MAX;
    if (result < -WE_LAYOUT_MAX) result = -WE_LAYOUT_MAX;
    return result;
}

static int math_value(const we_node_t *n, int slot, int basis, int fallback) {
    return math_used(n, slot, basis, fallback, 0);
}

static int math_nonnegative(const we_node_t *n, int slot, int basis, int fallback) {
    return math_used(n, slot, basis, fallback, 1);
}

static void math_space(we_math_parser_t *p) {
    while (p->at < p->length && w_space(p->text[p->at])) p->at++;
}

static int math_node(we_math_parser_t *p, int op, int a, int b, int c, int value, int dim) {
    we_math_node_t *node;
    int index = p->out->count;
    if (index >= WE_MATH_NODES_MAX) return -1;
    node = &p->out->node[index];
    node->op = (uint8_t)op; node->a = (uint8_t)a; node->b = (uint8_t)b; node->c = (uint8_t)c;
    node->value = (int16_t)value; node->dimension = (uint8_t)dim;
    p->out->count++;
    if (op == M_PERCENT) p->out->percent = 1;
    return index;
}

static int math_sum(we_math_parser_t *p);

static int math_atom(we_math_parser_t *p) {
    int first, number = 0, sign = 1, op = M_NUMBER, result;
    math_space(p);
    if (p->at == p->length) return -1;
    if (p->text[p->at] == '+' || p->text[p->at] == '-') {
        sign = p->text[p->at++] == '-' ? -1 : 1;
    }
    first = p->at;
    if (first < p->length && p->text[first] >= '0' && p->text[first] <= '9') {
        while (p->at < p->length && p->text[p->at] >= '0' && p->text[p->at] <= '9') p->at++;
        if (bounded_uint(p->text + first, p->at - first, WE_MATH_LITERAL_MAX, &number)) return -1;
        if (p->at < p->length && p->text[p->at] == '%') { op = M_PERCENT; p->at++; }
        else if (p->at + 2 <= p->length && w_ieq_n(p->text + p->at, 2, "px")) { op = M_PX; p->at += 2; }
        return math_node(p, op, 0, 0, 0, number * sign, op != M_NUMBER);
    }
    if (sign < 0 || first >= p->length || p->depth >= WE_MATH_DEPTH_MAX) return -1;
    p->depth++;
    if (p->text[p->at] == '(') {
        p->at++; result = math_sum(p); math_space(p);
        if (result < 0 || p->at == p->length || p->text[p->at++] != ')') return -1;
    } else {
        int name = p->at, kind, args[4], count = 0;
        while (p->at < p->length && w_low(p->text[p->at]) >= 'a' && w_low(p->text[p->at]) <= 'z') p->at++;
        kind = value_is(p->text + name, p->at - name, "calc") ? M_ADD :
               value_is(p->text + name, p->at - name, "min") ? M_MIN :
               value_is(p->text + name, p->at - name, "max") ? M_MAX :
               value_is(p->text + name, p->at - name, "clamp") ? M_CLAMP : -1;
        if (kind < 0 || p->at == p->length || p->text[p->at++] != '(') return -1;
        for (;;) {
            if (count == 4) return -1;
            args[count] = math_sum(p);
            if (args[count] < 0) return -1;
            if (count && p->out->node[args[count]].dimension != p->out->node[args[0]].dimension) return -1;
            count++; math_space(p);
            if (p->at == p->length) return -1;
            if (p->text[p->at] != ',') break;
            p->at++;
        }
        if (p->text[p->at++] != ')' || (kind == M_ADD && count != 1) || (kind == M_CLAMP && count != 3)) return -1;
        result = args[0];
        if (kind == M_CLAMP) result = math_node(p, kind, args[0], args[1], args[2], 0, p->out->node[args[0]].dimension);
        else if (kind != M_ADD) {
            int i;
            for (i = 1; i < count; i++) {
                result = math_node(p, kind, result, args[i], 0, 0, p->out->node[args[0]].dimension);
                if (result < 0) return -1;
            }
        }
    }
    p->depth--;
    return result;
}

static int math_product(we_math_parser_t *p) {
    int left = math_atom(p);
    if (left < 0) return -1;
    for (;;) {
        int op, right, ld, rd, saved = p->at;
        math_space(p);
        if (p->at == p->length || (p->text[p->at] != '*' && p->text[p->at] != '/')) { p->at = saved; break; }
        op = p->text[p->at++] == '*' ? M_MUL : M_DIV;
        right = math_atom(p);
        if (right < 0) return -1;
        ld = p->out->node[left].dimension; rd = p->out->node[right].dimension;
        if ((op == M_MUL && ld && rd) || (op == M_DIV && rd)) return -1;
        left = math_node(p, op, left, right, 0, 0, ld || rd);
        if (left < 0) return -1;
    }
    return left;
}

static int math_sum(we_math_parser_t *p) {
    int left = math_product(p);
    if (left < 0) return -1;
    for (;;) {
        int op, right, saved = p->at;
        math_space(p);
        if (p->at == p->length || (p->text[p->at] != '+' && p->text[p->at] != '-')) { p->at = saved; break; }
        /* Binary +/- require whitespace on each side; unary signs do not. */
        if (p->at == saved || p->at + 1 == p->length || !w_space(p->text[p->at + 1])) return -1;
        op = p->text[p->at++] == '+' ? M_ADD : M_SUB;
        right = math_product(p);
        if (right < 0 || p->out->node[left].dimension != p->out->node[right].dimension) return -1;
        left = math_node(p, op, left, right, 0, 0, p->out->node[left].dimension);
        if (left < 0) return -1;
    }
    return left;
}

static int parse_math(const char *v, int len, we_length_t *out) {
    we_math_parser_t parser;
    int root, result;
    w_zero(out, sizeof(*out)); w_zero(&parser, sizeof(parser));
    parser.text = v; parser.length = len; parser.out = out;
    root = math_sum(&parser); math_space(&parser);
    if (root < 0 || parser.at != len || root != out->count - 1 || !out->node[root].dimension) return -1;
    /* A zero denominator and constant overflow are rejected before cascade. */
    if (math_eval(out, 0, &result)) return -1;
    return 0;
}

static int math_slot(const char *prop) {
    static const char *const names[WE_LENGTH_PROPS] = {
        "width", "height", "min-width", "max-width", "min-height", "max-height", "flex-basis",
        "left", "top", "right", "bottom", "row-gap", "column-gap"
    };
    int i;
    for (i = 0; i < WE_LENGTH_PROPS; i++) if (w_eq(prop, names[i])) return i;
    return -1;
}

static int math_function(const char *v, int len) {
    return w_ieq_n(v, len, "calc(") || w_ieq_n(v, len, "min(") || w_ieq_n(v, len, "max(") || w_ieq_n(v, len, "clamp(");
}

static void set_math(we_node_t *n, int slot, const we_length_t *expr) {
    int fixed = 0;
    n->length[slot] = *expr;
    (void)math_eval(expr, 0, &fixed);
    if (fixed < 0) fixed = 0;
    if (fixed > WE_LAYOUT_MAX) fixed = WE_LAYOUT_MAX;
    if (slot == WE_LEN_WIDTH) {
        n->width_kind = 0; n->width = expr->percent ? -1 : (int16_t)fixed; n->width_pct = expr->percent ? 1 : 0;
    } else if (slot == WE_LEN_HEIGHT) {
        n->height = expr->percent ? -1 : (int16_t)fixed; n->height_percent = expr->percent; n->height_pct = 0;
    } else if (slot == WE_LEN_MIN_W || slot == WE_LEN_MAX_W) {
        if (slot == WE_LEN_MIN_W) n->min_width_kind = WE_SIZE_AUTO;
        else n->max_width_kind = WE_SIZE_AUTO;
        if (slot == WE_LEN_MIN_W) { n->min_width = (int16_t)fixed; n->min_pct = 0; }
        else { n->max_width = (int16_t)fixed; n->max_pct = 0; }
    } else if (slot == WE_LEN_MIN_H || slot == WE_LEN_MAX_H) {
        if (slot == WE_LEN_MIN_H) { n->min_height = (int16_t)fixed; n->min_height_percent = expr->percent; n->min_height_pct = 0; }
        else { n->max_height = (int16_t)fixed; n->max_height_percent = expr->percent; n->max_height_pct = 0; }
    } else if (slot == WE_LEN_BASIS) {
        n->basis_kind = WE_SIZE_AUTO;
        n->basis_content = 0; n->basis_set = 1; n->flex_basis = expr->percent ? -1 : (int16_t)fixed; n->basis_pct = expr->percent ? 1 : 0;
    } else if (slot >= WE_LEN_LEFT && slot <= WE_LEN_BOTTOM) {
        static const uint8_t flags[] = {WE_INSET_LEFT, WE_INSET_TOP, WE_INSET_RIGHT, WE_INSET_BOTTOM};
        n->inset_flags |= flags[slot - WE_LEN_LEFT];
    } else if (slot == WE_LEN_ROW_GAP) n->row_gap = (int16_t)fixed;
    else n->col_gap = (int16_t)fixed;
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

static int track_value(const char *v, int len, int16_t *size, int16_t *low, int16_t *cap) {
    int start, end, number, percent;
    *low = *cap = -1;
    if (w_ieq_n(v, len, "minmax(")) {
        start = 7;
        while (start < len && w_space(v[start])) start++;
        end = start;
        while (end < len && v[end] != ',') end++;
        if (end == len) return -1;
        { int last = end; while (last > start && w_space(v[last - 1])) last--;
          if (parse_dimension(v + start, last - start, &number, &percent) || percent) return -1; }
        *low = (int16_t)number;
        start = end + 1;
        while (start < len && w_space(v[start])) start++;
        end = len;
        while (end > start && w_space(v[end - 1])) end--;
        if (end <= start || v[end - 1] != ')') return -1;
        end--;
        while (end > start && w_space(v[end - 1])) end--;
        if (track_token(v + start, end - start, size)) return -1;
        if (*size >= 0) {
            *cap = *size < *low ? *low : *size;
            *size = WE_TRACK_AUTO;
        }
        return 0;
    }
    if (w_ieq_n(v, len, "fit-content(")) {
        start = 12; end = len;
        if (end <= start || v[--end] != ')') return -1;
        while (start < end && w_space(v[start])) start++;
        while (end > start && w_space(v[end - 1])) end--;
        if (parse_dimension(v + start, end - start, &number, &percent) || percent) return -1;
        *size = WE_TRACK_AUTO; *low = 0; *cap = (int16_t)number;
        return 0;
    }
    return track_token(v, len, size);
}

/* A list may mix bounded repeat groups and functional track tokens.
 * Parse into temporary arrays so a bad tail never changes the old list. */
typedef struct { int mode, at; } we_auto_repeat_t;

/* A definite track: fixed, or minmax with a fixed floor and a fr or fixed cap. */
static int definite_track(int size, int low, int cap) {
    if (size >= 0) return 1;
    return low >= 0 && (size != WE_TRACK_AUTO || cap >= 0);
}

static int parse_track_list(const char *v, int len, int16_t *sizes, int16_t *floors, int16_t *caps,
                            we_auto_repeat_t *automatic) {
    int i = 0, count = 0;
    while (i < len) {
        int first, nesting = 0, end, copies = 1, group_first = count, group_count, c;
        while (i < len && w_space(v[i])) i++;
        if (i == len) break;
        first = i;
        if (w_ieq_n(v + i, len - i, "repeat(")) {
            int mode = 0;
            i += 7; first = i;
            if (w_ieq_n(v + i, len - i, "auto-fill")) mode = WE_AUTO_FILL;
            else if (w_ieq_n(v + i, len - i, "auto-fit")) mode = WE_AUTO_FIT;
            if (mode) {
                if (!automatic || automatic->mode) return -1;
                i += mode == WE_AUTO_FILL ? 9 : 8;
            } else {
                while (i < len && v[i] >= '0' && v[i] <= '9') i++;
                if (bounded_uint(v + first, i - first, WE_TRACKS_MAX, &copies) || copies < 1) return -1;
            }
            while (i < len && w_space(v[i])) i++;
            if (i == len || v[i++] != ',') return -1;
            first = i; nesting = 1;
            while (i < len && nesting) { if (v[i] == '(') nesting++; if (v[i] == ')') nesting--; i++; }
            if (nesting) return -1;
            end = i - 1;
            /* Nested repeat is deliberately refused; functional track values are allowed. */
            { int j = first;
              while (j < end) {
                  int begin, depth = 0;
                  while (j < end && w_space(v[j])) j++;
                  if (j == end) break;
                  begin = j;
                  while (j < end && (depth || !w_space(v[j]))) {
                      if (v[j] == '(') depth++;
                      if (v[j] == ')') depth--;
                      if (depth < 0) return -1;
                      j++;
                  }
                  if (depth || count >= WE_TRACKS_MAX || track_value(v + begin, j - begin, &sizes[count], &floors[count], &caps[count])) return -1;
                  count++;
              }
            }
            group_count = count - group_first;
            if (!group_count || group_count * copies > WE_TRACKS_MAX - group_first) return -1;
            if (mode) {
                int at = first;
                while (at < end && w_space(v[at])) at++;
                /* fit-content() shares minmax(0, cap) storage but is not a fixed size. */
                if (w_ieq_n(v + at, end - at, "fit-content(")) return -1;
                if (group_count != 1 || !definite_track(sizes[group_first], floors[group_first], caps[group_first])) return -1;
                automatic->mode = mode;
                automatic->at = group_first;
            }
            for (c = 1; c < copies; c++) {
                int k;
                for (k = 0; k < group_count; k++, count++) {
                    sizes[count] = sizes[group_first + k];
                    floors[count] = floors[group_first + k]; caps[count] = caps[group_first + k];
                }
            }
        } else {
            while (i < len && (nesting || !w_space(v[i]))) {
                if (v[i] == '(') nesting++;
                if (v[i] == ')') nesting--;
                if (nesting < 0) return -1;
                i++;
            }
            if (nesting || count >= WE_TRACKS_MAX || track_value(v + first, i - first, &sizes[count], &floors[count], &caps[count])) return -1;
            count++;
        }
        if (i < len && !w_space(v[i])) return -1;
    }
    return count ? count : -1;
}

static int grid_name(const char *v, int len) {
    int i;
    if (len < 1 || len >= WE_GRID_NAME_MAX || !((v[0] >= 'a' && v[0] <= 'z') || (v[0] >= 'A' && v[0] <= 'Z') || v[0] == '_')) return 0;
    for (i = 1; i < len; i++) if (!w_namec(v[i])) return 0;
    return 1;
}

static int parse_areas(we_node_t *n, const char *v, int len) {
    we_grid_area_t areas[WE_GRID_AREAS_MAX];
    int seen[WE_GRID_AREAS_MAX], count = 0, row = 0, cols = -1, i = 0, k;
    w_zero(areas, sizeof(areas)); w_zero(seen, sizeof(seen));
    if (value_is(v, len, "none")) { n->area_count = n->area_rows = n->area_cols = 0; return 0; }
    while (i < len) {
        int col = 0, quote;
        while (i < len && w_space(v[i])) i++;
        if (i == len) break;
        if (row >= WE_TRACKS_MAX || (v[i] != '\'' && v[i] != '"')) return -1;
        quote = v[i++];
        while (i < len && v[i] != quote) {
            int start, end, dot = 1, found = -1;
            while (i < len && w_space(v[i])) i++;
            if (i == len || v[i] == quote) break;
            start = i;
            while (i < len && !w_space(v[i]) && v[i] != quote) i++;
            end = i;
            if (col >= WE_TRACKS_MAX) return -1;
            for (k = start; k < end; k++) if (v[k] != '.') dot = 0;
            if (!dot) {
                if (!grid_name(v + start, end - start)) return -1;
                for (k = 0; k < count; k++) if (w_len(areas[k].name) == end - start && sel_equal(v + start, areas[k].name, end - start, 0)) { found = k; break; }
                if (found < 0) {
                    if (count == WE_GRID_AREAS_MAX) return -1;
                    found = count++;
                    w_cpy(areas[found].name, WE_GRID_NAME_MAX, v + start, end - start);
                    areas[found].row = (uint8_t)row; areas[found].col = (uint8_t)col;
                }
                if (col < areas[found].col) return -1;
                areas[found].rows = (uint8_t)(row - areas[found].row + 1);
                if (col - areas[found].col + 1 > areas[found].cols) areas[found].cols = (uint8_t)(col - areas[found].col + 1);
                seen[found]++;
            }
            col++;
        }
        if (i == len || v[i++] != quote || !col || (cols >= 0 && col != cols)) return -1;
        cols = col; row++;
        if (i < len && !w_space(v[i])) return -1;
    }
    if (!row) return -1;
    for (k = 0; k < count; k++) if (seen[k] != areas[k].rows * areas[k].cols) return -1;
    for (k = 0; k < count; k++) n->areas[k] = areas[k];
    n->area_count = (uint8_t)count; n->area_cols = (uint8_t)cols; n->area_rows = (uint8_t)row;
    return 0;
}

static int parse_object_position(we_node_t *n, const char *v, int len) {
    int starts[4], lengths[4], count = tokens(v, len, starts, lengths), out[2] = {50, 50}, axis;
    if (count < 1 || count > 2) return -1;
    for (axis = 0; axis < count; axis++) {
        const char *p = v + starts[axis]; int l = lengths[axis];
        if (value_is(p, l, "center")) out[axis] = 50;
        else if (value_is(p, l, axis ? "top" : "left")) out[axis] = 0;
        else if (value_is(p, l, axis ? "bottom" : "right")) out[axis] = 100;
        else if (count == 1 && (value_is(p, l, "top") || value_is(p, l, "bottom"))) { out[1] = value_is(p, l, "top") ? 0 : 100; }
        else if (l > 1 && p[l - 1] == '%' && !bounded_uint(p, l - 1, 100, &out[axis])) { }
        else return -1;
    }
    n->object_x = (int16_t)out[0]; n->object_y = (int16_t)out[1];
    return 0;
}

static int bounded_signed(const char *v, int len, int max, int *out) {
    int sign = 1;
    if (len && v[0] == '+') { v++; len--; }
    else if (len && v[0] == '-') { sign = -1; v++; len--; }
    if (bounded_uint(v, len, max, out)) return -1;
    *out *= sign;
    return 0;
}

/* Lot 61: opacity as a percentage 0..100 from a number ("0.5", ".25",
 * "1") or a percentage ("40%"). Out-of-range values clamp (CSS Color 4);
 * more than two decimals are truncated. */
static int parse_opacity(const char *v, int len, int *out) {
    int at = 0, whole = 0, frac = 0, scale = 10, digits = 0, negative = 0, pct;
    if (len && (v[0] == '-' || v[0] == '+')) { negative = v[0] == '-'; at++; }
    pct = len > at && v[len - 1] == '%';
    if (pct) len--;
    while (at < len && v[at] >= '0' && v[at] <= '9') { if (whole < 1000) whole = whole * 10 + v[at] - '0'; at++; digits++; }
    if (at < len && v[at] == '.') {
        at++;
        if (at >= len) return -1;
        while (at < len && v[at] >= '0' && v[at] <= '9') {
            if (scale) { frac += (v[at] - '0') * scale; scale /= 10; }
            at++; digits++;
        }
    }
    if (!digits || at != len) return -1;
    whole = pct ? whole : whole * 100 + frac;
    if (negative) whole = 0;
    *out = whole > 100 ? 100 : whole;
    return 0;
}

static int line_identifier(const char *v, int len) {
    return grid_name(v, len) && !value_is(v, len, "auto") && !value_is(v, len, "span") &&
           !value_is(v, len, "none") && !value_is(v, len, "inherit") && !value_is(v, len, "initial") &&
           !value_is(v, len, "unset") && !value_is(v, len, "revert");
}

static int named_track_list(const char *v, int len, int16_t *sizes, int16_t *floors,
                            int16_t *caps, we_grid_names_t *names, we_auto_repeat_t *automatic) {
    int at = 0, count = 0, k;
    w_zero(names, sizeof(*names));
    automatic->mode = automatic->at = 0;
    while (at < len) {
        int first, depth = 0, end, added;
        while (at < len && w_space(v[at])) at++;
        if (at == len) break;
        if (v[at] == '[') {
            int aliases = 0;
            at++;
            while (at < len && v[at] != ']') {
                while (at < len && w_space(v[at])) at++;
                if (at == len || v[at] == ']') break;
                first = at;
                while (at < len && !w_space(v[at]) && v[at] != ']') at++;
                if (aliases == WE_GRID_LINE_ALIASES || !line_identifier(v + first, at - first) ||
                    names->name[count][aliases][0]) return -1;
                if (aliases && w_len(names->name[count][0]) == at - first &&
                    sel_equal(v + first, names->name[count][0], at - first, 0)) return -1;
                w_cpy(names->name[count][aliases++], WE_GRID_NAME_MAX, v + first, at - first);
            }
            if (!aliases || at == len || v[at++] != ']') return -1;
            continue;
        }
        if (count == WE_TRACKS_MAX) return -1;
        first = at;
        while (at < len && (depth || (!w_space(v[at]) && v[at] != '['))) {
            if (v[at] == '[' || v[at] == ']') return -1;
            if (v[at] == '(') depth++;
            if (v[at] == ')') depth--;
            if (depth < 0) return -1;
            at++;
        }
        end = at;
        if (depth || end == first) return -1;
        {
            int16_t group[WE_TRACKS_MAX], low[WE_TRACKS_MAX], high[WE_TRACKS_MAX];
            we_auto_repeat_t local;
            local.mode = local.at = 0;
            added = parse_track_list(v + first, end - first, group, low, high, automatic->mode ? 0 : &local);
            if (added < 1 || count + added > WE_TRACKS_MAX) return -1;
            if (local.mode) { automatic->mode = local.mode; automatic->at = count + local.at; }
            for (k = 0; k < added; k++, count++) {
                sizes[count] = group[k]; floors[count] = low[k]; caps[count] = high[k];
            }
        }
    }
    /* Every other track needs a definite size so the count is defined. */
    if (automatic->mode)
        for (k = 0; k < count; k++)
            if (k != automatic->at && !definite_track(sizes[k], floors[k], caps[k])) return -1;
    return count ? count : -1;
}

/* One edge: auto | N | name | N name | name N | span N | span name |
 * span N name. A name may carry an occurrence; negative counts from the end. */
static int grid_edge(const char *v, int len, we_grid_request_t *r, int side) {
    int a[4], l[4], count = tokens(v, len, a, l), k, first = 0, number = 0, numbers = 0, name = -1;
    char *out = r->edge[side];
    out[0] = 0; r->span[side] = 0; r->nth[side] = 0; r->named_span[side] = 0; r->bare[side] = 0;
    if (count < 1 || count > 3) return -1;
    if (count == 1 && value_is(v + a[0], l[0], "auto")) return 0;
    if (value_is(v + a[0], l[0], "span")) first = 1;
    if (first && count == 1) return -1;
    for (k = first; k < count; k++) {
        const char *t = v + a[k];
        if ((t[0] >= '0' && t[0] <= '9') || t[0] == '-' || t[0] == '+') {
            if (numbers++ || bounded_signed(t, l[k], WE_TRACKS_MAX + 1, &number) || !number) return -1;
        } else {
            if (name >= 0 || !line_identifier(t, l[k])) return -1;
            name = k;
        }
    }
    if (first) {
        if (numbers && (number < 1 || number > WE_TRACKS_MAX)) return -1;
        r->span[side] = (uint8_t)(numbers ? number : 1);
        if (name >= 0) { r->named_span[side] = 1; w_cpy(out, WE_GRID_NAME_MAX, v + a[name], l[name]); }
        return 0;
    }
    if (count > 2) return -1;
    if (name < 0) { w_cpy(out, WE_GRID_NAME_MAX, v + a[0], l[0]); return 0; }
    w_cpy(out, WE_GRID_NAME_MAX, v + a[name], l[name]);
    r->nth[side] = (int8_t)(numbers ? number : 1);
    r->bare[side] = (uint8_t)!numbers;
    return 0;
}

static int grid_request(we_node_t *n, int axis, const char *v, int len) {
    we_grid_request_t request;
    uint8_t start, span;
    int split = 0, a, b;
    if (!parse_placement(v, len, &start, &span)) {
        n->grid_request[axis].active = 0;
        n->grid_request[axis].plain_start = start;
        n->grid_request[axis].plain_span = span;
        if (axis) { n->grid_row_start = start; n->grid_row_span = span; }
        else { n->grid_col_start = start; n->grid_col_span = span; }
        n->grid_area[0] = 0;
        return 0;
    }
    w_zero(&request, sizeof(request));
    while (split < len && v[split] != '/') split++;
    if (grid_edge(v, split, &request, 0)) return -1;
    if (split < len && grid_edge(v + split + 1, len - split - 1, &request, 1)) return -1;
    /* A lone <custom-ident> also names the end edge (CSS Grid 8.4). */
    if (split >= len && request.bare[0]) {
        w_cpy(request.edge[1], sizeof(request.edge[1]), request.edge[0], w_len(request.edge[0]));
        request.nth[1] = request.nth[0];
        request.bare[1] = 1;
    }
    if (request.span[0] && (request.span[1] || !request.edge[1][0])) return -1;
    if (!request.nth[0] && !request.span[0] &&
        !bounded_signed(request.edge[0], w_len(request.edge[0]), WE_TRACKS_MAX + 1, &a) && a > 0) {
        if (a > WE_TRACKS_MAX) return -1;
        if (request.span[1] && !request.named_span[1] && a + request.span[1] - 1 > WE_TRACKS_MAX) return -1;
        if (!request.nth[1] && !request.span[1] &&
            !bounded_signed(request.edge[1], w_len(request.edge[1]), WE_TRACKS_MAX + 1, &b) && b > 0 && b <= a) return -1;
    }
    request.active = 1;
    n->grid_request[axis] = request;
    n->grid_area[0] = 0;
    return 0;
}

static int grid_line_count(const we_node_t *g, int axis) {
    int tracks = axis ? g->grid_rows : g->grid_cols, areas = axis ? g->area_rows : g->area_cols;
    if (areas > tracks) tracks = areas;
    if (tracks > WE_TRACKS_MAX) tracks = WE_TRACKS_MAX;
    return tracks + 1;
}

/* Explicit names and the implicit <area>-start / <area>-end aliases. */
/* Explicit names follow the template; lines inside an expanded automatic
 * repetition carry no names, lines after it shift by the extra copies. */
static int template_line(const we_node_t *g, int axis, int line) {
    int at = g->auto_at[axis], copies = g->auto_count[axis];
    if (!g->auto_mode[axis] || copies <= 1 || line <= at + 1) return line;
    if (line >= at + 1 + copies) return line - copies + 1;
    return 0;
}

static int line_has_name(const we_node_t *g, int axis, int line, const char *name) {
    int alias, k, len = w_len(name), t = template_line(g, axis, line);
    for (alias = 0; t > 0 && alias < WE_GRID_LINE_ALIASES; alias++)
        if (w_eq(name, g->line_names[axis].name[t - 1][alias])) return 1;
    for (k = 0; k < g->area_count; k++) {
        const we_grid_area_t *area = &g->areas[k];
        int alen = w_len(area->name), first = (axis ? area->row : area->col) + 1;
        int last = first + (axis ? area->rows : area->cols);
        if (len <= alen || !sel_equal(name, area->name, alen, 0)) continue;
        if (w_eq(name + alen, "-start") && line == first) return 1;
        if (w_eq(name + alen, "-end") && line == last) return 1;
    }
    return 0;
}

/* nth > 0 searches forward after line `after`; nth < 0 searches backward
 * before line `before`. Missing occurrences are not invented. */
static int nth_named_line(const we_node_t *g, int axis, const char *name, int nth, int after, int before) {
    int lines = grid_line_count(g, axis), line, seen = 0;
    if (nth > 0) {
        for (line = after + 1; line <= lines; line++)
            if (line_has_name(g, axis, line, name) && ++seen == nth) return line;
    } else if (nth < 0) {
        if (before > lines + 1) before = lines + 1;
        for (line = before - 1; line >= 1; line--)
            if (line_has_name(g, axis, line, name) && ++seen == -nth) return line;
    }
    return -1;
}

static int edge_line(const we_node_t *g, int axis, const we_grid_request_t *r, int side) {
    const char *edge = r->edge[side];
    int number, count = axis ? g->grid_rows : g->grid_cols;
    if (r->span[side] || !edge[0]) return 0;
    if (!r->nth[side]) {
        if (bounded_signed(edge, w_len(edge), WE_TRACKS_MAX + 1, &number)) return -1;
        if (number < 0) number = count + 2 + number;
        return number >= 1 && number <= WE_TRACKS_MAX + 1 ? number : -1;
    }
    if (r->bare[side]) {
        char alias[WE_GRID_NAME_MAX + 8];
        int len = w_len(edge), line;
        w_cpy(alias, sizeof(alias), edge, len);
        w_cpy(alias + len, sizeof(alias) - len, side ? "-end" : "-start", side ? 4 : 6);
        line = nth_named_line(g, axis, alias, 1, 0, 0);
        if (line > 0) return line;
    }
    return nth_named_line(g, axis, edge, r->nth[side], 0, WE_TRACKS_MAX + 2);
}

static void resolve_requests(we_doc_t *d, int idx, int report) {
    we_node_t *n = &d->nodes[idx];
    const we_node_t *g;
    int axis;
    if (n->parent < 0 || d->nodes[n->parent].display != WE_DISP_GRID) return;
    g = &d->nodes[n->parent];
    for (axis = 0; axis < 2; axis++) {
        const we_grid_request_t *request = &n->grid_request[axis];
        int start, end, span;
        if (!request->active) continue;
        start = edge_line(g, axis, request, 0);
        end = edge_line(g, axis, request, 1);
        if (start > 0 && request->named_span[1])
            end = nth_named_line(g, axis, request->edge[1], request->span[1], start, 0);
        else if (end > 0 && request->named_span[0])
            start = nth_named_line(g, axis, request->edge[0], -request->span[0], 0, end);
        else if (start >= 0 && end >= 0) {
            if (!start && end) start = end - (request->span[0] ? request->span[0] : 1);
            if (!end && start) end = start + (request->span[1] ? request->span[1] : 1);
        }
        if (start > 0 && end == start) end = start + 1;
        span = start > 0 && end > 0 ? end - start : request->span[1] && !request->named_span[1] ? request->span[1] : 1;
        if (start < 0 || end < 0 || start > WE_TRACKS_MAX || end > WE_TRACKS_MAX + 1 || span < 1 || span > WE_TRACKS_MAX) {
            if (report) diag(d, "grid line unresolved", idx);
            start = 0; span = 1;
        }
        if (axis) { n->grid_row_start = (uint8_t)start; n->grid_row_span = (uint8_t)span; }
        else { n->grid_col_start = (uint8_t)start; n->grid_col_span = (uint8_t)span; }
    }
}

static uint8_t subgrid_ok[WE_MAX_NODES];
static int subgrid_on(int s, int axis) { return s >= 0 && ((subgrid_ok[s] >> axis) & 1); }

/* Items of a subgrid resolve against inherited names: report later. */
static void resolve_grid_requests(we_doc_t *d) {
    uint32_t i;
    for (i = 0; i < d->nnodes; i++) {
        int p = d->nodes[i].parent;
        resolve_requests(d, (int)i, !(p >= 0 && subgrid_ok[p]));
    }
}

/* Lot 61 subgrid, bounded: one level, a grid parent, no box offsets on
 * the subgridded axis, no line-name list. Refusals fall back to none. */
static void subgrid_validate(we_doc_t *d) {
    uint32_t i;
    for (i = 0; i < d->nnodes; i++) {
        we_node_t *n = &d->nodes[i];
        int axis, p = n->parent, k;
        subgrid_ok[i] = 0;
        for (axis = 0; axis < 2; axis++) {
            if (!n->subgrid[axis]) continue;
            w_zero(&n->line_names[axis], sizeof(n->line_names[axis]));
            for (k = 0; k < WE_TRACKS_MAX; k++) {
                if (axis) { n->row_size[k] = WE_TRACK_AUTO; n->row_floor[k] = n->row_cap[k] = -1; }
                else { n->col_size[k] = -1; n->col_floor[k] = n->col_cap[k] = -1; }
            }
            if (axis) n->grid_rows = 0; else n->grid_cols = 0;
        }
        n->sub_gap[0] = n->sub_gap[1] = 0;
        if (!n->subgrid[0] && !n->subgrid[1]) continue;
        if (n->display != WE_DISP_GRID || n->absolute || p < 0 || d->nodes[p].display != WE_DISP_GRID) {
            diag(d, "subgrid requires grid parent", (int)i);
            continue;
        }
        if (d->nodes[p].subgrid[0] || d->nodes[p].subgrid[1]) {
            diag(d, "nested subgrid refused", (int)i);
            continue;
        }
        for (axis = 0; axis < 2; axis++) {
            int a = axis ? 0 : 3, b = axis ? 2 : 1;
            if (!n->subgrid[axis]) continue;
            if (n->border || n->padding[a] || n->padding[b] || n->margin[a] || n->margin[b] ||
                (n->margin_auto & (axis ? (WE_AUTO_TOP | WE_AUTO_BOTTOM) : (WE_AUTO_LEFT | WE_AUTO_RIGHT)))) {
                diag(d, "subgrid box offsets refused", (int)i);
                continue;
            }
            subgrid_ok[i] |= (uint8_t)(1 << axis);
        }
    }
}

static void name_copy(char *dst, const char *a, const char *b) {
    int la = w_len(a), lb = w_len(b);
    if (la + lb >= WE_GRID_NAME_MAX) return;
    w_cpy(dst, WE_GRID_NAME_MAX, a, la);
    w_cpy(dst + la, WE_GRID_NAME_MAX - la, b, lb);
}

/* The subgrid takes span tracks of the parent from `start` (0-based): track
 * count, auto placeholders, explicit names and area aliases of its lines.
 * Its items are re-resolved and clamped into the inherited tracks. */
static void subgrid_bind(we_doc_t *d, int p, int s, int axis, int start, int span) {
    const we_node_t *g = &d->nodes[p];
    we_node_t *n = &d->nodes[s];
    int line, k;
    if (span < 1) span = 1;
    if (start + span > WE_TRACKS_MAX) span = WE_TRACKS_MAX - start;
    if (span < 1) return;
    w_zero(&n->line_names[axis], sizeof(n->line_names[axis]));
    for (line = 0; line <= span; line++) {
        int at = start + line + 1, t = template_line(g, axis, at), alias = 0, a;
        char *slot;
        for (a = 0; t > 0 && a < WE_GRID_LINE_ALIASES; a++)
            if (g->line_names[axis].name[t - 1][a][0] && alias < WE_GRID_LINE_ALIASES) {
                slot = n->line_names[axis].name[line][alias++];
                name_copy(slot, g->line_names[axis].name[t - 1][a], "");
            }
        for (k = 0; k < g->area_count && alias < WE_GRID_LINE_ALIASES; k++) {
            const we_grid_area_t *area = &g->areas[k];
            int first = (axis ? area->row : area->col) + 1, last = first + (axis ? area->rows : area->cols);
            slot = n->line_names[axis].name[line][alias];
            if (at == first) { name_copy(slot, area->name, "-start"); if (slot[0]) alias++; }
            slot = n->line_names[axis].name[line][alias < WE_GRID_LINE_ALIASES ? alias : 0];
            if (at == last && alias < WE_GRID_LINE_ALIASES) { name_copy(slot, area->name, "-end"); if (slot[0]) alias++; }
        }
    }
    if (axis) n->grid_rows = (uint8_t)span; else n->grid_cols = (uint8_t)span;
}

static void subgrid_items(we_doc_t *d, int s, int report) {
    int c, axis;
    for (c = d->nodes[s].first; c >= 0; c = d->nodes[c].next) {
        we_node_t *child = &d->nodes[c];
        if (child->is_text) continue;
        resolve_requests(d, c, report);
        for (axis = 0; axis < 2; axis++) {
            int span = axis ? d->nodes[s].grid_rows : d->nodes[s].grid_cols;
            uint8_t *first = axis ? &child->grid_row_start : &child->grid_col_start;
            uint8_t *count = axis ? &child->grid_row_span : &child->grid_col_span;
            const we_grid_request_t *plain = &child->grid_request[axis];
            if (!subgrid_on(s, axis) || span < 1) continue;
            /* Rebind from the authored value: an earlier, narrower bind
             * clamped it before auto-repeat expanded the parent. */
            if (!plain->active && plain->plain_span) { *first = plain->plain_start; *count = plain->plain_span; }
            if (*count > span) *count = (uint8_t)span;
            if (*first && *first + *count - 1 > span) *first = (uint8_t)(span - *count + 1);
        }
    }
}

static int own_axis_gap(const we_node_t *n, int vertical);
static int w_max(int a, int b) { return a > b ? a : b; }

/* Gap difference of a subgrid on an axis: its own declared gap minus the
 * parent's, split h1 (after a track) and h2 (before it). */
static int subgrid_gap_delta(const we_node_t *s, int axis, int gap, int *h1, int *h2) {
    int delta = (s->gap_set >> axis) & 1 ? own_axis_gap(s, axis) - gap : 0;
    *h1 = delta / 2;
    *h2 = delta - *h1;
    return delta;
}

/* Extra margin of a subgrid item next to the subgrid's interior lines
 * (CSS Grid 2, 9.3): h2 before its first track, h1 after its last. */
static int subgrid_item_extra(const we_node_t *s, int axis, int first, int span, int gap) {
    int h1, h2, count = axis ? s->grid_rows : s->grid_cols, extra = 0;
    (void)subgrid_gap_delta(s, axis, gap, &h1, &h2);
    if (first > 0) extra += h2;
    if (first + span < count) extra += h1;
    return extra;
}

/* Lot 61: the subgrid's tracks follow the parent's real line positions
 * (pos), so the parent's content distribution reaches the subgrid. One
 * uniform subgrid gutter G is the parent's average effective gutter plus
 * the own-gap difference; track sizes then put every interior line at the
 * parent's line shifted by h2, and every track end h1 before it. */
static void subgrid_tracks(we_node_t *n, int axis, const int *sizes, const int *pos, int start, int gap) {
    int k, count = axis ? n->grid_rows : n->grid_cols, h1, h2, delta, step = gap, base = pos[start];
    delta = subgrid_gap_delta(n, axis, gap, &h1, &h2);
    if (count > 1) {
        int inner = pos[start + count - 1] - base;
        for (k = 0; k < count - 1; k++) inner -= sizes[start + k];
        step = inner / (count - 1) + delta;
        if (step < 0) step = 0;
    }
    for (k = 0; k < count; k++) {
        int left = pos[start + k] - base + (k > 0 ? h2 : 0), size;
        if (k + 1 < count) size = pos[start + k + 1] - base + h2 - step - left;
        else size = pos[start + k] + sizes[start + k] - base - left;
        if (size < 0) size = 0;
        if (axis) { n->row_size[k] = (int16_t)size; n->row_floor[k] = n->row_cap[k] = -1; }
        else { n->col_size[k] = (int16_t)size; n->col_floor[k] = n->col_cap[k] = -1; }
    }
    n->sub_gap[axis] = (int16_t)(step + 1);
}

/* Expand a valid margin into top,right,bottom,left and replace all auto bits. */
static int tf_angle(const char *v, int len, int *degrees);
static int tf_length(const char *v, int len, int16_t *out, int *percent);

/* ---------- Lot 62: border-radius ----------
 * border-radius: 1 to 4 radii, optionally "/" and 1 to 4 vertical radii;
 * the longhands take one or two. px (at most 128) or percent (at most
 * 100). Painted as a staircase of WE_RADIUS_STEPS rows per corner. */
#define WE_RADIUS_MAX 128
#define WE_RADIUS_STEPS 3
#define WE_RADIUS_SLICES (4 * WE_RADIUS_STEPS + 2)
static int radius_value(const char *v, int len, int *value, int *pct) {
    int16_t px;
    int percent;
    if (tf_length(v, len, &px, &percent) || px < 0) return -1;
    if (percent ? px > 100 : px > WE_RADIUS_MAX) return -1;
    *value = px; *pct = percent;
    return 0;
}

static int radius_list(const char *v, int len, int *values, int *pcts) {
    static const int8_t from[4][4] = { {0, 0, 0, 0}, {0, 1, 0, 1}, {0, 1, 2, 1}, {0, 1, 2, 3} };
    int a[4], l[4], c = tokens(v, len, a, l), k, val[4], pc[4];
    if (c < 1 || c > 4) return -1;
    for (k = a[c - 1] + l[c - 1]; k < len; k++) if (!w_space(v[k])) return -1;   /* a fifth value */
    for (k = 0; k < c; k++) if (radius_value(v + a[k], l[k], &val[k], &pc[k])) return -1;
    for (k = 0; k < 4; k++) { values[k] = val[from[c - 1][k]]; pcts[k] = pc[from[c - 1][k]]; }
    return 0;
}

static void radius_store(we_node_t *n, int corner, int x, int xp, int y, int yp) {
    n->rad_x[corner] = (uint8_t)x; n->rad_y[corner] = (uint8_t)y;
    n->rad_pct = (uint8_t)(n->rad_pct & ~((1u << corner) | (1u << (corner + 4))));
    if (xp) n->rad_pct |= (uint8_t)(1u << corner);
    if (yp) n->rad_pct |= (uint8_t)(1u << (corner + 4));
}

static int parse_radius(we_node_t *n, const char *v, int len) {
    int slash = -1, k, x[4], xp[4], y[4], yp[4];
    for (k = 0; k < len; k++) if (v[k] == '/') { if (slash >= 0) return -1; slash = k; }
    if (slash < 0) {
        if (radius_list(v, len, x, xp)) return -1;
        for (k = 0; k < 4; k++) { y[k] = x[k]; yp[k] = xp[k]; }
    } else if (radius_list(v, slash, x, xp) || radius_list(v + slash + 1, len - slash - 1, y, yp)) return -1;
    for (k = 0; k < 4; k++) radius_store(n, k, x[k], xp[k], y[k], yp[k]);
    return 0;
}

static int parse_corner(we_node_t *n, int corner, const char *v, int len) {
    int a[4], l[4], c = tokens(v, len, a, l), x, xp, y, yp;
    if (c < 1 || c > 2 || radius_value(v + a[0], l[0], &x, &xp)) return -1;
    if (c == 1) { y = x; yp = xp; }
    else if (radius_value(v + a[1], l[1], &y, &yp)) return -1;
    radius_store(n, corner, x, xp, y, yp);
    return 0;
}


/* Lot 62: split a functional value's arguments at top-level commas. */
static int comma_args(const char *v, int len, int *start, int *length, int cap) {
    int i, depth = 0, count = 0, at = 0;
    for (i = 0; i <= len; i++) {
        if (i < len && v[i] == '(') depth++;
        else if (i < len && v[i] == ')') depth--;
        else if (i == len || (v[i] == ',' && depth == 0)) {
            int a = at, b = i;
            while (a < b && w_space(v[a])) a++;
            while (b > a && w_space(v[b - 1])) b--;
            if (count >= cap || b == a) return -1;
            start[count] = a; length[count] = b - a; count++;
            at = i + 1;
        }
    }
    return depth ? -1 : count;
}

/* Lot 62: linear-gradient([to side | angle,] color [pct], color [pct]...).
 * Angles must be a multiple of 90deg (or 0.25turn); corners and length
 * positions are refused. Positions clamp up to the previous stop; missing
 * ones spread evenly between their neighbours. */
/* Lot 62 round 2: the radial-gradient shape argument: circle or ellipse,
 * an extent keyword, "at center"; 1 when it is one, -1 when refused. */
/* Lot 62/63: [circle|ellipse] [extent keyword | 1 or 2 sizes] [at position].
 * Returns 1 when the first argument is the shape, 0 for a colour stop. */
static int tf_length(const char *v, int len, int16_t *out, int *percent);
static int position_pair(const char *v, int len, int16_t out[2], uint8_t *pct_bits);
static int radial_shape(we_node_t *n, const char *a, int l, int *shape) {
    static const char *const ext[4] = { "farthest-corner", "closest-side", "farthest-side", "closest-corner" };
    int i = 0, any = 0, form = -1, extent = -1, sizes = 0, j;
    n->grad_at[0] = n->grad_at[1] = 50;
    n->grad_at_pct = 3;
    n->grad_size[0] = n->grad_size[1] = -1;
    while (1) {
        int b, pct;
        int16_t size;
        while (i < l && w_space(a[i])) i++;
        if (i >= l) break;
        b = i;
        while (i < l && !w_space(a[i])) i++;
        if (value_is(a + b, i - b, "at")) {
            uint8_t bits;
            if (position_pair(a + i, l - i, n->grad_at, &bits)) return -1;
            n->grad_at_pct = (uint8_t)((n->grad_at_pct & 12) | bits);
            any = 1;
            break;
        }
        if (value_is(a + b, i - b, "circle") || value_is(a + b, i - b, "ellipse")) {
            if (form >= 0) return -1;
            form = value_is(a + b, i - b, "circle");
        } else {
            for (j = 0; j < 4 && !value_is(a + b, i - b, ext[j]); j++) {}
            if (j < 4 && extent < 0 && !sizes) extent = j;
            else if (j == 4 && sizes < 2 && extent < 0 && tf_length(a + b, i - b, &size, &pct) == 0 && size >= 0) {
                n->grad_size[sizes] = size;
                if (pct) n->grad_at_pct |= (uint8_t)(4 << sizes);
                sizes++;
            } else return any ? -1 : 0;
        }
        any = 1;
    }
    if (form < 0) form = sizes == 1;
    /* a circle takes one length, an ellipse two sizes */
    if ((form == 1 && (sizes == 2 || (n->grad_at_pct & 4))) || (form == 0 && sizes == 1)) return -1;
    if (sizes == 1) n->grad_size[1] = n->grad_size[0];
    *shape = form | ((extent < 0 ? WE_EXT_FARTHEST_CORNER : extent) << 1);
    return any;
}

/* text-align keywords; other values give fallback. Lot 64: start and end
 * also set *logical (1, 2) for the resolution against direction. */
static int align_keyword(const char *v, int len, int fallback, int *logical) {
    *logical = value_is(v, len, "start") ? 1 : value_is(v, len, "end") ? 2 : 0;
    if (value_is(v, len, "left") || *logical == 1) return WE_ALIGN_LEFT;
    if (value_is(v, len, "center")) return WE_ALIGN_CENTER;
    if (value_is(v, len, "right") || *logical == 2) return WE_ALIGN_RIGHT;
    if (value_is(v, len, "justify")) return WE_ALIGN_JUSTIFY;
    return fallback;
}

static int parse_gradient(we_node_t *n, const char *v, int len) {
    int start[WE_GRAD_STOPS + 1], length[WE_GRAD_STOPS + 1], count, k, first = 0, dir = WE_GRAD_BOTTOM;
    int pos[WE_GRAD_STOPS], last, radial, shape = 0;
    uint32_t color[WE_GRAD_STOPS];
    if (len < 17 || v[len - 1] != ')') return -1;
    radial = w_ieq_n(v, 16, "radial-gradient(");
    if (!radial && !w_ieq_n(v, 16, "linear-gradient(")) return -1;
    count = comma_args(v + 16, len - 17, start, length, WE_GRAD_STOPS + 1);
    if (count < 2) return -1;
    if (radial) {
        int got = radial_shape(n, v + 16 + start[0], length[0], &shape);
        if (got < 0) return -1;
        first = got;
        dir = WE_GRAD_RADIAL;
    } else {
        const char *a = v + 16 + start[0];
        int l = length[0];
        int degrees;
        if (l > 3 && w_ieq_n(a, 3, "to ")) {
            int at = 3;
            while (at < l && w_space(a[at])) at++;
            if (value_is(a + at, l - at, "bottom")) dir = WE_GRAD_BOTTOM;
            else if (value_is(a + at, l - at, "right")) dir = WE_GRAD_RIGHT;
            else if (value_is(a + at, l - at, "top")) dir = WE_GRAD_TOP;
            else if (value_is(a + at, l - at, "left")) dir = WE_GRAD_LEFT;
            else return -1;
            first = 1;
        } else if (tf_angle(a, l, &degrees) == 0) {
            static const uint8_t by_quarter[4] = { WE_GRAD_TOP, WE_GRAD_RIGHT, WE_GRAD_BOTTOM, WE_GRAD_LEFT };
            if (degrees % 90) return -1;
            dir = by_quarter[((degrees / 90 % 4) + 4) % 4];
            first = 1;
        } else if (l > 0 && ((a[0] >= '0' && a[0] <= '9') || a[0] == '-')) return -1;
    }
    if (count - first < 2 || count - first > WE_GRAD_STOPS) return -1;
    for (k = 0; k < count - first; k++) {
        const char *a = v + 16 + start[first + k];
        int l = length[first + k], t[4], tl[4], c = tokens(a, l, t, tl), tenths;
        pos[k] = -1;
        if (c < 1 || c > 2 || parse_color(a + t[0], tl[0], &color[k])) return -1;
        if (c == 2) {
            if (spacing_percent(a + t[1], tl[1], 1, &tenths) || tenths > 1000) return -1;
            pos[k] = tenths < 0 ? 0 : tenths * 10;
        }
    }
    count -= first;
    if (pos[0] < 0) pos[0] = 0;
    if (pos[count - 1] < 0) pos[count - 1] = 10000;
    last = 0;
    for (k = 1; k < count; k++) {
        if (pos[k] < 0) continue;
        if (pos[k] < pos[last]) pos[k] = pos[last];
        if (k - last > 1) {
            int j;
            for (j = last + 1; j < k; j++) pos[j] = pos[last] + (pos[k] - pos[last]) * (j - last) / (k - last);
        }
        last = k;
    }
    n->grad_dir = (uint8_t)dir;
    n->grad_shape = (uint8_t)shape;
    n->grad_count = (uint8_t)count;
    for (k = 0; k < count; k++) { n->grad_pos[k] = (uint16_t)pos[k]; n->grad_color[k] = color[k]; }
    return 0;
}

/* Lot 62: list-style-type keywords (none included). */
static int list_keyword(const char *v, int len) {
    static const char *const names[] = { "none", "disc", "decimal", "square", "circle", "lower-alpha", "upper-alpha",
                                         "lower-roman", "upper-roman", "decimal-leading-zero", "lower-latin", "upper-latin" };
    static const uint8_t codes[] = { WE_LIST_NONE, WE_LIST_DISC, WE_LIST_DECIMAL, WE_LIST_SQUARE, WE_LIST_CIRCLE,
                                     WE_LIST_LOWER_ALPHA, WE_LIST_UPPER_ALPHA, WE_LIST_LOWER_ROMAN, WE_LIST_UPPER_ROMAN,
                                     WE_LIST_LEADING_ZERO, WE_LIST_LOWER_ALPHA, WE_LIST_UPPER_ALPHA };
    int k;
    for (k = 0; k < 12; k++) if (value_is(v, len, names[k])) return codes[k];
    return -1;
}

static int margin_auto_box(we_node_t *n, const char *v, int vl) {
    static const int8_t from[4][4] = { {0, 0, 0, 0}, {0, 1, 0, 1}, {0, 1, 2, 1}, {0, 1, 2, 3} };
    static const uint8_t flags[4] = {WE_AUTO_TOP, WE_AUTO_RIGHT, WE_AUTO_BOTTOM, WE_AUTO_LEFT};
    int a[4], l[4], c = tokens(v, vl, a, l), k, px;
    int vals[4], autos[4], pcts[4];
    if (c < 1 || c > 4) return 0;
    for (k = 0; k < c; k++) {
        pcts[k] = WE_NO_PCT;
        if (value_is(v + a[k], l[k], "auto")) { autos[k] = 1; vals[k] = 0; }
        else if (spacing_percent(v + a[k], l[k], 1, &pcts[k]) == 0) { autos[k] = 0; vals[k] = 0; }
        else if (parse_len(v + a[k], l[k], &px, 0) == 0) { autos[k] = 0; vals[k] = px; pcts[k] = WE_NO_PCT; }
        else return 0;
    }
    n->margin_auto = 0;
    for (k = 0; k < 4; k++) {
        int f = from[c - 1][k];
        n->margin[k] = (int16_t)vals[f];
        spacing_set(n, 0, k, pcts[f]);
        if (autos[f]) n->margin_auto |= flags[k];
    }
    return 1;
}

/* Lot 64: the physical side (0 top, 1 right, 2 bottom, 3 left) of a
 * logical edge under the element's writing-mode and direction. */
static int logical_side(const we_node_t *n, int block, int end) {
    int side;
    if (block) side = n->wmode == WE_WM_VERTICAL_RL ? 1 : n->wmode == WE_WM_VERTICAL_LR ? 3 : 0;
    else if (n->wmode != WE_WM_HORIZONTAL) side = n->dir_rtl ? 2 : 0;
    else side = n->dir_rtl ? 1 : 3;
    return end ? (side + 2) & 3 : side;
}

/* Lot 64: the physical corner radius property between a block side and an
 * inline side (perpendicular sides 0 top, 1 right, 2 bottom, 3 left). */
static const char *corner_property(int block, int inline_side) {
    int top = block == 0 || inline_side == 0, left = block == 3 || inline_side == 3;
    return top ? (left ? "border-top-left-radius" : "border-top-right-radius") :
                 (left ? "border-bottom-left-radius" : "border-bottom-right-radius");
}

static const char *logical_property(const we_node_t *n, const char *prop) {
    static const char *const sized[] = {"inline-size", "block-size", "min-inline-size", "max-inline-size",
                                        "min-block-size", "max-block-size"};
    static const char *const width_height[] = {"width", "height", "min-width", "max-width", "min-height", "max-height"};
    static const char *const prefixes[] = {"margin-", "padding-", "inset-"};
    static const char *const sides[3][4] = {
        {"margin-top", "margin-right", "margin-bottom", "margin-left"},
        {"padding-top", "padding-right", "padding-bottom", "padding-left"},
        {"top", "right", "bottom", "left"}};
    static const char *const edges[] = {"inline-start", "inline-end", "block-start", "block-end"};
    static const uint8_t swapped[6] = {1, 0, 4, 5, 2, 3};
    /* lot 64: logical borders and corner radii */
    static const char *const edge_props[4][4] = {
        {"border-top", "border-top-width", "border-top-style", "border-top-color"},
        {"border-right", "border-right-width", "border-right-style", "border-right-color"},
        {"border-bottom", "border-bottom-width", "border-bottom-style", "border-bottom-color"},
        {"border-left", "border-left-width", "border-left-style", "border-left-color"}};
    static const char *const suffixes[4] = {"", "-width", "-style", "-color"};
    static const char *const corners[] = {"border-start-start-radius", "border-start-end-radius",
                                          "border-end-start-radius", "border-end-end-radius"};
    int i, k, vertical = n->wmode != WE_WM_HORIZONTAL;
    if (w_eq(prop, "word-wrap")) return "overflow-wrap";
    if (w_ieq_n(prop, 7, "border-")) {
        for (k = 0; k < 4; k++) {
            int len = 7 + w_len(edges[k]);
            if (!w_ieq_n(prop + 7, w_len(edges[k]), edges[k])) continue;
            for (i = 0; i < 4; i++)
                if (w_eq(prop + len, suffixes[i])) return edge_props[logical_side(n, k >= 2, k & 1)][i];
        }
        for (k = 0; k < 4; k++)
            if (w_eq(prop, corners[k])) return corner_property(logical_side(n, 1, k >> 1), logical_side(n, 0, k & 1));
    }
    for (i = 0; i < 6; i++)
        if (w_eq(prop, sized[i])) return width_height[vertical ? swapped[i] : i];
    for (i = 0; i < 3; i++) {
        int len = w_len(prefixes[i]);
        if (!w_ieq_n(prop, len, prefixes[i])) continue;
        for (k = 0; k < 4; k++)
            if (w_eq(prop + len, edges[k])) return sides[i][logical_side(n, k >= 2, k & 1)];
    }
    return prop;
}

static int functional_tokens(const char *v, int len, int *starts, int *lengths, int cap) {
    int at = 0, count = 0;
    while (at < len) {
        int depth = 0, first;
        while (at < len && w_space(v[at])) at++;
        if (at == len) break;
        if (count == cap) return -1;
        first = at;
        while (at < len && (depth || !w_space(v[at]))) {
            if (v[at] == '(') depth++;
            if (v[at] == ')') depth--;
            if (depth < 0 || depth > WE_MATH_DEPTH_MAX) return -1;
            at++;
        }
        if (depth) return -1;
        starts[count] = first; lengths[count++] = at - first;
    }
    return count;
}

static int gap_declaration(we_node_t *n, const char *v, int len) {
    int starts[2], lengths[2], values[2], count = functional_tokens(v, len, starts, lengths, 2), i, percent;
    we_length_t expr[2];
    if (count < 1) return -1;
    w_zero(expr, sizeof(expr));
    for (i = 0; i < count; i++) {
        if (math_function(v + starts[i], lengths[i])) {
            if (parse_math(v + starts[i], lengths[i], &expr[i]) || expr[i].percent) return -1;
            if (math_eval(&expr[i], 0, &values[i])) return -1;
            if (values[i] < 0) values[i] = 0;
            if (values[i] > WE_LAYOUT_MAX) values[i] = WE_LAYOUT_MAX;
        } else if (parse_dimension(v + starts[i], lengths[i], &values[i], &percent) || percent) return -1;
    }
    if (count == 1) { values[1] = values[0]; expr[1] = expr[0]; }
    n->length[WE_LEN_ROW_GAP] = expr[0]; n->length[WE_LEN_COL_GAP] = expr[1];
    n->gap = (int16_t)values[0];
    n->gap_set = 3;
    n->row_gap = count == 1 && !expr[0].count ? -1 : (int16_t)values[0];
    n->col_gap = count == 1 && !expr[1].count ? -1 : (int16_t)values[1];
    return 0;
}

static int logical_pair(we_node_t *n, const char *prop, const char *v, int len) {
    int margin = w_eq(prop, "margin-inline") || w_eq(prop, "margin-block");
    int block = w_eq(prop, "margin-block") || w_eq(prop, "padding-block");
    int starts[4], lengths[4], count = tokens(v, len, starts, lengths), k, values[2], auto_bits[2] = {0, 0}, pcts[2];
    int sides[2] = {logical_side(n, block, 0), logical_side(n, block, 1)};
    static const uint8_t flags[4] = {WE_AUTO_TOP, WE_AUTO_RIGHT, WE_AUTO_BOTTOM, WE_AUTO_LEFT};
    if (count < 1 || count > 2) return -1;
    for (k = 0; k < count; k++) {
        int end = lengths[k];
        const char *token = v + starts[k];
        pcts[k] = WE_NO_PCT;
        if (margin && value_is(token, end, "auto")) { values[k] = 0; auto_bits[k] = 1; }
        else if (spacing_percent(token, end, margin, &pcts[k]) == 0) values[k] = 0;
        else {
            if (end >= 2 && value_is(token + end - 2, 2, "px")) end -= 2;
            if (bounded_signed(token, end, 2000, &values[k]) || (!margin && values[k] < 0)) return -1;
        }
    }
    if (count == 1) { values[1] = values[0]; auto_bits[1] = auto_bits[0]; pcts[1] = pcts[0]; }
    for (k = 0; k < 2; k++) {
        if (margin) {
            n->margin[sides[k]] = (int16_t)values[k];
            n->margin_auto &= (uint8_t)~flags[sides[k]];
            if (auto_bits[k]) n->margin_auto |= flags[sides[k]];
        } else n->padding[sides[k]] = (int16_t)values[k];
        spacing_set(n, !margin, sides[k], pcts[k]);
    }
    return 0;
}

static int parse_aspect(const char *v, int len, int16_t *numerator, int16_t *denominator) {
    int at = 0, first, a, b = 1;
    if (value_is(v, len, "auto")) { *numerator = 0; *denominator = 1; return 0; }
    while (at < len && w_space(v[at])) at++;
    first = at;
    while (at < len && v[at] >= '0' && v[at] <= '9') at++;
    if (bounded_uint(v + first, at - first, 2000, &a) || !a) return -1;
    while (at < len && w_space(v[at])) at++;
    if (at < len) {
        if (v[at++] != '/') return -1;
        while (at < len && w_space(v[at])) at++;
        first = at;
        while (at < len && v[at] >= '0' && v[at] <= '9') at++;
        if (bounded_uint(v + first, at - first, 2000, &b) || !b) return -1;
        while (at < len && w_space(v[at])) at++;
    }
    if (at != len) return -1;
    *numerator = (int16_t)a; *denominator = (int16_t)b;
    return 0;
}

static int legacy_length_valid(int slot, const char *v, int len) {
    int a, b;
    if (slot < 0) return 0;
    if (slot == WE_LEN_WIDTH || slot == WE_LEN_MIN_W || slot == WE_LEN_MAX_W) {
        if (value_is(v, len, "min-content") || value_is(v, len, "max-content") || value_is(v, len, "fit-content") || value_is(v, len, "none")) return 1;
        return parse_len(v, len, &a, &b) >= 0;
    }
    if (slot == WE_LEN_BASIS && (value_is(v, len, "auto") || value_is(v, len, "content"))) return 1;
    if (slot == WE_LEN_HEIGHT && value_is(v, len, "auto")) return 1;
    if (slot >= WE_LEN_LEFT && slot <= WE_LEN_BOTTOM) return parse_len(v, len, &a, 0) == 0;
    return parse_dimension(v, len, &a, &b) == 0 && (slot < WE_LEN_ROW_GAP || !b);
}

static int intrinsic_slot(int slot) {
    return slot == WE_LEN_WIDTH || slot == WE_LEN_MIN_W || slot == WE_LEN_MAX_W || slot == WE_LEN_BASIS;
}

static int intrinsic_kind(const we_node_t *n, int slot) {
    return slot == WE_LEN_WIDTH ? n->width_kind : slot == WE_LEN_MIN_W ? n->min_width_kind :
           slot == WE_LEN_MAX_W ? n->max_width_kind : n->basis_kind;
}

static void set_intrinsic_kind(we_node_t *n, int slot, int kind) {
    if (slot == WE_LEN_WIDTH) n->width_kind = (uint8_t)kind;
    else if (slot == WE_LEN_MIN_W) n->min_width_kind = (uint8_t)kind;
    else if (slot == WE_LEN_MAX_W) n->max_width_kind = (uint8_t)kind;
    else n->basis_kind = (uint8_t)kind;
}

static int intrinsic_declaration(we_node_t *n, int slot, const char *v, int len) {
    int kind = value_is(v, len, "min-content") ? WE_SIZE_MIN_CONTENT :
               value_is(v, len, "max-content") ? WE_SIZE_MAX_CONTENT :
               value_is(v, len, "fit-content") ? WE_SIZE_FIT_CONTENT : WE_SIZE_AUTO;
    we_length_t expression;
    w_zero(&expression, sizeof(expression));
    if (w_ieq_n(v, len, "fit-content(")) {
        int first = 12, end = len - 1, number, percent;
        if (end < first || v[end] != ')') return -1;
        while (first < end && w_space(v[first])) first++;
        while (end > first && w_space(v[end - 1])) end--;
        if (math_function(v + first, end - first)) {
            if (parse_math(v + first, end - first, &expression)) return -1;
        } else {
            if (parse_dimension(v + first, end - first, &number, &percent)) return -1;
            expression.count = 1; expression.percent = (uint8_t)percent;
            expression.node[0].op = percent ? M_PERCENT : M_PX;
            expression.node[0].value = (int16_t)number;
            expression.node[0].dimension = 1;
        }
        kind = WE_SIZE_FIT_LIMIT;
    }
    if (!kind) return 0;
    n->length[slot] = expression;
    set_intrinsic_kind(n, slot, kind);
    if (slot == WE_LEN_WIDTH) { n->width = -1; n->width_pct = 0; }
    else if (slot == WE_LEN_MIN_W) { n->min_width = -1; n->min_pct = 0; }
    else if (slot == WE_LEN_MAX_W) { n->max_width = -1; n->max_pct = 0; }
    else { n->basis_set = 1; n->basis_content = 0; n->flex_basis = -1; n->basis_pct = 0; }
    return 1;
}

static int inset_declaration(we_node_t *n, const char *prop, const char *v, int len) {
    int starts[4], lengths[4], slots[4], count, k, physical = math_slot(prop);
    int values[4] = {0, 0, 0, 0}, automatic[4] = {0, 0, 0, 0};
    we_length_t expression[4];
    static const uint8_t flags[4] = {WE_INSET_LEFT, WE_INSET_TOP, WE_INSET_RIGHT, WE_INSET_BOTTOM};
    count = functional_tokens(v, len, starts, lengths, 4);
    if (count < 1) return -1;
    if (physical >= WE_LEN_LEFT && physical <= WE_LEN_BOTTOM) {
        if (count != 1) return -1;
        slots[0] = physical;
    } else if (w_eq(prop, "inset")) {
        slots[0] = WE_LEN_TOP; slots[1] = WE_LEN_RIGHT;
        slots[2] = WE_LEN_BOTTOM; slots[3] = WE_LEN_LEFT;
    } else {
        if (count > 2) return -1;
        static const uint8_t side_slot[4] = {WE_LEN_TOP, WE_LEN_RIGHT, WE_LEN_BOTTOM, WE_LEN_LEFT};
        slots[0] = side_slot[logical_side(n, w_eq(prop, "inset-block"), 0)];
        slots[1] = side_slot[logical_side(n, w_eq(prop, "inset-block"), 1)];
    }
    w_zero(expression, sizeof(expression));
    for (k = 0; k < count; k++) {
        const char *token = v + starts[k];
        int size = lengths[k], end = size, percent;
        if (value_is(token, size, "auto")) automatic[k] = 1;
        else if (math_function(token, size)) {
            if (parse_math(token, size, &expression[k])) return -1;
        } else {
            percent = size && token[size - 1] == '%';
            if (percent) end--;
            else if (size >= 2 && value_is(token + size - 2, 2, "px")) end -= 2;
            if (bounded_signed(token, end, 2000, &values[k])) return -1;
            if (percent) {
                expression[k].count = expression[k].percent = 1;
                expression[k].node[0].op = M_PERCENT;
                expression[k].node[0].value = (int16_t)values[k];
                expression[k].node[0].dimension = 1;
            }
        }
    }
    {
        int total = physical >= WE_LEN_LEFT && physical <= WE_LEN_BOTTOM ? 1 : w_eq(prop, "inset") ? 4 : 2;
        for (k = 0; k < total; k++) {
            int from = k < count ? k : k == 2 ? 0 : count > 1 ? 1 : 0;
            int slot = slots[k], axis = slot - WE_LEN_LEFT;
            n->length[slot] = expression[from];
            if (automatic[from]) n->inset_flags &= (uint8_t)~flags[axis];
            else n->inset_flags |= flags[axis];
            if (slot == WE_LEN_LEFT) n->offset_x = (int16_t)values[from];
            else if (slot == WE_LEN_TOP) n->offset_y = (int16_t)values[from];
            else if (slot == WE_LEN_RIGHT) n->offset_r = (int16_t)values[from];
            else n->offset_b = (int16_t)values[from];
        }
    }
    return 0;
}

/* Lot 61 border edges: style keywords, one-edge shorthands and 1..4 value
 * lists. Codes: 1 none, 2 hidden, 3 solid, 4 double, 5 dashed, 6 dotted,
 * 7 ridge, 8 outset, 9 groove, 10 inset. */
static int border_style_code(const char *v, int l) {
    static const char *const names[] = { "none", "hidden", "solid", "double", "dashed", "dotted",
                                         "ridge", "outset", "groove", "inset" };
    int k;
    for (k = 0; k < 10; k++) if (w_ieq_n(v, l, names[k])) return k + 1;
    return 0;
}

/* CSS 17.6.2.1 style precedence: double > solid > dashed > dotted > ridge >
 * outset > groove > inset. Unset ranks as solid. */
static int border_style_rank(int style) {
    static const uint8_t rank[11] = { 7, 0, 0, 7, 8, 6, 5, 4, 3, 2, 1 };
    return style >= 0 && style <= 10 ? rank[style] : 0;
}

/* tokens() stops at four; a fifth token makes a list invalid. */
static int tokens_more(const char *v, int vl, const int *a, const int *l, int c) {
    int i;
    if (c < 4) return 0;
    for (i = a[3] + l[3]; i < vl; i++) if (!w_space(v[i])) return 1;
    return 0;
}

static int border_value_index(int count, int side) {
    if (count == 1) return 0;
    if (count == 2) return side & 1;
    if (count == 3) return side == 3 ? 1 : side;
    return side;
}

static int border_edge_decl(we_node_t *n, int side, const char *v, int vl) {
    int a[4], l[4], c = tokens(v, vl, a, l), k, px, w = -1, style = 0, has_color = 0;
    uint32_t col = 0;
    if (c < 1 || c > 3 || tokens_more(v, vl, a, l, c)) return -1;
    for (k = 0; k < c; k++) {
        int code = border_style_code(v + a[k], l[k]);
        if (code) { if (style) return -1; style = code; }
        else if (parse_color(v + a[k], l[k], &col) == 0) { if (has_color) return -1; has_color = 1; }
        else if (parse_len(v + a[k], l[k], &px, 0) == 0 && px >= 0 && px <= 64) { if (w >= 0) return -1; w = px; }
        else return -1;
    }
    n->edge_w[side] = (int8_t)(w < 0 ? -1 : w > 8 ? 8 : w);
    n->edge_style[side] = (uint8_t)style;
    if (has_color) { n->edge_color[side] = col; n->edge_color_set = (uint8_t)(n->edge_color_set | (1u << side)); }
    else n->edge_color_set = (uint8_t)(n->edge_color_set & ~(1u << side));
    return 0;
}

/* kind 0 style, 1 width, 2 color; one value sets the uniform property. */
static int border_list_decl(we_node_t *n, int kind, const char *v, int vl) {
    int a[4], l[4], c = tokens(v, vl, a, l), k, side, vals[4];
    uint32_t cols[4];
    if (c < 1 || tokens_more(v, vl, a, l, c)) return -1;
    for (k = 0; k < c; k++) {
        if (kind == 0) { vals[k] = border_style_code(v + a[k], l[k]); if (!vals[k]) return -1; }
        else if (kind == 1) { if (parse_len(v + a[k], l[k], &vals[k], 0) || vals[k] < 0 || vals[k] > 64) return -1; }
        else if (parse_color(v + a[k], l[k], &cols[k])) return -1;
    }
    for (side = 0; side < 4; side++) {
        k = border_value_index(c, side);
        if (c == 1) {
            if (kind == 0) { n->border_style = (uint8_t)vals[0]; n->edge_style[side] = 0; }
            else if (kind == 1) { n->border = (int16_t)vals[0]; n->edge_w[side] = -1; }
            else { n->border_color = cols[0]; n->edge_color_set = 0; }
        } else if (kind == 0) n->edge_style[side] = (uint8_t)vals[k];
        else if (kind == 1) n->edge_w[side] = (int8_t)(vals[k] > 8 ? 8 : vals[k]);
        else { n->edge_color[side] = cols[k]; n->edge_color_set = (uint8_t)(n->edge_color_set | (1u << side)); }
    }
    return 0;
}

/* Lot 64: border-<side>-width / -style / -color: side * 4 + part (1 width,
 * 2 style, 3 colour), -1 for other names. */
static int edge_longhand(const char *prop) {
    static const char *const sides[4] = {"top", "right", "bottom", "left"};
    static const char *const parts[4] = {"", "width", "style", "color"};
    int side, part, len;
    if (!w_ieq_n(prop, 7, "border-")) return -1;
    for (side = 0; side < 4; side++) {
        len = w_len(sides[side]);
        if (!w_ieq_n(prop + 7, len, sides[side]) || prop[7 + len] != '-') continue;
        for (part = 1; part < 4; part++) if (w_eq(prop + 8 + len, parts[part])) return side * 4 + part;
    }
    return -1;
}

static int border_edge_part(we_node_t *n, int side, int part, const char *v, int vl) {
    int px, code;
    uint32_t col;
    if (part == 1) {
        if (parse_len(v, vl, &px, 0) || px < 0 || px > 64) return -1;
        n->edge_w[side] = (int8_t)(px > 8 ? 8 : px);
    } else if (part == 2) {
        if (!(code = border_style_code(v, vl))) return -1;
        n->edge_style[side] = (uint8_t)code;
    } else {
        if (parse_color(v, vl, &col)) return -1;
        n->edge_color[side] = col;
        n->edge_color_set = (uint8_t)(n->edge_color_set | (1u << side));
    }
    return 0;
}

/* Lot 64: border-inline / border-block (one edge value for both sides) and
 * their -width / -style / -color (one value, or start and end). */
static int logical_side(const we_node_t *n, int block, int end);
static int border_logical_pair(we_node_t *n, const char *prop, const char *v, int vl) {
    int block = prop[7] == 'b', len = block ? 12 : 13, part, a[4], l[4], c, k, side[2];
    static const char *const parts[4] = {"", "-width", "-style", "-color"};
    for (part = 0; part < 4 && !w_eq(prop + len, parts[part]); part++) {}
    if (part == 4) return -1;
    side[0] = logical_side(n, block, 0); side[1] = logical_side(n, block, 1);
    if (part == 0) {
        static we_node_t probe;
        probe = *n;                          /* both sides or neither */
        if (border_edge_decl(&probe, side[0], v, vl)) return -1;
        (void)border_edge_decl(n, side[0], v, vl);
        (void)border_edge_decl(n, side[1], v, vl);
        return 0;
    }
    c = tokens(v, vl, a, l);
    if (c < 1 || c > 2 || tokens_more(v, vl, a, l, c)) return -1;
    {
        static we_node_t probe;
        probe = *n;
        for (k = 0; k < 2; k++) if (border_edge_part(&probe, side[k], part, v + a[k % c], l[k % c])) return -1;
        *n = probe;
    }
    return 0;
}

static void border_edges_reset(we_node_t *n) {
    int side;
    for (side = 0; side < 4; side++) { n->edge_w[side] = -1; n->edge_style[side] = 0; }
    n->edge_color_set = 0;
}

/* ---------- Lot 62: transform parsing ----------
 * transform: none or up to WE_TF_MAX of translate(x[, y]), translateX(),
 * translateY(), scale(s[, t]), scaleX(), scaleY(), rotate(a), and (lot 63)
 * skew(a[, b]), skewX(a) and skewY(a). Angles are deg, grad, rad or turn
 * (or a bare 0) and round to whole degrees; a skew angle, taken modulo
 * 180deg, stays within +-80deg (tan <= 5.7). Lengths are px (or em, or a
 * bare 0) and percentages of the border box; scales are numbers or
 * percentages in -8..8. Anything else is refused atomically. */
static int tf_args(const char *v, int len, int *start, int *length, int cap) {
    int at = 0, count = 0;
    while (at <= len) {
        int first, end;
        while (at < len && w_space(v[at])) at++;
        first = at;
        while (at < len && v[at] != ',') at++;
        end = at;
        while (end > first && w_space(v[end - 1])) end--;
        if (end == first || count == cap) return -1;
        start[count] = first; length[count++] = end - first;
        at++;
    }
    return count;
}

static int tf_length(const char *v, int len, int16_t *out, int *percent) {
    int px, pct, negative = len > 0 && v[0] == '-', digits = negative, unit;
    while (digits < len && ((v[digits] >= '0' && v[digits] <= '9') || v[digits] == '.')) digits++;
    unit = len - digits;
    if (digits == negative) return -1;
    if (!(unit == 0 && len - negative == 1 && v[negative] == '0') &&
        !(unit == 2 && (w_ieq_n(v + digits, 2, "px") || w_ieq_n(v + digits, 2, "em"))) &&
        !(unit == 1 && v[digits] == '%')) return -1;
    if (parse_len(v + negative, len - negative, &px, &pct) != 0) return -1;
    *percent = v[len - 1] == '%';
    if (*percent) px = pct > 1000 ? 1000 : pct;
    *out = (int16_t)(negative ? -px : px);
    return 0;
}

static int tf_scale(const char *v, int len, int16_t *out) {
    int milli, percent;
    if (len > 3 && w_ieq_n(v + len - 3, 3, "deg")) return -1;
    if (color_number(v, len, &milli, &percent)) return -1;
    if (percent) milli /= 100;
    if (milli < -8000 || milli > 8000) return -1;
    *out = (int16_t)milli;
    return 0;
}

/* An angle in whole degrees, rounded half away from zero. */
static int tf_angle(const char *v, int len, int *degrees) {
    static const struct { const char *unit; int16_t num, den; } units[] = {
        {"turn", 360, 1}, {"grad", 9, 10}, {"rad", 573, 10}, {"deg", 1, 1}, {0, 0, 0}};
    int milli, percent, k, u = -1;
    for (k = 0; units[k].unit && u < 0; k++) {
        int ul = w_len(units[k].unit);
        if (len > ul && w_ieq_n(v + len - ul, ul, units[k].unit)) { u = k; len -= ul; }
    }
    if (u < 0 && !(len == 1 && v[0] == '0')) return -1;
    if (v[len - 1] < '0' || v[len - 1] > '9' || color_number(v, len, &milli, &percent) || percent) return -1;
    if (u >= 0) {
        if (milli > 2000000000 / units[u].num || milli < -2000000000 / units[u].num) return -1;
        milli = milli * units[u].num / units[u].den;
    }
    *degrees = (milli >= 0 ? milli + 500 : milli - 500) / 1000;
    return 0;
}

/* A skew angle modulo 180deg in -80..80 (90deg has no tangent). */
static int tf_skew_angle(const char *v, int len, int16_t *out) {
    int deg;
    if (tf_angle(v, len, &deg)) return -1;
    deg %= 180;
    if (deg > 90) deg -= 180;
    if (deg < -90) deg += 180;
    if (deg > 80 || deg < -80) return -1;
    *out = (int16_t)deg;
    return 0;
}

static int tf_function(const char *v, int len, we_transform_t *t) {
    static const char *const names[] = { "translatex(", "translatey(", "translate(", "scalex(", "scaley(", "scale(", "rotate(",
                                         "skewx(", "skewy(", "skew(", 0 };
    int k, open = 0, start[2], length[2], count, pct = 0;
    for (k = 0; names[k]; k++) if (w_ieq_n(v, len, names[k])) { open = w_len(names[k]); break; }
    if (!names[k] || v[len - 1] != ')') return -1;
    count = tf_args(v + open, len - open - 1, start, length, 2);
    if (count < 1 || (count == 2 && k != 2 && k != 5 && k != 9)) return -1;
    v += open;
    w_zero(t, sizeof(*t));
    if (k <= 2) {
        int vertical = k == 1;
        t->kind = WE_TF_TRANSLATE;
        if (tf_length(v + start[0], length[0], vertical ? &t->y : &t->x, &pct)) return -1;
        if (pct) t->pct |= (uint8_t)(vertical ? 2 : 1);
        if (count == 2) {
            if (tf_length(v + start[1], length[1], &t->y, &pct)) return -1;
            if (pct) t->pct |= 2;
        }
    } else if (k <= 5) {
        t->kind = WE_TF_SCALE;
        t->x = t->y = 1000;
        if (tf_scale(v + start[0], length[0], k == 4 ? &t->y : &t->x)) return -1;
        if (k == 5 && tf_scale(v + start[count - 1], length[count - 1], &t->y)) return -1;
    } else if (k == 6) {
        int deg;
        t->kind = WE_TF_ROTATE;
        if (tf_angle(v + start[0], length[0], &deg)) return -1;
        deg %= 360;
        t->x = (int16_t)(deg < 0 ? deg + 360 : deg);
    } else {
        t->kind = WE_TF_SKEW;
        if (tf_skew_angle(v + start[0], length[0], k == 8 ? &t->y : &t->x)) return -1;
        if (count == 2 && tf_skew_angle(v + start[1], length[1], &t->y)) return -1;
    }
    return 0;
}

/* Lot 63: matrix(a, b, c, d, e, f): a..d numbers in -8..8 kept in 1/1000,
 * e and f px numbers in -4000..4000. It fills three slots: WE_TF_MATRIX
 * (a, b) then two WE_TF_DATA (c, d) and (e, f). */
static int tf_matrix(const char *v, int len, we_transform_t *t) {
    int start[6], length[6], count, k;
    /* the caller checked "matrix(" and the closing parenthesis */
    count = tf_args(v + 7, len - 8, start, length, 6);
    if (count != 6) return -1;
    v += 7;
    w_zero(t, 3 * sizeof(*t));
    t[0].kind = WE_TF_MATRIX; t[1].kind = t[2].kind = WE_TF_DATA;
    for (k = 0; k < 4; k++) if (tf_scale(v + start[k], length[k], k & 1 ? &t[k / 2].y : &t[k / 2].x)) return -1;
    for (k = 4; k < 6; k++) {
        int milli, percent;
        if (color_number(v + start[k], length[k], &milli, &percent) || percent || milli < -4000000 || milli > 4000000) return -1;
        milli = (milli >= 0 ? milli + 500 : milli - 500) / 1000;
        if (k == 4) t[2].x = (int16_t)milli; else t[2].y = (int16_t)milli;
    }
    return 0;
}

static int parse_transform(we_node_t *n, const char *v, int len) {
    int starts[WE_TF_MAX + 1], lengths[WE_TF_MAX + 1], count, k, used = 0;
    we_transform_t list[WE_TF_MAX];
    if (value_is(v, len, "none")) { n->tf_count = 0; return 0; }
    count = functional_tokens(v, len, starts, lengths, WE_TF_MAX + 1);
    if (count < 1 || count > WE_TF_MAX) return -1;
    for (k = 0; k < count; k++) {
        const char *f = v + starts[k];
        if (w_ieq_n(f, lengths[k], "matrix(")) {
            if (used + 3 > WE_TF_MAX || tf_matrix(f, lengths[k], &list[used])) return -1;
            used += 3;
        } else {
            if (used == WE_TF_MAX || tf_function(f, lengths[k], &list[used])) return -1;
            used++;
        }
    }
    for (k = 0; k < used; k++) n->tf[k] = list[k];
    n->tf_count = (uint8_t)used;
    return 0;
}

/* transform-origin: one or two of left/center/right/top/bottom, lengths
 * and percentages; a lone vertical keyword sets y, x stays centred. */
static int origin_value(const char *v, int len, int *axis, int16_t *value, int *percent) {
    static const struct { const char *name; int8_t axis, pct; } keys[] = {
        {"left", 0, 0}, {"right", 0, 100}, {"top", 1, 0}, {"bottom", 1, 100}, {"center", -1, 50}, {0, 0, 0}};
    int k;
    for (k = 0; keys[k].name; k++)
        if (value_is(v, len, keys[k].name)) { *axis = keys[k].axis; *value = keys[k].pct; *percent = 1; return 0; }
    *axis = -2;
    return tf_length(v, len, value, percent);
}

/* One or two position values (keywords, lengths, percentages), x first. */
static int position_pair(const char *v, int len, int16_t out[2], uint8_t *pct_bits) {
    int a[4], l[4], c = tokens(v, len, a, l), axis[2], pct[2], k, swap = 0;
    int16_t value[2];
    if (c < 1 || c > 2) return -1;
    for (k = 0; k < c; k++) if (origin_value(v + a[k], l[k], &axis[k], &value[k], &pct[k])) return -1;
    if (c == 1) {
        axis[1] = -1; value[1] = 50; pct[1] = 1;
        if (axis[0] == 1) swap = 1;
    } else {
        if (axis[0] == 1 || axis[1] == 0) swap = 1;
        if ((axis[0] >= 0 && axis[0] == axis[1]) || (swap && (axis[0] == 0 || axis[1] == 1 || axis[0] == -2 || axis[1] == -2)))
            return -1;
    }
    if (swap) {
        int16_t tv = value[0]; int tp = pct[0];
        value[0] = value[1]; pct[0] = pct[1]; value[1] = tv; pct[1] = tp;
    }
    out[0] = value[0]; out[1] = value[1];
    *pct_bits = (uint8_t)((pct[0] ? 1 : 0) | (pct[1] ? 2 : 0));
    return 0;
}

/* ---------- Lot 63: background-image url(), repeat and size ---------- */
/* The url stays in the pool where the declaration already lives; the
 * path rules are those of <img src> and are checked when the box paints.
 * An <img> keeps its own source and refuses a background image. */
/* lot 64: the url() span in the pool, shared by list-style-image */
static int url_span(const we_doc_t *d, const char *v, int len, uint16_t *off, uint16_t *ulen) {
    int a = 4, b = len - 1;
    if (len < 6 || !w_ieq_n(v, 4, "url(") || v[len - 1] != ')') return -1;
    while (a < b && w_space(v[a])) a++;
    while (b > a && w_space(v[b - 1])) b--;
    if (b - a >= 2 && (v[a] == '"' || v[a] == '\'') && v[b - 1] == v[a]) { a++; b--; }
    if (b <= a || v < d->pool || v + len > d->pool + d->pool_used) return -1;
    *off = (uint16_t)(v + a - d->pool);
    *ulen = (uint16_t)(b - a);
    return 0;
}

static int background_url(we_doc_t *d, we_node_t *n, const char *v, int len) {
    if (w_eq(n->tag, "img")) return -1;
    return url_span(d, v, len, &n->bgi_off, &n->bgi_len);
}

/* repeat, no-repeat, repeat-x, repeat-y, or one repeat/no-repeat per axis.
 * Lot 64: space and round repeat too, with their style in bgi_space. */
static int background_repeat(we_node_t *n, const char *v, int len) {
    int a[4], l[4], c = tokens(v, len, a, l), k, bits = 0, space = 0;
    if (c == 1 && value_is(v, len, "repeat-x")) bits = 1;
    else if (c == 1 && value_is(v, len, "repeat-y")) bits = 2;
    else if (c == 1 || c == 2) {
        for (k = 0; k < c; k++) {
            int axes = c == 1 ? 3 : 1 << k, style = value_is(v + a[k], l[k], "space") ? WE_BGR_SPACE :
                       value_is(v + a[k], l[k], "round") ? WE_BGR_ROUND : WE_BGR_PLAIN;
            if (style || value_is(v + a[k], l[k], "repeat")) bits |= axes;
            else if (!value_is(v + a[k], l[k], "no-repeat")) return -1;
            if (axes & 1) space |= style;
            if (axes & 2) space |= style << 2;
        }
    } else return -1;
    n->bgi_repeat = (uint8_t)bits;
    n->bgi_space = (uint8_t)space;
    return 0;
}

/* Lot 64: exchanges the first background layer with layer k (k >= 1), so
 * that the one-layer parsers and painter serve every layer. */
static void bgi_layer_swap(we_node_t *n, int k) {
    we_bg_layer_t *l = &n->bgi_more[k - 1], t = *l;
    l->off = n->bgi_off; l->len = n->bgi_len; l->repeat = n->bgi_repeat; l->size_kind = n->bgi_size_kind;
    l->pos_pct = n->bgi_pos_pct; l->space = n->bgi_space;
    l->pos[0] = n->bgi_pos[0]; l->pos[1] = n->bgi_pos[1]; l->size[0] = n->bgi_size[0]; l->size[1] = n->bgi_size[1];
    n->bgi_off = t.off; n->bgi_len = t.len; n->bgi_repeat = t.repeat; n->bgi_size_kind = t.size_kind;
    n->bgi_pos_pct = t.pos_pct; n->bgi_space = t.space;
    n->bgi_pos[0] = t.pos[0]; n->bgi_pos[1] = t.pos[1]; n->bgi_size[0] = t.size[0]; n->bgi_size[1] = t.size[1];
}

/* auto, contain, cover, or one or two lengths/percentages (auto allowed). */
static int background_size(we_node_t *n, const char *v, int len) {
    int a[4], l[4], c = tokens(v, len, a, l), k, pct;
    int16_t size[2] = {-1, -1};
    uint8_t bits = 0;
    if (c == 1 && value_is(v, len, "contain")) { n->bgi_size_kind = WE_BGS_CONTAIN; return 0; }
    if (c == 1 && value_is(v, len, "cover")) { n->bgi_size_kind = WE_BGS_COVER; return 0; }
    if (c < 1 || c > 2) return -1;
    for (k = 0; k < c; k++) {
        if (value_is(v + a[k], l[k], "auto")) continue;
        if (tf_length(v + a[k], l[k], &size[k], &pct) || size[k] < 0) return -1;
        if (pct) bits |= (uint8_t)(4 << k);
    }
    n->bgi_size_kind = size[0] < 0 && size[1] < 0 ? WE_BGS_AUTO : WE_BGS_EXPLICIT;
    n->bgi_size[0] = size[0]; n->bgi_size[1] = size[1];
    n->bgi_pos_pct = (uint8_t)((n->bgi_pos_pct & 0x33) | bits);
    return 0;
}

/* Lot 64: one axis of a position: a keyword, a length or percentage, or
 * an edge keyword with its offset (right 10px). Offsets from the far edge
 * set *far for a length and become 100 - p for a percentage. want is the
 * axis (0 x, 1 y); a centre or a bare length fits either. */
static int axis_offset(const char *v, int len, int want, int16_t *value, int *pct, int *far) {
    int a[4], l[4], c = tokens(v, len, a, l), axis, oaxis, opct;
    int16_t off;
    *far = 0;
    if (c == 1) {
        if (origin_value(v, len, &axis, value, pct)) return -1;
        return axis >= 0 && axis != want ? -1 : 0;
    }
    if (c != 2 || origin_value(v + a[0], l[0], &axis, value, pct) || axis != want ||
        origin_value(v + a[1], l[1], &oaxis, &off, &opct) || oaxis != -2) return -1;
    if (*value == 0) { *value = off; *pct = opct; return 0; }
    if (opct) { *value = (int16_t)(100 - off); *pct = 1; return 0; }
    *value = off; *pct = 0; *far = 1;
    return 0;
}

/* three or four values: two groups of an edge keyword, each with an
 * optional offset; centre takes none and the axes are told by the keywords */
static int position_edges(const char *v, int len, int16_t out[2], uint8_t *bits) {
    int a[4], l[4], c = tokens(v, len, a, l), k = 0, g = 0, start[2], end[2], axis[2], dummy, pct, far, ax;
    int16_t value;
    if (c < 3 || c > 4) return -1;
    for (k = a[c - 1] + l[c - 1]; k < len; k++) if (!w_space(v[k])) return -1;   /* tokens() stops at four */
    k = 0;
    while (k < c) {
        if (g == 2 || origin_value(v + a[k], l[k], &axis[g], &value, &pct) || axis[g] == -2) return -1;
        start[g] = k++;
        if (k < c && !origin_value(v + a[k], l[k], &dummy, &value, &pct) && dummy == -2) {
            if (axis[g] < 0) return -1;
            k++;
        }
        end[g++] = k;
    }
    if (g != 2 || (axis[0] >= 0 && axis[0] == axis[1])) return -1;
    if (axis[0] < 0) axis[0] = axis[1] < 0 ? 0 : 1 - axis[1];
    if (axis[1] < 0) axis[1] = 1 - axis[0];
    *bits = 0;
    for (k = 0; k < 2; k++) {
        const char *t = v + a[start[k]];
        ax = axis[k];
        if (axis_offset(t, (int)(v + a[end[k] - 1] + l[end[k] - 1] - t), ax, &value, &pct, &far)) return -1;
        out[ax] = value;
        *bits |= (uint8_t)((pct ? 1 << ax : 0) | (far ? 16 << ax : 0));
    }
    return 0;
}

static int background_position(we_node_t *n, const char *v, int len) {
    uint8_t bits;
    int a[4], l[4];
    if (tokens(v, len, a, l) > 2 ? position_edges(v, len, n->bgi_pos, &bits) : position_pair(v, len, n->bgi_pos, &bits)) return -1;
    n->bgi_pos_pct = (uint8_t)((n->bgi_pos_pct & 12) | bits);
    return 0;
}

/* background-position-x and -y (axis 0 or 1) */
static int background_position_axis(we_node_t *n, const char *v, int len, int axis) {
    int16_t value;
    int pct, far;
    if (axis_offset(v, len, axis, &value, &pct, &far)) return -1;
    n->bgi_pos[axis] = value;
    n->bgi_pos_pct = (uint8_t)((n->bgi_pos_pct & ~(0x11 << axis)) | (pct ? 1 << axis : 0) | (far ? 16 << axis : 0));
    return 0;
}

/* Lot 64: a comma list of background-repeat (kind 0), -position (1) or
 * -size (2) values: layer k takes item k modulo the count (CSS repeats a
 * short list). The node is changed only when every item parses. */
static we_node_t layer_scratch;

/* initial values of every background-image layer */
static void bgi_reset(we_node_t *n) {
    int k;
    n->bgi_len = 0; n->bgi_repeat = 3; n->bgi_size_kind = WE_BGS_AUTO; n->bgi_pos_pct = 3;
    n->bgi_pos[0] = n->bgi_pos[1] = 0; n->bgi_size[0] = n->bgi_size[1] = -1;
    n->bgi_space = 0; n->bgi_layers = 0;
    for (k = 0; k < WE_BG_LAYERS - 1; k++) {
        we_bg_layer_t *l = &n->bgi_more[k];
        l->off = l->len = 0; l->repeat = 3; l->size_kind = WE_BGS_AUTO; l->pos_pct = 3; l->space = 0;
        l->pos[0] = l->pos[1] = 0; l->size[0] = l->size[1] = -1;
    }
}
static int list_count(const char *v, int len) {
    int a[WE_BG_LAYERS + 1], l[WE_BG_LAYERS + 1];
    return comma_args(v, len, a, l, WE_BG_LAYERS + 1);
}
static int background_list(we_node_t *n, const char *v, int len, int kind) {
    int a[WE_BG_LAYERS], l[WE_BG_LAYERS], c = comma_args(v, len, a, l, WE_BG_LAYERS), k, bad = 0;
    if (c < 1) return -1;
    layer_scratch = *n;
    for (k = 0; k < WE_BG_LAYERS && !bad; k++) {
        const char *item = v + a[k % c];
        int il = l[k % c];
        if (k) bgi_layer_swap(&layer_scratch, k);
        bad = kind == 0 ? background_repeat(&layer_scratch, item, il) :
              kind == 1 ? background_position(&layer_scratch, item, il) :
              kind == 2 ? background_size(&layer_scratch, item, il) : background_position_axis(&layer_scratch, item, il, kind - 3);
        if (k) bgi_layer_swap(&layer_scratch, k);
    }
    if (bad) return -1;
    *n = layer_scratch;
    return 0;
}

/* Lot 64: the background shorthand with images: comma layers of
 * [url() | none | gradient] [repeat style] [position [/ size]], the last one
 * also a colour; a gradient only in the last layer. Every background
 * property not given returns to its initial value. Box keywords
 * (border-box ...) and attachment are refused with the declaration. */
static int parse_color(const char* v, int n, uint32_t* out);
static int shorthand_kind(const char *t, int l) {
    static const char *const repeats[] = {"repeat", "no-repeat", "repeat-x", "repeat-y", "space", "round", 0};
    static const char *const places[] = {"left", "right", "top", "bottom", "center", 0};
    int k;
    if (l > 4 && w_ieq_n(t, 4, "url(")) return 1;
    if (value_is(t, l, "none")) return 1;
    if (l > 16 && (w_ieq_n(t, 16, "linear-gradient(") || w_ieq_n(t, 16, "radial-gradient("))) return 2;
    for (k = 0; repeats[k]; k++) if (value_is(t, l, repeats[k])) return 3;
    for (k = 0; places[k]; k++) if (value_is(t, l, places[k])) return 4;
    if ((t[0] >= '0' && t[0] <= '9') || t[0] == '-' || t[0] == '.') return 4;
    if (value_is(t, l, "auto") || value_is(t, l, "contain") || value_is(t, l, "cover")) return 5;
    return 6;                                                    /* a colour, or nothing valid */
}

/* the shorthand takes this path with a url, a list or several tokens */
static int shorthand_layers(const char *v, int len) {
    int k, depth = 0;
    for (k = 0; k < len; k++) {
        if (v[k] == '(') depth++;
        else if (v[k] == ')') depth--;
        else if (!depth && (w_space(v[k]) || v[k] == ',' || v[k] == '/')) return 1;
        if (k + 4 <= len && w_ieq_n(v + k, 4, "url(")) return 1;
    }
    return 0;
}

static int background_shorthand(we_doc_t *d, we_node_t *n, const char *v, int len) {
    int a[WE_BG_LAYERS], l[WE_BG_LAYERS], c = comma_args(v, len, a, l, WE_BG_LAYERS), k, urls = 0;
    if (c < 1) return -1;
    layer_scratch = *n;
    layer_scratch.has_bg = 0; layer_scratch.grad_dir = 0;
    bgi_reset(&layer_scratch);
    for (k = 0; k < c; k++) {
        const char *s = v + a[k];
        int sl = l[k], at = 0, span[6][2], slash = 0, kind, bad = 0, m;
        for (m = 0; m < 6; m++) span[m][0] = span[m][1] = -1;
        while (at < sl && !bad) {
            int t, depth = 0;
            if (w_space(s[at])) { at++; continue; }
            if (s[at] == '/') { if (slash || span[3][0] < 0) return -1; slash = 1; at++; continue; }
            for (t = at; t < sl && (depth || (!w_space(s[t]) && s[t] != '/')); t++) {
                if (s[t] == '(') depth++;
                else if (s[t] == ')') depth--;
            }
            kind = shorthand_kind(s + at, t - at);
            if (slash && (kind == 4 || kind == 5)) kind = 5;
            else if (kind == 5) return -1;                      /* a size needs the slash */
            if (kind == 1 || kind == 2 || kind == 6) {
                if (span[kind - 1][0] >= 0 && kind != 6) return -1;   /* one image per layer */
                if (kind == 6 && (span[5][0] >= 0 || k != c - 1)) return -1;
            } else if (span[kind - 1][0] >= 0) {
                /* the tokens of a group must follow each other */
                int g;
                for (g = span[kind - 1][1]; g < at && (w_space(s[g]) || s[g] == '/'); g++) {}
                if (g != at) return -1;
            }
            if (span[kind - 1][0] < 0) span[kind - 1][0] = at;
            span[kind - 1][1] = t;
            at = t;
        }
        if (slash && span[4][0] < 0) return -1;
        if (span[1][0] >= 0 && k != c - 1) return -1;           /* a gradient only lowest */
        if (span[0][0] >= 0 && span[1][0] >= 0) return -1;
        if (span[1][0] >= 0 && parse_gradient(&layer_scratch, s + span[1][0], span[1][1] - span[1][0])) return -1;
        if (span[5][0] >= 0) {
            uint32_t col;
            if (parse_color(s + span[5][0], span[5][1] - span[5][0], &col)) return -1;
            if (!w_ieq_n(s + span[5][0], span[5][1] - span[5][0], "transparent")) { layer_scratch.bg = col; layer_scratch.has_bg = 1; }
        }
        if (span[1][0] >= 0) continue;
        if (urls >= WE_BG_LAYERS) return -1;
        if (urls) bgi_layer_swap(&layer_scratch, urls);
        if (span[0][0] >= 0 && !value_is(s + span[0][0], span[0][1] - span[0][0], "none"))
            bad = background_url(d, &layer_scratch, s + span[0][0], span[0][1] - span[0][0]);
        if (!bad && span[2][0] >= 0) bad = background_repeat(&layer_scratch, s + span[2][0], span[2][1] - span[2][0]);
        if (!bad && span[3][0] >= 0) bad = background_position(&layer_scratch, s + span[3][0], span[3][1] - span[3][0]);
        if (!bad && span[4][0] >= 0) bad = background_size(&layer_scratch, s + span[4][0], span[4][1] - span[4][0]);
        if (urls) bgi_layer_swap(&layer_scratch, urls);
        if (bad) return -1;
        urls++;
    }
    layer_scratch.bgi_layers = (uint8_t)(urls > 1 ? urls - 1 : 0);
    *n = layer_scratch;
    return 0;
}

/* Lot 64: background-image as a comma list: url() or none layers, the
 * first on top; a gradient may only be the last (lowest) one, since the
 * gradient paints under every image. */
static int parse_gradient(we_node_t *n, const char *v, int len);
static int background_layers(we_doc_t *d, we_node_t *n, const char *v, int len) {
    int a[WE_BG_LAYERS], l[WE_BG_LAYERS], c = comma_args(v, len, a, l, WE_BG_LAYERS), k, urls = 0, bad = 0;
    if (c < 1) return -1;
    layer_scratch = *n;
    layer_scratch.grad_dir = 0; layer_scratch.bgi_len = 0; layer_scratch.bgi_layers = 0;
    for (k = 0; k < c && !bad; k++) {
        const char *item = v + a[k];
        if (l[k] > 16 && (w_ieq_n(item, 16, "linear-gradient(") || w_ieq_n(item, 16, "radial-gradient("))) {
            bad = k != c - 1 || parse_gradient(&layer_scratch, item, l[k]);
            continue;
        }
        if (urls) bgi_layer_swap(&layer_scratch, urls);
        if (value_is(item, l[k], "none")) layer_scratch.bgi_len = 0;
        else bad = background_url(d, &layer_scratch, item, l[k]);
        if (urls) bgi_layer_swap(&layer_scratch, urls);
        urls++;
    }
    if (bad) return -1;
    layer_scratch.bgi_layers = (uint8_t)(urls ? urls - 1 : 0);
    *n = layer_scratch;
    return 0;
}

static int parse_transform_origin(we_node_t *n, const char *v, int len) {
    return position_pair(v, len, n->tf_origin, &n->tf_origin_pct);
}

/* ---------- Lot 62: outline and box-shadow parsing ---------- */
static int outline_width_value(const char *v, int len, int *out) {
    int px, pct;
    if (value_is(v, len, "thin")) { *out = 1; return 0; }
    if (value_is(v, len, "medium")) { *out = 3; return 0; }
    if (value_is(v, len, "thick")) { *out = 5; return 0; }
    if (len < 1 || v[0] < '0' || v[0] > '9' || parse_len(v, len, &px, &pct) != 0 || pct) return -1;
    if (!(len == 1 && v[0] == '0') && !(len > 2 && (w_ieq_n(v + len - 2, 2, "px") || w_ieq_n(v + len - 2, 2, "em")))) return -1;
    *out = px > 8 ? 8 : px;
    return 0;
}

/* outline-style accepts the border styles except hidden, and auto (solid). */
static int outline_style_value(const char *v, int len) {
    int code = value_is(v, len, "auto") ? WE_BS_SOLID : border_style_code(v, len);
    return code == WE_BS_HIDDEN ? 0 : code;
}

/* ---------- Lot 64: multi-column properties ---------- */
static int column_count_value(const char *v, int len, int *out) {
    int k, value = 0;
    if (value_is(v, len, "auto")) { *out = 0; return 0; }
    if (len < 1 || len > 3) return -1;
    for (k = 0; k < len; k++) {
        if (v[k] < '0' || v[k] > '9') return -1;
        value = value * 10 + v[k] - '0';
    }
    if (value < 1 || value > WE_COLUMNS_MAX) return -1;
    *out = value;
    return 0;
}

static int column_width_value(const char *v, int len, int *out) {
    int px;
    if (value_is(v, len, "auto")) { *out = -1; return 0; }
    /* no percentage pointer: parse_len refuses a percentage */
    if (len < 1 || v[0] < '0' || v[0] > '9' || parse_len(v, len, &px, 0) != 0 || px < 1) return -1;
    *out = px;
    return 0;
}

/* columns: one or two of <count> and <width> (auto for either) */
static int parse_columns(we_node_t *n, const char *v, int len) {
    int a[4], l[4], c = tokens(v, len, a, l), k, count = -1, width = -2, value, autos = 0;
    if (c < 1 || c > 2 || tokens_more(v, len, a, l, c)) return -1;
    for (k = 0; k < c; k++) {
        if (value_is(v + a[k], l[k], "auto")) { autos++; continue; }
        if (column_count_value(v + a[k], l[k], &value) == 0) { if (count >= 0) return -1; count = value; }
        else if (column_width_value(v + a[k], l[k], &value) == 0) { if (width != -2) return -1; width = value; }
        else return -1;
    }
    if (autos == 2 && c == 2) { count = 0; width = -1; }
    n->col_count = (uint8_t)(count < 0 ? 0 : count);
    n->col_width = (int16_t)(width == -2 ? -1 : width);
    return 0;
}

/* column-rule: width, style and colour in any order, each once */
static int parse_column_rule(we_node_t *n, const char *v, int len) {
    int a[4], l[4], c = tokens(v, len, a, l), k, style = 0, width = -1, has_color = 0;
    uint32_t color = 0;
    if (c < 1 || c > 3 || tokens_more(v, len, a, l, c)) return -1;
    for (k = 0; k < c; k++) {
        int code = border_style_code(v + a[k], l[k]), w;
        if (code) { if (style) return -1; style = code; }
        else if (outline_width_value(v + a[k], l[k], &w) == 0) { if (width >= 0) return -1; width = w; }
        else if (parse_color(v + a[k], l[k], &color) == 0) { if (has_color) return -1; has_color = 1; }
        else return -1;
    }
    n->rule_style = (uint8_t)style;
    n->rule_w = (uint8_t)(width < 0 ? 3 : width);
    n->rule_color_set = (uint8_t)has_color;
    n->rule_color = color;
    return 0;
}

static int parse_outline(we_node_t *n, const char *v, int len) {
    int a[4], l[4], c = tokens(v, len, a, l), k, style = 0, width = -1, has_color = 0;
    uint32_t color = 0;
    if (c < 1 || c > 3 || tokens_more(v, len, a, l, c)) return -1;
    for (k = 0; k < c; k++) {
        int code = outline_style_value(v + a[k], l[k]), w;
        if (code) { if (style) return -1; style = code; }
        else if (outline_width_value(v + a[k], l[k], &w) == 0) { if (width >= 0) return -1; width = w; }
        else if (parse_color(v + a[k], l[k], &color) == 0) { if (has_color) return -1; has_color = 1; }
        else return -1;
    }
    n->outline_style = (uint8_t)(style == WE_BS_NONE ? 0 : style);
    n->outline_w = (uint8_t)(width < 0 ? 3 : width);
    n->outline_color_set = (uint8_t)has_color;
    n->outline_color = color;
    return 0;
}

static int shadow_length(const char *v, int len, int low, int high, int *out) {
    int16_t value;
    int percent;
    if (tf_length(v, len, &value, &percent) || percent) return -1;
    *out = value < low ? low : value > high ? high : value;
    return 0;
}

/* box-shadow: none, or up to WE_SHADOWS_MAX comma-separated shadows of
 * [inset] x y [blur [spread]] [color] in any order of the colour and
 * inset keyword around the lengths. */
/* text: a text-shadow entry (no inset, no spread) */
static int parse_shadow(const char *v, int len, we_shadow_t *sh, int text) {
    int starts[6], lengths[6], count = functional_tokens(v, len, starts, lengths, 6), k, lens = 0, value[4], closed = 0;
    w_zero(sh, sizeof(*sh));
    if (count < 2) return -1;
    for (k = 0; k < count; k++) {
        const char *t = v + starts[k];
        int l = lengths[k];
        if ((t[0] >= '0' && t[0] <= '9') || t[0] == '-' || t[0] == '.') {
            /* the lengths are one run: no colour or inset inside it */
            if (lens == (text ? 3 : 4) || closed) return -1;
            if (shadow_length(t, l, -64, 64, &value[lens])) return -1;
            if (lens == 2 && value[2] < 0) return -1;
            if (lens == 2 && value[2] > 32) value[2] = 32;
            lens++;
            continue;
        }
        closed = lens > 0;
        if (!text && value_is(t, l, "inset")) { if (sh->inset) return -1; sh->inset = 1; continue; }
        if (sh->has_color || parse_color(t, l, &sh->color)) return -1;
        sh->has_color = 1;
    }
    if (lens < 2) return -1;
    sh->x = (int8_t)value[0]; sh->y = (int8_t)value[1];
    sh->blur = (uint8_t)(lens > 2 ? value[2] : 0);
    sh->spread = (int8_t)(lens > 3 ? value[3] : 0);
    return 0;
}

static int parse_shadows(we_shadow_t *out, uint8_t *out_count, const char *v, int len, int text) {
    we_shadow_t list[WE_SHADOWS_MAX];
    int count = 0, at = 0, depth = 0, start = 0, k;
    if (value_is(v, len, "none")) { *out_count = 0; return 0; }
    for (at = 0; at <= len; at++) {
        if (at < len && v[at] == '(') depth++;
        if (at < len && v[at] == ')') depth--;
        if (at < len && (depth || v[at] != ',')) continue;
        if (count == WE_SHADOWS_MAX || parse_shadow(v + start, at - start, &list[count], text)) return -1;
        count++;
        start = at + 1;
    }
    for (k = 0; k < count; k++) out[k] = list[k];
    *out_count = (uint8_t)count;
    return 0;
}

static int box_keyword(const char *v, int len) {
    if (value_is(v, len, "border-box")) return WE_BOX_BORDER;
    if (value_is(v, len, "padding-box")) return WE_BOX_PADDING;
    if (value_is(v, len, "content-box")) return WE_BOX_CONTENT;
    return -1;
}

/* ---------- Lot 62: text-decoration parsing ----------
 * Lines: none or any of underline, overline, line-through; style: solid,
 * double, dotted, dashed, wavy (painted solid, diagnosed); thickness:
 * auto, from-font or 1..4 px; colour: any colour. The shorthand takes
 * one of each in any order. */
static int deco_lines(const char *v, int len, int *lines) {
    if (value_is(v, len, "underline")) *lines = WE_DECO_UNDER;
    else if (value_is(v, len, "overline")) *lines = WE_DECO_OVER;
    else if (value_is(v, len, "line-through")) *lines = WE_DECO_THROUGH;
    else return -1;
    return 0;
}

static int deco_style_value(const char *v, int len) {
    int code = border_style_code(v, len);
    if (value_is(v, len, "wavy")) return WE_BS_WAVY;
    return code == WE_BS_SOLID || code == 4 || code == 5 || code == 6 ? code : 0;
}

static int deco_thickness(const char *v, int len, int *out) {
    int16_t value;
    int percent;
    if (value_is(v, len, "auto") || value_is(v, len, "from-font")) { *out = 1; return 0; }
    if (tf_length(v, len, &value, &percent) || percent || value < 0) return -1;
    *out = value < 1 ? 1 : value > 4 ? 4 : value;
    return 0;
}

/* kind 0 shorthand, 1 line, 2 style, 3 colour, 4 thickness */
static int parse_decoration(we_node_t *n, int kind, const char *v, int len) {
    int a[4], l[4], c = tokens(v, len, a, l), k, lines = 0, style = 0, thick = 0, has_color = 0, seen_line = 0;
    uint32_t color = 0;
    if (c < 1 || tokens_more(v, len, a, l, c)) return -1;
    if (kind == 3) { if (c != 1 || parse_color(v, len, &color)) return -1; n->deco_color = color; n->deco_color_set = 1; return 0; }
    if (kind == 4) { if (c != 1 || deco_thickness(v, len, &thick)) return -1; n->deco_thick = (uint8_t)thick; return 0; }
    if (kind == 2) { if (c != 1 || !(style = deco_style_value(v, len))) return -1; n->deco_style = (uint8_t)style; return 0; }
    for (k = 0; k < c; k++) {
        const char *t = v + a[k];
        int one, s, w;
        if (value_is(t, l[k], "none")) { if (seen_line) return -1; seen_line = 2; continue; }
        if (deco_lines(t, l[k], &one) == 0) {
            if (seen_line == 2 || (lines & one)) return -1;
            seen_line = 1; lines |= one;
            continue;
        }
        if (kind == 1) return -1;
        if ((s = deco_style_value(t, l[k])) != 0) { if (style) return -1; style = s; continue; }
        if (deco_thickness(t, l[k], &w) == 0) { if (thick) return -1; thick = w; continue; }
        if (parse_color(t, l[k], &color) == 0) { if (has_color) return -1; has_color = 1; continue; }
        return -1;
    }
    if (kind == 1 && !seen_line) return -1;
    n->deco_line = (uint8_t)lines;
    if (kind == 0) {
        n->deco_style = (uint8_t)(style ? style : WE_BS_SOLID);
        n->deco_thick = (uint8_t)(thick ? thick : 1);
        n->deco_color_set = (uint8_t)has_color;
        n->deco_color = color;
    }
    return 0;
}

/* ---------- Lot 63: generated content values ----------
 * content, quotes and counter-* keep their declared text in the pool; these
 * scanners validate it when declared and read it again when the boxes are
 * generated. Strings take CSS escapes; code points outside printable ASCII
 * become '?' (the kernel font), \A becomes a newline. */
static int list_keyword(const char *v, int len);

static void gen_put(char *out, int *olen, int cap, char c) {
    if (out && *olen < cap) out[*olen] = c;
    (*olen)++;
}

static int css_string(const char *v, int len, int *at, char *out, int *olen, int cap) {
    int i = *at, quote = v[i++];
    while (i < len && v[i] != quote) {
        char c = v[i++];
        if (c == '\n') return -1;
        if (c == '\\' && i < len) {
            int code = 0, digits = 0;
            while (i < len && digits < 6 && css_hex(v[i]) >= 0) { code = code * 16 + css_hex(v[i++]); digits++; }
            if (digits) {
                if (i < len && w_space(v[i])) i++;
                c = code == 0xA ? '\n' : code >= 32 && code < 127 ? (char)code : '?';
            } else if (v[i] == '\n') { i++; continue; }
            else c = v[i++];
        }
        gen_put(out, olen, cap, c);
    }
    if (i >= len) return -1;
    *at = i + 1;
    return 0;
}

static int gen_ident(const char *v, int len, int *at, char *out, int cap) {
    int i = *at, k = 0;
    if (i >= len || !(w_namec(v[i]) || v[i] == '-') || (v[i] >= '0' && v[i] <= '9')) return -1;
    while (i < len && (w_namec(v[i]) || v[i] == '-')) { if (k < cap - 1) out[k++] = w_low(v[i]); i++; }
    out[k] = 0;
    *at = i;
    return k < cap - 1 ? 0 : -1;
}

static void gen_skip(const char *v, int len, int *at) { while (*at < len && w_space(v[*at])) (*at)++; }

/* counter() / counters() style argument: a list-style-type keyword */
static int counter_style_arg(const char *v, int len, int *at, int *style) {
    char name[24];
    gen_skip(v, len, at);
    if (gen_ident(v, len, at, name, sizeof(name))) return -1;
    *style = list_keyword(name, w_len(name));
    return *style < 0 ? -1 : 0;
}

typedef struct { char name[16]; int value; int16_t scope; } we_counter_t;
typedef struct {
    we_counter_t c[WE_COUNTERS_MAX];
    int n, quote_depth;
} we_gen_t;

static int counter_find(const we_gen_t *g, const char *name) {
    int k;
    for (k = g->n - 1; k >= 0; k--) if (w_eq(g->c[k].name, name)) return k;
    return -1;
}

static int counter_text(int style, int number, char *out);
static int list_ordinal(const we_doc_t *d, int idx);

/* quotes: none or 1..2 pairs of strings; at most WE_QUOTE_PAIRS */
#define WE_QUOTE_PAIRS 2
static int quotes_scan(const char *v, int len, char text[2 * WE_QUOTE_PAIRS][4], int *pairs) {
    int i = 0, k = 0;
    while (1) {
        int l = 0;
        char buf[8];
        gen_skip(v, len, &i);
        if (i >= len) break;
        if (k == 2 * WE_QUOTE_PAIRS || (v[i] != '"' && v[i] != '\'') || css_string(v, len, &i, buf, &l, 7)) return -1;
        if (l > 3) return -1;
        if (text) { w_cpy(text[k], 4, buf, l); }
        k++;
    }
    if (k == 0 || k % 2) return -1;
    *pairs = k / 2;
    return 0;
}

/* counter-reset/-increment/-set: none or ident [integer] pairs (at most 4) */
#define WE_COUNTER_PAIRS 4
static int counter_scan(const char *v, int len, char names[WE_COUNTER_PAIRS][16], int *values, int fallback) {
    int i = 0, k = 0;
    if (value_is(v, len, "none")) return 0;
    while (1) {
        char name[16];
        int value = fallback;
        gen_skip(v, len, &i);
        if (i >= len) break;
        if (k == WE_COUNTER_PAIRS || gen_ident(v, len, &i, name, sizeof(name)) || w_eq(name, "none") ||
            w_eq(name, "inherit") || w_eq(name, "initial") || w_eq(name, "unset") || (i < len && !w_space(v[i]))) return -1;
        gen_skip(v, len, &i);
        if (i < len && (v[i] == '-' || v[i] == '+' || (v[i] >= '0' && v[i] <= '9'))) {
            int a = i, neg = v[i] == '-';
            value = 0;
            if (v[i] == '-' || v[i] == '+') i++;
            if (i >= len || v[i] < '0' || v[i] > '9') return -1;
            while (i < len && v[i] >= '0' && v[i] <= '9') { if (i - a > 6) return -1; value = value * 10 + (v[i++] - '0'); }
            if (i < len && !w_space(v[i])) return -1;
            if (neg) value = -value;
        }
        if (names) { w_cpy(names[k], 16, name, w_len(name)); values[k] = value; }
        k++;
    }
    return k ? k : -1;
}

/* Declarations are always read from the pool (rules and style attributes). */
static void pool_span(const we_doc_t *d, const char *v, int len, uint16_t *off, uint16_t *out_len) {
    *off = (uint16_t)(v - d->pool);
    *out_len = (uint16_t)len;
}

static int counter_value(const we_doc_t *d, const we_gen_t *g, int elem, const char *name) {
    int k = counter_find(g, name);
    if (k >= 0) return g->c[k].value;
    /* the implicit list-item counter of the nearest li */
    if (w_eq(name, "list-item"))
        while (elem > 0) { if (w_eq(d->nodes[elem].tag, "li")) return list_ordinal(d, elem); elem = d->nodes[elem].parent; }
    return 0;
}

static void gen_text(char *out, int *olen, int cap, const char *s) { while (*s) gen_put(out, olen, cap, *s++); }

static void gen_counter(char *out, int *olen, int cap, int style, int value) {
    char buf[24];
    (void)counter_text(style, value, buf);
    gen_text(out, olen, cap, buf);
}

/* open-quote/close-quote: the quotes of the pseudo box at the nesting
 * depth (the last pair past it); close at depth 0 paints nothing. */
static void gen_quote(we_doc_t *d, int pseudo, we_gen_t *g, const char *fn, char *out, int *olen, int cap) {
    static char text[2 * WE_QUOTE_PAIRS][4] = { "\"", "\"", "'", "'" };
    char list[2 * WE_QUOTE_PAIRS][4];
    const we_node_t *n = &d->nodes[pseudo];
    int pairs = 2, open = fn[0] == 'o' || fn[3] == 'o', show = fn[0] != 'n', level;
    if (!open) { if (g->quote_depth == 0) return; g->quote_depth--; }
    level = g->quote_depth;
    if (open) g->quote_depth++;
    if (!show || n->quotes_kind == WE_QUOTES_NONE) return;
    if (n->quotes_kind == WE_QUOTES_LIST) (void)quotes_scan(d->pool + n->quotes_off, n->quotes_len, list, &pairs);
    if (level >= pairs) level = pairs - 1;
    gen_text(out, olen, cap, (n->quotes_kind == WE_QUOTES_LIST ? list : text)[level * 2 + !open]);
}

/* Validates (pseudo < 0) or evaluates a content list for a pseudo box. */
static int gen_content(we_doc_t *d, const char *v, int len, int pseudo, we_gen_t *g, char *out, int *olen) {
    int i = 0, cap = WE_CONTENT_TEXT_MAX, elem = pseudo >= 0 ? d->nodes[pseudo].parent : -1;
    while (1) {
        char name[16], fn[16];
        gen_skip(v, len, &i);
        if (i >= len) break;
        if (v[i] == '"' || v[i] == '\'') { if (css_string(v, len, &i, out, olen, cap)) return -1; continue; }
        if (gen_ident(v, len, &i, fn, sizeof(fn))) return -1;
        if (i < len && v[i] == '(') {
            int style = WE_LIST_DECIMAL, close, sep_a = 0, sep_l = 0;
            i++; gen_skip(v, len, &i);
            if (gen_ident(v, len, &i, name, sizeof(name))) return -1;
            gen_skip(v, len, &i);
            if (w_eq(fn, "attr")) {
                if (i >= len || v[i] != ')') return -1;
                i++;
                if (elem > 0) {
                    const we_node_t *e = &d->nodes[elem];
                    const char *a = 0;
                    int l = 0, flag = w_eq(name, "id") ? 1 : w_eq(name, "class") ? 2 : w_eq(name, "href") ? 4 : w_eq(name, "src") ? 8 : w_eq(name, "alt") ? 16 : 0;
                    if (flag & e->attr_flags) {
                        if (flag == 1) { a = e->id; l = w_len(a); }
                        else if (flag == 2) { a = e->cls; l = w_len(a); }
                        else if (flag == 4) { a = d->pool + e->href_off; l = e->href_len; }
                        else if (flag == 8) { a = d->pool + e->src_off; l = e->src_len; }
                        else { a = d->pool + e->alt_off; l = e->alt_len; }
                    }
                    while (l-- > 0) gen_put(out, olen, cap, *a++);
                }
                continue;
            }
            if (!w_eq(fn, "counter") && !w_eq(fn, "counters")) return -1;
            if (w_eq(fn, "counters")) {
                int l = 0;
                if (i >= len || v[i++] != ',') return -1;
                gen_skip(v, len, &i);
                sep_a = i;
                if (i >= len || (v[i] != '"' && v[i] != '\'') || css_string(v, len, &i, 0, &l, 0)) return -1;
                sep_l = i - sep_a;
                gen_skip(v, len, &i);
            }
            if (i < len && v[i] == ',') { i++; if (counter_style_arg(v, len, &i, &style)) return -1; gen_skip(v, len, &i); }
            close = i < len && v[i] == ')';
            if (!close) return -1;
            i++;
            if (pseudo < 0) continue;
            if (w_eq(fn, "counter") || counter_find(g, name) < 0) { gen_counter(out, olen, cap, style, counter_value(d, g, elem, name)); continue; }
            {   /* counters(): every instance in scope, outermost first */
                int k, first = 1;
                for (k = 0; k < g->n; k++) {
                    int at = sep_a;
                    if (!w_eq(g->c[k].name, name)) continue;
                    if (!first) (void)css_string(v, sep_a + sep_l, &at, out, olen, cap);
                    gen_counter(out, olen, cap, style, g->c[k].value);
                    first = 0;
                }
            }
            continue;
        }
        if (w_eq(fn, "open-quote") || w_eq(fn, "close-quote") || w_eq(fn, "no-open-quote") || w_eq(fn, "no-close-quote")) {
            if (pseudo >= 0) gen_quote(d, pseudo, g, fn, out, olen, cap);
            continue;
        }
        return -1;
    }
    return 0;
}

/* Returns 1 when the declaration was a generated content property. */
static int content_declaration(we_doc_t *d, we_node_t *n, const char *prop, const char *v, int vl) {
    int pairs, l = 0, slot;
    if (w_eq(prop, "content")) {
        if (value_is(v, vl, "none") || value_is(v, vl, "normal")) n->content_kind = WE_CONTENT_NONE;
        else if (gen_content(d, v, vl, -1, 0, 0, &l)) diag(d, "content refused", -1);
        else { pool_span(d, v, vl, &n->content_off, &n->content_len); n->content_kind = WE_CONTENT_LIST; }
        return 1;
    }
    if (w_eq(prop, "quotes")) {
        if (value_is(v, vl, "none")) n->quotes_kind = WE_QUOTES_NONE;
        else if (value_is(v, vl, "auto")) n->quotes_kind = WE_QUOTES_AUTO;
        else if (quotes_scan(v, vl, 0, &pairs)) diag(d, "quotes refused", -1);
        else { pool_span(d, v, vl, &n->quotes_off, &n->quotes_len); n->quotes_kind = WE_QUOTES_LIST; }
        return 1;
    }
    slot = w_eq(prop, "counter-reset") ? 0 : w_eq(prop, "counter-increment") ? 1 : w_eq(prop, "counter-set") ? 2 : -1;
    if (slot < 0) return 0;
    if (counter_scan(v, vl, 0, 0, 0) < 0) diag(d, "counter refused", -1);
    else pool_span(d, v, vl, &n->counter_off[slot], &n->counter_len[slot]);
    return 1;
}

/* Lot 63: overflow keywords; hidden, clip, auto and scroll clip (no
 * scrollbars are painted). A box clips both axes once one axis does, as a
 * visible axis computes to auto next to a non-visible one. */
static int overflow_keyword(const char *v, int len) {
    if (value_is(v, len, "visible")) return 0;
    if (value_is(v, len, "hidden") || value_is(v, len, "clip") || value_is(v, len, "auto") || value_is(v, len, "scroll")) return 1;
    return -1;
}

static int overflow_declaration(we_node_t *n, const char *prop, const char *v, int vl) {
    int a[4], l[4], count = tokens(v, vl, a, l), x, y;
    if (count < 1 || count > (w_eq(prop, "overflow") ? 2 : 1)) return -1;
    x = overflow_keyword(v + a[0], l[0]);
    y = count == 2 ? overflow_keyword(v + a[1], l[1]) : x;
    if (x < 0 || y < 0) return -1;
    if (w_eq(prop, "overflow-x")) n->overflow_axes = (uint8_t)((n->overflow_axes & 2) | x);
    else if (w_eq(prop, "overflow-y")) n->overflow_axes = (uint8_t)((n->overflow_axes & 1) | (y << 1));
    else n->overflow_axes = (uint8_t)(x | (y << 1));
    n->overflow_hidden = (uint8_t)(n->overflow_axes != 0);
    return 0;
}

/* Lot 63: tab-size, a number of space advances (at most 31) or a px/em
 * length (at most 255 px), kept in px; 0 removes tabs. */
static int tab_size_value(we_node_t *n, const char *v, int vl) {
    int px, pct, k;
    for (k = 0; k < vl && v[k] >= '0' && v[k] <= '9'; k++) {}
    if (k == vl && vl > 0 && vl <= 2) {
        int count = 0;
        for (k = 0; k < vl; k++) count = count * 10 + v[k] - '0';
        if (count > 31) return -1;
        n->tab_px = (uint8_t)(count * WE_CHAR_W);
        return 0;
    }
    if (!(vl > 2 && (w_ieq_n(v + vl - 2, 2, "px") || w_ieq_n(v + vl - 2, 2, "em"))) || v[0] < '0' || v[0] > '9' ||
        parse_len(v, vl, &px, &pct) != 0 || pct || px > 255) return -1;
    n->tab_px = (uint8_t)px;
    return 0;
}

/* Lot 63: CSS Text 4 longhands of white-space over the same modes;
 * break-spaces keeps its mode across text-wrap-mode wrap. */
static int white_space_longhand(we_node_t *n, const char *prop, const char *v, int vl) {
    int preserve = n->ws_mode == WE_WS_PRE || n->ws_mode == WE_WS_PRE_WRAP || n->ws_mode == WE_WS_BREAK_SPACES, nowrap = n->nowrap;
    if (w_eq(prop, "white-space-collapse")) {
        if (value_is(v, vl, "collapse")) n->ws_mode = WE_WS_NORMAL;
        else if (value_is(v, vl, "preserve")) n->ws_mode = nowrap ? WE_WS_PRE : WE_WS_PRE_WRAP;
        else if (value_is(v, vl, "preserve-breaks")) n->ws_mode = WE_WS_PRE_LINE;
        else if (value_is(v, vl, "break-spaces")) n->ws_mode = nowrap ? WE_WS_PRE : WE_WS_BREAK_SPACES;
        else return -1;
        return 0;
    }
    if (value_is(v, vl, "nowrap")) nowrap = 1;
    else if (value_is(v, vl, "wrap") || (w_eq(prop, "text-wrap") && (value_is(v, vl, "balance") || value_is(v, vl, "pretty") || value_is(v, vl, "stable")))) nowrap = 0;
    else return -1;
    n->nowrap = (uint8_t)nowrap;
    if (preserve) n->ws_mode = nowrap ? WE_WS_PRE : n->ws_mode == WE_WS_BREAK_SPACES ? WE_WS_BREAK_SPACES : WE_WS_PRE_WRAP;
    return 0;
}

static int mode_pass;   /* lot 64: 1 while only writing-mode/direction apply */
static void apply_decl(we_doc_t* d, we_node_t* n, const char* p, int pl, const char* v, int vl) {
    char prop[40];
    int px, pct, k;
    uint32_t col;
    w_cpy(prop, sizeof(prop), p, pl);
    for (k = 0; prop[k]; k++) prop[k] = w_low(prop[k]);
    if (mode_pass && !w_eq(prop, "writing-mode") && !w_eq(prop, "direction") && !w_eq(prop, "text-orientation")) return;
    { const char *mapped = logical_property(n, prop); if (mapped != prop) w_cpy(prop, sizeof(prop), mapped, w_len(mapped)); }
    while (vl > 0 && w_space(v[vl - 1])) vl--;
    d->perf.decls_applied++;
    if (w_eq(prop, "inset") || w_eq(prop, "inset-inline") || w_eq(prop, "inset-block") ||
        (math_slot(prop) >= WE_LEN_LEFT && math_slot(prop) <= WE_LEN_BOTTOM)) {
        if (inset_declaration(n, prop, v, vl))
            diag(d, math_function(v, vl) && math_slot(prop) >= WE_LEN_LEFT ? "math length refused" : "position offset refused", -1);
        return;
    }
    {
        int slot = math_slot(prop);
        if (intrinsic_slot(slot)) {
            int result = intrinsic_declaration(n, slot, v, vl);
            if (result) { if (result < 0) diag(d, "intrinsic size refused", -1); return; }
        }
        if (slot >= 0 && math_function(v, vl)) {
            we_length_t expr;
            if (parse_math(v, vl, &expr)) diag(d, "math length refused", -1);
            else if (slot >= WE_LEN_ROW_GAP && expr.percent) diag(d, "math gap percent refused", -1);
            else set_math(n, slot, &expr);
            return;
        }
        if (legacy_length_valid(slot, v, vl)) {
            n->length[slot].count = 0;
            if (intrinsic_slot(slot)) set_intrinsic_kind(n, slot, WE_SIZE_AUTO);
        }
    }
    if (content_declaration(d, n, prop, v, vl)) return;
    if (w_eq(prop, "aspect-ratio")) {
        int16_t a, b;
        if (parse_aspect(v, vl, &a, &b)) diag(d, "aspect ratio refused", -1);
        else { n->aspect_num = a; n->aspect_den = b; }
    } else if (w_eq(prop, "margin-inline") || w_eq(prop, "margin-block") || w_eq(prop, "padding-inline") || w_eq(prop, "padding-block")) {
        if (logical_pair(n, prop, v, vl)) diag(d, "logical spacing refused", -1);
    } else if (w_eq(prop, "writing-mode")) {
        if (value_is(v, vl, "horizontal-tb")) n->wmode = WE_WM_HORIZONTAL;
        else if (value_is(v, vl, "vertical-rl")) n->wmode = WE_WM_VERTICAL_RL;
        else if (value_is(v, vl, "vertical-lr")) n->wmode = WE_WM_VERTICAL_LR;
        else if (!mode_pass) diag(d, "writing mode refused", -1);
    } else if (w_eq(prop, "direction")) {
        if (value_is(v, vl, "ltr") || value_is(v, vl, "rtl")) n->dir_rtl = (uint8_t)value_is(v, vl, "rtl");
        else if (!mode_pass) diag(d, "direction refused", -1);
    } else if (w_eq(prop, "text-orientation")) {
        if (value_is(v, vl, "mixed") || value_is(v, vl, "upright")) n->upright = (uint8_t)value_is(v, vl, "upright");
        else if (!mode_pass) diag(d, "text orientation refused", -1);   /* sideways needs rotated glyphs */
    } else if (w_eq(prop, "padding-top") || w_eq(prop, "padding-right") || w_eq(prop, "padding-bottom")) {
        int side = w_eq(prop, "padding-top") ? 0 : w_eq(prop, "padding-right") ? 1 : 2;
        if (spacing_percent(v, vl, 0, &pct) == 0) spacing_set(n, 1, side, pct);
        else if (parse_len(v, vl, &px, 0) == 0 && px >= 0) { n->padding[side] = (int16_t)px; spacing_set(n, 1, side, WE_NO_PCT); }
        else diag(d, "padding refused", -1);
    } else if (w_eq(prop, "display")) {
        if (value_is(v, vl, "none")) n->display = WE_DISP_NONE;
        else if (value_is(v, vl, "block")) n->display = WE_DISP_BLOCK;
        else if (value_is(v, vl, "inline")) n->display = WE_DISP_INLINE;
        else if (value_is(v, vl, "flex")) n->display = WE_DISP_FLEX;
        else if (value_is(v, vl, "grid")) n->display = WE_DISP_GRID;
        else if (value_is(v, vl, "table")) n->display = WE_DISP_TABLE;
        else if (value_is(v, vl, "table-row")) n->display = WE_DISP_ROW;
        else if (value_is(v, vl, "table-cell")) n->display = WE_DISP_CELL;
        else if (value_is(v, vl, "inline-block")) n->display = WE_DISP_INLINE_BLOCK;
        else diag(d, "display refused", -1);
    } else if (w_eq(prop, "flex-direction")) {
        if (value_is(v, vl, "row") || value_is(v, vl, "row-reverse")) {
            n->flex_dir = 0;
            n->flex_reverse = (uint8_t)value_is(v, vl, "row-reverse");
        } else if (value_is(v, vl, "column") || value_is(v, vl, "column-reverse")) {
            n->flex_dir = 1;
            n->flex_reverse = (uint8_t)value_is(v, vl, "column-reverse");
        } else diag(d, "flex-direction refused", -1);
    } else if (w_eq(prop, "order")) {
        if (bounded_signed(v, vl, WE_ORDER_MAX, &px) == 0) n->item_order = (int8_t)px;
        else diag(d, "order refused", -1);
    } else if (w_eq(prop, "flex-wrap")) {
        if (value_is(v, vl, "nowrap")) n->flex_wrap = 0;
        else if (value_is(v, vl, "wrap")) n->flex_wrap = 1;
        else if (value_is(v, vl, "wrap-reverse")) n->flex_wrap = 2;
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
        if ((w_eq(prop, "align-items") || w_eq(prop, "align-self")) &&
            (value_is(v, vl, "baseline") || value_is(v, vl, "first baseline"))) value = WE_PACK_BASELINE;
        if ((w_eq(prop, "align-items") || w_eq(prop, "align-self")) && value_is(v, vl, "last baseline"))
            value = WE_PACK_LAST_BASELINE;
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
        else if (w_eq(prop, "row-gap")) { n->row_gap = (int16_t)number; n->gap_set |= 2; }
        else { n->col_gap = (int16_t)number; n->gap_set |= 1; }
    } else if (w_eq(prop, "grid-auto-flow")) {
        int a[4], lengths[4], c = tokens(v, vl, a, lengths), dense = 0, column = 0, axis = 0, ok = c >= 1 && c <= 2;
        for (k = 0; k < c && k < 2; k++) {
            if (value_is(v + a[k], lengths[k], "dense") && !dense) dense = 1;
            else if (!axis && (value_is(v + a[k], lengths[k], "row") || value_is(v + a[k], lengths[k], "column"))) {
                column = value_is(v + a[k], lengths[k], "column"); axis = 1;
            } else ok = 0;
        }
        if (!ok) diag(d, "grid flow refused", -1);
        else { n->grid_dense = (uint8_t)dense; n->grid_column_flow = (uint8_t)column; }
    } else if (w_eq(prop, "grid-template-areas")) {
        if (parse_areas(n, v, vl)) diag(d, "grid areas refused", -1);
    } else if (w_eq(prop, "grid-area")) {
        if (value_is(v, vl, "auto")) n->grid_area[0] = 0;
        else if (grid_name(v, vl)) w_cpy(n->grid_area, WE_GRID_NAME_MAX, v, vl);
        else diag(d, "grid area name refused", -1);
    } else if (w_eq(prop, "grid-auto-columns") || w_eq(prop, "grid-auto-rows")) {
        int16_t size, low, cap;
        if (track_value(v, vl, &size, &low, &cap)) diag(d, "grid auto track refused", -1);
        else if (w_eq(prop, "grid-auto-columns")) { n->auto_col = size; n->auto_col_floor = low; n->auto_col_cap = cap; }
        else { n->auto_row = size; n->auto_row_floor = low; n->auto_row_cap = cap; }
    } else if (w_eq(prop, "object-fit")) {
        int fit = value_is(v, vl, "fill") ? WE_FIT_FILL : value_is(v, vl, "contain") ? WE_FIT_CONTAIN :
            value_is(v, vl, "cover") ? WE_FIT_COVER : value_is(v, vl, "none") ? WE_FIT_NONE :
            value_is(v, vl, "scale-down") ? WE_FIT_SCALE_DOWN : -1;
        if (fit < 0) diag(d, "object fit refused", -1);
        else { n->object_fit = (uint8_t)fit; n->object_fit_set = 1; }
    } else if (w_eq(prop, "object-position")) {
        if (parse_object_position(n, v, vl)) diag(d, "object position refused", -1);
    } else if (w_eq(prop, "grid-column") || w_eq(prop, "grid-row")) {
        if (grid_request(n, w_eq(prop, "grid-row"), v, vl)) diag(d, "grid placement refused", -1);
    } else if (w_eq(prop, "box-sizing")) {
        if (value_is(v, vl, "border-box")) n->border_box = 1;
        else if (value_is(v, vl, "content-box")) n->border_box = 0;
        else diag(d, "box sizing refused", -1);
    } else if (w_eq(prop, "white-space")) {
        if (value_is(v, vl, "normal")) { n->nowrap = 0; n->ws_mode = WE_WS_NORMAL; }
        else if (value_is(v, vl, "nowrap")) { n->nowrap = 1; n->ws_mode = WE_WS_NORMAL; }
        else if (value_is(v, vl, "pre")) { n->nowrap = 1; n->ws_mode = WE_WS_PRE; }
        else if (value_is(v, vl, "pre-wrap")) { n->nowrap = 0; n->ws_mode = WE_WS_PRE_WRAP; }
        else if (value_is(v, vl, "pre-line")) { n->nowrap = 0; n->ws_mode = WE_WS_PRE_LINE; }
        else if (value_is(v, vl, "break-spaces")) { n->nowrap = 0; n->ws_mode = WE_WS_BREAK_SPACES; }
        else diag(d, "white space refused", -1);
    } else if (w_eq(prop, "hyphens")) {
        if (value_is(v, vl, "none")) n->hyphens = WE_HYPHENS_NONE;
        else if (value_is(v, vl, "manual") || value_is(v, vl, "auto")) n->hyphens = WE_HYPHENS_MANUAL;  /* no dictionary */
        else diag(d, "hyphens refused", -1);
    } else if (w_eq(prop, "tab-size")) {
        if (tab_size_value(n, v, vl)) diag(d, "tab size refused", -1);
    } else if (w_eq(prop, "overflow-wrap")) {
        if (value_is(v, vl, "normal")) n->break_word = WE_WRAP_NORMAL;
        else if (value_is(v, vl, "break-word")) n->break_word = WE_WRAP_BREAK_WORD;
        else if (value_is(v, vl, "anywhere")) n->break_word = WE_WRAP_ANYWHERE;
        else diag(d, "word wrapping refused", -1);
    } else if (w_eq(prop, "word-break")) {
        /* keep-all only differs for CJK text; break-word is the legacy
         * normal + overflow-wrap: anywhere */
        if (value_is(v, vl, "normal") || value_is(v, vl, "break-all") || value_is(v, vl, "keep-all")) n->break_all = (uint8_t)value_is(v, vl, "break-all");
        else if (value_is(v, vl, "break-word")) { n->break_all = 0; n->break_word = WE_WRAP_ANYWHERE; }
        else diag(d, "word break refused", -1);
    } else if (w_eq(prop, "white-space-collapse") || w_eq(prop, "text-wrap-mode") || w_eq(prop, "text-wrap")) {
        if (white_space_longhand(n, prop, v, vl)) diag(d, "white space refused", -1);
    } else if (w_eq(prop, "overflow") || w_eq(prop, "overflow-x") || w_eq(prop, "overflow-y")) {
        if (overflow_declaration(n, prop, v, vl)) diag(d, "overflow refused", -1);
    } else if (w_eq(prop, "visibility")) {
        if (value_is(v, vl, "visible") || value_is(v, vl, "hidden")) n->visibility = (uint8_t)value_is(v, vl, "hidden");
        else diag(d, "visibility refused", -1);
    } else if (w_eq(prop, "float")) {
        if (value_is(v, vl, "left")) n->float_side = WE_FLOAT_LEFT;
        else if (value_is(v, vl, "right")) n->float_side = WE_FLOAT_RIGHT;
        else if (value_is(v, vl, "none")) n->float_side = WE_FLOAT_NONE;
        else diag(d, "float refused", -1);
    } else if (w_eq(prop, "clear")) {
        if (value_is(v, vl, "none")) n->clear_side = WE_CLEAR_NONE;
        else if (value_is(v, vl, "left")) n->clear_side = WE_CLEAR_LEFT;
        else if (value_is(v, vl, "right")) n->clear_side = WE_CLEAR_RIGHT;
        else if (value_is(v, vl, "both")) n->clear_side = WE_CLEAR_BOTH;
        else diag(d, "clear refused", -1);
    } else if (w_eq(prop, "opacity")) {
        if (parse_opacity(v, vl, &px) == 0) n->transparency = (uint8_t)(100 - px);
        else diag(d, "opacity refused", -1);
    } else if (w_eq(prop, "text-decoration") || w_eq(prop, "text-decoration-line") || w_eq(prop, "text-decoration-style") ||
               w_eq(prop, "text-decoration-color") || w_eq(prop, "text-decoration-thickness")) {
        int kind = w_eq(prop, "text-decoration") ? 0 : w_eq(prop, "text-decoration-line") ? 1 :
                   w_eq(prop, "text-decoration-style") ? 2 : w_eq(prop, "text-decoration-color") ? 3 : 4;
        if (parse_decoration(n, kind, v, vl)) diag(d, "text-decoration refused", -1);
    } else if (w_eq(prop, "outline")) {
        if (parse_outline(n, v, vl)) diag(d, "outline refused", -1);
    } else if (w_eq(prop, "outline-style")) {
        int code = outline_style_value(v, vl);
        if (code) n->outline_style = (uint8_t)(code == WE_BS_NONE ? 0 : code);
        else diag(d, "outline refused", -1);
    } else if (w_eq(prop, "outline-width")) {
        if (outline_width_value(v, vl, &px) == 0) n->outline_w = (uint8_t)px;
        else diag(d, "outline refused", -1);
    } else if (w_eq(prop, "outline-color")) {
        if (parse_color(v, vl, &col) == 0) { n->outline_color = col; n->outline_color_set = 1; }
        else diag(d, "outline refused", -1);
    } else if (w_eq(prop, "outline-offset")) {
        int16_t value;
        int percent;
        if (tf_length(v, vl, &value, &percent) == 0 && !percent) n->outline_offset = (int8_t)(value < -32 ? -32 : value > 32 ? 32 : value);
        else diag(d, "outline refused", -1);
    } else if (w_eq(prop, "box-shadow")) {
        if (parse_shadows(n->shadow, &n->shadow_count, v, vl, 0)) diag(d, "box-shadow refused", -1);
    } else if (w_eq(prop, "text-shadow")) {
        if (parse_shadows(n->tshadow, &n->tshadow_count, v, vl, 1)) diag(d, "text-shadow refused", -1);
    } else if (w_eq(prop, "border-radius")) {
        if (parse_radius(n, v, vl)) diag(d, "border-radius refused", -1);
    } else if (w_eq(prop, "border-top-left-radius") || w_eq(prop, "border-top-right-radius") ||
               w_eq(prop, "border-bottom-right-radius") || w_eq(prop, "border-bottom-left-radius")) {
        int corner = w_eq(prop, "border-top-left-radius") ? 0 : w_eq(prop, "border-top-right-radius") ? 1 :
                     w_eq(prop, "border-bottom-right-radius") ? 2 : 3;
        if (parse_corner(n, corner, v, vl)) diag(d, "border-radius refused", -1);
    } else if (w_eq(prop, "text-overflow")) {
        if (value_is(v, vl, "clip")) n->text_overflow = 0;
        else if (value_is(v, vl, "ellipsis")) n->text_overflow = 1;
        else diag(d, "text-overflow refused", -1);
    } else if (w_eq(prop, "text-transform")) {
        if (value_is(v, vl, "none")) n->text_transform = WE_TT_NONE;
        else if (value_is(v, vl, "uppercase")) n->text_transform = WE_TT_UPPER;
        else if (value_is(v, vl, "lowercase")) n->text_transform = WE_TT_LOWER;
        else if (value_is(v, vl, "capitalize")) n->text_transform = WE_TT_CAPITALIZE;
        else diag(d, "text-transform refused", -1);
    } else if (w_eq(prop, "background-clip") || w_eq(prop, "background-origin")) {
        int box = box_keyword(v, vl);
        if (box < 0) diag(d, "background box refused", -1);
        else if (w_eq(prop, "background-clip")) n->bg_clip = (uint8_t)box;
        else n->bg_origin = (uint8_t)box;
    } else if (w_eq(prop, "transform")) {
        if (parse_transform(n, v, vl)) diag(d, "transform refused", -1);
    } else if (w_eq(prop, "transform-origin")) {
        if (parse_transform_origin(n, v, vl)) diag(d, "transform origin refused", -1);
    } else if (w_eq(prop, "z-index")) {
        if (value_is(v, vl, "auto")) { n->z_set = 0; n->z_index = 0; }
        else if (bounded_signed(v, vl, WE_Z_MAX, &px) == 0) { n->z_index = (int8_t)px; n->z_set = 1; }
        else diag(d, "z-index refused", -1);
    } else if (w_eq(prop, "line-height")) {
        int end = vl, number = 0, unitless;
        if (value_is(v, vl, "normal")) n->line_height = 0;
        else {
            if (vl >= 2 && value_is(v + vl - 2, 2, "px")) end -= 2;
            unitless = end == vl;
            if (bounded_uint(v, end, unitless ? 4 : WE_LINE_H_MAX, &number) == 0 &&
                number >= (unitless ? 1 : WE_LINE_H))
                n->line_height = (int16_t)(unitless ? number * WE_LINE_H : number);
            else diag(d, "line height refused", -1);
        }
    } else if (w_eq(prop, "text-indent")) {
        int pct = 0;
        if (parse_len(v, vl, &px, &pct) == 0 && pct == 0 && px >= 0 && px <= WE_INDENT_MAX)
            n->text_indent = (int16_t)px;
        else diag(d, "text indent refused", -1);
    } else if (w_eq(prop, "vertical-align")) {
        if (value_is(v, vl, "top")) n->valign = WE_VALIGN_TOP;
        else if (value_is(v, vl, "middle")) n->valign = WE_VALIGN_MIDDLE;
        else if (value_is(v, vl, "bottom")) n->valign = WE_VALIGN_BOTTOM;
        else if (value_is(v, vl, "baseline")) n->valign = WE_VALIGN_BASELINE;
        else diag(d, "vertical align refused", -1);
    } else if (w_eq(prop, "position")) {
        if (value_is(v, vl, "relative")) { n->relative = 1; n->absolute = n->fixed = 0; }
        else if (value_is(v, vl, "absolute")) { n->absolute = 1; n->relative = n->fixed = 0; }
        else if (value_is(v, vl, "fixed")) { n->absolute = n->fixed = 1; n->relative = 0; }
        else if (value_is(v, vl, "static")) { n->relative = 0; n->absolute = n->fixed = 0; }
        else diag(d, "position refused", -1);
    } else if (w_eq(prop, "border-collapse")) {
        if (value_is(v, vl, "collapse")) n->border_collapse = 1;
        else if (value_is(v, vl, "separate")) n->border_collapse = 0;
        else diag(d, "border collapse refused", -1);
    } else if (w_eq(prop, "border-spacing")) {
        int a[4], lens[4], count = tokens(v, vl, a, lens), t;
        int hx = 0, hy = 0, ok = count == 1 || count == 2;
        for (t = 0; t < count && t < 2 && ok; t++) {
            int end = lens[t], number = 0;
            const char *tok = v + a[t];
            if (end >= 2 && value_is(tok + end - 2, 2, "px")) end -= 2;
            if (bounded_uint(tok, end, WE_SPACING_MAX, &number)) ok = 0;
            else if (t == 0) hx = hy = number;
            else hy = number;
        }
        if (!ok) diag(d, "border spacing refused", -1);
        else { n->border_spacing = (int16_t)hx; n->border_spacing_y = (int16_t)hy; }
    } else if (w_eq(prop, "list-style-type")) {
        if (value_is(v, vl, "none")) n->list_style = WE_LIST_NONE;
        else if (value_is(v, vl, "disc")) n->list_style = WE_LIST_DISC;
        else if (value_is(v, vl, "decimal")) n->list_style = WE_LIST_DECIMAL;
        else if (value_is(v, vl, "square")) n->list_style = WE_LIST_SQUARE;
        else if (list_keyword(v, vl) >= 0) n->list_style = (uint8_t)list_keyword(v, vl);
        else diag(d, "list marker refused", -1);
    } else if (w_eq(prop, "column-count")) {
        int value;
        if (column_count_value(v, vl, &value)) diag(d, "columns refused", -1);
        else n->col_count = (uint8_t)value;
    } else if (w_eq(prop, "column-width")) {
        int value;
        if (column_width_value(v, vl, &value)) diag(d, "columns refused", -1);
        else n->col_width = (int16_t)value;
    } else if (w_eq(prop, "columns")) {
        if (parse_columns(n, v, vl)) diag(d, "columns refused", -1);
    } else if (w_eq(prop, "column-fill")) {
        if (value_is(v, vl, "auto")) n->col_fill_auto = 1;
        else if (value_is(v, vl, "balance")) n->col_fill_auto = 0;
        else diag(d, "columns refused", -1);
    } else if (w_eq(prop, "column-span")) {
        if (value_is(v, vl, "none") || value_is(v, vl, "all")) n->col_span_all = (uint8_t)value_is(v, vl, "all");
        else diag(d, "column span refused", -1);
    } else if (w_eq(prop, "break-before") || w_eq(prop, "break-after")) {
        /* a forced break is kept between columns only; avoid has nothing to
         * do since a box is never fragmented and units break between boxes */
        int bit = prop[6] == 'b' ? 1 : 2;
        if (value_is(v, vl, "column")) n->col_break |= (uint8_t)bit;
        else if (value_is(v, vl, "auto") || value_is(v, vl, "avoid") || value_is(v, vl, "avoid-column")) n->col_break &= (uint8_t)~bit;
        else diag(d, "break refused", -1);
    } else if (w_eq(prop, "break-inside")) {
        if (!value_is(v, vl, "auto") && !value_is(v, vl, "avoid") && !value_is(v, vl, "avoid-column")) diag(d, "break refused", -1);
    } else if (w_eq(prop, "column-rule")) {
        if (parse_column_rule(n, v, vl)) diag(d, "column rule refused", -1);
    } else if (w_eq(prop, "column-rule-style")) {
        int code = border_style_code(v, vl);
        if (!code) diag(d, "column rule refused", -1);
        else n->rule_style = (uint8_t)code;
    } else if (w_eq(prop, "column-rule-width")) {
        int w;
        if (outline_width_value(v, vl, &w)) diag(d, "column rule refused", -1);
        else n->rule_w = (uint8_t)w;
    } else if (w_eq(prop, "column-rule-color")) {
        uint32_t color;
        if (parse_color(v, vl, &color)) diag(d, "column rule refused", -1);
        else { n->rule_color = color; n->rule_color_set = 1; }
    } else if (w_eq(prop, "list-style-image")) {
        /* lot 64: an image marker; the type stays the fallback */
        if (value_is(v, vl, "none")) n->lsi_len = 0;
        else if (url_span(d, v, vl, &n->lsi_off, &n->lsi_len)) diag(d, "list marker refused", -1);
    } else if (w_eq(prop, "list-style")) {
        int a[4], l[4], c = tokens(v, vl, a, l), k, style = -1, ok = c >= 1, image = -1;
        uint16_t off = 0, len = 0;
        for (k = 0; k < c && ok; k++) {
            int kw = list_keyword(v + a[k], l[k]);
            if (value_is(v + a[k], l[k], "outside")) continue;
            if (l[k] > 4 && w_ieq_n(v + a[k], 4, "url(")) {      /* lot 64: list-style-image */
                if (image >= 0 || url_span(d, v + a[k], l[k], &off, &len)) ok = 0;
                image = k;
                continue;
            }
            if (kw < 0 || style >= 0) ok = 0;
            else style = kw;
        }
        if (!ok) diag(d, "list marker refused", -1);
        else {
            n->list_style = (uint8_t)(style < 0 ? WE_LIST_DISC : style);
            n->lsi_off = off; n->lsi_len = len;
        }
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
        if (gap_declaration(n, v, vl)) diag(d, "gap refused", -1);
    } else if (w_eq(prop, "grid-template-columns") || w_eq(prop, "grid-template-rows")) {
        int16_t sizes[WE_TRACKS_MAX], floors[WE_TRACKS_MAX], caps[WE_TRACKS_MAX];
        we_grid_names_t names;
        we_auto_repeat_t automatic;
        int count, axis = w_eq(prop, "grid-template-rows");
        if (value_is(v, vl, "subgrid")) {
            n->subgrid[axis] = 1;
            n->auto_mode[axis] = n->tpl_count[axis] = 0;
            w_zero(&n->line_names[axis], sizeof(n->line_names[axis]));
            if (axis) n->grid_rows = 0; else n->grid_cols = 0;
            return;
        }
        if (vl > 8 && w_ieq_n(v, 7, "subgrid") && (v[7] == ' ' || v[7] == '[')) {
            diag(d, "subgrid line names refused", -1);
            return;
        }
        count = named_track_list(v, vl, sizes, floors, caps, &names, &automatic);
        if (count >= 0) n->subgrid[axis] = 0;
        if (count >= 0) {
            n->auto_mode[axis] = (uint8_t)automatic.mode;
            n->auto_at[axis] = (uint8_t)automatic.at;
            n->auto_count[axis] = 1;
            n->tpl_count[axis] = (uint8_t)count;
            for (k = 0; k < count; k++) {
                n->tpl_size[axis][k] = sizes[k]; n->tpl_floor[axis][k] = floors[k]; n->tpl_cap[axis][k] = caps[k];
            }
        }
        if (count < 0) diag(d, "grid tracks refused", -1);
        else if (!axis) {
            n->line_names[0] = names;
            n->grid_cols = (uint8_t)count;
            for (k = 0; k < count; k++) { n->col_size[k] = sizes[k]; n->col_floor[k] = floors[k]; n->col_cap[k] = caps[k]; }
        } else {
            n->line_names[1] = names;
            n->grid_rows = (uint8_t)count;
            for (k = 0; k < count; k++) { n->row_size[k] = sizes[k]; n->row_floor[k] = floors[k]; n->row_cap[k] = caps[k]; }
        }
    } else if (w_eq(prop, "color")) {
        if (parse_color(v, vl, &col) == 0) n->color = col;
        else if (vl > 0 && v[vl - 1] == ')') diag(d, "color refused", -1);
    }
    else if (w_eq(prop, "background") && shorthand_layers(v, vl)) {
        if (background_shorthand(d, n, v, vl)) diag(d, "background refused", -1);
    }
    else if ((w_eq(prop, "background") || w_eq(prop, "background-image")) && vl > 16 &&
             (w_ieq_n(v, 16, "linear-gradient(") || w_ieq_n(v, 16, "radial-gradient("))) {
        if (w_eq(prop, "background-image") && list_count(v, vl) > 1) {
            if (background_layers(d, n, v, vl)) diag(d, "background image refused", -1);
        } else if (parse_gradient(n, v, vl)) diag(d, "gradient refused", -1);
        else {
            n->bgi_len = 0; n->bgi_layers = 0;
            if (w_eq(prop, "background")) n->has_bg = 0;
        }
    }
    else if (w_eq(prop, "background-image")) {
        if (background_layers(d, n, v, vl)) diag(d, "background image refused", -1);
    }
    else if (w_eq(prop, "background-repeat")) {
        if (background_list(n, v, vl, 0)) diag(d, "background repeat refused", -1);
    }
    else if (w_eq(prop, "background-position")) {
        if (background_list(n, v, vl, 1)) diag(d, "background position refused", -1);
    }
    else if (w_eq(prop, "background-position-x") || w_eq(prop, "background-position-y")) {
        if (background_list(n, v, vl, prop[20] == 'x' ? 3 : 4)) diag(d, "background position refused", -1);
    }
    else if (w_eq(prop, "background-size")) {
        if (background_list(n, v, vl, 2)) diag(d, "background size refused", -1);
    }
    else if (w_eq(prop, "background") || w_eq(prop, "background-color")) {
        if (w_eq(prop, "background")) n->grad_dir = 0, n->bgi_len = 0, n->bgi_layers = 0;
        if (w_ieq_n(v, vl, "none") || w_ieq_n(v, vl, "transparent")) n->has_bg = 0;
        else if (parse_color(v, vl, &col) == 0) { n->bg = col; n->has_bg = 1; }
        else if (vl > 0 && v[vl - 1] == ')') diag(d, "color refused", -1);
    }
    else if (w_eq(prop, "margin")) { if (!margin_auto_box(n, v, vl)) box4(n, 0, v, vl, n->margin); }
    else if (w_eq(prop, "padding")) box4(n, 1, v, vl, n->padding);
    else if (w_eq(prop, "margin-top")) {
        if (value_is(v, vl, "auto")) { n->margin[0] = 0; n->margin_auto |= WE_AUTO_TOP; spacing_set(n, 0, 0, WE_NO_PCT); }
        else if (spacing_percent(v, vl, 1, &pct) == 0) { spacing_set(n, 0, 0, pct); n->margin_auto &= (uint8_t)~WE_AUTO_TOP; }
        else if (parse_len(v, vl, &px, 0) == 0) { n->margin[0] = (int16_t)px; n->margin_auto &= (uint8_t)~WE_AUTO_TOP; spacing_set(n, 0, 0, WE_NO_PCT); }
    }
    else if (w_eq(prop, "margin-right")) {
        if (value_is(v, vl, "auto")) { n->margin[1] = 0; n->margin_auto |= WE_AUTO_RIGHT; spacing_set(n, 0, 1, WE_NO_PCT); }
        else if (spacing_percent(v, vl, 1, &pct) == 0) { spacing_set(n, 0, 1, pct); n->margin_auto &= (uint8_t)~WE_AUTO_RIGHT; }
        else if (parse_len(v, vl, &px, 0) == 0) { n->margin[1] = (int16_t)px; n->margin_auto &= (uint8_t)~WE_AUTO_RIGHT; spacing_set(n, 0, 1, WE_NO_PCT); }
    }
    else if (w_eq(prop, "margin-bottom")) {
        if (value_is(v, vl, "auto")) { n->margin[2] = 0; n->margin_auto |= WE_AUTO_BOTTOM; spacing_set(n, 0, 2, WE_NO_PCT); }
        else if (spacing_percent(v, vl, 1, &pct) == 0) { spacing_set(n, 0, 2, pct); n->margin_auto &= (uint8_t)~WE_AUTO_BOTTOM; }
        else if (parse_len(v, vl, &px, 0) == 0) { n->margin[2] = (int16_t)px; n->margin_auto &= (uint8_t)~WE_AUTO_BOTTOM; spacing_set(n, 0, 2, WE_NO_PCT); }
    }
    else if (w_eq(prop, "margin-left")) {
        if (value_is(v, vl, "auto")) { n->margin[3] = 0; n->margin_auto |= WE_AUTO_LEFT; spacing_set(n, 0, 3, WE_NO_PCT); }
        else if (spacing_percent(v, vl, 1, &pct) == 0) { spacing_set(n, 0, 3, pct); n->margin_auto &= (uint8_t)~WE_AUTO_LEFT; }
        else if (parse_len(v, vl, &px, 0) == 0) { n->margin[3] = (int16_t)px; n->margin_auto &= (uint8_t)~WE_AUTO_LEFT; spacing_set(n, 0, 3, WE_NO_PCT); }
    }
    else if (w_eq(prop, "padding-left")) {
        if (spacing_percent(v, vl, 0, &pct) == 0) spacing_set(n, 1, 3, pct);
        else if (parse_len(v, vl, &px, 0) == 0 && px >= 0) { n->padding[3] = (int16_t)px; spacing_set(n, 1, 3, WE_NO_PCT); }
        else diag(d, "padding refused", -1);
    }
    else if (w_eq(prop, "width")) {
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
            int code = border_style_code(v + a[k], l[k]);
            if (code) n->border_style = (uint8_t)code;
            else if (parse_color(v + a[k], l[k], &col) == 0) n->border_color = col;
            else if (parse_len(v + a[k], l[k], &px, 0) == 0) n->border = (int16_t)px;
        }
        if (c == 1 && w_ieq_n(v, vl, "none")) n->border = 0;
        border_edges_reset(n);
    } else if (w_eq(prop, "border-top") || w_eq(prop, "border-right") ||
               w_eq(prop, "border-bottom") || w_eq(prop, "border-left")) {
        int side = w_eq(prop, "border-top") ? 0 : w_eq(prop, "border-right") ? 1 : w_eq(prop, "border-bottom") ? 2 : 3;
        if (border_edge_decl(n, side, v, vl)) diag(d, "border edge refused", -1);
    } else if (edge_longhand(prop) >= 0) {
        if (border_edge_part(n, edge_longhand(prop) >> 2, edge_longhand(prop) & 3, v, vl)) diag(d, "border edge refused", -1);
    } else if (w_ieq_n(prop, 13, "border-inline") || w_ieq_n(prop, 12, "border-block")) {
        if (border_logical_pair(n, prop, v, vl)) diag(d, "border edge refused", -1);
    } else if (w_eq(prop, "border-style") || w_eq(prop, "border-width") || w_eq(prop, "border-color")) {
        int kind = w_eq(prop, "border-style") ? 0 : w_eq(prop, "border-width") ? 1 : 2;
        if (border_list_decl(n, kind, v, vl)) diag(d, "border edge refused", -1);
    } else if (w_eq(prop, "font-weight")) {
        n->bold = (uint8_t)(w_ieq_n(v, vl, "bold") || w_ieq_n(v, vl, "700") || w_ieq_n(v, vl, "800") || w_ieq_n(v, vl, "900"));
    } else if (w_eq(prop, "text-align")) {
        int logical;
        n->align = (uint8_t)align_keyword(v, vl, WE_ALIGN_LEFT, &logical);
        n->align_logical = (uint8_t)((n->align_logical & 12) | logical);
    } else if (w_eq(prop, "text-align-last")) {
        /* Lot 62 round 2: alignment of the last line and of lines before a
         * forced break (left-to-right: start is left, end is right) */
        int logical = 0, a = value_is(v, vl, "auto") ? -1 : align_keyword(v, vl, -2, &logical);
        if (a == -2) diag(d, "text-align-last refused", -1);
        else {
            n->align_last = (uint8_t)(a + 1);
            n->align_logical = (uint8_t)((n->align_logical & 3) | logical << 2);
        }
    } else if (w_eq(prop, "text-justify")) {
        if (value_is(v, vl, "none")) n->justify_none = 1;
        else if (value_is(v, vl, "auto") || value_is(v, vl, "inter-word")) n->justify_none = 0;
        else diag(d, "text-justify refused", -1);
    } else if (w_eq(prop, "letter-spacing") || w_eq(prop, "word-spacing")) {
        /* Lot 62 round 2: px (or em) added after each glyph, or to each
         * word space; letter -4..16, word -8..32 */
        int letter = w_eq(prop, "letter-spacing"), low = letter ? -4 : -8, high = letter ? 16 : 32;
        if (value_is(v, vl, "normal")) px = 0;
        else if (parse_len(v, vl, &px, 0) != 0 || px < low || px > high) { diag(d, letter ? "letter-spacing refused" : "word-spacing refused", -1); px = letter ? n->letter_spacing : n->word_spacing; }
        if (letter) n->letter_spacing = (int8_t)px; else n->word_spacing = (int8_t)px;
    } else {
        d->perf.decls_applied--;
        d->perf.unknown_props++;
    }
}

static void apply_decls(we_doc_t* d, we_node_t* n, const char* s, int len, int important) {
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
        i = decl_end(s, len, i, ';');
        vb = i;
        {
            int end = vb, marked = 0;
            while (end > va && w_space(s[end - 1])) end--;
            if (end - va >= 9 && w_ieq_n(s + end - 9, 9, "important")) {
                int bang = end - 9;
                while (bang > va && w_space(s[bang - 1])) bang--;
                if (bang > va && s[bang - 1] == '!') { marked = 1; end = bang - 1; }
            }
            if (marked == important && pb > pa && end > va) apply_decl(d, n, s + pa, pb - pa, s + va, end - va);
        }
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
    if ((w_eq(t, "ol") || w_eq(t, "ul")) && n->list_type_attr) n->list_style = n->list_type_attr;
    if (w_eq(t, "body")) { for (i = 0; i < 4; i++) n->margin[i] = 8; }
    if (w_eq(t, "p") || w_eq(t, "ul") || w_eq(t, "ol")) { n->margin[0] = 8; n->margin[2] = 8; }
    if (w_eq(t, "h1") || w_eq(t, "h2") || w_eq(t, "h3") || w_eq(t, "h4")) { n->bold = 1; n->margin[0] = 8; n->margin[2] = 8; }
    if (w_eq(t, "ul")) n->padding[3] = 16;
    if (w_eq(t, "ol")) n->padding[3] = 40;
    if (w_eq(t, "b") || w_eq(t, "strong") || w_eq(t, "th")) n->bold = 1;
    if (w_eq(t, "a")) n->color = 0x0645AD;
    if (w_eq(t, "u") || w_eq(t, "ins")) n->deco_line = WE_DECO_UNDER;
    if (w_eq(t, "s") || w_eq(t, "strike") || w_eq(t, "del")) n->deco_line = WE_DECO_THROUGH;
    if (w_eq(t, "hr")) { n->height = 2; n->bg = 0x808080; n->has_bg = 1; n->margin[0] = 6; n->margin[2] = 6; }
    if (w_eq(t, "blockquote")) { n->margin[3] = 24; n->margin[1] = 24; }
    if (w_eq(t, "pre")) { n->display = WE_DISP_BLOCK; n->ws_mode = WE_WS_PRE; n->nowrap = 1; }
}

/* Rules for element idx (pseudo 0) or for its ::before/::after box n. */
static void cascade_rules(we_doc_t *d, int idx, we_node_t *n, int pseudo, int important) {
    uint8_t used[WE_MAX_RULES];
    uint32_t k, r;
    for (k = 0; k < d->nrules; k++) used[k] = (uint8_t)(d->rules[k].pseudo_el != pseudo || !match_rule(d, idx, &d->rules[k]));
    for (k = 0; k < d->nrules; k++) {
        int best = -1;
        for (r = 0; r < d->nrules; r++) {
            if (used[r]) continue;
            if (best < 0 || d->rules[r].spec < d->rules[best].spec ||
                (d->rules[r].spec == d->rules[best].spec && d->rules[r].order < d->rules[best].order)) best = (int)r;
        }
        if (best < 0) break;
        used[best] = 1;
        apply_decls(d, n, d->pool + d->rules[best].decl_off, d->rules[best].decl_len, important);
    }
    if (n->style_len) apply_decls(d, n, d->pool + n->style_off, n->style_len, important);
}

/* Computes node i: element i matches its own rules (pseudo 0); a generated
 * box matches the ::before/::after rules of its element match. */
static void style_node(we_doc_t *d, we_node_t *n, int i, int match, int pseudo) {
    {
        const we_node_t* p = n->parent >= 0 ? &d->nodes[n->parent] : 0;
        n->color = p ? p->color : 0x000000;
        n->bold = p ? p->bold : 0;
        n->align = p ? p->align : WE_ALIGN_LEFT;
        n->align_logical = p ? p->align_logical : 1;    /* initial text-align: start */
        n->width = -1; n->height = -1; n->width_pct = 0;
        w_zero(n->length, sizeof(n->length)); n->aspect_num = 0; n->aspect_den = 1;
        n->flex_grow = -1;
        n->flex_shrink = n->flex_basis = -1;
        n->basis_pct = n->basis_set = 0;
        n->min_width = n->max_width = -1;
        n->min_height = n->max_height = -1;
        n->row_gap = n->col_gap = -1;
        n->gap_set = 0;
        n->height_percent = n->min_height_percent = n->max_height_percent = 0;
        n->height_pct = n->min_height_pct = n->max_height_pct = 0;
        n->width_kind = n->basis_content = n->border_box = n->overflow_hidden = n->overflow_axes = 0;
        n->min_width_kind = n->max_width_kind = n->basis_kind = 0;
        n->justify_content = n->align_items = n->align_self = n->align_content = 0;
        n->justify_items = n->justify_self = 0;
        w_zero(n->line_names, sizeof(n->line_names));
        n->auto_mode[0] = n->auto_mode[1] = n->tpl_count[0] = n->tpl_count[1] = 0;
        n->subgrid[0] = n->subgrid[1] = 0;
        n->sub_gap[0] = n->sub_gap[1] = 0;
        w_zero(n->grid_request, sizeof(n->grid_request));
        n->grid_col_start = n->grid_row_start = 0;
        n->grid_dense = n->grid_column_flow = n->area_count = n->area_rows = n->area_cols = 0;
        n->grid_area[0] = 0;
        n->object_fit = n->object_fit_set = 0; n->object_x = n->object_y = 50;
        n->auto_col = -1; n->auto_row = WE_TRACK_AUTO;
        n->auto_col_floor = n->auto_row_floor = n->auto_col_cap = n->auto_row_cap = -1;
        n->relative = n->absolute = n->flex_reverse = n->margin_auto = n->border_collapse = 0;
        n->fixed = 0;
        n->border_style = n->collapse_cell = 0;
        border_edges_reset(n);
        n->item_order = 0;
        n->inset_flags = 0;
        n->offset_x = n->offset_y = n->offset_r = n->offset_b = 0;
        n->border_spacing = n->border_spacing_y = 0;
        n->clear_side = n->valign = n->z_set = 0;
        n->transparency = 0;
        n->tf_count = 0; n->tf_origin_pct = 3; n->tf_origin[0] = n->tf_origin[1] = 50;
        n->outline_style = 0; n->outline_w = 3; n->outline_offset = 0; n->outline_color_set = 0;
        n->shadow_count = 0;
        n->deco_line = n->deco_color_set = 0; n->deco_style = WE_BS_SOLID; n->deco_thick = 1;
        n->spacing_pct = 0; n->grad_dir = 0;
        n->bg_clip = WE_BOX_BORDER; n->bg_origin = WE_BOX_PADDING; n->text_overflow = 0;
        bgi_reset(n);
        w_zero(n->rad_x, sizeof(n->rad_x)); w_zero(n->rad_y, sizeof(n->rad_y)); n->rad_pct = 0;
        n->text_transform = p ? p->text_transform : WE_TT_NONE;
        n->letter_spacing = p ? p->letter_spacing : 0;
        n->word_spacing = p ? p->word_spacing : 0;
        n->align_last = p ? p->align_last : 0;
        n->justify_none = p ? p->justify_none : 0;
        n->tshadow_count = p ? p->tshadow_count : 0;
        if (p) { int k; for (k = 0; k < WE_SHADOWS_MAX; k++) n->tshadow[k] = p->tshadow[k]; }
        n->z_index = 0;
        n->line_height = p ? p->line_height : 0;
        n->text_indent = p ? p->text_indent : 0;
        n->ws_mode = p ? p->ws_mode : 0;
        n->tab_px = p ? p->tab_px : 8 * WE_CHAR_W;
        n->hyphens = p ? p->hyphens : WE_HYPHENS_MANUAL;
        n->wmode = p ? p->wmode : WE_WM_HORIZONTAL;
        n->dir_rtl = p ? p->dir_rtl : 0;
        n->upright = p ? p->upright : 0;
        n->nowrap = p ? p->nowrap : 0;
        n->break_word = p ? p->break_word : 0;
        n->break_all = p ? p->break_all : 0;
        n->visibility = p ? p->visibility : 0;
        n->grid_col_span = n->grid_row_span = 1;
        { int track; for (track = 0; track < WE_TRACKS_MAX; track++) {
            n->col_size[track] = -1;
            n->row_size[track] = WE_TRACK_AUTO;
            n->col_floor[track] = n->row_floor[track] = n->col_cap[track] = n->row_cap[track] = -1;
        } }
        n->list_style = p ? p->list_style : WE_LIST_NONE;
        n->lsi_off = p ? p->lsi_off : 0; n->lsi_len = p ? p->lsi_len : 0;
        n->col_count = 0; n->col_width = -1; n->col_fill_auto = 0; n->col_span_all = 0; n->col_break = 0;
        n->rule_w = 3; n->rule_style = 0; n->rule_color_set = 0; n->rule_color = 0;
        n->border_color = n->color;
        n->quotes_kind = p ? p->quotes_kind : WE_QUOTES_AUTO;
        n->quotes_off = p ? p->quotes_off : 0; n->quotes_len = p ? p->quotes_len : 0;
        n->content_kind = WE_CONTENT_NONE;
        if (n->is_text) { n->display = WE_DISP_INLINE; return; }
        if (i == 0) { n->display = WE_DISP_BLOCK; return; }
        if (pseudo) {
            /* UA: q::before { content: open-quote } q::after { content: close-quote } */
            n->display = WE_DISP_INLINE;
            if (w_eq(d->nodes[match].tag, "q")) n->content_kind = pseudo == WE_PSEUDO_BEFORE ? WE_CONTENT_UA_OPEN : WE_CONTENT_UA_CLOSE;
        } else ua_defaults(n);
        if (!pseudo && n->dir_attr) n->dir_rtl = (uint8_t)(n->dir_attr == 2);
        /* lot 64: writing-mode and direction first, so that the logical
         * properties of the same element map to the right sides */
        mode_pass = 1;
        cascade_rules(d, match, n, pseudo, 0);
        cascade_rules(d, match, n, pseudo, 1);
        mode_pass = 0;
        /* lot 64: the UA list indent is padding-inline-start: the right in rtl */
        if (!pseudo && n->dir_rtl && n->wmode == WE_WM_HORIZONTAL && (w_eq(n->tag, "ul") || w_eq(n->tag, "ol")))
            n->padding[1] = n->padding[3], n->padding[3] = 0;
        cascade_rules(d, match, n, pseudo, 0);
        cascade_rules(d, match, n, pseudo, 1);
        /* lot 64: start is left in ltr and right in rtl, end the other way */
        if (n->align_logical & 3) n->align = ((n->align_logical & 3) == 1) != n->dir_rtl ? WE_ALIGN_LEFT : WE_ALIGN_RIGHT;
        if (n->align_logical >> 2) n->align_last = (uint8_t)(1 + (((n->align_logical >> 2) == 1) != n->dir_rtl ? WE_ALIGN_LEFT : WE_ALIGN_RIGHT));
        if (n->border && n->border_color == (p ? p->color : 0)) n->border_color = n->color;
        if (p && p->display == WE_DISP_NONE) n->display = WE_DISP_NONE;
    }
}

/* ---------- Lot 63: ::before/::after boxes, counters and quotes ----------
 * After the cascade, each element whose ::before or ::after rules give a
 * content value gets an inline child box (first or last child). A tree-order
 * pass then applies counter-reset, counter-increment and counter-set (in
 * that order, CSS Lists 3), scopes counters to the element, its following
 * siblings and their descendants, and gives each generated box one text
 * child with the evaluated content. */
static void style_node(we_doc_t *d, we_node_t *n, int i, int match, int pseudo);

static int gen_allowed(const we_doc_t *d, int el) {
    static const char *const replaced[] = { "img", "input", "br", "textarea", "select", "iframe", "video", "audio", "canvas", "object", 0 };
    const we_node_t *e = &d->nodes[el];
    int k;
    if (e->is_text || e->pseudo_kind || e->display == WE_DISP_NONE) return 0;
    for (k = 0; replaced[k]; k++) if (w_eq(e->tag, replaced[k])) return 0;
    return 1;
}

static void unlink_last(we_doc_t *d, int parent, int old_last) {
    we_node_t *p = &d->nodes[parent];
    p->last = (int16_t)old_last;
    if (old_last >= 0) d->nodes[old_last].next = -1;
    else p->first = -1;
    d->nnodes--;
}

/* The box is styled in a scratch node first: only a box with content
 * takes a node (and can hit the node budget). */
static we_node_t g_gen_scratch;

static int gen_box(we_doc_t *d, int el, int kind) {
    we_node_t *p = &d->nodes[el], *s = &g_gen_scratch;
    int old_last = p->last, old_first = p->first, pending = p->pending_html_space, idx;
    w_zero(s, sizeof(*s));
    s->parent = (int16_t)el;
    s->first = s->last = s->next = -1;
    s->colspan = s->rowspan = 1;
    s->pseudo_kind = (uint8_t)kind;
    w_cpy(s->tag, 12, kind == WE_PSEUDO_BEFORE ? "::before" : "::after", kind == WE_PSEUDO_BEFORE ? 8 : 7);
    style_node(d, s, -1, el, kind);
    if (s->content_kind == WE_CONTENT_NONE) return -1;
    if (d->nnodes >= WE_MAX_NODES) { diag(d, "generated content budget", el); return -1; }
    idx = new_node(d, el);
    p->pending_html_space = (uint8_t)pending;
    d->nodes[idx] = *s;
    if (kind == WE_PSEUDO_BEFORE && old_first >= 0) {
        d->nodes[old_last].next = -1;
        p->last = (int16_t)old_last;
        d->nodes[idx].next = (int16_t)old_first;
        p->first = (int16_t)idx;
    }
    return idx;
}

static void counter_ops(we_doc_t *d, int idx, we_gen_t *g) {
    const we_node_t *n = &d->nodes[idx];
    int op;
    for (op = 0; op < 3; op++) {
        char names[WE_COUNTER_PAIRS][16];
        int values[WE_COUNTER_PAIRS], count, k;
        if (!n->counter_len[op]) continue;
        count = counter_scan(d->pool + n->counter_off[op], n->counter_len[op], names, values, op == 1 ? 1 : 0);
        for (k = 0; k < count; k++) {
            int at = counter_find(g, names[k]);
            /* a reset replaces the instance a sibling created, else nests */
            if (at < 0 || (op == 0 && g->c[at].scope != n->parent)) {
                if (g->n == WE_COUNTERS_MAX) { diag(d, "counter budget", idx); continue; }
                at = g->n++;
                w_cpy(g->c[at].name, 16, names[k], w_len(names[k]));
                g->c[at].scope = n->parent;
                g->c[at].value = 0;
            }
            if (op == 1) g->c[at].value = clamp_int(g->c[at].value + values[k], -999999, 999999);
            else g->c[at].value = values[k];
        }
    }
}

static void gen_fill(we_doc_t *d, int idx, we_gen_t *g) {
    char text[WE_CONTENT_TEXT_MAX];
    const we_node_t *n = &d->nodes[idx];
    int len = 0, t;
    if (n->content_kind == WE_CONTENT_LIST) (void)gen_content(d, d->pool + n->content_off, n->content_len, idx, g, text, &len);
    else gen_quote(d, idx, g, n->content_kind == WE_CONTENT_UA_OPEN ? "open-quote" : "close-quote", text, &len, WE_CONTENT_TEXT_MAX);
    if (len > WE_CONTENT_TEXT_MAX) { diag(d, "generated content truncated", idx); len = WE_CONTENT_TEXT_MAX; }
    if (len == 0) return;
    if (d->nnodes >= WE_MAX_NODES) { diag(d, "generated content budget", idx); return; }
    t = new_node(d, idx);
    d->nodes[t].is_text = 1;
    if (pool_put(d, text, len, &d->nodes[t].text_off) != 0) { diag(d, "generated content budget", idx); unlink_last(d, idx, -1); return; }
    d->nodes[t].text_len = (uint16_t)len;
    style_node(d, &d->nodes[t], t, t, 0);
}

static void gen_walk(we_doc_t *d, int idx, we_gen_t *g, int depth) {
    int c;
    if (d->nodes[idx].is_text || d->nodes[idx].display == WE_DISP_NONE || depth > WE_DEPTH_MAX) return;
    counter_ops(d, idx, g);
    if (d->nodes[idx].pseudo_kind) gen_fill(d, idx, g);
    for (c = d->nodes[idx].first; c >= 0; c = d->nodes[c].next) gen_walk(d, c, g, depth + 1);
    while (g->n > 0 && g->c[g->n - 1].scope == idx) g->n--;
}

static void generate_content(we_doc_t *d, int count) {
    we_gen_t g;
    int i, kind, any = 0;
    uint32_t r;
    for (r = 0; r < d->nrules; r++) any |= d->rules[r].pseudo_el;
    for (i = 1; i < count; i++) {
        if (!gen_allowed(d, i)) continue;
        for (kind = WE_PSEUDO_BEFORE; kind <= WE_PSEUDO_AFTER; kind++)
            if ((any & kind) || w_eq(d->nodes[i].tag, "q")) (void)gen_box(d, i, kind);
    }
    w_zero(&g, sizeof(g));
    gen_walk(d, 0, &g, 0);
}

/* Generated boxes are the first and last children past source_nodes. */
static void drop_generated(we_doc_t *d) {
    int i, src = d->source_nodes;
    for (i = 0; i < src; i++) {
        we_node_t *n = &d->nodes[i];
        int c, prev = -1;
        while (n->first >= src) n->first = d->nodes[n->first].next;
        for (c = n->first; c >= 0; c = d->nodes[c].next) { if (d->nodes[c].next >= src) d->nodes[c].next = -1; prev = c; }
        n->last = (int16_t)prev;
    }
    d->nnodes = (uint32_t)src;
    d->pool_used = d->source_pool;
}

int we_style(we_doc_t* d) {
    uint32_t i, count;
    if (!d || d->nnodes == 0) return -1;
    if (!d->css_parsed) { parse_css(d); d->css_parsed = 1; }
    if (d->source_nodes) drop_generated(d);
    count = d->source_nodes = (uint16_t)d->nnodes;
    d->source_pool = (uint16_t)d->pool_used;
    for (i = 0; i < count; i++) style_node(d, &d->nodes[i], (int)i, (int)i, 0);
    generate_content(d, (int)count);
    for (i = 0; i < d->nnodes; i++) {
        we_node_t* n = &d->nodes[i];
        int side;
        if (n->parent >= 0 && d->nodes[n->parent].display == WE_DISP_NONE) n->display = WE_DISP_NONE;
        /* none and hidden draw no border in either model. */
        if (n->border_style == WE_BS_NONE || n->border_style == WE_BS_HIDDEN) n->border = 0;
        /* A collapsed table has no border box of its own: its edges join
         * the outer cell edges (CSS 17.6.2). */
        if (n->display == WE_DISP_TABLE && n->border_collapse) {
            for (side = 0; side < 4; side++)
                if (n->edge_w[side] < 0) n->edge_w[side] = (int8_t)(n->border > 8 ? 8 : n->border);
            n->border = 0;
        }
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

/* An <img> reads its src, any other box its background-image url. */
static int image_local_path(we_doc_t *d, const we_node_t *n) {
    int i, start = 0, off = w_eq(n->tag, "img") ? n->src_off : n->bgi_off, len = w_eq(n->tag, "img") ? n->src_len : n->bgi_len;
    if (len >= (int)sizeof(image_work.path)) return -1;
    for (i = 0; i < len; i++) {
        char c = d->pool[off + i];
        if (c < 33 || c > 126 || c == ':' || c == '\\' || c == '%' || c == '?' || c == '#') return -1;
        image_work.path[i] = c;
    }
    image_work.path[i] = 0;
    if (image_work.path[0] != '/' || image_work.path[1] == '/') return -1;
    for (i = 1; i <= len; i++) {
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
    if (!(w_eq(n->tag, "img") ? n->src_len : n->bgi_len)) return -1;
    if (n->parent >= 0 && d->nodes[n->parent].wmode != WE_WM_HORIZONTAL)   /* lot 64: runs would be transposed */
        return image_fail(d, "image in vertical flow refused", idx);
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

static int image_dimensions(we_doc_t *d, int idx, int available, int assigned_w, int assigned_h, int *w, int *h);

static int modern_image(const we_node_t *n) {
    int slot;
    if (n->object_fit_set || n->aspect_num || n->width_kind || n->min_width_kind || n->max_width_kind) return 1;
    for (slot = WE_LEN_WIDTH; slot <= WE_LEN_MAX_H; slot++) if (n->length[slot].count) return 1;
    return 0;
}

static int image_intrinsic_width(we_doc_t *d, int idx) {
    const we_node_t *n = &d->nodes[idx];
    if (n->width >= 0) return n->width;
    if (!modern_image(n)) return n->text_len * WE_CHAR_W;
    if (!image_work.natural_w[idx] && image_open(d, idx)) return n->text_len * WE_CHAR_W;
    if (modern_image(n) && n->height >= 0)
        return n->height * (n->aspect_num ? n->aspect_num : image_work.natural_w[idx]) /
               (n->aspect_num ? n->aspect_den : image_work.natural_h[idx]);
    return image_work.natural_w[idx];
}

static int image_measure_height(we_doc_t *d, int idx, int available, int assigned_w) {
    int w, h;
    return image_dimensions(d, idx, available, assigned_w, -1, &w, &h) ? -1 : h;
}

static void image_fit_box(const we_node_t *n, int w, int h, int nw, int nh,
                          int *rw, int *rh, int *ox, int *oy) {
    *rw = w; *rh = h;
    if (n->object_fit == WE_FIT_NONE) { *rw = nw; *rh = nh; }
    else if (n->object_fit != WE_FIT_FILL) {
        int by_width = w * nh <= h * nw;
        if (n->object_fit == WE_FIT_COVER) by_width = !by_width;
        if (by_width) { *rw = w; *rh = w * nh / nw; }
        else { *rh = h; *rw = h * nw / nh; }
        if (*rw < 1) *rw = 1;
        if (*rh < 1) *rh = 1;
        if (n->object_fit == WE_FIT_SCALE_DOWN && *rw >= nw && *rh >= nh) { *rw = nw; *rh = nh; }
    }
    *ox = (w - *rw) * n->object_x / 100;
    *oy = (h - *rh) * n->object_y / 100;
}

static uint32_t image_sample(int x, int y, int rw, int rh) {
    unsigned char rgb[3];
    imgdec_pixel(&image_work.decoded, x * image_work.decoded.w / rw,
                 y * image_work.decoded.h / rh, rgb);
    return ((uint32_t)rgb[0] << 16) | ((uint32_t)rgb[1] << 8) | rgb[2];
}

/* Preflight the entire rectangle set. Failure never publishes half an
 * image and never consumes a display-list slot intended for the alt. */
static int image_paint(we_doc_t *d, int idx, int x, int y, int available, int assigned_w, int assigned_h) {
    we_node_t *n = &d->nodes[idx];
    int w, h, rw, rh, ox, oy, row, col, nr = 0, room = OS_FB_WEB_ITEMS - (int)d->nitems;
    int background = n->object_fit_set && n->has_bg && n->display == WE_DISP_INLINE;
    if (image_dimensions(d, idx, available, assigned_w, assigned_h, &w, &h) || image_open(d, idx)) return -1;
    if (background) room--;
    image_fit_box(n, w, h, image_work.decoded.w, image_work.decoded.h, &rw, &rh, &ox, &oy);
    for (row = 0; row < h; row++) {
        col = 0;
        while (col < w) {
            int first = col, r, found = -1;
            uint32_t color;
            if (col < ox || col >= ox + rw || row < oy || row >= oy + rh) { col++; continue; }
            color = image_sample(col - ox, row - oy, rw, rh);
            col++;
            while (col < w && col < ox + rw) {
                if (image_sample(col - ox, row - oy, rw, rh) != color) break;
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
    if (background) emit(d, idx, OS_FB_WEB_RECT, x, y, w, h, n->bg);
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
    int line_h, indent, indent_used, indent_shift, atom_h, pending_x;
    int baseline, origin_y;
    /* Lot 61: last measured baseline, painted first/last baselines (page y),
     * text seen on the open line and its baseline-aligned atomic boxes. */
    int last, lay_first, lay_last, line_text, line_start, natoms, flow;
    struct { int node, base, height; } atoms[8];
    int wrapped;   /* lot 62: the line being flushed ends at a soft wrap */
    int glue;      /* lot 63: the next piece joins the line without a break */
    int align_last, justify_none;
    int start;     /* lot 64: WE_ALIGN_RIGHT in rtl, where justify falls back */
} we_inline_t;
#define WE_NO_BASELINE (-32768)
#define WE_LINE_ATOMS 8

static void inline_image(we_doc_t *d, int idx, int x, int y, int w, int h);
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
        int band = f->line_h > 0 ? f->line_h : WE_LINE_H;
        for (i = 0; i < f->nfloat; i++) {
            int bottom = f->floats[i].y + f->floats[i].h;
            if (bottom <= f->y || f->floats[i].y >= f->y + band) continue;
            if (bottom < next) next = bottom;
            if (f->floats[i].side == WE_FLOAT_LEFT && f->floats[i].x + f->floats[i].w > left)
                left = f->floats[i].x + f->floats[i].w;
            if (f->floats[i].side == WE_FLOAT_RIGHT && f->floats[i].x < right)
                right = f->floats[i].x;
        }
        f->x0 = left;
        f->width = right > left ? right - left : 0;
        f->indent_used = f->indent_shift = 0;
        if (f->lines == 0 && f->indent > 0 && f->width > 0) {
            int take = f->indent;
            if (f->width - take < WE_CHAR_W && f->width > WE_CHAR_W) take = f->width - WE_CHAR_W;
            if (take > f->width) take = f->width;
            if (take < 0) take = 0;
            /* lot 64: in rtl the indent is at the right, the line start */
            if (f->start != WE_ALIGN_RIGHT) f->x0 += take, f->indent_shift = take;
            f->width -= take;
            f->indent_used = take;
        }
        if (f->width >= need || f->nwords || next == WE_LAYOUT_MAX ||
            (need > f->base_width && f->width >= f->base_width)) return;
        f->y = next;
    }
}

static int line_step(const we_inline_t *f) {
    int step = f->line_h > 0 ? f->line_h : WE_LINE_H;
    if (f->atom_h > step) step = f->atom_h;
    return step;
}

static int16_t layout_first[WE_MAX_NODES], layout_last[WE_MAX_NODES];
/* Lot 61: static position of absolutes without insets, page pixels. */
static int16_t static_x[WE_MAX_NODES], static_y[WE_MAX_NODES];
static uint8_t static_set[WE_MAX_NODES];
static void set_static(int idx, int x, int y) {
    static_x[idx] = (int16_t)x; static_y[idx] = (int16_t)y; static_set[idx] = 1;
}
static int baseline_get(we_doc_t *d, int idx, int outer, int depth, int last, int *real);
static int item_baseline(const we_doc_t *d, int c, int last);

static int in_subtree(const we_doc_t *d, int node, int root) {
    int guard;
    for (guard = 0; node >= 0 && guard <= WE_DEPTH_MAX + 1; guard++) {
        if (node == root) return 1;
        node = d->nodes[node].parent;
    }
    return 0;
}

/* Baseline-aligned atoms and the text of one line share one ascent. Text
 * moves by ascent - WE_TEXT_BASELINE, an atom by ascent - its baseline;
 * top-aligned atoms stay at the line top. */
static void shift_line(we_inline_t *f, int ascent, int has_text) {
    we_doc_t *d = f->d;
    int i, k, text_dy = has_text ? ascent - WE_TEXT_BASELINE : 0;
    for (i = f->line_start; i < (int)d->nitems; i++) {
        int owner = d->item_owner[i], dy = text_dy, up;
        for (up = owner; up >= 0 && up != f->flow; up = d->nodes[up].parent)
            if (d->nodes[up].float_side || d->nodes[up].absolute) { dy = 0; break; }
        for (k = 0; k < f->natoms; k++)
            if (in_subtree(d, owner, f->atoms[k].node)) { dy = f->atoms[k].base >= 0 ? ascent - f->atoms[k].base : 0; break; }
        d->items[i].y = (int16_t)limit_coord(d->items[i].y + dy);
    }
    for (k = 0; k < f->natoms; k++) {
        int dy = ascent - f->atoms[k].base, root = f->atoms[k].node;
        if (f->atoms[k].base < 0 || !dy) continue;
        for (i = root; i < (int)d->nnodes; i++)
            if (in_subtree(d, i, root)) {
                d->nodes[i].y = (int16_t)limit_coord(d->nodes[i].y + dy);
                if (static_set[i]) static_y[i] = (int16_t)limit_coord(static_y[i] + dy);
            }
        if (layout_first[root] != WE_NO_BASELINE) layout_first[root] = (int16_t)limit_coord(layout_first[root] + dy);
        if (layout_last[root] != WE_NO_BASELINE) layout_last[root] = (int16_t)limit_coord(layout_last[root] + dy);
    }
}

static void finish_line(we_inline_t *f, int has_text) {
    we_doc_t *d = f->d;
    int step = line_step(f), ascent = has_text ? WE_TEXT_BASELINE : 0, descent = 0, k, aligned = 0;
    int line_h = f->line_h > 0 ? f->line_h : WE_LINE_H;
    if (has_text) descent = line_h - WE_TEXT_BASELINE;
    for (k = 0; k < f->natoms; k++) {
        if (f->atoms[k].base < 0) continue;
        aligned = 1;
        if (f->atoms[k].base > ascent) ascent = f->atoms[k].base;
        if (f->atoms[k].height - f->atoms[k].base > descent) descent = f->atoms[k].height - f->atoms[k].base;
    }
    if (aligned && ascent + descent > step) step = ascent + descent;
    if (has_text || aligned) {
        int at = f->y + ascent;
        if (f->measuring) {
            if (f->baseline < 0) f->baseline = at - f->origin_y;
            f->last = at - f->origin_y;
        } else {
            if (f->lay_first == WE_NO_BASELINE) f->lay_first = at;
            f->lay_last = at;
        }
    }
    if (!f->measuring && aligned) shift_line(f, ascent, has_text);
    f->y = limit_coord(f->y + step);
    f->lines++;
    if (!f->measuring) d->perf.lines++;
    f->nwords = f->cur_w = f->pending_x = f->atom_h = 0;
    f->line_text = f->natoms = 0;
    f->line_start = (int)d->nitems;
}

static int line_aligned(const we_inline_t *f) {
    int k;
    for (k = 0; k < f->natoms; k++) if (f->atoms[k].base >= 0) return 1;
    return 0;
}

/* Lot 62: text decorations of a text run at (x, y) of width w, y being
 * the glyph top. Each line kind comes from the nearest ancestor that
 * declares it (its colour, style and thickness), walking up from the text
 * and stopping after a float, an absolute box or an inline-block, whose
 * own decorations still apply (CSS Text Decoration 3, 2.1). Underline
 * sits under the glyph cell, overline above it, line-through on row 4.
 * Wavy paints solid with one "wavy decoration painted solid". */
static int wavy_decoration;
static void edge_paint(we_doc_t *d, int idx, int style, uint32_t color, int vertical, int off, int from, int w, int len);

static void paint_decorations(we_doc_t *d, int text, int x, int y, int w) {
    int done = 0, kind, p = d->nodes[text].parent, guard;
    for (guard = 0; p >= 0 && guard <= WE_DEPTH_MAX + 1 && done != 7; guard++, p = d->nodes[p].parent) {
        const we_node_t *n = &d->nodes[p];
        for (kind = 1; kind <= 4; kind <<= 1) {
            int t = n->deco_thick ? n->deco_thick : 1, at, style = n->deco_style;
            uint32_t color = n->deco_color_set ? n->deco_color : n->color;
            if (!(n->deco_line & kind) || (done & kind)) continue;
            done |= kind;
            at = kind == WE_DECO_UNDER ? y + 8 : kind == WE_DECO_OVER ? y - t : y + 4 - t / 2;
            if (style == WE_BS_WAVY) { wavy_decoration = 1; style = WE_BS_SOLID; }
            if (style == 4) {   /* double: two lines, the gap equal to the thickness */
                edge_paint(d, text, WE_BS_SOLID, color, 0, kind == WE_DECO_OVER ? at - 2 * t : at, x, t, w);
                edge_paint(d, text, WE_BS_SOLID, color, 0, kind == WE_DECO_OVER ? at : at + 2 * t, x, t, w);
            } else edge_paint(d, text, style, color, 0, at, x, t, w);
        }
        if (n->float_side || n->absolute || n->display == WE_DISP_INLINE_BLOCK) break;
    }
}

/* Lot 62: text-transform on a run copy (ASCII; glyph widths are fixed).
 * capitalize upper-cases the first letter of each word; word_start says
 * whether the run itself starts a word. */
static void text_transform(char *buf, int len, int mode, int word_start) {
    int k, start = word_start;
    for (k = 0; k < len; k++) {
        char c = buf[k];
        if (mode == WE_TT_UPPER || (mode == WE_TT_CAPITALIZE && start)) { if (c >= 'a' && c <= 'z') c = (char)(c - 32); }
        else if (mode == WE_TT_LOWER) { if (c >= 'A' && c <= 'Z') c = (char)(c + 32); }
        buf[k] = c;
        start = c == ' ';
    }
}

/* Lot 62: text shadows paint under their run, the first one on top; the
 * colour defaults to the text colour. Blur paints sharp (one diagnostic). */
static int text_shadow_blur;
static void text_shadows(we_doc_t *d, int node, int x, int y, const char *buf, int len) {
    const we_node_t *t = &d->nodes[node];
    int k;
    for (k = t->tshadow_count - 1; k >= 0; k--) {
        const we_shadow_t *sh = &t->tshadow[k];
        os_fb_web_item_t *it = emit(d, node, OS_FB_WEB_TEXT, limit_coord(x + sh->x), limit_coord(y + sh->y), len * WE_CHAR_W, 8,
                                    sh->has_color ? sh->color : t->color);
        if (sh->blur) text_shadow_blur = 1;
        if (!it) return;
        it->bold = t->bold;
        w_cpy(it->text, OS_FB_WEB_TEXT_LEN, buf, len);
    }
}

/* Lot 62 round 2: letter-spacing widens every glyph cell of a text node,
 * word-spacing every word space (which takes the letter spacing too).
 * The kernel font has a fixed 8 px advance, so a letter-spaced run paints
 * one TEXT item per glyph; word-spaced or justified spaces end the merged
 * run. Justification spreads the free width of a soft-wrapped line over
 * its word spaces, the later spaces taking the rounding; the last line, a
 * forced break and lines with atomic boxes keep start alignment. Letter
 * spacing applies after every glyph, the last one included. */
static int glyph_adv(const we_doc_t *d, int node) {
    return WE_CHAR_W + d->nodes[node].letter_spacing;
}

static int space_adv(const we_doc_t *d, int node) {
    int v = WE_CHAR_W + d->nodes[node].letter_spacing + d->nodes[node].word_spacing;
    return v > 0 ? v : 0;
}

static int gap_extra(int extra, int gaps, int g) {
    return gaps ? extra * (g + 1) / gaps - extra * g / gaps : 0;
}

static void paint_run(we_doc_t *d, int node, int xs, int y, const char *buf, int len) {
    int adv = glyph_adv(d, node), k;
    if (adv == WE_CHAR_W) {
        os_fb_web_item_t *it;
        text_shadows(d, node, xs, y, buf, len);
        it = emit(d, node, OS_FB_WEB_TEXT, xs, y, len * WE_CHAR_W, 8, d->nodes[node].color);
        if (it) { it->bold = d->nodes[node].bold; w_cpy(it->text, OS_FB_WEB_TEXT_LEN, buf, len); }
        return;
    }
    for (k = 0; k < len; k++) {
        os_fb_web_item_t *it;
        if (buf[k] == ' ') continue;
        text_shadows(d, node, xs + k * adv, y, buf + k, 1);
        it = emit(d, node, OS_FB_WEB_TEXT, xs + k * adv, y, WE_CHAR_W, 8, d->nodes[node].color);
        if (!it) return;
        it->bold = d->nodes[node].bold;
        it->text[0] = buf[k];
        it->text[1] = 0;
    }
}

/* advance=0 paints a segment and keeps the line open for an atomic box. */
static void flush_line_mode(we_inline_t *f, int force, int advance) {
    int i, x, total = 0, box_h, wrapped = f->wrapped, extra = 0, gaps = 0, gap = 0, align = f->align;
    we_doc_t* d = f->d;
    f->wrapped = 0;
    box_h = f->line_h > 0 ? f->line_h : WE_LINE_H;
    if (f->nwords == 0 && !force) {
        if (advance && (f->atom_h > 0 || line_aligned(f))) finish_line(f, f->line_text);
        return;
    }
    if (f->measuring) {
        if (advance) finish_line(f, 1);
        else { f->nwords = 0; f->line_text = 1; }
        return;
    }
    for (i = 0; i < f->nwords; i++) {
        total += f->w[i].len * glyph_adv(d, f->w[i].node);
        if (i && f->w[i].space_before) { total += space_adv(d, f->w[i].node); gaps++; }
    }
    x = f->x0 + f->pending_x;
    /* the last line and a line before a forced break: text-align-last,
     * auto meaning text-align with justify falling back to start */
    if (!wrapped) align = f->align_last ? f->align_last - 1 : align == WE_ALIGN_JUSTIFY ? f->start : align;
    if (f->justify_none && align == WE_ALIGN_JUSTIFY) align = f->start;
    if (advance && f->pending_x == 0 && f->atom_h == 0) {
        if (align == WE_ALIGN_CENTER && total < f->width) x += (f->width - total) / 2;
        else if (align == WE_ALIGN_RIGHT && total < f->width) x += f->width - total;
        else if (align == WE_ALIGN_JUSTIFY && gaps && total < f->width) extra = f->width - total;
    }
    i = 0;
    while (i < f->nwords) {
        /* merge words of the same node into one TEXT item */
        int node = f->w[i].node, j = i, len = 0, xs, run_w, before;
        char buf[OS_FB_WEB_TEXT_LEN];
        const we_node_t* n;
        if (i && f->w[i].space_before) x += space_adv(d, node) + gap_extra(extra, gaps, gap++);
        xs = x;
        while (j < f->nwords && f->w[j].node == node) {
            int k, sp = (j > i && f->w[j].space_before) ? 1 : 0;
            if (sp && (space_adv(d, node) != WE_CHAR_W || gap_extra(extra, gaps, gap))) break;
            gap += sp;
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
        text_transform(buf, len, d->nodes[node].text_transform, i == 0 || f->w[i].space_before);
        n = &d->nodes[d->nodes[node].parent >= 0 ? d->nodes[node].parent : node];
        run_w = len * glyph_adv(d, node);
        if (n->has_bg && n->display == WE_DISP_INLINE) emit(d, node, OS_FB_WEB_RECT, xs, f->y, run_w, box_h, n->bg);
        if ((n->bgi_len || n->bgi_layers) && n->display == WE_DISP_INLINE && run_w > 0)
            inline_image(d, d->nodes[node].parent >= 0 ? d->nodes[node].parent : node, xs, f->y, run_w, box_h);
        before = (int)d->nitems;
        paint_run(d, node, xs, f->y + 1, buf, len);
        if ((int)d->nitems > before) paint_decorations(d, node, xs, f->y + 1, run_w);
        {   /* UX4: text nodes get the bounding box of their runs, so a
             * click on a link's words finds the <a> (we_link_at) */
            we_node_t* tn = &d->nodes[node];
            int x1 = xs + run_w, y1 = f->y + WE_LINE_H;
            if (tn->w <= 0) { tn->x = (int16_t)limit_coord(xs); tn->y = (int16_t)limit_coord(f->y); tn->w = (int16_t)(x1 - xs); tn->h = WE_LINE_H; }
            else {
                int ax = tn->x < xs ? tn->x : xs, ay = tn->y < f->y ? tn->y : f->y;
                int bx = tn->x + tn->w > x1 ? tn->x + tn->w : x1, by = tn->y + tn->h > y1 ? tn->y + tn->h : y1;
                tn->x = (int16_t)limit_coord(ax); tn->y = (int16_t)limit_coord(ay); tn->w = (int16_t)(bx - ax); tn->h = (int16_t)(by - ay);
            }
        }
        x = xs + run_w;
        i = j;
    }
    if (!advance) {
        f->pending_x = x - f->x0;
        if (f->pending_x < 0) f->pending_x = 0;
        f->cur_w = f->pending_x;
        f->nwords = 0;
        f->line_text = 1;
        return;
    }
    finish_line(f, 1);
}

static void flush_line(we_inline_t *f, int force) { flush_line_mode(f, force, 1); }
static void flush_segment(we_inline_t *f) { flush_line_mode(f, 0, 0); }

static void add_word_piece(we_inline_t* f, const char* s, int len, int node, int space) {
    int ww, word = len * glyph_adv(f->d, node);
    flow_region(f, word);
    ww = word + ((f->nwords && space) ? space_adv(f->d, node) : 0);
    if (!f->d->nodes[node].nowrap && !f->glue && (f->nwords || f->pending_x) && f->cur_w + ww > f->width) { f->wrapped = 1; flush_line(f, 0); flow_region(f, word); ww = word; }
    if (f->nwords >= WE_LINE_WORDS) {
        if (f->d->nodes[node].nowrap) diag(f->d, "inline word capacity", node);
        f->wrapped = 1;
        flush_line(f, 0);
        ww = word;
    }
    f->w[f->nwords].s = s; f->w[f->nwords].len = len; f->w[f->nwords].node = node;
    f->w[f->nwords].space_before = space;
    f->nwords++;
    f->cur_w += ww;
}

static void soft_wrap(we_inline_t *f) {
    f->wrapped = 1;
    flush_line(f, 0);
}

/* ---------- Lot 63: tabs and break-spaces ----------
 * A preserved tab advances to the next multiple of tab-size from the line
 * start, painted as space advances (at most 32); collapsed, it is a space.
 * break-spaces: preserved spaces wrap like letters, filling the line and
 * continuing on the next one instead of hanging. */
static const char tab_blank[33] = "                                ";

static int preserves_spaces(const we_node_t *n) {
    return n->ws_mode == WE_WS_PRE || n->ws_mode == WE_WS_PRE_WRAP || n->ws_mode == WE_WS_BREAK_SPACES;
}

static int tab_spaces(const we_node_t *n, int pen, int adv) {
    int stop = n->tab_px, count;
    if (stop <= 0 || adv <= 0) return 0;
    if (pen < 0) pen = 0;
    count = ((pen / stop + 1) * stop - pen + adv - 1) / adv;
    return count > 32 ? 32 : count;
}

static void add_word(we_inline_t *f, const char *s, int len, int node, int space);
static void space_run(we_inline_t *f, const char *s, int len, int node) {
    int adv = glyph_adv(f->d, node);
    if (f->d->nodes[node].ws_mode != WE_WS_BREAK_SPACES || f->d->nodes[node].nowrap) { add_word(f, s, len, node, 0); return; }
    while (len > 0) {
        int take;
        flow_region(f, adv);
        take = (f->width - f->cur_w) / adv;
        if (take < 1) take = 1;                    /* add_word_piece wraps a full line */
        if (take > len) take = len;
        add_word_piece(f, s, take, node, 0);
        s += take; len -= take;
    }
}

static void add_word(we_inline_t *f, const char *s, int len, int node, int space) {
    const we_node_t *n = &f->d->nodes[node];
    int adv = glyph_adv(f->d, node), gap = space_adv(f->d, node);
    if (n->break_all && !n->nowrap) {
        while (len > 0) {
            int room, take;
            flow_region(f, adv);
            room = f->width - f->cur_w - (f->nwords && space ? gap : 0);
            if (room < adv && (f->nwords || f->pending_x)) {
                soft_wrap(f); space = 0;
                continue;
            }
            take = room / adv;
            if (take < 1) take = 1;
            if (take > len) take = len;
            add_word_piece(f, s, take, node, space);
            s += take; len -= take; space = 0;
            if (len) soft_wrap(f);
        }
        return;
    }
    flow_region(f, adv);
    if (!n->break_word || n->nowrap || len * adv <= f->width) {
        add_word_piece(f, s, len, node, space);
        return;
    }
    if (f->nwords) { soft_wrap(f); space = 0; }
    while (len > 0) {
        int room, take;
        flow_region(f, adv);
        room = f->width - f->cur_w - (f->nwords && space ? gap : 0);
        if (room < adv) {
            soft_wrap(f);
            flow_region(f, adv);
            room = f->width;
            space = 0;
        }
        take = room / adv;
        if (take < 1) take = 1;
        if (take > len) take = len;
        add_word_piece(f, s, take, node, space);
        s += take;
        len -= take;
        space = 0;
        if (len) soft_wrap(f);
    }
}

static void place_float(we_inline_t *f, int idx, int depth);
static void place_inline_block(we_inline_t *f, int idx, int depth);

/* ---------- Lot 63: soft hyphens ----------
 * A word holding soft hyphens is laid out piece by piece. When they break
 * (hyphens manual, wrapping, not break-all), a line may end after a piece
 * and then shows a hyphen; a piece followed by another must fit with that
 * hyphen, else the line breaks before it (at the space for the first
 * piece). Otherwise the soft hyphens vanish: under hyphens:none or nowrap
 * the pieces are glued into one word; break-all and overflow-wrap words go
 * piece by piece through add_word (they may break at a piece end, with no
 * hyphen). */
static const char we_hyphen[2] = "-";

static int shy_breaks(const we_node_t *n) {
    return n->hyphens != WE_HYPHENS_NONE && !n->nowrap && !n->break_all;
}

static void add_text_word(we_inline_t *f, const char *s, int len, int node, int space) {
    const we_node_t *n = &f->d->nodes[node];
    int adv = glyph_adv(f->d, node), gap = space_adv(f->d, node), a = 0, first = 1, k, visible = 0;
    int hyph = shy_breaks(n), glue = !hyph && !n->break_all && !n->break_word;
    for (k = 0; k < len; k++) visible += s[k] != WE_SHY;
    if (visible == len) { add_word(f, s, len, node, space); return; }
    if (glue) {
        flow_region(f, adv);
        if (!n->nowrap && (f->nwords || f->pending_x) && f->cur_w + visible * adv + (f->nwords && space ? gap : 0) > f->width) { soft_wrap(f); space = 0; }
    }
    while (a < len) {
        int e = a, next;
        while (e < len && s[e] != WE_SHY) e++;
        for (next = e; next < len && s[next] == WE_SHY; next++) {}
        if (e > a) {
            if (hyph) {
                int need = (e - a + (next < len)) * adv + (first && f->nwords && space ? gap : 0);
                flow_region(f, adv);
                if ((f->nwords || f->pending_x) && f->cur_w + need > f->width) {
                    if (!first) { f->glue = 1; add_word_piece(f, we_hyphen, 1, node, 0); f->glue = 0; }
                    soft_wrap(f);
                }
            }
            if (glue && !first) { f->glue = 1; add_word_piece(f, s + a, e - a, node, 0); f->glue = 0; }
            else add_word(f, s + a, e - a, node, first ? space : 0);
            first = 0;
        }
        a = next;
    }
}

static void inline_node(we_inline_t* f, int idx, int depth) {
    we_doc_t* d = f->d;
    we_node_t* n = &d->nodes[idx];
    int c;
    if (n->html_space) f->pending_space = 1;
    if (n->display == WE_DISP_NONE || depth > WE_DEPTH_MAX) return;
    /* Lot 61: an absolute inside inline content leaves the line; its static
     * position is the current pen. */
    if (n->absolute && !n->is_text) {
        if (!f->measuring) set_static(idx, f->x0 + f->cur_w, f->y);
        return;
    }
    if (n->float_side && !n->is_text) { place_float(f, idx, depth); return; }
    if (n->is_text) {
        const char* s = d->pool + n->text_off;
        int i = 0, len = n->text_len;
        int keep_sp = preserves_spaces(n);
        int keep_nl = keep_sp || n->ws_mode == WE_WS_PRE_LINE;
        if (n->lead_space) f->pending_space = 1;
        while (i < len) {
            int a;
            if (s[i] == '\n') {
                if (keep_nl) { flush_line(f, 1); f->pending_space = 0; }
                else f->pending_space = 1;
                i++;
                continue;
            }
            if (s[i] == '\t' && keep_sp) {
                int adv = glyph_adv(d, idx), count = tab_spaces(n, f->cur_w, adv);
                /* a tab that does not fit starts the next line, from its start */
                if (!n->nowrap && f->nwords && f->cur_w + count * adv > f->width) { soft_wrap(f); count = tab_spaces(n, f->cur_w, adv); }
                if (count) add_word(f, tab_blank, count, idx, 0);
                i++;
                continue;
            }
            if (s[i] == ' ' && keep_sp) {
                a = i;
                while (i < len && s[i] == ' ') i++;
                space_run(f, s + a, i - a, idx);
                continue;
            }
            while (i < len && (s[i] == ' ' || s[i] == '\t' || s[i] == '\n')) {
                if (s[i] == '\n' && keep_nl) break;
                i++;
                f->pending_space = 1;
            }
            if (i < len && s[i] == '\n') continue;
            a = i;
            while (i < len && s[i] != ' ' && s[i] != '\t' && s[i] != '\n') i++;
            if (i > a) { add_text_word(f, s + a, i - a, idx, f->pending_space); f->pending_space = 0; }
        }
        if (n->trail_space) f->pending_space = 1;
        return;
    }
    if (n->display == WE_DISP_INLINE_BLOCK) { place_inline_block(f, idx, depth); return; }
    if (w_eq(n->tag, "br")) { flush_line(f, 1); f->pending_space = 0; return; }
    if (w_eq(n->tag, "img")) {
        int h;
        flush_line(f, 0);
        flow_region(f, WE_CHAR_W);
        if (f->measuring) {
            h = image_measure_height(d, idx, f->width, -1);
            if (h >= 0 && n->height >= 0) h = n->height;
        } else h = image_paint(d, idx, f->x0, f->y, f->width, -1, -1);
        if (h >= 0) { f->y = limit_coord(f->y + h); return; }
        if (n->text_len) add_word(f, d->pool + n->text_off, n->text_len, idx, f->pending_space);
        f->pending_space = 0;
        return;
    }
    for (c = n->first; c >= 0; c = d->nodes[c].next) inline_node(f, c, depth + 1);
}

static int layout_block(we_doc_t* d, int idx, int x, int y, int cw, int depth);
static void edge_paint(we_doc_t *d, int idx, int style, uint32_t color, int vertical, int off, int from, int w, int len);
static void frame_3d(we_doc_t *d, int idx, int style, uint32_t color, int x, int y, int w, int h, int t);
static int layout_flex(we_doc_t* d, int idx, int cx, int cy, int cwidth, int depth);
static int is_box(uint8_t display) {
    return display == WE_DISP_BLOCK || display == WE_DISP_FLEX || display == WE_DISP_GRID ||
           display == WE_DISP_TABLE || display == WE_DISP_ROW || display == WE_DISP_CELL;
}

/* Border width on one side of the box. A cell of a collapsed table uses
 * its resolved content inset (collapse_insets) instead of its border. */
static void edge_of(const we_node_t *n, int side, int *w, int *style, uint32_t *color);
static int box_edge(const we_node_t *n, int side) {
    if (n->collapse_cell) return n->cin[side];
    if (n->own_edges) {
        /* lot 64: the side's own width (hidden is none outside tables) */
        int w, style;
        uint32_t color;
        edge_of(n, side, &w, &style, &color);
        return style == WE_BS_HIDDEN ? 0 : w;
    }
    return n->border;
}

static int node_border_extra(const we_node_t *n) {
    return box_edge(n, 1) + box_edge(n, 3) + n->padding[1] + n->padding[3];
}

static int intrinsic_value(const we_node_t *n, int slot, int containing) {
    int kind = intrinsic_kind(n, slot), value;
    if (kind == WE_SIZE_MIN_CONTENT) return n->min_content;
    if (kind == WE_SIZE_MAX_CONTENT) return n->max_content;
    value = containing - node_border_extra(n) - n->margin[1] - n->margin[3];
    if (kind == WE_SIZE_FIT_LIMIT) {
        value = math_nonnegative(n, slot, containing, n->max_content);
        if (n->border_box) { value -= node_border_extra(n); if (value < 0) value = 0; }
    }
    if (value > n->max_content) value = n->max_content;
    if (value < n->min_content) value = n->min_content;
    return value;
}

static int width_limit(const we_node_t *n, int containing, int high) {
    int slot = high ? WE_LEN_MAX_W : WE_LEN_MIN_W;
    int kind = intrinsic_kind(n, slot);
    int pct = high ? n->max_pct : n->min_pct;
    int value = pct > 0 ? containing * pct / 100 : high ? n->max_width : n->min_width;
    if (kind) return intrinsic_value(n, slot, containing);
    value = math_nonnegative(n, slot, containing, n->length[slot].count ? (high ? -1 : 0) : value);
    if (value >= 0 && n->border_box) { value -= node_border_extra(n); if (value < 0) value = 0; }
    return value;
}

static int constrained_width(const we_node_t *n, int width, int containing) {
    int low = width_limit(n, containing, 0), high = width_limit(n, containing, 1);
    if (high >= 0 && width > high) width = high;
    if (low >= 0 && width < low) width = low;
    return width < 0 ? 0 : width > WE_LAYOUT_MAX ? WE_LAYOUT_MAX : width;
}

static int16_t forced_width[WE_MAX_NODES], forced_height[WE_MAX_NODES];
static int16_t resolved_height[WE_MAX_NODES], containing_height[WE_MAX_NODES];
static int16_t flow_baseline[WE_MAX_NODES], flow_last[WE_MAX_NODES];
static int baseline_probe;
static int measure_budget;
static uint8_t measure_content[WE_MAX_NODES];
typedef struct {
    int width, fixed, parent, result, baseline, last;
    uint8_t content, valid, probe;
} we_measure_cache_t;
static we_measure_cache_t measure_cache[WE_MAX_NODES];

static int own_axis_gap(const we_node_t *n, int vertical) {
    int value = vertical ? n->row_gap : n->col_gap;
    value = math_nonnegative(n, vertical ? WE_LEN_ROW_GAP : WE_LEN_COL_GAP, 0, value);
    return value >= 0 ? value : n->gap > 0 ? n->gap : 0;
}

static int axis_gap(const we_node_t *n, int vertical) {
    if (n->sub_gap[vertical] > 0) return n->sub_gap[vertical] - 1;
    return own_axis_gap(n, vertical);
}

static int vertical_extra(const we_node_t *n) {
    return box_edge(n, 0) + box_edge(n, 2) + n->padding[0] + n->padding[2];
}

static int content_value(int value, int extra, int border_box) {
    if (border_box) value -= extra;
    return value < 0 ? 0 : value > WE_LAYOUT_MAX ? WE_LAYOUT_MAX : value;
}

static int ratio_height(const we_node_t *n, int width) {
    int height;
    if (!n->aspect_num || n->aspect_den < 1) return 0;
    if (n->border_box) width += node_border_extra(n);
    if (width < 0) width = 0;
    if (width > WE_LAYOUT_MAX) width = WE_LAYOUT_MAX;
    height = width * n->aspect_den / n->aspect_num;
    if (n->border_box) height -= vertical_extra(n);
    return height < 0 ? 0 : height > WE_LAYOUT_MAX ? WE_LAYOUT_MAX : height;
}

/* Lot 64: rtl is the containing block's direction; a box without auto
 * margins then takes the free space on its left (the end side). */
static int horizontal_auto(const we_node_t *n, int containing, int outer, int rtl) {
    int free = containing - outer - n->margin[1] - n->margin[3];
    if (free <= 0) return 0;
    if (!(n->margin_auto & (WE_AUTO_LEFT | WE_AUTO_RIGHT))) return rtl ? free : 0;
    if (!(n->margin_auto & WE_AUTO_LEFT)) return 0;
    return n->margin_auto & WE_AUTO_RIGHT ? free / 2 : free;
}

static int specified_width(const we_node_t *n, int containing, int fallback) {
    int value;
    if (n->width_kind) value = intrinsic_value(n, WE_LEN_WIDTH, containing);
    else if (n->length[WE_LEN_WIDTH].count) value = content_value(math_nonnegative(n, WE_LEN_WIDTH, containing, fallback), node_border_extra(n), n->border_box);
    else {
        value = n->width_pct > 0 ? containing * n->width_pct / 100 :
                n->width >= 0 ? n->width : fallback;
        if (n->width >= 0 || n->width_pct > 0) value = content_value(value, node_border_extra(n), n->border_box);
    }
    return constrained_width(n, value, containing);
}

static void intrinsic_contribution(const we_node_t *n, int *low, int *high) {
    *low = n->min_content; *high = n->max_content;
    if ((n->width_kind == WE_SIZE_MIN_CONTENT || n->width_kind == WE_SIZE_MAX_CONTENT) ||
        (n->width_kind == WE_SIZE_FIT_LIMIT && !n->length[WE_LEN_WIDTH].percent) ||
        (!n->width_kind && (n->width >= 0 ||
         (n->length[WE_LEN_WIDTH].count && !n->length[WE_LEN_WIDTH].percent))))
        *low = *high = specified_width(n, 0, *high);
    *low = constrained_width(n, *low, 0);
    *high = constrained_width(n, *high, 0);
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
    if (depth > WE_DEPTH_MAX || n->display == WE_DISP_NONE || n->absolute) return;
    if (n->is_text) {
        if (n->lead_space) { intrinsic_word(flow); flow->pending = 1; }
        for (i = 0; i < n->text_len; i++) {
            char ch = d->pool[n->text_off + i];
            int keep_sp = preserves_spaces(n), count, k;
            int keep_nl = keep_sp || n->ws_mode == WE_WS_PRE_LINE;
            if (ch == '\t' && !keep_sp) ch = ' ';
            count = ch == '\t' ? tab_spaces(n, flow->line, WE_CHAR_W) : 1;
            if (ch == '\n') {
                if (keep_nl) intrinsic_line(flow);
                else { intrinsic_word(flow); flow->pending = 1; }
            } else if ((ch == ' ' || ch == '\t') && (n->ws_mode == WE_WS_PRE_WRAP || n->ws_mode == WE_WS_BREAK_SPACES)) {
                intrinsic_word(flow);
                flow->line = limit_coord(flow->line + count * WE_CHAR_W);
                if (count && WE_CHAR_W > flow->minimum) flow->minimum = WE_CHAR_W;
            } else if ((ch == ' ' || ch == '\t') && keep_sp) {
                for (k = 0; k < count; k++) {
                    flow->line = limit_coord(flow->line + WE_CHAR_W);
                    flow->word = limit_coord(flow->word + WE_CHAR_W);
                    if (!n->nowrap && (n->break_all || n->break_word == WE_WRAP_ANYWHERE)) intrinsic_word(flow);
                }
            } else if (ch == ' ') {
                intrinsic_word(flow);
                flow->pending = 1;
            } else if (ch == WE_SHY) {
                /* a breaking soft hyphen before a glyph ends a word with a hyphen
                 * (a leading one adds a hyphen no wider than that glyph) */
                char after = i + 1 < n->text_len ? d->pool[n->text_off + i + 1] : ' ';
                if (shy_breaks(n) && !w_space(after) && after != WE_SHY) {
                    if (flow->word + glyph_adv(d, idx) > flow->minimum) flow->minimum = flow->word + glyph_adv(d, idx);
                    flow->word = 0;
                }
            } else {
                if (flow->pending && flow->line) flow->line += space_adv(d, idx);
                flow->pending = 0;
                flow->line = limit_coord(flow->line + glyph_adv(d, idx));
                flow->word = limit_coord(flow->word + glyph_adv(d, idx));
                if (!n->nowrap && (n->break_all || n->break_word == WE_WRAP_ANYWHERE)) intrinsic_word(flow);
            }
        }
        if (n->trail_space) { intrinsic_word(flow); flow->pending = 1; }
    } else if (w_eq(n->tag, "br")) intrinsic_line(flow);
    else if (w_eq(n->tag, "img")) {
        int width = image_intrinsic_width(d, idx);
        intrinsic_word(flow);
        if (flow->pending && flow->line) flow->line += WE_CHAR_W;
        flow->pending = 0;
        flow->line = limit_coord(flow->line + width);
        if (width > flow->minimum) flow->minimum = width;
    } else if (n->display == WE_DISP_INLINE_BLOCK) {
        int extra = node_border_extra(n) + n->margin[1] + n->margin[3];
        int low, high;
        intrinsic_contribution(n, &low, &high);
        low += extra; high += extra;
        intrinsic_word(flow);
        if (flow->pending && flow->line) flow->line += WE_CHAR_W;
        flow->pending = 0;
        flow->line = limit_coord(flow->line + high);
        if (low > flow->minimum) flow->minimum = low;
    } else if (is_box(n->display)) {
        int extra = node_border_extra(n) + n->margin[1] + n->margin[3];
        int low, high;
        intrinsic_line(flow);
        intrinsic_contribution(n, &low, &high);
        low += extra; high += extra;
        if (low > flow->minimum) flow->minimum = low;
        if (high > flow->maximum) flow->maximum = high;
    } else {
        for (c = n->first; c >= 0; c = d->nodes[c].next) intrinsic_inline(d, c, flow, depth + 1);
    }
}

static void intrinsic_track_sizes(we_doc_t *d, int idx, int *minimum, int *maximum);

static int16_t vertical_inline[WE_MAX_NODES];   /* lot 64: logical max-content of an orthogonal root */
static int vertical_root(const we_doc_t *d, int idx);

static void intrinsic_sizes(we_doc_t *d) {
    int i;
    for (i = (int)d->nnodes - 1; i >= 0; i--) {
        we_node_t *n = &d->nodes[i];
        we_intrinsic_flow_t flow;
        int c;
        w_zero(&flow, sizeof(flow));
        if (n->display == WE_DISP_NONE) { n->min_content = n->max_content = 0; continue; }
        if (n->is_text) intrinsic_inline(d, i, &flow, 0);
        else if (w_eq(n->tag, "img")) flow.minimum = flow.maximum = image_intrinsic_width(d, i);
        else if (n->display == WE_DISP_GRID || n->display == WE_DISP_TABLE)
            intrinsic_track_sizes(d, i, &flow.minimum, &flow.maximum);
        else if (n->display == WE_DISP_FLEX && !n->flex_dir) {
            int count = 0, gap = axis_gap(n, 0), low = 0, high = 0;
            for (c = n->first; c >= 0; c = d->nodes[c].next) {
                const we_node_t *child = &d->nodes[c];
                int extra, a, b;
                if (child->display == WE_DISP_NONE || child->is_text || child->absolute) continue;
                extra = node_border_extra(child) + child->margin[1] + child->margin[3];
                intrinsic_contribution(child, &a, &b);
                a += extra; b += extra;
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
        if (n->text_indent > 0 && flow.maximum > 0) {
            flow.maximum = limit_coord(flow.maximum + n->text_indent);
            if (flow.minimum > 0) flow.minimum = limit_coord(flow.minimum + n->text_indent);
        }
        n->min_content = (int16_t)limit_coord(n->nowrap ? flow.maximum : flow.minimum);
        n->max_content = (int16_t)limit_coord(flow.maximum);
        if (vertical_root(d, i)) {
            /* lot 64: an orthogonal root contributes its block extent:
             * the lines its max-content needs at its inline size */
            int size = n->height > 0 ? n->height : flow.maximum, lines;
            vertical_inline[i] = (int16_t)limit_coord(flow.maximum);
            lines = size > 0 ? (flow.maximum + size - 1) / size : 1;
            if (lines < 1) lines = 1;
            n->min_content = n->max_content = (int16_t)limit_coord(lines * (n->line_height ? n->line_height : WE_LINE_H));
        }
    }
}

static int vertical_now = -1, vertical_block;   /* lot 64: root being laid out, its logical block size */
static int definite_height(const we_doc_t *d, int idx, int depth) {
    const we_node_t *n;
    int value, parent, low, high;
    if (idx < 0 || depth > WE_DEPTH_MAX) return -1;
    if (idx == vertical_now) return vertical_block;
    n = &d->nodes[idx];
    if (forced_height[idx] >= 0)
        return content_value(forced_height[idx] - n->margin[0] - n->margin[2], vertical_extra(n), 1);
    if (resolved_height[idx] >= 0) return resolved_height[idx];
    parent = containing_height[idx] >= 0 ? containing_height[idx] :
             n->height_percent || n->min_height_percent || n->max_height_percent ?
             definite_height(d, n->parent, depth + 1) : -1;
    if (n->length[WE_LEN_HEIGHT].count) {
        value = math_nonnegative(n, WE_LEN_HEIGHT, parent, -1);
        if (value < 0) return -1;
        value = content_value(value, vertical_extra(n), n->border_box);
    } else if (n->height_percent) {
        if (parent < 0) return -1;
        value = content_value(parent * n->height_pct / 100, vertical_extra(n), n->border_box);
    } else if (n->height >= 0) value = content_value(n->height, vertical_extra(n), n->border_box);
    else return -1;
    low = n->min_height_percent ? (parent >= 0 ? parent * n->min_height_pct / 100 : -1) : n->min_height;
    high = n->max_height_percent ? (parent >= 0 ? parent * n->max_height_pct / 100 : -1) : n->max_height;
    low = math_nonnegative(n, WE_LEN_MIN_H, parent, n->length[WE_LEN_MIN_H].count ? 0 : low);
    high = math_nonnegative(n, WE_LEN_MAX_H, parent, n->length[WE_LEN_MAX_H].count ? -1 : high);
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
    low = math_nonnegative(n, WE_LEN_MIN_H, parent, n->length[WE_LEN_MIN_H].count ? 0 : low);
    high = math_nonnegative(n, WE_LEN_MAX_H, parent, n->length[WE_LEN_MAX_H].count ? -1 : high);
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

static int image_dimensions(we_doc_t *d, int idx, int available, int assigned_w, int assigned_h, int *w, int *h) {
    const we_node_t *n = &d->nodes[idx];
    int nw, nh, parent, fixed;
    if (image_work.failed[idx]) return -1;
    if (!image_work.natural_w[idx] && image_open(d, idx)) return -1;
    nw = image_work.natural_w[idx]; nh = image_work.natural_h[idx];
    if (nw < 1 || nh < 1) return image_fail(d, "image dimensions invalid", idx);
    *w = n->width_pct > 0 ? available * n->width_pct / 100 : n->width >= 0 ? n->width : nw;
    *h = n->height >= 0 ? n->height : nh;
    if (modern_image(n)) {
        parent = containing_height[idx] >= 0 ? containing_height[idx] : definite_height(d, n->parent, 0);
        fixed = definite_height(d, idx, 0);
        if (assigned_w >= 0) *w = assigned_w;
        else *w = specified_width(n, available, fixed >= 0 ?
            fixed * (n->aspect_num ? n->aspect_num : nw) / (n->aspect_num ? n->aspect_den : nh) : nw);
        if (fixed >= 0) *h = fixed;
        else *h = n->aspect_num ? ratio_height(n, *w) : *w * nh / nw;
        if (assigned_h >= 0) *h = assigned_h;
        *h = height_constraint(d, idx, *h, parent);
    }
    if (*w < 1 || *h < 1 || *w > WE_IMAGE_MAX_W || *h > WE_IMAGE_MAX_H)
        return image_fail(d, "image output exceeds limit", idx);
    return 0;
}

static int measure_height(we_doc_t *d, int idx, int width, int depth);
static int measure_area_height(we_doc_t *d, int idx, int outer, int depth);
static int measure_grid(we_doc_t *d, int idx, int width, int depth);
static int measure_table(we_doc_t *d, int idx, int width, int depth);
static int measure_flex(we_doc_t *d, int idx, int width, int depth);
static int flow_children(we_doc_t *d, int idx, int x, int y, int width, int depth, int measuring);

static int measure_height(we_doc_t *d, int idx, int width, int depth) {
    const we_node_t *n = &d->nodes[idx];
    int fixed, content = 0, parent, ratio_width = 0;
    if (depth > WE_DEPTH_MAX || n->display == WE_DISP_NONE) return 0;
    {
        const we_measure_cache_t *cache = &measure_cache[idx];
        int key_width = forced_width[idx] >= 0 ? forced_width[idx] : width;
        int key_fixed = measure_content[idx] ? -1 : definite_height(d, idx, 0);
        int key_parent = containing_height[idx] >= 0 ? containing_height[idx] : definite_height(d, n->parent, 0);
        if (cache->valid && cache->width == key_width && cache->fixed == key_fixed &&
            cache->parent == key_parent && cache->content == measure_content[idx] && cache->probe == baseline_probe) {
            flow_baseline[idx] = (int16_t)cache->baseline;
            flow_last[idx] = (int16_t)cache->last;
            return cache->result;
        }
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
        cache->probe = (uint8_t)baseline_probe;
        cache->valid = 0;
    }
    flow_baseline[idx] = flow_last[idx] = -1;
    fixed = measure_content[idx] ? -1 : definite_height(d, idx, 0);
    if (fixed >= 0) content = fixed;
    else {
        if (forced_width[idx] >= 0) width = forced_width[idx] - node_border_extra(n) - n->margin[1] - n->margin[3];
        else width = specified_width(n, width, width - node_border_extra(n) - n->margin[1] - n->margin[3]);
        ratio_width = width;
        if (width < WE_CHAR_W) width = WE_CHAR_W;
        if (w_eq(n->tag, "img")) {
            content = image_measure_height(d, idx, width, modern_image(n) && forced_width[idx] >= 0 ? width : -1);
            if (content < 0) content = WE_LINE_H;
        } else if (n->display == WE_DISP_GRID) content = measure_grid(d, idx, width, depth);
        else if (n->display == WE_DISP_TABLE) content = measure_table(d, idx, width, depth);
        else if (n->display == WE_DISP_FLEX) content = measure_flex(d, idx, width, depth);
        else content = flow_children(d, idx, 0, 0, width, depth, 1);
        if (n->aspect_num && !w_eq(n->tag, "img")) {
            int ratio = ratio_height(n, ratio_width);
            if (ratio > content) content = ratio;
        }
    }
    parent = containing_height[idx] >= 0 ? containing_height[idx] : definite_height(d, n->parent, 0);
    content = height_constraint(d, idx, content, parent);
    measure_cache[idx].result = limit_coord(content + vertical_extra(n) + n->margin[0] + n->margin[2]);
    measure_cache[idx].baseline = flow_baseline[idx];
    measure_cache[idx].last = flow_last[idx];
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
    /* text-indent shifts the first inline line, not the float. */
    x = n->float_side == WE_FLOAT_RIGHT ? f->x0 + f->width - width : f->x0 - f->indent_shift;
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

static void clear_floats(we_inline_t *f, int side) {
    int guard;
    if (side == WE_CLEAR_NONE) return;
    for (guard = 0; guard <= WE_FLOATS_MAX; guard++) {
        int i, next = WE_LAYOUT_MAX, hit = 0;
        for (i = 0; i < f->nfloat; i++) {
            int bottom = f->floats[i].y + f->floats[i].h;
            int fs = f->floats[i].side;
            if (bottom <= f->y) continue;
            if (side == WE_CLEAR_LEFT && fs != WE_FLOAT_LEFT) continue;
            if (side == WE_CLEAR_RIGHT && fs != WE_FLOAT_RIGHT) continue;
            hit = 1;
            if (bottom < next) next = bottom;
        }
        if (!hit || next <= f->y) return;
        f->y = next;
    }
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
    f->origin_y = f->y; f->baseline = f->last = -1;
    f->lay_first = f->lay_last = WE_NO_BASELINE;
    f->line_start = (int)d->nitems;
    f->flow = idx;
    f->align = d->nodes[idx].align;
    f->align_last = d->nodes[idx].align_last;
    f->justify_none = d->nodes[idx].justify_none;
    f->start = d->nodes[idx].dir_rtl ? WE_ALIGN_RIGHT : WE_ALIGN_LEFT;
    f->line_h = d->nodes[idx].line_height > 0 ? d->nodes[idx].line_height : WE_LINE_H;
    f->indent = d->nodes[idx].text_indent;
    for (c = d->nodes[idx].first; c >= 0; c = d->nodes[c].next) {
        we_node_t *n = &d->nodes[c];
        if (n->html_space) f->pending_space = 1;
        if (n->absolute && n->display != WE_DISP_NONE && !measuring) {
            int open = f->nwords || f->pending_x > 0 || f->atom_h > 0;
            if (is_box(n->display)) set_static(c, f->x0, limit_coord(f->y + (open ? line_step(f) : 0)));
            else set_static(c, f->x0 + f->cur_w, f->y);
        }
        if (n->display == WE_DISP_NONE || n->absolute) continue;
        if (n->float_side && !n->is_text) {
            flush_line(f, 0);
            clear_floats(f, n->clear_side);
            place_float(f, c, depth + 1);
        } else if (is_box(n->display)) {
            flush_line(f, 0);
            clear_floats(f, n->clear_side);
            flow_region(f, WE_CHAR_W);
            /* A block child is not the indented first line. */
            {
                int height = measuring ? measure_height(d, c, f->width + f->indent_used, depth + 1) :
                    layout_block(d, c, f->x0 - f->indent_shift, f->y, f->width + f->indent_used, depth + 1);
                if (measuring && baseline_probe) {
                    int outer = preferred_outer(n, f->width, f->width - node_border_extra(n) - n->margin[1] - n->margin[3]);
                    int real, base;
                    if (f->baseline < 0) {
                        base = baseline_get(d, c, outer, depth + 1, 0, &real);
                        if (real) f->baseline = f->y - f->origin_y + base;
                    }
                    base = baseline_get(d, c, outer, depth + 1, 1, &real);
                    if (real) f->last = f->y - f->origin_y + base;
                }
                if (!measuring) {
                    if (f->lay_first == WE_NO_BASELINE) f->lay_first = layout_first[c];
                    if (layout_last[c] != WE_NO_BASELINE) f->lay_last = layout_last[c];
                }
                f->y = limit_coord(f->y + height);
            }
            f->pending_space = 0;
        } else inline_node(f, c, depth + 1);
    }
    flush_line(f, 0);
    if (measuring) {
        flow_baseline[idx] = (int16_t)f->baseline;
        flow_last[idx] = (int16_t)f->last;
    } else {
        layout_first[idx] = (int16_t)f->lay_first;
        layout_last[idx] = (int16_t)f->lay_last;
    }
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
    dx = n->inset_flags & WE_INSET_LEFT ? math_value(n, WE_LEN_LEFT, containing_w, dx) :
         n->inset_flags & WE_INSET_RIGHT ? -math_value(n, WE_LEN_RIGHT, containing_w, n->offset_r) : 0;
    if (high < low) dx = 0;
    else { if (dx < low) dx = low; if (dx > high) dx = high; }
    if (parent && definite_height(d, n->parent, 0) >= 0) {
        int top = parent->y + parent->border + parent->padding[0];
        dy = n->inset_flags & WE_INSET_TOP ? math_value(n, WE_LEN_TOP, definite_height(d, n->parent, 0), dy) :
             n->inset_flags & WE_INSET_BOTTOM ? -math_value(n, WE_LEN_BOTTOM, definite_height(d, n->parent, 0), n->offset_b) : 0;
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

static int layout_area(we_doc_t *d, int idx, int x, int y, int width, int height, int containing, int depth);
static void place_absolutes(we_doc_t *d, int idx, int pad_x, int pad_y, int pad_w, int pad_h, int depth);

/* ---------- Lot 62: box-shadow paint ----------
 * Outer shadows paint under the background, inset shadows over it and
 * under the content (CSS Backgrounds 3, 7.1), the first shadow on top.
 * Their items are reserved before the children and filled once the box
 * size is known; unused reserved items are removed after layout. An outer
 * shadow is the border box moved by the offsets and grown by the spread,
 * minus the border box itself; an inset shadow is the padding box minus
 * the padding box moved and shrunk. A blur radius b is two layers of half
 * the alpha each, the shape grown and shrunk by b / 2 (composited, so the
 * core keeps the full alpha). At most 4 items per layer. */
#define WE_ITEM_RESERVED 1
#define WE_ITEM_OUTLINE 2

static int shadow_layers(const we_shadow_t *sh) {
    return sh->blur ? 2 : 1;
}

/* Lot 62 round 2: a rounded box paints a shadow rounded when an opaque
 * colour background covers what the shadow must not show: an outer shadow
 * is then its whole shape under a border-box background, an inset shadow
 * without blur the padding-box shape with the background repainted inside
 * the inset shape (cut to the padding box). Other shadows of a rounded box
 * stay square ("rounded effect painted square"). */
static int shadow_rounded(const we_node_t *n, const we_shadow_t *sh) {
    if (!(n->rad_x[0] | n->rad_x[1] | n->rad_x[2] | n->rad_x[3]) || !n->has_bg || (n->bg >> WE_ALPHA_SHIFT)) return 0;
    if (!sh->inset) return n->bg_clip == WE_BOX_BORDER;
    return !sh->blur && n->bg_clip != WE_BOX_CONTENT && !n->grad_dir;
}

static int shadow_slot_count(const we_node_t *n, const we_shadow_t *sh) {
    if (!shadow_rounded(n, sh)) return 4 * shadow_layers(sh);
    return WE_RADIUS_SLICES * (sh->inset ? 2 : shadow_layers(sh));
}

static int shadow_reserve(we_doc_t *d, int idx, int inset, int *count) {
    const we_node_t *n = &d->nodes[idx];
    int first = (int)d->nitems, k, j;
    *count = 0;
    for (k = 0; k < n->shadow_count; k++) {
        if (!n->shadow[k].inset != !inset) continue;
        for (j = 0; j < shadow_slot_count(n, &n->shadow[k]); j++) {
            os_fb_web_item_t *it = emit(d, idx, OS_FB_WEB_RECT, 0, 0, 0, 0, 0);
            if (!it) return first;
            it->kind = 0;
            it->pad = WE_ITEM_RESERVED;
            (*count)++;
        }
    }
    return first;
}

/* Lot 62: colour of the gradient line at p (1/10000), interpolated in
 * premultiplied alpha between the surrounding stops. */
static uint32_t gradient_at(const we_node_t *n, int p) {
    int k = 0, t, a0, a1, total, ch, shift;
    uint32_t c0, c1, out = 0;
    if (p <= n->grad_pos[0]) return n->grad_color[0];
    while (k + 1 < n->grad_count && p > n->grad_pos[k + 1]) k++;
    if (k + 1 >= n->grad_count) return n->grad_color[n->grad_count - 1];
    if (n->grad_pos[k + 1] == n->grad_pos[k]) return n->grad_color[k + 1];
    t = (p - n->grad_pos[k]) * 10000 / (n->grad_pos[k + 1] - n->grad_pos[k]);
    c0 = n->grad_color[k]; c1 = n->grad_color[k + 1];
    a0 = 255 - (int)(c0 >> 24); a1 = 255 - (int)(c1 >> 24);
    total = a0 * (10000 - t) + a1 * t;
    if (total == 0) return with_alpha(0, 0);
    for (ch = 0, shift = 0; ch < 3; ch++, shift += 8) {
        int v0 = (int)((c0 >> shift) & 0xFF), v1 = (int)((c1 >> shift) & 0xFF);
        int num = v0 * a0 * (10000 - t) + v1 * a1 * t;
        out |= (uint32_t)((num + total / 2) / total) << shift;
    }
    return with_alpha(out, (total + 5000) / 10000);
}

#define WE_RADIAL_RINGS 3
#define WE_RADIAL_SLOTS (1 + WE_RADIAL_RINGS * 8)   /* 5 ellipse cuts + 2 clip cuts per ring */
/* lot 64: a translucent stop paints disjoint rings, two pieces per row */
#define WE_RADIAL_ALPHA_SLOTS 64
static int radial_translucent(const we_node_t *n) {
    int k;
    for (k = 0; k < n->grad_count; k++) if (n->grad_color[k] >> WE_ALPHA_SHIFT) return 1;
    return 0;
}

static int gradient_reserve(we_doc_t *d, int idx, int *count) {
    int first = (int)d->nitems, k, want;
    *count = 0;
    if (!d->nodes[idx].grad_dir) return first;
    want = d->nodes[idx].grad_dir != WE_GRAD_RADIAL ? WE_GRAD_BANDS :
           radial_translucent(&d->nodes[idx]) ? WE_RADIAL_ALPHA_SLOTS : WE_RADIAL_SLOTS;
    for (k = 0; k < want; k++) {
        os_fb_web_item_t *it = emit(d, idx, OS_FB_WEB_RECT, 0, 0, 0, 0, 0);
        if (!it) break;
        it->kind = 0;
        it->pad = WE_ITEM_RESERVED;
        (*count)++;
    }
    return first;
}

static int gradient_banded;

/* Bands tile the border box exactly; each takes the colour at its middle. */
/* Lot 62: ordinal of an li: the ol start (default 1, or the item count
 * when reversed), then +1 (or -1) per displayed li, an li value resetting
 * it (HTML list owner numbering, direct children only). */
static int list_ordinal(const we_doc_t *d, int idx) {
    const we_node_t *n = &d->nodes[idx];
    const we_node_t *owner = n->parent >= 0 ? &d->nodes[n->parent] : 0;
    int sibling, number, step = owner && owner->list_reversed ? -1 : 1, count = 0;
    if (!owner) return n->list_value_set ? n->list_value : 1;
    for (sibling = owner->first; sibling >= 0; sibling = d->nodes[sibling].next)
        if (w_eq(d->nodes[sibling].tag, "li") && d->nodes[sibling].display != WE_DISP_NONE) count++;
    number = owner->list_value_set ? owner->list_value : step < 0 ? count : 1;
    number -= step;
    for (sibling = owner->first; sibling >= 0; sibling = d->nodes[sibling].next) {
        const we_node_t *s = &d->nodes[sibling];
        if (!w_eq(s->tag, "li") || s->display == WE_DISP_NONE) { if (sibling == idx) break; continue; }
        number = s->list_value_set ? s->list_value : number + step;
        if (sibling == idx) break;
    }
    return number;
}

/* Lot 62: marker text; alpha needs 1.., roman 1..3999, else decimal. */
static int list_marker(int style, int number, char *out) {
    int len = 0;
    if ((style == WE_LIST_LOWER_ALPHA || style == WE_LIST_UPPER_ALPHA) && number >= 1) {
        char rev[8];
        int k = 0, base = style == WE_LIST_LOWER_ALPHA ? 'a' : 'A';
        while (number > 0 && k < 7) { number--; rev[k++] = (char)(base + number % 26); number /= 26; }
        while (k) out[len++] = rev[--k];
    } else if ((style == WE_LIST_LOWER_ROMAN || style == WE_LIST_UPPER_ROMAN) && number >= 1 && number <= 3999) {
        static const int values[13] = { 1000, 900, 500, 400, 100, 90, 50, 40, 10, 9, 5, 4, 1 };
        static const char *const upper[13] = { "M", "CM", "D", "CD", "C", "XC", "L", "XL", "X", "IX", "V", "IV", "I" };
        int k;
        for (k = 0; k < 13; k++) while (number >= values[k]) {
            const char *p = upper[k];
            while (*p) { out[len++] = style == WE_LIST_LOWER_ROMAN ? (char)(*p + 32) : *p; p++; }
            number -= values[k];
        }
    } else {
        if (style == WE_LIST_LEADING_ZERO && number > -10 && number < 10) {
            if (number < 0) { out[len++] = '-'; number = -number; }
            out[len++] = '0';
        }
        w_itoa(number, out + len);
        len = w_len(out);
    }
    out[len] = 0;
    return len;
}

/* Lot 63: counter text in a list style; the bullet styles use the same
 * ASCII glyphs as the list markers, none gives nothing. */
static int counter_text(int style, int number, char *out) {
    if (style == WE_LIST_NONE) { out[0] = 0; return 0; }
    if (style == WE_LIST_DISC || style == WE_LIST_SQUARE || style == WE_LIST_CIRCLE) {
        out[0] = style == WE_LIST_SQUARE ? '#' : style == WE_LIST_CIRCLE ? 'o' : '*';
        out[1] = 0;
        return 1;
    }
    return list_marker(style, number, out);
}

/* ---------- Lot 62: rounded shapes as row slices ----------
 * A shape is a box (w x h) with elliptical corners; it is cut into row
 * bands at WE_RADIUS_STEPS rows per corner, each band inset on the left
 * and right by the corner ellipse at the band's middle row. */
typedef struct { int rx[4], ry[4]; } we_radii_t;
static int radius_square;

static uint32_t isqrt32(uint32_t v) {
    uint32_t r = 0, bit = 1u << 30;
    while (bit > v) bit >>= 2;
    while (bit) {
        if (v >= r + bit) { v -= r + bit; r = (r >> 1) + bit; } else r >>= 1;
        bit >>= 2;
    }
    return r;
}

/* Used radii of a w x h border box: percentages resolved, then all radii
 * scaled down by the smallest side / (sum of its two radii) below 1. */
static void radii_scale(we_radii_t *r, int w, int h);
static void used_radii(const we_node_t *n, int w, int h, we_radii_t *r) {
    int k;
    for (k = 0; k < 4; k++) {
        r->rx[k] = n->rad_pct & (1u << k) ? n->rad_x[k] * w / 100 : n->rad_x[k];
        r->ry[k] = n->rad_pct & (1u << (k + 4)) ? n->rad_y[k] * h / 100 : n->rad_y[k];
        if (r->rx[k] > WE_RADIUS_MAX) r->rx[k] = WE_RADIUS_MAX;
        if (r->ry[k] > WE_RADIUS_MAX) r->ry[k] = WE_RADIUS_MAX;
        if (!r->rx[k] || !r->ry[k]) r->rx[k] = r->ry[k] = 0;
    }
    radii_scale(r, w, h);
}

static void radii_scale(we_radii_t *r, int w, int h) {
    static const int8_t side_corners[4][3] = { {0, 1, 0}, {1, 2, 1}, {3, 2, 0}, {0, 3, 1} };
    int k, num = 1, den = 1;
    for (k = 0; k < 4; k++) {
        int a = side_corners[k][0], b = side_corners[k][1], vertical = side_corners[k][2];
        int sum = vertical ? r->ry[a] + r->ry[b] : r->rx[a] + r->rx[b], len = vertical ? h : w;
        if (sum > len && len * den < num * sum) { num = len; den = sum; }
    }
    if (num < den) for (k = 0; k < 4; k++) {
        r->rx[k] = r->rx[k] * num / den; r->ry[k] = r->ry[k] * num / den;
        if (!r->rx[k] || !r->ry[k]) r->rx[k] = r->ry[k] = 0;
    }
}

static int radii_any(const we_radii_t *r) {
    return r->rx[0] || r->rx[1] || r->rx[2] || r->rx[3];
}

/* inset of one corner at doubled row y2 counted from the corner's edge */
static int corner_inset(int rx, int ry, int y2) {
    int r2 = 2 * ry, t2 = r2 - y2;
    uint32_t q;
    if (!rx || y2 >= r2) return 0;
    if (t2 > r2) t2 = r2;
    q = isqrt32((uint32_t)(rx * rx) * (uint32_t)(r2 * r2 - t2 * t2));
    return rx - (int)((q + (uint32_t)ry) / (uint32_t)r2);
}

static void shape_insets(const we_radii_t *r, int h, int y2, int *left, int *right) {
    int tl = corner_inset(r->rx[0], r->ry[0], y2), tr = corner_inset(r->rx[1], r->ry[1], y2);
    int bl = corner_inset(r->rx[3], r->ry[3], 2 * h - y2), br = corner_inset(r->rx[2], r->ry[2], 2 * h - y2);
    *left = tl > bl ? tl : bl;
    *right = tr > br ? tr : br;
}

static int add_break(int *list, int count, int cap, int v, int h) {
    int k;
    if (v <= 0 || v >= h) return count;
    for (k = 0; k < count; k++) if (list[k] == v) return count;
    if (count < cap) list[count++] = v;
    return count;
}

static int shape_breaks(const we_radii_t *r, int h, int offset, int *list, int count, int cap) {
    int c, s;
    for (c = 0; c < 4; c++) for (s = 1; s <= WE_RADIUS_STEPS; s++) {
        int at = r->ry[c] * s / WE_RADIUS_STEPS;
        if (!r->ry[c]) continue;
        count = add_break(list, count, cap, offset + (c < 2 ? at : h - at), offset + h + 1);
    }
    return count;
}

static void sort_ints(int *list, int count) {
    int i, j;
    for (i = 1; i < count; i++) {
        int v = list[i];
        for (j = i; j > 0 && list[j - 1] > v; j--) list[j] = list[j - 1];
        list[j] = v;
    }
}

/* Rounded background: slices into the reserved slots (unused stay reserved). */
static int radius_fill(we_doc_t *d, int slot, int count, const we_radii_t *r, int x, int y, int w, int h, uint32_t color) {
    int breaks[WE_RADIUS_SLICES + 2], n = 0, k, used = 0, prev_l = -1, prev_r = -1;
    n = shape_breaks(r, h, 0, breaks + 1, 0, WE_RADIUS_SLICES - 1);
    breaks[0] = 0;
    sort_ints(breaks + 1, n);
    breaks[n + 1] = h;
    for (k = 0; k <= n; k++) {
        int a = breaks[k], b = breaks[k + 1], left, right;
        if (b <= a) continue;
        shape_insets(r, h, a + b, &left, &right);
        if (used && left == prev_l && right == prev_r) { d->items[slot + used - 1].h = (uint16_t)(d->items[slot + used - 1].h + b - a); continue; }
        if (used == count || w - left - right <= 0) { prev_l = -1; continue; }
        d->items[slot + used].kind = OS_FB_WEB_RECT;
        d->items[slot + used].color = color;
        d->items[slot + used].x = (int16_t)(x + left); d->items[slot + used].y = (int16_t)(y + a);
        d->items[slot + used].w = (uint16_t)(w - left - right); d->items[slot + used].h = (uint16_t)(b - a);
        used++;
        prev_l = left; prev_r = right;
    }
    return used;
}

/* Rounded solid border of thickness t: outer shape minus the padding-box
 * shape (radii reduced by t), up to two rects per band. */
static int radius_ring(we_doc_t *d, int idx, const we_radii_t *r, int x, int y, int w, int h, int t, uint32_t color, uint32_t far) {
    we_radii_t in;
    int breaks[2 * WE_RADIUS_SLICES + 4], n, k, emitted = 0;
    int pl = -2, pil = -2, pir = -2, pr = -2;
    uint32_t pc = 0;
    os_fb_web_item_t *last[2] = {0, 0};
    if (t <= 0 || w <= 0 || h <= 0) return 0;
    for (k = 0; k < 4; k++) {
        in.rx[k] = r->rx[k] > t ? r->rx[k] - t : 0;
        in.ry[k] = r->ry[k] > t ? r->ry[k] - t : 0;
        if (!in.rx[k] || !in.ry[k]) in.rx[k] = in.ry[k] = 0;
    }
    n = shape_breaks(r, h, 0, breaks + 1, 0, 2 * WE_RADIUS_SLICES);
    n = shape_breaks(&in, h - 2 * t, t, breaks + 1, n, 2 * WE_RADIUS_SLICES);
    n = add_break(breaks + 1, n, 2 * WE_RADIUS_SLICES + 2, t, h);
    n = add_break(breaks + 1, n, 2 * WE_RADIUS_SLICES + 2, h - t, h);
    breaks[0] = 0;
    sort_ints(breaks + 1, n);
    breaks[n + 1] = h;
    for (k = 0; k <= n; k++) {
        int a = breaks[k], b = breaks[k + 1], ol, orr, il = -1, ir = -1;
        uint32_t full;
        if (b <= a) continue;
        shape_insets(r, h, a + b, &ol, &orr);
        if (a >= t && b <= h - t && w > 2 * t) {
            shape_insets(&in, h - 2 * t, a + b - 2 * t, &il, &ir);
            il += t; ir += t;
        }
        /* lot 62 round 2: a full band takes color above the middle and far
         * below it, the left piece color and the right piece far */
        full = a + b < h ? color : far;
        if (ol == pl && orr == pr && il == pil && ir == pir && last[0] && (il >= 0 || full == pc)) {
            last[0]->h = (uint16_t)(last[0]->h + b - a);
            if (last[1]) last[1]->h = (uint16_t)(last[1]->h + b - a);
            continue;
        }
        pl = ol; pr = orr; pil = il; pir = ir; pc = full;
        last[0] = last[1] = 0;
        if (il < 0) {
            if (w - ol - orr > 0) last[0] = emit(d, idx, OS_FB_WEB_RECT, x + ol, y + a, w - ol - orr, b - a, full);
        } else {
            if (il > ol) last[0] = emit(d, idx, OS_FB_WEB_RECT, x + ol, y + a, il - ol, b - a, color);
            if (ir > orr) last[1] = emit(d, idx, OS_FB_WEB_RECT, x + w - ir, y + a, ir - orr, b - a, far);
            if (!last[0]) { last[0] = last[1]; last[1] = 0; }
        }
        emitted += (last[0] != 0) + (last[1] != 0);
    }
    return emitted;
}

static int radius_stepped;

/* radii of an inner box: each reduced by the adjacent insets */
static void radii_inset(const we_radii_t *r, we_radii_t *out, int top, int right, int bottom, int left) {
    static const int8_t xs[4] = {3, 1, 1, 3}, ys[4] = {0, 0, 2, 2};
    int k, inset[4];
    inset[0] = top; inset[1] = right; inset[2] = bottom; inset[3] = left;
    for (k = 0; k < 4; k++) {
        out->rx[k] = r->rx[k] > inset[(int)xs[k]] ? r->rx[k] - inset[(int)xs[k]] : 0;
        out->ry[k] = r->ry[k] > inset[(int)ys[k]] ? r->ry[k] - inset[(int)ys[k]] : 0;
        if (!out->rx[k] || !out->ry[k]) out->rx[k] = out->ry[k] = 0;
    }
}

/* ---------- Lot 62 round 2: shapes as row bands ----------
 * A shape is a box with corner radii. Its cuts are its top and bottom
 * edges and the WE_RADIUS_STEPS rows of each corner; between two cuts a
 * band takes the corner insets at its middle row. shape_bands() gives the
 * merged bands of a shape, or of the intersection of two shapes, as
 * (y0, y1, x0, x1) in page pixels. */
typedef struct { int x, y, w, h; we_radii_t r; } we_shape_t;
#define WE_SHAPE_CUTS (2 * (4 * WE_RADIUS_STEPS + 2))
#define WE_ROUND_BANDS (WE_SHAPE_CUTS + 1)

static void shape_set(we_shape_t *s, int x, int y, int w, int h, const we_radii_t *r) {
    s->x = x; s->y = y; s->w = w; s->h = h;
    s->r = *r;
}

static int cut_add(int *list, int count, int v, int lo, int hi) {
    int k;
    if (v <= lo || v >= hi || count >= WE_SHAPE_CUTS) return count;
    for (k = 0; k < count; k++) if (list[k] == v) return count;
    list[count] = v;
    return count + 1;
}

static int shape_cuts(const we_shape_t *s, int *list, int count, int lo, int hi) {
    int c, k;
    count = cut_add(list, count, s->y, lo, hi);
    count = cut_add(list, count, s->y + s->h, lo, hi);
    for (c = 0; c < 4; c++) for (k = 1; k <= WE_RADIUS_STEPS && s->r.ry[c]; k++) {
        int at = s->r.ry[c] * k / WE_RADIUS_STEPS;
        count = cut_add(list, count, c < 2 ? s->y + at : s->y + s->h - at, lo, hi);
    }
    return count;
}

/* span of a shape over rows [y0, y1), which lie inside or outside it */
static int shape_span(const we_shape_t *s, int y0, int y1, int *x0, int *x1) {
    int l, r;
    if (y0 < s->y || y1 > s->y + s->h || s->w <= 0) return 0;
    shape_insets(&s->r, s->h, y0 + y1 - 2 * s->y, &l, &r);
    *x0 = s->x + l;
    *x1 = s->x + s->w - r;
    return *x1 > *x0;
}

static int shape_bands(const we_shape_t *a, const we_shape_t *b, int band[][4], int cap) {
    int cut[WE_SHAPE_CUTS], n, k, used = 0, lo = a->y, hi = a->y + a->h;
    if (a->w <= 0 || a->h <= 0) return 0;
    n = shape_cuts(a, cut, 0, lo, hi);
    if (b) n = shape_cuts(b, cut, n, lo, hi);
    sort_ints(cut, n);
    for (k = 0; k <= n; k++) {
        int y0 = k ? cut[k - 1] : lo, y1 = k < n ? cut[k] : hi, x0, x1, u0, u1;
        if (!shape_span(a, y0, y1, &x0, &x1)) continue;
        if (b) {
            if (!shape_span(b, y0, y1, &u0, &u1)) continue;
            if (u0 > x0) x0 = u0;
            if (u1 < x1) x1 = u1;
            if (x1 <= x0) continue;
        }
        if (used && band[used - 1][1] == y0 && band[used - 1][2] == x0 && band[used - 1][3] == x1) { band[used - 1][1] = y1; continue; }
        if (used == cap) break;
        band[used][0] = y0; band[used][1] = y1; band[used][2] = x0; band[used][3] = x1;
        used++;
    }
    return used;
}

/* bands as RECTs into reserved slots */
static void bands_into(we_doc_t *d, int slot, int band[][4], int count, uint32_t color) {
    int k;
    for (k = 0; k < count; k++) {
        os_fb_web_item_t *it = &d->items[slot + k];
        it->kind = OS_FB_WEB_RECT;
        it->color = color;
        it->x = (int16_t)limit_coord(band[k][2]); it->y = (int16_t)limit_coord(band[k][0]);
        it->w = (uint16_t)(band[k][3] - band[k][2]); it->h = (uint16_t)(band[k][1] - band[k][0]);
    }
}

/* Radii of a shadow shape: each non-zero radius grows by the spread (or
 * shrinks, not below 0), then the radii are scaled to the shape. */
static void radii_spread(const we_radii_t *r, we_radii_t *out, int spread, int w, int h) {
    int k;
    for (k = 0; k < 4; k++) {
        out->rx[k] = r->rx[k] ? r->rx[k] + spread : 0;
        out->ry[k] = r->ry[k] ? r->ry[k] + spread : 0;
        if (out->rx[k] <= 0 || out->ry[k] <= 0) out->rx[k] = out->ry[k] = 0;
    }
    radii_scale(out, w > 0 ? w : 0, h > 0 ? h : 0);
}

/* Rounded borders: solid, double (two rings of (t + 1) / 3) and the 3D
 * styles, whose left and upper parts take one shade and the right and
 * lower parts the other (groove and ridge as two nested rings). */
static uint32_t shade_3d(uint32_t color, int light);
static void radius_border(we_doc_t *d, int idx, const we_radii_t *r, int x, int y, int w, int h, int t, int style, uint32_t color) {
    uint32_t dark = shade_3d(color, 0), light = shade_3d(color, 1);
    we_radii_t in;
    int k = (t + 1) / 3, outer = t / 2;
    if (style == 4 && t >= 3) {
        radius_ring(d, idx, r, x, y, w, h, k, color, color);
        radii_inset(r, &in, t - k, t - k, t - k, t - k);
        radius_ring(d, idx, &in, x + t - k, y + t - k, w - 2 * (t - k), h - 2 * (t - k), k, color, color);
    } else if (style == 8 || style == 10) {
        radius_ring(d, idx, r, x, y, w, h, t, style == 8 ? light : dark, style == 8 ? dark : light);
    } else if (style == 7 || style == 9) {
        uint32_t first = style == 9 ? dark : light, second = style == 9 ? light : dark;
        radius_ring(d, idx, r, x, y, w, h, outer, first, second);
        radii_inset(r, &in, outer, outer, outer, outer);
        radius_ring(d, idx, &in, x + outer, y + outer, w - 2 * outer, h - 2 * outer, t - outer, second, first);
    } else radius_ring(d, idx, r, x, y, w, h, t, color, color);
}

/* Lot 63: rounded dashed and dotted borders. The ring is painted, then
 * kept only inside the four corner boxes (rx x ry at each rounded corner),
 * so the arcs are solid; the straight parts between the corners take the
 * dash or dot pattern of edge_paint(), each side restarting its pattern.
 * Lot 64: an arc long enough for one period takes the pattern too (see
 * arc_pattern()); a shorter one stays solid. */
/* cos of k * 90 / 16 degrees, times 1024 (sin is the table read backwards) */
static const int16_t quarter_cos[17] = {1024, 1019, 1004, 980, 946, 903, 851, 792, 724, 650, 569, 483, 392, 297, 200, 100, 0};

static int quarter_trig(int at, int span, int sine) {
    /* the angle at / span of 90 degrees, interpolated over the table */
    int pos = span > 0 ? at * 16 * 64 / span : 0, k, f;
    if (pos < 0) pos = 0;
    if (pos > 16 * 64) pos = 16 * 64;
    if (sine) pos = 16 * 64 - pos;
    k = pos / 64; f = pos % 64;
    if (k >= 16) return quarter_cos[16];
    return quarter_cos[k] + (quarter_cos[k + 1] - quarter_cos[k]) * f / 64;
}

/* Lot 64: the dash or dot pattern along one corner arc. The arc's centre
 * line (radii less t / 2) has length L ~ pi / 4 (rx + ry); it holds
 * round(L / period) periods (at most WE_ARC_PERIODS), each painting its
 * piece centred in it as t x t squares one t apart along the line (one
 * square for a dot), cut to the corner box. Returns 0 when L is shorter
 * than one period (the arc stays solid). */
#define WE_ARC_PERIODS 4
static int arc_pattern(we_doc_t *d, int idx, int corner, int x0, int y0, int rx, int ry, int t, int style, uint32_t color) {
    int piece = style == 5 ? (2 * t > 3 ? 2 * t : 3) : t, period = 2 * piece;
    int ax = rx - t / 2, ay = ry - t / 2, len, count, j, sx, sy, cx, cy;
    if (ax < 1) ax = 1;
    if (ay < 1) ay = 1;
    len = 157 * (ax + ay) / 200;
    if (len < period) return 0;
    count = (len + period / 2) / period;
    if (count > WE_ARC_PERIODS) count = WE_ARC_PERIODS;
    sx = corner == 0 || corner == 3 ? -1 : 1;
    sy = corner < 2 ? -1 : 1;
    cx = corner == 0 || corner == 3 ? x0 + rx : x0;            /* the ellipse centre */
    cy = corner < 2 ? y0 + ry : y0;
    for (j = 0; j < count; j++) {
        int mid = (2 * j + 1) * len / (2 * count), squares = style == 5 ? (piece + t - 1) / t : 1, q;
        for (q = 0; q < squares; q++) {
            int at = mid - (squares - 1) * t / 2 + q * t;
            int px = cx + sx * ax * quarter_trig(at, len, 0) / 1024, py = cy + sy * ay * quarter_trig(at, len, 1) / 1024;
            int bx = px - t / 2, by = py - t / 2, ex = bx + t, ey = by + t;
            if (bx < x0) bx = x0;
            if (by < y0) by = y0;
            if (ex > x0 + rx) ex = x0 + rx;
            if (ey > y0 + ry) ey = y0 + ry;
            if (ex > bx && ey > by) (void)emit(d, idx, OS_FB_WEB_RECT, bx, by, ex - bx, ey - by, color);
        }
    }
    return 1;
}

static void edge_paint(we_doc_t *d, int idx, int style, uint32_t color, int vertical, int off, int from, int w, int len);
static void radius_dashed(we_doc_t *d, int idx, const we_radii_t *r, int x, int y, int w, int h, int t, int style, uint32_t color) {
    static os_fb_web_item_t ring[OS_FB_WEB_ITEMS];
    int start = (int)d->nitems, count, i, k;
    int cx[4], cy[4], from[4], patterned[4];
    radius_ring(d, idx, r, x, y, w, h, t, color, color);
    count = (int)d->nitems - start;
    for (i = 0; i < count; i++) ring[i] = d->items[start + i];
    d->nitems = (uint32_t)start;
    cx[0] = cx[3] = x; cx[1] = cx[2] = x + w;
    cy[0] = cy[1] = y; cy[2] = cy[3] = y + h;
    for (k = 0; k < 4; k++) {
        int x0 = k == 0 || k == 3 ? cx[k] : cx[k] - r->rx[k], y0 = k < 2 ? cy[k] : cy[k] - r->ry[k];
        patterned[k] = r->rx[k] && r->ry[k] && arc_pattern(d, idx, k, x0, y0, r->rx[k], r->ry[k], t, style, color);
    }
    for (i = 0; i < count; i++)
        for (k = 0; k < 4; k++) {
            int x0 = k == 0 || k == 3 ? cx[k] : cx[k] - r->rx[k], x1 = x0 + r->rx[k];
            int y0 = k < 2 ? cy[k] : cy[k] - r->ry[k], y1 = y0 + r->ry[k];
            if (patterned[k]) continue;
            if (ring[i].x > x0) x0 = ring[i].x;
            if (ring[i].y > y0) y0 = ring[i].y;
            if (ring[i].x + ring[i].w < x1) x1 = ring[i].x + ring[i].w;
            if (ring[i].y + ring[i].h < y1) y1 = ring[i].y + ring[i].h;
            if (x1 > x0 && y1 > y0) (void)emit(d, idx, OS_FB_WEB_RECT, x0, y0, x1 - x0, y1 - y0, color);
        }
    edge_paint(d, idx, style, color, 0, y, x + r->rx[0], t, w - r->rx[0] - r->rx[1]);
    edge_paint(d, idx, style, color, 0, y + h - t, x + r->rx[3], t, w - r->rx[3] - r->rx[2]);
    /* a square corner keeps the horizontal side's t x t square */
    for (k = 0; k < 4; k++) from[k] = r->ry[k] ? r->ry[k] : t;
    edge_paint(d, idx, style, color, 1, x, y + from[0], t, h - from[0] - from[3]);
    edge_paint(d, idx, style, color, 1, x + w - t, y + from[1], t, h - from[1] - from[2]);
}

/* Lot 62: border, padding or content box of a laid out box, (x, y, w, h). */
static void background_box(const we_node_t *n, int box, int bx, int by, int bw, int bh, int out[4]) {
    int top = 0, right = 0, bottom = 0, left = 0;
    if (box >= WE_BOX_PADDING) { top = box_edge(n, 0); right = box_edge(n, 1); bottom = box_edge(n, 2); left = box_edge(n, 3); }
    if (box == WE_BOX_CONTENT) { top += n->padding[0]; right += n->padding[1]; bottom += n->padding[2]; left += n->padding[3]; }
    out[0] = bx + left; out[1] = by + top;
    out[2] = bw - left - right; out[3] = bh - top - bottom;
    if (out[2] < 0) out[2] = 0;
    if (out[3] < 0) out[3] = 0;
}

/* Bands tile the origin box exactly, each taking the colour at its middle,
 * then are cut to the clip box. The border area outside the origin box
 * stays unpainted (no image repetition). */
/* Lot 62 round 2: radial-gradient in the origin box; lot 63: centred at
 * its "at" position (default 50% 50%), with explicit sizes winning over
 * the extent. The ending shape: an ellipse through the nearer or farther
 * sides (closest/farthest-side) or through the corners (sqrt 2 larger,
 * the corner extents), a circle to the nearer or farther side distance or
 * to the corner, the distances taken from the centre. Painted as the last stop
 * over the clip box, then WE_RADIAL_RINGS nested ellipses from the
 * outside in, each taking the colour at its band's middle radius and cut
 * to the clip box. Lot 64: with a translucent stop the rings must not
 * overlap, so radial_rings() paints each one minus the next inner one. */
static int radial_rings(we_doc_t *d, int idx, int slot, int count, const int *c, int cx, int cy, int rx, int ry);
static void radial_fill(we_doc_t *d, int idx, int slot, int count, const int *o, const int *c) {
    const we_node_t *n = &d->nodes[idx];
    static int band[WE_ROUND_BANDS][4];
    int extent = n->grad_shape >> 1, rx, ry, ring, used = 1, k, far, sx, sy;
    uint32_t opaque = 0xFFFFFFFFu;
    int cx = o[0] + (n->grad_at_pct & 1 ? o[2] * n->grad_at[0] / 100 : n->grad_at[0]);
    int cy = o[1] + (n->grad_at_pct & 2 ? o[3] * n->grad_at[1] / 100 : n->grad_at[1]);
    we_shape_t clip, ring_shape;
    we_radii_t zero, r;
    if (o[2] <= 0 || o[3] <= 0 || c[2] <= 0 || c[3] <= 0) return;
    /* side distances from the centre: the nearer (far 0) or farther pair */
    far = extent == WE_EXT_FARTHEST_SIDE || extent == WE_EXT_FARTHEST_CORNER;
    sx = cx - o[0]; k = o[0] + o[2] - cx;
    if (far ? k > sx : k < sx) sx = k;
    sy = cy - o[1]; k = o[1] + o[3] - cy;
    if (far ? k > sy : k < sy) sy = k;
    if (sx < 0) sx = -sx;
    if (sy < 0) sy = -sy;
    if (n->grad_size[0] >= 0) {
        rx = n->grad_at_pct & 4 ? o[2] * n->grad_size[0] / 100 : n->grad_size[0];
        ry = n->grad_at_pct & 8 ? o[3] * n->grad_size[1] / 100 : n->grad_size[1];
    } else if (n->grad_shape & 1) {
        int low = sx < sy ? sx : sy, high = sx < sy ? sy : sx;
        rx = extent == WE_EXT_CLOSEST_SIDE ? low : extent == WE_EXT_FARTHEST_SIDE ? high :
             (int)isqrt32((uint32_t)(sx * sx + sy * sy));
        ry = rx;
    } else if (extent == WE_EXT_CLOSEST_SIDE || extent == WE_EXT_FARTHEST_SIDE) {
        rx = sx; ry = sy;
    } else {
        rx = sx * 1414 / 1000; ry = sy * 1414 / 1000;
    }
    if (rx < 1) rx = 1;
    if (ry < 1) ry = 1;
    if (radial_translucent(n)) {
        if (radial_rings(d, idx, slot, count, c, cx, cy, rx, ry)) return;
        diag(d, "radial alpha painted opaque", idx);     /* the nested rings, without their alpha */
        opaque = 0xFFFFFFu;
    }
    w_zero(&zero, sizeof(zero));
    shape_set(&clip, c[0], c[1], c[2], c[3], &zero);
    d->items[slot].kind = OS_FB_WEB_RECT;
    d->items[slot].color = gradient_at(n, 10000) & opaque;
    d->items[slot].x = (int16_t)c[0]; d->items[slot].y = (int16_t)c[1];
    d->items[slot].w = (uint16_t)c[2]; d->items[slot].h = (uint16_t)c[3];
    for (ring = WE_RADIAL_RINGS; ring >= 1; ring--) {
        int ex = rx * ring / WE_RADIAL_RINGS, ey = ry * ring / WE_RADIAL_RINGS, j;
        if (!ex || !ey) continue;
        for (j = 0; j < 4; j++) { r.rx[j] = ex; r.ry[j] = ey; }
        shape_set(&ring_shape, cx - ex, cy - ey, 2 * ex, 2 * ey, &r);
        k = shape_bands(&ring_shape, &clip, band, count - used);
        bands_into(d, slot + used, band, k, gradient_at(n, (2 * ring - 1) * 10000 / (2 * WE_RADIAL_RINGS)) & opaque);
        used += k;
    }
}

/* Lot 64: the ring between ellipse k (outer, the clip box for k = rings + 1)
 * and ellipse k - 1, over one row partition shared by all the ellipses so
 * that neighbouring rings meet exactly; a row gives up to two pieces, and
 * a piece continuing the same columns of the row above is merged into it.
 * Returns 0 (nothing painted) when the pieces need more than count slots. */
static int radial_rings(we_doc_t *d, int idx, int slot, int count, const int *c, int cx, int cy, int rx, int ry) {
    static int band[WE_RADIAL_ALPHA_SLOTS][4];
    static uint32_t tone[WE_RADIAL_ALPHA_SLOTS];
    const we_node_t *n = &d->nodes[idx];
    int cut[WE_SHAPE_CUTS], ncut = 0, used = 0, ring, k, j, piece[2][2], np;
    we_shape_t shape[WE_RADIAL_RINGS + 2];
    we_radii_t zero, r;
    w_zero(&zero, sizeof(zero));
    shape_set(&shape[0], 0, 0, 0, 0, &zero);                          /* the innermost hole: none */
    for (ring = 1; ring <= WE_RADIAL_RINGS; ring++) {
        int ex = rx * ring / WE_RADIAL_RINGS, ey = ry * ring / WE_RADIAL_RINGS;
        for (j = 0; j < 4; j++) { r.rx[j] = ex; r.ry[j] = ey; }
        shape_set(&shape[ring], cx - ex, cy - ey, ex && ey ? 2 * ex : 0, ex && ey ? 2 * ey : 0, &r);
        ncut = shape_cuts(&shape[ring], cut, ncut, c[1], c[1] + c[3]);
    }
    shape_set(&shape[WE_RADIAL_RINGS + 1], c[0], c[1], c[2], c[3], &zero);
    sort_ints(cut, ncut);
    for (ring = WE_RADIAL_RINGS + 1; ring >= 1; ring--) {
        int first = used;
        uint32_t color = gradient_at(n, ring > WE_RADIAL_RINGS ? 10000 : (2 * ring - 1) * 10000 / (2 * WE_RADIAL_RINGS));
        for (k = 0; k <= ncut; k++) {
            int y0 = k ? cut[k - 1] : c[1], y1 = k < ncut ? cut[k] : c[1] + c[3], x0, x1, u0, u1, p;
            if (y1 <= y0 || !shape_span(&shape[ring], y0, y1, &x0, &x1)) continue;
            if (x0 < c[0]) x0 = c[0];
            if (x1 > c[0] + c[2]) x1 = c[0] + c[2];
            np = 0;
            if (shape[ring - 1].w > 0 && shape_span(&shape[ring - 1], y0, y1, &u0, &u1) && u1 > x0 && u0 < x1) {
                if (u0 > x0) { piece[np][0] = x0; piece[np][1] = u0; np++; }
                if (u1 < x1) { piece[np][0] = u1; piece[np][1] = x1; np++; }
            } else if (x1 > x0) { piece[np][0] = x0; piece[np][1] = x1; np++; }
            for (p = 0; p < np; p++) {
                int merged = 0;
                for (j = used - 1; j >= first && j >= used - 2 && !merged; j--)
                    if (band[j][1] == y0 && band[j][2] == piece[p][0] && band[j][3] == piece[p][1]) { band[j][1] = y1; merged = 1; }
                if (merged) continue;
                if (used == count || used == WE_RADIAL_ALPHA_SLOTS) return 0;
                band[used][0] = y0; band[used][1] = y1; band[used][2] = piece[p][0]; band[used][3] = piece[p][1];
                tone[used++] = color;
            }
        }
    }
    for (k = 0; k < used; k++) bands_into(d, slot + k, band + k, 1, tone[k]);
    return 1;
}

static void gradient_fill(we_doc_t *d, int idx, int slot, int count, int bx, int by, int bw, int bh) {
    const we_node_t *n = &d->nodes[idx];
    int vertical = n->grad_dir == WE_GRAD_BOTTOM || n->grad_dir == WE_GRAD_TOP;
    int o[4], c[4], extent, bands, k;
    if (!n->grad_dir) return;
    if (n->grad_dir == WE_GRAD_RADIAL) {
        background_box(n, n->bg_origin, bx, by, bw, bh, o);
        background_box(n, n->bg_clip, bx, by, bw, bh, c);
        for (k = 1; k < n->grad_count; k++) if (n->grad_color[k] != n->grad_color[0]) gradient_banded = 1;
        if (count > 0) radial_fill(d, idx, slot, count, o, c);
        return;
    }
    background_box(n, n->bg_origin, bx, by, bw, bh, o);
    background_box(n, n->bg_clip, bx, by, bw, bh, c);
    extent = vertical ? o[3] : o[2];
    bands = extent < count ? extent : count;
    if (bands <= 0) return;
    for (k = 1; k < n->grad_count; k++) if (n->grad_color[k] != n->grad_color[0]) gradient_banded = 1;
    for (k = 0; k < bands; k++) {
        os_fb_web_item_t *it = &d->items[slot + k];
        int a = k * extent / bands, b = (k + 1) * extent / bands;
        int p = (2 * k + 1) * 10000 / (2 * bands);
        int x0 = vertical ? o[0] : o[0] + a, y0 = vertical ? o[1] + a : o[1];
        int x1 = vertical ? o[0] + o[2] : o[0] + b, y1 = vertical ? o[1] + b : o[1] + o[3];
        if (x0 < c[0]) x0 = c[0];
        if (y0 < c[1]) y0 = c[1];
        if (x1 > c[0] + c[2]) x1 = c[0] + c[2];
        if (y1 > c[1] + c[3]) y1 = c[1] + c[3];
        if (x1 <= x0 || y1 <= y0) continue;      /* stays reserved, dropped */
        if (n->grad_dir == WE_GRAD_TOP || n->grad_dir == WE_GRAD_LEFT) p = 10000 - p;
        it->kind = OS_FB_WEB_RECT;
        it->color = gradient_at(n, p);
        it->x = (int16_t)x0; it->y = (int16_t)y0;
        it->w = (uint16_t)(x1 - x0); it->h = (uint16_t)(y1 - y0);
    }
}

/* a minus b, both (x, y, w, h); at most 4 pieces into out (x, y, w, h each). */
static int rect_minus(const int *a, const int *b, int out[4][4]) {
    int ix = a[0] > b[0] ? a[0] : b[0], iy = a[1] > b[1] ? a[1] : b[1];
    int ir = a[0] + a[2] < b[0] + b[2] ? a[0] + a[2] : b[0] + b[2];
    int ib = a[1] + a[3] < b[1] + b[3] ? a[1] + a[3] : b[1] + b[3];
    int count = 0, k;
    int piece[4][4];
    if (a[2] <= 0 || a[3] <= 0) return 0;
    if (ir <= ix || ib <= iy) { for (k = 0; k < 4; k++) out[0][k] = a[k]; return 1; }
    piece[0][0] = a[0]; piece[0][1] = a[1]; piece[0][2] = a[2]; piece[0][3] = iy - a[1];
    piece[1][0] = a[0]; piece[1][1] = ib; piece[1][2] = a[2]; piece[1][3] = a[1] + a[3] - ib;
    piece[2][0] = a[0]; piece[2][1] = iy; piece[2][2] = ix - a[0]; piece[2][3] = ib - iy;
    piece[3][0] = ir; piece[3][1] = iy; piece[3][2] = a[0] + a[2] - ir; piece[3][3] = ib - iy;
    for (k = 0; k < 4; k++) {
        if (piece[k][2] <= 0 || piece[k][3] <= 0) continue;
        out[count][0] = piece[k][0]; out[count][1] = piece[k][1];
        out[count][2] = piece[k][2]; out[count][3] = piece[k][3];
        count++;
    }
    return count;
}

/* blur: the wide layer first (outer: grown by b / 2 more, inset: shrunk
 * by b / 2 more), then the narrow one */
static int shadow_grow(const we_shadow_t *sh, int layer) {
    return sh->spread + (shadow_layers(sh) == 2 ? (layer == 0 ? sh->blur / 2 : -(sh->blur / 2)) : 0);
}

static int shadow_alpha(const we_shadow_t *sh, int layer, int alpha) {
    return shadow_layers(sh) == 1 ? alpha : layer == 0 ? alpha / 2 : alpha * 255 / (2 * 255 - alpha);
}

/* A rounded shadow into cap slots; -1 when its bands need more. */
static int shadow_round(we_doc_t *d, const we_node_t *n, const we_shadow_t *sh, int slot, int cap,
                        const int *box, const we_radii_t *radii, uint32_t color) {
    static int band[WE_ROUND_BANDS][4], inner[WE_ROUND_BANDS][4];
    int count, extra, layer, used = 0, alpha = color_alpha_of(color), s = sh->spread;
    int px = box[0] + box_edge(n, 3), py = box[1] + box_edge(n, 0);
    int pw = box[2] - box_edge(n, 1) - box_edge(n, 3), ph = box[3] - box_edge(n, 0) - box_edge(n, 2);
    we_shape_t shape, pad;
    we_radii_t r, ri;
    if (!sh->inset) {
        for (layer = 0; layer < shadow_layers(sh); layer++) {
            int grow = shadow_grow(sh, layer), w = box[2] + 2 * grow, h = box[3] + 2 * grow;
            radii_spread(radii, &r, grow, w, h);
            shape_set(&shape, box[0] + sh->x - grow, box[1] + sh->y - grow, w, h, &r);
            count = shape_bands(&shape, 0, band, WE_ROUND_BANDS);
            if (used + count > cap) return -1;
            bands_into(d, slot + used, band, count, with_alpha(color, shadow_alpha(sh, layer, alpha)));
            used += count;
        }
        return used;
    }
    radii_inset(radii, &r, box_edge(n, 0), box_edge(n, 1), box_edge(n, 2), box_edge(n, 3));
    shape_set(&pad, px, py, pw, ph, &r);
    radii_spread(&r, &ri, -s, pw - 2 * s, ph - 2 * s);
    shape_set(&shape, px + sh->x + s, py + sh->y + s, pw - 2 * s, ph - 2 * s, &ri);
    count = shape_bands(&pad, 0, band, WE_ROUND_BANDS);
    extra = shape_bands(&shape, &pad, inner, WE_ROUND_BANDS);
    if (count + extra > cap) return -1;
    bands_into(d, slot, band, count, color);
    bands_into(d, slot + count, inner, extra, n->bg);
    return count + extra;
}

/* ---------- Lot 63: background images ----------
 * A block with a background-image url records where its image goes (after
 * its colour and gradient, before its inset shadow and children). Once its
 * box is known the tiles are sampled at the tile size, merged into runs
 * like an <img>, cut to the clip box, and inserted there in one move, so
 * the children still paint over them. A tile set that needs more runs than
 * the display list has room for is refused whole. */
/* auto keeps the natural size, contain/cover fit the area keeping the
 * ratio, one explicit size and auto keep the ratio too. */
static void bgi_tile(const we_node_t *n, int aw, int ah, int nw, int nh, int *tw, int *th) {
    *tw = nw; *th = nh;
    if (n->bgi_size_kind == WE_BGS_CONTAIN || n->bgi_size_kind == WE_BGS_COVER) {
        int by_width = aw * nh <= ah * nw;
        if (n->bgi_size_kind == WE_BGS_COVER) by_width = !by_width;
        if (by_width) { *tw = aw; *th = aw * nh / nw; }
        else { *th = ah; *tw = ah * nw / nh; }
    } else if (n->bgi_size_kind == WE_BGS_EXPLICIT) {
        int w = n->bgi_size[0], h = n->bgi_size[1];
        if (w >= 0 && (n->bgi_pos_pct & 4)) w = aw * w / 100;
        if (h >= 0 && (n->bgi_pos_pct & 8)) h = ah * h / 100;
        if (w < 0) w = h * nw / nh;
        if (h < 0) h = w * nh / nw;
        *tw = w; *th = h;
    }
}

static int bgi_wrap(int v, int t) {
    v %= t;
    return v < 0 ? v + t : v;
}

/* Lot 64: round fits a whole number of tiles in the area (the other axis
 * keeps the ratio when its size is auto); space fits as many whole tiles
 * as the area holds, the first and last at its edges, the free pixels
 * shared out as equal gaps (one tile only: placed by position, no repeat). */
static void bgi_round_space(const we_node_t *n, const int *area, int *tw, int *th, int *gap, int *repeat) {
    int axis, style[2], size_auto[2];
    style[0] = n->bgi_space & 3; style[1] = n->bgi_space >> 2 & 3;
    size_auto[0] = n->bgi_size_kind == WE_BGS_AUTO || (n->bgi_size_kind == WE_BGS_EXPLICIT && n->bgi_size[0] < 0);
    size_auto[1] = n->bgi_size_kind == WE_BGS_AUTO || (n->bgi_size_kind == WE_BGS_EXPLICIT && n->bgi_size[1] < 0);
    gap[0] = gap[1] = 0;
    for (axis = 0; axis < 2; axis++) {
        int *t = axis ? th : tw, *o = axis ? tw : th, extent = area[2 + axis], count, fit;
        if (!(*repeat & (1 << axis)) || *t < 1) continue;
        if (style[axis] == WE_BGR_ROUND) {
            count = (extent + *t / 2) / *t;
            if (count < 1) count = 1;
            fit = extent / count;
            if (fit < 1) fit = 1;
            if (style[!axis] != WE_BGR_ROUND && size_auto[!axis]) *o = *o * fit / *t;
            *t = fit;
            if (*o < 1) *o = 1;
        } else if (style[axis] == WE_BGR_SPACE) {
            count = extent / *t;
            if (count >= 2) gap[axis] = (extent - count * *t) / (count - 1);
            else *repeat &= ~(1 << axis);
        }
    }
}

/* the tile pixel at (col, row), 0 in a space gap */
static int bgi_pixel(int col, int row, int ox, int oy, int tw, int th, const int *gap, uint32_t *color) {
    int u = bgi_wrap(col - ox, tw + gap[0]), v = bgi_wrap(row - oy, th + gap[1]);
    if (u >= tw || v >= th) return 0;
    *color = image_sample(u, v, tw, th);
    return 1;
}

static int bgi_fill(we_doc_t *d, int idx, int at, int bx, int by, int bw, int bh) {
    we_node_t *n = &d->nodes[idx];
    int area[4], clip[4], tw, th, ox, oy, x0, y0, x1, y1, row, col, nr = 0, room = OS_FB_WEB_ITEMS - (int)d->nitems;
    int gap[2], repeat = n->bgi_repeat;
    if (!n->bgi_len || n->visibility || image_open(d, idx)) return 0;      /* an <img> never keeps a url */
    background_box(n, n->bg_origin, bx, by, bw, bh, area);
    background_box(n, n->bg_clip, bx, by, bw, bh, clip);
    bgi_tile(n, area[2], area[3], image_work.decoded.w, image_work.decoded.h, &tw, &th);
    if (tw < 1 || th < 1) return 0;
    bgi_round_space(n, area, &tw, &th, gap, &repeat);
    ox = area[0] + (n->bgi_pos_pct & 1 ? (area[2] - tw) * n->bgi_pos[0] / 100 :
                    n->bgi_pos_pct & 16 ? area[2] - tw - n->bgi_pos[0] : n->bgi_pos[0]);
    oy = area[1] + (n->bgi_pos_pct & 2 ? (area[3] - th) * n->bgi_pos[1] / 100 :
                    n->bgi_pos_pct & 32 ? area[3] - th - n->bgi_pos[1] : n->bgi_pos[1]);
    if (gap[0]) ox = area[0];                                   /* space ignores the position */
    if (gap[1]) oy = area[1];
    x0 = clip[0]; x1 = clip[0] + clip[2]; y0 = clip[1]; y1 = clip[1] + clip[3];
    if (!(repeat & 1)) { if (x0 < ox) x0 = ox; if (x1 > ox + tw) x1 = ox + tw; }
    if (!(repeat & 2)) { if (y0 < oy) y0 = oy; if (y1 > oy + th) y1 = oy + th; }
    for (row = y0; row < y1; row++) {
        for (col = x0; col < x1;) {
            int first = col, r, found = -1;
            uint32_t color, next;
            if (!bgi_pixel(col, row, ox, oy, tw, th, gap, &color)) { col++; continue; }
            for (col++; col < x1 && bgi_pixel(col, row, ox, oy, tw, th, gap, &next) && next == color; col++) {}
            for (r = nr - 1; r >= 0; r--) {
                os_fb_web_item_t *it = &image_work.rect[r];
                if (it->x == first && it->w == col - first && it->color == color && it->y + it->h == row) { found = r; break; }
            }
            if (found >= 0) image_work.rect[found].h++;
            else {
                os_fb_web_item_t *it;
                if (nr >= room) return image_fail(d, "background image budget", idx) + 1;
                it = &image_work.rect[nr++];
                w_zero(it, sizeof(*it));
                it->kind = OS_FB_WEB_RECT;
                it->x = (int16_t)first; it->y = (int16_t)row;
                it->w = (uint16_t)(col - first); it->h = 1;
                it->color = color;
            }
        }
    }
    for (row = (int)d->nitems - 1; row >= at; row--) {
        d->items[row + nr] = d->items[row];
        d->item_owner[row + nr] = d->item_owner[row];
    }
    for (row = 0; row < nr; row++) {
        d->items[at + row] = image_work.rect[row];
        d->item_owner[at + row] = (int16_t)idx;
    }
    d->nitems += (uint32_t)nr;
    return nr;
}

/* Lot 64: an inline box's background images, painted per text run (each
 * run is a fragment box with its own copy of the tiles, as
 * box-decoration-break: clone), over the colour and under the text. */
static int bgi_paint(we_doc_t *d, int idx, int at, int bx, int by, int bw, int bh);
static void inline_image(we_doc_t *d, int idx, int x, int y, int w, int h) {
    we_node_t *n = &d->nodes[idx];
    uint8_t origin = n->bg_origin, clip = n->bg_clip;
    n->bg_origin = n->bg_clip = WE_BOX_BORDER;          /* the fragment has no padding of its own */
    (void)bgi_paint(d, idx, (int)d->nitems, x, y, w, h);
    n->bg_origin = origin; n->bg_clip = clip;
}

/* Lot 64: list-style-image. The marker image is painted at its natural
 * size (scaled down to the 8 px glyph height when taller, keeping its
 * ratio: "list marker image scaled") outside the li, its right edge one
 * glyph before the border box (in rtl its left edge one glyph after it) and its top 1 px below the box's top, as the
 * text marker, through the background image painter (the
 * li's first layer is borrowed for the call). It is refused like any
 * image (path, reader, format, vertical flow, budget); the list-style-type
 * marker is then painted instead, as CSS asks for an image that fails. */
static int marker_image(we_doc_t *d, int idx, int bx, int by, int bw) {
    we_node_t *n = &d->nodes[idx];
    static we_node_t saved;
    int painted = 0, mw, mh;
    uint8_t failed = image_work.failed[idx];
    saved = *n;
    n->bgi_off = n->lsi_off; n->bgi_len = n->lsi_len;
    n->bgi_repeat = 0; n->bgi_size_kind = WE_BGS_AUTO; n->bgi_pos_pct = 0; n->bgi_space = 0;
    n->bgi_pos[0] = n->bgi_pos[1] = 0; n->bg_origin = n->bg_clip = WE_BOX_BORDER;
    image_work.ready_idx = -1;
    image_work.failed[idx] = 0;
    if (!n->visibility && image_open(d, idx) == 0) {
        mw = image_work.decoded.w; mh = image_work.decoded.h;
        if (mh > 8) {
            mw = mw * 8 / mh; mh = 8;
            if (mw < 1) mw = 1;
            n->bgi_size_kind = WE_BGS_EXPLICIT; n->bgi_size[0] = (int16_t)mw; n->bgi_size[1] = (int16_t)mh;
            diag(d, "list marker image scaled", idx);
        }
        painted = bgi_fill(d, idx, (int)d->nitems, n->dir_rtl ? bx + bw + WE_CHAR_W : bx - mw - WE_CHAR_W, by + 1, mw, mh) > 0;
    }
    image_work.ready_idx = -1;
    image_work.failed[idx] = failed;
    n->bgi_off = saved.bgi_off; n->bgi_len = saved.bgi_len; n->bgi_repeat = saved.bgi_repeat;
    n->bgi_size_kind = saved.bgi_size_kind; n->bgi_pos_pct = saved.bgi_pos_pct; n->bgi_space = saved.bgi_space;
    n->bgi_pos[0] = saved.bgi_pos[0]; n->bgi_pos[1] = saved.bgi_pos[1];
    n->bgi_size[0] = saved.bgi_size[0]; n->bgi_size[1] = saved.bgi_size[1];
    n->bg_origin = saved.bg_origin; n->bg_clip = saved.bg_clip;
    return painted;
}

/* Lot 64: the layers from the lowest up, each inserted over the previous */
static int bgi_paint(we_doc_t *d, int idx, int at, int bx, int by, int bw, int bh) {
    we_node_t *n = &d->nodes[idx];
    int k, total = 0, failed = 0;
    if (!n->bgi_layers) return bgi_fill(d, idx, at, bx, by, bw, bh);
    for (k = n->bgi_layers; k >= 0; k--) {
        /* each layer decodes its own url and may fail on its own */
        image_work.ready_idx = -1;
        image_work.failed[idx] = 0;
        if (k) bgi_layer_swap(n, k);
        total += bgi_fill(d, idx, at + total, bx, by, bw, bh);
        if (k) bgi_layer_swap(n, k);
        failed |= image_work.failed[idx];
    }
    image_work.ready_idx = -1;
    image_work.failed[idx] = (uint8_t)failed;
    return total;
}

static void shadow_fill(we_doc_t *d, int idx, int inset, int slot, int slots, int bx, int by, int bw, int bh, const we_radii_t *radii) {
    const we_node_t *n = &d->nodes[idx];
    int k, used = 0;
    int pad[4], border_box[4];
    pad[0] = bx + box_edge(n, 3); pad[1] = by + box_edge(n, 0);
    pad[2] = bw - box_edge(n, 1) - box_edge(n, 3); pad[3] = bh - box_edge(n, 0) - box_edge(n, 2);
    border_box[0] = bx; border_box[1] = by; border_box[2] = bw; border_box[3] = bh;
    for (k = n->shadow_count - 1; k >= 0; k--) {
        const we_shadow_t *sh = &n->shadow[k];
        uint32_t color = sh->has_color ? sh->color : n->color;
        int alpha = color_alpha_of(color), layer, layers = shadow_layers(sh);
        if (!sh->inset != !inset) continue;
        if (radii_any(radii)) {
            int got = shadow_rounded(n, sh) ? shadow_round(d, n, sh, slot + used, slots - used, border_box, radii, color) : -1;
            if (got >= 0) { used += got; continue; }
            radius_square = 1;
        }
        for (layer = 0; layer < layers; layer++) {
            int grow = shadow_grow(sh, layer), a = shadow_alpha(sh, layer, alpha);
            int shape[4], box[4], pieces[4][4], count, j;
            if (inset) {
                shape[0] = pad[0] + sh->x + grow; shape[1] = pad[1] + sh->y + grow;
                shape[2] = pad[2] - 2 * grow; shape[3] = pad[3] - 2 * grow;
                count = rect_minus(pad, shape, pieces);
            } else {
                shape[0] = bx + sh->x - grow; shape[1] = by + sh->y - grow;
                shape[2] = bw + 2 * grow; shape[3] = bh + 2 * grow;
                box[0] = bx; box[1] = by; box[2] = bw; box[3] = bh;
                count = rect_minus(shape, box, pieces);
            }
            for (j = 0; j < count && used < slots; j++, used++) {
                os_fb_web_item_t *it = &d->items[slot + used];
                it->kind = OS_FB_WEB_RECT;
                it->x = (int16_t)limit_coord(pieces[j][0]); it->y = (int16_t)limit_coord(pieces[j][1]);
                it->w = (uint16_t)pieces[j][2]; it->h = (uint16_t)pieces[j][3];
                it->color = with_alpha(color, a);
            }
        }
    }
}

static int inside(const we_doc_t *d, int node, int root);

/* Lot 64: direction rtl in a flex or grid container. The items are laid
 * out left to right and then mirrored inside the content box with their
 * subtrees, so the main start of a row (the cross start of a column, the
 * first grid column) is the right edge. Text inside an item keeps its
 * left-to-right order. */
static void rtl_mirror(we_doc_t *d, int idx, int first, int x, int width) {
    int c, i;
    for (c = d->nodes[idx].first; c >= 0; c = d->nodes[c].next) {
        const we_node_t *k = &d->nodes[c];
        int delta;
        if (k->absolute || k->display == WE_DISP_NONE) continue;
        delta = 2 * x + width - 2 * k->x - k->w;
        if (!delta) continue;
        for (i = c; i < (int)d->nnodes; i++)
            if (inside(d, i, c)) d->nodes[i].x = (int16_t)limit_coord(d->nodes[i].x + delta);
        for (i = first; i < (int)d->nitems; i++)
            if (inside(d, d->item_owner[i], c)) d->items[i].x = (int16_t)limit_coord(d->items[i].x + delta);
    }
}

/* ---------- Lot 64: multi-column layout (subset) ----------
 * A block with column-count or column-width lays its content out at the
 * column width (count from column-width as CSS: as many as fit, capped by
 * column-count and WE_COLUMNS_MAX; the gap is column-gap, normal 8 px),
 * then cuts it into units: each in-flow child box whole (no fragmentation
 * inside a box: break-inside avoid), and each line of the inline content.
 * The units are packed in order into columns of height h: the smallest h
 * that needs no more than the count (balance), or the definite height with
 * column-fill auto (and as a cap when balancing). Units that do not fit go
 * to further columns on the right (overflow columns). Each unit moves with
 * its subtree and items to its column. column-rule paints in the middle of
 * each gap between two used columns, over the column height, after the
 * content. Vertical writing modes are refused. */
typedef struct { int node, top, bottom, line, span, brk; } we_col_unit_t;
static we_col_unit_t col_unit[OS_FB_WEB_ITEMS + WE_MAX_NODES];
static int16_t col_of[OS_FB_WEB_ITEMS + WE_MAX_NODES];
static uint8_t col_moved[OS_FB_WEB_ITEMS];

static int multicol(const we_node_t *n) {
    return (n->col_count || n->col_width > 0) && n->display != WE_DISP_FLEX && n->display != WE_DISP_GRID &&
           n->display != WE_DISP_TABLE && n->wmode == WE_WM_HORIZONTAL;
}

static int column_plan(const we_node_t *n, int width, int *gap, int *cw) {
    int count, g = n->col_gap >= 0 ? n->col_gap : WE_CHAR_W;
    if (n->col_width > 0) {
        count = (width + g) / (n->col_width + g);
        if (count < 1) count = 1;
        if (n->col_count && n->col_count < count) count = n->col_count;
    } else count = n->col_count;
    if (count > WE_COLUMNS_MAX) count = WE_COLUMNS_MAX;
    while (count > 1 && (width - (count - 1) * g) / count < WE_CHAR_W) count--;
    if (count < 1) count = 1;
    *gap = g;
    *cw = (width - (count - 1) * g) / count;
    if (*cw < WE_CHAR_W) *cw = WE_CHAR_W;
    return count;
}

/* the units in order; returns their count */
static int column_units(we_doc_t *d, int idx, int first) {
    int c, count = 0, line_h = d->nodes[idx].line_height > 0 ? d->nodes[idx].line_height : WE_LINE_H;
    uint32_t i;
    for (c = d->nodes[idx].first; c >= 0; c = d->nodes[c].next) {
        const we_node_t *k = &d->nodes[c];
        if (k->display == WE_DISP_NONE || k->absolute) continue;
        if (k->is_text || k->display == WE_DISP_INLINE || k->display == WE_DISP_INLINE_BLOCK) {
            int open = -1;
            for (i = (uint32_t)first; i < d->nitems; i++) {
                const os_fb_web_item_t *it = &d->items[i];
                int top, bottom;
                if (!it->kind || !inside(d, d->item_owner[i], c)) continue;
                top = it->kind == OS_FB_WEB_TEXT ? it->y - 1 : it->y;
                bottom = it->kind == OS_FB_WEB_TEXT ? top + line_h : it->y + it->h;
                if (open >= 0 && top < col_unit[open].bottom) {
                    if (top < col_unit[open].top) col_unit[open].top = top;
                    if (bottom > col_unit[open].bottom) col_unit[open].bottom = bottom;
                    continue;
                }
                open = count++;
                col_unit[open].node = c; col_unit[open].line = 1; col_unit[open].span = col_unit[open].brk = 0;
                col_unit[open].top = top; col_unit[open].bottom = bottom;
            }
        } else {
            col_unit[count].node = c; col_unit[count].line = 0;
            col_unit[count].span = k->col_span_all && k->display != WE_DISP_INLINE_BLOCK;
            col_unit[count].brk = k->col_break;
            col_unit[count].top = k->y - k->margin[0];
            col_unit[count].bottom = k->y + k->h + k->margin[2];
            count++;
        }
    }
    return count;
}

/* columns used by packing units [from, to) at height h (col_of filled,
 * relative to the segment); a forced break opens a column */
static int column_pack(int from, int to, int h) {
    int u, column = 0, start = 0;
    for (u = from; u < to; u++) {
        if (u == from) start = col_unit[u].top;
        else if (col_unit[u].bottom - start > h || (col_unit[u].brk & 1) || (col_unit[u - 1].brk & 2)) {
            column++;
            start = col_unit[u].top;
        }
        col_of[u] = (int16_t)column;
    }
    return to > from ? column + 1 : 0;
}

static void column_shift(we_doc_t *d, int first, const we_col_unit_t *u, int dx, int dy) {
    uint32_t i;
    int k;
    for (i = (uint32_t)first; i < d->nitems; i++) {
        os_fb_web_item_t *it = &d->items[i];
        int top;
        if (!it->kind || col_moved[i] || !inside(d, d->item_owner[i], u->node)) continue;
        top = it->kind == OS_FB_WEB_TEXT ? it->y - 1 : it->y;
        if (u->line && (top < u->top || top >= u->bottom)) continue;
        col_moved[i] = 1;
        it->x = (int16_t)limit_coord(it->x + dx);
        it->y = (int16_t)limit_coord(it->y + dy);
    }
    if (u->line) return;
    for (k = u->node; k < (int)d->nnodes; k++)
        if (inside(d, k, u->node)) {
            d->nodes[k].x = (int16_t)limit_coord(d->nodes[k].x + dx);
            d->nodes[k].y = (int16_t)limit_coord(d->nodes[k].y + dy);
        }
}

static int width_auto(const we_node_t *n);

/* Lot 64: one segment of units between spanners, its first column at
 * top; returns its height */
static int column_segment(we_doc_t *d, int idx, int first, int from, int to, int x, int top, int cw, int gap, int count,
                          int fixed) {
    const we_node_t *n = &d->nodes[idx];
    int lo = 1, hi = col_unit[to - 1].bottom - col_unit[from].top, h, used, u, k, start[WE_COLUMNS_MAX * 4];
    for (u = from; u < to; u++) if (col_unit[u].bottom - col_unit[u].top > lo) lo = col_unit[u].bottom - col_unit[u].top;
    if (hi < lo) hi = lo;
    if (n->col_fill_auto && fixed > 0) h = fixed;
    else {
        while (lo < hi) {                      /* the smallest height using at most count columns */
            int mid = (lo + hi) / 2;
            if (column_pack(from, to, mid) <= count) hi = mid;
            else lo = mid + 1;
        }
        h = lo;
        if (fixed > 0 && h > fixed) h = fixed;
    }
    used = column_pack(from, to, h);
    if (used > WE_COLUMNS_MAX * 4) { diag(d, "columns overflow dropped", idx); used = WE_COLUMNS_MAX * 4; }
    if (used > count) diag(d, "columns overflow", idx);
    for (k = 0; k < used; k++) start[k] = -1;
    for (u = from; u < to; u++) if (col_of[u] < used && start[col_of[u]] < 0) start[col_of[u]] = col_unit[u].top;
    for (u = from; u < to; u++) {
        int column = col_of[u] < used ? col_of[u] : used - 1;
        column_shift(d, first, &col_unit[u], column * (cw + gap), top - start[column]);
    }
    if (n->rule_style > WE_BS_HIDDEN && n->rule_w > 0)
        for (k = 1; k < used && k < count; k++) {
            int rx = x + k * (cw + gap) - gap / 2 - n->rule_w / 2;
            edge_paint(d, idx, n->rule_style, n->rule_color_set ? n->rule_color : n->color, 1, rx, top, n->rule_w, h);
        }
    return h;
}

/* A column-span: all block child (a spanner) is laid out at the full
 * width (an auto width; a percentage resolves against the column) and
 * cuts the content into segments: each segment is balanced on its own,
 * the spanner goes below it and the next segment below the spanner. The
 * fill and the definite height apply only without spanners. */
static int layout_columns(we_doc_t *d, int idx, int x, int y, int width, int depth) {
    const we_node_t *n = &d->nodes[idx];
    int gap, cw, count = column_plan(n, width, &gap, &cw), first = (int)d->nitems, units, u, from = 0, top = y, spans = 0, c;
    int fixed = definite_height(d, idx, 0);
    for (c = n->first; c >= 0; c = d->nodes[c].next)
        if (d->nodes[c].col_span_all && d->nodes[c].display != WE_DISP_INLINE && d->nodes[c].display != WE_DISP_INLINE_BLOCK &&
            !d->nodes[c].is_text && !d->nodes[c].absolute && d->nodes[c].display != WE_DISP_NONE) {
            spans++;
            if (width_auto(&d->nodes[c])) forced_width[c] = (int16_t)width;
            else if (d->nodes[c].width_pct > 0) diag(d, "column span width on the column", c);
        }
    (void)layout_children(d, idx, x, y, cw, depth);
    for (c = n->first; c >= 0; c = d->nodes[c].next) if (d->nodes[c].col_span_all && width_auto(&d->nodes[c])) forced_width[c] = -1;
    units = column_units(d, idx, first);
    if (!units) return fixed > 0 ? fixed : 0;
    if (spans) fixed = -1;
    w_zero(col_moved, sizeof(col_moved));
    for (u = 0; u <= units; u++) {
        if (u < units && !col_unit[u].span) continue;
        if (u > from) top += column_segment(d, idx, first, from, u, x, top, cw, gap, count, fixed);
        if (u < units) {
            column_shift(d, first, &col_unit[u], 0, top - col_unit[u].top);
            top += col_unit[u].bottom - col_unit[u].top;
        }
        from = u + 1;
    }
    return top - y;
}

static int layout_content(we_doc_t *d, int idx, int x, int y, int width, int depth) {
    const we_node_t *n = &d->nodes[idx];
    if (multicol(n)) return layout_columns(d, idx, x, y, width, depth);
    if (n->display == WE_DISP_FLEX || n->display == WE_DISP_GRID) {
        int first = (int)d->nitems, h = layout_flex(d, idx, x, y, width, depth);
        if (n->dir_rtl) rtl_mirror(d, idx, first, x, width);
        return h;
    }
    if (n->display == WE_DISP_TABLE) {
        int first = (int)d->nitems, h = layout_table(d, idx, x, y, width, depth), r;
        /* rtl: the first column at the right; each row's cells are mirrored */
        if (n->dir_rtl)
            for (r = idx + 1; r < (int)d->nnodes && inside(d, r, idx); r++)
                if (d->nodes[r].display == WE_DISP_ROW) {
                    int t = d->nodes[r].parent;
                    while (t >= 0 && d->nodes[t].display != WE_DISP_TABLE) t = d->nodes[t].parent;
                    if (t == idx) rtl_mirror(d, r, first, x, width);      /* not a nested table's rows */
                }
        return h;
    }
    return layout_children(d, idx, x, y, width, depth);
}

/* ---------- Lot 64: vertical writing modes ----------
 * A block whose writing-mode is vertical-rl or vertical-lr under a
 * horizontal container is an orthogonal root: its content is laid out in
 * logical space (inline axis = physical down, inline size = its definite
 * content height, else its max-content) and then mapped to physical space:
 * vertical-rl puts the first line at the right, vertical-lr at the left.
 * The box keeps its physical margins, borders and padding; an auto width
 * becomes the block extent of its content. Text is stacked upright, one
 * glyph per item and 8 px per glyph (no rotated font: mixed orientation is
 * painted upright, "vertical text painted upright"); spaces paint nothing.
 * Descendants share the root's mode: another writing-mode inside it is
 * painted with the root's ("nested writing mode inherited"). */
static os_fb_web_item_t vertical_out[OS_FB_WEB_ITEMS];
static int16_t vertical_owner[OS_FB_WEB_ITEMS];

static int vertical_root(const we_doc_t *d, int idx) {
    const we_node_t *n = &d->nodes[idx];
    return n->wmode != WE_WM_HORIZONTAL && (n->parent < 0 || d->nodes[n->parent].wmode == WE_WM_HORIZONTAL);
}

static int width_auto(const we_node_t *n) {
    return !n->width_kind && n->width < 0 && n->width_pct <= 0 && !n->length[WE_LEN_WIDTH].count;
}

static int inside(const we_doc_t *d, int node, int root) {
    int k;
    for (k = 0; node >= 0 && k <= WE_DEPTH_MAX; k++, node = d->nodes[node].parent) if (node == root) return 1;
    return 0;
}

static void vertical_map(int rl, int ox, int oy, int extent, int *x, int *y, int *w, int *h) {
    int lx = *x - ox, ly = *y - oy, lw = *w, lh = *h;
    *x = rl ? ox + extent - ly - lh : ox + ly;
    *y = oy + lx;
    *w = lh; *h = lw;
}

static void vertical_remap(we_doc_t *d, int idx, int first, int ox, int oy, int extent) {
    int rl = d->nodes[idx].wmode == WE_WM_VERTICAL_RL, i, out = 0, room = OS_FB_WEB_ITEMS - first, dropped = 0, text = 0;
    for (i = 0; i < (int)d->nnodes; i++) {
        we_node_t *c = &d->nodes[i];
        int x = c->x, y = c->y, w = c->w, h = c->h;
        if (i == idx || !inside(d, i, idx)) continue;
        vertical_map(rl, ox, oy, extent, &x, &y, &w, &h);
        c->x = (int16_t)limit_coord(x); c->y = (int16_t)limit_coord(y); c->w = (int16_t)w; c->h = (int16_t)h;
    }
    for (i = first; i < (int)d->nitems; i++) {
        os_fb_web_item_t it = d->items[i];
        int x = it.x, y = it.y, w = it.w, h = it.h;
        if (it.kind == OS_FB_WEB_TEXT) {
            int len = 0, adv, k;
            while (len < OS_FB_WEB_TEXT_LEN && it.text[len]) len++;
            adv = len ? w / len : WE_CHAR_W;
            if (adv < 1) adv = 1;
            for (k = 0; k < len; k++) {
                os_fb_web_item_t glyph = it;
                int gx = x + k * adv, gy = y, gw = WE_CHAR_W, gh = h;
                if (it.text[k] == ' ') continue;
                if (out >= room) { dropped++; continue; }
                vertical_map(rl, ox, oy, extent, &gx, &gy, &gw, &gh);
                glyph.x = (int16_t)limit_coord(gx); glyph.y = (int16_t)limit_coord(gy);
                glyph.w = (uint16_t)gw; glyph.h = (uint16_t)gh;
                w_zero(glyph.text, sizeof(glyph.text));
                glyph.text[0] = it.text[k];
                vertical_out[out] = glyph;
                vertical_owner[out++] = d->item_owner[i];
                text = 1;
            }
            continue;
        }
        if (out >= room) { dropped++; continue; }
        if (it.kind) {
            vertical_map(rl, ox, oy, extent, &x, &y, &w, &h);
            it.x = (int16_t)limit_coord(x); it.y = (int16_t)limit_coord(y); it.w = (uint16_t)w; it.h = (uint16_t)h;
        }
        vertical_out[out] = it;
        vertical_owner[out++] = d->item_owner[i];
    }
    for (i = 0; i < out; i++) { d->items[first + i] = vertical_out[i]; d->item_owner[first + i] = vertical_owner[i]; }
    d->nitems = (uint32_t)(first + out);
    if (dropped) {
        if (!d->perf.items_dropped) diag(d, "display budget exceeded", idx);
        d->perf.items_dropped += (uint32_t)dropped;
    }
    if (text && !d->nodes[idx].upright) diag(d, "vertical text painted upright", idx);
}

/* Physical properties of the boxes inside an orthogonal root are turned
 * into logical ones for the layout and turned back afterwards (the later
 * passes read physical values): sides and corners rotate (logical top is
 * the physical right in vertical-rl, the left in vertical-lr; logical left
 * is the physical top), px widths and heights swap, radii, shadow and
 * gradient axes swap. Percent or keyword sizes stay as declared ("vertical
 * flow size kept physical"); background images inside are refused, images
 * paint their alt text (the runs would be transposed). Relative offsets
 * and absolute insets of boxes inside follow the logical axes. */
static const uint8_t vertical_side[2][4] = {{3, 2, 1, 0}, {1, 2, 3, 0}};      /* [rl][logical] */
static const uint8_t vertical_corner[2][4] = {{0, 3, 2, 1}, {1, 2, 3, 0}};

static void permute16(int16_t *a, const uint8_t *map, int forward) {
    int16_t t[4];
    int k;
    for (k = 0; k < 4; k++) t[k] = a[k];
    for (k = 0; k < 4; k++) { if (forward) a[k] = t[map[k]]; else a[map[k]] = t[k]; }
}

static uint8_t permute_bits(uint8_t bits, const uint8_t *map, const uint8_t *weight, int forward) {
    uint8_t out = (uint8_t)(bits & ~(weight[0] | weight[1] | weight[2] | weight[3]));
    int k;
    for (k = 0; k < 4; k++) {
        if (forward) { if (bits & weight[map[k]]) out |= weight[k]; }
        else if (bits & weight[k]) out |= weight[map[k]];
    }
    return out;
}

static int simple_size(const we_node_t *n) {
    return !n->width_kind && n->width_pct <= 0 && !n->length[WE_LEN_WIDTH].count && !n->height_percent &&
           !n->length[WE_LEN_HEIGHT].count && !n->min_width_kind && !n->max_width_kind && !n->min_pct && !n->max_pct &&
           !n->min_height_percent && !n->max_height_percent && !n->length[WE_LEN_MIN_W].count &&
           !n->length[WE_LEN_MAX_W].count && !n->length[WE_LEN_MIN_H].count && !n->length[WE_LEN_MAX_H].count;
}

static void swap16(int16_t *a, int16_t *b) { int16_t t = *a; *a = *b; *b = t; }

static void vertical_style(we_doc_t *d, int i, int rl, int forward) {
    static const uint8_t side_bit[4] = {WE_AUTO_TOP, WE_AUTO_RIGHT, WE_AUTO_BOTTOM, WE_AUTO_LEFT};
    static const uint8_t margin_pct[4] = {1, 2, 4, 8}, padding_pct[4] = {16, 32, 64, 128};
    static const uint8_t rad_x_bit[4] = {1, 2, 4, 8}, rad_y_bit[4] = {16, 32, 64, 128};
    static const uint8_t grad_map[2][6] = {{0, WE_GRAD_RIGHT, WE_GRAD_BOTTOM, WE_GRAD_LEFT, WE_GRAD_TOP, WE_GRAD_RADIAL},
                                          {0, WE_GRAD_RIGHT, WE_GRAD_TOP, WE_GRAD_LEFT, WE_GRAD_BOTTOM, WE_GRAD_RADIAL}};
    we_node_t *n = &d->nodes[i];
    const uint8_t *side = vertical_side[rl], *corner = vertical_corner[rl];
    int k;
    uint8_t x_bits, y_bits;
    permute16(n->margin, side, forward);
    permute16(n->padding, side, forward);
    permute16(n->spacing_value, side, forward);
    permute16(n->spacing_value + 4, side, forward);
    n->spacing_pct = permute_bits(n->spacing_pct, side, margin_pct, forward);
    n->spacing_pct = permute_bits(n->spacing_pct, side, padding_pct, forward);
    n->margin_auto = permute_bits(n->margin_auto, side, side_bit, forward);
    {   /* per-side borders rotate with the sides */
        static const uint8_t edge_bit[4] = {1, 2, 4, 8};
        int8_t w[4];
        uint8_t st[4];
        uint32_t col[4];
        for (k = 0; k < 4; k++) { w[k] = n->edge_w[k]; st[k] = n->edge_style[k]; col[k] = n->edge_color[k]; }
        for (k = 0; k < 4; k++) {
            int from = forward ? side[k] : k, to = forward ? k : side[k];
            n->edge_w[to] = w[from]; n->edge_style[to] = st[from]; n->edge_color[to] = col[from];
        }
        n->edge_color_set = permute_bits(n->edge_color_set, side, edge_bit, forward);
    }
    /* radii: corners rotate and the two radii of a corner swap axes */
    {
        uint8_t rx[4], ry[4];
        for (k = 0; k < 4; k++) { rx[k] = n->rad_x[k]; ry[k] = n->rad_y[k]; }
        for (k = 0; k < 4; k++) {
            if (forward) { n->rad_x[k] = ry[corner[k]]; n->rad_y[k] = rx[corner[k]]; }
            else { n->rad_y[corner[k]] = rx[k]; n->rad_x[corner[k]] = ry[k]; }
        }
        x_bits = permute_bits(n->rad_pct, corner, rad_x_bit, forward) & 15;
        y_bits = permute_bits(n->rad_pct, corner, rad_y_bit, forward) & 240;
        n->rad_pct = (uint8_t)((x_bits << 4) | (y_bits >> 4));
    }
    for (k = 0; k < n->shadow_count && k < WE_SHADOWS_MAX; k++) {
        int8_t x = n->shadow[k].x, y = n->shadow[k].y;
        if (forward) { n->shadow[k].x = y; n->shadow[k].y = (int8_t)(rl ? -x : x); }
        else { n->shadow[k].y = x; n->shadow[k].x = (int8_t)(rl ? -y : y); }
    }
    for (k = 0; k < n->tshadow_count && k < WE_SHADOWS_MAX; k++) {
        int8_t x = n->tshadow[k].x, y = n->tshadow[k].y;
        if (forward) { n->tshadow[k].x = y; n->tshadow[k].y = (int8_t)(rl ? -x : x); }
        else { n->tshadow[k].y = x; n->tshadow[k].x = (int8_t)(rl ? -y : y); }
    }
    if (n->grad_dir && n->grad_dir <= WE_GRAD_RADIAL) {
        if (forward) n->grad_dir = grad_map[rl][n->grad_dir];
        else for (k = 1; k <= WE_GRAD_RADIAL; k++) if (grad_map[rl][k] == n->grad_dir) { n->grad_dir = (uint8_t)k; break; }
    }
    if (n->grad_dir == WE_GRAD_RADIAL) {
        swap16(&n->grad_at[0], &n->grad_at[1]);
        swap16(&n->grad_size[0], &n->grad_size[1]);
        n->grad_at_pct = (uint8_t)((n->grad_at_pct & ~15) | ((n->grad_at_pct & 1) << 1) | ((n->grad_at_pct & 2) >> 1) |
                                   ((n->grad_at_pct & 4) << 1) | ((n->grad_at_pct & 8) >> 1));
    }
    swap16(&n->border_spacing, &n->border_spacing_y);
    swap16(&n->aspect_num, &n->aspect_den);
    if (simple_size(n)) {
        swap16(&n->width, &n->height);
        swap16(&n->min_width, &n->min_height);
        swap16(&n->max_width, &n->max_height);
    } else if (forward) diag(d, "vertical flow size kept physical", i);
    if (forward && (n->bgi_len || n->bgi_layers)) { n->bgi_len = 0; n->bgi_layers = 0; diag(d, "background image refused", i); }
}

/* forward 1 before the layout, 0 after it. */
static void vertical_styles(we_doc_t *d, int forward) {
    int i;
    for (i = 0; i < (int)d->nnodes; i++) {
        int root = d->nodes[i].parent;
        while (root >= 0 && !vertical_root(d, root)) root = d->nodes[root].parent;
        if (root < 0 || d->nodes[i].is_text) continue;
        if (forward && d->nodes[i].wmode != d->nodes[root].wmode) {
            d->nodes[i].wmode = d->nodes[root].wmode;
            diag(d, "nested writing mode inherited", i);
        }
        vertical_style(d, i, d->nodes[root].wmode == WE_WM_VERTICAL_RL, forward);
    }
}

static int layout_vertical(we_doc_t *d, int idx, int ox, int oy, int width, int *extent, int depth) {
    we_node_t *n = &d->nodes[idx];
    int size = resolved_height[idx] >= 0 ? resolved_height[idx] : definite_height(d, idx, 0), first = (int)d->nitems, e;
    (void)n;
    if (size < 0) size = vertical_inline[idx];      /* auto: max-content along the inline axis */
    if (size < WE_CHAR_W) size = WE_CHAR_W;
    if (size > WE_LAYOUT_MAX) size = WE_LAYOUT_MAX;
    {
        int saved_now = vertical_now, saved_block = vertical_block, saved_resolved = resolved_height[idx];
        vertical_now = idx; vertical_block = width;     /* the logical height: the physical width or indefinite */
        resolved_height[idx] = (int16_t)width;
        e = layout_content(d, idx, ox, oy, size, depth);
        vertical_now = saved_now; vertical_block = saved_block; resolved_height[idx] = (int16_t)saved_resolved;
    }
    *extent = width >= 0 ? width : e;
    vertical_remap(d, idx, first, ox, oy, *extent);
    return size;
}

static int layout_block(we_doc_t* d, int idx, int x, int y, int cw, int depth) {
    we_node_t* n = &d->nodes[idx];
    int bx, by, bw, bh, content_x, content_w, content_h, slot, border_slot = -1;
    int first_item = (int)d->nitems, shadow_slot[2], shadow_slots[2], grad_slot, grad_slots, radius_slots, bgi_at;
    we_radii_t radii;
    if (depth > WE_DEPTH_MAX) return 0;
    d->perf.boxes++;
    layout_first[idx] = layout_last[idx] = WE_NO_BASELINE;
    bx = x + n->margin[3];
    by = y + n->margin[0];
    if (forced_width[idx] >= 0) bw = forced_width[idx] - n->margin[1] - n->margin[3];
    else bw = specified_width(n, cw, cw - n->margin[1] - n->margin[3] - node_border_extra(n)) + node_border_extra(n);
    resolved_height[idx] = (int16_t)definite_height(d, idx, 0);
    if ((n->height_percent || n->min_height_percent || n->max_height_percent) && containing_height[idx] < 0 && definite_height(d, n->parent, 0) < 0)
        diag(d, "percent height indefinite", idx);
    if (bw < 0) bw = 0;
    if (bw > WE_LAYOUT_MAX) bw = WE_LAYOUT_MAX;
    if (forced_width[idx] < 0) bx += horizontal_auto(n, cw, bw, n->parent >= 0 && d->nodes[n->parent].dir_rtl &&
                                                   d->nodes[n->parent].display != WE_DISP_FLEX && d->nodes[n->parent].display != WE_DISP_GRID);
    bx = limit_coord(bx);
    by = limit_coord(by);
    n->x = (int16_t)bx; n->y = (int16_t)by; n->w = (int16_t)bw;
    /* reserve the background item now so it paints under the children;
     * lot 62: outer shadows go under it, inset shadows over it */
    shadow_slot[0] = shadow_reserve(d, idx, 0, &shadow_slots[0]);
    slot = n->has_bg ? (int)d->nitems : -1;
    if (n->has_bg && !emit(d, idx, OS_FB_WEB_RECT, 0, 0, 0, 0, n->bg)) slot = -1;
    /* lot 62: a rounded background takes up to WE_RADIUS_SLICES slots */
    radius_slots = 0;
    if (slot >= 0 && (n->rad_x[0] | n->rad_x[1] | n->rad_x[2] | n->rad_x[3])) {
        for (radius_slots = 1; radius_slots < WE_RADIUS_SLICES; radius_slots++) {
            os_fb_web_item_t *it = emit(d, idx, OS_FB_WEB_RECT, 0, 0, 0, 0, 0);
            if (!it) break;
            it->kind = 0;
            it->pad = WE_ITEM_RESERVED;
        }
    }
    grad_slot = gradient_reserve(d, idx, &grad_slots);
    bgi_at = (int)d->nitems;
    shadow_slot[1] = shadow_reserve(d, idx, 1, &shadow_slots[1]);
    content_x = limit_coord(bx + box_edge(n, 3) + n->padding[3]);
    content_w = bw - box_edge(n, 1) - box_edge(n, 3) - n->padding[1] - n->padding[3];
    if (content_w < WE_CHAR_W) content_w = WE_CHAR_W;
    spacing_resolve(d, idx, content_w, 0);
    {
        int c;
        for (c = n->first; c >= 0; c = d->nodes[c].next)
            if (d->nodes[c].absolute) set_static(c, content_x, limit_coord(by + box_edge(n, 0) + n->padding[0]));
    }
    if (w_eq(n->tag, "img")) {
        int image_h = image_paint(d, idx, content_x, by + box_edge(n, 0) + n->padding[0], content_w,
            modern_image(n) && forced_width[idx] >= 0 ? content_w : -1, modern_image(n) ? resolved_height[idx] : -1);
        if (image_h >= 0) {
            content_h = image_h;
            bw = n->w + node_border_extra(n);
        } else {
            os_fb_web_item_t *alt = emit(d, idx, OS_FB_WEB_TEXT, content_x,
                by + box_edge(n, 0) + n->padding[0] + 1, n->text_len * WE_CHAR_W, 8, n->color);
            if (alt) w_cpy(alt->text, OS_FB_WEB_TEXT_LEN, d->pool + n->text_off, n->text_len);
            content_h = WE_LINE_H;
        }
    } else if (vertical_root(d, idx)) {
        int extent, automatic = forced_width[idx] < 0 && width_auto(n);
        content_h = layout_vertical(d, idx, content_x, by + box_edge(n, 0) + n->padding[0], automatic ? -1 : content_w, &extent, depth);
        if (automatic) bw = limit_coord(extent + box_edge(n, 1) + box_edge(n, 3) + n->padding[1] + n->padding[3]);
    } else
        content_h = layout_content(d, idx, content_x, by + box_edge(n, 0) + n->padding[0], content_w, depth);
    if (resolved_height[idx] >= 0) content_h = resolved_height[idx];
    else if (n->aspect_num && !w_eq(n->tag, "img")) {
        int ratio = ratio_height(n, bw - node_border_extra(n));
        if (ratio > content_h) content_h = ratio;
    }
    if (forced_height[idx] < 0) content_h = height_constraint(d, idx, content_h, containing_height[idx] >= 0 ? containing_height[idx] : definite_height(d, n->parent, 0));
    bh = limit_coord(content_h + vertical_extra(n));
    if (bh < 0) bh = 0;
    n->x = (int16_t)bx; n->y = (int16_t)by; n->w = (int16_t)bw; n->h = (int16_t)bh;
    used_radii(n, bw, bh, &radii);
    if (radii_any(&radii)) {
        radius_stepped = 1;
        if (n->grad_dir) diag(d, "gradient radius painted square", idx);
    }
    if (slot >= 0) {
        os_fb_web_item_t* it = &d->items[slot];
        int box[4];
        background_box(n, n->bg_clip, bx, by, bw, bh, box);
        if (radius_slots && radii_any(&radii)) {
            we_radii_t clip;
            radii_inset(&radii, &clip, box[1] - by, bx + bw - box[0] - box[2], by + bh - box[1] - box[3], box[0] - bx);
            if (!radius_fill(d, slot, radius_slots, &clip, box[0], box[1], box[2], box[3], n->bg)) { it->kind = 0; it->pad = WE_ITEM_RESERVED; }
        } else { it->x = (int16_t)box[0]; it->y = (int16_t)box[1]; it->w = (uint16_t)box[2]; it->h = (uint16_t)box[3]; }
    }
    gradient_fill(d, idx, grad_slot, grad_slots, bx, by, bw, bh);
    shadow_slot[1] += bgi_paint(d, idx, bgi_at, bx, by, bw, bh);
    shadow_fill(d, idx, 0, shadow_slot[0], shadow_slots[0], bx, by, bw, bh, &radii);
    shadow_fill(d, idx, 1, shadow_slot[1], shadow_slots[1], bx, by, bw, bh, &radii);
    if (n->own_edges) {
        /* lot 64: per-side borders: the top and bottom across the box,
         * the left and right between them, each with its style and colour */
        int w[4], style[4], side;
        uint32_t color[4];
        for (side = 0; side < 4; side++) {
            edge_of(n, side, &w[side], &style[side], &color[side]);
            if (style[side] == WE_BS_HIDDEN) w[side] = 0;
        }
        if (radii_any(&radii)) diag(d, "edges radius painted square", idx);
        if (w[0]) edge_paint(d, idx, style[0], color[0], 0, by, bx, w[0], bw);
        if (w[2]) edge_paint(d, idx, style[2], color[2], 0, by + bh - w[2], bx, w[2], bw);
        if (w[3]) edge_paint(d, idx, style[3], color[3], 1, bx, by + w[0], w[3], bh - w[0] - w[2]);
        if (w[1]) edge_paint(d, idx, style[1], color[1], 1, bx + bw - w[1], by + w[0], w[1], bh - w[0] - w[2]);
        border_slot = 1;
    } else if (n->border > 0 && !n->collapse_cell && radii_any(&radii) && (n->border_style == 5 || n->border_style == 6)) {
        radius_dashed(d, idx, &radii, bx, by, bw, bh, n->border > 8 ? 8 : n->border, n->border_style, n->border_color);
        border_slot = 1;
    } else if (n->border > 0 && !n->collapse_cell && radii_any(&radii) &&
        (n->border_style == 0 || n->border_style == WE_BS_SOLID || n->border_style == 4 || (n->border_style >= 7 && n->border_style <= 10))) {
        radius_border(d, idx, &radii, bx, by, bw, bh, n->border > 8 ? 8 : n->border, n->border_style, n->border_color);
        border_slot = 1;
    } else if (n->border > 0 && !n->collapse_cell && n->border_style >= 7 && n->border_style <= 10) {
        frame_3d(d, idx, n->border_style, n->border_color, bx, by, bw, bh, n->border > 8 ? 8 : n->border);
        border_slot = 1;
    } else if (n->border > 0 && !n->collapse_cell && (n->border_style == 5 || n->border_style == 6 || (n->border_style == 4 && n->border >= 3))) {
        int t = n->border > 8 ? 8 : n->border, inner_h = bh - 2 * t;
        edge_paint(d, idx, n->border_style, n->border_color, 0, by, bx, t, bw);
        edge_paint(d, idx, n->border_style, n->border_color, 0, by + bh - t, bx, t, bw);
        edge_paint(d, idx, n->border_style, n->border_color, 1, bx, by + t, t, inner_h);
        edge_paint(d, idx, n->border_style, n->border_color, 1, bx + bw - t, by + t, t, inner_h);
        border_slot = 1;
    } else if (n->border > 0 && !n->collapse_cell) {
        os_fb_web_item_t* it = emit(d, idx, OS_FB_WEB_BORDER, bx, by, bw, bh, n->border_color);
        if (it) it->bold = (uint8_t)(n->border > 8 ? 8 : n->border);
        border_slot = 1;
    }
    (void)border_slot;
    if (w_eq(n->tag, "img") || (n->overflow_hidden && layout_first[idx] != WE_NO_BASELINE))
        layout_first[idx] = layout_last[idx] = (int16_t)limit_coord(by + bh);
    if (w_eq(n->tag, "li") && n->lsi_len && marker_image(d, idx, bx, by, bw)) {
        /* lot 64: list-style-image painted, the type marker not */
    } else if (w_eq(n->tag, "li") && n->list_style != WE_LIST_NONE) {
        char marker[24];
        int number = 1, sibling, len;
        os_fb_web_item_t *it;
        (void)sibling;
        number = list_ordinal(d, idx);
        len = counter_text(n->list_style, number, marker);
        if (n->list_style != WE_LIST_DISC && n->list_style != WE_LIST_SQUARE && n->list_style != WE_LIST_CIRCLE) {
            /* lot 64: in rtl the marker is right of the box and its full
             * stop, a neutral after the number, reads first: ".1" */
            if (n->dir_rtl) { int k; for (k = len; k > 0; k--) marker[k] = marker[k - 1]; marker[0] = '.'; }
            else marker[len] = '.';
            marker[++len] = 0;
        }
        it = emit(d, idx, OS_FB_WEB_TEXT, n->dir_rtl ? bx + bw + WE_CHAR_W : bx - (len + 1) * WE_CHAR_W, by + 1, len * WE_CHAR_W, 8, n->color);
        if (it) w_cpy(it->text, OS_FB_WEB_TEXT_LEN, marker, len);
    }
    place_absolutes(d, idx, bx + box_edge(n, 3), by + box_edge(n, 0), bw - box_edge(n, 1) - box_edge(n, 3),
                    bh - box_edge(n, 0) - box_edge(n, 2), depth);
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

/* An inline-block is atomic on the current line. Text before it is closed
 * as a left-packed segment. Text-align does not apply on that line.
 * The box top matches the line top; there is no baseline alignment. */
static void place_inline_block(we_inline_t *f, int idx, int depth) {
    we_doc_t *d = f->d;
    we_node_t *n = &d->nodes[idx];
    int extra = node_border_extra(n) + n->margin[1] + n->margin[3];
    int inner, outer, height;
    if (n->width >= 0 || n->width_pct > 0 || n->width_kind)
        inner = specified_width(n, f->base_width, n->max_content);
    else inner = n->max_content;
    if (inner < 0) inner = 0;
    inner = constrained_width(n, inner, f->base_width);
    outer = inner + extra;
    if (outer < 1) outer = 1;
    if (outer > WE_LAYOUT_MAX) outer = WE_LAYOUT_MAX;
    if (f->nwords && f->cur_w > 0 && f->cur_w + outer > f->width) flush_line(f, 0);
    if (f->nwords) flush_segment(f);
    flow_region(f, outer < f->base_width ? outer : WE_CHAR_W);
    if (f->pending_x > 0 && f->pending_x + outer > f->width) {
        flush_line(f, 0);
        flow_region(f, outer < f->base_width ? outer : WE_CHAR_W);
    }
    if (outer > f->width - f->pending_x && f->width > f->pending_x) outer = f->width - f->pending_x;
    if (outer < 1) outer = 1;
    if (f->measuring) height = measure_area_height(d, idx, outer, depth);
    else height = layout_assigned(d, idx, f->x0 + f->pending_x, f->y, outer, depth);
    f->pending_x += outer;
    f->cur_w = f->pending_x;
    /* Lot 61: vertical-align:baseline uses the last line box baseline, or
     * the bottom margin edge without one or when overflow is hidden. */
    if (n->valign == WE_VALIGN_BASELINE && f->natoms < WE_LINE_ATOMS) {
        int real, base = baseline_get(d, idx, outer, depth, 1, &real);
        if (!real || n->overflow_hidden) base = height;
        f->atoms[f->natoms].node = idx; f->atoms[f->natoms].base = base; f->atoms[f->natoms].height = height;
        f->natoms++;
        return;
    }
    if (n->valign == WE_VALIGN_BASELINE) diag(d, "inline baseline capacity", idx);
    if (f->natoms < WE_LINE_ATOMS) {
        f->atoms[f->natoms].node = idx; f->atoms[f->natoms].base = -1; f->atoms[f->natoms].height = height;
        f->natoms++;
    }
    if (height > f->atom_h) f->atom_h = height;
}

static int abs_containing(const we_doc_t *d, int idx) {
    int p = d->nodes[idx].parent;
    if (d->nodes[idx].fixed) return -1;
    while (p >= 0) {
        if (d->nodes[p].relative || d->nodes[p].absolute || d->nodes[p].tf_count) return p;
        p = d->nodes[p].parent;
    }
    return -1;
}

/* Shrink-to-fit margin box. A stretch between left and right uses the
 * padding edge and ignores the intrinsic size. Width auto is capped by
 * the padding edge; it does not stretch like a normal block. */
static int absolute_margin_width(const we_node_t *n, int pad_w, int left, int right, int stretch) {
    int extra = node_border_extra(n);
    int margins = n->margin[1] + n->margin[3];
    int content, room;
    if (stretch) {
        int w = pad_w - left - right;
        return w > 0 ? w : 0;
    }
    room = pad_w - extra - margins;
    if (room < 0) room = 0;
    if (n->width < 0 && n->width_pct <= 0 && n->width_kind == WE_SIZE_AUTO) {
        content = n->max_content;
        if (content > room) content = room;
        if (content < n->min_content) content = n->min_content;
        if (content > room) content = room;
    } else content = specified_width(n, pad_w, n->max_content);
    content = content + extra + margins;
    return content > WE_LAYOUT_MAX ? WE_LAYOUT_MAX : content;
}

/* Absolutes are placed after the containing block's in-flow height is
 * known, and before its relative offset, so the offset moves them too.
 * Lot 61: an axis without insets uses the static position recorded where
 * the box would have been in flow (content origin for flex/grid/table).
 * Fixed boxes use the viewport at the current scroll offset. */
static int absolute_stack_key(const we_doc_t *d, int owner, int want) {
    int n = owner;
    while (n >= 0) {
        if (d->nodes[n].absolute && abs_containing(d, n) == want)
            return d->nodes[n].z_set ? d->nodes[n].z_index : 0;
        n = d->nodes[n].parent;
    }
    return 0;
}

/* Stable order of the absolute slice. In-flow items stay where they were.
 * A negative key still paints after in-flow content of the same block. */
static void order_absolute_items(we_doc_t *d, int cb, int start) {
    int positioned = d->nodes[cb].relative || d->nodes[cb].absolute || d->nodes[cb].tf_count;
    int want = positioned ? cb : (d->nodes[cb].parent < 0 ? -1 : -2);
    int i;
    if (want < -1 || start < 0) return;
    for (i = start + 1; i < (int)d->nitems; i++) {
        os_fb_web_item_t item = d->items[i];
        int16_t owner = d->item_owner[i];
        int key = absolute_stack_key(d, owner, want);
        int j = i;
        while (j > start && absolute_stack_key(d, d->item_owner[j - 1], want) > key) {
            d->items[j] = d->items[j - 1];
            d->item_owner[j] = d->item_owner[j - 1];
            j--;
        }
        d->items[j] = item;
        d->item_owner[j] = owner;
    }
}

static void place_absolutes(we_doc_t *d, int cb, int pad_x, int pad_y, int pad_w, int pad_h, int depth) {
    int positioned = d->nodes[cb].relative || d->nodes[cb].absolute || d->nodes[cb].tf_count;
    int want = positioned ? cb : (d->nodes[cb].parent < 0 ? -1 : -2);
    int item_start = (int)d->nitems;
    uint32_t i;
    if (want < -1 || depth > WE_DEPTH_MAX) return;
    if (pad_w < 0) pad_w = 0;
    if (pad_h < 0) pad_h = 0;
    if (pad_w > WE_LAYOUT_MAX) pad_w = WE_LAYOUT_MAX;
    if (pad_h > WE_LAYOUT_MAX) pad_h = WE_LAYOUT_MAX;
    for (i = 0; i < d->nnodes; i++) {
        we_node_t *n = &d->nodes[i];
        int flags, left, right, top, bottom, stretch_w, stretch_h, mbw, mbh, x, y;
        int16_t saved_ch;
        int px = pad_x, py = pad_y, pw = pad_w, ph = pad_h;
        if ((int)i == cb || !n->absolute || n->display == WE_DISP_NONE) continue;
        if (abs_containing(d, (int)i) != want) continue;
        if (n->fixed) { pad_x = 0; pad_y = d->scroll_y; pad_w = d->viewport_w; pad_h = d->viewport_h; }
        spacing_node(n, pad_w);
        flags = n->inset_flags;
        left = (flags & WE_INSET_LEFT) ? math_value(n, WE_LEN_LEFT, pad_w, n->offset_x) : 0;
        right = (flags & WE_INSET_RIGHT) ? math_value(n, WE_LEN_RIGHT, pad_w, n->offset_r) : 0;
        top = (flags & WE_INSET_TOP) ? math_value(n, WE_LEN_TOP, pad_h, n->offset_y) : 0;
        bottom = (flags & WE_INSET_BOTTOM) ? math_value(n, WE_LEN_BOTTOM, pad_h, n->offset_b) : 0;
        stretch_w = (flags & WE_INSET_LEFT) && (flags & WE_INSET_RIGHT) &&
            n->width < 0 && n->width_pct <= 0 && n->width_kind == WE_SIZE_AUTO;
        stretch_h = (flags & WE_INSET_TOP) && (flags & WE_INSET_BOTTOM) &&
            n->height < 0 && !n->height_percent;
        mbw = absolute_margin_width(n, pad_w, left, right, stretch_w);
        saved_ch = containing_height[i];
        containing_height[i] = (int16_t)pad_h;
        if (stretch_h) {
            mbh = pad_h - top - bottom;
            if (mbh < 0) mbh = 0;
        } else mbh = -1;
        if (mbh < 0 && (flags & WE_INSET_BOTTOM) && !(flags & WE_INSET_TOP)) {
            int content = definite_height(d, (int)i, 0);
            if (content >= 0) mbh = content + vertical_extra(n) + n->margin[0] + n->margin[2];
            else {
                int16_t saved = forced_width[i];
                forced_width[i] = (int16_t)mbw;
                mbh = measure_height(d, (int)i, pad_w, depth + 1);
                forced_width[i] = saved;
            }
            if (mbh < 0) mbh = 0;
            if (mbh > WE_LAYOUT_MAX) mbh = WE_LAYOUT_MAX;
        }
        if (flags & WE_INSET_LEFT) x = pad_x + left;
        else if ((flags & WE_INSET_RIGHT) && mbw >= 0) x = pad_x + pad_w - right - mbw;
        else x = static_set[i] ? static_x[i] : pad_x;
        if (flags & WE_INSET_TOP) y = pad_y + top;
        else if ((flags & WE_INSET_BOTTOM) && mbh >= 0) y = pad_y + pad_h - bottom - mbh;
        else y = static_set[i] ? static_y[i] : pad_y;
        layout_area(d, (int)i, limit_coord(x), limit_coord(y), mbw, mbh, pad_w, depth + 1);
        containing_height[i] = saved_ch;
        pad_x = px; pad_y = py; pad_w = pw; pad_h = ph;
    }
    if ((int)d->nitems > item_start) order_absolute_items(d, cb, item_start);
}

typedef struct { int first, end, used, cross, ascent, descent, lascent, ldescent; } we_flex_line_t;
typedef struct {
    int node[WE_MAX_NODES];
    int cross[WE_MAX_NODES];
    int baseline[WE_MAX_NODES];
    we_flex_line_t lines[WE_MAX_NODES];
    int main, cross_available, main_gap, cross_gap, line_count, cross_total;
    int size[WE_MAX_NODES];
    uint32_t shrink_weight[WE_MAX_NODES];
} we_track_work_t;
static we_track_work_t track_work[WE_DEPTH_MAX + 1];

static int collect_items(we_doc_t *d, int idx, we_track_work_t *work) {
    int c, count = 0, i;
    for (c = d->nodes[idx].first; c >= 0; c = d->nodes[c].next)
        if (flex_item(&d->nodes[c]) && !d->nodes[c].absolute) work->node[count++] = c;
    /* Stable by source order when item_order ties. */
    for (i = 1; i < count; i++) {
        int id = work->node[i], j = i;
        while (j > 0 && d->nodes[work->node[j - 1]].item_order > d->nodes[id].item_order) {
            work->node[j] = work->node[j - 1];
            j--;
        }
        work->node[j] = id;
    }
    return count;
}

static int pack_distance(int mode, int free, int index, int count);
static int measure_area_height(we_doc_t *d, int idx, int outer, int depth);

typedef struct {
    int row[WE_MAX_NODES], col[WE_MAX_NODES];
    uint8_t occupied[WE_GRID_ROWS_MAX];
    int column[WE_TRACKS_MAX], column_x[WE_TRACKS_MAX], minimum[WE_TRACKS_MAX];
    int height[WE_GRID_ROWS_MAX], row_y[WE_GRID_ROWS_MAX];
    int ascent[WE_GRID_ROWS_MAX], descent[WE_GRID_ROWS_MAX], baseline[WE_MAX_NODES];
    int lascent[WE_GRID_ROWS_MAX], ldescent[WE_GRID_ROWS_MAX], item_h[WE_MAX_NODES];
    int span[WE_MAX_NODES];
    uint8_t collapsed[2][WE_TRACKS_MAX];
} we_grid_work_t;
static we_grid_work_t grid_work[WE_DEPTH_MAX + 1];
/* Column assignment for rowspan. One slot per layout depth. */
typedef struct {
    int col[WE_MAX_NODES];
    uint8_t occ[WE_GRID_ROWS_MAX];
    int height[WE_GRID_ROWS_MAX];
    int cols;
    /* Lot 61: vertical-align:baseline cells, rowspan 1 only. */
    int16_t base[WE_MAX_NODES];
    int ascent[WE_GRID_ROWS_MAX], descent[WE_GRID_ROWS_MAX];
    /* Lot 63: gap before track k (k >= 1): border-spacing, or for a
     * collapsed table minus the band of grid line k (lines 0..n). */
    int8_t gap_x[WE_TRACKS_MAX + 1], gap_y[WE_GRID_ROWS_MAX + 1];   /* edges <= 127, spacing <= 64 */
} we_table_slot_t;
static we_table_slot_t table_slot[WE_DEPTH_MAX + 1];
typedef char layout_workspace_is_bounded[
    sizeof(track_work) + sizeof(grid_work) + sizeof(table_slot) +
    sizeof(we_inline_t) * (WE_DEPTH_MAX + 1) +
    sizeof(forced_width) + sizeof(forced_height) + sizeof(resolved_height) +
    sizeof(containing_height) + sizeof(flow_baseline) + sizeof(measure_content) + sizeof(measure_cache) + sizeof(int16_t) * OS_FB_WEB_ITEMS +
    sizeof(os_fb_web_item_t) * OS_FB_WEB_ITEMS <= WE_LAYOUT_WORKSPACE_MAX ? 1 : -1];

static int track_extent(int size, int low, int cap) {
    if (size >= 0) return size;
    if (cap >= 0) return cap;
    return low > 0 ? low : 0;
}

/* Rebuild the axis from its template with the largest repetition count
 * that fits the definite size, 1 when indefinite, never above 8 tracks. */
static void grid_expand(we_node_t *n, int axis, int available, int gap) {
    int k, j, out = 0, copies = 1, at = n->auto_at[axis], total = n->tpl_count[axis], other = 0, repeat;
    int16_t *size = axis ? n->row_size : n->col_size;
    int16_t *low = axis ? n->row_floor : n->col_floor, *cap = axis ? n->row_cap : n->col_cap;
    if (!n->auto_mode[axis] || total < 1) return;
    repeat = track_extent(n->tpl_size[axis][at], n->tpl_floor[axis][at], n->tpl_cap[axis][at]);
    for (k = 0; k < total; k++)
        if (k != at) other += track_extent(n->tpl_size[axis][k], n->tpl_floor[axis][k], n->tpl_cap[axis][k]);
    if (available >= 0)
        while (total + copies <= WE_TRACKS_MAX &&
               other + (copies + 1) * repeat + (total + copies - 1) * gap <= available) copies++;
    for (k = 0; k < total; k++)
        for (j = 0; j < (k == at ? copies : 1); j++, out++) {
            size[out] = n->tpl_size[axis][k]; low[out] = n->tpl_floor[axis][k]; cap[out] = n->tpl_cap[axis][k];
        }
    for (k = out; k < WE_TRACKS_MAX; k++) { size[k] = axis ? WE_TRACK_AUTO : -1; low[k] = cap[k] = -1; }
    if (axis) n->grid_rows = (uint8_t)out; else n->grid_cols = (uint8_t)out;
    n->auto_count[axis] = (uint8_t)copies;
}

/* auto-fit: repeated tracks without an item collapse to 0 with their gaps. */
static void grid_collapse(we_node_t *n, int axis, const int *starts, const int *spans, int count, uint8_t *collapsed) {
    int k, i, at = n->auto_at[axis];
    if (n->auto_mode[axis] != WE_AUTO_FIT) return;
    for (k = at; k < at + n->auto_count[axis] && k < WE_TRACKS_MAX; k++) {
        int used = 0;
        for (i = 0; i < count; i++) if (starts[i] >= 0 && starts[i] <= k && k < starts[i] + spans[i]) used = 1;
        if (used) continue;
        collapsed[k] = 1;
        if (axis) { n->row_size[k] = 0; n->row_floor[k] = n->row_cap[k] = -1; }
        else { n->col_size[k] = 0; n->col_floor[k] = n->col_cap[k] = -1; }
    }
}

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
    const we_node_t *n;
    if (!count) return 0;
    n = &d->nodes[d->nodes[items->node[0]].parent];
    int pass, k, rows = 0, cursor = 0, row_limit = n->grid_rows > n->area_rows ? n->grid_rows : n->area_rows;
    if (row_limit < 1) row_limit = 1;
    for (k = 0; k < count; k++) {
        const we_node_t *child = &d->nodes[items->node[k]];
        int end = child->grid_row_start ? child->grid_row_start + child->grid_row_span - 1 : child->grid_row_span;
        if (end > row_limit) row_limit = end;
        work->row[k] = work->col[k] = -1;
    }
    if (row_limit > WE_TRACKS_MAX) row_limit = WE_TRACKS_MAX;
    for (pass = 0; pass < 2; pass++) {
        for (k = 0; k < count; k++) {
            const we_node_t *child = &d->nodes[items->node[k]];
            int explicit = child->grid_col_start || child->grid_row_start;
            int rs = child->grid_row_span, cs = child->grid_col_span;
            int at, first, placed = 0, capacity = n->grid_column_flow ? WE_TRACKS_MAX * row_limit : WE_GRID_ROWS_MAX * cols;
            if ((pass == 0) != explicit) continue;
            if (rs < 1) rs = 1;
            if (cs < 1) cs = 1;
            first = explicit || n->grid_dense ? 0 : cursor;
            for (at = first; at < capacity; at++) {
                int r = n->grid_column_flow ? at % row_limit : at / cols;
                int c = n->grid_column_flow ? at / row_limit : at % cols;
                if (n->grid_column_flow && r + rs > row_limit) continue;
                if (child->grid_row_start && r != child->grid_row_start - 1) continue;
                if (child->grid_col_start && c != child->grid_col_start - 1) continue;
                if (!grid_free(work, r, c, rs, cs, n->grid_column_flow ? WE_TRACKS_MAX : cols)) continue;
                grid_claim(work, r, c, rs, cs);
                work->row[k] = r; work->col[k] = c;
                if (r + rs > rows) rows = r + rs;
                if (!explicit) cursor = at + (n->grid_column_flow ? rs : cs);
                placed = 1;
                break;
            }
            if (!placed && report) diag(d, explicit ? "grid occupied or outside" : "grid placement capacity", items->node[k]);
        }
    }
    return rows;
}

static int grid_spec(const we_node_t *n, int vertical, int k) {
    int declared = vertical ? n->grid_rows : n->grid_cols;
    return k < declared ? (vertical ? n->row_size[k] : n->col_size[k]) : (vertical ? n->auto_row : n->auto_col);
}

static int grid_floor(const we_node_t *n, int vertical, int k) {
    int declared = vertical ? n->grid_rows : n->grid_cols;
    int low = k < declared ? (vertical ? n->row_floor[k] : n->col_floor[k]) : (vertical ? n->auto_row_floor : n->auto_col_floor);
    return low < 0 ? 0 : low;
}

static int grid_cap(const we_node_t *n, int vertical, int k) {
    int declared = vertical ? n->grid_rows : n->grid_cols;
    return k < declared ? (vertical ? n->row_cap[k] : n->col_cap[k]) : (vertical ? n->auto_row_cap : n->auto_col_cap);
}

/* Fractions share a common unit. Freeze floors above that unit and solve
 * again, rather than adding free space on top of unequal minima. */
static int grid_track_sizes(const we_node_t *n, int vertical, int count, int available,
                             int gap, int *sizes, const uint8_t *collapsed) {
    int k, pass, free = available, weight = 0, assigned = 0, gaps = -1;
    uint8_t frozen[WE_GRID_ROWS_MAX];
    w_zero(frozen, sizeof(frozen));
    for (k = 0; k < count; k++) if (!collapsed || k >= WE_TRACKS_MAX || !collapsed[k]) gaps++;
    if (gaps < 0) gaps = 0;
    if (free >= 0) free -= gaps * gap;
    for (k = 0; k < count; k++) {
        int spec = grid_spec(n, vertical, k), low = grid_floor(n, vertical, k), cap = grid_cap(n, vertical, k);
        if (spec >= 0) sizes[k] = spec;
        else if (spec != WE_TRACK_AUTO && available >= 0) sizes[k] = low;
        else {
            if (sizes[k] < low) sizes[k] = low;
            if (cap >= 0 && sizes[k] > cap) sizes[k] = cap;
        }
        if (spec < 0 && spec != WE_TRACK_AUTO && available >= 0) weight -= spec;
        else if (free >= 0) free -= sizes[k];
    }
    if (free < 0) free = 0;
    for (pass = 0; weight > 0 && pass <= count; pass++) {
        int froze = 0;
        for (k = 0; k < count; k++) {
            int spec = grid_spec(n, vertical, k);
            if (frozen[k] || spec >= 0 || spec == WE_TRACK_AUTO) continue;
            if (sizes[k] > free * (-spec) / weight) {
                frozen[k] = 1; free -= sizes[k]; weight += spec; froze = 1;
                if (free < 0) free = 0;
            }
        }
        if (!froze) break;
    }
    if (weight > 0) {
        int used = 0, remainder;
        for (k = 0; k < count; k++) {
            int spec = grid_spec(n, vertical, k);
            if (!frozen[k] && spec < 0 && spec != WE_TRACK_AUTO) { sizes[k] = free * (-spec) / weight; used += sizes[k]; }
        }
        remainder = free - used;
        for (k = 0; k < count && remainder; k++) {
            int spec = grid_spec(n, vertical, k);
            if (!frozen[k] && spec < 0 && spec != WE_TRACK_AUTO) { sizes[k]++; remainder--; }
        }
    }
    assigned = gaps * gap;
    for (k = 0; k < count; k++) assigned += sizes[k];
    return assigned;
}

static void grid_span_minimum(const we_node_t *n, int vertical, int start, int span,
                              int wanted, int gap, int definite, int *sizes) {
    int k, pass;
    for (k = start; k < start + span; k++) {
        int spec = grid_spec(n, vertical, k), low = grid_floor(n, vertical, k);
        if (sizes[k] < low) sizes[k] = low;
        if (spec >= 0) sizes[k] = spec;
    }
    for (pass = 0; pass < span; pass++) {
        int current = (span - 1) * gap, receivers = 0;
        for (k = start; k < start + span; k++) {
            int spec = grid_spec(n, vertical, k), cap = grid_cap(n, vertical, k);
            current += sizes[k];
            if ((spec == WE_TRACK_AUTO || (definite < 0 && spec < 0)) && (cap < 0 || sizes[k] < cap)) receivers++;
        }
        if (wanted <= current || !receivers) return;
        current = wanted - current;
        for (k = start; k < start + span; k++) {
            int spec = grid_spec(n, vertical, k), cap = grid_cap(n, vertical, k), take;
            if (!(spec == WE_TRACK_AUTO || (definite < 0 && spec < 0)) || (cap >= 0 && sizes[k] >= cap)) continue;
            take = current / receivers + (current % receivers != 0);
            if (cap >= 0 && take > cap - sizes[k]) take = cap - sizes[k];
            sizes[k] += take; current -= take; receivers--;
        }
    }
}

static uint32_t flex_share(uint32_t a, uint32_t b, uint32_t denominator);

static void fit_auto_columns(const we_node_t *n, int cols, int width, int gap,
                             const int *minimum, int *sizes) {
    int k, used = (cols - 1) * gap, room = 0, excess, removed = 0;
    for (k = 0; k < cols; k++) {
        int spec = grid_spec(n, 0, k);
        used += spec >= 0 ? spec : spec == WE_TRACK_AUTO ? sizes[k] : 0;
        if (spec == WE_TRACK_AUTO && sizes[k] > minimum[k]) room += sizes[k] - minimum[k];
    }
    excess = used > width ? used - width : 0;
    if (excess > room) excess = room;
    if (!room || !excess) return;
    for (k = 0; k < cols; k++) {
        int spec = grid_spec(n, 0, k);
        if (spec == WE_TRACK_AUTO && sizes[k] > minimum[k]) {
            int take = (int)flex_share((uint32_t)excess, (uint32_t)(sizes[k] - minimum[k]), (uint32_t)room);
            sizes[k] -= take;
            removed += take;
        }
    }
    for (k = 0; k < cols && removed < excess; k++) {
        int spec = grid_spec(n, 0, k);
        if (spec == WE_TRACK_AUTO && sizes[k] > minimum[k]) { sizes[k]--; removed++; }
    }
}

static int grid_item_width(const we_node_t *n, const we_node_t *child, int width, int ci) {
    int align = child->justify_self ? child->justify_self : n->justify_items;
    int intrinsic = (align == WE_PACK_END || align == WE_PACK_CENTER || align == WE_PACK_START ||
                    (child->margin_auto & (WE_AUTO_LEFT | WE_AUTO_RIGHT))) && !subgrid_on(ci, 0);
    return preferred_outer(child, width, intrinsic ? child->max_content : width - node_border_extra(child) - child->margin[1] - child->margin[3]);
}

static void subgrid_bind_items(we_doc_t *d, int p, const we_track_work_t *items,
                               const we_grid_work_t *work, int count, int report) {
    int k;
    for (k = 0; k < count; k++) {
        int s = items->node[k];
        if (!subgrid_ok[s] || work->row[k] < 0) continue;
        if (subgrid_on(s, 0)) subgrid_bind(d, p, s, 0, work->col[k], d->nodes[s].grid_col_span);
        if (subgrid_on(s, 1)) subgrid_bind(d, p, s, 1, work->row[k], d->nodes[s].grid_row_span);
        subgrid_items(d, s, report);
    }
}

static int grid_columns_for(const we_doc_t *d, const we_node_t *n, const we_track_work_t *items, int count) {
    int k, cols = n->grid_cols ? n->grid_cols : 1;
    for (k = 0; k < count; k++) {
        const we_node_t *child = &d->nodes[items->node[k]];
        int end = child->grid_col_start ? child->grid_col_start + child->grid_col_span - 1 : child->grid_col_span;
        if (end > cols) cols = end;
    }
    if (cols > WE_TRACKS_MAX) cols = WE_TRACKS_MAX;
    if (n->area_cols > cols) cols = n->area_cols;
    return cols;
}

/* Layout entry: bind every valid subgrid once and report its items. */
static void subgrid_bind_all(we_doc_t *d) {
    uint32_t p;
    for (p = 0; p < d->nnodes; p++) {
        const we_node_t *n = &d->nodes[p];
        we_track_work_t *items = &track_work[0];
        we_grid_work_t *work = &grid_work[0];
        int c, any = 0, count;
        if (n->display != WE_DISP_GRID) continue;
        for (c = n->first; c >= 0; c = d->nodes[c].next) if (subgrid_ok[c]) any = 1;
        if (!any) continue;
        count = collect_items(d, (int)p, items);
        w_zero(work, sizeof(*work));
        grid_place(d, items, work, count, grid_columns_for(d, n, items, count), 0);
        subgrid_bind_items(d, (int)p, items, work, count, 1);
    }
}

/* Column contributions of a column subgrid's items to the parent tracks. */
static void subgrid_columns(we_doc_t *d, int p, int s, int start, int cw, int gap, we_grid_work_t *work, int depth) {
    we_track_work_t *items = &track_work[depth + 1];
    we_grid_work_t *inner = &grid_work[depth + 1];
    const we_node_t *g = &d->nodes[p];
    int count = collect_items(d, s, items), k;
    if (depth + 1 > WE_DEPTH_MAX) return;
    w_zero(inner, sizeof(*inner));
    grid_place(d, items, inner, count, d->nodes[s].grid_cols, 0);
    for (k = 0; k < count; k++) {
        const we_node_t *child = &d->nodes[items->node[k]];
        int extra;
        if (inner->row[k] < 0) continue;
        extra = subgrid_item_extra(&d->nodes[s], 0, inner->col[k], child->grid_col_span, gap);
        grid_span_minimum(g, 0, start + inner->col[k], child->grid_col_span,
                          w_max(0, preferred_outer(child, cw, child->max_content) + extra), gap, cw, work->column);
        grid_span_minimum(g, 0, start + inner->col[k], child->grid_col_span,
                          w_max(0, preferred_outer(child, cw, child->min_content) + extra), gap, cw, work->minimum);
    }
}

static int prepare_grid(we_doc_t *d, int idx, int cx, int cy, int cw, int depth);

/* Lot 61: a row subgrid's baseline-aligned items join the parent's
 * baseline groups (CSS Grid 2, 9.6). The parent's shared ascent and
 * last descent per spanned row are handed back to the subgrid, in its own
 * row coordinates, before it places its items. */
static int16_t subgrid_ascent[WE_MAX_NODES][WE_TRACKS_MAX], subgrid_ldescent[WE_MAX_NODES][WE_TRACKS_MAX];

/* Offset of subgrid row k's top and bottom from the parent's lines. */
static int subgrid_row_edge(const we_node_t *sub, int k, int gap, int bottom) {
    int h1, h2;
    (void)subgrid_gap_delta(sub, 1, gap, &h1, &h2);
    return bottom ? (k + 1 < sub->grid_rows ? h1 : 0) : (k > 0 ? h2 : 0);
}

/* Row contributions of a row subgrid's items, measured at the widths the
 * subgrid gives them (inherited or its own columns). */
static void subgrid_rows(we_doc_t *d, int p, int s, int start, int width, int fixed_height, int gap,
                         we_grid_work_t *work, int depth) {
    we_track_work_t *items = &track_work[depth + 1];
    we_grid_work_t *inner = &grid_work[depth + 1];
    const we_node_t *g = &d->nodes[p], *sub = &d->nodes[s];
    int count, k;
    if (depth + 1 > WE_DEPTH_MAX) return;
    (void)prepare_grid(d, s, 0, 0, width, depth + 1);
    count = collect_items(d, s, items);
    for (k = 0; k < count; k++) {
        const we_node_t *child = &d->nodes[items->node[k]];
        int c = inner->col[k], span, w, h;
        if (inner->row[k] < 0) continue;
        span = child->grid_col_span;
        w = inner->column_x[c + span - 1] + inner->column[c + span - 1] - inner->column_x[c];
        h = measure_area_height(d, items->node[k], grid_item_width(sub, child, w, items->node[k]), depth + 2);
        {
            int align = child->align_self ? child->align_self : sub->align_items;
            int last = align == WE_PACK_LAST_BASELINE, real, base, row, top, bottom;
            if ((align == WE_PACK_BASELINE || last) && !(child->margin_auto & (WE_AUTO_TOP | WE_AUTO_BOTTOM))) {
                row = last ? inner->row[k] + child->grid_row_span - 1 : inner->row[k];
                base = baseline_get(d, items->node[k], grid_item_width(sub, child, w, items->node[k]), depth + 2, last, &real);
                top = subgrid_row_edge(sub, inner->row[k], gap, 0);
                bottom = subgrid_row_edge(sub, inner->row[k] + child->grid_row_span - 1, gap, 1);
                if (start + row < WE_GRID_ROWS_MAX) {
                    if (last) {
                        if (h - base + bottom > work->ldescent[start + row]) work->ldescent[start + row] = h - base + bottom;
                        if (child->grid_row_span == 1 && base + top > work->lascent[start + row]) work->lascent[start + row] = base + top;
                    } else {
                        if (base + top > work->ascent[start + row]) work->ascent[start + row] = base + top;
                        if (child->grid_row_span == 1 && h - base + bottom > work->descent[start + row]) work->descent[start + row] = h - base + bottom;
                    }
                }
            }
        }
        h = w_max(0, h + subgrid_item_extra(sub, 1, inner->row[k], child->grid_row_span, gap));
        grid_span_minimum(g, 1, start + inner->row[k], child->grid_row_span, h, gap, fixed_height, work->height);
    }
}

/* After the parent sized its rows: hand the shared groups to subgrid s. */
static void subgrid_share(we_doc_t *d, int s, int start, int gap, const we_grid_work_t *work) {
    const we_node_t *sub = &d->nodes[s];
    int k;
    for (k = 0; k < sub->grid_rows && k < WE_TRACKS_MAX && start + k < WE_GRID_ROWS_MAX; k++) {
        subgrid_ascent[s][k] = (int16_t)w_max(0, work->ascent[start + k] - subgrid_row_edge(sub, k, gap, 0));
        subgrid_ldescent[s][k] = (int16_t)w_max(0, work->ldescent[start + k] - subgrid_row_edge(sub, k, gap, 1));
    }
}

static int prepare_grid(we_doc_t *d, int idx, int cx, int cy, int cw, int depth) {
    we_node_t *n = &d->nodes[idx];
    we_track_work_t *items = &track_work[depth];
    we_grid_work_t *work = &grid_work[depth];
    int count, cols, seen;
    int rows, k, used_width, used_height, fixed_height = definite_height(d, idx, 0);
    int gx = axis_gap(n, 0), gy = axis_gap(n, 1), position;
    if (n->auto_mode[0] || n->auto_mode[1]) {
        int c;
        grid_expand(n, 0, cw, gx);
        grid_expand(n, 1, fixed_height, gy);
        for (c = n->first; c >= 0; c = d->nodes[c].next) resolve_requests(d, c, 0);
    }
    count = collect_items(d, idx, items);
    cols = n->grid_cols ? n->grid_cols : 1;
    w_zero(work, sizeof(*work));
    for (k = 0; k < count; k++) {
        const we_node_t *child = &d->nodes[items->node[k]];
        int end = child->grid_col_start ? child->grid_col_start + child->grid_col_span - 1 : child->grid_col_span;
        if (end > cols) cols = end;
    }
    if (cols > WE_TRACKS_MAX) cols = WE_TRACKS_MAX;
    if (n->area_cols > cols) cols = n->area_cols;
    rows = grid_place(d, items, work, count, cols, 1);
    if (n->grid_column_flow) for (k = 0; k < count; k++) {
        int end = work->col[k] + d->nodes[items->node[k]].grid_col_span;
        if (work->col[k] >= 0 && end > cols) cols = end;
    }
    if (n->area_rows > rows) rows = n->area_rows;
    if (n->grid_rows > rows) rows = n->grid_rows;
    subgrid_bind_items(d, idx, items, work, count, 0);
    for (k = 0; k < count; k++) work->span[k] = d->nodes[items->node[k]].grid_col_span;
    grid_collapse(n, 0, work->col, work->span, count, work->collapsed[0]);
    for (k = 0; k < count; k++) work->span[k] = d->nodes[items->node[k]].grid_row_span;
    grid_collapse(n, 1, work->row, work->span, count, work->collapsed[1]);
    for (k = 0; k < count; k++) {
        const we_node_t *child = &d->nodes[items->node[k]];
        if (work->row[k] < 0) continue;
        if (subgrid_on(items->node[k], 0)) {
            subgrid_columns(d, idx, items->node[k], work->col[k], cw, gx, work, depth);
            continue;
        }
        grid_span_minimum(n, 0, work->col[k], child->grid_col_span,
                         preferred_outer(child, cw, child->max_content), gx, cw, work->column);
        grid_span_minimum(n, 0, work->col[k], child->grid_col_span,
                         preferred_outer(child, cw, child->min_content), gx, cw, work->minimum);
    }
    fit_auto_columns(n, cols, cw, gx, work->minimum, work->column);
    used_width = grid_track_sizes(n, 0, cols, cw, gx, work->column, work->collapsed[0]);
    if (used_width > cw) diag(d, "grid width overflow", idx);
    position = cx; seen = 0;
    for (k = 0; k < cols; k++) {
        if (seen && !work->collapsed[0][k]) position += gx;
        work->column_x[k] = limit_coord(position + pack_distance(n->justify_content, cw - used_width, k, cols));
        position += work->column[k];
        if (!work->collapsed[0][k]) seen = 1;
    }
    for (k = 0; k < count; k++)
        if (work->row[k] >= 0 && subgrid_on(items->node[k], 0))
            subgrid_tracks(&d->nodes[items->node[k]], 0, work->column, work->column_x, work->col[k], gx);
    /* Lot 61: first-baseline groups share the start row and last-baseline
     * groups the end row, spanning items included. A spanning item adds the
     * shift its group imposes to its span contribution. */
    for (k = 0; k < count; k++) {
        const we_node_t *child = &d->nodes[items->node[k]];
        int width, height, align = child->align_self ? child->align_self : n->align_items;
        if (work->row[k] < 0) continue;
        width = work->column_x[work->col[k] + child->grid_col_span - 1] +
                work->column[work->col[k] + child->grid_col_span - 1] - work->column_x[work->col[k]];
        spacing_node(&d->nodes[items->node[k]], width);
        if (subgrid_on(items->node[k], 1)) {
            subgrid_rows(d, idx, items->node[k], work->row[k], grid_item_width(n, child, width, items->node[k]), fixed_height, gy, work, depth);
            work->item_h[k] = -1;
            continue;
        }
        height = measure_area_height(d, items->node[k], grid_item_width(n, child, width, items->node[k]), depth + 1);
        work->item_h[k] = height;
        work->baseline[k] = 0;
        if ((align == WE_PACK_BASELINE || align == WE_PACK_LAST_BASELINE) &&
            !(child->margin_auto & (WE_AUTO_TOP | WE_AUTO_BOTTOM))) {
            int real, last = align == WE_PACK_LAST_BASELINE;
            int row = last ? work->row[k] + child->grid_row_span - 1 : work->row[k], below;
            work->baseline[k] = baseline_get(d, items->node[k], grid_item_width(n, child, width, items->node[k]), depth + 1, last, &real);
            below = height - work->baseline[k];
            if (row >= WE_GRID_ROWS_MAX) continue;
            if (last) {
                if (below > work->ldescent[row]) work->ldescent[row] = below;
                if (child->grid_row_span == 1 && work->baseline[k] > work->lascent[row]) work->lascent[row] = work->baseline[k];
            } else {
                if (work->baseline[k] > work->ascent[row]) work->ascent[row] = work->baseline[k];
                if (child->grid_row_span == 1 && below > work->descent[row]) work->descent[row] = below;
            }
        }
    }
    for (k = 0; k < count; k++) {
        const we_node_t *child = &d->nodes[items->node[k]];
        int align = child->align_self ? child->align_self : n->align_items, height = work->item_h[k];
        int end = work->row[k] + child->grid_row_span - 1;
        if (work->row[k] < 0 || height < 0) continue;
        if (!(child->margin_auto & (WE_AUTO_TOP | WE_AUTO_BOTTOM)) && child->grid_row_span > 1) {
            if (align == WE_PACK_BASELINE) height += work->ascent[work->row[k]] - work->baseline[k];
            else if (align == WE_PACK_LAST_BASELINE && end < WE_GRID_ROWS_MAX)
                height += work->ldescent[end] - (work->item_h[k] - work->baseline[k]);
        }
        grid_span_minimum(n, 1, work->row[k], child->grid_row_span, height, gy, fixed_height, work->height);
    }
    for (k = 0; k < rows && k < WE_GRID_ROWS_MAX; k++) {
        grid_span_minimum(n, 1, k, 1, work->ascent[k] + work->descent[k], gy, fixed_height, work->height);
        grid_span_minimum(n, 1, k, 1, work->lascent[k] + work->ldescent[k], gy, fixed_height, work->height);
    }
    used_height = grid_track_sizes(n, 1, rows, fixed_height, gy, work->height, work->collapsed[1]);
    position = cy; seen = 0;
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
            if (seen && !(k < WE_TRACKS_MAX && work->collapsed[1][k])) position += gy;
            work->row_y[k] = limit_coord(position + (n->align_content == WE_PACK_STRETCH ? 0 :
                pack_distance(n->align_content, fixed_height - used_height, k, rows)));
            work->height[k] += extra;
            position += work->height[k];
            if (!(k < WE_TRACKS_MAX && work->collapsed[1][k])) seen = 1;
        }
    }
    for (k = 0; k < count; k++)
        if (work->row[k] >= 0 && subgrid_on(items->node[k], 1)) {
            subgrid_tracks(&d->nodes[items->node[k]], 1, work->height, work->row_y, work->row[k], gy);
            subgrid_share(d, items->node[k], work->row[k], gy, work);
        }
    if (subgrid_on(idx, 1))
        for (k = 0; k < rows && k < WE_TRACKS_MAX; k++) {
            if (subgrid_ascent[idx][k] > work->ascent[k]) work->ascent[k] = subgrid_ascent[idx][k];
            if (subgrid_ldescent[idx][k] > work->ldescent[k]) work->ldescent[k] = subgrid_ldescent[idx][k];
        }
    return rows;
}

static int measure_grid(we_doc_t *d, int idx, int width, int depth) {
    int rows = prepare_grid(d, idx, 0, 0, width, depth);
    const we_grid_work_t *work = &grid_work[depth];
    return rows ? limit_coord(work->row_y[rows - 1] + work->height[rows - 1]) : 0;
}

/* Grid baseline export: the first row's first-baseline group, else its
 * leftmost item; the last row's last-baseline group, else its rightmost. */
typedef struct { int row[2], col[2], group[2], item[2]; } we_grid_export_t;
static we_grid_export_t grid_exports[WE_DEPTH_MAX + 1];
static void export_reset(we_grid_export_t *e) {
    int i;
    for (i = 0; i < 2; i++) {
        e->row[i] = i ? -1 : 0x7fff; e->col[i] = i ? -1 : 0x7fff;
        e->group[i] = e->item[i] = WE_NO_BASELINE;
    }
}
static void grid_export(we_grid_export_t *e, int r, int c, int end_r, int end_c,
                        int align, int group, int first, int last, int aligned) {
    if (r < e->row[0]) { e->row[0] = r; e->col[0] = 0x7fff; e->group[0] = WE_NO_BASELINE; }
    if (r == e->row[0]) {
        if (aligned && align == WE_PACK_BASELINE && e->group[0] == WE_NO_BASELINE) e->group[0] = group;
        if (c < e->col[0]) { e->col[0] = c; e->item[0] = first; }
    }
    if (end_r > e->row[1]) { e->row[1] = end_r; e->col[1] = -1; e->group[1] = WE_NO_BASELINE; }
    if (end_r == e->row[1]) {
        if (aligned && align == WE_PACK_LAST_BASELINE) e->group[1] = group;
        if (end_c > e->col[1]) { e->col[1] = end_c; e->item[1] = last; }
    }
}

static int layout_grid(we_doc_t *d, int idx, int cx, int cy, int cw, int depth) {
    we_node_t *n = &d->nodes[idx];
    we_track_work_t *items = &track_work[depth];
    we_grid_work_t *work = &grid_work[depth];
    int rows = prepare_grid(d, idx, cx, cy, cw, depth);
    int count = collect_items(d, idx, items), k;
    int fixed_height = definite_height(d, idx, 0);
    we_grid_export_t *e = &grid_exports[depth];
    export_reset(e);
    for (k = 0; k < count; k++) {
        we_node_t *child = &d->nodes[items->node[k]];
        int r = work->row[k], c = work->col[k], width, height, child_width, child_height;
        int horizontal = child->justify_self ? child->justify_self : n->justify_items;
        int vertical = child->align_self ? child->align_self : n->align_items;
        int x, y, assigned_h = -1;
        if (r < 0) continue;
        /* A subgrid ignores self-alignment on its subgridded axes (columns:
         * grid_item_width keeps the full track width). */
        if (subgrid_on(items->node[k], 1)) vertical = WE_PACK_STRETCH;
        width = work->column_x[c + child->grid_col_span - 1] + work->column[c + child->grid_col_span - 1] - work->column_x[c];
        height = work->row_y[r + child->grid_row_span - 1] + work->height[r + child->grid_row_span - 1] - work->row_y[r];
        child_width = grid_item_width(n, child, width, items->node[k]);
        {
            int child_idx = items->node[k];
            int16_t saved = containing_height[child_idx];
            containing_height[child_idx] = (int16_t)height;
            child_height = measure_area_height(d, child_idx, child_width, depth + 1);
            containing_height[child_idx] = saved;
        }
        if (!(child->margin_auto & (WE_AUTO_TOP | WE_AUTO_BOTTOM)) && vertical == WE_PACK_STRETCH && child->height < 0 && !child->height_percent)
            child_height = height_constraint(d, items->node[k], height - vertical_extra(child) - child->margin[0] - child->margin[2], fixed_height) + vertical_extra(child) + child->margin[0] + child->margin[2];
        if (!(child->margin_auto & (WE_AUTO_TOP | WE_AUTO_BOTTOM)) && vertical == WE_PACK_STRETCH && child->height < 0 && !child->height_percent) assigned_h = child_height;
        x = work->column_x[c] + ((child->margin_auto & (WE_AUTO_LEFT | WE_AUTO_RIGHT)) ?
            horizontal_auto(child, width, child_width - child->margin[1] - child->margin[3], 0) :
            pack_distance(horizontal, width - child_width, 0, 1));
        if (child->margin_auto & (WE_AUTO_TOP | WE_AUTO_BOTTOM)) {
            int free = height > child_height ? height - child_height : 0;
            y = work->row_y[r] + ((child->margin_auto & WE_AUTO_TOP) ?
                (child->margin_auto & WE_AUTO_BOTTOM ? free / 2 : free) : 0);
        } else if (vertical == WE_PACK_BASELINE)
            y = work->row_y[r] + work->ascent[r] - work->baseline[k];
        else if (vertical == WE_PACK_LAST_BASELINE) {
            int end = r + child->grid_row_span - 1;
            y = work->row_y[end] + work->height[end] - work->ldescent[end] - work->baseline[k];
        } else y = work->row_y[r] + pack_distance(vertical, height - child_height, 0, 1);
        {
            int child_idx = items->node[k];
            int16_t saved = containing_height[child_idx];
            containing_height[child_idx] = (int16_t)height;
            layout_area(d, child_idx, limit_coord(x), limit_coord(y), child_width, assigned_h, width, depth + 1);
            containing_height[child_idx] = saved;
        }
        grid_export(e, r, c, r + child->grid_row_span - 1, c + child->grid_col_span - 1,
                    vertical, limit_coord(y + work->baseline[k]), item_baseline(d, items->node[k], 0),
                    item_baseline(d, items->node[k], 1), !(child->margin_auto & (WE_AUTO_TOP | WE_AUTO_BOTTOM)));
    }
    {
        int i;
        for (i = 0; i < 2; i++) {
            int value = e->group[i] != WE_NO_BASELINE ? e->group[i] : e->item[i];
            if (i) layout_last[idx] = (int16_t)value;
            else layout_first[idx] = (int16_t)value;
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
    if (!vertical) return width_limit(n, containing, high);
    if (vertical) {
        int percent = high ? n->max_height_percent : n->min_height_percent;
        value = high ? n->max_height : n->min_height;
        if (percent) value = containing >= 0 ? containing * (high ? n->max_height_pct : n->min_height_pct) / 100 : -1;
    }
    {
        int slot = vertical ? (high ? WE_LEN_MAX_H : WE_LEN_MIN_H) : (high ? WE_LEN_MAX_W : WE_LEN_MIN_W);
        value = math_nonnegative(n, slot, containing, n->length[slot].count ? (high ? -1 : 0) : value);
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

/* Lot 61 container baselines: a flex, grid or table box is laid out for
 * real at (0, 0) and its exported layout_first/layout_last is read back.
 * Geometry, items, diagnostics, counters and baselines are restored, the
 * nesting is capped at WE_PROBE_DEPTH and the count at WE_PROBE_MAX. */
#define WE_PROBE_DEPTH 3
#define WE_PROBE_MAX 64
typedef struct {
    int16_t x[WE_MAX_NODES], y[WE_MAX_NODES], w[WE_MAX_NODES], h[WE_MAX_NODES];
    int16_t first[WE_MAX_NODES], last[WE_MAX_NODES], resolved[WE_MAX_NODES], containing[WE_MAX_NODES];
    uint8_t failed[WE_MAX_NODES], set[WE_MAX_NODES];
    int16_t sx[WE_MAX_NODES], sy[WE_MAX_NODES];
} we_probe_save_t;
static we_probe_save_t probe_save[WE_PROBE_DEPTH];
static int probe_level, probe_count;

static int probe_container(we_doc_t *d, int idx, int outer, int depth, int last) {
    we_probe_save_t *save;
    uint32_t nitems = d->nitems, ndiag = d->ndiag, dropped = d->diag_dropped;
    we_perf_t perf = d->perf;
    int i, value, saved_probe = baseline_probe;
    if (probe_level >= WE_PROBE_DEPTH) return WE_NO_BASELINE;
    if (probe_count >= WE_PROBE_MAX) {
        if (probe_count++ == WE_PROBE_MAX) diag(d, "baseline probe budget", idx);
        return WE_NO_BASELINE;
    }
    probe_count++;
    save = &probe_save[probe_level++];
    for (i = 0; i < (int)d->nnodes; i++) {
        save->x[i] = d->nodes[i].x; save->y[i] = d->nodes[i].y;
        save->w[i] = d->nodes[i].w; save->h[i] = d->nodes[i].h;
        save->first[i] = layout_first[i]; save->last[i] = layout_last[i];
        save->resolved[i] = resolved_height[i]; save->containing[i] = containing_height[i];
        save->failed[i] = image_work.failed[i];
        save->set[i] = static_set[i]; save->sx[i] = static_x[i]; save->sy[i] = static_y[i];
    }
    baseline_probe = 0;
    (void)layout_assigned(d, idx, 0, 0, outer, depth);
    value = last ? layout_last[idx] : layout_first[idx];
    baseline_probe = saved_probe;
    for (i = 0; i < (int)d->nnodes; i++) {
        d->nodes[i].x = save->x[i]; d->nodes[i].y = save->y[i];
        d->nodes[i].w = save->w[i]; d->nodes[i].h = save->h[i];
        layout_first[i] = save->first[i]; layout_last[i] = save->last[i];
        resolved_height[i] = save->resolved[i]; containing_height[i] = save->containing[i];
        image_work.failed[i] = save->failed[i];
        static_set[i] = save->set[i]; static_x[i] = save->sx[i]; static_y[i] = save->sy[i];
    }
    probe_level--;
    d->nitems = nitems; d->ndiag = ndiag; d->diag_dropped = dropped; d->perf = perf;
    return value;
}

/* First (last=0) or last baseline from the margin-box top. *real is 0 when
 * the value is the synthesized bottom border edge. */
static int baseline_get(we_doc_t *d, int idx, int outer, int depth, int last, int *real) {
    const we_node_t *n = &d->nodes[idx];
    int height = measure_area_height(d, idx, outer, depth), value, saved_probe = baseline_probe;
    int bottom = limit_coord(height - n->margin[2]);
    uint8_t saved_content = measure_content[idx];
    *real = 0;
    if (depth > WE_DEPTH_MAX) return bottom;
    if (w_eq(n->tag, "img")) { *real = 1; return bottom; }
    if (n->overflow_hidden) {
        *real = (last ? flow_last[idx] : flow_baseline[idx]) >= 0;
        return bottom;
    }
    if (n->display == WE_DISP_FLEX || n->display == WE_DISP_GRID || n->display == WE_DISP_TABLE) {
        value = probe_container(d, idx, outer, depth, last);
        if (value == WE_NO_BASELINE) return bottom;
        *real = 1;
        return limit_coord(value);
    }
    baseline_probe = 1; measure_content[idx] = 1;
    (void)measure_area_height(d, idx, outer, depth);
    measure_content[idx] = saved_content; baseline_probe = saved_probe;
    value = last ? flow_last[idx] : flow_baseline[idx];
    if (value >= 0) { *real = 1; value += n->margin[0] + n->border + n->padding[0]; }
    else value = bottom;
    return limit_coord(value);
}

/* Baseline of a laid-out item, synthesized from its bottom border edge. */
static int item_baseline(const we_doc_t *d, int c, int last) {
    int value = last ? layout_last[c] : layout_first[c];
    if (value != WE_NO_BASELINE) return value;
    return limit_coord(d->nodes[c].y + d->nodes[c].h);
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
        if (child->basis_kind && !vertical) basis = intrinsic_value(child, WE_LEN_BASIS, cw);
        else if (child->basis_kind && vertical) {
            measure_content[work->node[k]] = 1;
            basis = measure_area_height(d, work->node[k], cw, depth + 1) - flex_extra(child, 1);
            measure_content[work->node[k]] = 0;
        } else if (child->length[WE_LEN_BASIS].count) {
            if (main < 0 && child->length[WE_LEN_BASIS].percent) {
                diag(d, "column percent requires height", work->node[k]);
                basis = child->height >= 0 ? child->height : 0;
            } else basis = content_value(math_nonnegative(child, WE_LEN_BASIS, main, 0),
                vertical ? vertical_extra(child) : node_border_extra(child), child->border_box);
        } else if (child->basis_pct > 0 && main < 0) {
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
        line->ascent = line->descent = line->lascent = line->ldescent = 0;
        for (j = line->first; j < k; j++) {
            const we_node_t *child = &d->nodes[work->node[j]];
            int align = child->align_self ? child->align_self : n->align_items;
            line->used += work->size[j];
            if (vertical) {
                int fallback = wrap ? WE_CHAR_W : align == WE_PACK_END || align == WE_PACK_CENTER ||
                    align == WE_PACK_BASELINE || align == WE_PACK_LAST_BASELINE ||
                    (child->margin_auto & (WE_AUTO_LEFT | WE_AUTO_RIGHT)) ? child->max_content : cw - flex_extra(child, 0);
                work->cross[j] = preferred_outer(child, cw, fallback);
            } else work->cross[j] = measure_area_height(d, work->node[j], work->size[j], depth + 1);
            work->baseline[j] = 0;
            if (align == WE_PACK_BASELINE || align == WE_PACK_LAST_BASELINE) {
                /* Column flex: no baseline along the horizontal cross axis,
                 * so first baseline falls back to start and last baseline to
                 * end (CSS Box Alignment 9.3); nothing is diagnosed. */
                if (!vertical && !(child->margin_auto & (WE_AUTO_TOP | WE_AUTO_BOTTOM))) {
                    int below, real, last = align == WE_PACK_LAST_BASELINE;
                    work->baseline[j] = baseline_get(d, work->node[j], work->size[j], depth + 1, last, &real);
                    below = work->cross[j] - work->baseline[j];
                    if (last) {
                        if (work->baseline[j] > line->lascent) line->lascent = work->baseline[j];
                        if (below > line->ldescent) line->ldescent = below;
                    } else {
                        if (work->baseline[j] > line->ascent) line->ascent = work->baseline[j];
                        if (below > line->descent) line->descent = below;
                    }
                }
            }
            if (work->cross[j] > line->cross) line->cross = work->cross[j];
        }
        if (line->ascent + line->descent > line->cross) line->cross = line->ascent + line->descent;
        if (line->lascent + line->ldescent > line->cross) line->cross = line->lascent + line->ldescent;
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
    *modern = n->justify_content || n->align_items || n->align_content || n->row_gap >= 0 || n->col_gap >= 0 || n->flex_reverse || n->flex_wrap == 2;
    for (k = 0; k < count; k++) {
        const we_node_t *child = &d->nodes[work->node[k]];
        if (child->margin_auto) *modern = 1;
        if (child->length[WE_LEN_WIDTH].count || child->length[WE_LEN_MIN_W].count || child->length[WE_LEN_MAX_W].count ||
            child->min_width_kind || child->max_width_kind || child->basis_kind) *modern = 1;
        if (child->align_self || child->basis_content || child->border_box || child->aspect_num || child->height_percent ||
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
    int group_first = WE_NO_BASELINE, group_last = WE_NO_BASELINE;
    int item_first = WE_NO_BASELINE, item_last = WE_NO_BASELINE;
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
        int j, line_cross = line->cross + extra;
        int line_autos = 0, auto_share = 0, auto_rem = 0;
        if (n->flex_wrap == 2) {
            int limit = cross_available > cross_total ? cross_available : cross_total;
            int used = cross_start + line_cross;
            if (limit < used) limit = used;
            cross_start = limit - used;
            if (cross_start < 0) cross_start = 0;
        }
        uint8_t before_bit = vertical ? WE_AUTO_TOP : WE_AUTO_LEFT;
        uint8_t after_bit = vertical ? WE_AUTO_BOTTOM : WE_AUTO_RIGHT;
        /* Reverse lays out from the other main edge. Swap the physical
         * auto-margin sides first so the later mirror keeps left on the left. */
        if (n->flex_reverse) {
            uint8_t swap = before_bit;
            before_bit = after_bit;
            after_bit = swap;
        }
        for (j = line->first; j < line->end; j++) {
            uint8_t bits = d->nodes[work->node[j]].margin_auto;
            if (bits & before_bit) line_autos++;
            if (bits & after_bit) line_autos++;
        }
        if (line_autos && main_free > 0) {
            auto_share = main_free / line_autos;
            auto_rem = main_free % line_autos;
            main_free = 0;
        }
        for (j = line->first; j < line->end; j++) {
            we_node_t *child = &d->nodes[work->node[j]];
            int align = child->align_self ? child->align_self : n->align_items;
            int cross = work->cross[j], main_start, off, x, y, width, height;
            int cross_before = child->margin_auto & (vertical ? WE_AUTO_LEFT : WE_AUTO_TOP);
            int cross_after = child->margin_auto & (vertical ? WE_AUTO_RIGHT : WE_AUTO_BOTTOM);
            int cross_auto = cross_before || cross_after;
            if (align == WE_PACK_STRETCH && !cross_auto) {
                if (vertical && child->width < 0 && !child->width_pct && !child->width_kind)
                    cross = constrained_width(child, line_cross - flex_extra(child, 0), cw) + flex_extra(child, 0);
                if (!vertical && child->height < 0 && !child->height_percent)
                    cross = height_constraint(d, work->node[j], line_cross - flex_extra(child, 1), cross_available) + flex_extra(child, 1);
            }
            if (cross_auto) {
                int free = line_cross > cross ? line_cross - cross : 0;
                off = cross_before ? (cross_after ? free / 2 : free) : 0;
            } else if (!vertical && align == WE_PACK_BASELINE) off = line->ascent - work->baseline[j];
            else if (!vertical && align == WE_PACK_LAST_BASELINE) off = line_cross - line->ldescent - work->baseline[j];
            else if (align == WE_PACK_LAST_BASELINE) off = line_cross > cross ? line_cross - cross : 0;
            else off = pack_distance(align, line_cross - cross, 0, 1);
            {
                int cursor = 0, t, rem_left = auto_rem;
                for (t = line->first; t < j; t++) {
                    uint8_t bits = d->nodes[work->node[t]].margin_auto;
                    cursor += work->size[t] + gap;
                    if (bits & before_bit) { cursor += auto_share; if (rem_left) { cursor++; rem_left--; } }
                    if (bits & after_bit) { cursor += auto_share; if (rem_left) { cursor++; rem_left--; } }
                }
                {
                    uint8_t bits = child->margin_auto;
                    int before = 0;
                    if (bits & before_bit) { before = auto_share; if (rem_left) { before++; rem_left--; } }
                    main_start = cursor + before + pack_distance(n->justify_content, main_free, j - line->first, line->end - line->first);
                }
            }
            if (n->flex_reverse) {
                int limit = main >= 0 ? main : line->used;
                main_start = limit - main_start - work->size[j];
            }
            x = cx + (vertical ? cross_start + off : main_start);
            y = cy + (vertical ? main_start : cross_start + off);
            width = vertical ? cross : work->size[j];
            height = vertical ? work->size[j] : !cross_auto && align == WE_PACK_STRETCH && child->height < 0 && !child->height_percent ? cross : -1;
            if (vertical && wrap && (x - cx + width > cw || width < 0)) {
                diag(d, "column wrap width exceeded", work->node[j]);
                return main >= 0 ? main : 0;
            }
            layout_area(d, work->node[j], limit_coord(x), limit_coord(y), width, height, width, depth + 1);
            /* Export: a baseline group of the first/last line wins, else the
             * main-start item of the first line or main-end item of the last. */
            if (!vertical && !cross_auto && k == 0 && align == WE_PACK_BASELINE && group_first == WE_NO_BASELINE)
                group_first = limit_coord(y + work->baseline[j]);
            if (!vertical && !cross_auto && k == lines - 1 && align == WE_PACK_LAST_BASELINE)
                group_last = limit_coord(y + work->baseline[j]);
            if (k == 0 && j == line->first) item_first = item_baseline(d, work->node[j], 0);
            if (k == lines - 1 && j == line->end - 1) item_last = item_baseline(d, work->node[j], 1);
        }
        extent = limit_coord(extent + line->cross + (k + 1 < lines ? cross_gap : 0));
        d->perf.flex_lines++;
    }
    if (vertical) {
        item_first = item_baseline(d, work->node[0], 0);
        item_last = item_baseline(d, work->node[count - 1], 1);
    }
    layout_first[idx] = (int16_t)(group_first != WE_NO_BASELINE ? group_first : item_first);
    layout_last[idx] = (int16_t)(group_last != WE_NO_BASELINE ? group_last : item_last);
    if (!vertical) return cross_total + (cross_available > cross_total && n->align_content == WE_PACK_STRETCH ? cross_available - cross_total : 0);
    if (main >= 0) return main;
    return work->lines[0].used;
}

static void export_items(we_doc_t *d, int idx, const we_track_work_t *work, int count) {
    if (count <= 0) return;
    layout_first[idx] = (int16_t)item_baseline(d, work->node[0], 0);
    layout_last[idx] = (int16_t)item_baseline(d, work->node[count - 1], 1);
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
            export_items(d, idx, work, k);
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
        export_items(d, idx, work, count);
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
        export_items(d, idx, work, count);
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

static int table_column_tag(const we_node_t *n) {
    return w_eq(n->tag, "col") ? 1 : w_eq(n->tag, "colgroup") ? 2 : 0;
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
        } else if (report && !table_column_tag(n)) diag(d, "table child refused", c);
    }
    return count;
}

/* Occupancy is one bit per column. A rowspan cell blocks those columns on
 * the following rows. A span past the last row is clamped. Returns -1 when
 * the table must not be painted. */
static int table_assign(we_doc_t *d, int idx, int *rows, int nr, int depth, int report) {
    we_table_slot_t *slot = &table_slot[depth];
    int r, n, cols = 0;
    w_zero(slot, sizeof(*slot));
    for (n = 0; n < WE_MAX_NODES; n++) slot->col[n] = -1;
    for (r = 0; r < nr && r < WE_GRID_ROWS_MAX; r++) {
        int c, next = 0;
        for (c = d->nodes[rows[r]].first; c >= 0; c = d->nodes[c].next) {
            we_node_t *cell = &d->nodes[c];
            int span, rs, col, rr, mask;
            if (cell->display == WE_DISP_NONE || cell->absolute) continue;
            if (cell->display != WE_DISP_CELL) {
                if (report) diag(d, "table cell refused", c);
                continue;
            }
            if (cell->span_refused) {
                if (report) diag(d, "table refused after span", idx);
                return -1;
            }
            span = cell->colspan < 1 ? 1 : cell->colspan;
            rs = cell->rowspan < 1 ? 1 : cell->rowspan;
            if (rs > WE_TRACKS_MAX) rs = WE_TRACKS_MAX;
            if (r + rs > nr) {
                if (report && cell->rowspan != (uint8_t)(nr - r)) diag(d, "table rowspan clamped", c);
                rs = nr - r;
                if (rs < 1) rs = 1;
            }
            while (next < WE_TRACKS_MAX && (slot->occ[r] & (uint8_t)(1u << next))) next++;
            col = next;
            if (col + span > WE_TRACKS_MAX) {
                if (report) diag(d, "table columns exceed limit", idx);
                return -1;
            }
            mask = ((1 << span) - 1) << col;
            for (rr = 0; rr < rs && r + rr < WE_GRID_ROWS_MAX; rr++)
                slot->occ[r + rr] = (uint8_t)(slot->occ[r + rr] | (uint8_t)mask);
            slot->col[c] = col;
            if (col + span > cols) cols = col + span;
            next = col + span;
        }
    }
    slot->cols = cols;
    return cols;
}

/* Lot 61: border-collapse shares cell borders. Lot 63: adjacent cells
 * overlap by the band of their grid line (table_gaps); the resolved line
 * of each edge paints once inside that band (collapse_paint). */
static int table_spacing_axis(const we_node_t *n, int vertical) {
    int spacing;
    if (n->border_collapse) return 0;
    spacing = vertical ? n->border_spacing_y : n->border_spacing;
    if (spacing < 0) return 0;
    if (spacing > WE_SPACING_MAX) return WE_SPACING_MAX;
    return spacing;
}

static int table_spacing_of(const we_node_t *n) {
    return table_spacing_axis(n, 0);
}

static void table_gaps(const we_doc_t *d, int idx, const int *rows, int nr, int depth);

static int table_spacing_y_of(const we_node_t *n) {
    return table_spacing_axis(n, 1);
}

static int table_rowspan_of(const we_node_t *cell, int row, int nr) {
    int rs = cell->rowspan < 1 ? 1 : cell->rowspan;
    if (rs > WE_TRACKS_MAX) rs = WE_TRACKS_MAX;
    if (row + rs > nr) rs = nr - row;
    return rs < 1 ? 1 : rs;
}

/* Intrinsic pass: a column subgrid contributes its items, not itself. */
static void subgrid_intrinsic(we_doc_t *d, const we_node_t *g, int s, int start, int gap, we_grid_work_t *work) {
    we_track_work_t *items = &track_work[1];
    we_grid_work_t *inner = &grid_work[1];
    int count = collect_items(d, s, items), k, pass;
    w_zero(inner, sizeof(*inner));
    grid_place(d, items, inner, count, d->nodes[s].grid_cols, 0);
    for (pass = 0; pass < 2; pass++)
        for (k = 0; k < count; k++) {
            const we_node_t *child = &d->nodes[items->node[k]];
            int extra = node_border_extra(child) + child->margin[1] + child->margin[3], low, high;
            if (inner->row[k] < 0 || (child->grid_col_span == 1) != (pass == 0)) continue;
            extra += subgrid_item_extra(&d->nodes[s], 0, inner->col[k], child->grid_col_span, gap);
            intrinsic_contribution(child, &low, &high);
            grid_span_minimum(g, 0, start + inner->col[k], child->grid_col_span, constrained_width(child, low, 0) + extra, gap, -1, work->column);
            grid_span_minimum(g, 0, start + inner->col[k], child->grid_col_span, constrained_width(child, high, 0) + extra, gap, -1, work->column_x);
        }
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
        if (n->area_cols > cols) cols = n->area_cols;
        grid_place(d, items, work, count, cols, 0);
        if (n->grid_column_flow) for (k = 0; k < count; k++) {
            int end = work->col[k] + d->nodes[items->node[k]].grid_col_span;
            if (work->col[k] >= 0 && end > cols) cols = end;
        }
    } else {
        int r, placed;
        rows = collect_table_rows(d, idx, items->node, 0);
        placed = table_assign(d, idx, items->node, rows, 0, 0);
        count = 0;
        cols = placed < 0 ? 0 : placed;
        if (placed < 0) {
            int ncell;
            for (ncell = 0; ncell < WE_MAX_NODES; ncell++) table_slot[0].col[ncell] = -1;
            for (r = 0; r < rows; r++) {
                int c, column = 0;
                for (c = d->nodes[items->node[r]].first; c >= 0; c = d->nodes[c].next)
                    if (d->nodes[c].display == WE_DISP_CELL && !d->nodes[c].absolute) column += d->nodes[c].colspan;
                if (column > cols) cols = column;
            }
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
                int start = grid ? work->col[r] : (table_slot[0].col[c] >= 0 ? table_slot[0].col[c] : column);
                if (grid && pass == 0 && start >= 0 && subgrid_on(c, 0)) {
                    subgrid_intrinsic(d, n, c, start, gap, work);
                    break;
                }
                if (grid && subgrid_on(c, 0)) break;
                if (child->display != WE_DISP_NONE && !child->absolute && (grid || child->display == WE_DISP_CELL)) {
                    if ((span == 1) == (pass == 0) && start >= 0 && start + span <= cols) {
                        int extra = node_border_extra(child) + child->margin[1] + child->margin[3];
                        int low, high;
                        intrinsic_contribution(child, &low, &high);
                        if (grid) {
                            grid_span_minimum(n, 0, start, span, constrained_width(child, low, 0) + extra, gap, -1, work->column);
                            grid_span_minimum(n, 0, start, span, constrained_width(child, high, 0) + extra, gap, -1, work->column_x);
                        } else {
                            intrinsic_track_request(n, cols, start, span, constrained_width(child, low, 0) + extra, gap, 0, work->column);
                            intrinsic_track_request(n, cols, start, span, constrained_width(child, high, 0) + extra, gap, 0, work->column_x);
                        }
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
    if (!grid && cols > 1) {
        table_gaps(d, idx, items->node, rows, 0);
        for (k = 1; k < cols; k++) {
            *minimum = limit_coord(*minimum + table_slot[0].gap_x[k]);
            *maximum = limit_coord(*maximum + table_slot[0].gap_x[k]);
        }
    }
}

/* Column widths share one planner with paint. Rowspan occupies later rows.
 * border-spacing is removed from the distributable width; collapse forces 0. */
static int table_tracks(we_doc_t *d, int idx, int width, int depth, int *widths) {
    int *rows = track_work[depth].node;
    int nr = 0, c, r, cols, col, inner;
    int requested[WE_TRACKS_MAX], sum = 0, auto_cols = 0;
    nr = collect_table_rows(d, idx, rows, 1);
    cols = table_assign(d, idx, rows, nr, depth, 1);
    if (cols < 0) return 0;
    for (col = 0; col < WE_TRACKS_MAX; col++) requested[col] = 0;
    table_gaps(d, idx, rows, nr, depth);
    for (r = 0; r < nr; r++) {
        for (c = d->nodes[rows[r]].first; c >= 0; c = d->nodes[c].next) {
            we_node_t *cell = &d->nodes[c];
            int want, column;
            if (cell->display != WE_DISP_CELL || cell->absolute || table_slot[depth].col[c] < 0) continue;
            column = table_slot[depth].col[c];
            want = preferred_outer(cell, width, d->nodes[idx].width_kind == WE_SIZE_MIN_CONTENT ? cell->min_content :
                                    d->nodes[idx].width_kind ? cell->max_content : 0);
            if (cell->colspan == 1 && (d->nodes[idx].width_kind || cell->width >= 0 || cell->width_pct > 0 || cell->min_width >= 0 || cell->min_pct > 0))
                if (want > requested[column]) requested[column] = want;
        }
    }
    for (r = 0; r < nr; r++) {
        for (c = d->nodes[rows[r]].first; c >= 0; c = d->nodes[c].next) {
            const we_node_t *cell = &d->nodes[c];
            int span, column, current = 0, wanted, k, deficit;
            if (cell->display != WE_DISP_CELL || cell->absolute || table_slot[depth].col[c] < 0) continue;
            span = cell->colspan < 1 ? 1 : cell->colspan;
            column = table_slot[depth].col[c];
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
        }
    }
    if (!cols && !nr) return 0;
    if (!cols) cols = 1;
    inner = width;
    for (col = 1; col < cols; col++) inner -= table_slot[depth].gap_x[col];
    if (inner < 0) inner = 0;
    for (col = 0; col < cols; col++) {
        sum += requested[col];
        if (!requested[col]) auto_cols++;
    }
    for (col = 0; col < cols; col++) {
        if (sum > inner) widths[col] = inner * requested[col] / sum;
        else if (!requested[col]) widths[col] = auto_cols ? (inner - sum) / auto_cols : 0;
        else widths[col] = requested[col];
    }
    /* A definite table width is exhausted exactly. Extra width goes to
     * automatic columns, or equally to all columns when all are fixed.
     * Spacing stays outside the columns. */
    {
        int assigned = 0, remaining, receivers;
        for (col = 0; col < cols; col++) assigned += widths[col];
        remaining = inner - assigned;
        receivers = sum > inner ? cols : auto_cols ? auto_cols : cols;
        for (col = 0; col < cols && remaining > 0; col++) {
            int take;
            if (sum <= inner && auto_cols && requested[col]) continue;
            take = remaining / receivers + (remaining % receivers != 0);
            widths[col] += take;
            remaining -= take;
            receivers--;
        }
    }
    return nr;
}

static int table_origin(const int *sizes, int index, const int8_t *gaps) {
    int k, at = 0;
    for (k = 0; k < index; k++) at = limit_coord(at + sizes[k] + gaps[k + 1]);
    return at;
}

static int table_span_size(const int *sizes, int index, int span, const int8_t *gaps) {
    int k, total = 0;
    if (span < 1) span = 1;
    for (k = 0; k < span; k++) total = limit_coord(total + sizes[index + k] + (k ? gaps[index + k] : 0));
    return total;
}

/* Row heights match paint. A rowspan taller than the rows it covers grows
 * the last of those rows. Single-row cells do not stretch their own box. */
static int table_row_heights(we_doc_t *d, int idx, int depth, const int *widths, int nr) {
    we_table_slot_t *slot = &table_slot[depth];
    int *rows = track_work[depth].node;
    int r, c, total = 0;
    (void)idx;
    for (r = 0; r < nr; r++) {
        int high = d->nodes[rows[r]].height > 0 ? d->nodes[rows[r]].height : 0;
        if (r < WE_GRID_ROWS_MAX) slot->ascent[r] = slot->descent[r] = 0;
        for (c = d->nodes[rows[r]].first; c >= 0; c = d->nodes[c].next) {
            const we_node_t *cell = &d->nodes[c];
            int h, width;
            if (cell->display != WE_DISP_CELL || cell->absolute || slot->col[c] < 0) continue;
            int spanning = table_rowspan_of(cell, r, nr) != 1;
            if (spanning && cell->valign != WE_VALIGN_BASELINE) continue;
            width = table_span_size(widths, slot->col[c], cell->colspan, slot->gap_x);
            h = measure_area_height(d, c, width, depth + 1);
            if (h > high && !spanning) high = h;
            /* Lot 61: a row-spanning baseline cell aligns in its first row
             * (CSS 2.1 17.5.3); its height is checked over the span below. */
            if (cell->valign == WE_VALIGN_BASELINE && r < WE_GRID_ROWS_MAX) {
                int real, base = baseline_get(d, c, width, depth + 1, 0, &real);
                if (!real) base = limit_coord(h - cell->margin[2] - box_edge(cell, 2) - cell->padding[2]);
                slot->base[c] = (int16_t)base;
                if (base > slot->ascent[r]) slot->ascent[r] = base;
                if (!spanning && h - base > slot->descent[r]) slot->descent[r] = h - base;
            }
        }
        if (r < WE_GRID_ROWS_MAX && slot->ascent[r] + slot->descent[r] > high) high = slot->ascent[r] + slot->descent[r];
        slot->height[r] = high;
    }
    for (r = 0; r < nr; r++) {
        for (c = d->nodes[rows[r]].first; c >= 0; c = d->nodes[c].next) {
            const we_node_t *cell = &d->nodes[c];
            int rs, sum, h;
            if (cell->display != WE_DISP_CELL || cell->absolute || slot->col[c] < 0) continue;
            rs = table_rowspan_of(cell, r, nr);
            if (rs < 2) continue;
            sum = table_span_size(slot->height, r, rs, slot->gap_y);
            h = measure_area_height(d, c, table_span_size(widths, slot->col[c], cell->colspan, slot->gap_x), depth + 1);
            if (cell->valign == WE_VALIGN_BASELINE && r < WE_GRID_ROWS_MAX) h = limit_coord(h + slot->ascent[r] - slot->base[c]);
            if (h > sum) slot->height[r + rs - 1] = limit_coord(slot->height[r + rs - 1] + h - sum);
        }
    }
    for (r = 0; r < nr; r++) total = limit_coord(total + slot->height[r] + (r ? slot->gap_y[r] : 0));
    return total;
}

static int measure_table(we_doc_t *d, int idx, int width, int depth) {
    int widths[WE_TRACKS_MAX], nr = table_tracks(d, idx, width, depth, widths);
    if (!nr) return 0;
    return table_row_heights(d, idx, depth, widths, nr);
}

/* Shift in-flow descendants inside a cell that is taller than its content.
 * The cell border box stays put. Top alignment does not move anything. */
static void shift_cell(we_doc_t *d, int idx, int first_item, int shift);
static void valign_cell(we_doc_t *d, int idx, int first_item) {
    we_node_t *cell = &d->nodes[idx];
    int top, inner, bottom, used, shift, i;
    if (cell->valign == WE_VALIGN_TOP || first_item < 0) return;
    top = cell->y + box_edge(cell, 0) + cell->padding[0];
    inner = cell->h - vertical_extra(cell);
    if (inner < 0) inner = 0;
    bottom = top;
    for (i = idx + 1; i < (int)d->nnodes; i++) {
        int ancestor = i;
        if (d->nodes[i].absolute || d->nodes[i].display == WE_DISP_NONE) continue;
        while (ancestor >= 0 && ancestor != idx) ancestor = d->nodes[ancestor].parent;
        if (ancestor != idx) continue;
        if (d->nodes[i].y + d->nodes[i].h > bottom) bottom = d->nodes[i].y + d->nodes[i].h;
    }
    used = bottom - top;
    if (used < 0 || used >= inner) return;
    shift = cell->valign == WE_VALIGN_MIDDLE ? (inner - used) / 2 : inner - used;
    shift_cell(d, idx, first_item, shift);
}

static void shift_cell(we_doc_t *d, int idx, int first_item, int shift) {
    int i;
    if (shift <= 0) return;
    if (layout_first[idx] != WE_NO_BASELINE) layout_first[idx] = (int16_t)limit_coord(layout_first[idx] + shift);
    if (layout_last[idx] != WE_NO_BASELINE) layout_last[idx] = (int16_t)limit_coord(layout_last[idx] + shift);
    for (i = idx + 1; i < (int)d->nnodes; i++) {
        int ancestor = i;
        while (ancestor >= 0 && ancestor != idx) ancestor = d->nodes[ancestor].parent;
        if (ancestor == idx) {
            d->nodes[i].y = (int16_t)limit_coord(d->nodes[i].y + shift);
            if (static_set[i]) static_y[i] = (int16_t)limit_coord(static_y[i] + shift);
        }
    }
    for (i = first_item; i < (int)d->nitems; i++) {
        int owner = d->item_owner[i], ancestor = owner;
        if (owner == idx) continue;
        while (ancestor >= 0 && ancestor != idx) ancestor = d->nodes[ancestor].parent;
        if (ancestor == idx) d->items[i].y = (int16_t)limit_coord(d->items[i].y + shift);
    }
}

/* One resolved edge: width 0..8, style code, colour. none gives width 0;
 * a collapsed table keeps its moved width in edge_w (border is 0). */
static void edge_of(const we_node_t *n, int side, int *w, int *style, uint32_t *color) {
    *w = n->edge_w[side] >= 0 ? n->edge_w[side] : (n->border > 8 ? 8 : n->border);
    *style = n->edge_style[side] ? n->edge_style[side] : n->border_style;
    *color = (n->edge_color_set >> side) & 1 ? n->edge_color[side] : n->border_color;
    if (*w < 0) *w = 0;
    if (*style == WE_BS_NONE || !*style) *w = 0;      /* lot 64: no style is none */
}

static int edges_set(const we_node_t *n) {
    int side;
    if (n->edge_color_set) return 1;
    for (side = 0; side < 4; side++) if (n->edge_w[side] >= 0 || n->edge_style[side]) return 1;
    return 0;
}

static int table_group(const we_node_t *n) {
    return w_eq(n->tag, "tbody") || w_eq(n->tag, "thead") || w_eq(n->tag, "tfoot");
}

/* Lot 61: collapsed border resolution (CSS 2.1 17.6.2.1) on the table
 * grid. Every grid edge segment takes the winner among the cells on both
 * sides, their rows, row groups and, on the outside, the table: hidden
 * suppresses it, then the wider edge, then style precedence, then origin
 * cell > row > row group > column > column group > table, then the cell
 * above or to the left. The winner paints at B/2 - w/2 in the band of
 * width B of its grid line (lot 63, table_gaps); equal neighbours along a line
 * are merged. Vertical lines paint after horizontal ones, so a corner shows
 * the vertical winner. Cell content insets follow the same split. */
typedef struct { int w, style, hidden; uint32_t color; } we_edge_pick_t;
static int16_t collapse_map[WE_GRID_ROWS_MAX][WE_TRACKS_MAX];

static void edge_consider(const we_doc_t *d, we_edge_pick_t *best, int node, int side) {
    int w, style;
    uint32_t color;
    if (node < 0) return;
    edge_of(&d->nodes[node], side, &w, &style, &color);
    if (style == WE_BS_HIDDEN) { best->hidden = 1; return; }
    if (w <= 0) return;
    /* Candidates arrive cell, row, row group, table, upper/left cell
     * first, so keeping the earlier one on a full tie applies origin. */
    if (w > best->w || (w == best->w && border_style_rank(style) > border_style_rank(best->style))) {
        best->w = w; best->style = style; best->color = color;
    }
}

static int row_group_of(const we_doc_t *d, int row) {
    int p = d->nodes[row].parent;
    return p >= 0 && table_group(&d->nodes[p]) ? p : -1;
}

/* One border line of width w and length len at (off, from) for a vertical
 * line, (from, off) for a horizontal one. */
static void edge_rect(we_doc_t *d, int idx, uint32_t color, int vertical, int off, int from, int w, int len) {
    if (w <= 0 || len <= 0) return;
    if (vertical) (void)emit(d, idx, OS_FB_WEB_RECT, off, from, w, len, color);
    else (void)emit(d, idx, OS_FB_WEB_RECT, from, off, len, w, color);
}

/* Lot 61 border styles from RECT items only, bounded per line:
 * dashed: dashes of max(2w, 3) px with equal gaps; dotted: w x w dots
 * with w gaps; at most WE_DASH_MAX pieces, the period grows past that.
 * double (w >= 3): two lines of (w + 1) / 3 px around the gap. Lot 62:
 * the 3D styles paint two shaded halves (collapsed lines) or frames. */
#define WE_DASH_MAX 8

/* Lot 62 3D shades: dark keeps two thirds of each channel, light moves a
 * third of the way to white; the alpha byte is kept. */
static uint32_t shade_3d(uint32_t color, int light) {
    uint32_t out = color & 0xFF000000u;
    int k;
    for (k = 0; k < 24; k += 8) {
        int c = (int)((color >> k) & 255u);
        c = light ? c + (255 - c) / 3 : c * 2 / 3;
        out |= (uint32_t)c << k;
    }
    return out;
}

static void edge_paint(we_doc_t *d, int idx, int style, uint32_t color, int vertical, int off, int from, int w, int len) {
    if (style == 5 || style == 6) {
        int piece = style == 5 ? (2 * w > 3 ? 2 * w : 3) : w, period = 2 * piece, at;
        if (len > WE_DASH_MAX * period) period = (len + WE_DASH_MAX - 1) / WE_DASH_MAX;
        if (piece > period) piece = period;
        for (at = 0; at < len; at += period)
            edge_rect(d, idx, color, vertical, off, from + at, w, at + piece > len ? len - at : piece);
        return;
    }
    if (style == 4 && w >= 3) {
        int t = (w + 1) / 3;
        edge_rect(d, idx, color, vertical, off, from, t, len);
        edge_rect(d, idx, color, vertical, off + w - t, from, t, len);
        return;
    }
    if (style >= 7 && style <= 10) {
        /* Collapsed model (CSS 2.1 17.6.2): inset draws as ridge, outset
         * as groove. Groove: the top or left half dark, the other light. */
        int groove = style == 8 || style == 9, half = w / 2;
        edge_rect(d, idx, shade_3d(color, !groove), vertical, off, from, half, len);
        edge_rect(d, idx, shade_3d(color, groove), vertical, off + half, from, w - half, len);
        return;
    }
    edge_rect(d, idx, color, vertical, off, from, w, len);
}

/* Lot 62: separate 3D borders as two nested frames (groove and ridge) or
 * one (inset and outset). Each frame paints its top and left sides in one
 * shade and its right and bottom sides in the other; the top-left corner
 * belongs to the top side, the other corners to the right and bottom
 * sides (no diagonal joins). */
static void frame_sides(we_doc_t *d, int idx, uint32_t top_left, uint32_t bottom_right, int x, int y, int w, int h, int t) {
    if (t <= 0 || w <= 0 || h <= 0) return;
    if (2 * t > w) t = w / 2 > 0 ? w / 2 : 1;
    if (2 * t > h) t = h / 2 > 0 ? h / 2 : 1;
    edge_rect(d, idx, top_left, 0, y, x, t, w - t);
    edge_rect(d, idx, top_left, 1, x, y + t, t, h - 2 * t);
    edge_rect(d, idx, bottom_right, 1, x + w - t, y, t, h - t);
    edge_rect(d, idx, bottom_right, 0, y + h - t, x, t, w);
}

static void frame_3d(we_doc_t *d, int idx, int style, uint32_t color, int x, int y, int w, int h, int t) {
    uint32_t dark = shade_3d(color, 0), light = shade_3d(color, 1);
    int outer = t / 2;
    if (style == 8) { frame_sides(d, idx, light, dark, x, y, w, h, t); return; }    /* outset */
    if (style == 10) { frame_sides(d, idx, dark, light, x, y, w, h, t); return; }   /* inset */
    /* groove: dark then light on the top-left; ridge: the reverse */
    frame_sides(d, idx, style == 9 ? dark : light, style == 9 ? light : dark, x, y, w, h, outer);
    frame_sides(d, idx, style == 9 ? light : dark, style == 9 ? dark : light, x + outer, y + outer, w - 2 * outer, h - 2 * outer, t - outer);
}

/* <col>/<colgroup> origins per column: [0] the col, [1] its colgroup. A
 * colgroup without col children covers span columns itself. */
static int16_t collapse_col[2][WE_TRACKS_MAX];

static void collapse_columns(const we_doc_t *d, int idx) {
    int c, k = 0, j;
    for (j = 0; j < WE_TRACKS_MAX; j++) collapse_col[0][j] = collapse_col[1][j] = -1;
    for (c = d->nodes[idx].first; c >= 0 && k < WE_TRACKS_MAX; c = d->nodes[c].next) {
        int kind = table_column_tag(&d->nodes[c]), g, any = 0;
        if (kind == 1) {
            for (j = 0; j < (d->nodes[c].colspan > 1 ? d->nodes[c].colspan : 1) && k < WE_TRACKS_MAX; j++) collapse_col[0][k++] = (int16_t)c;
        } else if (kind == 2) {
            for (g = d->nodes[c].first; g >= 0; g = d->nodes[g].next) {
                if (table_column_tag(&d->nodes[g]) != 1) continue;
                any = 1;
                for (j = 0; j < (d->nodes[g].colspan > 1 ? d->nodes[g].colspan : 1) && k < WE_TRACKS_MAX; j++) {
                    collapse_col[1][k] = (int16_t)c;
                    collapse_col[0][k++] = (int16_t)g;
                }
            }
            if (!any)
                for (j = 0; j < (d->nodes[c].colspan > 1 ? d->nodes[c].colspan : 1) && k < WE_TRACKS_MAX; j++) collapse_col[1][k++] = (int16_t)c;
        }
    }
}

static int collapse_build(const we_doc_t *d, int idx, const int *rows, int nr, int depth) {
    const we_table_slot_t *slot = &table_slot[depth];
    int cols = slot->cols, r, k;
    if (nr > WE_GRID_ROWS_MAX) nr = WE_GRID_ROWS_MAX;
    for (r = 0; r < nr; r++) for (k = 0; k < WE_TRACKS_MAX; k++) collapse_map[r][k] = -1;
    for (r = 0; r < nr; r++) {
        int c;
        for (c = d->nodes[rows[r]].first; c >= 0; c = d->nodes[c].next) {
            const we_node_t *cell = &d->nodes[c];
            int span, rs, rr, kk;
            if (cell->display != WE_DISP_CELL || cell->absolute || slot->col[c] < 0) continue;
            span = cell->colspan < 1 ? 1 : cell->colspan;
            rs = table_rowspan_of(cell, r, nr);
            for (rr = r; rr < r + rs && rr < nr; rr++)
                for (kk = slot->col[c]; kk < slot->col[c] + span && kk < cols; kk++) collapse_map[rr][kk] = (int16_t)c;
        }
    }
    collapse_columns(d, idx);
    return cols;
}

/* Segment of horizontal line r (0..nr) over column k. Origin order:
 * cells, rows, row groups, columns, column groups, table. */
static void collapse_h(const we_doc_t *d, int idx, const int *rows, int nr, int r, int k, we_edge_pick_t *e) {
    int above = r > 0 ? collapse_map[r - 1][k] : -1, below = r < nr ? collapse_map[r][k] : -1;
    int gup = r > 0 ? row_group_of(d, rows[r - 1]) : -1, gdown = r < nr ? row_group_of(d, rows[r]) : -1;
    w_zero(e, sizeof(*e));
    if (above >= 0 && above == below) return;
    edge_consider(d, e, above, 2);
    edge_consider(d, e, below, 0);
    if (r > 0) edge_consider(d, e, rows[r - 1], 2);
    if (r < nr) edge_consider(d, e, rows[r], 0);
    if (gup != gdown) { edge_consider(d, e, gup, 2); edge_consider(d, e, gdown, 0); }
    if (r == 0 || r == nr) {
        edge_consider(d, e, collapse_col[0][k], r ? 2 : 0);
        edge_consider(d, e, collapse_col[1][k], r ? 2 : 0);
        edge_consider(d, e, idx, r ? 2 : 0);
    }
}

/* Segment of vertical line k (0..cols) over row r. */
static void collapse_v(const we_doc_t *d, int idx, const int *rows, int cols, int r, int k, we_edge_pick_t *e) {
    int left = k > 0 ? collapse_map[r][k - 1] : -1, right = k < cols ? collapse_map[r][k] : -1;
    int g = row_group_of(d, rows[r]);
    int cl = k > 0 ? collapse_col[0][k - 1] : -1, cr = k < cols ? collapse_col[0][k] : -1;
    int gl = k > 0 ? collapse_col[1][k - 1] : -1, gr = k < cols ? collapse_col[1][k] : -1;
    w_zero(e, sizeof(*e));
    if (left >= 0 && left == right) return;
    edge_consider(d, e, left, 1);
    edge_consider(d, e, right, 3);
    if (k == 0 || k == cols) { edge_consider(d, e, rows[r], k ? 1 : 3); edge_consider(d, e, g, k ? 1 : 3); }
    if (cl != cr) { edge_consider(d, e, cl, 1); edge_consider(d, e, cr, 3); }
    if (gl != gr) { edge_consider(d, e, gl, 1); edge_consider(d, e, gr, 3); }
    if (k == 0 || k == cols) edge_consider(d, e, idx, k ? 1 : 3);
}

/* The resolved line of width w sits at B/2 - w/2 in its band of width B,
 * so each side of the grid line (at B/2) holds half of it; an odd pixel
 * goes to the cell after the line (right or below). */
static int edge_offset(int b, int w) {
    return b / 2 - w / 2;
}

static void edge_emit(we_doc_t *d, int idx, const we_edge_pick_t *e, int vertical, int band, int from, int len, int b) {
    int off = band + edge_offset(b, e->w);
    if (e->hidden || e->w <= 0 || len <= 0) return;
    edge_paint(d, idx, e->style, e->color, vertical, off, from, e->w, len);
}

static int pick_w(const we_edge_pick_t *e) {
    return e->hidden ? 0 : e->w;
}

/* Lot 63: every grid line of a collapsed table has its own band, the
 * widest resolved segment along it (0 when all are hidden or none); the
 * tracks on both sides overlap by that band. Separate tables keep
 * border-spacing between tracks. */
static void table_gaps(const we_doc_t *d, int idx, const int *rows, int nr, int depth) {
    we_table_slot_t *slot = &table_slot[depth];
    int r, k, cols = slot->cols;
    we_edge_pick_t e;
    if (nr > WE_GRID_ROWS_MAX) nr = WE_GRID_ROWS_MAX;
    if (cols > WE_TRACKS_MAX) cols = WE_TRACKS_MAX;
    /* the spacing of a collapsed table is 0 */
    for (k = 0; k <= WE_TRACKS_MAX; k++) slot->gap_x[k] = (int8_t)table_spacing_of(&d->nodes[idx]);
    for (r = 0; r <= WE_GRID_ROWS_MAX; r++) slot->gap_y[r] = (int8_t)table_spacing_y_of(&d->nodes[idx]);
    if (!d->nodes[idx].border_collapse || nr <= 0 || cols <= 0) return;
    (void)collapse_build(d, idx, rows, nr, depth);
    for (r = 0; r <= nr; r++)
        for (k = 0; k < cols; k++) {
            collapse_h(d, idx, rows, nr, r, k, &e);
            if (pick_w(&e) > -slot->gap_y[r]) slot->gap_y[r] = (int8_t)-pick_w(&e);
        }
    for (k = 0; k <= cols; k++)
        for (r = 0; r < nr; r++) {
            collapse_v(d, idx, rows, cols, r, k, &e);
            if (pick_w(&e) > -slot->gap_x[k]) slot->gap_x[k] = (int8_t)-pick_w(&e);
        }
}

/* The band of grid line k of a collapsed table. */
static int line_band(const int8_t *gaps, int k) {
    return -gaps[k];
}

static int edge_same(const we_edge_pick_t *a, const we_edge_pick_t *b) {
    int va = !a->hidden && a->w > 0, vb = !b->hidden && b->w > 0;
    if (va != vb) return 0;
    return !va || (a->w == b->w && a->color == b->color && a->style == b->style);
}

static void collapse_paint(we_doc_t *d, int idx, const int *rows, int nr, const int *widths, int x, int depth) {
    we_table_slot_t *slot = &table_slot[depth];
    int cols, r, k;
    if (nr > WE_GRID_ROWS_MAX) nr = WE_GRID_ROWS_MAX;
    if (d->nodes[idx].collapse_w <= 0 || nr <= 0 || slot->cols <= 0) return;
    cols = collapse_build(d, idx, rows, nr, depth);
    /* Horizontal lines 0..nr, one segment per column. */
    for (r = 0; r <= nr; r++) {
        const we_node_t *ref = &d->nodes[rows[r < nr ? r : nr - 1]];
        int b = line_band(slot->gap_y, r), band = r < nr ? ref->y : ref->y + ref->h - b, start = 0;
        we_edge_pick_t run, e;
        w_zero(&run, sizeof(run));
        for (k = 0; k <= cols; k++) {
            w_zero(&e, sizeof(e));
            if (k < cols) collapse_h(d, idx, rows, nr, r, k, &e);
            if (k > 0 && (k == cols || !edge_same(&run, &e))) {
                int x0 = x + table_origin(widths, start, slot->gap_x);
                int x1 = x + table_origin(widths, k - 1, slot->gap_x) + widths[k - 1];
                edge_emit(d, idx, &run, 0, band, x0, x1 - x0, b);
                start = k;
            }
            if (k == 0 || k == start) run = e;
        }
    }
    /* Vertical lines 0..cols, one segment per row. */
    for (k = 0; k <= cols; k++) {
        int b = line_band(slot->gap_x, k);
        int band = k < cols ? x + table_origin(widths, k, slot->gap_x)
                            : x + table_origin(widths, cols - 1, slot->gap_x) + widths[cols - 1] - b;
        int start = 0;
        we_edge_pick_t run, e;
        w_zero(&run, sizeof(run));
        for (r = 0; r <= nr; r++) {
            w_zero(&e, sizeof(e));
            if (r < nr) collapse_v(d, idx, rows, cols, r, k, &e);
            if (r > 0 && (r == nr || !edge_same(&run, &e))) {
                int y0 = d->nodes[rows[start]].y;
                int y1 = d->nodes[rows[r - 1]].y + d->nodes[rows[r - 1]].h;
                edge_emit(d, idx, &run, 1, band, y0, y1 - y0, b);
                start = r;
            }
            if (r == 0 || r == start) run = e;
        }
    }
}

/* Content insets of the cells of collapsed table idx (CSS 2.1 17.6.2.1):
 * a cell box covers the whole band on each side; its content starts past
 * its half of the widest resolved line along that side. */
static void collapse_insets(we_doc_t *d, int idx) {
    int *rows = track_work[0].node, nr, cols, r;
    we_table_slot_t *slot = &table_slot[0];
    nr = collect_table_rows(d, idx, rows, 0);
    if (nr > WE_GRID_ROWS_MAX) nr = WE_GRID_ROWS_MAX;
    if (nr <= 0 || table_assign(d, idx, rows, nr, 0, 0) < 0) return;
    table_gaps(d, idx, rows, nr, 0);
    cols = collapse_build(d, idx, rows, nr, 0);
    for (r = 0; r < nr; r++) {
        int c;
        for (c = d->nodes[rows[r]].first; c >= 0; c = d->nodes[c].next) {
            we_node_t *cell = &d->nodes[c];
            int span, rs, k, rr, w[4] = { 0, 0, 0, 0 };
            we_edge_pick_t e;
            if (cell->display != WE_DISP_CELL || cell->absolute || slot->col[c] < 0) continue;
            span = cell->colspan < 1 ? 1 : cell->colspan;
            rs = table_rowspan_of(cell, r, nr);
            for (k = slot->col[c]; k < slot->col[c] + span && k < cols; k++) {
                collapse_h(d, idx, rows, nr, r, k, &e); if (pick_w(&e) > w[0]) w[0] = pick_w(&e);
                collapse_h(d, idx, rows, nr, r + rs, k, &e); if (pick_w(&e) > w[2]) w[2] = pick_w(&e);
            }
            for (rr = r; rr < r + rs && rr < nr; rr++) {
                collapse_v(d, idx, rows, cols, rr, slot->col[c], &e); if (pick_w(&e) > w[3]) w[3] = pick_w(&e);
                collapse_v(d, idx, rows, cols, rr, slot->col[c] + span, &e); if (pick_w(&e) > w[1]) w[1] = pick_w(&e);
            }
            /* each side against the band of its own grid line */
            cell->cin[0] = (uint8_t)(edge_offset(line_band(slot->gap_y, r), w[0]) + w[0]);
            cell->cin[3] = (uint8_t)(edge_offset(line_band(slot->gap_x, slot->col[c]), w[3]) + w[3]);
            cell->cin[2] = (uint8_t)(line_band(slot->gap_y, r + rs) - edge_offset(line_band(slot->gap_y, r + rs), w[2]));
            cell->cin[1] = (uint8_t)(line_band(slot->gap_x, slot->col[c] + span) - edge_offset(line_band(slot->gap_x, slot->col[c] + span), w[1]));
        }
    }
}

static int layout_table(we_doc_t *d, int idx, int x, int y, int width, int depth) {
    int widths[WE_TRACKS_MAX], nr = table_tracks(d, idx, width, depth, widths);
    int *rows = track_work[depth].node;
    we_table_slot_t *slot = &table_slot[depth];
    int r, c, cy = y;
    if (!nr) return 0;
    table_row_heights(d, idx, depth, widths, nr);
    for (r = 0; r < nr; r++) {
        we_node_t *row = &d->nodes[rows[r]];
        int high = slot->height[r], bg_slot = -1, row_base = WE_NO_BASELINE, row_aligned = 0;
        row->x = (int16_t)x;
        row->y = (int16_t)cy;
        row->w = (int16_t)width;
        row->h = (int16_t)high;
        if (row->has_bg) {
            bg_slot = (int)d->nitems;
            if (!emit(d, rows[r], OS_FB_WEB_RECT, x, cy, width, high, row->bg)) bg_slot = -1;
        }
        (void)bg_slot;
        for (c = row->first; c >= 0; c = d->nodes[c].next) {
            we_node_t *cell = &d->nodes[c];
            int span, rs, cell_w, cell_h, cx;
            if (cell->display != WE_DISP_CELL || cell->absolute || slot->col[c] < 0) continue;
            span = cell->colspan < 1 ? 1 : cell->colspan;
            rs = table_rowspan_of(cell, r, nr);
            cell_w = table_span_size(widths, slot->col[c], span, slot->gap_x);
            cx = limit_coord(x + table_origin(widths, slot->col[c], slot->gap_x));
            {
                int first_item = (int)d->nitems;
                int stretch = rs > 1 || cell->valign != WE_VALIGN_TOP;
                if (stretch) {
                    cell_h = rs > 1 ? table_span_size(slot->height, r, rs, slot->gap_y) : high;
                    layout_area(d, c, cx, cy, cell_w, cell_h, cell_w, depth + 1);
                    if (cell->valign == WE_VALIGN_BASELINE && r < WE_GRID_ROWS_MAX)
                        shift_cell(d, c, first_item, slot->ascent[r] - slot->base[c]);
                    else if (cell->valign != WE_VALIGN_TOP && cell->valign != WE_VALIGN_BASELINE)
                        valign_cell(d, c, first_item);
                } else layout_assigned(d, c, cx, cy, cell_w, depth + 1);
            }
            /* Row baseline: the shared baseline of its aligned cells, else
             * the first cell with a line box. */
            if (cell->valign == WE_VALIGN_BASELINE && r < WE_GRID_ROWS_MAX) {
                if (row_base == WE_NO_BASELINE || !row_aligned) row_base = limit_coord(cy + slot->ascent[r]);
                row_aligned = 1;
            } else if (!row_aligned && row_base == WE_NO_BASELINE && layout_first[c] != WE_NO_BASELINE)
                row_base = layout_first[c];
        }
        if (r == 0) layout_first[idx] = (int16_t)row_base;
        if (row_base != WE_NO_BASELINE) layout_last[idx] = (int16_t)row_base;
        cy = limit_coord(cy + high + (r + 1 < nr ? slot->gap_y[r + 1] : 0));
        d->perf.table_rows++;
    }
    if (d->nodes[idx].border_collapse) collapse_paint(d, idx, rows, nr, widths, x, depth);
    return cy - y;
}

static int clip_bounds(const we_doc_t *d, int owner, int *left, int *top, int *right, int *bottom) {
    int p = owner >= 0 ? d->nodes[owner].parent : -1, q = owner;
    *left = *top = -WE_LAYOUT_MAX;
    *right = *bottom = WE_LAYOUT_MAX;
    for (; p >= 0; q = p, p = d->nodes[p].parent) {
        const we_node_t *n = &d->nodes[p];
        int x, y, r, b;
        if (q >= 0 && d->nodes[q].fixed) break; /* fixed boxes escape ancestor clips */
        if (!n->overflow_hidden) continue;
        x = n->x + box_edge(n, 3);
        y = n->y + box_edge(n, 0);
        r = n->x + n->w - box_edge(n, 1);
        b = n->y + n->h - box_edge(n, 2);
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

/* Lot 61: partial glyph clipping. A glyph whose lit pixels are all inside
 * the clip stays text; whole visible glyphs are grouped into text pieces.
 * A partly visible glyph becomes RECT runs of its visible lit pixels from
 * the kernel font (bit k at x + k, bold also at x + k + 1, rows 0..7),
 * merged down identical columns. Invisible glyphs and spaces emit nothing. */
#define WE_CLIP_FRAGMENTS 64
static unsigned glyph_mask(char ch, int row, int bold) {
    unsigned bits;
    if ((unsigned char)ch < 32 || (unsigned char)ch > 126) ch = '?';
    bits = gfx_font8[(unsigned char)ch - 32u][row];
    return bold ? bits | (bits << 1) : bits;
}

static int glyph_inside(const os_fb_web_item_t *it, int k, int left, int top, int right, int bottom) {
    int row, bit, x = it->x + k * WE_CHAR_W;
    for (row = 0; row < 8; row++) {
        unsigned m = glyph_mask(it->text[k], row, it->bold);
        if (!m) continue;
        if (it->y + row < top || it->y + row >= bottom) return 0;
        for (bit = 0; bit < 9; bit++)
            if ((m >> bit) & 1u && (x + bit < left || x + bit >= right)) return 0;
    }
    return 1;
}

/* Returns the fragments written, or -1 when out would overflow. */
static int glyph_rects(const os_fb_web_item_t *it, int k, int left, int top, int right, int bottom,
                       os_fb_web_item_t *out, int used, int cap) {
    int row, bit, x = it->x + k * WE_CHAR_W, first = used;
    for (row = 0; row < 8; row++) {
        unsigned m = glyph_mask(it->text[k], row, it->bold);
        int y = it->y + row;
        if (!m || y < top || y >= bottom) continue;
        for (bit = 0; bit < 9; bit++) {
            int x0, x1, j, merged = 0;
            if (!((m >> bit) & 1u) || x + bit < left || x + bit >= right) continue;
            x0 = x + bit;
            while (bit + 1 < 9 && ((m >> (bit + 1)) & 1u) && x + bit + 1 < right) bit++;
            x1 = x + bit + 1;
            for (j = first; j < used; j++)
                if (out[j].x == x0 && out[j].w == x1 - x0 && out[j].y + out[j].h == y) { out[j].h++; merged = 1; break; }
            if (merged) continue;
            if (used >= cap) return -1;
            out[used] = *it;
            out[used].kind = OS_FB_WEB_RECT;
            out[used].text[0] = 0;
            out[used].bold = 0;
            out[used].x = (int16_t)x0; out[used].y = (int16_t)y;
            out[used].w = (uint16_t)(x1 - x0); out[used].h = 1;
            used++;
        }
    }
    return used - first;
}

static int clip_glyphs(const os_fb_web_item_t *it, int left, int top, int right, int bottom,
                       os_fb_web_item_t *out, int cap) {
    int len = w_len(it->text), k = 0, used = 0;
    while (k < len) {
        if (glyph_inside(it, k, left, top, right, bottom) && it->text[k] != ' ') {
            int end = k, j;
            while (end < len && glyph_inside(it, end, left, top, right, bottom)) end++;
            while (end > k && it->text[end - 1] == ' ') end--;
            if (used >= cap) return -1;
            out[used] = *it;
            for (j = k; j < end; j++) out[used].text[j - k] = it->text[j];
            out[used].text[end - k] = 0;
            out[used].x = (int16_t)(it->x + k * WE_CHAR_W);
            out[used].w = (uint16_t)((end - k) * WE_CHAR_W);
            used++;
            k = end;
            continue;
        }
        if (it->text[k] != ' ') {
            int n = glyph_rects(it, k, left, top, right, bottom, out, used, cap);
            if (n < 0) return -1;
            used += n;
        }
        k++;
    }
    return used;
}

/* Side k of a BORDER item as a RECT: top, bottom, then left and right
 * between them (the kernel painter strokes the same pixels). */
static void border_side(const os_fb_web_item_t *item, int k, os_fb_web_item_t *edge) {
    int thickness = item->bold ? item->bold : 1, w = item->w, h = item->h;
    if (thickness > w) thickness = w;
    if (thickness > h) thickness = h;
    *edge = *item;
    edge->kind = OS_FB_WEB_RECT;
    edge->bold = 0;
    if (k < 2) {
        edge->h = (uint16_t)thickness;
        if (k) edge->y = (int16_t)(item->y + h - thickness);
    } else {
        edge->w = (uint16_t)thickness;
        edge->y = (int16_t)(item->y + thickness);
        edge->h = (uint16_t)(h > 2 * thickness ? h - 2 * thickness : 0);
        if (k == 3) edge->x = (int16_t)(item->x + w - thickness);
    }
}

/* Clips every fragment to (left, top, right, bottom): RECT pieces are cut,
 * TEXT is cut at glyph level, BORDER splits into its visible sides.
 * Returns the new count, or -1 when the fragments overflow cap. */
static int clip_frags(os_fb_web_item_t *frag, int count, int left, int top, int right, int bottom, int cap) {
    static os_fb_web_item_t out[WE_CLIP_FRAGMENTS], pieces[WE_CLIP_FRAGMENTS];
    int i, k, used = 0;
    for (i = 0; i < count; i++) {
        os_fb_web_item_t item = frag[i];
        int n = 0;
        if (item.x >= left && item.y >= top && item.x + item.w <= right && item.y + item.h <= bottom) {
            pieces[n++] = item;
        } else if (item.kind == OS_FB_WEB_TEXT) {
            n = clip_glyphs(&item, left, top, right, bottom, pieces, WE_CLIP_FRAGMENTS);
            if (n < 0) return -1;
        } else if (item.kind == OS_FB_WEB_BORDER) {
            for (k = 0; k < 4; k++) {
                border_side(&item, k, &pieces[n]);
                if (clip_rectangle(&pieces[n], left, top, right, bottom)) n++;
            }
        } else if (clip_rectangle(&item, left, top, right, bottom)) pieces[n++] = item;
        if (used + n > cap) return -1;
        for (k = 0; k < n; k++) out[used++] = pieces[k];
    }
    for (i = 0; i < used; i++) frag[i] = out[i];
    return used;
}

/* ---------- Lot 62 round 2: rounded overflow clip ----------
 * overflow:hidden with border-radius clips to the padding-box shape (the
 * border radii reduced by the borders, CSS Backgrounds 3, 5.3): after the
 * rectangle clip, a fragment that the shape's bands do not wholly contain
 * is cut band by band (TEXT at glyph level, as for rectangles). Nested
 * rounded clips apply innermost first; a fixed box escapes them. Lot 63:
 * under a transform each rounded clip applies in its box's own space,
 * before that box's transform maps the pieces. Over WE_CLIP_FRAGMENTS the
 * item is dropped with "clipping display budget" (or "transform display
 * budget" under a transform). */
static int round_bands(const we_node_t *n, int band[][4]) {
    we_radii_t outer, r;
    we_shape_t pad;
    used_radii(n, n->w, n->h, &outer);
    if (!radii_any(&outer)) return -1;
    radii_inset(&outer, &r, box_edge(n, 0), box_edge(n, 1), box_edge(n, 2), box_edge(n, 3));
    if (!radii_any(&r)) return -1;
    shape_set(&pad, n->x + box_edge(n, 3), n->y + box_edge(n, 0), n->w - box_edge(n, 1) - box_edge(n, 3),
              n->h - box_edge(n, 0) - box_edge(n, 2), &r);
    return shape_bands(&pad, 0, band, WE_ROUND_BANDS);
}

static int bands_contain(int band[][4], int count, const os_fb_web_item_t *it) {
    int k, covered = it->y;
    for (k = 0; k < count && covered < it->y + it->h; k++) {
        if (band[k][1] <= covered) continue;
        if (band[k][0] > covered || it->x < band[k][2] || it->x + it->w > band[k][3]) return 0;
        covered = band[k][1];
    }
    return covered >= it->y + it->h;
}

/* TEXT against bands: whole visible glyphs stay text, a partly visible
 * glyph becomes RECT runs band by band. */
static int glyph_in_bands(const os_fb_web_item_t *it, int k, int band[][4], int count) {
    int row, bit, b, x = it->x + k * WE_CHAR_W;
    for (row = 0; row < 8; row++) {
        unsigned m = glyph_mask(it->text[k], row, it->bold);
        int y = it->y + row;
        if (!m) continue;
        for (b = 0; b < count && !(y >= band[b][0] && y < band[b][1]); b++) {}
        if (b == count) return 0;
        for (bit = 0; bit < 9; bit++)
            if ((m >> bit) & 1u && (x + bit < band[b][2] || x + bit >= band[b][3])) return 0;
    }
    return 1;
}

static int round_text(const os_fb_web_item_t *it, int band[][4], int count, os_fb_web_item_t *out, int cap) {
    int len = w_len(it->text), k = 0, used = 0, b, j;
    while (k < len) {
        if (it->text[k] != ' ' && glyph_in_bands(it, k, band, count)) {
            int end = k;
            while (end < len && glyph_in_bands(it, end, band, count)) end++;
            while (end > k && it->text[end - 1] == ' ') end--;
            if (used >= cap) return -1;
            out[used] = *it;
            for (j = k; j < end; j++) out[used].text[j - k] = it->text[j];
            out[used].text[end - k] = 0;
            out[used].x = (int16_t)(it->x + k * WE_CHAR_W);
            out[used].w = (uint16_t)((end - k) * WE_CHAR_W);
            used++;
            k = end;
            continue;
        }
        if (it->text[k] != ' ')
            for (b = 0; b < count; b++) {
                int n = glyph_rects(it, k, band[b][2], band[b][0], band[b][3], band[b][1], out, used, cap);
                if (n < 0) return -1;
                used += n;
            }
        k++;
    }
    return used;
}

/* The rounded clip of one box n, in n's own (untransformed) space. */
static int round_clip_node(const we_node_t *n, os_fb_web_item_t *frag, int count, int cap) {
    static os_fb_web_item_t out[WE_CLIP_FRAGMENTS], one[WE_CLIP_FRAGMENTS];
    static int band[WE_ROUND_BANDS][4];
    int bands, i, b, k, used = 0;
    if (!n->overflow_hidden || (bands = round_bands(n, band)) < 0) return count;
    for (i = 0; i < count; i++) {
        if (bands_contain(band, bands, &frag[i])) {
            out[used++] = frag[i];                 /* used <= i < count <= cap */
            continue;
        }
        if (frag[i].kind == OS_FB_WEB_TEXT) {
            int got = round_text(&frag[i], band, bands, one, WE_CLIP_FRAGMENTS);
            if (got < 0 || used + got > cap) return -1;
            for (k = 0; k < got; k++) out[used++] = one[k];
            continue;
        }
        for (b = 0; b < bands; b++) {
            int got;
            if (frag[i].y + frag[i].h <= band[b][0] || frag[i].y >= band[b][1]) continue;
            one[0] = frag[i];
            got = clip_frags(one, 1, band[b][2], band[b][0], band[b][3], band[b][1], WE_CLIP_FRAGMENTS);
            if (got < 0 || used + got > cap) return -1;
            for (k = 0; k < got; k++) out[used++] = one[k];
        }
    }
    for (i = 0; i < used; i++) frag[i] = out[i];
    return used;
}

static int round_clip(const we_doc_t *d, int owner, os_fb_web_item_t *frag, int count, int cap) {
    int p = owner >= 0 ? d->nodes[owner].parent : -1, q = owner;
    for (; p >= 0 && count > 0; q = p, p = d->nodes[p].parent) {
        if (q >= 0 && d->nodes[q].fixed) break;
        if ((count = round_clip_node(&d->nodes[p], frag, count, cap)) < 0) return -1;
    }
    return count;
}

/* ---------- Lot 62: paint-time transforms ----------
 * The layout boxes do not move (CSS Transforms 1). At paint time the items
 * of a transformed box and of its subtree are mapped by its matrix about
 * transform-origin, innermost box first, each ancestor clip applied in the
 * space of that ancestor. translate keeps items as they are; scale and
 * quarter-turn rotation map rectangles to rectangles, so TEXT becomes RECT
 * runs of its glyph pixels (kernel/gfx_font8.h) and BORDER its four sides.
 * Coordinates round to whole pixels; an item whose fragments pass
 * WE_CLIP_FRAGMENTS is dropped with "transform display budget". */
typedef struct { int a, b, c, d, e, f; } we_matrix_t;   /* a..d in 1/1000, e and f in 1/16 px */
#define WE_TF_EDGE (WE_LAYOUT_MAX * 16)

static int floor_div(int num, int den) {
    return num >= 0 ? num / den : -((-num + den - 1) / den);
}

static int div_round(int num, int den) {
    return floor_div(2 * num + den, 2 * den);
}

/* m = m * F with F = (a b; c d), translation (e, f) in whole px. */
static void matrix_mul(we_matrix_t *m, int a, int b, int c, int dd, int e, int f) {
    we_matrix_t r;
    r.a = clamp_int(div_round(m->a * a + m->b * c, 1000), -8000, 8000);
    r.b = clamp_int(div_round(m->a * b + m->b * dd, 1000), -8000, 8000);
    r.c = clamp_int(div_round(m->c * a + m->d * c, 1000), -8000, 8000);
    r.d = clamp_int(div_round(m->c * b + m->d * dd, 1000), -8000, 8000);
    r.e = clamp_int(m->e + floor_div(2 * (m->a * e + m->b * f), 125), -WE_TF_EDGE, WE_TF_EDGE);
    r.f = clamp_int(m->f + floor_div(2 * (m->c * e + m->d * f), 125), -WE_TF_EDGE, WE_TF_EDGE);
    *m = r;
}

/* sin of whole degrees in 1/1000 (lot 63). */
static int sin_deg(int deg) {
    static const int16_t quarter[91] = {
        0, 17, 35, 52, 70, 87, 105, 122, 139, 156, 174, 191, 208, 225, 242, 259, 276, 292, 309, 326, 342, 358, 375,
        391, 407, 423, 438, 454, 469, 485, 500, 515, 530, 545, 559, 574, 588, 602, 616, 629, 643, 656, 669, 682, 695,
        707, 719, 731, 743, 755, 766, 777, 788, 799, 809, 819, 829, 839, 848, 857, 866, 875, 883, 891, 899, 906, 914,
        921, 927, 934, 940, 946, 951, 956, 961, 966, 970, 974, 978, 982, 985, 988, 990, 993, 995, 996, 998, 999, 999,
        1000, 1000};
    deg %= 360;
    if (deg < 0) deg += 360;
    if (deg <= 90) return quarter[deg];
    if (deg <= 180) return quarter[180 - deg];
    if (deg <= 270) return -quarter[deg - 180];
    return -quarter[360 - deg];
}

static int tan_deg(int deg) {
    return div_round(sin_deg(deg) * 1000, sin_deg(deg + 90));
}

static void node_matrix(const we_node_t *n, we_matrix_t *m) {
    int ox = n->x + ((n->tf_origin_pct & 1) ? n->w * n->tf_origin[0] / 100 : n->tf_origin[0]);
    int oy = n->y + ((n->tf_origin_pct & 2) ? n->h * n->tf_origin[1] / 100 : n->tf_origin[1]);
    int k;
    m->a = m->d = 1000; m->b = m->c = 0; m->e = ox * 16; m->f = oy * 16;
    for (k = 0; k < n->tf_count && k < WE_TF_MAX; k++) {
        const we_transform_t *t = &n->tf[k];
        if (t->kind == WE_TF_TRANSLATE)
            matrix_mul(m, 1000, 0, 0, 1000, (t->pct & 1) ? n->w * t->x / 100 : t->x, (t->pct & 2) ? n->h * t->y / 100 : t->y);
        else if (t->kind == WE_TF_SCALE) matrix_mul(m, t->x, 0, 0, t->y, 0, 0);
        else if (t->kind == WE_TF_MATRIX) {
            /* CSS maps x' = a x + c y + e, y' = b x + d y + f */
            const we_transform_t *cd = &n->tf[k + 1], *ef = &n->tf[k + 2];
            matrix_mul(m, t->x, cd->x, t->y, cd->y, ef->x, ef->y);
            k += 2;
        }
        else if (t->kind == WE_TF_ROTATE) {                            /* clockwise, y down */
            int sn = sin_deg(t->x), cs = sin_deg(t->x + 90);
            matrix_mul(m, cs, -sn, sn, cs, 0, 0);
        } else matrix_mul(m, 1000, tan_deg(t->x), tan_deg(t->y), 1000, 0, 0);
    }
    matrix_mul(m, 1000, 0, 0, 1000, -ox, -oy);
}

/* Mapped coordinates round half up: floor(v + 1/2). */
static int map_x(const we_matrix_t *m, int x, int y) { return floor_div(floor_div(2 * (m->a * x + m->b * y), 125) + m->e + 8, 16); }
static int map_y(const we_matrix_t *m, int x, int y) { return floor_div(floor_div(2 * (m->c * x + m->d * y), 125) + m->f + 8, 16); }

/* Lot 63: a rectangle under a matrix that is not axis-aligned maps to a
 * parallelogram. It is painted as at most WE_TF_BANDS row bands of equal
 * height, each spanning the parallelogram at its middle row. Corners
 * round to the nearest half pixel and clamp to +-WE_LAYOUT_MAX / 2 px. */
#define WE_TF_BANDS 8
static int tf_bands(const we_matrix_t *m, const os_fb_web_item_t *r, os_fb_web_item_t *out, int used, int cap) {
    int px[4], py[4], k, top, bottom, rows, bands, b, last = -1;
    for (k = 0; k < 4; k++) {
        int x = r->x + ((k == 1 || k == 2) ? r->w : 0), y = r->y + (k >= 2 ? r->h : 0);
        px[k] = clamp_int(floor_div(floor_div(2 * (m->a * x + m->b * y), 125) + m->e + 4, 8), -WE_LAYOUT_MAX, WE_LAYOUT_MAX);
        py[k] = clamp_int(floor_div(floor_div(2 * (m->c * x + m->d * y), 125) + m->f + 4, 8), -WE_LAYOUT_MAX, WE_LAYOUT_MAX);
    }
    top = bottom = py[0];
    for (k = 1; k < 4; k++) { if (py[k] < top) top = py[k]; if (py[k] > bottom) bottom = py[k]; }
    top = floor_div(top + 1, 2); bottom = floor_div(bottom + 1, 2);
    rows = bottom - top;
    bands = rows < WE_TF_BANDS ? rows : WE_TF_BANDS;
    for (b = 0; b < bands; b++) {
        int ra = top + rows * b / bands, rb = top + rows * (b + 1) / bands, mid = ra + rb, lo = 0, hi = 0, hit = 0, left, right;
        for (k = 0; k < 4; k++) {
            int j = (k + 1) & 3, lo_k = py[k] < py[j] ? k : j, hi_k = lo_k == k ? j : k, x;
            if (mid < py[lo_k] || mid >= py[hi_k]) continue;
            x = px[lo_k] + floor_div((px[hi_k] - px[lo_k]) * (mid - py[lo_k]), py[hi_k] - py[lo_k]);
            if (!hit || x < lo) lo = x;
            if (!hit || x > hi) hi = x;
            hit = 1;
        }
        left = floor_div(lo + 1, 2); right = floor_div(hi + 1, 2);
        if (!hit || right <= left) continue;      /* convex: an empty band has no equal neighbours */
        if (last >= 0 && out[last].x == left && out[last].w == right - left) { out[last].h = (uint16_t)(out[last].h + rb - ra); continue; }
        if (used >= cap) return -1;
        out[used] = *r;
        out[used].kind = OS_FB_WEB_RECT;
        out[used].x = (int16_t)left; out[used].y = (int16_t)ra;
        out[used].w = (uint16_t)(right - left); out[used].h = (uint16_t)(rb - ra);
        last = used++;
    }
    return used;
}

/* Text under a matrix that is not axis-aligned stays upright, its centre
 * mapped ("transform text upright"). */
static int tf_upright_text;

static int tf_apply(const we_matrix_t *m, os_fb_web_item_t *frag, int count, int cap) {
    static os_fb_web_item_t rects[WE_CLIP_FRAGMENTS];
    int i, k, used = 0, out = 0, skewed = (m->b || m->c) && (m->a || m->d);
    if (m->a == 1000 && m->d == 1000 && !m->b && !m->c) {
        int dx = floor_div(m->e + 8, 16), dy = floor_div(m->f + 8, 16);
        for (i = 0; i < count; i++) {
            frag[i].x = (int16_t)limit_coord(frag[i].x + dx);
            frag[i].y = (int16_t)limit_coord(frag[i].y + dy);
        }
        return count;
    }
    for (i = 0; i < count; i++) {
        const os_fb_web_item_t *it = &frag[i];
        if (it->kind == OS_FB_WEB_TEXT && skewed) {
            int cx = 2 * it->x + it->w, cy = 2 * it->y + it->h;
            if (used >= cap) return -1;
            rects[used] = *it;
            rects[used].x = (int16_t)limit_coord(floor_div(floor_div(floor_div(m->a * cx + m->b * cy, 125) + m->e, 8) - it->w + 1, 2));
            rects[used].y = (int16_t)limit_coord(floor_div(floor_div(floor_div(m->c * cx + m->d * cy, 125) + m->f, 8) - it->h + 1, 2));
            used++;
            tf_upright_text = 1;
        } else if (it->kind == OS_FB_WEB_TEXT) {
            int len = w_len(it->text);
            for (k = 0; k < len; k++) {
                int n;
                if (it->text[k] == ' ') continue;
                n = glyph_rects(it, k, -WE_LAYOUT_MAX, -WE_LAYOUT_MAX, WE_LAYOUT_MAX, WE_LAYOUT_MAX, rects, used, cap);
                if (n < 0) return -1;
                used += n;
            }
        } else if (it->kind == OS_FB_WEB_BORDER) {
            for (k = 0; k < 4; k++) {
                if (used >= cap) return -1;
                border_side(it, k, &rects[used++]);
            }
        } else {
            if (used >= cap) return -1;
            rects[used++] = *it;
        }
    }
    if (skewed) {
        for (i = 0; i < used; i++) {
            if (rects[i].kind == OS_FB_WEB_TEXT) { if (out >= cap) return -1; frag[out++] = rects[i]; }
            else if ((out = tf_bands(m, &rects[i], frag, out, cap)) < 0) return -1;
        }
        return out;
    }
    for (i = 0; i < used; i++) {
        os_fb_web_item_t r = rects[i];
        int x0 = map_x(m, r.x, r.y), y0 = map_y(m, r.x, r.y);
        int x1 = map_x(m, r.x + r.w, r.y + r.h), y1 = map_y(m, r.x + r.w, r.y + r.h), t;
        if (x1 < x0) { t = x0; x0 = x1; x1 = t; }
        if (y1 < y0) { t = y0; y0 = y1; y1 = t; }
        if (x1 <= x0 || y1 <= y0) continue;
        r.x = (int16_t)limit_coord(x0); r.y = (int16_t)limit_coord(y0);
        r.w = (uint16_t)(x1 - x0 > WE_LAYOUT_MAX ? WE_LAYOUT_MAX : x1 - x0);
        r.h = (uint16_t)(y1 - y0 > WE_LAYOUT_MAX ? WE_LAYOUT_MAX : y1 - y0);
        frag[out++] = r;
    }
    return out;
}

static int transformed_chain(const we_doc_t *d, int owner) {
    int guard;
    for (guard = 0; owner >= 0 && guard <= WE_DEPTH_MAX + 1; guard++, owner = d->nodes[owner].parent) {
        if (d->nodes[owner].tf_count) return 1;
        if (d->nodes[owner].fixed) return 0;
    }
    return 0;
}

/* Lot 63: m = l * m, so that m applies first. */
static void matrix_compose(we_matrix_t *m, const we_matrix_t *l) {
    we_matrix_t r;
    r.a = clamp_int(div_round(l->a * m->a + l->b * m->c, 1000), -8000, 8000);
    r.b = clamp_int(div_round(l->a * m->b + l->b * m->d, 1000), -8000, 8000);
    r.c = clamp_int(div_round(l->c * m->a + l->d * m->c, 1000), -8000, 8000);
    r.d = clamp_int(div_round(l->c * m->b + l->d * m->d, 1000), -8000, 8000);
    r.e = clamp_int(l->e + div_round(l->a * m->e, 1000) + div_round(l->b * m->f, 1000), -WE_TF_EDGE, WE_TF_EDGE);
    r.f = clamp_int(l->f + div_round(l->c * m->e, 1000) + div_round(l->d * m->f, 1000), -WE_TF_EDGE, WE_TF_EDGE);
    *m = r;
}

/* Owner transform, then for each ancestor its clip and its transform.
 * Lot 63: transforms with no clip between them compose into one matrix,
 * applied before the next clip or at the end, so rotate(45deg) inside
 * rotate(-45deg) paints its box upright. */
static int transform_clip_item(const we_doc_t *d, const os_fb_web_item_t *item, int owner, os_fb_web_item_t *frag, int cap) {
    int count = 1, p, q, guard, pending = 0;
    we_matrix_t m, step;
    frag[0] = *item;
    if (owner < 0) return 1;
    if (d->nodes[owner].tf_count) { node_matrix(&d->nodes[owner], &m); pending = 1; }
    for (q = owner, p = d->nodes[owner].parent, guard = 0; p >= 0 && count > 0 && guard <= WE_DEPTH_MAX + 1;
         q = p, p = d->nodes[p].parent, guard++) {
        const we_node_t *n = &d->nodes[p];
        if (d->nodes[q].fixed) break;
        if (n->overflow_hidden) {
            if (pending && (count = tf_apply(&m, frag, count, cap)) < 0) return -1;
            pending = 0;
            if ((count = clip_frags(frag, count, n->x + box_edge(n, 3), n->y + box_edge(n, 0),
                                    n->x + n->w - box_edge(n, 1), n->y + n->h - box_edge(n, 2), cap)) < 0) return -1;
            if ((count = round_clip_node(n, frag, count, cap)) < 0) return -1;
        }
        if (n->tf_count) {
            node_matrix(n, &step);
            if (pending) matrix_compose(&m, &step);
            else m = step;
            pending = 1;
        }
    }
    if (pending && (count = tf_apply(&m, frag, count, cap)) < 0) return -1;
    return count;
}

/* ---------- Lot 62: text-overflow ellipsis ----------
 * In a block container with overflow hidden and text-overflow ellipsis,
 * a text run crossing the content box's end edge keeps the characters
 * that fit with "..." (three ASCII dots, the 8x8 font has no U+2026).
 * When the runs that fit end before hidden ones, the last visible run
 * ends with the dots. Runs past the edge are left to the clip. */
static int ellipsis_block(const we_doc_t *d, int owner, int *edge) {
    int o = owner, guard;
    for (guard = 0; o >= 0 && guard <= WE_DEPTH_MAX + 1; guard++, o = d->nodes[o].parent) {
        const we_node_t *n = &d->nodes[o];
        if (n->is_text || n->display == WE_DISP_INLINE) continue;
        if (!n->text_overflow || !n->overflow_hidden) return -1;
        *edge = n->x + n->w - box_edge(n, 1) - n->padding[1];
        return o;
    }
    return -1;
}

static void ellipsis_cut(os_fb_web_item_t *it, int keep_px) {
    int fit = keep_px / WE_CHAR_W, len = w_len(it->text), keep, k;
    if (fit > len) fit = len;
    if (fit < 0) fit = 0;
    keep = fit - 3 < 0 ? 0 : fit - 3;
    for (k = keep; k < fit; k++) it->text[k] = '.';
    it->text[fit] = 0;
    it->w = (uint16_t)(fit * WE_CHAR_W);
}

static void ellipsis_items(we_doc_t *d) {
    int i, j;
    for (i = 0; i < (int)d->nitems; i++) {
        os_fb_web_item_t *it = &d->items[i];
        int edge, block, crossing = 0, hidden = 0, last = -1;
        if (it->kind != OS_FB_WEB_TEXT || (block = ellipsis_block(d, d->item_owner[i], &edge)) < 0) continue;
        if (it->x >= edge || it->x + it->w <= edge) {
            if (it->x + it->w > edge) continue;
            /* a fitting run: the last one before hidden runs takes the dots */
            for (j = 0; j < (int)d->nitems; j++) {
                const os_fb_web_item_t *o = &d->items[j];
                int e2;
                if (j == i || o->kind != OS_FB_WEB_TEXT || o->y != it->y) continue;
                if (ellipsis_block(d, d->item_owner[j], &e2) != block) continue;
                if (o->x < edge && o->x + o->w > edge) crossing = 1;
                else if (o->x >= edge) hidden = 1;
                else if (o->x + o->w > it->x + it->w || (o->x + o->w == it->x + it->w && j > i)) last = j;
            }
            if (hidden && !crossing && last < 0) ellipsis_cut(it, it->w);
            continue;
        }
        ellipsis_cut(it, edge - it->x);
    }
}

static void clip_display_list(we_doc_t *d) {
    static os_fb_web_item_t output[OS_FB_WEB_ITEMS];
    static int16_t owners[OS_FB_WEB_ITEMS];
    int i, count = 0;
    for (i = 0; i < (int)d->nitems; i++) {
        os_fb_web_item_t item = d->items[i];
        int left, top, right, bottom, k, fragments;
        static os_fb_web_item_t edges[WE_CLIP_FRAGMENTS];
        if (transformed_chain(d, d->item_owner[i])) {
            tf_upright_text = 0;
            fragments = transform_clip_item(d, &item, d->item_owner[i], edges, WE_CLIP_FRAGMENTS);
            if (tf_upright_text) diag(d, "transform text upright", d->item_owner[i]);
            if (fragments < 0) {
                d->perf.items_dropped++;
                diag(d, "transform display budget", d->item_owner[i]);
                continue;
            }
        } else {
            if (!clip_bounds(d, d->item_owner[i], &left, &top, &right, &bottom)) continue;
            edges[0] = item;
            fragments = clip_frags(edges, 1, left, top, right, bottom, WE_CLIP_FRAGMENTS);
            if (fragments > 0) fragments = round_clip(d, d->item_owner[i], edges, fragments, WE_CLIP_FRAGMENTS);
            if (fragments < 0) fragments = OS_FB_WEB_ITEMS + 1;
        }
        if (!fragments) continue;
        if (count + fragments > OS_FB_WEB_ITEMS) {
            if (fragments > OS_FB_WEB_ITEMS) fragments = 1;
            d->perf.items_dropped += (uint32_t)fragments;
            diag(d, "clipping display budget", d->item_owner[i]);
            continue;
        }
        for (k = 0; k < fragments; k++) { owners[count] = d->item_owner[i]; output[count++] = edges[k]; }
    }
    for (i = 0; i < count; i++) { d->items[i] = output[i]; d->item_owner[i] = owners[i]; }
    d->nitems = (uint32_t)count;
}

/* The collapsed table of a cell, row or row group, or -1. */
static int collapse_table_of(const we_doc_t *d, int i) {
    int t = i;
    if (d->nodes[i].display == WE_DISP_CELL) t = d->nodes[i].parent;
    if (t >= 0 && d->nodes[t].display == WE_DISP_ROW) t = d->nodes[t].parent;
    if (t >= 0 && w_eq(d->nodes[t].tag, "col") && d->nodes[t].parent >= 0) t = d->nodes[t].parent;
    if (t >= 0 && w_eq(d->nodes[t].tag, "colgroup")) t = d->nodes[t].parent;
    if (t >= 0 && table_group(&d->nodes[t])) t = d->nodes[t].parent;
    if (t < 0 || d->nodes[t].display != WE_DISP_TABLE || !d->nodes[t].border_collapse) return -1;
    return t;
}

/* Lot 61: collapse_w of a collapsed table is its widest visible edge
 * among cells, rows, row groups and the table (0: nothing to paint).
 * Per-edge declarations outside a collapsed table are diagnosed: the
 * separate model paints one uniform border. */
static void collapse_widths(we_doc_t *d) {
    uint32_t i;
    for (i = 0; i < d->nnodes; i++) {
        d->nodes[i].collapse_w = 0; d->nodes[i].collapse_cell = 0; d->nodes[i].own_edges = 0;
        w_zero(d->nodes[i].cin, sizeof(d->nodes[i].cin));
    }
    for (i = 0; i < d->nnodes; i++) {
        we_node_t *n = &d->nodes[i];
        int table = -1, side, w, style;
        uint32_t color;
        if (n->is_text || n->display == WE_DISP_NONE) continue;
        if (n->display == WE_DISP_TABLE && n->border_collapse) table = (int)i;
        else if (n->display == WE_DISP_CELL || n->display == WE_DISP_ROW || table_group(n) || table_column_tag(n))
            table = collapse_table_of(d, (int)i);
        if (table < 0) {
            n->own_edges = (uint8_t)edges_set(n);     /* lot 64: painted per side */
            continue;
        }
        if (n->display == WE_DISP_CELL) n->collapse_cell = 1;
        for (side = 0; side < 4; side++) {
            edge_of(n, side, &w, &style, &color);
            if (style != WE_BS_HIDDEN && w > d->nodes[table].collapse_w) d->nodes[table].collapse_w = (uint8_t)w;
        }
    }
    for (i = 0; i < d->nnodes; i++)
        if (d->nodes[i].display == WE_DISP_TABLE && d->nodes[i].border_collapse && d->nodes[i].collapse_w)
            collapse_insets(d, (int)i);
}

static void resolve_grid_areas(we_doc_t *d) {
    uint32_t i;
    for (i = 0; i < d->nnodes; i++) {
        we_node_t *child = &d->nodes[i];
        const we_node_t *parent;
        int k;
        if (!child->grid_area[0] || child->parent < 0) continue;
        parent = &d->nodes[child->parent];
        if (parent->display != WE_DISP_GRID) continue;
        child->grid_col_start = child->grid_row_start = 0;
        child->grid_col_span = child->grid_row_span = 1;
        for (k = 0; k < parent->area_count; k++) {
            const we_grid_area_t *area = &parent->areas[k];
            if (!w_eq(child->grid_area, area->name)) continue;
            child->grid_col_start = (uint8_t)(area->col + 1); child->grid_row_start = (uint8_t)(area->row + 1);
            child->grid_col_span = area->cols; child->grid_row_span = area->rows;
            break;
        }
        if (k == parent->area_count) diag(d, "grid area name unknown", (int)i);
    }
}

static int fixed_owner(const we_doc_t *d, int owner) {
    int guard;
    for (guard = 0; owner >= 0 && guard <= WE_DEPTH_MAX + 1; guard++, owner = d->nodes[owner].parent)
        if (d->nodes[owner].fixed) return 1;
    return 0;
}

/* Lot 61: bounded stacking contexts (CSS 2.1 appendix E, painting order
 * per context: own background, negative z children, in-flow content, then
 * z >= 0 children). A relative or absolute box with an integer z-index
 * groups its subtree. Each item gets one entry per enclosing context,
 * outermost first: z < 0 is 10 + z, z >= 0 is 22 + 2z; after the chain,
 * the context's own items take 0 and its in-flow content 20. At the root
 * the root element and body own the background (0). A stable sort keeps
 * tree order on ties. opacity below 1 also forms a context (z 0).
 * Documents without a non-absolute context or a negative z-index keep
 * the per-containing-block absolute order. Contexts
 * deeper than WE_STACK_DEPTH - 1 are flattened into their parent. */
#define WE_STACK_DEPTH 8
#define WE_STACK_KEY (WE_STACK_DEPTH + 1)
static int stack_context(const we_node_t *n) {
    return ((n->relative || n->absolute) && n->z_set && !n->fixed) || ((n->transparency || n->tf_count) && !n->fixed);
}

/* z of a context: opacity below 1 alone forms a z-index 0 context. */
static int stack_z(const we_node_t *n) {
    return (n->relative || n->absolute) && n->z_set ? n->z_index : 0;
}

/* The document, its root element and a body directly under it paint the
 * root context background (body's background stands for the canvas). */
static int root_background(const we_doc_t *d, int owner) {
    int p;
    if (owner <= 0) return owner == 0;
    p = d->nodes[owner].parent;
    if (p == 0) return 1;
    return p > 0 && d->nodes[p].parent == 0 && w_eq(d->nodes[owner].tag, "body");
}

/* Lot 62: CSS 2.1 appendix E steps 4 to 8 inside the in-flow level of a
 * context: 0 block-level backgrounds and borders, 1 floats (each whole,
 * as an atomic group), 2 inline content (text, inline box backgrounds and
 * inline-blocks as atomic groups), 3 positioned boxes with z-index auto.
 * The outermost float, inline-block or positioned box under the context
 * decides; stop is the context node (-1 at the root). */
static int paint_class(const we_doc_t *d, int owner, int kind, int stop) {
    int o, guard, cls = -1;
    for (guard = 0, o = owner; o >= 0 && o != stop && guard <= WE_DEPTH_MAX + 1; guard++, o = d->nodes[o].parent) {
        const we_node_t *n = &d->nodes[o];
        int parent_display = n->parent >= 0 ? d->nodes[n->parent].display : WE_DISP_BLOCK;
        if (n->relative || n->absolute) cls = 3;
        else if (n->float_side && parent_display != WE_DISP_FLEX && parent_display != WE_DISP_GRID) cls = 1;
        else if (n->display == WE_DISP_INLINE_BLOCK) cls = 2;
    }
    if (cls >= 0) return cls;
    if (kind == OS_FB_WEB_TEXT || d->nodes[owner].is_text || d->nodes[owner].display == WE_DISP_INLINE) return 2;
    return 0;
}

static int stack_key(const we_doc_t *d, int owner, int outline, int kind, int8_t *key) {
    int chain[WE_DEPTH_MAX + 2], count = 0, used, k, o, guard;
    for (guard = 0, o = owner; o >= 0 && guard <= WE_DEPTH_MAX + 1; guard++, o = d->nodes[o].parent)
        if (stack_context(&d->nodes[o])) chain[count++] = o;
    used = count < WE_STACK_DEPTH ? count : WE_STACK_DEPTH - 1;
    for (k = 0; k < used; k++) {
        const we_node_t *c = &d->nodes[chain[count - 1 - k]];
        key[k] = (int8_t)(stack_z(c) < 0 ? 10 + stack_z(c) : 22 + 2 * stack_z(c));
    }
    for (k = used; k < WE_STACK_KEY; k++) key[k] = 0;
    if (used) key[used] = (int8_t)(chain[count - used] == owner ? 0 : 20);
    else key[0] = (int8_t)(root_background(d, owner) ? 0 : 20);
    if (key[used] == 20) key[used + 1] = (int8_t)paint_class(d, owner, kind, used ? chain[count - used] : -1);
    /* lot 62: outlines after the z >= 0 children of their context */
    if (outline) { key[used] = 60; key[used + 1] = 0; }
    return count > used;
}

static int stack_before(const int8_t *a, const int8_t *b) {
    int k;
    for (k = 0; k < WE_STACK_KEY; k++) if (a[k] != b[k]) return a[k] < b[k];
    return 0;
}

static int items_overlap(const os_fb_web_item_t *a, const os_fb_web_item_t *b) {
    return a->x < b->x + b->w && b->x < a->x + a->w && a->y < b->y + b->h && b->y < a->y + a->h;
}

/* Lot 62: without a stacking context, keep emission order except where two
 * overlapping items paint out of appendix E order: the smallest original
 * index whose overlapping predecessors (in key order) are placed goes
 * next, so pages without conflicts keep their item order. */
static void paint_order(we_doc_t *d, int8_t keys[][WE_STACK_KEY]) {
    static os_fb_web_item_t output[OS_FB_WEB_ITEMS];
    static int16_t owners[OS_FB_WEB_ITEMS];
    static uint8_t placed[OS_FB_WEB_ITEMS];
    int n = (int)d->nitems, i, j, out, moved = 0;
    for (i = 0; i < n && !moved; i++)
        for (j = 0; j < i; j++)
            if (stack_before(keys[i], keys[j]) && items_overlap(&d->items[i], &d->items[j])) { moved = 1; break; }
    if (!moved) return;
    w_zero(placed, sizeof(placed));
    for (out = 0; out < n; out++) {
        for (i = 0; i < n; i++) {
            if (placed[i]) continue;
            for (j = 0; j < n; j++) {
                int before;
                if (j == i || placed[j] || !items_overlap(&d->items[i], &d->items[j])) continue;
                before = stack_before(keys[j], keys[i]) || (!stack_before(keys[i], keys[j]) && j < i);
                if (before) break;
            }
            if (j == n) break;
        }
        placed[i] = 1;
        output[out] = d->items[i];
        owners[out] = d->item_owner[i];
    }
    for (i = 0; i < n; i++) { d->items[i] = output[i]; d->item_owner[i] = owners[i]; }
}

static void stack_items(we_doc_t *d) {
    static int8_t keys[OS_FB_WEB_ITEMS][WE_STACK_KEY];
    int i, j, k, any = 0, flat = 0;
    for (i = 0; i < (int)d->nnodes; i++)
        if (stack_context(&d->nodes[i]) && (!d->nodes[i].absolute || stack_z(&d->nodes[i]) < 0)) any = 1;
    for (i = 0; i < (int)d->nitems; i++)
        flat |= stack_key(d, d->item_owner[i], d->items[i].pad & WE_ITEM_OUTLINE, d->items[i].kind, keys[i]);
    if (!any) { paint_order(d, keys); return; }
    if (flat) diag(d, "stacking depth", -1);
    for (i = 1; i < (int)d->nitems; i++) {
        os_fb_web_item_t item = d->items[i];
        int16_t owner = d->item_owner[i];
        int8_t key[WE_STACK_KEY];
        for (k = 0; k < WE_STACK_KEY; k++) key[k] = keys[i][k];
        for (j = i; j > 0 && stack_before(key, keys[j - 1]); j--) {
            d->items[j] = d->items[j - 1];
            d->item_owner[j] = d->item_owner[j - 1];
            for (k = 0; k < WE_STACK_KEY; k++) keys[j][k] = keys[j - 1][k];
        }
        d->items[j] = item;
        d->item_owner[j] = owner;
        for (k = 0; k < WE_STACK_KEY; k++) keys[j][k] = key[k];
    }
}

/* Lot 61: opacity 0 removes the items of its subtree (the boxes keep their
 * layout). Lot 62: 0 < opacity < 1 is composited by composite_items. */
static void opacity_items(we_doc_t *d) {
    int i, out = 0;
    for (i = 0; i < (int)d->nitems; i++) {
        int o = d->item_owner[i], guard, hidden = 0;
        for (guard = 0; o >= 0 && guard <= WE_DEPTH_MAX + 1; guard++, o = d->nodes[o].parent)
            if (d->nodes[o].transparency == 100) { hidden = 1; break; }
        if (hidden) continue;
        d->items[out] = d->items[i];
        d->item_owner[out++] = d->item_owner[i];
    }
    d->nitems = (uint32_t)out;
}

/* ---------- Lot 62: alpha compositing ----------
 * The shared item has no alpha and the kernel painter masks bits 24..31,
 * so translucent paint is resolved here into opaque fragments. Every
 * translucent item is split along the edges of what is painted under it
 * (RECT items, BORDER sides, TEXT boxes) and each piece takes "source over
 * backdrop". A TEXT item under a translucent RECT is repainted on top,
 * cut to the piece, with its own blended colour. An opacity group (CSS
 * Color 4: 0 < opacity < 1) is composited in its own space first, deepest
 * group first: its items blend over earlier items of the same group only,
 * then their alpha is multiplied by the group alpha, so the group blends
 * as one layer. The root backdrop is the white canvas of the web pane.
 * Fragments share the 96-item display list; on overflow the list is kept
 * unblended (opaque colours, fully transparent items dropped) with
 * "alpha display budget". Text over text blends with the paint under the
 * lower text. */
#define WE_ALPHA_REGIONS 32
#define WE_ALPHA_OVERLAYS 8
#define WE_ALPHA_STACK 48
#define WE_CANVAS 0xFFFFFFu
typedef struct { int16_t x, y, w, h; uint32_t dst; int16_t item; } we_region_t;
static we_region_t alpha_region[WE_ALPHA_REGIONS], alpha_overlay[WE_ALPHA_OVERLAYS];
static int alpha_regions, alpha_overlays;
static os_fb_web_item_t alpha_out[OS_FB_WEB_ITEMS], alpha_saved[OS_FB_WEB_ITEMS];
static int16_t alpha_owner[OS_FB_WEB_ITEMS], alpha_child_of[OS_FB_WEB_ITEMS], alpha_saved_owner[OS_FB_WEB_ITEMS];
static int alpha_count;

static int color_alpha_of(uint32_t c) {
    return 255 - (int)(c >> WE_ALPHA_SHIFT);
}

/* Source over destination with straight colours (Porter-Duff over). */
static uint32_t alpha_over(uint32_t src, uint32_t dst) {
    int sa = color_alpha_of(src), da = color_alpha_of(dst), oa, k;
    uint32_t out = 0;
    if (sa == 255 || da == 0) return src;
    if (sa == 0) return dst;
    oa = sa + (da * (255 - sa) + 127) / 255;
    for (k = 16; k >= 0; k -= 8) {
        int sc = (int)((src >> k) & 255u), dc = (int)((dst >> k) & 255u);
        int num = sc * sa * 255 + dc * da * (255 - sa);
        out |= (uint32_t)clamp_int((num + oa * 255 / 2) / (oa * 255), 0, 255) << k;
    }
    return with_alpha(out, oa);
}

static int alpha_group(const we_node_t *n) {
    return n->transparency > 0 && n->transparency < 100;
}

static int group_alpha(const we_node_t *n) {
    return ((100 - n->transparency) * 255 + 50) / 100;
}

/* For the chain of owner (inclusive): the alpha group directly inside g,
 * -1 when owner belongs to g itself, -2 when owner is outside g (g -1 is
 * the root and contains everything). */
static int alpha_child(const we_doc_t *d, int owner, int g) {
    int last = -1, guard;
    for (guard = 0; owner >= 0 && guard <= WE_DEPTH_MAX + 1; guard++, owner = d->nodes[owner].parent) {
        if (owner == g) return last;
        if (alpha_group(&d->nodes[owner])) last = owner;
    }
    return g < 0 ? last : -2;
}

static int alpha_depth(const we_doc_t *d, int owner) {
    int depth = 0, guard;
    for (guard = 0; owner >= 0 && guard <= WE_DEPTH_MAX + 1; guard++, owner = d->nodes[owner].parent)
        if (alpha_group(&d->nodes[owner])) depth++;
    return depth;
}

/* Painted area k of an output item: the box of a RECT or TEXT (k 0, bold
 * text one pixel wider), a side of a BORDER (k 0..3). */
static int alpha_shape(const os_fb_web_item_t *it, int k, int *x, int *y, int *w, int *h) {
    if (it->kind == OS_FB_WEB_BORDER) {
        os_fb_web_item_t side;
        border_side(it, k, &side);
        *x = side.x; *y = side.y; *w = side.w; *h = side.h;
    } else {
        if (k) return 0;
        *x = it->x; *y = it->y; *w = it->w + (it->kind == OS_FB_WEB_TEXT && it->bold ? 1 : 0); *h = it->h;
    }
    return *w > 0 && *h > 0;
}

static int alpha_add(we_region_t *list, int *count, int cap, int x, int y, int w, int h, uint32_t dst, int item) {
    if (*count >= cap) return -1;
    list[*count].x = (int16_t)x; list[*count].y = (int16_t)y;
    list[*count].w = (int16_t)w; list[*count].h = (int16_t)h;
    list[*count].dst = dst; list[*count].item = (int16_t)item;
    (*count)++;
    return 0;
}

/* Splits (x, y, w, h) into pieces of one backdrop each, looking at the
 * output items below layer `up` (item * 4 + side) that belong to the
 * group being composited and not to the source's own child group. */
static int alpha_query(int g, int child, int text_src, int x, int y, int w, int h) {
    struct { int16_t x, y, w, h, up; } stack[WE_ALPHA_STACK];
    int sp = 0;
    alpha_regions = alpha_overlays = 0;
    stack[sp].x = (int16_t)x; stack[sp].y = (int16_t)y; stack[sp].w = (int16_t)w; stack[sp].h = (int16_t)h;
    stack[sp++].up = (int16_t)(alpha_count * 4);
    while (sp) {
        int rx, ry, rw, rh, up, layer, found = 0;
        sp--;
        rx = stack[sp].x; ry = stack[sp].y; rw = stack[sp].w; rh = stack[sp].h; up = stack[sp].up;
        for (layer = up - 1; layer >= 0 && !found; layer--) {
            const os_fb_web_item_t *it = &alpha_out[layer >> 2];
            int c = alpha_child_of[layer >> 2], sx, sy, sw, sh, ix, iy, ir, ib, piece;
            if (c == -2 || (child >= 0 && c == child)) continue;
            if (text_src && it->kind == OS_FB_WEB_TEXT) continue;
            if (!alpha_shape(it, layer & 3, &sx, &sy, &sw, &sh)) continue;
            ix = sx > rx ? sx : rx; iy = sy > ry ? sy : ry;
            ir = sx + sw < rx + rw ? sx + sw : rx + rw; ib = sy + sh < ry + rh ? sy + sh : ry + rh;
            if (ir <= ix || ib <= iy) continue;
            found = 1;
            if (it->kind == OS_FB_WEB_TEXT) {
                if (alpha_add(alpha_overlay, &alpha_overlays, WE_ALPHA_OVERLAYS, ix, iy, ir - ix, ib - iy, it->color, layer >> 2)) return -1;
                if (sp >= WE_ALPHA_STACK) return -1;
                stack[sp].x = (int16_t)ix; stack[sp].y = (int16_t)iy; stack[sp].w = (int16_t)(ir - ix);
                stack[sp].h = (int16_t)(ib - iy); stack[sp++].up = (int16_t)layer;
            } else if (alpha_add(alpha_region, &alpha_regions, WE_ALPHA_REGIONS, ix, iy, ir - ix, ib - iy, it->color, -1)) return -1;
            /* the rest of the piece: bands above, below, left and right */
            for (piece = 0; piece < 4; piece++) {
                int px = rx, py = ry, pw = rw, ph = rh;
                if (piece == 0) ph = iy - ry;
                else if (piece == 1) { py = ib; ph = ry + rh - ib; }
                else { py = iy; ph = ib - iy; if (piece == 2) pw = ix - rx; else { px = ir; pw = rx + rw - ir; } }
                if (pw <= 0 || ph <= 0) continue;
                if (sp >= WE_ALPHA_STACK) return -1;
                stack[sp].x = (int16_t)px; stack[sp].y = (int16_t)py; stack[sp].w = (int16_t)pw;
                stack[sp].h = (int16_t)ph; stack[sp++].up = (int16_t)layer;
            }
        }
        if (!found && alpha_add(alpha_region, &alpha_regions, WE_ALPHA_REGIONS, rx, ry, rw, rh,
                                g < 0 ? WE_CANVAS : with_alpha(0, 0), -1)) return -1;
    }
    return 0;
}

static int alpha_push(const os_fb_web_item_t *it, int owner, int child) {
    if (alpha_count >= OS_FB_WEB_ITEMS) return -1;
    alpha_out[alpha_count] = *it;
    alpha_owner[alpha_count] = (int16_t)owner;
    alpha_child_of[alpha_count++] = (int16_t)child;
    return 0;
}

/* Text item `text` cut to a piece, every fragment in colour `color`. */
static int alpha_push_text(const os_fb_web_item_t *text, int x, int y, int w, int h, uint32_t color, int owner, int child) {
    static os_fb_web_item_t pieces[WE_CLIP_FRAGMENTS];
    int count = clip_glyphs(text, x, y, x + w, y + h, pieces, WE_CLIP_FRAGMENTS), k;
    if (count < 0) return -1;
    for (k = 0; k < count; k++) {
        pieces[k].color = color;
        if (alpha_push(&pieces[k], owner, child)) return -1;
    }
    return 0;
}

/* One translucent RECT or TEXT source over the output built so far. */
static int alpha_source(const os_fb_web_item_t *src, int owner, int g, int child) {
    int k, text = src->kind == OS_FB_WEB_TEXT, x, y, w, h;
    if (!alpha_shape(src, 0, &x, &y, &w, &h)) return 0;
    if (alpha_query(g, child, text, x, y, w, h)) return -1;
    if (text && alpha_regions == 1 && alpha_region[0].w == w && alpha_region[0].h == h) {
        os_fb_web_item_t one = *src;
        one.color = alpha_over(src->color, alpha_region[0].dst);
        return alpha_push(&one, owner, child);
    }
    for (k = 0; k < alpha_regions; k++) {
        const we_region_t *r = &alpha_region[k];
        uint32_t color = alpha_over(src->color, r->dst);
        if (text) { if (alpha_push_text(src, r->x, r->y, r->w, r->h, color, owner, child)) return -1; }
        else {
            os_fb_web_item_t piece = *src;
            piece.x = r->x; piece.y = r->y; piece.w = (uint16_t)r->w; piece.h = (uint16_t)r->h;
            piece.color = color;
            if (alpha_push(&piece, owner, child)) return -1;
        }
    }
    for (k = 0; k < alpha_overlays; k++) {
        const we_region_t *r = &alpha_overlay[k];
        os_fb_web_item_t under = alpha_out[r->item];
        if (alpha_push_text(&under, r->x, r->y, r->w, r->h, alpha_over(src->color, r->dst), owner, child)) return -1;
    }
    return 0;
}

/* Composites the items of group g (-1: the whole page over the canvas). */
static int alpha_pass(we_doc_t *d, int g) {
    int i, k, ga = g >= 0 ? group_alpha(&d->nodes[g]) : 255;
    alpha_count = 0;
    for (i = 0; i < (int)d->nitems; i++) {
        os_fb_web_item_t it = d->items[i];
        int owner = d->item_owner[i], child = alpha_child(d, owner, g), a = color_alpha_of(it.color);
        if (child == -2 || a == 255) { if (alpha_push(&it, owner, child)) return -1; continue; }
        if (a == 0) continue;
        if (it.kind == OS_FB_WEB_BORDER) {
            for (k = 0; k < 4; k++) {
                os_fb_web_item_t side;
                border_side(&it, k, &side);
                if (alpha_source(&side, owner, g, child)) return -1;
            }
        } else if (alpha_source(&it, owner, g, child)) return -1;
    }
    for (i = 0; i < alpha_count; i++) {
        d->items[i] = alpha_out[i];
        d->item_owner[i] = alpha_owner[i];
        if (g >= 0 && alpha_child_of[i] != -2)
            d->items[i].color = with_alpha(d->items[i].color, (color_alpha_of(d->items[i].color) * ga + 127) / 255);
    }
    d->nitems = (uint32_t)alpha_count;
    return 0;
}

static void composite_items(we_doc_t *d) {
    int i, any = 0, depth, max_depth = 0, failed = 0, saved = (int)d->nitems, out = 0;
    for (i = 0; i < (int)d->nitems; i++) {
        int depth_i = alpha_depth(d, d->item_owner[i]);
        if (depth_i > max_depth) max_depth = depth_i;
        if (depth_i || color_alpha_of(d->items[i].color) != 255) any = 1;
    }
    if (!any) return;
    for (i = 0; i < (int)d->nitems; i++) { alpha_saved[i] = d->items[i]; alpha_saved_owner[i] = d->item_owner[i]; }
    for (depth = max_depth; depth >= 1 && !failed; depth--)
        for (i = 0; i < (int)d->nnodes && !failed; i++)
            if (alpha_group(&d->nodes[i]) && alpha_depth(d, i) == depth) failed = alpha_pass(d, i) != 0;
    if (!failed) failed = alpha_pass(d, -1) != 0;
    if (!failed) {
        for (i = 0; i < (int)d->nitems; i++) d->items[i].color &= 0xFFFFFFu;
        return;
    }
    diag(d, "alpha display budget", -1);
    for (i = 0; i < saved; i++) {
        if (!color_alpha_of(alpha_saved[i].color)) continue;
        d->items[out] = alpha_saved[i];
        d->items[out].color &= 0xFFFFFFu;
        d->item_owner[out++] = alpha_saved_owner[i];
    }
    d->nitems = (uint32_t)out;
}

/* Lot 62: outlines (CSS UI 4) after layout: the border box grown by the
 * offset plus the width, with no layout effect, painted last in their
 * stacking context (CSS 2.1 appendix E step 10). Inline boxes have no
 * outline in this subset. */
static void paint_outlines(we_doc_t *d) {
    uint32_t i, k;
    for (i = 0; i < d->nnodes; i++) {
        const we_node_t *n = &d->nodes[i];
        int w = n->outline_w, grow = n->outline_offset + n->outline_w, x, y, ow, oh, first = (int)d->nitems, style = n->outline_style;
        uint32_t color = n->outline_color_set ? n->outline_color : n->color;
        if (n->is_text || !style || !w || n->display == WE_DISP_NONE || n->display == WE_DISP_INLINE) continue;
        if (!n->w && !n->h) continue;
        x = n->x - grow; y = n->y - grow; ow = n->w + 2 * grow; oh = n->h + 2 * grow;
        if (ow <= 0 || oh <= 0) continue;
        if (style >= 7 && style <= 10) frame_3d(d, (int)i, style, color, x, y, ow, oh, w);
        else if (style == 5 || style == 6 || (style == 4 && w >= 3)) {
            edge_paint(d, (int)i, style, color, 0, y, x, w, ow);
            edge_paint(d, (int)i, style, color, 0, y + oh - w, x, w, ow);
            edge_paint(d, (int)i, style, color, 1, x, y + w, w, oh - 2 * w);
            edge_paint(d, (int)i, style, color, 1, x + ow - w, y + w, w, oh - 2 * w);
        } else {
            os_fb_web_item_t *it = emit(d, (int)i, OS_FB_WEB_BORDER, x, y, ow, oh, color);
            if (it) it->bold = (uint8_t)w;
        }
        for (k = (uint32_t)first; k < d->nitems; k++) d->items[k].pad |= WE_ITEM_OUTLINE;
    }
}

/* Reserved shadow items that stayed unused are removed. */
static void drop_reserved(we_doc_t *d) {
    uint32_t i, out = 0;
    for (i = 0; i < d->nitems; i++) {
        if (!d->items[i].kind) continue;
        d->items[out] = d->items[i];
        d->items[out].pad = (uint16_t)(d->items[out].pad & ~WE_ITEM_RESERVED);
        d->item_owner[out++] = d->item_owner[i];
    }
    d->nitems = out;
}

/* Fixed boxes paint after the rest of the page, in their own order. */
static void raise_fixed(we_doc_t *d) {
    static os_fb_web_item_t output[OS_FB_WEB_ITEMS];
    static int16_t owners[OS_FB_WEB_ITEMS];
    int pass, i, count = 0;
    for (pass = 0; pass < 2; pass++)
        for (i = 0; i < (int)d->nitems; i++)
            if (fixed_owner(d, d->item_owner[i]) == pass) { output[count] = d->items[i]; owners[count++] = d->item_owner[i]; }
    for (i = 0; i < count; i++) { d->items[i] = output[i]; d->item_owner[i] = owners[i]; }
}

int we_scroll(we_doc_t* d, int y) {
    int limit, dy, i;
    if (!d) return 0;
    limit = (int)d->perf.page_h - d->viewport_h;
    if (limit < 0) limit = 0;
    if (y > limit) y = limit;
    if (y < 0) y = 0;
    dy = y - d->scroll_y;
    if (!dy) return y;
    for (i = 0; i < (int)d->nitems; i++)
        if (fixed_owner(d, d->item_owner[i])) d->items[i].y = (int16_t)limit_coord(d->items[i].y + dy);
    for (i = 0; i < (int)d->nnodes; i++)
        if (fixed_owner(d, i)) d->nodes[i].y = (int16_t)limit_coord(d->nodes[i].y + dy);
    d->scroll_y = (int16_t)y;
    return y;
}

int we_layout(we_doc_t* d, int viewport_w) {
    return we_layout_viewport(d, viewport_w, WE_VIEWPORT_H_DEFAULT);
}

int we_layout_viewport(we_doc_t* d, int viewport_w, int viewport_h) {
    int h;
    if (!d || d->nnodes == 0 || viewport_w < 64 || viewport_h < 1) return -1;
    if (viewport_h > WE_LAYOUT_MAX) viewport_h = WE_LAYOUT_MAX;
    d->viewport_w = (int16_t)(viewport_w > WE_LAYOUT_MAX ? WE_LAYOUT_MAX : viewport_w);
    d->viewport_h = (int16_t)viewport_h;
    d->scroll_y = 0;
    w_zero(static_set, sizeof(static_set));
    d->nitems = 0; math_refused = 0; wavy_decoration = 0; gradient_banded = 0; text_shadow_blur = 0; radius_stepped = 0; radius_square = 0; baseline_probe = 0; probe_level = probe_count = 0;
    d->perf.boxes = d->perf.lines = d->perf.items_dropped = 0;
    d->perf.flex_lines = d->perf.grid_lines = d->perf.floats = 0;
    d->perf.table_rows = d->perf.images = d->perf.image_refused = 0;
    { uint32_t i; int bit; for (i = 0; i < d->nnodes; i++) {
        we_node_t *n = &d->nodes[i];
        n->x = n->y = n->w = n->h = 0;
        for (bit = 0; bit < 8; bit++) if (n->spacing_pct & (1u << bit)) {
            if (bit < 4) n->margin[bit] = 0; else n->padding[bit - 4] = 0;
        }
    } }
    if (viewport_w > WE_LAYOUT_MAX) viewport_w = WE_LAYOUT_MAX;
    { uint32_t i; for (i = 0; i < d->nnodes; i++) {
        forced_width[i] = forced_height[i] = resolved_height[i] = containing_height[i] = -1;
        measure_content[i] = 0;
        flow_baseline[i] = -1;
    } }
    image_work.ready_idx = -1;
    w_zero(image_work.failed, sizeof(image_work.failed));
    w_zero(image_work.natural_w, sizeof(image_work.natural_w));
    w_zero(image_work.natural_h, sizeof(image_work.natural_h));
    w_zero(measure_cache, sizeof(measure_cache));
    measure_budget = WE_MEASURE_BUDGET;
    { uint32_t i; for (i = 0; i < d->nnodes; i++) {
        grid_expand(&d->nodes[i], 0, -1, 0);
        grid_expand(&d->nodes[i], 1, -1, 0);
    } }
    subgrid_validate(d);
    collapse_widths(d);
    resolve_grid_requests(d);
    resolve_grid_areas(d);
    subgrid_bind_all(d);
    vertical_styles(d, 1);
    intrinsic_sizes(d);
    { we_node_t *root = &d->nodes[0]; int bit;
      for (bit = 0; bit < 8; bit++) if (root->spacing_pct & (1u << bit)) {
          int t = root->spacing_value[bit], v = t < 0 ? -((-t * viewport_w) / 1000) : (t * viewport_w) / 1000;
          if (bit < 4) root->margin[bit] = (int16_t)v; else root->padding[bit - 4] = (int16_t)v;
      } }
    h = layout_block(d, 0, 0, 0, viewport_w, 0);
    vertical_styles(d, 0);
    drop_reserved(d);
    ellipsis_items(d);
    paint_outlines(d);
    stack_items(d);
    opacity_items(d);
    raise_fixed(d);
    clip_display_list(d);
    composite_items(d);
    { uint32_t i; for (i = 0; i < d->nitems; i++) d->items[i].pad = 0; }
    if (math_refused) diag(d, "math evaluation refused", -1);
    if (wavy_decoration) diag(d, "wavy decoration painted solid", -1);
    if (gradient_banded) diag(d, "gradient painted in bands", -1);
    if (text_shadow_blur) diag(d, "text-shadow blur painted sharp", -1);
    if (radius_stepped) diag(d, "border-radius painted in steps", -1);
    if (radius_square) diag(d, "rounded effect painted square", -1);
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
    out(ctx, n->align == WE_ALIGN_CENTER ? " align center" : n->align == WE_ALIGN_RIGHT ? " align right" :
             n->align == WE_ALIGN_JUSTIFY ? " align justify" : " align left");
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
