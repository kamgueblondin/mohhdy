/* osui_gui.c - Boucle gui guest. Cerveau = osui_runtime.c.
 * Surface produit = framebuffer VBE QEMU (fenetre graphique, pas HTML).
 * Pas Chromium. Pas un LLM de production. Pas agent/.
 */

#include "osui_scale.h"
#include "osui_gui.h"
#include "wsearch.h"
#include "osui_runtime.h"
#include "osui_ide.h"
#include "webeng.h"
#include "webrt.h"
#include "os_syscalls.h"
#include "os_charset.h"
#include "osui_panes.h"
#include "osui_files.h"
#include "osui_parity.h"

#define COLS OS_VGA_COLS
#define ROWS OS_VGA_ROWS

#define ATTR_TITLE 0x1F
#define ATTR_HINT  0x0B
#define ATTR_DIM   0x08
#define ATTR_DOCK  0x70

#define GUI_TERM_MAX 16
#define GUI_TERM_LEN 64

static void serial_puts(const char *s);   /* below: guest serial or test stub */
static char g_input[96];
static int g_ilen;
static int g_leave;
static int g_idle;
/* Set when the desktop hands over to the console terminal window. */
static int g_term_req;
static char g_pending[96];
static osui_ide_t g_ide;
static osui_ide_load_t g_ide_load;
static osui_ide_save_t g_ide_save;
static osui_ide_exec_t g_ide_exec;
static char g_ide_buf[OSUI_IDE_TEXT_MAX + 1];
static char g_term[GUI_TERM_MAX][GUI_TERM_LEN];
static int g_term_i;
static int g_term_n;
/* Lot 6 (US-031): page rendered by the guest web engine in the web pane. */
static we_doc_t g_web;
static int g_web_active;
static int g_web_scroll;
static char g_web_file[256];   /* Lot 67: room for a URL (was 40) */
static int g_web_mobile;
static osui_mobile_op_t g_mobile_op;
static char g_web_src[OSUI_WEB_SRC_MAX];
/* desktop apps (scene version 7) */
static const fsops_io_t *g_fs_io;
static osui_files_t g_files;
static osui_ide_t g_edit;
static int g_focus = -1;
static int g_evt_on, g_evt_len;
static char g_evt[96];
static char g_app_buf[FSOPS_FILE_MAX + 1];
/* UX5: the user heap (umalloc in the guest) for files over FSOPS_FILE_MAX */
static void *(*g_urealloc)(void *, unsigned);
static char *g_big;
void osui_gui_set_heap(void *(*realloc_fn)(void *, unsigned)) {
    g_urealloc = realloc_fn;
    osui_ide_set_heap(realloc_fn);
}
/* Buffer of n + 1 bytes: the static one, else the heap; 0 = no memory. */
static char *big_buf(int n) {
    char *m;
    if (n <= FSOPS_FILE_MAX) return g_app_buf;
    if (!g_urealloc) return 0;
    m = (char *)g_urealloc(g_big, (unsigned)n + 1U);
    if (m) g_big = m;
    return m;
}
static osui_app_view_t g_view;
/* UX4: generic window slots. g_slot[i] is the app kind shown in window i
 * (0: free); any app takes the first free slot, one window per app. */
static int g_slot[OS_FB_APPS];

static int slot_of(int kind) {
    int i;
    for (i = 0; i < OS_FB_APPS; i++) if (g_slot[i] == kind) return i;
    return -1;
}
static int slot_take(int kind) {
    int i = slot_of(kind);
    if (i >= 0) return i;
    for (i = 0; i < OS_FB_APPS; i++) if (!g_slot[i]) { g_slot[i] = kind; return i; }
    return -1;
}
static void slot_drop(int kind) {
    int i = slot_of(kind);
    if (i < 0) return;
    g_slot[i] = 0;
    if (g_focus == i || g_focus < 0 || !g_slot[g_focus]) {
        g_focus = -1;
        for (i = 0; i < OS_FB_APPS; i++) if (g_slot[i]) { g_focus = i; break; }
    }
}

static int slen(const char *s) {
    int n = 0;
    if (!s) return 0;
    while (s[n]) n++;
    return n;
}

static void scpy(char *d, int max, const char *s) {
    int i = 0;
    if (!s) s = "";
    if (max <= 0) return;
    while (s[i] && i < max - 1) {
        d[i] = s[i];
        i++;
    }
    d[i] = 0;
}

static void scat(char *d, int max, const char *s) {
    int len = slen(d);
    if (len < max) scpy(d + len, max - len, s);
}

static int scmp(const char *a, const char *b) {
    int i = 0;
    if (!a) a = "";
    if (!b) b = "";
    while (a[i] && b[i]) {
        if (a[i] != b[i]) return a[i] - b[i];
        i++;
    }
    return a[i] - b[i];
}

static int sncmp(const char *a, const char *b, int n) {
    int i;
    for (i = 0; i < n; i++) {
        if (a[i] != b[i]) return (unsigned char)a[i] - (unsigned char)b[i];
        if (!a[i]) return 0;
    }
    return 0;
}

static void web_num(char *d, int max, const char *label, int v) {
    char t[12];
    int k = 0, n = slen(d);
    unsigned u = (unsigned)(v < 0 ? -v : v);
    scpy(d + n, max - n, label);
    n = slen(d);
    if (v < 0 && n < max - 1) d[n++] = '-';
    do { t[k++] = (char)('0' + u % 10U); u /= 10U; } while (u && k < 11);
    while (k && n < max - 1) d[n++] = t[--k];
    d[n] = 0;
}

/* Lot 6: page pipeline. US-042 <template> components are expanded in the
 * HTML, then the engine parses; the collected CSS goes through US-040
 * @media flattening for the viewport and US-041 var() resolution before the
 * cascade. Counters are kept for the status line. */
static int g_web_components, g_web_media_kept, g_web_media_dropped, g_web_vars, g_web_vars_unresolved;
static char g_web_tmp[OSUI_WEB_SRC_MAX];
static char g_web_san[OSUI_WEB_SRC_MAX];
static int g_web_sanitized;
/* Local resource reads reuse the existing FS adapter and shell rights. */
static int web_image_read(void *ctx, const char *path, unsigned char *dst, int cap) {
    os_dirent_t st;
    (void)ctx;
    if (!g_fs_io || !g_fs_io->stat || g_fs_io->stat(path, &st) != 0 || st.flags == OS_DIRENT_DIR) return -1;
    if (st.size > (unsigned)cap) return cap + 1;
    return fsops_read(g_fs_io, path, (char *)dst, cap);
}

/* Lot 68: one text scale state for the console command and the GUI */
static osui_scale_t g_scale;
static int g_scale_ready, g_scale_loaded;
static void scale_load_once(void);
static int scale_save(void);
osui_scale_t *osui_gui_scale(void) {
    if (!g_scale_ready) { osui_scale_init(&g_scale); g_scale_ready = 1; }
    if (!g_scale_loaded && g_fs_io) scale_load_once();
    return &g_scale;
}
/* Page viewport: 640 CSS px at 1x as before; a zoomed page gets the view
 * width divided by the zoom (at least 240), so it reflows instead of
 * being cut. */
static int g_web_vw = OSUI_WEB_VIEWPORT;
static int web_viewport_w(void) {
    osui_scale_t *sc = osui_gui_scale();
    int q = osui_scale_effective(sc, OS_UI_Z_WEB), pw = sc->web_pw > 0 ? sc->web_pw : 992, v;
    if (q <= OS_UI_Q1) return OSUI_WEB_VIEWPORT;
    v = pw * 4 / q;
    if (v > OSUI_WEB_VIEWPORT) v = OSUI_WEB_VIEWPORT;
    if (v < 240) v = 240;
    return v;
}

static int (*g_wops_atlas_dims)(void *ctx, int slot, int *w, int *h);
static int osui_web_pipeline(const char *src, int n) {
    int defined = 0, expanded = 0, kept = 0, dropped = 0, vdef = 0, unres = 0, m, rc, k;
    webrt_viewport_t vp;
    static char css1[WE_CSS_MAX], css2[WE_CSS_MAX];
    g_web_components = g_web_media_kept = g_web_media_dropped = g_web_vars = g_web_vars_unresolved = 0;
    g_web_sanitized = 0;
    /* Lot 7 (US-039): sanitizer first (script, iframe, object, embed, on*,
     * javascript: URLs, unsafe CSS), then the rest of the pipeline. */
    m = webrt_sanitize_page(src, g_web_san, OSUI_WEB_SRC_MAX, &g_web_sanitized);
    if (m < 0 || m >= OSUI_WEB_SRC_MAX) return -1;
    g_web_san[m] = 0; src = g_web_san; n = m;
    m = webrt_components_expand(src, g_web_tmp, OSUI_WEB_SRC_MAX, &defined, &expanded);
    if (m > 0 && expanded > 0) { src = g_web_tmp; n = m; g_web_components = expanded; }
    rc = we_parse(&g_web, src, n);
    if (rc != 0) return rc;
    if (g_web.css_len > 0) {
        g_web.css[g_web.css_len < WE_CSS_MAX ? g_web.css_len : WE_CSS_MAX - 1] = 0;
        g_web_vw = web_viewport_w();
        vp.width = g_web_vw; vp.height = OSUI_WEB_VIEWPORT_H; vp.dark = 0;
        m = webrt_css_flatten_media(g_web.css, &vp, css1, WE_CSS_MAX, &kept, &dropped);
        if (m >= 0) {
            m = webrt_css_resolve_vars(css1, css2, WE_CSS_MAX, &vdef, &unres);
            if (m >= 0) {
                for (k = 0; k < m && k < WE_CSS_MAX - 1; k++) g_web.css[k] = css2[k];
                g_web.css_len = (uint32_t)k;
            }
        }
        g_web_media_kept = kept; g_web_media_dropped = dropped;
        g_web_vars = vdef; g_web_vars_unresolved = unres;
    }
    rc = we_style(&g_web);
    if (rc != 0) return rc;
    we_set_image_reader(web_image_read, 0);
    we_set_image_atlas(g_wops_atlas_dims, 0);   /* Lot 72: fetched images */
    g_web_vw = web_viewport_w();
    return we_layout_viewport(&g_web, g_web_vw, OSUI_WEB_VIEWPORT_H);
}

we_doc_t *osui_gui_web_doc(void) { return &g_web; }
int osui_gui_web_active(void) { return g_web_active; }
int osui_gui_web_sanitized(void) { return g_web_sanitized; }

static osui_web_fetch_t g_web_fetch;
static char g_web_final[256];
void osui_gui_set_web_fetch(osui_web_fetch_t fn) { g_web_fetch = fn; }
int osui_gui_web_is_url(const char *s) {
    return s && ((sncmp(s, "https://", 8) == 0) || (sncmp(s, "http://", 7) == 0));
}

static int web_is_search(const char *f) { return f && sncmp(f, "search:", 7) == 0 && f[7]; }

static int g_web_errpage;

/* Lot 69: a small HTML page explaining a failed network load. */
static int web_error_page(const char *url, const char *why, char *o, int cap) {
    static const char *head = "<html><head><title>Page impossible a ouvrir</title></head><body>"
                              "<h1>Page impossible a ouvrir</h1><p>Adresse :</p><pre>";
    const char *mid = "</pre><p>Raison :</p><pre>";
    const char *hint = "</pre><p>Etapes : DNS, connexion TCP, TLS (certificat), HTTP. "
                       "Le message ci-dessus dit laquelle a echoue. Verifier le reseau avec net-if, "
                       "ou essayer fetch -v URL dans le terminal.</p></body></html>";
    const char *parts[5];
    int k = 0, i, j;
    const char *w = why;
    if (cap < 64) return -1;
    if (w && w[0] == 'o' && w[1] == 's') {           /* skip the "osui web fetch error=" prefix */
        for (i = 0; w[i]; i++) if (w[i] == '=') { w = w + i + 1; break; }
    }
    parts[0] = head; parts[1] = url; parts[2] = mid; parts[3] = w ? w : "?"; parts[4] = hint;
    for (j = 0; j < 5; j++) {
        for (i = 0; parts[j][i] && k < cap - 8; i++) {
            char c = parts[j][i];
            if ((j == 1 || j == 3) && (c == '<' || c == '>' || c == '&')) c = c == '&' ? '+' : (c == '<' ? '(' : ')');
            o[k++] = c;
        }
    }
    o[k] = 0;
    return k;
}

/* Lot 72: a local page is UTF-8; the web engine reads display bytes (one
 * byte per cell, include/os_charset.h). Each run between '<' and '>' is
 * converted on its own, so every text chunk the engine sees starts in
 * glyph page 0 and carries its own page selector. Returns the new length
 * (never longer). */
int osui_gui_web_src_to_display(char *s, int n) {
    int r = 0, w = 0;
    while (r < n) {
        int e = r, m, k;
        char keep;
        if (s[r] == '<' || s[r] == '>') { s[w++] = s[r++]; continue; }
        while (e < n && s[e] != '<' && s[e] != '>') e++;
        keep = e < n ? s[e] : 0;
        s[e] = 0;
        m = os_text_to_display_inplace(s + r, e - r + 1);
        for (k = 0; k < m; k++) s[w + k] = s[r + k];
        w += m;
        if (e < n) s[e] = keep;
        r = e;
    }
    if (w < n) s[w] = 0;
    return w;
}

int osui_gui_web_load(const char *file, char *status, int max) {
    int n = -1, rc;
    static char sq[WSEARCH_QUERY + 8];
    if (status && max > 0) status[0] = 0;
    g_web_final[0] = 0;
    if (osui_gui_web_is_url(file) || web_is_search(file)) {
        /* Lot 67: real network page (shell_fetch_page), reader view */
        static char fst[512];
        if (!g_web_fetch) {
            if (status) scpy(status, max, "osui web error=no_network_loader (url refused)");
            return -1;
        }
        n = g_web_fetch(file, g_web_src, OSUI_WEB_SRC_MAX, g_web_final, (int)sizeof(g_web_final), fst, (int)sizeof(fst));
        if (n < 0) {
            /* Lot 69: the window shows why the page did not open (DNS, TCP,
             * TLS, HTTP) instead of staying closed; the status keeps the
             * error and the call still fails for the console callers. */
            if (status) scpy(status, max, fst);
            n = web_error_page(file, fst, g_web_src, OSUI_WEB_SRC_MAX);
            g_web_errpage = (n > 0 && osui_web_pipeline(g_web_src, n) == 0);
            return -1;
        }
        g_web_src[n < OSUI_WEB_SRC_MAX ? n : OSUI_WEB_SRC_MAX - 1] = 0;
        rc = osui_web_pipeline(g_web_src, n);
        if (status) {
            scpy(status, max, fst);
            web_num(status, max, rc == 0 ? " render ok nodes " : " render error nodes ", (int)g_web.perf.nodes);
            web_num(status, max, " items ", (int)g_web.perf.items);
            web_num(status, max, " dropped ", (int)g_web.perf.items_dropped);
            web_num(status, max, " sanitized ", g_web_sanitized);
        }
        return rc;
    }
    if (g_ide_load) n = g_ide_load(file, g_web_src, OSUI_WEB_SRC_MAX - 1);
    if (n < 0 && g_web_fetch && wsearch_is_query(file) && slen(file) <= WSEARCH_QUERY) {
        /* Lot 69: the address bar searches text that is not a URL nor a
         * page (same path as `search`, through the fetch hook) */
        scpy(sq, (int)sizeof(sq), "search:");
        scpy(sq + 7, (int)sizeof(sq) - 7, file);
        return osui_gui_web_load(sq, status, max);
    }
    if (n < 0) { if (status) { scpy(status, max, "osui web error=not_found file="); scpy(status + slen(status), max - slen(status), file); } return -1; }
    g_web_src[n] = 0;
    n = osui_gui_web_src_to_display(g_web_src, n);
    rc = osui_web_pipeline(g_web_src, n);
    if (status) {
        scpy(status, max, rc == 0 ? "osui web render file=" : "osui web error=render file=");
        scpy(status + slen(status), max - slen(status), file);
        web_num(status, max, " nodes ", (int)g_web.perf.nodes);
        web_num(status, max, " rules ", (int)g_web.perf.rules);
        web_num(status, max, " boxes ", (int)g_web.perf.boxes);
        web_num(status, max, " lines ", (int)g_web.perf.lines);
        web_num(status, max, " items ", (int)g_web.perf.items);
        web_num(status, max, " dropped ", (int)g_web.perf.items_dropped);
        web_num(status, max, " page ", (int)g_web.perf.page_w);
        web_num(status, max, "x", (int)g_web.perf.page_h);
        web_num(status, max, " diag ", (int)g_web.ndiag);
        web_num(status, max, " components ", g_web_components);
        web_num(status, max, " media_kept ", g_web_media_kept);
        web_num(status, max, " media_dropped ", g_web_media_dropped);
        web_num(status, max, " vars ", g_web_vars);
        web_num(status, max, " sanitized ", g_web_sanitized);
    }
    return rc;
}

static void app_out(char *out, int out_max, const char *a, const char *b);
static int evt_int(const char **p);

/* UX4: browser history (local pages) and link navigation */
#define WEB_HIST 8
static char g_web_hist[WEB_HIST][256];
static int g_web_hn, g_web_hi = -1;

static int web_go(const char *file, int push, char *out, int out_max) {
    char status[320];
    int k;
    g_web_errpage = 0;
    if (osui_gui_web_load(file, status, (int)sizeof(status)) != 0) {
        if (out) { scat(out, out_max, status); scat(out, out_max, "\n"); }
        if (g_web_errpage) {
            /* the error page is shown in the browser window */
            g_web_errpage = 0;
            scpy(g_web_file, (int)sizeof(g_web_file), file);
            g_web_active = 1;
            g_web_mobile = 0;
            g_web_scroll = 0;
            if (out) scat(out, out_max, "osui web error-page shown\n");
        }
        return -1;
    }
    scpy(g_web_file, (int)sizeof(g_web_file), g_web_final[0] ? g_web_final : file);
    g_web_active = 1;
    g_web_mobile = 0;
    g_web_scroll = 0;
    if (push) {
        if (g_web_hi + 1 >= WEB_HIST) {
            for (k = 0; k + 1 < WEB_HIST; k++) scpy(g_web_hist[k], 256, g_web_hist[k + 1]);
            g_web_hi--;
        }
        g_web_hi++;
        scpy(g_web_hist[g_web_hi], 256, g_web_final[0] ? g_web_final : file);
        g_web_hn = g_web_hi + 1;           /* a new page drops the forward list */
    }
    if (out) { scat(out, out_max, status); scat(out, out_max, "\n"); }
    return 0;
}

static void gui_cat_u(char *d, int max, unsigned v);
/* Lot 71: page ops of the shell (sections, forms) */
static const osui_web_page_ops_t *g_wops;
void osui_gui_set_web_page_ops(const osui_web_page_ops_t *ops) {
    g_wops = ops;
    g_wops_atlas_dims = ops ? ops->atlas_dims : 0;
}
static int web_nsections(void) { return (g_wops && g_wops->sections && osui_gui_web_is_url(g_web_file)) ? g_wops->sections() : 0; }

/* Shows section k of the kept page; keep: the scroll offset stays (a
 * field edit), else top (or bottom when to_end). */
static int web_show_section(int k, int keep, int to_end, char *out, int out_max) {
    int n, rc, scroll = g_web_scroll;
    char m[64];
    if (web_nsections() <= 0 || !g_wops->section) return -1;
    n = g_wops->section(k, g_web_src, OSUI_WEB_SRC_MAX);
    if (n <= 0) return -1;
    rc = osui_web_pipeline(g_web_src, n);
    if (rc != 0) return -1;
    g_web_scroll = keep ? we_scroll(&g_web, scroll) : to_end ? we_scroll(&g_web, 1 << 20) : 0;
    if (!keep && out) {
        scpy(m, (int)sizeof(m), " ");
        gui_cat_u(m, (int)sizeof(m), (unsigned)(k + 1));
        scat(m, (int)sizeof(m), "/");
        gui_cat_u(m, (int)sizeof(m), (unsigned)web_nsections());
        app_out(out, out_max, "osui web section", m);
    }
    return 0;
}

/* Submits the form of field n (-1: the focused one) and shows the answer. */
static int web_submit(int n, char *out, int out_max) {
    static char status[512];
    int len, rc;
    if (!g_wops || !g_wops->submit || web_nsections() <= 0) {
        app_out(out, out_max, "osui web form error=no_form", 0);
        return -1;
    }
    g_web_final[0] = 0;
    len = g_wops->submit(n, g_web_src, OSUI_WEB_SRC_MAX, g_web_final, (int)sizeof(g_web_final), status, (int)sizeof(status));
    if (len < 0) { app_out(out, out_max, status[0] ? status : "osui web form error=failed", 0); return -1; }
    g_web_src[len < OSUI_WEB_SRC_MAX ? len : OSUI_WEB_SRC_MAX - 1] = 0;
    rc = osui_web_pipeline(g_web_src, len);
    app_out(out, out_max, "osui web form submit ", status);
    if (rc != 0) return -1;
    if (g_web_final[0]) scpy(g_web_file, (int)sizeof(g_web_file), g_web_final);
    g_web_active = 1;
    g_web_mobile = 0;
    g_web_scroll = 0;
    if (g_web_hi + 1 >= WEB_HIST) {
        int k;
        for (k = 0; k + 1 < WEB_HIST; k++) scpy(g_web_hist[k], 256, g_web_hist[k + 1]);
        g_web_hi--;
    }
    g_web_hi++;
    scpy(g_web_hist[g_web_hi], 256, g_web_file);
    g_web_hn = g_web_hi + 1;
    return 0;
}

static void web_open_line(const char *line, char *out, int out_max);
int osui_gui_web_command(const char *line, char *out, int out_max) {
    const char *a;
    int n = 0, neg = 0;
    if (out && out_max > 0) out[0] = 0;
    if (sncmp(line, "web-open", 8) == 0 && (line[8] == ' ' || !line[8])) {
        static char wl[300];
        scpy(wl, (int)sizeof(wl), "web ");
        scat(wl, (int)sizeof(wl), line + 8);
        web_open_line(wl, out, out_max);
        n = web_nsections();
        if (n > 0) {
            char m[48];
            scpy(m, (int)sizeof(m), " sections ");
            gui_cat_u(m, (int)sizeof(m), (unsigned)n);
            app_out(out, out_max, "osui web page", m);
        }
        return 1;
    }
    if (scmp(line, "web-forms") == 0) {
        static char fl[2048];
        if (!g_wops || !g_wops->forms_list || web_nsections() <= 0) { app_out(out, out_max, "osui web forms none", 0); return 1; }
        (void)g_wops->forms_list(fl, (int)sizeof(fl));
        if (out) scat(out, out_max, fl);
        return 1;
    }
    if (sncmp(line, "web-section", 11) == 0 && (line[11] == ' ' || !line[11])) {
        int cnt = web_nsections(), cur = g_wops && g_wops->current ? g_wops->current() : 0;
        a = line + 11;
        while (*a == ' ') a++;
        if (cnt <= 0) { app_out(out, out_max, "osui web section error=no_page (open an http(s) page first)", 0); return 1; }
        if (!*a) {
            char m[48];
            scpy(m, (int)sizeof(m), " ");
            gui_cat_u(m, (int)sizeof(m), (unsigned)(cur + 1));
            scat(m, (int)sizeof(m), "/");
            gui_cat_u(m, (int)sizeof(m), (unsigned)cnt);
            app_out(out, out_max, "osui web section", m);
            return 1;
        }
        if (sncmp(a, "next", 4) == 0) n = cur + 1;
        else if (sncmp(a, "prev", 4) == 0) n = cur - 1;
        else { while (*a >= '0' && *a <= '9' && n < 1000) n = n * 10 + (*a++ - '0'); n--; }
        if (n < 0 || n >= cnt || web_show_section(n, 0, 0, out, out_max) != 0)
            app_out(out, out_max, "osui web section error=out_of_range", 0);
        return 1;
    }
    if (sncmp(line, "web-find", 8) == 0 && (line[8] == ' ' || !line[8])) {
        /* Lot 71: find text in the whole page; "web-find" alone: next match */
        static char last[64];
        int cnt = web_nsections(), cur = g_wops && g_wops->current ? g_wops->current() : 0, hits = 0, k, from;
        char m[96];
        a = line + 8;
        while (*a == ' ') a++;
        if (cnt <= 0 || !g_wops || !g_wops->find) { app_out(out, out_max, "osui web find error=no_page (open an http(s) page first)", 0); return 1; }
        if (*a) { scpy(last, (int)sizeof(last), a); from = cur; }
        else if (last[0]) from = cur + 1 < cnt ? cur + 1 : 0;
        else { app_out(out, out_max, "osui web find error=usage web-find TEXT", 0); return 1; }
        k = g_wops->find(last, from, &hits);
        if (k < 0) { app_out(out, out_max, "osui web find none text=", last); return 1; }
        if (k != cur && web_show_section(k, 0, 0, 0, 0) != 0) { app_out(out, out_max, "osui web find error=section", 0); return 1; }
        scpy(m, (int)sizeof(m), " section ");
        gui_cat_u(m, (int)sizeof(m), (unsigned)(k + 1));
        scat(m, (int)sizeof(m), "/");
        gui_cat_u(m, (int)sizeof(m), (unsigned)cnt);
        scat(m, (int)sizeof(m), " matches ");
        gui_cat_u(m, (int)sizeof(m), (unsigned)hits);
        app_out(out, out_max, "osui web find ok", m);
        return 1;
    }
    if (sncmp(line, "web-input", 9) == 0 && (line[9] == ' ' || !line[9])) {
        a = line + 9;
        while (*a == ' ') a++;
        while (*a >= '0' && *a <= '9' && n < 1000) n = n * 10 + (*a++ - '0');
        if (*a == ' ') a++;
        if (!g_wops || !g_wops->form_set || web_nsections() <= 0 || g_wops->form_set(n, a) != 0) {
            app_out(out, out_max, "osui web input error=no_field usage web-input N TEXT (N from the page links)", 0);
            return 1;
        }
        (void)web_show_section(g_wops->current ? g_wops->current() : 0, 1, 0, 0, 0);
        app_out(out, out_max, "osui web input ok", 0);
        return 1;
    }
    if (sncmp(line, "web-submit", 10) == 0 && (line[10] == ' ' || !line[10])) {
        a = line + 10;
        while (*a == ' ') a++;
        if (*a >= '0' && *a <= '9') { while (*a >= '0' && *a <= '9' && n < 1000) n = n * 10 + (*a++ - '0'); }
        else neg = 1;
        (void)web_submit(neg ? -1 : n, out, out_max);
        return 1;
    }
    return 0;
}

/* href relative to the current page; only local pages: a scheme
 * (javascript:, http:, file:) or a fragment is refused */
int osui_gui_web_resolve(const char *base, const char *href, char *dst, int max) {
    char dir[FSOPS_PATH_MAX];
    int i;
    if (!href || !href[0] || href[0] == '#') return -1;
    /* Lot 67: on a fetched page the reader view made every link an
     * absolute http(s) URL; it is followed over the network. A local page
     * still refuses any scheme. */
    if (osui_gui_web_is_url(base)) {
        if (!osui_gui_web_is_url(href) || slen(href) >= max) return -1;
        scpy(dst, max, href);
        return 0;
    }
    for (i = 0; href[i]; i++) if (href[i] == ':' || href[i] == '?' || href[i] == '#') return -1;
    fsops_parent(base && base[0] ? base : "/web/index.html", dir, (int)sizeof(dir));
    fsops_resolve(dir, href, dst, max);
    return slen(dst) < 40 ? 0 : -1;
}

static void gui_cat_u(char *d, int max, unsigned v);
/* Lot 68: lay the open page out again for the current zoom */
static void web_reflow(char *out, int out_max) {
    char m[48];
    if (!g_web_active || !g_web_file[0]) return;
    if (web_viewport_w() == g_web_vw) return;
    g_web_vw = web_viewport_w();
    if (we_layout_viewport(&g_web, g_web_vw, OSUI_WEB_VIEWPORT_H) != 0) return;
    if (g_web_scroll > g_web.perf.page_h) g_web_scroll = 0;
    scpy(m, (int)sizeof(m), " viewport ");
    gui_cat_u(m, (int)sizeof(m), (unsigned)g_web_vw);
    app_out(out, out_max, "osui web reflow", m);
}
static void web_event(const char *p, char *out, int out_max) {
    if (sncmp(p, "view", 4) == 0) {
        /* Lot 68: kernel view size and scale; a new zoom relays the page */
        int pw, ph, q, fw, fh, before = web_viewport_w();
        p += 4;
        pw = evt_int(&p); ph = evt_int(&p); q = evt_int(&p); fw = evt_int(&p); fh = evt_int(&p);
        osui_scale_web_view(osui_gui_scale(), pw, ph, q, fw, fh);
        if (web_viewport_w() != before) web_reflow(out, out_max);
        return;
    }
    if (!g_web_active) return;
    if (sncmp(p, "back", 4) == 0 || sncmp(p, "fwd", 3) == 0) {
        int to = g_web_hi + (p[0] == 'b' ? -1 : 1);
        if (to < 0 || to >= g_web_hn) { app_out(out, out_max, p[0] == 'b' ? "osui web back none" : "osui web fwd none", 0); return; }
        if (web_go(g_web_hist[to], 0, out, out_max) == 0) g_web_hi = to;
        app_out(out, out_max, p[0] == 'b' ? "osui web back file=" : "osui web fwd file=", g_web_file);
        return;
    }
    if (sncmp(p, "click", 5) == 0) {
        char href[256], dst[256];   /* Lot 67: absolute URLs of fetched pages */
        int x, y;
        p += 5;
        x = evt_int(&p);
        y = evt_int(&p);
        if (we_link_at(&g_web, x, y, href, (int)sizeof(href)) <= 0) return;
        if (sncmp(href, "mohhdy-form:", 12) == 0) {
            /* Lot 71: a form control: focus the field or submit */
            int t = (g_wops && g_wops->form_link) ? g_wops->form_link(href) : -1;
            if (t == 0) {
                (void)web_show_section(g_wops->current ? g_wops->current() : 0, 1, 0, 0, 0);
                app_out(out, out_max, "osui web form focus ", href + 12);
            } else if (t == 1) {
                (void)web_submit(-1, out, out_max);
            } else if (t == 2) {
                /* Lot 71: checkbox / radio / select changed */
                (void)web_show_section(g_wops->current ? g_wops->current() : 0, 1, 0, 0, 0);
                app_out(out, out_max, "osui web form toggle ", href + 12);
            } else app_out(out, out_max, "osui web form error=unknown_field", 0);
            return;
        }
        if (osui_gui_web_resolve(g_web_file, href, dst, (int)sizeof(dst)) != 0) {
            app_out(out, out_max, "osui web link refused href=", href);
            return;
        }
        app_out(out, out_max, "osui web link href=", href);
        (void)web_go(dst, 1, out, out_max);
    }
}

static void web_open_line(const char *line, char *out, int out_max) {
    const char *f = line + 4;
    char status[320];
    while (*f == ' ') f++;
    if (!*f || slen(f) >= (int)sizeof(g_web_file)) {
        if (out) scpy(out, out_max, "osui web error=usage web FILE\n");
        return;
    }
    (void)status;
    if (out && out_max > 0) out[0] = 0;
    if (!g_web_active) { g_web_hi = -1; g_web_hn = 0; }   /* a fresh browser session */
    (void)web_go(f, 1, out, out_max);
}

/* Lot 69: Internet search on the desktop. "search WORDS" and the chat
 * shortcut "/search WORDS" open the result page in /browser (links are
 * the results); "search-open N" opens result N of the last search. The
 * shell owns the results (userspace/shell_search.c) through the hook. */
static osui_search_url_t g_search_url;
void osui_gui_set_search_url(osui_search_url_t fn) { g_search_url = fn; }
static int search_line(const char *in, char *out, int out_max) {
    static char line[WSEARCH_QUERY + 16];
    const char *q = 0;
    if (sncmp(in, "search-open", 11) == 0 && (in[11] == ' ' || in[11] == 0)) {
        int n = 0;
        const char *a = in + 11;
        char url[WSEARCH_URL];
        while (*a == ' ') a++;
        while (*a >= '0' && *a <= '9' && n < 100) n = n * 10 + (*a++ - '0');
        if (!g_search_url || g_search_url(n, url, (int)sizeof(url)) != 0) {
            if (out) scpy(out, out_max, "osui search error=no_result usage search-open N (after search WORDS)\n");
            return 1;
        }
        scpy(line, (int)sizeof(line), "web ");
        scpy(line + 4, (int)sizeof(line) - 4, url);
        web_open_line(line, out, out_max);
        return 1;
    }
    if (sncmp(in, "search ", 7) == 0) q = in + 7;
    else if (sncmp(in, "/search ", 8) == 0) q = in + 8;
    else if (scmp(in, "search") == 0) {   /* bare "/search" stays the Recherche panel */
        if (out) scpy(out, out_max, "osui search error=usage search WORDS\n");
        return 1;
    }
    if (!q) return 0;
    while (*q == ' ') q++;
    if (!*q || slen(q) > WSEARCH_QUERY) {
        if (out) scpy(out, out_max, "osui search error=usage search WORDS (200 bytes max)\n");
        return 1;
    }
    scpy(line, (int)sizeof(line), "web search:");
    scpy(line + 11, (int)sizeof(line) - 11, q);
    web_open_line(line, out, out_max);
    return 1;
}

void osui_gui_set_mobile_op(osui_mobile_op_t fn) { g_mobile_op = fn; }
int osui_gui_mobile_active(void) { return g_web_active && g_web_mobile; }

int osui_gui_show_items(const char *title, const os_fb_web_item_t *items, int n, int page_w, int page_h) {
    int k;
    if (!items || n < 0) return -1;
    if (n > OS_FB_WEB_ITEMS) n = OS_FB_WEB_ITEMS;
    for (k = 0; k < n; k++) g_web.items[k] = items[k];
    g_web.nitems = (uint32_t)n;
    g_web.perf.page_w = (uint32_t)page_w;
    g_web.perf.page_h = (uint32_t)page_h;
    g_web.perf.items = (uint32_t)n;
    g_web.perf.items_dropped = 0;
    scpy(g_web.title, (int)sizeof(g_web.title), title ? title : "");
    scpy(g_web_file, (int)sizeof(g_web_file), "mobile");
    g_web_active = 1;
    g_web_mobile = 1;
    g_web_scroll = 0;
    return n;
}

void osui_gui_set_ide_ops(osui_ide_load_t load, osui_ide_save_t save, osui_ide_exec_t exec) {
    g_ide_load = load;
    g_ide_save = save;
    g_ide_exec = exec;
}

int osui_gui_ide_active(void) { return g_ide.active; }

void osui_gui_ide_frame_row(int r, char *dst, int max) { osui_ide_frame_row(&g_ide, r, dst, max); }

void osui_gui_ide_output(const char *text) { if (g_ide.active) osui_ide_out_append(&g_ide, text); }

static void ide_out(char *out, int out_max, const char *a, const char *b) {
    if (!out || out_max <= 0) return;
    scpy(out, out_max, a);
    if (b) scpy(out + slen(out), out_max - slen(out), b);
    scpy(out + slen(out), out_max - slen(out), "\n");
}

/* "pm-ide FILE" typed on the desktop opens the editor full screen. */
static int ide_open_line(const char *line, char *out, int out_max) {
    const char *f = line + 6;
    int n = -1;
    while (*f == ' ') f++;
    if (!*f) { ide_out(out, out_max, "osui pm-ide error=usage pm-ide FILE", 0); return 1; }
    if (slen(f) >= (int)sizeof(g_ide.file)) { ide_out(out, out_max, "osui pm-ide error=name_too_long", 0); return 1; }
    if (g_ide_load) n = g_ide_load(f, g_ide_buf, OSUI_IDE_TEXT_MAX);
    osui_ide_open(&g_ide, f, n > 0 ? g_ide_buf : "", n > 0 ? n : 0);
    ide_out(out, out_max, n >= 0 ? "osui pm-ide open fullscreen file=" : "osui pm-ide open fullscreen new file=", f);
    return 0;
}

static void ide_action(int act, char *out, int out_max) {
    char msg[OSUI_IDE_ROW_LEN];
    int len;
    if (act == OSUI_IDE_QUIT) {
        ide_out(out, out_max, "osui pm-ide quit file=", g_ide.file);
        g_ide.active = 0;
        return;
    }
    len = osui_ide_text(&g_ide, g_ide_buf, (int)sizeof(g_ide_buf));
    if (len < 0) { osui_ide_set_status(&g_ide, "buffer too large"); return; }
    if (act == OSUI_IDE_SAVE) {
        if (!g_ide_save || g_ide_save(g_ide.file, g_ide_buf, len) != 0) {
            osui_ide_set_status(&g_ide, "save failed");
            ide_out(out, out_max, "osui pm-ide save error file=", g_ide.file);
            return;
        }
        g_ide.modified = 0;
        scpy(msg, (int)sizeof(msg), "saved ");
        scpy(msg + slen(msg), (int)sizeof(msg) - slen(msg), g_ide.file);
        osui_ide_set_status(&g_ide, msg);
        ide_out(out, out_max, "osui pm-ide saved file=", g_ide.file);
        return;
    }
    msg[0] = 0;
    if (!g_ide_exec) {
        osui_ide_set_status(&g_ide, "no PromptMessage VM in this binary");
        ide_out(out, out_max, "osui pm-ide exec=missing", 0);
        return;
    }
    if (act == OSUI_IDE_RUN) osui_ide_out_clear(&g_ide);
    (void)g_ide_exec(act, g_ide.file, g_ide_buf, len, msg, (int)sizeof(msg));
    osui_ide_set_status(&g_ide, msg);
    if (act == OSUI_IDE_RUN) {
        char line[OSUI_IDE_ROW_LEN + 16];
        if (g_ide.out_total == 0) osui_ide_out_append(&g_ide, "(no output)");
        else g_ide.out_visible = 1;
        scpy(line, (int)sizeof(line), msg);
        scpy(line + slen(line), (int)sizeof(line) - slen(line), " output ");
        {
            char num[12]; int k = 0, v = g_ide.out_total, j; char t[12];
            do { t[k++] = (char)('0' + v % 10); v /= 10; } while (v && k < 11);
            for (j = 0; j < k; j++) num[j] = t[k - 1 - j];
            num[k] = 0;
            scpy(line + slen(line), (int)sizeof(line) - slen(line), num);
        }
        ide_out(out, out_max, "osui pm-ide run ", line);
        return;
    }
    ide_out(out, out_max, "osui pm-ide check ", msg);
}


/* ---- desktop apps: file explorer and text editor --------------------- */

void osui_gui_set_fs_io(const fsops_io_t *io) { g_fs_io = io; }

/* Lot 68: load the saved scale once (missing file: auto), then run a
 * ui-scale / text-size line and save on change. Same code for the console
 * and the GUI chat, which both reach this through shell dispatch. */
static int scale_save(void) {
    osui_scale_t *sc = osui_gui_scale();
    char buf[160];
    int n;
    if (!g_fs_io) return -1;
    if (g_fs_io->mkdir) (void)g_fs_io->mkdir("/persist");
    n = osui_scale_save(sc, buf, (int)sizeof(buf));
    if (n <= 0 || fsops_write(g_fs_io, OSUI_SCALE_CFG, buf, n) != 0) return -1;
    sc->saved_at = sc->changes;
    return 0;
}
/* Lot 69: first boot (no /persist/ui.cfg): the auto rule, unless the
 * QEMU harness pinned a scale through fw_cfg (kernel display info). */
static int g_scale_hint_q;
void osui_gui_set_scale_hint(int q) { g_scale_hint_q = (q >= OS_UI_Q_MIN && q <= OS_UI_Q_MAX) ? q : 0; }
static void scale_load_once(void) {
    char buf[160];
    int n;
    if (g_scale_loaded || !g_fs_io) return;
    g_scale_loaded = 1;
    n = web_image_read(0, OSUI_SCALE_CFG, (unsigned char *)buf, (int)sizeof(buf) - 1);
    if (n > 0 && n < (int)sizeof(buf)) { buf[n] = 0; if (osui_scale_load(osui_gui_scale(), buf, n) == 0) return; }
    if (g_scale_hint_q) osui_gui_scale()->ui_q = (uint8_t)g_scale_hint_q;
}
int osui_gui_scale_command(const char *line, char *out, int cap) {
    osui_scale_t *sc;
    unsigned before;
    int rc;
    scale_load_once();
    sc = osui_gui_scale();
    before = sc->changes;
    rc = osui_scale_command(sc, line, out, cap);
    if (rc == 0 && sc->changes != before && scale_save() == 0) scat(out, cap, " saved " OSUI_SCALE_CFG);
    return rc;
}
/* Lot 71: small /persist files for other shell modules (search.cfg),
 * through the same FS adapter and rights as ui.cfg. */
int osui_gui_persist_read(const char *path, char *buf, int cap) {
    int n = web_image_read(0, path, (unsigned char *)buf, cap);
    return (n > cap) ? -1 : n;
}
int osui_gui_persist_write(const char *path, const char *buf, int n) {
    if (!g_fs_io || n < 0) return -1;
    if (g_fs_io->mkdir) (void)g_fs_io->mkdir("/persist");
    return fsops_write(g_fs_io, path, buf, n) == 0 ? 0 : -1;
}
int osui_gui_focus(void) { return g_focus; }

/* Lot 46: decimal number appended to a log line */
static void gui_cat_u(char *d, int max, unsigned v) {
    char tmp[12];
    int k = 0, n;
    if (!v) tmp[k++] = '0';
    while (v) { tmp[k++] = (char)('0' + v % 10U); v /= 10U; }
    n = slen(d);
    while (k && n < max - 1) d[n++] = tmp[--k];
    d[n] = 0;
}

static void app_out(char *out, int out_max, const char *a, const char *b) {
    int n;
    if (!out || out_max <= 0) return;
    n = slen(out);
    scpy(out + n, out_max - n, a);
    if (b) scat(out, out_max, b);
    scat(out, out_max, "\n");
}

int osui_gui_files_open(const char *dir, char *out, int out_max) {
    if (!g_fs_io) { app_out(out, out_max, "osui files error=no_fs", 0); return -1; }
    if (slot_of(OS_FB_APP_FILES) < 0 && slot_take(OS_FB_APP_FILES) < 0) { app_out(out, out_max, "osui app error=no_slot", 0); return -1; }
    if (!g_files.active) osui_files_open(&g_files, g_fs_io, dir && dir[0] ? dir : "/");
    else if (dir && dir[0]) (void)osui_files_cd(&g_files, dir);
    g_focus = slot_of(OS_FB_APP_FILES);
    app_out(out, out_max, "osui files open dir=", g_files.cwd);
    return 0;
}

int osui_gui_edit_open(const char *file, char *out, int out_max) {
    int n = 0;
    char path[FSOPS_PATH_MAX];
    if (!g_fs_io) { app_out(out, out_max, "osui edit error=no_fs", 0); return -1; }
    if (g_edit.active && g_edit.modified) {
        osui_ide_set_status(&g_edit, "non enregistre: enregistrez avant d'ouvrir un autre fichier");
        g_focus = slot_of(OS_FB_APP_EDIT);
        app_out(out, out_max, "osui edit busy unsaved=", g_edit.file);
        return -1;
    }
    if (slot_of(OS_FB_APP_EDIT) < 0 && slot_take(OS_FB_APP_EDIT) < 0) { app_out(out, out_max, "osui app error=no_slot", 0); return -1; }
    fsops_resolve(g_files.active ? g_files.cwd : "/", file && file[0] ? file : "sans_titre.txt", path, (int)sizeof(path));
    if (slen(path) >= (int)sizeof(g_edit.file)) { app_out(out, out_max, "osui edit error=name_too_long", 0); return -1; }
    {
        os_dirent_t st;
        if (g_fs_io->stat && g_fs_io->stat(path, &st) == 0) {
            char *buf = big_buf((int)st.size);
            n = buf ? fsops_read(g_fs_io, path, buf, (int)st.size > FSOPS_FILE_MAX ? (int)st.size : FSOPS_FILE_MAX)
                    : FSOPS_ERR_TOOBIG;
            if (n < 0) {
                app_out(out, out_max, "osui edit error=", fsops_strerror(n));
                if (g_files.active) scpy(g_files.status, (int)sizeof(g_files.status), fsops_strerror(n));
                if (!g_edit.active) slot_drop(OS_FB_APP_EDIT);
                return -1;
            }
            osui_ide_open_plain(&g_edit, path, buf, n);
            app_out(out, out_max, "osui edit open file=", path);
            {
                char num[48];
                int v = g_edit.nlines, k = 0, j;
                char rev[12];
                do { rev[k++] = (char)('0' + v % 10); v /= 10; } while (v && k < 11);
                scpy(num, (int)sizeof(num), "osui edit lines=");
                j = slen(num);
                while (k && j < (int)sizeof(num) - 1) num[j++] = rev[--k];
                num[j] = 0;
                app_out(out, out_max, num, g_edit.truncated ? " truncated" : " complete");
            }
        } else {
            osui_ide_open_plain(&g_edit, path, "", 0);
            osui_ide_set_status(&g_edit, "nouveau fichier (cree a l'enregistrement)");
            app_out(out, out_max, "osui edit open new file=", path);
        }
    }
    g_focus = slot_of(OS_FB_APP_EDIT);
    return 0;
}

int osui_gui_tool_open(int kind, const char *arg, char *out, int out_max);

static int is_image(const char *p) {
    /* Lot 50: same predicate as the parity table (.jpg included). */
    return osui_path_is_image(p);
}

static void edit_action(int act, char *out, int out_max) {
    int len, rc;
    char *sbuf;
    if (act == OSUI_IDE_QUIT) {
        app_out(out, out_max, "osui edit close file=", g_edit.file);
        g_edit.active = 0;
        slot_drop(OS_FB_APP_EDIT);
        return;
    }
    if (act == OSUI_IDE_OPEN) {
        char p[FSOPS_PATH_MAX];
        scpy(p, (int)sizeof(p), g_edit.prompt_out);
        (void)osui_gui_edit_open(p, out, out_max);
        return;
    }
    if (act == OSUI_IDE_RUN) {
        /* ESC r: run the saved .pm file in the PromptMessage window */
        char p[FSOPS_PATH_MAX];
        if (g_edit.modified) { osui_ide_set_status(&g_edit, "enregistrez d'abord (ESC w), puis ESC r"); return; }
        scpy(p, (int)sizeof(p), g_edit.file);
        (void)osui_gui_tool_open(OS_FB_APP_PMRUN, p, out, out_max);
        return;
    }
    if (act != OSUI_IDE_SAVE && act != OSUI_IDE_SAVE_AS) return;
    {
        int need = osui_ide_text_size(&g_edit);
        sbuf = big_buf(need);
        len = sbuf ? osui_ide_save_text(&g_edit, sbuf, need > FSOPS_FILE_MAX ? need + 1 : (int)sizeof(g_app_buf)) : -1;
    }
    if (len < 0) { osui_ide_set_status(&g_edit, "texte trop grand (memoire)"); return; }
    if (act == OSUI_IDE_SAVE_AS) {
        char p[FSOPS_PATH_MAX];
        fsops_resolve(g_files.active ? g_files.cwd : "/", g_edit.file, p, (int)sizeof(p));
        scpy(g_edit.file, (int)sizeof(g_edit.file), p);
    }
    rc = fsops_write(g_fs_io, g_edit.file, sbuf, len);
    if (rc != 0) {
        char msg[OSUI_IDE_ROW_LEN];
        scpy(msg, (int)sizeof(msg), "erreur: ");
        scat(msg, (int)sizeof(msg), fsops_strerror(rc));
        if (act == OSUI_IDE_SAVE_AS) osui_ide_save_as_failed(&g_edit);
        osui_ide_set_status(&g_edit, msg);
        app_out(out, out_max, "osui edit save error=", fsops_strerror(rc));
        return;
    }
    g_edit.modified = 0;
    g_edit.quit_armed = 0;
    osui_ide_set_status(&g_edit, "enregistre");
    app_out(out, out_max, "osui edit saved file=", g_edit.file);
    if (g_files.active) (void)osui_files_refresh(&g_files);
}

static void files_result(int r, const char *log_before, char *out, int out_max) {
    if (g_files.log[0] && scmp(g_files.log, log_before) != 0) app_out(out, out_max, g_files.log, 0);
    if (r == OSUI_FILES_OPEN) {
        /* open with: PNM images in the viewer, everything else in the editor */
        if (is_image(g_files.open_path)) (void)osui_gui_tool_open(OS_FB_APP_VIEW, g_files.open_path, out, out_max);
        else (void)osui_gui_edit_open(g_files.open_path, out, out_max);
    }
    else if (r == OSUI_FILES_CLOSE) {
        app_out(out, out_max, "osui files close", 0);
        g_files.active = 0;
        slot_drop(OS_FB_APP_FILES);
    }
}

static int evt_int(const char **p) {
    int v = 0, neg = 0;
    while (**p == ' ') (*p)++;
    if (**p == '-') { neg = 1; (*p)++; }
    while (**p >= '0' && **p <= '9') { v = v * 10 + (**p - '0'); (*p)++; }
    return neg ? -v : v;
}

static void files_event(const char *p, char *out, int out_max) {
    static char before[160];
    int r = OSUI_FILES_NONE;
    scpy(before, (int)sizeof(before), g_files.log);
    if (sncmp(p, "close", 5) == 0) r = OSUI_FILES_CLOSE;
    else if (sncmp(p, "tool", 4) == 0) { p += 4; r = osui_files_tool(&g_files, osui_app_tool_hit(osui_files_toolbar, evt_int(&p))); }
    else if (sncmp(p, "dclick", 6) == 0) { int rr, cc; p += 6; rr = evt_int(&p); cc = evt_int(&p); r = osui_files_click(&g_files, rr, cc, 1); }
    else if (sncmp(p, "click", 5) == 0) { int rr, cc; p += 5; rr = evt_int(&p); cc = evt_int(&p); r = osui_files_click(&g_files, rr, cc, 0); }
    files_result(r, before, out, out_max);
}

static void edit_event(const char *p, char *out, int out_max) {
    int act = OSUI_IDE_NONE;
    if (sncmp(p, "close", 5) == 0) {
        act = g_edit.modified && !g_edit.quit_armed ? OSUI_IDE_NONE : OSUI_IDE_QUIT;
        if (act == OSUI_IDE_NONE) {
            g_edit.quit_armed = 1;
            osui_ide_set_status(&g_edit, "non enregistre: Fermer encore abandonne, ESC w enregistre");
            app_out(out, out_max, "osui edit close refused unsaved=", g_edit.file);
        }
    } else if (sncmp(p, "tool", 4) == 0) { p += 4; act = osui_ide_tool(&g_edit, osui_app_tool_hit(osui_ide_toolbar, evt_int(&p))); }
    else if (sncmp(p, "drag", 4) == 0) { int a, b, c, d; p += 4; a = evt_int(&p); b = evt_int(&p); c = evt_int(&p); d = evt_int(&p); osui_ide_drag(&g_edit, a, b, c, d); }
    else if (sncmp(p, "click", 5) == 0 || sncmp(p, "dclick", 6) == 0) { int rr, cc; p += (*p == 'd') ? 6 : 5; rr = evt_int(&p); cc = evt_int(&p); osui_ide_click(&g_edit, rr, cc); }
    edit_action(act, out, out_max);
}

static void files_key(int key, char *out, int out_max) {
    static char before[160];
    if (key == 'v' && g_files.mode == OSUI_FILES_BROWSE) {
        /* "open with" the viewer (text or image) */
        char p[FSOPS_PATH_MAX];
        if (osui_files_selected(&g_files, p, (int)sizeof(p))) (void)osui_gui_tool_open(OS_FB_APP_VIEW, p, out, out_max);
        return;
    }
    if (key == 'y' && g_files.mode == OSUI_FILES_BROWSE) {
        /* copy the selected path to the shared clipboard (paste it in the
         * terminal with the right button, in the editor with ESC v) */
        char p[FSOPS_PATH_MAX];
        if (osui_files_selected(&g_files, p, (int)sizeof(p))) {
            osui_ide_set_clipboard(p, slen(p));
            scpy(g_files.status, (int)sizeof(g_files.status), "chemin copie dans le presse-papiers");
            app_out(out, out_max, "osui files clip ", p);
        }
        return;
    }
    scpy(before, (int)sizeof(before), g_files.log);
    files_result(osui_files_key(&g_files, key), before, out, out_max);
}

static void edit_key(int key, char *out, int out_max) { edit_action(osui_ide_key(&g_edit, key), out, out_max); }

static void sys_vga_image(const os_fb_image_t *img);
#include "osui_gui_apps.inc"

void osui_gui_event(const char *line, char *out, int out_max) {
    const char *p = line;
    if (sncmp(p, "focus ", 6) == 0) {
        p += 6;
        if (*p == 'd') g_focus = -1;
        else {
            int k = evt_int(&p);
            if (k >= 0 && k < OS_FB_APPS && g_slot[k]) g_focus = k;
        }
        return;
    }
    if (sncmp(p, "web ", 4) == 0) { web_event(p + 4, out, out_max); return; }
    if (sncmp(p, "cmd ", 4) == 0) {
        /* Lot 69: a desktop click (dock, menu, pane close) runs its command
         * as its own line; a half-typed chat line is kept, not prefixed */
        static char keep[sizeof(g_input)];
        int keep_n = g_ilen, i;
        scpy(keep, (int)sizeof(keep), g_input);
        g_ilen = 0;
        g_input[0] = 0;
        /* the close button of the full screen web / editor window: the
         * typed "/center" would be eaten by those windows (keys go to the
         * page or the editor), so close them the way their keys do */
        if (scmp(p + 4, "/center") == 0 && (g_ide.active || g_web_active)) {
            g_ilen = keep_n;
            scpy(g_input, (int)sizeof(g_input), keep);
            if (g_ide.active) ide_action(OSUI_IDE_QUIT, out, out_max);
            else (void)osui_gui_feed_key(OS_VGA_KEY_ESC, out, out_max);
            return;
        }
        /* same serial proof as a typed desktop line */
        serial_puts("osui gui line=");
        serial_puts(p + 4);
        serial_puts("\n");
        for (i = 4; p[i]; i++) (void)osui_gui_feed_key((unsigned char)p[i], out, out_max);
        (void)osui_gui_feed_key('\n', out, out_max);
        scpy(g_input, (int)sizeof(g_input), keep);
        g_ilen = keep_n;
        return;
    }
    if (sncmp(p, "app ", 4) != 0) return;
    p += 4;
    {
        int slot = evt_int(&p), kind;
        while (*p == ' ') p++;
        if (slot < 0 || slot >= OS_FB_APPS || !(kind = g_slot[slot])) return;
        if (sncmp(p, "size", 4) == 0) {
            /* UX5: the window grid changed (maximize / restore) */
            int rr, cc;
            p += 4; rr = evt_int(&p); cc = evt_int(&p);
            if (kind == OS_FB_APP_EDIT) {
                osui_ide_set_view(&g_edit, rr, cc);
                app_out(out, out_max, "osui edit view resized", 0);
            } else if (kind == OS_FB_APP_FILES) {
                /* Lot 46: the explorer columns follow the width */
                char m[48];
                osui_files_set_view(&g_files, rr, cc);
                scpy(m, (int)sizeof(m), " cols=");
                gui_cat_u(m, (int)sizeof(m), (unsigned)(cc > 0 ? cc : 0));
                app_out(out, out_max, "osui files view resized", m);
            } else if (tool_of(kind)) {
                /* Lot 45 (US-G15): every tool window follows its grid */
                char m[96];
                osui_tool_t *tt = tool_of(kind);
                osui_tool_set_view(tt, rr, cc, &g_tool_src);
                scpy(m, (int)sizeof(m), osui_tool_name(kind));
                scat(m, (int)sizeof(m), " rows=");
                { char nb[12]; int v = rr, k = 0, j; char tmp[12]; if (!v) tmp[k++] = '0'; while (v) { tmp[k++] = (char)('0' + v % 10); v /= 10; } for (j = 0; j < k; j++) nb[j] = tmp[k - 1 - j]; nb[k] = 0; scat(m, (int)sizeof(m), nb); }
                /* Lot 46: the output already shown is wrapped again */
                scat(m, (int)sizeof(m), " cols=");
                gui_cat_u(m, (int)sizeof(m), (unsigned)(cc > 0 ? cc : 0));
                scat(m, (int)sizeof(m), " lines=");
                gui_cat_u(m, (int)sizeof(m), (unsigned)tt->n);
                scat(m, (int)sizeof(m), " w1=");
                gui_cat_u(m, (int)sizeof(m), (unsigned)(tt->n > 1 ? slen(tt->lines[1]) : 0));
                app_out(out, out_max, "osui tool view resized app=", m);
            }
            return;
        }
        g_focus = slot;
        app_event(kind, p, out, out_max);
    }
}

static void app_to_scene(const osui_app_view_t *v, os_fb_app_t *a, int kind) {
    int r;
    a->kind = (uint8_t)kind;
    a->nrows = (uint8_t)v->nrows;
    a->cur_row = (int8_t)v->cur_row;
    a->cur_col = (int8_t)v->cur_col;
    a->dirty = (uint8_t)v->dirty;
    scpy(a->title, (int)sizeof(a->title), v->title);
    scpy(a->tool, OS_FB_APP_COLS, v->tool);
    scpy(a->status, OS_FB_APP_COLS, v->status);
    for (r = 0; r < OS_FB_APP_MAX_ROWS && r < OSUI_APP_MAX_ROWS; r++) {
        int k;
        scpy(a->rows[r], OS_FB_APP_MAX_COLS, v->rows[r]);
        for (k = 0; k < OS_FB_APP_MAX_COLS && k < OSUI_APP_MAX_COLS; k++) a->attr[r][k] = v->attr[r][k];
    }
}

int osui_gui_term_requested(void) { return g_term_req; }

int osui_gui_take_pending(char *dst, int max) {
    int had = g_pending[0] != 0;
    if (dst && max > 0) scpy(dst, max, g_pending);
    g_pending[0] = 0;
    g_term_req = 0;
    return had;
}

/* Hand the screen to the console: the desktop loop ends, the shell goes
 * back to its own prompt (same line editor, same commands) and the kernel
 * draws the console in a VBE window. The typed line, if any, is run there. */
static int request_terminal(const char *line, char *out, int out_max) {
    g_term_req = 1;
    scpy(g_pending, (int)sizeof(g_pending), line ? line : "");
    if (scmp(osui_get_pane(), "shell") == 0) osui_dispatch_line("/close", 0, 0);
    g_leave = 1;
    g_ilen = 0;
    g_input[0] = 0;
    if (out) {
        scpy(out, out_max, "osui gui term chrome=vbe_term same_console=true line=");
        scat(out, out_max, g_pending);
        scat(out, out_max, "\n");
    }
    return 1;
}

void osui_gui_term_reset(void) {
    g_term_i = 0;
    g_term_n = 0;
}

int osui_gui_term_count(void) {
    return g_term_n;
}

void osui_gui_term_at(int i, char *dst, int max) {
    int slot;
    if (!dst || max <= 0) return;
    dst[0] = 0;
    if (i < 0 || i >= g_term_n) return;
    slot = (g_term_i + i) % GUI_TERM_MAX;
    scpy(dst, max, g_term[slot]);
}

void osui_gui_term_push(const char *text) {
    char line[GUI_TERM_LEN];
    int n = 0;
    int ansi = 0; /* the line had a color code (ESC [ ... letter) */
    if (!text) return;
    while (*text) {
        if (*text == 27 && text[1] == '[') {
            /* Shell color codes are not text: drop the whole sequence. */
            text += 2;
            while (*text && !(*text >= 0x40 && *text <= 0x7E)) text++;
            if (*text) text++;
            ansi = 1;
            continue;
        }
        if (*text == '\n' || n == GUI_TERM_LEN - 1) {
            line[n] = 0;
            if (n > 0 || (*text == '\n' && !ansi)) {
                int slot;
                if (g_term_n < GUI_TERM_MAX) {
                    slot = (g_term_i + g_term_n) % GUI_TERM_MAX;
                    g_term_n++;
                } else {
                    slot = g_term_i;
                    g_term_i = (g_term_i + 1) % GUI_TERM_MAX;
                }
                scpy(g_term[slot], GUI_TERM_LEN, line);
            }
            n = 0;
            if (*text == '\n') {
                ansi = 0;
                text++;
                continue;
            }
        }
        if (*text >= 32 && *text <= 126) line[n++] = *text;
        if (*text) text++;
    }
    if (n > 0) {
        int slot;
        line[n] = 0;
        if (g_term_n < GUI_TERM_MAX) {
            slot = (g_term_i + g_term_n) % GUI_TERM_MAX;
            g_term_n++;
        } else {
            slot = g_term_i;
            g_term_i = (g_term_i + 1) % GUI_TERM_MAX;
        }
        scpy(g_term[slot], GUI_TERM_LEN, line);
    }
}

static void first_token(const char *s, char *d, int max) {
    int n = 0;
    if (!d || max <= 0) return;
    d[0] = 0;
    if (!s) return;
    while (*s == ' ' || *s == '\t') s++;
    while (*s && *s != ' ' && *s != '\t' && n < max - 1) d[n++] = *s++;
    d[n] = 0;
}

static int token_ai(const char *s) {
    while (s && (*s == ' ' || *s == '\t')) s++;
    if (!s || s[0] != 'a' || s[1] != 'i') return 0;
    return s[2] == 0 || s[2] == ' ' || s[2] == '\t' || s[2] == '-';
}

void osui_gui_browser_inspect(void) {
    osui_gui_term_push("Browser-OS Native Panel : FS-as-web, containerized tabs, isolated storage");
    osui_gui_term_push("Tab container: active_origin=self allowed=self,http://127.0.0.1,p2p://node");
    osui_gui_term_push("DOM Tree Sandbox & JS Eval: ready (dom_simulator)");
    osui_gui_term_push("Performance Metrics: web-perf & browser-telemetry perf active");
}

void osui_gui_vfs_inspect(const char *path) {
    char line[128];
    scpy(line, (int)sizeof(line), "VFS Sandbox Inspection (FS-as-web) : ");
    if (path && path[0]) {
        scat(line, (int)sizeof(line), path);
    } else {
        scat(line, (int)sizeof(line), "initrd/, overlay/, fat16/, fat32/");
    }
    osui_gui_term_push(line);
    osui_gui_term_push("Mount points: /initrd (ro), /overlay (rw), /fat16 (rw), /fat32 (ro)");
    osui_gui_term_push("Sandbox files: /etc/pm_ca.pub, /web_app.json, /demo/hello.txt");
    osui_gui_term_push("Nodes: html, head, body, div#app, button#menu-toggle, form#invoice-form");
}

static int is_word(const char *s, const char *w) {
    int n = slen(w);
    return sncmp(s, w, n) == 0 && (s[n] == 0 || s[n] == ' ' || s[n] == '\t');
}

/* One decision point for a line typed on the desktop.
 * HOST: needs the GUI loop itself (leave, web pane, editor). mobile-ui is
 *       caught before when the mobile shell is linked.
 * SHELL: the desktop gives the screen to the console terminal window and
 *        the line runs at the real shell prompt (userspace/shell.c): one
 *        line editor, one command table, one output path.
 * OSUI: chat and other panes keep the OS-UI command line. */
int osui_gui_route_line(const char *line, const char *pane) {
    if (!line) return OSUI_GUI_ROUTE_OSUI;
    while (*line == ' ' || *line == '\t') line++;
    if (scmp(line, "console") == 0 || scmp(line, "gui-exit") == 0 || scmp(line, "/console") == 0
        || is_word(line, "web") || is_word(line, "pm-ide") || is_word(line, "search")
        || is_word(line, "search-open") || sncmp(line, "/search ", 8) == 0)
        return OSUI_GUI_ROUTE_HOST;
    if (line[0] == '!' || token_ai(line)) return OSUI_GUI_ROUTE_SHELL;
    /* Lot 67: the network GET runs in the shell (same command as the console) */
    if (is_word(line, "fetch") || is_word(line, "http-get") || is_word(line, "search-engine")) return OSUI_GUI_ROUTE_SHELL;
    if (pane && scmp(pane, "shell") == 0) return OSUI_GUI_ROUTE_SHELL;
    return OSUI_GUI_ROUTE_OSUI;
}

static void seed_browser_term(void) {
    if (g_term_n > 0) return;
    osui_gui_browser_inspect();
}

static void seed_fs_term(void) {
    if (g_term_n > 0) return;
    osui_gui_vfs_inspect(0);
}

static void put_cell(uint16_t *cells, int x, int y, char ch, unsigned char attr) {
    if (x < 0 || y < 0 || x >= COLS || y >= ROWS) return;
    cells[y * COLS + x] = (uint16_t)(unsigned char)ch | ((uint16_t)attr << 8);
}

static void fill_rect(uint16_t *cells, int x, int y, int w, int h, char ch, unsigned char attr) {
    int r, c;
    for (r = y; r < y + h; r++) {
        for (c = x; c < x + w; c++) put_cell(cells, c, r, ch, attr);
    }
}

static void put_text(uint16_t *cells, int x, int y, const char *s, unsigned char attr) {
    int i = 0;
    if (!s) return;
    while (s[i]) {
        put_cell(cells, x + i, y, s[i], attr);
        i++;
    }
}

#ifndef KERNEL_TEST
static void sys_vga_blit_scene(const os_fb_scene_t *scene) {
    int rc;
    asm volatile("int $0x80" : "=a"(rc) : "a"(SYS_VGA_BLIT), "b"(scene));
    (void)rc;
}

static int sys_vga_term(void) {
    int rc;
    asm volatile("int $0x80" : "=a"(rc) : "a"(SYS_VGA_BLIT), "b"(1));
    return rc;
}

/* Lot 72: web pane images (SYS_VGA_BLIT 6); 0 clears */
static void sys_vga_webimg(const os_fb_webimg_t *a) {
    int rc;
    asm volatile("int $0x80" : "=a"(rc) : "a"(SYS_VGA_BLIT), "b"(6), "c"(a) : "memory");
    (void)rc;
}

/* UX5: viewer image (SYS_VGA_BLIT 5); 0 clears */
static void sys_vga_image(const os_fb_image_t *img) {
    int rc;
    asm volatile("int $0x80" : "=a"(rc) : "a"(SYS_VGA_BLIT), "b"(5), "c"(img) : "memory");
    (void)rc;
}

static void sys_vga_leave(void) {
    int rc;
    asm volatile("int $0x80" : "=a"(rc) : "a"(SYS_VGA_BLIT), "b"(0));
    (void)rc;
}

static int sys_getc_gui(void) {
    int c;
    /* ebx=0 keeps the blocking read. The shell passes ebx=1. */
    asm volatile("int $0x80" : "=a"(c) : "a"(SYS_GETC), "b"(0));
    return c;
}

static void serial_puts(const char *s) {
    if (!s) return;
    while (*s) {
        asm volatile("int $0x80" : : "a"(SYS_PUTC), "b"(*s));
        s++;
    }
}

static void yield_gui(void) {
    asm volatile("int $0x80" : : "a"(SYS_YIELD));
}
#else
static void sys_vga_blit_scene(const os_fb_scene_t *scene) { (void)scene; }
/* host tests: the last image sent (osui_gui_test_image) */
static const os_fb_image_t *g_test_img;
static int g_test_img_calls;
static void sys_vga_image(const os_fb_image_t *img) { g_test_img = img; g_test_img_calls++; }
static const os_fb_webimg_t *g_test_webimg;
static int g_test_webimg_calls;
static void sys_vga_webimg(const os_fb_webimg_t *a) { g_test_webimg = a; g_test_webimg_calls++; }
const os_fb_webimg_t *osui_gui_test_webimg(int *calls) { if (calls) *calls = g_test_webimg_calls; return g_test_webimg; }
const os_fb_image_t *osui_gui_test_image(int *calls) { if (calls) *calls = g_test_img_calls; return g_test_img; }
static void sys_vga_leave(void) {}
static int sys_vga_term(void) { return 0; }
static int sys_getc_gui(void) { return 0; }
static void serial_puts(const char *s) { (void)s; }
static void yield_gui(void) {}
#endif

/* Lot 72: the atlas of the fetched page goes to the compositor when its
 * generation changes; a local page or a closed browser clears it. */
static uint32_t g_webimg_gen;
static int g_webimg_sent;
static void webimg_sync(void) {
    const os_fb_webimg_t *a = 0;
    if (g_web_active && g_wops && g_wops->atlas && osui_gui_web_is_url(g_web_file)) a = g_wops->atlas();
    if (a) {
        if (g_webimg_sent && g_webimg_gen == a->gen) return;
        sys_vga_webimg(a);
        g_webimg_sent = 1;
        g_webimg_gen = a->gen;
    } else if (g_webimg_sent) {
        sys_vga_webimg(0);
        g_webimg_sent = 0;
    }
}

void osui_gui_set_input(const char *text) {
    scpy(g_input, 96, text);
    g_ilen = slen(g_input);
}

const char *osui_gui_input(void) {
    return g_input;
}

void osui_gui_ascii_row(const uint16_t *cells, int y, char *dst, int max) {
    int x;
    if (!dst || max <= 0) return;
    dst[0] = 0;
    if (!cells || y < 0 || y >= ROWS) return;
    for (x = 0; x < COLS && x < max - 1; x++) {
        char ch = (char)(cells[y * COLS + x] & 0xFF);
        if (ch < 32) ch = ' ';
        dst[x] = ch;
    }
    dst[x] = 0;
}

static void snap_add(char *dst, int max, int *pos, const char *s) {
    if (!s) return;
    while (*s && *pos < max - 1) dst[(*pos)++] = *s++;
    if (max > 0) dst[*pos] = 0;
}

static void snap_u(char *dst, int max, int *pos, unsigned v) {
    char buf[12];
    int n = 0;
    unsigned t = v;
    if (t == 0) {
        snap_add(dst, max, pos, "0");
        return;
    }
    while (t && n < 11) {
        buf[n++] = (char)('0' + (t % 10));
        t /= 10;
    }
    while (n--) {
        if (*pos < max - 1) dst[(*pos)++] = buf[n];
    }
    if (max > 0) dst[*pos] = 0;
}

void osui_gui_write_snap(char *dst, int max) {
    int p = 0;
    int i, n;
    const char *pane = osui_get_pane();
    if (!dst || max <= 0) return;
    dst[0] = 0;
    snap_add(dst, max, &p,
        "OSUI-SNAP chrome=qemu_fb display_surface=vbe_lfb llm=");
    snap_add(dst, max, &p, osui_get_stage_llm());
    snap_add(dst, max, &p, " ai=");
    snap_add(dst, max, &p, osui_get_ai_state());
    snap_add(dst, max, &p,
        " us031=true python=false phase3=false guest_html_stage=false chat_mode=");
    snap_add(dst, max, &p, osui_get_chat_mode());
    snap_add(dst, max, &p, " pane=");
    snap_add(dst, max, &p, pane && pane[0] ? pane : "none");
    snap_add(dst, max, &p, " stage=");
    snap_add(dst, max, &p, osui_get_stage_mode());
    snap_add(dst, max, &p, " kind=");
    snap_add(dst, max, &p, osui_get_stage_kind());
    snap_add(dst, max, &p, " session=");
    snap_add(dst, max, &p, osui_get_session_id());
    snap_add(dst, max, &p, " chat_x=");
    snap_u(dst, max, &p, (unsigned)osui_get_chat_x());
    snap_add(dst, max, &p, " chat_y=");
    snap_u(dst, max, &p, (unsigned)osui_get_chat_y());
    snap_add(dst, max, &p, " nmsg=");
    n = osui_msg_count();
    snap_u(dst, max, &p, (unsigned)n);
    snap_add(dst, max, &p, " nterm=");
    snap_u(dst, max, &p, (unsigned)osui_gui_term_count());
    if (osui_gui_term_count() > 0) {
        char last[80];
        osui_gui_term_at(osui_gui_term_count() - 1, last, (int)sizeof(last));
        snap_add(dst, max, &p, " term=");
        snap_add(dst, max, &p, last);
    }
    snap_add(dst, max, &p, " input=");
    snap_add(dst, max, &p, g_input);
    if (g_ide.active) {
        char row[OSUI_IDE_ROW_LEN];
        snap_add(dst, max, &p, " ide=");
        snap_add(dst, max, &p, g_ide.file);
        snap_add(dst, max, &p, " ide_ln=");
        snap_u(dst, max, &p, (unsigned)(g_ide.row + 1));
        snap_add(dst, max, &p, " ide_col=");
        snap_u(dst, max, &p, (unsigned)(g_ide.col + 1));
        snap_add(dst, max, &p, " ide_lines=");
        snap_u(dst, max, &p, (unsigned)g_ide.nlines);
        snap_add(dst, max, &p, " ide_mod=");
        snap_u(dst, max, &p, (unsigned)g_ide.modified);
        snap_add(dst, max, &p, " ide_out=");
        snap_u(dst, max, &p, (unsigned)g_ide.out_total);
        snap_add(dst, max, &p, " ide_pane=");
        snap_u(dst, max, &p, (unsigned)g_ide.out_visible);
        snap_add(dst, max, &p, " ide_tabs=");
        snap_u(dst, max, &p, (unsigned)g_ide.completions);
        snap_add(dst, max, &p, "\nOSUI-IDE status=");
        snap_add(dst, max, &p, g_ide.status);
        osui_ide_frame_row(&g_ide, 1 + g_ide.row - g_ide.top, row, (int)sizeof(row));
        snap_add(dst, max, &p, " cursor_row=");
        snap_add(dst, max, &p, row);
    }
    if (osui_get_media_active()) {
        snap_add(dst, max, &p, " media=");
        snap_add(dst, max, &p, osui_get_media_paused() ? "paused" : "playing");
        snap_add(dst, max, &p, ":");
        snap_add(dst, max, &p, osui_get_media_title());
    }
    snap_add(dst, max, &p, "\n");
    for (i = 0; i < n; i++) {
        char msg[160];
        osui_msg_at(i, msg, (int)sizeof(msg));
        snap_add(dst, max, &p, "OSUI-MSG ");
        snap_u(dst, max, &p, (unsigned)i);
        snap_add(dst, max, &p, " ");
        snap_add(dst, max, &p, msg);
        snap_add(dst, max, &p, "\n");
    }
    snap_add(dst, max, &p, "OSUI-END\n");
}

/* Live window content (osui_panes.c), refreshed about once a second. */
static const osui_pane_src_t *g_pane_src;
static char g_pane_lines[OS_FB_TERM_MAX][OSUI_PANES_LINE];
static int g_pane_n, g_pane_for = -1;
static unsigned g_pane_at;

void osui_gui_set_pane_source(const osui_pane_src_t *src) { g_pane_src = src; g_pane_for = -1; }

static int pane_live_fill(int pane) {
    unsigned now;
    if (!g_pane_src || (pane != OS_FB_PANE_STATUS && pane != OS_FB_PANE_FS && pane != OS_FB_PANE_ADMIN &&
                        pane != OS_FB_PANE_BROWSER && !osui_panes_command(pane))) return 0;
    now = g_pane_src->ticks ? g_pane_src->ticks() : 0;
    /* command panes run their command when they open (not every second) */
    if (pane != g_pane_for || (!osui_panes_command(pane) && now - g_pane_at >= 100U)) {
        g_pane_n = osui_panes_fill(pane, g_pane_src, g_pane_lines, OS_FB_TERM_MAX);
        g_pane_for = pane;
        g_pane_at = now;
    }
    return 1;
}

void osui_gui_fill_scene(os_fb_scene_t *scene) {
    int i, n;
    const char *mode = osui_get_chat_mode();
    const char *pane = osui_get_pane();
    const char *kind = osui_get_stage_kind();
    const char *stmode = osui_get_stage_mode();
    const char *sid;
    int k;
    if (!scene) return;
    image_sync();   /* UX5: viewer pixels, only when they change */
    webimg_sync();  /* Lot 72: web page images, only when they change */
    for (k = 0; k < (int)sizeof(*scene); k++) ((char *)scene)[k] = 0;
    scene->magic = OS_FB_MAGIC;
    scene->version = 8; /* 3: term_attr (pm-ide syntax classes), 4: web display list, 5: llm, 6: pane_live, 7: app windows, 8: text scale */
    {
        osui_scale_t *us = osui_gui_scale();
        int z;
        scene->ui_scale_q = us->ui_q;
        for (z = 0; z < OS_UI_Z_COUNT; z++) scene->zoom_q[z] = us->zoom[z];
        scene->scale_pad[0] = us->font_mode;
        scene->scale_pad[1] = (uint8_t)osui_scale_form_bits(us);
    }
    scene->chat_mode = (mode && mode[0] == 'f') ? OS_FB_CHAT_FLOAT : OS_FB_CHAT_CENTER;
    /* Lot 69 (US-080): the phone plan keeps the chat full screen while no
     * program window is open (an open window still floats it) */
    if (osui_gui_scale()->chat_pref && scene->chat_mode == OS_FB_CHAT_CENTER) scene->chat_mode = OS_FB_CHAT_FULL;
    scene->pane = OS_FB_PANE_NONE;
    if (pane) {
        if (scmp(pane, "browser") == 0) scene->pane = OS_FB_PANE_BROWSER;
        else if (scmp(pane, "shell") == 0) scene->pane = OS_FB_PANE_SHELL;
        else if (scmp(pane, "admin") == 0) scene->pane = OS_FB_PANE_ADMIN;
        else if (scmp(pane, "support") == 0) scene->pane = OS_FB_PANE_SUPPORT;
        else if (scmp(pane, "status") == 0) scene->pane = OS_FB_PANE_STATUS;
        else if (scmp(pane, "fs") == 0) scene->pane = OS_FB_PANE_FS;
        else if (scmp(pane, "p2p") == 0) scene->pane = OS_FB_PANE_P2P;
        else if (scmp(pane, "collab") == 0) scene->pane = OS_FB_PANE_COLLAB;
        else if (scmp(pane, "prod") == 0) scene->pane = OS_FB_PANE_PROD;
    }
    scene->stage_mode = OS_FB_STAGE_REFLECTING;
    if (stmode && stmode[0] == 'a') scene->stage_mode = OS_FB_STAGE_ACTING;
    if (stmode && stmode[0] == 'p') scene->stage_mode = OS_FB_STAGE_PRESENTING;
    scene->stage_kind = OS_FB_KIND_PLAN;
    if (kind) {
        if (scmp(kind, "circle") == 0) scene->stage_kind = OS_FB_KIND_CIRCLE;
        else if (scmp(kind, "boxes") == 0) scene->stage_kind = OS_FB_KIND_BOXES;
        else if (scmp(kind, "graph") == 0) scene->stage_kind = OS_FB_KIND_GRAPH;
        else if (scmp(kind, "tree") == 0) scene->stage_kind = OS_FB_KIND_TREE;
        else if (scmp(kind, "clock") == 0) scene->stage_kind = OS_FB_KIND_CLOCK;
        else if (scmp(kind, "sim") == 0) scene->stage_kind = OS_FB_KIND_SIM;
        else if (scmp(kind, "media") == 0) scene->stage_kind = OS_FB_KIND_MEDIA;
    }
    if (osui_get_media_active()) {
        scene->stage_kind = OS_FB_KIND_MEDIA;
        scene->reserved = (uint8_t)((1U << 7) | (osui_get_media_paused() ? (1U << 6) : 0U) | (osui_get_media_ticks() & 0x3FU));
    } else {
        scene->reserved = 0U;
    }
    scene->chat_x = (uint16_t)osui_get_chat_x();
    scene->chat_y = (uint16_t)osui_get_chat_y();
    sid = osui_get_session_id();
    scpy(scene->session, 12, sid ? sid : "s0001");
    scpy(scene->input, OS_FB_INPUT_LEN, g_input);
    scpy(scene->llm, OS_FB_LLM_LEN, osui_get_stage_llm());
    n = osui_msg_count();
    {
        /* The chat window shows the newest messages, not the oldest. */
        int first = n > OS_FB_MSG_MAX ? n - OS_FB_MSG_MAX : 0;
        n -= first;
        scene->nmsg = (uint16_t)n;
        for (i = 0; i < n; i++) osui_msg_at(first + i, scene->messages[i], OS_FB_MSG_LEN);
    }
    n = osui_gui_term_count();
    if (n > OS_FB_TERM_MAX) {
        int skip = n - OS_FB_TERM_MAX;
        for (i = 0; i < OS_FB_TERM_MAX; i++)
            osui_gui_term_at(skip + i, scene->term[i], OS_FB_TERM_LEN);
        n = OS_FB_TERM_MAX;
    } else {
        for (i = 0; i < n; i++) osui_gui_term_at(i, scene->term[i], OS_FB_TERM_LEN);
    }
    scene->nterm = (uint8_t)n;
    if (pane_live_fill(scene->pane)) {
        for (i = 0; i < g_pane_n && i < OS_FB_TERM_MAX; i++) scpy(scene->term[i], OS_FB_TERM_LEN, g_pane_lines[i]);
        scene->nterm = (uint8_t)i;
        scene->pane_live = 1;
    }
    scene->nweb = 0;
    if (g_web_active) {
        uint32_t k;
        scene->pane = OS_FB_PANE_WEB;
        scene->nweb = (uint16_t)g_web.nitems;
        scene->web_page_w = (uint16_t)g_web.perf.page_w;
        scene->web_page_h = (uint16_t)g_web.perf.page_h;
        scene->web_scroll = (uint16_t)g_web_scroll;
        scpy(scene->web_title, OS_FB_TERM_LEN, "web ");
        scpy(scene->web_title + slen(scene->web_title), OS_FB_TERM_LEN - slen(scene->web_title), g_web_file);
        if (g_web.title[0]) {
            scpy(scene->web_title + slen(scene->web_title), OS_FB_TERM_LEN - slen(scene->web_title), " - ");
            scpy(scene->web_title + slen(scene->web_title), OS_FB_TERM_LEN - slen(scene->web_title), g_web.title);
        }
        for (k = 0; k < g_web.nitems && k < OS_FB_WEB_ITEMS; k++) scene->web[k] = g_web.items[k];
        if (!g_web_mobile) scene->input[0] = 0;
    }
    scene->app_focus = (uint8_t)(g_focus < 0 ? 0xFF : g_focus);
    for (i = 0; i < OS_FB_APPS; i++)
        if (g_slot[i] && app_render(g_slot[i], &g_view)) app_to_scene(&g_view, &scene->app[i], g_slot[i]);
    if (g_ide.active) {
        /* US-050: the editor takes the whole desktop (OS_FB_PANE_IDE). */
        scene->pane = OS_FB_PANE_IDE;
        for (i = 0; i < OSUI_IDE_FRAME && i < OS_FB_TERM_MAX; i++) {
            osui_ide_frame_row(&g_ide, i, scene->term[i], OS_FB_TERM_LEN);
            (void)osui_ide_frame_attr(&g_ide, i, scene->term_attr[i], OS_FB_TERM_LEN);
        }
        scene->nterm = (uint8_t)i;
        scene->input[0] = 0;
    }
    /* Lot 71: the desktop fonts draw display bytes (CP1252, see
     * include/os_charset.h): UTF-8 from the LLM, the shell and local
     * pages is converted here, once; display text passes unchanged. */
    for (i = 0; i < (int)scene->nmsg && i < OS_FB_MSG_MAX; i++) os_text_to_display_inplace(scene->messages[i], OS_FB_MSG_LEN);
    if (!g_ide.active)
        for (i = 0; i < (int)scene->nterm && i < OS_FB_TERM_MAX; i++) os_text_to_display_inplace(scene->term[i], OS_FB_TERM_LEN);
    os_text_to_display_inplace(scene->web_title, OS_FB_TERM_LEN);
    os_text_to_display_inplace(scene->input, OS_FB_INPUT_LEN);
    /* Lot 72: a TEXT item of another glyph page (pad bits 8..10) holds
     * that page's bytes, which may look like UTF-8: it is left as is */
    for (i = 0; i < (int)scene->nweb && i < OS_FB_WEB_ITEMS; i++)
        if (scene->web[i].kind != OS_FB_WEB_TEXT || !((scene->web[i].pad >> 8) & 7u))
            os_text_to_display_inplace(scene->web[i].text, OS_FB_WEB_TEXT_LEN);
}

void osui_gui_render(uint16_t *cells) {
    fill_rect(cells, 0, 0, COLS, ROWS, ' ', ATTR_HINT);
    put_text(cells, 0, 0,
        " MOHHDY OS  gui  chrome=qemu_fb  vbe_lfb  llm=stub_echo  us031=true ",
        ATTR_TITLE);
    put_text(cells, 0, 2,
        " Bureau produit = fenetre graphique QEMU (VBE 1024x768), pas HTML.",
        ATTR_HINT);
    put_text(cells, 0, 3,
        " Cerveau = osui_runtime.c. Pas un bureau ASCII. Pas Chromium.",
        ATTR_HINT);
    put_text(cells, 0, 5,
        " make run-gui  ouvre QEMU GTK ; tapez gui apres MOHHDY>",
        ATTR_HINT);
    put_text(cells, 0, 7,
        " guest_html_stage=false  python_facade=false  display_host=false",
        ATTR_DIM);
    if (osui_get_media_active()) {
        char media_buf[96];
        scpy(media_buf, 96, " [MEDIA ");
        if (osui_get_media_paused()) {
            scpy(media_buf + slen(media_buf), 96 - slen(media_buf), "PAUSED ");
        } else {
            scpy(media_buf + slen(media_buf), 96 - slen(media_buf), "PLAY ");
        }
        scpy(media_buf + slen(media_buf), 96 - slen(media_buf), osui_get_media_title());
        scpy(media_buf + slen(media_buf), 96 - slen(media_buf), " type=");
        scpy(media_buf + slen(media_buf), 96 - slen(media_buf), osui_get_media_type());
        scpy(media_buf + slen(media_buf), 96 - slen(media_buf), " ticks=");
        /* simple num to string formatting for media ticks */
        unsigned t = osui_get_media_ticks();
        char num_buf[16];
        int np = 0;
        if (t == 0) { num_buf[np++] = '0'; }
        else {
            char rev[16]; int rp = 0;
            while (t > 0) { rev[rp++] = (char)('0' + (t % 10)); t /= 10; }
            while (rp > 0) num_buf[np++] = rev[--rp];
        }
        num_buf[np] = 0;
        scpy(media_buf + slen(media_buf), 96 - slen(media_buf), num_buf);
        scpy(media_buf + slen(media_buf), 96 - slen(media_buf), "]");
        put_text(cells, 0, 9, media_buf, ATTR_TITLE);
    }
}

/* Lot 68: window that Ctrl+'+'/'-'/'0' zooms: the page, the editor, the
 * focused app, else the open pane (shell or panel), else the chat. */
int osui_gui_zoom_target(void) {
    const char *pane = osui_get_pane();
    if (g_web_active) return OS_UI_Z_WEB;
    if (g_ide.active) return OS_UI_Z_IDE;
    if (g_focus >= 0 && g_focus < OS_FB_APPS && g_slot[g_focus]) return OS_UI_Z_APP0 + g_focus;
    if (pane && pane[0] && scmp(pane, "none") != 0) return scmp(pane, "shell") == 0 ? OS_UI_Z_SHELL : OS_UI_Z_PANE;
    return OS_UI_Z_CHAT;
}

int osui_gui_feed_key(int key, char *out, int out_max) {
    if (out && out_max > 0) out[0] = 0;
    if (key == 0) {
        osui_stage_tick(out, out_max);
        return 0;
    }
    if (key == OS_FB_EVT) { g_evt_on = 1; g_evt_len = 0; return 0; }
    if (g_evt_on) {
        if (key == '\n' || key == '\r') {
            g_evt[g_evt_len] = 0;
            g_evt_on = 0;
            osui_gui_event(g_evt, out, out_max);
        } else if (g_evt_len < (int)sizeof(g_evt) - 1) g_evt[g_evt_len++] = (char)key;
        return 0;
    }
    if (key == OS_KEY_ZOOM_IN || key == OS_KEY_ZOOM_OUT || key == OS_KEY_ZOOM_RESET) {
        /* Lot 68: Ctrl+'+' / Ctrl+'-' / Ctrl+'0' zoom the focused window */
        osui_scale_t *us = osui_gui_scale();
        int slot = osui_gui_zoom_target(), z;
        char m[80];
        z = osui_scale_zoom(us, slot, key == OS_KEY_ZOOM_IN ? 1 : key == OS_KEY_ZOOM_OUT ? -1 : 0);
        char qb[16];
        scpy(m, (int)sizeof(m), osui_scale_slot_name(slot));
        scat(m, (int)sizeof(m), " zoom ");
        os_ui_q_format(z, qb);
        scat(m, (int)sizeof(m), qb);
        scat(m, (int)sizeof(m), " effective ");
        os_ui_q_format(osui_scale_effective(us, slot), qb);
        scat(m, (int)sizeof(m), qb);
        if (us->changes != us->saved_at) (void)scale_save();
        app_out(out, out_max, "osui zoom ", m);
        if (slot == OS_UI_Z_WEB && out) {
            int l = slen(out);
            web_reflow(out + l, out_max - l);
        }
        return 0;
    }
    if (g_focus >= 0 && g_focus < OS_FB_APPS && g_slot[g_focus] && !g_ide.active && !g_web_active) {
        app_key(g_slot[g_focus], key, out, out_max);
        return 0;
    }
    if (g_web_active && !(g_web_mobile && key != OS_VGA_KEY_ESC && key != OS_VGA_KEY_UP && key != OS_VGA_KEY_DOWN)) {
        /* Lot 6: the web pane takes the desktop; arrows scroll, ESC closes.
         * Lot 9: the mobile pane lets text keys reach the input line.
         * Lot 71: a focused form field takes the keys first (Enter submits,
         * ESC leaves the field); arrows past the end change section. */
        int fk = (!g_web_mobile && web_nsections() > 0 && g_wops->form_key) ? g_wops->form_key(key) : -1;
        if (fk == 0 || fk == 2) {
            (void)web_show_section(g_wops->current ? g_wops->current() : 0, 1, 0, 0, 0);
            if (fk == 2) app_out(out, out_max, "osui web form blur", 0);
            return 0;
        }
        if (fk == 1) { (void)web_submit(-1, out, out_max); return 0; }
        if (key == OS_VGA_KEY_ESC) {
            g_web_active = 0;
            g_web_mobile = 0;
            if (out) { scpy(out, out_max, "osui web close file="); scpy(out + slen(out), out_max - slen(out), g_web_file); scpy(out + slen(out), out_max - slen(out), "\n"); }
        } else if (key == OS_VGA_KEY_DOWN && g_web_mobile) {
            if (g_web_scroll + 20 < (int)g_web.perf.page_h) g_web_scroll += 20;
        } else if (key == OS_VGA_KEY_UP && g_web_mobile) {
            g_web_scroll = g_web_scroll >= 20 ? g_web_scroll - 20 : 0;
        } else if (key == OS_VGA_KEY_DOWN || key == OS_VGA_KEY_UP) {
            /* Lot 61: the engine bounds the offset to the viewport and keeps
             * position:fixed boxes in place; the kernel subtracts it. */
            int before = g_web_scroll, cnt = web_nsections();
            g_web_scroll = we_scroll(&g_web, g_web_scroll + (key == OS_VGA_KEY_DOWN ? 20 : -20));
            if (g_web_scroll == before && cnt > 1) {
                int cur = g_wops->current ? g_wops->current() : 0;
                if (key == OS_VGA_KEY_DOWN && cur + 1 < cnt) (void)web_show_section(cur + 1, 0, 0, out, out_max);
                else if (key == OS_VGA_KEY_UP && cur > 0) (void)web_show_section(cur - 1, 0, 1, out, out_max);
            }
        }
        return 0;
    }
    if (g_ide.active) {
        /* Full screen: every key goes to the editor, ESC is its command
         * prefix and does not leave the desktop. */
        int act = osui_ide_key(&g_ide, key);
        if (act != OSUI_IDE_NONE) ide_action(act, out, out_max);
        return 0;
    }
    if (key == OS_VGA_KEY_ESC) {
        /* ESC first clears a half typed line; ESC on an empty line
         * leaves the desktop. */
        if (g_ilen > 0) {
            g_ilen = 0;
            g_input[0] = 0;
            if (out) scpy(out, out_max, "osui gui input cleared\n");
            return 0;
        }
        g_leave = 1;
        if (out) scpy(out, out_max, "osui gui exit chat_mode=center chrome=qemu_fb\n");
        return 1;
    }
    if (key == OS_VGA_KEY_LEFT) {
        osui_move_chat(-2, 0);
        return 0;
    }
    if (key == OS_VGA_KEY_RIGHT) {
        osui_move_chat(2, 0);
        return 0;
    }
    if (key == OS_VGA_KEY_UP) {
        osui_move_chat(0, -1);
        return 0;
    }
    if (key == OS_VGA_KEY_DOWN) {
        osui_move_chat(0, 1);
        return 0;
    }
    if (key == '\b') {
        if (g_ilen > 0) {
            g_ilen--;
            g_input[g_ilen] = 0;
        }
        return 0;
    }
    if (key == '\n' || key == '\r') {
        if (!g_input[0]) return 0;
        if (scmp(g_input, "console") == 0 || scmp(g_input, "gui-exit") == 0
            || scmp(g_input, "/console") == 0) {
            g_leave = 1;
            g_ilen = 0;
            g_input[0] = 0;
            if (out) scpy(out, out_max, "osui gui exit chat_mode=center chrome=qemu_fb\n");
            return 1;
        }
        if (g_mobile_op && g_input[0] == 'm' && g_input[1] == 'o' && g_input[2] == 'b' && g_input[3] == 'i' &&
            g_input[4] == 'l' && g_input[5] == 'e' && g_input[6] == '-' && g_input[7] == 'u' && g_input[8] == 'i' &&
            (g_input[9] == ' ' || g_input[9] == 0)) {
            const char *a = g_input + 9;
            while (*a == ' ') a++;
            (void)g_mobile_op(a, out, out_max);
            g_ilen = 0;
            g_input[0] = 0;
            return 0;
        }
        if (search_line(g_input, out, out_max)) {
            g_ilen = 0;
            g_input[0] = 0;
            return 0;
        }
        if (osui_gui_web_command(g_input, out, out_max)) {
            g_ilen = 0;
            g_input[0] = 0;
            return 0;
        }
        if (g_input[0] == 'w' && g_input[1] == 'e' && g_input[2] == 'b' && (g_input[3] == ' ' || g_input[3] == 0)) {
            web_open_line(g_input, out, out_max);
            g_ilen = 0;
            g_input[0] = 0;
            return 0;
        }
        if (scmp(g_input, "/browse") == 0) {
            /* UX4: the local site home, links clickable, < > history */
            web_open_line("web /web/index.html", out, out_max);
            g_ilen = 0;
            g_input[0] = 0;
            return 0;
        }
        if (tool_slash(g_input, out, out_max)) {
            g_ilen = 0;
            g_input[0] = 0;
            return 0;
        }
        if (sncmp(g_input, "/files", 6) == 0 && (g_input[6] == ' ' || g_input[6] == 0)) {
            const char *a = g_input + 6;
            while (*a == ' ') a++;
            (void)osui_gui_files_open(a, out, out_max);
            g_ilen = 0;
            g_input[0] = 0;
            return 0;
        }
        if (sncmp(g_input, "/edit", 5) == 0 && (g_input[5] == ' ' || g_input[5] == 0)) {
            const char *a = g_input + 5;
            while (*a == ' ') a++;
            (void)osui_gui_edit_open(a, out, out_max);
            g_ilen = 0;
            g_input[0] = 0;
            return 0;
        }
        if (g_input[0] == 'p' && g_input[1] == 'm' && g_input[2] == '-' && g_input[3] == 'i' &&
            g_input[4] == 'd' && g_input[5] == 'e' && (g_input[6] == ' ' || g_input[6] == 0)) {
            (void)ide_open_line(g_input, out, out_max);
            g_ilen = 0;
            g_input[0] = 0;
            return 0;
        }
        {
            char tok[32];
            int program = 0;
            const char *eval_line = g_input;
            first_token(g_input, tok, (int)sizeof(tok));
            program = osui_gui_route_line(g_input, osui_get_pane()) == OSUI_GUI_ROUTE_SHELL;
            if (g_input[0] == '!') {
                eval_line = g_input + 1;
                while (*eval_line == ' ' || *eval_line == '\t') eval_line++;
            }
            if (program) return request_terminal(eval_line, out, out_max);
            if (osui_dispatch_line(g_input, out, out_max) != 0 && out
                && sncmp(out, "osui error=commande inconnue", 28) == 0
                && scmp(osui_get_pane(), "browser") == 0 && g_input[0] != '/') {
                /* Lot 69: Browser-OS address bar: an URL opens, words search
                 * (web_open_line falls back to search: for non URLs) */
                static char wl[sizeof(g_input) + 8];
                int words = wsearch_is_query(g_input);
                scpy(wl, (int)sizeof(wl), words ? "search " : "web ");
                scat(wl, (int)sizeof(wl), g_input);
                out[0] = 0;
                if (!words || !search_line(wl, out, out_max)) web_open_line(wl, out, out_max);
                g_ilen = 0;
                g_input[0] = 0;
                return 0;
            }
            if (out && sncmp(out, "osui error=commande inconnue", 28) == 0) {
                /* The error used to reach the serial line only: say it
                 * in the chat window too. */
                static char note[OS_FB_MSG_LEN]; /* off the GUI stack */
                scpy(note, (int)sizeof(note), "commande inconnue: ");
                scat(note, (int)sizeof(note), tok);
                scat(note, (int)sizeof(note), " - ai <question> ou /help");
                osui_note(note);
            }
            /* "/shell", the Shell icon or menu: the Shell is the console. */
            if (scmp(osui_get_pane(), "shell") == 0) return request_terminal("", out, out_max);
            if (scmp(osui_get_pane(), "browser") == 0) seed_browser_term();
            else if (scmp(osui_get_pane(), "fs") == 0) seed_fs_term();
        }
        if (osui_gui_should_leave()) {
            osui_gui_ack_leave();
            g_leave = 1;
            g_ilen = 0;
            g_input[0] = 0;
            return 1;
        }
        g_ilen = 0;
        g_input[0] = 0;
        return 0;
    }
    if (key >= 32 && key <= 126) {
        if (g_ilen < 90) {
            g_input[g_ilen++] = (char)key;
            g_input[g_ilen] = 0;
        }
        return 0;
    }
    return 0;
}

static void emit_snap(void) {
    static char snap[OSUI_SNAP_MAX];
    osui_gui_write_snap(snap, OSUI_SNAP_MAX);
    serial_puts(snap);
}

void osui_gui_run(void) {
    static os_fb_scene_t scene;
    static char out[OSUI_OUT_MAX];
    int key;
    int stop;

    g_leave = 0;
    g_ilen = 0;
    g_idle = 0;
    g_input[0] = 0;
    g_ide.active = 0;
    g_web_active = 0;
    g_web_mobile = 0;
    g_term_req = 0;
    g_pending[0] = 0;
    g_focus = -1;
    g_evt_on = 0;
    osui_gui_term_reset();
    osui_gui_ack_enter();
    serial_puts("osui gui live chrome=qemu_fb display_surface=vbe_lfb us031_complete=false\n");
    osui_gui_fill_scene(&scene);
    sys_vga_blit_scene(&scene);
    emit_snap();

    while (!g_leave) {
        key = sys_getc_gui();
        out[0] = 0;
        if ((key == '\n' || key == '\r') && !g_evt_on && g_focus < 0) {
            serial_puts("osui gui line=");
            serial_puts(g_input);
            serial_puts("\n");
        }
        stop = osui_gui_feed_key(key, out, OSUI_OUT_MAX);
        if (out[0]) serial_puts(out);
        /* Lot 49: the kernel sends a window event (resize, click, drag) as
         * a line of characters; repainting the whole scene for each of
         * them made a maximize show the old grid for about 2 s. Paint
         * once, when the event line is complete. */
        if (g_evt_on && key != 0 && !stop) continue;
        osui_gui_fill_scene(&scene);
        sys_vga_blit_scene(&scene);
        if (key != 0 || (g_idle++ % 8) == 0) emit_snap();
        if (stop) break;
        if (key == 0 && osui_gui_idle(out, OSUI_OUT_MAX)) {
            if (out[0]) serial_puts(out);
            osui_gui_fill_scene(&scene);
            sys_vga_blit_scene(&scene);
            continue;
        }
        if (key == 0) yield_gui();
    }
    osui_gui_closed();
    if (g_term_req && sys_vga_term() == 0) {
        serial_puts("osui gui exit chrome=vbe_term prompt=MOHHDY>\n");
        return;
    }
    g_term_req = 0;
    g_pending[0] = 0;
    sys_vga_leave();
    serial_puts("osui gui exit chrome=text prompt=MOHHDY>\n");
}
