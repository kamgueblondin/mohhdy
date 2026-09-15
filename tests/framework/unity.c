/* unity.c - Implémentation du framework Unity pour MOHHDY */

#include "unity.h"

// Variables globales
unity_stats_t unity_stats;

// Fonctions externes pour l'output (définies dans le kernel ou stubs)
void __attribute__((weak)) unity_putc_redirect(char c) {
    putchar(c);
}
extern void print_string(const char* str);

// Initialization du framework
void unity_init(void) {
    unity_stats.tests_run = 0;
    unity_stats.tests_passed = 0;
    unity_stats.tests_failed = 0;
    unity_stats.tests_ignored = 0;
    unity_stats.current_test_name = NULL;
    unity_stats.last_failure_msg = NULL;
    unity_stats.last_failure_file = NULL;
    unity_stats.last_failure_line = 0;
    
    unity_print_string("Unity Test Framework for MOHHDY\n");
    unity_print_string("==============================\n\n");
}

void unity_cleanup(void) {
    // Nettoyage si nécessaire
}

void unity_print_results(void) {
    unity_print_string("\n==============================\n");
    unity_print_string("Test Results Summary:\n");
    unity_print_string("Tests Run: ");
    unity_print_number(unity_stats.tests_run);
    unity_print_string("\nTests Passed: ");
    unity_print_number(unity_stats.tests_passed);
    unity_print_string("\nTests Failed: ");
    unity_print_number(unity_stats.tests_failed);
    unity_print_string("\nTests Ignored: ");
    unity_print_number(unity_stats.tests_ignored);
    unity_print_string("\n==============================\n");
    
    if (unity_stats.tests_failed == 0) {
        unity_print_string("ALL TESTS PASSED!\n");
    } else {
        unity_print_string("SOME TESTS FAILED!\n");
        if (unity_stats.last_failure_msg) {
            unity_print_string("Last failure: ");
            unity_print_string(unity_stats.last_failure_msg);
            unity_print_string(" at ");
            unity_print_string(unity_stats.last_failure_file);
            unity_print_string(":");
            unity_print_number(unity_stats.last_failure_line);
            unity_print_string("\n");
        }
    }
}

void unity_run_test(void (*test_func)(void), const char* test_name, 
                   const char* file __attribute__((unused)), uint32_t line __attribute__((unused))) {
    unity_stats.current_test_name = test_name;
    unity_stats.tests_run++;
    
    unity_print_string("Running ");
    unity_print_string(test_name);
    unity_print_string("... ");
    
    // Exécuter le test
    test_func();
    
    // Si on arrive ici, le test a réussi
    unity_test_pass();
}

void unity_test_fail(const char* message, const char* file, uint32_t line) {
    unity_stats.tests_failed++;
    unity_stats.last_failure_msg = message;
    unity_stats.last_failure_file = file;
    unity_stats.last_failure_line = line;
    
    unity_print_string("FAIL\n");
    unity_print_string("  ");
    unity_print_string(message);
    unity_print_string(" at ");
    unity_print_string(file);
    unity_print_string(":");
    unity_print_number(line);
    unity_print_string("\n");
}

void unity_test_pass(void) {
    unity_stats.tests_passed++;
    unity_print_string("PASS\n");
}

void unity_test_ignore(const char* message) {
    unity_stats.tests_ignored++;
    unity_print_string("IGNORE\n");
    unity_print_string("  ");
    unity_print_string(message);
    unity_print_string("\n");
}

// Fonctions utilitaires d'affichage
void unity_print_char(char c) {
    putc(c);
}

void unity_print_string(const char* str) {
    if (!str) return;
    
    while (*str) {
        unity_print_char(*str);
        str++;
    }
}

void unity_print_number(uint32_t number) {
    char buffer[12]; // Assez pour un uint32_t
    int i = 0;
    
    if (number == 0) {
        unity_print_char('0');
        return;
    }
    
    // Convertir en chaîne (en inversé)
    while (number > 0) {
        buffer[i++] = '0' + (number % 10);
        number /= 10;
    }
    
    // Afficher en ordre correct
    for (int j = i - 1; j >= 0; j--) {
        unity_print_char(buffer[j]);
    }
}

void unity_print_hex(uint32_t number) {
    const char hex_digits[] = "0123456789ABCDEF";
    
    unity_print_string("0x");
    
    for (int i = 7; i >= 0; i--) {
        uint32_t digit = (number >> (i * 4)) & 0xF;
        unity_print_char(hex_digits[digit]);
    }
}

// Fonctions de comparaison
int unity_compare_int(int expected, int actual) {
    return (expected == actual);
}

int unity_compare_uint(uint32_t expected, uint32_t actual) {
    return (expected == actual);
}

int unity_compare_ptr(const void* expected, const void* actual) {
    return (expected == actual);
}

int unity_compare_memory(const void* expected, const void* actual, size_t len) {
    if (!expected || !actual) return 0;
    
    const uint8_t* exp_bytes = (const uint8_t*)expected;
    const uint8_t* act_bytes = (const uint8_t*)actual;
    
    for (size_t i = 0; i < len; i++) {
        if (exp_bytes[i] != act_bytes[i]) {
            return 0;
        }
    }
    
    return 1;
}

int unity_compare_string(const char* expected, const char* actual) {
    if (!expected || !actual) return 0;
    
    while (*expected && *actual) {
        if (*expected != *actual) return 0;
        expected++;
        actual++;
    }
    
    return (*expected == *actual);
}

// Stub pour la fréquence CPU (peut être implémenté plus tard)
uint32_t get_cpu_frequency_mhz(void) {
    return 1000; // 1GHz par défaut pour les calculs
}

// Fonctions de support pour les tests kernel

// Fonction pour vérifier si une adresse est dans l'espace kernel
int is_kernel_address(uint32_t addr) {
    return (addr >= 0xC0000000);
}

// Fonction pour vérifier si une adresse est alignée sur une page
int is_page_aligned(uint32_t addr) {
    return ((addr & 0xFFF) == 0);
}

// Fonction pour calculer la différence entre deux adresses
uint32_t address_diff(uint32_t addr1, uint32_t addr2) {
    return (addr1 > addr2) ? (addr1 - addr2) : (addr2 - addr1);
}

// Fonction pour simuler une condition de test avec retry
int test_with_retry(int (*test_condition)(void), int max_retries, int delay_ms) {
    for (int i = 0; i < max_retries; i++) {
        if (test_condition()) {
            return 1;
        }
        
        // Delay simple (approximatif)
        for (volatile int j = 0; j < delay_ms * 1000; j++) {
            // Busy wait
        }
    }
    return 0;
}

// Fonction pour afficher l'état de la mémoire (debug)
void unity_dump_memory(const void* addr, size_t len) {
    const uint8_t* bytes = (const uint8_t*)addr;
    
    unity_print_string("Memory dump at ");
    unity_print_hex((uint32_t)addr);
    unity_print_string(":\n");
    
    for (size_t i = 0; i < len; i += 16) {
        unity_print_hex((uint32_t)(addr + i));
        unity_print_string(": ");
        
        // Afficher les bytes en hexadecimal
        for (size_t j = 0; j < 16 && (i + j) < len; j++) {
            if (j == 8) unity_print_char(' ');
            
            uint8_t byte = bytes[i + j];
            unity_print_char("0123456789ABCDEF"[byte >> 4]);
            unity_print_char("0123456789ABCDEF"[byte & 0xF]);
            unity_print_char(' ');
        }
        
        unity_print_char('\n');
    }
}

// Fonction pour mesurer le temps d'exécution
uint64_t unity_measure_execution_time(void (*code_block)(void)) {
    uint64_t start = rdtsc();
    code_block();
    uint64_t end = rdtsc();
    return end - start;
}

// Fonction pour vérifier l'intégrité d'une structure de données
int unity_verify_structure_integrity(const void* structure, size_t size, uint32_t magic) {
    if (!structure) return 0;
    
    // Vérifier le magic number au début
    const uint32_t* magic_ptr = (const uint32_t*)structure;
    if (*magic_ptr != magic) return 0;
    
    // Vérifier le magic number à la fin
    const uint32_t* end_magic = (const uint32_t*)((const char*)structure + size - sizeof(uint32_t));
    if (*end_magic != magic) return 0;
    
    return 1;
}
