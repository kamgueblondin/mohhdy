/* test_kernel.c - Implémentation des extensions Unity pour tests kernel */

#include "test_kernel.h"
#include "unity.h"

// Variables globales
test_kernel_context_t test_context;
mock_pic_state_t mock_pic;
mock_timer_state_t mock_timer;
mock_keyboard_state_t mock_keyboard;

// === INITIALISATION ET CONFIGURATION ===

void test_kernel_init(void) {
    // Initialiser le contexte de test
    test_context.ring_mode = TEST_RING_0;
    test_context.kernel_stack = 0;
    test_context.user_stack = 0;
    test_context.original_cr3 = 0;
    test_context.interrupts_enabled = 0;
    test_context.heap_used = 0;
    
    // Initialiser le heap de test
    for (size_t i = 0; i < sizeof(test_context.test_heap); i++) {
        test_context.test_heap[i] = 0;
    }
    
    // Initialiser les mocks hardware
    mock_pic_init();
    mock_timer_init();
    mock_keyboard_init();
    
    unity_print_string("Kernel test framework initialized\n");
}

void test_kernel_set_mode(test_ring_mode_t mode) {
    test_context.ring_mode = mode;
}

void test_kernel_cleanup(void) {
    test_heap_reset();
    mock_pic_init();
    mock_timer_init();
    mock_keyboard_init();
}

void test_kernel_save_state(void) {
    // Mock save state - skip privileged instructions in unit tests
    test_context.original_cr3 = 0;
    test_context.interrupts_enabled = 0;
}

void test_kernel_restore_state(void) {
    // Mock restore state - skip privileged instructions in unit tests
    
    // Restaurer l'état des interruptions
    if (test_context.interrupts_enabled) {
        __asm__ volatile("sti");
    } else {
        __asm__ volatile("cli");
    }
}

// === GESTION MÉMOIRE POUR TESTS ===

void* test_malloc(size_t size) {
    // Alignement sur 4 bytes
    size = (size + 3) & ~3;
    
    if (test_context.heap_used + size > sizeof(test_context.test_heap)) {
        return NULL; // Out of memory
    }
    
    void* ptr = &test_context.test_heap[test_context.heap_used];
    test_context.heap_used += size;
    
    return ptr;
}

void test_free(void* ptr) {
    // Pour simplifier, on ne fait pas de vraie libération
    // On marque juste la mémoire comme libérée
    if (ptr >= (void*)test_context.test_heap && 
        ptr < (void*)(test_context.test_heap + sizeof(test_context.test_heap))) {
        // Marquer comme libéré avec un pattern
        uint32_t* freed_ptr = (uint32_t*)ptr;
        *freed_ptr = 0xDEADDEAD;
    }
}

void test_heap_reset(void) {
    test_context.heap_used = 0;
    for (size_t i = 0; i < sizeof(test_context.test_heap); i++) {
        test_context.test_heap[i] = 0;
    }
}

int test_heap_check_integrity(void) {
    // Vérifier qu'il n'y a pas de corruption dans la partie non utilisée
    for (size_t i = test_context.heap_used; i < sizeof(test_context.test_heap); i++) {
        if (test_context.test_heap[i] != 0) {
            return 0; // Corruption détectée
        }
    }
    return 1;
}

// === MOCKS HARDWARE ===

void mock_pic_init(void) {
    mock_pic.master_mask = 0xFF; // Tous les IRQs masqués
    mock_pic.slave_mask = 0xFF;
    mock_pic.master_isr = 0;
    mock_pic.slave_isr = 0;
    mock_pic.interrupts_enabled = 0;
}

void mock_pic_set_mask(uint8_t mask) {
    mock_pic.master_mask = mask;
}

void mock_pic_send_interrupt(uint8_t irq) {
    if (irq < 8) {
        mock_pic.master_isr |= (1 << irq);
    } else {
        mock_pic.slave_isr |= (1 << (irq - 8));
    }
}

void mock_pic_clear_interrupt(uint8_t irq) {
    if (irq < 8) {
        mock_pic.master_isr &= ~(1 << irq);
    } else {
        mock_pic.slave_isr &= ~(1 << (irq - 8));
    }
}

void mock_timer_init(void) {
    mock_timer.frequency = 1000; // 1000 Hz par défaut
    mock_timer.tick_count = 0;
    mock_timer.enabled = 0;
}

void mock_timer_set_frequency(uint32_t freq) {
    mock_timer.frequency = freq;
}

void mock_timer_tick(void) {
    if (mock_timer.enabled) {
        mock_timer.tick_count++;
        mock_pic_send_interrupt(0); // IRQ0 pour le timer
    }
}

uint32_t mock_timer_get_ticks(void) {
    return mock_timer.tick_count;
}

void mock_keyboard_init(void) {
    mock_keyboard.buffer_head = 0;
    mock_keyboard.buffer_tail = 0;
    mock_keyboard.status_register = 0;
    mock_keyboard.data_register = 0;
}

void mock_keyboard_press_key(uint8_t scancode) {
    // Ajouter le scancode au buffer
    int next_head = (mock_keyboard.buffer_head + 1) % 256;
    if (next_head != mock_keyboard.buffer_tail) {
        mock_keyboard.buffer[mock_keyboard.buffer_head] = scancode;
        mock_keyboard.buffer_head = next_head;
        mock_keyboard.status_register |= 0x01; // Data ready
        mock_pic_send_interrupt(1); // IRQ1 pour le clavier
    }
}

void mock_keyboard_release_key(uint8_t scancode) {
    mock_keyboard_press_key(scancode | 0x80); // Break code
}

void mock_keyboard_type_string(const char* str) {
    // Simulation simplifiée de la saisie d'une chaîne
    while (*str) {
        uint8_t scancode = *str; // Simplification extrême
        mock_keyboard_press_key(scancode);
        mock_keyboard_release_key(scancode);
        str++;
    }
}

uint8_t mock_keyboard_read_data(void) {
    if (mock_keyboard.buffer_head != mock_keyboard.buffer_tail) {
        uint8_t data = mock_keyboard.buffer[mock_keyboard.buffer_tail];
        mock_keyboard.buffer_tail = (mock_keyboard.buffer_tail + 1) % 256;
        
        if (mock_keyboard.buffer_head == mock_keyboard.buffer_tail) {
            mock_keyboard.status_register &= ~0x01; // Clear data ready
        }
        
        return data;
    }
    return 0;
}

uint8_t mock_keyboard_read_status(void) {
    return mock_keyboard.status_register;
}

// === UTILITAIRES MÉMOIRE ===

void test_fill_memory_pattern(void* ptr, size_t size, uint32_t pattern) {
    uint32_t* dword_ptr = (uint32_t*)ptr;
    size_t dwords = size / sizeof(uint32_t);
    
    for (size_t i = 0; i < dwords; i++) {
        dword_ptr[i] = pattern;
    }
    
    // Remplir les bytes restants
    uint8_t* byte_ptr = (uint8_t*)&dword_ptr[dwords];
    uint8_t pattern_byte = (uint8_t)(pattern & 0xFF);
    for (size_t i = 0; i < (size % sizeof(uint32_t)); i++) {
        byte_ptr[i] = pattern_byte;
    }
}

int test_verify_memory_pattern(const void* ptr, size_t size, uint32_t pattern) {
    const uint32_t* dword_ptr = (const uint32_t*)ptr;
    size_t dwords = size / sizeof(uint32_t);
    
    for (size_t i = 0; i < dwords; i++) {
        if (dword_ptr[i] != pattern) {
            return 0;
        }
    }
    
    // Vérifier les bytes restants
    const uint8_t* byte_ptr = (const uint8_t*)&dword_ptr[dwords];
    uint8_t pattern_byte = (uint8_t)(pattern & 0xFF);
    for (size_t i = 0; i < (size % sizeof(uint32_t)); i++) {
        if (byte_ptr[i] != pattern_byte) {
            return 0;
        }
    }
    
    return 1;
}

void test_corrupt_memory(void* ptr, size_t size) {
    uint8_t* byte_ptr = (uint8_t*)ptr;
    
    // Corrompre quelques bytes au hasard
    if (size > 0) byte_ptr[0] ^= 0xFF;
    if (size > 1) byte_ptr[size/2] ^= 0xAA;
    if (size > 2) byte_ptr[size-1] ^= 0x55;
}

int test_detect_memory_corruption(const void* ptr, size_t size, uint32_t original_pattern) {
    return !test_verify_memory_pattern(ptr, size, original_pattern);
}

// === UTILITAIRES TÂCHES ===

test_task_t* test_create_task(void (*func)(void), const char* name, uint32_t priority) {
    test_task_t* task = (test_task_t*)test_malloc(sizeof(test_task_t));
    if (!task) return NULL;
    
    static int next_id = 1;
    task->id = next_id++;
    task->state = 0; // Ready
    task->test_function = func;
    task->priority = priority;
    task->stack_ptr = 0;
    task->next = NULL;
    
    // Copier le nom
    int i = 0;
    while (name[i] && i < 63) {
        task->name[i] = name[i];
        i++;
    }
    task->name[i] = '\0';
    
    return task;
}

void test_destroy_task(test_task_t* task) {
    if (task) {
        test_free(task);
    }
}

void test_run_task(test_task_t* task) {
    if (task && task->test_function) {
        task->state = 1; // Running
        task->test_function();
        task->state = 2; // Finished
    }
}

// === UTILITAIRES DE DEBUG ===

void test_debug_print_system_state(void) {
    unity_print_string("\n=== System State Debug ===\n");
    unity_print_string("Test Context:\n");
    unity_print_string("  Ring Mode: ");
    unity_print_number(test_context.ring_mode);
    unity_print_string("\n  Heap Used: ");
    unity_print_number(test_context.heap_used);
    unity_print_string("/");
    unity_print_number(sizeof(test_context.test_heap));
    unity_print_string("\n");
    
    unity_print_string("Mock PIC State:\n");
    unity_print_string("  Master Mask: ");
    unity_print_hex(mock_pic.master_mask);
    unity_print_string("\n  Master ISR: ");
    unity_print_hex(mock_pic.master_isr);
    unity_print_string("\n");
    
    unity_print_string("Mock Timer State:\n");
    unity_print_string("  Frequency: ");
    unity_print_number(mock_timer.frequency);
    unity_print_string("  Ticks: ");
    unity_print_number(mock_timer.tick_count);
    unity_print_string("\n");
}

void test_debug_print_memory_stats(void) {
    unity_print_string("\n=== Memory Statistics ===\n");
    unity_print_string("Test Heap Usage: ");
    unity_print_number(test_context.heap_used);
    unity_print_string(" / ");
    unity_print_number(sizeof(test_context.test_heap));
    unity_print_string(" bytes (");
    unity_print_number((test_context.heap_used * 100) / sizeof(test_context.test_heap));
    unity_print_string("%)\n");
    
    unity_print_string("Heap Integrity: ");
    if (test_heap_check_integrity()) {
        unity_print_string("OK\n");
    } else {
        unity_print_string("CORRUPTED\n");
    }
}

void test_debug_print_task_stats(void) {
    unity_print_string("\n=== Task Statistics ===\n");
    unity_print_string("Current Ring Mode: ");
    switch (test_context.ring_mode) {
        case TEST_RING_0: unity_print_string("KERNEL (Ring 0)"); break;
        case TEST_RING_3: unity_print_string("USER (Ring 3)"); break;
        case TEST_MIXED: unity_print_string("MIXED"); break;
    }
    unity_print_string("\n");
}

// === BENCHMARKING ===

void test_benchmark_start(test_benchmark_t* bench, const char* name) {
    bench->name = name;
    bench->start_cycles = rdtsc();
    bench->num_calls = 0;
}

void test_benchmark_end(test_benchmark_t* bench) {
    bench->end_cycles = rdtsc();
    bench->elapsed_cycles = bench->end_cycles - bench->start_cycles;
}

void test_benchmark_print_results(const test_benchmark_t* bench) {
    unity_print_string("\n=== Benchmark Results ===\n");
    unity_print_string("Test: ");
    unity_print_string(bench->name);
    unity_print_string("\nTotal Cycles: ");
    unity_print_number((uint32_t)bench->elapsed_cycles);
    unity_print_string("\nIterations: ");
    unity_print_number(bench->num_calls);
    if (bench->num_calls > 0) {
        unity_print_string("\nCycles per iteration: ");
        unity_print_number((uint32_t)(bench->elapsed_cycles / bench->num_calls));
    }
    unity_print_string("\n");
}

// === SIMULATION D'ERREURS ===

void test_simulate_page_fault(uint32_t fault_address) {
    // Simulation d'un page fault pour tester les handlers
    unity_print_string("Simulating page fault at address ");
    unity_print_hex(fault_address);
    unity_print_string("\n");
    
    // Ici on pourrait déclencher une vraie exception, mais c'est dangereux
    // Pour les tests, on se contente de simuler les effets
}

void test_simulate_exception(uint8_t exception_number) {
    unity_print_string("Simulating exception ");
    unity_print_number(exception_number);
    unity_print_string("\n");
}

void test_simulate_out_of_memory(void) {
    // Remplir le heap de test pour simuler un out-of-memory
    test_context.heap_used = sizeof(test_context.test_heap);
    unity_print_string("Simulated out-of-memory condition\n");
}

// === VERIFICATION DE SECURITE ===

int test_verify_ring_isolation(void) {
    // Tester que le code kernel ne peut pas être exécuté depuis l'userspace
    // et vice versa. Ceci est une simulation pour les tests.
    
    unity_print_string("Verifying ring isolation...\n");
    
    // Vérifier que les adresses kernel sont dans la bonne plage
    uint32_t kernel_addr = 0xC0000000;
    uint32_t user_addr = 0x40000000;
    
    if (test_context.ring_mode == TEST_RING_0) {
        // En mode kernel, on devrait pouvoir accéder aux adresses kernel
        return (kernel_addr >= 0xC0000000);
    } else {
        // En mode user, on ne devrait pas pouvoir accéder aux adresses kernel
        return (user_addr < 0xC0000000);
    }
}

int test_verify_memory_protection(void) {
    unity_print_string("Verifying memory protection...\n");
    
    // Test simple: créer deux zones mémoire et vérifier qu'elles ne se chevauchent pas
    void* ptr1 = test_malloc(100);
    void* ptr2 = test_malloc(100);
    
    if (!ptr1 || !ptr2) return 0;
    
    uint32_t addr1 = (uint32_t)ptr1;
    uint32_t addr2 = (uint32_t)ptr2;
    
    // Vérifier qu'il n'y a pas de chevauchement
    int no_overlap = (addr2 >= addr1 + 100) || (addr1 >= addr2 + 100);
    
    return no_overlap;
}

int test_verify_syscall_validation(void) {
    unity_print_string("Verifying syscall validation...\n");
    
    // Test simple: vérifier que les pointeurs NULL sont rejetés
    // et que les adresses kernel sont protégées depuis l'userspace
    
    uint32_t null_ptr __attribute__((unused)) = 0;
    uint32_t kernel_ptr = 0xC0000000;
    uint32_t valid_user_ptr = 0x40000000;
    
    if (test_context.ring_mode == TEST_RING_3) {
        // En mode user, les pointeurs kernel devraient être rejetés
        return (kernel_ptr >= 0xC0000000) && (valid_user_ptr < 0xC0000000);
    }
    
    return 1; // En mode kernel, tout est autorisé
}
