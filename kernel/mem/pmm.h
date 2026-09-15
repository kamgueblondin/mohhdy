#ifndef PMM_H
#define PMM_H

#include <stdint.h>

#define PAGE_SIZE 4096

// Fonctions publiques du Physical Memory Manager
void pmm_init(uint32_t memory_size, uint32_t multiboot_addr);
void* pmm_alloc_page();
void* pmm_alloc_pages(uint32_t page_count);
void pmm_free_page(void* page);
void pmm_free_pages(void* page, uint32_t page_count);
uint32_t pmm_get_total_pages();
uint32_t pmm_get_used_pages();
uint32_t pmm_get_free_pages();

// Fonctions utilitaires internes
void pmm_set_page(uint32_t page_num);
void pmm_clear_page(uint32_t page_num);
int pmm_test_page(uint32_t page_num);
uint32_t pmm_find_free_page();

#endif

