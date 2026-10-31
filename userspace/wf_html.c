/* wf_html.c - Lot 67: reader-view reduction of a fetched page (see wf_html.h). */
#include "wf_html.h"

static char lc(char c) { return (c >= 'A' && c <= 'Z') ? (char)(c + 32) : c; }
static int is_sp(char c) { return c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == '\f'; }

typedef struct { char* o; int cap; int n; int full; } ob_t;

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

static int in_list(const char* name, const char* const* list) {
    int i;
    for (i = 0; list[i]; i++) if (seq(name, list[i])) return 1;
    return 0;
}

static const char* const DROP_BLOCK[] = {"script", "style", "noscript", "svg", "template", "iframe", "object",
                                         "canvas", "form", "select", "textarea", "button", "head", "nav", "footer", 0};
static const char* const KEEP[] = {"h1", "h2", "h3", "h4", "h5", "h6", "p", "ul", "ol", "li", "a", "br", "pre",
                                   "b", "strong", "i", "em", "code", "blockquote", "hr", 0};
static const char* const AS_P[] = {"div", "section", "article", "main", "header", "tr", "dt", "dd", "td", "th",
                                   "figcaption", "caption", "summary", "details", "table", "aside", 0};

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

/* copies text, escaping < and > that would confuse the engine; collapses spaces */
static void put_text(ob_t* b, const char* in, int s, int e, int* last_sp, int pre) {
    int i;
    for (i = s; i < e; i++) {
        char c = in[i];
        if (!pre && is_sp(c)) {
            if (!*last_sp) { ob_c(b, ' '); *last_sp = 1; }
            continue;
        }
        *last_sp = 0;
        if ((unsigned char)c < 0x20U && c != '\n' && c != '\t') continue;
        ob_c(b, c);
    }
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

/* Decodes the common entities of a title in place; < and > become spaces
 * (the title is re-escaped when written). */
static void title_decode(char* t) {
    static const char* const ent[] = {"&amp;", "&", "&lt;", " ", "&gt;", " ", "&quot;", "\"", "&#39;", "'",
                                      "&apos;", "'", "&nbsp;", " ", 0};
    int r = 0, w = 0, j;
    while (t[r]) {
        int hit = 0;
        if (t[r] == '&') {
            for (j = 0; ent[j]; j += 2) {
                int l = 0;
                while (ent[j][l] && t[r + l] == ent[j][l]) l++;
                if (!ent[j][l]) { t[w++] = ent[j + 1][0]; r += l; hit = 1; break; }
            }
        }
        if (!hit) t[w++] = t[r++];
    }
    t[w] = 0;
}

int wf_html_reduce(const char* in, int n, const wf_url_t* base, char* out, int cap, wf_html_stats_t* st) {
    static wf_url_t link;
    static char href[WF_URL_PATH_MAX], abs_url[WF_URL_HOST_MAX + WF_URL_PATH_MAX + 16];
    ob_t b;
    int i = 0, last_sp = 1, pre = 0, in_title = 0, tn = 0, k, body_seen = 0;
    wf_html_stats_t dummy;
    if (!st) st = &dummy;
    for (k = 0; k < (int)sizeof(*st); k++) ((char*)st)[k] = 0;
    st->in_bytes = n;
    b.o = out; b.cap = cap; b.n = 0; b.full = 0;
    if (cap < 64) { if (cap > 0) out[0] = 0; return 0; }
    /* the title first, from <head> */
    {
        int t = 0;
        while (t + 7 < n) {
            if (in[t] == '<' && lc(in[t + 1]) == 't' && lc(in[t + 2]) == 'i' && lc(in[t + 3]) == 't' &&
                lc(in[t + 4]) == 'l' && lc(in[t + 5]) == 'e' && (in[t + 6] == '>' || is_sp(in[t + 6]))) {
                while (t < n && in[t] != '>') t++;
                t++;
                while (t < n && in[t] != '<' && tn < (int)sizeof(st->title) - 1) {
                    char c = in[t++];
                    if (is_sp(c)) { if (tn && st->title[tn - 1] != ' ') st->title[tn++] = ' '; }
                    else if ((unsigned char)c >= 0x20U && c != '<' && c != '>') st->title[tn++] = c;
                }
                while (tn && st->title[tn - 1] == ' ') tn--;
                st->title[tn] = 0;
                title_decode(st->title);
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
            if (!close && in_list(name, DROP_BLOCK)) {
                st->blocks_dropped++;
                i = skip_block(in, n, e < n ? e + 1 : n, name);
                continue;
            }
            if (seq(name, "title")) { i = skip_block(in, n, e < n ? e + 1 : n, name); continue; }
            if (in_list(name, KEEP)) {
                if (!ob_room(&b, 600)) { b.full = 1; break; }
                st->tags_kept++;
                if (seq(name, "pre")) pre = !close;
                if (!close && seq(name, "a")) {
                    int hl = get_attr(in, j, e, "href", href, (int)sizeof(href));
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
                ob_s(&b, close ? "</p>" : "<p>");
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

int wf_html_text(const char* in, int n, char* out, int cap) {
    ob_t b;
    int i = 0, last_sp = 1, nl = 1;
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
        if (in[i] == '&') {
            static const char* const ent[] = {"&amp;", "&", "&lt;", "<", "&gt;", ">", "&quot;", "\"", "&#39;", "'",
                                              "&nbsp;", " ", "&#x27;", "'", 0};
            int k, m = 0;
            for (k = 0; ent[k]; k += 2) {
                int l = 0;
                while (ent[k][l] && i + l < n && lc(in[i + l]) == ent[k][l]) l++;
                if (!ent[k][l]) { ob_s(&b, ent[k + 1]); i += l; m = 1; nl = 0; last_sp = 0; break; }
            }
            if (m) continue;
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
