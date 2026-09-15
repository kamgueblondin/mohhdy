#ifndef TIMER_H
#define TIMER_H

#include <stdint.h>

// Fréquence par défaut du timer (en Hz)
#define TIMER_FREQUENCY 100

// Ports du PIT (Programmable Interval Timer)
#define PIT_CHANNEL_0   0x40
#define PIT_CHANNEL_1   0x41
#define PIT_CHANNEL_2   0x42
#define PIT_COMMAND     0x43

// Commandes du PIT
#define PIT_CHANNEL_0_SELECT    0x00
#define PIT_ACCESS_LOHI         0x30
#define PIT_MODE_SQUARE_WAVE    0x06

// Variables globales
extern uint32_t timer_ticks;
extern int timer_mode;

#include "task/task.h" // Pour cpu_state_t

// Fonctions publiques
void timer_init(uint32_t frequency);
void timer_handler(cpu_state_t* cpu);
uint32_t timer_get_ticks();
void timer_wait(uint32_t ticks);
void timer_update();
void software_timer_tick();

#endif

