#include "vga_console.h"

#ifndef KERNEL_TEST
extern void outb(unsigned short port, unsigned char data);
#endif

/* Geometry: 80x25 in text mode; the VBE terminal window may set another
 * one (vga_console_resize) and the content is reflowed to the new width.
 * wrap[] marks a row continued on the next one (soft wrap), so logical
 * lines can be rebuilt. */
static int g_cols = VGA_COLS, g_rows = VGA_ROWS;
static uint16_t live[VGA_MAX_ROWS][VGA_MAX_COLS];
static uint8_t live_wrap[VGA_MAX_ROWS];
static uint16_t hist[VGA_HIST_MAX][VGA_MAX_COLS];
static uint8_t hist_wrap[VGA_HIST_MAX];
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
static uint16_t frame[VGA_MAX_ROWS * VGA_MAX_COLS]; /* g_cols stride */
static int frame_cx, frame_cy, frame_cvis;

static void hw_put(int index, uint16_t cell) {
    frame[index] = cell;
}

static void hw_cursor(int x, int y, int visible) {
    if (x < 0) x = 0;
    if (y < 0) y = 0;
    if (x >= g_cols) x = g_cols - 1;
    if (y >= g_rows) y = g_rows - 1;
    frame_cx = x;
    frame_cy = y;
    frame_cvis = visible ? 1 : 0;
}

#ifdef KERNEL_TEST
uint16_t vga_test_fb[VGA_ROWS * VGA_COLS];
int vga_test_route_ring3;
int vga_test_ring0_paints;

static int vga_route_frame(void) { return vga_test_route_ring3; }

/* The 80x25 text page view of the frame (text mode geometry is 80x25). */
static uint16_t text_cell(int i) {
    int r = i / VGA_COLS, c = i % VGA_COLS;
    if (r < g_rows && c < g_cols) return frame[r * g_cols + c];
    return blank_cell;
}

void vga_console_paint_ring0(void) {
    int i;
    for (i = 0; i < VGA_ROWS * VGA_COLS; i++) vga_test_fb[i] = text_cell(i);
    vga_test_ring0_paints++;
}
#else
extern int vga_route_frame(void);
static volatile uint16_t* const vga_hw = (volatile uint16_t*)0xB8000;

static uint16_t text_cell(int i) {
    int r = i / VGA_COLS, c = i % VGA_COLS;
    if (r < g_rows && c < g_cols) return frame[r * g_cols + c];
    return blank_cell;
}

void vga_console_paint_ring0(void) {
    int i;
    uint16_t pos;
    for (i = 0; i < VGA_ROWS * VGA_COLS; i++) vga_hw[i] = text_cell(i);
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
        gfx_sink(frame, g_cols, g_rows, frame_cx, frame_cy, frame_cvis);
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
    if (cells) for (i = 0; i < VGA_ROWS * VGA_COLS; i++) cells[i] = text_cell(i);
    if (cx) *cx = (uint32_t)frame_cx;
    if (cy) *cy = (uint32_t)frame_cy;
    if (visible) *visible = (uint32_t)frame_cvis;
}

void vga_console_frame_copy_n(uint16_t *cells, int max_cells, int *cols, int *rows) {
    int i, n = g_cols * g_rows;
    if (cols) *cols = g_cols;
    if (rows) *rows = g_rows;
    if (!cells) return;
    if (n > max_cells) n = max_cells;
    for (i = 0; i < n; i++) cells[i] = frame[i];
}

int vga_console_cols(void) { return g_cols; }
int vga_console_rows(void) { return g_rows; }
void vga_console_get_cursor(int *x, int *y) { if (x) *x = cur_x; if (y) *y = cur_y; }
void vga_console_mark_wrap(int y) { if (y >= 0 && y < g_rows) live_wrap[y] = 1; }

static uint16_t hist_get(int oldest_index, int col) {
    int idx = (hist_head + oldest_index) % VGA_HIST_MAX;
    return hist[idx][col];
}

static void hist_push_row(const uint16_t* row, uint8_t wrap) {
    int idx;
    int col;

    if (hist_count < VGA_HIST_MAX) {
        idx = (hist_head + hist_count) % VGA_HIST_MAX;
        hist_count++;
    } else {
        idx = hist_head;
        hist_head = (hist_head + 1) % VGA_HIST_MAX;
    }
    for (col = 0; col < g_cols; col++) {
        hist[idx][col] = row[col];
    }
    hist_wrap[idx] = wrap;
}

static uint16_t combined_cell(int abs_row, int col) {
    if (abs_row < hist_count) {
        return hist_get(abs_row, col);
    }
    return live[abs_row - hist_count][col];
}

static void compose_visible(void) {
    int combined = hist_count + g_rows;
    int start = combined - g_rows - view_off;
    int row;
    int col;

    if (desktop_on) {
        return;
    }
    if (start < 0) start = 0;
    for (row = 0; row < g_rows; row++) {
        for (col = 0; col < g_cols; col++) {
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
            hw_put(row * g_cols + col, cell);
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
    g_cols = VGA_COLS;
    g_rows = VGA_ROWS;
    for (row = 0; row < VGA_MAX_ROWS; row++) {
        live_wrap[row] = 0;
        for (col = 0; col < VGA_MAX_COLS; col++) {
            live[row][col] = blank_cell;
        }
    }
    for (row = 0; row < VGA_HIST_MAX; row++) {
        hist_wrap[row] = 0;
        for (col = 0; col < VGA_MAX_COLS; col++) {
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
    if (x < 0 || y < 0 || x >= g_cols || y >= g_rows) {
        return;
    }
    if (view_off != 0) {
        view_off = 0;
    }
    cell = (uint16_t)(unsigned char)c | ((uint16_t)color << 8);
    live[y][x] = cell;
    compose_visible();
}

uint16_t vga_console_cell(int x, int y) {
    if (x < 0 || y < 0 || x >= g_cols || y >= g_rows) return 0;
    return live[y][x];
}

void vga_console_scroll(void) {
    int row;
    int col;

    hist_push_row(live[0], live_wrap[0]);
    for (row = 0; row < g_rows - 1; row++) {
        for (col = 0; col < g_cols; col++) {
            live[row][col] = live[row + 1][col];
        }
        live_wrap[row] = live_wrap[row + 1];
    }
    for (col = 0; col < g_cols; col++) {
        live[g_rows - 1][col] = blank_cell;
    }
    live_wrap[g_rows - 1] = 0;
    view_off = 0;
    compose_visible();
}

void vga_console_clear(char color) {
    int row;
    int col;

    blank_cell = (uint16_t)' ' | ((uint16_t)color << 8);
    for (row = 0; row < g_rows; row++) {
        live_wrap[row] = 0;
        for (col = 0; col < g_cols; col++) {
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

    if (x < 0 || y < 0 || x >= g_cols || y >= g_rows) {
        return 0;
    }
    if (desktop_on) {
        return live[y][x];
    }
    combined = hist_count + g_rows;
    start = combined - g_rows - view_off;
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
    if (g_cols != VGA_COLS || g_rows != VGA_ROWS) (void)vga_console_resize(VGA_COLS, VGA_ROWS);
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

/* Reflow: rebuild logical lines from the scrollback and the live rows
 * (wrap flags join soft wrapped rows, other rows lose trailing blanks),
 * then cut them again at the new width. The cursor keeps its place in its
 * line; the newest rows end up in the live grid, older ones in history. */
#define RF_ROWS (VGA_HIST_MAX + VGA_MAX_ROWS)
static uint16_t rf_cells[RF_ROWS * VGA_MAX_COLS];
static int rf_start[RF_ROWS], rf_len[RF_ROWS];
static uint16_t rf_out[RF_ROWS][VGA_MAX_COLS];
static uint8_t rf_wrap[RF_ROWS];

static const uint16_t *row_ptr(int abs_row, uint8_t *wrap) {
    if (abs_row < hist_count) {
        int idx = (hist_head + abs_row) % VGA_HIST_MAX;
        *wrap = hist_wrap[idx];
        return hist[idx];
    }
    *wrap = live_wrap[abs_row - hist_count];
    return live[abs_row - hist_count];
}

int vga_console_resize(int cols, int rows) {
    int used, r, nl = 0, pos = 0, open = 0, cur_line = -1, cur_off = 0, line_cells = 0;
    int total = 0, cur_row = 0, cur_col = 0, live_start, i, k;
    if (cols < 8) cols = 8;
    if (rows < 2) rows = 2;
    if (cols > VGA_MAX_COLS) cols = VGA_MAX_COLS;
    if (rows > VGA_MAX_ROWS) rows = VGA_MAX_ROWS;
    if (cols == g_cols && rows == g_rows) return 0;
    /* rows in use: up to the cursor or the last non blank live row */
    used = cur_y + 1;
    for (r = g_rows - 1; r >= used; r--) {
        int c, blank = 1;
        for (c = 0; c < g_cols; c++) if (live[r][c] != blank_cell) { blank = 0; break; }
        if (!blank) { used = r + 1; break; }
    }
    for (r = 0; r < hist_count + used; r++) {
        uint8_t wrap;
        const uint16_t *row = row_ptr(r, &wrap);
        int len = g_cols;
        if (!open) { rf_start[nl] = pos; line_cells = 0; open = 1; }
        if (!wrap) while (len > 0 && row[len - 1] == blank_cell) len--;
        if (r == hist_count + cur_y) {
            cur_line = nl;
            cur_off = line_cells + cur_x;
            if (len < cur_x) len = cur_x;
        }
        for (k = 0; k < len && pos < RF_ROWS * VGA_MAX_COLS; k++) rf_cells[pos++] = row[k];
        line_cells += len;
        if (!wrap || r == hist_count + used - 1) {
            rf_len[nl] = line_cells;
            nl++;
            open = 0;
            if (nl >= RF_ROWS) break;
        }
    }
    /* cut at the new width into a ring of the newest RF_ROWS rows */
    for (i = 0; i < nl; i++) {
        int n = (rf_len[i] + cols - 1) / cols;
        if (n < 1) n = 1;
        if (i == cur_line && cur_off / cols + 1 > n) n = cur_off / cols + 1;
        for (k = 0; k < n; k++) {
            int c, slot = total % RF_ROWS;
            for (c = 0; c < VGA_MAX_COLS; c++) {
                int src = k * cols + c;
                rf_out[slot][c] = (c < cols && src < rf_len[i]) ? rf_cells[rf_start[i] + src] : blank_cell;
            }
            rf_wrap[slot] = (uint8_t)(k < n - 1);
            total++;
        }
        if (i == cur_line) { cur_row = total - n + cur_off / cols; cur_col = cur_off % cols; }
    }
    if (cur_line < 0) { cur_row = total; cur_col = 0; }
    live_start = total - rows;
    if (live_start < 0) live_start = 0;
    if (cur_row < live_start) live_start = cur_row;
    if (cur_row >= live_start + rows) live_start = cur_row - rows + 1;
    if (live_start < total - RF_ROWS) live_start = total - RF_ROWS;
    g_cols = cols;
    g_rows = rows;
    hist_head = 0;
    hist_count = 0;
    for (r = live_start - VGA_HIST_MAX; r < live_start; r++) {
        if (r < 0 || r < total - RF_ROWS) continue;
        for (k = 0; k < VGA_MAX_COLS; k++) hist[hist_count][k] = rf_out[r % RF_ROWS][k];
        hist_wrap[hist_count] = rf_wrap[r % RF_ROWS];
        hist_count++;
    }
    for (r = 0; r < VGA_MAX_ROWS; r++) {
        int a = live_start + r;
        for (k = 0; k < VGA_MAX_COLS; k++)
            live[r][k] = (r < rows && a < total) ? rf_out[a % RF_ROWS][k] : blank_cell;
        live_wrap[r] = (uint8_t)((r < rows && a < total) ? rf_wrap[a % RF_ROWS] : 0);
    }
    cur_x = cur_col;
    cur_y = cur_row - live_start;
    view_off = 0;
    compose_visible();
    return 1;
}
