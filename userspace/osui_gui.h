/* osui_gui.h - bureau VGA 80x25 dirige par osui_runtime.c. */
#ifndef OSUI_GUI_H
#define OSUI_GUI_H

#include <stdint.h>

void osui_gui_render(uint16_t *cells);
void osui_gui_ascii_row(const uint16_t *cells, int y, char *dst, int max);
int osui_gui_feed_key(int key, char *out, int out_max);
void osui_gui_set_input(const char *text);
const char *osui_gui_input(void);
void osui_gui_run(void);

#endif
