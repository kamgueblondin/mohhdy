/* wf_guest.c - Lot 67: Mohhdy transport for wf_fetch (see wf_guest.h).
 * Everything runs in the calling Ring 3 task; sockets are the public
 * SYS_SOCKET_* calls relayed to the networker (no new kernel rights). */
#include "wf_guest.h"
#include "wf_dns.h"
#include "../kernel/sha256.h"
#include "os_syscalls.h"

static int sys1(int nr, uint32_t a) { int r; asm volatile("int $0x80" : "=a"(r) : "a"(nr), "b"(a) : "memory"); return r; }
static int sys3(int nr, uint32_t a, uint32_t b, uint32_t c) {
    int r; asm volatile("int $0x80" : "=a"(r) : "a"(nr), "b"(a), "c"(b), "d"(c) : "memory"); return r;
}
static unsigned ticks(void) { int t; asm volatile("int $0x80" : "=a"(t) : "a"(SYS_TICKS) : "memory"); return (unsigned)t; }
static void yield(void) { asm volatile("int $0x80" : : "a"(SYS_YIELD) : "memory"); }

/* ------------------------------------------------------------ entropy */

static int g_rand_kind = -1;   /* 1 rdrand, 0 weak */
static uint8_t g_pool[32];
static uint32_t g_pool_ctr;

static int cpu_has_rdrand(void) {
    uint32_t a = 1, b, c, d;
    asm volatile("cpuid" : "+a"(a), "=b"(b), "=c"(c), "=d"(d));
    (void)b; (void)d;
    return (c >> 30) & 1U;
}

static int rdrand32(uint32_t* v) {
    int i;
    for (i = 0; i < 10; i++) {
        uint8_t ok;
        uint32_t x;
        asm volatile(".byte 0x0f, 0xc7, 0xf0; setc %1" : "=a"(x), "=qm"(ok) : : "cc");   /* rdrand %eax */
        if (ok) { *v = x; return 0; }
    }
    return -1;
}

static uint64_t rdtsc(void) {
    uint32_t lo, hi;
    asm volatile("rdtsc" : "=a"(lo), "=d"(hi));
    return ((uint64_t)hi << 32) | lo;
}

static void pool_stir(void) {
    sha256_ctx_t c;
    uint8_t blk[64];
    char rtc[16];
    int i;
    sha256_init(&c);
    sha256_update(&c, g_pool, 32);
    for (i = 0; i < 16; i++) {
        uint64_t t0 = rdtsc(), t1;
        yield();
        t1 = rdtsc();
        *(uint64_t*)(blk + (i & 7) * 8) = t1 - t0 + t1;
        if ((i & 7) == 7) sha256_update(&c, blk, 64);
    }
    {
        uint32_t tk = ticks();
        sha256_update(&c, (const uint8_t*)&tk, 4);
    }
    rtc[0] = 0;
    asm volatile("int $0x80" : : "a"(SYS_RTC_UTC), "c"(rtc) : "memory");
    sha256_update(&c, (const uint8_t*)rtc, 16);
    g_pool_ctr++;
    sha256_update(&c, (const uint8_t*)&g_pool_ctr, 4);
    sha256_final(&c, g_pool);
}

static int g_rand(void* net, uint8_t* out, int n) {
    int i = 0;
    (void)net;
    if (g_rand_kind < 0) { g_rand_kind = cpu_has_rdrand(); pool_stir(); }
    while (i < n) {
        uint32_t v;
        if (g_rand_kind == 1 && rdrand32(&v) == 0) {
            int k;
            for (k = 0; k < 4 && i < n; k++) out[i++] = (uint8_t)(v >> (8 * k));
        } else {
            uint8_t blk[32];
            sha256_ctx_t c;
            int k;
            if (g_rand_kind == 1) g_rand_kind = 0;   /* RDRAND kept failing: say weak */
            pool_stir();
            sha256_init(&c);
            sha256_update(&c, g_pool, 32);
            sha256_update(&c, (const uint8_t*)"out", 3);
            sha256_final(&c, blk);
            for (k = 0; k < 32 && i < n; k++) out[i++] = blk[k];
        }
    }
    return 0;
}

const char* wf_guest_entropy(void) {
    if (g_rand_kind < 0) { g_rand_kind = cpu_has_rdrand(); pool_stir(); }
    return g_rand_kind == 1 ? "rdrand" : "weak-tsc";
}

/* ------------------------------------------------------------ clock */

static char g_now[16];
const char* wf_guest_now(void) {
    char rtc[16];
    int i, rc;
    for (i = 0; i < 16; i++) rtc[i] = 0;
    asm volatile("int $0x80" : "=a"(rc) : "a"(SYS_RTC_UTC), "c"(rtc) : "memory");
    if (rc != 0) return 0;
    for (i = 0; i < 14; i++) if (rtc[i] < '0' || rtc[i] > '9') return 0;
    if (rtc[0] < '2' || (rtc[0] == '2' && rtc[1] == '0' && (rtc[2] < '2' || (rtc[2] == '2' && rtc[3] < '5'))))
        return 0;   /* RTC unset (1970, 2000...): not a clock we can check dates with */
    for (i = 0; i < 14; i++) g_now[i] = rtc[i];
    g_now[14] = 0;
    return g_now;
}

/* ------------------------------------------------------------ roots */

static wf_trust_t g_trust;
/* Lot 69: the PEM is decoded in place (DER is always shorter than its
 * base64 and is written behind the read position), so the whole Mozilla
 * set (about 222 KiB of PEM) costs one buffer. The test CA has its own. */
static char g_pem[256 * 1024];
static char g_test_pem[16 * 1024];
static int g_roots_loaded, g_test_ca;

static void load_roots(void) {
    int n, used = 0;
    if (g_roots_loaded) return;
    g_roots_loaded = 1;
    n = sys3(SYS_READFILE, (uint32_t)"/etc/ca-roots.pem", (uint32_t)g_pem, sizeof(g_pem) - 1);
    if (n > 0) wf_trust_add_pem(&g_trust, g_pem, n, (uint8_t*)g_pem, n, &used);
    used = 0;
    n = sys3(SYS_READFILE, (uint32_t)"/etc/ca-test.pem", (uint32_t)g_test_pem, sizeof(g_test_pem) - 1);
    if (n > 0 && wf_trust_add_pem(&g_trust, g_test_pem, n, (uint8_t*)g_test_pem, n, &used) > 0) g_test_ca = 1;
}

int wf_guest_roots(int* test_ca) {
    load_roots();
    if (test_ca) *test_ca = g_test_ca;
    return g_trust.count;
}

/* ------------------------------------------------------------ interface */

typedef struct {
    uint8_t ip[4], mask[4], gw[4], dns[4];
    int ok;
} ifc_t;

static int ifc_query(os_peer_data_request_t* r, uint8_t mode) {
    int i;
    for (i = 0; i < 20; i++) r->data[i] = 0;
    r->op = (uint8_t)OS_PEER_IFCONFIG; r->slot = 0; r->attempts = 0; r->length = 0; r->port = 0; r->flags = 0;
    r->data[0] = mode;
    return sys1(SYS_PEER_DATA, (uint32_t)r);
}

static int ifc_get(ifc_t* f) {
    static os_peer_data_request_t r;
    int i, rc;
    rc = ifc_query(&r, (uint8_t)OS_IFCONFIG_GET);
    f->ok = 0;
    /* No address yet (mode none at boot): ask the networker for a DHCP
     * lease once, the same request as `net-if dhcp`. */
    if (rc == 0 && r.data[0] == (uint8_t)OS_IFCONFIG_MODE_NONE)
        rc = ifc_query(&r, (uint8_t)OS_IFCONFIG_DHCP);
    if (rc != 0 || r.data[0] == (uint8_t)OS_IFCONFIG_MODE_NONE) return -1;
    for (i = 0; i < 4; i++) {
        f->ip[i] = r.data[1 + i]; f->mask[i] = r.data[5 + i]; f->gw[i] = r.data[9 + i];
        f->dns[i] = r.length >= 20U ? r.data[16 + i] : 0U;
    }
    if (!(f->ip[0] | f->ip[1] | f->ip[2] | f->ip[3])) return -1;
    if (!(f->dns[0] | f->dns[1] | f->dns[2] | f->dns[3])) for (i = 0; i < 4; i++) f->dns[i] = f->gw[i];
    f->ok = 1;
    return 0;
}

/* ------------------------------------------------------------ sockets */

typedef struct {
    ifc_t ifc;
    wf_guest_log_fn log;
    unsigned timeout_ticks;
    char err[64];
} gnet_t;

static uint16_t g_port_ctr;

static int g_connect(void* net, const uint8_t ip[4], int port) {
    gnet_t* g = (gnet_t*)net;
    os_socket_connect_request_t c;
    uint8_t seq[4];
    int i, off_link = 0, sid;
    for (i = 0; i < (int)sizeof(c); i++) ((uint8_t*)&c)[i] = 0;
    if (!g->ifc.ok && ifc_get(&g->ifc) != 0) return -1;
    for (i = 0; i < 4; i++) {
        c.local_ip[i] = g->ifc.ip[i];
        c.remote_ip[i] = ip[i];
        if ((ip[i] & g->ifc.mask[i]) != (g->ifc.ip[i] & g->ifc.mask[i])) off_link = 1;
    }
    if (off_link) for (i = 0; i < 4; i++) c.next_hop_ip[i] = g->ifc.gw[i];
    (void)g_rand(0, seq, 4);
    if (!g_port_ctr) g_port_ctr = (uint16_t)(seq[0] | (seq[1] << 8));
    c.local_port = (uint16_t)(43000U + (g_port_ctr++ % 12000U));
    c.remote_port = (uint16_t)port;
    c.local_sequence = (uint32_t)seq[0] | ((uint32_t)seq[1] << 8) | ((uint32_t)seq[2] << 16) | ((uint32_t)seq[3] << 24);
    if (!c.local_sequence) c.local_sequence = 1;
    c.attempts = 2000U;
    sid = sys1(SYS_SOCKET_CONNECT, (uint32_t)&c);
    return sid < 0 ? -1 : sid;
}

static int g_send(void* net, int h, const uint8_t* d, int n) {
    static uint8_t seg[640];
    os_socket_send_request_t s;
    uint16_t seg_len = 0U;
    int k = n > (int)OS_NET_RELAY_MAX_IN ? (int)OS_NET_RELAY_MAX_IN : n, rc, tries;
    (void)net;
    for (tries = 0; tries < 4; tries++) {
        s.socket_id = h; s.payload = d; s.length = (uint16_t)k;
        s.segment = seg; s.capacity = sizeof(seg); s.out_length = &seg_len;
        rc = sys1(SYS_SOCKET_SEND, (uint32_t)&s);
        if (rc == 0) return k;
        if (rc != OS_NET_WIRE_TIMEOUT) return -1;
    }
    return -1;
}

static int g_recv(void* net, int h, uint8_t* d, int cap) {
    gnet_t* g = (gnet_t*)net;
    os_socket_receive_request_t r;
    uint16_t got = 0U;
    unsigned t0 = ticks();
    int rc;
    if (cap > 256) cap = 256;
    for (;;) {
        got = 0U;
        r.socket_id = h; r.buffer = d; r.capacity = (uint16_t)cap; r.out_length = &got;
        rc = sys1(SYS_SOCKET_RECEIVE, (uint32_t)&r);
        if (rc == 0 && got > 0U) return got;
        if (rc == 0) return 0;                       /* FIN */
        if (rc != OS_NET_WIRE_TIMEOUT) return -1;    /* reset / not connected */
        if (ticks() - t0 > g->timeout_ticks) return -1;
    }
}

static void g_close(void* net, int h) { (void)net; (void)sys1(SYS_SOCKET_CLOSE, (uint32_t)h); }

/* DNS over TCP (RFC 1035 4.2.2): QEMU user-net forwards TCP to 10.0.2.3:53
 * to the host resolver, so no UDP path is needed. */
static int g_resolve(void* net, const char* host, uint8_t ip[4]) {
    static uint8_t q[300], a[1024];
    gnet_t* g = (gnet_t*)net;
    uint8_t idb[2];
    uint16_t id;
    int n, h, got = 0, want = -1, r, off = 0;
    if (!g->ifc.ok && ifc_get(&g->ifc) != 0) return -1;
    (void)g_rand(0, idb, 2);
    id = (uint16_t)(idb[0] | (idb[1] << 8));
    n = wf_dns_build_query(host, id, q + 2, (int)sizeof(q) - 2);
    if (n < 0) return -1;
    q[0] = (uint8_t)(n >> 8); q[1] = (uint8_t)n;
    h = g_connect(net, g->ifc.dns, 53);
    if (h < 0) return -1;
    while (off < n + 2) {
        r = g_send(net, h, q + off, n + 2 - off);
        if (r <= 0) { g_close(net, h); return -1; }
        off += r;
    }
    while (want < 0 || got < want + 2) {
        r = g_recv(net, h, a + got, (int)sizeof(a) - got);
        if (r <= 0) break;
        got += r;
        if (want < 0 && got >= 2) { want = (a[0] << 8) | a[1]; if (want + 2 > (int)sizeof(a)) break; }
    }
    g_close(net, h);
    if (want < 0 || got < want + 2) return -1;
    return wf_dns_parse_a(a + 2, want, id, host, ip) == 0 ? 0 : -1;
}

static wf_guest_log_fn g_log_fn;
static void g_log(void* net, const char* line) { (void)net; if (g_log_fn) g_log_fn(line); }

int wf_guest_fetch(const char* url, uint8_t* body, int cap, wf_fetch_result_t* res, wf_guest_log_fn log) {
    return wf_guest_fetch_ex(url, 0, body, cap, res, log);
}

static wf_cookie_jar_t g_jar;
static int g_jar_ready;
wf_cookie_jar_t* wf_guest_cookies(void) {
    if (!g_jar_ready) { wf_cookie_jar_init(&g_jar); g_jar_ready = 1; }
    return &g_jar;
}

/* Lot 72: HTTP keep-alive (on by default) and the forced TLS suite */
static int g_keepalive = 1;
static uint16_t g_tls_suite;
static unsigned g_last_fetch;
void wf_guest_set_keepalive(int on) { g_keepalive = on ? 1 : 0; }
int wf_guest_keepalive(void) { return g_keepalive; }
void wf_guest_set_tls_suite(uint16_t id) { g_tls_suite = id; }
uint16_t wf_guest_tls_suite(void) { return g_tls_suite; }

int wf_guest_fetch_ex(const char* url, const wf_fetch_req_t* req, uint8_t* body, int cap, wf_fetch_result_t* res,
                      wf_guest_log_fn log) {
    static gnet_t g;
    static wf_fetch_env_t e;
    int i, rc;
    for (i = 0; i < (int)sizeof(g); i++) ((uint8_t*)&g)[i] = 0;
    for (i = 0; i < (int)sizeof(e); i++) ((uint8_t*)&e)[i] = 0;
    load_roots();
    g.log = log;
    g.timeout_ticks = 2000U;   /* 20 s without a byte on a connection */
    g_log_fn = log;
    e.net = &g;
    e.resolve = g_resolve; e.connect = g_connect; e.send = g_send; e.recv = g_recv; e.close = g_close;
    e.rand = g_rand; e.log = log ? g_log : 0;
    e.trust = &g_trust;
    e.now = wf_guest_now();
    e.max_redirects = WF_FETCH_MAX_REDIRECTS;
    e.jar = wf_guest_cookies();   /* Lot 71: shared by fetch, the browser, forms and search */
    e.keepalive = g_keepalive;
    e.tls_suite = g_tls_suite;
    if (ifc_get(&g.ifc) != 0) {
        const char* m = "no network interface (no DHCP lease: boot with -netdev user + ne2k_isa)";
        for (i = 0; i < (int)sizeof(res->err) - 1 && m[i]; i++) res->err[i] = m[i];
        res->err[i] = 0;
        return -1;
    }
    /* an idle kept-alive connection older than 10 s is dropped first (most
     * servers close theirs after 5-15 s; a dead one would cost a timeout) */
    if (!g_keepalive || ticks() - g_last_fetch > 1000U) wf_fetch_drop(&e);
    rc = wf_fetch_ex(&e, url, req, body, cap, res);
    g_last_fetch = ticks();
    return rc;
}
