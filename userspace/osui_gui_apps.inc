/* osui_gui_apps.inc - included by osui_gui.c: per-app dispatch of the
 * generic window slots (keys, kernel events, rendering). */

static void app_key(int kind, int key, char *out, int out_max) {
    switch (kind) {
    case OS_FB_APP_FILES: files_key(key, out, out_max); break;
    case OS_FB_APP_EDIT: edit_key(key, out, out_max); break;
    default: break;
    }
}

static void app_event(int kind, const char *p, char *out, int out_max) {
    switch (kind) {
    case OS_FB_APP_FILES: files_event(p, out, out_max); break;
    case OS_FB_APP_EDIT: edit_event(p, out, out_max); break;
    default: if (sncmp(p, "close", 5) == 0) slot_drop(kind); break;
    }
}

/* 1 when the app has a view to show in its window */
static int app_render(int kind, osui_app_view_t *v) {
    switch (kind) {
    case OS_FB_APP_FILES: if (!g_files.active) return 0; osui_files_render(&g_files, v); return 1;
    case OS_FB_APP_EDIT: if (!g_edit.active) return 0; osui_ide_render_app(&g_edit, v); return 1;
    default: return 0;
    }
}
