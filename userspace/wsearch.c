/* wsearch.c - Lot 69: Internet search without JavaScript (see wsearch.h). */
#include "wsearch.h"
#include "../include/os_charset.h"

static int w_len(const char *s) { int n = 0; while (s && s[n]) n++; return n; }
static int w_low(int c) { return c >= 'A' && c <= 'Z' ? c + 32 : c; }
static int w_pre_ci(const char *s, int n, const char *p) {
    int i;
    for (i = 0; p[i]; i++) if (i >= n || w_low((unsigned char)s[i]) != w_low((unsigned char)p[i])) return 0;
    return 1;
}
static int w_find_ci(const char *s, int n, const char *p) {
    int i;
    for (i = 0; i < n; i++) if (w_pre_ci(s + i, n - i, p)) return i;
    return -1;
}
static void w_cpy(char *d, int cap, const char *s) {
    int i = 0;
    if (cap <= 0) return;
    while (s && s[i] && i < cap - 1) { d[i] = s[i]; i++; }
    d[i] = 0;
}
static int w_cat(char *d, int cap, int n, const char *s) {
    while (s && *s && n < cap - 1) d[n++] = *s++;
    if (cap > 0) d[n < cap ? n : cap - 1] = 0;
    return n;
}
static int hexv(int c) {
    if (c >= '0' && c <= '9') return c - '0';
    c = w_low(c);
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    return -1;
}

int wsearch_urlencode(const char *in, char *out, int cap) {
    static const char hx[] = "0123456789ABCDEF";
    int n = 0;
    if (!out || cap <= 0) return -1;
    out[0] = 0;
    if (!in) return 0;
    for (; *in; in++) {
        unsigned char c = (unsigned char)*in;
        if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') ||
            c == '-' || c == '_' || c == '.' || c == '~') {
            if (n + 1 >= cap) return -1;
            out[n++] = (char)c;
        } else if (c == ' ') {
            if (n + 1 >= cap) return -1;
            out[n++] = '+';
        } else {
            if (n + 3 >= cap) return -1;
            out[n++] = '%'; out[n++] = hx[c >> 4]; out[n++] = hx[c & 15];
        }
    }
    out[n] = 0;
    return n;
}

int wsearch_urldecode(const char *in, int len, char *out, int cap) {
    int i, n = 0;
    if (!out || cap <= 0) return -1;
    for (i = 0; i < len && in[i] && n < cap - 1; i++) {
        int c = (unsigned char)in[i];
        if (c == '%' && i + 2 < len && hexv(in[i + 1]) >= 0 && hexv(in[i + 2]) >= 0) {
            c = hexv(in[i + 1]) * 16 + hexv(in[i + 2]);
            i += 2;
        } else if (c == '+') c = ' ';
        out[n++] = (char)c;
    }
    out[n] = 0;
    return n;
}

static const char *const k_names[] = { "ddg", "ddg-html", "ddg-lite", 0 };
static const char *const k_tpl[] = { "https://html.duckduckgo.com/html/?q=%s", "https://html.duckduckgo.com/html/?q=%s",
                                     "https://lite.duckduckgo.com/lite/?q=%s", 0 };

const char *wsearch_engine_template(const char *name) {
    int i, k;
    if (!name) return 0;
    for (i = 0; k_names[i]; i++) {
        for (k = 0; name[k] && name[k] == k_names[i][k]; k++) {}
        if (!name[k] && !k_names[i][k]) return k_tpl[i];
    }
    return 0;
}

int wsearch_build_url(const char *engine, const char *query, char *out, int cap) {
    char enc[WSEARCH_QUERY * 3 + 4];
    const char *t = wsearch_engine_template(engine ? engine : "ddg");
    int i, n = 0, put = 0, tl;
    if (!out || cap <= 0) return -1;
    out[0] = 0;
    if (!t) t = engine;
    if (!t || !(w_pre_ci(t, w_len(t), "http://") || w_pre_ci(t, w_len(t), "https://"))) return -1;
    if (!query || !query[0] || w_len(query) > WSEARCH_QUERY) return -1;
    if (wsearch_urlencode(query, enc, (int)sizeof(enc)) < 0) return -1;
    tl = w_len(t);
    for (i = 0; i < tl; i++) {
        if (t[i] == '%' && t[i + 1] == 's' && !put) {
            int k;
            for (k = 0; enc[k]; k++) { if (n >= cap - 1) return -1; out[n++] = enc[k]; }
            put = 1; i++;
        } else {
            if (n >= cap - 1) return -1;
            out[n++] = t[i];
        }
    }
    out[n] = 0;
    if (!put) {
        if (tl == 0 || t[tl - 1] != '=') return -1;
        if (n + w_len(enc) >= cap) return -1;
        n = w_cat(out, cap, n, enc);
    }
    return 0;
}

int wsearch_decode_href(const char *href, char *out, int cap) {
    char tmp[WSEARCH_URL + 8];
    int n = w_len(href), u;
    if (!out || cap <= 0) return -1;
    out[0] = 0;
    if (!href || !n) return -1;
    /* the page escapes & as &amp; in attributes */
    {
        int i, k = 0;
        for (i = 0; i < n && k < (int)sizeof(tmp) - 1; i++) {
            if (href[i] == '&' && w_pre_ci(href + i, n - i, "&amp;")) { tmp[k++] = '&'; i += 4; }
            else tmp[k++] = href[i];
        }
        tmp[k] = 0;
        n = k;
    }
    u = w_find_ci(tmp, n, "uddg=");
    if (u >= 0) {
        int e = u + 5;
        while (e < n && tmp[e] != '&') e++;
        if (wsearch_urldecode(tmp + u + 5, e - u - 5, out, cap) <= 0) return -1;
    } else if (tmp[0] == '/' && tmp[1] == '/') {
        int k = w_cat(out, cap, 0, "https:");
        w_cat(out, cap, k, tmp);
    } else w_cpy(out, cap, tmp);
    if (!(w_pre_ci(out, w_len(out), "http://") || w_pre_ci(out, w_len(out), "https://"))) { out[0] = 0; return -1; }
    {
        int i, l = w_len(out);
        for (i = 0; i < l; i++) if ((unsigned char)out[i] <= ' ' || out[i] == '"' || out[i] == '<' || out[i] == '>') { out[0] = 0; return -1; }
    }
    return w_len(out) >= cap - 1 ? -1 : 0;
}

/* text of html[a..b) without tags, entities decoded, spaces collapsed */
static void text_of(const char *h, int a, int b, char *out, int cap) {
    int i, n = 0, sp = 0;
    if (cap <= 0) return;
    for (i = a; i < b && n < cap - 1; i++) {
        char c = h[i];
        if (c == '<') { while (i < b && h[i] != '>') i++; sp = n > 0; continue; }
        if (c == '&') {
            static const char *const ent[] = { "&amp;", "&lt;", "&gt;", "&quot;", "&#39;", "&#x27;", "&nbsp;", "&apos;", 0 };
            static const char rep[] = { '&', '<', '>', '"', '\'', '\'', ' ', '\'' };
            int k, hit = 0;
            for (k = 0; ent[k]; k++) if (w_pre_ci(h + i, b - i, ent[k])) { c = rep[k]; i += w_len(ent[k]) - 1; hit = 1; break; }
            if (!hit && h[i + 1] == '#') {
                int j = i + 2, v = 0;
                while (j < b && h[j] >= '0' && h[j] <= '9' && v < 100000) v = v * 10 + (h[j++] - '0');
                if (j < b && h[j] == ';' && j > i + 2) { c = v < 128 && v >= 32 ? (char)v : '?'; i = j; }
            }
        }
        if ((unsigned char)c < 32 || c == ' ') { sp = n > 0; continue; }
        if (sp && n < cap - 2) out[n++] = ' ';
        sp = 0;
        out[n++] = c;
    }
    out[n] = 0;
}

/* value of attribute name inside the tag html[a..b) */
static int attr(const char *h, int a, int b, const char *name, char *out, int cap) {
    int i, nl = w_len(name);
    out[0] = 0;
    for (i = a; i + nl < b; i++) {
        if ((h[i - 1] == ' ' || h[i - 1] == '\t' || h[i - 1] == '\n') && w_pre_ci(h + i, b - i, name)) {
            int j = i + nl, q, k = 0;
            while (j < b && h[j] == ' ') j++;
            if (j >= b || h[j] != '=') continue;
            j++;
            while (j < b && h[j] == ' ') j++;
            q = (h[j] == '"' || h[j] == '\'') ? h[j++] : 0;
            while (j < b && k < cap - 1 && (q ? h[j] != q : (h[j] != ' ' && h[j] != '>'))) out[k++] = h[j++];
            out[k] = 0;
            return k;
        }
    }
    return 0;
}

static int class_has(const char *cls, const char *want) {
    int n = w_len(cls), p = 0, wl = w_len(want);
    while (p < n) {
        int s = p;
        while (p < n && cls[p] != ' ') p++;
        if (p - s == wl && w_pre_ci(cls + s, p - s, want)) return 1;
        while (p < n && cls[p] == ' ') p++;
    }
    return 0;
}

static int dup_url(const wsearch_page_t *pg, const char *u) {
    int i, k;
    for (i = 0; i < pg->n; i++) {
        for (k = 0; u[k] && pg->r[i].url[k] == u[k]; k++) {}
        if (!u[k] && !pg->r[i].url[k]) return 1;
    }
    return 0;
}

static int internal_host(const char *u) {
    int n = w_len(u), s = w_find_ci(u, n, "://"), e;
    if (s < 0) return 1;
    s += 3; e = s;
    while (e < n && u[e] != '/' && u[e] != '?' && u[e] != ':') e++;
    if (e - s >= 14 && w_pre_ci(u + e - 14, 14, "duckduckgo.com")) return 1;
    return 0;
}

/* end of the element that starts at tag position a (matching close tag of
 * the same name, bounded search) */
static int elem_end(const char *h, int len, int open_end, const char *tag) {
    char close[16];
    int k = w_cat(close, (int)sizeof(close), 0, "</");
    int e;
    w_cat(close, (int)sizeof(close), k, tag);
    e = w_find_ci(h + open_end, len - open_end > 4096 ? 4096 : len - open_end, close);
    return e < 0 ? -1 : open_end + e;
}

static void add_result(wsearch_page_t *pg, const char *href, const char *h, int ta, int tb) {
    char url[WSEARCH_URL];
    wsearch_result_t *r;
    if (pg->n >= WSEARCH_MAX) return;
    if (wsearch_decode_href(href, url, (int)sizeof(url)) != 0 || internal_host(url) || dup_url(pg, url)) { pg->skipped++; return; }
    r = &pg->r[pg->n];
    text_of(h, ta, tb, r->title, (int)sizeof(r->title));
    if (!r->title[0]) { pg->skipped++; return; }
    w_cpy(r->url, (int)sizeof(r->url), url);
    r->snippet[0] = 0;
    pg->n++;
}

/* one pass over every tag; class names decide what a tag is */
static void scan(const char *h, int len, wsearch_page_t *pg, int layout) {
    int i = 0;
    char cls[128], href[WSEARCH_URL + 64], tag[12];
    while (i < len && pg->n <= WSEARCH_MAX) {
        int a, b, k, t, e;
        if (h[i] != '<' || h[i + 1] == '/' || h[i + 1] == '!') { i++; continue; }
        a = i + 1;
        b = a;
        while (b < len && h[b] != '>' && b - a < 2048) b++;
        if (b >= len || h[b] != '>') break;
        for (k = 0, t = a; t < b && h[t] != ' ' && h[t] != '\n' && k < 11; t++) tag[k++] = (char)w_low((unsigned char)h[t]);
        tag[k] = 0;
        attr(h, a, b, "class", cls, (int)sizeof(cls));
        if (tag[0] == 'a' && tag[1] == 0) {
            int is_title = layout == WSEARCH_LAYOUT_DDG_HTML ? class_has(cls, "result__a")
                         : layout == WSEARCH_LAYOUT_DDG_LITE ? class_has(cls, "result-link")
                         : 1;
            if (is_title && attr(h, a, b, "href", href, (int)sizeof(href)) > 0) {
                e = elem_end(h, len, b + 1, "a");
                if (e > 0) {
                    if (pg->n < WSEARCH_MAX) add_result(pg, href, h, b + 1, e);
                    else pg->skipped++;
                    i = e;
                    continue;
                }
            }
        }
        if (pg->n > 0 && !pg->r[pg->n - 1].snippet[0] &&
            ((layout == WSEARCH_LAYOUT_DDG_HTML && class_has(cls, "result__snippet")) ||
             (layout == WSEARCH_LAYOUT_DDG_LITE && class_has(cls, "result-snippet")))) {
            e = elem_end(h, len, b + 1, tag);
            if (e > 0) {
                text_of(h, b + 1, e, pg->r[pg->n - 1].snippet, WSEARCH_SNIPPET);
                i = e;
                continue;
            }
        }
        i = b + 1;
    }
}

int wsearch_parse(const char *html, int len, wsearch_page_t *pg) {
    int i;
    if (!pg) return 0;
    for (i = 0; i < (int)sizeof(*pg); i++) ((char *)pg)[i] = 0;
    if (!html || len <= 0) return 0;
    if (w_find_ci(html, len, "anomaly-modal") >= 0 || w_find_ci(html, len, "bots use duckduckgo") >= 0 ||
        w_find_ci(html, len, "g-recaptcha") >= 0 || w_find_ci(html, len, "challenge-form") >= 0)
        pg->blocked = 1;
    if (w_find_ci(html, len, "result__a") >= 0) { scan(html, len, pg, WSEARCH_LAYOUT_DDG_HTML); if (pg->n) pg->layout = WSEARCH_LAYOUT_DDG_HTML; }
    if (!pg->n && w_find_ci(html, len, "result-link") >= 0) { pg->skipped = 0; scan(html, len, pg, WSEARCH_LAYOUT_DDG_LITE); if (pg->n) pg->layout = WSEARCH_LAYOUT_DDG_LITE; }
    if (!pg->n && !pg->blocked) { pg->skipped = 0; scan(html, len, pg, WSEARCH_LAYOUT_GENERIC); if (pg->n) pg->layout = WSEARCH_LAYOUT_GENERIC; }
    return pg->n;
}

static int cat_int(char *o, int cap, int n, int v) {
    char t[12];
    int k = 0;
    do { t[k++] = (char)('0' + v % 10); v /= 10; } while (v && k < 11);
    while (k && n < cap - 1) o[n++] = t[--k];
    o[n] = 0;
    return n;
}

int wsearch_format(const wsearch_page_t *pg, char *o, int cap) {
    int i, n = 0;
    if (!o || cap <= 0) return 0;
    o[0] = 0;
    for (i = 0; pg && i < pg->n; i++) {
        n = w_cat(o, cap, n, "search ");
        n = cat_int(o, cap, n, i + 1);
        n = w_cat(o, cap, n, " ");
        n = w_cat(o, cap, n, pg->r[i].title);
        n = w_cat(o, cap, n, "\n  ");
        n = w_cat(o, cap, n, pg->r[i].url);
        n = w_cat(o, cap, n, "\n");
        if (pg->r[i].snippet[0]) {
            n = w_cat(o, cap, n, "  ");
            n = w_cat(o, cap, n, pg->r[i].snippet);
            n = w_cat(o, cap, n, "\n");
        }
    }
    return n;
}

/* text escaped for HTML */
/* Lot 71: the results are UTF-8 (console); the page for the web engine
 * gets display bytes (include/os_charset.h) so accents are drawn. */
/* Lot 72: Greek, Cyrillic... with their glyph page selector; the text
 * ends in page 0 */
static int cat_esc(char *o, int cap, int n, const char *s) {
    int sl = w_len(s), page = 0;
    for (; s && *s && n < cap - 7; s++) {
        if ((unsigned char)*s >= 0x80u && os_utf8_seq_ok(s, sl, 0)) {
            int k = 0, m;
            uint32_t cp = os_utf8_next(s, sl, &k);
            m = os_display_put(cp, &page, o + n, 2);
            if (m > 0) { n += m; o[n] = 0; }
            s += k - 1;
            sl -= k;
            continue;
        }
        sl--;
        if (*s == '<') n = w_cat(o, cap, n, "&lt;");
        else if (*s == '>') n = w_cat(o, cap, n, "&gt;");
        else if (*s == '&') n = w_cat(o, cap, n, "&amp;");
        else if (*s == '"') n = w_cat(o, cap, n, "&quot;");
        else {
            if ((unsigned char)*s >= 0x80u && page) { o[n++] = (char)os_page_sel[0]; page = 0; }
            o[n++] = *s; o[n] = 0;
        }
    }
    if (page && n < cap - 1) { o[n++] = (char)os_page_sel[0]; o[n] = 0; }
    return n;
}

int wsearch_results_html(const wsearch_page_t *pg, const char *q, const char *engine_url, char *o, int cap) {
    return wsearch_results_html_note(pg, q, engine_url, 0, o, cap);
}

int wsearch_results_html_note(const wsearch_page_t *pg, const char *q, const char *engine_url, const char *note,
                              char *o, int cap) {
    int i, n = 0;
    if (!o || cap < 256) return -1;
    o[0] = 0;
    n = w_cat(o, cap, n, "<html><head><title>Recherche : ");
    n = cat_esc(o, cap, n, q);
    n = w_cat(o, cap, n, "</title><style>body{margin:8px;color:#202020}h1{color:#003366;margin:2px 0}"
                         ".u{color:#2e7d32}.s{color:#404040}li{margin-bottom:6px}.m{color:#707070}</style></head><body>");
    n = w_cat(o, cap, n, "<h1>Recherche</h1><p class=m>");
    n = cat_esc(o, cap, n, q);
    n = w_cat(o, cap, n, " - ");
    n = cat_int(o, cap, n, pg ? pg->n : 0);
    n = w_cat(o, cap, n, " resultats via ");
    n = cat_esc(o, cap, n, engine_url);
    n = w_cat(o, cap, n, "</p>");
    if (note && note[0]) {   /* Lot 72: why another engine answered */
        n = w_cat(o, cap, n, "<p class=m>");
        n = cat_esc(o, cap, n, note);
        n = w_cat(o, cap, n, "</p>");
    }
    n = w_cat(o, cap, n, "<ol>");
    for (i = 0; pg && i < pg->n && n < cap - 900; i++) {
        n = w_cat(o, cap, n, "<li><a href=\"");
        n = cat_esc(o, cap, n, pg->r[i].url);
        n = w_cat(o, cap, n, "\">");
        n = cat_esc(o, cap, n, pg->r[i].title);
        n = w_cat(o, cap, n, "</a><div class=u>");
        n = cat_esc(o, cap, n, pg->r[i].url);
        n = w_cat(o, cap, n, "</div><div class=s>");
        n = cat_esc(o, cap, n, pg->r[i].snippet);
        n = w_cat(o, cap, n, "</div></li>");
    }
    n = w_cat(o, cap, n, "</ol>");
    if (pg && pg->blocked && !pg->n) n = w_cat(o, cap, n, "<p>Le moteur a renvoye une verification anti-robot : aucun resultat.</p>");
    else if (!pg || !pg->n) n = w_cat(o, cap, n, "<p>Aucun resultat lisible (le balisage du moteur a peut-etre change).</p>");
    n = w_cat(o, cap, n, "</body></html>");
    return n;
}

int wsearch_is_query(const char *t) {
    int n = w_len(t), i, dot = 0, sp = 0;
    if (!t) return 0;
    while (*t == ' ') { t++; n--; }
    if (n <= 0) return 0;
    if (t[0] == '/' || t[0] == '.') return 0;
    if (w_pre_ci(t, n, "http://") || w_pre_ci(t, n, "https://") || w_pre_ci(t, n, "file:")) return 0;
    for (i = 0; i < n; i++) { if (t[i] == ' ') sp = 1; if (t[i] == '.') dot = 1; }
    /* "example.com" without spaces is a host, not a query */
    if (!sp && dot) return 0;
    return 1;
}
