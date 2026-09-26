#ifndef MOHHDY_NET_WIRE_H
#define MOHHDY_NET_WIRE_H

/* Tranche 5 slice 3: worker-only NE2000 wire path for the socket registry.
 *
 * The kernel only enters these functions from SYS_NET_WIRE_* issued by the
 * live net-driver worker (see kernel/syscall/syscall.c). They turn a socket
 * of the Ring 0 registry (kernel/net_socket.c) into real Ethernet frames:
 * ARP resolution of the on-link peer, IPv4/TCP framing through
 * ne2k_tcp_segment(), and a receive demux that feeds only frames addressed
 * to a wire-bound socket (local ip/port, remote ip/port) into that socket.
 * The NE2000 driver, its IRQ handler and the TCP state machine stay in
 * Ring 0; only the decision to send/receive is taken by the Ring 3 worker. */

#include <stdint.h>
#include "ne2k.h"
#include "net_socket.h"
#include "net_ethernet_arp.h"
#include "../include/os_syscalls.h"

#define NET_WIRE_BINDINGS NET_SOCKET_CAPACITY
#define NET_WIRE_DEFAULT_ATTEMPTS 200U
#define NET_WIRE_MAX_ATTEMPTS 2000U

/* net_wire_demux() results (>= 0 is the bound socket id). */
#define NET_WIRE_DEMUX_DROP (-1)
#define NET_WIRE_DEMUX_ARP_REQUEST (-2) /* ARP request for a bound local IP */
#define NET_WIRE_DEMUX_ARP_REPLY (-3)   /* ARP reply addressed to a bound local IP */

typedef struct {
    uint8_t bound;
    uint8_t local_ip[4];
    uint8_t remote_ip[4];
    uint16_t local_port;
    uint16_t remote_port;
} net_wire_binding_t;

typedef struct {
    ne2k_device_t* device;
    const ne2k_io_t* io;
    net_arp_cache_t* cache;
    uint8_t* tx;       /* frame buffers, capacity bytes each */
    uint8_t* rx;
    uint16_t capacity;
} net_wire_ctx_t;

/* Pure bookkeeping (unit tested). */
void net_wire_reset(void);
int net_wire_bind(int socket_id, const uint8_t local_ip[4], const uint8_t remote_ip[4],
                  uint16_t local_port, uint16_t remote_port);
int net_wire_unbind(int socket_id);
int net_wire_is_bound(int socket_id);
uint32_t net_wire_bound_count(void);
/* Classifies one received Ethernet frame against the bindings. For a TCP
 * frame to a bound socket, returns the socket id and the TCP segment
 * offset/length inside the frame. */
int net_wire_demux(const uint8_t* frame, uint16_t length, uint16_t* tcp_offset,
                   uint16_t* tcp_length);
void net_wire_note_refused(void);
void net_wire_fill_status(os_net_wire_status_t* out, int32_t worker_pid);

/* NE2000-driving operations (Ring 0; reached only through the worker). */
int net_wire_connect(const net_wire_ctx_t* ctx, const os_net_wire_connect_t* request);
int net_wire_send(const net_wire_ctx_t* ctx, int socket_id, const uint8_t* data,
                  uint16_t length, uint8_t* segment_out, uint16_t segment_capacity,
                  uint16_t* segment_length, uint16_t attempts);
int net_wire_recv(const net_wire_ctx_t* ctx, int socket_id, uint8_t* buffer,
                  uint16_t capacity, uint16_t* out_length, uint16_t attempts);
int net_wire_close(const net_wire_ctx_t* ctx, int socket_id, uint16_t attempts);

#endif
