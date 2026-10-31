/* p2p_pace.c - Lot 68 (US-068/US-073): see p2p_pace.h. */
#include "p2p_pace.h"

static int z_len(const char* s) { int n = 0; while (s && s[n]) n++; return n; }
static int z_eq(const char* a, const char* b) {
    int i = 0;
    if (!a || !b) return 0;
    while (a[i] && a[i] == b[i]) i++;
    return a[i] == b[i];
}
static void z_cat(char* d, int cap, const char* s) {
    int n = z_len(d), i = 0;
    while (s && s[i] && n < cap - 1) d[n++] = s[i++];
    if (cap > 0) d[n < cap ? n : cap - 1] = 0;
}
static void z_cat_u(char* d, int cap, uint32_t v) {
    char t[12];
    int k = 0;
    char o[12];
    int j;
    do { t[k++] = (char)('0' + v % 10U); v /= 10U; } while (v && k < 11);
    for (j = 0; j < k; j++) o[j] = t[k - 1 - j];
    o[k] = 0;
    z_cat(d, cap, o);
}
static int z_parse_u(const char* s, uint32_t* out) {
    uint32_t v = 0;
    int i = 0;
    if (!s || !s[0]) return -1;
    for (; s[i]; i++) {
        if (s[i] < '0' || s[i] > '9') return -1;
        if (v > 429496728U) return -1;
        v = v * 10U + (uint32_t)(s[i] - '0');
    }
    *out = v;
    return 0;
}
/* wrap-safe "a is later than b" */
static int later(uint32_t a, uint32_t b) { return (int32_t)(a - b) > 0; }

void p2p_pace_init(p2p_pace_t* p) {
    if (!p) return;
    p->mode = P2P_PACE_AUTO;
    p->source = 0;
    p->cap_bps = 0;
    p->burst_pkts = P2P_PACE_BURST_DEFAULT;
    p->rate_bps = 0;
    p->next_us = 0;
    p->admitted = p->deferred = p->bytes = 0;
    p->last_cost_us = 0;
}

uint32_t p2p_pace_rate(p2p_pace_t* p, const p2p_bwe_t* bwe, int bwe_ready) {
    uint32_t r = 0;
    if (!p) return 0;
    p->source = 0;
    if (p->mode == P2P_PACE_OFF) { p->rate_bps = 0; return 0; }
    if (bwe && bwe_ready && bwe->samples > 0U && bwe->pacing_bps > 0U) { r = bwe->pacing_bps; p->source = 1; }
    else if (p->mode == P2P_PACE_ON) { r = bwe && bwe_ready && bwe->initial_bps ? bwe->initial_bps : 20000U; p->source = 2; }
    if (!r) { p->rate_bps = 0; return 0; }
    if (r < P2P_PACE_FLOOR_BPS) r = P2P_PACE_FLOOR_BPS;
    if (r > P2P_PACE_CEIL_BPS) r = P2P_PACE_CEIL_BPS;
    if (p->cap_bps && r > p->cap_bps) r = p->cap_bps;
    p->rate_bps = r;
    return r;
}

int p2p_pace_admit(p2p_pace_t* p, uint32_t rate, uint32_t bytes, uint32_t now_us) {
    uint32_t cost, credit, b1k, rem;
    if (!p) return 0;
    if (!rate) { p->admitted++; p->bytes += bytes; return 1; }
    if (bytes > 65536U) bytes = 65536U;
    /* 32-bit only (no 64-bit division on the guest): ms part, then the
     * remainder in 10 us steps, rounded up */
    b1k = (bytes + P2P_PACE_OVERHEAD) * 1000U;
    cost = (b1k / rate) * 1000U;
    rem = b1k % rate;
    cost += ((rem * 100U + rate - 1U) / rate) * 10U;
    if (!cost) cost = 1U;
    p->last_cost_us = cost;
    credit = cost * (p->burst_pkts ? p->burst_pkts : 1U);
    /* idle: restart the schedule with at most `burst` datagrams of credit */
    if (p->next_us == 0U || later(now_us - credit + cost, p->next_us)) p->next_us = now_us - credit + cost;
    if (later(p->next_us, now_us)) { p->deferred++; return 0; }
    p->next_us += cost;
    p->admitted++;
    p->bytes += bytes;
    return 1;
}

uint32_t p2p_pace_wait_us(const p2p_pace_t* p, uint32_t now_us) {
    if (!p || !p->last_cost_us) return 0;
    if (!later(p->next_us, now_us)) return 0;
    return p->next_us - now_us;
}

const char* p2p_pace_mode_name(int mode) {
    return mode == P2P_PACE_OFF ? "off" : mode == P2P_PACE_ON ? "on" : "auto";
}

const char* p2p_pace_source_name(int source) {
    return source == 1 ? "estimator" : source == 2 ? "initial" : "window";
}

void p2p_pace_status(const p2p_pace_t* p, char* out, int cap) {
    if (!out || cap <= 0) return;
    out[0] = 0;
    if (!p) return;
    z_cat(out, cap, "p2p pace mode "); z_cat(out, cap, p2p_pace_mode_name(p->mode));
    z_cat(out, cap, " source "); z_cat(out, cap, p2p_pace_source_name(p->source));
    z_cat(out, cap, " rate_bps "); z_cat_u(out, cap, p->rate_bps);
    z_cat(out, cap, " cap_bps "); z_cat_u(out, cap, p->cap_bps);
    z_cat(out, cap, " burst "); z_cat_u(out, cap, p->burst_pkts);
    z_cat(out, cap, " admitted "); z_cat_u(out, cap, p->admitted);
    z_cat(out, cap, " deferred "); z_cat_u(out, cap, p->deferred);
    z_cat(out, cap, " bytes "); z_cat_u(out, cap, p->bytes);
    z_cat(out, cap, "\n");
}

int p2p_pace_command(p2p_pace_t* p, int argc, const char* const* argv, char* out, int cap) {
    uint32_t v;
    if (!p || !out || cap <= 0) return 1;
    out[0] = 0;
    if (argc <= 0 || z_eq(argv[0], "status")) { p2p_pace_status(p, out, cap); return 0; }
    if (argc == 1 && (z_eq(argv[0], "on") || z_eq(argv[0], "off") || z_eq(argv[0], "auto"))) {
        p->mode = (uint8_t)(z_eq(argv[0], "on") ? P2P_PACE_ON : z_eq(argv[0], "off") ? P2P_PACE_OFF : P2P_PACE_AUTO);
        p->next_us = 0;
        p2p_pace_status(p, out, cap);
        return 0;
    }
    if (argc == 2 && z_eq(argv[0], "cap") && z_parse_u(argv[1], &v) == 0 &&
        (v == 0U || (v >= P2P_PACE_FLOOR_BPS && v <= P2P_PACE_CEIL_BPS))) {
        p->cap_bps = v;
        p2p_pace_status(p, out, cap);
        return 0;
    }
    if (argc == 2 && z_eq(argv[0], "burst") && z_parse_u(argv[1], &v) == 0 && v >= 1U && v <= P2P_PACE_BURST_MAX) {
        p->burst_pkts = v;
        p2p_pace_status(p, out, cap);
        return 0;
    }
    z_cat(out, cap, "p2p-pace error usage: p2p-pace [status|on|off|auto|cap BPS (0 or 1000..8000000)|burst N (1..32)]\n");
    return 1;
}
