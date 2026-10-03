#include "ipc_wait.h"

int ipc_wait_decide(int recv_rc, int empty_rc, uint32_t timeout, int may_block) {
    if (recv_rc != empty_rc) return IPC_WAIT_RETURN; /* message or error */
    if (timeout == 0U || !may_block) return IPC_WAIT_RETURN;
    return IPC_WAIT_BLOCK;
}

void ipc_wait_begin(ipc_wait_t* w, uint32_t now, uint32_t timeout, uint32_t forever_value) {
    if (!w) return;
    w->blocked = 1U;
    w->forever = timeout == forever_value ? 1U : 0U;
    w->timed_out = 0U;
    w->woken = 0U;
    w->deadline = now + timeout;
    w->waits++;
}

int ipc_wait_on_message(ipc_wait_t* w) {
    if (!w || !w->blocked) return 0;
    w->blocked = 0U;
    w->woken = 1U;
    w->wakes++;
    return 1;
}

int ipc_wait_on_tick(ipc_wait_t* w, uint32_t now) {
    if (!w || !w->blocked || w->forever) return 0;
    /* Wrap-safe: expired once now is at or past the deadline. */
    if ((int32_t)(now - w->deadline) < 0) return 0;
    w->blocked = 0U;
    w->timed_out = 1U;
    w->timeouts++;
    return 1;
}

int ipc_wait_after_wake(ipc_wait_t* w, int recv_rc, int empty_rc, int timeout_rc,
                        uint32_t now, int* out_rc) {
    if (!w || !out_rc) return IPC_WAIT_RETURN;
    if (recv_rc != empty_rc) {
        w->blocked = 0U;
        *out_rc = recv_rc;
        return IPC_WAIT_RETURN;
    }
    if (w->timed_out || (!w->forever && (int32_t)(now - w->deadline) >= 0)) {
        if (!w->timed_out) w->timeouts++;
        w->timed_out = 1U;
        w->blocked = 0U;
        *out_rc = timeout_rc;
        return IPC_WAIT_RETURN;
    }
    /* Spurious: keep the original deadline. */
    w->blocked = 1U;
    w->woken = 0U;
    return IPC_WAIT_BLOCK;
}

void ipc_wait_cancel(ipc_wait_t* w) {
    if (!w) return;
    w->blocked = 0U;
    w->woken = 0U;
    w->timed_out = 0U;
}
