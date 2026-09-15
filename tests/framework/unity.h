/* unity.h - Framework de test Unity adapté pour MOHHDY
 * Version simplifiée et optimisée pour le développement kernel
 */

#ifndef UNITY_H
#define UNITY_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

// Configuration
#define UNITY_MAX_TEST_NAME_LENGTH 64
#define UNITY_MAX_MESSAGE_LENGTH 256
#define UNITY_MAX_TESTS 1000

// Types de résultats
typedef enum {
    UNITY_PASS = 0,
    UNITY_FAIL = 1,
    UNITY_IGNORE = 2
} unity_test_result_t;

// Structure pour les statistiques de tests
typedef struct {
    uint32_t tests_run;
    uint32_t tests_passed;
    uint32_t tests_failed;
    uint32_t tests_ignored;
    const char* current_test_name;
    const char* last_failure_msg;
    const char* last_failure_file;
    uint32_t last_failure_line;
} unity_stats_t;

// Variables globales
extern unity_stats_t unity_stats;

// Fonctions principales du framework
void unity_init(void);
void unity_cleanup(void);
void unity_print_results(void);

// Fonctions pour l'exécution des tests
void unity_run_test(void (*test_func)(void), const char* test_name, 
                   const char* file, uint32_t line);
void unity_test_fail(const char* message, const char* file, uint32_t line);
void unity_test_pass(void);
void unity_test_ignore(const char* message);

// Fonctions utilitaires
void unity_print_char(char c);
#define putc unity_putc_redirect
void unity_print_string(const char* str);
void unity_print_number(uint32_t number);
void unity_print_hex(uint32_t number);

// Comparaisons de base
int unity_compare_int(int expected, int actual);
int unity_compare_uint(uint32_t expected, uint32_t actual);
int unity_compare_ptr(const void* expected, const void* actual);
int unity_compare_memory(const void* expected, const void* actual, size_t len);
int unity_compare_string(const char* expected, const char* actual);

// Macros principales
#define RUN_TEST(test_func) \
    unity_run_test(test_func, #test_func, __FILE__, __LINE__)

#define TEST_ASSERT(condition) \
    do { \
        if (!(condition)) { \
            unity_test_fail("Assertion failed: " #condition, __FILE__, __LINE__); \
            return; \
        } \
    } while(0)

#define TEST_ASSERT_TRUE(condition) TEST_ASSERT(condition)
#define TEST_ASSERT_FALSE(condition) TEST_ASSERT(!(condition))

#define TEST_ASSERT_EQUAL(expected, actual) \
    do { \
        if (!unity_compare_int((int)(expected), (int)(actual))) { \
            unity_test_fail("Expected equal values", __FILE__, __LINE__); \
            return; \
        } \
    } while(0)

#define TEST_ASSERT_NOT_EQUAL(expected, actual) \
    do { \
        if (unity_compare_int((int)(expected), (int)(actual))) { \
            unity_test_fail("Expected different values", __FILE__, __LINE__); \
            return; \
        } \
    } while(0)

#define TEST_ASSERT_EQUAL_PTR(expected, actual) \
    do { \
        if (!unity_compare_ptr(expected, actual)) { \
            unity_test_fail("Expected equal pointers", __FILE__, __LINE__); \
            return; \
        } \
    } while(0)

#define TEST_ASSERT_NULL(ptr) \
    do { \
        if ((ptr) != NULL) { \
            unity_test_fail("Expected NULL pointer", __FILE__, __LINE__); \
            return; \
        } \
    } while(0)

#define TEST_ASSERT_NOT_NULL(ptr) \
    do { \
        if ((ptr) == NULL) { \
            unity_test_fail("Expected non-NULL pointer", __FILE__, __LINE__); \
            return; \
        } \
    } while(0)

#define TEST_ASSERT_EQUAL_MEMORY(expected, actual, len) \
    do { \
        if (!unity_compare_memory(expected, actual, len)) { \
            unity_test_fail("Memory comparison failed", __FILE__, __LINE__); \
            return; \
        } \
    } while(0)

#define TEST_ASSERT_EQUAL_STRING(expected, actual) \
    do { \
        if (!unity_compare_string(expected, actual)) { \
            unity_test_fail("String comparison failed", __FILE__, __LINE__); \
            return; \
        } \
    } while(0)

#define TEST_ASSERT_GREATER_THAN(threshold, actual) \
    do { \
        if (!((actual) > (threshold))) { \
            unity_test_fail("Expected greater value", __FILE__, __LINE__); \
            return; \
        } \
    } while(0)

#define TEST_ASSERT_LESS_THAN(threshold, actual) \
    do { \
        if (!((actual) < (threshold))) { \
            unity_test_fail("Expected smaller value", __FILE__, __LINE__); \
            return; \
        } \
    } while(0)

// Macros spécialisées pour le kernel
#define TEST_ASSERT_KERNEL_ADDRESS(addr) \
    do { \
        if ((uint32_t)(addr) < 0xC0000000) { \
            unity_test_fail("Expected kernel address", __FILE__, __LINE__); \
            return; \
        } \
    } while(0)

#define TEST_ASSERT_USER_ADDRESS(addr) \
    do { \
        if ((uint32_t)(addr) >= 0xC0000000) { \
            unity_test_fail("Expected user address", __FILE__, __LINE__); \
            return; \
        } \
    } while(0)

#define TEST_ASSERT_PAGE_ALIGNED(addr) \
    do { \
        if (((uint32_t)(addr) & 0xFFF) != 0) { \
            unity_test_fail("Expected page-aligned address", __FILE__, __LINE__); \
            return; \
        } \
    } while(0)

// Macros pour ignorer des tests
#define TEST_IGNORE() \
    do { \
        unity_test_ignore("Test ignored"); \
        return; \
    } while(0)

#define TEST_IGNORE_MESSAGE(message) \
    do { \
        unity_test_ignore(message); \
        return; \
    } while(0)

// Macro pour faire échouer un test explicitement
#define TEST_FAIL() \
    do { \
        unity_test_fail("Test failed explicitly", __FILE__, __LINE__); \
        return; \
    } while(0)

// Macros pour les tests de performance
#define TEST_ASSERT_EXECUTION_TIME_LESS_THAN(threshold_us, code_block) \
    do { \
        uint64_t start_cycles = rdtsc(); \
        code_block; \
        uint64_t end_cycles = rdtsc(); \
        uint64_t execution_time = (end_cycles - start_cycles) / get_cpu_frequency_mhz(); \
        if (execution_time > (threshold_us)) { \
            unity_test_fail("Execution time exceeded threshold", __FILE__, __LINE__); \
            return; \
        } \
    } while(0)

// Fonction pour lire le timestamp counter (pour les tests de performance)
static inline uint64_t rdtsc(void) {
    uint32_t lo, hi;
    __asm__ volatile("rdtsc" : "=a"(lo), "=d"(hi));
    return ((uint64_t)hi << 32) | lo;
}

// Fonction pour obtenir la fréquence CPU (stub pour tests)
uint32_t get_cpu_frequency_mhz(void);

#ifdef __cplusplus
}
#endif

#endif // UNITY_H
