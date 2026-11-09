/* wf_feed.c - Lot 86: see wf_feed.h. */
#include "wf_feed.h"

static char flow(char c) { return (c >= 'A' && c <= 'Z') ? (char)(c + 32) : c; }
static int flen(const char *s) { int n = 0; while (s && s[n]) n++; return n; }

static int starts(const char *s, int n, int i, const char *w) {
    int k = 0;
    while (w[k]) {
        if (i + k >= n || flow(s[i + k]) != w[k]) return 0;
        k++;
    }
    return 1;
}

static int find_tag(const char *s, int n, int from, const char *name, int *end) {
    int i, nl = flen(name);
    for (i = from; i + nl + 1 < n; i++) {
        if (s[i] != '<') continue;
        if (i + 1 < n && (s[i + 1] == '!' || s[i + 1] == '?')) {
            while (i < n && s[i] != '>') i++;
            continue;
        }
        if (i + 1 < n && s[i + 1] == '/') continue;
        if (!starts(s, n, i + 1, name)) continue;
        {
            int c = s[i + 1 + nl];
            if (c && c != '>' && c != ' ' && c != '\t' && c != '\n' && c != '/' && c != '\r') continue;
        }
        while (i < n && s[i] != '>') i++;
        if (end) *end = i < n ? i + 1 : n;
        return i;
    }
    return -1;
}

static int skip_el(const char *s, int n, int i, const char *name) {
    int depth = 1, nl = flen(name);
    while (i < n && depth) {
        if (s[i] == '<' && i + 1 < n && s[i + 1] != '!' && s[i + 1] != '?') {
            int close = s[i + 1] == '/';
            int j = i + 1 + close;
            if (starts(s, n, j, name)) {
                int c = (j + nl < n) ? s[j + nl] : 0;
                if (c == '>' || c == ' ' || c == '\t' || c == '\n' || c == '/' || c == '\r' || c == 0) {
                    if (close) depth--; else depth++;
                }
            }
        }
        i++;
    }
    return i;
}

static int hexv(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
}

static void plain(const char *s, int from, int to, char *out, int cap) {
    int i = from, o = 0, sp = 1;
    if (cap <= 0) return;
    while (i < to && o < cap - 1) {
        unsigned char c = (unsigned char)s[i];
        if (c == '<') {
            if (starts(s, to, i + 1, "br")) {
                if (!sp && o < cap - 1) { out[o++] = ' '; sp = 1; }
            }
            while (i < to && s[i] != '>') i++;
            if (i < to) i++;
            continue;
        }
        if (c == '&') {
            int k = i + 1;
            if (starts(s, to, k, "amp")) { c = '&'; i = k + 3; }
            else if (starts(s, to, k, "lt")) { c = '<'; i = k + 2; }
            else if (starts(s, to, k, "gt")) { c = '>'; i = k + 2; }
            else if (starts(s, to, k, "quot")) { c = '"'; i = k + 4; }
            else if (starts(s, to, k, "apos")) { c = '\''; i = k + 4; }
            else if (starts(s, to, k, "nbsp")) { c = ' '; i = k + 4; }
            else if (k < to && s[k] == '#') {
                int hex = k + 1 < to && (s[k + 1] == 'x' || s[k + 1] == 'X');
                int p = k + 1 + hex, code = 0;
                while (p < to && s[p] != ';') {
                    int h = hexv(s[p]);
                    if (h < 0) break;
                    if (!hex && (s[p] < '0' || s[p] > '9')) break;
                    code = code * (hex ? 16 : 10) + (hex ? h : s[p] - '0');
                    p++;
                }
                c = (code >= 32 && code < 127) ? (unsigned char)code : ' ';
                i = (p < to && s[p] == ';') ? p + 1 : i + 1;
            } else { i++; c = '&'; }
            if (i < to && s[i] == ';') i++;
        } else i++;
        if (c < 32 || c == 127) c = ' ';
        if (c == ' ') { if (sp) continue; sp = 1; }
        else sp = 0;
        out[o++] = (char)c;
    }
    while (o > 0 && out[o - 1] == ' ') o--;
    out[o] = 0;
}

static int attr(const char *s, int from, int to, const char *key, char *out, int cap) {
    int i, kl = flen(key);
    for (i = from; i + kl + 2 < to; i++) {
        if (!starts(s, to, i, key)) continue;
        if (s[i + kl] != '=') continue;
        {
            int q = s[i + kl + 1];
            int p = i + kl + 1;
            if (q == '"' || q == '\'') {
                p++;
                plain(s, p, to, out, cap);
                /* plain stops at '<' ; trim at the quote by rewriting */
                {
                    int k = 0, o = 0, sp = 1;
                    while (p < to && s[p] != q && o < cap - 1) {
                        char c = s[p++];
                        if (c == '&') continue;
                        if (c < 32) c = ' ';
                        if (c == ' ') { if (sp) continue; sp = 1; } else sp = 0;
                        out[o++] = c;
                        k++;
                    }
                    while (o > 0 && out[o - 1] == ' ') o--;
                    out[o] = 0;
                    return o > 0;
                }
            }
        }
    }
    if (cap > 0) out[0] = 0;
    return 0;
}

static void take_child(const char *s, int from, int to, const char *tag, char *out, int cap) {
    int end = 0, at = find_tag(s, to, from, tag, &end);
    if (at < 0 || at > to) { if (cap > 0) out[0] = 0; return; }
    {
        int stop = skip_el(s, to, end, tag);
        /* skip_el stops just after the '<' of the closing tag. */
        if (stop > end && s[stop - 1] == '<') stop--;
        plain(s, end, stop > end ? stop : end, out, cap);
    }
}

int wf_feed_looks(const char *s, int n) {
    int i;
    if (!s || n < 8) return 0;
    for (i = 0; i < n && i < 80 && (s[i] == ' ' || s[i] == '\n' || s[i] == '\r' || s[i] == '\t'); i++) {}
    if (starts(s, n, i, "<rss") || starts(s, n, i, "<feed") || starts(s, n, i, "<?xml")) return 1;
    return find_tag(s, n, 0, "rss", 0) >= 0 || find_tag(s, n, 0, "feed", 0) >= 0;
}

int wf_feed_parse(const char *s, int n, wf_feed_t *f) {
    int i, atom;
    if (!f) return -1;
    for (i = 0; i < (int)sizeof(*f); i++) ((unsigned char *)f)[i] = 0;
    if (!s || n < 0 || !wf_feed_looks(s, n)) return -1;
    atom = find_tag(s, n, 0, "feed", 0) >= 0 && find_tag(s, n, 0, "rss", 0) < 0;
    f->atom = atom;
    take_child(s, 0, n, "title", f->title, WF_FEED_TITLE);
    i = 0;
    while (i < n && f->n < WF_FEED_ITEMS) {
        const char *tag = atom ? "entry" : "item";
        int end = 0, at = find_tag(s, n, i, tag, &end);
        int stop;
        wf_feed_item_t *it;
        if (at < 0) break;
        stop = skip_el(s, n, end, tag);
        it = &f->item[f->n];
        take_child(s, end, stop, "title", it->title, WF_FEED_TITLE);
        if (atom) {
            if (!attr(s, end, stop, "href", it->link, WF_FEED_LINK))
                take_child(s, end, stop, "id", it->link, WF_FEED_LINK);
            take_child(s, end, stop, "summary", it->blurb, WF_FEED_BLURB);
            if (!it->blurb[0]) take_child(s, end, stop, "content", it->blurb, WF_FEED_BLURB);
        } else {
            take_child(s, end, stop, "link", it->link, WF_FEED_LINK);
            take_child(s, end, stop, "description", it->blurb, WF_FEED_BLURB);
        }
        if (it->title[0] || it->link[0]) f->n++;
        else f->dropped++;
        i = stop;
    }
    if (f->n == WF_FEED_ITEMS) {
        /* count leftovers so the caller can say the feed was cut */
        const char *tag = atom ? "entry" : "item";
        while (i < n) {
            int end = 0, at = find_tag(s, n, i, tag, &end);
            if (at < 0) break;
            f->dropped++;
            i = skip_el(s, n, end, tag);
        }
    }
    return 0;
}

static int app(char *o, int cap, int p, const char *s) {
    while (s && *s) { if (p >= cap - 1) return -1; o[p++] = *s++; }
    return p;
}

int wf_feed_text(const wf_feed_t *f, char *out, int cap) {
    int p = 0, i;
    if (!f || !out || cap < 8) return -1;
    p = app(out, cap, p, f->atom ? "atom " : "rss ");
    p = app(out, cap, p, f->title[0] ? f->title : "(sans titre)");
    if (p < 0 || p >= cap - 1) return -1;
    out[p++] = '\n';
    for (i = 0; i < f->n; i++) {
        p = app(out, cap, p, f->item[i].title[0] ? f->item[i].title : f->item[i].link);
        if (p < 0 || p >= cap - 1) return -1;
        out[p++] = '\n';
        if (f->item[i].link[0]) {
            p = app(out, cap, p, f->item[i].link);
            if (p < 0 || p >= cap - 1) return -1;
            out[p++] = '\n';
        }
        if (f->item[i].blurb[0]) {
            p = app(out, cap, p, f->item[i].blurb);
            if (p < 0 || p >= cap - 1) return -1;
            out[p++] = '\n';
        }
    }
    out[p] = 0;
    return p;
}
