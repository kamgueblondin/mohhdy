#ifndef OS_IPC_DEFERRED_H
#define OS_IPC_DEFERRED_H

#include <stdint.h>
#include "os_syscalls.h"

/* File locale Ring 3 : même capacité que l'endpoint noyau, sans allocation. */
#define OS_IPC_DEFERRED_CAPACITY 4U

typedef struct {
    os_ipc_message_t messages[OS_IPC_DEFERRED_CAPACITY];
    uint32_t count;
} os_ipc_deferred_t;

static inline void os_ipc_deferred_init(os_ipc_deferred_t* queue) {
    uint32_t i;
    uint32_t j;
    if (!queue) return;
    queue->count = 0U;
    for (i = 0U; i < OS_IPC_DEFERRED_CAPACITY; i++) {
        queue->messages[i].sender_pid = -1;
        queue->messages[i].type = 0U;
        queue->messages[i].size = 0U;
        queue->messages[i].request_id = 0U;
        for (j = 0U; j < OS_IPC_MAX_DATA; j++) queue->messages[i].data[j] = 0U;
    }
}

static inline int os_ipc_deferred_push(os_ipc_deferred_t* queue,
                                       const os_ipc_message_t* message) {
    uint32_t i;
    if (!queue || !message || message->size > OS_IPC_MAX_DATA) return OS_IPC_BAD_MESSAGE;
    if (queue->count >= OS_IPC_DEFERRED_CAPACITY) return OS_IPC_FULL;
    queue->messages[queue->count].sender_pid = message->sender_pid;
    queue->messages[queue->count].type = message->type;
    queue->messages[queue->count].size = message->size;
    queue->messages[queue->count].request_id = message->request_id;
    for (i = 0U; i < OS_IPC_MAX_DATA; i++) queue->messages[queue->count].data[i] = message->data[i];
    queue->count++;
    return 0;
}

static inline int os_ipc_deferred_take(os_ipc_deferred_t* queue,
                                       os_ipc_message_t* out) {
    uint32_t i;
    uint32_t j;
    if (!queue || !out) return OS_IPC_BAD_MESSAGE;
    if (queue->count == 0U) return OS_IPC_EMPTY;
    *out = queue->messages[0];
    for (i = 1U; i < queue->count; i++) queue->messages[i - 1U] = queue->messages[i];
    queue->count--;
    queue->messages[queue->count].sender_pid = -1;
    queue->messages[queue->count].type = 0U;
    queue->messages[queue->count].size = 0U;
    queue->messages[queue->count].request_id = 0U;
    for (j = 0U; j < OS_IPC_MAX_DATA; j++) queue->messages[queue->count].data[j] = 0U;
    return 0;
}

/* Extrait une réponse corrélée de façon complète. L’expéditeur est inclus
 * afin qu’une réponse de même type et même identifiant provenant d’un autre
 * service reste disponible dans la file locale. */
static inline int os_ipc_deferred_take_matching_from(os_ipc_deferred_t* queue,
                                                     int32_t sender_pid, uint32_t type,
                                                     uint32_t request_id,
                                                     os_ipc_message_t* out) {
    uint32_t i;
    uint32_t j;
    if (!queue || !out) return OS_IPC_BAD_MESSAGE;
    for (i = 0U; i < queue->count; i++) {
        if (queue->messages[i].sender_pid == sender_pid && queue->messages[i].type == type &&
            queue->messages[i].request_id == request_id) {
            *out = queue->messages[i];
            for (; i + 1U < queue->count; i++) queue->messages[i] = queue->messages[i + 1U];
            queue->count--;
            queue->messages[queue->count].sender_pid = -1;
            queue->messages[queue->count].type = 0U;
            queue->messages[queue->count].size = 0U;
            queue->messages[queue->count].request_id = 0U;
            for (j = 0U; j < OS_IPC_MAX_DATA; j++) queue->messages[queue->count].data[j] = 0U;
            return 0;
        }
    }
    return OS_IPC_EMPTY;
}

static inline int os_ipc_deferred_take_matching(os_ipc_deferred_t* queue,
                                                uint32_t type, uint32_t request_id,
                                                os_ipc_message_t* out) {
    uint32_t i;
    uint32_t j;
    if (!queue || !out) return OS_IPC_BAD_MESSAGE;
    for (i = 0U; i < queue->count; i++) {
        if (queue->messages[i].type == type && queue->messages[i].request_id == request_id) {
            *out = queue->messages[i];
            for (; i + 1U < queue->count; i++) queue->messages[i] = queue->messages[i + 1U];
            queue->count--;
            queue->messages[queue->count].sender_pid = -1;
            queue->messages[queue->count].type = 0U;
            queue->messages[queue->count].size = 0U;
            queue->messages[queue->count].request_id = 0U;
            for (j = 0U; j < OS_IPC_MAX_DATA; j++) queue->messages[queue->count].data[j] = 0U;
            return 0;
        }
    }
    return OS_IPC_EMPTY;
}

#endif
