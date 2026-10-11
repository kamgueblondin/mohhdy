/* userspace/shell_fleet.c - session-*, sync-*, fleet-*, deploy-stage/
 * promote/rollback/status shell commands over the P2P node (fleet.c). */
#include "fleet.h"
#include "p2p.h"

void print_string(const char* s);
p2p_node_t* shell_p2p_node(void);
const p2p_host_t* shell_p2p_host(void);
int shell_session_export(char* out, int cap);
int shell_session_import(const char* s, int len, int* vars, int* hist);
const char* shell_session_cwd(void);
int shell_file_read(const char* p, char* buf, int cap);
int shell_file_write(const char* p, const char* d, int len);
void shell_prod_summary(char* metric, int mcap, char* log, int lcap);

static fleet_t g_f;
static int g_init;
static char g_buf[FL_PAYLOAD + 64];
static char g_line[200];

static int f_send(void* c, uint32_t to, const uint8_t* d, int len) { (void)c; return p2p_send_app(shell_p2p_node(), shell_p2p_host(), to, d, len); }
static void f_out(void* c, const char* s) { (void)c; print_string(s); print_string("\n"); }
int sys_mkdir(const char* path);
static int f_write(void* c, const char* p, const char* d, int len) {
    (void)c;
    /* deployments land in /app (created on first use) */
    if (p[0] == '/' && p[1] == 'a' && p[2] == 'p' && p[3] == 'p' && p[4] == '/') (void)sys_mkdir("/app");
    return shell_file_write(p, d, len);
}
static const char* f_name(void* c, uint32_t id) { (void)c; return p2p_peer_name(shell_p2p_node(), id); }
static uint32_t f_members(void* c) { (void)c; return p2p_member_count(shell_p2p_node()); }
unsigned int sys_ticks(void);
static uint32_t f_now(void* c) { (void)c; return sys_ticks(); }
static int f_peers(void* c, uint32_t* ids, int max) {
    p2p_node_t* n = shell_p2p_node(); int i, k = 0;
    (void)c;
    for (i = 0; i < P2P_PEERS && k < max; i++) if (n->peers[i].used && n->peers[i].keyed) ids[k++] = n->peers[i].id;
    return k;
}
static fleet_host_t host(void) {
    fleet_host_t h;
    h.ctx = 0; h.self = shell_p2p_node()->id; h.send = f_send; h.out = f_out; h.write_file = f_write; h.name = f_name; h.members = f_members; h.now = f_now; h.peers = f_peers;
    return h;
}
static void init(void) { if (!g_init) { fleet_init(&g_f); g_init = 1; } }
/* background pump (shell_p2p_poll): re-send unacknowledged deploy steps */
void shell_fleet_tick(void) { fleet_host_t h; if (!g_init || !shell_p2p_node()->up) return; h = host(); (void)fleet_tick(&g_f, &h, sys_ticks()); }
void shell_fleet_app(uint32_t from, const uint8_t* d, int len) { fleet_host_t h = host(); init(); fleet_receive(&g_f, &h, from, d, len); }

static int s_eq(const char* a, const char* b) { while (*a && *a == *b) { a++; b++; } return *a == *b; }
static const char* word(const char* s, char* w, int cap) {
    int n = 0;
    while (*s == ' ') s++;
    while (*s && *s != ' ') { if (n < cap - 1) w[n++] = *s; s++; }
    w[n] = 0;
    while (*s == ' ') s++;
    return s;
}
static void cat(char* d, const char* s) { int n = 0; while (d[n]) n++; while (*s && n < (int)sizeof(g_line) - 1) d[n++] = *s++; d[n] = 0; }
static void catu(char* d, uint32_t v) { char t[11], r[12]; int n = 0, i = 0; do { t[n++] = (char)('0' + v % 10U); v /= 10U; } while (v); while (n) r[i++] = t[--n]; r[i] = 0; cat(d, r); }
static void hex(char* d, uint32_t v) { char r[9]; int i; for (i = 7; i >= 0; i--) { r[i] = "0123456789abcdef"[v & 15U]; v >>= 4; } r[8] = 0; cat(d, r); }
static void emit(void) { print_string(g_line); print_string("\n"); }
static uint32_t peer(const char* name) { p2p_peer_t* p = p2p_find(shell_p2p_node(), name); return p ? p->id : 0; }

static const char* const k_cmds[] = {
    "session-handoff", "session-resume", "sync-push", "sync-status", "fleet-report", "fleet-collect",
    "deploy-stage", "deploy-promote", "deploy-rollback", "deploy-status", "mkfile", 0
};
int shell_fleet_is(const char* line) {
    char w[24]; int i;
    word(line, w, (int)sizeof(w));
    for (i = 0; k_cmds[i]; i++) if (s_eq(w, k_cmds[i])) return 1;
    return 0;
}

int shell_fleet_line(const char* line) {
    char cmd[24], a[48], b[48], c[48];
    const char* rest = word(line, cmd, (int)sizeof(cmd));
    fleet_host_t h;
    int n, i;
    if (s_eq(cmd, "mkfile")) {
        /* mkfile PATH BYTES: deterministic multi-block test file
         * ("mk-NNNN-a..z" lines, last 4 bytes "END\n"). */
        static char big[4096];
        int want = 0, k = 0, ln = 0, w;
        rest = word(rest, a, 48); rest = word(rest, b, 48);
        for (i = 0; b[i] >= '0' && b[i] <= '9'; i++) want = want * 10 + (b[i] - '0');
        if (!a[0] || want <= 0 || want > (int)sizeof(big)) { print_string("mkfile error usage: mkfile PATH BYTES (1..4096)\n"); return 1; }
        while (k < want) {
            char t[40]; int j = 0, v = ln++;
            t[j++] = 'm'; t[j++] = 'k'; t[j++] = '-';
            t[j++] = (char)('0' + (v / 1000) % 10); t[j++] = (char)('0' + (v / 100) % 10); t[j++] = (char)('0' + (v / 10) % 10); t[j++] = (char)('0' + v % 10);
            t[j++] = '-';
            for (w = 0; w < 26; w++) t[j++] = (char)('a' + w);
            t[j++] = '\n';
            for (w = 0; w < j && k < want; w++) big[k++] = t[w];
        }
        if (want >= 4) { big[want - 4] = 'E'; big[want - 3] = 'N'; big[want - 2] = 'D'; big[want - 1] = '\n'; }
        n = shell_file_write(a, big, want);
        if (n != want) { print_string("mkfile error write "); print_string(a); print_string("\n"); return 1; }
        g_line[0] = 0; cat(g_line, "mkfile ok "); cat(g_line, a); cat(g_line, " bytes "); catu(g_line, (uint32_t)want); emit();
        return 0;
    }
    if (!shell_p2p_node()->up) { print_string(cmd); print_string(" error p2p down (p2p-up first)\n"); return 1; }
    init(); h = host();
    rest = word(rest, a, 48); rest = word(rest, b, 48); rest = word(rest, c, 48);
    g_line[0] = 0;
    if (s_eq(cmd, "session-handoff")) {
        uint32_t to = peer(a);
        if (!to) { print_string("session-handoff error unknown peer\n"); return 1; }
        n = shell_session_export(g_buf, FL_PAYLOAD);
        if (n < 0 || fleet_session_send(&g_f, &h, to, g_buf, n) != 0) { print_string("session-handoff error send failed\n"); return 1; }
        cat(g_line, "session-handoff ok to "); cat(g_line, a); cat(g_line, " cwd "); cat(g_line, shell_session_cwd());
        cat(g_line, " bytes "); catu(g_line, (uint32_t)n); emit();
        return 0;
    }
    if (s_eq(cmd, "session-resume")) {
        int vars, hist;
        if (!g_f.sess_len) { print_string("session-resume error no session offered\n"); return 1; }
        shell_session_import(g_f.sess, g_f.sess_len, &vars, &hist);
        cat(g_line, "session-resume ok from "); cat(g_line, p2p_peer_name(shell_p2p_node(), g_f.sess_from));
        cat(g_line, " cwd "); cat(g_line, shell_session_cwd()); cat(g_line, " vars "); catu(g_line, (uint32_t)vars);
        cat(g_line, " history "); catu(g_line, (uint32_t)hist); emit();
        g_f.sess_len = 0;
        return 0;
    }
    if (s_eq(cmd, "sync-push")) {
        int v;
        n = shell_file_read(a, g_buf, FL_DATA_MAX + 1);
        if (n > FL_DATA_MAX) { print_string("sync-push error too large (3900 bytes max)\n"); return 1; }
        if (n < 0) { print_string("sync-push error file not found\n"); return 1; }
        v = fleet_sync_push(&g_f, &h, a, g_buf, n);
        if (v < 0) { print_string("sync-push error path too long (39) or send queue full\n"); return 1; }
        cat(g_line, "sync-push ok "); cat(g_line, a); cat(g_line, " v"); catu(g_line, (uint32_t)v);
        cat(g_line, " bytes "); catu(g_line, (uint32_t)n); cat(g_line, " sum "); hex(g_line, fleet_sum(g_buf, n)); emit();
        return 0;
    }
    if (s_eq(cmd, "sync-status")) {
        int k = 0;
        for (i = 0; i < FL_FILES; i++) if (g_f.files[i].used) {
            g_line[0] = 0; cat(g_line, "sync "); cat(g_line, g_f.files[i].path); cat(g_line, " v"); catu(g_line, g_f.files[i].ver);
            cat(g_line, " by "); cat(g_line, p2p_peer_name(shell_p2p_node(), g_f.files[i].origin)); cat(g_line, " sum "); hex(g_line, g_f.files[i].sum); emit(); k++;
        }
        g_line[0] = 0; cat(g_line, "sync-status ok files "); catu(g_line, (uint32_t)k); cat(g_line, " rejected "); catu(g_line, g_f.rejected);
        cat(g_line, " delivered "); catu(g_line, g_f.delivered); cat(g_line, " resent "); catu(g_line, g_f.resent);
        cat(g_line, " failed "); catu(g_line, g_f.failed); cat(g_line, " pending "); catu(g_line, (uint32_t)fleet_pending(&g_f)); emit();
        return 0;
    }
    if (s_eq(cmd, "fleet-report")) {
        static char m[100], l[80];
        uint32_t to = a[0] ? peer(a) : 0;
        if (a[0] && !to) { print_string("fleet-report error unknown peer\n"); return 1; }
        shell_prod_summary(m, (int)sizeof(m), l, (int)sizeof(l));
        if (fleet_report(&g_f, &h, to, m, l) != 0) { print_string("fleet-report error no peer\n"); return 1; }
        cat(g_line, "fleet-report ok "); cat(g_line, m); emit();
        return 0;
    }
    if (s_eq(cmd, "fleet-collect")) {
        int k = 0, j;
        for (i = 0; i < FL_NODES; i++) if (g_f.nodes[i].used) {
            g_line[0] = 0; cat(g_line, "fleet node "); cat(g_line, p2p_peer_name(shell_p2p_node(), g_f.nodes[i].id));
            cat(g_line, " reports "); catu(g_line, g_f.nodes[i].reports); cat(g_line, " "); cat(g_line, g_f.nodes[i].metric); emit();
            for (j = 0; j < g_f.nodes[i].nlog; j++) { g_line[0] = 0; cat(g_line, "fleet node "); cat(g_line, p2p_peer_name(shell_p2p_node(), g_f.nodes[i].id)); cat(g_line, " log "); cat(g_line, g_f.nodes[i].log[j]); emit(); }
            k++;
        }
        g_line[0] = 0; cat(g_line, "fleet-collect ok nodes "); catu(g_line, (uint32_t)k); emit();
        return 0;
    }
    if (s_eq(cmd, "deploy-stage")) {
        uint32_t to = peer(c); int v;
        if (!a[0] || !to) { print_string("deploy-stage error usage: deploy-stage NAME FILE CANARY_PEER\n"); return 1; }
        n = shell_file_read(b, g_buf, FL_DATA_MAX + 1);
        if (n > FL_DATA_MAX) { print_string("deploy-stage error too large (3900 bytes max)\n"); return 1; }
        if (n < 0) { print_string("deploy-stage error file not found\n"); return 1; }
        v = fleet_deploy_stage(&g_f, &h, a, g_buf, n, to);
        if (v < 0) { print_string("deploy-stage error\n"); return 1; }
        cat(g_line, "deploy-stage ok "); cat(g_line, a); cat(g_line, " v"); catu(g_line, (uint32_t)v); cat(g_line, " canary "); cat(g_line, c); emit();
        return 0;
    }
    if (s_eq(cmd, "deploy-promote")) {
        int r = fleet_deploy_promote(&g_f, &h, a);
        if (r == -2) { print_string("deploy-promote refused canary not acknowledged ok\n"); return 1; }
        if (r != 0) { print_string("deploy-promote error\n"); return 1; }
        cat(g_line, "deploy-promote ok "); cat(g_line, a); cat(g_line, " v"); catu(g_line, fleet_deploy_find(&g_f, a)->ver); cat(g_line, " to all"); emit();
        return 0;
    }
    if (s_eq(cmd, "deploy-rollback")) {
        int v = fleet_deploy_rollback(&g_f, &h, a);
        if (v < 0) { print_string("deploy-rollback error no previous version\n"); return 1; }
        cat(g_line, "deploy-rollback ok "); cat(g_line, a); cat(g_line, " v"); catu(g_line, (uint32_t)v); emit();
        return 0;
    }
    if (s_eq(cmd, "deploy-status")) {
        fl_deploy_t* dp = fleet_deploy_find(&g_f, a);
        int k = 0;
        if (!dp) { print_string("deploy-status error unknown deployment\n"); return 1; }
        for (i = 0; i < FL_NODES; i++) if (dp->ack[i].used) {
            g_line[0] = 0; cat(g_line, "deploy "); cat(g_line, a); cat(g_line, " node "); cat(g_line, p2p_peer_name(shell_p2p_node(), dp->ack[i].id));
            cat(g_line, " v"); catu(g_line, dp->ack[i].ver); cat(g_line, dp->ack[i].ok ? " ok" : " failed"); emit(); k++;
        }
        g_line[0] = 0; cat(g_line, "deploy-status ok "); cat(g_line, a); cat(g_line, " v"); catu(g_line, dp->ver);
        cat(g_line, dp->stage == FL_STAGE_CANARY ? " stage canary" : dp->stage == FL_STAGE_ALL ? " stage all" : " stage rollback");
        cat(g_line, " acks "); catu(g_line, (uint32_t)k); emit();
        return 0;
    }
    return 1;
}
