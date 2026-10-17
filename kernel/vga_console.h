#ifndef VGA_CONSOLE_H
#define VGA_CONSOLE_H

#include <stdint.h>

#define VGA_COLS 80
#define VGA_ROWS 25
#define VGA_HIST_MAX 80

void vga_console_init(char color);
void vga_console_put_xy(char c, int x, int y, char color);
void vga_console_scroll(void);
void vga_console_clear(char color);
void vga_console_set_cursor(int x, int y);
int vga_console_view_up(int lines);
int vga_console_view_down(int lines);
void vga_console_view_live(void);
int vga_console_view_offset(void);
int vga_console_hist_count(void);
uint16_t vga_console_visible_cell(int x, int y);
int vga_desktop_active(void);
void vga_desktop_set(int on);
void vga_desktop_blit(const uint16_t *cells);
/* Lot 7 (US-001): the rendered frame, for the Ring 3 driver and Ring 0. */
void vga_console_frame_copy(uint16_t* cells, uint32_t* cx, uint32_t* cy, uint32_t* visible);
void vga_console_paint_ring0(void);

#ifdef KERNEL_TEST
extern uint16_t vga_test_fb[VGA_ROWS * VGA_COLS];
extern int vga_test_route_ring3;
extern int vga_test_ring0_paints;
#endif

#endif
