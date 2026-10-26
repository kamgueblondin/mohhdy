/* net_ifcfg.h - Lot 47 (US-G04): the `net-if` shell command, shared by the
 * console and the /net window (one code path). Freestanding, host testable:
 * the caller passes the SYS_PEER_DATA call. */
#ifndef NET_IFCFG_H
#define NET_IFCFG_H
#include "os_syscalls.h"

typedef int (*net_ifcfg_call_t)(os_peer_data_request_t* r);

/* Parses "a.b.c.d"; 0 or -1. */
int net_ifcfg_parse_ip(const char* s, uint8_t out[4]);
/* argv: "" (show) | "static IP MASK GW" | "dhcp". Writes one result line
 * ("net-if ok mode static ip ... mask ... gw ..." or "net-if error ...")
 * into out; returns 0 on success, 1 on error. */
int net_ifcfg_run(int argc, const char* const* argv, net_ifcfg_call_t call,
                  os_peer_data_request_t* scratch, char* out, int max);
#endif
