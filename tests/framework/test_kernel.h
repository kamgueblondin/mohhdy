/* test_kernel.h - Extensions Unity pour les tests kernel MOHHDY */

#ifndef TEST_KERNEL_H
#define TEST_KERNEL_H

#include "unity.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// Configuration spécifique aux tests kernel
#define TEST_KERNEL_STACK_SIZE 8192
#define TEST_MAX_PAGES 1000
#define TEST_MAX_TASKS 32

// Types de tests kernel
typedef enum {
    TEST_RING_0,    // Test en mode kernel
    TEST_RING_3,    // Test en mode utilisateur
    TEST_MIXED      // Test nécessitant les deux modes
} test_ring_mode_t;

// Structure pour le contexte de test kernel
typedef struct {
    test_ring_mode_t ring_mode;
    uint32_t kernel_stack;
    uint32_t user_stack;
    uint64_t original_cr3;  // 64-bit pour compatibilité
    int interrupts_enabled;
    char test_heap[256 * 1024]; // Increased heap size for performance and stress testing
    size_t heap_used;
} test_kernel_context_t;

// Variables globales du contexte de test
extern test_kernel_context_t test_context;

// === INITIALISATION ET CONFIGURATION ===

// Initialiser le contexte de test kernel
void test_kernel_init(void);

// Configurer le mode de test
void test_kernel_set_mode(test_ring_mode_t mode);

// Nettoyer après les tests
void test_kernel_cleanup(void);

// Sauvegarder l'état actuel du système
void test_kernel_save_state(void);

// Restaurer l'état du système
void test_kernel_restore_state(void);

// === GESTION MÉMOIRE POUR TESTS ===

// Allocateur simple pour les tests
void* test_malloc(size_t size);
void test_free(void* ptr);
void test_heap_reset(void);

// Vérification de l'intégrité du heap de test
int test_heap_check_integrity(void);

// === MOCKS HARDWARE ===

// Mock du contrôleur PIC
typedef struct {
    uint8_t master_mask;
    uint8_t slave_mask;
    uint8_t master_isr;
    uint8_t slave_isr;
    int interrupts_enabled;
} mock_pic_state_t;

extern mock_pic_state_t mock_pic;

void mock_pic_init(void);
void mock_pic_set_mask(uint8_t mask);
void mock_pic_send_interrupt(uint8_t irq);
void mock_pic_clear_interrupt(uint8_t irq);

// Mock du timer
typedef struct {
    uint32_t frequency;
    uint32_t tick_count;
    int enabled;
} mock_timer_state_t;

extern mock_timer_state_t mock_timer;

void mock_timer_init(void);
void mock_timer_set_frequency(uint32_t freq);
void mock_timer_tick(void);
uint32_t mock_timer_get_ticks(void);

// Mock du clavier
typedef struct {
    uint8_t buffer[256];
    int buffer_head;
    int buffer_tail;
    uint8_t status_register;
    uint8_t data_register;
} mock_keyboard_state_t;

extern mock_keyboard_state_t mock_keyboard;

void mock_keyboard_init(void);
void mock_keyboard_press_key(uint8_t scancode);
void mock_keyboard_release_key(uint8_t scancode);
void mock_keyboard_type_string(const char* str);
uint8_t mock_keyboard_read_data(void);
uint8_t mock_keyboard_read_status(void);

// === UTILITAIRES MÉMOIRE ===

// Pattern pour vérifier l'intégrité mémoire
#define TEST_MEMORY_PATTERN_1 0xDEADBEEF
#define TEST_MEMORY_PATTERN_2 0xCAFEBABE
#define TEST_MEMORY_PATTERN_3 0x12345678

void test_fill_memory_pattern(void* ptr, size_t size, uint32_t pattern);
int test_verify_memory_pattern(const void* ptr, size_t size, uint32_t pattern);

// Test de corruption mémoire
void test_corrupt_memory(void* ptr, size_t size);
int test_detect_memory_corruption(const void* ptr, size_t size, uint32_t original_pattern);

// === UTILITAIRES TÂCHES ===

// Structure pour simuler une tâche de test
typedef struct test_task {
    int id;
    int state;
    void (*test_function)(void);
    char name[64];
    uint32_t stack_ptr;
    uint32_t priority;
    struct test_task* next;
} test_task_t;

test_task_t* test_create_task(void (*func)(void), const char* name, uint32_t priority);
void test_destroy_task(test_task_t* task);
void test_run_task(test_task_t* task);

// === MACROS DE TEST SPÉCIALISÉES ===

// Test spécifique à la gestion mémoire
#define TEST_ASSERT_VALID_KERNEL_PTR(ptr) \
    do { \
        TEST_ASSERT_NOT_NULL(ptr); \
        TEST_ASSERT_KERNEL_ADDRESS((uint32_t)(ptr)); \
        TEST_ASSERT((uint32_t)(ptr) % sizeof(void*) == 0); \
    } while(0)

#define TEST_ASSERT_VALID_USER_PTR(ptr) \
    do { \
        TEST_ASSERT_NOT_NULL(ptr); \
        TEST_ASSERT_USER_ADDRESS((uint32_t)(ptr)); \
        TEST_ASSERT((uint32_t)(ptr) % sizeof(void*) == 0); \
    } while(0)

// Test de l'alignement mémoire
#define TEST_ASSERT_ALIGNED(ptr, alignment) \
    do { \
        TEST_ASSERT(((uint32_t)(ptr) % (alignment)) == 0); \
    } while(0)

// Test de plage d'adresses
#define TEST_ASSERT_ADDRESS_RANGE(ptr, start, end) \
    do { \
        uint32_t addr = (uint32_t)(ptr); \
        TEST_ASSERT(addr >= (uint32_t)(start)); \
        TEST_ASSERT(addr < (uint32_t)(end)); \
    } while(0)

// Test d'intégrité de structure avec magic numbers
#define TEST_ASSERT_STRUCTURE_INTEGRITY(ptr, type, magic_field, expected_magic) \
    do { \
        TEST_ASSERT_NOT_NULL(ptr); \
        type* struct_ptr = (type*)(ptr); \
        TEST_ASSERT_EQUAL(expected_magic, struct_ptr->magic_field); \
    } while(0)

// Test de performance avec cycles CPU
#define TEST_ASSERT_CYCLES_LESS_THAN(threshold, code_block) \
    do { \
        uint64_t start_cycles = rdtsc(); \
        code_block; \
        uint64_t end_cycles = rdtsc(); \
        uint64_t elapsed = end_cycles - start_cycles; \
        if (elapsed >= (threshold)) { \
            unity_test_fail("Execution exceeded cycle threshold", __FILE__, __LINE__); \
            return; \
        } \
    } while(0)

// Test avec timeout
#define TEST_ASSERT_WITH_TIMEOUT(condition, timeout_ms) \
    do { \
        int timeout_counter = timeout_ms * 1000; \
        while (!(condition) && timeout_counter > 0) { \
            timeout_counter--; \
        } \
        TEST_ASSERT(condition); \
    } while(0)

// Test d'interruption
#define TEST_ASSERT_INTERRUPT_OCCURRED(irq) \
    do { \
        TEST_ASSERT(mock_pic.master_isr & (1 << (irq))); \
    } while(0)

#define TEST_ASSERT_NO_INTERRUPT_OCCURRED(irq) \
    do { \
        TEST_ASSERT(!(mock_pic.master_isr & (1 << (irq)))); \
    } while(0)

// === UTILITAIRES DE DEBUG ===

// Afficher l'état du système pour debug
void test_debug_print_system_state(void);

// Afficher les statistiques mémoire
void test_debug_print_memory_stats(void);

// Afficher l'état des tâches
void test_debug_print_task_stats(void);

// Tracer l'exécution d'une fonction
#define TEST_TRACE_FUNCTION(func) \
    do { \
        unity_print_string("TRACE: Entering " #func "\n"); \
        func; \
        unity_print_string("TRACE: Exiting " #func "\n"); \
    } while(0)

// === BENCHMARKING ===

typedef struct {
    const char* name;
    uint64_t start_cycles;
    uint64_t end_cycles;
    uint64_t elapsed_cycles;
    uint32_t num_calls;  // Temporarily renamed from iterations
} test_benchmark_t;

void test_benchmark_start(test_benchmark_t* bench, const char* name);
void test_benchmark_end(test_benchmark_t* bench);
void test_benchmark_print_results(const test_benchmark_t* bench);

// Macro pour benchmarker facilement
#define TEST_BENCHMARK(name, iterations, code_block) \
    do { \
        test_benchmark_t bench; \
        uint32_t iter_count = (uint32_t)(iterations); \
        test_benchmark_start(&bench, name); \
        bench.num_calls = iter_count; \
        for (uint32_t i = 0; i < iter_count; i++) \
            code_block \
        test_benchmark_end(&bench); \
        test_benchmark_print_results(&bench); \
    } while(0)

// === SIMULATION D'ERREURS ===

// Simuler une page fault
void test_simulate_page_fault(uint32_t fault_address);

// Simuler une exception
void test_simulate_exception(uint8_t exception_number);

// Simuler un out-of-memory
void test_simulate_out_of_memory(void);

// === VERIFICATION DE SECURITE ===

// Tester l'isolation Ring 0/3
int test_verify_ring_isolation(void);

// Tester la protection mémoire
int test_verify_memory_protection(void);

// Tester les validations de syscall
int test_verify_syscall_validation(void);

#ifdef __cplusplus
}
#endif

#endif // TEST_KERNEL_H
