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

#ifdef KERNEL_TEST
uint16_t vga_test_fb[VGA_ROWS * VGA_COLS];

static void hw_put(int index, uint16_t cell) {
    vga_test_fb[index] = cell;
}

static void hw_cursor(int x, int y, int visible) {
    (void)x;
    (void)y;
    (void)visible;
}
#else
static volatile uint16_t* const vga_hw = (volatile uint16_t*)0xB8000;

static void hw_put(int index, uint16_t cell) {
    vga_hw[index] = cell;
}

static void hw_cursor(int x, int y, int visible) {
    uint16_t pos;

    if (!visible) {
        outb(0x3D4, 0x0A);
        outb(0x3D5, 0x20);
        return;
    }
    if (x < 0) x = 0;
    if (y < 0) y = 0;
    if (x >= VGA_COLS) x = VGA_COLS - 1;
    if (y >= VGA_ROWS) y = VGA_ROWS - 1;
    pos = (uint16_t)(y * VGA_COLS + x);
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
    }
}

void vga_console_put_xy(char c, int x, int y, char color) {
    uint16_t cell;

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
    combined = hist_count + VGA_ROWS;
    start = combined - VGA_ROWS - view_off;
    if (start < 0) {
        start = 0;
    }
    return combined_cell(start + y, x);
}
