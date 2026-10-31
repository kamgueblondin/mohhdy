/* p2p_flow.c - Lot 69 (US-068/US-073): see p2p_flow.h. */
#include "p2p_flow.h"

static int f_len(const char* s) { int n = 0; while (s && s[n]) n++; return n; }
static void f_cat(char* d, int cap, const char* s) {
    int n = f_len(d), i = 0;
    while (s && s[i] && n < cap - 1) d[n++] = s[i++];
    if (cap > 0) d[n < cap ? n : cap - 1] = 0;
}
static void f_cat_u(char* d, int cap, uint32_t v) {
    char t[12], o[12];
    int k = 0, j;
    do { t[k++] = (char)('0' + v % 10U); v /= 10U; } while (v && k < 11);
    for (j = 0; j < k; j++) o[j] = t[k - 1 - j];
    o[k] = 0;
    f_cat(d, cap, o);
}
static void f_zero(void* p, int n) { uint8_t* b = (uint8_t*)p; while (n-- > 0) *b++ = 0; }
static uint32_t get32(const uint8_t* b) { return ((uint32_t)b[0] << 24) | ((uint32_t)b[1] << 16) | ((uint32_t)b[2] << 8) | b[3]; }
static void put32(uint8_t* b, uint32_t v) { b[0] = (uint8_t)(v >> 24); b[1] = (uint8_t)(v >> 16); b[2] = (uint8_t)(v >> 8); b[3] = (uint8_t)v; }

void p2p_flow_init(p2p_flow_t* f) {
    if (!f) return;
    f_zero(f, (int)sizeof(*f));
    p2p_pace_init(&f->pace);
}

uint32_t p2p_flow_loss_timeout_us(const p2p_flow_t* f) {
    uint32_t t = f && f->srtt_us ? f->srtt_us * 4U : 1000000U;
    if (t < P2P_FLOW_LOSS_MIN_US) t = P2P_FLOW_LOSS_MIN_US;
    if (t > P2P_FLOW_LOSS_MAX_US) t = P2P_FLOW_LOSS_MAX_US;
    return t;
}

uint32_t p2p_flow_rate(const p2p_flow_t* f) {
    uint32_t r;
    if (!f || !f->btl_bps) return 0;
    /* 32-bit only: the guest has no 64-bit division */
    r = f->btl_bps / P2P_FLOW_GAIN_DEN * P2P_FLOW_GAIN_NUM + (f->btl_bps % P2P_FLOW_GAIN_DEN) * P2P_FLOW_GAIN_NUM / P2P_FLOW_GAIN_DEN;
    if (f->cap_bps && r > f->cap_bps) r = f->cap_bps;
    if (f->pace.cap_bps && r > f->pace.cap_bps) r = f->pace.cap_bps;
    if (r < P2P_PACE_FLOOR_BPS) r = P2P_PACE_FLOOR_BPS;
    if (r > P2P_PACE_CEIL_BPS) r = P2P_PACE_CEIL_BPS;
    return r;
}

int p2p_flow_admit(p2p_flow_t* f, uint32_t bytes, uint32_t now_us) {
    uint32_t r;
    if (!f) return 1;
    if (f->inflight >= P2P_FLOW_INFLIGHT) { f->window_full++; return 0; }
    r = p2p_flow_rate(f);
    if (!r) {
        if (f->inflight >= P2P_FLOW_INIT_WINDOW) { f->window_full++; return 0; }
        return 1;
    }
    return p2p_pace_admit(&f->pace, r, bytes, now_us);
}

int p2p_flow_on_send(p2p_flow_t* f, uint32_t ctr, uint32_t bytes, uint32_t now_us) {
    uint32_t i, slot = P2P_FLOW_INFLIGHT, oldest = 0;
    if (!f) return -1;
    for (i = 0; i < P2P_FLOW_INFLIGHT; i++) {
        if (!f->pkt[i].used) { slot = i; break; }
        if ((int32_t)(f->pkt[i].sent_us - f->pkt[oldest].sent_us) < 0) oldest = i;
    }
    if (slot == P2P_FLOW_INFLIGHT) {
        /* table full (sender ignored admit): the oldest counts as lost */
        slot = oldest;
        f->lost++;
        f->inflight--;
    }
    f->pkt[slot].used = 1;
    f->pkt[slot].ctr = ctr;
    f->pkt[slot].sent_us = now_us;
    f->pkt[slot].bytes = (uint16_t)(bytes > 0xFFFFU ? 0xFFFFU : bytes);
    f->pkt[slot].delivered_at = f->delivered;
    f->inflight++;
    f->sent++;
    return 0;
}

static void take_sample(p2p_flow_t* f, uint32_t bps) {
    uint32_t i, m = 0;
    f->sample[f->sample_pos] = bps;
    f->sample_pos = (f->sample_pos + 1U) % P2P_FLOW_SAMPLES;
    if (f->nsample < P2P_FLOW_SAMPLES) f->nsample++;
    for (i = 0; i < f->nsample; i++) if (f->sample[i] > m) m = f->sample[i];
    f->btl_bps = m;
    /* a loss cap lifts once the delivery rate shows the path recovered */
    if (f->cap_bps && bps > f->cap_bps) f->cap_bps = 0;
}

int p2p_flow_on_ack(p2p_flow_t* f, uint32_t ctr, uint32_t now_us) {
    uint32_t i;
    if (!f) return -1;
    for (i = 0; i < P2P_FLOW_INFLIGHT; i++) {
        p2p_flow_pkt_t* k = &f->pkt[i];
        uint32_t rtt, el, bytes;
        if (!k->used || k->ctr != ctr) continue;
        rtt = now_us - k->sent_us;
        if (!rtt) rtt = 1;
        f->srtt_us = f->srtt_us ? (f->srtt_us * 7U + rtt) / 8U : rtt;
        if (!f->min_rtt_us || rtt < f->min_rtt_us) f->min_rtt_us = rtt;
        f->delivered += k->bytes;
        bytes = f->delivered - k->delivered_at;
        el = rtt / 1000U;                     /* ms; below 1 ms the clock cannot tell */
        if (!el) el = 1U;
        if (bytes > 4000000U) bytes = 4000000U;
        take_sample(f, bytes * 1000U / el); /* bytes/s, like p2p_pace */
        k->used = 0;
        f->inflight--;
        f->acked++;
        return 0;
    }
    /* acked after the loss timeout, or twice */
    if (!f->sent) { f->unknown_acks++; return -1; }
    f->dup_acks++;
    return 1;
}

int p2p_flow_expire(p2p_flow_t* f, uint32_t now_us) {
    uint32_t i, to;
    int n = 0;
    if (!f) return 0;
    to = p2p_flow_loss_timeout_us(f);
    for (i = 0; i < P2P_FLOW_INFLIGHT; i++) {
        if (!f->pkt[i].used || now_us - f->pkt[i].sent_us < to) continue;
        f->pkt[i].used = 0;
        f->inflight--;
        f->lost++;
        n++;
    }
    if (n) {
        /* back off: the next rate is at most 7/8 of the current one */
        uint32_t r = p2p_flow_rate(f);
        if (r) { f->cap_bps = r * 7U / 8U; if (f->cap_bps < P2P_PACE_FLOOR_BPS) f->cap_bps = P2P_PACE_FLOOR_BPS; }
        f->loss_backoffs++;
    }
    return n;
}

int p2p_flow_note_rx(p2p_flow_t* f, uint32_t ctr) {
    if (!f) return 0;
    if (f->nack_pend < P2P_FLOW_ACK_BATCH) f->ack_pend[f->nack_pend++] = ctr;
    return f->nack_pend >= P2P_FLOW_ACK_BATCH;
}

int p2p_flow_take_acks(p2p_flow_t* f, uint8_t* body, int cap) {
    uint32_t i, n;
    if (!f || !body || cap < 5 || !f->nack_pend) return 0;
    n = f->nack_pend;
    if ((int)(1U + 4U * n) > cap) n = (uint32_t)(cap - 1) / 4U;
    body[0] = (uint8_t)n;
    for (i = 0; i < n; i++) put32(body + 1 + 4 * i, f->ack_pend[i]);
    for (i = n; i < f->nack_pend; i++) f->ack_pend[i - n] = f->ack_pend[i];
    f->nack_pend -= n;
    f->acks_sent += n;
    f->ack_frames++;
    return (int)(1U + 4U * n);
}

int p2p_flow_apply_acks(p2p_flow_t* f, const uint8_t* body, int len, uint32_t now_us) {
    int n, i, applied = 0;
    if (!f || !body || len < 1) return -1;
    n = body[0];
    if (n < 1 || n > (int)P2P_FLOW_ACK_BATCH || len != 1 + 4 * n) return -1;
    for (i = 0; i < n; i++) if (p2p_flow_on_ack(f, get32(body + 1 + 4 * i), now_us) == 0) applied++;
    return applied;
}

void p2p_flow_status(const p2p_flow_t* f, char* out, int cap) {
    if (!out || cap <= 0) return;
    out[0] = 0;
    if (!f) return;
    f_cat(out, cap, "sent "); f_cat_u(out, cap, f->sent);
    f_cat(out, cap, " acked "); f_cat_u(out, cap, f->acked);
    f_cat(out, cap, " lost "); f_cat_u(out, cap, f->lost);
    f_cat(out, cap, " inflight "); f_cat_u(out, cap, f->inflight);
    f_cat(out, cap, " rate_bps "); f_cat_u(out, cap, p2p_flow_rate(f));
    f_cat(out, cap, " btl_bps "); f_cat_u(out, cap, f->btl_bps);
    f_cat(out, cap, " srtt_ms "); f_cat_u(out, cap, f->srtt_us / 1000U);
    f_cat(out, cap, " min_rtt_ms "); f_cat_u(out, cap, f->min_rtt_us / 1000U);
    f_cat(out, cap, " samples "); f_cat_u(out, cap, f->nsample);
    f_cat(out, cap, " paced "); f_cat_u(out, cap, f->pace.admitted);
    f_cat(out, cap, " deferred "); f_cat_u(out, cap, f->pace.deferred + f->window_full);
    f_cat(out, cap, " backoffs "); f_cat_u(out, cap, f->loss_backoffs);
    f_cat(out, cap, " acks_rx_sent "); f_cat_u(out, cap, f->acks_sent);
    f_cat(out, cap, " dack_frames "); f_cat_u(out, cap, f->ack_frames);
}
