/* userspace/shell_platform.c - guest shell front end of userspace/platform.c.
 * Probes use existing syscalls only (meminfo, ps/task metrics, net status,
 * ATA status, ramfs files) and the CPUID instruction (allowed at CPL 3). */
#include "shell_platform.h"
#include "platform.h"
#include "dosemu.h"
#include "mobile_ui.h"
#include "devsync.h"
#include "p2p.h"
#include "osui_gui.h"
#include "osui_scale.h"
#include "plat_form.h"
#include "plat_energy.h"
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
p2p_node_t* shell_p2p_node(void);
const p2p_host_t* shell_p2p_host(void);
void shell_p2p_poll(void);
void shell_p2p_app(void* ctx, uint32_t from, const uint8_t* d, int len);

static int g_profile = PLAT_PWR_BALANCED;
static int g_auto;
static plat_notes_t g_notes;
static plat_dev_rules_t g_dev_rules;
static plat_admin_nodes_t g_admin_nodes;
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

static const char* k_cmds[] = { "form-factor", "power-apply", "power-sim",
    "hal-info", "hal-port", "screen-adapt", "screen-resize", "gesture", "gesture-multi", "power-profile", "power-status", "dev-list",
    "dev-rules", "dev-hotplug", "admin-nodes", "admin-node-add",
    "compat-check", "compat-scan", "notify-push", "notify-list", "notify-ack", "migrate-export",
    "migrate-import", "migrate-verify", "deploy-make", "deploy-apply", "deploy-verify", "admin-all",
    "sensor-read", "continuity-export", "continuity-import", "legacy-emul",
    "mobile-ui", "devsync-set", "devsync-del", "devsync-sync", "devsync-show", "devsync-pull", 0
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

/* US-086 (Lot 7): runs a DOS .COM file with the 8086 interpreter
 * (dosemu.c). legacy-emul FILE [MAX_STEPS]; FILE without '/' is /dos/FILE. */
static int cmd_legacy_emul(const char* rest) {
    static unsigned char img[0xFE00];
    static char out[1024];
    char a[64], b[16], path[96];
    dos_result_t r;
    int n, max = 1000000, st;
    rest = word(rest, a, 64); rest = word(rest, b, 16);
    if (!a[0]) { print_string("legacy-emul error usage: legacy-emul FILE.COM [MAX_STEPS]\n"); return 1; }
    if (b[0]) {
        int i;
        for (max = 0, i = 0; b[i] >= '0' && b[i] <= '9' && max < 10000000; i++) max = max * 10 + (b[i] - '0');
        if (b[i] || max <= 0) { print_string("legacy-emul error bad MAX_STEPS\n"); return 1; }
    }
    path[0] = 0;
    if (a[0] != '/') s_cat(path, "/dos/", 96);
    s_cat(path, a, 96);
    n = sys_readfile(path, (char*)img, (int)sizeof(img));
    if (n <= 0) { print_string("legacy-emul error cannot read "); print_string(path); print_string("\n"); return 1; }
    st = dos_run_com(img, n, out, (int)sizeof(out), (unsigned)max, &r);
    if (r.out_len > 0) {
        print_string(out);
        if (out[r.out_len - 1] != '\n') print_string("\n");
    }
    g_out[0] = 0;
    s_cat(g_out, st == DOS_EXITED ? "legacy-emul ok file " : "legacy-emul error file ", 4096);
    s_cat(g_out, path, 4096);
    s_cat(g_out, " status ", 4096); s_cat(g_out, dos_status_name(st), 4096);
    s_cat(g_out, " exit_code ", 4096); cat_u(g_out, (uint32_t)r.exit_code, 4096);
    s_cat(g_out, " steps ", 4096); cat_u(g_out, r.steps, 4096);
    s_cat(g_out, " ints ", 4096); cat_u(g_out, r.ints, 4096);
    s_cat(g_out, " bytes ", 4096); cat_u(g_out, (uint32_t)n, 4096);
    if (st != DOS_EXITED) {
        s_cat(g_out, " fault_ip ", 4096); cat_hex(g_out, r.fault_ip, 4096);
        s_cat(g_out, " fault_op ", 4096); cat_hex(g_out, (uint32_t)r.fault_op, 4096);
    }
    s_cat(g_out, " cpu=8086-subset dos=int21-subset\n", 4096);
    print_string(g_out);
    return st == DOS_EXITED ? 0 : 2;
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
        if (ents[i].flags == OS_DIRENT_DIR) continue;
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
        if (ents[i].flags == OS_DIRENT_DIR) continue;
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
    s_cat(g_out, " refresh_hz ", 4096); cat_u(g_out, p.refresh_hz, 4096);
    s_cat(g_out, "\n", 4096);
    print_string(g_out);
}


/* ------------------------------------------------- Lot 8: US-077 mobile */
static mui_layout_t g_mui;
static mui_state_t g_mui_state;
static int g_mui_on;
static char* g_mcap;            /* Lot 9: capture for the VBE desktop hook */
static int g_mcap_max;

static void mprint(const char* t) {
    int n, k = 0;
    if (!g_mcap) { print_string(t); return; }
    n = 0;
    while (g_mcap[n]) n++;
    while (t[k] && n < g_mcap_max - 1) g_mcap[n++] = t[k++];
    g_mcap[n] = 0;
}

static void mui_line(const char* what, int act) {
    g_out[0] = 0;
    s_cat(g_out, "mobile-ui ", 4096); s_cat(g_out, what, 4096);
    s_cat(g_out, " action ", 4096); s_cat(g_out, mui_action_name(act), 4096);
    s_cat(g_out, " screen ", 4096);
    s_cat(g_out, g_mui_state.screen == MUI_SCREEN_HOME ? "home" : g_mui_state.screen == MUI_SCREEN_APP ? "app" : "recents", 4096);
    if (g_mui_state.screen == MUI_SCREEN_APP) { s_cat(g_out, " app ", 4096); s_cat(g_out, mui_app_name(g_mui_state.app), 4096); }
    s_cat(g_out, " depth ", 4096); cat_u(g_out, (uint32_t)g_mui_state.depth, 4096);
    s_cat(g_out, g_mui_state.shade ? " shade 1" : " shade 0", 4096);
    s_cat(g_out, " frame ", 4096); cat_hex(g_out, mui_frame_hash(&g_mui, &g_mui_state), 4096);
    s_cat(g_out, "\n", 4096);
    mprint(g_out);
}

/* mobile-ui WxH | mobile-ui tap X Y | mobile-ui swipe X0 Y0 X1 Y1 |
 * mobile-ui app NAME (taps the icon of NAME) */
static int cmd_mobile(const char* rest) {
    char w[24], a[16], b[16], c[16], d[16];
    int ok1, ok2, ok3, ok4;
    rest = word(rest, w, (int)sizeof(w));
    if (w[0] >= '0' && w[0] <= '9') {
        int i = 0, W, H;
        char ws[8], hs[8];
        int j = 0;
        while (w[i] && w[i] != 'x' && j < 7) ws[j++] = w[i++];
        ws[j] = 0;
        if (w[i] == 'x') i++;
        j = 0;
        while (w[i] && j < 7) hs[j++] = w[i++];
        hs[j] = 0;
        W = to_int(ws, &ok1); H = to_int(hs, &ok2);
        if (!ok1 || !ok2 || mui_layout(W, H, &g_mui) != 0) { mprint("mobile-ui error screen too small or bad size\n"); return 1; }
        mui_init(&g_mui_state);
        g_mui_on = 1;
        g_out[0] = 0;
        s_cat(g_out, "mobile-ui ok form ", 4096); s_cat(g_out, plat_form_name(g_mui.form), 4096);
        s_cat(g_out, g_mui.portrait ? " portrait" : " landscape", 4096);
        s_cat(g_out, " grid ", 4096); cat_u(g_out, (uint32_t)g_mui.cols, 4096); s_cat(g_out, "x", 4096); cat_u(g_out, (uint32_t)g_mui.rows, 4096);
        s_cat(g_out, " icon ", 4096); cat_u(g_out, (uint32_t)g_mui.icon, 4096);
        s_cat(g_out, " min_target ", 4096); cat_u(g_out, (uint32_t)g_mui.min_target, 4096);
        s_cat(g_out, " targets ", 4096); cat_u(g_out, (uint32_t)g_mui.ntargets, 4096);
        s_cat(g_out, " status ", 4096); cat_u(g_out, (uint32_t)g_mui.status_h, 4096);
        s_cat(g_out, " nav ", 4096); cat_u(g_out, (uint32_t)g_mui.nav_h, 4096);
        s_cat(g_out, " frame ", 4096); cat_hex(g_out, mui_frame_hash(&g_mui, &g_mui_state), 4096);
        s_cat(g_out, " input simule (no touch hardware)\n", 4096);
        mprint(g_out);
        return 0;
    }
    if (!g_mui_on) { mprint("mobile-ui error usage: mobile-ui WxH first\n"); return 1; }
    if (s_eq(w, "tap")) {
        rest = word(rest, a, 16); rest = word(rest, b, 16);
        int x = to_int(a, &ok1), y = to_int(b, &ok2);
        if (!ok1 || !ok2) { mprint("mobile-ui error usage: mobile-ui tap X Y\n"); return 1; }
        mui_line("tap", mui_tap(&g_mui_state, &g_mui, x, y));
        return 0;
    }
    if (s_eq(w, "app")) {
        int i, id = -1;
        rest = word(rest, a, 16);
        for (i = 0; i < MUI_APPS; i++) if (s_eq(a, mui_app_name(i))) id = i;
        if (id < 0) { mprint("mobile-ui error unknown app\n"); return 1; }
        mui_line("tap", mui_tap(&g_mui_state, &g_mui, g_mui.t[id].x + g_mui.t[id].w / 2, g_mui.t[id].y + g_mui.t[id].h / 2));
        return 0;
    }
    if (s_eq(w, "swipe")) {
        plat_point_t p[4];
        int x0, y0, x1, y1;
        rest = word(rest, a, 16); rest = word(rest, b, 16); rest = word(rest, c, 16); rest = word(rest, d, 16);
        x0 = to_int(a, &ok1); y0 = to_int(b, &ok2); x1 = to_int(c, &ok3); y1 = to_int(d, &ok4);
        if (!ok1 || !ok2 || !ok3 || !ok4) { mprint("mobile-ui error usage: mobile-ui swipe X0 Y0 X1 Y1\n"); return 1; }
        p[0].x = x0; p[0].y = y0; p[0].t_ms = 0U; p[0].down = 1;
        p[1].x = (x0 + x1) / 2; p[1].y = (y0 + y1) / 2; p[1].t_ms = 80U; p[1].down = 1;
        p[2].x = x1; p[2].y = y1; p[2].t_ms = 160U; p[2].down = 1;
        p[3].x = x1; p[3].y = y1; p[3].t_ms = 170U; p[3].down = 0;
        mui_line("swipe", mui_gesture(&g_mui_state, &g_mui, p, 4));
        return 0;
    }
    mprint("mobile-ui error usage: mobile-ui WxH | tap X Y | app NAME | swipe X0 Y0 X1 Y1\n");
    return 1;
}

/* Lot 9 (US-077): "mobile-ui ARGS" typed on the OS-UI VBE desktop. Same
 * state as the shell command; the frame is sent to the web pane as a
 * display list (mui_display_list) and painted by the kernel on the LFB. */
static os_fb_web_item_t g_mui_dl[OS_FB_WEB_ITEMS];

int shell_mobile_gui(const char* args, char* out, int out_max) {
    char title[48];
    int rc, n, k, nt = 0;
    if (!out || out_max <= 0) return -1;
    out[0] = 0;
    g_mcap = out; g_mcap_max = out_max;
    rc = cmd_mobile(args && *args ? args : "320x448");
    g_mcap = 0;
    if (!g_mui_on) return rc;
    n = mui_display_list(&g_mui, &g_mui_state, g_mui_dl, OS_FB_WEB_ITEMS);
    if (n < 0) return 1;
    for (k = 0; k < n; k++) if (g_mui_dl[k].kind == OS_FB_WEB_TEXT) nt++;
    title[0] = 0;
    s_cat(title, plat_form_name(g_mui.form), 48); s_cat(title, " ", 48);
    s_cat(title, g_mui_state.screen == MUI_SCREEN_HOME ? "home" : g_mui_state.screen == MUI_SCREEN_APP ? mui_app_name(g_mui_state.app) : "recents", 48);
    if (g_mui_state.shade) s_cat(title, " shade", 48);
    osui_gui_show_items(title, g_mui_dl, n, g_mui.w, g_mui.h);
    g_out[0] = 0;
    s_cat(g_out, "mobile-ui vbe pane web items ", 4096); cat_u(g_out, (uint32_t)n, 4096);
    s_cat(g_out, " text ", 4096); cat_u(g_out, (uint32_t)nt, 4096);
    s_cat(g_out, " page ", 4096); cat_u(g_out, (uint32_t)g_mui.w, 4096); s_cat(g_out, "x", 4096); cat_u(g_out, (uint32_t)g_mui.h, 4096);
    s_cat(g_out, " title ", 4096); s_cat(g_out, title, 4096);
    s_cat(g_out, "\n", 4096);
    g_mcap = out; mprint(g_out); g_mcap = 0;
    return rc;
}

/* ------------------------------------------------ Lot 8: US-083 devsync */
static ds_replica_t g_ds[3];
static int g_ds_ready;

static ds_replica_t* ds_dev(const char* name) {
    int i;
    if (!g_ds_ready) { for (i = 0; i < 3; i++) ds_init(&g_ds[i], i + 1); g_ds_ready = 1; }
    if (name[0] >= 'A' && name[0] <= 'C' && !name[1]) return &g_ds[name[0] - 'A'];
    return 0;
}

static void ds_show(const char* name, ds_replica_t* r) {
    int i;
    g_out[0] = 0;
    s_cat(g_out, "devsync-show ", 4096); s_cat(g_out, name, 4096);
    s_cat(g_out, " live ", 4096); cat_u(g_out, (uint32_t)ds_live(r), 4096);
    s_cat(g_out, " clock ", 4096); cat_u(g_out, r->clock, 4096);
    s_cat(g_out, " vv ", 4096);
    for (i = 1; i <= 3; i++) { cat_u(g_out, r->vv[i], 4096); if (i < 3) s_cat(g_out, ",", 4096); }
    s_cat(g_out, " conflicts ", 4096); cat_u(g_out, r->conflicts, 4096);
    s_cat(g_out, " digest ", 4096); cat_hex(g_out, ds_digest(r), 4096);
    for (i = 0; i < r->n; i++) {
        if (r->e[i].dead) continue;
        s_cat(g_out, " ", 4096); s_cat(g_out, r->e[i].key, 4096);
        s_cat(g_out, "=", 4096); s_cat(g_out, r->e[i].val, 4096);
    }
    s_cat(g_out, "\n", 4096);
    print_string(g_out);
}

/* Lot 9 (US-083): devsync between two guests over the phase 5 P2P link
 * (sealed P2P_I_APP payloads, X25519 + AES-128-GCM per pair).
 *   'D' 'Q' RDEV vv[8] (32 bytes LE)  pull request: delta of RDEV for vv
 *   'D' 'R' RDEV text                 reply: ds_export text (sum checked)
 * Replies are answered from the idle P2P pump, so the peer shell can sit
 * at its prompt. One datagram per delta (at most 396 bytes of text). */
#define DS_NET_TEXT 396
static volatile int g_dsn_wait;          /* 1 waiting, 2 got reply */
static uint32_t g_dsn_from;
static char g_dsn_text[DS_NET_TEXT + 1];
static uint32_t g_dsn_served, g_dsn_bytes_out;
static void ds_put32(uint8_t* p, uint32_t v) { p[0] = (uint8_t)v; p[1] = (uint8_t)(v >> 8); p[2] = (uint8_t)(v >> 16); p[3] = (uint8_t)(v >> 24); }
static uint32_t ds_get32(const uint8_t* p) { return (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24); }
void shell_devsync_app(uint32_t from, const uint8_t* d, int len) {
    static uint8_t reply[400];
    char nm[2];
    ds_replica_t* r;
    int i, n;
    if (len < 3 || d[0] != 'D') return;
    nm[0] = (char)d[2]; nm[1] = 0;
    r = ds_dev(nm);
    if (!r) return;
    if (d[1] == 'Q' && len == 3 + 4 * DS_DEVICES) {
        uint32_t vv[DS_DEVICES];
        for (i = 0; i < DS_DEVICES; i++) vv[i] = ds_get32(d + 3 + 4 * i);
        n = ds_export(r, vv, g_big, DS_NET_TEXT + 1);
        if (n <= 0 || n > DS_NET_TEXT) return;          /* too large for one datagram */
        reply[0] = 'D'; reply[1] = 'R'; reply[2] = d[2];
        for (i = 0; i < n; i++) reply[3 + i] = (uint8_t)g_big[i];
        if (p2p_send_app(shell_p2p_node(), shell_p2p_host(), from, reply, 3 + n) > 0) { g_dsn_served++; g_dsn_bytes_out += (uint32_t)n; }
        return;
    }
    if (d[1] == 'R' && g_dsn_wait == 1 && len - 3 <= DS_NET_TEXT) {
        for (i = 0; i < len - 3; i++) g_dsn_text[i] = (char)d[3 + i];
        g_dsn_text[len - 3] = 0;
        g_dsn_from = from;
        g_dsn_wait = 2;
    }
}

/* devsync-pull DEV PEER RDEV [SECONDS] */
static int cmd_devsync_pull(const char* rest) {
    char dv[8], peer[32], rd[8], sc[8];
    uint8_t q[3 + 4 * DS_DEVICES];
    ds_replica_t *r, *rr;
    p2p_node_t* node = shell_p2p_node();
    p2p_peer_t* p;
    unsigned t0, secs = 10;
    int i, rc;
    rest = word(rest, dv, 8); rest = word(rest, peer, 32); rest = word(rest, rd, 8); rest = word(rest, sc, 8);
    if (dv[0] >= 'a' && dv[0] <= 'z') dv[0] = (char)(dv[0] - 32);
    if (rd[0] >= 'a' && rd[0] <= 'z') rd[0] = (char)(rd[0] - 32);
    if (sc[0]) { secs = 0; for (i = 0; sc[i] >= '0' && sc[i] <= '9'; i++) secs = secs * 10U + (unsigned)(sc[i] - '0'); }
    r = ds_dev(dv); rr = ds_dev(rd);
    if (!r || !rr || !peer[0] || !secs || secs > 120) {
        print_string("devsync-pull error usage: devsync-pull DEV PEER REMOTE_DEV [SECONDS]\n"); return 1;
    }
    if (!node->up) { print_string("devsync-pull error p2p down (p2p-up first)\n"); return 1; }
    p = p2p_find(node, peer);
    if (!p || !p->keyed) { print_string("devsync-pull error peer unknown or not keyed\n"); return 1; }
    node->app = shell_p2p_app; node->app_ctx = 0;
    q[0] = 'D'; q[1] = 'Q'; q[2] = (uint8_t)rd[0];
    for (i = 0; i < DS_DEVICES; i++) ds_put32(q + 3 + 4 * i, r->vv[i]);
    g_dsn_wait = 1;
    if (p2p_send_app(node, shell_p2p_host(), p->id, q, (int)sizeof(q)) <= 0) {
        g_dsn_wait = 0; print_string("devsync-pull error send\n"); return 1;
    }
    t0 = sys_ticks();
    while (g_dsn_wait == 1 && sys_ticks() - t0 < secs * 100U) { shell_p2p_poll(); (void)sys0(SYS_YIELD); }
    if (g_dsn_wait != 2) {
        g_dsn_wait = 0;
        print_string("devsync-pull error timeout (no reply or delta over one datagram)\n"); return 2;
    }
    g_dsn_wait = 0;
    rc = ds_import(r, g_dsn_text);
    g_out[0] = 0; s_cat(g_out, "devsync-pull ", 4096); s_cat(g_out, dv, 4096); s_cat(g_out, " from ", 4096);
    s_cat(g_out, p2p_peer_name(node, g_dsn_from), 4096); s_cat(g_out, ":", 4096); s_cat(g_out, rd, 4096);
    if (rc < 0) {
        s_cat(g_out, rc == -2 ? " error sum\n" : " error format\n", 4096); print_string(g_out); return 1;
    }
    s_cat(g_out, " ok delta_bytes ", 4096); cat_u(g_out, (uint32_t)s_len(g_dsn_text), 4096);
    s_cat(g_out, " applied ", 4096); cat_u(g_out, (uint32_t)rc, 4096);
    s_cat(g_out, " ms ", 4096); cat_u(g_out, (sys_ticks() - t0) * 10U, 4096);
    s_cat(g_out, " transport=p2p-sealed\n", 4096);
    print_string(g_out);
    return 0;
}

/* devsync-set DEV KEY VAL, devsync-del DEV KEY, devsync-sync SRC DST,
 * devsync-show DEV. DEV is A, B or C (or a, b, c): three device replicas inside this
 * guest (the devices are simule; the merge and the wire delta are real). */
static int cmd_devsync(const char* cmd, const char* rest) {
    char dv[8], k[DS_KEY_LEN + 4], v[DS_VAL_LEN + 4];
    ds_replica_t* r;
    rest = word(rest, dv, 8);
    if (dv[0] >= 'a' && dv[0] <= 'z') dv[0] = (char)(dv[0] - 32);   /* sendkey types lower case */
    r = ds_dev(dv);
    if (!r) { print_string("devsync error device must be A, B or C\n"); return 1; }
    if (s_eq(cmd, "devsync-set")) {
        rest = word(rest, k, (int)sizeof(k)); rest = word(rest, v, (int)sizeof(v));
        if (ds_set(r, k, v) != 0) { print_string("devsync-set error bad key or value\n"); return 1; }
        g_out[0] = 0; s_cat(g_out, "devsync-set ok ", 4096); s_cat(g_out, dv, 4096);
        s_cat(g_out, " clock ", 4096); cat_u(g_out, r->clock, 4096); s_cat(g_out, "\n", 4096);
        print_string(g_out);
        return 0;
    }
    if (s_eq(cmd, "devsync-del")) {
        rest = word(rest, k, (int)sizeof(k));
        if (ds_del(r, k) != 0) { print_string("devsync-del error unknown key\n"); return 1; }
        print_string("devsync-del ok\n");
        return 0;
    }
    if (s_eq(cmd, "devsync-sync")) {
        ds_replica_t* dst;
        int n, rc;
        rest = word(rest, k, 8);
        if (k[0] >= 'a' && k[0] <= 'z') k[0] = (char)(k[0] - 32);
        dst = ds_dev(k);
        if (!dst || dst == r) { print_string("devsync-sync error usage: devsync-sync SRC DST\n"); return 1; }
        n = ds_export(r, dst->vv, g_big, (int)sizeof(g_big));
        rc = n > 0 ? ds_import(dst, g_big) : -1;
        g_out[0] = 0; s_cat(g_out, "devsync-sync ", 4096); s_cat(g_out, dv, 4096); s_cat(g_out, " to ", 4096);
        s_cat(g_out, k, 4096);
        if (rc < 0) { s_cat(g_out, " error import rc ", 4096); s_cat(g_out, rc == -2 ? "sum" : "format", 4096); s_cat(g_out, "\n", 4096); print_string(g_out); return 1; }
        s_cat(g_out, " ok delta_bytes ", 4096); cat_u(g_out, (uint32_t)n, 4096);
        s_cat(g_out, " applied ", 4096); cat_u(g_out, (uint32_t)rc, 4096);
        s_cat(g_out, "\n", 4096);
        print_string(g_out);
        return 0;
    }
    ds_show(dv, r);
    return 0;
}

/* Lot 68 (US-080): form-factor plan applied to the real desktop scale */
p2p_node_t* shell_p2p_node(void);
static int cmd_form_factor(const char* rest) {
    char a[24], b[24], c[24];
    plat_form_plan_t pl;
    osui_scale_t* sc = osui_gui_scale();
    int w = sc->fb_w > 0 ? sc->fb_w : 1024, h = sc->fb_h > 0 ? sc->fb_h : 768, diag = 0, form = 0, apply = 0, ok = 1;
    rest = word(rest, a, (int)sizeof(a));
    if (s_eq(a, "apply")) { apply = 1; rest = word(rest, a, (int)sizeof(a)); }
    if (a[0]) {
        form = plat_form_kind_parse(a);
        if (form < 0) { print_string("form-factor error usage: form-factor [apply] [auto|phone|tablet|desktop|tv] [WxH] [DIAG_INCH_TENTHS]\n"); return 1; }
    }
    rest = word(rest, b, (int)sizeof(b));
    if (b[0]) {
        char num[8];
        int i = 0, j = 0;
        while (b[i] && b[i] != 'x' && j < 7) num[j++] = b[i++];
        num[j] = 0;
        w = to_int(num, &ok);
        if (b[i] == 'x') h = to_int(b + i + 1, &ok); else ok = 0;
    }
    rest = word(rest, c, (int)sizeof(c));
    if (c[0]) diag = to_int(c, &ok);
    if (!ok || plat_form_plan(w, h, diag, form, &pl) != 0) {
        print_string("form-factor error size WxH 160x120..8192x8192, diagonal 0..1500 tenths of an inch\n");
        return 1;
    }
    plat_form_report(&pl, g_out, (int)sizeof(g_out));
    print_string(g_out);
    if (apply) {
        char line[32], qb[16];
        os_ui_q_format(pl.ui_q, qb);
        line[0] = 0;
        s_cat(line, "ui-scale ", (int)sizeof(line));
        s_cat(line, qb, (int)sizeof(line));
        line[s_len(line) - 1] = 0;   /* "2x" -> "2" */
        g_out[0] = 0;
        (void)osui_gui_scale_command(line, g_out, (int)sizeof(g_out));
        print_string("form-factor applied ");
        print_string(g_out);
    }
    return 0;
}

/* Lot 68 (US-081): profile applied to real settings; battery simule */
static plat_energy_t g_energy;
static int g_energy_ready;
static void energy_ensure(void) {
    if (g_energy_ready) return;
    plat_energy_init(&g_energy);
    g_energy_ready = 1;
}
static void energy_push_pace(void) {
    char out[256], num[12], t[12];
    const char* av[2];
    int k = 0, j;
    uint32_t v = g_energy.pace_cap_bps;
    do { t[k++] = (char)('0' + v % 10U); v /= 10U; } while (v && k < 11);
    for (j = 0; j < k; j++) num[j] = t[k - 1 - j];
    num[k] = 0;
    av[0] = "cap"; av[1] = num;
    (void)p2p_pace_cmd(shell_p2p_node(), 2, av, out, (int)sizeof(out));
}
static int cmd_power_apply(const char* rest) {
    char a[24];
    int p;
    energy_ensure();
    word(rest, a, (int)sizeof(a));
    if (!a[0] || s_eq(a, "status")) { plat_energy_report(&g_energy, g_out, (int)sizeof(g_out)); print_string(g_out); return 0; }
    if (s_eq(a, "auto")) {
        g_energy.auto_mode = 1;
        p = plat_power_auto(busy_sample(), g_energy.sim_on && !g_energy.charging);
    } else {
        p = plat_power_parse(a);
        if (!p) { print_string("power-apply error use performance|balanced|saver|auto|status\n"); return 1; }
        g_energy.auto_mode = 0;
    }
    (void)plat_energy_apply(&g_energy, p);
    g_profile = p; g_auto = g_energy.auto_mode;
    energy_push_pace();
    plat_energy_report(&g_energy, g_out, (int)sizeof(g_out));
    print_string(g_out);
    return 0;
}
static int cmd_power_sim(const char* rest) {
    char a[24], b[24], c[24];
    int ok = 1;
    energy_ensure();
    rest = word(rest, a, (int)sizeof(a));
    rest = word(rest, b, (int)sizeof(b));
    rest = word(rest, c, (int)sizeof(c));
    if (s_eq(a, "start")) {
        int cap = b[0] ? to_int(b, &ok) : 50000, pct = c[0] ? to_int(c, &ok) : 100;
        if (!ok || plat_energy_sim_start(&g_energy, (uint32_t)cap, (uint32_t)pct) != 0) {
            print_string("power-sim error usage: power-sim start [CAP_MWH 1000..200000] [PCT 0..100]\n");
            return 1;
        }
    } else if (s_eq(a, "tick")) {
        int ms = to_int(b, &ok), busy = c[0] ? to_int(c, &ok) : (int)busy_sample();
        if (!ok || !g_energy.sim_on || ms < 0) { print_string("power-sim error usage: power-sim tick MS [BUSY] (after start)\n"); return 1; }
        if (plat_energy_sim_tick(&g_energy, (uint32_t)ms, (uint32_t)busy)) {
            g_profile = g_energy.profile;
            energy_push_pace();
            print_string("power-sim auto switch ");
            print_string(plat_power_name(g_energy.profile));
            print_string("\n");
        }
    } else if (s_eq(a, "charge")) {
        g_energy.charging = s_eq(b, "on");
    } else if (s_eq(a, "stop")) {
        plat_energy_sim_stop(&g_energy);
    } else if (a[0] && !s_eq(a, "status")) {
        print_string("power-sim error usage: power-sim start|tick|charge on|off|stop|status\n");
        return 1;
    }
    plat_energy_report(&g_energy, g_out, (int)sizeof(g_out));
    print_string(g_out);
    return 0;
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
    if (s_eq(cmd, "screen-adapt") || s_eq(cmd, "screen-resize")) {
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
        s_cat(g_out, s_eq(cmd, "screen-resize") ? "screen-resize ok " : "screen-adapt ok ", 4096);
        cat_u(g_out, (uint32_t)w, 4096); s_cat(g_out, "x", 4096); cat_u(g_out, (uint32_t)h, 4096);
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
    if (s_eq(cmd, "dev-rules")) {
        char name_p[16], cls_p[12], act[32];
        const char* r2 = word(rest, name_p, 16);
        r2 = word(r2, cls_p, 12);
        word(r2, act, 32);
        if (!name_p[0]) {
            int i;
            g_out[0] = 0;
            for (i = 0; i < g_dev_rules.n_rules; i++) {
                s_cat(g_out, "rule ", 4096); s_cat(g_out, g_dev_rules.rules[i].name_pattern, 4096);
                s_cat(g_out, " class ", 4096); s_cat(g_out, g_dev_rules.rules[i].cls_pattern, 4096);
                s_cat(g_out, " action ", 4096); s_cat(g_out, g_dev_rules.rules[i].action_cmd, 4096); s_cat(g_out, "\n", 4096);
            }
            s_cat(g_out, "dev-rules ok count ", 4096); cat_u(g_out, (uint32_t)g_dev_rules.n_rules, 4096); s_cat(g_out, "\n", 4096);
            print_string(g_out);
            return 0;
        }
        if (plat_dev_rule_add(&g_dev_rules, name_p, cls_p[0] ? cls_p : "*", act[0] ? act : "log") == 0) {
            print_string("dev-rules ok rule added\n");
            return 0;
        }
        print_string("dev-rules error capacity full\n");
        return 1;
    }
    if (s_eq(cmd, "dev-hotplug")) {
        static plat_devices_t r;
        char dname[16], dcls[12], pres_str[8], act[32];
        int pres_ok, pres_val;
        dev_probe(&r);
        const char* r2 = word(rest, dname, 16);
        r2 = word(r2, dcls, 12);
        r2 = word(r2, pres_str, 8);
        pres_val = to_int(pres_str, &pres_ok);
        if (!dname[0] || !dcls[0] || !pres_ok) {
            print_string("dev-hotplug error usage: dev-hotplug NAME CLASS 0|1 [DETAIL]\n");
            return 1;
        }
        plat_dev_hotplug_event(&r, &g_dev_rules, dname, dcls, pres_val, r2[0] ? r2 : "hotplug simulated", act, sizeof(act));
        g_out[0] = 0;
        s_cat(g_out, "dev-hotplug ok device ", 4096); s_cat(g_out, dname, 4096);
        s_cat(g_out, pres_val ? " attached" : " detached", 4096);
        s_cat(g_out, " action ", 4096); s_cat(g_out, act[0] ? act : "none", 4096); s_cat(g_out, "\n", 4096);
        print_string(g_out);
        return 0;
    }
    if (s_eq(cmd, "admin-nodes")) {
        if (g_admin_nodes.count == 0) {
            /* add local node by default */
            plat_hal_t h;
            hal_probe(&h);
            plat_admin_node_add(&g_admin_nodes, 1, "local-node", h.arch, h.mem_free_kib, busy_sample());
        }
        plat_admin_nodes_report(&g_admin_nodes, g_out, (int)sizeof(g_out));
        print_string(g_out);
        return 0;
    }
    if (s_eq(cmd, "admin-node-add")) {
        char nid_str[12], nname[16], narch[16], free_str[12], busy_str[12];
        int id_ok, free_ok, busy_ok;
        uint32_t nid, mem_free, busy;
        const char* r2 = word(rest, nid_str, 12);
        r2 = word(r2, nname, 16);
        r2 = word(r2, narch, 16);
        r2 = word(r2, free_str, 12);
        word(r2, busy_str, 12);
        nid = (uint32_t)to_int(nid_str, &id_ok);
        mem_free = (uint32_t)to_int(free_str, &free_ok);
        busy = (uint32_t)to_int(busy_str, &busy_ok);
        if (!id_ok || !nname[0] || !narch[0] || !free_ok || !busy_ok) {
            print_string("admin-node-add error usage: admin-node-add ID NAME ARCH FREE_KIB BUSY_PCT\n");
            return 1;
        }
        if (plat_admin_node_add(&g_admin_nodes, nid, nname, narch, mem_free, busy) == 0) {
            print_string("admin-node-add ok node added\n");
            return 0;
        }
        print_string("admin-node-add error max nodes reached\n");
        return 1;
    }
    if (s_eq(cmd, "gesture")) return cmd_gesture(rest);
    if (s_eq(cmd, "mobile-ui")) return cmd_mobile(rest);
    if (s_eq(cmd, "devsync-pull")) return cmd_devsync_pull(rest);
    if (s_eq(cmd, "devsync-set") || s_eq(cmd, "devsync-del") || s_eq(cmd, "devsync-sync") || s_eq(cmd, "devsync-show"))
        return cmd_devsync(cmd, rest);
    if (s_eq(cmd, "gesture-multi")) {
        plat_point_t p1[16], p2[16];
        int n = 0, g;
        char w1[48], w2[48];
        while (*rest && n < 16) {
            int v1[4], v2[4], k = 0, i = 0, ok1, ok2;
            char part[12];
            rest = word(rest, w1, 48); rest = word(rest, w2, 48);
            if (!w1[0] || !w2[0]) break;
            while (k < 4) {
                int j = 0;
                while (w1[i] && w1[i] != ',' && j < 11) part[j++] = w1[i++];
                part[j] = 0; v1[k] = to_int(part, &ok1);
                if (w1[i] == ',') i++;
                j = 0; int i2 = 0;
                while (w2[i2] && w2[i2] != ',' && j < 11) part[j++] = w2[i2++];
                part[j] = 0; v2[k] = to_int(part, &ok2);
                k++;
            }
            p1[n].x = v1[0]; p1[n].y = v1[1]; p1[n].t_ms = (uint32_t)v1[2]; p1[n].down = v1[3];
            p2[n].x = v2[0]; p2[n].y = v2[1]; p2[n].t_ms = (uint32_t)v2[2]; p2[n].down = v2[3];
            n++;
        }
        g = plat_gesture_multi(p1, p2, n);
        g_out[0] = 0; s_cat(g_out, "gesture-multi ok ", 4096); s_cat(g_out, plat_gesture_name(g), 4096);
        s_cat(g_out, " points ", 4096); cat_u(g_out, (uint32_t)n, 4096); s_cat(g_out, "\n", 4096);
        print_string(g_out);
        return 0;
    }
    if (s_eq(cmd, "sensor-read")) {
        plat_sensors_t s;
        plat_sensor_read(&s);
        plat_sensor_report(&s, g_out, (int)sizeof(g_out));
        print_string(g_out);
        return 0;
    }
    if (s_eq(cmd, "continuity-export")) {
        plat_continuity_t c;
        s_copy(c.session_id, "sess1", 16);
        s_copy(c.device_from, "node1", 16);
        s_copy(c.clipboard, "shared text clip", 128);
        s_copy(c.active_task, "shell", 32);
        c.state_seq = 100;
        plat_continuity_export(&c, g_out, (int)sizeof(g_out));
        print_string(g_out);
        return 0;
    }
    if (s_eq(cmd, "continuity-import")) {
        plat_continuity_t c;
        word(rest, a, 64);
        plat_continuity_import(a[0] ? a : "CONT1\n", &c);
        print_string("continuity-import ok session "); print_string(c.session_id); print_string("\n");
        return 0;
    }
    if (s_eq(cmd, "legacy-emul")) return cmd_legacy_emul(rest);
    if (s_eq(cmd, "form-factor")) return cmd_form_factor(rest);
    if (s_eq(cmd, "power-apply")) return cmd_power_apply(rest);
    if (s_eq(cmd, "power-sim")) return cmd_power_sim(rest);
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
            if (scan) { if (ents[i].flags == OS_DIRENT_DIR) continue; join(path, a, ents[i].name, 160); }
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
