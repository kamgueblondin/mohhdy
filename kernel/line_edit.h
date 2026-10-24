/* line_edit.h - one line editor for SYS_GETS, shared by the VGA text
 * console and the VBE terminal window (same vga_console underneath).
 * Pure logic: output goes through callbacks so it is host testable. */
#ifndef LINE_EDIT_H
#define LINE_EDIT_H

#define LE_MAX 256
#define LE_HIST 16

#define LE_KEY_LEFT 28   /* OS_VGA_KEY_LEFT */
#define LE_KEY_RIGHT 29  /* OS_VGA_KEY_RIGHT */
#define LE_KEY_UP 30     /* OS_VGA_KEY_UP */
#define LE_KEY_DOWN 31   /* OS_VGA_KEY_DOWN */
#define LE_KEY_HOME 0x12 /* OS_TERM_KEY_HOME */
#define LE_KEY_END 0x13  /* OS_TERM_KEY_END */
#define LE_KEY_DEL 0x14  /* OS_TERM_KEY_DEL */

typedef struct {
    void (*put)(char c);       /* print one char, cursor moves right */
    void (*left)(void);        /* move the cursor one cell left (no erase) */
    void (*reprompt)(void);    /* reprint the prompt after a completion list */
} line_edit_io_t;

typedef struct {
    char buf[LE_MAX];
    int len, cur, size;
    int hist_pos;              /* -1: editing the live line */
    char saved[LE_MAX];        /* live line while browsing history */
    int last_tab;
    int no_hist;               /* do not record this line */
    const char *const *words;
    int nwords;
    const line_edit_io_t *io;
} line_edit_t;

void line_edit_begin(line_edit_t *le, int size, const char *const *words, int nwords,
                     const line_edit_io_t *io);
/* Feed one key. Returns 1 when the line is complete (Enter): buf holds it
 * NUL terminated and it was added to the history. */
int line_edit_key(line_edit_t *le, char c);
/* Replace the whole line (used by the terminal close key). */
void line_edit_set(line_edit_t *le, const char *s);
void line_edit_history_reset(void);
int line_edit_history_count(void);

#endif
