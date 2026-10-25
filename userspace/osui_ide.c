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

static void *(*g_heap)(void *, unsigned);
void osui_ide_set_heap(void *(*realloc_fn)(void *, unsigned)) { g_heap = realloc_fn; }

/* Line store that holds n lines: the built-in one, else the heap (UX5). */
static int ide_reserve(osui_ide_t *ide, int n) {
    int cap, i, k;
    char (*m)[OSUI_IDE_COLS + 1];
    if (!ide->lines) { ide->lines = ide->lines0; ide->cap = OSUI_IDE_LINES; }
    if (n <= ide->cap) return 0;
    if (!g_heap) return -1;
    cap = ide->cap * 2;
    while (cap < n) cap *= 2;
    if (ide->lines == ide->lines0) {
        m = (char (*)[OSUI_IDE_COLS + 1])g_heap(0, (unsigned)cap * (OSUI_IDE_COLS + 1));
        if (!m) return -1;
        for (i = 0; i < ide->cap; i++) for (k = 0; k <= OSUI_IDE_COLS; k++) m[i][k] = ide->lines0[i][k];
    } else {
        m = (char (*)[OSUI_IDE_COLS + 1])g_heap(ide->lines, (unsigned)cap * (OSUI_IDE_COLS + 1));
        if (!m) return -1;
    }
    for (i = ide->cap; i < cap; i++) for (k = 0; k <= OSUI_IDE_COLS; k++) m[i][k] = 0;
    ide->lines = m;
    ide->cap = cap;
    return 0;
}

static int plain_rows(const osui_ide_t *ide) {
    int r = ide->view_rows > 0 ? ide->view_rows : OSUI_APP_ROWS;
    return r > OSUI_APP_MAX_ROWS ? OSUI_APP_MAX_ROWS : r;
}
static int plain_cols(const osui_ide_t *ide) {
    int c = ide->view_cols > 0 ? ide->view_cols : OSUI_APP_COLS - 1;
    return c > OSUI_APP_MAX_COLS - 1 ? OSUI_APP_MAX_COLS - 1 : c;
}

void osui_ide_set_view(osui_ide_t *ide, int rows, int cols) {
    if (!ide) return;
    ide->view_rows = rows < 3 ? 3 : rows;
    ide->view_cols = cols < 12 ? 12 : cols;
}

static void ide_follow(osui_ide_t *ide) {
    int width = ide->plain ? plain_cols(ide) - 5 : OSUI_IDE_ROW_LEN - 1 - 5; /* gutter "NNN> " */
    int rows = ide->plain ? plain_rows(ide) : osui_ide_text_rows(ide);
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
    char (*keep)[OSUI_IDE_COLS + 1] = ide->lines;
    int keep_cap = ide->cap, vr = ide->view_rows, vc = ide->view_cols, k;
    for (i = 0; i < (int)sizeof(*ide); i++) p[i] = 0;
    ide->view_rows = vr; ide->view_cols = vc;
    if (keep && keep != ide->lines0 && keep_cap > OSUI_IDE_LINES && g_heap) {
        ide->lines = keep; ide->cap = keep_cap;   /* reuse the heap store */
        for (i = 0; i < keep_cap; i++) for (k = 0; k <= OSUI_IDE_COLS; k++) keep[i][k] = 0;
    } else { ide->lines = ide->lines0; ide->cap = OSUI_IDE_LINES; }
    ide->active = 1;
    ide_cpy(ide->file, (int)sizeof(ide->file), file);
    for (i = 0; text && i < len; i++) {
        char ch = text[i];
        if (ch == '\r') continue;
        if (ch == '\n') {
            if (ide_reserve(ide, r + 2) != 0) { ide->truncated = 1; break; }
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
    if (ide_reserve(ide, ide->nlines + 1) != 0) { osui_ide_set_status(ide, "buffer full"); return; }
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

static int plain_key(osui_ide_t *ide, int key);

int osui_ide_key(osui_ide_t *ide, int key) {
    int n;
    if (!ide || !ide->active) return OSUI_IDE_NONE;
    if (ide->plain) return plain_key(ide, key);
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

/* ==== plain text mode: the desktop text editor ======================== */

const char osui_ide_toolbar[] =
    "[Ouvrir] [Enreg] [Sous] [Couper] [Copier] [Coller] [Annul] [Retab] [Cherch]";

static char g_clip[OSUI_IDE_TEXT_MAX];
static int g_clip_len;
/* UX4: the shared desktop clipboard (kernel SYS_CLIPBOARD), when set */
static int (*g_clip_put)(const char *text, int len);
static int (*g_clip_take)(char *out, int max);

void osui_ide_set_clip_ops(int (*put)(const char *, int), int (*take)(char *, int)) {
    g_clip_put = put;
    g_clip_take = take;
}

const char *osui_ide_clipboard(void) {
    if (g_clip_take) {
        int n = g_clip_take(g_clip, (int)sizeof(g_clip));
        g_clip_len = n > 0 ? n : 0;
        g_clip[g_clip_len] = 0;
    }
    return g_clip;
}

void osui_ide_set_clipboard(const char *text, int len) {
    int i;
    if (len < 0) len = 0;
    if (len > OSUI_IDE_TEXT_MAX - 1) len = OSUI_IDE_TEXT_MAX - 1;
    for (i = 0; i < len; i++) g_clip[i] = text[i];
    g_clip[len] = 0;
    g_clip_len = len;
    if (g_clip_put) (void)g_clip_put(g_clip, len);
}

static void reload_text(osui_ide_t *ide, const char *text, int row, int col) {
    int r = 0, c = 0, i, k;
    for (r = 0; r < ide->cap; r++) for (k = 0; k <= OSUI_IDE_COLS; k++) ide->lines[r][k] = 0;
    r = 0;
    for (i = 0; text[i]; i++) {
        if (text[i] == '\n') { if (ide_reserve(ide, r + 2) != 0) break; r++; c = 0; continue; }
        if (c < OSUI_IDE_COLS) ide->lines[r][c++] = text[i];
    }
    ide->nlines = r + 1;
    if (ide->nlines > 1 && i > 0 && text[i - 1] == '\n' && ide->lines[r][0] == 0) ide->nlines = r;
    ide->row = row < ide->nlines ? row : ide->nlines - 1;
    ide->col = col;
    if (ide->col > ide_len(ide->lines[ide->row])) ide->col = ide_len(ide->lines[ide->row]);
    ide->mark_on = 0;
    ide_follow(ide);
}

static void snap_push(char stack[][OSUI_IDE_SNAP_MAX], int rc[][2], int *n, const osui_ide_t *ide) {
    int i, k;
    if (*n == OSUI_IDE_UNDO) {
        for (i = 0; i + 1 < OSUI_IDE_UNDO; i++) {
            for (k = 0; k < OSUI_IDE_SNAP_MAX; k++) stack[i][k] = stack[i + 1][k];
            rc[i][0] = rc[i + 1][0]; rc[i][1] = rc[i + 1][1];
        }
        (*n)--;
    }
    if (osui_ide_text(ide, stack[*n], OSUI_IDE_SNAP_MAX) < 0) stack[*n][0] = 0;
    rc[*n][0] = ide->row; rc[*n][1] = ide->col;
    (*n)++;
}

/* Before an edit: one undo step per edit, typed characters grouped. */
static void undo_point(osui_ide_t *ide, int kind) {
    if (kind == 1 && ide->last_edit == 1) return;
    if (osui_ide_text_size(ide) > OSUI_IDE_SNAP_MAX) {
        /* UX5: a snapshot would be cut; no undo rather than a wrong one */
        ide->undo_off = 1; ide->undo_n = ide->redo_n = 0; ide->last_edit = kind;
        return;
    }
    ide->undo_off = 0;
    snap_push(ide->undo, ide->undo_rc, &ide->undo_n, ide);
    ide->redo_n = 0;
    ide->last_edit = kind;
}

static void do_undo(osui_ide_t *ide, int redo) {
    char (*from)[OSUI_IDE_SNAP_MAX] = redo ? ide->redo : ide->undo;
    int (*frc)[2] = redo ? ide->redo_rc : ide->undo_rc;
    int *fn = redo ? &ide->redo_n : &ide->undo_n;
    if (*fn == 0 && ide->undo_off) { osui_ide_set_status(ide, "annuler indisponible: fichier > 8 Kio"); return; }
    if (*fn == 0) { osui_ide_set_status(ide, redo ? "rien a retablir" : "rien a annuler"); return; }
    if (redo) snap_push(ide->undo, ide->undo_rc, &ide->undo_n, ide);
    else snap_push(ide->redo, ide->redo_rc, &ide->redo_n, ide);
    (*fn)--;
    reload_text(ide, from[*fn], frc[*fn][0], frc[*fn][1]);
    ide->modified = 1;
    ide->last_edit = 0;
    osui_ide_set_status(ide, redo ? "retabli" : "annule");
}

static int pos_less(int r0, int c0, int r1, int c1) { return r0 < r1 || (r0 == r1 && c0 < c1); }

static int sel_range(const osui_ide_t *ide, int *r0, int *c0, int *r1, int *c1) {
    if (!ide->mark_on) return 0;
    if (ide->mark_row == ide->row && ide->mark_col == ide->col) return 0;
    if (pos_less(ide->mark_row, ide->mark_col, ide->row, ide->col)) {
        *r0 = ide->mark_row; *c0 = ide->mark_col; *r1 = ide->row; *c1 = ide->col;
    } else {
        *r0 = ide->row; *c0 = ide->col; *r1 = ide->mark_row; *c1 = ide->mark_col;
    }
    return 1;
}

int osui_ide_selection(const osui_ide_t *ide, char *out, int max) {
    int r0, c0, r1, c1, r, n = 0;
    if (!ide || !out || max <= 0 || !sel_range(ide, &r0, &c0, &r1, &c1)) { if (out && max > 0) out[0] = 0; return 0; }
    for (r = r0; r <= r1; r++) {
        int a = r == r0 ? c0 : 0, b = r == r1 ? c1 : ide_len(ide->lines[r]), k;
        for (k = a; k < b && n < max - 1; k++) out[n++] = ide->lines[r][k];
        if (r < r1 && n < max - 1) out[n++] = '\n';
    }
    out[n] = 0;
    return n;
}

static int delete_range(osui_ide_t *ide, int r0, int c0, int r1, int c1) {
    char *a = ide->lines[r0];
    const char *b = ide->lines[r1] + c1;
    int bl = ide_len(b), i, k;
    if (c0 + bl > OSUI_IDE_COLS) { osui_ide_set_status(ide, "ligne trop longue"); return -1; }
    for (k = 0; k <= bl; k++) a[c0 + k] = b[k];
    for (k = c0 + bl + 1; k <= OSUI_IDE_COLS; k++) a[k] = 0;
    if (r1 > r0) {
        int gone = r1 - r0;
        for (i = r0 + 1; i + gone < ide->nlines; i++) ide_cpy(ide->lines[i], OSUI_IDE_COLS + 1, ide->lines[i + gone]);
        for (i = ide->nlines - gone; i < ide->nlines; i++) for (k = 0; k <= OSUI_IDE_COLS; k++) ide->lines[i][k] = 0;
        ide->nlines -= gone;
    }
    ide->row = r0; ide->col = c0;
    ide->mark_on = 0;
    ide->modified = 1;
    return 0;
}

static int delete_selection(osui_ide_t *ide) {
    int r0, c0, r1, c1;
    if (!sel_range(ide, &r0, &c0, &r1, &c1)) return 0;
    return delete_range(ide, r0, c0, r1, c1) == 0 ? 1 : -1;
}

static void insert_text(osui_ide_t *ide, const char *t) {
    for (; t && *t; t++) {
        if (*t == '\n') ide_newline(ide);
        else if ((unsigned char)*t >= 32 && (unsigned char)*t <= 126) ide_insert(ide, *t);
    }
}

static int find_next(osui_ide_t *ide) {
    int fl = ide_len(ide->find), r, start_c, pass;
    if (fl == 0) { osui_ide_set_status(ide, "rien a chercher"); return 0; }
    for (pass = 0; pass < 2; pass++) {
        for (r = pass ? 0 : ide->row; r < ide->nlines; r++) {
            const char *l = ide->lines[r];
            int n = ide_len(l), c;
            start_c = (!pass && r == ide->row) ? ide->col : 0;
            for (c = start_c; c + fl <= n; c++) {
                int k = 0;
                while (k < fl && l[c + k] == ide->find[k]) k++;
                if (k == fl) {
                    char msg[OSUI_IDE_ROW_LEN];
                    ide->mark_on = 1; ide->mark_row = r; ide->mark_col = c;
                    ide->row = r; ide->col = c + fl;
                    ide_follow(ide);
                    ide_cpy(msg, (int)sizeof(msg), "trouve ligne ");
                    ide_cat_u(msg, (int)sizeof(msg), (unsigned)(r + 1));
                    osui_ide_set_status(ide, msg);
                    return 1;
                }
            }
        }
    }
    osui_ide_set_status(ide, "introuvable");
    return 0;
}

static void do_copy(osui_ide_t *ide, int cut) {
    static char tmp[OSUI_IDE_TEXT_MAX];
    int n = osui_ide_selection(ide, tmp, (int)sizeof(tmp));
    if (n <= 0) { osui_ide_set_status(ide, "pas de selection (ESC m pose la marque)"); return; }
    osui_ide_set_clipboard(tmp, n);
    if (cut) { undo_point(ide, 2); (void)delete_selection(ide); osui_ide_set_status(ide, "coupe"); }
    else osui_ide_set_status(ide, "copie");
    ide_follow(ide);
}

static void do_paste(osui_ide_t *ide) {
    (void)osui_ide_clipboard();   /* the terminal or the explorer may have copied since */
    if (!g_clip_len) { osui_ide_set_status(ide, "presse-papiers vide"); return; }
    undo_point(ide, 3);
    if (delete_selection(ide) < 0) return;
    insert_text(ide, g_clip);
    ide->mark_on = 0;
    ide_follow(ide);
    osui_ide_set_status(ide, "colle");
}

static void start_prompt(osui_ide_t *ide, int which, const char *initial) {
    ide->prompt = which;
    ide_cpy(ide->prompt_buf, (int)sizeof(ide->prompt_buf), initial ? initial : "");
}

static int quit_request(osui_ide_t *ide) {
    if (ide->modified && !ide->quit_armed) {
        ide->quit_armed = 1;
        osui_ide_set_status(ide, "non enregistre: ESC w enregistre, ESC Q (ou Fermer encore) abandonne");
        return OSUI_IDE_NONE;
    }
    return OSUI_IDE_QUIT;
}

int osui_ide_tool(osui_ide_t *ide, int b) {
    if (!ide || !ide->active || !ide->plain) return OSUI_IDE_NONE;
    ide->prompt = OSUI_IDE_P_NONE;
    switch (b) {
    case OSUI_IDE_T_OPEN: start_prompt(ide, OSUI_IDE_P_OPEN, ""); return OSUI_IDE_NONE;
    case OSUI_IDE_T_SAVE:
        if (ide->truncated) { osui_ide_set_status(ide, "fichier tronque a l'ouverture: Enreg. sous un autre nom"); return OSUI_IDE_NONE; }
        return OSUI_IDE_SAVE;
    case OSUI_IDE_T_SAVE_AS: start_prompt(ide, OSUI_IDE_P_SAVE_AS, ide->file); return OSUI_IDE_NONE;
    case OSUI_IDE_T_CUT: do_copy(ide, 1); return OSUI_IDE_NONE;
    case OSUI_IDE_T_COPY: do_copy(ide, 0); return OSUI_IDE_NONE;
    case OSUI_IDE_T_PASTE: do_paste(ide); return OSUI_IDE_NONE;
    case OSUI_IDE_T_UNDO: do_undo(ide, 0); return OSUI_IDE_NONE;
    case OSUI_IDE_T_REDO: do_undo(ide, 1); return OSUI_IDE_NONE;
    case OSUI_IDE_T_FIND: start_prompt(ide, OSUI_IDE_P_FIND, ide->find); return OSUI_IDE_NONE;
    case OSUI_IDE_T_MARK:
        ide->mark_on = !ide->mark_on;
        ide->mark_row = ide->row; ide->mark_col = ide->col;
        osui_ide_set_status(ide, ide->mark_on ? "marque posee: deplacez le curseur pour selectionner" : "marque retiree");
        return OSUI_IDE_NONE;
    default: return OSUI_IDE_NONE;
    }
}

static int prompt_key(osui_ide_t *ide, int key) {
    int n = ide_len(ide->prompt_buf), which = ide->prompt;
    if (key == OSUI_IDE_KEY_ESC) { ide->prompt = OSUI_IDE_P_NONE; osui_ide_set_status(ide, "annule"); return OSUI_IDE_NONE; }
    if (key == '\b' || key == 127) { if (n > 0) ide->prompt_buf[n - 1] = 0; return OSUI_IDE_NONE; }
    if (key >= 32 && key <= 126) {
        if (n < (int)sizeof(ide->prompt_buf) - 1) { ide->prompt_buf[n] = (char)key; ide->prompt_buf[n + 1] = 0; }
        return OSUI_IDE_NONE;
    }
    if (key != '\n' && key != '\r') return OSUI_IDE_NONE;
    ide->prompt = OSUI_IDE_P_NONE;
    if (!ide->prompt_buf[0]) { osui_ide_set_status(ide, "annule"); return OSUI_IDE_NONE; }
    if (which == OSUI_IDE_P_FIND) { ide_cpy(ide->find, (int)sizeof(ide->find), ide->prompt_buf); (void)find_next(ide); return OSUI_IDE_NONE; }
    ide_cpy(ide->prompt_out, (int)sizeof(ide->prompt_out), ide->prompt_buf);
    if (which == OSUI_IDE_P_SAVE_AS) { ide_cpy(ide->file, (int)sizeof(ide->file), ide->prompt_buf); ide->truncated = 0; return OSUI_IDE_SAVE_AS; }
    if (which == OSUI_IDE_P_OPEN) {
        if (ide->modified) { osui_ide_set_status(ide, "non enregistre: enregistrez avant d'ouvrir un autre fichier"); return OSUI_IDE_NONE; }
        return OSUI_IDE_OPEN;
    }
    return OSUI_IDE_NONE;
}

static int plain_key(osui_ide_t *ide, int key) {
    int n, moved = 0;
    if (ide->prompt) return prompt_key(ide, key);
    if (ide->cmd_mode) {
        ide->cmd_mode = 0;
        switch (key) {
        case 'w': return osui_ide_tool(ide, OSUI_IDE_T_SAVE);
        case 'a': return osui_ide_tool(ide, OSUI_IDE_T_SAVE_AS);
        case 'o': return osui_ide_tool(ide, OSUI_IDE_T_OPEN);
        case 'x': return osui_ide_tool(ide, OSUI_IDE_T_CUT);
        case 'c': return osui_ide_tool(ide, OSUI_IDE_T_COPY);
        case 'v': return osui_ide_tool(ide, OSUI_IDE_T_PASTE);
        case 'u': return osui_ide_tool(ide, OSUI_IDE_T_UNDO);
        case 'y': return osui_ide_tool(ide, OSUI_IDE_T_REDO);
        case 'f': return osui_ide_tool(ide, OSUI_IDE_T_FIND);
        case 'n': (void)find_next(ide); return OSUI_IDE_NONE;
        case 'm': return osui_ide_tool(ide, OSUI_IDE_T_MARK);
        case 'q': return quit_request(ide);
        case 'Q': return OSUI_IDE_QUIT;
        case 'r': return OSUI_IDE_RUN;    /* desktop: run the .pm file (PromptMessage window) */
        default: osui_ide_set_status(ide, "commande annulee"); return OSUI_IDE_NONE;
        }
    }
    if (key == OSUI_IDE_KEY_ESC) {
        ide->cmd_mode = 1;
        osui_ide_set_status(ide, "ESC: w enreg. a sous o ouvrir x c v couper/copier/coller u y annuler/retablir f n chercher m marque q");
        return OSUI_IDE_NONE;
    }
    ide->quit_armed = 0;
    n = ide_len(ide->lines[ide->row]);
    switch (key) {
    case OSUI_IDE_KEY_LEFT:
        if (ide->col > 0) ide->col--;
        else if (ide->row > 0) { ide->row--; ide->col = ide_len(ide->lines[ide->row]); }
        moved = 1; break;
    case OSUI_IDE_KEY_RIGHT:
        if (ide->col < n) ide->col++;
        else if (ide->row + 1 < ide->nlines) { ide->row++; ide->col = 0; }
        moved = 1; break;
    case OSUI_IDE_KEY_UP: if (ide->row > 0) ide->row--; moved = 1; break;
    case OSUI_IDE_KEY_DOWN: if (ide->row + 1 < ide->nlines) ide->row++; moved = 1; break;
    case OSUI_IDE_KEY_HOME: ide->col = 0; moved = 1; break;
    case OSUI_IDE_KEY_END: ide->col = n; moved = 1; break;
    case '\n': case '\r':
        undo_point(ide, 2);
        if (delete_selection(ide) < 0) break;
        ide_newline(ide);
        break;
    case '\b': case 127:
        undo_point(ide, 2);
        { int d = delete_selection(ide); if (d == 0) ide_backspace(ide); }
        break;
    case OSUI_IDE_KEY_DEL:
        undo_point(ide, 2);
        {
            int d = delete_selection(ide);
            if (d == 0) {
                if (ide->col < n) { ide->col++; ide_backspace(ide); }
                else if (ide->row + 1 < ide->nlines) { ide->row++; ide->col = 0; ide_backspace(ide); }
            }
        }
        break;
    case '\t':
        undo_point(ide, 1);
        if (delete_selection(ide) < 0) break;
        ide_insert(ide, ' '); ide_insert(ide, ' '); ide_insert(ide, ' '); ide_insert(ide, ' ');
        break;
    default:
        if (key < 32 || key > 126) return OSUI_IDE_NONE;
        undo_point(ide, 1);
        if (delete_selection(ide) < 0) break;
        ide_insert(ide, (char)key);
        break;
    }
    if (moved) ide->last_edit = 0;
    n = ide_len(ide->lines[ide->row]);
    if (ide->col > n) ide->col = n;
    ide_follow(ide);
    return OSUI_IDE_NONE;
}

void osui_ide_open_plain(osui_ide_t *ide, const char *file, const char *text, int len) {
    osui_ide_open(ide, file, text, len);
    ide->plain = 1;
    osui_ide_set_status(ide, ide->truncated ? "fichier tronque (lignes > 72 colonnes ou memoire pleine)"
                                            : "ESC puis w enregistre, q ferme; barre d'outils cliquable");
}

#define APP_GUTTER 5   /* "NNNN " */
#define APP_TEXT_ROWS (plain_rows(ide))

static void plain_follow_app(osui_ide_t *ide) {
    int width = plain_cols(ide) - APP_GUTTER;
    if (ide->row < ide->top) ide->top = ide->row;
    if (ide->row >= ide->top + APP_TEXT_ROWS) ide->top = ide->row - APP_TEXT_ROWS + 1;
    if (ide->col < ide->left) ide->left = ide->col;
    if (ide->col >= ide->left + width) ide->left = ide->col - width + 1;
}

void osui_ide_click(osui_ide_t *ide, int row, int col) {
    int line, c, n;
    if (!ide || !ide->active) return;
    line = ide->top + row;
    if (line >= ide->nlines) line = ide->nlines - 1;
    if (line < 0) line = 0;
    c = ide->left + col - APP_GUTTER;
    if (c < 0) c = 0;
    n = ide_len(ide->lines[line]);
    if (c > n) c = n;
    ide->row = line; ide->col = c;
    ide->mark_on = 0;
    ide->last_edit = 0;
}

void osui_ide_drag(osui_ide_t *ide, int r0, int c0, int r1, int c1) {
    int mr, mc;
    if (!ide || !ide->active) return;
    osui_ide_click(ide, r0, c0);
    mr = ide->row; mc = ide->col;
    osui_ide_click(ide, r1, c1);
    if (mr != ide->row || mc != ide->col) { ide->mark_on = 1; ide->mark_row = mr; ide->mark_col = mc; }
}

int osui_ide_text_size(const osui_ide_t *ide) {
    int r, n = 1;
    if (!ide || !ide->lines) return 1;
    for (r = 0; r < ide->nlines; r++) n += ide_len(ide->lines[r]) + 1;
    return n;
}

int osui_ide_save_text(const osui_ide_t *ide, char *out, int max) {
    int n = osui_ide_text(ide, out, max);
    if (n == 1 && ide->nlines == 1 && ide->lines[0][0] == 0) { out[0] = 0; return 0; }
    return n;
}

void osui_ide_render_app(const osui_ide_t *ide0, osui_app_view_t *v) {
    osui_ide_t *ide = (osui_ide_t *)ide0;   /* scroll follows the window size */
    int r, r0 = 0, c0 = 0, r1 = 0, c1 = 0, has;
    {
        int i; char *p = (char *)v;
        for (i = 0; i < (int)sizeof(*v); i++) p[i] = 0;
        v->cur_row = v->cur_col = -1;
    }
    if (!ide || !ide->active) return;
    plain_follow_app(ide);
    ide_cpy(v->title, (int)sizeof(v->title), "Editeur - ");
    ide_cat(v->title, (int)sizeof(v->title), ide->file);
    if (ide->modified) ide_cat(v->title, (int)sizeof(v->title), " *");
    v->dirty = ide->modified;
    ide_cpy(v->tool, OSUI_APP_COLS, osui_ide_toolbar);
    has = sel_range(ide, &r0, &c0, &r1, &c1);
    for (r = 0; r < APP_TEXT_ROWS; r++) {
        int line = ide->top + r, k = 0, c;
        char num[8];
        char *d = v->rows[r];
        if (line >= ide->nlines) { ide_cpy(d, OSUI_APP_MAX_COLS, "   ~"); for (k = 0; k < 4; k++) v->attr[r][k] = OSUI_APP_A_DIM; continue; }
        num[0] = 0;
        ide_cat_u(num, (int)sizeof(num), (unsigned)(line + 1));
        for (c = ide_len(num); c < 4; c++) d[k++] = ' ';
        for (c = 0; num[c]; c++) d[k++] = num[c];
        d[k++] = ' ';
        for (c = 0; c < APP_GUTTER; c++) v->attr[r][c] = OSUI_APP_A_DIM;
        for (c = ide->left; c <= OSUI_IDE_COLS && ide->lines[line][c] && k < plain_cols(ide); c++, k++) {
            d[k] = ide->lines[line][c];
            v->attr[r][k] = (has && !pos_less(line, c, r0, c0) && pos_less(line, c, r1, c1)) ? OSUI_APP_A_SEL : OSUI_APP_A_TEXT;
        }
        d[k] = 0;
    }
    v->nrows = APP_TEXT_ROWS;
    v->cur_row = ide->row - ide->top;
    v->cur_col = APP_GUTTER + ide->col - ide->left;
    if (ide->prompt) {
        const char *q = ide->prompt == OSUI_IDE_P_FIND ? "Chercher: " : ide->prompt == OSUI_IDE_P_SAVE_AS ? "Enregistrer sous: " : "Ouvrir: ";
        ide_cpy(v->status, OSUI_APP_COLS, q);
        ide_cat(v->status, OSUI_APP_COLS, ide->prompt_buf);
        ide_cat(v->status, OSUI_APP_COLS, "_  (Entree valide, Echap annule)");
    } else {
        ide_cpy(v->status, OSUI_APP_COLS, "Ln ");
        ide_cat_u(v->status, OSUI_APP_COLS, (unsigned)(ide->row + 1));
        ide_cat(v->status, OSUI_APP_COLS, " Col ");
        ide_cat_u(v->status, OSUI_APP_COLS, (unsigned)(ide->col + 1));
        ide_cat(v->status, OSUI_APP_COLS, ide->cmd_mode ? "  CMD  " : "  ");
        ide_cat(v->status, OSUI_APP_COLS, ide->status);
    }
}
