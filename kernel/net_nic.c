#include "net_nic.h"

int net_nic_queue_init(net_nic_queue_t* queue,
                       uint8_t* storage, uint32_t storage_size,
                       uint16_t frame_capacity) {
    uint32_t i;
    if (!queue || !storage || frame_capacity == 0U ||
        frame_capacity > NET_NIC_FRAME_CAPACITY ||
        storage_size < frame_capacity * NET_NIC_QUEUE_CAPACITY)
        return -1;
    queue->head = 0U;
    queue->tail = 0U;
    queue->count = 0U;
    for (i = 0; i < NET_NIC_QUEUE_CAPACITY; ++i) {
        queue->frames[i].storage = storage + i * frame_capacity;
        queue->frames[i].capacity = frame_capacity;
        queue->frames[i].length = 0U;
    }
    return 0;
}

void net_nic_queue_reset(net_nic_queue_t* queue) {
    uint32_t i;
    if (!queue) return;
    queue->head = 0U;
    queue->tail = 0U;
    queue->count = 0U;
    for (i = 0; i < NET_NIC_QUEUE_CAPACITY; ++i)
        queue->frames[i].length = 0U;
}

int net_nic_queue_acquire(net_nic_queue_t* queue, uint8_t** buffer,
                          uint16_t* capacity) {
    if (!queue || !buffer || !capacity || queue->count >= NET_NIC_QUEUE_CAPACITY)
        return -1;
    *buffer = queue->frames[queue->tail].storage;
    *capacity = queue->frames[queue->tail].capacity;
    return 0;
}

int net_nic_queue_commit(net_nic_queue_t* queue, uint16_t length) {
    if (!queue || queue->count >= NET_NIC_QUEUE_CAPACITY ||
        length > queue->frames[queue->tail].capacity)
        return -1;
    queue->frames[queue->tail].length = length;
    queue->tail = (uint8_t)((queue->tail + 1U) % NET_NIC_QUEUE_CAPACITY);
    queue->count++;
    return 0;
}

int net_nic_queue_pop(net_nic_queue_t* queue, uint8_t** buffer,
                      uint16_t* length) {
    if (!queue || !buffer || !length || queue->count == 0U)
        return -1;
    *buffer = queue->frames[queue->head].storage;
    *length = queue->frames[queue->head].length;
    queue->head = (uint8_t)((queue->head + 1U) % NET_NIC_QUEUE_CAPACITY);
    queue->count--;
    return 0;
}

uint8_t net_nic_queue_count(const net_nic_queue_t* queue) {
    return queue ? queue->count : 0U;
}

int net_nic_queue_push_frame(net_nic_queue_t* queue, const uint8_t* frame,
                             uint16_t length) {
    uint8_t* destination;
    uint16_t capacity;
    uint16_t i;
    if (!queue || !frame || length < NET_NIC_ETHERNET_MIN_FRAME)
        return -1;
    if (net_nic_queue_acquire(queue, &destination, &capacity) != 0 ||
        length > capacity)
        return -2;
    for (i = 0; i < length; ++i) destination[i] = frame[i];
    return net_nic_queue_commit(queue, length);
}
