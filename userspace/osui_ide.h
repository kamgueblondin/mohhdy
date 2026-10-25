#ifndef OSUI_IDE_H
#define OSUI_IDE_H

/* US-050: full-screen PromptMessage IDE surface of OS-UI. Pure model:
 * text buffer, cursor, scrolling, key bindings and the frame rows the
 * desktop draws. Save, check and run are actions returned to the caller
 * (osui_gui.c), which owns the file system and the PromptMessage VM. */

#define OSUI_IDE_LINES 96     /* built-in line store (no heap attached) */
#define OSUI_IDE_COLS 72      /* characters per line, without the NUL */
/* UX5: with osui_ide_set_heap the line store grows on the user heap, so a
 * file is bounded by memory; undo keeps snapshots up to OSUI_IDE_SNAP_MAX
 * bytes and is switched off (status says so) for larger buffers. */
#define OSUI_IDE_SNAP_MAX 8192
#define OSUI_IDE_VIEW 12      /* text rows on screen (frame = 14 rows) */
#define OSUI_IDE_FRAME 14     /* title + OSUI_IDE_VIEW + status */
#define OSUI_IDE_ROW_LEN 64   /* frame row buffer, NUL included */
/* Lot 45: copy / paste buffer, the kernel clipboard bound (OS_CLIP_MAX) */
#define OSUI_IDE_CLIP_MAX 65536
#define OSUI_IDE_TEXT_MAX (OSUI_IDE_LINES * (OSUI_IDE_COLS + 1))

#define OSUI_IDE_KEY_ESC 27
#define OSUI_IDE_KEY_LEFT 28
#define OSUI_IDE_KEY_RIGHT 29
#define OSUI_IDE_KEY_UP 30
#define OSUI_IDE_KEY_DOWN 31

/* Syntax classes of a PromptMessage character (osui_ide_highlight) and of
 * a frame row character (osui_ide_frame_attr). */
#define OSUI_IDE_TOK_TEXT 0
#define OSUI_IDE_TOK_KEYWORD 1
#define OSUI_IDE_TOK_STRING 2
#define OSUI_IDE_TOK_NUMBER 3
#define OSUI_IDE_TOK_COMMENT 4
#define OSUI_IDE_TOK_GUTTER 5
#define OSUI_IDE_TOK_OUTPUT 6
#define OSUI_IDE_TOK_RULE 7

/* Program output pane: last OSUI_IDE_OUT_LINES lines of the last run,
 * OSUI_IDE_OUT_VIEW of them on screen under the text when visible. */
#define OSUI_IDE_OUT_LINES 16
#define OSUI_IDE_OUT_VIEW 4

enum {
    OSUI_IDE_NONE = 0,
    OSUI_IDE_SAVE = 1,
    OSUI_IDE_CHECK = 2,
    OSUI_IDE_RUN = 3,
    OSUI_IDE_QUIT = 4,
    OSUI_IDE_SAVE_AS = 5,   /* plain mode: ide->file holds the new name */
    OSUI_IDE_OPEN = 6       /* plain mode: ide->prompt_out holds the path */
};

/* Text editor of the desktop (plain mode of the same core): extra keys. */
#define OSUI_IDE_KEY_HOME 0x12
#define OSUI_IDE_KEY_END 0x13
#define OSUI_IDE_KEY_DEL 0x14
#define OSUI_IDE_UNDO 8
/* prompts of the plain mode */
enum { OSUI_IDE_P_NONE = 0, OSUI_IDE_P_FIND, OSUI_IDE_P_SAVE_AS, OSUI_IDE_P_OPEN };
/* toolbar of the text editor window, in osui_ide_tool order */
enum { OSUI_IDE_T_OPEN = 0, OSUI_IDE_T_SAVE, OSUI_IDE_T_SAVE_AS, OSUI_IDE_T_CUT, OSUI_IDE_T_COPY,
       OSUI_IDE_T_PASTE, OSUI_IDE_T_UNDO, OSUI_IDE_T_REDO, OSUI_IDE_T_FIND, OSUI_IDE_T_MARK };
extern const char osui_ide_toolbar[];

typedef struct {
    int active;
    char file[48];
    char (*lines)[OSUI_IDE_COLS + 1];   /* lines0 or a heap block */
    int cap;                            /* lines the store holds */
    char lines0[OSUI_IDE_LINES][OSUI_IDE_COLS + 1];
    int view_rows, view_cols;           /* window grid (0 = default) */
    int undo_off;                       /* buffer larger than a snapshot */
    int nlines;
    int row, col;   /* cursor, 0-based */
    int top;        /* first visible line */
    int left;       /* first visible column */
    int modified;
    int cmd_mode;   /* ESC pressed: next key is a command */
    int truncated;  /* file did not fit when opened */
    char status[OSUI_IDE_ROW_LEN];
    char out[OSUI_IDE_OUT_LINES][OSUI_IDE_ROW_LEN];
    int out_n;        /* lines kept */
    int out_total;    /* lines received since the last clear */
    int out_visible;  /* output pane shown (ESC o toggles) */
    int completions;  /* Tab completions applied */
    /* plain text mode (desktop text editor), see osui_ide_open_plain */
    int plain;
    int mark_on, mark_row, mark_col;  /* selection: mark .. cursor */
    int prompt;                       /* OSUI_IDE_P_* */
    char prompt_buf[48];
    char prompt_out[48];
    char find[48];
    int quit_armed;                   /* q on a modified buffer asked once */
    int last_edit;                    /* undo grouping of typed characters */
    int undo_n, redo_n;
    int undo_rc[OSUI_IDE_UNDO][2], redo_rc[OSUI_IDE_UNDO][2];
    char undo[OSUI_IDE_UNDO][OSUI_IDE_SNAP_MAX];
    char redo[OSUI_IDE_UNDO][OSUI_IDE_SNAP_MAX];
} osui_ide_t;

void osui_ide_open(osui_ide_t *ide, const char *file, const char *text, int len);
int osui_ide_text(const osui_ide_t *ide, char *out, int max);
int osui_ide_key(osui_ide_t *ide, int key);
void osui_ide_set_status(osui_ide_t *ide, const char *msg);
void osui_ide_frame_row(const osui_ide_t *ide, int r, char *dst, int max);
/* Syntax class of every character of a source line; returns its length. */
int osui_ide_highlight(const char *line, unsigned char *cls, int max);
/* Class of every character of frame row r (same layout as frame_row). */
int osui_ide_frame_attr(const osui_ide_t *ide, int r, unsigned char *attr, int max);
/* Tab: complete the word before the cursor from the PromptMessage keywords
 * and the words of the buffer. Returns the number of candidates. */
int osui_ide_complete(osui_ide_t *ide);
/* Program output pane. */
void osui_ide_out_clear(osui_ide_t *ide);
void osui_ide_out_append(osui_ide_t *ide, const char *text);
int osui_ide_text_rows(const osui_ide_t *ide);

/* ---- plain text mode: the desktop text editor ----------------------- */
#include "osui_app.h"
void osui_ide_open_plain(osui_ide_t *ide, const char *file, const char *text, int len);
/* Toolbar button (OSUI_IDE_T_*): same actions as the ESC commands. */
int osui_ide_tool(osui_ide_t *ide, int button);
/* Mouse in the text grid of the window (row/col of the content grid).
 * click places the caret; drag selects from (r0,c0) to (r1,c1). */
void osui_ide_click(osui_ide_t *ide, int row, int col);
void osui_ide_drag(osui_ide_t *ide, int r0, int c0, int r1, int c1);
/* Selected text (0 when none) and the shared clipboard. */
int osui_ide_selection(const osui_ide_t *ide, char *out, int max);
const char *osui_ide_clipboard(void);
void osui_ide_set_clipboard(const char *text, int len);
/* UX4: route the clipboard through the shared desktop clipboard (the
 * kernel one the VBE terminal selection also uses); 0, 0 = local only. */
void osui_ide_set_clip_ops(int (*put)(const char *, int), int (*take)(char *, int));
/* Content grid of the window: line numbers, text, selection, caret. */
void osui_ide_render_app(const osui_ide_t *ide, osui_app_view_t *v);
/* Text to write: like osui_ide_text, an untouched empty buffer is 0 bytes. */
int osui_ide_save_text(const osui_ide_t *ide, char *out, int max);

/* UX5: heap for the line store (realloc-like, 0 = built-in store only). */
void osui_ide_set_heap(void *(*realloc_fn)(void *, unsigned));
/* UX5: text grid of the editor window (rows, cols incl. the gutter). */
void osui_ide_set_view(osui_ide_t *ide, int rows, int cols);
/* Bytes osui_ide_text needs (text + NUL). */
int osui_ide_text_size(const osui_ide_t *ide);

#endif
