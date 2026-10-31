/* wf_fetch.c - Lot 67: HTTP(S) GET with bounded redirects (see wf_fetch.h). */
#include "wf_fetch.h"
#include "wf_dns.h"
#include "wf_inflate.h"

static wf_tls_t g_tls;
static wf_http_resp_t g_resp;

typedef struct { const wf_fetch_env_t* env; int h; } bridge_t;

static int b_send(void* io, const uint8_t* d, int n) { bridge_t* b = (bridge_t*)io; return b->env->send(b->env->net, b->h, d, n); }
static int b_recv(void* io, uint8_t* d, int n) { bridge_t* b = (bridge_t*)io; return b->env->recv(b->env->net, b->h, d, n); }
static int b_rand(void* io, uint8_t* d, int n) { bridge_t* b = (bridge_t*)io; return b->env->rand(b->env->net, d, n); }

static void scopy(char* d, int cap, const char* s) {
    int i;
    for (i = 0; i < cap - 1 && s && s[i]; i++) d[i] = s[i];
    d[i] = 0;
}

static void sapp(char* d, int cap, const char* s) {
    int n = 0;
    while (d[n]) n++;
    scopy(d + n, cap - n, s);
}

static void logf2(const wf_fetch_env_t* e, const char* a, const char* b) {
    static char line[400];
    if (!e->log) return;
    line[0] = 0;
    sapp(line, (int)sizeof(line), a);
    if (b) sapp(line, (int)sizeof(line), b);
    e->log(e->net, line);
}

static void ip_str(const uint8_t ip[4], char* o) {
    int i, p = 0;
    for (i = 0; i < 4; i++) {
        int v = ip[i];
        if (v >= 100) o[p++] = (char)('0' + v / 100);
        if (v >= 10) o[p++] = (char)('0' + (v / 10) % 10);
        o[p++] = (char)('0' + v % 10);
        if (i < 3) o[p++] = '.';
    }
    o[p] = 0;
}

/* One request on a fresh connection. 0 = response parsed. */
static int one(const wf_fetch_env_t* env, const wf_url_t* u, uint8_t* body, int cap, wf_fetch_result_t* res) {
    static char req[WF_URL_PATH_MAX + WF_URL_HOST_MAX + 256];
    static uint8_t buf[4096];
    static char ipb[20];
    bridge_t br;
    int h, n, rl, rc = 0, got;
    uint8_t ip[4];
    if (wf_ipv4_parse(u->host, ip) != 0) {
        if (env->resolve(env->net, u->host, ip) != 0) {
            scopy(res->err, (int)sizeof(res->err), "DNS: cannot resolve ");
            sapp(res->err, (int)sizeof(res->err), u->host);
            return -1;
        }
    }
    res->ip[0] = ip[0]; res->ip[1] = ip[1]; res->ip[2] = ip[2]; res->ip[3] = ip[3];
    ip_str(ip, ipb);
    logf2(env, "connect ", ipb);
    h = env->connect(env->net, ip, u->port);
    if (h < 0) { scopy(res->err, (int)sizeof(res->err), "TCP connect failed"); return -1; }
    br.env = env; br.h = h;
    rl = wf_http_build_get(u, req, (int)sizeof(req));
    if (rl < 0) { env->close(env->net, h); scopy(res->err, (int)sizeof(res->err), "request too long"); return -1; }
    res->https = u->https;
    if (u->https) {
        g_tls.io = &br; g_tls.send = b_send; g_tls.recv = b_recv; g_tls.rand = b_rand;
        g_tls.trust = env->trust; g_tls.host = u->host; g_tls.now = env->now;
        if (wf_tls_handshake(&g_tls) != 0) {
            scopy(res->err, (int)sizeof(res->err), "TLS: ");
            sapp(res->err, (int)sizeof(res->err), g_tls.err);
            env->close(env->net, h);
            return -1;
        }
        scopy(res->suite, (int)sizeof(res->suite), wf_tls_suite_name(g_tls.suite));
        scopy(res->leaf_cn, (int)sizeof(res->leaf_cn), g_tls.chain.leaf_cn);
        scopy(res->anchor_cn, (int)sizeof(res->anchor_cn), g_tls.chain.anchor_cn);
        scopy(res->not_after, (int)sizeof(res->not_after), g_tls.chain.not_after);
        res->time_checked = g_tls.chain.time_checked;
        logf2(env, "tls ok: ", res->suite);
        if (wf_tls_write(&g_tls, (const uint8_t*)req, rl) != rl) {
            scopy(res->err, (int)sizeof(res->err), "TLS write failed");
            env->close(env->net, h);
            return -1;
        }
    } else {
        int off = 0;
        while (off < rl) {
            n = env->send(env->net, h, (const uint8_t*)req + off, rl - off);
            if (n <= 0) { scopy(res->err, (int)sizeof(res->err), "send failed"); env->close(env->net, h); return -1; }
            off += n;
        }
    }
    wf_http_resp_init(&g_resp, body, cap);
    for (;;) {
        got = u->https ? wf_tls_read(&g_tls, buf, (int)sizeof(buf)) : env->recv(env->net, h, buf, (int)sizeof(buf));
        if (got < 0) {
            scopy(res->err, (int)sizeof(res->err), u->https ? "TLS read: " : "receive failed");
            if (u->https) sapp(res->err, (int)sizeof(res->err), g_tls.err);
            rc = -1;
            break;
        }
        if (got == 0) {
            if (wf_http_eof(&g_resp) < 0) {
                scopy(res->err, (int)sizeof(res->err), "HTTP: ");
                sapp(res->err, (int)sizeof(res->err), g_resp.error_text);
                rc = -1;
            }
            break;
        }
        n = wf_http_feed(&g_resp, buf, got);
        if (n < 0) {
            scopy(res->err, (int)sizeof(res->err), "HTTP: ");
            sapp(res->err, (int)sizeof(res->err), g_resp.error_text);
            rc = -1;
            break;
        }
        if (n == 1) break;
        /* Lot 69: the buffer is full; the rest would be discarded anyway,
         * so stop reading (a 900 KiB page no longer costs its full download) */
        if (g_resp.truncated && g_resp.status >= 200 && g_resp.status < 300) break;
    }
    if (u->https) wf_tls_close(&g_tls);
    env->close(env->net, h);
    if (rc != 0) return rc;
    res->status = g_resp.status;
    res->body_len = g_resp.body_len;
    res->truncated = g_resp.truncated;
    res->encoding = g_resp.content_encoding;
    res->wire_len = g_resp.body_len;
    if (g_resp.content_encoding != 0 && g_resp.body_len > 0) {
        int tr = 0, n = wf_decode_body_inplace(g_resp.content_encoding, body, g_resp.body_len, cap, &tr);
        if (n < 0) { scopy(res->err, (int)sizeof(res->err), "HTTP: corrupt gzip/deflate body"); return -1; }
        res->body_len = n;
        if (tr) res->truncated = 1;
    }
    scopy(res->content_type, (int)sizeof(res->content_type), g_resp.content_type);
    return 0;
}

int wf_fetch(const wf_fetch_env_t* env, const char* url, uint8_t* body, int cap, wf_fetch_result_t* res) {
    static wf_url_t u, next;
    int max = env->max_redirects, i;
    uint8_t* z = (uint8_t*)res;
    for (i = 0; i < (int)sizeof(*res); i++) z[i] = 0;
    if (max < 0) max = 0;
    if (max > WF_FETCH_MAX_REDIRECTS) max = WF_FETCH_MAX_REDIRECTS;
    if (wf_url_parse(url, &u) != 0) { scopy(res->err, (int)sizeof(res->err), "bad URL"); return -1; }
    for (i = 0;; i++) {
        wf_url_format(&u, res->url, (int)sizeof(res->url));
        logf2(env, "GET ", res->url);
        res->suite[0] = 0; res->leaf_cn[0] = 0; res->anchor_cn[0] = 0; res->time_checked = 0;
        if (one(env, &u, body, cap, res) != 0) return -1;
        if (!(res->status == 301 || res->status == 302 || res->status == 303 || res->status == 307 || res->status == 308))
            return 0;
        if (!g_resp.location[0]) return 0;
        if (i >= max) {
            scopy(res->err, (int)sizeof(res->err), "too many redirects");
            return -1;
        }
        if (wf_url_resolve(&u, g_resp.location, &next) != 0) {
            scopy(res->err, (int)sizeof(res->err), "bad redirect Location");
            return -1;
        }
        if (u.https && !next.https) {
            scopy(res->err, (int)sizeof(res->err), "redirect from https to http refused");
            return -1;
        }
        logf2(env, "redirect -> ", g_resp.location);
        u = next;
        res->redirects++;
    }
}
