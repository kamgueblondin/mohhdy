/* wf_compat.c - Lot 86: see wf_compat.h.
 * Measured 2026-11-09 from this guest's network path (curl as a stand-in
 * for the same URLs the guest fetches): google gbv=1 was 92874 bytes and
 * still the enablejs page; the Wikipedia query API returned a JSON search;
 * example.com was 577 bytes of text; news.ycombinator.com was 33820 bytes
 * with a stories table. */
#include "wf_compat.h"

static char clow(char c) { return (c >= 'A' && c <= 'Z') ? (char)(c + 32) : c; }
static int cstart(const char *s, int n, int i, const char *w) {
    int k = 0;
    while (w[k]) {
        if (i + k >= n || clow(s[i + k]) != w[k]) return 0;
        k++;
    }
    return 1;
}
static void czero(void *p, int n) { unsigned char *b = (unsigned char *)p; while (n--) *b++ = 0; }

static int find_ci(const char *s, int n, const char *w) {
    int i, wn = 0;
    while (w[wn]) wn++;
    for (i = 0; i + wn <= n; i++) if (cstart(s, n, i, w)) return i;
    return -1;
}

void wf_compat_scan(const char *html, int n, wf_compat_t *c) {
    int i = 0, in_script = 0, in_style = 0, title = 0;
    if (!c) return;
    czero(c, (int)sizeof(*c));
    if (!html || n < 0) return;
    c->bytes = n;
    while (i < n) {
        if (html[i] == '<') {
            int close = (i + 1 < n && html[i + 1] == '/');
            int j = i + 1 + close;
            if (cstart(html, n, j, "script")) {
                if (close) in_script = 0; else { in_script = 1; c->scripts++; }
            } else if (cstart(html, n, j, "style")) {
                if (close) in_style = 0; else in_style = 1;
            } else if (!close && !in_script && !in_style) {
                if (cstart(html, n, j, "a ") || cstart(html, n, j, "a>") || cstart(html, n, j, "a\n")) c->links++;
                else if (cstart(html, n, j, "form")) c->forms++;
                else if (cstart(html, n, j, "table")) c->tables++;
                else if (cstart(html, n, j, "img")) c->images++;
                else if (cstart(html, n, j, "article") || cstart(html, n, j, "main")) c->has_article = 1;
                else if (cstart(html, n, j, "title")) title = 1;
                else if (cstart(html, n, j, "input")) {
                    /* a search box is an input whose type or name says so */
                }
            } else if (close && cstart(html, n, j, "title")) title = 0;
            while (i < n && html[i] != '>') i++;
            if (i < n) i++;
            continue;
        }
        if (!in_script && !in_style) {
            unsigned char ch = (unsigned char)html[i];
            if (ch >= 32 || ch == '\n' || ch == '\t') c->text_bytes++;
            if (title && c->title_len < 95 && ch >= 32) c->title[c->title_len++] = (char)ch;
        }
        i++;
    }
    c->title[c->title_len] = 0;
    if (find_ci(html, n, "enablejs") >= 0 || find_ci(html, n, "update your browser") >= 0 ||
        find_ci(html, n, "please enable javascript") >= 0)
        c->js_wall = 1;
    if (find_ci(html, n, "cookie") >= 0 && (find_ci(html, n, "consent") >= 0 || find_ci(html, n, "gdpr") >= 0))
        c->cookie_banner = 1;
    if (find_ci(html, n, "name=\"q\"") >= 0 || find_ci(html, n, "name=\"search\"") >= 0 ||
        find_ci(html, n, "type=\"search\"") >= 0)
        c->search_form = 1;
    /* A page whose only visible instruction is inside noscript, with a
     * script and almost no other text, is a script wall even without the
     * word enablejs. */
    if (!c->js_wall && c->scripts > 0 && c->text_bytes < 80 && c->forms == 0 && !c->has_article)
        c->js_wall = 1;
}

const char *wf_compat_verdict(const wf_compat_t *c) {
    if (!c || c->bytes <= 0) return "empty";
    if (c->js_wall) return "js-wall";
    if (c->tables > 0 && c->text_bytes > 120) return "table";
    if (c->has_article || c->text_bytes > 80) return "reader";
    return "short";
}

static int cat(char *o, int cap, int p, const char *s) {
    while (s && *s) { if (p >= cap - 1) return -1; o[p++] = *s++; }
    return p;
}
static int cat_u(char *o, int cap, int p, int v) {
    char b[12];
    int n = 0;
    if (v < 0) v = 0;
    do { b[n++] = (char)('0' + v % 10); v /= 10; } while (v && n < 11);
    while (n) { if (p >= cap - 1) return -1; o[p++] = b[--n]; }
    return p;
}

int wf_compat_line(const char *host, const wf_compat_t *c, char *out, int cap) {
    int p = 0;
    if (!c || !out || cap < 16) return -1;
    p = cat(out, cap, p, "compat ");
    p = cat(out, cap, p, host && host[0] ? host : "-");
    p = cat(out, cap, p, " ");
    p = cat(out, cap, p, wf_compat_verdict(c));
    p = cat(out, cap, p, " bytes ");
    p = cat_u(out, cap, p, c->bytes);
    p = cat(out, cap, p, " text ");
    p = cat_u(out, cap, p, c->text_bytes);
    p = cat(out, cap, p, " scripts ");
    p = cat_u(out, cap, p, c->scripts);
    p = cat(out, cap, p, " links ");
    p = cat_u(out, cap, p, c->links);
    p = cat(out, cap, p, " forms ");
    p = cat_u(out, cap, p, c->forms);
    p = cat(out, cap, p, " tables ");
    p = cat_u(out, cap, p, c->tables);
    if (c->title_len) { p = cat(out, cap, p, " title "); p = cat(out, cap, p, c->title); }
    if (p < 0) return -1;
    out[p] = 0;
    return p;
}

static const wf_compat_expect_t k_expect[] = {
    /* min_bytes is the fixture kept in the repo. The live sizes measured
     * on 2026-11-09 are in docs/lot86_web_compat.md (google 92874). */
    { "www.google.com", "js-wall", 200, 1 },
    { "en.wikipedia.org", "reader", 40, 0 },
    { "example.com", "reader", 100, 0 },
    { "news.ycombinator.com", "table", 200, 0 },
};

int wf_compat_expect_count(void) { return (int)(sizeof(k_expect) / sizeof(k_expect[0])); }
const wf_compat_expect_t *wf_compat_expect_at(int i) {
    if (i < 0 || i >= wf_compat_expect_count()) return 0;
    return &k_expect[i];
}
