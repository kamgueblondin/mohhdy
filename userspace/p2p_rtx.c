/* p2p_rtx.c - Lot 71 (US-068/US-073): see p2p_rtx.h. */
#include "p2p_rtx.h"

static void put32(uint8_t* b, uint32_t v) {
    b[0] = (uint8_t)(v >> 24); b[1] = (uint8_t)(v >> 16); b[2] = (uint8_t)(v >> 8); b[3] = (uint8_t)v;
}
static uint32_t get32(const uint8_t* b) {
    return ((uint32_t)b[0] << 24) | ((uint32_t)b[1] << 16) | ((uint32_t)b[2] << 8) | (uint32_t)b[3];
}

void p2p_rtx_init(p2p_rtx_t* r) {
    uint8_t* p = (uint8_t*)r;
    uint32_t i;
    if (!r) return;
    for (i = 0; i < (uint32_t)sizeof(*r); i++) p[i] = 0;
}

static int find(const p2p_rtx_t* r, uint32_t ctr) {
    uint32_t i;
    for (i = 0; i < P2P_RTX_SLOTS; i++) if (r->s[i].used && r->s[i].ctr == ctr) return (int)i;
    return -1;
}

int p2p_rtx_keep(p2p_rtx_t* r, uint32_t ctr, uint8_t inner, const uint8_t* body, int len) {
    uint32_t i, best = 0, best_age = 0xffffffffU;
    p2p_rtx_slot_t* s;
    int k;
    if (!r || len < 0 || (len > 0 && !body)) return -1;
    if ((uint32_t)len > P2P_RTX_BODY) { r->too_big++; return -1; }
    for (i = 0; i < P2P_RTX_SLOTS; i++) {
        if (!r->s[i].used) { best = i; best_age = 0; break; }
        if (r->s[i].age < best_age) { best = i; best_age = r->s[i].age; }
    }
    s = &r->s[best];
    if (s->used) r->evicted++;
    s->used = 1;
    s->ctr = ctr;
    s->orig = ctr;
    s->inner = inner;
    s->tries = 0;
    s->len = (uint16_t)len;
    s->age = ++r->clock;
    for (k = 0; k < len; k++) s->body[k] = body[k];
    r->kept++;
    return 0;
}

int p2p_rtx_acked(p2p_rtx_t* r, uint32_t ctr) {
    int i;
    if (!r || (i = find(r, ctr)) < 0) return 0;
    r->s[i].used = 0;
    r->acked++;
    return 1;
}

int p2p_rtx_lost(p2p_rtx_t* r, uint32_t ctr) {
    int i;
    if (!r || (i = find(r, ctr)) < 0) return -1;
    if (r->s[i].tries >= P2P_RTX_TRIES) {
        r->s[i].used = 0;
        r->gave_up++;
        return -1;
    }
    return i;
}

int p2p_rtx_frame(const p2p_rtx_t* r, int idx, uint8_t* out, int cap) {
    const p2p_rtx_slot_t* s;
    int k;
    if (!r || !out || idx < 0 || idx >= (int)P2P_RTX_SLOTS) return -1;
    s = &r->s[idx];
    if (!s->used || cap < (int)P2P_RTX_HDR + (int)s->len) return -1;
    put32(out, s->orig);
    out[4] = s->inner;
    for (k = 0; k < (int)s->len; k++) out[P2P_RTX_HDR + k] = s->body[k];
    return (int)P2P_RTX_HDR + (int)s->len;
}

void p2p_rtx_resent(p2p_rtx_t* r, int idx, uint32_t ctr) {
    if (!r || idx < 0 || idx >= (int)P2P_RTX_SLOTS || !r->s[idx].used) return;
    r->s[idx].ctr = ctr;
    r->s[idx].tries++;
    r->resent++;
}

static int bit_get(const p2p_rtx_t* r, uint32_t d) { return (int)((r->seen_mask[d / 32U] >> (d % 32U)) & 1U); }
static void bit_set(p2p_rtx_t* r, uint32_t d) { r->seen_mask[d / 32U] |= 1U << (d % 32U); }
/* moves the window up by d counters */
static void shift_up(p2p_rtx_t* r, uint32_t d) {
    int i;
    uint32_t w = d / 32U, b = d % 32U;
    if (d >= P2P_RTX_WINDOW) { for (i = 0; i < (int)P2P_RTX_WORDS; i++) r->seen_mask[i] = 0; return; }
    for (i = (int)P2P_RTX_WORDS - 1; i >= 0; i--) {
        uint32_t v = 0;
        int src = i - (int)w;
        if (src >= 0) {
            v = r->seen_mask[src] << b;
            if (b && src - 1 >= 0) v |= r->seen_mask[src - 1] >> (32U - b);
        }
        r->seen_mask[i] = v;
    }
}

int p2p_rtx_accept(p2p_rtx_t* r, uint32_t orig) {
    uint32_t d;
    if (!r || !orig) return 0;
    if (!r->seen_hi || orig > r->seen_hi) {
        shift_up(r, r->seen_hi ? orig - r->seen_hi : P2P_RTX_WINDOW);
        bit_set(r, 0);
        r->seen_hi = orig;
        r->delivered++;
        return 1;
    }
    d = r->seen_hi - orig;
    if (d >= P2P_RTX_WINDOW) { r->old_rx++; return 0; }
    if (bit_get(r, d)) { r->dup_rx++; return 0; }
    bit_set(r, d);
    r->delivered++;
    return 1;
}

int p2p_rtx_parse(const uint8_t* m, int len, uint32_t* orig, uint8_t* inner, const uint8_t** body, int* blen) {
    if (!m || len < (int)P2P_RTX_HDR || !orig || !inner || !body || !blen) return -1;
    *orig = get32(m);
    *inner = m[4];
    if (!*orig) return -1;
    *body = m + P2P_RTX_HDR;
    *blen = len - (int)P2P_RTX_HDR;
    return 0;
}

static void cat(char* d, int cap, const char* s) {
    int i = 0;
    while (i < cap - 1 && d[i]) i++;
    while (i < cap - 1 && *s) d[i++] = *s++;
    if (cap > 0) d[i] = 0;
}
static void catu(char* d, int cap, const char* label, uint32_t v) {
    char t[12];
    int n = 0, i = 0;
    cat(d, cap, label);
    do { t[n++] = (char)('0' + v % 10U); v /= 10U; } while (v && n < 11);
    while (i < cap - 1 && d[i]) i++;
    while (n && i < cap - 1) d[i++] = t[--n];
    if (cap > 0) d[i] = 0;
}

void p2p_rtx_status(const p2p_rtx_t* r, char* out, int cap) {
    uint32_t i, pend = 0;
    if (!out || cap <= 0) return;
    out[0] = 0;
    if (!r) return;
    for (i = 0; i < P2P_RTX_SLOTS; i++) pend += r->s[i].used ? 1U : 0U;
    catu(out, cap, "kept ", r->kept);
    catu(out, cap, " acked ", r->acked);
    catu(out, cap, " resent ", r->resent);
    catu(out, cap, " gave_up ", r->gave_up);
    catu(out, cap, " evicted ", r->evicted);
    catu(out, cap, " too_big ", r->too_big);
    catu(out, cap, " pending ", pend);
    catu(out, cap, " delivered ", r->delivered);
    catu(out, cap, " dup_rx ", r->dup_rx);
    catu(out, cap, " old_rx ", r->old_rx);
}
