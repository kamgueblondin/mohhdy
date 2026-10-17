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

/* Tranche 5 suite: NE2000 ISA window (command/data/reset ports). */
#define IO_BITMAP_NE2K_BASE 0x300U
#define IO_BITMAP_NE2K_LAST 0x31FU
void io_bitmap_apply_ne2k(uint8_t* map, uint32_t len, int allow);

/* Lot 4 (US-001): PCI configuration mechanism #1 (address 0xCF8, data
 * 0xCFC-0xCFF), opened only for the live Ring 3 pcidriver. */
#define IO_BITMAP_PCI_BASE 0xCF8U
#define IO_BITMAP_PCI_LAST 0xCFFU
void io_bitmap_apply_pci(uint8_t* map, uint32_t len, int allow);

/* Lot 5 (US-001): PS/2 controller (data 0x60, status/command 0x64) for the
 * live Ring 3 ps2driver, COM1 (0x3F8-0x3FF) for the live serialdriver. */
#define IO_BITMAP_PS2_DATA 0x60U
#define IO_BITMAP_PS2_CMD 0x64U
#define IO_BITMAP_COM1_BASE 0x3F8U
#define IO_BITMAP_COM1_LAST 0x3FFU
void io_bitmap_apply_ps2(uint8_t* map, uint32_t len, int allow);
void io_bitmap_apply_serial(uint8_t* map, uint32_t len, int allow);

/* Lot 7 (US-001): VGA CRTC index/data (cursor) for the live vgadriver. */
#define IO_BITMAP_VGA_CRTC_INDEX 0x3D4U
#define IO_BITMAP_VGA_CRTC_DATA 0x3D5U
void io_bitmap_apply_vga(uint8_t* map, uint32_t len, int allow);

#endif
