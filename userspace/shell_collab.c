/* userspace/shell_collab.c - collab-* shell commands (Phase 7, docs/collab.md).
 * The ledger rides on the phase 5 P2P node (sealed P2P_I_APP payloads). */
#include "collab.h"
#include "csig.h"
#include "p2p.h"
#include "../kernel/sha256.h"

void print_string(const char* s);
p2p_node_t* shell_p2p_node(void);
const p2p_host_t* shell_p2p_host(void);

static collab_t g_c;
static int g_joined;
static char g_out[8192];

static int c_send(void* ctx, uint32_t to, const uint8_t* d, int len) {
    (void)ctx;
    return p2p_send_app(shell_p2p_node(), shell_p2p_host(), to, d, len);
}
static void c_out(void* ctx, const char* s) { (void)ctx; print_string(s); print_string("\n"); }
static const char* c_name(void* ctx, uint32_t id) { (void)ctx; return p2p_peer_name(shell_p2p_node(), id); }
static uint32_t c_members(void* ctx) { (void)ctx; return p2p_member_count(shell_p2p_node()); }
static const collab_host_t g_h = {0, c_send, c_out, c_name, c_members};
static void on_app(void* ctx, uint32_t from, const uint8_t* d, int len) { (void)ctx; collab_receive(&g_c, &g_h, from, d, len); }

static int s_eq(const char* a, const char* b) { while (*a && *a == *b) { a++; b++; } return *a == *b; }
static int s_len(const char* s) { int n = 0; while (s[n]) n++; return n; }
static const char* skip(const char* s) { while (*s == ' ') s++; return s; }
static const char* word(const char* s, char* w, int cap) {
    int n = 0;
    s = skip(s);
    while (*s && *s != ' ') { if (n < cap - 1) w[n++] = *s; s++; }
    w[n] = 0;
    return skip(s);
}
static int num(const char* s, uint32_t* v) {
    uint32_t r = 0; int any = 0;
    while (*s >= '0' && *s <= '9') { r = r * 10U + (uint32_t)(*s++ - '0'); any = 1; }
    if (!any || *s) return -1;
    *v = r; return 0;
}
static void put_u(uint32_t v) {
    char t[11], r[12]; int n = 0, i = 0;
    do { t[n++] = (char)('0' + v % 10U); v /= 10U; } while (v);
    while (n) r[i++] = t[--n];
    r[i] = 0; print_string(r);
}
static uint32_t peer_id(const char* name) {
    p2p_node_t* n = shell_p2p_node();
    p2p_peer_t* p;
    if (s_eq(name, n->name)) return n->id;
    p = p2p_find(n, name);
    return p ? p->id : 0;
}
static int emitted(const char* cmd, int seq) {
    print_string(cmd);
    if (seq > 0) { print_string(" ok seq "); put_u((uint32_t)seq); print_string("\n"); return 0; }
    print_string(seq == -2 ? " error ledger full\n" : " error bad arguments\n");
    return 1;
}
static int find_entry(uint32_t origin, uint32_t seq) {
    int i;
    for (i = 0; i < g_c.n; i++) if (g_c.e[i].origin == origin && g_c.e[i].seq == seq) return i;
    return -1;
}

int shell_collab_line(const char* line) {
    char cmd[24], a[48], b[48], c[48];
    uint32_t x, y, pid;
    const char* rest = word(line, cmd, (int)sizeof(cmd));
    p2p_node_t* node = shell_p2p_node();
    if (!node->up) { print_string(cmd); print_string(" error p2p down (p2p-up first)\n"); return 1; }
    if (s_eq(cmd, "collab-join")) {
        if (!g_joined || g_c.self != node->id) { collab_init(&g_c, node->id); g_joined = 1; }
        node->app = on_app; node->app_ctx = 0;
        if (!g_c.signing) {
            /* per-node signing key: seed from the TSC, ticks and node id */
            sha256_ctx_t hc; uint8_t seed[32]; uint32_t lo, hi, k;
            char fpr[17];
            sha256_init(&hc);
            for (k = 0; k < 8; k++) { asm volatile("rdtsc" : "=a"(lo), "=d"(hi)); sha256_update(&hc, (const uint8_t*)&lo, 4); sha256_update(&hc, (const uint8_t*)&hi, 4); }
            sha256_update(&hc, (const uint8_t*)&node->id, 4); sha256_update(&hc, (const uint8_t*)node->secret, 32);
            sha256_final(&hc, seed);
            if (collab_keys_set(&g_c, seed) != 0) { print_string("collab-join error key generation\n"); return 1; }
            collab_key_fpr(g_c.pk, fpr);
            print_string("collab key schnorr-1024/160 fpr "); print_string(fpr); print_string("\n");
        }
        {
            /* entries sent before this node joined were dropped: catch up */
            int rc = emitted(cmd, collab_emit(&g_c, &g_h, CE_JOIN, 0, 0, 0, 0, node->name));
            (void)collab_sync(&g_c, &g_h);
            return rc;
        }
    }
    if (!g_joined) { print_string(cmd); print_string(" error collab-join first\n"); return 1; }
    node->app = on_app;
    rest = word(rest, a, 48);
    if (s_eq(cmd, "collab-pay")) {
        rest = word(rest, b, 48);
        if (!(pid = peer_id(a)) || num(b, &x)) return emitted(cmd, -1);
        return emitted(cmd, collab_emit(&g_c, &g_h, CE_TRANSFER, pid, 0, x, 0, ""));
    }
    if (s_eq(cmd, "collab-offer")) {
        rest = word(rest, b, 48); rest = word(rest, c, 48);
        if (num(b, &x) || num(c, &y)) return emitted(cmd, -1);
        return emitted(cmd, collab_emit(&g_c, &g_h, CE_OFFER, 0, 0, x, y, a));
    }
    if (s_eq(cmd, "collab-reserve") || s_eq(cmd, "collab-claim") || s_eq(cmd, "collab-work")) {
        rest = word(rest, b, 48);
        if (!(pid = peer_id(a)) || num(b, &x)) return emitted(cmd, -1);
        if (s_eq(cmd, "collab-reserve")) return emitted(cmd, collab_emit(&g_c, &g_h, CE_RESERVE, pid, x, 0, 0, ""));
        if (s_eq(cmd, "collab-claim")) return emitted(cmd, collab_emit(&g_c, &g_h, CE_CLAIM, pid, x, 0, 0, ""));
        {
            int k = find_entry(pid, x);
            uint32_t r;
            if (k < 0 || g_c.e[k].type != CE_TASK || collab_task_eval(g_c.e[k].text, &r) != 0) {
                print_string("collab-work error unknown task\n"); return 1;
            }
            word(rest, c, 48);
            if (s_eq(c, "wrong")) r++; /* test hook: a dishonest worker */
            print_string("collab-work computed "); print_string(g_c.e[k].text); print_string(" = "); put_u(r); print_string("\n");
            return emitted(cmd, collab_emit(&g_c, &g_h, CE_DONE, pid, x, 0, r, ""));
        }
    }
    if (s_eq(cmd, "collab-task")) {
        if (num(a, &x) || !*rest) return emitted(cmd, -1);
        return emitted(cmd, collab_emit(&g_c, &g_h, CE_TASK, 0, 0, x, 0, rest));
    }
    if (s_eq(cmd, "collab-review")) {
        int i, k, found = -1;
        uint32_t truth;
        if (num(a, &x) || (k = find_entry(g_c.self, x)) < 0 || collab_task_eval(g_c.e[k].text, &truth) != 0) {
            print_string("collab-review error unknown task\n"); return 1;
        }
        for (i = 0; i < g_c.n; i++)
            if (g_c.e[i].type == CE_DONE && g_c.e[i].peer == g_c.self && g_c.e[i].ref == x) found = i;
        if (found < 0) { print_string("collab-review error no result yet\n"); return 1; }
        print_string(g_c.e[found].aux == truth ? "collab-review result correct, accepting\n" : "collab-review result wrong, rejecting\n");
        return emitted(cmd, collab_emit(&g_c, &g_h, g_c.e[found].aux == truth ? CE_ACCEPT : CE_REJECT,
                                         g_c.e[found].origin, x, 0, 0, ""));
    }
    if (s_eq(cmd, "collab-rate")) {
        rest = word(rest, b, 48);
        if (!(pid = peer_id(a)) || num(b, &x)) return emitted(cmd, -1);
        return emitted(cmd, collab_emit(&g_c, &g_h, CE_RATE, pid, 0, x, 0, ""));
    }
    if (s_eq(cmd, "collab-propose")) {
        char text[COLLAB_TEXT]; int i = 0;
        const char* t = line; /* whole text after the command */
        t = skip(t); while (*t && *t != ' ') t++; t = skip(t);
        while (t[i] && i < COLLAB_TEXT - 1) { text[i] = t[i]; i++; }
        text[i] = 0;
        return emitted(cmd, collab_emit(&g_c, &g_h, CE_PROPOSE, 0, 0, 0, 0, text));
    }
    if (s_eq(cmd, "collab-vote")) {
        rest = word(rest, b, 48); rest = word(rest, c, 48);
        if (!(pid = peer_id(a)) || num(b, &x) || (!s_eq(c, "yes") && !s_eq(c, "no"))) return emitted(cmd, -1);
        return emitted(cmd, collab_emit(&g_c, &g_h, CE_VOTE, pid, x, s_eq(c, "yes") ? 1U : 0U, 0, ""));
    }
    if (s_eq(cmd, "collab-profile")) {
        if (s_eq(a, "show")) {
            word(rest, b, 48);
            if (!(pid = peer_id(b))) { print_string("collab-profile error unknown peer\n"); return 1; }
            collab_report(&g_c, &g_h, "profile", pid, g_out, (int)sizeof(g_out));
            print_string(g_out);
            return 0;
        }
        if (s_eq(a, "set")) {
            char kv[COLLAB_TEXT];
            int shared, i = 0, j = 0;
            rest = word(rest, b, 48); rest = word(rest, c, 48);
            shared = s_eq(rest, "shared");
            if (!shared && !s_eq(rest, "private")) { print_string("collab-profile error usage: set KEY VALUE shared|private\n"); return 1; }
            if (collab_profile_set(&g_c, b, c, shared) != 0) { print_string("collab-profile error bad field\n"); return 1; }
            if (!shared) { print_string("collab-profile ok private kept local\n"); return 0; }
            while (b[i] && j < COLLAB_TEXT - 2) kv[j++] = b[i++];
            kv[j++] = '='; i = 0;
            while (c[i] && j < COLLAB_TEXT - 1) kv[j++] = c[i++];
            kv[j] = 0;
            return emitted(cmd, collab_emit(&g_c, &g_h, CE_PROFILE, 0, 0, 0, 0, kv));
        }
        print_string("collab-profile error usage: set|show\n");
        return 1;
    }
    if (s_eq(cmd, "collab-forget")) return emitted(cmd, collab_forget(&g_c, &g_h));
    if (s_eq(cmd, "collab-ticket")) {
        char text[COLLAB_TEXT]; int i = 0;
        const char* t = skip(line); while (*t && *t != ' ') t++; t = skip(t);
        while (t[i] && i < COLLAB_TEXT - 1) { text[i] = t[i]; i++; }
        text[i] = 0;
        return emitted(cmd, collab_emit(&g_c, &g_h, CE_TICKET, 0, 0, 0, 0, text));
    }
    if (s_eq(cmd, "collab-answer")) {
        rest = word(rest, b, 48);
        if (!(pid = peer_id(a)) || num(b, &x) || !*rest || s_len(rest) >= COLLAB_TEXT) return emitted(cmd, -1);
        return emitted(cmd, collab_emit(&g_c, &g_h, CE_ANSWER, pid, x, 0, 0, rest));
    }
    if (s_eq(cmd, "collab-keys")) {
        int i; char fpr[17];
        for (i = 0; i < g_c.nkeys; i++) {
            collab_key_fpr(g_c.key_pk[i], fpr);
            print_string("collab key "); print_string(p2p_peer_name(node, g_c.key_origin[i])); print_string(" fpr "); print_string(fpr); print_string("\n");
        }
        print_string("collab-keys ok known "); put_u((uint32_t)g_c.nkeys); print_string(" rejected "); put_u(g_c.forged);
        print_string(g_c.signing ? " signing on\n" : " signing off\n");
        return 0;
    }
    if (s_eq(cmd, "collab-forge")) {
        /* test hook: forge a transfer from victim A to us */
        int mode, rc;
        rest = word(rest, b, 48); rest = word(rest, c, 48);
        mode = s_eq(c, "victimkey") ? 0 : s_eq(c, "ownkey") ? 1 : s_eq(c, "unsigned") ? 2 : -1;
        if (!(pid = peer_id(a)) || num(b, &x) || mode < 0) { print_string("collab-forge error usage: NAME AMOUNT victimkey|ownkey|unsigned\n"); return 1; }
        rc = collab_forge(&g_c, &g_h, pid, x, mode);
        if (rc < 0) { print_string("collab-forge error\n"); return 1; }
        print_string("collab-forge sent "); print_string(c); print_string(" to "); put_u((uint32_t)rc); print_string("\n");
        return 0;
    }
    if (s_eq(cmd, "collab-sync")) {
        int n = collab_sync(&g_c, &g_h);
        print_string("collab-sync ok asked "); put_u(n > 0 ? (uint32_t)n : 0U); print_string("\n");
        return 0;
    }
    if (s_eq(cmd, "collab-market-search")) {
        collab_market_search(&g_c, &g_h, a, g_out, (int)sizeof(g_out));
        print_string(g_out);
        return 0;
    }
    if (s_eq(cmd, "collab-pay-convert")) {
        uint32_t pts, conv = 0;
        rest = word(rest, b, 48);
        if (num(a, &pts) || !b[0]) { print_string("collab-pay-convert error usage: POINTS CURRENCY\n"); return 1; }
        collab_point_convert(pts, b, &conv);
        print_string("collab-pay-convert simule points "); put_u(pts); print_string(" currency "); print_string(b);
        print_string(" amount "); put_u(conv); print_string(" (fixed rate, no payment)\n");
        return 0;
    }
    /* Lot 7 (US-099): escrow contracts folded from the ledger */
    if (s_eq(cmd, "collab-escrow-create") || s_eq(cmd, "collab-contract-create")) {
        uint32_t payee_id, arbiter_id = 0, amt;
        rest = word(rest, b, 48); rest = word(rest, c, 48);
        payee_id = peer_id(a);
        if (!s_eq(b, "none")) arbiter_id = peer_id(b);
        if (!payee_id || (!arbiter_id && !s_eq(b, "none")) || num(c, &amt)) {
            print_string(cmd); print_string(" error usage: PAYEE ARBITER|none AMOUNT [NAME]\n");
            return 1;
        }
        return emitted(cmd, collab_escrow_create(&g_c, &g_h, payee_id, arbiter_id, amt, rest));
    }
    if (s_eq(cmd, "collab-escrow-release") || s_eq(cmd, "collab-escrow-refund") || s_eq(cmd, "collab-contract-exec")) {
        int release = s_eq(cmd, "collab-escrow-release");
        rest = word(rest, b, 48);
        if (s_eq(cmd, "collab-contract-exec")) { rest = word(rest, c, 48); release = s_eq(c, "release"); if (!release && !s_eq(c, "refund")) b[0] = 0; }
        if (!(pid = peer_id(a)) || num(b, &x)) { print_string(cmd); print_string(" error usage: PAYER SEQ\n"); return 1; }
        return emitted(cmd, collab_escrow_settle(&g_c, &g_h, pid, x, release));
    }
    /* US-101 */
    if (s_eq(cmd, "collab-dispute-open")) {
        rest = word(rest, b, 48);
        if (!(pid = peer_id(a)) || num(b, &x)) { print_string("collab-dispute-open error usage: PAYER SEQ EVIDENCE\n"); return 1; }
        return emitted(cmd, collab_dispute_open(&g_c, &g_h, pid, x, rest));
    }
    if (s_eq(cmd, "collab-dispute-vote")) {
        rest = word(rest, b, 48); rest = word(rest, c, 48);
        if (!(pid = peer_id(a)) || num(b, &x) || (!s_eq(c, "payer") && !s_eq(c, "payee"))) {
            print_string("collab-dispute-vote error usage: PAYER SEQ payer|payee\n");
            return 1;
        }
        return emitted(cmd, collab_dispute_vote(&g_c, &g_h, pid, x, s_eq(c, "payer")));
    }
    /* US-094 auctions */
    if (s_eq(cmd, "collab-auction")) {
        rest = word(rest, b, 48);
        if (!a[0] || num(b, &x)) { print_string("collab-auction error usage: ITEM RESERVE\n"); return 1; }
        return emitted(cmd, collab_auction_open(&g_c, &g_h, a, x));
    }
    if (s_eq(cmd, "collab-market-bid")) {
        rest = word(rest, b, 48); rest = word(rest, c, 48);
        if (!(pid = peer_id(a)) || num(b, &x) || num(c, &y)) { print_string("collab-market-bid error usage: SELLER SEQ AMOUNT\n"); return 1; }
        return emitted(cmd, collab_auction_bid(&g_c, &g_h, pid, x, y));
    }
    if (s_eq(cmd, "collab-auction-close")) {
        if (num(a, &x)) { print_string("collab-auction-close error usage: SEQ\n"); return 1; }
        return emitted(cmd, collab_auction_close(&g_c, &g_h, x));
    }
    if (s_eq(cmd, "collab-ticket-route")) {
        rest = word(rest, b, 48);
        if (num(a, &x)) return emitted(cmd, -1);
        y = peer_id(b);
        if (!y) (void)num(b, &y);
        return emitted(cmd, collab_ticket_route(&g_c, &g_h, x, y));
    }
    if (s_eq(cmd, "collab-ticket-assign")) {
        rest = word(rest, b, 48);
        if (num(a, &x) || !b[0]) return emitted(cmd, -1);
        return emitted(cmd, collab_ticket_assign(&g_c, &g_h, x, b));
    }
    if (s_eq(cmd, "collab-market-price")) {
        uint32_t avg = 0;
        if (!a[0]) { print_string("collab-market-price error usage: ITEM\n"); return 1; }
        collab_market_price(&g_c, a, &avg);
        print_string("market price avg "); put_u(avg); print_string("\n");
        return 0;
    }
    if (s_eq(cmd, "collab-privacy-proof")) {
        uint8_t proof[32];
        rest = word(rest, b, 48);
        if (num(a, &x) || num(b, &y)) { print_string("collab-privacy-proof error usage: VALUE SECRET\n"); return 1; }
        collab_privacy_proof_gen(x, y, proof);
        print_string("collab-privacy-proof commitment ");
        { int i; for (i = 0; i < 8; i++) { char hx[3]; hx[0] = "0123456789abcdef"[proof[i] >> 4]; hx[1] = "0123456789abcdef"[proof[i] & 15]; hx[2] = 0; print_string(hx); } }
        print_string(collab_privacy_proof_verify(x, y, proof) == 0 ? " opened ok" : " open failed");
        print_string(collab_privacy_proof_verify(x + 1U, y, proof) == 0 ? " wrong-value accepted" : " wrong-value rejected");
        print_string(" (hash commitment, not zero-knowledge)\n");
        return 0;
    }
    if (s_eq(cmd, "collab-zk-prove")) {
        /* US-103 (Lot 8): Schnorr NIZK; the secret never leaves this call */
        collab_zk_proof_t pr, bad;
        uint64_t pub;
        int ok, tamper, wrong_msg, wrong_pub;
        rest = word(rest, b, 48);
        if (num(a, &x) || !b[0]) { print_string("collab-zk-prove error usage: SECRET MESSAGE\n"); return 1; }
        if (collab_zk_prove((uint64_t)x * 2654435761ULL + 12345ULL, b, &pr) != 0) { print_string("collab-zk-prove error bad secret\n"); return 1; }
        pub = collab_zk_public((uint64_t)x * 2654435761ULL + 12345ULL);
        ok = collab_zk_verify(pub, b, &pr) == 0;
        bad = pr; bad.s = bad.s ? bad.s - 1U : 1U;
        tamper = collab_zk_verify(pub, b, &bad) == 0;
        wrong_msg = collab_zk_verify(pub, "other", &pr) == 0;
        wrong_pub = collab_zk_verify(collab_zk_public(7U), b, &pr) == 0;
        print_string("collab-zk-prove public ");
        { int i; for (i = 60; i >= 0; i -= 4) { char hx[2]; hx[0] = "0123456789abcdef"[(uint32_t)(pub >> i) & 15U]; hx[1] = 0; print_string(hx); } }
        print_string(ok ? " verify ok" : " verify FAILED");
        print_string(tamper ? " tampered accepted" : " tampered rejected");
        print_string(wrong_msg ? " wrong-message accepted" : " wrong-message rejected");
        print_string(wrong_pub ? " wrong-public accepted" : " wrong-public rejected");
        print_string(" (Schnorr NIZK, 62-bit toy group: simule strength)\n");
        return 0;
    }
    if (s_eq(cmd, "collab-compliance")) {
        collab_compliance_report(&g_c, g_out, (int)sizeof(g_out));
        print_string(g_out);
        return 0;
    }
    {
        static const char* const views[][2] = {
            {"collab-balances", "balances"}, {"collab-audit", "audit"}, {"collab-tasks", "tasks"},
            {"collab-offers", "offers"}, {"collab-votes", "votes"}, {"collab-tickets", "tickets"},
            {"collab-export", "export"}, {"collab-escrows", "escrows"}, {"collab-auctions", "auctions"}, {0, 0}
        };
        int i;
        for (i = 0; views[i][0]; i++)
            if (s_eq(cmd, views[i][0])) {
                collab_report(&g_c, &g_h, views[i][1], 0, g_out, (int)sizeof(g_out));
                print_string(g_out);
                return 0;
            }
    }
    print_string(cmd); print_string(" error unknown collab command\n");
    return 1;
}

/* persistence hooks for shell_persist.c */
int shell_collab_save(uint8_t* out, int cap) { return g_joined ? collab_save(&g_c, out, cap) : 0; }
int shell_collab_load(const uint8_t* in, int len) {
    int n = collab_load(&g_c, in, len);
    if (n < 0) { g_joined = 0; return n; }
    g_joined = 1;
    return n;
}
uint32_t shell_collab_self(void) { return g_joined ? g_c.self : 0; }
/* key at rest: the persisted blob carries the secret only encrypted (or not
 * at all); after a load the shell installs it here, or locks signing */
void shell_collab_key_install(const uint8_t* sk) {
    int i;
    if (!sk) { for (i = 0; i < 20; i++) g_c.sk[i] = 0; g_c.signing = 0; return; }
    for (i = 0; i < 20; i++) g_c.sk[i] = sk[i];
    g_c.signing = 1;
}
int shell_collab_signing(void) { return g_joined && g_c.signing; }
/* US-060: sign with this node's collab key (pm-certify) */
int shell_collab_sign(const uint8_t* m, int len, uint8_t sig[40], uint8_t pk[128]) {
    int i;
    if (!g_joined || !g_c.signing) return -1;
    if (csig_sign(g_c.sk, m, len, sig) != 0) return -1;
    for (i = 0; i < 128; i++) pk[i] = g_c.pk[i];
    return 0;
}
