/* osui_frame.c - Lot 88 (US-081): see osui_frame.h. */
#include "osui_frame.h"

static int fr_len(const char *s) { int n = 0; while (s && s[n]) n++; return n; }
static void fr_cat(char *d, int cap, const char *s) {
    int n = fr_len(d), i = 0;
    while (s && s[i] && n < cap - 1) d[n++] = s[i++];
    if (cap > 0) d[n < cap ? n : cap - 1] = 0;
}
static void fr_cat_u(char *d, int cap, uint32_t v) {
    char t[12], o[12];
    int k = 0, j;
    do { t[k++] = (char)('0' + v % 10U); v /= 10U; } while (v && k < 11);
    for (j = 0; j < k; j++) o[j] = t[k - 1 - j];
    o[k] = 0;
    fr_cat(d, cap, o);
}

static void fr_phase_reset(osui_frame_t *f) {
    int i;
    for (i = 0; i < OSUI_FRAME_LEVELS; i++) {
        f->wall_ms[i] = 0;
        f->paint_ms[i] = 0;
        f->paint_frames[i] = 0;
    }
}

static int fr_level(int dim) {
    if (dim < 0) return 0;
    return dim >= OSUI_FRAME_LEVELS ? OSUI_FRAME_LEVELS - 1 : dim;
}

/* saturating add: a counter stops at 0xFFFFFFFF instead of wrapping */
static uint32_t fr_add(uint32_t a, uint32_t b) {
    return a > 0xFFFFFFFFU - b ? 0xFFFFFFFFU : a + b;
}

void osui_frame_init(osui_frame_t *f, uint32_t now_ms) {
    if (!f) return;
    f->started = 1;
    f->last_draw_ms = now_ms;
    f->last_input_ms = now_ms;
    f->hz = 0;
    f->dim = 0;
    f->drawn = 0;
    f->skipped = 0;
    f->input_frames = 0;
    f->idle_drawn = 0;
    f->idle_skipped = 0;
    f->wakes = 0;
    f->last_call_ms = now_ms;
    f->cost_ms = 0;
    f->cost_frames = 0;
    fr_phase_reset(f);
}

uint32_t osui_frame_idle_ms(const osui_frame_t *f, uint32_t now_ms) {
    if (!f || !f->started) return 0;
    return now_ms - f->last_input_ms;
}

int osui_frame_due(osui_frame_t *f, uint32_t now_ms, int input, uint32_t hz, int dim) {
    uint32_t period;
    if (!f) return 1;
    if (!f->started) osui_frame_init(f, now_ms);
    f->hz = hz;
    /* the time since the previous call was spent at the previous level */
    f->wall_ms[fr_level(f->dim)] = fr_add(f->wall_ms[fr_level(f->dim)], now_ms - f->last_call_ms);
    f->last_call_ms = now_ms;
    if (input) {
        fr_phase_reset(f);
        if (f->dim) f->wakes++;
        f->dim = dim;
        f->last_input_ms = now_ms;
        f->last_draw_ms = now_ms;
        f->idle_drawn = 0;
        f->idle_skipped = 0;
        f->drawn++;
        f->input_frames++;
        return 1;
    }
    f->dim = dim;
    if (hz) {
        /* a budget above the loop rate never skips; 1000 Hz at most */
        period = 1000U / (hz > 1000U ? 1000U : hz);
        if (now_ms - f->last_draw_ms < period) {
            f->skipped++;
            f->idle_skipped++;
            return 0;
        }
    }
    f->last_draw_ms = now_ms;
    f->drawn++;
    f->idle_drawn++;
    return 1;
}

uint32_t osui_frame_idle_fps10(const osui_frame_t *f, uint32_t now_ms) {
    uint32_t ms;
    if (!f || !f->started) return 0;
    ms = now_ms - f->last_input_ms;
    if (ms < 1000U) return 0;
    /* frames * 100 / (ms / 100): no 64-bit division in the guest; the
     * product stays in 32 bits below 40 million idle frames */
    if (f->idle_drawn >= 40000000U) return 0;
    return f->idle_drawn * 100U / (ms / 100U);
}

int osui_frame_report(const osui_frame_t *f, char *out, int cap) {
    if (!out || cap <= 0) return 0;
    out[0] = 0;
    if (!f) return 0;
    fr_cat(out, cap, "frames drawn ");
    fr_cat_u(out, cap, f->drawn);
    fr_cat(out, cap, " skipped ");
    fr_cat_u(out, cap, f->skipped);
    fr_cat(out, cap, " input ");
    fr_cat_u(out, cap, f->input_frames);
    fr_cat(out, cap, " idle_drawn ");
    fr_cat_u(out, cap, f->idle_drawn);
    fr_cat(out, cap, " idle_skipped ");
    fr_cat_u(out, cap, f->idle_skipped);
    return fr_len(out);
}

void osui_frame_cost(osui_frame_t *f, uint32_t start_ms, uint32_t end_ms, int idle) {
    uint32_t ms;
    int lv;
    if (!f || !f->started) return;
    /* unsigned: a tick wrap between the two reads is still a difference;
     * a difference above a minute is a clock that went back, counted 0 */
    ms = end_ms - start_ms;
    if (ms > 60000U) ms = 0;
    /* keep the running average bounded: halve both before an overflow */
    if (f->cost_frames >= 0x100000U || f->cost_ms > 0xFFFFFFFFU - ms) {
        f->cost_frames /= 2U;
        f->cost_ms /= 2U;
    }
    f->cost_ms += ms;
    f->cost_frames++;
    if (!idle) return;
    lv = fr_level(f->dim);
    f->paint_ms[lv] = fr_add(f->paint_ms[lv], ms);
    f->paint_frames[lv] = fr_add(f->paint_frames[lv], 1U);
}

uint32_t osui_frame_cost_us(const osui_frame_t *f) {
    uint32_t ms;
    if (!f || !f->cost_frames) return 0;
    ms = f->cost_ms;
    /* ms * 1000 stays in 32 bits below 4294967 ms (71 min of painting);
     * beyond, divide first and lose the sub-millisecond part */
    if (ms < 4294967U) return ms * 1000U / f->cost_frames;
    return ms / f->cost_frames * 1000U;
}

uint32_t osui_frame_saved_ms(const osui_frame_t *f) {
    uint32_t us, n;
    if (!f) return 0;
    us = osui_frame_cost_us(f);
    n = f->idle_skipped;
    if (!us || !n) return 0;
    if (n > 0xFFFFFFFFU / us) return 0xFFFFFFFFU / 1000U;
    return n * us / 1000U;
}

uint32_t osui_frame_busy_permille(const osui_frame_t *f, int level) {
    uint32_t p, w;
    if (!f || level < 0 || level >= OSUI_FRAME_LEVELS) return 0;
    p = f->paint_ms[level];
    w = f->wall_ms[level];
    if (!w) return 0;
    /* a paint charged after the last call may make p exceed w by one frame */
    if (p >= w) return 1000U;
    while (p >= 4294967U) { p /= 2U; w /= 2U; }
    return w ? p * 1000U / w : 0;
}

int osui_frame_cost_report(const osui_frame_t *f, char *out, int cap) {
    int i;
    if (!out || cap <= 0) return 0;
    out[0] = 0;
    if (!f) return 0;
    fr_cat(out, cap, "paint_ms");
    for (i = 0; i < OSUI_FRAME_LEVELS; i++) { fr_cat(out, cap, " "); fr_cat_u(out, cap, f->paint_ms[i]); }
    fr_cat(out, cap, " wall_ms");
    for (i = 0; i < OSUI_FRAME_LEVELS; i++) { fr_cat(out, cap, " "); fr_cat_u(out, cap, f->wall_ms[i]); }
    fr_cat(out, cap, " busy_permille");
    for (i = 0; i < OSUI_FRAME_LEVELS; i++) { fr_cat(out, cap, " "); fr_cat_u(out, cap, osui_frame_busy_permille(f, i)); }
    fr_cat(out, cap, " cost_us ");
    fr_cat_u(out, cap, osui_frame_cost_us(f));
    fr_cat(out, cap, " saved_ms ");
    fr_cat_u(out, cap, osui_frame_saved_ms(f));
    return fr_len(out);
}
