/* wsearch_api.c - Lot 71: SearXNG, Brave and Google search answers (see
 * wsearch_api.h). Bounded: the JSON walker keeps a fixed depth and a few
 * fixed-size fields per object; no heap. */
#include "wsearch_api.h"
#include "wf_entity.h"

static int a_len(const char *s) { int n = 0; while (s && s[n]) n++; return n; }
static int a_low(int c) { return c >= 'A' && c <= 'Z' ? c + 32 : c; }
static void a_cpy(char *d, int cap, const char *s) {
    int i = 0;
    if (cap <= 0) return;
    while (s && s[i] && i < cap - 1) { d[i] = s[i]; i++; }
    d[i] = 0;
}
static int a_eq(const char *a, const char *b) { while (*a && *a == *b) { a++; b++; } return *a == *b; }
static int a_pre_ci(const char *s, int n, const char *p) {
    int i;
    for (i = 0; p[i]; i++) if (i >= n || a_low((unsigned char)s[i]) != p[i]) return 0;
    return 1;
}
static int a_find_ci(const char *s, int n, int from, const char *p) {
    int i;
    for (i = from; i < n; i++) if (a_pre_ci(s + i, n - i, p)) return i;
    return -1;
}

/* ---------------------------------------------------------------- config */

void wsearch_cfg_default(wsearch_cfg_t *c) {
    int i;
    for (i = 0; i < (int)sizeof(*c); i++) ((char *)c)[i] = 0;
    a_cpy(c->engine, (int)sizeof(c->engine), "ddg");
    a_cpy(c->wiki, (int)sizeof(c->wiki), "fr");
}
/* language of the last Wikipedia request (result URLs) */
static char g_wlang[8] = "fr";

static int key_ok(const char *v) {
    int i;
    for (i = 0; v[i]; i++) {
        unsigned char c = (unsigned char)v[i];
        if (c <= ' ' || c >= 0x7Fu || c == '"' || c == '\\') return 0;
    }
    return i < WSEARCH_KEY_MAX;
}

int wsearch_cfg_set(wsearch_cfg_t *c, const char *k, const char *v) {
    if (!k || !v) return -1;
    if (a_eq(k, "engine")) {
        if (wsearch_kind(v) < 0 || a_len(v) >= (int)sizeof(c->engine)) return -1;
        a_cpy(c->engine, (int)sizeof(c->engine), v);
        return 0;
    }
    if (a_eq(k, "searx")) {
        int n = a_len(v);
        if (n && !(a_pre_ci(v, n, "https://") || a_pre_ci(v, n, "http://"))) return -1;
        if (n >= (int)sizeof(c->searx) - 40 || !key_ok(v)) return -1;
        a_cpy(c->searx, (int)sizeof(c->searx), v);
        while (n > 0 && c->searx[n - 1] == '/') c->searx[--n] = 0;   /* "https://h/" -> "https://h" */
        return 0;
    }
    if (a_eq(k, "wiki_lang")) {
        int n = a_len(v), i;
        if (n < 2 || n >= (int)sizeof(c->wiki)) return -1;
        for (i = 0; i < n; i++) if (!((v[i] >= 'a' && v[i] <= 'z') || v[i] == '-')) return -1;
        a_cpy(c->wiki, (int)sizeof(c->wiki), v);
        return 0;
    }
    if (a_eq(k, "searx_json")) { c->searx_json = v[0] == '1' || v[0] == 'y' || v[0] == 'o'; return 0; }
    if (a_eq(k, "brave_key")) { if (!key_ok(v)) return -1; a_cpy(c->brave_key, WSEARCH_KEY_MAX, v); return 0; }
    if (a_eq(k, "google_key")) { if (!key_ok(v)) return -1; a_cpy(c->google_key, WSEARCH_KEY_MAX, v); return 0; }
    if (a_eq(k, "google_cx")) { if (!key_ok(v)) return -1; a_cpy(c->google_cx, WSEARCH_KEY_MAX, v); return 0; }
    return -1;
}

int wsearch_cfg_parse(wsearch_cfg_t *c, const char *t, int n) {
    int i = 0, got = 0;
    while (i < n) {
        char key[24], val[WSEARCH_ENGINE_URL];
        int s = i, e, eq = -1, k;
        while (i < n && t[i] != '\n') { if (t[i] == '=' && eq < 0) eq = i; i++; }
        e = i;
        i++;
        while (e > s && (t[e - 1] == '\r' || t[e - 1] == ' ')) e--;
        while (s < e && t[s] == ' ') s++;
        if (s >= e || t[s] == '#' || eq < 0 || eq - s >= (int)sizeof(key) || e - eq - 1 >= (int)sizeof(val)) continue;
        for (k = 0; k < eq - s; k++) key[k] = t[s + k];
        while (k > 0 && key[k - 1] == ' ') k--;
        key[k] = 0;
        s = eq + 1;
        while (s < e && t[s] == ' ') s++;
        for (k = 0; s + k < e; k++) val[k] = t[s + k];
        val[k] = 0;
        if (wsearch_cfg_set(c, key, val) == 0) got++;
    }
    return got;
}

static int put_line(char *o, int cap, int n, const char *k, const char *v) {
    int i;
    for (i = 0; k[i] && n < cap - 1; i++) o[n++] = k[i];
    if (n < cap - 1) o[n++] = '=';
    for (i = 0; v[i] && n < cap - 1; i++) o[n++] = v[i];
    if (n < cap - 1) o[n++] = '\n';
    o[n] = 0;
    return n;
}

int wsearch_cfg_format(const wsearch_cfg_t *c, char *o, int cap) {
    int n = 0;
    if (cap <= 0) return 0;
    o[0] = 0;
    n = put_line(o, cap, n, "engine", c->engine);
    if (c->searx[0]) n = put_line(o, cap, n, "searx", c->searx);
    if (c->searx_json) n = put_line(o, cap, n, "searx_json", "1");
    if (c->brave_key[0]) n = put_line(o, cap, n, "brave_key", c->brave_key);
    if (c->google_key[0]) n = put_line(o, cap, n, "google_key", c->google_key);
    if (c->google_cx[0]) n = put_line(o, cap, n, "google_cx", c->google_cx);
    if (c->wiki[0] && !a_eq(c->wiki, "fr")) n = put_line(o, cap, n, "wiki_lang", c->wiki);
    return n;
}

void wsearch_key_mask(const char *key, char *out, int cap) {
    int n = a_len(key), o = 0, i;
    char t[12];
    if (cap < 24) { if (cap > 0) out[0] = 0; return; }
    if (!n) { a_cpy(out, cap, "(none)"); return; }
    if (n > 8) {
        for (i = 0; i < 4; i++) out[o++] = key[i];
        out[o++] = '.'; out[o++] = '.'; out[o++] = '.';
        for (i = n - 4; i < n; i++) out[o++] = key[i];
    } else {
        for (i = 0; i < n; i++) out[o++] = '*';
    }
    out[o++] = ' '; out[o++] = '(';
    i = 0;
    do { t[i++] = (char)('0' + n % 10); n /= 10; } while (n && i < 10);
    while (i && o < cap - 10) out[o++] = t[--i];
    a_cpy(out + o, cap - o, " chars)");
}

/* ---------------------------------------------------------------- engines */

int wsearch_kind(const char *e) {
    int n = a_len(e);
    if (!e || !n) return -1;
    if (a_eq(e, "ddg") || a_eq(e, "ddg-html") || a_eq(e, "ddg-lite")) return WSEARCH_KIND_DDG;
    if (a_eq(e, "searx") || a_eq(e, "searxng")) return WSEARCH_KIND_SEARX;
    if (a_eq(e, "brave-api") || a_eq(e, "brave")) return WSEARCH_KIND_BRAVE;
    if (a_eq(e, "google-api")) return WSEARCH_KIND_GOOGLE_API;
    if (a_eq(e, "google")) return WSEARCH_KIND_GOOGLE;
    if (a_eq(e, "wikipedia") || a_eq(e, "wiki")) return WSEARCH_KIND_WIKI;
    if (a_pre_ci(e, n, "https://") || a_pre_ci(e, n, "http://")) return WSEARCH_KIND_URL;
    return -1;
}

static int build(char *out, int cap, const char *a, const char *enc, const char *b, const char *c, const char *d) {
    const char *parts[5];
    int n = 0, i, k;
    parts[0] = a; parts[1] = enc; parts[2] = b; parts[3] = c; parts[4] = d;
    for (k = 0; k < 5; k++)
        for (i = 0; parts[k] && parts[k][i]; i++) {
            if (n >= cap - 1) return -1;
            out[n++] = parts[k][i];
        }
    out[n] = 0;
    return 0;
}

int wsearch_api_request(const wsearch_cfg_t *c, int kind, const char *q, char *url, int cap, char *extra, int ecap) {
    char enc[WSEARCH_QUERY * 3 + 4];
    if (extra && ecap > 0) extra[0] = 0;
    if (!q || !q[0] || a_len(q) > WSEARCH_QUERY || wsearch_urlencode(q, enc, (int)sizeof(enc)) < 0) return -1;
    switch (kind) {
    case WSEARCH_KIND_SEARX:
        if (!c->searx[0]) return -1;
        return build(url, cap, c->searx, "/search?q=", enc, c->searx_json ? "&format=json" : "&categories=general", "&language=fr");
    case WSEARCH_KIND_BRAVE:
        if (!c->brave_key[0] || !extra) return -1;
        if (build(url, cap, "https://api.search.brave.com/res/v1/web/search?q=", enc, "&count=10", 0, 0) != 0) return -1;
        return build(extra, ecap, "Accept: application/json\r\nX-Subscription-Token: ", c->brave_key, "\r\n", 0, 0);
    case WSEARCH_KIND_GOOGLE_API:
        if (!c->google_key[0] || !c->google_cx[0]) return -1;
        if (build(url, cap, "https://www.googleapis.com/customsearch/v1?key=", c->google_key, "&cx=", c->google_cx, "&q=") != 0) return -1;
        {
            int n = a_len(url), i;
            for (i = 0; enc[i]; i++) { if (n >= cap - 1) return -1; url[n++] = enc[i]; }
            url[n] = 0;
        }
        return 0;
    case WSEARCH_KIND_GOOGLE:
        return build(url, cap, "https://www.google.com/search?q=", enc, "&hl=fr&gbv=1&num=10", 0, 0);
    case WSEARCH_KIND_WIKI: {
        char host[48];
        a_cpy(g_wlang, (int)sizeof(g_wlang), c->wiki[0] ? c->wiki : "fr");
        build(host, (int)sizeof(host), "https://", g_wlang, ".wikipedia.org", 0, 0);
        return build(url, cap, host, "/w/api.php?action=query&list=search&format=json&utf8=1&srlimit=10&srsearch=", enc, 0, 0);
    }
    default:
        return wsearch_build_url(c->engine[0] ? c->engine : "ddg", q, url, cap);
    }
}

/* ---------------------------------------------------------------- text */

static int put_utf8(char *o, int cap, int n, uint32_t cp) {
    if (cp < 0x80u) { if (n < cap - 1) o[n++] = (char)cp; }
    else if (cp < 0x800u) { if (n < cap - 2) { o[n++] = (char)(0xC0u | (cp >> 6)); o[n++] = (char)(0x80u | (cp & 0x3Fu)); } }
    else if (cp < 0x10000u) {
        if (n < cap - 3) { o[n++] = (char)(0xE0u | (cp >> 12)); o[n++] = (char)(0x80u | ((cp >> 6) & 0x3Fu)); o[n++] = (char)(0x80u | (cp & 0x3Fu)); }
    } else if (n < cap - 4) {
        o[n++] = (char)(0xF0u | (cp >> 18)); o[n++] = (char)(0x80u | ((cp >> 12) & 0x3Fu));
        o[n++] = (char)(0x80u | ((cp >> 6) & 0x3Fu)); o[n++] = (char)(0x80u | (cp & 0x3Fu));
    }
    return n;
}

int wsearch_html_text(const char *s, int n, char *out, int cap) {
    int i = 0, o = 0, sp = 1;
    if (cap <= 0) return 0;
    while (i < n) {
        uint32_t cp;
        if (s[i] == '<') {
            while (i < n && s[i] != '>') i++;
            i++;
            if (!sp && o < cap - 1) { out[o++] = ' '; sp = 1; }
            continue;
        }
        cp = wf_text_next(s, n, &i, WF_CS_UTF8, 1);
        if (!cp) break;
        if (cp == ' ' || cp == '\n' || cp == '\t' || cp == '\r' || cp == 0xA0u) {
            if (!sp && o < cap - 1) { out[o++] = ' '; sp = 1; }
            continue;
        }
        if (cp < 0x20u) continue;
        o = put_utf8(out, cap, o, cp);
        sp = 0;
    }
    while (o > 0 && out[o - 1] == ' ') o--;
    out[o] = 0;
    return o;
}

static int hex4(const char *s, uint32_t *v) {
    int i;
    *v = 0;
    for (i = 0; i < 4; i++) {
        int c = a_low((unsigned char)s[i]), d;
        if (c >= '0' && c <= '9') d = c - '0';
        else if (c >= 'a' && c <= 'f') d = c - 'a' + 10;
        else return -1;
        *v = (*v << 4) | (uint32_t)d;
    }
    return 0;
}

int wsearch_json_string(const char *s, int n, char *out, int cap) {
    int i = 1, o = 0;
    if (n < 2 || s[0] != '"' || cap <= 0) return -1;
    while (i < n && s[i] != '"') {
        if (s[i] == '\\') {
            char e;
            if (i + 1 >= n) return -1;
            e = s[i + 1];
            i += 2;
            if (e == 'u') {
                uint32_t cp, lo;
                if (i + 4 > n || hex4(s + i, &cp) != 0) return -1;
                i += 4;
                if (cp >= 0xD800u && cp <= 0xDBFFu && i + 6 <= n && s[i] == '\\' && s[i + 1] == 'u' && hex4(s + i + 2, &lo) == 0 &&
                    lo >= 0xDC00u && lo <= 0xDFFFu) {
                    cp = 0x10000u + ((cp - 0xD800u) << 10) + (lo - 0xDC00u);
                    i += 6;
                } else if (cp >= 0xD800u && cp <= 0xDFFFu) cp = 0xFFFDu;
                o = put_utf8(out, cap, o, cp);
            } else {
                char c = e == 'n' ? '\n' : e == 't' ? '\t' : e == 'r' ? '\r' : e == 'b' ? '\b' : e == 'f' ? '\f' : e;
                if (o < cap - 1) out[o++] = c;
            }
            continue;
        }
        if (o < cap - 1) out[o++] = s[i];
        i++;
    }
    if (i >= n) return -1;
    out[o] = 0;
    return i + 1;
}

/* ---------------------------------------------------------------- results */

static int dup(const wsearch_page_t *pg, const char *u) {
    int i;
    for (i = 0; i < pg->n; i++) if (a_eq(pg->r[i].url, u)) return 1;
    return 0;
}

static int url_ok(const char *u) {
    int i, n = a_len(u);
    if (!(a_pre_ci(u, n, "https://") || a_pre_ci(u, n, "http://"))) return 0;
    for (i = 0; i < n; i++) if ((unsigned char)u[i] <= ' ' || u[i] == '"' || u[i] == '<' || u[i] == '>') return 0;
    return n < WSEARCH_URL;
}

static void add(wsearch_page_t *pg, const char *url, const char *title, const char *snip) {
    static char t[WSEARCH_TITLE * 2], sn[WSEARCH_SNIPPET * 2];
    if (pg->n >= WSEARCH_MAX) return;
    if (!url_ok(url) || dup(pg, url)) { pg->skipped++; return; }
    /* titles and snippets may hold markup (<b>, &amp;) */
    wsearch_html_text(title, a_len(title), t, (int)sizeof(t));
    wsearch_html_text(snip ? snip : "", a_len(snip), sn, (int)sizeof(sn));
    a_cpy(pg->r[pg->n].url, WSEARCH_URL, url);
    a_cpy(pg->r[pg->n].title, WSEARCH_TITLE, t[0] ? t : url);
    a_cpy(pg->r[pg->n].snippet, WSEARCH_SNIPPET, sn);
    {
        /* a UTF-8 sequence cut by the copy is dropped */
        char *x = pg->r[pg->n].title;
        int l = a_len(x), k = l;
        while (k > 0 && ((unsigned char)x[k - 1] & 0xC0u) == 0x80u) k--;
        if (k > 0 && (unsigned char)x[k - 1] >= 0xC0u) {
            unsigned char c = (unsigned char)x[k - 1];
            int need = c >= 0xF0u ? 4 : c >= 0xE0u ? 3 : 2;
            if (l - (k - 1) < need) x[k - 1] = 0;
        }
        x = pg->r[pg->n].snippet;
        l = a_len(x); k = l;
        while (k > 0 && ((unsigned char)x[k - 1] & 0xC0u) == 0x80u) k--;
        if (k > 0 && (unsigned char)x[k - 1] >= 0xC0u) {
            unsigned char c = (unsigned char)x[k - 1];
            int need = c >= 0xF0u ? 4 : c >= 0xE0u ? 3 : 2;
            if (l - (k - 1) < need) x[k - 1] = 0;
        }
    }
    pg->n++;
}

/* JSON walker: every object that has a url-like and a title-like string
 * member is a result (SearXNG results[], Brave web.results[], Google CSE
 * items[]). Depth and field sizes are bounded. */
#define JDEPTH 16
typedef struct { char url[WSEARCH_URL]; char title[WSEARCH_TITLE * 2]; char snip[WSEARCH_SNIPPET * 2]; } jobj_t;

int wsearch_parse_json(const char *j, int n, wsearch_page_t *pg) {
    static jobj_t st[JDEPTH];
    static char key[32], val[1024];
    int i = 0, d = -1, expect_key[JDEPTH], depth_obj[JDEPTH], keypend = 0;
    int k;
    for (k = 0; k < (int)sizeof(*pg); k++) ((char *)pg)[k] = 0;
    for (k = 0; k < JDEPTH; k++) { expect_key[k] = 0; depth_obj[k] = 0; }
    key[0] = 0;
    while (i < n) {
        char c = j[i];
        if (c == '{' || c == '[') {
            d++;
            if (d >= JDEPTH) return pg->n;     /* too deep: stop, keep what was found */
            depth_obj[d] = c == '{';
            expect_key[d] = c == '{';
            if (c == '{') { st[d].url[0] = 0; st[d].title[0] = 0; st[d].snip[0] = 0; }
            keypend = 0;
            i++;
            continue;
        }
        if (c == '}' || c == ']') {
            if (d >= 0 && depth_obj[d] && st[d].url[0] && st[d].title[0]) add(pg, st[d].url, st[d].title, st[d].snip);
            d--;
            if (d < -1) return pg->n;
            i++;
            continue;
        }
        if (c == '"') {
            int used;
            if (d >= 0 && depth_obj[d] && expect_key[d] && !keypend) {
                used = wsearch_json_string(j + i, n - i, key, (int)sizeof(key));
                if (used < 0) return pg->n;
                keypend = 1;
                i += used;
                continue;
            }
            used = wsearch_json_string(j + i, n - i, val, (int)sizeof(val));
            if (used < 0) return pg->n;
            if (d >= 0 && depth_obj[d] && keypend) {
                if ((a_eq(key, "url") || a_eq(key, "link")) && !st[d].url[0]) a_cpy(st[d].url, WSEARCH_URL, val);
                else if (a_eq(key, "title") && !st[d].title[0]) a_cpy(st[d].title, (int)sizeof(st[d].title), val);
                else if ((a_eq(key, "content") || a_eq(key, "snippet") || a_eq(key, "description")) && !st[d].snip[0])
                    a_cpy(st[d].snip, (int)sizeof(st[d].snip), val);
            }
            keypend = 0;
            i += used;
            continue;
        }
        if (c == ':') { i++; continue; }
        if (c == ',') {
            if (d >= 0 && depth_obj[d]) { expect_key[d] = 1; keypend = 0; }
            i++;
            continue;
        }
        if (d >= 0 && depth_obj[d] && keypend && c != ' ' && c != '\n' && c != '\r' && c != '\t') {
            /* number, true, false, null: skipped */
            while (i < n && j[i] != ',' && j[i] != '}' && j[i] != ']') i++;
            keypend = 0;
            continue;
        }
        i++;
    }
    if (pg->n) pg->layout = WSEARCH_LAYOUT_JSON;
    return pg->n;
}

static int attr_val(const char *h, int a, int b, const char *name, char *out, int cap) {
    int i, l = a_len(name);
    out[0] = 0;
    for (i = a; i + l < b; i++) {
        if ((i == a || h[i - 1] == ' ' || h[i - 1] == '\n') && a_pre_ci(h + i, b - i, name) && h[i + l] == '=') {
            int s = i + l + 1, e, q = 0, o = 0;
            if (h[s] == '"' || h[s] == '\'') q = h[s++];
            e = s;
            while (e < b && (q ? h[e] != q : (h[e] != ' ' && h[e] != '>'))) e++;
            /* &amp; in attribute values */
            while (s < e && o < cap - 1) {
                if (h[s] == '&' && a_pre_ci(h + s, e - s, "&amp;")) { out[o++] = '&'; s += 5; continue; }
                out[o++] = h[s++];
            }
            out[o] = 0;
            return o;
        }
    }
    return -1;
}

static int tag_end(const char *h, int n, int i) {
    int q = 0;
    while (i < n && (q || h[i] != '>')) { if (h[i] == '"' || h[i] == '\'') q = (q == h[i]) ? 0 : (q ? q : h[i]); i++; }
    return i;
}

int wsearch_parse_searx_html(const char *h, int n, wsearch_page_t *pg) {
    static char url[WSEARCH_URL], title[WSEARCH_TITLE * 2], snip[WSEARCH_SNIPPET * 2], cls[96];
    int i = 0, k;
    for (k = 0; k < (int)sizeof(*pg); k++) ((char *)pg)[k] = 0;
    while (pg->n < WSEARCH_MAX) {
        int a = a_find_ci(h, n, i, "<article"), e, end, h3, x;
        if (a < 0) break;
        e = tag_end(h, n, a);
        end = a_find_ci(h, n, e, "</article>");
        if (end < 0) end = n;
        i = end;
        if (attr_val(h, a, e, "class", cls, (int)sizeof(cls)) < 0 || a_find_ci(cls, a_len(cls), 0, "result") < 0) continue;
        url[0] = title[0] = snip[0] = 0;
        h3 = a_find_ci(h, end, e, "<h3");
        if (h3 >= 0) {
            int ae = a_find_ci(h, end, h3, "<a "), te, ce;
            if (ae >= 0) {
                te = tag_end(h, end, ae);
                attr_val(h, ae, te, "href", url, (int)sizeof(url));
                ce = a_find_ci(h, end, te, "</a>");
                if (ce > te) {
                    int m = ce - te - 1;
                    if (m > (int)sizeof(title) - 1) m = (int)sizeof(title) - 1;
                    for (x = 0; x < m; x++) title[x] = h[te + 1 + x];
                    title[m] = 0;
                }
            }
        }
        x = a_find_ci(h, end, e, "class=\"content");
        if (x >= 0) {
            int te = tag_end(h, end, x), ce = a_find_ci(h, end, te, "</p>"), m;
            if (ce < 0) ce = end;
            m = ce - te - 1;
            if (m > (int)sizeof(snip) - 1) m = (int)sizeof(snip) - 1;
            if (m < 0) m = 0;
            for (k = 0; k < m; k++) snip[k] = h[te + 1 + k];
            snip[m] = 0;
        }
        if (url[0]) add(pg, url, title, snip);
    }
    if (pg->n) pg->layout = WSEARCH_LAYOUT_SEARX;
    return pg->n;
}

static int google_internal(const char *u) {
    int n = a_len(u), s = a_pre_ci(u, n, "https://") ? 8 : 7, e = s;
    while (e < n && u[e] != '/' && u[e] != ':' && u[e] != '?') e++;
    {
        /* host ends with google.TLD / google.co.TLD, or is a Google service */
        int i;
        for (i = s; i < e; i++)
            if (a_pre_ci(u + i, e - i, "google.") && (i == s || u[i - 1] == '.')) return 1;
        for (i = s; i < e; i++)
            if (a_pre_ci(u + i, e - i, "gstatic.") || a_pre_ci(u + i, e - i, "googleusercontent.")) return 1;
    }
    return 0;
}

int wsearch_parse_google_html(const char *h, int n, wsearch_page_t *pg) {
    static char href[WSEARCH_URL * 2], url[WSEARCH_URL], title[WSEARCH_TITLE * 2], snip[WSEARCH_SNIPPET * 2];
    int i = 0, k;
    for (k = 0; k < (int)sizeof(*pg); k++) ((char *)pg)[k] = 0;
    if (a_find_ci(h, n, 0, "/sorry/") >= 0 || a_find_ci(h, n, 0, "unusual traffic") >= 0 || a_find_ci(h, n, 0, "g-recaptcha") >= 0)
        pg->blocked = 1;
    /* Lot 71 (measured 2026-11): Google answers clients without
     * JavaScript with a redirect to /httpservice/retry/enablejs or an
     * "update your browser" page: no result to parse, try another engine */
    if (a_find_ci(h, n, 0, "/httpservice/retry/enablejs") >= 0 || a_find_ci(h, n, 0, "browser isn't supported") >= 0)
        pg->blocked = 2;
    /* Lot 72 (measured 2026-11-01 with gbv=1 and text browser agents):
     * "Mettez a jour votre navigateur" / "no longer supported", with a
     * link to the help article on browsers without JavaScript */
    if (a_find_ci(h, n, 0, "websearch/answer/16515119") >= 0 || a_find_ci(h, n, 0, "n'est plus pris en charge") >= 0 ||
        a_find_ci(h, n, 0, "is no longer supported") >= 0)
        pg->blocked = 2;
    while (pg->n < WSEARCH_MAX) {
        int a = a_find_ci(h, n, i, "<a "), e, ce, q, m;
        if (a < 0) break;
        e = tag_end(h, n, a);
        i = e;
        if (attr_val(h, a, e, "href", href, (int)sizeof(href)) <= 0) continue;
        if (!a_pre_ci(href, a_len(href), "/url?") ) continue;
        q = a_find_ci(href, a_len(href), 0, "q=");
        if (q < 0) q = a_find_ci(href, a_len(href), 0, "url=");
        if (q < 0) continue;
        q += href[q] == 'q' ? 2 : 4;
        m = q;
        while (href[m] && href[m] != '&') m++;
        if (wsearch_urldecode(href + q, m - q, url, (int)sizeof(url)) <= 0) continue;
        if (!url_ok(url) || google_internal(url)) { pg->skipped++; continue; }
        ce = a_find_ci(h, n, e, "</a>");
        if (ce < 0) break;
        m = ce - e - 1;
        if (m > (int)sizeof(title) - 1) m = (int)sizeof(title) - 1;
        if (m < 0) m = 0;
        for (k = 0; k < m; k++) title[k] = h[e + 1 + k];
        title[m] = 0;
        {
            /* the snippet: text after the link, up to the next result link */
            int s = ce + 4, t = a_find_ci(h, n, s, "<a href=\"/url?"), l;
            if (t < 0) t = n;
            l = t - s;
            if (l > (int)sizeof(snip) - 1) l = (int)sizeof(snip) - 1;
            for (k = 0; k < l; k++) snip[k] = h[s + k];
            snip[l] = 0;
        }
        i = ce;
        {
            /* a title without text (an image link): keep looking */
            static char tt[WSEARCH_TITLE * 2];
            if (wsearch_html_text(title, a_len(title), tt, (int)sizeof(tt)) == 0) continue;
        }
        add(pg, url, title, snip);
    }
    if (pg->n) pg->layout = WSEARCH_LAYOUT_GOOGLE;
    return pg->n;
}

/* Lot 71: MediaWiki list=search answer */
int wsearch_parse_wiki(const char *j, int n, const char *lang, wsearch_page_t *pg) {
    static char title[WSEARCH_TITLE * 2], raw[WSEARCH_SNIPPET * 4], snip[WSEARCH_SNIPPET], url[WSEARCH_URL];
    static const char hx[] = "0123456789ABCDEF";
    int i, k;
    for (k = 0; k < (int)sizeof(*pg); k++) ((char *)pg)[k] = 0;
    i = a_find_ci(j, n, 0, "\"search\":[");
    if (i < 0) return 0;
    while (pg->n < WSEARCH_MAX) {
        int t = a_find_ci(j, n, i, "\"title\":"), sn, used, o;
        if (t < 0) break;
        t += 8;
        while (t < n && j[t] == ' ') t++;
        used = wsearch_json_string(j + t, n - t, title, (int)sizeof(title));
        if (used < 0) break;
        i = t + used;
        snip[0] = 0;
        sn = a_find_ci(j, n, i, "\"snippet\":");
        if (sn >= 0) {
            int nt = a_find_ci(j, n, i, "\"title\":");
            sn += 10;
            while (sn < n && j[sn] == ' ') sn++;
            if (nt < 0 || sn < nt) {
                int u2 = wsearch_json_string(j + sn, n - sn, raw, (int)sizeof(raw));
                if (u2 > 0) wsearch_html_text(raw, a_len(raw), snip, (int)sizeof(snip));
            }
        }
        /* https://LANG.wikipedia.org/wiki/Title_with_underscores (UTF-8 %XX) */
        o = 0;
        {
            const char *pre = "https://", *mid = ".wikipedia.org/wiki/";
            for (k = 0; pre[k]; k++) url[o++] = pre[k];
            for (k = 0; lang[k] && k < 7; k++) url[o++] = lang[k];
            for (k = 0; mid[k]; k++) url[o++] = mid[k];
        }
        for (k = 0; title[k] && o < WSEARCH_URL - 4; k++) {
            unsigned char c = (unsigned char)title[k];
            if (c == ' ') url[o++] = '_';
            else if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '-' || c == '_' || c == '.' ||
                     c == '(' || c == ')' || c == ',') url[o++] = (char)c;
            else { url[o++] = '%'; url[o++] = hx[c >> 4]; url[o++] = hx[c & 15]; }
        }
        url[o] = 0;
        add(pg, url, title, snip);
    }
    if (pg->n) pg->layout = WSEARCH_LAYOUT_WIKI;
    return pg->n;
}

int wsearch_api_parse(int kind, const char *body, int len, wsearch_page_t *pg) {
    int k = 0;
    while (k < len && (body[k] == ' ' || body[k] == '\n' || body[k] == '\r' || body[k] == '\t')) k++;
    if (kind == WSEARCH_KIND_WIKI) return wsearch_parse_wiki(body, len, g_wlang, pg);
    if (k < len && (body[k] == '{' || body[k] == '[')) return wsearch_parse_json(body, len, pg);
    if (kind == WSEARCH_KIND_SEARX) {
        if (wsearch_parse_searx_html(body, len, pg) > 0) return pg->n;
    }
    if (kind == WSEARCH_KIND_GOOGLE) {
        if (wsearch_parse_google_html(body, len, pg) > 0 || pg->blocked) return pg->n;
    }
    return wsearch_parse(body, len, pg);
}

/* Lot 72: a form of a Google page submitted from the browser. The
 * target is https://www.google.TLD/search (or /webhp) with q=...; the
 * query is decoded (+ and %XX, UTF-8 kept). 1 and the query, else 0. */
int wsearch_google_form(const char *url, char *q, int cap) {
    int i = 0, h0, h1, k;
    if (!url || !q || cap < 2) return 0;
    q[0] = 0;
    if (a_pre_ci(url, a_len(url), "https://")) i = 8;
    else if (a_pre_ci(url, a_len(url), "http://")) i = 7;
    else return 0;
    h0 = i;
    while (url[i] && url[i] != '/' && url[i] != '?' && url[i] != ':') i++;
    h1 = i;
    {
        /* host [www.]google.TLD with TLD "com", two letters, "co.XX" or
         * "com.XX" (google.fr, google.co.uk, google.com.au); nothing else */
        int s = h0, ok = 0, r, len;
        if (h1 - s > 4 && a_pre_ci(url + s, h1 - s, "www.")) s += 4;
        if (h1 - s > 7 && a_pre_ci(url + s, h1 - s, "google.")) {
            r = s + 7;
            len = h1 - r;
            for (k = r; k < h1; k++)
                if (!((url[k] >= 'a' && url[k] <= 'z') || (url[k] >= 'A' && url[k] <= 'Z') || url[k] == '.')) len = -1;
            if (len == 3 && a_pre_ci(url + r, 3, "com")) ok = 1;
            else if (len == 2 && url[r + 1] != '.' && url[r] != '.') ok = 1;
            else if (len == 5 && a_pre_ci(url + r, 3, "co.") && url[r + 3] != '.' && url[r + 4] != '.') ok = 1;
            else if (len == 6 && a_pre_ci(url + r, 4, "com.") && url[r + 4] != '.' && url[r + 5] != '.') ok = 1;
        }
        if (!ok) return 0;
    }
    while (url[i] == ':' || (url[i] >= '0' && url[i] <= '9')) i++;
    if (!(a_pre_ci(url + i, a_len(url + i), "/search?") || a_pre_ci(url + i, a_len(url + i), "/webhp?"))) return 0;
    while (url[i] && url[i] != '?') i++;
    while (url[i]) {
        int s, e;
        i++;
        s = i;
        while (url[i] && url[i] != '&' && url[i] != '#') i++;
        e = i;
        if (e - s >= 2 && url[s] == 'q' && url[s + 1] == '=') {
            int n = wsearch_urldecode(url + s + 2, e - s - 2, q, cap);
            if (n > 0) return 1;
            q[0] = 0;
            return 0;
        }
        if (url[i] == '#') break;
    }
    return 0;
}
