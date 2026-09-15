#include "task.h"
#include "kernel/mem/pmm.h"
#include "kernel/mem/vmm.h"
#include <stddef.h>
#include "kernel/elf.h"
#include "kernel/mem/string.h"
#include "kernel.h"
#include "kernel/mem/heap.h"
#include "fs/initrd.h"
#include "kernel/gdt.h"
#include "kernel/timer.h"

// Variables globales
task_t* current_task = NULL;
task_t* task_queue = NULL;
int next_task_id = 0;
volatile int g_reschedule_needed = 0;
/* Tâche détachée à libérer lors d’un passage ultérieur, hors de sa pile et de son VMM. */
static task_t* deferred_reap_task = NULL;

/* 16 Kio : le polling TLS empile plusieurs instantanes handshake/client
 * puis un verify RSA. 4 Kio suffisait au ServerHello mais pas a l'identite. */
#define TASK_STATIC_KERNEL_STACK_SIZE 16384U
static task_t task_static_pool[OS_TASK_GLOBAL_CAPACITY];
static uint8_t task_static_used[OS_TASK_GLOBAL_CAPACITY];
static uint8_t task_static_kernel_stacks[OS_TASK_GLOBAL_CAPACITY][TASK_STATIC_KERNEL_STACK_SIZE] __attribute__((aligned(16)));
static vmm_directory_t task_static_vmm_pool[OS_TASK_GLOBAL_CAPACITY];
static uint8_t task_static_vmm_used[OS_TASK_GLOBAL_CAPACITY];
static page_table_t* task_static_vmm_tables[OS_TASK_GLOBAL_CAPACITY][ENTRIES_PER_TABLE];
static page_directory_t task_static_vmm_physical_dirs[OS_TASK_GLOBAL_CAPACITY] __attribute__((aligned(PAGE_SIZE)));

static vmm_directory_t* task_static_vmm_acquire(void) {
    uint32_t index, table_index;
    vmm_directory_t* dir;
    for (index = 0U; index < OS_TASK_GLOBAL_CAPACITY; index++) {
        if (task_static_vmm_used[index]) continue;
        task_static_vmm_used[index] = 1U;
        dir = &task_static_vmm_pool[index];
        memset(dir, 0, sizeof(vmm_directory_t));
        memset(task_static_vmm_tables[index], 0, sizeof(task_static_vmm_tables[index]));
        memset(&task_static_vmm_physical_dirs[index], 0, sizeof(page_directory_t));
        dir->tables = task_static_vmm_tables[index];
        dir->physical_dir = &task_static_vmm_physical_dirs[index];
        dir->physical_addr = (uint32_t)dir->physical_dir;
        dir->static_storage = 1U;
        for (table_index = 0U; table_index < ENTRIES_PER_TABLE; table_index++) {
            if (kernel_directory->tables[table_index]) {
                dir->tables[table_index] = kernel_directory->tables[table_index];
                dir->physical_dir->tablesPhysical[table_index] = kernel_directory->physical_dir->tablesPhysical[table_index];
            }
        }
        return dir;
    }
    return NULL;
}

static void task_static_vmm_release(vmm_directory_t* dir) {
    uint32_t index;
    for (index = 0U; index < OS_TASK_GLOBAL_CAPACITY; index++) {
        if (dir == &task_static_vmm_pool[index]) {
            task_static_vmm_used[index] = 0U;
            return;
        }
    }
}

static task_t* task_static_acquire(void) {
    uint32_t index;
    for (index = 0U; index < OS_TASK_GLOBAL_CAPACITY; index++) {
        if (!task_static_used[index]) {
            task_static_used[index] = 1U;
            memset(&task_static_pool[index], 0, sizeof(task_t));
            return &task_static_pool[index];
        }
    }
    return NULL;
}

static void task_static_release(task_t* task) {
    uint32_t index;
    for (index = 0U; index < OS_TASK_GLOBAL_CAPACITY; index++) {
        if (task == &task_static_pool[index]) {
            memset(&task_static_pool[index], 0, sizeof(task_t));
            task_static_used[index] = 0U;
            return;
        }
    }
}

static uint32_t task_static_kernel_stack_top(const task_t* task) {
    uint32_t index;
    for (index = 0U; index < OS_TASK_GLOBAL_CAPACITY; index++) {
        if (task == &task_static_pool[index]) return (uint32_t)&task_static_kernel_stacks[index][TASK_STATIC_KERNEL_STACK_SIZE];
    }
    return 0U;
}

// Externes
extern vmm_directory_t* kernel_directory;
extern vmm_directory_t* current_directory;
extern void print_string_serial(const char* str);

// Prototypes
vmm_directory_t* create_user_vmm_directory();
uint32_t allocate_user_stack(vmm_directory_t* vmm_dir);
void setup_initial_user_context(task_t* task, uint32_t entry_point, uint32_t stack_top);


void tasking_init() {
    deferred_reap_task = NULL;
    memset(task_static_used, 0, sizeof(task_static_used));
    memset(task_static_vmm_used, 0, sizeof(task_static_vmm_used));
    current_task = task_static_acquire();
    if (!current_task) return;
    current_task->id = next_task_id++;
    current_task->state = TASK_RUNNING;
    current_task->type = TASK_TYPE_KERNEL;
    current_task->priority = OS_TASK_PRIORITY_NORMAL;
    current_task->vmm_dir = kernel_directory;
    current_task->kernel_stack_p = 0;
    current_task->parent_pid = -1;
    current_task->waiter_pid = 0;
    current_task->created_ticks = timer_get_ticks();
    current_task->last_scheduled_ticks = current_task->created_ticks;
    current_task->run_ticks = 0U;
    current_task->switch_count = 1U;
    current_task->last_child_pid = -1;
    current_task->last_child_exit_code = 0;
    current_task->last_child_exit_reason = 0U;
    current_task->last_child_finished_ticks = 0U;
    current_task->child_exit_history_start = 0U;
    current_task->child_exit_history_count = 0U;
    current_task->child_exit_history_generation = 1U;
    current_task->direct_child_exit_count = 0U;
    current_task->supervision_event_start = 0U;
    current_task->supervision_event_count = 0U;
    current_task->supervision_event_generation = 0U;
    current_task->supervision_event_sequence = 0U;
    current_task->supervision_notify_enabled = 0U;
    current_task->supervision_notify_mask = OS_TASK_SUPERVISION_NOTIFY_ALL;
    current_task->supervision_watch_enabled = 0U;
    current_task->supervision_watch_count = 0U;
    memset(current_task->supervision_watch_pids, 0, sizeof(current_task->supervision_watch_pids));
    current_task->supervision_delivery_attempted = 0U;
    current_task->supervision_delivery_delivered = 0U;
    current_task->supervision_delivery_dropped = 0U;
    current_task->supervision_priority_child_pid = -1;
    current_task->supervision_notify_budget_limit = 0U;
    current_task->supervision_notify_budget_used = 0U;
    ipc_endpoint_init(&current_task->ipc_endpoint);
    current_task->name[0] = 'k';
    current_task->name[1] = 'e';
    current_task->name[2] = 'r';
    current_task->name[3] = 'n';
    current_task->name[4] = 'e';
    current_task->name[5] = 'l';
    current_task->name[6] = '\0';
    current_task->next = current_task;
    current_task->prev = current_task;
    task_queue = current_task;
    print_string_serial("Tache kernel creee.\n");
}

void add_task_to_queue(task_t* task) {
    if (!task_queue) {
        task_queue = task;
        task->next = task;
        task->prev = task;
    } else {
        task->next = task_queue;
        task->prev = task_queue->prev;
        task_queue->prev->next = task;
        task_queue->prev = task;
    }
}

// Déclaration de la fonction assembleur pour le changement de contexte
extern void jump_to_task(cpu_state_t* next_state);
static void unlink_task(task_t* task);

static int task_destroy_user_vmm(vmm_directory_t* dir) {
    if (!dir) return 0;
    if (vmm_destroy_user_directory(dir) != 0) return -1;
    task_static_vmm_release(dir);
    return 0;
}

static void task_release_detached(task_t* task) {
    if (!task || task->type != TASK_TYPE_USER || !task->kernel_stack_p) return;
    if (task_destroy_user_vmm(task->vmm_dir) != 0) return;
    task->vmm_dir = NULL;
    task->kernel_stack_p = 0U;
    task_static_release(task);
}

static void task_reap_deferred(void) {
    task_t* task = deferred_reap_task;
    deferred_reap_task = NULL;
    task_release_detached(task);
}

void remove_task(task_t* task) {
    task_t* cursor;
    int found = 0;
    if (!task || task == current_task || !task_queue) return;
    cursor = task_queue;
    do {
        if (cursor == task) { found = 1; break; }
        cursor = cursor->next;
    } while (cursor && cursor != task_queue);
    if (!found) return;
    unlink_task(task);
    task_release_detached(task);
}

static void unlink_task(task_t* task) {
    if (!task_queue || !task) return;
    if (task->next == task) {
        // Single element in queue
        task_queue = NULL;
        current_task = NULL;
        return;
    }
    task->prev->next = task->next;
    task->next->prev = task->prev;
    if (task_queue == task) task_queue = task->next;
    if (current_task == task) current_task = task->next;
}

void schedule(cpu_state_t* cpu) {
    uint32_t now = timer_get_ticks();
    asm volatile("cli"); // Désactiver les interruptions pour la planification
    task_reap_deferred();
    if (!current_task) {
        asm volatile("sti");
        return;
    }

    // Si la tache courante est terminee, ne pas sauver l'etat; retirer de la file
    if (current_task->state != TASK_TERMINATED) {
        // Sauvegarder l'état de la tâche actuelle
        if (now >= current_task->last_scheduled_ticks) current_task->run_ticks += now - current_task->last_scheduled_ticks;
        memcpy(&current_task->cpu_state, cpu, sizeof(cpu_state_t));
    } else {
        task_t* terminated_task = current_task;
        print_string_serial("[SCHED] removing terminated task\n");
        unlink_task(terminated_task);
        deferred_reap_task = terminated_task;
        if (!task_queue) {
            asm volatile("sti");
            while(1) asm volatile("hlt");
        }
    }

    // Si la tâche tournait, elle est maintenant prête à être replanifiée plus tard
    if (current_task->state == TASK_RUNNING) {
        current_task->state = TASK_READY;
    }

    /* Préférer une tâche Ring 3 prête. Parmi les candidates de même classe,
     * la priorité la plus haute gagne ; le parcours circulaire conserve le
     * round-robin lorsque la priorité est égale. La tâche noyau reste READY
     * après le premier saut vers le shell et ne doit pas recevoir un ancien
     * cadre utilisateur. */
    {
        task_t* start = current_task ? current_task : task_queue;
        task_t* t = start;
        task_t* next_task = start;
        uint32_t best_priority = 0U;
        int found_user = 0;

        do {
            t = t->next;
            if (t->state == TASK_READY && t->type == TASK_TYPE_USER &&
                (!found_user || t->priority > best_priority)) {
                next_task = t;
                best_priority = t->priority;
                found_user = 1;
            }
        } while (t != start);

        if (start->state == TASK_READY && start->type == TASK_TYPE_USER &&
            (!found_user || start->priority > best_priority)) {
            next_task = start;
            found_user = 1;
        }

        if (!found_user) {
            t = start;
            do {
                t = t->next;
                if (t->state == TASK_READY) {
                    next_task = t;
                    break;
                }
            } while (t != start);
        }

        current_task = next_task;
    }

    print_string_serial("[SCHED] switching to task ");
    write_serial('0' + (current_task->id % 10));
    print_string_serial("\n");
    current_task->state = TASK_RUNNING;
    current_task->last_scheduled_ticks = now;
    current_task->switch_count++;

    // Mettre à jour le TSS avec la pile noyau de la nouvelle tâche
    if (current_task->type == TASK_TYPE_USER) {
        tss_set_stack(0x10, current_task->kernel_stack_p);
    }

    // Changer de répertoire de pages si nécessaire
    if (current_directory != current_task->vmm_dir) {
        vmm_switch_page_directory(current_task->vmm_dir->physical_addr);
        current_directory = current_task->vmm_dir;
    }

    // Sauter à la nouvelle tâche. Ne retourne jamais.
    jump_to_task(&current_task->cpu_state);
}

void task_exit() {
    if (!current_task) return;

    current_task->state = TASK_TERMINATED;
    print_string_serial("[TASK_EXIT] terminating, scheduling now\n");

    // Utiliser l'appel système pour quitter proprement
    // Cela déclenchera le scheduler sans sauvegarder l'état de cette tâche
    asm volatile("int $0x80" : : "a"(0), "b"(0));

    // On ne devrait jamais arriver ici
    while(1) { asm volatile("hlt"); }
}

task_t* create_task_from_initrd_file(const char* filename) {
    uint8_t* file_data;
    const char* name_src;
    char alt[256];

    if (task_can_create_global() != 0) {
        print_string_serial("ERREUR: Capacite globale de taches atteinte\n");
        return NULL;
    }
    file_data = (uint8_t*)initrd_read_file(filename);
    name_src = filename;
    if (!file_data && filename && filename[0] && filename[0] != '/') {
        int slash = 0;
        int i = 0;
        while (filename[i]) {
            if (filename[i] == '/') slash = 1;
            i++;
        }
        if (!slash) {
            alt[0] = 'b'; alt[1] = 'i'; alt[2] = 'n'; alt[3] = '/';
            i = 0;
            while (filename[i] && i < 250) {
                alt[4 + i] = filename[i];
                i++;
            }
            alt[4 + i] = '\0';
            file_data = (uint8_t*)initrd_read_file(alt);
            if (file_data) name_src = alt;
        }
    }
    if (!file_data) {
        print_string_serial("ERREUR: Fichier non trouve dans l'initrd\n");
        return NULL;
    }

    vmm_directory_t* vmm_dir = create_user_vmm_directory();
    if (!vmm_dir) {
        print_string_serial("ERREUR: Creation VMM directory a echoue\n");
        return NULL;
    }

    uint32_t entry_point = 0;
    uint32_t user_stack_top = 0;

    // --- Critical section for new task's address space ---
    vmm_directory_t* old_dir = current_directory;
    vmm_switch_page_directory(vmm_dir->physical_addr);
    current_directory = vmm_dir;

    // Load the executable into the new address space
    entry_point = elf_load(file_data, vmm_dir);
    if (entry_point != 0) {
        // Allocate the user stack in the new address space
        user_stack_top = allocate_user_stack(vmm_dir);
    }

    // Restore the kernel's address space
    vmm_switch_page_directory(old_dir->physical_addr);
    current_directory = old_dir;
    // --- End of critical section ---

    if (entry_point == 0 || user_stack_top == 0) {
        print_string_serial("ERREUR: Chargement ELF ou allocation de pile a echoue\n");
        (void)task_destroy_user_vmm(vmm_dir);
        return NULL;
    }

    task_t* new_task = task_static_acquire();
    if (!new_task) {
        (void)task_destroy_user_vmm(vmm_dir);
        return NULL;
    }
    new_task->id = next_task_id++;
    new_task->state = TASK_READY;
    new_task->type = TASK_TYPE_USER;
    new_task->priority = OS_TASK_PRIORITY_NORMAL;
    new_task->vmm_dir = vmm_dir;
    new_task->parent_pid = -1;
    new_task->waiter_pid = 0;
    new_task->created_ticks = timer_get_ticks();
    new_task->last_scheduled_ticks = new_task->created_ticks;
    new_task->run_ticks = 0U;
    new_task->switch_count = 0U;
    new_task->last_child_pid = -1;
    new_task->last_child_exit_code = 0;
    new_task->last_child_exit_reason = 0U;
    new_task->last_child_finished_ticks = 0U;
    new_task->child_exit_history_start = 0U;
    new_task->child_exit_history_count = 0U;
    new_task->child_exit_history_generation = 1U;
    new_task->direct_child_exit_count = 0U;
    new_task->supervision_event_start = 0U;
    new_task->supervision_event_count = 0U;
    new_task->supervision_event_generation = 0U;
    new_task->supervision_event_sequence = 0U;
    new_task->supervision_notify_enabled = 0U;
    new_task->supervision_notify_mask = OS_TASK_SUPERVISION_NOTIFY_ALL;
    new_task->supervision_watch_enabled = 0U;
    new_task->supervision_watch_count = 0U;
    memset(new_task->supervision_watch_pids, 0, sizeof(new_task->supervision_watch_pids));
    new_task->supervision_delivery_attempted = 0U;
    new_task->supervision_delivery_delivered = 0U;
    new_task->supervision_delivery_dropped = 0U;
    new_task->supervision_priority_child_pid = -1;
    new_task->supervision_notify_budget_limit = 0U;
    new_task->supervision_notify_budget_used = 0U;
    ipc_endpoint_init(&new_task->ipc_endpoint);
    {
        int i = 0;
        const char* n = name_src ? name_src : "user";
        const char* base = n;
        while (*n) {
            if (*n == '/') base = n + 1;
            n++;
        }
        if (!base[0]) base = "user";
        while (base[i] && i < 31) {
            new_task->name[i] = base[i];
            i++;
        }
        new_task->name[i] = '\0';
    }

    /* Pile noyau dédiée issue du pool statique, sans allocation de tas. */
    new_task->kernel_stack_p = task_static_kernel_stack_top(new_task);
    if (!new_task->kernel_stack_p) {
        task_static_release(new_task);
        (void)task_destroy_user_vmm(vmm_dir);
        return NULL;
    }

    setup_initial_user_context(new_task, entry_point, user_stack_top);
    add_task_to_queue(new_task);

    print_string_serial("Tache utilisateur creee avec succes\n");
    return new_task;
}

void setup_initial_user_context(task_t* task, uint32_t entry_point, uint32_t stack_top) {
    memset(&task->cpu_state, 0, sizeof(cpu_state_t));
    
    // Configuration des registres généraux
    task->cpu_state.eax = 0;
    task->cpu_state.ebx = 0;
    task->cpu_state.ecx = 0;
    task->cpu_state.edx = 0;
    task->cpu_state.esi = 0;
    task->cpu_state.edi = 0;
    task->cpu_state.ebp = 0;
    
    // Configuration de l'exécution
    task->cpu_state.eip = entry_point;
    task->cpu_state.useresp = stack_top;
    task->cpu_state.eflags = 0x202; // Interruptions activées
    
    // Configuration des segments utilisateur (Ring 3)
    task->cpu_state.cs = 0x1B;  // Code segment utilisateur (Ring 3)
    task->cpu_state.ds = 0x23;  // Data segment utilisateur (Ring 3)
    task->cpu_state.es = 0x23;
    task->cpu_state.fs = 0x23;
    task->cpu_state.gs = 0x23;
    task->cpu_state.ss = 0x23;  // Stack segment utilisateur (Ring 3)
    
    print_string_serial("User context setup complete. EIP=0x");
    print_hex_serial(task->cpu_state.eip);
    print_string_serial(", ESP=0x");
    print_hex_serial(task->cpu_state.useresp);
    print_string_serial("\n");
}

vmm_directory_t* create_user_vmm_directory() {
    vmm_directory_t* dir;
    print_string_serial("create_user_vmm_directory: static pool\n");
    dir = task_static_vmm_acquire();
    if (!dir) print_string_serial("create_user_vmm_directory: static pool exhausted\n");
    return dir;
}

#define USER_STACK_TOP 0xB0000000
#define USER_STACK_PAGES 16
#define USER_STACK_SIZE (USER_STACK_PAGES * PAGE_SIZE)
#define USER_STACK_BOTTOM (USER_STACK_TOP - USER_STACK_SIZE)

uint32_t allocate_user_stack(vmm_directory_t* vmm_dir) {
    print_string_serial("Allocating user stack...\n");
    for (uint32_t addr = USER_STACK_BOTTOM; addr < USER_STACK_TOP; addr += PAGE_SIZE) {
        void* stack_phys_page = pmm_alloc_page();
        if (!stack_phys_page) {
            print_string_serial("ERROR: Could not allocate physical page for user stack\n");
            // In a real scenario, we should free previously allocated pages
            return 0;
        }
        if (vmm_map_page_in_directory(vmm_dir, stack_phys_page, (void*)addr, PAGE_PRESENT | PAGE_WRITE | PAGE_USER) != 0) {
            pmm_free_page(stack_phys_page);
            return 0;
        }
    }

    print_string_serial("User stack allocated at 0x");
    print_hex_serial(USER_STACK_BOTTOM);
    print_string_serial(" - 0x");
    print_hex_serial(USER_STACK_TOP);
    print_string_serial("\n");

    return USER_STACK_TOP;
}

task_t* find_task_waiting_for_input() {
    if (!task_queue) {
        return NULL;
    }

    task_t* temp = task_queue;
    do {
        if (temp->state == TASK_WAITING_FOR_INPUT) {
            return temp;
        }
        temp = temp->next;
    } while (temp != task_queue);

    return NULL;
}

task_t* get_task_by_id(int id) {
    task_t* t;
    if (!task_queue) return NULL;
    t = task_queue;
    do {
        if (t->id == id) return t;
        t = t->next;
    } while (t && t != task_queue);
    return NULL;
}

int task_has_other_ready_user(void) {
    task_t* t;
    if (!task_queue || !current_task) return 0;
    t = current_task->next;
    while (t && t != current_task) {
        if (t->type == TASK_TYPE_USER && t->state == TASK_READY) return 1;
        t = t->next;
    }
    return 0;
}

int get_task_count(void) {
    int n = 0;
    task_t* t;
    if (!task_queue) return 0;
    t = task_queue;
    do {
        n++;
        t = t->next;
    } while (t && t != task_queue);
    return n;
}

void task_reparent_children(task_t* departing) {
    task_t* t;
    if (!departing || !task_queue) return;
    t = task_queue;
    do {
        if (t != departing && t->parent_pid == departing->id) {
            t->parent_pid = departing->parent_pid;
        }
        t = t->next;
    } while (t && t != task_queue);
}

uint32_t task_count_direct_children(int pid) {
    task_t* t;
    uint32_t count = 0U;
    if (!task_queue) return 0U;
    t = task_queue;
    do {
        if (t->parent_pid == pid) count++;
        t = t->next;
    } while (t && t != task_queue);
    return count;
}

int task_can_create_child(int pid) {
    if (!get_task_by_id(pid)) return OS_TASK_NOT_FOUND;
    if (task_count_direct_children(pid) >= OS_TASK_CHILD_CAPACITY) {
        return OS_TASK_CHILD_LIMIT;
    }
    return 0;
}

int task_can_create_global(void) {
    if ((uint32_t)get_task_count() >= OS_TASK_GLOBAL_CAPACITY) {
        return OS_TASK_GLOBAL_LIMIT;
    }
    return 0;
}

int task_wait_for_child(int requester_pid, int child_pid) {
    task_t* parent = get_task_by_id(requester_pid);
    task_t* child = get_task_by_id(child_pid);
    if (!parent || !child) return OS_TASK_NOT_FOUND;
    if (child->parent_pid != requester_pid) return OS_TASK_NOT_CHILD;
    if (child->waiter_pid != 0 && child->waiter_pid != requester_pid) {
        return OS_TASK_CONTROL_DENIED;
    }
    child->waiter_pid = requester_pid;
    parent->state = TASK_WAITING;
    return 0;
}

int task_wait_for_any_child(int requester_pid) {
    task_t* parent = get_task_by_id(requester_pid);
    task_t* t;
    int found = 0;
    if (!parent || !task_queue) return OS_TASK_NOT_FOUND;
    t = task_queue;
    do {
        if (t->parent_pid == requester_pid) {
            if (t->waiter_pid != 0 && t->waiter_pid != requester_pid) {
                return OS_TASK_CONTROL_DENIED;
            }
            t->waiter_pid = requester_pid;
            found = 1;
        }
        t = t->next;
    } while (t && t != task_queue);
    if (!found) return OS_TASK_NO_DIRECT_CHILD;
    parent->state = TASK_WAITING;
    return 0;
}

int task_kill(int requester_pid, int pid) {
    task_t* t;
    if (pid == 0) return -2;
    t = get_task_by_id(pid);
    if (!t) return -1;
    if (requester_pid == pid) return -3;
    if (requester_pid != t->parent_pid) return OS_TASK_CONTROL_DENIED;
    task_report_parent_exit(t, OS_TASK_EXIT_KILLED, OS_TASK_EVENT_KILLED);
    task_wake_waiter(t);
    task_reparent_children(t);
    t->state = TASK_TERMINATED;
    unlink_task(t);
    task_release_detached(t);
    return 0;
}

int task_kill_direct_children(int requester_pid) {
    int child_pids[OS_TASK_CHILD_CAPACITY];
    task_t* t;
    uint32_t count = 0U;
    uint32_t i;
    if (!get_task_by_id(requester_pid) || !task_queue) return OS_TASK_NOT_FOUND;
    t = task_queue;
    do {
        if (t->parent_pid == requester_pid && count < OS_TASK_CHILD_CAPACITY) {
            child_pids[count++] = t->id;
        }
        t = t->next;
    } while (t && t != task_queue);
    for (i = 0U; i < count; i++) {
        task_t* child = get_task_by_id(child_pids[i]);
        if (!child || child->parent_pid != requester_pid) continue;
        task_report_parent_exit(child, OS_TASK_EXIT_KILLED, OS_TASK_EVENT_KILLED);
        task_wake_waiter(child);
        task_reparent_children(child);
        child->state = TASK_TERMINATED;
        unlink_task(child);
        task_release_detached(child);
    }
    return (int)count;
}

static uint32_t task_supervision_notify_bit(uint32_t action) {
    if (action == OS_TASK_SUPERVISION_EXIT) return OS_TASK_SUPERVISION_NOTIFY_EXIT;
    if (action == OS_TASK_SUPERVISION_SUSPEND) return OS_TASK_SUPERVISION_NOTIFY_SUSPEND;
    if (action == OS_TASK_SUPERVISION_RESUME) return OS_TASK_SUPERVISION_NOTIFY_RESUME;
    if (action == OS_TASK_SUPERVISION_DELEGATE_OUT) return OS_TASK_SUPERVISION_NOTIFY_DELEGATE_OUT;
    if (action == OS_TASK_SUPERVISION_DELEGATE_IN) return OS_TASK_SUPERVISION_NOTIFY_DELEGATE_IN;
    return 0U;
}

static int task_supervision_watch_index(const task_t* parent, int child_pid) {
    uint32_t i;
    if (!parent || child_pid <= 0) return -1;
    for (i = 0U; i < parent->supervision_watch_count; i++) {
        if (parent->supervision_watch_pids[i] == child_pid) return (int)i;
    }
    return -1;
}

static void task_remove_supervision_watch(task_t* parent, int child_pid) {
    int index;
    uint32_t i;
    if (!parent) return;
    index = task_supervision_watch_index(parent, child_pid);
    if (index < 0) return;
    for (i = (uint32_t)index; i + 1U < parent->supervision_watch_count; i++) {
        parent->supervision_watch_pids[i] = parent->supervision_watch_pids[i + 1U];
    }
    if (parent->supervision_watch_count > 0U) parent->supervision_watch_count--;
    if (parent->supervision_watch_count == 0U) parent->supervision_watch_enabled = 0U;
}

static int task_supervision_watch_allows(const task_t* parent, int child_pid) {
    if (!parent || parent->supervision_watch_enabled == 0U) return 1;
    return task_supervision_watch_index(parent, child_pid) >= 0;
}

static void task_notify_supervision_event(task_t* parent,
                                          const os_task_supervision_event_t* event) {
    os_ipc_payload_t payload;
    uint32_t bit;
    int priority;
    if (!parent || !event || parent->supervision_notify_enabled == 0U ||
        parent->type != TASK_TYPE_USER || parent->state == TASK_TERMINATED) return;
    bit = task_supervision_notify_bit(event->action);
    priority = parent->supervision_priority_child_pid == event->child_pid;
    if (!priority && (bit == 0U || (parent->supervision_notify_mask & bit) == 0U)) return;
    if (!priority && !task_supervision_watch_allows(parent, event->child_pid)) return;
    if (parent->supervision_notify_budget_limit != 0U &&
        parent->supervision_notify_budget_used >= parent->supervision_notify_budget_limit) return;
    if (os_task_make_supervision_event(&payload, event) != 0) return;
    parent->supervision_notify_budget_used++;
    parent->supervision_delivery_attempted++;
    /* Best effort : une boîte pleine ne retarde jamais une transition de supervision. */
    if (ipc_endpoint_send(&parent->ipc_endpoint, 0, &payload) == 0) {
        parent->supervision_delivery_delivered++;
    } else {
        parent->supervision_delivery_dropped++;
    }
}

static void task_record_supervision_event(task_t* parent, uint32_t action,
                                          int child_pid, int related_pid, uint32_t detail) {
    uint32_t index;
    os_task_supervision_event_t* event;
    if (!parent) return;
    if (parent->supervision_event_count < OS_TASK_SUPERVISION_EVENT_CAPACITY) {
        index = (parent->supervision_event_start + parent->supervision_event_count) %
                OS_TASK_SUPERVISION_EVENT_CAPACITY;
        parent->supervision_event_count++;
    } else {
        index = parent->supervision_event_start;
        parent->supervision_event_start = (parent->supervision_event_start + 1U) %
                                          OS_TASK_SUPERVISION_EVENT_CAPACITY;
    }
    event = &parent->supervision_events[index];
    parent->supervision_event_sequence++;
    if (parent->supervision_event_sequence == 0U) parent->supervision_event_sequence = 1U;
    event->sequence = parent->supervision_event_sequence;
    event->action = action;
    event->child_pid = child_pid;
    event->related_pid = related_pid;
    event->detail = detail;
    event->ticks = timer_get_ticks();
    parent->supervision_event_generation++;
    if (parent->supervision_event_generation == 0U) parent->supervision_event_generation = 1U;
    task_notify_supervision_event(parent, event);
}

int task_suspend_child(int requester_pid, int child_pid) {
    task_t* parent = get_task_by_id(requester_pid);
    task_t* child = get_task_by_id(child_pid);
    if (!child) return OS_TASK_NOT_FOUND;
    if (child->type != TASK_TYPE_USER || child->parent_pid != requester_pid) {
        return OS_TASK_CONTROL_DENIED;
    }
    if (child->state != TASK_READY) return OS_TASK_BAD_STATE;
    child->state = TASK_SUSPENDED;
    task_record_supervision_event(parent, OS_TASK_SUPERVISION_SUSPEND, child_pid, 0, 0U);
    return 0;
}

int task_resume_child(int requester_pid, int child_pid) {
    task_t* parent = get_task_by_id(requester_pid);
    task_t* child = get_task_by_id(child_pid);
    if (!child) return OS_TASK_NOT_FOUND;
    if (child->type != TASK_TYPE_USER || child->parent_pid != requester_pid) {
        return OS_TASK_CONTROL_DENIED;
    }
    if (child->state != TASK_SUSPENDED) return OS_TASK_BAD_STATE;
    child->state = TASK_READY;
    task_record_supervision_event(parent, OS_TASK_SUPERVISION_RESUME, child_pid, 0, 0U);
    return 0;
}

void task_wake_waiter(task_t* child) {
    task_t* parent;
    if (!child || child->waiter_pid <= 0) return;
    parent = get_task_by_id(child->waiter_pid);
    if (parent && parent->state == TASK_WAITING) {
        parent->state = TASK_READY;
    }
    child->waiter_pid = 0;
}

void task_report_parent_exit(task_t* child, int exit_code, uint32_t reason) {
    task_t* parent;
    os_ipc_payload_t payload;
    os_task_exit_result_t result;
    uint32_t index;
    if (!child || child->parent_pid <= 0) return;
    parent = get_task_by_id(child->parent_pid);
    if (!parent || parent->type != TASK_TYPE_USER || parent->state == TASK_TERMINATED) return;
    result.child_pid = child->id;
    result.exit_code = exit_code;
    result.reason = reason;
    result.finished_ticks = timer_get_ticks();
    parent->last_child_pid = result.child_pid;
    parent->last_child_exit_code = result.exit_code;
    parent->last_child_exit_reason = result.reason;
    parent->last_child_finished_ticks = result.finished_ticks;
    parent->direct_child_exit_count++;
    task_record_supervision_event(parent, OS_TASK_SUPERVISION_EXIT, child->id, 0, reason);
    if (parent->child_exit_history_count < OS_TASK_EXIT_HISTORY_CAPACITY) {
        index = (parent->child_exit_history_start + parent->child_exit_history_count) %
                OS_TASK_EXIT_HISTORY_CAPACITY;
        parent->child_exit_history_count++;
    } else {
        index = parent->child_exit_history_start;
        parent->child_exit_history_start = (parent->child_exit_history_start + 1U) %
                                           OS_TASK_EXIT_HISTORY_CAPACITY;
    }
    parent->child_exit_history[index] = result;
    parent->child_exit_history_generation++;
    if (parent->child_exit_history_generation == 0U) parent->child_exit_history_generation = 1U;
    task_remove_supervision_watch(parent, child->id);
    if (parent->supervision_priority_child_pid == child->id) {
        parent->supervision_priority_child_pid = -1;
    }
    if (os_task_make_event(&payload, child->id, reason) != 0) return;
    /* Best effort : la terminaison ne dépend jamais d’une boîte IPC disponible. */
    (void)ipc_endpoint_send(&parent->ipc_endpoint, 0, &payload);
}

static int32_t map_task_state(task_state_t s) {
    if (s == TASK_RUNNING) return OS_TASK_RUNNING;
    if (s == TASK_READY) return OS_TASK_READY;
    if (s == TASK_SUSPENDED) return OS_TASK_SUSPENDED;
    if (s == TASK_TERMINATED) return OS_TASK_TERMINATED;
    return OS_TASK_WAITING;
}

int task_get_child_exit_count(int requester_pid, uint32_t* out) {
    task_t* parent;
    if (!out) return OS_TASK_NOT_FOUND;
    parent = get_task_by_id(requester_pid);
    if (!parent) return OS_TASK_NOT_FOUND;
    *out = parent->direct_child_exit_count;
    return 0;
}

int task_delegate_child(int requester_pid, int child_pid, int supervisor_pid) {
    task_t* child;
    task_t* supervisor;
    task_t* cursor;
    uint32_t depth = 0U;

    if (!get_task_by_id(requester_pid)) return OS_TASK_NOT_FOUND;
    child = get_task_by_id(child_pid);
    supervisor = get_task_by_id(supervisor_pid);
    if (!child || !supervisor) return OS_TASK_NOT_FOUND;
    if (child->parent_pid != requester_pid) return OS_TASK_NOT_CHILD;
    if (supervisor->type != TASK_TYPE_USER || supervisor->state == TASK_TERMINATED ||
        supervisor_pid == requester_pid || supervisor_pid == child_pid) {
        return OS_TASK_BAD_DELEGATE;
    }
    if (child->waiter_pid != 0) return OS_TASK_BAD_STATE;
    if (task_count_direct_children(supervisor_pid) >= OS_TASK_CHILD_CAPACITY) {
        return OS_TASK_CHILD_LIMIT;
    }

    cursor = supervisor;
    while (cursor && depth++ < OS_TASK_GLOBAL_CAPACITY) {
        if (cursor->id == child_pid) return OS_TASK_BAD_DELEGATE;
        if (cursor->parent_pid < 0) break;
        cursor = get_task_by_id(cursor->parent_pid);
    }
    if (depth > OS_TASK_GLOBAL_CAPACITY) return OS_TASK_BAD_DELEGATE;

    task_record_supervision_event(get_task_by_id(requester_pid),
                                  OS_TASK_SUPERVISION_DELEGATE_OUT,
                                  child_pid, supervisor_pid, 0U);
    task_record_supervision_event(supervisor, OS_TASK_SUPERVISION_DELEGATE_IN,
                                  child_pid, requester_pid, 0U);
    {
        task_t* requester = get_task_by_id(requester_pid);
        task_remove_supervision_watch(requester, child_pid);
        if (requester && requester->supervision_priority_child_pid == child_pid) {
            requester->supervision_priority_child_pid = -1;
        }
    }
    child->parent_pid = supervisor_pid;
    return 0;
}

int task_fill_supervision_events(int requester_pid, os_task_supervision_events_t* out) {
    task_t* parent;
    uint32_t i;
    if (!out) return OS_TASK_NOT_FOUND;
    memset(out, 0, sizeof(*out));
    parent = get_task_by_id(requester_pid);
    if (!parent) return OS_TASK_NOT_FOUND;
    out->generation = parent->supervision_event_generation;
    out->count = parent->supervision_event_count;
    for (i = 0U; i < out->count; i++) {
        uint32_t index = (parent->supervision_event_start + i) %
                         OS_TASK_SUPERVISION_EVENT_CAPACITY;
        out->entries[i] = parent->supervision_events[index];
    }
    return 0;
}

int task_ack_supervision_events(int requester_pid) {
    task_t* parent = get_task_by_id(requester_pid);
    if (!parent) return OS_TASK_NOT_FOUND;
    parent->supervision_event_start = 0U;
    parent->supervision_event_count = 0U;
    parent->supervision_event_generation++;
    if (parent->supervision_event_generation == 0U) parent->supervision_event_generation = 1U;
    return (int)parent->supervision_event_generation;
}

int task_observe_supervision_events(int requester_pid, uint32_t expected_generation,
                                    os_task_supervision_events_observation_t* out) {
    task_t* parent;
    int rc;
    if (!out) return OS_TASK_NOT_FOUND;
    memset(out, 0, sizeof(*out));
    parent = get_task_by_id(requester_pid);
    if (!parent) return OS_TASK_NOT_FOUND;
    out->generation = parent->supervision_event_generation;
    if (expected_generation != out->generation) return OS_TASK_HISTORY_STALE;
    rc = task_fill_supervision_events(requester_pid, &out->events);
    return rc;
}

int task_find_supervision_event(int requester_pid, uint32_t sequence,
                                os_task_supervision_event_t* out) {
    task_t* parent;
    uint32_t i;
    if (!out || sequence == 0U) return OS_TASK_NO_SUPERVISION_EVENT;
    memset(out, 0, sizeof(*out));
    parent = get_task_by_id(requester_pid);
    if (!parent) return OS_TASK_NOT_FOUND;
    for (i = 0U; i < parent->supervision_event_count; i++) {
        uint32_t index = (parent->supervision_event_start + i) %
                         OS_TASK_SUPERVISION_EVENT_CAPACITY;
        if (parent->supervision_events[index].sequence == sequence) {
            *out = parent->supervision_events[index];
            return 0;
        }
    }
    return OS_TASK_NO_SUPERVISION_EVENT;
}

int task_forget_supervision_event(int requester_pid, uint32_t sequence) {
    task_t* parent;
    os_task_supervision_event_t retained[OS_TASK_SUPERVISION_EVENT_CAPACITY];
    uint32_t i;
    uint32_t kept = 0U;
    int found = 0;
    if (sequence == 0U) return OS_TASK_NO_SUPERVISION_EVENT;
    parent = get_task_by_id(requester_pid);
    if (!parent) return OS_TASK_NOT_FOUND;
    for (i = 0U; i < parent->supervision_event_count; i++) {
        uint32_t index = (parent->supervision_event_start + i) %
                         OS_TASK_SUPERVISION_EVENT_CAPACITY;
        if (parent->supervision_events[index].sequence == sequence) {
            found = 1;
        } else {
            retained[kept++] = parent->supervision_events[index];
        }
    }
    if (!found) return OS_TASK_NO_SUPERVISION_EVENT;
    parent->supervision_event_start = 0U;
    parent->supervision_event_count = kept;
    for (i = 0U; i < kept; i++) parent->supervision_events[i] = retained[i];
    parent->supervision_event_generation++;
    if (parent->supervision_event_generation == 0U) parent->supervision_event_generation = 1U;
    return (int)kept;
}

int task_set_supervision_notify(int requester_pid, uint32_t enabled) {
    task_t* parent;
    if (enabled > 1U) return OS_TASK_BAD_NOTIFY;
    parent = get_task_by_id(requester_pid);
    if (!parent || parent->type != TASK_TYPE_USER || parent->state == TASK_TERMINATED) {
        return OS_TASK_NOT_FOUND;
    }
    parent->supervision_notify_enabled = enabled;
    return (int)enabled;
}

int task_set_supervision_notify_filter(int requester_pid, uint32_t mask) {
    task_t* parent;
    if ((mask & ~OS_TASK_SUPERVISION_NOTIFY_ALL) != 0U) return OS_TASK_BAD_NOTIFY_FILTER;
    parent = get_task_by_id(requester_pid);
    if (!parent || parent->type != TASK_TYPE_USER || parent->state == TASK_TERMINATED) {
        return OS_TASK_NOT_FOUND;
    }
    parent->supervision_notify_mask = mask;
    return 0;
}

int task_fill_supervision_notify_status(int requester_pid,
                                        os_task_supervision_notify_status_t* out) {
    task_t* parent;
    if (!out) return OS_TASK_NOT_FOUND;
    memset(out, 0, sizeof(*out));
    parent = get_task_by_id(requester_pid);
    if (!parent || parent->type != TASK_TYPE_USER || parent->state == TASK_TERMINATED) {
        return OS_TASK_NOT_FOUND;
    }
    out->enabled = parent->supervision_notify_enabled;
    out->mask = parent->supervision_notify_mask;
    return 0;
}

int task_update_supervision_watch(int requester_pid, int child_pid, uint32_t enabled) {
    task_t* parent;
    task_t* child;
    int index;
    if (enabled > 1U) return OS_TASK_BAD_WATCH;
    parent = get_task_by_id(requester_pid);
    if (!parent || parent->type != TASK_TYPE_USER || parent->state == TASK_TERMINATED) {
        return OS_TASK_NOT_FOUND;
    }
    if (enabled == 0U && child_pid == 0) {
        parent->supervision_watch_enabled = 0U;
        parent->supervision_watch_count = 0U;
        memset(parent->supervision_watch_pids, 0, sizeof(parent->supervision_watch_pids));
        return 0;
    }
    if (child_pid <= 0) return OS_TASK_BAD_WATCH;
    child = get_task_by_id(child_pid);
    if (!child) return OS_TASK_NOT_FOUND;
    if (child->type != TASK_TYPE_USER || child->state == TASK_TERMINATED ||
        child->parent_pid != requester_pid) return OS_TASK_NOT_CHILD;
    index = task_supervision_watch_index(parent, child_pid);
    if (enabled != 0U) {
        if (index >= 0) return (int)parent->supervision_watch_count;
        if (parent->supervision_watch_count >= OS_TASK_SUPERVISION_WATCH_CAPACITY) {
            return OS_TASK_WATCH_FULL;
        }
        parent->supervision_watch_pids[parent->supervision_watch_count++] = child_pid;
        parent->supervision_watch_enabled = 1U;
        return (int)parent->supervision_watch_count;
    }
    if (index < 0) return OS_TASK_NO_SUPERVISION_WATCH;
    task_remove_supervision_watch(parent, child_pid);
    return (int)parent->supervision_watch_count;
}

int task_fill_supervision_watch_status(int requester_pid,
                                       os_task_supervision_watch_status_t* out) {
    task_t* parent;
    uint32_t i;
    if (!out) return OS_TASK_NOT_FOUND;
    memset(out, 0, sizeof(*out));
    parent = get_task_by_id(requester_pid);
    if (!parent || parent->type != TASK_TYPE_USER || parent->state == TASK_TERMINATED) {
        return OS_TASK_NOT_FOUND;
    }
    out->enabled = parent->supervision_watch_enabled;
    out->count = parent->supervision_watch_count;
    for (i = 0U; i < out->count; i++) out->pids[i] = parent->supervision_watch_pids[i];
    return 0;
}

int task_fill_supervision_delivery_stats(int requester_pid,
                                         os_task_supervision_delivery_stats_t* out) {
    task_t* parent;
    if (!out) return OS_TASK_NOT_FOUND;
    memset(out, 0, sizeof(*out));
    parent = get_task_by_id(requester_pid);
    if (!parent || parent->type != TASK_TYPE_USER || parent->state == TASK_TERMINATED) {
        return OS_TASK_NOT_FOUND;
    }
    out->attempted = parent->supervision_delivery_attempted;
    out->delivered = parent->supervision_delivery_delivered;
    out->dropped = parent->supervision_delivery_dropped;
    return 0;
}

int task_ack_supervision_delivery_stats(int requester_pid) {
    task_t* parent = get_task_by_id(requester_pid);
    if (!parent || parent->type != TASK_TYPE_USER || parent->state == TASK_TERMINATED) {
        return OS_TASK_NOT_FOUND;
    }
    parent->supervision_delivery_attempted = 0U;
    parent->supervision_delivery_delivered = 0U;
    parent->supervision_delivery_dropped = 0U;
    return 0;
}

int task_replay_supervision_event(int requester_pid, uint32_t sequence) {
    task_t* parent;
    os_task_supervision_event_t event;
    os_ipc_payload_t payload;
    int rc;
    if (sequence == 0U) return OS_TASK_NO_SUPERVISION_EVENT;
    parent = get_task_by_id(requester_pid);
    if (!parent || parent->type != TASK_TYPE_USER || parent->state == TASK_TERMINATED) {
        return OS_TASK_NOT_FOUND;
    }
    rc = task_find_supervision_event(requester_pid, sequence, &event);
    if (rc != 0) return rc;
    if (os_task_make_supervision_event(&payload, &event) != 0) return OS_IPC_BAD_MESSAGE;
    parent->supervision_delivery_attempted++;
    rc = ipc_endpoint_send(&parent->ipc_endpoint, 0, &payload);
    if (rc == 0) {
        parent->supervision_delivery_delivered++;
        return 0;
    }
    parent->supervision_delivery_dropped++;
    return rc;
}

int task_set_supervision_priority(int requester_pid, int child_pid) {
    task_t* parent;
    task_t* child;
    parent = get_task_by_id(requester_pid);
    if (!parent || parent->type != TASK_TYPE_USER || parent->state == TASK_TERMINATED) {
        return OS_TASK_NOT_FOUND;
    }
    if (child_pid == 0) {
        parent->supervision_priority_child_pid = -1;
        return 0;
    }
    child = get_task_by_id(child_pid);
    if (!child) return OS_TASK_NOT_FOUND;
    if (child->type != TASK_TYPE_USER || child->state == TASK_TERMINATED ||
        child->parent_pid != requester_pid) return OS_TASK_NOT_CHILD;
    parent->supervision_priority_child_pid = child_pid;
    return child_pid;
}

int task_fill_supervision_priority_status(int requester_pid,
                                          os_task_supervision_priority_status_t* out) {
    task_t* parent;
    if (!out) return OS_TASK_NOT_FOUND;
    parent = get_task_by_id(requester_pid);
    if (!parent || parent->type != TASK_TYPE_USER || parent->state == TASK_TERMINATED) {
        return OS_TASK_NOT_FOUND;
    }
    out->child_pid = parent->supervision_priority_child_pid;
    return 0;
}

int task_set_supervision_notify_budget(int requester_pid, uint32_t limit) {
    task_t* parent = get_task_by_id(requester_pid);
    if (!parent || parent->type != TASK_TYPE_USER || parent->state == TASK_TERMINATED) return OS_TASK_NOT_FOUND;
    parent->supervision_notify_budget_limit = limit;
    parent->supervision_notify_budget_used = 0U;
    return 0;
}

int task_fill_supervision_notify_budget_status(int requester_pid,
                                                os_task_supervision_notify_budget_status_t* out) {
    task_t* parent = get_task_by_id(requester_pid);
    if (!out || !parent || parent->type != TASK_TYPE_USER || parent->state == TASK_TERMINATED) return OS_TASK_NOT_FOUND;
    out->limit = parent->supervision_notify_budget_limit;
    out->used = parent->supervision_notify_budget_used;
    return 0;
}

int task_fill_supervision_summary(int requester_pid, os_task_supervision_summary_t* out) {
    task_t* parent;
    task_t* t;
    if (!out) return OS_TASK_NOT_FOUND;
    memset(out, 0, sizeof(*out));
    parent = get_task_by_id(requester_pid);
    if (!parent || !task_queue) return OS_TASK_NOT_FOUND;
    t = task_queue;
    do {
        if (t->parent_pid == requester_pid && t->state != TASK_TERMINATED) {
            out->active_children++;
            if (t->state == TASK_SUSPENDED) out->suspended_children++;
        }
        t = t->next;
    } while (t && t != task_queue);
    out->generation = parent->supervision_event_generation;
    out->child_exit_count = parent->direct_child_exit_count;
    out->retained_events = parent->supervision_event_count;
    return 0;
}

int task_fill_direct_children(int requester_pid, os_task_children_t* out) {
    task_t* parent;
    task_t* t;
    uint32_t count = 0U;
    if (!out) return OS_TASK_NOT_FOUND;
    memset(out, 0, sizeof(*out));
    parent = get_task_by_id(requester_pid);
    if (!parent || !task_queue) return OS_TASK_NOT_FOUND;
    t = task_queue;
    do {
        if (t->parent_pid == requester_pid && count < OS_TASK_CHILD_CAPACITY) {
            int i = 0;
            out->entries[count].pid = t->id;
            out->entries[count].parent_pid = t->parent_pid;
            out->entries[count].state = map_task_state(t->state);
            out->entries[count].type = (t->type == TASK_TYPE_USER) ? OS_TASK_USER : OS_TASK_KERNEL;
            while (t->name[i] && i < OS_PROC_NAME_MAX - 1) {
                out->entries[count].name[i] = t->name[i];
                i++;
            }
            out->entries[count].name[i] = '\0';
            if (out->entries[count].name[0] == '\0') {
                out->entries[count].name[0] = '?';
                out->entries[count].name[1] = '\0';
            }
            count++;
        }
        t = t->next;
    } while (t && t != task_queue);
    out->count = count;
    return 0;
}

int task_fill_ps(os_proc_t* out, int max_n) {
    int count = 0;
    task_t* t;
    if (!out || max_n <= 0 || !task_queue) return 0;
    t = task_queue;
    do {
        int i;
        if (count >= max_n) break;
        out[count].pid = t->id;
        out[count].parent_pid = t->parent_pid;
        out[count].state = map_task_state(t->state);
        out[count].type = (t->type == TASK_TYPE_USER) ? OS_TASK_USER : OS_TASK_KERNEL;
        i = 0;
        while (t->name[i] && i < OS_PROC_NAME_MAX - 1) {
            out[count].name[i] = t->name[i];
            i++;
        }
        out[count].name[i] = '\0';
        if (out[count].name[0] == '\0') {
            out[count].name[0] = '?';
            out[count].name[1] = '\0';
        }
        count++;
        t = t->next;
    } while (t && t != task_queue);
    return count;
}

int task_fill_metrics(int pid, os_task_metrics_t* out) {
    task_t* t;
    uint32_t now;
    uint32_t run;
    if (!out) return OS_TASK_NOT_FOUND;
    memset(out, 0, sizeof(*out));
    t = get_task_by_id(pid);
    if (!t) return OS_TASK_NOT_FOUND;
    now = timer_get_ticks();
    run = t->run_ticks;
    if (t == current_task && t->state == TASK_RUNNING && now >= t->last_scheduled_ticks) {
        run += now - t->last_scheduled_ticks;
    }
    out->pid = t->id;
    out->parent_pid = t->parent_pid;
    out->state = map_task_state(t->state);
    out->type = (t->type == TASK_TYPE_USER) ? OS_TASK_USER : OS_TASK_KERNEL;
    out->priority = t->priority;
    out->created_ticks = t->created_ticks;
    out->age_ticks = now >= t->created_ticks ? now - t->created_ticks : 0U;
    out->run_ticks = run;
    out->switch_count = t->switch_count;
    out->direct_children = task_count_direct_children(t->id);
    return 0;
}

int task_fill_capacity(os_task_capacity_t* out) {
    uint32_t active;
    if (!out) return OS_TASK_NOT_FOUND;
    active = (uint32_t)get_task_count();
    out->active = active;
    out->capacity = OS_TASK_GLOBAL_CAPACITY;
    out->available = active < OS_TASK_GLOBAL_CAPACITY ? OS_TASK_GLOBAL_CAPACITY - active : 0U;
    return 0;
}

int task_set_priority(int requester_pid, int pid, uint32_t priority) {
    task_t* t;
    if (priority < OS_TASK_PRIORITY_LOW || priority > OS_TASK_PRIORITY_HIGH) {
        return OS_TASK_BAD_PRIORITY;
    }
    t = get_task_by_id(pid);
    if (!t) return OS_TASK_NOT_FOUND;
    if (requester_pid != t->id && requester_pid != t->parent_pid) {
        return OS_TASK_CONTROL_DENIED;
    }
    t->priority = priority;
    return 0;
}

int task_set_name(int requester_pid, int pid, const char* name) {
    task_t* t;
    int i = 0;
    if (!name || !name[0]) return OS_TASK_BAD_NAME;
    while (name[i]) {
        unsigned char c = (unsigned char)name[i];
        if (i >= OS_PROC_NAME_MAX - 1 || c < 32U || c > 126U) return OS_TASK_BAD_NAME;
        i++;
    }
    t = get_task_by_id(pid);
    if (!t) return OS_TASK_NOT_FOUND;
    if (requester_pid != t->id && requester_pid != t->parent_pid) {
        return OS_TASK_CONTROL_DENIED;
    }
    for (i = 0; name[i]; i++) t->name[i] = name[i];
    t->name[i] = '\0';
    return 0;
}

int task_get_child_result(int requester_pid, int child_pid, os_task_exit_result_t* out) {
    task_t* parent;
    if (!out) return OS_TASK_NO_CHILD_RESULT;
    parent = get_task_by_id(requester_pid);
    if (!parent) return OS_TASK_NOT_FOUND;
    if (parent->last_child_pid != child_pid) return OS_TASK_NO_CHILD_RESULT;
    out->child_pid = parent->last_child_pid;
    out->exit_code = parent->last_child_exit_code;
    out->reason = parent->last_child_exit_reason;
    out->finished_ticks = parent->last_child_finished_ticks;
    return 0;
}

int task_fill_child_result_history(int requester_pid, os_task_exit_history_t* out) {
    task_t* parent;
    uint32_t i;
    if (!out) return OS_TASK_NO_CHILD_RESULT;
    memset(out, 0, sizeof(*out));
    parent = get_task_by_id(requester_pid);
    if (!parent) return OS_TASK_NOT_FOUND;
    out->count = parent->child_exit_history_count;
    for (i = 0U; i < out->count; i++) {
        uint32_t index = (parent->child_exit_history_start + i) % OS_TASK_EXIT_HISTORY_CAPACITY;
        out->entries[i] = parent->child_exit_history[index];
    }
    return 0;
}

int task_ack_child_result_history(int requester_pid) {
    task_t* parent = get_task_by_id(requester_pid);
    if (!parent) return OS_TASK_NOT_FOUND;
    parent->last_child_pid = -1;
    parent->last_child_exit_code = 0;
    parent->last_child_exit_reason = 0U;
    parent->last_child_finished_ticks = 0U;
    parent->child_exit_history_start = 0U;
    parent->child_exit_history_count = 0U;
    parent->child_exit_history_generation++;
    if (parent->child_exit_history_generation == 0U) parent->child_exit_history_generation = 1U;
    return (int)parent->child_exit_history_generation;
}

int task_observe_child_result_history(int requester_pid, uint32_t expected_generation,
                                      os_task_exit_history_observation_t* out) {
    task_t* parent;
    int rc;
    if (!out) return OS_TASK_NO_CHILD_RESULT;
    memset(out, 0, sizeof(*out));
    parent = get_task_by_id(requester_pid);
    if (!parent) return OS_TASK_NOT_FOUND;
    out->generation = parent->child_exit_history_generation;
    if (expected_generation != out->generation) return OS_TASK_HISTORY_STALE;
    rc = task_fill_child_result_history(requester_pid, &out->history);
    return rc;
}

int task_find_child_result_history(int requester_pid, int child_pid, os_task_exit_result_t* out) {
    task_t* parent;
    uint32_t i;
    if (!out || child_pid < 0) return OS_TASK_NO_CHILD_RESULT;
    memset(out, 0, sizeof(*out));
    parent = get_task_by_id(requester_pid);
    if (!parent) return OS_TASK_NOT_FOUND;
    for (i = 0U; i < parent->child_exit_history_count; i++) {
        uint32_t index = (parent->child_exit_history_start + i) % OS_TASK_EXIT_HISTORY_CAPACITY;
        if (parent->child_exit_history[index].child_pid == child_pid) {
            *out = parent->child_exit_history[index];
            return 0;
        }
    }
    return OS_TASK_NO_CHILD_RESULT;
}

int task_forget_child_result_history(int requester_pid, int child_pid) {
    task_t* parent;
    os_task_exit_result_t retained[OS_TASK_EXIT_HISTORY_CAPACITY];
    uint32_t i;
    uint32_t kept = 0U;
    int found = 0;
    if (child_pid < 0) return OS_TASK_NO_CHILD_RESULT;
    parent = get_task_by_id(requester_pid);
    if (!parent) return OS_TASK_NOT_FOUND;
    for (i = 0U; i < parent->child_exit_history_count; i++) {
        uint32_t index = (parent->child_exit_history_start + i) % OS_TASK_EXIT_HISTORY_CAPACITY;
        os_task_exit_result_t entry = parent->child_exit_history[index];
        if (!found && entry.child_pid == child_pid) {
            found = 1;
            continue;
        }
        retained[kept++] = entry;
    }
    if (!found) return OS_TASK_NO_CHILD_RESULT;
    memset(parent->child_exit_history, 0, sizeof(parent->child_exit_history));
    for (i = 0U; i < kept; i++) parent->child_exit_history[i] = retained[i];
    parent->child_exit_history_start = 0U;
    parent->child_exit_history_count = kept;
    if (kept > 0U) {
        os_task_exit_result_t last = parent->child_exit_history[kept - 1U];
        parent->last_child_pid = last.child_pid;
        parent->last_child_exit_code = last.exit_code;
        parent->last_child_exit_reason = last.reason;
        parent->last_child_finished_ticks = last.finished_ticks;
    } else {
        parent->last_child_pid = -1;
        parent->last_child_exit_code = 0;
        parent->last_child_exit_reason = 0U;
        parent->last_child_finished_ticks = 0U;
    }
    parent->child_exit_history_generation++;
    if (parent->child_exit_history_generation == 0U) parent->child_exit_history_generation = 1U;
    return (int)parent->child_exit_history_generation;
}
