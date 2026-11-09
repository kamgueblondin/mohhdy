/* plat_energy.c - Lot 68 (US-081): see plat_energy.h. */
#include "plat_energy.h"
#include "platform.h"

static int e_len(const char* s) { int n = 0; while (s && s[n]) n++; return n; }
static void e_cat(char* d, int cap, const char* s) {
    int n = e_len(d), i = 0;
    while (s && s[i] && n < cap - 1) d[n++] = s[i++];
    if (cap > 0) d[n < cap ? n : cap - 1] = 0;
}
static void e_cat_u(char* d, int cap, uint32_t v) {
    char t[12], o[12];
    int k = 0, j;
    do { t[k++] = (char)('0' + v % 10U); v /= 10U; } while (v && k < 11);
    for (j = 0; j < k; j++) o[j] = t[k - 1 - j];
    o[k] = 0;
    e_cat(d, cap, o);
}

void plat_energy_init(plat_energy_t* e) {
    if (!e) return;
    e->profile = PLAT_PWR_BALANCED;
    e->auto_mode = 0;
    e->pace_cap_bps = 0;
    e->background_ms = 50;
    e->refresh_hz = 30;
    e->poll_budget = 4;
    e->applied = 0;
    e->sim_on = 0;
    e->cap_mwh = 0;
    e->level_uwh = 0;
    e->draw_mw = 0;
    e->sim_ms = 0;
    e->auto_switches = 0;
    e->charging = 0;
    e->idle_ms = 0;
    e->bright_hz = 30;
    e->bright_pace = 0;
    e->dim = 0;
}

int plat_energy_apply(plat_energy_t* e, int profile) {
    plat_power_policy_t p;
    if (!e || plat_power_policy(profile, &p) != 0) return -1;
    e->profile = profile;
    e->background_ms = p.background_ms;
    e->refresh_hz = p.refresh_hz;
    e->poll_budget = p.poll_budget;
    /* saver bounds the mesh data rate; balanced a looser bound */
    e->pace_cap_bps = profile == PLAT_PWR_SAVER ? 64000U : profile == PLAT_PWR_BALANCED ? 1000000U : 0U;
    e->bright_hz = p.refresh_hz;
    e->bright_pace = e->pace_cap_bps;
    e->dim = 0;
    e->idle_ms = 0;
    e->applied++;
    return 0;
}

int plat_energy_idle(plat_energy_t* e, uint32_t idle_ms) {
    plat_power_policy_t p;
    if (!e || plat_power_policy(e->profile, &p) != 0) return -1;
    e->idle_ms = idle_ms;
    if (!e->bright_hz) e->bright_hz = p.refresh_hz;
    if (p.screen_dim_s && idle_ms >= p.screen_dim_s * 1000U) {
        int deep = idle_ms >= p.screen_dim_s * 2000U;
        uint32_t div = deep ? 4U : 2U;
        uint32_t hz = e->bright_hz / div;
        /* Lot 86: a second idle period quarters the refresh. The battery
         * stays simulated. performance never reaches this (screen_dim_s 0). */
        e->dim = deep ? 2 : 1;
        e->refresh_hz = hz < 5U ? 5U : hz;
        /* The mesh cap is part of the same idle policy. performance has
         * no cap (bright_pace 0), so this leaves it alone. A second idle
         * call at the same depth must not cut again: bright_pace is the
         * value apply() stored. */
        if (e->bright_pace) {
            uint32_t cut = e->bright_pace / div;
            e->pace_cap_bps = cut < 8000U ? 8000U : cut;
        }
        return 1;
    }
    e->dim = 0;
    e->refresh_hz = e->bright_hz;
    e->pace_cap_bps = e->bright_pace;
    return 0;
}

/* Model, laptop class: SoC base, + load, + display (panel at refresh). */
uint32_t plat_energy_draw_mw(int profile, uint32_t busy, uint32_t hz) {
    uint32_t base, per_busy;
    if (busy > 100U) busy = 100U;
    if (hz > 240U) hz = 240U;
    switch (profile) {
    case PLAT_PWR_PERFORMANCE: base = 3500U; per_busy = 120U; break;
    case PLAT_PWR_SAVER: base = 1200U; per_busy = 50U; break;
    default: base = 2200U; per_busy = 80U; break;
    }
    return base + per_busy * busy + 1500U + hz * 20U;
}

int plat_energy_sim_start(plat_energy_t* e, uint32_t cap_mwh, uint32_t pct) {
    if (!e || cap_mwh < 1000U || cap_mwh > 200000U || pct > 100U) return -1;
    e->sim_on = 1;
    e->cap_mwh = cap_mwh;
    e->level_uwh = cap_mwh * 10U * pct;   /* cap * 1000 * pct / 100 */
    e->sim_ms = 0;
    e->draw_mw = plat_energy_draw_mw(e->profile, 0, e->refresh_hz);
    return 0;
}

void plat_energy_sim_stop(plat_energy_t* e) {
    if (e) e->sim_on = 0;
}

uint32_t plat_energy_pct(const plat_energy_t* e) {
    if (!e || !e->sim_on || !e->cap_mwh) return 0;
    return e->level_uwh / (e->cap_mwh * 10U);
}

int plat_energy_sim_tick(plat_energy_t* e, uint32_t ms, uint32_t busy) {
    uint32_t used;
    if (!e || !e->sim_on) return 0;
    if (ms > 3600000U) ms = 3600000U;
    e->draw_mw = plat_energy_draw_mw(e->profile, busy, e->refresh_hz);
    /* uWh = mW * ms / 3600 ; split to stay in 32 bits */
    used = (e->draw_mw / 10U) * (ms / 360U) + ((e->draw_mw % 10U) * (ms / 360U)) / 10U;
    if (e->charging) {
        uint32_t add = 3000U * (ms / 360U);   /* 30 W charger, simulated */
        uint32_t full = e->cap_mwh * 1000U;
        e->level_uwh = e->level_uwh + add > full ? full : e->level_uwh + add;
    } else {
        e->level_uwh = used >= e->level_uwh ? 0U : e->level_uwh - used;
    }
    e->sim_ms += ms;
    if (e->auto_mode) {
        int want = plat_energy_pct(e) < 20U && !e->charging ? PLAT_PWR_SAVER : plat_power_auto(busy, !e->charging);
        if (want != e->profile) { (void)plat_energy_apply(e, want); e->auto_switches++; return 1; }
    }
    return 0;
}

uint32_t plat_energy_minutes_left(const plat_energy_t* e) {
    if (!e || !e->sim_on || !e->draw_mw) return 0;
    /* uWh / mW = ms*1000/3600... minutes = uWh * 60 / (mW * 1000) */
    return (e->level_uwh / e->draw_mw) * 60U / 1000U;
}

int plat_energy_report(const plat_energy_t* e, char* out, int cap) {
    if (!out || cap <= 0) return -1;
    out[0] = 0;
    if (!e) return -1;
    e_cat(out, cap, "power-apply profile "); e_cat(out, cap, plat_power_name(e->profile));
    e_cat(out, cap, e->auto_mode ? " auto" : " manual");
    e_cat(out, cap, " pace_cap_bps "); e_cat_u(out, cap, e->pace_cap_bps);
    e_cat(out, cap, " background_ms "); e_cat_u(out, cap, e->background_ms);
    e_cat(out, cap, " refresh_hz "); e_cat_u(out, cap, e->refresh_hz);
    e_cat(out, cap, " poll_budget "); e_cat_u(out, cap, e->poll_budget);
    if (e->sim_on) {
        e_cat(out, cap, " battery simule pct "); e_cat_u(out, cap, plat_energy_pct(e));
        e_cat(out, cap, " draw_mw "); e_cat_u(out, cap, e->draw_mw);
        e_cat(out, cap, " minutes_left "); e_cat_u(out, cap, plat_energy_minutes_left(e));
        e_cat(out, cap, e->charging ? " charging" : " discharging");
        e_cat(out, cap, " switches "); e_cat_u(out, cap, e->auto_switches);
    } else {
        e_cat(out, cap, " battery none (emulated pc, mains)");
    }
    e_cat(out, cap, " dim "); e_cat_u(out, cap, (uint32_t)e->dim);
    e_cat(out, cap, "\n");
    return e_len(out);
}
