/* userspace/shell_raft.c - raft-* commands (US-064, userspace/raft.h). */
#include "raft.h"
#include "p2p.h"

void print_string(const char* s);
p2p_node_t* shell_p2p_node(void);
const p2p_host_t* shell_p2p_host(void);
unsigned int sys_ticks(void);

static raft_t g_r;

static int r_send(void* c, uint32_t to, const uint8_t* d, int len) { (void)c; return p2p_send_app(shell_p2p_node(), shell_p2p_host(), to, d, len); }
static void r_out(void* c, const char* s) { (void)c; print_string(s); print_string("\n"); }
static const char* r_name(void* c, uint32_t id) { (void)c; return p2p_peer_name(shell_p2p_node(), id); }
static const raft_host_t g_h = {0, r_send, r_out, r_name};

void shell_raft_app(uint32_t from, const uint8_t* d, int len) { raft_receive(&g_r, &g_h, from, d, len, sys_ticks()); }
void shell_raft_tick(void) { if (g_r.on) raft_tick(&g_r, &g_h, sys_ticks()); }

static int s_eq(const char* a, const char* b) { while (*a && *a == *b) { a++; b++; } return *a == *b; }
static const char* skip(const char* s) { while (*s == ' ') s++; return s; }
static const char* word(const char* s, char* w, int cap) {
    int n = 0; s = skip(s);
    while (*s && *s != ' ') { if (n < cap - 1) w[n++] = *s; s++; }
    w[n] = 0; return skip(s);
}
static void put_u(uint32_t v) { char t[11], r[12]; int n = 0, i = 0; do { t[n++] = (char)('0' + v % 10U); v /= 10U; } while (v); while (n) r[i++] = t[--n]; r[i] = 0; print_string(r); }
static void put_hex(uint32_t v) { char t[9]; int i; for (i = 7; i >= 0; i--) { t[i] = "0123456789abcdef"[v & 15U]; v >>= 4; } t[8] = 0; print_string(t); }

int shell_raft_is(const char* line) { return line[0] == 'r' && line[1] == 'a' && line[2] == 'f' && line[3] == 't' && line[4] == '-'; }
int shell_raft_line(const char* line) {
    char cmd[24], a[24], b[24];
    const char* rest = word(line, cmd, (int)sizeof(cmd));
    p2p_node_t* n = shell_p2p_node();
    if (!p2p_ready(n)) { print_string(cmd); print_string(" error p2p down (p2p-up first)\n"); return 1; }
    if (s_eq(cmd, "raft-start")) {
        /* cluster = this node + every keyed peer; SIZE must match */
        uint32_t ids[RAFT_MAX_NODES]; int i, k = 0, want = 0;
        word(rest, a, (int)sizeof(a));
        for (i = 0; a[i] >= '0' && a[i] <= '9'; i++) want = want * 10 + (a[i] - '0');
        for (i = 0; i < P2P_PEERS && k < RAFT_MAX_NODES; i++) if (n->peers[i].used && n->peers[i].keyed) ids[k++] = n->peers[i].id;
        if (want < 1 || k + 1 != want) { print_string("raft-start error cluster size "); put_u((uint32_t)(k + 1)); print_string(" (need SIZE = keyed peers + 1)\n"); return 1; }
        raft_start(&g_r, n->id, ids, k, sys_ticks());
        print_string("raft-start ok members "); put_u((uint32_t)want); print_string(" majority "); put_u((uint32_t)(want / 2 + 1)); print_string("\n");
        return 0;
    }
    if (!g_r.on) { print_string(cmd); print_string(" error raft-start first\n"); return 1; }
    if (s_eq(cmd, "raft-put")) {
        int rc;
        rest = word(rest, a, (int)sizeof(a)); word(rest, b, (int)sizeof(b));
        rc = raft_propose(&g_r, &g_h, a, b, sys_ticks());
        if (rc == -1) { print_string("raft-put error not leader (leader "); print_string(g_r.leader ? p2p_peer_name(n, g_r.leader) : "unknown"); print_string(")\n"); return 1; }
        if (rc < 0) { print_string("raft-put error bad arguments or log full\n"); return 1; }
        print_string("raft-put ok index "); put_u((uint32_t)rc); print_string(" term "); put_u(g_r.term); print_string(" pending\n");
        return 0;
    }
    if (s_eq(cmd, "raft-get")) {
        const char* v;
        word(rest, a, (int)sizeof(a));
        v = raft_get(&g_r, a);
        print_string("raft-get "); print_string(a); print_string(v ? "=" : " missing"); if (v) print_string(v); print_string("\n");
        return v ? 0 : 1;
    }
    if (s_eq(cmd, "raft-status")) {
        print_string("raft-status role "); print_string(g_r.role == RAFT_LEADER ? "leader" : g_r.role == RAFT_CANDIDATE ? "candidate" : "follower");
        print_string(" term "); put_u(g_r.term);
        print_string(" leader "); print_string(g_r.leader == n->id ? n->name : g_r.leader ? p2p_peer_name(n, g_r.leader) : "none");
        print_string(" log "); put_u(g_r.nlog); print_string(" commit "); put_u(g_r.commit);
        print_string(" digest "); put_hex(raft_digest(&g_r));
        print_string(" elections "); put_u(g_r.elections); print_string(" truncated "); put_u(g_r.truncated); print_string("\n");
        return 0;
    }
    print_string(cmd); print_string(" error unknown raft command\n");
    return 1;
}
