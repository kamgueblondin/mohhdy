/* US-064 Raft-like consensus on an in-process bus with partitions. */
#include "../../framework/unity.h"
#include <string.h>
#include <stdio.h>
#include "../../../userspace/raft.h"

#define N 3
static raft_t g_r[N];
static raft_host_t g_h[N];
static int g_idx[N];
static int g_cut[N][N];
static char g_out[N][16384];
static uint32_t g_now;
static const uint32_t k_id[N] = {11, 22, 33};
static const char* k_nm[N] = {"alpha", "beta", "gamma"};
typedef struct { int to; uint32_t from; uint8_t d[256]; int len; } msg_t;
static msg_t g_q[2048]; static int g_qn;

static int idx_of(uint32_t id) { int i; for (i = 0; i < N; i++) if (k_id[i] == id) return i; return -1; }
static int h_send(void* c, uint32_t to, const uint8_t* d, int len) {
    int me = *(int*)c, j = idx_of(to);
    if (j < 0 || g_cut[me][j] || g_qn >= 2048 || len > 256) return 0;
    g_q[g_qn].to = j; g_q[g_qn].from = k_id[me]; memcpy(g_q[g_qn].d, d, len); g_q[g_qn].len = len; g_qn++;
    return 1;
}
static void h_out(void* c, const char* s) { int me = *(int*)c; if (strlen(g_out[me]) + strlen(s) + 2 < sizeof(g_out[me])) { strcat(g_out[me], s); strcat(g_out[me], "\n"); } }
static const char* h_name(void* c, uint32_t id) { (void)c; return idx_of(id) >= 0 ? k_nm[idx_of(id)] : "?"; }
static void step(uint32_t ticks) {
    uint32_t t; int i;
    for (t = 0; t < ticks; t += 10) {
        g_now += 10;
        for (i = 0; i < N; i++) raft_tick(&g_r[i], &g_h[i], g_now);
        while (g_qn) {
            static msg_t q[2048]; int n = g_qn, k;
            memcpy(q, g_q, sizeof(msg_t) * n); g_qn = 0;
            for (k = 0; k < n; k++) raft_receive(&g_r[q[k].to], &g_h[q[k].to], q[k].from, q[k].d, q[k].len, g_now);
        }
    }
}
static int leader(void) { int i, l = -1, c = 0; for (i = 0; i < N; i++) if (g_r[i].role == RAFT_LEADER) { l = i; c++; } return c == 1 ? l : -1; }
static void setup(void) {
    int i;
    memset(g_cut, 0, sizeof(g_cut)); memset(g_out, 0, sizeof(g_out)); g_qn = 0; g_now = 1000;
    for (i = 0; i < N; i++) {
        g_idx[i] = i; g_h[i].ctx = &g_idx[i]; g_h[i].send = h_send; g_h[i].out = h_out; g_h[i].name = h_name;
        TEST_ASSERT_EQUAL(0, raft_start(&g_r[i], k_id[i], k_id, N, g_now));
    }
}
static void isolate(int x, int on) { int j; for (j = 0; j < N; j++) if (j != x) { g_cut[x][j] = on; g_cut[j][x] = on; } }

void test_election_and_replication(void) {
    int l, i;
    setup();
    step(1000);
    l = leader();
    TEST_ASSERT_TRUE(l >= 0);
    TEST_ASSERT_EQUAL(-1, raft_propose(&g_r[(l + 1) % N], &g_h[0], "k", "v", g_now));
    TEST_ASSERT_EQUAL(1, raft_propose(&g_r[l], &g_h[l], "color", "blue", g_now));
    step(200);
    for (i = 0; i < N; i++) {
        TEST_ASSERT_EQUAL(1, (int)g_r[i].commit);
        TEST_ASSERT_EQUAL_STRING("blue", raft_get(&g_r[i], "color"));
        TEST_ASSERT_NOT_NULL(strstr(g_out[i], "raft applied 1 color=blue term "));
    }
}

/* The leader is cut off: the majority elects a new leader in a higher term
 * and commits; the old leader's write never commits; after healing it steps
 * down, its conflicting entry is replaced and the logs converge. */
void test_partition_new_leader_and_convergence(void) {
    int old, nl, i;
    uint32_t t0;
    setup();
    step(1000);
    old = leader(); TEST_ASSERT_TRUE(old >= 0);
    raft_propose(&g_r[old], &g_h[old], "a", "1", g_now);
    step(200);
    t0 = g_r[old].term;
    isolate(old, 1);
    TEST_ASSERT_EQUAL(2, raft_propose(&g_r[old], &g_h[old], "x", "lost", g_now));
    step(1500);
    nl = -1;
    for (i = 0; i < N; i++) if (i != old && g_r[i].role == RAFT_LEADER) nl = i;
    TEST_ASSERT_TRUE(nl >= 0);
    TEST_ASSERT_TRUE(g_r[nl].term > t0);
    TEST_ASSERT_EQUAL(RAFT_LEADER, g_r[old].role);             /* still thinks so, alone */
    TEST_ASSERT_EQUAL(1, (int)g_r[old].commit);                /* cannot commit without a majority */
    TEST_ASSERT_EQUAL(2, raft_propose(&g_r[nl], &g_h[nl], "y", "new", g_now));
    step(300);
    TEST_ASSERT_EQUAL(2, (int)g_r[nl].commit);
    isolate(old, 0);
    step(800);
    TEST_ASSERT_EQUAL(RAFT_FOLLOWER, g_r[old].role);
    TEST_ASSERT_NOT_NULL(strstr(g_out[old], "raft step down"));
    TEST_ASSERT_TRUE(g_r[old].truncated >= 1);
    for (i = 0; i < N; i++) {
        TEST_ASSERT_EQUAL(2, (int)g_r[i].commit);
        TEST_ASSERT_EQUAL_STRING("new", raft_get(&g_r[i], "y"));
        TEST_ASSERT_NULL(raft_get(&g_r[i], "x"));
        TEST_ASSERT_EQUAL(raft_digest(&g_r[nl]), raft_digest(&g_r[i]));
    }
}

/* A follower that missed several entries is caught up (next index walks back). */
void test_lagging_follower_catch_up(void) {
    int l, f, i;
    char k[4];
    setup();
    step(1000);
    l = leader(); f = (l + 1) % N;
    isolate(f, 1);
    for (i = 0; i < 6; i++) { k[0] = 'k'; k[1] = (char)('0' + i); k[2] = 0; raft_propose(&g_r[l], &g_h[l], k, "z", g_now); step(60); }
    TEST_ASSERT_EQUAL(6, (int)g_r[l].commit);
    TEST_ASSERT_EQUAL(0, (int)g_r[f].commit);
    isolate(f, 0);
    step(1000);
    TEST_ASSERT_EQUAL(6, (int)g_r[f].commit);
    TEST_ASSERT_EQUAL(raft_digest(&g_r[l]), raft_digest(&g_r[f]));
}

int main(void) {
    unity_init();
    RUN_TEST(test_election_and_replication);
    RUN_TEST(test_partition_new_leader_and_convergence);
    RUN_TEST(test_lagging_follower_catch_up);
    unity_print_results();
    unity_cleanup();
    return (unity_stats.tests_failed == 0) ? 0 : 1;
}
