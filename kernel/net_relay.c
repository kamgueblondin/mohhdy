#include "net_relay.h"

static uint32_t g_state;
static int32_t g_owner;
static int32_t g_worker;
static uint32_t g_op;
static uint32_t g_job_id;
static uint32_t g_next_job;
static uint32_t g_started;
static uint32_t g_polls;
static int32_t g_result;
static uint32_t g_out_length;
static uint8_t g_out[OS_NET_RELAY_MAX_OUT];
static os_net_relay_status_t g_stats;

void net_relay_init(void) {
    uint32_t i;
    uint8_t* s = (uint8_t*)&g_stats;
    g_state = NET_RELAY_FREE;
    g_owner = 0;
    g_worker = 0;
    g_op = 0U;
    g_job_id = 0U;
    g_next_job = 1U;
    g_started = 0U;
    g_result = 0;
    g_out_length = 0U;
    for (i = 0U; i < sizeof(g_stats); i++) s[i] = 0U;
}

int net_relay_supported(uint32_t syscall_number) {
    return syscall_number >= SYS_SOCKET_OPEN && syscall_number <= SYS_SOCKET_ACCEPT_ACK;
}

uint32_t net_relay_state_for(int32_t pid) {
    if (g_state == NET_RELAY_FREE || pid <= 0 || pid != g_owner) return NET_RELAY_FREE;
    return g_state;
}

int32_t net_relay_owner(void) { return g_state == NET_RELAY_FREE ? 0 : g_owner; }
int32_t net_relay_worker(void) { return g_state == NET_RELAY_FREE ? 0 : g_worker; }

int32_t net_relay_begin(int32_t pid, int32_t worker_pid, uint32_t op, uint32_t now) {
    if (g_state != NET_RELAY_FREE || pid <= 0 || worker_pid <= 0 || pid == worker_pid ||
        !net_relay_supported(op)) return -1;
    g_state = NET_RELAY_SENT;
    g_owner = pid;
    g_worker = worker_pid;
    g_op = op;
    if (g_next_job == 0U) g_next_job = 1U;
    g_job_id = g_next_job++;
    if (g_next_job == 0U || g_next_job > 0x7FFFFFFFU) g_next_job = 1U;
    g_started = now;
    g_polls = 0U;
    g_result = 0;
    g_out_length = 0U;
    g_stats.forwarded++;
    return (int32_t)g_job_id;
}

void net_relay_cancel(void) {
    if (g_state == NET_RELAY_SENT && g_stats.forwarded > 0U) g_stats.forwarded--;
    g_state = NET_RELAY_FREE;
    g_owner = 0;
}

int net_relay_complete(int32_t worker_pid, uint32_t job_id, int32_t result,
                       const uint8_t* out, uint32_t out_length) {
    uint32_t i;
    if (g_state != NET_RELAY_SENT || worker_pid != g_worker || job_id != g_job_id ||
        out_length > OS_NET_RELAY_MAX_OUT || (out_length > 0U && !out)) {
        g_stats.stale++;
        return -1;
    }
    for (i = 0U; i < out_length; i++) g_out[i] = out[i];
    g_out_length = out_length;
    g_result = result;
    g_state = NET_RELAY_DONE;
    return 0;
}

void net_relay_note_poll(void) {
    if (g_state == NET_RELAY_SENT && g_polls < 0xFFFFFFFFU) g_polls++;
}

/* Both conditions: enough time, and enough caller turns that a runnable
 * worker was scheduled in between (the shell can starve every task while
 * it waits for a key, which must not look like a dead worker). */
int net_relay_expired(uint32_t now) {
    return g_state == NET_RELAY_SENT && g_polls >= NET_RELAY_TIMEOUT_POLLS &&
           (uint32_t)(now - g_started) > NET_RELAY_TIMEOUT_TICKS;
}

int32_t net_relay_take(int32_t pid, uint32_t* op, uint8_t* out, uint32_t capacity,
                       uint32_t* out_length) {
    uint32_t i, n;
    int32_t result;
    if (g_state != NET_RELAY_DONE || pid != g_owner) return OS_NET_RELAY_ABORTED;
    n = g_out_length < capacity ? g_out_length : capacity;
    for (i = 0U; out && i < n; i++) out[i] = g_out[i];
    if (out_length) *out_length = out ? n : 0U;
    if (op) *op = g_op;
    result = g_result;
    if (g_job_id != 0U) g_stats.completed++; /* a worker reply, not a failure */
    g_state = NET_RELAY_FREE;
    g_owner = 0;
    return result;
}

void net_relay_fail(int32_t error) {
    if (g_state != NET_RELAY_SENT) return;
    if (error == OS_NET_RELAY_TIMEOUT) g_stats.timeouts++;
    else g_stats.aborted++;
    g_job_id = 0U; /* a late reply is stale */
    g_result = error;
    g_out_length = 0U;
    g_state = NET_RELAY_DONE;
}

void net_relay_drop_owner(void) {
    g_state = NET_RELAY_FREE;
    g_owner = 0;
    g_job_id = 0U;
}

void net_relay_note_denied(void) { g_stats.denied++; }

void net_relay_fill_status(os_net_relay_status_t* out, int32_t live_worker) {
    if (!out) return;
    *out = g_stats;
    out->pending = g_state == NET_RELAY_SENT ? 1U : 0U;
    out->worker_pid = live_worker > 0 ? live_worker : 0;
}
