/* osui_app.h - the content grid of a desktop app window (file explorer,
 * text editor). The Ring 3 app model fills it; the kernel draws it in a
 * window of the shared window manager (kernel/gfx_wm.c) and turns mouse
 * clicks into app events (row, column, toolbar column). */
#ifndef OSUI_APP_H
#define OSUI_APP_H

#define OSUI_APP_ROWS 22
#define OSUI_APP_COLS 78      /* characters per row, NUL included */

/* character classes of a grid row */
#define OSUI_APP_A_TEXT 0
#define OSUI_APP_A_DIR 1
#define OSUI_APP_A_DIM 2      /* gutter, header, hints */
#define OSUI_APP_A_SEL 3      /* selected row / text selection */
#define OSUI_APP_A_LINK 4     /* breadcrumb segment */
#define OSUI_APP_A_ERR 5
#define OSUI_APP_A_MATCH 6

typedef struct {
    char title[64];
    char tool[OSUI_APP_COLS];            /* "[Label] [Label] ..." */
    char rows[OSUI_APP_ROWS][OSUI_APP_COLS];
    unsigned char attr[OSUI_APP_ROWS][OSUI_APP_COLS];
    char status[OSUI_APP_COLS];
    int nrows;
    int cur_row, cur_col;                /* caret in grid cells, -1 none */
    int dirty;                           /* title marker */
} osui_app_view_t;

/* Index of the "[...]" group of the toolbar at column col, -1 none. */
int osui_app_tool_hit(const char *tool, int col);
void osui_app_view_clear(osui_app_view_t *v);
#endif
