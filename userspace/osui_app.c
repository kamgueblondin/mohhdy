#include "osui_app.h"

int osui_app_tool_hit(const char *tool, int col) {
    int i, idx = -1, in = 0;
    if (!tool || col < 0) return -1;
    for (i = 0; tool[i] && i <= col; i++) {
        if (tool[i] == '[') { idx++; in = 1; }
        if (i == col) return in ? idx : -1;
        if (tool[i] == ']') in = 0;
    }
    return -1;
}

void osui_app_view_clear(osui_app_view_t *v) {
    int i;
    char *p = (char *)v;
    if (!v) return;
    for (i = 0; i < (int)sizeof(*v); i++) p[i] = 0;
    v->cur_row = v->cur_col = -1;
}
