/* osui_gui.h - boucle gui guest + bureau VBE QEMU (pas une fenetre HTML). */
#ifndef OSUI_GUI_H
#define OSUI_GUI_H

#include <stdint.h>
#include "os_syscalls.h"

#define OSUI_SNAP_MAX 3072

typedef int (*osui_program_eval_t)(const char *line, char *captured, int cap_max);

void osui_gui_set_program_eval(osui_program_eval_t fn);
void osui_gui_term_reset(void);
int osui_gui_term_count(void);
void osui_gui_term_at(int i, char *dst, int max);
void osui_gui_term_push(const char *text);

void osui_gui_render(uint16_t *cells);
void osui_gui_fill_scene(os_fb_scene_t *scene);
void osui_gui_ascii_row(const uint16_t *cells, int y, char *dst, int max);
void osui_gui_write_snap(char *dst, int max);
int osui_gui_feed_key(int key, char *out, int out_max);
void osui_gui_set_input(const char *text);
const char *osui_gui_input(void);
void osui_gui_run(void);

#endif
