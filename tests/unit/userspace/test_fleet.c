/* Fleet services over the P2P app channel (userspace/fleet.c). */
#include "../../framework/unity.h"
#include <string.h>
#include "../../../userspace/fleet.h"

#define N 3
typedef struct { int to; uint32_t from; uint8_t d[400]; int len; } msg_t;
static msg_t g_q[512]; static int g_nq;
static fleet_t g_f[N]; static fleet_host_t g_h[N];
static char g_out[N][4096];
static char g_fs[N][4][2][2048]; /* per node: path, content */
static int g_drop;
static uint32_t g_now, g_lossy, g_seed = 7;
/* g_drop: lose everything; g_lossy: lose every frame whose pseudo-random
 * draw falls under g_lossy percent */
static int dropped(void) { if (g_drop) return 1; g_seed = g_seed * 1103515245U + 12345U; return (int)((g_seed >> 16) % 100U) < (int)g_lossy; }
static uint32_t h_now(void* c) { (void)c; return g_now; }
static int h_peers(void* c, uint32_t* ids, int max) { int me = (int)(long)c, i, n = 0; for (i = 0; i < N && n < max; i++) if (i != me) ids[n++] = (uint32_t)i + 1; return n; }

static int idx(uint32_t id) { return (int)id - 1; }
static int h_send(void* c, uint32_t to, const uint8_t* d, int len) {
    int me = (int)(long)c, i, sent = 0;
    for (i = 0; i < N; i++) {
        if (i == me || (to && idx(to) != i)) continue;
        if (g_nq < 512 && !dropped()) { g_q[g_nq].to = i; g_q[g_nq].from = (uint32_t)me + 1; memcpy(g_q[g_nq].d, d, (size_t)len); g_q[g_nq].len = len; g_nq++; }
        sent++;
    }
    return sent;
}
static void h_out(void* c, const char* s) { int me = (int)(long)c; strcat(g_out[me], s); strcat(g_out[me], "\n"); }
static int h_write(void* c, const char* p, const char* d, int len) {
    int me = (int)(long)c, i;
    for (i = 0; i < 4; i++) if (!g_fs[me][i][0][0] || !strcmp(g_fs[me][i][0], p)) {
        strcpy(g_fs[me][i][0], p); memcpy(g_fs[me][i][1], d, (size_t)len); g_fs[me][i][1][len] = 0; return len;
    }
    return -1;
}
static uint32_t h_members(void* c) { (void)c; return 3; }
static const char* h_name(void* c, uint32_t id) { static const char* n[] = {"alpha", "beta", "gamma"}; (void)c; return id >= 1 && id <= 3 ? n[id - 1] : "?"; }
static const char* file_of(int node, const char* p) { int i; for (i = 0; i < 4; i++) if (!strcmp(g_fs[node][i][0], p)) return g_fs[node][i][1]; return 0; }
static void pump(void) {
    int i = 0;
    while (i < g_nq) { msg_t m = g_q[i++]; fleet_receive(&g_f[m.to], &g_h[m.to], m.from, m.d, m.len); }
    g_nq = 0;
}
static void setup(void) {
    int i;
    memset(g_out, 0, sizeof(g_out)); memset(g_fs, 0, sizeof(g_fs)); g_nq = 0; g_drop = 0; g_lossy = 0; g_now = 0;
    for (i = 0; i < N; i++) {
        fleet_init(&g_f[i]);
        g_h[i].ctx = (void*)(long)i; g_h[i].self = (uint32_t)i + 1; g_h[i].send = h_send;
        g_h[i].out = h_out; g_h[i].write_file = h_write; g_h[i].name = h_name; g_h[i].members = h_members; g_h[i].now = h_now; g_h[i].peers = h_peers;
    }
}

static void test_session_handoff_and_sync(void) {
    setup();
    TEST_ASSERT_EQUAL(0, fleet_session_send(&g_f[0], &g_h[0], 2, "cwd=/home\x1eUSER=kb", 17));
    pump();
    TEST_ASSERT_NOT_NULL(strstr(g_out[1], "fleet session offered by alpha bytes 17"));
    TEST_ASSERT_EQUAL(1, (int)g_f[1].sess_from);
    TEST_ASSERT_EQUAL(0, memcmp(g_f[1].sess, "cwd=/home\x1eUSER=kb", 17));
    TEST_ASSERT_EQUAL(0, (int)g_f[2].sess_len);   /* only the chosen peer */
    TEST_ASSERT_EQUAL(1, fleet_sync_push(&g_f[0], &g_h[0], "/notes.txt", "v1 text", 7));
    pump();
    TEST_ASSERT_EQUAL_STRING("v1 text", file_of(1, "/notes.txt"));
    TEST_ASSERT_EQUAL_STRING("v1 text", file_of(2, "/notes.txt"));
    /* beta edits on top of v1: v2 wins everywhere */
    TEST_ASSERT_EQUAL(2, fleet_sync_push(&g_f[1], &g_h[1], "/notes.txt", "v2 beta", 7));
    pump();
    TEST_ASSERT_EQUAL_STRING("v2 beta", file_of(0, "/notes.txt"));
    TEST_ASSERT_NOT_NULL(strstr(g_out[2], "fleet sync applied /notes.txt v2 bytes 7 from beta"));
    /* a stale v1 replayed later is kept out */
    {
        uint8_t b[64]; uint32_t s = fleet_sum("old", 3); int k = 0;
        b[k++] = FL_SYNC; b[k++] = 1; b[k++] = 0; b[k++] = 0; b[k++] = 0; b[k++] = 1; b[k++] = 0; b[k++] = 0; b[k++] = 0;
        b[k++] = (uint8_t)s; b[k++] = (uint8_t)(s >> 8); b[k++] = (uint8_t)(s >> 16); b[k++] = (uint8_t)(s >> 24);
        memcpy(b + k, "/notes.txt", 11); k += 11; memcpy(b + k, "old", 3); k += 3;
        fleet_receive(&g_f[2], &g_h[2], 1, b, k);
        TEST_ASSERT_EQUAL_STRING("v2 beta", file_of(2, "/notes.txt"));
        TEST_ASSERT_NOT_NULL(strstr(g_out[2], "fleet sync kept /notes.txt v2 (older v1 from alpha)"));
        b[k - 1] = 'X';   /* content no longer matches its checksum */
        b[1] = 9;
        fleet_receive(&g_f[2], &g_h[2], 1, b, k);
        TEST_ASSERT_NOT_NULL(strstr(g_out[2], "fleet sync rejected (checksum)"));
    }
}

static void test_metrics_and_logs(void) {
    int i;
    setup();
    TEST_ASSERT_EQUAL(0, fleet_report(&g_f[1], &g_h[1], 1, "mem_used=10 procs=4 firing=0", "boot ok"));
    for (i = 0; i < 5; i++) fleet_report(&g_f[2], &g_h[2], 1, "mem_used=20 procs=6 firing=1", i == 4 ? "ERROR disk full" : "warn x");
    pump();
    TEST_ASSERT_NOT_NULL(strstr(g_out[0], "fleet metric from beta: mem_used=10 procs=4 firing=0"));
    TEST_ASSERT_NOT_NULL(strstr(g_out[0], "fleet log from gamma: ERROR disk full"));
    TEST_ASSERT_EQUAL(5, (int)g_f[0].nodes[1].reports);
    TEST_ASSERT_EQUAL(FL_LOGS, g_f[0].nodes[1].nlog);
    TEST_ASSERT_EQUAL_STRING("ERROR disk full", g_f[0].nodes[1].log[FL_LOGS - 1]);
    TEST_ASSERT_EQUAL(0, (int)g_f[1].nodes[0].used); /* reports went to alpha only */
}

static void test_staged_deploy(void) {
    fl_deploy_t* dp;
    setup();
    TEST_ASSERT_EQUAL(1, fleet_deploy_stage(&g_f[0], &g_h[0], "web", "release 1", 9, 2));
    TEST_ASSERT_EQUAL(-2, fleet_deploy_promote(&g_f[0], &g_h[0], "web")); /* no ack yet */
    pump(); pump();
    TEST_ASSERT_EQUAL_STRING("release 1", file_of(1, "/app/web"));
    TEST_ASSERT_NULL(file_of(2, "/app/web"));                                /* canary only */
    TEST_ASSERT_NOT_NULL(strstr(g_out[0], "fleet deploy ack web v1 ok from beta"));
    TEST_ASSERT_EQUAL(0, fleet_deploy_promote(&g_f[0], &g_h[0], "web"));
    pump(); pump();
    TEST_ASSERT_EQUAL_STRING("release 1", file_of(2, "/app/web"));
    TEST_ASSERT_NOT_NULL(strstr(g_out[2], "fleet deploy web v1 all applied from alpha"));
    TEST_ASSERT_NOT_NULL(strstr(g_out[1], "fleet deploy web v1 all applied from alpha")); /* repeat is fine */
    /* v2 staged, then rolled back to release 1 as v3 */
    TEST_ASSERT_EQUAL(2, fleet_deploy_stage(&g_f[0], &g_h[0], "web", "release 2", 9, 3));
    pump(); pump();
    TEST_ASSERT_EQUAL_STRING("release 2", file_of(2, "/app/web"));
    TEST_ASSERT_EQUAL_STRING("release 1", file_of(1, "/app/web"));
    TEST_ASSERT_EQUAL(3, fleet_deploy_rollback(&g_f[0], &g_h[0], "web"));
    pump(); pump();
    TEST_ASSERT_EQUAL_STRING("release 1", file_of(2, "/app/web"));
    TEST_ASSERT_NOT_NULL(strstr(g_out[2], "fleet deploy web v3 rollback applied"));
    dp = fleet_deploy_find(&g_f[0], "web");
    TEST_ASSERT_EQUAL(3, (int)dp->ver);
    /* a corrupt canary is refused and cannot be promoted */
    TEST_ASSERT_EQUAL(4, fleet_deploy_stage(&g_f[0], &g_h[0], "web", "release 4", 9, 2));
    g_q[0].d[g_q[0].len - 1] ^= 1;
    pump(); pump();
    TEST_ASSERT_NOT_NULL(strstr(g_out[1], "fleet deploy web v4 canary refused"));
    TEST_ASSERT_NOT_NULL(strstr(g_out[0], "fleet deploy ack web v4 failed from beta"));
    TEST_ASSERT_EQUAL(-2, fleet_deploy_promote(&g_f[0], &g_h[0], "web"));
    TEST_ASSERT_EQUAL(-1, fleet_deploy_promote(&g_f[0], &g_h[0], "nope"));
}

/* run every node's tick for a while, delivering what gets through */
static void run_ticks(int steps) {
    int k, i;
    for (k = 0; k < steps; k++) {
        g_now += 50;
        for (i = 0; i < N; i++) fleet_tick(&g_f[i], &g_h[i], g_now);
        pump();
    }
}

/* lost frames are re-sent until acknowledged (bounded, then reported) */
static void test_reliable_resend(void) {
    setup();
    g_drop = 1;
    TEST_ASSERT_EQUAL(0, fleet_session_send(&g_f[0], &g_h[0], 2, "cwd=/x", 6));
    g_drop = 0;
    TEST_ASSERT_EQUAL(1, fleet_pending(&g_f[0]));
    run_ticks(4);
    TEST_ASSERT_NOT_NULL(strstr(g_out[1], "fleet session offered by alpha bytes 6"));
    TEST_ASSERT_EQUAL(0, fleet_pending(&g_f[0]));
    TEST_ASSERT_TRUE(g_f[0].resent >= 1);
    /* lost acks: delivered once only, the duplicate is acked again */
    g_lossy = 0;
    fleet_report(&g_f[1], &g_h[1], 1, "m=1", "");
    {   /* deliver the data frame, lose the ack */
        msg_t m = g_q[0]; g_nq = 0;
        fleet_receive(&g_f[m.to], &g_h[m.to], m.from, m.d, m.len);
        g_nq = 0;
    }
    run_ticks(4);
    TEST_ASSERT_EQUAL(1, (int)g_f[0].nodes[0].reports);
    TEST_ASSERT_TRUE(g_f[0].dup >= 1);
    TEST_ASSERT_EQUAL(0, fleet_pending(&g_f[1]));
    /* unreachable peer: bounded, then reported as failed */
    g_drop = 1;
    fleet_report(&g_f[2], &g_h[2], 1, "m=2", "");
    run_ticks(40);
    TEST_ASSERT_EQUAL(0, fleet_pending(&g_f[2]));
    TEST_ASSERT_EQUAL(1, (int)g_f[2].failed);
    TEST_ASSERT_NOT_NULL(strstr(g_out[2], "fleet send to alpha failed after 6 re-sends"));
}

/* payloads larger than one datagram are chunked; 30% frame loss */
static void test_chunked_large_and_lossy(void) {
    static char big[1800];
    int i;
    setup();
    for (i = 0; i < (int)sizeof(big); i++) big[i] = (char)('a' + i % 26);
    big[sizeof(big) - 1] = 0;
    g_lossy = 30;
    TEST_ASSERT_EQUAL(1, fleet_sync_push(&g_f[0], &g_h[0], "/big.txt", big, (int)sizeof(big) - 1));
    run_ticks(30);
    TEST_ASSERT_EQUAL_STRING(big, file_of(1, "/big.txt"));
    TEST_ASSERT_EQUAL_STRING(big, file_of(2, "/big.txt"));
    TEST_ASSERT_EQUAL(1, fleet_deploy_stage(&g_f[0], &g_h[0], "big", big, 1500, 2));
    run_ticks(30);
    TEST_ASSERT_EQUAL(0, strncmp(big, file_of(1, "/app/big"), 1500));
    TEST_ASSERT_EQUAL(0, fleet_deploy_promote(&g_f[0], &g_h[0], "big"));
    run_ticks(30);
    TEST_ASSERT_EQUAL(1500, (int)strlen(file_of(2, "/app/big")));
    TEST_ASSERT_EQUAL(-1, fleet_sync_push(&g_f[0], &g_h[0], "/x", big, FL_DATA_MAX + 1));
    g_lossy = 0;
    /* malformed chunk headers are rejected */
    {
        uint8_t bad[12] = {FL_FRAG, 1, 0, 0, 0, 5, 3, 'x'};
        uint32_t r = g_f[1].rejected;
        fleet_receive(&g_f[1], &g_h[1], 1, bad, 8);
        TEST_ASSERT_EQUAL(r + 1, g_f[1].rejected);
    }
}

int main(void) {
    unity_init();
    RUN_TEST(test_session_handoff_and_sync);
    RUN_TEST(test_metrics_and_logs);
    RUN_TEST(test_staged_deploy);
    RUN_TEST(test_reliable_resend);
    RUN_TEST(test_chunked_large_and_lossy);
    unity_print_results();
    unity_cleanup();
    return unity_stats.tests_failed == 0 ? 0 : 1;
}
