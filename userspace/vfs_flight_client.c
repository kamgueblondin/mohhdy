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

static int data_equal(const uint8_t* data, uint32_t size, const char* expected) {
    uint32_t index = 0U;
    while (expected[index] != '\0') {
        if (index >= size || data[index] != (uint8_t)expected[index]) return 0;
        index++;
    }
    return index == size;
}

void main(void) {
    static const char expected[] = "vfsserver ring3 policy\n";
    static const uint32_t request_id = 0x56465346U;
    os_ipc_payload_t request;
    os_ipc_message_t message;
    os_vfs_read_reply_t reply;
    int server_pid = service_lookup("vfs");
    if (server_pid <= 0 ||
        os_vfs_make_read_request(&request, "vfs-info", request_id) != OS_VFS_STATUS_OK ||
        ipc_send(server_pid, &request) != 0) {
        puts("vfsflight request failed\n");
        for (;;) yield();
    }
    puts("vfsflight waiting vfs-info\n");
    for (;;) {
        if (ipc_receive(&message) == 0 && message.sender_pid == server_pid &&
            message.type == OS_IPC_VFS_READ_REPLY &&
            os_vfs_parse_read_reply(&message, &reply, request_id) == OS_VFS_STATUS_OK) {
            if (reply.status == OS_VFS_STATUS_OK &&
                data_equal(reply.data, reply.size, expected)) {
                puts("vfsflight local reply ok\n");
            } else {
                puts("vfsflight reply invalid\n");
            }
            for (;;) yield();
        }
        yield();
    }
}
