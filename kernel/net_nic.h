#ifndef MOHHDY_NET_NIC_H
#define MOHHDY_NET_NIC_H

#include <stdint.h>

#define NET_NIC_QUEUE_CAPACITY 8U
#define NET_NIC_FRAME_CAPACITY 1536U
#define NET_NIC_ETHERNET_MIN_FRAME 60U

typedef struct {
    uint8_t* storage;
    uint16_t capacity;
    uint16_t length;
} net_nic_frame_t;

typedef struct {
    net_nic_frame_t frames[NET_NIC_QUEUE_CAPACITY];
    uint8_t head;
    uint8_t tail;
    uint8_t count;
} net_nic_queue_t;

/* Initialise une file avec des buffers fournis par l’appelant. */
int net_nic_queue_init(net_nic_queue_t* queue,
                       uint8_t* storage, uint32_t storage_size,
                       uint16_t frame_capacity);
/* Réinitialisation O(1): les buffers restent la propriété de l’appelant. */
void net_nic_queue_reset(net_nic_queue_t* queue);
/* Réserve le prochain emplacement RX/TX sans copier la trame. */
int net_nic_queue_acquire(net_nic_queue_t* queue, uint8_t** buffer,
                          uint16_t* capacity);
/* Publie la longueur de l’emplacement réservé. */
int net_nic_queue_commit(net_nic_queue_t* queue, uint16_t length);
/* Retire la prochaine trame prête et retourne sa vue bornée. */
int net_nic_queue_pop(net_nic_queue_t* queue, uint8_t** buffer,
                      uint16_t* length);
uint8_t net_nic_queue_count(const net_nic_queue_t* queue);
/* Copie et publie une trame Ethernet TX après validation des bornes. */
int net_nic_queue_push_frame(net_nic_queue_t* queue, const uint8_t* frame,
                             uint16_t length);

#endif
