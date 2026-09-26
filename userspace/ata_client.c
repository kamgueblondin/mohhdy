/* Tranche 4 positive proof: owner of "ata-client" writes then reads back a
 * 16-byte window of a scratch sector through the Ring 3 atadriver over IPC.
 * The scratch LBA lies past the overlay snapshot (LBA 0-63) and past the FAT16
 * volume fence published by the kernel (SYS_ATA_STATUS client_min_lba). */
#include "os_syscalls.h"

static void putc(char c) { asm volatile("int $0x80" : : "a"(SYS_PUTC), "b"(c)); }
static void puts(const char* t) { int i = 0; while (t[i]) putc(t[i++]); }
static void yield(void) { asm volatile("int $0x80" : : "a"(SYS_YIELD)); }
static int sc1(uint32_t n, uint32_t a) { int r; asm volatile("int $0x80" : "=a"(r) : "a"(n), "b"(a)); return r; }
static int sc2(uint32_t n, uint32_t a, uint32_t b) { int r; asm volatile("int $0x80" : "=a"(r) : "a"(n), "b"(a), "c"(b)); return r; }

#define SCRATCH_LBA_MIN 2000U

static int32_t get32(const uint8_t* p) { return (int32_t)(p[0] | (p[1]<<8) | ((uint32_t)p[2]<<16) | ((uint32_t)p[3]<<24)); }

static int ata_call(int driver, uint32_t type, uint32_t rid, uint32_t lba, const char* data,
                    uint8_t len, os_ipc_message_t* reply) {
    os_ipc_payload_t p;
    uint32_t i, turns;
    for (i = 0; i < sizeof(p); i++) ((uint8_t*)&p)[i] = 0;
    p.type = type; p.request_id = rid; p.size = 8U + len;
    p.data[0] = (uint8_t)lba; p.data[1] = (uint8_t)(lba >> 8);
    p.data[2] = (uint8_t)(lba >> 16); p.data[3] = (uint8_t)(lba >> 24);
    p.data[4] = 0; p.data[5] = 0; p.data[6] = len;
    for (i = 0; data && i < len; i++) p.data[8 + i] = (uint8_t)data[i];
    if (sc2(SYS_IPC_SEND, (uint32_t)driver, (uint32_t)&p) != 0) return -1;
    for (turns = 0; turns < 400U; turns++) {
        if (sc1(SYS_IPC_RECV, (uint32_t)reply) == 0 && reply->type == OS_IPC_ATA_REPLY &&
            reply->request_id == rid && reply->sender_pid == driver)
            return get32(reply->data);
        yield();
    }
    return -2;
}

void main(void) {
    const char* pattern = "mohhdy-ring3-ata";
    os_ipc_message_t reply;
    int driver, rc, i, ok;
    os_ata_status_t st;
    uint32_t scratch = SCRATCH_LBA_MIN;
    if (sc1(SYS_SERVICE_REGISTER, (uint32_t)OS_ATA_IPC_CLIENT_SERVICE) != 0) {
        puts("ataclient register failed\n"); for (;;) yield();
    }
    driver = sc1(SYS_SERVICE_LOOKUP, (uint32_t)"ata-driver");
    if (driver <= 0) { puts("ataclient no driver\n"); for (;;) yield(); }
    if (sc1(SYS_ATA_STATUS, (uint32_t)&st) != 0) { puts("ataclient status failed\n"); for (;;) yield(); }
    if (st.client_min_lba + 64U > scratch) scratch = st.client_min_lba + 64U;
    rc = ata_call(driver, OS_IPC_ATA_WRITE, 1U, scratch, pattern, 16U, &reply);
    if (rc != 16) { puts("ataclient write failed\n"); for (;;) yield(); }
    rc = ata_call(driver, OS_IPC_ATA_READ, 2U, scratch, 0, 16U, &reply);
    ok = rc == 16;
    for (i = 0; ok && i < 16; i++) if (reply.data[4 + i] != (uint8_t)pattern[i]) ok = 0;
    puts(ok ? "ataclient ring3 sector roundtrip ok\n" : "ataclient readback mismatch\n");
    /* The overlay snapshot sectors stay owned by the kernel Ring 0 path. */
    rc = ata_call(driver, OS_IPC_ATA_WRITE, 3U, 0U, pattern, 16U, &reply);
    puts(rc == OS_ATA_DRIVER_REQUIRED ? "ataclient overlay region refused\n"
                                      : "ataclient overlay region unexpected\n");
    /* Slice 2: the FAT16 volume (LBA 64 .. client_min_lba-1) is fenced too. */
    if (st.client_min_lba > OS_ATA_KERNEL_RESERVED_LBAS) {
        rc = ata_call(driver, OS_IPC_ATA_WRITE, 4U, st.client_min_lba - 1U, pattern, 16U, &reply);
        puts(rc == OS_ATA_DRIVER_REQUIRED ? "ataclient fat region refused\n"
                                          : "ataclient fat region unexpected\n");
    } else {
        puts("ataclient fat region absent\n");
    }
    for (;;) yield();
}
