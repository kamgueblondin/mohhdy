/* ipcwait: proof server for SYS_IPC_RECV_WAIT (blocking receive).
 *
 * Registers "ipc-wait", checks the two non-blocking paths (NULL buffer is
 * refused, timeout 0 is a poll), then sleeps in SYS_IPC_RECV_WAIT without
 * deadline. Each message is printed. The payload "timeout" makes it wait
 * once more with a 50-tick deadline (500 ms) and print the result. */
#include "ipc_wait_common.h"

static int data_is(const os_ipc_message_t* m, const char* text) {
    uint32_t i = 0U;
    while (text[i] != '\0') {
        if (i >= m->size || m->data[i] != (uint8_t)text[i]) return 0;
        i++;
    }
    return i == m->size;
}

static void print_message(const os_ipc_message_t* m) {
    uint32_t i;
    iw_puts("ipcwait got from ");
    iw_print_int(m->sender_pid);
    iw_puts(" type ");
    iw_print_int((int)m->type);
    iw_puts(" data ");
    for (i = 0U; i < m->size && i < OS_IPC_MAX_DATA; i++) iw_putc((char)m->data[i]);
    iw_putc('\n');
}

void main(void) {
    os_ipc_message_t message;
    int rc;
    rc = iw_service_register("ipc-wait");
    iw_puts("ipcwait registered rc ");
    iw_print_int(rc);
    iw_putc('\n');
    rc = iw_recv_wait((os_ipc_message_t*)0, 5U);
    iw_puts("ipcwait null rc ");
    iw_print_int(rc);
    iw_putc('\n');
    rc = iw_recv_wait(&message, 0U);
    iw_puts("ipcwait poll rc ");
    iw_print_int(rc);
    iw_putc('\n');
    iw_puts("ipcwait ready\n");
    for (;;) {
        rc = iw_recv_wait(&message, OS_IPC_WAIT_FOREVER);
        if (rc != 0) {
            iw_puts("ipcwait rc ");
            iw_print_int(rc);
            iw_putc('\n');
            continue;
        }
        print_message(&message);
        if (data_is(&message, "timeout")) {
            rc = iw_recv_wait(&message, 50U);
            if (rc == 0) {
                print_message(&message);
            } else {
                iw_puts("ipcwait timeout rc ");
                iw_print_int(rc);
                iw_putc('\n');
            }
        }
    }
}
