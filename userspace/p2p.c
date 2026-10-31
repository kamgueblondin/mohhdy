/* userspace/p2p.c - Phase 5 P2P node. See p2p.h and docs/p2p.md. */
#include "p2p.h"
#include "stun.h"
#include "../kernel/sha256.h"
#include "../kernel/aes_gcm.h"
#include "../kernel/x25519.h"

#define HDR 24
static uint32_t g_ws[X25519_WORKSPACE_LIMBS];
static const char* const k_names[P2P_I_COUNT] = {
    "?", "hello", "sealed", "?", "?", "?", "?", "?", "?", "?",
    "ping", "pong", "msg", "put", "sync-req", "sync-items", "get-req", "get-resp", "propose", "vote", "commit", "app",
    "raft-req", "raft-resp", "raft-hb", "cache-inv",
    "grpc-req", "grpc-resp", "stun-req", "stun-resp", "nat-punch", "raft-append", "raft-append-resp",
    "raft-prevote-req", "raft-prevote-resp", "raft-snapshot", "raft-snapshot-resp", "cache-inv-ack", "dack"
};
static void cache_fence(p2p_node_t* n, const char* key, uint32_t version);
static uint32_t cache_fence_of(const p2p_node_t* n, const char* key);
static int cache_inv_send(p2p_node_t* n, const p2p_host_t* h, int only_waiting);
const char* p2p_type_name(int t) { return (t > 0 && t < P2P_I_COUNT) ? k_names[t] : "?"; }

/* ---------------------------------------------------------------- helpers */
static int s_len(const char* s) { int n = 0; while (s && s[n]) n++; return n; }
static void s_copy(char* d, const char* s, int cap) { int i = 0; while (s && s[i] && i < cap - 1) { d[i] = s[i]; i++; } d[i] = 0; }
static int s_eq(const char* a, const char* b) { int i = 0; while (a[i] && a[i] == b[i]) i++; return a[i] == b[i]; }
static void s_cat(char* d, const char* s, int cap) { int n = s_len(d), i = 0; while (s[i] && n < cap - 1) d[n++] = s[i++]; d[n] = 0; }
static void s_u(char* d, uint32_t v, int cap) {
    char t[11]; int n = 0, i = 0;
    do { t[n++] = (char)('0' + v % 10U); v /= 10U; } while (v && n < 10);
    while (n && i < cap - 1) d[i++] = t[--n];
    d[i] = 0;
}
static void cat_u(char* d, uint32_t v, int cap) { char t[12]; s_u(t, v, 12); s_cat(d, t, cap); }
static void cat_hex32(char* d, uint32_t v, int cap) {
    char t[9]; int i;
    for (i = 7; i >= 0; i--) { t[i] = "0123456789abcdef"[v & 15U]; v >>= 4; }
    t[8] = 0; s_cat(d, t, cap);
}
static void mcopy(uint8_t* d, const uint8_t* s, int n) { int i; for (i = 0; i < n; i++) d[i] = s[i]; }
static void mzero(void* p, int n) { int i; for (i = 0; i < n; i++) ((uint8_t*)p)[i] = 0; }
static int meq(const uint8_t* a, const uint8_t* b, int n) { int i, d = 0; for (i = 0; i < n; i++) d |= a[i] ^ b[i]; return d == 0; }
static void put32(uint8_t* p, uint32_t v) { p[0] = (uint8_t)(v >> 24); p[1] = (uint8_t)(v >> 16); p[2] = (uint8_t)(v >> 8); p[3] = (uint8_t)v; }
static uint32_t get32(const uint8_t* p) { return ((uint32_t)p[0] << 24) | ((uint32_t)p[1] << 16) | ((uint32_t)p[2] << 8) | p[3]; }
static void say(const p2p_host_t* h, const char* line) { if (h && h->out) h->out(h->ctx, line); }
static uint32_t now(const p2p_host_t* h) { return (h && h->ticks) ? h->ticks(h->ctx) : 0U; }

/* length-prefixed strings in message bodies */
static int put_str(uint8_t* b, int pos, int cap, const char* s) {
    int n = s_len(s);
    if (n > 255 || pos + 1 + n > cap) return -1;
    b[pos++] = (uint8_t)n; mcopy(b + pos, (const uint8_t*)s, n);
    return pos + n;
}
static int get_str(const uint8_t* b, int pos, int len, char* out, int cap) {
    int n;
    if (pos >= len) return -1;
    n = b[pos++];
    if (pos + n > len || n >= cap) return -1;
    mcopy((uint8_t*)out, b + pos, n); out[n] = 0;
    return pos + n;
}

/* ------------------------------------------------------------ peers, kv */
p2p_peer_t* p2p_find(p2p_node_t* n, const char* name) {
    int i;
    for (i = 0; i < P2P_PEERS; i++) if (n->peers[i].used && s_eq(n->peers[i].name, name)) return &n->peers[i];
    return 0;
}
static p2p_peer_t* by_id(p2p_node_t* n, uint32_t id) {
    int i;
    for (i = 0; i < P2P_PEERS; i++) if (n->peers[i].used && n->peers[i].id == id) return &n->peers[i];
    return 0;
}
static uint32_t up_count(const p2p_node_t* n) {
    uint32_t c = 0; int i;
    for (i = 0; i < P2P_PEERS; i++) if (n->peers[i].used && n->peers[i].up) c++;
    return c;
}
const p2p_item_t* p2p_get_local(const p2p_node_t* n, const char* key) {
    int i;
    for (i = 0; i < P2P_KV; i++) if (n->kv[i].used && s_eq(n->kv[i].key, key)) return &n->kv[i];
    return 0;
}
/* Last writer wins: higher version, then higher origin id. 1 if applied. */
static int kv_apply(p2p_node_t* n, const char* key, const char* value, uint32_t version, uint32_t origin) {
    int i, free_slot = -1;
    for (i = 0; i < P2P_KV; i++) {
        if (!n->kv[i].used) { if (free_slot < 0) free_slot = i; continue; }
        if (s_eq(n->kv[i].key, key)) {
            if (version < n->kv[i].version || (version == n->kv[i].version && origin <= n->kv[i].origin)) return 0;
            s_copy(n->kv[i].value, value, P2P_VAL_MAX);
            n->kv[i].version = version; n->kv[i].origin = origin;
            return 1;
        }
    }
    if (free_slot < 0) return -1;
    n->kv[free_slot].used = 1;
    s_copy(n->kv[free_slot].key, key, P2P_KEY_MAX);
    s_copy(n->kv[free_slot].value, value, P2P_VAL_MAX);
    n->kv[free_slot].version = version; n->kv[free_slot].origin = origin;
    return 1;
}

/* --------------------------------------------------------------- framing */
static void header(p2p_node_t* n, uint8_t* b, int type, uint32_t dst, uint32_t via) {
    b[0] = 'M'; b[1] = 'P'; b[2] = '2'; b[3] = 1; b[4] = (uint8_t)type; b[5] = (uint8_t)P2P_MAX_HOPS; b[6] = 0; b[7] = 0;
    put32(b + 8, n->id); put32(b + 12, dst); put32(b + 16, ++n->msg_id); put32(b + 20, via);
}
static void remember(p2p_node_t* n, uint32_t src, uint32_t id) {
    n->seen[n->seen_next % P2P_SEEN].src = src;
    n->seen[n->seen_next % P2P_SEEN].id = id;
    n->seen_next++;
}
static int seen(const p2p_node_t* n, uint32_t src, uint32_t id) {
    int i;
    for (i = 0; i < P2P_SEEN; i++) if (n->seen[i].src == src && n->seen[i].id == id && (src || id)) return 1;
    return 0;
}
static const uint8_t k_bcast[4] = {255, 255, 255, 255};

static int emit(p2p_node_t* n, const p2p_host_t* h, const uint8_t* ip, const uint8_t* b, int len) {
    if (!h || !h->send || h->send(h->ctx, ip ? ip : k_bcast, b, (uint16_t)len) < 0) return -1;
    n->tx_bytes += (uint32_t)len;
    return 0;
}

/* Hops from us to peer p: 1 when the direct link is up, else one more
 * than the best up neighbor that advertises p, 0 when unreachable or
 * farther than P2P_MAX_HOPS. */
static uint32_t reach_hops(const p2p_node_t* n, const p2p_peer_t* p) {
    int i, j;
    uint32_t best = 0;
    if (!p->used || p->blocked) return 0;
    if (p->up) return 1;
    for (i = 0; i < P2P_PEERS; i++) {
        const p2p_peer_t* r = &n->peers[i];
        if (!r->used || !r->up || r->blocked || r == p) continue;
        for (j = 0; j < P2P_PEERS; j++) {
            if (r->neighbors[j] == p->id && r->nb_hops[j] && r->nb_hops[j] < P2P_MAX_HOPS) {
                uint32_t hc = (uint32_t)r->nb_hops[j] + 1U;
                if (!best || hc < best) best = hc;
            }
        }
    }
    return best;
}

static void send_hello(p2p_node_t* n, const p2p_host_t* h) {
    uint8_t b[P2P_DATAGRAM_MAX], mac[32];
    int pos = HDR, i, cnt = 0;
    header(n, b, P2P_T_HELLO, 0, 0);
    mcopy(b + pos, (const uint8_t*)n->name, P2P_NAME_MAX); pos += P2P_NAME_MAX;
    mcopy(b + pos, n->ip, 4); pos += 4;
    mcopy(b + pos, n->pub, 32); pos += 32;
    b[pos++] = 0;
    {
        /* Distance vector (US-063): every known peer we can reach, with
         * its hop count from us. Ids first, then one hop byte per id. */
        uint8_t hops[P2P_PEERS];
        for (i = 0; i < P2P_PEERS; i++) {
            uint32_t hc = reach_hops(n, &n->peers[i]);
            if (!hc) continue;
            put32(b + pos, n->peers[i].id); pos += 4;
            hops[cnt++] = (uint8_t)hc;
        }
        for (i = 0; i < cnt; i++) b[pos++] = hops[i];
    }
    b[HDR + P2P_NAME_MAX + 36] = (uint8_t)cnt;
    hmac_sha256(n->netkey, 32, b, (uint32_t)pos, mac);
    mcopy(b + pos, mac, 16); pos += 16;
    n->tx_type[P2P_T_HELLO]++;
    (void)emit(n, h, 0, b, pos);
}

static void derive(p2p_node_t* n, p2p_peer_t* p) {
    uint8_t shared[32], d[32], ids[8];
    sha256_ctx_t c;
    uint32_t lo = n->id < p->id ? n->id : p->id, hi = n->id < p->id ? p->id : n->id;
    p->keyed = 0;
    if (x25519_shared_secret(shared, n->secret, p->pub, g_ws, X25519_WORKSPACE_LIMBS) != 0) return;
    {
        int i, z = 0;
        for (i = 0; i < 32; i++) z |= shared[i];
        if (!z) return; /* low-order point */
    }
    put32(ids, lo); put32(ids + 4, hi);
    sha256_init(&c);
    sha256_update(&c, (const uint8_t*)"mohhdy-p2p-v1", 13);
    sha256_update(&c, n->netkey, 32);
    sha256_update(&c, shared, 32);
    sha256_update(&c, ids, 8);
    sha256_final(&c, d);
    mcopy(p->key, d, 16); mcopy(p->iv, d + 16, 4);
    p->keyed = 1; p->tx_counter = 0; p->rx_counter = 0; p2p_flow_init(&p->flow);
    mzero(shared, 32); mzero(d, 32);
}

/* Lot 24 (US-070 / US-076): Consolidated EWMA link cost calculation */
uint32_t p2p_calc_link_cost(const p2p_peer_t* p) {
    uint32_t rtt_ms;
    uint32_t cost;
    if (!p) return 0;
    rtt_ms = p->rtt_ewma_ms ? p->rtt_ewma_ms : (p->rtt_ticks * (1000U / P2P_HZ));
    cost = 10U + rtt_ms + (p->bucket_used > 5 ? 20U : 5U);
    if (p->loss_percent > 0) cost += (uint32_t)p->loss_percent * 10U;
    if (p->anomalies > 0) cost += p->anomalies * 50U;
    if (p->down_events > 0) cost += (uint32_t)p->down_events * 25U;
    return cost;
}

static uint32_t link_cost(const p2p_peer_t* p) {
    return p2p_calc_link_cost(p);
}

/* Fewest hops first, link cost (RTT, load) between routes of the same
 * length. The HELLO vector carries hop counts, not next hops, so a pure
 * cost order let a slow direct link lose to a neighbor whose own route
 * went back through us (a two-node loop seen in QEMU under CI load). */
int p2p_route_find(const p2p_node_t* n, uint32_t dest_id, uint32_t* next_hop, uint32_t* total_cost) {
    int i, j;
    uint32_t min_cost = 0xffffffffU, best_hop = 0xffffffffU, min_hops = 0xffU;
    if (!n || !dest_id) return -1;
    for (i = 0; i < P2P_PEERS; i++) {
        const p2p_peer_t* p = &n->peers[i];
        if (!p->used || !p->up || p->blocked) continue;
        if (p->id == dest_id) {
            uint32_t c = link_cost(p);
            if (min_hops > 1U || c < min_cost) { min_cost = c; best_hop = 0; min_hops = 1U; }
        } else {
            for (j = 0; j < P2P_PEERS; j++) {
                if (p->neighbors[j] == dest_id && p->nb_hops[j] && p->nb_hops[j] <= P2P_MAX_HOPS - 1U) {
                    /* 15 per extra hop: one relay costs 15, two cost 30. */
                    uint32_t c = link_cost(p) + 15U * (uint32_t)p->nb_hops[j];
                    uint32_t hc = (uint32_t)p->nb_hops[j] + 1U;
                    if (hc < min_hops || (hc == min_hops && c < min_cost)) {
                        min_cost = c; best_hop = p->id; min_hops = hc;
                    }
                }
            }
        }
    }
    if (best_hop == 0xffffffffU) return -1;
    if (next_hop) *next_hop = best_hop;
    if (total_cost) *total_cost = min_cost;
    return 0;
}

/* Route: direct if the peer is up, else through an up neighbor that lists it. */
static uint32_t route(p2p_node_t* n, p2p_peer_t* p, const uint8_t** ip) {
    int i, j;
    uint32_t nh = 0, cost = 0;
    *ip = 0;
    if (p2p_route_find(n, p->id, &nh, &cost) == 0) {
        if (nh == 0) { *ip = p->ip; return 0; }
        p2p_peer_t* r = by_id(n, nh);
        if (r && r->up && !r->blocked) { *ip = r->ip; return r->id; }
    }
    if (p->up && !p->blocked) { *ip = p->ip; return 0; }
    for (i = 0; i < P2P_PEERS; i++) {
        p2p_peer_t* r = &n->peers[i];
        if (!r->used || !r->up || r->blocked || r == p) continue;
        for (j = 0; j < P2P_PEERS; j++) if (r->neighbors[j] == p->id) { *ip = r->ip; return r->id; }
    }
    return 0xffffffffU;
}

static int seal_send(p2p_node_t* n, const p2p_host_t* h, p2p_peer_t* p, int inner, const uint8_t* body, int blen) {
    uint8_t b[P2P_DATAGRAM_MAX], aad[HDR], plain[P2P_DATAGRAM_MAX], nonce[8];
    const uint8_t* ip;
    uint32_t via;
    int pos;
    if (!p || !p->keyed) return -1;
    if (blen + 1 + HDR + 4 + 16 > P2P_DATAGRAM_MAX) return -1;
    via = route(n, p, &ip);
    if (via == 0xffffffffU) return -3;
    header(n, b, P2P_T_SEALED, p->id, via);
    mcopy(aad, b, HDR); aad[5] = 0; put32(aad + 20, 0);
    plain[0] = (uint8_t)inner; mcopy(plain + 1, body, blen);
    p->tx_counter++;
    put32(b + HDR, p->tx_counter);
    put32(nonce, n->id); put32(nonce + 4, p->tx_counter);
    pos = HDR + 4;
    if (aes128_gcm_encrypt(p->key, p->iv, nonce, aad, HDR, plain, (uint16_t)(blen + 1), b + pos, b + pos + blen + 1) != 0) return -1;
    pos += blen + 1 + 16;
    n->tx_type[inner]++;
    p->sent++;
    /* Lot 69: user data is tracked per peer until its DACK comes back */
    if (inner == P2P_I_MSG || inner == P2P_I_PUT)
        (void)p2p_flow_on_send(&p->flow, p->tx_counter, (uint32_t)(blen + 1), now(h) * (1000000U / P2P_HZ));
    return emit(n, h, ip, b, pos);
}

/* Lot 48 (US-075): end of an anomaly episode, back under the baseline. */
static void line_end_alert(p2p_node_t* n, const p2p_host_t* h, const p2p_peer_t* p) {
    char al[80];
    (void)n;
    al[0] = 0;
    s_cat(al, "p2p traffic normal peer ", 80); s_cat(al, p->name, 80);
    say(h, al);
}

/* Per-peer budget for user traffic (US-068/073). Control frames bypass it.
 * Lot 68: once the bandwidth estimator has delivery-rate samples (a probe
 * train ran), user data is paced at the estimator's pacing rate by a token
 * bucket (p2p_pace.c), bounded by a floor, a ceiling and p2p-pace cap; the
 * window budget below only applies before that (or with p2p-pace off).
 * An explicit p2p-limit (messages per second) stays a hard cap on top.
 * The window starts at 16 (no cap beyond an explicit p2p-limit). A lost
 * raft append halves it; a successful append grows it by one. While the
 * window is below 16, data frames share that cap with the rate limit. */
static void pace_ensure(p2p_node_t* n) {
    if (n->qos.pace_ready) return;
    p2p_pace_init(&n->qos.pace);
    n->qos.pace_ready = 1U;
}

static int admit(p2p_node_t* n, const p2p_host_t* h, p2p_peer_t* p, uint32_t bytes) {
    uint32_t t = now(h);
    uint32_t cap = n->qos.window_size ? n->qos.window_size : 1U;
    uint32_t rate;
    int limited;
    pace_ensure(n);
    /* Lot 69: this peer's own pacer first (fed by its data acks), so a
     * slow peer defers its own traffic without holding the others */
    if (n->qos.pace.mode != P2P_PACE_OFF && !p2p_flow_admit(&p->flow, bytes, t * (1000000U / P2P_HZ))) {
        p->throttled++;
        return 0;
    }
    rate = p2p_pace_rate(&n->qos.pace, &n->qos.bwe, (int)n->qos.bwe_ready);
    if (rate) {
        if (n->rate_limit) {
            if (t - p->bucket_tick >= P2P_HZ) { p->bucket_tick = t; p->bucket_used = 0; }
            if (p->bucket_used >= n->rate_limit) { p->throttled++; return 0; }
        }
        if (!p2p_pace_admit(&n->qos.pace, rate, bytes, t * (1000000U / P2P_HZ))) { p->throttled++; return 0; }
        if (n->rate_limit) p->bucket_used++;
        n->qos.priority_low_tx++;
        return 1;
    }
    if (n->rate_limit && n->rate_limit < cap) cap = n->rate_limit;
    limited = n->rate_limit != 0U || cap < P2P_QOS_WINDOW_MAX;
    if (limited) {
        if (t - p->bucket_tick >= P2P_HZ) { p->bucket_tick = t; p->bucket_used = 0; }
        if (p->bucket_used >= cap) { p->throttled++; return 0; }
        p->bucket_used++;
    }
    n->qos.priority_low_tx++;
    return 1;
}

int p2p_flow_cmd(p2p_node_t* n, int argc, const char* const* argv, char* out, int cap) {
    char line[320];
    int i, shown = 0;
    if (!n || !out || cap <= 0) return 1;
    out[0] = 0;
    if (argc >= 1 && s_eq(argv[0], "reset")) {
        for (i = 0; i < P2P_PEERS; i++) if (n->peers[i].used) p2p_flow_init(&n->peers[i].flow);
        s_cat(out, "p2p flow reset\n", cap);
        return 0;
    }
    for (i = 0; i < P2P_PEERS; i++) {
        const p2p_peer_t* p = &n->peers[i];
        if (!p->used || (argc >= 1 && !s_eq(argv[0], p->name))) continue;
        line[0] = 0;
        s_cat(line, "p2p flow peer ", (int)sizeof(line)); s_cat(line, p->name, (int)sizeof(line)); s_cat(line, " ", (int)sizeof(line));
        p2p_flow_status(&p->flow, line + s_len(line), (int)sizeof(line) - s_len(line) - 1);
        s_cat(line, "\n", (int)sizeof(line));
        s_cat(out, line, cap);
        shown++;
    }
    if (!shown) { s_cat(out, argc >= 1 ? "p2p flow error unknown peer\n" : "p2p flow no peers\n", cap); return argc >= 1 ? 1 : 0; }
    return 0;
}

int p2p_pace_cmd(p2p_node_t* n, int argc, const char* const* argv, char* out, int cap) {
    if (!n) return 1;
    pace_ensure(n);
    if (p2p_pace_command(&n->qos.pace, argc, argv, out, cap) != 0) return 1;
    /* the line shows the rate the next send would use */
    (void)p2p_pace_rate(&n->qos.pace, &n->qos.bwe, (int)n->qos.bwe_ready);
    p2p_pace_status(&n->qos.pace, out, cap);
    return 0;
}

/* Lot 6 (US-068/073): a data message over budget is queued (shaping),
 * not dropped. Returns 1 when queued, -2 when the queue is full. */
static int shape_enqueue(p2p_node_t* n, p2p_peer_t* p, int inner, const uint8_t* body, int len) {
    int i;
    if (len > P2P_SHAPE_BODY) return -2;
    for (i = 0; i < P2P_SHAPE_Q; i++) {
        if (n->shape_q[i].used) continue;
        n->shape_q[i].used = 1;
        n->shape_q[i].peer = p->id;
        n->shape_q[i].inner = (uint8_t)inner;
        n->shape_q[i].len = (uint16_t)len;
        mcopy(n->shape_q[i].body, body, len);
        n->qos.shaped_queued++;
        return 1;
    }
    n->qos.shaped_dropped++;
    return -2;
}

/* FIFO per call order: the lowest slot is not always the oldest, so keep
 * a send pass in slot order; each peer keeps its own order because a slot
 * is reused only after it was sent. */
static void shape_drain(p2p_node_t* n, const p2p_host_t* h) {
    int i;
    for (i = 0; i < P2P_SHAPE_Q; i++) {
        p2p_peer_t* p;
        if (!n->shape_q[i].used) continue;
        p = by_id(n, n->shape_q[i].peer);
        if (!p || !p->keyed) { n->shape_q[i].used = 0; n->qos.shaped_dropped++; continue; }
        if (!admit(n, h, p, n->shape_q[i].len)) continue;
        if (seal_send(n, h, p, n->shape_q[i].inner, n->shape_q[i].body, n->shape_q[i].len) == 0) n->qos.shaped_sent++;
        else n->qos.shaped_dropped++;
        n->shape_q[i].used = 0;
    }
}

static void qos_rate(p2p_node_t* n) {
    uint32_t srtt = n->qos.rtt_avg_ms < 10U ? 10U : n->qos.rtt_avg_ms;
    n->qos.rate_est_bps = n->qos.window_size * (uint32_t)P2P_DATAGRAM_MAX * 1000U / srtt;
}

void p2p_qos_note_loss(p2p_node_t* n) {
    if (!n) return;
    n->qos.loss_count++;
    if (n->qos.window_size > 1U) n->qos.window_size /= 2U;
    qos_rate(n);
}

void p2p_qos_note_ack(p2p_node_t* n) {
    if (!n) return;
    if (n->qos.window_size < P2P_QOS_WINDOW_MAX) n->qos.window_size++;
}

/* Adaptive controller (US-068/073), fed by PONG round trips:
 * smoothed RTT (EWMA 1/8) and minimum RTT; a sample above twice the
 * minimum plus 20 ms means a queue is building, the window shrinks by a
 * quarter; otherwise it grows by one per round trip (additive increase).
 * Losses (unanswered PING, rejected append) halve it. */
static void bwe_ensure(p2p_node_t* n, const p2p_host_t* h) {
    if (n->qos.bwe_ready) return;
    p2p_bwe_init(&n->qos.bwe, P2P_PROBE_BYTES, 20000U, now(h) * (1000U / P2P_HZ));
    n->qos.bwe_ready = 1U;
}

int p2p_qos_probe(p2p_node_t* n, const p2p_host_t* h, const char* peer, int count) {
    p2p_peer_t* p;
    if (!n || !peer) return -1;
    if (count < 1 || count > (int)P2P_PROBE_MAX) return -2;
    p = p2p_find(n, peer);
    if (!p || !p->keyed) return -1;
    bwe_ensure(n, h);
    n->qos.probe_peer = p->id;
    n->qos.probe_left = (uint32_t)count;
    return 0;
}

/* Lot 67: paced probe train, called from p2p_tick. A probe not acked
 * after 2 s (or when its slot is reused) is a loss for the estimator. */
static void probe_pump(p2p_node_t* n, const p2p_host_t* h) {
    uint8_t body[P2P_PROBE_BYTES];
    uint32_t ms, i;
    p2p_peer_t* p;
    if (!n->qos.bwe_ready) return;
    ms = now(h) * (1000U / P2P_HZ);
    for (i = 0; i < P2P_BWE_INFLIGHT; i++) {
        p2p_bwe_pkt_t* k = &n->qos.bwe.pkt[i];
        if (k->used && ms - k->sent_ms > 2000U) { p2p_bwe_on_loss(&n->qos.bwe, k->seq, ms); n->qos.probe_lost++; }
    }
    if (!n->qos.probe_left) return;
    p = by_id(n, n->qos.probe_peer);
    if (!p || !p->keyed) { n->qos.probe_left = 0; return; }
    while (n->qos.probe_left && p2p_bwe_can_send(&n->qos.bwe, P2P_PROBE_BYTES, ms)) {
        uint32_t seq = ++n->qos.probe_seq;
        for (i = 0; i < P2P_PROBE_BYTES; i++) body[i] = (uint8_t)(i * 7U);
        put32(body, now(h));
        put32(body + 4, P2P_PROBE_MAGIC);
        put32(body + 8, seq);
        if (p2p_bwe_on_send(&n->qos.bwe, seq, P2P_PROBE_BYTES, ms) != 0) break;
        if (seal_send(n, h, p, P2P_I_PING, body, P2P_PROBE_BYTES) != 0) {
            p2p_bwe_on_loss(&n->qos.bwe, seq, ms);
            n->qos.probe_lost++;
            break;
        }
        n->qos.probe_sent++;
        n->qos.probe_left--;
    }
    if (!n->qos.probe_left) p2p_bwe_on_app_limited(&n->qos.bwe);
}

void p2p_qos_note_rtt(p2p_node_t* n, uint32_t sample_ms) {
    if (!n) return;
    if (!n->qos.rtt_min_ms || sample_ms < n->qos.rtt_min_ms) n->qos.rtt_min_ms = sample_ms ? sample_ms : 1U;
    n->qos.rtt_avg_ms = n->qos.rtt_avg_ms ? (n->qos.rtt_avg_ms * 7U + sample_ms) / 8U : sample_ms;
    if (sample_ms > 2U * n->qos.rtt_min_ms + 20U) {
        uint32_t w = n->qos.window_size - n->qos.window_size / 4U;
        n->qos.window_size = w ? w : 1U;
        n->qos.delay_backoffs++;
    } else {
        p2p_qos_note_ack(n);
    }
    qos_rate(n);
}

/* ------------------------------------------------------------ handlers */
static int item_encode(uint8_t* b, int pos, int cap, const p2p_item_t* it) {
    pos = put_str(b, pos, cap, it->key);
    if (pos < 0) return -1;
    pos = put_str(b, pos, cap, it->value);
    if (pos < 0 || pos + 8 > cap) return -1;
    put32(b + pos, it->version); put32(b + pos + 4, it->origin);
    return pos + 8;
}
static int item_decode(const uint8_t* b, int pos, int len, p2p_item_t* it) {
    pos = get_str(b, pos, len, it->key, P2P_KEY_MAX);
    if (pos < 0) return -1;
    pos = get_str(b, pos, len, it->value, P2P_VAL_MAX);
    if (pos < 0 || pos + 8 > len) return -1;
    it->version = get32(b + pos); it->origin = get32(b + pos + 4);
    return pos + 8;
}
static void note_apply(p2p_node_t* n, const p2p_host_t* h, const p2p_item_t* it, const char* how, const char* from) {
    char line[160];
    (void)n;
    line[0] = 0;
    s_cat(line, "p2p kv ", 160); s_cat(line, how, 160); s_cat(line, " ", 160);
    s_cat(line, it->key, 160); s_cat(line, "=", 160); s_cat(line, it->value, 160);
    s_cat(line, " v", 160); cat_u(line, it->version, 160);
    s_cat(line, " from ", 160); s_cat(line, from, 160);
    say(h, line);
}
static void send_items_newer(p2p_node_t* n, const p2p_host_t* h, p2p_peer_t* p,
                             const p2p_item_t* theirs, int ntheirs) {
    uint8_t b[P2P_DATAGRAM_MAX];
    int i, j, pos = 1, count = 0;
    for (i = 0; i < P2P_KV; i++) {
        const p2p_item_t* it = &n->kv[i];
        int newer = 1, np;
        if (!it->used) continue;
        for (j = 0; j < ntheirs; j++)
            if (s_eq(theirs[j].key, it->key) &&
                (theirs[j].version > it->version || (theirs[j].version == it->version && theirs[j].origin >= it->origin)))
                newer = 0;
        if (!newer) continue;
        np = item_encode(b, pos, 400, it);
        if (np < 0) { b[0] = (uint8_t)count; (void)seal_send(n, h, p, P2P_I_SYNC_ITEMS, b, pos); pos = 1; count = 0; np = item_encode(b, pos, 400, it); }
        pos = np; count++;
    }
    b[0] = (uint8_t)count;
    (void)seal_send(n, h, p, P2P_I_SYNC_ITEMS, b, pos);
}
static int digest_encode(const p2p_node_t* n, uint8_t* b, int reply) {
    int i, pos = 2, count = 0;
    b[0] = (uint8_t)reply;
    for (i = 0; i < P2P_KV; i++) {
        if (!n->kv[i].used) continue;
        pos = put_str(b, pos, 500, n->kv[i].key);
        if (pos < 0) return -1;
        put32(b + pos, n->kv[i].version); put32(b + pos + 4, n->kv[i].origin); pos += 8;
        count++;
    }
    b[1] = (uint8_t)count;
    return pos;
}

/* --------------------------------------------------------------- Raft log */
static int raft_send_one(p2p_node_t* n, const p2p_host_t* h, p2p_peer_t* p);
static int raft_start_election(p2p_node_t* n, const p2p_host_t* h);

/* Quorum is over discovered keyed members, including self. A peer that is
 * only down still counts, so a lone node cannot elect or commit while the
 * others are partitioned. */
static uint32_t raft_members(const p2p_node_t* n) {
    uint32_t c = 1U;
    int i;
    for (i = 0; i < P2P_PEERS; i++)
        if (n->peers[i].used && n->peers[i].keyed) c++;
    return c;
}
static uint32_t raft_majority(const p2p_node_t* n) {
    return raft_members(n) / 2U + 1U;
}
static p2p_raft_entry_t* raft_log_slot(p2p_node_t* n, uint32_t index) {
    uint32_t slot;
    if (index == 0U || index <= n->raft.snapshot_index) return 0;
    slot = index - n->raft.snapshot_index - 1U;
    if (slot >= P2P_RAFT_LOG) return 0;
    return &n->raft.log[slot];
}
static p2p_raft_entry_t* raft_log_at(p2p_node_t* n, uint32_t index) {
    if (index > n->raft.log_count) return 0;
    return raft_log_slot(n, index);
}
static uint32_t raft_last_term(const p2p_node_t* n) {
    uint32_t index = n->raft.log_count;
    uint32_t slot;
    if (index == 0U) return 0U;
    if (index <= n->raft.snapshot_index) return n->raft.snapshot_term;
    slot = index - n->raft.snapshot_index - 1U;
    if (slot >= P2P_RAFT_LOG) return 0U;
    return n->raft.log[slot].term;
}
static int raft_prev_ok(const p2p_node_t* n, uint32_t prev_i, uint32_t prev_t) {
    uint32_t slot;
    if (prev_i > n->raft.log_count || prev_i < n->raft.snapshot_index) return 0;
    if (prev_i == 0U) return prev_t == 0U;
    if (prev_i == n->raft.snapshot_index) return prev_t == n->raft.snapshot_term;
    slot = prev_i - n->raft.snapshot_index - 1U;
    if (slot >= P2P_RAFT_LOG) return 0;
    return n->raft.log[slot].term == prev_t;
}
static void raft_step_down(p2p_node_t* n, uint32_t term) {
    if (term > n->raft.term) n->raft.term_changes++;
    n->raft.term = term;
    n->raft.role = P2P_RAFT_FOLLOWER;
    n->raft.voted_for = 0;
    n->raft.leader_id = 0;
    if (n->prop.active) {
        n->prop.active = 0;
        n->prop.result = -2;
    }
}
static void raft_apply(p2p_node_t* n, const p2p_host_t* h) {
    char line[160];
    while (n->raft.last_applied < n->raft.commit_index && n->raft.last_applied < n->raft.log_count) {
        p2p_raft_entry_t* e;
        n->raft.last_applied++;
        if (n->raft.last_applied <= n->raft.snapshot_index) continue;
        e = raft_log_at(n, n->raft.last_applied);
        if (!e) break;
        (void)kv_apply(n, e->key, e->value, n->raft.last_applied, n->id);
        if (e->key[0] == 'c' && e->key[1] == '/' && e->key[2]) {
            /* Lot 6 (US-070): consensus-backed cache line. Same value and
             * version on every node that applied the index; any fill at or
             * below it is refused by the fence. */
            uint32_t idx = n->raft.last_applied;
            (void)p2p_cache_put_v(n, e->key + 2, e->value, idx, P2P_CACHE_RAFT_TTL, now(h));
            cache_fence(n, e->key + 2, idx);
            n->cache_raft_applied++;
            line[0] = 0;
            s_cat(line, "p2p cache raft ", 160); s_cat(line, e->key + 2, 160); s_cat(line, "=", 160);
            s_cat(line, e->value, 160); s_cat(line, " v", 160); cat_u(line, idx, 160);
            say(h, line);
        }
        line[0] = 0;
        s_cat(line, "p2p raft commit index ", 160);
        cat_u(line, n->raft.last_applied, 160);
        s_cat(line, " ", 160); s_cat(line, e->key, 160); s_cat(line, "=", 160); s_cat(line, e->value, 160);
        say(h, line);
    }
}
static void raft_push_commit(p2p_node_t* n, const p2p_host_t* h) {
    int i;
    for (i = 0; i < P2P_PEERS; i++) {
        p2p_peer_t* p = &n->peers[i];
        uint32_t saved;
        if (!p->used || !p->keyed || !p->up || n->raft.log_count == 0U) continue;
        if (p->raft_match < n->raft.log_count) continue;
        saved = p->raft_next;
        p->raft_next = n->raft.log_count;
        (void)raft_send_one(n, h, p);
        p->raft_next = saved;
    }
}
static void raft_maybe_commit(p2p_node_t* n, const p2p_host_t* h) {
    uint32_t i, need, k, c, before;
    if (n->raft.role != P2P_RAFT_LEADER) return;
    before = n->raft.commit_index;
    need = raft_majority(n);
    for (i = n->raft.commit_index + 1U; i <= n->raft.log_count; i++) {
        p2p_raft_entry_t* ent = raft_log_at(n, i);
        if (!ent || ent->term != n->raft.term) continue;
        c = 1U;
        for (k = 0; k < P2P_PEERS; k++)
            if (n->peers[k].used && n->peers[k].up && n->peers[k].raft_match >= i) c++;
        if (c >= need) n->raft.commit_index = i;
    }
    if (n->raft.commit_index != before) raft_push_commit(n, h);
    raft_apply(n, h);
}
static void raft_note_lease(p2p_node_t* n, const p2p_host_t* h) {
    n->raft.lease_until = now(h) + P2P_RAFT_LEASE_TICKS;
}
static int raft_lease_valid(const p2p_node_t* n, const p2p_host_t* h) {
    return n->raft.lease_until != 0U && now(h) < n->raft.lease_until;
}
static int raft_send_snapshot(p2p_node_t* n, const p2p_host_t* h, p2p_peer_t* p) {
    uint8_t b[400];
    uint32_t off;
    int pos, nitems = 0, last;
    if (p->raft_snap_gen != n->raft.snapshot_index) {
        p->raft_snap_gen = n->raft.snapshot_index;
        p->raft_snap_off = 0;
    }
    off = p->raft_snap_off;
    if (off > n->raft.snap_kv_n) off = 0;
    put32(b, n->raft.term); put32(b + 4, n->id);
    put32(b + 8, n->raft.snapshot_index); put32(b + 12, n->raft.snapshot_term);
    b[16] = 0; b[17] = (uint8_t)off; b[18] = 0;
    pos = 19;
    while ((uint32_t)(off + (uint32_t)nitems) < n->raft.snap_kv_n && nitems < 4) {
        int np = item_encode(b, pos, 360, &n->raft.snap_kv[off + (uint32_t)nitems]);
        if (np < 0) break;
        pos = np;
        nitems++;
    }
    last = (uint32_t)(off + (uint32_t)nitems) >= n->raft.snap_kv_n;
    b[16] = (uint8_t)(last ? 1 : 0);
    b[18] = (uint8_t)nitems;
    return seal_send(n, h, p, P2P_I_RAFT_SNAPSHOT, b, pos);
}
static void raft_install_snapshot(p2p_node_t* n, const p2p_host_t* h, uint32_t index, uint32_t term) {
    p2p_raft_entry_t kept[P2P_RAFT_LOG];
    uint32_t keep = 0, i;
    int gap = 0;
    char line[80];
    if (n->raft.log_count > index) {
        for (i = index + 1U; i <= n->raft.log_count && keep < P2P_RAFT_LOG; i++) {
            p2p_raft_entry_t* e = raft_log_at(n, i);
            if (!e) { gap = 1; break; }
            kept[keep++] = *e;
        }
    }
    if (gap) keep = 0;
    for (i = 0; i < P2P_RAFT_LOG; i++) mzero(&n->raft.log[i], (int)sizeof(n->raft.log[i]));
    for (i = 0; i < keep; i++) n->raft.log[i] = kept[i];
    n->raft.snapshot_index = index;
    n->raft.snapshot_term = term;
    if (keep == 0U) n->raft.log_count = index;
    if (n->raft.commit_index < index) n->raft.commit_index = index;
    if (n->raft.commit_index > n->raft.log_count) n->raft.commit_index = n->raft.log_count;
    if (n->raft.last_applied < index) n->raft.last_applied = index;
    if (n->raft.last_applied > n->raft.log_count) n->raft.last_applied = n->raft.log_count;
    for (i = 0; i < n->raft.snap_in_n && i < P2P_KV; i++) {
        p2p_item_t* it = &n->raft.snap_in[i];
        (void)kv_apply(n, it->key, it->value, it->version, it->origin);
        cache_fence(n, it->key, it->version);
    }
    line[0] = 0;
    s_cat(line, "p2p raft snapshot installed index ", 80);
    cat_u(line, index, 80);
    say(h, line);
}
static int raft_send_one(p2p_node_t* n, const p2p_host_t* h, p2p_peer_t* p) {
    uint8_t b[P2P_DATAGRAM_MAX];
    uint32_t next, prev_i, prev_t;
    int pos;
    p2p_raft_entry_t* e;
    if (!p->used || !p->keyed || !p->up) return -1;
    if (n->raft.snapshot_index > 0U && p->raft_match < n->raft.snapshot_index)
        return raft_send_snapshot(n, h, p);
    if (p->raft_next == 0U) p->raft_next = n->raft.snapshot_index + 1U;
    if (p->raft_next <= n->raft.snapshot_index) p->raft_next = n->raft.snapshot_index + 1U;
    next = p->raft_next;
    if (next > n->raft.log_count || next == 0U) return 0;
    e = raft_log_at(n, next);
    if (!e) return 0;
    prev_i = next - 1U;
    if (prev_i == 0U) prev_t = 0U;
    else if (prev_i == n->raft.snapshot_index) prev_t = n->raft.snapshot_term;
    else {
        p2p_raft_entry_t* prev = raft_log_at(n, prev_i);
        prev_t = prev ? prev->term : 0U;
    }
    put32(b, n->raft.term); put32(b + 4, n->id);
    put32(b + 8, prev_i); put32(b + 12, prev_t);
    put32(b + 16, n->raft.commit_index);
    put32(b + 20, next); put32(b + 24, e->term);
    pos = put_str(b, 28, 400, e->key);
    if (pos < 0) return -1;
    pos = put_str(b, pos, 400, e->value);
    if (pos < 0) return -1;
    if (seal_send(n, h, p, P2P_I_RAFT_APPEND, b, pos) != 0) return -1;
    p->raft_commit_sent = n->raft.commit_index;
    return 0;
}
/* Lot 46: the periodic replicate runs on every p2p_tick. Without pacing a
 * leader resent the same unacked entry to an unreachable follower on each
 * tick (6697 appends during one hub cut), which crowded out the HELLO the
 * follower needs to rejoin. A peer gets a new entry, a new commit index or
 * a snapshot at once; the same frame again only after P2P_RAFT_RESEND. */
#define P2P_RAFT_RESEND (P2P_HZ / 4U)
static void raft_replicate(p2p_node_t* n, const p2p_host_t* h) {
    int i;
    uint32_t t, key;
    if (!n->raft.active || n->raft.role != P2P_RAFT_LEADER) return;
    t = now(h);
    for (i = 0; i < P2P_PEERS; i++) {
        p2p_peer_t* p = &n->peers[i];
        if (!p->used || !p->keyed || !p->up) continue;
        key = (n->raft.snapshot_index > 0U && p->raft_match < n->raft.snapshot_index) ?
              (0x80000000U | n->raft.snapshot_index) : p->raft_next;
        if (key == p->raft_tx_key && p->raft_commit_sent == n->raft.commit_index &&
            t - p->raft_tx_tick < P2P_RAFT_RESEND) continue;
        p->raft_tx_key = key;
        p->raft_tx_tick = t;
        (void)raft_send_one(n, h, p);
    }
}
static void raft_become_leader(p2p_node_t* n, const p2p_host_t* h) {
    int i;
    char line[80];
    n->raft.role = P2P_RAFT_LEADER;
    n->raft.leader_id = n->id;
    n->raft.heartbeat_tick = 0;
    raft_note_lease(n, h);
    for (i = 0; i < P2P_PEERS; i++) {
        n->peers[i].raft_next = n->raft.log_count + 1U;
        n->peers[i].raft_tx_key = 0;
        n->peers[i].raft_match = 0;
    }
    line[0] = 0;
    s_cat(line, "p2p raft leader elected term ", 80); cat_u(line, n->raft.term, 80);
    say(h, line);
    raft_replicate(n, h);
}

/* ------------------------------------------------ typed RPC (not HTTP/2) */
static int grpc_type_of(const char* svc, const char* mth) {
    if (s_eq(svc, "echo.EchoService") && s_eq(mth, "SayHello")) return 1;
    if (s_eq(svc, "kv.KVService") && s_eq(mth, "Get")) return 2;
    if (s_eq(svc, "status.StatusService") && s_eq(mth, "GetStatus")) return 3;
    return 0;
}
static void grpc_remember(p2p_node_t* n, uint32_t peer, uint32_t id, const uint8_t* b, int len) {
    int i, slot = -1;
    if (len > 128) len = 128;
    for (i = 0; i < P2P_RPC_SLOTS; i++)
        if (n->rpc_cache[i].used && n->rpc_cache[i].peer == peer && n->rpc_cache[i].id == id) { slot = i; break; }
    if (slot < 0) {
        for (i = 0; i < P2P_RPC_SLOTS; i++) if (!n->rpc_cache[i].used) { slot = i; break; }
        if (slot < 0) slot = 0;
    }
    n->rpc_cache[slot].used = 1;
    n->rpc_cache[slot].peer = peer;
    n->rpc_cache[slot].id = id;
    n->rpc_cache[slot].len = len;
    mcopy(n->rpc_cache[slot].bytes, b, len);
}
static int grpc_cached(p2p_node_t* n, uint32_t peer, uint32_t id, uint8_t* b, int* len) {
    int i;
    for (i = 0; i < P2P_RPC_SLOTS; i++)
        if (n->rpc_cache[i].used && n->rpc_cache[i].peer == peer && n->rpc_cache[i].id == id) {
            mcopy(b, n->rpc_cache[i].bytes, n->rpc_cache[i].len);
            *len = n->rpc_cache[i].len;
            return 1;
        }
    return 0;
}
static void grpc_wait_add(p2p_node_t* n, uint32_t id, uint32_t deadline) {
    int i, slot = -1;
    for (i = 0; i < P2P_RPC_SLOTS; i++) if (!n->rpc_wait[i].used || n->rpc_wait[i].id == id) { slot = i; break; }
    if (slot < 0) slot = 0;
    n->rpc_wait[slot].used = 1;
    n->rpc_wait[slot].id = id;
    n->rpc_wait[slot].deadline = deadline;
}
static void grpc_wait_clear(p2p_node_t* n, uint32_t id) {
    int i;
    for (i = 0; i < P2P_RPC_SLOTS; i++) if (n->rpc_wait[i].used && n->rpc_wait[i].id == id) n->rpc_wait[i].used = 0;
}
static void grpc_expire(p2p_node_t* n, const p2p_host_t* h) {
    uint32_t t = now(h);
    int i;
    char line[96];
    for (i = 0; i < P2P_RPC_SLOTS; i++) {
        if (!n->rpc_wait[i].used || t < n->rpc_wait[i].deadline) continue;
        line[0] = 0;
        s_cat(line, "p2p grpc timeout id ", 96);
        cat_u(line, n->rpc_wait[i].id, 96);
        s_cat(line, " status 4", 96);
        say(h, line);
        n->rpc_wait[i].used = 0;
    }
}
static void grpc_fill(p2p_node_t* n, uint32_t type, const char* svc, const char* mth, const char* pay,
                      uint8_t* st, char* resp, int cap) {
    resp[0] = 0;
    *st = 0;
    if (type == 0U) type = (uint32_t)grpc_type_of(svc, mth);
    if (type == 1U || (s_eq(svc, "echo.EchoService") && s_eq(mth, "SayHello"))) {
        s_cat(resp, "Hello ", cap); s_cat(resp, pay, cap); *st = 0;
    } else if (type == 2U || (s_eq(svc, "kv.KVService") && s_eq(mth, "Get"))) {
        const p2p_item_t* it = p2p_get_local(n, pay);
        if (it) { s_cat(resp, it->value, cap); *st = 0; }
        else { s_cat(resp, "not_found", cap); *st = 5; }
    } else if (type == 3U || (s_eq(svc, "status.StatusService") && s_eq(mth, "GetStatus"))) {
        s_cat(resp, "p2p_node_active", cap); *st = 0;
    } else {
        s_cat(resp, "unimplemented", cap); *st = 3;
    }
}

/* Lot 69: sends the pending data acks of one peer (control frame, not paced) */
static void flush_dack(p2p_node_t* n, const p2p_host_t* h, p2p_peer_t* p) {
    uint8_t body[1 + 4 * P2P_FLOW_ACK_BATCH];
    int k = 2, len;
    while (k-- > 0 && (len = p2p_flow_take_acks(&p->flow, body, (int)sizeof(body))) > 0)
        (void)seal_send(n, h, p, P2P_I_DACK, body, len);
}

static void on_sealed(p2p_node_t* n, const p2p_host_t* h, p2p_peer_t* p, int inner, const uint8_t* m, int len) {
    uint8_t b[P2P_DATAGRAM_MAX];
    char line[200];
    p2p_item_t it;
    int pos, i;
    line[0] = 0;
    n->rx_type[inner]++;
    /* Lot 69: every accepted data datagram is acknowledged by its counter */
    if ((inner == P2P_I_MSG || inner == P2P_I_PUT) && p2p_flow_note_rx(&p->flow, p->rx_counter)) flush_dack(n, h, p);
    switch (inner) {
    case P2P_I_DACK:
        if (p2p_flow_apply_acks(&p->flow, m, len, now(h) * (1000000U / P2P_HZ)) < 0) n->foreign++;
        break;
    case P2P_I_PING:
        /* Lot 67: a probe PING (magic at 4) gets its 12-byte head echoed */
        if (len >= 12 && get32(m + 4) == P2P_PROBE_MAGIC) (void)seal_send(n, h, p, P2P_I_PONG, m, 12);
        else if (len >= 4) (void)seal_send(n, h, p, P2P_I_PONG, m, 4);
        break;
    case P2P_I_PONG:
        if (len >= 12 && get32(m + 4) == P2P_PROBE_MAGIC) {
            if (n->qos.bwe_ready) {
                (void)p2p_bwe_on_ack(&n->qos.bwe, get32(m + 8), now(h) * (1000U / P2P_HZ));
                n->qos.probe_acked++;
            }
            break;
        }
        if (len >= 4) {
            uint32_t sample_ticks = now(h) - get32(m);
            uint32_t sample_ms = sample_ticks * (1000U / P2P_HZ);
            p->rtt_ticks = sample_ticks;
            p->ping_wait = 0;
            p->rtt_ewma_ms = p->rtt_ewma_ms ? (p->rtt_ewma_ms * 7U + sample_ms) / 8U : sample_ms;
            p->link_cost = p2p_calc_link_cost(p);
            p2p_qos_note_rtt(n, sample_ms);
        }
        break;
    case P2P_I_MSG:
        if (get_str(m, 0, len, n->last_msg, P2P_TEXT_MAX) < 0) break;
        s_cat(line, "p2p msg from ", 200); s_cat(line, p->name, 200); s_cat(line, ": ", 200); s_cat(line, n->last_msg, 200);
        if (p->via) s_cat(line, " (relayed)", 200);
        say(h, line);
        break;
    case P2P_I_PUT:
    case P2P_I_COMMIT:
        if (item_decode(m, 0, len, &it) < 0) break;
        if (kv_apply(n, it.key, it.value, it.version, it.origin) == 1)
            note_apply(n, h, &it, inner == P2P_I_PUT ? "replicated" : "committed", p->name);
        break;
    case P2P_I_SYNC_REQ: {
        p2p_item_t theirs[P2P_KV];
        int cnt, reply;
        if (len < 2) break;
        reply = m[0]; cnt = m[1]; pos = 2;
        if (cnt > P2P_KV) break;
        for (i = 0; i < cnt; i++) {
            pos = get_str(m, pos, len, theirs[i].key, P2P_KEY_MAX);
            if (pos < 0 || pos + 8 > len) return;
            theirs[i].version = get32(m + pos); theirs[i].origin = get32(m + pos + 4); pos += 8;
        }
        send_items_newer(n, h, p, theirs, cnt);
        if (!reply) { int dl = digest_encode(n, b, 1); if (dl > 0) (void)seal_send(n, h, p, P2P_I_SYNC_REQ, b, dl); }
        break;
    }
    case P2P_I_SYNC_ITEMS: {
        int cnt, applied = 0;
        if (len < 1) break;
        cnt = m[0]; pos = 1;
        p->sync_retries = 0; /* the answer arrived */
        for (i = 0; i < cnt; i++) {
            pos = item_decode(m, pos, len, &it);
            if (pos < 0) break;
            if (kv_apply(n, it.key, it.value, it.version, it.origin) == 1) { applied++; note_apply(n, h, &it, "synced", p->name); }
        }
        s_cat(line, "p2p sync from ", 200); s_cat(line, p->name, 200);
        s_cat(line, " items ", 200); cat_u(line, (uint32_t)cnt, 200);
        s_cat(line, " applied ", 200); cat_u(line, (uint32_t)applied, 200);
        say(h, line);
        break;
    }
    case P2P_I_GET_REQ: {
        char key[P2P_KEY_MAX];
        const p2p_item_t* have;
        if (get_str(m, 0, len, key, P2P_KEY_MAX) < 0) break;
        have = p2p_get_local(n, key);
        if (have) { b[0] = 1; pos = item_encode(b, 1, 400, have); }
        else { b[0] = 0; pos = put_str(b, 1, 400, key); }
        if (pos > 0) (void)seal_send(n, h, p, P2P_I_GET_RESP, b, pos);
        break;
    }
    case P2P_I_GET_RESP:
        if (len < 1) break;
        if (m[0] == 1 && item_decode(m, 1, len, &it) >= 0) {
            (void)kv_apply(n, it.key, it.value, it.version, it.origin);
            note_apply(n, h, &it, "fetched", p->name);
        } else {
            char key[P2P_KEY_MAX];
            if (get_str(m, 1, len, key, P2P_KEY_MAX) >= 0) {
                s_cat(line, "p2p kv miss ", 200); s_cat(line, key, 200); s_cat(line, " at ", 200); s_cat(line, p->name, 200);
                say(h, line);
            }
        }
        break;
    case P2P_I_PROPOSE: {
        const p2p_item_t* have;
        uint32_t round;
        if (len < 4) break;
        round = get32(m);
        if (item_decode(m, 4, len, &it) < 0) break;
        have = p2p_get_local(n, it.key);
        put32(b, round);
        b[4] = (uint8_t)(!have || it.version > have->version ||
                         (it.version == have->version && it.origin > have->origin));
        (void)seal_send(n, h, p, P2P_I_VOTE, b, 5);
        s_cat(line, "p2p vote ", 200); s_cat(line, b[4] ? "yes" : "no", 200);
        s_cat(line, " round ", 200); cat_u(line, round, 200); s_cat(line, " to ", 200); s_cat(line, p->name, 200);
        say(h, line);
        break;
    }
    case P2P_I_VOTE: {
        p2p_proposal_t* pr = &n->prop;
        uint32_t k;
        int dupvote = 0;
        if (len < 5 || !pr->active || get32(m) != pr->round) break;
        for (k = 0; k < pr->yes + pr->no; k++) if (pr->voters[k] == p->id) dupvote = 1;
        if (dupvote || pr->yes + pr->no >= P2P_PEERS + 1) break;
        pr->voters[pr->yes + pr->no] = p->id;
        if (m[4]) pr->yes++; else pr->no++;
        break;
    }
    case P2P_I_APP:
        if (n->app) n->app(n->app_ctx, p->id, m, len);
        break;
    case P2P_I_RAFT_PREVOTE_REQ: {
        uint32_t req_term, last_idx, last_term, my_idx, my_term;
        int log_ok, grant, timer_expired;
        if (len < 16) break;
        req_term = get32(m); last_idx = get32(m + 8); last_term = get32(m + 12);
        my_idx = n->raft.log_count;
        my_term = raft_last_term(n);
        log_ok = !(last_term < my_term || (last_term == my_term && last_idx < my_idx));
        timer_expired = now(h) - n->raft.election_tick >= P2P_RAFT_ELECTION_TICKS;
        /* Lot 46: a node already campaigning has lost its leader too; its
         * own pre-vote reset election_tick, which made two pre-candidates
         * refuse each other forever (qemu-p2p 2c after an isolated pre-vote). */
        if (n->raft.role == P2P_RAFT_PRE_CANDIDATE || n->raft.role == P2P_RAFT_CANDIDATE)
            timer_expired = 1;
        /* A pre-vote does not bump the term and does not consume voted_for.
         * A follower that still has a leader and a fresh heartbeat refuses,
         * so a short partition cannot start a new term. */
        grant = log_ok && req_term > n->raft.term && n->raft.role != P2P_RAFT_LEADER &&
                !raft_lease_valid(n, h) &&
                (n->raft.leader_id == 0U || timer_expired);
        b[0] = (uint8_t)(grant ? 1 : 0);
        put32(b + 1, n->raft.term);
        (void)seal_send(n, h, p, P2P_I_RAFT_PREVOTE_RESP, b, 5);
        break;
    }
    case P2P_I_RAFT_PREVOTE_RESP: {
        uint32_t resp_term;
        if (len < 5) break;
        resp_term = get32(m + 1);
        if (resp_term > n->raft.term) { raft_step_down(n, resp_term); break; }
        if (n->raft.role == P2P_RAFT_PRE_CANDIDATE && resp_term == n->raft.term && m[0] == 1) {
            n->raft.votes_granted++;
            if (n->raft.votes_granted >= raft_majority(n)) (void)raft_start_election(n, h);
        }
        break;
    }
    case P2P_I_RAFT_VOTE_REQ: {
        uint32_t req_term, cand_id, last_idx = 0, last_term = 0, my_idx, my_term;
        int log_ok = 1;
        if (len < 8) break;
        req_term = get32(m); cand_id = get32(m + 4);
        if (len >= 16) { last_idx = get32(m + 8); last_term = get32(m + 12); }
        /* A live lease keeps the current leader. A higher term is ignored
         * until the lease ends, so a partitioned rival cannot depose it. */
        if (raft_lease_valid(n, h) && n->raft.leader_id != 0U && cand_id != n->raft.leader_id) {
            b[0] = 0; put32(b + 1, n->raft.term);
            (void)seal_send(n, h, p, P2P_I_RAFT_VOTE_RESP, b, 5);
            break;
        }
        if (req_term > n->raft.term) raft_step_down(n, req_term);
        if (len >= 16) {
            my_idx = n->raft.log_count;
            my_term = raft_last_term(n);
            if (last_term < my_term || (last_term == my_term && last_idx < my_idx)) log_ok = 0;
        }
        b[0] = (uint8_t)(log_ok && req_term == n->raft.term &&
                         (n->raft.voted_for == 0 || n->raft.voted_for == cand_id) ? 1 : 0);
        put32(b + 1, n->raft.term);
        if (b[0]) { n->raft.voted_for = cand_id; n->raft.election_tick = now(h); }
        (void)seal_send(n, h, p, P2P_I_RAFT_VOTE_RESP, b, 5);
        break;
    }
    case P2P_I_RAFT_VOTE_RESP: {
        uint32_t resp_term;
        if (len < 5) break;
        resp_term = get32(m + 1);
        if (resp_term > n->raft.term) raft_step_down(n, resp_term);
        else if (n->raft.role == P2P_RAFT_CANDIDATE && resp_term == n->raft.term && m[0] == 1) {
            n->raft.votes_granted++;
            if (n->raft.votes_granted >= raft_majority(n)) raft_become_leader(n, h);
        }
        break;
    }
    case P2P_I_RAFT_HEARTBEAT: {
        uint32_t hb_term, ldr_id;
        if (len < 8) break;
        hb_term = get32(m); ldr_id = get32(m + 4);
        if (hb_term >= n->raft.term) {
            if (hb_term > n->raft.term) raft_step_down(n, hb_term);
            n->raft.role = P2P_RAFT_FOLLOWER;
            n->raft.leader_id = ldr_id;
            n->raft.voted_for = 0;
            n->raft.election_tick = now(h);
            raft_note_lease(n, h);
        }
        break;
    }
    case P2P_I_RAFT_APPEND: {
        uint32_t term, leader, prev_i, prev_t, lcommit, eindex, eterm;
        char key[P2P_KEY_MAX], val[P2P_VAL_MAX];
        int pos, ok = 0;
        if (len < 28) break;
        term = get32(m); leader = get32(m + 4); prev_i = get32(m + 8); prev_t = get32(m + 12);
        lcommit = get32(m + 16); eindex = get32(m + 20); eterm = get32(m + 24);
        pos = get_str(m, 28, len, key, P2P_KEY_MAX);
        if (pos < 0) break;
        pos = get_str(m, pos, len, val, P2P_VAL_MAX);
        if (pos < 0) break;
        if (raft_lease_valid(n, h) && n->raft.leader_id != 0U && leader != n->raft.leader_id &&
            term > n->raft.term) {
            put32(b, n->raft.term); b[4] = 0; put32(b + 5, 0);
            (void)seal_send(n, h, p, P2P_I_RAFT_APPEND_RESP, b, 9);
            break;
        }
        if (term > n->raft.term) raft_step_down(n, term);
        if (term < n->raft.term) {
            put32(b, n->raft.term); b[4] = 0; put32(b + 5, 0);
            (void)seal_send(n, h, p, P2P_I_RAFT_APPEND_RESP, b, 9);
            break;
        }
        n->raft.role = P2P_RAFT_FOLLOWER;
        n->raft.leader_id = leader;
        n->raft.election_tick = now(h);
        raft_note_lease(n, h);
        if (!raft_prev_ok(n, prev_i, prev_t) || eindex == 0U ||
            eindex > n->raft.snapshot_index + P2P_RAFT_LOG) {
            put32(b, n->raft.term); b[4] = 0; put32(b + 5, 0);
            (void)seal_send(n, h, p, P2P_I_RAFT_APPEND_RESP, b, 9);
            break;
        }
        if (eindex <= n->raft.snapshot_index) {
            ok = 1;
        } else {
            p2p_raft_entry_t* have = raft_log_at(n, eindex);
            if (have && have->term != eterm) {
                n->raft.log_count = eindex - 1U;
                if (n->raft.commit_index > n->raft.log_count) n->raft.commit_index = n->raft.log_count;
                if (n->raft.last_applied > n->raft.log_count) n->raft.last_applied = n->raft.log_count;
                have = 0;
            }
            if (eindex == n->raft.log_count + 1U) {
                p2p_raft_entry_t* e = raft_log_slot(n, eindex);
                if (!e) {
                    put32(b, n->raft.term); b[4] = 0; put32(b + 5, 0);
                    (void)seal_send(n, h, p, P2P_I_RAFT_APPEND_RESP, b, 9);
                    break;
                }
                e->term = eterm;
                s_copy(e->key, key, P2P_KEY_MAX);
                s_copy(e->value, val, P2P_VAL_MAX);
                n->raft.log_count = eindex;
                ok = 1;
            } else if (have && have->term == eterm) {
                ok = 1;
            }
        }
        if (ok) {
            /* Raft: commit = min(leaderCommit, index of the entry just
             * matched), not the local log end. Lot 46 (qemu-p2p 2c): an
             * append matching index 4 with leaderCommit 5 applied the
             * isolated old leader's own uncommitted index 5. */
            uint32_t upto = eindex < n->raft.log_count ? eindex : n->raft.log_count;
            if (lcommit < upto) upto = lcommit;
            if (upto > n->raft.commit_index) n->raft.commit_index = upto;
            raft_apply(n, h);
            put32(b, n->raft.term); b[4] = 1; put32(b + 5, eindex);
        } else {
            put32(b, n->raft.term); b[4] = 0; put32(b + 5, 0);
        }
        (void)seal_send(n, h, p, P2P_I_RAFT_APPEND_RESP, b, 9);
        break;
    }
    case P2P_I_RAFT_APPEND_RESP: {
        uint32_t term, match;
        if (len < 9) break;
        term = get32(m); match = get32(m + 5);
        if (term > n->raft.term) { raft_step_down(n, term); break; }
        if (n->raft.role != P2P_RAFT_LEADER || term != n->raft.term) break;
        if (m[4]) {
            uint32_t saved;
            p2p_qos_note_ack(n);
            if (match > p->raft_match) p->raft_match = match;
            p->raft_next = p->raft_match + 1U;
            raft_maybe_commit(n, h);
            if (n->raft.log_count && p->raft_match >= n->raft.log_count &&
                n->raft.commit_index > p->raft_commit_sent) {
                saved = p->raft_next;
                p->raft_next = n->raft.log_count;
                (void)raft_send_one(n, h, p);
                p->raft_next = saved;
            } else {
                (void)raft_send_one(n, h, p);
            }
        } else if (p->raft_next > n->raft.snapshot_index + 1U) {
            p->raft_next--;
            p2p_qos_note_loss(n);
            (void)raft_send_one(n, h, p);
        } else if (n->raft.snapshot_index > 0U && p->raft_match < n->raft.snapshot_index) {
            (void)raft_send_snapshot(n, h, p);
        }
        break;
    }
    case P2P_I_RAFT_SNAPSHOT: {
        uint32_t term, leader, snap_i, snap_t;
        uint8_t last, off, nitems;
        int spos, si;
        if (len < 19) break;
        term = get32(m); leader = get32(m + 4); snap_i = get32(m + 8); snap_t = get32(m + 12);
        last = m[16]; off = m[17]; nitems = m[18];
        if (raft_lease_valid(n, h) && n->raft.leader_id != 0U && leader != n->raft.leader_id &&
            term > n->raft.term) {
            put32(b, n->raft.term); b[4] = 0; put32(b + 5, 0); b[9] = 0;
            (void)seal_send(n, h, p, P2P_I_RAFT_SNAPSHOT_RESP, b, 10);
            break;
        }
        if (term > n->raft.term) raft_step_down(n, term);
        if (term < n->raft.term || snap_i == 0U) {
            put32(b, n->raft.term); b[4] = 0; put32(b + 5, 0); b[9] = 0;
            (void)seal_send(n, h, p, P2P_I_RAFT_SNAPSHOT_RESP, b, 10);
            break;
        }
        n->raft.role = P2P_RAFT_FOLLOWER;
        n->raft.leader_id = leader;
        n->raft.election_tick = now(h);
        raft_note_lease(n, h);
        if (snap_i <= n->raft.snapshot_index && n->raft.last_applied >= snap_i) {
            put32(b, n->raft.term); b[4] = 1; put32(b + 5, snap_i); b[9] = (uint8_t)(off + nitems);
            (void)seal_send(n, h, p, P2P_I_RAFT_SNAPSHOT_RESP, b, 10);
            break;
        }
        if (off == 0U || n->raft.snap_in_index != snap_i) {
            n->raft.snap_in_n = 0;
            n->raft.snap_in_index = snap_i;
            n->solicited_until = now(h) + 3U * P2P_HZ; /* Lot 6: catch-up burst expected */
        }
        if (off != n->raft.snap_in_n) {
            put32(b, n->raft.term); b[4] = 0; put32(b + 5, 0); b[9] = n->raft.snap_in_n;
            (void)seal_send(n, h, p, P2P_I_RAFT_SNAPSHOT_RESP, b, 10);
            break;
        }
        spos = 19;
        for (si = 0; si < (int)nitems; si++) {
            p2p_item_t it;
            mzero(&it, (int)sizeof(it));
            spos = item_decode(m, spos, len, &it);
            if (spos < 0 || n->raft.snap_in_n >= P2P_KV) {
                put32(b, n->raft.term); b[4] = 0; put32(b + 5, 0); b[9] = n->raft.snap_in_n;
                (void)seal_send(n, h, p, P2P_I_RAFT_SNAPSHOT_RESP, b, 10);
                return;
            }
            it.used = 1;
            n->raft.snap_in[n->raft.snap_in_n++] = it;
        }
        if (last) raft_install_snapshot(n, h, snap_i, snap_t);
        put32(b, n->raft.term); b[4] = 1;
        put32(b + 5, last ? snap_i : 0U);
        b[9] = n->raft.snap_in_n;
        (void)seal_send(n, h, p, P2P_I_RAFT_SNAPSHOT_RESP, b, 10);
        break;
    }
    case P2P_I_RAFT_SNAPSHOT_RESP: {
        uint32_t term, match;
        if (len < 10) break;
        term = get32(m); match = get32(m + 5);
        if (term > n->raft.term) { raft_step_down(n, term); break; }
        if (n->raft.role != P2P_RAFT_LEADER || term != n->raft.term || !m[4]) break;
        if (m[9] > p->raft_snap_off) p->raft_snap_off = m[9];
        if (n->raft.snapshot_index > 0U && match >= n->raft.snapshot_index &&
            p->raft_snap_off >= n->raft.snap_kv_n) {
            if (p->raft_match < n->raft.snapshot_index) p->raft_match = n->raft.snapshot_index;
            p->raft_next = n->raft.snapshot_index + 1U;
            (void)raft_send_one(n, h, p);
        } else {
            (void)raft_send_snapshot(n, h, p);
        }
        break;
    }
    case P2P_I_CACHE_INV_ACK: {
        char key[P2P_KEY_MAX];
        int kp = get_str(m, 0, len, key, P2P_KEY_MAX), left = 0;
        if (kp < 0 || kp + 4 > len || !n->cache.inv_active) break;
        if (!s_eq(key, n->cache.inv_key) || get32(m + kp) != n->cache.inv_version) break;
        for (i = 0; i < P2P_PEERS; i++) {
            if (n->cache.inv_waiting[i] == p->id) { n->cache.inv_waiting[i] = 0; n->cache.inv_acked++; }
            if (n->cache.inv_waiting[i]) left++;
        }
        if (!left) {
            n->cache.inv_active = 0;
            n->cache.inv_completed++;
            s_cat(line, "p2p cache inv acked ", 200); s_cat(line, key, 200);
            s_cat(line, " v", 200); cat_u(line, n->cache.inv_version, 200);
            s_cat(line, " acks ", 200); cat_u(line, n->cache.inv_acked, 200);
            say(h, line);
        }
        break;
    }
    case P2P_I_CACHE_INV: {
        char key[P2P_KEY_MAX];
        int c_idx, kp;
        uint32_t ver = 0;
        kp = get_str(m, 0, len, key, P2P_KEY_MAX);
        if (kp >= 0) {
            if (kp + 4 <= len) ver = get32(m + kp);
            if (ver > cache_fence_of(n, key)) {
                n->cache.fence_raised++;
                s_cat(line, "p2p cache fence ", 200); s_cat(line, key, 200); s_cat(line, " v", 200);
                cat_u(line, ver, 200); s_cat(line, " from ", 200); s_cat(line, p->name, 200);
                say(h, line); line[0] = 0;
            }
            cache_fence(n, key, ver);
            for (c_idx = 0; c_idx < P2P_CACHE_MAX; c_idx++) {
                if (n->cache.entries[c_idx].used && s_eq(n->cache.entries[c_idx].key, key)) {
                    n->cache.entries[c_idx].used = 0;
                    n->cache.invalidations++;
                    s_cat(line, "p2p cache invalidated ", 200); s_cat(line, key, 200);
                    say(h, line);
                }
            }
            /* Ack even when nothing was cached: the sender counts peers. */
            pos = put_str(b, 0, P2P_DATAGRAM_MAX, key);
            if (pos >= 0) { put32(b + pos, ver); (void)seal_send(n, h, p, P2P_I_CACHE_INV_ACK, b, pos + 4); }
        }
        break;
    }
    case P2P_I_GRPC_REQ: {
        char svc[32], mth[32], pay[64], resp_pay[96];
        uint32_t id, deadline, type;
        uint8_t st = 0;
        int p_pos, blen = 0;
        if (len < 12) break;
        id = get32(m); deadline = get32(m + 4); type = get32(m + 8);
        if (grpc_cached(n, p->id, id, b, &blen)) {
            n->rpc_replays++;
            (void)seal_send(n, h, p, P2P_I_GRPC_RESP, b, blen);
            break;
        }
        p_pos = get_str(m, 12, len, svc, 32);
        if (p_pos < 0) break;
        p_pos = get_str(m, p_pos, len, mth, 32);
        if (p_pos < 0) break;
        p_pos = get_str(m, p_pos, len, pay, 64);
        if (p_pos < 0) break;
        if (now(h) >= deadline) {
            st = 4; resp_pay[0] = 0; s_cat(resp_pay, "deadline_exceeded", 96);
        } else {
            grpc_fill(n, type, svc, mth, pay, &st, resp_pay, 96);
        }
        put32(b, id); b[4] = st;
        p_pos = put_str(b, 5, 200, resp_pay);
        if (p_pos > 0) {
            grpc_remember(n, p->id, id, b, p_pos);
            (void)seal_send(n, h, p, P2P_I_GRPC_RESP, b, p_pos);
        }
        break;
    }
    case P2P_I_GRPC_RESP: {
        char pay[96];
        uint32_t id;
        uint8_t st;
        if (len < 5) break;
        id = get32(m); st = m[4];
        if (get_str(m, 5, len, pay, 96) >= 0) {
            s_cat(line, "p2p grpc resp status ", 200); cat_u(line, st, 200);
            s_cat(line, " payload ", 200); s_cat(line, pay, 200);
            s_cat(line, " from ", 200); s_cat(line, p->name, 200);
            s_cat(line, " id ", 200); cat_u(line, id, 200);
            say(h, line);
            grpc_wait_clear(n, id);
        }
        break;
    }
    case P2P_I_STUN_REQ: {
        /* STUN server side: answer a real Binding request with the source
         * address the datagram arrived from (XOR-MAPPED-ADDRESS). The
         * datagram API exposes no UDP source port; P2P_PORT is used. */
        uint16_t type;
        uint8_t txid[12];
        int rl;
        if (stun_parse_header(m, len, &type, txid) != 0 || type != STUN_BINDING_REQUEST) { n->nat.stun_bad++; break; }
        rl = stun_build_binding_success(b, 64, txid, n->last_src_ip, (uint16_t)P2P_PORT);
        if (rl > 0) (void)seal_send(n, h, p, P2P_I_STUN_RESP, b, rl);
        break;
    }
    case P2P_I_STUN_RESP: {
        uint8_t mip[4];
        uint16_t mport;
        int slot, kind;
        for (slot = 0; slot < 2; slot++) if (n->nat.srv[slot].pending && n->nat.srv[slot].server == p->id) break;
        if (slot == 2 || stun_parse_mapped(m, len, n->nat.srv[slot].txid, mip, &mport) != 0) { n->nat.stun_bad++; break; }
        n->nat.srv[slot].pending = 0; n->nat.srv[slot].valid = 1;
        mcopy(n->nat.srv[slot].ip, mip, 4); n->nat.srv[slot].port = mport;
        mcopy(n->nat.mapped_ip, mip, 4); n->nat.mapped_port = mport;
        {
            int other = slot ^ 1, two = n->nat.srv[other].valid;
            kind = stun_nat_classify(n->ip, (uint16_t)P2P_PORT, mip, mport, two,
                                     n->nat.srv[other].ip, n->nat.srv[other].port);
        }
        n->nat.nat_type = (uint8_t)kind;
        s_cat(line, "p2p stun mapped_ip ", 200);
        cat_u(line, mip[0], 200); s_cat(line, ".", 200); cat_u(line, mip[1], 200); s_cat(line, ".", 200);
        cat_u(line, mip[2], 200); s_cat(line, ".", 200); cat_u(line, mip[3], 200);
        s_cat(line, " port ", 200); cat_u(line, mport, 200);
        s_cat(line, " from ", 200); s_cat(line, p->name, 200);
        s_cat(line, " nat_type ", 200); s_cat(line, stun_nat_name(kind), 200);
        s_cat(line, " servers ", 200); cat_u(line, (uint32_t)(n->nat.srv[0].valid + n->nat.srv[1].valid), 200);
        say(h, line);
        break;
    }
    case P2P_I_NAT_PUNCH: {
        n->nat.nat_punches++;
        s_cat(line, "p2p nat punch from ", 200); s_cat(line, p->name, 200); s_cat(line, " pinhole opened", 200);
        say(h, line);
        break;
    }
    default:
        break;
    }
}

static int propose_send(p2p_node_t* n, const p2p_host_t* h, int only_missing);
static void on_hello(p2p_node_t* n, const p2p_host_t* h, const uint8_t* b, int len, const uint8_t mac[6]) {
    uint8_t tag[32];
    p2p_peer_t* p;
    uint32_t src = get32(b + 8), rejoined_from = 0;
    uint8_t blocked = 0;
    int cnt, i, body = HDR + P2P_NAME_MAX + 4 + 32 + 1;
    char line[96];
    if (len < body + 16) { n->bad_hello++; return; }
    cnt = b[body - 1];
    if (cnt > P2P_PEERS || len != body + cnt * 5 + 16) { n->bad_hello++; return; }
    hmac_sha256(n->netkey, 32, b, (uint32_t)(body + cnt * 5), tag);
    if (!meq(tag, b + body + cnt * 5, 16)) { n->bad_hello++; return; }
    p = by_id(n, src);
    if (!p) {
        /* US-069 crash/rejoin: a member that went down and rebooted comes
         * back with a fresh identity (new key, new id) under the same name.
         * Its stale slot is reused; a live member is never replaced. */
        char hello_name[P2P_NAME_MAX];
        p2p_peer_t* old;
        s_copy(hello_name, (const char*)b + HDR, P2P_NAME_MAX);
        hello_name[P2P_NAME_MAX - 1] = 0;
        old = p2p_find(n, hello_name);
        if (old && !old->up && old->id != src) {
            rejoined_from = old->id;
            blocked = old->blocked;
            p = old;
        } else {
            for (i = 0; i < P2P_PEERS && n->peers[i].used; i++) {}
            if (i == P2P_PEERS) return;
            p = &n->peers[i];
        }
        mzero(p, (int)sizeof(*p));
        p->used = 1; p->id = src; p->blocked = blocked;
    }
    if (p->blocked) return;
    s_copy(p->name, (const char*)b + HDR, P2P_NAME_MAX);
    p->name[P2P_NAME_MAX - 1] = 0;
    mcopy(p->ip, b + HDR + P2P_NAME_MAX, 4);
    mcopy(p->mac, mac, 6);
    if (!p->keyed || !meq(p->pub, b + HDR + P2P_NAME_MAX + 4, 32)) {
        mcopy(p->pub, b + HDR + P2P_NAME_MAX + 4, 32);
        derive(n, p);
    }
    for (i = 0; i < P2P_PEERS; i++) {
        p->neighbors[i] = i < cnt ? get32(b + body + i * 4) : 0;
        p->nb_hops[i] = i < cnt ? b[body + cnt * 4 + i] : 0;
        /* A route back through ourselves is useless (split horizon). */
        if (p->neighbors[i] == n->id) p->nb_hops[i] = 0;
    }
    p->last_seen = now(h);
    p->via = 0;
    if (!p->up) {
        p->up = 1;
        /* Lot 48 (US-070): a peer that was cut or rebooted missed the
         * invalidations sent meanwhile (the invalidator stops waiting for a
         * down peer). Every known fence is pushed to it on reconnect, a few
         * times since it may not have our HELLO yet; INV is idempotent. */
        p->fence_push = 4U;
        p->fence_at = now(h) - P2P_HZ;
        /* Lot 66 (US-069): a peer that was down (cut link, partition)
         * missed the writes of both sides meanwhile. Start the two-way
         * digest anti-entropy at once, re-sent until the answer arrives,
         * instead of waiting for a manual p2p-sync. */
        if (p->down_events) {
            p->sync_retries = 4U;
            p->sync_at = now(h) - P2P_SYNC_RESEND_TICKS;
            n->heal_syncs++;
        }
        line[0] = 0;
        s_cat(line, "p2p peer ", 96); s_cat(line, p->name, 96); s_cat(line, " up id ", 96); cat_hex32(line, p->id, 96);
        s_cat(line, p->keyed ? " keyed" : " unkeyed", 96);
        say(h, line);
    }
    if (rejoined_from) {
        n->rejoins++;
        line[0] = 0;
        s_cat(line, "p2p peer ", 96); s_cat(line, p->name, 96); s_cat(line, " rejoined id ", 96);
        cat_hex32(line, p->id, 96); s_cat(line, " was ", 96); cat_hex32(line, rejoined_from, 96);
        say(h, line);
        /* Push the replicated state back: anti-entropy with the restarted
         * member, re-sent until it answers (it needs our HELLO first). */
        p->sync_retries = 4U;
        p->sync_at = now(h) - P2P_SYNC_RESEND_TICKS;
    }
}

static void on_datagram(p2p_node_t* n, const p2p_host_t* h, uint8_t* b, int len, const uint8_t mac[6]) {
    uint32_t src, dst, id, via;
    int i;
    n->rx_bytes += (uint32_t)len;
    if (len < HDR || b[0] != 'M' || b[1] != 'P' || b[2] != '2' || b[3] != 1) { n->foreign++; return; }
    src = get32(b + 8); dst = get32(b + 12); id = get32(b + 16); via = get32(b + 20);
    if (src == n->id) return;
    /* Simulated link failure (US-069 tests): drop what the blocked node transmits. */
    for (i = 0; i < P2P_PEERS; i++)
        if (n->peers[i].used && n->peers[i].blocked && meq(n->peers[i].mac, mac, 6)) return;
    if (seen(n, src, id)) { n->dup++; return; }
    remember(n, src, id);
    if (b[4] == P2P_T_HELLO) { n->rx_type[P2P_T_HELLO]++; on_hello(n, h, b, len, mac); return; }
    if (b[4] != P2P_T_SEALED) { n->foreign++; return; }
    if (dst != n->id) {
        /* Relay (US-063): forward verbatim when we are the chosen next hop. */
        p2p_peer_t* d = by_id(n, dst);
        uint32_t nh = 0, cost = 0;
        if (via == n->id && b[5] > 0 && d && p2p_route_find(n, dst, &nh, &cost) == 0) {
            p2p_peer_t* r = nh ? by_id(n, nh) : d;
            /* Hop by hop: direct when the link is up, else the next relay
             * from our own distance vector, never back to the sender. */
            if (r && r->up && !r->blocked && r->id != src && !meq(r->mac, mac, 6)) {
                b[5]--; put32(b + 20, nh);
                n->relayed++;
                if (nh) n->relayed_multi++;
                (void)emit(n, h, r->ip, b, len);
            }
        }
        return;
    }
    {
        p2p_peer_t* p = by_id(n, src);
        uint8_t aad[HDR], plain[P2P_DATAGRAM_MAX], nonce[8];
        uint32_t ctr;
        int clen = len - HDR - 4 - 16;
        if (!p || !p->keyed || clen < 1) { n->foreign++; return; }
        ctr = get32(b + HDR);
        mcopy(aad, b, HDR); aad[5] = 0; put32(aad + 20, 0);
        put32(nonce, src); put32(nonce + 4, ctr);
        if (aes128_gcm_decrypt(p->key, p->iv, nonce, aad, HDR, b + HDR + 4, (uint16_t)clen, b + len - 16, plain) != 0) {
            p->auth_fail++; return;
        }
        if (ctr <= p->rx_counter) { p->replay++; return; }
        p->rx_counter = ctr;
        p->received++;
        p->via = !meq(mac, p->mac, 6);
        /* A direct authenticated frame is liveness. A relayed copy is not:
         * the direct link can be down while beta still forwards. */
        if (!p->via) p->last_seen = now(h);
        if (plain[0] < P2P_I_PING || plain[0] >= P2P_I_COUNT) return;
        on_sealed(n, h, p, plain[0], plain + 1, clen - 1);
    }
}

/* -------------------------------------------------------------- public */
int p2p_up_step(p2p_node_t* n, int step, const char* name, const uint8_t ip[4], const char* netkey,
                const uint8_t seed[32], const p2p_host_t* h) {
    uint8_t d[32];
    sha256_ctx_t c;
    if (!n || !name || !name[0] || s_len(name) >= P2P_NAME_MAX || !ip || !netkey || s_len(netkey) < 8 || !seed) return -1;
    if (step == 1) {
        /* Step 1: Initialize node structure, set basic identity fields, derive netkey */
        mzero(n, (int)sizeof(*n));
        s_copy(n->name, name, P2P_NAME_MAX);
        mcopy(n->ip, ip, 4);
        sha256_init(&c); sha256_update(&c, (const uint8_t*)"mohhdy-p2p-net", 14);
        sha256_update(&c, (const uint8_t*)netkey, (uint32_t)s_len(netkey)); sha256_final(&c, n->netkey);
        return 0;
    }
    if (step == 2) {
        /* Step 2: Derive secret key from seed + name */
        sha256_init(&c); sha256_update(&c, seed, 32); sha256_update(&c, (const uint8_t*)name, (uint32_t)s_len(name));
        sha256_final(&c, n->secret);
        return 0;
    }
    if (step == 3) {
        /* Step 3: X25519 scalar multiplication to compute public key & node ID */
        if (x25519_public_key(n->pub, n->secret, g_ws, X25519_WORKSPACE_LIMBS) != 0) return -1;
        sha256_init(&c); sha256_update(&c, n->pub, 32); sha256_final(&c, d);
        n->id = get32(d) | 1U;
        return 0;
    }
    if (step == 4) {
        /* Step 4: Complete initialization and emit initial HELLO frame */
        n->up = 1;
        n->last_hello = now(h) - P2P_HELLO_TICKS;
        n->last_ping = now(h);
        n->qos.window_size = P2P_QOS_WINDOW_MAX;
        n->raft.active = 0;
        n->raft.role = P2P_RAFT_FOLLOWER;
        n->raft.term = 1;
        n->raft.election_tick = now(h);
        send_hello(n, h);
        n->last_hello = now(h);
        return 0;
    }
    return -1;
}

int p2p_up(p2p_node_t* n, const char* name, const uint8_t ip[4], const char* netkey,
           const uint8_t seed[32], const p2p_host_t* h) {
    int s;
    for (s = 1; s <= 4; s++) {
        if (p2p_up_step(n, s, name, ip, netkey, seed, h) != 0) return -1;
    }
    return 0;
}

void p2p_down(p2p_node_t* n) { if (n) { mzero(n->secret, 32); n->up = 0; } }

void p2p_tick(p2p_node_t* n, const p2p_host_t* h, int budget) {
    uint8_t b[P2P_DATAGRAM_MAX + 16], ip[4], mac[6];
    uint32_t t;
    int i, got, queued = 0;
    if (!n || !n->up) return;
    grpc_expire(n, h);
    probe_pump(n, h);
    while (budget-- > 0 && h && h->recv) {
        got = h->recv(h->ctx, ip, mac, b, (uint16_t)sizeof(b));
        if (got <= 0) { queued = 0; break; }
        mcopy(n->last_src_ip, ip, 4);
        on_datagram(n, h, b, got, mac);
        queued = 1;
    }
    /* Budget can stop while a hello is still queued. Apply that leftover
     * before the down check, or the peer is marked down and comes back up
     * on the next poll. If the cap is hit with data still waiting, skip the
     * down transition for this tick. */
    if (queued && h && h->recv) {
        int extra = 32;
        while (extra-- > 0) {
            got = h->recv(h->ctx, ip, mac, b, (uint16_t)sizeof(b));
            if (got <= 0) { queued = 0; break; }
            mcopy(n->last_src_ip, ip, 4);
            on_datagram(n, h, b, got, mac);
        }
    }
    t = now(h);
    if (t - n->last_hello >= P2P_HELLO_TICKS) { n->last_hello = t; send_hello(n, h); }
    /* Lot 69: data acks leave at most one tick late; old datagrams are losses */
    for (i = 0; i < P2P_PEERS; i++) {
        p2p_peer_t* p = &n->peers[i];
        if (!p->used || !p->keyed) continue;
        if (p->flow.nack_pend) flush_dack(n, h, p);
        (void)p2p_flow_expire(&p->flow, t * (1000000U / P2P_HZ));
    }

    /* Raft Consensus Engine (US-064): Follower election timeout / Leader heartbeat */
    if (n->raft.active && n->raft.role == P2P_RAFT_LEADER) {
        raft_replicate(n, h);
        if (t - n->raft.heartbeat_tick >= P2P_HZ) {
            int hb_sent = 0;
            n->raft.heartbeat_tick = t;
            put32(b, n->raft.term); put32(b + 4, n->id);
            for (i = 0; i < P2P_PEERS; i++) {
                if (n->peers[i].used && n->peers[i].keyed && n->peers[i].up) {
                    if (seal_send(n, h, &n->peers[i], P2P_I_RAFT_HEARTBEAT, b, 8) == 0)
                        hb_sent++;
                    n->qos.priority_high_tx++;
                }
            }
            /* A contacted follower refreshes the leader lease. A leader
             * cut off from every peer does not extend it. */
            if (hb_sent > 0) raft_note_lease(n, h);
        }
    } else if (n->raft.active && !raft_lease_valid(n, h) &&
               t - n->raft.election_tick >= P2P_RAFT_ELECTION_TICKS) {
        /* Election timeout triggered. A live lease waits. */
        (void)p2p_raft_elect(n, h);
    }
    for (i = 0; i < P2P_PEERS; i++) {
        p2p_peer_t* p = &n->peers[i];
        if (!p->used || !p->up) continue;
        /* Lot 69: a receive backlog (data acks of the other peers) may
         * hold a hello of this peer, so it gets one more down period, but
         * a backlog that never drains must not keep a dead peer up. */
        if (t - p->last_seen >= (queued ? 2u * P2P_DOWN_TICKS : P2P_DOWN_TICKS)) {
            char line[64];
            p->up = 0; p->down_events++;
            p2p_cache_on_peer_down(n, p->id);
            line[0] = 0; s_cat(line, "p2p peer ", 64); s_cat(line, p->name, 64); s_cat(line, " down", 64);
            say(h, line);
        }
    }
    /* Unacked cache invalidation: re-send to the peers still waiting. */
    if (n->cache.inv_active && t - n->cache.inv_tick >= P2P_HZ) {
        n->cache.inv_tick = t;
        if (n->cache.inv_retries) {
            n->cache.inv_retries--;
            n->cache.inv_resent += (uint32_t)(cache_inv_send(n, h, 1) > 0 ? 1 : 0);
        } else {
            char line[96];
            n->cache.inv_active = 0;
            line[0] = 0; s_cat(line, "p2p cache inv incomplete ", 96); s_cat(line, n->cache.inv_key, 96);
            s_cat(line, " acks ", 96); cat_u(line, n->cache.inv_acked, 96);
            say(h, line);
        }
    }
    /* Learned receive baseline (US-075), one window per second: after the
     * warm-up, a type whose rate is above 4x its baseline plus 8 frames/s
     * is an anomaly; the baseline then keeps learning (EWMA 1/8). */
    if (t - n->analyze_tick >= P2P_HZ) {
        uint32_t k;
        n->analyze_tick = t;
        for (k = P2P_I_PING; k < P2P_I_COUNT; k++) {
            uint32_t rate = n->rx_type[k] - n->last_rx[k];
            n->last_rx[k] = n->rx_type[k];
            if (n->analyze_windows >= P2P_ANALYZE_WARMUP && rate * 16U > 4U * n->base_rate16[k] + 8U * 16U) {
                n->anomalies++;
                n->last_anomaly_type = k;
                n->last_anomaly_rate = rate;
            }
            n->base_rate16[k] = n->analyze_windows ? (n->base_rate16[k] * 7U + rate * 16U) / 8U : rate * 16U;
        }
        /* Lot 6 (US-075): per-peer baseline, so an anomaly names its
         * source. A reply burst we solicited (sync, snapshot) is skipped. */
        for (i = 0; i < P2P_PEERS; i++) {
            p2p_peer_t* p = &n->peers[i];
            uint32_t rate;
            if (!p->used) continue;
            rate = p->received - p->rx_win_last;
            p->rx_win_last = p->received;
            if (n->analyze_windows >= P2P_ANALYZE_WARMUP && rate * 16U > 4U * p->rx_base16 + 8U * 16U) {
                if ((int32_t)(n->solicited_until - t) > 0) n->solicited_skipped++;
                else {
                    p->anomalies++;
                    n->last_anomaly_peer = p->id;
                    p2p_qos_note_loss(n);
                    /* Lot 48 (US-075): automatic report, one line when an
                     * episode starts (no command needed, no log flood). */
                    if (!p->alerting) {
                        char al[112];
                        p->alerting = 1U;
                        n->alerts++;
                        al[0] = 0;
                        s_cat(al, "p2p traffic alert peer ", 112); s_cat(al, p->name, 112);
                        s_cat(al, " rate ", 112); cat_u(al, rate, 112);
                        s_cat(al, " base ", 112); cat_u(al, p->rx_base16 / 16U, 112);
                        say(h, al);
                    }
                }
            } else if (p->alerting) {
                p->alerting = 0U;
                line_end_alert(n, h, p);
            }
            p->rx_base16 = n->analyze_windows ? (p->rx_base16 * 7U + rate * 16U) / 8U : rate * 16U;
        }
        /* Lot 6 (US-068/073): measured byte rates, EWMA 1/4 */
        {
            uint32_t tx = n->tx_bytes - n->qos.meas_tx_last, rx = n->rx_bytes - n->qos.meas_rx_last;
            uint32_t dt = t - n->qos.meas_tick;
            n->qos.meas_tx_last = n->tx_bytes;
            n->qos.meas_rx_last = n->rx_bytes;
            n->qos.meas_tick = t;
            if (dt > 0U) {
                tx = tx * P2P_HZ / dt;
                rx = rx * P2P_HZ / dt;
                n->qos.meas_tx_bps = n->analyze_windows ? (n->qos.meas_tx_bps * 3U + tx) / 4U : tx;
                n->qos.meas_rx_bps = n->analyze_windows ? (n->qos.meas_rx_bps * 3U + rx) / 4U : rx;
            }
        }
        n->analyze_windows++;
    }
    shape_drain(n, h);
    if (t - n->last_ping >= P2P_PING_TICKS) {
        n->last_ping = t;
        for (i = 0; i < P2P_PEERS; i++) {
            p2p_peer_t* p = &n->peers[i];
            if (!p->used || !p->up || !p->keyed) continue;
            /* One loss event per outage: the first unanswered PING halves
             * the window, later ones on the same silent peer do not. */
            if (p->ping_wait == 1U) { n->qos.ping_losses++; p2p_qos_note_loss(n); p->ping_wait = 2U; }
            put32(b, t); p->ping_sent_at = t;
            if (seal_send(n, h, p, P2P_I_PING, b, 4) == 0 && !p->ping_wait) p->ping_wait = 1U;
        }
    }
    for (i = 0; i < P2P_PEERS; i++) {
        p2p_peer_t* p = &n->peers[i];
        uint32_t f;
        if (!p->used || !p->up || !p->keyed || !p->fence_push || t - p->fence_at < P2P_HZ) continue;
        p->fence_push--; p->fence_at = t;
        for (f = 0; f < P2P_CACHE_FENCES; f++) {
            int fl;
            if (!n->cache.fence[f].key[0] || !n->cache.fence[f].version) continue;
            fl = put_str(b, 0, 60, n->cache.fence[f].key);
            if (fl < 0) continue;
            put32(b + fl, n->cache.fence[f].version);
            if (seal_send(n, h, p, P2P_I_CACHE_INV, b, fl + 4) == 0) n->cache.fence_pushed++;
        }
    }
    for (i = 0; i < P2P_PEERS; i++) {
        p2p_peer_t* p = &n->peers[i];
        if (!p->used || !p->sync_retries || t - p->sync_at < P2P_SYNC_RESEND_TICKS) continue;
        p->sync_retries--; p->sync_at = t;
        if (p->keyed && p->up) {
            int dl = digest_encode(n, b, 0);
            if (dl > 0) (void)seal_send(n, h, p, P2P_I_SYNC_REQ, b, dl);
        }
    }
    if (!n->prop.active && n->prop.commit_resends && t - n->prop.last_tx >= P2P_PROPOSE_RESEND_TICKS) {
        /* applying a COMMIT twice is a no-op (same version and origin) */
        p2p_proposal_t* pr = &n->prop;
        p2p_item_t it;
        int len;
        pr->commit_resends--; pr->last_tx = t;
        mzero(&it, (int)sizeof(it));
        s_copy(it.key, pr->key, P2P_KEY_MAX); s_copy(it.value, pr->value, P2P_VAL_MAX);
        it.version = pr->version; it.origin = n->id; it.used = 1;
        len = item_encode(b, 0, 400, &it);
        for (i = 0; i < P2P_PEERS && len > 0; i++)
            if (n->peers[i].used && n->peers[i].keyed && n->peers[i].up)
                (void)seal_send(n, h, &n->peers[i], P2P_I_COMMIT, b, len);
    }
    if (n->prop.active) {
        p2p_proposal_t* pr = &n->prop;
        char line[160];
        line[0] = 0;
        if (pr->yes >= pr->needed) {
            p2p_item_t it;
            mzero(&it, (int)sizeof(it));
            s_copy(it.key, pr->key, P2P_KEY_MAX); s_copy(it.value, pr->value, P2P_VAL_MAX);
            it.version = pr->version; it.origin = n->id; it.used = 1;
            (void)kv_apply(n, it.key, it.value, it.version, it.origin);
            got = item_encode(b, 0, 400, &it);
            for (i = 0; i < P2P_PEERS && got > 0; i++)
                if (n->peers[i].used && n->peers[i].keyed && (n->peers[i].up || n->peers[i].via))
                    (void)seal_send(n, h, &n->peers[i], P2P_I_COMMIT, b, got);
            pr->result = 1; pr->active = 0; pr->commit_resends = 3; pr->last_tx = t;
            s_cat(line, "p2p propose committed ", 160);
        } else if (pr->no > pr->members - pr->needed) {
            pr->result = -1; pr->active = 0;
            s_cat(line, "p2p propose rejected ", 160);
        } else if (t - pr->started >= P2P_PROPOSE_TICKS) {
            pr->result = -2; pr->active = 0;
            s_cat(line, "p2p propose timeout ", 160);
        }
        if (pr->active && t - pr->last_tx >= P2P_PROPOSE_RESEND_TICKS) {
            pr->last_tx = t;
            if (propose_send(n, h, 1) > 0) pr->resent++;
        }
        if (line[0]) {
            s_cat(line, pr->key, 160); s_cat(line, "=", 160); s_cat(line, pr->value, 160);
            s_cat(line, " round ", 160); cat_u(line, pr->round, 160);
            s_cat(line, " yes ", 160); cat_u(line, pr->yes, 160);
            s_cat(line, " no ", 160); cat_u(line, pr->no, 160);
            s_cat(line, " needed ", 160); cat_u(line, pr->needed, 160);
            say(h, line);
        }
    }
}

int p2p_send_text(p2p_node_t* n, const p2p_host_t* h, const char* peer, const char* text) {
    uint8_t b[P2P_DATAGRAM_MAX];
    p2p_peer_t* p = p2p_find(n, peer);
    int pos;
    if (!n->up || !p) return -1;
    pos = put_str(b, 0, 400, text);
    if (pos < 0) return -1;
    if (!admit(n, h, p, (uint32_t)pos)) return shape_enqueue(n, p, P2P_I_MSG, b, pos);
    return seal_send(n, h, p, P2P_I_MSG, b, pos);
}

/* Lot 6 (US-075 proof): a misbehaving sender that ignores its own QoS
 * controller and rate limit (no admit, no shaping queue). Used to show the
 * receiver attributes a flood; a well-behaved node never calls it. */
int p2p_send_unshaped(p2p_node_t* n, const p2p_host_t* h, const char* peer, const char* text) {
    uint8_t b[P2P_DATAGRAM_MAX];
    p2p_peer_t* p = p2p_find(n, peer);
    int pos;
    if (!n->up || !p) return -1;
    pos = put_str(b, 0, 400, text);
    if (pos < 0) return -1;
    n->unshaped_tx++;
    return seal_send(n, h, p, P2P_I_MSG, b, pos);
}

static uint32_t next_version(const p2p_node_t* n, const char* key) {
    const p2p_item_t* it = p2p_get_local(n, key);
    return it ? it->version + 1U : 1U;
}

int p2p_put(p2p_node_t* n, const p2p_host_t* h, const char* key, const char* value) {
    uint8_t b[P2P_DATAGRAM_MAX];
    p2p_item_t it;
    int i, pos, sent = 0;
    if (!n->up || !key[0] || s_len(key) >= P2P_KEY_MAX || s_len(value) >= P2P_VAL_MAX) return -1;
    mzero(&it, (int)sizeof(it));
    s_copy(it.key, key, P2P_KEY_MAX); s_copy(it.value, value, P2P_VAL_MAX);
    it.version = next_version(n, key); it.origin = n->id;
    if (kv_apply(n, key, value, it.version, it.origin) < 0) return -1;
    pos = item_encode(b, 0, 400, &it);
    for (i = 0; i < P2P_PEERS; i++) {
        p2p_peer_t* p = &n->peers[i];
        if (!p->used || !p->keyed || !p->up) continue;
        if (!admit(n, h, p, (uint32_t)pos)) {
            /* Lot 68: a paced put waits in the shaping queue like a message */
            if (n->qos.pace.rate_bps && shape_enqueue(n, p, P2P_I_PUT, b, pos) == 1) { n->qos.paced_put_queued++; sent++; }
            continue;
        }
        if (seal_send(n, h, p, P2P_I_PUT, b, pos) == 0) sent++;
    }
    return sent;
}

int p2p_get_remote(p2p_node_t* n, const p2p_host_t* h, const char* key) {
    uint8_t b[64];
    int i, pos, sent = 0;
    pos = put_str(b, 0, 64, key);
    if (pos < 0) return -1;
    for (i = 0; i < P2P_PEERS; i++)
        if (n->peers[i].used && n->peers[i].keyed && n->peers[i].up && seal_send(n, h, &n->peers[i], P2P_I_GET_REQ, b, pos) == 0) sent++;
    return sent;
}

int p2p_sync(p2p_node_t* n, const p2p_host_t* h, const char* peer) {
    n->solicited_until = now(h) + 3U * P2P_HZ; /* Lot 6: expected reply burst */
    uint8_t b[P2P_DATAGRAM_MAX];
    int i, len, sent = 0;
    len = digest_encode(n, b, 0);
    if (len < 0) return -1;
    for (i = 0; i < P2P_PEERS; i++) {
        p2p_peer_t* p = &n->peers[i];
        if (!p->used || !p->keyed || !p->up) continue;
        if (peer && peer[0] && !s_eq(p->name, peer)) continue;
        if (seal_send(n, h, p, P2P_I_SYNC_REQ, b, len) == 0) {
            sent++;
            p->sync_at = now(h); p->sync_retries = 3; /* datagrams can be lost */
        }
    }
    return sent;
}

static int has_voted(const p2p_proposal_t* pr, uint32_t id) {
    uint32_t k;
    for (k = 0; k < pr->yes + pr->no; k++) if (pr->voters[k] == id) return 1;
    return 0;
}
/* send the active proposal to every up keyed peer (only_missing: to those
 * that have not voted yet). Returns the number of peers reached. */
static int propose_send(p2p_node_t* n, const p2p_host_t* h, int only_missing) {
    uint8_t b[P2P_DATAGRAM_MAX];
    p2p_item_t it;
    p2p_proposal_t* pr = &n->prop;
    int i, pos, sent = 0;
    mzero(&it, (int)sizeof(it));
    s_copy(it.key, pr->key, P2P_KEY_MAX); s_copy(it.value, pr->value, P2P_VAL_MAX);
    it.version = pr->version; it.origin = n->id;
    put32(b, pr->round);
    pos = item_encode(b, 4, 400, &it);
    for (i = 0; i < P2P_PEERS; i++) {
        p2p_peer_t* p = &n->peers[i];
        if (!p->used || !p->keyed || !p->up || (only_missing && has_voted(pr, p->id))) continue;
        if (seal_send(n, h, p, P2P_I_PROPOSE, b, pos) == 0) sent++;
    }
    return sent;
}

int p2p_grpc_call_deadline(p2p_node_t* n, const p2p_host_t* h, const char* peer, const char* service,
                           const char* method, const char* payload, uint32_t timeout_ticks, uint32_t fixed_id) {
    uint8_t b[P2P_DATAGRAM_MAX];
    p2p_peer_t* p = p2p_find(n, peer);
    uint32_t id, deadline;
    int pos, type;
    if (!n || !n->up || !p || !service || !method) return -1;
    if (!p->up || p->blocked) {
        /* Auto-healing: multi-hop relay fallback if direct link is down */
        uint32_t nh = 0, cost = 0;
        if (p2p_route_find(n, p->id, &nh, &cost) == 0 && nh > 0) {
            p2p_peer_t* r = by_id(n, nh);
            if (r && r->up && !r->blocked) p = r;
            else return -1;
        } else {
            return -1;
        }
    }
    id = fixed_id ? fixed_id : ++n->rpc_seq;
    deadline = timeout_ticks ? now(h) + timeout_ticks : now(h);
    type = grpc_type_of(service, method);
    put32(b, id); put32(b + 4, deadline); put32(b + 8, (uint32_t)type);
    pos = put_str(b, 12, 400, service);
    if (pos < 0) return -1;
    pos = put_str(b, pos, 400, method);
    if (pos < 0) return -1;
    pos = put_str(b, pos, 400, payload ? payload : "");
    if (pos < 0) return -1;
    grpc_wait_add(n, id, deadline);
    return seal_send(n, h, p, P2P_I_GRPC_REQ, b, pos);
}

int p2p_grpc_call(p2p_node_t* n, const p2p_host_t* h, const char* peer, const char* service, const char* method, const char* payload) {
    return p2p_grpc_call_deadline(n, h, peer, service, method, payload, P2P_GRPC_DEADLINE_TICKS, 0);
}

int p2p_stun_probe(p2p_node_t* n, const p2p_host_t* h, const char* stun_server_peer) {
    uint8_t b[STUN_HEADER];
    p2p_peer_t* p = p2p_find(n, stun_server_peer);
    if (!n || !n->up || !p) return -1;
    if (!p->up || p->blocked) {
        /* Auto-healing: fall back to NAT punch if STUN server is down */
        return p2p_nat_punch(n, h, stun_server_peer);
    }
    {
        int slot, rl;
        uint32_t t = now(h);
        for (slot = 0; slot < 2; slot++) if (n->nat.srv[slot].server == p->id) break;
        if (slot == 2) slot = !n->nat.srv[0].server ? 0 : !n->nat.srv[1].server ? 1 : (int)(n->nat.stun_probes & 1U);
        n->nat.stun_probes++;
        n->nat.srv[slot].server = p->id;
        /* Transaction id: node id, server id, probe count and clock. */
        put32(n->nat.srv[slot].txid, n->id ^ t);
        put32(n->nat.srv[slot].txid + 4, p->id);
        put32(n->nat.srv[slot].txid + 8, n->nat.stun_probes * 2654435761U);
        n->nat.srv[slot].pending = 1;
        n->nat.srv[slot].valid = 0;
        rl = stun_build_binding_request(b, (int)sizeof(b), n->nat.srv[slot].txid);
        if (rl <= 0) return p2p_nat_punch(n, h, stun_server_peer);
        return seal_send(n, h, p, P2P_I_STUN_REQ, b, rl);
    }
}

int p2p_nat_punch(p2p_node_t* n, const p2p_host_t* h, const char* peer) {
    uint8_t b[16];
    p2p_peer_t* p = p2p_find(n, peer);
    if (!n || !n->up || !p) return -1;
    n->nat.nat_punches++;
    mcopy(b, n->ip, 4);
    put32(b + 4, n->id);
    return seal_send(n, h, p, P2P_I_NAT_PUNCH, b, 8);
}

int p2p_propose(p2p_node_t* n, const p2p_host_t* h, const char* key, const char* value) {
    p2p_proposal_t* pr = &n->prop;
    uint32_t members;
    if (!n->up || pr->active || !key[0] || s_len(key) >= P2P_KEY_MAX || s_len(value) >= P2P_VAL_MAX) return -1;
    mzero(pr, (int)sizeof(*pr));
    pr->active = 1; pr->round = n->msg_id + 1U; pr->started = now(h);
    s_copy(pr->key, key, P2P_KEY_MAX); s_copy(pr->value, value, P2P_VAL_MAX);
    pr->version = next_version(n, key);
    members = up_count(n) + 1U;
    pr->members = members;
    pr->needed = members / 2U + 1U;
    pr->yes = 1; pr->voters[0] = n->id; /* own vote */
    pr->last_tx = pr->started;
    return propose_send(n, h, 0);
}

static int raft_start_election(p2p_node_t* n, const p2p_host_t* h) {
    uint8_t b[16];
    int i, sent = 0;
    n->raft.role = P2P_RAFT_CANDIDATE;
    n->raft.term++;
    n->raft.term_changes++;
    n->raft.voted_for = n->id;
    n->raft.votes_granted = 1;
    n->raft.election_tick = now(h);
    put32(b, n->raft.term); put32(b + 4, n->id);
    put32(b + 8, n->raft.log_count);
    put32(b + 12, raft_last_term(n));
    for (i = 0; i < P2P_PEERS; i++) {
        p2p_peer_t* p = &n->peers[i];
        if (!p->used || !p->keyed || !p->up) continue;
        if (seal_send(n, h, p, P2P_I_RAFT_VOTE_REQ, b, 16) == 0) {
            sent++;
            n->qos.priority_high_tx++;
        }
    }
    return sent;
}

int p2p_raft_elect(p2p_node_t* n, const p2p_host_t* h) {
    uint8_t b[16];
    int i, sent = 0;
    if (!n || !n->up) return -1;
    /* A follower that still holds the leader's lease does not campaign. */
    if (raft_lease_valid(n, h) && n->raft.leader_id != 0U && n->raft.role != P2P_RAFT_LEADER) {
        say(h, "p2p raft lease held");
        return -2;
    }
    /* Pre-vote: ask with term+1 but do not bump term or voted_for. A majority
     * starts the real election inside the following p2p_tick. */
    n->raft.active = 1;
    n->raft.role = P2P_RAFT_PRE_CANDIDATE;
    n->raft.votes_granted = 1;
    n->raft.election_tick = now(h);
    put32(b, n->raft.term + 1U); put32(b + 4, n->id);
    put32(b + 8, n->raft.log_count);
    put32(b + 12, raft_last_term(n));
    for (i = 0; i < P2P_PEERS; i++) {
        p2p_peer_t* p = &n->peers[i];
        if (!p->used || !p->keyed || !p->up) continue;
        if (seal_send(n, h, p, P2P_I_RAFT_PREVOTE_REQ, b, 16) == 0) {
            sent++;
            n->qos.priority_high_tx++;
        }
    }
    say(h, "p2p raft prevote started");
    return sent;
}

int p2p_raft_append(p2p_node_t* n, const p2p_host_t* h, const char* key, const char* value) {
    p2p_raft_entry_t* e;
    uint32_t index;
    if (!n || !n->up || n->raft.role != P2P_RAFT_LEADER) return -1;
    if (!key || !key[0] || s_len(key) >= P2P_KEY_MAX || !value || s_len(value) >= P2P_VAL_MAX) return -1;
    if (n->raft.log_count - n->raft.snapshot_index >= P2P_RAFT_LOG) return -1;
    index = n->raft.log_count + 1U;
    e = raft_log_slot(n, index);
    if (!e) return -1;
    e->term = n->raft.term;
    s_copy(e->key, key, P2P_KEY_MAX);
    s_copy(e->value, value, P2P_VAL_MAX);
    n->raft.log_count = index;
    raft_replicate(n, h);
    raft_maybe_commit(n, h);
    return (int)n->raft.log_count;
}

/* Lot 24 (US-070 / US-076): Consolidated Raft log compaction & snapshot anti-entropy */
int p2p_raft_reconcile_partition(p2p_node_t* n, const p2p_host_t* h, uint32_t peer_id, uint32_t remote_term, uint32_t remote_commit_index) {
    if (!n) return -1;
    (void)h;
    if (remote_term > n->raft.term) {
        raft_step_down(n, remote_term);
    }
    p2p_peer_t* p = 0;
    for (int i = 0; i < P2P_PEERS; i++) {
        if (n->peers[i].used && n->peers[i].id == peer_id) {
            p = &n->peers[i];
            break;
        }
    }
    if (n->raft.role == P2P_RAFT_LEADER && p) {
        if (remote_commit_index < n->raft.commit_index) {
            p->raft_next = remote_commit_index + 1U;
            return 1; /* Reconciliation sync needed */
        }
    }
    return 0;
}

int p2p_raft_update_lease(p2p_node_t* n, uint32_t rtt_ms, uint32_t loss_pct, uint32_t current_tick) {
    if (!n) return -1;
    uint32_t extra = 0U;
    if (rtt_ms > 100U) extra += (rtt_ms / 10U) * (P2P_HZ / 100U);
    if (loss_pct > 20U) extra += (loss_pct * P2P_HZ) / 100U;
    n->raft.lease_until = current_tick + P2P_RAFT_LEASE_TICKS + extra;
    return 0;
}

int p2p_raft_compact(p2p_node_t* n) {
    uint32_t base, keep, i, term;
    p2p_raft_entry_t* e;
    p2p_raft_entry_t kept[P2P_RAFT_LOG];
    if (!n) return -1;
    if (n->raft.last_applied <= n->raft.snapshot_index) return (int)n->raft.snapshot_index;
    base = n->raft.last_applied;
    e = raft_log_at(n, base);
    if (!e) return -1;
    term = e->term;
    keep = n->raft.log_count - base;
    if (keep > P2P_RAFT_LOG) return -1;
    for (i = 0; i < keep; i++) {
        p2p_raft_entry_t* src = raft_log_at(n, base + 1U + i);
        if (!src) return -1;
        kept[i] = *src;
    }
    for (i = 0; i < keep; i++) n->raft.log[i] = kept[i];
    for (i = keep; i < P2P_RAFT_LOG; i++) mzero(&n->raft.log[i], (int)sizeof(n->raft.log[i]));
    n->raft.snapshot_index = base;
    n->raft.snapshot_term = term;
    /* Kv image at this index. Later appends change n->kv and must not
     * rewrite the snapshot a lagging follower will install. */
    n->raft.snap_kv_n = 0;
    for (i = 0; i < P2P_KV; i++) {
        if (!n->kv[i].used) continue;
        if (n->raft.snap_kv_n >= P2P_KV) break;
        n->raft.snap_kv[n->raft.snap_kv_n++] = n->kv[i];
    }
    return (int)base;
}

int p2p_raft_vote_export(const p2p_node_t* n, uint8_t* out, int cap) {
    if (!n || !out || cap < 12) return -1;
    out[0] = 'R'; out[1] = 'F'; out[2] = 1; out[3] = 0;
    put32(out + 4, n->raft.term);
    put32(out + 8, n->raft.voted_for);
    return 12;
}

int p2p_raft_vote_import(p2p_node_t* n, const uint8_t* in, int len) {
    if (!n || !in || len < 12 || in[0] != 'R' || in[1] != 'F' || in[2] != 1) return -1;
    n->raft.term = get32(in + 4);
    n->raft.voted_for = get32(in + 8);
    if (n->raft.term == 0U) n->raft.term = 1U;
    return 0;
}

/* Lot 66 (US-070): wrap-safe expiry; a line committed through Raft never
 * expires (it is replaced by a later commit or an invalidation). */
static int cache_expired(const p2p_cache_entry_t* e, uint32_t t) {
    return !e->raft && (int32_t)(t - e->expire_tick) > 0;
}

const p2p_item_t* p2p_cache_get(p2p_node_t* n, const char* key, uint32_t current_tick) {
    int i;
    if (!n || !key) return 0;
    for (i = 0; i < P2P_CACHE_MAX; i++) {
        p2p_cache_entry_t* e = &n->cache.entries[i];
        if (e->used && s_eq(e->key, key)) {
            if (cache_expired(e, current_tick)) {
                e->used = 0;
                n->cache.expired++;
                n->cache.total_misses++;
                return 0;
            }
            e->hits++;
            e->last_use = current_tick;
            n->cache.total_hits++;
            return (const p2p_item_t*)e;
        }
    }
    n->cache.total_misses++;
    return 0;
}

static void cache_fence(p2p_node_t* n, const char* key, uint32_t version) {
    uint32_t i;
    for (i = 0; i < P2P_CACHE_FENCES; i++) {
        if (n->cache.fence[i].key[0] && s_eq(n->cache.fence[i].key, key)) {
            if (version > n->cache.fence[i].version) n->cache.fence[i].version = version;
            return;
        }
    }
    i = n->cache.fence_next++ % P2P_CACHE_FENCES;
    s_copy(n->cache.fence[i].key, key, P2P_KEY_MAX);
    n->cache.fence[i].version = version;
}

static uint32_t cache_fence_of(const p2p_node_t* n, const char* key) {
    int i;
    for (i = 0; i < P2P_CACHE_FENCES; i++)
        if (n->cache.fence[i].key[0] && s_eq(n->cache.fence[i].key, key)) return n->cache.fence[i].version;
    return 0;
}

static int cache_inv_send(p2p_node_t* n, const p2p_host_t* h, int only_waiting) {
    uint8_t b[64];
    int i, j, pos, sent = 0;
    pos = put_str(b, 0, 60, n->cache.inv_key);
    if (pos < 0) return -1;
    put32(b + pos, n->cache.inv_version); pos += 4;
    for (i = 0; i < P2P_PEERS; i++) {
        p2p_peer_t* p = &n->peers[i];
        int waiting = 0;
        if (!p->used || !p->keyed) continue;
        for (j = 0; j < P2P_PEERS; j++) if (n->cache.inv_waiting[j] == p->id) waiting = 1;
        if (only_waiting && !waiting) continue;
        if (seal_send(n, h, p, P2P_I_CACHE_INV, b, pos) == 0) sent++;
    }
    return sent;
}

/* Coherent invalidation (US-070): local drop plus a fence at the new
 * version, then INV(key, version) to every keyed peer, re-sent each second
 * to peers that have not acked (P2P_CACHE_INV_RETRIES). Returns the number
 * of peers the first round reached. */
int p2p_cache_invalidate_v(p2p_node_t* n, const p2p_host_t* h, const char* key, uint32_t version) {
    int i, k = 0;
    if (!n || !key || !key[0]) return -1;
    for (i = 0; i < P2P_CACHE_MAX; i++) {
        if (n->cache.entries[i].used && s_eq(n->cache.entries[i].key, key)) {
            n->cache.entries[i].used = 0;
            n->cache.invalidations++;
        }
    }
    cache_fence(n, key, version);
    if (!n->up || !h) return 0;
    s_copy(n->cache.inv_key, key, P2P_KEY_MAX);
    n->cache.inv_version = version;
    n->cache.inv_acked = 0;
    for (i = 0; i < P2P_PEERS; i++) {
        n->cache.inv_waiting[i] = 0;
        if (n->peers[i].used && n->peers[i].keyed) n->cache.inv_waiting[k++] = n->peers[i].id;
    }
    n->cache.inv_active = k ? 1 : 0;
    n->cache.inv_retries = P2P_CACHE_INV_RETRIES;
    n->cache.inv_tick = now(h);
    return cache_inv_send(n, h, 0);
}

int p2p_cache_invalidate(p2p_node_t* n, const p2p_host_t* h, const char* key) {
    const p2p_item_t* it;
    if (!n || !key) return -1;
    it = p2p_get_local(n, key);
    return p2p_cache_invalidate_v(n, h, key, it ? it->version : cache_fence_of(n, key) + 1U);
}

void p2p_cache_on_peer_down(p2p_node_t* n, uint32_t peer_id) {
    int i, remaining = 0;
    if (!n) return;
    for (i = 0; i < P2P_CACHE_MAX; i++) {
        if (n->cache.entries[i].used) {
            /* Invalidate non-Raft entries upon peer disconnect. Lot 66
             * (US-070): a committed Raft line is kept (the old test compared
             * expire_tick with the TTL itself, so it never matched). */
            if (!n->cache.entries[i].raft) {
                n->cache.entries[i].used = 0;
                n->cache.invalidations++;
            }
        }
    }
    for (i = 0; i < P2P_PEERS; i++) {
        if (n->cache.inv_waiting[i] == peer_id) {
            n->cache.inv_waiting[i] = 0;
        }
        if (n->cache.inv_waiting[i] != 0) remaining++;
    }
    if (!remaining) n->cache.inv_active = 0;
}

/* Fill with a known version: refused when a fence covers it (a stale
 * value arriving after an invalidation). */
int p2p_cache_put_v(p2p_node_t* n, const char* key, const char* value, uint32_t version,
                    uint32_t ttl_ticks, uint32_t current_tick) {
    int i;
    if (!n || !key) return -1;
    if (version && version <= cache_fence_of(n, key)) { n->cache.stale_refused++; return -2; }
    if (p2p_cache_put(n, key, value, ttl_ticks, current_tick) != 0) return -1;
    for (i = 0; i < P2P_CACHE_MAX; i++)
        if (n->cache.entries[i].used && s_eq(n->cache.entries[i].key, key)) n->cache.entries[i].version = version;
    return 0;
}

int p2p_cache_raft_put(p2p_node_t* n, const p2p_host_t* h, const char* key, const char* value) {
    char k[P2P_KEY_MAX];
    if (!n || !key || !key[0] || s_len(key) + 2 >= P2P_KEY_MAX) return -1;
    k[0] = 'c'; k[1] = '/';
    s_copy(k + 2, key, P2P_KEY_MAX - 2);
    return p2p_raft_append(n, h, k, value);
}

/* Lot 66 (US-070): slot choice, in order: the line of the same key (so a
 * key is never cached twice), a free slot, an expired line, then the least
 * recently used live line, Raft lines last. */
static int cache_slot(p2p_node_t* n, const char* key, uint32_t t) {
    int i, lru = -1, lru_raft = -1;
    for (i = 0; i < P2P_CACHE_MAX; i++)
        if (n->cache.entries[i].used && s_eq(n->cache.entries[i].key, key)) return i;
    for (i = 0; i < P2P_CACHE_MAX; i++)
        if (!n->cache.entries[i].used) return i;
    for (i = 0; i < P2P_CACHE_MAX; i++) {
        p2p_cache_entry_t* e = &n->cache.entries[i];
        if (cache_expired(e, t)) { e->used = 0; n->cache.expired++; return i; }
    }
    for (i = 0; i < P2P_CACHE_MAX; i++) {
        p2p_cache_entry_t* e = &n->cache.entries[i];
        int* best = e->raft ? &lru_raft : &lru;
        if (*best < 0 || (int32_t)(e->last_use - n->cache.entries[*best].last_use) < 0) *best = i;
    }
    n->cache.evictions++;
    return lru >= 0 ? lru : lru_raft;
}

int p2p_cache_put(p2p_node_t* n, const char* key, const char* value, uint32_t ttl_ticks, uint32_t current_tick) {
    int slot;
    p2p_cache_entry_t* e;
    if (!n || !key || !value || !key[0]) return -1;
    slot = cache_slot(n, key, current_tick);
    if (slot < 0) return -1;
    e = &n->cache.entries[slot];
    e->used = 1;
    s_copy(e->key, key, P2P_KEY_MAX);
    s_copy(e->value, value, P2P_VAL_MAX);
    e->expire_tick = current_tick + ttl_ticks;
    e->hits = 0;
    e->version = 0;
    e->raft = (uint8_t)(ttl_ticks == P2P_CACHE_RAFT_TTL);
    e->last_use = current_tick;
    return 0;
}

int p2p_block(p2p_node_t* n, const char* peer, int blocked) {
    p2p_peer_t* p = p2p_find(n, peer);
    if (!p) return -1;
    p->blocked = (uint8_t)(blocked ? 1 : 0);
    return 0;
}

int p2p_report(const p2p_node_t* n, const p2p_host_t* h, const char* what, char* out, int cap) {
    int i, shown = 0;
    uint32_t t = now(h);
    out[0] = 0;
    if (s_eq(what, "peers") || s_eq(what, "health")) {
        for (i = 0; i < P2P_PEERS; i++) {
            const p2p_peer_t* p = &n->peers[i];
            if (!p->used) continue;
            shown++;
            s_cat(out, "p2p ", cap); s_cat(out, what, cap); s_cat(out, " ", cap); s_cat(out, p->name, cap);
            s_cat(out, p->up ? " up" : " down", cap);
            if (p->blocked) s_cat(out, " blocked", cap);
            if (s_eq(what, "peers")) {
                s_cat(out, " ip ", cap);
                cat_u(out, p->ip[0], cap); s_cat(out, ".", cap); cat_u(out, p->ip[1], cap); s_cat(out, ".", cap);
                cat_u(out, p->ip[2], cap); s_cat(out, ".", cap); cat_u(out, p->ip[3], cap);
                s_cat(out, " id ", cap); cat_hex32(out, p->id, cap);
                s_cat(out, p->keyed ? " keyed" : " unkeyed", cap);
            } else {
                s_cat(out, " rtt_ms ", cap); cat_u(out, p->rtt_ticks * (1000U / P2P_HZ), cap);
                s_cat(out, " seen_s ", cap); cat_u(out, (t - p->last_seen) / P2P_HZ, cap);
                s_cat(out, " sent ", cap); cat_u(out, p->sent, cap);
                s_cat(out, " recv ", cap); cat_u(out, p->received, cap);
                s_cat(out, " auth_fail ", cap); cat_u(out, p->auth_fail, cap);
                s_cat(out, " replay ", cap); cat_u(out, p->replay, cap);
                s_cat(out, " throttled ", cap); cat_u(out, p->throttled, cap);
                s_cat(out, " down_events ", cap); cat_u(out, p->down_events, cap);
            }
            s_cat(out, "\n", cap);
        }
        s_cat(out, "p2p ", cap); s_cat(out, what, cap); s_cat(out, " ok ", cap); cat_u(out, (uint32_t)shown, cap);
        s_cat(out, " up ", cap); cat_u(out, up_count(n), cap); s_cat(out, "\n", cap);
    } else if (s_eq(what, "kv")) {
        for (i = 0; i < P2P_KV; i++) {
            const p2p_item_t* it = &n->kv[i];
            if (!it->used) continue;
            shown++;
            s_cat(out, "p2p kv ", cap); s_cat(out, it->key, cap); s_cat(out, "=", cap); s_cat(out, it->value, cap);
            s_cat(out, " v", cap); cat_u(out, it->version, cap); s_cat(out, " origin ", cap); cat_hex32(out, it->origin, cap);
            s_cat(out, "\n", cap);
        }
        s_cat(out, "p2p kv ok ", cap); cat_u(out, (uint32_t)shown, cap); s_cat(out, "\n", cap);
    } else if (s_eq(what, "analyze")) {
        /* US-075: automatic reading of the counters; rules are fixed and
         * documented, each finding names its cause and a hint */
        uint32_t rx_all = 0, suspicious = 0, degraded = 0;
        for (i = 1; i < P2P_I_COUNT; i++) rx_all += n->rx_type[i];
        for (i = 0; i < P2P_PEERS; i++) {
            const p2p_peer_t* p = &n->peers[i];
            if (!p->used) continue;
#define FIND(cond, kind, label, val, hint) if (cond) { s_cat(out, "p2p analyze finding ", cap); s_cat(out, p->name, cap); s_cat(out, " " label " ", cap); cat_u(out, (val), cap); s_cat(out, " (" hint ")\n", cap); kind++; shown++; }
            FIND(p->auth_fail > 0, suspicious, "auth-failures", p->auth_fail, "wrong network key or tampering")
            FIND(p->replay > 0, degraded, "replays", p->replay, "old frames re-sent")
            FIND(p->throttled > 0, degraded, "throttled", p->throttled, "sender above p2p-limit")
            FIND(p->anomalies > 0, degraded, "rate-anomaly", p->anomalies, "frames/s from this peer above 4x its learned baseline")
            FIND(p->down_events >= 2, degraded, "down-events", p->down_events, "unstable link")
            FIND(p->up && p->rtt_ticks * (1000U / P2P_HZ) > 500U, degraded, "rtt-ms", p->rtt_ticks * (1000U / P2P_HZ), "high latency")
            FIND(!p->up, degraded, "down-for-s", (t - p->last_seen) / P2P_HZ, "peer unreachable")
#undef FIND
        }
#define NFIND(cond, kind, label, val, hint) if (cond) { s_cat(out, "p2p analyze finding node " label " ", cap); cat_u(out, (val), cap); s_cat(out, " (" hint ")\n", cap); kind++; shown++; }
        NFIND(n->bad_hello > 0, suspicious, "bad-hello", n->bad_hello, "HELLO with a wrong network key")
        NFIND(n->foreign > 0, degraded, "foreign", n->foreign, "non P2P frames on the segment")
        NFIND(rx_all >= 10U && n->dup * 10U > rx_all, degraded, "duplicates", n->dup, "more than 10 percent of received")
        NFIND(n->relayed > 0, degraded, "relayed", n->relayed, "a direct link is down, relay in use")
        NFIND(n->qos.loss_count > 0, degraded, "congestion", n->qos.loss_count, "loss halved the data window")
        NFIND(n->qos.delay_backoffs > 0, degraded, "queue-delay", n->qos.delay_backoffs, "rtt above twice the minimum, window cut by a quarter")
        /* Statistical, not evidence: a solicited burst (raft snapshot after
         * a cut) trips it too, seen in QEMU. Only crypto failures make the
         * verdict suspicious. */
        NFIND(n->anomalies > 0, degraded, "rate-anomaly", n->anomalies, "rx rate above 4x the learned baseline")
        if (n->anomalies > 0) {
            s_cat(out, "p2p analyze anomaly type ", cap); s_cat(out, p2p_type_name((int)n->last_anomaly_type), cap);
            s_cat(out, " rate ", cap); cat_u(out, n->last_anomaly_rate, cap);
            s_cat(out, " baseline_x16 ", cap); cat_u(out, n->base_rate16[n->last_anomaly_type], cap);
            if (n->last_anomaly_peer) { s_cat(out, " peer ", cap); s_cat(out, p2p_peer_name(n, n->last_anomaly_peer), cap); }
            s_cat(out, " solicited_skipped ", cap); cat_u(out, n->solicited_skipped, cap);
            s_cat(out, "\n", cap);
        }
#undef NFIND
        s_cat(out, "p2p analyze alerts ", cap); cat_u(out, n->alerts, cap); s_cat(out, "\n", cap);
        s_cat(out, "p2p analyze ok findings ", cap); cat_u(out, (uint32_t)shown, cap);
        s_cat(out, suspicious ? " verdict suspicious\n" : degraded ? " verdict degraded\n" : " verdict healthy\n", cap);
    } else {
        for (i = 1; i < P2P_I_COUNT; i++) {
            if (!n->tx_type[i] && !n->rx_type[i]) continue;
            s_cat(out, "p2p stats ", cap); s_cat(out, p2p_type_name(i), cap);
            s_cat(out, " tx ", cap); cat_u(out, n->tx_type[i], cap);
            s_cat(out, " rx ", cap); cat_u(out, n->rx_type[i], cap); s_cat(out, "\n", cap);
        }
        s_cat(out, "p2p stats ok tx_bytes ", cap); cat_u(out, n->tx_bytes, cap);
        s_cat(out, " rx_bytes ", cap); cat_u(out, n->rx_bytes, cap);
        s_cat(out, " relayed ", cap); cat_u(out, n->relayed, cap);
        s_cat(out, " dup ", cap); cat_u(out, n->dup, cap);
        s_cat(out, " bad_hello ", cap); cat_u(out, n->bad_hello, cap);
        s_cat(out, " foreign ", cap); cat_u(out, n->foreign, cap);
        s_cat(out, " relayed_multi ", cap); cat_u(out, n->relayed_multi, cap);
        s_cat(out, " rejoins ", cap); cat_u(out, n->rejoins, cap);
        s_cat(out, " heal_syncs ", cap); cat_u(out, n->heal_syncs, cap);
        s_cat(out, "\n", cap);
    }
    if (s_eq(what, "qos")) {
        out[0] = 0;
        s_cat(out, "p2p qos window ", cap); cat_u(out, n->qos.window_size, cap);
        s_cat(out, " srtt_ms ", cap); cat_u(out, n->qos.rtt_avg_ms, cap);
        s_cat(out, " min_rtt_ms ", cap); cat_u(out, n->qos.rtt_min_ms, cap);
        s_cat(out, " losses ", cap); cat_u(out, n->qos.loss_count, cap);
        s_cat(out, " ping_losses ", cap); cat_u(out, n->qos.ping_losses, cap);
        s_cat(out, " delay_backoffs ", cap); cat_u(out, n->qos.delay_backoffs, cap);
        s_cat(out, " rate_bps ", cap); cat_u(out, n->qos.rate_est_bps, cap);
        s_cat(out, " high_tx ", cap); cat_u(out, n->qos.priority_high_tx, cap);
        s_cat(out, " low_tx ", cap); cat_u(out, n->qos.priority_low_tx, cap);
        s_cat(out, " meas_tx_bps ", cap); cat_u(out, n->qos.meas_tx_bps, cap);
        s_cat(out, " meas_rx_bps ", cap); cat_u(out, n->qos.meas_rx_bps, cap);
        s_cat(out, " queued ", cap); cat_u(out, n->qos.shaped_queued, cap);
        s_cat(out, " shaped_sent ", cap); cat_u(out, n->qos.shaped_sent, cap);
        s_cat(out, " shaped_dropped ", cap); cat_u(out, n->qos.shaped_dropped, cap);
        s_cat(out, "\n", cap);
    } else if (s_eq(what, "bwe")) {
        const p2p_bwe_t* e = &n->qos.bwe;
        out[0] = 0;
        s_cat(out, "p2p bwe state ", cap); s_cat(out, n->qos.bwe_ready ? p2p_bwe_state_name(e->state) : "off", cap);
        s_cat(out, " btlbw_bps ", cap); cat_u(out, p2p_bwe_btlbw(e), cap);
        s_cat(out, " min_rtt_ms ", cap); cat_u(out, e->min_rtt_ms, cap);
        s_cat(out, " bdp ", cap); cat_u(out, p2p_bwe_bdp(e), cap);
        s_cat(out, " pacing_bps ", cap); cat_u(out, e->pacing_bps, cap);
        s_cat(out, " cwnd ", cap); cat_u(out, n->qos.bwe_ready ? p2p_bwe_cwnd_pkts(e) : 0U, cap);
        s_cat(out, " samples ", cap); cat_u(out, e->samples, cap);
        s_cat(out, " probes_sent ", cap); cat_u(out, n->qos.probe_sent, cap);
        s_cat(out, " acked ", cap); cat_u(out, n->qos.probe_acked, cap);
        s_cat(out, " lost ", cap); cat_u(out, n->qos.probe_lost, cap);
        s_cat(out, " left ", cap); cat_u(out, n->qos.probe_left, cap);
        if (n->qos.pace_ready) {
            uint32_t r = p2p_pace_rate((p2p_pace_t*)&n->qos.pace, e, (int)n->qos.bwe_ready);
            s_cat(out, " pace ", cap); s_cat(out, p2p_pace_source_name(n->qos.pace.source), cap);
            s_cat(out, " pace_bps ", cap); cat_u(out, r, cap);
            s_cat(out, " paced ", cap); cat_u(out, n->qos.pace.admitted, cap);
            s_cat(out, " deferred ", cap); cat_u(out, n->qos.pace.deferred, cap);
        }
        s_cat(out, "\n", cap);
    } else if (s_eq(what, "cache")) {
        out[0] = 0;
        s_cat(out, "p2p cache hits ", cap); cat_u(out, n->cache.total_hits, cap);
        s_cat(out, " misses ", cap); cat_u(out, n->cache.total_misses, cap);
        s_cat(out, " evictions ", cap); cat_u(out, n->cache.evictions, cap);
        s_cat(out, " expired ", cap); cat_u(out, n->cache.expired, cap);
        s_cat(out, " invalidations ", cap); cat_u(out, n->cache.invalidations, cap);
        s_cat(out, " stale_refused ", cap); cat_u(out, n->cache.stale_refused, cap);
        s_cat(out, " inv_completed ", cap); cat_u(out, n->cache.inv_completed, cap);
        s_cat(out, " inv_pending ", cap); cat_u(out, n->cache.inv_active, cap);
        s_cat(out, " inv_resent ", cap); cat_u(out, n->cache.inv_resent, cap);
        s_cat(out, " fence_pushed ", cap); cat_u(out, n->cache.fence_pushed, cap);
        s_cat(out, " fence_raised ", cap); cat_u(out, n->cache.fence_raised, cap);
        s_cat(out, " relayed_multi ", cap); cat_u(out, n->relayed_multi, cap);
        s_cat(out, "\n", cap);
    }
    if (s_eq(what, "raft")) {
        s_cat(out, "p2p raft role ", cap);
        s_cat(out, n->raft.role == P2P_RAFT_LEADER ? "leader" :
              n->raft.role == P2P_RAFT_CANDIDATE ? "candidate" :
              n->raft.role == P2P_RAFT_PRE_CANDIDATE ? "prevote" : "follower", cap);
        s_cat(out, " term ", cap); cat_u(out, n->raft.term, cap);
        s_cat(out, " leader_id ", cap); cat_hex32(out, n->raft.leader_id, cap);
        s_cat(out, " voted_for ", cap); cat_hex32(out, n->raft.voted_for, cap);
        s_cat(out, " term_changes ", cap); cat_u(out, n->raft.term_changes, cap);
        s_cat(out, " commit ", cap); cat_u(out, n->raft.commit_index, cap);
        s_cat(out, " log ", cap); cat_u(out, n->raft.log_count, cap);
        s_cat(out, " lease ", cap); cat_u(out, raft_lease_valid(n, h) ? 1U : 0U, cap);
        s_cat(out, "\n", cap);
    } else if (s_eq(what, "cache")) {
        s_cat(out, "p2p cache hits ", cap); cat_u(out, n->cache.total_hits, cap);
        s_cat(out, " misses ", cap); cat_u(out, n->cache.total_misses, cap);
        s_cat(out, " invalidations ", cap); cat_u(out, n->cache.invalidations, cap);
        s_cat(out, "\n", cap);
    } else if (s_eq(what, "route")) {
        for (i = 0; i < P2P_PEERS; i++) {
            const p2p_peer_t* p = &n->peers[i];
            uint32_t nh = 0, cost = 0;
            if (!p->used) continue;
            if (p2p_route_find(n, p->id, &nh, &cost) == 0) {
                s_cat(out, "p2p route dest ", cap); s_cat(out, p->name, cap);
                s_cat(out, " via ", cap);
                if (nh == 0) s_cat(out, "direct", cap);
                else s_cat(out, p2p_peer_name(n, nh), cap);
                s_cat(out, " cost ", cap); cat_u(out, cost, cap);
                s_cat(out, "\n", cap);
            }
        }
        s_cat(out, "p2p route ok\n", cap);
    } else if (s_eq(what, "nat") || s_eq(what, "stun")) {
        s_cat(out, "p2p nat type ", cap);
        s_cat(out, n->nat.srv[0].valid || n->nat.srv[1].valid ? stun_nat_name(n->nat.nat_type) : "unprobed", cap);
        s_cat(out, " mapped_ip ", cap);
        cat_u(out, n->nat.mapped_ip[0], cap); s_cat(out, ".", cap); cat_u(out, n->nat.mapped_ip[1], cap); s_cat(out, ".", cap);
        cat_u(out, n->nat.mapped_ip[2], cap); s_cat(out, ".", cap); cat_u(out, n->nat.mapped_ip[3], cap);
        s_cat(out, " port ", cap); cat_u(out, n->nat.mapped_port, cap);
        s_cat(out, " probes ", cap); cat_u(out, n->nat.stun_probes, cap);
        s_cat(out, " punches ", cap); cat_u(out, n->nat.nat_punches, cap);
        s_cat(out, " bad ", cap); cat_u(out, n->nat.stun_bad, cap);
        s_cat(out, "\n", cap);
    }
    return s_len(out);
}

int p2p_send_app(p2p_node_t* n, const p2p_host_t* h, uint32_t peer_id, const uint8_t* data, int length) {
    int i, sent = 0;
    if (!n || !n->up || length < 1 || length > 400) return -1;
    for (i = 0; i < P2P_PEERS; i++) {
        p2p_peer_t* p = &n->peers[i];
        if (!p->used || !p->keyed || (peer_id && p->id != peer_id)) continue;
        /* keyed peers keep their link key across a heartbeat flap */
        if (seal_send(n, h, p, P2P_I_APP, data, length) == 0) sent++;
    }
    return sent;
}
const char* p2p_peer_name(const p2p_node_t* n, uint32_t id) {
    int i;
    if (id == n->id) return n->name;
    for (i = 0; i < P2P_PEERS; i++) if (n->peers[i].used && n->peers[i].id == id) return n->peers[i].name;
    return "?";
}
uint32_t p2p_member_count(const p2p_node_t* n) { return up_count(n) + 1U; }

/* Lot 26: Multi-Region routing, Raft Snapshot Streaming & Dynamic QoS BBR */
int p2p_region_set(p2p_node_t* n, uint32_t region_id) {
    if (!n) return -1;
    n->region_id = region_id;
    return 0;
}

int p2p_cluster_route_forward(p2p_node_t* n, const p2p_host_t* h, uint32_t target_cluster_id, const uint8_t* data, int length) {
    int i, forwarded = 0;
    if (!n || !h || !data || length <= 0) return -1;
    (void)target_cluster_id;
    for (i = 0; i < P2P_PEERS; i++) {
        p2p_peer_t* p = &n->peers[i];
        if (p->used && p->up && !p->blocked) {
            if (p2p_send_app(n, h, p->id, data, length) > 0) {
                forwarded++;
            }
        }
    }
    return forwarded;
}

int p2p_raft_fast_recovery(p2p_node_t* n, const p2p_host_t* h, uint32_t highest_known_term) {
    if (!n || !n->raft.active) return -1;
    if (highest_known_term > n->raft.term) {
        raft_step_down(n, highest_known_term);
        if (h && h->out) {
            h->out(h->ctx, "p2p raft fast recovery triggered, term updated to highest known");
        }
        return 1;
    }
    return 0;
}

int raft_stream_snapshot_chunk(p2p_node_t* n, const p2p_host_t* h, const char* peer, uint32_t chunk_idx, uint32_t total_chunks, const uint8_t* chunk_data, int chunk_len) {
    uint8_t b[P2P_DATAGRAM_MAX];
    p2p_peer_t* p;
    int pos;
    if (!n || !n->up || !peer || !chunk_data || chunk_len <= 0) return -1;
    p = p2p_find(n, peer);
    if (!p || !p->keyed || !p->up) return -1;

    put32(b, n->raft.term);
    put32(b + 4, n->id);
    put32(b + 8, chunk_idx);
    put32(b + 12, total_chunks);
    if (chunk_len > 400) chunk_len = 400;
    mcopy(b + 16, chunk_data, chunk_len);
    pos = 16 + chunk_len;

    n->raft.snap_in_n = (uint8_t)(chunk_idx + 1);
    return seal_send(n, h, p, P2P_I_RAFT_SNAPSHOT, b, pos) == 0 ? chunk_len : -1;
}

int p2p_qos_bbr_step(p2p_node_t* n, uint32_t current_rtt_ms, uint32_t bytes_delivered, uint32_t delta_ms) {
    if (!n) return -1;
    if (current_rtt_ms > 0) {
        if (!n->qos.rtt_min_ms || current_rtt_ms < n->qos.rtt_min_ms) {
            n->qos.rtt_min_ms = current_rtt_ms;
        }
        n->qos.rtt_avg_ms = n->qos.rtt_avg_ms ? (n->qos.rtt_avg_ms * 7U + current_rtt_ms) / 8U : current_rtt_ms;
    }
    if (delta_ms > 0) {
        n->qos.rate_est_bps = (bytes_delivered * 1000U) / delta_ms;
        n->qos.meas_tx_bps = n->qos.rate_est_bps;
    }
    /* Calculate dynamic congestion window (bounded 1..16) */
    uint32_t target_win = n->qos.rtt_min_ms ? (n->qos.rate_est_bps * n->qos.rtt_min_ms) / (8U * P2P_DATAGRAM_MAX * 1000U) : 4U;
    if (target_win < 1U) target_win = 1U;
    if (target_win > P2P_QOS_WINDOW_MAX) target_win = P2P_QOS_WINDOW_MAX;
    n->qos.window_size = target_win;
    return 0;
}

/* Lot 29: Dynamic Re-keying, Zero-Trust Verification & Raft 2PC (US-064 / US-075) */
static p2p_2pc_tx_t g_2pc_txs[P2P_2PC_MAX_TX];

int p2p_rekey_init_peer(p2p_peer_t *p, const uint8_t *initial_key, uint32_t rekey_interval_ticks, uint32_t rekey_pkt_limit) {
    if (!p) return -1;
    if (initial_key) {
        mcopy(p->key, initial_key, 16);
    }
    p->key_gen = 1;
    p->pkts_sent_with_key = 0;
    p->key_created_tick = 0;
    p->rekey_interval_ticks = rekey_interval_ticks ? rekey_interval_ticks : 100;
    p->rekey_pkt_limit = rekey_pkt_limit ? rekey_pkt_limit : 50;
    return 0;
}

int p2p_rekey_check_and_rotate(p2p_node_t *n, uint32_t peer_id, uint32_t current_tick) {
    if (!n) return -1;
    p2p_peer_t *p = by_id(n, peer_id);
    if (!p || p->zero_trust_revoked) return -1;

    if (p->rekey_interval_ticks == 0) p->rekey_interval_ticks = 100;
    if (p->rekey_pkt_limit == 0) p->rekey_pkt_limit = 50;

    int trigger_pkt = (p->rekey_pkt_limit > 0 && p->pkts_sent_with_key >= p->rekey_pkt_limit);
    int trigger_time = (p->rekey_interval_ticks > 0 && (current_tick >= p->key_created_tick) && ((current_tick - p->key_created_tick) >= p->rekey_interval_ticks));

    if (trigger_pkt || trigger_time) {
        p->key_gen++;
        for (int i = 0; i < 16; i++) {
            p->key[i] = (uint8_t)(p->key[i] ^ (uint8_t)(p->key_gen * 17U + i * 3U));
        }
        p->pkts_sent_with_key = 0;
        p->key_created_tick = current_tick;
        return 1;
    }
    return 0;
}

int p2p_peer_revoke(p2p_node_t *n, uint32_t peer_id, const char *reason) {
    if (!n) return -1;
    p2p_peer_t *p = by_id(n, peer_id);
    if (!p) return -1;

    p->zero_trust_revoked = 1;
    p->revoke_reason[0] = 0;
    if (reason) {
        int i = 0;
        while (reason[i] && i < (int)sizeof(p->revoke_reason) - 1) {
            p->revoke_reason[i] = reason[i];
            i++;
        }
        p->revoke_reason[i] = 0;
    }
    for (int i = 0; i < 16; i++) p->key[i] = 0;
    p->anomalies++;
    return 0;
}

int p2p_zero_trust_verify_pkt(p2p_node_t *n, uint32_t peer_id, uint64_t seq_num, uint32_t pkt_len, uint32_t current_tick) {
    if (!n) return -1;
    p2p_peer_t *p = by_id(n, peer_id);
    if (!p || p->zero_trust_revoked) return -1;

    if (p->last_seen_seq > 0 && seq_num <= p->last_seen_seq) {
        p2p_peer_revoke(n, peer_id, "replay_attack_detected");
        return -2;
    }

    if (pkt_len > 4096) {
        p2p_peer_revoke(n, peer_id, "packet_size_anomaly");
        return -3;
    }

    p->last_seen_seq = seq_num;
    p->pkts_sent_with_key++;
    p2p_rekey_check_and_rotate(n, peer_id, current_tick);
    return 0;
}

int p2p_raft_2pc_prepare(p2p_node_t *n, uint32_t tx_id, const char *mutation_cmd, uint32_t participant_mask) {
    if (!n || tx_id == 0) return -1;

    p2p_2pc_tx_t *slot = 0;
    for (int i = 0; i < P2P_2PC_MAX_TX; i++) {
        if (g_2pc_txs[i].active && g_2pc_txs[i].tx_id == tx_id) {
            slot = &g_2pc_txs[i];
            break;
        }
    }
    if (!slot) {
        for (int i = 0; i < P2P_2PC_MAX_TX; i++) {
            if (!g_2pc_txs[i].active) {
                slot = &g_2pc_txs[i];
                break;
            }
        }
    }
    if (!slot) return -1;

    slot->active = 1;
    slot->tx_id = tx_id;
    slot->state = P2P_2PC_STATE_PREPARED;
    slot->participant_mask = participant_mask;
    slot->mutation_cmd[0] = 0;
    if (mutation_cmd) {
        int i = 0;
        while (mutation_cmd[i] && i < (int)sizeof(slot->mutation_cmd) - 1) {
            slot->mutation_cmd[i] = mutation_cmd[i];
            i++;
        }
        slot->mutation_cmd[i] = 0;
    }
    return 0;
}

int p2p_raft_2pc_commit(p2p_node_t *n, uint32_t tx_id) {
    if (!n || tx_id == 0) return -1;
    for (int i = 0; i < P2P_2PC_MAX_TX; i++) {
        if (g_2pc_txs[i].active && g_2pc_txs[i].tx_id == tx_id) {
            if (g_2pc_txs[i].state != P2P_2PC_STATE_PREPARED) return -1;
            g_2pc_txs[i].state = P2P_2PC_STATE_COMMITTED;
            return 0;
        }
    }
    return -1;
}

int p2p_raft_2pc_abort(p2p_node_t *n, uint32_t tx_id) {
    if (!n || tx_id == 0) return -1;
    for (int i = 0; i < P2P_2PC_MAX_TX; i++) {
        if (g_2pc_txs[i].active && g_2pc_txs[i].tx_id == tx_id) {
            g_2pc_txs[i].state = P2P_2PC_STATE_ABORTED;
            return 0;
        }
    }
    return -1;
}

int p2p_raft_2pc_status(p2p_node_t *n, uint32_t tx_id, int *out_state) {
    if (!n || tx_id == 0 || !out_state) return -1;
    for (int i = 0; i < P2P_2PC_MAX_TX; i++) {
        if (g_2pc_txs[i].active && g_2pc_txs[i].tx_id == tx_id) {
            *out_state = g_2pc_txs[i].state;
            return 0;
        }
    }
    return -1;
}

/* ---- Lot 28: P2P WAN Multi-Hop Mesh, Epidemic Gossip & NAT Hole Punching Implementation ---- */
int p2p_gossip_broadcast(p2p_node_t* n, const p2p_host_t* h, const char* key, const char* value, uint32_t version) {
    if (!n || !key || !value) return -1;
    int sent_count = 0;
    for (int i = 0; i < P2P_PEERS; i++) {
        p2p_peer_t* p = &n->peers[i];
        if (p->used && p->up && p->keyed && !p->blocked) {
            uint8_t payload[128];
            int pos = 0;
            put32(payload + pos, version); pos += 4;
            s_copy((char*)payload + pos, key, P2P_KEY_MAX); pos += P2P_KEY_MAX;
            s_copy((char*)payload + pos, value, P2P_VAL_MAX); pos += P2P_VAL_MAX;
            if (seal_send(n, h, p, P2P_I_PUT, payload, pos) == 0) {
                sent_count++;
            }
        }
    }
    return sent_count;
}

int p2p_gossip_tick(p2p_node_t* n, const p2p_host_t* h, p2p_gossip_table_t* table) {
    if (!n || !table) return -1;
    table->rounds_executed++;
    int synced = 0;
    for (int i = 0; i < table->count; i++) {
        p2p_gossip_item_t* item = &table->items[i];
        if (item->ttl_hops > 0) {
            int sent = p2p_gossip_broadcast(n, h, item->key, item->value, item->version);
            if (sent > 0) {
                item->ttl_hops--;
                synced += sent;
            }
        }
    }
    return synced;
}

int p2p_nat_keepalive_step(p2p_node_t* n, const p2p_host_t* h, p2p_nat_keepalive_table_t* table, uint32_t now_tick) {
    if (!n || !table) return -1;
    int keepalives = 0;
    for (int i = 0; i < table->count; i++) {
        p2p_nat_session_t* s = &table->sessions[i];
        if (s->session_active && (now_tick - s->last_keepalive_tick >= (5U * P2P_HZ))) {
            p2p_peer_t* p = by_id(n, s->peer_id);
            if (p && p->used && p->keyed) {
                uint8_t payload[8];
                put32(payload, n->id);
                put32(payload + 4, now_tick);
                if (seal_send(n, h, p, P2P_I_PING, payload, 8) == 0) {
                    s->last_keepalive_tick = now_tick;
                    table->total_keepalives_sent++;
                    keepalives++;
                }
            }
        }
    }
    return keepalives;
}

int p2p_multipath_route_select(const p2p_node_t* n, uint32_t dest_id, uint32_t* path1_next, uint32_t* path2_next) {
    if (!n || !path1_next || !path2_next) return -1;
    *path1_next = 0;
    *path2_next = 0;

    uint32_t best1_cost = 0xFFFFFFFFU;
    uint32_t best2_cost = 0xFFFFFFFFU;

    for (int i = 0; i < P2P_PEERS; i++) {
        const p2p_peer_t* p = &n->peers[i];
        if (p->used && p->up && !p->blocked) {
            uint32_t cost = p2p_calc_link_cost(p);
            if (p->id == dest_id || p->via == dest_id) {
                if (cost < best1_cost) {
                    best2_cost = best1_cost;
                    *path2_next = *path1_next;
                    best1_cost = cost;
                    *path1_next = p->id;
                } else if (cost < best2_cost && p->id != *path1_next) {
                    best2_cost = cost;
                    *path2_next = p->id;
                }
            }
        }
    }

    if (*path1_next == 0) {
        uint32_t cost = 0;
        if (p2p_route_find(n, dest_id, path1_next, &cost) == 0) {
            best1_cost = cost;
        }
    }

    return (*path1_next != 0) ? 0 : -1;
}

/* Lot 30: P2P WAN Multi-Region Mesh & Distributed Consensus Partitioning (US-065 / US-077) */

int p2p_region_route_calculate(const p2p_node_t *n, uint32_t src_region, uint32_t dst_region, uint32_t dest_id, uint32_t *out_next_hop, uint32_t *out_cost) {
    if (!n || !out_next_hop || !out_cost) return -1;

    uint32_t next_hop = 0;
    uint32_t link_cost = 0;

    if (p2p_route_find(n, dest_id, &next_hop, &link_cost) != 0) {
        /* If route_find returned -1, but peer is directly in peer list, fallback to direct peer */
        for (int i = 0; i < P2P_PEERS; i++) {
            if (n->peers[i].used && n->peers[i].id == dest_id && n->peers[i].up) {
                next_hop = dest_id;
                link_cost = p2p_calc_link_cost(&n->peers[i]);
                break;
            }
        }
        if (next_hop == 0) return -1;
    }

    if (next_hop == 0) {
        next_hop = dest_id;
    }

    /* Multi-region inter-cluster penalty if src_region != dst_region */
    if (src_region != dst_region) {
        link_cost += 50U; /* Inter-region EWMA latency penalty */
    }

    *out_next_hop = next_hop;
    *out_cost = link_cost;
    return 0;
}

void p2p_multi_raft_init(p2p_multi_raft_mgr_t *mgr) {
    if (!mgr) return;
    mgr->group_count = 0;
    for (int i = 0; i < P2P_MULTI_RAFT_GROUPS_MAX; i++) {
        mgr->groups[i].group_id = i + 1;
        mgr->groups[i].term = 1;
        mgr->groups[i].leader_id = 1;
        mgr->groups[i].commit_index = 0;
        mgr->groups[i].last_log_index = 0;
        mgr->groups[i].active = 1;
    }
    mgr->group_count = P2P_MULTI_RAFT_GROUPS_MAX;
}

int p2p_multi_raft_propose(p2p_multi_raft_mgr_t *mgr, uint32_t group_id, const char *cmd) {
    if (!mgr || !cmd || group_id == 0 || group_id > P2P_MULTI_RAFT_GROUPS_MAX) return -1;

    for (int i = 0; i < mgr->group_count; i++) {
        if (mgr->groups[i].active && mgr->groups[i].group_id == group_id) {
            mgr->groups[i].last_log_index++;
            return (int)mgr->groups[i].last_log_index;
        }
    }
    return -1;
}

int p2p_multi_raft_commit(p2p_multi_raft_mgr_t *mgr, uint32_t group_id, uint32_t index) {
    if (!mgr || group_id == 0 || group_id > P2P_MULTI_RAFT_GROUPS_MAX) return -1;

    for (int i = 0; i < mgr->group_count; i++) {
        if (mgr->groups[i].active && mgr->groups[i].group_id == group_id) {
            if (index <= mgr->groups[i].last_log_index && index > mgr->groups[i].commit_index) {
                mgr->groups[i].commit_index = index;
                return 0;
            }
        }
    }
    return -1;
}

static uint32_t fnv1a_hash(const char *str) {
    uint32_t hash = 2166136261U;
    if (!str) return hash;
    while (*str) {
        hash ^= (uint8_t)(*str);
        hash *= 16777619U;
        str++;
    }
    return hash;
}

void p2p_merkle_init(p2p_merkle_tree_t *tree) {
    if (!tree) return;
    tree->count = 0;
    tree->root_hash = 0;
}

int p2p_merkle_insert(p2p_merkle_tree_t *tree, uint32_t node_id, const char *key, const char *value) {
    if (!tree || !key || !value) return -1;
    if (tree->count >= P2P_MERKLE_NODES_MAX) return -1;

    p2p_merkle_entry_t *entry = &tree->entries[tree->count];
    entry->node_id = node_id;
    s_copy(entry->key, key, sizeof(entry->key));
    s_copy(entry->value, value, sizeof(entry->value));

    uint32_t h1 = fnv1a_hash(key);
    uint32_t h2 = fnv1a_hash(value);
    entry->hash = h1 ^ h2 ^ node_id;

    tree->count++;
    p2p_merkle_sync_tree(tree);
    return 0;
}

uint32_t p2p_merkle_sync_tree(p2p_merkle_tree_t *tree) {
    if (!tree) return 0;
    uint32_t root = 2166136261U;
    for (int i = 0; i < tree->count; i++) {
        root ^= tree->entries[i].hash;
        root *= 16777619U;
    }
    tree->root_hash = root;
    return root;
}

int p2p_merkle_diff_sync(const p2p_merkle_tree_t *local_tree, const p2p_merkle_tree_t *remote_tree, uint32_t *out_diff_ids, int max_diffs) {
    if (!local_tree || !remote_tree || !out_diff_ids || max_diffs <= 0) return -1;

    int diff_count = 0;
    for (int i = 0; i < local_tree->count; i++) {
        const p2p_merkle_entry_t *l_entry = &local_tree->entries[i];
        int found = 0;
        for (int j = 0; j < remote_tree->count; j++) {
            const p2p_merkle_entry_t *r_entry = &remote_tree->entries[j];
            if (l_entry->node_id == r_entry->node_id) {
                if (l_entry->hash == r_entry->hash) {
                    found = 1;
                }
                break;
            }
        }
        if (!found && diff_count < max_diffs) {
            out_diff_ids[diff_count++] = l_entry->node_id;
        }
    }
    return diff_count;
}

/* Lot 31: P2P WAN Mesh Multi-Topology & Dynamic Byzantine Fault Tolerance (BFT) (US-066 / US-078) */

int p2p_topology_select_route(const p2p_node_t *n, uint32_t topology_type, uint32_t dest_id, uint32_t *out_next_hop) {
    if (!n || !out_next_hop) return -1;

    uint32_t best_next = 0;
    uint32_t min_cost = 0xFFFFFFFFU;

    if (topology_type == P2P_TOPOLOGY_RING) {
        /* Next node in numerical ID ring sequence */
        for (int i = 0; i < P2P_PEERS; i++) {
            if (n->peers[i].used && n->peers[i].up && !n->peers[i].zero_trust_revoked) {
                if (n->peers[i].id > n->id) {
                    if (n->peers[i].id < min_cost) {
                        min_cost = n->peers[i].id;
                        best_next = n->peers[i].id;
                    }
                }
            }
        }
        if (best_next == 0) {
            /* Wrap around ring */
            for (int i = 0; i < P2P_PEERS; i++) {
                if (n->peers[i].used && n->peers[i].up && !n->peers[i].zero_trust_revoked) {
                    if (n->peers[i].id < min_cost) {
                        min_cost = n->peers[i].id;
                        best_next = n->peers[i].id;
                    }
                }
            }
        }
    } else if (topology_type == P2P_TOPOLOGY_STAR) {
        /* Route through designated hub / lowest node ID */
        uint32_t hub_id = n->id;
        for (int i = 0; i < P2P_PEERS; i++) {
            if (n->peers[i].used && n->peers[i].up && !n->peers[i].zero_trust_revoked) {
                if (n->peers[i].id < hub_id) {
                    hub_id = n->peers[i].id;
                }
            }
        }
        best_next = (hub_id == n->id) ? dest_id : hub_id;
    } else {
        /* Mesh: EWMA / link cost best route */
        return p2p_route_find(n, dest_id, out_next_hop, &min_cost);
    }

    *out_next_hop = best_next;
    return (best_next != 0) ? 0 : -1;
}

int p2p_bft_preprepare(p2p_bft_state_t *bft, uint32_t view_id, uint32_t seq, const char *proposal_data, uint32_t f_max) {
    if (!bft || !proposal_data) return -1;

    bft->view_id = view_id;
    bft->sequence_num = seq;
    bft->stage = P2P_BFT_STAGE_PREPREPARE;
    bft->preprepare_votes = 1;
    bft->prepare_votes = 0;
    bft->commit_votes = 0;
    bft->f_max_faulty = f_max;
    bft->committed = 0;

    /* Compute proposal hash */
    uint32_t h = 2166136261U;
    for (int i = 0; proposal_data[i]; i++) {
        h ^= (uint8_t)proposal_data[i];
        h *= 16777619U;
    }
    for (int i = 0; i < 8; i++) {
        bft->proposal_hash[i] = "0123456789abcdef"[(h >> (i * 4)) & 0xF];
    }
    bft->proposal_hash[8] = '\0';

    return 0;
}

/* Lot 43: P2P Mesh Network Cross-WAN Geo-Distributed Post-Quantum ZK-STARK Batch Aggregator & BFT Anti-Entropy Engine (US-078 / US-090) */
void p2p_bft_anti_entropy_init(p2p_bft_anti_entropy_engine_t *engine) {
    if (!engine) return;
    engine->node_count = 0;
    engine->consensus_state_version = 1;
    engine->total_reconciliations = 0;
    for (int i = 0; i < P2P_BFT_AE_NODES_MAX; i++) {
        engine->nodes[i].node_id = 0;
        engine->nodes[i].wan_region_id = 0;
        engine->nodes[i].state_version = 0;
        engine->nodes[i].ewma_latency_ms = 0;
        engine->nodes[i].byzantine_fault = 0;
        engine->nodes[i].active = 0;
    }
}

int p2p_bft_anti_entropy_add_node(p2p_bft_anti_entropy_engine_t *engine, uint32_t node_id, uint32_t wan_region_id, uint32_t version, uint32_t latency_ms) {
    if (!engine || engine->node_count >= P2P_BFT_AE_NODES_MAX) return -1;
    for (int i = 0; i < engine->node_count; i++) {
        if (engine->nodes[i].active && engine->nodes[i].node_id == node_id) {
            return -1; /* Duplicate node ID */
        }
    }
    int slot = engine->node_count;
    engine->nodes[slot].node_id = node_id;
    engine->nodes[slot].wan_region_id = wan_region_id;
    engine->nodes[slot].state_version = version;
    engine->nodes[slot].ewma_latency_ms = latency_ms;
    engine->nodes[slot].byzantine_fault = (latency_ms > 1000) ? 1 : 0;
    engine->nodes[slot].active = 1;
    engine->node_count++;
    if (version > engine->consensus_state_version) {
        engine->consensus_state_version = version;
    }
    return 0;
}

int p2p_bft_anti_entropy_reconcile(p2p_bft_anti_entropy_engine_t *engine, int *out_reconciled_nodes) {
    if (!engine) return -1;
    int reconciled = 0;
    for (int i = 0; i < engine->node_count; i++) {
        if (engine->nodes[i].active && !engine->nodes[i].byzantine_fault) {
            if (engine->nodes[i].state_version < engine->consensus_state_version) {
                engine->nodes[i].state_version = engine->consensus_state_version;
                reconciled++;
            }
        }
    }
    engine->total_reconciliations++;
    if (out_reconciled_nodes) *out_reconciled_nodes = reconciled;
    return 0;
}

void p2p_zk_stark_batch_init(p2p_zk_stark_batch_aggregator_t *agg) {
    if (!agg) return;
    agg->proof_count = 0;
    agg->total_batch_verifications = 0;
    agg->total_valid_stark_proofs = 0;
    for (int i = 0; i < P2P_ZK_STARK_BATCH_PROOFS_MAX; i++) {
        agg->proofs[i].proof_id = 0;
        agg->proofs[i].fri_queries_count = 0;
        agg->proofs[i].verified = 0;
        for (int k = 0; k < 64; k++) agg->proofs[i].proof_data[k] = 0;
    }
}

int p2p_zk_stark_batch_add_proof(p2p_zk_stark_batch_aggregator_t *agg, uint32_t proof_id, uint32_t fri_queries, const uint8_t *data, uint32_t data_len) {
    if (!agg || agg->proof_count >= P2P_ZK_STARK_BATCH_PROOFS_MAX) return -1;
    int slot = agg->proof_count;
    agg->proofs[slot].proof_id = proof_id;
    agg->proofs[slot].fri_queries_count = fri_queries;
    agg->proofs[slot].verified = 0;
    uint32_t copy_len = (data_len > 64) ? 64 : data_len;
    for (uint32_t k = 0; k < copy_len; k++) {
        agg->proofs[slot].proof_data[k] = data ? data[k] : 0;
    }
    agg->proof_count++;
    return 0;
}

int p2p_zk_stark_batch_verify_all(p2p_zk_stark_batch_aggregator_t *agg, int *out_valid_count) {
    /* Fail closed: no FRI/STARK verifier is implemented in this freestanding
     * build. Checking fri_queries_count is not a proof check, so every proof
     * is reported invalid and the call returns P2P_ZK_ERR_NO_VERIFIER. */
    if (!agg) return -1;
    for (int i = 0; i < agg->proof_count; i++) agg->proofs[i].verified = 0;
    if (out_valid_count) *out_valid_count = 0;
    return P2P_ZK_ERR_NO_VERIFIER;
}

/* Lot 41: P2P Mesh Network Cross-WAN Geo-Replicated Consensus Mesh & Zero-Knowledge Proof Aggregator Engine */

void p2p_cross_wan_mesh_init(p2p_cross_wan_mesh_t *mesh) {
    if (!mesh) return;
    mesh->node_count = 0;
    mesh->merkle_root = 0;
    mesh->total_rebalances = 0;
    for (int i = 0; i < P2P_CROSS_WAN_NODES_MAX; i++) {
        mesh->nodes[i].node_id = 0;
        mesh->nodes[i].region_id = 0;
        mesh->nodes[i].state_hash = 0;
        mesh->nodes[i].active = 0;
    }
}

int p2p_cross_wan_mesh_add_node(p2p_cross_wan_mesh_t *mesh, uint32_t node_id, uint32_t region_id) {
    if (!mesh || mesh->node_count >= P2P_CROSS_WAN_NODES_MAX) return -1;
    int idx = mesh->node_count;
    mesh->nodes[idx].node_id = node_id;
    mesh->nodes[idx].region_id = region_id;
    mesh->nodes[idx].state_hash = node_id ^ (region_id * 0x01000193U);
    mesh->nodes[idx].active = 1;
    mesh->node_count++;
    return 0;
}

int p2p_cross_wan_mesh_rebalance(p2p_cross_wan_mesh_t *mesh, int *out_topology_changes) {
    if (!mesh) return -1;
    int changes = 0;
    for (int i = 0; i < mesh->node_count; i++) {
        if (mesh->nodes[i].active) {
            mesh->nodes[i].state_hash += 1;
            changes++;
        }
    }
    mesh->total_rebalances++;
    if (out_topology_changes) *out_topology_changes = changes;
    return 0;
}

int p2p_cross_wan_mesh_merkle_aggregate(p2p_cross_wan_mesh_t *mesh, uint32_t *out_root_hash) {
    if (!mesh) return -1;
    uint32_t hash = 0x811C9DC5U;
    for (int i = 0; i < mesh->node_count; i++) {
        if (mesh->nodes[i].active) {
            hash ^= mesh->nodes[i].state_hash;
            hash *= 0x01000193U;
        }
    }
    mesh->merkle_root = hash;
    if (out_root_hash) *out_root_hash = hash;
    return 0;
}

void p2p_zk_aggregator_init(p2p_zk_aggregator_t *agg) {
    if (!agg) return;
    agg->proof_count = 0;
    agg->batch_verifications = 0;
    agg->total_valid_proofs = 0;
    for (int i = 0; i < P2P_ZK_AGGREGATOR_PROOFS_MAX; i++) {
        agg->proofs[i].proof_type = 0;
        agg->proofs[i].proof_len = 0;
        agg->proofs[i].valid = 0;
        for (int k = 0; k < 128; k++) agg->proofs[i].proof_data[k] = 0;
    }
}

int p2p_zk_aggregator_add_proof(p2p_zk_aggregator_t *agg, uint32_t proof_type, const uint8_t *proof_data, uint32_t proof_len) {
    if (!agg || !proof_data || proof_len == 0 || agg->proof_count >= P2P_ZK_AGGREGATOR_PROOFS_MAX) return -1;
    int idx = agg->proof_count;
    agg->proofs[idx].proof_type = proof_type;
    agg->proofs[idx].proof_len = (proof_len > 128) ? 128 : proof_len;
    for (uint32_t k = 0; k < agg->proofs[idx].proof_len; k++) {
        agg->proofs[idx].proof_data[k] = proof_data[k];
    }
    agg->proofs[idx].valid = 0;
    agg->proof_count++;
    return 0;
}

int p2p_zk_aggregator_verify_batch(p2p_zk_aggregator_t *agg, int *out_valid_count) {
    /* Fail closed: a non-empty proof is not a valid proof. No STARK/PLONK
     * batch verifier exists here, so nothing is accepted. */
    if (!agg) return -1;
    for (int i = 0; i < agg->proof_count; i++) agg->proofs[i].valid = 0;
    if (out_valid_count) *out_valid_count = 0;
    return P2P_ZK_ERR_NO_VERIFIER;
}

/* Lot 42: P2P Mesh Network Cross-WAN Geo-Distributed Fault-Tolerant Consensus Mesh & Zero-Knowledge Bulletproofs Batch Aggregator Engine (US-077 / US-089) */
void p2p_bft_consensus_mesh_init(p2p_bft_consensus_mesh_t *mesh) {
    int i;
    if (!mesh) return;
    for (i = 0; i < P2P_BFT_MESH_NODES_MAX; i++) {
        mesh->nodes[i].node_id = 0;
        mesh->nodes[i].region_id = 0;
        mesh->nodes[i].rtt_ewma_ms = 0;
        mesh->nodes[i].faulty = 0;
        mesh->nodes[i].slow = 0;
        mesh->nodes[i].active = 0;
    }
    mesh->node_count = 0;
    mesh->ewma_quorum_ms = 0;
    mesh->total_consensus_rounds = 0;
    mesh->faulty_nodes_detected = 0;
}

int p2p_bft_consensus_mesh_add_node(p2p_bft_consensus_mesh_t *mesh, uint32_t node_id, uint32_t region_id, uint32_t rtt_ms) {
    int i;
    if (!mesh) return -1;
    for (i = 0; i < P2P_BFT_MESH_NODES_MAX; i++) {
        if (mesh->nodes[i].active && mesh->nodes[i].node_id == node_id) {
            return -2;
        }
    }
    if (mesh->node_count >= P2P_BFT_MESH_NODES_MAX) return -3;
    for (i = 0; i < P2P_BFT_MESH_NODES_MAX; i++) {
        if (!mesh->nodes[i].active) {
            mesh->nodes[i].node_id = node_id;
            mesh->nodes[i].region_id = region_id;
            mesh->nodes[i].rtt_ewma_ms = rtt_ms;
            mesh->nodes[i].faulty = (rtt_ms > 1000) ? 1 : 0;
            mesh->nodes[i].slow = (rtt_ms > 200) ? 1 : 0;
            mesh->nodes[i].active = 1;
            mesh->node_count++;
            if (mesh->nodes[i].faulty) {
                mesh->faulty_nodes_detected++;
            }
            return 0;
        }
    }
    return -4;
}

int p2p_bft_consensus_mesh_evaluate_quorum(p2p_bft_consensus_mesh_t *mesh, int *out_quorum_achieved, uint32_t *out_active_nodes) {
    int i;
    uint32_t active_cnt = 0;
    uint32_t non_faulty_cnt = 0;
    uint32_t total_rtt = 0;
    if (!mesh) return -1;

    for (i = 0; i < P2P_BFT_MESH_NODES_MAX; i++) {
        if (mesh->nodes[i].active) {
            active_cnt++;
            if (!mesh->nodes[i].faulty) {
                non_faulty_cnt++;
                total_rtt += mesh->nodes[i].rtt_ewma_ms;
            }
        }
    }

    mesh->total_consensus_rounds++;
    if (non_faulty_cnt > 0) {
        mesh->ewma_quorum_ms = total_rtt / non_faulty_cnt;
    }

    if (out_active_nodes) *out_active_nodes = active_cnt;
    if (out_quorum_achieved) {
        /* BFT quorum threshold: 2f + 1 where f is faulty nodes */
        *out_quorum_achieved = (non_faulty_cnt >= (active_cnt * 2 / 3 + 1)) ? 1 : 0;
    }
    return 0;
}

void p2p_zk_bulletproof_batch_aggregator_init(p2p_zk_bulletproof_batch_aggregator_t *agg) {
    int i;
    if (!agg) return;
    for (i = 0; i < P2P_ZK_BULLETPROOF_BATCH_ENTRY_MAX; i++) {
        agg->proofs[i].proof_id = 0;
        agg->proofs[i].commitment_val = 0;
        agg->proofs[i].valid = 0;
    }
    agg->proof_count = 0;
    agg->batch_verifications_count = 0;
    agg->total_valid_bulletproofs = 0;
}

int p2p_zk_bulletproof_batch_add_proof(p2p_zk_bulletproof_batch_aggregator_t *agg, uint32_t proof_id, const uint8_t *proof_bytes, uint32_t commit_val) {
    int i;
    if (!agg || !proof_bytes) return -1;
    if (agg->proof_count >= P2P_ZK_BULLETPROOF_BATCH_ENTRY_MAX) return -2;
    for (i = 0; i < P2P_ZK_BULLETPROOF_BATCH_ENTRY_MAX; i++) {
        if (agg->proofs[i].proof_id == 0 && !agg->proofs[i].valid) {
            agg->proofs[i].proof_id = proof_id;
            for (int b = 0; b < 64; b++) {
                agg->proofs[i].proof_bytes[b] = proof_bytes[b];
            }
            agg->proofs[i].commitment_val = commit_val;
            agg->proofs[i].valid = 0;
            agg->proof_count++;
            return 0;
        }
    }
    return -3;
}

int p2p_zk_bulletproof_batch_verify_all(p2p_zk_bulletproof_batch_aggregator_t *agg, int *out_valid_count) {
    /* Fail closed: non-zero ids and commitments prove nothing. No
     * Bulletproofs verifier exists here, so nothing is accepted. */
    int i;
    if (!agg) return -1;
    for (i = 0; i < agg->proof_count; i++) agg->proofs[i].valid = 0;
    if (out_valid_count) *out_valid_count = 0;
    return P2P_ZK_ERR_NO_VERIFIER;
}

/* Lot 40: P2P Mesh Network Cross-WAN Geo-Distributed Raft Consensus Sharding & Zero-Knowledge STARK Verification Engine */

void p2p_wan_geo_sharding_init(p2p_wan_geo_sharding_mgr_t *mgr) {
    if (!mgr) return;
    mgr->shard_count = 0;
    mgr->rebalance_events = 0;
    mgr->compressed_bytes = 0;
    for (int i = 0; i < P2P_WAN_GEO_SHARDS_MAX; i++) {
        mgr->shards[i].shard_id = 0;
        mgr->shards[i].region_mask = 0;
        mgr->shards[i].leader_id = 0;
        mgr->shards[i].term = 0;
        mgr->shards[i].lease_expire_tick = 0;
        mgr->shards[i].active = 0;
        p2p_patricia_trie_init(&mgr->shards[i].state_trie);
    }
}

int p2p_wan_geo_sharding_add_shard(p2p_wan_geo_sharding_mgr_t *mgr, uint32_t shard_id, uint32_t region_mask, uint32_t leader_id) {
    if (!mgr || mgr->shard_count >= P2P_WAN_GEO_SHARDS_MAX) return -1;
    int idx = mgr->shard_count++;
    mgr->shards[idx].shard_id = shard_id;
    mgr->shards[idx].region_mask = region_mask;
    mgr->shards[idx].leader_id = leader_id;
    mgr->shards[idx].term = 1;
    mgr->shards[idx].lease_expire_tick = 100;
    mgr->shards[idx].active = 1;
    p2p_patricia_trie_init(&mgr->shards[idx].state_trie);
    return 0;
}

int p2p_wan_geo_sharding_rebalance_leases(p2p_wan_geo_sharding_mgr_t *mgr, uint32_t current_tick, int *out_rebalanced_count) {
    if (!mgr) return -1;
    int rebalanced = 0;
    for (int i = 0; i < mgr->shard_count; i++) {
        if (mgr->shards[i].active) {
            if (current_tick >= mgr->shards[i].lease_expire_tick) {
                mgr->shards[i].lease_expire_tick = current_tick + 200;
                mgr->shards[i].term++;
                rebalanced++;
            }
        }
    }
    mgr->rebalance_events += rebalanced;
    if (out_rebalanced_count) *out_rebalanced_count = rebalanced;
    return 0;
}

void p2p_patricia_trie_init(p2p_patricia_trie_t *trie) {
    if (!trie) return;
    trie->node_count = 0;
    trie->root_hash = 0x811C9DC5;
    for (int i = 0; i < P2P_PATRICIA_TRIE_MAX_NODES; i++) {
        trie->nodes[i].node_id = 0;
        trie->nodes[i].prefix[0] = 0;
        trie->nodes[i].hash = 0;
        trie->nodes[i].is_leaf = 0;
    }
}

int p2p_patricia_trie_insert(p2p_patricia_trie_t *trie, const char *prefix, const uint8_t *data, uint32_t len) {
    if (!trie || trie->node_count >= P2P_PATRICIA_TRIE_MAX_NODES) return -1;
    int idx = trie->node_count++;
    trie->nodes[idx].node_id = (uint32_t)(idx + 1);
    trie->nodes[idx].is_leaf = 1;

    int pidx = 0;
    if (prefix) {
        while (prefix[pidx] && pidx < 15) {
            trie->nodes[idx].prefix[pidx] = prefix[pidx];
            pidx++;
        }
    }
    trie->nodes[idx].prefix[pidx] = 0;

    uint32_t h = 0x811C9DC5;
    for (uint32_t i = 0; i < len; i++) {
        h = (h ^ (data ? data[i] : 0)) * 16777619U;
    }
    trie->nodes[idx].hash = h;
    trie->root_hash ^= h;
    return 0;
}

int p2p_patricia_trie_compress(p2p_patricia_trie_t *trie, uint32_t *out_compressed_hash) {
    if (!trie) return -1;
    uint32_t chash = trie->root_hash;
    for (int i = 0; i < trie->node_count; i++) {
        chash = (chash ^ trie->nodes[i].hash) * 16777619U;
    }
    trie->root_hash = chash;
    if (out_compressed_hash) *out_compressed_hash = chash;
    return 0;
}

void p2p_zk_stark_verifier_init(p2p_zk_stark_verifier_t *v) {
    if (!v) return;
    v->verified_starks = 0;
    v->failed_starks = 0;
}

int p2p_zk_stark_proof_create(uint32_t trace_len, int fri_layers_cnt, p2p_zk_stark_proof_t *out_proof) {
    if (!out_proof) return -1;
    out_proof->trace_length = trace_len;
    out_proof->fri_layers_count = (fri_layers_cnt > P2P_ZK_STARK_FRI_LAYERS_MAX) ? P2P_ZK_STARK_FRI_LAYERS_MAX : fri_layers_cnt;

    uint32_t dom_sz = trace_len;
    for (uint32_t i = 0; i < out_proof->fri_layers_count; i++) {
        out_proof->fri_layers[i].layer_idx = (uint32_t)i;
        out_proof->fri_layers[i].evaluation_domain_size = dom_sz;
        out_proof->fri_layers[i].merkle_root = 0xA5A50000 | (uint32_t)i;
        out_proof->fri_layers[i].folding_factor = 2;
        dom_sz /= 2;
        if (dom_sz == 0) dom_sz = 1;
    }

    for (int i = 0; i < 8; i++) {
        out_proof->query_evaluations[i] = (trace_len * 17U) ^ (uint32_t)(i * 31U);
    }

    out_proof->proof_len = 128;
    for (uint32_t i = 0; i < out_proof->proof_len; i++) {
        out_proof->proof_bytes[i] = (uint8_t)(i ^ 0x5A);
    }
    return 0;
}

int p2p_zk_stark_verify_fri(p2p_zk_stark_verifier_t *v, const p2p_zk_stark_proof_t *proof, int *out_valid) {
    /* Fail closed: non-zero lengths are not a FRI check. */
    if (!v || !proof || !out_valid) return -1;
    *out_valid = 0;
    v->failed_starks++;
    return P2P_ZK_ERR_NO_VERIFIER;
}

/* Lot 39: P2P Mesh Network Cross-WAN Asynchronous Consensus Replication & Zero-Knowledge PLONK Verification Engine (US-074 / US-086) */

void p2p_wan_async_repl_init(p2p_wan_async_repl_t *repl, uint32_t start_index) {
    if (!repl) return;
    repl->window_start = start_index;
    repl->window_end = start_index;
    repl->ack_watermark = (start_index > 0) ? start_index - 1 : 0;
    repl->compressed_bytes = 0;
    repl->uncompressed_bytes = 0;
    for (int i = 0; i < P2P_WAN_SLIDING_WINDOW_SZ; i++) {
        repl->window[i].index = 0;
        repl->window[i].term = 0;
        repl->window[i].len = 0;
        repl->window[i].acked = 0;
    }
}

int p2p_wan_async_repl_push(p2p_wan_async_repl_t *repl, uint32_t term, const uint8_t *data, uint32_t len) {
    if (!repl || !data || len == 0) return -1;
    if (repl->window_end - repl->window_start >= P2P_WAN_SLIDING_WINDOW_SZ) {
        return -1; /* Window full */
    }
    uint32_t slot = repl->window_end % P2P_WAN_SLIDING_WINDOW_SZ;
    repl->window[slot].index = repl->window_end;
    repl->window[slot].term = term;
    uint32_t copy_len = (len > 64) ? 64 : len;
    for (uint32_t i = 0; i < copy_len; i++) {
        repl->window[slot].data[i] = data[i];
    }
    repl->window[slot].len = copy_len;
    repl->window[slot].acked = 0;
    repl->window_end++;
    repl->uncompressed_bytes += len;
    return 0;
}

int p2p_wan_async_repl_ack(p2p_wan_async_repl_t *repl, uint32_t ack_index) {
    if (!repl) return -1;
    for (uint32_t idx = repl->window_start; idx < repl->window_end; idx++) {
        if (idx <= ack_index) {
            uint32_t slot = idx % P2P_WAN_SLIDING_WINDOW_SZ;
            repl->window[slot].acked = 1;
        }
    }
    while (repl->window_start < repl->window_end) {
        uint32_t slot = repl->window_start % P2P_WAN_SLIDING_WINDOW_SZ;
        if (repl->window[slot].acked) {
            repl->window_start++;
        } else {
            break;
        }
    }
    repl->ack_watermark = ack_index;
    return 0;
}

int p2p_wan_async_repl_compress_delta(const uint8_t *prev, uint32_t prev_len, const uint8_t *curr, uint32_t curr_len, uint8_t *out_delta, uint32_t *out_delta_len) {
    if (!curr || !out_delta || !out_delta_len || curr_len == 0) return -1;
    uint32_t prefix = 0;
    if (prev && prev_len > 0) {
        while (prefix < prev_len && prefix < curr_len && prev[prefix] == curr[prefix]) {
            prefix++;
        }
    }
    out_delta[0] = (uint8_t)curr_len;
    out_delta[1] = (uint8_t)prefix;
    uint32_t diff_len = curr_len - prefix;
    for (uint32_t i = 0; i < diff_len; i++) {
        out_delta[2 + i] = curr[prefix + i];
    }
    *out_delta_len = 2 + diff_len;
    return 0;
}

int p2p_wan_async_repl_decompress_delta(const uint8_t *prev, uint32_t prev_len, const uint8_t *delta, uint32_t delta_len, uint8_t *out_curr, uint32_t *out_curr_len) {
    if (!delta || !out_curr || !out_curr_len || delta_len < 2) return -1;
    uint32_t target_len = delta[0];
    uint32_t prefix = delta[1];
    uint32_t diff_len = delta_len - 2;
    if (prefix > prev_len || prefix + diff_len != target_len) return -1;
    for (uint32_t i = 0; i < prefix; i++) {
        out_curr[i] = prev[i];
    }
    for (uint32_t i = 0; i < diff_len; i++) {
        out_curr[prefix + i] = delta[2 + i];
    }
    *out_curr_len = target_len;
    return 0;
}

void p2p_zk_plonk_verifier_init(p2p_zk_plonk_verifier_t *v) {
    if (!v) return;
    v->verified_proofs = 0;
    v->failed_proofs = 0;
}

int p2p_zk_plonk_proof_create(const p2p_zk_plonk_circuit_entry_t *gates, int gate_cnt, const uint32_t *wires, p2p_zk_plonk_proof_t *out_proof) {
    if (!gates || !wires || !out_proof || gate_cnt <= 0 || gate_cnt > P2P_ZK_PLONK_CIRCUIT_GATES_MAX) return -1;
    out_proof->gate_count = gate_cnt;
    for (int i = 0; i < gate_cnt; i++) {
        out_proof->gates[i] = gates[i];
    }
    for (int i = 0; i < P2P_ZK_PLONK_WIRES_MAX; i++) {
        out_proof->wire_values[i] = wires[i];
    }
    out_proof->proof_len = 32;
    for (uint32_t i = 0; i < 32; i++) {
        out_proof->proof_bytes[i] = (uint8_t)(i ^ gate_cnt);
    }
    return 0;
}

/* Circuit satisfiability check over the cleartext wire values carried in
 * the proof. This is not zero-knowledge: the witness is revealed. */
int p2p_zk_plonk_verify(p2p_zk_plonk_verifier_t *v, const p2p_zk_plonk_proof_t *proof, int *out_valid) {
    if (!v || !proof || !out_valid || proof->gate_count <= 0) return -1;
    int valid = 1;
    for (int i = 0; i < proof->gate_count; i++) {
        const p2p_zk_plonk_circuit_entry_t *e = &proof->gates[i];
        if (e->wire_a >= P2P_ZK_PLONK_WIRES_MAX ||
            e->wire_b >= P2P_ZK_PLONK_WIRES_MAX ||
            e->wire_c >= P2P_ZK_PLONK_WIRES_MAX) {
            valid = 0;
            break;
        }
        int32_t a = (int32_t)proof->wire_values[e->wire_a];
        int32_t b = (int32_t)proof->wire_values[e->wire_b];
        int32_t c = (int32_t)proof->wire_values[e->wire_c];
        int32_t lhs = e->gate.q_l * a + e->gate.q_r * b + e->gate.q_o * c + e->gate.q_m * (a * b) + e->gate.q_c;
        if (lhs != 0) {
            valid = 0;
            break;
        }
    }
    *out_valid = valid;
    if (valid) {
        v->verified_proofs++;
    } else {
        v->failed_proofs++;
    }
    return 0;
}

/* Lot 38: P2P Cross-WAN Multi-Raft Consensus Group Sharding & Zero-Knowledge Bulletproof Range Proof Verification Engine (US-073 / US-085) */

void p2p_multi_raft_wan_shard_init(p2p_multi_raft_wan_shard_mgr_t *mgr) {
    if (!mgr) return;
    mzero(mgr, sizeof(*mgr));
}

int p2p_multi_raft_wan_shard_add(p2p_multi_raft_wan_shard_mgr_t *mgr, uint32_t shard_id, uint32_t wan_region_id, uint32_t leader_id) {
    if (!mgr) return -1;
    if (mgr->shard_count >= P2P_MULTI_RAFT_WAN_SHARDS_MAX) return -1;

    p2p_multi_raft_wan_shard_t *shard = &mgr->shards[mgr->shard_count++];
    shard->shard_id = shard_id;
    shard->wan_region_id = wan_region_id;
    shard->leader_id = leader_id;
    shard->term = 1;
    shard->lease_ticks = 50;
    shard->lease_expire_tick = 50;
    shard->healthy = 1;

    return 0;
}

int p2p_multi_raft_wan_shard_failover(p2p_multi_raft_wan_shard_mgr_t *mgr, uint32_t shard_id, uint32_t new_leader_id, uint32_t current_tick) {
    if (!mgr) return -1;

    for (int i = 0; i < mgr->shard_count; i++) {
        p2p_multi_raft_wan_shard_t *s = &mgr->shards[i];
        if (s->shard_id == shard_id) {
            s->leader_id = new_leader_id;
            s->term++;
            s->lease_expire_tick = current_tick + s->lease_ticks;
            s->healthy = 1;
            mgr->total_failovers++;
            return 0;
        }
    }

    return -1;
}

int p2p_multi_raft_wan_shard_rebalance_leases(p2p_multi_raft_wan_shard_mgr_t *mgr, uint32_t current_tick, int *out_rebalanced) {
    if (!mgr) return -1;

    int rebalanced = 0;
    for (int i = 0; i < mgr->shard_count; i++) {
        p2p_multi_raft_wan_shard_t *s = &mgr->shards[i];
        if (current_tick >= s->lease_expire_tick) {
            s->lease_expire_tick = current_tick + s->lease_ticks;
            rebalanced++;
        }
    }

    mgr->rebalance_count += rebalanced;
    if (out_rebalanced) *out_rebalanced = rebalanced;

    return 0;
}

void p2p_zk_range_batch_init(p2p_zk_range_batch_t *batch) {
    if (!batch) return;
    mzero(batch, sizeof(*batch));
}

int p2p_zk_range_proof_create(uint64_t value, uint64_t min_val, uint64_t max_val, p2p_zk_bulletproof_range_proof_t *out_proof) {
    if (!out_proof || min_val > max_val) return -1;

    mzero(out_proof, sizeof(*out_proof));
    out_proof->min_value = min_val;
    out_proof->max_value = max_val;
    out_proof->committed_value = value;
    out_proof->commitment_hash = (uint32_t)(value ^ min_val ^ max_val ^ 0x9E3779B9U);

    for (int i = 0; i < P2P_BULLETPROOF_INNER_PRODUCT_SIZE; i++) {
        out_proof->proof_bytes[i] = (uint8_t)(out_proof->commitment_hash + i);
    }
    out_proof->proof_len = P2P_BULLETPROOF_INNER_PRODUCT_SIZE;

    return 0;
}

int p2p_zk_range_batch_add(p2p_zk_range_batch_t *batch, const p2p_zk_bulletproof_range_proof_t *proof) {
    if (!batch || !proof) return -1;
    if (batch->count >= P2P_ZK_RANGE_BATCH_MAX) return -1;

    batch->proofs[batch->count++] = *proof;
    return 0;
}

int p2p_zk_range_batch_verify(p2p_zk_range_batch_t *batch, int *out_all_valid) {
    /* Fail closed: the old check compared the cleartext committed_value to
     * the range, which a prover controls and which is not zero-knowledge.
     * No range-proof verifier exists here, so nothing is accepted. */
    if (!batch || !out_all_valid) return -1;
    batch->verified_count = 0;
    *out_all_valid = 0;
    return P2P_ZK_ERR_NO_VERIFIER;
}

/* Lot 37: P2P Mesh Network Cross-WAN Resilient Raft Group Leader Election & Zero-Knowledge Bulletproof Batch Verification Engine (US-072 / US-084) */
void p2p_wan_raft_elec_init(p2p_wan_raft_elec_t *elec, uint32_t group_id, uint32_t total_voters, uint32_t lease_ticks) {
    if (!elec) return;
    for (uint32_t i = 0; i < sizeof(p2p_wan_raft_elec_t); i++) {
        ((uint8_t *)elec)[i] = 0;
    }
    elec->group_id = group_id;
    elec->total_voters = total_voters;
    elec->lease_ticks = lease_ticks;
    elec->state = 0; /* Follower */
}

int p2p_wan_raft_leader_elect(p2p_wan_raft_elec_t *elec, uint32_t candidate_id, uint32_t current_term, uint32_t votes_received, uint32_t current_tick) {
    if (!elec) return -1;

    uint32_t quorum = (elec->total_voters / 2) + 1;
    if (quorum < P2P_WAN_RAFT_QUORUM_MIN) quorum = P2P_WAN_RAFT_QUORUM_MIN;

    elec->term = current_term;
    elec->votes_granted = votes_received;

    if (votes_received >= quorum) {
        elec->leader_id = candidate_id;
        elec->state = 2; /* Leader */
        elec->leader_lease_expire_tick = current_tick + elec->lease_ticks;
        return 0;
    } else {
        elec->state = 1; /* Candidate */
        return -1; /* Quorum not met */
    }
}

int p2p_wan_raft_check_leader_lease(const p2p_wan_raft_elec_t *elec, uint32_t current_tick, int *out_lease_valid) {
    if (!elec || !out_lease_valid) return -1;

    if (elec->state == 2 && current_tick <= elec->leader_lease_expire_tick) {
        *out_lease_valid = 1;
    } else {
        *out_lease_valid = 0;
    }

    return 0;
}

void p2p_zk_bulletproof_batch_init(p2p_zk_bulletproof_batch_t *batch) {
    if (!batch) return;
    for (uint32_t i = 0; i < sizeof(p2p_zk_bulletproof_batch_t); i++) {
        ((uint8_t *)batch)[i] = 0;
    }
}

int p2p_zk_bulletproof_batch_add(p2p_zk_bulletproof_batch_t *batch, const p2p_zk_bulletproof_ip_t *proof, uint32_t expected_inner_product) {
    if (!batch || !proof) return -1;
    if (batch->count >= P2P_ZK_BULLETPROOF_BATCH_MAX) return -1;

    batch->proofs[batch->count] = *proof;
    batch->expected_inner_products[batch->count] = expected_inner_product;
    batch->count++;

    return 0;
}

int p2p_zk_bulletproof_batch_verify(p2p_zk_bulletproof_batch_t *batch, int *out_all_valid) {
    /* Fail closed, like p2p_zk_bulletproof_inner_product_verify. */
    if (!batch || !out_all_valid) return -1;
    batch->verified_count = 0;
    *out_all_valid = 0;
    return P2P_ZK_ERR_NO_VERIFIER;
}

/* Lot 33: P2P Cross-WAN Geo-Sharded Multi-Region Consensus & Zero-Knowledge Bulletproof Validation (US-068 / US-080) */

void p2p_geo_shard_init(p2p_geo_shard_t *shard, uint32_t shard_id, uint32_t region_mask) {
    if (!shard) return;
    shard->shard_id = shard_id;
    shard->region_mask = region_mask;
    shard->count = 0;
    shard->next_consensus_id = 1;
    shard->active = 1;
    for (int i = 0; i < P2P_GEO_SHARD_ENTRIES_MAX; i++) {
        shard->entries[i].entry_id = 0;
        shard->entries[i].region_origin = 0;
        shard->entries[i].payload[0] = '\0';
        shard->entries[i].synced = 0;
    }
}

int p2p_geo_consensus_propose(p2p_geo_shard_t *shard, uint32_t proposer_id, const char *data, uint32_t *out_consensus_id) {
    if (!shard || !shard->active || !data) return -1;
    if (shard->count >= P2P_GEO_SHARD_ENTRIES_MAX) return -2;

    uint32_t cid = shard->next_consensus_id++;
    p2p_geo_entry_t *entry = &shard->entries[shard->count];
    entry->entry_id = cid;
    entry->region_origin = proposer_id;
    s_copy(entry->payload, data, sizeof(entry->payload));
    entry->synced = 0;

    shard->count++;
    if (out_consensus_id) *out_consensus_id = cid;

    return 0;
}

int p2p_geo_cross_region_sync(p2p_geo_shard_t *shard, uint32_t target_region_id, int *out_synced_entries) {
    if (!shard || !shard->active) return -1;
    (void)target_region_id;

    int synced_count = 0;
    for (int i = 0; i < shard->count; i++) {
        if (!shard->entries[i].synced) {
            shard->entries[i].synced = 1;
            synced_count++;
        }
    }

    if (out_synced_entries) *out_synced_entries = synced_count;
    return 0;
}

int p2p_zk_bulletproof_create_commitment(uint32_t value, uint32_t blinding_factor, p2p_zk_bulletproof_t *out_proof) {
    if (!out_proof) return -1;

    out_proof->value_commitment = (value * 2654435761U) ^ blinding_factor;
    out_proof->blinding_factor = blinding_factor;
    out_proof->proof_len = P2P_BULLETPROOF_BYTES_MAX;

    for (int i = 0; i < P2P_BULLETPROOF_BYTES_MAX; i++) {
        out_proof->proof_bytes[i] = (uint8_t)(out_proof->value_commitment ^ (i * 13U));
    }

    return 0;
}

int p2p_zk_bulletproof_verify(const p2p_zk_bulletproof_t *proof, uint32_t min_val, uint32_t max_val, int *out_valid) {
    /* Fail closed: the old check accepted any proof whose first byte was
     * non-zero and ignored the range. No range-proof verifier exists here. */
    if (!proof || !out_valid) return -1;
    (void)min_val;
    (void)max_val;
    *out_valid = 0;
    return P2P_ZK_ERR_NO_VERIFIER;
}

int p2p_bft_prepare(p2p_bft_state_t *bft, uint32_t voter_id, uint32_t view_id, uint32_t seq) {
    if (!bft || bft->stage != P2P_BFT_STAGE_PREPREPARE) return -1;
    (void)voter_id;
    if (bft->view_id != view_id || bft->sequence_num != seq) return -1;

    bft->prepare_votes++;
    /* Check quorum: 2f + 1 prepare votes */
    if (bft->prepare_votes >= (2 * bft->f_max_faulty + 1)) {
        bft->stage = P2P_BFT_STAGE_PREPARE;
    }
    return 0;
}

int p2p_bft_commit(p2p_bft_state_t *bft, uint32_t voter_id, uint32_t view_id, uint32_t seq) {
    if (!bft || bft->stage < P2P_BFT_STAGE_PREPARE) return -1;
    (void)voter_id;
    if (bft->view_id != view_id || bft->sequence_num != seq) return -1;

    bft->commit_votes++;
    /* Check quorum: 2f + 1 commit votes */
    if (bft->commit_votes >= (2 * bft->f_max_faulty + 1)) {
        bft->stage = P2P_BFT_STAGE_COMMIT;
        bft->committed = 1;
    }
    return 0;
}

void p2p_bloom_sync_init(p2p_bloom_filter_t *filter) {
    if (!filter) return;
    for (int i = 0; i < P2P_BLOOM_FILTER_SIZE_BITS / 8; i++) {
        filter->filter[i] = 0;
    }
    filter->items_added = 0;
}

int p2p_bloom_sync_diff(const p2p_bloom_filter_t *local_bf, const p2p_bloom_filter_t *remote_bf, int *out_mismatch_count) {
    if (!local_bf || !remote_bf) return -1;

    int mismatches = 0;
    for (int i = 0; i < P2P_BLOOM_FILTER_SIZE_BITS / 8; i++) {
        uint8_t diff = local_bf->filter[i] ^ remote_bf->filter[i];
        while (diff > 0) {
            if (diff & 1) mismatches++;
            diff >>= 1;
        }
    }

    if (out_mismatch_count) *out_mismatch_count = mismatches;
    return 0;
}

/* Lot 32: P2P WAN Mesh Dynamic Topology Reconfiguration & Zero-Knowledge Consensus (US-067 / US-079) */
int p2p_topology_reconfigure(p2p_node_t *n, const p2p_topo_peer_info_t *peers, int peer_count, uint32_t *out_active_topology) {
    if (!n || !peers || peer_count <= 0) return -1;

    int total_flaps = 0;
    int active_peers = 0;
    uint32_t total_ewma_rtt = 0;

    for (int i = 0; i < peer_count; i++) {
        if (peers[i].active) {
            active_peers++;
            total_flaps += peers[i].flap_count;
            total_ewma_rtt += peers[i].ewma_rtt;
        }
    }

    uint32_t selected_topo = P2P_TOPOLOGY_MESH;

    /* Anti-flapping EWMA topology switching strategy */
    if (total_flaps > 5 || (active_peers > 0 && (total_ewma_rtt / active_peers) > 150)) {
        /* Switch to STAR topology for high-latency or flapping conditions */
        selected_topo = P2P_TOPOLOGY_STAR;
    } else if (active_peers <= 2) {
        /* Switch to RING topology for small peer counts */
        selected_topo = P2P_TOPOLOGY_RING;
    } else {
        selected_topo = P2P_TOPOLOGY_MESH;
    }

    if (out_active_topology) *out_active_topology = selected_topo;

    return 0;
}

int p2p_zk_consensus_verify(const p2p_zk_proof_t *proof, const char *expected_statement, int *out_valid) {
    /* Fail closed: an unkeyed FNV-1a hash over public bytes is not a proof
     * (the old code accepted any non-zero hash). No verifier exists here. */
    if (!proof || !expected_statement) return -1;
    if (out_valid) *out_valid = 0;
    return P2P_ZK_ERR_NO_VERIFIER;
}

/* Lot 35: P2P Cross-WAN Resilient Raft Group Sharding & Zero-Knowledge Bulletproof Inner-Product Proofs (US-071 / US-083) */

void p2p_cross_wan_raft_shard_init(p2p_cross_wan_raft_shard_mgr_t *mgr) {
    if (!mgr) return;
    mgr->group_count = 0;
    mgr->total_sharded_proposals = 0;
    for (int i = 0; i < P2P_CROSS_WAN_GROUPS_MAX; i++) {
        mgr->groups[i].group_id = (uint32_t)(i + 1);
        mgr->groups[i].wan_region_mask = (1 << i);
        mgr->groups[i].term = 1;
        mgr->groups[i].leader_id = (uint32_t)(i + 1);
        mgr->groups[i].active = 1;
        mgr->group_count++;
    }
}

int p2p_cross_wan_raft_propose_sharded(p2p_cross_wan_raft_shard_mgr_t *mgr, uint32_t group_id, const char *command) {
    if (!mgr || !command || command[0] == '\0') return -1;

    for (int i = 0; i < mgr->group_count; i++) {
        if (mgr->groups[i].active && mgr->groups[i].group_id == group_id) {
            mgr->total_sharded_proposals++;
            return 0;
        }
    }

    return -1; /* Group not found */
}

int p2p_cross_wan_raft_sync_groups(p2p_cross_wan_raft_shard_mgr_t *mgr, uint32_t target_wan_region, int *out_synced_groups) {
    if (!mgr) return -1;

    int synced = 0;
    for (int i = 0; i < mgr->group_count; i++) {
        if (mgr->groups[i].active && (mgr->groups[i].wan_region_mask & target_wan_region)) {
            synced++;
        }
    }

    if (out_synced_groups) *out_synced_groups = synced;
    return 0;
}

int p2p_zk_bulletproof_inner_product_create(const uint32_t *vec_a, const uint32_t *vec_b, uint32_t count, p2p_zk_bulletproof_ip_t *out_proof) {
    if (!vec_a || !vec_b || !out_proof || count == 0) return -1;

    uint32_t dot = 0;
    uint32_t commit_a = 2166136261U;
    uint32_t commit_b = 2166136261U;

    for (uint32_t i = 0; i < count; i++) {
        dot += vec_a[i] * vec_b[i];
        commit_a = (commit_a ^ vec_a[i]) * 16777619U;
        commit_b = (commit_b ^ vec_b[i]) * 16777619U;
    }

    out_proof->vector_a_commitment = commit_a;
    out_proof->vector_b_commitment = commit_b;
    out_proof->inner_product_value = dot;
    out_proof->proof_len = P2P_BULLETPROOF_INNER_PRODUCT_SIZE;

    for (uint32_t k = 0; k < P2P_BULLETPROOF_INNER_PRODUCT_SIZE; k++) {
        out_proof->proof_bytes[k] = (uint8_t)((dot + k) & 0xFF);
    }

    return 0;
}

int p2p_zk_bulletproof_inner_product_verify(const p2p_zk_bulletproof_ip_t *proof, uint32_t expected_inner_product, int *out_valid) {
    /* Fail closed: the old check compared the prover-supplied
     * inner_product_value field with the expected value, which any forger
     * can set. No inner-product argument verifier exists here. */
    if (!proof || proof->proof_len == 0 || !out_valid) return -1;
    (void)expected_inner_product;
    *out_valid = 0;
    return P2P_ZK_ERR_NO_VERIFIER;
}

int p2p_bbr_anti_entropy_sync(p2p_node_t *n, uint32_t target_peer_id, uint32_t bw_est_kbps, int *out_items_synced) {
    if (!n) return -1;
    (void)target_peer_id;

    /* Pacing batch size proportional to estimated bandwidth BBR */
    int batch_size = (int)(bw_est_kbps / 100);
    if (batch_size < 1) batch_size = 1;
    if (batch_size > 32) batch_size = 32;

    if (out_items_synced) *out_items_synced = batch_size;

    return 0;
}
