/* Lot 8 (US-076): ARMv7-A entry for QEMU -M virt (cortex-a15), PL1.
 * Sets one stack per exception mode, clears BSS, installs the vector
 * table (VBAR) and calls arm_main. Exceptions taken from user tasks save
 * the full user context into *g_cur_ctx (arm_ctx_t layout: r0-r12 at 0,
 * sp_usr 52, lr_usr 56, pc 60, cpsr 64), call a C handler that may
 * switch g_cur_ctx, then restore whatever context is current. */
    .syntax unified
    .arm
    .section .text.boot, "ax"
    .global _start
_start:
    cpsid if
    /* IRQ, ABT, UND stacks, then SVC (stays the kernel mode). */
    cps #0x12
    ldr sp, =__irq_stack_top
    cps #0x17
    ldr sp, =__abt_stack_top
    cps #0x1B
    ldr sp, =__und_stack_top
    cps #0x13
    ldr sp, =__svc_stack_top
    ldr r0, =__bss_start
    ldr r1, =__bss_end
    mov r2, #0
1:  cmp r0, r1
    strlo r2, [r0], #4
    blo 1b
    ldr r0, =arm_vectors
    mcr p15, 0, r0, c12, c0, 0      /* VBAR */
    isb
    bl arm_main
2:  wfi
    b 2b

    .balign 32
    .global arm_vectors
arm_vectors:
    b _start
    b und_entry
    b svc_entry
    b pabt_entry
    b dabt_entry
    b .
    b irq_entry
    b .

/* SAVE_CTX: lr already holds the resume pc. */
.macro SAVE_CTX
    stmfd sp!, {r0}
    ldr r0, =g_cur_ctx
    ldr r0, [r0]
    add r0, r0, #4
    stmia r0, {r1-r12}
    sub r0, r0, #4
    ldmfd sp!, {r1}
    str r1, [r0]
    add r1, r0, #52
    stmia r1, {sp, lr}^
    nop
    str lr, [r0, #60]
    mrs r1, spsr
    str r1, [r0, #64]
.endm

    .global arm_restore_ctx
arm_restore_ctx:
    ldr r0, =g_cur_ctx
    ldr r0, [r0]
    ldr r1, [r0, #64]
    msr spsr_cxsf, r1
    ldr lr, [r0, #60]
    add r1, r0, #52
    ldmia r1, {sp, lr}^
    nop
    ldmia r0, {r0-r12}
    movs pc, lr

irq_entry:
    sub lr, lr, #4
    SAVE_CTX
    bl arm_irq_handler
    b arm_restore_ctx

svc_entry:
    SAVE_CTX
    bl arm_svc_handler
    b arm_restore_ctx

dabt_entry:
    sub lr, lr, #8
    SAVE_CTX
    mrc p15, 0, r0, c5, c0, 0       /* DFSR */
    mrc p15, 0, r1, c6, c0, 0       /* DFAR */
    bl arm_abort_handler
    b arm_restore_ctx

pabt_entry:
    sub lr, lr, #4
    SAVE_CTX
    mrc p15, 0, r0, c5, c0, 1       /* IFSR */
    mrc p15, 0, r1, c6, c0, 2       /* IFAR */
    orr r0, r0, #0x80000000         /* mark prefetch */
    bl arm_abort_handler
    b arm_restore_ctx

und_entry:
    sub lr, lr, #4
    SAVE_CTX
    mov r0, #0x40000000             /* mark undefined */
    mov r1, lr
    bl arm_abort_handler
    b arm_restore_ctx
