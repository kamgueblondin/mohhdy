/* wf_fetch_host.c - Lot 67: host driver for the shared wf_fetch stack over
 * POSIX sockets. Used by tests/scripts/test_lot67_web_host.sh against the
 * local test server (and, manually, against a real site).
 *   wf_fetch_host ROOTS.pem URL [now|noclock] [resolve-ip] [MODE]
 * Lot 71 MODE: "page" (reader view: sections, forms, UTF-8 text),
 * "submit:N=VALUE" (fill field N, submit its form with GET or POST and
 * print the answer), "search:KIND:QUERY" (URL is the engine base: the
 * search request and the parsed results, wsearch_api.c). */
#define _GNU_SOURCE
#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>
#include "wf_fetch.h"
#include "wf_html.h"
#include "wf_entity.h"
#include "wf_form.h"
#include "wf_section.h"
#include "wsearch_api.h"

static const char* g_force_ip;
static int h_resolve(void* n, const char* host, uint8_t ip[4]) {
    struct addrinfo hints, *res = 0;
    (void)n;
    if (g_force_ip) return inet_pton(AF_INET, g_force_ip, ip) == 1 ? 0 : -1;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    if (getaddrinfo(host, 0, &hints, &res) != 0 || !res) return -1;
    memcpy(ip, &((struct sockaddr_in*)res->ai_addr)->sin_addr, 4);
    freeaddrinfo(res);
    return 0;
}
static int h_connect(void* n, const uint8_t ip[4], int port) {
    struct sockaddr_in a;
    struct timeval tv = {10, 0};
    int s = socket(AF_INET, SOCK_STREAM, 0);
    (void)n;
    if (s < 0) return -1;
    setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
    memset(&a, 0, sizeof(a));
    a.sin_family = AF_INET;
    a.sin_port = htons((uint16_t)port);
    memcpy(&a.sin_addr, ip, 4);
    if (connect(s, (struct sockaddr*)&a, sizeof(a)) != 0) { close(s); return -1; }
    return s;
}
static int h_send(void* n, int h, const uint8_t* d, int len) { (void)n; return (int)send(h, d, (size_t)len, MSG_NOSIGNAL); }
static int h_recv(void* n, int h, uint8_t* d, int cap) { (void)n; return (int)recv(h, d, (size_t)cap, 0); }
static void h_close(void* n, int h) { (void)n; close(h); }
static int h_rand(void* n, uint8_t* o, int len) {
    FILE* f = fopen("/dev/urandom", "rb");
    (void)n;
    if (!f || fread(o, 1, (size_t)len, f) != (size_t)len) { if (f) fclose(f); return -1; }
    fclose(f);
    return 0;
}
static void h_log(void* n, const char* l) { (void)n; printf("[fetch] %s\n", l); }

static wf_trust_t g_trust;
static uint8_t g_der[256 * 1024];
static char g_pem[320 * 1024];
static uint8_t g_body[256 * 1024];
static char g_red[192 * 1024];
static char g_sec[64 * 1024];
static char g_txt[16 * 1024];
static wf_form_page_t g_forms;
static wf_sections_t g_secs;
static wsearch_page_t g_page;

static int reduce_page(const wf_fetch_result_t* r) {
    wf_url_t base;
    wf_html_stats_t st;
    int n, k, t;
    if (wf_url_parse(r->url, &base) != 0) return -1;
    wf_html_set_charset(wf_charset_header(r->content_type, (int)strlen(r->content_type)));
    n = wf_html_reduce_ex((const char*)g_body, r->body_len, &base, g_red, (int)sizeof(g_red), &st, 6000, &g_forms);
    if (n <= 0 || wf_section_index(g_red, n, &g_secs) <= 0) { printf("PAGE FAIL reduce=%d\n", n); return -1; }
    printf("PAGE title=%s nodes=%d bytes=%d sections=%d forms=%d fields=%d charset=%d\n", st.title, g_secs.nodes, n,
           g_secs.n, g_forms.nforms, g_forms.nfields, st.charset);
    for (k = 0; k < g_secs.n && k < 3; k++) {
        t = wf_section_render(g_red, n, &g_secs, k, &g_forms, g_sec, (int)sizeof(g_sec));
        { int z, m = wf_html_text_display(g_sec, t, g_txt, 400); for (z = 0; z < m; z++) if (g_txt[z] == '\n') g_txt[z] = ' '; }
        printf("SECTION %d bytes=%d text=%s\n", k + 1, t, g_txt);
    }
    if (g_secs.n > 3) {
        t = wf_section_render(g_red, n, &g_secs, g_secs.n - 1, &g_forms, g_sec, (int)sizeof(g_sec));
        t = wf_html_text_display(g_sec, t, g_txt, (int)sizeof(g_txt));
        { int z; for (z = 0; z < t; z++) if (g_txt[z] == '\n') g_txt[z] = ' '; }
        printf("SECTION %d bytes=%d tail=%s\n", g_secs.n, t, t > 120 ? g_txt + t - 120 : g_txt);
    }
    for (k = 0; k < g_forms.nforms; k++)
        printf("FORM %d method=%s action=%s\n", k, g_forms.form[k].method == WF_METHOD_POST ? "POST" : "GET", g_forms.form[k].action);
    for (k = 0; k < g_forms.nfields; k++)
        printf("FIELD %d type=%d name=%s value=%s\n", k, g_forms.field[k].type, g_forms.field[k].name, g_forms.field[k].value);
    return n;
}

static wf_cookie_jar_t g_jar;

int main(int argc, char** argv) {
    wf_fetch_env_t e;
    wf_fetch_result_t r;
    char now[16];
    int used = 0, len, rc;
    FILE* f;
    if (argc < 3) { fprintf(stderr, "usage: %s roots.pem url [now|noclock|YYYYMMDDHHMMSS] [ip]\n", argv[0]); return 2; }
    f = fopen(argv[1], "rb");
    if (!f) { perror(argv[1]); return 2; }
    len = (int)fread(g_pem, 1, sizeof(g_pem) - 1, f);
    fclose(f);
    wf_trust_add_pem(&g_trust, g_pem, len, g_der, (int)sizeof(g_der), &used);
    printf("[fetch] roots=%d\n", g_trust.count);
    memset(&e, 0, sizeof(e));
    e.resolve = h_resolve; e.connect = h_connect; e.send = h_send; e.recv = h_recv;
    e.close = h_close; e.rand = h_rand; e.log = h_log; e.trust = &g_trust; e.max_redirects = 5;
    wf_cookie_jar_init(&g_jar);
    e.jar = &g_jar;   /* Lot 71 */
    if (argc > 3 && strcmp(argv[3], "noclock") == 0) e.now = 0;
    else if (argc > 3 && strlen(argv[3]) == 14) e.now = argv[3];
    else {
        time_t t = time(0);
        strftime(now, sizeof(now), "%Y%m%d%H%M%S", gmtime(&t));
        e.now = now;
    }
    if (argc > 4 && strcmp(argv[4], "-") != 0) g_force_ip = argv[4];
    if (argc > 5 && strncmp(argv[5], "search:", 7) == 0) {
        /* search:KIND:QUERY against the engine base URL argv[2] */
        static wsearch_cfg_t cfg;
        static char url[1024], extra[256], kind[32];
        wf_fetch_req_t rq;
        const char* q = argv[5] + 7;
        int k = 0, kd, i;
        while (*q && *q != ':' && k < 31) kind[k++] = *q++;
        kind[k] = 0;
        if (*q == ':') q++;
        wsearch_cfg_default(&cfg);
        if (wsearch_cfg_set(&cfg, "searx", argv[2]) != 0) return 2;
        if (strcmp(kind, "searx-json") == 0) { (void)wsearch_cfg_set(&cfg, "searx_json", "1"); strcpy(kind, "searx"); }
        kd = wsearch_kind(kind);
        if (wsearch_api_request(&cfg, kd, q, url, (int)sizeof(url), extra, (int)sizeof(extra)) != 0) { printf("SEARCH FAIL request\n"); return 1; }
        printf("SEARCH url=%s\n", url);
        memset(&rq, 0, sizeof(rq));
        rq.extra = extra[0] ? extra : 0;
        if (wf_fetch_ex(&e, url, &rq, g_body, (int)sizeof(g_body) - 1, &r) != 0) { printf("FETCH FAIL: %s\n", r.err); return 1; }
        g_body[r.body_len] = 0;
        wsearch_api_parse(kd, (const char*)g_body, r.body_len, &g_page);
        printf("SEARCH status=%d results=%d layout=%d\n", r.status, g_page.n, g_page.layout);
        for (i = 0; i < g_page.n; i++) printf("RESULT %d title=%s url=%s\n", i + 1, g_page.r[i].title, g_page.r[i].url);
        return 0;
    }
    rc = wf_fetch(&e, argv[2], g_body, (int)sizeof(g_body) - 1, &r);
    if (rc != 0) { printf("FETCH FAIL: %s\n", r.err); return 1; }
    printf("FETCH OK status=%d bytes=%d truncated=%d redirects=%d url=%s\n", r.status, r.body_len, r.truncated, r.redirects, r.url);
    if (r.cookies_set || r.cookies_sent || g_jar.refused) {
        static char list[4096];
        printf("COOKIES set=%d sent=%d refused=%u kept=%d\n", r.cookies_set, r.cookies_sent, g_jar.refused, wf_cookie_count(&g_jar));
        wf_cookie_list(&g_jar, list, (int)sizeof(list));
        printf("%s", list);
    }
    if (r.encoding) printf("ENCODING %s wire=%d\n", r.encoding == 1 ? "gzip" : "deflate", r.wire_len);
    if (r.https) printf("TLS suite=%s leaf=%s anchor=%s not_after=%s time_checked=%d\n", r.suite, r.leaf_cn, r.anchor_cn, r.not_after, r.time_checked);
    g_body[r.body_len] = 0;
    if (argc > 5 && strcmp(argv[5], "page") == 0) return reduce_page(&r) > 0 ? 0 : 1;
    if (argc > 5 && strncmp(argv[5], "submit:", 7) == 0) {
        static wf_form_submit_t sub;
        wf_fetch_req_t rq;
        int fi = 0;
        const char* d = argv[5] + 7;
        while (*d >= '0' && *d <= '9') fi = fi * 10 + (*d++ - '0');
        const char* v = strchr(argv[5], '=');
        if (reduce_page(&r) <= 0 || !v || wf_form_set(&g_forms, fi, v + 1) != 0) { printf("SUBMIT FAIL field\n"); return 1; }
        if (wf_form_submit(&g_forms, g_forms.field[fi].form, -1, &sub) != 0) { printf("SUBMIT FAIL build\n"); return 1; }
        printf("SUBMIT method=%s url=%s body=%s\n", sub.method == WF_METHOD_POST ? "POST" : "GET", sub.url, sub.body);
        memset(&rq, 0, sizeof(rq));
        if (sub.method == WF_METHOD_POST) { rq.method = "POST"; rq.body = sub.body; rq.body_len = sub.body_len; rq.ctype = sub.content_type; }
        if (wf_fetch_ex(&e, sub.url, sub.method == WF_METHOD_POST ? &rq : 0, g_body, (int)sizeof(g_body) - 1, &r) != 0) {
            printf("FETCH FAIL: %s\n", r.err);
            return 1;
        }
        g_body[r.body_len] = 0;
        printf("ANSWER status=%d redirects=%d url=%s\n", r.status, r.redirects, r.url);
        printf("BODY-BEGIN\n%.*s\nBODY-END\n", r.body_len > 2000 ? 2000 : r.body_len, (char*)g_body);
        return 0;
    }
    printf("BODY-BEGIN\n%.*s\nBODY-END\n", r.body_len > 2000 ? 2000 : r.body_len, (char*)g_body);
    return 0;
}
