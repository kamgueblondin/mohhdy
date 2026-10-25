/* osui_files.h - file explorer of the desktop. Pure model over the shared
 * file library (fsops.c, the same code as the shell commands ls, mkdir,
 * touch, mv, cp, rm, rmdir, stat). The caller (osui_gui.c) owns the window
 * and opens text files in the editor when osui_files_key/click return
 * OSUI_FILES_OPEN. */
#ifndef OSUI_FILES_H
#define OSUI_FILES_H
#include "fsops.h"
#include "osui_app.h"

#define OSUI_FILES_MAX 64
#define OSUI_FILES_VIEW (OSUI_APP_ROWS - 2)   /* breadcrumb + header */

enum { OSUI_FILES_NONE = 0, OSUI_FILES_OPEN = 1, OSUI_FILES_CLOSE = 2 };
enum { OSUI_FILES_BROWSE = 0, OSUI_FILES_PROMPT = 1, OSUI_FILES_CONFIRM = 2 };
enum { OSUI_FOP_NEWFILE = 1, OSUI_FOP_NEWDIR, OSUI_FOP_RENAME, OSUI_FOP_COPY, OSUI_FOP_MOVE, OSUI_FOP_DELETE };

/* toolbar buttons, in osui_files_toolbar order */
enum { OSUI_FT_PARENT = 0, OSUI_FT_OPEN, OSUI_FT_NEWFILE, OSUI_FT_NEWDIR, OSUI_FT_RENAME,
       OSUI_FT_DELETE, OSUI_FT_COPY, OSUI_FT_MOVE, OSUI_FT_INFO };

typedef struct {
    int active;
    const fsops_io_t *io;
    char cwd[FSOPS_PATH_MAX];
    os_dirent_t ent[OSUI_FILES_MAX];
    int n, sel, top;
    int mode, op;
    char input[OS_NAME_MAX + 64];
    int ilen;
    char status[OSUI_APP_COLS];
    char open_path[FSOPS_PATH_MAX];   /* file to open (OSUI_FILES_OPEN) */
    char log[160];                     /* last operation, for the serial line */
    int vrows, vcols;                  /* Lot 45: window grid, 0 default */
} osui_files_t;

extern const char osui_files_toolbar[];

void osui_files_open(osui_files_t *f, const fsops_io_t *io, const char *dir);
int osui_files_refresh(osui_files_t *f);
int osui_files_cd(osui_files_t *f, const char *dir);
int osui_files_key(osui_files_t *f, int key);
/* Grid click: row/col of the content grid; dbl = double click. */
int osui_files_click(osui_files_t *f, int row, int col, int dbl);
int osui_files_tool(osui_files_t *f, int button);
void osui_files_render(const osui_files_t *f, osui_app_view_t *v);
/* Path of the selected entry; 0 when the list is empty. */
int osui_files_selected(const osui_files_t *f, char *out, int max);
/* Lot 45 (US-G15): the window grid changed; more rows list more entries */
void osui_files_set_view(osui_files_t *f, int rows, int cols);
#endif
