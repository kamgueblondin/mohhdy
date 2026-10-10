/* Phase 7 collaborative ledger (US-091..US-105) on an in-process bus. */
#include "../../framework/unity.h"
#include <string.h>
#include <stdio.h>
#include "../../../userspace/collab.h"

#define N 3
static collab_t g_c[N];
static collab_host_t g_h[N];
static int g_idx[N];
static int g_cut[N]; /* 1: node isolated */
static char g_out[N][8192];
static char rep[8192];
static const uint32_t k_id[N] = {101, 202, 303};
static const char* k_nm[N] = {"alpha", "beta", "gamma"};
typedef struct { int to; uint32_t from; uint8_t d[128]; int len; } msg_t;
static msg_t g_q[512]; static int g_qn;

static int idx_of(uint32_t id) { int i; for (i = 0; i < N; i++) if (k_id[i] == id) return i; return -1; }
static int h_send(void* ctx, uint32_t to, const uint8_t* d, int len) {
    int me = *(int*)ctx, j, sent = 0;
    if (g_cut[me]) return 0;
    for (j = 0; j < N; j++) {
        if (j == me || g_cut[j] || (to && k_id[j] != to) || g_qn >= 512) continue;
        g_q[g_qn].to = j; g_q[g_qn].from = k_id[me]; memcpy(g_q[g_qn].d, d, len); g_q[g_qn].len = len; g_qn++; sent++;
    }
    return sent;
}
static void h_out(void* ctx, const char* s) { int me = *(int*)ctx; strcat(g_out[me], s); strcat(g_out[me], "\n"); }
static const char* h_name(void* ctx, uint32_t id) { (void)ctx; return idx_of(id) >= 0 ? k_nm[idx_of(id)] : "?"; }
static uint32_t h_members(void* ctx) { (void)ctx; return N; }
static void deliver(void) {
    int i;
    for (i = 0; i < g_qn; i++) collab_receive(&g_c[g_q[i].to], &g_h[g_q[i].to], g_q[i].from, g_q[i].d, g_q[i].len);
    g_qn = 0;
}
static void setup(void) {
    int i;
    memset(g_out, 0, sizeof(g_out)); memset(g_cut, 0, sizeof(g_cut)); g_qn = 0;
    for (i = 0; i < N; i++) {
        g_idx[i] = i; collab_init(&g_c[i], k_id[i]);
        g_h[i].ctx = &g_idx[i]; g_h[i].send = h_send; g_h[i].out = h_out; g_h[i].name = h_name; g_h[i].members = h_members;
    }
    for (i = 0; i < N; i++) TEST_ASSERT_GREATER_THAN(0, collab_emit(&g_c[i], &g_h[i], CE_JOIN, 0, 0, 0, 0, k_nm[i]));
    deliver();
}
static const char* report(int i, const char* what, uint32_t arg) { collab_report(&g_c[i], &g_h[i], what, arg, rep, sizeof(rep)); return rep; }
static void same_digest(void) {
    char d0[128];
    const char* p;
    report(0, "audit", 0);
    p = strstr(rep, "digest "); TEST_ASSERT_NOT_NULL(p); strcpy(d0, p);
    report(1, "audit", 0); TEST_ASSERT_NOT_NULL(strstr(rep, d0));
    report(2, "audit", 0); TEST_ASSERT_NOT_NULL(strstr(rep, d0));
}

void test_points_transfer_overdraft_and_convergence(void) {
    setup();
    collab_emit(&g_c[0], &g_h[0], CE_TRANSFER, k_id[1], 0, 30, 0, "");
    collab_emit(&g_c[1], &g_h[1], CE_TRANSFER, k_id[2], 0, 500, 0, ""); /* overdraft */
    collab_emit(&g_c[2], &g_h[2], CE_JOIN, 0, 0, 0, 0, "again");       /* second grant */
    deliver();
    report(2, "balances", 0);
    TEST_ASSERT_NOT_NULL(strstr(rep, "collab acct alpha balance 70"));
    TEST_ASSERT_NOT_NULL(strstr(rep, "collab acct beta balance 130"));
    TEST_ASSERT_NOT_NULL(strstr(rep, "collab acct gamma balance 100"));
    TEST_ASSERT_NOT_NULL(strstr(rep, "collab balances ok members 3"));
    report(0, "audit", 0);
    TEST_ASSERT_NOT_NULL(strstr(rep, "collab rejected transfer beta seq 2 reason insufficient-points"));
    TEST_ASSERT_NOT_NULL(strstr(rep, "collab rejected join gamma seq 2 reason duplicate"));
    TEST_ASSERT_NOT_NULL(strstr(rep, "applied 4 rejected 2"));
    same_digest();
    TEST_ASSERT_NOT_NULL(strstr(g_out[1], "collab got transfer from alpha seq 2"));
}

void test_resources_marketplace_and_reputation(void) {
    setup();
    collab_emit(&g_c[0], &g_h[0], CE_OFFER, 0, 0, 10, 1, "gpu-slot");
    deliver();
    collab_emit(&g_c[1], &g_h[1], CE_RESERVE, k_id[0], 2, 0, 0, "");
    deliver();
    collab_emit(&g_c[2], &g_h[2], CE_RESERVE, k_id[0], 2, 0, 0, ""); /* capacity 1 */
    collab_emit(&g_c[2], &g_h[2], CE_RATE, k_id[0], 0, 5, 0, "");    /* no right */
    collab_emit(&g_c[1], &g_h[1], CE_RATE, k_id[0], 0, 4, 0, "");    /* right from booking */
    collab_emit(&g_c[1], &g_h[1], CE_RESERVE, k_id[0], 9, 0, 0, ""); /* unknown offer */
    deliver();
    report(0, "offers", 0);
    TEST_ASSERT_NOT_NULL(strstr(rep, "collab offer alpha#2 gpu-slot price 10 used 1/1"));
    report(0, "balances", 0);
    TEST_ASSERT_NOT_NULL(strstr(rep, "collab acct alpha balance 110 rep 4.0 ratings 1"));
    TEST_ASSERT_NOT_NULL(strstr(rep, "collab acct beta balance 90"));
    report(0, "audit", 0);
    TEST_ASSERT_NOT_NULL(strstr(rep, "collab rejected reserve gamma seq 2 reason capacity-full"));
    TEST_ASSERT_NOT_NULL(strstr(rep, "collab rejected rate gamma seq 3 reason no-rating-right"));
    TEST_ASSERT_NOT_NULL(strstr(rep, "reason unknown-ref"));
    same_digest();
}

void test_distributed_task_escrow_and_arbitration(void) {
    uint32_t r;
    setup();
    collab_emit(&g_c[0], &g_h[0], CE_TASK, 0, 0, 20, 0, "sum 100"); deliver();
    report(1, "balances", 0); TEST_ASSERT_NOT_NULL(strstr(rep, "collab acct alpha balance 80")); /* escrow */
    collab_emit(&g_c[0], &g_h[0], CE_CLAIM, k_id[0], 2, 0, 0, "");   /* own task */
    collab_emit(&g_c[2], &g_h[2], CE_CLAIM, k_id[0], 2, 0, 0, ""); deliver();
    collab_emit(&g_c[1], &g_h[1], CE_CLAIM, k_id[0], 2, 0, 0, ""); deliver(); /* already claimed */
    TEST_ASSERT_EQUAL(0, collab_task_eval("sum 100", &r)); TEST_ASSERT_EQUAL(5050, (int)r);
    collab_emit(&g_c[2], &g_h[2], CE_DONE, k_id[0], 2, 0, r, ""); deliver();
    collab_emit(&g_c[0], &g_h[0], CE_REJECT, k_id[2], 2, 0, 0, "");  /* refusing a right result */
    collab_emit(&g_c[0], &g_h[0], CE_ACCEPT, k_id[2], 2, 0, 0, ""); deliver();
    report(1, "tasks", 0);
    TEST_ASSERT_NOT_NULL(strstr(rep, "collab task alpha#2 'sum 100' reward 20 state paid worker gamma"));
    report(1, "balances", 0);
    TEST_ASSERT_NOT_NULL(strstr(rep, "collab acct gamma balance 120"));
    report(1, "audit", 0);
    TEST_ASSERT_NOT_NULL(strstr(rep, "collab rejected claim alpha seq 3 reason not-allowed"));
    TEST_ASSERT_NOT_NULL(strstr(rep, "collab rejected claim beta seq 2 reason bad-state"));
    TEST_ASSERT_NOT_NULL(strstr(rep, "collab rejected reject alpha seq 4 reason wrong-result"));
    /* wrong result: accept refused, reject refunds */
    collab_emit(&g_c[1], &g_h[1], CE_TASK, 0, 0, 15, 0, "primes 100"); deliver();
    collab_emit(&g_c[2], &g_h[2], CE_CLAIM, k_id[1], 3, 0, 0, ""); deliver();
    collab_emit(&g_c[2], &g_h[2], CE_DONE, k_id[1], 3, 0, 24, ""); deliver(); /* truth is 25 */
    collab_emit(&g_c[1], &g_h[1], CE_ACCEPT, k_id[2], 3, 0, 0, "");
    collab_emit(&g_c[1], &g_h[1], CE_REJECT, k_id[2], 3, 0, 0, ""); deliver();
    report(0, "tasks", 0);
    TEST_ASSERT_NOT_NULL(strstr(rep, "'primes 100' reward 15 state refused worker gamma"));
    report(0, "balances", 0);
    TEST_ASSERT_NOT_NULL(strstr(rep, "collab acct beta balance 100"));
    collab_emit(&g_c[2], &g_h[2], CE_TASK, 0, 0, 5, 0, "bogus task"); deliver();
    report(0, "audit", 0);
    TEST_ASSERT_NOT_NULL(strstr(rep, "collab rejected task gamma"));
    same_digest();
    /* rating right after a paid task */
    collab_emit(&g_c[0], &g_h[0], CE_RATE, k_id[2], 0, 5, 0, ""); deliver();
    report(2, "balances", 0);
    TEST_ASSERT_NOT_NULL(strstr(rep, "collab acct gamma balance 120 rep 5.0 ratings 1"));
}

void test_governance(void) {
    setup();
    collab_emit(&g_c[0], &g_h[0], CE_PROPOSE, 0, 0, 0, 0, "raise grant"); deliver();
    collab_emit(&g_c[1], &g_h[1], CE_VOTE, k_id[0], 2, 1, 0, "");
    collab_emit(&g_c[1], &g_h[1], CE_VOTE, k_id[0], 2, 0, 0, ""); /* duplicate */
    deliver();
    report(2, "votes", 0);
    TEST_ASSERT_NOT_NULL(strstr(rep, "'raise grant' yes 1 no 0 members 3 status open"));
    collab_emit(&g_c[2], &g_h[2], CE_VOTE, k_id[0], 2, 1, 0, ""); deliver();
    report(0, "votes", 0);
    TEST_ASSERT_NOT_NULL(strstr(rep, "yes 2 no 0 members 3 status passed"));
    collab_emit(&g_c[1], &g_h[1], CE_PROPOSE, 0, 0, 0, 0, "close shop"); deliver();
    collab_emit(&g_c[0], &g_h[0], CE_VOTE, k_id[1], 4, 0, 0, "");
    collab_emit(&g_c[2], &g_h[2], CE_VOTE, k_id[1], 4, 0, 0, ""); deliver();
    report(0, "votes", 0);
    TEST_ASSERT_NOT_NULL(strstr(rep, "'close shop' yes 0 no 2 members 3 status rejected"));
    report(0, "audit", 0);
    TEST_ASSERT_NOT_NULL(strstr(rep, "collab rejected vote beta seq 3 reason duplicate"));
    same_digest();
}

void test_privacy_forget_and_export(void) {
    int i, found = 0;
    setup();
    TEST_ASSERT_EQUAL(0, collab_profile_set(&g_c[1], "email", "b@example.org", 0));
    TEST_ASSERT_EQUAL(0, collab_profile_set(&g_c[1], "city", "paris", 1));
    TEST_ASSERT_EQUAL(-1, collab_profile_set(&g_c[1], "bad key", "x", 1));
    collab_emit(&g_c[1], &g_h[1], CE_PROFILE, 0, 0, 0, 0, "city=paris");
    collab_emit(&g_c[1], &g_h[1], CE_PROFILE, 0, 0, 0, 0, "city=lyon"); deliver();
    report(0, "profile", k_id[1]);
    TEST_ASSERT_NOT_NULL(strstr(rep, "collab profile beta shared city=lyon;"));
    TEST_ASSERT_NULL(strstr(rep, "paris"));
    for (i = 0; i < g_c[0].n; i++) if (strstr(g_c[0].e[i].text, "example.org")) found = 1;
    TEST_ASSERT_EQUAL(0, found); /* private field never left the node */
    report(1, "export", 0);
    TEST_ASSERT_NOT_NULL(strstr(rep, "collab export field email=b@example.org private"));
    TEST_ASSERT_NOT_NULL(strstr(rep, "collab export profile seq 2 'city=paris'"));
    TEST_ASSERT_GREATER_THAN(0, collab_forget(&g_c[1], &g_h[1])); deliver();
    report(0, "profile", k_id[1]);
    TEST_ASSERT_NOT_NULL(strstr(rep, "collab profile beta shared (none)"));
    for (i = 0; i < g_c[0].n; i++) if (strstr(g_c[0].e[i].text, "lyon") || strstr(g_c[0].e[i].text, "paris")) found = 1;
    TEST_ASSERT_EQUAL(0, found); /* ledger copies redacted */
    report(1, "export", 0);
    TEST_ASSERT_NULL(strstr(rep, "email="));
    same_digest();
}

void test_sync_after_partition_and_tickets(void) {
    setup();
    g_cut[2] = 1;
    collab_emit(&g_c[0], &g_h[0], CE_TRANSFER, k_id[1], 0, 7, 0, "");
    collab_emit(&g_c[0], &g_h[0], CE_TICKET, 0, 0, 0, 0, "how do i reserve?"); deliver();
    g_cut[2] = 0;
    report(2, "balances", 0); TEST_ASSERT_NOT_NULL(strstr(rep, "collab acct beta balance 100"));
    TEST_ASSERT_EQUAL(2, collab_sync(&g_c[2], &g_h[2])); deliver(); deliver();
    report(2, "balances", 0); TEST_ASSERT_NOT_NULL(strstr(rep, "collab acct beta balance 107"));
    TEST_ASSERT_NOT_NULL(strstr(g_out[0], "collab sync to gamma sent 2"));
    same_digest();
    collab_emit(&g_c[1], &g_h[1], CE_ANSWER, k_id[0], 3, 0, 0, "use collab-reserve"); deliver();
    collab_emit(&g_c[1], &g_h[1], CE_ANSWER, k_id[0], 99, 0, 0, "nothing"); deliver();
    report(2, "tickets", 0);
    TEST_ASSERT_NOT_NULL(strstr(rep, "collab ticket alpha#3 answered 'how do i reserve?'"));
    TEST_ASSERT_NOT_NULL(strstr(rep, "collab answer beta to alpha#3 'use collab-reserve'"));
    TEST_ASSERT_NOT_NULL(strstr(rep, "collab tickets ok open 0 answered 1"));
}

void test_codec_and_eval(void) {
    collab_entry_t e, d;
    uint8_t b[COLLAB_WIRE];
    uint32_t r;
    memset(&e, 0, sizeof(e));
    e.type = CE_OFFER; e.origin = 7; e.seq = 3; e.lamport = 9; e.amount = 12; e.aux = 2; strcpy(e.text, "disk");
    TEST_ASSERT_EQUAL(COLLAB_WIRE, collab_encode(&e, b));
    TEST_ASSERT_EQUAL(0, collab_decode(b, COLLAB_WIRE, &d));
    TEST_ASSERT_EQUAL(12, (int)d.amount); TEST_ASSERT_EQUAL_STRING("disk", d.text);
    TEST_ASSERT_EQUAL(-1, collab_decode(b, 10, &d));
    b[0] = 99; TEST_ASSERT_EQUAL(-1, collab_decode(b, COLLAB_WIRE, &d));
    TEST_ASSERT_EQUAL(0, collab_task_eval("primes 100", &r)); TEST_ASSERT_EQUAL(25, (int)r);
    TEST_ASSERT_EQUAL(0, collab_task_eval("fnv a", &r)); TEST_ASSERT_EQUAL((int)0xe40c292c, (int)r);
    TEST_ASSERT_EQUAL(-1, collab_task_eval("sum x", &r));
    TEST_ASSERT_EQUAL(-1, collab_task_eval("primes 999999", &r));
    setup();
    TEST_ASSERT_EQUAL(-1, collab_emit(&g_c[0], &g_h[0], 0, 0, 0, 0, 0, ""));
    while (g_c[0].n < COLLAB_ENTRIES) collab_emit(&g_c[0], &g_h[0], CE_TICKET, 0, 0, 0, 0, "x");
    TEST_ASSERT_EQUAL(-2, collab_emit(&g_c[0], &g_h[0], CE_TICKET, 0, 0, 0, 0, "x"));
    g_qn = 0;
}

int main(void) {
    unity_init();
    RUN_TEST(test_points_transfer_overdraft_and_convergence);
    RUN_TEST(test_resources_marketplace_and_reputation);
    RUN_TEST(test_distributed_task_escrow_and_arbitration);
    RUN_TEST(test_governance);
    RUN_TEST(test_privacy_forget_and_export);
    RUN_TEST(test_sync_after_partition_and_tickets);
    RUN_TEST(test_codec_and_eval);
    unity_print_results();
    unity_cleanup();
    return (unity_stats.tests_failed == 0) ? 0 : 1;
}
