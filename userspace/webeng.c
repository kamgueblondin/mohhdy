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
            if (w_ieq_n(s + i, n - i, "&amp;")) { c = '&'; i += 5; }
            else if (w_ieq_n(s + i, n - i, "&lt;")) { c = '<'; i += 4; }
            else if (w_ieq_n(s + i, n - i, "&gt;")) { c = '>'; i += 4; }
            else if (w_ieq_n(s + i, n - i, "&quot;")) { c = '"'; i += 6; }
            else if (w_ieq_n(s + i, n - i, "&nbsp;")) { c = ' '; i += 6; }
            else i++;
            if (c != ' ') graphic = 1;
            buf[o++] = c;
            continue;
        }
        if (w_space(c)) {
            buf[o++] = c == '\n' ? '\n' : ' ';
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
                    else if (w_eq(an, "colspan") || w_eq(an, "rowspan")) {
                        int span = 0, digit, valid = vk > 0;
                        for (digit = 0; digit < vk; digit++) {
                            if (av[digit] < '0' || av[digit] > '9' || span > WE_TRACKS_MAX) { valid = 0; break; }
                            span = span * 10 + av[digit] - '0';
                        }
                        if (!valid || span < 1 || span > WE_TRACKS_MAX) {
                            n->span_refused = 1;
                            diag(d, "table span refused", idx);
                        } else if (w_eq(an, "colspan")) n->colspan = (uint8_t)span;
                        else n->rowspan = (uint8_t)span;
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
                if (part->pseudo || sel_ident(s, len, &i, pseudo, sizeof(pseudo), 1)) return -1;
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
            if (!subgrid_on(s, axis) || span < 1) continue;
            if (*count > span) *count = (uint8_t)span;
            if (*first && *first + *count - 1 > span) *first = (uint8_t)(span - *count + 1);
        }
    }
}

static void subgrid_tracks(we_node_t *n, int axis, const int *sizes, int start, int gap) {
    int k, count = axis ? n->grid_rows : n->grid_cols;
    for (k = 0; k < count; k++) {
        if (axis) { n->row_size[k] = (int16_t)sizes[start + k]; n->row_floor[k] = n->row_cap[k] = -1; }
        else { n->col_size[k] = (int16_t)sizes[start + k]; n->col_floor[k] = n->col_cap[k] = -1; }
    }
    n->sub_gap[axis] = (int16_t)(gap + 1);
}

/* Expand a valid margin into top,right,bottom,left and replace all auto bits. */
static int margin_auto_box(we_node_t *n, const char *v, int vl) {
    int a[4], l[4], c = tokens(v, vl, a, l), k, px;
    int vals[4], autos[4], top, right, bottom, left;
    if (c < 1 || c > 4) return 0;
    for (k = 0; k < c; k++) {
        if (value_is(v + a[k], l[k], "auto")) { autos[k] = 1; vals[k] = 0; }
        else if (parse_len(v + a[k], l[k], &px, 0) == 0) { autos[k] = 0; vals[k] = px; }
        else return 0;
    }
    top = vals[0];
    right = c >= 2 ? vals[1] : vals[0];
    bottom = c >= 3 ? vals[2] : top;
    left = c == 4 ? vals[3] : right;
    n->margin[0] = (int16_t)top;
    n->margin[1] = (int16_t)right;
    n->margin[2] = (int16_t)bottom;
    n->margin[3] = (int16_t)left;
    n->margin_auto = 0;
    if (c == 1 && autos[0]) n->margin_auto = (uint8_t)(WE_AUTO_TOP | WE_AUTO_RIGHT | WE_AUTO_BOTTOM | WE_AUTO_LEFT);
    else {
        int at = autos[0];
        int ar = c >= 2 ? autos[1] : at;
        int ab = c >= 3 ? autos[2] : at;
        int al = c == 4 ? autos[3] : ar;
        if (at) n->margin_auto |= WE_AUTO_TOP;
        if (ar) n->margin_auto |= WE_AUTO_RIGHT;
        if (ab) n->margin_auto |= WE_AUTO_BOTTOM;
        if (al) n->margin_auto |= WE_AUTO_LEFT;
    }
    return 1;
}

static const char *logical_property(const char *prop) {
    static const char *const logical[] = {
        "inline-size", "block-size", "min-inline-size", "max-inline-size", "min-block-size", "max-block-size",
        "margin-inline-start", "margin-inline-end", "margin-block-start", "margin-block-end",
        "padding-inline-start", "padding-inline-end", "padding-block-start", "padding-block-end",
        "inset-inline-start", "inset-inline-end", "inset-block-start", "inset-block-end"
    };
    static const char *const physical[] = {
        "width", "height", "min-width", "max-width", "min-height", "max-height",
        "margin-left", "margin-right", "margin-top", "margin-bottom",
        "padding-left", "padding-right", "padding-top", "padding-bottom",
        "left", "right", "top", "bottom"
    };
    int i;
    for (i = 0; i < 18; i++) if (w_eq(prop, logical[i])) return physical[i];
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
    n->row_gap = count == 1 && !expr[0].count ? -1 : (int16_t)values[0];
    n->col_gap = count == 1 && !expr[1].count ? -1 : (int16_t)values[1];
    return 0;
}

static int logical_pair(we_node_t *n, const char *prop, const char *v, int len) {
    int margin = w_eq(prop, "margin-inline") || w_eq(prop, "margin-block");
    int block = w_eq(prop, "margin-block") || w_eq(prop, "padding-block");
    int starts[4], lengths[4], count = tokens(v, len, starts, lengths), k, values[2], auto_bits[2] = {0, 0};
    int sides[2] = {block ? 0 : 3, block ? 2 : 1};
    static const uint8_t flags[4] = {WE_AUTO_TOP, WE_AUTO_RIGHT, WE_AUTO_BOTTOM, WE_AUTO_LEFT};
    if (count < 1 || count > 2) return -1;
    for (k = 0; k < count; k++) {
        int end = lengths[k];
        const char *token = v + starts[k];
        if (margin && value_is(token, end, "auto")) { values[k] = 0; auto_bits[k] = 1; }
        else {
            if (end >= 2 && value_is(token + end - 2, 2, "px")) end -= 2;
            if (bounded_signed(token, end, 2000, &values[k]) || (!margin && values[k] < 0)) return -1;
        }
    }
    if (count == 1) { values[1] = values[0]; auto_bits[1] = auto_bits[0]; }
    for (k = 0; k < 2; k++) {
        if (margin) {
            n->margin[sides[k]] = (int16_t)values[k];
            n->margin_auto &= (uint8_t)~flags[sides[k]];
            if (auto_bits[k]) n->margin_auto |= flags[sides[k]];
        } else n->padding[sides[k]] = (int16_t)values[k];
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
        slots[0] = w_eq(prop, "inset-inline") ? WE_LEN_LEFT : WE_LEN_TOP;
        slots[1] = w_eq(prop, "inset-inline") ? WE_LEN_RIGHT : WE_LEN_BOTTOM;
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

static void apply_decl(we_doc_t* d, we_node_t* n, const char* p, int pl, const char* v, int vl) {
    char prop[24];
    int px, pct, k;
    uint32_t col;
    w_cpy(prop, 24, p, pl);
    for (k = 0; prop[k]; k++) prop[k] = w_low(prop[k]);
    { const char *mapped = logical_property(prop); if (mapped != prop) w_cpy(prop, sizeof(prop), mapped, w_len(mapped)); }
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
    if (w_eq(prop, "aspect-ratio")) {
        int16_t a, b;
        if (parse_aspect(v, vl, &a, &b)) diag(d, "aspect ratio refused", -1);
        else { n->aspect_num = a; n->aspect_den = b; }
    } else if (w_eq(prop, "margin-inline") || w_eq(prop, "margin-block") || w_eq(prop, "padding-inline") || w_eq(prop, "padding-block")) {
        if (logical_pair(n, prop, v, vl)) diag(d, "logical spacing refused", -1);
    } else if (w_eq(prop, "writing-mode")) {
        if (!value_is(v, vl, "horizontal-tb")) diag(d, "writing mode refused", -1);
    } else if (w_eq(prop, "direction")) {
        if (!value_is(v, vl, "ltr")) diag(d, "direction refused", -1);
    } else if (w_eq(prop, "padding-top") || w_eq(prop, "padding-right") || w_eq(prop, "padding-bottom")) {
        if (parse_len(v, vl, &px, 0) == 0 && px >= 0)
            n->padding[w_eq(prop, "padding-top") ? 0 : w_eq(prop, "padding-right") ? 1 : 2] = (int16_t)px;
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
        else if (w_eq(prop, "row-gap")) n->row_gap = (int16_t)number;
        else n->col_gap = (int16_t)number;
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
        else diag(d, "white space refused", -1);
    } else if (w_eq(prop, "overflow-wrap")) {
        if (value_is(v, vl, "normal")) n->break_word = WE_WRAP_NORMAL;
        else if (value_is(v, vl, "break-word")) n->break_word = WE_WRAP_BREAK_WORD;
        else if (value_is(v, vl, "anywhere")) n->break_word = WE_WRAP_ANYWHERE;
        else diag(d, "word wrapping refused", -1);
    } else if (w_eq(prop, "word-break")) {
        if (value_is(v, vl, "normal") || value_is(v, vl, "break-all")) n->break_all = (uint8_t)value_is(v, vl, "break-all");
        else diag(d, "word break refused", -1);
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
    } else if (w_eq(prop, "clear")) {
        if (value_is(v, vl, "none")) n->clear_side = WE_CLEAR_NONE;
        else if (value_is(v, vl, "left")) n->clear_side = WE_CLEAR_LEFT;
        else if (value_is(v, vl, "right")) n->clear_side = WE_CLEAR_RIGHT;
        else if (value_is(v, vl, "both")) n->clear_side = WE_CLEAR_BOTH;
        else diag(d, "clear refused", -1);
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
    } else if (w_eq(prop, "color")) { if (parse_color(v, vl, &col) == 0) n->color = col; }
    else if (w_eq(prop, "background") || w_eq(prop, "background-color")) {
        if (parse_color(v, vl, &col) == 0) { n->bg = col; n->has_bg = 1; }
        else if (w_ieq_n(v, vl, "none") || w_ieq_n(v, vl, "transparent")) n->has_bg = 0;
    }
    else if (w_eq(prop, "margin")) { if (!margin_auto_box(n, v, vl)) box4(v, vl, n->margin); }
    else if (w_eq(prop, "padding")) box4(v, vl, n->padding);
    else if (w_eq(prop, "margin-top")) {
        if (value_is(v, vl, "auto")) { n->margin[0] = 0; n->margin_auto |= WE_AUTO_TOP; }
        else if (parse_len(v, vl, &px, 0) == 0) { n->margin[0] = (int16_t)px; n->margin_auto &= (uint8_t)~WE_AUTO_TOP; }
    }
    else if (w_eq(prop, "margin-right")) {
        if (value_is(v, vl, "auto")) { n->margin[1] = 0; n->margin_auto |= WE_AUTO_RIGHT; }
        else if (parse_len(v, vl, &px, 0) == 0) { n->margin[1] = (int16_t)px; n->margin_auto &= (uint8_t)~WE_AUTO_RIGHT; }
    }
    else if (w_eq(prop, "margin-bottom")) {
        if (value_is(v, vl, "auto")) { n->margin[2] = 0; n->margin_auto |= WE_AUTO_BOTTOM; }
        else if (parse_len(v, vl, &px, 0) == 0) { n->margin[2] = (int16_t)px; n->margin_auto &= (uint8_t)~WE_AUTO_BOTTOM; }
    }
    else if (w_eq(prop, "margin-left")) {
        if (value_is(v, vl, "auto")) { n->margin[3] = 0; n->margin_auto |= WE_AUTO_LEFT; }
        else if (parse_len(v, vl, &px, 0) == 0) { n->margin[3] = (int16_t)px; n->margin_auto &= (uint8_t)~WE_AUTO_LEFT; }
    }
    else if (w_eq(prop, "padding-left")) {
        if (parse_len(v, vl, &px, 0) == 0 && px >= 0) n->padding[3] = (int16_t)px;
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
        while (i < len && s[i] != ';') i++;
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
    if (w_eq(t, "body")) { for (i = 0; i < 4; i++) n->margin[i] = 8; }
    if (w_eq(t, "p") || w_eq(t, "ul") || w_eq(t, "ol")) { n->margin[0] = 8; n->margin[2] = 8; }
    if (w_eq(t, "h1") || w_eq(t, "h2") || w_eq(t, "h3") || w_eq(t, "h4")) { n->bold = 1; n->margin[0] = 8; n->margin[2] = 8; }
    if (w_eq(t, "ul")) n->padding[3] = 16;
    if (w_eq(t, "ol")) n->padding[3] = 40;
    if (w_eq(t, "b") || w_eq(t, "strong") || w_eq(t, "th")) n->bold = 1;
    if (w_eq(t, "a")) n->color = 0x0645AD;
    if (w_eq(t, "hr")) { n->height = 2; n->bg = 0x808080; n->has_bg = 1; n->margin[0] = 6; n->margin[2] = 6; }
    if (w_eq(t, "blockquote")) { n->margin[3] = 24; n->margin[1] = 24; }
    if (w_eq(t, "pre")) { n->display = WE_DISP_BLOCK; n->ws_mode = WE_WS_PRE; n->nowrap = 1; }
}

static void cascade_rules(we_doc_t *d, int idx, int important) {
    uint8_t used[WE_MAX_RULES];
    uint32_t k, r;
    we_node_t *n = &d->nodes[idx];
    for (k = 0; k < d->nrules; k++) used[k] = (uint8_t)!match_rule(d, idx, &d->rules[k]);
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

int we_style(we_doc_t* d) {
    uint32_t i;
    if (!d || d->nnodes == 0) return -1;
    if (!d->css_parsed) { parse_css(d); d->css_parsed = 1; }
    for (i = 0; i < d->nnodes; i++) {
        we_node_t* n = &d->nodes[i];
        const we_node_t* p = n->parent >= 0 ? &d->nodes[n->parent] : 0;
        n->color = p ? p->color : 0x000000;
        n->bold = p ? p->bold : 0;
        n->align = p ? p->align : WE_ALIGN_LEFT;
        n->width = -1; n->height = -1; n->width_pct = 0;
        w_zero(n->length, sizeof(n->length)); n->aspect_num = 0; n->aspect_den = 1;
        n->flex_grow = -1;
        n->flex_shrink = n->flex_basis = -1;
        n->basis_pct = n->basis_set = 0;
        n->min_width = n->max_width = -1;
        n->min_height = n->max_height = -1;
        n->row_gap = n->col_gap = -1;
        n->height_percent = n->min_height_percent = n->max_height_percent = 0;
        n->height_pct = n->min_height_pct = n->max_height_pct = 0;
        n->width_kind = n->basis_content = n->border_box = n->overflow_hidden = 0;
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
        n->item_order = 0;
        n->inset_flags = 0;
        n->offset_x = n->offset_y = n->offset_r = n->offset_b = 0;
        n->border_spacing = n->border_spacing_y = 0;
        n->clear_side = n->valign = n->z_set = 0;
        n->z_index = 0;
        n->line_height = p ? p->line_height : 0;
        n->text_indent = p ? p->text_indent : 0;
        n->ws_mode = p ? p->ws_mode : 0;
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
        n->border_color = n->color;
        if (n->is_text) { n->display = WE_DISP_INLINE; continue; }
        if (i == 0) { n->display = WE_DISP_BLOCK; continue; }
        ua_defaults(n);
        cascade_rules(d, (int)i, 0);
        cascade_rules(d, (int)i, 1);
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
    int line_h, indent, indent_used, atom_h, pending_x;
    int baseline, origin_y;
    /* Lot 61: last measured baseline, painted first/last baselines (page y),
     * text seen on the open line and its baseline-aligned atomic boxes. */
    int last, lay_first, lay_last, line_text, line_start, natoms, flow;
    struct { int node, base, height; } atoms[8];
} we_inline_t;
#define WE_NO_BASELINE (-32768)
#define WE_LINE_ATOMS 8

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
        f->indent_used = 0;
        if (f->lines == 0 && f->indent > 0 && f->width > 0) {
            int take = f->indent;
            if (f->width - take < WE_CHAR_W && f->width > WE_CHAR_W) take = f->width - WE_CHAR_W;
            if (take > f->width) take = f->width;
            if (take < 0) take = 0;
            f->x0 += take;
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

/* advance=0 paints a segment and keeps the line open for an atomic box. */
static void flush_line_mode(we_inline_t *f, int force, int advance) {
    int i, x, total = 0, box_h;
    we_doc_t* d = f->d;
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
    for (i = 0; i < f->nwords; i++) total += (f->w[i].len + (i && f->w[i].space_before ? 1 : 0)) * WE_CHAR_W;
    x = f->x0 + f->pending_x;
    if (advance && f->pending_x == 0 && f->atom_h == 0) {
        if (f->align == WE_ALIGN_CENTER && total < f->width) x += (f->width - total) / 2;
        else if (f->align == WE_ALIGN_RIGHT && total < f->width) x += f->width - total;
    }
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
        if (n->has_bg && n->display == WE_DISP_INLINE) emit(d, node, OS_FB_WEB_RECT, xs, f->y, len * WE_CHAR_W, box_h, n->bg);
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
    int ww;
    flow_region(f, len * WE_CHAR_W);
    ww = len * WE_CHAR_W + ((f->nwords && space) ? WE_CHAR_W : 0);
    if (!f->d->nodes[node].nowrap && (f->nwords || f->pending_x) && f->cur_w + ww > f->width) { flush_line(f, 0); flow_region(f, len * WE_CHAR_W); ww = len * WE_CHAR_W; }
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
    if (n->break_all && !n->nowrap) {
        while (len > 0) {
            int room, take;
            flow_region(f, WE_CHAR_W);
            room = f->width - f->cur_w - (f->nwords && space ? WE_CHAR_W : 0);
            if (room < WE_CHAR_W && (f->nwords || f->pending_x)) {
                flush_line(f, 0); space = 0;
                continue;
            }
            take = room / WE_CHAR_W;
            if (take < 1) take = 1;
            if (take > len) take = len;
            add_word_piece(f, s, take, node, space);
            s += take; len -= take; space = 0;
            if (len) flush_line(f, 0);
        }
        return;
    }
    flow_region(f, WE_CHAR_W);
    if (!n->break_word || n->nowrap || len * WE_CHAR_W <= f->width) {
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
static void place_inline_block(we_inline_t *f, int idx, int depth);

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
        int keep_sp = n->ws_mode == WE_WS_PRE || n->ws_mode == WE_WS_PRE_WRAP;
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
            if (s[i] == ' ' && keep_sp) {
                a = i;
                while (i < len && s[i] == ' ') i++;
                add_word(f, s + a, i - a, idx, 0);
                continue;
            }
            while (i < len && (s[i] == ' ' || s[i] == '\n')) {
                if (s[i] == '\n' && keep_nl) break;
                i++;
                f->pending_space = 1;
            }
            if (i < len && s[i] == '\n') continue;
            a = i;
            while (i < len && s[i] != ' ' && s[i] != '\n') i++;
            if (i > a) { add_word(f, s + a, i - a, idx, f->pending_space); f->pending_space = 0; }
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
static int layout_flex(we_doc_t* d, int idx, int cx, int cy, int cwidth, int depth);
static int is_box(uint8_t display) {
    return display == WE_DISP_BLOCK || display == WE_DISP_FLEX || display == WE_DISP_GRID ||
           display == WE_DISP_TABLE || display == WE_DISP_ROW || display == WE_DISP_CELL;
}

static int node_border_extra(const we_node_t *n) {
    return 2 * n->border + n->padding[1] + n->padding[3];
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

static int axis_gap(const we_node_t *n, int vertical) {
    int value = vertical ? n->row_gap : n->col_gap;
    if (n->sub_gap[vertical] > 0) return n->sub_gap[vertical] - 1;
    value = math_nonnegative(n, vertical ? WE_LEN_ROW_GAP : WE_LEN_COL_GAP, 0, value);
    return value >= 0 ? value : n->gap > 0 ? n->gap : 0;
}

static int vertical_extra(const we_node_t *n) {
    return 2 * n->border + n->padding[0] + n->padding[2];
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

static int horizontal_auto(const we_node_t *n, int containing, int outer) {
    int free = containing - outer - n->margin[1] - n->margin[3];
    if (free <= 0 || !(n->margin_auto & WE_AUTO_LEFT)) return 0;
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
            int keep_sp = n->ws_mode == WE_WS_PRE || n->ws_mode == WE_WS_PRE_WRAP;
            int keep_nl = keep_sp || n->ws_mode == WE_WS_PRE_LINE;
            if (ch == '\n') {
                if (keep_nl) intrinsic_line(flow);
                else { intrinsic_word(flow); flow->pending = 1; }
            } else if (ch == ' ' && n->ws_mode == WE_WS_PRE_WRAP) {
                intrinsic_word(flow);
                flow->line = limit_coord(flow->line + WE_CHAR_W);
                if (WE_CHAR_W > flow->minimum) flow->minimum = WE_CHAR_W;
            } else if (ch == ' ' && keep_sp) {
                flow->line = limit_coord(flow->line + WE_CHAR_W);
                flow->word = limit_coord(flow->word + WE_CHAR_W);
                if (!n->nowrap && (n->break_all || n->break_word == WE_WRAP_ANYWHERE)) intrinsic_word(flow);
            } else if (ch == ' ') {
                intrinsic_word(flow);
                flow->pending = 1;
            } else {
                if (flow->pending && flow->line) flow->line += WE_CHAR_W;
                flow->pending = 0;
                flow->line = limit_coord(flow->line + WE_CHAR_W);
                flow->word = limit_coord(flow->word + WE_CHAR_W);
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
    x = n->float_side == WE_FLOAT_RIGHT ? f->x0 + f->width - width : f->x0 - f->indent_used;
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
                    layout_block(d, c, f->x0 - f->indent_used, f->y, f->width + f->indent_used, depth + 1);
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

static int layout_block(we_doc_t* d, int idx, int x, int y, int cw, int depth) {
    we_node_t* n = &d->nodes[idx];
    int bx, by, bw, bh, content_x, content_w, content_h, slot, border_slot = -1;
    int first_item = (int)d->nitems;
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
    if (forced_width[idx] < 0) bx += horizontal_auto(n, cw, bw);
    bx = limit_coord(bx);
    by = limit_coord(by);
    n->x = (int16_t)bx; n->y = (int16_t)by; n->w = (int16_t)bw;
    /* reserve the background item now so it paints under the children */
    slot = n->has_bg ? (int)d->nitems : -1;
    if (n->has_bg && !emit(d, idx, OS_FB_WEB_RECT, 0, 0, 0, 0, n->bg)) slot = -1;
    content_x = limit_coord(bx + n->border + n->padding[3]);
    content_w = bw - 2 * n->border - n->padding[1] - n->padding[3];
    if (content_w < WE_CHAR_W) content_w = WE_CHAR_W;
    {
        int c;
        for (c = n->first; c >= 0; c = d->nodes[c].next)
            if (d->nodes[c].absolute) set_static(c, content_x, limit_coord(by + n->border + n->padding[0]));
    }
    if (w_eq(n->tag, "img")) {
        int image_h = image_paint(d, idx, content_x, by + n->border + n->padding[0], content_w,
            modern_image(n) && forced_width[idx] >= 0 ? content_w : -1, modern_image(n) ? resolved_height[idx] : -1);
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
    else if (n->aspect_num && !w_eq(n->tag, "img")) {
        int ratio = ratio_height(n, bw - node_border_extra(n));
        if (ratio > content_h) content_h = ratio;
    }
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
    if (w_eq(n->tag, "img") || (n->overflow_hidden && layout_first[idx] != WE_NO_BASELINE))
        layout_first[idx] = layout_last[idx] = (int16_t)limit_coord(by + bh);
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
    place_absolutes(d, idx, bx + n->border, by + n->border, bw - 2 * n->border, bh - 2 * n->border, depth);
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
        if (d->nodes[p].relative || d->nodes[p].absolute) return p;
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
    int positioned = d->nodes[cb].relative || d->nodes[cb].absolute;
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
    int positioned = d->nodes[cb].relative || d->nodes[cb].absolute;
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

static int grid_item_width(const we_node_t *n, const we_node_t *child, int width) {
    int align = child->justify_self ? child->justify_self : n->justify_items;
    int intrinsic = align == WE_PACK_END || align == WE_PACK_CENTER || align == WE_PACK_START ||
                    (child->margin_auto & (WE_AUTO_LEFT | WE_AUTO_RIGHT));
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
        if (inner->row[k] < 0) continue;
        grid_span_minimum(g, 0, start + inner->col[k], child->grid_col_span,
                          preferred_outer(child, cw, child->max_content), gap, cw, work->column);
        grid_span_minimum(g, 0, start + inner->col[k], child->grid_col_span,
                          preferred_outer(child, cw, child->min_content), gap, cw, work->minimum);
    }
}

static int prepare_grid(we_doc_t *d, int idx, int cx, int cy, int cw, int depth);

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
        h = measure_area_height(d, items->node[k], grid_item_width(sub, child, w), depth + 2);
        grid_span_minimum(g, 1, start + inner->row[k], child->grid_row_span, h, gap, fixed_height, work->height);
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
            subgrid_tracks(&d->nodes[items->node[k]], 0, work->column, work->col[k], gx);
    /* Lot 61: first-baseline groups share the start row and last-baseline
     * groups the end row, spanning items included. A spanning item adds the
     * shift its group imposes to its span contribution. */
    for (k = 0; k < count; k++) {
        const we_node_t *child = &d->nodes[items->node[k]];
        int width, height, align = child->align_self ? child->align_self : n->align_items;
        if (work->row[k] < 0) continue;
        width = work->column_x[work->col[k] + child->grid_col_span - 1] +
                work->column[work->col[k] + child->grid_col_span - 1] - work->column_x[work->col[k]];
        if (subgrid_on(items->node[k], 1)) {
            subgrid_rows(d, idx, items->node[k], work->row[k], grid_item_width(n, child, width), fixed_height, gy, work, depth);
            work->item_h[k] = -1;
            continue;
        }
        height = measure_area_height(d, items->node[k], grid_item_width(n, child, width), depth + 1);
        work->item_h[k] = height;
        work->baseline[k] = 0;
        if ((align == WE_PACK_BASELINE || align == WE_PACK_LAST_BASELINE) &&
            !(child->margin_auto & (WE_AUTO_TOP | WE_AUTO_BOTTOM))) {
            int real, last = align == WE_PACK_LAST_BASELINE;
            int row = last ? work->row[k] + child->grid_row_span - 1 : work->row[k], below;
            work->baseline[k] = baseline_get(d, items->node[k], grid_item_width(n, child, width), depth + 1, last, &real);
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
        if (work->row[k] >= 0 && subgrid_on(items->node[k], 1))
            subgrid_tracks(&d->nodes[items->node[k]], 1, work->height, work->row[k], gy);
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
        width = work->column_x[c + child->grid_col_span - 1] + work->column[c + child->grid_col_span - 1] - work->column_x[c];
        height = work->row_y[r + child->grid_row_span - 1] + work->height[r + child->grid_row_span - 1] - work->row_y[r];
        child_width = grid_item_width(n, child, width);
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
            horizontal_auto(child, width, child_width - child->margin[1] - child->margin[3]) :
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
                    (child->margin_auto & (WE_AUTO_LEFT | WE_AUTO_RIGHT)) ? child->max_content : cw - flex_extra(child, 0);
                work->cross[j] = preferred_outer(child, cw, fallback);
            } else work->cross[j] = measure_area_height(d, work->node[j], work->size[j], depth + 1);
            work->baseline[j] = 0;
            if (align == WE_PACK_BASELINE || align == WE_PACK_LAST_BASELINE) {
                if (vertical) diag(d, "column baseline unsupported", work->node[j]);
                else if (!(child->margin_auto & (WE_AUTO_TOP | WE_AUTO_BOTTOM))) {
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

/* Lot 61: border-collapse shares cell borders. Adjacent cells overlap by
 * the widest cell border of the table (negative spacing), so neighbouring
 * edges paint once; the later cell's colour wins on the shared edge. */
static int table_spacing_axis(const we_node_t *n, int vertical) {
    int spacing;
    if (n->border_collapse) return -(int)n->collapse_w;
    spacing = vertical ? n->border_spacing_y : n->border_spacing;
    if (spacing < 0) return 0;
    if (spacing > WE_SPACING_MAX) return WE_SPACING_MAX;
    return spacing;
}

static int table_spacing_of(const we_node_t *n) {
    return table_spacing_axis(n, 0);
}

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
        int spacing = table_spacing_of(n);
        *minimum = limit_coord(*minimum + (cols - 1) * spacing);
        *maximum = limit_coord(*maximum + (cols - 1) * spacing);
    }
}

/* Column widths share one planner with paint. Rowspan occupies later rows.
 * border-spacing is removed from the distributable width; collapse forces 0. */
static int table_tracks(we_doc_t *d, int idx, int width, int depth, int *widths) {
    int *rows = track_work[depth].node;
    int nr = 0, c, r, cols, col, spacing, inner;
    int requested[WE_TRACKS_MAX], sum = 0, auto_cols = 0;
    nr = collect_table_rows(d, idx, rows, 1);
    cols = table_assign(d, idx, rows, nr, depth, 1);
    if (cols < 0) return 0;
    for (col = 0; col < WE_TRACKS_MAX; col++) requested[col] = 0;
    spacing = table_spacing_of(&d->nodes[idx]);
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
    inner = width - (cols > 1 ? spacing * (cols - 1) : 0);
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

static int table_origin(const int *sizes, int index, int spacing) {
    int k, at = 0;
    for (k = 0; k < index; k++) at = limit_coord(at + sizes[k] + spacing);
    return at;
}

static int table_span_size(const int *sizes, int index, int span, int spacing) {
    int k, total = 0;
    if (span < 1) span = 1;
    for (k = 0; k < span; k++) total = limit_coord(total + sizes[index + k] + (k ? spacing : 0));
    return total;
}

/* Row heights match paint. A rowspan taller than the rows it covers grows
 * the last of those rows. Single-row cells do not stretch their own box. */
static int table_row_heights(we_doc_t *d, int idx, int depth, const int *widths, int nr, int spacing_x, int spacing_y) {
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
            if (table_rowspan_of(cell, r, nr) != 1) {
                if (cell->valign == WE_VALIGN_BASELINE) diag(d, "spanning baseline unsupported", c);
                continue;
            }
            width = table_span_size(widths, slot->col[c], cell->colspan, spacing_x);
            h = measure_area_height(d, c, width, depth + 1);
            if (h > high) high = h;
            if (cell->valign == WE_VALIGN_BASELINE && r < WE_GRID_ROWS_MAX) {
                int real, base = baseline_get(d, c, width, depth + 1, 0, &real);
                if (!real) base = limit_coord(h - cell->margin[2] - cell->border - cell->padding[2]);
                slot->base[c] = (int16_t)base;
                if (base > slot->ascent[r]) slot->ascent[r] = base;
                if (h - base > slot->descent[r]) slot->descent[r] = h - base;
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
            sum = table_span_size(slot->height, r, rs, spacing_y);
            h = measure_area_height(d, c, table_span_size(widths, slot->col[c], cell->colspan, spacing_x), depth + 1);
            if (h > sum) slot->height[r + rs - 1] = limit_coord(slot->height[r + rs - 1] + h - sum);
        }
    }
    for (r = 0; r < nr; r++) total = limit_coord(total + slot->height[r] + (r ? spacing_y : 0));
    return total;
}

static int measure_table(we_doc_t *d, int idx, int width, int depth) {
    int widths[WE_TRACKS_MAX], nr = table_tracks(d, idx, width, depth, widths);
    if (!nr) return 0;
    return table_row_heights(d, idx, depth, widths, nr, table_spacing_of(&d->nodes[idx]), table_spacing_y_of(&d->nodes[idx]));
}

/* Shift in-flow descendants inside a cell that is taller than its content.
 * The cell border box stays put. Top alignment does not move anything. */
static void shift_cell(we_doc_t *d, int idx, int first_item, int shift);
static void valign_cell(we_doc_t *d, int idx, int first_item) {
    we_node_t *cell = &d->nodes[idx];
    int top, inner, bottom, used, shift, i;
    if (cell->valign == WE_VALIGN_TOP || first_item < 0) return;
    top = cell->y + cell->border + cell->padding[0];
    inner = cell->h - 2 * cell->border - cell->padding[0] - cell->padding[2];
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

static int layout_table(we_doc_t *d, int idx, int x, int y, int width, int depth) {
    int widths[WE_TRACKS_MAX], nr = table_tracks(d, idx, width, depth, widths);
    int *rows = track_work[depth].node;
    we_table_slot_t *slot = &table_slot[depth];
    int spacing = table_spacing_of(&d->nodes[idx]);
    int spacing_y = table_spacing_y_of(&d->nodes[idx]);
    int r, c, cy = y;
    if (!nr) return 0;
    table_row_heights(d, idx, depth, widths, nr, spacing, spacing_y);
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
            cell_w = table_span_size(widths, slot->col[c], span, spacing);
            cx = limit_coord(x + table_origin(widths, slot->col[c], spacing));
            {
                int first_item = (int)d->nitems;
                int stretch = rs > 1 || cell->valign != WE_VALIGN_TOP;
                if (stretch) {
                    cell_h = rs > 1 ? table_span_size(slot->height, r, rs, spacing_y) : high;
                    layout_area(d, c, cx, cy, cell_w, cell_h, cell_w, depth + 1);
                    if (cell->valign == WE_VALIGN_BASELINE && rs == 1 && r < WE_GRID_ROWS_MAX)
                        shift_cell(d, c, first_item, slot->ascent[r] - slot->base[c]);
                    else if (cell->valign != WE_VALIGN_TOP && cell->valign != WE_VALIGN_BASELINE)
                        valign_cell(d, c, first_item);
                } else layout_assigned(d, c, cx, cy, cell_w, depth + 1);
            }
            /* Row baseline: the shared baseline of its aligned cells, else
             * the first cell with a line box. */
            if (cell->valign == WE_VALIGN_BASELINE && rs == 1 && r < WE_GRID_ROWS_MAX) {
                if (row_base == WE_NO_BASELINE || !row_aligned) row_base = limit_coord(cy + slot->ascent[r]);
                row_aligned = 1;
            } else if (!row_aligned && row_base == WE_NO_BASELINE && layout_first[c] != WE_NO_BASELINE)
                row_base = layout_first[c];
        }
        if (r == 0) layout_first[idx] = (int16_t)row_base;
        if (row_base != WE_NO_BASELINE) layout_last[idx] = (int16_t)row_base;
        cy = limit_coord(cy + high + (r + 1 < nr ? spacing_y : 0));
        d->perf.table_rows++;
    }
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

static void clip_display_list(we_doc_t *d) {
    static os_fb_web_item_t output[OS_FB_WEB_ITEMS];
    static int16_t owners[OS_FB_WEB_ITEMS];
    int i, count = 0;
    for (i = 0; i < (int)d->nitems; i++) {
        os_fb_web_item_t item = d->items[i];
        int left, top, right, bottom, k, fragments = 1;
        static os_fb_web_item_t edges[WE_CLIP_FRAGMENTS];
        if (!clip_bounds(d, d->item_owner[i], &left, &top, &right, &bottom)) continue;
        if (item.x >= left && item.y >= top && item.x + item.w <= right && item.y + item.h <= bottom) {
            edges[0] = item;
        } else if (item.kind == OS_FB_WEB_TEXT) {
            fragments = clip_glyphs(&item, left, top, right, bottom, edges, WE_CLIP_FRAGMENTS);
            if (fragments < 0) fragments = OS_FB_WEB_ITEMS + 1;
            if (!fragments) continue;
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

static void collapse_widths(we_doc_t *d) {
    uint32_t i;
    for (i = 0; i < d->nnodes; i++) d->nodes[i].collapse_w = 0;
    for (i = 0; i < d->nnodes; i++) {
        const we_node_t *cell = &d->nodes[i];
        int row = cell->parent, table, b = cell->border > 8 ? 8 : cell->border;
        if (cell->display != WE_DISP_CELL || row < 0) continue;
        for (table = row; table >= 0 && d->nodes[table].display != WE_DISP_TABLE; table = d->nodes[table].parent)
            if (d->nodes[table].display != WE_DISP_ROW && !w_eq(d->nodes[table].tag, "tbody") &&
                !w_eq(d->nodes[table].tag, "thead") && !w_eq(d->nodes[table].tag, "tfoot")) { table = -1; break; }
        if (table < 0 || !d->nodes[table].border_collapse) continue;
        if (b > d->nodes[table].collapse_w) d->nodes[table].collapse_w = (uint8_t)b;
    }
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
    d->nitems = 0; math_refused = 0; baseline_probe = 0; probe_level = probe_count = 0;
    d->perf.boxes = d->perf.lines = d->perf.items_dropped = 0;
    d->perf.flex_lines = d->perf.grid_lines = d->perf.floats = 0;
    d->perf.table_rows = d->perf.images = d->perf.image_refused = 0;
    { uint32_t i; for (i = 0; i < d->nnodes; i++)
        d->nodes[i].x = d->nodes[i].y = d->nodes[i].w = d->nodes[i].h = 0; }
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
    intrinsic_sizes(d);
    h = layout_block(d, 0, 0, 0, viewport_w, 0);
    raise_fixed(d);
    clip_display_list(d);
    if (math_refused) diag(d, "math evaluation refused", -1);
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
