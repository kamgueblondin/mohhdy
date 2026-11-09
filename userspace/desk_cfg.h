/* desk_cfg.h - Lot 86: the settings the console and the desktop share.
 *
 * ui-scale (quarters, include/os_ui_scale.h), whether the guest may open
 * a network fetch, and which search engine the shell uses. The battery
 * and a real NIC power rail are not here: net off only tells wf_fetch to
 * refuse before a socket is opened. The engine name is one of the built-in
 * names; a custom URL template stays on search-engine and is not cycled.
 * Caller-owned, no heap. */
#ifndef DESK_CFG_H
#define DESK_CFG_H
#include <stdint.h>

#define DESK_ENGINES 6
#define DESK_ENGINE_LEN 16

typedef struct {
    int scale_q;                /* OS_UI_Q_MIN .. OS_UI_Q_MAX */
    int net_on;                 /* 1 fetches allowed, 0 refused */
    int engine;                 /* index into desk_cfg_engine_name */
    uint32_t changes;
} desk_cfg_t;

void desk_cfg_init(desk_cfg_t *c);
const char *desk_cfg_engine_name(int index);
int desk_cfg_engine_count(void);

/* 0 ok, -1 rejected. scale accepts a quarter or the words 1, 1.5, 2, 3, 4. */
int desk_cfg_set_scale(desk_cfg_t *c, int q);
int desk_cfg_set_scale_word(desk_cfg_t *c, const char *word);
int desk_cfg_step_scale(desk_cfg_t *c, int dir);
int desk_cfg_set_net(desk_cfg_t *c, int on);
int desk_cfg_set_engine(desk_cfg_t *c, const char *name);
int desk_cfg_cycle_engine(desk_cfg_t *c);

/* Toolbar of /settings, after Refresh (0) and before Close:
 *  1 scale up, 2 scale down, 3 net toggle, 4 next engine.
 * Returns 1 when the store changed, 0 when the button is not one of those. */
int desk_cfg_button(desk_cfg_t *c, int button);

/* "settings scale N\nsettings net on|off\nsettings engine NAME\n" */
int desk_cfg_save(const desk_cfg_t *c, char *out, int cap);
/* Unknown lines are skipped. A bad value on a known line fails the load
 * and leaves *c unchanged. 0 ok, -1 bad. */
int desk_cfg_load(desk_cfg_t *c, const char *text, int len);

/* Multi-line report for the settings window and the console. */
int desk_cfg_report(const desk_cfg_t *c, char *out, int cap);

/* Chrome text scale for a window slot. Title bars stop at OS_UI_Q_CHROME_MAX
 * even when the slot zoom is higher, so a button label stays inside the
 * bar. Returns the quarters actually used to draw that chrome. */
int desk_cfg_chrome_q(int slot_q);

/* 0 when every built-in window slot, at every scale step, keeps its
 * chrome text at or under the title-bar cap and its body text at the
 * slot scale. The slots are web, chat, shell, pane, ide and the app
 * windows (OS_UI_Z_*). */
int desk_cfg_chrome_audit(char *out, int cap);
#endif
