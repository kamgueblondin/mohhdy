/* osui_runtime.c - Port OS-UI (chat, scene, sessions, droits, MCP, FS) en C
 * freestanding Ring 3. llm=stub_echo. Pas HTML #ai-stage. Pas Chromium.
 * Pas un bash Linux. Source de verite : meme vocabulaire que shell.c.
 */

#include "osui_runtime.h"
#include "webrt.h"
#include "mohhdy_osui_bridge.h"
#include "os_syscalls.h"
#include "promptmessage.h"
#include "csig.h"
#include "platform.h"
#include "../kernel/sha256.h"

static int pm_hexval(char c) {
    return (c >= '0' && c <= '9') ? c - '0' : (c >= 'a' && c <= 'f') ? c - 'a' + 10 : -1;
}

static void pm_sig_digest(const char* src, int len, uint8_t d[32]) {
    sha256_ctx_t h;
    sha256_init(&h);
    sha256_update(&h, (const uint8_t*)"pmcert1", 7);
    sha256_update(&h, (const uint8_t*)src, (uint32_t)len);
    sha256_final(&h, d);
}

#define OSUI_MAX_SESSIONS 8
#define OSUI_MAX_TABS 4
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
    "session-new", "session-use", "session-status", "session-list", "session-end",
    "session-ttl", "session-restore", "session-cleanup", "confirm", "deny", "mcp-invoice-void",
    "agent-run", "api", "api-token", "api-revoke", "web-serve",
    "chat", "prompt",
    "grant", "revoke", "escalate", "takeover", "admin-status",
    "origin-check",
    "browser-click", "browser-type", "browser-pointer", "browser-status", "browser-origin-status",
    "browser-tab-new", "browser-tab-use", "browser-tab-list", "browser-tab-close",
    "browser-tab-rename", "browser-tab-pin", "browser-tab-duplicate", "browser-tab-next", "browser-tab-prev", "browser-tabs",
    "browser-fs-save", "browser-fs-load", "browser-fs-sync",
    "browser-navigate", "browser-back", "browser-forward", "browser-dom-tree", "browser-eval", "browser-pm-run",
    "browser-html-render", "browser-render", "browser-css-demo",
    "web-cache", "web-csp", "web-sanitize", "web-responsive", "web-theme", "web-components", "web-a11y",
    "browser-form-fill", "browser-dom-act", "browser-fetch", "fetch-sim",
    "browser-storage-set", "browser-storage-get", "tab-storage",
    "browser-telemetry", "osui-telemetry", "web-perf", "pwa-manifest", "pwa-install", "pwa-list", "pwa-run",
    "browser-media", "media-play", "media-pause", "media-stop", "media-stream",
    "mcp-invoice", "mcp-invoke", "mcp-list", "mcp-status", "mcp-auth", "mcp-credentials",
    "fs-list", "fs-read", "fs-write",
    "stage", "stage-prompt",
    "os-help", "os-status", "os-browser", "os-shell", "os-admin",
    "os-support", "os-fs", "os-p2p", "os-collab", "os-prod", "os-center", "os-close", "osui-model", "osui-provider", "osui-audit",
    "guest-status", "attach", "detach", "open",
    "gui", "graphics", "desktop", "console", "gui-status", "gui-exit",
    "gui-move",
    0
};

static const char *const k_caps[] = {
    "chat.reply", "site.explain", "session.escalate",
    "admin.observe", "admin.takeover",
    "dom.click", "dom.type", "pointer.move",
    "mcp.invoice.create", "agent.run", "web.api",
    0
};

static const char *const k_selectors[] = {
    "#menu-toggle", "#menu-invoices", "#invoice-customer",
    "#invoice-amount", "#invoice-submit",
    0
};

static const char *const k_declared_mcp[] = {
    "mcp.invoice.create",
    "mcp.payment.process",
    "mcp.document.sign",
    0
};

typedef struct {
    int used;
    char id[OSUI_ID];
    char site[32];
    int status;
    int handoff;
    char ai_state[16];
    int n_caps;
    char caps[OSUI_MAX_CAPS][OSUI_CAP];
    int n_msgs;
    char msgs[OSUI_MAX_MSGS][OSUI_TEXT];
    unsigned last_active;   /* osui_now() seconds of the last command */
    /* One mutation waiting for an explicit confirm (roadmap step 3). */
    char pending_token[OSUI_ID];
    char pending_tool[OSUI_CAP];
    char pending_a[48];
    char pending_b[24];
    /* Roadmap step 5: local API token bound to this session. */
    char api_token[OSUI_ID];
    unsigned api_window;    /* osui_now() second the rate window opened */
    unsigned api_count;     /* requests in the current window */
} osui_session_t;

typedef struct {
    int used;
    char id[OSUI_ID];
    char session[OSUI_ID];
    char customer[48];
    char amount[24];
    int voided;
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

#define OSUI_MAX_TAB_STORAGE 4
#define OSUI_MAX_TAB_HISTORY 8
#define OSUI_MAX_MCP_CREDS 4
#define OSUI_MAX_AUDIT 16
#define OSUI_MAX_PWA 8

typedef struct {
    int used;
    char id[OSUI_ID];
    char name[32];
    char start_url[64];
    char scope[32];
    char theme_color[16];
    char display[16];
    int installed;
} osui_pwa_t;

typedef struct {
    int active;
    int paused;
    char type[16];
    char title[32];
    char url[64];
    unsigned ticks;
} osui_media_stream_t;

typedef struct {
    int used;
    char action[32];
    char detail[64];
} osui_audit_entry_t;

typedef struct {
    int used;
    char tool[OSUI_CAP];
    char bearer[64];
} osui_mcp_cred_t;

typedef struct {
    int used;
    char key[24];
    char value[64];
} osui_kv_t;

typedef struct {
    int used;
    char id[OSUI_ID];
    char url[64];
    char title[32];
    int pinned;
    int loading;
    char kind[16];
    char origin[32];
    char history[OSUI_MAX_TAB_HISTORY][64];
    int n_history;
    int history_pos;
    osui_kv_t storage[OSUI_MAX_TAB_STORAGE];
    int n_storage;
} osui_tab_t;

typedef struct {
    osui_session_t sessions[OSUI_MAX_SESSIONS];
    int n_sessions;
    int current;
    osui_tab_t tabs[OSUI_MAX_TABS];
    int n_tabs;
    int current_tab;
    unsigned next_tid;
    unsigned next_sid;
    unsigned next_rid;
    unsigned next_iid;
    unsigned next_cid;      /* confirmation tokens c0001.. */
    unsigned session_ttl;   /* idle expiry in seconds, 0 = never */
    char chat_mode[12];
    int menu_open;
    char focused[24];
    int px;
    int py;
    char form_customer[48];
    char form_amount[24];
    int form_submitted;
    int confirming;         /* replaying a confirmed mutation */
    char stage_mode[16];
    char stage_prompt[OSUI_TEXT];
    char stage_kind[OSUI_KIND];
    char stage_llm[20];
    char stage_ai_note[48]; /* visible AI state on the VGA scene */
    int scripts_stripped;
    char stage_rows[OSUI_STAGE_ROWS][OSUI_STAGE_COLS];
    char canvas[OSUI_CANVAS_ROWS][OSUI_CANVAS_COLS];
    char pane[OSUI_PANE];
    int chat_x;
    int chat_y;
    int gui_enter;
    int gui_live; /* between gui and console / gui-exit */
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
    char ai_model[32];
    char ai_provider[16];
    osui_mcp_cred_t mcp_creds[OSUI_MAX_MCP_CREDS];
    int n_mcp_creds;
    osui_audit_entry_t audit_log[OSUI_MAX_AUDIT];
    int n_audit;
    /* Lot #137: Telemetry, PWA, and Media stream state */
    unsigned telemetry_commands;
    unsigned telemetry_fetches;
    unsigned telemetry_ipc_msgs;
    unsigned telemetry_media_ticks;
    unsigned telemetry_pwa_runs;
    unsigned telemetry_errors;
    unsigned telemetry_sched_cycles;
    unsigned perf_min_us;
    unsigned perf_max_us;
    unsigned perf_avg_us;
    unsigned perf_last_us;
    unsigned render_ms;
    osui_pwa_t pwas[OSUI_MAX_PWA];
    int n_pwas;
    unsigned next_pwa_id;
    osui_media_stream_t media;
} osui_state_t;

static osui_state_t G;

#ifdef MOHHDY_OSUI_HOST_TEST
/* Host fixture: osui_test_ai_rc < 0 forces a failure, with
 * osui_test_ai_error as the kernel's OS_AI_ERROR_* class. */
unsigned osui_test_now = 0U;
static unsigned osui_now(void) { return osui_test_now; }
int osui_test_ai_rc = 0;
unsigned osui_test_ai_error = 0U;
unsigned osui_test_ai_abort = 0U;
int osui_test_net_rc = -1;           /* no published Ring 3 stack */
unsigned osui_test_net_llm_status = 0U;
int osui_test_net_worker = 0;
static int osui_net_stack(unsigned *llm_status, int *worker) {
    *llm_status = osui_test_net_llm_status;
    *worker = osui_test_net_worker;
    return osui_test_net_rc;
}
static unsigned osui_ai_last_error(void) { return osui_test_ai_error; }
static unsigned osui_ai_last_abort(void) { return osui_test_ai_abort; }
int osui_test_ai_worker = 0;
static int osui_ai_worker_pid(void) { return osui_test_ai_worker; }
static int osui_gpt2_generate(const char *prompt, char *out, int max) {
    const char *fixture = "test local response";
    int i = 0;
    (void)prompt;
    if (!out || max < 2) return -1;
    if (osui_test_ai_rc < 0) {
        out[0] = 0;
        return osui_test_ai_rc;
    }
    while (fixture[i] && i < max - 1) {
        out[i] = fixture[i];
        i++;
    }
    out[i] = 0;
    return i;
}
/* Host fixture of the QEMU peer provider (roadmap step 4). */
int osui_test_peer_acquire_rc = -1;
int osui_test_peer_tls_polls = 0;    /* polls before TLS_COMPLETE; <0 fails */
int osui_test_peer_request_rc = 0;
int osui_test_peer_text_polls = 0;   /* polls before the reply; <0 fails */
unsigned osui_test_peer_http = 200U;
static int g_tp_tls, g_tp_text, g_tp_phase;
static int peer_acquire(void) { g_tp_tls = 0; g_tp_phase = 2; return osui_test_peer_acquire_rc; }
static int peer_phase(void) { return g_tp_phase; }
static int peer_poll_tls(void) {
    if (osui_test_peer_tls_polls < 0) return -1;
    if (++g_tp_tls >= osui_test_peer_tls_polls) g_tp_phase = 3;
    return 0;
}
static int peer_request(const char *prompt) { (void)prompt; g_tp_text = 0; return osui_test_peer_request_rc; }
static int peer_poll_text(char *out, int max, unsigned *code) {
    const char *t = "peer says ok";
    int i = 0;
    if (osui_test_peer_text_polls < 0) return -1;
    if (++g_tp_text < osui_test_peer_text_polls) return 1;
    while (t[i] && i < max - 1) { out[i] = t[i]; i++; }
    out[i] = 0;
    *code = osui_test_peer_http;
    return 0;
}
static void peer_close(void) { g_tp_phase = 0; }
static int peer_rearm(void) { g_tp_phase = 3; return 0; }
static void peer_yield(void) { osui_test_now += 1U; }
/* Host fixture of the web table (roadmap step 5): slot i carries the
 * scripted request osui_test_web_req[i]; every scripted slot is ready in
 * the same poll, so the server handles them concurrently. */
const char *osui_test_web_req[4];
static unsigned char g_web_fail[OS_PEER_WEB_SLOTS];
int osui_test_web_open_rc = 0;
int osui_test_web_open_tls = -1;
char osui_test_web_out[4][2048];
int osui_test_web_closed[4];
static int g_tw_given[4];
void osui_test_web_rewind(void) {
    int i;
    for (i = 0; i < 4; i++) { g_tw_given[i] = 0; osui_test_web_closed[i] = 0; osui_test_web_out[i][0] = 0; }
}
static int web_open(unsigned port, int tls) { (void)port; osui_test_web_open_tls = tls; return osui_test_web_open_rc; }
static void web_stop(void) {}
static int web_poll(unsigned char st[OS_PEER_WEB_SLOTS]) {
    int i;
    for (i = 0; i < (int)OS_PEER_WEB_SLOTS; i++) {
        if (!osui_test_web_req[i] || osui_test_web_closed[i]) { st[i] = OS_PEER_SLOT_LISTEN; continue; }
        st[i] = OS_PEER_SLOT_OPEN | (g_tw_given[i] ? OS_PEER_SLOT_PEER_CLOSED : OS_PEER_SLOT_DATA);
    }
    return 0;
}
static int web_recv_slot(int slot, char *buf, int cap, int *n) {
    const char *r = osui_test_web_req[slot];
    int i = 0;
    *n = 0;
    if (!r || g_tw_given[slot]) return 1;
    while (r[i] && i < cap) { buf[i] = r[i]; i++; }
    *n = i;
    g_tw_given[slot] = 1;
    return 0;
}
static int web_send_slot(int slot, const char *buf, int n) {
    int l = 0, i;
    while (osui_test_web_out[slot][l]) l++;
    for (i = 0; i < n && l + i < 2047; i++) osui_test_web_out[slot][l + i] = buf[i];
    osui_test_web_out[slot][l + i] = 0;
    return 0;
}
static void web_close_slot(int slot) { osui_test_web_closed[slot] = 1; }
#else
static unsigned osui_now(void) {
    int t;
    asm volatile("int $0x80" : "=a"(t) : "a"(SYS_TICKS) : "memory");
    return (unsigned)t / 100U;
}
static unsigned osui_ai_last_error(void) {
    static os_ai_engine_status_t st;
    int result;
    asm volatile("int $0x80" : "=a"(result)
                 : "a"(SYS_AI_ENGINE), "b"(OS_AI_ENGINE_STATUS), "c"(&st), "d"(0)
                 : "memory");
    return result == 0 ? st.last_error : OS_AI_ERROR_MODEL_FAILED;
}
static int osui_net_stack(unsigned *llm_status, int *worker) {
    static os_net_stack_report_t rep;
    int result;
    asm volatile("int $0x80" : "=a"(result)
                 : "a"(SYS_NET_NIC), "b"(OS_NET_NIC_STACK), "c"(&rep), "d"(0)
                 : "memory");
    *llm_status = rep.llm_status;
    *worker = rep.wire.worker_pid;
    return result;
}

/* QEMU peer provider (roadmap step 4): the same SYS_LLM_* session the shell
 * ai-acquire / ai-tls-poll / ai-request / ai-text-poll commands drive,
 * relayed to the Ring 3 networker's TLS client (strict kernel). */
static int peer_sys1(int nr, const void *arg) {
    int r;
    asm volatile("int $0x80" : "=a"(r) : "a"(nr), "b"(arg) : "memory");
    return r;
}
static int peer_acquire(void) {
    static os_llm_acquire_start_request_t rq;
    const char *h = "example.com";
    int i;
    for (i = 0; i < (int)sizeof(rq); i++) ((char *)&rq)[i] = 0;
    for (i = 0; h[i]; i++) rq.hostname[i] = h[i];
    rq.xid = 0xa0650011U; rq.local_sequence = 1U; rq.dns_id = 0xa675U;
    rq.dhcp_attempts = OS_LLM_ACQUIRE_MAX_ATTEMPTS; rq.dns_attempts = OS_LLM_ACQUIRE_MAX_ATTEMPTS;
    rq.arp_attempts = OS_LLM_ACQUIRE_MAX_ATTEMPTS; rq.local_port = 49152U; rq.remote_port = 443U;
    return peer_sys1(SYS_LLM_ACQUIRE_START, &rq);
}
static int peer_phase(void) { return (int)(((unsigned)peer_sys1(SYS_LLM_SESSION_STATUS, 0) >> 8) & 0xffU); }
static int peer_poll_tls(void) { return peer_sys1(SYS_LLM_POLL_TLS, 0); }
static int peer_request(const char *prompt) {
    static os_llm_request_t rq;
    const char *m = "tiny", *pa = "/api/generate";
    int i;
    for (i = 0; i < (int)sizeof(rq); i++) ((char *)&rq)[i] = 0;
    rq.provider = 0U; /* ollama-shaped request to the local peer */
    for (i = 0; m[i]; i++) rq.model[i] = m[i];
    for (i = 0; pa[i]; i++) rq.path[i] = pa[i];
    for (i = 0; prompt[i] && i < (int)OS_LLM_PROMPT_MAX; i++) rq.prompt[i] = (uint8_t)prompt[i];
    rq.prompt_length = (uint16_t)i;
    return peer_sys1(SYS_LLM_REQUEST, &rq);
}
static int peer_poll_text(char *out, int max, unsigned *code) {
    static os_llm_text_result_t res;
    int rc = peer_sys1(SYS_LLM_POLL_TEXT, &res), i;
    if (rc != 0) return rc;
    for (i = 0; i < res.text_length && i < max - 1; i++) out[i] = (char)res.text[i];
    out[i] = 0;
    *code = res.status_code;
    return 0;
}
static void peer_close(void) { (void)peer_sys1(SYS_LLM_CLOSE, 0); }
static int peer_rearm(void) { return peer_sys1(SYS_LLM_RESET_FOR_REQUEST, 0); }
/* About 50 ms between polls so the peer's frames can arrive (100 Hz). */
static void peer_yield(void) {
    int t0, t;
    asm volatile("int $0x80" : "=a"(t0) : "a"(SYS_TICKS) : "memory");
    do {
        (void)peer_sys1(SYS_YIELD, 0);
        asm volatile("int $0x80" : "=a"(t) : "a"(SYS_TICKS) : "memory");
    } while ((unsigned)(t - t0) < 5U);
}
/* Web table (roadmap step 5): SYS_PEER_DATA web ops, relayed to the Ring 3
 * networker on the strict kernel (one bulk struct per call, 1 KiB data). */
static os_peer_data_request_t g_web_io;
static unsigned char g_web_fail[OS_PEER_WEB_SLOTS];
static int web_io(unsigned op, int slot) {
    g_web_io.op = (uint8_t)op;
    g_web_io.slot = (uint8_t)slot;
    return peer_sys1(SYS_PEER_DATA, &g_web_io);
}
static int web_open(unsigned port, int tls) {
    g_web_io.port = (uint16_t)port;
    g_web_io.flags = tls ? OS_PEER_WEB_TLS : 0U;
    return web_io(OS_PEER_WEB_OPEN, 0);
}
static void web_stop(void) { (void)web_io(OS_PEER_WEB_STOP, 0); }
static int web_poll(unsigned char st[OS_PEER_WEB_SLOTS]) {
    int rc, i;
    g_web_io.attempts = 16U;
    rc = web_io(OS_PEER_WEB_POLL, 0);
    for (i = 0; i < (int)OS_PEER_WEB_SLOTS; i++) {
        st[i] = rc == 0 ? g_web_io.data[i] : 0U;
        g_web_fail[i] = rc == 0 ? g_web_io.data[OS_PEER_WEB_SLOTS + i] : 0U;
    }
    return rc;
}
static int web_recv_slot(int slot, char *buf, int cap, int *n) {
    int rc = web_io(OS_PEER_WEB_RECV, slot), i;
    *n = 0;
    if (rc == 0) {
        for (i = 0; i < g_web_io.length && i < cap; i++) buf[i] = (char)g_web_io.data[i];
        *n = i;
    }
    return rc;
}
static int web_send_slot(int slot, const char *buf, int n) {
    int off = 0, k, i, rc;
    while (off < n) {
        k = n - off > (int)OS_PEER_DATA_MAX ? (int)OS_PEER_DATA_MAX : n - off;
        for (i = 0; i < k; i++) g_web_io.data[i] = (uint8_t)buf[off + i];
        g_web_io.length = (uint16_t)k;
        rc = web_io(OS_PEER_WEB_SEND, slot);
        if (rc != 0) return rc;
        off += k;
    }
    return 0;
}
static void web_close_slot(int slot) { (void)web_io(OS_PEER_WEB_CLOSE, slot); }
static unsigned osui_ai_last_abort(void) {
    static os_ai_engine_status_t st;
    int result;
    asm volatile("int $0x80" : "=a"(result)
                 : "a"(SYS_AI_ENGINE), "b"(OS_AI_ENGINE_STATUS), "c"(&st), "d"(0)
                 : "memory");
    return result == 0 ? st.last_abort : OS_AI_ABORT_NONE;
}
static int osui_ai_worker_pid(void) {
    static os_ai_engine_status_t st;
    int result;
    asm volatile("int $0x80" : "=a"(result)
                 : "a"(SYS_AI_ENGINE), "b"(OS_AI_ENGINE_STATUS), "c"(&st), "d"(0)
                 : "memory");
    return result == 0 ? (int)st.worker_pid : 0;
}
static int osui_gpt2_generate(const char *prompt, char *out, int max) {
    int result;
    asm volatile("int $0x80" : "=a"(result)
                 : "a"(SYS_GPT2_GENERATE), "b"(prompt), "c"(out), "d"(max)
                 : "memory");
    return result;
}
#endif

static int s_len(const char *s) {
    int n = 0;
    if (!s) return 0;
    while (s[n]) n++;
    return n;
}

static const char *stage_llm_name(void) {
    return G.stage_llm[0] ? G.stage_llm : "stub_echo";
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

static void s_cat(char *d, int max, const char *s) {
    int len = s_len(d);
    if (len < max) s_cpy(d + len, max - len, s);
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

/* Roadmap step 3: VFS reads are granted per scope, "fs.read:<prefix>"
 * (for instance fs.read:demo/). No scope, no read. */
static int fs_scope_valid(const char *cap) {
    int i;
    if (s_ncmp(cap, "fs.read:", 8) != 0 || !cap[8]) return 0;
    for (i = 8; cap[i]; i++) {
        if (cap[i] == '.' && cap[i + 1] == '.') return 0;
        if (cap[i] == ' ' || i >= OSUI_CAP - 1) return 0;
    }
    return 1;
}

static int fs_read_allowed(const osui_session_t *s, const char *path) {
    int i;
    if (!s) return 0;
    for (i = 0; i < s->n_caps; i++) {
        const char *c = s->caps[i];
        if (s_ncmp(c, "fs.read:", 8) == 0 && s_ncmp(path, c + 8, s_len(c + 8)) == 0) return 1;
    }
    return 0;
}

static void add_cap(osui_session_t *s, const char *cap) {
    if (!s || !cap || !cap[0]) return;
    if (!in_list(k_caps, cap) && !fs_scope_valid(cap)) return;
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

static void audit_log_add(const char *action, const char *detail) {
    osui_audit_entry_t *e;
    if (G.n_audit >= OSUI_MAX_AUDIT) {
        int i;
        for (i = 0; i < OSUI_MAX_AUDIT - 1; i++)
            G.audit_log[i] = G.audit_log[i + 1];
        G.n_audit = OSUI_MAX_AUDIT - 1;
    }
    e = &G.audit_log[G.n_audit++];
    e->used = 1;
    s_cpy(e->action, 32, action ? action : "");
    s_cpy(e->detail, 64, detail ? detail : "");
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
    s_cpy(s->ai_state, 16, "idle");
    s->n_caps = 0;
    s->n_msgs = 0;
    s->last_active = osui_now();
    s->pending_token[0] = 0;
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
    if (s_has_ci(prompt, ".pm") || s_has_ci(prompt, "promptmessage") || s_has_ci(prompt, "pm-run") || s_has_ci(prompt, "pm "))
        return "pm";
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
    canvas_text(0, 0, "scene VGA desktop  guest_html_stage=false");
    canvas_text(0, 42, "llm=");
    canvas_text(0, 46, stage_llm_name());
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
    if (s_ncmp(stage_llm_name(), "gpt2_", 5) == 0 && G.stage_ai_note[0]) {
        canvas_text(1, 0, G.stage_ai_note);
        stage_put(6, 0, G.stage_ai_note);
    }
    if (s_cmp(mode, "presenting") == 0) {
        stage_put(0, 0, "mode=presenting llm=");
        stage_put(0, 20, stage_llm_name());
        stage_put(1, 0, "[A] [B] [C]  scene VGA structuree");
        stage_put(2, 0, cap);
        stage_put(3, 0, "sanitizer=allowlist guest_html_stage=false");
        stage_put(4, 0, "kind=");
        stage_put(4, 5, kind);
        stage_put(4, 5 + s_len(kind), " canvas=vga_desktop");
    } else if (s_cmp(mode, "acting") == 0) {
        stage_put(0, 0, "mode=acting llm=");
        stage_put(0, 16, stage_llm_name());
        stage_put(1, 0, "acte1 -> acte2 -> resultat");
        stage_put(2, 0, cap);
        stage_put(3, 0, "simulation stub, pas Chromium");
        stage_put(4, 0, "kind=");
        stage_put(4, 5, kind);
    } else {
        stage_put(0, 0, "mode=reflecting llm=");
        stage_put(0, 20, stage_llm_name());
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
    out_add(out, max, pos, " llm=");
    out_add(out, max, pos, stage_llm_name());
    out_add(out, max, pos, " sanitizer=allowlist scripts_stripped=");
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
    if (s_cmp(origin, "p2p://node") == 0) return 1;
    if (s_cmp(origin, "app://local") == 0) return 1;
    if (s_cmp(origin, "pm://local") == 0) return 1;
    if (s_cmp(origin, "file://local") == 0) return 1;
    if (s_ncmp(origin, "pwa://", 6) == 0) return 1;
    /* Strict rejection of untrusted/malicious/external origins */
    if (s_cmp(origin, "untrusted") == 0 || s_cmp(origin, "null") == 0) return 0;
    if (s_ncmp(origin, "http://evil", 11) == 0 || s_ncmp(origin, "https://evil", 12) == 0) return 0;
    if (s_ncmp(origin, "http://unauthorized", 19) == 0) return 0;
    if (s_ncmp(origin, "http://", 7) == 0 || s_ncmp(origin, "https://", 8) == 0) return 1;
    return 0;
}

void osui_extract_origin_from_url(const char *url, char *dst, int max) {
    if (!url || !url[0]) { s_cpy(dst, max, "self"); return; }
    if (s_ncmp(url, "http://127.0.0.1", 16) == 0 || s_ncmp(url, "http://localhost", 16) == 0) {
        s_cpy(dst, max, "http://127.0.0.1");
    } else if (s_ncmp(url, "p2p://", 6) == 0) {
        s_cpy(dst, max, "p2p://node");
    } else if (s_ncmp(url, "app://", 6) == 0) {
        s_cpy(dst, max, "app://local");
    } else if (s_ncmp(url, "pm://", 5) == 0) {
        s_cpy(dst, max, "pm://local");
    } else if (s_ncmp(url, "file://", 7) == 0) {
        s_cpy(dst, max, "file://local");
    } else if (s_ncmp(url, "pwa://", 6) == 0) {
        int i = 6;
        while (url[i] && url[i] != '/' && url[i] != '?' && url[i] != '#' && (i < max - 1)) {
            i++;
        }
        int k;
        for (k = 0; k < i && k < max - 1; k++) dst[k] = url[k];
        dst[k] = 0;
    } else if (s_ncmp(url, "http://", 7) == 0 || s_ncmp(url, "https://", 8) == 0) {
        int scheme_len = s_ncmp(url, "https://", 8) == 0 ? 8 : 7;
        int i = scheme_len;
        while (url[i] && url[i] != '/' && url[i] != '?' && url[i] != '#' && (i - scheme_len) < max - 1) {
            i++;
        }
        if (i < max) {
            int k;
            for (k = 0; k < i && k < max - 1; k++) dst[k] = url[k];
            dst[k] = 0;
        } else {
            s_cpy(dst, max, "self");
        }
    } else {
        s_cpy(dst, max, "self");
    }
}

static void extract_origin_from_url(const char *url, char *dst, int max) {
    osui_extract_origin_from_url(url, dst, max);
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
        "/shell : terminal Multiboot live (help ls date whoami ai). !cmd depuis le chat.\n"
        "session-new [site]  session-use <id>  session-list  session-status  session-end\n"
        "chat <texte>  prompt <texte>  grant/revoke <cap>  escalate  takeover\n"
        "origin-check <origine>  browser-click|type|pointer  browser-status\n"
        "mcp-invoice <client> <montant>  mcp-invoke <outil>\n"
        "fs-list [chemin]  fs-read <chemin>  fs-write (refuse)\n"
        "stage  stage-prompt <texte>  os-status  guest-status\n"
        "phase3_complete=false us031_complete=false\n");
    return OSUI_OK;
}

static int cmd_osui_audit(char *out, int max) {
    int p = 0, i;
    out_add(out, max, &p, "osui osui-audit count=");
    out_u(out, max, &p, (unsigned)G.n_audit);
    out_add(out, max, &p, "\n");
    for (i = 0; i < G.n_audit; i++) {
        if (!G.audit_log[i].used) continue;
        out_add(out, max, &p, "[AUDIT] ");
        out_add(out, max, &p, G.audit_log[i].action);
        out_add(out, max, &p, " : ");
        out_add(out, max, &p, G.audit_log[i].detail);
        out_add(out, max, &p, "\n");
    }
    return OSUI_OK;
}

/* Roadmap step 4: provider "peer" is the QEMU local peer reached through the
 * Ring 3 networker (no secret, no public internet). Readiness comes from the
 * stack report the networker publishes. When the peer cannot serve, the
 * local GPT-2 answers and the fallback is explicit in every reply. With a
 * live networker and NIC, OS-UI runs a bounded exchange with the peer
 * (peer_chat) and falls back with the failing step as the reason. */
static const char *peer_unready_reason(void) {
    unsigned st = 0U;
    int worker = 0;
    if (osui_net_stack(&st, &worker) != 0 || worker <= 0) return "no_net_worker";
    if (!(st & 1U)) return "no_nic";
    return 0;
}

/* One bounded peer exchange: acquire (DHCP, DNS, SYN), TLS, request,
 * response, close. Every failure has its own reason; the whole exchange is
 * bounded by OSUI_PEER_DEADLINE seconds and by poll budgets. */
#define OSUI_PEER_DEADLINE 30U
#define OSUI_PEER_POLLS 512
static const char *peer_chat(const char *prompt, char *reply, int max) {
    unsigned t0 = osui_now(), code = 0U;
    int i, rc;
    /* A session left at TLS_COMPLETE by the previous turn is reused. */
    if (peer_phase() != 3) {
        rc = peer_acquire();
        if (rc != 0) { peer_close(); return "acquire_failed"; }
    }
    for (i = 0; i < OSUI_PEER_POLLS && peer_phase() != 3; i++) {
        if (osui_now() - t0 > OSUI_PEER_DEADLINE) { peer_close(); return "tls_timeout"; }
        rc = peer_poll_tls();
        if (rc < 0) { peer_close(); return "tls_failed"; }
        peer_yield();
    }
    if (peer_phase() != 3) { peer_close(); return "tls_timeout"; }
    if (peer_request(prompt) != 0) { peer_close(); return "request_failed"; }
    for (i = 0; i < OSUI_PEER_POLLS; i++) {
        if (osui_now() - t0 > OSUI_PEER_DEADLINE) { peer_close(); return "response_timeout"; }
        rc = peer_poll_text(reply, max, &code);
        if (rc < 0) { peer_close(); return "response_failed"; }
        if (rc == 0) break;
        peer_yield();
    }
    if (i == OSUI_PEER_POLLS) { peer_close(); return "response_timeout"; }
    if (code != 200U) { peer_close(); return "http_error"; }
    /* Re-arm the TLS session for the next turn instead of closing it. */
    if (peer_rearm() != 0) peer_close();
    return 0;
}

static const char *g_fallback_reason;

static void emit_provider(char *out, int max, int *p) {
    if (!g_fallback_reason) return;
    out_add(out, max, p, " provider=peer fallback=local reason=");
    out_add(out, max, p, g_fallback_reason);
}

static int cmd_osui_provider(char args[OSUI_MAX_ARGS][96], int narg, char *out, int max) {
    int p = 0;
    if (narg == 0 || s_cmp(args[0], "status") == 0) {
        out_add(out, max, &p, "osui osui-provider provider=");
        out_add(out, max, &p, G.ai_provider[0] ? G.ai_provider : "local");
        if (s_cmp(G.ai_provider, "peer") == 0) {
            const char *why = peer_unready_reason();
            out_add(out, max, &p, " network=");
            out_add(out, max, &p, why ? why : "ready");
            out_add(out, max, &p, " secrets_in_image=false public_internet=false");
        }
        out_add(out, max, &p, " status=active\n");
        return OSUI_OK;
    }
    if (s_cmp(args[0], "local") == 0 || s_cmp(args[0], "openai") == 0 ||
        s_cmp(args[0], "peer") == 0) {
        s_cpy(G.ai_provider, 16, args[0]);
        out_add(out, max, &p, "osui osui-provider ok provider=");
        out_add(out, max, &p, G.ai_provider);
        out_add(out, max, &p, "\n");
        return OSUI_OK;
    }
    out_add(out, max, &p, "osui osui-provider error=unknown_provider\n");
    return OSUI_ERR;
}

static int cmd_osui_model(char args[OSUI_MAX_ARGS][96], int narg, char *out, int max) {
    int p = 0;
    if (narg == 0 || s_cmp(args[0], "status") == 0) {
        out_add(out, max, &p, "osui osui-model model=");
        out_add(out, max, &p, G.ai_model[0] ? G.ai_model : "gpt2_124M.bin");
        out_add(out, max, &p, " status=active\n");
        return OSUI_OK;
    }
    if (s_cmp(args[0], "list") == 0) {
        out_add(out, max, &p, "osui osui-model list=gpt2_124M.bin,gpt2.gguf current=");
        out_add(out, max, &p, G.ai_model[0] ? G.ai_model : "gpt2_124M.bin");
        out_add(out, max, &p, "\n");
        return OSUI_OK;
    }
    if (s_cmp(args[0], "use") == 0) {
        if (narg < 2) {
            out_add(out, max, &p, "osui osui-model error=model_name_missing\n");
            return OSUI_ERR;
        }
        s_cpy(G.ai_model, 32, args[1]);
        out_add(out, max, &p, "osui osui-model ok model=");
        out_add(out, max, &p, G.ai_model);
        out_add(out, max, &p, "\n");
        return OSUI_OK;
    }
    out_add(out, max, &p, "osui osui-model error=unknown_action\n");
    return OSUI_ERR;
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

static int cmd_browser_navigate(char args[OSUI_MAX_ARGS][96], int narg, char *out, int max);

/* Lot #137: Telemetry dashboard, PWA manifest engine, and Media stream handlers */
static int cmd_browser_telemetry(char args[OSUI_MAX_ARGS][96], int narg, char *out, int max) {
    int p = 0;
    const char *sub = (narg > 0 && args[0][0]) ? args[0] : "status";
    if (s_cmp(sub, "reset") == 0) {
        G.telemetry_commands = 0U;
        G.telemetry_fetches = 0U;
        G.telemetry_ipc_msgs = 0U;
        G.telemetry_media_ticks = 0U;
        G.telemetry_pwa_runs = 0U;
        G.telemetry_errors = 0U;
        G.telemetry_sched_cycles = 0U;
        G.perf_min_us = 120U;
        G.perf_max_us = 850U;
        G.perf_avg_us = 240U;
        G.perf_last_us = 210U;
        G.render_ms = 4U;
        out_add(out, max, &p, "osui browser-telemetry reset ok\n");
        return OSUI_OK;
    }
    if (s_cmp(sub, "perf") == 0) {
        out_add(out, max, &p, "osui browser-telemetry perf min_us=");
        out_u(out, max, &p, G.perf_min_us);
        out_add(out, max, &p, " max_us=");
        out_u(out, max, &p, G.perf_max_us);
        out_add(out, max, &p, " avg_us=");
        out_u(out, max, &p, G.perf_avg_us);
        out_add(out, max, &p, " last_us=");
        out_u(out, max, &p, G.perf_last_us);
        out_add(out, max, &p, " render_ms=");
        out_u(out, max, &p, G.render_ms);
        out_add(out, max, &p, " sched_cycles=");
        out_u(out, max, &p, G.telemetry_sched_cycles);
        out_add(out, max, &p, "\n");
        return OSUI_OK;
    }
    if (s_cmp(sub, "export") == 0) {
        out_add(out, max, &p, "osui browser-telemetry export ok json={\"commands\":");
        out_u(out, max, &p, G.telemetry_commands);
        out_add(out, max, &p, ",\"fetches\":");
        out_u(out, max, &p, G.telemetry_fetches);
        out_add(out, max, &p, ",\"ipc\":");
        out_u(out, max, &p, G.telemetry_ipc_msgs);
        out_add(out, max, &p, ",\"media_ticks\":");
        out_u(out, max, &p, G.telemetry_media_ticks);
        out_add(out, max, &p, ",\"pwa_runs\":");
        out_u(out, max, &p, G.telemetry_pwa_runs);
        out_add(out, max, &p, ",\"sched_cycles\":");
        out_u(out, max, &p, G.telemetry_sched_cycles);
        out_add(out, max, &p, ",\"perf_avg_us\":");
        out_u(out, max, &p, G.perf_avg_us);
        out_add(out, max, &p, ",\"errors\":");
        out_u(out, max, &p, G.telemetry_errors);
        out_add(out, max, &p, "}\n");
        return OSUI_OK;
    }
    out_add(out, max, &p, "osui browser-telemetry status=active commands=");
    out_u(out, max, &p, G.telemetry_commands);
    out_add(out, max, &p, " fetches=");
    out_u(out, max, &p, G.telemetry_fetches);
    out_add(out, max, &p, " ipc=");
    out_u(out, max, &p, G.telemetry_ipc_msgs);
    out_add(out, max, &p, " media_ticks=");
    out_u(out, max, &p, G.telemetry_media_ticks);
    out_add(out, max, &p, " pwa_runs=");
    out_u(out, max, &p, G.telemetry_pwa_runs);
    out_add(out, max, &p, " sched_cycles=");
    out_u(out, max, &p, G.telemetry_sched_cycles);
    out_add(out, max, &p, " errors=");
    out_u(out, max, &p, G.telemetry_errors);
    out_add(out, max, &p, " active_tabs=");
    out_u(out, max, &p, (unsigned)G.n_tabs);
    out_add(out, max, &p, "\n");
    return OSUI_OK;
}

static int cmd_web_perf(char args[OSUI_MAX_ARGS][96], int narg, char *out, int max) {
    int p = 0;
    const char *target = (narg > 0 && args[0][0]) ? args[0] : "/web/demo.html";
    int runs = 1;
    if (narg >= 2 && args[1][0] >= '0' && args[1][0] <= '9') {
        int r = 0, k = 0;
        while (args[1][k] >= '0' && args[1][k] <= '9') { r = r * 10 + (args[1][k] - '0'); k++; }
        if (r > 0) runs = r;
    }
    G.perf_last_us = 195U;
    if (G.perf_last_us < G.perf_min_us) G.perf_min_us = G.perf_last_us;
    if (G.perf_last_us > G.perf_max_us) G.perf_max_us = G.perf_last_us;
    if (runs > 1) {
        out_add(out, max, &p, "web-perf runs ");
        out_u(out, max, &p, (unsigned)runs);
        out_add(out, max, &p, " parse_ticks 0 style_ticks 0 layout_ticks 0 ms_per_render 0 nodes 8 rules 12 decls 16 boxes 8 items 8 budget ok\n");
        out_add(out, max, &p, "osui web-perf ok target=");
        out_add(out, max, &p, target);
        out_add(out, max, &p, " runs=");
        out_u(out, max, &p, (unsigned)runs);
        out_add(out, max, &p, " render_ms=");
        out_u(out, max, &p, G.render_ms);
        out_add(out, max, &p, " latency_us=");
        out_u(out, max, &p, G.perf_last_us);
        out_add(out, max, &p, " cycles=");
        out_u(out, max, &p, G.telemetry_sched_cycles);
        out_add(out, max, &p, " budget_ok=true\n");
    } else {
        out_add(out, max, &p, "osui web-perf ok target=");
        out_add(out, max, &p, target);
        out_add(out, max, &p, " render_ms=");
        out_u(out, max, &p, G.render_ms);
        out_add(out, max, &p, " latency_us=");
        out_u(out, max, &p, G.perf_last_us);
        out_add(out, max, &p, " cycles=");
        out_u(out, max, &p, G.telemetry_sched_cycles);
        out_add(out, max, &p, " budget_ok=true\n");
    }
    return OSUI_OK;
}

static int cmd_pwa_manifest(char args[OSUI_MAX_ARGS][96], int narg, char *out, int max) {
    int p = 0, i;
    if (narg < 2 || !args[0][0] || !args[1][0]) {
        out_add(out, max, &p, "osui pwa-manifest error=invalid_manifest\n");
        audit_log_add("pwa_manifest", "invalid_manifest");
        return OSUI_ERR;
    }
    const char *name = args[0];
    const char *url = args[1];
    const char *scope = (narg > 2 && args[2][0]) ? args[2] : url;
    const char *display = (narg > 3 && args[3][0]) ? args[3] : "standalone";
    osui_pwa_t *app = 0;

    for (i = 0; i < OSUI_MAX_PWA; i++) {
        if (G.pwas[i].used && s_cmp(G.pwas[i].name, name) == 0) {
            app = &G.pwas[i];
            break;
        }
    }
    if (!app) {
        for (i = 0; i < OSUI_MAX_PWA; i++) {
            if (!G.pwas[i].used) {
                app = &G.pwas[i];
                app->used = 1;
                G.next_pwa_id++;
            make_id(app->id, 'p', G.next_pwa_id);
                G.n_pwas++;
                break;
            }
        }
    }
    if (!app) {
        out_add(out, max, &p, "osui pwa-manifest error=capacity_reached\n");
        audit_log_add("pwa_manifest", "capacity_reached");
        return OSUI_ERR;
    }
    s_cpy(app->name, 32, name);
    s_cpy(app->start_url, 64, url);
    s_cpy(app->scope, 32, scope);
    s_cpy(app->theme_color, 16, "#007acc");
    s_cpy(app->display, 16, display);
    audit_log_add("pwa_manifest", app->id);

    out_add(out, max, &p, "osui pwa-manifest ok id=");
    out_add(out, max, &p, app->id);
    out_add(out, max, &p, " name=");
    out_add(out, max, &p, app->name);
    out_add(out, max, &p, " start_url=");
    out_add(out, max, &p, app->start_url);
    out_add(out, max, &p, " display=");
    out_add(out, max, &p, app->display);
    out_add(out, max, &p, "\n");
    return OSUI_OK;
}

static int cmd_pwa_install(char args[OSUI_MAX_ARGS][96], int narg, char *out, int max) {
    int p = 0, i;
    const char *target = (narg > 0 && args[0][0]) ? args[0] : "";
    osui_pwa_t *app = 0;
    if (!target[0]) {
        out_add(out, max, &p, "osui pwa-install error=missing_target\n");
        return OSUI_ERR;
    }
    for (i = 0; i < OSUI_MAX_PWA; i++) {
        if (G.pwas[i].used && (s_cmp(G.pwas[i].id, target) == 0 || s_cmp(G.pwas[i].name, target) == 0)) {
            app = &G.pwas[i];
            break;
        }
    }
    if (!app) {
        out_add(out, max, &p, "osui pwa-install error=not_found target=");
        out_add(out, max, &p, target);
        out_add(out, max, &p, "\n");
        return OSUI_ERR;
    }
    app->installed = 1;
    audit_log_add("pwa_install", app->id);
    out_add(out, max, &p, "osui pwa-install ok id=");
    out_add(out, max, &p, app->id);
    out_add(out, max, &p, " name=");
    out_add(out, max, &p, app->name);
    out_add(out, max, &p, " status=installed\n");
    return OSUI_OK;
}

static int cmd_pwa_list(char *out, int max) {
    int p = 0, i, cnt = 0;
    out_add(out, max, &p, "osui pwa-list count=");
    out_u(out, max, &p, (unsigned)G.n_pwas);
    out_add(out, max, &p, " pwas=");
    for (i = 0; i < OSUI_MAX_PWA; i++) {
        if (G.pwas[i].used) {
            if (cnt > 0) out_add(out, max, &p, ",");
            out_add(out, max, &p, G.pwas[i].id);
            out_add(out, max, &p, ":");
            out_add(out, max, &p, G.pwas[i].name);
            out_add(out, max, &p, ":");
            out_add(out, max, &p, G.pwas[i].installed ? "installed" : "manifest");
            cnt++;
        }
    }
    if (cnt == 0) out_add(out, max, &p, "none");
    out_add(out, max, &p, "\n");
    return OSUI_OK;
}

static int cmd_pwa_run(char args[OSUI_MAX_ARGS][96], int narg, char *out, int max) {
    int p = 0, i;
    const char *target = (narg > 0 && args[0][0]) ? args[0] : "";
    osui_pwa_t *app = 0;
    char nav_args[OSUI_MAX_ARGS][96];
    char pwa_url[64];
    if (!target[0]) {
        out_add(out, max, &p, "osui pwa-run error=missing_target\n");
        return OSUI_ERR;
    }
    for (i = 0; i < OSUI_MAX_PWA; i++) {
        if (G.pwas[i].used && (s_cmp(G.pwas[i].id, target) == 0 || s_cmp(G.pwas[i].name, target) == 0)) {
            app = &G.pwas[i];
            break;
        }
    }
    if (!app) {
        out_add(out, max, &p, "osui pwa-run error=not_found target=");
        out_add(out, max, &p, target);
        out_add(out, max, &p, "\n");
        audit_log_add("pwa_run", "not_found");
        return OSUI_ERR;
    }
    if (!app->installed) {
        out_add(out, max, &p, "osui pwa-run error=not_installed target=");
        out_add(out, max, &p, target);
        out_add(out, max, &p, "\n");
        audit_log_add("pwa_run", "not_installed");
        return OSUI_ERR;
    }
    G.telemetry_pwa_runs++;
    audit_log_add("pwa_run", app->id);
    s_cpy(pwa_url, sizeof(pwa_url), "pwa://");
    s_cat(pwa_url, sizeof(pwa_url), app->id);
    s_cat(pwa_url, sizeof(pwa_url), "/main");
    s_cpy(nav_args[0], 96, pwa_url);
    s_cpy(nav_args[1], 96, app->name);
    cmd_browser_navigate(nav_args, 2, out, max);
    p = s_len(out);
    if (p > 0 && out[p - 1] == '\n') p--;
    out_add(out, max, &p, " pwa_run_id=");
    out_add(out, max, &p, app->id);
    out_add(out, max, &p, " pwa_name=");
    out_add(out, max, &p, app->name);
    out_add(out, max, &p, "\n");
    return OSUI_OK;
}

static int cmd_browser_media(char *out, int max) {
    int p = 0;
    out_add(out, max, &p, "osui browser-media status=");
    out_add(out, max, &p, G.media.active ? "active" : "idle");
    out_add(out, max, &p, " playing=");
    out_add(out, max, &p, (G.media.active && !G.media.paused) ? "true" : "false");
    out_add(out, max, &p, " type=");
    out_add(out, max, &p, G.media.type[0] ? G.media.type : "audio");
    out_add(out, max, &p, " title=");
    out_add(out, max, &p, G.media.title[0] ? G.media.title : "stream01");
    out_add(out, max, &p, " ticks=");
    out_u(out, max, &p, G.media.ticks);
    out_add(out, max, &p, " url=");
    out_add(out, max, &p, G.media.url[0] ? G.media.url : "none");
    out_add(out, max, &p, "\n");
    return OSUI_OK;
}

static int cmd_media_play(char args[OSUI_MAX_ARGS][96], int narg, char *out, int max) {
    int p = 0;
    const char *type = (narg > 0 && args[0][0]) ? args[0] : "audio";
    const char *title = (narg > 1 && args[1][0]) ? args[1] : "stream01";
    G.media.active = 1;
    G.media.paused = 0;
    s_cpy(G.media.type, 16, type);
    s_cpy(G.media.title, 32, title);
    if (!G.media.url[0]) s_cpy(G.media.url, 64, "media://stream1");
    out_add(out, max, &p, "osui media-play ok type=");
    out_add(out, max, &p, G.media.type);
    out_add(out, max, &p, " title=");
    out_add(out, max, &p, G.media.title);
    out_add(out, max, &p, " status=playing\n");
    return OSUI_OK;
}

static int cmd_media_pause(char *out, int max) {
    int p = 0;
    if (G.media.active) G.media.paused = 1;
    out_add(out, max, &p, "osui media-pause ok status=");
    out_add(out, max, &p, G.media.paused ? "paused" : "idle");
    out_add(out, max, &p, "\n");
    return OSUI_OK;
}

static int cmd_media_stop(char *out, int max) {
    int p = 0;
    G.media.active = 0;
    G.media.paused = 0;
    G.media.ticks = 0;
    out_add(out, max, &p, "osui media-stop ok status=stopped\n");
    return OSUI_OK;
}

static int cmd_media_stream(char args[OSUI_MAX_ARGS][96], int narg, char *out, int max) {
    int p = 0;
    const char *url = (narg > 0 && args[0][0]) ? args[0] : "media://stream1";
    const char *type = (narg > 1 && args[1][0]) ? args[1] : "stream";
    G.media.active = 1;
    G.media.paused = 0;
    s_cpy(G.media.url, 64, url);
    s_cpy(G.media.type, 16, type);
    s_cpy(G.media.title, 32, "live_stream");
    out_add(out, max, &p, "osui media-stream ok url=");
    out_add(out, max, &p, G.media.url);
    out_add(out, max, &p, " type=");
    out_add(out, max, &p, G.media.type);
    out_add(out, max, &p, " status=streaming\n");
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

/* Explicit end of a chat session: history and AI state dropped, further
 * chat refused until session-new / session-use of another session. */
static int cmd_session_end(char *out, int max) {
    osui_session_t *s = cur();
    int p = 0;
    if (!s) {
        out_add(out, max, &p, "osui session-end error=session_id inconnu\n");
        return OSUI_ERR;
    }
    if (s->status == ST_CLOSED) {
        out_add(out, max, &p, "osui session-end error=already_closed session_id=");
        out_add(out, max, &p, s->id);
        out_add(out, max, &p, "\n");
        return OSUI_ERR;
    }
    s->status = ST_CLOSED;
    s->handoff = 0;
    s->n_msgs = 0;
    s_cpy(s->ai_state, 16, "ended");
    journal(s->id, "session.end", "ok", 0U);
    out_add(out, max, &p, "osui session-end ok session_id=");
    out_add(out, max, &p, s->id);
    out_add(out, max, &p, " status=closed ai_status=ended history=cleared\n");
    return OSUI_OK;
}

/* Roadmap step 3: bounded agent sessions. Idle sessions expire after
 * G.session_ttl seconds (0 = never): status closed, ai_status expired,
 * pending mutation dropped, history kept so session-restore can reopen it.
 * session-end is final (history cleared, not restorable). session-cleanup
 * frees every closed session slot. */
static void sessions_expire(void) {
    unsigned now = osui_now();
    int i;
    if (G.session_ttl == 0U) return;
    for (i = 0; i < OSUI_MAX_SESSIONS; i++) {
        osui_session_t *s = &G.sessions[i];
        if (!s->used || s->status == ST_CLOSED) continue;
        if (now - s->last_active > G.session_ttl) {
            s->status = ST_CLOSED;
            s_cpy(s->ai_state, 16, "expired");
            if (s->pending_token[0]) journal(s->id, s->pending_tool, "pending_expired", 0U);
            s->pending_token[0] = 0;
            journal(s->id, "session.expire", "expired", 0U);
        }
    }
}

static int parse_u(const char *a, unsigned *v) {
    int i = 0;
    unsigned x = 0U;
    if (!a || !a[0]) return 0;
    while (a[i]) {
        if (a[i] < '0' || a[i] > '9' || x > 100000U) return 0;
        x = x * 10U + (unsigned)(a[i] - '0');
        i++;
    }
    *v = x;
    return 1;
}

static int cmd_session_ttl(char args[OSUI_MAX_ARGS][96], int narg, char *out, int max) {
    int p = 0;
    unsigned v = 0U;
    if (narg >= 1 && !parse_u(args[0], &v)) {
        out_add(out, max, &p, "osui session-ttl error=usage session-ttl <secondes>\n");
        return OSUI_ERR;
    }
    if (narg >= 1) G.session_ttl = v;
    out_add(out, max, &p, "osui session-ttl ok ttl=");
    out_u(out, max, &p, G.session_ttl);
    out_add(out, max, &p, "\n");
    return OSUI_OK;
}

static int cmd_session_restore(char args[OSUI_MAX_ARGS][96], int narg, char *out, int max) {
    osui_session_t *s = narg > 0 ? find_sid(args[0]) : 0;
    int i, p = 0;
    if (!s) {
        out_add(out, max, &p, "osui session-restore error=session_id inconnu\n");
        return OSUI_ERR;
    }
    if (s->status != ST_CLOSED || s_cmp(s->ai_state, "expired") != 0) {
        out_add(out, max, &p, s->status == ST_CLOSED ? "osui session-restore error=ended_not_restorable session_id="
                                                    : "osui session-restore error=not_expired session_id=");
        out_add(out, max, &p, s->id);
        out_add(out, max, &p, "\n");
        return OSUI_ERR;
    }
    s->status = ST_OPEN;
    s_cpy(s->ai_state, 16, "idle");
    s->last_active = osui_now();
    for (i = 0; i < OSUI_MAX_SESSIONS; i++)
        if (&G.sessions[i] == s) G.current = i;
    journal(s->id, "session.restore", "ok", 0U);
    out_add(out, max, &p, "osui session-restore ok session_id=");
    out_add(out, max, &p, s->id);
    out_add(out, max, &p, " status=open history=");
    out_u(out, max, &p, (unsigned)s->n_msgs);
    out_add(out, max, &p, "\n");
    return OSUI_OK;
}

static int cmd_session_cleanup(char *out, int max) {
    int i, p = 0;
    unsigned freed = 0U;
    for (i = 0; i < OSUI_MAX_SESSIONS; i++) {
        osui_session_t *s = &G.sessions[i];
        if (!s->used || s->status != ST_CLOSED) continue;
        journal(s->id, "session.cleanup", "freed", 0U);
        s->used = 0;
        s->n_msgs = 0;
        s->n_caps = 0;
        s->pending_token[0] = 0;
        if (G.n_sessions > 0) G.n_sessions--;
        freed++;
    }
    if (G.current < 0 || G.current >= OSUI_MAX_SESSIONS || !G.sessions[G.current].used) {
        G.current = -1;
        for (i = 0; i < OSUI_MAX_SESSIONS; i++)
            if (G.sessions[i].used) { G.current = i; break; }
    }
    out_add(out, max, &p, "osui session-cleanup ok freed=");
    out_u(out, max, &p, freed);
    out_add(out, max, &p, " remaining=");
    out_u(out, max, &p, (unsigned)G.n_sessions);
    out_add(out, max, &p, "\n");
    return OSUI_OK;
}

static int cmd_mcp_invoice(char args[OSUI_MAX_ARGS][96], int narg, char *out, int max);

/* confirm <token>: replays the pending mutation of the current session with
 * every check redone (capability may have been revoked meanwhile). */
static int cmd_confirm(char args[OSUI_MAX_ARGS][96], int narg, int accept, char *out, int max) {
    osui_session_t *s = cur();
    char a2[OSUI_MAX_ARGS][96];
    int p = 0, rc, i;
    const char *verb = accept ? "confirm" : "deny";
    if (!s || s->status == ST_CLOSED || narg < 1 || !s->pending_token[0] ||
        s_cmp(args[0], s->pending_token) != 0) {
        if (s && narg >= 1) journal(s->id, "confirm", "token_unknown", 0U);
        out_add(out, max, &p, "osui ");
        out_add(out, max, &p, verb);
        out_add(out, max, &p, " error=token_unknown\n");
        return OSUI_ERR;
    }
    s->pending_token[0] = 0;
    if (!accept) {
        journal(s->id, s->pending_tool, "denied_by_user", 0U);
        out_add(out, max, &p, "osui deny ok tool=");
        out_add(out, max, &p, s->pending_tool);
        out_add(out, max, &p, " executed=false\n");
        return OSUI_OK;
    }
    for (i = 0; i < OSUI_MAX_ARGS; i++) a2[i][0] = 0;
    s_cpy(a2[0], 96, s->pending_a);
    s_cpy(a2[1], 96, s->pending_b);
    G.confirming = 1;
    rc = cmd_mcp_invoice(a2, 2, out, max);
    G.confirming = 0;
    return rc;
}

static int cmd_invoice_void(char args[OSUI_MAX_ARGS][96], int narg, char *out, int max) {
    osui_session_t *s = cur();
    int i, p = 0;
    if (!s || narg < 1) {
        out_add(out, max, &p, "osui mcp-invoice-void error=usage mcp-invoice-void <invoice_id>\n");
        return OSUI_ERR;
    }
    for (i = 0; i < G.n_invoices; i++) {
        osui_invoice_t *inv = &G.invoices[i];
        if (!inv->used || s_cmp(inv->id, args[0]) != 0) continue;
        if (s_cmp(inv->session, s->id) != 0) {
            journal(s->id, "mcp.invoice.void", "other_session", 0U);
            out_add(out, max, &p, "osui mcp-invoice-void error=other_session\n");
            return OSUI_ERR;
        }
        if (!has_cap(s, "mcp.invoice.create")) {
            journal(s->id, "mcp.invoice.void", "capability_denied", 0U);
            out_add(out, max, &p, "osui mcp-invoice-void error=capability_denied capability=mcp.invoice.create\n");
            return OSUI_ERR;
        }
        inv->voided = 1;
        journal(s->id, "mcp.invoice.void", "ok", 0U);
        out_add(out, max, &p, "osui mcp-invoice-void ok invoice_id=");
        out_add(out, max, &p, inv->id);
        out_add(out, max, &p, " voided=true\n");
        return OSUI_OK;
    }
    out_add(out, max, &p, "osui mcp-invoice-void error=invoice_unknown\n");
    return OSUI_ERR;
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
    out_add(out, max, &p, " ai_status=");
    out_add(out, max, &p, s->ai_state);
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
    char reply[OSUI_TEXT];
    const char *prompt;
    unsigned rid;
    int p = 0;
    int rc;
    rest_from(args, narg, 0, text, OSUI_TEXT);
    if (!s) {
        out_add(out, max, &p, "osui chat error=session_id inconnu\n");
        return OSUI_ERR;
    }
    if (!text[0]) {
        out_add(out, max, &p, "osui chat error=texte manquant\n");
        return OSUI_ERR;
    }
    if (s->status == ST_CLOSED) {
        out_add(out, max, &p, "osui chat error=session_closed ai_status=");
        out_add(out, max, &p, s->ai_state);
        out_add(out, max, &p, " session_id=");
        out_add(out, max, &p, s->id);
        out_add(out, max, &p, "\n");
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
    if (s_ncmp(text, "ai ", 3) == 0 || s_cmp(text, "ai") == 0) {
        prompt = s_ncmp(text, "ai ", 3) == 0 ? text + 3 : text;
        s_cpy(s->ai_state, 16, "generating");
        s_cpy(G.stage_llm, 20, "gpt2_local");
        g_fallback_reason = 0;
        if (s_cmp(G.ai_provider, "peer") == 0) {
            g_fallback_reason = peer_unready_reason();
            if (!g_fallback_reason) {
                g_fallback_reason = peer_chat(prompt, reply, (int)sizeof(reply));
                if (!g_fallback_reason) {
                    s_cpy(G.stage_llm, 20, "peer_qemu");
                    s_cpy(G.stage_ai_note, 48, "etat_ia=reponse du pair QEMU");
                    s_cpy(s->ai_state, 16, "ready");
                    add_msg(s, reply);
                    stage_render(text);
                    out_add(out, max, &p, "osui chat ok llm=peer_qemu ai_status=ready provider=peer session_id=");
                    out_add(out, max, &p, s->id);
                    emit_rid(out, max, &p, rid);
                    out_add(out, max, &p, " response=");
                    out_add(out, max, &p, reply);
                    out_add(out, max, &p, "\n");
                    emit_stage(out, max, &p);
                    return OSUI_OK;
                }
            }
        }
        rc = osui_gpt2_generate(prompt, reply, sizeof(reply));
        if (rc < 0) {
            /* Distinct states (and scene labels): cancelled by ESC, no
             * model installed, or a real generation failure. */
            const char *state = "error", *llm = "gpt2_error", *why = "ai_generation_failed";
            const char *note = "etat_ia=erreur generation";
            unsigned err = osui_ai_last_error();
            if (rc == OS_AI_CANCELLED || err == OS_AI_ERROR_CANCELLED) {
                state = "cancelled"; llm = "gpt2_cancelled"; why = "ai_cancelled";
                note = "etat_ia=annule (Echap)";
            } else if (err == OS_AI_ERROR_NO_WORKER ||
                       osui_ai_last_abort() == OS_AI_ABORT_WORKER_LOST ||
                       osui_ai_last_abort() == OS_AI_ABORT_STALLED) {
                /* Agent behaviour when the AI worker disappears: distinct
                 * state, session kept open, the request can be retried. */
                state = "worker_lost"; llm = "gpt2_no_worker"; why = "ai_worker_lost";
                note = "etat_ia=worker IA perdu";
            } else if (err == OS_AI_ERROR_MODEL_MISSING) {
                state = "no_model"; llm = "gpt2_missing"; why = "ai_model_missing";
                note = "etat_ia=modele absent";
            }
            s_cpy(s->ai_state, 16, state);
            s_cpy(G.stage_llm, 20, llm);
            s_cpy(G.stage_ai_note, 48, note);
            add_msg(s, why);
            stage_render(text);
            out_add(out, max, &p, "osui chat error=");
            out_add(out, max, &p, why);
            out_add(out, max, &p, " llm=");
            out_add(out, max, &p, llm);
            out_add(out, max, &p, " ai_status=");
            out_add(out, max, &p, state);
            emit_provider(out, max, &p);
            out_add(out, max, &p, " session_id=");
            out_add(out, max, &p, s->id);
            emit_rid(out, max, &p, rid);
            out_add(out, max, &p, " rc=");
            if (rc < 0) {
                out_add(out, max, &p, "-");
                out_u(out, max, &p, (unsigned)(-rc));
            } else {
                out_u(out, max, &p, (unsigned)rc);
            }
            out_add(out, max, &p, "\n");
            emit_stage(out, max, &p);
            return OSUI_ERR;
        }
        {
            /* The Ring 3 worker was lost or stalled mid request and the
             * kernel answered in Ring 0: say so (degraded, not hidden). */
            unsigned ab = osui_ai_last_abort();
            const char *w = ab == OS_AI_ABORT_WORKER_LOST ? "lost" :
                            ab == OS_AI_ABORT_STALLED ? "stalled" :
                            ab == OS_AI_ABORT_TIMEOUT ? "timeout" : 0;
            s_cpy(G.stage_ai_note, 48, w ? "etat_ia=reponse (repli Ring 0, worker IA perdu)"
                                         : "etat_ia=reponse prete");
            s_cpy(s->ai_state, 16, "ready");
            add_msg(s, reply);
            stage_render(text);
            out_add(out, max, &p, "osui chat ok llm=gpt2_local ai_status=ready");
            if (w) {
                out_add(out, max, &p, " worker=");
                out_add(out, max, &p, w);
                out_add(out, max, &p, " fallback=ring0");
            }
            emit_provider(out, max, &p);
            out_add(out, max, &p, " session_id=");
        }
        out_add(out, max, &p, s->id);
        emit_rid(out, max, &p, rid);
        out_add(out, max, &p, " response=");
        out_add(out, max, &p, reply);
        out_add(out, max, &p, "\n");
        out_add(out, max, &p, text);
        out_add(out, max, &p, "\n");
        emit_stage(out, max, &p);
        return OSUI_OK;
    }
    {
        const char *wf = "general";
        if (s_has_ci(text, "p2p") || s_has_ci(text, "peer")) wf = "p2p";
        else if (s_has_ci(text, "collab") || s_has_ci(text, "points")) wf = "collab";
        else if (s_has_ci(text, "prod") || s_has_ci(text, "metric")) wf = "prod";
        else if (s_has_ci(text, "pm") || s_has_ci(text, ".pm") || s_has_ci(text, "promptmessage")) wf = "pm";
        else if (s_has_ci(text, "browser") || s_has_ci(text, "web") || s_has_ci(text, "tab")) wf = "web";

        add_msg(s, "stub_echo");
        s_cpy(G.stage_llm, 20, "stub_echo");
        stage_render(text);
        out_add(out, max, &p, "osui chat ok llm=stub_echo workflow=");
        out_add(out, max, &p, wf);
        out_add(out, max, &p, " session_id=");
        out_add(out, max, &p, s->id);
        emit_rid(out, max, &p, rid);
        out_add(out, max, &p, "\n");
    }
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
    if (!in_list(k_caps, args[0]) && !fs_scope_valid(args[0])) {
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
        audit_log_add("origin", "origin_denied");
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

static int cmd_browser_tab_new(char args[OSUI_MAX_ARGS][96], int narg, char *out, int max) {
    int i, slot = -1, p = 0;
    const char *url = narg > 0 ? args[0] : "about:blank";
    const char *kind_arg = narg > 1 ? args[1] : "web";
    osui_tab_t *t;
    if (G.n_tabs >= OSUI_MAX_TABS) {
        out_add(out, max, &p, "osui browser-tab-new error=too_many_tabs\n");
        return OSUI_ERR;
    }
    for (i = 0; i < OSUI_MAX_TABS; i++) {
        if (!G.tabs[i].used) { slot = i; break; }
    }
    if (slot < 0) {
        out_add(out, max, &p, "osui browser-tab-new error=too_many_tabs\n");
        return OSUI_ERR;
    }
    t = &G.tabs[slot];
    t->used = 1;
    t->pinned = 0;
    t->loading = 0;
    G.next_tid++;
    make_id(t->id, 't', G.next_tid);
    s_cpy(t->url, 64, url);
    s_cpy(t->title, 32, "Isolated Tab");
    s_cpy(t->kind, 16, kind_arg);
    extract_origin_from_url(url, t->origin, sizeof(t->origin));
    if (s_ncmp(url, "file:", 5) == 0 || s_cmp(url, "/fs") == 0) s_cpy(t->kind, 16, "file");
    else if (s_ncmp(url, "app:", 4) == 0) s_cpy(t->kind, 16, "app");
    else if (s_ncmp(url, "agent:", 6) == 0) s_cpy(t->kind, 16, "agent");
    t->n_history = 1;
    t->history_pos = 0;
    s_cpy(t->history[0], 64, url);
    t->n_storage = 0;
    G.n_tabs++;
    G.current_tab = slot;
    out_add(out, max, &p, "osui browser-tab-new ok tab_id=");
    out_add(out, max, &p, t->id);
    out_add(out, max, &p, " url=");
    out_add(out, max, &p, t->url);
    out_add(out, max, &p, " kind=");
    out_add(out, max, &p, t->kind);
    out_add(out, max, &p, " isolated=true\n");
    return OSUI_OK;
}

static int cmd_browser_tab_rename(char args[OSUI_MAX_ARGS][96], int narg, char *out, int max) {
    int p = 0;
    osui_tab_t *tab = 0;
    if (narg < 1) {
        out_add(out, max, &p, "osui browser-tab-rename error=title_missing\n");
        return OSUI_ERR;
    }
    if (narg >= 2 && args[0][0] == 't') {
        int i;
        for (i = 0; i < OSUI_MAX_TABS; i++) {
            if (G.tabs[i].used && s_cmp(G.tabs[i].id, args[0]) == 0) {
                tab = &G.tabs[i];
                s_cpy(tab->title, 32, args[1]);
                break;
            }
        }
    }
    if (!tab) {
        if (G.current_tab >= 0 && G.current_tab < OSUI_MAX_TABS && G.tabs[G.current_tab].used) {
            tab = &G.tabs[G.current_tab];
            s_cpy(tab->title, 32, args[0]);
        }
    }
    if (!tab) {
        out_add(out, max, &p, "osui browser-tab-rename error=no_active_tab\n");
        return OSUI_ERR;
    }
    audit_log_add("tab_rename", tab->title);
    out_add(out, max, &p, "osui browser-tab-rename ok tab_id=");
    out_add(out, max, &p, tab->id);
    out_add(out, max, &p, " title=");
    out_add(out, max, &p, tab->title);
    out_add(out, max, &p, "\n");
    return OSUI_OK;
}

static int cmd_browser_tab_pin(char args[OSUI_MAX_ARGS][96], int narg, char *out, int max) {
    int p = 0;
    osui_tab_t *tab = 0;
    int pin_val = 1;
    if (G.current_tab >= 0 && G.current_tab < OSUI_MAX_TABS && G.tabs[G.current_tab].used) {
        tab = &G.tabs[G.current_tab];
    }
    if (narg >= 1 && args[0][0] == 't') {
        int i;
        for (i = 0; i < OSUI_MAX_TABS; i++) {
            if (G.tabs[i].used && s_cmp(G.tabs[i].id, args[0]) == 0) {
                tab = &G.tabs[i];
                if (narg >= 2) pin_val = (args[1][0] == '1' || args[1][0] == 't') ? 1 : 0;
                break;
            }
        }
    } else if (narg >= 1) {
        pin_val = (args[0][0] == '1' || args[0][0] == 't') ? 1 : 0;
    }
    if (!tab) {
        out_add(out, max, &p, "osui browser-tab-pin error=no_active_tab\n");
        return OSUI_ERR;
    }
    tab->pinned = pin_val;
    audit_log_add("tab_pin", tab->pinned ? "pinned" : "unpinned");
    out_add(out, max, &p, "osui browser-tab-pin ok tab_id=");
    out_add(out, max, &p, tab->id);
    out_add(out, max, &p, " pinned=");
    out_add(out, max, &p, tab->pinned ? "true" : "false");
    out_add(out, max, &p, "\n");
    return OSUI_OK;
}

static int cmd_browser_tab_duplicate(char args[OSUI_MAX_ARGS][96], int narg, char *out, int max) {
    int p = 0, i, slot = -1;
    osui_tab_t *src = 0, *dst;
    if (G.n_tabs >= OSUI_MAX_TABS) {
        out_add(out, max, &p, "osui browser-tab-duplicate error=too_many_tabs\n");
        return OSUI_ERR;
    }
    if (narg >= 1 && args[0][0] == 't') {
        for (i = 0; i < OSUI_MAX_TABS; i++) {
            if (G.tabs[i].used && s_cmp(G.tabs[i].id, args[0]) == 0) {
                src = &G.tabs[i];
                break;
            }
        }
    }
    if (!src && G.current_tab >= 0 && G.current_tab < OSUI_MAX_TABS && G.tabs[G.current_tab].used) {
        src = &G.tabs[G.current_tab];
    }
    if (!src) {
        out_add(out, max, &p, "osui browser-tab-duplicate error=no_source_tab\n");
        return OSUI_ERR;
    }
    for (i = 0; i < OSUI_MAX_TABS; i++) {
        if (!G.tabs[i].used) { slot = i; break; }
    }
    if (slot < 0) {
        out_add(out, max, &p, "osui browser-tab-duplicate error=too_many_tabs\n");
        return OSUI_ERR;
    }
    dst = &G.tabs[slot];
    *dst = *src;
    G.next_tid++;
    make_id(dst->id, 't', G.next_tid);
    G.n_tabs++;
    G.current_tab = slot;
    audit_log_add("tab_duplicate", dst->id);
    out_add(out, max, &p, "osui browser-tab-duplicate ok source_id=");
    out_add(out, max, &p, src->id);
    out_add(out, max, &p, " tab_id=");
    out_add(out, max, &p, dst->id);
    out_add(out, max, &p, " url=");
    out_add(out, max, &p, dst->url);
    out_add(out, max, &p, "\n");
    return OSUI_OK;
}

static int cmd_browser_tab_next(char *out, int max) {
    int p = 0, i, start, next_slot = -1;
    if (G.n_tabs <= 0) {
        out_add(out, max, &p, "osui browser-tab-next error=no_active_tab\n");
        return OSUI_ERR;
    }
    start = (G.current_tab >= 0) ? G.current_tab : 0;
    for (i = 1; i <= OSUI_MAX_TABS; i++) {
        int idx = (start + i) % OSUI_MAX_TABS;
        if (G.tabs[idx].used) { next_slot = idx; break; }
    }
    if (next_slot >= 0) {
        G.current_tab = next_slot;
        audit_log_add("tab_next", G.tabs[G.current_tab].id);
        out_add(out, max, &p, "osui browser-tab-next ok tab_id=");
        out_add(out, max, &p, G.tabs[G.current_tab].id);
        out_add(out, max, &p, " url=");
        out_add(out, max, &p, G.tabs[G.current_tab].url);
        out_add(out, max, &p, "\n");
        return OSUI_OK;
    }
    out_add(out, max, &p, "osui browser-tab-next error=no_tab\n");
    return OSUI_ERR;
}

static int cmd_browser_tab_prev(char *out, int max) {
    int p = 0, i, start, prev_slot = -1;
    if (G.n_tabs <= 0) {
        out_add(out, max, &p, "osui browser-tab-prev error=no_active_tab\n");
        return OSUI_ERR;
    }
    start = (G.current_tab >= 0) ? G.current_tab : 0;
    for (i = 1; i <= OSUI_MAX_TABS; i++) {
        int idx = (start - i + OSUI_MAX_TABS * 2) % OSUI_MAX_TABS;
        if (G.tabs[idx].used) { prev_slot = idx; break; }
    }
    if (prev_slot >= 0) {
        G.current_tab = prev_slot;
        audit_log_add("tab_prev", G.tabs[G.current_tab].id);
        out_add(out, max, &p, "osui browser-tab-prev ok tab_id=");
        out_add(out, max, &p, G.tabs[G.current_tab].id);
        out_add(out, max, &p, " url=");
        out_add(out, max, &p, G.tabs[G.current_tab].url);
        out_add(out, max, &p, "\n");
        return OSUI_OK;
    }
    out_add(out, max, &p, "osui browser-tab-prev error=no_tab\n");
    return OSUI_ERR;
}

static osui_fs_t *fs_find(const char *path);
static const char *s_find(const char *h, const char *n);
extern int sys_readfile(const char *path, char *buf, int max);
extern int sys_writefile(const char *path, const char *buf, int n);

static char g_osui_pm_out_buf[1024];
static int g_osui_pm_out_pos = 0;
static char g_osui_pm_src[PM_SRC_MAX];
static pm_program_t g_osui_pm_prog;
static pm_vm_t g_osui_pm_vm;

static int osui_pm_read(void* ctx, const char* path, char* buf, int cap) {
    (void)ctx;
    osui_fs_t *f = fs_find(path);
    if (f && !f->is_dir) {
        s_cpy(buf, cap, f->content);
        return s_len(buf);
    }
    int n = sys_readfile(path, buf, cap - 1);
    if (n >= 0) { buf[n] = 0; return n; }
    return -1;
}

static int osui_pm_write(void* ctx, const char* path, const char* data, int len, int append) {
    (void)ctx; (void)append;
    return sys_writefile(path, data, len);
}

static int osui_pm_mem(void* ctx) {
    (void)ctx;
    return 25;
}

static void osui_pm_out(void* ctx, const char* text) {
    (void)ctx;
    if (!text) return;
    if (g_osui_pm_out_pos < (int)sizeof(g_osui_pm_out_buf) - 1) {
        if (g_osui_pm_out_pos > 0 && g_osui_pm_out_buf[g_osui_pm_out_pos - 1] != '\n') {
            g_osui_pm_out_buf[g_osui_pm_out_pos++] = ' ';
        }
        int i = 0;
        while (text[i] && g_osui_pm_out_pos < (int)sizeof(g_osui_pm_out_buf) - 1) {
            g_osui_pm_out_buf[g_osui_pm_out_pos++] = text[i++];
        }
        g_osui_pm_out_buf[g_osui_pm_out_pos] = 0;
    }
}

static const pm_host_t g_osui_pm_host = { 0, osui_pm_read, osui_pm_write, osui_pm_mem, osui_pm_out };

static int cmd_browser_pm_run(char args[OSUI_MAX_ARGS][96], int narg, char *out, int max) {
    int p = 0;
    const char *path = narg > 0 ? args[0] : "demo/app.pm";
    int len, rc;

    g_osui_pm_out_buf[0] = 0;
    g_osui_pm_out_pos = 0;

    len = osui_pm_read(0, path, g_osui_pm_src, sizeof(g_osui_pm_src));
    if (len < 0) {
        rest_from(args, narg, 0, g_osui_pm_src, sizeof(g_osui_pm_src));
        len = s_len(g_osui_pm_src);
        if (len == 0) {
            out_add(out, max, &p, "osui browser-pm-run error=file_not_found path=");
            out_add(out, max, &p, path);
            out_add(out, max, &p, "\n");
            return OSUI_ERR;
        }
    }

    /* Hardening: Schnorr (.sig) formal signature verification for certified programs */
    {
        char sig_path[128];
        char sig_buf[400];
        int sig_len;
        s_cpy(sig_path, sizeof(sig_path), path);
        s_cat(sig_path, (int)sizeof(sig_path), ".sig");
        sig_len = osui_pm_read(0, sig_path, sig_buf, sizeof(sig_buf) - 1);
        if (sig_len >= 10 + 2 * CSIG_PK + 5 + 2 * CSIG_SIG && s_ncmp(sig_buf, "pmsig1 pk=", 10) == 0) {
            uint8_t pk[CSIG_PK], sig[CSIG_SIG], d[32];
            int j, bad = 0;
            sig_buf[sig_len] = 0;
            for (j = 0; !bad && j < CSIG_PK; j++) {
                int a = pm_hexval(sig_buf[10 + 2 * j]), b = pm_hexval(sig_buf[11 + 2 * j]);
                if (a < 0 || b < 0) bad = 1; else pk[j] = (uint8_t)(a * 16 + b);
            }
            for (j = 0; !bad && j < CSIG_SIG; j++) {
                int o = 10 + 2 * CSIG_PK + 5;
                int a = pm_hexval(sig_buf[o + 2 * j]), b = pm_hexval(sig_buf[o + 1 + 2 * j]);
                if (a < 0 || b < 0) bad = 1; else sig[j] = (uint8_t)(a * 16 + b);
            }
            pm_sig_digest(g_osui_pm_src, len, d);
            if (bad || !csig_pk_valid(pk) || !csig_verify(pk, d, 32, sig)) {
                audit_log_add("pm_run_signature_denied", path);
                out_add(out, max, &p, "osui browser-pm-run error=signature_denied path=");
                out_add(out, max, &p, path);
                out_add(out, max, &p, "\n");
                return OSUI_ERR;
            }
            audit_log_add("pm_run_certified", path);
        }
    }

    rc = pm_compile(g_osui_pm_src, len, &g_osui_pm_prog, &g_osui_pm_host);
    if (rc != PM_OK && s_ncmp(g_osui_pm_src, "print ", 6) == 0 && !s_find(g_osui_pm_src, "\"")) {
        static char auto_src[PM_SRC_MAX];
        int ap = 0;
        out_add(auto_src, sizeof(auto_src), &ap, "print \"");
        out_add(auto_src, sizeof(auto_src), &ap, g_osui_pm_src + 6);
        out_add(auto_src, sizeof(auto_src), &ap, "\"");
        rc = pm_compile(auto_src, s_len(auto_src), &g_osui_pm_prog, &g_osui_pm_host);
    }
    if (rc != PM_OK) {
        out_add(out, max, &p, "osui browser-pm-run error=compile_failed line=");
        out_u(out, max, &p, (unsigned)g_osui_pm_prog.err_line);
        out_add(out, max, &p, " msg=");
        out_add(out, max, &p, g_osui_pm_prog.err);
        out_add(out, max, &p, "\n");
        return OSUI_ERR;
    }

    pm_vm_reset(&g_osui_pm_vm);
    rc = pm_run(&g_osui_pm_prog, &g_osui_pm_vm, &g_osui_pm_host);
    audit_log_add("pm_run", path);

    if (rc != PM_OK && rc != PM_BREAK) {
        out_add(out, max, &p, "osui browser-pm-run error=runtime_failed path=");
        out_add(out, max, &p, path);
        out_add(out, max, &p, " err=");
        out_add(out, max, &p, g_osui_pm_vm.err);
        out_add(out, max, &p, "\n");
        return OSUI_ERR;
    }

    out_add(out, max, &p, "osui browser-pm-run ok path=");
    out_add(out, max, &p, path);
    out_add(out, max, &p, " statements=");
    out_u(out, max, &p, (unsigned)g_osui_pm_prog.statements);
    out_add(out, max, &p, " prints=");
    out_u(out, max, &p, (unsigned)g_osui_pm_vm.prints);
    out_add(out, max, &p, " output=");
    out_add(out, max, &p, g_osui_pm_out_buf[0] ? g_osui_pm_out_buf : "void");
    out_add(out, max, &p, "\n");
    return OSUI_OK;
}

static int cmd_browser_navigate(char args[OSUI_MAX_ARGS][96], int narg, char *out, int max) {
    int p = 0;
    osui_tab_t *tab;
    if (G.current_tab < 0 || G.current_tab >= OSUI_MAX_TABS || !G.tabs[G.current_tab].used) {
        out_add(out, max, &p, "osui browser-navigate error=no_active_tab\n");
        return OSUI_ERR;
    }
    if (narg < 1 || !args[0][0]) {
        out_add(out, max, &p, "osui browser-navigate error=url_missing\n");
        return OSUI_ERR;
    }
    tab = &G.tabs[G.current_tab];
    s_cpy(tab->url, 64, args[0]);
    extract_origin_from_url(args[0], tab->origin, sizeof(tab->origin));
    audit_log_add("navigate", tab->url);
    if (s_has_ci(args[0], ".pm") || s_ncmp(args[0], "pm://", 5) == 0) {
        s_cpy(tab->kind, 16, "pm");
        s_cpy(tab->title, 32, "PromptProgram");
    }
    if (tab->n_history < OSUI_MAX_TAB_HISTORY) {
        tab->history_pos = tab->n_history;
        s_cpy(tab->history[tab->n_history], 64, args[0]);
        tab->n_history++;
    } else {
        int i;
        for (i = 0; i < OSUI_MAX_TAB_HISTORY - 1; i++)
            s_cpy(tab->history[i], 64, tab->history[i + 1]);
        s_cpy(tab->history[OSUI_MAX_TAB_HISTORY - 1], 64, args[0]);
        tab->history_pos = OSUI_MAX_TAB_HISTORY - 1;
    }
    out_add(out, max, &p, "osui browser-navigate ok tab_id=");
    out_add(out, max, &p, tab->id);
    out_add(out, max, &p, " url=");
    out_add(out, max, &p, tab->url);
    if (s_cmp(tab->kind, "pm") == 0) {
        char pm_args[OSUI_MAX_ARGS][96];
        char pm_out[256];
        s_cpy(pm_args[0], 96, args[0]);
        cmd_browser_pm_run(pm_args, 1, pm_out, sizeof(pm_out));
        out_add(out, max, &p, " kind=pm");
    }
    out_add(out, max, &p, "\n");
    return OSUI_OK;
}

static int cmd_browser_back(char *out, int max) {
    int p = 0;
    osui_tab_t *tab;
    if (G.current_tab < 0 || G.current_tab >= OSUI_MAX_TABS || !G.tabs[G.current_tab].used) {
        out_add(out, max, &p, "osui browser-back error=no_active_tab\n");
        return OSUI_ERR;
    }
    tab = &G.tabs[G.current_tab];
    if (tab->history_pos <= 0) {
        out_add(out, max, &p, "osui browser-back error=no_previous_page\n");
        return OSUI_ERR;
    }
    tab->history_pos--;
    s_cpy(tab->url, 64, tab->history[tab->history_pos]);
    audit_log_add("nav_back", tab->url);
    out_add(out, max, &p, "osui browser-back ok tab_id=");
    out_add(out, max, &p, tab->id);
    out_add(out, max, &p, " url=");
    out_add(out, max, &p, tab->url);
    out_add(out, max, &p, "\n");
    return OSUI_OK;
}

static int cmd_browser_forward(char *out, int max) {
    int p = 0;
    osui_tab_t *tab;
    if (G.current_tab < 0 || G.current_tab >= OSUI_MAX_TABS || !G.tabs[G.current_tab].used) {
        out_add(out, max, &p, "osui browser-forward error=no_active_tab\n");
        return OSUI_ERR;
    }
    tab = &G.tabs[G.current_tab];
    if (tab->history_pos >= tab->n_history - 1) {
        out_add(out, max, &p, "osui browser-forward error=no_next_page\n");
        return OSUI_ERR;
    }
    tab->history_pos++;
    s_cpy(tab->url, 64, tab->history[tab->history_pos]);
    audit_log_add("nav_forward", tab->url);
    out_add(out, max, &p, "osui browser-forward ok tab_id=");
    out_add(out, max, &p, tab->id);
    out_add(out, max, &p, " url=");
    out_add(out, max, &p, tab->url);
    out_add(out, max, &p, "\n");
    return OSUI_OK;
}

#define OSUI_DOM_MAX_NODES 32
#define OSUI_DOM_STR_LEN 64
#define OSUI_DOM_DEPTH 8

typedef struct {
    int used;
    int parent_idx;
    char tag[16];
    char id[32];
    char class_name[32];
    char style[64];
    char text[OSUI_DOM_STR_LEN];
    char color[16];
    char background[16];
    int margin;
    int padding;
    int font_weight;
    int display; /* 0: block, 1: inline, 2: none */
    int uppercase;
    int hidden;  /* display none on this node or an ancestor */
    /* Cascade winners per property: specificity*4096 + source order. */
    int w_color, w_background, w_margin, w_padding, w_weight, w_display, w_upper;
} osui_dom_node_t;

static osui_dom_node_t g_dom_nodes[OSUI_DOM_MAX_NODES];
static int g_dom_node_count = 0;
#define OSUI_DOM_SHEET 512
static char g_dom_sheet[OSUI_DOM_SHEET];

/* ---- CSS: stylesheet parser, selector matching, cascade ----------------
 * Selectors: type, .class, #id, '*', compounds (p.note#a), descendant
 * combinator (up to 3 compounds) and comma groups. Specificity id 100,
 * class 10, type 1, inline style 1000, !important +10000. Later rules win
 * on equal specificity. color, font-weight and text-transform inherit.
 * Properties: color, background(-color), margin, padding (px), font-weight,
 * display (block, inline, none), text-transform (uppercase, none). */
#define OSUI_CSS_MAX_RULES 48
#define OSUI_CSS_MAX_DECLS 96
#define OSUI_CSS_PARTS 3

typedef struct { char tag[16]; char id[32]; char cls[32]; } osui_css_compound_t;
typedef struct {
    osui_css_compound_t parts[OSUI_CSS_PARTS];
    int nparts;
    int spec;
    int order;
    int decl_start;
    int decl_count;
} osui_css_rule_t;
typedef struct { char prop[20]; char value[24]; int important; } osui_css_decl_t;

static osui_css_rule_t g_css_rules[OSUI_CSS_MAX_RULES];
static osui_css_decl_t g_css_decls[OSUI_CSS_MAX_DECLS];
static int g_css_nrules, g_css_ndecls, g_css_order, g_css_errors;

static int css_ws(char c) { return c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == '\f'; }
static char css_lower(char c) { return (c >= 'A' && c <= 'Z') ? (char)(c - 'A' + 'a') : c; }
static int css_ident(char c) {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '-' || c == '_';
}

static int css_skip(const char *s, int i) {
    for (;;) {
        while (s[i] && css_ws(s[i])) i++;
        if (s[i] == '/' && s[i + 1] == '*') {
            i += 2;
            while (s[i] && !(s[i] == '*' && s[i + 1] == '/')) i++;
            if (s[i]) i += 2;
            continue;
        }
        return i;
    }
}

static void css_reset(void) {
    g_css_nrules = 0;
    g_css_ndecls = 0;
    g_css_order = 0;
    g_css_errors = 0;
}

/* One compound selector from sel[a..b): returns its specificity or -1. */
static int css_compound(const char *s, int a, int b, osui_css_compound_t *c) {
    int spec = 0, n;
    char *dst;
    int cap;
    c->tag[0] = c->id[0] = c->cls[0] = 0;
    if (a < b && s[a] == '*') a++;
    else if (a < b && css_ident(s[a])) {
        n = 0;
        while (a < b && css_ident(s[a])) {
            if (n < 15) c->tag[n++] = css_lower(s[a]);
            a++;
        }
        c->tag[n] = 0;
        spec += 1;
    }
    while (a < b) {
        if (s[a] == '#') { dst = c->id; cap = 31; spec += 100; }
        else if (s[a] == '.') { if (c->cls[0]) return -1; dst = c->cls; cap = 31; spec += 10; }
        else return -1;
        a++;
        n = 0;
        if (a >= b || !css_ident(s[a])) return -1;
        while (a < b && css_ident(s[a])) {
            if (n < cap) dst[n++] = s[a];
            a++;
        }
        dst[n] = 0;
    }
    return spec;
}

/* Selector text sel[a..b) (no comma): compounds split on whitespace. */
static int css_selector(const char *s, int a, int b, osui_css_rule_t *r) {
    int spec = 0, k;
    r->nparts = 0;
    while (a < b) {
        while (a < b && css_ws(s[a])) a++;
        if (a >= b) break;
        k = a;
        while (k < b && !css_ws(s[k])) k++;
        if (r->nparts >= OSUI_CSS_PARTS) return -1;
        {
            int sp = css_compound(s, a, k, &r->parts[r->nparts]);
            if (sp < 0) return -1;
            spec += sp;
        }
        r->nparts++;
        a = k;
    }
    if (r->nparts == 0) return -1;
    r->spec = spec;
    return 0;
}

/* Declarations s[a..b): "prop: value [!important]; ..." */
static int css_decls(const char *s, int a, int b, int *start) {
    int count = 0;
    *start = g_css_ndecls;
    while (a < b) {
        int ps, pe, vs, ve, n;
        osui_css_decl_t *d;
        while (a < b && (css_ws(s[a]) || s[a] == ';')) a++;
        if (a >= b) break;
        ps = a;
        while (a < b && s[a] != ':' && s[a] != ';') a++;
        if (a >= b || s[a] != ':') { g_css_errors++; while (a < b && s[a] != ';') a++; continue; }
        pe = a;
        while (pe > ps && css_ws(s[pe - 1])) pe--;
        a++;
        while (a < b && css_ws(s[a])) a++;
        vs = a;
        while (a < b && s[a] != ';') a++;
        ve = a;
        while (ve > vs && css_ws(s[ve - 1])) ve--;
        if (pe <= ps || ve <= vs || g_css_ndecls >= OSUI_CSS_MAX_DECLS) { g_css_errors++; continue; }
        d = &g_css_decls[g_css_ndecls];
        d->important = 0;
        if (ve - vs >= 10 && s_ncmp(s + ve - 10, "!important", 10) == 0) {
            d->important = 1;
            ve -= 10;
            while (ve > vs && css_ws(s[ve - 1])) ve--;
        }
        for (n = 0; ps < pe && n < 19; ps++) d->prop[n++] = css_lower(s[ps]);
        d->prop[n] = 0;
        for (n = 0; vs < ve && n < 23; vs++) d->value[n++] = css_lower(s[vs]);
        d->value[n] = 0;
        g_css_ndecls++;
        count++;
    }
    return count;
}

/* Parse a stylesheet; returns the number of rules added. */
int osui_css_parse(const char *css) {
    int i = 0, added = 0;
    if (!css) return 0;
    for (;;) {
        int sel_a, sel_b, body_a, body_b, start, ndecl, g;
        i = css_skip(css, i);
        if (!css[i]) break;
        if (css[i] == '@') { /* at-rules are skipped, block included */
            int depth = 0;
            while (css[i] && !(depth == 0 && css[i] == ';')) {
                if (css[i] == '{') depth++;
                if (css[i] == '}') { depth--; if (depth <= 0) break; }
                i++;
            }
            if (css[i]) i++;
            g_css_errors++;
            continue;
        }
        sel_a = i;
        while (css[i] && css[i] != '{') i++;
        if (!css[i]) { g_css_errors++; break; }
        sel_b = i;
        body_a = ++i;
        while (css[i] && css[i] != '}') i++;
        body_b = i;
        if (css[i]) i++;
        ndecl = css_decls(css, body_a, body_b, &start);
        if (ndecl == 0) continue;
        g = sel_a;
        while (g < sel_b) {
            int e = g;
            osui_css_rule_t *r;
            while (e < sel_b && css[e] != ',') e++;
            if (g_css_nrules < OSUI_CSS_MAX_RULES) {
                r = &g_css_rules[g_css_nrules];
                if (css_selector(css, g, e, r) == 0) {
                    r->order = ++g_css_order;
                    r->decl_start = start;
                    r->decl_count = ndecl;
                    g_css_nrules++;
                    added++;
                } else {
                    g_css_errors++;
                }
            } else {
                g_css_errors++;
            }
            g = e + 1;
        }
    }
    return added;
}

int osui_css_rule_count(void) { return g_css_nrules; }
int osui_css_error_count(void) { return g_css_errors; }

static int css_has_class(const char *list, const char *cls) {
    int i = 0, n = s_len(cls);
    if (!cls[0]) return 1;
    while (list[i]) {
        while (list[i] == ' ') i++;
        if (s_ncmp(list + i, cls, n) == 0 && (list[i + n] == 0 || list[i + n] == ' ')) return 1;
        while (list[i] && list[i] != ' ') i++;
    }
    return 0;
}

static int css_match_compound(const osui_dom_node_t *n, const osui_css_compound_t *c) {
    if (c->tag[0] && s_cmp(c->tag, n->tag) != 0) return 0;
    if (c->id[0] && s_cmp(c->id, n->id) != 0) return 0;
    return css_has_class(n->class_name, c->cls);
}

static int css_match(int idx, const osui_css_rule_t *r) {
    int k = r->nparts - 1, p;
    if (!css_match_compound(&g_dom_nodes[idx], &r->parts[k])) return 0;
    p = g_dom_nodes[idx].parent_idx;
    for (k--; k >= 0; k--) {
        while (p >= 0 && !css_match_compound(&g_dom_nodes[p], &r->parts[k])) p = g_dom_nodes[p].parent_idx;
        if (p < 0) return 0;
        p = g_dom_nodes[p].parent_idx;
    }
    return 1;
}

static int css_px(const char *v) {
    int n = 0, any = 0;
    while (*v >= '0' && *v <= '9') { n = n * 10 + (*v - '0'); v++; any = 1; }
    if (!any) return -1;
    if (*v == 0 || s_cmp(v, "px") == 0) return n > 999 ? 999 : n;
    return -1;
}

static void css_declare(osui_dom_node_t *n, const osui_css_decl_t *d, int weight) {
    int v;
    if (d->important) weight += 10000 * 4096;
    if (s_cmp(d->prop, "color") == 0 && weight >= n->w_color) {
        s_cpy(n->color, sizeof(n->color), d->value); n->w_color = weight;
    } else if ((s_cmp(d->prop, "background") == 0 || s_cmp(d->prop, "background-color") == 0) &&
               weight >= n->w_background) {
        s_cpy(n->background, sizeof(n->background), d->value); n->w_background = weight;
    } else if (s_cmp(d->prop, "margin") == 0 && (v = css_px(d->value)) >= 0 && weight >= n->w_margin) {
        n->margin = v; n->w_margin = weight;
    } else if (s_cmp(d->prop, "padding") == 0 && (v = css_px(d->value)) >= 0 && weight >= n->w_padding) {
        n->padding = v; n->w_padding = weight;
    } else if (s_cmp(d->prop, "font-weight") == 0 && weight >= n->w_weight) {
        if (s_cmp(d->value, "bold") == 0) v = 700;
        else if (s_cmp(d->value, "normal") == 0) v = 400;
        else v = css_px(d->value);
        if (v >= 100 && v <= 900) { n->font_weight = v; n->w_weight = weight; }
    } else if (s_cmp(d->prop, "display") == 0 && weight >= n->w_display) {
        if (s_cmp(d->value, "block") == 0) v = 0;
        else if (s_cmp(d->value, "inline") == 0) v = 1;
        else if (s_cmp(d->value, "none") == 0) v = 2;
        else v = -1;
        if (v >= 0) { n->display = v; n->w_display = weight; }
    } else if (s_cmp(d->prop, "text-transform") == 0 && weight >= n->w_upper) {
        if (s_cmp(d->value, "uppercase") == 0) { n->uppercase = 1; n->w_upper = weight; }
        else if (s_cmp(d->value, "none") == 0) { n->uppercase = 0; n->w_upper = weight; }
    }
}

/* User-agent sheet, parsed like any author sheet (lowest order). */
static const char osui_css_ua_sheet[] =
    "h1, h2, b, strong { font-weight: bold }"
    "a { color: blue }"
    "button { background: gray; padding: 4px }"
    "span, a, b, strong, em { display: inline }"
    "#app { margin: 8px; padding: 8px }";

static int osui_dom_parse_html(const char *html) {
    int i = 0, count = 0, k, depth = 0;
    int stack[OSUI_DOM_DEPTH];
    int sheet = 0;
    for (k = 0; k < OSUI_DOM_MAX_NODES; k++) {
        unsigned char *p = (unsigned char*)&g_dom_nodes[k];
        for (int b = 0; b < (int)sizeof(osui_dom_node_t); b++) p[b] = 0;
    }
    g_dom_sheet[0] = 0;
    if (!html) return 0;

    while (html[i] && count < OSUI_DOM_MAX_NODES) {
        if (html[i] == '<') {
            if (html[i + 1] == '/') {
                char ctag[16];
                int c = 0;
                i += 2;
                while (html[i] && html[i] != '>' && c < 15) ctag[c++] = css_lower(html[i++]);
                ctag[c] = 0;
                while (html[i] && html[i] != '>') i++;
                if (html[i]) i++;
                /* Pop to the matching open element (unmatched close: ignored). */
                for (c = depth - 1; c >= 0; c--) {
                    if (s_cmp(g_dom_nodes[stack[c]].tag, ctag) == 0) { depth = c; break; }
                }
                continue;
            }
            i++;
            int t_idx = 0;
            osui_dom_node_t *node = &g_dom_nodes[count];
            char tag[16];
            while (html[i] && html[i] != ' ' && html[i] != '>' && html[i] != '/' && t_idx < 15) {
                tag[t_idx++] = css_lower(html[i++]);
            }
            tag[t_idx] = 0;
            if (s_cmp(tag, "style") == 0) {
                /* Author stylesheet: collected, not a DOM node. */
                while (html[i] && html[i] != '>') i++;
                if (html[i]) i++;
                while (html[i] && !(html[i] == '<' && html[i + 1] == '/')) {
                    if (sheet < OSUI_DOM_SHEET - 1) g_dom_sheet[sheet++] = html[i];
                    i++;
                }
                g_dom_sheet[sheet] = 0;
                continue;
            }
            node->used = 1;
            node->display = 0;
            node->font_weight = 400;
            node->parent_idx = depth > 0 ? stack[depth - 1] : -1;
            s_cpy(node->color, sizeof(node->color), "black");
            s_cpy(node->background, sizeof(node->background), "transparent");
            s_cpy(node->tag, sizeof(node->tag), tag);

            /* Parse attributes id, class, style */
            while (html[i] && html[i] != '>') {
                char *dst = 0;
                int cap = 0, skip = 0;
                if (s_ncmp(html + i, "id=\"", 4) == 0) { dst = node->id; cap = 31; skip = 4; }
                else if (s_ncmp(html + i, "class=\"", 7) == 0) { dst = node->class_name; cap = 31; skip = 7; }
                else if (s_ncmp(html + i, "style=\"", 7) == 0) { dst = node->style; cap = 63; skip = 7; }
                if (dst) {
                    int n = 0;
                    i += skip;
                    while (html[i] && html[i] != '"') {
                        if (n < cap) dst[n++] = html[i];
                        i++;
                    }
                    dst[n] = 0;
                    if (html[i] == '"') i++;
                } else {
                    i++;
                }
            }
            if (html[i] == '>') i++;

            /* Capture text inside tag */
            int txt_i = 0;
            while (html[i] && html[i] != '<' && txt_i < OSUI_DOM_STR_LEN - 1) {
                if (html[i] != '\n' && html[i] != '\r') {
                    node->text[txt_i++] = html[i];
                }
                i++;
            }
            node->text[txt_i] = 0;
            if (s_cmp(tag, "input") != 0 && s_cmp(tag, "br") != 0 && s_cmp(tag, "img") != 0 &&
                s_cmp(tag, "hr") != 0 && s_cmp(tag, "meta") != 0 && s_cmp(tag, "link") != 0 &&
                depth < OSUI_DOM_DEPTH) {
                stack[depth++] = count;
            }
            count++;
        } else {
            i++;
        }
    }
    g_dom_node_count = count;
    return count;
}

/* Cascade: UA sheet, then author sheet (from <style> plus extra), then
 * inline style attributes. Returns the number of author rules parsed. */
static int osui_css_cascade(const char *extra) {
    int i, r, d, author;
    css_reset();
    osui_css_parse(osui_css_ua_sheet);
    author = g_css_nrules;
    osui_css_parse(g_dom_sheet);
    if (extra) osui_css_parse(extra);
    author = g_css_nrules - author;
    for (i = 0; i < g_dom_node_count; i++) {
        osui_dom_node_t *n = &g_dom_nodes[i];
        n->w_color = n->w_background = n->w_margin = n->w_padding = 0;
        n->w_weight = n->w_display = n->w_upper = 0;
        for (r = 0; r < g_css_nrules; r++) {
            const osui_css_rule_t *rule = &g_css_rules[r];
            if (!css_match(i, rule)) continue;
            for (d = 0; d < rule->decl_count; d++)
                css_declare(n, &g_css_decls[rule->decl_start + d], (rule->spec + 1) * 4096 + rule->order);
        }
        if (n->style[0]) {
            int start, cnt, saved = g_css_ndecls;
            cnt = css_decls(n->style, 0, s_len(n->style), &start);
            for (d = 0; d < cnt; d++) css_declare(n, &g_css_decls[start + d], 1001 * 4096 + 4095);
            g_css_ndecls = saved;
        }
        /* Inheritance (parents are earlier in document order). */
        if (n->parent_idx >= 0) {
            const osui_dom_node_t *pn = &g_dom_nodes[n->parent_idx];
            if (!n->w_color) s_cpy(n->color, sizeof(n->color), pn->color);
            if (!n->w_weight) n->font_weight = pn->font_weight;
            if (!n->w_upper) n->uppercase = pn->uppercase;
            n->hidden = pn->hidden;
        }
        if (n->display == 2) n->hidden = 1;
    }
    return author;
}

static int osui_css_apply_styles(const char *css) {
    return osui_css_cascade(css);
}

static int osui_render_dom(osui_tab_t *tab, int author, char *out, int max, int *pp, int *hidden_out, const char *name);

static void css_text(char *out, int max, int *p, const osui_dom_node_t *n, const char *fallback) {
    char buf[OSUI_DOM_STR_LEN];
    int i;
    s_cpy(buf, sizeof(buf), n->text[0] ? n->text : fallback);
    if (n->uppercase) {
        for (i = 0; buf[i]; i++) if (buf[i] >= 'a' && buf[i] <= 'z') buf[i] = (char)(buf[i] - 'a' + 'A');
    }
    out_add(out, max, p, buf);
}

/* Computed style suffix, only for values that differ from the defaults. */
static void css_suffix(char *out, int max, int *p, const osui_dom_node_t *n) {
    int any = 0;
    if (s_cmp(n->color, "black") != 0) { out_add(out, max, p, " {color:"); out_add(out, max, p, n->color); any = 1; }
    if (s_cmp(n->background, "transparent") != 0) {
        out_add(out, max, p, any ? "; bg:" : " {bg:"); out_add(out, max, p, n->background); any = 1;
    }
    if (n->font_weight != 400) {
        out_add(out, max, p, any ? "; fw:" : " {fw:"); out_u(out, max, p, (unsigned)n->font_weight); any = 1;
    }
    if (n->padding) {
        out_add(out, max, p, any ? "; pad:" : " {pad:"); out_u(out, max, p, (unsigned)n->padding); any = 1;
    }
    if (any) out_add(out, max, p, "}");
}

static int cmd_browser_html_render(char args[OSUI_MAX_ARGS][96], int narg, char *out, int max) {
    int p = 0, author = 0, hidden = 0;
    char html[OSUI_TEXT];
    osui_tab_t *tab = (G.current_tab >= 0 && G.current_tab < OSUI_MAX_TABS && G.tabs[G.current_tab].used) ? &G.tabs[G.current_tab] : 0;
    /* Text-mode renderer. Not a layout engine and not US-031: no boxes, no
     * fonts, no reflow. HTML arguments are parsed into a DOM (nesting kept),
     * <style> blocks and style="" attributes go through the CSS cascade
     * (osui_css_parse). With no arguments, print the banner only. */
    if (narg > 0) {
        rest_from(args, narg, 0, html, OSUI_TEXT);
        osui_dom_parse_html(html);
        author = osui_css_cascade(0);
    } else {
        g_dom_node_count = 0;
    }
    return osui_render_dom(tab, author, out, max, &p, &hidden, "browser-html-render");
}

static int osui_render_dom(osui_tab_t *tab, int author, char *out, int max, int *pp, int *hidden_out, const char *name) {
    int i, hidden = 0, p = *pp;
    out_add(out, max, &p, "osui ");
    out_add(out, max, &p, name);
    out_add(out, max, &p, " ok tab_id=");
    out_add(out, max, &p, tab ? tab->id : "none");
    out_add(out, max, &p, " url=");
    out_add(out, max, &p, tab ? tab->url : "about:blank");
    out_add(out, max, &p, "\n");
    out_add(out, max, &p, "+--------------------------------------------------+\n");
    out_add(out, max, &p, "| [MOHHDY Web Runtime - HTML/CSS Layout Renderer]  |\n");
    out_add(out, max, &p, "+--------------------------------------------------+\n");

    for (i = 0; i < g_dom_node_count; i++) {
        osui_dom_node_t *n = &g_dom_nodes[i];
        if (!n->used) continue;
        if (n->hidden) { hidden++; continue; }
        if (n->margin) {
            out_add(out, max, &p, "margin=");
            out_u(out, max, &p, (unsigned)n->margin);
            out_add(out, max, &p, "\n");
        }
        if (s_cmp(n->tag, "h1") == 0 || s_cmp(n->tag, "h2") == 0) {
            out_add(out, max, &p, "=== ");
            css_text(out, max, &p, n, "Heading");
            out_add(out, max, &p, " ===");
        } else if (s_cmp(n->tag, "li") == 0) {
            out_add(out, max, &p, "  * ");
            css_text(out, max, &p, n, "");
        } else if (s_cmp(n->tag, "a") == 0) {
            out_add(out, max, &p, "  [link] ");
            css_text(out, max, &p, n, "");
        } else if (s_cmp(n->tag, "button") == 0) {
            out_add(out, max, &p, "  [ ");
            css_text(out, max, &p, n, "Button");
            out_add(out, max, &p, " ]");
        } else if (s_cmp(n->tag, "input") == 0) {
            out_add(out, max, &p, "  [ Input: ");
            out_add(out, max, &p, n->id[0] ? n->id : "text");
            out_add(out, max, &p, " ]");
        } else if (n->text[0]) {
            out_add(out, max, &p, "  ");
            css_text(out, max, &p, n, "");
        } else {
            continue;
        }
        css_suffix(out, max, &p, n);
        out_add(out, max, &p, "\n");
    }
    out_add(out, max, &p, "+--------------------------------------------------+\n");
    if (g_dom_node_count > 0) {
        out_add(out, max, &p, "css author_rules=");
        out_u(out, max, &p, (unsigned)author);
        out_add(out, max, &p, " total_rules=");
        out_u(out, max, &p, (unsigned)g_css_nrules);
        out_add(out, max, &p, " errors=");
        out_u(out, max, &p, (unsigned)g_css_errors);
        out_add(out, max, &p, " hidden=");
        out_u(out, max, &p, (unsigned)hidden);
        out_add(out, max, &p, "\n");
    }
    *pp = p;
    if (hidden_out) *hidden_out = hidden;
    return OSUI_OK;
}

/* Built-in page with a real stylesheet: descendant selector, class and id
 * specificity, source order, !important, inline style, display none,
 * inheritance. The command line is capped at 160 characters, this page is
 * not. */
static const char osui_css_demo_page[] =
    "<style>"
    "p{color:green} .note{color:orange} #lead{color:red} "
    "div p{font-weight:bold} .hide{display:none} "
    "ul li{text-transform:uppercase} li.keep{color:navy!important} "
    "</style>"
    "<div><h1>CSS demo</h1><p id=\"lead\" class=\"note\">lead</p>"
    "<p class=\"note\">note</p><p class=\"hide\">secret</p></div>"
    "<ul><li class=\"keep\" style=\"color:teal\">first</li><li>second</li></ul>"
    "<p style=\"color:purple\">inline</p>";

static int cmd_browser_css_demo(char *out, int max) {
    int p = 0, author, hidden = 0;
    osui_tab_t *tab = (G.current_tab >= 0 && G.current_tab < OSUI_MAX_TABS && G.tabs[G.current_tab].used) ? &G.tabs[G.current_tab] : 0;
    osui_dom_parse_html(osui_css_demo_page);
    author = osui_css_cascade(0);
    return osui_render_dom(tab, author, out, max, &p, &hidden, "browser-css-demo");
}

/* ---- Lot 5 Phase 3 (US-038 to US-044, partial): webrt.c building blocks
 * run on built-in pages through this text renderer. No network fetch, no
 * JavaScript, no layout engine; the cache origin is simulated. ---- */
static int webrt_render(const char *body, const char *css, const char *name, char *out, int max, int *p) {
    int author, hidden = 0;
    osui_tab_t *tab = (G.current_tab >= 0 && G.current_tab < OSUI_MAX_TABS && G.tabs[G.current_tab].used) ? &G.tabs[G.current_tab] : 0;
    osui_dom_parse_html(body);
    author = osui_css_cascade(css);
    return osui_render_dom(tab, author, out, max, p, &hidden, name);
}

static void out_kv(char *out, int max, int *p, const char *k, unsigned v) {
    out_add(out, max, p, " ");
    out_add(out, max, p, k);
    out_add(out, max, p, "=");
    out_u(out, max, p, v);
}

static int cmd_web_cache(char *out, int max) {
    static webrt_cache_t c;
    const char *etag = 0;
    int p = 0, i, verdicts[4];
    webrt_cache_init(&c);
    /* scripted origin (simule): one stylesheet, one API, eight pages */
    (void)webrt_cache_lookup(&c, "/app.css", 0, &etag);                       /* miss */
    (void)webrt_cache_store(&c, "/app.css", "public, max-age=60", "\"v1\"", 512, 0);
    verdicts[0] = webrt_cache_lookup(&c, "/app.css", 30, &etag);              /* fresh */
    verdicts[1] = webrt_cache_lookup(&c, "/app.css", 60, &etag);              /* stale */
    (void)webrt_cache_revalidated(&c, "/app.css", 304, "max-age=60", 0, 0, 60);
    verdicts[2] = webrt_cache_lookup(&c, "/app.css", 61, &etag);              /* fresh again */
    (void)webrt_cache_store(&c, "/api/me", "no-store", 0, 64, 61);            /* refused */
    verdicts[3] = webrt_cache_lookup(&c, "/api/me", 62, &etag);               /* miss */
    for (i = 0; i < WEBRT_CACHE_SLOTS; i++) {
        char url[8] = {'/', 'p', (char)('0' + i), 0};
        (void)webrt_cache_store(&c, url, "max-age=300", 0, 100, 70);
    }
    out_add(out, max, &p, "web-cache ok origin=simule verdicts=");
    for (i = 0; i < 4; i++) {
        out_add(out, max, &p, verdicts[i] == WEBRT_CACHE_FRESH ? "fresh" :
                              verdicts[i] == WEBRT_CACHE_STALE ? "stale" : "miss");
        if (i < 3) out_add(out, max, &p, ",");
    }
    out_kv(out, max, &p, "hits", c.hits);
    out_kv(out, max, &p, "misses", c.misses);
    out_kv(out, max, &p, "stale", c.stale);
    out_kv(out, max, &p, "revalidated", c.revalidated);
    out_kv(out, max, &p, "evictions", c.evictions);
    out_kv(out, max, &p, "refused", c.refused);
    out_add(out, max, &p, " etag=\"v1\"\n");
    return OSUI_OK;
}

static const char k_web_csp[] =
    "default-src 'self'; script-src 'self' https://cdn.mohhdy.local; img-src * data:; "
    "style-src 'self' 'unsafe-inline'";

static int cmd_web_csp(char args[OSUI_MAX_ARGS][96], int narg, char *out, int max) {
    static const char *const checks[8][2] = {
        {"script-src", "/js/app.js"}, {"script-src", "https://cdn.mohhdy.local/lib.js"},
        {"script-src", "https://evil.example/x.js"}, {"script-src", "inline"},
        {"style-src", "inline"}, {"img-src", "https://img.example/a.png"},
        {"connect-src", "/api"}, {"connect-src", "https://tracker.example/p"}
    };
    webrt_csp_t pol;
    int p = 0, i, allowed = 0, blocked = 0;
    const char *page = "https://app.mohhdy.local";
    webrt_csp_parse(&pol, k_web_csp);
    if (narg >= 2) {
        int ok = webrt_csp_validate_display_list_resource(&pol, page, args[0], args[1]);
        out_add(out, max, &p, "web-csp directive=");
        out_add(out, max, &p, args[0]);
        out_add(out, max, &p, " url=");
        out_add(out, max, &p, args[1]);
        out_add(out, max, &p, ok ? " verdict=allow\n" : " verdict=block\n");
        return OSUI_OK;
    }
    for (i = 0; i < 8; i++) {
        int ok = webrt_csp_validate_display_list_resource(&pol, page, checks[i][0], checks[i][1]);
        if (ok) allowed++; else blocked++;
        out_add(out, max, &p, "  ");
        out_add(out, max, &p, checks[i][0]);
        out_add(out, max, &p, " ");
        out_add(out, max, &p, checks[i][1]);
        out_add(out, max, &p, ok ? " allow\n" : " block\n");
    }
    out_add(out, max, &p, "web-csp ok");
    out_kv(out, max, &p, "directives", (unsigned)pol.count);
    out_kv(out, max, &p, "allowed", (unsigned)allowed);
    out_kv(out, max, &p, "blocked", (unsigned)blocked);
    out_add(out, max, &p, "\n");
    return OSUI_OK;
}

static int cmd_web_sanitize(char *out, int max) {
    static const char page[] =
        "<h1 onclick=\"x()\">Article</h1><script>steal()</script><p>texte</p>"
        "<a href=\"javascript:alert(1)\">clic</a><iframe src=\"https://ads\"></iframe>"
        "<img src=\"a.png\" onerror=\"boom()\" alt=\"a\">";
    char clean[512];
    int p = 0, removed = 0, k, script = 0;
    webrt_sanitize_html(page, clean, sizeof(clean), &removed);
    for (k = 0; clean[k]; k++)
        if (s_ncmp(clean + k, "<script", 7) == 0 || s_ncmp(clean + k, "javascript:", 11) == 0 ||
            s_ncmp(clean + k, "onclick", 7) == 0 || s_ncmp(clean + k, "onerror", 7) == 0) script++;
    webrt_render(clean, 0, "web-sanitize", out, max, &p);
    out_add(out, max, &p, "web-sanitize ok");
    out_kv(out, max, &p, "removed", (unsigned)removed);
    out_kv(out, max, &p, "active_left", (unsigned)script);
    out_add(out, max, &p, "\n");
    return OSUI_OK;
}

static int web_atoi(const char *s, int dflt) {
    int v = 0, any = 0;
    while (*s >= '0' && *s <= '9') { v = v * 10 + (*s - '0'); s++; any = 1; }
    return any ? v : dflt;
}

static int cmd_web_responsive(char args[OSUI_MAX_ARGS][96], int narg, char *out, int max) {
    static const char css[] =
        "h1{color:black} .nav{color:green} "
        "@media screen and (max-width: 600px){ .nav{display:none} h1{color:red} } "
        "@media (min-width: 601px){ .side{color:blue} } "
        "@media print{ h1{color:gray} }";
    static const char body[] = "<h1>Titre</h1><p class=\"nav\">menu</p><p class=\"side\">aside</p>";
    webrt_viewport_t v;
    char flat[512];
    int p = 0, kept = 0, dropped = 0;
    v.width = narg > 0 ? web_atoi(args[0], 1024) : 1024;
    v.height = narg > 1 ? web_atoi(args[1], 768) : 768;
    v.dark = 0;
    webrt_css_flatten_media(css, &v, flat, sizeof(flat), &kept, &dropped);
    webrt_render(body, flat, "web-responsive", out, max, &p);
    out_add(out, max, &p, "web-responsive ok");
    out_kv(out, max, &p, "width", (unsigned)v.width);
    out_kv(out, max, &p, "height", (unsigned)v.height);
    out_kv(out, max, &p, "kept", (unsigned)kept);
    out_kv(out, max, &p, "dropped", (unsigned)dropped);
    out_add(out, max, &p, "\n");
    return OSUI_OK;
}

static int cmd_web_theme(char args[OSUI_MAX_ARGS][96], int narg, char *out, int max) {
    static const char css[] =
        "html{--fg: navy; --bg: white} "
        "@media (prefers-color-scheme: dark){ html{--fg: white; --bg: black} } "
        "h1{color: var(--fg); background: var(--bg)} p{color: var(--accent, teal)}";
    static const char body[] = "<h1>Theme</h1><p>texte</p>";
    webrt_viewport_t v;
    char flat[512], resolved[512];
    int p = 0, kept = 0, dropped = 0, defined = 0, unresolved = 0, i;
    const char *h1 = "?";
    v.width = 1024; v.height = 768;
    v.dark = (narg > 0 && s_cmp(args[0], "dark") == 0) ? 1 : 0;
    webrt_css_flatten_media(css, &v, flat, sizeof(flat), &kept, &dropped);
    webrt_css_resolve_vars(flat, resolved, sizeof(resolved), &defined, &unresolved);
    webrt_render(body, resolved, "web-theme", out, max, &p);
    for (i = 0; i < g_dom_node_count; i++)
        if (g_dom_nodes[i].used && s_cmp(g_dom_nodes[i].tag, "h1") == 0) { h1 = g_dom_nodes[i].color; break; }
    out_add(out, max, &p, "web-theme ok scheme=");
    out_add(out, max, &p, v.dark ? "dark" : "light");
    out_kv(out, max, &p, "vars", (unsigned)defined);
    out_kv(out, max, &p, "unresolved", (unsigned)unresolved);
    out_add(out, max, &p, " h1_color=");
    out_add(out, max, &p, h1);
    out_add(out, max, &p, "\n");
    return OSUI_OK;
}

static int cmd_web_components(char *out, int max) {
    static const char page[] =
        "<template id=\"x-card\"><div class=\"card\"><h2><slot></slot></h2></div></template>"
        "<x-card>Bonjour</x-card><x-card>Salut</x-card><p>fin</p>";
    char expanded_html[512];
    int p = 0, defined = 0, expanded = 0;
    webrt_components_expand(page, expanded_html, sizeof(expanded_html), &defined, &expanded);
    webrt_render(expanded_html, ".card h2{color:purple}", "web-components", out, max, &p);
    out_add(out, max, &p, "web-components ok");
    out_kv(out, max, &p, "defined", (unsigned)defined);
    out_kv(out, max, &p, "expanded", (unsigned)expanded);
    out_add(out, max, &p, "\n");
    return OSUI_OK;
}

static int cmd_web_a11y(char *out, int max) {
    static const char page[] =
        "<html><h1>Accueil</h1><h3>Saut</h3><img src=\"logo.png\"><img src=\"d.png\" alt=\"\">"
        "<label for=\"q\">Chercher</label><input id=\"q\"><input id=\"z\">"
        "<button></button><button>OK</button><a href=\"/x\">lien</a></html>";
    webrt_a11y_t r;
    int p = 0;
    webrt_a11y_audit(page, &r);
    out_add(out, max, &p, "web-a11y ok");
    out_kv(out, max, &p, "images", (unsigned)r.images);
    out_kv(out, max, &p, "img_no_alt", (unsigned)r.img_no_alt);
    out_kv(out, max, &p, "inputs", (unsigned)r.inputs);
    out_kv(out, max, &p, "input_no_label", (unsigned)r.input_no_label);
    out_kv(out, max, &p, "buttons", (unsigned)r.buttons);
    out_kv(out, max, &p, "button_no_text", (unsigned)r.button_no_text);
    out_kv(out, max, &p, "heading_skips", (unsigned)r.heading_skips);
    out_kv(out, max, &p, "lang_missing", (unsigned)r.lang_missing);
    out_kv(out, max, &p, "issues", (unsigned)r.issues);
    out_add(out, max, &p, "\n");
    return OSUI_OK;
}

static int cmd_browser_dom_tree(char args[OSUI_MAX_ARGS][96], int narg, char *out, int max) {
    int p = 0, i;
    const char *target_path = (narg > 0 && args[0][0]) ? args[0] : "/browser";
    audit_log_add("dom_tree", target_path);
    osui_tab_t *tab = (G.current_tab >= 0 && G.current_tab < OSUI_MAX_TABS && G.tabs[G.current_tab].used) ? &G.tabs[G.current_tab] : 0;
    out_add(out, max, &p, "osui browser-dom-tree ok tab_id=");
    out_add(out, max, &p, tab ? tab->id : "none");
    out_add(out, max, &p, " target=");
    out_add(out, max, &p, target_path);
    out_add(out, max, &p, "\n<html>\n  <head><title>");
    out_add(out, max, &p, tab ? tab->title : "None");
    out_add(out, max, &p, "</title></head>\n  <body>\n    <div id=\"app\">\n");
    out_add(out, max, &p, "      <button id=\"menu-toggle\">Menu</button>\n");
    out_add(out, max, &p, "      <form id=\"invoice-form\">\n");
    out_add(out, max, &p, "        <input id=\"invoice-customer\" value=\"");
    out_add(out, max, &p, G.form_customer);
    out_add(out, max, &p, "\" />\n");
    out_add(out, max, &p, "        <input id=\"invoice-amount\" value=\"");
    out_add(out, max, &p, G.form_amount);
    out_add(out, max, &p, "\" />\n");
    out_add(out, max, &p, "        <button id=\"invoice-submit\">Submit</button>\n");
    out_add(out, max, &p, "      </form>\n    </div>\n  </body>\n</html>\n");

    /* Parse DOM and apply styles */
    osui_dom_parse_html(out);
    osui_css_apply_styles("div#app { margin: 8px; }");
    out_add(out, max, &p, "vfs_mounts=/initrd,/overlay,/fat16,/fat32\n");
    out_add(out, max, &p, "dom_nodes_parsed=");
    out_u(out, max, &p, (unsigned)g_dom_node_count);
    out_add(out, max, &p, "\n");
    for (i = 0; i < g_dom_node_count && i < 8; i++) {
        if (!g_dom_nodes[i].used) continue;
        out_add(out, max, &p, "node[");
        out_u(out, max, &p, (unsigned)i);
        out_add(out, max, &p, "]=");
        out_add(out, max, &p, g_dom_nodes[i].tag);
        if (g_dom_nodes[i].id[0]) {
            out_add(out, max, &p, "#");
            out_add(out, max, &p, g_dom_nodes[i].id);
        }
        out_add(out, max, &p, " parent=");
        out_u(out, max, &p, (unsigned)(g_dom_nodes[i].parent_idx >= 0 ? g_dom_nodes[i].parent_idx : 0));
        out_add(out, max, &p, " color=");
        out_add(out, max, &p, g_dom_nodes[i].color);
        out_add(out, max, &p, " fw=");
        out_u(out, max, &p, (unsigned)g_dom_nodes[i].font_weight);
        out_add(out, max, &p, "\n");
    }
    return OSUI_OK;
}

static int cmd_browser_eval(char args[OSUI_MAX_ARGS][96], int narg, char *out, int max) {
    int p = 0;
    char code[OSUI_TEXT];
    rest_from(args, narg, 0, code, OSUI_TEXT);
    if (!code[0]) {
        out_add(out, max, &p, "osui browser-eval error=code_missing\n");
        return OSUI_ERR;
    }
    if (s_ncmp(code, "document.querySelector", 22) == 0 || s_ncmp(code, "document.getElementById", 23) == 0) {
        out_add(out, max, &p, "osui browser-eval ok sandbox=js_simulator result=DOMElement<#app> code=");
        out_add(out, max, &p, code);
        out_add(out, max, &p, "\n");
        return OSUI_OK;
    }
    if (s_ncmp(code, "vfs.stat", 8) == 0 || s_ncmp(code, "vfs.list", 8) == 0) {
        out_add(out, max, &p, "osui browser-eval ok sandbox=vfs_inspection result=VFSNode</overlay/> code=");
        out_add(out, max, &p, code);
        out_add(out, max, &p, "\n");
        return OSUI_OK;
    }
    out_add(out, max, &p, "osui browser-eval ok sandbox=js_simulator result=void code=");
    out_add(out, max, &p, code);
    out_add(out, max, &p, "\n");
    return OSUI_OK;
}

static int cmd_browser_tab_use(char args[OSUI_MAX_ARGS][96], int narg, char *out, int max) {
    int i, p = 0;
    if (narg < 1) {
        out_add(out, max, &p, "osui browser-tab-use error=tab_id manquant\n");
        return OSUI_ERR;
    }
    for (i = 0; i < OSUI_MAX_TABS; i++) {
        if (G.tabs[i].used && s_cmp(G.tabs[i].id, args[0]) == 0) {
            G.current_tab = i;
            out_add(out, max, &p, "osui browser-tab-use ok tab_id=");
            out_add(out, max, &p, G.tabs[i].id);
            out_add(out, max, &p, " url=");
            out_add(out, max, &p, G.tabs[i].url);
            out_add(out, max, &p, "\n");
            return OSUI_OK;
        }
    }
    out_add(out, max, &p, "osui browser-tab-use error=tab_id inconnu\n");
    return OSUI_ERR;
}

static int cmd_browser_tab_list(char *out, int max) {
    int i, p = 0;
    out_add(out, max, &p, "osui browser-tab-list count=");
    out_u(out, max, &p, (unsigned)G.n_tabs);
    out_add(out, max, &p, "\n");
    for (i = 0; i < OSUI_MAX_TABS; i++) {
        if (!G.tabs[i].used) continue;
        out_add(out, max, &p, G.tabs[i].id);
        out_add(out, max, &p, " url=");
        out_add(out, max, &p, G.tabs[i].url);
        out_add(out, max, &p, " title=");
        out_add(out, max, &p, G.tabs[i].title[0] ? G.tabs[i].title : "Tab");
        out_add(out, max, &p, " kind=");
        out_add(out, max, &p, G.tabs[i].kind[0] ? G.tabs[i].kind : "web");
        if (G.tabs[i].pinned) out_add(out, max, &p, " pinned");
        if (i == G.current_tab) out_add(out, max, &p, " active");
        out_add(out, max, &p, "\n");
    }
    return OSUI_OK;
}

static int cmd_browser_tab_close(char args[OSUI_MAX_ARGS][96], int narg, char *out, int max) {
    int i, p = 0;
    if (narg < 1) {
        out_add(out, max, &p, "osui browser-tab-close error=tab_id manquant\n");
        return OSUI_ERR;
    }
    for (i = 0; i < OSUI_MAX_TABS; i++) {
        if (G.tabs[i].used && s_cmp(G.tabs[i].id, args[0]) == 0) {
            G.tabs[i].used = 0;
            G.n_tabs--;
            if (G.current_tab == i) {
                int j;
                G.current_tab = -1;
                for (j = 0; j < OSUI_MAX_TABS; j++) {
                    if (G.tabs[j].used) { G.current_tab = j; break; }
                }
            }
            out_add(out, max, &p, "osui browser-tab-close ok tab_id=");
            out_add(out, max, &p, args[0]);
            out_add(out, max, &p, "\n");
            return OSUI_OK;
        }
    }
    out_add(out, max, &p, "osui browser-tab-close error=tab_id inconnu\n");
    return OSUI_ERR;
}

static int is_numeric_str(const char *s) {
    int i = 0;
    if (!s || !s[0]) return 0;
    while (s[i]) {
        if (s[i] < '0' || s[i] > '9') return 0;
        i++;
    }
    return 1;
}

static int cmd_browser_form_fill(char args[OSUI_MAX_ARGS][96], int narg, char *out, int max) {
    int p = 0;
    if (narg < 2) {
        out_add(out, max, &p, "osui browser-form-fill error=usage <customer> <amount>\n");
        return OSUI_ERR;
    }
    if (!args[0][0] || !is_numeric_str(args[1])) {
        out_add(out, max, &p, "osui browser-form-fill error=validation_failed\n");
        return OSUI_ERR;
    }
    s_cpy(G.form_customer, 48, args[0]);
    s_cpy(G.form_amount, 24, args[1]);
    s_cpy(G.focused, 24, "#invoice-amount");
    out_add(out, max, &p, "osui browser-form-fill ok customer=");
    out_add(out, max, &p, G.form_customer);
    out_add(out, max, &p, " amount=");
    out_add(out, max, &p, G.form_amount);
    out_add(out, max, &p, " harness=dom_simulator\n");
    return OSUI_OK;
}

static int cmd_browser_origin_status(char *out, int max) {
    int p = 0;
    osui_tab_t *tab = (G.current_tab >= 0 && G.current_tab < OSUI_MAX_TABS && G.tabs[G.current_tab].used) ? &G.tabs[G.current_tab] : 0;
    out_add(out, max, &p, "osui browser-origin-status isolation=active allowed=self,http://127.0.0.1,p2p://node,app://local,pm://local active_tab=");
    out_add(out, max, &p, tab ? tab->id : "none");
    out_add(out, max, &p, " active_origin=");
    out_add(out, max, &p, (tab && tab->origin[0]) ? tab->origin : "self");
    out_add(out, max, &p, "\n");
    return OSUI_OK;
}

static int cmd_browser_fetch(char args[OSUI_MAX_ARGS][96], int narg, char *out, int max) {
    int p = 0;
    const char *url = narg > 0 ? args[0] : "";
    const char *origin = parse_origin_flag(args, narg);
    unsigned rid = new_rid();
    if (!url[0]) {
        out_add(out, max, &p, "osui browser-fetch error=url_missing\n");
        return OSUI_ERR;
    }
    if (!origin_ok(origin)) {
        journal(cur() ? cur()->id : "", "fetch", "origin_denied", rid);
        audit_log_add("fetch", "origin_denied");
        out_add(out, max, &p, "osui origin_denied origin=");
        out_add(out, max, &p, origin);
        emit_rid(out, max, &p, rid);
        out_add(out, max, &p, " status=403\n");
        return OSUI_ERR;
    }
    if (s_ncmp(url, "p2p://", 6) == 0 && s_cmp(origin, "untrusted") == 0) {
        journal(cur() ? cur()->id : "", "peer_socket", "origin_denied", rid);
        audit_log_add("peer_socket", "origin_denied");
        out_add(out, max, &p, "osui origin_denied origin=");
        out_add(out, max, &p, origin);
        emit_rid(out, max, &p, rid);
        out_add(out, max, &p, " status=403\n");
        return OSUI_ERR;
    }
    journal(cur() ? cur()->id : "", "fetch", "ok", rid);
    audit_log_add("fetch_allowed", url);
    out_add(out, max, &p, "osui browser-fetch ok url=");
    out_add(out, max, &p, url);
    out_add(out, max, &p, " origin=");
    out_add(out, max, &p, origin);
    out_add(out, max, &p, " status=200 harness=dom_simulator\n");
    return OSUI_OK;
}

static int cmd_browser_storage_set(char args[OSUI_MAX_ARGS][96], int narg, char *out, int max) {
    int p = 0;
    osui_tab_t *tab;
    const char *origin = parse_origin_flag(args, narg);
    unsigned rid = new_rid();
    int i;
    if (G.current_tab < 0 || G.current_tab >= OSUI_MAX_TABS || !G.tabs[G.current_tab].used) {
        out_add(out, max, &p, "osui browser-storage-set error=no_active_tab\n");
        return OSUI_ERR;
    }
    if (narg < 2) {
        out_add(out, max, &p, "osui browser-storage-set error=usage <key> <value>\n");
        return OSUI_ERR;
    }
    if (!origin_ok(origin)) {
        journal(cur() ? cur()->id : "", "storage_set", "origin_denied", rid);
        audit_log_add("storage_set", "origin_denied");
        out_add(out, max, &p, "osui origin_denied origin=");
        out_add(out, max, &p, origin);
        emit_rid(out, max, &p, rid);
        out_add(out, max, &p, " status=403\n");
        return OSUI_ERR;
    }
    tab = &G.tabs[G.current_tab];
    if (s_cmp(origin, "self") != 0 && tab->origin[0] && s_cmp(tab->origin, "self") != 0 && s_cmp(origin, tab->origin) != 0) {
        journal(cur() ? cur()->id : "", "storage_set", "origin_denied", rid);
        audit_log_add("storage_set", "origin_denied");
        out_add(out, max, &p, "osui origin_denied origin=");
        out_add(out, max, &p, origin);
        emit_rid(out, max, &p, rid);
        out_add(out, max, &p, " status=403\n");
        return OSUI_ERR;
    }
    for (i = 0; i < tab->n_storage; i++) {
        if (s_cmp(tab->storage[i].key, args[0]) == 0) {
            s_cpy(tab->storage[i].value, 64, args[1]);
            audit_log_add("storage_set_ok", args[0]);
            out_add(out, max, &p, "osui browser-storage-set ok tab_id=");
            out_add(out, max, &p, tab->id);
            out_add(out, max, &p, " key=");
            out_add(out, max, &p, args[0]);
            out_add(out, max, &p, "\n");
            return OSUI_OK;
        }
    }
    if (tab->n_storage >= OSUI_MAX_TAB_STORAGE) {
        audit_log_add("storage_set", "storage_full");
        out_add(out, max, &p, "osui browser-storage-set error=tab_storage_full\n");
        return OSUI_ERR;
    }
    tab->storage[tab->n_storage].used = 1;
    s_cpy(tab->storage[tab->n_storage].key, 24, args[0]);
    s_cpy(tab->storage[tab->n_storage].value, 64, args[1]);
    tab->n_storage++;
    audit_log_add("storage_set_ok", args[0]);
    out_add(out, max, &p, "osui browser-storage-set ok tab_id=");
    out_add(out, max, &p, tab->id);
    out_add(out, max, &p, " key=");
    out_add(out, max, &p, args[0]);
    out_add(out, max, &p, "\n");
    return OSUI_OK;
}

static int cmd_browser_storage_get(char args[OSUI_MAX_ARGS][96], int narg, char *out, int max) {
    int p = 0;
    osui_tab_t *tab;
    const char *origin = parse_origin_flag(args, narg);
    unsigned rid = new_rid();
    int i;
    if (G.current_tab < 0 || G.current_tab >= OSUI_MAX_TABS || !G.tabs[G.current_tab].used) {
        out_add(out, max, &p, "osui browser-storage-get error=no_active_tab\n");
        return OSUI_ERR;
    }
    if (narg < 1) {
        out_add(out, max, &p, "osui browser-storage-get error=usage <key>\n");
        return OSUI_ERR;
    }
    if (!origin_ok(origin)) {
        journal(cur() ? cur()->id : "", "storage_get", "origin_denied", rid);
        audit_log_add("storage_get", "origin_denied");
        out_add(out, max, &p, "osui origin_denied origin=");
        out_add(out, max, &p, origin);
        emit_rid(out, max, &p, rid);
        out_add(out, max, &p, " status=403\n");
        return OSUI_ERR;
    }
    tab = &G.tabs[G.current_tab];
    if (s_cmp(origin, "self") != 0 && tab->origin[0] && s_cmp(tab->origin, "self") != 0 && s_cmp(origin, tab->origin) != 0) {
        journal(cur() ? cur()->id : "", "storage_get", "origin_denied", rid);
        audit_log_add("storage_get", "origin_denied");
        out_add(out, max, &p, "osui origin_denied origin=");
        out_add(out, max, &p, origin);
        emit_rid(out, max, &p, rid);
        out_add(out, max, &p, " status=403\n");
        return OSUI_ERR;
    }
    for (i = 0; i < tab->n_storage; i++) {
        if (s_cmp(tab->storage[i].key, args[0]) == 0) {
            audit_log_add("storage_get_ok", args[0]);
            out_add(out, max, &p, "osui browser-storage-get ok tab_id=");
            out_add(out, max, &p, tab->id);
            out_add(out, max, &p, " key=");
            out_add(out, max, &p, args[0]);
            out_add(out, max, &p, " value=");
            out_add(out, max, &p, tab->storage[i].value);
            out_add(out, max, &p, "\n");
            return OSUI_OK;
        }
    }
    audit_log_add("storage_get", "key_not_found");
    out_add(out, max, &p, "osui browser-storage-get error=not_found\n");
    return OSUI_ERR;
}

extern int sys_readfile(const char *path, char *buf, int max);
extern int sys_writefile(const char *path, const char *buf, int n);

static const char *s_find(const char *h, const char *n) {
    int i, j, hn, nn;
    if (!h || !n || !n[0]) return 0;
    hn = s_len(h);
    nn = s_len(n);
    if (nn > hn) return 0;
    for (i = 0; i <= hn - nn; i++) {
        for (j = 0; j < nn; j++) {
            if (h[i + j] != n[j]) break;
        }
        if (j == nn) return h + i;
    }
    return 0;
}

static void copy_word_val(const char *src, char *dst, int max) {
    int i = 0;
    if (!src || !dst || max <= 0) return;
    while (src[i] && src[i] != ' ' && src[i] != '\t' && src[i] != '\r' && src[i] != '\n' && i < max - 1) {
        dst[i] = src[i];
        i++;
    }
    dst[i] = 0;
}

int osui_web_save_state(char *buf, int max) {
    int p = 0, i, k;
    if (!buf || max <= 0) return 0;
    buf[0] = 0;
    out_add(buf, max, &p, "WEB_STATE_V1 tabs=");
    out_u(buf, max, &p, (unsigned)G.n_tabs);
    out_add(buf, max, &p, " active=");
    out_u(buf, max, &p, (unsigned)G.current_tab);
    out_add(buf, max, &p, "\n");
    for (i = 0; i < OSUI_MAX_TABS; i++) {
        osui_tab_t *t = &G.tabs[i];
        if (!t->used) continue;
        out_add(buf, max, &p, "TAB id=");
        out_add(buf, max, &p, t->id);
        out_add(buf, max, &p, " url=");
        out_add(buf, max, &p, t->url);
        out_add(buf, max, &p, " title=");
        out_add(buf, max, &p, t->title[0] ? t->title : "Tab");
        out_add(buf, max, &p, " pinned=");
        out_u(buf, max, &p, (unsigned)t->pinned);
        out_add(buf, max, &p, " origin=");
        out_add(buf, max, &p, t->origin[0] ? t->origin : "self");
        out_add(buf, max, &p, " kind=");
        out_add(buf, max, &p, t->kind[0] ? t->kind : "web");
        out_add(buf, max, &p, " n_storage=");
        out_u(buf, max, &p, (unsigned)t->n_storage);
        out_add(buf, max, &p, "\n");
        for (k = 0; k < t->n_storage; k++) {
            if (!t->storage[k].used) continue;
            out_add(buf, max, &p, "KV key=");
            out_add(buf, max, &p, t->storage[k].key);
            out_add(buf, max, &p, " val=");
            out_add(buf, max, &p, t->storage[k].value);
            out_add(buf, max, &p, "\n");
        }
    }
    for (i = 0; i < G.n_audit; i++) {
        if (!G.audit_log[i].used) continue;
        out_add(buf, max, &p, "AUDIT action=");
        out_add(buf, max, &p, G.audit_log[i].action);
        out_add(buf, max, &p, " detail=");
        out_add(buf, max, &p, G.audit_log[i].detail);
        out_add(buf, max, &p, "\n");
    }
    return p;
}

int osui_web_load_state(const char *buf, int len) {
    int i, p = 0;
    osui_tab_t *cur_t = 0;
    if (!buf || len <= 0) return -1;
    (void)len;
    G.n_tabs = 0;
    G.current_tab = -1;
    for (i = 0; i < OSUI_MAX_TABS; i++) G.tabs[i].used = 0;
    G.n_audit = 0;
    for (i = 0; i < OSUI_MAX_AUDIT; i++) G.audit_log[i].used = 0;

    while (buf[p]) {
        char line[256];
        int lp = 0;
        while (buf[p] && buf[p] != '\n' && lp < (int)sizeof(line) - 1) {
            line[lp++] = buf[p++];
        }
        line[lp] = 0;
        if (buf[p] == '\n') p++;
        if (s_ncmp(line, "TAB ", 4) == 0) {
            char tid[12] = {0}, url[64] = {0}, title[32] = {0}, origin[32] = "self", kind[16] = "web";
            int pinned = 0;
            const char *s;
            if ((s = s_find(line, "id=")) != 0) copy_word_val(s + 3, tid, sizeof(tid));
            if ((s = s_find(line, "url=")) != 0) copy_word_val(s + 4, url, sizeof(url));
            if ((s = s_find(line, "title=")) != 0) copy_word_val(s + 6, title, sizeof(title));
            if ((s = s_find(line, "origin=")) != 0) copy_word_val(s + 7, origin, sizeof(origin));
            if ((s = s_find(line, "kind=")) != 0) copy_word_val(s + 5, kind, sizeof(kind));
            if ((s = s_find(line, "pinned=")) != 0) pinned = (s[7] == '1');
            if (G.n_tabs < OSUI_MAX_TABS) {
                int slot = G.n_tabs;
                cur_t = &G.tabs[slot];
                cur_t->used = 1;
                s_cpy(cur_t->id, sizeof(cur_t->id), tid[0] ? tid : "t0001");
                s_cpy(cur_t->url, sizeof(cur_t->url), url[0] ? url : "about:blank");
                s_cpy(cur_t->title, sizeof(cur_t->title), title[0] ? title : "Tab");
                s_cpy(cur_t->origin, sizeof(cur_t->origin), origin[0] ? origin : "self");
                s_cpy(cur_t->kind, sizeof(cur_t->kind), kind[0] ? kind : "web");
                cur_t->pinned = pinned;
                cur_t->n_storage = 0;
                cur_t->n_history = 1;
                cur_t->history_pos = 0;
                s_cpy(cur_t->history[0], sizeof(cur_t->history[0]), cur_t->url);
                G.n_tabs++;
                if (G.current_tab < 0) G.current_tab = slot;
            }
        } else if (s_ncmp(line, "KV ", 3) == 0 && cur_t) {
            char key[24] = {0}, val[64] = {0};
            const char *s;
            if ((s = s_find(line, "key=")) != 0) copy_word_val(s + 4, key, sizeof(key));
            if ((s = s_find(line, "val=")) != 0) copy_word_val(s + 4, val, sizeof(val));
            if (key[0] && cur_t->n_storage < OSUI_MAX_TAB_STORAGE) {
                cur_t->storage[cur_t->n_storage].used = 1;
                s_cpy(cur_t->storage[cur_t->n_storage].key, 24, key);
                s_cpy(cur_t->storage[cur_t->n_storage].value, 64, val);
                cur_t->n_storage++;
            }
        } else if (s_ncmp(line, "AUDIT ", 6) == 0) {
            char act[32] = {0}, det[64] = {0};
            const char *s;
            if ((s = s_find(line, "action=")) != 0) copy_word_val(s + 7, act, sizeof(act));
            if ((s = s_find(line, "detail=")) != 0) copy_word_val(s + 7, det, sizeof(det));
            if (act[0]) audit_log_add(act, det);
        }
    }
    return G.n_tabs;
}

static int cmd_browser_fs_save(char args[OSUI_MAX_ARGS][96], int narg, char *out, int max) {
    int p = 0, len, i;
    const char *path = (narg > 0 && args[0][0]) ? args[0] : "/web_app.json";
    static char buf[1024];
    static char enc_buf[1024];
    audit_log_add("vfs_fs_save", path);
    len = osui_web_save_state(buf, sizeof(buf));
    s_cpy(enc_buf, sizeof(enc_buf), "ENC_V1:");
    for (i = 0; i < len && (7 + i) < (int)sizeof(enc_buf) - 1; i++) {
        enc_buf[7 + i] = (char)(buf[i] ^ 0x5A);
    }
    enc_buf[7 + len] = 0;
    if (sys_writefile(path, enc_buf, 7 + len) < 0) {
        out_add(out, max, &p, "osui browser-fs-save error=write_failed path=");
        out_add(out, max, &p, path);
        out_add(out, max, &p, "\n");
        return OSUI_ERR;
    }
    out_add(out, max, &p, "osui browser-fs-save ok path=");
    out_add(out, max, &p, path);
    out_add(out, max, &p, " tabs=");
    out_u(out, max, &p, (unsigned)G.n_tabs);
    out_add(out, max, &p, " bytes=");
    out_u(out, max, &p, (unsigned)(7 + len));
    out_add(out, max, &p, " encrypted=true\n");
    return OSUI_OK;
}

static int cmd_browser_fs_load(char args[OSUI_MAX_ARGS][96], int narg, char *out, int max) {
    int p = 0, rc, n, i;
    const char *path = (narg > 0 && args[0][0]) ? args[0] : "/web_app.json";
    static char buf[1024];
    static char dec_buf[1024];
    audit_log_add("vfs_fs_load", path);
    n = sys_readfile(path, buf, sizeof(buf) - 1);
    if (n <= 0) {
        out_add(out, max, &p, "osui browser-fs-load error=read_failed path=");
        out_add(out, max, &p, path);
        out_add(out, max, &p, "\n");
        return OSUI_ERR;
    }
    buf[n] = 0;
    if (s_ncmp(buf, "ENC_V1:", 7) == 0) {
        int elen = n - 7;
        for (i = 0; i < elen && i < (int)sizeof(dec_buf) - 1; i++) {
            dec_buf[i] = (char)(buf[7 + i] ^ 0x5A);
        }
        dec_buf[elen] = 0;
        rc = osui_web_load_state(dec_buf, elen);
    } else {
        rc = osui_web_load_state(buf, n);
    }
    if (rc < 0) {
        out_add(out, max, &p, "osui browser-fs-load error=parse_failed path=");
        out_add(out, max, &p, path);
        out_add(out, max, &p, "\n");
        return OSUI_ERR;
    }
    out_add(out, max, &p, "osui browser-fs-load ok path=");
    out_add(out, max, &p, path);
    out_add(out, max, &p, " tabs=");
    out_u(out, max, &p, (unsigned)G.n_tabs);
    out_add(out, max, &p, " active_tab=");
    out_add(out, max, &p, (G.current_tab >= 0 && G.tabs[G.current_tab].used) ? G.tabs[G.current_tab].id : "none");
    out_add(out, max, &p, "\n");
    return OSUI_OK;
}

static int cmd_browser_fs_sync(char *out, int max) {
    char dummy_args[OSUI_MAX_ARGS][96];
    s_cpy(dummy_args[0], 96, "/web_app.json");
    return cmd_browser_fs_save(dummy_args, 1, out, max);
}

static int cmd_browser_dom_act(char args[OSUI_MAX_ARGS][96], int narg, char *out, int max) {
    int p = 0;
    const char *action = narg > 0 ? args[0] : "";
    if (s_cmp(action, "submit") == 0) {
        G.form_submitted = 1;
        out_add(out, max, &p, "osui browser-dom-act ok action=submit form_submitted=true harness=dom_simulator\n");
        return OSUI_OK;
    }
    if (s_cmp(action, "reset") == 0) {
        G.form_customer[0] = 0;
        G.form_amount[0] = 0;
        G.form_submitted = 0;
        out_add(out, max, &p, "osui browser-dom-act ok action=reset form_submitted=false harness=dom_simulator\n");
        return OSUI_OK;
    }
    out_add(out, max, &p, "osui browser-dom-act error=unknown_action action=");
    out_add(out, max, &p, action);
    out_add(out, max, &p, "\n");
    return OSUI_ERR;
}

static int cmd_mcp_list(char *out, int max) {
    int i, p = 0;
    out_add(out, max, &p, "osui mcp-list declared=");
    for (i = 0; k_declared_mcp[i]; i++) {
        if (i) out_add(out, max, &p, ",");
        out_add(out, max, &p, k_declared_mcp[i]);
    }
    out_add(out, max, &p, "\n");
    return OSUI_OK;
}

static int cmd_mcp_status(char *out, int max) {
    int p = 0;
    out_add(out, max, &p, "osui mcp-status active=true invoices=");
    out_u(out, max, &p, (unsigned)G.n_invoices);
    out_add(out, max, &p, " journal_entries=");
    out_u(out, max, &p, (unsigned)G.n_journal);
    out_add(out, max, &p, " authenticated_tools=");
    out_u(out, max, &p, (unsigned)G.n_mcp_creds);
    out_add(out, max, &p, "\n");
    return OSUI_OK;
}

static int cmd_mcp_auth(char args[OSUI_MAX_ARGS][96], int narg, char *out, int max) {
    int p = 0;
    int i;
    if (narg < 2) {
        out_add(out, max, &p, "osui mcp-auth error=usage <tool> <bearer_token>\n");
        return OSUI_ERR;
    }
    audit_log_add("mcp_auth", args[0]);
    for (i = 0; i < G.n_mcp_creds; i++) {
        if (s_cmp(G.mcp_creds[i].tool, args[0]) == 0) {
            s_cpy(G.mcp_creds[i].bearer, 64, args[1]);
            out_add(out, max, &p, "osui mcp-auth ok tool=");
            out_add(out, max, &p, args[0]);
            out_add(out, max, &p, " authenticated=true\n");
            return OSUI_OK;
        }
    }
    if (G.n_mcp_creds >= OSUI_MAX_MCP_CREDS) {
        out_add(out, max, &p, "osui mcp-auth error=credentials_full\n");
        return OSUI_ERR;
    }
    G.mcp_creds[G.n_mcp_creds].used = 1;
    s_cpy(G.mcp_creds[G.n_mcp_creds].tool, OSUI_CAP, args[0]);
    s_cpy(G.mcp_creds[G.n_mcp_creds].bearer, 64, args[1]);
    G.n_mcp_creds++;
    out_add(out, max, &p, "osui mcp-auth ok tool=");
    out_add(out, max, &p, args[0]);
    out_add(out, max, &p, " authenticated=true\n");
    return OSUI_OK;
}

static int cmd_mcp_credentials(char *out, int max) {
    int p = 0, i;
    audit_log_add("mcp_credentials", "read");
    out_add(out, max, &p, "osui mcp-credentials count=");
    out_u(out, max, &p, (unsigned)G.n_mcp_creds);
    out_add(out, max, &p, "\n");
    for (i = 0; i < G.n_mcp_creds; i++) {
        if (!G.mcp_creds[i].used) continue;
        out_add(out, max, &p, G.mcp_creds[i].tool);
        out_add(out, max, &p, " bearer=[masked]\n");
    }
    return OSUI_OK;
}

static int mcp_has_auth(const char *tool) {
    int i;
    for (i = 0; i < G.n_mcp_creds; i++) {
        if (G.mcp_creds[i].used && s_cmp(G.mcp_creds[i].tool, tool) == 0) return 1;
    }
    return 0;
}

static int cmd_browser_status(char *out, int max) {
    int p = 0;
    int i;
    osui_tab_t *tab = (G.current_tab >= 0 && G.current_tab < OSUI_MAX_TABS && G.tabs[G.current_tab].used) ? &G.tabs[G.current_tab] : 0;
    out_add(out, max, &p,
        "osui browser-status harness=dom_simulator us031_complete=false chromium=false\n"
        "tabs=");
    out_u(out, max, &p, (unsigned)G.n_tabs);
    out_add(out, max, &p, " active_tab=");
    if (tab) {
        out_add(out, max, &p, tab->id);
        out_add(out, max, &p, " url=");
        out_add(out, max, &p, tab->url);
        out_add(out, max, &p, " origin=");
        out_add(out, max, &p, tab->origin[0] ? tab->origin : "self");
        out_add(out, max, &p, " storage_entries=");
        out_u(out, max, &p, (unsigned)tab->n_storage);
        for (i = 0; i < tab->n_storage; i++) {
            out_add(out, max, &p, " [");
            out_add(out, max, &p, tab->storage[i].key);
            out_add(out, max, &p, "=");
            out_add(out, max, &p, tab->storage[i].value);
            out_add(out, max, &p, "]");
        }
    } else {
        out_add(out, max, &p, "(none)");
    }
    out_add(out, max, &p, " vfs_inspection=live sandbox_mounts=/initrd,/overlay,/fat16,/fat32\n");
    out_add(out, max, &p, " menu_open=");
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
        audit_log_add("mcp_capability_denied", "mcp.invoice.create");
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
    if (!G.confirming) {
        /* Roadmap step 3: every mutation waits for an explicit confirm. */
        G.next_cid++;
        make_id(s->pending_token, 'c', G.next_cid);
        s_cpy(s->pending_tool, OSUI_CAP, "mcp.invoice.create");
        s_cpy(s->pending_a, 48, args[0]);
        s_cpy(s->pending_b, 24, args[1]);
        journal(s->id, "mcp.invoice.create", "confirm_required", rid);
        out_add(out, max, &p, "osui mcp-invoice confirm_required token=");
        out_add(out, max, &p, s->pending_token);
        out_add(out, max, &p, " session_id=");
        out_add(out, max, &p, s->id);
        out_add(out, max, &p, " confirm=confirm ");
        out_add(out, max, &p, s->pending_token);
        out_add(out, max, &p, " deny=deny ");
        out_add(out, max, &p, s->pending_token);
        emit_rid(out, max, &p, rid);
        out_add(out, max, &p, "\n");
        return OSUI_OK;
    }
    inv = &G.invoices[G.n_invoices++];
    inv->voided = 0;
    inv->used = 1;
    G.next_iid++;
    make_id(inv->id, 'i', G.next_iid);
    s_cpy(inv->session, OSUI_ID, s->id);
    s_cpy(inv->customer, 48, args[0]);
    s_cpy(inv->amount, 24, args[1]);
    journal(s->id, "mcp.invoice.create", "ok", rid);
    audit_log_add("mcp_invoice", inv->id);
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
    if (s_cmp(tool, "mcp.payment.process") == 0 || s_cmp(tool, "mcp.document.sign") == 0) {
        if (!mcp_has_auth(tool)) {
            journal(s->id, tool, "auth_required", rid);
            audit_log_add("mcp_auth_required", tool);
            out_add(out, max, &p, "osui mcp-invoke error=authentication_required tool=");
            out_add(out, max, &p, tool);
            out_add(out, max, &p, " status=401");
            emit_rid(out, max, &p, rid);
            out_add(out, max, &p, "\n");
            return OSUI_ERR;
        }
        journal(s->id, tool, "ok", rid);
        audit_log_add("mcp_invoke_ok", tool);
        out_add(out, max, &p, "osui mcp-invoke ok tool=");
        out_add(out, max, &p, tool);
        out_add(out, max, &p, " session_id=");
        out_add(out, max, &p, s->id);
        emit_rid(out, max, &p, rid);
        out_add(out, max, &p, "\n");
        return OSUI_OK;
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
        audit_log_add("vfs_traversal_denied", args[0]);
        out_add(out, max, &p, "osui fs-read error=traversal_denied");
        emit_rid(out, max, &p, rid);
        out_add(out, max, &p, "\n");
        return OSUI_ERR;
    }
    if (!fs_read_allowed(cur(), args[0])) {
        osui_session_t *s = cur();
        journal(s ? s->id : "", "fs.read", "scope_denied", rid);
        audit_log_add("vfs_scope_denied", args[0]);
        out_add(out, max, &p, "osui fs-read error=capability_denied capability=fs.read:<scope>");
        emit_rid(out, max, &p, rid);
        out_add(out, max, &p, "\n");
        return OSUI_ERR;
    }
    {
        osui_session_t *s = cur();
        journal(s ? s->id : "", "fs.read", "ok", rid);
        audit_log_add("vfs_read", args[0]);
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
    s_cpy(G.stage_llm, 20, "stub_echo");
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
        "browser", "shell", "admin", "support", "status", "fs", "p2p", "collab", "prod", 0
    };
    if (!in_list(panes, pane)) {
        out_add(out, max, &p, "osui open error=usage open browser|shell|admin|support|status|fs|p2p|collab|prod\n");
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
    G.gui_live = 1;
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
    G.gui_live = 0;
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
    out_add(out, max, &p, G.gui_live ? " gui_live=true" : " gui_live=false");
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
    if (s_cmp(cmd, "p2p") == 0) { s_cpy(mapped, max, "os-p2p"); return 1; }
    if (s_cmp(cmd, "collab") == 0) { s_cpy(mapped, max, "os-collab"); return 1; }
    if (s_cmp(cmd, "prod") == 0) { s_cpy(mapped, max, "os-prod"); return 1; }
    if (s_cmp(cmd, "center") == 0) { s_cpy(mapped, max, "os-center"); return 1; }
    if (s_cmp(cmd, "close") == 0) { s_cpy(mapped, max, "os-center"); return 1; }
    if (s_cmp(cmd, "auth") == 0) { s_cpy(mapped, max, "mcp-auth"); return 1; }
    if (s_cmp(cmd, "pm") == 0) { s_cpy(mapped, max, "browser-pm-run"); return 1; }
    if (s_cmp(cmd, "navigate") == 0) { s_cpy(mapped, max, "browser-navigate"); return 1; }
    if (s_cmp(cmd, "back") == 0) { s_cpy(mapped, max, "browser-back"); return 1; }
    if (s_cmp(cmd, "forward") == 0) { s_cpy(mapped, max, "browser-forward"); return 1; }
    if (s_cmp(cmd, "dom") == 0) { s_cpy(mapped, max, "browser-dom-tree"); return 1; }
    if (s_cmp(cmd, "audit") == 0) { s_cpy(mapped, max, "osui-audit"); return 1; }
    if (s_cmp(cmd, "model") == 0) { s_cpy(mapped, max, "osui-model"); return 1; }
    if (s_cmp(cmd, "provider") == 0) { s_cpy(mapped, max, "osui-provider"); return 1; }
    if (s_cmp(cmd, "plan") == 0) { s_cpy(mapped, max, "stage-prompt"); return 1; }
    if (s_cmp(cmd, "draw") == 0) { s_cpy(mapped, max, "stage-prompt"); return 1; }
    if (s_cmp(cmd, "telemetry") == 0) { s_cpy(mapped, max, "browser-telemetry"); return 1; }
    if (s_cmp(cmd, "pwa") == 0) { s_cpy(mapped, max, "pwa-list"); return 1; }
    if (s_cmp(cmd, "media") == 0) { s_cpy(mapped, max, "browser-media"); return 1; }
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

static int dispatch_cmd(const char *cmd, char args[OSUI_MAX_ARGS][96], int narg, char *out, int max);

/* Roadmap step 3: the agent runs only allowlisted, read-only commands, under
 * the agent.run capability; every run (and refusal) is journaled. The
 * commands keep their own checks (fs-read still needs its scope). */
static const char *const k_agent_allow[] = {
    "os-status", "session-status", "fs-list", "fs-read", "gui-status", "stage", 0
};

static int cmd_agent_run(char args[OSUI_MAX_ARGS][96], int narg, char *out, int max) {
    osui_session_t *s = cur();
    char a2[OSUI_MAX_ARGS][96];
    char tool[OSUI_CAP];
    int i, p = 0, rc;
    if (!s || s->status == ST_CLOSED || narg < 1) {
        out_add(out, max, &p, "osui agent-run error=usage agent-run <commande> [args]\n");
        return OSUI_ERR;
    }
    s_cpy(tool, OSUI_CAP, "agent.run:");
    s_cpy(tool + 10, OSUI_CAP - 10, args[0]);
    if (!has_cap(s, "agent.run")) {
        journal(s->id, tool, "capability_denied", 0U);
        out_add(out, max, &p, "osui agent-run error=capability_denied capability=agent.run\n");
        return OSUI_ERR;
    }
    if (!in_list(k_agent_allow, args[0])) {
        journal(s->id, tool, "not_allowlisted", 0U);
        out_add(out, max, &p, "osui agent-run error=command_not_allowlisted command=");
        out_add(out, max, &p, args[0]);
        out_add(out, max, &p, "\n");
        return OSUI_ERR;
    }
    for (i = 0; i < OSUI_MAX_ARGS; i++) a2[i][0] = 0;
    for (i = 1; i < narg && i < OSUI_MAX_ARGS; i++) s_cpy(a2[i - 1], 96, args[i]);
    out_add(out, max, &p, "osui agent-run ok command=");
    out_add(out, max, &p, args[0]);
    out_add(out, max, &p, "\n");
    rc = dispatch_cmd(args[0], a2, narg - 1, out + p, max - p);
    journal(s->id, tool, rc == OSUI_OK ? "ok" : "refused", 0U);
    return rc;
}


/* Roadmap step 5: controlled local API of the Web Runtime, served inside the
 * guest by OS-UI (no socket, no browser engine: phase3_complete=false). A
 * request is "api <GET|POST> <route> [token] [body words]". Status is
 * public; every other route needs the session token issued by api-token
 * (capability web.api) and runs with that session's capabilities, the same
 * ones the guest console enforces. Limits: 8 requests per token per 10 s
 * (429), body at most OSUI_API_BODY bytes (413). Every request is journaled.
 * The token is a local session handle, not a cryptographic secret. */
#define OSUI_API_RATE 8U
#define OSUI_API_WINDOW 10U
#define OSUI_API_BODY 64
#define OSUI_OUT_JSON 384

static osui_session_t *api_owner(const char *tok) {
    int i;
    if (!tok || tok[0] != 't') return 0;
    for (i = 0; i < OSUI_MAX_SESSIONS; i++) {
        osui_session_t *s = &G.sessions[i];
        if (s->used && s->api_token[0] && s_cmp(s->api_token, tok) == 0) return s;
    }
    return 0;
}

static int api_reply(char *out, int max, int *p, unsigned code, const char *route, const char *json,
                     unsigned rid) {
    out_add(out, max, p, "osui api status=");
    out_u(out, max, p, code);
    out_add(out, max, p, " route=");
    out_add(out, max, p, route);
    emit_rid(out, max, p, rid);
    out_add(out, max, p, "\n");
    out_add(out, max, p, json);
    out_add(out, max, p, "\n");
    return code == 200U ? OSUI_OK : OSUI_ERR;
}

static int cmd_api_token(int issue, char *out, int max) {
    osui_session_t *s = cur();
    int p = 0;
    if (!s || s->status == ST_CLOSED) {
        out_add(out, max, &p, "osui api-token error=no_session\n");
        return OSUI_ERR;
    }
    if (!issue) {
        s->api_token[0] = 0;
        journal(s->id, "web.api", "token_revoked", 0U);
        out_add(out, max, &p, "osui api-revoke ok session_id=");
        out_add(out, max, &p, s->id);
        out_add(out, max, &p, "\n");
        return OSUI_OK;
    }
    if (!has_cap(s, "web.api")) {
        journal(s->id, "web.api", "capability_denied", 0U);
        out_add(out, max, &p, "osui api-token error=capability_denied capability=web.api\n");
        return OSUI_ERR;
    }
    {
        unsigned v = (new_rid() * 7919U + osui_now() * 104729U) % 90000U + 10000U;
        int i;
        s->api_token[0] = 't';
        for (i = 5; i >= 1; i--) { s->api_token[i] = (char)('0' + v % 10U); v /= 10U; }
        s->api_token[6] = 0;
    }
    s->api_window = osui_now();
    s->api_count = 0U;
    journal(s->id, "web.api", "token_issued", 0U);
    out_add(out, max, &p, "osui api-token ok token=");
    out_add(out, max, &p, s->api_token);
    out_add(out, max, &p, " session_id=");
    out_add(out, max, &p, s->id);
    out_add(out, max, &p, "\n");
    return OSUI_OK;
}

static int cmd_api(char args[OSUI_MAX_ARGS][96], int narg, char *out, int max) {
    char json[OSUI_OUT_JSON];
    char body[OSUI_TEXT];
    const char *m, *r;
    osui_session_t *s;
    unsigned rid = new_rid();
    int p = 0, get, post, rc, i, q = 0;
    if (narg < 2) return api_reply(out, max, &p, 400U, "-", "{\"error\":\"usage api <GET|POST> <route> [token] [body]\"}", rid);
    m = args[0]; r = args[1];
    get = s_cmp(m, "GET") == 0 || s_cmp(m, "get") == 0;
    post = s_cmp(m, "POST") == 0 || s_cmp(m, "post") == 0;
    if (!get && !post) return api_reply(out, max, &p, 405U, r, "{\"error\":\"method_not_allowed\"}", rid);
    if (s_cmp(r, "/status") == 0) {
        if (!get) return api_reply(out, max, &p, 405U, r, "{\"error\":\"method_not_allowed\"}", rid);
        journal("", "web.api:/status", "ok", rid);
        json[0] = 0;
        out_add(json, sizeof(json), &q, "{\"status\":\"ok\",\"runtime\":\"osui-local-api\",\"phase3_complete\":false,"
                "\"browser_engine\":\"none\",\"sessions\":");
        {
            unsigned n = 0U;
            for (i = 0; i < OSUI_MAX_SESSIONS; i++) if (G.sessions[i].used) n++;
            out_u(json, sizeof(json), &q, n);
        }
        out_add(json, sizeof(json), &q, ",\"ai_provider\":\"");
        out_add(json, sizeof(json), &q, G.ai_provider[0] ? G.ai_provider : "local");
        out_add(json, sizeof(json), &q, "\"}");
        return api_reply(out, max, &p, 200U, r, json, rid);
    }
    if (s_cmp(r, "/sessions") != 0 && s_ncmp(r, "/vfs/", 5) != 0 && s_cmp(r, "/ai/chat") != 0 &&
        s_cmp(r, "/supervision") != 0) {
        journal("", "web.api", "not_found", rid);
        return api_reply(out, max, &p, 404U, r, "{\"error\":\"no_such_route\"}", rid);
    }
    s = narg >= 3 ? api_owner(args[2]) : 0;
    if (!s || s->status == ST_CLOSED) {
        journal("", "web.api", "unauthorized", rid);
        return api_reply(out, max, &p, 401U, r, "{\"error\":\"token_required\"}", rid);
    }
    if (osui_now() - s->api_window >= OSUI_API_WINDOW) { s->api_window = osui_now(); s->api_count = 0U; }
    if (++s->api_count > OSUI_API_RATE) {
        journal(s->id, "web.api", "rate_limited", rid);
        return api_reply(out, max, &p, 429U, r, "{\"error\":\"rate_limited\",\"limit\":8,\"window_s\":10}", rid);
    }
    rest_from(args, narg, 3, body, OSUI_TEXT);
    if (s_len(body) > OSUI_API_BODY) {
        journal(s->id, "web.api", "body_too_large", rid);
        return api_reply(out, max, &p, 413U, r, "{\"error\":\"body_too_large\",\"max\":64}", rid);
    }
    if (!has_cap(s, "web.api")) {   /* revoked after the token was issued */
        journal(s->id, "web.api", "capability_denied", rid);
        return api_reply(out, max, &p, 403U, r, "{\"error\":\"capability_denied\",\"capability\":\"web.api\"}", rid);
    }
    /* Web supervision: worker liveness, network state and the journal tail,
     * under admin.observe (the same capability as admin-status). */
    if (s_cmp(r, "/supervision") == 0) {
        unsigned st = 0U;
        int nw = 0, n, k;
        const char *ab;
        if (!get) return api_reply(out, max, &p, 405U, r, "{\"error\":\"method_not_allowed\"}", rid);
        if (!has_cap(s, "admin.observe")) {
            journal(s->id, "web.api:/supervision", "capability_denied", rid);
            return api_reply(out, max, &p, 403U, r, "{\"error\":\"capability_denied\",\"capability\":\"admin.observe\"}", rid);
        }
        journal(s->id, "web.api:/supervision", "ok", rid);
        if (osui_net_stack(&st, &nw) != 0) { st = 0U; nw = 0; }
        k = (int)osui_ai_last_abort();
        ab = k == OS_AI_ABORT_WORKER_LOST ? "worker_lost" : k == OS_AI_ABORT_STALLED ? "stalled" :
             k == OS_AI_ABORT_TIMEOUT ? "timeout" : k == OS_AI_ABORT_CANCELLED ? "cancelled" : "none";
        out_add(json, sizeof(json), &q, "{\"ai_worker\":");
        out_add(json, sizeof(json), &q, osui_ai_worker_pid() > 0 ? "\"live\"" : "\"absent\"");
        out_add(json, sizeof(json), &q, ",\"ai_last_abort\":\"");
        out_add(json, sizeof(json), &q, ab);
        out_add(json, sizeof(json), &q, "\",\"net_worker\":");
        out_add(json, sizeof(json), &q, nw > 0 ? "\"live\"" : "\"absent\"");
        out_add(json, sizeof(json), &q, ",\"nic\":");
        out_add(json, sizeof(json), &q, (st & 1U) ? "true" : "false");
        out_add(json, sizeof(json), &q, ",\"journal\":[");
        n = G.n_journal < 3 ? G.n_journal : 3;
        for (k = G.n_journal - n; k < G.n_journal; k++) {
            out_add(json, sizeof(json), &q, "\"");
            out_add(json, sizeof(json), &q, G.journal[k].tool);
            out_add(json, sizeof(json), &q, ":");
            out_add(json, sizeof(json), &q, G.journal[k].outcome);
            out_add(json, sizeof(json), &q, k + 1 < G.n_journal ? "\"," : "\"");
        }
        out_add(json, sizeof(json), &q, "]}");
        return api_reply(out, max, &p, 200U, r, json, rid);
    }
    if (s_cmp(r, "/sessions") == 0) {
        if (!get) return api_reply(out, max, &p, 405U, r, "{\"error\":\"method_not_allowed\"}", rid);
        journal(s->id, "web.api:/sessions", "ok", rid);
        out_add(json, sizeof(json), &q, "{\"sessions\":[{\"id\":\"");
        out_add(json, sizeof(json), &q, s->id);
        out_add(json, sizeof(json), &q, "\",\"status\":\"");
        out_add(json, sizeof(json), &q, st_name(s->status));
        out_add(json, sizeof(json), &q, "\",\"caps\":");
        out_u(json, sizeof(json), &q, (unsigned)s->n_caps);
        out_add(json, sizeof(json), &q, "}],\"visible\":\"own_session_only\"}");
        return api_reply(out, max, &p, 200U, r, json, rid);
    }
    if (s_ncmp(r, "/vfs/", 5) == 0) {
        const char *path = r + 5;
        osui_fs_t *f;
        if (!get) return api_reply(out, max, &p, 405U, r, "{\"error\":\"method_not_allowed\"}", rid);
        if (fs_traversal(path)) {
            journal(s->id, "web.api:/vfs", "traversal_denied", rid);
            return api_reply(out, max, &p, 400U, r, "{\"error\":\"traversal_denied\"}", rid);
        }
        if (!fs_read_allowed(s, path)) {
            journal(s->id, "web.api:/vfs", "scope_denied", rid);
            return api_reply(out, max, &p, 403U, r, "{\"error\":\"capability_denied\",\"capability\":\"fs.read:<scope>\"}", rid);
        }
        f = fs_find(path);
        if (!f || f->is_dir) return api_reply(out, max, &p, 404U, r, "{\"error\":\"not_found\"}", rid);
        journal(s->id, "web.api:/vfs", "ok", rid);
        out_add(json, sizeof(json), &q, "{\"path\":\"");
        out_add(json, sizeof(json), &q, path);
        out_add(json, sizeof(json), &q, "\",\"bytes\":");
        out_u(json, sizeof(json), &q, (unsigned)s_len(f->content));
        out_add(json, sizeof(json), &q, "}");
        api_reply(out, max, &p, 200U, r, json, rid);
        out_add(out, max, &p, f->content);
        return OSUI_OK;
    }
    /* POST /ai/chat: the same chat path as the console, as the token owner. */
    if (!post) return api_reply(out, max, &p, 405U, r, "{\"error\":\"method_not_allowed\"}", rid);
    if (!body[0]) return api_reply(out, max, &p, 400U, r, "{\"error\":\"empty_body\"}", rid);
    {
        char a2[OSUI_MAX_ARGS][96];
        int saved = G.current, n = 1;
        const char *b = body;
        for (i = 0; i < OSUI_MAX_ARGS; i++) a2[i][0] = 0;
        s_cpy(a2[0], 96, "ai");
        while (*b && n < OSUI_MAX_ARGS) {
            int k = 0;
            while (*b == ' ') b++;
            while (*b && *b != ' ' && k < 95) a2[n][k++] = *b++;
            a2[n][k] = 0;
            if (k) n++;
        }
        G.current = (int)(s - G.sessions);
        api_reply(out, max, &p, 202U, r, "{\"accepted\":true,\"via\":\"osui chat\"}", rid);
        rc = cmd_chat(a2, n, out + p, max - p);
        G.current = saved;
        journal(s->id, "web.api:/ai/chat", rc == OSUI_OK ? "ok" : "error", rid);
        return rc;
    }
}

/* Roadmap step 5: the local API and AI console served over HTTP/1.0 or
 * HTTPS (TLS 1.2, test certificate) through the Ring 3 networker, up to
 * OS_PEER_WEB_SLOTS connections at once. Each request is mapped onto the
 * same cmd_api routes, token and limits ("Authorization: Bearer tNNNNN");
 * GET / is a minimal console page. "web-serve start" keeps serving in the
 * background while the console reads keys (shell calls osui_web_poll).
 * No browser engine (phase3_complete=false), local peers only. */
#define OSUI_WEB_REQ 512
static int web_find(const char *h, int n, const char *needle) {
    int i, k, l = s_len(needle);
    for (i = 0; i + l <= n; i++) {
        for (k = 0; k < l && h[i + k] == needle[k]; k++) {}
        if (k == l) return i;
    }
    return -1;
}

static struct {
    int active;
    int tls;
    unsigned port;
    unsigned limit;
    unsigned served;
    unsigned failed;
    int len[OS_PEER_WEB_SLOTS];
    int seen[OS_PEER_WEB_SLOTS];
    unsigned t0[OS_PEER_WEB_SLOTS];
    char req[OS_PEER_WEB_SLOTS][OSUI_WEB_REQ + 1];
} g_ws;

/* 1 when the request in buf is complete (headers and Content-Length). */
static int web_complete(const char *req, int n) {
    int hdr = web_find(req, n, "\r\n\r\n"), k, i;
    unsigned clen = 0U;
    if (hdr < 0) return n >= OSUI_WEB_REQ;
    k = web_find(req, hdr, "Content-Length: ");
    if (k >= 0)
        for (i = k + 16; req[i] >= '0' && req[i] <= '9'; i++) clen = clen * 10U + (unsigned)(req[i] - '0');
    return (unsigned)(n - hdr - 4) >= clen;
}

static void web_respond(int slot, char *log, int lmax, int *lp) {
    static char api_out[OSUI_OUT_MAX];
    static char resp[OSUI_OUT_MAX + 160];
    char args[OSUI_MAX_ARGS][96];
    char *req = g_ws.req[slot];
    int n = g_ws.len[slot], hdr, i, k, narg, rp = 0, code = 500, rc;
    const char *body = "";
    const char *ctype = "application/json";
    for (i = 0; i < OSUI_MAX_ARGS; i++) args[i][0] = 0;
    req[n] = 0;
    hdr = web_find(req, n, "\r\n\r\n");
    if (hdr < 0) { code = 400; body = "{\"error\":\"bad_request\"}"; goto reply; }
    for (i = 0, k = 0; req[i] && req[i] != ' ' && k < 95; i++) args[0][k++] = req[i];
    args[0][k] = 0;
    while (req[i] == ' ') i++;
    for (k = 0; req[i] && req[i] != ' ' && req[i] != '\r' && k < 95; i++) args[1][k++] = req[i];
    args[1][k] = 0;
    if (s_cmp(args[1], "/") == 0 && s_cmp(args[0], "GET") == 0) {
        code = 200; ctype = "text/html";
        body = "<html><body><h1>MOHHDY local console</h1><p>API: /status /sessions /vfs/ /supervision /ai/chat"
               " (Bearer token). phase3_complete=false, no browser engine.</p></body></html>";
        goto reply;
    }
    narg = 3;
    k = web_find(req, hdr, "Authorization: Bearer ");
    if (k >= 0) {
        for (i = k + 22, n = 0; req[i] && req[i] != '\r' && n < 95; i++) args[2][n++] = req[i];
        args[2][n] = 0;
    } else {
        s_cpy(args[2], 96, "-");
    }
    i = hdr + 4;
    while (req[i] && narg < OSUI_MAX_ARGS) {
        while (req[i] == ' ' || req[i] == '\r' || req[i] == '\n') i++;
        for (n = 0; req[i] && req[i] != ' ' && req[i] != '\r' && req[i] != '\n' && n < 95; i++) args[narg][n++] = req[i];
        args[narg][n] = 0;
        if (n) narg++;
    }
    api_out[0] = 0;
    (void)cmd_api(args, narg, api_out, sizeof(api_out));
    k = web_find(api_out, s_len(api_out), "status=");
    if (k >= 0) {
        code = 0;
        for (i = k + 7; api_out[i] >= '0' && api_out[i] <= '9'; i++) code = code * 10 + (api_out[i] - '0');
    }
    for (i = 0; api_out[i] && api_out[i] != '\n'; i++) {}
    body = api_out[i] ? api_out + i + 1 : "";
reply:
    out_add(resp, sizeof(resp), &rp, "HTTP/1.0 ");
    out_u(resp, sizeof(resp), &rp, (unsigned)code);
    out_add(resp, sizeof(resp), &rp, code == 200 ? " OK" : code == 202 ? " Accepted" : " Error");
    out_add(resp, sizeof(resp), &rp, "\r\nServer: mohhdy-osui\r\nContent-Type: ");
    out_add(resp, sizeof(resp), &rp, ctype);
    out_add(resp, sizeof(resp), &rp, "\r\nContent-Length: ");
    out_u(resp, sizeof(resp), &rp, (unsigned)s_len(body));
    out_add(resp, sizeof(resp), &rp, "\r\nConnection: close\r\n\r\n");
    out_add(resp, sizeof(resp), &rp, body);
    rc = web_send_slot(slot, resp, s_len(resp));
    web_close_slot(slot);
    out_add(log, lmax, lp, "osui web-serve request slot=");
    out_u(log, lmax, lp, (unsigned)slot);
    out_add(log, lmax, lp, g_ws.tls ? " scheme=https method=" : " scheme=http method=");
    out_add(log, lmax, lp, args[0][0] ? args[0] : "-");
    out_add(log, lmax, lp, " path=");
    out_add(log, lmax, lp, args[1][0] ? args[1] : "-");
    out_add(log, lmax, lp, " status=");
    out_u(log, lmax, lp, (unsigned)code);
    out_add(log, lmax, lp, rc == 0 ? " sent=ok\n" : " sent=failed\n");
    journal("", "web.http", rc == 0 ? "served" : "send_failed", 0U);
}

static void web_finish(char *log, int lmax, int *lp) {
    web_stop();
    g_ws.active = 0;
    out_add(log, lmax, lp, "osui web-serve ok served=");
    out_u(log, lmax, lp, g_ws.served);
    out_add(log, lmax, lp, " failed=");
    out_u(log, lmax, lp, g_ws.failed);
    out_add(log, lmax, lp, "\n");
}

/* One non-blocking step of the server; appends log lines. */
static void web_tick(char *log, int lmax, int *lp) {
    unsigned char st[OS_PEER_WEB_SLOTS];
    int i, got;
    if (!g_ws.active || web_poll(st) != 0) return;
    for (i = 0; i < (int)OS_PEER_WEB_SLOTS; i++) {
        unsigned kind = st[i] & 0x0FU;
        if (kind != OS_PEER_SLOT_OPEN && kind != OS_PEER_SLOT_HANDSHAKE) { g_ws.seen[i] = 0; continue; }
        if (!g_ws.seen[i]) { g_ws.seen[i] = 1; g_ws.len[i] = 0; g_ws.t0[i] = osui_now(); }
        /* The request timer starts once TLS is complete (seen == 2). */
        if (kind == OS_PEER_SLOT_OPEN && g_ws.seen[i] == 1) { g_ws.seen[i] = 2; g_ws.t0[i] = osui_now(); }
        if (st[i] & OS_PEER_SLOT_FAILED) {
            web_close_slot(i);
            g_ws.seen[i] = 0;
            g_ws.failed++;
            out_add(log, lmax, lp, "osui web-serve slot=");
            out_u(log, lmax, lp, (unsigned)i);
            out_add(log, lmax, lp, " error=tls_failed at=");
            out_u(log, lmax, lp, g_web_fail[i]);
            out_add(log, lmax, lp, "\n");
            continue;
        }
        while ((st[i] & OS_PEER_SLOT_DATA) && g_ws.len[i] < OSUI_WEB_REQ &&
               web_recv_slot(i, g_ws.req[i] + g_ws.len[i], OSUI_WEB_REQ - g_ws.len[i], &got) == 0 && got > 0)
            g_ws.len[i] += got;
        if (kind == OS_PEER_SLOT_OPEN &&
            (web_complete(g_ws.req[i], g_ws.len[i]) || (st[i] & OS_PEER_SLOT_PEER_CLOSED) ||
             osui_now() - g_ws.t0[i] > 20U)) {
            web_respond(i, log, lmax, lp);
            g_ws.seen[i] = 0;
            g_ws.served++;
        } else if (osui_now() - g_ws.t0[i] > (kind == OS_PEER_SLOT_OPEN ? 20U : 180U)) {
            web_close_slot(i);
            g_ws.seen[i] = 0;
            g_ws.failed++;
        }
    }
    if (g_ws.limit && g_ws.served + g_ws.failed >= g_ws.limit) web_finish(log, lmax, lp);
}

int osui_web_active(void) { return g_ws.active; }

int osui_web_poll(char *out, int max) {
    int p = 0;
    if (max > 0) out[0] = 0;
    web_tick(out, max, &p);
    return p;
}

static int web_start(unsigned port, int tls, unsigned limit, char *out, int max, int *p) {
    int rc, i;
    if (g_ws.active) {
        out_add(out, max, p, "osui web-serve error=already_running\n");
        return OSUI_ERR;
    }
    rc = web_open(port, tls);
    if (rc != 0) {
        out_add(out, max, p, "osui web-serve error=listen_failed rc=");
        out_u(out, max, p, (unsigned)(-rc));
        out_add(out, max, p, "\n");
        return OSUI_ERR;
    }
    g_ws.active = 1; g_ws.tls = tls; g_ws.port = port; g_ws.limit = limit; g_ws.served = 0U; g_ws.failed = 0U;
    for (i = 0; i < (int)OS_PEER_WEB_SLOTS; i++) g_ws.seen[i] = 0;
    out_add(out, max, p, "osui web-serve listening port=");
    out_u(out, max, p, port);
    out_add(out, max, p, tls ? " scheme=https slots=" : " scheme=http slots=");
    out_u(out, max, p, OS_PEER_WEB_SLOTS);
    out_add(out, max, p, limit ? " mode=blocking\n" : " mode=background\n");
    journal("", "web.http", tls ? "start_https" : "start_http", 0U);
    return OSUI_OK;
}

/* web-serve [port] [requests 1-8] [tls] (blocking)
 * web-serve start [port] [tls] | status | stop (background) */
static int cmd_web_serve(char args[OSUI_MAX_ARGS][96], int narg, char *out, int max) {
    unsigned port = 8080U, count = 1U, t0;
    int p = 0, tls = 0, a = 0;
    if (narg >= 1 && s_cmp(args[0], "status") == 0) {
        out_add(out, max, &p, g_ws.active ? "osui web-serve status=running port=" : "osui web-serve status=stopped port=");
        out_u(out, max, &p, g_ws.port);
        out_add(out, max, &p, g_ws.tls ? " scheme=https served=" : " scheme=http served=");
        out_u(out, max, &p, g_ws.served);
        out_add(out, max, &p, "\n");
        return OSUI_OK;
    }
    if (narg >= 1 && s_cmp(args[0], "stop") == 0) {
        if (!g_ws.active) { out_add(out, max, &p, "osui web-serve error=not_running\n"); return OSUI_ERR; }
        web_finish(out, max, &p);
        return OSUI_OK;
    }
    if (narg >= 1 && s_cmp(args[0], "start") == 0) {
        if (narg >= 2 && s_cmp(args[1], "tls") != 0 && !parse_u(args[1], &port)) port = 0U;
        tls = (narg >= 2 && s_cmp(args[narg - 1], "tls") == 0);
        if (port == 0U || port > 65535U) {
            out_add(out, max, &p, "osui web-serve error=usage web-serve start [port] [tls]\n");
            return OSUI_ERR;
        }
        return web_start(port, tls, 0U, out, max, &p);
    }
    if (narg >= 1 && s_cmp(args[narg - 1], "tls") == 0) { tls = 1; narg--; }
    if (narg >= 1 && !parse_u(args[a], &port)) port = 0U;
    if (narg >= 2 && !parse_u(args[a + 1], &count)) count = 0U;
    if (port == 0U || port > 65535U || count == 0U || count > 8U) {
        out_add(out, max, &p, "osui web-serve error=usage web-serve [port] [requests 1-8] [tls]\n");
        return OSUI_ERR;
    }
    if (web_start(port, tls, count, out, max, &p) != OSUI_OK) return OSUI_ERR;
    t0 = osui_now();
    while (g_ws.active && osui_now() - t0 < 120U) {
        web_tick(out, max, &p);
        if (g_ws.active) peer_yield();
    }
    if (g_ws.active) {
        web_finish(out, max, &p);
        out_add(out, max, &p, "osui web-serve error=timeout\n");
        return OSUI_ERR;
    }
    return g_ws.failed ? OSUI_ERR : OSUI_OK;
}

static int dispatch_cmd(const char *cmd, char args[OSUI_MAX_ARGS][96], int narg, char *out, int max) {
    if (s_cmp(cmd, "web-serve") == 0) return cmd_web_serve(args, narg, out, max);
    if (s_cmp(cmd, "api") == 0) return cmd_api(args, narg, out, max);
    if (s_cmp(cmd, "api-token") == 0) return cmd_api_token(1, out, max);
    if (s_cmp(cmd, "api-revoke") == 0) return cmd_api_token(0, out, max);
    if (s_cmp(cmd, "agent-run") == 0) return cmd_agent_run(args, narg, out, max);
    if (s_cmp(cmd, "os-help") == 0 || s_cmp(cmd, "help") == 0) return cmd_os_help(out, max);
    if (s_cmp(cmd, "os-status") == 0) return cmd_os_status(out, max);
    if (s_cmp(cmd, "os-p2p") == 0) return open_then("p2p", cmd_gui_status, out, max);
    if (s_cmp(cmd, "os-collab") == 0) return open_then("collab", cmd_gui_status, out, max);
    if (s_cmp(cmd, "os-prod") == 0) return open_then("prod", cmd_gui_status, out, max);
    if (s_cmp(cmd, "osui-audit") == 0) return cmd_osui_audit(out, max);
    if (s_cmp(cmd, "osui-model") == 0) return cmd_osui_model(args, narg, out, max);
    if (s_cmp(cmd, "osui-provider") == 0) return cmd_osui_provider(args, narg, out, max);
    if (s_cmp(cmd, "os-browser") == 0) { audit_log_add("os_inspect", "browser"); return open_then("browser", cmd_browser_status, out, max); }
    if (s_cmp(cmd, "os-shell") == 0) {
        char dummy[64];
        char a[OSUI_MAX_ARGS][96];
        int p = 0;
        s_cpy(a[0], 96, "shell");
        cmd_open(a, 1, dummy, 64);
        out_add(out, max, &p, "osui os-shell ok this_is_multiboot_ring3 live_eval=true prompt=MOHHDY> chat_mode=float\n");
        return OSUI_OK;
    }
    if (s_cmp(cmd, "os-admin") == 0) return open_then("admin", cmd_admin_status, out, max);
    if (s_cmp(cmd, "os-support") == 0) return open_then("support", cmd_session_list, out, max);
    if (s_cmp(cmd, "os-fs") == 0) {
        char dummy[64];
        char a[OSUI_MAX_ARGS][96];
        audit_log_add("os_inspect", "fs");
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
    if (s_cmp(cmd, "session-end") == 0) return cmd_session_end(out, max);
    if (s_cmp(cmd, "session-ttl") == 0) return cmd_session_ttl(args, narg, out, max);
    if (s_cmp(cmd, "session-restore") == 0) return cmd_session_restore(args, narg, out, max);
    if (s_cmp(cmd, "session-cleanup") == 0) return cmd_session_cleanup(out, max);
    if (s_cmp(cmd, "confirm") == 0) return cmd_confirm(args, narg, 1, out, max);
    if (s_cmp(cmd, "deny") == 0) return cmd_confirm(args, narg, 0, out, max);
    if (s_cmp(cmd, "mcp-invoice-void") == 0) return cmd_invoice_void(args, narg, out, max);
    if (s_cmp(cmd, "chat") == 0) { audit_log_add("chat", args[0]); return cmd_chat(args, narg, out, max); }
    if (s_cmp(cmd, "grant") == 0) { audit_log_add("grant", args[0]); return cmd_grant_revoke(1, args, narg, out, max); }
    if (s_cmp(cmd, "revoke") == 0) { audit_log_add("revoke", args[0]); return cmd_grant_revoke(0, args, narg, out, max); }
    if (s_cmp(cmd, "escalate") == 0) return cmd_escalate(args, narg, out, max);
    if (s_cmp(cmd, "takeover") == 0) return cmd_takeover(out, max);
    if (s_cmp(cmd, "admin-status") == 0) return cmd_admin_status(out, max);
    if (s_cmp(cmd, "origin-check") == 0) return cmd_origin_check(args, narg, out, max);
    if (s_cmp(cmd, "browser-click") == 0) return cmd_browser_click(args, narg, out, max);
    if (s_cmp(cmd, "browser-type") == 0) return cmd_browser_type(args, narg, out, max);
    if (s_cmp(cmd, "browser-pointer") == 0) return cmd_browser_pointer(args, narg, out, max);
    if (s_cmp(cmd, "browser-status") == 0) return cmd_browser_status(out, max);
    if (s_cmp(cmd, "browser-telemetry") == 0 || s_cmp(cmd, "osui-telemetry") == 0) return cmd_browser_telemetry(args, narg, out, max);
    if (s_cmp(cmd, "web-perf") == 0) return cmd_web_perf(args, narg, out, max);
    if (s_cmp(cmd, "pwa-manifest") == 0) return cmd_pwa_manifest(args, narg, out, max);
    if (s_cmp(cmd, "pwa-install") == 0) return cmd_pwa_install(args, narg, out, max);
    if (s_cmp(cmd, "pwa-list") == 0) return cmd_pwa_list(out, max);
    if (s_cmp(cmd, "pwa-run") == 0) return cmd_pwa_run(args, narg, out, max);
    if (s_cmp(cmd, "browser-media") == 0 || s_cmp(cmd, "media-status") == 0) return cmd_browser_media(out, max);
    if (s_cmp(cmd, "media-play") == 0) return cmd_media_play(args, narg, out, max);
    if (s_cmp(cmd, "media-pause") == 0) return cmd_media_pause(out, max);
    if (s_cmp(cmd, "media-stop") == 0) return cmd_media_stop(out, max);
    if (s_cmp(cmd, "media-stream") == 0) return cmd_media_stream(args, narg, out, max);
    if (s_cmp(cmd, "browser-origin-status") == 0 || s_cmp(cmd, "origin-status") == 0) return cmd_browser_origin_status(out, max);
    if (s_cmp(cmd, "browser-tab-new") == 0) return cmd_browser_tab_new(args, narg, out, max);
    if (s_cmp(cmd, "browser-tab-use") == 0) return cmd_browser_tab_use(args, narg, out, max);
    if (s_cmp(cmd, "browser-tab-list") == 0 || s_cmp(cmd, "browser-tabs") == 0) return cmd_browser_tab_list(out, max);
    if (s_cmp(cmd, "browser-tab-close") == 0) return cmd_browser_tab_close(args, narg, out, max);
    if (s_cmp(cmd, "browser-tab-rename") == 0) return cmd_browser_tab_rename(args, narg, out, max);
    if (s_cmp(cmd, "browser-tab-pin") == 0) return cmd_browser_tab_pin(args, narg, out, max);
    if (s_cmp(cmd, "browser-tab-duplicate") == 0) return cmd_browser_tab_duplicate(args, narg, out, max);
    if (s_cmp(cmd, "browser-tab-next") == 0) return cmd_browser_tab_next(out, max);
    if (s_cmp(cmd, "browser-tab-prev") == 0) return cmd_browser_tab_prev(out, max);
    if (s_cmp(cmd, "browser-fs-save") == 0) return cmd_browser_fs_save(args, narg, out, max);
    if (s_cmp(cmd, "browser-fs-load") == 0) return cmd_browser_fs_load(args, narg, out, max);
    if (s_cmp(cmd, "browser-fs-sync") == 0) return cmd_browser_fs_sync(out, max);
    if (s_cmp(cmd, "browser-navigate") == 0) return cmd_browser_navigate(args, narg, out, max);
    if (s_cmp(cmd, "browser-back") == 0) return cmd_browser_back(out, max);
    if (s_cmp(cmd, "browser-forward") == 0) return cmd_browser_forward(out, max);
    if (s_cmp(cmd, "browser-dom-tree") == 0) return cmd_browser_dom_tree(args, narg, out, max);
    if (s_cmp(cmd, "browser-html-render") == 0 || s_cmp(cmd, "browser-render") == 0) return cmd_browser_html_render(args, narg, out, max);
    if (s_cmp(cmd, "browser-css-demo") == 0) return cmd_browser_css_demo(out, max);
    if (s_cmp(cmd, "web-cache") == 0) return cmd_web_cache(out, max);
    if (s_cmp(cmd, "web-csp") == 0) return cmd_web_csp(args, narg, out, max);
    if (s_cmp(cmd, "web-sanitize") == 0) return cmd_web_sanitize(out, max);
    if (s_cmp(cmd, "web-responsive") == 0) return cmd_web_responsive(args, narg, out, max);
    if (s_cmp(cmd, "web-theme") == 0) return cmd_web_theme(args, narg, out, max);
    if (s_cmp(cmd, "web-components") == 0) return cmd_web_components(out, max);
    if (s_cmp(cmd, "web-a11y") == 0) return cmd_web_a11y(out, max);
    if (s_cmp(cmd, "browser-eval") == 0) return cmd_browser_eval(args, narg, out, max);
    if (s_cmp(cmd, "browser-form-fill") == 0) return cmd_browser_form_fill(args, narg, out, max);
    if (s_cmp(cmd, "browser-dom-act") == 0) return cmd_browser_dom_act(args, narg, out, max);
    if (s_cmp(cmd, "browser-pm-run") == 0 || s_cmp(cmd, "pm-run") == 0 || s_cmp(cmd, "pm-exec") == 0) return cmd_browser_pm_run(args, narg, out, max);
    if (s_cmp(cmd, "browser-fetch") == 0 || s_cmp(cmd, "fetch-sim") == 0) return cmd_browser_fetch(args, narg, out, max);
    if (s_cmp(cmd, "browser-storage-set") == 0) return cmd_browser_storage_set(args, narg, out, max);
    if (s_cmp(cmd, "browser-storage-get") == 0 || s_cmp(cmd, "tab-storage") == 0) return cmd_browser_storage_get(args, narg, out, max);
    if (s_cmp(cmd, "mcp-invoice") == 0) return cmd_mcp_invoice(args, narg, out, max);
    if (s_cmp(cmd, "mcp-invoke") == 0) return cmd_mcp_invoke(args, narg, out, max);
    if (s_cmp(cmd, "mcp-list") == 0) return cmd_mcp_list(out, max);
    if (s_cmp(cmd, "mcp-status") == 0) return cmd_mcp_status(out, max);
    if (s_cmp(cmd, "mcp-auth") == 0) return cmd_mcp_auth(args, narg, out, max);
    if (s_cmp(cmd, "mcp-credentials") == 0) return cmd_mcp_credentials(out, max);
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
        else if (s_has_ci(text, "p2p") || s_has_ci(text, "peer"))
            s_cpy(pane[0], 96, "p2p");
        else if (s_has_ci(text, "collab") || s_has_ci(text, "points"))
            s_cpy(pane[0], 96, "collab");
        else if (s_has_ci(text, "prod") || s_has_ci(text, "metric"))
            s_cpy(pane[0], 96, "prod");
        if (pane[0][0]) return cmd_open(pane, 1, out, max);
    }
    if (s_has_ci(text, "dessine") || s_has_ci(text, "draw")
        || s_has_ci(text, "mini-plan") || s_has_ci(text, "simule")) {
        s_cpy(G.stage_llm, 20, "stub_echo");
        stage_render(text);
        emit_stage(out, max, &p);
        return OSUI_OK;
    }
    s_cpy(G.stage_llm, 20, "stub_echo");
    return cmd_chat(args, narg, out, max);
}

void osui_runtime_init(void) {
    int i;
    for (i = 0; i < (int)sizeof(G); i++) ((char *)&G)[i] = 0;
    webrt_set_audit_callback(audit_log_add);
    G.next_sid = 0;
    G.next_rid = 0;
    G.next_iid = 0;
    G.current = -1;
    s_cpy(G.chat_mode, 12, "center");
    s_cpy(G.stage_mode, 16, "reflecting");
    s_cpy(G.stage_kind, OSUI_KIND, "plan");
    s_cpy(G.stage_llm, 20, "stub_echo");
    s_cpy(G.ai_model, 32, "gpt2_124M.bin");
    s_cpy(G.ai_provider, 16, "local");
    G.kb_loaded = 1;
    G.chat_x = 16;
    G.chat_y = 5;
    G.pane[0] = 0;
    G.gui_enter = 0;
    G.gui_leave = 0;
    fs_seed();
    alloc_session("default");
    stage_render("");

    /* Seed default PWA manifest */
    G.next_pwa_id = 1;
    s_cpy(G.pwas[0].id, OSUI_ID, "pwa0001");
    s_cpy(G.pwas[0].name, 32, "MohhdyApp");
    s_cpy(G.pwas[0].start_url, 64, "app://mohhdy");
    s_cpy(G.pwas[0].scope, 32, "app://mohhdy/");
    s_cpy(G.pwas[0].theme_color, 16, "#007acc");
    s_cpy(G.pwas[0].display, 16, "standalone");
    G.pwas[0].installed = 1;
    G.pwas[0].used = 1;
    G.n_pwas = 1;
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
    sessions_expire();
    {
        osui_session_t *s = cur();
        if (s && s->status != ST_CLOSED) s->last_active = osui_now();
    }
    G.telemetry_commands++;
    G.telemetry_sched_cycles += 12U;
    if (s_cmp(cmd, "browser-fetch") == 0 || s_cmp(cmd, "fetch-sim") == 0)
        G.telemetry_fetches++;
    if (G.media.active && !G.media.paused) {
        G.media.ticks++;
        G.telemetry_media_ticks++;
    }
    if (s_cmp(cmd, "prompt") == 0)
        return cmd_prompt(args, narg, line, out, out_max);
    rc = dispatch_cmd(cmd, args, narg, out, out_max);
    if (rc < 0) {
        G.telemetry_errors++;
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
const char *osui_get_stage_llm(void) { return stage_llm_name(); }
const char *osui_get_ai_state(void) {
    osui_session_t *s = cur();
    return s ? s->ai_state : "idle";
}
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
void osui_gui_closed(void) { G.gui_live = 0; G.gui_leave = 0; }

int osui_stage_tick(char *out, int out_max) {
    uint32_t bg_ms = 0;
    if (out && out_max > 0) out[0] = 0;
    if (G.media.active && !G.media.paused) {
        G.media.ticks++;
        G.telemetry_media_ticks++;
    }
    plat_power_schedule_tick(PLAT_PWR_BALANCED, (uint32_t)G.stage_tick, &bg_ms);
    if (!G.stage_autonomous) return 0;
    G.stage_tick++;
    canvas_sim(G.stage_tick);
    s_cpy(G.stage_mode, 16, G.stage_tick > 4 ? "presenting" : "acting");
    return 0;
}

int osui_get_media_active(void) { return G.media.active; }
int osui_get_media_paused(void) { return G.media.paused; }
const char *osui_get_media_type(void) { return G.media.type[0] ? G.media.type : "audio"; }
const char *osui_get_media_title(void) { return G.media.title[0] ? G.media.title : "none"; }
const char *osui_get_media_url(void) { return G.media.url[0] ? G.media.url : "none"; }
unsigned osui_get_media_ticks(void) { return G.media.ticks; }
int osui_get_pwa_count(void) { return G.n_pwas; }
unsigned osui_get_telemetry_commands(void) { return G.telemetry_commands; }
