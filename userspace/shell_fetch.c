/* shell_fetch.c - Lot 67: the `fetch` / `http-get` shell commands and the
 * /browser network loader (see shell_fetch.h). Real network: DNS over TCP,
 * TCP through the networker, TLS 1.2 with certificate checks against the
 * root store in /etc/ca-roots.pem. Offline pages in /web stay local files. */
#include "shell_fetch.h"
#include "shell_desk.h"
#include "wf_blocks.h"
#include "wf_feed.h"
#include "wf_guest.h"
#include "wf_html.h"
#include "wf_entity.h"
#include "wf_form.h"
#include "wf_article.h"
#include "wf_section.h"
#include "shell_search.h"
#include "wf_webimg.h"
#include "wsearch.h"
#include "wsearch_api.h"
#include "wf_searchform.h"
#include "wf_formfill.h"
#include "fsops.h"
#include "wf_csslink.h"
#include "wf_compat.h"
#include "wf_sitecss.h"
#include "webeng.h"   /* Lot 104: we_geom_feed */
#include "wf_jsweb.h"  /* Lot 113: the page script pass */
#include "wf_jsdom.h"  /* Lot 114: the live page, its fetch hook */
#include "wf_jslayout.h"  /* Lot 115: layout values from the engine */

extern const fsops_io_t shell_fs_io;
#include "../include/os_syscalls.h"

void print_string(const char* s);

/* Lot 85: 384 KiB, still one static buffer (no heap). A longer article
 * fits; the reader then keeps the main text inside the node budget. */
static uint8_t g_body[384 * 1024];
static wf_fetch_result_t g_res;
static char g_text[8192];
/* Lot 71: the whole reduced page of the GUI (shown by sections) and its
 * forms. Bounded: 192 KiB of reduced HTML, WF_SECTION_MAX sections. */
#define PAGE_FULL_MAX (256 * 1024)
#define PAGE_NODE_BUDGET 6000
static char g_full[PAGE_FULL_MAX];
static int g_full_n;
static wf_sections_t g_sec;
static wf_form_page_t g_forms;
static int g_cur;
static int g_page_ok;
static char g_blocks[2048];
static int g_blocks_n;

/* Lot 72: images of the page shown in the GUI (atlas + decode buffers,
 * about 1.7 MiB, see wf_webimg.h) */
static wf_webimg_t g_wi;
static char g_img_tmp[16384];
static wf_webimg_stats_t g_img_st;
static int g_img_on = 1;

static int img_fetch(void* ctx, const char* url, uint8_t* buf, int cap) {
    static wf_fetch_result_t r;
    (void)ctx;
    if (wf_guest_fetch_ex(url, 0, buf, cap, &r, 0) != 0) return -1;
    if (r.status != 200) return -1;
    if (r.truncated) return cap + 1;
    return r.body_len;
}
static const wf_webimg_io_t g_img_io = {0, img_fetch};

const os_fb_webimg_t* shell_page_atlas(void) { return g_wi.atlas.magic ? &g_wi.atlas : 0; }
int shell_page_atlas_dims(void* ctx, int slot, int* w, int* h) { (void)ctx; return wf_webimg_dims(&g_wi, slot, w, h); }
void shell_page_images(int on) { g_img_on = on ? 1 : 0; }
int shell_page_images_on(void) { return g_img_on; }
const wf_webimg_stats_t* shell_page_image_stats(void) { return &g_img_st; }
static unsigned anim_ticks(void) { int t; asm volatile("int $0x80" : "=a"(t) : "a"(SYS_TICKS) : "memory"); return (unsigned)t; }
int shell_page_anim_tick(void) { return wf_webimg_tick(&g_wi, anim_ticks()); }
unsigned shell_page_anim_changes(void) { return g_wi.anim_changes; }
int shell_page_anim_text(char* out, int cap) { return wf_webimg_anim_text(&g_wi, out, cap); }

/* Section src (n bytes) with its images fetched and rewritten in place. */
static int with_images(char* src, int n, int src_max) {
    int m, k;
    if (!g_img_on || n <= 0 || n >= (int)sizeof(g_img_tmp)) return n;
    for (k = 0; k < n; k++) g_img_tmp[k] = src[k];
    m = wf_webimg_section(&g_wi, g_img_tmp, n, src, src_max, &g_img_io, &g_img_st);
    if (m < 0) {   /* did not fit: the section without its images */
        for (k = 0; k < n; k++) src[k] = g_img_tmp[k];
        src[n] = 0;
        return n;
    }
    return m;
}

static int s_pre(const char* s, const char* p) { while (*p && *s == *p) { s++; p++; } return *p == 0; }
static int s_eq(const char* a, const char* b) { while (*a && *a == *b) { a++; b++; } return *a == *b; }
static void scat(char* d, int max, const char* s) {
    int n = 0;
    while (d[n]) n++;
    while (*s && n < max - 1) d[n++] = *s++;
    d[n] = 0;
}
static void scat_int(char* d, int max, int v) {
    char t[12];
    int k = 0;
    if (v < 0) { scat(d, max, "-"); v = -v; }
    do { t[k++] = (char)('0' + v % 10); v /= 10; } while (v && k < 11);
    while (k) { char c[2]; c[0] = t[--k]; c[1] = 0; scat(d, max, c); }
}

static int ctlen(const char* s) { int n = 0; while (s[n]) n++; return n; }
static int g_verbose;
static void log_line(const char* l) {
    if (!g_verbose) return;
    print_string("fetch: ");
    print_string(l);
    print_string("\n");
}

static int is_html(const wf_fetch_result_t* r, const uint8_t* b, int n) {
    const char* c = r->content_type;
    int i;
    for (i = 0; c[i]; i++)
        if ((c[i] == 'h' || c[i] == 'H') && s_pre(c + i + 1, "tml")) return 1;
    for (i = 0; i + 5 < n && i < 512; i++)
        if (b[i] == '<' && (b[i + 1] == 'h' || b[i + 1] == 'H' || b[i + 1] == '!')) return 1;
    return 0;
}

/* One summary line for the console and the GUI status. */
static void summary(char* o, int max, const wf_fetch_result_t* r) {
    int tc = 0, roots = wf_guest_roots(&tc);
    o[0] = 0;
    scat(o, max, "status "); scat_int(o, max, r->status);
    scat(o, max, " bytes "); scat_int(o, max, r->body_len);
    if (r->truncated) scat(o, max, " truncated 1");
    if (r->encoding) { scat(o, max, r->encoding == 1 ? " gzip " : " deflate "); scat_int(o, max, r->wire_len); }
    scat(o, max, " redirects "); scat_int(o, max, r->redirects);
    scat(o, max, " url "); scat(o, max, r->url);
    if (r->https) {
        scat(o, max, " tls "); scat(o, max, r->suite);
        scat(o, max, " leaf "); scat(o, max, r->leaf_cn);
        scat(o, max, " root \""); scat(o, max, r->anchor_cn); scat(o, max, "\"");
        scat(o, max, r->time_checked ? " dates checked" : " dates NOT checked (guest clock not trusted)");
        scat(o, max, " roots "); scat_int(o, max, roots);
        if (tc) scat(o, max, " test-ca 1");
    } else {
        scat(o, max, " tls none (plain http)");
    }
    scat(o, max, " entropy "); scat(o, max, wf_guest_entropy());
}

static const char* word(const char* s, char* out, int cap) {
    int n = 0;
    while (*s == ' ') s++;
    while (*s && *s != ' ' && n < cap - 1) out[n++] = *s++;
    out[n] = 0;
    return s;
}

int shell_fetch_is(const char* line) {
    return (s_pre(line, "fetch") && (line[5] == ' ' || !line[5])) ||
           (s_pre(line, "http-get") && (line[8] == ' ' || !line[8]));
}

int shell_fetch_line(const char* line) {
    static char tok[1100], url[1100], line_out[600];
    const char* p;
    int raw = 0, headers_only = 0, max_show = 1500, rc, i;
    p = word(line, tok, (int)sizeof(tok));   /* command name */
    url[0] = 0;
    g_verbose = 0;
    for (;;) {
        p = word(p, tok, (int)sizeof(tok));
        if (!tok[0]) break;
        if (s_eq(tok, "-v")) g_verbose = 1;
        else if (s_eq(tok, "-raw")) raw = 1;
        else if (s_eq(tok, "-I")) headers_only = 1;
        else if (s_eq(tok, "-n")) {
            p = word(p, tok, (int)sizeof(tok));
            max_show = 0;
            for (i = 0; tok[i] >= '0' && tok[i] <= '9'; i++) max_show = max_show * 10 + (tok[i] - '0');
            if (max_show > (int)sizeof(g_text) - 1) max_show = (int)sizeof(g_text) - 1;
        } else {
            for (i = 0; tok[i] && i < (int)sizeof(url) - 1; i++) url[i] = tok[i];
            url[i] = 0;
        }
    }
    if (!url[0]) {
        print_string("fetch error usage: fetch [-v] [-I] [-raw] [-n BYTES] URL   (http:// or https://, redirects <= 5)\n");
        return 1;
    }
    rc = wf_guest_fetch(url, g_body, (int)sizeof(g_body) - 1, &g_res, log_line);
    if (rc != 0) {
        print_string("fetch error ");
        print_string(g_res.err[0] ? g_res.err : "failed");
        print_string(" url ");
        print_string(g_res.url[0] ? g_res.url : url);
        print_string("\n");
        return 2;
    }
    summary(line_out, (int)sizeof(line_out), &g_res);
    print_string("fetch ok ");
    print_string(line_out);
    print_string("\n");
    if (g_res.content_type[0]) { print_string("fetch content-type "); print_string(g_res.content_type); print_string("\n"); }
    /* Lot 73: answered from the HTTP cache */
    if (g_res.cached) { print_string(g_res.cached == 1 ? "fetch cache fresh\n" : "fetch cache revalidated\n"); }
    if (headers_only) return 0;
    g_body[g_res.body_len] = 0;
    if (!raw && is_html(&g_res, g_body, g_res.body_len)) {
        wf_html_stats_t st;
        wf_html_set_charset(wf_charset_header(g_res.content_type, ctlen(g_res.content_type)));
        static char reduced[16384];
        (void)wf_html_reduce((const char*)g_body, g_res.body_len, 0, reduced, (int)sizeof(reduced), &st);
        if (st.title[0]) {
            /* Lot 72: the title is display bytes (glyph pages); the console
             * prints UTF-8, and names a page charset other than UTF-8 */
            static char tu[sizeof(st.title) * 3 + 1];
            wf_display_to_utf8(st.title, (int)sizeof(st.title), tu, (int)sizeof(tu));
            print_string("fetch title "); print_string(tu); print_string("\n");
        }
        if (st.charset != WF_CS_UTF8) { print_string("fetch charset "); print_string(wf_charset_name(st.charset)); print_string("\n"); }
        (void)wf_html_text_display(reduced, st.out_bytes, g_text, max_show + 1 < (int)sizeof(g_text) ? max_show + 1 : (int)sizeof(g_text));
    } else {
        int n = g_res.body_len < max_show ? g_res.body_len : max_show;
        for (i = 0; i < n; i++) {
            char c = (char)g_body[i];
            g_text[i] = (c == '\n' || c == '\t' || ((unsigned char)c >= 0x20U && (unsigned char)c < 0x7fU)) ? c : '.';
        }
        g_text[n] = 0;
    }
    if (!shell_desk_nav_held()) shell_desk_note(g_res.url[0] ? g_res.url : url, 0);
    print_string("fetch body-begin\n");
    print_string(g_text);
    print_string("\nfetch body-end\n");
    return 0;
}

/* Lot 86: the reader applies <style> only when this is on, or for web-load. */
static int g_css_on;
/* Lot 87: external stylesheets of the presented page */
static char g_extcss[WF_SITE_CSS];
static int g_extcss_n;    /* Lot 104: its length, for the geometry pass */
static wf_cssl_stats_t g_css_st;
/* Lot 104: the layout geometry handed back to the reader (shell_page_geom)
 * and 1 while the presented page waits for it */
static wf_geom_t g_geom;
static int g_geom_pending;
/* Lot 105: 1 while the presented page has timelines that read a real
 * scroll offset (box scrollers, the GUI root position): it is styled
 * again from the layout when the GUI scrolls */
static int g_geom_live;
static wf_url_t g_geom_base;
static int g_css_ran;
static char g_load_dir[96];
/* Lot 87: script wall / block page reading of the presented page */
static wf_compat_t g_compat;

/* wf_csslink callback: web-load reads a linked sheet from the directory of
 * the saved page (same file name); a fetched page gets it from the network
 * through the same guest fetch path (settings net off refuses it). */
static int sheet_get(void *ctx, const char *url, char *buf, int cap) {
    static wf_fetch_result_t sres;
    int i;
    (void)ctx;
    if (g_load_dir[0]) {
        static char p[160];
        int q = 0, last = -1, k = 0, n;
        for (i = 0; url[i] && url[i] != '?' && url[i] != '#'; i++) if (url[i] == '/') last = i;
        if (last < 0) return -1;
        for (i = 0; g_load_dir[i] && k < (int)sizeof(p) - 2; i++) p[k++] = g_load_dir[i];
        p[k++] = '/';
        for (q = last + 1; url[q] && url[q] != '?' && url[q] != '#' && k < (int)sizeof(p) - 1; q++) {
            if (url[q] == '/' || (url[q] == '.' && url[q + 1] == '.')) return -1;
            p[k++] = url[q];
        }
        p[k] = 0;
        n = fsops_read(&shell_fs_io, p, buf, cap);
        return n;
    }
    for (i = 0; i < (int)sizeof(sres); i++) ((char *)&sres)[i] = 0;
    if (wf_guest_fetch_ex(url, 0, (uint8_t *)buf, cap, &sres, 0) != 0) return -1;
    if (sres.status != 200) return -1;
    return sres.body_len;
}

static int page_present(const wf_fetch_req_t* req, int use_css, char* src, int src_max, char* final_url, int url_max,
                        char* status, int st_max);

/* Lot 71: GET or a form request (POST body) for the GUI. The reduced page
 * keeps its forms; section 0 is written into src. */
static int page_fetch(const char* url, const wf_fetch_req_t* req, char* src, int src_max, char* final_url, int url_max,
                      char* status, int st_max) {
    g_verbose = 0;
    if (status && st_max > 0) status[0] = 0;
    /* Lot 69: "search:QUERY" is the result page of a search */
    if (!req && s_pre(url, "search:")) {
        g_page_ok = 0;
        return shell_search_page(url + 7, src, src_max, final_url, url_max, status, st_max);
    }
    if (wf_guest_fetch_ex(url, req, g_body, (int)sizeof(g_body) - 1, &g_res, 0) != 0) {
        if (status) {
            scat(status, st_max, "osui web fetch error=");
            scat(status, st_max, g_res.err[0] ? g_res.err : "failed");
            scat(status, st_max, " url=");
            scat(status, st_max, url);
        }
        return -1;
    }
    return page_present(req, g_css_on, src, src_max, final_url, url_max, status, st_max);
}

static int g_js_replay;     /* Lot 114: the page written again by an event: no new script pass */

/* Lot 114 (US-031): the network of the page pass. External scripts,
 * fetch() and XMLHttpRequest go through the same guest fetch path as the
 * page (settings net off refuses them); a page opened with web-load
 * reads its scripts from the same directory, as its sheets (sheet_get). */
static int js_fetch_hook(void *ctx, wf_jsreq_t *r) {
    static wf_fetch_result_t jres;
    wf_fetch_req_t q;
    int i;
    (void)ctx;
    if (g_load_dir[0]) {
        if (r->method && r->method[0] == 'P') return -1;
        r->len = sheet_get(0, r->url, r->out, r->out_max);
        if (r->len < 0) return -1;
        r->status = 200;
        return 0;
    }
    for (i = 0; i < (int)sizeof(jres); i++) ((char *)&jres)[i] = 0;
    q.method = r->method;
    q.body = r->body;
    q.body_len = r->body ? r->body_len : 0;
    q.ctype = r->ctype;
    q.extra = 0;
    if (wf_guest_fetch_ex(r->url, &q, (uint8_t *)r->out, r->out_max, &jres, 0) != 0) return -1;
    r->status = jres.status;
    r->len = jres.body_len;
    r->truncated = jres.truncated;
    for (i = 0; jres.content_type[i] && i < (int)sizeof(r->ctype_out) - 1; i++) r->ctype_out[i] = jres.content_type[i];
    r->ctype_out[i] = 0;
    for (i = 0; jres.url[i] && i < (int)sizeof(r->final_url) - 1; i++) r->final_url[i] = jres.url[i];
    r->final_url[i] = 0;
    return 0;
}

static int page_present(const wf_fetch_req_t* req, int use_css, char* src, int src_max, char* final_url, int url_max,
                        char* status, int st_max) {
    static char line_out[600];
    static wf_url_t base;
    wf_html_stats_t st;
    int i, n;
    for (i = 0; i < url_max - 1 && g_res.url[i]; i++) final_url[i] = g_res.url[i];
    final_url[i] = 0;
    g_body[g_res.body_len] = 0;
    g_page_ok = 0;
    g_blocks_n = 0;
    /* Lot 113 (US-031): with web-js on, the inline scripts of an HTML page
     * run against its DOM first; everything below (blocks, reducer, the
     * sanitizer of the GUI) sees the page as the scripts left it */
    if (!g_js_replay) wf_jspage_close();     /* Lot 114: a new page ends the live one */
    if (!g_js_replay && wf_jsweb_enabled() && is_html(&g_res, g_body, g_res.body_len)) {
        wf_jspage_set_fetch(js_fetch_hook, 0);
        wf_jspage_set_layout(wf_jslayout_engine, 0);    /* Lot 115: the renderer's boxes */
        g_res.body_len = wf_jsweb_page((char *)g_body, g_res.body_len, (int)sizeof(g_body), g_res.url);
        g_body[g_res.body_len] = 0;
    }
    if (wf_feed_looks((const char *)g_body, g_res.body_len)) {
        static wf_feed_t feed;
        if (wf_feed_parse((const char *)g_body, g_res.body_len, &feed) == 0)
            g_blocks_n = wf_feed_text(&feed, g_blocks, (int)sizeof(g_blocks));
    }
    if (g_blocks_n <= 0)
        g_blocks_n = wf_blocks_text((const char *)g_body, g_res.body_len, g_blocks, (int)sizeof(g_blocks), 0);
    if (wf_url_parse(g_res.url, &base) != 0) return -1;
    /* Lot 104: a new page starts without geometry */
    g_geom_pending = 0;
    wf_html_set_cssgeom(0);
    if (is_html(&g_res, g_body, g_res.body_len)) {
        wf_html_set_charset(wf_charset_header(g_res.content_type, ctlen(g_res.content_type)));
        wf_html_set_images(g_img_on);
        wf_html_set_sitecss(use_css);
        g_css_ran = 0;
        if (use_css) {
            int en = wf_csslink_collect((const char *)g_body, g_res.body_len, g_res.url, 640, 480, sheet_get, 0,
                                        g_extcss, (int)sizeof(g_extcss), &g_css_st);
            wf_html_set_extcss(en > 0 ? g_extcss : 0, en > 0 ? en : 0);
            g_extcss_n = en > 0 ? en : 0;
            g_css_ran = 1;
            /* the sheets use their own buffers; keep the body terminated */
            g_body[g_res.body_len] = 0;
        }
        wf_html_set_geommarks(use_css);   /* Lot 104 */
        g_full_n = wf_html_reduce_ex((const char*)g_body, g_res.body_len, &base, g_full, (int)sizeof(g_full), &st,
                                     PAGE_NODE_BUDGET, &g_forms);
        wf_html_set_geommarks(0);
        /* Lot 104: view timelines wait for the layout (shell_page_geom) */
        g_geom_pending = use_css && wf_html_sitecss_geom_need() > 0;
        /* Lot 105: set before g_page_ok (cleared above) is, so a page
         * that is not presented is never styled again */
        g_geom_live = g_geom_pending && wf_html_sitecss_geom_live();
        g_geom_base = base;
        wf_html_set_sitecss(0);
        wf_html_set_extcss(0, 0);
        wf_compat_scan_ex((const char *)g_body, g_res.body_len, g_res.status, &g_compat);
        if (g_full_n > 0 && wf_section_index(g_full, g_full_n, &g_sec) > 0) {
            g_page_ok = 1;
            g_cur = 0;
            if (!shell_desk_nav_held()) shell_desk_note(final_url[0] ? final_url : g_res.url, st.title);
            n = wf_section_render(g_full, g_full_n, &g_sec, 0, &g_forms, src, src_max);
            /* Lot 72: favicon and the images of section 0 */
            if (g_img_on) {
                static char page_url[sizeof(g_res.url)];
                for (i = 0; g_res.url[i] && i < (int)sizeof(page_url) - 1; i++) page_url[i] = g_res.url[i];
                page_url[i] = 0;
                wf_webimg_reset(&g_wi);
                for (i = 0; i < (int)sizeof(g_img_st); i++) ((char*)&g_img_st)[i] = 0;
                (void)wf_webimg_favicon(&g_wi, page_url, st.icon[0] ? st.icon : 0, &g_img_io, &g_img_st);
                if (n > 0) n = with_images(src, n, src_max);
            }
        } else {
            wf_html_set_sitecss(use_css);
            n = wf_html_reduce((const char*)g_body, g_res.body_len, &base, src, src_max, &st);
            wf_html_set_sitecss(0);
        }
    } else {
        for (i = 0; i < (int)sizeof(g_compat); i++) ((char *)&g_compat)[i] = 0;
        /* text/plain and others: shown escaped in a <pre> */
        int k = 0, lim = g_res.body_len;
        const char* head = "<html><head><title>texte</title></head><body><pre>";
        const char* tail = "</pre></body></html>";
        for (i = 0; head[i] && k < src_max - 32; i++) src[k++] = head[i];
        for (i = 0; i < lim && k < src_max - 32; i++) {
            char c = (char)g_body[i];
            if (c == '<') c = '[';
            else if (c == '>') c = ']';
            else if ((unsigned char)c < 0x20U && c != '\n') c = ' ';
            src[k++] = c;
        }
        for (i = 0; tail[i] && k < src_max - 1; i++) src[k++] = tail[i];
        src[k] = 0;
        n = k;
        st.out_bytes = n;
    }
    if (status) {
        summary(line_out, (int)sizeof(line_out), &g_res);
        scat(status, st_max, "osui web fetch ok ");
        scat(status, st_max, line_out);
        scat(status, st_max, " reduced ");
        scat_int(status, st_max, g_page_ok ? g_full_n : n);
        if (req && req->method) { scat(status, st_max, " method "); scat(status, st_max, req->method); }
        wf_jsweb_note(status, st_max);   /* Lot 113 */
        if (g_page_ok) {
            scat(status, st_max, " sections "); scat_int(status, st_max, g_sec.n);
            scat(status, st_max, " forms "); scat_int(status, st_max, g_forms.nforms);
            scat(status, st_max, " fields "); scat_int(status, st_max, g_forms.nfields);
            if (g_img_on) {
                scat(status, st_max, " images "); scat_int(status, st_max, g_img_st.shown);
                scat(status, st_max, "/"); scat_int(status, st_max, g_img_st.found);
                scat(status, st_max, " favicon "); scat(status, st_max, g_wi.atlas.slot[0].w ? "ok" : "none");
            }
        }
        /* Lot 87: a script wall or block page is said, never shown as a working page */
        if (g_compat.wall_kind) {
            char adv[300];
            scat(status, st_max, " wall ");
            scat(status, st_max, wf_compat_wall_name(g_compat.wall_kind));
            if (wf_compat_advice(&g_compat, adv, (int)sizeof(adv)) > 0) {
                scat(status, st_max, "\n");
                scat(status, st_max, adv);
            }
        }
    }
    return n;
}

int shell_fetch_page(const char* url, char* src, int src_max, char* final_url, int url_max, char* status, int st_max) {
    return page_fetch(url, 0, src, src_max, final_url, url_max, status, st_max);
}

int shell_page_sections(void) { return g_page_ok ? g_sec.n : 0; }

/* Lot 104: after the first layout of the presented page (doc, a
 * we_doc_t), the boxes of its view timeline subjects (id="wgN") go back
 * to the reader, which reduces the page again; section 0 of it (with its
 * images, already fetched) in src. The marks and the layout inputs are the
 * same (the reader passes colours only for them), so the layout is too.
 * Length, 0 when the page did not wait for its geometry. */
/* Lot 105: a byte copy of the reader form state (no memcpy here) */
static void fcopy(wf_form_page_t *dst, const wf_form_page_t *src) {
    int k;
    for (k = 0; k < (int)sizeof(*dst); k++) ((char *)dst)[k] = ((const char *)src)[k];
}

/* Lot 105: also after the first pass while the page is live (its
 * timelines read real scroll offsets) and section 0 is shown: the GUI
 * scrolled (the root, or a box), so the layout's scroll offsets go back
 * to the reader; the reader form state the user gave is kept. */
int shell_page_geom(const void *doc, char *src, int src_max) {
    static wf_form_page_t keep;
    wf_html_stats_t st;
    int n, again;
    if (!doc || !src || src_max <= 0) return 0;
    again = !g_geom_pending;
    if (again && !(g_geom_live && g_page_ok && g_cur == 0)) return 0;
    g_geom_pending = 0;
    if (again) fcopy(&keep, &g_forms);
    (void)we_geom_feed((const we_doc_t *)doc, "wg", &g_geom);
    wf_html_set_cssgeom(&g_geom);
    wf_html_set_sitecss(1);
    wf_html_set_extcss(g_extcss_n > 0 ? g_extcss : 0, g_extcss_n);
    wf_html_set_geommarks(1);
    g_body[g_res.body_len] = 0;
    g_full_n = wf_html_reduce_ex((const char*)g_body, g_res.body_len, &g_geom_base, g_full, (int)sizeof(g_full), &st,
                                 PAGE_NODE_BUDGET, &g_forms);
    wf_html_set_geommarks(0);
    wf_html_set_sitecss(0);
    wf_html_set_extcss(0, 0);
    if (again) fcopy(&g_forms, &keep);
    if (g_full_n <= 0 || wf_section_index(g_full, g_full_n, &g_sec) <= 0) { g_page_ok = 0; return 0; }
    g_cur = 0;
    n = wf_section_render(g_full, g_full_n, &g_sec, 0, &g_forms, src, src_max);
    if (g_img_on && n > 0) {
        /* the counters stay those of the first pass */
        wf_webimg_stats_t keep = g_img_st;
        n = with_images(src, n, src_max);
        g_img_st = keep;
    }
    return n > 0 ? n : 0;
}
/* Lot 71: "web-find": section of the first match at or after from */
int shell_page_find(const char* text, int from, int* count) {
    if (count) *count = 0;
    if (!g_page_ok) return -1;
    return wf_section_find(g_full, g_full_n, &g_sec, text, from, count);
}
int shell_page_current(void) { return g_page_ok ? g_cur : 0; }
int shell_page_section(int k, char* src, int src_max) {
    if (!g_page_ok || k < 0 || k >= g_sec.n) return -1;
    g_cur = k;
    {
        int n = wf_section_render(g_full, g_full_n, &g_sec, k, &g_forms, src, src_max);
        return n > 0 ? with_images(src, n, src_max) : n;   /* Lot 72 */
    }
}

/* "mohhdy-form:f:N" focuses field N (0), "mohhdy-form:s:N" asks to submit
 * with button N (1), other links -1. */
int shell_page_form_link(const char* href) {
    int n, t;
    if (!g_page_ok) return -1;
    t = wf_form_href(href, &n);
    if (t <= 0 || n < 0 || n >= g_forms.nfields) return -1;
    if (g_forms.field[n].type == WF_FIELD_SUBMIT) { g_forms.focus = n; return 1; }
    /* Lot 71: a checkbox, radio or select acts on the click */
    if (wf_form_is_toggle(&g_forms, n)) {
        if (wf_form_activate(&g_forms, n) != 0) return -1;
        (void)wf_form_focus(&g_forms, n);   /* Tab, Space and arrows go on from here */
        return 2;
    }
    (void)wf_form_focus(&g_forms, n);
    return 0;
}

/* Key in the focused field: 0 changed (render again), 1 Enter (submit),
 * 2 focus left (ESC), -1 not used (no focused field). */
int shell_page_form_key(int key) {
    if (!g_page_ok || g_forms.focus < 0) return -1;
    if (key == OS_VGA_KEY_ESC) { g_forms.focus = -1; return 2; }
    if (key == '\n' || key == '\r') return 1;
    if (key == '\t') { (void)wf_form_focus_next(&g_forms, 1); return 0; }
    if (key == '\b') { (void)wf_form_backspace(&g_forms); return 0; }
    if (key == OS_TERM_KEY_DEL) { (void)wf_form_delete(&g_forms); return 0; }
    if (key == OS_VGA_KEY_LEFT) { (void)wf_form_move(&g_forms, -1); return 0; }
    if (key == OS_VGA_KEY_RIGHT) { (void)wf_form_move(&g_forms, 1); return 0; }
    /* Lot 86: Down in a text field takes the remembered value (autofill);
     * with nothing remembered the key goes on (the page scrolls). */
    if (key == OS_VGA_KEY_DOWN && shell_desk_fill_key(&g_forms, g_forms.focus, g_res.url) == 0) return 0;
    if (key == OS_TERM_KEY_HOME) { (void)wf_form_move(&g_forms, -1000); return 0; }
    if (key == OS_TERM_KEY_END) { (void)wf_form_move(&g_forms, 1000); return 0; }
    if (key >= 0x20 && key <= 0xff && key != 0x7f) { (void)wf_form_insert(&g_forms, (char)key); return 0; }
    return -1;
}

/* Lot 71: "web-forms": one line per form and per visible field. */
int shell_page_forms_list(char* out, int cap) {
    int i;
    out[0] = 0;
    if (!g_page_ok) { scat(out, cap, "osui web forms none\n"); return 0; }
    scat(out, cap, "osui web forms "); scat_int(out, cap, g_forms.nforms);
    scat(out, cap, " fields "); scat_int(out, cap, g_forms.nfields);
    scat(out, cap, " dropped "); scat_int(out, cap, g_forms.dropped); scat(out, cap, "\n");
    for (i = 0; i < g_forms.nforms; i++) {
        scat(out, cap, "form "); scat_int(out, cap, i);
        scat(out, cap, g_forms.form[i].method == WF_METHOD_POST ? " POST " : " GET ");
        scat(out, cap, g_forms.form[i].action);
        scat(out, cap, "\n");
    }
    for (i = 0; i < g_forms.nfields; i++) {
        const wf_field_t* f = &g_forms.field[i];
        static const char* const tn[10] = { "?", "text", "password", "textarea", "hidden", "submit", "checkbox", "radio", "select", "file" };
        scat(out, cap, "field "); scat_int(out, cap, i);
        scat(out, cap, " form "); scat_int(out, cap, f->form);
        scat(out, cap, " "); scat(out, cap, tn[f->type <= 9 ? f->type : 0]);
        scat(out, cap, " name="); scat(out, cap, f->name);
        if (f->type != WF_FIELD_PASSWORD) { scat(out, cap, " value="); scat(out, cap, f->value); }
        if (f->type == WF_FIELD_CHECKBOX || f->type == WF_FIELD_RADIO) scat(out, cap, f->checked ? " checked" : " unchecked");
        if (f->type == WF_FIELD_SELECT) {
            int k;
            scat(out, cap, " options");
            for (k = 0; k < f->nopt; k++) {
                const char* ov = wf_form_option_value(&g_forms, i, k);
                scat(out, cap, k == f->sel ? " *" : " ");
                scat(out, cap, ov && ov[0] ? ov : "\"\"");
            }
        }
        if (g_forms.focus == i) scat(out, cap, " focus");
        scat(out, cap, "\n");
    }
    return 0;
}

/* Lot 85: "web-read". The reduced page as plain text, bounded by cap. */
int shell_page_article(char* out, int cap) {
    int n;
    if (!out || cap < 16) return -1;
    out[0] = 0;
    if (!g_page_ok || g_full_n <= 0) { scat(out, cap, "osui web read none\n"); return 0; }
    n = wf_article_text(g_full, g_full_n, out, cap - 1);
    if (n < 0) n = 0;
    if (n < cap - 1) { out[n++] = '\n'; out[n] = 0; }
    /* Lot 86: the raw page's tables and lists, kept beside the reader text. */
    if (g_blocks_n > 0 && n < cap - 1) {
        int k = 0;
        while (k < g_blocks_n && n < cap - 1) out[n++] = g_blocks[k++];
        out[n] = 0;
    }
    return n;
}

/* Sets field n (console "web-input"); 0 or -1. */
int shell_page_form_set(int n, const char* value) {
    if (!g_page_ok || n < 0 || n >= g_forms.nfields) return -1;
    if (wf_form_set(&g_forms, n, value) != 0) return -1;
    g_forms.focus = wf_form_is_toggle(&g_forms, n) ? -1 : n;
    return 0;
}

/* Lot 74: a form of the rendered page (webeng_finput.inc) sent with POST */
int shell_page_request(const char* url, const char* body, int body_len, const char* ctype,
                       char* src, int src_max, char* final_url, int url_max,
                       char* status, int st_max) {
    wf_fetch_req_t rq;
    if (status && st_max > 0) status[0] = 0;
    if (!url || !body || body_len < 0) return -1;
    rq.method = "POST"; rq.body = body; rq.body_len = body_len;
    rq.ctype = (ctype && ctype[0]) ? ctype : "application/x-www-form-urlencoded"; rq.extra = 0;
    return page_fetch(url, &rq, src, src_max, final_url, url_max, status, st_max);
}

/* Submits the form of the focused field (or field n when n >= 0): GET
 * builds the URL, POST sends the urlencoded body. Same output as
 * shell_fetch_page for the new page. */
int shell_page_submit(int n, char* src, int src_max, char* final_url, int url_max, char* status, int st_max) {
    static wf_form_submit_t sub;
    static char url[WF_FORM_SUBMIT_MAX];
    wf_fetch_req_t rq;
    int f, submitter = -1, i;
    if (status && st_max > 0) status[0] = 0;
    if (!g_page_ok) { if (status) scat(status, st_max, "osui web form error=no_form"); return -1; }
    if (n < 0) n = g_forms.focus;
    if (n < 0 || n >= g_forms.nfields) n = wf_form_first_text(&g_forms, -1);
    if (n < 0) { if (status) scat(status, st_max, "osui web form error=no_form"); return -1; }
    f = g_forms.field[n].form;
    if (g_forms.field[n].type == WF_FIELD_SUBMIT) submitter = n;
    if (wf_form_submit(&g_forms, f, submitter, &sub) != 0) {
        if (status) scat(status, st_max, "osui web form error=submit_refused (multipart or too long)");
        return -1;
    }
    for (i = 0; sub.url[i] && i < (int)sizeof(url) - 1; i++) url[i] = sub.url[i];
    url[i] = 0;
    /* Lot 86: remember typed values before the next page replaces the form. */
    (void)shell_desk_remember_form(&g_forms, f, g_res.url[0] ? g_res.url : url);
    {
        /* Lot 72: Google search form: the page without JavaScript, then
         * the fallback engines (shell_search_google_form) */
        static char gq[WSEARCH_QUERY + 1];
        if (sub.method != WF_METHOD_POST && wsearch_google_form(url, gq, (int)sizeof(gq))) {
            g_page_ok = 0;
            return shell_search_google_form(gq, src, src_max, final_url, url_max, status, st_max);
        }
        /* Lot 85: Wikipedia search forms (GET or POST) go to the API.
         * A POST to Google does too. Any other host is fetched for real:
         * GET uses the query string, POST sends the urlencoded body. */
        {
            static char sq[WSEARCH_QUERY + 1];
            int route = wf_search_route(url, sub.method == WF_METHOD_POST ? sub.body : 0, sq, (int)sizeof(sq));
            if (route == WF_ROUTE_WIKI || (route == WF_ROUTE_GOOGLE && sub.method == WF_METHOD_POST)) {
                g_page_ok = 0;
                if (route == WF_ROUTE_WIKI)
                    return shell_search_wiki_form(sq, src, src_max, final_url, url_max, status, st_max);
                return shell_search_google_form(sq, src, src_max, final_url, url_max, status, st_max);
            }
        }
    }
    if (sub.method == WF_METHOD_POST) {
        rq.method = "POST"; rq.body = sub.body; rq.body_len = sub.body_len; rq.ctype = sub.content_type; rq.extra = 0;
        return page_fetch(url, &rq, src, src_max, final_url, url_max, status, st_max);
    }
    return page_fetch(url, 0, src, src_max, final_url, url_max, status, st_max);
}

static int take_num(const char **s) {
    int n = 0, any = 0;
    while (**s == ' ') (*s)++;
    while (**s >= '0' && **s <= '9' && n < 1000) { n = n * 10 + (**s - '0'); (*s)++; any = 1; }
    return any ? n : -1;
}

int shell_page_load(const char *path, const char *url, char *status, int st_max) {
    int n, i;
    static char src[128];
    static char final_url[8];
    if (status && st_max > 0) status[0] = 0;
    if (!path || !path[0] || !url || !url[0]) {
        if (status) scat(status, st_max, "osui web load error=usage web-load PATH URL");
        return -1;
    }
    n = fsops_read(&shell_fs_io, path, (char *)g_body, (int)sizeof(g_body) - 1);
    if (n < 0) {
        if (status) scat(status, st_max, "osui web load error=read");
        return -1;
    }
    for (i = 0; i < (int)sizeof(g_res); i++) ((char *)&g_res)[i] = 0;
    for (i = 0; url[i] && i < (int)sizeof(g_res.url) - 1; i++) g_res.url[i] = url[i];
    g_res.url[i] = 0;
    for (i = 0; i < 9; i++) g_res.content_type[i] = "text/html"[i];
    g_res.status = 200;
    g_res.body_len = n;
    g_body[n] = 0;
    /* Lot 87: linked sheets come from the same directory as the file */
    {
        int last = -1, k;
        for (k = 0; path[k]; k++) if (path[k] == '/') last = k;
        for (k = 0; k < last && k < (int)sizeof(g_load_dir) - 1; k++) g_load_dir[k] = path[k];
        g_load_dir[k] = 0;
        if (last == 0) { g_load_dir[0] = '/'; g_load_dir[1] = 0; }
    }
    /* A saved page is opened with its stylesheet. The operator flag is unchanged. */
    {
        int r = page_present(0, 1, src, (int)sizeof(src), final_url, (int)sizeof(final_url), status, st_max);
        g_load_dir[0] = 0;
        if (g_css_ran && status && st_max > 0) {
            char line[160];
            int ln = wf_csslink_stats_text(&g_css_st, line, (int)sizeof(line));
            if (ln > 0) {
                if (line[ln - 1] == '\n') line[ln - 1] = 0;
                scat(status, st_max, "\n");
                scat(status, st_max, line);
            }
        }
        return r;
    }
}

int shell_page_preview(int field, char *out, int cap) {
    static char body[WF_FORM_SUBMIT_MAX + WF_FORM_BODY_MAX + 16];
    int n;
    if (!out || cap < 16) return -1;
    out[0] = 0;
    if (!g_page_ok) { scat(out, cap, "web-preview none\n"); return -1; }
    if (field < 0) field = g_forms.focus;
    if (field < 0 || field >= g_forms.nfields) field = wf_form_first_text(&g_forms, -1);
    n = wf_formfill_preview(&g_forms, field, body, (int)sizeof(body));
    if (n < 0) { scat(out, cap, "web-preview error=submit_refused\n"); return -1; }
    scat(out, cap, "web-preview ");
    scat(out, cap, body);
    return n;
}

int shell_page_fill_cmd(const char *line, char *out, int cap) {
    const char *a = line ? line : "";
    int field, offer, i, n;
    wf_fill_offer_t got[WF_FILL_OFFERS];
    if (!out || cap < 16) return -1;
    out[0] = 0;
    while (*a && *a != ' ') a++;
    while (*a == ' ') a++;
    if (!g_page_ok) { scat(out, cap, "web-fill none\n"); return -1; }
    if (a[0] == 'r' && a[1] == 'e') {
        n = shell_desk_remember_form(&g_forms, -1, g_res.url);
        scat(out, cap, "web-fill remembered ");
        scat_int(out, cap, n);
        scat(out, cap, "\n");
        return 0;
    }
    if (a[0] == 'a' && a[1] == 'p') {
        const char *p = a;
        int n1, n2;
        while (*p && *p != ' ') p++;
        n1 = take_num(&p);
        n2 = take_num(&p);
        if (n2 >= 0) { field = n1; offer = n2; }
        else { field = g_forms.focus; offer = n1; }
        if (field < 0) field = wf_form_first_text(&g_forms, -1);
        if (offer < 0 || shell_desk_apply_offer(&g_forms, field, offer, g_res.url) != 0) {
            scat(out, cap, "web-fill error=no_offer\n");
            return -1;
        }
        scat(out, cap, "web-fill applied ");
        scat_int(out, cap, offer);
        scat(out, cap, "\n");
        return 0;
    }
    field = take_num(&a);
    if (field < 0) field = g_forms.focus;
    if (field < 0) field = wf_form_first_text(&g_forms, -1);
    n = shell_desk_offers(&g_forms, field, g_res.url, got, WF_FILL_OFFERS);
    scat(out, cap, "web-fill field ");
    scat_int(out, cap, field);
    scat(out, cap, " offers ");
    scat_int(out, cap, n);
    scat(out, cap, "\n");
    for (i = 0; i < n; i++) {
        scat(out, cap, "offer ");
        scat_int(out, cap, i);
        scat(out, cap, " ");
        scat(out, cap, got[i].name);
        scat(out, cap, " ");
        scat(out, cap, got[i].value);
        if (got[i].site) {
            /* Lot 88: offered from another host of the same site */
            scat(out, cap, " site ");
            scat(out, cap, got[i].host);
        }
        scat(out, cap, "\n");
    }
    return 0;
}

int shell_page_css_cmd(const char *line, char *out, int cap) {
    const char *a = line ? line : "";
    if (!out || cap < 16) return -1;
    out[0] = 0;
    while (*a && *a != ' ') a++;
    while (*a == ' ') a++;
    if (a[0] == 'o' && a[1] == 'n' && (a[2] == 0 || a[2] == ' ')) {
        g_css_on = 1;
        scat(out, cap, "web-css on\n");
        return 0;
    }
    if (a[0] == 'o' && a[1] == 'f' && a[2] == 'f') {
        g_css_on = 0;
        scat(out, cap, "web-css off\n");
        return 0;
    }
    if (a[0] == 't' && a[1] == 'i' && a[2] == 'm' && a[3] == 'e' && (a[4] == 0 || a[4] == ' ')) {
        /* Lot 97: "web-css time N": the reader clock in ms (0..1e8) for
         * the animations and transitions of the next page */
        const char *t = a + 4;
        long v = 0;
        int dg = 0;
        while (*t == ' ') t++;
        while (*t >= '0' && *t <= '9' && dg < 10) { v = v * 10 + (*t - '0'); t++; dg++; }
        while (*t == ' ') t++;
        if (!dg || *t || v > WF_CSSANIM_MS_MAX) {
            scat(out, cap, "usage: web-css time <0..100000000>\n");
            return -1;
        }
        wf_html_set_csstime((int)v);
        scat(out, cap, "web-css time ");
        scat_int(out, cap, (int)v);
        scat(out, cap, "\n");
        return 0;
    }
    if (a[0] == 's' && a[1] == 'c' && a[2] == 'r' && a[3] == 'o' && a[4] == 'l' && a[5] == 'l' && (a[6] == 0 || a[6] == ' ')) {
        /* Lot 101: "web-css scroll N": the reader scroll position in
         * percent of the root scroll range (0..100) for the scroll
         * timelines of the next page */
        const char *t = a + 6;
        int v = 0, dg = 0;
        while (*t == ' ') t++;
        if (t[0] == 'g' && t[1] == 'u' && t[2] == 'i' && !t[3]) {
            /* Lot 105: "web-css scroll gui": the GUI scroll position of
             * the page (the layout's root scroll offset) */
            wf_html_set_cssscroll_gui(1);
            scat(out, cap, "web-css scroll gui\n");
            return 0;
        }
        while (*t >= '0' && *t <= '9' && dg < 4) { v = v * 10 + (*t - '0'); t++; dg++; }
        while (*t == ' ') t++;
        if (!dg || *t || v > 100) {
            scat(out, cap, "usage: web-css scroll <0..100>\n");
            return -1;
        }
        wf_html_set_cssscroll_gui(0);
        wf_html_set_cssscroll(v * 100);
        scat(out, cap, "web-css scroll ");
        scat_int(out, cap, v);
        scat(out, cap, "%\n");
        return 0;
    }
    if (wf_html_sitecss_report(out, cap) <= 0) scat(out, cap, "css none\n");
    {
        /* Lot 88: the @scope summary of the page */
        char sc[128];  /* Lot 90: 96 (the css-props line is up to 80); Lot 101: 128 (css-anim scroll) */
        if (wf_html_sitecss_scope(sc, (int)sizeof(sc)) > 0) {
            scat(out, cap, "css-scope ");
            scat(out, cap, sc);
            scat(out, cap, "\n");
        }
        /* Lot 89: custom properties set per element */
        if (wf_html_sitecss_vars(sc, (int)sizeof(sc)) > 0) {
            scat(out, cap, "css-vars ");
            scat(out, cap, sc);
            scat(out, cap, "\n");
        }
        /* Lot 90: @property registrations and declaration-time values */
        if (wf_html_sitecss_props(sc, (int)sizeof(sc)) > 0) {
            scat(out, cap, "css-props ");
            scat(out, cap, sc);
            scat(out, cap, "\n");
        }
        /* Lot 97: keyframes, animated, transitioned and font-relative
         * registered values (only on a page that has any) */
        if (wf_html_sitecss_anim(sc, (int)sizeof(sc)) > 0) {
            scat(out, cap, "css-anim ");
            scat(out, cap, sc);
            scat(out, cap, "\n");
        }
    }
    if (g_css_ran) {
        char line[160];
        if (wf_csslink_stats_text(&g_css_st, line, (int)sizeof(line)) > 0) scat(out, cap, line);
    }
    return 0;
}

/* Lot 114 (US-031): events after load, one path for the console words
 * (web-js click|key|input|focus|blur|submit|event|clicks|field|rfield) and the GUI
 * (a click on a "?__wfjs=N" link, keys the page view does not use, as
 * "web-js gkey KEY"). The event runs in the live realm of the page, the
 * page is written again and presented without a new script pass (the
 * reader, the sanitizer and the forms see it as the scripts left it).
 * Returns the source length of the current section (src), 0 when there
 * is nothing to show again, -1 when the line is not for a live page. */
static const char *js_word(const char *s, char *w, int cap) {
    int n = 0;
    while (*s == ' ') s++;
    while (*s && *s != ' ' && n < cap - 1) w[n++] = *s++;
    w[n] = 0;
    while (*s == ' ') s++;
    return s;
}

/* a word followed by one space at most: the rest is a value kept as typed */
static const char *js_last_word(const char *s, char *w, int cap) {
    int n = 0;
    while (*s == ' ') s++;
    while (*s && *s != ' ' && n < cap - 1) w[n++] = *s++;
    w[n] = 0;
    if (*s == ' ') s++;
    return s;
}

int shell_page_js(const char *line, char *src, int src_max, char *out, int out_max) {
    static char word[24], target[256], type[40], fu[600], st[640], cur_url[600];
    const char *a = line, *arg = 0;
    int r, n = 0, cur, log0 = 0, gui_key = 0, i, field = 0, fl, rfield = 0;
    if (out && out_max > 0) out[0] = 0;
    a = js_word(a, word, (int)sizeof(word));          /* web-js */
    a = js_word(a, word, (int)sizeof(word));
    if (s_eq(word, "clicks")) {
        static char cl[3000];
        int len = wf_jsweb_clickables(cl, (int)sizeof(cl)), c = 0;
        for (i = 0; i < len; i++) if (cl[i] == '\n') c++;
        scat(out, out_max, "osui web-js clicks ");
        scat_int(out, out_max, wf_jspage_alive() ? c : -1);
        scat(out, out_max, "\n");
        if (len > 0) scat(out, out_max, cl);
        return 0;
    }
    if (s_eq(word, "gkey")) {
        if (!g_page_ok || !wf_jsweb_wants_keys()) return -1;
        gui_key = 1;
        word[0] = 0; scat(word, (int)sizeof(word), "key");
    }
    if (s_eq(word, "key")) {
        static char key[24];
        a = js_word(a, key, (int)sizeof(key));
        type[0] = 0; scat(type, (int)sizeof(type), "key");
        arg = key;
        target[0] = 0; scat(target, (int)sizeof(target), a);
    } else if (s_eq(word, "field") || s_eq(word, "rfield")) {
        /* Lot 114: a reader form field: "field ACTION ORD NTH NAME|- [VALUE]"
         * (the GUI's engine controls) or "rfield ACTION N [VALUE]" (field N
         * of the console's Lot 71 forms: its name and rank among the
         * fields of that name) */
        static char act[16], num[16], nm[64], rv[WF_FORM_VALUE];
        int ord = 0, nth = 0;
        a = js_word(a, act, (int)sizeof(act));
        if (s_eq(word, "field")) a = js_word(a, num, (int)sizeof(num));
        else a = js_last_word(a, num, (int)sizeof(num));
        if (s_eq(word, "rfield") && s_eq(num, "f")) {      /* the focused field */
            if (!g_page_ok || g_forms.focus < 0) return -1;
            num[0] = 0; scat_int(num, (int)sizeof(num), g_forms.focus);
        }
        for (i = 0; num[i] >= '0' && num[i] <= '9' && ord < 100000; i++) ord = ord * 10 + (num[i] - '0');
        if (!act[0] || !num[0] || num[i]) return -1;
        nm[0] = 0;
        if (s_eq(word, "field")) {
            a = js_word(a, num, (int)sizeof(num));
            for (i = 0; num[i] >= '0' && num[i] <= '9' && nth < 100000; i++) nth = nth * 10 + (num[i] - '0');
            a = js_last_word(a, nm, (int)sizeof(nm));     /* the value keeps its spaces */
            if (s_eq(nm, "-")) nm[0] = 0;
        } else {
            int q;
            if (!g_page_ok || ord >= g_forms.nfields) return -1;
            const wf_field_t *f = &g_forms.field[ord];
            scat(nm, (int)sizeof(nm), f->name);
            for (q = 0; q < ord; q++) if (nm[0] && s_eq(g_forms.field[q].name, nm)) nth++;
            if (s_eq(act, "set")) {
                /* the field's state now, as the action its kind takes */
                rv[0] = 0;
                act[0] = 0;
                if (f->type == WF_FIELD_CHECKBOX || f->type == WF_FIELD_RADIO) {
                    scat(act, (int)sizeof(act), "check");
                    scat(rv, (int)sizeof(rv), f->checked ? "1" : "0");
                } else if (f->type == WF_FIELD_SELECT) {
                    const char *ov = wf_form_option_value(&g_forms, ord, f->sel);
                    scat(act, (int)sizeof(act), "change");
                    scat(rv, (int)sizeof(rv), ov ? ov : "");
                } else if (f->type == WF_FIELD_TEXT || f->type == WF_FIELD_PASSWORD || f->type == WF_FIELD_TEXTAREA) {
                    scat(act, (int)sizeof(act), "input");
                    scat(rv, (int)sizeof(rv), f->value);
                } else return -1;
                a = rv;
            }
            rfield = ord + 1;
        }
        type[0] = 0; scat(type, (int)sizeof(type), "field-"); scat(type, (int)sizeof(type), act);
        target[0] = 0; scat(target, (int)sizeof(target), "=");
        scat_int(target, (int)sizeof(target), ord); scat(target, (int)sizeof(target), ",");
        scat_int(target, (int)sizeof(target), nth); scat(target, (int)sizeof(target), ",");
        scat(target, (int)sizeof(target), nm);
        arg = a;
        field = 1;
    } else if (s_eq(word, "event")) {
        a = js_word(a, type, (int)sizeof(type));
        target[0] = 0; scat(target, (int)sizeof(target), a);
    } else if (s_eq(word, "click") || s_eq(word, "input") || s_eq(word, "focus") ||
               s_eq(word, "blur") || s_eq(word, "submit")) {
        type[0] = 0; scat(type, (int)sizeof(type), word);
        a = js_word(a, target, (int)sizeof(target));
        arg = a;
    } else return -1;
    {   /* a "?__wfjs=N" URL from the GUI is node N */
        int k = wf_jspage_mark_node(target);
        if (k >= 0) {
            target[0] = '@';
            target[1] = 0;
            scat_int(target, (int)sizeof(target), k);
        }
    }
    if (!g_page_ok || !wf_jspage_alive()) {
        if (field) return -1;      /* the reader goes on alone */
        if (!gui_key) scat(out, out_max, "osui web-js event error=no_live_page (web-js on, then open a page whose scripts ran)\n");
        return -1;
    }
    (void)wf_jspage_log(&log0);
    r = wf_jsweb_event(type, target, arg, (char *)g_body, (int)sizeof(g_body));
    if (r < 0 && field) return -1;     /* not a field of the scripts' page */
    if (r < 0) {
        scat(out, out_max, r == -2 ? "osui web-js event error=no_target " : r == -3 ? "osui web-js event error=arena_full " :
                           "osui web-js event error=no_live_page ");
        scat(out, out_max, type);
        scat(out, out_max, " ");
        scat(out, out_max, target[0] ? target : "(focus)");
        scat(out, out_max, "\n");
        return r == -2 ? 0 : -1;
    }
    cur = g_cur;
    cur_url[0] = 0; scat(cur_url, (int)sizeof(cur_url), g_res.url);
    fl = wf_jspage_event_flags();
    g_res.body_len = r;       /* the page as the scripts left it */
    if (field && !(fl & WF_JSEV_DOM)) {
        /* the scripts left the DOM alone: the reader keeps its page (and
         * the state of its controls) */
        scat(out, out_max, "osui web-js field ");
        scat(out, out_max, type + 6);
        scat(out, out_max, " ");
        scat(out, out_max, target);
        scat(out, out_max, " ok dom=0");
        scat(out, out_max, fl & WF_JSEV_PREVENTED ? " prevented=1" : " prevented=0");
        scat(out, out_max, "\n");
        n = 0;
    } else {
        int keep = g_forms.focus;
        g_js_replay = 1;
        n = page_present(0, g_css_on, src, src_max, fu, (int)sizeof(fu), st, (int)sizeof(st));
        g_js_replay = 0;
        if (rfield > 0 && keep >= 0 && keep < g_forms.nfields) (void)wf_form_focus(&g_forms, keep);   /* typing goes on */
        if (n > 0 && cur > 0 && cur < g_sec.n) n = shell_page_section(cur, src, src_max);
    }
    if (rfield > 0 && (fl & WF_JSEV_VALUE) && !(fl & WF_JSEV_DOM) && rfield - 1 < g_forms.nfields)
        (void)wf_form_set(&g_forms, rfield - 1, wf_jspage_field_value());   /* a script's value */
    if (field && (fl & WF_JSEV_VALUE)) {
        scat(out, out_max, "osui web-js field-value ");
        scat(out, out_max, wf_jspage_field_value());
        scat(out, out_max, "\n");
    }
    if (field && (fl & WF_JSEV_DOM)) {
        scat(out, out_max, "osui web-js field ");
        scat(out, out_max, type + 6);
        scat(out, out_max, " ");
        scat(out, out_max, target);
        scat(out, out_max, n > 0 ? " ok dom=1" : " error=present dom=1");
        scat(out, out_max, fl & WF_JSEV_PREVENTED ? " prevented=1" : " prevented=0");
        scat(out, out_max, "\n");
    } else if (!field) {
    scat(out, out_max, "osui web-js event ");
    scat(out, out_max, type);
    scat(out, out_max, " ");
    scat(out, out_max, target[0] ? target : "(focus)");
    scat(out, out_max, n > 0 ? " ok" : " error=present");
    wf_jsweb_event_note(out, out_max);
    scat(out, out_max, "\n");
    }
    {   /* the console lines the event added */
        int ln;
        const char *lg = wf_jspage_log(&ln);
        int s0 = log0;
        while (s0 < ln) {
            int e = s0;
            while (e < ln && lg[e] != '\n') e++;
            scat(out, out_max, "osui web-js log: ");
            {
                static char one[200];
                int m = e - s0 < (int)sizeof(one) - 1 ? e - s0 : (int)sizeof(one) - 1;
                for (i = 0; i < m; i++) one[i] = lg[s0 + i];
                one[m] = 0;
                scat(out, out_max, one);
            }
            scat(out, out_max, "\n");
            s0 = e + 1;
        }
    }
    (void)cur_url;
    return n > 0 ? n : 0;
}
