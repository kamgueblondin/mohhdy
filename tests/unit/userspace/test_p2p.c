/* Phase 5 P2P node (US-061..US-075) on an in-process broadcast bus. */
#include "../../framework/unity.h"
#include <string.h>
#include <stdio.h>
#include "../../../userspace/p2p.h"

#define NODES 4
#define QLEN 96
typedef struct { uint8_t ip[4], mac[6]; uint8_t data[P2P_DATAGRAM_MAX]; int len; } dgram_t;
typedef struct { int idx; } ctx_t;
static dgram_t g_q[NODES][QLEN];
static int g_qh[NODES], g_qt[NODES];
static int g_cut[NODES][NODES];      /* 1: frames from i never reach j */
static uint32_t g_ticks;
static char g_out[NODES][65536];
static p2p_node_t g_n[NODES];
static ctx_t g_ctx[NODES];
static p2p_host_t g_h[NODES];
static uint8_t g_last[P2P_DATAGRAM_MAX]; static int g_last_len; static int g_last_from;

static int bus_send(void* c, const uint8_t ip[4], const uint8_t* d, uint16_t len) {
    int from = ((ctx_t*)c)->idx, j;
    (void)ip;
    memcpy(g_last, d, len); g_last_len = len; g_last_from = from;
    for (j = 0; j < NODES; j++) {
        dgram_t* q;
        if (j == from || g_cut[from][j] || (g_qt[j] + 1) % QLEN == g_qh[j]) continue;
        q = &g_q[j][g_qt[j]];
        memcpy(q->ip, g_n[from].ip, 4);
        memset(q->mac, 0, 6); q->mac[5] = (uint8_t)(from + 1);
        memcpy(q->data, d, len); q->len = len;
        g_qt[j] = (g_qt[j] + 1) % QLEN;
    }
    return 0;
}
static void inject(int to, int from, const uint8_t* d, int len) {
    dgram_t* q = &g_q[to][g_qt[to]];
    memset(q->mac, 0, 6); q->mac[5] = (uint8_t)(from + 1);
    memcpy(q->data, d, len); q->len = len;
    g_qt[to] = (g_qt[to] + 1) % QLEN;
}
static int bus_recv(void* c, uint8_t ip[4], uint8_t mac[6], uint8_t* d, uint16_t cap) {
    int me = ((ctx_t*)c)->idx;
    dgram_t* q;
    if (g_qh[me] == g_qt[me]) return 0;
    q = &g_q[me][g_qh[me]];
    g_qh[me] = (g_qh[me] + 1) % QLEN;
    if (q->len > cap) return 0;
    memcpy(ip, q->ip, 4); memcpy(mac, q->mac, 6); memcpy(d, q->data, q->len);
    return q->len;
}
static uint32_t bus_ticks(void* c) { (void)c; return g_ticks; }
static void bus_out(void* c, const char* s) {
    int me = ((ctx_t*)c)->idx;
    if (strlen(g_out[me]) + strlen(s) + 2 < sizeof(g_out[me])) { strcat(g_out[me], s); strcat(g_out[me], "\n"); }
}

static void setup(int count, const char* key_c) {
    int i;
    static const char* names[NODES] = {"alpha", "beta", "gamma", "delta"};
    memset(g_q, 0, sizeof(g_q)); memset(g_qh, 0, sizeof(g_qh)); memset(g_qt, 0, sizeof(g_qt));
    memset(g_cut, 0, sizeof(g_cut)); memset(g_out, 0, sizeof(g_out)); memset(g_n, 0, sizeof(g_n));
    g_ticks = 1000;
    for (i = 0; i < count; i++) {
        uint8_t ip[4] = {10, 77, 0, (uint8_t)(i + 1)}, seed[32];
        memset(seed, 0x40 + i, 32);
        g_ctx[i].idx = i;
        g_h[i].ctx = &g_ctx[i]; g_h[i].send = bus_send; g_h[i].recv = bus_recv; g_h[i].ticks = bus_ticks; g_h[i].out = bus_out;
        TEST_ASSERT_EQUAL(0, p2p_up(&g_n[i], names[i], ip, (i == 2 && key_c) ? key_c : "mohhdy-net-key", seed, &g_h[i]));
    }
}
/* advance simulated time in steps, pumping every node */
static void run(int count, uint32_t ticks) {
    uint32_t t;
    int i;
    for (t = 0; t < ticks; t += 10) {
        g_ticks += 10;
        for (i = 0; i < count; i++) p2p_tick(&g_n[i], &g_h[i], 32);
    }
}

void test_discovery_and_keys(void) {
    char rep[2048];
    setup(3, 0);
    run(3, 300);
    TEST_ASSERT_NOT_NULL(p2p_find(&g_n[0], "beta"));
    TEST_ASSERT_NOT_NULL(p2p_find(&g_n[0], "gamma"));
    TEST_ASSERT_TRUE(p2p_find(&g_n[0], "beta")->up);
    TEST_ASSERT_TRUE(p2p_find(&g_n[0], "beta")->keyed);
    /* both ends derived the same pair key */
    TEST_ASSERT_EQUAL(0, memcmp(p2p_find(&g_n[0], "beta")->key, p2p_find(&g_n[1], "alpha")->key, 16));
    TEST_ASSERT_NOT_EQUAL(0, memcmp(p2p_find(&g_n[0], "beta")->key, p2p_find(&g_n[0], "gamma")->key, 16));
    TEST_ASSERT_NOT_NULL(strstr(g_out[0], "p2p peer beta up id "));
    p2p_report(&g_n[0], &g_h[0], "peers", rep, sizeof(rep));
    TEST_ASSERT_NOT_NULL(strstr(rep, "p2p peers gamma up ip 10.77.0.3 id "));
    TEST_ASSERT_NOT_NULL(strstr(rep, "p2p peers ok 2 up 2"));
}

void test_wrong_network_key_is_kept_out(void) {
    setup(3, "another-network");
    run(3, 300);
    TEST_ASSERT_NULL(p2p_find(&g_n[0], "gamma"));
    TEST_ASSERT_NULL(p2p_find(&g_n[2], "alpha"));
    TEST_ASSERT_NOT_NULL(p2p_find(&g_n[0], "beta"));
    TEST_ASSERT_GREATER_THAN(0, (int)g_n[0].bad_hello);
}

void test_encrypted_message_tamper_and_replay(void) {
    int i, found = 0;
    setup(2, 0);
    run(2, 300);
    TEST_ASSERT_EQUAL(0, p2p_send_text(&g_n[0], &g_h[0], "beta", "secret bonjour"));
    for (i = 0; i + 6 <= g_last_len; i++) if (!memcmp(g_last + i, "secret", 6)) found = 1;
    TEST_ASSERT_EQUAL(0, found); /* not in clear on the wire */
    run(2, 20);
    TEST_ASSERT_NOT_NULL(strstr(g_out[1], "p2p msg from alpha: secret bonjour"));
    /* replay of the same datagram: delivered once */
    g_out[1][0] = 0;
    inject(1, 0, g_last, g_last_len);
    run(2, 20);
    TEST_ASSERT_NULL(strstr(g_out[1], "secret bonjour"));
    TEST_ASSERT_GREATER_THAN(0, (int)g_n[1].dup);
    /* tampered ciphertext with a fresh message id: authentication fails */
    TEST_ASSERT_EQUAL(0, p2p_send_text(&g_n[0], &g_h[0], "beta", "second"));
    g_qt[1] = (g_qt[1] + QLEN - 1) % QLEN; /* drop the genuine one */
    g_last[30] ^= 1;
    inject(1, 0, g_last, g_last_len);
    run(2, 20);
    TEST_ASSERT_EQUAL(1, (int)p2p_find(&g_n[1], "alpha")->auth_fail);
    TEST_ASSERT_NULL(strstr(g_out[1], "second"));
}

void test_replication_lww_get_and_sync(void) {
    const p2p_item_t* it;
    setup(3, 0);
    run(3, 300);
    TEST_ASSERT_EQUAL(2, p2p_put(&g_n[0], &g_h[0], "color", "blue"));
    run(3, 30);
    it = p2p_get_local(&g_n[2], "color");
    TEST_ASSERT_NOT_NULL(it);
    TEST_ASSERT_EQUAL_STRING("blue", it->value);
    TEST_ASSERT_EQUAL(1, (int)it->version);
    TEST_ASSERT_NOT_NULL(strstr(g_out[1], "p2p kv replicated color=blue v1 from alpha"));
    TEST_ASSERT_EQUAL(2, p2p_put(&g_n[1], &g_h[1], "color", "red"));
    run(3, 30);
    TEST_ASSERT_EQUAL_STRING("red", p2p_get_local(&g_n[0], "color")->value);
    TEST_ASSERT_EQUAL(2, (int)p2p_get_local(&g_n[0], "color")->version);
    /* partition gamma, write, heal, sync */
    g_cut[0][2] = g_cut[1][2] = 1;
    TEST_ASSERT_EQUAL(2, p2p_put(&g_n[0], &g_h[0], "mode", "eco"));
    run(3, 30);
    TEST_ASSERT_NULL(p2p_get_local(&g_n[2], "mode"));
    g_cut[0][2] = g_cut[1][2] = 0;
    TEST_ASSERT_EQUAL(1, p2p_sync(&g_n[2], &g_h[2], "alpha"));
    run(3, 40);
    TEST_ASSERT_NOT_NULL(p2p_get_local(&g_n[2], "mode"));
    TEST_ASSERT_NOT_NULL(strstr(g_out[2], "p2p kv synced mode=eco v1 from alpha"));
    TEST_ASSERT_NOT_NULL(strstr(g_out[2], "p2p sync from alpha items 1 applied 1"));
    /* gamma's newer local write flows back through the reply digest */
    g_cut[2][0] = g_cut[2][1] = 1;
    TEST_ASSERT_EQUAL(2, p2p_put(&g_n[2], &g_h[2], "only", "gamma"));
    g_cut[2][0] = g_cut[2][1] = 0;
    TEST_ASSERT_EQUAL(1, p2p_sync(&g_n[0], &g_h[0], "gamma"));
    run(3, 40);
    TEST_ASSERT_NOT_NULL(p2p_get_local(&g_n[0], "only"));
    /* remote get (distributed cache) */
    g_cut[1][0] = 1;
    TEST_ASSERT_EQUAL(2, p2p_put(&g_n[1], &g_h[1], "late", "x"));
    g_cut[1][0] = 0;
    TEST_ASSERT_NULL(p2p_get_local(&g_n[0], "late"));
    TEST_ASSERT_EQUAL(2, p2p_get_remote(&g_n[0], &g_h[0], "late"));
    run(3, 40);
    TEST_ASSERT_NOT_NULL(strstr(g_out[0], "p2p kv fetched late=x v1 from beta"));
    TEST_ASSERT_EQUAL(2, p2p_get_remote(&g_n[0], &g_h[0], "nokey"));
    run(3, 40);
    TEST_ASSERT_NOT_NULL(strstr(g_out[0], "p2p kv miss nokey at beta"));
    TEST_ASSERT_NOT_NULL(strstr(g_out[0], "p2p kv miss nokey at gamma"));
}

void test_consensus_commit_reject_timeout(void) {
    setup(3, 0);
    run(3, 300);
    TEST_ASSERT_EQUAL(2, p2p_propose(&g_n[0], &g_h[0], "leader", "alpha"));
    run(3, 60);
    TEST_ASSERT_NOT_NULL(strstr(g_out[0], "p2p propose committed leader=alpha"));
    TEST_ASSERT_EQUAL(1, g_n[0].prop.result);
    TEST_ASSERT_EQUAL_STRING("alpha", p2p_get_local(&g_n[2], "leader")->value);
    TEST_ASSERT_NOT_NULL(strstr(g_out[1], "p2p kv committed leader=alpha"));
    /* both peers already at a newer version: they vote no */
    g_cut[1][0] = g_cut[2][0] = 1; /* alpha hears nothing */
    TEST_ASSERT_EQUAL(2, p2p_put(&g_n[1], &g_h[1], "leader", "beta"));
    run(3, 30);
    TEST_ASSERT_EQUAL(2, p2p_put(&g_n[1], &g_h[1], "leader", "beta2"));
    run(3, 30);
    g_cut[1][0] = g_cut[2][0] = 0;
    /* alpha still holds version 1: proposes version 2, which loses */
    TEST_ASSERT_EQUAL_STRING("alpha", p2p_get_local(&g_n[0], "leader")->value);
    TEST_ASSERT_EQUAL(2, p2p_propose(&g_n[0], &g_h[0], "leader", "again"));
    run(3, 60);
    TEST_ASSERT_EQUAL(-1, g_n[0].prop.result);
    TEST_ASSERT_NOT_NULL(strstr(g_out[0], "p2p propose rejected leader=again"));
    /* no answer at all: timeout */
    g_cut[1][0] = g_cut[2][0] = 1;
    TEST_ASSERT_EQUAL(2, p2p_propose(&g_n[0], &g_h[0], "x", "y"));
    run(3, 1600);
    TEST_ASSERT_EQUAL(-2, g_n[0].prop.result);
}

void test_failure_detection_and_relay(void) {
    char rep[2048];
    setup(3, 0);
    run(3, 400);
    /* link alpha <-> gamma fails; beta still sees both */
    g_cut[0][2] = g_cut[2][0] = 1;
    run(3, 1200);
    TEST_ASSERT_FALSE(p2p_find(&g_n[0], "gamma")->up);
    TEST_ASSERT_NOT_NULL(strstr(g_out[0], "p2p peer gamma down"));
    TEST_ASSERT_TRUE(p2p_find(&g_n[0], "beta")->up);
    TEST_ASSERT_EQUAL(0, p2p_send_text(&g_n[0], &g_h[0], "gamma", "via beta"));
    run(3, 30);
    TEST_ASSERT_NOT_NULL(strstr(g_out[2], "p2p msg from alpha: via beta (relayed)"));
    TEST_ASSERT_EQUAL(1, (int)g_n[1].relayed);
    p2p_report(&g_n[0], &g_h[0], "health", rep, sizeof(rep));
    TEST_ASSERT_NOT_NULL(strstr(rep, "p2p health gamma down"));
    TEST_ASSERT_NOT_NULL(strstr(rep, "down_events 1"));
    TEST_ASSERT_NOT_NULL(strstr(rep, "p2p health beta up rtt_ms "));
    /* link heals: gamma comes back up directly */
    g_cut[0][2] = g_cut[2][0] = 0;
    run(3, 300);
    TEST_ASSERT_TRUE(p2p_find(&g_n[0], "gamma")->up);
    /* manual block (p2p-block) behaves like a cut link on our side */
    TEST_ASSERT_EQUAL(0, p2p_block(&g_n[0], "beta", 1));
    run(3, 1200);
    TEST_ASSERT_FALSE(p2p_find(&g_n[0], "beta")->up);
    TEST_ASSERT_EQUAL(-1, p2p_block(&g_n[0], "nobody", 1));
}

void test_rate_limit_and_stats(void) {
    char rep[2048];
    int i, ok = 0, throttled = 0;
    setup(2, 0);
    run(2, 300);
    g_n[0].rate_limit = 3;
    for (i = 0; i < 6; i++) {
        int rc = p2p_send_text(&g_n[0], &g_h[0], "beta", "burst");
        if (rc == 0) ok++; else if (rc == -2) throttled++;
    }
    TEST_ASSERT_EQUAL(3, ok);
    TEST_ASSERT_EQUAL(3, throttled);
    TEST_ASSERT_EQUAL(3, (int)p2p_find(&g_n[0], "beta")->throttled);
    run(2, 120);
    TEST_ASSERT_EQUAL(0, p2p_send_text(&g_n[0], &g_h[0], "beta", "later"));
    run(2, 20);
    p2p_report(&g_n[1], &g_h[1], "stats", rep, sizeof(rep));
    TEST_ASSERT_NOT_NULL(strstr(rep, "p2p stats msg tx 0 rx 4"));
    TEST_ASSERT_NOT_NULL(strstr(rep, "p2p stats hello tx "));
    TEST_ASSERT_NOT_NULL(strstr(rep, "p2p stats ping"));
    TEST_ASSERT_NOT_NULL(strstr(rep, "p2p stats ok tx_bytes "));
    TEST_ASSERT_EQUAL(-1, p2p_send_text(&g_n[0], &g_h[0], "nobody", "x"));
}

void test_traffic_analysis(void) {
    char rep[2048];
    int i;
    setup(2, 0);
    run(2, 300);
    p2p_report(&g_n[0], &g_h[0], "analyze", rep, sizeof(rep));
    TEST_ASSERT_NOT_NULL(strstr(rep, "p2p analyze ok findings 0 verdict healthy"));
    g_n[0].rate_limit = 2;
    for (i = 0; i < 5; i++) (void)p2p_send_text(&g_n[0], &g_h[0], "beta", "burst");
    p2p_report(&g_n[0], &g_h[0], "analyze", rep, sizeof(rep));
    TEST_ASSERT_NOT_NULL(strstr(rep, "p2p analyze finding beta throttled 3 (sender above p2p-limit)"));
    TEST_ASSERT_NOT_NULL(strstr(rep, "verdict degraded"));
    /* a peer that stops answering is reported as unreachable */
    g_cut[1][0] = 1;
    run(2, 1500);
    p2p_report(&g_n[0], &g_h[0], "analyze", rep, sizeof(rep));
    TEST_ASSERT_NOT_NULL(strstr(rep, "p2p analyze finding beta down-for-s "));
    g_n[0].bad_hello = 2;
    p2p_report(&g_n[0], &g_h[0], "analyze", rep, sizeof(rep));
    TEST_ASSERT_NOT_NULL(strstr(rep, "p2p analyze finding node bad-hello 2"));
    TEST_ASSERT_NOT_NULL(strstr(rep, "verdict suspicious"));
    g_cut[1][0] = 0;
}

void test_bad_inputs(void) {
    uint8_t ip[4] = {10, 77, 0, 9}, seed[32] = {0};
    p2p_node_t n;
    TEST_ASSERT_EQUAL(-1, p2p_up(&n, "", ip, "mohhdy-net-key", seed, &g_h[0]));
    TEST_ASSERT_EQUAL(-1, p2p_up(&n, "x", ip, "short", seed, &g_h[0]));
    TEST_ASSERT_EQUAL(-1, p2p_up(&n, "a-very-long-node-name", ip, "mohhdy-net-key", seed, &g_h[0]));
    setup(2, 0);
    run(2, 300);
    TEST_ASSERT_EQUAL(-1, p2p_put(&g_n[0], &g_h[0], "", "v"));
    TEST_ASSERT_EQUAL(-1, p2p_put(&g_n[0], &g_h[0], "k", "a value that is much too long to be stored in one item slot"));
    /* garbage datagrams are counted and ignored */
    inject(0, 1, (const uint8_t*)"hello world, not p2p at all....", 30);
    run(2, 20);
    TEST_ASSERT_EQUAL(1, (int)g_n[0].foreign);
}

void test_raft_and_cache(void) {
    char rep[512];
    setup(3, 0);
    run(3, 300);
    /* Trigger Raft election */
    TEST_ASSERT_EQUAL(2, p2p_raft_elect(&g_n[0], &g_h[0]));
    run(3, 80);
    TEST_ASSERT_EQUAL(P2P_RAFT_LEADER, g_n[0].raft.role);
    p2p_report(&g_n[0], &g_h[0], "raft", rep, sizeof(rep));
    TEST_ASSERT_NOT_NULL(strstr(rep, "p2p raft role leader term 2"));

    /* Cache test */
    TEST_ASSERT_EQUAL(0, p2p_cache_put(&g_n[0], "k1", "v1", 100, g_ticks));
    TEST_ASSERT_NOT_NULL(p2p_cache_get(&g_n[0], "k1", g_ticks));
    TEST_ASSERT_NULL(p2p_cache_get(&g_n[0], "k1", g_ticks + 200)); /* expired */
    p2p_report(&g_n[0], &g_h[0], "cache", rep, sizeof(rep));
    TEST_ASSERT_NOT_NULL(strstr(rep, "p2p cache hits 1 misses 1"));
}

void test_raft_log_catchup_and_typed_rpc(void) {
    const p2p_item_t* it;
    uint8_t blob[12];
    setup(3, 0);
    run(3, 300);
    TEST_ASSERT_EQUAL(2, p2p_raft_elect(&g_n[0], &g_h[0]));
    run(3, 80);
    TEST_ASSERT_EQUAL(P2P_RAFT_LEADER, g_n[0].raft.role);
    TEST_ASSERT_EQUAL(1, p2p_raft_append(&g_n[0], &g_h[0], "tone", "blue"));
    run(3, 40);
    TEST_ASSERT_NOT_NULL(strstr(g_out[1], "p2p raft commit index 1 tone=blue"));
    TEST_ASSERT_NOT_NULL(strstr(g_out[2], "p2p raft commit index 1 tone=blue"));
    it = p2p_get_local(&g_n[2], "tone");
    TEST_ASSERT_NOT_NULL(it);
    TEST_ASSERT_EQUAL_STRING("blue", it->value);
    /* Partition gamma: frames from alpha and beta never arrive. */
    g_cut[0][2] = 1;
    g_cut[1][2] = 1;
    TEST_ASSERT_EQUAL(2, p2p_raft_append(&g_n[0], &g_h[0], "late", "yes"));
    run(3, 40);
    TEST_ASSERT_NOT_NULL(p2p_get_local(&g_n[1], "late"));
    TEST_ASSERT_NULL(p2p_get_local(&g_n[2], "late"));
    g_cut[0][2] = 0;
    g_cut[1][2] = 0;
    run(3, 80);
    it = p2p_get_local(&g_n[2], "late");
    TEST_ASSERT_NOT_NULL(it);
    TEST_ASSERT_EQUAL_STRING("yes", it->value);
    TEST_ASSERT_EQUAL(12, p2p_raft_vote_export(&g_n[0], blob, 12));
    TEST_ASSERT_EQUAL(0, p2p_raft_vote_import(&g_n[1], blob, 12));
    TEST_ASSERT_EQUAL(g_n[0].raft.term, g_n[1].raft.term);
    TEST_ASSERT_EQUAL(g_n[0].raft.voted_for, g_n[1].raft.voted_for);

    g_out[0][0] = 0;
    TEST_ASSERT_EQUAL(0, p2p_grpc_call(&g_n[0], &g_h[0], "beta", "echo.EchoService", "SayHello", "World"));
    run(3, 30);
    TEST_ASSERT_NOT_NULL(strstr(g_out[0], "p2p grpc resp status 0 payload Hello World from beta id "));
    TEST_ASSERT_EQUAL(0, p2p_grpc_call_deadline(&g_n[0], &g_h[0], "beta", "echo.EchoService", "SayHello", "World",
                                                P2P_GRPC_DEADLINE_TICKS, 1));
    run(3, 30);
    TEST_ASSERT_TRUE(g_n[1].rpc_replays >= 1U);
    g_out[0][0] = 0;
    TEST_ASSERT_EQUAL(0, p2p_grpc_call_deadline(&g_n[0], &g_h[0], "beta", "echo.EchoService", "SayHello", "World", 0, 0));
    run(3, 20);
    TEST_ASSERT_NOT_NULL(strstr(g_out[0], "status 4"));
}

void test_raft_prevote_does_not_bump_a_healthy_term(void) {
    uint32_t term;
    setup(3, 0);
    run(3, 300);
    TEST_ASSERT_EQUAL(2, p2p_raft_elect(&g_n[0], &g_h[0]));
    run(3, 80);
    TEST_ASSERT_EQUAL(P2P_RAFT_LEADER, g_n[0].raft.role);
    TEST_ASSERT_NOT_NULL(strstr(g_out[0], "p2p raft prevote started"));
    term = g_n[0].raft.term;
    TEST_ASSERT_EQUAL(2, (int)term);
    /* gamma still holds the leader lease, so it does not campaign at all. */
    TEST_ASSERT_EQUAL(-2, p2p_raft_elect(&g_n[2], &g_h[2]));
    TEST_ASSERT_NOT_NULL(strstr(g_out[2], "p2p raft lease held"));
    TEST_ASSERT_EQUAL(term, g_n[2].raft.term);
    /* Heartbeats keep the leader lease alive past the original 10 s. */
    {
        char rep[2048];
        run(3, P2P_RAFT_LEASE_TICKS + (2U * P2P_HZ));
        p2p_report(&g_n[0], &g_h[0], "raft", rep, (int)sizeof(rep));
        TEST_ASSERT_NOT_NULL(strstr(rep, " lease 1"));
        g_out[2][0] = 0;
        TEST_ASSERT_EQUAL(-2, p2p_raft_elect(&g_n[2], &g_h[2]));
        TEST_ASSERT_NOT_NULL(strstr(g_out[2], "p2p raft lease held"));
    }
    /* Past the lease, still inside the election timeout. A pre-vote is sent
     * and refused, and the cluster term stays put. */
    g_ticks += P2P_RAFT_LEASE_TICKS + (5U * P2P_HZ);
    g_out[2][0] = 0;
    TEST_ASSERT_EQUAL(2, p2p_raft_elect(&g_n[2], &g_h[2]));
    TEST_ASSERT_NOT_NULL(strstr(g_out[2], "p2p raft prevote started"));
    run(3, 80);
    TEST_ASSERT_EQUAL(term, g_n[0].raft.term);
    TEST_ASSERT_EQUAL(term, g_n[1].raft.term);
    TEST_ASSERT_EQUAL(term, g_n[2].raft.term);
    TEST_ASSERT_EQUAL(P2P_RAFT_LEADER, g_n[0].raft.role);
    TEST_ASSERT_TRUE(g_n[2].raft.role != P2P_RAFT_LEADER);
}

void test_raft_snapshot_installs_for_a_lagging_follower(void) {
    const p2p_item_t* it;
    setup(3, 0);
    run(3, 300);
    TEST_ASSERT_EQUAL(2, p2p_raft_elect(&g_n[0], &g_h[0]));
    run(3, 80);
    TEST_ASSERT_EQUAL(P2P_RAFT_LEADER, g_n[0].raft.role);
    TEST_ASSERT_EQUAL(1, p2p_raft_append(&g_n[0], &g_h[0], "tone", "blue"));
    run(3, 40);
    TEST_ASSERT_NOT_NULL(p2p_get_local(&g_n[2], "tone"));
    g_cut[0][2] = 1;
    g_cut[1][2] = 1;
    TEST_ASSERT_EQUAL(2, p2p_raft_append(&g_n[0], &g_h[0], "late", "yes"));
    run(3, 40);
    TEST_ASSERT_NOT_NULL(p2p_get_local(&g_n[1], "late"));
    TEST_ASSERT_NULL(p2p_get_local(&g_n[2], "late"));
    TEST_ASSERT_EQUAL(2, p2p_raft_compact(&g_n[0]));
    TEST_ASSERT_EQUAL(3, p2p_raft_append(&g_n[0], &g_h[0], "next", "ok"));
    g_cut[0][2] = 0;
    g_cut[1][2] = 0;
    g_out[2][0] = 0;
    run(3, 160);
    TEST_ASSERT_NOT_NULL(strstr(g_out[2], "p2p raft snapshot installed index 2"));
    it = p2p_get_local(&g_n[2], "late");
    TEST_ASSERT_NOT_NULL(it);
    TEST_ASSERT_EQUAL_STRING("yes", it->value);
    it = p2p_get_local(&g_n[2], "next");
    TEST_ASSERT_NOT_NULL(it);
    TEST_ASSERT_EQUAL_STRING("ok", it->value);
    TEST_ASSERT_EQUAL(3, (int)g_n[2].raft.commit_index);
    TEST_ASSERT_EQUAL(2, (int)g_n[2].raft.snapshot_index);
}

void test_qos_window_and_cache_invalidate(void) {
    char rep[512];
    int i, ok = 0, throttled = 0;
    setup(2, 0);
    run(2, 300);
    TEST_ASSERT_EQUAL(P2P_QOS_WINDOW_MAX, g_n[0].qos.window_size);
    TEST_ASSERT_EQUAL(0, p2p_cache_put(&g_n[1], "k", "v", 500, g_ticks));
    TEST_ASSERT_NOT_NULL(p2p_cache_get(&g_n[1], "k", g_ticks));
    TEST_ASSERT_EQUAL(1, p2p_cache_invalidate(&g_n[0], &g_h[0], "k"));
    run(2, 30);
    TEST_ASSERT_NOT_NULL(strstr(g_out[1], "p2p cache invalidated k"));
    TEST_ASSERT_NULL(p2p_cache_get(&g_n[1], "k", g_ticks));
    p2p_qos_note_loss(&g_n[0]);
    TEST_ASSERT_EQUAL(P2P_QOS_WINDOW_MAX / 2U, g_n[0].qos.window_size);
    p2p_qos_note_ack(&g_n[0]);
    TEST_ASSERT_EQUAL(P2P_QOS_WINDOW_MAX / 2U + 1U, g_n[0].qos.window_size);
    p2p_qos_note_loss(&g_n[0]);
    p2p_qos_note_loss(&g_n[0]);
    TEST_ASSERT_TRUE(g_n[0].qos.window_size < P2P_QOS_WINDOW_MAX);
    for (i = 0; i < (int)g_n[0].qos.window_size + 2; i++) {
        int rc = p2p_send_text(&g_n[0], &g_h[0], "beta", "burst");
        if (rc == 0) ok++; else if (rc == -2) throttled++;
    }
    TEST_ASSERT_EQUAL((int)g_n[0].qos.window_size, ok);
    TEST_ASSERT_EQUAL(2, throttled);
    TEST_ASSERT_TRUE(g_n[0].qos.priority_low_tx >= (uint32_t)ok);
    p2p_report(&g_n[0], &g_h[0], "analyze", rep, sizeof(rep));
    TEST_ASSERT_NOT_NULL(strstr(rep, "p2p analyze finding node congestion "));
    TEST_ASSERT_NOT_NULL(strstr(rep, "verdict degraded"));
}

void test_raft_compact_past_physical_window(void) {
    int i;
    char val[16];
    const p2p_item_t* it;
    setup(3, 0);
    run(3, 300);
    TEST_ASSERT_EQUAL(2, p2p_raft_elect(&g_n[0], &g_h[0]));
    run(3, 80);
    TEST_ASSERT_EQUAL(P2P_RAFT_LEADER, g_n[0].raft.role);
    for (i = 0; i < P2P_RAFT_LOG; i++) {
        snprintf(val, sizeof(val), "v%d", i);
        TEST_ASSERT_EQUAL(i + 1, p2p_raft_append(&g_n[0], &g_h[0], "k", val));
        run(3, 60);
    }
    TEST_ASSERT_EQUAL(P2P_RAFT_LOG, (int)g_n[0].raft.log_count);
    TEST_ASSERT_EQUAL(P2P_RAFT_LOG, (int)g_n[0].raft.last_applied);
    TEST_ASSERT_EQUAL(P2P_RAFT_LOG, (int)g_n[1].raft.last_applied);
    TEST_ASSERT_EQUAL(P2P_RAFT_LOG, (int)g_n[2].raft.last_applied);
    TEST_ASSERT_EQUAL(-1, p2p_raft_append(&g_n[0], &g_h[0], "k", "overflow"));
    TEST_ASSERT_EQUAL(P2P_RAFT_LOG, p2p_raft_compact(&g_n[0]));
    TEST_ASSERT_EQUAL(P2P_RAFT_LOG, p2p_raft_compact(&g_n[1]));
    TEST_ASSERT_EQUAL(P2P_RAFT_LOG, p2p_raft_compact(&g_n[2]));
    TEST_ASSERT_EQUAL(P2P_RAFT_LOG + 1, p2p_raft_append(&g_n[0], &g_h[0], "k", "past"));
    run(3, 80);
    it = p2p_get_local(&g_n[1], "k");
    TEST_ASSERT_NOT_NULL(it);
    TEST_ASSERT_EQUAL_STRING("past", it->value);
    it = p2p_get_local(&g_n[2], "k");
    TEST_ASSERT_NOT_NULL(it);
    TEST_ASSERT_EQUAL_STRING("past", it->value);
    TEST_ASSERT_EQUAL(P2P_RAFT_LOG + 1, (int)g_n[2].raft.commit_index);
}

void test_routing_grpc_and_stun(void) {
    char rep[512];
    uint32_t nh = 0, cost = 0;
    setup(3, 0);
    run(3, 300);

    /* Test US-063 Routing cost */
    TEST_ASSERT_EQUAL(0, p2p_route_find(&g_n[0], g_n[1].id, &nh, &cost));
    TEST_ASSERT_EQUAL(0, (int)nh);
    TEST_ASSERT_GREATER_THAN(0, (int)cost);

    p2p_report(&g_n[0], &g_h[0], "route", rep, sizeof(rep));
    TEST_ASSERT_NOT_NULL(strstr(rep, "p2p route dest beta via direct cost "));

    /* Test US-067 gRPC call */
    TEST_ASSERT_EQUAL(0, p2p_grpc_call(&g_n[0], &g_h[0], "beta", "echo.EchoService", "SayHello", "World"));
    run(3, 30);
    TEST_ASSERT_NOT_NULL(strstr(g_out[0], "p2p grpc resp status 0 payload Hello World from beta"));

    /* Test US-074 STUN probe & NAT punch */
    TEST_ASSERT_EQUAL(0, p2p_stun_probe(&g_n[0], &g_h[0], "beta"));
    run(3, 30);
    TEST_ASSERT_NOT_NULL(strstr(g_out[0], "p2p stun mapped_ip 10.77.0.1 port "));
    p2p_report(&g_n[0], &g_h[0], "stun", rep, sizeof(rep));
    TEST_ASSERT_NOT_NULL(strstr(rep, "p2p nat type full_cone mapped_ip 10.77.0.1"));

    TEST_ASSERT_EQUAL(0, p2p_nat_punch(&g_n[0], &g_h[0], "beta"));
    run(3, 30);
    TEST_ASSERT_NOT_NULL(strstr(g_out[1], "p2p nat punch from alpha pinhole opened"));
}

void test_p2p_consolidation_and_fallback(void) {
    char rep[512];
    uint32_t nh = 0, cost = 0;
    setup(3, 0);
    run(3, 300);

    /* Unreachable peer gRPC fallback */
    TEST_ASSERT_EQUAL(-1, p2p_grpc_call(&g_n[0], &g_h[0], "nobody", "svc", "method", "data"));
    TEST_ASSERT_EQUAL(-1, p2p_stun_probe(&g_n[0], &g_h[0], "nobody"));
    TEST_ASSERT_EQUAL(-1, p2p_nat_punch(&g_n[0], &g_h[0], "nobody"));

    /* Cut direct link alpha-gamma, route fallback to relayed via beta */
    g_cut[0][2] = g_cut[2][0] = 1;
    run(3, 1200);
    TEST_ASSERT_EQUAL(0, p2p_route_find(&g_n[0], g_n[2].id, &nh, &cost));
    TEST_ASSERT_EQUAL((int)g_n[1].id, (int)nh); /* next hop is beta */

    p2p_report(&g_n[0], &g_h[0], "stats", rep, sizeof(rep));
    TEST_ASSERT_NOT_NULL(strstr(rep, "p2p stats ok"));
}

/* US-063: chain alpha - beta - gamma - delta after discovery; alpha
 * reaches delta through two relays from the distance vector in HELLO. */
void test_multi_relay_chain(void) {
    char rep[1024];
    setup(4, 0);
    run(4, 400);
    TEST_ASSERT_TRUE(p2p_find(&g_n[0], "delta")->keyed);
    g_cut[0][2] = g_cut[2][0] = 1;
    g_cut[0][3] = g_cut[3][0] = 1;
    g_cut[1][3] = g_cut[3][1] = 1;
    run(4, 1500);
    TEST_ASSERT_FALSE(p2p_find(&g_n[0], "delta")->up);
    TEST_ASSERT_FALSE(p2p_find(&g_n[0], "gamma")->up);
    TEST_ASSERT_EQUAL(0, p2p_send_text(&g_n[0], &g_h[0], "delta", "two relays"));
    run(4, 40);
    TEST_ASSERT_NOT_NULL(strstr(g_out[3], "p2p msg from alpha: two relays (relayed)"));
    TEST_ASSERT_EQUAL(1, (int)g_n[1].relayed_multi);
    TEST_ASSERT_TRUE(g_n[2].relayed >= 1U);
    TEST_ASSERT_EQUAL(0, (int)g_n[2].relayed_multi);
    /* and back: delta reaches alpha the same way */
    TEST_ASSERT_EQUAL(0, p2p_send_text(&g_n[3], &g_h[3], "alpha", "back"));
    run(4, 40);
    TEST_ASSERT_NOT_NULL(strstr(g_out[0], "p2p msg from delta: back (relayed)"));
    /* beyond P2P_MAX_HOPS nothing is advertised: a 4-hop chain is not routed */
    p2p_report(&g_n[1], &g_h[1], "cache", rep, sizeof(rep));
    TEST_ASSERT_NOT_NULL(strstr(rep, "relayed_multi 2"));
}

/* US-068/073: delay-based and loss-based window control. */
void test_qos_adaptive_controller(void) {
    char rep[512];
    uint32_t w;
    int i;
    setup(2, 0);
    run(2, 1000);
    TEST_ASSERT_EQUAL(P2P_QOS_WINDOW_MAX, g_n[0].qos.window_size);
    TEST_ASSERT_TRUE(g_n[0].qos.rtt_min_ms > 0U);
    /* queueing delay: sample far above the minimum cuts a quarter */
    p2p_qos_note_rtt(&g_n[0], g_n[0].qos.rtt_min_ms * 2U + 200U);
    TEST_ASSERT_EQUAL(P2P_QOS_WINDOW_MAX - P2P_QOS_WINDOW_MAX / 4U, g_n[0].qos.window_size);
    TEST_ASSERT_EQUAL(1, (int)g_n[0].qos.delay_backoffs);
    /* unanswered PINGs: cut the link for two ping rounds (below down time) */
    g_cut[1][0] = 1;
    run(2, 700);
    g_cut[1][0] = 0;
    TEST_ASSERT_TRUE(g_n[0].qos.ping_losses >= 1U);
    w = g_n[0].qos.window_size;
    TEST_ASSERT_TRUE(w < P2P_QOS_WINDOW_MAX / 2U);
    TEST_ASSERT_TRUE(p2p_find(&g_n[0], "beta")->up);
    /* recovery: clean round trips grow the window again */
    run(2, 3000);
    TEST_ASSERT_TRUE(g_n[0].qos.window_size > w);
    for (i = 0; i < 20; i++) p2p_qos_note_rtt(&g_n[0], g_n[0].qos.rtt_min_ms);
    TEST_ASSERT_EQUAL(P2P_QOS_WINDOW_MAX, g_n[0].qos.window_size);
    TEST_ASSERT_TRUE(g_n[0].qos.rate_est_bps > 0U);
    p2p_report(&g_n[0], &g_h[0], "qos", rep, sizeof(rep));
    TEST_ASSERT_NOT_NULL(strstr(rep, "p2p qos window 16 srtt_ms "));
    TEST_ASSERT_NOT_NULL(strstr(rep, " delay_backoffs 1 "));
}

/* US-070: versioned invalidation, fences against stale fills, acks and
 * re-send to a peer that missed the first INV. */
void test_cache_versioned_acked_invalidation(void) {
    char rep[512];
    setup(3, 0);
    run(3, 400);
    TEST_ASSERT_EQUAL(0, p2p_cache_put_v(&g_n[1], "cfg", "old", 4, 5000, g_ticks));
    TEST_ASSERT_EQUAL(0, p2p_cache_put_v(&g_n[2], "cfg", "old", 4, 5000, g_ticks));
    /* gamma misses the first round */
    g_cut[0][2] = 1;
    TEST_ASSERT_EQUAL(2, p2p_cache_invalidate_v(&g_n[0], &g_h[0], "cfg", 5));
    run(3, 50);
    TEST_ASSERT_NULL(p2p_cache_get(&g_n[1], "cfg", g_ticks));
    TEST_ASSERT_NOT_NULL(p2p_cache_get(&g_n[2], "cfg", g_ticks));
    TEST_ASSERT_EQUAL(1, (int)g_n[0].cache.inv_active);
    g_cut[0][2] = 0;
    run(3, 1200);
    TEST_ASSERT_NULL(p2p_cache_get(&g_n[2], "cfg", g_ticks));
    TEST_ASSERT_EQUAL(0, (int)g_n[0].cache.inv_active);
    TEST_ASSERT_TRUE(g_n[0].cache.inv_resent >= 1U);
    TEST_ASSERT_NOT_NULL(strstr(g_out[0], "p2p cache inv acked cfg v5 acks 2"));
    /* a late fill at or below the fence is refused, a newer one is kept */
    TEST_ASSERT_EQUAL(-2, p2p_cache_put_v(&g_n[1], "cfg", "old", 4, 5000, g_ticks));
    TEST_ASSERT_EQUAL(-2, p2p_cache_put_v(&g_n[2], "cfg", "old", 5, 5000, g_ticks));
    TEST_ASSERT_EQUAL(0, p2p_cache_put_v(&g_n[2], "cfg", "new", 6, 5000, g_ticks));
    TEST_ASSERT_EQUAL_STRING("new", p2p_cache_get(&g_n[2], "cfg", g_ticks)->value);
    p2p_report(&g_n[2], &g_h[2], "cache", rep, sizeof(rep));
    TEST_ASSERT_NOT_NULL(strstr(rep, "stale_refused 1"));
    /* a peer that never answers: the sender gives up and says so */
    g_cut[0][1] = 1;
    TEST_ASSERT_EQUAL(2, p2p_cache_invalidate_v(&g_n[0], &g_h[0], "cfg", 7));
    run(3, 8000);
    TEST_ASSERT_NOT_NULL(strstr(g_out[0], "p2p cache inv incomplete cfg acks 1"));
}

/* US-075: learned per-type baseline, a burst far above it is flagged. */
void test_traffic_baseline_anomaly(void) {
    char rep[2048];
    int i;
    setup(2, 0);
    run(2, 8000);
    TEST_ASSERT_EQUAL(0, (int)g_n[1].anomalies);
    p2p_report(&g_n[1], &g_h[1], "analyze", rep, sizeof(rep));
    TEST_ASSERT_NULL(strstr(rep, "rate-anomaly"));
    for (i = 0; i < 14; i++) {
        TEST_ASSERT_EQUAL(0, p2p_send_text(&g_n[0], &g_h[0], "beta", "burst"));
        run(2, 10);
    }
    run(2, 1100);
    TEST_ASSERT_TRUE(g_n[1].anomalies >= 1U);
    TEST_ASSERT_EQUAL(P2P_I_MSG, (int)g_n[1].last_anomaly_type);
    p2p_report(&g_n[1], &g_h[1], "analyze", rep, sizeof(rep));
    TEST_ASSERT_NOT_NULL(strstr(rep, "p2p analyze finding node rate-anomaly "));
    TEST_ASSERT_NOT_NULL(strstr(rep, "p2p analyze anomaly type msg rate 14"));
    TEST_ASSERT_NOT_NULL(strstr(rep, "verdict suspicious"));
}

int main(void) {
    unity_init();
    RUN_TEST(test_discovery_and_keys);
    RUN_TEST(test_multi_relay_chain);
    RUN_TEST(test_qos_adaptive_controller);
    RUN_TEST(test_cache_versioned_acked_invalidation);
    RUN_TEST(test_traffic_baseline_anomaly);
    RUN_TEST(test_wrong_network_key_is_kept_out);
    RUN_TEST(test_encrypted_message_tamper_and_replay);
    RUN_TEST(test_replication_lww_get_and_sync);
    RUN_TEST(test_consensus_commit_reject_timeout);
    RUN_TEST(test_failure_detection_and_relay);
    RUN_TEST(test_rate_limit_and_stats);
    RUN_TEST(test_traffic_analysis);
    RUN_TEST(test_bad_inputs);
    RUN_TEST(test_raft_and_cache);
    RUN_TEST(test_raft_log_catchup_and_typed_rpc);
    RUN_TEST(test_raft_prevote_does_not_bump_a_healthy_term);
    RUN_TEST(test_raft_compact_past_physical_window);
    RUN_TEST(test_raft_snapshot_installs_for_a_lagging_follower);
    RUN_TEST(test_qos_window_and_cache_invalidate);
    RUN_TEST(test_routing_grpc_and_stun);
    RUN_TEST(test_p2p_consolidation_and_fallback);
    unity_print_results();
    unity_cleanup();
    return (unity_stats.tests_failed == 0) ? 0 : 1;
}
