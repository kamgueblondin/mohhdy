/* wf_sitecss.c - Lot 86: see wf_sitecss.h. Freestanding, no heap. */
#include "wf_sitecss.h"

int wf_site_apply(wf_site_t *d);

static int slen(const char *s) {
    int n = 0;
    while (s && s[n]) n++;
    return n;
}
static int eq(const char *a, const char *b) {
    int i = 0;
    if (!a || !b) return 0;
    while (a[i] && a[i] == b[i]) i++;
    return a[i] == b[i];
}
static char lc(char c) {
    return (c >= 'A' && c <= 'Z') ? (char)(c - 'A' + 'a') : c;
}
static int is_sp(char c) {
    return c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == '\f';
}
static void clear(void *p, int n) {
    unsigned char *b = (unsigned char *)p;
    int i;
    for (i = 0; i < n; i++) b[i] = 0;
}
static void scopy(char *d, int cap, const char *s, int n) {
    int i = 0;
    if (cap <= 0) return;
    while (i < n && s[i] && i < cap - 1) { d[i] = s[i]; i++; }
    d[i] = 0;
}
static void slower(char *d, int cap, const char *s, int n) {
    int i = 0;
    if (cap <= 0) return;
    while (i < n && s[i] && i < cap - 1) { d[i] = lc(s[i]); i++; }
    d[i] = 0;
}

static int void_tag(const char *t) {
    static const char *const v[] = {
        "area", "base", "br", "col", "embed", "hr", "img", "input", "link",
        "meta", "param", "source", "track", "wbr", 0
    };
    int i;
    for (i = 0; v[i]; i++) if (eq(t, v[i])) return 1;
    return 0;
}

static int skip_comment_html(const char *s, int n, int i) {
    if (i + 3 < n && s[i + 1] == '!' && s[i + 2] == '-' && s[i + 3] == '-') {
        i += 4;
        while (i + 2 < n && !(s[i] == '-' && s[i + 1] == '-' && s[i + 2] == '>')) i++;
        return i + 3 <= n ? i + 3 : n;
    }
    return i;
}

static int tag_end(const char *s, int n, int i) {
    int q = 0;
    while (i < n && (q || s[i] != '>')) {
        if (s[i] == '"' || s[i] == '\'') q = (q == s[i]) ? 0 : (q ? q : s[i]);
        i++;
    }
    return i;
}

static int name_at(const char *s, int n, int i, char *out, int cap) {
    int k = 0;
    while (i < n && ((s[i] >= 'A' && s[i] <= 'Z') || (s[i] >= 'a' && s[i] <= 'z') ||
                     (s[i] >= '0' && s[i] <= '9') || s[i] == '-' || s[i] == ':')) {
        if (k < cap - 1) out[k++] = lc(s[i]);
        i++;
    }
    if (cap > 0) out[k] = 0;
    return i;
}

/* Attribute value starting at s[i] (the first value byte, quote already
 * consumed). Writes into out and returns the index after the value. */
static int attr_value(const char *s, int n, int i, int end, char *out, int cap) {
    (void)n;
    int q = 0, k = 0;
    if (i < end && (s[i] == '"' || s[i] == '\'')) { q = s[i]; i++; }
    while (i < end) {
        if (q) { if (s[i] == q) { i++; break; } }
        else if (is_sp(s[i]) || s[i] == '>' || s[i] == '/') break;
        if (k < cap - 1) out[k++] = s[i];
        i++;
    }
    if (cap > 0) out[k] = 0;
    return i;
}

static void take_attr(wf_site_node_t *nd, const char *name, const char *val) {
    if (eq(name, "id") && !nd->id[0]) { scopy(nd->id, (int)sizeof(nd->id), val, slen(val)); return; }
    if (eq(name, "class")) {
        int i = 0;
        while (val[i] && nd->ncls < WF_SITE_CLS) {
            int a;
            while (val[i] && is_sp(val[i])) i++;
            a = i;
            while (val[i] && !is_sp(val[i])) i++;
            if (i > a && nd->ncls < WF_SITE_CLS) {
                scopy(nd->cls[nd->ncls], (int)sizeof(nd->cls[0]), val + a, i - a);
                nd->ncls++;
            }
        }
        return;
    }
    if (eq(name, "style") && !nd->style[0]) { scopy(nd->style, (int)sizeof(nd->style), val, slen(val)); return; }
    if (eq(name, "hidden")) { nd->hidden_attr = 1; return; }
    if (nd->nattr < WF_SITE_ATTR && (eq(name, "name") || eq(name, "type") || eq(name, "href") || eq(name, "role"))) {
        scopy(nd->aname[nd->nattr], (int)sizeof(nd->aname[0]), name, slen(name));
        scopy(nd->aval[nd->nattr], (int)sizeof(nd->aval[0]), val, slen(val));
        nd->nattr++;
    }
}

static void read_attrs(wf_site_node_t *nd, const char *s, int from, int end) {
    int i = from;
    while (i < end) {
        char name[16], val[48];
        while (i < end && (is_sp(s[i]) || s[i] == '/')) i++;
        if (i >= end || s[i] == '>') break;
        i = name_at(s, end, i, name, (int)sizeof(name));
        if (!name[0]) { i++; continue; }
        while (i < end && is_sp(s[i])) i++;
        val[0] = 0;
        if (i < end && s[i] == '=') {
            i++;
            while (i < end && is_sp(s[i])) i++;
            i = attr_value(s, end, i, end, val, (int)sizeof(val));
        }
        take_attr(nd, name, val);
    }
}

static int find_end_tag(const char *s, int n, int i, const char *tag) {
    int tl = slen(tag);
    while (i < n) {
        if (s[i] == '<' && i + 1 < n && s[i + 1] == '/') {
            int k = i + 2, j = 0;
            while (k < n && j < tl && lc(s[k]) == tag[j]) { k++; j++; }
            if (j == tl && (k >= n || s[k] == '>' || is_sp(s[k]))) {
                while (k < n && s[k] != '>') k++;
                return k < n ? k + 1 : n;
            }
        }
        i++;
    }
    return n;
}

static int css_append(wf_site_t *d, const char *s, int a, int b) {
    while (a < b) {
        if (d->css_n + 1 >= WF_SITE_CSS) return -1;
        d->css[d->css_n++] = s[a++];
    }
    if (d->css_n + 1 < WF_SITE_CSS) d->css[d->css_n++] = '\n';
    d->css[d->css_n] = 0;
    return 0;
}

static void copy_part(wf_site_part_t *d, const wf_site_part_t *s) {
    const unsigned char *a = (const unsigned char *)s;
    unsigned char *b = (unsigned char *)d;
    int i;
    for (i = 0; i < (int)sizeof(*d); i++) b[i] = a[i];
}

static void link_child(wf_site_t *d, int parent, int idx) {
    wf_site_node_t *nd = &d->node[idx];
    nd->parent = parent;
    nd->first = nd->last = nd->next = nd->prev = -1;
    if (parent < 0) {
        int k, prev = -1, count = 0;
        for (k = 0; k < idx; k++) if (d->node[k].parent < 0) { prev = k; count++; }
        nd->child_i = count + 1;
        nd->prev = prev;
        if (prev >= 0) d->node[prev].next = idx;
        nd->nsib = count + 1;
        return;
    }
    {
        wf_site_node_t *p = &d->node[parent];
        if (p->first < 0) p->first = idx;
        if (p->last >= 0) {
            d->node[p->last].next = idx;
            nd->prev = p->last;
        }
        p->last = idx;
        nd->child_i = (p->first == idx) ? 1 : d->node[nd->prev].child_i + 1;
    }
}

static void finish_counts(wf_site_t *d) {
    int i;
    for (i = 0; i < d->nnodes; i++) {
        int p = d->node[i].parent;
        int count = 0, c;
        if (p < 0) {
            int k;
            for (k = 0; k < d->nnodes; k++) if (d->node[k].parent < 0) count++;
            d->node[i].nsib = count;
        } else {
            for (c = d->node[p].first; c >= 0; c = d->node[c].next) count++;
            d->node[i].nsib = count;
        }
    }
}

int wf_site_prepare_vw(wf_site_t *d, const char *html, int n, int vw) {
    int i = 0, parent = -1;
    if (!d) return -1;
    clear(d, (int)sizeof(*d));
    d->vw = vw > 0 ? vw : 640;
    if (!html || n < 0) n = 0;
    while (i < n) {
        char tag[12];
        int end, close, self;
        if (html[i] != '<') { i++; continue; }
        if (i + 3 < n && html[i + 1] == '!' && html[i + 2] == '-' ) {
            i = skip_comment_html(html, n, i);
            continue;
        }
        if (i + 1 < n && (html[i + 1] == '!' || html[i + 1] == '?')) {
            end = tag_end(html, n, i + 2);
            i = end < n ? end + 1 : n;
            continue;
        }
        close = (i + 1 < n && html[i + 1] == '/');
        end = tag_end(html, n, i + 1 + close);
        name_at(html, n, i + 1 + close, tag, (int)sizeof(tag));
        self = (end > i && html[end - 1] == '/');
        if (!tag[0]) { i++; continue; }
        if (close) {
            while (parent >= 0 && !eq(d->node[parent].tag, tag)) parent = d->node[parent].parent;
            if (parent >= 0) parent = d->node[parent].parent;
            i = end < n ? end + 1 : n;
            continue;
        }
        if (eq(tag, "style")) {
            int body = end < n ? end + 1 : n;
            int after = find_end_tag(html, n, body, "style");
            int stop = after;
            /* drop the closing tag from the copied text */
            if (after > body) {
                int k = after - 1;
                while (k > body && html[k - 1] != '<') k--;
                if (k > body && html[k - 1] == '<') stop = k - 1;
            }
            if (css_append(d, html, body, stop) != 0) d->errors++;
            i = after;
            continue;
        }
        if (eq(tag, "script") || eq(tag, "noscript")) {
            int body = end < n ? end + 1 : n;
            i = find_end_tag(html, n, body, tag);
            continue;
        }
        if (d->nnodes >= WF_SITE_NODES) {
            d->dropped_nodes++;
            i = end < n ? end + 1 : n;
            if (!void_tag(tag) && !self) i = find_end_tag(html, n, i, tag);
            continue;
        }
        {
            int idx = d->nnodes++;
            wf_site_node_t *nd = &d->node[idx];
            clear(nd, (int)sizeof(*nd));
            scopy(nd->tag, (int)sizeof(nd->tag), tag, slen(tag));
            nd->start = i;
            read_attrs(nd, html, i + 1, end);
            link_child(d, parent, idx);
            if (!void_tag(tag) && !self) parent = idx;
        }
        i = end < n ? end + 1 : n;
    }
    finish_counts(d);
    return wf_site_apply(d);
}

int wf_site_prepare(wf_site_t *d, const char *html, int n) {
    return wf_site_prepare_vw(d, html, n, 640);
}

/* The apply step is below the parser. A forward declaration keeps the
 * builder above the cascade. */
int wf_site_apply(wf_site_t *d);

static int ident(const char *s, int n, int i, char *out, int cap) {
    int k = 0;
    if (i < n && s[i] == '*') {
        if (cap > 0) out[0] = 0;
        return i + 1;
    }
    while (i < n && ((s[i] >= 'A' && s[i] <= 'Z') || (s[i] >= 'a' && s[i] <= 'z') ||
                     (s[i] >= '0' && s[i] <= '9') || s[i] == '-' || s[i] == '_')) {
        if (k < cap - 1) out[k++] = lc(s[i]);
        i++;
    }
    if (cap > 0) out[k] = 0;
    return i;
}

static int hexdig(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
}

static int parse_color(const char *s, int n, uint32_t *out) {
    int i = 0;
    while (i < n && is_sp(s[i])) i++;
    if (i < n && s[i] == '#') {
        int k = 0, v[8];
        i++;
        while (i < n && k < 8 && hexdig(s[i]) >= 0) v[k++] = hexdig(s[i++]);
        if (k == 3) {
            *out = (uint32_t)((v[0] * 17 << 16) | (v[1] * 17 << 8) | (v[2] * 17));
            return 1;
        }
        if (k == 6) {
            *out = (uint32_t)((v[0] << 20) | (v[1] << 16) | (v[2] << 12) | (v[3] << 8) | (v[4] << 4) | v[5]);
            return 1;
        }
        return 0;
    }
    {
        char name[16];
        int k = 0;
        static const char *const nm[] = { "black", "white", "red", "green", "blue", "gray", "grey", "navy", 0 };
        static const uint32_t cv[] = { 0x000000u, 0xfffffeu, 0xff0000u, 0x008000u, 0x0000ffu, 0x808080u, 0x808080u, 0x000080u };
        while (i < n && k < 15 && !is_sp(s[i]) && s[i] != ';' && s[i] != '!') name[k++] = lc(s[i++]);
        name[k] = 0;
        /* white is 0xfffffe so a stored 0 is still "unset" only via color_set */
        for (k = 0; nm[k]; k++) if (eq(name, nm[k])) { *out = cv[k] == 0xfffffeu ? 0xffffffu : cv[k]; return 1; }
    }
    return 0;
}

static int nth_match(const wf_site_part_t *pt, int index) {
    int a = (int)pt->nth_a, b = pt->nth_b;
    if (pt->nth_b_neg) b = -b;
    if (index <= 0) return 0;
    if (a == 0) return index == b;
    if ((index - b) % a) return 0;
    return (index - b) / a >= 0;
}

static int has_class(const wf_site_node_t *nd, const char *c) {
    int i;
    for (i = 0; i < nd->ncls; i++) if (eq(nd->cls[i], c)) return 1;
    return 0;
}

static int has_attr(const wf_site_node_t *nd, const wf_site_part_t *pt) {
    int i;
    if (!pt->attr_kind) return 1;
    if (eq(pt->aname, "id")) {
        if (!nd->id[0]) return 0;
        if (pt->attr_kind == 1) return 1;
        return eq(nd->id, pt->aval);
    }
    if (eq(pt->aname, "class")) {
        if (pt->attr_kind == 1) return nd->ncls > 0;
        if (pt->attr_kind == 3) return has_class(nd, pt->aval);
        return has_class(nd, pt->aval);
    }
    if (eq(pt->aname, "hidden")) return nd->hidden_attr;
    for (i = 0; i < nd->nattr; i++) {
        if (!eq(nd->aname[i], pt->aname)) continue;
        if (pt->attr_kind == 1) return 1;
        if (pt->attr_kind == 2) return eq(nd->aval[i], pt->aval);
        if (pt->attr_kind == 3) {
            int p = 0;
            const char *v = nd->aval[i];
            int wl = slen(pt->aval);
            while (v[p]) {
                int a = p;
                while (v[p] && !is_sp(v[p])) p++;
                if (p - a == wl) {
                    int k = 0;
                    while (k < wl && v[a + k] == pt->aval[k]) k++;
                    if (k == wl) return 1;
                }
                while (v[p] && is_sp(v[p])) p++;
            }
        }
    }
    return 0;
}

static int part_match(const wf_site_t *d, int idx, const wf_site_part_t *pt) {
    const wf_site_node_t *nd;
    int i;
    if (idx < 0 || idx >= d->nnodes) return 0;
    nd = &d->node[idx];
    if (pt->tag[0] && !eq(pt->tag, nd->tag)) return 0;
    if (pt->id[0] && !eq(pt->id, nd->id)) return 0;
    for (i = 0; i < pt->ncls; i++) if (!has_class(nd, pt->cls[i])) return 0;
    if (!has_attr(nd, pt)) return 0;
    if (pt->pseudo == WF_SITE_PS_FIRST && nd->child_i != 1) return 0;
    if (pt->pseudo == WF_SITE_PS_LAST && nd->child_i != nd->nsib) return 0;
    if (pt->pseudo == WF_SITE_PS_LINK && !eq(nd->tag, "a")) return 0;
    if (pt->pseudo == WF_SITE_PS_NOT && pt->not_tag[0] && eq(nd->tag, pt->not_tag)) return 0;
    if (pt->pseudo == WF_SITE_PS_NTH && !nth_match(pt, nd->child_i)) return 0;
    return 1;
}

static int sel_match(const wf_site_t *d, int idx, const wf_site_rule_t *r) {
    int k, cur;
    if (r->nparts <= 0) return 0;
    if (!part_match(d, idx, &r->part[r->nparts - 1])) return 0;
    cur = idx;
    for (k = r->nparts - 1; k > 0; k--) {
        uint8_t rel = r->part[k].rel;
        if (rel == WF_SITE_REL_CHILD) {
            cur = cur >= 0 ? d->node[cur].parent : -1;
            if (!part_match(d, cur, &r->part[k - 1])) return 0;
        } else if (rel == WF_SITE_REL_DESC) {
            cur = cur >= 0 ? d->node[cur].parent : -1;
            while (cur >= 0 && !part_match(d, cur, &r->part[k - 1])) cur = d->node[cur].parent;
            if (cur < 0) return 0;
        } else if (rel == WF_SITE_REL_ADJ) {
            cur = cur >= 0 ? d->node[cur].prev : -1;
            if (!part_match(d, cur, &r->part[k - 1])) return 0;
        } else {
            cur = cur >= 0 ? d->node[cur].prev : -1;
            while (cur >= 0 && !part_match(d, cur, &r->part[k - 1])) cur = d->node[cur].prev;
            if (cur < 0) return 0;
        }
    }
    return 1;
}

static int specificity(const wf_site_rule_t *r) {
    int s = 0, p;
    for (p = 0; p < r->nparts; p++) {
        const wf_site_part_t *pt = &r->part[p];
        if (pt->id[0]) s += 10000;
        s += pt->ncls * 100;
        if (pt->attr_kind) s += 100;
        if (pt->pseudo == WF_SITE_PS_NOT) s += pt->not_tag[0] ? 1 : 0;
        else if (pt->pseudo) s += 100;
        if (pt->tag[0]) s += 1;
    }
    return s;
}

static int rank_of(const wf_site_rule_t *r) {
    /* important, then origin (inline > author > ua), then specificity, then order */
    return r->imp * 1000000000 + (int)r->origin * 1000000 + r->spec * 10 + (r->order & 1023);
}

static void paint_rule(wf_site_t *d, int idx, const wf_site_rule_t *r) {
    wf_site_node_t *nd = &d->node[idx];
    int rank = rank_of(r);
    if (r->kind == 1) {
        if (rank >= nd->disp_rank) { nd->disp = r->val; nd->disp_imp = r->imp; nd->disp_rank = rank; }
    } else if (r->kind == 2) {
        if (rank >= nd->weight_rank) { nd->weight = r->val; nd->weight_imp = r->imp; nd->weight_rank = rank; }
    } else if (r->kind == 3) {
        if (rank >= nd->align_rank) { nd->align = r->val; nd->align_imp = r->imp; nd->align_rank = rank; }
    } else if (r->kind == 4) {
        if (rank >= nd->color_rank) { nd->color = r->color; nd->color_set = 1; nd->color_imp = r->imp; nd->color_rank = rank; }
    } else if (r->kind == 5 || r->kind == 6) {
        uint8_t *side = r->kind == 5 ? nd->margin : nd->pad;
        uint8_t *flag = r->kind == 5 ? &nd->margin_set : &nd->pad_set;
        int *rr = r->kind == 5 ? &nd->margin_rank : &nd->pad_rank;
        int t, ri, bo, le;
        if (rank < *rr || r->box_n < 1) return;
        if (r->box_n == 1) { t = ri = bo = le = r->box[0]; }
        else if (r->box_n == 2) { t = bo = r->box[0]; ri = le = r->box[1]; }
        else if (r->box_n == 3) { t = r->box[0]; ri = le = r->box[1]; bo = r->box[2]; }
        else { t = r->box[0]; ri = r->box[1]; bo = r->box[2]; le = r->box[3]; }
        side[0] = (uint8_t)t; side[1] = (uint8_t)ri; side[2] = (uint8_t)bo; side[3] = (uint8_t)le;
        *flag = 0x0F;
        *rr = rank;
    }
}

static const wf_site_node_t *node_by_start(const wf_site_t *d, int offset) {
    int i;
    if (!d || !d->ready) return 0;
    for (i = 0; i < d->nnodes; i++) if (d->node[i].start == offset) return &d->node[i];
    return 0;
}

int wf_site_hidden_at(const wf_site_t *d, int offset) {
    const wf_site_node_t *nd = node_by_start(d, offset);
    int guard = 0;
    while (nd && guard++ < WF_SITE_NODES) {
        if (nd->disp == WF_SITE_DISP_NONE) return 1;
        if (nd->parent < 0) break;
        nd = &d->node[nd->parent];
    }
    return 0;
}

const wf_site_node_t *wf_site_at(const wf_site_t *d, int offset) {
    return node_by_start(d, offset);
}

static int app(char *o, int cap, int n, const char *s) {
    while (*s && n < cap - 1) o[n++] = *s++;
    if (n < cap) o[n] = 0;
    return n;
}

static int app_u(char *o, int cap, int n, unsigned v) {
    char t[8];
    int k = 0;
    if (!v) return app(o, cap, n, "0");
    while (v && k < 7) { t[k++] = (char)('0' + v % 10u); v /= 10u; }
    while (k) { char c[2]; c[0] = t[--k]; c[1] = 0; n = app(o, cap, n, c); }
    return n;
}

static int app_box(char *o, int cap, int n, const uint8_t side[4]) {
    int i;
    for (i = 0; i < 4; i++) {
        if (i) n = app(o, cap, n, " ");
        n = app_u(o, cap, n, side[i]);
        n = app(o, cap, n, "px");
    }
    return n;
}

static int app_hex(char *o, int cap, int n, uint32_t c) {
    char t[8];
    int i;
    const char *h = "0123456789abcdef";
    for (i = 5; i >= 0; i--) { t[5 - i] = h[(c >> (i * 4)) & 15]; }
    t[6] = 0;
    n = app(o, cap, n, "#");
    return app(o, cap, n, t);
}

int wf_site_style_attr(const wf_site_t *d, int offset, char *out, int cap) {
    const wf_site_node_t *nd = node_by_start(d, offset);
    int n = 0, any = 0;
    if (!out || cap < 8) return 0;
    out[0] = 0;
    if (!nd) return 0;
    if (!nd->color_set && nd->weight != 2 && nd->align < 2 && !nd->margin_set && !nd->pad_set) return 0;
    n = app(out, cap, n, " style=\"");
    if (nd->color_set) { n = app(out, cap, n, "color:"); n = app_hex(out, cap, n, nd->color); any = 1; }
    if (nd->weight == 2) { if (any) n = app(out, cap, n, ";"); n = app(out, cap, n, "font-weight:bold"); any = 1; }
    if (nd->align >= 2) {
        if (any) n = app(out, cap, n, ";");
        n = app(out, cap, n, "text-align:");
        n = app(out, cap, n, nd->align == 2 ? "center" : "right");
        any = 1;
    }
    if (nd->margin_set) {
        if (any) n = app(out, cap, n, ";");
        n = app(out, cap, n, "margin:");
        n = app_box(out, cap, n, nd->margin);
        any = 1;
    }
    if (nd->pad_set) {
        if (any) n = app(out, cap, n, ";");
        n = app(out, cap, n, "padding:");
        n = app_box(out, cap, n, nd->pad);
    }
    n = app(out, cap, n, "\"");
    return n;
}

int wf_site_report(const wf_site_t *d, char *out, int cap) {
    int n = 0, i;
    if (!out || cap < 8) return 0;
    out[0] = 0;
    if (!d || !d->ready) return app(out, cap, 0, "css none\n");
    n = app(out, cap, n, "css nodes ");
    {
        char t[12];
        int k = 0, v = d->nnodes;
        if (!v) t[k++] = '0';
        while (v && k < 11) { t[k++] = (char)('0' + v % 10); v /= 10; }
        while (k) { char c[2]; c[0] = t[--k]; c[1] = 0; n = app(out, cap, n, c); }
    }
    n = app(out, cap, n, " rules ");
    {
        char t[12];
        int k = 0, v = d->nrules;
        if (!v) t[k++] = '0';
        while (v && k < 11) { t[k++] = (char)('0' + v % 10); v /= 10; }
        while (k) { char c[2]; c[0] = t[--k]; c[1] = 0; n = app(out, cap, n, c); }
    }
    n = app(out, cap, n, "\n");
    for (i = 0; i < d->nnodes && n < cap - 48; i++) {
        const wf_site_node_t *nd = &d->node[i];
        n = app(out, cap, n, "node ");
        n = app(out, cap, n, nd->tag);
        if (nd->id[0]) { n = app(out, cap, n, " #"); n = app(out, cap, n, nd->id); }
        if (nd->ncls) { n = app(out, cap, n, " ."); n = app(out, cap, n, nd->cls[0]); }
        n = app(out, cap, n, nd->disp == WF_SITE_DISP_NONE ? " display none" :
                              nd->disp == WF_SITE_DISP_INLINE ? " display inline" :
                              nd->disp == WF_SITE_DISP_BLOCK ? " display block" : " display auto");
        if (nd->weight == 2) n = app(out, cap, n, " weight bold");
        if (nd->align == 2) n = app(out, cap, n, " align center");
        else if (nd->align == 3) n = app(out, cap, n, " align right");
        if (nd->color_set) { n = app(out, cap, n, " color "); n = app_hex(out, cap, n, nd->color); }
        n = app(out, cap, n, "\n");
    }
    return n;
}

/* ---- declaration and selector parsers --------------------------------- */

static int skip_noise(const char *s, int n, int i) {
    for (;;) {
        int p = i;
        while (i < n && is_sp(s[i])) i++;
        if (i + 1 < n && s[i] == '/' && s[i + 1] == '*') {
            i += 2;
            while (i + 1 < n && !(s[i] == '*' && s[i + 1] == '/')) i++;
            i = i + 2 <= n ? i + 2 : n;
        }
        if (i == p) return i;
    }
}

static int skip_block(const char *s, int n, int i) {
    int depth = 0;
    if (i < n && s[i] == '{') { depth = 1; i++; }
    while (i < n && depth) {
        if (s[i] == '{') depth++;
        else if (s[i] == '}') depth--;
        else if (s[i] == '"' || s[i] == '\'') {
            char q = s[i++];
            while (i < n && s[i] != q) i++;
        }
        i++;
    }
    return i;
}

static int add_rule(wf_site_t *d, const wf_site_rule_t *src) {
    wf_site_rule_t *r;
    int p, b;
    if (d->nrules >= WF_SITE_RULES) { d->dropped_rules++; return -1; }
    r = &d->rule[d->nrules];
    clear(r, (int)sizeof(*r));
    r->nparts = src->nparts;
    r->imp = src->imp;
    r->origin = src->origin;
    r->kind = src->kind;
    r->val = src->val;
    r->box_n = src->box_n;
    r->box[0] = src->box[0];
    r->box[1] = src->box[1];
    r->box[2] = src->box[2];
    r->box[3] = src->box[3];
    r->color = src->color;
    r->order = d->nrules;
    for (p = 0; p < src->nparts && p < WF_SITE_PARTS; p++) {
        const unsigned char *a = (const unsigned char *)&src->part[p];
        unsigned char *dst = (unsigned char *)&r->part[p];
        for (b = 0; b < (int)sizeof(wf_site_part_t); b++) dst[b] = a[b];
    }
    r->spec = specificity(r);
    d->nrules++;
    return 0;
}

static int display_val(const char *s) {
    if (eq(s, "none")) return WF_SITE_DISP_NONE;
    if (eq(s, "inline")) return WF_SITE_DISP_INLINE;
    if (eq(s, "block") || eq(s, "flex") || eq(s, "list-item") || eq(s, "table") || eq(s, "flow-root"))
        return WF_SITE_DISP_BLOCK;
    return 0;
}

static int weight_val(const char *s) {
    int n = 0, i = 0;
    if (eq(s, "bold") || eq(s, "bolder")) return 2;
    if (eq(s, "normal") || eq(s, "lighter")) return 1;
    while (s[i] >= '0' && s[i] <= '9') { n = n * 10 + s[i] - '0'; i++; }
    if (i && n >= 600) return 2;
    if (i && n > 0) return 1;
    return 0;
}

static int align_val(const char *s) {
    if (eq(s, "left") || eq(s, "start")) return 1;
    if (eq(s, "center")) return 2;
    if (eq(s, "right") || eq(s, "end")) return 3;
    return 0;
}

static void trim_word(const char *s, int n, char *out, int cap, int *imp) {
    int a = 0, b = n, k = 0;
    *imp = 0;
    while (a < b && is_sp(s[a])) a++;
    while (b > a && is_sp(s[b - 1])) b--;
    if (b - a >= 10) {
        int p = a;
        while (p + 10 <= b) {
            if (s[p] == '!' && lc(s[p + 1]) == 'i') {
                *imp = 1;
                b = p;
                while (b > a && is_sp(s[b - 1])) b--;
                break;
            }
            p++;
        }
    }
    while (a < b && k < cap - 1) out[k++] = lc(s[a++]);
    out[k] = 0;
}

static int parse_box(const char *s, wf_site_rule_t *proto) {
    int i = 0, n = 0;
    while (s[i] && n < 4) {
        int v = 0, any = 0;
        while (is_sp(s[i])) i++;
        if (!s[i]) break;
        while (s[i] >= '0' && s[i] <= '9') { v = v * 10 + (s[i] - '0'); i++; any = 1; }
        if (!any) return 0;
        if (v > 255) v = 255;
        if (s[i] == 'p' && s[i + 1] == 'x') i += 2;
        proto->box[n++] = (uint8_t)v;
    }
    proto->box_n = (uint8_t)n;
    return n > 0;
}

static int parse_decl(const char *body, int n, wf_site_rule_t *proto) {
    int i = 0;
    proto->kind = 0;
    while (i < n) {
        char prop[24], val[48];
        int a, b, imp = 0, vs, ve;
        i = skip_noise(body, n, i);
        a = i;
        while (i < n && body[i] != ':' && body[i] != ';' && body[i] != '}') i++;
        if (i >= n || body[i] != ':') { while (i < n && body[i] != ';') i++; if (i < n) i++; continue; }
        b = i;
        while (b > a && is_sp(body[b - 1])) b--;
        slower(prop, (int)sizeof(prop), body + a, b - a);
        i++;
        vs = i;
        while (i < n && body[i] != ';') i++;
        ve = i;
        trim_word(body + vs, ve - vs, val, (int)sizeof(val), &imp);
        proto->imp = (uint8_t)imp;
        if (eq(prop, "display")) {
            int v = display_val(val);
            if (v) { proto->kind = 1; proto->val = (uint8_t)v; return 1; }
        } else if (eq(prop, "font-weight")) {
            int v = weight_val(val);
            if (v) { proto->kind = 2; proto->val = (uint8_t)v; return 1; }
        } else if (eq(prop, "text-align")) {
            int v = align_val(val);
            if (v) { proto->kind = 3; proto->val = (uint8_t)v; return 1; }
        } else if (eq(prop, "color")) {
            uint32_t c = 0;
            if (parse_color(val, slen(val), &c)) { proto->kind = 4; proto->color = c; proto->val = 1; return 1; }
        } else if (eq(prop, "margin") || eq(prop, "padding")) {
            if (parse_box(val, proto)) { proto->kind = (uint8_t)(eq(prop, "margin") ? 5 : 6); return 1; }
        }
        if (i < n && body[i] == ';') i++;
    }
    return 0;
}

static int parse_nth(const char *s, int n, wf_site_part_t *pt) {
    int i = 0, a = 0, b = 0, bneg = 0, sign = 1;
    while (i < n && is_sp(s[i])) i++;
    if (i + 3 <= n && lc(s[i]) == 'o' && lc(s[i + 1]) == 'd' && lc(s[i + 2]) == 'd') { a = 2; b = 1; }
    else if (i + 4 <= n && lc(s[i]) == 'e') { a = 2; b = 0; }
    else {
        if (s[i] == '+' || s[i] == '-') { sign = s[i] == '-' ? -1 : 1; i++; }
        if (i < n && lc(s[i]) == 'n') { a = sign; }
        else {
            int v = 0, any = 0;
            while (i < n && s[i] >= '0' && s[i] <= '9') { v = v * 10 + s[i] - '0'; i++; any = 1; }
            if (!any) return 0;
            if (i < n && lc(s[i]) == 'n') { a = sign * v; i++; }
            else { a = 0; b = sign < 0 ? v : v; bneg = sign < 0; pt->nth_a = 0; pt->nth_b = b; pt->nth_b_neg = (uint8_t)bneg; return 1; }
        }
        if (i < n && lc(s[i]) == 'n') i++;
        while (i < n && is_sp(s[i])) i++;
        if (i < n && (s[i] == '+' || s[i] == '-')) {
            int v = 0;
            bneg = s[i] == '-';
            i++;
            while (i < n && is_sp(s[i])) i++;
            while (i < n && s[i] >= '0' && s[i] <= '9') { v = v * 10 + s[i] - '0'; i++; }
            b = v;
        }
    }
    if (a < 0) a = 0;
    if (a > 255) a = 255;
    if (b > 255) b = 255;
    pt->nth_a = (uint8_t)a;
    pt->nth_b = b;
    pt->nth_b_neg = (uint8_t)bneg;
    return 1;
}

static int parse_compound(const char *s, int n, int i, wf_site_part_t *pt) {
    clear(pt, (int)sizeof(*pt));
    i = skip_noise(s, n, i);
    if (i < n && s[i] != '.' && s[i] != '#' && s[i] != '[' && s[i] != ':' && s[i] != '*')
        i = ident(s, n, i, pt->tag, (int)sizeof(pt->tag));
    else if (i < n && s[i] == '*') i++;
    while (i < n) {
        if (s[i] == '#') {
            i = ident(s, n, i + 1, pt->id, (int)sizeof(pt->id));
        } else if (s[i] == '.') {
            if (pt->ncls < 2) i = ident(s, n, i + 1, pt->cls[pt->ncls], (int)sizeof(pt->cls[0]));
            else { i++; while (i < n && s[i] != '.' && s[i] != '#' && s[i] != '[' && s[i] != ':' && !is_sp(s[i]) && s[i] != '>' && s[i] != '+' && s[i] != '~' && s[i] != ',' && s[i] != '{') i++; }
            if (pt->ncls < 2 && pt->cls[pt->ncls][0]) pt->ncls++;
        } else if (s[i] == '[') {
            char name[12], val[20];
            int kind = 1;
            i++;
            i = skip_noise(s, n, i);
            i = ident(s, n, i, name, (int)sizeof(name));
            i = skip_noise(s, n, i);
            val[0] = 0;
            if (i + 1 < n && s[i] == '~' && s[i + 1] == '=') { kind = 3; i += 2; }
            else if (i < n && s[i] == '=') { kind = 2; i++; }
            if (kind != 1) {
                i = skip_noise(s, n, i);
                i = attr_value(s, n, i, n, val, (int)sizeof(val));
            }
            while (i < n && s[i] != ']') i++;
            if (i < n) i++;
            scopy(pt->aname, (int)sizeof(pt->aname), name, slen(name));
            scopy(pt->aval, (int)sizeof(pt->aval), val, slen(val));
            pt->attr_kind = (uint8_t)kind;
        } else if (s[i] == ':') {
            char ps[16];
            i++;
            if (i < n && s[i] == ':') i++;   /* ::pseudo-element is ignored */
            i = ident(s, n, i, ps, (int)sizeof(ps));
            if (eq(ps, "first-child")) pt->pseudo = WF_SITE_PS_FIRST;
            else if (eq(ps, "last-child")) pt->pseudo = WF_SITE_PS_LAST;
            else if (eq(ps, "link") || eq(ps, "visited")) pt->pseudo = WF_SITE_PS_LINK;
            else if (eq(ps, "not") && i < n && s[i] == '(') {
                i++;
                i = skip_noise(s, n, i);
                i = ident(s, n, i, pt->not_tag, (int)sizeof(pt->not_tag));
                while (i < n && s[i] != ')') i++;
                if (i < n) i++;
                pt->pseudo = WF_SITE_PS_NOT;
            } else if (eq(ps, "nth-child") && i < n && s[i] == '(') {
                int a = ++i;
                while (i < n && s[i] != ')') i++;
                if (!parse_nth(s + a, i - a, pt)) return -1;
                if (i < n) i++;
                pt->pseudo = WF_SITE_PS_NTH;
            }
        } else break;
    }
    return i;
}

static int parse_selector(wf_site_t *d, const char *s, int n, const wf_site_rule_t *proto) {
    int i = 0;
    while (i < n) {
        wf_site_rule_t rule;
        int parts = 0;
        uint8_t rel = WF_SITE_REL_DESC;
        clear(&rule, (int)sizeof(rule));
        rule.kind = proto->kind;
        rule.val = proto->val;
        rule.box_n = proto->box_n;
        rule.box[0] = proto->box[0];
        rule.box[1] = proto->box[1];
        rule.box[2] = proto->box[2];
        rule.box[3] = proto->box[3];
        rule.color = proto->color;
        rule.imp = proto->imp;
        rule.origin = proto->origin;
        i = skip_noise(s, n, i);
        while (i < n && s[i] != ',') {
            wf_site_part_t part;
            int before;
            i = skip_noise(s, n, i);
            if (i >= n || s[i] == ',') break;
            if (s[i] == '>') { rel = WF_SITE_REL_CHILD; i++; continue; }
            if (s[i] == '+') { rel = WF_SITE_REL_ADJ; i++; continue; }
            if (s[i] == '~') { rel = WF_SITE_REL_SIB; i++; continue; }
            before = i;
            i = parse_compound(s, n, i, &part);
            if (i < 0) return -1;
            if (i == before) { i++; continue; }
            part.rel = parts ? rel : WF_SITE_REL_DESC;
            rel = WF_SITE_REL_DESC;
            if (parts < WF_SITE_PARTS) copy_part(&rule.part[parts++], &part);
            else d->errors++;
        }
        rule.nparts = parts;
        if (parts) add_rule(d, &rule);
        if (i < n && s[i] == ',') i++;
    }
    return 0;
}

static int media_px(const char *s, int n, const char *key) {
    int i = 0, kl = slen(key);
    while (i + kl < n) {
        int k = 0;
        while (k < kl && lc(s[i + k]) == key[k]) k++;
        if (k == kl) {
            int v = 0, any = 0;
            i += kl;
            while (i < n && (is_sp(s[i]) || s[i] == ':')) i++;
            while (i < n && s[i] >= '0' && s[i] <= '9') { v = v * 10 + s[i] - '0'; i++; any = 1; }
            if (any) return v;
            return -1;
        }
        i++;
    }
    return -1;
}

/* 1 when this @media prelude applies at viewport vw. Unknown features
 * (screen, print) apply. "not" does not. */
static int media_ok(const char *s, int n, int vw) {
    int mn, mx, i;
    for (i = 0; i < n; i++) if (lc(s[i]) == 'n' && i + 3 <= n && lc(s[i + 1]) == 'o' && lc(s[i + 2]) == 't') return 0;
    mn = media_px(s, n, "min-width");
    mx = media_px(s, n, "max-width");
    if (mn >= 0 && vw < mn) return 0;
    if (mx >= 0 && vw > mx) return 0;
    return 1;
}

static int parse_sheet(wf_site_t *d, const char *s, int n, uint8_t origin) {
    int i = 0;
    while (i < n) {
        int sel, brace;
        i = skip_noise(s, n, i);
        if (i >= n) break;
        if (s[i] == '@') {
            int prelude = i;
            while (i < n && s[i] != '{' && s[i] != ';') i++;
            if (i < n && s[i] == ';') { i++; continue; }
            if (i < n && s[i] == '{' && media_ok(s + prelude, i - prelude, d->vw)) {
                int body = i + 1;
                int after = skip_block(s, n, i);
                int blen = after - 1 - body;
                if (blen > 0) parse_sheet(d, s + body, blen, origin);
                i = after;
            } else i = skip_block(s, n, i);
            continue;
        }
        sel = i;
        while (i < n && s[i] != '{') i++;
        if (i >= n) break;
        brace = i;
        i = skip_block(s, n, i);
        {
            wf_site_rule_t proto;
            int body = brace + 1;
            int body_n = i - 1 - body;
            if (body_n < 0) body_n = 0;
            clear(&proto, (int)sizeof(proto));
            proto.origin = origin;
            /* one declaration per rule keeps the cascade table small; a
             * block with several properties becomes several selectors */
            {
                int p = 0;
                while (p < body_n) {
                    int semi = p;
                    while (semi < body_n && s[body + semi] != ';') semi++;
                    clear(&proto, (int)sizeof(proto));
                    proto.origin = origin;
                    if (parse_decl(s + body + p, semi - p, &proto) && proto.kind)
                        parse_selector(d, s + sel, brace - sel, &proto);
                    p = semi < body_n ? semi + 1 : body_n;
                }
            }
        }
    }
    return 0;
}

static void add_ua(wf_site_t *d, const char *sel, int disp) {
    wf_site_rule_t proto;
    clear(&proto, (int)sizeof(proto));
    proto.origin = 1;
    proto.kind = 1;
    proto.val = (uint8_t)disp;
    parse_selector(d, sel, slen(sel), &proto);
}

static void apply_inline(wf_site_t *d) {
    int i;
    for (i = 0; i < d->nnodes; i++) {
        wf_site_rule_t proto;
        if (!d->node[i].style[0]) continue;
        clear(&proto, (int)sizeof(proto));
        proto.origin = 3;
        if (!parse_decl(d->node[i].style, slen(d->node[i].style), &proto) || !proto.kind) continue;
        proto.spec = 100000;
        proto.order = 1000 + i;
        proto.nparts = 0;
        paint_rule(d, i, &proto);
    }
}

int wf_site_apply(wf_site_t *d) {
    int i, r;
    if (!d) return -1;
    add_ua(d, "[hidden]", WF_SITE_DISP_NONE);
    add_ua(d, "script", WF_SITE_DISP_NONE);
    add_ua(d, "style", WF_SITE_DISP_NONE);
    if (d->css_n) parse_sheet(d, d->css, d->css_n, 2);
    for (r = 0; r < d->nrules; r++) {
        for (i = 0; i < d->nnodes; i++) if (sel_match(d, i, &d->rule[r])) paint_rule(d, i, &d->rule[r]);
    }
    apply_inline(d);
    d->ready = 1;
    return 0;
}
