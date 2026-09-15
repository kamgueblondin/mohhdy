#include "vmm.h"
#include "pmm.h"
#include "heap.h"
#include "string.h"

// Externe pour le logging
extern void print_string_serial(const char* str);

// Variables globales pour la gestion de la mémoire virtuelle
vmm_directory_t *kernel_directory = 0;
vmm_directory_t *current_directory = 0;

/*
 * Le premier moteur GPT-2 local charge ses poids comme module Multiboot.
 * Le noyau reste en mode 32 bits : le plafond identite est donc borne a 1 Gio
 * pour garder un chemin simple et verifiable avant le portage PAE/x86_64.
 */
#define VMM_IDENTITY_LIMIT (1024U * 1024U * 1024U)


// Initialise le gestionnaire de mémoire virtuelle
void vmm_init() {
    // Alloue la structure de gestion (ne peut pas utiliser kmalloc ici)
    kernel_directory = (vmm_directory_t*)pmm_alloc_page();
    if (!kernel_directory) return; // Erreur critique
    memset(kernel_directory, 0, sizeof(vmm_directory_t));

    // Alloue la table des tables de pages pour le noyau
    kernel_directory->tables = (page_table_t**)pmm_alloc_page();
    if (!kernel_directory->tables) return; // Erreur critique
    memset(kernel_directory->tables, 0, sizeof(page_table_t*) * 1024);

    // Alloue le répertoire de pages physique (aligné sur 4k)
    kernel_directory->physical_dir = (page_directory_t*)pmm_alloc_page();
    if (!kernel_directory->physical_dir) return; // Erreur critique
    memset(kernel_directory->physical_dir, 0, sizeof(page_directory_t));
    kernel_directory->physical_addr = (uint32_t)kernel_directory->physical_dir;

    // Initialise les entrées du répertoire physique
    for (int i = 0; i < 1024; i++) {
        kernel_directory->physical_dir->tablesPhysical[i] = 0x00000002; // Non présent, R/W
    }

    /*
     * Mapping identite pour le noyau, l'initrd et les modules de modele.
     * La version precedente ne mappait que 4 Mio, insuffisant des qu'un
     * checkpoint est place au-dessus de cette zone par Multiboot/GRUB.
     */
    uint32_t total_frames = pmm_get_total_pages();
    uint32_t limit_frames = VMM_IDENTITY_LIMIT / PAGE_SIZE;
    if (total_frames < limit_frames) limit_frames = total_frames;
    uint32_t table_count = (limit_frames + ENTRIES_PER_TABLE - 1) / ENTRIES_PER_TABLE;

    for (uint32_t table_index = 0; table_index < table_count; table_index++) {
        page_table_t* pt = (page_table_t*)pmm_alloc_page();
        if (!pt) return; // Erreur critique
        memset(pt, 0, sizeof(page_table_t));

        for (uint32_t page_index = 0; page_index < ENTRIES_PER_TABLE; page_index++) {
            uint32_t frame = table_index * ENTRIES_PER_TABLE + page_index;
            if (frame >= limit_frames) break;
            pt->pages[page_index].present = 1;
            pt->pages[page_index].rw = 1;
            pt->pages[page_index].user = 0;
            pt->pages[page_index].frame = frame;
        }

        kernel_directory->tables[table_index] = pt;
        kernel_directory->physical_dir->tablesPhysical[table_index] = (uint32_t)pt | 3;
    }

    // Charger le nouveau répertoire de pages et activer
    load_page_directory(kernel_directory->physical_addr);
    enable_paging();

    current_directory = kernel_directory;

    // Initialisation du tas (heap) maintenant que le VMM est prêt
    init_heap();
}

// Change le répertoire de pages actuel
void vmm_switch_page_directory(uint32_t physical_addr) {
    load_page_directory(physical_addr);
}

// Obtient une page depuis une adresse virtuelle
page_t *vmm_get_page(uint32_t address, int make, vmm_directory_t *dir) {
    address /= 0x1000;
    uint32_t table_idx = address / 1024;

    if (dir->tables[table_idx]) {
        return &dir->tables[table_idx]->pages[address % 1024];
    } else if (make) {
        page_table_t* new_table = (page_table_t*)pmm_alloc_page();
        if (!new_table) return 0;
        memset(new_table, 0, sizeof(page_table_t));
        
        dir->tables[table_idx] = new_table;
        dir->physical_dir->tablesPhysical[table_idx] = (uint32_t)new_table | 0x7; // P, RW, US
        
        return &dir->tables[table_idx]->pages[address % 1024];
    } else {
        return 0;
    }
}

static uint8_t vmm_table_is_private(const vmm_directory_t* dir, uint32_t table_index) {
    return (uint8_t)((dir->private_table_mask[table_index / 32U] >> (table_index % 32U)) & 1U);
}

static void vmm_table_mark_private(vmm_directory_t* dir, uint32_t table_index) {
    dir->private_table_mask[table_index / 32U] |= (uint32_t)1U << (table_index % 32U);
}

static int vmm_ensure_private_user_table(vmm_directory_t *dir, uint32_t table_index) {
    page_table_t* private_table;
    if (!dir || table_index >= ENTRIES_PER_TABLE) return -1;
    if (vmm_table_is_private(dir, table_index)) return 0;
    private_table = (page_table_t*)pmm_alloc_page();
    if (!private_table) return -2;
    if (dir->tables[table_index]) memcpy(private_table, dir->tables[table_index], sizeof(page_table_t));
    else memset(private_table, 0, sizeof(page_table_t));
    dir->tables[table_index] = private_table;
    dir->physical_dir->tablesPhysical[table_index] = (uint32_t)private_table | 0x7U;
    vmm_table_mark_private(dir, table_index);
    return 0;
}

/* Mappe une page physique à une adresse virtuelle dans un répertoire spécifique. */
int vmm_map_page_in_directory(vmm_directory_t *dir, void *physaddr, void *virtualaddr, uint32_t flags) {
    uint32_t table_index; page_t *page;
    if (!dir || !physaddr || !virtualaddr) return -1;
    table_index = ((uint32_t)virtualaddr / PAGE_SIZE) / ENTRIES_PER_TABLE;
    if ((flags & PAGE_USER) && dir != kernel_directory && vmm_ensure_private_user_table(dir, table_index) != 0) return -2;
    page = vmm_get_page((uint32_t)virtualaddr, 1, dir);
    if (!page) return -3;
    page->present = (flags & PAGE_PRESENT) ? 1 : 0;
    page->rw = (flags & PAGE_WRITE) ? 1 : 0;
    page->user = (flags & PAGE_USER) ? 1 : 0;
    page->frame = (uint32_t)physaddr / PAGE_SIZE;
    asm volatile ("invlpg (%0)" :: "r" (virtualaddr) : "memory");
    return 0;
}

int vmm_destroy_user_directory(vmm_directory_t *dir) {
    uint32_t table_index, page_index;
    if (!dir || dir == kernel_directory || dir == current_directory) return -1;
    if (!dir->static_storage) return -2;
    for (table_index = 0U; table_index < ENTRIES_PER_TABLE; table_index++) {
        page_table_t* table = dir->tables[table_index];
        if (!vmm_table_is_private(dir, table_index) || !table) continue;
        for (page_index = 0U; page_index < ENTRIES_PER_TABLE; page_index++) {
            page_t* page = &table->pages[page_index];
            if (page->present && page->user) pmm_free_page((void*)(page->frame * PAGE_SIZE));
        }
        pmm_free_page(table);
    }
    return 0;
}
