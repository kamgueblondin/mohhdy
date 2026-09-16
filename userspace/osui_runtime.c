/* osui_runtime.c - Port OS-UI (chat, scene, sessions, droits, MCP, FS) en C
 * freestanding Ring 3. llm=stub_echo. Pas HTML #ai-stage. Pas Chromium.
 * Pas un bash Linux. Source de verite : meme vocabulaire que shell.c.
 */

#include "osui_runtime.h"
#include "mohhdy_osui_bridge.h"

#define OSUI_MAX_SESSIONS 8
#define OSUI_MAX_MSGS 12
#define OSUI_MAX_CAPS 12
#define OSUI_MAX_INVOICES 8
#define OSUI_MAX_JOURNAL 12
#define OSUI_MAX_FS 6
#define OSUI_ID 12
#define OSUI_TEXT 160
#define OSUI_CAP 32
#define OSUI_STAGE_ROWS 8
#define OSUI_STAGE_COLS 48
#define OSUI_MAX_ARGS 12
#define OSUI_KIND 16
#define OSUI_PANE 16

#define ST_OPEN 0
#define ST_WAIT 1
#define ST_HUMAN 2
#define ST_CLOSED 3

#define OSUI_OK 0
#define OSUI_ERR 1

static const char *const k_linux_traps[] = {
    "bash", "sh", "zsh", "apt", "apt-get", "yum", "dnf", "dpkg", "sudo",
    "systemctl", "chmod", "chown", "uname", "docker", "systemd", 0
};

static const char *const k_cmds[] = {
    "session-new", "session-use", "session-status", "session-list",
    "chat", "prompt",
    "grant", "revoke", "escalate", "takeover", "admin-status",
    "origin-check",
    "browser-click", "browser-type", "browser-pointer", "browser-status",
    "mcp-invoice", "mcp-invoke",
    "fs-list", "fs-read", "fs-write",
    "stage", "stage-prompt",
    "os-help", "os-status", "os-browser", "os-shell", "os-admin",
    "os-support", "os-fs", "os-center", "os-close",
    "guest-status", "attach", "detach", "open",
    "gui", "graphics", "desktop", "console", "gui-status", "gui-exit",
    "gui-move",
    0
};

static const char *const k_caps[] = {
    "chat.reply", "site.explain", "session.escalate",
    "admin.observe", "admin.takeover",
    "dom.click", "dom.type", "pointer.move",
    "mcp.invoice.create",
    0
};

static const char *const k_selectors[] = {
    "#menu-toggle", "#menu-invoices", "#invoice-customer",
    "#invoice-amount", "#invoice-submit",
    0
};

static const char *const k_declared_mcp[] = {
    "mcp.invoice.create",
    0
};

typedef struct {
    int used;
    char id[OSUI_ID];
    char site[32];
    int status;
    int handoff;
    int n_caps;
    char caps[OSUI_MAX_CAPS][OSUI_CAP];
    int n_msgs;
    char msgs[OSUI_MAX_MSGS][OSUI_TEXT];
} osui_session_t;

typedef struct {
    int used;
    char id[OSUI_ID];
    char session[OSUI_ID];
    char customer[48];
    char amount[24];
} osui_invoice_t;

typedef struct {
    int used;
    char request[OSUI_ID];
    char session[OSUI_ID];
    char tool[OSUI_CAP];
    char outcome[24];
} osui_journal_t;

typedef struct {
    int used;
    int is_dir;
    char path[48];
    char content[96];
} osui_fs_t;

typedef struct {
    osui_session_t sessions[OSUI_MAX_SESSIONS];
    int n_sessions;
    int current;
    unsigned next_sid;
    unsigned next_rid;
    unsigned next_iid;
    char chat_mode[12];
    int menu_open;
    char focused[24];
    int px;
    int py;
    char form_customer[48];
    char form_amount[24];
    int form_submitted;
    char stage_mode[16];
    char stage_prompt[OSUI_TEXT];
    char stage_kind[OSUI_KIND];
    int scripts_stripped;
    char stage_rows[OSUI_STAGE_ROWS][OSUI_STAGE_COLS];
    char canvas[OSUI_CANVAS_ROWS][OSUI_CANVAS_COLS];
    char pane[OSUI_PANE];
    int chat_x;
    int chat_y;
    int gui_enter;
    int gui_leave;
    int stage_tick;
    int stage_autonomous;
    osui_invoice_t invoices[OSUI_MAX_INVOICES];
    int n_invoices;
    osui_journal_t journal[OSUI_MAX_JOURNAL];
    int n_journal;
    osui_fs_t fs[OSUI_MAX_FS];
    int n_fs;
    int kb_loaded;
} osui_state_t;

static osui_state_t G;

static int s_len(const char *s) {
    int n = 0;
    if (!s) return 0;
    while (s[n]) n++;
    return n;
}

static int s_cmp(const char *a, const char *b) {
    int i = 0;
    if (!a) a = "";
    if (!b) b = "";
    while (a[i] && b[i]) {
        if (a[i] != b[i]) return a[i] - b[i];
        i++;
    }
    return a[i] - b[i];
}

static int s_ncmp(const char *a, const char *b, int n) {
    int i;
    if (!a) a = "";
    if (!b) b = "";
    for (i = 0; i < n; i++) {
        if (a[i] != b[i]) return (unsigned char)a[i] - (unsigned char)b[i];
        if (!a[i]) return 0;
    }
    return 0;
}

static void s_cpy(char *d, int max, const char *s) {
    int i = 0;
    if (!s) s = "";
    if (max <= 0) return;
    while (s[i] && i < max - 1) {
        d[i] = s[i];
        i++;
    }
    d[i] = 0;
}

static char s_low(char c) {
    if (c >= 'A' && c <= 'Z') return (char)(c + 32);
    return c;
}

static int s_has_ci(const char *h, const char *n) {
    int i, j, hn, nn;
    if (!h || !n || !n[0]) return 0;
    hn = s_len(h);
    nn = s_len(n);
    if (nn > hn) return 0;
    for (i = 0; i <= hn - nn; i++) {
        for (j = 0; j < nn; j++) {
            if (s_low(h[i + j]) != s_low(n[j])) break;
        }
        if (j == nn) return 1;
    }
    return 0;
}

static void out_add(char *out, int max, int *pos, const char *s) {
    if (!s) return;
    while (*s && *pos < max - 1) out[(*pos)++] = *s++;
    if (max > 0) out[*pos] = 0;
}

static void out_u(char *out, int max, int *pos, unsigned v) {
    char tmp[16];
    char rev[16];
    int n = 0;
    int r = 0;
    if (v == 0) {
        tmp[n++] = '0';
    } else {
        while (v && r < 15) {
            rev[r++] = (char)('0' + (v % 10));
            v /= 10;
        }
        while (r) tmp[n++] = rev[--r];
    }
    tmp[n] = 0;
    out_add(out, max, pos, tmp);
}

static void make_id(char *dst, char prefix, unsigned n) {
    dst[0] = prefix;
    dst[1] = (char)('0' + ((n / 1000) % 10));
    dst[2] = (char)('0' + ((n / 100) % 10));
    dst[3] = (char)('0' + ((n / 10) % 10));
    dst[4] = (char)('0' + (n % 10));
    dst[5] = 0;
}

static int in_list(const char *const *list, const char *name) {
    int i;
    if (!name) return 0;
    for (i = 0; list[i]; i++) {
        if (s_cmp(list[i], name) == 0) return 1;
    }
    return 0;
}

static osui_session_t *cur(void) {
    if (G.current < 0 || G.current >= OSUI_MAX_SESSIONS) return 0;
    if (!G.sessions[G.current].used) return 0;
    return &G.sessions[G.current];
}

static osui_session_t *find_sid(const char *id) {
    int i;
    for (i = 0; i < OSUI_MAX_SESSIONS; i++) {
        if (G.sessions[i].used && s_cmp(G.sessions[i].id, id) == 0)
            return &G.sessions[i];
    }
    return 0;
}

static int has_cap(const osui_session_t *s, const char *cap) {
    int i;
    if (!s) return 0;
    for (i = 0; i < s->n_caps; i++) {
        if (s_cmp(s->caps[i], cap) == 0) return 1;
    }
    return 0;
}

static void add_cap(osui_session_t *s, const char *cap) {
    if (!s || !cap || !cap[0]) return;
    if (!in_list(k_caps, cap)) return;
    if (has_cap(s, cap)) return;
    if (s->n_caps >= OSUI_MAX_CAPS) return;
    s_cpy(s->caps[s->n_caps], OSUI_CAP, cap);
    s->n_caps++;
}

static void drop_cap(osui_session_t *s, const char *cap) {
    int i, j;
    if (!s) return;
    for (i = 0; i < s->n_caps; i++) {
        if (s_cmp(s->caps[i], cap) == 0) {
            for (j = i; j < s->n_caps - 1; j++)
                s_cpy(s->caps[j], OSUI_CAP, s->caps[j + 1]);
            s->n_caps--;
            return;
        }
    }
}

static void add_msg(osui_session_t *s, const char *text) {
    if (!s || !text) return;
    if (s->n_msgs >= OSUI_MAX_MSGS) {
        int i;
        for (i = 0; i < OSUI_MAX_MSGS - 1; i++)
            s_cpy(s->msgs[i], OSUI_TEXT, s->msgs[i + 1]);
        s->n_msgs = OSUI_MAX_MSGS - 1;
    }
    s_cpy(s->msgs[s->n_msgs], OSUI_TEXT, text);
    s->n_msgs++;
}

static unsigned new_rid(void) {
    G.next_rid++;
    return G.next_rid;
}

static void journal(const char *sid, const char *tool, const char *outcome, unsigned rid) {
    osui_journal_t *j;
    if (G.n_journal >= OSUI_MAX_JOURNAL) {
        int i;
        for (i = 0; i < OSUI_MAX_JOURNAL - 1; i++)
            G.journal[i] = G.journal[i + 1];
        G.n_journal = OSUI_MAX_JOURNAL - 1;
    }
    j = &G.journal[G.n_journal++];
    j->used = 1;
    make_id(j->request, 'r', rid);
    s_cpy(j->session, OSUI_ID, sid ? sid : "");
    s_cpy(j->tool, OSUI_CAP, tool ? tool : "");
    s_cpy(j->outcome, 24, outcome ? outcome : "");
}

static void emit_rid(char *out, int max, int *pos, unsigned rid) {
    out_add(out, max, pos, " request_id=r");
    if (rid < 1000) out_add(out, max, pos, "0");
    if (rid < 100) out_add(out, max, pos, "0");
    if (rid < 10) out_add(out, max, pos, "0");
    out_u(out, max, pos, rid);
}

static const char *st_name(int st) {
    if (st == ST_WAIT) return "waiting_human";
    if (st == ST_HUMAN) return "human_active";
    if (st == ST_CLOSED) return "closed";
    return "open";
}

static void fs_seed(void) {
    G.n_fs = 0;
    G.fs[G.n_fs].used = 1;
    G.fs[G.n_fs].is_dir = 1;
    s_cpy(G.fs[G.n_fs].path, 48, "demo/");
    G.fs[G.n_fs].content[0] = 0;
    G.n_fs++;
    G.fs[G.n_fs].used = 1;
    G.fs[G.n_fs].is_dir = 0;
    s_cpy(G.fs[G.n_fs].path, 48, "demo/hello.txt");
    s_cpy(G.fs[G.n_fs].content, 96, "hello from guest FS sandbox\n");
    G.n_fs++;
    G.fs[G.n_fs].used = 1;
    G.fs[G.n_fs].is_dir = 0;
    s_cpy(G.fs[G.n_fs].path, 48, "demo/invoice.txt");
    s_cpy(G.fs[G.n_fs].content, 96, "invoice stub\n");
    G.n_fs++;
    G.fs[G.n_fs].used = 1;
    G.fs[G.n_fs].is_dir = 1;
    s_cpy(G.fs[G.n_fs].path, 48, "data/");
    G.fs[G.n_fs].content[0] = 0;
    G.n_fs++;
    G.fs[G.n_fs].used = 1;
    G.fs[G.n_fs].is_dir = 0;
    s_cpy(G.fs[G.n_fs].path, 48, "data/notes.txt");
    s_cpy(G.fs[G.n_fs].content, 96, "notes sandbox\n");
    G.n_fs++;
}

static osui_session_t *alloc_session(const char *site) {
    int i;
    osui_session_t *s;
    for (i = 0; i < OSUI_MAX_SESSIONS; i++) {
        if (!G.sessions[i].used) break;
    }
    if (i >= OSUI_MAX_SESSIONS) return 0;
    s = &G.sessions[i];
    s->used = 1;
    G.next_sid++;
    make_id(s->id, 's', G.next_sid);
    s_cpy(s->site, 32, site && site[0] ? site : "default");
    s->status = ST_OPEN;
    s->handoff = 0;
    s->n_caps = 0;
    s->n_msgs = 0;
    add_cap(s, "chat.reply");
    add_cap(s, "site.explain");
    add_cap(s, "session.escalate");
    G.n_sessions++;
    G.current = i;
    return s;
}

static void stage_clear(void) {
    int r, c;
    for (r = 0; r < OSUI_STAGE_ROWS; r++) {
        for (c = 0; c < OSUI_STAGE_COLS; c++) G.stage_rows[r][c] = ' ';
        G.stage_rows[r][OSUI_STAGE_COLS - 1] = 0;
    }
}

static void canvas_clear(void) {
    int r, c;
    for (r = 0; r < OSUI_CANVAS_ROWS; r++) {
        for (c = 0; c < OSUI_CANVAS_COLS; c++) G.canvas[r][c] = ' ';
        G.canvas[r][OSUI_CANVAS_COLS - 1] = 0;
    }
}

static void canvas_put(int r, int c, char ch) {
    if (r < 0 || r >= OSUI_CANVAS_ROWS) return;
    if (c < 0 || c >= OSUI_CANVAS_COLS - 1) return;
    G.canvas[r][c] = ch;
}

static void canvas_text(int r, int c, const char *s) {
    int i = 0;
    if (!s) return;
    while (s[i]) {
        canvas_put(r, c + i, s[i]);
        i++;
    }
}

static void canvas_box(int r, int c, int h, int w) {
    int i;
    if (h < 2 || w < 2) return;
    canvas_put(r, c, '+');
    canvas_put(r, c + w - 1, '+');
    canvas_put(r + h - 1, c, '+');
    canvas_put(r + h - 1, c + w - 1, '+');
    for (i = 1; i < w - 1; i++) {
        canvas_put(r, c + i, '-');
        canvas_put(r + h - 1, c + i, '-');
    }
    for (i = 1; i < h - 1; i++) {
        canvas_put(r + i, c, '|');
        canvas_put(r + i, c + w - 1, '|');
    }
}

static void canvas_circle(int cr, int cc, int rad) {
    int x, y, p;
    x = 0;
    y = rad;
    p = 1 - rad;
    while (x <= y) {
        canvas_put(cr + y, cc + x, '*');
        canvas_put(cr + y, cc - x, '*');
        canvas_put(cr - y, cc + x, '*');
        canvas_put(cr - y, cc - x, '*');
        canvas_put(cr + x, cc + y, '*');
        canvas_put(cr + x, cc - y, '*');
        canvas_put(cr - x, cc + y, '*');
        canvas_put(cr - x, cc - y, '*');
        x++;
        if (p < 0) p += 2 * x + 1;
        else {
            y--;
            p += 2 * (x - y) + 1;
        }
    }
}

static void canvas_graph(void) {
    static const char *bars = " .:;+=xX#";
    int c, h;
    canvas_text(1, 2, "graphe stub (pas un moteur HTML)");
    for (c = 2; c < 40; c++) {
        h = 2 + ((c * 3) % 8);
        canvas_put(12 - h, c, bars[h % 9]);
        canvas_put(12, c, '_');
    }
}

static void canvas_tree(void) {
    canvas_text(2, 4, "demo/");
    canvas_text(3, 6, "|-- hello.txt");
    canvas_text(4, 6, "|-- invoice.txt");
    canvas_text(5, 4, "data/");
    canvas_text(6, 6, "`-- notes.txt");
}

static void canvas_clock(void) {
    canvas_box(2, 20, 9, 21);
    canvas_text(4, 24, "  12  ");
    canvas_text(6, 22, "9   o   3");
    canvas_text(8, 24, "  6   ");
    canvas_text(11, 18, "horloge stub llm=stub_echo");
}

static void canvas_sim(int tick) {
    int pos = tick % 40;
    canvas_text(2, 2, "simulation stub  acte1 -> acte2 -> resultat");
    canvas_text(4, 2, "[");
    canvas_put(4, 3 + pos, '>');
    canvas_text(4, 44, "]");
    canvas_put(8, 8 + (tick % 20), 'o');
    canvas_text(10, 2, "pas Chromium  us031_complete=false");
}

static void stage_put(int r, int c, const char *s) {
    int i = 0;
    if (r < 0 || r >= OSUI_STAGE_ROWS) return;
    if (c < 0) c = 0;
    while (s[i] && c + i < OSUI_STAGE_COLS - 1) {
        G.stage_rows[r][c + i] = s[i];
        i++;
    }
}

static const char *classify_kind(const char *prompt) {
    if (s_has_ci(prompt, "cercle") || s_has_ci(prompt, "circle")) return "circle";
    if (s_has_ci(prompt, "graphe") || s_has_ci(prompt, "graph")) return "graph";
    if (s_has_ci(prompt, "arbre") || s_has_ci(prompt, "tree") || s_has_ci(prompt, "vfs"))
        return "tree";
    if (s_has_ci(prompt, "horloge") || s_has_ci(prompt, "clock")) return "clock";
    if (s_has_ci(prompt, "boite") || s_has_ci(prompt, "box") || s_has_ci(prompt, "dessine")
        || s_has_ci(prompt, "draw"))
        return "boxes";
    if (s_has_ci(prompt, "simule") || s_has_ci(prompt, "simulation")
        || s_has_ci(prompt, "mini-plan") || s_has_ci(prompt, "acting"))
        return "sim";
    return "plan";
}

static const char *classify_mode(const char *prompt) {
    if (s_has_ci(prompt, "simule") || s_has_ci(prompt, "simulation")
        || s_has_ci(prompt, "agir") || s_has_ci(prompt, "agis")
        || s_has_ci(prompt, "acting") || s_has_ci(prompt, "execute")
        || s_has_ci(prompt, "plan autonome") || s_has_ci(prompt, "mini-plan"))
        return "acting";
    if (s_has_ci(prompt, "dessine") || s_has_ci(prompt, "dessiner")
        || s_has_ci(prompt, "draw") || s_has_ci(prompt, "svg")
        || s_has_ci(prompt, "boite") || s_has_ci(prompt, "box")
        || s_has_ci(prompt, "presente") || s_has_ci(prompt, "scene"))
        return "presenting";
    return "reflecting";
}

static void stage_render(const char *prompt) {
    const char *mode = classify_mode(prompt);
    const char *kind;
    char cap[64];
    int i, n;
    G.scripts_stripped = 0;
    s_cpy(G.stage_mode, 16, mode);
    s_cpy(G.stage_prompt, OSUI_TEXT, prompt ? prompt : "");
    if (s_has_ci(prompt, "<script") || s_has_ci(prompt, "javascript:"))
        G.scripts_stripped = 1;
    kind = classify_kind(prompt);
    s_cpy(G.stage_kind, OSUI_KIND, kind);
    G.stage_autonomous = s_has_ci(prompt, "mini-plan") || s_has_ci(prompt, "plan autonome");
    G.stage_tick = 0;
    stage_clear();
    canvas_clear();
    n = s_len(prompt);
    if (n > 40) n = 40;
    for (i = 0; i < n; i++) {
        char c = prompt[i];
        if (c < 32) c = ' ';
        cap[i] = c;
    }
    cap[n] = 0;
    canvas_text(0, 0, "scene VGA desktop  guest_html_stage=false  llm=stub_echo");
    if (s_cmp(kind, "circle") == 0) {
        canvas_circle(10, 24, 6);
        canvas_text(18, 2, "construction=cercle (cellules VGA, pas SVG HTML)");
    } else if (s_cmp(kind, "boxes") == 0) {
        canvas_box(3, 4, 6, 12);
        canvas_text(5, 7, "A");
        canvas_box(3, 20, 6, 12);
        canvas_text(5, 23, "B");
        canvas_box(3, 36, 6, 12);
        canvas_text(5, 39, "C");
        canvas_text(11, 4, "trois boites structurees");
    } else if (s_cmp(kind, "graph") == 0) {
        canvas_graph();
    } else if (s_cmp(kind, "tree") == 0) {
        canvas_tree();
    } else if (s_cmp(kind, "clock") == 0) {
        canvas_clock();
    } else if (s_cmp(kind, "sim") == 0) {
        canvas_sim(0);
    } else {
        canvas_box(3, 2, 5, 22);
        canvas_text(4, 4, "lire prompt");
        canvas_box(3, 26, 5, 22);
        canvas_text(4, 28, "contrat Ring 3");
        canvas_box(3, 50, 5, 22);
        canvas_text(4, 52, "plan stub");
    }
    canvas_text(20, 0, cap);
    if (s_cmp(mode, "presenting") == 0) {
        stage_put(0, 0, "mode=presenting llm=stub_echo");
        stage_put(1, 0, "[A] [B] [C]  scene VGA structuree");
        stage_put(2, 0, cap);
        stage_put(3, 0, "sanitizer=allowlist guest_html_stage=false");
        stage_put(4, 0, "kind=");
        stage_put(4, 5, kind);
        stage_put(4, 5 + s_len(kind), " canvas=vga_desktop");
    } else if (s_cmp(mode, "acting") == 0) {
        stage_put(0, 0, "mode=acting llm=stub_echo");
        stage_put(1, 0, "acte1 -> acte2 -> resultat");
        stage_put(2, 0, cap);
        stage_put(3, 0, "simulation stub, pas Chromium");
        stage_put(4, 0, "kind=");
        stage_put(4, 5, kind);
    } else {
        stage_put(0, 0, "mode=reflecting llm=stub_echo");
        stage_put(1, 0, "lire prompt | contrat Ring 3 | plan");
        stage_put(2, 0, cap);
        stage_put(3, 0, "scene VGA, pas #ai-stage HTML");
        stage_put(4, 0, "kind=");
        stage_put(4, 5, kind);
    }
}

static void emit_stage(char *out, int max, int *pos) {
    int r;
    out_add(out, max, pos, "osui stage mode=");
    out_add(out, max, pos, G.stage_mode);
    out_add(out, max, pos, " llm=stub_echo sanitizer=allowlist scripts_stripped=");
    out_u(out, max, pos, (unsigned)G.scripts_stripped);
    out_add(out, max, pos, " guest_html_stage=false kind=");
    out_add(out, max, pos, G.stage_kind[0] ? G.stage_kind : "plan");
    out_add(out, max, pos, " canvas=vga_desktop\n");
    for (r = 0; r < 4; r++) {
        out_add(out, max, pos, "| ");
        out_add(out, max, pos, G.stage_rows[r]);
        out_add(out, max, pos, "\n");
    }
}

static int looks_explain(const char *t) {
    return s_has_ci(t, "explique") || s_has_ci(t, "comment")
        || s_has_ci(t, "pourquoi") || s_has_ci(t, "what")
        || s_has_ci(t, "how") || s_has_ci(t, "aide");
}

static const char *norm_selector(const char *raw, char *tmp, int max) {
    if (!raw || !raw[0]) return "";
    if (raw[0] == '#') return raw;
    tmp[0] = '#';
    s_cpy(tmp + 1, max - 1, raw);
    return tmp;
}

static int origin_ok(const char *origin) {
    if (!origin || !origin[0] || s_cmp(origin, "self") == 0) return 1;
    if (s_cmp(origin, "http://127.0.0.1") == 0) return 1;
    return 0;
}

static int split_line(const char *line, char cmd[64], char args[OSUI_MAX_ARGS][96], int *narg) {
    int i = 0;
    int a = 0;
    int p = 0;
    cmd[0] = 0;
    *narg = 0;
    if (!line) return 0;
    while (line[i] == ' ' || line[i] == '\t') i++;
    if (line[i] == '/') i++;
    while (line[i] && line[i] != ' ' && line[i] != '\t' && p < 63) {
        cmd[p++] = s_low(line[i++]);
    }
    cmd[p] = 0;
    while (line[i] == ' ' || line[i] == '\t') i++;
    while (line[i] && a < OSUI_MAX_ARGS) {
        p = 0;
        while (line[i] && line[i] != ' ' && line[i] != '\t' && p < 95) {
            args[a][p++] = line[i++];
        }
        args[a][p] = 0;
        if (p) {
            a++;
            (*narg)++;
        }
        while (line[i] == ' ' || line[i] == '\t') i++;
    }
    return cmd[0] != 0;
}

static void rest_from(char args[OSUI_MAX_ARGS][96], int narg, int start, char *dst, int max) {
    int i, pos = 0;
    dst[0] = 0;
    for (i = start; i < narg; i++) {
        if (pos && pos < max - 1) dst[pos++] = ' ';
        s_cpy(dst + pos, max - pos, args[i]);
        pos = s_len(dst);
    }
}

static int cmd_os_help(char *out, int max) {
    int p = 0;
    out_add(out, max, &p,
        "osui help llm=stub_echo live_guest=true python_facade=false\n"
        "prompt=MOHHDY>  Pas un bash Linux. Scene guest = etat C, bureau VBE QEMU.\n"
        "slash: /help /browser /shell /admin /support /status /fs /center /close /plan /draw\n"
        "gui (aliases graphics, desktop) : entre le bureau graphique QEMU. console : retour texte.\n"
        "session-new [site]  session-use <id>  session-list  session-status\n"
        "chat <texte>  prompt <texte>  grant/revoke <cap>  escalate  takeover\n"
        "origin-check <origine>  browser-click|type|pointer  browser-status\n"
        "mcp-invoice <client> <montant>  mcp-invoke <outil>\n"
        "fs-list [chemin]  fs-read <chemin>  fs-write (refuse)\n"
        "stage  stage-prompt <texte>  os-status  guest-status\n"
        "phase3_complete=false us031_complete=false\n");
    return OSUI_OK;
}

static int cmd_os_status(char *out, int max) {
    osui_session_t *s = cur();
    int p = 0;
    out_add(out, max, &p,
        "osui os-status service=mohhdy-os llm=stub_echo harness=dom_simulator\n"
        "live_guest=true chrome=qemu_fb display_surface=vbe_lfb python_facade=false guest_html_stage=false\n"
        "gui_cmd=gui aliases=graphics,desktop leave=console\n"
        "phase3_complete=false us031_complete=false billing=false kb_loaded=");
    out_add(out, max, &p, G.kb_loaded ? "true" : "false");
    out_add(out, max, &p, " chat_mode=");
    out_add(out, max, &p, G.chat_mode);
    out_add(out, max, &p, " sessions=");
    out_u(out, max, &p, (unsigned)G.n_sessions);
    if (s) {
        out_add(out, max, &p, " session_id=");
        out_add(out, max, &p, s->id);
        out_add(out, max, &p, " status=");
        out_add(out, max, &p, st_name(s->status));
    }
    out_add(out, max, &p, "\n");
    return OSUI_OK;
}

static int cmd_session_new(char args[OSUI_MAX_ARGS][96], int narg, char *out, int max) {
    osui_session_t *s = alloc_session(narg > 0 ? args[0] : "default");
    int p = 0;
    if (!s) {
        out_add(out, max, &p, "osui session-new error=too_many_sessions\n");
        return OSUI_ERR;
    }
    out_add(out, max, &p, "osui session-new ok session_id=");
    out_add(out, max, &p, s->id);
    out_add(out, max, &p, " site_id=");
    out_add(out, max, &p, s->site);
    out_add(out, max, &p, " status=open isolated=true\n");
    return OSUI_OK;
}

static int cmd_session_use(char args[OSUI_MAX_ARGS][96], int narg, char *out, int max) {
    osui_session_t *s;
    int i, p = 0;
    if (narg < 1) {
        out_add(out, max, &p, "osui session-use error=session_id manquant\n");
        return OSUI_ERR;
    }
    s = find_sid(args[0]);
    if (!s) {
        out_add(out, max, &p, "osui session-use error=session_id inconnu\n");
        return OSUI_ERR;
    }
    for (i = 0; i < OSUI_MAX_SESSIONS; i++) {
        if (&G.sessions[i] == s) G.current = i;
    }
    out_add(out, max, &p, "osui session-use ok session_id=");
    out_add(out, max, &p, s->id);
    out_add(out, max, &p, "\n");
    return OSUI_OK;
}

static void emit_caps(osui_session_t *s, char *out, int max, int *p) {
    int i;
    out_add(out, max, p, " caps=");
    if (!s || s->n_caps == 0) {
        out_add(out, max, p, "(none)");
        return;
    }
    for (i = 0; i < s->n_caps; i++) {
        if (i) out_add(out, max, p, ",");
        out_add(out, max, p, s->caps[i]);
    }
}

static int cmd_session_status(char *out, int max) {
    osui_session_t *s = cur();
    int p = 0, i;
    if (!s) {
        out_add(out, max, &p, "osui session-status error=session_id inconnu\n");
        return OSUI_ERR;
    }
    out_add(out, max, &p, "osui session-status session_id=");
    out_add(out, max, &p, s->id);
    out_add(out, max, &p, " site_id=");
    out_add(out, max, &p, s->site);
    out_add(out, max, &p, " status=");
    out_add(out, max, &p, st_name(s->status));
    out_add(out, max, &p, " handoff=");
    out_add(out, max, &p, s->handoff ? "true" : "false");
    emit_caps(s, out, max, &p);
    out_add(out, max, &p, " messages=");
    out_u(out, max, &p, (unsigned)s->n_msgs);
    out_add(out, max, &p, "\n");
    for (i = 0; i < s->n_msgs; i++) {
        out_add(out, max, &p, "- ");
        out_add(out, max, &p, s->msgs[i]);
        out_add(out, max, &p, "\n");
    }
    return OSUI_OK;
}

static int cmd_session_list(char *out, int max) {
    int i, p = 0;
    out_add(out, max, &p, "osui session-list count=");
    out_u(out, max, &p, (unsigned)G.n_sessions);
    out_add(out, max, &p, "\n");
    for (i = 0; i < OSUI_MAX_SESSIONS; i++) {
        if (!G.sessions[i].used) continue;
        out_add(out, max, &p, G.sessions[i].id);
        out_add(out, max, &p, " site=");
        out_add(out, max, &p, G.sessions[i].site);
        out_add(out, max, &p, " status=");
        out_add(out, max, &p, st_name(G.sessions[i].status));
        if (i == G.current) out_add(out, max, &p, " current");
        out_add(out, max, &p, "\n");
    }
    return OSUI_OK;
}

static int cmd_chat(char args[OSUI_MAX_ARGS][96], int narg, char *out, int max) {
    osui_session_t *s = cur();
    char text[OSUI_TEXT];
    unsigned rid;
    int p = 0;
    rest_from(args, narg, 0, text, OSUI_TEXT);
    if (!s) {
        out_add(out, max, &p, "osui chat error=session_id inconnu\n");
        return OSUI_ERR;
    }
    if (!text[0]) {
        out_add(out, max, &p, "osui chat error=texte manquant\n");
        return OSUI_ERR;
    }
    rid = new_rid();
    add_msg(s, text);
    if (s->status == ST_HUMAN || s->handoff) {
        out_add(out, max, &p, "osui chat ok auto_reply=false status=human_active llm=none session_id=");
        out_add(out, max, &p, s->id);
        emit_rid(out, max, &p, rid);
        out_add(out, max, &p, "\n");
        return OSUI_OK;
    }
    if (!has_cap(s, "chat.reply")) {
        journal(s->id, "chat.reply", "capability_denied", rid);
        add_msg(s, "capability_denied chat.reply");
        out_add(out, max, &p, "osui chat error=capability_denied capability=chat.reply session_id=");
        out_add(out, max, &p, s->id);
        emit_rid(out, max, &p, rid);
        out_add(out, max, &p, "\n");
        return OSUI_ERR;
    }
    if (G.kb_loaded && looks_explain(text) && !has_cap(s, "site.explain")) {
        add_msg(s, "stub_refusal site.explain");
        out_add(out, max, &p, "osui chat llm=stub_refusal session_id=");
        out_add(out, max, &p, s->id);
        emit_rid(out, max, &p, rid);
        out_add(out, max, &p, "\nLe droit site.explain n'est pas accorde.\n");
        return OSUI_OK;
    }
    add_msg(s, "stub_echo");
    stage_render(text);
    out_add(out, max, &p, "osui chat ok llm=stub_echo session_id=");
    out_add(out, max, &p, s->id);
    emit_rid(out, max, &p, rid);
    out_add(out, max, &p, "\n");
    out_add(out, max, &p, text);
    out_add(out, max, &p, "\n");
    emit_stage(out, max, &p);
    return OSUI_OK;
}

static int cmd_grant_revoke(int grant, char args[OSUI_MAX_ARGS][96], int narg, char *out, int max) {
    osui_session_t *s = cur();
    int p = 0;
    if (!s) {
        out_add(out, max, &p, "osui error=session_id inconnu\n");
        return OSUI_ERR;
    }
    if (narg < 1) {
        out_add(out, max, &p, "osui error=capability manquante\n");
        return OSUI_ERR;
    }
    if (!in_list(k_caps, args[0])) {
        out_add(out, max, &p, "osui error=capability inconnue\n");
        return OSUI_ERR;
    }
    if (grant) add_cap(s, args[0]);
    else drop_cap(s, args[0]);
    out_add(out, max, &p, grant ? "osui grant ok capability=" : "osui revoke ok capability=");
    out_add(out, max, &p, args[0]);
    out_add(out, max, &p, " session_id=");
    out_add(out, max, &p, s->id);
    emit_caps(s, out, max, &p);
    out_add(out, max, &p, "\n");
    return OSUI_OK;
}

static int cmd_escalate(char args[OSUI_MAX_ARGS][96], int narg, char *out, int max) {
    osui_session_t *s = cur();
    unsigned rid;
    char reason[OSUI_TEXT];
    int p = 0;
    rest_from(args, narg, 0, reason, OSUI_TEXT);
    if (!s) {
        out_add(out, max, &p, "osui escalate error=session_id inconnu\n");
        return OSUI_ERR;
    }
    if (!has_cap(s, "session.escalate")) {
        rid = new_rid();
        journal(s->id, "session.escalate", "capability_denied", rid);
        out_add(out, max, &p, "osui escalate error=capability_denied capability=session.escalate");
        emit_rid(out, max, &p, rid);
        out_add(out, max, &p, "\n");
        return OSUI_ERR;
    }
    rid = new_rid();
    s->status = ST_WAIT;
    add_msg(s, "escalate waiting_human");
    journal(s->id, "session.escalate", "ok", rid);
    out_add(out, max, &p, "osui escalate ok status=waiting_human session_id=");
    out_add(out, max, &p, s->id);
    emit_rid(out, max, &p, rid);
    if (reason[0]) {
        out_add(out, max, &p, " reason=");
        out_add(out, max, &p, reason);
    }
    out_add(out, max, &p, "\n");
    return OSUI_OK;
}

static int cmd_takeover(char *out, int max) {
    osui_session_t *s = cur();
    unsigned rid;
    int p = 0;
    if (!s) {
        out_add(out, max, &p, "osui takeover error=session_id inconnu\n");
        return OSUI_ERR;
    }
    if (!has_cap(s, "admin.takeover")) {
        rid = new_rid();
        journal(s->id, "admin.takeover", "capability_denied", rid);
        out_add(out, max, &p, "osui takeover error=capability_denied capability=admin.takeover");
        emit_rid(out, max, &p, rid);
        out_add(out, max, &p, "\n");
        return OSUI_ERR;
    }
    rid = new_rid();
    s->status = ST_HUMAN;
    s->handoff = 1;
    add_msg(s, "handoff human_active");
    journal(s->id, "admin.takeover", "ok", rid);
    out_add(out, max, &p, "osui takeover ok status=human_active handoff=true session_id=");
    out_add(out, max, &p, s->id);
    emit_rid(out, max, &p, rid);
    out_add(out, max, &p, "\n");
    return OSUI_OK;
}

static int cmd_admin_status(char *out, int max) {
    osui_session_t *s = cur();
    int p = 0, i;
    out_add(out, max, &p, "osui admin-status llm=stub_echo\n");
    if (s) {
        out_add(out, max, &p, "session_id=");
        out_add(out, max, &p, s->id);
        out_add(out, max, &p, " status=");
        out_add(out, max, &p, st_name(s->status));
        emit_caps(s, out, max, &p);
        out_add(out, max, &p, "\n");
    }
    out_add(out, max, &p, "journal\n");
    for (i = 0; i < G.n_journal; i++) {
        out_add(out, max, &p, G.journal[i].request);
        out_add(out, max, &p, " ");
        out_add(out, max, &p, G.journal[i].tool);
        out_add(out, max, &p, " ");
        out_add(out, max, &p, G.journal[i].outcome);
        out_add(out, max, &p, "\n");
    }
    return OSUI_OK;
}

static int cmd_origin_check(char args[OSUI_MAX_ARGS][96], int narg, char *out, int max) {
    unsigned rid = new_rid();
    osui_session_t *s = cur();
    const char *origin = narg > 0 ? args[0] : "";
    int p = 0;
    if (!origin_ok(origin)) {
        journal(s ? s->id : "", "origin", "origin_denied", rid);
        out_add(out, max, &p, "osui origin_denied origin=");
        out_add(out, max, &p, origin);
        emit_rid(out, max, &p, rid);
        out_add(out, max, &p, " status=403\n");
        return OSUI_ERR;
    }
    out_add(out, max, &p, "osui origin-check ok origin=");
    out_add(out, max, &p, origin[0] ? origin : "self");
    emit_rid(out, max, &p, rid);
    out_add(out, max, &p, "\n");
    return OSUI_OK;
}

static const char *parse_origin_flag(char args[OSUI_MAX_ARGS][96], int narg) {
    int i;
    for (i = 0; i < narg; i++) {
        if (s_ncmp(args[i], "origin=", 7) == 0) return args[i] + 7;
    }
    return "self";
}

static int cmd_browser_click(char args[OSUI_MAX_ARGS][96], int narg, char *out, int max) {
    osui_session_t *s = cur();
    char tmp[32];
    const char *sel;
    const char *origin;
    unsigned rid;
    int p = 0;
    if (!s) {
        out_add(out, max, &p, "osui browser-click error=session_id inconnu\n");
        return OSUI_ERR;
    }
    if (narg < 1) {
        out_add(out, max, &p, "osui browser-click error=selector manquant\n");
        return OSUI_ERR;
    }
    sel = norm_selector(args[0], tmp, 32);
    origin = parse_origin_flag(args, narg);
    rid = new_rid();
    if (!has_cap(s, "dom.click")) {
        journal(s->id, "dom.click", "capability_denied", rid);
        out_add(out, max, &p, "osui browser-click error=capability_denied capability=dom.click");
        emit_rid(out, max, &p, rid);
        out_add(out, max, &p, "\n");
        return OSUI_ERR;
    }
    if (!origin_ok(origin)) {
        journal(s->id, "dom.click", "origin_denied", rid);
        out_add(out, max, &p, "osui origin_denied origin=");
        out_add(out, max, &p, origin);
        emit_rid(out, max, &p, rid);
        out_add(out, max, &p, " status=403\n");
        return OSUI_ERR;
    }
    if (!in_list(k_selectors, sel)) {
        journal(s->id, "dom.click", "selector_denied", rid);
        out_add(out, max, &p, "osui browser-click error=selecteur non allowliste selector=");
        out_add(out, max, &p, sel);
        emit_rid(out, max, &p, rid);
        out_add(out, max, &p, "\n");
        return OSUI_ERR;
    }
    s_cpy(G.focused, 24, sel);
    if (s_cmp(sel, "#menu-toggle") == 0) G.menu_open = !G.menu_open;
    if (s_cmp(sel, "#menu-invoices") == 0) {
        G.menu_open = 1;
        s_cpy(G.focused, 24, "#invoice-customer");
    }
    if (s_cmp(sel, "#invoice-submit") == 0) G.form_submitted = 1;
    journal(s->id, "dom.click", "ok", rid);
    out_add(out, max, &p, "osui browser-click ok harness=dom_simulator selector=");
    out_add(out, max, &p, sel);
    out_add(out, max, &p, " menu_open=");
    out_add(out, max, &p, G.menu_open ? "true" : "false");
    out_add(out, max, &p, " session_id=");
    out_add(out, max, &p, s->id);
    emit_rid(out, max, &p, rid);
    out_add(out, max, &p, "\n");
    return OSUI_OK;
}

static int cmd_browser_type(char args[OSUI_MAX_ARGS][96], int narg, char *out, int max) {
    osui_session_t *s = cur();
    char tmp[32];
    char text[OSUI_TEXT];
    const char *sel;
    unsigned rid;
    int p = 0;
    if (!s) {
        out_add(out, max, &p, "osui browser-type error=session_id inconnu\n");
        return OSUI_ERR;
    }
    if (narg < 2) {
        out_add(out, max, &p, "osui browser-type error=selector+texte\n");
        return OSUI_ERR;
    }
    sel = norm_selector(args[0], tmp, 32);
    rest_from(args, narg, 1, text, OSUI_TEXT);
    rid = new_rid();
    if (!has_cap(s, "dom.type")) {
        journal(s->id, "dom.type", "capability_denied", rid);
        out_add(out, max, &p, "osui browser-type error=capability_denied capability=dom.type");
        emit_rid(out, max, &p, rid);
        out_add(out, max, &p, "\n");
        return OSUI_ERR;
    }
    if (!in_list(k_selectors, sel)) {
        out_add(out, max, &p, "osui browser-type error=selecteur non allowliste\n");
        return OSUI_ERR;
    }
    if (s_cmp(sel, "#invoice-customer") == 0) s_cpy(G.form_customer, 48, text);
    else if (s_cmp(sel, "#invoice-amount") == 0) s_cpy(G.form_amount, 24, text);
    else {
        out_add(out, max, &p, "osui browser-type error=selecteur non saisissable\n");
        return OSUI_ERR;
    }
    s_cpy(G.focused, 24, sel);
    journal(s->id, "dom.type", "ok", rid);
    out_add(out, max, &p, "osui browser-type ok harness=dom_simulator selector=");
    out_add(out, max, &p, sel);
    out_add(out, max, &p, " session_id=");
    out_add(out, max, &p, s->id);
    emit_rid(out, max, &p, rid);
    out_add(out, max, &p, "\n");
    return OSUI_OK;
}

static int cmd_browser_pointer(char args[OSUI_MAX_ARGS][96], int narg, char *out, int max) {
    osui_session_t *s = cur();
    unsigned rid;
    int p = 0;
    int x = 0, y = 0, i;
    if (!s) {
        out_add(out, max, &p, "osui browser-pointer error=session_id inconnu\n");
        return OSUI_ERR;
    }
    rid = new_rid();
    if (!has_cap(s, "pointer.move")) {
        journal(s->id, "pointer.move", "capability_denied", rid);
        out_add(out, max, &p, "osui browser-pointer error=capability_denied capability=pointer.move");
        emit_rid(out, max, &p, rid);
        out_add(out, max, &p, "\n");
        return OSUI_ERR;
    }
    if (narg >= 1) {
        for (i = 0; args[0][i] >= '0' && args[0][i] <= '9'; i++)
            x = x * 10 + (args[0][i] - '0');
    }
    if (narg >= 2) {
        for (i = 0; args[1][i] >= '0' && args[1][i] <= '9'; i++)
            y = y * 10 + (args[1][i] - '0');
    }
    if (x > 1000) x = 1000;
    if (y > 1000) y = 1000;
    G.px = x;
    G.py = y;
    journal(s->id, "pointer.move", "ok", rid);
    out_add(out, max, &p, "osui browser-pointer ok x=");
    out_u(out, max, &p, (unsigned)x);
    out_add(out, max, &p, " y=");
    out_u(out, max, &p, (unsigned)y);
    emit_rid(out, max, &p, rid);
    out_add(out, max, &p, "\n");
    return OSUI_OK;
}

static int cmd_browser_status(char *out, int max) {
    int p = 0;
    out_add(out, max, &p,
        "osui browser-status harness=dom_simulator us031_complete=false chromium=false\n"
        "menu_open=");
    out_add(out, max, &p, G.menu_open ? "true" : "false");
    out_add(out, max, &p, " focused=");
    out_add(out, max, &p, G.focused[0] ? G.focused : "(none)");
    out_add(out, max, &p, " pointer=");
    out_u(out, max, &p, (unsigned)G.px);
    out_add(out, max, &p, ",");
    out_u(out, max, &p, (unsigned)G.py);
    out_add(out, max, &p, " form.customer=");
    out_add(out, max, &p, G.form_customer);
    out_add(out, max, &p, " form.amount=");
    out_add(out, max, &p, G.form_amount);
    out_add(out, max, &p, " submitted=");
    out_add(out, max, &p, G.form_submitted ? "true" : "false");
    out_add(out, max, &p, " chat_mode=");
    out_add(out, max, &p, G.chat_mode);
    out_add(out, max, &p, " pane=");
    out_add(out, max, &p, G.pane[0] ? G.pane : "none");
    out_add(out, max, &p, "\n");
    return OSUI_OK;
}

static int cmd_mcp_invoice(char args[OSUI_MAX_ARGS][96], int narg, char *out, int max) {
    osui_session_t *s = cur();
    unsigned rid;
    osui_invoice_t *inv;
    int p = 0;
    if (!s) {
        out_add(out, max, &p, "osui mcp-invoice error=session_id inconnu\n");
        return OSUI_ERR;
    }
    rid = new_rid();
    if (!has_cap(s, "mcp.invoice.create")) {
        journal(s->id, "mcp.invoice.create", "capability_denied", rid);
        out_add(out, max, &p, "osui mcp-invoice error=capability_denied capability=mcp.invoice.create");
        emit_rid(out, max, &p, rid);
        out_add(out, max, &p, "\n");
        return OSUI_ERR;
    }
    if (!in_list(k_declared_mcp, "mcp.invoice.create")) {
        journal(s->id, "mcp.invoice.create", "undeclared", rid);
        out_add(out, max, &p, "osui mcp-invoice error=tool_undeclared");
        emit_rid(out, max, &p, rid);
        out_add(out, max, &p, "\n");
        return OSUI_ERR;
    }
    if (narg < 2) {
        out_add(out, max, &p, "osui mcp-invoice error=usage mcp-invoice <client> <montant>\n");
        return OSUI_ERR;
    }
    if (G.n_invoices >= OSUI_MAX_INVOICES) {
        out_add(out, max, &p, "osui mcp-invoice error=too_many\n");
        return OSUI_ERR;
    }
    inv = &G.invoices[G.n_invoices++];
    inv->used = 1;
    G.next_iid++;
    make_id(inv->id, 'i', G.next_iid);
    s_cpy(inv->session, OSUI_ID, s->id);
    s_cpy(inv->customer, 48, args[0]);
    s_cpy(inv->amount, 24, args[1]);
    journal(s->id, "mcp.invoice.create", "ok", rid);
    out_add(out, max, &p, "osui mcp-invoice ok invoice_id=");
    out_add(out, max, &p, inv->id);
    out_add(out, max, &p, " session_id=");
    out_add(out, max, &p, s->id);
    out_add(out, max, &p, " customer=");
    out_add(out, max, &p, inv->customer);
    out_add(out, max, &p, " amount=");
    out_add(out, max, &p, inv->amount);
    emit_rid(out, max, &p, rid);
    out_add(out, max, &p, "\n");
    return OSUI_OK;
}

static int cmd_mcp_invoke(char args[OSUI_MAX_ARGS][96], int narg, char *out, int max) {
    osui_session_t *s = cur();
    const char *tool;
    unsigned rid;
    int p = 0;
    if (!s) {
        out_add(out, max, &p, "osui mcp-invoke error=session_id inconnu\n");
        return OSUI_ERR;
    }
    if (narg < 1) {
        out_add(out, max, &p, "osui mcp-invoke error=outil manquant\n");
        return OSUI_ERR;
    }
    tool = args[0];
    rid = new_rid();
    if (s_ncmp(tool, "mcp.", 4) != 0) {
        out_add(out, max, &p, "osui mcp-invoke error=not_mcp\n");
        return OSUI_ERR;
    }
    if (!in_list(k_declared_mcp, tool)) {
        journal(s->id, tool, "undeclared", rid);
        if (s->status == ST_OPEN) s->status = ST_WAIT;
        add_msg(s, "tool_undeclared escalate");
        out_add(out, max, &p, "osui mcp-invoke error=tool_undeclared tool=");
        out_add(out, max, &p, tool);
        out_add(out, max, &p, " status=403");
        emit_rid(out, max, &p, rid);
        out_add(out, max, &p, "\n");
        return OSUI_ERR;
    }
    if (s_cmp(tool, "mcp.invoice.create") == 0) {
        char rest[OSUI_MAX_ARGS][96];
        int i, m = 0;
        for (i = 1; i < narg && m < OSUI_MAX_ARGS; i++) {
            s_cpy(rest[m], 96, args[i]);
            m++;
        }
        return cmd_mcp_invoice(rest, m, out, max);
    }
    out_add(out, max, &p, "osui mcp-invoke error=unhandled\n");
    return OSUI_ERR;
}

static int fs_traversal(const char *path) {
    int i;
    if (!path) return 1;
    if (path[0] == '/' ) return 1;
    for (i = 0; path[i]; i++) {
        if (path[i] == '.' && path[i + 1] == '.') return 1;
        if (path[i] == '\\') return 1;
    }
    return 0;
}

static osui_fs_t *fs_find(const char *path) {
    int i;
    for (i = 0; i < G.n_fs; i++) {
        if (s_cmp(G.fs[i].path, path) == 0) return &G.fs[i];
    }
    return 0;
}

static int cmd_fs_list(char args[OSUI_MAX_ARGS][96], int narg, char *out, int max) {
    const char *path = narg > 0 ? args[0] : "";
    int i, p = 0, n = 0;
    if (path[0] && fs_traversal(path)) {
        unsigned rid = new_rid();
        out_add(out, max, &p, "osui fs-list error=traversal_denied");
        emit_rid(out, max, &p, rid);
        out_add(out, max, &p, "\n");
        return OSUI_ERR;
    }
    out_add(out, max, &p, "osui fs-list ok sandbox=true write=false\n");
    for (i = 0; i < G.n_fs; i++) {
        if (!path[0] || s_ncmp(G.fs[i].path, path, s_len(path)) == 0) {
            out_add(out, max, &p, G.fs[i].path);
            out_add(out, max, &p, G.fs[i].is_dir ? " dir\n" : " file\n");
            n++;
        }
    }
    if (!n) out_add(out, max, &p, "(vide)\n");
    return OSUI_OK;
}

static int cmd_fs_read(char args[OSUI_MAX_ARGS][96], int narg, char *out, int max) {
    osui_fs_t *f;
    unsigned rid;
    int p = 0;
    if (narg < 1) {
        out_add(out, max, &p, "osui fs-read error=chemin manquant\n");
        return OSUI_ERR;
    }
    rid = new_rid();
    if (fs_traversal(args[0])) {
        out_add(out, max, &p, "osui fs-read error=traversal_denied");
        emit_rid(out, max, &p, rid);
        out_add(out, max, &p, "\n");
        return OSUI_ERR;
    }
    f = fs_find(args[0]);
    if (!f || f->is_dir) {
        out_add(out, max, &p, "osui fs-read error=not_found\n");
        return OSUI_ERR;
    }
    out_add(out, max, &p, "osui fs-read ok ");
    out_add(out, max, &p, args[0]);
    emit_rid(out, max, &p, rid);
    out_add(out, max, &p, "\n");
    out_add(out, max, &p, f->content);
    return OSUI_OK;
}

static int cmd_fs_write(char *out, int max) {
    unsigned rid = new_rid();
    int p = 0;
    out_add(out, max, &p, "osui fs-write error=write_denied policy=read_only");
    emit_rid(out, max, &p, rid);
    out_add(out, max, &p, "\n");
    return OSUI_ERR;
}

static int cmd_stage(char *out, int max) {
    int p = 0;
    emit_stage(out, max, &p);
    return OSUI_OK;
}

static int cmd_stage_prompt(char args[OSUI_MAX_ARGS][96], int narg, char *out, int max) {
    char text[OSUI_TEXT];
    int p = 0;
    rest_from(args, narg, 0, text, OSUI_TEXT);
    if (!text[0]) s_cpy(text, OSUI_TEXT, "prompt vide");
    stage_render(text);
    emit_stage(out, max, &p);
    return OSUI_OK;
}

static int cmd_guest_status(char *out, int max) {
    int p = 0;
    out_add(out, max, &p,
        "guest-status attachment=live_guest live_guest=true qemu_serial=true\n"
        "source=userspace/shell.c prompt=MOHHDY>\n"
        "scene=vga_text guest_html_stage=false python_facade=false\n");
    return OSUI_OK;
}

static int cmd_attach(char *out, int max) {
    int p = 0;
    out_add(out, max, &p,
        "attach ok live_guest=true transport=in_guest prompt=MOHHDY>\n"
        "Cette instance EST le guest Multiboot. Pas d'attache hote Python.\n");
    return OSUI_OK;
}

static int cmd_detach(char *out, int max) {
    int p = 0;
    out_add(out, max, &p,
        "detach refuse: le shell tourne dans le guest. live_guest=true\n");
    return OSUI_ERR;
}

static int cmd_open(char args[OSUI_MAX_ARGS][96], int narg, char *out, int max) {
    const char *pane = narg > 0 ? args[0] : "";
    int p = 0;
    static const char *panes[] = {
        "browser", "shell", "admin", "support", "status", "fs", 0
    };
    if (!in_list(panes, pane)) {
        out_add(out, max, &p, "osui open error=usage open browser|shell|admin|support|status|fs\n");
        return OSUI_ERR;
    }
    s_cpy(G.chat_mode, 12, "float");
    s_cpy(G.pane, OSUI_PANE, pane);
    /* Park the floating chat on the right so it does not cover the program pane. */
    G.chat_x = 51;
    G.chat_y = 14;
    out_add(out, max, &p, "osui open ok pane=");
    out_add(out, max, &p, pane);
    out_add(out, max, &p, " chat_mode=float live_guest=true\n");
    return OSUI_OK;
}

static int cmd_center(char *out, int max) {
    int p = 0;
    s_cpy(G.chat_mode, 12, "center");
    G.pane[0] = 0;
    G.chat_x = 16;
    G.chat_y = 5;
    out_add(out, max, &p, "osui center ok chat_mode=center\n");
    return OSUI_OK;
}

static int cmd_gui(char *out, int max) {
    int p = 0;
    G.gui_enter = 1;
    G.gui_leave = 0;
    out_add(out, max, &p,
        "osui gui ok chrome=qemu_fb display_surface=vbe_lfb canonical=gui aliases=graphics,desktop\n"
        "leave=console chat_mode=");
    out_add(out, max, &p, G.chat_mode);
    out_add(out, max, &p, " llm=stub_echo us031_complete=false guest_html_stage=false python_facade=false\n");
    return OSUI_OK;
}

static int cmd_gui_exit(char *out, int max) {
    int p = 0;
    G.gui_leave = 1;
    G.gui_enter = 0;
    s_cpy(G.chat_mode, 12, "center");
    G.pane[0] = 0;
    out_add(out, max, &p, "osui gui exit chat_mode=center chrome=text prompt=MOHHDY>\n");
    return OSUI_OK;
}

static int cmd_gui_status(char *out, int max) {
    int p = 0, r;
    out_add(out, max, &p, "osui gui-status chrome=qemu_fb display_surface=vbe_lfb canonical=gui chat_mode=");
    out_add(out, max, &p, G.chat_mode);
    out_add(out, max, &p, " pane=");
    out_add(out, max, &p, G.pane[0] ? G.pane : "none");
    out_add(out, max, &p, " kind=");
    out_add(out, max, &p, G.stage_kind);
    out_add(out, max, &p, " mode=");
    out_add(out, max, &p, G.stage_mode);
    out_add(out, max, &p, " us031_complete=false guest_html_stage=false python_facade=false\n");
    for (r = 0; r < 8 && r < OSUI_CANVAS_ROWS; r++) {
        out_add(out, max, &p, "|");
        out_add(out, max, &p, G.canvas[r]);
        out_add(out, max, &p, "\n");
    }
    return OSUI_OK;
}

static int cmd_gui_move(char args[OSUI_MAX_ARGS][96], int narg, char *out, int max) {
    const char *dir = narg > 0 ? args[0] : "";
    int p = 0;
    if (s_cmp(G.chat_mode, "float") != 0) {
        out_add(out, max, &p, "osui gui-move error=chat_mode=center (ouvre un programme d'abord)\n");
        return OSUI_ERR;
    }
    if (s_cmp(dir, "left") == 0) G.chat_x -= 2;
    else if (s_cmp(dir, "right") == 0) G.chat_x += 2;
    else if (s_cmp(dir, "up") == 0) G.chat_y -= 1;
    else if (s_cmp(dir, "down") == 0) G.chat_y += 1;
    else {
        out_add(out, max, &p, "osui gui-move error=usage gui-move left|right|up|down\n");
        return OSUI_ERR;
    }
    if (G.chat_x < 0) G.chat_x = 0;
    if (G.chat_y < 1) G.chat_y = 1;
    if (G.chat_x > 52) G.chat_x = 52;
    if (G.chat_y > 14) G.chat_y = 14;
    out_add(out, max, &p, "osui gui-move ok chat_mode=float x=");
    out_u(out, max, &p, (unsigned)G.chat_x);
    out_add(out, max, &p, " y=");
    out_u(out, max, &p, (unsigned)G.chat_y);
    out_add(out, max, &p, "\n");
    return OSUI_OK;
}

static int map_slash(const char *cmd, char *mapped, int max) {
    if (s_cmp(cmd, "help") == 0) { s_cpy(mapped, max, "os-help"); return 1; }
    if (s_cmp(cmd, "browser") == 0) { s_cpy(mapped, max, "os-browser"); return 1; }
    if (s_cmp(cmd, "shell") == 0) { s_cpy(mapped, max, "os-shell"); return 1; }
    if (s_cmp(cmd, "admin") == 0) { s_cpy(mapped, max, "os-admin"); return 1; }
    if (s_cmp(cmd, "support") == 0) { s_cpy(mapped, max, "os-support"); return 1; }
    if (s_cmp(cmd, "status") == 0) { s_cpy(mapped, max, "os-pane-status"); return 1; }
    if (s_cmp(cmd, "fs") == 0) { s_cpy(mapped, max, "os-fs"); return 1; }
    if (s_cmp(cmd, "center") == 0) { s_cpy(mapped, max, "os-center"); return 1; }
    if (s_cmp(cmd, "close") == 0) { s_cpy(mapped, max, "os-center"); return 1; }
    if (s_cmp(cmd, "plan") == 0) { s_cpy(mapped, max, "stage-prompt"); return 1; }
    if (s_cmp(cmd, "draw") == 0) { s_cpy(mapped, max, "stage-prompt"); return 1; }
    if (s_cmp(cmd, "stage") == 0) { s_cpy(mapped, max, "stage-prompt"); return 1; }
    if (s_cmp(cmd, "guest") == 0) { s_cpy(mapped, max, "guest-status"); return 1; }
    if (s_cmp(cmd, "gui") == 0 || s_cmp(cmd, "graphics") == 0 || s_cmp(cmd, "desktop") == 0)
        { s_cpy(mapped, max, "gui"); return 1; }
    if (s_cmp(cmd, "console") == 0) { s_cpy(mapped, max, "gui-exit"); return 1; }
    return 0;
}

static int open_then(const char *pane, int (*fn)(char *, int), char *out, int max) {
    char args[OSUI_MAX_ARGS][96];
    char unused[64];
    s_cpy(args[0], 96, pane);
    cmd_open(args, 1, unused, 64);
    return fn(out, max);
}

static int dispatch_cmd(const char *cmd, char args[OSUI_MAX_ARGS][96], int narg, char *out, int max) {
    if (s_cmp(cmd, "os-help") == 0 || s_cmp(cmd, "help") == 0) return cmd_os_help(out, max);
    if (s_cmp(cmd, "os-status") == 0) return cmd_os_status(out, max);
    if (s_cmp(cmd, "os-browser") == 0) return open_then("browser", cmd_browser_status, out, max);
    if (s_cmp(cmd, "os-shell") == 0) {
        char dummy[64];
        char a[OSUI_MAX_ARGS][96];
        int p = 0;
        s_cpy(a[0], 96, "shell");
        cmd_open(a, 1, dummy, 64);
        out_add(out, max, &p, "osui os-shell ok this_is_multiboot_ring3 prompt=MOHHDY> chat_mode=float\n");
        return OSUI_OK;
    }
    if (s_cmp(cmd, "os-admin") == 0) return open_then("admin", cmd_admin_status, out, max);
    if (s_cmp(cmd, "os-support") == 0) return open_then("support", cmd_session_list, out, max);
    if (s_cmp(cmd, "os-fs") == 0) {
        char dummy[64];
        char a[OSUI_MAX_ARGS][96];
        s_cpy(a[0], 96, "fs");
        cmd_open(a, 1, dummy, 64);
        return cmd_fs_list(args, narg, out, max);
    }
    if (s_cmp(cmd, "os-center") == 0 || s_cmp(cmd, "os-close") == 0)
        return cmd_center(out, max);
    if (s_cmp(cmd, "os-pane-status") == 0)
        return open_then("status", cmd_os_status, out, max);
    if (s_cmp(cmd, "session-new") == 0) return cmd_session_new(args, narg, out, max);
    if (s_cmp(cmd, "session-use") == 0) return cmd_session_use(args, narg, out, max);
    if (s_cmp(cmd, "session-status") == 0) return cmd_session_status(out, max);
    if (s_cmp(cmd, "session-list") == 0) return cmd_session_list(out, max);
    if (s_cmp(cmd, "chat") == 0) return cmd_chat(args, narg, out, max);
    if (s_cmp(cmd, "grant") == 0) return cmd_grant_revoke(1, args, narg, out, max);
    if (s_cmp(cmd, "revoke") == 0) return cmd_grant_revoke(0, args, narg, out, max);
    if (s_cmp(cmd, "escalate") == 0) return cmd_escalate(args, narg, out, max);
    if (s_cmp(cmd, "takeover") == 0) return cmd_takeover(out, max);
    if (s_cmp(cmd, "admin-status") == 0) return cmd_admin_status(out, max);
    if (s_cmp(cmd, "origin-check") == 0) return cmd_origin_check(args, narg, out, max);
    if (s_cmp(cmd, "browser-click") == 0) return cmd_browser_click(args, narg, out, max);
    if (s_cmp(cmd, "browser-type") == 0) return cmd_browser_type(args, narg, out, max);
    if (s_cmp(cmd, "browser-pointer") == 0) return cmd_browser_pointer(args, narg, out, max);
    if (s_cmp(cmd, "browser-status") == 0) return cmd_browser_status(out, max);
    if (s_cmp(cmd, "mcp-invoice") == 0) return cmd_mcp_invoice(args, narg, out, max);
    if (s_cmp(cmd, "mcp-invoke") == 0) return cmd_mcp_invoke(args, narg, out, max);
    if (s_cmp(cmd, "fs-list") == 0) return cmd_fs_list(args, narg, out, max);
    if (s_cmp(cmd, "fs-read") == 0) return cmd_fs_read(args, narg, out, max);
    if (s_cmp(cmd, "fs-write") == 0) return cmd_fs_write(out, max);
    if (s_cmp(cmd, "stage") == 0) return cmd_stage(out, max);
    if (s_cmp(cmd, "stage-prompt") == 0) return cmd_stage_prompt(args, narg, out, max);
    if (s_cmp(cmd, "guest-status") == 0) return cmd_guest_status(out, max);
    if (s_cmp(cmd, "attach") == 0) return cmd_attach(out, max);
    if (s_cmp(cmd, "detach") == 0) return cmd_detach(out, max);
    if (s_cmp(cmd, "open") == 0) return cmd_open(args, narg, out, max);
    if (s_cmp(cmd, "gui") == 0 || s_cmp(cmd, "graphics") == 0 || s_cmp(cmd, "desktop") == 0)
        return cmd_gui(out, max);
    if (s_cmp(cmd, "console") == 0 || s_cmp(cmd, "gui-exit") == 0)
        return cmd_gui_exit(out, max);
    if (s_cmp(cmd, "gui-status") == 0) return cmd_gui_status(out, max);
    if (s_cmp(cmd, "gui-move") == 0) return cmd_gui_move(args, narg, out, max);
    return -1;
}

static int cmd_prompt(char args[OSUI_MAX_ARGS][96], int narg, const char *raw, char *out, int max) {
    char text[OSUI_TEXT];
    char first[64];
    int p = 0;
    rest_from(args, narg, 0, text, OSUI_TEXT);
    if (!text[0] && raw) {
        const char *r = raw;
        while (*r == ' ') r++;
        if (s_ncmp(r, "prompt", 6) == 0) r += 6;
        while (*r == ' ') r++;
        s_cpy(text, OSUI_TEXT, r);
    }
    if (!text[0]) {
        out_add(out, max, &p, "osui prompt error=texte manquant\n");
        return OSUI_ERR;
    }
    first[0] = 0;
    {
        int i = 0;
        while (text[i] && text[i] != ' ' && i < 63) {
            first[i] = s_low(text[i]);
            i++;
        }
        first[i] = 0;
    }
    if (osui_is_linux_trap(first)) {
        out_add(out, max, &p,
            "Pas un bash Linux. Shell Multiboot (userspace/shell.c). Tapez help.\n");
        return OSUI_ERR;
    }
    if (s_has_ci(text, "ouvre") || s_has_ci(text, "open ")) {
        char pane[OSUI_MAX_ARGS][96];
        pane[0][0] = 0;
        if (s_has_ci(text, "browser") || s_has_ci(text, "navigateur"))
            s_cpy(pane[0], 96, "browser");
        else if (s_has_ci(text, "shell") || s_has_ci(text, "terminal"))
            s_cpy(pane[0], 96, "shell");
        else if (s_has_ci(text, "admin"))
            s_cpy(pane[0], 96, "admin");
        else if (s_has_ci(text, "support"))
            s_cpy(pane[0], 96, "support");
        else if (s_has_ci(text, "status") || s_has_ci(text, "statut"))
            s_cpy(pane[0], 96, "status");
        else if (s_has_ci(text, "fs") || s_has_ci(text, "fichier"))
            s_cpy(pane[0], 96, "fs");
        if (pane[0][0]) return cmd_open(pane, 1, out, max);
    }
    if (s_has_ci(text, "dessine") || s_has_ci(text, "draw")
        || s_has_ci(text, "mini-plan") || s_has_ci(text, "simule")) {
        stage_render(text);
        emit_stage(out, max, &p);
        return OSUI_OK;
    }
    return cmd_chat(args, narg, out, max);
}

void osui_runtime_init(void) {
    int i;
    for (i = 0; i < (int)sizeof(G); i++) ((char *)&G)[i] = 0;
    G.next_sid = 0;
    G.next_rid = 0;
    G.next_iid = 0;
    G.current = -1;
    s_cpy(G.chat_mode, 12, "center");
    s_cpy(G.stage_mode, 16, "reflecting");
    s_cpy(G.stage_kind, OSUI_KIND, "plan");
    G.kb_loaded = 1;
    G.chat_x = 16;
    G.chat_y = 5;
    G.pane[0] = 0;
    G.gui_enter = 0;
    G.gui_leave = 0;
    fs_seed();
    alloc_session("default");
    stage_render("");
}

int osui_is_command(const char *cmd) {
    if (!cmd || !cmd[0]) return 0;
    return in_list(k_cmds, cmd);
}

int osui_is_linux_trap(const char *cmd) {
    return in_list(k_linux_traps, cmd);
}

int osui_dispatch_line(const char *line, char *out, int out_max) {
    char cmd[64];
    char mapped[32];
    char args[OSUI_MAX_ARGS][96];
    int narg = 0;
    int rc;
    int p = 0;
    if (out && out_max > 0) out[0] = 0;
    if (!split_line(line, cmd, args, &narg)) {
        out_add(out, out_max, &p, "osui error=empty\n");
        return OSUI_ERR;
    }
    if (osui_is_linux_trap(cmd)) {
        out_add(out, out_max, &p,
            "Pas un bash Linux. Shell Multiboot (userspace/shell.c). Tapez help.\n");
        return OSUI_ERR;
    }
    if (map_slash(cmd, mapped, 32)) {
        if (s_cmp(mapped, "stage-prompt") == 0 && narg == 0) {
            if (s_cmp(cmd, "plan") == 0)
                s_cpy(args[0], 96, "mini-plan autonome stub");
            else if (s_cmp(cmd, "draw") == 0)
                s_cpy(args[0], 96, "dessine trois boites");
            if (args[0][0]) narg = 1;
        }
        s_cpy(cmd, 64, mapped);
    }
    if (s_cmp(cmd, "prompt") == 0)
        return cmd_prompt(args, narg, line, out, out_max);
    rc = dispatch_cmd(cmd, args, narg, out, out_max);
    if (rc < 0) {
        p = 0;
        out_add(out, out_max, &p, "osui error=commande inconnue. os-help pour la liste.\n");
        return OSUI_ERR;
    }
    return rc;
}

int osui_guest_command_count(void) {
    (void)mohhdy_shell_commands;
    (void)MOHHDY_OSUI_GUEST_HTML_STAGE;
    return MOHHDY_SHELL_COMMAND_COUNT;
}

const char *osui_get_chat_mode(void) { return G.chat_mode; }
const char *osui_get_pane(void) { return G.pane[0] ? G.pane : "none"; }
const char *osui_get_stage_mode(void) { return G.stage_mode; }
const char *osui_get_stage_kind(void) { return G.stage_kind[0] ? G.stage_kind : "plan"; }
const char *osui_get_session_id(void) {
    osui_session_t *s = cur();
    return s ? s->id : "";
}
int osui_get_chat_x(void) { return G.chat_x; }
int osui_get_chat_y(void) { return G.chat_y; }

int osui_move_chat(int dx, int dy) {
    if (s_cmp(G.chat_mode, "float") != 0) return 0;
    G.chat_x += dx;
    G.chat_y += dy;
    if (G.chat_x < 0) G.chat_x = 0;
    if (G.chat_y < 1) G.chat_y = 1;
    if (G.chat_x > 52) G.chat_x = 52;
    if (G.chat_y > 14) G.chat_y = 14;
    return 1;
}

int osui_msg_count(void) {
    osui_session_t *s = cur();
    return s ? s->n_msgs : 0;
}

void osui_msg_at(int i, char *dst, int max) {
    osui_session_t *s = cur();
    dst[0] = 0;
    if (!s || i < 0 || i >= s->n_msgs) return;
    s_cpy(dst, max, s->msgs[i]);
}

void osui_canvas_row(int r, char *dst, int max) {
    dst[0] = 0;
    if (r < 0 || r >= OSUI_CANVAS_ROWS) return;
    s_cpy(dst, max, G.canvas[r]);
}

int osui_gui_should_enter(void) { return G.gui_enter; }
void osui_gui_ack_enter(void) { G.gui_enter = 0; }
int osui_gui_should_leave(void) { return G.gui_leave; }
void osui_gui_ack_leave(void) { G.gui_leave = 0; }

int osui_stage_tick(char *out, int out_max) {
    if (out && out_max > 0) out[0] = 0;
    if (!G.stage_autonomous) return 0;
    G.stage_tick++;
    canvas_sim(G.stage_tick);
    s_cpy(G.stage_mode, 16, G.stage_tick > 4 ? "presenting" : "acting");
    return 0;
}
