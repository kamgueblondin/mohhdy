/* wf_dns.h - Lot 67: DNS A-record query/answer codec (RFC 1035) used over
 * TCP (2-byte length prefix) by the Ring 3 resolver. */
#ifndef WF_DNS_H
#define WF_DNS_H
#include <stdint.h>
/* Writes a recursive A query for host (without the TCP length prefix).
 * Returns the length or -1. */
int wf_dns_build_query(const char* host, uint16_t id, uint8_t* out, int cap);
/* Parses an answer: checks id, QR, RCODE, the echoed question, follows a
 * CNAME chain inside the message and returns the first matching A
 * record. 0 = found, -1 malformed, -2 NXDOMAIN/error rcode, -3 no A. */
int wf_dns_parse_a(const uint8_t* msg, int len, uint16_t id, const char* host, uint8_t ip[4]);
/* 0 and the 4 bytes when s is a dotted IPv4 literal. */
int wf_ipv4_parse(const char* s, uint8_t ip[4]);
#endif
