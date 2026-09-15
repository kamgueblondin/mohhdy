#ifndef TASK_H
#define TASK_H

#include <stdint.h>
#include "kernel/mem/vmm.h" // Inclure pour vmm_directory_t
#include "os_syscalls.h"
#include "../ipc.h"

// États possibles d'une tâche
typedef enum {
    TASK_RUNNING,
    TASK_READY,
    TASK_WAITING,
    TASK_WAITING_FOR_INPUT,
    TASK_SUSPENDED,
    TASK_TERMINATED
} task_state_t;

// Types de tâches
typedef enum {
    TASK_TYPE_KERNEL,
    TASK_TYPE_USER
} task_type_t;

// Structure pour sauvegarder l'état du CPU
// L'ordre doit correspondre à ce qui est poussé sur la pile par les ISR stubs.
// L'ordre des push dans l'ISR est: push ds, es, fs, gs, puis pushad.
// La pile (de l'adresse la plus basse à la plus haute) est donc: edi, esi, ..., eax, gs, fs, es, ds.
typedef struct cpu_state {
    // Pushed by PUSHAD
    uint32_t edi, esi, ebp, esp_dummy, ebx, edx, ecx, eax;
    // Pushed manually by our ISR stub
    uint32_t gs, fs, es, ds;
    // Pushed by the CPU on interrupt from user mode
    uint32_t eip, cs, eflags, useresp, ss;
} cpu_state_t;

// Structure pour une tâche
typedef struct task {
    int id;
    cpu_state_t cpu_state;
    task_state_t state;
    task_type_t type;          // Type de tâche (kernel/user)
    uint32_t priority;          // Politique CPU locale : 1 (bas) à 3 (haut)
    vmm_directory_t* vmm_dir;  // Répertoire de pages de la tâche
    uint32_t kernel_stack_p;   // Pointeur vers le sommet de la pile noyau
    char name[32];
    int parent_pid;            // Créateur direct, -1 pour la tâche racine
    int waiter_pid;            // Parent TASK_WAITING (SYS_EXEC), 0 sinon
    uint32_t created_ticks;    // Instant de création, horloge système locale
    uint32_t last_scheduled_ticks;
    uint32_t run_ticks;        // Temps cumulé approximatif en tâche courante
    uint32_t switch_count;     // Nombre de sélections par l’ordonnanceur
    int last_child_pid;         // Dernier enfant direct terminé observé par ce parent
    int last_child_exit_code;   // Code SYS_EXIT ou OS_TASK_EXIT_KILLED
    uint32_t last_child_exit_reason;
    uint32_t last_child_finished_ticks;
    os_task_exit_result_t child_exit_history[OS_TASK_EXIT_HISTORY_CAPACITY];
    uint32_t child_exit_history_start;
    uint32_t child_exit_history_count;
    uint32_t child_exit_history_generation;
    uint32_t direct_child_exit_count; /* Départs directs cumulés depuis la création. */
    os_task_supervision_event_t supervision_events[OS_TASK_SUPERVISION_EVENT_CAPACITY];
    uint32_t supervision_event_start;
    uint32_t supervision_event_count;
    uint32_t supervision_event_generation;
    uint32_t supervision_event_sequence;
    uint32_t supervision_notify_enabled; /* Souscription IPC locale et volatile. */
    uint32_t supervision_notify_mask; /* Filtre local des transitions à délivrer. */
    uint32_t supervision_watch_enabled; /* Watchlist détaillée locale et volatile. */
    uint32_t supervision_watch_count;
    int32_t supervision_watch_pids[OS_TASK_SUPERVISION_WATCH_CAPACITY];
    uint32_t supervision_delivery_attempted;
    uint32_t supervision_delivery_delivered;
    uint32_t supervision_delivery_dropped;
    int32_t supervision_priority_child_pid;
    uint32_t supervision_notify_budget_limit;
    uint32_t supervision_notify_budget_used;
    ipc_endpoint_t ipc_endpoint; // Boîte aux lettres IPC propre à la tâche
    struct task* next;         // Pour la liste chaînée de tâches
    struct task* prev;         // Liste doublement chaînée
} task_t;

// Variables globales
extern task_t* current_task;
extern task_t* task_queue;
extern int next_task_id;
extern volatile int g_reschedule_needed;

// Fonctions publiques
void tasking_init();
task_t* create_task(void (*entry_point)());
task_t* create_task_from_initrd_file(const char* filename);
task_t* load_elf_task(uint8_t* elf_data, uint32_t size);
void schedule(cpu_state_t* cpu);
void jump_to_task(cpu_state_t* state);
void task_exit();
void task_yield();

// Fonctions utilitaires
task_t* get_task_by_id(int id);
void remove_task(task_t* task);
void add_task_to_queue(task_t* task);
int get_task_count();
task_t* find_task_waiting_for_input(void);
/* Vrai lorsqu’une autre tâche Ring 3 prête peut recevoir un quantum IRQ0. */
int task_has_other_ready_user(void);
int task_kill(int requester_pid, int pid);
void task_reparent_children(task_t* departing);
uint32_t task_count_direct_children(int pid);
int task_can_create_child(int pid);
int task_can_create_global(void);
int task_wait_for_child(int requester_pid, int child_pid);
int task_wait_for_any_child(int requester_pid);
int task_fill_direct_children(int requester_pid, os_task_children_t* out);
int task_fill_ps(os_proc_t* out, int max_n);
int task_fill_metrics(int pid, os_task_metrics_t* out);
int task_fill_capacity(os_task_capacity_t* out);
int task_set_priority(int requester_pid, int pid, uint32_t priority);
int task_set_name(int requester_pid, int pid, const char* name);
int task_get_child_result(int requester_pid, int child_pid, os_task_exit_result_t* out);
int task_fill_child_result_history(int requester_pid, os_task_exit_history_t* out);
int task_ack_child_result_history(int requester_pid);
int task_observe_child_result_history(int requester_pid, uint32_t expected_generation,
                                      os_task_exit_history_observation_t* out);
int task_find_child_result_history(int requester_pid, int child_pid, os_task_exit_result_t* out);
int task_forget_child_result_history(int requester_pid, int child_pid);
int task_suspend_child(int requester_pid, int child_pid);
int task_resume_child(int requester_pid, int child_pid);
int task_kill_direct_children(int requester_pid);
int task_get_child_exit_count(int requester_pid, uint32_t* out);
int task_delegate_child(int requester_pid, int child_pid, int supervisor_pid);
int task_fill_supervision_events(int requester_pid, os_task_supervision_events_t* out);
int task_ack_supervision_events(int requester_pid);
int task_observe_supervision_events(int requester_pid, uint32_t expected_generation,
                                    os_task_supervision_events_observation_t* out);
int task_find_supervision_event(int requester_pid, uint32_t sequence,
                                os_task_supervision_event_t* out);
int task_forget_supervision_event(int requester_pid, uint32_t sequence);
int task_fill_supervision_summary(int requester_pid, os_task_supervision_summary_t* out);
int task_set_supervision_notify(int requester_pid, uint32_t enabled);
int task_set_supervision_notify_filter(int requester_pid, uint32_t mask);
int task_fill_supervision_notify_status(int requester_pid,
                                        os_task_supervision_notify_status_t* out);
int task_update_supervision_watch(int requester_pid, int child_pid, uint32_t enabled);
int task_fill_supervision_watch_status(int requester_pid,
                                       os_task_supervision_watch_status_t* out);
int task_fill_supervision_delivery_stats(int requester_pid,
                                         os_task_supervision_delivery_stats_t* out);
int task_ack_supervision_delivery_stats(int requester_pid);
int task_replay_supervision_event(int requester_pid, uint32_t sequence);
int task_set_supervision_priority(int requester_pid, int child_pid);
int task_fill_supervision_priority_status(int requester_pid,
                                          os_task_supervision_priority_status_t* out);
int task_set_supervision_notify_budget(int requester_pid, uint32_t limit);
int task_fill_supervision_notify_budget_status(int requester_pid,
                                                os_task_supervision_notify_budget_status_t* out);
void task_wake_waiter(task_t* child);
void task_report_parent_exit(task_t* child, int exit_code, uint32_t reason);

#endif

