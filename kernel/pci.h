#ifndef MOHHDY_PCI_H
#define MOHHDY_PCI_H

#include <stdint.h>

typedef struct {
    uint8_t bus;
    uint8_t slot;
    uint8_t function;
    uint16_t vendor_id;
    uint16_t device_id;
    uint8_t class_code;
    uint8_t subclass;
    uint8_t prog_if;
} pci_device_t;

uint32_t pci_config_address(uint8_t bus, uint8_t slot, uint8_t function,
                             uint8_t offset);
void pci_decode_id(uint32_t value, pci_device_t* device);
uint32_t pci_config_read32(uint8_t bus, uint8_t slot, uint8_t function,
                           uint8_t offset);
int pci_find_class(uint8_t class_code, uint8_t subclass, pci_device_t* out);
int pci_find_device(uint16_t vendor_id, uint16_t device_id, pci_device_t* out);

#endif
