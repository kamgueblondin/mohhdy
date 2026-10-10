/* userspace/shell_p2p.c - guest shell front end of the Phase 5 P2P node.
 * Datagrams go through SYS_PEER_DATA OS_PEER_P2P_* (relayed to the Ring 3
 * networker on the strict kernel). Output goes to the console. */
#include "shell_p2p.h"
#include "p2p.h"
#include "../include/os_syscalls.h"

void print_string(const char* s);
int osui_web_active(void);

static p2p_node_t g_node;
static os_peer_data_request_t g_io;
static p2p_host_t g_host;

static int sys1(int nr, const void* a) { int r; asm volatile("int $0x80" : "=a"(r) : "a"(nr), "b"(a) : "memory"); return r; }
static unsigned ticks(void) { int t; asm volatile("int $0x80" : "=a"(t) : "a"(SYS_TICKS) : "memory"); return (unsigned)t; }
static void pause_tick(void) { (void)sys1(SYS_YIELD, 0); }

static int h_send(void* c, const uint8_t dst[4], const uint8_t* d, uint16_t len) {
    int i;
    (void)c;
    if (len + 8U > OS_PEER_DATA_MAX) return -1;
    for (i = 0; i < 4; i++) { g_io.data[i] = g_node.ip[i]; g_io.data[4 + i] = dst[i]; }
    for (i = 0; i < len; i++) g_io.data[8 + i] = d[i];
    g_io.op = OS_PEER_P2P_SEND; g_io.port = P2P_PORT; g_io.length = (uint16_t)(len + 8U); g_io.attempts = 0;
    return sys1(SYS_PEER_DATA, &g_io) >= 0 ? 0 : -1;
}
static int h_recv(void* c, uint8_t ip[4], uint8_t mac[6], uint8_t* d, uint16_t cap) {
    int i, n;
    (void)c;
    g_io.op = OS_PEER_P2P_RECV; g_io.port = P2P_PORT; g_io.length = 0; g_io.attempts = 8;
    if (sys1(SYS_PEER_DATA, &g_io) != 1 || g_io.length < 10U) return 0;
    n = g_io.length - 10;
    if (n > cap) return 0;
    for (i = 0; i < 4; i++) ip[i] = g_io.data[i];
    for (i = 0; i < 6; i++) mac[i] = g_io.data[4 + i];
    for (i = 0; i < n; i++) d[i] = g_io.data[10 + i];
    return n;
}
static uint32_t h_ticks(void* c) { (void)c; return ticks(); }
static void h_out(void* c, const char* line) { (void)c; print_string(line); print_string("\n"); }

static int s_eq(const char* a, const char* b) { while (*a && *a == *b) { a++; b++; } return *a == *b; }
static const char* skip(const char* s) { while (*s == ' ') s++; return s; }
/* copies the next word into w, returns the rest */
static const char* word(const char* s, char* w, int cap) {
    int n = 0;
    s = skip(s);
    while (*s && *s != ' ') { if (n < cap - 1) w[n++] = *s; s++; }
    w[n] = 0;
    return skip(s);
}
static int parse_ip(const char* s, uint8_t ip[4]) {
    int part = 0, v = -1;
    for (;; s++) {
        if (*s >= '0' && *s <= '9') { v = (v < 0 ? 0 : v) * 10 + (*s - '0'); if (v > 255) return -1; }
        else if ((*s == '.' || !*s) && v >= 0 && part < 4) { ip[part++] = (uint8_t)v; v = -1; if (!*s) break; }
        else return -1;
    }
    return part == 4 ? 0 : -1;
}
static int to_int(const char* s) { int v = 0; while (*s >= '0' && *s <= '9') v = v * 10 + (*s++ - '0'); return v; }
static void say_rc(const char* what, int rc) {
    char n[12]; int i = 0, k; unsigned v = (unsigned)(rc < 0 ? -rc : rc);
    char t[12];
    do { t[i++] = (char)('0' + v % 10U); v /= 10U; } while (v);
    k = 0; if (rc < 0) n[k++] = '-'; while (i) n[k++] = t[--i]; n[k] = 0;
    print_string(what); print_string(n); print_string("\n");
}
static void seed_from_machine(uint8_t seed[32], const char* name) {
    /* Emulated machine: entropy is the TSC and the tick counter mixed with the
     * node name (documented as weak in docs/p2p.md; not a CSPRNG). */
    unsigned lo, hi, i;
    for (i = 0; i < 32U; i += 8U) {
        asm volatile("rdtsc" : "=a"(lo), "=d"(hi));
        lo ^= ticks() * 2654435761U;
        seed[i] = (uint8_t)lo; seed[i + 1] = (uint8_t)(lo >> 8); seed[i + 2] = (uint8_t)(lo >> 16); seed[i + 3] = (uint8_t)(lo >> 24);
        seed[i + 4] = (uint8_t)hi; seed[i + 5] = (uint8_t)(hi >> 8);
        seed[i + 6] = (uint8_t)(name[0] + i); seed[i + 7] = (uint8_t)(hi >> 24);
        pause_tick();
    }
}
static void report(const char* what) {
    static char out[2048];
    p2p_report(&g_node, &g_host, what, out, (int)sizeof(out));
    print_string(out);
}

int shell_p2p_active(void) { return g_node.up; }
void shell_p2p_poll(void) { if (g_node.up) p2p_tick(&g_node, &g_host, 4); }

int shell_p2p_line(const char* line) {
    char cmd[24], a[P2P_TEXT_MAX], b[P2P_TEXT_MAX];
    const char* rest;
    int rc;
    g_host.ctx = 0; g_host.send = h_send; g_host.recv = h_recv; g_host.ticks = h_ticks; g_host.out = h_out;
    rest = word(line, cmd, (int)sizeof(cmd));
    if (s_eq(cmd, "p2p-up")) {
        uint8_t ip[4], seed[32];
        char key[48];
        rest = word(rest, a, (int)sizeof(a));
        rest = word(rest, b, (int)sizeof(b));
        rest = word(rest, key, (int)sizeof(key));
        if (!a[0] || parse_ip(b, ip) != 0) { print_string("p2p-up error usage: p2p-up NAME IP [NETKEY]\n"); return 1; }
        if (osui_web_active()) { print_string("p2p-up error web-serve active\n"); return 1; }
        if (!key[0]) { int i; const char* d = "mohhdy-p2p-lab"; for (i = 0; d[i]; i++) key[i] = d[i]; key[i] = 0; }
        seed_from_machine(seed, a);
        rc = p2p_up(&g_node, a, ip, key, seed, &g_host);
        if (rc != 0) { print_string("p2p-up error bad arguments\n"); return 1; }
        print_string("p2p-up ok name "); print_string(a); print_string(" ip "); print_string(b);
        print_string(" port 7700 crypto x25519+aes128gcm\n");
        return 0;
    }
    if (!g_node.up) { print_string(cmd); print_string(" error p2p down (p2p-up first)\n"); return 1; }
    if (s_eq(cmd, "p2p-down")) { p2p_down(&g_node); print_string("p2p-down ok\n"); return 0; }
    if (s_eq(cmd, "p2p-peers")) { report("peers"); return 0; }
    if (s_eq(cmd, "p2p-health")) { report("health"); return 0; }
    if (s_eq(cmd, "p2p-stats")) { report("stats"); return 0; }
    if (s_eq(cmd, "p2p-kv")) { report("kv"); return 0; }
    if (s_eq(cmd, "p2p-poll")) {
        unsigned until;
        rest = word(rest, a, (int)sizeof(a));
        until = ticks() + (unsigned)(a[0] ? to_int(a) : 1) * P2P_HZ;
        while ((int)(until - ticks()) > 0) { p2p_tick(&g_node, &g_host, 8); pause_tick(); }
        print_string("p2p-poll ok\n");
        return 0;
    }
    if (s_eq(cmd, "p2p-send")) {
        rest = word(rest, a, (int)sizeof(a));
        if (!a[0] || !*rest) { print_string("p2p-send error usage: p2p-send PEER TEXT\n"); return 1; }
        rc = p2p_send_text(&g_node, &g_host, a, rest);
        if (rc == 0) { print_string("p2p-send ok to "); print_string(a); print_string("\n"); return 0; }
        print_string(rc == -2 ? "p2p-send error throttled\n" : rc == -3 ? "p2p-send error no route\n" : "p2p-send error unknown peer\n");
        return 1;
    }
    if (s_eq(cmd, "p2p-put") || s_eq(cmd, "p2p-propose")) {
        rest = word(rest, a, (int)sizeof(a));
        rest = word(rest, b, (int)sizeof(b));
        if (!a[0] || !b[0]) { print_string(cmd); print_string(" error usage: KEY VALUE\n"); return 1; }
        rc = s_eq(cmd, "p2p-put") ? p2p_put(&g_node, &g_host, a, b) : p2p_propose(&g_node, &g_host, a, b);
        if (rc < 0) { print_string(cmd); print_string(" error rejected\n"); return 1; }
        print_string(cmd); print_string(" ok "); print_string(a); say_rc(" sent ", rc);
        return 0;
    }
    if (s_eq(cmd, "p2p-get")) {
        const p2p_item_t* it;
        rest = word(rest, a, (int)sizeof(a));
        it = p2p_get_local(&g_node, a);
        if (it) { print_string("p2p-get ok local "); print_string(a); print_string("="); print_string(it->value); say_rc(" v", (int)it->version); return 0; }
        rc = p2p_get_remote(&g_node, &g_host, a);
        say_rc("p2p-get pending asked ", rc);
        return 0;
    }
    if (s_eq(cmd, "p2p-sync")) {
        rest = word(rest, a, (int)sizeof(a));
        rc = p2p_sync(&g_node, &g_host, a);
        say_rc("p2p-sync ok asked ", rc);
        return 0;
    }
    if (s_eq(cmd, "p2p-block") || s_eq(cmd, "p2p-unblock")) {
        rest = word(rest, a, (int)sizeof(a));
        if (s_eq(cmd, "p2p-unblock") && g_node.up) {
            /* Frames sent while the link was cut may still sit in the
             * networker receive queue (the shell was busy reading keys).
             * Drain them while the peer is still blocked so they are
             * dropped as on a real cut link, not applied after healing. */
            int k;
            for (k = 0; k < 16; k++) p2p_tick(&g_node, &g_host, 8);
        }
        if (p2p_block(&g_node, a, s_eq(cmd, "p2p-block")) != 0) { print_string(cmd); print_string(" error unknown peer\n"); return 1; }
        print_string(cmd); print_string(" ok "); print_string(a); print_string("\n");
        return 0;
    }
    if (s_eq(cmd, "p2p-limit")) {
        rest = word(rest, a, (int)sizeof(a));
        g_node.rate_limit = (uint32_t)to_int(a);
        say_rc("p2p-limit ok per_second ", (int)g_node.rate_limit);
        return 0;
    }
    print_string(cmd); print_string(" error unknown p2p command\n");
    return 1;
}
