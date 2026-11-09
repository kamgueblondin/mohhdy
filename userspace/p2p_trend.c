/* p2p_trend.c - Lot 87: delay-gradient overuse detector (see p2p_trend.h). */
#include "p2p_trend.h"

static uint64_t udiv64(uint64_t n, uint64_t d) {
    uint64_t q = 0, r = 0;
    int i;
    if (d == 0) return ~(uint64_t)0;
    for (i = 63; i >= 0; i--) {
        r = (r << 1) | ((n >> i) & 1u);
        if (r >= d) { r -= d; q |= (uint64_t)1 << i; }
    }
    return q;
}

int64_t p2p_trend_sdiv64(int64_t n, int64_t d) {
    int neg = 0;
    uint64_t un, ud, q;
    if (d == 0) return 0;
    if (n < 0) { neg = !neg; un = (uint64_t)(-(n + 1)) + 1u; } else un = (uint64_t)n;
    if (d < 0) { neg = !neg; ud = (uint64_t)(-(d + 1)) + 1u; } else ud = (uint64_t)d;
    q = udiv64(un, ud);
    return neg ? -(int64_t)q : (int64_t)q;
}

static void zero(void* p, unsigned n) {
    unsigned char* b = (unsigned char*)p;
    while (n--) *b++ = 0;
}

void p2p_trend_init(p2p_trend_t* t, uint32_t now_ms) {
    if (!t) return;
    zero(t, (unsigned)sizeof(*t));
    t->gamma_milli = P2P_TREND_GAMMA0;
    t->last_update_ms = now_ms;
    t->cap_stamp_ms = now_ms;
    t->ack_win_start = now_ms;
    t->state = P2P_TREND_NORMAL;
}

/* least-squares slope of ys over xs, x 1000 (ys in us, xs in ms) */
static int32_t slope_milli(const p2p_trend_t* t) {
    int64_t sx = 0, sy = 0, num = 0, den = 0, mx, my;
    int i, n = t->nwin;
    if (n < 2) return 0;
    for (i = 0; i < n; i++) { sx += t->xs[i]; sy += t->ys[i]; }
    mx = p2p_trend_sdiv64(sx, n);
    my = p2p_trend_sdiv64(sy, n);
    for (i = 0; i < n; i++) {
        int64_t dx = (int64_t)t->xs[i] - mx, dy = (int64_t)t->ys[i] - my;
        num += dx * dy;
        den += dx * dx;
    }
    if (den == 0) return 0;
    /* ys are us and xs ms: num/den is already us per ms = trend x 1000 */
    {
        int64_t s = p2p_trend_sdiv64(num, den);
        if (s > 2000000) s = 2000000;
        if (s < -2000000) s = -2000000;
        return (int32_t)s;
    }
}

static void update_threshold(p2p_trend_t* t, int32_t m, uint32_t now_ms) {
    int32_t am = m < 0 ? -m : m;
    uint32_t dt = now_ms - t->last_update_ms;
    int64_t k, step;
    t->last_update_ms = now_ms;
    /* a spike far above the threshold (a route change) does not move it */
    if (am > t->gamma_milli + 15000) return;
    if (dt > 100u) dt = 100u;
    /* k in 1/100000 per ms: 0.0087 -> 870, 0.039 -> 3900 */
    k = am < t->gamma_milli ? 3900 : 870;
    step = p2p_trend_sdiv64(k * (int64_t)(am - t->gamma_milli) * (int64_t)dt, 100000);
    t->gamma_milli += (int32_t)step;
    if (t->gamma_milli < P2P_TREND_GAMMA_MIN) t->gamma_milli = P2P_TREND_GAMMA_MIN;
    if (t->gamma_milli > P2P_TREND_GAMMA_MAX) t->gamma_milli = P2P_TREND_GAMMA_MAX;
}

static void detect(p2p_trend_t* t, uint32_t group_dt_ms, uint32_t now_ms) {
    int32_t m = t->modified_milli;
    if (t->deltas < 2) { t->prev_modified = m; return; }
    if (m > t->gamma_milli) {
        t->over_ms += group_dt_ms ? group_dt_ms : 1u;
        t->over_count++;
        if (t->over_ms > P2P_TREND_OVER_MS && t->over_count > 1 && m >= t->prev_modified) {
            if (t->state != P2P_TREND_OVERUSE) t->overuses++;
            t->state = P2P_TREND_OVERUSE;
            t->over_ms = 0;
            t->over_count = 0;
        }
    } else if (m < -t->gamma_milli) {
        t->over_ms = 0;
        t->over_count = 0;
        if (t->state != P2P_TREND_UNDERUSE) t->underuses++;
        t->state = P2P_TREND_UNDERUSE;
    } else {
        t->over_ms = 0;
        t->over_count = 0;
        t->state = P2P_TREND_NORMAL;
    }
    t->prev_modified = m;
    update_threshold(t, m, now_ms);
}

static uint32_t mul_div(uint32_t v, uint32_t num, uint32_t den) {
    uint64_t r = udiv64((uint64_t)v * num, den);
    return r > 0xffffffffu ? 0xffffffffu : (uint32_t)r;
}

static void control(p2p_trend_t* t, uint32_t now_ms) {
    uint32_t dt = now_ms - t->cap_stamp_ms;
    if (t->state == P2P_TREND_OVERUSE) {
        if (t->acked_bps) {
            uint32_t c = mul_div(t->acked_bps, 85u, 100u);
            if (!t->cap_bps || c < t->cap_bps) t->cap_bps = c;
        }
        t->cap_stamp_ms = now_ms;
        return;
    }
    if (t->state == P2P_TREND_UNDERUSE) { t->cap_stamp_ms = now_ms; return; }
    /* NORMAL: +8 % per second, by elapsed time, never past 1.5 x acked */
    if (t->cap_bps && dt >= 50u) {
        uint32_t inc = mul_div(t->cap_bps, 8u * (dt > 1000u ? 1000u : dt), 100000u);
        uint32_t c = t->cap_bps + (inc ? inc : 1u);
        if (t->acked_bps && c > t->acked_bps + t->acked_bps / 2u) c = t->acked_bps + t->acked_bps / 2u;
        if (c > t->cap_bps) t->cap_bps = c;
        t->cap_stamp_ms = now_ms;
    }
}

static void close_group(p2p_trend_t* t) {
    if (t->has_prev) {
        int32_t send_d = (int32_t)(t->g_last_send - t->p_last_send);
        int32_t arr_d = (int32_t)(t->g_last_arrival - t->p_last_arrival);
        int32_t d_us = (arr_d - send_d) * 1000;
        if (arr_d < 0) {
            /* reordered group: dropped, the previous group stays */
            t->reordered++;
            return;
        }
        t->deltas++;
        t->acc_us += d_us;
        /* alpha 0.9 in 1/10 */
        t->smooth_us = (int32_t)p2p_trend_sdiv64((int64_t)t->smooth_us * 9 + t->acc_us, 10);
        t->xs[t->wpos] = t->g_last_arrival - t->first_arrival;
        t->ys[t->wpos] = t->smooth_us;
        t->wpos = (uint8_t)((t->wpos + 1u) % P2P_TREND_WIN);
        if (t->nwin < P2P_TREND_WIN) t->nwin++;
        if (t->nwin >= 2) {
            int64_t m;
            uint32_t n = t->deltas < 60u ? t->deltas : 60u;
            t->trend_milli = slope_milli(t);
            m = (int64_t)n * t->trend_milli * 4;
            if (m > 2000000000) m = 2000000000;
            if (m < -2000000000) m = -2000000000;
            t->modified_milli = (int32_t)m;
        }
        detect(t, (uint32_t)(arr_d > 0 ? arr_d : 0), t->g_last_arrival);
        control(t, t->g_last_arrival);
    } else {
        t->first_arrival = t->g_last_arrival;
    }
    t->has_prev = 1;
    t->p_last_send = t->g_last_send;
    t->p_last_arrival = t->g_last_arrival;
    t->groups++;
}

void p2p_trend_on_packet(p2p_trend_t* t, uint32_t send_ms, uint32_t arrival_ms, uint32_t bytes) {
    if (!t) return;
    t->samples++;
    /* acked rate over windows of at least 500 ms */
    t->ack_bytes += bytes;
    if (arrival_ms - t->ack_win_start >= 500u) {
        t->acked_bps = mul_div(t->ack_bytes, 1000u, arrival_ms - t->ack_win_start);
        t->ack_bytes = 0;
        t->ack_win_start = arrival_ms;
    }
    if (t->has_group && (int32_t)(send_ms - t->g_first_send) < 0) {
        /* sent before the open group started: out of order, ignored */
        t->reordered++;
        return;
    }
    if (t->has_group && send_ms - t->g_first_send <= P2P_TREND_BURST_MS) {
        if ((int32_t)(send_ms - t->g_last_send) >= 0) t->g_last_send = send_ms;
        if ((int32_t)(arrival_ms - t->g_last_arrival) >= 0) t->g_last_arrival = arrival_ms;
        t->g_bytes += bytes;
        return;
    }
    if (t->has_group) close_group(t);
    t->has_group = 1;
    t->g_first_send = send_ms;
    t->g_last_send = send_ms;
    t->g_last_arrival = arrival_ms;
    t->g_bytes = bytes;
}

uint32_t p2p_trend_cap(const p2p_trend_t* t) { return t ? t->cap_bps : 0u; }

uint32_t p2p_trend_limit(const p2p_trend_t* t, uint32_t rate_bps, uint32_t now_ms) {
    uint32_t c = p2p_trend_cap(t);
    if (!c || !rate_bps) return rate_bps;
    if (now_ms - t->last_update_ms > P2P_TREND_STALE_MS) return rate_bps;
    return rate_bps < c ? rate_bps : c;
}

const char* p2p_trend_state_name(p2p_trend_state_t s) {
    switch (s) {
    case P2P_TREND_OVERUSE: return "overuse";
    case P2P_TREND_UNDERUSE: return "underuse";
    default: return "normal";
    }
}

static int put(char* o, int cap, int n, const char* s) {
    while (*s && n < cap - 1) o[n++] = *s++;
    o[n] = 0;
    return n;
}
static int put_i(char* o, int cap, int n, int32_t v) {
    char t[12];
    int i = 0;
    uint32_t u;
    if (v < 0) { n = put(o, cap, n, "-"); u = (uint32_t)(-(v + 1)) + 1u; } else u = (uint32_t)v;
    do { t[i++] = (char)('0' + u % 10u); u /= 10u; } while (u && i < 11);
    while (i > 0 && n < cap - 1) o[n++] = t[--i];
    o[n] = 0;
    return n;
}

void p2p_trend_status(const p2p_trend_t* t, char* out, int cap) {
    int n = 0;
    if (!out || cap < 2) return;
    out[0] = 0;
    if (!t) return;
    n = put(out, cap, n, "p2p trend state ");
    n = put(out, cap, n, p2p_trend_state_name(t->state));
    n = put(out, cap, n, " trend_milli ");
    n = put_i(out, cap, n, t->trend_milli);
    n = put(out, cap, n, " modified ");
    n = put_i(out, cap, n, t->modified_milli);
    n = put(out, cap, n, " gamma ");
    n = put_i(out, cap, n, t->gamma_milli);
    n = put(out, cap, n, " deltas ");
    n = put_i(out, cap, n, (int32_t)t->deltas);
    n = put(out, cap, n, " cap_bps ");
    n = put_i(out, cap, n, (int32_t)(t->cap_bps > 0x7fffffffu ? 0x7fffffffu : t->cap_bps));
    n = put(out, cap, n, " acked_bps ");
    n = put_i(out, cap, n, (int32_t)(t->acked_bps > 0x7fffffffu ? 0x7fffffffu : t->acked_bps));
    n = put(out, cap, n, " overuses ");
    n = put_i(out, cap, n, (int32_t)t->overuses);
    n = put(out, cap, n, " groups ");
    (void)put_i(out, cap, n, (int32_t)t->groups);
}
