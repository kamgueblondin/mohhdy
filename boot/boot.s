bits 32

section .note.GNU-stack noalloc noexec nowrite progbits

section .multiboot
align 4
    MULTIBOOT_HEADER_MAGIC: equ 0x1BADB002
    MULTIBOOT_HEADER_FLAGS: equ 0x0001 ; Aligner les modules
    CHECKSUM: equ -(MULTIBOOT_HEADER_MAGIC + MULTIBOOT_HEADER_FLAGS)
    dd MULTIBOOT_HEADER_MAGIC
    dd MULTIBOOT_HEADER_FLAGS
    dd CHECKSUM

section .text

global _start
_start:
    ; 1. Mettre en place la pile
    mov esp, stack_top

    ; 2. Passer les infos Multiboot au noyau
    push ebx
    push eax

    ; 3. Appeler kmain
    extern kmain
    call kmain

    cli
    hlt

section .bss
align 16
stack_bottom:
    resb 16384
stack_top:
