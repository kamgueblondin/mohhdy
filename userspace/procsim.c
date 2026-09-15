/* procsim.c - Table de processus simulée (pas le vrai scheduler). */

#include "procsim.h"

static procsim_entry_t g_procs[PROCSIM_MAX];
static int g_n;

static void set_proc(int i, int pid, int ppid, const char *name, char state, int cpu, int mem) {
    int k = 0;
    g_procs[i].pid = pid;
    g_procs[i].ppid = ppid;
    while (name[k] && k < 31) {
        g_procs[i].name[k] = name[k];
        k++;
    }
    g_procs[i].name[k] = 0;
    g_procs[i].state = state;
    g_procs[i].alive = 1;
    g_procs[i].cpu = cpu;
    g_procs[i].mem = mem;
}

void procsim_init(void) {
    int i;
    for (i = 0; i < PROCSIM_MAX; i++) {
        g_procs[i].pid = -1;
        g_procs[i].alive = 0;
        g_procs[i].name[0] = 0;
    }
    g_n = 5;
    set_proc(0, 0, 0, "[kernel]", 'R', 1, 21);
    set_proc(1, 1, 0, "init", 'S', 0, 15);
    set_proc(2, 2, 1, "ai-shell", 'R', 2, 32);
    set_proc(3, 3, 2, "ai-assistant", 'S', 0, 8);
    set_proc(4, 4, 1, "memory-manager", 'S', 0, 5);
}

int procsim_count(void) {
    return g_n;
}

int procsim_alive_count(void) {
    int c = 0;
    for (int i = 0; i < g_n; i++) {
        if (g_procs[i].alive) c++;
    }
    return c;
}

const procsim_entry_t *procsim_get_by_index(int i) {
    if (i < 0 || i >= g_n) return 0;
    return &g_procs[i];
}

const procsim_entry_t *procsim_get_by_pid(int pid) {
    for (int i = 0; i < g_n; i++) {
        if (g_procs[i].pid == pid) return &g_procs[i];
    }
    return 0;
}

int procsim_kill(int pid) {
    procsim_entry_t *p = 0;
    for (int i = 0; i < g_n; i++) {
        if (g_procs[i].pid == pid) {
            p = &g_procs[i];
            break;
        }
    }
    if (!p) return -1;
    if (pid == 0 || pid == 1) return -2;
    if (!p->alive) return -1;
    p->alive = 0;
    p->state = 'Z';
    p->cpu = 0;
    return 0;
}
