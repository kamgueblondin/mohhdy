/* osui_gui.c - Boucle gui guest. Cerveau = osui_runtime.c.
 * Surface produit = framebuffer VBE QEMU (fenetre graphique, pas HTML).
 * Pas Chromium. Pas un LLM de production. Pas agent/.
 */

#include "osui_gui.h"
#include "osui_runtime.h"
#include "osui_ide.h"
#include "webeng.h"
#include "webrt.h"
#include "os_syscalls.h"

#define COLS OS_VGA_COLS
#define ROWS OS_VGA_ROWS

#define ATTR_TITLE 0x1F
#define ATTR_HINT  0x0B
#define ATTR_DIM   0x08
#define ATTR_DOCK  0x70

#define GUI_TERM_MAX 16
#define GUI_TERM_LEN 64

static char g_input[96];
static int g_ilen;
static int g_leave;
static int g_idle;
/* Set when the desktop hands over to the console terminal window. */
static int g_term_req;
static char g_pending[96];
static osui_ide_t g_ide;
static osui_ide_load_t g_ide_load;
static osui_ide_save_t g_ide_save;
static osui_ide_exec_t g_ide_exec;
static char g_ide_buf[OSUI_IDE_TEXT_MAX + 1];
static char g_term[GUI_TERM_MAX][GUI_TERM_LEN];
static int g_term_i;
static int g_term_n;
/* Lot 6 (US-031): page rendered by the guest web engine in the web pane. */
static we_doc_t g_web;
static int g_web_active;
static int g_web_scroll;
static char g_web_file[40];
static int g_web_mobile;
static osui_mobile_op_t g_mobile_op;
static char g_web_src[OSUI_WEB_SRC_MAX];

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

static void scat(char *d, int max, const char *s) {
    int len = slen(d);
    if (len < max) scpy(d + len, max - len, s);
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

static int sncmp(const char *a, const char *b, int n) {
    int i;
    for (i = 0; i < n; i++) {
        if (a[i] != b[i]) return (unsigned char)a[i] - (unsigned char)b[i];
        if (!a[i]) return 0;
    }
    return 0;
}

static void web_num(char *d, int max, const char *label, int v) {
    char t[12];
    int k = 0, n = slen(d);
    unsigned u = (unsigned)(v < 0 ? -v : v);
    scpy(d + n, max - n, label);
    n = slen(d);
    if (v < 0 && n < max - 1) d[n++] = '-';
    do { t[k++] = (char)('0' + u % 10U); u /= 10U; } while (u && k < 11);
    while (k && n < max - 1) d[n++] = t[--k];
    d[n] = 0;
}

/* Lot 6: page pipeline. US-042 <template> components are expanded in the
 * HTML, then the engine parses; the collected CSS goes through US-040
 * @media flattening for the viewport and US-041 var() resolution before the
 * cascade. Counters are kept for the status line. */
static int g_web_components, g_web_media_kept, g_web_media_dropped, g_web_vars, g_web_vars_unresolved;
static char g_web_tmp[OSUI_WEB_SRC_MAX];
static char g_web_san[OSUI_WEB_SRC_MAX];
static int g_web_sanitized;
static int osui_web_pipeline(const char *src, int n) {
    int defined = 0, expanded = 0, kept = 0, dropped = 0, vdef = 0, unres = 0, m, rc, k;
    webrt_viewport_t vp;
    static char css1[WE_CSS_MAX], css2[WE_CSS_MAX];
    g_web_components = g_web_media_kept = g_web_media_dropped = g_web_vars = g_web_vars_unresolved = 0;
    g_web_sanitized = 0;
    /* Lot 7 (US-039): sanitizer first (script, iframe, object, embed, on*,
     * javascript: URLs, unsafe CSS), then the rest of the pipeline. */
    m = webrt_sanitize_page(src, g_web_san, OSUI_WEB_SRC_MAX, &g_web_sanitized);
    if (m < 0 || m >= OSUI_WEB_SRC_MAX) return -1;
    g_web_san[m] = 0; src = g_web_san; n = m;
    m = webrt_components_expand(src, g_web_tmp, OSUI_WEB_SRC_MAX, &defined, &expanded);
    if (m > 0 && expanded > 0) { src = g_web_tmp; n = m; g_web_components = expanded; }
    rc = we_parse(&g_web, src, n);
    if (rc != 0) return rc;
    if (g_web.css_len > 0) {
        g_web.css[g_web.css_len < WE_CSS_MAX ? g_web.css_len : WE_CSS_MAX - 1] = 0;
        vp.width = OSUI_WEB_VIEWPORT; vp.height = 480; vp.dark = 0;
        m = webrt_css_flatten_media(g_web.css, &vp, css1, WE_CSS_MAX, &kept, &dropped);
        if (m >= 0) {
            m = webrt_css_resolve_vars(css1, css2, WE_CSS_MAX, &vdef, &unres);
            if (m >= 0) {
                for (k = 0; k < m && k < WE_CSS_MAX - 1; k++) g_web.css[k] = css2[k];
                g_web.css_len = (uint32_t)k;
            }
        }
        g_web_media_kept = kept; g_web_media_dropped = dropped;
        g_web_vars = vdef; g_web_vars_unresolved = unres;
    }
    rc = we_style(&g_web);
    if (rc != 0) return rc;
    return we_layout(&g_web, OSUI_WEB_VIEWPORT);
}

we_doc_t *osui_gui_web_doc(void) { return &g_web; }
int osui_gui_web_active(void) { return g_web_active; }
int osui_gui_web_sanitized(void) { return g_web_sanitized; }

int osui_gui_web_load(const char *file, char *status, int max) {
    int n = -1, rc;
    if (status && max > 0) status[0] = 0;
    if (g_ide_load) n = g_ide_load(file, g_web_src, OSUI_WEB_SRC_MAX - 1);
    if (n < 0) { if (status) { scpy(status, max, "osui web error=not_found file="); scpy(status + slen(status), max - slen(status), file); } return -1; }
    g_web_src[n] = 0;
    rc = osui_web_pipeline(g_web_src, n);
    if (status) {
        scpy(status, max, rc == 0 ? "osui web render file=" : "osui web error=render file=");
        scpy(status + slen(status), max - slen(status), file);
        web_num(status, max, " nodes ", (int)g_web.perf.nodes);
        web_num(status, max, " rules ", (int)g_web.perf.rules);
        web_num(status, max, " boxes ", (int)g_web.perf.boxes);
        web_num(status, max, " lines ", (int)g_web.perf.lines);
        web_num(status, max, " items ", (int)g_web.perf.items);
        web_num(status, max, " dropped ", (int)g_web.perf.items_dropped);
        web_num(status, max, " page ", (int)g_web.perf.page_w);
        web_num(status, max, "x", (int)g_web.perf.page_h);
        web_num(status, max, " diag ", (int)g_web.ndiag);
        web_num(status, max, " components ", g_web_components);
        web_num(status, max, " media_kept ", g_web_media_kept);
        web_num(status, max, " media_dropped ", g_web_media_dropped);
        web_num(status, max, " vars ", g_web_vars);
        web_num(status, max, " sanitized ", g_web_sanitized);
    }
    return rc;
}

static void web_open_line(const char *line, char *out, int out_max) {
    const char *f = line + 4;
    char status[320];
    while (*f == ' ') f++;
    if (!*f || slen(f) >= (int)sizeof(g_web_file)) {
        if (out) scpy(out, out_max, "osui web error=usage web FILE\n");
        return;
    }
    if (osui_gui_web_load(f, status, (int)sizeof(status)) == 0) {
        scpy(g_web_file, (int)sizeof(g_web_file), f);
        g_web_active = 1;
        g_web_mobile = 0;
        g_web_scroll = 0;
    }
    if (out) { scpy(out, out_max, status); scpy(out + slen(out), out_max - slen(out), "\n"); }
}

void osui_gui_set_mobile_op(osui_mobile_op_t fn) { g_mobile_op = fn; }
int osui_gui_mobile_active(void) { return g_web_active && g_web_mobile; }

int osui_gui_show_items(const char *title, const os_fb_web_item_t *items, int n, int page_w, int page_h) {
    int k;
    if (!items || n < 0) return -1;
    if (n > OS_FB_WEB_ITEMS) n = OS_FB_WEB_ITEMS;
    for (k = 0; k < n; k++) g_web.items[k] = items[k];
    g_web.nitems = (uint32_t)n;
    g_web.perf.page_w = (uint32_t)page_w;
    g_web.perf.page_h = (uint32_t)page_h;
    g_web.perf.items = (uint32_t)n;
    g_web.perf.items_dropped = 0;
    scpy(g_web.title, (int)sizeof(g_web.title), title ? title : "");
    scpy(g_web_file, (int)sizeof(g_web_file), "mobile");
    g_web_active = 1;
    g_web_mobile = 1;
    g_web_scroll = 0;
    return n;
}

void osui_gui_set_ide_ops(osui_ide_load_t load, osui_ide_save_t save, osui_ide_exec_t exec) {
    g_ide_load = load;
    g_ide_save = save;
    g_ide_exec = exec;
}

int osui_gui_ide_active(void) { return g_ide.active; }

void osui_gui_ide_frame_row(int r, char *dst, int max) { osui_ide_frame_row(&g_ide, r, dst, max); }

void osui_gui_ide_output(const char *text) { if (g_ide.active) osui_ide_out_append(&g_ide, text); }

static void ide_out(char *out, int out_max, const char *a, const char *b) {
    if (!out || out_max <= 0) return;
    scpy(out, out_max, a);
    if (b) scpy(out + slen(out), out_max - slen(out), b);
    scpy(out + slen(out), out_max - slen(out), "\n");
}

/* "pm-ide FILE" typed on the desktop opens the editor full screen. */
static int ide_open_line(const char *line, char *out, int out_max) {
    const char *f = line + 6;
    int n = -1;
    while (*f == ' ') f++;
    if (!*f) { ide_out(out, out_max, "osui pm-ide error=usage pm-ide FILE", 0); return 1; }
    if (slen(f) >= (int)sizeof(g_ide.file)) { ide_out(out, out_max, "osui pm-ide error=name_too_long", 0); return 1; }
    if (g_ide_load) n = g_ide_load(f, g_ide_buf, OSUI_IDE_TEXT_MAX);
    osui_ide_open(&g_ide, f, n > 0 ? g_ide_buf : "", n > 0 ? n : 0);
    ide_out(out, out_max, n >= 0 ? "osui pm-ide open fullscreen file=" : "osui pm-ide open fullscreen new file=", f);
    return 0;
}

static void ide_action(int act, char *out, int out_max) {
    char msg[OSUI_IDE_ROW_LEN];
    int len;
    if (act == OSUI_IDE_QUIT) {
        ide_out(out, out_max, "osui pm-ide quit file=", g_ide.file);
        g_ide.active = 0;
        return;
    }
    len = osui_ide_text(&g_ide, g_ide_buf, (int)sizeof(g_ide_buf));
    if (len < 0) { osui_ide_set_status(&g_ide, "buffer too large"); return; }
    if (act == OSUI_IDE_SAVE) {
        if (!g_ide_save || g_ide_save(g_ide.file, g_ide_buf, len) != 0) {
            osui_ide_set_status(&g_ide, "save failed");
            ide_out(out, out_max, "osui pm-ide save error file=", g_ide.file);
            return;
        }
        g_ide.modified = 0;
        scpy(msg, (int)sizeof(msg), "saved ");
        scpy(msg + slen(msg), (int)sizeof(msg) - slen(msg), g_ide.file);
        osui_ide_set_status(&g_ide, msg);
        ide_out(out, out_max, "osui pm-ide saved file=", g_ide.file);
        return;
    }
    msg[0] = 0;
    if (!g_ide_exec) {
        osui_ide_set_status(&g_ide, "no PromptMessage VM in this binary");
        ide_out(out, out_max, "osui pm-ide exec=missing", 0);
        return;
    }
    if (act == OSUI_IDE_RUN) osui_ide_out_clear(&g_ide);
    (void)g_ide_exec(act, g_ide.file, g_ide_buf, len, msg, (int)sizeof(msg));
    osui_ide_set_status(&g_ide, msg);
    if (act == OSUI_IDE_RUN) {
        char line[OSUI_IDE_ROW_LEN + 16];
        if (g_ide.out_total == 0) osui_ide_out_append(&g_ide, "(no output)");
        else g_ide.out_visible = 1;
        scpy(line, (int)sizeof(line), msg);
        scpy(line + slen(line), (int)sizeof(line) - slen(line), " output ");
        {
            char num[12]; int k = 0, v = g_ide.out_total, j; char t[12];
            do { t[k++] = (char)('0' + v % 10); v /= 10; } while (v && k < 11);
            for (j = 0; j < k; j++) num[j] = t[k - 1 - j];
            num[k] = 0;
            scpy(line + slen(line), (int)sizeof(line) - slen(line), num);
        }
        ide_out(out, out_max, "osui pm-ide run ", line);
        return;
    }
    ide_out(out, out_max, "osui pm-ide check ", msg);
}

int osui_gui_term_requested(void) { return g_term_req; }

int osui_gui_take_pending(char *dst, int max) {
    int had = g_pending[0] != 0;
    if (dst && max > 0) scpy(dst, max, g_pending);
    g_pending[0] = 0;
    g_term_req = 0;
    return had;
}

/* Hand the screen to the console: the desktop loop ends, the shell goes
 * back to its own prompt (same line editor, same commands) and the kernel
 * draws the console in a VBE window. The typed line, if any, is run there. */
static int request_terminal(const char *line, char *out, int out_max) {
    g_term_req = 1;
    scpy(g_pending, (int)sizeof(g_pending), line ? line : "");
    if (scmp(osui_get_pane(), "shell") == 0) osui_dispatch_line("/close", 0, 0);
    g_leave = 1;
    g_ilen = 0;
    g_input[0] = 0;
    if (out) {
        scpy(out, out_max, "osui gui term chrome=vbe_term same_console=true line=");
        scat(out, out_max, g_pending);
        scat(out, out_max, "\n");
    }
    return 1;
}

void osui_gui_term_reset(void) {
    g_term_i = 0;
    g_term_n = 0;
}

int osui_gui_term_count(void) {
    return g_term_n;
}

void osui_gui_term_at(int i, char *dst, int max) {
    int slot;
    if (!dst || max <= 0) return;
    dst[0] = 0;
    if (i < 0 || i >= g_term_n) return;
    slot = (g_term_i + i) % GUI_TERM_MAX;
    scpy(dst, max, g_term[slot]);
}

void osui_gui_term_push(const char *text) {
    char line[GUI_TERM_LEN];
    int n = 0;
    int ansi = 0; /* the line had a color code (ESC [ ... letter) */
    if (!text) return;
    while (*text) {
        if (*text == 27 && text[1] == '[') {
            /* Shell color codes are not text: drop the whole sequence. */
            text += 2;
            while (*text && !(*text >= 0x40 && *text <= 0x7E)) text++;
            if (*text) text++;
            ansi = 1;
            continue;
        }
        if (*text == '\n' || n == GUI_TERM_LEN - 1) {
            line[n] = 0;
            if (n > 0 || (*text == '\n' && !ansi)) {
                int slot;
                if (g_term_n < GUI_TERM_MAX) {
                    slot = (g_term_i + g_term_n) % GUI_TERM_MAX;
                    g_term_n++;
                } else {
                    slot = g_term_i;
                    g_term_i = (g_term_i + 1) % GUI_TERM_MAX;
                }
                scpy(g_term[slot], GUI_TERM_LEN, line);
            }
            n = 0;
            if (*text == '\n') {
                ansi = 0;
                text++;
                continue;
            }
        }
        if (*text >= 32 && *text <= 126) line[n++] = *text;
        if (*text) text++;
    }
    if (n > 0) {
        int slot;
        line[n] = 0;
        if (g_term_n < GUI_TERM_MAX) {
            slot = (g_term_i + g_term_n) % GUI_TERM_MAX;
            g_term_n++;
        } else {
            slot = g_term_i;
            g_term_i = (g_term_i + 1) % GUI_TERM_MAX;
        }
        scpy(g_term[slot], GUI_TERM_LEN, line);
    }
}

static void first_token(const char *s, char *d, int max) {
    int n = 0;
    if (!d || max <= 0) return;
    d[0] = 0;
    if (!s) return;
    while (*s == ' ' || *s == '\t') s++;
    while (*s && *s != ' ' && *s != '\t' && n < max - 1) d[n++] = *s++;
    d[n] = 0;
}

static int token_ai(const char *s) {
    while (s && (*s == ' ' || *s == '\t')) s++;
    if (!s || s[0] != 'a' || s[1] != 'i') return 0;
    return s[2] == 0 || s[2] == ' ' || s[2] == '\t' || s[2] == '-';
}

void osui_gui_browser_inspect(void) {
    osui_gui_term_push("Browser-OS Native Panel : FS-as-web, containerized tabs, isolated storage");
    osui_gui_term_push("Tab container: active_origin=self allowed=self,http://127.0.0.1,p2p://node");
    osui_gui_term_push("DOM Tree Sandbox & JS Eval: ready (dom_simulator)");
    osui_gui_term_push("Performance Metrics: web-perf & browser-telemetry perf active");
}

void osui_gui_vfs_inspect(const char *path) {
    char line[128];
    scpy(line, (int)sizeof(line), "VFS Sandbox Inspection (FS-as-web) : ");
    if (path && path[0]) {
        scat(line, (int)sizeof(line), path);
    } else {
        scat(line, (int)sizeof(line), "initrd/, overlay/, fat16/, fat32/");
    }
    osui_gui_term_push(line);
    osui_gui_term_push("Mount points: /initrd (ro), /overlay (rw), /fat16 (rw), /fat32 (ro)");
    osui_gui_term_push("Sandbox files: /etc/pm_ca.pub, /web_app.json, /demo/hello.txt");
    osui_gui_term_push("Nodes: html, head, body, div#app, button#menu-toggle, form#invoice-form");
}

static int is_word(const char *s, const char *w) {
    int n = slen(w);
    return sncmp(s, w, n) == 0 && (s[n] == 0 || s[n] == ' ' || s[n] == '\t');
}

/* One decision point for a line typed on the desktop.
 * HOST: needs the GUI loop itself (leave, web pane, editor). mobile-ui is
 *       caught before when the mobile shell is linked.
 * SHELL: the desktop gives the screen to the console terminal window and
 *        the line runs at the real shell prompt (userspace/shell.c): one
 *        line editor, one command table, one output path.
 * OSUI: chat and other panes keep the OS-UI command line. */
int osui_gui_route_line(const char *line, const char *pane) {
    if (!line) return OSUI_GUI_ROUTE_OSUI;
    while (*line == ' ' || *line == '\t') line++;
    if (scmp(line, "console") == 0 || scmp(line, "gui-exit") == 0 || scmp(line, "/console") == 0
        || is_word(line, "web") || is_word(line, "pm-ide"))
        return OSUI_GUI_ROUTE_HOST;
    if (line[0] == '!' || token_ai(line)) return OSUI_GUI_ROUTE_SHELL;
    if (pane && scmp(pane, "shell") == 0) return OSUI_GUI_ROUTE_SHELL;
    return OSUI_GUI_ROUTE_OSUI;
}

static void seed_browser_term(void) {
    if (g_term_n > 0) return;
    osui_gui_browser_inspect();
}

static void seed_fs_term(void) {
    if (g_term_n > 0) return;
    osui_gui_vfs_inspect(0);
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

static int sys_vga_term(void) {
    int rc;
    asm volatile("int $0x80" : "=a"(rc) : "a"(SYS_VGA_BLIT), "b"(1));
    return rc;
}

static void sys_vga_leave(void) {
    int rc;
    asm volatile("int $0x80" : "=a"(rc) : "a"(SYS_VGA_BLIT), "b"(0));
    (void)rc;
}

static int sys_getc_gui(void) {
    int c;
    /* ebx=0 keeps the blocking read. The shell passes ebx=1. */
    asm volatile("int $0x80" : "=a"(c) : "a"(SYS_GETC), "b"(0));
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
static int sys_vga_term(void) { return 0; }
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
        "OSUI-SNAP chrome=qemu_fb display_surface=vbe_lfb llm=");
    snap_add(dst, max, &p, osui_get_stage_llm());
    snap_add(dst, max, &p, " ai=");
    snap_add(dst, max, &p, osui_get_ai_state());
    snap_add(dst, max, &p,
        " us031=true python=false phase3=false guest_html_stage=false chat_mode=");
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
    snap_add(dst, max, &p, " nterm=");
    snap_u(dst, max, &p, (unsigned)osui_gui_term_count());
    if (osui_gui_term_count() > 0) {
        char last[80];
        osui_gui_term_at(osui_gui_term_count() - 1, last, (int)sizeof(last));
        snap_add(dst, max, &p, " term=");
        snap_add(dst, max, &p, last);
    }
    snap_add(dst, max, &p, " input=");
    snap_add(dst, max, &p, g_input);
    if (g_ide.active) {
        char row[OSUI_IDE_ROW_LEN];
        snap_add(dst, max, &p, " ide=");
        snap_add(dst, max, &p, g_ide.file);
        snap_add(dst, max, &p, " ide_ln=");
        snap_u(dst, max, &p, (unsigned)(g_ide.row + 1));
        snap_add(dst, max, &p, " ide_col=");
        snap_u(dst, max, &p, (unsigned)(g_ide.col + 1));
        snap_add(dst, max, &p, " ide_lines=");
        snap_u(dst, max, &p, (unsigned)g_ide.nlines);
        snap_add(dst, max, &p, " ide_mod=");
        snap_u(dst, max, &p, (unsigned)g_ide.modified);
        snap_add(dst, max, &p, " ide_out=");
        snap_u(dst, max, &p, (unsigned)g_ide.out_total);
        snap_add(dst, max, &p, " ide_pane=");
        snap_u(dst, max, &p, (unsigned)g_ide.out_visible);
        snap_add(dst, max, &p, " ide_tabs=");
        snap_u(dst, max, &p, (unsigned)g_ide.completions);
        snap_add(dst, max, &p, "\nOSUI-IDE status=");
        snap_add(dst, max, &p, g_ide.status);
        osui_ide_frame_row(&g_ide, 1 + g_ide.row - g_ide.top, row, (int)sizeof(row));
        snap_add(dst, max, &p, " cursor_row=");
        snap_add(dst, max, &p, row);
    }
    if (osui_get_media_active()) {
        snap_add(dst, max, &p, " media=");
        snap_add(dst, max, &p, osui_get_media_paused() ? "paused" : "playing");
        snap_add(dst, max, &p, ":");
        snap_add(dst, max, &p, osui_get_media_title());
    }
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
    scene->version = 5; /* 3: term_attr (pm-ide syntax classes), 4: web display list, 5: llm */
    scene->chat_mode = (mode && mode[0] == 'f') ? OS_FB_CHAT_FLOAT : OS_FB_CHAT_CENTER;
    scene->pane = OS_FB_PANE_NONE;
    if (pane) {
        if (scmp(pane, "browser") == 0) scene->pane = OS_FB_PANE_BROWSER;
        else if (scmp(pane, "shell") == 0) scene->pane = OS_FB_PANE_SHELL;
        else if (scmp(pane, "admin") == 0) scene->pane = OS_FB_PANE_ADMIN;
        else if (scmp(pane, "support") == 0) scene->pane = OS_FB_PANE_SUPPORT;
        else if (scmp(pane, "status") == 0) scene->pane = OS_FB_PANE_STATUS;
        else if (scmp(pane, "fs") == 0) scene->pane = OS_FB_PANE_FS;
        else if (scmp(pane, "p2p") == 0) scene->pane = OS_FB_PANE_P2P;
        else if (scmp(pane, "collab") == 0) scene->pane = OS_FB_PANE_COLLAB;
        else if (scmp(pane, "prod") == 0) scene->pane = OS_FB_PANE_PROD;
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
        else if (scmp(kind, "media") == 0) scene->stage_kind = OS_FB_KIND_MEDIA;
    }
    if (osui_get_media_active()) {
        scene->stage_kind = OS_FB_KIND_MEDIA;
        scene->reserved = (uint8_t)((1U << 7) | (osui_get_media_paused() ? (1U << 6) : 0U) | (osui_get_media_ticks() & 0x3FU));
    } else {
        scene->reserved = 0U;
    }
    scene->chat_x = (uint16_t)osui_get_chat_x();
    scene->chat_y = (uint16_t)osui_get_chat_y();
    sid = osui_get_session_id();
    scpy(scene->session, 12, sid ? sid : "s0001");
    scpy(scene->input, OS_FB_INPUT_LEN, g_input);
    scpy(scene->llm, OS_FB_LLM_LEN, osui_get_stage_llm());
    n = osui_msg_count();
    {
        /* The chat window shows the newest messages, not the oldest. */
        int first = n > OS_FB_MSG_MAX ? n - OS_FB_MSG_MAX : 0;
        n -= first;
        scene->nmsg = (uint16_t)n;
        for (i = 0; i < n; i++) osui_msg_at(first + i, scene->messages[i], OS_FB_MSG_LEN);
    }
    n = osui_gui_term_count();
    if (n > OS_FB_TERM_MAX) {
        int skip = n - OS_FB_TERM_MAX;
        for (i = 0; i < OS_FB_TERM_MAX; i++)
            osui_gui_term_at(skip + i, scene->term[i], OS_FB_TERM_LEN);
        n = OS_FB_TERM_MAX;
    } else {
        for (i = 0; i < n; i++) osui_gui_term_at(i, scene->term[i], OS_FB_TERM_LEN);
    }
    scene->nterm = (uint8_t)n;
    scene->nweb = 0;
    if (g_web_active) {
        uint32_t k;
        scene->pane = OS_FB_PANE_WEB;
        scene->nweb = (uint16_t)g_web.nitems;
        scene->web_page_w = (uint16_t)g_web.perf.page_w;
        scene->web_page_h = (uint16_t)g_web.perf.page_h;
        scene->web_scroll = (uint16_t)g_web_scroll;
        scpy(scene->web_title, OS_FB_TERM_LEN, "web ");
        scpy(scene->web_title + slen(scene->web_title), OS_FB_TERM_LEN - slen(scene->web_title), g_web_file);
        if (g_web.title[0]) {
            scpy(scene->web_title + slen(scene->web_title), OS_FB_TERM_LEN - slen(scene->web_title), " - ");
            scpy(scene->web_title + slen(scene->web_title), OS_FB_TERM_LEN - slen(scene->web_title), g_web.title);
        }
        for (k = 0; k < g_web.nitems && k < OS_FB_WEB_ITEMS; k++) scene->web[k] = g_web.items[k];
        if (!g_web_mobile) scene->input[0] = 0;
    }
    if (g_ide.active) {
        /* US-050: the editor takes the whole desktop (OS_FB_PANE_IDE). */
        scene->pane = OS_FB_PANE_IDE;
        for (i = 0; i < OSUI_IDE_FRAME && i < OS_FB_TERM_MAX; i++) {
            osui_ide_frame_row(&g_ide, i, scene->term[i], OS_FB_TERM_LEN);
            (void)osui_ide_frame_attr(&g_ide, i, scene->term_attr[i], OS_FB_TERM_LEN);
        }
        scene->nterm = (uint8_t)i;
        scene->input[0] = 0;
    }
}

void osui_gui_render(uint16_t *cells) {
    fill_rect(cells, 0, 0, COLS, ROWS, ' ', ATTR_HINT);
    put_text(cells, 0, 0,
        " MOHHDY OS  gui  chrome=qemu_fb  vbe_lfb  llm=stub_echo  us031=true ",
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
    if (osui_get_media_active()) {
        char media_buf[96];
        scpy(media_buf, 96, " [MEDIA ");
        if (osui_get_media_paused()) {
            scpy(media_buf + slen(media_buf), 96 - slen(media_buf), "PAUSED ");
        } else {
            scpy(media_buf + slen(media_buf), 96 - slen(media_buf), "PLAY ");
        }
        scpy(media_buf + slen(media_buf), 96 - slen(media_buf), osui_get_media_title());
        scpy(media_buf + slen(media_buf), 96 - slen(media_buf), " type=");
        scpy(media_buf + slen(media_buf), 96 - slen(media_buf), osui_get_media_type());
        scpy(media_buf + slen(media_buf), 96 - slen(media_buf), " ticks=");
        /* simple num to string formatting for media ticks */
        unsigned t = osui_get_media_ticks();
        char num_buf[16];
        int np = 0;
        if (t == 0) { num_buf[np++] = '0'; }
        else {
            char rev[16]; int rp = 0;
            while (t > 0) { rev[rp++] = (char)('0' + (t % 10)); t /= 10; }
            while (rp > 0) num_buf[np++] = rev[--rp];
        }
        num_buf[np] = 0;
        scpy(media_buf + slen(media_buf), 96 - slen(media_buf), num_buf);
        scpy(media_buf + slen(media_buf), 96 - slen(media_buf), "]");
        put_text(cells, 0, 9, media_buf, ATTR_TITLE);
    }
}

int osui_gui_feed_key(int key, char *out, int out_max) {
    if (out && out_max > 0) out[0] = 0;
    if (key == 0) {
        osui_stage_tick(out, out_max);
        return 0;
    }
    if (g_web_active && !(g_web_mobile && key != OS_VGA_KEY_ESC && key != OS_VGA_KEY_UP && key != OS_VGA_KEY_DOWN)) {
        /* Lot 6: the web pane takes the desktop; arrows scroll, ESC closes.
         * Lot 9: the mobile pane lets text keys reach the input line. */
        if (key == OS_VGA_KEY_ESC) {
            g_web_active = 0;
            g_web_mobile = 0;
            if (out) { scpy(out, out_max, "osui web close file="); scpy(out + slen(out), out_max - slen(out), g_web_file); scpy(out + slen(out), out_max - slen(out), "\n"); }
        } else if (key == OS_VGA_KEY_DOWN) {
            if (g_web_scroll + 20 < (int)g_web.perf.page_h) g_web_scroll += 20;
        } else if (key == OS_VGA_KEY_UP) {
            g_web_scroll = g_web_scroll >= 20 ? g_web_scroll - 20 : 0;
        }
        return 0;
    }
    if (g_ide.active) {
        /* Full screen: every key goes to the editor, ESC is its command
         * prefix and does not leave the desktop. */
        int act = osui_ide_key(&g_ide, key);
        if (act != OSUI_IDE_NONE) ide_action(act, out, out_max);
        return 0;
    }
    if (key == OS_VGA_KEY_ESC) {
        /* ESC first clears a half typed line; ESC on an empty line
         * leaves the desktop. */
        if (g_ilen > 0) {
            g_ilen = 0;
            g_input[0] = 0;
            if (out) scpy(out, out_max, "osui gui input cleared\n");
            return 0;
        }
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
        if (g_mobile_op && g_input[0] == 'm' && g_input[1] == 'o' && g_input[2] == 'b' && g_input[3] == 'i' &&
            g_input[4] == 'l' && g_input[5] == 'e' && g_input[6] == '-' && g_input[7] == 'u' && g_input[8] == 'i' &&
            (g_input[9] == ' ' || g_input[9] == 0)) {
            const char *a = g_input + 9;
            while (*a == ' ') a++;
            (void)g_mobile_op(a, out, out_max);
            g_ilen = 0;
            g_input[0] = 0;
            return 0;
        }
        if (g_input[0] == 'w' && g_input[1] == 'e' && g_input[2] == 'b' && (g_input[3] == ' ' || g_input[3] == 0)) {
            web_open_line(g_input, out, out_max);
            g_ilen = 0;
            g_input[0] = 0;
            return 0;
        }
        if (g_input[0] == 'p' && g_input[1] == 'm' && g_input[2] == '-' && g_input[3] == 'i' &&
            g_input[4] == 'd' && g_input[5] == 'e' && (g_input[6] == ' ' || g_input[6] == 0)) {
            (void)ide_open_line(g_input, out, out_max);
            g_ilen = 0;
            g_input[0] = 0;
            return 0;
        }
        {
            char tok[32];
            int program = 0;
            const char *eval_line = g_input;
            first_token(g_input, tok, (int)sizeof(tok));
            program = osui_gui_route_line(g_input, osui_get_pane()) == OSUI_GUI_ROUTE_SHELL;
            if (g_input[0] == '!') {
                eval_line = g_input + 1;
                while (*eval_line == ' ' || *eval_line == '\t') eval_line++;
            }
            if (program) return request_terminal(eval_line, out, out_max);
            if (osui_dispatch_line(g_input, out, out_max) != 0 && out
                && sncmp(out, "osui error=commande inconnue", 28) == 0) {
                /* The error used to reach the serial line only: say it
                 * in the chat window too. */
                static char note[OS_FB_MSG_LEN]; /* off the GUI stack */
                scpy(note, (int)sizeof(note), "commande inconnue: ");
                scat(note, (int)sizeof(note), tok);
                scat(note, (int)sizeof(note), " - ai <question> ou /help");
                osui_note(note);
            }
            /* "/shell", the Shell icon or menu: the Shell is the console. */
            if (scmp(osui_get_pane(), "shell") == 0) return request_terminal("", out, out_max);
            if (scmp(osui_get_pane(), "browser") == 0) seed_browser_term();
            else if (scmp(osui_get_pane(), "fs") == 0) seed_fs_term();
        }
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
    static char snap[OSUI_SNAP_MAX];
    osui_gui_write_snap(snap, OSUI_SNAP_MAX);
    serial_puts(snap);
}

void osui_gui_run(void) {
    static os_fb_scene_t scene;
    static char out[OSUI_OUT_MAX];
    int key;
    int stop;

    g_leave = 0;
    g_ilen = 0;
    g_idle = 0;
    g_input[0] = 0;
    g_ide.active = 0;
    g_web_active = 0;
    g_web_mobile = 0;
    g_term_req = 0;
    g_pending[0] = 0;
    osui_gui_term_reset();
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
    osui_gui_closed();
    if (g_term_req && sys_vga_term() == 0) {
        serial_puts("osui gui exit chrome=vbe_term prompt=MOHHDY>\n");
        return;
    }
    g_term_req = 0;
    g_pending[0] = 0;
    sys_vga_leave();
    serial_puts("osui gui exit chrome=text prompt=MOHHDY>\n");
}
