#include "net_nic_owner.h"

/* Tranche 5 suite: see net_nic_owner.h. */

static int32_t g_owner;
static uint32_t g_pending_irq;
static os_net_nic_status_t g_st;

void nic_owner_init(void) {
    uint32_t i;
    uint8_t* p = (uint8_t*)&g_st;
    for (i = 0U; i < sizeof(g_st); i++) p[i] = 0U;
    g_owner = 0;
    g_pending_irq = 0U;
}

int nic_owner_claim(int32_t pid, int32_t live_worker, int nic_present) {
    if (pid <= 0 || pid != live_worker) return OS_NET_WORKER_REQUIRED;
    if (!nic_present) return OS_NET_NIC_ABSENT;
    if (g_owner == pid) return 0; /* idempotent */
    if (g_owner != 0) return OS_NET_WORKER_REQUIRED; /* stale owner not reclaimed yet */
    g_owner = pid;
    g_pending_irq = 0U;
    g_st.claims++;
    return 0;
}

int32_t nic_owner_pid(void) { return g_owner; }

int nic_owner_ports_open(int32_t task_pid, int32_t live_worker) {
    return g_owner > 0 && task_pid == g_owner && g_owner == live_worker;
}

int nic_owner_kernel_may_touch(void) { return g_owner == 0; }

int nic_owner_drop_if_gone(int32_t live_worker) {
    if (g_owner == 0 || g_owner == live_worker) return 0;
    g_owner = 0;
    g_pending_irq = 0U;
    return 1;
}

int nic_owner_irq(void) {
    if (g_owner == 0) return 0;
    g_pending_irq++;
    g_st.irq_forwarded++;
    return 1;
}

uint32_t nic_owner_irq_take(int32_t pid) {
    uint32_t n;
    if (pid <= 0 || pid != g_owner) return 0U;
    n = g_pending_irq;
    g_pending_irq = 0U;
    return n;
}

void nic_owner_note_kernel_refused(void) { g_st.kernel_refused++; }
void nic_owner_note_kernel_gated(void) { g_st.kernel_gated++; }
void nic_owner_note_reclaim(void) { g_st.reclaims++; }

void nic_owner_note_pump(uint32_t frames_out, uint32_t frame_in, uint32_t tx_ok, uint32_t tx_failed) {
    g_st.pumps++;
    g_st.frames_out += frames_out;
    g_st.frames_in += frame_in;
    g_st.worker_tx_ok += tx_ok;
    g_st.worker_tx_failed += tx_failed;
}

void nic_owner_fill_status(os_net_nic_status_t* out) {
    if (!out) return;
    *out = g_st;
    out->owner_pid = g_owner;
}
