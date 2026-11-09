/* wf_blocks.c - Lot 86: see wf_blocks.h. */
#include "wf_blocks.h"

#define BL_TABLES 3
#define BL_ROWS 8
#define BL_COLS 4
#define BL_CELL 40
#define BL_DEFS 12
#define BL_FIGS 4
#define BL_DETAILS 6

static char bl_low(char c) { return (c >= 'A' && c <= 'Z') ? (char)(c + 32) : c; }
static int bl_len(const char *s) { int n = 0; while (s && s[n]) n++; return n; }
static int bl_sp(char c) { return c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == '\f'; }

static int name_is(const char *s, int n, int i, const char *name) {
    int k = 0;
    if (i < n && s[i] == '/') i++;
    while (name[k]) {
        if (i >= n || bl_low(s[i]) != name[k]) return 0;
        i++; k++;
    }
    return i >= n || bl_sp(s[i]) || s[i] == '>' || s[i] == '/';
}

static int find_tag(const char *s, int n, int from, const char *name, int *close, int *end) {
    int i;
    for (i = from; i < n; i++) {
        if (s[i] != '<') continue;
        if (i + 1 < n && s[i + 1] == '!') {
            /* comment or doctype: skip to '>' */
            int j = i + 2;
            if (j + 1 < n && s[j] == '-' && s[j + 1] == '-') {
                j += 2;
                while (j + 2 < n && !(s[j] == '-' && s[j + 1] == '-' && s[j + 2] == '>')) j++;
                i = j + 2;
                continue;
            }
            while (i < n && s[i] != '>') i++;
            continue;
        }
        {
            int c = (i + 1 < n && s[i + 1] == '/');
            int j = i + 1 + c;
            if (!name_is(s, n, j, name)) continue;
            while (j < n && s[j] != '>') j++;
            if (close) *close = c;
            if (end) *end = j < n ? j + 1 : n;
            return i;
        }
    }
    return -1;
}

static int skip_block(const char *s, int n, int i, const char *name) {
    int depth = 1;
    while (i < n && depth) {
        int close = 0, end = 0, at = find_tag(s, n, i, name, &close, &end);
        if (at < 0) return n;
        if (close) depth--; else depth++;
        i = end;
    }
    return i;
}

static int hexv(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
}

/* Plain text of [from, to), tags removed, a few entities decoded, collapsed
 * spaces. Stops at cap-1. */
static void cell_text(const char *s, int from, int to, char *out, int cap) {
    int i = from, o = 0, sp = 1;
    if (cap <= 0) return;
    while (i < to && o < cap - 1) {
        unsigned char c = (unsigned char)s[i];
        if (c == '<') {
            if (i + 3 < to && s[i + 1] == 'b' && s[i + 2] == 'r') { if (!sp && o < cap - 1) { out[o++] = ' '; sp = 1; } }
            while (i < to && s[i] != '>') i++;
            if (i < to) i++;
            continue;
        }
        if (c == '&') {
            int k = i + 1, code = 0;
            if (k + 2 < to && s[k] == 'a' && s[k + 1] == 'm' && s[k + 2] == 'p') { c = '&'; i = k + 3; }
            else if (k + 1 < to && s[k] == 'l' && s[k + 1] == 't') { c = '<'; i = k + 2; }
            else if (k + 1 < to && s[k] == 'g' && s[k + 1] == 't') { c = '>'; i = k + 2; }
            else if (k + 3 < to && s[k] == 'n' && s[k + 1] == 'b' && s[k + 2] == 's' && s[k + 3] == 'p') { c = ' '; i = k + 4; }
            else if (k < to && s[k] == '#') {
                int hex = (k + 1 < to && (s[k + 1] == 'x' || s[k + 1] == 'X'));
                int p = k + 1 + hex;
                while (p < to && s[p] != ';') {
                    int h = hexv(s[p]);
                    if (h < 0 || (!hex && (s[p] < '0' || s[p] > '9'))) break;
                    code = code * (hex ? 16 : 10) + (hex ? h : s[p] - '0');
                    p++;
                }
                if (p < to && s[p] == ';') {
                    c = (code >= 32 && code < 127) ? (unsigned char)code : ' ';
                    i = p + 1;
                } else { i++; c = '&'; }
            } else { i++; c = '&'; }
            if (s[i - 1] != ';') {
                /* the simple named forms advanced i past the name; require ';' */
            }
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

static int ob_add(char *out, int cap, int *n, const char *s) {
    int i = 0;
    if (*n < 0) return -1;
    while (s && s[i]) {
        if (*n >= cap - 1) { *n = -1; return -1; }
        out[(*n)++] = s[i++];
    }
    return 0;
}

static int ob_ch(char *out, int cap, int *n, char c) {
    char s[2];
    s[0] = c; s[1] = 0;
    return ob_add(out, cap, n, s);
}

static void one_table(const char *s, int from, int to, char *out, int cap, int *n, wf_blocks_stats_t *st, int index) {
    char cell[BL_COLS][BL_CELL];
    int row = 0, i = from;
    char head[16];
    int h = 0;
    head[h++] = 't'; head[h++] = 'a'; head[h++] = 'b'; head[h++] = 'l'; head[h++] = 'e'; head[h++] = ' ';
    head[h++] = (char)('0' + (index % 10));
    head[h++] = '\n';
    head[h] = 0;
    if (ob_add(out, cap, n, head) != 0) { if (st) st->truncated = 1; return; }
    if (st) st->tables++;
    while (i < to && row < BL_ROWS) {
        int close = 0, end = 0, at = find_tag(s, to, i, "tr", &close, &end);
        int row_end, col = 0, j;
        if (at < 0 || close) break;
        row_end = skip_block(s, to, end, "tr");
        for (col = 0; col < BL_COLS; col++) cell[col][0] = 0;
        col = 0;
        j = end;
        while (j < row_end && col < BL_COLS) {
            int cclose = 0, cend = 0;
            int td = find_tag(s, row_end, j, "td", &cclose, &cend);
            int th = find_tag(s, row_end, j, "th", &cclose, &cend);
            int which, wend, inner;
            const char *nm;
            if (td < 0 && th < 0) break;
            if (th >= 0 && (td < 0 || th < td)) { which = th; wend = cend; nm = "th"; }
            else {
                which = td;
                /* recompute end for td if th was found first in the other call */
                find_tag(s, row_end, j, "td", &cclose, &wend);
                nm = "td";
                (void)which;
            }
            if (cclose) { j = wend; continue; }
            inner = skip_block(s, row_end, wend, nm);
            cell_text(s, wend, inner > wend ? inner - 1 : wend, cell[col], BL_CELL);
            if (st) st->cells++;
            col++;
            j = inner;
        }
        if (col) {
            int c;
            for (c = 0; c < col; c++) {
                if (c && ob_add(out, cap, n, " | ") != 0) { if (st) st->truncated = 1; return; }
                if (ob_add(out, cap, n, cell[c][0] ? cell[c] : "-") != 0) { if (st) st->truncated = 1; return; }
            }
            if (ob_ch(out, cap, n, '\n') != 0) { if (st) st->truncated = 1; return; }
            if (st) st->rows++;
            row++;
        }
        i = row_end;
    }
}

static void one_dl(const char *s, int from, int to, char *out, int cap, int *n, wf_blocks_stats_t *st) {
    int i = from, kept = 0;
    if (ob_add(out, cap, n, "dl\n") != 0) { if (st) st->truncated = 1; return; }
    while (i < to && kept < BL_DEFS) {
        int close = 0, end = 0, at = find_tag(s, to, i, "dt", &close, &end);
        char term[BL_CELL], def[BL_CELL];
        int dend;
        if (at < 0 || close) break;
        dend = skip_block(s, to, end, "dt");
        cell_text(s, end, dend > end ? dend - 1 : end, term, BL_CELL);
        i = dend;
        def[0] = 0;
        {
            int dc = 0, de = 0, da = find_tag(s, to, i, "dd", &dc, &de);
            if (da >= 0 && !dc) {
                int ddend = skip_block(s, to, de, "dd");
                cell_text(s, de, ddend > de ? ddend - 1 : de, def, BL_CELL);
                i = ddend;
            }
        }
        if (term[0] || def[0]) {
            if (ob_add(out, cap, n, "  ") != 0 || ob_add(out, cap, n, term[0] ? term : "?") != 0 ||
                ob_add(out, cap, n, ": ") != 0 || ob_add(out, cap, n, def) != 0 ||
                ob_ch(out, cap, n, '\n') != 0) { if (st) st->truncated = 1; return; }
            if (st) st->defs++;
            kept++;
        }
    }
}

static int has_open(const char *s, int from, int tag_end) {
    int i;
    for (i = from; i < tag_end; i++) {
        if ((s[i] == 'o' || s[i] == 'O') && i + 4 <= tag_end &&
            bl_low(s[i + 1]) == 'p' && bl_low(s[i + 2]) == 'e' && bl_low(s[i + 3]) == 'n' &&
            (i + 4 == tag_end || bl_sp(s[i + 4]) || s[i + 4] == '>' || s[i + 4] == '='))
            return 1;
    }
    return 0;
}

static void one_details(const char *s, int tag, int tag_end, int block_end, char *out, int cap, int *n, wf_blocks_stats_t *st) {
    int open = has_open(s, tag, tag_end);
    int close = 0, end = 0, at = find_tag(s, block_end, tag_end, "summary", &close, &end);
    char sum[BL_CELL];
    sum[0] = 0;
    if (at >= 0 && !close) {
        int se = skip_block(s, block_end, end, "summary");
        cell_text(s, end, se > end ? se - 1 : end, sum, BL_CELL);
    }
    if (ob_add(out, cap, n, open ? "details open " : "details ") != 0 ||
        ob_add(out, cap, n, sum[0] ? sum : "(sans resume)") != 0 ||
        ob_ch(out, cap, n, '\n') != 0) { if (st) st->truncated = 1; return; }
    if (st) st->details++;
    if (open) {
        char body[80];
        cell_text(s, tag_end, block_end, body, 80);
        if (body[0] && (ob_add(out, cap, n, "  ") != 0 || ob_add(out, cap, n, body) != 0 ||
                        ob_ch(out, cap, n, '\n') != 0)) { if (st) st->truncated = 1; }
    }
}

static void one_figure(const char *s, int from, int to, char *out, int cap, int *n, wf_blocks_stats_t *st) {
    int close = 0, end = 0, at = find_tag(s, to, from, "figcaption", &close, &end);
    char captext[BL_CELL];
    captext[0] = 0;
    if (at >= 0 && !close) {
        int ce = skip_block(s, to, end, "figcaption");
        cell_text(s, end, ce > end ? ce - 1 : end, captext, BL_CELL);
    }
    if (!captext[0]) cell_text(s, from, to, captext, BL_CELL);
    if (ob_add(out, cap, n, "figure ") != 0 || ob_add(out, cap, n, captext[0] ? captext : "(vide)") != 0 ||
        ob_ch(out, cap, n, '\n') != 0) { if (st) st->truncated = 1; return; }
    if (st) st->figures++;
}

int wf_blocks_text(const char *html, int n, char *out, int cap, wf_blocks_stats_t *st) {
    int i = 0, o = 0, tables = 0, dls = 0, figs = 0, dets = 0;
    wf_blocks_stats_t local;
    if (st) { int k; unsigned char *p = (unsigned char *)st; for (k = 0; k < (int)sizeof(*st); k++) p[k] = 0; }
    else st = &local;
    if (!html || n < 0 || !out || cap < 2) return 0;
    out[0] = 0;
    while (i < n) {
        int close = 0, end = 0;
        if (html[i] != '<') { i++; continue; }
        if (name_is(html, n, i + 1, "script") || name_is(html, n, i + 1, "style")) {
            const char *nm = name_is(html, n, i + 1, "script") ? "script" : "style";
            while (i < n && html[i] != '>') i++;
            i = skip_block(html, n, i < n ? i + 1 : n, nm);
            continue;
        }
        if (tables < BL_TABLES && name_is(html, n, i + 1, "table") && !(i + 1 < n && html[i + 1] == '/')) {
            int at = find_tag(html, n, i, "table", &close, &end);
            int be;
            if (at == i && !close) {
                be = skip_block(html, n, end, "table");
                one_table(html, end, be, out, cap, &o, st, tables + 1);
                tables++;
                i = be;
                if (o < 0) break;
                continue;
            }
        }
        if (dls < 2 && name_is(html, n, i + 1, "dl") && !(i + 1 < n && html[i + 1] == '/')) {
            int at = find_tag(html, n, i, "dl", &close, &end);
            if (at == i && !close) {
                int be = skip_block(html, n, end, "dl");
                one_dl(html, end, be, out, cap, &o, st);
                dls++;
                i = be;
                if (o < 0) break;
                continue;
            }
        }
        if (figs < BL_FIGS && name_is(html, n, i + 1, "figure") && !(i + 1 < n && html[i + 1] == '/')) {
            int at = find_tag(html, n, i, "figure", &close, &end);
            if (at == i && !close) {
                int be = skip_block(html, n, end, "figure");
                one_figure(html, end, be, out, cap, &o, st);
                figs++;
                i = be;
                if (o < 0) break;
                continue;
            }
        }
        if (dets < BL_DETAILS && name_is(html, n, i + 1, "details") && !(i + 1 < n && html[i + 1] == '/')) {
            int at = find_tag(html, n, i, "details", &close, &end);
            if (at == i && !close) {
                int be = skip_block(html, n, end, "details");
                one_details(html, i, end, be, out, cap, &o, st);
                dets++;
                i = be;
                if (o < 0) break;
                continue;
            }
        }
        i++;
    }
    if (o < 0) {
        st->truncated = 1;
        if (cap > 0) out[cap - 1] = 0;
        return bl_len(out);
    }
    out[o] = 0;
    return o;
}
