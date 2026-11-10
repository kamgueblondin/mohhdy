/* wf_fetch.c - Lot 67: HTTP(S) GET with bounded redirects (see wf_fetch.h). */
#include "wf_fetch.h"
#include "wf_dns.h"
#include "wf_inflate.h"

static wf_tls_t g_tls;
static wf_http_resp_t g_resp;

typedef struct { const wf_fetch_env_t* env; int h; } bridge_t;

/* Lot 72: one kept-alive connection (bounded: a single socket + the TLS
 * state already in g_tls) */
static struct {
    int open, h, https, port;
    char host[WF_URL_HOST_MAX];
    bridge_t br;
} g_conn;
static int g_opened, g_reused;
/* Lot 89: responses followed by bytes nobody asked for (connection dropped) */
static int g_unsolicited;
/* Lot 86: settings "net off". Refuses before resolve/connect. */
static int g_offline;

void wf_fetch_set_offline(int on) { g_offline = on ? 1 : 0; }
int wf_fetch_offline(void) { return g_offline; }

void wf_fetch_conn_stats(int* opened, int* reused) { if (opened) *opened = g_opened; if (reused) *reused = g_reused; }
int wf_fetch_unsolicited(void) { return g_unsolicited; }

static int seq(const char* a, const char* b) { int i; for (i = 0; a[i] && a[i] == b[i]; i++) {} return a[i] == b[i]; }

static void conn_close(const wf_fetch_env_t* env) {
    if (!g_conn.open) return;
    if (g_conn.https) wf_tls_close(&g_tls);
    env->close(env->net, g_conn.h);
    g_conn.open = 0;
}

void wf_fetch_drop(const wf_fetch_env_t* env) { conn_close(env); }

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

/* One request, on the kept-alive connection when it matches (Lot 72),
 * else on a fresh one. 0 = response parsed. *stale = 1 when a reused
 * connection turned out closed before any answer byte (caller retries). */
static int one(const wf_fetch_env_t* env, const wf_url_t* u, const wf_fetch_req_t* rq, uint8_t* body, int cap, wf_fetch_result_t* res, int* stale) {
    static char req[WF_URL_PATH_MAX + WF_URL_HOST_MAX + 256];
    static uint8_t buf[4096];
    static char ipb[20];
    bridge_t* br = &g_conn.br;
    int h, n, rl, rc = 0, got, reuse, total = 0, keep;
    uint8_t ip[4];
    *stale = 0;
    reuse = env->keepalive && g_conn.open && g_conn.https == u->https && g_conn.port == u->port && seq(g_conn.host, u->host) &&
            (!u->https || g_tls.established);
    if (!reuse) conn_close(env);
    if (reuse) {
        h = g_conn.h;
        logf2(env, "reuse connection to ", u->host);
        goto connected;
    }
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
    g_opened++;
    res->connections++;
connected:
    br->env = env; br->h = h;
    {
        /* Lot 71: the jar's Cookie line goes after the caller's extra lines */
        static char ex[WF_COOKIE_HDR + 1024];
        const char* extra = rq ? rq->extra : 0;
        if (env->jar) {
            int el = 0, ck;
            uint32_t before = env->jar->sent;
            ex[0] = 0;
            if (extra) { while (extra[el] && el < (int)sizeof(ex) - 1) { ex[el] = extra[el]; el++; } ex[el] = 0; }
            if (extra && extra[el]) { env->close(env->net, h); g_conn.open = 0; scopy(res->err, (int)sizeof(res->err), "request too long"); return -1; }
            ck = wf_cookie_header(env->jar, u->host, u->path, u->https, ex + el, (int)sizeof(ex) - el);
            if (ck > 0) { extra = ex; res->cookies_sent = (int)(env->jar->sent - before); }
            else res->cookies_sent = 0;
        }
        rl = wf_http_build_request_ka(u, rq && rq->method ? rq->method : "GET", rq ? rq->ctype : 0,
                                      rq && rq->body ? rq->body_len : -1, extra, env->keepalive, req, (int)sizeof(req));
    }
    if (rl < 0) { env->close(env->net, h); g_conn.open = 0; scopy(res->err, (int)sizeof(res->err), "request too long"); return -1; }
    res->https = u->https;
    if (u->https && !reuse) {
        g_tls.io = br; g_tls.send = b_send; g_tls.recv = b_recv; g_tls.rand = b_rand;
        g_tls.trust = env->trust; g_tls.host = u->host; g_tls.now = env->now;
        g_tls.only_suite = env->tls_suite;
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
    }
    if (reuse) {
        g_reused++;
        res->reused++;
        if (u->https) {
            scopy(res->suite, (int)sizeof(res->suite), wf_tls_suite_name(g_tls.suite));
            scopy(res->leaf_cn, (int)sizeof(res->leaf_cn), g_tls.chain.leaf_cn);
            scopy(res->anchor_cn, (int)sizeof(res->anchor_cn), g_tls.chain.anchor_cn);
            scopy(res->not_after, (int)sizeof(res->not_after), g_tls.chain.not_after);
            res->time_checked = g_tls.chain.time_checked;
        }
    }
    g_conn.open = 0;                          /* re-armed below when the answer allows it */
    if (u->https) {
        if (wf_tls_write(&g_tls, (const uint8_t*)req, rl) != rl ||
            (rq && rq->body && rq->body_len > 0 && wf_tls_write(&g_tls, (const uint8_t*)rq->body, rq->body_len) != rq->body_len)) {
            scopy(res->err, (int)sizeof(res->err), "TLS write failed");
            env->close(env->net, h);
            if (reuse) *stale = 1;
            return -1;
        }
    } else {
        int off = 0;
        while (off < rl) {
            n = env->send(env->net, h, (const uint8_t*)req + off, rl - off);
            if (n <= 0) { scopy(res->err, (int)sizeof(res->err), "send failed"); env->close(env->net, h); if (reuse) *stale = 1; return -1; }
            off += n;
        }
        off = 0;
        while (rq && rq->body && off < rq->body_len) {
            n = env->send(env->net, h, (const uint8_t*)rq->body + off, rq->body_len - off);
            if (n <= 0) { scopy(res->err, (int)sizeof(res->err), "send failed"); env->close(env->net, h); return -1; }
            off += n;
        }
    }
    wf_http_resp_init(&g_resp, body, cap);
    for (;;) {
        got = u->https ? wf_tls_read(&g_tls, buf, (int)sizeof(buf)) : env->recv(env->net, h, buf, (int)sizeof(buf));
        if (got <= 0 && reuse && total == 0) {
            /* the server closed the idle connection: retry on a new one */
            if (u->https) g_tls.established = 0;
            env->close(env->net, h);
            *stale = 1;
            scopy(res->err, (int)sizeof(res->err), "kept-alive connection closed by the server");
            return -1;
        }
        if (got > 0) total += got;
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
    keep = env->keepalive && rc == 0 && g_resp.done && !g_resp.conn_close && !g_resp.truncated && (!u->https || !g_tls.peer_closed);
    if (rc == 0 && g_resp.done && g_resp.trailing > 0) {
        /* Lot 89: more bytes came after this answer in the same read. The
         * next request on this connection would take them as its answer
         * (an HTTP desync), so the connection is never kept. */
        g_unsolicited++;
        res->unsolicited = g_resp.trailing;
        if (keep) logf2(env, "unsolicited bytes after the response: connection not kept, host ", u->host);
        keep = 0;
    }
    if (keep) {
        g_conn.open = 1; g_conn.h = h; g_conn.https = u->https; g_conn.port = u->port;
        scopy(g_conn.host, (int)sizeof(g_conn.host), u->host);
    } else {
        if (u->https) wf_tls_close(&g_tls);
        env->close(env->net, h);
    }
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
    if (env->jar && g_resp.status) {
        int k = wf_cookie_from_head(env->jar, u->host, u->path, u->https, g_resp.hdr, g_resp.hdr_len, env->now);
        res->cookies_set += k;
        if (k) logf2(env, "cookies stored for ", u->host);
    }
    return 0;
}

/* Lot 73: the final answer of a plain GET (no redirect) and the cache:
 * 304 for a stale entry serves its body, 200 is stored when allowed */
static void cache_after(const wf_fetch_env_t* env, const char* key, int idx, uint8_t* body, int cap, wf_fetch_result_t* res) {
    static wf_cache_meta_t m;
    wf_cache_meta(g_resp.hdr, g_resp.hdr_len, &m);
    if (res->status == 304 && idx >= 0) {
        int n = wf_cache_body(env->cache, idx, body, cap);
        if (n < 0) return;
        wf_cache_refresh(env->cache, idx, &m, env->now_s);
        res->status = 200;
        res->body_len = n;
        res->truncated = 0;
        scopy(res->content_type, (int)sizeof(res->content_type), env->cache->e[idx].ctype);
        res->cached = 2;
        logf2(env, "cache revalidated ", key);
        return;
    }
    if (res->status == 200 && !res->truncated) {
        if (wf_cache_store(env->cache, key, &m, res->content_type, body, res->body_len, env->now_s) >= 0)
            logf2(env, "cache stored ", key);
    } else if (res->status != 304) (void)wf_cache_drop(env->cache, key);
}

int wf_fetch(const wf_fetch_env_t* env, const char* url, uint8_t* body, int cap, wf_fetch_result_t* res) {
    return wf_fetch_ex(env, url, 0, body, cap, res);
}

int wf_fetch_ex(const wf_fetch_env_t* env, const char* url, const wf_fetch_req_t* req0, uint8_t* body, int cap,
                wf_fetch_result_t* res) {
    static wf_url_t u, next;
    static wf_fetch_req_t rq, vrq;
    static char key[WF_CACHE_URL], vext[1024];
    const wf_fetch_req_t* req = req0;
    int max = env->max_redirects, i, plain, cache_idx;
    uint8_t* z = (uint8_t*)res;
    for (i = 0; i < (int)sizeof(*res); i++) z[i] = 0;
    if (g_offline) { scopy(res->err, (int)sizeof(res->err), "network off"); return -1; }
    if (env && env->jar && env->now) (void)wf_cookie_expire(env->jar, env->now);
    if (max < 0) max = 0;
    if (max > WF_FETCH_MAX_REDIRECTS) max = WF_FETCH_MAX_REDIRECTS;
    if (wf_url_parse(url, &u) != 0) { scopy(res->err, (int)sizeof(res->err), "bad URL"); return -1; }
    cache_idx = -1;
    plain = env->cache && (!req0 || ((!req0->method || seq(req0->method, "GET")) && !req0->body));
    if (plain) {
        int state;
        wf_url_format(&u, key, (int)sizeof(key));
        state = wf_cache_lookup(env->cache, key, env->now_s, &cache_idx);
        if (state == WF_CACHE_FRESH) {
            int n = wf_cache_body(env->cache, cache_idx, body, cap);
            if (n >= 0) {
                scopy(res->url, (int)sizeof(res->url), key);
                scopy(res->content_type, (int)sizeof(res->content_type), env->cache->e[cache_idx].ctype);
                res->status = 200;
                res->body_len = n;
                res->cached = 1;
                logf2(env, "cache fresh ", key);
                return 0;
            }
        }
        if (state == WF_CACHE_STALE) {
            /* the validators go after the caller's extra lines */
            int el = 0, vl;
            const char* ex0 = req0 ? req0->extra : 0;
            if (ex0) while (ex0[el] && el < (int)sizeof(vext) - 1) { vext[el] = ex0[el]; el++; }
            vext[el] = 0;
            vl = (ex0 && ex0[el]) ? -1 : wf_cache_validators(env->cache, cache_idx, vext + el, (int)sizeof(vext) - el);
            if (vl > 0) {
                if (req0) vrq = *req0; else { vrq.method = 0; vrq.body = 0; vrq.body_len = 0; vrq.ctype = 0; }
                vrq.extra = vext;
                req = &vrq;
            }
        } else cache_idx = -1;
    }
    for (i = 0;; i++) {
        wf_url_format(&u, res->url, (int)sizeof(res->url));
        logf2(env, req && req->method ? "POST " : "GET ", res->url);
        res->suite[0] = 0; res->leaf_cn[0] = 0; res->anchor_cn[0] = 0; res->time_checked = 0;
        {
            int stale = 0;
            if (one(env, &u, req, body, cap, res, &stale) != 0) {
                /* Lot 72: a stale kept-alive connection is retried once on a
                 * fresh one, for a GET only (never replay a POST body) */
                if (!stale || (req && req->method && !seq(req->method, "GET"))) return -1;
                logf2(env, "retry on a new connection: ", res->err);
                res->err[0] = 0;
                if (one(env, &u, req, body, cap, res, &stale) != 0) return -1;
            }
        }
        if (!(res->status == 301 || res->status == 302 || res->status == 303 || res->status == 307 || res->status == 308) ||
            !g_resp.location[0]) {
            if (plain && res->redirects == 0) cache_after(env, key, cache_idx, body, cap, res);
            return 0;
        }
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
        if (req == &vrq) req = req0;          /* Lot 73: validators are for the first URL only */
        if (req) {
            /* 301/302/303 after a POST: GET without body; the extra headers
             * (API keys) never follow to another host */
            int same = 1, k;
            for (k = 0; u.host[k] || next.host[k]; k++) if (u.host[k] != next.host[k]) { same = 0; break; }
            rq = *req;
            if (res->status != 307 && res->status != 308) { rq.method = 0; rq.body = 0; rq.body_len = 0; rq.ctype = 0; }
            if (!same || u.port != next.port) rq.extra = 0;
            req = (rq.method || rq.extra) ? &rq : 0;
        }
        u = next;
        res->redirects++;
    }
}
