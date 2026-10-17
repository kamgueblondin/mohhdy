/* Lot 4 (US-001): pcirogue proves the PCI capability is pinned to the
 * live pcidriver: publish, claim and the "pci-driver" name are refused,
 * then a raw IN on 0xCFC must raise #GP (TSS IOPB denies it). */
#include "ipc_wait_common.h"

static int pci_op(uint32_t op, uint32_t arg, uint32_t count) {
    int rc;
    asm volatile("int $0x80" : "=a"(rc) : "a"(SYS_PCI_SERVICE), "b"(op), "c"(arg), "d"(count) : "memory");
    return rc;
}

void main(void) {
    static os_pci_record_t fake[1];
    uint32_t value;
    int rc;
    fake[0].vendor_id = 0x1234U;
    fake[0].device_id = 0x5678U;
    fake[0].class_code = 0x03U;
    fake[0].bar0 = 0xDEAD0000U;
    rc = pci_op(OS_PCI_OP_PUBLISH, (uint32_t)fake, 1U);
    iw_puts(rc == OS_PCI_DENIED ? "pcirogue publish refused\n" : "pcirogue publish unexpected\n");
    rc = pci_op(OS_PCI_OP_CLAIM, 0U, 0U);
    iw_puts(rc == OS_PCI_DENIED ? "pcirogue claim refused\n" : "pcirogue claim unexpected\n");
    rc = iw_service_register("pci-driver");
    iw_puts(rc < 0 ? "pcirogue register refused\n" : "pcirogue register unexpected\n");
    iw_puts("pcirogue port probe\n");
    asm volatile("outl %0, %1" : : "a"(0x80000000U), "Nd"((uint16_t)0xCF8U));
    asm volatile("inl %1, %0" : "=a"(value) : "Nd"((uint16_t)0xCFCU));
    (void)value;
    iw_puts("pcirogue port unexpectedly allowed\n");
}
