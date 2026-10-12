/* Performance, endurance, load, and fallback test suite for Web Runtime, P2P mesh, and Production engine. */
#include "../framework/unity.h"
#include "../framework/test_kernel.h"
#include <string.h>
#include <stdio.h>

#include "../../userspace/osui_runtime.h"
#include "../../userspace/p2p.h"
#include "../../userspace/prod.h"

/* ----------------------------------------------------------- Web Runtime */
static char g_osui_out[OSUI_OUT_MAX];

static int run_osui(const char *line) {
    memset(g_osui_out, 0, sizeof(g_osui_out));
    return osui_dispatch_line(line, g_osui_out, (int)sizeof(g_osui_out));
}

void test_osui_web_runtime_perf_load_and_fallback(void) {
    int rc, i;
    osui_runtime_init();

    /* Load test: dispatch 50 commands and verify telemetry latency metrics */
    for (i = 0; i < 50; i++) {
        rc = run_osui("os-status");
        TEST_ASSERT_EQUAL(0, rc);
    }

    rc = run_osui("browser-telemetry status");
    TEST_ASSERT_EQUAL(0, rc);
    TEST_ASSERT_NOT_NULL(strstr(g_osui_out, "status=active"));
    TEST_ASSERT_NOT_NULL(strstr(g_osui_out, "commands="));

    rc = run_osui("browser-telemetry export");
    TEST_ASSERT_EQUAL(0, rc);
    TEST_ASSERT_NOT_NULL(strstr(g_osui_out, "export ok json="));

    /* Multi-tab, PWA, and PromptProgram orchestration under load */
    for (i = 0; i < 2; i++) {
        char cmd_buf[128];
        snprintf(cmd_buf, sizeof(cmd_buf), "browser-tab-new http://mohhdy.local/tab%d tab%d", i, i);
        TEST_ASSERT_EQUAL(0, run_osui(cmd_buf));
    }
    TEST_ASSERT_EQUAL(0, run_osui("browser-tabs"));
    TEST_ASSERT_NOT_NULL(strstr(g_osui_out, "count="));

    TEST_ASSERT_EQUAL(0, run_osui("browser-pm-run print \"hello_load_test\""));
    TEST_ASSERT_NOT_NULL(strstr(g_osui_out, "browser-pm-run ok"));

    TEST_ASSERT_EQUAL(0, run_osui("pwa-manifest LoadApp app://load/start app://load/ standalone"));
    TEST_ASSERT_EQUAL(0, run_osui("pwa-install LoadApp"));
    TEST_ASSERT_EQUAL(0, run_osui("pwa-run LoadApp"));

    /* Security and fallback tests */
    rc = run_osui("browser-fetch http://untrusted.org/data origin=evil");
    TEST_ASSERT_EQUAL(1, rc);
    TEST_ASSERT_NOT_NULL(strstr(g_osui_out, "origin_denied"));

    /* Session TTL cleanup test */
    TEST_ASSERT_EQUAL(0, run_osui("session-new load_sess"));
    TEST_ASSERT_EQUAL(0, run_osui("session-ttl 1"));
    TEST_ASSERT_EQUAL(0, run_osui("session-cleanup"));
}

/* ------------------------------------------------------------- P2P Mesh */
#define P2P_NODES 4
#define P2P_QLEN 128
typedef struct { uint8_t ip[4], mac[6]; uint8_t data[P2P_DATAGRAM_MAX]; int len; } p2p_dgram_t;
typedef struct { int idx; } p2p_ctx_t;

static p2p_dgram_t g_p2p_q[P2P_NODES][P2P_QLEN];
static int g_p2p_qh[P2P_NODES], g_p2p_qt[P2P_NODES];
static int g_p2p_cut[P2P_NODES][P2P_NODES];
static uint32_t g_p2p_ticks;
static char g_p2p_out[P2P_NODES][16384];
static p2p_node_t g_p2p_n[P2P_NODES];
static p2p_ctx_t g_p2p_ctx[P2P_NODES];
static p2p_host_t g_p2p_h[P2P_NODES];

static int p2p_bus_send(void* c, const uint8_t ip[4], const uint8_t* d, uint16_t len) {
    int from = ((p2p_ctx_t*)c)->idx, j;
    (void)ip;
    for (j = 0; j < P2P_NODES; j++) {
        p2p_dgram_t* q;
        if (j == from || g_p2p_cut[from][j] || (g_p2p_qt[j] + 1) % P2P_QLEN == g_p2p_qh[j]) continue;
        q = &g_p2p_q[j][g_p2p_qt[j]];
        memcpy(q->ip, g_p2p_n[from].ip, 4);
        memset(q->mac, 0, 6); q->mac[5] = (uint8_t)(from + 1);
        memcpy(q->data, d, len); q->len = len;
        g_p2p_qt[j] = (g_p2p_qt[j] + 1) % P2P_QLEN;
    }
    return 0;
}

static int p2p_bus_recv(void* c, uint8_t ip[4], uint8_t mac[6], uint8_t* d, uint16_t cap) {
    int me = ((p2p_ctx_t*)c)->idx;
    p2p_dgram_t* q;
    if (g_p2p_qh[me] == g_p2p_qt[me]) return 0;
    q = &g_p2p_q[me][g_p2p_qh[me]];
    g_p2p_qh[me] = (g_p2p_qh[me] + 1) % P2P_QLEN;
    if (q->len > cap) return 0;
    memcpy(ip, q->ip, 4); memcpy(mac, q->mac, 6); memcpy(d, q->data, q->len);
    return q->len;
}

static uint32_t p2p_bus_ticks(void* c) { (void)c; return g_p2p_ticks; }
static void p2p_bus_out(void* c, const char* s) {
    int me = ((p2p_ctx_t*)c)->idx;
    if (strlen(g_p2p_out[me]) + strlen(s) + 2 < sizeof(g_p2p_out[me])) {
        strcat(g_p2p_out[me], s); strcat(g_p2p_out[me], "\n");
    }
}

static void p2p_setup_mesh(int count) {
    int i;
    static const char* names[P2P_NODES] = {"alpha", "beta", "gamma", "delta"};
    memset(g_p2p_q, 0, sizeof(g_p2p_q)); memset(g_p2p_qh, 0, sizeof(g_p2p_qh)); memset(g_p2p_qt, 0, sizeof(g_p2p_qt));
    memset(g_p2p_cut, 0, sizeof(g_p2p_cut)); memset(g_p2p_out, 0, sizeof(g_p2p_out)); memset(g_p2p_n, 0, sizeof(g_p2p_n));
    g_p2p_ticks = 1000;
    for (i = 0; i < count; i++) {
        uint8_t ip[4] = {10, 77, 0, (uint8_t)(i + 1)}, seed[32];
        memset(seed, 0x50 + i, 32);
        g_p2p_ctx[i].idx = i;
        g_p2p_h[i].ctx = &g_p2p_ctx[i]; g_p2p_h[i].send = p2p_bus_send; g_p2p_h[i].recv = p2p_bus_recv;
        g_p2p_h[i].ticks = p2p_bus_ticks; g_p2p_h[i].out = p2p_bus_out;
        TEST_ASSERT_EQUAL(0, p2p_up(&g_p2p_n[i], names[i], ip, "mohhdy-perf-key", seed, &g_p2p_h[i]));
    }
}

static void p2p_run_mesh(int count, uint32_t ticks) {
    uint32_t t; int i;
    for (t = 0; t < ticks; t += 10) {
        g_p2p_ticks += 10;
        for (i = 0; i < count; i++) p2p_tick(&g_p2p_n[i], &g_p2p_h[i], 32);
    }
}

void test_p2p_mesh_endurance_multi_node_and_fallback(void) {
    char rep[1024];
    uint32_t nh = 0, cost = 0;

    p2p_setup_mesh(4);
    p2p_run_mesh(4, 300);

    /* Verify discovery across 4 nodes */
    TEST_ASSERT_NOT_NULL(p2p_find(&g_p2p_n[0], "beta"));
    TEST_ASSERT_NOT_NULL(p2p_find(&g_p2p_n[0], "gamma"));
    TEST_ASSERT_NOT_NULL(p2p_find(&g_p2p_n[0], "delta"));

    /* gRPC call load and fallbacks */
    TEST_ASSERT_EQUAL(0, p2p_grpc_call(&g_p2p_n[0], &g_p2p_h[0], "beta", "echo.EchoService", "SayHello", "MohhdyLoad"));
    p2p_run_mesh(4, 30);
    TEST_ASSERT_NOT_NULL(strstr(g_p2p_out[0], "p2p grpc resp status 0 payload Hello MohhdyLoad from beta"));

    /* Unimplemented gRPC service returns fallback status 3 */
    TEST_ASSERT_EQUAL(0, p2p_grpc_call(&g_p2p_n[0], &g_p2p_h[0], "beta", "unknown.Service", "UnknownMethod", "test"));
    p2p_run_mesh(4, 30);
    TEST_ASSERT_NOT_NULL(strstr(g_p2p_out[0], "p2p grpc resp status 3"));

    /* Raft consensus election under multi-node load */
    TEST_ASSERT_EQUAL(3, p2p_raft_elect(&g_p2p_n[0], &g_p2p_h[0]));
    p2p_run_mesh(4, 50);
    TEST_ASSERT_EQUAL(P2P_RAFT_LEADER, g_p2p_n[0].raft.role);

    /* Anti-entropy sync and LWW propagation */
    TEST_ASSERT_EQUAL(3, p2p_put(&g_p2p_n[0], &g_p2p_h[0], "cluster_state", "active"));
    p2p_run_mesh(4, 50);
    TEST_ASSERT_NOT_NULL(p2p_get_local(&g_p2p_n[3], "cluster_state"));

    /* STUN probe and NAT punch */
    TEST_ASSERT_EQUAL(0, p2p_stun_probe(&g_p2p_n[0], &g_p2p_h[0], "beta"));
    p2p_run_mesh(4, 30);
    TEST_ASSERT_NOT_NULL(strstr(g_p2p_out[0], "p2p stun mapped_ip 10.77.0.1"));

    TEST_ASSERT_EQUAL(0, p2p_nat_punch(&g_p2p_n[0], &g_p2p_h[0], "beta"));
    p2p_run_mesh(4, 30);
    TEST_ASSERT_NOT_NULL(strstr(g_p2p_out[1], "p2p nat punch from alpha pinhole opened"));

    /* Route failover on link cut */
    g_p2p_cut[0][2] = g_p2p_cut[2][0] = 1; /* Cut alpha <-> gamma */
    p2p_run_mesh(4, 1200);
    TEST_ASSERT_EQUAL(0, p2p_route_find(&g_p2p_n[0], g_p2p_n[2].id, &nh, &cost));
    TEST_ASSERT_EQUAL((int)g_p2p_n[1].id, (int)nh); /* Routed via beta */

    p2p_report(&g_p2p_n[0], &g_p2p_h[0], "analyze", rep, sizeof(rep));
    TEST_ASSERT_NOT_NULL(strstr(rep, "p2p analyze ok findings"));
}

/* ----------------------------------------------------- Production Engine */
static prod_sample_t prod_S(uint32_t t, uint32_t used, uint32_t busy) {
    prod_sample_t s;
    memset(&s, 0, sizeof(s));
    s.tick = t;
    s.v[PM_MEM_USED] = used;
    s.v[PM_MEM_FREE] = 1000 - used;
    s.v[PM_BUSY] = busy;
    s.v[PM_PROCS] = 4;
    return s;
}

void test_prod_engine_load_monitoring_and_fallback(void) {
    static prod_metrics_t m;
    static prod_alerts_t a;
    prod_alert_learn_t al;
    prod_perf_monitor_t pm;
    prod_scaler_t sc;
    prod_autoscale_workload_t wl;
    prod_pipeline_t pipe;
    prod_sec_scan_t scan;
    char out_buf[512];
    int i;

    prod_metrics_init(&m);
    prod_alerts_init(&a);

    /* Push high-rate samples into ring buffer */
    for (i = 0; i < 100; i++) {
        prod_sample_t s = prod_S((uint32_t)i * 50U, 300U + (uint32_t)(i % 20), 40U + (uint32_t)(i % 30));
        prod_metrics_push(&m, &s);
    }

    /* Smart alert threshold learning under load */
    TEST_ASSERT_EQUAL(0, prod_alert_learn_update(&al, &m, PM_BUSY));
    TEST_ASSERT_GREATER_THAN(0, (int)al.sample_count);
    TEST_ASSERT_EQUAL(0, prod_alert_learn_threshold(&al, 15));

    /* Performance monitor evaluation */
    TEST_ASSERT_GREATER_THAN(0, prod_perf_monitor_eval(&m, &a, &pm, out_buf, sizeof(out_buf)));
    TEST_ASSERT_EQUAL(100, (int)pm.overall_health_score);
    TEST_ASSERT_NOT_NULL(strstr(out_buf, "perf-monitor health 100"));

    /* Autoscaling workload manager */
    prod_scaler_init(&sc, 1, 4, 4000U, 1000U, 2U);
    memset(&wl, 0, sizeof(wl));
    wl.queue_depth = 25;
    TEST_ASSERT_EQUAL(0, prod_autoscale_workload_eval(&sc, &wl, 100, out_buf, sizeof(out_buf)));
    TEST_ASSERT_GREATER_THAN(0, (int)wl.active_workers);

    /* CI/CD pipeline simulation */
    TEST_ASSERT_EQUAL(0, prod_pipeline_run(&pipe, "/mohhdy", out_buf, sizeof(out_buf)));
    TEST_ASSERT_EQUAL(6, pipe.stage);

    /* Security scanner */
    TEST_ASSERT_EQUAL(0, prod_sec_scan_run(&scan, out_buf, sizeof(out_buf)));
    TEST_ASSERT_EQUAL(0, (int)scan.vulnerabilities_found);

    /* Corrupt backup refusal and fallback test */
    {
        static prod_archive_t ar;
        int bad = 0;
        prod_archive_init(&ar, "test_bup", 1);
        TEST_ASSERT_EQUAL(0, prod_archive_add(&ar, "/etc/test", "content", 7));
        TEST_ASSERT_EQUAL(0, prod_archive_verify(&ar, &bad));
        ar.data[2] ^= 0xFF; /* corrupt data */
        TEST_ASSERT_EQUAL(1, prod_archive_verify(&ar, &bad));
    }
}

int main(void) {
    unity_init();
    RUN_TEST(test_osui_web_runtime_perf_load_and_fallback);
    RUN_TEST(test_p2p_mesh_endurance_multi_node_and_fallback);
    RUN_TEST(test_prod_engine_load_monitoring_and_fallback);
    unity_print_results();
    unity_cleanup();
    return (unity_stats.tests_failed == 0) ? 0 : 1;
}
