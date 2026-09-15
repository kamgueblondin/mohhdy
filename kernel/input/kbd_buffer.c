#include "kbd_buffer.h"
#include <stdint.h>

// Buffer simple et efficace
#define KBD_BUF_SZ 128
static volatile uint8_t scancode_buffer[KBD_BUF_SZ];
static volatile unsigned int head = 0;
static volatile unsigned int tail = 0;

// Push scancode dans le buffer (appelé par les interruptions)
void kbd_push_scancode(uint8_t scancode) {
    // Calculer la prochaine position de head
    unsigned int next_head = (head + 1) & (KBD_BUF_SZ - 1);
    
    // Éviter l'overflow du buffer
    if (next_head != tail) {
        scancode_buffer[head] = scancode;
        __atomic_store_n(&head, next_head, __ATOMIC_RELEASE);
    }
    // Si buffer plein, ignorer silencieusement (pas de blocage)
}

// Pop scancode du buffer (appelé par le code qui traite les scancodes)
int kbd_pop_scancode(uint8_t *out) {
    // Lire la position actuelle de tail
    unsigned int current_tail = __atomic_load_n(&tail, __ATOMIC_ACQUIRE);
    unsigned int current_head = __atomic_load_n(&head, __ATOMIC_ACQUIRE);
    
    // Vérifier si buffer vide
    if (current_tail == current_head) {
        return 0; // Buffer vide
    }
    
    // Lire le scancode
    *out = scancode_buffer[current_tail];
    
    // Avancer tail
    unsigned int next_tail = (current_tail + 1) & (KBD_BUF_SZ - 1);
    __atomic_store_n(&tail, next_tail, __ATOMIC_RELEASE);
    
    return 1; // Succès
}

// Vérifier si le buffer est vide (non-bloquant)
int kbd_buffer_empty(void) {
    unsigned int current_tail = __atomic_load_n(&tail, __ATOMIC_ACQUIRE);
    unsigned int current_head = __atomic_load_n(&head, __ATOMIC_ACQUIRE);
    return (current_tail == current_head);
}

// Vider le buffer (pour reset)
void kbd_clear_buffer(void) {
    __atomic_store_n(&tail, 0, __ATOMIC_RELEASE);
    __atomic_store_n(&head, 0, __ATOMIC_RELEASE);
}
