/* test_syscall.c - Tests unitaires pour le système d'appels système */

#include <string.h>
#include "../../framework/unity.h"
#include "../../framework/test_kernel.h"

// Include du module à tester
#include "../../../kernel/syscall/syscall.h"
#include "../../../fs/overlay.h"
// Ne pas inclure les en-têtes réels du kernel pour éviter les conflits de types

// === MOCK DATA ET HELPERS ===

static char test_output_buffer[1024];
static int test_output_index = 0;
static char test_input_buffer[256];
static int test_input_index = 0;
static int test_input_length = 0;

// Mock des fonctions I/O
void mock_putc(char c) {
    if (test_output_index < sizeof(test_output_buffer) - 1) {
        test_output_buffer[test_output_index++] = c;
        test_output_buffer[test_output_index] = '\0';
    }
}

char mock_getc(void) {
    if (test_input_index < test_input_length) {
        return test_input_buffer[test_input_index++];
    }
    return 0;
}

void setup_test_input(const char* input) {
    test_input_index = 0;
    test_input_length = 0;
    
    while (input[test_input_length] && test_input_length < sizeof(test_input_buffer) - 1) {
        test_input_buffer[test_input_length] = input[test_input_length];
        test_input_length++;
    }
    test_input_buffer[test_input_length] = '\0';
}

void clear_test_output(void) {
    test_output_index = 0;
    test_output_buffer[0] = '\0';
}

// Mock task functions
static int mock_task_exit_called = 0;
static uint32_t mock_exit_code = 0;

void mock_task_exit_with_code(uint32_t code) {
    mock_task_exit_called = 1;
    mock_exit_code = code;
}

// Fonction dummy pour les tâches de test
void dummy_task_function(void) {
    // Fonction vide pour les tests
}

// === SETUP ET TEARDOWN ===

void setUp(void) {
    test_kernel_init();
    test_kernel_save_state();
    
    clear_test_output();
    test_input_index = 0;
    test_input_length = 0;
    mock_task_exit_called = 0;
    mock_exit_code = 0;
    
    syscall_init();
}

void tearDown(void) {
    test_kernel_restore_state();
    test_kernel_cleanup();
}

// === TESTS D'INITIALISATION ===

void test_syscall_init_basic(void) {
    syscall_init();
    
    // Vérifier que l'initialisation s'est bien passée
    // (Pas de vérification spécifique car c'est une fonction void)
    // On teste indirectement via les autres tests
    TEST_ASSERT_TRUE(1);
}

// === TESTS DES APPELS SYSTÈME INDIVIDUELS ===

void test_sys_putc_single_character(void) {
    clear_test_output();
    
    sys_putc('A');
    
    TEST_ASSERT_EQUAL_STRING("A", test_output_buffer);
}

void test_sys_putc_multiple_characters(void) {
    clear_test_output();
    
    sys_putc('H');
    sys_putc('e');
    sys_putc('l');
    sys_putc('l');
    sys_putc('o');
    
    TEST_ASSERT_EQUAL_STRING("Hello", test_output_buffer);
}

void test_sys_putc_special_characters(void) {
    clear_test_output();
    
    sys_putc('\n');
    sys_putc('\t');
    sys_putc('\r');
    sys_putc(' ');
    
    TEST_ASSERT_EQUAL('\n', test_output_buffer[0]);
    TEST_ASSERT_EQUAL('\t', test_output_buffer[1]);
    TEST_ASSERT_EQUAL('\r', test_output_buffer[2]);
    TEST_ASSERT_EQUAL(' ', test_output_buffer[3]);
}

void test_sys_puts_basic_string(void) {
    clear_test_output();
    
    sys_puts("Hello, World!");
    
    TEST_ASSERT_EQUAL_STRING("Hello, World!", test_output_buffer);
}

void test_sys_puts_empty_string(void) {
    clear_test_output();
    
    sys_puts("");
    
    TEST_ASSERT_EQUAL_STRING("", test_output_buffer);
}

void test_sys_puts_null_pointer(void) {
    clear_test_output();
    
    sys_puts(NULL);
    
    // Devrait gérer gracieusement le pointeur NULL
    // Comportement peut varier, mais ne devrait pas crash
    TEST_ASSERT_TRUE(1);
}

void test_sys_getc_single_character(void) {
    setup_test_input("A");
    
    char c = sys_getc();
    
    TEST_ASSERT_EQUAL('A', c);
}

void test_sys_getc_multiple_calls(void) {
    setup_test_input("ABC");
    
    TEST_ASSERT_EQUAL('A', sys_getc());
    TEST_ASSERT_EQUAL('B', sys_getc());
    TEST_ASSERT_EQUAL('C', sys_getc());
}

void test_sys_getc_no_input(void) {
    setup_test_input("");
    
    char c = sys_getc();
    
    // Devrait retourner 0 ou attendre
    TEST_ASSERT_EQUAL(0, c);
}

void test_sys_gets_basic_input(void) {
    setup_test_input("Hello\n");
    
    char buffer[64];
    sys_gets(buffer, sizeof(buffer));
    
    TEST_ASSERT_EQUAL_STRING("Hello", buffer);
}

void test_sys_gets_buffer_size_limit(void) {
    setup_test_input("This is a very long string that should be truncated");
    
    char buffer[10];
    sys_gets(buffer, sizeof(buffer));
    
    // Devrait être tronqué à la taille du buffer
    TEST_ASSERT_EQUAL(9, strlen(buffer)); // -1 pour le \0
    buffer[9] = '\0'; // Assurer la terminaison
}

void test_sys_gets_null_buffer(void) {
    setup_test_input("Test");
    
    sys_gets(NULL, 10);
    
    // Devrait gérer gracieusement le pointeur NULL
    TEST_ASSERT_TRUE(1);
}

void test_sys_exit_with_code(void) {
    sys_exit(42);
    
    TEST_ASSERT_TRUE(mock_task_exit_called);
    TEST_ASSERT_EQUAL(42, mock_exit_code);
}

void test_sys_exit_zero_code(void) {
    sys_exit(0);
    
    TEST_ASSERT_TRUE(mock_task_exit_called);
    TEST_ASSERT_EQUAL(0, mock_exit_code);
}

// === TESTS DU HANDLER SYSCALL ===

void test_syscall_handler_putc(void) {
    clear_test_output();
    
    cpu_state_t cpu_state = {0};
    cpu_state.eax = SYS_PUTC;
    cpu_state.ebx = 'X';
    
    syscall_handler(&cpu_state);
    
    TEST_ASSERT_EQUAL_STRING("X", test_output_buffer);
}

void test_syscall_handler_exit(void) {
    cpu_state_t cpu_state = {0};
    cpu_state.eax = SYS_EXIT;
    cpu_state.ebx = 123;
    
    syscall_handler(&cpu_state);
    
    TEST_ASSERT_TRUE(mock_task_exit_called);
    TEST_ASSERT_EQUAL(123, mock_exit_code);
}

void test_syscall_handler_invalid_syscall(void) {
    cpu_state_t cpu_state = {0};
    cpu_state.eax = 999; // Numéro invalide
    cpu_state.ebx = 0;
    
    // Ne devrait pas crasher
    syscall_handler(&cpu_state);
    
    TEST_ASSERT_TRUE(1);
}

void test_syscall_handler_boundary_syscall_numbers(void) {
    cpu_state_t cpu_state = {0};
    
    // Test avec le plus petit numéro valide
    cpu_state.eax = 0; // SYS_EXIT
    cpu_state.ebx = 0;
    syscall_handler(&cpu_state);
    
    // Reset pour test suivant
    mock_task_exit_called = 0;
    
    // Test avec le plus grand numéro valide
    cpu_state.eax = MAX_SYSCALLS - 1;
    cpu_state.ebx = 0;
    syscall_handler(&cpu_state);
    
    TEST_ASSERT_TRUE(1);
}

// === TESTS DE PERFORMANCE ===

void test_syscall_performance_putc(void) {
    clear_test_output();
    
    const int num_calls = 1000;
    
    TEST_BENCHMARK("SYS_PUTC Performance", num_calls, {
        sys_putc('.');
    });
    
    TEST_ASSERT_EQUAL(num_calls, strlen(test_output_buffer));
}

void test_syscall_performance_getc(void) {
    // Préparer un long input
    char long_input[1001];
    for (int i = 0; i < 1000; i++) {
        long_input[i] = 'A' + (i % 26);
    }
    long_input[1000] = '\0';
    
    setup_test_input(long_input);
    
    const int num_calls = 1000;
    
    TEST_BENCHMARK("SYS_GETC Performance", num_calls, {
        char c = sys_getc();
        (void)c; // Éviter warning unused
    });
}

void test_syscall_handler_performance(void) {
    clear_test_output();
    
    cpu_state_t cpu_state = {0};
    cpu_state.eax = SYS_PUTC;
    cpu_state.ebx = '.';
    
    const int num_calls = 1000;
    
    TEST_BENCHMARK("Syscall Handler Performance", num_calls, {
        syscall_handler(&cpu_state);
    });
}

// === TESTS DE ROBUSTESSE ===

void test_syscall_with_corrupted_state(void) {
    cpu_state_t cpu_state;
    
    // Remplir avec un pattern pour simuler corruption
    test_fill_memory_pattern(&cpu_state, sizeof(cpu_state), 0xDEADBEEF);
    
    // Configurer un syscall valide malgré la corruption
    cpu_state.eax = SYS_PUTC;
    cpu_state.ebx = 'Z';
    
    clear_test_output();
    
    // Ne devrait pas crasher
    syscall_handler(&cpu_state);
    
    // Devrait quand même fonctionner
    TEST_ASSERT_EQUAL_STRING("Z", test_output_buffer);
}

void test_syscall_buffer_overflow_protection(void) {
    // Test avec des paramètres qui pourraient causer un overflow
    char small_buffer[5];
    
    setup_test_input("This is way too long for the buffer");
    
    sys_gets(small_buffer, sizeof(small_buffer));
    
    // Vérifier qu'il n'y a pas eu d'overflow
    for (int i = 0; i < sizeof(small_buffer); i++) {
        // Le buffer ne devrait pas contenir de caractères inattendus
        if (small_buffer[i] == '\0') break;
        TEST_ASSERT(small_buffer[i] >= ' ' && small_buffer[i] <= '~');
    }
}

void test_syscall_parameter_validation(void) {
    cpu_state_t cpu_state = {0};
    
    // Test avec des pointeurs invalides
    cpu_state.eax = SYS_PUTS;
    cpu_state.ebx = 0; // NULL pointer
    
    // Ne devrait pas crasher
    syscall_handler(&cpu_state);
    
    // Test avec une adresse kernel depuis userspace
    cpu_state.eax = SYS_PUTS;
    cpu_state.ebx = 0xC0000000; // Adresse kernel
    
    // Devrait être rejeté ou géré gracieusement
    syscall_handler(&cpu_state);
    
    TEST_ASSERT_TRUE(1);
}

// === TESTS D'INTÉGRATION ===

void test_syscall_integration_with_tasks(void) {
    // Simuler une tâche utilisateur qui fait des appels système
    test_task_t* user_task = test_create_task(dummy_task_function, "user_task", 1);
    // Adapter vers le type mocké task_t défini dans kernel_mocks.c
    current_task = (task_t*)user_task;
    
    // Test d'appels système depuis une tâche utilisateur
    clear_test_output();
    
    cpu_state_t cpu_state = {0};
    cpu_state.eax = SYS_PUTC;
    cpu_state.ebx = 'U';
    
    syscall_handler(&cpu_state);
    
    TEST_ASSERT_EQUAL_STRING("U", test_output_buffer);
    
    test_destroy_task(user_task);
}

void test_syscall_exec_basic(void) {
    // Test basique de sys_exec (simulation)
    char* argv[] = {"test_program", NULL};
    
    int result = sys_exec("/bin/test_program", argv);
    
    // Le résultat dépend de l'implémentation
    // On teste juste que ça ne crash pas
    (void)result;
    TEST_ASSERT_TRUE(1);
}

void test_syscall_yield_integration(void) {
    // Créer plusieurs tâches pour tester yield
    test_task_t* task1 = test_create_task(dummy_task_function, "task1", 1);
    test_task_t* task2 = test_create_task(dummy_task_function, "task2", 1);
    
    add_task_to_queue((task_t*)task1);
    add_task_to_queue((task_t*)task2);
    
    current_task = (task_t*)task1;
    
    // Appeler sys_yield
    sys_yield();
    
    // Vérifier qu'un reschedule a été demandé
    TEST_ASSERT_TRUE(g_reschedule_needed);
    
    test_destroy_task(task1);
    test_destroy_task(task2);
}

// === TESTS DE SÉCURITÉ ===

void test_syscall_ring_isolation(void) {
    // Tester que les syscalls respectent l'isolation Ring 0/3
    test_kernel_set_mode(TEST_RING_3); // Mode utilisateur
    
    cpu_state_t cpu_state = {0};
    cpu_state.eax = SYS_PUTC;
    cpu_state.ebx = 'S';
    cpu_state.cs = 0x1B; // User code segment
    
    clear_test_output();
    
    syscall_handler(&cpu_state);
    
    // Devrait fonctionner depuis l'userspace
    TEST_ASSERT_EQUAL_STRING("S", test_output_buffer);
}

void test_syscall_privilege_escalation_prevention(void) {
    // Tenter d'accéder à des fonctions kernel depuis userspace
    test_kernel_set_mode(TEST_RING_3);
    
    cpu_state_t cpu_state = {0};
    cpu_state.eax = SYS_PUTS;
    cpu_state.ebx = 0xC0000000; // Adresse kernel
    cpu_state.cs = 0x1B; // User segment
    
    // Devrait être rejeté
    syscall_handler(&cpu_state);
    
    TEST_ASSERT_TRUE(1); // Test passe si pas de crash
}

void test_sys_getpid_and_ticks(void) {
    test_task_t* task = test_create_task(dummy_task_function, "shell", 1);
    current_task = (task_t*)task;
    current_task->id = 2;

    cpu_state_t cpu = {0};
    cpu.eax = SYS_GETPID;
    syscall_handler(&cpu);
    TEST_ASSERT_EQUAL(2, (int)cpu.eax);

    cpu.eax = SYS_TICKS;
    syscall_handler(&cpu);
    TEST_ASSERT_EQUAL(42, (int)cpu.eax);

    test_destroy_task(task);
}

void test_sys_gpt2_gguf_continue_requires_session(void) {
    cpu_state_t cpu = {0};
    char output[8] = {0};
    cpu.eax = SYS_GPT2_GGUF_CONTINUE;
    cpu.ecx = (uint32_t)output;
    cpu.edx = sizeof(output);
    syscall_handler(&cpu);
    TEST_ASSERT_EQUAL(-6, (int)cpu.eax);
    TEST_ASSERT_EQUAL(0, output[0]);
}

void test_sys_readfile_and_listdir(void) {
    char buf[64];
    os_dirent_t ents[8];
    cpu_state_t cpu = {0};
    int n;
    int found_hello = 0;

    cpu.eax = SYS_READFILE;
    cpu.ebx = (uint32_t)"hello.txt";
    cpu.ecx = (uint32_t)buf;
    cpu.edx = sizeof(buf);
    syscall_handler(&cpu);
    TEST_ASSERT_EQUAL(18, (int)cpu.eax);
    buf[18] = '\0';
    TEST_ASSERT_EQUAL_STRING("hello from initrd\n", buf);

    cpu.eax = SYS_LISTDIR;
    cpu.ebx = (uint32_t)"/";
    cpu.ecx = (uint32_t)ents;
    cpu.edx = 8;
    syscall_handler(&cpu);
    n = (int)cpu.eax;
    TEST_ASSERT(n > 0);
    for (int i = 0; i < n; i++) {
        if (ents[i].name[0] == 'h') found_hello = 1;
    }
    TEST_ASSERT(found_hello);
}

void test_sys_kill_protects_kernel(void) {
    cpu_state_t cpu = {0};
    cpu.eax = SYS_KILL;
    cpu.ebx = 0;
    syscall_handler(&cpu);
    TEST_ASSERT_EQUAL(-2, (int)cpu.eax);
}

void test_sys_meminfo(void) {
    os_meminfo_t info;
    cpu_state_t cpu = {0};
    cpu.eax = SYS_MEMINFO;
    cpu.ebx = (uint32_t)&info;
    syscall_handler(&cpu);
    TEST_ASSERT_EQUAL(0, (int)cpu.eax);
    TEST_ASSERT_EQUAL(32768, (int)info.total_pages);
    TEST_ASSERT(info.free_pages > 0);
}

void test_sys_task_priority_and_metrics(void) {
    task_t* old_queue = task_queue;
    task_t* old_current = current_task;
    task_t* task;
    task_t* unrelated;
    os_task_metrics_t metrics;
    cpu_state_t cpu = {0};

    task_queue = NULL;
    current_task = NULL;
    task = create_task(dummy_task_function);
    TEST_ASSERT_NOT_NULL(task);
    task->type = TASK_TYPE_USER;
    task->state = TASK_RUNNING;
    add_task_to_queue(task);
    unrelated = create_task(dummy_task_function);
    TEST_ASSERT_NOT_NULL(unrelated);
    unrelated->type = TASK_TYPE_USER;
    add_task_to_queue(unrelated);
    current_task = task;

    cpu.eax = SYS_TASK_SET_PRIORITY;
    cpu.ebx = (uint32_t)task->id;
    cpu.ecx = OS_TASK_PRIORITY_HIGH;
    syscall_handler(&cpu);
    TEST_ASSERT_EQUAL(0, (int)cpu.eax);

    cpu.eax = SYS_TASK_METRICS;
    cpu.ebx = (uint32_t)task->id;
    cpu.ecx = (uint32_t)&metrics;
    syscall_handler(&cpu);
    TEST_ASSERT_EQUAL(0, (int)cpu.eax);
    TEST_ASSERT_EQUAL(OS_TASK_PRIORITY_HIGH, metrics.priority);

    cpu.eax = SYS_TASK_SET_PRIORITY;
    cpu.ebx = (uint32_t)task->id;
    cpu.ecx = 4;
    syscall_handler(&cpu);
    TEST_ASSERT_EQUAL(OS_TASK_BAD_PRIORITY, (int)cpu.eax);
    TEST_ASSERT_EQUAL(OS_TASK_PRIORITY_HIGH, task->priority);

    current_task = unrelated;
    cpu.eax = SYS_TASK_SET_PRIORITY;
    cpu.ebx = (uint32_t)task->id;
    cpu.ecx = OS_TASK_PRIORITY_LOW;
    syscall_handler(&cpu);
    TEST_ASSERT_EQUAL(OS_TASK_CONTROL_DENIED, (int)cpu.eax);
    TEST_ASSERT_EQUAL(OS_TASK_PRIORITY_HIGH, task->priority);

    task_queue = old_queue;
    current_task = old_current;
}

void test_sys_task_name_and_capacity(void) {
    task_t* old_queue = task_queue;
    task_t* old_current = current_task;
    task_t* parent;
    task_t* child;
    os_task_capacity_t capacity;
    os_task_exit_result_t result;
    os_task_exit_history_t history;
    os_task_exit_history_observation_t observation;
    os_task_children_t children;
    os_task_child_exit_count_t exit_count;
    cpu_state_t cpu = {0};

    task_queue = NULL;
    current_task = NULL;
    parent = create_task(dummy_task_function);
    child = create_task(dummy_task_function);
    TEST_ASSERT_NOT_NULL(parent);
    TEST_ASSERT_NOT_NULL(child);
    parent->type = TASK_TYPE_USER;
    parent->state = TASK_RUNNING;
    child->type = TASK_TYPE_USER;
    child->parent_pid = parent->id;
    add_task_to_queue(parent);
    add_task_to_queue(child);
    current_task = parent;

    cpu.eax = SYS_TASK_SET_NAME;
    cpu.ebx = (uint32_t)child->id;
    cpu.ecx = (uint32_t)"worker";
    syscall_handler(&cpu);
    TEST_ASSERT_EQUAL(0, (int)cpu.eax);
    TEST_ASSERT_EQUAL_STRING("worker", child->name);

    current_task = child;
    cpu.eax = SYS_TASK_SET_NAME;
    cpu.ebx = (uint32_t)parent->id;
    cpu.ecx = (uint32_t)"blocked";
    syscall_handler(&cpu);
    TEST_ASSERT_EQUAL(OS_TASK_CONTROL_DENIED, (int)cpu.eax);

    current_task = parent;
    cpu.eax = SYS_TASK_CHILDREN;
    cpu.ebx = (uint32_t)&children;
    syscall_handler(&cpu);
    TEST_ASSERT_EQUAL(0, (int)cpu.eax);
    TEST_ASSERT_EQUAL(1, children.count);
    TEST_ASSERT_EQUAL(child->id, children.entries[0].pid);
    cpu.eax = SYS_TASK_WAIT_ANY;
    syscall_handler(&cpu);
    TEST_ASSERT_EQUAL(0, (int)cpu.eax);
    TEST_ASSERT_EQUAL(TASK_WAITING, parent->state);
    parent->state = TASK_RUNNING;

    cpu.eax = SYS_TASK_SUSPEND;
    cpu.ebx = (uint32_t)child->id;
    syscall_handler(&cpu);
    TEST_ASSERT_EQUAL(0, (int)cpu.eax);
    TEST_ASSERT_EQUAL(TASK_SUSPENDED, child->state);
    cpu.eax = SYS_TASK_SUSPEND;
    syscall_handler(&cpu);
    TEST_ASSERT_EQUAL(OS_TASK_BAD_STATE, (int)cpu.eax);
    cpu.eax = SYS_TASK_RESUME;
    syscall_handler(&cpu);
    TEST_ASSERT_EQUAL(0, (int)cpu.eax);
    TEST_ASSERT_EQUAL(TASK_READY, child->state);
    cpu.eax = SYS_TASK_RESUME;
    syscall_handler(&cpu);
    TEST_ASSERT_EQUAL(OS_TASK_BAD_STATE, (int)cpu.eax);

    cpu.eax = SYS_TASK_CAPACITY;
    cpu.ebx = (uint32_t)&capacity;
    syscall_handler(&cpu);
    TEST_ASSERT_EQUAL(0, (int)cpu.eax);
    TEST_ASSERT_EQUAL(2, capacity.active);
    TEST_ASSERT_EQUAL(OS_TASK_GLOBAL_CAPACITY, capacity.capacity);
    TEST_ASSERT_EQUAL(OS_TASK_GLOBAL_CAPACITY - 2U, capacity.available);

    task_report_parent_exit(child, 42, OS_TASK_EVENT_EXITED);
    current_task = parent;
    cpu.eax = SYS_TASK_CHILD_EXIT_COUNT;
    cpu.ebx = (uint32_t)&exit_count;
    syscall_handler(&cpu);
    TEST_ASSERT_EQUAL(0, (int)cpu.eax);
    TEST_ASSERT_EQUAL(1, exit_count.count);
    cpu.eax = SYS_TASK_CHILD_RESULT;
    cpu.ebx = (uint32_t)child->id;
    cpu.ecx = (uint32_t)&result;
    syscall_handler(&cpu);
    TEST_ASSERT_EQUAL(0, (int)cpu.eax);
    TEST_ASSERT_EQUAL(child->id, result.child_pid);
    TEST_ASSERT_EQUAL(42, result.exit_code);
    TEST_ASSERT_EQUAL(OS_TASK_EVENT_EXITED, result.reason);

    cpu.eax = SYS_TASK_CHILD_RESULT;
    cpu.ebx = (uint32_t)(child->id + 1);
    cpu.ecx = (uint32_t)&result;
    syscall_handler(&cpu);
    TEST_ASSERT_EQUAL(OS_TASK_NO_CHILD_RESULT, (int)cpu.eax);

    cpu.eax = SYS_TASK_CHILD_RESULT_LIST;
    cpu.ebx = (uint32_t)&history;
    syscall_handler(&cpu);
    TEST_ASSERT_EQUAL(0, (int)cpu.eax);
    TEST_ASSERT_EQUAL(1, history.count);
    TEST_ASSERT_EQUAL(child->id, history.entries[0].child_pid);
    TEST_ASSERT_EQUAL(42, history.entries[0].exit_code);

    cpu.eax = SYS_TASK_CHILD_RESULT_OBSERVE;
    cpu.ebx = 1U;
    cpu.ecx = (uint32_t)&observation;
    syscall_handler(&cpu);
    TEST_ASSERT_EQUAL(OS_TASK_HISTORY_STALE, (int)cpu.eax);
    TEST_ASSERT_EQUAL(2, observation.generation);

    cpu.eax = SYS_TASK_CHILD_RESULT_OBSERVE;
    cpu.ebx = 2U;
    cpu.ecx = (uint32_t)&observation;
    syscall_handler(&cpu);
    TEST_ASSERT_EQUAL(0, (int)cpu.eax);
    TEST_ASSERT_EQUAL(1, observation.history.count);

    cpu.eax = SYS_TASK_CHILD_RESULT_ACK;
    syscall_handler(&cpu);
    TEST_ASSERT_EQUAL(3, (int)cpu.eax);
    cpu.eax = SYS_TASK_CHILD_EXIT_COUNT;
    cpu.ebx = (uint32_t)&exit_count;
    syscall_handler(&cpu);
    TEST_ASSERT_EQUAL(0, (int)cpu.eax);
    TEST_ASSERT_EQUAL(1, exit_count.count);
    cpu.eax = SYS_TASK_CHILD_RESULT_LIST;
    cpu.ebx = (uint32_t)&history;
    syscall_handler(&cpu);
    TEST_ASSERT_EQUAL(0, (int)cpu.eax);
    TEST_ASSERT_EQUAL(0, history.count);

    task_report_parent_exit(child, 13, OS_TASK_EVENT_EXITED);
    cpu.eax = SYS_TASK_CHILD_EXIT_COUNT;
    cpu.ebx = (uint32_t)&exit_count;
    syscall_handler(&cpu);
    TEST_ASSERT_EQUAL(0, (int)cpu.eax);
    TEST_ASSERT_EQUAL(2, exit_count.count);
    cpu.eax = SYS_TASK_CHILD_RESULT_FIND;
    cpu.ebx = (uint32_t)child->id;
    cpu.ecx = (uint32_t)&result;
    syscall_handler(&cpu);
    TEST_ASSERT_EQUAL(0, (int)cpu.eax);
    TEST_ASSERT_EQUAL(13, result.exit_code);

    cpu.eax = SYS_TASK_CHILD_RESULT_FORGET;
    cpu.ebx = (uint32_t)child->id;
    syscall_handler(&cpu);
    TEST_ASSERT_EQUAL(5, (int)cpu.eax);
    cpu.eax = SYS_TASK_CHILD_RESULT_FIND;
    cpu.ebx = (uint32_t)child->id;
    cpu.ecx = (uint32_t)&result;
    syscall_handler(&cpu);
    TEST_ASSERT_EQUAL(OS_TASK_NO_CHILD_RESULT, (int)cpu.eax);

    cpu.eax = SYS_TASK_KILL_CHILDREN;
    syscall_handler(&cpu);
    TEST_ASSERT_EQUAL(1, (int)cpu.eax);
    TEST_ASSERT_NULL(get_task_by_id(child->id));
    cpu.eax = SYS_TASK_KILL_CHILDREN;
    syscall_handler(&cpu);
    TEST_ASSERT_EQUAL(0, (int)cpu.eax);
    cpu.eax = SYS_TASK_WAIT_ANY;
    syscall_handler(&cpu);
    TEST_ASSERT_EQUAL(OS_TASK_NO_DIRECT_CHILD, (int)cpu.eax);

    task_queue = old_queue;
    current_task = old_current;
}

void test_sys_task_delegate_child(void) {
    task_t* old_queue = task_queue;
    task_t* old_current = current_task;
    task_t* parent;
    task_t* supervisor;
    task_t* child;
    cpu_state_t cpu = {0};

    task_queue = NULL;
    current_task = NULL;
    parent = create_task(dummy_task_function);
    supervisor = create_task(dummy_task_function);
    child = create_task(dummy_task_function);
    TEST_ASSERT_NOT_NULL(parent);
    TEST_ASSERT_NOT_NULL(supervisor);
    TEST_ASSERT_NOT_NULL(child);
    parent->type = TASK_TYPE_USER;
    supervisor->type = TASK_TYPE_USER;
    child->type = TASK_TYPE_USER;
    child->parent_pid = parent->id;
    add_task_to_queue(parent);
    add_task_to_queue(supervisor);
    add_task_to_queue(child);

    current_task = parent;
    cpu.eax = SYS_TASK_DELEGATE_CHILD;
    cpu.ebx = (uint32_t)child->id;
    cpu.ecx = (uint32_t)supervisor->id;
    syscall_handler(&cpu);
    TEST_ASSERT_EQUAL(0, (int)cpu.eax);
    TEST_ASSERT_EQUAL(supervisor->id, child->parent_pid);

    cpu.eax = SYS_TASK_DELEGATE_CHILD;
    cpu.ebx = (uint32_t)child->id;
    cpu.ecx = (uint32_t)parent->id;
    syscall_handler(&cpu);
    TEST_ASSERT_EQUAL(OS_TASK_NOT_CHILD, (int)cpu.eax);

    current_task = supervisor;
    cpu.eax = SYS_TASK_DELEGATE_CHILD;
    cpu.ebx = (uint32_t)child->id;
    cpu.ecx = (uint32_t)parent->id;
    syscall_handler(&cpu);
    TEST_ASSERT_EQUAL(0, (int)cpu.eax);
    TEST_ASSERT_EQUAL(parent->id, child->parent_pid);

    current_task = parent;
    cpu.eax = SYS_TASK_DELEGATE_CHILD;
    cpu.ebx = (uint32_t)child->id;
    cpu.ecx = (uint32_t)child->id;
    syscall_handler(&cpu);
    TEST_ASSERT_EQUAL(OS_TASK_BAD_DELEGATE, (int)cpu.eax);

    task_queue = old_queue;
    current_task = old_current;
}

void test_sys_task_supervision_events(void) {
    task_t* old_queue = task_queue;
    task_t* old_current = current_task;
    task_t* parent;
    task_t* child;
    task_t* supervisor;
    os_task_supervision_events_t events;
    os_task_supervision_events_observation_t observation;
    cpu_state_t cpu = {0};

    task_queue = NULL;
    current_task = NULL;
    parent = create_task(dummy_task_function);
    child = create_task(dummy_task_function);
    supervisor = create_task(dummy_task_function);
    TEST_ASSERT_NOT_NULL(parent);
    TEST_ASSERT_NOT_NULL(child);
    TEST_ASSERT_NOT_NULL(supervisor);
    parent->type = TASK_TYPE_USER;
    child->type = TASK_TYPE_USER;
    supervisor->type = TASK_TYPE_USER;
    child->parent_pid = parent->id;
    add_task_to_queue(parent);
    add_task_to_queue(child);
    add_task_to_queue(supervisor);

    current_task = parent;
    cpu.eax = SYS_TASK_SUSPEND;
    cpu.ebx = (uint32_t)child->id;
    syscall_handler(&cpu);
    TEST_ASSERT_EQUAL(0, (int)cpu.eax);
    cpu.eax = SYS_TASK_RESUME;
    syscall_handler(&cpu);
    TEST_ASSERT_EQUAL(0, (int)cpu.eax);
    cpu.eax = SYS_TASK_SUPERVISION_EVENTS;
    cpu.ebx = (uint32_t)&events;
    syscall_handler(&cpu);
    TEST_ASSERT_EQUAL(0, (int)cpu.eax);
    TEST_ASSERT_EQUAL(2, events.count);
    TEST_ASSERT_EQUAL(OS_TASK_SUPERVISION_SUSPEND, events.entries[0].action);
    TEST_ASSERT_EQUAL(OS_TASK_SUPERVISION_RESUME, events.entries[1].action);
    cpu.eax = SYS_TASK_SUPERVISION_EVENTS_OBSERVE;
    cpu.ebx = 1U;
    cpu.ecx = (uint32_t)&observation;
    syscall_handler(&cpu);
    TEST_ASSERT_EQUAL(OS_TASK_HISTORY_STALE, (int)cpu.eax);
    TEST_ASSERT_EQUAL(2, observation.generation);
    cpu.eax = SYS_TASK_SUPERVISION_EVENTS_OBSERVE;
    cpu.ebx = 2U;
    cpu.ecx = (uint32_t)&observation;
    syscall_handler(&cpu);
    TEST_ASSERT_EQUAL(0, (int)cpu.eax);
    TEST_ASSERT_EQUAL(2, observation.events.count);
    cpu.eax = SYS_TASK_SUPERVISION_EVENTS_ACK;
    syscall_handler(&cpu);
    TEST_ASSERT_EQUAL(3, (int)cpu.eax);
    cpu.eax = SYS_TASK_SUPERVISION_EVENTS;
    cpu.ebx = (uint32_t)&events;
    syscall_handler(&cpu);
    TEST_ASSERT_EQUAL(0, (int)cpu.eax);
    TEST_ASSERT_EQUAL(3, events.generation);
    TEST_ASSERT_EQUAL(0, events.count);

    cpu.eax = SYS_TASK_DELEGATE_CHILD;
    cpu.ebx = (uint32_t)child->id;
    cpu.ecx = (uint32_t)supervisor->id;
    syscall_handler(&cpu);
    TEST_ASSERT_EQUAL(0, (int)cpu.eax);
    cpu.eax = SYS_TASK_SUPERVISION_EVENTS;
    cpu.ebx = (uint32_t)&events;
    syscall_handler(&cpu);
    TEST_ASSERT_EQUAL(0, (int)cpu.eax);
    TEST_ASSERT_EQUAL(1, events.count);
    TEST_ASSERT_EQUAL(4, events.generation);
    TEST_ASSERT_EQUAL(OS_TASK_SUPERVISION_DELEGATE_OUT, events.entries[0].action);

    current_task = supervisor;
    cpu.eax = SYS_TASK_SUPERVISION_EVENTS;
    cpu.ebx = (uint32_t)&events;
    syscall_handler(&cpu);
    TEST_ASSERT_EQUAL(0, (int)cpu.eax);
    TEST_ASSERT_EQUAL(1, events.count);
    TEST_ASSERT_EQUAL(OS_TASK_SUPERVISION_DELEGATE_IN, events.entries[0].action);

    task_queue = old_queue;
    current_task = old_current;
}

void test_sys_task_supervision_event_selective(void) {
    task_t* old_queue = task_queue;
    task_t* old_current = current_task;
    task_t* parent;
    task_t* child;
    os_task_supervision_events_t events;
    os_task_supervision_event_t event;
    cpu_state_t cpu = {0};

    task_queue = NULL;
    current_task = NULL;
    parent = create_task(dummy_task_function);
    child = create_task(dummy_task_function);
    TEST_ASSERT_NOT_NULL(parent);
    TEST_ASSERT_NOT_NULL(child);
    parent->type = TASK_TYPE_USER;
    child->type = TASK_TYPE_USER;
    child->parent_pid = parent->id;
    add_task_to_queue(parent);
    add_task_to_queue(child);
    current_task = parent;

    cpu.eax = SYS_TASK_SUSPEND;
    cpu.ebx = (uint32_t)child->id;
    syscall_handler(&cpu);
    TEST_ASSERT_EQUAL(0, (int)cpu.eax);
    cpu.eax = SYS_TASK_RESUME;
    syscall_handler(&cpu);
    TEST_ASSERT_EQUAL(0, (int)cpu.eax);
    cpu.eax = SYS_TASK_SUPERVISION_EVENT_FIND;
    cpu.ebx = 1U;
    cpu.ecx = (uint32_t)&event;
    syscall_handler(&cpu);
    TEST_ASSERT_EQUAL(0, (int)cpu.eax);
    TEST_ASSERT_EQUAL(OS_TASK_SUPERVISION_SUSPEND, event.action);
    cpu.eax = SYS_TASK_SUPERVISION_EVENT_FORGET;
    cpu.ebx = 1U;
    syscall_handler(&cpu);
    TEST_ASSERT_EQUAL(1, (int)cpu.eax);
    cpu.eax = SYS_TASK_SUPERVISION_EVENTS;
    cpu.ebx = (uint32_t)&events;
    syscall_handler(&cpu);
    TEST_ASSERT_EQUAL(0, (int)cpu.eax);
    TEST_ASSERT_EQUAL(3, events.generation);
    TEST_ASSERT_EQUAL(1, events.count);
    TEST_ASSERT_EQUAL(2, events.entries[0].sequence);
    cpu.eax = SYS_TASK_SUPERVISION_EVENT_FIND;
    cpu.ebx = 1U;
    cpu.ecx = (uint32_t)&event;
    syscall_handler(&cpu);
    TEST_ASSERT_EQUAL(OS_TASK_NO_SUPERVISION_EVENT, (int)cpu.eax);

    task_queue = old_queue;
    current_task = old_current;
}

void test_sys_task_supervision_notify(void) {
    task_t* old_queue = task_queue;
    task_t* old_current = current_task;
    task_t* parent;
    task_t* child;
    os_ipc_message_t message;
    os_task_supervision_event_t event;
    cpu_state_t cpu = {0};

    task_queue = NULL;
    current_task = NULL;
    parent = create_task(dummy_task_function);
    child = create_task(dummy_task_function);
    TEST_ASSERT_NOT_NULL(parent);
    TEST_ASSERT_NOT_NULL(child);
    parent->type = TASK_TYPE_USER;
    child->type = TASK_TYPE_USER;
    child->parent_pid = parent->id;
    add_task_to_queue(parent);
    add_task_to_queue(child);
    current_task = parent;

    cpu.eax = SYS_TASK_SUPERVISION_NOTIFY;
    cpu.ebx = 1U;
    syscall_handler(&cpu);
    TEST_ASSERT_EQUAL(1, (int)cpu.eax);
    cpu.eax = SYS_TASK_SUSPEND;
    cpu.ebx = (uint32_t)child->id;
    syscall_handler(&cpu);
    TEST_ASSERT_EQUAL(0, (int)cpu.eax);
    TEST_ASSERT_EQUAL(1, parent->ipc_endpoint.count);
    message = parent->ipc_endpoint.messages[parent->ipc_endpoint.read_index];
    parent->ipc_endpoint.read_index = (parent->ipc_endpoint.read_index + 1U) % IPC_ENDPOINT_CAPACITY;
    parent->ipc_endpoint.count--;
    TEST_ASSERT_EQUAL(0, os_task_parse_supervision_event(&message, &event));
    TEST_ASSERT_EQUAL(OS_TASK_SUPERVISION_SUSPEND, event.action);
    TEST_ASSERT_EQUAL(child->id, event.child_pid);

    cpu.eax = SYS_TASK_SUPERVISION_NOTIFY;
    cpu.ebx = 2U;
    syscall_handler(&cpu);
    TEST_ASSERT_EQUAL(OS_TASK_BAD_NOTIFY, (int)cpu.eax);
    cpu.eax = SYS_TASK_SUPERVISION_NOTIFY;
    cpu.ebx = 0U;
    syscall_handler(&cpu);
    TEST_ASSERT_EQUAL(0, (int)cpu.eax);
    cpu.eax = SYS_TASK_RESUME;
    cpu.ebx = (uint32_t)child->id;
    syscall_handler(&cpu);
    TEST_ASSERT_EQUAL(0, (int)cpu.eax);
    TEST_ASSERT_EQUAL(0, parent->ipc_endpoint.count);

    task_queue = old_queue;
    current_task = old_current;
}

void test_sys_task_supervision_notify_policy(void) {
    task_t* old_queue = task_queue;
    task_t* old_current = current_task;
    task_t* parent;
    task_t* child;
    os_task_supervision_notify_status_t status;
    os_ipc_message_t message;
    os_task_supervision_event_t event;
    cpu_state_t cpu = {0};

    task_queue = NULL;
    current_task = NULL;
    parent = create_task(dummy_task_function);
    child = create_task(dummy_task_function);
    TEST_ASSERT_NOT_NULL(parent);
    TEST_ASSERT_NOT_NULL(child);
    parent->type = TASK_TYPE_USER;
    child->type = TASK_TYPE_USER;
    child->parent_pid = parent->id;
    add_task_to_queue(parent);
    add_task_to_queue(child);
    current_task = parent;

    cpu.eax = SYS_TASK_SUPERVISION_NOTIFY_STATUS;
    cpu.ebx = (uint32_t)&status;
    syscall_handler(&cpu);
    TEST_ASSERT_EQUAL(0, (int)cpu.eax);
    TEST_ASSERT_EQUAL(0, status.enabled);
    TEST_ASSERT_EQUAL(OS_TASK_SUPERVISION_NOTIFY_ALL, status.mask);
    cpu.eax = SYS_TASK_SUPERVISION_NOTIFY_FILTER;
    cpu.ebx = 1U << 5;
    syscall_handler(&cpu);
    TEST_ASSERT_EQUAL(OS_TASK_BAD_NOTIFY_FILTER, (int)cpu.eax);
    cpu.eax = SYS_TASK_SUPERVISION_NOTIFY_FILTER;
    cpu.ebx = OS_TASK_SUPERVISION_NOTIFY_SUSPEND;
    syscall_handler(&cpu);
    TEST_ASSERT_EQUAL(0, (int)cpu.eax);
    cpu.eax = SYS_TASK_SUPERVISION_NOTIFY;
    cpu.ebx = 1U;
    syscall_handler(&cpu);
    TEST_ASSERT_EQUAL(1, (int)cpu.eax);
    cpu.eax = SYS_TASK_SUSPEND;
    cpu.ebx = (uint32_t)child->id;
    syscall_handler(&cpu);
    TEST_ASSERT_EQUAL(0, (int)cpu.eax);
    TEST_ASSERT_EQUAL(1, parent->ipc_endpoint.count);
    message = parent->ipc_endpoint.messages[parent->ipc_endpoint.read_index];
    parent->ipc_endpoint.read_index = (parent->ipc_endpoint.read_index + 1U) % IPC_ENDPOINT_CAPACITY;
    parent->ipc_endpoint.count--;
    TEST_ASSERT_EQUAL(0, os_task_parse_supervision_event(&message, &event));
    TEST_ASSERT_EQUAL(OS_TASK_SUPERVISION_SUSPEND, event.action);
    cpu.eax = SYS_TASK_RESUME;
    cpu.ebx = (uint32_t)child->id;
    syscall_handler(&cpu);
    TEST_ASSERT_EQUAL(0, (int)cpu.eax);
    TEST_ASSERT_EQUAL(0, parent->ipc_endpoint.count);
    cpu.eax = SYS_TASK_SUPERVISION_NOTIFY_STATUS;
    cpu.ebx = (uint32_t)&status;
    syscall_handler(&cpu);
    TEST_ASSERT_EQUAL(0, (int)cpu.eax);
    TEST_ASSERT_EQUAL(1, status.enabled);
    TEST_ASSERT_EQUAL(OS_TASK_SUPERVISION_NOTIFY_SUSPEND, status.mask);

    task_queue = old_queue;
    current_task = old_current;
}

void test_sys_task_supervision_watchlist(void) {
    task_t* old_queue = task_queue;
    task_t* old_current = current_task;
    task_t* parent;
    task_t* child;
    task_t* outsider;
    os_task_supervision_watch_status_t status;
    os_ipc_message_t message;
    os_task_supervision_event_t event;
    cpu_state_t cpu = {0};

    task_queue = NULL;
    current_task = NULL;
    parent = create_task(dummy_task_function);
    child = create_task(dummy_task_function);
    outsider = create_task(dummy_task_function);
    TEST_ASSERT_NOT_NULL(parent);
    TEST_ASSERT_NOT_NULL(child);
    TEST_ASSERT_NOT_NULL(outsider);
    parent->type = TASK_TYPE_USER;
    child->type = TASK_TYPE_USER;
    outsider->type = TASK_TYPE_USER;
    child->parent_pid = parent->id;
    add_task_to_queue(parent);
    add_task_to_queue(child);
    add_task_to_queue(outsider);
    current_task = parent;

    cpu.eax = SYS_TASK_SUPERVISION_WATCH_STATUS;
    cpu.ebx = (uint32_t)&status;
    syscall_handler(&cpu);
    TEST_ASSERT_EQUAL(0, (int)cpu.eax);
    TEST_ASSERT_EQUAL(0, status.enabled);
    TEST_ASSERT_EQUAL(0, status.count);
    cpu.eax = SYS_TASK_SUPERVISION_WATCH;
    cpu.ebx = (uint32_t)outsider->id;
    cpu.ecx = 1U;
    syscall_handler(&cpu);
    TEST_ASSERT_EQUAL(OS_TASK_NOT_CHILD, (int)cpu.eax);
    cpu.eax = SYS_TASK_SUPERVISION_WATCH;
    cpu.ebx = (uint32_t)child->id;
    cpu.ecx = 1U;
    syscall_handler(&cpu);
    TEST_ASSERT_EQUAL(1, (int)cpu.eax);
    cpu.eax = SYS_TASK_SUPERVISION_NOTIFY;
    cpu.ebx = 1U;
    syscall_handler(&cpu);
    TEST_ASSERT_EQUAL(1, (int)cpu.eax);
    cpu.eax = SYS_TASK_SUSPEND;
    cpu.ebx = (uint32_t)child->id;
    syscall_handler(&cpu);
    TEST_ASSERT_EQUAL(0, (int)cpu.eax);
    TEST_ASSERT_EQUAL(1, parent->ipc_endpoint.count);
    message = parent->ipc_endpoint.messages[parent->ipc_endpoint.read_index];
    TEST_ASSERT_EQUAL(0, os_task_parse_supervision_event(&message, &event));
    TEST_ASSERT_EQUAL(child->id, event.child_pid);
    cpu.eax = SYS_TASK_SUPERVISION_WATCH;
    cpu.ebx = 0U;
    cpu.ecx = 0U;
    syscall_handler(&cpu);
    TEST_ASSERT_EQUAL(0, (int)cpu.eax);
    cpu.eax = SYS_TASK_SUPERVISION_WATCH_STATUS;
    cpu.ebx = (uint32_t)&status;
    syscall_handler(&cpu);
    TEST_ASSERT_EQUAL(0, (int)cpu.eax);
    TEST_ASSERT_EQUAL(0, status.enabled);
    TEST_ASSERT_EQUAL(0, status.count);

    task_queue = old_queue;
    current_task = old_current;
}

void test_sys_task_supervision_delivery_stats(void) {
    task_t* old_queue = task_queue;
    task_t* old_current = current_task;
    task_t* parent;
    task_t* child;
    os_task_supervision_delivery_stats_t stats;
    cpu_state_t cpu = {0};

    task_queue = NULL;
    current_task = NULL;
    parent = create_task(dummy_task_function);
    child = create_task(dummy_task_function);
    TEST_ASSERT_NOT_NULL(parent);
    TEST_ASSERT_NOT_NULL(child);
    parent->type = TASK_TYPE_USER;
    child->type = TASK_TYPE_USER;
    child->parent_pid = parent->id;
    add_task_to_queue(parent);
    add_task_to_queue(child);
    current_task = parent;

    cpu.eax = SYS_TASK_SUPERVISION_NOTIFY;
    cpu.ebx = 1U;
    syscall_handler(&cpu);
    TEST_ASSERT_EQUAL(1, (int)cpu.eax);
    cpu.eax = SYS_TASK_SUSPEND;
    cpu.ebx = (uint32_t)child->id;
    syscall_handler(&cpu);
    TEST_ASSERT_EQUAL(0, (int)cpu.eax);
    cpu.eax = SYS_TASK_SUPERVISION_DELIVERY_STATS;
    cpu.ebx = (uint32_t)&stats;
    syscall_handler(&cpu);
    TEST_ASSERT_EQUAL(0, (int)cpu.eax);
    TEST_ASSERT_EQUAL(1, stats.attempted);
    TEST_ASSERT_EQUAL(1, stats.delivered);
    TEST_ASSERT_EQUAL(0, stats.dropped);
    cpu.eax = SYS_TASK_SUPERVISION_DELIVERY_STATS_ACK;
    syscall_handler(&cpu);
    TEST_ASSERT_EQUAL(0, (int)cpu.eax);
    cpu.eax = SYS_TASK_SUPERVISION_DELIVERY_STATS;
    cpu.ebx = (uint32_t)&stats;
    syscall_handler(&cpu);
    TEST_ASSERT_EQUAL(0, (int)cpu.eax);
    TEST_ASSERT_EQUAL(0, stats.attempted);
    TEST_ASSERT_EQUAL(0, stats.delivered);
    TEST_ASSERT_EQUAL(0, stats.dropped);

    task_queue = old_queue;
    current_task = old_current;
}

void test_sys_task_supervision_event_replay(void) {
    task_t* old_queue = task_queue;
    task_t* old_current = current_task;
    task_t* parent;
    task_t* child;
    os_ipc_message_t message;
    os_task_supervision_event_t event;
    cpu_state_t cpu = {0};

    task_queue = NULL;
    current_task = NULL;
    parent = create_task(dummy_task_function);
    child = create_task(dummy_task_function);
    TEST_ASSERT_NOT_NULL(parent);
    TEST_ASSERT_NOT_NULL(child);
    parent->type = TASK_TYPE_USER;
    child->type = TASK_TYPE_USER;
    child->parent_pid = parent->id;
    add_task_to_queue(parent);
    add_task_to_queue(child);
    current_task = parent;

    cpu.eax = SYS_TASK_SUSPEND;
    cpu.ebx = (uint32_t)child->id;
    syscall_handler(&cpu);
    TEST_ASSERT_EQUAL(0, (int)cpu.eax);
    TEST_ASSERT_EQUAL(0, parent->ipc_endpoint.count);
    cpu.eax = SYS_TASK_SUPERVISION_EVENT_REPLAY;
    cpu.ebx = 1U;
    syscall_handler(&cpu);
    TEST_ASSERT_EQUAL(0, (int)cpu.eax);
    TEST_ASSERT_EQUAL(1, parent->ipc_endpoint.count);
    message = parent->ipc_endpoint.messages[parent->ipc_endpoint.read_index];
    TEST_ASSERT_EQUAL(0, os_task_parse_supervision_event(&message, &event));
    TEST_ASSERT_EQUAL(1, event.sequence);
    TEST_ASSERT_EQUAL(OS_TASK_SUPERVISION_SUSPEND, event.action);
    cpu.eax = SYS_TASK_SUPERVISION_EVENT_REPLAY;
    cpu.ebx = 2U;
    syscall_handler(&cpu);
    TEST_ASSERT_EQUAL(OS_TASK_NO_SUPERVISION_EVENT, (int)cpu.eax);

    task_queue = old_queue;
    current_task = old_current;
}

void test_sys_task_supervision_priority(void) {
    task_t* old_queue = task_queue;
    task_t* old_current = current_task;
    task_t* parent;
    task_t* child;
    os_task_supervision_priority_status_t status;
    cpu_state_t cpu = {0};
    task_queue = NULL; current_task = NULL;
    parent = create_task(dummy_task_function);
    child = create_task(dummy_task_function);
    TEST_ASSERT_NOT_NULL(parent); TEST_ASSERT_NOT_NULL(child);
    parent->type = TASK_TYPE_USER; child->type = TASK_TYPE_USER;
    child->parent_pid = parent->id;
    add_task_to_queue(parent); add_task_to_queue(child); current_task = parent;
    cpu.eax = SYS_TASK_SUPERVISION_PRIORITY_STATUS; cpu.ebx = (uint32_t)&status;
    syscall_handler(&cpu);
    TEST_ASSERT_EQUAL(0, (int)cpu.eax); TEST_ASSERT_EQUAL(-1, status.child_pid);
    cpu.eax = SYS_TASK_SUPERVISION_PRIORITY; cpu.ebx = (uint32_t)child->id;
    syscall_handler(&cpu);
    TEST_ASSERT_EQUAL(child->id, (int)cpu.eax);
    cpu.eax = SYS_TASK_SUPERVISION_PRIORITY_STATUS; cpu.ebx = (uint32_t)&status;
    syscall_handler(&cpu);
    TEST_ASSERT_EQUAL(0, (int)cpu.eax); TEST_ASSERT_EQUAL(child->id, status.child_pid);
    cpu.eax = SYS_TASK_SUPERVISION_PRIORITY; cpu.ebx = 0U;
    syscall_handler(&cpu);
    TEST_ASSERT_EQUAL(0, (int)cpu.eax);
    task_queue = old_queue; current_task = old_current;
}

void test_sys_task_supervision_notify_budget(void) {
    task_t* old_queue = task_queue;
    task_t* old_current = current_task;
    task_t* parent;
    task_t* child;
    os_task_supervision_notify_budget_status_t status;
    cpu_state_t cpu = {0};

    task_queue = NULL;
    current_task = NULL;
    parent = create_task(dummy_task_function);
    child = create_task(dummy_task_function);
    TEST_ASSERT_NOT_NULL(parent);
    TEST_ASSERT_NOT_NULL(child);
    parent->type = TASK_TYPE_USER;
    child->type = TASK_TYPE_USER;
    child->parent_pid = parent->id;
    add_task_to_queue(parent);
    add_task_to_queue(child);
    current_task = parent;

    cpu.eax = SYS_TASK_SUPERVISION_NOTIFY_BUDGET_STATUS;
    cpu.ebx = (uint32_t)&status;
    syscall_handler(&cpu);
    TEST_ASSERT_EQUAL(0, (int)cpu.eax);
    TEST_ASSERT_EQUAL(0, status.limit);
    TEST_ASSERT_EQUAL(0, status.used);

    cpu.eax = SYS_TASK_SUPERVISION_NOTIFY;
    cpu.ebx = 1U;
    syscall_handler(&cpu);
    TEST_ASSERT_EQUAL(1, (int)cpu.eax);
    cpu.eax = SYS_TASK_SUPERVISION_NOTIFY_BUDGET;
    cpu.ebx = 2U;
    syscall_handler(&cpu);
    TEST_ASSERT_EQUAL(0, (int)cpu.eax);
    cpu.eax = SYS_TASK_SUSPEND;
    cpu.ebx = (uint32_t)child->id;
    syscall_handler(&cpu);
    TEST_ASSERT_EQUAL(0, (int)cpu.eax);
    cpu.eax = SYS_TASK_SUPERVISION_NOTIFY_BUDGET_STATUS;
    cpu.ebx = (uint32_t)&status;
    syscall_handler(&cpu);
    TEST_ASSERT_EQUAL(0, (int)cpu.eax);
    TEST_ASSERT_EQUAL(2, status.limit);
    TEST_ASSERT_EQUAL(1, status.used);

    cpu.eax = SYS_TASK_SUPERVISION_NOTIFY_BUDGET;
    cpu.ebx = 0U;
    syscall_handler(&cpu);
    TEST_ASSERT_EQUAL(0, (int)cpu.eax);
    cpu.eax = SYS_TASK_SUPERVISION_NOTIFY_BUDGET_STATUS;
    cpu.ebx = (uint32_t)&status;
    syscall_handler(&cpu);
    TEST_ASSERT_EQUAL(0, (int)cpu.eax);
    TEST_ASSERT_EQUAL(0, status.limit);
    TEST_ASSERT_EQUAL(0, status.used);

    task_queue = old_queue;
    current_task = old_current;
}

void test_sys_fat16_syscalls(void) {
    task_t* old_queue = task_queue;
    task_t* old_current = current_task;
    task_t* task;
    cpu_state_t cpu = {0};
    os_fat16_dirent_t entries[2];
    char buffer[16];

    task_queue = NULL;
    current_task = NULL;
    task = create_task(dummy_task_function);
    TEST_ASSERT_NOT_NULL(task);
    task->type = TASK_TYPE_USER;
    add_task_to_queue(task);
    current_task = task;

    cpu.eax = SYS_FAT16_LIST;
    cpu.ebx = (uint32_t)entries;
    cpu.ecx = 2U;
    syscall_handler(&cpu);
    TEST_ASSERT_EQUAL(OS_FAT16_NOT_MOUNTED, (int)cpu.eax);

    cpu.eax = SYS_FAT16_READ;
    cpu.ebx = (uint32_t)"FATOK.TXT";
    cpu.ecx = (uint32_t)buffer;
    cpu.edx = sizeof(buffer);
    syscall_handler(&cpu);
    TEST_ASSERT_EQUAL(OS_FAT16_NOT_MOUNTED, (int)cpu.eax);

    task_queue = old_queue;
    current_task = old_current;
}

void test_sys_task_supervision_summary(void) {
    task_t* old_queue = task_queue;
    task_t* old_current = current_task;
    task_t* parent;
    task_t* first;
    task_t* second;
    os_task_supervision_summary_t summary;
    cpu_state_t cpu = {0};

    task_queue = NULL;
    current_task = NULL;
    parent = create_task(dummy_task_function);
    first = create_task(dummy_task_function);
    second = create_task(dummy_task_function);
    TEST_ASSERT_NOT_NULL(parent);
    TEST_ASSERT_NOT_NULL(first);
    TEST_ASSERT_NOT_NULL(second);
    parent->type = TASK_TYPE_USER;
    first->type = TASK_TYPE_USER;
    second->type = TASK_TYPE_USER;
    first->parent_pid = parent->id;
    second->parent_pid = parent->id;
    add_task_to_queue(parent);
    add_task_to_queue(first);
    add_task_to_queue(second);
    current_task = parent;

    cpu.eax = SYS_TASK_SUSPEND;
    cpu.ebx = (uint32_t)first->id;
    syscall_handler(&cpu);
    TEST_ASSERT_EQUAL(0, (int)cpu.eax);
    cpu.eax = SYS_TASK_SUPERVISION_SUMMARY;
    cpu.ebx = (uint32_t)&summary;
    syscall_handler(&cpu);
    TEST_ASSERT_EQUAL(0, (int)cpu.eax);
    TEST_ASSERT_EQUAL(1, summary.generation);
    TEST_ASSERT_EQUAL(2, summary.active_children);
    TEST_ASSERT_EQUAL(1, summary.suspended_children);
    TEST_ASSERT_EQUAL(0, summary.child_exit_count);
    TEST_ASSERT_EQUAL(1, summary.retained_events);
    TEST_ASSERT_EQUAL(0, task_kill(parent->id, second->id));
    cpu.eax = SYS_TASK_SUPERVISION_SUMMARY;
    cpu.ebx = (uint32_t)&summary;
    syscall_handler(&cpu);
    TEST_ASSERT_EQUAL(0, (int)cpu.eax);
    TEST_ASSERT_EQUAL(2, summary.generation);
    TEST_ASSERT_EQUAL(1, summary.active_children);
    TEST_ASSERT_EQUAL(1, summary.suspended_children);
    TEST_ASSERT_EQUAL(1, summary.child_exit_count);
    TEST_ASSERT_EQUAL(2, summary.retained_events);

    task_queue = old_queue;
    current_task = old_current;
}

void test_sys_task_wait_child(void) {
    task_t* old_queue = task_queue;
    task_t* old_current = current_task;
    task_t* parent;
    task_t* child;
    task_t* unrelated;
    cpu_state_t cpu = {0};

    task_queue = NULL;
    current_task = NULL;
    parent = create_task(dummy_task_function);
    child = create_task(dummy_task_function);
    unrelated = create_task(dummy_task_function);
    child->parent_pid = parent->id;
    add_task_to_queue(parent);
    add_task_to_queue(child);
    add_task_to_queue(unrelated);
    current_task = parent;

    cpu.eax = SYS_TASK_WAIT;
    cpu.ebx = (uint32_t)child->id;
    syscall_handler(&cpu);
    TEST_ASSERT_EQUAL(0, (int)cpu.eax);
    TEST_ASSERT_EQUAL(TASK_WAITING, parent->state);
    TEST_ASSERT_EQUAL(parent->id, child->waiter_pid);
    task_wake_waiter(child);
    TEST_ASSERT_EQUAL(TASK_READY, parent->state);

    current_task = unrelated;
    cpu.eax = SYS_TASK_WAIT;
    cpu.ebx = (uint32_t)child->id;
    syscall_handler(&cpu);
    TEST_ASSERT_EQUAL(OS_TASK_NOT_CHILD, (int)cpu.eax);

    task_queue = old_queue;
    current_task = old_current;
}

void test_sys_ps_lists_task(void) {
    task_t* old_queue = task_queue;
    task_t* old_current = current_task;
    task_t* t;
    os_proc_t procs[8];
    cpu_state_t cpu = {0};
    int n;
    int found = 0;

    task_queue = NULL;
    current_task = NULL;
    t = create_task(dummy_task_function);
    TEST_ASSERT_NOT_NULL(t);
    t->id = 3;
    t->type = TASK_TYPE_USER;
    t->name[0] = 's';
    t->name[1] = 'h';
    t->name[2] = 'e';
    t->name[3] = 'l';
    t->name[4] = 'l';
    t->name[5] = '\0';
    add_task_to_queue(t);
    current_task = t;

    cpu.eax = SYS_PS;
    cpu.ebx = (uint32_t)procs;
    cpu.ecx = 8;
    syscall_handler(&cpu);
    n = (int)cpu.eax;
    TEST_ASSERT(n >= 1);
    for (int i = 0; i < n; i++) {
        if (procs[i].pid == 3) {
            found = 1;
            TEST_ASSERT_EQUAL(-1, procs[i].parent_pid);
            TEST_ASSERT_EQUAL_STRING("shell", procs[i].name);
        }
    }
    TEST_ASSERT(found);

    task_queue = old_queue;
    current_task = old_current;
}

void test_sys_kill_unknown_pid(void) {
    cpu_state_t cpu = {0};
    cpu.eax = SYS_KILL;
    cpu.ebx = 9999;
    syscall_handler(&cpu);
    TEST_ASSERT_EQUAL(-1, (int)cpu.eax);
}

void test_sys_kill_removes_ready_task(void) {
    task_t* old_queue = task_queue;
    task_t* old_current = current_task;
    task_t* shell;
    task_t* child;
    task_t* unrelated;
    os_proc_t procs[8];
    cpu_state_t cpu = {0};
    int n;
    int found_child;

    task_queue = NULL;
    current_task = NULL;
    shell = create_task(dummy_task_function);
    child = create_task(dummy_task_function);
    TEST_ASSERT_NOT_NULL(shell);
    TEST_ASSERT_NOT_NULL(child);
    shell->id = 1;
    child->id = 2;
    child->parent_pid = shell->id;
    child->type = TASK_TYPE_USER;
    child->name[0] = 'i';
    child->name[1] = 'd';
    child->name[2] = 'l';
    child->name[3] = 'e';
    child->name[4] = '\0';
    add_task_to_queue(shell);
    add_task_to_queue(child);
    unrelated = create_task(dummy_task_function);
    TEST_ASSERT_NOT_NULL(unrelated);
    unrelated->id = 3;
    unrelated->type = TASK_TYPE_USER;
    add_task_to_queue(unrelated);
    current_task = shell;

    cpu.eax = SYS_KILL;
    cpu.ebx = 3;
    syscall_handler(&cpu);
    TEST_ASSERT_EQUAL(OS_TASK_CONTROL_DENIED, (int)cpu.eax);

    cpu.eax = SYS_KILL;
    cpu.ebx = 2;
    syscall_handler(&cpu);
    TEST_ASSERT_EQUAL(0, (int)cpu.eax);

    cpu.eax = SYS_PS;
    cpu.ebx = (uint32_t)procs;
    cpu.ecx = 8;
    syscall_handler(&cpu);
    n = (int)cpu.eax;
    found_child = 0;
    for (int i = 0; i < n; i++) {
        if (procs[i].pid == 2) found_child = 1;
    }
    TEST_ASSERT_EQUAL(0, found_child);

    task_queue = old_queue;
    current_task = old_current;
}

void test_sys_overlay_mkdir_listdir_unlink(void) {
    os_dirent_t ents[16];
    os_dirent_t st;
    cpu_state_t cpu = {0};
    int n;
    int found;
    int i;

    overlay_init();

    cpu.eax = SYS_MKDIR;
    cpu.ebx = (uint32_t)"mydir";
    syscall_handler(&cpu);
    TEST_ASSERT_EQUAL(0, (int)cpu.eax);

    cpu.eax = SYS_STAT;
    cpu.ebx = (uint32_t)"mydir";
    cpu.ecx = (uint32_t)&st;
    syscall_handler(&cpu);
    TEST_ASSERT_EQUAL(0, (int)cpu.eax);
    TEST_ASSERT_EQUAL(OS_DIRENT_DIR, (int)st.flags);

    cpu.eax = SYS_LISTDIR;
    cpu.ebx = (uint32_t)"/";
    cpu.ecx = (uint32_t)ents;
    cpu.edx = 16;
    syscall_handler(&cpu);
    n = (int)cpu.eax;
    found = 0;
    TEST_ASSERT(n > 0);
    for (i = 0; i < n; i++) {
        if (strcmp(ents[i].name, "mydir") == 0 && ents[i].flags == OS_DIRENT_DIR) {
            found = 1;
        }
    }
    TEST_ASSERT(found);

    cpu.eax = SYS_LISTDIR;
    cpu.ebx = (uint32_t)"mydir";
    cpu.ecx = (uint32_t)ents;
    cpu.edx = 16;
    syscall_handler(&cpu);
    TEST_ASSERT_EQUAL(0, (int)cpu.eax);

    cpu.eax = SYS_UNLINK;
    cpu.ebx = (uint32_t)"mydir";
    syscall_handler(&cpu);
    TEST_ASSERT_EQUAL(0, (int)cpu.eax);

    cpu.eax = SYS_LISTDIR;
    cpu.ebx = (uint32_t)"/";
    cpu.ecx = (uint32_t)ents;
    cpu.edx = 16;
    syscall_handler(&cpu);
    n = (int)cpu.eax;
    found = 0;
    for (i = 0; i < n; i++) {
        if (strcmp(ents[i].name, "mydir") == 0) found = 1;
    }
    TEST_ASSERT_EQUAL(0, found);
}

void test_sys_overlay_write_read(void) {
    char buf[64];
    cpu_state_t cpu = {0};
    const char* payload = "hello overlay\n";

    overlay_init();

    cpu.eax = SYS_WRITEFILE;
    cpu.ebx = (uint32_t)"notes.txt";
    cpu.ecx = (uint32_t)payload;
    cpu.edx = 14;
    syscall_handler(&cpu);
    TEST_ASSERT_EQUAL(14, (int)cpu.eax);

    cpu.eax = SYS_READFILE;
    cpu.ebx = (uint32_t)"notes.txt";
    cpu.ecx = (uint32_t)buf;
    cpu.edx = sizeof(buf);
    syscall_handler(&cpu);
    TEST_ASSERT_EQUAL(14, (int)cpu.eax);
    buf[14] = '\0';
    TEST_ASSERT_EQUAL_STRING("hello overlay\n", buf);

    cpu.eax = SYS_UNLINK;
    cpu.ebx = (uint32_t)"notes.txt";
    syscall_handler(&cpu);
    TEST_ASSERT_EQUAL(0, (int)cpu.eax);
}

void test_sys_overlay_write_empty(void) {
    os_dirent_t st;
    char buf[8];
    cpu_state_t cpu = {0};

    overlay_init();

    cpu.eax = SYS_WRITEFILE;
    cpu.ebx = (uint32_t)"empty.txt";
    cpu.ecx = (uint32_t)"";
    cpu.edx = 0;
    syscall_handler(&cpu);
    TEST_ASSERT_EQUAL(0, (int)cpu.eax);

    cpu.eax = SYS_STAT;
    cpu.ebx = (uint32_t)"empty.txt";
    cpu.ecx = (uint32_t)&st;
    syscall_handler(&cpu);
    TEST_ASSERT_EQUAL(0, (int)cpu.eax);
    TEST_ASSERT_EQUAL(OS_DIRENT_FILE, (int)st.flags);
    TEST_ASSERT_EQUAL(0, (int)st.size);

    cpu.eax = SYS_READFILE;
    cpu.ebx = (uint32_t)"empty.txt";
    cpu.ecx = (uint32_t)buf;
    cpu.edx = sizeof(buf);
    syscall_handler(&cpu);
    TEST_ASSERT_EQUAL(0, (int)cpu.eax);
}

void test_sys_overlay_append(void) {
    char buf[64];
    os_dirent_t st;
    cpu_state_t cpu = {0};
    char big[OV_SNAP_DATA];
    int i;

    overlay_init();

    cpu.eax = SYS_WRITEFILE;
    cpu.ebx = (uint32_t)"notes.txt";
    cpu.ecx = (uint32_t)"hello\n";
    cpu.edx = 6;
    syscall_handler(&cpu);
    TEST_ASSERT_EQUAL(6, (int)cpu.eax);

    cpu.eax = SYS_APPEND;
    cpu.ebx = (uint32_t)"notes.txt";
    cpu.ecx = (uint32_t)"world\n";
    cpu.edx = 6;
    syscall_handler(&cpu);
    TEST_ASSERT_EQUAL(6, (int)cpu.eax);

    cpu.eax = SYS_READFILE;
    cpu.ebx = (uint32_t)"notes.txt";
    cpu.ecx = (uint32_t)buf;
    cpu.edx = sizeof(buf);
    syscall_handler(&cpu);
    TEST_ASSERT_EQUAL(12, (int)cpu.eax);
    buf[12] = '\0';
    TEST_ASSERT_EQUAL_STRING("hello\nworld\n", buf);

    cpu.eax = SYS_APPEND;
    cpu.ebx = (uint32_t)"new.txt";
    cpu.ecx = (uint32_t)"x\n";
    cpu.edx = 2;
    syscall_handler(&cpu);
    TEST_ASSERT_EQUAL(2, (int)cpu.eax);

    cpu.eax = SYS_STAT;
    cpu.ebx = (uint32_t)"new.txt";
    cpu.ecx = (uint32_t)&st;
    syscall_handler(&cpu);
    TEST_ASSERT_EQUAL(0, (int)cpu.eax);
    TEST_ASSERT_EQUAL(OS_DIRENT_FILE, (int)st.flags);
    TEST_ASSERT_EQUAL(2, (int)st.size);

    cpu.eax = SYS_MKDIR;
    cpu.ebx = (uint32_t)"mydir";
    syscall_handler(&cpu);
    TEST_ASSERT_EQUAL(0, (int)cpu.eax);

    cpu.eax = SYS_APPEND;
    cpu.ebx = (uint32_t)"mydir";
    cpu.ecx = (uint32_t)"x";
    cpu.edx = 1;
    syscall_handler(&cpu);
    TEST_ASSERT_EQUAL(OV_ERR_ISDIR, (int)cpu.eax);

    cpu.eax = SYS_APPEND;
    cpu.ebx = (uint32_t)"hello.txt";
    cpu.ecx = (uint32_t)"Z";
    cpu.edx = 1;
    syscall_handler(&cpu);
    TEST_ASSERT_EQUAL(1, (int)cpu.eax);

    cpu.eax = SYS_READFILE;
    cpu.ebx = (uint32_t)"hello.txt";
    cpu.ecx = (uint32_t)buf;
    cpu.edx = sizeof(buf);
    syscall_handler(&cpu);
    TEST_ASSERT_EQUAL(19, (int)cpu.eax);
    buf[19] = '\0';
    TEST_ASSERT_EQUAL_STRING("hello from initrd\nZ", buf);

    for (i = 0; i < (int)OV_SNAP_DATA; i++) big[i] = 'a';
    cpu.eax = SYS_WRITEFILE;
    cpu.ebx = (uint32_t)"full.txt";
    cpu.ecx = (uint32_t)big;
    cpu.edx = OV_SNAP_DATA;
    syscall_handler(&cpu);
    TEST_ASSERT_EQUAL((int)OV_SNAP_DATA, (int)cpu.eax);

    cpu.eax = SYS_APPEND;
    cpu.ebx = (uint32_t)"full.txt";
    cpu.ecx = (uint32_t)"b";
    cpu.edx = 1;
    syscall_handler(&cpu);
    TEST_ASSERT_EQUAL(OV_ERR_NOSPACE, (int)cpu.eax);
}

void test_sys_overlay_protects_initrd(void) {
    cpu_state_t cpu = {0};
    overlay_init();

    cpu.eax = SYS_UNLINK;
    cpu.ebx = (uint32_t)"hello.txt";
    syscall_handler(&cpu);
    TEST_ASSERT_EQUAL(OV_ERR_PROTECTED, (int)cpu.eax);

    cpu.eax = SYS_MKDIR;
    cpu.ebx = (uint32_t)"no/such/dir";
    syscall_handler(&cpu);
    TEST_ASSERT_EQUAL(OV_ERR_NOTDIR, (int)cpu.eax);
}

void test_sys_stat_initrd_file_and_overlay_dir(void) {
    os_dirent_t st;
    cpu_state_t cpu = {0};

    overlay_init();

    cpu.eax = SYS_STAT;
    cpu.ebx = (uint32_t)"hello.txt";
    cpu.ecx = (uint32_t)&st;
    syscall_handler(&cpu);
    TEST_ASSERT_EQUAL(0, (int)cpu.eax);
    TEST_ASSERT_EQUAL(OS_DIRENT_FILE, (int)st.flags);
    TEST_ASSERT_EQUAL(18, (int)st.size);

    cpu.eax = SYS_MKDIR;
    cpu.ebx = (uint32_t)"mydir";
    syscall_handler(&cpu);
    TEST_ASSERT_EQUAL(0, (int)cpu.eax);

    cpu.eax = SYS_STAT;
    cpu.ebx = (uint32_t)"mydir";
    cpu.ecx = (uint32_t)&st;
    syscall_handler(&cpu);
    TEST_ASSERT_EQUAL(0, (int)cpu.eax);
    TEST_ASSERT_EQUAL(OS_DIRENT_DIR, (int)st.flags);
}

void test_sys_stat_missing(void) {
    os_dirent_t st;
    cpu_state_t cpu = {0};

    overlay_init();

    cpu.eax = SYS_STAT;
    cpu.ebx = (uint32_t)"nosuch";
    cpu.ecx = (uint32_t)&st;
    syscall_handler(&cpu);
    TEST_ASSERT((int)cpu.eax != 0);
}

void test_sys_overlay_copy_from_initrd(void) {
    char src[64];
    char dst[64];
    os_dirent_t ents[16];
    cpu_state_t cpu = {0};
    int n;
    int found;
    int i;

    overlay_init();

    cpu.eax = SYS_READFILE;
    cpu.ebx = (uint32_t)"hello.txt";
    cpu.ecx = (uint32_t)src;
    cpu.edx = sizeof(src);
    syscall_handler(&cpu);
    TEST_ASSERT_EQUAL(18, (int)cpu.eax);

    cpu.eax = SYS_WRITEFILE;
    cpu.ebx = (uint32_t)"copy.txt";
    cpu.ecx = (uint32_t)src;
    cpu.edx = 18;
    syscall_handler(&cpu);
    TEST_ASSERT_EQUAL(18, (int)cpu.eax);

    cpu.eax = SYS_LISTDIR;
    cpu.ebx = (uint32_t)"/";
    cpu.ecx = (uint32_t)ents;
    cpu.edx = 16;
    syscall_handler(&cpu);
    n = (int)cpu.eax;
    found = 0;
    for (i = 0; i < n; i++) {
        if (strcmp(ents[i].name, "copy.txt") == 0) found = 1;
    }
    TEST_ASSERT(found);

    cpu.eax = SYS_READFILE;
    cpu.ebx = (uint32_t)"copy.txt";
    cpu.ecx = (uint32_t)dst;
    cpu.edx = sizeof(dst);
    syscall_handler(&cpu);
    TEST_ASSERT_EQUAL(18, (int)cpu.eax);
    dst[18] = '\0';
    TEST_ASSERT_EQUAL_STRING("hello from initrd\n", dst);

    cpu.eax = SYS_UNLINK;
    cpu.ebx = (uint32_t)"copy.txt";
    syscall_handler(&cpu);
    TEST_ASSERT_EQUAL(0, (int)cpu.eax);
}

void test_sys_overlay_nested_mkdir_notempty(void) {
    char payload[] = "nested\n";
    os_dirent_t ents[16];
    cpu_state_t cpu = {0};
    int n;
    int found;
    int i;

    overlay_init();

    cpu.eax = SYS_MKDIR;
    cpu.ebx = (uint32_t)"mydir";
    syscall_handler(&cpu);
    TEST_ASSERT_EQUAL(0, (int)cpu.eax);

    cpu.eax = SYS_MKDIR;
    cpu.ebx = (uint32_t)"mydir/sub";
    syscall_handler(&cpu);
    TEST_ASSERT_EQUAL(0, (int)cpu.eax);

    cpu.eax = SYS_WRITEFILE;
    cpu.ebx = (uint32_t)"mydir/sub/a.txt";
    cpu.ecx = (uint32_t)payload;
    cpu.edx = 7;
    syscall_handler(&cpu);
    TEST_ASSERT_EQUAL(7, (int)cpu.eax);

    cpu.eax = SYS_LISTDIR;
    cpu.ebx = (uint32_t)"mydir";
    cpu.ecx = (uint32_t)ents;
    cpu.edx = 16;
    syscall_handler(&cpu);
    n = (int)cpu.eax;
    found = 0;
    for (i = 0; i < n; i++) {
        if (strcmp(ents[i].name, "sub") == 0 && ents[i].flags == OS_DIRENT_DIR) {
            found = 1;
        }
    }
    TEST_ASSERT(found);

    cpu.eax = SYS_UNLINK;
    cpu.ebx = (uint32_t)"mydir";
    syscall_handler(&cpu);
    TEST_ASSERT_EQUAL(OV_ERR_NOTEMPTY, (int)cpu.eax);

    cpu.eax = SYS_UNLINK;
    cpu.ebx = (uint32_t)"mydir/sub";
    syscall_handler(&cpu);
    TEST_ASSERT_EQUAL(OV_ERR_NOTEMPTY, (int)cpu.eax);

    cpu.eax = SYS_UNLINK;
    cpu.ebx = (uint32_t)"mydir/sub/a.txt";
    syscall_handler(&cpu);
    TEST_ASSERT_EQUAL(0, (int)cpu.eax);

    cpu.eax = SYS_UNLINK;
    cpu.ebx = (uint32_t)"mydir/sub";
    syscall_handler(&cpu);
    TEST_ASSERT_EQUAL(0, (int)cpu.eax);

    cpu.eax = SYS_UNLINK;
    cpu.ebx = (uint32_t)"mydir";
    syscall_handler(&cpu);
    TEST_ASSERT_EQUAL(0, (int)cpu.eax);
}

void test_sys_overlay_rename_file_and_dir(void) {
    char payload[] = "renamed\n";
    char buf[32];
    os_dirent_t ents[16];
    os_dirent_t st;
    cpu_state_t cpu = {0};
    int n;
    int found_old;
    int found_new;
    int found_child;
    int i;

    overlay_init();

    cpu.eax = SYS_WRITEFILE;
    cpu.ebx = (uint32_t)"a.txt";
    cpu.ecx = (uint32_t)payload;
    cpu.edx = 8;
    syscall_handler(&cpu);
    TEST_ASSERT_EQUAL(8, (int)cpu.eax);

    cpu.eax = SYS_RENAME;
    cpu.ebx = (uint32_t)"a.txt";
    cpu.ecx = (uint32_t)"b.txt";
    syscall_handler(&cpu);
    TEST_ASSERT_EQUAL(0, (int)cpu.eax);

    cpu.eax = SYS_READFILE;
    cpu.ebx = (uint32_t)"b.txt";
    cpu.ecx = (uint32_t)buf;
    cpu.edx = sizeof(buf);
    syscall_handler(&cpu);
    TEST_ASSERT_EQUAL(8, (int)cpu.eax);
    buf[8] = '\0';
    TEST_ASSERT_EQUAL_STRING("renamed\n", buf);

    cpu.eax = SYS_STAT;
    cpu.ebx = (uint32_t)"a.txt";
    cpu.ecx = (uint32_t)&st;
    syscall_handler(&cpu);
    TEST_ASSERT_EQUAL(-1, (int)cpu.eax);

    cpu.eax = SYS_MKDIR;
    cpu.ebx = (uint32_t)"oldd";
    syscall_handler(&cpu);
    TEST_ASSERT_EQUAL(0, (int)cpu.eax);

    cpu.eax = SYS_MKDIR;
    cpu.ebx = (uint32_t)"oldd/sub";
    syscall_handler(&cpu);
    TEST_ASSERT_EQUAL(0, (int)cpu.eax);

    cpu.eax = SYS_WRITEFILE;
    cpu.ebx = (uint32_t)"oldd/sub/f.txt";
    cpu.ecx = (uint32_t)payload;
    cpu.edx = 8;
    syscall_handler(&cpu);
    TEST_ASSERT_EQUAL(8, (int)cpu.eax);

    cpu.eax = SYS_RENAME;
    cpu.ebx = (uint32_t)"oldd";
    cpu.ecx = (uint32_t)"newd";
    syscall_handler(&cpu);
    TEST_ASSERT_EQUAL(0, (int)cpu.eax);

    cpu.eax = SYS_STAT;
    cpu.ebx = (uint32_t)"newd";
    cpu.ecx = (uint32_t)&st;
    syscall_handler(&cpu);
    TEST_ASSERT_EQUAL(0, (int)cpu.eax);
    TEST_ASSERT_EQUAL(OS_DIRENT_DIR, (int)st.flags);

    cpu.eax = SYS_STAT;
    cpu.ebx = (uint32_t)"oldd";
    cpu.ecx = (uint32_t)&st;
    syscall_handler(&cpu);
    TEST_ASSERT_EQUAL(-1, (int)cpu.eax);

    cpu.eax = SYS_LISTDIR;
    cpu.ebx = (uint32_t)"/";
    cpu.ecx = (uint32_t)ents;
    cpu.edx = 16;
    syscall_handler(&cpu);
    n = (int)cpu.eax;
    found_old = 0;
    found_new = 0;
    for (i = 0; i < n; i++) {
        if (strcmp(ents[i].name, "oldd") == 0) found_old = 1;
        if (strcmp(ents[i].name, "newd") == 0) found_new = 1;
    }
    TEST_ASSERT_EQUAL(0, found_old);
    TEST_ASSERT(found_new);

    cpu.eax = SYS_LISTDIR;
    cpu.ebx = (uint32_t)"newd";
    cpu.ecx = (uint32_t)ents;
    cpu.edx = 16;
    syscall_handler(&cpu);
    n = (int)cpu.eax;
    found_child = 0;
    for (i = 0; i < n; i++) {
        if (strcmp(ents[i].name, "sub") == 0 && ents[i].flags == OS_DIRENT_DIR) {
            found_child = 1;
        }
    }
    TEST_ASSERT(found_child);

    cpu.eax = SYS_READFILE;
    cpu.ebx = (uint32_t)"newd/sub/f.txt";
    cpu.ecx = (uint32_t)buf;
    cpu.edx = sizeof(buf);
    syscall_handler(&cpu);
    TEST_ASSERT_EQUAL(8, (int)cpu.eax);
    buf[8] = '\0';
    TEST_ASSERT_EQUAL_STRING("renamed\n", buf);

    cpu.eax = SYS_RENAME;
    cpu.ebx = (uint32_t)"hello.txt";
    cpu.ecx = (uint32_t)"moved.txt";
    syscall_handler(&cpu);
    TEST_ASSERT_EQUAL(OV_ERR_PROTECTED, (int)cpu.eax);

    cpu.eax = SYS_RENAME;
    cpu.ebx = (uint32_t)"newd";
    cpu.ecx = (uint32_t)"newd/nested";
    syscall_handler(&cpu);
    TEST_ASSERT_EQUAL(OV_ERR_INVAL, (int)cpu.eax);
}

void test_sys_overlay_copy_dir_tree(void) {
    char payload[] = "copied\n";
    char buf[32];
    os_dirent_t ents[16];
    os_dirent_t st;
    cpu_state_t cpu = {0};
    int n;
    int found_old;
    int found_new;
    int found_child;
    int i;

    overlay_init();

    cpu.eax = SYS_MKDIR;
    cpu.ebx = (uint32_t)"oldd";
    syscall_handler(&cpu);
    TEST_ASSERT_EQUAL(0, (int)cpu.eax);

    cpu.eax = SYS_MKDIR;
    cpu.ebx = (uint32_t)"oldd/sub";
    syscall_handler(&cpu);
    TEST_ASSERT_EQUAL(0, (int)cpu.eax);

    cpu.eax = SYS_WRITEFILE;
    cpu.ebx = (uint32_t)"oldd/sub/f.txt";
    cpu.ecx = (uint32_t)payload;
    cpu.edx = 7;
    syscall_handler(&cpu);
    TEST_ASSERT_EQUAL(7, (int)cpu.eax);

    cpu.eax = SYS_COPY;
    cpu.ebx = (uint32_t)"oldd";
    cpu.ecx = (uint32_t)"newd";
    syscall_handler(&cpu);
    TEST_ASSERT_EQUAL(0, (int)cpu.eax);

    cpu.eax = SYS_STAT;
    cpu.ebx = (uint32_t)"oldd";
    cpu.ecx = (uint32_t)&st;
    syscall_handler(&cpu);
    TEST_ASSERT_EQUAL(0, (int)cpu.eax);
    TEST_ASSERT_EQUAL(OS_DIRENT_DIR, (int)st.flags);

    cpu.eax = SYS_STAT;
    cpu.ebx = (uint32_t)"newd";
    cpu.ecx = (uint32_t)&st;
    syscall_handler(&cpu);
    TEST_ASSERT_EQUAL(0, (int)cpu.eax);
    TEST_ASSERT_EQUAL(OS_DIRENT_DIR, (int)st.flags);

    cpu.eax = SYS_LISTDIR;
    cpu.ebx = (uint32_t)"/";
    cpu.ecx = (uint32_t)ents;
    cpu.edx = 16;
    syscall_handler(&cpu);
    n = (int)cpu.eax;
    found_old = 0;
    found_new = 0;
    for (i = 0; i < n; i++) {
        if (strcmp(ents[i].name, "oldd") == 0) found_old = 1;
        if (strcmp(ents[i].name, "newd") == 0) found_new = 1;
    }
    TEST_ASSERT(found_old);
    TEST_ASSERT(found_new);

    cpu.eax = SYS_LISTDIR;
    cpu.ebx = (uint32_t)"newd";
    cpu.ecx = (uint32_t)ents;
    cpu.edx = 16;
    syscall_handler(&cpu);
    n = (int)cpu.eax;
    found_child = 0;
    for (i = 0; i < n; i++) {
        if (strcmp(ents[i].name, "sub") == 0 && ents[i].flags == OS_DIRENT_DIR) {
            found_child = 1;
        }
    }
    TEST_ASSERT(found_child);

    cpu.eax = SYS_READFILE;
    cpu.ebx = (uint32_t)"newd/sub/f.txt";
    cpu.ecx = (uint32_t)buf;
    cpu.edx = sizeof(buf);
    syscall_handler(&cpu);
    TEST_ASSERT_EQUAL(7, (int)cpu.eax);
    buf[7] = '\0';
    TEST_ASSERT_EQUAL_STRING("copied\n", buf);

    cpu.eax = SYS_READFILE;
    cpu.ebx = (uint32_t)"oldd/sub/f.txt";
    cpu.ecx = (uint32_t)buf;
    cpu.edx = sizeof(buf);
    syscall_handler(&cpu);
    TEST_ASSERT_EQUAL(7, (int)cpu.eax);

    cpu.eax = SYS_COPY;
    cpu.ebx = (uint32_t)"hello.txt";
    cpu.ecx = (uint32_t)"from-initrd.txt";
    syscall_handler(&cpu);
    TEST_ASSERT_EQUAL(OV_ERR_PROTECTED, (int)cpu.eax);

    cpu.eax = SYS_COPY;
    cpu.ebx = (uint32_t)"oldd";
    cpu.ecx = (uint32_t)"oldd/nested";
    syscall_handler(&cpu);
    TEST_ASSERT_EQUAL(OV_ERR_INVAL, (int)cpu.eax);

    cpu.eax = SYS_COPY;
    cpu.ebx = (uint32_t)"oldd";
    cpu.ecx = (uint32_t)"newd";
    syscall_handler(&cpu);
    TEST_ASSERT_EQUAL(OV_ERR_EXISTS, (int)cpu.eax);
}

// === RUNNER PRINCIPAL ===

int main(void) {
    unity_init();
    
    // Tests d'initialisation
    RUN_TEST(test_syscall_init_basic);
    
    // Tests des syscalls individuels
    RUN_TEST(test_sys_putc_single_character);
    RUN_TEST(test_sys_putc_multiple_characters);
    RUN_TEST(test_sys_putc_special_characters);
    RUN_TEST(test_sys_puts_basic_string);
    RUN_TEST(test_sys_puts_empty_string);
    RUN_TEST(test_sys_puts_null_pointer);
    RUN_TEST(test_sys_getc_single_character);
    RUN_TEST(test_sys_getc_multiple_calls);
    RUN_TEST(test_sys_getc_no_input);
    RUN_TEST(test_sys_gets_basic_input);
    RUN_TEST(test_sys_gets_buffer_size_limit);
    RUN_TEST(test_sys_gets_null_buffer);
    RUN_TEST(test_sys_exit_with_code);
    RUN_TEST(test_sys_exit_zero_code);
    
    // Tests du handler
    RUN_TEST(test_syscall_handler_putc);
    RUN_TEST(test_syscall_handler_exit);
    RUN_TEST(test_syscall_handler_invalid_syscall);
    RUN_TEST(test_syscall_handler_boundary_syscall_numbers);
    
    // Tests de performance
    RUN_TEST(test_syscall_performance_putc);
    RUN_TEST(test_syscall_performance_getc);
    RUN_TEST(test_syscall_handler_performance);
    
    // Tests de robustesse
    RUN_TEST(test_syscall_with_corrupted_state);
    RUN_TEST(test_syscall_buffer_overflow_protection);
    RUN_TEST(test_syscall_parameter_validation);
    
    // Tests d'intégration
    RUN_TEST(test_syscall_integration_with_tasks);
    RUN_TEST(test_syscall_exec_basic);
    RUN_TEST(test_syscall_yield_integration);
    
    // Tests de sécurité
    RUN_TEST(test_syscall_ring_isolation);
    RUN_TEST(test_syscall_privilege_escalation_prevention);
    RUN_TEST(test_sys_getpid_and_ticks);
    RUN_TEST(test_sys_gpt2_gguf_continue_requires_session);
    RUN_TEST(test_sys_readfile_and_listdir);
    RUN_TEST(test_sys_kill_protects_kernel);
    RUN_TEST(test_sys_meminfo);
    RUN_TEST(test_sys_task_priority_and_metrics);
    RUN_TEST(test_sys_task_name_and_capacity);
    RUN_TEST(test_sys_task_delegate_child);
    RUN_TEST(test_sys_task_supervision_events);
    RUN_TEST(test_sys_task_supervision_event_selective);
    RUN_TEST(test_sys_task_supervision_notify);
    RUN_TEST(test_sys_task_supervision_notify_policy);
    RUN_TEST(test_sys_task_supervision_watchlist);
    RUN_TEST(test_sys_task_supervision_delivery_stats);
    RUN_TEST(test_sys_task_supervision_event_replay);
    RUN_TEST(test_sys_task_supervision_priority);
    RUN_TEST(test_sys_task_supervision_notify_budget);
    RUN_TEST(test_sys_fat16_syscalls);
    RUN_TEST(test_sys_task_supervision_summary);
    RUN_TEST(test_sys_task_wait_child);
    RUN_TEST(test_sys_ps_lists_task);
    RUN_TEST(test_sys_kill_unknown_pid);
    RUN_TEST(test_sys_kill_removes_ready_task);
    RUN_TEST(test_sys_overlay_mkdir_listdir_unlink);
    RUN_TEST(test_sys_overlay_write_read);
    RUN_TEST(test_sys_overlay_write_empty);
    RUN_TEST(test_sys_overlay_append);
    RUN_TEST(test_sys_overlay_protects_initrd);
    RUN_TEST(test_sys_stat_initrd_file_and_overlay_dir);
    RUN_TEST(test_sys_stat_missing);
    RUN_TEST(test_sys_overlay_copy_from_initrd);
    RUN_TEST(test_sys_overlay_nested_mkdir_notempty);
    RUN_TEST(test_sys_overlay_rename_file_and_dir);
    RUN_TEST(test_sys_overlay_copy_dir_tree);
    
    unity_print_results();
    unity_cleanup();
    
    return (unity_stats.tests_failed == 0) ? 0 : 1;
}
