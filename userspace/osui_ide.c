/* US-050: full-screen PromptMessage IDE model for the OS-UI desktop.
 * See osui_ide.h. No libc: the guest build is freestanding. */
#include "osui_ide.h"

static int ide_len(const char *s) { int n = 0; while (s && s[n]) n++; return n; }

static void ide_cpy(char *d, int max, const char *s) {
    int i = 0;
    if (!d || max <= 0) return;
    while (s && s[i] && i < max - 1) { d[i] = s[i]; i++; }
    d[i] = 0;
}

static void ide_cat(char *d, int max, const char *s) {
    int n = ide_len(d);
    if (n < max) ide_cpy(d + n, max - n, s);
}

static void ide_cat_u(char *d, int max, unsigned v) {
    char b[12];
    int n = 0, i;
    char r[12];
    do { b[n++] = (char)('0' + v % 10U); v /= 10U; } while (v && n < 11);
    for (i = 0; i < n; i++) r[i] = b[n - 1 - i];
    r[n] = 0;
    ide_cat(d, max, r);
}

static void ide_follow(osui_ide_t *ide) {
    int width = OSUI_IDE_ROW_LEN - 1 - 5; /* gutter "NNN> " */
    if (ide->row < ide->top) ide->top = ide->row;
    if (ide->row >= ide->top + OSUI_IDE_VIEW) ide->top = ide->row - OSUI_IDE_VIEW + 1;
    if (ide->col < ide->left) ide->left = ide->col;
    if (ide->col >= ide->left + width) ide->left = ide->col - width + 1;
}

void osui_ide_set_status(osui_ide_t *ide, const char *msg) {
    if (ide) ide_cpy(ide->status, (int)sizeof(ide->status), msg);
}

void osui_ide_open(osui_ide_t *ide, const char *file, const char *text, int len) {
    int i, r = 0, c = 0;
    char *p = (char *)ide;
    for (i = 0; i < (int)sizeof(*ide); i++) p[i] = 0;
    ide->active = 1;
    ide_cpy(ide->file, (int)sizeof(ide->file), file);
    for (i = 0; text && i < len; i++) {
        char ch = text[i];
        if (ch == '\r') continue;
        if (ch == '\n') {
            if (r + 1 >= OSUI_IDE_LINES) { ide->truncated = 1; break; }
            r++; c = 0;
            continue;
        }
        if (ch == '\t') ch = ' ';
        if ((unsigned char)ch < 32) continue;
        if (c >= OSUI_IDE_COLS) { ide->truncated = 1; continue; }
        ide->lines[r][c++] = ch;
    }
    /* A final newline does not open an extra empty line. */
    ide->nlines = r + 1;
    if (ide->nlines > 1 && text && len > 0 && text[len - 1] == '\n' && ide->lines[r][0] == 0)
        ide->nlines = r;
    osui_ide_set_status(ide, ide->truncated ? "file truncated to the editor size"
                                            : "ESC then w save, c check, r run, q quit");
}

int osui_ide_text(const osui_ide_t *ide, char *out, int max) {
    int r, n = 0, k;
    if (!ide || !out || max <= 0) return -1;
    for (r = 0; r < ide->nlines; r++) {
        for (k = 0; ide->lines[r][k]; k++) {
            if (n >= max - 1) return -1;
            out[n++] = ide->lines[r][k];
        }
        if (n >= max - 1) return -1;
        out[n++] = '\n';
    }
    out[n] = 0;
    return n;
}

static void ide_insert(osui_ide_t *ide, char ch) {
    char *l = ide->lines[ide->row];
    int n = ide_len(l), i;
    if (n >= OSUI_IDE_COLS) { osui_ide_set_status(ide, "line full"); return; }
    for (i = n; i >= ide->col; i--) l[i + 1] = l[i];
    l[ide->col++] = ch;
    ide->modified = 1;
}

static void ide_newline(osui_ide_t *ide) {
    int i, k;
    char *l;
    if (ide->nlines >= OSUI_IDE_LINES) { osui_ide_set_status(ide, "buffer full"); return; }
    for (i = ide->nlines; i > ide->row + 1; i--) ide_cpy(ide->lines[i], OSUI_IDE_COLS + 1, ide->lines[i - 1]);
    l = ide->lines[ide->row];
    ide_cpy(ide->lines[ide->row + 1], OSUI_IDE_COLS + 1, l + ide->col);
    l[ide->col] = 0;
    for (k = ide_len(ide->lines[ide->row + 1]) + 1; k <= OSUI_IDE_COLS; k++) ide->lines[ide->row + 1][k] = 0;
    ide->nlines++;
    ide->row++;
    ide->col = 0;
    ide->modified = 1;
}

static void ide_backspace(osui_ide_t *ide) {
    char *l = ide->lines[ide->row];
    int i;
    if (ide->col > 0) {
        for (i = ide->col - 1; l[i]; i++) l[i] = l[i + 1];
        ide->col--;
        ide->modified = 1;
        return;
    }
    if (ide->row == 0) return;
    {
        char *prev = ide->lines[ide->row - 1];
        int pn = ide_len(prev), n = ide_len(l);
        if (pn + n > OSUI_IDE_COLS) { osui_ide_set_status(ide, "joined line too long"); return; }
        ide_cat(prev, OSUI_IDE_COLS + 1, l);
        for (i = ide->row; i + 1 < ide->nlines; i++) ide_cpy(ide->lines[i], OSUI_IDE_COLS + 1, ide->lines[i + 1]);
        ide->lines[ide->nlines - 1][0] = 0;
        ide->nlines--;
        ide->row--;
        ide->col = pn;
        ide->modified = 1;
    }
}

int osui_ide_key(osui_ide_t *ide, int key) {
    int n;
    if (!ide || !ide->active) return OSUI_IDE_NONE;
    if (ide->cmd_mode) {
        ide->cmd_mode = 0;
        if (key == 'w') return OSUI_IDE_SAVE;
        if (key == 'c') return OSUI_IDE_CHECK;
        if (key == 'r') return OSUI_IDE_RUN;
        if (key == 'Q') return OSUI_IDE_QUIT;
        if (key == 'q') {
            if (ide->modified) {
                osui_ide_set_status(ide, "unsaved: ESC w to save, ESC Q to discard");
                return OSUI_IDE_NONE;
            }
            return OSUI_IDE_QUIT;
        }
        osui_ide_set_status(ide, "command cancelled");
        return OSUI_IDE_NONE;
    }
    if (key == OSUI_IDE_KEY_ESC) {
        ide->cmd_mode = 1;
        osui_ide_set_status(ide, "command: w save, c check, r run, q quit, Q discard");
        return OSUI_IDE_NONE;
    }
    n = ide_len(ide->lines[ide->row]);
    if (key == OSUI_IDE_KEY_LEFT) {
        if (ide->col > 0) ide->col--;
        else if (ide->row > 0) { ide->row--; ide->col = ide_len(ide->lines[ide->row]); }
    } else if (key == OSUI_IDE_KEY_RIGHT) {
        if (ide->col < n) ide->col++;
        else if (ide->row + 1 < ide->nlines) { ide->row++; ide->col = 0; }
    } else if (key == OSUI_IDE_KEY_UP) {
        if (ide->row > 0) ide->row--;
    } else if (key == OSUI_IDE_KEY_DOWN) {
        if (ide->row + 1 < ide->nlines) ide->row++;
    } else if (key == '\n' || key == '\r') {
        ide_newline(ide);
    } else if (key == '\b' || key == 127) {
        ide_backspace(ide);
    } else if (key == '\t') {
        ide_insert(ide, ' ');
        ide_insert(ide, ' ');
    } else if (key >= 32 && key <= 126) {
        ide_insert(ide, (char)key);
    } else {
        return OSUI_IDE_NONE;
    }
    n = ide_len(ide->lines[ide->row]);
    if (ide->col > n) ide->col = n;
    ide_follow(ide);
    return OSUI_IDE_NONE;
}

/* Frame row r: 0 title, 1..OSUI_IDE_VIEW text with a line-number gutter
 * (the cursor line is marked '>' and its column '_' when it is on a
 * space or past the end), last row the status line. */
void osui_ide_frame_row(const osui_ide_t *ide, int r, char *dst, int max) {
    if (!dst || max <= 0) return;
    dst[0] = 0;
    if (!ide || !ide->active) return;
    if (r == 0) {
        ide_cpy(dst, max, "pm-ide ");
        ide_cat(dst, max, ide->file);
        ide_cat(dst, max, ide->modified ? " [+]" : "");
        ide_cat(dst, max, " Ln ");
        ide_cat_u(dst, max, (unsigned)(ide->row + 1));
        ide_cat(dst, max, " Col ");
        ide_cat_u(dst, max, (unsigned)(ide->col + 1));
        ide_cat(dst, max, " /");
        ide_cat_u(dst, max, (unsigned)ide->nlines);
        return;
    }
    if (r == OSUI_IDE_FRAME - 1) {
        ide_cpy(dst, max, ide->cmd_mode ? "CMD " : "");
        ide_cat(dst, max, ide->status);
        return;
    }
    {
        int line = ide->top + r - 1, k = 0, c, i;
        char num[8];
        if (line >= ide->nlines) { ide_cpy(dst, max, "   ~"); return; }
        num[0] = 0;
        ide_cat_u(num, (int)sizeof(num), (unsigned)(line + 1));
        for (i = ide_len(num); i < 3 && k < max - 1; i++) dst[k++] = ' ';
        for (i = 0; num[i] && k < max - 1; i++) dst[k++] = num[i];
        if (k < max - 1) dst[k++] = line == ide->row ? '>' : '|';
        if (k < max - 1) dst[k++] = ' ';
        for (c = ide->left; k < max - 1; c++) {
            char ch = c < OSUI_IDE_COLS + 1 ? ide->lines[line][c] : 0;
            if (line == ide->row && c == ide->col && (ch == 0 || ch == ' ')) { dst[k++] = '_'; if (!ch) break; continue; }
            if (!ch) break;
            dst[k++] = ch;
        }
        dst[k] = 0;
    }
}
