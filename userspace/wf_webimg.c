/* wf_webimg.c - Lot 72: fetched images of the guest browser (see wf_webimg.h). */
#include "wf_webimg.h"
#include "imgdec.h"
#include "webeng.h"

static int s_len(const char* s) { int n = 0; while (s[n]) n++; return n; }
static int s_eq(const char* a, const char* b) { while (*a && *a == *b) { a++; b++; } return *a == *b; }
static void s_cpy(char* d, const char* s, int cap) {
    int i;
    for (i = 0; i < cap - 1 && s[i]; i++) d[i] = s[i];
    d[i] = 0;
}
static char lc(char c) { return (c >= 'A' && c <= 'Z') ? (char)(c + 32) : c; }
static int is_sp(char c) { return c == ' ' || c == '\t' || c == '\n' || c == '\r'; }

static void err(wf_webimg_stats_t* st, const char* what, const char* url) {
    int k = 0, i;
    if (!st) return;
    for (i = 0; what[i] && k < (int)sizeof(st->last_err) - 1; i++) st->last_err[k++] = what[i];
    if (url && k < (int)sizeof(st->last_err) - 2) {
        st->last_err[k++] = ' ';
        for (i = 0; url[i] && k < (int)sizeof(st->last_err) - 1; i++) st->last_err[k++] = url[i];
    }
    st->last_err[k] = 0;
}


void wf_webimg_reset(wf_webimg_t* w) {
    int k;
    w->atlas.magic = OS_FB_WEBIMG_MAGIC;
    for (k = 0; k < OS_FB_WEBIMG_SLOTS; k++) {
        w->atlas.slot[k].w = w->atlas.slot[k].h = 0;
        w->atlas.slot[k].off = 0;
        w->url[k][0] = 0;
        w->disp_w[k] = w->disp_h[k] = 0;
    }
    w->used = WF_WEBIMG_FAVICON_PX * WF_WEBIMG_FAVICON_PX;   /* favicon area is reserved */
    w->gen++;
    w->atlas.gen = w->gen;
}

int wf_webimg_dims(void* ctx, int slot, int* width, int* height) {
    wf_webimg_t* w = (wf_webimg_t*)ctx;
    if (!w || slot < 0 || slot >= OS_FB_WEBIMG_SLOTS || !w->atlas.slot[slot].w) return -1;
    *width = w->disp_w[slot] ? w->disp_w[slot] : w->atlas.slot[slot].w;
    *height = w->disp_h[slot] ? w->disp_h[slot] : w->atlas.slot[slot].h;
    return 0;
}

void wf_webimg_box(int aw, int ah, int nw, int nh, int* w, int* h) {
    if (nw < 1) nw = 1;
    if (nh < 1) nh = 1;
    if (aw > 0 && ah > 0) { *w = aw; *h = ah; }
    else if (aw > 0) { *w = aw; *h = (int)((unsigned)aw * nh / nw); }
    else if (ah > 0) { *h = ah; *w = (int)((unsigned)ah * nw / nh); }
    else { *w = nw; *h = nh; }
    if (*w > WF_WEBIMG_BOX_W) { *h = (int)((unsigned)*h * WF_WEBIMG_BOX_W / *w); *w = WF_WEBIMG_BOX_W; }
    if (*h > WF_WEBIMG_BOX_H) { *w = (int)((unsigned)*w * WF_WEBIMG_BOX_H / *h); *h = WF_WEBIMG_BOX_H; }
    if (*w < 1) *w = 1;
    if (*h < 1) *h = 1;
}

/* Area average of the source rectangle that maps to each output pixel
 * (nearest when enlarging). */
static void scale_into(const imgdec_t* img, uint8_t* out, int ow, int oh) {
    int x, y;
    for (y = 0; y < oh; y++) {
        int y0 = (int)((unsigned)y * img->h / oh), y1 = (int)((unsigned)(y + 1) * img->h / oh);
        if (y1 <= y0) y1 = y0 + 1;
        for (x = 0; x < ow; x++) {
            int x0 = (int)((unsigned)x * img->w / ow), x1 = (int)((unsigned)(x + 1) * img->w / ow), sx, sy;
            unsigned r = 0, g = 0, b = 0, cnt = 0;
            uint8_t* o = out + ((unsigned)y * (unsigned)ow + (unsigned)x) * 3U;
            if (x1 <= x0) x1 = x0 + 1;
            /* bounded: at most 16 x 16 samples per output pixel */
            for (sy = y0; sy < y1; sy += (y1 - y0 + 15) / 16)
                for (sx = x0; sx < x1; sx += (x1 - x0 + 15) / 16) {
                    unsigned char c[3];
                    imgdec_pixel(img, sx, sy, c);
                    r += c[0]; g += c[1]; b += c[2]; cnt++;
                }
            o[0] = (uint8_t)(r / cnt); o[1] = (uint8_t)(g / cnt); o[2] = (uint8_t)(b / cnt);
        }
    }
}

/* Fetches and decodes url; 0 and img opened over w->scratch, or < 0 */
static int load(wf_webimg_t* w, const char* url, const wf_webimg_io_t* io, imgdec_t* img, wf_webimg_stats_t* st) {
    int n, rc, shift = 0;
    if (!io || !io->fetch) { err(st, "pas de transport", url); return -1; }
    n = io->fetch(io->ctx, url, w->src, WF_WEBIMG_SRC_MAX);
    if (st) st->fetched++;
    if (n < 0) { err(st, "fetch", url); return -1; }
    if (n > WF_WEBIMG_SRC_MAX) { err(st, "fichier trop gros", url); return -1; }
    rc = imgdec_probe(w->src, n, img);
    if (rc) { err(st, imgdec_strerror(rc), url); return -1; }
    while (imgdec_scratch_need_scaled(img, shift) > WF_WEBIMG_SCRATCH && img->fmt == IMGDEC_JPEG && shift < 3) shift++;
    if (imgdec_scratch_need_scaled(img, shift) > WF_WEBIMG_SCRATCH) { err(st, "image trop grande", url); return -1; }
    rc = imgdec_open_scaled(w->src, n, w->scratch, WF_WEBIMG_SCRATCH, shift, img);
    if (rc) { err(st, imgdec_strerror(rc), url); return -1; }
    img->over_white = 1;                       /* transparent PNG over the white page */
    if (shift && st) st->scaled++;
    return 0;
}

/* Allocates ow x oh pixels (shrunk by halves while the atlas is short,
 * down to 8 px); returns the offset or -1. */
static int alloc_px(wf_webimg_t* w, int* ow, int* oh) {
    for (;;) {
        uint32_t px = (uint32_t)*ow * (uint32_t)*oh;
        if (w->used + px <= OS_FB_WEBIMG_PIXELS) {
            uint32_t off = w->used;
            w->used += px;
            return (int)off;
        }
        if (*ow <= 8 || *oh <= 8) return -1;
        *ow = (*ow + 1) / 2;
        *oh = (*oh + 1) / 2;
    }
}

/* Keeps only the slots whose URL is in keep[] (and the favicon), packing
 * their pixels at the start of the atlas. */
static void compact(wf_webimg_t* w, const uint8_t* keep) {
    uint32_t at = WF_WEBIMG_FAVICON_PX * WF_WEBIMG_FAVICON_PX;
    int k, done[OS_FB_WEBIMG_SLOTS];
    for (k = 1; k < OS_FB_WEBIMG_SLOTS; k++) {
        done[k] = 0;
        if (!keep[k]) { w->atlas.slot[k].w = w->atlas.slot[k].h = 0; w->url[k][0] = 0; done[k] = 1; }
        else if (!w->atlas.slot[k].w) done[k] = 1;
    }
    for (;;) {   /* lowest offset first, so a move never overwrites a kept slot */
        int best = -1;
        uint32_t i, n;
        for (k = 1; k < OS_FB_WEBIMG_SLOTS; k++)
            if (!done[k] && (best < 0 || w->atlas.slot[k].off < w->atlas.slot[best].off)) best = k;
        if (best < 0) break;
        n = (uint32_t)w->atlas.slot[best].w * w->atlas.slot[best].h * 3U;
        if (w->atlas.slot[best].off != at)
            for (i = 0; i < n; i++) w->atlas.rgb[at * 3U + i] = w->atlas.rgb[w->atlas.slot[best].off * 3U + i];
        w->atlas.slot[best].off = at;
        at += (uint32_t)w->atlas.slot[best].w * w->atlas.slot[best].h;
        done[best] = 1;
    }
    w->used = at;
}

static int find_slot(const wf_webimg_t* w, const char* url) {
    int k;
    for (k = 1; k < OS_FB_WEBIMG_SLOTS; k++) if (w->atlas.slot[k].w && s_eq(w->url[k], url)) return k;
    return -1;
}

/* value of attr in tag[s..e) into out; -1 when absent */
static int tag_attr(const char* in, int s, int e, const char* attr, char* out, int cap) {
    int i, al = s_len(attr), k;
    for (i = s; i + al < e; i++) {
        if (!is_sp(in[i - 1])) continue;
        for (k = 0; k < al && lc(in[i + k]) == attr[k]; k++) {}
        if (k == al && in[i + al] == '=') {
            int j = i + al + 1, o = 0, q = 0;
            if (j < e && (in[j] == '"' || in[j] == '\'')) q = in[j++];
            while (j < e && o < cap - 1 && (q ? in[j] != q : !is_sp(in[j]) && in[j] != '>')) out[o++] = in[j++];
            out[o] = 0;
            return o;
        }
    }
    out[0] = 0;
    return -1;
}

static int num(const char* s) {
    int v = 0, k;
    for (k = 0; s[k] >= '0' && s[k] <= '9' && k < 5; k++) v = v * 10 + (s[k] - '0');
    return v;
}

typedef struct { char* o; int cap, n, full; } ob_t;
static void put(ob_t* b, const char* s, int n) {
    int i;
    for (i = 0; i < n; i++) { if (b->n < b->cap - 1) b->o[b->n++] = s[i]; else b->full = 1; }
}
static void puts_(ob_t* b, const char* s) { put(b, s, s_len(s)); }
static void putn(ob_t* b, int v) {
    char t[8];
    int k = 0;
    do { t[k++] = (char)('0' + v % 10); v /= 10; } while (v && k < 7);
    while (k) { char c = t[--k]; put(b, &c, 1); }
}

/* end of the tag that starts at in[i] ('<'), quotes respected */
static int tag_end(const char* in, int n, int i) {
    int q = 0;
    while (i < n && (q || in[i] != '>')) {
        if (in[i] == '"' || in[i] == '\'') q = (q == in[i]) ? 0 : (q ? q : in[i]);
        i++;
    }
    return i;
}

static int is_img(const char* in, int n, int i) {
    return i + 4 < n && in[i] == '<' && lc(in[i + 1]) == 'i' && lc(in[i + 2]) == 'm' && lc(in[i + 3]) == 'g' && is_sp(in[i + 4]);
}

int wf_webimg_section(wf_webimg_t* w, const char* in, int n, char* out, int cap, const wf_webimg_io_t* io,
                      wf_webimg_stats_t* st) {
    static char url[WF_WEBIMG_URL], alt[96], v[16];
    static imgdec_t img;
    uint8_t keep[OS_FB_WEBIMG_SLOTS];
    wf_webimg_stats_t dummy;
    ob_t b;
    int i, k, fetches = 0;
    if (!st) st = &dummy;
    for (k = 0; k < (int)sizeof(*st); k++) ((char*)st)[k] = 0;
    if (w->atlas.magic != OS_FB_WEBIMG_MAGIC) wf_webimg_reset(w);
    if (!out || cap < 2) return -1;
    /* slots this section uses again stay; the others are dropped when
     * the atlas is past half full */
    for (k = 0; k < OS_FB_WEBIMG_SLOTS; k++) keep[k] = 0;
    for (i = 0; i < n; i++) {
        if (!is_img(in, n, i)) continue;
        {
            int e = tag_end(in, n, i), s;
            if (tag_attr(in, i + 4, e, "src", url, (int)sizeof(url)) > 0 && (s = find_slot(w, url)) > 0) keep[s] = 1;
            i = e;
        }
    }
    if (w->used > OS_FB_WEBIMG_PIXELS / 2) { compact(w, keep); w->gen++; }
    b.o = out; b.cap = cap; b.n = 0; b.full = 0;
    i = 0;
    while (i < n) {
        int e, slot, aw, ah, bw, bh;
        if (!is_img(in, n, i)) { put(&b, in + i, 1); i++; continue; }
        e = tag_end(in, n, i);
        st->found++;
        aw = tag_attr(in, i + 4, e, "width", v, (int)sizeof(v)) > 0 ? num(v) : 0;
        ah = tag_attr(in, i + 4, e, "height", v, (int)sizeof(v)) > 0 ? num(v) : 0;
        tag_attr(in, i + 4, e, "alt", alt, (int)sizeof(alt));
        slot = -1;
        if (tag_attr(in, i + 4, e, "src", url, (int)sizeof(url)) > 0) {
            slot = find_slot(w, url);
            if (slot > 0) st->reused++;
            else if (fetches >= WF_WEBIMG_FETCH_MAX) { st->skipped++; slot = -1; }
            else {
                int free_slot = -1, off, ow, oh;
                for (k = 1; k < OS_FB_WEBIMG_SLOTS; k++) if (!w->atlas.slot[k].w) { free_slot = k; break; }
                if (free_slot < 0) { st->skipped++; err(st, "plus de place (images)", url); }
                else {
                    fetches++;
                    if (load(w, url, io, &img, st) != 0) st->failed++;
                    else {
                        wf_webimg_box(aw, ah, img.full_w ? img.full_w : img.w, img.full_h ? img.full_h : img.h, &bw, &bh);
                        ow = bw; oh = bh;
                        off = alloc_px(w, &ow, &oh);
                        if (off < 0) { st->skipped++; err(st, "plus de place (pixels)", url); }
                        else {
                            scale_into(&img, w->atlas.rgb + (unsigned)off * 3U, ow, oh);
                            w->atlas.slot[free_slot].w = (uint16_t)ow;
                            w->atlas.slot[free_slot].h = (uint16_t)oh;
                            w->atlas.slot[free_slot].off = (uint32_t)off;
                            w->disp_w[free_slot] = (uint16_t)bw;
                            w->disp_h[free_slot] = (uint16_t)bh;
                            s_cpy(w->url[free_slot], url, WF_WEBIMG_URL);
                            w->gen++;
                            slot = free_slot;
                        }
                    }
                }
            }
        }
        if (slot > 0) {
            bw = w->disp_w[slot]; bh = w->disp_h[slot];
            if (aw > 0 || ah > 0) wf_webimg_box(aw, ah, bw, bh, &bw, &bh);
            puts_(&b, "<img src=\"" WE_ATLAS_SCHEME);
            putn(&b, slot);
            /* the engine sizes images from CSS (no presentational
             * width / height attributes), so both are given */
            puts_(&b, "\" width=\""); putn(&b, bw);
            puts_(&b, "\" height=\""); putn(&b, bh);
            puts_(&b, "\" style=\"width:"); putn(&b, bw);
            puts_(&b, "px;height:"); putn(&b, bh);
            puts_(&b, "px\">");
            st->shown++;
        } else if (alt[0]) {
            puts_(&b, "[image: ");
            puts_(&b, alt);
            puts_(&b, "]");
        }
        i = e < n ? e + 1 : n;
    }
    out[b.n] = 0;
    w->atlas.gen = w->gen;
    return b.full ? -1 : b.n;
}

int wf_webimg_favicon(wf_webimg_t* w, const char* page_url, const char* icon_url, const wf_webimg_io_t* io,
                      wf_webimg_stats_t* st) {
    static char url[WF_WEBIMG_URL];
    static imgdec_t img;
    if (w->atlas.magic != OS_FB_WEBIMG_MAGIC) wf_webimg_reset(w);
    w->atlas.slot[OS_FB_WEBIMG_FAVICON].w = w->atlas.slot[OS_FB_WEBIMG_FAVICON].h = 0;
    w->url[OS_FB_WEBIMG_FAVICON][0] = 0;
    w->gen++;
    w->atlas.gen = w->gen;
    if (icon_url && icon_url[0]) s_cpy(url, icon_url, (int)sizeof(url));
    else {
        /* scheme://host[:port]/favicon.ico */
        int i = 0, slashes = 0, k = 0;
        if (!page_url) return -1;
        while (page_url[i] && k < (int)sizeof(url) - 13) {
            if (page_url[i] == '/' && ++slashes == 3) break;
            url[k++] = page_url[i++];
        }
        if (slashes < 2) return -1;
        s_cpy(url + k, "/favicon.ico", (int)sizeof(url) - k);
    }
    if (load(w, url, io, &img, st) != 0) return -1;
    w->atlas.slot[OS_FB_WEBIMG_FAVICON].off = 0;
    scale_into(&img, w->atlas.rgb, WF_WEBIMG_FAVICON_PX, WF_WEBIMG_FAVICON_PX);
    w->atlas.slot[OS_FB_WEBIMG_FAVICON].w = WF_WEBIMG_FAVICON_PX;
    w->atlas.slot[OS_FB_WEBIMG_FAVICON].h = WF_WEBIMG_FAVICON_PX;
    w->disp_w[0] = w->disp_h[0] = WF_WEBIMG_FAVICON_PX;
    s_cpy(w->url[OS_FB_WEBIMG_FAVICON], url, WF_WEBIMG_URL);
    w->gen++;
    w->atlas.gen = w->gen;
    return 0;
}
