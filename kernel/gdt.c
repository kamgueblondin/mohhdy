#include "gdt.h"
#include "kernel/mem/string.h" // For memset

// GDT pointer and entries
typedef struct {
    uint16_t limit;
    uint32_t base;
} __attribute__((packed)) gdt_ptr_t;

gdt_entry_t gdt_entries[6];
gdt_ptr_t   gdt_ptr;
tss_entry_t tss_entry;

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
    uint32_t tss_base = (uint32_t)&tss_entry;
    uint32_t tss_limit = sizeof(tss_entry);
    gdt_set_gate(5, tss_base, tss_limit, 0x89, 0x00); // 0x89 = Present, DPL=0, TSS

    // Initialize TSS
    memset(&tss_entry, 0, sizeof(tss_entry));
    tss_entry.ss0  = 0x10;  // Kernel data segment selector
    tss_entry.esp0 = 0x0;   // Will be set by the scheduler
    tss_entry.cs   = 0x0b;
    tss_entry.ss = tss_entry.ds = tss_entry.es = tss_entry.fs = tss_entry.gs = 0x13;

    // Flush GDT and TSS
    gdt_flush((uint32_t)&gdt_ptr);
    tss_flush();
}

// Called by scheduler to update kernel stack pointer
void tss_set_stack(uint32_t ss, uint32_t esp) {
    tss_entry.ss0 = ss;
    tss_entry.esp0 = esp;
}
