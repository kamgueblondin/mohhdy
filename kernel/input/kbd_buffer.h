#ifndef KBD_BUFFER_H
#define KBD_BUFFER_H

#include <stdint.h>

// Buffer de scancodes pour le clavier
// Interface simple et thread-safe

// Ajouter un scancode au buffer (appelé par les interruptions)
void kbd_push_scancode(uint8_t scancode);

// Retirer un scancode du buffer (appelé par le code de traitement)
// Retourne 1 si succès, 0 si buffer vide
int kbd_pop_scancode(uint8_t *out);

// Vérifier si le buffer est vide
int kbd_buffer_empty(void);

// Vider complètement le buffer
void kbd_clear_buffer(void);

#endif // KBD_BUFFER_H
