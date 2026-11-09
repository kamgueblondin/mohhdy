/* wf_article.c - Lot 85. See wf_article.h. */
#include "wf_article.h"

static int spc(char c) { return c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == '\f'; }
static char lc(char c) { return (c >= 'A' && c <= 'Z') ? (char)(c + 32) : c; }

static int eq(const char *a, const char *b) {
    while (*a && *a == *b) { a++; b++; }
    return *a == *b;
}

static int tag_name(const char *in, int n, int i, char *name, int cap, int *close, int *end) {
    int k = 0, q = 0;
    if (i >= n || in[i] != '<') return 0;
    i++;
    if (i < n && (in[i] == '!' || in[i] == '?')) return 0;
    *close = (i < n && in[i] == '/');
    if (*close) i++;
    while (i < n && k < cap - 1) {
        char c = lc(in[i]);
        if (!((c >= 'a' && c <= 'z') || (c >= '0' && c <= '9'))) break;
        name[k++] = c;
        i++;
    }
    name[k] = 0;
    if (!k) return 0;
    while (i < n && (q || in[i] != '>')) {
        if (in[i] == '"' || in[i] == '\'') q = (q == in[i]) ? 0 : (q ? q : in[i]);
        i++;
    }
    *end = i < n ? i + 1 : n;
    return 1;
}

static int heading(const char *name) {
    if (name[0] == 'h' && name[1] >= '1' && name[1] <= '6' && name[2] == 0) return name[1] - '0';
    return 0;
}

static int block_break(const char *name) {
    return heading(name) || eq(name, "p") || eq(name, "div") || eq(name, "li") ||
           eq(name, "tr") || eq(name, "pre") || eq(name, "blockquote") ||
           eq(name, "h1") || eq(name, "br") || eq(name, "table") ||
           eq(name, "ul") || eq(name, "ol") || eq(name, "section") ||
           eq(name, "article") || eq(name, "main");
}

typedef struct {
    char *o;
    int cap, n;
    int skip;       /* inside script or style */
    int pre;        /* inside pre */
    int cell;       /* columns written on the current row */
    int list;       /* list depth */
    int at_break;   /* the output already ends at a paragraph break */
    int any;
} art_t;

static void putc_art(art_t *a, char c) {
    if (a->n < a->cap - 1) a->o[a->n++] = c;
}

static void puts_art(art_t *a, const char *s) {
    while (*s) putc_art(a, *s++);
}

static void break_par(art_t *a) {
    if (!a->any) { a->at_break = 1; return; }
    if (a->at_break) return;
    putc_art(a, '\n');
    putc_art(a, '\n');
    a->at_break = 1;
    a->cell = 0;
}

static void put_text_char(art_t *a, char c) {
    if (a->pre) {
        if (c == '\r') return;
        putc_art(a, c);
        a->at_break = (c == '\n');
        a->any = 1;
        return;
    }
    if (spc(c)) {
        if (!a->any || a->at_break) return;
        if (a->n > 0 && a->o[a->n - 1] == ' ') return;
        putc_art(a, ' ');
        return;
    }
    putc_art(a, c);
    a->at_break = 0;
    a->any = 1;
}

int wf_article_text(const char *html, int n, char *out, int cap) {
    art_t a;
    int i = 0;
    a.o = out; a.cap = cap; a.n = 0; a.skip = 0; a.pre = 0; a.cell = 0; a.list = 0; a.at_break = 1; a.any = 0;
    if (!out || cap < 2) return 0;
    out[0] = 0;
    if (!html || n <= 0) return 0;
    while (i < n) {
        char name[16];
        int close = 0, end = 0, level;
        if (html[i] != '<') {
            if (!a.skip) put_text_char(&a, html[i]);
            i++;
            continue;
        }
        if (i + 3 < n && html[i + 1] == '!' && html[i + 2] == '-' && html[i + 3] == '-') {
            i += 4;
            while (i + 2 < n && !(html[i] == '-' && html[i + 1] == '-' && html[i + 2] == '>')) i++;
            i += 3;
            continue;
        }
        if (!tag_name(html, n, i, name, (int)sizeof(name), &close, &end)) { i++; continue; }
        if (eq(name, "script") || eq(name, "style") || eq(name, "noscript")) {
            if (!close) a.skip++;
            else if (a.skip) a.skip--;
            i = end;
            continue;
        }
        if (a.skip) { i = end; continue; }
        level = heading(name);
        if (!close && eq(name, "pre")) { break_par(&a); a.pre = 1; }
        else if (close && eq(name, "pre")) { a.pre = 0; break_par(&a); }
        else if (!close && (eq(name, "ul") || eq(name, "ol"))) { if (a.list < 6) a.list++; break_par(&a); }
        else if (close && (eq(name, "ul") || eq(name, "ol"))) { if (a.list) a.list--; break_par(&a); }
        else if (!close && eq(name, "li")) {
            int k;
            break_par(&a);
            for (k = 1; k < a.list; k++) puts_art(&a, "  ");
            puts_art(&a, "- ");
            a.at_break = 0;
            a.any = 1;
        } else if (!close && eq(name, "tr")) {
            break_par(&a);
            a.cell = 0;
        } else if (!close && (eq(name, "td") || eq(name, "th"))) {
            if (a.cell) puts_art(&a, " | ");
            a.cell++;
            a.at_break = 0;
            a.any = 1;
        } else if (!close && level) {
            int k;
            break_par(&a);
            for (k = 0; k < level; k++) putc_art(&a, '#');
            putc_art(&a, ' ');
            a.at_break = 0;
            a.any = 1;
        } else if (!close && eq(name, "blockquote")) {
            break_par(&a);
            puts_art(&a, "> ");
            a.at_break = 0;
            a.any = 1;
        } else if (!close && eq(name, "br")) {
            putc_art(&a, '\n');
            a.at_break = 1;
        } else if (!close && block_break(name)) {
            break_par(&a);
        } else if (close && (level || eq(name, "p") || eq(name, "li") || eq(name, "tr") || eq(name, "blockquote"))) {
            break_par(&a);
        }
        i = end;
    }
    while (a.n > 0 && (a.o[a.n - 1] == '\n' || a.o[a.n - 1] == ' ')) a.n--;
    if (a.n < a.cap) a.o[a.n] = 0;
    else if (a.cap > 0) a.o[a.cap - 1] = 0;
    return a.n;
}
