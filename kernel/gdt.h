#ifndef GDT_H
#define GDT_H

#include <stdint.h>

// GDT entry structure
typedef struct {
    uint16_t limit_low;
    uint16_t base_low;
    uint8_t  base_middle;
    uint8_t  access;
    uint8_t  granularity;
    uint8_t  base_high;
} __attribute__((packed)) gdt_entry_t;

// TSS entry structure
typedef struct {
    uint32_t prev_tss;
    uint32_t esp0;       // Kernel stack pointer
    uint32_t ss0;        // Kernel stack segment
    uint32_t esp1;
    uint32_t ss1;
    uint32_t esp2;
    uint32_t ss2;
    uint32_t cr3;
    uint32_t eip;
    uint32_t eflags;
    uint32_t eax;
    uint32_t ecx;
    uint32_t edx;
    uint32_t ebx;
    uint32_t esp;
    uint32_t ebp;
    uint32_t esi;
    uint32_t edi;
    uint32_t es;
    uint32_t cs;
    uint32_t ss;
    uint32_t ds;
    uint32_t fs;
    uint32_t gs;
    uint32_t ldt;
    uint16_t trap;
    uint16_t iomap_base;
} __attribute__((packed)) tss_entry_t;

/* Tranche 4: full TSS with an I/O permission bitmap so a granted Ring 3 task
 * can execute ATA PIO (ports 0x1F0-0x1F7, 0x3F6) directly. iomap_base points
 * at io_bitmap; a trailing 0xFF byte terminates the map per the x86 rules. */
#define TSS_IO_BITMAP_BYTES 8192U

typedef struct {
    tss_entry_t tss;
    uint8_t io_bitmap[TSS_IO_BITMAP_BYTES];
    uint8_t io_bitmap_end; /* must stay 0xFF */
} __attribute__((packed)) tss_full_t;

void gdt_init();
void gdt_set_gate(int num, uint32_t base, uint32_t limit, uint8_t access, uint8_t gran);
void tss_set_stack(uint32_t ss, uint32_t esp);
/* Load (grant=1) or clear (grant=0) the ATA port range in the live TSS IOPB.
 * Idempotent; only rewrites the map when the state changes. */
void tss_set_ata_io(int grant);

#endif
