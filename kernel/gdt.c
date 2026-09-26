#include "gdt.h"
#include "kernel/mem/string.h" // For memset
#include "io_bitmap.h"

// GDT pointer and entries
typedef struct {
    uint16_t limit;
    uint32_t base;
} __attribute__((packed)) gdt_ptr_t;

gdt_entry_t gdt_entries[6];
gdt_ptr_t   gdt_ptr;
tss_full_t tss_full;
#define tss_entry (tss_full.tss)
static int g_tss_ata_granted = -1;

// External assembly functions
extern void gdt_flush(uint32_t);
extern void tss_flush();

// Set a GDT entry
void gdt_set_gate(int num, uint32_t base, uint32_t limit, uint8_t access, uint8_t gran) {
    gdt_entries[num].base_low    = (base & 0xFFFF);
    gdt_entries[num].base_middle = (base >> 16) & 0xFF;
    gdt_entries[num].base_high   = (base >> 24) & 0xFF;

    gdt_entries[num].limit_low   = (limit & 0xFFFF);
    gdt_entries[num].granularity = (limit >> 16) & 0x0F;

    gdt_entries[num].granularity |= gran & 0xF0;
    gdt_entries[num].access      = access;
}

// Initialize GDT and TSS
void gdt_init() {
    gdt_ptr.limit = (sizeof(gdt_entry_t) * 6) - 1;
    gdt_ptr.base  = (uint32_t)&gdt_entries;

    gdt_set_gate(0, 0, 0, 0, 0);                // Null segment
    gdt_set_gate(1, 0, 0xFFFFFFFF, 0x9A, 0xCF); // Kernel Code Segment
    gdt_set_gate(2, 0, 0xFFFFFFFF, 0x92, 0xCF); // Kernel Data Segment
    gdt_set_gate(3, 0, 0xFFFFFFFF, 0xFA, 0xCF); // User Code Segment
    gdt_set_gate(4, 0, 0xFFFFFFFF, 0xF2, 0xCF); // User Data Segment

    // Create TSS entry
    uint32_t tss_base = (uint32_t)&tss_full;
    uint32_t tss_limit = sizeof(tss_full) - 1U;
    gdt_set_gate(5, tss_base, tss_limit, 0x89, 0x00); // 0x89 = Present, DPL=0, TSS

    // Initialize TSS + I/O bitmap (deny every port by default).
    memset(&tss_full, 0, sizeof(tss_full));
    tss_entry.ss0  = 0x10;  // Kernel data segment selector
    tss_entry.esp0 = 0x0;   // Will be set by the scheduler
    tss_entry.cs   = 0x0b;
    tss_entry.ss = tss_entry.ds = tss_entry.es = tss_entry.fs = tss_entry.gs = 0x13;
    /* iomap_base is measured from the TSS base; place the bitmap right after
     * the 104-byte hardware TSS. */
    tss_entry.iomap_base = (uint16_t)((uint32_t)&tss_full.io_bitmap - (uint32_t)&tss_full);
    io_bitmap_deny_all(tss_full.io_bitmap, TSS_IO_BITMAP_BYTES);
    tss_full.io_bitmap_end = 0xFFU;
    g_tss_ata_granted = 0;

    // Flush GDT and TSS
    gdt_flush((uint32_t)&gdt_ptr);
    tss_flush();
}

// Called by scheduler to update kernel stack pointer
void tss_set_stack(uint32_t ss, uint32_t esp) {
    tss_entry.ss0 = ss;
    tss_entry.esp0 = esp;
}

/* Tranche 4: rewrite the ATA range in the live TSS IOPB only on transitions.
 * The scheduler calls this with the incoming task's grant flag, so exactly one
 * task (the ata-driver holder) ever sees the ATA ports opened at Ring 3. */
void tss_set_ata_io(int grant) {
    grant = grant ? 1 : 0;
    if (g_tss_ata_granted == grant) return;
    io_bitmap_apply_ata(tss_full.io_bitmap, TSS_IO_BITMAP_BYTES, grant);
    g_tss_ata_granted = grant;
}
