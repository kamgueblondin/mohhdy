/* osui_panes.c - see osui_panes.h. No fake data: a source that fails
 * prints "indisponible", an absent service prints "absent". */
#include "osui_panes.h"

const char *const osui_panes_services[] = {
    "vfs", "ai-engine", "net-driver", "pci-driver", "usb-driver",
    "ps2-driver", "serial-driver", "vga-driver", "ipc-wait"
};
const int osui_panes_service_count = 9;

typedef struct { char *s; int n, max; } buf_t;

static void put_s(buf_t *b, const char *s) {
    while (s && *s && b->n < b->max - 1) b->s[b->n++] = *s++;
    b->s[b->n] = 0;
}
static void put_u(buf_t *b, unsigned v) {
    char t[12];
    int i = 0;
    do { t[i++] = (char)('0' + v % 10); v /= 10; } while (v && i < 11);
    while (i > 0 && b->n < b->max - 1) b->s[b->n++] = t[--i];
    b->s[b->n] = 0;
}
static void put_2(buf_t *b, unsigned v) { if (v < 10) put_s(b, "0"); put_u(b, v); }
static buf_t line(char lines[][OSUI_PANES_LINE], int i) {
    buf_t b;
    b.s = lines[i]; b.n = 0; b.max = OSUI_PANES_LINE;
    b.s[0] = 0;
    return b;
}

static int fill_status(const osui_pane_src_t *src, char lines[][OSUI_PANES_LINE], int max) {
    int n = 0, i, k;
    buf_t b;
    os_meminfo_t mi;
    static os_proc_t procs[16];
    if (n < max) {
        unsigned t = src->ticks ? src->ticks() : 0, sec = t / 100;
        b = line(lines, n++);
        put_s(&b, "uptime ");
        put_u(&b, sec / 3600); put_s(&b, ":"); put_2(&b, (sec % 3600) / 60); put_s(&b, ":"); put_2(&b, sec % 60);
        put_s(&b, "  (PIT ticks "); put_u(&b, t); put_s(&b, ")");
    }
    if (n < max) {
        b = line(lines, n++);
        if (src->meminfo && src->meminfo(&mi) == 0) {
            put_s(&b, "memoire pages: "); put_u(&b, mi.used_pages); put_s(&b, " utilisees / ");
            put_u(&b, mi.total_pages); put_s(&b, " ("); put_u(&b, mi.free_pages); put_s(&b, " libres, 4 KB)");
        } else put_s(&b, "memoire: indisponible");
    }
    k = src->ps ? src->ps(procs, 16) : -1;
    if (n < max) {
        b = line(lines, n++);
        if (k < 0) { put_s(&b, "taches: indisponible"); k = 0; }
        else { put_s(&b, "taches noyau: "); put_u(&b, (unsigned)k); put_s(&b, " (ps)"); }
    }
    for (i = 0; i < k && n < max; i++) {
        b = line(lines, n++);
        put_s(&b, "  pid "); put_u(&b, (unsigned)procs[i].pid);
        put_s(&b, procs[i].type == OS_TASK_USER ? " user " : " kern ");
        put_s(&b, procs[i].name);
    }
    return n;
}

static int fill_fs(const osui_pane_src_t *src, char lines[][OSUI_PANES_LINE], int max) {
    static os_dirent_t ents[32];
    int n = 0, i, k = src->listdir ? src->listdir("/", ents, 32) : -1;
    buf_t b;
    if (max <= 0) return 0;
    b = line(lines, n++);
    if (k < 0) { put_s(&b, "ls /: indisponible"); return n; }
    put_s(&b, "ls /  ("); put_u(&b, (unsigned)k); put_s(&b, " entrees, sys_listdir)");
    for (i = 0; i < k && n < max; i++) {
        b = line(lines, n++);
        if (ents[i].flags == OS_DIRENT_DIR) { put_s(&b, "  d  "); put_s(&b, ents[i].name); put_s(&b, "/"); }
        else { put_s(&b, "  -  "); put_s(&b, ents[i].name); put_s(&b, "  "); put_u(&b, ents[i].size); }
    }
    if (k > max - 1 && max > 1) {
        b = line(lines, max - 1);
        put_s(&b, "  ... ("); put_u(&b, (unsigned)(k - (max - 2))); put_s(&b, " de plus: ls /)");
    }
    return n;
}

static int fill_admin(const osui_pane_src_t *src, char lines[][OSUI_PANES_LINE], int max) {
    int n = 0, i, up = 0;
    buf_t b;
    if (max <= 0) return 0;
    n++;  /* header, written last with the count */
    for (i = 0; i < osui_panes_service_count && n < max; i++) {
        os_service_status_t st;
        int rc = src->service_status ? src->service_status(osui_panes_services[i], &st) : -1;
        b = line(lines, n++);
        put_s(&b, "  "); put_s(&b, osui_panes_services[i]); put_s(&b, ": ");
        if (rc == 0) {
            up++;
            put_s(&b, "actif pid "); put_u(&b, (unsigned)st.owner_pid);
            put_s(&b, " file "); put_u(&b, st.queued_messages);
        } else if (i == 0) {
            /* the vfs server is transient: the FAT commands start it for
             * one request; files go through the kernel VFS otherwise */
            put_s(&b, "a la demande (non resident, fichiers via le noyau)");
        } else put_s(&b, "absent");
    }
    b = line(lines, 0);
    put_s(&b, "services (service-status): "); put_u(&b, (unsigned)up); put_s(&b, " actifs / ");
    put_u(&b, (unsigned)osui_panes_service_count);
    return n;
}

const char *osui_panes_command(int pane) {
    switch (pane) {
    case OS_FB_PANE_P2P: return "p2p-stats";
    case OS_FB_PANE_COLLAB: return "collab-balances";
    case OS_FB_PANE_PROD: return "prod-metrics";
    case OS_FB_PANE_SUPPORT: return "prod-diag";
    default: return 0;
    }
}

/* "$ cmd" then the captured output, one window line per output line
 * (long lines wrap). */
static int fill_cmd(const osui_pane_src_t *src, const char *cmd, char lines[][OSUI_PANES_LINE], int max) {
    static char outb[2048];
    int n = 0, k, i, col = 0;
    buf_t b = line(lines, n++);
    put_s(&b, "$ "); put_s(&b, cmd); put_s(&b, "  (sortie du shell)");
    k = src->run ? src->run(cmd, outb, (int)sizeof(outb)) : -1;
    if (k < 0) { if (n < max) { b = line(lines, n++); put_s(&b, "  indisponible"); } return n; }
    if (k == 0) { if (n < max) { b = line(lines, n++); put_s(&b, "  (aucune sortie)"); } return n; }
    if (n >= max) return n;
    b = line(lines, n++);
    for (i = 0; i < k && outb[i]; i++) {
        char c = outb[i];
        if (c == '\r') continue;
        if (c == '\n' || col >= OSUI_PANES_LINE - 1) {
            if (n >= max) return n;
            b = line(lines, n++);
            col = 0;
            if (c == '\n') continue;
        }
        if (c == '\t') c = ' ';
        if (c < 32 || c > 126) continue;
        if (b.n < b.max - 1) { b.s[b.n++] = c; b.s[b.n] = 0; col++; }
    }
    if (n > 1 && lines[n - 1][0] == 0) n--;
    return n;
}

/* Browser-OS: the local pages the web-render engine can show. */
static int fill_browser(const osui_pane_src_t *src, char lines[][OSUI_PANES_LINE], int max) {
    static os_dirent_t ents[32];
    int n = 0, i, k = src->listdir ? src->listdir("/web", ents, 32) : -1;
    buf_t b = line(lines, n++);
    put_s(&b, "Pages locales /web : tapez web-render /web/<page>");
    if (k < 0) { if (n < max) { b = line(lines, n++); put_s(&b, "  /web indisponible"); } return n; }
    for (i = 0; i < k && n < max; i++) {
        if (ents[i].flags == OS_DIRENT_DIR) continue;
        b = line(lines, n++);
        put_s(&b, "  /web/"); put_s(&b, ents[i].name); put_s(&b, "  "); put_u(&b, ents[i].size); put_s(&b, " o");
    }
    return n;
}

int osui_panes_fill(int pane, const osui_pane_src_t *src, char lines[][OSUI_PANES_LINE], int max) {
    if (!src || !lines || max <= 0) return 0;
    if (osui_panes_command(pane)) return fill_cmd(src, osui_panes_command(pane), lines, max);
    if (pane == OS_FB_PANE_BROWSER) return fill_browser(src, lines, max);
    if (pane == OS_FB_PANE_STATUS) return fill_status(src, lines, max);
    if (pane == OS_FB_PANE_FS) return fill_fs(src, lines, max);
    if (pane == OS_FB_PANE_ADMIN) return fill_admin(src, lines, max);
    return 0;
}
