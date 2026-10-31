/* p2p_bwe.c - Lot 67 (US-068/US-073): see p2p_bwe.h. */
#include "p2p_bwe.h"

#define GAIN_HIGH 739U   /* 2/ln2 = 2.885 * 256 */
#define GAIN_DRAIN 88U   /* 1/2.885 * 256 */
#define GAIN_UP 320U     /* 1.25 */
#define GAIN_DOWN 192U   /* 0.75 */
#define GAIN_ONE 256U
#define CWND_GAIN 512U   /* 2.0 */

/* 64/32 division by shift and subtract: the guest is freestanding i386
 * without libgcc (__udivdi3). Saturates to 0xffffffff. */
static uint32_t div64(uint64_t n, uint32_t d) {
    uint64_t q = 0, r = 0;
    int i;
    if (!d) return 0xffffffffU;
    if (!(n >> 32)) return (uint32_t)n / d;
    for (i = 63; i >= 0; i--) {
        r = (r << 1) | ((n >> i) & 1U);
        if (r >= d) { r -= d; q |= (uint64_t)1 << i; }
    }
    return q > 0xffffffffULL ? 0xffffffffU : (uint32_t)q;
}

static uint32_t mul_gain(uint32_t v, uint32_t gain) {
    return div64((uint64_t)v * gain, P2P_BWE_GAIN_UNIT);
}

static int after(uint32_t a, uint32_t b) { return (int32_t)(a - b) > 0; }

const char* p2p_bwe_state_name(p2p_bwe_state_t s) {
    switch (s) {
        case P2P_BWE_STARTUP: return "startup";
        case P2P_BWE_DRAIN: return "drain";
        case P2P_BWE_PROBE_BW: return "probe_bw";
        case P2P_BWE_PROBE_RTT: return "probe_rtt";
        default: return "?";
    }
}

uint32_t p2p_bwe_btlbw(const p2p_bwe_t* b) { return b ? b->bw[0].value : 0U; }

uint32_t p2p_bwe_bdp(const p2p_bwe_t* b) {
    if (!b || !b->bw[0].value || !b->min_rtt_ms) return 0U;
    return div64((uint64_t)b->bw[0].value * b->min_rtt_ms, 1000U);
}

uint32_t p2p_bwe_cwnd_pkts(const p2p_bwe_t* b) {
    uint32_t p;
    if (!b || !b->mss) return P2P_BWE_MIN_CWND;
    p = b->cwnd_bytes / b->mss;
    return p < P2P_BWE_MIN_CWND ? P2P_BWE_MIN_CWND : p;
}

/* Windowed max filter (K. Nichols' minmax): best, second, third best over
 * the window, each newer than the one before. */
static void bw_filter_update(p2p_bwe_t* b, uint32_t v, uint32_t round) {
    p2p_bwe_maxent_t s;
    s.value = v; s.round = round;
    if (!b->bw[0].value || v >= b->bw[0].value || round - b->bw[2].round > P2P_BWE_BW_ROUNDS) {
        b->bw[0] = b->bw[1] = b->bw[2] = s;
        return;
    }
    if (v >= b->bw[1].value) b->bw[1] = b->bw[2] = s;
    else if (v >= b->bw[2].value) b->bw[2] = s;
    /* expire */
    if (round - b->bw[0].round > P2P_BWE_BW_ROUNDS) {
        b->bw[0] = b->bw[1];
        b->bw[1] = b->bw[2];
        b->bw[2] = s;
        if (round - b->bw[0].round > P2P_BWE_BW_ROUNDS) {
            b->bw[0] = b->bw[1];
            b->bw[1] = b->bw[2];
        }
    } else if (b->bw[1].round == b->bw[0].round && round - b->bw[1].round > P2P_BWE_BW_ROUNDS / 4U) {
        b->bw[1] = b->bw[2] = s;
    } else if (b->bw[2].round == b->bw[1].round && round - b->bw[2].round > P2P_BWE_BW_ROUNDS / 2U) {
        b->bw[2] = s;
    }
}

static void update_outputs(p2p_bwe_t* b) {
    uint32_t bw = b->bw[0].value ? b->bw[0].value : b->initial_bps;
    uint32_t bdp = p2p_bwe_bdp(b);
    uint32_t min_cwnd = P2P_BWE_MIN_CWND * b->mss;
    b->pacing_bps = mul_gain(bw, b->pacing_gain);
    if (!b->pacing_bps) b->pacing_bps = 1U;
    if (b->state == P2P_BWE_PROBE_RTT) b->cwnd_bytes = min_cwnd;
    else if (!bdp) b->cwnd_bytes = 10U * b->mss;
    else {
        b->cwnd_bytes = mul_gain(bdp, b->cwnd_gain);
        if (b->cwnd_bytes < min_cwnd) b->cwnd_bytes = min_cwnd;
    }
}

void p2p_bwe_init(p2p_bwe_t* b, uint32_t mss, uint32_t initial_bps, uint32_t now_ms) {
    uint32_t i;
    uint8_t* z = (uint8_t*)b;
    if (!b) return;
    for (i = 0; i < sizeof(*b); i++) z[i] = 0;
    b->mss = mss ? mss : 1U;
    b->initial_bps = initial_bps ? initial_bps : 10000U;
    b->delivered_ms = now_ms;
    b->first_sent_ms = now_ms;
    b->min_rtt_stamp = now_ms;
    b->cycle_stamp = now_ms;
    b->state = P2P_BWE_STARTUP;
    b->pacing_gain = GAIN_HIGH;
    b->cwnd_gain = GAIN_HIGH;
    b->pace_next_us = now_ms * 1000U;
    update_outputs(b);
}

int p2p_bwe_on_send(p2p_bwe_t* b, uint32_t seq, uint32_t bytes, uint32_t now_ms) {
    uint32_t i;
    if (!b) return -1;
    if (b->inflight_bytes == 0U) {
        /* restart from idle: the send interval starts now */
        b->first_sent_ms = now_ms;
        b->delivered_ms = now_ms;
    }
    b->sent++;
    for (i = 0; i < P2P_BWE_INFLIGHT; i++) {
        p2p_bwe_pkt_t* p = &b->pkt[i];
        if (p->used) continue;
        p->used = 1U;
        p->seq = seq;
        p->bytes = bytes;
        p->sent_ms = now_ms;
        p->delivered = b->delivered;
        p->delivered_ms = b->delivered_ms;
        p->first_sent_ms = b->first_sent_ms;
        p->app_limited = (uint8_t)(b->app_limited_until != 0U);
        b->inflight_bytes += bytes;
        return 0;
    }
    return -1;
}

void p2p_bwe_on_app_limited(p2p_bwe_t* b) {
    if (!b) return;
    b->app_limited_until = (uint32_t)(b->delivered + b->inflight_bytes);
    if (!b->app_limited_until) b->app_limited_until = 1U;
}

static p2p_bwe_pkt_t* find(p2p_bwe_t* b, uint32_t seq) {
    uint32_t i;
    for (i = 0; i < P2P_BWE_INFLIGHT; i++)
        if (b->pkt[i].used && b->pkt[i].seq == seq) return &b->pkt[i];
    return 0;
}

static void min_rtt_update(p2p_bwe_t* b, uint32_t rtt, uint32_t now_ms) {
    int expired = (now_ms - b->min_rtt_stamp) > P2P_BWE_RTT_WIN_MS;
    if (!rtt) rtt = 1U;
    b->last_rtt_ms = rtt;
    if (!b->min_rtt_ms || rtt <= b->min_rtt_ms || expired) {
        /* as in BBR: a stale minimum takes the current sample, then
         * PROBE_RTT drains the queue to look for a lower one */
        b->min_rtt_ms = rtt;
        b->min_rtt_stamp = now_ms;
        if (expired && b->state != P2P_BWE_PROBE_RTT) {
            b->state = P2P_BWE_PROBE_RTT;
            b->pacing_gain = GAIN_ONE;
            b->cwnd_gain = GAIN_ONE;
            b->probe_rtt_done = 0U;
            b->probe_rtt_round_done = 0U;
        }
    }
}

static void check_full_bw(p2p_bwe_t* b) {
    if (b->full_bw_reached) return;
    if (b->bw[0].value >= mul_gain(b->full_bw, GAIN_UP)) {
        b->full_bw = b->bw[0].value;
        b->full_bw_count = 0U;
        return;
    }
    if (++b->full_bw_count >= 3U) b->full_bw_reached = 1U;
}

static void enter_probe_bw(p2p_bwe_t* b, uint32_t now_ms) {
    b->state = P2P_BWE_PROBE_BW;
    b->cwnd_gain = CWND_GAIN;
    b->cycle_idx = 2U;   /* start in a cruise phase, not with a probe */
    b->cycle_stamp = now_ms;
    b->pacing_gain = GAIN_ONE;
}

static void advance_cycle(p2p_bwe_t* b, uint32_t now_ms) {
    static const uint32_t cycle[8] = {GAIN_UP, GAIN_DOWN, GAIN_ONE, GAIN_ONE, GAIN_ONE, GAIN_ONE, GAIN_ONE, GAIN_ONE};
    uint32_t rtt = b->min_rtt_ms ? b->min_rtt_ms : 100U;
    int next = (now_ms - b->cycle_stamp) > rtt;
    /* leave the 3/4 phase early once the queue is drained */
    if (b->pacing_gain == GAIN_DOWN && b->inflight_bytes <= p2p_bwe_bdp(b)) next = 1;
    /* stay in the 5/4 probe until a round elapsed and inflight grew */
    if (b->pacing_gain == GAIN_UP && b->inflight_bytes < mul_gain(p2p_bwe_bdp(b), GAIN_UP) && !next) next = 0;
    if (!next) return;
    b->cycle_idx = (uint8_t)((b->cycle_idx + 1U) & 7U);
    b->cycle_stamp = now_ms;
    b->pacing_gain = cycle[b->cycle_idx];
}

static void state_step(p2p_bwe_t* b, int round_start, uint32_t now_ms) {
    if (round_start && b->state == P2P_BWE_STARTUP) check_full_bw(b);
    switch (b->state) {
        case P2P_BWE_STARTUP:
            if (b->full_bw_reached) {
                b->state = P2P_BWE_DRAIN;
                b->pacing_gain = GAIN_DRAIN;
                b->cwnd_gain = GAIN_HIGH;
            }
            break;
        case P2P_BWE_DRAIN:
            if (b->inflight_bytes <= p2p_bwe_bdp(b)) enter_probe_bw(b, now_ms);
            break;
        case P2P_BWE_PROBE_BW:
            advance_cycle(b, now_ms);
            break;
        case P2P_BWE_PROBE_RTT:
            if (!b->probe_rtt_done && b->inflight_bytes <= P2P_BWE_MIN_CWND * b->mss) {
                b->probe_rtt_done = now_ms + P2P_BWE_PROBE_RTT_MS;
                if (!b->probe_rtt_done) b->probe_rtt_done = 1U;
                b->probe_rtt_round_done = 0U;
            } else if (b->probe_rtt_done) {
                if (round_start) b->probe_rtt_round_done = 1U;
                if (b->probe_rtt_round_done && after(now_ms, b->probe_rtt_done)) {
                    b->min_rtt_stamp = now_ms;
                    if (b->full_bw_reached) enter_probe_bw(b, now_ms);
                    else { b->state = P2P_BWE_STARTUP; b->pacing_gain = GAIN_HIGH; b->cwnd_gain = GAIN_HIGH; }
                }
            }
            break;
    }
}

uint32_t p2p_bwe_on_ack(p2p_bwe_t* b, uint32_t seq, uint32_t now_ms) {
    p2p_bwe_pkt_t* p;
    uint32_t send_iv, ack_iv, iv, rate = 0U;
    uint64_t dd;
    int round_start = 0;
    if (!b) return 0U;
    p = find(b, seq);
    if (!p) return 0U;
    b->acked++;
    b->delivered += p->bytes;
    b->delivered_ms = now_ms;
    b->inflight_bytes = b->inflight_bytes >= p->bytes ? b->inflight_bytes - p->bytes : 0U;
    if (b->app_limited_until && (uint32_t)b->delivered >= b->app_limited_until) b->app_limited_until = 0U;
    /* round trip counting: a round ends when a packet sent after the
     * previous round end is acked */
    if (p->delivered >= b->next_round_delivered) {
        b->next_round_delivered = b->delivered;
        b->round++;
        round_start = 1;
    }
    min_rtt_update(b, now_ms - p->sent_ms, now_ms);
    /* delivery-rate sample */
    send_iv = p->sent_ms - p->first_sent_ms;
    ack_iv = now_ms - p->delivered_ms;
    iv = send_iv > ack_iv ? send_iv : ack_iv;
    dd = b->delivered - p->delivered;
    b->first_sent_ms = p->sent_ms;
    if (iv && b->min_rtt_ms && iv >= b->min_rtt_ms / 2U) {
        rate = div64(dd * 1000U, iv);
        b->samples++;
        b->last_sample_bps = rate;
        if (p->app_limited) b->samples_app_limited++;
        /* an app-limited sample only counts when it raises the estimate */
        if (!p->app_limited || rate >= b->bw[0].value) bw_filter_update(b, rate, b->round);
    }
    p->used = 0U;
    state_step(b, round_start, now_ms);
    update_outputs(b);
    return rate;
}

void p2p_bwe_on_loss(p2p_bwe_t* b, uint32_t seq, uint32_t now_ms) {
    p2p_bwe_pkt_t* p;
    if (!b) return;
    p = find(b, seq);
    if (!p) return;
    b->losses++;
    b->inflight_bytes = b->inflight_bytes >= p->bytes ? b->inflight_bytes - p->bytes : 0U;
    p->used = 0U;
    (void)now_ms;
    update_outputs(b);
}

void p2p_bwe_on_rtt(p2p_bwe_t* b, uint32_t rtt_ms, uint32_t now_ms) {
    if (!b) return;
    min_rtt_update(b, rtt_ms, now_ms);
    update_outputs(b);
}

int p2p_bwe_can_send(p2p_bwe_t* b, uint32_t bytes, uint32_t now_ms) {
    uint32_t cost_us, now_us = now_ms * 1000U;
    if (!b) return 0;
    if (b->inflight_bytes + bytes > b->cwnd_bytes && b->inflight_bytes > 0U) { b->paced_wait++; return 0; }
    /* times in microseconds so a 3.3 ms spacing is not rounded to 4 */
    if (after(b->pace_next_us, now_us + 999U)) { b->paced_wait++; return 0; }
    cost_us = div64((uint64_t)bytes * 1000000U + b->pacing_bps - 1U, b->pacing_bps);
    if (!cost_us) cost_us = 1U;
    /* idle: no burst credit beyond two datagrams */
    if (after(now_us, b->pace_next_us + 2U * cost_us)) b->pace_next_us = now_us;
    b->pace_next_us += cost_us;
    return 1;
}
