bits 32

section .text

global gdt_flush
gdt_flush:
    mov eax, [esp+4]    ; Get pointer to the GDT descriptor
    lgdt [eax]          ; Load the new GDT

    mov ax, 0x10        ; 0x10 is the offset of the kernel data segment
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    jmp 0x08:.flush     ; 0x08 is the offset of the kernel code segment
.flush:
    ret

global tss_flush
tss_flush:
    mov ax, 0x28        ; 0x28 is the offset of our TSS segment (5 * 8)
    ltr ax
    ret
