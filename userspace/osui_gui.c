/* osui_gui.c - Boucle gui guest. Cerveau = osui_runtime.c.
 * Surface produit = framebuffer VBE QEMU (fenetre graphique, pas HTML).
 * Pas Chromium. Pas un LLM de production. Pas agent/.
 */

#include "osui_gui.h"
#include "osui_runtime.h"
#include "os_syscalls.h"

#define COLS OS_VGA_COLS
#define ROWS OS_VGA_ROWS

#define ATTR_TITLE 0x1F
#define ATTR_HINT  0x0B
#define ATTR_DIM   0x08
#define ATTR_DOCK  0x70

static char g_input[96];
static int g_ilen;
static int g_leave;
static int g_idle;

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

#ifndef KERNEL_TEST
static void sys_vga_blit_scene(const os_fb_scene_t *scene) {
    int rc;
    asm volatile("int $0x80" : "=a"(rc) : "a"(SYS_VGA_BLIT), "b"(scene));
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
static void sys_vga_blit_scene(const os_fb_scene_t *scene) { (void)scene; }
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

static void snap_add(char *dst, int max, int *pos, const char *s) {
    if (!s) return;
    while (*s && *pos < max - 1) dst[(*pos)++] = *s++;
    if (max > 0) dst[*pos] = 0;
}

static void snap_u(char *dst, int max, int *pos, unsigned v) {
    char buf[12];
    int n = 0;
    unsigned t = v;
    if (t == 0) {
        snap_add(dst, max, pos, "0");
        return;
    }
    while (t && n < 11) {
        buf[n++] = (char)('0' + (t % 10));
        t /= 10;
    }
    while (n--) {
        if (*pos < max - 1) dst[(*pos)++] = buf[n];
    }
    if (max > 0) dst[*pos] = 0;
}

void osui_gui_write_snap(char *dst, int max) {
    int p = 0;
    int i, n;
    const char *pane = osui_get_pane();
    if (!dst || max <= 0) return;
    dst[0] = 0;
    snap_add(dst, max, &p,
        "OSUI-SNAP chrome=qemu_fb display_surface=vbe_lfb llm=stub_echo "
        "us031=false python=false phase3=false guest_html_stage=false chat_mode=");
    snap_add(dst, max, &p, osui_get_chat_mode());
    snap_add(dst, max, &p, " pane=");
    snap_add(dst, max, &p, pane && pane[0] ? pane : "none");
    snap_add(dst, max, &p, " stage=");
    snap_add(dst, max, &p, osui_get_stage_mode());
    snap_add(dst, max, &p, " kind=");
    snap_add(dst, max, &p, osui_get_stage_kind());
    snap_add(dst, max, &p, " session=");
    snap_add(dst, max, &p, osui_get_session_id());
    snap_add(dst, max, &p, " chat_x=");
    snap_u(dst, max, &p, (unsigned)osui_get_chat_x());
    snap_add(dst, max, &p, " chat_y=");
    snap_u(dst, max, &p, (unsigned)osui_get_chat_y());
    snap_add(dst, max, &p, " nmsg=");
    n = osui_msg_count();
    snap_u(dst, max, &p, (unsigned)n);
    snap_add(dst, max, &p, " input=");
    snap_add(dst, max, &p, g_input);
    snap_add(dst, max, &p, "\n");
    for (i = 0; i < n; i++) {
        char msg[160];
        osui_msg_at(i, msg, (int)sizeof(msg));
        snap_add(dst, max, &p, "OSUI-MSG ");
        snap_u(dst, max, &p, (unsigned)i);
        snap_add(dst, max, &p, " ");
        snap_add(dst, max, &p, msg);
        snap_add(dst, max, &p, "\n");
    }
    snap_add(dst, max, &p, "OSUI-END\n");
}

void osui_gui_fill_scene(os_fb_scene_t *scene) {
    int i, n;
    const char *mode = osui_get_chat_mode();
    const char *pane = osui_get_pane();
    const char *kind = osui_get_stage_kind();
    const char *stmode = osui_get_stage_mode();
    const char *sid;
    int k;
    if (!scene) return;
    for (k = 0; k < (int)sizeof(*scene); k++) ((char *)scene)[k] = 0;
    scene->magic = OS_FB_MAGIC;
    scene->version = 1;
    scene->chat_mode = (mode && mode[0] == 'f') ? OS_FB_CHAT_FLOAT : OS_FB_CHAT_CENTER;
    scene->pane = OS_FB_PANE_NONE;
    if (pane) {
        if (scmp(pane, "browser") == 0) scene->pane = OS_FB_PANE_BROWSER;
        else if (scmp(pane, "shell") == 0) scene->pane = OS_FB_PANE_SHELL;
        else if (scmp(pane, "admin") == 0) scene->pane = OS_FB_PANE_ADMIN;
        else if (scmp(pane, "support") == 0) scene->pane = OS_FB_PANE_SUPPORT;
        else if (scmp(pane, "status") == 0) scene->pane = OS_FB_PANE_STATUS;
        else if (scmp(pane, "fs") == 0) scene->pane = OS_FB_PANE_FS;
    }
    scene->stage_mode = OS_FB_STAGE_REFLECTING;
    if (stmode && stmode[0] == 'a') scene->stage_mode = OS_FB_STAGE_ACTING;
    if (stmode && stmode[0] == 'p') scene->stage_mode = OS_FB_STAGE_PRESENTING;
    scene->stage_kind = OS_FB_KIND_PLAN;
    if (kind) {
        if (scmp(kind, "circle") == 0) scene->stage_kind = OS_FB_KIND_CIRCLE;
        else if (scmp(kind, "boxes") == 0) scene->stage_kind = OS_FB_KIND_BOXES;
        else if (scmp(kind, "graph") == 0) scene->stage_kind = OS_FB_KIND_GRAPH;
        else if (scmp(kind, "tree") == 0) scene->stage_kind = OS_FB_KIND_TREE;
        else if (scmp(kind, "clock") == 0) scene->stage_kind = OS_FB_KIND_CLOCK;
        else if (scmp(kind, "sim") == 0) scene->stage_kind = OS_FB_KIND_SIM;
    }
    scene->chat_x = (uint16_t)osui_get_chat_x();
    scene->chat_y = (uint16_t)osui_get_chat_y();
    sid = osui_get_session_id();
    scpy(scene->session, 12, sid ? sid : "s0001");
    scpy(scene->input, OS_FB_INPUT_LEN, g_input);
    n = osui_msg_count();
    if (n > OS_FB_MSG_MAX) n = OS_FB_MSG_MAX;
    scene->nmsg = (uint16_t)n;
    for (i = 0; i < n; i++) osui_msg_at(i, scene->messages[i], OS_FB_MSG_LEN);
}

void osui_gui_render(uint16_t *cells) {
    fill_rect(cells, 0, 0, COLS, ROWS, ' ', ATTR_HINT);
    put_text(cells, 0, 0,
        " MOHHDY OS  gui  chrome=qemu_fb  vbe_lfb  llm=stub_echo  us031=false ",
        ATTR_TITLE);
    put_text(cells, 0, 2,
        " Bureau produit = fenetre graphique QEMU (VBE 1024x768), pas HTML.",
        ATTR_HINT);
    put_text(cells, 0, 3,
        " Cerveau = osui_runtime.c. Pas un bureau ASCII. Pas Chromium.",
        ATTR_HINT);
    put_text(cells, 0, 5,
        " make run-gui  ouvre QEMU GTK ; tapez gui apres MOHHDY>",
        ATTR_HINT);
    put_text(cells, 0, 7,
        " guest_html_stage=false  python_facade=false  display_host=false",
        ATTR_DIM);
}

int osui_gui_feed_key(int key, char *out, int out_max) {
    if (out && out_max > 0) out[0] = 0;
    if (key == 0) {
        osui_stage_tick(out, out_max);
        return 0;
    }
    if (key == OS_VGA_KEY_ESC) {
        g_leave = 1;
        if (out) scpy(out, out_max, "osui gui exit chat_mode=center chrome=qemu_fb\n");
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
            if (out) scpy(out, out_max, "osui gui exit chat_mode=center chrome=qemu_fb\n");
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
    return 0;
}

static void emit_snap(void) {
    char snap[OSUI_SNAP_MAX];
    osui_gui_write_snap(snap, OSUI_SNAP_MAX);
    serial_puts(snap);
}

void osui_gui_run(void) {
    static os_fb_scene_t scene;
    char out[OSUI_OUT_MAX];
    int key;
    int stop;

    g_leave = 0;
    g_ilen = 0;
    g_idle = 0;
    g_input[0] = 0;
    osui_gui_ack_enter();
    serial_puts("osui gui live chrome=qemu_fb display_surface=vbe_lfb us031_complete=false\n");
    osui_gui_fill_scene(&scene);
    sys_vga_blit_scene(&scene);
    emit_snap();

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
        osui_gui_fill_scene(&scene);
        sys_vga_blit_scene(&scene);
        if (key != 0 || (g_idle++ % 8) == 0) emit_snap();
        if (stop) break;
        if (key == 0) yield_gui();
    }
    sys_vga_leave();
    serial_puts("osui gui exit chrome=text prompt=MOHHDY>\n");
}
