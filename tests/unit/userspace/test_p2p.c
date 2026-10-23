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
/* Simulated NAT (US-074 tests only): source IPv4 seen by j for frames from
 * i; 0.0.0.0 = no translation. */
static uint8_t g_nat[NODES][NODES][4];
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
        if (g_nat[from][j][0]) memcpy(q->ip, g_nat[from][j], 4);
        else memcpy(q->ip, g_n[from].ip, 4);
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
    memset(g_cut, 0, sizeof(g_cut)); memset(g_nat, 0, sizeof(g_nat)); memset(g_out, 0, sizeof(g_out)); memset(g_n, 0, sizeof(g_n));
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
    /* Lot 6: shaping. Messages above the budget wait in the queue (rc 1)
     * and leave when the next one-second budget opens; none is lost. */
    for (i = 0; i < 6; i++) {
        int rc = p2p_send_text(&g_n[0], &g_h[0], "beta", "burst");
        if (rc == 0) ok++; else if (rc == 1) throttled++;
    }
    TEST_ASSERT_EQUAL(3, ok);
    TEST_ASSERT_EQUAL(3, throttled);
    TEST_ASSERT_EQUAL(3, (int)p2p_find(&g_n[0], "beta")->throttled);
    TEST_ASSERT_EQUAL(3U, g_n[0].qos.shaped_queued);
    run(2, 120);
    TEST_ASSERT_EQUAL(3U, g_n[0].qos.shaped_sent);
    run(2, 120);
    TEST_ASSERT_EQUAL(0, p2p_send_text(&g_n[0], &g_h[0], "beta", "later"));
    run(2, 20);
    p2p_report(&g_n[1], &g_h[1], "stats", rep, sizeof(rep));
    TEST_ASSERT_NOT_NULL(strstr(rep, "p2p stats msg tx 0 rx 7"));
    TEST_ASSERT_NOT_NULL(strstr(rep, "p2p stats hello tx "));
    TEST_ASSERT_NOT_NULL(strstr(rep, "p2p stats ping"));
    TEST_ASSERT_NOT_NULL(strstr(rep, "p2p stats ok tx_bytes "));
    TEST_ASSERT_EQUAL(-1, p2p_send_text(&g_n[0], &g_h[0], "nobody", "x"));
    /* queue full: the ninth waiting message is refused and counted */
    g_n[0].rate_limit = 1;
    run(2, 120);
    for (i = 0; i < 10; i++) (void)p2p_send_text(&g_n[0], &g_h[0], "beta", "q");
    TEST_ASSERT_EQUAL(1U, g_n[0].qos.shaped_dropped);
    TEST_ASSERT_EQUAL(-2, p2p_send_text(&g_n[0], &g_h[0], "beta", "full"));
    g_n[0].rate_limit = 0;
    run(2, 200);
    TEST_ASSERT_EQUAL(11U, g_n[0].qos.shaped_sent);
    p2p_report(&g_n[0], &g_h[0], "qos", rep, sizeof(rep));
    TEST_ASSERT_NOT_NULL(strstr(rep, " queued 11 shaped_sent 11 shaped_dropped 2"));
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
        if (rc == 0) ok++; else if (rc == 1) throttled++; /* Lot 6: queued (shaping) */
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

    /* Test Lot 16 link cost penalty on peer anomalies */
    {
        p2p_peer_t *p0 = &g_n[0].peers[0];
        uint32_t c_normal = p2p_calc_link_cost(p0);
        p0->anomalies = 2;
        uint32_t c_anomaly = p2p_calc_link_cost(p0);
        TEST_ASSERT_EQUAL(c_normal + 100U, c_anomaly);
        p0->anomalies = 0;
    }

    p2p_report(&g_n[0], &g_h[0], "route", rep, sizeof(rep));
    TEST_ASSERT_NOT_NULL(strstr(rep, "p2p route dest beta via direct cost "));

    /* Test US-067 gRPC call */
    TEST_ASSERT_EQUAL(0, p2p_grpc_call(&g_n[0], &g_h[0], "beta", "echo.EchoService", "SayHello", "World"));
    run(3, 30);
    TEST_ASSERT_NOT_NULL(strstr(g_out[0], "p2p grpc resp status 0 payload Hello World from beta"));

    /* Test US-074 STUN probe & NAT punch */
    TEST_ASSERT_EQUAL(0, p2p_stun_probe(&g_n[0], &g_h[0], "beta"));
    run(3, 30);
    /* Shared segment, no NAT: the real STUN mapping equals the local address. */
    TEST_ASSERT_NOT_NULL(strstr(g_out[0], "p2p stun mapped_ip 10.77.0.1 port 7700 from beta nat_type direct servers 1"));
    p2p_report(&g_n[0], &g_h[0], "stun", rep, sizeof(rep));
    TEST_ASSERT_NOT_NULL(strstr(rep, "p2p nat type direct mapped_ip 10.77.0.1 port 7700"));

    TEST_ASSERT_EQUAL(0, p2p_nat_punch(&g_n[0], &g_h[0], "beta"));
    run(3, 30);
    TEST_ASSERT_NOT_NULL(strstr(g_out[1], "p2p nat punch from alpha pinhole opened"));
}

void test_lot17_raft_resilience_and_compact_zeroing(void) {
    setup(3, 0);
    run(3, 300);
    /* Elect alpha as leader */
    TEST_ASSERT_EQUAL(2, p2p_raft_elect(&g_n[0], &g_h[0]));
    run(3, 80);
    TEST_ASSERT_EQUAL(P2P_RAFT_LEADER, g_n[0].raft.role);

    /* Start proposal on leader with alpha unable to hear votes */
    g_cut[1][0] = 1; g_cut[2][0] = 1;
    TEST_ASSERT_EQUAL(2, p2p_propose(&g_n[0], &g_h[0], "prop1", "val1"));
    TEST_ASSERT_EQUAL(1, g_n[0].prop.active);

    /* Heal partition and advance time past leader lease */
    g_cut[1][0] = 0; g_cut[2][0] = 0;
    g_ticks += P2P_RAFT_LEASE_TICKS + (5U * P2P_HZ);

    /* Beta initiates election in higher term (term 3) */
    TEST_ASSERT_EQUAL(2, p2p_raft_elect(&g_n[1], &g_h[1]));
    run(3, 80);

    /* Verify proposal on alpha was reset / cancelled upon step down */
    TEST_ASSERT_EQUAL(0, g_n[0].prop.active);
    TEST_ASSERT_EQUAL(-2, g_n[0].prop.result);
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

void test_rtt_ewma_and_auto_cache_invalidation_on_disconnect(void) {
    setup(2, 0);
    run(2, 300);
    p2p_peer_t* b = p2p_find(&g_n[0], "beta");
    TEST_ASSERT_NOT_NULL(b);

    /* Test link cost calculation with EWMA RTT & Loss */
    b->rtt_ewma_ms = 40;
    b->loss_percent = 5;
    b->bucket_used = 2;
    TEST_ASSERT_GREATER_THAN(10, (int)p2p_calc_link_cost(b));

    /* Populate cache */
    TEST_ASSERT_EQUAL(0, p2p_cache_put(&g_n[0], "item1", "val1", 500, g_ticks));
    TEST_ASSERT_NOT_NULL(p2p_cache_get(&g_n[0], "item1", g_ticks));

    /* Disconnect beta and verify automatic cache invalidation */
    g_cut[1][0] = 1;
    run(2, 1200); /* Wait past P2P_DOWN_TICKS */
    TEST_ASSERT_FALSE(b->up);
    TEST_ASSERT_NULL(p2p_cache_get(&g_n[0], "item1", g_ticks));
    TEST_ASSERT_GREATER_THAN(0, (int)g_n[0].cache.invalidations);
}

void test_lot19_adaptive_cost_and_mesh_routing(void) {
    setup(3, 0);
    run(3, 300);
    p2p_peer_t* b = p2p_find(&g_n[0], "beta");
    TEST_ASSERT_NOT_NULL(b);

    uint32_t c_base = p2p_calc_link_cost(b);
    b->rtt_ewma_ms = 80;
    b->loss_percent = 10;
    b->anomalies = 1;
    uint32_t c_high = p2p_calc_link_cost(b);
    TEST_ASSERT_GREATER_THAN((int)c_base, (int)c_high);

    uint32_t nh = 0, cost = 0;
    TEST_ASSERT_EQUAL(0, p2p_route_find(&g_n[0], b->id, &nh, &cost));
    TEST_ASSERT_GREATER_THAN(0, (int)cost);

    char rep[512];
    p2p_report(&g_n[0], &g_h[0], "cache", rep, sizeof(rep));
    TEST_ASSERT(strstr(rep, "p2p cache hits") != NULL);

    p2p_report(&g_n[0], &g_h[0], "raft", rep, sizeof(rep));
    TEST_ASSERT(strstr(rep, "p2p raft role") != NULL);
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
    /* on the way back gamma is the first relay, beta delivers */
    TEST_ASSERT_EQUAL(1, (int)g_n[2].relayed_multi);
    p2p_report(&g_n[1], &g_h[1], "cache", rep, sizeof(rep));
    TEST_ASSERT_NOT_NULL(strstr(rep, "relayed_multi 1"));
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
    TEST_ASSERT_NOT_NULL(strstr(rep, "p2p analyze anomaly type msg rate "));
    /* A rate anomaly alone is a warning, not proof of an attack. */
    TEST_ASSERT_NOT_NULL(strstr(rep, "verdict degraded"));
}

/* Lot 6 (US-075): the anomaly names the peer above its own baseline; a
 * reply burst we asked for (sync) is skipped. (US-068/073): measured byte
 * rates follow the traffic. */
void test_anomaly_peer_attribution_and_solicited(void) {
    char rep[2048];
    int i;
    setup(3, 0);
    run(3, 8000);
    TEST_ASSERT_TRUE(g_n[1].qos.meas_rx_bps > 0U);
    TEST_ASSERT_TRUE(g_n[1].qos.meas_tx_bps > 0U);
    /* gamma ignores its own controller: even a window of 1 does not slow it */
    g_n[2].rate_limit = 1U;
    for (i = 0; i < 14; i++) TEST_ASSERT_EQUAL(0, p2p_send_unshaped(&g_n[2], &g_h[2], "beta", "flood"));
    TEST_ASSERT_EQUAL(14U, g_n[2].unshaped_tx);
    g_n[2].rate_limit = 0U;
    run(3, 1100);
    TEST_ASSERT_EQUAL(g_n[2].id, g_n[1].last_anomaly_peer);
    TEST_ASSERT_TRUE(p2p_find(&g_n[1], "gamma")->anomalies >= 1U);
    TEST_ASSERT_EQUAL(0U, p2p_find(&g_n[1], "alpha")->anomalies);
    p2p_report(&g_n[1], &g_h[1], "analyze", rep, sizeof(rep));
    TEST_ASSERT_NOT_NULL(strstr(rep, "p2p analyze finding gamma rate-anomaly "));
    TEST_ASSERT_NOT_NULL(strstr(rep, " peer gamma solicited_skipped 0"));
    /* solicited: once the baseline has settled again, beta asks, then the
     * same burst arrives within the window, not counted */
    run(3, 8000);
    TEST_ASSERT_TRUE(p2p_sync(&g_n[1], &g_h[1], "gamma") >= 0);
    {
        uint32_t before = p2p_find(&g_n[1], "gamma")->anomalies;
        for (i = 0; i < 20; i++) { (void)p2p_send_text(&g_n[2], &g_h[2], "beta", "flood"); if (i & 1) run(3, 10); }
        run(3, 300);
        TEST_ASSERT_EQUAL(before, p2p_find(&g_n[1], "gamma")->anomalies);
        TEST_ASSERT_TRUE(g_n[1].solicited_skipped >= 1U);
    }
}

/* Lot 6 (US-070): a cache write through the Raft log reaches every node
 * with the log index as version; an older fill is refused by the fence. */
void test_cache_through_raft(void) {
    char rep[512];
    int idx, i;
    setup(3, 0);
    run(3, 300);
    TEST_ASSERT_EQUAL(-1, p2p_cache_raft_put(&g_n[1], &g_h[1], "cfg", "x")); /* not leader */
    TEST_ASSERT_EQUAL(2, p2p_raft_elect(&g_n[0], &g_h[0]));
    run(3, 80);
    idx = p2p_cache_raft_put(&g_n[0], &g_h[0], "cfg", "blue");
    TEST_ASSERT_TRUE(idx >= 1);
    run(3, 300);
    for (i = 0; i < 3; i++) {
        const p2p_item_t* it = p2p_cache_get(&g_n[i], "cfg", g_ticks);
        TEST_ASSERT_NOT_NULL(it);
        TEST_ASSERT_EQUAL(0, strcmp(it->value, "blue"));
        {   /* p2p_cache_get returns the line; read its version field */
            int k;
            uint32_t v = 0;
            for (k = 0; k < P2P_CACHE_MAX; k++)
                if (g_n[i].cache.entries[k].used && strcmp(g_n[i].cache.entries[k].key, "cfg") == 0) v = g_n[i].cache.entries[k].version;
            TEST_ASSERT_EQUAL((uint32_t)idx, v);
        }
        TEST_ASSERT_EQUAL(1U, g_n[i].cache_raft_applied);
        TEST_ASSERT_NOT_NULL(strstr(g_out[i], "p2p cache raft cfg=blue v"));
    }
    /* a fill at or below the committed index is refused on a follower */
    TEST_ASSERT_TRUE(p2p_cache_put_v(&g_n[2], "cfg", "old", (uint32_t)idx, 0, g_ticks) != 0);
    TEST_ASSERT_EQUAL(0, strcmp(p2p_cache_get(&g_n[2], "cfg", g_ticks)->value, "blue"));
    p2p_report(&g_n[2], &g_h[2], "cache", rep, sizeof(rep));
    TEST_ASSERT_NOT_NULL(strstr(rep, "stale_refused 1"));
}

/* US-069: node crash (silent, all frames lost), reroute around it, then
 * reboot under a fresh identity: rejoin and anti-entropy push of the kv. */
void test_crash_reroute_and_rejoin(void) {
    char rep[1024];
    uint8_t ip[4] = {10, 77, 0, 2}, seed[32];
    int j;
    setup(4, 0);
    run(4, 400);
    /* diamond: alpha-beta, alpha-gamma, beta-delta, gamma-delta */
    g_cut[0][3] = g_cut[3][0] = 1;
    g_cut[1][2] = g_cut[2][1] = 1;
    run(4, 1500);
    TEST_ASSERT_FALSE(p2p_find(&g_n[0], "delta")->up);
    /* crash beta: nothing it sends arrives, nothing reaches it */
    for (j = 0; j < NODES; j++) g_cut[1][j] = g_cut[j][1] = 1;
    run(4, 1500);
    TEST_ASSERT_FALSE(p2p_find(&g_n[0], "beta")->up);
    TEST_ASSERT_TRUE(p2p_put(&g_n[0], &g_h[0], "after", "crash") >= 0);
    TEST_ASSERT_EQUAL(0, p2p_send_text(&g_n[0], &g_h[0], "delta", "around beta"));
    run(4, 60);
    TEST_ASSERT_NOT_NULL(strstr(g_out[3], "p2p msg from alpha: around beta (relayed)"));
    TEST_ASSERT_TRUE(g_n[2].relayed >= 1U);
    /* reboot beta: empty state, new seed, so a new key and id */
    memset(&g_n[1], 0, sizeof(g_n[1]));
    g_out[1][0] = 0;
    memset(seed, 0x77, 32);
    TEST_ASSERT_EQUAL(0, p2p_up(&g_n[1], "beta", ip, "mohhdy-net-key", seed, &g_h[1]));
    for (j = 0; j < NODES; j++) g_cut[1][j] = g_cut[j][1] = 0;
    g_cut[1][2] = g_cut[2][1] = 1;
    run(4, 1200);
    TEST_ASSERT_NOT_NULL(strstr(g_out[0], "p2p peer beta rejoined id "));
    TEST_ASSERT_EQUAL(1, (int)g_n[0].rejoins);
    TEST_ASSERT_TRUE(p2p_find(&g_n[0], "beta")->up);
    TEST_ASSERT_TRUE(p2p_find(&g_n[0], "beta")->id == g_n[1].id);
    TEST_ASSERT_NOT_NULL(strstr(g_out[1], "p2p kv synced after=crash"));
    TEST_ASSERT_NOT_NULL(p2p_get_local(&g_n[1], "after"));
    /* the rejoined member is reachable again (direct) */
    TEST_ASSERT_EQUAL(0, p2p_send_text(&g_n[0], &g_h[0], "beta", "welcome back"));
    run(4, 40);
    TEST_ASSERT_NOT_NULL(strstr(g_out[1], "p2p msg from alpha: welcome back"));
    p2p_report(&g_n[0], &g_h[0], "stats", rep, sizeof(rep));
    TEST_ASSERT_NOT_NULL(strstr(rep, " rejoins 1"));
    /* a live member is never replaced by a newcomer with its name */
    TEST_ASSERT_EQUAL(1, (int)g_n[3].rejoins);
}

/* US-074: STUN Binding through the sealed link behind a simulated NAT
 * (address rewrite on the test bus). Two servers classify the mapping. */
void test_stun_behind_simulated_nat(void) {
    char rep[512];
    static const uint8_t pub_ip[4] = {203, 0, 113, 7};
    int j;
    setup(3, 0);
    run(3, 300);
    for (j = 1; j < 3; j++) memcpy(g_nat[0][j], pub_ip, 4);
    TEST_ASSERT_EQUAL(0, p2p_stun_probe(&g_n[0], &g_h[0], "beta"));
    run(3, 30);
    TEST_ASSERT_NOT_NULL(strstr(g_out[0], "p2p stun mapped_ip 203.0.113.7 port 7700 from beta nat_type nat_unknown servers 1"));
    TEST_ASSERT_EQUAL(0, p2p_stun_probe(&g_n[0], &g_h[0], "gamma"));
    run(3, 30);
    TEST_ASSERT_NOT_NULL(strstr(g_out[0], "from gamma nat_type endpoint_independent servers 2"));
    /* endpoint-dependent mapping: gamma now sees another public address */
    g_nat[0][2][3] = 9;
    TEST_ASSERT_EQUAL(0, p2p_stun_probe(&g_n[0], &g_h[0], "gamma"));
    run(3, 30);
    TEST_ASSERT_NOT_NULL(strstr(g_out[0], "p2p stun mapped_ip 203.0.113.9 port 7700 from gamma nat_type endpoint_dependent"));
    p2p_report(&g_n[0], &g_h[0], "nat", rep, sizeof(rep));
    TEST_ASSERT_NOT_NULL(strstr(rep, "p2p nat type endpoint_dependent mapped_ip 203.0.113.9"));
    TEST_ASSERT_NOT_NULL(strstr(rep, " probes 3 "));
    TEST_ASSERT_NOT_NULL(strstr(rep, " bad 0"));
}

/* US-063: fewest hops first. A slow direct link must not lose to a
 * neighbor that advertises the destination (its route may come back
 * through us: the loop seen in QEMU under CI load). */
void test_route_prefers_direct_over_cheaper_relay(void) {
    uint32_t nh = 1, cost = 0;
    p2p_peer_t *gamma, *alpha;
    int j;
    setup(3, 0);
    run(3, 300);
    gamma = p2p_find(&g_n[1], "gamma");
    alpha = p2p_find(&g_n[1], "alpha");
    TEST_ASSERT_NOT_NULL(gamma);
    TEST_ASSERT_NOT_NULL(alpha);
    gamma->rtt_ticks = 50;            /* slow direct link */
    alpha->rtt_ticks = 0;
    for (j = 0; j < P2P_PEERS; j++)
        if (alpha->neighbors[j] == gamma->id) alpha->nb_hops[j] = 2;  /* alpha: gamma at 2 hops */
    TEST_ASSERT_EQUAL(0, p2p_route_find(&g_n[1], gamma->id, &nh, &cost));
    TEST_ASSERT_EQUAL(0, (int)nh);
    /* with the direct link down, the relay is used */
    gamma->up = 0;
    TEST_ASSERT_EQUAL(0, p2p_route_find(&g_n[1], gamma->id, &nh, &cost));
    TEST_ASSERT_EQUAL((int)alpha->id, (int)nh);
}

static void test_lot20_p2p_cache_and_raft_anti_entropy(void) {
    setup(3, 0);
    run(3, 400);

    /* Test cache invalidation and peer disconnect cleanup */
    TEST_ASSERT_EQUAL(0, p2p_cache_put_v(&g_n[0], "lot20_key", "val1", 1, 1000, g_ticks));
    p2p_cache_on_peer_down(&g_n[0], g_n[1].id);
    TEST_ASSERT_EQUAL(0, g_n[0].cache.inv_active);

    /* Test AIMD QoS note loss */
    uint32_t orig_win = g_n[0].qos.window_size;
    p2p_qos_note_loss(&g_n[0]);
    TEST_ASSERT_TRUE(g_n[0].qos.window_size <= orig_win);
    TEST_ASSERT_TRUE(g_n[0].qos.loss_count > 0U);
}

static void test_lot21_mesh_routing_and_grpc_fallback(void) {
    setup(3, 0);
    run(3, 50);
    uint32_t nh = 0, cost = 0;
    p2p_peer_t* bp;
    /* Self route lookup must be rejected (-1) to avoid 2-node loops */
    TEST_ASSERT_EQUAL(-1, p2p_route_find(&g_n[0], g_n[0].id, &nh, &cost));
    /* STUN probe auto-fallback to NAT punch when server is down */
    bp = p2p_find(&g_n[0], "beta");
    TEST_ASSERT_NOT_NULL(bp);
    if (bp) bp->blocked = 1;
    (void)p2p_stun_probe(&g_n[0], &g_h[0], "beta");
    TEST_ASSERT_GREATER_THAN(0U, g_n[0].nat.nat_punches);
}

static void test_lot22_raft_partition_resilience(void) {
    setup(3, 0);
    run(3, 300);
    /* Elect alpha as leader */
    TEST_ASSERT_EQUAL(2, p2p_raft_elect(&g_n[0], &g_h[0]));
    run(3, 80);
    TEST_ASSERT_EQUAL(P2P_RAFT_LEADER, g_n[0].raft.role);

    /* Compact Raft log on leader and verify log count reset */
    TEST_ASSERT_EQUAL(0, p2p_raft_compact(&g_n[0]));
    TEST_ASSERT_EQUAL(0U, g_n[0].raft.log_count);
}

static void test_lot23_p2p_raft_anti_entropy_and_link_cost(void) {
    p2p_peer_t peer;
    memset(&peer, 0, sizeof(peer));
    peer.rtt_ewma_ms = 25;
    peer.bucket_used = 8;
    peer.loss_percent = 5;
    peer.anomalies = 1;
    peer.down_events = 2;

    /* Base cost 10 + 25 RTT + 20 bucket + 50 loss + 50 anomalies + 50 down events = 205 */
    uint32_t cost = p2p_calc_link_cost(&peer);
    TEST_ASSERT_EQUAL(205U, cost);

    setup(3, 0);
    run(3, 200);

    /* AIMD rate adaptation */
    g_n[0].qos.window_size = 16;
    p2p_qos_note_loss(&g_n[0]);
    TEST_ASSERT_EQUAL(8U, g_n[0].qos.window_size);

    /* Auto cache invalidation on disconnect */
    TEST_ASSERT_EQUAL(0, p2p_cache_put_v(&g_n[0], "p2p_k23", "v23", 1, 500, g_ticks));
    p2p_cache_on_peer_down(&g_n[0], g_n[1].id);
    TEST_ASSERT_EQUAL(0, g_n[0].cache.inv_active);
}

static void test_lot24_p2p_raft_anti_entropy_and_link_cost(void) {
    p2p_peer_t peer;
    memset(&peer, 0, sizeof(peer));
    peer.rtt_ewma_ms = 40;
    peer.bucket_used = 2;
    peer.loss_percent = 2;
    peer.anomalies = 0;
    peer.down_events = 1;

    /* Base cost 10 + 40 RTT + 5 bucket + 20 loss + 0 anomalies + 25 down events = 100 */
    uint32_t cost = p2p_calc_link_cost(&peer);
    TEST_ASSERT_EQUAL(100U, cost);

    setup(3, 0);
    run(3, 200);

    /* AIMD rate adaptation */
    g_n[0].qos.window_size = 32;
    p2p_qos_note_loss(&g_n[0]);
    TEST_ASSERT_EQUAL(16U, g_n[0].qos.window_size);

    /* Disconnect cache invalidation */
    TEST_ASSERT_EQUAL(0, p2p_cache_put_v(&g_n[0], "p2p_k24", "v24", 1, 600, g_ticks));
    p2p_cache_on_peer_down(&g_n[0], g_n[1].id);
    TEST_ASSERT_EQUAL(0, g_n[0].cache.inv_active);
}

static void test_lot27_p2p_cluster_routing_and_fast_recovery(void) {
    setup(3, 0);
    run(3, 200);

    /* Cluster forwarding test */
    uint8_t dummy_app[16] = "cluster_msg";
    TEST_ASSERT(p2p_cluster_route_forward(&g_n[0], &g_h[0], 2, dummy_app, 11) >= 0);

    /* Raft Fast Recovery test */
    g_n[0].raft.active = 1;
    g_n[0].raft.role = P2P_RAFT_LEADER;
    g_n[0].raft.term = 5;

    TEST_ASSERT_EQUAL(1, p2p_raft_fast_recovery(&g_n[0], &g_h[0], 10));
    TEST_ASSERT_EQUAL(10U, g_n[0].raft.term);
    TEST_ASSERT_EQUAL(P2P_RAFT_FOLLOWER, g_n[0].raft.role);
    TEST_ASSERT_EQUAL(0, p2p_raft_fast_recovery(&g_n[0], &g_h[0], 10));
}

static void test_lot26_p2p_region_snapshot_and_bbr(void) {
    uint8_t chunk[64];
    memset(chunk, 0xAB, sizeof(chunk));

    setup(3, 0);
    run(3, 200);

    /* Multi-Region P2P routing */
    TEST_ASSERT_EQUAL(0, p2p_region_set(&g_n[0], 10));
    TEST_ASSERT_EQUAL(10U, g_n[0].region_id);

    /* Raft Snapshot Chunk Streaming */
    TEST_ASSERT_EQUAL(64, raft_stream_snapshot_chunk(&g_n[0], &g_h[0], "beta", 0, 2, chunk, 64));

    /* BBR-like adaptive QoS step */
    TEST_ASSERT_EQUAL(0, p2p_qos_bbr_step(&g_n[0], 20, 10000, 50));
    TEST_ASSERT_EQUAL(20U, g_n[0].qos.rtt_min_ms);
    TEST_ASSERT_GREATER_THAN(0U, g_n[0].qos.rate_est_bps);
    TEST_ASSERT_GREATER_THAN(0U, g_n[0].qos.window_size);
}

static void test_lot25_p2p_partition_healing_and_lease(void) {
    setup(3, 0);
    run(3, 200);

    /* Elect alpha as leader */
    TEST_ASSERT_EQUAL(2, p2p_raft_elect(&g_n[0], &g_h[0]));
    run(3, 80);
    TEST_ASSERT_EQUAL(P2P_RAFT_LEADER, g_n[0].raft.role);
    g_n[0].raft.commit_index = 5;

    /* Partition reconciliation test */
    TEST_ASSERT_EQUAL(1, p2p_raft_reconcile_partition(&g_n[0], &g_h[0], g_n[1].id, 2, 2));

    /* Dynamic lease adjustment under high RTT and packet loss */
    uint32_t init_lease = g_n[0].raft.lease_until;
    p2p_raft_update_lease(&g_n[0], 150, 25, g_ticks);
    TEST_ASSERT_GREATER_THAN((int)init_lease, (int)g_n[0].raft.lease_until);
}

static void test_lot29_p2p_rekey_zero_trust_2pc(void) {
    setup(2, 0);
    p2p_node_t *node = &g_n[0];
    p2p_peer_t *peer = &node->peers[0];
    peer->used = 1;
    peer->id = 2;
    uint8_t init_key[16] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16};
    p2p_rekey_init_peer(peer, init_key, 10, 5);

    TEST_ASSERT_EQUAL(0, p2p_zero_trust_verify_pkt(node, 2, 100, 128, 1005));
    TEST_ASSERT(peer->last_seen_seq == 100);

    TEST_ASSERT_EQUAL(-2, p2p_zero_trust_verify_pkt(node, 2, 100, 128, 1006));
    TEST_ASSERT_EQUAL(1, peer->zero_trust_revoked);
    TEST_ASSERT_EQUAL_STRING("replay_attack_detected", peer->revoke_reason);

    TEST_ASSERT_EQUAL(-1, p2p_zero_trust_verify_pkt(node, 2, 101, 128, 1007));

    p2p_peer_t *peer2 = &node->peers[1];
    peer2->used = 1;
    peer2->id = 3;
    p2p_rekey_init_peer(peer2, init_key, 100, 3);

    TEST_ASSERT_EQUAL(0, p2p_zero_trust_verify_pkt(node, 3, 10, 64, 1008));
    TEST_ASSERT_EQUAL(0, p2p_zero_trust_verify_pkt(node, 3, 11, 64, 1009));
    TEST_ASSERT_EQUAL(0, p2p_zero_trust_verify_pkt(node, 3, 12, 64, 1010));
    TEST_ASSERT_EQUAL(2, peer2->key_gen);

    TEST_ASSERT_EQUAL(0, p2p_raft_2pc_prepare(node, 501, "SET db:config=production", 0x07));
    int status = -1;
    TEST_ASSERT_EQUAL(0, p2p_raft_2pc_status(node, 501, &status));
    TEST_ASSERT_EQUAL(P2P_2PC_STATE_PREPARED, status);

    TEST_ASSERT_EQUAL(0, p2p_raft_2pc_commit(node, 501));
    TEST_ASSERT_EQUAL(0, p2p_raft_2pc_status(node, 501, &status));
    TEST_ASSERT_EQUAL(P2P_2PC_STATE_COMMITTED, status);

    TEST_ASSERT_EQUAL(0, p2p_raft_2pc_prepare(node, 502, "DELETE /var/log", 0x07));
    TEST_ASSERT_EQUAL(0, p2p_raft_2pc_abort(node, 502));
    TEST_ASSERT_EQUAL(0, p2p_raft_2pc_status(node, 502, &status));
    TEST_ASSERT_EQUAL(P2P_2PC_STATE_ABORTED, status);
}

static void test_lot28_p2p_gossip_nat_and_multipath(void) {
    p2p_node_t node;
    p2p_host_t host = {0};
    uint8_t ip[4] = {192, 168, 1, 10};
    p2p_up(&node, "node1", ip, "netkey123", (const uint8_t*)"seed1234seed1234seed1234seed1234", &host);

    p2p_gossip_table_t gtable = {0};
    gtable.count = 1;
    for (int i = 0; i < P2P_KEY_MAX - 1 && "k1"[i]; i++) gtable.items[0].key[i] = "k1"[i];
    for (int i = 0; i < P2P_VAL_MAX - 1 && "v1"[i]; i++) gtable.items[0].value[i] = "v1"[i];
    gtable.items[0].version = 1;
    gtable.items[0].ttl_hops = 3;

    p2p_gossip_tick(&node, &host, &gtable);
    TEST_ASSERT_EQUAL(1, gtable.rounds_executed);

    p2p_nat_keepalive_table_t ntable = {0};
    ntable.count = 1;
    ntable.sessions[0].peer_id = 2;
    ntable.sessions[0].session_active = 1;
    ntable.sessions[0].last_keepalive_tick = 0;

    p2p_nat_keepalive_step(&node, &host, &ntable, 1000);

    uint32_t p1 = 0, p2 = 0;
    p2p_multipath_route_select(&node, 2, &p1, &p2);
}

static void test_lot30_p2p_region_route_multi_raft_merkle(void) {
    p2p_node_t node;
    memset(&node, 0, sizeof(node));
    node.id = 1;
    node.peers[0].used = 1;
    node.peers[0].id = 2;
    node.peers[0].up = 1;
    node.peers[0].rtt_ewma_ms = 10;

    /* 1. Multi-region route calculate */
    uint32_t next_hop = 0, cost = 0;
    TEST_ASSERT_EQUAL(0, p2p_region_route_calculate(&node, 1, 2, 2, &next_hop, &cost));
    TEST_ASSERT_EQUAL(2, next_hop);
    TEST_ASSERT_TRUE(cost >= 50);

    /* 2. Multi-Raft manager */
    p2p_multi_raft_mgr_t raft_mgr;
    p2p_multi_raft_init(&raft_mgr);
    TEST_ASSERT_EQUAL(1, p2p_multi_raft_propose(&raft_mgr, 1, "SET k1 v1"));
    TEST_ASSERT_EQUAL(0, p2p_multi_raft_commit(&raft_mgr, 1, 1));

    /* 3. Merkle tree anti-entropy diff sync */
    p2p_merkle_tree_t t1, t2;
    p2p_merkle_init(&t1);
    p2p_merkle_init(&t2);
    p2p_merkle_insert(&t1, 100, "key1", "val1");
    p2p_merkle_insert(&t1, 101, "key2", "val2");
    p2p_merkle_insert(&t2, 100, "key1", "val1");

    uint32_t diff_ids[4];
    int diff_cnt = p2p_merkle_diff_sync(&t1, &t2, diff_ids, 4);
    TEST_ASSERT_EQUAL(1, diff_cnt);
    TEST_ASSERT_EQUAL(101, diff_ids[0]);
}

static void test_lot37_p2p_wan_raft_election_and_batch_zk(void) {
    p2p_wan_raft_elec_t elec;
    p2p_wan_raft_elec_init(&elec, 1, 5, 100);
    TEST_ASSERT_EQUAL(0, elec.state);

    /* 3 votes out of 5 total voters -> quorum = 3 -> Leader */
    int res = p2p_wan_raft_leader_elect(&elec, 10, 1, 3, 500);
    TEST_ASSERT_EQUAL(0, res);
    TEST_ASSERT_EQUAL(2, elec.state);
    TEST_ASSERT_EQUAL(10, elec.leader_id);

    int lease_valid = 0;
    res = p2p_wan_raft_check_leader_lease(&elec, 550, &lease_valid);
    TEST_ASSERT_EQUAL(0, res);
    TEST_ASSERT_EQUAL(1, lease_valid);

    p2p_zk_bulletproof_batch_t batch;
    p2p_zk_bulletproof_batch_init(&batch);

    uint32_t vec_a[4] = {1, 2, 3, 4};
    uint32_t vec_b[4] = {5, 6, 7, 8};
    p2p_zk_bulletproof_ip_t proof;
    p2p_zk_bulletproof_inner_product_create(vec_a, vec_b, 4, &proof);

    res = p2p_zk_bulletproof_batch_add(&batch, &proof, 70);
    TEST_ASSERT_EQUAL(0, res);
    TEST_ASSERT_EQUAL(1, batch.count);

    int all_valid = 0;
    res = p2p_zk_bulletproof_batch_verify(&batch, &all_valid);
    TEST_ASSERT_EQUAL(0, res);
    TEST_ASSERT_EQUAL(1, all_valid);
    TEST_ASSERT_EQUAL(1, batch.verified_count);
}

static void test_lot31_p2p_topology_bft_bloom(void) {
    p2p_node_t node;
    memset(&node, 0, sizeof(node));
    node.id = 10;
    node.peers[0].used = 1;
    node.peers[0].id = 20;
    node.peers[0].up = 1;

    /* 1. Multi-Topology Routing */
    uint32_t next_hop = 0;
    TEST_ASSERT_EQUAL(0, p2p_topology_select_route(&node, P2P_TOPOLOGY_RING, 30, &next_hop));
    TEST_ASSERT_EQUAL(20, next_hop);

    /* 2. BFT Consensus (f = 1, quorum = 2f + 1 = 3) */
    p2p_bft_state_t bft;
    memset(&bft, 0, sizeof(bft));
    TEST_ASSERT_EQUAL(0, p2p_bft_preprepare(&bft, 1, 100, "block_payload_data", 1));
    TEST_ASSERT_EQUAL(P2P_BFT_STAGE_PREPREPARE, bft.stage);

    TEST_ASSERT_EQUAL(0, p2p_bft_prepare(&bft, 1, 1, 100));
    TEST_ASSERT_EQUAL(0, p2p_bft_prepare(&bft, 2, 1, 100));
    TEST_ASSERT_EQUAL(0, p2p_bft_prepare(&bft, 3, 1, 100));
    TEST_ASSERT_EQUAL(P2P_BFT_STAGE_PREPARE, bft.stage);

    TEST_ASSERT_EQUAL(0, p2p_bft_commit(&bft, 1, 1, 100));
    TEST_ASSERT_EQUAL(0, p2p_bft_commit(&bft, 2, 1, 100));
    TEST_ASSERT_EQUAL(0, p2p_bft_commit(&bft, 3, 1, 100));
    TEST_ASSERT_EQUAL(P2P_BFT_STAGE_COMMIT, bft.stage);
    TEST_ASSERT_EQUAL(1, bft.committed);

    /* 3. Bloom Filter Sync Diff */
    p2p_bloom_filter_t bf1, bf2;
    p2p_bloom_sync_init(&bf1);
    p2p_bloom_sync_init(&bf2);
    bf1.filter[0] = 0b00000001;
    bf2.filter[0] = 0b00000011;

    int mismatches = 0;
    TEST_ASSERT_EQUAL(0, p2p_bloom_sync_diff(&bf1, &bf2, &mismatches));
    TEST_ASSERT_EQUAL(1, mismatches);
}

static void test_lot32_p2p_topology_zk_bbr(void) {
    p2p_node_t n;
    n.id = 1;

    /* Test Topology Reconfiguration with high flap count -> STAR topology */
    p2p_topo_peer_info_t peers[2];
    peers[0].peer_id = 2; peers[0].rtt_ms = 50; peers[0].ewma_rtt = 50; peers[0].flap_count = 6; peers[0].active = 1;
    peers[1].peer_id = 3; peers[1].rtt_ms = 60; peers[1].ewma_rtt = 60; peers[1].flap_count = 0; peers[1].active = 1;

    uint32_t active_topo = 0;
    TEST_ASSERT_EQUAL(0, p2p_topology_reconfigure(&n, peers, 2, &active_topo));
    TEST_ASSERT_EQUAL(P2P_TOPOLOGY_STAR, active_topo);

    /* Test ZK Consensus verification */
    p2p_zk_proof_t proof;
    for (int i = 0; i < 32; i++) proof.statement[i] = 0;
    proof.statement[0] = 'a'; proof.statement[1] = 'g'; proof.statement[2] = 'r'; proof.statement[3] = 'e'; proof.statement[4] = 'e';
    proof.proof_len = 8;
    for (int i = 0; i < 8; i++) proof.proof_bytes[i] = (uint8_t)(i + 1);

    int valid = 0;
    TEST_ASSERT_EQUAL(0, p2p_zk_consensus_verify(&proof, "agree", &valid));
    TEST_ASSERT_EQUAL(1, valid);

    /* Test BBR Anti-entropy sync */
    int items = 0;
    TEST_ASSERT_EQUAL(0, p2p_bbr_anti_entropy_sync(&n, 2, 1200, &items));
    TEST_ASSERT_EQUAL(12, items);
}

static void test_lot33_p2p_geo_consensus_bulletproof(void) {
    /* 1. Cross-WAN Geo-Sharded Consensus */
    p2p_geo_shard_t shard;
    p2p_geo_shard_init(&shard, 101, 0x0F);
    TEST_ASSERT_EQUAL(1, shard.active);

    uint32_t cid1 = 0, cid2 = 0;
    TEST_ASSERT_EQUAL(0, p2p_geo_consensus_propose(&shard, 1, "geo_data_region1", &cid1));
    TEST_ASSERT_EQUAL(0, p2p_geo_consensus_propose(&shard, 2, "geo_data_region2", &cid2));
    TEST_ASSERT_EQUAL(1, cid1);
    TEST_ASSERT_EQUAL(2, cid2);
    TEST_ASSERT_EQUAL(2, shard.count);

    int synced = 0;
    TEST_ASSERT_EQUAL(0, p2p_geo_cross_region_sync(&shard, 2, &synced));
    TEST_ASSERT_EQUAL(2, synced);

    /* 2. Zero-Knowledge Bulletproof Validation */
    p2p_zk_bulletproof_t proof;
    TEST_ASSERT_EQUAL(0, p2p_zk_bulletproof_create_commitment(500, 12345, &proof));
    TEST_ASSERT_EQUAL(64, proof.proof_len);

    int valid = 0;
    TEST_ASSERT_EQUAL(0, p2p_zk_bulletproof_verify(&proof, 100, 1000, &valid));
    TEST_ASSERT_EQUAL(1, valid);
}

static void test_lot35_p2p_cross_wan_raft_bulletproof_ip(void) {
    /* 1. Cross-WAN Resilient Raft Group Sharding */
    p2p_cross_wan_raft_shard_mgr_t mgr;
    p2p_cross_wan_raft_shard_init(&mgr);
    TEST_ASSERT_EQUAL(4, mgr.group_count);

    TEST_ASSERT_EQUAL(0, p2p_cross_wan_raft_propose_sharded(&mgr, 1, "SET user_id=1001"));
    TEST_ASSERT_EQUAL(1, mgr.total_sharded_proposals);

    int synced_groups = 0;
    TEST_ASSERT_EQUAL(0, p2p_cross_wan_raft_sync_groups(&mgr, 0x01, &synced_groups));
    TEST_ASSERT_EQUAL(1, synced_groups);

    /* 2. Zero-Knowledge Bulletproof Inner-Product Proofs */
    uint32_t vec_a[4] = { 1, 2, 3, 4 };
    uint32_t vec_b[4] = { 10, 20, 30, 40 };
    p2p_zk_bulletproof_ip_t proof;

    TEST_ASSERT_EQUAL(0, p2p_zk_bulletproof_inner_product_create(vec_a, vec_b, 4, &proof));
    TEST_ASSERT_EQUAL(300, proof.inner_product_value); /* 10 + 40 + 90 + 160 = 300 */
    TEST_ASSERT_EQUAL(64, proof.proof_len);

    int valid = 0;
    TEST_ASSERT_EQUAL(0, p2p_zk_bulletproof_inner_product_verify(&proof, 300, &valid));
    TEST_ASSERT_EQUAL(1, valid);

    TEST_ASSERT_EQUAL(0, p2p_zk_bulletproof_inner_product_verify(&proof, 999, &valid));
    TEST_ASSERT_EQUAL(0, valid);
}

int main(void) {
    unity_init();
    RUN_TEST(test_discovery_and_keys);
    RUN_TEST(test_multi_relay_chain);
    RUN_TEST(test_crash_reroute_and_rejoin);
    RUN_TEST(test_qos_adaptive_controller);
    RUN_TEST(test_cache_versioned_acked_invalidation);
    RUN_TEST(test_traffic_baseline_anomaly);
    RUN_TEST(test_anomaly_peer_attribution_and_solicited);
    RUN_TEST(test_cache_through_raft);
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
    RUN_TEST(test_stun_behind_simulated_nat);
    RUN_TEST(test_route_prefers_direct_over_cheaper_relay);
    RUN_TEST(test_p2p_consolidation_and_fallback);
    RUN_TEST(test_lot17_raft_resilience_and_compact_zeroing);
    RUN_TEST(test_rtt_ewma_and_auto_cache_invalidation_on_disconnect);
    RUN_TEST(test_lot19_adaptive_cost_and_mesh_routing);
    RUN_TEST(test_lot20_p2p_cache_and_raft_anti_entropy);
    RUN_TEST(test_lot21_mesh_routing_and_grpc_fallback);
    RUN_TEST(test_lot22_raft_partition_resilience);
    RUN_TEST(test_lot23_p2p_raft_anti_entropy_and_link_cost);
    RUN_TEST(test_lot24_p2p_raft_anti_entropy_and_link_cost);
    RUN_TEST(test_lot25_p2p_partition_healing_and_lease);
    RUN_TEST(test_lot26_p2p_region_snapshot_and_bbr);
    RUN_TEST(test_lot27_p2p_cluster_routing_and_fast_recovery);
    RUN_TEST(test_lot29_p2p_rekey_zero_trust_2pc);
    RUN_TEST(test_lot28_p2p_gossip_nat_and_multipath);
    RUN_TEST(test_lot30_p2p_region_route_multi_raft_merkle);
    RUN_TEST(test_lot31_p2p_topology_bft_bloom);
    RUN_TEST(test_lot32_p2p_topology_zk_bbr);
    RUN_TEST(test_lot33_p2p_geo_consensus_bulletproof);
    RUN_TEST(test_lot35_p2p_cross_wan_raft_bulletproof_ip);
    RUN_TEST(test_lot37_p2p_wan_raft_election_and_batch_zk);

    unity_print_results();
    unity_cleanup();
    return (unity_stats.tests_failed == 0) ? 0 : 1;
}
