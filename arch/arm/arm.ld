/* Lot 8 (US-076): QEMU virt RAM at 0x40000000. Kernel in the first 1 MB
 * section (PL1 only), user tasks in the second section (user access). */
ENTRY(_start)
SECTIONS
{
    . = 0x40000000;
    .text : { *(.text.boot) *(EXCLUDE_FILE(*user_tasks.o) .text*) }
    .rodata : { *(EXCLUDE_FILE(*user_tasks.o) .rodata*) }
    .data : { *(EXCLUDE_FILE(*user_tasks.o) .data*) }
    .bss (NOLOAD) : {
        . = ALIGN(16384);
        __bss_start = .;
        *(EXCLUDE_FILE(*user_tasks.o) .bss*) *(EXCLUDE_FILE(*user_tasks.o) COMMON)
        . = ALIGN(8);
        . += 0x1000; __irq_stack_top = .;
        . += 0x1000; __abt_stack_top = .;
        . += 0x1000; __und_stack_top = .;
        . += 0x2000; __svc_stack_top = .;
        __bss_end = .;
    }
    ASSERT(. <= 0x40100000, "ARM kernel image exceeds its 1 MB section")
    . = 0x40100000;
    __user_start = .;
    .user : { *user_tasks.o(.text* .rodata* .data* .bss* COMMON) }
    __user_end = .;
    ASSERT(. <= 0x40200000, "ARM user image exceeds its 1 MB section")
    /DISCARD/ : { *(.ARM.exidx*) *(.ARM.attributes) *(.comment) }
}
