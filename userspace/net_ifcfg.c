/* net_ifcfg.c - Lot 47 (US-G04), see net_ifcfg.h. */
#include "net_ifcfg.h"

static int ifc_eq(const char* a, const char* b) {
    while (*a && *a == *b) { a++; b++; }
    return *a == *b;
}
static void ifc_cat(char* out, int max, int* p, const char* s) {
    while (*s && *p < max - 1) out[(*p)++] = *s++;
    out[*p] = 0;
}
static void ifc_u8(char* out, int max, int* p, uint32_t v) {
    char b[4]; int n = 0;
    do { b[n++] = (char)('0' + v % 10U); v /= 10U; } while (v && n < 3);
    while (n && *p < max - 1) out[(*p)++] = b[--n];
    out[*p] = 0;
}
static void ifc_ip(char* out, int max, int* p, const uint8_t* ip) {
    int i;
    for (i = 0; i < 4; i++) { if (i) ifc_cat(out, max, p, "."); ifc_u8(out, max, p, ip[i]); }
}

int net_ifcfg_parse_ip(const char* s, uint8_t out[4]) {
    int part = 0, digits = 0;
    uint32_t v = 0U;
    if (!s || !out) return -1;
    for (;; s++) {
        if (*s >= '0' && *s <= '9') {
            v = v * 10U + (uint32_t)(*s - '0');
            if (++digits > 3 || v > 255U) return -1;
        } else if (*s == '.' || *s == 0) {
            if (!digits || part > 3) return -1;
            out[part++] = (uint8_t)v;
            v = 0U; digits = 0;
            if (*s == 0) break;
        } else return -1;
    }
    return part == 4 ? 0 : -1;
}

static const char* ifc_err(int rc) {
    if (rc == OS_PEER_DENIED) return "denied (root shell only)";
    if (rc == OS_PEER_BAD_ADDRESS) return "bad address (ip, mask or gateway)";
    if (rc == OS_PEER_UNAVAILABLE || rc == OS_NET_WIRE_UNAVAILABLE) return "network unavailable";
    if (rc == OS_PEER_FAILED) return "dhcp failed";
    return "failed";
}

int net_ifcfg_run(int argc, const char* const* argv, net_ifcfg_call_t call,
                  os_peer_data_request_t* r, char* out, int max) {
    int p = 0, i, rc;
    out[0] = 0;
    if (!call || !r) return 1;
    for (i = 0; i < (int)sizeof(r->data) && i < 16; i++) r->data[i] = 0U;
    r->op = (uint8_t)OS_PEER_IFCONFIG; r->slot = 0; r->attempts = 0; r->length = 0; r->port = 0; r->flags = 0;
    if (argc <= 0 || !argv[0][0]) r->data[0] = (uint8_t)OS_IFCONFIG_GET;
    else if (ifc_eq(argv[0], "dhcp") && argc == 1) r->data[0] = (uint8_t)OS_IFCONFIG_DHCP;
    else if (ifc_eq(argv[0], "static") && argc == 4) {
        if (net_ifcfg_parse_ip(argv[1], r->data + 1) || net_ifcfg_parse_ip(argv[2], r->data + 5) ||
            net_ifcfg_parse_ip(argv[3], r->data + 9)) {
            ifc_cat(out, max, &p, "net-if error bad address syntax (a.b.c.d)\n");
            return 1;
        }
        r->data[0] = (uint8_t)OS_IFCONFIG_STATIC;
    } else {
        ifc_cat(out, max, &p, "net-if error usage: net-if | net-if static IP MASK GW | net-if dhcp\n");
        return 1;
    }
    rc = call(r);
    if (rc != 0) {
        ifc_cat(out, max, &p, "net-if error ");
        ifc_cat(out, max, &p, ifc_err(rc));
        ifc_cat(out, max, &p, "\n");
        return 1;
    }
    ifc_cat(out, max, &p, "net-if ok mode ");
    ifc_cat(out, max, &p, r->data[0] == OS_IFCONFIG_MODE_STATIC ? "static" :
                          r->data[0] == OS_IFCONFIG_MODE_DHCP ? "dhcp" : "none");
    ifc_cat(out, max, &p, " ip "); ifc_ip(out, max, &p, r->data + 1);
    ifc_cat(out, max, &p, " mask "); ifc_ip(out, max, &p, r->data + 5);
    ifc_cat(out, max, &p, " gw "); ifc_ip(out, max, &p, r->data + 9);
    ifc_cat(out, max, &p, r->data[13] ? " lease 1" : " lease 0");
    /* Lot 48: TCP connections on the old address, ended with FIN by this change */
    if (r->length >= 15U && r->data[14]) { ifc_cat(out, max, &p, " closed "); ifc_u8(out, max, &p, r->data[14]); }
    if (r->length >= 16U && r->data[15]) ifc_cat(out, max, &p, " llm-closed 1");
    ifc_cat(out, max, &p, "\n");
    return 0;
}
