/* shell_fetch.c - Lot 67: the `fetch` / `http-get` shell commands and the
 * /browser network loader (see shell_fetch.h). Real network: DNS over TCP,
 * TCP through the networker, TLS 1.2 with certificate checks against the
 * root store in /etc/ca-roots.pem. Offline pages in /web stay local files. */
#include "shell_fetch.h"
#include "wf_guest.h"
#include "wf_html.h"
#include "shell_search.h"

void print_string(const char* s);

static uint8_t g_body[256 * 1024];
static wf_fetch_result_t g_res;
static char g_text[8192];

static int s_pre(const char* s, const char* p) { while (*p && *s == *p) { s++; p++; } return *p == 0; }
static int s_eq(const char* a, const char* b) { while (*a && *a == *b) { a++; b++; } return *a == *b; }
static void scat(char* d, int max, const char* s) {
    int n = 0;
    while (d[n]) n++;
    while (*s && n < max - 1) d[n++] = *s++;
    d[n] = 0;
}
static void scat_int(char* d, int max, int v) {
    char t[12];
    int k = 0;
    if (v < 0) { scat(d, max, "-"); v = -v; }
    do { t[k++] = (char)('0' + v % 10); v /= 10; } while (v && k < 11);
    while (k) { char c[2]; c[0] = t[--k]; c[1] = 0; scat(d, max, c); }
}

static int g_verbose;
static void log_line(const char* l) {
    if (!g_verbose) return;
    print_string("fetch: ");
    print_string(l);
    print_string("\n");
}

static int is_html(const wf_fetch_result_t* r, const uint8_t* b, int n) {
    const char* c = r->content_type;
    int i;
    for (i = 0; c[i]; i++)
        if ((c[i] == 'h' || c[i] == 'H') && s_pre(c + i + 1, "tml")) return 1;
    for (i = 0; i + 5 < n && i < 512; i++)
        if (b[i] == '<' && (b[i + 1] == 'h' || b[i + 1] == 'H' || b[i + 1] == '!')) return 1;
    return 0;
}

/* One summary line for the console and the GUI status. */
static void summary(char* o, int max, const wf_fetch_result_t* r) {
    int tc = 0, roots = wf_guest_roots(&tc);
    o[0] = 0;
    scat(o, max, "status "); scat_int(o, max, r->status);
    scat(o, max, " bytes "); scat_int(o, max, r->body_len);
    if (r->truncated) scat(o, max, " truncated 1");
    if (r->encoding) { scat(o, max, r->encoding == 1 ? " gzip " : " deflate "); scat_int(o, max, r->wire_len); }
    scat(o, max, " redirects "); scat_int(o, max, r->redirects);
    scat(o, max, " url "); scat(o, max, r->url);
    if (r->https) {
        scat(o, max, " tls "); scat(o, max, r->suite);
        scat(o, max, " leaf "); scat(o, max, r->leaf_cn);
        scat(o, max, " root \""); scat(o, max, r->anchor_cn); scat(o, max, "\"");
        scat(o, max, r->time_checked ? " dates checked" : " dates NOT checked (guest clock not trusted)");
        scat(o, max, " roots "); scat_int(o, max, roots);
        if (tc) scat(o, max, " test-ca 1");
    } else {
        scat(o, max, " tls none (plain http)");
    }
    scat(o, max, " entropy "); scat(o, max, wf_guest_entropy());
}

static const char* word(const char* s, char* out, int cap) {
    int n = 0;
    while (*s == ' ') s++;
    while (*s && *s != ' ' && n < cap - 1) out[n++] = *s++;
    out[n] = 0;
    return s;
}

int shell_fetch_is(const char* line) {
    return (s_pre(line, "fetch") && (line[5] == ' ' || !line[5])) ||
           (s_pre(line, "http-get") && (line[8] == ' ' || !line[8]));
}

int shell_fetch_line(const char* line) {
    static char tok[1100], url[1100], line_out[600];
    const char* p;
    int raw = 0, headers_only = 0, max_show = 1500, rc, i;
    p = word(line, tok, (int)sizeof(tok));   /* command name */
    url[0] = 0;
    g_verbose = 0;
    for (;;) {
        p = word(p, tok, (int)sizeof(tok));
        if (!tok[0]) break;
        if (s_eq(tok, "-v")) g_verbose = 1;
        else if (s_eq(tok, "-raw")) raw = 1;
        else if (s_eq(tok, "-I")) headers_only = 1;
        else if (s_eq(tok, "-n")) {
            p = word(p, tok, (int)sizeof(tok));
            max_show = 0;
            for (i = 0; tok[i] >= '0' && tok[i] <= '9'; i++) max_show = max_show * 10 + (tok[i] - '0');
            if (max_show > (int)sizeof(g_text) - 1) max_show = (int)sizeof(g_text) - 1;
        } else {
            for (i = 0; tok[i] && i < (int)sizeof(url) - 1; i++) url[i] = tok[i];
            url[i] = 0;
        }
    }
    if (!url[0]) {
        print_string("fetch error usage: fetch [-v] [-I] [-raw] [-n BYTES] URL   (http:// or https://, redirects <= 5)\n");
        return 1;
    }
    rc = wf_guest_fetch(url, g_body, (int)sizeof(g_body) - 1, &g_res, log_line);
    if (rc != 0) {
        print_string("fetch error ");
        print_string(g_res.err[0] ? g_res.err : "failed");
        print_string(" url ");
        print_string(g_res.url[0] ? g_res.url : url);
        print_string("\n");
        return 2;
    }
    summary(line_out, (int)sizeof(line_out), &g_res);
    print_string("fetch ok ");
    print_string(line_out);
    print_string("\n");
    if (g_res.content_type[0]) { print_string("fetch content-type "); print_string(g_res.content_type); print_string("\n"); }
    if (headers_only) return 0;
    g_body[g_res.body_len] = 0;
    if (!raw && is_html(&g_res, g_body, g_res.body_len)) {
        wf_html_stats_t st;
        static char reduced[16384];
        (void)wf_html_reduce((const char*)g_body, g_res.body_len, 0, reduced, (int)sizeof(reduced), &st);
        if (st.title[0]) { print_string("fetch title "); print_string(st.title); print_string("\n"); }
        (void)wf_html_text(reduced, st.out_bytes, g_text, max_show + 1 < (int)sizeof(g_text) ? max_show + 1 : (int)sizeof(g_text));
    } else {
        int n = g_res.body_len < max_show ? g_res.body_len : max_show;
        for (i = 0; i < n; i++) {
            char c = (char)g_body[i];
            g_text[i] = (c == '\n' || c == '\t' || ((unsigned char)c >= 0x20U && (unsigned char)c < 0x7fU)) ? c : '.';
        }
        g_text[n] = 0;
    }
    print_string("fetch body-begin\n");
    print_string(g_text);
    print_string("\nfetch body-end\n");
    return 0;
}

int shell_fetch_page(const char* url, char* src, int src_max, char* final_url, int url_max, char* status, int st_max) {
    static char line_out[600];
    static wf_url_t base;
    wf_html_stats_t st;
    int i, n;
    g_verbose = 0;
    if (status && st_max > 0) status[0] = 0;
    /* Lot 69: "search:QUERY" is the result page of a search */
    if (s_pre(url, "search:")) return shell_search_page(url + 7, src, src_max, final_url, url_max, status, st_max);
    if (wf_guest_fetch(url, g_body, (int)sizeof(g_body) - 1, &g_res, 0) != 0) {
        if (status) {
            scat(status, st_max, "osui web fetch error=");
            scat(status, st_max, g_res.err[0] ? g_res.err : "failed");
            scat(status, st_max, " url=");
            scat(status, st_max, url);
        }
        return -1;
    }
    for (i = 0; i < url_max - 1 && g_res.url[i]; i++) final_url[i] = g_res.url[i];
    final_url[i] = 0;
    g_body[g_res.body_len] = 0;
    if (wf_url_parse(g_res.url, &base) != 0) return -1;
    if (is_html(&g_res, g_body, g_res.body_len)) {
        n = wf_html_reduce((const char*)g_body, g_res.body_len, &base, src, src_max, &st);
    } else {
        /* text/plain and others: shown escaped in a <pre> */
        int k = 0, lim = g_res.body_len;
        const char* head = "<html><head><title>texte</title></head><body><pre>";
        const char* tail = "</pre></body></html>";
        for (i = 0; head[i] && k < src_max - 32; i++) src[k++] = head[i];
        for (i = 0; i < lim && k < src_max - 32; i++) {
            char c = (char)g_body[i];
            if (c == '<') c = '[';
            else if (c == '>') c = ']';
            else if ((unsigned char)c < 0x20U && c != '\n') c = ' ';
            src[k++] = c;
        }
        for (i = 0; tail[i] && k < src_max - 1; i++) src[k++] = tail[i];
        src[k] = 0;
        n = k;
        st.out_bytes = n;
    }
    if (status) {
        summary(line_out, (int)sizeof(line_out), &g_res);
        scat(status, st_max, "osui web fetch ok ");
        scat(status, st_max, line_out);
        scat(status, st_max, " reduced ");
        scat_int(status, st_max, n);
    }
    return n;
}
