/* osui_tools.c - see osui_tools.h. Freestanding: no libc. */
#include "osui_tools.h"
#include "osui_panes.h"
#include "osui_parity.h"
#include "os_syscalls.h"

#define K_UP 30
#define K_DOWN 31
#define K_HOME 0x12
#define K_END 0x13
#define K_ESC 27
#define CHAT_MAX_STEPS 48
#define CHAT_SENTENCE_MIN 40

static char g_out[OSUI_TOOL_OUT];

static int t_len(const char *s) { int n = 0; while (s && s[n]) n++; return n; }
static void t_cpy(char *d, int max, const char *s) {
    int i = 0;
    if (!d || max <= 0) return;
    while (s && s[i] && i < max - 1) { d[i] = s[i]; i++; }
    d[i] = 0;
}
static void t_cat(char *d, int max, const char *s) { int n = t_len(d); if (n < max) t_cpy(d + n, max - n, s); }
static int t_ncmp(const char *a, const char *b, int n) {
    int i;
    for (i = 0; i < n; i++) { if (a[i] != b[i]) return 1; if (!a[i]) return 0; }
    return 0;
}
static void t_cat_u(char *d, int max, unsigned v) {
    char t[12], r[12]; int n = 0, i;
    do { t[n++] = (char)('0' + v % 10U); v /= 10U; } while (v && n < 11);
    for (i = 0; i < n; i++) r[i] = t[n - 1 - i];
    r[n] = 0;
    t_cat(d, max, r);
}

static const char k_tb_tasks[] = "[Actualiser] [Tuer] [Processus/Services] [Fermer]";
static const char k_tb_view[] = "[Haut] [Bas] [Editer] [Fermer]";
static const char k_tb_settings[] = "[Actualiser] [Fermer]";
static const char k_tb_p2p[] = "[Pairs] [Stats] [Up] [Down] [Envoyer] [Fermer]";
static const char k_tb_pmrun[] = "[Relancer] [Verifier] [Editer] [Fermer]";
static const char k_tb_panel[] = "[Executer] [Menu] [Actualiser] [Fermer]";
static const char k_tb_chat[] = "[Envoyer] [Suite] [Annuler] [Modele] [Effacer] [Fermer]";

const char *osui_tool_toolbar(int kind) {
    switch (kind) {
    case OS_FB_APP_TASKS: return k_tb_tasks;
    case OS_FB_APP_VIEW: return k_tb_view;
    case OS_FB_APP_SETTINGS: return k_tb_settings;
    case OS_FB_APP_P2P: return k_tb_p2p;
    case OS_FB_APP_PMRUN: return k_tb_pmrun;
    case OS_FB_APP_CHAT: return k_tb_chat;
    default: return osui_par_family(kind) ? k_tb_panel : "";
    }
}

const char *osui_tool_name(int kind) {
    switch (kind) {
    case OS_FB_APP_TASKS: return "Gestionnaire des taches";
    case OS_FB_APP_VIEW: return "Visionneuse";
    case OS_FB_APP_SETTINGS: return "Parametres";
    case OS_FB_APP_P2P: return "P2P";
    case OS_FB_APP_PMRUN: return "PromptMessage";
    case OS_FB_APP_CHAT: return "Chat IA";
    default: { const osui_par_family_t *f = osui_par_family(kind); return f ? f->title : "App"; }
    }
}

/* Only non-interactive commands that print and return: a GUI window must
 * never start an editor, the desktop again or a blocking loop. */
int osui_tool_cmd_allowed(const char *cmd) {
    static const char *const exact[] = { "ps", "mem", "uptime", "date", "rtc-check", "whoami", "hal-info", 0 };
    static const char *const prefix[] = { "kill ", "service-status ", "p2p-", "pm-run ", "pm-check ",
                                          "ai-model", "screen-adapt", 0 };
    int i;
    if (!cmd || !cmd[0]) return 0;
    for (i = 0; cmd[i]; i++) if (cmd[i] == ';' || cmd[i] == '|' || cmd[i] == '&' || cmd[i] == '>' || cmd[i] == '<' || cmd[i] == '`' || cmd[i] == '$') return 0;
    for (i = 0; exact[i]; i++) {
        int n = t_len(exact[i]);
        if (t_ncmp(cmd, exact[i], n) == 0 && (cmd[n] == 0)) return 1;
    }
    for (i = 0; prefix[i]; i++) if (t_ncmp(cmd, prefix[i], t_len(prefix[i])) == 0) return 1;
    return 0;
}

/* Terminal-only commands: they read the keyboard in a loop, own the
 * screen, end the session or would nest a desktop or an AI runtime. A
 * window must stay responsive, so they stay in the terminal. */
static const char *const k_term_only[] = {
    "gui", "gui-exit", "console", "exit", "quit", "logout", "clear", "cls", "top", "edit", "nano", "vi",
    "pm-ide", "reboot", "shutdown", "halt", "poweroff", "login", "su", "passwd", "sh", "shell", "run",
    "exec", "watch", "sleep", "read", "osui-start", "osui-run", "osui-session", "osui", "ai-chat",
    "ai-repl", "chat", "screen-mode", "kbd-test", "mouse-test", "serial-console", "p2p-chat", 0
};

int osui_tool_cmd_check(const char *cmd, int (*known)(const char *name)) {
    char name[40];
    int i = 0, n = 0;
    if (!cmd) return OSUI_CMD_EMPTY;
    while (cmd[i] == ' ') i++;
    if (!cmd[i]) return OSUI_CMD_EMPTY;
    for (n = i; cmd[n]; n++) if (cmd[n] == ';' || cmd[n] == '|' || cmd[n] == '&' || cmd[n] == '>' || cmd[n] == '<' || cmd[n] == '`' || cmd[n] == '$' || cmd[n] == '\n') return OSUI_CMD_META;
    if (!known) return osui_tool_cmd_allowed(cmd + i) ? OSUI_CMD_OK : OSUI_CMD_UNKNOWN;
    n = 0;
    while (cmd[i] && cmd[i] != ' ' && n < (int)sizeof(name) - 1) name[n++] = cmd[i++];
    name[n] = 0;
    if (cmd[i] && cmd[i] != ' ') return OSUI_CMD_UNKNOWN;
    for (n = 0; k_term_only[n]; n++) {
        int l = t_len(k_term_only[n]);
        if (t_ncmp(name, k_term_only[n], l + 1) == 0) return OSUI_CMD_TERMINAL;
    }
    /* the osui runtime (AI sessions) can open its own desktop */
    if (t_ncmp(name, "osui-", 5) == 0 && t_ncmp(name, "osui-audit", 11) != 0) return OSUI_CMD_TERMINAL;
    return known(name) ? OSUI_CMD_OK : OSUI_CMD_UNKNOWN;
}

const char *osui_tool_cmd_reason(int code) {
    switch (code) {
    case OSUI_CMD_OK: return "ok";
    case OSUI_CMD_EMPTY: return "commande vide";
    case OSUI_CMD_META: return "caracteres shell interdits (; | & > < ` $)";
    case OSUI_CMD_UNKNOWN: return "commande inconnue du registre shell";
    case OSUI_CMD_TERMINAL: return "commande reservee au terminal (interactive)";
    default: return "refusee";
    }
}

int osui_tool_is_panel(int kind) { return osui_par_family(kind) != 0; }

int osui_tool_line_pid(const char *line) {
    int i = 0, v = 0, got = 0;
    while (line[i] && !(line[i] >= '0' && line[i] <= '9')) {
        /* a header or a text line has letters before any digit */
        if ((line[i] >= 'a' && line[i] <= 'z') || (line[i] >= 'A' && line[i] <= 'Z')) return -1;
        i++;
    }
    while (line[i] >= '0' && line[i] <= '9') { v = v * 10 + (line[i] - '0'); i++; got = 1; }
    return got ? v : -1;
}

/* ---- line buffer ---------------------------------------------------- */

static void clear_lines(osui_tool_t *t) { t->n = 0; t->top = 0; if (t->sel >= 0) t->sel = 0; }

static void add_line(osui_tool_t *t, const char *s, int len, int cls) {
    int k;
    if (t->n >= OSUI_TOOL_LINES) {
        for (k = 0; k + 1 < OSUI_TOOL_LINES; k++) { t_cpy(t->lines[k], OSUI_APP_COLS, t->lines[k + 1]); t->cls[k] = t->cls[k + 1]; }
        t->n = OSUI_TOOL_LINES - 1;
    }
    if (len < 0) len = t_len(s);
    {
        /* the grid font is ASCII: Latin-1 letters of UTF-8 text (French
         * shell messages) lose their accent instead of showing "??" */
        int o = 0;
        for (k = 0; k < len && o < OSUI_APP_COLS - 1; k++) {
            unsigned char c = (unsigned char)s[k];
            if (c == 0xC3 && k + 1 < len) {
                unsigned char b = (unsigned char)s[++k] | 0x20;   /* lower case */
                c = (b >= 0xA0 && b <= 0xA5) ? 'a' : b == 0xA7 ? 'c' : (b >= 0xA8 && b <= 0xAB) ? 'e'
                  : (b >= 0xAC && b <= 0xAF) ? 'i' : (b >= 0xB2 && b <= 0xB6) ? 'o' : (b >= 0xB9 && b <= 0xBC) ? 'u' : '?';
            } else if (c >= 0x80) { while (k + 1 < len && ((unsigned char)s[k + 1] & 0xC0) == 0x80) k++; c = '?'; }
            else if (c == '\t' || c < 32) c = ' ';
            t->lines[t->n][o++] = (char)c;
        }
        len = o;
    }
    t->lines[t->n][len] = 0;
    t->cls[t->n] = (unsigned char)cls;
    t->n++;
}

/* drop ANSI colour sequences (ESC [ ... letter) the shell prints */
static int strip_ansi(char *s, int n) {
    int i = 0, o = 0;
    while (i < n) {
        if (s[i] == 27 && i + 1 < n && s[i + 1] == '[') {
            i += 2;
            while (i < n && !((s[i] >= 'A' && s[i] <= 'Z') || (s[i] >= 'a' && s[i] <= 'z'))) i++;
            if (i < n) i++;
            continue;
        }
        s[o++] = s[i++];
    }
    s[o] = 0;
    return o;
}

/* split text into lines, wrapping at the grid width */
static void add_text(osui_tool_t *t, const char *s, int n, int cls) {
    int i = 0, start = 0, w = OSUI_APP_COLS - 2;
    while (i <= n) {
        if (i == n || s[i] == '\n' || i - start >= w) {
            if (i > start || (i < n && s[i] == '\n')) add_line(t, s + start, i - start, cls);
            if (i < n && s[i] == '\n') i++;
            start = i;
            if (i == n) break;
            continue;
        }
        i++;
    }
}

static int view_rows(const osui_tool_t *t) {
    return (t->kind == OS_FB_APP_P2P || t->kind == OS_FB_APP_CHAT || osui_par_family(t->kind)) ? OSUI_APP_ROWS - 2 : OSUI_APP_ROWS - 1;
}

static void follow_end(osui_tool_t *t) { int v = view_rows(t); t->top = t->n > v ? t->n - v : 0; }

static void follow_sel(osui_tool_t *t) {
    int v = view_rows(t);
    if (t->sel < 0) return;
    if (t->sel >= t->n) t->sel = t->n ? t->n - 1 : 0;
    if (t->sel < t->top) t->top = t->sel;
    if (t->sel >= t->top + v) t->top = t->sel - v + 1;
}

static int run_into(osui_tool_t *t, const osui_tool_src_t *src, const char *cmd, int cls) {
    int n;
    if (!src || !src->run) { add_line(t, "commande indisponible (pas de shell)", -1, OSUI_APP_A_ERR); return -1; }
    n = src->run(cmd, g_out, (int)sizeof(g_out));
    if (n < 0) {
        int why = osui_tool_cmd_check(cmd, src->known);
        add_line(t, "commande refusee: ", -1, OSUI_APP_A_ERR); t_cat(t->lines[t->n - 1], OSUI_APP_COLS, cmd);
        if (why != OSUI_CMD_OK) { t_cat(t->lines[t->n - 1], OSUI_APP_COLS, " ("); t_cat(t->lines[t->n - 1], OSUI_APP_COLS, osui_tool_cmd_reason(why)); t_cat(t->lines[t->n - 1], OSUI_APP_COLS, ")"); }
        return n;
    }
    n = strip_ansi(g_out, n);
    add_text(t, g_out, n, cls);
    return n;
}

static void set_status(osui_tool_t *t, const char *a, const char *b) {
    t_cpy(t->status, (int)sizeof(t->status), a);
    if (b) t_cat(t->status, (int)sizeof(t->status), b);
}

static void first_line(char *d, int max, const char *s) {
    int i = 0;
    (void)strip_ansi((char *)s, t_len(s));
    while (s[i] && s[i] != '\n' && i < max - 1) { d[i] = s[i]; i++; }
    d[i] = 0;
}

/* ---- per app content ------------------------------------------------ */

static void tasks_fill(osui_tool_t *t, const osui_tool_src_t *src) {
    int i;
    clear_lines(t);
    if (t->mode == 0) {
        (void)run_into(t, src, "ps", OSUI_APP_A_TEXT);
        add_line(t, "", 0, OSUI_APP_A_TEXT);
        (void)run_into(t, src, "mem", OSUI_APP_A_DIM);
        /* select the first process row */
        for (i = 0; i < t->n; i++) if (osui_tool_line_pid(t->lines[i]) >= 0) { t->sel = i; break; }
        set_status(t, "selectionnez un processus puis Tuer (le noyau refuse les PID proteges)", 0);
    } else {
        char cmd[48];
        for (i = 0; i < osui_panes_service_count; i++) {
            t_cpy(cmd, (int)sizeof(cmd), "service-status ");
            t_cat(cmd, (int)sizeof(cmd), osui_panes_services[i]);
            (void)run_into(t, src, cmd, OSUI_APP_A_TEXT);
        }
        set_status(t, "services du registre noyau (lecture seule)", 0);
    }
}

static void tasks_kill(osui_tool_t *t, const osui_tool_src_t *src) {
    int pid = (t->sel >= 0 && t->sel < t->n) ? osui_tool_line_pid(t->lines[t->sel]) : -1;
    char cmd[24], res[OSUI_APP_COLS];
    if (t->mode != 0 || pid < 0) { set_status(t, "selectionnez une ligne de processus", 0); return; }
    if (t->armed_pid != pid) {
        t->armed_pid = pid;
        cmd[0] = 0; t_cat_u(cmd, (int)sizeof(cmd), (unsigned)pid);
        set_status(t, "confirmer: Tuer encore pour terminer le PID ", cmd);
        return;
    }
    t->armed_pid = -1;
    t_cpy(cmd, (int)sizeof(cmd), "kill ");
    t_cat_u(cmd, (int)sizeof(cmd), (unsigned)pid);
    if (!src || !src->run || src->run(cmd, g_out, (int)sizeof(g_out)) < 0) { set_status(t, "kill indisponible", 0); return; }
    first_line(res, (int)sizeof(res), g_out);
    t_cpy(t->log, (int)sizeof(t->log), "osui tasks kill pid=");
    t_cat(t->log, (int)sizeof(t->log), cmd + 5);
    t_cat(t->log, (int)sizeof(t->log), " result=");
    t_cat(t->log, (int)sizeof(t->log), res);
    tasks_fill(t, src);
    set_status(t, res, 0);
}

static int ends_with(const char *s, const char *suf) {
    int a = t_len(s), b = t_len(suf);
    return a >= b && t_ncmp(s + a - b, suf, b) == 0;
}

static int skip_ws_comments(const char *b, int n, int i) {
    for (;;) {
        while (i < n && (b[i] == ' ' || b[i] == '\n' || b[i] == '\r' || b[i] == '\t')) i++;
        if (i < n && b[i] == '#') { while (i < n && b[i] != '\n') i++; continue; }
        return i;
    }
}
static int read_num(const char *b, int n, int *i, int *v) {
    int x = 0, got = 0;
    *i = skip_ws_comments(b, n, *i);
    while (*i < n && b[*i] >= '0' && b[*i] <= '9') { x = x * 10 + (b[*i] - '0'); (*i)++; got = 1; }
    *v = x;
    return got ? 0 : -1;
}

int osui_tool_image_ascii(const char *buf, int n, char out[][OSUI_APP_COLS], int cols, int rows) {
    static const char ramp[] = " .:-=+*#%@";
    int i = 2, w, h, maxv, ascii, color, r, c;
    if (!buf || n < 3 || buf[0] != 'P') return -1;
    if (buf[1] == '2') { ascii = 1; color = 0; }
    else if (buf[1] == '5') { ascii = 0; color = 0; }
    else if (buf[1] == '3') { ascii = 1; color = 1; }
    else if (buf[1] == '6') { ascii = 0; color = 1; }
    else return -1;
    if (read_num(buf, n, &i, &w) || read_num(buf, n, &i, &h) || read_num(buf, n, &i, &maxv)) return -1;
    if (w <= 0 || h <= 0 || maxv <= 0 || maxv > 255 || w > 4096 || h > 4096) return -1;
    if (cols > OSUI_APP_COLS - 1) cols = OSUI_APP_COLS - 1;
    /* keep the aspect: a cell is about twice as tall as wide */
    if (w < cols) cols = w;
    {
        int want_rows = (h * cols) / (w * 2);
        if (want_rows < 1) want_rows = 1;
        if (want_rows < rows) rows = want_rows;
    }
    if (!ascii) i++;                     /* single whitespace after maxval */
    {
        int data = i;
        for (r = 0; r < rows; r++) {
            int y = (r * h) / rows;
            for (c = 0; c < cols; c++) {
                int x = (c * w) / cols, lum = 0, k;
                int idx = (y * w + x) * (color ? 3 : 1);
                if (ascii) {
                    int j = data, cnt = 0, v = 0, need = idx + (color ? 3 : 1), s = 0;
                    while (cnt < need) { if (read_num(buf, n, &j, &v)) return -1; if (cnt >= idx) s += v; cnt++; }
                    lum = color ? s / 3 : s;
                } else {
                    if (data + idx + (color ? 2 : 0) >= n) return -1;
                    if (color) { for (k = 0; k < 3; k++) lum += (unsigned char)buf[data + idx + k]; lum /= 3; }
                    else lum = (unsigned char)buf[data + idx];
                }
                out[r][c] = ramp[(lum * 9) / maxv];
            }
            out[r][c] = 0;
        }
    }
    return rows;
}

static unsigned char g_img_rgb[OS_FB_IMG_MAX_W * OS_FB_IMG_MAX_H * 3];
const unsigned char *osui_tool_image_rgb(void) { return g_img_rgb; }

int osui_tool_image_decode(const char *buf, int n, unsigned char *rgb, int max_w, int max_h,
                           int *w_out, int *h_out, int *sw_out, int *sh_out) {
    int i = 2, w, h, maxv, ascii, color, tw, th, sy, ty = 0, data;
    if (!buf || !rgb || n < 3 || buf[0] != 'P' || max_w <= 0 || max_h <= 0) return -1;
    if (buf[1] == '2') { ascii = 1; color = 0; }
    else if (buf[1] == '5') { ascii = 0; color = 0; }
    else if (buf[1] == '3') { ascii = 1; color = 1; }
    else if (buf[1] == '6') { ascii = 0; color = 1; }
    else return -1;
    if (read_num(buf, n, &i, &w) || read_num(buf, n, &i, &h) || read_num(buf, n, &i, &maxv)) return -1;
    if (w <= 0 || h <= 0 || maxv <= 0 || maxv > 255 || w > 4096 || h > 4096) return -1;
    /* fit, aspect kept, never upscaled (the compositor scales up) */
    tw = w; th = h;
    if (tw > max_w) { th = (th * max_w) / tw; tw = max_w; }
    if (th > max_h) { tw = (tw * max_h) / th; th = max_h; }
    if (tw < 1) tw = 1;
    if (th < 1) th = 1;
    if (!ascii) i++;
    data = i;
    for (sy = 0; sy < h && ty < th; sy++) {
        int take_row = (ty * h) / th == sy, sx, tx = 0;
        for (sx = 0; sx < w; sx++) {
            int v[3], k, nc = color ? 3 : 1;
            for (k = 0; k < nc; k++) {
                if (ascii) { if (read_num(buf, n, &data, &v[k])) return -1; }
                else { if (data >= n) return -1; v[k] = (unsigned char)buf[data++]; }
                if (v[k] > maxv) v[k] = maxv;
                v[k] = (v[k] * 255) / maxv;
            }
            if (!color) v[1] = v[2] = v[0];
            if (take_row && tx < tw && (tx * w) / tw == sx) {
                unsigned char *d = rgb + (ty * tw + tx) * 3;
                d[0] = (unsigned char)v[0]; d[1] = (unsigned char)v[1]; d[2] = (unsigned char)v[2];
                tx++;
            }
        }
        if (take_row) ty++;
    }
    if (ty < th) return -1;
    if (w_out) *w_out = tw;
    if (h_out) *h_out = th;
    if (sw_out) *sw_out = w;
    if (sh_out) *sh_out = h;
    return 0;
}

static char g_file[8192];

/* UX5: the image is drawn in pixels by the compositor; the grid keeps a
 * caption line for the serial log and the tests. */
static void view_image_lines(osui_tool_t *t) {
    char cap[OSUI_APP_COLS];
    t->image = 1;
    t->img_gen++;
    t_cpy(cap, (int)sizeof(cap), "image ");
    t_cat_u(cap, (int)sizeof(cap), (unsigned)t->img_src_w);
    t_cat(cap, (int)sizeof(cap), "x");
    t_cat_u(cap, (int)sizeof(cap), (unsigned)t->img_src_h);
    t_cat(cap, (int)sizeof(cap), " affichee en pixels ");
    t_cat_u(cap, (int)sizeof(cap), (unsigned)t->img_w);
    t_cat(cap, (int)sizeof(cap), "x");
    t_cat_u(cap, (int)sizeof(cap), (unsigned)t->img_h);
    add_line(t, cap, -1, OSUI_APP_A_DIM);
    set_status(t, "image PNM en pixels (mise a l'echelle dans la fenetre)", 0);
}

static void view_fill(osui_tool_t *t, const osui_tool_src_t *src) {
    int n;
    static char img[OSUI_APP_ROWS][OSUI_APP_COLS];
    clear_lines(t);
    t->image = 0;
    {
        const char *big = (src && src->load) ? src->load(t->arg, &n) : 0;
        if (big && n >= 0) {
            /* UX5: any size through the heap; the text view shows the head */
            if (osui_tool_image_decode(big, n, g_img_rgb, OS_FB_IMG_MAX_W, OS_FB_IMG_MAX_H,
                                       &t->img_w, &t->img_h, &t->img_src_w, &t->img_src_h) == 0) {
                view_image_lines(t);
                return;
            }
            if (n > (int)sizeof(g_file) - 1) n = (int)sizeof(g_file) - 1;
            {
                int k;
                for (k = 0; k < n; k++) g_file[k] = big[k];
            }
        } else n = (src && src->read) ? src->read(t->arg, g_file, (int)sizeof(g_file) - 1) : -1;
    }
    if (n < 0) { add_line(t, "lecture impossible: ", -1, OSUI_APP_A_ERR); t_cat(t->lines[0], OSUI_APP_COLS, t->arg); set_status(t, "erreur", 0); return; }
    g_file[n] = 0;
    if (osui_tool_image_decode(g_file, n, g_img_rgb, OS_FB_IMG_MAX_W, OS_FB_IMG_MAX_H,
                               &t->img_w, &t->img_h, &t->img_src_w, &t->img_src_h) == 0) {
        view_image_lines(t);
        return;
    }
    (void)img;
    add_text(t, g_file, n, OSUI_APP_A_TEXT);
    {
        char s[OSUI_APP_COLS];
        s[0] = 0;
        t_cat_u(s, (int)sizeof(s), (unsigned)n);
        t_cat(s, (int)sizeof(s), " octets, ");
        t_cat_u(s, (int)sizeof(s), (unsigned)t->n);
        t_cat(s, (int)sizeof(s), n >= (int)sizeof(g_file) - 1 ? " lignes (tronque a 8 Kio)" : " lignes");
        set_status(t, s, 0);
    }
}

static void settings_fill(osui_tool_t *t, const osui_tool_src_t *src) {
    char s[OSUI_APP_COLS];
    int w = 0, h = 0;
    clear_lines(t);
    add_line(t, "Date et heure (RTC CMOS, UTC):", -1, OSUI_APP_A_DIR);
    (void)run_into(t, src, "date", OSUI_APP_A_TEXT);
    (void)run_into(t, src, "uptime", OSUI_APP_A_DIM);
    add_line(t, "", 0, OSUI_APP_A_TEXT);
    add_line(t, "Ecran:", -1, OSUI_APP_A_DIR);
    if (src && src->screen && src->screen(&w, &h) == 0 && w > 0) {
        t_cpy(s, (int)sizeof(s), "  framebuffer VBE ");
        t_cat_u(s, (int)sizeof(s), (unsigned)w); t_cat(s, (int)sizeof(s), "x"); t_cat_u(s, (int)sizeof(s), (unsigned)h);
        t_cat(s, (int)sizeof(s), " 32 bpp (suit la fenetre QEMU via run-gpt2/fit)");
        add_line(t, s, -1, OSUI_APP_A_TEXT);
    } else add_line(t, "  taille inconnue (bureau texte)", -1, OSUI_APP_A_DIM);
    add_line(t, "", 0, OSUI_APP_A_TEXT);
    add_line(t, "Theme:", -1, OSUI_APP_A_DIR);
    add_line(t, "  sombre (unique theme du compositeur; pas de selection)", -1, OSUI_APP_A_TEXT);
    add_line(t, "Clavier:", -1, OSUI_APP_A_DIR);
    add_line(t, "  US QWERTY, PS/2 set 1 (seule disposition du decodeur)", -1, OSUI_APP_A_TEXT);
    add_line(t, "Utilisateur:", -1, OSUI_APP_A_DIR);
    (void)run_into(t, src, "whoami", OSUI_APP_A_TEXT);
    set_status(t, "lecture seule: ces valeurs viennent du noyau et du shell", 0);
}

static void p2p_fill(osui_tool_t *t, const osui_tool_src_t *src, const char *cmd) {
    add_line(t, "", 0, OSUI_APP_A_TEXT);
    add_line(t, "$ ", -1, OSUI_APP_A_DIM);
    t_cat(t->lines[t->n - 1], OSUI_APP_COLS, cmd);
    (void)run_into(t, src, cmd, OSUI_APP_A_TEXT);
    follow_end(t);
}

static void p2p_with_input(osui_tool_t *t, const osui_tool_src_t *src, const char *verb, const char *usage) {
    char cmd[OSUI_TOOL_INPUT + 16];
    if (!t->ilen) { set_status(t, "saisissez d'abord: ", usage); return; }
    t_cpy(cmd, (int)sizeof(cmd), verb);
    t_cat(cmd, (int)sizeof(cmd), t->input);
    p2p_fill(t, src, cmd);
    t->ilen = 0; t->input[0] = 0;
    set_status(t, "ok", 0);
}

static void pm_fill(osui_tool_t *t, const osui_tool_src_t *src, const char *verb) {
    char cmd[96];
    clear_lines(t);
    t_cpy(cmd, (int)sizeof(cmd), verb);
    t_cat(cmd, (int)sizeof(cmd), t->arg);
    add_line(t, "$ ", -1, OSUI_APP_A_DIM);
    t_cat(t->lines[0], OSUI_APP_COLS, cmd);
    (void)run_into(t, src, cmd, OSUI_APP_A_TEXT);
    set_status(t, "sortie reelle de ", cmd);
}

/* ---- chat ------------------------------------------------------------ */

static void chat_start(osui_tool_t *t, int first) {
    if (first) {
        if (!t->ilen) { set_status(t, "ecrivez une question puis Entree", 0); return; }
        t_cpy(t->prompt, (int)sizeof(t->prompt), t->input);
        add_line(t, "Vous: ", -1, OSUI_APP_A_DIR);
        t_cat(t->lines[t->n - 1], OSUI_APP_COLS, t->prompt);
        t->ilen = 0; t->input[0] = 0;
    } else if (!t->prompt[0]) { set_status(t, "rien a continuer", 0); return; }
    add_line(t, first ? "IA: " : "IA (suite): ", -1, OSUI_APP_A_LINK);
    t->gen = first ? 1 : 2;
    t->steps = 0;
    t->cur_len = 0;
    set_status(t, "generation... (Annuler ou Echap pour arreter)", 0);
    follow_end(t);
}

static void chat_append(osui_tool_t *t, const char *p, int n) {
    int i;
    for (i = 0; i < n; i++) {
        char *ln = t->lines[t->n - 1];
        int len = t_len(ln);
        if (p[i] == '\n' || len >= OSUI_APP_COLS - 2) {
            add_line(t, "    ", -1, OSUI_APP_A_LINK);
            if (p[i] == '\n') continue;
            ln = t->lines[t->n - 1];
            len = t_len(ln);
        }
        if ((unsigned char)p[i] >= 32 && (unsigned char)p[i] < 127) { ln[len] = p[i]; ln[len + 1] = 0; }
        t->cur_len++;
    }
}

static void chat_stop(osui_tool_t *t, const char *why) {
    char s[OSUI_APP_COLS];
    t->gen = 0;
    t_cpy(s, (int)sizeof(s), why);
    t_cat(s, (int)sizeof(s), " (");
    t_cat_u(s, (int)sizeof(s), (unsigned)t->steps);
    t_cat(s, (int)sizeof(s), " etapes) - Suite continue la reponse");
    set_status(t, s, 0);
    t_cpy(t->log, (int)sizeof(t->log), "osui chat done ");
    t_cat(t->log, (int)sizeof(t->log), why);
}

int osui_tool_idle(osui_tool_t *t, const osui_tool_src_t *src) {
    static char piece[256];
    int n, k;
    if (!t || !t->active || t->kind != OS_FB_APP_CHAT || !t->gen) return 0;
    if (!src || !src->ai_step) { chat_stop(t, "IA indisponible"); return 1; }
    n = src->ai_step(t->prompt, t->gen == 1, piece, (int)sizeof(piece) - 1);
    t->gen = 2;
    t->steps++;
    if (n == OS_AI_CANCELLED) { chat_stop(t, "annule (Echap)"); return 1; }
    if (n < 0) { chat_stop(t, "erreur du moteur IA"); return 1; }
    if (n == 0) { chat_stop(t, "fin"); return 1; }
    chat_append(t, piece, n);
    follow_end(t);
    for (k = 0; k < n; k++)
        if ((piece[k] == '.' || piece[k] == '!' || piece[k] == '?') && t->cur_len >= CHAT_SENTENCE_MIN) { chat_stop(t, "phrase complete"); return 1; }
    if (t->steps >= CHAT_MAX_STEPS) chat_stop(t, "limite d'etapes");
    return 1;
}

static void chat_model(osui_tool_t *t, const osui_tool_src_t *src) {
    const char *cur = (src && src->ai_model) ? src->ai_model() : "";
    const char *next = (cur && ends_with(cur, ".gguf")) ? "gpt2_124M.bin" : "gpt2.gguf";
    char cmd[48], res[OSUI_APP_COLS];
    if (t->gen) { set_status(t, "annulez la generation avant de changer de modele", 0); return; }
    t_cpy(cmd, (int)sizeof(cmd), "ai-model use ");
    t_cat(cmd, (int)sizeof(cmd), next);
    if (!src || !src->run || src->run(cmd, g_out, (int)sizeof(g_out)) < 0) { set_status(t, "ai-model indisponible", 0); return; }
    first_line(res, (int)sizeof(res), g_out);
    add_line(t, "Modele: ", -1, OSUI_APP_A_DIM);
    t_cat(t->lines[t->n - 1], OSUI_APP_COLS, next);
    set_status(t, res, 0);
    follow_end(t);
}

/* ---- UX5 parity panels (osui_parity.h) -------------------------------- */
/* mode 0: menu of the family's entries (sel), mode 1: output of the last
 * command. The input line holds the argument of ARG entries (or the whole
 * command in the generic /cmd window). MUT entries arm on the first
 * Enter and run on the second (armed_pid holds the armed entry). */

static void panel_menu(osui_tool_t *t) {
    const osui_par_family_t *f = osui_par_family(t->kind);
    int i;
    clear_lines(t);
    t->mode = 0;
    t->armed_pid = -1;
    if (!f) return;
    if (!f->count) {
        add_line(t, "Tapez une commande du terminal puis Entree (ex: uptime, ls /samples).", -1, OSUI_APP_A_DIM);
        add_line(t, "Droits: ceux du shell (meme tache); refusees: commandes interactives", -1, OSUI_APP_A_DIM);
        add_line(t, "du terminal (gui, top, edit, exit...) et caracteres ; | & > < ` $.", -1, OSUI_APP_A_DIM);
        t->sel = -1;
        set_status(t, "commande libre du registre shell", 0);
        return;
    }
    for (i = 0; i < f->count; i++) {
        char *l;
        add_line(t, f->entries[i].label, -1, OSUI_APP_A_TEXT);
        l = t->lines[t->n - 1];
        t_cat(l, OSUI_APP_COLS, "  [");
        t_cat(l, OSUI_APP_COLS, f->entries[i].cmd);
        if (f->entries[i].flags & OSUI_PAR_ARG) { t_cat(l, OSUI_APP_COLS, " "); t_cat(l, OSUI_APP_COLS, f->entries[i].hint); }
        t_cat(l, OSUI_APP_COLS, "]");
        if (f->entries[i].flags & OSUI_PAR_MUT) t_cat(l, OSUI_APP_COLS, " *");
    }
    t->sel = (t->psel >= 0 && t->psel < t->n) ? t->psel : 0;
    follow_sel(t);
    set_status(t, "Entree: executer (* = modifie l'etat, confirmer)  saisie: argument", 0);
}

static void panel_exec(osui_tool_t *t, const osui_tool_src_t *src, const char *cmd) {
    int n;
    t_cpy(t->last, (int)sizeof(t->last), cmd);
    if (t->mode == 0) t->psel = t->sel;
    clear_lines(t);
    t->mode = 1;
    t->sel = -1;
    add_line(t, "$ ", -1, OSUI_APP_A_DIM);
    t_cat(t->lines[0], OSUI_APP_COLS, cmd);
    n = run_into(t, src, cmd, OSUI_APP_A_TEXT);
    t->top = 0;
    t_cpy(t->log, (int)sizeof(t->log), "osui panel run cmd=");
    t_cat(t->log, (int)sizeof(t->log), cmd);
    t_cat(t->log, (int)sizeof(t->log), n < 0 ? " refused" : " ok");
    if (t->n > 1) {
        /* first output line, for the serial log and the QEMU tests */
        t_cat(t->log, (int)sizeof(t->log), "\nosui panel out=");
        t_cat(t->log, (int)sizeof(t->log), t->lines[1]);
    }
    set_status(t, n < 0 ? "refusee: " : "sortie reelle de ", cmd);
    t_cat(t->status, (int)sizeof(t->status), "  (Echap/Menu: retour)");
}

static void panel_run(osui_tool_t *t, const osui_tool_src_t *src) {
    const osui_par_family_t *f = osui_par_family(t->kind);
    const osui_par_entry_t *e;
    char cmd[OSUI_TOOL_INPUT + 40];
    if (!f) return;
    if (!f->count) {
        if (!t->ilen) { set_status(t, "tapez d'abord une commande", 0); return; }
        panel_exec(t, src, t->input);
        t->ilen = 0; t->input[0] = 0;
        return;
    }
    if (t->mode == 1) { panel_menu(t); return; }
    if (t->sel < 0 || t->sel >= f->count) return;
    e = &f->entries[t->sel];
    if (osui_par_build(e, t->input, cmd, (int)sizeof(cmd)) < 0) { set_status(t, "saisissez d'abord: ", e->hint); return; }
    if ((e->flags & OSUI_PAR_MUT) && t->armed_pid != t->sel) {
        t->armed_pid = t->sel;
        set_status(t, "modifie l'etat: Entree encore pour executer ", cmd);
        t_cpy(t->log, (int)sizeof(t->log), "osui panel armed cmd=");
        t_cat(t->log, (int)sizeof(t->log), cmd);
        return;
    }
    t->armed_pid = -1;
    panel_exec(t, src, cmd);
    if (e->flags & OSUI_PAR_ARG) { t->ilen = 0; t->input[0] = 0; }
}

/* ---- common entry points --------------------------------------------- */

void osui_tool_refresh(osui_tool_t *t, const osui_tool_src_t *src) {
    switch (t->kind) {
    case OS_FB_APP_TASKS: tasks_fill(t, src); break;
    case OS_FB_APP_VIEW: view_fill(t, src); break;
    case OS_FB_APP_SETTINGS: settings_fill(t, src); break;
    case OS_FB_APP_P2P: clear_lines(t); p2p_fill(t, src, "p2p-peers"); p2p_fill(t, src, "p2p-stats"); t->top = 0;
        set_status(t, "Up: NOM IP, Envoyer: PAIR TEXTE (saisie en bas)", 0); break;
    case OS_FB_APP_PMRUN: pm_fill(t, src, "pm-run "); break;
    default:
        if (osui_par_family(t->kind)) {
            if (t->mode == 1 && t->last[0]) { char c[sizeof(t->last)]; t_cpy(c, (int)sizeof(c), t->last); panel_exec(t, src, c); }
            else panel_menu(t);
        }
        break;
    }
}

void osui_tool_open(osui_tool_t *t, int kind, const char *arg, const osui_tool_src_t *src) {
    int keep = t->active && t->kind == kind && kind == OS_FB_APP_CHAT;
    if (!keep) {
        t->n = 0; t->top = 0; t->ilen = 0; t->input[0] = 0; t->gen = 0; t->prompt[0] = 0;
        t->status[0] = 0; t->mode = 0; t->image = 0; t->last[0] = 0; t->psel = 0;
    }
    t->kind = kind;
    t->active = 1;
    t->armed_pid = -1;
    t->log[0] = 0;
    t->sel = (kind == OS_FB_APP_TASKS || osui_par_family(kind)) ? 0 : -1;
    t_cpy(t->arg, (int)sizeof(t->arg), arg ? arg : "");
    if (kind == OS_FB_APP_CHAT) {
        if (!keep) {
            add_line(t, "Chat GPT-2 local (moteur du noyau, pas de reseau).", -1, OSUI_APP_A_DIM);
            add_line(t, "Modele: ", -1, OSUI_APP_A_DIM);
            t_cat(t->lines[t->n - 1], OSUI_APP_COLS, (src && src->ai_model) ? src->ai_model() : "?");
            set_status(t, "ecrivez une question puis Entree", 0);
        }
        return;
    }
    osui_tool_refresh(t, src);
}

int osui_tool_tool(osui_tool_t *t, int b, const osui_tool_src_t *src) {
    if (!t || !t->active) return OSUI_TOOL_NONE;
    switch (t->kind) {
    case OS_FB_APP_TASKS:
        if (b == 0) { t->armed_pid = -1; tasks_fill(t, src); }
        else if (b == 1) tasks_kill(t, src);
        else if (b == 2) { t->mode = !t->mode; t->sel = 0; t->armed_pid = -1; tasks_fill(t, src); }
        else if (b == 3) return OSUI_TOOL_CLOSE;
        return OSUI_TOOL_CHANGED;
    case OS_FB_APP_VIEW:
        if (b == 0) t->top = 0;
        else if (b == 1) follow_end(t);
        else if (b == 2) return t->image ? OSUI_TOOL_NONE : OSUI_TOOL_EDIT;
        else if (b == 3) return OSUI_TOOL_CLOSE;
        return OSUI_TOOL_CHANGED;
    case OS_FB_APP_SETTINGS:
        if (b == 0) settings_fill(t, src);
        else if (b == 1) return OSUI_TOOL_CLOSE;
        return OSUI_TOOL_CHANGED;
    case OS_FB_APP_P2P:
        if (b == 0) p2p_fill(t, src, "p2p-peers");
        else if (b == 1) p2p_fill(t, src, "p2p-stats");
        else if (b == 2) p2p_with_input(t, src, "p2p-up ", "NOM IP [CLE]");
        else if (b == 3) p2p_fill(t, src, "p2p-down");
        else if (b == 4) p2p_with_input(t, src, "p2p-send ", "PAIR TEXTE");
        else if (b == 5) return OSUI_TOOL_CLOSE;
        return OSUI_TOOL_CHANGED;
    case OS_FB_APP_PMRUN:
        if (b == 0) pm_fill(t, src, "pm-run ");
        else if (b == 1) pm_fill(t, src, "pm-check ");
        else if (b == 2) return OSUI_TOOL_EDIT;
        else if (b == 3) return OSUI_TOOL_CLOSE;
        return OSUI_TOOL_CHANGED;
    case OS_FB_APP_CHAT:
        if (b == 0) { if (!t->gen) chat_start(t, 1); }
        else if (b == 1) { if (!t->gen) chat_start(t, 0); }
        else if (b == 2) { if (t->gen) chat_stop(t, "annule"); }
        else if (b == 3) chat_model(t, src);
        else if (b == 4) { if (!t->gen) { t->n = 0; t->top = 0; t->prompt[0] = 0; set_status(t, "conversation effacee", 0); } }
        else if (b == 5) { t->gen = 0; return OSUI_TOOL_CLOSE; }
        return OSUI_TOOL_CHANGED;
    default:
        if (!osui_par_family(t->kind)) return OSUI_TOOL_NONE;
        if (b == 0) panel_run(t, src);
        else if (b == 1) panel_menu(t);
        else if (b == 2) osui_tool_refresh(t, src);
        else if (b == 3) return OSUI_TOOL_CLOSE;
        return OSUI_TOOL_CHANGED;
    }
}

int osui_tool_click(osui_tool_t *t, int row, int col, int dbl, const osui_tool_src_t *src) {
    int idx;
    (void)col;
    if (!t || !t->active || row < 0) return OSUI_TOOL_NONE;
    idx = t->top + row;
    if (t->kind == OS_FB_APP_TASKS && idx < t->n) {
        if (t->sel != idx) t->armed_pid = -1;
        t->sel = idx;
        if (dbl) tasks_kill(t, src);
        return OSUI_TOOL_CHANGED;
    }
    if (osui_par_family(t->kind) && t->mode == 0 && t->sel >= 0 && idx < t->n) {
        if (t->sel != idx) t->armed_pid = -1;
        t->sel = idx;
        if (dbl) panel_run(t, src);
        return OSUI_TOOL_CHANGED;
    }
    return OSUI_TOOL_NONE;
}

int osui_tool_key(osui_tool_t *t, int key, const osui_tool_src_t *src) {
    int v;
    if (!t || !t->active) return OSUI_TOOL_NONE;
    v = view_rows(t);
    if (osui_par_family(t->kind)) {
        if (key == K_ESC) {
            if (t->mode == 1 && osui_par_family(t->kind)->count) { panel_menu(t); return OSUI_TOOL_CHANGED; }
            return OSUI_TOOL_CLOSE;
        }
        if (key == '\n' || key == '\r') { panel_run(t, src); return OSUI_TOOL_CHANGED; }
        if (key == '\b' || key == 127) { if (t->ilen > 0) t->input[--t->ilen] = 0; return OSUI_TOOL_CHANGED; }
        if (key >= 32 && key <= 126) {
            if (t->ilen < OSUI_TOOL_INPUT - 1) { t->input[t->ilen++] = (char)key; t->input[t->ilen] = 0; }
            t->armed_pid = -1;
            return OSUI_TOOL_CHANGED;
        }
        if (t->sel >= 0 && (key == K_UP || key == K_DOWN)) t->armed_pid = -1;
    } else if (t->kind == OS_FB_APP_P2P || t->kind == OS_FB_APP_CHAT) {
        if (key == K_ESC) {
            if (t->kind == OS_FB_APP_CHAT && t->gen) { chat_stop(t, "annule"); return OSUI_TOOL_CHANGED; }
            return OSUI_TOOL_CLOSE;
        }
        if (key == '\n' || key == '\r') {
            if (t->kind == OS_FB_APP_CHAT) { if (!t->gen) chat_start(t, 1); }
            else p2p_with_input(t, src, "p2p-send ", "PAIR TEXTE");
            return OSUI_TOOL_CHANGED;
        }
        if (key == '\b' || key == 127) { if (t->ilen > 0) t->input[--t->ilen] = 0; return OSUI_TOOL_CHANGED; }
        if (key >= 32 && key <= 126) {
            if (t->ilen < OSUI_TOOL_INPUT - 1) { t->input[t->ilen++] = (char)key; t->input[t->ilen] = 0; }
            return OSUI_TOOL_CHANGED;
        }
    } else if (key == K_ESC) return OSUI_TOOL_CLOSE;
    else if (key == 'u' || key == 'r') { osui_tool_refresh(t, src); return OSUI_TOOL_CHANGED; }
    else if (key == 'e' && (t->kind == OS_FB_APP_VIEW || t->kind == OS_FB_APP_PMRUN)) return osui_tool_tool(t, 2, src);
    else if (key == 'k' && t->kind == OS_FB_APP_TASKS) { tasks_kill(t, src); return OSUI_TOOL_CHANGED; }
    if (t->sel >= 0) {
        if (key == K_UP && t->sel > 0) { t->sel--; t->armed_pid = -1; }
        else if (key == K_DOWN && t->sel + 1 < t->n) { t->sel++; t->armed_pid = -1; }
        else if (key == K_HOME) t->sel = 0;
        else if (key == K_END) t->sel = t->n ? t->n - 1 : 0;
        else return OSUI_TOOL_NONE;
        follow_sel(t);
        return OSUI_TOOL_CHANGED;
    }
    if (key == K_UP && t->top > 0) t->top--;
    else if (key == K_DOWN && t->top + v < t->n) t->top++;
    else if (key == K_HOME) t->top = 0;
    else if (key == K_END) follow_end(t);
    else return OSUI_TOOL_NONE;
    return OSUI_TOOL_CHANGED;
}

void osui_tool_render(const osui_tool_t *t, osui_app_view_t *v) {
    int r, k, rows = view_rows(t);
    osui_app_view_clear(v);
    t_cpy(v->title, (int)sizeof(v->title), osui_tool_name(t->kind));
    if (t->arg[0]) { t_cat(v->title, (int)sizeof(v->title), " - "); t_cat(v->title, (int)sizeof(v->title), t->arg); }
    else if (t->kind == OS_FB_APP_CHAT && t->gen) t_cat(v->title, (int)sizeof(v->title), " - generation");
    t_cpy(v->tool, OSUI_APP_COLS, osui_tool_toolbar(t->kind));
    t_cpy(v->status, OSUI_APP_COLS, t->status);
    v->cur_row = -1; v->cur_col = -1;
    for (r = 0; r < rows && t->top + r < t->n; r++) {
        int i = t->top + r;
        t_cpy(v->rows[r], OSUI_APP_COLS, t->lines[i]);
        for (k = 0; k < OSUI_APP_COLS; k++) v->attr[r][k] = (unsigned char)(i == t->sel ? OSUI_APP_A_SEL : t->cls[i]);
    }
    v->nrows = r;
    if (t->kind == OS_FB_APP_P2P || t->kind == OS_FB_APP_CHAT || osui_par_family(t->kind)) {
        int last = OSUI_APP_ROWS - 1;
        const osui_par_family_t *f = osui_par_family(t->kind);
        t_cpy(v->rows[last], OSUI_APP_COLS, t->kind == OS_FB_APP_CHAT ? "> " : f ? (f->count ? "arg> " : "cmd> ") : "p2p> ");
        t_cat(v->rows[last], OSUI_APP_COLS, t->input);
        for (k = 0; k < OSUI_APP_COLS; k++) v->attr[last][k] = OSUI_APP_A_MATCH;
        v->nrows = OSUI_APP_ROWS;
        v->cur_row = last;
        v->cur_col = t_len(v->rows[last]);
    }
}
