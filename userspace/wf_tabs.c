/* wf_tabs.c - Lot 86: see wf_tabs.h. */
#include "wf_tabs.h"

static int tlen(const char *s) { int n = 0; while (s && s[n]) n++; return n; }
static int teq(const char *a, const char *b) {
    int i = 0;
    if (!a || !b) return 0;
    while (a[i] && a[i] == b[i]) i++;
    return a[i] == b[i];
}
static void tcpy(char *d, int cap, const char *s) {
    int i = 0;
    if (cap <= 0) return;
    while (s && s[i] && i < cap - 1) { d[i] = s[i]; i++; }
    d[i] = 0;
}
static void tzero(void *p, uint32_t n) {
    uint8_t *b = (uint8_t *)p;
    while (n--) *b++ = 0;
}
static int bad(const char *url) {
    int i;
    if (!url || !url[0] || tlen(url) >= WF_NAV_URL) return 1;
    for (i = 0; url[i]; i++) if ((unsigned char)url[i] < 0x20u) return 1;
    if (url[0] == 'j' || url[0] == 'J') return 1;
    return 0;
}
static void title_of(char *d, const char *s) {
    int i = 0, o = 0, sp = 1;
    if (!s) { d[0] = 0; return; }
    while (s[i] && o < WF_NAV_TITLE - 1) {
        unsigned char c = (unsigned char)s[i++];
        if (c < 32) c = ' ';
        if (c == ' ') { if (sp) continue; sp = 1; } else sp = 0;
        d[o++] = (char)c;
    }
    while (o > 0 && d[o - 1] == ' ') o--;
    d[o] = 0;
}

void wf_tabs_init(wf_tabs_t *t) {
    if (!t) return;
    tzero(t, (uint32_t)sizeof(*t));
    t->current = -1;
}

int wf_tabs_find(const wf_tabs_t *t, const char *url) {
    int i;
    if (!t || !url) return -1;
    for (i = 0; i < WF_TABS_MAX; i++) if (t->tab[i].used && teq(t->tab[i].url, url)) return i;
    return -1;
}

int wf_tabs_open(wf_tabs_t *t, const char *url, const char *title, int *index) {
    int i, free_i = -1;
    if (index) *index = -1;
    if (!t || bad(url)) return -1;
    i = wf_tabs_find(t, url);
    if (i >= 0) {
        if (title && title[0]) title_of(t->tab[i].title, title);
        t->current = i;
        t->focused++;
        if (index) *index = i;
        return 0;
    }
    for (i = 0; i < WF_TABS_MAX; i++) if (!t->tab[i].used) { free_i = i; break; }
    if (free_i < 0) return -1;
    tzero(&t->tab[free_i], (uint32_t)sizeof(t->tab[free_i]));
    t->tab[free_i].used = 1;
    tcpy(t->tab[free_i].url, WF_NAV_URL, url);
    title_of(t->tab[free_i].title, title);
    t->n++;
    t->current = free_i;
    t->opened++;
    if (index) *index = free_i;
    return 0;
}

int wf_tabs_close(wf_tabs_t *t, int index) {
    int i, next = -1;
    if (!t || index < 0 || index >= WF_TABS_MAX || !t->tab[index].used) return -1;
    t->tab[index].used = 0;
    t->n--;
    t->closed++;
    if (t->current == index) {
        for (i = index + 1; i < WF_TABS_MAX; i++) if (t->tab[i].used) { next = i; break; }
        if (next < 0) for (i = index - 1; i >= 0; i--) if (t->tab[i].used) { next = i; break; }
        t->current = next;
    }
    return 0;
}

int wf_tabs_select(wf_tabs_t *t, int index) {
    if (!t || index < 0 || index >= WF_TABS_MAX || !t->tab[index].used) return -1;
    t->current = index;
    t->focused++;
    return 0;
}

int wf_tabs_next(wf_tabs_t *t, int dir) {
    int i, step, start;
    if (!t || t->n <= 0 || t->current < 0) return -1;
    step = dir < 0 ? -1 : 1;
    start = t->current;
    for (i = 1; i <= WF_TABS_MAX; i++) {
        int k = (start + step * i) % WF_TABS_MAX;
        if (k < 0) k += WF_TABS_MAX;
        if (t->tab[k].used) { t->current = k; t->focused++; return k; }
    }
    return -1;
}

int wf_tabs_set_title(wf_tabs_t *t, int index, const char *title) {
    if (!t || index < 0 || index >= WF_TABS_MAX || !t->tab[index].used) return -1;
    title_of(t->tab[index].title, title);
    return 0;
}

int wf_tabs_set_scroll(wf_tabs_t *t, int index, int row) {
    if (!t || index < 0 || index >= WF_TABS_MAX || !t->tab[index].used) return -1;
    if (row < 0) row = 0;
    if (row > 100000) row = 100000;
    t->tab[index].scroll = row;
    return 0;
}

const wf_tab_t *wf_tabs_at(const wf_tabs_t *t, int index) {
    if (!t || index < 0 || index >= WF_TABS_MAX || !t->tab[index].used) return 0;
    return &t->tab[index];
}

static int app(char *o, int cap, int p, const char *s) {
    while (s && *s) { if (p >= cap - 1) return -1; o[p++] = *s++; }
    return p;
}
static int app_u(char *o, int cap, int p, int v) {
    char b[12];
    int n = 0;
    if (v < 0) v = 0;
    do { b[n++] = (char)('0' + v % 10); v /= 10; } while (v && n < 11);
    while (n) { if (p >= cap - 1) return -1; o[p++] = b[--n]; }
    return p;
}

int wf_tabs_save(const wf_tabs_t *t, char *out, int cap) {
    int p = 0, i;
    if (!t || !out || cap < 8) return -1;
    p = app(out, cap, p, "T ");
    p = app_u(out, cap, p, t->current);
    if (p < 0 || p >= cap - 1) return -1;
    out[p++] = '\n';
    for (i = 0; i < WF_TABS_MAX; i++) if (t->tab[i].used) {
        if (p >= cap - 1) return -1;
        out[p++] = 'U';
        out[p++] = ' ';
        p = app_u(out, cap, p, i);
        if (p < 0 || p >= cap - 1) return -1;
        out[p++] = '\t';
        p = app_u(out, cap, p, t->tab[i].scroll);
        if (p < 0 || p >= cap - 1) return -1;
        out[p++] = '\t';
        p = app(out, cap, p, t->tab[i].url);
        if (p < 0 || p >= cap - 1) return -1;
        out[p++] = '\t';
        p = app(out, cap, p, t->tab[i].title);
        if (p < 0 || p >= cap - 1) return -1;
        out[p++] = '\n';
    }
    out[p] = 0;
    return p;
}

int wf_tabs_load(wf_tabs_t *t, const char *text, int len) {
    wf_tabs_t tmp;
    int i = 0;
    if (!t || !text || len < 0) return -1;
    wf_tabs_init(&tmp);
    while (i < len) {
        char line[WF_NAV_URL + WF_NAV_TITLE + 32];
        int k = 0;
        while (i < len && text[i] != '\n' && k < (int)sizeof(line) - 1) line[k++] = text[i++];
        if (i < len && text[i] == '\n') i++;
        line[k] = 0;
        if (k == 0) continue;
        if (line[0] == 'T' && line[1] == ' ') {
            int v = 0, p = 2, neg = 0;
            if (line[p] == '-') { neg = 1; p++; }
            while (line[p] >= '0' && line[p] <= '9') { v = v * 10 + line[p] - '0'; p++; }
            tmp.current = neg ? -1 : v;
        } else if (line[0] == 'U' && line[1] == ' ') {
            int idx = 0, scroll = 0, p = 2;
            char *u, *title;
            while (line[p] >= '0' && line[p] <= '9') { idx = idx * 10 + line[p] - '0'; p++; }
            if (line[p] != '\t' || idx < 0 || idx >= WF_TABS_MAX) return -1;
            p++;
            while (line[p] >= '0' && line[p] <= '9') { scroll = scroll * 10 + line[p] - '0'; p++; }
            if (line[p] != '\t') return -1;
            p++;
            u = line + p;
            while (line[p] && line[p] != '\t') p++;
            if (line[p] != '\t') return -1;
            line[p] = 0;
            title = line + p + 1;
            if (bad(u)) return -1;
            tmp.tab[idx].used = 1;
            tmp.tab[idx].scroll = scroll;
            tcpy(tmp.tab[idx].url, WF_NAV_URL, u);
            title_of(tmp.tab[idx].title, title);
            tmp.n++;
        }
    }
    if (tmp.current < 0 || tmp.current >= WF_TABS_MAX || !tmp.tab[tmp.current].used) {
        tmp.current = -1;
        for (i = 0; i < WF_TABS_MAX; i++) if (tmp.tab[i].used) { tmp.current = i; break; }
    }
    {
        uint8_t *d = (uint8_t *)t;
        const uint8_t *s = (const uint8_t *)&tmp;
        uint32_t n = (uint32_t)sizeof(tmp);
        while (n--) *d++ = *s++;
    }
    return 0;
}

int wf_tabs_text(const wf_tabs_t *t, char *out, int cap) {
    int p = 0, i;
    if (!t || !out || cap < 8) return -1;
    p = app(out, cap, p, "tabs ");
    p = app_u(out, cap, p, t->n);
    p = app(out, cap, p, " current ");
    p = app_u(out, cap, p, t->current < 0 ? 0 : t->current);
    if (p < 0) return -1;
    if (t->current < 0) { if (p < cap - 1) out[p++] = '\n'; out[p] = 0; return p; }
    for (i = 0; i < WF_TABS_MAX; i++) if (t->tab[i].used) {
        if (p >= cap - 1) return -1;
        out[p++] = '\n';
        if (i == t->current && p < cap - 1) out[p++] = '*';
        p = app_u(out, cap, p, i);
        if (p < 0 || p >= cap - 1) return -1;
        out[p++] = ' ';
        p = app(out, cap, p, t->tab[i].title[0] ? t->tab[i].title : t->tab[i].url);
        if (p < 0) return -1;
    }
    if (p < cap - 1) out[p++] = '\n';
    out[p] = 0;
    return p;
}
