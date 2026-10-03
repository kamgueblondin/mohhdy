extern keyboard_interrupt_handler
extern timer_handler
extern ne2k_irq_handler
extern syscall_handler

extern current_task
extern task_fx_state_offset

; Per-task FPU/SSE state (FXSAVE). On an entry from Ring 3 the user state is
; saved into current_task->fx_state and restored on the way back, so kernel
; code compiled with -msse2 (and other tasks) never leak into a Ring 3 SSE2
; user. An entry from Ring 0 (IRQ during a syscall, e.g. the kernel GPT-2
; fallback) saves the state on the kernel stack instead (nesting-safe).
; %1 = offset of the saved CS from ESP right after the register pushes.
; EBP keeps the frame pointer (callee-saved by the C handlers).
%macro FPU_ENTER 1
    mov ebp, esp
    test dword [ebp + %1], 3
    jz %%ring0
    mov eax, [current_task]
    test eax, eax
    jz %%done
    add eax, [task_fx_state_offset]
    fxsave [eax]
    jmp %%done
%%ring0:
    sub esp, 512
    and esp, 0xFFFFFFF0
    fxsave [esp]
%%done:
%endmacro

%macro FPU_LEAVE 1
    test dword [ebp + %1], 3
    jz %%ring0
    mov eax, [current_task]
    test eax, eax
    jz %%done
    add eax, [task_fx_state_offset]
    fxrstor [eax]
    jmp %%done
%%ring0:
    fxrstor [esp]
%%done:
    mov esp, ebp
%endmacro

; Frame of the IRQ/syscall stubs: pushad (32) + gs fs es ds (16), then
; EIP at +48 and CS at +52.
%define IRQ_CS 52

global irq0
global irq1
global irq3
global isr_syscall

; ISR pour le timer (IRQ 0)
irq0:
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

    mov al, 0x20
    out 0x20, al

    FPU_ENTER IRQ_CS
    push ebp
    call timer_handler
    add esp, 4
    FPU_LEAVE IRQ_CS

    popad
    pop gs
    pop fs
    pop es
    pop ds

    iret

irq1:
    push ds               ; Sauvegarde des segments
    push es
    push fs
    push gs
    pushad                ; Sauvegarde EAX, ECX, EDX, EBX, ESP, EBP, ESI, EDI

    mov ax, 0x10          ; Segment de donnees du noyau
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    FPU_ENTER IRQ_CS
    call keyboard_interrupt_handler
    FPU_LEAVE IRQ_CS

    mov al, 0x20          ; Commande EOI
    out 0x20, al          ; Envoie a PIC1

    popad                 ; Restaure tous les registres generaux
    pop gs
    pop fs
    pop es
    pop ds

    iret

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
    FPU_ENTER IRQ_CS
    call ne2k_irq_handler
    FPU_LEAVE IRQ_CS
    mov al, 0x20
    out 0x20, al
    popad
    pop gs
    pop fs
    pop es
    pop ds
    iret

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

    FPU_ENTER IRQ_CS
    push ebp
    call timer_handler ; Le timer handler appelle schedule, c'est ce qu'on veut
    add esp, 4
    FPU_LEAVE IRQ_CS

    popad
    pop gs
    pop fs
    pop es
    pop ds

    iret

isr_syscall:
    push ds
    push es
    push fs
    push gs
    pushad

    mov ax, 0x10  ; Segment de donnees du noyau
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    FPU_ENTER IRQ_CS
    push ebp      ; Pointeur vers la structure (cpu_state_t*)

    call syscall_handler

    add esp, 4
    FPU_LEAVE IRQ_CS

    ; Restaure l'etat du CPU
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
    ; Ring 3 faults: FPU/SSE state into the task (restored if it resumes).
    ; Ring 0 faults: stack save. CS is at +60 (segments, pushad, int, err).
    FPU_ENTER 60
    push ebp
    call fault_handler_c
    add esp, 4 ; Clean up the stack pointer argument
    FPU_LEAVE 60

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
