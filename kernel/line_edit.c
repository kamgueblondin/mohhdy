/* line_edit.c - see line_edit.h. */
#include "line_edit.h"

static char g_hist[LE_HIST][LE_MAX];
static int g_hist_n;      /* total entries ever added (ring) */

void line_edit_history_reset(void) { g_hist_n = 0; }
int line_edit_history_count(void) { return g_hist_n < LE_HIST ? g_hist_n : LE_HIST; }

static int le_strlen(const char *s) { int n = 0; while (s[n]) n++; return n; }
static void le_strcpy(char *d, const char *s, int max) {
    int i = 0;
    while (s[i] && i < max - 1) { d[i] = s[i]; i++; }
    d[i] = 0;
}
static int le_streq(const char *a, const char *b) {
    while (*a && *a == *b) { a++; b++; }
    return *a == *b;
}

static const char *hist_get(int back) {   /* back=0: newest */
    int n = line_edit_history_count();
    if (back < 0 || back >= n) return 0;
    return g_hist[(g_hist_n - 1 - back) % LE_HIST];
}

static void hist_add(const char *s) {
    const char *last = hist_get(0);
    if (!s[0]) return;
    if (last && le_streq(last, s)) return;
    le_strcpy(g_hist[g_hist_n % LE_HIST], s, LE_MAX);
    g_hist_n++;
}

void line_edit_begin(line_edit_t *le, int size, const char *const *words, int nwords,
                     const line_edit_io_t *io) {
    le->len = le->cur = 0;
    le->size = size > LE_MAX ? LE_MAX : size;
    le->buf[0] = 0;
    le->hist_pos = -1;
    le->saved[0] = 0;
    le->last_tab = 0;
    le->words = words;
    le->nwords = nwords;
    le->io = io;
}

static void mv_left(line_edit_t *le, int n) { while (n-- > 0) le->io->left(); }

/* Redraw from position p to the end, erasing `extra` stale cells, and put
 * the cursor back at le->cur. The screen cursor is at `from`. */
static void redraw(line_edit_t *le, int from, int p, int extra) {
    int i;
    mv_left(le, from - p);
    for (i = p; i < le->len; i++) le->io->put(le->buf[i]);
    for (i = 0; i < extra; i++) le->io->put(' ');
    mv_left(le, le->len + extra - le->cur);
}

static void replace_all(line_edit_t *le, const char *s) {
    int old = le->len, from = le->cur;
    le_strcpy(le->buf, s, le->size);
    le->len = le_strlen(le->buf);
    le->cur = le->len;
    redraw(le, from, 0, old > le->len ? old - le->len : 0);
}

void line_edit_set(line_edit_t *le, const char *s) { replace_all(le, s); }

static void insert(line_edit_t *le, const char *s, int n) {
    int i, from = le->cur;
    if (le->len + n > le->size - 1) n = le->size - 1 - le->len;
    if (n <= 0) return;
    for (i = le->len; i >= le->cur; i--) le->buf[i + n] = le->buf[i];
    for (i = 0; i < n; i++) le->buf[le->cur + i] = s[i];
    le->len += n;
    le->buf[le->len] = 0;
    le->cur += n;
    /* cursor is at `from`, redraw from there */
    {
        int k;
        for (k = from; k < le->len; k++) le->io->put(le->buf[k]);
        mv_left(le, le->len - le->cur);
    }
}

static void delete_at(line_edit_t *le, int p) {   /* remove buf[p] */
    int i, from = le->cur;
    if (p < 0 || p >= le->len) return;
    for (i = p; i < le->len; i++) le->buf[i] = le->buf[i + 1];
    le->len--;
    if (le->cur > p) le->cur--;
    redraw(le, from, p, 1);
}

static void complete(line_edit_t *le) {
    int i, start = 0, plen, n = 0, first = -1, common = 0;
    for (i = 0; i < le->cur; i++) if (le->buf[i] == ' ') return;   /* first word only */
    plen = le->cur;
    for (i = 0; i < le->nwords; i++) {
        const char *w = le->words[i];
        int k = 0;
        while (k < plen && w[k] && w[k] == le->buf[start + k]) k++;
        if (k != plen) continue;
        if (first < 0) { first = i; common = le_strlen(w); }
        else {
            const char *f = le->words[first];
            int c = 0;
            while (c < common && f[c] == w[c]) c++;
            common = c;
        }
        n++;
    }
    if (n == 0) { le->last_tab = 0; return; }
    if (common > plen) {
        insert(le, le->words[first] + plen, common - plen);
        if (n == 1 && le->cur == le->len) insert(le, " ", 1);
        le->last_tab = 0;
        return;
    }
    if (n == 1) { if (le->cur == le->len) insert(le, " ", 1); le->last_tab = 0; return; }
    if (!le->last_tab) { le->last_tab = 1; return; }
    /* double Tab: list the candidates, then the prompt and the line */
    {
        int col = 0;
        const char *s;
        le->io->put('\n');
        for (i = 0; i < le->nwords; i++) {
            const char *w = le->words[i];
            int k = 0, wl;
            while (k < plen && w[k] && w[k] == le->buf[k]) k++;
            if (k != plen) continue;
            wl = le_strlen(w);
            if (col && col + wl + 2 > 78) { le->io->put('\n'); col = 0; }
            for (s = w; *s; s++) le->io->put(*s);
            le->io->put(' '); le->io->put(' ');
            col += wl + 2;
        }
        le->io->put('\n');
        if (le->io->reprompt) le->io->reprompt();
        for (i = 0; i < le->len; i++) le->io->put(le->buf[i]);
        mv_left(le, le->len - le->cur);
    }
    le->last_tab = 0;
}

int line_edit_key(line_edit_t *le, char c) {
    unsigned char uc = (unsigned char)c;
    if (c != '\t') le->last_tab = 0;
    if (c == '\r' || c == '\n') {
        int i;
        for (i = le->cur; i < le->len; i++) le->io->put(le->buf[i]);
        le->buf[le->len] = 0;
        hist_add(le->buf);
        return 1;
    }
    switch (uc) {
    case '\b':
        if (le->cur > 0) delete_at(le, le->cur - 1);
        return 0;
    case LE_KEY_DEL:
        delete_at(le, le->cur);
        return 0;
    case LE_KEY_LEFT:
        if (le->cur > 0) { le->cur--; le->io->left(); }
        return 0;
    case LE_KEY_RIGHT:
        if (le->cur < le->len) { le->io->put(le->buf[le->cur]); le->cur++; }
        return 0;
    case LE_KEY_HOME:
        mv_left(le, le->cur); le->cur = 0;
        return 0;
    case LE_KEY_END:
        while (le->cur < le->len) le->io->put(le->buf[le->cur++]);
        return 0;
    case LE_KEY_UP: {
        const char *h = hist_get(le->hist_pos + 1);
        if (!h) return 0;
        if (le->hist_pos < 0) le_strcpy(le->saved, le->buf, LE_MAX);
        le->hist_pos++;
        replace_all(le, h);
        return 0;
    }
    case LE_KEY_DOWN:
        if (le->hist_pos < 0) return 0;
        le->hist_pos--;
        replace_all(le, le->hist_pos < 0 ? le->saved : hist_get(le->hist_pos));
        return 0;
    case '\t':
        complete(le);
        return 0;
    default:
        if (uc >= 32 && uc <= 126) insert(le, &c, 1);
        return 0;
    }
}
