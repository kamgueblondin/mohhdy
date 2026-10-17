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

/* Text rows on screen: the output pane takes a rule and OSUI_IDE_OUT_VIEW rows. */
int osui_ide_text_rows(const osui_ide_t *ide) {
    if (ide && ide->out_visible && ide->out_n > 0) return OSUI_IDE_VIEW - 1 - OSUI_IDE_OUT_VIEW;
    return OSUI_IDE_VIEW;
}

static void ide_follow(osui_ide_t *ide) {
    int width = OSUI_IDE_ROW_LEN - 1 - 5; /* gutter "NNN> " */
    int rows = osui_ide_text_rows(ide);
    if (ide->row < ide->top) ide->top = ide->row;
    if (ide->row >= ide->top + rows) ide->top = ide->row - rows + 1;
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
        if (key == 'o') {
            if (ide->out_n == 0) { osui_ide_set_status(ide, "no program output yet: ESC r runs"); return OSUI_IDE_NONE; }
            ide->out_visible = !ide->out_visible;
            osui_ide_set_status(ide, ide->out_visible ? "output pane shown" : "output pane hidden");
            ide_follow(ide);
            return OSUI_IDE_NONE;
        }
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
        osui_ide_set_status(ide, "command: w save, c check, r run, o output, q quit, Q discard");
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
        (void)osui_ide_complete(ide);
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
        int rows = osui_ide_text_rows(ide);
        if (r == rows + 1) {
            ide_cpy(dst, max, "---- output ");
            ide_cat_u(dst, max, (unsigned)ide->out_total);
            ide_cat(dst, max, ide->out_total == 1 ? " line" : " lines");
            ide_cat(dst, max, " (ESC o hides) ----");
            return;
        }
        if (r > rows + 1) {
            int base = ide->out_n > OSUI_IDE_OUT_VIEW ? ide->out_n - OSUI_IDE_OUT_VIEW : 0;
            int idx = base + (r - rows - 2);
            if (idx < 0 || idx >= ide->out_n) { ide_cpy(dst, max, "    :"); return; }
            ide_cpy(dst, max, "    : ");
            ide_cat(dst, max, ide->out[idx]);
            return;
        }
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

/* ---- syntax coloring ------------------------------------------------ */

static const char *const ide_keywords[] = {
    "say", "print", "dit", "set", "to", "if", "then", "else", "end", "expect",
    "repeat", "times", "when", "user", "says", "file", "create", "append",
    "content", "with", "show", "stop", "use", "triggers", 0
};

static int ide_word_ch(char c) {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '_' || c == '.';
}

static int ide_is_keyword(const char *w, int n) {
    int i, k;
    for (i = 0; ide_keywords[i]; i++) {
        for (k = 0; k < n && ide_keywords[i][k] == w[k]; k++) {}
        if (k == n && ide_keywords[i][n] == 0) return 1;
    }
    return 0;
}

/* PromptMessage: a comment is a line whose first non-blank characters are
 * '#' or "//" (promptmessage.c); strings are double quoted with '\'
 * escapes; numbers may carry a decimal point and a trailing '%'. */
int osui_ide_highlight(const char *line, unsigned char *cls, int max) {
    int n = ide_len(line), i = 0, k;
    if (!cls || max <= 0) return -1;
    if (n > max) n = max;
    for (k = 0; k < n; k++) cls[k] = OSUI_IDE_TOK_TEXT;
    while (i < n && line[i] == ' ') i++;
    if (i < n && (line[i] == '#' || (line[i] == '/' && i + 1 < n && line[i + 1] == '/'))) {
        for (k = i; k < n; k++) cls[k] = OSUI_IDE_TOK_COMMENT;
        return n;
    }
    while (i < n) {
        char c = line[i];
        if (c == '"') {
            int j = i + 1;
            while (j < n && line[j] != '"') { if (line[j] == '\\' && j + 1 < n) j++; j++; }
            if (j < n) j++;
            for (k = i; k < j; k++) cls[k] = OSUI_IDE_TOK_STRING;
            i = j;
        } else if (c >= '0' && c <= '9') {
            int j = i;
            while (j < n && ((line[j] >= '0' && line[j] <= '9') || line[j] == '.')) j++;
            if (j < n && line[j] == '%') j++;
            if (j < n && ide_word_ch(line[j])) {           /* 2x, 3abc: a word */
                while (j < n && ide_word_ch(line[j])) j++;
            } else {
                for (k = i; k < j; k++) cls[k] = OSUI_IDE_TOK_NUMBER;
            }
            i = j;
        } else if (ide_word_ch(c)) {
            int j = i;
            while (j < n && ide_word_ch(line[j])) j++;
            if (ide_is_keyword(line + i, j - i))
                for (k = i; k < j; k++) cls[k] = OSUI_IDE_TOK_KEYWORD;
            i = j;
        } else {
            i++;
        }
    }
    return n;
}

int osui_ide_frame_attr(const osui_ide_t *ide, int r, unsigned char *attr, int max) {
    char row[OSUI_IDE_ROW_LEN];
    int n, k, rows;
    if (!attr || max <= 0) return -1;
    osui_ide_frame_row(ide, r, row, (int)sizeof(row));
    n = ide_len(row);
    if (n > max) n = max;
    for (k = 0; k < n; k++) attr[k] = OSUI_IDE_TOK_TEXT;
    if (!ide || !ide->active || r <= 0 || r >= OSUI_IDE_FRAME - 1) return n;
    rows = osui_ide_text_rows(ide);
    if (r == rows + 1) { for (k = 0; k < n; k++) attr[k] = OSUI_IDE_TOK_RULE; return n; }
    if (r > rows + 1) {
        for (k = 0; k < n; k++) attr[k] = k < 5 ? OSUI_IDE_TOK_GUTTER : OSUI_IDE_TOK_OUTPUT;
        return n;
    }
    {
        int line = ide->top + r - 1, c;
        unsigned char cls[OSUI_IDE_COLS + 1];
        int ll;
        if (line >= ide->nlines) { for (k = 0; k < n; k++) attr[k] = OSUI_IDE_TOK_GUTTER; return n; }
        ll = osui_ide_highlight(ide->lines[line], cls, OSUI_IDE_COLS);
        for (k = 0; k < n && k < 5; k++) attr[k] = OSUI_IDE_TOK_GUTTER;
        for (c = ide->left; k < n; k++, c++) attr[k] = c < ll ? cls[c] : OSUI_IDE_TOK_TEXT;
        /* the cursor mark '_' stands on a blank: plain text */
        if (line == ide->row && ide->col >= ide->left && 5 + ide->col - ide->left < n &&
            row[5 + ide->col - ide->left] == '_' && (ide->col >= ll || ide->lines[line][ide->col] == ' '))
            attr[5 + ide->col - ide->left] = OSUI_IDE_TOK_TEXT;
    }
    return n;
}

/* ---- completion ----------------------------------------------------- */

#define IDE_CAND_MAX 8
#define IDE_WORD_MAX 24

static int ide_cand_add(char cand[][IDE_WORD_MAX], int nc, const char *w, int wl, const char *pre, int pl) {
    int i, k;
    if (wl <= pl || wl >= IDE_WORD_MAX) return nc;
    for (k = 0; k < pl; k++) if (w[k] != pre[k]) return nc;
    for (i = 0; i < nc; i++) {
        for (k = 0; k < wl && cand[i][k] == w[k]; k++) {}
        if (k == wl && cand[i][wl] == 0) return nc;   /* already listed */
    }
    if (nc >= IDE_CAND_MAX) return nc + 1;            /* counted, not kept */
    for (k = 0; k < wl; k++) cand[nc][k] = w[k];
    cand[nc][wl] = 0;
    return nc + 1;
}

int osui_ide_complete(osui_ide_t *ide) {
    char pre[IDE_WORD_MAX], cand[IDE_CAND_MAX][IDE_WORD_MAX], msg[OSUI_IDE_ROW_LEN];
    const char *l;
    int start, pl, nc = 0, i, k, kept, common;
    if (!ide || !ide->active) return 0;
    l = ide->lines[ide->row];
    start = ide->col;
    while (start > 0 && ide_word_ch(l[start - 1])) start--;
    pl = ide->col - start;
    if (pl == 0 || pl >= IDE_WORD_MAX || (l[start] >= '0' && l[start] <= '9')) {
        ide_insert(ide, ' ');                         /* no word: indent */
        ide_insert(ide, ' ');
        return 0;
    }
    for (k = 0; k < pl; k++) pre[k] = l[start + k];
    pre[pl] = 0;
    for (i = 0; ide_keywords[i]; i++) nc = ide_cand_add(cand, nc, ide_keywords[i], ide_len(ide_keywords[i]), pre, pl);
    for (i = 0; i < ide->nlines; i++) {
        unsigned char cls[OSUI_IDE_COLS + 1];
        int n = osui_ide_highlight(ide->lines[i], cls, OSUI_IDE_COLS), j = 0;
        while (j < n) {
            int e = j;
            if (cls[j] != OSUI_IDE_TOK_TEXT || !ide_word_ch(ide->lines[i][j]) ||
                (ide->lines[i][j] >= '0' && ide->lines[i][j] <= '9')) { j++; continue; }
            while (e < n && cls[e] == OSUI_IDE_TOK_TEXT && ide_word_ch(ide->lines[i][e])) e++;
            if (!(i == ide->row && j == start))       /* not the word being typed */
                nc = ide_cand_add(cand, nc, ide->lines[i] + j, e - j, pre, pl);
            j = e;
        }
    }
    if (nc == 0) {
        ide_cpy(msg, (int)sizeof(msg), "no completion for ");
        ide_cat(msg, (int)sizeof(msg), pre);
        osui_ide_set_status(ide, msg);
        return 0;
    }
    kept = nc < IDE_CAND_MAX ? nc : IDE_CAND_MAX;
    common = ide_len(cand[0]);
    for (i = 1; i < kept; i++) {
        for (k = 0; k < common && cand[i][k] == cand[0][k]; k++) {}
        common = k;
    }
    if (nc > IDE_CAND_MAX) common = pl;              /* list incomplete: no guess */
    {
        int before = ide_len(l);
        for (k = pl; k < common; k++) ide_insert(ide, cand[0][k]);
        if (ide_len(l) - before < common - pl) return nc;   /* "line full" stays */
    }
    if (common > pl) ide->completions++;
    if (nc == 1) {
        ide_cpy(msg, (int)sizeof(msg), "completed ");
        ide_cat(msg, (int)sizeof(msg), cand[0]);
    } else {
        ide_cpy(msg, (int)sizeof(msg), "complete:");
        for (i = 0; i < kept; i++) { ide_cat(msg, (int)sizeof(msg), " "); ide_cat(msg, (int)sizeof(msg), cand[i]); }
        if (nc > kept) ide_cat(msg, (int)sizeof(msg), " ...");
    }
    osui_ide_set_status(ide, msg);
    return nc;
}

/* ---- program output pane -------------------------------------------- */

void osui_ide_out_clear(osui_ide_t *ide) {
    int i;
    if (!ide) return;
    for (i = 0; i < OSUI_IDE_OUT_LINES; i++) ide->out[i][0] = 0;
    ide->out_n = 0;
    ide->out_total = 0;
    ide_follow(ide);
}

static void ide_out_line(osui_ide_t *ide, const char *s, int n) {
    int i, k;
    if (ide->out_n == OSUI_IDE_OUT_LINES) {
        for (i = 0; i + 1 < OSUI_IDE_OUT_LINES; i++) ide_cpy(ide->out[i], OSUI_IDE_ROW_LEN, ide->out[i + 1]);
        ide->out_n--;
    }
    for (k = 0; k < n && k < OSUI_IDE_ROW_LEN - 1; k++) {
        char c = s[k];
        ide->out[ide->out_n][k] = ((unsigned char)c < 32 || (unsigned char)c > 126) ? ' ' : c;
    }
    ide->out[ide->out_n][k] = 0;
    ide->out_n++;
    ide->out_total++;
}

void osui_ide_out_append(osui_ide_t *ide, const char *text) {
    int i = 0, start = 0;
    if (!ide || !text) return;
    for (;; i++) {
        if (text[i] == '\n' || text[i] == 0) {
            if (text[i] == 0 && i == start && i > 0) break;  /* trailing newline */
            ide_out_line(ide, text + start, i - start);
            if (text[i] == 0) break;
            start = i + 1;
        }
    }
    ide->out_visible = 1;
    ide_follow(ide);
}
