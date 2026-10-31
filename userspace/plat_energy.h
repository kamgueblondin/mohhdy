/* plat_energy.h - Lot 68 (US-081): power profiles that change real
 * settings, and a battery that is SIMULATED (the emulated PC has no
 * battery and no ACPI battery driver here).
 *  - plat_energy_apply(): the profile's policy (platform.c) becomes the
 *    values other parts read: P2P data pacer cap (saver bounds the mesh
 *    send rate), background service period, GUI refresh budget.
 *  - simulated battery: capacity in mWh, a draw model in mW from the
 *    profile, the busy percentage and the screen refresh; drains with the
 *    ticks given by the caller, estimates the remaining minutes, and in
 *    auto mode switches to saver under 20 percent. Every report line says
 *    "simule". Integer code, host testable, no heap. */
#ifndef PLAT_ENERGY_H
#define PLAT_ENERGY_H
#include <stdint.h>

typedef struct {
    int profile;              /* PLAT_PWR_* (platform.h) */
    int auto_mode;
    /* applied values */
    uint32_t pace_cap_bps;    /* 0 = no cap */
    uint32_t background_ms;
    uint32_t refresh_hz;
    uint32_t poll_budget;
    uint32_t applied;         /* times a profile was applied */
    /* simulated battery */
    int sim_on;
    uint32_t cap_mwh;         /* full capacity */
    uint32_t level_uwh;       /* remaining, micro watt hours */
    uint32_t draw_mw;         /* last modelled draw */
    uint32_t sim_ms;          /* simulated time elapsed */
    uint32_t auto_switches;
    int charging;
} plat_energy_t;

void plat_energy_init(plat_energy_t* e);
/* apply a profile (PLAT_PWR_*); returns 0 or -1 */
int plat_energy_apply(plat_energy_t* e, int profile);
/* modelled draw in mW for a profile, busy percent and refresh rate */
uint32_t plat_energy_draw_mw(int profile, uint32_t busy_percent, uint32_t refresh_hz);
/* start the simulated battery at `pct` percent of `cap_mwh` */
int plat_energy_sim_start(plat_energy_t* e, uint32_t cap_mwh, uint32_t pct);
void plat_energy_sim_stop(plat_energy_t* e);
/* advance the simulation by `ms` at `busy_percent`; auto mode may switch
 * the profile (returns 1 when it did) */
int plat_energy_sim_tick(plat_energy_t* e, uint32_t ms, uint32_t busy_percent);
uint32_t plat_energy_pct(const plat_energy_t* e);
/* minutes left at the last draw (0 when no simulation) */
uint32_t plat_energy_minutes_left(const plat_energy_t* e);
int plat_energy_report(const plat_energy_t* e, char* out, int cap);
#endif
