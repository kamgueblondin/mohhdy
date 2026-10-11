/* userspace/shell_prod.c - prod-* shell commands (Phase 8, docs/production.md).
 * Probes use existing syscalls only (meminfo, ps, task metrics, ticks, files). */
#include "prod.h"
#include "../include/os_syscalls.h"

void print_string(const char* s);
int sys_listdir(const char* path, os_dirent_t* out, int max_n);
int sys_readfile(const char* path, char* buf, int max);
int sys_writefile(const char* path, const char* buf, int n);
int sys_mkdir(const char* path);
int sys_meminfo(os_meminfo_t* info);
int sys_ps(os_proc_t* out, int max_n);
int sys_task_metrics(int pid, os_task_metrics_t* out);
unsigned int sys_ticks(void);
int spawn(const char* path, char* argv[]);
int sys_kill_pid(int pid);

#define SLOTS 3
static prod_metrics_t g_m;
static prod_alerts_t g_a;
static prod_archive_t g_arch[SLOTS];
static char g_dir[SLOTS][64];
static prod_baseline_t g_base;
static prod_feedback_t g_fb;
static prod_usage_t g_use;
static uint32_t g_custom, g_run_prev, g_tick_prev;
static int g_init;
#define POOL_MAX 3
static int g_pool[POOL_MAX], g_npool;
static prod_scaler_t g_sc;
static uint32_t g_sc_t;
static int g_sc_on;
static char g_out[4096];
static char g_buf[4096];
static char g_tmp[4096];

static int sys0(int nr) { int r; asm volatile("int $0x80" : "=a"(r) : "a"(nr) : "memory"); return r; }
static int s_len(const char* s) { int n = 0; while (s && s[n]) n++; return n; }
static int s_eq(const char* a, const char* b) { while (*a && *a == *b) { a++; b++; } return *a == *b; }
static int s_pre(const char* s, const char* p) { while (*p && *s == *p) { s++; p++; } return *p == 0; }
static void s_copy(char* d, const char* s, int cap) { int i = 0; while (s && s[i] && i < cap - 1) { d[i] = s[i]; i++; } d[i] = 0; }
static void s_cat(char* d, const char* s, int cap) { int n = s_len(d), i = 0; while (s && s[i] && n < cap - 1) d[n++] = s[i++]; d[n] = 0; }
static void cat_u(char* d, uint32_t v, int cap) {
    char t[11], r[12]; int n = 0, i = 0;
    do { t[n++] = (char)('0' + v % 10U); v /= 10U; } while (v);
    while (n) r[i++] = t[--n];
    r[i] = 0; s_cat(d, r, cap);
}
static void cat_i(char* d, int32_t v, int cap) { if (v < 0) { s_cat(d, "-", cap); v = -v; } cat_u(d, (uint32_t)v, cap); }
static void cat_hex(char* d, uint32_t v, int cap) {
    char t[9]; int i;
    for (i = 7; i >= 0; i--) { t[i] = "0123456789abcdef"[v & 15U]; v >>= 4; }
    t[8] = 0; s_cat(d, t, cap);
}
static const char* skip(const char* s) { while (*s == ' ') s++; return s; }
static const char* word(const char* s, char* w, int cap) {
    int n = 0;
    s = skip(s);
    while (*s && *s != ' ') { if (n < cap - 1) w[n++] = *s; s++; }
    w[n] = 0;
    return skip(s);
}
static int to_u(const char* s, uint32_t* v) {
    uint32_t r = 0; int any = 0;
    while (*s >= '0' && *s <= '9') { r = r * 10U + (uint32_t)(*s++ - '0'); any = 1; if (r > 100000000U) return -1; }
    if (!any || *s) return -1;
    *v = r; return 0;
}
static void out(const char* s) { print_string(s); }
static void line(const char* a, uint32_t v, const char* b) { char t[160]; t[0] = 0; s_cat(t, a, 160); cat_u(t, v, 160); s_cat(t, b, 160); out(t); }
static void join(char* d, const char* dir, const char* name, int cap) {
    d[0] = 0; s_cat(d, dir, cap);
    if (s_len(d) == 0 || d[s_len(d) - 1] != '/') s_cat(d, "/", cap);
    s_cat(d, name, cap);
}

static const char* k_cmds[] = {
    "prod-sample", "prod-inject", "prod-metrics", "prod-predict", "prod-alert-add", "prod-alert-del", "prod-alerts",
    "prod-log-append", "prod-log-analyze", "prod-backup", "prod-backups", "prod-backup-verify", "prod-backup-corrupt",
    "prod-restore", "prod-manifest", "prod-deploy", "prod-rollback", "prod-scale-sim", "prod-scale-run", "prod-scale-status", "prod-scale-stop", "prod-integrity", "prod-bench", "prod-diag",
    "prod-tutorial", "prod-feedback", "prod-usage", "prod-roadmap", 0
};
int shell_prod_is(const char* l) {
    char w[24]; int i;
    word(l, w, (int)sizeof(w));
    for (i = 0; k_cmds[i]; i++) if (s_eq(w, k_cmds[i])) return 1;
    return 0;
}
/* business metrics (US-119): every shell command word is counted */
void shell_prod_count(const char* l) {
    char w[20];
    word(l, w, (int)sizeof(w));
    if (w[0]) prod_usage_hit(&g_use, w);
}

static void init(void) {
    if (g_init) return;
    g_init = 1;
    prod_metrics_init(&g_m); prod_alerts_init(&g_a);
}

/* ------------------------------------------------------------ sampling */
static uint32_t others_run_ticks(void) {
    static os_proc_t ps[16];
    os_task_metrics_t tm;
    int n = sys_ps(ps, 16), i;
    uint32_t sum = 0;
    for (i = 0; i < n; i++) if (sys_task_metrics(ps[i].pid, &tm) == 0) sum += tm.run_ticks;
    return sum;
}
static void take(prod_sample_t* s) {
    os_meminfo_t mi;
    static os_proc_t ps[16];
    uint32_t now = sys_ticks(), run = others_run_ticks();
    s->tick = now;
    s->v[PM_MEM_USED] = s->v[PM_MEM_FREE] = 0;
    if (sys_meminfo(&mi) == 0) { s->v[PM_MEM_FREE] = mi.free_pages * 4U; s->v[PM_MEM_USED] = (mi.total_pages - mi.free_pages) * 4U; }
    s->v[PM_PROCS] = (uint32_t)(sys_ps(ps, 16) > 0 ? sys_ps(ps, 16) : 0);
    s->v[PM_BUSY] = 0;
    if (g_tick_prev && now > g_tick_prev && run >= g_run_prev) {
        uint32_t b = (run - g_run_prev) * 100U / (now - g_tick_prev);
        s->v[PM_BUSY] = b > 100U ? 100U : b;
    }
    g_run_prev = run; g_tick_prev = now;
    s->v[PM_CUSTOM] = g_custom;
}
static void push(const prod_sample_t* s) {
    int ch;
    prod_metrics_push(&g_m, s);
    ch = prod_alerts_eval(&g_a, s);
    if (ch) {
        int i, k = (g_a.ev_head - ch + PROD_EVENTS) % PROD_EVENTS;
        for (i = 0; i < ch; i++) { out("prod-alert "); out(g_a.ev[(k + i) % PROD_EVENTS].text); out("\n"); }
    }
}
static void pause(uint32_t ticks) { uint32_t t0 = sys_ticks(); while (sys_ticks() - t0 < ticks) (void)sys0(SYS_YIELD); }

/* ------------------------------------------------------------- files */
static int read_all(const char* p, char* b, int cap) { int n = sys_readfile(p, b, cap - 1); if (n < 0) return n; b[n] = 0; return n; }
static int rd_cb(void* ctx, const char* p, char* b, int cap) { (void)ctx; return sys_readfile(p, b, cap); }
static int slot_of(const char* name, int create) {
    int i, free_slot = -1;
    for (i = 0; i < SLOTS; i++) {
        if (g_arch[i].label[0] && s_eq(g_arch[i].label, name)) return i;
        if (!g_arch[i].label[0] && free_slot < 0) free_slot = i;
    }
    if (!create) return -1;
    if (free_slot < 0) free_slot = 0; /* oldest slot reused */
    return free_slot;
}
static int backup_dir(const char* name, const char* dir) {
    static os_dirent_t ents[32];
    char path[96];
    int n = sys_listdir(dir, ents, 32), i, s;
    if (n < 0 || s_len(name) >= 16 || s_len(dir) >= 64) return -1;
    s = slot_of(name, 1);
    prod_archive_init(&g_arch[s], name, sys_ticks());
    s_copy(g_dir[s], dir, 64);
    for (i = 0; i < n; i++) {
        int len;
        if (ents[i].flags == OS_DIRENT_DIR) continue;
        join(path, dir, ents[i].name, 96);
        len = sys_readfile(path, g_buf, (int)sizeof(g_buf));
        if (len < 0) continue;
        if (prod_archive_add(&g_arch[s], path, g_buf, (uint32_t)len) < 0) { g_arch[s].label[0] = 0; return -2; }
    }
    return s;
}
static int restore_slot(int s) {
    int i, bad, n = 0;
    if (prod_archive_verify(&g_arch[s], &bad) != 0) return -1;
    for (i = 0; i < g_arch[s].n; i++) {
        prod_arch_entry_t* e = &g_arch[s].e[i];
        if (sys_writefile(e->path, (const char*)g_arch[s].data + e->off, (int)e->len) < 0) return -2;
        n++;
    }
    return n;
}

/* ------------------------------------------------------------- bench */
static void bench(void) {
    static uint8_t a[16384], b[16384];
    volatile uint32_t acc = 0;
    uint32_t t0, t, i, k, iters;
    char tl[160];
    /* integer ALU */
    t0 = sys_ticks(); iters = 0;
    while ((t = sys_ticks() - t0) < 20U) { for (i = 0; i < 20000U; i++) acc = acc * 1664525U + 1013904223U; iters += 20000U; }
    tl[0] = 0; s_cat(tl, "prod-bench alu ops_per_s ", 160); cat_u(tl, iters / t * 100U, 160); s_cat(tl, "\n", 160); out(tl);
    /* memory copy */
    t0 = sys_ticks(); iters = 0;
    while ((t = sys_ticks() - t0) < 20U) { for (k = 0; k < 4; k++) for (i = 0; i < sizeof(a); i++) b[i] = a[i] ^ (uint8_t)k; iters += 4U; }
    tl[0] = 0; s_cat(tl, "prod-bench memcpy kib_per_s ", 160); cat_u(tl, iters * 16U / t * 100U, 160); s_cat(tl, "\n", 160); out(tl);
    /* syscall round trip */
    t0 = sys_ticks(); iters = 0;
    while ((t = sys_ticks() - t0) < 20U) { for (i = 0; i < 200U; i++) (void)sys_ticks(); iters += 200U; }
    tl[0] = 0; s_cat(tl, "prod-bench syscall calls_per_s ", 160); cat_u(tl, iters / t * 100U, 160); s_cat(tl, "\n", 160); out(tl);
    /* small file write + read */
    (void)sys_mkdir("/tmp");
    t0 = sys_ticks(); iters = 0;
    while ((t = sys_ticks() - t0) < 20U) {
        for (i = 0; i < 8U; i++) { (void)sys_writefile("/tmp/bench.dat", (const char*)a, 512); (void)sys_readfile("/tmp/bench.dat", (char*)b, 512); }
        iters += 8U;
    }
    tl[0] = 0; s_cat(tl, "prod-bench file512 rw_per_s ", 160); cat_u(tl, iters / t * 100U, 160); s_cat(tl, "\n", 160); out(tl);
    line("prod-bench ok checksum ", acc & 0xFFU, "\n");
}

static void show_alerts(void) {
    int i;
    for (i = 0; i < PROD_RULES; i++) if (g_a.r[i].used) {
        prod_rule_t* r = &g_a.r[i];
        g_tmp[0] = 0;
        s_cat(g_tmp, "prod-rule ", 4096); s_cat(g_tmp, r->name, 4096); s_cat(g_tmp, " ", 4096);
        s_cat(g_tmp, prod_metric_name(r->metric), 4096); s_cat(g_tmp, r->above ? " > " : " < ", 4096); cat_u(g_tmp, r->threshold, 4096);
        s_cat(g_tmp, " for ", 4096); cat_u(g_tmp, (uint32_t)r->for_n, 4096);
        s_cat(g_tmp, r->firing ? " state firing\n" : " state ok\n", 4096);
        out(g_tmp);
    }
    for (i = 0; i < g_a.ev_n; i++) {
        prod_event_t* e = &g_a.ev[(g_a.ev_head - g_a.ev_n + i + PROD_EVENTS) % PROD_EVENTS];
        g_tmp[0] = 0; s_cat(g_tmp, "prod-event t=", 4096); cat_u(g_tmp, e->tick, 4096); s_cat(g_tmp, " ", 4096); s_cat(g_tmp, e->text, 4096); s_cat(g_tmp, "\n", 4096);
        out(g_tmp);
    }
    g_tmp[0] = 0; s_cat(g_tmp, "prod-alerts ok fired ", 4096); cat_u(g_tmp, g_a.fired_total, 4096);
    s_cat(g_tmp, " resolved ", 4096); cat_u(g_tmp, g_a.resolved_total, 4096);
    s_cat(g_tmp, " deduped ", 4096); cat_u(g_tmp, g_a.suppressed, 4096); s_cat(g_tmp, "\n", 4096);
    out(g_tmp);
}

int shell_prod_line(const char* l) {
    char cmd[24], a[64], b[64], c[64], d[64];
    const char* rest = word(l, cmd, (int)sizeof(cmd));
    uint32_t x, y;
    init();
    if (s_eq(cmd, "prod-sample")) {
        uint32_t n = 1, gap = 10, i;
        prod_sample_t s;
        rest = word(rest, a, 64); rest = word(rest, b, 64);
        if (a[0] && (to_u(a, &n) || n == 0 || n > 64)) { out("prod-sample error count 1..64\n"); return 1; }
        if (b[0] && (to_u(b, &gap) || gap > 500)) { out("prod-sample error gap 0..500 ticks\n"); return 1; }
        for (i = 0; i < n; i++) { if (i) pause(gap); take(&s); push(&s); }
        g_out[0] = 0; s_cat(g_out, "prod-sample ok n ", 4096); cat_u(g_out, (uint32_t)g_m.n, 4096);
        s_cat(g_out, " mem_used ", 4096); cat_u(g_out, s.v[PM_MEM_USED], 4096);
        s_cat(g_out, " mem_free ", 4096); cat_u(g_out, s.v[PM_MEM_FREE], 4096);
        s_cat(g_out, " procs ", 4096); cat_u(g_out, s.v[PM_PROCS], 4096);
        s_cat(g_out, " busy ", 4096); cat_u(g_out, s.v[PM_BUSY], 4096);
        s_cat(g_out, " custom ", 4096); cat_u(g_out, s.v[PM_CUSTOM], 4096); s_cat(g_out, "\n", 4096);
        out(g_out); return 0;
    }
    if (s_eq(cmd, "prod-inject")) { /* set the application metric, then sample */
        prod_sample_t s;
        rest = word(rest, a, 64);
        if (to_u(a, &x)) { out("prod-inject error value\n"); return 1; }
        g_custom = x; take(&s); push(&s);
        line("prod-inject ok custom ", x, "\n"); return 0;
    }
    if (s_eq(cmd, "prod-metrics")) {
        int i;
        prod_stat_t st;
        if (g_m.n == 0) { out("prod-metrics error no samples (prod-sample first)\n"); return 1; }
        for (i = 0; i < PM_COUNT; i++) {
            prod_metric_stat(&g_m, i, &st);
            g_out[0] = 0; s_cat(g_out, "prod-metric ", 4096); s_cat(g_out, prod_metric_name(i), 4096);
            s_cat(g_out, " last ", 4096); cat_u(g_out, st.last, 4096); s_cat(g_out, " min ", 4096); cat_u(g_out, st.min, 4096);
            s_cat(g_out, " avg ", 4096); cat_u(g_out, st.avg, 4096); s_cat(g_out, " max ", 4096); cat_u(g_out, st.max, 4096); s_cat(g_out, "\n", 4096);
            out(g_out);
        }
        line("prod-metrics ok samples ", (uint32_t)g_m.n, ""); line(" total ", g_m.total, "\n");
        return 0;
    }
    if (s_eq(cmd, "prod-predict")) {
        int32_t slope; uint32_t left; int id;
        rest = word(rest, a, 64); rest = word(rest, b, 64);
        id = prod_metric_id(a);
        if (id < 0 || to_u(b, &x)) { out("prod-predict error usage: prod-predict METRIC LIMIT\n"); return 1; }
        if (prod_predict(&g_m, id, x, &slope, &left) != 0) { out("prod-predict error need 4 samples over time\n"); return 1; }
        g_out[0] = 0; s_cat(g_out, "prod-predict ", 4096); s_cat(g_out, a, 4096);
        s_cat(g_out, " slope_milli_per_tick ", 4096); cat_i(g_out, slope, 4096);
        if (left == 0xFFFFFFFFU) s_cat(g_out, " limit not approaching\n", 4096);
        else { s_cat(g_out, " reaches ", 4096); cat_u(g_out, x, 4096); s_cat(g_out, " in_ticks ", 4096); cat_u(g_out, left, 4096);
               s_cat(g_out, left < 6000U ? " maintenance soon\n" : " ok\n", 4096); }
        out(g_out); return 0;
    }
    if (s_eq(cmd, "prod-alert-add")) {
        int id, rc;
        rest = word(rest, a, 64); rest = word(rest, b, 64); rest = word(rest, c, 64); rest = word(rest, d, 64);
        id = prod_metric_id(b);
        if (id < 0 || (!s_eq(c, "above") && !s_eq(c, "below")) || to_u(d, &x) || to_u(rest[0] ? rest : "1", &y)) {
            out("prod-alert-add error usage: NAME METRIC above|below THRESHOLD [FOR]\n"); return 1;
        }
        rc = prod_alert_add(&g_a, a, id, s_eq(c, "above"), x, (int)y);
        if (rc < 0) { out(rc == -2 ? "prod-alert-add error exists\n" : "prod-alert-add error invalid or full\n"); return 1; }
        out("prod-alert-add ok "); out(a); out("\n"); return 0;
    }
    if (s_eq(cmd, "prod-alert-del")) {
        word(rest, a, 64);
        if (prod_alert_remove(&g_a, a)) { out("prod-alert-del error unknown\n"); return 1; }
        out("prod-alert-del ok\n"); return 0;
    }
    if (s_eq(cmd, "prod-alerts")) { show_alerts(); return 0; }
    if (s_eq(cmd, "prod-log-append")) {
        int n;
        rest = word(rest, a, 64);
        if (!a[0] || !*rest) { out("prod-log-append error usage: PATH TEXT\n"); return 1; }
        n = read_all(a, g_buf, (int)sizeof(g_buf));
        if (n < 0) { g_buf[0] = 0; n = 0; }
        if (n + s_len(rest) + 2 > 1000) { out("prod-log-append error file full (ramfs 1000 bytes)\n"); return 1; }
        s_cat(g_buf, rest, (int)sizeof(g_buf)); s_cat(g_buf, "\n", (int)sizeof(g_buf));
        if (sys_writefile(a, g_buf, s_len(g_buf)) < 0) { out("prod-log-append error write\n"); return 1; }
        line("prod-log-append ok bytes ", (uint32_t)s_len(g_buf), "\n"); return 0;
    }
    if (s_eq(cmd, "prod-log-analyze")) {
        static prod_log_report_t r;
        int i;
        word(rest, a, 64);
        if (read_all(a, g_buf, (int)sizeof(g_buf)) < 0) { out("prod-log-analyze error cannot read\n"); return 1; }
        prod_log_analyze(g_buf, &r);
        g_out[0] = 0; s_cat(g_out, "prod-log lines ", 4096); cat_u(g_out, r.lines, 4096);
        s_cat(g_out, " errors ", 4096); cat_u(g_out, r.errors, 4096); s_cat(g_out, " warns ", 4096); cat_u(g_out, r.warns, 4096);
        s_cat(g_out, " infos ", 4096); cat_u(g_out, r.infos, 4096); s_cat(g_out, " other ", 4096); cat_u(g_out, r.other, 4096);
        s_cat(g_out, "\nprod-log burst ", 4096); cat_u(g_out, r.burst_max, 4096); s_cat(g_out, " errors in 10 lines ending at ", 4096); cat_u(g_out, r.burst_line, 4096);
        if (r.first_error[0]) { s_cat(g_out, "\nprod-log first-error ", 4096); s_cat(g_out, r.first_error, 4096); }
        s_cat(g_out, "\n", 4096);
        for (i = 0; i < r.ntop; i++) { s_cat(g_out, "prod-log top ", 4096); cat_u(g_out, r.top[i].count, 4096); s_cat(g_out, " x ", 4096); s_cat(g_out, r.top[i].pattern, 4096); s_cat(g_out, "\n", 4096); }
        s_cat(g_out, r.burst_max >= 3 ? "prod-log verdict incident\n" : "prod-log verdict normal\n", 4096);
        out(g_out);
        if (r.burst_max >= 3) prod_event(&g_a, sys_ticks(), "LOG incident burst");
        return 0;
    }
    if (s_eq(cmd, "prod-backup")) {
        int s;
        rest = word(rest, a, 64); word(rest, b, 64);
        if (!a[0] || !b[0]) { out("prod-backup error usage: NAME DIR\n"); return 1; }
        s = backup_dir(a, b);
        if (s < 0) { out(s == -2 ? "prod-backup error too large\n" : "prod-backup error cannot list\n"); return 1; }
        g_out[0] = 0; s_cat(g_out, "prod-backup ok ", 4096); s_cat(g_out, a, 4096); s_cat(g_out, " files ", 4096);
        cat_u(g_out, (uint32_t)g_arch[s].n, 4096); s_cat(g_out, " bytes ", 4096); cat_u(g_out, g_arch[s].used, 4096); s_cat(g_out, "\n", 4096);
        out(g_out); return 0;
    }
    if (s_eq(cmd, "prod-backups")) {
        int i, bad, k = 0;
        for (i = 0; i < SLOTS; i++) if (g_arch[i].label[0]) {
            k++;
            g_out[0] = 0; s_cat(g_out, "prod-backup ", 4096); s_cat(g_out, g_arch[i].label, 4096); s_cat(g_out, " dir ", 4096); s_cat(g_out, g_dir[i], 4096);
            s_cat(g_out, " files ", 4096); cat_u(g_out, (uint32_t)g_arch[i].n, 4096); s_cat(g_out, " tick ", 4096); cat_u(g_out, g_arch[i].tick, 4096);
            s_cat(g_out, prod_archive_verify(&g_arch[i], &bad) ? " corrupt\n" : " intact\n", 4096);
            out(g_out);
        }
        line("prod-backups ok ", (uint32_t)k, "\n"); return 0;
    }
    if (s_eq(cmd, "prod-backup-verify") || s_eq(cmd, "prod-backup-corrupt") || s_eq(cmd, "prod-restore")) {
        int s, bad, n;
        word(rest, a, 64);
        s = slot_of(a, 0);
        if (s < 0) { out(cmd); out(" error unknown backup\n"); return 1; }
        if (s_eq(cmd, "prod-backup-corrupt")) { /* test hook: simulated media damage */
            if (g_arch[s].used == 0) { out("prod-backup-corrupt error empty\n"); return 1; }
            g_arch[s].data[g_arch[s].used / 2] ^= 0x5A; out("prod-backup-corrupt ok\n"); return 0;
        }
        if (s_eq(cmd, "prod-backup-verify")) {
            n = prod_archive_verify(&g_arch[s], &bad);
            if (n) { out("prod-backup-verify error corrupt "); out(g_arch[s].e[bad].path); out("\n"); return 1; }
            line("prod-backup-verify ok files ", (uint32_t)g_arch[s].n, "\n"); return 0;
        }
        n = restore_slot(s);
        if (n < 0) { out(n == -1 ? "prod-restore error backup corrupt, nothing written\n" : "prod-restore error write\n"); return 1; }
        line("prod-restore ok files ", (uint32_t)n, "\n"); return 0;
    }
    if (s_eq(cmd, "prod-manifest")) {
        /* STAGING/MANIFEST lists TARGET/<file> with the staged checksum */
        static os_dirent_t ents[32];
        char p[96], q[96];
        int n, i, files = 0;
        rest = word(rest, a, 64); word(rest, b, 64);
        if (!a[0] || !b[0]) { out("prod-manifest error usage: STAGING TARGET\n"); return 1; }
        n = sys_listdir(a, ents, 32);
        if (n < 0) { out("prod-manifest error cannot list\n"); return 1; }
        g_tmp[0] = 0;
        for (i = 0; i < n; i++) {
            int len;
            if (ents[i].flags == OS_DIRENT_DIR || s_eq(ents[i].name, "MANIFEST")) continue;
            join(p, a, ents[i].name, 96);
            len = sys_readfile(p, g_buf, (int)sizeof(g_buf));
            if (len < 0) continue;
            join(q, b, ents[i].name, 96);
            s_cat(g_tmp, q, 4096); s_cat(g_tmp, " ", 4096); cat_hex(g_tmp, prod_fnv(g_buf, (uint32_t)len), 4096); s_cat(g_tmp, "\n", 4096);
            files++;
        }
        join(p, a, "MANIFEST", 96);
        if (!files || s_len(g_tmp) > 1000 || sys_writefile(p, g_tmp, s_len(g_tmp)) < 0) { out("prod-manifest error write\n"); return 1; }
        line("prod-manifest ok files ", (uint32_t)files, "\n"); return 0;
    }
    if (s_eq(cmd, "prod-deploy")) {
        /* US-108/109: copy STAGING into TARGET after a backup, check TARGET
         * against STAGING/MANIFEST and TARGET/health, roll back on failure. */
        static os_dirent_t ents[32];
        char src[96], dst[96], why[64];
        int n, i, s, files = 0;
        rest = word(rest, a, 64); word(rest, b, 64);
        if (!a[0] || !b[0]) { out("prod-deploy error usage: STAGING TARGET\n"); return 1; }
        join(src, a, "MANIFEST", 96);
        if (read_all(src, g_tmp, (int)sizeof(g_tmp)) < 0) { out("prod-deploy error no MANIFEST in staging\n"); return 1; }
        s = backup_dir("pre-deploy", b);
        if (s < 0) { out("prod-deploy error cannot back up target\n"); return 1; }
        line("prod-deploy backup pre-deploy files ", (uint32_t)g_arch[s].n, "\n");
        n = sys_listdir(a, ents, 32);
        for (i = 0; i < n; i++) {
            int len;
            if (ents[i].flags == OS_DIRENT_DIR || s_eq(ents[i].name, "MANIFEST")) continue;
            join(src, a, ents[i].name, 96); join(dst, b, ents[i].name, 96);
            len = sys_readfile(src, g_buf, (int)sizeof(g_buf));
            if (len >= 0 && sys_writefile(dst, g_buf, len) >= 0) files++;
        }
        line("prod-deploy copied ", (uint32_t)files, "\n");
        n = prod_manifest_check(g_tmp, rd_cb, 0, why, (int)sizeof(why));
        join(dst, b, "health", 96);
        if (n > 0 && (read_all(dst, g_buf, (int)sizeof(g_buf)) < 2 || !s_pre(g_buf, "ok"))) { n = -4; s_copy(why, "health check not ok", 64); }
        if (n > 0) { line("prod-deploy ok healthy files ", (uint32_t)n, "\n"); prod_event(&g_a, sys_ticks(), "DEPLOY ok"); return 0; }
        out("prod-deploy health failed: "); out(why); out("\n");
        i = restore_slot(s);
        line("prod-deploy rolled back files ", (uint32_t)(i > 0 ? i : 0), "\n");
        prod_event(&g_a, sys_ticks(), "DEPLOY rolled back");
        return 1;
    }
    if (s_eq(cmd, "prod-rollback")) {
        int s = slot_of("pre-deploy", 0), n;
        if (s < 0) { out("prod-rollback error no deployment backup\n"); return 1; }
        n = restore_slot(s);
        if (n < 0) { out("prod-rollback error backup corrupt\n"); return 1; }
        line("prod-rollback ok files ", (uint32_t)n, "\n"); prod_event(&g_a, sys_ticks(), "ROLLBACK manual"); return 0;
    }
    if (s_eq(cmd, "prod-scale-sim")) {
        prod_scaler_t sc;
        uint32_t t = 0;
        prod_scaler_init(&sc, 1, 4, 4000U, 1000U, 2U);
        g_out[0] = 0; s_cat(g_out, "prod-scale workers", 4096);
        while (*rest) {
            rest = word(rest, a, 64);
            if (to_u(a, &x)) { out("prod-scale-sim error queue values\n"); return 1; }
            s_cat(g_out, " ", 4096); cat_u(g_out, (uint32_t)prod_scaler_step(&sc, x, t), 4096); t++;
        }
        s_cat(g_out, "\nprod-scale-sim ok ups ", 4096); cat_u(g_out, (uint32_t)sc.ups, 4096);
        s_cat(g_out, " downs ", 4096); cat_u(g_out, (uint32_t)sc.downs, 4096); s_cat(g_out, "\n", 4096);
        out(g_out); return 0;
    }
    if (s_eq(cmd, "prod-scale-run") || s_eq(cmd, "prod-scale-status") || s_eq(cmd, "prod-scale-stop")) {
        /* US-110: the scaler drives a pool of real worker tasks (the `idle`
         * program, which yields forever); one step per queue value. */
        static os_proc_t ps[16];
        int i, n, live = 0;
        if (s_eq(cmd, "prod-scale-run")) {
            if (!g_sc_on) { prod_scaler_init(&g_sc, 1, POOL_MAX, 4000U, 1000U, 2U); g_sc_on = 1; g_sc_t = 0; }
            while (*rest) {
                int want;
                rest = word(rest, a, 64);
                if (to_u(a, &x)) { out("prod-scale-run error queue values\n"); return 1; }
                want = prod_scaler_step(&g_sc, x, g_sc_t++);
                while (g_npool < want) {
                    int pid = spawn("idle", 0);
                    if (pid <= 0) { out("prod-scale-run error spawn refused (child capacity)\n"); break; }
                    g_pool[g_npool++] = pid;
                }
                if (g_npool < want) break;
                while (g_npool > want) { (void)sys_kill_pid(g_pool[--g_npool]); }
                pause(5);
                g_out[0] = 0; s_cat(g_out, "prod-scale step queue ", 4096); cat_u(g_out, x, 4096);
                s_cat(g_out, " workers ", 4096); cat_u(g_out, (uint32_t)g_npool, 4096); s_cat(g_out, " pids", 4096);
                for (i = 0; i < g_npool; i++) { s_cat(g_out, " ", 4096); cat_u(g_out, (uint32_t)g_pool[i], 4096); }
                s_cat(g_out, "\n", 4096); out(g_out);
            }
        } else if (s_eq(cmd, "prod-scale-stop")) {
            while (g_npool > 0) (void)sys_kill_pid(g_pool[--g_npool]);
            g_sc_on = 0;
            pause(5);
        }
        n = sys_ps(ps, 16);
        for (i = 0; i < n; i++) { int k; for (k = 0; k < g_npool; k++) if (ps[i].pid == g_pool[k]) live++; }
        g_out[0] = 0; s_cat(g_out, cmd, 4096); s_cat(g_out, " ok workers ", 4096); cat_u(g_out, (uint32_t)g_npool, 4096);
        s_cat(g_out, " live_in_ps ", 4096); cat_u(g_out, (uint32_t)live, 4096);
        s_cat(g_out, " ups ", 4096); cat_u(g_out, (uint32_t)g_sc.ups, 4096); s_cat(g_out, " downs ", 4096); cat_u(g_out, (uint32_t)g_sc.downs, 4096);
        s_cat(g_out, "\n", 4096); out(g_out);
        return 0;
    }
    if (s_eq(cmd, "prod-integrity")) {
        static os_dirent_t ents[48];
        char p[96];
        int n, i, base, changed = 0, added = 0, same = 0;
        rest = word(rest, a, 64); word(rest, b, 64);
        base = s_eq(a, "baseline");
        if ((!base && !s_eq(a, "check")) || !b[0]) { out("prod-integrity error usage: baseline|check DIR\n"); return 1; }
        n = sys_listdir(b, ents, 48);
        if (n < 0) { out("prod-integrity error cannot list\n"); return 1; }
        if (base) g_base.n = 0;
        for (i = 0; i < n; i++) {
            int len; uint32_t h;
            if (ents[i].flags == OS_DIRENT_DIR) continue;
            join(p, b, ents[i].name, 96);
            len = sys_readfile(p, g_buf, (int)sizeof(g_buf));
            if (len < 0) continue;
            h = prod_fnv(g_buf, (uint32_t)len);
            if (base) { (void)prod_baseline_add(&g_base, p, h); continue; }
            switch (prod_baseline_cmp(&g_base, p, h)) {
            case 0: same++; break;
            case 1: changed++; out("prod-integrity changed "); out(p); out("\n"); break;
            default: added++; out("prod-integrity new "); out(p); out("\n"); break;
            }
        }
        if (base) { line("prod-integrity baseline files ", (uint32_t)g_base.n, "\n"); return 0; }
        g_out[0] = 0; s_cat(g_out, "prod-integrity check same ", 4096); cat_u(g_out, (uint32_t)same, 4096);
        s_cat(g_out, " changed ", 4096); cat_u(g_out, (uint32_t)changed, 4096); s_cat(g_out, " new ", 4096); cat_u(g_out, (uint32_t)added, 4096);
        s_cat(g_out, changed || added ? " verdict drift\n" : " verdict clean\n", 4096);
        out(g_out);
        if (changed || added) prod_event(&g_a, sys_ticks(), "SECURITY integrity drift");
        return (changed || added) ? 1 : 0;
    }
    if (s_eq(cmd, "prod-bench")) { bench(); return 0; }
    if (s_eq(cmd, "prod-diag")) {
        prod_sample_t s;
        int i, firing = 0;
        take(&s);
        for (i = 0; i < PROD_RULES; i++) firing += g_a.r[i].used && g_a.r[i].firing;
        g_out[0] = 0;
        s_cat(g_out, "prod-diag uptime_ticks ", 4096); cat_u(g_out, s.tick, 4096);
        s_cat(g_out, " mem_used ", 4096); cat_u(g_out, s.v[PM_MEM_USED], 4096); s_cat(g_out, " mem_free ", 4096); cat_u(g_out, s.v[PM_MEM_FREE], 4096);
        s_cat(g_out, " procs ", 4096); cat_u(g_out, s.v[PM_PROCS], 4096);
        s_cat(g_out, "\nprod-diag net_status ", 4096); cat_i(g_out, sys0(SYS_NET_STATUS), 4096);
        s_cat(g_out, " alerts_firing ", 4096); cat_u(g_out, (uint32_t)firing, 4096);
        s_cat(g_out, " events ", 4096); cat_u(g_out, (uint32_t)g_a.ev_n, 4096);
        s_cat(g_out, " samples ", 4096); cat_u(g_out, (uint32_t)g_m.n, 4096);
        s_cat(g_out, "\nprod-diag ok attach this block to a support request\n", 4096);
        out(g_out); return 0;
    }
    if (s_eq(cmd, "prod-tutorial")) {
        static const char* const steps[] = {
            "1 monitor: prod-sample 5 20 then prod-metrics",
            "2 alert: prod-alert-add hot busy above 80 2 then prod-alerts",
            "3 logs: prod-log-append /var/app.log ERROR x then prod-log-analyze /var/app.log",
            "4 backup: prod-backup daily /etc then prod-backup-verify daily",
            "5 deploy: prod-deploy /staging /app (rolls back if unhealthy)",
            "6 support: prod-diag and prod-feedback 5 text", 0 };
        int i;
        word(rest, a, 64);
        if (a[0]) {
            if (to_u(a, &x) || x < 1 || x > 6) { out("prod-tutorial error lesson 1..6\n"); return 1; }
            out("prod-tutorial lesson "); out(steps[x - 1]); out("\n"); return 0;
        }
        for (i = 0; steps[i]; i++) { out("prod-tutorial "); out(steps[i]); out("\n"); }
        out("prod-tutorial ok lessons 6\n"); return 0;
    }
    if (s_eq(cmd, "prod-feedback")) {
        int i;
        rest = word(rest, a, 64);
        if (s_eq(a, "summary")) {
            g_out[0] = 0; s_cat(g_out, "prod-feedback total ", 4096); cat_u(g_out, g_fb.total, 4096);
            if (g_fb.total) { uint32_t t10 = g_fb.sum * 10U / g_fb.total; s_cat(g_out, " avg ", 4096); cat_u(g_out, t10 / 10U, 4096); s_cat(g_out, ".", 4096); cat_u(g_out, t10 % 10U, 4096); }
            for (i = 1; i <= 5; i++) { s_cat(g_out, " r", 4096); cat_u(g_out, (uint32_t)i, 4096); s_cat(g_out, "=", 4096); cat_u(g_out, g_fb.hist[i], 4096); }
            s_cat(g_out, "\n", 4096);
            for (i = 0; i < g_fb.n; i++) { s_cat(g_out, "prod-feedback ", 4096); cat_u(g_out, g_fb.f[i].rating, 4096); s_cat(g_out, " ", 4096); s_cat(g_out, g_fb.f[i].text, 4096); s_cat(g_out, "\n", 4096); }
            out(g_out); return 0;
        }
        if (to_u(a, &x) || prod_feedback_add(&g_fb, (int)x, rest) != 0) { out("prod-feedback error usage: RATING(1-5) TEXT | summary\n"); return 1; }
        line("prod-feedback ok total ", g_fb.total, "\n"); return 0;
    }
    if (s_eq(cmd, "prod-usage")) {
        int i;
        prod_usage_sort(&g_use);
        g_out[0] = 0;
        for (i = 0; i < g_use.n && i < 8; i++) { s_cat(g_out, "prod-usage ", 4096); s_cat(g_out, g_use.u[i].name, 4096); s_cat(g_out, " ", 4096); cat_u(g_out, g_use.u[i].count, 4096); s_cat(g_out, "\n", 4096); }
        s_cat(g_out, "prod-usage ok commands ", 4096); cat_u(g_out, g_use.total, 4096); s_cat(g_out, " distinct ", 4096); cat_u(g_out, (uint32_t)g_use.n, 4096); s_cat(g_out, "\n", 4096);
        out(g_out); return 0;
    }
    if (s_eq(cmd, "prod-roadmap")) {
        out("prod-roadmap phase 1-3 delivered (see docs/ETAT_REEL.md)\n");
        out("prod-roadmap phase 4 promptmessage delivered #116\n");
        out("prod-roadmap phase 5 p2p #117, phase 6 platform #118, phase 7 collab #119 (open)\n");
        out("prod-roadmap phase 8 production this build (docs/production.md)\n");
        out("prod-roadmap ok phases 8\n");
        return 0;
    }
    out(cmd); out(" error unknown prod command\n");
    return 1;
}

/* persistence hooks for shell_persist.c */
static prod_state_ref_t ref(void) {
    prod_state_ref_t r;
    r.m = &g_m; r.a = &g_a; r.arch = g_arch; r.dir = g_dir; r.slots = SLOTS; r.fb = &g_fb;
    return r;
}
int shell_prod_save(uint8_t* o, int cap) { prod_state_ref_t r = ref(); if (!g_init) return 0; return prod_state_save(&r, o, cap); }
int shell_prod_load(const uint8_t* in, int len) { prod_state_ref_t r = ref(); init(); return prod_state_load(&r, in, len); }
/* fleet-report (shell_fleet.c): take a fresh sample, summarise it and the
 * newest alert event for the guest-to-guest collector */
void shell_prod_summary(char* metric, int mcap, char* log, int lcap) {
    prod_sample_t s;
    int i, firing = 0;
    init();
    take(&s); push(&s);
    for (i = 0; i < PROD_RULES; i++) if (g_a.r[i].used && g_a.r[i].firing) firing++;
    metric[0] = 0;
    s_cat(metric, "mem_used_kb=", mcap); cat_u(metric, s.v[PM_MEM_USED], mcap);
    s_cat(metric, " procs=", mcap); cat_u(metric, s.v[PM_PROCS], mcap);
    s_cat(metric, " busy=", mcap); cat_u(metric, s.v[PM_BUSY], mcap);
    s_cat(metric, " custom=", mcap); cat_u(metric, s.v[PM_CUSTOM], mcap);
    s_cat(metric, " firing=", mcap); cat_u(metric, (uint32_t)firing, mcap);
    s_cat(metric, " samples=", mcap); cat_u(metric, (uint32_t)g_m.n, mcap);
    log[0] = 0;
    if (g_a.ev_n > 0) s_cat(log, g_a.ev[(g_a.ev_head - 1 + PROD_EVENTS) % PROD_EVENTS].text, lcap);
}
