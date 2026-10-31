/* wf_section.c - Lot 71: sections of a long reduced page (see wf_section.h). */
#include "wf_section.h"

static char s_lc(char c) { return (c >= 'A' && c <= 'Z') ? (char)(c + 32) : c; }

static int s_find(const char* s, int n, int from, const char* p) {
    int i, k;
    for (i = from; i < n; i++) {
        for (k = 0; p[k] && i + k < n && s_lc(s[i + k]) == p[k]; k++) {}
        if (!p[k]) return i;
    }
    return -1;
}

static int s_eq(const char* a, const char* b) { while (*a && *a == *b) { a++; b++; } return *a == *b; }

/* tag at src[i] == '<': name (lowercase), closing flag, end ('>' + 1) */
static int s_tag(const char* src, int n, int i, char* name, int cap, int* close) {
    int j = i + 1, k = 0;
    *close = 0;
    if (j < n && src[j] == '/') { *close = 1; j++; }
    while (j < n && k < cap - 1 && ((src[j] >= 'a' && src[j] <= 'z') || (src[j] >= 'A' && src[j] <= 'Z') || (src[j] >= '0' && src[j] <= '9')))
        name[k++] = s_lc(src[j++]);
    name[k] = 0;
    while (j < n && src[j] != '>') j++;
    return j < n ? j + 1 : n;
}

static int is_void(const char* t) { return s_eq(t, "br") || s_eq(t, "hr") || s_eq(t, "img"); }
static int is_block(const char* t) {
    return s_eq(t, "p") || s_eq(t, "li") || s_eq(t, "ul") || s_eq(t, "ol") || s_eq(t, "pre") || s_eq(t, "hr") ||
           s_eq(t, "blockquote") || (t[0] == 'h' && t[1] >= '1' && t[1] <= '6' && !t[2]);
}
/* elements a clean cut may stay inside */
static int is_container(const char* t) { return s_eq(t, "ul") || s_eq(t, "ol") || s_eq(t, "blockquote"); }

typedef struct { char t[WF_SECTION_DEPTH][WF_SECTION_TAG]; int d; } stk_t;

static void stk_push(stk_t* st, const char* t) {
    int k;
    if (st->d >= WF_SECTION_DEPTH) { st->d++; return; }   /* deeper levels are counted, not stored */
    for (k = 0; t[k] && k < WF_SECTION_TAG - 1; k++) st->t[st->d][k] = t[k];
    st->t[st->d][k] = 0;
    st->d++;
}
static void stk_pop(stk_t* st, const char* t) {
    int k;
    if (st->d > WF_SECTION_DEPTH) { st->d--; return; }
    for (k = st->d - 1; k >= 0; k--)
        if (s_eq(st->t[k], t)) { st->d = k; return; }
}
static int stk_clean(const stk_t* st) {
    int k;
    if (st->d > WF_SECTION_DEPTH) return 0;
    for (k = 0; k < st->d; k++) if (!is_container(st->t[k])) return 0;
    return 1;
}
static int stk_has(const stk_t* st, const char* t) {
    int k;
    for (k = 0; k < st->d && k < WF_SECTION_DEPTH; k++) if (s_eq(st->t[k], t)) return 1;
    return 0;
}

static void mark(wf_sections_t* s, int pos, const stk_t* st) {
    int k, d = st->d > WF_SECTION_DEPTH ? WF_SECTION_DEPTH : st->d, c;
    s->off[s->n] = pos;
    s->depth[s->n] = (uint8_t)d;
    for (k = 0; k < d; k++)
        for (c = 0; c < WF_SECTION_TAG; c++) s->open[s->n][k][c] = st->t[k][c];
    s->n++;
}

int wf_section_index(const char* src, int n, wf_sections_t* s) {
    stk_t st;
    int i, nodes = 0, start, b, t;
    for (i = 0; i < (int)sizeof(*s); i++) ((char*)s)[i] = 0;
    st.d = 0;
    b = s_find(src, n, 0, "<body>");
    if (b < 0) return -1;
    t = s_find(src, b, 0, "<title>");
    if (t >= 0) {
        s->title_start = t + 7;
        s->title_end = s_find(src, b, t, "</title>");
        if (s->title_end < 0) s->title_end = s->title_start;
    }
    s->body_start = b + 6;
    s->body_end = s_find(src, n, s->body_start, "</body>");
    if (s->body_end < 0) s->body_end = n;
    start = s->body_start;
    mark(s, start, &st);
    i = start;
    while (i < s->body_end) {
        if (src[i] == '<') {
            char name[WF_SECTION_TAG];
            int close, e = s_tag(src, s->body_end, i, name, (int)sizeof(name), &close);
            int bytes = i - s->off[s->n - 1];
            if (!close && name[0]) {
                int clean = is_block(name) && stk_clean(&st), force = bytes >= WF_SECTION_BYTES && !stk_has(&st, "a");
                if ((nodes >= WF_SECTION_NODES && clean) || force || nodes >= WF_SECTION_NODES + 30) {
                    if (s->n >= WF_SECTION_MAX) { s->cut = 1; break; }
                    mark(s, i, &st);
                    nodes = 0;
                }
                nodes++;
                s->nodes++;
                if (!is_void(name)) stk_push(&st, name);
            } else if (close && name[0]) {
                stk_pop(&st, name);
            }
            i = e;
            continue;
        }
        {
            int j = i, any = 0;
            while (j < s->body_end && src[j] != '<') {
                if (src[j] != ' ' && src[j] != '\n') any = 1;
                j++;
                /* a very long text run is cut at a space */
                if (j - s->off[s->n - 1] >= WF_SECTION_BYTES + 2000 && src[j] == ' ' && !stk_has(&st, "a")) {
                    if (s->n >= WF_SECTION_MAX) { s->cut = 1; break; }
                    if (any) { nodes++; s->nodes++; }
                    mark(s, j, &st);
                    nodes = 0;
                    any = 0;
                }
            }
            if (s->cut) break;
            if (any) { nodes++; s->nodes++; }
            i = j;
        }
    }
    s->off[s->n] = s->cut ? i : s->body_end;
    return s->n;
}

int wf_section_of(const wf_sections_t* s, int pos) {
    int k;
    for (k = s->n - 1; k > 0; k--) if (pos >= s->off[k]) return k;
    return 0;
}

typedef struct { char* o; int cap; int n; int full; } sb_t;
static void sb_c(sb_t* b, char c) { if (b->n < b->cap - 1) b->o[b->n++] = c; else b->full = 1; }
static void sb_s(sb_t* b, const char* s) { while (*s) sb_c(b, *s++); }
static void sb_n(sb_t* b, int v) {
    char t[12];
    int k = 0;
    if (v < 0) { sb_c(b, '-'); v = -v; }
    do { t[k++] = (char)('0' + v % 10); v /= 10; } while (v && k < 11);
    while (k) sb_c(b, t[--k]);
}

/* the stack at off[k + 1], recomputed from section k's open list and its content */
static void end_stack(const char* src, const wf_sections_t* s, int k, stk_t* st) {
    int i, c;
    st->d = s->depth[k];
    for (i = 0; i < st->d; i++) for (c = 0; c < WF_SECTION_TAG; c++) st->t[i][c] = s->open[k][i][c];
    i = s->off[k];
    while (i < s->off[k + 1]) {
        if (src[i] == '<') {
            char name[WF_SECTION_TAG];
            int close, e = s_tag(src, s->off[k + 1], i, name, (int)sizeof(name), &close);
            if (name[0]) { if (close) stk_pop(st, name); else if (!is_void(name)) stk_push(st, name); }
            i = e;
        } else i++;
    }
}

int wf_section_render(const char* src, int n, const wf_sections_t* s, int k, const wf_form_page_t* forms,
                      char* out, int cap) {
    sb_t b;
    stk_t st;
    int i, d;
    static char fd[WF_FORM_VALUE + 16];
    (void)n;
    if (!s || k < 0 || k >= s->n || cap < 512) return -1;
    b.o = out; b.cap = cap - 260; b.n = 0; b.full = 0;   /* room kept for the closing part */
    sb_s(&b, "<html><head><title>");
    for (i = s->title_start; i < s->title_end; i++) sb_c(&b, src[i]);
    sb_s(&b, "</title></head><body>");
    if (s->n > 1 && k > 0) {
        sb_s(&b, "<p>[section "); sb_n(&b, k + 1); sb_c(&b, '/'); sb_n(&b, s->n);
        sb_s(&b, " - fleche haut en haut de page: section precedente]</p>");
    }
    for (d = 0; d < s->depth[k]; d++) { sb_c(&b, '<'); sb_s(&b, s->open[k][d]); sb_c(&b, '>'); }
    for (i = s->off[k]; i < s->off[k + 1] && !b.full; i++) {
        char c = src[i];
        if (c == '\001') {
            int v = 0, j = i + 1;
            while (j < s->off[k + 1] && src[j] >= '0' && src[j] <= '9') v = v * 10 + (src[j++] - '0');
            if (j < s->off[k + 1] && src[j] == '\002') {
                int q;
                if (forms) wf_form_display(forms, v, fd, (int)sizeof(fd));
                else { fd[0] = '['; fd[1] = ' '; fd[2] = ']'; fd[3] = 0; }
                for (q = 0; fd[q]; q++) {
                    if (fd[q] == '<') sb_s(&b, "&lt;");
                    else if (fd[q] == '&') sb_s(&b, "&amp;");
                    else if (fd[q] == '>') sb_s(&b, "&gt;");
                    else sb_c(&b, fd[q]);
                }
                i = j;
                continue;
            }
        }
        sb_c(&b, c);
    }
    /* a section cut by the byte budget may end inside a tag */
    {
        int q = b.n - 1;
        while (q > 0 && b.o[q] != '<' && b.o[q] != '>') q--;
        if (q >= 0 && b.o[q] == '<') b.n = q;
    }
    b.cap = cap;
    b.full = 0;
    end_stack(src, s, k, &st);
    for (d = (st.d > WF_SECTION_DEPTH ? WF_SECTION_DEPTH : st.d) - 1; d >= 0; d--) { sb_s(&b, "</"); sb_s(&b, st.t[d]); sb_c(&b, '>'); }
    if (s->n > 1) {
        sb_s(&b, "<p>[section "); sb_n(&b, k + 1); sb_c(&b, '/'); sb_n(&b, s->n);
        sb_s(&b, k + 1 < s->n ? " - fleche bas en fin de page: suite]</p>" : " - fin de la page]</p>");
    }
    sb_s(&b, "</body></html>");
    b.o[b.n] = 0;
    return b.full ? -1 : b.n;
}
