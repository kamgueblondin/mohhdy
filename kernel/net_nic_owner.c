#include "net_nic_owner.h"

/* Tranche 5 suite: see net_nic_owner.h. */

static int32_t g_owner;
static uint32_t g_pending_irq;
static os_net_nic_status_t g_st;
static os_net_stack_report_t g_report;
static os_net_wire_status_t g_retired;
static uint8_t g_live_reports;

static void owner_zero(void* p, uint32_t n) {
    uint8_t* b = (uint8_t*)p;
    while (n--) *b++ = 0U;
}

static void wire_add(os_net_wire_status_t* a, const os_net_wire_status_t* b) {
    a->connects += b->connects; a->frames_tx += b->frames_tx; a->frames_rx += b->frames_rx;
    a->arp_tx += b->arp_tx; a->sends += b->sends; a->recvs += b->recvs;
    a->closes += b->closes; a->refused += b->refused; a->demuxed += b->demuxed;
    a->dropped += b->dropped; a->arp_replies += b->arp_replies; a->peer_fins += b->peer_fins;
}

static void owner_retire(void) {
    uint32_t reports = g_report.reports;
    wire_add(&g_retired, &g_report.wire);
    owner_zero(&g_report, sizeof(g_report));
    g_report.reports = reports;
    g_live_reports = 0U;
}

void nic_owner_init(void) {
    uint32_t i;
    uint8_t* p = (uint8_t*)&g_st;
    for (i = 0U; i < sizeof(g_st); i++) p[i] = 0U;
    g_owner = 0;
    g_pending_irq = 0U;
    owner_zero(&g_report, sizeof(g_report));
    owner_zero(&g_retired, sizeof(g_retired));
    g_live_reports = 0U;
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

int nic_owner_release(int32_t pid) {
    if (pid <= 0 || g_owner != pid) return OS_NET_WORKER_REQUIRED;
    g_owner = 0;
    g_pending_irq = 0U;
    return 0;
}

int32_t nic_owner_pid(void) { return g_owner; }

int nic_owner_ports_open(int32_t task_pid, int32_t live_worker) {
    return g_owner > 0 && task_pid == g_owner && g_owner == live_worker;
}

int nic_owner_kernel_may_touch(void) { return g_owner == 0; }

int nic_owner_drop_if_gone(int32_t live_worker) {
    if (g_owner == 0 || g_owner == live_worker) return 0;
    owner_retire();
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

int nic_owner_publish(int32_t pid, const os_net_stack_report_t* report) {
    uint32_t reports;
    if (!report || pid <= 0 || pid != g_owner) return OS_NET_WORKER_REQUIRED;
    reports = g_report.reports + 1U;
    g_report = *report;
    g_report.wire.worker_pid = pid;
    g_report.reports = reports;
    g_live_reports = 1U;
    return 0;
}

int nic_owner_stack(os_net_stack_report_t* out) {
    if (!out) return 0;
    if (g_owner <= 0 || !g_live_reports) {
        owner_zero(out, sizeof(*out));
        out->reports = g_report.reports;
        return 0;
    }
    *out = g_report;
    return 1;
}

void nic_owner_merge_wire(os_net_wire_status_t* inout) {
    if (!inout) return;
    wire_add(inout, &g_retired);
    if (g_owner > 0 && g_live_reports) {
        wire_add(inout, &g_report.wire);
        inout->bound += g_report.wire.bound;
    }
}

uint32_t nic_owner_llm_status(uint32_t kernel_word) {
    return (g_owner > 0 && g_live_reports) ? g_report.llm_status : kernel_word;
}
