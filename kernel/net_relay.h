#ifndef NET_RELAY_H
#define NET_RELAY_H

/* Tranche 5 slice 2: single-slot relay of socket syscalls from non-worker
 * tasks to the live net-driver worker (pure logic, unit tested). The IPC
 * send, user copies and scheduling live in kernel/syscall/syscall.c. */

#include <stdint.h>
#include "../include/os_syscalls.h"

#define NET_RELAY_FREE 0U
#define NET_RELAY_SENT 1U  /* forwarded, waiting for the worker reply */
#define NET_RELAY_DONE 2U  /* reply stored, waiting for the caller */

/* No reply after this many timer ticks (100 Hz): OS_NET_RELAY_TIMEOUT. */
#define NET_RELAY_TIMEOUT_TICKS 500U
/* ...and only after the caller was scheduled this many times while waiting
 * (so a runnable worker had turns too). */
#define NET_RELAY_TIMEOUT_POLLS 3U

void net_relay_init(void);
/* 1 for the socket syscalls 99-108 that are relayed. */
int net_relay_supported(uint32_t syscall_number);
/* Slot state as seen by pid: FREE (slot free or owned by someone else is
 * reported through net_relay_owner), SENT or DONE when owned by pid. */
uint32_t net_relay_state_for(int32_t pid);
int32_t net_relay_owner(void);
int32_t net_relay_worker(void);
/* Take the free slot for pid, returns the job id (> 0) or -1 when busy. */
int32_t net_relay_begin(int32_t pid, int32_t worker_pid, uint32_t op, uint32_t now);
/* The IPC send failed: give the slot back without counting a forward. */
void net_relay_cancel(void);
/* Worker reply: 0 accepted, OS_NET_RELAY_... error or -1 when stale. */
int net_relay_complete(int32_t worker_pid, uint32_t job_id, int32_t result,
                       const uint8_t* out, uint32_t out_length);
/* Caller re-entered while its request is SENT. */
void net_relay_note_poll(void);
/* 1 when the SENT request is older than the timeout. */
int net_relay_expired(uint32_t now);
/* Caller-side end of a DONE slot: copies out the stored reply. */
int32_t net_relay_take(int32_t pid, uint32_t* op, uint8_t* out, uint32_t capacity,
                       uint32_t* out_length);
/* Worker gone or timeout while SENT: the slot becomes DONE with this error
 * so the caller gets it once (never replayed). */
void net_relay_fail(int32_t error);
/* Owner task gone: slot freed, a late reply becomes stale. */
void net_relay_drop_owner(void);
void net_relay_note_denied(void);
void net_relay_fill_status(os_net_relay_status_t* out, int32_t live_worker);

#endif
