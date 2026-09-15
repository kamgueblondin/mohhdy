extern keyboard_interrupt_handler
extern timer_handler
extern ne2k_irq_handler
extern syscall_handler

global irq0
global irq1
global irq3
global isr_syscall

; ISR pour le timer (IRQ 0) - Version robuste
irq0:
    ; Sauvegarde complète de l'état du processeur
    push ds
    push es
    push fs
    push gs
    pushad
    
    ; Charge les segments du noyau
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    ; EOI avant le handler C : schedule() fait iret et ne revient jamais.
    ; Sans ceci, IRQ0 reste in-service et le PIC bloque IRQ1 (clavier).
    mov al, 0x20
    out 0x20, al
    
    ; Passe un pointeur vers la structure de registres au handler C
    push esp
    call timer_handler
    add esp, 4
    
    ; Restaure l'état complet du processeur
    popad
    pop gs
    pop fs
    pop es
    pop ds
    
    ; Retour d'interruption
    iret

; ISR pour le clavier (IRQ 1) - Version robuste
irq1:
    ; Sauvegarde complète de l'état du processeur
    push ds               ; Sauvegarde des segments
    push es
    push fs
    push gs
    pushad                ; Sauvegarde EAX, ECX, EDX, EBX, ESP, EBP, ESI, EDI
    
    ; Charge les segments du noyau
    mov ax, 0x10          ; Segment de données du noyau
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    
    ; Appelle le handler C du clavier
    call keyboard_interrupt_handler
    
    ; Envoie EOI au PIC pour IRQ 1
    mov al, 0x20          ; Commande EOI
    out 0x20, al          ; Envoie à PIC1
    
    ; Restaure l'état complet du processeur
    popad                 ; Restaure tous les registres généraux
    pop gs
    pop fs
    pop es
    pop ds
    
    ; Retour d'interruption
    iret

; ISR pour la carte réseau NE2000 (IRQ 3)
irq3:
    push ds
    push es
    push fs
    push gs
    pushad
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    call ne2k_irq_handler
    mov al, 0x20
    out 0x20, al
    popad
    pop gs
    pop fs
    pop es
    pop ds
    iret

; ISR pour le scheduler volontaire (INT 0x30)
global isr_schedule
isr_schedule:
    push ds
    push es
    push fs
    push gs
    pushad

    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    push esp
    call timer_handler ; Le timer handler appelle schedule, c'est ce qu'on veut
    add esp, 4

    popad
    pop gs
    pop fs
    pop es
    pop ds

    iret

; ISR pour les appels système (INT 0x80)
isr_syscall:
    ; Sauvegarde l'état complet du CPU
    push ds
    push es
    push fs
    push gs
    pushad
    
    ; Charge les segments du noyau
    mov ax, 0x10  ; Segment de données du noyau
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    
    ; Prépare la structure cpu_state_t sur la pile
    ; L'ordre doit correspondre à la structure dans task.h
    push esp      ; Pointeur vers la structure
    
    ; Appelle le handler C des syscalls
    call syscall_handler
    
    ; Nettoie la pile
    add esp, 4
    
    ; Restaure l'état du CPU
    popad
    pop gs
    pop fs
    pop es
    pop ds
    
    ; Retour d'interruption
    iret

; Common C-level fault handler
extern fault_handler_c

; Macro for ISRs that don't push an error code
%macro ISR_NO_ERR 1
global isr%1
isr%1:
    cli
    push 0      ; Push a dummy error code
    push %1     ; Push the interrupt number
    jmp isr_common_stub
%endmacro

; Macro for ISRs that do push an error code
%macro ISR_ERR 1
global isr%1
isr%1:
    cli
    ; Error code is already on the stack
    push %1     ; Push the interrupt number
    jmp isr_common_stub
%endmacro

; Common stub that saves state and calls the C handler
isr_common_stub:
    ; 1. Save general purpose registers
    pushad

    ; 2. Save data segment registers
    push gs
    push fs
    push es
    push ds

    ; 3. Load kernel data segments
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    ; 4. Call the C handler, passing a pointer to the stack frame
    push esp
    call fault_handler_c
    add esp, 4 ; Clean up the stack pointer argument

    ; 5. Restore data segment registers
    pop ds
    pop es
    pop fs
    pop gs

    ; 6. Restore general purpose registers
    popad

    ; 7. Clean up the interrupt number and error code from the stack
    add esp, 8

    ; 8. Return from interrupt
    iret

; Create the 32 ISR stubs
ISR_NO_ERR  0   ; Divide by zero
ISR_NO_ERR  1   ; Debug
ISR_NO_ERR  2   ; Non-maskable Interrupt
ISR_NO_ERR  3   ; Breakpoint
ISR_NO_ERR  4   ; Overflow
ISR_NO_ERR  5   ; Bound Range Exceeded
ISR_NO_ERR  6   ; Invalid Opcode
ISR_NO_ERR  7   ; Device Not Available
ISR_ERR     8   ; Double Fault
ISR_NO_ERR  9   ; Coprocessor Segment Overrun
ISR_ERR     10  ; Invalid TSS
ISR_ERR     11  ; Segment Not Present
ISR_ERR     12  ; Stack-Segment Fault
ISR_ERR     13  ; General Protection Fault
ISR_ERR     14  ; Page Fault
ISR_NO_ERR  15  ; Reserved
ISR_NO_ERR  16  ; x87 Floating-Point Exception
ISR_ERR     17  ; Alignment Check
ISR_NO_ERR  18  ; Machine Check
ISR_NO_ERR  19  ; SIMD Floating-Point Exception
ISR_NO_ERR  20  ; Virtualization Exception
ISR_ERR     21  ; Control Protection Exception
ISR_NO_ERR  22
ISR_NO_ERR  23
ISR_NO_ERR  24
ISR_NO_ERR  25
ISR_NO_ERR  26
ISR_NO_ERR  27
ISR_NO_ERR  28
ISR_NO_ERR  29
ISR_NO_ERR  30  ; Security Exception
ISR_NO_ERR  31  ; Reserved


; Section GNU stack (sécurité - pile non exécutable)
section .note.GNU-stack
