#ifndef MOHHDY_NET_ETHERNET_ARP_H
#define MOHHDY_NET_ETHERNET_ARP_H

#include <stdint.h>

#define NET_ETHERNET_HEADER_SIZE 14U
#define NET_ARP_PACKET_SIZE 28U
#define NET_ETHERTYPE_IPV4 0x0800U
#define NET_ETHERTYPE_ARP  0x0806U
#define NET_ARP_HTYPE_ETHERNET 1U
#define NET_ARP_PTYPE_IPV4 0x0800U
#define NET_ARP_OPCODE_REQUEST 1U
#define NET_ARP_OPCODE_REPLY 2U
#define NET_ARP_CACHE_CAPACITY 8U

/* Aucune structure ne pointe dans une trame reçue: les adresses sont copiées. */
typedef struct {
    uint8_t destination[6];
    uint8_t source[6];
    uint16_t ethertype;
} net_ethernet_header_t;

typedef struct {
    uint8_t valid;
    uint8_t ipv4[4];
    uint8_t mac[6];
} net_arp_cache_entry_t;

typedef struct {
    net_arp_cache_entry_t entries[NET_ARP_CACHE_CAPACITY];
} net_arp_cache_t;

typedef struct {
    uint16_t hardware_type;
    uint16_t protocol_type;
    uint8_t hardware_size;
    uint8_t protocol_size;
    uint16_t opcode;
    uint8_t sender_mac[6];
    uint8_t sender_ipv4[4];
    uint8_t target_mac[6];
    uint8_t target_ipv4[4];
} net_arp_packet_t;

/* Parse exactement un en-tête Ethernet. Retourne 0 si la trame est trop courte. */
int net_ethernet_parse(const uint8_t* frame, uint32_t length,
                       net_ethernet_header_t* out);
/* Parse ARP Ethernet/IPv4 à partir du début d’une trame Ethernet. */
int net_arp_parse(const uint8_t* frame, uint32_t length,
                  net_arp_packet_t* out);
/* Construit une requête ARP Ethernet/IPv4 dans le buffer caller-owned. */
int net_arp_build_request(uint8_t* frame, uint32_t capacity,
                          const uint8_t sender_mac[6], const uint8_t sender_ipv4[4],
                          const uint8_t target_ipv4[4]);
/* Construit une réponse ARP à partir d’une requête décodée, sans allocation. */
int net_arp_build_reply(uint8_t* frame, uint32_t capacity,
                        const net_arp_packet_t* request,
                        const uint8_t local_mac[6], const uint8_t local_ipv4[4]);
/* Reconnaît uniquement une réponse ARP destinée à l’adresse IPv4 fournie. */
int net_arp_is_reply_for(const net_arp_packet_t* packet,
                         const uint8_t local_ipv4[4],
                         const uint8_t requested_ipv4[4]);
/* Cache ARP caller-owned de capacité fixe, sans allocation dynamique. */
int net_arp_cache_init(net_arp_cache_t* cache);
int net_arp_cache_put(net_arp_cache_t* cache, const uint8_t ipv4[4], const uint8_t mac[6]);
int net_arp_cache_lookup(const net_arp_cache_t* cache, const uint8_t ipv4[4], uint8_t mac[6]);
int net_arp_cache_invalidate(net_arp_cache_t* cache, const uint8_t ipv4[4]);

#endif
