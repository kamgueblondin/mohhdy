/* Tranche 4 negative proofs from a task that is NOT the ATA driver.
 * mode "ipc"  : cannot publish ata-driver (-58) and the driver refuses its
 *               sector IPC (not the ata-client owner).
 * mode "port" : a raw IN on 0x1F7 must raise #GP (TSS IOPB denies it); the
 *               kernel kills this task instead of halting. */
#include "os_syscalls.h"

static void putc(char c) { asm volatile("int $0x80" : : "a"(SYS_PUTC), "b"(c)); }
static void puts(const char* t) { int i = 0; while (t[i]) putc(t[i++]); }
static void yield(void) { asm volatile("int $0x80" : : "a"(SYS_YIELD)); }
static int sc1(uint32_t n, uint32_t a) { int r; asm volatile("int $0x80" : "=a"(r) : "a"(n), "b"(a)); return r; }
static int sc2(uint32_t n, uint32_t a, uint32_t b) { int r; asm volatile("int $0x80" : "=a"(r) : "a"(n), "b"(a), "c"(b)); return r; }

void main(void) {
    os_ipc_payload_t p;
    os_ipc_message_t m;
    uint32_t i, turns;
    int driver, rc;
    puts("atarogue start\n");
    rc = sc1(SYS_SERVICE_REGISTER, (uint32_t)"ata-driver");
    if (rc == OS_ATA_DRIVER_REQUIRED) puts("atarogue register refused\n");
    else puts("atarogue register unexpected\n");
    rc = sc1(SYS_SERVICE_REGISTER, (uint32_t)OS_ATA_IPC_CLIENT_SERVICE);
    if (rc == OS_ATA_DRIVER_REQUIRED) puts("atarogue client claim refused\n");
    else puts("atarogue client claim unexpected\n");
    driver = sc1(SYS_SERVICE_LOOKUP, (uint32_t)"ata-driver");
    if (driver > 0) {
        for (i = 0; i < sizeof(p); i++) ((uint8_t*)&p)[i] = 0;
        p.type = OS_IPC_ATA_READ; p.request_id = 77U; p.size = 8U; p.data[6] = 16U;
        (void)sc2(SYS_IPC_SEND, (uint32_t)driver, (uint32_t)&p);
        for (turns = 0; turns < 400U; turns++) {
            if (sc1(SYS_IPC_RECV, (uint32_t)&m) == 0 && m.type == OS_IPC_ATA_REPLY &&
                m.request_id == 77U) {
                int32_t st = (int32_t)(m.data[0] | (m.data[1] << 8) |
                                       ((uint32_t)m.data[2] << 16) | ((uint32_t)m.data[3] << 24));
                puts(st == OS_ATA_DRIVER_REQUIRED ? "atarogue sector ipc refused\n"
                                                  : "atarogue sector ipc unexpected\n");
                break;
            }
            yield();
        }
    }
    puts("atarogue port probe\n");
    {
        uint8_t v;
        asm volatile("inb %1, %0" : "=a"(v) : "Nd"((uint16_t)0x1F7));
        (void)v;
    }
    puts("atarogue port unexpectedly allowed\n");
    for (;;) yield();
}
