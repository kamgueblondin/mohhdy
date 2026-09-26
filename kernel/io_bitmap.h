#ifndef IO_BITMAP_H
#define IO_BITMAP_H

#include <stdint.h>

/* Tranche 4: TSS I/O permission bitmap helpers (pure logic, no privileged
 * instruction) so the port capability can be unit tested on the host.
 * Convention: bit set (1) = access denied, bit clear (0) = access allowed,
 * matching the x86 TSS IOPB semantics. */

#define IO_BITMAP_PORTS 65536U
#define IO_BITMAP_BYTES (IO_BITMAP_PORTS / 8U)

/* ATA PIO primary command block + alternate status. This is the only port
 * range a granted Ring 3 storage driver may touch. */
#define IO_BITMAP_ATA_CMD_BASE 0x1F0U
#define IO_BITMAP_ATA_CMD_LAST 0x1F7U
#define IO_BITMAP_ATA_CTRL     0x3F6U

void io_bitmap_deny_all(uint8_t* map, uint32_t len);
void io_bitmap_set_port(uint8_t* map, uint32_t len, uint16_t port, int allow);
int io_bitmap_port_allowed(const uint8_t* map, uint32_t len, uint16_t port);
/* Allow (allow=1) or deny (allow=0) exactly the ATA port range above. */
void io_bitmap_apply_ata(uint8_t* map, uint32_t len, int allow);

#endif
