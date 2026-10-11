#include "timer.h"
#include "kernel.h"
#include "task/task.h"
#include "vga_console.h"
#include "gfx_fb.h"
#include "input/usb_tablet.h"
#include "sched_quantum.h"
volatile uint32_t g_sched_quantum_ticks = TIMER_PREEMPT_QUANTUM;
volatile uint32_t g_sched_preemptions;
#include "syscall/syscall.h"

// Fonctions externes
extern void outb(unsigned short port, unsigned char data);
extern unsigned char inb(unsigned short port);
extern void print_string_serial(const char* str);
extern void pic_send_eoi(unsigned char irq);

// Variables globales
uint32_t timer_ticks = 0;
uint32_t software_timer_counter = 0;
int timer_mode = 0; // 0 = logiciel, 1 = matériel

/* La préemption IRQ0 est limitée aux retours Ring 3 : un cadre noyau issu d’un
 * syscall ne possède pas l’ESP/SS utilisateur requis par jump_to_task(). */

static int timer_user_frame(const cpu_state_t* cpu) {
    return cpu && (cpu->cs & 3U) == 3U && (cpu->ss & 3U) == 3U;
}

// Timer logiciel de secours
void software_timer_tick() {
    software_timer_counter++;
    
    // Simule un tick timer toutes les 100000 itérations (approximativement)
    if (software_timer_counter % 100000 == 0) {
        timer_ticks++;
        
        // Log périodique pour monitoring
        if (timer_ticks % 10 == 0) {
            print_string_serial("S"); // S pour Software timer
        }
        
        // PHASE 4: Réactivation progressive du multitâche
        // Appel conditionnel à l'ordonnanceur si activé
        if (timer_ticks > 50) { // Attendre 50 ticks avant d'activer le multitâche
            // schedule(); // À activer quand l'ordonnanceur sera stable
        }
    }
}

// Handler appelé par l'ISR du timer matériel
void timer_handler(cpu_state_t* cpu) {
    extern volatile int g_reschedule_needed;
    timer_ticks++;
    
    
    if (vga_desktop_active()) {
        usb_tablet_poll();
        gfx_fb_update_cursor();
    }

    /* SYS_IPC_RECV_WAIT deadlines: expired sleepers become READY here. */
    task_ipc_wait_tick(timer_ticks);

    /* Inventory item 4: a relayed GPT-2 job whose aiworker died or stalled
     * wakes its caller for the Ring 0 fallback. If only the idle kernel task
     * was running, switch now (same path as the boot hand-off). */
    if (syscall_ai_relay_watchdog(timer_ticks) && current_task &&
        current_task->type == TASK_TYPE_KERNEL) {
        schedule(cpu);
        return;
    }

    // Changement explicite existant (lancement du shell / yield coopératif).
    if (g_reschedule_needed) {
        g_reschedule_needed = 0;
        schedule(cpu);
    }

    /* Préemption matérielle : uniquement entre deux cadres utilisateur valides.
     * Le garde Ring 3 évite le basculement depuis un syscall ou une IRQ noyau. */
    if (current_task &&
        sched_quantum_preempt_due_q(timer_user_frame(cpu),
                                    current_task->type == TASK_TYPE_USER,
                                    task_has_other_ready_user(), timer_ticks,
                                    current_task->last_scheduled_ticks,
                                    g_sched_quantum_ticks)) {
        g_sched_preemptions++;
        schedule(cpu);
    }
}

/* SYS_SCHED_TUNE: 0 = query quantum, 1 = query preemption count,
 * SCHED_QUANTUM_MIN..MAX = set the quantum (power profiles, US-081). */
uint32_t timer_sched_tune(uint32_t arg) {
    if (arg == 0U) return g_sched_quantum_ticks;
    if (arg == 1U) return g_sched_preemptions;
    if (!sched_quantum_valid(arg)) return 0xFFFFFFFFU;
    g_sched_quantum_ticks = arg;
    return arg;
}

// Fonction unifiée pour obtenir les ticks (marche avec les deux modes)
uint32_t timer_get_ticks() {
    return timer_ticks;
}

// Fonction pour mettre à jour le timer (à appeler régulièrement)
void timer_update() {
    if (timer_mode == 0) {
        software_timer_tick();
    }
    // En mode matériel, les ticks sont gérés par l'ISR
}

// Initialise le timer matériel (PIT) pour le scheduling préemptif
void timer_init(uint32_t frequency) {
    timer_mode = 1; // Mode matériel

    // Le PIT (Programmable Interval Timer) utilise une fréquence de base de 1.193182 MHz
    uint32_t divisor = 1193182 / frequency;

    // Envoie l'octet de commande pour le canal 0
    // 0x36 = 00110110b -> Canal 0, LSB/MSB, Mode 2 (rate generator)
    outb(0x43, 0x36);

    // Envoie le diviseur
    outb(0x40, (uint8_t)(divisor & 0xFF));
    outb(0x40, (uint8_t)((divisor >> 8) & 0xFF));
}

// Attend un certain nombre de ticks
void timer_wait(uint32_t ticks) {
    uint32_t start_ticks = timer_ticks;
    while (timer_ticks < start_ticks + ticks) {
        asm volatile("hlt"); // Attend la prochaine interruption
    }
}
