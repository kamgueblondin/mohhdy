/* Lot 4 (US-001): PCI inventory published by the Ring 3 pcidriver.
 * The worker enumerates the bus through ports 0xCF8/0xCFC opened in its
 * TSS IOPB; the kernel only validates and stores the records. */
#include "pci_service.h"

static os_pci_record_t g_records[OS_PCI_MAX_RECORDS];
static uint32_t g_count;
static uint32_t g_generation;
static uint32_t g_respawns;
static uint32_t g_ring0_denied;
static uint32_t g_rejected;
static uint32_t g_stale;
static uint32_t g_handed_off;
static int32_t g_publisher;

void pci_service_reset(void) {
    uint32_t i;
    for (i = 0U; i < OS_PCI_MAX_RECORDS; i++) {
        uint8_t* bytes = (uint8_t*)&g_records[i];
        uint32_t j;
        for (j = 0U; j < sizeof(g_records[i]); j++) bytes[j] = 0U;
    }
    g_count = g_generation = g_respawns = g_ring0_denied = g_rejected = g_stale = g_handed_off = 0U;
    g_publisher = 0;
}

int pci_record_valid(const os_pci_record_t* r) {
    if (!r) return 0;
    if (r->vendor_id == 0xffffU || r->vendor_id == 0U) return 0;
    if (r->slot >= 32U || r->function >= 8U) return 0;
    if (r->reserved != 0U) return 0;
    return 1;
}

int pci_service_publish(int32_t pid, int32_t live_owner, const os_pci_record_t* records, uint32_t count) {
    uint32_t i, j;
    if (pid <= 0 || live_owner <= 0 || pid != live_owner) { g_rejected++; return OS_PCI_DENIED; }
    if (!records || count == 0U || count > OS_PCI_MAX_RECORDS) { g_rejected++; return OS_PCI_BAD_REQUEST; }
    for (i = 0U; i < count; i++) {
        if (!pci_record_valid(&records[i])) { g_rejected++; return OS_PCI_BAD_REQUEST; }
        /* Une fonction (bus, slot, fonction) n'apparait qu'une fois. */
        for (j = 0U; j < i; j++)
            if (records[j].bus == records[i].bus && records[j].slot == records[i].slot &&
                records[j].function == records[i].function) { g_rejected++; return OS_PCI_BAD_REQUEST; }
    }
    for (i = 0U; i < count; i++) g_records[i] = records[i];
    g_count = count;
    g_generation++;
    g_publisher = pid;
    g_stale = 0U;
    g_handed_off = 1U;
    return (int)count;
}

int pci_service_list(os_pci_record_t* out, uint32_t capacity) {
    uint32_t i, n;
    if (!out) return OS_PCI_BAD_REQUEST;
    if (!g_handed_off) return OS_PCI_NO_INVENTORY;
    n = g_count < capacity ? g_count : capacity;
    for (i = 0U; i < n; i++) out[i] = g_records[i];
    return (int)n;
}

void pci_service_status(int32_t live_owner, os_pci_status_t* out) {
    if (!out) return;
    out->owner_pid = live_owner;
    out->publisher_pid = g_publisher;
    out->count = g_count;
    out->generation = g_generation;
    out->respawns = g_respawns;
    out->ring0_denied = g_ring0_denied;
    out->rejected_publish = g_rejected;
    out->stale = g_stale;
    out->handed_off = g_handed_off;
}

int pci_service_find_class(uint8_t class_code, uint8_t subclass, uint8_t any_subclass, os_pci_record_t* out) {
    uint32_t i;
    for (i = 0U; i < g_count; i++) {
        if (g_records[i].class_code != class_code) continue;
        if (!any_subclass && g_records[i].subclass != subclass) continue;
        if (out) *out = g_records[i];
        return 0;
    }
    return -1;
}

int pci_service_handed_off(void) { return g_handed_off ? 1 : 0; }

void pci_service_owner_gone(void) { if (g_handed_off) g_stale = 1U; }

int pci_service_should_respawn(int32_t live_owner, int pending_alive) {
    if (!g_handed_off || live_owner > 0 || pending_alive) return 0;
    return g_respawns < PCI_SERVICE_RESPAWN_LIMIT ? 1 : 0;
}

void pci_service_note_respawn(void) { g_respawns++; }

void pci_service_note_ring0_denied(void) { g_ring0_denied++; }
