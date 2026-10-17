/* userspace/stun.h - STUN (RFC 5389) Binding codec and NAT mapping
 * classification for the P2P node (US-074). Pure C, no allocation.
 * MESSAGE-INTEGRITY and FINGERPRINT are skipped when present (not checked). */
#ifndef MOHHDY_STUN_H
#define MOHHDY_STUN_H
#include <stdint.h>

#define STUN_HEADER 20
#define STUN_MAGIC 0x2112A442U
#define STUN_BINDING_REQUEST 0x0001U
#define STUN_BINDING_SUCCESS 0x0101U
#define STUN_ATTR_MAPPED_ADDRESS 0x0001U
#define STUN_ATTR_XOR_MAPPED_ADDRESS 0x0020U
#define STUN_BINDING_SUCCESS_LENGTH (STUN_HEADER + 12)

/* Mapping behaviour seen from one or two STUN servers (RFC 4787 terms). */
#define STUN_NAT_DIRECT 0      /* mapped address = local address: no NAT */
#define STUN_NAT_INDEPENDENT 1 /* NAT, same mapping towards both servers */
#define STUN_NAT_DEPENDENT 3   /* NAT, mapping changes per destination (symmetric) */
#define STUN_NAT_UNKNOWN 4     /* one server only and behind a NAT */

int stun_build_binding_request(uint8_t* out, int cap, const uint8_t txid[12]);
int stun_build_binding_success(uint8_t* out, int cap, const uint8_t txid[12],
                               const uint8_t ip[4], uint16_t port);
/* Returns 0 and the message type / transaction id, or < 0 if not STUN. */
int stun_parse_header(const uint8_t* m, int len, uint16_t* type, uint8_t txid[12]);
/* Binding success matching `txid`: mapped address (XOR-MAPPED preferred). */
int stun_parse_mapped(const uint8_t* m, int len, const uint8_t txid[12], uint8_t ip[4], uint16_t* port);
/* `second_valid` = 0 when only one server answered. */
int stun_nat_classify(const uint8_t local_ip[4], uint16_t local_port,
                      const uint8_t ip1[4], uint16_t port1,
                      int second_valid, const uint8_t ip2[4], uint16_t port2);
const char* stun_nat_name(int kind);
#endif
