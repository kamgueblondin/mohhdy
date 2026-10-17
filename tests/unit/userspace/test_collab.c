/* Phase 7 collaborative ledger (US-091..US-105) on an in-process bus. */
#include "../../framework/unity.h"
#include <string.h>
#include <stdio.h>
#include "../../../userspace/collab.h"
#include "../../../userspace/csig.h"
#include "../../../kernel/sha256.h"
#include "../../../kernel/aes_gcm.h"

#define N 3
static collab_t g_c[N];
static collab_host_t g_h[N];
static int g_idx[N];
static int g_cut[N]; /* 1: node isolated */
static char g_out[N][8192];
static char rep[8192];
static const uint32_t k_id[N] = {101, 202, 303};
static const char* k_nm[N] = {"alpha", "beta", "gamma"};
typedef struct { int to; uint32_t from; uint8_t d[320]; int len; } msg_t;
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
static int g_sign;
static void setup(void) {
    int i;
    memset(g_out, 0, sizeof(g_out)); memset(g_cut, 0, sizeof(g_cut)); g_qn = 0;
    for (i = 0; i < N; i++) {
        g_idx[i] = i; collab_init(&g_c[i], k_id[i]);
        g_h[i].ctx = &g_idx[i]; g_h[i].send = h_send; g_h[i].out = h_out; g_h[i].name = h_name; g_h[i].members = h_members;
        if (g_sign) { uint8_t seed[32]; memset(seed, 0x40 + i, sizeof(seed)); TEST_ASSERT_EQUAL(0, collab_keys_set(&g_c[i], seed)); }
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


static int count(const char* hay, const char* needle) { int n = 0; const char* p = hay; while ((p = strstr(p, needle)) != 0) { n++; p++; } return n; }
void test_signed_entries_forgery_and_redaction(void) {
    char fpr[17];
    g_sign = 1; setup(); g_sign = 0;
    TEST_ASSERT_EQUAL(3, g_c[0].nkeys);
    TEST_ASSERT_EQUAL(1, g_c[1].has_sig[1]);
    collab_key_fpr(g_c[0].pk, fpr); TEST_ASSERT_EQUAL(16, (int)strlen(fpr));
    collab_emit(&g_c[0], &g_h[0], CE_TRANSFER, k_id[1], 0, 30, 0, ""); deliver();
    /* gamma forges alpha -> gamma transfers three ways */
    TEST_ASSERT_GREATER_THAN(0, collab_forge(&g_c[2], &g_h[2], k_id[0], 50, 0)); deliver();
    TEST_ASSERT_GREATER_THAN(0, collab_forge(&g_c[2], &g_h[2], k_id[0], 50, 1)); deliver();
    TEST_ASSERT_GREATER_THAN(0, collab_forge(&g_c[2], &g_h[2], k_id[0], 50, 2)); deliver();
    TEST_ASSERT_NOT_NULL(strstr(g_out[1], "collab reject bad-signature transfer claimed from alpha seq 3 via gamma"));
    TEST_ASSERT_NOT_NULL(strstr(g_out[1], "collab reject key-mismatch transfer claimed from alpha seq 3 via gamma"));
    TEST_ASSERT_NOT_NULL(strstr(g_out[1], "collab reject unsigned transfer claimed from alpha seq 3 via gamma"));
    TEST_ASSERT_EQUAL(3, (int)g_c[0].forged); TEST_ASSERT_EQUAL(3, (int)g_c[1].forged);
    report(1, "balances", 0);
    TEST_ASSERT_NOT_NULL(strstr(rep, "collab acct alpha balance 70"));
    TEST_ASSERT_NOT_NULL(strstr(rep, "collab acct gamma balance 100"));
    /* the real alpha seq 3 is still accepted afterwards */
    collab_emit(&g_c[0], &g_h[0], CE_TRANSFER, k_id[2], 0, 5, 0, ""); deliver();
    report(1, "balances", 0); TEST_ASSERT_NOT_NULL(strstr(rep, "collab acct gamma balance 105"));
    /* redaction while gamma is cut off; gamma verifies the redacted entry on sync */
    g_cut[2] = 1;
    collab_emit(&g_c[1], &g_h[1], CE_PROFILE, 0, 0, 0, 0, "city=lyon"); deliver();
    TEST_ASSERT_GREATER_THAN(0, collab_forget(&g_c[1], &g_h[1])); deliver();
    g_cut[2] = 0;
    collab_sync(&g_c[2], &g_h[2]); deliver(); deliver();
    TEST_ASSERT_EQUAL(0, count(g_out[2], "collab reject"));
    TEST_ASSERT_NOT_NULL(strstr(g_out[2], "collab got profile from beta"));
    same_digest();
}
void test_save_load_roundtrip(void) {
    static uint8_t buf[32768];
    static collab_t copy;
    collab_state_t a, b;
    int n, i;
    g_sign = 1; setup(); g_sign = 0;
    collab_emit(&g_c[0], &g_h[0], CE_TRANSFER, k_id[1], 0, 10, 0, "");
    collab_emit(&g_c[1], &g_h[1], CE_PROFILE, 0, 0, 0, 0, "city=lyon"); deliver();
    collab_forget(&g_c[1], &g_h[1]); deliver();
    n = collab_save(&g_c[2], buf, sizeof(buf));
    TEST_ASSERT_GREATER_THAN(0, n);
    TEST_ASSERT_EQUAL(-1, collab_save(&g_c[2], buf, 100));
    memset(&copy, 0, sizeof(copy));
    TEST_ASSERT_EQUAL(g_c[2].n, collab_load(&copy, buf, n));
    collab_fold(&g_c[2], &a); collab_fold(&copy, &b);
    TEST_ASSERT_EQUAL(0, memcmp(a.digest, b.digest, 32));
    TEST_ASSERT_EQUAL(1, copy.signing); TEST_ASSERT_EQUAL(0, memcmp(copy.sk, g_c[2].sk, 20));
    for (i = 0; i < copy.n; i++) TEST_ASSERT_EQUAL(0, memcmp(copy.th[i], g_c[2].th[i], 32));
    /* the restored node re-serves signed entries that a fresh peer accepts */
    g_c[2] = copy;
    collab_init(&g_c[0], k_id[0]);
    { uint8_t seed[32]; memset(seed, 0x40, 32); collab_keys_set(&g_c[0], seed); }
    g_out[0][0] = 0;
    collab_sync(&g_c[0], &g_h[0]); deliver(); deliver();
    TEST_ASSERT_EQUAL(0, count(g_out[0], "collab reject"));
    buf[0] ^= 1; TEST_ASSERT_EQUAL(-1, collab_load(&copy, buf, n));
}

static int bal(int node, const char* who) {
    char needle[48];
    const char* p;
    report(node, "balances", 0);
    snprintf(needle, sizeof(needle), "collab acct %s balance ", who);
    p = strstr(rep, needle);
    if (!p) return -9999;
    {
        int v = 0, neg = 0;
        p += strlen(needle);
        if (*p == '-') { neg = 1; p++; }
        while (*p >= '0' && *p <= '9') v = v * 10 + (*p++ - '0');
        return neg ? -v : v;
    }
}

void test_marketplace_contracts_disputes_privacy_compliance(void) {
    uint32_t price = 0, blinded = 0;
    uint8_t proof[32];
    collab_compliance_t comp;
    g_sign = 1;
    setup();
    g_sign = 0;

    /* US-094 Marketplace search & price */
    collab_emit(&g_c[0], &g_h[0], CE_OFFER, 0, 0, 15, 2, "compute-node"); deliver();
    TEST_ASSERT_GREATER_THAN(0, collab_market_search(&g_c[0], &g_h[0], "compute", rep, sizeof(rep)));
    TEST_ASSERT_NOT_NULL(strstr(rep, "market offer alpha#2 'compute-node' price 15"));
    TEST_ASSERT_EQUAL(1, collab_market_price(&g_c[0], "compute", &price));
    TEST_ASSERT_EQUAL(15, (int)price);

    /* US-103: hash commitment opened with value and secret; anything else fails */
    TEST_ASSERT_EQUAL(0, collab_privacy_proof_gen(100, 1234, proof));
    TEST_ASSERT_EQUAL(0, collab_privacy_proof_verify(100, 1234, proof));
    TEST_ASSERT_EQUAL(-1, collab_privacy_proof_verify(101, 1234, proof));
    TEST_ASSERT_EQUAL(-1, collab_privacy_proof_verify(100, 1235, proof));
    proof[31] ^= 1;
    TEST_ASSERT_EQUAL(-1, collab_privacy_proof_verify(100, 1234, proof));
    TEST_ASSERT_EQUAL(0, collab_anonymize_id(k_id[0], 99, &blinded));
    TEST_ASSERT_NOT_EQUAL(k_id[0], blinded);

    /* US-104: computed checks; signed ledger and no leak: 6/6 */
    TEST_ASSERT_EQUAL(0, collab_compliance_check(&g_c[0], &comp));
    TEST_ASSERT_EQUAL(1, comp.signing_on);
    TEST_ASSERT_EQUAL(1, comp.all_signed);
    TEST_ASSERT_EQUAL(1, comp.private_kept_local);
    TEST_ASSERT_EQUAL(100, (int)comp.compliance_score);
    TEST_ASSERT_GREATER_THAN(0, collab_compliance_report(&g_c[0], rep, sizeof(rep)));
    TEST_ASSERT_NOT_NULL(strstr(rep, "compliance signing on all_signed yes forged_applied 0 private_local yes"));
    TEST_ASSERT_NOT_NULL(strstr(rep, "score 100 passed 6/6 (self-check, not a certification)"));
    TEST_ASSERT_NOT_NULL(strstr(rep, "compliance verified 4/4 keys_bound yes erasure effective unerased 0"));
    TEST_ASSERT_EQUAL(1, comp.no_forgery_accepted);
    TEST_ASSERT_EQUAL(4, comp.verified);
    /* Lot 9: a signature altered at rest is found by re-verification */
    g_c[0].sig[1][5] ^= 0x40;
    TEST_ASSERT_EQUAL(0, collab_compliance_check(&g_c[0], &comp));
    TEST_ASSERT_EQUAL(0, comp.no_forgery_accepted);
    TEST_ASSERT_EQUAL(1, comp.bad_signatures);
    TEST_ASSERT_EQUAL(83, (int)comp.compliance_score);
    g_c[0].sig[1][5] ^= 0x40;
    /* Lot 9: erasure is checked on the stored texts */
    TEST_ASSERT_EQUAL(0, collab_profile_set(&g_c[1], "city", "lyon", 1));
    TEST_ASSERT_GREATER_THAN(0, collab_emit(&g_c[1], &g_h[1], CE_PROFILE, 0, 0, 0, 0, "city=lyon")); deliver();
    TEST_ASSERT_GREATER_THAN(0, collab_forget(&g_c[1], &g_h[1])); deliver();
    TEST_ASSERT_EQUAL(0, collab_compliance_check(&g_c[0], &comp));
    TEST_ASSERT_EQUAL(1, comp.erasure_effective);
    TEST_ASSERT_EQUAL(100, (int)comp.compliance_score);
    {
        int i;
        for (i = 0; i < g_c[0].n; i++)
            if (g_c[0].e[i].type == CE_PROFILE && g_c[0].e[i].origin == k_id[1]) strcpy(g_c[0].e[i].text, "city=lyon");
    }
    TEST_ASSERT_EQUAL(0, collab_compliance_check(&g_c[0], &comp));
    TEST_ASSERT_EQUAL(0, comp.erasure_effective);
    TEST_ASSERT_GREATER_THAN(0, comp.unerased);
    {
        int i;
        for (i = 0; i < g_c[0].n; i++)
            if (g_c[0].e[i].type == CE_PROFILE && g_c[0].e[i].origin == k_id[1]) strcpy(g_c[0].e[i].text, "[redacted]");
    }
    /* a private value that leaked into a shared entry is detected */
    TEST_ASSERT_EQUAL(0, collab_profile_set(&g_c[0], "phone", "5550101", 0));
    collab_emit(&g_c[0], &g_h[0], CE_TICKET, 0, 0, 0, 0, "call me 5550101"); deliver();
    TEST_ASSERT_EQUAL(0, collab_compliance_check(&g_c[0], &comp));
    TEST_ASSERT_EQUAL(0, comp.private_kept_local);
    TEST_ASSERT_EQUAL(83, (int)comp.compliance_score);
    /* an unsigned node fails the signing checks */
    setup();
    TEST_ASSERT_EQUAL(0, collab_compliance_check(&g_c[0], &comp));
    TEST_ASSERT_EQUAL(0, comp.signing_on);
    TEST_ASSERT_EQUAL(0, comp.keys_bound);
    TEST_ASSERT_EQUAL(33, (int)comp.compliance_score);
}

/* US-099: escrow contract; US-101: dispute decided by rulings. */
void test_escrow_contracts_and_disputes(void) {
    int s1, s2, s3;
    setup();                                       /* 100 points each */
    /* alpha pays beta 30 through an escrow arbitrated by gamma */
    s1 = collab_escrow_create(&g_c[0], &g_h[0], k_id[1], k_id[2], 30, "gpu-job"); deliver();
    TEST_ASSERT_EQUAL(2, s1);
    TEST_ASSERT_EQUAL(70, bal(1, "alpha"));        /* locked */
    /* the payee cannot release to itself; gamma (arbiter) or alpha can */
    collab_escrow_settle(&g_c[1], &g_h[1], k_id[0], 2, 1); deliver();
    TEST_ASSERT_EQUAL(100, bal(2, "beta"));
    collab_escrow_settle(&g_c[0], &g_h[0], k_id[0], 2, 1); deliver();
    TEST_ASSERT_EQUAL(130, bal(2, "beta"));
    /* settled once: a later refund is rejected */
    collab_escrow_settle(&g_c[2], &g_h[2], k_id[0], 2, 0); deliver();
    TEST_ASSERT_EQUAL(70, bal(1, "alpha"));
    report(0, "escrows", 0);
    TEST_ASSERT_NOT_NULL(strstr(rep, "collab escrow alpha#2 'gpu-job' payee beta arbiter gamma amount 30 state released"));
    report(0, "audit", 0);
    TEST_ASSERT_NOT_NULL(strstr(rep, "reason not-allowed"));
    TEST_ASSERT_NOT_NULL(strstr(rep, "reason bad-state"));
    /* refund by the payee, insufficient funds rejected */
    s2 = collab_escrow_create(&g_c[1], &g_h[1], k_id[0], 0, 500, "too-much"); deliver();
    TEST_ASSERT_EQUAL(130, bal(0, "beta"));
    (void)s2;
    /* beta pays alpha 20, no arbiter; alpha disputes; gamma is the only
     * eligible member (quorum 1) and rules for the payer: beta refunded */
    s3 = collab_escrow_create(&g_c[1], &g_h[1], k_id[0], 0, 20, "delivery"); deliver();
    TEST_ASSERT_EQUAL(110, bal(0, "beta"));
    collab_dispute_open(&g_c[0], &g_h[0], k_id[1], (uint32_t)s3, "never delivered"); deliver();
    /* frozen: release refused while disputed */
    collab_escrow_settle(&g_c[1], &g_h[1], k_id[1], (uint32_t)s3, 1); deliver();
    TEST_ASSERT_EQUAL(70, bal(0, "alpha"));
    /* parties cannot rule */
    collab_dispute_vote(&g_c[0], &g_h[0], k_id[1], (uint32_t)s3, 0); deliver();
    TEST_ASSERT_EQUAL(70, bal(0, "alpha"));
    collab_dispute_vote(&g_c[2], &g_h[2], k_id[1], (uint32_t)s3, 1); deliver();
    TEST_ASSERT_EQUAL(130, bal(0, "beta"));
    TEST_ASSERT_EQUAL(70, bal(0, "alpha"));
    report(2, "escrows", 0);
    TEST_ASSERT_NOT_NULL(strstr(rep, "'delivery' payee alpha arbiter none amount 20 state refunded rulings payer 1 payee 0"));
    /* a second ruling after the decision is rejected; points conserved */
    collab_dispute_vote(&g_c[2], &g_h[2], k_id[1], (uint32_t)s3, 0); deliver();
    TEST_ASSERT_EQUAL(300, bal(0, "alpha") + bal(0, "beta") + bal(0, "gamma"));
    same_digest();
}

/* US-094: auctions with locked bids. */
void test_auctions(void) {
    int a;
    setup();
    a = collab_auction_open(&g_c[2], &g_h[2], "lamp", 10); deliver();
    collab_auction_bid(&g_c[0], &g_h[0], k_id[2], (uint32_t)a, 9); deliver();     /* below reserve */
    collab_auction_bid(&g_c[0], &g_h[0], k_id[2], (uint32_t)a, 12); deliver();
    TEST_ASSERT_EQUAL(88, bal(1, "alpha"));                                      /* locked */
    collab_auction_bid(&g_c[1], &g_h[1], k_id[2], (uint32_t)a, 15); deliver();
    TEST_ASSERT_EQUAL(100, bal(1, "alpha"));                                     /* outbid: repaid */
    TEST_ASSERT_EQUAL(85, bal(1, "beta"));
    collab_auction_bid(&g_c[0], &g_h[0], k_id[2], (uint32_t)a, 14); deliver();    /* does not beat 15 */
    collab_auction_bid(&g_c[2], &g_h[2], k_id[2], (uint32_t)a, 50); deliver();    /* seller cannot bid */
    collab_auction_close(&g_c[1], &g_h[1], (uint32_t)a); deliver();              /* only the seller closes */
    report(0, "auctions", 0);
    TEST_ASSERT_NOT_NULL(strstr(rep, "collab auction gamma#2 'lamp' reserve 10 best 15 bidder beta bids 2 state open"));
    collab_auction_close(&g_c[2], &g_h[2], (uint32_t)a); deliver();
    TEST_ASSERT_EQUAL(115, bal(0, "gamma"));
    report(1, "auctions", 0);
    TEST_ASSERT_NOT_NULL(strstr(rep, "state sold"));
    collab_auction_bid(&g_c[0], &g_h[0], k_id[2], (uint32_t)a, 40); deliver();    /* closed */
    TEST_ASSERT_EQUAL(100, bal(2, "alpha"));
    report(0, "audit", 0);
    TEST_ASSERT_NOT_NULL(strstr(rep, "reason bid-too-low"));
    TEST_ASSERT_EQUAL(300, bal(0, "alpha") + bal(0, "beta") + bal(0, "gamma"));
    /* no bid: unsold, nobody paid */
    a = collab_auction_open(&g_c[0], &g_h[0], "chair", 5); deliver();
    collab_auction_close(&g_c[0], &g_h[0], (uint32_t)a); deliver();
    report(2, "auctions", 0);
    TEST_ASSERT_NOT_NULL(strstr(rep, "'chair' reserve 5 best 0 bidder none bids 0 state unsold"));
    same_digest();
}

void test_payment_conversion_escrow_and_ticket_routing(void) {
    uint32_t conv = 0;
    setup();
    /* US-097: fixed-rate conversion only (simule, no payment) */
    TEST_ASSERT_EQUAL(0, collab_point_convert(100, "USD", &conv));
    TEST_ASSERT_EQUAL(10, (int)conv);
    TEST_ASSERT_EQUAL(0, collab_point_convert(100, "TOKEN", &conv));
    TEST_ASSERT_EQUAL(200, (int)conv);
    /* US-105 Ticket routing */
    collab_emit(&g_c[0], &g_h[0], CE_TICKET, 0, 0, 0, 0, "network issue"); deliver();
    TEST_ASSERT_GREATER_THAN(0, collab_ticket_route(&g_c[1], &g_h[1], 2, k_id[2])); deliver();
    TEST_ASSERT_GREATER_THAN(0, collab_ticket_assign(&g_c[2], &g_h[2], 2, "agent_beta")); deliver();
}

void test_collab_consolidation_and_fallback(void) {
    uint32_t price = 0;
    setup();
    /* references to unknown escrows and auctions are rejected by the fold */
    collab_escrow_settle(&g_c[0], &g_h[0], k_id[1], 999, 1); deliver();
    collab_dispute_open(&g_c[0], &g_h[0], k_id[1], 999, "unknown"); deliver();
    collab_auction_bid(&g_c[0], &g_h[0], k_id[1], 999, 5); deliver();
    report(1, "audit", 0);
    TEST_ASSERT_NOT_NULL(strstr(rep, "rejected 3"));
    TEST_ASSERT_GREATER_THAN(0, collab_ticket_route(&g_c[0], &g_h[0], 999, k_id[1]));
    TEST_ASSERT_GREATER_THAN(0, collab_ticket_assign(&g_c[0], &g_h[0], 999, "none"));
    TEST_ASSERT_EQUAL(0, collab_market_price(&g_c[0], "missing-item", &price));
}

void test_crypto_schnorr_pbkdf2_aes_gcm_pipeline(void) {
    uint8_t seed1[32], seed2[32];
    uint8_t sk1[CSIG_SK], pk1[CSIG_PK];
    uint8_t sk2[CSIG_SK], pk2[CSIG_PK];
    uint8_t sig[CSIG_SIG];
    const uint8_t msg[] = "MOHHDY Cryptographic Stack Verification 2026";
    int msg_len = (int)sizeof(msg) - 1;

    memset(seed1, 0xA5, sizeof(seed1));
    memset(seed2, 0x5A, sizeof(seed2));

    /* Key generation and validation */
    TEST_ASSERT_EQUAL(0, csig_keypair(seed1, sk1, pk1));
    TEST_ASSERT_EQUAL(0, csig_keypair(seed2, sk2, pk2));
    TEST_ASSERT_EQUAL(1, csig_pk_valid(pk1));
    TEST_ASSERT_EQUAL(1, csig_pk_valid(pk2));

    /* Schnorr signature creation & verification */
    TEST_ASSERT_EQUAL(0, csig_sign(sk1, msg, msg_len, sig));
    TEST_ASSERT_EQUAL(1, csig_verify(pk1, msg, msg_len, sig));
    TEST_ASSERT_EQUAL(0, csig_verify(pk2, msg, msg_len, sig)); /* WRONG PK */

    /* Alter message -> verify fail */
    TEST_ASSERT_EQUAL(0, csig_verify(pk1, (const uint8_t*)"Corrupted message", 17, sig));

    /* Alter signature -> verify fail */
    sig[0] ^= 0xFF;
    TEST_ASSERT_EQUAL(0, csig_verify(pk1, msg, msg_len, sig));
    sig[0] ^= 0xFF; /* restore */

    /* PBKDF2-HMAC-SHA256 & AES-128-GCM Key wrap test for secret storage */
    {
        const char *passphrase = "SecretPassphrase2026";
        uint8_t salt[16] = {0x10,0x20,0x30,0x40,0x50,0x60,0x70,0x80,0x90,0xA0,0xB0,0xC0,0xD0,0xE0,0xF0,0x00};
        uint8_t key[16], fixed_iv[4] = {1,2,3,4}, nonce[8] = {5,6,7,8,9,10,11,12}, tag[16], aad[4] = {'A','E','S','1'};
        uint8_t ciphertext[CSIG_SK], decrypted[CSIG_SK];
        uint8_t hmac_input[20], u[32], acc[32];
        int i, k, pl = strlen(passphrase);

        /* PBKDF2 derivation (1 block, 2000 iterations, 16 bytes) */
        memcpy(hmac_input, salt, 16);
        hmac_input[16] = 0; hmac_input[17] = 0; hmac_input[18] = 0; hmac_input[19] = 1;
        hmac_sha256((const uint8_t*)passphrase, pl, hmac_input, 20, u);
        memcpy(acc, u, 32);
        for (k = 1; k < 2000; k++) {
            hmac_sha256((const uint8_t*)passphrase, pl, u, 32, u);
            for (i = 0; i < 32; i++) acc[i] ^= u[i];
        }
        memcpy(key, acc, 16);

        /* AES-128-GCM encrypt signing secret key */
        TEST_ASSERT_EQUAL(0, aes128_gcm_encrypt(key, fixed_iv, nonce, aad, 4, sk1, CSIG_SK, ciphertext, tag));

        /* AES-128-GCM decrypt with correct key */
        TEST_ASSERT_EQUAL(0, aes128_gcm_decrypt(key, fixed_iv, nonce, aad, 4, ciphertext, CSIG_SK, tag, decrypted));
        TEST_ASSERT_EQUAL_MEMORY(sk1, decrypted, CSIG_SK);

        /* AES-128-GCM decrypt with corrupted tag -> fail */
        tag[0] ^= 0x01;
        TEST_ASSERT_NOT_EQUAL(0, aes128_gcm_decrypt(key, fixed_iv, nonce, aad, 4, ciphertext, CSIG_SK, tag, decrypted));
    }
}


/* Lot 8 US-103: Schnorr NIZK over the 62-bit safe-prime group. */
/* Lot 9 (US-103): the same proof in a 2048-bit group, 256-bit subgroup.
 * The expected bytes come from an independent Python big-integer run of
 * the same equations (see docs/collab.md). */
void test_zk_schnorr_2048(void) {
    uint8_t seed[32], sk[CSIG_ZK_SK], pk[CSIG_ZK_PK], pr[CSIG_ZK_PROOF], bad[CSIG_ZK_PROOF];
    uint8_t sk2[CSIG_ZK_SK], pk2[CSIG_ZK_PK];
    int i;
    for (i = 0; i < 32; i++) seed[i] = (uint8_t)i;
    TEST_ASSERT_EQUAL(1, csig_zk_group_check());
    TEST_ASSERT_EQUAL(0, csig_zk_keypair(seed, sk, pk));
    TEST_ASSERT_EQUAL(0, csig_zk_prove(sk, (const uint8_t*)"pay 10", 6, pr));
    TEST_ASSERT_EQUAL(1, csig_zk_verify(pk, (const uint8_t*)"pay 10", 6, pr));
    {   /* known answer from the Python reference */
        static const uint8_t kpk[8] = { 0x16, 0xb2, 0x63, 0xd5, 0x47, 0xc6, 0x31, 0x5c };
        static const uint8_t kt[8] = { 0x38, 0xae, 0x04, 0x83, 0x02, 0xbe, 0x85, 0xbc };
        static const uint8_t ks[8] = { 0xc5, 0x93, 0xaf, 0x76, 0xe0, 0x8f, 0x56, 0x26 };
        TEST_ASSERT_EQUAL(0, memcmp(pk, kpk, 8));
        TEST_ASSERT_EQUAL(0, memcmp(pr, kt, 8));
        TEST_ASSERT_EQUAL(0, memcmp(pr + CSIG_ZK_PROOF - 8, ks, 8));
    }
    TEST_ASSERT_EQUAL(0, csig_zk_verify(pk, (const uint8_t*)"pay 11", 6, pr));   /* bound to the message */
    for (i = 0; i < CSIG_ZK_PROOF; i++) bad[i] = pr[i];
    bad[300 - 20] ^= 1U;                                                     /* s */
    TEST_ASSERT_EQUAL(0, csig_zk_verify(pk, (const uint8_t*)"pay 10", 6, bad));
    for (i = 0; i < CSIG_ZK_PROOF; i++) bad[i] = pr[i];
    bad[100] ^= 1U;                                                          /* t */
    TEST_ASSERT_EQUAL(0, csig_zk_verify(pk, (const uint8_t*)"pay 10", 6, bad));
    seed[0] = 99;
    TEST_ASSERT_EQUAL(0, csig_zk_keypair(seed, sk2, pk2));
    TEST_ASSERT_EQUAL(0, csig_zk_verify(pk2, (const uint8_t*)"pay 10", 6, pr)); /* other key */
    for (i = 0; i < CSIG_ZK_PK; i++) pk2[i] = 0;
    pk2[CSIG_ZK_PK - 1] = 1;
    TEST_ASSERT_EQUAL(0, csig_zk_verify(pk2, (const uint8_t*)"pay 10", 6, pr)); /* y = 1 refused */
    for (i = 0; i < CSIG_ZK_SK; i++) sk2[i] = 0;
    TEST_ASSERT_EQUAL(-1, csig_zk_prove(sk2, (const uint8_t*)"x", 1, pr));      /* x = 0 */
}

void test_zk_schnorr_proof(void) {
    collab_zk_proof_t pr, bad;
    uint64_t y = collab_zk_public(123456789ULL);
    TEST_ASSERT_TRUE(collab_zk_powmod(COLLAB_ZK_G, COLLAB_ZK_Q) == 1ULL);   /* g has order q */
    TEST_ASSERT_TRUE(collab_zk_powmod(2ULL, COLLAB_ZK_P - 1ULL) == 1ULL);    /* Fermat on p */
    TEST_ASSERT_TRUE(collab_zk_powmod(3ULL, 5ULL) == 243ULL);
    TEST_ASSERT_EQUAL(0, collab_zk_prove(123456789ULL, "pay 10", &pr));
    TEST_ASSERT_EQUAL(0, collab_zk_verify(y, "pay 10", &pr));
    TEST_ASSERT_EQUAL(-1, collab_zk_verify(y, "pay 11", &pr));             /* bound to the message */
    TEST_ASSERT_EQUAL(-1, collab_zk_verify(collab_zk_public(987654321ULL), "pay 10", &pr));
    bad = pr; bad.s ^= 1ULL;
    TEST_ASSERT_EQUAL(-1, collab_zk_verify(y, "pay 10", &bad));
    bad = pr; bad.t = 2ULL;                                                 /* not in the subgroup */
    TEST_ASSERT_EQUAL(-1, collab_zk_verify(y, "pay 10", &bad));
    TEST_ASSERT_EQUAL(-1, collab_zk_verify(1ULL, "pay 10", &pr));
    TEST_ASSERT_EQUAL(-1, collab_zk_prove(0ULL, "x", &pr));
    TEST_ASSERT_EQUAL(-1, collab_zk_prove(COLLAB_ZK_Q, "x", &pr));         /* x = 0 mod q */
    /* the proof does not contain the secret */
    TEST_ASSERT_TRUE(pr.s != 123456789ULL && pr.t != 123456789ULL);
}


/* Lot 9 (US-099): contract language, compiled and run in the fold. */
void test_contract_language_compile(void) {
    collab_mcl_t m;
    collab_mcl_facts_t f;
    TEST_ASSERT_EQUAL(0, collab_mcl_compile("pay if payee&arb;refund if after 20", &m));
    TEST_ASSERT_EQUAL(2, m.n);
    TEST_ASSERT_EQUAL(MCL_PAY, m.cl[0].action);
    TEST_ASSERT_EQUAL(2, m.cl[0].nc);
    TEST_ASSERT_EQUAL(MCL_ARB, m.cl[0].cond[1].kind);
    TEST_ASSERT_EQUAL(MCL_AFTER, m.cl[1].cond[0].kind);
    TEST_ASSERT_EQUAL(20, m.cl[1].cond[0].arg);
    TEST_ASSERT_EQUAL(0, collab_mcl_compile("pay if payee arb else refund if after 9", &m));
    TEST_ASSERT_EQUAL(2, m.n);
    TEST_ASSERT_EQUAL(2, m.cl[0].nc);
    TEST_ASSERT_EQUAL(0, collab_mcl_compile("split 25 if sigs 1", &m));
    TEST_ASSERT_EQUAL(MCL_SPLIT, m.cl[0].action);
    TEST_ASSERT_EQUAL(25, m.cl[0].pct);
    /* errors carry the 1-based offset */
    TEST_ASSERT_EQUAL(-1, collab_mcl_compile("send if payee", &m));
    TEST_ASSERT_EQUAL(-5, collab_mcl_compile("pay payee", &m));
    TEST_ASSERT_TRUE(collab_mcl_compile("split 0 if payee", &m) < 0);
    TEST_ASSERT_TRUE(collab_mcl_compile("split 100 if payee", &m) < 0);
    TEST_ASSERT_TRUE(collab_mcl_compile("pay if sigs 0", &m) < 0);
    TEST_ASSERT_TRUE(collab_mcl_compile("pay if", &m) < 0);
    TEST_ASSERT_TRUE(collab_mcl_compile("pay if payee;", &m) < 0);
    TEST_ASSERT_TRUE(collab_mcl_compile("pay if payee x", &m) < 0);
    TEST_ASSERT_TRUE(collab_mcl_compile("pay if payee arb after 1 sigs 2", &m) < 0);   /* 4 conditions */
    TEST_ASSERT_TRUE(collab_mcl_compile("pay if payee;pay if arb;pay if sigs 1;refund if after 1", &m) < 0);
    TEST_ASSERT_TRUE(collab_mcl_compile("payee if payee", &m) < 0);                /* keyword boundary */
    /* evaluation: first clause that holds */
    TEST_ASSERT_EQUAL(0, collab_mcl_compile("pay if payee&arb;refund if after 20", &m));
    memset(&f, 0, sizeof(f));
    f.payee = 7; f.arbiter = 9;
    TEST_ASSERT_EQUAL(-1, collab_mcl_eval(&m, &f));
    f.signers[f.ns++] = 7;
    TEST_ASSERT_EQUAL(-1, collab_mcl_eval(&m, &f));
    f.elapsed = 20;
    TEST_ASSERT_EQUAL(1, collab_mcl_eval(&m, &f));
    f.signers[f.ns++] = 9;
    TEST_ASSERT_EQUAL(0, collab_mcl_eval(&m, &f));
    f.arbiter = 0; f.signers[1] = 0;                                     /* no arbiter: arb never holds */
    f.elapsed = 0;
    TEST_ASSERT_EQUAL(-1, collab_mcl_eval(&m, &f));
}

void test_contract_language_in_the_ledger(void) {
    int c1, c2, c3, c4;
    setup();                                                  /* 100 points each */
    /* alpha -> beta 40, arbiter gamma: paid when beta and gamma sign off */
    c1 = collab_contract_deploy(&g_c[0], &g_h[0], k_id[1], k_id[2], 40, "pay if payee arb;refund if after 30");
    TEST_ASSERT_GREATER_THAN(0, c1);
    deliver();
    TEST_ASSERT_EQUAL(60, bal(1, "alpha"));
    collab_contract_signoff(&g_c[1], &g_h[1], k_id[0], (uint32_t)c1); deliver();
    TEST_ASSERT_EQUAL(100, bal(2, "beta"));                    /* one condition only */
    collab_contract_signoff(&g_c[0], &g_h[0], k_id[0], (uint32_t)c1); deliver();   /* payer cannot sign */
    collab_contract_signoff(&g_c[1], &g_h[1], k_id[0], (uint32_t)c1); deliver();   /* duplicate */
    collab_contract_signoff(&g_c[2], &g_h[2], k_id[0], (uint32_t)c1); deliver();
    TEST_ASSERT_EQUAL(140, bal(0, "beta"));
    TEST_ASSERT_EQUAL(140, bal(1, "beta"));
    TEST_ASSERT_EQUAL(140, bal(2, "beta"));
    report(0, "escrows", 0);
    TEST_ASSERT_NOT_NULL(strstr(rep, "payee beta arbiter gamma amount 40 state released"));
    TEST_ASSERT_NOT_NULL(strstr(rep, "program clauses 2 signoffs 2 fired 1 to_payee 40"));
    /* fired once: a later sign-off is rejected (bad-state) */
    collab_contract_signoff(&g_c[2], &g_h[2], k_id[0], (uint32_t)c1); deliver();
    report(1, "audit", 0);
    TEST_ASSERT_NOT_NULL(strstr(rep, "reason not-allowed"));
    TEST_ASSERT_NOT_NULL(strstr(rep, "reason duplicate"));
    /* split: gamma -> alpha 20, 25 percent after one sign-off */
    c2 = collab_contract_deploy(&g_c[2], &g_h[2], k_id[0], 0, 20, "split 25 if sigs 1"); deliver();
    collab_contract_signoff(&g_c[1], &g_h[1], k_id[2], (uint32_t)c2); deliver();
    TEST_ASSERT_EQUAL(65, bal(0, "alpha"));
    TEST_ASSERT_EQUAL(95, bal(0, "gamma"));
    /* deadline: beta -> gamma 10, refunded once the ledger clock moves 3 */
    c3 = collab_contract_deploy(&g_c[1], &g_h[1], k_id[2], 0, 10, "pay if payee;refund if after 3"); deliver();
    TEST_ASSERT_EQUAL(130, bal(0, "beta"));
    collab_emit(&g_c[0], &g_h[0], CE_TRANSFER, k_id[1], 0, 1, 0, ""); deliver();
    collab_emit(&g_c[0], &g_h[0], CE_TRANSFER, k_id[1], 0, 1, 0, ""); deliver();
    collab_emit(&g_c[0], &g_h[0], CE_TRANSFER, k_id[1], 0, 1, 0, ""); deliver();
    collab_emit(&g_c[0], &g_h[0], CE_TRANSFER, k_id[1], 0, 1, 0, ""); deliver();
    report(2, "escrows", 0);
    TEST_ASSERT_NOT_NULL(strstr(rep, "payee gamma arbiter none amount 10 state refunded program clauses 2 signoffs 0 fired 2"));
    (void)c3;
    /* a program that names the arbiter without one is rejected everywhere,
     * nothing is locked; a bad program is refused before emission */
    c4 = collab_contract_deploy(&g_c[0], &g_h[0], k_id[1], 0, 5, "pay if arb"); deliver();
    TEST_ASSERT_GREATER_THAN(0, c4);
    report(1, "audit", 0);
    TEST_ASSERT_NOT_NULL(strstr(rep, "reason bad-contract"));
    TEST_ASSERT_EQUAL(-1, collab_contract_deploy(&g_c[0], &g_h[0], k_id[1], 0, 5, "pay when payee"));
    TEST_ASSERT_EQUAL(300, bal(0, "alpha") + bal(0, "beta") + bal(0, "gamma"));
    same_digest();
}

int main(void) {
    unity_init();
    RUN_TEST(test_zk_schnorr_proof);
    RUN_TEST(test_zk_schnorr_2048);
    RUN_TEST(test_points_transfer_overdraft_and_convergence);
    RUN_TEST(test_resources_marketplace_and_reputation);
    RUN_TEST(test_distributed_task_escrow_and_arbitration);
    RUN_TEST(test_governance);
    RUN_TEST(test_privacy_forget_and_export);
    RUN_TEST(test_sync_after_partition_and_tickets);
    RUN_TEST(test_codec_and_eval);
    RUN_TEST(test_signed_entries_forgery_and_redaction);
    RUN_TEST(test_save_load_roundtrip);
    RUN_TEST(test_marketplace_contracts_disputes_privacy_compliance);
    RUN_TEST(test_escrow_contracts_and_disputes);
    RUN_TEST(test_auctions);
    RUN_TEST(test_contract_language_compile);
    RUN_TEST(test_contract_language_in_the_ledger);
    RUN_TEST(test_payment_conversion_escrow_and_ticket_routing);
    RUN_TEST(test_collab_consolidation_and_fallback);
    RUN_TEST(test_crypto_schnorr_pbkdf2_aes_gcm_pipeline);
    unity_print_results();
    unity_cleanup();
    return (unity_stats.tests_failed == 0) ? 0 : 1;
}
