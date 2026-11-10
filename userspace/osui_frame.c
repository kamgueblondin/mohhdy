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
    if (input) {
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
