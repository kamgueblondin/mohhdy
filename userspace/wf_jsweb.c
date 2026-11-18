/* wf_jsweb.c - Lot 113: js and web-js commands and the page pass glue
 * (see wf_jsweb.h). Freestanding: no libc. */
#include "wf_jsweb.h"
#include "wf_js.h"
#include "wf_jsdom.h"
#include "umalloc.h"

static int g_on;
static js_vm_t *g_repl;
static void *g_repl_mem, *g_page_mem;
static char *g_page_out;
static int g_page_out_max;
static int g_pages, g_last_valid, g_last_rc;
static wf_jspage_stats_t g_last;

static int ws_len(const char *s) { int n = 0; while (s[n]) n++; return n; }

static void ws_cat(char *d, int max, const char *s) {
    int n = ws_len(d), i;
    for (i = 0; s[i] && n < max - 1; i++) d[n++] = s[i];
    if (max > 0) d[n < max ? n : max - 1] = 0;
}

static void ws_catn(char *d, int max, const char *s, int len) {
    int n = ws_len(d), i;
    for (i = 0; i < len && n < max - 1; i++) d[n++] = s[i];
    if (max > 0) d[n < max ? n : max - 1] = 0;
}

static void ws_cat_u(char *d, int max, unsigned v) {
    char b[12];
    int i = 11;
    b[i] = 0;
    do { b[--i] = (char)('0' + v % 10u); v /= 10u; } while (v && i > 0);
    ws_cat(d, max, b + i);
}

static void ws_cat_i(char *d, int max, int v) {
    if (v < 0) { ws_cat(d, max, "-"); v = -v; }
    ws_cat_u(d, max, (unsigned)v);
}

static int word_is(const char *s, const char *w) {
    int i;
    for (i = 0; w[i]; i++) if (s[i] != w[i]) return 0;
    return s[i] == 0 || s[i] == ' ' || s[i] == '\t';
}

static const char *skip_ws(const char *s) { while (*s == ' ' || *s == '\t') s++; return s; }

int wf_jsweb_enabled(void) { return g_on; }
void wf_jsweb_set(int on) { g_on = on ? 1 : 0; }

/* each log line as "<prefix><line>" */
static void put_log(char *out, int out_max, const char *pre, const char *log, int len) {
    int i = 0;
    while (i < len) {
        int j = i;
        while (j < len && log[j] != '\n') j++;
        ws_cat(out, out_max, pre);
        ws_catn(out, out_max, log + i, j - i);
        ws_cat(out, out_max, "\n");
        i = j + 1;
    }
}

static int repl_ready(char *out, int out_max) {
    if (g_repl) return 1;
    if (!g_repl_mem) g_repl_mem = umalloc(WF_JSWEB_REPL_ARENA);
    if (!g_repl_mem) { ws_cat(out, out_max, "osui js error=no_memory\n"); return 0; }
    g_repl = (js_vm_t *)umalloc((unsigned)js_vm_size());
    if (!g_repl) { ws_cat(out, out_max, "osui js error=no_memory\n"); return 0; }
    if (js_init(g_repl, g_repl_mem, WF_JSWEB_REPL_ARENA) != 0) {
        ufree(g_repl);
        g_repl = 0;
        ws_cat(out, out_max, "osui js error=init\n");
        return 0;
    }
    return 1;
}

static void cmd_js(const char *code, char *out, int out_max) {
    static char disp[1024];
    const char *lg;
    int rc, ln;
    jv v;
    if (!*code) {
        ws_cat(out, out_max, "osui js usage: js CODE (a JavaScript subset, no DOM; web-js for pages)\n");
        if (g_repl) {
            ws_cat(out, out_max, "osui js realm mem ");
            ws_cat_u(out, out_max, js_mem_used(g_repl));
            ws_cat(out, out_max, "/");
            ws_cat_u(out, out_max, js_mem_size(g_repl));
            ws_cat(out, out_max, "\n");
        }
        return;
    }
    if (!repl_ready(out, out_max)) return;
    js_log_clear(g_repl);
    js_set_budget(g_repl, WF_JSWEB_REPL_BUDGET);
    rc = js_eval(g_repl, code, ws_len(code), &v);
    lg = js_log_text(g_repl, &ln);
    put_log(out, out_max, "osui js log: ", lg, ln);
    if (rc == JS_OK) {
        int n = js_display(g_repl, v, disp, (int)sizeof(disp) - 1, 1);
        disp[n < 0 ? 0 : n] = 0;
        ws_cat(out, out_max, "osui js => ");
        ws_cat(out, out_max, disp);
        ws_cat(out, out_max, "\n");
    } else {
        ws_cat(out, out_max, rc == JS_THROWN ? "osui js error=thrown " : "osui js error=aborted ");
        ws_cat(out, out_max, js_last_error(g_repl));
        ws_cat(out, out_max, "\n");
    }
    ws_cat(out, out_max, "osui js steps ");
    ws_cat_u(out, out_max, js_steps_used(g_repl));
    ws_cat(out, out_max, " mem ");
    ws_cat_u(out, out_max, js_mem_used(g_repl));
    ws_cat(out, out_max, "/");
    ws_cat_u(out, out_max, js_mem_size(g_repl));
    ws_cat(out, out_max, "\n");
    /* an exhausted realm cannot recover (no GC): start a new one next time */
    if (rc == JS_ABORTED && js_mem_used(g_repl) + 65536u >= js_mem_size(g_repl)) {
        ufree(g_repl);
        g_repl = 0;
        ws_cat(out, out_max, "osui js realm reset (arena full)\n");
    }
}

static void stats_line(char *out, int out_max) {
    ws_cat(out, out_max, "osui web-js ");
    ws_cat(out, out_max, g_on ? "on" : "off");
    ws_cat(out, out_max, " pages ");
    ws_cat_i(out, out_max, g_pages);
    if (g_last_valid) {
        ws_cat(out, out_max, " last rc ");
        ws_cat_i(out, out_max, g_last_rc);
        ws_cat(out, out_max, " scripts ");
        ws_cat_i(out, out_max, g_last.scripts);
        ws_cat(out, out_max, " ran ");
        ws_cat_i(out, out_max, g_last.ran);
        ws_cat(out, out_max, " failed ");
        ws_cat_i(out, out_max, g_last.failed);
        ws_cat(out, out_max, " external ");
        ws_cat_i(out, out_max, g_last.external);
        ws_cat(out, out_max, " ext_ran ");
        ws_cat_i(out, out_max, g_last.ext_ran);
        ws_cat(out, out_max, " ext_failed ");
        ws_cat_i(out, out_max, g_last.ext_failed);
        ws_cat(out, out_max, " ext_skipped ");
        ws_cat_i(out, out_max, g_last.ext_skipped);
        ws_cat(out, out_max, " other ");
        ws_cat_i(out, out_max, g_last.other_type);
        ws_cat(out, out_max, " dynamic ");
        ws_cat_i(out, out_max, g_last.dynamic);
        ws_cat(out, out_max, " microtasks ");
        ws_cat_i(out, out_max, g_last.microtasks);
        ws_cat(out, out_max, " rejections ");
        ws_cat_i(out, out_max, g_last.rejections);
        ws_cat(out, out_max, " net ");
        ws_cat_i(out, out_max, g_last.net_requests);
        ws_cat(out, out_max, " net_failed ");
        ws_cat_i(out, out_max, g_last.net_failed);
        ws_cat(out, out_max, " layout ");
        ws_cat_i(out, out_max, g_last.layout_reads);
        ws_cat(out, out_max, " live ");
        ws_cat_i(out, out_max, g_last.live_events);
        ws_cat(out, out_max, " marks ");
        ws_cat_i(out, out_max, g_last.marks);
        ws_cat(out, out_max, " alive ");
        ws_cat_i(out, out_max, wf_jspage_alive());
        ws_cat(out, out_max, " writes ");
        ws_cat_i(out, out_max, g_last.writes);
        ws_cat(out, out_max, " timers ");
        ws_cat_i(out, out_max, g_last.timers);
        ws_cat(out, out_max, " events ");
        ws_cat_i(out, out_max, g_last.events);
        ws_cat(out, out_max, " nodes ");
        ws_cat_i(out, out_max, g_last.nodes);
        ws_cat(out, out_max, " truncated ");
        ws_cat_i(out, out_max, g_last.truncated);
        ws_cat(out, out_max, " steps ");
        ws_cat_u(out, out_max, g_last.steps);
        ws_cat(out, out_max, " mem ");
        ws_cat_u(out, out_max, g_last.mem);
        if (g_last.first_err[0]) {
            ws_cat(out, out_max, " error=");
            ws_cat(out, out_max, g_last.first_err);
        }
    }
    ws_cat(out, out_max, "\n");
}

int wf_jsweb_command(const char *line, char *out, int out_max) {
    const char *a;
    if (!line || !out || out_max <= 0) return 0;
    line = skip_ws(line);
    if (word_is(line, "js")) {
        cmd_js(skip_ws(line + 2), out, out_max);
        return 1;
    }
    if (!word_is(line, "web-js")) return 0;
    a = skip_ws(line + 6);
    if (word_is(a, "on") || word_is(a, "off")) {
        g_on = a[1] == 'n';
        ws_cat(out, out_max, g_on ? "osui web-js on (inline scripts of the next pages run, bounded)\n"
                                  : "osui web-js off (pages are shown without running scripts)\n");
        return 1;
    }
    if (!*a || word_is(a, "status")) { stats_line(out, out_max); return 1; }
    if (word_is(a, "log")) {
        int ln;
        const char *lg = wf_jspage_log(&ln);
        if (!g_last_valid || ln <= 0) ws_cat(out, out_max, "osui web-js log empty\n");
        else put_log(out, out_max, "osui web-js log: ", lg, ln);
        return 1;
    }
    if (word_is(a, "reset")) {
        if (g_repl) { ufree(g_repl); g_repl = 0; }
        ws_cat(out, out_max, "osui js realm reset\n");
        return 1;
    }
    if (word_is(a, "click") || word_is(a, "key") || word_is(a, "input") || word_is(a, "focus") || word_is(a, "blur") ||
        word_is(a, "submit") || word_is(a, "event") || word_is(a, "clicks")) {
        /* Lot 114: reached only without the shell page (shell_page_js) */
        ws_cat(out, out_max, "osui web-js event error=no_page (open a page with web-js on)\n");
        return 1;
    }
    ws_cat(out, out_max, "osui web-js usage: web-js on|off|status|log|reset|click|key|input|focus|blur|submit|event|clicks\n");
    return 1;
}

int wf_jsweb_page(char *body, int n, int max, const char *url) {
    int r, i;
    if (!g_on || !body || n <= 0 || max <= 1) return n;
    if (!g_page_mem) g_page_mem = umalloc(WF_JSWEB_PAGE_ARENA);
    if (g_page_out_max < max) {
        if (g_page_out) ufree(g_page_out);
        g_page_out = (char *)umalloc((unsigned)max);
        g_page_out_max = g_page_out ? max : 0;
    }
    if (!g_page_mem || !g_page_out) return n;
    wf_jspage_close();
    r = wf_jspage_run(g_page_mem, WF_JSWEB_PAGE_ARENA, body, n, url, g_page_out, max, &g_last);
    g_last_rc = r;
    if (r == 0) { g_last_valid = 0; return n; }
    g_last_valid = 1;
    g_pages++;
    if (r < 0 || r >= max - 1) return n;     /* failed or cut: keep the page as it came */
    for (i = 0; i < r; i++) body[i] = g_page_out[i];
    body[r] = 0;
    return r;
}

int wf_jsweb_event(const char *type, const char *target, const char *arg, char *body, int max) {
    int r, i;
    if (!g_on || !g_page_out || !body || max <= 1) return -1;
    r = wf_jspage_event(type, target, arg, g_page_out, g_page_out_max < max ? g_page_out_max : max, &g_last);
    if (r < 0) return r;
    if (r >= max - 1) return -3;
    for (i = 0; i < r; i++) body[i] = g_page_out[i];
    body[r] = 0;
    return r;
}

int wf_jsweb_clickables(char *out, int out_max) { return wf_jspage_clickables(out, out_max); }
int wf_jsweb_wants_keys(void) { return g_on && wf_jspage_wants_keys(); }

void wf_jsweb_event_note(char *out, int out_max) {
    if (!out) return;
    ws_cat(out, out_max, " events ");
    ws_cat_i(out, out_max, g_last.events);
    ws_cat(out, out_max, " live ");
    ws_cat_i(out, out_max, g_last.live_events);
    ws_cat(out, out_max, " timers ");
    ws_cat_i(out, out_max, g_last.timers);
    ws_cat(out, out_max, " microtasks ");
    ws_cat_i(out, out_max, g_last.microtasks);
    ws_cat(out, out_max, " marks ");
    ws_cat_i(out, out_max, g_last.marks);
    ws_cat(out, out_max, " failed ");
    ws_cat_i(out, out_max, g_last.failed);
    ws_cat(out, out_max, " mem ");
    ws_cat_u(out, out_max, g_last.mem);
}

void wf_jsweb_note(char *out, int out_max) {
    if (!g_on || !g_last_valid || !out) return;
    ws_cat(out, out_max, " js scripts=");
    ws_cat_i(out, out_max, g_last.scripts);
    ws_cat(out, out_max, " ran=");
    ws_cat_i(out, out_max, g_last.ran);
    ws_cat(out, out_max, " failed=");
    ws_cat_i(out, out_max, g_last.failed);
}
