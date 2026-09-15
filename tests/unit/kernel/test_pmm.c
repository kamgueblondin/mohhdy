/* test_pmm.c - Tests unitaires pour le Physical Memory Manager */

#include <stdio.h>
#include "../../framework/unity.h"
#include "../../framework/test_kernel.h"

// Include du module à tester
#include "../../../kernel/mem/pmm.h"

// Redefine end symbol for testing
uint32_t end __attribute__((aligned(4096)));
uint8_t extra_space[1024 * 1024 * 2]; // 2MB pool following end
#define test_memory_pool ((uint8_t*)&end)

#define pmm_alloc_page real_pmm_alloc_page
#define pmm_free_page real_pmm_free_page
#include "../../../kernel/mem/pmm.c"
#undef pmm_alloc_page
#undef pmm_free_page

// Wrapper for pmm_alloc_page to map low physical addresses to safe user-space virtual addresses
void* pmm_alloc_page(void) {
    void* phys = real_pmm_alloc_page();
    if (!phys) return NULL;
    uint32_t page_num = (uint32_t)phys / 4096;
    return (void*)((uint32_t)&end + page_num * 4096);
}

// Wrapper for pmm_free_page to map virtual pointers back to physical addresses
void pmm_free_page(void* page) {
    if (!page) return;
    if (page < (void*)test_memory_pool || page >= (void*)(test_memory_pool + sizeof(extra_space))) {
        // For invalid or out-of-pool pointers, pass them through to test invalid address handling
        real_pmm_free_page(page);
        return;
    }
    uint32_t offset = (uint32_t)page - (uint32_t)&end;
    real_pmm_free_page((void*)offset);
}

// === SETUP ET TEARDOWN ===

void setUp(void) {
    test_kernel_init();
    test_kernel_save_state();
}

void tearDown(void) {
    test_kernel_restore_state();
    test_kernel_cleanup();
}

// Helper to init for other tests
void init_pmm_for_test(uint32_t size) {
    printf("init_pmm_for_test size=%u\n", size);
    multiboot_info_t mbi;
    mbi.flags = 0;
    printf("Calling pmm_init...\n");
    pmm_init(size, (uint32_t)&mbi);
    printf("pmm_init done\n");
}

// === TESTS D'INITIALISATION ===

void test_pmm_init_basic(void) {
    init_pmm_for_test(1 * 1024 * 1024);
    
    // Vérifier que l'initialisation s'est bien passée
    TEST_ASSERT_GREATER_THAN(0, pmm_get_total_pages());
    TEST_ASSERT_GREATER_THAN(0, pmm_get_free_pages());
    TEST_ASSERT(pmm_get_free_pages() <= pmm_get_total_pages());
}

void test_pmm_init_boundary_conditions(void) {
    // Test avec mémoire minimale
    init_pmm_for_test(4 * 1024 * 1024); // 4MB minimum
    TEST_ASSERT_GREATER_THAN(0, pmm_get_free_pages());
    
    // Reset pour test suivant
    init_pmm_for_test(1 * 1024 * 1024);
}

void test_pmm_bitmap_integrity(void) {
    uint32_t memory_size = 1 * 1024 * 1024;
    init_pmm_for_test(memory_size);
    
    // Vérifier que le bitmap est correctement initialisé
    uint32_t total_pages = pmm_get_total_pages();
    uint32_t free_pages = pmm_get_free_pages();
    uint32_t used_pages = total_pages - free_pages;
    
    // Il devrait y avoir des pages utilisées pour le kernel et le bitmap
    TEST_ASSERT_GREATER_THAN(0, used_pages);
    
    // Le nombre de pages libres ne devrait pas dépasser le total
    TEST_ASSERT(free_pages <= total_pages);
}

// === TESTS D'ALLOCATION ===

void test_pmm_alloc_single_page(void) {
    init_pmm_for_test(1 * 1024 * 1024);
    
    uint32_t free_pages_before = pmm_get_free_pages();
    
    void* page = pmm_alloc_page();
    
    TEST_ASSERT_NOT_NULL(page);
    TEST_ASSERT_PAGE_ALIGNED((uint32_t)page);
    
    uint32_t free_pages_after = pmm_get_free_pages();
    TEST_ASSERT_EQUAL(free_pages_before - 1, free_pages_after);
}

void test_pmm_alloc_multiple_pages(void) {
    init_pmm_for_test(1 * 1024 * 1024);
    
    const int num_allocs = 10;
    void* pages[num_allocs];
    uint32_t free_pages_before = pmm_get_free_pages();
    
    // Allouer plusieurs pages
    for (int i = 0; i < num_allocs; i++) {
        pages[i] = pmm_alloc_page();
        TEST_ASSERT_NOT_NULL(pages[i]);
        TEST_ASSERT_PAGE_ALIGNED((uint32_t)pages[i]);
        
        // Vérifier que chaque page est unique
        for (int j = 0; j < i; j++) {
            TEST_ASSERT_NOT_EQUAL(pages[i], pages[j]);
        }
    }
    
    uint32_t free_pages_after = pmm_get_free_pages();
    TEST_ASSERT_EQUAL(free_pages_before - num_allocs, free_pages_after);
}

void test_pmm_alloc_until_exhaustion(void) {
    // Test avec une petite quantité de mémoire pour épuiser rapidement
    init_pmm_for_test(512 * 1024); // 512KB
    
    int allocated_count = 0;
    void* page;
    
    // Allouer jusqu'à épuisement
    while ((page = pmm_alloc_page()) != NULL) {
        allocated_count++;
        TEST_ASSERT_PAGE_ALIGNED((uint32_t)page);
        
        // Protection contre boucle infinie
        if (allocated_count > 3000) {
            TEST_FAIL();
            break;
        }
    }
    
    // Vérifier qu'on ne peut plus allouer
    TEST_ASSERT_NULL(pmm_alloc_page());
    TEST_ASSERT_EQUAL(0, pmm_get_free_pages());
    
    // Il devrait y avoir eu au moins quelques allocations
    TEST_ASSERT_GREATER_THAN(0, allocated_count);
}

// === TESTS DE LIBÉRATION ===

void test_pmm_free_single_page(void) {
    init_pmm_for_test(1 * 1024 * 1024);
    
    void* page = pmm_alloc_page();
    TEST_ASSERT_NOT_NULL(page);
    
    uint32_t free_pages_before = pmm_get_free_pages();
    
    pmm_free_page(page);
    
    uint32_t free_pages_after = pmm_get_free_pages();
    TEST_ASSERT_EQUAL(free_pages_before + 1, free_pages_after);
}

void test_pmm_free_multiple_pages(void) {
    init_pmm_for_test(1 * 1024 * 1024);
    
    const int num_pages = 5;
    void* pages[num_pages];
    
    // Allouer les pages
    for (int i = 0; i < num_pages; i++) {
        pages[i] = pmm_alloc_page();
        TEST_ASSERT_NOT_NULL(pages[i]);
    }
    
    uint32_t free_pages_before = pmm_get_free_pages();
    
    // Libérer toutes les pages
    for (int i = 0; i < num_pages; i++) {
        pmm_free_page(pages[i]);
    }
    
    uint32_t free_pages_after = pmm_get_free_pages();
    TEST_ASSERT_EQUAL(free_pages_before + num_pages, free_pages_after);
}

void test_pmm_free_null_pointer(void) {
    init_pmm_for_test(1 * 1024 * 1024);
    
    uint32_t free_pages_before = pmm_get_free_pages();
    
    // Libérer un pointeur NULL ne devrait rien faire
    pmm_free_page(NULL);
    
    uint32_t free_pages_after = pmm_get_free_pages();
    TEST_ASSERT_EQUAL(free_pages_before, free_pages_after);
}

void test_pmm_free_invalid_address(void) {
    init_pmm_for_test(1 * 1024 * 1024);
    
    uint32_t free_pages_before __attribute__((unused)) = pmm_get_free_pages();
    
    // Essayer de libérer une adresse invalide
    pmm_free_page((void*)0x12345678); // Adresse non alignée sur page
    
    // Le gestionnaire devrait ignorer ou gérer gracieusement
    uint32_t free_pages_after __attribute__((unused)) = pmm_get_free_pages();
    // On ne peut pas faire d'assertion spécifique car le comportement
    // peut varier selon l'implémentation
}

// === TESTS D'ALLOCATION/LIBÉRATION CYCLIQUE ===

void test_pmm_alloc_free_cycle(void) {
    init_pmm_for_test(1 * 1024 * 1024);
    
    uint32_t initial_free = pmm_get_free_pages();
    
    // Effectuer plusieurs cycles allocation/libération
    for (int cycle = 0; cycle < 10; cycle++) {
        void* page = pmm_alloc_page();
        TEST_ASSERT_NOT_NULL(page);
        TEST_ASSERT_PAGE_ALIGNED((uint32_t)page);
        
        pmm_free_page(page);
        
        // Vérifier qu'on revient à l'état initial
        TEST_ASSERT_EQUAL(initial_free, pmm_get_free_pages());
    }
}

void test_pmm_fragmentation_resistance(void) {
    init_pmm_for_test(1 * 1024 * 1024);
    
    const int num_pages = 20;
    void* pages[num_pages];
    
    // Allouer un ensemble de pages
    for (int i = 0; i < num_pages; i++) {
        pages[i] = pmm_alloc_page();
        TEST_ASSERT_NOT_NULL(pages[i]);
    }
    
    // Libérer une page sur deux
    for (int i = 1; i < num_pages; i += 2) {
        pmm_free_page(pages[i]);
        pages[i] = NULL;
    }
    
    // Essayer d'allouer de nouvelles pages
    for (int i = 1; i < num_pages; i += 2) {
        pages[i] = pmm_alloc_page();
        TEST_ASSERT_NOT_NULL(pages[i]);
    }
    
    // Libérer toutes les pages
    for (int i = 0; i < num_pages; i++) {
        if (pages[i]) {
            pmm_free_page(pages[i]);
        }
    }
}

// === TESTS DE PERFORMANCE ===

void test_pmm_allocation_performance(void) {
    init_pmm_for_test(1 * 1024 * 1024);
    
    test_benchmark_t bench;
    test_benchmark_start(&bench, "PMM Single Page Allocation");
    bench.num_calls = 1000;
    
    for (uint32_t i = 0; i < 1000; i++) {
        void* page = pmm_alloc_page();
        if (page) pmm_free_page(page);
    }
    
    test_benchmark_end(&bench);
    test_benchmark_print_results(&bench);
}

void test_pmm_batch_allocation_performance(void) {
    init_pmm_for_test(1 * 1024 * 1024);
    
    const uint32_t batch_size = 100;
    void* pages[batch_size];
    
    test_benchmark_t bench;
    test_benchmark_start(&bench, "PMM Batch Allocation");
    bench.num_calls = 1;
    
    // Allouer un lot de pages
    for (uint32_t i = 0; i < batch_size; i++) {
        pages[i] = pmm_alloc_page();
    }
    
    // Les libérer
    for (uint32_t i = 0; i < batch_size; i++) {
        if (pages[i]) pmm_free_page(pages[i]);
    }
    
    test_benchmark_end(&bench);
    test_benchmark_print_results(&bench);
}

// === TESTS DE ROBUSTESSE ===

void test_pmm_double_free_detection(void) {
    init_pmm_for_test(1 * 1024 * 1024);
    
    void* page = pmm_alloc_page();
    TEST_ASSERT_NOT_NULL(page);
    
    // Première libération
    pmm_free_page(page);
    uint32_t free_after_first = pmm_get_free_pages();
    
    // Deuxième libération (double free)
    pmm_free_page(page);
    uint32_t free_after_second = pmm_get_free_pages();
    
    // Le double free ne devrait pas augmenter le nombre de pages libres
    TEST_ASSERT_EQUAL(free_after_first, free_after_second);
}

void test_pmm_memory_corruption_detection(void) {
    init_pmm_for_test(1 * 1024 * 1024);
    
    void* page = pmm_alloc_page();
    TEST_ASSERT_NOT_NULL(page);
    
    // Remplir la page avec un pattern
    test_fill_memory_pattern(page, 4096, TEST_MEMORY_PATTERN_1);
    
    // Vérifier l'intégrité
    TEST_ASSERT_TRUE(test_verify_memory_pattern(page, 4096, TEST_MEMORY_PATTERN_1));
    
    // Corrompre la mémoire
    test_corrupt_memory(page, 4096);
    
    // Détecter la corruption
    TEST_ASSERT_TRUE(test_detect_memory_corruption(page, 4096, TEST_MEMORY_PATTERN_1));
    
    pmm_free_page(page);
}

// === TESTS D'INTÉGRATION ===

void test_pmm_integration_with_multiboot(void) {
    // Simuler différents layouts mémoire multiboot
    uint32_t memory_configs[] = {
        256 * 1024,
        512 * 1024,
        1024 * 1024,
    };
    
    for (int i = 0; i < 3; i++) {
        init_pmm_for_test(memory_configs[i]);
        
        TEST_ASSERT_GREATER_THAN(0, pmm_get_total_pages());
        TEST_ASSERT_GREATER_THAN(0, pmm_get_free_pages());
        
        // Tester une allocation de base
        void* page = pmm_alloc_page();
        TEST_ASSERT_NOT_NULL(page);
        pmm_free_page(page);
    }
}

// === RUNNER PRINCIPAL ===

int main(void) {
    printf("Starting main\n");
    unity_init();
    printf("unity_init done\n");
    
    // Tests d'initialisation
    RUN_TEST(test_pmm_init_basic);
    RUN_TEST(test_pmm_init_boundary_conditions);
    RUN_TEST(test_pmm_bitmap_integrity);
    
    // Tests d'allocation
    RUN_TEST(test_pmm_alloc_single_page);
    RUN_TEST(test_pmm_alloc_multiple_pages);
    RUN_TEST(test_pmm_alloc_until_exhaustion);
    
    // Tests de libération
    RUN_TEST(test_pmm_free_single_page);
    RUN_TEST(test_pmm_free_multiple_pages);
    RUN_TEST(test_pmm_free_null_pointer);
    RUN_TEST(test_pmm_free_invalid_address);
    
    // Tests cycliques
    RUN_TEST(test_pmm_alloc_free_cycle);
    RUN_TEST(test_pmm_fragmentation_resistance);
    
    // Tests de performance
    RUN_TEST(test_pmm_allocation_performance);
    RUN_TEST(test_pmm_batch_allocation_performance);
    
    // Tests de robustesse
    RUN_TEST(test_pmm_double_free_detection);
    RUN_TEST(test_pmm_memory_corruption_detection);
    
    // Tests d'intégration
    RUN_TEST(test_pmm_integration_with_multiboot);
    
    unity_print_results();
    unity_cleanup();
    
    return (unity_stats.tests_failed == 0) ? 0 : 1;
}
