/* Lot 8 (US-076): Mohhdy kernel core on ARMv7-A (QEMU -M virt, cortex-a15).
 *
 * What this port does, on real ARM code paths (no simulation):
 *   - PL011 UART console, GICv2 distributor + CPU interface, ARM generic
 *     timer (CNTP, PPI 30) at 100 Hz;
 *   - MMU on with 1 MB sections: the kernel section and the devices are
 *     PL1-only, one section (ARM_USER_BASE) is user accessible;
 *   - user tasks run in USR mode (the ARM equivalent of Ring 3), enter
 *     the kernel with SVC using the i386 syscall numbers (SYS_PUTS,
 *     SYS_YIELD, SYS_GETPID, SYS_IPC_SEND, SYS_IPC_RECV, SYS_EXIT) and are
 *     preempted by the timer (round robin);
 *   - every user pointer is checked against the user section; a user task
 *     touching kernel memory takes a permission fault and only that task
 *     dies;
 *   - portable kernel code compiled unchanged from kernel/: ipc.c (the
 *     same mailbox as i386) and sha256.c (boot self-test).
 * Not ported (see docs/platform.md): the drivers, the filesystem, the AI
 * engine, the shell and the OS-UI. */
#include <stdint.h>
#include "arm_mmu.h"
#include "os_syscalls.h"
#include "ipc.h"
#include "sha256.h"

#define UART0 0x09000000U
#define GICD 0x08000000U
#define GICC 0x08010000U
#define TIMER_IRQ 30U
#define HZ 100U
#define MAX_TASKS 6
#define REPORT_TICK 150U

typedef struct { uint32_t r[13]; uint32_t sp, lr, pc, cpsr; } arm_ctx_t;

enum { T_FREE = 0, T_READY, T_DEAD };

typedef struct {
    arm_ctx_t ctx;
    int state;
    int pid;
    const char* name;
    uint32_t runs;
    ipc_endpoint_t box;
} arm_task_t;

arm_ctx_t* g_cur_ctx;
static arm_task_t g_tasks[MAX_TASKS];
static int g_cur;
static int g_count;
static volatile uint32_t g_ticks;
static uint32_t g_switches, g_svc_calls, g_refused, g_killed;
static uint32_t g_ttb[4096] __attribute__((aligned(16384)));
static arm_ctx_t g_boot_ctx;

extern void arm_restore_ctx(void);
extern char __user_start[], __user_end[];

/* user task entry points and stacks (arch/arm/user_tasks.c) */
extern void user_reporter(void);
extern void user_consumer(void);
extern void user_spinner(void);
extern void user_spinner2(void);
extern void user_rogue(void);
extern uint32_t user_stacks[][256];
extern volatile uint32_t user_spin_counter;
extern volatile uint32_t user_spin_counter2;

static inline void w32(uint32_t a, uint32_t v) { *(volatile uint32_t*)a = v; }
static inline uint32_t r32(uint32_t a) { return *(volatile uint32_t*)a; }

static void uart_putc(char c) {
    while (r32(UART0 + 0x18U) & (1U << 5)) { }
    w32(UART0, (uint32_t)(unsigned char)c);
}
static void kputs(const char* s) { while (*s) { if (*s == '\n') uart_putc('\r'); uart_putc(*s++); } }
static void kput_u32(uint32_t v) {
    char b[11]; int i = 10; b[i] = 0;
    if (!v) { uart_putc('0'); return; }
    while (v && i) { b[--i] = (char)('0' + v % 10U); v /= 10U; }
    kputs(&b[i]);
}
static void kput_hex(uint32_t v) {
    int i; kputs("0x");
    for (i = 28; i >= 0; i -= 4) uart_putc("0123456789abcdef"[(v >> i) & 0xFU]);
}

static void mmu_init(void) {
    uint32_t i;
    for (i = 0; i < 4096U; i++) g_ttb[i] = 0U;                  /* fault */
    g_ttb[arm_section_index(GICD)] = arm_section_desc(GICD, ARM_AP_PRIV_ONLY, ARM_MEM_DEVICE, 1);
    g_ttb[arm_section_index(UART0)] = arm_section_desc(UART0, ARM_AP_PRIV_ONLY, ARM_MEM_DEVICE, 1);
    g_ttb[arm_section_index(ARM_KERNEL_BASE)] =
        arm_section_desc(ARM_KERNEL_BASE, ARM_AP_PRIV_ONLY, ARM_MEM_NORMAL, 0);
    g_ttb[arm_section_index(ARM_USER_BASE)] =
        arm_section_desc(ARM_USER_BASE, ARM_AP_FULL, ARM_MEM_NORMAL, 0);
    __asm__ volatile(
        "mov r0, #0\n"
        "mcr p15, 0, r0, c2, c0, 2\n"      /* TTBCR = 0: TTBR0 only */
        "mcr p15, 0, %0, c2, c0, 0\n"      /* TTBR0 */
        "ldr r0, =0x55555555\n"
        "mcr p15, 0, r0, c3, c0, 0\n"      /* DACR: client, AP is checked */
        "mov r0, #0\n"
        "mcr p15, 0, r0, c8, c7, 0\n"      /* TLBIALL */
        "dsb\n isb\n"
        "mrc p15, 0, r0, c1, c0, 0\n"
        "orr r0, r0, #1\n"                 /* M */
        "orr r0, r0, #(1 << 2)\n"          /* C */
        "orr r0, r0, #(1 << 12)\n"         /* I */
        "mcr p15, 0, r0, c1, c0, 0\n"
        "isb\n"
        :: "r"(g_ttb) : "r0", "memory");
}

static void timer_arm(void) {
    uint32_t frq;
    __asm__ volatile("mrc p15, 0, %0, c14, c0, 0" : "=r"(frq));
    __asm__ volatile("mcr p15, 0, %0, c14, c2, 0" :: "r"(frq / HZ));   /* CNTP_TVAL */
    __asm__ volatile("mcr p15, 0, %0, c14, c2, 1" :: "r"(1U));         /* CNTP_CTL enable */
    __asm__ volatile("isb");
}

static void gic_init(void) {
    w32(GICD + 0x000U, 1U);                                  /* GICD_CTLR */
    w32(GICD + 0x400U + (TIMER_IRQ & ~3U), 0xA0A0A0A0U);     /* priority */
    w32(GICD + 0x100U, 1U << TIMER_IRQ);                     /* ISENABLER0 */
    w32(GICC + 0x004U, 0xF0U);                               /* PMR */
    w32(GICC + 0x000U, 1U);                                  /* GICC_CTLR */
}

static int spawn(const char* name, void (*entry)(void), int stack) {
    arm_task_t* t;
    uint32_t i;
    if (g_count >= MAX_TASKS) return -1;
    t = &g_tasks[g_count];
    for (i = 0; i < 13U; i++) t->ctx.r[i] = 0U;
    t->ctx.sp = (uint32_t)&user_stacks[stack][256];
    t->ctx.lr = 0U;
    t->ctx.pc = (uint32_t)entry;
    t->ctx.cpsr = 0x10U;                                     /* USR, IRQ on, ARM */
    t->state = T_READY;
    t->pid = g_count + 1;
    t->name = name;
    t->runs = 0U;
    ipc_endpoint_init(&t->box);
    g_count++;
    return t->pid;
}

static void schedule(void) {
    int i, n;
    for (i = 1; i <= g_count; i++) {
        n = (g_cur + i) % g_count;
        if (g_tasks[n].state == T_READY) {
            if (n != g_cur) g_switches++;
            g_cur = n;
            g_tasks[n].runs++;
            g_cur_ctx = &g_tasks[n].ctx;
            return;
        }
    }
    kputs("arm kernel: no runnable task, halt\n");
    for (;;) __asm__ volatile("wfi");
}

static void kill_current(const char* why, uint32_t detail) {
    arm_task_t* t = &g_tasks[g_cur];
    t->state = T_DEAD;
    g_killed++;
    kputs("arm task ");
    kput_u32((uint32_t)t->pid);
    kputs(" (");
    kputs(t->name);
    kputs(") killed: ");
    kputs(why);
    kputs(" at ");
    kput_hex(detail);
    kputs("\n");
    schedule();
}

static void report(void) {
    uint32_t i;
    kputs("arm preempt report ticks ");
    kput_u32(g_ticks);
    kputs(" switches ");
    kput_u32(g_switches);
    kputs(" svc ");
    kput_u32(g_svc_calls);
    kputs(" refused ");
    kput_u32(g_refused);
    kputs(" killed ");
    kput_u32(g_killed);
    kputs(" spinner ");
    kputs(user_spin_counter ? "advanced" : "stuck");
    kputs(" spinner2 ");
    kputs(user_spin_counter2 ? "advanced" : "stuck");
    for (i = 0; i < (uint32_t)g_count; i++) {
        kputs(" ");
        kputs(g_tasks[i].name);
        kputs("=");
        kput_u32(g_tasks[i].runs);
    }
    kputs("\n");
}

void arm_irq_handler(void) {
    uint32_t iar = r32(GICC + 0x00CU);
    uint32_t id = iar & 0x3FFU;
    if (id == TIMER_IRQ) {
        timer_arm();
        g_ticks++;
        if (g_ticks == REPORT_TICK) report();
        w32(GICC + 0x010U, iar);
        if ((g_cur_ctx->cpsr & 0x1FU) == 0x10U) schedule();     /* preempt user */
        return;
    }
    if (id < 1020U) w32(GICC + 0x010U, iar);
}

static int sys_puts(uint32_t ptr) {
    uint32_t n = 0U;
    const char* s = (const char*)ptr;
    while (n < 256U) {
        if (!arm_user_range_ok(ptr + n, 1U)) return -1;
        if (s[n] == 0) break;
        n++;
    }
    if (n == 256U) return -1;
    kputs(s);
    return (int)n;
}

void arm_svc_handler(void) {
    arm_ctx_t* c = g_cur_ctx;
    arm_task_t* t = &g_tasks[g_cur];
    uint32_t num = c->r[0];
    int rc = -1;
    g_svc_calls++;
    if ((c->cpsr & 0x1FU) != 0x10U) { kputs("arm kernel: SVC from PL1, halt\n"); for (;;) { } }
    switch (num) {
    case SYS_PUTS:
        rc = sys_puts(c->r[1]);
        if (rc < 0) g_refused++;
        break;
    case SYS_GETPID:
        rc = t->pid;
        break;
    case SYS_YIELD:
        c->r[0] = 0U;
        schedule();
        return;
    case SYS_EXIT:
        t->state = T_DEAD;
        schedule();
        return;
    case SYS_IPC_SEND: {
        int dest = (int)c->r[1];
        if (!arm_user_range_ok(c->r[2], sizeof(os_ipc_payload_t))) { g_refused++; rc = OS_IPC_BAD_MESSAGE; break; }
        if (dest < 1 || dest > g_count || g_tasks[dest - 1].state != T_READY) { rc = -1; break; }
        rc = ipc_endpoint_send(&g_tasks[dest - 1].box, t->pid, (const os_ipc_payload_t*)c->r[2]);
        break;
    }
    case SYS_IPC_RECV:
        if (!arm_user_range_ok(c->r[1], sizeof(os_ipc_message_t))) { g_refused++; rc = OS_IPC_BAD_MESSAGE; break; }
        rc = ipc_endpoint_receive(&t->box, (os_ipc_message_t*)c->r[1]);
        break;
    default:
        rc = -1;
        break;
    }
    c->r[0] = (uint32_t)rc;
}

void arm_abort_handler(uint32_t status, uint32_t addr) {
    if ((g_cur_ctx->cpsr & 0x1FU) != 0x10U) {
        kputs("arm kernel panic: abort in PL1 status ");
        kput_hex(status);
        kputs(" addr ");
        kput_hex(addr);
        kputs("\n");
        for (;;) __asm__ volatile("wfi");
    }
    if (status == 0x40000000U) { kill_current("undefined instruction", addr); return; }
    if (status & 0x80000000U) { kill_current("prefetch abort", addr); return; }
    kill_current(arm_dfsr_kind(status) == 1 ? "data abort (permission, kernel section)" : "data abort", addr);
}

static int sha256_selftest(void) {
    static const uint8_t want[4] = { 0xbaU, 0x78U, 0x16U, 0xbfU };   /* SHA-256("abc") */
    sha256_ctx_t ctx;
    uint8_t d[32];
    int i;
    sha256_init(&ctx);
    sha256_update(&ctx, (const uint8_t*)"abc", 3U);
    sha256_final(&ctx, d);
    for (i = 0; i < 4; i++) if (d[i] != want[i]) return 0;
    return d[31] == 0xadU;
}

void arm_main(void) {
    uint32_t midr;
    __asm__ volatile("mrc p15, 0, %0, c0, c0, 0" : "=r"(midr));
    kputs("\nMohhdy ARM kernel (US-076) armv7-a midr ");
    kput_hex(midr);
    kputs("\n");
    mmu_init();
    kputs("arm mmu on: kernel section PL1-only, user section ");
    kput_hex(ARM_USER_BASE);
    kputs(" (user image ");
    kput_hex((uint32_t)__user_start);
    kputs("..");
    kput_hex((uint32_t)__user_end);
    kputs(")\n");
    kputs(sha256_selftest() ? "arm sha256 self-test ok (kernel/sha256.c)\n"
                            : "arm sha256 self-test FAILED\n");
    gic_init();
    timer_arm();
    kputs("arm gic + generic timer armed at 100 Hz\n");
    spawn("reporter", user_reporter, 0);
    spawn("consumer", user_consumer, 1);
    spawn("spinner", user_spinner, 2);
    spawn("rogue", user_rogue, 3);
    spawn("spinner2", user_spinner2, 4);
    kputs("arm tasks spawned ");
    kput_u32((uint32_t)g_count);
    kputs(" (USR mode)\n");
    g_cur = g_count - 1;
    g_cur_ctx = &g_boot_ctx;
    schedule();
    arm_restore_ctx();
}
