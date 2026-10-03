/* ipcpoke: sender for the SYS_IPC_RECV_WAIT contract.
 *
 * Finds "ipc-wait", sends "poke" (type 7), then itself sleeps in
 * SYS_IPC_RECV_WAIT without deadline so that the shell can kill a task
 * that is blocked in the call (killed receiver) after it sent (killed
 * sender). */
#include "ipc_wait_common.h"

void main(void) {
    os_ipc_payload_t payload;
    os_ipc_message_t message;
    uint32_t i;
    int target = iw_service_lookup("ipc-wait");
    int rc;
    if (target <= 0) {
        iw_puts("ipcpoke no target\n");
        return;
    }
    payload.type = 7U;
    payload.request_id = 0U;
    payload.size = 4U;
    for (i = 0U; i < OS_IPC_MAX_DATA; i++) payload.data[i] = 0U;
    payload.data[0] = 'p';
    payload.data[1] = 'o';
    payload.data[2] = 'k';
    payload.data[3] = 'e';
    rc = iw_send(target, &payload);
    iw_puts("ipcpoke sent rc ");
    iw_print_int(rc);
    iw_puts(" to ");
    iw_print_int(target);
    iw_putc('\n');
    iw_puts("ipcpoke waiting\n");
    rc = iw_recv_wait(&message, OS_IPC_WAIT_FOREVER);
    iw_puts("ipcpoke woke rc ");
    iw_print_int(rc);
    iw_putc('\n');
}
