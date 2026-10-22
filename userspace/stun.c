/* userspace/stun.c - STUN (RFC 5389) Binding codec (Lot 23, US-070/US-076). */
#include "stun.h"

static void put16(uint8_t* b, uint16_t v) { b[0] = (uint8_t)(v >> 8); b[1] = (uint8_t)v; }
static uint16_t get16(const uint8_t* b) { return (uint16_t)(((uint16_t)b[0] << 8) | b[1]); }
static uint32_t get32(const uint8_t* b) {
    return ((uint32_t)b[0] << 24) | ((uint32_t)b[1] << 16) | ((uint32_t)b[2] << 8) | b[3];
}

static int header(uint8_t* out, int cap, uint16_t type, uint16_t length, const uint8_t txid[12]) {
    int i;
    if (!out || !txid || cap < STUN_HEADER + length) return -1;
    put16(out, type); put16(out + 2, length);
    out[4] = 0x21; out[5] = 0x12; out[6] = 0xA4; out[7] = 0x42;
    for (i = 0; i < 12; i++) out[8 + i] = txid[i];
    return 0;
}

int stun_build_binding_request(uint8_t* out, int cap, const uint8_t txid[12]) {
    return header(out, cap, STUN_BINDING_REQUEST, 0U, txid) == 0 ? STUN_HEADER : -1;
}

int stun_build_binding_success(uint8_t* out, int cap, const uint8_t txid[12],
                               const uint8_t ip[4], uint16_t port) {
    uint8_t* a;
    if (!ip || header(out, cap, STUN_BINDING_SUCCESS, 12U, txid) != 0) return -1;
    a = out + STUN_HEADER;
    put16(a, STUN_ATTR_XOR_MAPPED_ADDRESS); put16(a + 2, 8U);
    a[4] = 0; a[5] = 0x01; /* IPv4 */
    put16(a + 6, (uint16_t)(port ^ (STUN_MAGIC >> 16)));
    a[8] = (uint8_t)(ip[0] ^ 0x21); a[9] = (uint8_t)(ip[1] ^ 0x12);
    a[10] = (uint8_t)(ip[2] ^ 0xA4); a[11] = (uint8_t)(ip[3] ^ 0x42);
    return STUN_BINDING_SUCCESS_LENGTH;
}

int stun_parse_header(const uint8_t* m, int len, uint16_t* type, uint8_t txid[12]) {
    uint16_t length;
    int i;
    if (!m || len < STUN_HEADER) return -1;
    if ((m[0] & 0xC0U) != 0U) return -2;            /* top two bits are zero */
    if (get32(m + 4) != STUN_MAGIC) return -3;
    length = get16(m + 2);
    if ((length & 3U) != 0U || STUN_HEADER + (int)length != len) return -4;
    if (type) *type = get16(m);
    if (txid) for (i = 0; i < 12; i++) txid[i] = m[8 + i];
    return 0;
}

int stun_parse_mapped(const uint8_t* m, int len, const uint8_t txid[12], uint8_t ip[4], uint16_t* port) {
    uint16_t type;
    uint8_t got[12];
    int pos, i, found = 0;
    if (!txid || !ip || !port || stun_parse_header(m, len, &type, got) != 0) return -1;
    if (type != STUN_BINDING_SUCCESS) return -2;
    for (i = 0; i < 12; i++) if (got[i] != txid[i]) return -3;
    pos = STUN_HEADER;
    while (pos + 4 <= len) {
        uint16_t at = get16(m + pos), al = get16(m + pos + 2);
        const uint8_t* v = m + pos + 4;
        if (pos + 4 + (int)al > len) return -4;
        if ((at == STUN_ATTR_XOR_MAPPED_ADDRESS || (at == STUN_ATTR_MAPPED_ADDRESS && !found)) &&
            al >= 8U && v[1] == 0x01) {
            if (at == STUN_ATTR_XOR_MAPPED_ADDRESS) {
                *port = (uint16_t)(get16(v + 2) ^ (STUN_MAGIC >> 16));
                ip[0] = (uint8_t)(v[4] ^ 0x21); ip[1] = (uint8_t)(v[5] ^ 0x12);
                ip[2] = (uint8_t)(v[6] ^ 0xA4); ip[3] = (uint8_t)(v[7] ^ 0x42);
                found = 2;
            } else {
                *port = get16(v + 2);
                for (i = 0; i < 4; i++) ip[i] = v[4 + i];
                found = 1;
            }
        }
        pos += 4 + (int)((al + 3U) & ~3U);
    }
    return found ? 0 : -5;
}

static int same(const uint8_t a[4], uint16_t pa, const uint8_t b[4], uint16_t pb) {
    return a[0] == b[0] && a[1] == b[1] && a[2] == b[2] && a[3] == b[3] && pa == pb;
}

int stun_nat_classify(const uint8_t local_ip[4], uint16_t local_port,
                      const uint8_t ip1[4], uint16_t port1,
                      int second_valid, const uint8_t ip2[4], uint16_t port2) {
    if (!local_ip || !ip1) return -1;
    if (same(local_ip, local_port, ip1, port1) &&
        (!second_valid || (ip2 && same(local_ip, local_port, ip2, port2)))) return STUN_NAT_DIRECT;
    if (!second_valid || !ip2) return STUN_NAT_UNKNOWN;
    return same(ip1, port1, ip2, port2) ? STUN_NAT_INDEPENDENT : STUN_NAT_DEPENDENT;
}

const char* stun_nat_name(int kind) {
    switch (kind) {
    case STUN_NAT_DIRECT: return "direct";
    case STUN_NAT_INDEPENDENT: return "endpoint_independent";
    case STUN_NAT_DEPENDENT: return "endpoint_dependent";
    case STUN_NAT_UNKNOWN: return "nat_unknown";
    default: return "invalid";
    }
}
