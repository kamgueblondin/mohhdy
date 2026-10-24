/* osui_gui_apps.inc - included by osui_gui.c: per-app dispatch of the
 * generic window slots (keys, kernel events, rendering) and the tool apps
 * of osui_tools.c (task manager, viewer, settings, P2P, PM run, chat). */
#include "osui_tools.h"

#define TOOL_FIRST OS_FB_APP_TASKS
#define TOOL_LAST OS_FB_APP_CHAT
static osui_tool_t g_tools[TOOL_LAST - TOOL_FIRST + 1];
static osui_tool_src_t g_tool_src;

static osui_tool_t *tool_of(int kind) {
    return (kind >= TOOL_FIRST && kind <= TOOL_LAST) ? &g_tools[kind - TOOL_FIRST] : 0;
}

static int tool_read(const char *path, char *buf, int max) {
    if (!g_fs_io) return -1;
    return fsops_read(g_fs_io, path, buf, max);
}

void osui_gui_set_tool_ops(int (*run)(const char *, char *, int),
                           int (*ai_step)(const char *, int, char *, int),
                           const char *(*ai_model)(void), int (*screen)(int *, int *)) {
    g_tool_src.run = run;
    g_tool_src.read = tool_read;
    g_tool_src.ai_step = ai_step;
    g_tool_src.ai_model = ai_model;
    g_tool_src.screen = screen;
}

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

/* "/tasks", "/view F", "/settings", "/peers", "/pmrun F", "/chat" */
static int tool_slash(const char *line, char *out, int out_max) {
    static const struct { const char *cmd; int kind; } k[] = {
        { "/tasks", OS_FB_APP_TASKS }, { "/view", OS_FB_APP_VIEW }, { "/settings", OS_FB_APP_SETTINGS },
        { "/peers", OS_FB_APP_P2P }, { "/pmrun", OS_FB_APP_PMRUN }, { "/chat", OS_FB_APP_CHAT },
    };
    int i;
    for (i = 0; i < (int)(sizeof(k) / sizeof(k[0])); i++) {
        int n = slen(k[i].cmd);
        if (sncmp(line, k[i].cmd, n) == 0 && (line[n] == ' ' || line[n] == 0)) {
            const char *a = line + n;
            while (*a == ' ') a++;
            (void)osui_gui_tool_open(k[i].kind, a, out, out_max);
            return 1;
        }
    }
    return 0;
}
