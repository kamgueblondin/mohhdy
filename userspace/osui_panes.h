/* osui_panes.h - live content of the Statut, Fichiers and Admin windows of
 * the desktop. Every line comes from a real guest syscall (the same ones
 * as the shell commands uptime, mem, ps, ls and service-status); the
 * sources are injected so the formatting is host testable. */
#ifndef OSUI_PANES_H
#define OSUI_PANES_H
#include "os_syscalls.h"

typedef struct {
    int (*listdir)(const char *path, os_dirent_t *out, int max_n);
    int (*ps)(os_proc_t *out, int max_n);
    int (*meminfo)(os_meminfo_t *info);
    unsigned int (*ticks)(void);
    int (*service_status)(const char *name, os_service_status_t *st);
    /* Runs one shell command and captures what it prints (P2P, Collab,
     * Prod, Support windows); returns the byte count or < 0. */
    int (*run)(const char *cmd, char *out, int max);
} osui_pane_src_t;

#define OSUI_PANES_LINE 64
/* Shell command whose real output fills the pane, 0 when the pane has
 * none (P2P p2p-stats, Collab collab-balances, Prod prod-metrics,
 * Support prod-diag). */
const char *osui_panes_command(int pane);
/* Fill up to max lines for pane (STATUS / FS / ADMIN / BROWSER and the
 * command panes above).
 * Returns the line count, 0 for other panes or without a source. */
int osui_panes_fill(int pane, const osui_pane_src_t *src, char lines[][OSUI_PANES_LINE], int max);
/* Services the Admin window asks the registry about. */
extern const char *const osui_panes_services[];
extern const int osui_panes_service_count;
#endif
