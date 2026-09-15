#include "os_vfs_service.h"

static void putc(char value) {
    asm volatile("int $0x80" : : "a"(SYS_PUTC), "b"(value));
}

static void puts(const char* text) {
    uint32_t index = 0U;
    while (text[index] != '\0') putc(text[index++]);
}

static int ipc_receive(os_ipc_message_t* message) {
    int result;
    asm volatile("int $0x80" : "=a"(result) : "a"(SYS_IPC_RECV), "b"(message));
    return result;
}

static int ipc_send(int target_pid, const os_ipc_payload_t* payload) {
    int result;
    asm volatile("int $0x80" : "=a"(result) : "a"(SYS_IPC_SEND), "b"(target_pid), "c"(payload));
    return result;
}

static int service_lookup(const char* name) {
    int result;
    asm volatile("int $0x80" : "=a"(result) : "a"(SYS_SERVICE_LOOKUP), "b"(name));
    return result;
}

static void yield(void) {
    asm volatile("int $0x80" : : "a"(SYS_YIELD));
}

void main(void) {
    static const uint32_t request_id = 0x56465347U;
    os_ipc_payload_t request;
    os_ipc_message_t message;
    os_vfs_read_reply_t reply;
    int server_pid = service_lookup("vfs");
    if (server_pid <= 0 ||
        os_vfs_make_read_request(&request, "assets/bin/shell", request_id) != OS_VFS_STATUS_OK ||
        ipc_send(server_pid, &request) != 0) {
        puts("vfsaliasflight request failed\n");
        for (;;) yield();
    }
    puts("vfsaliasflight waiting assets/bin/shell\n");
    for (;;) {
        if (ipc_receive(&message) == 0 && message.sender_pid == server_pid &&
            message.type == OS_IPC_VFS_READ_REPLY &&
            os_vfs_parse_read_reply(&message, &reply, request_id) == OS_VFS_STATUS_OK) {
            if (reply.status == OS_VFS_STATUS_INVALID) puts("vfsaliasflight bounded invalid\n");
            else puts("vfsaliasflight unexpected reply\n");
            for (;;) yield();
        }
        yield();
    }
}
