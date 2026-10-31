/* wf_fetch_host.c - Lot 67: host driver for the shared wf_fetch stack over
 * POSIX sockets. Used by tests/scripts/test_lot67_web_host.sh against the
 * local test server (and, manually, against a real site).
 *   wf_fetch_host ROOTS.pem URL [now|noclock] [resolve-ip] */
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
    if (argc > 3 && strcmp(argv[3], "noclock") == 0) e.now = 0;
    else if (argc > 3 && strlen(argv[3]) == 14) e.now = argv[3];
    else {
        time_t t = time(0);
        strftime(now, sizeof(now), "%Y%m%d%H%M%S", gmtime(&t));
        e.now = now;
    }
    if (argc > 4) g_force_ip = argv[4];
    rc = wf_fetch(&e, argv[2], g_body, (int)sizeof(g_body) - 1, &r);
    if (rc != 0) { printf("FETCH FAIL: %s\n", r.err); return 1; }
    printf("FETCH OK status=%d bytes=%d truncated=%d redirects=%d url=%s\n", r.status, r.body_len, r.truncated, r.redirects, r.url);
    if (r.encoding) printf("ENCODING %s wire=%d\n", r.encoding == 1 ? "gzip" : "deflate", r.wire_len);
    if (r.https) printf("TLS suite=%s leaf=%s anchor=%s not_after=%s time_checked=%d\n", r.suite, r.leaf_cn, r.anchor_cn, r.not_after, r.time_checked);
    g_body[r.body_len] = 0;
    printf("BODY-BEGIN\n%.*s\nBODY-END\n", r.body_len > 2000 ? 2000 : r.body_len, (char*)g_body);
    return 0;
}
