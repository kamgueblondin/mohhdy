#ifndef KEYBOARD_H
#define KEYBOARD_H

#include <stdint.h>

// Initialise le clavier
void keyboard_init();

// Le handler appelé par l'ISR pour traiter une interruption clavier
void keyboard_interrupt_handler();

// Convertit un scancode en caractère ASCII.
char scancode_to_ascii(uint8_t scancode);

// Récupère un caractère depuis le buffer clavier (utilisé par le kernel)
char keyboard_getc(void);

// Lecture non-bloquante depuis le buffer ASCII du clavier
int kbd_get_char_nonblock(char *out);

/* One poll of the serial line and the keyboard buffer. 1 and *out set when
 * a character is waiting, 0 when the prompt should let other tasks run. */
int keyboard_poll_char(char* out);

// Fonction pour ajouter un caractère au buffer
void kbd_put_char(char c);

// Fonction pour ajouter une chaîne au buffer
void kbd_put_string(const char *s);

// Helper function pour afficher un byte en hexadécimal
void print_hex_byte_serial(uint8_t byte);

/* Lot 5 (US-001): Ring 0 fallback drain and events from the Ring 3 ps2driver. */
#include "os_syscalls.h"
uint32_t keyboard_ring0_drain(void);
void keyboard_apply_event(const os_input_event_t* e);

/* Lot 6 (US-001): modifier bits (OS_INPUT_MOD_*), driver loss resync and
 * mouse statistics for SYS_DEVICE_SERVICE. */
uint32_t keyboard_mods(void);
void keyboard_driver_lost(void);
void keyboard_mouse_stats(uint32_t* packets, int32_t* dx, int32_t* dy, uint32_t* buttons);

#endif

