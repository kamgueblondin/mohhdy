/* wf_html.c - Lot 67: reader-view reduction of a fetched page (see wf_html.h). */
#include "wf_html.h"
#include "wf_entity.h"
#include "../include/os_charset.h"

static char lc(char c) { return (c >= 'A' && c <= 'Z') ? (char)(c + 32) : c; }
static int is_sp(char c) { return c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == '\f'; }

typedef struct { char* o; int cap; int n; int full; int nodes; } ob_t;

/* Lot 69: the layout engine holds WE_MAX_NODES (160) nodes; a real page
 * (fr.wikipedia.org) produced far more and was refused whole ("too many
 * nodes", render error). The reader view now stops at a node budget and
 * says the page was cut, and empty paragraphs from nested divs are not
 * emitted at all. */
#define WF_HTML_NODE_BUDGET 140

/* Lot 71: source charset: the Content-Type charset of the server when it
 * has one (HTML rule: the transport wins), else a <meta> of the page,
 * else UTF-8; and the form sink of the GUI reader view. */
static int g_cs = WF_CS_UTF8;
static int g_cs_set = WF_CS_NONE;
static wf_form_page_t* g_forms;
void wf_html_set_charset(int cs) { g_cs_set = cs == WF_CS_LATIN1 ? WF_CS_LATIN1 : cs == WF_CS_UTF8 ? WF_CS_UTF8 : WF_CS_NONE; }

static int ends_with(const ob_t* b, const char* s) {
    int l = 0, i;
    while (s[l]) l++;
    if (b->n < l) return 0;
    for (i = 0; i < l; i++) if (b->o[b->n - l + i] != s[i]) return 0;
    return 1;
}

static void ob_c(ob_t* b, char c) {
    if (b->n < b->cap - 1) b->o[b->n++] = c;
    else b->full = 1;
}
static void ob_s(ob_t* b, const char* s) { while (*s) ob_c(b, *s++); }
static int ob_room(const ob_t* b, int need) { return b->n + need < b->cap - 1; }

/* tag name at in[i] (after '<' and optional '/'), lowercased into name. */
static int tag_name(const char* in, int n, int i, char* name, int cap) {
    int k = 0;
    while (i < n && k < cap - 1) {
        char c = lc(in[i]);
        if (!((c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '-')) break;
        name[k++] = c;
        i++;
    }
    name[k] = 0;
    return i;
}

static int seq(const char* a, const char* b) { while (*a && *a == *b) { a++; b++; } return *a == *b; }
static int get_attr(const char* in, int s, int e, const char* attr, char* out, int cap);
static int seq_pre(const char* s, const char* p) { while (*p && *s == *p) { s++; p++; } return *p == 0; }

static int in_list(const char* name, const char* const* list) {
    int i;
    for (i = 0; list[i]; i++) if (seq(name, list[i])) return 1;
    return 0;
}

static const char* const DROP_BLOCK[] = {"script", "style", "noscript", "svg", "template", "iframe", "object",
                                         "canvas", "form", "select", "textarea", "button", "head", "nav", "footer",
                                         "aside", "dialog", "menu", 0};
static const char* const KEEP[] = {"h1", "h2", "h3", "h4", "h5", "h6", "p", "ul", "ol", "li", "a", "br", "pre",
                                   "b", "strong", "i", "em", "code", "blockquote", "hr", 0};
static const char* const AS_P[] = {"div", "section", "article", "main", "header", "tr", "dt", "dd", "td", "th",
                                   "figcaption", "caption", "summary", "details", "table", 0};

/* Lot 69: navigation chrome of real pages (role=navigation, hidden
 * blocks) is skipped so the reader view keeps the content within the
 * node budget. */
static int chrome_block(const char* in, int s, int e) {
    static char v[24];
    if (get_attr(in, s, e, "role", v, (int)sizeof(v)) > 0) {
        if (seq(v, "navigation") || seq(v, "banner") || seq(v, "contentinfo") || (!g_forms && seq(v, "search")) ||
            seq(v, "menu") || seq(v, "menubar") || seq(v, "dialog") || seq(v, "complementary")) return 1;
    }
    if (get_attr(in, s, e, "aria-hidden", v, (int)sizeof(v)) > 0 && seq(v, "true")) return 1;
    {
        /* boolean "hidden" (with or without a value) */
        int i;
        for (i = s; i + 6 <= e; i++)
            if (is_sp(in[i - 1]) && lc(in[i]) == 'h' && lc(in[i + 1]) == 'i' && lc(in[i + 2]) == 'd' && lc(in[i + 3]) == 'd' &&
                lc(in[i + 4]) == 'e' && lc(in[i + 5]) == 'n' &&
                (i + 6 == e || is_sp(in[i + 6]) || in[i + 6] == '=' || in[i + 6] == '/' || in[i + 6] == '>'))
                return 1;
    }
    /* language pickers (Wikipedia lists 300+ languages before the content) */
    if (get_attr(in, s, e, "hreflang", v, (int)sizeof(v)) > 0) return 1;
    {
        static char cl[160];
        int k;
        if (get_attr(in, s, e, "class", cl, (int)sizeof(cl)) > 0)
            for (k = 0; cl[k]; k++)
                if (seq_pre(cl + k, "interlanguage") || seq_pre(cl + k, "language-list") || seq_pre(cl + k, "skip-link") ||
                    seq_pre(cl + k, "visually-hidden") || seq_pre(cl + k, "sr-only") || seq_pre(cl + k, "vector-menu"))
                    return 1;
    }
    return 0;
}

/* Finds the end of the element named `name` starting after its open tag. */
static int skip_block(const char* in, int n, int i, const char* name) {
    int depth = 1;
    while (i < n) {
        if (in[i] == '<') {
            char nm[16];
            int close = (i + 1 < n && in[i + 1] == '/');
            int j = tag_name(in, n, i + 1 + close, nm, (int)sizeof(nm));
            if (seq(nm, name)) {
                if (close) { if (--depth == 0) { while (j < n && in[j] != '>') j++; return j < n ? j + 1 : n; } }
                else depth++;
            }
        }
        i++;
    }
    return n;
}

/* value of attribute attr inside a tag in[s..e) */
static int get_attr(const char* in, int s, int e, const char* attr, char* out, int cap) {
    int i = s, al = 0, k;
    while (attr[al]) al++;
    while (i < e) {
        if ((i == s || is_sp(in[i - 1])) && i + al < e) {
            for (k = 0; k < al && lc(in[i + k]) == attr[k]; k++) {}
            if (k == al) {
                int j = i + al, q = 0, o = 0;
                while (j < e && is_sp(in[j])) j++;
                if (j < e && in[j] == '=') {
                    j++;
                    while (j < e && is_sp(in[j])) j++;
                    if (j < e && (in[j] == '"' || in[j] == '\'')) q = in[j++];
                    while (j < e && o < cap - 1) {
                        char c = in[j];
                        if (q ? c == q : (is_sp(c) || c == '>')) break;
                        out[o++] = c;
                        j++;
                    }
                    out[o] = 0;
                    return o;
                }
            }
        }
        i++;
    }
    out[0] = 0;
    return -1;
}

/* copies text, escaping < and & that would confuse the engine; collapses spaces */
/* Lot 71: text is decoded (UTF-8 or Latin-1, character references) to
 * display bytes (include/os_charset.h): French accents, the oe ligature,
 * typographic quotes and the euro sign are kept for the desktop fonts;
 * Latin Extended-A letters become their base letter, others '?'. */
static void put_text(ob_t* b, const char* in, int s, int e, int* last_sp, int pre) {
    int i = s, any = 0;
    while (i < e) {
        uint32_t cp = wf_text_next(in, e, &i, g_cs, 1);
        unsigned char c;
        if (cp == 0) break;
        if (cp == 0xA0u || (!pre && (cp == ' ' || cp == '\t' || cp == '\n' || cp == '\r' || cp == '\f'))) {
            if (!*last_sp) { ob_c(b, ' '); *last_sp = 1; }
            continue;
        }
        c = os_cp1252_from_cp(cp);
        if (!c) continue;
        if (pre && cp == '\r') continue;
        *last_sp = 0;
        any = 1;
        if (c == '<') ob_s(b, "&lt;");
        else if (c == '>') ob_s(b, "&gt;");
        else if (c == '&') ob_s(b, "&amp;");
        else ob_c(b, (char)c);
    }
    if (any) b->nodes++;
}

/* Only web links are kept: relative ones and http(s); javascript:,
 * mailto:, data: and other schemes are dropped. */
static int href_web(const char* h) {
    int k;
    for (k = 0; h[k] && h[k] != '/' && h[k] != '?' && h[k] != '#'; k++) {
        if (h[k] == ':') {
            return (k == 4 && (h[0] | 32) == 'h' && (h[1] | 32) == 't' && (h[2] | 32) == 't' && (h[3] | 32) == 'p') ||
                   (k == 5 && (h[0] | 32) == 'h' && (h[1] | 32) == 't' && (h[2] | 32) == 't' && (h[3] | 32) == 'p' &&
                    (h[4] | 32) == 's');
        }
    }
    return 1;
}

/* Lot 71: decoded attribute value (entities, charset) as display bytes */
static int get_attr_text(const char* in, int s, int e, const char* attr, char* out, int cap) {
    static char raw[WF_FORM_VALUE * 2];
    int n = get_attr(in, s, e, attr, raw, (int)sizeof(raw));
    if (n < 0) { out[0] = 0; return -1; }
    return wf_text_display(raw, n, g_cs, 0, out, cap);
}

/* href with its character references decoded (&amp; in query strings) */
static int get_href(const char* in, int s, int e, char* out, int cap) {
    int n = get_attr(in, s, e, "href", out, cap), r = 0, w = 0;
    if (n <= 0) return n;
    while (r < n) {
        if (out[r] == '&') {
            int used = 0;
            uint32_t cp = wf_entity_decode(out + r, n - r, &used);
            if (used && cp > 0 && cp < 0x80u) { out[w++] = (char)cp; r += used; continue; }
        }
        out[w++] = out[r++];
    }
    out[w] = 0;
    return w;
}

static int has_attr(const char* in, int s, int e, const char* name) {
    char v[4];
    int i, l = 0;
    if (get_attr(in, s, e, name, v, (int)sizeof(v)) >= 0) return 1;
    while (name[l]) l++;
    for (i = s; i + l <= e; i++) {
        int k;
        if (!is_sp(in[i - 1])) continue;
        for (k = 0; k < l && lc(in[i + k]) == name[k]; k++) {}
        if (k == l && (i + l == e || is_sp(in[i + l]) || in[i + l] == '>' || in[i + l] == '/')) return 1;
    }
    return 0;
}

static void put_num(ob_t* b, int v) {
    char t[8];
    int k = 0;
    do { t[k++] = (char)('0' + v % 10); v /= 10; } while (v && k < 7);
    while (k) ob_c(b, t[--k]);
}

/* A form control becomes a link "mohhdy-form:f:N" (or s: for a submit)
 * whose text is the placeholder \001N\002, replaced by the section
 * builder (wf_html_section) with the current value of field N. */
static void put_field(ob_t* b, int idx, int submit) {
    ob_s(b, "<a href=\"mohhdy-form:");
    ob_c(b, submit ? 's' : 'f');
    ob_c(b, ':');
    put_num(b, idx);
    ob_s(b, "\">");
    ob_c(b, '\001');
    put_num(b, idx);
    ob_c(b, '\002');
    ob_s(b, "</a>");
    b->nodes++;
}

/* Inner text of the element whose open tag ends at i (for <button>,
 * <textarea>, <option>), decoded, up to the closing tag. */
static int inner_text(const char* in, int n, int i, const char* name, char* out, int cap, int collapse) {
    int end = skip_block(in, n, i, name), close = end;
    while (close > i && in[close - 1] != '<') close--;
    if (close > i) close--;
    if (close < i) close = i;
    {
        /* drop nested tags */
        static char tmp[WF_FORM_VALUE * 2];
        int k = 0, j, intag = 0;
        for (j = i; j < close && k < (int)sizeof(tmp) - 1; j++) {
            if (in[j] == '<') { intag = 1; continue; }
            if (in[j] == '>') { intag = 0; continue; }
            if (!intag) tmp[k++] = in[j];
        }
        wf_text_display(tmp, k, g_cs, collapse, out, cap);
    }
    return end;
}

/* <select>: every option (value, label, selected) goes to the form; the
 * selected one, else the first, is the value. */
static int select_options(const char* in, int n, int i, int field) {
    int end = skip_block(in, n, i, "select"), j;
    for (j = i; j + 7 < end; j++) {
        if (in[j] == '<' && lc(in[j + 1]) == 'o' && lc(in[j + 2]) == 'p' && lc(in[j + 3]) == 't') {
            int e = j, q = 0, hv;
            static char v[WF_FORM_VALUE], l[WF_FORM_LABEL];
            while (e < end && (q || in[e] != '>')) { if (in[e] == '"' || in[e] == '\'') q = (q == in[e]) ? 0 : (q ? q : in[e]); e++; }
            hv = get_attr_text(in, j + 7, e, "value", v, (int)sizeof(v)) >= 0;
            inner_text(in, end, e + 1, "option", l, (int)sizeof(l), 1);
            if (!hv) { int k; for (k = 0; l[k] && k < (int)sizeof(v) - 1; k++) v[k] = l[k]; v[k] = 0; }
            if (field >= 0 && !has_attr(in, j + 7, e, "disabled"))
                (void)wf_form_add_option(g_forms, field, v, l, has_attr(in, j + 7, e, "selected"));
            j = e;
        }
    }
    return end;
}

/* Returns the next read position after a form element at in[i] (tag
 * name name, attributes in[j..e)), or -1 when it is not a form element. */
static int form_tag(ob_t* b, const char* in, int n, int i, int j, int e, int close, const char* name, const wf_url_t* base) {
    static char type[24], fname[WF_FORM_NAME], value[WF_FORM_VALUE], label[WF_FORM_LABEL], action[WF_FORM_ACTION], method[8], enc[40], num[8];
    int next = e < n ? e + 1 : n, idx, maxlen = 0, k;
    (void)i;
    if (seq(name, "form")) {
        if (close) { wf_form_end(g_forms); return next; }
        get_attr_text(in, j, e, "action", action, (int)sizeof(action));
        get_attr(in, j, e, "method", method, (int)sizeof(method));
        get_attr(in, j, e, "enctype", enc, (int)sizeof(enc));
        { int r = 0, w = 0; while (action[r]) { if ((unsigned char)action[r] >= 0x20u) action[w++] = action[r]; r++; } action[w] = 0; }
        wf_form_begin(g_forms, base, action, method, enc);
        return next;
    }
    if (close) return seq(name, "input") || seq(name, "textarea") || seq(name, "button") || seq(name, "select") ? next : -1;
    if (!seq(name, "input") && !seq(name, "textarea") && !seq(name, "button") && !seq(name, "select")) return -1;
    get_attr_text(in, j, e, "name", fname, (int)sizeof(fname));
    if (get_attr_text(in, j, e, "placeholder", label, (int)sizeof(label)) <= 0 &&
        get_attr_text(in, j, e, "aria-label", label, (int)sizeof(label)) <= 0)
        get_attr_text(in, j, e, "title", label, (int)sizeof(label));
    if (get_attr(in, j, e, "maxlength", num, (int)sizeof(num)) > 0)
        for (k = 0; num[k] >= '0' && num[k] <= '9'; k++) maxlen = maxlen * 10 + (num[k] - '0');
    if (has_attr(in, j, e, "disabled")) {
        if (seq(name, "input")) return next;
        return skip_block(in, n, next, name);
    }
    if (seq(name, "textarea")) {
        next = inner_text(in, n, next, "textarea", value, (int)sizeof(value), 0);
        idx = wf_form_add(g_forms, base, "textarea", fname, value, label, 0, maxlen);
        if (idx >= 0) put_field(b, idx, 0);
        return next;
    }
    if (seq(name, "select")) {
        idx = wf_form_add(g_forms, base, "select", fname, "", label, 0, 0);
        next = select_options(in, n, next, idx);
        if (idx >= 0) put_field(b, idx, 0);
        return next;
    }
    if (seq(name, "button")) {
        if (get_attr(in, j, e, "type", type, (int)sizeof(type)) < 0) type[0] = 0;
        get_attr_text(in, j, e, "value", value, (int)sizeof(value));
        next = inner_text(in, n, next, "button", label, (int)sizeof(label), 1);
        if (type[0] && !seq(type, "submit")) return next;
        idx = wf_form_add(g_forms, base, "button-submit", fname, value, label, 0, 0);
        if (idx >= 0) put_field(b, idx, 1);
        return next;
    }
    if (get_attr(in, j, e, "type", type, (int)sizeof(type)) < 0) type[0] = 0;
    for (k = 0; type[k]; k++) type[k] = lc(type[k]);
    get_attr_text(in, j, e, "value", value, (int)sizeof(value));
    if (seq(type, "submit") || seq(type, "image")) {
        if (seq(type, "image") && !label[0]) get_attr_text(in, j, e, "alt", label, (int)sizeof(label));
        if (seq(type, "submit")) label[0] = 0;   /* a submit shows its value */
    }
    idx = wf_form_add(g_forms, base, type, fname, value, label, has_attr(in, j, e, "checked"), maxlen);
    if (idx >= 0 && g_forms->field[idx].type != WF_FIELD_HIDDEN) put_field(b, idx, g_forms->field[idx].type == WF_FIELD_SUBMIT);
    return next;
}

int wf_html_reduce(const char* in, int n, const wf_url_t* base, char* out, int cap, wf_html_stats_t* st) {
    return wf_html_reduce_ex(in, n, base, out, cap, st, WF_HTML_NODE_BUDGET, 0);
}

int wf_html_reduce_ex(const char* in, int n, const wf_url_t* base, char* out, int cap, wf_html_stats_t* st,
                      int node_budget, wf_form_page_t* forms) {
    static wf_url_t link;
    static char href[WF_URL_PATH_MAX], abs_url[WF_URL_HOST_MAX + WF_URL_PATH_MAX + 16];
    ob_t b;
    int i = 0, last_sp = 1, pre = 0, in_title = 0, tn = 0, k, body_seen = 0;
    wf_html_stats_t dummy;
    if (!st) st = &dummy;
    for (k = 0; k < (int)sizeof(*st); k++) ((char*)st)[k] = 0;
    st->in_bytes = n;
    g_forms = forms;
    if (forms) wf_form_reset(forms);
    g_cs = g_cs_set;
    if (g_cs == WF_CS_NONE) {
        int sn = wf_charset_sniff(in, n);
        g_cs = sn >= 0 ? sn : WF_CS_UTF8;
    }
    st->charset = g_cs;
    if (forms) forms->latin1 = g_cs == WF_CS_LATIN1;
    if (node_budget <= 0) node_budget = WF_HTML_NODE_BUDGET;
    b.o = out; b.cap = cap; b.n = 0; b.full = 0; b.nodes = 0;
    if (cap < 64) { if (cap > 0) out[0] = 0; return 0; }
    /* the title first, from <head> */
    {
        int t = 0;
        while (t + 7 < n) {
            if (in[t] == '<' && lc(in[t + 1]) == 't' && lc(in[t + 2]) == 'i' && lc(in[t + 3]) == 't' &&
                lc(in[t + 4]) == 'l' && lc(in[t + 5]) == 'e' && (in[t + 6] == '>' || is_sp(in[t + 6]))) {
                while (t < n && in[t] != '>') t++;
                t++;
                {
                    int t0 = t;
                    while (t < n && in[t] != '<') t++;
                    tn = wf_text_display(in + t0, t - t0, g_cs, 1, st->title, (int)sizeof(st->title));
                }
                {   /* < and > of a title become spaces (it is re-escaped when written) */
                    int q;
                    for (q = 0; q < tn; q++) if (st->title[q] == '<' || st->title[q] == '>') st->title[q] = ' ';
                }
                break;
            }
            t++;
        }
    }
    ob_s(&b, "<html><head><title>");
    if (!st->title[0]) ob_s(&b, "(untitled)");
    for (k = 0; st->title[k]; k++) {
        if (st->title[k] == '&') ob_s(&b, "&amp;");
        else { char one[2]; one[0] = st->title[k]; one[1] = 0; ob_s(&b, one); }
    }
    ob_s(&b, "</title></head><body>");
    (void)in_title;
    while (i < n && !b.full) {
        if (b.nodes >= node_budget) { b.full = 1; break; }
        if (in[i] == '<') {
            char name[16];
            int close, j, e;
            if (i + 3 < n && in[i + 1] == '!' && in[i + 2] == '-' && in[i + 3] == '-') {   /* comment */
                j = i + 4;
                while (j + 2 < n && !(in[j] == '-' && in[j + 1] == '-' && in[j + 2] == '>')) j++;
                i = j + 3;
                continue;
            }
            if (i + 1 < n && (in[i + 1] == '!' || in[i + 1] == '?')) {   /* doctype, PI */
                while (i < n && in[i] != '>') i++;
                i++;
                continue;
            }
            close = (i + 1 < n && in[i + 1] == '/');
            j = tag_name(in, n, i + 1 + close, name, (int)sizeof(name));
            e = j;
            {
                int q = 0;
                while (e < n && (q || in[e] != '>')) {
                    if (in[e] == '"' || in[e] == '\'') q = (q == in[e]) ? 0 : (q ? q : in[e]);
                    e++;
                }
            }
            if (!name[0]) { put_text(&b, in, i, i + 1, &last_sp, pre); i++; continue; }   /* a lone '<' */
            if (seq(name, "body")) body_seen = 1;
            if (g_forms) {
                int nx;
                if (!ob_room(&b, 600)) { b.full = 1; break; }
                nx = form_tag(&b, in, n, i, j, e, close, name, base);
                if (nx >= 0) { last_sp = 0; i = nx; continue; }
            }
            if (!close && in_list(name, DROP_BLOCK)) {
                st->blocks_dropped++;
                i = skip_block(in, n, e < n ? e + 1 : n, name);
                continue;
            }
            if (seq(name, "title")) { i = skip_block(in, n, e < n ? e + 1 : n, name); continue; }
            if (!close && (in_list(name, KEEP) || in_list(name, AS_P) || seq(name, "span")) && e < n && in[e - 1] != '/' &&
                chrome_block(in, j, e)) {
                st->blocks_dropped++;
                i = skip_block(in, n, e + 1, name);
                continue;
            }
            if (in_list(name, KEEP)) {
                if (!ob_room(&b, 600)) { b.full = 1; break; }
                st->tags_kept++;
                if (!close) b.nodes++;
                if (seq(name, "pre")) pre = !close;
                if (!close && seq(name, "a")) {
                    int hl = get_href(in, j, e, href, (int)sizeof(href));
                    ob_s(&b, "<a");
                    if (hl > 0 && base && href_web(href) && wf_url_resolve(base, href, &link) == 0) {
                        wf_url_format(&link, abs_url, (int)sizeof(abs_url));
                        ob_s(&b, " href=\"");
                        for (k = 0; abs_url[k]; k++) if (abs_url[k] != '"') ob_c(&b, abs_url[k]);
                        ob_c(&b, '"');
                        st->links++;
                    }
                    ob_c(&b, '>');
                } else {
                    ob_c(&b, '<');
                    if (close) ob_c(&b, '/');
                    ob_s(&b, name);
                    ob_c(&b, '>');
                }
                if (!seq(name, "a") && !seq(name, "b") && !seq(name, "i") && !seq(name, "em") && !seq(name, "strong") &&
                    !seq(name, "code")) last_sp = 1;
            } else if (in_list(name, AS_P)) {
                if (!ob_room(&b, 600)) { b.full = 1; break; }
                st->tags_kept++;
                if (close) {
                    if (ends_with(&b, "<p>")) { b.n -= 3; b.nodes--; }      /* empty paragraph: dropped */
                    else if (!ends_with(&b, "</p>")) ob_s(&b, "</p>");
                } else if (!ends_with(&b, "<p>")) {
                    ob_s(&b, "<p>");
                    b.nodes++;
                }
                last_sp = 1;
            } else {
                st->tags_dropped++;
            }
            i = e < n ? e + 1 : n;
            continue;
        }
        {
            int s = i;
            while (i < n && in[i] != '<') i++;
            if (!ob_room(&b, 32)) { b.full = 1; break; }
            put_text(&b, in, s, i, &last_sp, pre);
        }
    }
    (void)body_seen;
    st->truncated = b.full;
    /* room was kept for the closing tags */
    b.full = 0;
    if (b.n > cap - 40) b.n = cap - 40;
    if (st->truncated) ob_s(&b, "<p>[page tronquee]</p>");
    ob_s(&b, "</body></html>");
    b.o[b.n] = 0;
    st->out_bytes = b.n;
    return b.n;
}

static int html_text(const char* in, int n, char* out, int cap, int display);
int wf_html_text(const char* in, int n, char* out, int cap) { return html_text(in, n, out, cap, 0); }
/* Lot 71: text of a reduced page (display bytes, wf_html_reduce output):
 * bytes >= 0x80 are CP1252 display bytes, written as UTF-8. */
int wf_html_text_display(const char* in, int n, char* out, int cap) { return html_text(in, n, out, cap, 1); }
static int html_text(const char* in, int n, char* out, int cap, int display) {
    ob_t b;
    int i = 0, last_sp = 1, nl = 1, cs = g_cs_set;
    if (!display && cs == WF_CS_NONE) {
        int sn = wf_charset_sniff(in, n);
        cs = sn >= 0 ? sn : WF_CS_UTF8;
    }
    b.o = out; b.cap = cap; b.n = 0; b.full = 0;
    if (cap <= 0) return 0;
    while (i < n && !b.full) {
        if (in[i] == '<') {
            char name[16];
            int close = (i + 1 < n && in[i + 1] == '/'), j, e;
            j = tag_name(in, n, i + 1 + close, name, (int)sizeof(name));
            e = j;
            while (e < n && in[e] != '>') e++;
            if (!close && (in_list(name, DROP_BLOCK) || seq(name, "title"))) { i = skip_block(in, n, e < n ? e + 1 : n, name); continue; }
            if (seq(name, "p") || seq(name, "br") || seq(name, "li") || seq(name, "div") || seq(name, "tr") ||
                (name[0] == 'h' && name[1] >= '1' && name[1] <= '6' && !name[2])) {
                if (!nl) { ob_c(&b, '\n'); nl = 1; last_sp = 1; }
                if (!close && seq(name, "li")) { ob_s(&b, "- "); nl = 0; }
            }
            i = e < n ? e + 1 : n;
            continue;
        }
        if (display && (unsigned char)in[i] >= 0x80U) {
            char u[4];
            int k, m = os_display_to_utf8((unsigned char)in[i], u, (int)sizeof(u));
            for (k = 0; k < m; k++) ob_c(&b, u[k]);
            i++; nl = 0; last_sp = 0;
            continue;
        }
        if (in[i] == '&' || (!display && cs == WF_CS_LATIN1 && (unsigned char)in[i] >= 0x80U)) {
            /* Lot 71: any character reference, and Latin-1 bytes, as UTF-8 */
            int k = i;
            uint32_t cp = wf_text_next(in, n, &k, cs, 1);
            if (cp && (k > i + 1 || cp != '&')) {
                if (cp == 0xA0U) cp = ' ';
                if (cp < 0x80U) ob_c(&b, (char)cp);
                else if (cp < 0x800U) { ob_c(&b, (char)(0xC0U | (cp >> 6))); ob_c(&b, (char)(0x80U | (cp & 0x3FU))); }
                else if (cp < 0x10000U) {
                    ob_c(&b, (char)(0xE0U | (cp >> 12))); ob_c(&b, (char)(0x80U | ((cp >> 6) & 0x3FU))); ob_c(&b, (char)(0x80U | (cp & 0x3FU)));
                } else {
                    ob_c(&b, (char)(0xF0U | (cp >> 18))); ob_c(&b, (char)(0x80U | ((cp >> 12) & 0x3FU)));
                    ob_c(&b, (char)(0x80U | ((cp >> 6) & 0x3FU))); ob_c(&b, (char)(0x80U | (cp & 0x3FU)));
                }
                i = k; nl = 0; last_sp = cp == ' ';
                continue;
            }
        }
        if (is_sp(in[i])) {
            if (!last_sp) { ob_c(&b, ' '); last_sp = 1; }
        } else if ((unsigned char)in[i] >= 0x20U) {
            ob_c(&b, in[i]);
            last_sp = 0;
            nl = 0;
        }
        i++;
    }
    out[b.n] = 0;
    return b.n;
}
