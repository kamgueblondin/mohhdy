/* userspace/p2p.c - Phase 5 P2P node. See p2p.h and docs/p2p.md. */
#include "p2p.h"
#include "../kernel/sha256.h"
#include "../kernel/aes_gcm.h"
#include "../kernel/x25519.h"

#define HDR 24
static uint32_t g_ws[X25519_WORKSPACE_LIMBS];
static const char* const k_names[P2P_I_COUNT] = {
    "?", "hello", "sealed", "?", "?", "?", "?", "?", "?", "?",
    "ping", "pong", "msg", "put", "sync-req", "sync-items", "get-req", "get-resp", "propose", "vote", "commit"
};
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
    b[0] = 'M'; b[1] = 'P'; b[2] = '2'; b[3] = 1; b[4] = (uint8_t)type; b[5] = 3; b[6] = 0; b[7] = 0;
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

static void send_hello(p2p_node_t* n, const p2p_host_t* h) {
    uint8_t b[P2P_DATAGRAM_MAX], mac[32];
    int pos = HDR, i, cnt = 0;
    header(n, b, P2P_T_HELLO, 0, 0);
    mcopy(b + pos, (const uint8_t*)n->name, P2P_NAME_MAX); pos += P2P_NAME_MAX;
    mcopy(b + pos, n->ip, 4); pos += 4;
    mcopy(b + pos, n->pub, 32); pos += 32;
    b[pos++] = 0;
    for (i = 0; i < P2P_PEERS; i++)
        if (n->peers[i].used && n->peers[i].up) { put32(b + pos, n->peers[i].id); pos += 4; cnt++; }
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
    p->keyed = 1; p->tx_counter = 0; p->rx_counter = 0;
    mzero(shared, 32); mzero(d, 32);
}

/* Route: direct if the peer is up, else through an up neighbor that lists it. */
static uint32_t route(p2p_node_t* n, p2p_peer_t* p, const uint8_t** ip) {
    int i, j;
    *ip = 0;
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
    return emit(n, h, ip, b, pos);
}

/* Per-peer budget for user traffic (US-068/073). Control frames bypass it. */
static int admit(p2p_node_t* n, const p2p_host_t* h, p2p_peer_t* p) {
    uint32_t t = now(h);
    if (!n->rate_limit) return 1;
    if (t - p->bucket_tick >= P2P_HZ) { p->bucket_tick = t; p->bucket_used = 0; }
    if (p->bucket_used >= n->rate_limit) { p->throttled++; return 0; }
    p->bucket_used++;
    return 1;
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

static void on_sealed(p2p_node_t* n, const p2p_host_t* h, p2p_peer_t* p, int inner, const uint8_t* m, int len) {
    uint8_t b[P2P_DATAGRAM_MAX];
    char line[200];
    p2p_item_t it;
    int pos, i;
    line[0] = 0;
    n->rx_type[inner]++;
    switch (inner) {
    case P2P_I_PING:
        if (len >= 4) (void)seal_send(n, h, p, P2P_I_PONG, m, 4);
        break;
    case P2P_I_PONG:
        if (len >= 4) p->rtt_ticks = now(h) - get32(m);
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
    default:
        break;
    }
}

static int propose_send(p2p_node_t* n, const p2p_host_t* h, int only_missing);
static void on_hello(p2p_node_t* n, const p2p_host_t* h, const uint8_t* b, int len, const uint8_t mac[6]) {
    uint8_t tag[32];
    p2p_peer_t* p;
    uint32_t src = get32(b + 8);
    int cnt, i, body = HDR + P2P_NAME_MAX + 4 + 32 + 1;
    char line[96];
    if (len < body + 16) { n->bad_hello++; return; }
    cnt = b[body - 1];
    if (cnt > P2P_PEERS || len != body + cnt * 4 + 16) { n->bad_hello++; return; }
    hmac_sha256(n->netkey, 32, b, (uint32_t)(body + cnt * 4), tag);
    if (!meq(tag, b + body + cnt * 4, 16)) { n->bad_hello++; return; }
    p = by_id(n, src);
    if (!p) {
        for (i = 0; i < P2P_PEERS && n->peers[i].used; i++) {}
        if (i == P2P_PEERS) return;
        p = &n->peers[i];
        mzero(p, (int)sizeof(*p));
        p->used = 1; p->id = src;
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
    for (i = 0; i < P2P_PEERS; i++) p->neighbors[i] = i < cnt ? get32(b + body + i * 4) : 0;
    p->last_seen = now(h);
    p->via = 0;
    if (!p->up) {
        p->up = 1;
        line[0] = 0;
        s_cat(line, "p2p peer ", 96); s_cat(line, p->name, 96); s_cat(line, " up id ", 96); cat_hex32(line, p->id, 96);
        s_cat(line, p->keyed ? " keyed" : " unkeyed", 96);
        say(h, line);
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
        if (via == n->id && b[5] > 0 && d && d->up && !d->blocked) {
            b[5]--; put32(b + 20, 0);
            n->relayed++;
            (void)emit(n, h, d->ip, b, len);
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
        if (plain[0] < P2P_I_PING || plain[0] >= P2P_I_COUNT) return;
        on_sealed(n, h, p, plain[0], plain + 1, clen - 1);
    }
}

/* -------------------------------------------------------------- public */
int p2p_up(p2p_node_t* n, const char* name, const uint8_t ip[4], const char* netkey,
           const uint8_t seed[32], const p2p_host_t* h) {
    uint8_t d[32];
    sha256_ctx_t c;
    if (!n || !name || !name[0] || s_len(name) >= P2P_NAME_MAX || !ip || !netkey || s_len(netkey) < 8 || !seed) return -1;
    mzero(n, (int)sizeof(*n));
    s_copy(n->name, name, P2P_NAME_MAX);
    mcopy(n->ip, ip, 4);
    sha256_init(&c); sha256_update(&c, (const uint8_t*)"mohhdy-p2p-net", 14);
    sha256_update(&c, (const uint8_t*)netkey, (uint32_t)s_len(netkey)); sha256_final(&c, n->netkey);
    sha256_init(&c); sha256_update(&c, seed, 32); sha256_update(&c, (const uint8_t*)name, (uint32_t)s_len(name));
    sha256_final(&c, n->secret);
    if (x25519_public_key(n->pub, n->secret, g_ws, X25519_WORKSPACE_LIMBS) != 0) return -1;
    sha256_init(&c); sha256_update(&c, n->pub, 32); sha256_final(&c, d);
    n->id = get32(d) | 1U;
    n->up = 1;
    n->last_hello = now(h) - P2P_HELLO_TICKS;
    n->last_ping = now(h);
    send_hello(n, h);
    n->last_hello = now(h);
    return 0;
}

void p2p_down(p2p_node_t* n) { if (n) { mzero(n->secret, 32); n->up = 0; } }

void p2p_tick(p2p_node_t* n, const p2p_host_t* h, int budget) {
    uint8_t b[P2P_DATAGRAM_MAX + 16], ip[4], mac[6];
    uint32_t t;
    int i, got;
    if (!n || !n->up) return;
    while (budget-- > 0 && h && h->recv) {
        got = h->recv(h->ctx, ip, mac, b, (uint16_t)sizeof(b));
        if (got <= 0) break;
        on_datagram(n, h, b, got, mac);
    }
    t = now(h);
    if (t - n->last_hello >= P2P_HELLO_TICKS) { n->last_hello = t; send_hello(n, h); }
    for (i = 0; i < P2P_PEERS; i++) {
        p2p_peer_t* p = &n->peers[i];
        if (!p->used || !p->up) continue;
        if (t - p->last_seen >= P2P_DOWN_TICKS) {
            char line[64];
            p->up = 0; p->down_events++;
            line[0] = 0; s_cat(line, "p2p peer ", 64); s_cat(line, p->name, 64); s_cat(line, " down", 64);
            say(h, line);
        }
    }
    if (t - n->last_ping >= P2P_PING_TICKS) {
        n->last_ping = t;
        for (i = 0; i < P2P_PEERS; i++) {
            p2p_peer_t* p = &n->peers[i];
            if (!p->used || !p->up || !p->keyed) continue;
            put32(b, t); p->ping_sent_at = t;
            (void)seal_send(n, h, p, P2P_I_PING, b, 4);
        }
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
            pr->result = 1; pr->active = 0;
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
    if (!admit(n, h, p)) return -2;
    pos = put_str(b, 0, 400, text);
    if (pos < 0) return -1;
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
        if (!admit(n, h, p)) continue;
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
    uint8_t b[P2P_DATAGRAM_MAX];
    int i, len, sent = 0;
    len = digest_encode(n, b, 0);
    if (len < 0) return -1;
    for (i = 0; i < P2P_PEERS; i++) {
        p2p_peer_t* p = &n->peers[i];
        if (!p->used || !p->keyed || !p->up) continue;
        if (peer && peer[0] && !s_eq(p->name, peer)) continue;
        if (seal_send(n, h, p, P2P_I_SYNC_REQ, b, len) == 0) sent++;
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
        s_cat(out, "\n", cap);
    }
    return s_len(out);
}
