/* osui_gui_apps.inc - included by osui_gui.c: per-app dispatch of the
 * generic window slots (keys, kernel events, rendering) and the tool apps
 * of osui_tools.c (task manager, viewer, settings, P2P, PM run, chat). */
#include "osui_tools.h"

#define TOOL_FIRST OS_FB_APP_TASKS
#define TOOL_LAST OS_FB_APP_KIND_MAX   /* UX5: parity panels 9..19 */
static osui_tool_t g_tools[TOOL_LAST - TOOL_FIRST + 1];
static osui_tool_src_t g_tool_src;

static osui_tool_t *tool_of(int kind) {
    return (kind >= TOOL_FIRST && kind <= TOOL_LAST) ? &g_tools[kind - TOOL_FIRST] : 0;
}

static int tool_read(const char *path, char *buf, int max) {
    if (!g_fs_io) return -1;
    return fsops_read(g_fs_io, path, buf, max);
}

/* UX5: whole file through the heap (viewer images over 8 KB). */
static const char *tool_load(const char *path, int *n) {
    os_dirent_t st;
    char *buf;
    if (!g_fs_io || !g_fs_io->stat || g_fs_io->stat(path, &st) != 0 || st.flags == OS_DIRENT_DIR) return 0;
    if ((int)st.size <= FSOPS_FILE_MAX) return 0;          /* small: the read path */
    buf = big_buf((int)st.size);
    if (!buf) return 0;
    *n = fsops_read(g_fs_io, path, buf, (int)st.size);
    if (*n < 0) return 0;
    buf[*n] = 0;
    return buf;
}

/* UX5: send the viewer image to the compositor when it changes. */
static os_fb_image_t g_img_msg;
static unsigned g_img_sent_gen;
static int g_img_sent_slot = -1;
static void image_sync(void) {
    osui_tool_t *t = tool_of(OS_FB_APP_VIEW);
    int slot = slot_of(OS_FB_APP_VIEW);
    if (t && t->active && t->image && slot >= 0) {
        int k, n;
        const unsigned char *px = osui_tool_image_rgb();
        if (g_img_sent_slot == slot && g_img_sent_gen == t->img_gen) return;
        g_img_msg.magic = OS_FB_IMG_MAGIC;
        g_img_msg.slot = (uint8_t)slot;
        g_img_msg.w = (uint16_t)t->img_w;
        g_img_msg.h = (uint16_t)t->img_h;
        n = t->img_w * t->img_h * 3;
        for (k = 0; k < n; k++) g_img_msg.rgb[k] = px[k];
        sys_vga_image(&g_img_msg);
        g_img_sent_slot = slot;
        g_img_sent_gen = t->img_gen;
    } else if (g_img_sent_slot >= 0) {
        sys_vga_image(0);
        g_img_sent_slot = -1;
    }
}

static int tool_stat(const char *path, os_dirent_t *out) {
    if (!g_fs_io || !g_fs_io->stat) return -1;
    return g_fs_io->stat(path, out);
}

void osui_gui_set_tool_ops(int (*run)(const char *, char *, int),
                           int (*ai_step)(const char *, int, char *, int),
                           const char *(*ai_model)(void), int (*screen)(int *, int *)) {
    g_tool_src.run = run;
    g_tool_src.read = tool_read;
    g_tool_src.stat = tool_stat;
    g_tool_src.ai_step = ai_step;
    g_tool_src.ai_model = ai_model;
    g_tool_src.screen = screen;
    g_tool_src.load = tool_load;
}

void osui_gui_set_tool_known(int (*known)(const char *)) { g_tool_src.known = known; }

static void tool_log(osui_tool_t *t, char *out, int out_max) {
    if (t->log[0]) { app_out(out, out_max, t->log, 0); t->log[0] = 0; }
}

int osui_gui_tool_open(int kind, const char *arg, char *out, int out_max) {
    osui_tool_t *t = tool_of(kind);
    char path[FSOPS_PATH_MAX];
    if (!t) return -1;
    if ((kind == OS_FB_APP_VIEW || kind == OS_FB_APP_PMRUN) && (!arg || !arg[0])) {
        app_out(out, out_max, "osui tool error=file_missing app=", osui_tool_name(kind));
        osui_note(kind == OS_FB_APP_VIEW ? "usage: /view FICHIER" : "usage: /pmrun FICHIER.pm");
        return -1;
    }
    if (slot_of(kind) < 0 && slot_take(kind) < 0) { app_out(out, out_max, "osui app error=no_slot", 0); return -1; }
    path[0] = 0;
    if (osui_tool_is_panel(kind)) {
        /* a panel argument prefills the input line (no path) */
        int k = 0;
        osui_tool_open(t, kind, "", &g_tool_src);
        while (arg && arg[k] && k < OSUI_TOOL_INPUT - 1) { t->input[k] = arg[k]; k++; }
        t->input[k] = 0; t->ilen = k;
        g_focus = slot_of(kind);
        app_out(out, out_max, "osui tool open app=", osui_tool_name(kind));
        for (k = 0; k < t->n && k < 3; k++) { scat(out, out_max, "osui tool line="); scat(out, out_max, t->lines[k]); scat(out, out_max, "\n"); }
        return 0;
    }
    if (arg && arg[0]) fsops_resolve(g_files.active ? g_files.cwd : "/", arg, path, (int)sizeof(path));
    osui_tool_open(t, kind, path, &g_tool_src);
    g_focus = slot_of(kind);
    app_out(out, out_max, "osui tool open app=", osui_tool_name(kind));
    if (path[0]) { scat(out, out_max, "osui tool file="); scat(out, out_max, path); scat(out, out_max, "\n"); }
    {
        /* the first lines of real content, for the serial log and the tests */
        int i;
        for (i = 0; i < t->n && i < 6; i++) { scat(out, out_max, "osui tool line="); scat(out, out_max, t->lines[i]); scat(out, out_max, "\n"); }
    }
    return 0;
}

static void tool_result(int kind, int r, char *out, int out_max) {
    osui_tool_t *t = tool_of(kind);
    tool_log(t, out, out_max);
    if (r == OSUI_TOOL_CLOSE) {
        t->active = 0;
        t->gen = 0;
        slot_drop(kind);
        app_out(out, out_max, "osui tool close app=", osui_tool_name(kind));
    } else if (r == OSUI_TOOL_EDIT) {
        char p[64];
        scpy(p, (int)sizeof(p), t->arg);
        (void)osui_gui_edit_open(p, out, out_max);
    }
}

static void tool_event(int kind, const char *p, char *out, int out_max) {
    osui_tool_t *t = tool_of(kind);
    int r = OSUI_TOOL_NONE;
    if (sncmp(p, "close", 5) == 0) r = OSUI_TOOL_CLOSE;
    else if (sncmp(p, "tool", 4) == 0) { p += 4; r = osui_tool_tool(t, osui_app_tool_hit(osui_tool_toolbar(kind), evt_int(&p)), &g_tool_src); }
    else if (sncmp(p, "dclick", 6) == 0) { int rr, cc; p += 6; rr = evt_int(&p); cc = evt_int(&p); r = osui_tool_click(t, rr, cc, 1, &g_tool_src); }
    else if (sncmp(p, "click", 5) == 0) { int rr, cc; p += 5; rr = evt_int(&p); cc = evt_int(&p); r = osui_tool_click(t, rr, cc, 0, &g_tool_src); }
    tool_result(kind, r, out, out_max);
}

/* Desktop idle tick: the chat streams one GPT-2 step per tick, so each
 * piece reaches the window as soon as the kernel returns it. */
int osui_gui_idle(char *out, int out_max) {
    osui_tool_t *t = tool_of(OS_FB_APP_CHAT);
    int changed;
    if (!t->active || !t->gen) return 0;
    changed = osui_tool_idle(t, &g_tool_src);
    if (!t->gen && t->log[0]) {
        tool_log(t, out, out_max);
        scat(out, out_max, "osui chat reply=");
        {
            /* the last answer lines, for the serial log and the tests */
            int i = t->n - 1;
            while (i > 0 && sncmp(t->lines[i], "IA", 2) != 0) i--;
            for (; i < t->n; i++) { scat(out, out_max, t->lines[i]); scat(out, out_max, " "); }
        }
        scat(out, out_max, "\n");
    }
    return changed;
}

static void app_key(int kind, int key, char *out, int out_max) {
    switch (kind) {
    case OS_FB_APP_FILES: files_key(key, out, out_max); break;
    case OS_FB_APP_EDIT: edit_key(key, out, out_max); break;
    default: if (tool_of(kind)) tool_result(kind, osui_tool_key(tool_of(kind), key, &g_tool_src), out, out_max); break;
    }
}

static void app_event(int kind, const char *p, char *out, int out_max) {
    switch (kind) {
    case OS_FB_APP_FILES: files_event(p, out, out_max); break;
    case OS_FB_APP_EDIT: edit_event(p, out, out_max); break;
    default:
        if (tool_of(kind)) tool_event(kind, p, out, out_max);
        else if (sncmp(p, "close", 5) == 0) slot_drop(kind);
        break;
    }
}

/* 1 when the app has a view to show in its window */
static int app_render(int kind, osui_app_view_t *v) {
    switch (kind) {
    case OS_FB_APP_FILES: if (!g_files.active) return 0; osui_files_render(&g_files, v); return 1;
    case OS_FB_APP_EDIT: if (!g_edit.active) return 0; osui_ide_render_app(&g_edit, v); return 1;
    default:
        if (!tool_of(kind) || !tool_of(kind)->active) return 0;
        osui_tool_render(tool_of(kind), v);
        return 1;
    }
}

/* "/tasks", "/view F", "/settings", "/peers", "/pmrun F", "/chat", and the
 * UX5 parity panels of osui_parity.c ("/net", "/disk", ... "/cmd [CMD]") */
static int tool_slash(const char *line, char *out, int out_max) {
    int i;
    for (i = 0; i < osui_par_builtin_count; i++) {
        const char *sl = osui_par_builtins[i].slash;
        int n = slen(sl);
        if (!osui_par_builtins[i].kind) continue;
        if (sncmp(line, sl, n) == 0 && (line[n] == ' ' || line[n] == 0)) {
            const char *a = line + n;
            while (*a == ' ') a++;
            (void)osui_gui_tool_open(osui_par_builtins[i].kind, a, out, out_max);
            return 1;
        }
    }
    for (i = 0; i < osui_par_family_count; i++) {
        const char *sl = osui_par_families[i].slash;
        int n = slen(sl);
        if (sncmp(line, sl, n) == 0 && (line[n] == ' ' || line[n] == 0)) {
            const char *a = line + n;
            while (*a == ' ') a++;
            if (osui_gui_tool_open(osui_par_families[i].kind, a, out, out_max) == 0
                && osui_par_families[i].kind == OS_FB_APP_HELP) {
                /* Lot 45: /help shows the list at once (first entry) */
                int k = slen(out);
                app_key(OS_FB_APP_HELP, '\n', out + k, out_max - k);
            }
            return 1;
        }
    }
    return 0;
}
