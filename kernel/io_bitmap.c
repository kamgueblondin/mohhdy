#include "io_bitmap.h"

void io_bitmap_deny_all(uint8_t* map, uint32_t len) {
    uint32_t i;
    if (!map) return;
    for (i = 0U; i < len; i++) map[i] = 0xFFU;
}

void io_bitmap_set_port(uint8_t* map, uint32_t len, uint16_t port, int allow) {
    uint32_t byte = (uint32_t)port >> 3;
    uint8_t mask = (uint8_t)(1U << (port & 7U));
    if (!map || byte >= len) return;
    if (allow) map[byte] &= (uint8_t)~mask;  /* 0 = allowed */
    else map[byte] |= mask;                  /* 1 = denied */
}

int io_bitmap_port_allowed(const uint8_t* map, uint32_t len, uint16_t port) {
    uint32_t byte = (uint32_t)port >> 3;
    uint8_t mask = (uint8_t)(1U << (port & 7U));
    if (!map || byte >= len) return 0;
    return (map[byte] & mask) == 0U;
}

void io_bitmap_apply_ata(uint8_t* map, uint32_t len, int allow) {
    uint16_t port;
    for (port = IO_BITMAP_ATA_CMD_BASE; port <= IO_BITMAP_ATA_CMD_LAST; port++)
        io_bitmap_set_port(map, len, port, allow);
    io_bitmap_set_port(map, len, IO_BITMAP_ATA_CTRL, allow);
}
