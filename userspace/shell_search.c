/* shell_search.c - Lot 69: Internet search commands (see shell_search.h).
 * Engine: DuckDuckGo html (no JavaScript), falling back to DuckDuckGo
 * lite when the first page gives no result (bot check or changed
 * markup). `search-engine URL` sets another engine (a template with %s
 * or ending with '='), for example a local test server. Results stay in
 * a static table (WSEARCH_MAX entries); nothing is stored on disk. */
#include "shell_search.h"
#include "wsearch.h"
#include "wf_guest.h"
#include "osui_gui.h"

void print_string(const char* s);

static char g_engine[WSEARCH_ENGINE_URL] = "ddg";
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
           (s_pre(line, "search-open") && (line[11] == ' ' || !line[11])) ||
           (s_pre(line, "search-engine") && (line[13] == ' ' || !line[13]));
}

static const char* layout_name(int l) {
    return l == WSEARCH_LAYOUT_DDG_HTML ? "ddg-html" : l == WSEARCH_LAYOUT_DDG_LITE ? "ddg-lite"
         : l == WSEARCH_LAYOUT_GENERIC ? "generic-links" : "none";
}

/* one engine try; 0 when the page was received (any result count) */
static int try_engine(const char* engine, const char* q, char* err, int err_max) {
    if (wsearch_build_url(engine, q, g_used, (int)sizeof(g_used)) != 0) {
        s_cpy(err, err_max, "bad engine or query");
        return -1;
    }
    if (wf_guest_fetch(g_used, g_sbody, (int)sizeof(g_sbody) - 1, &g_sres, 0) != 0) {
        s_cpy(err, err_max, g_sres.err[0] ? g_sres.err : "fetch failed");
        return -1;
    }
    g_sbody[g_sres.body_len] = 0;
    (void)wsearch_parse((const char*)g_sbody, g_sres.body_len, &g_page);
    return 0;
}

/* runs the query; fills g_page; status line in st */
static int run_search(const char* q, char* st, int st_max) {
    char err[160];
    int rc, fell = 0;
    g_page.n = 0;
    s_cpy(g_query, (int)sizeof(g_query), q);
    rc = try_engine(g_engine, q, err, (int)sizeof(err));
    if ((rc != 0 || g_page.n == 0) && s_eq(g_engine, "ddg")) {
        rc = try_engine("ddg-lite", q, err, (int)sizeof(err));
        fell = 1;
    }
    st[0] = 0;
    if (rc != 0) {
        s_cat(st, st_max, "search error "); s_cat(st, st_max, err);
        s_cat(st, st_max, " url "); s_cat(st, st_max, g_used);
        return -1;
    }
    s_cat(st, st_max, "search ok results "); s_int(st, st_max, g_page.n);
    s_cat(st, st_max, " layout "); s_cat(st, st_max, layout_name(g_page.layout));
    s_cat(st, st_max, " status "); s_int(st, st_max, g_sres.status);
    s_cat(st, st_max, " bytes "); s_int(st, st_max, g_sres.body_len);
    if (fell) s_cat(st, st_max, " fallback ddg-lite");
    if (g_page.blocked) s_cat(st, st_max, " blocked bot-check");
    s_cat(st, st_max, " url "); s_cat(st, st_max, g_used);
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
    if (s_pre(line, "search-engine")) {
        a = line + 13;
        while (*a == ' ') a++;
        if (*a) {
            char test[WSEARCH_URL + 64];
            if (s_len(a) >= (int)sizeof(g_engine) || wsearch_build_url(a, "x", test, (int)sizeof(test)) != 0) {
                print_string("search-engine error usage: search-engine [ddg|ddg-html|ddg-lite|URL with %s or ending with =]\n");
                return 1;
            }
            s_cpy(g_engine, (int)sizeof(g_engine), a);
        }
        print_string("search-engine ok ");
        print_string(g_engine);
        if (wsearch_engine_template(g_engine)) { print_string(" "); print_string(wsearch_engine_template(g_engine)); }
        print_string(s_eq(g_engine, "ddg") ? " (fallback ddg-lite)\n" : "\n");
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
        print_string("search error usage: search WORDS (200 bytes max); search-open N; search-engine [NAME|URL]\n");
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
