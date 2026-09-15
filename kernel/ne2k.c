#include "ne2k.h"
#include "net_socket.h"
#include "net_llm_socket.h"
#include <stdint.h>
extern uint32_t timer_get_ticks(void) __attribute__((weak));

#ifdef __i386__
static uint8_t ne2k_i386_inb(void* context, uint16_t port) {
    uint8_t value; (void)context;
    __asm__ volatile ("inb %1, %0" : "=a"(value) : "Nd"(port));
    return value;
}
static void ne2k_i386_outb(void* context, uint16_t port, uint8_t value) {
    (void)context;
    __asm__ volatile ("outb %0, %1" : : "a"(value), "Nd"(port));
}
#endif

static ne2k_device_t* ne2k_irq_device;
static const ne2k_io_t* ne2k_irq_io;
static volatile uint32_t ne2k_irq_events;

/* Laisse au contrôleur matériel le temps de publier RX sans dépendre d’une
 * temporisation bloquante. Hors noyau (fixtures unitaires), le symbole faible
 * est absent et la pause devient immédiatement nulle. */
static void ne2k_poll_pause(void) {
    uint32_t before, spins = 0U;
    if (!timer_get_ticks) return;
    before = timer_get_ticks();
    /* 200 k PAUSE s’achèvent avant un tick sous QEMU TCG. La limite reste
     * finie, mais couvre une fenêtre d’ordonnancement matérielle effective. */
    while (timer_get_ticks() == before && spins < 20000000U) {
#ifdef __i386__
        __asm__ volatile ("pause" : : : "memory");
#endif
        ++spins;
    }
}

int ne2k_tx_udp(ne2k_device_t* device, const ne2k_io_t* io,
                uint8_t* frame, uint16_t frame_capacity,
                const uint8_t destination_mac[6],
                const uint8_t source_ipv4[4], const uint8_t destination_ipv4[4],
                uint16_t source_port, uint16_t destination_port,
                const uint8_t* payload, uint16_t payload_length) {
    uint16_t ip_length;
    uint32_t total_length;
    uint8_t i;
    if (!device || !io || !frame || !destination_mac || !source_ipv4 ||
        !destination_ipv4 || (!payload && payload_length != 0U) ||
        !device->mac_valid) return -1;
    total_length = NET_ETHERNET_HEADER_SIZE + NET_IPV4_HEADER_SIZE +
                   NET_UDP_HEADER_SIZE + payload_length;
    if (total_length > frame_capacity || total_length > NE2K_ETHERNET_MAX_FRAME)
        return -2;
    for (i = 0; i < 6U; ++i) { frame[i] = destination_mac[i]; frame[6U + i] = device->mac[i]; }
    frame[12] = (uint8_t)(NET_ETHERTYPE_IPV4 >> 8);
    frame[13] = (uint8_t)NET_ETHERTYPE_IPV4;
    ip_length = (uint16_t)net_udp_build_ipv4(frame + NET_ETHERNET_HEADER_SIZE,
                                             frame_capacity - NET_ETHERNET_HEADER_SIZE,
                                             source_ipv4, destination_ipv4,
                                             source_port, destination_port,
                                             payload, payload_length);
    if (ip_length == 0U) return -3;
    return ne2k_tx_submit(device, io, frame, (uint16_t)(NET_ETHERNET_HEADER_SIZE + ip_length));
}

int ne2k_arp_resolve(ne2k_device_t* device, const ne2k_io_t* io,
                     net_arp_cache_t* cache,
                     uint8_t* request_frame, uint16_t request_capacity,
                     uint8_t* rx_frame, uint16_t rx_capacity,
                     const uint8_t local_mac[6], const uint8_t local_ipv4[4],
                     const uint8_t target_ipv4[4], uint16_t attempts) {
    uint8_t destination_mac[6]; uint16_t request_length, rx_length, i;
    net_ethernet_header_t ethernet; net_arp_packet_t arp; int status;
    if (!device || !io || !cache || !request_frame || !rx_frame || !local_mac ||
        !local_ipv4 || !target_ipv4 || attempts == 0U) return -1;
    if (net_arp_cache_lookup(cache, target_ipv4, destination_mac) == 0) return 0;
    request_length = (uint16_t)net_arp_build_request(request_frame, request_capacity,
                                                      local_mac, local_ipv4, target_ipv4);
    if (request_length == 0U) return -2;
    status = ne2k_tx_submit(device, io, request_frame, request_length);
    if (status != 0) return -3;
    for (i = 0; i < attempts; ++i) {
        status = ne2k_rx_poll_arp(device, io, rx_frame, rx_capacity, &rx_length,
                                  &ethernet, &arp);
        if (status == 0 && net_arp_is_reply_for(&arp, local_ipv4, target_ipv4)) {
            if (net_arp_cache_put(cache, target_ipv4, arp.sender_mac) != 0) return -4;
            return 0;
        }
        ne2k_poll_pause();
    }
    return -5;
}

int ne2k_tx_udp_resolve(ne2k_device_t* device, const ne2k_io_t* io,
                        net_arp_cache_t* cache,
                        uint8_t* request_frame, uint16_t request_capacity,
                        uint8_t* rx_frame, uint16_t rx_capacity,
                        uint8_t* tx_frame, uint16_t tx_capacity,
                        const uint8_t local_ipv4[4], const uint8_t target_ipv4[4],
                        uint16_t source_port, uint16_t destination_port,
                        const uint8_t* payload, uint16_t payload_length,
                        uint16_t attempts) {
    uint8_t destination_mac[6]; int status;
    status = ne2k_arp_resolve(device, io, cache, request_frame, request_capacity,
                              rx_frame, rx_capacity, device ? device->mac : 0,
                              local_ipv4, target_ipv4, attempts);
    if (status != 0) return status;
    if (net_arp_cache_lookup(cache, target_ipv4, destination_mac) != 0) return -6;
    return ne2k_tx_udp(device, io, tx_frame, tx_capacity, destination_mac,
                       local_ipv4, target_ipv4, source_port, destination_port,
                       payload, payload_length);
}

int ne2k_tx_udp_via(ne2k_device_t* device,const ne2k_io_t* io,net_arp_cache_t* cache,uint8_t* request_frame,uint16_t request_capacity,uint8_t* rx_frame,uint16_t rx_capacity,uint8_t* tx_frame,uint16_t tx_capacity,const uint8_t local_ipv4[4],const uint8_t destination_ipv4[4],const uint8_t next_hop_ipv4[4],uint16_t source_port,uint16_t destination_port,const uint8_t* payload,uint16_t payload_length,uint16_t attempts){uint8_t destination_mac[6];int status;status=ne2k_arp_resolve(device,io,cache,request_frame,request_capacity,rx_frame,rx_capacity,device?device->mac:0,local_ipv4,next_hop_ipv4,attempts);if(status!=0)return status;if(net_arp_cache_lookup(cache,next_hop_ipv4,destination_mac)!=0)return -6;return ne2k_tx_udp(device,io,tx_frame,tx_capacity,destination_mac,local_ipv4,destination_ipv4,source_port,destination_port,payload,payload_length);}

int ne2k_dhcp_discover(ne2k_device_t* device, const ne2k_io_t* io,
                       uint8_t* frame, uint16_t frame_capacity, uint32_t xid) {
    static const uint8_t broadcast_mac[6] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
    static const uint8_t zero_ip[4] = {0, 0, 0, 0};
    static const uint8_t broadcast_ip[4] = {255, 255, 255, 255};
    int payload_length;
    if (!device || !io || !frame ||
        frame_capacity < NET_ETHERNET_HEADER_SIZE + NET_IPV4_HEADER_SIZE +
                          NET_UDP_HEADER_SIZE + 244U)
        return -1;
    payload_length = net_dhcp_build_discover(frame + NET_ETHERNET_HEADER_SIZE +
                                              NET_IPV4_HEADER_SIZE + NET_UDP_HEADER_SIZE,
                                              frame_capacity - NET_ETHERNET_HEADER_SIZE -
                                              NET_IPV4_HEADER_SIZE - NET_UDP_HEADER_SIZE,
                                              xid, device->mac);
    if (payload_length < 0) return -2;
    return ne2k_tx_udp(device, io, frame, frame_capacity, broadcast_mac,
                       zero_ip, broadcast_ip, 68U, 67U,
                       frame + NET_ETHERNET_HEADER_SIZE + NET_IPV4_HEADER_SIZE +
                       NET_UDP_HEADER_SIZE, (uint16_t)payload_length);
}

int ne2k_dhcp_request(ne2k_device_t* device, const ne2k_io_t* io,
                      uint8_t* frame, uint16_t frame_capacity,
                      uint32_t xid, const uint8_t requested_ip[4],
                      const uint8_t server_ip[4]) {
    static const uint8_t broadcast_mac[6] = {0xff,0xff,0xff,0xff,0xff,0xff};
    static const uint8_t zero_ip[4] = {0,0,0,0};
    static const uint8_t broadcast_ip[4] = {255,255,255,255};
    int payload_length;
    if (!device || !io || !frame || !requested_ip || !server_ip ||
        frame_capacity < NET_ETHERNET_HEADER_SIZE + NET_IPV4_HEADER_SIZE + NET_UDP_HEADER_SIZE + 255U)
        return -1;
    payload_length = net_dhcp_build_request(frame + NET_ETHERNET_HEADER_SIZE + NET_IPV4_HEADER_SIZE + NET_UDP_HEADER_SIZE,
        frame_capacity - NET_ETHERNET_HEADER_SIZE - NET_IPV4_HEADER_SIZE - NET_UDP_HEADER_SIZE,
        xid, device->mac, requested_ip, server_ip);
    if (payload_length < 0) return -2;
    return ne2k_tx_udp(device, io, frame, frame_capacity, broadcast_mac, zero_ip, broadcast_ip,
                       68U, 67U, frame + NET_ETHERNET_HEADER_SIZE + NET_IPV4_HEADER_SIZE + NET_UDP_HEADER_SIZE,
                       (uint16_t)payload_length);
}

int ne2k_dns_query_via(ne2k_device_t* device,const ne2k_io_t* io,net_arp_cache_t* cache,uint8_t* arp_request,uint16_t arp_request_capacity,uint8_t* arp_rx,uint16_t arp_rx_capacity,uint8_t* frame,uint16_t frame_capacity,const uint8_t local_ip[4],const uint8_t dns_ip[4],const uint8_t next_hop_ip[4],uint16_t id,const char* hostname){int payload_length;if(!device||!io||!cache||!arp_request||!arp_rx||!frame||!local_ip||!dns_ip||!next_hop_ip||!hostname||frame_capacity<NET_ETHERNET_HEADER_SIZE+NET_IPV4_HEADER_SIZE+NET_UDP_HEADER_SIZE+12U)return -1;payload_length=net_dns_build_a_query(frame+NET_ETHERNET_HEADER_SIZE+NET_IPV4_HEADER_SIZE+NET_UDP_HEADER_SIZE,frame_capacity-NET_ETHERNET_HEADER_SIZE-NET_IPV4_HEADER_SIZE-NET_UDP_HEADER_SIZE,id,hostname);if(payload_length<0)return -2;return ne2k_tx_udp_via(device,io,cache,arp_request,arp_request_capacity,arp_rx,arp_rx_capacity,frame,frame_capacity,local_ip,dns_ip,next_hop_ip,49152U,53U,frame+NET_ETHERNET_HEADER_SIZE+NET_IPV4_HEADER_SIZE+NET_UDP_HEADER_SIZE,(uint16_t)payload_length,8U);}

int ne2k_dns_query(ne2k_device_t* device,const ne2k_io_t* io,net_arp_cache_t* cache,uint8_t* arp_request,uint16_t arp_request_capacity,uint8_t* arp_rx,uint16_t arp_rx_capacity,uint8_t* frame,uint16_t frame_capacity,const uint8_t local_ip[4],const uint8_t dns_ip[4],uint16_t id,const char* hostname){return ne2k_dns_query_via(device,io,cache,arp_request,arp_request_capacity,arp_rx,arp_rx_capacity,frame,frame_capacity,local_ip,dns_ip,dns_ip,id,hostname);}

int ne2k_tcp_syn(ne2k_device_t* device, const ne2k_io_t* io,
                 net_arp_cache_t* cache, uint8_t* arp_request, uint16_t arp_request_capacity,
                 uint8_t* arp_rx, uint16_t arp_rx_capacity, uint8_t* frame, uint16_t frame_capacity,
                 const uint8_t local_ip[4], const uint8_t remote_ip[4],
                 uint16_t local_port, uint16_t remote_port, uint32_t sequence) {
    int tcp_length;
    if (!device || !io || !cache || !arp_request || !arp_rx || !frame || !local_ip || !remote_ip ||
        frame_capacity < NET_ETHERNET_HEADER_SIZE + 40U) return -1;
    tcp_length = net_tcp_build_syn_ipv4(frame + NET_ETHERNET_HEADER_SIZE,
        frame_capacity - NET_ETHERNET_HEADER_SIZE, local_ip, remote_ip, local_port, remote_port, sequence);
    if (tcp_length < 0) return -2;
    { uint8_t destination_mac[6]; uint16_t i;
      if (net_arp_cache_lookup(cache, remote_ip, destination_mac) != 0) return -3;
      for (i = 0; i < 6U; ++i) { frame[i] = destination_mac[i]; frame[6U+i] = device->mac[i]; }
      frame[12] = 0x08U; frame[13] = 0x00U;
      return ne2k_tx_submit(device, io, frame, (uint16_t)(NET_ETHERNET_HEADER_SIZE + tcp_length)); }
}

int ne2k_tcp_syn_ack_via(ne2k_device_t* device,const ne2k_io_t* io,net_arp_cache_t* cache,uint8_t* arp_request,uint16_t arp_request_capacity,uint8_t* arp_rx,uint16_t arp_rx_capacity,uint8_t* frame,uint16_t frame_capacity,const uint8_t local_ip[4],const uint8_t remote_ip[4],const uint8_t next_hop_ip[4],uint16_t local_port,uint16_t remote_port,uint32_t sequence,uint32_t acknowledgment,uint16_t attempts){int tcp_length,status;uint8_t destination_mac[6],i;if(!device||!io||!cache||!arp_request||!arp_rx||!frame||!local_ip||!remote_ip||!next_hop_ip||attempts==0U||frame_capacity<NET_ETHERNET_HEADER_SIZE+40U)return -1;tcp_length=net_tcp_build_syn_ack_ipv4(frame+NET_ETHERNET_HEADER_SIZE,frame_capacity-NET_ETHERNET_HEADER_SIZE,local_ip,remote_ip,local_port,remote_port,sequence,acknowledgment);if(tcp_length<0)return -2;status=ne2k_arp_resolve(device,io,cache,arp_request,arp_request_capacity,arp_rx,arp_rx_capacity,device->mac,local_ip,next_hop_ip,attempts);if(status!=0)return -3;if(net_arp_cache_lookup(cache,next_hop_ip,destination_mac)!=0)return -4;for(i=0U;i<6U;i++){frame[i]=destination_mac[i];frame[6U+i]=device->mac[i];}frame[12]=0x08U;frame[13]=0x00U;return ne2k_tx_submit(device,io,frame,(uint16_t)(NET_ETHERNET_HEADER_SIZE+tcp_length));}

int ne2k_tcp_syn_via(ne2k_device_t* device,const ne2k_io_t* io,net_arp_cache_t* cache,uint8_t* arp_request,uint16_t arp_request_capacity,uint8_t* arp_rx,uint16_t arp_rx_capacity,uint8_t* frame,uint16_t frame_capacity,const uint8_t local_ip[4],const uint8_t remote_ip[4],const uint8_t next_hop_ip[4],uint16_t local_port,uint16_t remote_port,uint32_t sequence,uint16_t attempts){int tcp_length,status;uint8_t destination_mac[6],i;if(!device||!io||!cache||!arp_request||!arp_rx||!frame||!local_ip||!remote_ip||!next_hop_ip||attempts==0U||frame_capacity<NET_ETHERNET_HEADER_SIZE+40U)return -1;tcp_length=net_tcp_build_syn_ipv4(frame+NET_ETHERNET_HEADER_SIZE,frame_capacity-NET_ETHERNET_HEADER_SIZE,local_ip,remote_ip,local_port,remote_port,sequence);if(tcp_length<0)return -2;status=ne2k_arp_resolve(device,io,cache,arp_request,arp_request_capacity,arp_rx,arp_rx_capacity,device->mac,local_ip,next_hop_ip,attempts);if(status!=0)return -3;if(net_arp_cache_lookup(cache,next_hop_ip,destination_mac)!=0)return -4;for(i=0U;i<6U;i++){frame[i]=destination_mac[i];frame[6U+i]=device->mac[i];}frame[12]=0x08U;frame[13]=0x00U;return ne2k_tx_submit(device,io,frame,(uint16_t)(NET_ETHERNET_HEADER_SIZE+tcp_length));}

int ne2k_llm_dns_syn_bootstrap(ne2k_device_t* device,const ne2k_io_t* io,net_arp_cache_t* cache,uint8_t* arp_request,uint16_t arp_request_capacity,uint8_t* arp_rx,uint16_t arp_rx_capacity,uint8_t* frame,uint16_t frame_capacity,const uint8_t local_ip[4],const uint8_t dns_ip[4],uint16_t dns_id,const char* hostname,uint16_t dns_attempts,uint16_t arp_attempts,uint16_t local_port,uint16_t remote_port,uint32_t local_sequence,uint8_t remote_ip[4],net_tcp_connection_t* connection){net_dns_a_result_t result;net_tcp_connection_t next_connection;uint8_t next_remote_ip[4];uint8_t index;int status;if(!device||!io||!cache||!arp_request||!arp_rx||!frame||!local_ip||!dns_ip||!hostname||!remote_ip||!connection||dns_attempts==0U||arp_attempts==0U)return -1;status=ne2k_dns_query(device,io,cache,arp_request,arp_request_capacity,arp_rx,arp_rx_capacity,frame,frame_capacity,local_ip,dns_ip,dns_id,hostname);if(status<0)return -2;status=ne2k_dns_poll_a(device,io,frame,frame_capacity,dns_attempts,dns_id,&result);if(status!=0)return -3;for(index=0U;index<4U;index++)next_remote_ip[index]=result.address[index];status=ne2k_arp_resolve(device,io,cache,arp_request,arp_request_capacity,arp_rx,arp_rx_capacity,device->mac,local_ip,next_remote_ip,arp_attempts);if(status!=0)return -4;if(net_tcp_connection_open(&next_connection,local_port,remote_port,local_sequence)!=0)return -5;status=ne2k_tcp_syn(device,io,cache,arp_request,arp_request_capacity,arp_rx,arp_rx_capacity,frame,frame_capacity,local_ip,next_remote_ip,local_port,remote_port,local_sequence);if(status<0)return -6;for(index=0U;index<4U;index++)remote_ip[index]=next_remote_ip[index];*connection=next_connection;return 0;}

int ne2k_llm_connection_state_init(ne2k_llm_connection_state_t* state){uint8_t index;if(!state)return -1;for(index=0U;index<4U;index++)state->remote_ip[index]=0U;state->phase=NE2K_LLM_CONNECTION_IDLE;return 0;}
int ne2k_llm_dns_syn_bootstrap_dhcp(ne2k_device_t* device,const ne2k_io_t* io,net_arp_cache_t* cache,uint8_t* arp_request,uint16_t arp_request_capacity,uint8_t* arp_rx,uint16_t arp_rx_capacity,uint8_t* frame,uint16_t frame_capacity,const net_dhcp_lease_t* lease,uint16_t dns_id,const char* hostname,uint16_t dns_attempts,uint16_t arp_attempts,uint16_t local_port,uint16_t remote_port,uint32_t local_sequence,uint8_t remote_ip[4],net_tcp_connection_t* connection){net_dns_a_result_t result;net_tcp_connection_t next_connection;uint8_t next_remote_ip[4],dns_hop[4],remote_hop[4],remote_mac[6],index;int status;if(!device||!io||!cache||!arp_request||!arp_rx||!frame||!lease||!lease->valid||!lease->dns_valid||!hostname||!remote_ip||!connection||dns_attempts==0U||arp_attempts==0U)return -1;status=net_dhcp_lease_next_hop(lease,lease->dns_ipv4,dns_hop);if(status!=0)return -2;status=ne2k_dns_query_via(device,io,cache,arp_request,arp_request_capacity,arp_rx,arp_rx_capacity,frame,frame_capacity,lease->ipv4,lease->dns_ipv4,dns_hop,dns_id,hostname);if(status<0)return -3;status=ne2k_dns_poll_a(device,io,frame,frame_capacity,dns_attempts,dns_id,&result);if(status!=0)return -4;for(index=0U;index<4U;index++)next_remote_ip[index]=result.address[index];status=net_dhcp_lease_next_hop(lease,next_remote_ip,remote_hop);if(status!=0)return -5;if(net_tcp_connection_open(&next_connection,local_port,remote_port,local_sequence)!=0)return -6;status=ne2k_tcp_syn_via(device,io,cache,arp_request,arp_request_capacity,arp_rx,arp_rx_capacity,frame,frame_capacity,lease->ipv4,next_remote_ip,remote_hop,local_port,remote_port,local_sequence,arp_attempts);if(status<0)return -7;/* Les phases TLS/HTTP adressent l’IP logique distante. Associer cette clé
 * au voisin déjà résolu du prochain saut conserve l’IPv4 de destination et
 * évite un ARP erroné vers un hôte situé derrière le routeur DHCP. */
if(net_arp_cache_lookup(cache,remote_hop,remote_mac)!=0 ||
   net_arp_cache_put(cache,next_remote_ip,remote_mac)!=0) return -8;
for(index=0U;index<4U;index++) remote_ip[index]=next_remote_ip[index];
*connection=next_connection;
return 0;}

int ne2k_llm_connection_start_dhcp(ne2k_device_t* device,const ne2k_io_t* io,net_arp_cache_t* cache,uint8_t* arp_request,uint16_t arp_request_capacity,uint8_t* arp_rx,uint16_t arp_rx_capacity,uint8_t* frame,uint16_t frame_capacity,const net_dhcp_lease_t* lease,uint16_t dns_id,const char* hostname,uint16_t dns_attempts,uint16_t arp_attempts,uint16_t local_port,uint16_t remote_port,uint32_t local_sequence,ne2k_llm_connection_state_t* state,net_tcp_connection_t* connection){ne2k_llm_connection_state_t next_state;net_tcp_connection_t next_connection;int status;if(!state||!connection)return -1;if(state->phase!=NE2K_LLM_CONNECTION_IDLE)return -2;next_state=*state;next_connection=*connection;status=ne2k_llm_dns_syn_bootstrap_dhcp(device,io,cache,arp_request,arp_request_capacity,arp_rx,arp_rx_capacity,frame,frame_capacity,lease,dns_id,hostname,dns_attempts,arp_attempts,local_port,remote_port,local_sequence,next_state.remote_ip,&next_connection);if(status!=0)return -3;next_state.phase=NE2K_LLM_CONNECTION_SYN_SENT;*state=next_state;*connection=next_connection;return 0;}

int ne2k_llm_connection_acquire_start_dhcp(ne2k_device_t* device,const ne2k_io_t* io,net_arp_cache_t* cache,uint8_t* dhcp_tx,uint16_t dhcp_tx_capacity,uint8_t* dhcp_rx,uint16_t dhcp_rx_capacity,uint32_t xid,uint16_t dhcp_attempts,uint8_t* arp_request,uint16_t arp_request_capacity,uint8_t* arp_rx,uint16_t arp_rx_capacity,uint8_t* frame,uint16_t frame_capacity,uint16_t dns_id,const char* hostname,uint16_t dns_attempts,uint16_t arp_attempts,uint16_t local_port,uint16_t remote_port,uint32_t local_sequence,net_dhcp_lease_t* lease,ne2k_llm_connection_state_t* state,net_tcp_connection_t* connection){net_dhcp_lease_t next_lease;ne2k_llm_connection_state_t next_state;net_tcp_connection_t next_connection;int status;if(!lease||!state||!connection)return -1;if(state->phase!=NE2K_LLM_CONNECTION_IDLE)return -2;next_state=*state;next_connection=*connection;status=ne2k_dhcp_acquire(device,io,dhcp_tx,dhcp_tx_capacity,dhcp_rx,dhcp_rx_capacity,xid,dhcp_attempts,&next_lease);if(status!=0)return -3;status=ne2k_llm_connection_start_dhcp(device,io,cache,arp_request,arp_request_capacity,arp_rx,arp_rx_capacity,frame,frame_capacity,&next_lease,dns_id,hostname,dns_attempts,arp_attempts,local_port,remote_port,local_sequence,&next_state,&next_connection);if(status!=0)return -4;*lease=next_lease;*state=next_state;*connection=next_connection;return 0;}

int ne2k_llm_network_context_init(ne2k_llm_network_context_t* context){net_tcp_connection_t empty={0};if(!context)return -1;net_dhcp_lease_clear(&context->lease);if(ne2k_llm_connection_state_init(&context->session)!=0)return -2;context->connection=empty;ne2k_llm_network_context_sse_resume_clear(context);return 0;}
int ne2k_llm_network_context_reset_for_request(ne2k_llm_network_context_t* context){if(!context)return -1;return ne2k_llm_connection_reset_for_request(&context->session);}
int ne2k_llm_network_context_reconcile_lease(ne2k_llm_network_context_t* context,const net_dhcp_lease_t* lease){ne2k_llm_network_context_t next;net_tcp_connection_t empty={0};uint8_t index,changed=0U;if(!context||!lease)return -1;next=*context;if(context->lease.valid!=lease->valid||context->lease.subnet_valid!=lease->subnet_valid||context->lease.router_valid!=lease->router_valid||context->lease.dns_valid!=lease->dns_valid)changed=1U;for(index=0U;index<4U;index++)if(context->lease.ipv4[index]!=lease->ipv4[index]||context->lease.server_ipv4[index]!=lease->server_ipv4[index]||context->lease.subnet_mask[index]!=lease->subnet_mask[index]||context->lease.router_ipv4[index]!=lease->router_ipv4[index]||context->lease.dns_ipv4[index]!=lease->dns_ipv4[index])changed=1U;next.lease=*lease;if(changed){if(ne2k_llm_connection_state_init(&next.session)!=0)return -2;next.connection=empty;}*context=next;return changed?1:0;}
int ne2k_llm_network_context_dhcp_renew_if_due(ne2k_llm_network_context_t* context,ne2k_device_t* device,const ne2k_io_t* io,uint8_t* tx_frame,uint16_t tx_capacity,uint8_t* rx_frame,uint16_t rx_capacity,uint32_t xid,uint16_t poll_attempts,uint32_t now){net_dhcp_lease_t next_lease;int status;if(!context)return -1;next_lease=context->lease;status=ne2k_dhcp_renew_if_due(device,io,tx_frame,tx_capacity,rx_frame,rx_capacity,xid,poll_attempts,now,&next_lease);if(status<0)return status;if(status==1)return ne2k_llm_network_context_reconcile_lease(context,&next_lease);return status;}
int ne2k_llm_network_context_sse_checkpoint(ne2k_llm_network_context_t* context,uint8_t provider,uint8_t retries_used,const net_llm_sse_response_t* response){net_llm_sse_persisted_state_t next;int status;if(!context)return -1;next=context->sse_resume;status=net_llm_sse_persist_save(&next,provider,retries_used,response);if(status!=0)return status;context->sse_resume=next;context->sse_resume_valid=1U;return 0;}
int ne2k_llm_network_context_sse_resume_load(const ne2k_llm_network_context_t* context,uint8_t* provider,uint8_t* retries_used,net_llm_sse_response_t* response){net_llm_sse_response_t next_response;uint8_t next_provider,next_retries;int status;if(!context||!provider||!retries_used||!response)return -1;if(!context->sse_resume_valid)return -2;next_response=*response;next_provider=*provider;next_retries=*retries_used;status=net_llm_sse_persist_load(&context->sse_resume,&next_provider,&next_retries,&next_response);if(status!=0)return status;*response=next_response;*provider=next_provider;*retries_used=next_retries;return 0;}
void ne2k_llm_network_context_sse_resume_clear(ne2k_llm_network_context_t* context){uint8_t index;if(!context)return;for(index=0U;index<(uint8_t)sizeof(context->sse_resume);index++)((uint8_t*)&context->sse_resume)[index]=0U;context->sse_resume_valid=0U;}


int ne2k_llm_network_context_acquire_start_dhcp(ne2k_device_t* device,const ne2k_io_t* io,net_arp_cache_t* cache,uint8_t* dhcp_tx,uint16_t dhcp_tx_capacity,uint8_t* dhcp_rx,uint16_t dhcp_rx_capacity,uint32_t xid,uint16_t dhcp_attempts,uint8_t* arp_request,uint16_t arp_request_capacity,uint8_t* arp_rx,uint16_t arp_rx_capacity,uint8_t* frame,uint16_t frame_capacity,uint16_t dns_id,const char* hostname,uint16_t dns_attempts,uint16_t arp_attempts,uint16_t local_port,uint16_t remote_port,uint32_t local_sequence,ne2k_llm_network_context_t* context){if(!context)return -1;return ne2k_llm_connection_acquire_start_dhcp(device,io,cache,dhcp_tx,dhcp_tx_capacity,dhcp_rx,dhcp_rx_capacity,xid,dhcp_attempts,arp_request,arp_request_capacity,arp_rx,arp_rx_capacity,frame,frame_capacity,dns_id,hostname,dns_attempts,arp_attempts,local_port,remote_port,local_sequence,&context->lease,&context->session,&context->connection);}

int ne2k_llm_socket_session_init(ne2k_llm_socket_session_t* session) {
    if (!session) return -1;
    if (ne2k_llm_connection_state_init(&session->state) != 0) return -2;
    session->socket_id = -1;
    return 0;
}

int ne2k_llm_socket_session_start(ne2k_device_t* device,const ne2k_io_t* io,net_arp_cache_t* cache,
                                  uint8_t* arp_request,uint16_t arp_request_capacity,uint8_t* arp_rx,uint16_t arp_rx_capacity,
                                  uint8_t* frame,uint16_t frame_capacity,const uint8_t local_ip[4],const uint8_t dns_ip[4],
                                  uint16_t dns_id,const char* hostname,uint16_t dns_attempts,uint16_t arp_attempts,
                                  uint16_t local_port,uint16_t remote_port,uint32_t local_sequence,
                                  ne2k_llm_socket_session_t* session) {
    net_tcp_connection_t scratch_connection; uint8_t remote_ip[4]; int socket_id, status;
    if (!session || session->state.phase != NE2K_LLM_CONNECTION_IDLE || session->socket_id >= 0) return -1;
    socket_id = net_socket_open(local_port, remote_port, local_sequence);
    if (socket_id < 0) return -2;
    status = ne2k_llm_dns_syn_bootstrap(device, io, cache, arp_request, arp_request_capacity, arp_rx,
                                        arp_rx_capacity, frame, frame_capacity, local_ip, dns_ip, dns_id,
                                        hostname, dns_attempts, arp_attempts, local_port, remote_port,
                                        local_sequence, remote_ip, &scratch_connection);
    if (status != 0) { (void)net_socket_close(socket_id); return -3; }
    if (ne2k_llm_socket_session_attach(session, socket_id, remote_ip) != 0) {
        (void)net_socket_close(socket_id); return -4;
    }
    return 0;
}

int ne2k_llm_socket_session_start_dhcp(ne2k_device_t* device,const ne2k_io_t* io,net_arp_cache_t* cache,
                                       uint8_t* arp_request,uint16_t arp_request_capacity,uint8_t* arp_rx,uint16_t arp_rx_capacity,
                                       uint8_t* frame,uint16_t frame_capacity,const net_dhcp_lease_t* lease,
                                       uint16_t dns_id,const char* hostname,uint16_t dns_attempts,uint16_t arp_attempts,
                                       uint16_t local_port,uint16_t remote_port,uint32_t local_sequence,
                                       ne2k_llm_socket_session_t* session) {
    net_tcp_connection_t scratch_connection; uint8_t remote_ip[4]; int socket_id, status;
    if (!session || session->state.phase != NE2K_LLM_CONNECTION_IDLE || session->socket_id >= 0) return -1;
    socket_id = net_socket_open(local_port, remote_port, local_sequence);
    if (socket_id < 0) return -2;
    status = ne2k_llm_dns_syn_bootstrap_dhcp(device, io, cache, arp_request, arp_request_capacity, arp_rx,
                                             arp_rx_capacity, frame, frame_capacity, lease, dns_id, hostname,
                                             dns_attempts, arp_attempts, local_port, remote_port, local_sequence,
                                             remote_ip, &scratch_connection);
    if (status != 0) { (void)net_socket_close(socket_id); return -3; }
    if (ne2k_llm_socket_session_attach(session, socket_id, remote_ip) != 0) {
        (void)net_socket_close(socket_id); return -4;
    }
    return 0;
}

int ne2k_llm_socket_session_acquire_start_dhcp(ne2k_device_t* device,const ne2k_io_t* io,net_arp_cache_t* cache,
                                               uint8_t* dhcp_tx,uint16_t dhcp_tx_capacity,uint8_t* dhcp_rx,uint16_t dhcp_rx_capacity,
                                               uint32_t xid,uint16_t dhcp_attempts,uint8_t* arp_request,uint16_t arp_request_capacity,
                                               uint8_t* arp_rx,uint16_t arp_rx_capacity,uint8_t* frame,uint16_t frame_capacity,
                                               uint16_t dns_id,const char* hostname,uint16_t dns_attempts,uint16_t arp_attempts,
                                               uint16_t local_port,uint16_t remote_port,uint32_t local_sequence,
                                               net_dhcp_lease_t* lease,ne2k_llm_socket_session_t* session) {
    net_dhcp_lease_t next_lease; int status;
    if (!lease || !session || session->state.phase != NE2K_LLM_CONNECTION_IDLE || session->socket_id >= 0) return -1;
    status = ne2k_dhcp_acquire(device, io, dhcp_tx, dhcp_tx_capacity, dhcp_rx, dhcp_rx_capacity,
                               xid, dhcp_attempts, &next_lease);
    if (status != 0) return status;
    status = ne2k_llm_socket_session_start_dhcp(device, io, cache, arp_request, arp_request_capacity,
                                                 arp_rx, arp_rx_capacity, frame, frame_capacity, &next_lease,
                                                 dns_id, hostname, dns_attempts, arp_attempts, local_port,
                                                 remote_port, local_sequence, session);
    if (status != 0) return -3;
    *lease = next_lease;
    return 0;
}

int ne2k_llm_socket_session_attach(ne2k_llm_socket_session_t* session, int socket_id,
                                   const uint8_t remote_ip[4]) {
    uint8_t socket_state; uint8_t i;
    if (!session || !remote_ip || socket_id < 0) return -1;
    if (session->state.phase != NE2K_LLM_CONNECTION_IDLE ||
        net_socket_get_state(socket_id, &socket_state) != 0 ||
        socket_state != NET_TCP_STATE_SYN_SENT) return -2;
    for (i = 0U; i < 4U; i++) session->state.remote_ip[i] = remote_ip[i];
    session->socket_id = socket_id; session->state.phase = NE2K_LLM_CONNECTION_SYN_SENT;
    return 0;
}

int ne2k_llm_socket_session_poll_tls_start(ne2k_device_t* device,const ne2k_io_t* io,const net_arp_cache_t* cache,
                                            uint8_t* rx_frame,uint16_t rx_capacity,uint8_t* tx_frame,uint16_t tx_capacity,
                                            const uint8_t local_ip[4],ne2k_llm_socket_session_t* session,
                                            ne2k_tls_client_t* client,const uint8_t client_random[32],
                                            uint8_t* client_hello_record,uint32_t client_hello_capacity,
                                            uint8_t* tcp_segment,uint16_t tcp_segment_capacity,uint8_t retransmit_limit) {
    ne2k_llm_socket_session_t previous_session; ne2k_tls_client_t previous_client;
    net_tcp_view_t syn_ack; uint16_t frame_length = 0U; int status;
    if (!session || !client) return -1;
    if (session->state.phase != NE2K_LLM_CONNECTION_SYN_SENT || session->socket_id < 0) return -2;
    status = ne2k_rx_poll_tcp(device, io, rx_frame, rx_capacity, &frame_length, &syn_ack);
    if (status != 0) return status;
    previous_session = *session; previous_client = *client;
    status = ne2k_socket_tls_accept_syn_ack_start(device, io, cache, tx_frame, tx_capacity, local_ip,
                                                   session->state.remote_ip, session->socket_id, &syn_ack, client,
                                                   client_random, client_hello_record, client_hello_capacity,
                                                   tcp_segment, tcp_segment_capacity, retransmit_limit);
    if (status < 0) { *session = previous_session; *client = previous_client; return -3; }
    session->state.phase = NE2K_LLM_CONNECTION_TLS_STARTED;
    return status;
}

int ne2k_llm_socket_session_poll_tls(ne2k_device_t* device,const ne2k_io_t* io,const net_arp_cache_t* cache,
                                      uint8_t* rx_frame,uint16_t rx_capacity,uint8_t* tx_frame,uint16_t tx_capacity,
                                      const uint8_t local_ip[4],ne2k_llm_socket_session_t* session,
                                      ne2k_tls_client_t* client,const uint8_t client_random[32],
                                      const uint8_t client_private[NET_TLS_X25519_KEY_LENGTH],
                                      const x509_certificate_view_t* trust_anchor,const char* hostname,const char* utc_time,
                                      uint32_t* rsa_workspace,uint16_t rsa_workspace_length,
                                      uint32_t* x25519_workspace,uint16_t x25519_workspace_length,
                                      uint8_t* prf_workspace,uint32_t prf_workspace_capacity,
                                      uint8_t* tcp_segment,uint32_t tcp_segment_capacity,
                                      uint8_t* flight_records,uint32_t flight_records_capacity,uint32_t* flight_records_length,
                                      uint8_t* plaintext,uint16_t plaintext_capacity,uint8_t retransmit_limit,uint16_t* consumed) {
    static ne2k_llm_socket_session_t previous_session;
    static ne2k_tls_client_t previous_client;
    int status;
    if (!session || !client) return -1;
    if (session->state.phase != NE2K_LLM_CONNECTION_TLS_STARTED || session->socket_id < 0) return -2;
    previous_session = *session; previous_client = *client;
    status = ne2k_socket_tls_poll(device, io, cache, rx_frame, rx_capacity, tx_frame, tx_capacity, local_ip,
                                  session->state.remote_ip, session->socket_id, client, client_random,
                                  client_private, trust_anchor, hostname, utc_time, rsa_workspace,
                                  rsa_workspace_length, x25519_workspace, x25519_workspace_length,
                                  prf_workspace, prf_workspace_capacity, tcp_segment, tcp_segment_capacity,
                                  flight_records, flight_records_capacity, flight_records_length, plaintext,
                                  plaintext_capacity, retransmit_limit, consumed);
    if (status < 0) { *session = previous_session; *client = previous_client; return status; }
    if (client->complete && net_tls_handshake_is_complete(&client->handshake))
        session->state.phase = NE2K_LLM_CONNECTION_TLS_COMPLETE;
    return status;
}

int ne2k_llm_socket_session_request(ne2k_device_t* device,const ne2k_io_t* io,const net_arp_cache_t* cache,
                                    uint8_t* tx_frame,uint16_t tx_capacity,const uint8_t local_ip[4],
                                    ne2k_llm_socket_session_t* session,net_tls_aes_gcm_session_t* tls_session,
                                    uint8_t provider,uint8_t stream,uint8_t* json,uint16_t json_capacity,
                                    uint8_t* request,uint16_t request_capacity,const char* host,const char* path,
                                    const char* bearer_token,const char* model,const uint8_t* prompt,uint16_t prompt_length,
                                    uint8_t* tls_record,uint32_t tls_capacity,uint8_t* tcp_segment,
                                    uint16_t tcp_segment_capacity,uint8_t retransmit_limit) {
    ne2k_llm_socket_session_t previous_session; int status;
    if (!session || !tls_session) return -1;
    if (session->state.phase != NE2K_LLM_CONNECTION_TLS_COMPLETE || session->socket_id < 0) return -2;
    previous_session = *session;
    status = ne2k_socket_llm_request(device, io, cache, tx_frame, tx_capacity, local_ip,
                                     session->state.remote_ip, session->socket_id, tls_session, provider, stream,
                                     json, json_capacity, request, request_capacity, host, path, bearer_token,
                                     model, prompt, prompt_length, tls_record, tls_capacity, tcp_segment,
                                     tcp_segment_capacity, retransmit_limit);
    if (status < 0) { *session = previous_session; return status; }
        session->state.phase = NE2K_LLM_CONNECTION_REQUEST_SENT;
    return status;
}
int ne2k_llm_socket_session_resume_sse(ne2k_device_t* device,const ne2k_io_t* io,const net_arp_cache_t* cache,
                                       uint8_t* tx_frame,uint16_t tx_capacity,const uint8_t local_ip[4],
                                       ne2k_llm_socket_session_t* session,net_tls_aes_gcm_session_t* tls_session,
                                       uint8_t* request,uint16_t request_capacity,const char* host,const char* path,
                                       const net_llm_sse_response_t* response,uint8_t* tls_record,uint32_t tls_capacity,
                                       uint8_t* tcp_segment,uint16_t tcp_segment_capacity,uint8_t retransmit_limit) {
    ne2k_llm_socket_session_t previous_session;
    int status;
    if (!session || !tls_session || !response) return -1;
    if (session->state.phase != NE2K_LLM_CONNECTION_TLS_COMPLETE || session->socket_id < 0) return -2;
    previous_session = *session;
    status = ne2k_socket_llm_resume_sse(device, io, cache, tx_frame, tx_capacity, local_ip,
                                         session->state.remote_ip, session->socket_id, tls_session,
                                         request, request_capacity, host, path, response, tls_record,
                                         tls_capacity, tcp_segment, tcp_segment_capacity, retransmit_limit);
    if (status < 0) { *session = previous_session; return status; }
    session->state.phase = NE2K_LLM_CONNECTION_REQUEST_SENT;
    return status;
}
int ne2k_llm_socket_session_poll_response(ne2k_device_t* device,const ne2k_io_t* io,const net_arp_cache_t* cache,
                                          uint8_t* rx_frame,uint16_t rx_capacity,uint8_t* tx_frame,uint16_t tx_capacity,
                                          const uint8_t local_ip[4],ne2k_llm_socket_session_t* session,
                                          net_tls_aes_gcm_session_t* tls_session,uint8_t* plaintext,
                                          uint16_t plaintext_capacity,net_http_response_accumulator_t* accumulator,
                                          net_http_response_view_t* response,uint16_t* consumed) {
    ne2k_llm_socket_session_t previous_session; int status;
    if (!session || !tls_session) return -1;
    if (session->state.phase != NE2K_LLM_CONNECTION_REQUEST_SENT || session->socket_id < 0) return -2;
    previous_session = *session;
    status = ne2k_socket_llm_poll_response(device, io, cache, rx_frame, rx_capacity, tx_frame, tx_capacity,
                                            local_ip, session->state.remote_ip, session->socket_id, tls_session,
                                            plaintext, plaintext_capacity, accumulator, response, consumed);
    if (status < 0) { *session = previous_session; return status; }
    if (status == 0) session->state.phase = NE2K_LLM_CONNECTION_RESPONSE_READY;
    return status;
}

int ne2k_llm_socket_session_poll_sse(ne2k_device_t* device,const ne2k_io_t* io,const net_arp_cache_t* cache,
                                     uint8_t* rx_frame,uint16_t rx_capacity,uint8_t* tx_frame,uint16_t tx_capacity,
                                     const uint8_t local_ip[4],ne2k_llm_socket_session_t* session,
                                     net_tls_aes_gcm_session_t* tls_session,uint8_t* plaintext,
                                     uint16_t plaintext_capacity,net_llm_sse_response_t* response,uint8_t provider,
                                     uint8_t* text,uint16_t text_capacity,uint16_t* text_length,uint16_t* consumed) {
    ne2k_llm_socket_session_t previous_session; int status;
    if (!session || !tls_session) return -1;
    if ((session->state.phase != NE2K_LLM_CONNECTION_REQUEST_SENT &&
         session->state.phase != NE2K_LLM_CONNECTION_STREAMING) || session->socket_id < 0) return -2;
    previous_session = *session;
    status = ne2k_socket_llm_poll_sse(device, io, cache, rx_frame, rx_capacity, tx_frame, tx_capacity,
                                       local_ip, session->state.remote_ip, session->socket_id, tls_session,
                                       plaintext, plaintext_capacity, response, provider, text, text_capacity,
                                       text_length, consumed);
    if (status < 0) { *session = previous_session; return status; }
    if (status == NET_HTTP_TLS_STATUS_CLOSE_NOTIFY) {
        if (ne2k_socket_aes_gcm_close_notify(device, io, cache, tx_frame, tx_capacity,
                                             local_ip, session->state.remote_ip, session->socket_id,
                                             tls_session, plaintext, plaintext_capacity, 2U) < 0) {
            *session = previous_session;
            return -3;
        }
        session->state.phase = NE2K_LLM_CONNECTION_RESPONSE_READY;
        return 0;
    }
    session->state.phase = status == 0 ? NE2K_LLM_CONNECTION_RESPONSE_READY : NE2K_LLM_CONNECTION_STREAMING;
    return status;
}

int ne2k_llm_socket_session_reset_for_request(ne2k_llm_socket_session_t* session) {
    if (!session) return -1;
    if (session->state.phase != NE2K_LLM_CONNECTION_RESPONSE_READY || session->socket_id < 0) return -2;
    session->state.phase = NE2K_LLM_CONNECTION_TLS_COMPLETE;
    return 0;
}

int ne2k_llm_connection_start(ne2k_device_t* device,const ne2k_io_t* io,net_arp_cache_t* cache,uint8_t* arp_request,uint16_t arp_request_capacity,uint8_t* arp_rx,uint16_t arp_rx_capacity,uint8_t* frame,uint16_t frame_capacity,const uint8_t local_ip[4],const uint8_t dns_ip[4],uint16_t dns_id,const char* hostname,uint16_t dns_attempts,uint16_t arp_attempts,uint16_t local_port,uint16_t remote_port,uint32_t local_sequence,ne2k_llm_connection_state_t* state,net_tcp_connection_t* connection){ne2k_llm_connection_state_t next_state;net_tcp_connection_t next_connection;int status;if(!state||!connection)return -1;if(state->phase!=NE2K_LLM_CONNECTION_IDLE)return -2;next_state=*state;next_connection=*connection;status=ne2k_llm_dns_syn_bootstrap(device,io,cache,arp_request,arp_request_capacity,arp_rx,arp_rx_capacity,frame,frame_capacity,local_ip,dns_ip,dns_id,hostname,dns_attempts,arp_attempts,local_port,remote_port,local_sequence,next_state.remote_ip,&next_connection);if(status!=0)return -3;next_state.phase=NE2K_LLM_CONNECTION_SYN_SENT;*state=next_state;*connection=next_connection;return 0;}
int ne2k_llm_connection_poll_tls_start(ne2k_device_t* device,const ne2k_io_t* io,const net_arp_cache_t* cache,uint8_t* rx_frame,uint16_t rx_capacity,uint8_t* tx_frame,uint16_t tx_capacity,const uint8_t local_ip[4],ne2k_llm_connection_state_t* state,net_tcp_connection_t* connection,ne2k_tls_client_t* client,const uint8_t client_random[32],uint8_t* client_hello_record,uint32_t client_hello_capacity,uint8_t retransmit_limit){ne2k_llm_connection_state_t next_state;net_tcp_connection_t next_connection;ne2k_tls_client_t next_client;int status;if(!state||!connection||!client)return -1;if(state->phase!=NE2K_LLM_CONNECTION_SYN_SENT)return -2;next_state=*state;next_connection=*connection;next_client=*client;status=ne2k_llm_syn_ack_tls_start(device,io,cache,rx_frame,rx_capacity,tx_frame,tx_capacity,local_ip,next_state.remote_ip,&next_connection,&next_client,client_random,client_hello_record,client_hello_capacity,retransmit_limit);if(status!=0)return status;next_state.phase=NE2K_LLM_CONNECTION_TLS_STARTED;*state=next_state;*connection=next_connection;*client=next_client;return 0;}

int ne2k_llm_connection_poll_tls(ne2k_device_t* device,const ne2k_io_t* io,const net_arp_cache_t* cache,uint8_t* rx_frame,uint16_t rx_capacity,uint8_t* tx_frame,uint16_t tx_capacity,const uint8_t local_ip[4],ne2k_llm_connection_state_t* state,net_tcp_connection_t* connection,ne2k_tls_client_t* client,const uint8_t client_random[32],const uint8_t client_private[NET_TLS_X25519_KEY_LENGTH],const x509_certificate_view_t* trust_anchor,const char* hostname,const rtc_io_t* rtc_io,uint32_t* rsa_workspace,uint16_t rsa_workspace_length,uint32_t* x25519_workspace,uint16_t x25519_workspace_length,uint8_t* prf_workspace,uint32_t prf_workspace_capacity,uint8_t* tcp_segment,uint32_t tcp_segment_capacity,uint8_t* flight_records,uint32_t flight_records_capacity,uint32_t* flight_records_length,uint8_t* plaintext,uint16_t plaintext_capacity,uint8_t retransmit_limit,uint16_t* consumed){ne2k_llm_connection_state_t next_state;net_tcp_connection_t next_connection;ne2k_tls_client_t next_client;int status;if(!state||!connection||!client)return -1;if(state->phase!=NE2K_LLM_CONNECTION_TLS_STARTED)return -2;next_state=*state;next_connection=*connection;next_client=*client;status=ne2k_tls_client_poll_received_chain_rtc(device,io,cache,rx_frame,rx_capacity,tx_frame,tx_capacity,local_ip,next_state.remote_ip,&next_connection,&next_client,client_random,client_private,trust_anchor,hostname,rtc_io,rsa_workspace,rsa_workspace_length,x25519_workspace,x25519_workspace_length,prf_workspace,prf_workspace_capacity,tcp_segment,tcp_segment_capacity,flight_records,flight_records_capacity,flight_records_length,plaintext,plaintext_capacity,retransmit_limit,consumed);if(status<0)return status;if(next_client.complete&&net_tls_handshake_is_complete(&next_client.handshake))next_state.phase=NE2K_LLM_CONNECTION_TLS_COMPLETE;*state=next_state;*connection=next_connection;*client=next_client;return status;}

int ne2k_llm_connection_request(ne2k_device_t* device,const ne2k_io_t* io,const net_arp_cache_t* cache,uint8_t* tx_frame,uint16_t tx_capacity,const uint8_t local_ip[4],ne2k_llm_connection_state_t* state,net_tcp_connection_t* connection,ne2k_tls_client_t* client,uint8_t provider,uint8_t* json,uint16_t json_capacity,uint8_t* request,uint16_t request_capacity,const char* host,const char* path,const char* bearer_token,const char* model,const uint8_t* prompt,uint16_t prompt_length,uint8_t* tls_record,uint32_t tls_capacity,uint8_t retransmit_limit){ne2k_llm_connection_state_t next_state;net_tcp_connection_t next_connection;ne2k_tls_client_t next_client;int status;if(!state||!connection||!client)return -1;if(state->phase!=NE2K_LLM_CONNECTION_TLS_COMPLETE)return -2;next_state=*state;next_connection=*connection;next_client=*client;status=ne2k_https_llm_request(device,io,cache,tx_frame,tx_capacity,local_ip,next_state.remote_ip,&next_connection,&next_client,provider,json,json_capacity,request,request_capacity,host,path,bearer_token,model,prompt,prompt_length,tls_record,tls_capacity,retransmit_limit);if(status<0)return status;next_state.phase=NE2K_LLM_CONNECTION_REQUEST_SENT;*state=next_state;*connection=next_connection;*client=next_client;return status;}

int ne2k_llm_connection_stream_request(ne2k_device_t* device,const ne2k_io_t* io,const net_arp_cache_t* cache,uint8_t* tx_frame,uint16_t tx_capacity,const uint8_t local_ip[4],ne2k_llm_connection_state_t* state,net_tcp_connection_t* connection,ne2k_tls_client_t* client,uint8_t provider,uint8_t* json,uint16_t json_capacity,uint8_t* request,uint16_t request_capacity,const char* host,const char* path,const char* bearer_token,const char* model,const uint8_t* prompt,uint16_t prompt_length,uint8_t* tls_record,uint32_t tls_capacity,uint8_t retransmit_limit){ne2k_llm_connection_state_t next_state;net_tcp_connection_t next_connection;ne2k_tls_client_t next_client;int status;if(!state||!connection||!client)return -1;if(state->phase!=NE2K_LLM_CONNECTION_TLS_COMPLETE)return -2;next_state=*state;next_connection=*connection;next_client=*client;status=ne2k_https_llm_stream_request(device,io,cache,tx_frame,tx_capacity,local_ip,next_state.remote_ip,&next_connection,&next_client,provider,json,json_capacity,request,request_capacity,host,path,bearer_token,model,prompt,prompt_length,tls_record,tls_capacity,retransmit_limit);if(status<0)return status;next_state.phase=NE2K_LLM_CONNECTION_REQUEST_SENT;*state=next_state;*connection=next_connection;*client=next_client;return status;}

int ne2k_llm_connection_poll_text(ne2k_device_t* device,const ne2k_io_t* io,const net_arp_cache_t* cache,uint8_t* rx_frame,uint16_t rx_capacity,uint8_t* tx_frame,uint16_t tx_capacity,const uint8_t local_ip[4],ne2k_llm_connection_state_t* state,net_tcp_connection_t* connection,ne2k_tls_client_t* client,uint8_t provider,uint8_t* plaintext,uint16_t plaintext_capacity,net_http_response_accumulator_t* accumulator,net_http_response_view_t* response,uint8_t* text,uint16_t text_capacity,uint16_t* text_length,uint16_t* consumed){ne2k_llm_connection_state_t next_state;net_tcp_connection_t next_connection;ne2k_tls_client_t next_client;net_http_response_accumulator_t next_accumulator;net_http_response_view_t next_response;uint16_t next_text_length=0U,next_consumed=0U;int status;if(!state||!connection||!client||!accumulator||!response||!text||!text_length||!consumed)return -1;if(state->phase!=NE2K_LLM_CONNECTION_REQUEST_SENT)return -2;next_state=*state;next_connection=*connection;next_client=*client;next_accumulator=*accumulator;next_response=*response;status=ne2k_https_llm_poll_text(device,io,cache,rx_frame,rx_capacity,tx_frame,tx_capacity,local_ip,next_state.remote_ip,&next_connection,&next_client,provider,plaintext,plaintext_capacity,&next_accumulator,&next_response,text,text_capacity,&next_text_length,&next_consumed);if(status<0){*text_length=0U;*consumed=0U;return status;}if(status==0)next_state.phase=NE2K_LLM_CONNECTION_RESPONSE_READY;*state=next_state;*connection=next_connection;*client=next_client;*accumulator=next_accumulator;*response=next_response;*text_length=next_text_length;*consumed=next_consumed;return status;}

int ne2k_llm_connection_reset_for_request(ne2k_llm_connection_state_t* state){ne2k_llm_connection_state_t next_state;if(!state)return -1;if(state->phase!=NE2K_LLM_CONNECTION_RESPONSE_READY)return -2;next_state=*state;next_state.phase=NE2K_LLM_CONNECTION_TLS_COMPLETE;*state=next_state;return 0;}

int ne2k_llm_connection_poll_sse(ne2k_device_t* device,const ne2k_io_t* io,const net_arp_cache_t* cache,uint8_t* rx_frame,uint16_t rx_capacity,uint8_t* tx_frame,uint16_t tx_capacity,const uint8_t local_ip[4],ne2k_llm_connection_state_t* state,net_tcp_connection_t* connection,ne2k_tls_client_t* client,uint8_t provider,uint8_t* plaintext,uint16_t plaintext_capacity,net_llm_sse_response_t* response,uint8_t* text,uint16_t text_capacity,uint16_t* text_length,uint16_t* consumed){ne2k_llm_connection_state_t next_state;net_tcp_connection_t next_connection;ne2k_tls_client_t next_client;net_llm_sse_response_t next_response;uint16_t next_text_length=0U,next_consumed=0U;int status;if(!state||!connection||!client||!response||!text||!text_length||!consumed)return -1;if(state->phase!=NE2K_LLM_CONNECTION_REQUEST_SENT&&state->phase!=NE2K_LLM_CONNECTION_STREAMING)return -2;next_state=*state;next_connection=*connection;next_client=*client;next_response=*response;status=ne2k_https_llm_poll_sse(device,io,cache,rx_frame,rx_capacity,tx_frame,tx_capacity,local_ip,next_state.remote_ip,&next_connection,&next_client,provider,plaintext,plaintext_capacity,&next_response,text,text_capacity,&next_text_length,&next_consumed);if(status<0){*text_length=0U;*consumed=0U;return status;}if(status==0)next_state.phase=NE2K_LLM_CONNECTION_RESPONSE_READY;else next_state.phase=NE2K_LLM_CONNECTION_STREAMING;*state=next_state;*connection=next_connection;*client=next_client;*response=next_response;*text_length=next_text_length;*consumed=next_consumed;return status;}

int ne2k_tcp_ack(ne2k_device_t* device, const ne2k_io_t* io,
                 const net_arp_cache_t* cache, uint8_t* frame, uint16_t frame_capacity,
                 const uint8_t local_ip[4], const uint8_t remote_ip[4],
                 const net_tcp_connection_t* connection) {
    uint8_t destination_mac[6]; uint16_t tcp_length, i; uint32_t sum = 0U;
    if (!device || !io || !cache || !frame || !local_ip || !remote_ip || !connection ||
        !device->mac_valid || frame_capacity < NET_ETHERNET_HEADER_SIZE + 40U) return -1;
    if (net_arp_cache_lookup(cache, remote_ip, destination_mac) != 0) return -2;
    for (i = 0; i < 6U; ++i) { frame[i] = destination_mac[i]; frame[6U+i] = device->mac[i]; }
    frame[12] = 0x08U; frame[13] = 0x00U;
    for (i = 0; i < 40U; ++i) frame[NET_ETHERNET_HEADER_SIZE+i] = 0U;
    frame[NET_ETHERNET_HEADER_SIZE] = 0x45U;
    frame[NET_ETHERNET_HEADER_SIZE+2U] = 0U; frame[NET_ETHERNET_HEADER_SIZE+3U] = 40U;
    frame[NET_ETHERNET_HEADER_SIZE+8U] = 64U; frame[NET_ETHERNET_HEADER_SIZE+9U] = NET_TCP_PROTOCOL;
    for (i = 0; i < 4U; ++i) { frame[NET_ETHERNET_HEADER_SIZE+12U+i] = local_ip[i]; frame[NET_ETHERNET_HEADER_SIZE+16U+i] = remote_ip[i]; }
    tcp_length = (uint16_t)net_tcp_connection_build_ack(connection, frame + NET_ETHERNET_HEADER_SIZE + 20U,
                                                         frame_capacity - NET_ETHERNET_HEADER_SIZE - 20U);
    if ((int16_t)tcp_length < 0) return -3;
    /* Le segment TCP est construit dans la zone caller-owned; calculer son checksum. */
    frame[NET_ETHERNET_HEADER_SIZE+36U] = 0U; frame[NET_ETHERNET_HEADER_SIZE+37U] = 0U;
    { uint16_t checksum = net_tcp_checksum_ipv4(local_ip, remote_ip, frame + NET_ETHERNET_HEADER_SIZE + 20U, tcp_length);
      frame[NET_ETHERNET_HEADER_SIZE+36U] = (uint8_t)(checksum >> 8); frame[NET_ETHERNET_HEADER_SIZE+37U] = (uint8_t)checksum; }
    for (i = 0; i < 20U; i += 2U) { sum += ((uint16_t)frame[NET_ETHERNET_HEADER_SIZE+i] << 8) | frame[NET_ETHERNET_HEADER_SIZE+i+1U]; while (sum >> 16) sum = (sum & 0xffffU) + (sum >> 16); }
    sum = (~sum) & 0xffffU; frame[NET_ETHERNET_HEADER_SIZE+10U] = (uint8_t)(sum >> 8); frame[NET_ETHERNET_HEADER_SIZE+11U] = (uint8_t)sum;
    return ne2k_tx_submit(device, io, frame, (uint16_t)(NET_ETHERNET_HEADER_SIZE + 40U));
}

int ne2k_socket_ack(ne2k_device_t* device, const ne2k_io_t* io,
                    const net_arp_cache_t* cache, uint8_t* frame, uint16_t frame_capacity,
                    const uint8_t local_ip[4], const uint8_t remote_ip[4], int socket_id) {
    uint8_t segment[NET_TCP_HEADER_SIZE]; uint16_t segment_length;
    if (net_socket_build_ack(socket_id, segment, sizeof(segment), &segment_length) != 0) return -1;
    return ne2k_tcp_segment(device, io, cache, frame, frame_capacity, local_ip, remote_ip,
                            segment, segment_length);
}

int ne2k_tcp_fin(ne2k_device_t* device, const ne2k_io_t* io,
                 const net_arp_cache_t* cache, uint8_t* frame, uint16_t frame_capacity,
                 const uint8_t local_ip[4], const uint8_t remote_ip[4],
                 net_tcp_connection_t* connection) {
    uint8_t destination_mac[6]; uint16_t i; uint32_t sum = 0U; int tcp_length, status; net_tcp_connection_t next;
    if (!device || !io || !cache || !frame || !local_ip || !remote_ip || !connection || !device->mac_valid ||
        frame_capacity < NET_ETHERNET_HEADER_SIZE + 40U || connection->state != NET_TCP_STATE_ESTABLISHED) return -1;
    if (net_arp_cache_lookup(cache, remote_ip, destination_mac) != 0) return -2;
    for (i = 0; i < 6U; ++i) { frame[i] = destination_mac[i]; frame[6U+i] = device->mac[i]; }
    frame[12] = 0x08U; frame[13] = 0x00U;
    for (i = 0; i < 40U; ++i) frame[NET_ETHERNET_HEADER_SIZE+i] = 0U;
    frame[14] = 0x45U; frame[16] = 0U; frame[17] = 40U; frame[22] = 64U; frame[23] = NET_TCP_PROTOCOL;
    for (i = 0; i < 4U; ++i) { frame[26U+i] = local_ip[i]; frame[30U+i] = remote_ip[i]; }
    next = *connection;
    tcp_length = net_tcp_connection_begin_close(&next, frame + 34U, frame_capacity - 34U);
    if (tcp_length < 0) return -3;
    { uint16_t checksum = net_tcp_checksum_ipv4(local_ip, remote_ip, frame + 34U, (uint16_t)tcp_length);
      frame[50U] = (uint8_t)(checksum >> 8); frame[51U] = (uint8_t)checksum; }
    for (i = 0; i < 20U; i += 2U) { sum += ((uint16_t)frame[14U+i] << 8) | frame[15U+i]; while (sum >> 16) sum = (sum & 0xffffU) + (sum >> 16); }
    sum = (~sum) & 0xffffU; frame[24] = (uint8_t)(sum >> 8); frame[25] = (uint8_t)sum;
    status = ne2k_tx_submit(device, io, frame, NET_ETHERNET_HEADER_SIZE + 40U);
    if (status != 0) return status;
    *connection = next;
    return 0;
}

int ne2k_tcp_data(ne2k_device_t* device, const ne2k_io_t* io,
                  const net_arp_cache_t* cache, uint8_t* frame, uint16_t frame_capacity,
                  const uint8_t local_ip[4], const uint8_t remote_ip[4],
                  const net_tcp_connection_t* connection, const uint8_t* payload,
                  uint16_t payload_length) {
    uint8_t destination_mac[6]; uint16_t tcp_length, i, ip_length; uint32_t sum = 0U;
    if (!device || !io || !cache || !frame || !local_ip || !remote_ip || !connection ||
        (!payload && payload_length != 0U) || !device->mac_valid) return -1;
    if (net_arp_cache_lookup(cache, remote_ip, destination_mac) != 0) return -2;
    ip_length = (uint16_t)(20U + NET_TCP_HEADER_SIZE + payload_length);
    if ((uint32_t)NET_ETHERNET_HEADER_SIZE + ip_length > frame_capacity ||
        (uint32_t)NET_ETHERNET_HEADER_SIZE + ip_length > NE2K_ETHERNET_MAX_FRAME) return -3;
    for (i = 0; i < 6U; ++i) { frame[i] = destination_mac[i]; frame[6U+i] = device->mac[i]; }
    frame[12] = 0x08U; frame[13] = 0x00U;
    for (i = 0; i < ip_length; ++i) frame[NET_ETHERNET_HEADER_SIZE+i] = 0U;
    frame[14] = 0x45U; frame[16] = (uint8_t)(ip_length >> 8); frame[17] = (uint8_t)ip_length;
    frame[22] = 64U; frame[23] = NET_TCP_PROTOCOL;
    for (i = 0; i < 4U; ++i) { frame[26U+i] = local_ip[i]; frame[30U+i] = remote_ip[i]; }
    tcp_length = (uint16_t)net_tcp_build_data(frame + 34U, frame_capacity - 34U,
                                               connection->local_port, connection->remote_port,
                                               connection->local_sequence, connection->remote_sequence,
                                               payload, payload_length);
    if ((int16_t)tcp_length < 0) return -4;
    { uint16_t checksum = net_tcp_checksum_ipv4(local_ip, remote_ip, frame + 34U, tcp_length);
      frame[50U] = (uint8_t)(checksum >> 8); frame[51U] = (uint8_t)checksum; }
    for (i = 0; i < 20U; i += 2U) { sum += ((uint16_t)frame[14U+i] << 8) | frame[15U+i]; while (sum >> 16) sum = (sum & 0xffffU) + (sum >> 16); }
    sum = (~sum) & 0xffffU; frame[24] = (uint8_t)(sum >> 8); frame[25] = (uint8_t)sum;
    return ne2k_tx_submit(device, io, frame, (uint16_t)(NET_ETHERNET_HEADER_SIZE + ip_length));
}

int ne2k_tcp_segment(ne2k_device_t* device, const ne2k_io_t* io,
                     const net_arp_cache_t* cache, uint8_t* frame, uint16_t frame_capacity,
                     const uint8_t local_ip[4], const uint8_t remote_ip[4],
                     const uint8_t* segment, uint16_t segment_length) {
    uint8_t destination_mac[6]; net_tcp_view_t view; uint16_t i, ip_length, checksum; uint32_t sum = 0U;
    if (!device || !io || !cache || !frame || !local_ip || !remote_ip || !segment || !device->mac_valid) return -1;
    if (segment_length < NET_TCP_HEADER_SIZE || net_tcp_parse(segment, segment_length, &view) != 0) return -2;
    if (net_arp_cache_lookup(cache, remote_ip, destination_mac) != 0) return -3;
    ip_length = (uint16_t)(NET_IPV4_HEADER_SIZE + segment_length);
    if ((uint32_t)NET_ETHERNET_HEADER_SIZE + ip_length > frame_capacity ||
        (uint32_t)NET_ETHERNET_HEADER_SIZE + ip_length > NE2K_ETHERNET_MAX_FRAME) return -4;
    for (i = 0U; i < 6U; i++) { frame[i] = destination_mac[i]; frame[6U + i] = device->mac[i]; }
    frame[12] = 0x08U; frame[13] = 0x00U;
    for (i = 0U; i < ip_length; i++) frame[NET_ETHERNET_HEADER_SIZE + i] = 0U;
    frame[14] = 0x45U; frame[16] = (uint8_t)(ip_length >> 8); frame[17] = (uint8_t)ip_length;
    frame[22] = 64U; frame[23] = NET_TCP_PROTOCOL;
    for (i = 0U; i < 4U; i++) { frame[26U + i] = local_ip[i]; frame[30U + i] = remote_ip[i]; }
    for (i = 0U; i < segment_length; i++) frame[34U + i] = segment[i];
    frame[50] = 0U; frame[51] = 0U;
    checksum = net_tcp_checksum_ipv4(local_ip, remote_ip, frame + 34U, segment_length);
    frame[50] = (uint8_t)(checksum >> 8); frame[51] = (uint8_t)checksum;
    for (i = 0U; i < NET_IPV4_HEADER_SIZE; i += 2U) { sum += ((uint16_t)frame[14U + i] << 8) | frame[15U + i]; while (sum >> 16) sum = (sum & 0xffffU) + (sum >> 16); }
    sum = (~sum) & 0xffffU; frame[24] = (uint8_t)(sum >> 8); frame[25] = (uint8_t)sum;
    return ne2k_tx_submit(device, io, frame, (uint16_t)(NET_ETHERNET_HEADER_SIZE + ip_length));
}

int ne2k_socket_fin(ne2k_device_t* device, const ne2k_io_t* io,
                    const net_arp_cache_t* cache, uint8_t* frame, uint16_t frame_capacity,
                    const uint8_t local_ip[4], const uint8_t remote_ip[4], int socket_id) {
    net_tcp_connection_t previous_connection;
    uint8_t segment[64]; uint16_t segment_length = 0U;
    if (net_socket_connection_snapshot(socket_id, &previous_connection) != 0) return -1;
    if (net_socket_begin_close(socket_id, segment, sizeof(segment), &segment_length) != 0 ||
        ne2k_tcp_segment(device, io, cache, frame, frame_capacity, local_ip, remote_ip,
                         segment, segment_length) != 0) {
        (void)net_socket_connection_restore(socket_id, &previous_connection);
        return -2;
    }
    return 0;
}

int ne2k_socket_syn(ne2k_device_t* device, const ne2k_io_t* io,
                    const net_arp_cache_t* cache, uint8_t* frame, uint16_t frame_capacity,
                    const uint8_t local_ip[4], const uint8_t remote_ip[4], int socket_id,
                    uint8_t* segment, uint16_t segment_capacity) {
    uint16_t segment_length;
    int status;
    if (!segment) return -1;
    status = net_socket_build_syn(socket_id, segment, segment_capacity, &segment_length);
    if (status != 0) return status;
    return ne2k_tcp_segment(device, io, cache, frame, frame_capacity, local_ip, remote_ip,
                            segment, segment_length);
}

int ne2k_tcp_retransmit(ne2k_device_t* device, const ne2k_io_t* io,
                        const net_arp_cache_t* cache, uint8_t* frame, uint16_t frame_capacity,
                        const uint8_t local_ip[4], const uint8_t remote_ip[4],
                        net_tcp_connection_t* connection) {
    net_tcp_connection_t retransmit_view; int status;
    if (!connection || !net_tcp_connection_retransmit_allowed(connection)) return -1;
    retransmit_view = *connection;
    retransmit_view.local_sequence = connection->local_sequence - connection->pending_length;
    status = ne2k_tcp_data(device, io, cache, frame, frame_capacity, local_ip, remote_ip,
                           &retransmit_view, connection->pending_payload, connection->pending_length);
    if (status != 0) return status;
    return net_tcp_connection_note_retransmit(connection);
}

int ne2k_tcp_receive(const uint8_t* frame, uint16_t frame_length,
                     net_tcp_connection_t* connection, uint8_t* payload,
                     uint16_t payload_capacity, uint16_t* payload_length) {
    uint16_t ip_offset = NET_ETHERNET_HEADER_SIZE, tcp_offset, ip_length, tcp_length, accepted, i;
    net_tcp_view_t view;
    if (!frame || !connection || !payload_length || frame_length < NET_ETHERNET_HEADER_SIZE + 40U) return -1;
    if (frame[12] != 0x08U || frame[13] != 0x00U || (frame[ip_offset] >> 4) != 4U) return -2;
    if ((frame[ip_offset + 9U]) != NET_TCP_PROTOCOL) return -3;
    if (net_ipv4_checksum(frame + ip_offset, NET_IPV4_HEADER_SIZE) != 0U) return -4;
    ip_length = (uint16_t)(((uint16_t)frame[ip_offset + 2U] << 8) | frame[ip_offset + 3U]);
    if (ip_length < 40U || (uint32_t)NET_ETHERNET_HEADER_SIZE + ip_length > frame_length) return -5;
    tcp_offset = (uint16_t)(ip_offset + ((frame[ip_offset] & 0x0fU) * 4U));
    if (tcp_offset + NET_TCP_HEADER_SIZE > frame_length) return -6;
    tcp_length = (uint16_t)(ip_length - (tcp_offset - ip_offset));
    if (net_tcp_checksum_ipv4(frame + ip_offset + 12U, frame + ip_offset + 16U,
                              frame + tcp_offset, tcp_length) != 0U) return -7;
    if (net_tcp_parse(frame + tcp_offset, tcp_length, &view) != 0) return -8;
    if (view.payload_length > payload_capacity || (!payload && view.payload_length != 0U)) return -8;
    if (net_tcp_connection_accept_data(connection, &view, &accepted) != 0) return -9;
    for (i = 0; i < accepted; ++i) payload[i] = view.payload[i];
    *payload_length = accepted; return 0;
}

int ne2k_tcp_poll(ne2k_device_t* device, const ne2k_io_t* io,
                  uint8_t* frame, uint16_t frame_capacity,
                  net_tcp_connection_t* connection, uint8_t* payload,
                  uint16_t payload_capacity, uint16_t* payload_length) {
    uint16_t frame_length = 0U; int status;
    if (!device || !io || !frame || !connection || !payload_length) return -1;
    *payload_length = 0U;
    status = ne2k_rx_poll(device, io, frame, frame_capacity, &frame_length);
    if (status != 0) return status;
    if (frame_length == 0U) return 1;
    return ne2k_tcp_receive(frame, frame_length, connection, payload, payload_capacity, payload_length);
}

int ne2k_tcp_poll_ack(ne2k_device_t* device, const ne2k_io_t* io,
                      const net_arp_cache_t* cache, uint8_t* rx_frame,
                      uint16_t rx_capacity, uint8_t* tx_frame, uint16_t tx_capacity,
                      const uint8_t local_ip[4], const uint8_t remote_ip[4],
                      net_tcp_connection_t* connection, uint8_t* payload,
                      uint16_t payload_capacity, uint16_t* payload_length) {
    int status;
    if (!cache || !tx_frame || !local_ip || !remote_ip || !connection || !payload_length) return -1;
    *payload_length = 0U;
    status = ne2k_tcp_poll(device, io, rx_frame, rx_capacity, connection, payload,
                           payload_capacity, payload_length);
    if (status != 0 || *payload_length == 0U) return status;
    status = ne2k_tcp_ack(device, io, cache, tx_frame, tx_capacity, local_ip, remote_ip, connection);
    if (status != 0) return status;
    return 0;
}

int ne2k_tcp_poll_fin_ack(ne2k_device_t* device, const ne2k_io_t* io,
                          const net_arp_cache_t* cache, uint8_t* rx_frame,
                          uint16_t rx_capacity, uint8_t* tx_frame, uint16_t tx_capacity,
                          const uint8_t local_ip[4], const uint8_t remote_ip[4],
                          net_tcp_connection_t* connection) {
    uint16_t frame_length = 0U, ip_header_size, tcp_offset, ip_length, tcp_length; net_tcp_view_t view; int status;
    if (!cache || !rx_frame || !tx_frame || !local_ip || !remote_ip || !connection) return -1;
    status = ne2k_rx_poll(device, io, rx_frame, rx_capacity, &frame_length);
    if (status != 0) return status;
    if (frame_length == 0U) return 1;
    if (frame_length < NET_ETHERNET_HEADER_SIZE + 40U || rx_frame[12] != 0x08U || rx_frame[13] != 0x00U ||
        (rx_frame[14] >> 4) != 4U || rx_frame[23] != NET_TCP_PROTOCOL) return -2;
    if (net_ipv4_checksum(rx_frame + 14U, 20U) != 0U) return -3;
    ip_length = (uint16_t)(((uint16_t)rx_frame[16] << 8) | rx_frame[17]);
    ip_header_size = (uint16_t)((rx_frame[14] & 0x0fU) * 4U);
    if (ip_header_size < 20U || ip_length < ip_header_size + NET_TCP_HEADER_SIZE ||
        (uint32_t)NET_ETHERNET_HEADER_SIZE + ip_length > frame_length) return -4;
    tcp_offset = (uint16_t)(14U + ip_header_size); tcp_length = (uint16_t)(ip_length - ip_header_size);
    if (net_tcp_checksum_ipv4(rx_frame + 26U, rx_frame + 30U, rx_frame + tcp_offset, tcp_length) != 0U ||
        net_tcp_parse(rx_frame + tcp_offset, tcp_length, &view) != 0) return -5;
    if ((view.flags & NET_TCP_FLAG_FIN) == 0U || net_tcp_connection_accept_fin(connection, &view) != 0) return -6;
    return ne2k_tcp_ack(device, io, cache, tx_frame, tx_capacity, local_ip, remote_ip, connection);
}

int ne2k_dns_poll_a(ne2k_device_t* device, const ne2k_io_t* io,
                    uint8_t* frame, uint16_t frame_capacity, uint16_t attempts,
                    uint16_t expected_id, net_dns_a_result_t* result) {
    uint16_t frame_length, i; net_udp_view_t udp; int status;
    if (!device || !io || !frame || !result || attempts == 0U) return -1;
    for (i = 0; i < attempts; ++i) {
        status = ne2k_rx_poll_udp(device, io, frame, frame_capacity, &frame_length, &udp);
        if (status == 0 && udp.source_port == 53U && udp.destination_port == 49152U &&
            net_dns_parse_a_response(udp.payload, udp.payload_length, expected_id, result) == 0)
            return 0;
        ne2k_poll_pause();
    }
    return -2;
}

int ne2k_dhcp_poll_offer(ne2k_device_t* device, const ne2k_io_t* io,
                         uint8_t* frame, uint16_t frame_capacity,
                         uint32_t expected_xid, uint16_t attempts,
                         net_dhcp_offer_t* offer) {
    uint16_t frame_length, i; net_udp_view_t udp; int status;
    if (!device || !io || !frame || !offer || attempts == 0U) return -1;
    for (i = 0; i < attempts; ++i) {
        status = ne2k_rx_poll_udp(device, io, frame, frame_capacity,
                                  &frame_length, &udp);
        if (status == 0 && udp.source_port == 67U && udp.destination_port == 68U &&
            udp.payload_length >= 244U &&
            net_dhcp_parse_offer(udp.payload, udp.payload_length, expected_xid, offer) == 0)
            return 0;
        ne2k_poll_pause();
    }
    return -2;
}

int ne2k_dhcp_poll_ack(ne2k_device_t* device,const ne2k_io_t* io,uint8_t* frame,uint16_t frame_capacity,uint32_t expected_xid,uint16_t attempts,net_dhcp_lease_t* lease){uint16_t frame_length,i;net_udp_view_t udp;net_dhcp_lease_t next_lease;int status;if(!device||!io||!frame||!lease||attempts==0U)return -1;for(i=0U;i<attempts;i++){status=ne2k_rx_poll_udp(device,io,frame,frame_capacity,&frame_length,&udp);if(status==0&&udp.source_port==67U&&udp.destination_port==68U&&udp.payload_length>=NET_DHCP_FIXED_HEADER+NET_DHCP_COOKIE_SIZE&&net_dhcp_parse_ack(udp.payload,udp.payload_length,expected_xid,&next_lease)==0){*lease=next_lease;return 0;}ne2k_poll_pause();}return -2;}

int ne2k_dhcp_acquire(ne2k_device_t* device,const ne2k_io_t* io,uint8_t* tx_frame,uint16_t tx_capacity,uint8_t* rx_frame,uint16_t rx_capacity,uint32_t xid,uint16_t poll_attempts,net_dhcp_lease_t* lease){net_dhcp_offer_t offer;net_dhcp_lease_t next_lease;int status;if(!device||!io||!tx_frame||!rx_frame||!lease||poll_attempts==0U)return -1;status=ne2k_dhcp_discover(device,io,tx_frame,tx_capacity,xid);if(status!=0)return -12;status=ne2k_dhcp_poll_offer(device,io,rx_frame,rx_capacity,xid,poll_attempts,&offer);if(status!=0)return -13;status=ne2k_dhcp_request(device,io,tx_frame,tx_capacity,xid,offer.offered_ip,offer.server_ip);if(status!=0)return -14;status=ne2k_dhcp_poll_ack(device,io,rx_frame,rx_capacity,xid,poll_attempts,&next_lease);if(status!=0)return -15;*lease=next_lease;return 0;}

int ne2k_dhcp_renew(ne2k_device_t* device, const ne2k_io_t* io,
                    uint8_t* frame, uint16_t frame_capacity,
                    uint32_t xid, const net_dhcp_lease_t* lease) {
    static const uint8_t broadcast_mac[6] = {0xffU,0xffU,0xffU,0xffU,0xffU,0xffU};
    static const uint8_t broadcast_ip[4] = {255U,255U,255U,255U};
    int payload_length;
    if (!device || !io || !frame || !lease || !lease->valid ||
        frame_capacity < NET_ETHERNET_HEADER_SIZE + NET_IPV4_HEADER_SIZE + NET_UDP_HEADER_SIZE + 249U) return -1;
    payload_length = net_dhcp_build_renew(frame + NET_ETHERNET_HEADER_SIZE + NET_IPV4_HEADER_SIZE + NET_UDP_HEADER_SIZE,
                                          frame_capacity - NET_ETHERNET_HEADER_SIZE - NET_IPV4_HEADER_SIZE - NET_UDP_HEADER_SIZE,
                                          xid, device->mac, lease->ipv4);
    if (payload_length < 0) return -2;
    return ne2k_tx_udp(device, io, frame, frame_capacity, broadcast_mac, lease->ipv4, broadcast_ip,
                       68U, 67U, frame + NET_ETHERNET_HEADER_SIZE + NET_IPV4_HEADER_SIZE + NET_UDP_HEADER_SIZE,
                       (uint16_t)payload_length);
}

int ne2k_dhcp_renew_if_due(ne2k_device_t* device, const ne2k_io_t* io,
                            uint8_t* tx_frame, uint16_t tx_capacity,
                            uint8_t* rx_frame, uint16_t rx_capacity,
                            uint32_t xid, uint16_t poll_attempts,
                            uint32_t now, net_dhcp_lease_t* lease) {
    net_dhcp_lease_t next_lease;
    int status;
    if (!device || !io || !tx_frame || !rx_frame || !lease || poll_attempts == 0U) return -1;
    if (!net_dhcp_lease_is_valid_at(lease, now)) return -2;
    if (!net_dhcp_lease_renewal_due(lease, now)) return 0;
    next_lease = *lease;
    status = ne2k_dhcp_renew(device, io, tx_frame, tx_capacity, xid, &next_lease);
    if (status != 0) return -3;
    status = ne2k_dhcp_poll_ack(device, io, rx_frame, rx_capacity, xid, poll_attempts, &next_lease);
    if (status != 0) return -4;
    if (net_dhcp_lease_mark_acquired(&next_lease, now) != 0) return -5;
    *lease = next_lease;
    return 1;
}

int ne2k_irq_attach(ne2k_device_t* device, const ne2k_io_t* io) {
    if (!device || !io || !io->inb || !io->outb || device->base_port == 0U)
        return -1;
    ne2k_irq_device = device;
    ne2k_irq_io = io;
    ne2k_irq_events = 0U;
    return 0;
}

void ne2k_irq_service(void) {
    uint8_t status;
    if (!ne2k_irq_device || !ne2k_irq_io) return;
    status = ne2k_irq_io->inb(ne2k_irq_io->context,
                              (uint16_t)(ne2k_irq_device->base_port + NE2K_REG_ISR));
    if (status == 0U) return;
    ne2k_irq_io->outb(ne2k_irq_io->context,
                      (uint16_t)(ne2k_irq_device->base_port + NE2K_REG_ISR), status);
    ++ne2k_irq_events;
}

uint32_t ne2k_irq_count(void) {
    return ne2k_irq_events;
}

static void ne2k_remote_read_setup(const ne2k_io_t* io, uint16_t base,
                                   uint16_t address, uint16_t length) {
    io->outb(io->context, (uint16_t)(base + NE2K_REG_RBCR0), (uint8_t)length);
    io->outb(io->context, (uint16_t)(base + NE2K_REG_RBCR1), (uint8_t)(length >> 8));
    io->outb(io->context, (uint16_t)(base + NE2K_REG_RSAR0), (uint8_t)address);
    io->outb(io->context, (uint16_t)(base + NE2K_REG_RSAR1), (uint8_t)(address >> 8));
    io->outb(io->context, (uint16_t)(base + NE2K_REG_COMMAND),
             NE2K_COMMAND_REMOTE_READ);
}

int ne2k_rx_poll(ne2k_device_t* device, const ne2k_io_t* io,
                 uint8_t* frame, uint16_t frame_capacity,
                 uint16_t* frame_length) {
    uint16_t base, page, packet_length, payload_length;
    uint8_t header[NE2K_RX_HEADER_SIZE];
    uint8_t next_page, current_page;
    uint32_t i;
    if (!device || !io || !io->inb || !io->outb || !frame || !frame_length ||
        !device->initialized || frame_capacity == 0U || device->base_port == 0U)
        return -1;
    *frame_length = 0U;
    base = device->base_port;
    /* L’IRQ peut déjà avoir acquitté PRX : CURR/BNRY reste l’état persistant
     * du ring et évite de perdre une trame DHCP ou DNS arrivée entre deux tours. */
    io->outb(io->context, (uint16_t)(base + NE2K_REG_COMMAND), 0x42U);
    current_page = io->inb(io->context, (uint16_t)(base + NE2K_REG_CURR));
    io->outb(io->context, (uint16_t)(base + NE2K_REG_COMMAND), 0x22U);
    page = (uint16_t)io->inb(io->context, (uint16_t)(base + NE2K_REG_BNRY)) + 1U;
    if (page >= NE2K_RX_PAGE_STOP) page = NE2K_RX_PAGE_START;
    if (current_page < NE2K_RX_PAGE_START || current_page >= NE2K_RX_PAGE_STOP)
        return -2;
    if ((uint8_t)page == current_page) return 1;
    ne2k_remote_read_setup(io, base, (uint16_t)(page << 8), NE2K_RX_HEADER_SIZE);
    for (i = 0; i < NE2K_RX_HEADER_SIZE; ++i)
        header[i] = io->inb(io->context, (uint16_t)(base + NE2K_REG_DATA));
    if ((header[0] & NE2K_RX_STATUS_OK) == 0U) return -3;
    next_page = header[1];
    packet_length = (uint16_t)(header[2] | ((uint16_t)header[3] << 8));
    if (packet_length < NE2K_RX_HEADER_SIZE || packet_length > NE2K_ETHERNET_MAX_FRAME)
        return -4;
    payload_length = (uint16_t)(packet_length - NE2K_RX_HEADER_SIZE);
    if (payload_length > frame_capacity) return -5;
    ne2k_remote_read_setup(io, base, (uint16_t)((page << 8) + NE2K_RX_HEADER_SIZE),
                           payload_length);
    for (i = 0; i < payload_length; ++i)
        frame[i] = io->inb(io->context, (uint16_t)(base + NE2K_REG_DATA));
    if (next_page < NE2K_RX_PAGE_START || next_page >= NE2K_RX_PAGE_STOP)
        return -6;
    io->outb(io->context, (uint16_t)(base + NE2K_REG_BNRY),
             (uint8_t)(next_page == NE2K_RX_PAGE_START ? NE2K_RX_PAGE_STOP - 1U : next_page - 1U));
    *frame_length = payload_length;
    return 0;
}

int ne2k_rx_poll_arp(ne2k_device_t* device, const ne2k_io_t* io,
                     uint8_t* frame, uint16_t frame_capacity,
                     uint16_t* frame_length,
                     net_ethernet_header_t* ethernet,
                     net_arp_packet_t* arp) {
    int status;
    if (!ethernet || !arp) return -1;
    status = ne2k_rx_poll(device, io, frame, frame_capacity, frame_length);
    if (status != 0) return status;
    if (net_ethernet_parse(frame, *frame_length, ethernet) != 0)
        return -2;
    if (ethernet->ethertype != NET_ETHERTYPE_ARP)
        return 2;
    if (net_arp_parse(frame, *frame_length, arp) != 0)
        return -3;
    return 0;
}

int ne2k_arp_service(ne2k_device_t* device, const ne2k_io_t* io,
                     uint8_t* rx_frame, uint16_t rx_capacity,
                     uint8_t* tx_frame, uint16_t tx_capacity,
                     const uint8_t local_mac[6], const uint8_t local_ipv4[4]) {
    uint16_t rx_length = 0U;
    net_ethernet_header_t ethernet;
    net_arp_packet_t arp;
    int status;
    if (!tx_frame || !local_mac || !local_ipv4) return -1;
    status = ne2k_rx_poll_arp(device, io, rx_frame, rx_capacity, &rx_length,
                              &ethernet, &arp);
    if (status != 0) return status;
    if (arp.opcode != NET_ARP_OPCODE_REQUEST ||
        arp.target_ipv4[0] != local_ipv4[0] || arp.target_ipv4[1] != local_ipv4[1] ||
        arp.target_ipv4[2] != local_ipv4[2] || arp.target_ipv4[3] != local_ipv4[3])
        return 2;
    status = net_arp_build_reply(tx_frame, tx_capacity, &arp, local_mac, local_ipv4);
    if (status < 0) return -2;
    return ne2k_tx_submit(device, io, tx_frame, (uint16_t)status);
}

int ne2k_rx_poll_tcp(ne2k_device_t* device, const ne2k_io_t* io,
                     uint8_t* frame, uint16_t frame_capacity,
                     uint16_t* frame_length, net_tcp_view_t* tcp) {
    net_ethernet_header_t ethernet; uint16_t ip_header_size; int status;
    if (!tcp) return -1;
    status = ne2k_rx_poll(device, io, frame, frame_capacity, frame_length);
    if (status != 0) return status;
    if (net_ethernet_parse(frame, *frame_length, &ethernet) != 0) return -2;
    if (ethernet.ethertype != NET_ETHERTYPE_IPV4) return 2;
    if (*frame_length < NET_ETHERNET_HEADER_SIZE + 20U) return -3;
    ip_header_size = (uint16_t)(frame[NET_ETHERNET_HEADER_SIZE] & 0x0fU) * 4U;
    if (ip_header_size < 20U || frame[NET_ETHERNET_HEADER_SIZE + 9U] != NET_TCP_PROTOCOL ||
        *frame_length < NET_ETHERNET_HEADER_SIZE + ip_header_size) return -3;
    if (net_tcp_parse(frame + NET_ETHERNET_HEADER_SIZE + ip_header_size,
                      (uint32_t)(*frame_length - NET_ETHERNET_HEADER_SIZE - ip_header_size), tcp) != 0)
        return -4;
    return 0;
}

int ne2k_socket_poll_tcp(ne2k_device_t* device, const ne2k_io_t* io,
                         uint8_t* frame, uint16_t frame_capacity, int socket_id) {
    net_tcp_view_t view;
    uint16_t frame_length, ip_header_size, tcp_offset;
    int status;
    status = ne2k_rx_poll_tcp(device, io, frame, frame_capacity, &frame_length, &view);
    if (status != 0) return status;
    ip_header_size = (uint16_t)(frame[NET_ETHERNET_HEADER_SIZE] & 0x0fU) * 4U;
    tcp_offset = (uint16_t)(NET_ETHERNET_HEADER_SIZE + ip_header_size);
    if (tcp_offset >= frame_length) return -5;
    return net_socket_feed(socket_id, frame + tcp_offset, (uint16_t)(frame_length - tcp_offset));
}

int ne2k_rx_poll_udp(ne2k_device_t* device, const ne2k_io_t* io,
                     uint8_t* frame, uint16_t frame_capacity,
                     uint16_t* frame_length, net_udp_view_t* udp) {
    net_ethernet_header_t ethernet;
    int status;
    if (!udp) return -1;
    status = ne2k_rx_poll(device, io, frame, frame_capacity, frame_length);
    if (status != 0) return status;
    if (net_ethernet_parse(frame, *frame_length, &ethernet) != 0)
        return -2;
    if (ethernet.ethertype != NET_ETHERTYPE_IPV4)
        return 2;
    if (*frame_length <= NET_ETHERNET_HEADER_SIZE ||
        net_udp_parse_ipv4(frame + NET_ETHERNET_HEADER_SIZE,
                           (uint32_t)(*frame_length - NET_ETHERNET_HEADER_SIZE), udp) != 0)
        return -3;
    return 0;
}

int ne2k_i386_io(ne2k_io_t* io) {
    if (!io) return -1;
#ifdef __i386__
    io->context = (void*)0; io->inb = ne2k_i386_inb; io->outb = ne2k_i386_outb; return 0;
#else
    io->context = (void*)0; io->inb = (ne2k_inb_fn)0; io->outb = (ne2k_outb_fn)0; return -1;
#endif
}

int ne2k_probe(ne2k_device_t* device, uint16_t base_port, const ne2k_io_t* io) {
    uint8_t reset_value;
    uint8_t isr_value;
    if (!device || !io || !io->inb || !io->outb || base_port == 0U) return -1;
    device->base_port = base_port;
    device->initialized = 0U;
    device->mac[0] = device->mac[1] = device->mac[2] = 0U;
    device->mac[3] = device->mac[4] = device->mac[5] = 0U;
    device->mac_valid = 0U;
    io->outb(io->context, (uint16_t)(base_port + NE2K_REG_COMMAND),
             NE2K_COMMAND_STOP | NE2K_COMMAND_PAGE0);
    reset_value = io->inb(io->context, (uint16_t)(base_port + NE2K_REG_RESET));
    io->outb(io->context, (uint16_t)(base_port + NE2K_REG_RESET), reset_value);
    isr_value = io->inb(io->context, (uint16_t)(base_port + NE2K_REG_ISR));
    if ((isr_value & NE2K_ISR_RESET) == 0U)
        return -2;
    io->outb(io->context, (uint16_t)(base_port + NE2K_REG_DCR), NE2K_DCR_WORD_MODE);
    /* QEMU ne relit pas le DCR sur ce modèle; les ports flottants renvoient 0xff. */
    if (reset_value == 0xffU || isr_value == 0xffU) return -3;
    return 0;
}

int ne2k_configure_rings(ne2k_device_t* device, const ne2k_io_t* io) {
    uint16_t base;
    uint8_t index;
    if (!device || !io || !io->outb || device->base_port == 0U) return -1;
    base = device->base_port;
    io->outb(io->context, (uint16_t)(base + NE2K_REG_COMMAND),
             NE2K_COMMAND_STOP | NE2K_COMMAND_NODMA | NE2K_COMMAND_PAGE0);
    io->outb(io->context, (uint16_t)(base + NE2K_REG_TPSR), 0x40U);
    io->outb(io->context, (uint16_t)(base + NE2K_REG_PSTART), 0x46U);
    io->outb(io->context, (uint16_t)(base + NE2K_REG_PSTOP), 0x60U);
    io->outb(io->context, (uint16_t)(base + NE2K_REG_BNRY), 0x46U);
    /* Broadcast DHCP et unicast strictement filtré par PAR sont acceptés. */
    io->outb(io->context, (uint16_t)(base + NE2K_REG_RCR), 0x04U);
    io->outb(io->context, (uint16_t)(base + NE2K_REG_TCR), 0x00U);
    /* CURR vit en page 1 : la première page RX est occupée par la frontière. */
    io->outb(io->context, (uint16_t)(base + NE2K_REG_COMMAND),
             NE2K_COMMAND_STOP | NE2K_COMMAND_NODMA | NE2K_COMMAND_PAGE1);
    /* PAR[0..5] partage les offsets PSTART..TBCR0 de la page 0. */
    if (device->mac_valid)
        for (index = 0U; index < 6U; ++index)
            io->outb(io->context, (uint16_t)(base + 1U + index), device->mac[index]);
    io->outb(io->context, (uint16_t)(base + NE2K_REG_CURR), NE2K_RX_PAGE_START + 1U);
    io->outb(io->context, (uint16_t)(base + NE2K_REG_COMMAND), 0x22U);
    return 0;
}

int ne2k_set_mac(ne2k_device_t* device, const uint8_t mac[6]) {
    uint32_t i;
    uint8_t nonzero = 0U;
    if (!device || !mac || (mac[0] & 1U) != 0U) return -1;
    for (i = 0; i < 6U; ++i) {
        device->mac[i] = mac[i];
        if (mac[i] != 0U) nonzero = 1U;
    }
    device->mac_valid = nonzero;
    return nonzero ? 0 : -2;
}

int ne2k_read_mac(ne2k_device_t* device, const ne2k_io_t* io) {
    uint8_t prom[12];
    uint16_t base;
    uint32_t i;
    if (!device || !io || !io->inb || !io->outb || device->base_port == 0U)
        return -1;
    base = device->base_port;
    /* La PROM NE2000 expose la MAC sur les octets pairs d’une lecture DMA
     * distante de 12 octets. Lire DATA sans initialiser RSAR/RBCR récupérait
     * une position résiduelle et programmait un PAR différent de la MAC QEMU. */
    io->outb(io->context, (uint16_t)(base + NE2K_REG_COMMAND),
             NE2K_COMMAND_STOP | NE2K_COMMAND_NODMA | NE2K_COMMAND_PAGE0);
    io->outb(io->context, (uint16_t)(base + NE2K_REG_DCR), NE2K_DCR_BYTE_MODE);
    io->outb(io->context, (uint16_t)(base + NE2K_REG_RBCR0), 12U);
    io->outb(io->context, (uint16_t)(base + NE2K_REG_RBCR1), 0U);
    io->outb(io->context, (uint16_t)(base + NE2K_REG_RSAR0), 0U);
    io->outb(io->context, (uint16_t)(base + NE2K_REG_RSAR1), 0U);
    io->outb(io->context, (uint16_t)(base + NE2K_REG_COMMAND), NE2K_COMMAND_REMOTE_READ);
    for (i = 0; i < 12U; ++i)
        prom[i] = io->inb(io->context, (uint16_t)(base + NE2K_REG_DATA));
    for (i = 0; i < 6U; ++i)
        device->mac[i] = prom[i * 2U];
    device->mac_valid = 0U;
    for (i = 0; i < 6U; ++i)
        if (device->mac[i] != 0U) device->mac_valid = 1U;
    if ((device->mac[0] & 1U) != 0U || device->mac_valid == 0U) {
        device->mac_valid = 0U;
        return -2;
    }
    return 0;
}

int ne2k_rx_extract(const uint8_t* dma_buffer, uint16_t dma_length,
                    net_nic_queue_t* rx_queue) {
    uint16_t packet_length;
    uint8_t* destination;
    uint16_t capacity;
    if (!dma_buffer || !rx_queue || dma_length < NE2K_RX_HEADER_SIZE)
        return -1;
    if ((dma_buffer[0] & NE2K_RX_STATUS_OK) == 0U) return -2;
    packet_length = (uint16_t)(dma_buffer[2] | ((uint16_t)dma_buffer[3] << 8));
    if (packet_length < NE2K_RX_HEADER_SIZE || packet_length > dma_length)
        return -3;
    if (net_nic_queue_acquire(rx_queue, &destination, &capacity) != 0 ||
        packet_length - NE2K_RX_HEADER_SIZE > capacity)
        return -4;
    {
        uint16_t i;
        for (i = 0; i < packet_length - NE2K_RX_HEADER_SIZE; ++i)
            destination[i] = dma_buffer[NE2K_RX_HEADER_SIZE + i];
    }
    return net_nic_queue_commit(rx_queue,
                                 (uint16_t)(packet_length - NE2K_RX_HEADER_SIZE));
}

int ne2k_tx_submit(ne2k_device_t* device, const ne2k_io_t* io,
                   const uint8_t* frame, uint16_t length) {
    uint16_t base;
    uint16_t wire_length;
    uint32_t i;
    if (!device || !io || !io->inb || !io->outb || !frame ||
        !device->initialized || device->base_port == 0U || length == 0U ||
        length > NE2K_ETHERNET_MAX_FRAME)
        return -1;
    base = device->base_port;
    wire_length = length < NE2K_ETHERNET_MIN_FRAME ? NE2K_ETHERNET_MIN_FRAME : length;
    io->outb(io->context, (uint16_t)(base + NE2K_REG_COMMAND),
             NE2K_COMMAND_STOP | NE2K_COMMAND_NODMA | NE2K_COMMAND_PAGE0);
    io->outb(io->context, (uint16_t)(base + NE2K_REG_DCR), NE2K_DCR_BYTE_MODE);
    io->outb(io->context, (uint16_t)(base + NE2K_REG_RBCR0), (uint8_t)wire_length);
    io->outb(io->context, (uint16_t)(base + NE2K_REG_RBCR1), (uint8_t)(wire_length >> 8));
    io->outb(io->context, (uint16_t)(base + NE2K_REG_RSAR0), 0U);
    io->outb(io->context, (uint16_t)(base + NE2K_REG_RSAR1), NE2K_TX_PAGE);
    /* Acquitter RDC avant le DMA : l’acquittement après copie effaçait le
     * signal d’achèvement avant la boucle d’attente, notamment sous QEMU. */
    io->outb(io->context, (uint16_t)(base + NE2K_REG_ISR), NE2K_ISR_RDC);
    io->outb(io->context, (uint16_t)(base + NE2K_REG_COMMAND), NE2K_COMMAND_REMOTE_WRITE);
    for (i = 0; i < (uint32_t)wire_length; ++i)
        io->outb(io->context, (uint16_t)(base + NE2K_REG_DATA),
                 i < length ? frame[i] : 0U);
    for (i = 0; i < 65535U; ++i)
        if ((io->inb(io->context, (uint16_t)(base + NE2K_REG_ISR)) & NE2K_ISR_RDC) != 0U)
            break;
    if (i == 65535U) return -2;
    io->outb(io->context, (uint16_t)(base + NE2K_REG_TPSR), NE2K_TX_PAGE);
    io->outb(io->context, (uint16_t)(base + NE2K_REG_TBCR0), (uint8_t)wire_length);
    io->outb(io->context, (uint16_t)(base + NE2K_REG_TBCR1), (uint8_t)(wire_length >> 8));
    io->outb(io->context, (uint16_t)(base + NE2K_REG_COMMAND), NE2K_COMMAND_TRANSMIT);
    return 0;
}

int ne2k_prepare(ne2k_device_t* device, const ne2k_io_t* io) {
    uint16_t base;
    if (!device || !io || !io->outb || device->base_port == 0U) return -1;
    base = device->base_port;
    io->outb(io->context, (uint16_t)(base + NE2K_REG_COMMAND),
             NE2K_COMMAND_STOP | NE2K_COMMAND_NODMA | NE2K_COMMAND_PAGE0);
    io->outb(io->context, (uint16_t)(base + NE2K_REG_DCR), NE2K_DCR_WORD_MODE);
    io->outb(io->context, (uint16_t)(base + NE2K_REG_ISR), 0xffU);
    device->initialized = 1U;
    return 0;
}

int ne2k_tls_client_init(ne2k_tls_client_t* client,uint8_t* record_buffer,uint16_t record_capacity,
                         uint8_t* handshake_buffer,uint16_t handshake_capacity,
                         uint8_t* transcript_buffer,uint16_t transcript_capacity){
    uint8_t index;
    if(!client)return -1;
    if(net_tcp_tls_stream_init(&client->stream,record_buffer,record_capacity,handshake_buffer,handshake_capacity)!=0)return -2;
    if(net_tls_handshake_init(&client->handshake)!=0)return -3;
    if(net_tls_transcript_init(&client->transcript,transcript_buffer,transcript_capacity)!=0)return -4;
    for(index=0U;index<48U;index++)client->master_secret[index]=0U;
    for(index=0U;index<NET_TLS_AES_128_GCM_KEY_BLOCK_LENGTH;index++)client->key_block[index]=0U;
    for(index=0U;index<NET_TLS_X25519_KEY_LENGTH;index++){client->x25519.client_public[index]=0U;client->x25519.shared_secret[index]=0U;}
    client->x25519.ready=0U;client->session.write_key=0;client->session.read_key=0;client->session.write_fixed_iv=0;client->session.read_fixed_iv=0;client->session.write_sequence=0U;client->session.read_sequence=0U;client->peer_identity_validated=0U;client->complete=0U;
    return 0;
}

int ne2k_tls_client_retry_reset(net_tcp_connection_t* connection,ne2k_tls_client_t* client,net_tcp_connection_retry_t* retry,uint32_t local_sequence){
    ne2k_tls_client_t next_client;net_tcp_connection_t next_connection;uint8_t index;int status;
    if(!connection||!client||!retry)return -1;
    next_connection=*connection;next_client=*client;
    status=net_tcp_connection_retry_reopen(&next_connection,retry,local_sequence);
    if(status<=0)return status;
    if(net_tls_handshake_init(&next_client.handshake)!=0)return -2;
    next_client.stream.record_accumulator.length=0U;next_client.stream.handshake_accumulator.length=0U;next_client.transcript.length=0U;
    for(index=0U;index<48U;index++)next_client.master_secret[index]=0U;
    for(index=0U;index<NET_TLS_AES_128_GCM_KEY_BLOCK_LENGTH;index++)next_client.key_block[index]=0U;
    for(index=0U;index<NET_TLS_X25519_KEY_LENGTH;index++){next_client.x25519.client_public[index]=0U;next_client.x25519.shared_secret[index]=0U;}
    next_client.x25519.ready=0U;next_client.session.write_key=0;next_client.session.read_key=0;next_client.session.write_fixed_iv=0;next_client.session.read_fixed_iv=0;next_client.session.write_sequence=0U;next_client.session.read_sequence=0U;next_client.peer_identity_validated=0U;next_client.complete=0U;
    *connection=next_connection;*client=next_client;return 1;
}

int ne2k_tls_client_start(ne2k_device_t* device,const ne2k_io_t* io,const net_arp_cache_t* cache,
                          uint8_t* tx_frame,uint16_t tx_capacity,const uint8_t local_ip[4],const uint8_t remote_ip[4],
                          net_tcp_connection_t* connection,ne2k_tls_client_t* client,
                          const uint8_t client_random[32],uint8_t* client_hello_record,uint32_t client_hello_capacity,
                          uint8_t retransmit_limit){
    ne2k_tls_client_t next_client;net_tcp_connection_t next_connection;net_tls_record_view_t record;int length,status;
    if(!device||!io||!cache||!tx_frame||!local_ip||!remote_ip||!connection||!client||!client_random||!client_hello_record)return -1;
    if(connection->state!=NET_TCP_STATE_ESTABLISHED||client->handshake.state!=NET_TLS_HANDSHAKE_IDLE)return -2;
    next_client=*client;next_connection=*connection;
    length=net_tls_client_hello_build(client_hello_record,client_hello_capacity,client_random);
    if(length<0||net_tls_record_parse(client_hello_record,(uint32_t)length,&record)!=0)return -3;
    if(net_tls_handshake_note_client_hello(&next_client.handshake)!=0||net_tls_transcript_append(&next_client.transcript,record.payload,record.payload_length)!=0)return -4;
    if(net_tcp_connection_track_send(&next_connection,client_hello_record,(uint16_t)length,retransmit_limit)!=0)return -5;
    status=ne2k_tcp_data(device,io,cache,tx_frame,tx_capacity,local_ip,remote_ip,&next_connection,client_hello_record,(uint16_t)length);
    if(status!=0)return -6;
    if(net_tcp_connection_commit_send(&next_connection,(uint16_t)length)!=0)return -7;
    *connection=next_connection;*client=next_client;return length;
}

int ne2k_socket_tls_start(ne2k_device_t* device,const ne2k_io_t* io,const net_arp_cache_t* cache,
                          uint8_t* tx_frame,uint16_t tx_capacity,const uint8_t local_ip[4],const uint8_t remote_ip[4],
                          int socket_id,ne2k_tls_client_t* client,const uint8_t client_random[32],
                          uint8_t* client_hello_record,uint32_t client_hello_capacity,
                          uint8_t* tcp_segment,uint16_t tcp_segment_capacity,uint8_t retransmit_limit) {
    ne2k_tls_client_t next_client; net_tcp_connection_t previous_connection; net_tls_record_view_t record;
    uint16_t segment_length; int length, status;
    if (!device || !io || !cache || !tx_frame || !local_ip || !remote_ip || !client || !client_random ||
        !client_hello_record || !tcp_segment) return -1;
    if (net_socket_connection_snapshot(socket_id, &previous_connection) != 0 ||
        previous_connection.state != NET_TCP_STATE_ESTABLISHED || client->handshake.state != NET_TLS_HANDSHAKE_IDLE) return -2;
    next_client = *client;
    length = net_tls_client_hello_build(client_hello_record, client_hello_capacity, client_random);
    if (length < 0 || net_tls_record_parse(client_hello_record, (uint32_t)length, &record) != 0) return -3;
    if (net_tls_handshake_note_client_hello(&next_client.handshake) != 0 ||
        net_tls_transcript_append(&next_client.transcript, record.payload, record.payload_length) != 0) return -4;
    status = net_socket_send_limit(socket_id, client_hello_record, (uint16_t)length, tcp_segment,
                                   tcp_segment_capacity, &segment_length, retransmit_limit);
    if (status != 0) return -5;
    status = ne2k_tcp_segment(device, io, cache, tx_frame, tx_capacity, local_ip, remote_ip,
                              tcp_segment, segment_length);
    if (status != 0) { (void)net_socket_connection_restore(socket_id, &previous_connection); return -6; }
    *client = next_client;
    return length;
}

int ne2k_socket_tls_close_notify(ne2k_device_t* device,const ne2k_io_t* io,const net_arp_cache_t* cache,uint8_t* tx_frame,uint16_t tx_capacity,const uint8_t local_ip[4],const uint8_t remote_ip[4],int socket_id,ne2k_tls_client_t* client,uint8_t* tls_record,uint32_t tls_capacity,uint8_t retransmit_limit){net_tcp_connection_t previous_connection,connection;ne2k_tls_client_t previous_client;int record_length,status;if(!device||!io||!cache||!tx_frame||!local_ip||!remote_ip||!client||!tls_record)return -1;if(!client->complete||!net_tls_handshake_is_complete(&client->handshake))return -2;if(net_socket_connection_snapshot(socket_id,&connection)!=0)return -3;previous_connection=connection;previous_client=*client;record_length=net_tls_close_notify_build(&client->session,tls_record,tls_capacity);if(record_length<0)goto rollback;if(net_tcp_connection_track_send(&connection,tls_record,(uint16_t)record_length,retransmit_limit)!=0)goto rollback;status=ne2k_tcp_data(device,io,cache,tx_frame,tx_capacity,local_ip,remote_ip,&connection,tls_record,(uint16_t)record_length);if(status!=0)goto rollback;if(net_tcp_connection_commit_send(&connection,(uint16_t)record_length)!=0)goto rollback;if(net_socket_connection_restore(socket_id,&connection)!=0)goto rollback;return record_length;rollback:(void)net_socket_connection_restore(socket_id,&previous_connection);*client=previous_client;return -4;}

int ne2k_socket_aes_gcm_close_notify(ne2k_device_t* device,const ne2k_io_t* io,const net_arp_cache_t* cache,uint8_t* tx_frame,uint16_t tx_capacity,const uint8_t local_ip[4],const uint8_t remote_ip[4],int socket_id,net_tls_aes_gcm_session_t* session,uint8_t* tls_record,uint32_t tls_capacity,uint8_t retransmit_limit){net_tcp_connection_t previous_connection,connection;net_tls_aes_gcm_session_t previous_session;int record_length,status;if(!device||!io||!cache||!tx_frame||!local_ip||!remote_ip||!session||!tls_record)return -1;if(net_socket_connection_snapshot(socket_id,&connection)!=0)return -2;previous_connection=connection;previous_session=*session;record_length=net_tls_close_notify_build(session,tls_record,tls_capacity);if(record_length<0)goto rollback;if(net_tcp_connection_track_send(&connection,tls_record,(uint16_t)record_length,retransmit_limit)!=0)goto rollback;status=ne2k_tcp_data(device,io,cache,tx_frame,tx_capacity,local_ip,remote_ip,&connection,tls_record,(uint16_t)record_length);if(status!=0)goto rollback;if(net_tcp_connection_commit_send(&connection,(uint16_t)record_length)!=0)goto rollback;if(net_socket_connection_restore(socket_id,&connection)!=0)goto rollback;return record_length;rollback:(void)net_socket_connection_restore(socket_id,&previous_connection);*session=previous_session;return -3;}
int ne2k_socket_tls_accept_syn_ack_start(ne2k_device_t* device,const ne2k_io_t* io,const net_arp_cache_t* cache,
                                         uint8_t* tx_frame,uint16_t tx_capacity,const uint8_t local_ip[4],const uint8_t remote_ip[4],
                                         int socket_id,const net_tcp_view_t* syn_ack,ne2k_tls_client_t* client,
                                         const uint8_t client_random[32],uint8_t* client_hello_record,
                                         uint32_t client_hello_capacity,uint8_t* tcp_segment,
                                         uint16_t tcp_segment_capacity,uint8_t retransmit_limit) {
    net_tcp_connection_t previous_connection; ne2k_tls_client_t previous_client; int status;
    if (!syn_ack || !client) return -1;
    if (net_socket_connection_snapshot(socket_id, &previous_connection) != 0) return -2;
    previous_client = *client;
    if (net_socket_accept_syn_ack(socket_id, syn_ack) != 0) return -3;
    status = ne2k_socket_tls_start(device, io, cache, tx_frame, tx_capacity, local_ip, remote_ip,
                                   socket_id, client, client_random, client_hello_record,
                                   client_hello_capacity, tcp_segment, tcp_segment_capacity, retransmit_limit);
    if (status < 0) { (void)net_socket_connection_restore(socket_id, &previous_connection); *client = previous_client; return -4; }
    return status;
}

int ne2k_tls_client_accept_syn_ack_start(ne2k_device_t* device,const ne2k_io_t* io,const net_arp_cache_t* cache,uint8_t* tx_frame,uint16_t tx_capacity,const uint8_t local_ip[4],const uint8_t remote_ip[4],const net_tcp_view_t* syn_ack,net_tcp_connection_t* connection,ne2k_tls_client_t* client,const uint8_t client_random[32],uint8_t* client_hello_record,uint32_t client_hello_capacity,uint8_t retransmit_limit){net_tcp_connection_t next_connection;ne2k_tls_client_t next_client;int status;if(!device||!io||!cache||!tx_frame||!local_ip||!remote_ip||!syn_ack||!connection||!client||!client_random||!client_hello_record)return -1;next_connection=*connection;next_client=*client;if(net_tcp_connection_accept_syn_ack(&next_connection,syn_ack)!=0)return -2;status=ne2k_tls_client_start(device,io,cache,tx_frame,tx_capacity,local_ip,remote_ip,&next_connection,&next_client,client_random,client_hello_record,client_hello_capacity,retransmit_limit);if(status<0)return -3;*connection=next_connection;*client=next_client;return status;}
int ne2k_llm_syn_ack_tls_start(ne2k_device_t* device,const ne2k_io_t* io,const net_arp_cache_t* cache,uint8_t* rx_frame,uint16_t rx_capacity,uint8_t* tx_frame,uint16_t tx_capacity,const uint8_t local_ip[4],const uint8_t remote_ip[4],net_tcp_connection_t* connection,ne2k_tls_client_t* client,const uint8_t client_random[32],uint8_t* client_hello_record,uint32_t client_hello_capacity,uint8_t retransmit_limit){net_tcp_view_t syn_ack;uint16_t frame_length=0U;int status;if(!device||!io||!cache||!rx_frame||!tx_frame||!local_ip||!remote_ip||!connection||!client||!client_random||!client_hello_record)return -1;status=ne2k_rx_poll_tcp(device,io,rx_frame,rx_capacity,&frame_length,&syn_ack);if(status!=0)return status;return ne2k_tls_client_accept_syn_ack_start(device,io,cache,tx_frame,tx_capacity,local_ip,remote_ip,&syn_ack,connection,client,client_random,client_hello_record,client_hello_capacity,retransmit_limit);}

static int ne2k_tls_client_poll_internal(ne2k_device_t* device,const ne2k_io_t* io,const net_arp_cache_t* cache,
                         uint8_t* rx_frame,uint16_t rx_capacity,uint8_t* tx_frame,uint16_t tx_capacity,
                         const uint8_t local_ip[4],const uint8_t remote_ip[4],net_tcp_connection_t* connection,
                         ne2k_tls_client_t* client,const uint8_t client_random[32],const uint8_t client_private[NET_TLS_X25519_KEY_LENGTH],
                         const x509_certificate_view_t* intermediate,const x509_certificate_view_t* trust_anchor,const char* hostname,const char* utc_time,
                         uint32_t* rsa_workspace,uint16_t rsa_workspace_length,
                         uint32_t* x25519_workspace,uint16_t x25519_workspace_length,
                         uint8_t* prf_workspace,uint32_t prf_workspace_capacity,
                         uint8_t* tcp_segment,uint32_t tcp_segment_capacity,
                         uint8_t* flight_records,uint32_t flight_records_capacity,uint32_t* flight_records_length,
                         uint8_t* plaintext,uint16_t plaintext_capacity,uint8_t retransmit_limit,uint16_t* consumed){
    static ne2k_tls_client_t previous_client;static net_tcp_connection_t previous_connection;net_tcp_view_t view;uint16_t frame_length=0U;uint32_t local_flight_length=0U;int status;
    if(!device||!io||!cache||!rx_frame||!tx_frame||!local_ip||!remote_ip||!connection||!client||!client_random||!client_private||!trust_anchor||!hostname||!utc_time||!rsa_workspace||!x25519_workspace||!prf_workspace||!tcp_segment||!flight_records||!flight_records_length||!plaintext||!consumed)return -1;
    *consumed=0U;*flight_records_length=0U;
    status=ne2k_rx_poll_tcp(device,io,rx_frame,rx_capacity,&frame_length,&view);
    if(status!=0)return status;
    previous_client=*client;previous_connection=*connection;
    if(client->handshake.state==NET_TLS_HANDSHAKE_FINISHED_SENT||client->handshake.state==NET_TLS_HANDSHAKE_SERVER_CHANGE_CIPHER_SPEC_RECEIVED){
        status=net_tcp_connection_accept_tls_x25519_postflight(connection,&client->handshake,&client->transcript,client->master_secret,&client->session,&view,plaintext,plaintext_capacity,prf_workspace,prf_workspace_capacity,consumed);
        if(status!=0)goto rollback;
        if(ne2k_tcp_ack(device,io,cache,tx_frame,tx_capacity,local_ip,remote_ip,connection)!=0)goto rollback;
        if(net_tls_handshake_is_complete(&client->handshake))client->complete=1U;
        return 0;
    }
    status=net_tcp_connection_accept_tls_authenticated_fragment(connection,&view,&client->stream,&client->handshake,client_random,&client->transcript,rsa_workspace,rsa_workspace_length,consumed);
    if(status<0)goto rollback;
    if(ne2k_tcp_ack(device,io,cache,tx_frame,tx_capacity,local_ip,remote_ip,connection)!=0)goto rollback;
    if(status==1)return 1;
    if(client->handshake.state==NET_TLS_HANDSHAKE_CERTIFICATE_RECEIVED){
        if(client->handshake.server_intermediate_two){
            if(!client->handshake.server_intermediate_x509_valid||!client->handshake.server_intermediate_two_x509_valid)goto rollback;
            if(x509_certificate_tls_identity_validate_three(&client->handshake.server_x509,&client->handshake.server_intermediate_x509,&client->handshake.server_intermediate_two_x509,trust_anchor,hostname,utc_time,rsa_workspace,rsa_workspace_length)!=0)goto rollback;
        }else{
            if(intermediate&&!client->handshake.server_intermediate_x509_valid)goto rollback;
            if((intermediate?x509_certificate_tls_identity_validate_two(&client->handshake.server_x509,intermediate,trust_anchor,hostname,utc_time,rsa_workspace,rsa_workspace_length):x509_certificate_tls_identity_validate(&client->handshake.server_x509,trust_anchor,hostname,utc_time,rsa_workspace,rsa_workspace_length))!=0)goto rollback;
        }
        client->peer_identity_validated=1U;
    }
    if(client->handshake.state!=NET_TLS_HANDSHAKE_SERVER_HELLO_DONE_RECEIVED)return 0;
    if(!client->peer_identity_validated||client->handshake.certificate_requested)return 0;
    status=net_tcp_connection_build_tls_x25519_flight(connection,&client->handshake,&client->x25519,client_private,client_random,&client->transcript,client->master_secret,client->key_block,&client->session,tcp_segment,tcp_segment_capacity,flight_records,flight_records_capacity,&local_flight_length,x25519_workspace,x25519_workspace_length,prf_workspace,prf_workspace_capacity,retransmit_limit);
    if(status<0)goto rollback;
    if(ne2k_tcp_data(device,io,cache,tx_frame,tx_capacity,local_ip,remote_ip,connection,flight_records,(uint16_t)local_flight_length)!=0)goto rollback;
    if(net_tcp_connection_commit_send(connection,(uint16_t)local_flight_length)!=0)goto rollback;
    *flight_records_length=local_flight_length;
    return 0;
rollback:
    *client=previous_client;*connection=previous_connection;*consumed=0U;*flight_records_length=0U;return -2;
}
int ne2k_socket_tls_poll(ne2k_device_t* device,const ne2k_io_t* io,const net_arp_cache_t* cache,
                         uint8_t* rx_frame,uint16_t rx_capacity,uint8_t* tx_frame,uint16_t tx_capacity,
                         const uint8_t local_ip[4],const uint8_t remote_ip[4],int socket_id,
                         ne2k_tls_client_t* client,const uint8_t client_random[32],
                         const uint8_t client_private[NET_TLS_X25519_KEY_LENGTH],
                         const x509_certificate_view_t* trust_anchor,const char* hostname,const char* utc_time,
                         uint32_t* rsa_workspace,uint16_t rsa_workspace_length,
                         uint32_t* x25519_workspace,uint16_t x25519_workspace_length,
                         uint8_t* prf_workspace,uint32_t prf_workspace_capacity,
                         uint8_t* tcp_segment,uint32_t tcp_segment_capacity,
                         uint8_t* flight_records,uint32_t flight_records_capacity,uint32_t* flight_records_length,
                         uint8_t* plaintext,uint16_t plaintext_capacity,uint8_t retransmit_limit,uint16_t* consumed) {
    static ne2k_tls_client_t previous_client;
    static net_tcp_connection_t previous_connection;
    net_tcp_view_t view;
    uint16_t frame_length = 0U; uint32_t local_flight_length = 0U; int status;
    uint8_t acked_this_poll = 0U;
    if (!device || !io || !cache || !rx_frame || !tx_frame || !local_ip || !remote_ip || !client ||
        !client_random || !client_private || !trust_anchor || !hostname || !utc_time || !rsa_workspace ||
        !x25519_workspace || !prf_workspace || !tcp_segment || !flight_records || !flight_records_length ||
        !plaintext || !consumed) return -1;
    *consumed = 0U; *flight_records_length = 0U;
    if (net_socket_connection_snapshot(socket_id, &previous_connection) != 0) return -2;
    previous_client = *client;
    if (client->handshake.state != NET_TLS_HANDSHAKE_FINISHED_SENT &&
        client->handshake.state != NET_TLS_HANDSHAKE_SERVER_CHANGE_CIPHER_SPEC_RECEIVED) {
        status = net_tcp_tls_stream_accept_pending(&client->stream, &client->handshake, client_random,
                                                   &client->transcript, rsa_workspace, rsa_workspace_length);
        if (status < 0) goto rollback_socket;
        if (status == 0) goto after_handshake_message;
    }
    status = ne2k_rx_poll_tcp(device, io, rx_frame, rx_capacity, &frame_length, &view);
    if (status != 0) {
        /* Apres ServerHelloDone, emettre le vol sur un sondage sans ACK :
         * QEMU perd souvent la trame si elle part dans le meme tour que l'ACK. */
        if (status == 1 &&
            client->handshake.state == NET_TLS_HANDSHAKE_SERVER_HELLO_DONE_RECEIVED &&
            client->peer_identity_validated && !client->handshake.certificate_requested)
            goto after_handshake_message;
        return status;
    }
    if (client->handshake.state == NET_TLS_HANDSHAKE_FINISHED_SENT ||
        client->handshake.state == NET_TLS_HANDSHAKE_SERVER_CHANGE_CIPHER_SPEC_RECEIVED) {
        status = net_socket_accept_tls_x25519_postflight(socket_id, &client->handshake,
                                                          &client->transcript, client->master_secret,
                                                          &client->session, &view, plaintext,
                                                          plaintext_capacity, prf_workspace,
                                                          prf_workspace_capacity, consumed);
        if (status != 0 || ne2k_socket_ack(device, io, cache, tx_frame, tx_capacity,
                                            local_ip, remote_ip, socket_id) != 0) goto rollback_socket;
        if (net_tls_handshake_is_complete(&client->handshake)) client->complete = 1U;
        return 0;
    }
    status = net_socket_accept_tls_authenticated_fragment(socket_id, &view, &client->stream,
                                                           &client->handshake, client_random,
                                                           &client->transcript, rsa_workspace,
                                                           rsa_workspace_length, consumed);
    if (status < 0 || ne2k_socket_ack(device, io, cache, tx_frame, tx_capacity,
                                      local_ip, remote_ip, socket_id) != 0) goto rollback_socket;
    acked_this_poll = 1U;
    if (status == 1) return 1;
after_handshake_message:
    if (client->handshake.state == NET_TLS_HANDSHAKE_CERTIFICATE_RECEIVED) {
        if (client->handshake.server_intermediate_two) {
            if (!client->handshake.server_intermediate_x509_valid ||
                !client->handshake.server_intermediate_two_x509_valid ||
                x509_certificate_tls_identity_validate_three(&client->handshake.server_x509,
                                                              &client->handshake.server_intermediate_x509,
                                                              &client->handshake.server_intermediate_two_x509,
                                                              trust_anchor, hostname, utc_time,
                                                              rsa_workspace, rsa_workspace_length) != 0) goto rollback_socket;
        } else if (client->handshake.server_intermediate) {
            if (!client->handshake.server_intermediate_x509_valid ||
                x509_certificate_tls_identity_validate_two(&client->handshake.server_x509,
                                                           &client->handshake.server_intermediate_x509,
                                                           trust_anchor, hostname, utc_time, rsa_workspace,
                                                           rsa_workspace_length) != 0) goto rollback_socket;
        } else if (x509_certificate_tls_identity_validate(&client->handshake.server_x509, trust_anchor,
                                                           hostname, utc_time, rsa_workspace,
                                                           rsa_workspace_length) != 0) goto rollback_socket;
        client->peer_identity_validated = 1U;
    }
    if (client->handshake.state != NET_TLS_HANDSHAKE_SERVER_HELLO_DONE_RECEIVED ||
        !client->peer_identity_validated || client->handshake.certificate_requested) return 0;
    if (acked_this_poll) return 0;
    status = net_socket_build_tls_x25519_flight(socket_id, &client->handshake, &client->x25519,
                                                 client_private, client_random, &client->transcript,
                                                 client->master_secret, client->key_block, &client->session,
                                                 tcp_segment, (uint16_t)tcp_segment_capacity, flight_records,
                                                 flight_records_capacity, &local_flight_length,
                                                 x25519_workspace, x25519_workspace_length, prf_workspace,
                                                 prf_workspace_capacity, retransmit_limit);
    if (status < 0 || local_flight_length == 0U || status > (int)tcp_segment_capacity ||
        ne2k_tcp_segment(device, io, cache, tx_frame, tx_capacity, local_ip, remote_ip,
                         tcp_segment, (uint16_t)status) != 0 ||
        net_socket_commit_send(socket_id, (uint16_t)local_flight_length) != 0) goto rollback_socket;
    *flight_records_length = local_flight_length;
    return 0;
rollback_socket:
    (void)net_socket_connection_restore(socket_id, &previous_connection);
    *client = previous_client; *consumed = 0U; *flight_records_length = 0U;
    return -3;
}

int ne2k_tls_client_poll(ne2k_device_t* device,const ne2k_io_t* io,const net_arp_cache_t* cache,uint8_t* rx_frame,uint16_t rx_capacity,uint8_t* tx_frame,uint16_t tx_capacity,const uint8_t local_ip[4],const uint8_t remote_ip[4],net_tcp_connection_t* connection,ne2k_tls_client_t* client,const uint8_t client_random[32],const uint8_t client_private[NET_TLS_X25519_KEY_LENGTH],const x509_certificate_view_t* trust_anchor,const char* hostname,const char* utc_time,uint32_t* rsa_workspace,uint16_t rsa_workspace_length,uint32_t* x25519_workspace,uint16_t x25519_workspace_length,uint8_t* prf_workspace,uint32_t prf_workspace_capacity,uint8_t* tcp_segment,uint32_t tcp_segment_capacity,uint8_t* flight_records,uint32_t flight_records_capacity,uint32_t* flight_records_length,uint8_t* plaintext,uint16_t plaintext_capacity,uint8_t retransmit_limit,uint16_t* consumed){return ne2k_tls_client_poll_internal(device,io,cache,rx_frame,rx_capacity,tx_frame,tx_capacity,local_ip,remote_ip,connection,client,client_random,client_private,0,trust_anchor,hostname,utc_time,rsa_workspace,rsa_workspace_length,x25519_workspace,x25519_workspace_length,prf_workspace,prf_workspace_capacity,tcp_segment,tcp_segment_capacity,flight_records,flight_records_capacity,flight_records_length,plaintext,plaintext_capacity,retransmit_limit,consumed);}
int ne2k_tls_client_poll_chain_two(ne2k_device_t* device,const ne2k_io_t* io,const net_arp_cache_t* cache,uint8_t* rx_frame,uint16_t rx_capacity,uint8_t* tx_frame,uint16_t tx_capacity,const uint8_t local_ip[4],const uint8_t remote_ip[4],net_tcp_connection_t* connection,ne2k_tls_client_t* client,const uint8_t client_random[32],const uint8_t client_private[NET_TLS_X25519_KEY_LENGTH],const x509_certificate_view_t* intermediate,const x509_certificate_view_t* trust_anchor,const char* hostname,const char* utc_time,uint32_t* rsa_workspace,uint16_t rsa_workspace_length,uint32_t* x25519_workspace,uint16_t x25519_workspace_length,uint8_t* prf_workspace,uint32_t prf_workspace_capacity,uint8_t* tcp_segment,uint32_t tcp_segment_capacity,uint8_t* flight_records,uint32_t flight_records_capacity,uint32_t* flight_records_length,uint8_t* plaintext,uint16_t plaintext_capacity,uint8_t retransmit_limit,uint16_t* consumed){if(!intermediate)return -1;return ne2k_tls_client_poll_internal(device,io,cache,rx_frame,rx_capacity,tx_frame,tx_capacity,local_ip,remote_ip,connection,client,client_random,client_private,intermediate,trust_anchor,hostname,utc_time,rsa_workspace,rsa_workspace_length,x25519_workspace,x25519_workspace_length,prf_workspace,prf_workspace_capacity,tcp_segment,tcp_segment_capacity,flight_records,flight_records_capacity,flight_records_length,plaintext,plaintext_capacity,retransmit_limit,consumed);}
int ne2k_tls_client_poll_received_chain(ne2k_device_t* device,const ne2k_io_t* io,const net_arp_cache_t* cache,uint8_t* rx_frame,uint16_t rx_capacity,uint8_t* tx_frame,uint16_t tx_capacity,const uint8_t local_ip[4],const uint8_t remote_ip[4],net_tcp_connection_t* connection,ne2k_tls_client_t* client,const uint8_t client_random[32],const uint8_t client_private[NET_TLS_X25519_KEY_LENGTH],const x509_certificate_view_t* trust_anchor,const char* hostname,const char* utc_time,uint32_t* rsa_workspace,uint16_t rsa_workspace_length,uint32_t* x25519_workspace,uint16_t x25519_workspace_length,uint8_t* prf_workspace,uint32_t prf_workspace_capacity,uint8_t* tcp_segment,uint32_t tcp_segment_capacity,uint8_t* flight_records,uint32_t flight_records_capacity,uint32_t* flight_records_length,uint8_t* plaintext,uint16_t plaintext_capacity,uint8_t retransmit_limit,uint16_t* consumed){if(!client)return -1;return ne2k_tls_client_poll_chain_two(device,io,cache,rx_frame,rx_capacity,tx_frame,tx_capacity,local_ip,remote_ip,connection,client,client_random,client_private,&client->handshake.server_intermediate_x509,trust_anchor,hostname,utc_time,rsa_workspace,rsa_workspace_length,x25519_workspace,x25519_workspace_length,prf_workspace,prf_workspace_capacity,tcp_segment,tcp_segment_capacity,flight_records,flight_records_capacity,flight_records_length,plaintext,plaintext_capacity,retransmit_limit,consumed);}
int ne2k_tls_client_poll_received_chain_rtc(ne2k_device_t* device,const ne2k_io_t* io,const net_arp_cache_t* cache,uint8_t* rx_frame,uint16_t rx_capacity,uint8_t* tx_frame,uint16_t tx_capacity,const uint8_t local_ip[4],const uint8_t remote_ip[4],net_tcp_connection_t* connection,ne2k_tls_client_t* client,const uint8_t client_random[32],const uint8_t client_private[NET_TLS_X25519_KEY_LENGTH],const x509_certificate_view_t* trust_anchor,const char* hostname,const rtc_io_t* rtc_io,uint32_t* rsa_workspace,uint16_t rsa_workspace_length,uint32_t* x25519_workspace,uint16_t x25519_workspace_length,uint8_t* prf_workspace,uint32_t prf_workspace_capacity,uint8_t* tcp_segment,uint32_t tcp_segment_capacity,uint8_t* flight_records,uint32_t flight_records_capacity,uint32_t* flight_records_length,uint8_t* plaintext,uint16_t plaintext_capacity,uint8_t retransmit_limit,uint16_t* consumed){char utc_time[RTC_UTC_BUFFER_LENGTH];if(rtc_read_utc(rtc_io,utc_time,sizeof(utc_time))!=0)return -1;return ne2k_tls_client_poll_received_chain(device,io,cache,rx_frame,rx_capacity,tx_frame,tx_capacity,local_ip,remote_ip,connection,client,client_random,client_private,trust_anchor,hostname,utc_time,rsa_workspace,rsa_workspace_length,x25519_workspace,x25519_workspace_length,prf_workspace,prf_workspace_capacity,tcp_segment,tcp_segment_capacity,flight_records,flight_records_capacity,flight_records_length,plaintext,plaintext_capacity,retransmit_limit,consumed);}

int ne2k_socket_llm_request(ne2k_device_t* device,const ne2k_io_t* io,const net_arp_cache_t* cache,
                            uint8_t* tx_frame,uint16_t tx_capacity,const uint8_t local_ip[4],
                            const uint8_t remote_ip[4],int socket_id,net_tls_aes_gcm_session_t* session,
                            uint8_t provider,uint8_t stream,uint8_t* json,uint16_t json_capacity,
                            uint8_t* request,uint16_t request_capacity,const char* host,const char* path,
                            const char* bearer_token,const char* model,const uint8_t* prompt,uint16_t prompt_length,
                            uint8_t* tls_record,uint32_t tls_capacity,uint8_t* tcp_segment,
                            uint16_t tcp_segment_capacity,uint8_t retransmit_limit) {
    net_tcp_connection_t previous_connection;
    net_tls_aes_gcm_session_t previous_session;
    int status;
    if (!device || !io || !cache || !tx_frame || !local_ip || !remote_ip || !session || !json ||
        !request || !host || !path || !model || (!prompt && prompt_length) || !tls_record ||
        !tcp_segment) return -1;
    if (net_socket_connection_snapshot(socket_id, &previous_connection) != 0) return -2;
    previous_session = *session;
    status = net_llm_socket_build_request(socket_id, session, provider, stream, json, json_capacity,
                                          request, request_capacity, host, path, bearer_token, model,
                                          prompt, prompt_length, tls_record, tls_capacity, tcp_segment,
                                          tcp_segment_capacity, retransmit_limit);
    if (status < 0 || ne2k_tcp_segment(device, io, cache, tx_frame, tx_capacity, local_ip, remote_ip,
                                       tcp_segment, (uint16_t)status) != 0) goto rollback_request;
    return status;
rollback_request:
    (void)net_socket_connection_restore(socket_id, &previous_connection);
    *session = previous_session;
    return -3;
}
int ne2k_socket_llm_resume_sse(ne2k_device_t* device,const ne2k_io_t* io,const net_arp_cache_t* cache,
                               uint8_t* tx_frame,uint16_t tx_capacity,const uint8_t local_ip[4],
                               const uint8_t remote_ip[4],int socket_id,net_tls_aes_gcm_session_t* session,
                               uint8_t* request,uint16_t request_capacity,const char* host,const char* path,
                               const net_llm_sse_response_t* response,uint8_t* tls_record,uint32_t tls_capacity,
                               uint8_t* tcp_segment,uint16_t tcp_segment_capacity,uint8_t retransmit_limit) {
    net_tcp_connection_t previous_connection;
    net_tls_aes_gcm_session_t previous_session;
    int status;
    if (!device || !io || !cache || !tx_frame || !local_ip || !remote_ip || !session || !request ||
        !host || !path || !response || !tls_record || !tcp_segment) return -1;
    if (net_socket_connection_snapshot(socket_id, &previous_connection) != 0) return -2;
    previous_session = *session;
    status = net_llm_socket_build_sse_resume(socket_id, session, request, request_capacity, host, path,
                                              response, tls_record, tls_capacity, tcp_segment,
                                              tcp_segment_capacity, retransmit_limit);
    if (status < 0 || ne2k_tcp_segment(device, io, cache, tx_frame, tx_capacity, local_ip, remote_ip,
                                       tcp_segment, (uint16_t)status) != 0) goto rollback_resume;
    return status;
rollback_resume:
    (void)net_socket_connection_restore(socket_id, &previous_connection);
    *session = previous_session;
    return -3;
}
int ne2k_socket_llm_poll_response(ne2k_device_t* device,const ne2k_io_t* io,const net_arp_cache_t* cache,
                                  uint8_t* rx_frame,uint16_t rx_capacity,uint8_t* tx_frame,uint16_t tx_capacity,
                                  const uint8_t local_ip[4],const uint8_t remote_ip[4],int socket_id,
                                  net_tls_aes_gcm_session_t* session,uint8_t* plaintext,uint16_t plaintext_capacity,
                                  net_http_response_accumulator_t* accumulator,net_http_response_view_t* response,
                                  uint16_t* consumed) {
    net_tcp_connection_t previous_connection; net_tls_aes_gcm_session_t previous_session;
    net_http_response_accumulator_t previous_accumulator; net_http_response_view_t previous_response;
    net_tcp_view_t view; uint16_t frame_length = 0U; int status;
    if (!device || !io || !cache || !rx_frame || !tx_frame || !local_ip || !remote_ip || !session ||
        !plaintext || !accumulator || !response || !consumed) return -1;
    *consumed = 0U;
    status = ne2k_rx_poll_tcp(device, io, rx_frame, rx_capacity, &frame_length, &view);
    if (status != 0) return status;
    if (net_socket_connection_snapshot(socket_id, &previous_connection) != 0) return -2;
    previous_session = *session; previous_accumulator = *accumulator; previous_response = *response;
    status = net_llm_socket_open_response(socket_id, session, &view, plaintext, plaintext_capacity,
                                          accumulator, response, consumed);
    if (status < 0 || ne2k_socket_ack(device, io, cache, tx_frame, tx_capacity,
                                      local_ip, remote_ip, socket_id) != 0) goto rollback_response;
    return status;
rollback_response:
    (void)net_socket_connection_restore(socket_id, &previous_connection);
    *session = previous_session; *accumulator = previous_accumulator; *response = previous_response;
    *consumed = 0U;
    return -3;
}

int ne2k_socket_llm_poll_sse(ne2k_device_t* device,const ne2k_io_t* io,const net_arp_cache_t* cache,
                             uint8_t* rx_frame,uint16_t rx_capacity,uint8_t* tx_frame,uint16_t tx_capacity,
                             const uint8_t local_ip[4],const uint8_t remote_ip[4],int socket_id,
                             net_tls_aes_gcm_session_t* session,uint8_t* plaintext,uint16_t plaintext_capacity,
                             net_llm_sse_response_t* response,uint8_t provider,uint8_t* text,
                             uint16_t text_capacity,uint16_t* text_length,uint16_t* consumed) {
    net_tcp_connection_t previous_connection; net_tls_aes_gcm_session_t previous_session;
    net_llm_sse_response_t previous_response; net_tcp_view_t view; uint16_t frame_length = 0U; int status;
    if (!device || !io || !cache || !rx_frame || !tx_frame || !local_ip || !remote_ip || !session ||
        !plaintext || !response || !text || !text_length || !consumed) return -1;
    *text_length = 0U; *consumed = 0U;
    status = ne2k_rx_poll_tcp(device, io, rx_frame, rx_capacity, &frame_length, &view);
    if (status != 0) return status;
    if (net_socket_connection_snapshot(socket_id, &previous_connection) != 0) return -2;
    previous_session = *session; previous_response = *response;
    status = net_llm_socket_open_sse(socket_id, session, &view, plaintext, plaintext_capacity,
                                     response, provider, text, text_capacity, text_length, consumed);
    if (status < 0 || ne2k_socket_ack(device, io, cache, tx_frame, tx_capacity,
                                      local_ip, remote_ip, socket_id) != 0) goto rollback_sse;
    return status;
rollback_sse:
    (void)net_socket_connection_restore(socket_id, &previous_connection);
    *session = previous_session; *response = previous_response; *text_length = 0U; *consumed = 0U;
    return -3;
}

int ne2k_https_llm_post_json(ne2k_device_t* device,const ne2k_io_t* io,const net_arp_cache_t* cache,
                             uint8_t* tx_frame,uint16_t tx_capacity,const uint8_t local_ip[4],const uint8_t remote_ip[4],
                             net_tcp_connection_t* connection,ne2k_tls_client_t* client,
                             uint8_t* request,uint16_t request_capacity,const char* host,const char* path,
                             const uint8_t* json,uint16_t json_length,uint8_t* tls_record,uint32_t tls_capacity,
                             uint8_t retransmit_limit){
    net_tcp_connection_t previous_connection;uint64_t previous_sequence;int request_length,record_length,status;
    if(!device||!io||!cache||!tx_frame||!local_ip||!remote_ip||!connection||!client||!request||!host||!path||(!json&&json_length)||!tls_record)return -1;
    if(!client->complete||!net_tls_handshake_is_complete(&client->handshake))return -2;
    previous_connection=*connection;previous_sequence=client->session.write_sequence;
    request_length=net_http_build_post_json(request,request_capacity,host,path,json,json_length);
    if(request_length<0)return -3;
    record_length=net_tls_aes_gcm_session_build(&client->session,tls_record,tls_capacity,NET_TLS_CONTENT_APPLICATION_DATA,request,(uint16_t)request_length);
    if(record_length<0)goto rollback;
    if(net_tcp_connection_track_send(connection,tls_record,(uint16_t)record_length,retransmit_limit)!=0)goto rollback;
    status=ne2k_tcp_data(device,io,cache,tx_frame,tx_capacity,local_ip,remote_ip,connection,tls_record,(uint16_t)record_length);
    if(status!=0)goto rollback;
    if(net_tcp_connection_commit_send(connection,(uint16_t)record_length)!=0)goto rollback;
    return record_length;
rollback:
    *connection=previous_connection;client->session.write_sequence=previous_sequence;return -4;
}

int ne2k_https_llm_poll_response(ne2k_device_t* device,const ne2k_io_t* io,const net_arp_cache_t* cache,
                                  uint8_t* rx_frame,uint16_t rx_capacity,uint8_t* tx_frame,uint16_t tx_capacity,
                                  const uint8_t local_ip[4],const uint8_t remote_ip[4],net_tcp_connection_t* connection,
                                  ne2k_tls_client_t* client,uint8_t* plaintext,uint16_t plaintext_capacity,
                                  net_http_response_accumulator_t* accumulator,net_http_response_view_t* response,uint16_t* consumed){
    ne2k_tls_client_t previous_client;net_tcp_connection_t previous_connection;net_tcp_view_t view;uint16_t frame_length=0U;int status;
    if(!device||!io||!cache||!rx_frame||!tx_frame||!local_ip||!remote_ip||!connection||!client||!plaintext||!accumulator||!response||!consumed)return -1;
    if(!client->complete||!net_tls_handshake_is_complete(&client->handshake))return -2;
    *consumed=0U;status=ne2k_rx_poll_tcp(device,io,rx_frame,rx_capacity,&frame_length,&view);if(status!=0)return status;
    previous_client=*client;previous_connection=*connection;
    status=net_http_tls_open_response_stream(connection,&client->session,&view,plaintext,plaintext_capacity,accumulator,response,consumed);
    if(status<0)goto rollback;
    if(ne2k_tcp_ack(device,io,cache,tx_frame,tx_capacity,local_ip,remote_ip,connection)!=0)goto rollback;
    return status;
rollback:
    *client=previous_client;*connection=previous_connection;*consumed=0U;return -3;
}

int ne2k_https_llm_request(ne2k_device_t* device,const ne2k_io_t* io,const net_arp_cache_t* cache,uint8_t* tx_frame,uint16_t tx_capacity,const uint8_t local_ip[4],const uint8_t remote_ip[4],net_tcp_connection_t* connection,ne2k_tls_client_t* client,uint8_t provider,uint8_t* json,uint16_t json_capacity,uint8_t* request,uint16_t request_capacity,const char* host,const char* path,const char* bearer_token,const char* model,const uint8_t* prompt,uint16_t prompt_length,uint8_t* tls_record,uint32_t tls_capacity,uint8_t retransmit_limit){net_tcp_connection_t previous_connection;uint64_t previous_sequence;int json_length,request_length,record_length,status;if(!device||!io||!cache||!tx_frame||!local_ip||!remote_ip||!connection||!client||!json||!request||!host||!path||!model||(!prompt&&prompt_length)||!tls_record)return -1;if(provider!=NE2K_LLM_PROVIDER_OLLAMA&&provider!=NE2K_LLM_PROVIDER_OPENAI)return -2;if(provider==NE2K_LLM_PROVIDER_OPENAI&&(!bearer_token||!bearer_token[0]))return -3;if(!client->complete||!net_tls_handshake_is_complete(&client->handshake))return -4;json_length=provider==NE2K_LLM_PROVIDER_OLLAMA?net_llm_build_ollama_generate_json(json,json_capacity,model,prompt,prompt_length):net_llm_build_openai_chat_json(json,json_capacity,model,prompt,prompt_length);if(json_length<0)return -5;request_length=(bearer_token&&bearer_token[0])?net_http_build_post_json_bearer(request,request_capacity,host,path,bearer_token,json,(uint16_t)json_length):net_http_build_post_json(request,request_capacity,host,path,json,(uint16_t)json_length);if(request_length<0)return -6;previous_connection=*connection;previous_sequence=client->session.write_sequence;record_length=net_tls_aes_gcm_session_build(&client->session,tls_record,tls_capacity,NET_TLS_CONTENT_APPLICATION_DATA,request,(uint16_t)request_length);if(record_length<0)goto rollback;if(net_tcp_connection_track_send(connection,tls_record,(uint16_t)record_length,retransmit_limit)!=0)goto rollback;status=ne2k_tcp_data(device,io,cache,tx_frame,tx_capacity,local_ip,remote_ip,connection,tls_record,(uint16_t)record_length);if(status!=0)goto rollback;if(net_tcp_connection_commit_send(connection,(uint16_t)record_length)!=0)goto rollback;return record_length;rollback:*connection=previous_connection;client->session.write_sequence=previous_sequence;return -7;}

int ne2k_https_llm_stream_request(ne2k_device_t* device,const ne2k_io_t* io,const net_arp_cache_t* cache,uint8_t* tx_frame,uint16_t tx_capacity,const uint8_t local_ip[4],const uint8_t remote_ip[4],net_tcp_connection_t* connection,ne2k_tls_client_t* client,uint8_t provider,uint8_t* json,uint16_t json_capacity,uint8_t* request,uint16_t request_capacity,const char* host,const char* path,const char* bearer_token,const char* model,const uint8_t* prompt,uint16_t prompt_length,uint8_t* tls_record,uint32_t tls_capacity,uint8_t retransmit_limit){net_tcp_connection_t previous_connection;uint64_t previous_sequence;int json_length,request_length,record_length,status;if(!device||!io||!cache||!tx_frame||!local_ip||!remote_ip||!connection||!client||!json||!request||!host||!path||!model||(!prompt&&prompt_length)||!tls_record)return -1;if(provider!=NE2K_LLM_PROVIDER_OLLAMA&&provider!=NE2K_LLM_PROVIDER_OPENAI)return -2;if(provider==NE2K_LLM_PROVIDER_OPENAI&&(!bearer_token||!bearer_token[0]))return -3;if(!client->complete||!net_tls_handshake_is_complete(&client->handshake))return -4;json_length=provider==NE2K_LLM_PROVIDER_OLLAMA?net_llm_build_ollama_generate_stream_json(json,json_capacity,model,prompt,prompt_length):net_llm_build_openai_chat_stream_json(json,json_capacity,model,prompt,prompt_length);if(json_length<0)return -5;request_length=(bearer_token&&bearer_token[0])?net_http_build_post_json_bearer(request,request_capacity,host,path,bearer_token,json,(uint16_t)json_length):net_http_build_post_json(request,request_capacity,host,path,json,(uint16_t)json_length);if(request_length<0)return -6;previous_connection=*connection;previous_sequence=client->session.write_sequence;record_length=net_tls_aes_gcm_session_build(&client->session,tls_record,tls_capacity,NET_TLS_CONTENT_APPLICATION_DATA,request,(uint16_t)request_length);if(record_length<0)goto rollback;if(net_tcp_connection_track_send(connection,tls_record,(uint16_t)record_length,retransmit_limit)!=0)goto rollback;status=ne2k_tcp_data(device,io,cache,tx_frame,tx_capacity,local_ip,remote_ip,connection,tls_record,(uint16_t)record_length);if(status!=0)goto rollback;if(net_tcp_connection_commit_send(connection,(uint16_t)record_length)!=0)goto rollback;return record_length;rollback:*connection=previous_connection;client->session.write_sequence=previous_sequence;return -7;}
int ne2k_https_llm_poll_sse(ne2k_device_t* device,const ne2k_io_t* io,const net_arp_cache_t* cache,uint8_t* rx_frame,uint16_t rx_capacity,uint8_t* tx_frame,uint16_t tx_capacity,const uint8_t local_ip[4],const uint8_t remote_ip[4],net_tcp_connection_t* connection,ne2k_tls_client_t* client,uint8_t provider,uint8_t* plaintext,uint16_t plaintext_capacity,net_llm_sse_response_t* response,uint8_t* text,uint16_t text_capacity,uint16_t* text_length,uint16_t* consumed){ne2k_tls_client_t previous_client;net_tcp_connection_t previous_connection;net_llm_sse_response_t previous_response;net_tcp_view_t view;uint16_t frame_length=0U;int status;if(!device||!io||!cache||!rx_frame||!tx_frame||!local_ip||!remote_ip||!connection||!client||!plaintext||!response||!text||!text_length||!consumed)return -1;if(provider!=NE2K_LLM_PROVIDER_OLLAMA&&provider!=NE2K_LLM_PROVIDER_OPENAI)return -2;if(!client->complete||!net_tls_handshake_is_complete(&client->handshake))return -3;*text_length=0U;*consumed=0U;status=ne2k_rx_poll_tcp(device,io,rx_frame,rx_capacity,&frame_length,&view);if(status!=0)return status;previous_client=*client;previous_connection=*connection;previous_response=*response;status=net_http_tls_open_sse_stream(connection,&client->session,&view,plaintext,plaintext_capacity,response,provider,text,text_capacity,text_length,consumed);if(status<0)goto rollback;if(ne2k_tcp_ack(device,io,cache,tx_frame,tx_capacity,local_ip,remote_ip,connection)!=0)goto rollback;return status;rollback:*client=previous_client;*connection=previous_connection;*response=previous_response;*text_length=0U;*consumed=0U;return -4;}

int ne2k_https_llm_poll_text(ne2k_device_t* device,const ne2k_io_t* io,const net_arp_cache_t* cache,uint8_t* rx_frame,uint16_t rx_capacity,uint8_t* tx_frame,uint16_t tx_capacity,const uint8_t local_ip[4],const uint8_t remote_ip[4],net_tcp_connection_t* connection,ne2k_tls_client_t* client,uint8_t provider,uint8_t* plaintext,uint16_t plaintext_capacity,net_http_response_accumulator_t* accumulator,net_http_response_view_t* response,uint8_t* text,uint16_t text_capacity,uint16_t* text_length,uint16_t* consumed){int status;if(!text||!text_length)return -1;if(provider!=NE2K_LLM_PROVIDER_OLLAMA&&provider!=NE2K_LLM_PROVIDER_OPENAI)return -2;*text_length=0U;status=ne2k_https_llm_poll_response(device,io,cache,rx_frame,rx_capacity,tx_frame,tx_capacity,local_ip,remote_ip,connection,client,plaintext,plaintext_capacity,accumulator,response,consumed);if(status!=0)return status;if(response->status_code<200U||response->status_code>=300U)return -3;return provider==NE2K_LLM_PROVIDER_OLLAMA?net_llm_ollama_response_extract(response->body,response->body_length,text,text_capacity,text_length):net_llm_openai_response_extract(response->body,response->body_length,text,text_capacity,text_length);}

int ne2k_https_llm_sse_resume_request(ne2k_device_t* device,const ne2k_io_t* io,const net_arp_cache_t* cache,uint8_t* tx_frame,uint16_t tx_capacity,const uint8_t local_ip[4],const uint8_t remote_ip[4],net_tcp_connection_t* connection,ne2k_tls_client_t* client,net_llm_sse_response_t* response,uint8_t* request,uint16_t request_capacity,const char* host,const char* path,uint8_t* tls_record,uint32_t tls_capacity,uint8_t retransmit_limit){net_tcp_connection_t previous_connection;uint64_t previous_sequence;int request_length,record_length,status;if(!device||!io||!cache||!tx_frame||!local_ip||!remote_ip||!connection||!client||!response||!request||!host||!path||!tls_record)return -1;if(!response->sse.event_id_valid)return -2;if(!client->complete||!net_tls_handshake_is_complete(&client->handshake))return -3;request_length=net_llm_sse_build_resume_get(request,request_capacity,host,path,response);if(request_length<0)return -4;previous_connection=*connection;previous_sequence=client->session.write_sequence;record_length=net_tls_aes_gcm_session_build(&client->session,tls_record,tls_capacity,NET_TLS_CONTENT_APPLICATION_DATA,request,(uint16_t)request_length);if(record_length<0)goto rollback;if(net_tcp_connection_track_send(connection,tls_record,(uint16_t)record_length,retransmit_limit)!=0)goto rollback;status=ne2k_tcp_data(device,io,cache,tx_frame,tx_capacity,local_ip,remote_ip,connection,tls_record,(uint16_t)record_length);if(status!=0)goto rollback;if(net_tcp_connection_commit_send(connection,(uint16_t)record_length)!=0)goto rollback;return record_length;rollback:*connection=previous_connection;client->session.write_sequence=previous_sequence;return -5;}

int ne2k_https_llm_close_notify(ne2k_device_t* device,const ne2k_io_t* io,const net_arp_cache_t* cache,uint8_t* tx_frame,uint16_t tx_capacity,const uint8_t local_ip[4],const uint8_t remote_ip[4],net_tcp_connection_t* connection,ne2k_tls_client_t* client,uint8_t* tls_record,uint32_t tls_capacity,uint8_t retransmit_limit){net_tcp_connection_t previous_connection;uint64_t previous_sequence;int record_length,status;if(!device||!io||!cache||!tx_frame||!local_ip||!remote_ip||!connection||!client||!tls_record)return -1;if(!client->complete||!net_tls_handshake_is_complete(&client->handshake))return -2;previous_connection=*connection;previous_sequence=client->session.write_sequence;record_length=net_tls_close_notify_build(&client->session,tls_record,tls_capacity);if(record_length<0)goto rollback;if(net_tcp_connection_track_send(connection,tls_record,(uint16_t)record_length,retransmit_limit)!=0)goto rollback;status=ne2k_tcp_data(device,io,cache,tx_frame,tx_capacity,local_ip,remote_ip,connection,tls_record,(uint16_t)record_length);if(status!=0)goto rollback;if(net_tcp_connection_commit_send(connection,(uint16_t)record_length)!=0)goto rollback;return record_length;rollback:*connection=previous_connection;client->session.write_sequence=previous_sequence;return -3;}
int ne2k_llm_connection_schedule_sse_retry(ne2k_llm_connection_state_t* state,net_llm_sse_reconnect_t* reconnect,net_llm_sse_response_t* response,uint16_t status_code,uint32_t base_delay,uint32_t max_delay,uint32_t now){ne2k_llm_connection_state_t next_state;net_llm_sse_reconnect_t next_reconnect;net_llm_sse_response_t next_response;int status;if(!state||!reconnect||!response)return -1;if(state->phase!=NE2K_LLM_CONNECTION_REQUEST_SENT&&state->phase!=NE2K_LLM_CONNECTION_STREAMING&&state->phase!=NE2K_LLM_CONNECTION_RESPONSE_READY)return -2;next_state=*state;next_reconnect=*reconnect;next_response=*response;status=net_llm_sse_reconnect_schedule(&next_reconnect,&next_response,status_code,base_delay,max_delay,now);if(status<=0)return status;next_state.phase=NE2K_LLM_CONNECTION_TLS_COMPLETE;*state=next_state;*reconnect=next_reconnect;*response=next_response;return 1;}
int ne2k_llm_connection_sse_retry_ready(const ne2k_llm_connection_state_t* state,const net_llm_sse_reconnect_t* reconnect,uint32_t now){if(!state||!reconnect)return -1;if(state->phase!=NE2K_LLM_CONNECTION_TLS_COMPLETE)return -2;return net_llm_sse_reconnect_ready(reconnect,now);}

int ne2k_llm_connection_poll_sse_or_resume(ne2k_device_t* device,const ne2k_io_t* io,const net_arp_cache_t* cache,uint8_t* rx_frame,uint16_t rx_capacity,uint8_t* tx_frame,uint16_t tx_capacity,const uint8_t local_ip[4],ne2k_llm_connection_state_t* state,net_tcp_connection_t* connection,ne2k_tls_client_t* client,uint8_t provider,uint8_t* plaintext,uint16_t plaintext_capacity,net_llm_sse_response_t* response,uint8_t* text,uint16_t text_capacity,uint16_t* text_length,uint16_t* consumed,net_llm_sse_reconnect_t* reconnect,uint32_t now,uint8_t* request,uint16_t request_capacity,const char* host,const char* path,uint8_t* tls_record,uint32_t tls_capacity,uint8_t retransmit_limit){int status;if(!state||!reconnect||!text_length||!consumed)return -1;if(state->phase==NE2K_LLM_CONNECTION_TLS_COMPLETE){status=ne2k_llm_connection_sse_retry_ready(state,reconnect,now);if(status<=0)return status;if(!response)return -1;status=ne2k_https_llm_sse_resume_request(device,io,cache,tx_frame,tx_capacity,local_ip,state->remote_ip,connection,client,response,request,request_capacity,host,path,tls_record,tls_capacity,retransmit_limit);if(status<0)return status;state->phase=NE2K_LLM_CONNECTION_REQUEST_SENT;*text_length=0U;*consumed=0U;return status;}return ne2k_llm_connection_poll_sse(device,io,cache,rx_frame,rx_capacity,tx_frame,tx_capacity,local_ip,state,connection,client,provider,plaintext,plaintext_capacity,response,text,text_capacity,text_length,consumed);}

int ne2k_llm_connection_poll_sse_or_resume_now(ne2k_device_t* device,const ne2k_io_t* io,const net_arp_cache_t* cache,uint8_t* rx_frame,uint16_t rx_capacity,uint8_t* tx_frame,uint16_t tx_capacity,const uint8_t local_ip[4],ne2k_llm_connection_state_t* state,net_tcp_connection_t* connection,ne2k_tls_client_t* client,uint8_t provider,uint8_t* plaintext,uint16_t plaintext_capacity,net_llm_sse_response_t* response,uint8_t* text,uint16_t text_capacity,uint16_t* text_length,uint16_t* consumed,net_llm_sse_reconnect_t* reconnect,uint8_t* request,uint16_t request_capacity,const char* host,const char* path,uint8_t* tls_record,uint32_t tls_capacity,uint8_t retransmit_limit){return ne2k_llm_connection_poll_sse_or_resume(device,io,cache,rx_frame,rx_capacity,tx_frame,tx_capacity,local_ip,state,connection,client,provider,plaintext,plaintext_capacity,response,text,text_capacity,text_length,consumed,reconnect,timer_get_ticks ? timer_get_ticks() : 0U,request,request_capacity,host,path,tls_record,tls_capacity,retransmit_limit);}
int ne2k_llm_connection_sse_peer_close_notify(ne2k_llm_connection_state_t* state){if(!state)return -1;state->phase=NE2K_LLM_CONNECTION_RESPONSE_READY;return 0;}

int ne2k_llm_connection_sse_event_tick(ne2k_device_t* device,const ne2k_io_t* io,const net_arp_cache_t* cache,uint8_t* rx_frame,uint16_t rx_capacity,uint8_t* tx_frame,uint16_t tx_capacity,const uint8_t local_ip[4],ne2k_llm_connection_state_t* state,net_tcp_connection_t* connection,ne2k_tls_client_t* client,uint8_t provider,uint8_t* plaintext,uint16_t plaintext_capacity,net_llm_sse_response_t* response,uint8_t* text,uint16_t text_capacity,uint16_t* text_length,uint16_t* consumed,net_llm_sse_reconnect_t* reconnect,uint32_t now,uint8_t* request,uint16_t request_capacity,const char* host,const char* path,uint8_t* tls_record,uint32_t tls_capacity,uint8_t retransmit_limit,uint32_t base_delay,uint32_t max_delay){uint8_t waiting;int status;if(!state||!response||!reconnect||!text_length||!consumed||base_delay==0U||max_delay<base_delay)return -1;waiting=(uint8_t)(state->phase==NE2K_LLM_CONNECTION_TLS_COMPLETE);status=ne2k_llm_connection_poll_sse_or_resume(device,io,cache,rx_frame,rx_capacity,tx_frame,tx_capacity,local_ip,state,connection,client,provider,plaintext,plaintext_capacity,response,text,text_capacity,text_length,consumed,reconnect,now,request,request_capacity,host,path,tls_record,tls_capacity,retransmit_limit);if(waiting)return status;if(status==NET_HTTP_TLS_STATUS_CLOSE_NOTIFY){(void)ne2k_https_llm_close_notify(device,io,cache,tx_frame,tx_capacity,local_ip,state->remote_ip,connection,client,tls_record,tls_capacity,retransmit_limit);return ne2k_llm_connection_sse_peer_close_notify(state);}if(status>0)return status;return ne2k_llm_connection_handle_sse_terminal(state,reconnect,response,status,response->http.status_code,base_delay,max_delay,now);}
int ne2k_llm_network_context_sse_event_tick(ne2k_llm_network_context_t* context,ne2k_device_t* device,const ne2k_io_t* io,const net_arp_cache_t* cache,uint8_t* rx_frame,uint16_t rx_capacity,uint8_t* tx_frame,uint16_t tx_capacity,const uint8_t local_ip[4],ne2k_tls_client_t* client,uint8_t provider,uint8_t* plaintext,uint16_t plaintext_capacity,net_llm_sse_response_t* response,uint8_t* text,uint16_t text_capacity,uint16_t* text_length,uint16_t* consumed,net_llm_sse_reconnect_t* reconnect,uint32_t now,uint8_t* request,uint16_t request_capacity,const char* host,const char* path,uint8_t* tls_record,uint32_t tls_capacity,uint8_t retransmit_limit,uint32_t base_delay,uint32_t max_delay){ne2k_llm_network_context_t next_context;net_llm_sse_response_t next_response;net_llm_sse_reconnect_t next_reconnect;uint16_t next_text_length,next_consumed;int status;if(!context||!response||!reconnect||!text_length||!consumed)return -1;next_context=*context;next_response=*response;next_reconnect=*reconnect;next_text_length=*text_length;next_consumed=*consumed;status=ne2k_llm_connection_sse_event_tick(device,io,cache,rx_frame,rx_capacity,tx_frame,tx_capacity,local_ip,&next_context.session,&next_context.connection,client,provider,plaintext,plaintext_capacity,&next_response,text,text_capacity,&next_text_length,&next_consumed,&next_reconnect,now,request,request_capacity,host,path,tls_record,tls_capacity,retransmit_limit,base_delay,max_delay);if(status<0)return status;if(ne2k_llm_network_context_sse_checkpoint(&next_context,provider,next_reconnect.retries_used,&next_response)!=0)return -2;*context=next_context;*response=next_response;*reconnect=next_reconnect;*text_length=next_text_length;*consumed=next_consumed;return status;}
int ne2k_llm_network_context_sse_rotate_provider(ne2k_llm_network_context_t* context,uint8_t* provider,net_llm_sse_reconnect_t* reconnect,const net_llm_sse_response_t* response){ne2k_llm_network_context_t next_context;net_llm_sse_reconnect_t next_reconnect;net_llm_sse_response_t next_response;uint8_t next_provider,index;int status;if(!context||!provider||!reconnect||!response)return -1;if(*provider!=NE2K_LLM_PROVIDER_OLLAMA&&*provider!=NE2K_LLM_PROVIDER_OPENAI)return -2;if(reconnect->scheduled||reconnect->retries_used<reconnect->retry_limit)return 0;next_context=*context;next_reconnect=*reconnect;next_response=*response;next_provider=*provider;status=ne2k_llm_connection_rotate_provider(&next_provider,next_reconnect.retry_limit,next_reconnect.retries_used);if(status<=0)return status;if(net_llm_sse_reconnect_init(&next_reconnect,next_reconnect.retry_limit)!=0)return -2;next_response.sse.event_id_valid=0U;next_response.sse.event_id_length=0U;for(index=0U;index<NET_LLM_SSE_EVENT_ID_MAX;index++)next_response.sse.event_id[index]=0U;if(ne2k_llm_network_context_sse_checkpoint(&next_context,next_provider,0U,&next_response)!=0)return -3;*context=next_context;*reconnect=next_reconnect;*provider=next_provider;return 1;}
int ne2k_llm_network_context_sse_schedule_jittered(ne2k_llm_network_context_t* context,uint8_t provider,net_llm_sse_reconnect_t* reconnect,net_llm_sse_response_t* response,uint16_t status_code,uint32_t base_delay,uint32_t max_delay,uint32_t now,uint32_t jitter_window,uint32_t* jitter_seed){ne2k_llm_network_context_t next_context;net_llm_sse_reconnect_t next_reconnect;net_llm_sse_response_t next_response;uint32_t next_seed;int status;if(!context||!reconnect||!response||!jitter_seed)return -1;if(context->session.phase!=NE2K_LLM_CONNECTION_REQUEST_SENT&&context->session.phase!=NE2K_LLM_CONNECTION_STREAMING&&context->session.phase!=NE2K_LLM_CONNECTION_RESPONSE_READY)return -2;next_context=*context;next_reconnect=*reconnect;next_response=*response;next_seed=*jitter_seed;status=net_llm_sse_reconnect_schedule_jittered(&next_reconnect,&next_response,status_code,base_delay,max_delay,now,jitter_window,&next_seed);if(status<=0)return status;next_context.session.phase=NE2K_LLM_CONNECTION_TLS_COMPLETE;if(ne2k_llm_network_context_sse_checkpoint(&next_context,provider,next_reconnect.retries_used,&next_response)!=0)return -3;*context=next_context;*reconnect=next_reconnect;*response=next_response;*jitter_seed=next_seed;return 1;}
int ne2k_llm_network_context_sse_resume_decide(const ne2k_llm_network_context_t* context,const net_llm_sse_response_t* response){if(!context||!response)return -1;if(context->session.phase!=NE2K_LLM_CONNECTION_TLS_COMPLETE)return -2;return response->sse.event_id_valid&&response->sse.event_id_length>0U?1:0;}
int ne2k_llm_connection_classify_sse_result(int poll_status,uint16_t status_code){if(poll_status>0)return NE2K_LLM_SSE_RESULT_PROGRESS;if(poll_status==0)return NE2K_LLM_SSE_RESULT_COMPLETED;if(status_code==408U||status_code==425U||status_code==429U||status_code==500U||status_code==502U||status_code==503U||status_code==504U)return NE2K_LLM_SSE_RESULT_RETRYABLE;if(poll_status==-1||poll_status==-4||poll_status==-5)return NE2K_LLM_SSE_RESULT_TRANSPORT;return NE2K_LLM_SSE_RESULT_TERMINAL;}
int ne2k_llm_connection_handle_sse_terminal(ne2k_llm_connection_state_t* state,net_llm_sse_reconnect_t* reconnect,net_llm_sse_response_t* response,int poll_status,uint16_t status_code,uint32_t base_delay,uint32_t max_delay,uint32_t now){int result;if(!state||!reconnect||!response)return -1;result=ne2k_llm_connection_classify_sse_result(poll_status,status_code);if(result==NE2K_LLM_SSE_RESULT_RETRYABLE||result==NE2K_LLM_SSE_RESULT_TRANSPORT)return ne2k_llm_connection_schedule_sse_retry(state,reconnect,response,result==NE2K_LLM_SSE_RESULT_TRANSPORT?503U:status_code,base_delay,max_delay,now);if(result==NE2K_LLM_SSE_RESULT_COMPLETED){state->phase=NE2K_LLM_CONNECTION_RESPONSE_READY;return 0;}return -2;}

uint8_t ne2k_llm_provider_next(uint8_t provider){if(provider==NE2K_LLM_PROVIDER_OLLAMA)return NE2K_LLM_PROVIDER_OPENAI;if(provider==NE2K_LLM_PROVIDER_OPENAI)return NE2K_LLM_PROVIDER_OLLAMA;return 0xffU;}
int ne2k_llm_connection_rotate_provider(uint8_t* provider,uint8_t retry_limit,uint8_t retries_used){uint8_t next;if(!provider)return -1;if(*provider!=NE2K_LLM_PROVIDER_OLLAMA&&*provider!=NE2K_LLM_PROVIDER_OPENAI)return -2;if(retries_used<retry_limit)return 0;next=ne2k_llm_provider_next(*provider);if(next==0xffU)return -3;*provider=next;return 1;}
