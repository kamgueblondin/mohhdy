#ifndef MOHHDY_PCI_SERVICE_H
#define MOHHDY_PCI_SERVICE_H

/* Lot 4 (US-001): inventory published by the Ring 3 pcidriver. Pure logic
 * (no port I/O, no task table) so the host unit tests cover it. */
#include <stdint.h>
#include "../include/os_syscalls.h"

#define PCI_SERVICE_RESPAWN_LIMIT 4U

void pci_service_reset(void);
/* Publish from `pid`; `live_owner` is the current live pci-driver pid. */
int pci_service_publish(int32_t pid, int32_t live_owner, const os_pci_record_t* records, uint32_t count);
int pci_service_list(os_pci_record_t* out, uint32_t capacity);
void pci_service_status(int32_t live_owner, os_pci_status_t* out);
int pci_service_find_class(uint8_t class_code, uint8_t subclass, uint8_t any_subclass, os_pci_record_t* out);
/* 1 once an inventory was published: Ring 0 config reads are then refused. */
int pci_service_handed_off(void);
/* Called when the live owner is gone: inventory kept but marked stale. */
void pci_service_owner_gone(void);
/* Supervisor decision: 1 = spawn bin/pcidriver now. `pending_alive` = a
 * previously spawned driver is still starting. */
int pci_service_should_respawn(int32_t live_owner, int pending_alive);
void pci_service_note_respawn(void);
void pci_service_note_ring0_denied(void);
int pci_record_valid(const os_pci_record_t* record);

/* Kernel glue (syscall.c): live "pci-driver" owner pid (binary pcidriver),
 * 0 when none; supervisor tick (respawn after death, budgeted). */
int32_t pci_live_driver(void);
void pci_supervise(void);

#endif
