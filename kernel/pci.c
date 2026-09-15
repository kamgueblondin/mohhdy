#include "pci.h"

static inline void pci_outl(uint16_t port, uint32_t value) {
    __asm__ volatile ("outl %0, %1" : : "a"(value), "Nd"(port));
}

static inline uint32_t pci_inl(uint16_t port) {
    uint32_t value;
    __asm__ volatile ("inl %1, %0" : "=a"(value) : "Nd"(port));
    return value;
}

uint32_t pci_config_address(uint8_t bus, uint8_t slot, uint8_t function,
                             uint8_t offset) {
    return 0x80000000U |
           ((uint32_t)bus << 16) |
           ((uint32_t)(slot & 0x1fU) << 11) |
           ((uint32_t)(function & 0x07U) << 8) |
           ((uint32_t)(offset & 0xfcU));
}

void pci_decode_id(uint32_t value, pci_device_t* device) {
    if (!device) return;
    device->vendor_id = (uint16_t)(value & 0xffffU);
    device->device_id = (uint16_t)(value >> 16);
}

#ifdef KERNEL_TEST
uint32_t pci_config_read32(uint8_t bus, uint8_t slot, uint8_t function,
                           uint8_t offset) {
    (void)bus; (void)slot; (void)function; (void)offset;
    return 0xffffffffU;
}
#else
uint32_t pci_config_read32(uint8_t bus, uint8_t slot, uint8_t function,
                           uint8_t offset) {
    if ((offset & 3U) != 0U) return 0xffffffffU;
    pci_outl(0xcf8U, pci_config_address(bus, slot, function, offset));
    return pci_inl(0xcfcU);
}
#endif

int pci_find_class(uint8_t class_code, uint8_t subclass, pci_device_t* out) {
    uint32_t bus;
    uint32_t slot;
    uint32_t function;
    if (!out) return -1;
    for (bus = 0; bus < 256U; ++bus) {
        for (slot = 0; slot < 32U; ++slot) {
            for (function = 0; function < 8U; ++function) {
                uint32_t id = pci_config_read32((uint8_t)bus, (uint8_t)slot,
                                                (uint8_t)function, 0);
                uint32_t class_info;
                if ((id & 0xffffU) == 0xffffU) continue;
                class_info = pci_config_read32((uint8_t)bus, (uint8_t)slot,
                                               (uint8_t)function, 8);
                if ((class_info >> 24) != class_code ||
                    ((class_info >> 16) & 0xffU) != subclass)
                    continue;
                out->bus = (uint8_t)bus;
                out->slot = (uint8_t)slot;
                out->function = (uint8_t)function;
                pci_decode_id(id, out);
                out->prog_if = (uint8_t)((class_info >> 8) & 0xffU);
                out->subclass = (uint8_t)((class_info >> 16) & 0xffU);
                out->class_code = (uint8_t)(class_info >> 24);
                return 0;
            }
        }
    }
    return -2;
}

int pci_find_device(uint16_t vendor_id, uint16_t device_id, pci_device_t* out) {
    uint32_t bus;
    uint32_t slot;
    uint32_t function;
    if (!out) return -1;
    for (bus = 0; bus < 256U; ++bus) {
        for (slot = 0; slot < 32U; ++slot) {
            for (function = 0; function < 8U; ++function) {
                uint32_t id = pci_config_read32((uint8_t)bus, (uint8_t)slot,
                                                (uint8_t)function, 0);
                if ((id & 0xffffU) == 0xffffU) continue;
                if ((id & 0xffffU) == vendor_id && ((id >> 16) & 0xffffU) == device_id) {
                    uint32_t class_info = pci_config_read32((uint8_t)bus, (uint8_t)slot,
                                                           (uint8_t)function, 8);
                    out->bus = (uint8_t)bus;
                    out->slot = (uint8_t)slot;
                    out->function = (uint8_t)function;
                    pci_decode_id(id, out);
                    out->prog_if = (uint8_t)((class_info >> 8) & 0xffU);
                    out->subclass = (uint8_t)((class_info >> 16) & 0xffU);
                    out->class_code = (uint8_t)(class_info >> 24);
                    return 0;
                }
            }
        }
    }
    return -2;
}
