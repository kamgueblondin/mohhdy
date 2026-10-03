#ifndef IPC_WAIT_H
#define IPC_WAIT_H

#include <stdint.h>

/* Blocking receive (SYS_IPC_RECV_WAIT) state machine, one per task.
 *
 * Pure logic, no scheduler access, so it is unit tested on the host
 * (tests/unit/kernel/test_ipc_wait.c). kernel/task/task.c applies the
 * decisions: TASK_BLOCKED_IPC <-> TASK_READY. */
typedef struct {
    uint8_t blocked;    /* waiting for a message */
    uint8_t forever;    /* no deadline */
    uint8_t timed_out;  /* deadline reached before a message */
    uint8_t woken;      /* a message was queued while blocked */
    uint32_t deadline;  /* tick at which the wait expires (if !forever) */
    uint32_t waits;     /* number of times the task actually blocked */
    uint32_t wakes;     /* wake-ups by a queued message */
    uint32_t timeouts;  /* wake-ups by the deadline */
} ipc_wait_t;

#define IPC_WAIT_RETURN 0  /* answer the caller now with the receive result */
#define IPC_WAIT_BLOCK  1  /* sleep until a message or the deadline */

/* After a non-blocking receive attempt. timeout 0 is a plain poll; a task
 * that may not block (kernel task, ATA RPC owner) gets the poll result. */
int ipc_wait_decide(int recv_rc, int empty_rc, uint32_t timeout, int may_block);
/* Enter the blocked state. timeout == forever_value means no deadline. */
void ipc_wait_begin(ipc_wait_t* w, uint32_t now, uint32_t timeout, uint32_t forever_value);
/* A message was queued for the task. 1 if a blocked waiter must be made ready. */
int ipc_wait_on_message(ipc_wait_t* w);
/* Timer tick. 1 if a blocked waiter reached its deadline (make it ready). */
int ipc_wait_on_tick(ipc_wait_t* w, uint32_t now);
/* Back in the syscall after a wake-up and a new receive attempt.
 * Returns IPC_WAIT_RETURN with *out_rc set, or IPC_WAIT_BLOCK when the
 * wake-up was spurious and the deadline is still ahead: the task blocks
 * again with the same deadline. */
int ipc_wait_after_wake(ipc_wait_t* w, int recv_rc, int empty_rc, int timeout_rc,
                        uint32_t now, int* out_rc);
/* Forget any wait (task exit/kill or reuse). Counters are kept. */
void ipc_wait_cancel(ipc_wait_t* w);

#endif
