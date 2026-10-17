/* Lot 4 (US-001): Ring 3 PCI enumeration worker (binary pcidriver).
 *
 * Registers "pci-driver" (name pinned to this binary), asks the kernel to
 * open ports 0xCF8-0xCFF in its TSS IOPB (OS_PCI_OP_CLAIM), walks
 * configuration mechanism #1 itself at CPL 3 and publishes the inventory
 * (OS_PCI_OP_PUBLISH). From then on the kernel refuses its own config
 * reads and serves `lspci` and the framebuffer BAR from this inventory.
 * The task then sleeps in SYS_IPC_RECV_WAIT; a "rescan" message makes it
 * walk the bus again and republish. After its death the kernel respawns
 * bin/pcidriver on the next client request (budgeted). */
#include "ipc_wait_common.h"

static inline void outl(uint16_t port, uint32_t value) {
    asm volatile("outl %0, %1" : : "a"(value), "Nd"(port));
}
static inline uint32_t inl(uint16_t port) {
    uint32_t value;
    asm volatile("inl %1, %0" : "=a"(value) : "Nd"(port));
    return value;
}

static uint32_t cfg_read(uint32_t bus, uint32_t slot, uint32_t function, uint32_t offset) {
    outl(0xCF8U, 0x80000000U | (bus << 16) | (slot << 11) | (function << 8) | (offset & 0xFCU));
    return inl(0xCFCU);
}

static int pci_op(uint32_t op, uint32_t arg, uint32_t count) {
    int rc;
    asm volatile("int $0x80" : "=a"(rc) : "a"(SYS_PCI_SERVICE), "b"(op), "c"(arg), "d"(count) : "memory");
    return rc;
}

static os_pci_record_t records[OS_PCI_MAX_RECORDS];

/* Buses 0..7 (QEMU pc: bus 0), 32 slots, 8 functions when multi-function. */
static uint32_t enumerate(void) {
    uint32_t bus, slot, function, count = 0U;
    for (bus = 0U; bus < 8U; bus++) {
        for (slot = 0U; slot < 32U; slot++) {
            uint32_t functions = 1U;
            for (function = 0U; function < functions; function++) {
                uint32_t id = cfg_read(bus, slot, function, 0x00U);
                uint32_t class_info, header;
                os_pci_record_t* r;
                if ((id & 0xffffU) == 0xffffU || (id & 0xffffU) == 0U) continue;
                header = (cfg_read(bus, slot, function, 0x0CU) >> 16) & 0xffU;
                if (function == 0U && (header & 0x80U)) functions = 8U;
                if (count >= OS_PCI_MAX_RECORDS) return count;
                class_info = cfg_read(bus, slot, function, 0x08U);
                r = &records[count++];
                r->bus = (uint8_t)bus;
                r->slot = (uint8_t)slot;
                r->function = (uint8_t)function;
                r->vendor_id = (uint16_t)(id & 0xffffU);
                r->device_id = (uint16_t)(id >> 16);
                r->class_code = (uint8_t)(class_info >> 24);
                r->subclass = (uint8_t)(class_info >> 16);
                r->prog_if = (uint8_t)(class_info >> 8);
                r->header_type = (uint8_t)header;
                r->reserved = 0U;
                r->bar0 = (header & 0x7fU) == 0U ? cfg_read(bus, slot, function, 0x10U) : 0U;
            }
        }
    }
    return count;
}

static void publish(const char* what) {
    uint32_t count = enumerate();
    int rc = pci_op(OS_PCI_OP_PUBLISH, (uint32_t)records, count);
    iw_puts("pcidriver ");
    iw_puts(what);
    iw_puts(" ring3 enumeration functions ");
    iw_print_int((int)count);
    iw_puts(" publish rc ");
    iw_print_int(rc);
    iw_putc('\n');
}

static int data_is(const os_ipc_message_t* m, const char* text) {
    uint32_t i = 0U;
    while (text[i] != '\0') {
        if (i >= m->size || m->data[i] != (uint8_t)text[i]) return 0;
        i++;
    }
    return i == m->size;
}

void main(void) {
    os_ipc_message_t message;
    int rc = iw_service_register("pci-driver");
    if (rc < 0) {
        iw_puts("pcidriver register refused rc ");
        iw_print_int(rc);
        iw_putc('\n');
        return;
    }
    rc = pci_op(OS_PCI_OP_CLAIM, 0U, 0U);
    if (rc != 0) {
        iw_puts("pcidriver claim refused rc ");
        iw_print_int(rc);
        iw_putc('\n');
        return;
    }
    publish("boot");
    for (;;) {
        rc = iw_recv_wait(&message, OS_IPC_WAIT_FOREVER);
        if (rc != 0) continue;
        if (data_is(&message, "rescan")) publish("rescan");
    }
}
