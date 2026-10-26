/* out_capture.c - Lot 46: bounded capture of the output of the programs a
 * desktop window launches (see out_capture.h). */
#include "out_capture.h"

static int32_t g_owner;
static char g_buf[OS_OUTCAP_MAX];
static uint32_t g_n;          /* bytes kept, not read yet */
static uint32_t g_total;      /* bytes seen since begin */
static uint32_t g_dropped;    /* bytes past the bound */
static uint32_t g_untracked;  /* children not tracked (pid table full) */
static int32_t g_pids[OS_OUTCAP_PIDS];
static int g_npids;

void out_cap_reset(void) {
    g_owner = 0; g_n = 0; g_total = 0; g_dropped = 0; g_untracked = 0; g_npids = 0;
}

int out_cap_begin(int32_t owner) {
    if (owner <= 0) return OS_OUTCAP_BAD;
    if (g_owner > 0 && g_owner != owner) return OS_OUTCAP_BUSY;
    out_cap_reset();
    g_owner = owner;
    return 0;
}

int out_cap_end(int32_t owner) {
    int d;
    if (owner <= 0 || owner != g_owner) return OS_OUTCAP_BAD;
    d = (int)g_dropped;
    out_cap_reset();
    return d;
}

int32_t out_cap_owner(void) { return g_owner; }

int out_cap_tracked(int32_t pid) {
    int k;
    if (g_owner <= 0 || pid <= 0) return 0;
    for (k = 0; k < g_npids; k++) if (g_pids[k] == pid) return 1;
    return 0;
}

int out_cap_child(int32_t parent, int32_t child) {
    if (g_owner <= 0 || child <= 0 || child == g_owner) return 0;
    if (parent != g_owner && !out_cap_tracked(parent)) return 0;
    if (out_cap_tracked(child)) return 1;
    if (g_npids >= OS_OUTCAP_PIDS) { g_untracked++; return 0; }
    g_pids[g_npids++] = child;
    return 1;
}

int out_cap_put(int32_t pid, char c) {
    if (!out_cap_tracked(pid)) return 0;
    g_total++;
    if (g_n < OS_OUTCAP_MAX) g_buf[g_n++] = c;
    else g_dropped++;
    return 1;
}

int out_cap_read(int32_t caller, char *dst, int max) {
    uint32_t k, take;
    if (caller <= 0 || caller != g_owner || !dst || max <= 0) return OS_OUTCAP_BAD;
    take = g_n;
    if (take > (uint32_t)(max - 1)) take = (uint32_t)(max - 1);
    for (k = 0; k < take; k++) dst[k] = g_buf[k];
    dst[take] = 0;
    /* keep the unread rest at the front (a caller with a small buffer reads
     * it in several calls) */
    for (k = take; k < g_n; k++) g_buf[k - take] = g_buf[k];
    g_n -= take;
    return (int)take;
}

uint32_t out_cap_dropped(void) { return g_dropped; }
uint32_t out_cap_untracked(void) { return g_untracked; }
uint32_t out_cap_total(void) { return g_total; }
int out_cap_pid_count(void) { return g_owner > 0 ? g_npids : 0; }
int32_t out_cap_pid_at(int k) { return (k >= 0 && k < g_npids) ? g_pids[k] : 0; }
