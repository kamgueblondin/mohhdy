#include "ipc.h"

void ipc_endpoint_init(ipc_endpoint_t* endpoint) {
    uint32_t i;
    uint32_t j;
    if (!endpoint) return;
    endpoint->read_index = 0U;
    endpoint->write_index = 0U;
    endpoint->count = 0U;
    for (i = 0U; i < IPC_ENDPOINT_CAPACITY; i++) {
        endpoint->messages[i].sender_pid = -1;
        endpoint->messages[i].type = 0U;
        endpoint->messages[i].size = 0U;
        endpoint->messages[i].request_id = 0U;
        for (j = 0U; j < OS_IPC_MAX_DATA; j++) endpoint->messages[i].data[j] = 0U;
    }
}

int ipc_endpoint_send(ipc_endpoint_t* endpoint, int32_t sender_pid,
                      const os_ipc_payload_t* payload) {
    os_ipc_message_t* destination;
    uint32_t i;
    if (!endpoint || !payload || payload->size > OS_IPC_MAX_DATA) {
        return OS_IPC_BAD_MESSAGE;
    }
    if (endpoint->count >= IPC_ENDPOINT_CAPACITY) return OS_IPC_FULL;

    destination = &endpoint->messages[endpoint->write_index];
    destination->sender_pid = sender_pid;
    destination->type = payload->type;
    destination->size = payload->size;
    destination->request_id = payload->request_id;
    for (i = 0U; i < payload->size; i++) destination->data[i] = payload->data[i];
    for (; i < OS_IPC_MAX_DATA; i++) destination->data[i] = 0U;

    endpoint->write_index = (endpoint->write_index + 1U) % IPC_ENDPOINT_CAPACITY;
    endpoint->count++;
    return 0;
}

int ipc_endpoint_receive(ipc_endpoint_t* endpoint, os_ipc_message_t* out) {
    os_ipc_message_t* source;
    uint32_t i;
    if (!endpoint || !out) return OS_IPC_BAD_MESSAGE;
    if (endpoint->count == 0U) return OS_IPC_EMPTY;

    source = &endpoint->messages[endpoint->read_index];
    out->sender_pid = source->sender_pid;
    out->type = source->type;
    out->size = source->size;
    out->request_id = source->request_id;
    for (i = 0U; i < OS_IPC_MAX_DATA; i++) out->data[i] = source->data[i];

    source->sender_pid = -1;
    source->type = 0U;
    source->size = 0U;
    source->request_id = 0U;
    for (i = 0U; i < OS_IPC_MAX_DATA; i++) source->data[i] = 0U;
    endpoint->read_index = (endpoint->read_index + 1U) % IPC_ENDPOINT_CAPACITY;
    endpoint->count--;
    return 0;
}
