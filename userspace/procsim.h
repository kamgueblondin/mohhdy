/* procsim.h - Table de processus simulée pour ps/kill/jobs/top */

#ifndef MOHHDY_PROCSIM_H
#define MOHHDY_PROCSIM_H

#define PROCSIM_MAX 16

typedef struct {
    int pid;
    int ppid;
    char name[32];
    char state; /* R, S, Z */
    int alive;
    int cpu; /* dixièmes de % */
    int mem; /* dixièmes de % */
} procsim_entry_t;

void procsim_init(void);
int procsim_count(void);
int procsim_alive_count(void);
const procsim_entry_t *procsim_get_by_index(int i);
const procsim_entry_t *procsim_get_by_pid(int pid);
/* 0 ok, -1 introuvable, -2 protégé (kernel/init) */
int procsim_kill(int pid);

#endif
