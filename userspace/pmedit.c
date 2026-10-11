/* userspace/pmedit.c - see pmedit.h */
#include "pmedit.h"

#define K_ESC 27
#define K_LEFT 28
#define K_RIGHT 29
#define K_UP 30
#define K_DOWN 31
#define A_TITLE 0x1F
#define A_TEXT 0x07
#define A_NUM 0x08
#define A_CUR 0x70
#define A_STATUS 0x30
#define A_HELP 0x0B

static int slen(const char* s) { int n = 0; while (s[n]) n++; return n; }
static void scpy(char* d, const char* s, int cap) { int i = 0; while (s[i] && i < cap - 1) { d[i] = s[i]; i++; } d[i] = 0; }

void pme_status(pme_t* e, const char* text) { scpy(e->status, text, PME_COLS + 1); }

void pme_load(pme_t* e, const char* file, const char* text, int len) {
    int i, k = 0;
    char* z = (char*)e;
    for (i = 0; i < (int)sizeof(*e); i++) z[i] = 0;
    scpy(e->file, file, (int)sizeof(e->file));
    e->n = 1;
    for (i = 0; i < len && text[i]; i++) {
        char c = text[i];
        if (c == '\r') continue;
        if (c == '\n') {
            if (e->n >= PME_LINES) break;
            e->n++; k = 0;
            continue;
        }
        if (c == '\t') c = ' ';
        if ((unsigned char)c < 32 || (unsigned char)c > 126) continue;
        if (k < PME_LEN) { e->line[e->n - 1][k++] = c; e->line[e->n - 1][k] = 0; }
    }
    /* a trailing newline does not make an extra empty line */
    if (e->n > 1 && len > 0 && text[len - 1] == '\n' && !e->line[e->n - 1][0]) e->n--;
    pme_status(e, "ESC then s save, c check, x save+quit, q quit");
}

int pme_text(const pme_t* e, char* out, int cap) {
    int i, k = 0, j;
    for (i = 0; i < e->n; i++) {
        for (j = 0; e->line[i][j]; j++) { if (k >= cap - 1) return -1; out[k++] = e->line[i][j]; }
        if (k >= cap - 1) return -1;
        out[k++] = '\n';
    }
    out[k] = 0;
    return k;
}

static void clamp(pme_t* e) {
    int l;
    if (e->cy < 0) e->cy = 0;
    if (e->cy >= e->n) e->cy = e->n - 1;
    l = slen(e->line[e->cy]);
    if (e->cx > l) e->cx = l;
    if (e->cx < 0) e->cx = 0;
    if (e->cy < e->top) e->top = e->cy;
    if (e->cy >= e->top + PME_VIEW) e->top = e->cy - PME_VIEW + 1;
}

int pme_key(pme_t* e, int key) {
    char* l = e->line[e->cy];
    int len = slen(l), i;
    if (e->esc) {
        e->esc = 0;
        if (key == 's') return PME_SAVE;
        if (key == 'c') return PME_CHECK;
        if (key == 'q') return PME_QUIT;
        if (key == 'x') { e->esc = 2; return PME_SAVE; } /* caller quits after a good save */
        pme_status(e, "unknown command (s c x q)");
        return PME_NONE;
    }
    if (key == K_ESC) { e->esc = 1; pme_status(e, "command: s save  c check  x save+quit  q quit"); return PME_NONE; }
    if (key == K_LEFT) { if (e->cx > 0) e->cx--; else if (e->cy > 0) { e->cy--; e->cx = PME_LEN; } }
    else if (key == K_RIGHT) { if (e->cx < len) e->cx++; else if (e->cy < e->n - 1) { e->cy++; e->cx = 0; } }
    else if (key == K_UP) e->cy--;
    else if (key == K_DOWN) e->cy++;
    else if (key == '\b') {
        if (e->cx > 0) {
            for (i = e->cx - 1; i < len; i++) l[i] = l[i + 1];
            e->cx--; e->modified = 1;
        } else if (e->cy > 0) {
            char* p = e->line[e->cy - 1];
            int pl = slen(p);
            if (pl + len > PME_LEN) { pme_status(e, "line too long to join"); return PME_NONE; }
            for (i = 0; i <= len; i++) p[pl + i] = l[i];
            for (i = e->cy; i < e->n - 1; i++) scpy(e->line[i], e->line[i + 1], PME_LEN + 1);
            e->line[e->n - 1][0] = 0;
            e->n--; e->cy--; e->cx = pl; e->modified = 1;
        }
    } else if (key == '\n' || key == '\r') {
        if (e->n >= PME_LINES) { pme_status(e, "buffer full (64 lines)"); return PME_NONE; }
        for (i = e->n; i > e->cy + 1; i--) scpy(e->line[i], e->line[i - 1], PME_LEN + 1);
        scpy(e->line[e->cy + 1], l + e->cx, PME_LEN + 1);
        l[e->cx] = 0;
        e->n++; e->cy++; e->cx = 0; e->modified = 1;
    } else if (key >= 32 && key <= 126) {
        if (len >= PME_LEN) { pme_status(e, "line full (76 columns)"); return PME_NONE; }
        for (i = len + 1; i > e->cx; i--) l[i] = l[i - 1];
        l[e->cx++] = (char)key; e->modified = 1;
    } else return PME_NONE;
    clamp(e);
    return PME_NONE;
}

static void put(uint16_t* c, int x, int y, char ch, uint8_t a) {
    if (x < 0 || y < 0 || x >= PME_COLS || y >= PME_ROWS) return;
    c[y * PME_COLS + x] = (uint16_t)((unsigned char)ch | ((uint16_t)a << 8));
}
static void text(uint16_t* c, int x, int y, const char* s, uint8_t a) { while (*s && x < PME_COLS) put(c, x++, y, *s++, a); }
static void fill(uint16_t* c, int y, uint8_t a) { int x; for (x = 0; x < PME_COLS; x++) put(c, x, y, ' ', a); }
static void num(char* o, int v) { o[0] = (char)(v >= 10 ? '0' + v / 10 : ' '); o[1] = (char)('0' + v % 10); o[2] = 0; }

void pme_render(const pme_t* e, uint16_t* cells) {
    int y, i;
    char t[96], n[3];
    for (y = 0; y < PME_ROWS; y++) fill(cells, y, A_TEXT);
    fill(cells, 0, A_TITLE);
    scpy(t, " PromptMessage IDE  ", (int)sizeof(t));
    text(cells, 0, 0, t, A_TITLE);
    text(cells, 20, 0, e->file, A_TITLE);
    if (e->modified) text(cells, 21 + slen(e->file), 0, "[modified]", A_TITLE);
    num(n, e->cy + 1 > 99 ? 99 : e->cy + 1);
    text(cells, 66, 0, "line ", A_TITLE); text(cells, 71, 0, n, A_TITLE);
    num(n, e->n > 99 ? 99 : e->n);
    text(cells, 73, 0, "/", A_TITLE); text(cells, 74, 0, n, A_TITLE);
    for (i = 0; i < PME_VIEW; i++) {
        int li = e->top + i;
        if (li >= e->n) { put(cells, 0, 2 + i, '~', A_NUM); continue; }
        num(n, (li + 1) % 100);
        text(cells, 0, 2 + i, n, A_NUM);
        text(cells, 3, 2 + i, e->line[li], A_TEXT);
        if (li == e->cy) {
            int x = 3 + e->cx;
            char ch = e->line[li][e->cx] ? e->line[li][e->cx] : ' ';
            put(cells, x, 2 + i, ch, A_CUR);
        }
    }
    fill(cells, 23, A_STATUS);
    text(cells, 0, 23, e->status, A_STATUS);
    text(cells, 0, 24, " arrows move  ENTER split  BACKSPACE join  ESC s/c/x/q", A_HELP);
}

void pme_row(const uint16_t* cells, int y, char* out, int cap) {
    int x, k = 0, last = -1;
    for (x = 0; x < PME_COLS && k < cap - 1; x++) {
        char ch = (char)(cells[y * PME_COLS + x] & 0xFF);
        if (ch < 32 || ch > 126) ch = ' ';
        out[k++] = ch;
        if (ch != ' ') last = k - 1;
    }
    out[last + 1] = 0;
}
