#include "vga_console.h"

#ifndef KERNEL_TEST
extern void outb(unsigned short port, unsigned char data);
#endif

static uint16_t live[VGA_ROWS][VGA_COLS];
static uint16_t hist[VGA_HIST_MAX][VGA_COLS];
static int hist_count;
static int hist_head;
static int view_off;
static int cur_x;
static int cur_y;
static uint16_t blank_cell;
static int desktop_on;

/* Lot 7 (US-001): every update renders into `frame` (cells and cursor);
 * present() then hands it to the Ring 3 vgadriver when one owns the console
 * (vga_route_frame, kernel glue in syscall.c) or paints it from Ring 0:
 * early boot before the claim, after a driver death, on a stall and after a
 * kernel exception. */
static uint16_t frame[VGA_ROWS * VGA_COLS];
static int frame_cx, frame_cy, frame_cvis;

static void hw_put(int index, uint16_t cell) {
    frame[index] = cell;
}

static void hw_cursor(int x, int y, int visible) {
    if (x < 0) x = 0;
    if (y < 0) y = 0;
    if (x >= VGA_COLS) x = VGA_COLS - 1;
    if (y >= VGA_ROWS) y = VGA_ROWS - 1;
    frame_cx = x;
    frame_cy = y;
    frame_cvis = visible ? 1 : 0;
}

#ifdef KERNEL_TEST
uint16_t vga_test_fb[VGA_ROWS * VGA_COLS];
int vga_test_route_ring3;
int vga_test_ring0_paints;

static int vga_route_frame(void) { return vga_test_route_ring3; }

void vga_console_paint_ring0(void) {
    int i;
    for (i = 0; i < VGA_ROWS * VGA_COLS; i++) vga_test_fb[i] = frame[i];
    vga_test_ring0_paints++;
}
#else
extern int vga_route_frame(void);
static volatile uint16_t* const vga_hw = (volatile uint16_t*)0xB8000;

void vga_console_paint_ring0(void) {
    int i;
    uint16_t pos;
    for (i = 0; i < VGA_ROWS * VGA_COLS; i++) vga_hw[i] = frame[i];
    if (!frame_cvis) {
        outb(0x3D4, 0x0A);
        outb(0x3D5, 0x20);
        return;
    }
    pos = (uint16_t)(frame_cy * VGA_COLS + frame_cx);
    outb(0x3D4, 0x0A);
    outb(0x3D5, 0x00);
    outb(0x3D4, 0x0B);
    outb(0x3D5, 0x0F);
    outb(0x3D4, 0x0F);
    outb(0x3D5, (unsigned char)(pos & 0xFF));
    outb(0x3D4, 0x0E);
    outb(0x3D5, (unsigned char)((pos >> 8) & 0xFF));
}
#endif

static vga_gfx_sink_t gfx_sink;

static void present(void) {
    if (gfx_sink) {                    /* the VBE terminal window paints it */
        gfx_sink(frame, frame_cx, frame_cy, frame_cvis);
        return;
    }
    if (vga_route_frame()) return;     /* the Ring 3 driver paints it */
    vga_console_paint_ring0();
}

static uint32_t vram_epoch;

uint32_t vga_console_vram_epoch(void) { return vram_epoch; }

/* The text cells in video RAM no longer match what was painted (the VBE
 * linear framebuffer shares that RAM). The Ring 3 driver repaints every
 * cell when the epoch changes; the Ring 0 path always repaints them. */
void vga_console_vram_lost(void) { vram_epoch++; }

void vga_console_frame_copy(uint16_t* cells, uint32_t* cx, uint32_t* cy, uint32_t* visible) {
    int i;
    if (cells) for (i = 0; i < VGA_ROWS * VGA_COLS; i++) cells[i] = frame[i];
    if (cx) *cx = (uint32_t)frame_cx;
    if (cy) *cy = (uint32_t)frame_cy;
    if (visible) *visible = (uint32_t)frame_cvis;
}

static uint16_t hist_get(int oldest_index, int col) {
    int idx = (hist_head + oldest_index) % VGA_HIST_MAX;
    return hist[idx][col];
}

static void hist_push_row(const uint16_t* row) {
    int idx;
    int col;

    if (hist_count < VGA_HIST_MAX) {
        idx = (hist_head + hist_count) % VGA_HIST_MAX;
        hist_count++;
    } else {
        idx = hist_head;
        hist_head = (hist_head + 1) % VGA_HIST_MAX;
    }
    for (col = 0; col < VGA_COLS; col++) {
        hist[idx][col] = row[col];
    }
}

static uint16_t combined_cell(int abs_row, int col) {
    if (abs_row < hist_count) {
        return hist_get(abs_row, col);
    }
    return live[abs_row - hist_count][col];
}

static void compose_visible(void) {
    int combined = hist_count + VGA_ROWS;
    int start = combined - VGA_ROWS - view_off;
    int row;
    int col;

    if (desktop_on) {
        return;
    }
    if (start < 0) start = 0;
    for (row = 0; row < VGA_ROWS; row++) {
        for (col = 0; col < VGA_COLS; col++) {
            uint16_t cell = combined_cell(start + row, col);
            if (view_off == 0 && row == cur_y && col == cur_x) {
                unsigned char attr = (unsigned char)(cell >> 8);
                unsigned char fg = (unsigned char)(attr & 0x0F);
                unsigned char bg = (unsigned char)((attr >> 4) & 0x07);
                unsigned char inv = (unsigned char)((fg << 4) | bg);
                if (inv == attr) {
                    inv = 0x70;
                }
                cell = (uint16_t)((cell & 0x00FF) | ((uint16_t)inv << 8));
            }
            hw_put(row * VGA_COLS + col, cell);
        }
    }
    if (view_off == 0) {
        hw_cursor(cur_x, cur_y, 1);
    } else {
        hw_cursor(0, 0, 0);
    }
    present();
}

void vga_console_init(char color) {
    int row;
    int col;

    blank_cell = (uint16_t)' ' | ((uint16_t)color << 8);
    hist_count = 0;
    hist_head = 0;
    view_off = 0;
    cur_x = 0;
    cur_y = 0;
    for (row = 0; row < VGA_ROWS; row++) {
        for (col = 0; col < VGA_COLS; col++) {
            live[row][col] = blank_cell;
        }
    }
    for (row = 0; row < VGA_HIST_MAX; row++) {
        for (col = 0; col < VGA_COLS; col++) {
            hist[row][col] = blank_cell;
        }
    }
    compose_visible();
}

void vga_console_view_live(void) {
    if (view_off != 0) {
        view_off = 0;
        compose_visible();
    } else {
        hw_cursor(cur_x, cur_y, 1);
        present();
    }
}

void vga_console_put_xy(char c, int x, int y, char color) {
    uint16_t cell;

    if (desktop_on) {
        return;
    }
    if (x < 0 || y < 0 || x >= VGA_COLS || y >= VGA_ROWS) {
        return;
    }
    if (view_off != 0) {
        view_off = 0;
    }
    cell = (uint16_t)(unsigned char)c | ((uint16_t)color << 8);
    live[y][x] = cell;
    compose_visible();
}

void vga_console_scroll(void) {
    int row;
    int col;

    hist_push_row(live[0]);
    for (row = 0; row < VGA_ROWS - 1; row++) {
        for (col = 0; col < VGA_COLS; col++) {
            live[row][col] = live[row + 1][col];
        }
    }
    for (col = 0; col < VGA_COLS; col++) {
        live[VGA_ROWS - 1][col] = blank_cell;
    }
    view_off = 0;
    compose_visible();
}

void vga_console_clear(char color) {
    int row;
    int col;

    blank_cell = (uint16_t)' ' | ((uint16_t)color << 8);
    for (row = 0; row < VGA_ROWS; row++) {
        for (col = 0; col < VGA_COLS; col++) {
            live[row][col] = blank_cell;
        }
    }
    view_off = 0;
    cur_x = 0;
    cur_y = 0;
    compose_visible();
}

void vga_console_set_cursor(int x, int y) {
    cur_x = x;
    cur_y = y;
    compose_visible();
}

int vga_console_view_up(int lines) {
    int max_off;

    if (desktop_on) {
        return 0;
    }
    if (lines <= 0) {
        return view_off;
    }
    max_off = hist_count;
    view_off += lines;
    if (view_off > max_off) {
        view_off = max_off;
    }
    compose_visible();
    return view_off;
}

int vga_console_view_down(int lines) {
    if (desktop_on) {
        return 0;
    }
    if (lines <= 0) {
        return view_off;
    }
    view_off -= lines;
    if (view_off < 0) {
        view_off = 0;
    }
    compose_visible();
    return view_off;
}

int vga_console_view_offset(void) {
    return view_off;
}

int vga_console_hist_count(void) {
    return hist_count;
}

uint16_t vga_console_visible_cell(int x, int y) {
    int combined;
    int start;

    if (x < 0 || y < 0 || x >= VGA_COLS || y >= VGA_ROWS) {
        return 0;
    }
    if (desktop_on) {
        return live[y][x];
    }
    combined = hist_count + VGA_ROWS;
    start = combined - VGA_ROWS - view_off;
    if (start < 0) {
        start = 0;
    }
    return combined_cell(start + y, x);
}

int vga_desktop_active(void) {
    return desktop_on;
}

void vga_console_set_gfx_sink(vga_gfx_sink_t fn) {
    gfx_sink = fn;
    if (fn) {
        desktop_on = 0;
        view_off = 0;
        compose_visible();
    }
}

int vga_console_gfx_sink_active(void) { return gfx_sink != 0; }

void vga_console_redraw(void) {
    desktop_on = 0;
    compose_visible();
}

void vga_desktop_set(int on) {
    if (on) {
        desktop_on = 1;
        view_off = 0;
        hw_cursor(0, 0, 0);
        present();
        return;
    }
    desktop_on = 0;
    vga_console_clear(0x07);
}

void vga_desktop_blit(const uint16_t *cells) {
    int row;
    int col;
    int index;

    if (!cells) {
        vga_desktop_set(0);
        return;
    }
    desktop_on = 1;
    view_off = 0;
    index = 0;
    for (row = 0; row < VGA_ROWS; row++) {
        for (col = 0; col < VGA_COLS; col++) {
            live[row][col] = cells[index];
            hw_put(index, cells[index]);
            index++;
        }
    }
    hw_cursor(0, 0, 0);
    present();
}
