#ifndef MULTIBOOT_H
#define MULTIBOOT_H

#include <stdint.h>

// Magic number pour Multiboot
#define MULTIBOOT_MAGIC 0x2BADB002

// Flags pour l'en-tête Multiboot
#define MULTIBOOT_FLAG_MEM     0x001
#define MULTIBOOT_FLAG_DEVICE  0x002
#define MULTIBOOT_FLAG_CMDLINE 0x004
#define MULTIBOOT_FLAG_MODS    0x008
#define MULTIBOOT_FLAG_AOUT    0x010
#define MULTIBOOT_FLAG_ELF     0x020
#define MULTIBOOT_FLAG_MMAP    0x040
#define MULTIBOOT_FLAG_CONFIG  0x080
#define MULTIBOOT_FLAG_LOADER  0x100
#define MULTIBOOT_FLAG_APM     0x200
#define MULTIBOOT_FLAG_VBE     0x400

// Structure d'information Multiboot
typedef struct {
    uint32_t flags;
    uint32_t mem_lower;
    uint32_t mem_upper;
    uint32_t boot_device;
    uint32_t cmdline;
    uint32_t mods_count;
    uint32_t mods_addr;
    uint32_t num;
    uint32_t size;
    uint32_t addr;
    uint32_t shndx;
    uint32_t mmap_length;
    uint32_t mmap_addr;
    uint32_t drives_length;
    uint32_t drives_addr;
    uint32_t config_table;
    uint32_t boot_loader_name;
    uint32_t apm_table;
    uint32_t vbe_control_info;
    uint32_t vbe_mode_info;
    uint16_t vbe_mode;
    uint16_t vbe_interface_seg;
    uint16_t vbe_interface_off;
    uint16_t vbe_interface_len;
} __attribute__((packed)) multiboot_info_t;

// Structure pour un module Multiboot
typedef struct {
    uint32_t mod_start;  // Adresse de début du module
    uint32_t mod_end;    // Adresse de fin du module
    uint32_t cmdline;    // Ligne de commande du module
    uint32_t pad;        // Padding
} __attribute__((packed)) multiboot_module_t;

// Structure pour une entrée de memory map
typedef struct {
    uint32_t size;
    uint64_t addr;
    uint64_t len;
    uint32_t type;
} __attribute__((packed)) multiboot_memory_map_t;

// Types de mémoire
#define MULTIBOOT_MEMORY_AVAILABLE        1
#define MULTIBOOT_MEMORY_RESERVED         2
#define MULTIBOOT_MEMORY_ACPI_RECLAIMABLE 3
#define MULTIBOOT_MEMORY_NVS              4
#define MULTIBOOT_MEMORY_BADRAM           5

// Fonctions utilitaires pour Multiboot
uint32_t multiboot_get_memory_size(multiboot_info_t* mbi);
multiboot_module_t* multiboot_get_module(multiboot_info_t* mbi, uint32_t index);
uint32_t multiboot_get_module_count(multiboot_info_t* mbi);
void multiboot_print_info(multiboot_info_t* mbi);

#endif

