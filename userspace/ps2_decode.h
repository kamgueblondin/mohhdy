/* userspace/ps2_decode.h - PS/2 scan code set 1 and mouse packet decoder of
 * the Ring 3 ps2driver (Lot 5, US-001). Pure C, no I/O: the driver feeds
 * the bytes it read from port 0x60 and submits the events to the kernel. */
#ifndef MOHHDY_PS2_DECODE_H
#define MOHHDY_PS2_DECODE_H
#include <stdint.h>
#include "os_syscalls.h"

typedef struct {
    uint8_t shift, caps, e0;
    uint8_t mouse_cycle;
    uint8_t mouse[3];
    uint32_t bytes, ignored, mouse_packets, mouse_dropped;
} ps2_decoder_t;

void ps2_decoder_init(ps2_decoder_t* d);
/* One byte from port 0x60; aux = byte came from the mouse (status bit 5).
 * Appends at most one event to out[*n] (cap events). Returns 1 if an event
 * was produced. */
int ps2_decode_byte(ps2_decoder_t* d, uint8_t byte, int aux, os_input_event_t* out, uint32_t* n, uint32_t cap);
/* Character of a make code with the current Shift/Caps state (0: none). */
char ps2_map(const ps2_decoder_t* d, uint8_t make);
#endif
