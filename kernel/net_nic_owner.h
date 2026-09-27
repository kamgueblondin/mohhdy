#ifndef MOHHDY_NET_NIC_OWNER_H
#define MOHHDY_NET_NIC_OWNER_H

/* Tranche 5 suite: who drives the NE2000 (pure logic, unit-tested).
 *
 * 0 = the kernel (boot probe, degraded path, reclaim). Otherwise the PID of
 * the live net-driver worker that claimed it: the TSS I/O bitmap opens
 * 0x300-0x31F only for that PID, IRQ3 becomes a counter it takes, and every
 * kernel NE2000 access is refused (counted) until the worker dies and the
 * kernel reclaims the card. */

#include <stdint.h>
#include "os_syscalls.h"

void nic_owner_init(void);
/* 0, OS_NET_WORKER_REQUIRED (not the live worker), OS_NET_NIC_ABSENT. */
int nic_owner_claim(int32_t pid, int32_t live_worker, int nic_present);
int32_t nic_owner_pid(void);
/* Ports open for this task on a switch: it owns the NIC and is still the
 * live worker. */
int nic_owner_ports_open(int32_t task_pid, int32_t live_worker);
int nic_owner_kernel_may_touch(void);
/* Owner no longer the live worker: drop it. 1 = the kernel must reclaim. */
int nic_owner_drop_if_gone(int32_t live_worker);
/* IRQ3: 1 = forwarded to the owner (kernel must not touch the ports). */
int nic_owner_irq(void);
uint32_t nic_owner_irq_take(int32_t pid);
void nic_owner_note_kernel_refused(void);
void nic_owner_note_kernel_gated(void);
void nic_owner_note_reclaim(void);
void nic_owner_note_pump(uint32_t frames_out, uint32_t frame_in, uint32_t tx_ok, uint32_t tx_failed);
void nic_owner_fill_status(os_net_nic_status_t* out);

#endif
