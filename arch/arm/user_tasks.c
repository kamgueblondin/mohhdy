/* Lot 8 (US-076): user tasks of the ARM port. This object is linked into
 * the user section only (arch/arm/arm.ld): code, strings, data and
 * stacks are all user accessible, nothing here can call kernel code. */
#include <stdint.h>
#include "os_syscalls.h"

uint32_t user_stacks[5][256] __attribute__((aligned(8)));
volatile uint32_t user_spin_counter;
volatile uint32_t user_spin_counter2;

static int svc(uint32_t n, uint32_t a, uint32_t b) {
    register uint32_t r0 __asm__("r0") = n;
    register uint32_t r1 __asm__("r1") = a;
    register uint32_t r2 __asm__("r2") = b;
    __asm__ volatile("svc #0" : "+r"(r0) : "r"(r1), "r"(r2) : "memory");
    return (int)r0;
}

static void say(const char* s) { (void)svc(SYS_PUTS, (uint32_t)s, 0U); }

static void say_hex(uint32_t v) {
    char b[11];
    int i;
    b[0] = '0'; b[1] = 'x';
    for (i = 0; i < 8; i++) b[2 + i] = "0123456789abcdef"[(v >> (28 - 4 * i)) & 0xFU];
    b[10] = 0;
    say(b);
}

void user_reporter(void) {
    os_ipc_payload_t p;
    int pid = svc(SYS_GETPID, 0U, 0U);
    uint32_t i;
    say("arm user reporter pid ");
    say_hex((uint32_t)pid);
    say(" running in USR mode via svc\n");
    p.type = 7U;
    p.size = 4U;
    p.request_id = 0xC0FFEEU;
    for (i = 0; i < 96U; i++) p.data[i] = 0U;
    p.data[0] = 'm'; p.data[1] = 'o'; p.data[2] = 'h'; p.data[3] = 0;
    say(svc(SYS_IPC_SEND, 2U, (uint32_t)&p) == 0 ? "arm user ipc send ok\n" : "arm user ipc send failed\n");
    (void)svc(SYS_EXIT, 0U, 0U);
}

void user_consumer(void) {
    os_ipc_message_t m;
    for (;;) {
        int rc = svc(SYS_IPC_RECV, (uint32_t)&m, 0U);
        if (rc == 0) break;
        (void)svc(SYS_YIELD, 0U, 0U);
    }
    say("arm user ipc received from pid ");
    say_hex((uint32_t)m.sender_pid);
    say(" request ");
    say_hex(m.request_id);
    say(" data ");
    say((const char*)m.data);
    say("\n");
    (void)svc(SYS_EXIT, 0U, 0U);
}

/* Never enters the kernel: only the timer can take the CPU away. */
void user_spinner(void) {
    for (;;) user_spin_counter++;
}

/* A second task that never yields: both only run if the timer preempts. */
void user_spinner2(void) {
    for (;;) user_spin_counter2++;
}

void user_rogue(void) {
    int rc = svc(SYS_PUTS, 0x40000100U, 0U);    /* kernel pointer */
    say("arm user rogue puts kernel pointer rc ");
    say_hex((uint32_t)rc);
    say("\n");
    say("arm user rogue reads kernel memory\n");
    user_spin_counter2 += *(volatile uint32_t*)0x40000000U;
    say("arm user rogue UNEXPECTED: kernel memory readable\n");
    (void)svc(SYS_EXIT, 0U, 0U);
}
