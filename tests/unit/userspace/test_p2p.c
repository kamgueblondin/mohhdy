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
static char g_out[NODES][16384];
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

/* Key agreement is spread over several ticks (no single long call) and a
 * peer is announced up only once its link key exists. */
void test_key_agreement_is_stepwise(void) {
    int ticks = 0;
    setup(2, 0);
    run(2, 10);
    TEST_ASSERT_NOT_NULL(p2p_find(&g_n[0], "beta"));
    TEST_ASSERT_FALSE(p2p_find(&g_n[0], "beta")->keyed);
    TEST_ASSERT_FALSE(p2p_find(&g_n[0], "beta")->up);
    TEST_ASSERT_NULL(strstr(g_out[0], "p2p peer beta up"));
    while (!p2p_find(&g_n[0], "beta")->keyed && ticks < 100) { run(2, 10); ticks++; }
    TEST_ASSERT_TRUE(ticks >= (X25519_JOB_STEPS / P2P_KX_BUDGET) - 1);
    TEST_ASSERT_TRUE(p2p_find(&g_n[0], "beta")->keyed);
    run(2, 30);
    TEST_ASSERT_NOT_NULL(strstr(g_out[0], "p2p peer beta up id "));
    TEST_ASSERT_EQUAL(0, memcmp(p2p_find(&g_n[0], "beta")->key, p2p_find(&g_n[1], "alpha")->key, 16));
    TEST_ASSERT_EQUAL(1, (int)g_n[0].kx_done);
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

/* US-063 multi-hop: line topology alpha-beta-gamma-delta from boot.
 * Hellos are relayed across partial links, so the ends discover and key
 * each other, and sealed frames follow the shortest path hop by hop. */
void test_multihop_line_topology(void) {
    int hops = 0, i, j;
    uint32_t nx;
    static const int cut[3][2] = {{0, 2}, {0, 3}, {1, 3}};
    setup(4, 0);
    for (i = 0; i < 3; i++) { g_cut[cut[i][0]][cut[i][1]] = 1; g_cut[cut[i][1]][cut[i][0]] = 1; }
    memset(g_qh, 0, sizeof(g_qh)); memset(g_qt, 0, sizeof(g_qt)); /* links cut from boot */
    run(4, 3000);
    TEST_ASSERT_NOT_NULL(p2p_find(&g_n[0], "delta"));
    TEST_ASSERT_TRUE(p2p_find(&g_n[0], "delta")->keyed);
    TEST_ASSERT_FALSE(p2p_find(&g_n[0], "delta")->up);
    nx = p2p_route(&g_n[0], p2p_find(&g_n[0], "delta")->id, &hops);
    TEST_ASSERT_EQUAL(p2p_find(&g_n[0], "beta")->id, nx);
    TEST_ASSERT_EQUAL(3, hops);
    nx = p2p_route(&g_n[0], p2p_find(&g_n[0], "gamma")->id, &hops);
    TEST_ASSERT_EQUAL(2, hops);
    TEST_ASSERT_NOT_NULL(strstr(g_out[0], "p2p peer delta reachable hops 3 via beta"));
    TEST_ASSERT_NOT_NULL(strstr(g_out[3], "p2p peer alpha reachable hops 3 via gamma"));
    TEST_ASSERT_TRUE(g_n[1].hello_relayed > 0 && g_n[2].hello_relayed > 0);
    TEST_ASSERT_EQUAL(0, p2p_send_text(&g_n[0], &g_h[0], "delta", "three hops"));
    run(4, 30);
    TEST_ASSERT_NOT_NULL(strstr(g_out[3], "p2p msg from alpha: three hops (relayed)"));
    TEST_ASSERT_TRUE(g_n[1].relayed >= 1 && g_n[2].relayed >= 1);
    TEST_ASSERT_EQUAL(0, p2p_send_text(&g_n[3], &g_h[3], "alpha", "back"));
    run(4, 30);
    TEST_ASSERT_NOT_NULL(strstr(g_out[0], "p2p msg from delta: back (relayed)"));
    /* no relay of hellos on a full mesh */
    for (i = 0; i < 4; i++) for (j = 0; j < 4; j++) g_cut[i][j] = 0;
    run(4, 600);
    nx = p2p_route(&g_n[0], p2p_find(&g_n[0], "delta")->id, &hops);
    TEST_ASSERT_EQUAL(0, nx); TEST_ASSERT_EQUAL(1, hops);
    {
        uint32_t before = g_n[1].hello_relayed + g_n[2].hello_relayed;
        run(4, 1000);
        TEST_ASSERT_EQUAL(before, g_n[1].hello_relayed + g_n[2].hello_relayed);
    }
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

int main(void) {
    unity_init();
    RUN_TEST(test_discovery_and_keys);
    RUN_TEST(test_key_agreement_is_stepwise);
    RUN_TEST(test_wrong_network_key_is_kept_out);
    RUN_TEST(test_encrypted_message_tamper_and_replay);
    RUN_TEST(test_replication_lww_get_and_sync);
    RUN_TEST(test_consensus_commit_reject_timeout);
    RUN_TEST(test_failure_detection_and_relay);
    RUN_TEST(test_multihop_line_topology);
    RUN_TEST(test_rate_limit_and_stats);
    RUN_TEST(test_traffic_analysis);
    RUN_TEST(test_bad_inputs);
    unity_print_results();
    unity_cleanup();
    return (unity_stats.tests_failed == 0) ? 0 : 1;
}
