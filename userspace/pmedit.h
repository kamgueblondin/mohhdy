/* userspace/pmedit.h - full-screen PromptMessage editor model (US-050).
 * Pure C: text buffer, cursor, key handling and rendering into 80x25 VGA
 * text cells. The shell (pm-ide) feeds keys from SYS_GETC and blits the
 * cells with SYS_VGA_BLIT; saving and checking go through callbacks. */
#ifndef MOHHDY_PMEDIT_H
#define MOHHDY_PMEDIT_H
#include <stdint.h>

#define PME_COLS 80
#define PME_ROWS 25
#define PME_LINES 64
#define PME_LEN 76
#define PME_VIEW 21              /* text rows 2..22 */

enum { PME_NONE = 0, PME_SAVE, PME_QUIT, PME_CHECK };

typedef struct {
    char line[PME_LINES][PME_LEN + 1];
    int n;                       /* number of lines (>= 1) */
    int cy, cx, top;
    int modified;
    int esc;                     /* ESC pressed: next key is a command */
    char file[48];
    char status[PME_COLS + 1];
} pme_t;

void pme_load(pme_t* e, const char* file, const char* text, int len);
/* serialises the buffer ("\n" after each line); returns length or -1 */
int pme_text(const pme_t* e, char* out, int cap);
/* one key: printable, '\b', '\n', OS_VGA_KEY_* arrows, ESC then
 * s (save) c (check) q (quit) x (save and quit). Returns PME_* action. */
int pme_key(pme_t* e, int key);
void pme_status(pme_t* e, const char* text);
void pme_render(const pme_t* e, uint16_t* cells);
/* ASCII copy of one rendered row (tests, serial snapshot) */
void pme_row(const uint16_t* cells, int y, char* out, int cap);
#endif
