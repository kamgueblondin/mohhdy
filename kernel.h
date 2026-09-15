#ifndef KERNEL_H
#define KERNEL_H

#include <stdint.h>

// Include headers for major modules
#include "kernel/mem/vmm.h"
#include "kernel/task/task.h"
#include "fs/initrd.h"
#include "kernel/elf.h"

// --- Function Prototypes ---

// Initialization functions
void init_gdt();
void init_idt();
void init_pit(int frequency);
void init_pmm(); // Assuming this is in a pmm.h, but let's declare for safety
void init_scheduler_timer();

// Utility functions
void print_string(const char* str);
void print_string_serial(const char* str); // Used in many places
void print_hex_serial(uint32_t n);
unsigned char inb(unsigned short port);
void outb(unsigned short port, unsigned char data);

// Assembly helper functions/macros
static inline void sti() {
    asm volatile ("sti");
}

static inline void cli() {
    asm volatile ("cli");
}

static inline void halt() {
    asm volatile ("hlt");
}

#endif // KERNEL_H
