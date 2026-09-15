#include "elf.h"
#include "mem/pmm.h"
#include "mem/vmm.h"
#include "mem/string.h"
#include "kernel/mem/vmm.h"
#include "kernel/mem/pmm.h"
#include "kernel/mem/string.h"

extern void print_string_serial(const char* str);
extern void print_hex_serial(uint32_t n);

int elf_validate(uint8_t* elf_data) {
    if (!elf_data) return 0;
    elf32_ehdr_t* header = (elf32_ehdr_t*)elf_data;
    if (header->e_ident[0] != 0x7F || header->e_ident[1] != 'E' || header->e_ident[2] != 'L' || header->e_ident[3] != 'F') {
        return 0;
    }
    if (header->e_type != ET_EXEC) return 0;
    return 1;
}

uint32_t elf_load(uint8_t* file_data, vmm_directory_t* vmm_dir) {
    if (!elf_validate(file_data)) {
        print_string_serial("ERROR: Invalid ELF file.\n");
        return 0;
    }

    elf32_ehdr_t* header = (elf32_ehdr_t*)file_data;
    elf32_phdr_t* pheaders = (elf32_phdr_t*)(file_data + header->e_phoff);

    print_string_serial("ELF Loader: Starting...\n");

    // It is safer to modify a non-active page directory by temporarily mapping
    // pages into the kernel's address space. However, the current design
    // switches to the new directory first. The code below assumes this design
    // but is now corrected for BSS handling and permissions.

    for (int i = 0; i < header->e_phnum; i++) {
        if (pheaders[i].p_type == PT_LOAD) {
            uint32_t vaddr = pheaders[i].p_vaddr;
            uint32_t filesz = pheaders[i].p_filesz;
            uint32_t memsz = pheaders[i].p_memsz;
            uint32_t flags = pheaders[i].p_flags;
            uint8_t* segment_data = file_data + pheaders[i].p_offset;

            // Determine page flags
            uint32_t page_flags = PAGE_PRESENT | PAGE_USER;
            if (flags & PF_W) {
                page_flags |= PAGE_WRITE;
            }

            print_string_serial("  - Segment: vaddr=0x");
            print_hex_serial(vaddr);
            print_string_serial(", memsz=0x");
            print_hex_serial(memsz);
            print_string_serial(", perms=");
            print_string_serial((flags & PF_W) ? "RW" : "R-");
            print_string_serial("\n");

            // Map all pages for this segment
            uint32_t start_addr = vaddr & ~0xFFF; // Page-align start address
            uint32_t end_addr = (vaddr + memsz + 0xFFF) & ~0xFFF;
            for (uint32_t page_addr = start_addr; page_addr < end_addr; page_addr += PAGE_SIZE) {
                void* phys_page = pmm_alloc_page();
                if (!phys_page) {
                    print_string_serial("ERROR: pmm_alloc_page failed for ELF segment.\n");
                    /* Le chargeur rend l’échec à l’appelant. Après restauration du
                     * répertoire actif, task_destroy_user_vmm() détruit le VMM partiel,
                     * restitue les pages utilisateur et les tables privées au PMM. */
                    return 0;
                }
                if (vmm_map_page_in_directory(vmm_dir, phys_page, (void*)page_addr, page_flags) != 0) {
                    pmm_free_page(phys_page);
                    print_string_serial("ERROR: Could not map ELF segment page.\n");
                    return 0;
                }
            }

            // Copy data from file (filesz)
            if (filesz > 0) {
                memcpy((void*)vaddr, segment_data, filesz);
            }

            // Zero-fill the .bss section part of the segment (memsz - filesz)
            if (memsz > filesz) {
                memset((void*)(vaddr + filesz), 0, memsz - filesz);
            }
        }
    }

    print_string_serial("ELF loading complete. Entry point: 0x");
    print_hex_serial(header->e_entry);
    print_string_serial("\n");

    return header->e_entry;
}
