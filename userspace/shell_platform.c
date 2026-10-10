/* userspace/shell_platform.c - guest shell front end of userspace/platform.c.
 * Probes use existing syscalls only (meminfo, ps/task metrics, net status,
 * ATA status, ramfs files) and the CPUID instruction (allowed at CPL 3). */
#include "shell_platform.h"
#include "platform.h"
#include "../include/os_syscalls.h"

void print_string(const char* s);
int sys_listdir(const char* path, os_dirent_t* out, int max_n);
int sys_readfile(const char* path, char* buf, int max);
int sys_writefile(const char* path, const char* buf, int n);
int sys_mkdir(const char* path);
int sys_unlink(const char* path);
int sys_meminfo(os_meminfo_t* info);
int sys_getpid(void);
int sys_ps(os_proc_t* out, int max_n);
int sys_task_metrics(int pid, os_task_metrics_t* out);
unsigned int sys_ticks(void);

static int g_profile = PLAT_PWR_BALANCED;
static int g_auto;
static plat_notes_t g_notes;
static char g_out[4096];
static char g_big[16384];
static char g_file[1100];

static int sys0(int nr) { int r; asm volatile("int $0x80" : "=a"(r) : "a"(nr) : "memory"); return r; }
static int sys1(int nr, const void* a) { int r; asm volatile("int $0x80" : "=a"(r) : "a"(nr), "b"(a) : "memory"); return r; }

static int s_len(const char* s) { int n = 0; while (s && s[n]) n++; return n; }
static int s_eq(const char* a, const char* b) { while (*a && *a == *b) { a++; b++; } return *a == *b; }
static void s_copy(char* d, const char* s, int cap) { int i = 0; while (s && s[i] && i < cap - 1) { d[i] = s[i]; i++; } d[i] = 0; }
static void s_cat(char* d, const char* s, int cap) { int n = s_len(d), i = 0; while (s && s[i] && n < cap - 1) d[n++] = s[i++]; d[n] = 0; }
static void cat_u(char* d, uint32_t v, int cap) {
    char t[11], r[12]; int n = 0, i = 0;
    do { t[n++] = (char)('0' + v % 10U); v /= 10U; } while (v);
    while (n) r[i++] = t[--n];
    r[i] = 0; s_cat(d, r, cap);
}
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
static int to_int(const char* s, int* ok) {
    int v = 0, any = 0, neg = 0;
    if (*s == '-') { neg = 1; s++; }
    while (*s >= '0' && *s <= '9') { v = v * 10 + (*s++ - '0'); any = 1; }
    if (ok) *ok = any && !*s;
    return neg ? -v : v;
}
static void join(char* d, const char* dir, const char* name, int cap) {
    d[0] = 0; s_cat(d, dir, cap);
    if (s_len(d) > 1 && d[s_len(d) - 1] != '/') s_cat(d, "/", cap);
    if (s_len(d) == 0) s_cat(d, "/", cap);
    s_cat(d, name, cap);
}

static const char* k_cmds[] = {
    "hal-info", "hal-port", "screen-adapt", "gesture", "power-profile", "power-status", "dev-list",
    "compat-check", "compat-scan", "notify-push", "notify-list", "notify-ack", "migrate-export",
    "migrate-import", "migrate-verify", "deploy-make", "deploy-apply", "deploy-verify", "admin-all", 0
};
int shell_platform_is(const char* line) {
    char w[24];
    int i;
    word(line, w, (int)sizeof(w));
    for (i = 0; k_cmds[i]; i++) if (s_eq(w, k_cmds[i])) return 1;
    return 0;
}

/* ------------------------------------------------------------- probes */
static void cpuid(uint32_t leaf, uint32_t* a, uint32_t* b, uint32_t* c, uint32_t* d) {
    asm volatile("cpuid" : "=a"(*a), "=b"(*b), "=c"(*c), "=d"(*d) : "a"(leaf), "c"(0));
}
static void hal_probe(plat_hal_t* h) {
    uint32_t a, b, c, d;
    os_meminfo_t m;
    int i;
    h->arch = "i386"; h->word_bits = 32; h->page_size = 4096; h->little_endian = 1;
    cpuid(0, &a, &b, &c, &d);
    for (i = 0; i < 4; i++) { h->cpu_vendor[i] = (char)(b >> (8 * i)); h->cpu_vendor[4 + i] = (char)(d >> (8 * i)); h->cpu_vendor[8 + i] = (char)(c >> (8 * i)); }
    h->cpu_vendor[12] = 0;
    cpuid(1, &a, &b, &c, &d);
    h->cpu_family = (a >> 8) & 15U; h->cpu_model = (a >> 4) & 15U;
    h->features_edx = d; h->features_ecx = c;
    h->mem_total_kib = h->mem_free_kib = 0;
    if (sys_meminfo(&m) == 0) { h->mem_total_kib = m.total_pages * 4U; h->mem_free_kib = m.free_pages * 4U; }
}
static void dev_probe(plat_devices_t* r) {
    plat_hal_t h;
    char t[48];
    int net = sys0(SYS_NET_STATUS);
    static os_ata_status_t ata;
    int ata_rc;
    r->n = 0;
    hal_probe(&h);
    t[0] = 0; s_cat(t, h.cpu_vendor, 48); s_cat(t, " family ", 48); cat_u(t, h.cpu_family, 48);
    plat_dev_add(r, "cpu0", "cpu", 1, t);
    t[0] = 0; s_cat(t, "kib ", 48); cat_u(t, h.mem_total_kib, 48);
    plat_dev_add(r, "mem0", "memory", h.mem_total_kib > 0, t);
    plat_dev_add(r, "kbd0", "input", 1, "ps2 keyboard irq1");
    plat_dev_add(r, "vga0", "display", 1, "vga text 80x25");
    plat_dev_add(r, "rtc0", "clock", 1, "cmos rtc");
    plat_dev_add(r, "pit0", "timer", 1, "pit 100 hz");
    plat_dev_add(r, "eth0", "net", net > 0 && (net & 1), (net > 0 && (net & 1)) ? "ne2000 isa 0x300" : "ne2000 not detected");
    ata_rc = sys1(SYS_ATA_STATUS, &ata);
    plat_dev_add(r, "ata0", "storage", ata_rc == 0, ata_rc == 0 ? "ata pio primary" : "ata status unavailable");
    plat_dev_add(r, "touch0", "input", 0, "no touch controller emulated");
    plat_dev_add(r, "sensor0", "sensor", 0, "no sensors emulated");
}
/* busy share of the other tasks over ~0.5 s (scheduler run ticks). */
static uint32_t busy_sample(void) {
    static os_proc_t ps[16];
    static uint32_t before[16];
    os_task_metrics_t m;
    int n, i, self = sys_getpid();
    uint32_t t0, t1, used = 0;
    n = sys_ps(ps, 16);
    if (n <= 0) return 0;
    for (i = 0; i < n; i++) before[i] = (ps[i].pid != self && sys_task_metrics(ps[i].pid, &m) == 0) ? m.run_ticks : 0;
    t0 = sys_ticks();
    while (sys_ticks() - t0 < 50U) (void)sys0(SYS_YIELD);
    t1 = sys_ticks();
    for (i = 0; i < n; i++)
        if (ps[i].pid != self && sys_task_metrics(ps[i].pid, &m) == 0 && m.run_ticks >= before[i]) used += m.run_ticks - before[i];
    if (t1 == t0) return 0;
    used = used * 100U / (t1 - t0);
    return used > 100U ? 100U : used;
}

/* ------------------------------------------------------------ helpers */
static int read_all(const char* path, char* buf, int cap) {
    int n = sys_readfile(path, buf, cap - 1);
    if (n < 0) return n;
    buf[n] = 0;
    return n;
}
static int load_chunks(const char* name, char* buf, int cap) {
    char path[96], num[12];
    int k, total = 0, n;
    for (k = 0; k < 32; k++) {
        path[0] = 0; s_cat(path, name, 96); s_cat(path, ".", 96); num[0] = 0; cat_u(num, (uint32_t)k, 12); s_cat(path, num, 96);
        n = sys_readfile(path, buf + total, cap - total - 1);
        if (n < 0) break;
        total += n;
        if (n < 1000) { k++; break; }
    }
    buf[total] = 0;
    return k == 0 ? -1 : total;
}

static int cmd_migrate_export(const char* rest) {
    static os_dirent_t ents[PLAT_MIG_FILES + 8];
    static plat_mig_entry_t e[PLAT_MIG_FILES];
    static char data[PLAT_MIG_FILES][1025];
    char dir[64], name[64], path[160], num[12];
    int n, i, cnt = 0, len, k;
    rest = word(rest, dir, 64); rest = word(rest, name, 64);
    if (!dir[0] || !name[0]) { print_string("migrate-export error usage: migrate-export DIR ARCHIVE\n"); return 1; }
    n = sys_listdir(dir, ents, PLAT_MIG_FILES + 8);
    if (n < 0) { print_string("migrate-export error no such directory\n"); return 1; }
    for (i = 0; i < n; i++) {
        int got;
        if (!(ents[i].flags & OS_DIRENT_FILE)) continue;
        if (cnt >= PLAT_MIG_FILES) { print_string("migrate-export error too many files\n"); return 1; }
        join(path, dir, ents[i].name, 160);
        got = sys_readfile(path, data[cnt], 1024);
        if (got < 0) { print_string("migrate-export error unreadable "); print_string(path); print_string("\n"); return 1; }
        s_copy(e[cnt].name, ents[i].name, 64); e[cnt].data = data[cnt]; e[cnt].len = got;
        cnt++;
    }
    len = plat_mig_pack(e, cnt, g_big, (int)sizeof(g_big));
    if (len < 0) { print_string("migrate-export error pack failed\n"); return 1; }
    for (k = 0; k * 1000 < len || k == 0; k++) {
        int part = len - k * 1000 > 1000 ? 1000 : len - k * 1000;
        path[0] = 0; s_cat(path, name, 160); s_cat(path, ".", 160); num[0] = 0; cat_u(num, (uint32_t)k, 12); s_cat(path, num, 160);
        if (sys_writefile(path, g_big + k * 1000, part) < 0) { print_string("migrate-export error write failed\n"); return 1; }
    }
    g_out[0] = 0;
    s_cat(g_out, "migrate-export ok files ", 4096); cat_u(g_out, (uint32_t)cnt, 4096);
    s_cat(g_out, " bytes ", 4096); cat_u(g_out, (uint32_t)len, 4096);
    s_cat(g_out, " chunks ", 4096); cat_u(g_out, (uint32_t)k, 4096);
    s_cat(g_out, " fnv ", 4096); cat_hex(g_out, plat_fnv((const uint8_t*)g_big, len), 4096); s_cat(g_out, "\n", 4096);
    print_string(g_out);
    return 0;
}
static int cmd_migrate_import(const char* rest, int verify_only) {
    static plat_mig_entry_t e[PLAT_MIG_FILES];
    char name[64], dir[64], path[160];
    int len, n, i;
    const char* cmd = verify_only ? "migrate-verify" : "migrate-import";
    rest = word(rest, name, 64); rest = word(rest, dir, 64);
    if (!name[0] || (!verify_only && !dir[0])) { print_string(cmd); print_string(" error usage: ARCHIVE [DIR]\n"); return 1; }
    len = load_chunks(name, g_big, (int)sizeof(g_big));
    if (len < 0) { print_string(cmd); print_string(" error archive not found\n"); return 1; }
    n = plat_mig_unpack(g_big, len, e, PLAT_MIG_FILES);
    if (n < 0) {
        print_string(cmd);
        print_string(n == -2 ? " error checksum mismatch\n" : n == -3 ? " error too many files\n" : " error bad archive\n");
        return 1;
    }
    if (!verify_only) {
        (void)sys_mkdir(dir);
        for (i = 0; i < n; i++) {
            join(path, dir, e[i].name, 160);
            (void)sys_unlink(path);
            if (sys_writefile(path, e[i].data, e[i].len) < 0) { print_string("migrate-import error write "); print_string(path); print_string("\n"); return 1; }
        }
    }
    g_out[0] = 0;
    s_cat(g_out, cmd, 4096); s_cat(g_out, " ok files ", 4096); cat_u(g_out, (uint32_t)n, 4096);
    for (i = 0; i < n; i++) { s_cat(g_out, " ", 4096); s_cat(g_out, e[i].name, 4096); }
    s_cat(g_out, "\n", 4096);
    print_string(g_out);
    return 0;
}

static int cmd_deploy_make(const char* rest) {
    static os_dirent_t ents[24];
    char src[64], dst[64], man[64], path[160];
    int n, i, cnt = 0, got;
    rest = word(rest, src, 64); rest = word(rest, dst, 64); rest = word(rest, man, 64);
    if (src[0] != '/' || dst[0] != '/' || !man[0]) { print_string("deploy-make error usage: deploy-make /SRCDIR /DSTDIR MANIFEST\n"); return 1; }
    n = sys_listdir(src, ents, 24);
    if (n < 0) { print_string("deploy-make error no such directory\n"); return 1; }
    g_big[0] = 0; s_cat(g_big, "DEPLOY1\n", 1024);
    for (i = 0; i < n; i++) {
        if (!(ents[i].flags & OS_DIRENT_FILE)) continue;
        join(path, src, ents[i].name, 160);
        got = sys_readfile(path, g_file, 1024);
        if (got < 0) continue;
        s_cat(g_big, "file ", 1024); s_cat(g_big, path, 1024); s_cat(g_big, " ", 1024);
        join(path, dst, ents[i].name, 160); s_cat(g_big, path, 1024); s_cat(g_big, " ", 1024);
        cat_hex(g_big, plat_fnv((const uint8_t*)g_file, got), 1024); s_cat(g_big, "\n", 1024);
        cnt++;
    }
    if (s_len(g_big) >= 1020) { print_string("deploy-make error manifest too large\n"); return 1; }
    if (sys_writefile(man, g_big, s_len(g_big)) < 0) { print_string("deploy-make error write failed\n"); return 1; }
    g_out[0] = 0; s_cat(g_out, "deploy-make ok files ", 4096); cat_u(g_out, (uint32_t)cnt, 4096); s_cat(g_out, "\n", 4096);
    print_string(g_out);
    return 0;
}
static int cmd_deploy(const char* rest, int apply) {
    static plat_deploy_item_t it[16];
    char man[64];
    const char* cmd = apply ? "deploy-apply" : "deploy-verify";
    int n, i, got, done = 0, bad = 0;
    rest = word(rest, man, 64);
    got = read_all(man, g_big, 1100);
    if (got < 0) { print_string(cmd); print_string(" error manifest not found\n"); return 1; }
    n = plat_deploy_parse(g_big, got, it, 16);
    if (n < 0) { print_string(cmd); print_string(" error bad manifest\n"); return 1; }
    g_out[0] = 0;
    for (i = 0; i < n; i++) { /* phase 1: every source (apply) or target (verify) */
        const char* p = apply ? it[i].src : it[i].dst;
        got = sys_readfile(p, g_file, 1024);
        if (got < 0 || plat_fnv((const uint8_t*)g_file, got) != it[i].fnv) {
            bad++;
            s_cat(g_out, cmd, 4096); s_cat(g_out, got < 0 ? " missing " : " checksum ", 4096); s_cat(g_out, p, 4096); s_cat(g_out, "\n", 4096);
        }
    }
    if (bad) {
        s_cat(g_out, cmd, 4096); s_cat(g_out, " error failed ", 4096); cat_u(g_out, (uint32_t)bad, 4096);
        s_cat(g_out, apply ? " nothing changed\n" : "\n", 4096);
        print_string(g_out);
        return 1;
    }
    if (apply) { /* phase 2: copy, read back, roll back on failure */
        for (i = 0; i < n; i++) {
            char dir[64]; int k, slash = 0;
            for (k = 0; it[i].dst[k]; k++) if (it[i].dst[k] == '/') slash = k;
            s_copy(dir, it[i].dst, slash + 1 < 64 ? slash + 1 : 64);
            if (slash > 0) (void)sys_mkdir(dir);
            got = sys_readfile(it[i].src, g_file, 1024);
            (void)sys_unlink(it[i].dst);
            if (got < 0 || sys_writefile(it[i].dst, g_file, got) < 0 ||
                sys_readfile(it[i].dst, g_file, 1024) != got || plat_fnv((const uint8_t*)g_file, got) != it[i].fnv) {
                int j;
                for (j = 0; j <= i; j++) (void)sys_unlink(it[j].dst);
                g_out[0] = 0; s_cat(g_out, "deploy-apply error write ", 4096); s_cat(g_out, it[i].dst, 4096);
                s_cat(g_out, " rolled_back ", 4096); cat_u(g_out, (uint32_t)(i + 1), 4096); s_cat(g_out, "\n", 4096);
                print_string(g_out);
                return 1;
            }
            done++;
        }
    } else done = n;
    g_out[0] = 0; s_cat(g_out, cmd, 4096); s_cat(g_out, " ok files ", 4096); cat_u(g_out, (uint32_t)done, 4096); s_cat(g_out, "\n", 4096);
    print_string(g_out);
    return 0;
}

static int cmd_gesture(const char* rest) {
    plat_point_t p[16];
    int n = 0, g;
    char w[48];
    while (*rest && n < 16) {
        int v[4], k = 0, i = 0, ok;
        char part[12];
        rest = word(rest, w, 48);
        while (k < 4) {
            int j = 0;
            while (w[i] && w[i] != ',' && j < 11) part[j++] = w[i++];
            part[j] = 0;
            v[k++] = to_int(part, &ok);
            if (!ok) { print_string("gesture error usage: gesture x,y,ms,down ...\n"); return 1; }
            if (w[i] == ',') i++;
        }
        p[n].x = v[0]; p[n].y = v[1]; p[n].t_ms = (uint32_t)v[2]; p[n].down = v[3]; n++;
    }
    g = plat_gesture(p, n);
    g_out[0] = 0; s_cat(g_out, "gesture ok ", 4096); s_cat(g_out, plat_gesture_name(g), 4096);
    s_cat(g_out, " points ", 4096); cat_u(g_out, (uint32_t)n, 4096); s_cat(g_out, "\n", 4096);
    print_string(g_out);
    return 0;
}

static void power_line(void) {
    plat_power_policy_t p;
    plat_power_policy(g_profile, &p);
    g_out[0] = 0;
    s_cat(g_out, "power profile ", 4096); s_cat(g_out, plat_power_name(g_profile), 4096);
    s_cat(g_out, g_auto ? " mode auto" : " mode manual", 4096);
    s_cat(g_out, " idle_yields ", 4096); cat_u(g_out, p.idle_yields, 4096);
    s_cat(g_out, " poll_budget ", 4096); cat_u(g_out, p.poll_budget, 4096);
    s_cat(g_out, " screen_dim_s ", 4096); cat_u(g_out, p.screen_dim_s, 4096);
    s_cat(g_out, " background_ms ", 4096); cat_u(g_out, p.background_ms, 4096);
    s_cat(g_out, "\n", 4096);
    print_string(g_out);
}

int shell_platform_line(const char* line) {
    char cmd[24], a[64];
    const char* rest = word(line, cmd, (int)sizeof(cmd));
    if (s_eq(cmd, "hal-info")) {
        plat_hal_t h;
        hal_probe(&h);
        plat_hal_report(&h, g_out, (int)sizeof(g_out));
        print_string(g_out);
        return 0;
    }
    if (s_eq(cmd, "hal-port")) {
        const char* why;
        int ok;
        word(rest, a, 64);
        ok = plat_port_supported(a, &why);
        print_string(ok ? "hal-port ok " : "hal-port error "); print_string(a); print_string(" "); print_string(why); print_string("\n");
        return ok ? 0 : 1;
    }
    if (s_eq(cmd, "screen-adapt")) {
        plat_layout_t l;
        int w = 720, h = 400, ok = 1, i = 0;
        char num[8];
        word(rest, a, 64);
        if (a[0]) { /* WxH */
            int j = 0;
            while (a[i] && a[i] != 'x' && j < 7) num[j++] = a[i++];
            num[j] = 0; w = to_int(num, &ok);
            if (a[i] == 'x') { i++; h = to_int(a + i, &ok); } else ok = 0;
        }
        if (!ok || plat_layout(w, h, &l) != 0) { print_string("screen-adapt error size WxH between 160x120 and 8192x8192\n"); return 1; }
        g_out[0] = 0;
        s_cat(g_out, "screen-adapt ok ", 4096); cat_u(g_out, (uint32_t)w, 4096); s_cat(g_out, "x", 4096); cat_u(g_out, (uint32_t)h, 4096);
        s_cat(g_out, a[0] ? "" : " (vga text 80x25)", 4096);
        s_cat(g_out, " form ", 4096); s_cat(g_out, plat_form_name(l.form), 4096);
        s_cat(g_out, l.portrait ? " portrait" : " landscape", 4096);
        s_cat(g_out, " grid ", 4096); cat_u(g_out, (uint32_t)l.cols, 4096); s_cat(g_out, "x", 4096); cat_u(g_out, (uint32_t)l.rows, 4096);
        s_cat(g_out, " font_px ", 4096); cat_u(g_out, (uint32_t)l.font_px, 4096);
        s_cat(g_out, " panels ", 4096); cat_u(g_out, (uint32_t)l.panels, 4096);
        s_cat(g_out, l.compact ? " compact\n" : " full\n", 4096);
        print_string(g_out);
        return 0;
    }
    if (s_eq(cmd, "gesture")) return cmd_gesture(rest);
    if (s_eq(cmd, "power-profile")) {
        int p;
        word(rest, a, 64);
        if (!a[0]) { power_line(); return 0; }
        if (s_eq(a, "auto")) {
            uint32_t busy = busy_sample();
            g_auto = 1; g_profile = plat_power_auto(busy, 0);
            g_out[0] = 0; s_cat(g_out, "power-profile auto busy_percent ", 4096); cat_u(g_out, busy, 4096); s_cat(g_out, "\n", 4096);
            print_string(g_out);
            power_line();
            return 0;
        }
        p = plat_power_parse(a);
        if (!p) { print_string("power-profile error use performance|balanced|saver|auto\n"); return 1; }
        g_auto = 0; g_profile = p;
        power_line();
        return 0;
    }
    if (s_eq(cmd, "power-status")) {
        uint32_t busy = busy_sample();
        if (g_auto) g_profile = plat_power_auto(busy, 0);
        g_out[0] = 0; s_cat(g_out, "power-status busy_percent ", 4096); cat_u(g_out, busy, 4096);
        s_cat(g_out, " uptime_s ", 4096); cat_u(g_out, sys_ticks() / 100U, 4096);
        s_cat(g_out, " battery none (emulated pc, mains)\n", 4096);
        print_string(g_out);
        power_line();
        return 0;
    }
    if (s_eq(cmd, "dev-list")) {
        static plat_devices_t r;
        dev_probe(&r);
        word(rest, a, 64);
        plat_dev_report(&r, a, g_out, (int)sizeof(g_out));
        print_string(g_out);
        return 0;
    }
    if (s_eq(cmd, "compat-check") || s_eq(cmd, "compat-scan")) {
        static os_dirent_t ents[48];
        plat_compat_t c;
        int n = 1, i, ok_n = 0, bad_n = 0, scan = s_eq(cmd, "compat-scan");
        char path[160];
        word(rest, a, 64);
        if (scan) { if (!a[0]) s_copy(a, "/bin", 64); n = sys_listdir(a, ents, 48); }
        if (!a[0] || n < 0) { print_string(cmd); print_string(" error usage: compat-check PATH | compat-scan [DIR]\n"); return 1; }
        for (i = 0; i < n; i++) {
            int got;
            if (scan) { if (!(ents[i].flags & OS_DIRENT_FILE)) continue; join(path, a, ents[i].name, 160); }
            else s_copy(path, a, 160);
            got = sys_readfile(path, g_file, 64);
            if (got < 0) { print_string(cmd); print_string(" error unreadable "); print_string(path); print_string("\n"); return 1; }
            plat_elf_check((const uint8_t*)g_file, got, &c);
            if (c.ok) ok_n++; else bad_n++;
            g_out[0] = 0;
            s_cat(g_out, "compat ", 4096); s_cat(g_out, path, 4096); s_cat(g_out, c.ok ? " ok " : " incompatible ", 4096);
            s_cat(g_out, c.reason, 4096);
            if (c.ok) { s_cat(g_out, " entry 0x", 4096); cat_hex(g_out, c.entry, 4096); }
            s_cat(g_out, "\n", 4096);
            print_string(g_out);
        }
        g_out[0] = 0; s_cat(g_out, cmd, 4096); s_cat(g_out, " done ok ", 4096); cat_u(g_out, (uint32_t)ok_n, 4096);
        s_cat(g_out, " incompatible ", 4096); cat_u(g_out, (uint32_t)bad_n, 4096); s_cat(g_out, "\n", 4096);
        print_string(g_out);
        return 0;
    }
    if (s_eq(cmd, "notify-push")) {
        int ok, prio;
        uint32_t id;
        rest = word(rest, a, 64);
        prio = to_int(a, &ok);
        id = ok ? plat_note_push(&g_notes, prio, rest) : 0;
        if (!id) { print_string("notify-push error usage: notify-push 1-3 TEXT (or dropped)\n"); return 1; }
        g_out[0] = 0; s_cat(g_out, "notify-push ok id ", 4096); cat_u(g_out, id, 4096); s_cat(g_out, "\n", 4096);
        print_string(g_out);
        return 0;
    }
    if (s_eq(cmd, "notify-list")) { plat_note_report(&g_notes, g_out, (int)sizeof(g_out)); print_string(g_out); return 0; }
    if (s_eq(cmd, "notify-ack")) {
        int ok, id;
        word(rest, a, 64);
        id = to_int(a, &ok);
        if (!ok || plat_note_ack(&g_notes, (uint32_t)id) != 0) { print_string("notify-ack error unknown id\n"); return 1; }
        print_string("notify-ack ok\n");
        return 0;
    }
    if (s_eq(cmd, "migrate-export")) return cmd_migrate_export(rest);
    if (s_eq(cmd, "migrate-import")) return cmd_migrate_import(rest, 0);
    if (s_eq(cmd, "migrate-verify")) return cmd_migrate_import(rest, 1);
    if (s_eq(cmd, "deploy-make")) return cmd_deploy_make(rest);
    if (s_eq(cmd, "deploy-apply")) return cmd_deploy(rest, 1);
    if (s_eq(cmd, "deploy-verify")) return cmd_deploy(rest, 0);
    if (s_eq(cmd, "admin-all")) {
        plat_hal_t h;
        static plat_devices_t r;
        int i, present = 0;
        hal_probe(&h); dev_probe(&r);
        for (i = 0; i < r.n; i++) present += r.d[i].present;
        g_out[0] = 0;
        s_cat(g_out, "admin-all arch ", 4096); s_cat(g_out, h.arch, 4096);
        s_cat(g_out, " cpu ", 4096); s_cat(g_out, h.cpu_vendor, 4096);
        s_cat(g_out, " mem_free_kib ", 4096); cat_u(g_out, h.mem_free_kib, 4096);
        s_cat(g_out, " devices ", 4096); cat_u(g_out, (uint32_t)present, 4096); s_cat(g_out, "/", 4096); cat_u(g_out, (uint32_t)r.n, 4096);
        s_cat(g_out, " power ", 4096); s_cat(g_out, plat_power_name(g_profile), 4096);
        s_cat(g_out, " notes ", 4096);
        {
            char tmp[1024];
            cat_u(g_out, (uint32_t)plat_note_report(&g_notes, tmp, (int)sizeof(tmp)), 4096);
        }
        s_cat(g_out, " uptime_s ", 4096); cat_u(g_out, sys_ticks() / 100U, 4096);
        s_cat(g_out, "\n", 4096);
        print_string(g_out);
        return 0;
    }
    return 1;
}
