/* osui_gui.h - boucle gui guest + instantane pour la surface HTML hote. */
#ifndef OSUI_GUI_H
#define OSUI_GUI_H

#include <stdint.h>

#define OSUI_SNAP_MAX 3072

void osui_gui_render(uint16_t *cells);
void osui_gui_ascii_row(const uint16_t *cells, int y, char *dst, int max);
void osui_gui_write_snap(char *dst, int max);
int osui_gui_feed_key(int key, char *out, int out_max);
void osui_gui_set_input(const char *text);
const char *osui_gui_input(void);
void osui_gui_run(void);

#endif
