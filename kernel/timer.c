#include "timer.h"
#include "task/task.h"

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
#define TIMER_PREEMPT_QUANTUM 20U
static uint32_t timer_last_preempt_tick;

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
    
    // Debug périodique pour tracer l'activité du timer
    if (timer_ticks % 100 == 0) {
        print_string_serial("TIMER_ALIVE: tick=");
        if (timer_ticks < 10000) {
            int thousands = timer_ticks / 1000;
            int hundreds = (timer_ticks / 100) % 10;
            if (thousands > 0) write_serial('0' + thousands);
            write_serial('0' + hundreds);
            write_serial('0');
            write_serial('0');
        } else {
            write_serial('9');
            write_serial('9');
            write_serial('+');
        }
        print_string_serial("\n");
    }
    
    // Changement explicite existant (lancement du shell / yield coopératif).
    if (g_reschedule_needed) {
        g_reschedule_needed = 0;
        schedule(cpu);
    }

    /* Préemption matérielle : uniquement entre deux cadres utilisateur valides.
     * Le garde Ring 3 évite le basculement depuis un syscall ou une IRQ noyau. */
    if (timer_user_frame(cpu) && current_task && current_task->type == TASK_TYPE_USER &&
        task_has_other_ready_user() &&
        timer_ticks - timer_last_preempt_tick >= TIMER_PREEMPT_QUANTUM) {
        timer_last_preempt_tick = timer_ticks;
        schedule(cpu);
    }
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
    timer_last_preempt_tick = 0U;

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
