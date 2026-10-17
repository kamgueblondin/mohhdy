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
} osui_ide_t;

void osui_ide_open(osui_ide_t *ide, const char *file, const char *text, int len);
int osui_ide_text(const osui_ide_t *ide, char *out, int max);
int osui_ide_key(osui_ide_t *ide, int key);
void osui_ide_set_status(osui_ide_t *ide, const char *msg);
void osui_ide_frame_row(const osui_ide_t *ide, int r, char *dst, int max);

#endif
