#ifndef OSUI_IDE_H
#define OSUI_IDE_H

/* US-050: full-screen PromptMessage IDE surface of OS-UI. Pure model:
 * text buffer, cursor, scrolling, key bindings and the frame rows the
 * desktop draws. Save, check and run are actions returned to the caller
 * (osui_gui.c), which owns the file system and the PromptMessage VM. */

#define OSUI_IDE_LINES 96
#define OSUI_IDE_COLS 72      /* characters per line, without the NUL */
#define OSUI_IDE_VIEW 12      /* text rows on screen (frame = 14 rows) */
#define OSUI_IDE_FRAME 14     /* title + OSUI_IDE_VIEW + status */
#define OSUI_IDE_ROW_LEN 64   /* frame row buffer, NUL included */
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
    OSUI_IDE_QUIT = 4
};

typedef struct {
    int active;
    char file[48];
    char lines[OSUI_IDE_LINES][OSUI_IDE_COLS + 1];
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

#endif
