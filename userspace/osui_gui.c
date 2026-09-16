/* osui_gui.c - Chrome VGA 80x25 (chat central / flottant, scene, panes).
 * Meme etat que osui_runtime.c. Pas HTML #ai-stage. Pas Chromium.
 */

#include "osui_gui.h"
#include "osui_runtime.h"
#include "os_syscalls.h"

#define COLS OS_VGA_COLS
#define ROWS OS_VGA_ROWS

#define ATTR_TITLE 0x1F
#define ATTR_STAGE 0x0B
#define ATTR_DOCK  0x70
#define ATTR_CHAT  0x1E
#define ATTR_PANE  0x0F
#define ATTR_FLOAT 0x2F
#define ATTR_INP   0x4F
#define ATTR_DIM   0x08

static char g_input[96];
static int g_ilen;
static int g_leave;

static int slen(const char *s) {
    int n = 0;
    if (!s) return 0;
    while (s[n]) n++;
    return n;
}

static void scpy(char *d, int max, const char *s) {
    int i = 0;
    if (!s) s = "";
    if (max <= 0) return;
    while (s[i] && i < max - 1) {
        d[i] = s[i];
        i++;
    }
    d[i] = 0;
}

static int scmp(const char *a, const char *b) {
    int i = 0;
    if (!a) a = "";
    if (!b) b = "";
    while (a[i] && b[i]) {
        if (a[i] != b[i]) return a[i] - b[i];
        i++;
    }
    return a[i] - b[i];
}

static void put_cell(uint16_t *cells, int x, int y, char ch, unsigned char attr) {
    if (x < 0 || y < 0 || x >= COLS || y >= ROWS) return;
    cells[y * COLS + x] = (uint16_t)(unsigned char)ch | ((uint16_t)attr << 8);
}

static void fill_rect(uint16_t *cells, int x, int y, int w, int h, char ch, unsigned char attr) {
    int r, c;
    for (r = y; r < y + h; r++) {
        for (c = x; c < x + w; c++) put_cell(cells, c, r, ch, attr);
    }
}

static void put_text(uint16_t *cells, int x, int y, const char *s, unsigned char attr) {
    int i = 0;
    if (!s) return;
    while (s[i]) {
        put_cell(cells, x + i, y, s[i], attr);
        i++;
    }
}

static void draw_box(uint16_t *cells, int x, int y, int w, int h, unsigned char attr) {
    int i;
    if (w < 2 || h < 2) return;
    put_cell(cells, x, y, '+', attr);
    put_cell(cells, x + w - 1, y, '+', attr);
    put_cell(cells, x, y + h - 1, '+', attr);
    put_cell(cells, x + w - 1, y + h - 1, '+', attr);
    for (i = 1; i < w - 1; i++) {
        put_cell(cells, x + i, y, '-', attr);
        put_cell(cells, x + i, y + h - 1, '-', attr);
    }
    for (i = 1; i < h - 1; i++) {
        put_cell(cells, x, y + i, '|', attr);
        put_cell(cells, x + w - 1, y + i, '|', attr);
    }
}

#ifndef KERNEL_TEST
static void sys_vga_blit_frame(const uint16_t *cells) {
    os_vga_frame_t frame;
    int i;
    int rc;
    for (i = 0; i < ROWS * COLS; i++) frame.cells[i] = cells[i];
    asm volatile("int $0x80" : "=a"(rc) : "a"(SYS_VGA_BLIT), "b"(&frame));
    (void)rc;
}

static void sys_vga_leave(void) {
    int rc;
    asm volatile("int $0x80" : "=a"(rc) : "a"(SYS_VGA_BLIT), "b"(0));
    (void)rc;
}

static int sys_getc_gui(void) {
    int c;
    asm volatile("int $0x80" : "=a"(c) : "a"(SYS_GETC));
    return c;
}

static void serial_puts(const char *s) {
    if (!s) return;
    while (*s) {
        asm volatile("int $0x80" : : "a"(SYS_PUTC), "b"(*s));
        s++;
    }
}

static void yield_gui(void) {
    asm volatile("int $0x80" : : "a"(SYS_YIELD));
}
#else
static void sys_vga_blit_frame(const uint16_t *cells) { (void)cells; }
static void sys_vga_leave(void) {}
static int sys_getc_gui(void) { return 0; }
static void serial_puts(const char *s) { (void)s; }
static void yield_gui(void) {}
#endif

void osui_gui_set_input(const char *text) {
    scpy(g_input, 96, text);
    g_ilen = slen(g_input);
}

const char *osui_gui_input(void) {
    return g_input;
}

void osui_gui_ascii_row(const uint16_t *cells, int y, char *dst, int max) {
    int x;
    if (!dst || max <= 0) return;
    dst[0] = 0;
    if (!cells || y < 0 || y >= ROWS) return;
    for (x = 0; x < COLS && x < max - 1; x++) {
        char ch = (char)(cells[y * COLS + x] & 0xFF);
        if (ch < 32) ch = ' ';
        dst[x] = ch;
    }
    dst[x] = 0;
}

void osui_gui_render(uint16_t *cells) {
    char row[80];
    char line[96];
    const char *mode = osui_get_chat_mode();
    const char *pane = osui_get_pane();
    const char *kind = osui_get_stage_kind();
    const char *stmode = osui_get_stage_mode();
    int r, i, n;
    int center;
    int chat_x, chat_y, chat_w, chat_h;
    int pane_open;

    fill_rect(cells, 0, 0, COLS, ROWS, ' ', ATTR_STAGE);
    put_text(cells, 0, 0, " MOHHDY OS  gui  llm=stub_echo  us031=false  python=false ", ATTR_TITLE);

    for (r = 0; r < OSUI_CANVAS_ROWS && r < ROWS - 3; r++) {
        osui_canvas_row(r, row, 80);
        put_text(cells, 1, 1 + r, row, ATTR_STAGE);
    }

    put_text(cells, 0, 22, " [Browser] [Shell] [Admin] [Support] [Status] [FS]   /help /center /console ", ATTR_DOCK);
    scpy(line, 96, " stage=");
    {
        int p = slen(line);
        scpy(line + p, 96 - p, stmode);
        p = slen(line);
        scpy(line + p, 96 - p, " kind=");
        p = slen(line);
        scpy(line + p, 96 - p, kind);
        p = slen(line);
        scpy(line + p, 96 - p, " chat=");
        p = slen(line);
        scpy(line + p, 96 - p, mode);
        p = slen(line);
        scpy(line + p, 96 - p, " pane=");
        p = slen(line);
        scpy(line + p, 96 - p, pane && pane[0] ? pane : "none");
    }
    put_text(cells, 0, 23, line, ATTR_DIM);
    put_text(cells, 0, 24, " tapez un prompt ou /shell /browser...  ESC ou console = retour MOHHDY> ", ATTR_DOCK);

    pane_open = pane && pane[0] && scmp(pane, "none") != 0;
    center = (scmp(mode, "float") != 0);

    if (pane_open) {
        draw_box(cells, 1, 2, 50, 19, ATTR_PANE);
        scpy(line, 96, " ");
        scpy(line + 1, 90, pane);
        put_text(cells, 3, 2, line, ATTR_PANE);
        if (scmp(pane, "browser") == 0) {
            put_text(cells, 3, 4, "Browser-OS  harness=dom_simulator", ATTR_PANE);
            put_text(cells, 3, 5, "us031_complete=false  chromium=false", ATTR_PANE);
            put_text(cells, 3, 7, "demo-app simulateur  #menu-toggle", ATTR_PANE);
            put_text(cells, 3, 8, "browser-click / type / pointer", ATTR_PANE);
            put_text(cells, 3, 10, "Pas un moteur Chromium.", ATTR_PANE);
        } else if (scmp(pane, "shell") == 0) {
            put_text(cells, 3, 4, "Shell Multiboot  prompt=MOHHDY>", ATTR_PANE);
            put_text(cells, 3, 5, "Vocabulaire Ring 3. Pas un bash Linux.", ATTR_PANE);
            put_text(cells, 3, 7, "help  ls  ai  vfs-list  os-status", ATTR_PANE);
        } else if (scmp(pane, "admin") == 0) {
            put_text(cells, 3, 4, "Admin  grant/revoke  takeover", ATTR_PANE);
            put_text(cells, 3, 5, "session_id=", ATTR_PANE);
            put_text(cells, 15, 5, osui_get_session_id(), ATTR_PANE);
        } else if (scmp(pane, "support") == 0) {
            put_text(cells, 3, 4, "Support  escalate / sessions", ATTR_PANE);
            put_text(cells, 3, 5, "session_id=", ATTR_PANE);
            put_text(cells, 15, 5, osui_get_session_id(), ATTR_PANE);
        } else if (scmp(pane, "status") == 0) {
            put_text(cells, 3, 4, "Statut instance  service=mohhdy-os", ATTR_PANE);
            put_text(cells, 3, 5, "llm=stub_echo  phase3_complete=false", ATTR_PANE);
        } else if (scmp(pane, "fs") == 0) {
            put_text(cells, 3, 4, "FS sandbox  write=false", ATTR_PANE);
            put_text(cells, 3, 6, "demo/hello.txt", ATTR_PANE);
            put_text(cells, 3, 7, "demo/invoice.txt", ATTR_PANE);
            put_text(cells, 3, 8, "data/notes.txt", ATTR_PANE);
        }
    }

    if (center) {
        chat_x = 16;
        chat_y = 5;
        chat_w = 48;
        chat_h = 13;
        draw_box(cells, chat_x, chat_y, chat_w, chat_h, ATTR_CHAT);
        put_text(cells, chat_x + 2, chat_y, " CHAT central ", ATTR_CHAT);
    } else {
        chat_x = osui_get_chat_x();
        chat_y = osui_get_chat_y();
        chat_w = 28;
        chat_h = 8;
        if (chat_x < 0) chat_x = 0;
        if (chat_y < 0) chat_y = 0;
        if (chat_x + chat_w > COLS) chat_x = COLS - chat_w;
        if (chat_y + chat_h > 22) chat_y = 22 - chat_h;
        fill_rect(cells, chat_x, chat_y, chat_w, chat_h, ' ', ATTR_FLOAT);
        draw_box(cells, chat_x, chat_y, chat_w, chat_h, ATTR_FLOAT);
        put_text(cells, chat_x + 1, chat_y, " CHAT float ", ATTR_FLOAT);
    }

    n = osui_msg_count();
    {
        int start = 0;
        int vis = chat_h - 4;
        if (vis < 1) vis = 1;
        if (n > vis) start = n - vis;
        for (i = start; i < n; i++) {
            char msg[80];
            osui_msg_at(i, msg, 80);
            put_text(cells, chat_x + 2, chat_y + 1 + (i - start), msg, center ? ATTR_CHAT : ATTR_FLOAT);
        }
    }

    fill_rect(cells, chat_x + 1, chat_y + chat_h - 2, chat_w - 2, 1, ' ', ATTR_INP);
    put_cell(cells, chat_x + 2, chat_y + chat_h - 2, '>', ATTR_INP);
    put_text(cells, chat_x + 4, chat_y + chat_h - 2, g_input, ATTR_INP);
}

int osui_gui_feed_key(int key, char *out, int out_max) {
    char mapped[8];
    if (out && out_max > 0) out[0] = 0;
    if (key == 0) {
        osui_stage_tick(out, out_max);
        return 0;
    }
    if (key == OS_VGA_KEY_ESC) {
        g_leave = 1;
        if (out) scpy(out, out_max, "osui gui exit chat_mode=center chrome=vga_text\n");
        return 1;
    }
    if (key == OS_VGA_KEY_LEFT) {
        osui_move_chat(-2, 0);
        return 0;
    }
    if (key == OS_VGA_KEY_RIGHT) {
        osui_move_chat(2, 0);
        return 0;
    }
    if (key == OS_VGA_KEY_UP) {
        osui_move_chat(0, -1);
        return 0;
    }
    if (key == OS_VGA_KEY_DOWN) {
        osui_move_chat(0, 1);
        return 0;
    }
    if (key == '\b') {
        if (g_ilen > 0) {
            g_ilen--;
            g_input[g_ilen] = 0;
        }
        return 0;
    }
    if (key == '\n' || key == '\r') {
        if (!g_input[0]) return 0;
        if (scmp(g_input, "console") == 0 || scmp(g_input, "gui-exit") == 0
            || scmp(g_input, "/console") == 0) {
            g_leave = 1;
            g_ilen = 0;
            g_input[0] = 0;
            if (out) scpy(out, out_max, "osui gui exit chat_mode=center chrome=vga_text\n");
            return 1;
        }
        osui_dispatch_line(g_input, out, out_max);
        if (osui_gui_should_leave()) {
            osui_gui_ack_leave();
            g_leave = 1;
            g_ilen = 0;
            g_input[0] = 0;
            return 1;
        }
        g_ilen = 0;
        g_input[0] = 0;
        return 0;
    }
    if (key >= 32 && key <= 126) {
        if (g_ilen < 90) {
            g_input[g_ilen++] = (char)key;
            g_input[g_ilen] = 0;
        }
        return 0;
    }
    (void)mapped;
    return 0;
}

void osui_gui_run(void) {
    static uint16_t frame[ROWS * COLS];
    char out[OSUI_OUT_MAX];
    int key;
    int stop;

    g_leave = 0;
    g_ilen = 0;
    g_input[0] = 0;
    osui_gui_ack_enter();
    serial_puts("osui gui live chrome=vga_desktop us031_complete=false\n");
    osui_gui_render(frame);
    sys_vga_blit_frame(frame);

    while (!g_leave) {
        key = sys_getc_gui();
        out[0] = 0;
        if (key == '\n' || key == '\r') {
            serial_puts("osui gui line=");
            serial_puts(g_input);
            serial_puts("\n");
        }
        stop = osui_gui_feed_key(key, out, OSUI_OUT_MAX);
        if (out[0]) serial_puts(out);
        osui_gui_render(frame);
        sys_vga_blit_frame(frame);
        if (stop) break;
        if (key == 0) yield_gui();
    }
    sys_vga_leave();
    serial_puts("osui gui exit chrome=vga_text prompt=MOHHDY>\n");
}
