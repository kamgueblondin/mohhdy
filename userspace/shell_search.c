/* shell_search.c - Lot 69: Internet search commands (see shell_search.h).
 * Engine: DuckDuckGo html (no JavaScript), falling back to DuckDuckGo
 * lite when the first page gives no result (bot check or changed
 * markup). `search-engine URL` sets another engine (a template with %s
 * or ending with '='), for example a local test server. Results stay in
 * a static table (WSEARCH_MAX entries); nothing is stored on disk. */
#include "shell_search.h"
#include "wsearch.h"
#include "wsearch_api.h"
#include "wf_guest.h"
#include "osui_gui.h"

void print_string(const char* s);

static char g_engine[WSEARCH_ENGINE_URL] = "ddg";
/* Lot 71: engines and keys from /persist/search.cfg (never in the repo) */
#define SEARCH_CFG "/persist/search.cfg"
static wsearch_cfg_t g_cfg;
static int g_cfg_loaded;
static char g_extra[WSEARCH_KEY_MAX + 96];
static char g_tried[160];
static wsearch_page_t g_page;
static char g_query[WSEARCH_QUERY + 1];
static char g_used[WSEARCH_URL + 64];
static uint8_t g_sbody[192 * 1024];
static wf_fetch_result_t g_sres;
static char g_fmt[8192];

static int s_pre(const char* s, const char* p) { while (*p && *s == *p) { s++; p++; } return *p == 0; }
static int s_eq(const char* a, const char* b) { while (*a && *a == *b) { a++; b++; } return *a == *b; }
static int s_len(const char* s) { int n = 0; while (s[n]) n++; return n; }
static void s_cpy(char* d, int cap, const char* s) { int i = 0; while (s[i] && i < cap - 1) { d[i] = s[i]; i++; } d[i] = 0; }
static void s_cat(char* d, int cap, const char* s) { int n = s_len(d); while (*s && n < cap - 1) d[n++] = *s++; d[n] = 0; }
static void s_int(char* d, int cap, int v) {
    char t[12]; int k = 0;
    if (v < 0) { s_cat(d, cap, "-"); v = -v; }
    do { t[k++] = (char)('0' + v % 10); v /= 10; } while (v && k < 11);
    while (k) { char c[2]; c[0] = t[--k]; c[1] = 0; s_cat(d, cap, c); }
}

int shell_search_is(const char* line) {
    return (s_pre(line, "search") && (line[6] == ' ' || !line[6])) ||
           (s_pre(line, "search-config") && (line[13] == ' ' || !line[13])) ||
           (s_pre(line, "search-open") && (line[11] == ' ' || !line[11])) ||
           (s_pre(line, "search-engine") && (line[13] == ' ' || !line[13]));
}

static const char* layout_name(int l) {
    return l == WSEARCH_LAYOUT_DDG_HTML ? "ddg-html" : l == WSEARCH_LAYOUT_DDG_LITE ? "ddg-lite"
         : l == WSEARCH_LAYOUT_GENERIC ? "generic-links" : l == WSEARCH_LAYOUT_SEARX ? "searx-html"
         : l == WSEARCH_LAYOUT_JSON ? "json-api" : l == WSEARCH_LAYOUT_GOOGLE ? "google-html"
         : l == WSEARCH_LAYOUT_WIKI ? "wikipedia-api" : "none";
}

static void cfg_load(void) {
    static char buf[WSEARCH_CFG_MAX];
    int n;
    if (g_cfg_loaded) return;
    g_cfg_loaded = 1;
    wsearch_cfg_default(&g_cfg);
    n = osui_gui_persist_read(SEARCH_CFG, buf, (int)sizeof(buf) - 1);
    if (n > 0) { buf[n] = 0; wsearch_cfg_parse(&g_cfg, buf, n); }
    s_cpy(g_engine, (int)sizeof(g_engine), g_cfg.engine);
}

static int cfg_save(void) {
    static char buf[WSEARCH_CFG_MAX];
    int n;
    s_cpy(g_cfg.engine, (int)sizeof(g_cfg.engine), g_engine);
    n = wsearch_cfg_format(&g_cfg, buf, (int)sizeof(buf));
    return osui_gui_persist_write(SEARCH_CFG, buf, n);
}

static const char* kind_name(int k) {
    return k == WSEARCH_KIND_SEARX ? "searx" : k == WSEARCH_KIND_BRAVE ? "brave-api" : k == WSEARCH_KIND_GOOGLE_API ? "google-api"
         : k == WSEARCH_KIND_GOOGLE ? "google" : k == WSEARCH_KIND_URL ? "url" : k == WSEARCH_KIND_WIKI ? "wikipedia" : "ddg";
}

/* one engine try; 0 when the page was received (any result count) */
static int try_kind(int kind, const char* engine, const char* q, char* err, int err_max) {
    wf_fetch_req_t rq;
    int rc;
    g_extra[0] = 0;
    if (kind == WSEARCH_KIND_DDG || kind == WSEARCH_KIND_URL) rc = wsearch_build_url(engine, q, g_used, (int)sizeof(g_used));
    else rc = wsearch_api_request(&g_cfg, kind, q, g_used, (int)sizeof(g_used), g_extra, (int)sizeof(g_extra));
    if (g_tried[0]) s_cat(g_tried, (int)sizeof(g_tried), ",");
    s_cat(g_tried, (int)sizeof(g_tried), kind == WSEARCH_KIND_DDG ? engine : kind_name(kind));
    if (rc != 0) {
        s_cpy(err, err_max, kind == WSEARCH_KIND_SEARX ? "searx not configured (search-config searx URL)"
                          : kind == WSEARCH_KIND_BRAVE ? "brave-api key missing (search-config key brave KEY)"
                          : kind == WSEARCH_KIND_GOOGLE_API ? "google-api key missing (search-config key google KEY CX)"
                          : "bad engine or query");
        return -1;
    }
    rq.method = 0; rq.body = 0; rq.body_len = 0; rq.ctype = 0; rq.extra = g_extra[0] ? g_extra : 0;
    if (wf_guest_fetch_ex(g_used, rq.extra ? &rq : 0, g_sbody, (int)sizeof(g_sbody) - 1, &g_sres, 0) != 0) {
        s_cpy(err, err_max, g_sres.err[0] ? g_sres.err : "fetch failed");
        return -1;
    }
    g_sbody[g_sres.body_len] = 0;
    if (g_sres.status == 401 || g_sres.status == 403 || g_sres.status == 429) {
        /* refused (bad key, quota, rate limit): try the next engine */
        s_cpy(err, err_max, "HTTP refused status ");
        s_int(err, err_max, g_sres.status);
        g_page.n = 0;
        return 0;
    }
    (void)wsearch_api_parse(kind, (const char*)g_sbody, g_sres.body_len, &g_page);
    return 0;
}

/* runs the query; fills g_page; status line in st. Lot 71: the chosen
 * engine first, then the fallbacks that need no key: DuckDuckGo html,
 * DuckDuckGo lite, the Google basic results page (today it asks for
 * JavaScript: reported "blocked js-required"), the Wikipedia search API. */
static int run_search(const char* q, char* st, int st_max) {
    static char err[160];
    int rc, fell = 0, kind, i;
    static const int chain[4] = { WSEARCH_KIND_DDG, WSEARCH_KIND_DDG, WSEARCH_KIND_GOOGLE, WSEARCH_KIND_WIKI };
    static const char* const chain_eng[4] = { "ddg", "ddg-lite", "google", "wikipedia" };
    cfg_load();
    g_page.n = 0;
    g_tried[0] = 0;
    err[0] = 0;
    s_cpy(g_query, (int)sizeof(g_query), q);
    kind = wsearch_kind(g_engine);
    if (kind < 0) kind = WSEARCH_KIND_DDG;
    rc = try_kind(kind, g_engine, q, err, (int)sizeof(err));
    /* a user template (a test server) has no fallback; names do */
    for (i = 0; i < 4 && (rc != 0 || g_page.n == 0) && kind != WSEARCH_KIND_URL; i++) {
        if ((kind == WSEARCH_KIND_DDG && s_eq(chain_eng[i], g_engine)) || (kind == WSEARCH_KIND_DDG && i == 0 && s_eq(g_engine, "ddg-html")))
            continue;
        if ((kind == WSEARCH_KIND_GOOGLE && i == 2) || (kind == WSEARCH_KIND_WIKI && i == 3)) continue;
        rc = try_kind(chain[i], chain_eng[i], q, err, (int)sizeof(err));
        fell = 1;
    }
    st[0] = 0;
    if (rc != 0) {
        s_cat(st, st_max, "search error "); s_cat(st, st_max, err);
        s_cat(st, st_max, " tried "); s_cat(st, st_max, g_tried);
        s_cat(st, st_max, " url "); s_cat(st, st_max, g_used);
        return -1;
    }
    s_cat(st, st_max, "search ok results "); s_int(st, st_max, g_page.n);
    s_cat(st, st_max, " layout "); s_cat(st, st_max, layout_name(g_page.layout));
    s_cat(st, st_max, " status "); s_int(st, st_max, g_sres.status);
    s_cat(st, st_max, " bytes "); s_int(st, st_max, g_sres.body_len);
    if (fell) { s_cat(st, st_max, " tried "); s_cat(st, st_max, g_tried); }
    if (g_page.blocked) s_cat(st, st_max, g_page.blocked == 2 ? " blocked js-required" : " blocked bot-check");
    s_cat(st, st_max, " url "); s_cat(st, st_max, g_used);
    return 0;
}

/* Lot 71: search-config (show, searx URL, key brave KEY, key google KEY CX,
 * json on|off, clear). Keys are stored in /persist/search.cfg and shown
 * masked only. */
static int config_line(const char* a) {
    static char m[64], t[WSEARCH_ENGINE_URL];
    char w1[24], w2[WSEARCH_ENGINE_URL], w3[WSEARCH_KEY_MAX];
    int k, rc = 0;
    cfg_load();
    w1[0] = w2[0] = w3[0] = 0;
    while (*a == ' ') a++;
    for (k = 0; *a && *a != ' ' && k < (int)sizeof(w1) - 1; k++) w1[k] = *a++;
    w1[k] = 0;
    while (*a == ' ') a++;
    for (k = 0; *a && *a != ' ' && k < (int)sizeof(w2) - 1; k++) w2[k] = *a++;
    w2[k] = 0;
    while (*a == ' ') a++;
    for (k = 0; *a && *a != ' ' && k < (int)sizeof(w3) - 1; k++) w3[k] = *a++;
    w3[k] = 0;
    while (*a == ' ') a++;
    if (w1[0] && s_eq(w1, "searx")) rc = wsearch_cfg_set(&g_cfg, "searx", w2);
    else if (s_eq(w1, "wiki")) rc = wsearch_cfg_set(&g_cfg, "wiki_lang", w2);
    else if (s_eq(w1, "json")) rc = wsearch_cfg_set(&g_cfg, "searx_json", s_eq(w2, "on") ? "1" : "0");
    else if (s_eq(w1, "key") && s_eq(w2, "brave")) rc = wsearch_cfg_set(&g_cfg, "brave_key", w3);
    else if (s_eq(w1, "key") && s_eq(w2, "google")) {
        char cx[WSEARCH_KEY_MAX];
        for (k = 0; *a && *a != ' ' && k < (int)sizeof(cx) - 1; k++) cx[k] = *a++;
        cx[k] = 0;
        rc = wsearch_cfg_set(&g_cfg, "google_key", w3);
        if (rc == 0) rc = wsearch_cfg_set(&g_cfg, "google_cx", cx);
    } else if (s_eq(w1, "clear")) {
        wsearch_cfg_default(&g_cfg);
        s_cpy(g_engine, (int)sizeof(g_engine), "ddg");
    } else if (w1[0]) {
        print_string("search-config error usage: search-config [searx URL | json on|off | key brave KEY | key google KEY CX | wiki LANG | clear]\n");
        return 1;
    }
    if (rc != 0) { print_string("search-config error bad value\n"); return 1; }
    if (w1[0] && cfg_save() != 0) print_string("search-config warning not saved (no /persist)\n");
    print_string("search-config engine ");
    print_string(g_engine);
    print_string(" searx ");
    print_string(g_cfg.searx[0] ? g_cfg.searx : "(none)");
    print_string(g_cfg.searx_json ? " json on" : " json off");
    wsearch_key_mask(g_cfg.brave_key, m, (int)sizeof(m));
    print_string(" brave_key "); print_string(m);
    wsearch_key_mask(g_cfg.google_key, m, (int)sizeof(m));
    print_string(" google_key "); print_string(m);
    s_cpy(t, (int)sizeof(t), g_cfg.google_cx[0] ? "set" : "(none)");
    print_string(" google_cx "); print_string(t);
    print_string(" wiki "); print_string(g_cfg.wiki);
    print_string(" file " SEARCH_CFG "\n");
    return 0;
}

int shell_search_url(int n, char* url, int max) {
    if (n < 1 || n > g_page.n) return -1;
    s_cpy(url, max, g_page.r[n - 1].url);
    return 0;
}

int shell_search_line(const char* line) {
    static char st[700];
    const char* a;
    if (s_pre(line, "search-config")) return config_line(line + 13);
    if (s_pre(line, "search-engine")) {
        cfg_load();
        a = line + 13;
        while (*a == ' ') a++;
        if (*a) {
            char test[WSEARCH_URL + 64];
            int kd = wsearch_kind(a);
            if (s_len(a) >= (int)sizeof(g_engine) || kd < 0 ||
                ((kd == WSEARCH_KIND_DDG || kd == WSEARCH_KIND_URL) && wsearch_build_url(a, "x", test, (int)sizeof(test)) != 0)) {
                print_string("search-engine error usage: search-engine [ddg|ddg-html|ddg-lite|searx|brave-api|google-api|google|wikipedia|URL with %s or ending with =]\n");
                return 1;
            }
            s_cpy(g_engine, (int)sizeof(g_engine), a);
            if (kd != WSEARCH_KIND_URL) (void)cfg_save();   /* a test template is not kept */
        }
        print_string("search-engine ok ");
        print_string(g_engine);
        if (wsearch_engine_template(g_engine)) { print_string(" "); print_string(wsearch_engine_template(g_engine)); }
        print_string(wsearch_kind(g_engine) == WSEARCH_KIND_URL ? "\n" : " (fallback ddg, ddg-lite, google, wikipedia)\n");
        return 0;
    }
    if (s_pre(line, "search-open")) {
        int n = 0;
        static char status[512];
        char url[WSEARCH_URL];
        a = line + 11;
        while (*a == ' ') a++;
        while (*a >= '0' && *a <= '9' && n < 100) n = n * 10 + (*a++ - '0');
        if (shell_search_url(n, url, (int)sizeof(url)) != 0) {
            print_string("search-open error no result ");
            { char t[8]; t[0] = 0; s_int(t, 8, n); print_string(t); }
            print_string(" (run search WORDS first)\n");
            return 1;
        }
        print_string("search-open url "); print_string(url); print_string("\n");
        /* same loader as the GUI /browser: the page is ready in the web pane */
        if (osui_gui_web_load(url, status, (int)sizeof(status)) != 0) { print_string(status); print_string("\n"); return 2; }
        print_string(status); print_string("\n");
        return 0;
    }
    a = line + 6;
    while (*a == ' ') a++;
    if (!*a || s_len(a) > WSEARCH_QUERY) {
        print_string("search error usage: search WORDS (200 bytes max); search-open N; search-engine [NAME|URL]; search-config\n");
        return 1;
    }
    if (run_search(a, st, (int)sizeof(st)) != 0) { print_string(st); print_string("\n"); return 2; }
    print_string(st); print_string("\n");
    (void)wsearch_format(&g_page, g_fmt, (int)sizeof(g_fmt));
    print_string(g_fmt);
    if (!g_page.n) print_string(g_page.blocked ? "search note: the engine sent a bot check page\n"
                                               : "search note: no readable result (markup may have changed)\n");
    print_string("search end\n");
    return 0;
}

int shell_search_page(const char* q, char* src, int src_max, char* final_url, int url_max, char* status, int st_max) {
    static char st[700];
    int n, rc;
    rc = run_search(q, st, (int)sizeof(st));
    if (status && st_max > 0) { status[0] = 0; s_cat(status, st_max, "osui web "); s_cat(status, st_max, st); }
    if (rc != 0) return -1;
    n = wsearch_results_html(&g_page, q, g_used, src, src_max);
    if (n < 0) return -1;
    if (final_url && url_max > 0) { final_url[0] = 0; s_cat(final_url, url_max, "search:"); s_cat(final_url, url_max, q); }
    return n;
}
