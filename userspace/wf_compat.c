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

static int attr_val(const char *s, int n, int i, int end, const char *name, char *out, int cap);
static int is_root_id(const char *s, int n, int i, int end);

void wf_compat_scan(const char *html, int n, wf_compat_t *c) { wf_compat_scan_ex(html, n, 0, c); }

void wf_compat_scan_ex(const char *html, int n, int http_status, wf_compat_t *c) {
    int i = 0, in_script = 0, in_style = 0, title = 0, in_noscript = 0, ns_text = 0;
    if (!c) return;
    czero(c, (int)sizeof(*c));
    if (!html || n < 0) return;
    c->bytes = n;
    c->http_status = http_status;
    while (i < n) {
        if (html[i] == '<') {
            int close = (i + 1 < n && html[i + 1] == '/');
            int j = i + 1 + close;
            if (cstart(html, n, j, "script")) {
                if (close) in_script = 0; else { in_script = 1; c->scripts++; }
            } else if (cstart(html, n, j, "style")) {
                if (close) in_style = 0; else in_style = 1;
            } else if (!in_script && !in_style && cstart(html, n, j, "noscript")) {
                in_noscript = close ? 0 : 1;
            } else if (!close && !in_script && !in_style && cstart(html, n, j, "meta")) {
                int end = i;
                char v[160];
                while (end < n && html[end] != '>') end++;
                if (attr_val(html, n, i, end, "http-equiv", v, 16) > 0 && cstart(v, 7, 0, "refresh") &&
                    attr_val(html, n, i, end, "content", v, (int)sizeof(v)) > 0) {
                    int k = 0, o = 0;
                    if (!in_noscript) c->meta_refresh = 1;
                    while (v[k] && !(clow(v[k]) == 'u' && clow(v[k + 1]) == 'r' && clow(v[k + 2]) == 'l' &&
                                     v[k + 3] == '='))
                        k++;
                    if (v[k]) k += 4;
                    if (in_noscript && v[k] && !c->fallback[0]) {
                        while (v[k] && o < (int)sizeof(c->fallback) - 1) c->fallback[o++] = v[k++];
                        c->fallback[o] = 0;
                    }
                }
            } else if (!close && !in_script && !in_style) {
                int end = i;
                while (end < n && html[end] != '>') end++;
                if (cstart(html, n, j, "a ") || cstart(html, n, j, "a>") || cstart(html, n, j, "a\n")) {
                    c->links++;
                    if (in_noscript && !c->fallback[0])
                        (void)attr_val(html, n, i, end, "href", c->fallback, (int)sizeof(c->fallback));
                } else if (cstart(html, n, j, "div") && is_root_id(html, n, i, end)) {
                    /* the next tag closes it with no text between */
                    int k = end + 1;
                    while (k < n && (html[k] == ' ' || html[k] == '\n' || html[k] == '\r' || html[k] == '\t')) k++;
                    if (cstart(html, n, k, "</div")) c->empty_root = 1;
                }
                else if (cstart(html, n, j, "form")) c->forms++;
                else if (cstart(html, n, j, "table")) c->tables++;
                else if (cstart(html, n, j, "img")) c->images++;
                else if (cstart(html, n, j, "article") || cstart(html, n, j, "main")) c->has_article = 1;
                else if (cstart(html, n, j, "title")) title = 1;
                else if (cstart(html, n, j, "input") || cstart(html, n, j, "textarea") ||
                         cstart(html, n, j, "select")) {
                    char t[16], nm[16];
                    t[0] = 0;
                    (void)attr_val(html, n, i, end, "type", t, (int)sizeof(t));
                    if (!cstart(t, 6, 0, "hidden")) c->inputs++;
                    /* a search box is an input whose type or name says so */
                    if (cstart(t, 6, 0, "search")) c->search_form = 1;
                    if (attr_val(html, n, i, end, "name", nm, (int)sizeof(nm)) > 0 &&
                        ((nm[0] == 'q' && nm[1] == 0) || cstart(nm, 6, 0, "search") || cstart(nm, 5, 0, "query")))
                        c->search_form = 1;
                }
            } else if (close && cstart(html, n, j, "title")) title = 0;
            while (i < n && html[i] != '>') i++;
            if (i < n) i++;
            continue;
        }
        if (!in_script && !in_style) {
            unsigned char ch = (unsigned char)html[i];
            if (ch >= 32 || ch == '\n' || ch == '\t') {
                c->text_bytes++;
                if (in_noscript && ch > 32) ns_text++;
            }
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
    /* Lot 87: name the wall. The order is the most specific first. */
    if (find_ci(html, n, "cf-chl") >= 0 || find_ci(html, n, "challenge-platform") >= 0 ||
        find_ci(html, n, "just a moment...") >= 0 || find_ci(html, n, "checking your browser") >= 0 ||
        find_ci(html, n, "verify you are human") >= 0 ||
        /* a captcha widget alone is a challenge only on an almost empty page:
         * a login form may carry one too */
        (c->text_bytes < 600 && (find_ci(html, n, "g-recaptcha") >= 0 || find_ci(html, n, "h-captcha") >= 0)))
        c->wall_kind = WF_WALL_CHALLENGE;
    else if ((http_status == 403 || http_status == 429 || http_status == 451) &&
             (find_ci(html, n, "blocked") >= 0 || find_ci(html, n, "access denied") >= 0 ||
              find_ci(html, n, "forbidden") >= 0 || find_ci(html, n, "too many requests") >= 0))
        c->wall_kind = WF_WALL_BLOCKED;
    else if (find_ci(html, n, "enablejs") >= 0 || find_ci(html, n, "enable javascript") >= 0 ||
             find_ci(html, n, "javascript is disabled") >= 0 || find_ci(html, n, "javascript is required") >= 0)
        c->wall_kind = WF_WALL_ENABLEJS;
    else if (find_ci(html, n, "update your browser") >= 0 || find_ci(html, n, "browser is not supported") >= 0)
        c->wall_kind = WF_WALL_BROWSER;
    else if (c->empty_root && c->scripts > 0 && c->text_bytes - ns_text < 200)
        c->wall_kind = WF_WALL_SPA;
    else if (c->js_wall || (c->fallback[0] && c->text_bytes - ns_text < 40))
        c->wall_kind = WF_WALL_NOSCRIPT;
    if (c->wall_kind == WF_WALL_CHALLENGE || c->wall_kind == WF_WALL_SPA || c->wall_kind == WF_WALL_BROWSER ||
        c->wall_kind == WF_WALL_ENABLEJS || c->wall_kind == WF_WALL_NOSCRIPT)
        c->js_wall = 1;
}

/* value of name="..." (or '...' or bare) inside the tag [i, end) */
static int attr_val(const char *s, int n, int i, int end, const char *name, char *out, int cap) {
    int k, ln = 0, o = 0;
    while (name[ln]) ln++;
    if (cap < 1) return -1;
    out[0] = 0;
    if (end > n) end = n;
    for (k = i + 1; k + ln < end; k++) {
        char q;
        int v;
        if (!(s[k - 1] == ' ' || s[k - 1] == '\t' || s[k - 1] == '\n' || s[k - 1] == '"' || s[k - 1] == '\''))
            continue;
        if (!cstart(s, end, k, name)) continue;
        v = k + ln;
        while (v < end && s[v] == ' ') v++;
        if (v >= end || s[v] != '=') continue;
        v++;
        while (v < end && s[v] == ' ') v++;
        q = (v < end && (s[v] == '"' || s[v] == '\'')) ? s[v++] : 0;
        while (v < end && o < cap - 1) {
            if (q ? s[v] == q : (s[v] == ' ' || s[v] == '>' || s[v] == '/')) break;
            if (s[v] == '&' && cstart(s, end, v, "&amp;")) { out[o++] = '&'; v += 5; continue; }
            out[o++] = s[v++];
        }
        out[o] = 0;
        return o;
    }
    return -1;
}

static int is_root_id(const char *s, int n, int i, int end) {
    static const char *const ids[] = { "root", "app", "__next", "__nuxt", "svelte", 0 };
    char v[16];
    int k;
    if (attr_val(s, n, i, end, "id", v, (int)sizeof(v)) <= 0) return 0;
    for (k = 0; ids[k]; k++) {
        int a = 0;
        while (ids[k][a] && clow(v[a]) == ids[k][a]) a++;
        if (!ids[k][a] && !v[a]) return 1;
    }
    return 0;
}

const char *wf_compat_wall_name(int kind) {
    switch (kind) {
    case WF_WALL_ENABLEJS: return "enablejs";
    case WF_WALL_NOSCRIPT: return "noscript";
    case WF_WALL_CHALLENGE: return "challenge";
    case WF_WALL_BLOCKED: return "blocked";
    case WF_WALL_SPA: return "spa-shell";
    case WF_WALL_BROWSER: return "old-browser";
    default: return "none";
    }
}

const char *wf_compat_verdict(const wf_compat_t *c) {
    if (!c || c->bytes <= 0) return "empty";
    if (c->wall_kind == WF_WALL_BLOCKED) return "blocked";
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
    if (c->wall_kind) { p = cat(out, cap, p, " wall "); p = cat(out, cap, p, wf_compat_wall_name(c->wall_kind)); }
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

int wf_compat_advice(const wf_compat_t *c, char *out, int cap) {
    int p = 0;
    const char *msg;
    if (!c || !out || cap < 16) return -1;
    switch (c->wall_kind) {
    case WF_WALL_ENABLEJS:
        msg = "needs JavaScript; Mohhdy does not run site scripts. Use web-search (lite engines) or the fallback";
        break;
    case WF_WALL_NOSCRIPT:
        msg = "only a noscript notice without scripts. Open the fallback link if it is a plain page";
        break;
    case WF_WALL_CHALLENGE:
        msg = "bot challenge page; it cannot be solved without scripts. Not retried";
        break;
    case WF_WALL_BLOCKED:
        msg = "the site refused this client (block page). Not retried";
        break;
    case WF_WALL_SPA:
        msg = "empty app shell filled by scripts; the reader shows only the static text";
        break;
    case WF_WALL_BROWSER:
        msg = "the site asks for a newer browser; the reader shows only the static text";
        break;
    default:
        msg = 0;
        break;
    }
    p = cat(out, cap, p, "web-compat ");
    p = cat(out, cap, p, wf_compat_verdict(c));
    if (msg) {
        p = cat(out, cap, p, ": ");
        p = cat(out, cap, p, msg);
        if (c->fallback[0] && (c->wall_kind == WF_WALL_ENABLEJS || c->wall_kind == WF_WALL_NOSCRIPT)) {
            p = cat(out, cap, p, " fallback ");
            p = cat(out, cap, p, c->fallback);
        }
    } else {
        p = cat(out, cap, p, ": readable without scripts");
        if (c->search_form) p = cat(out, cap, p, ", search form");
    }
    if (p < 0) return -1;
    out[p] = 0;
    return p;
}

typedef struct {
    wf_compat_expect_t e;
    const char *file;
    int status;
    int wall;
} expect87_t;

static const expect87_t k_expect87[] = {
    /* live: 93419 bytes, 5 scripts, still the enablejs page */
    { { "www.google.com", "js-wall", 200, 1 }, "google_20261109.html", 200, WF_WALL_ENABLEJS },
    /* live: 22535 bytes, no script, a POST search form and a result table */
    { { "lite.duckduckgo.com", "table", 300, 0 }, "ddglite_20261109.html", 200, WF_WALL_NONE },
    /* live: 6079 bytes, no script, a list of stories */
    { { "text.npr.org", "reader", 200, 0 }, "npr_20261109.html", 200, WF_WALL_NONE },
    /* live: 403, 23406 bytes, "You've been blocked by network security" */
    { { "old.reddit.com", "blocked", 100, 0 }, "reddit_20261109.html", 403, WF_WALL_BLOCKED },
    /* a bot challenge page, reduced by hand from the usual markers */
    { { "challenge.example", "js-wall", 100, 1 }, "challenge_20261109.html", 403, WF_WALL_CHALLENGE },
    /* an app shell: empty root and a bundle */
    { { "app.example", "js-wall", 100, 1 }, "spa_20261109.html", 200, WF_WALL_SPA },
};

int wf_compat_expect87_count(void) { return (int)(sizeof(k_expect87) / sizeof(k_expect87[0])); }
const wf_compat_expect_t *wf_compat_expect87_at(int i) {
    if (i < 0 || i >= wf_compat_expect87_count()) return 0;
    return &k_expect87[i].e;
}
const char *wf_compat_expect87_file(int i) {
    if (i < 0 || i >= wf_compat_expect87_count()) return 0;
    return k_expect87[i].file;
}
int wf_compat_expect87_status(int i) {
    if (i < 0 || i >= wf_compat_expect87_count()) return 0;
    return k_expect87[i].status;
}
int wf_compat_expect87_wall(int i) {
    if (i < 0 || i >= wf_compat_expect87_count()) return -1;
    return k_expect87[i].wall;
}
