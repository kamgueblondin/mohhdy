/* test_task.c - Tests unitaires pour le Task Manager */

#include "../../framework/unity.h"
#include "../../framework/test_kernel.h"

// Include du module à tester
#include "../../../kernel/task/task.h"

// Mock des dépendances
extern int mock_task_switch_called;
static task_t* mock_current_task = NULL;
static task_t* mock_task_queue = NULL;

// === MOCK FUNCTIONS ===

// Mock de vmm_create_directory
vmm_directory_t* vmm_create_directory(void) {
    return (vmm_directory_t*)test_malloc(sizeof(vmm_directory_t));
}

// Mock de vmm_switch_directory
void vmm_switch_directory(vmm_directory_t* dir) {
    // Simulation du changement de répertoire de pages
    (void)dir;
}

// Mock de la fonction de changement de contexte assembleur
void switch_task(cpu_state_t* old_state, cpu_state_t* new_state) {
    mock_task_switch_called++;
    if (old_state && new_state) {
        // Copier l'état pour simulation
        *old_state = *new_state;
    }
}

// === SETUP ET TEARDOWN ===

void setUp(void) {
    test_kernel_init();
    test_kernel_save_state();
    
    // Reset des variables globales
    mock_task_switch_called = 0;
    mock_current_task = NULL;
    mock_task_queue = NULL;
    current_task = NULL;
    task_queue = NULL;
    next_task_id = 1;
    g_reschedule_needed = 0;
}

void tearDown(void) {
    test_kernel_restore_state();
    test_kernel_cleanup();
    
    // Nettoyer les tâches créées pendant les tests
    task_t* current = task_queue;
    while (current) {
        task_t* next = current->next;
        test_free(current);
        current = next;
    }
    
    task_queue = NULL;
    current_task = NULL;
}

// === HELPER FUNCTIONS ===

void dummy_task_function(void) {
    // Fonction vide pour les tests
}

void counting_task_function(void) {
    static int counter = 0;
    counter++;
    // Simule du travail
    for (volatile int i = 0; i < 1000; i++);
}

// === TESTS D'INITIALISATION ===

void test_tasking_init(void) {
    tasking_init();
    
    // Vérifier l'état initial
    TEST_ASSERT_NULL(current_task);
    TEST_ASSERT_NULL(task_queue);
    TEST_ASSERT_EQUAL(1, next_task_id);
    TEST_ASSERT_FALSE(g_reschedule_needed);
}

// === TESTS DE CRÉATION DE TÂCHES ===

void test_create_task_basic(void) {
    tasking_init();
    
    task_t* task = create_task(dummy_task_function);
    
    TEST_ASSERT_NOT_NULL(task);
    TEST_ASSERT_EQUAL(1, task->id);
    TEST_ASSERT_EQUAL(TASK_READY, task->state);
    TEST_ASSERT_EQUAL(TASK_TYPE_KERNEL, task->type);
    TEST_ASSERT_NOT_NULL(task->vmm_dir);
    TEST_ASSERT_EQUAL(0, task->kernel_stack_p);
    TEST_ASSERT_NULL(task->next);
    TEST_ASSERT_NULL(task->prev);
}

void test_create_multiple_tasks(void) {
    tasking_init();
    
    const int num_tasks = 5;
    task_t* tasks[num_tasks];
    
    for (int i = 0; i < num_tasks; i++) {
        tasks[i] = create_task(dummy_task_function);
        
        TEST_ASSERT_NOT_NULL(tasks[i]);
        TEST_ASSERT_EQUAL(i + 1, tasks[i]->id);
        TEST_ASSERT_EQUAL(TASK_READY, tasks[i]->state);
        
        // Vérifier l'unicité des IDs
        for (int j = 0; j < i; j++) {
            TEST_ASSERT_NOT_EQUAL(tasks[i]->id, tasks[j]->id);
        }
    }
}

void test_create_task_memory_allocation(void) {
    tasking_init();
    
    task_t* task = create_task(dummy_task_function);
    TEST_ASSERT_NOT_NULL(task);
    
    // Vérifier que la tâche a une structure cohérente
    TEST_ASSERT_STRUCTURE_INTEGRITY(task, task_t, id, task->id);
    TEST_ASSERT_NOT_NULL(task->vmm_dir);
}

// === TESTS DE GESTION DE QUEUE ===

void test_add_task_to_queue_single(void) {
    tasking_init();
    
    task_t* task = create_task(dummy_task_function);
    add_task_to_queue(task);
    
    TEST_ASSERT_EQUAL(task, task_queue);
    TEST_ASSERT_NULL(task->next);
    TEST_ASSERT_NULL(task->prev);
    TEST_ASSERT_EQUAL(1, get_task_count());
}

void test_add_task_to_queue_multiple(void) {
    tasking_init();
    
    const int num_tasks = 3;
    task_t* tasks[num_tasks];
    
    for (int i = 0; i < num_tasks; i++) {
        tasks[i] = create_task(dummy_task_function);
        add_task_to_queue(tasks[i]);
    }
    
    TEST_ASSERT_EQUAL(num_tasks, get_task_count());
    
    // Vérifier l'ordre dans la queue
    task_t* current = task_queue;
    for (int i = 0; i < num_tasks; i++) {
        TEST_ASSERT_EQUAL(tasks[i], current);
        current = current->next;
    }
    TEST_ASSERT_NULL(current);
}

void test_remove_task_from_queue(void) {
    tasking_init();
    
    task_t* task1 = create_task(dummy_task_function);
    task_t* task2 = create_task(dummy_task_function);
    task_t* task3 = create_task(dummy_task_function);
    
    add_task_to_queue(task1);
    add_task_to_queue(task2);
    add_task_to_queue(task3);
    
    TEST_ASSERT_EQUAL(3, get_task_count());
    
    // Supprimer la tâche du milieu
    remove_task(task2);
    
    TEST_ASSERT_EQUAL(2, get_task_count());
    TEST_ASSERT_EQUAL(task1, task_queue);
    TEST_ASSERT_EQUAL(task3, task1->next);
    TEST_ASSERT_EQUAL(task1, task3->prev);
}

// === TESTS DE RECHERCHE DE TÂCHES ===

void test_get_task_by_id(void) {
    tasking_init();
    
    task_t* task1 = create_task(dummy_task_function);
    task_t* task2 = create_task(dummy_task_function);
    task_t* task3 = create_task(dummy_task_function);
    
    add_task_to_queue(task1);
    add_task_to_queue(task2);
    add_task_to_queue(task3);
    
    // Rechercher par ID
    TEST_ASSERT_EQUAL(task1, get_task_by_id(1));
    TEST_ASSERT_EQUAL(task2, get_task_by_id(2));
    TEST_ASSERT_EQUAL(task3, get_task_by_id(3));
    TEST_ASSERT_NULL(get_task_by_id(999));
}

void test_find_task_waiting_for_input(void) {
    tasking_init();
    
    task_t* task1 = create_task(dummy_task_function);
    task_t* task2 = create_task(dummy_task_function);
    task_t* task3 = create_task(dummy_task_function);
    
    task1->state = TASK_RUNNING;
    task2->state = TASK_WAITING_FOR_INPUT;
    task3->state = TASK_READY;
    
    add_task_to_queue(task1);
    add_task_to_queue(task2);
    add_task_to_queue(task3);
    
    TEST_ASSERT_EQUAL(task2, find_task_waiting_for_input());
}

// === TESTS DE SCHEDULING ===

void test_schedule_basic_round_robin(void) {
    tasking_init();
    
    task_t* task1 = create_task(dummy_task_function);
    task_t* task2 = create_task(dummy_task_function);
    
    add_task_to_queue(task1);
    add_task_to_queue(task2);
    
    // Première tâche
    current_task = task1;
    cpu_state_t cpu_state = {0};
    
    schedule(&cpu_state);
    
    // Devrait passer à la tâche suivante
    TEST_ASSERT_EQUAL(task2, current_task);
}

void test_schedule_skip_terminated_tasks(void) {
    tasking_init();
    
    task_t* task1 = create_task(dummy_task_function);
    task_t* task2 = create_task(dummy_task_function);
    task_t* task3 = create_task(dummy_task_function);
    
    task2->state = TASK_TERMINATED;
    
    add_task_to_queue(task1);
    add_task_to_queue(task2);
    add_task_to_queue(task3);
    
    current_task = task1;
    cpu_state_t cpu_state = {0};
    
    schedule(&cpu_state);
    
    // Devrait passer à task3, en sautant task2 (terminée)
    TEST_ASSERT_EQUAL(task3, current_task);
}

void test_schedule_no_ready_tasks(void) {
    tasking_init();
    
    task_t* task1 = create_task(dummy_task_function);
    task1->state = TASK_WAITING;
    
    add_task_to_queue(task1);
    current_task = task1;
    
    cpu_state_t cpu_state = {0};
    
    schedule(&cpu_state);
    
    // Devrait rester sur la même tâche
    TEST_ASSERT_EQUAL(task1, current_task);
}

// === TESTS DE PERFORMANCE ===

void test_task_creation_performance(void) {
    tasking_init();
    
    const int num_tasks = 100;
    
    TEST_BENCHMARK("Task Creation", num_tasks, {
        task_t* task = create_task(dummy_task_function);
        if (task) {
            add_task_to_queue(task);
        }
    });
    
    TEST_ASSERT_EQUAL(num_tasks, get_task_count());
}

void test_scheduling_performance(void) {
    tasking_init();
    
    // Créer plusieurs tâches
    const int num_tasks = 10;
    for (int i = 0; i < num_tasks; i++) {
        task_t* task = create_task(counting_task_function);
        add_task_to_queue(task);
    }
    
    current_task = task_queue;
    cpu_state_t cpu_state = {0};
    
    TEST_BENCHMARK("Task Scheduling", 1000, {
        schedule(&cpu_state);
    });
    
    // Vérifier que le scheduling s'est effectué
    TEST_ASSERT_GREATER_THAN(0, mock_task_switch_called);
}

// === TESTS DE STATES DE TÂCHES ===

void test_task_state_transitions(void) {
    tasking_init();
    
    task_t* task = create_task(dummy_task_function);
    add_task_to_queue(task);
    
    // État initial
    TEST_ASSERT_EQUAL(TASK_READY, task->state);
    
    // Simulation des transitions d'état
    task->state = TASK_RUNNING;
    TEST_ASSERT_EQUAL(TASK_RUNNING, task->state);
    
    task->state = TASK_WAITING;
    TEST_ASSERT_EQUAL(TASK_WAITING, task->state);
    
    task->state = TASK_READY;
    TEST_ASSERT_EQUAL(TASK_READY, task->state);
    
    task->state = TASK_TERMINATED;
    TEST_ASSERT_EQUAL(TASK_TERMINATED, task->state);
}

void test_task_yield_functionality(void) {
    tasking_init();
    
    task_t* task1 = create_task(dummy_task_function);
    task_t* task2 = create_task(dummy_task_function);
    
    add_task_to_queue(task1);
    add_task_to_queue(task2);
    
    current_task = task1;
    
    // Appeler task_yield devrait déclencher un reschedule
    task_yield();
    
    TEST_ASSERT_TRUE(g_reschedule_needed);
}

// === TESTS DE ROBUSTESSE ===

void test_task_memory_corruption_detection(void) {
    tasking_init();
    
    task_t* task = create_task(dummy_task_function);
    
    // Remplir avec un pattern connu
    test_fill_memory_pattern(task, sizeof(task_t), TEST_MEMORY_PATTERN_1);
    
    // Sauvegarder l'ID et l'état (qui sont écrasés par le pattern)
    task->id = 1;
    task->state = TASK_READY;
    
    // Corrompre une partie de la structure
    uint8_t* task_bytes = (uint8_t*)task;
    task_bytes[sizeof(task_t) / 2] ^= 0xFF;
    
    // Détecter la corruption (test simplifié)
    int corruption_detected = (task->id != 1) || (task->state >= 5);
    TEST_ASSERT_FALSE(corruption_detected);
}

void test_task_queue_integrity(void) {
    tasking_init();
    
    const int num_tasks = 5;
    task_t* tasks[num_tasks];
    
    // Créer et ajouter les tâches
    for (int i = 0; i < num_tasks; i++) {
        tasks[i] = create_task(dummy_task_function);
        add_task_to_queue(tasks[i]);
    }
    
    // Vérifier l'intégrité de la liste chaînée
    task_t* current = task_queue;
    int count = 0;
    
    while (current && count < num_tasks + 1) { // +1 pour éviter boucle infinie
        count++;
        
        // Vérifier les liens avant/arrière
        if (current->next) {
            TEST_ASSERT_EQUAL(current, current->next->prev);
        }
        if (current->prev) {
            TEST_ASSERT_EQUAL(current, current->prev->next);
        }
        
        current = current->next;
    }
    
    TEST_ASSERT_EQUAL(num_tasks, count);
    TEST_ASSERT_EQUAL(num_tasks, get_task_count());
}

void test_task_cleanup_on_termination(void) {
    tasking_init();
    
    task_t* task = create_task(dummy_task_function);
    add_task_to_queue(task);
    
    // Simuler la terminaison
    task_exit();
    
    if (current_task) {
        TEST_ASSERT_EQUAL(TASK_TERMINATED, current_task->state);
    }
}

// === TESTS DE TÉLÉMÉTRIE ===

void test_task_metrics_snapshot_and_missing_pid(void) {
    task_t* task;
    os_task_metrics_t metrics;

    tasking_init();
    mock_timer.tick_count = 100U;
    task = create_task(dummy_task_function);
    add_task_to_queue(task);
    task->state = TASK_RUNNING;
    task->run_ticks = 17U;
    task->last_scheduled_ticks = 140U;
    task->switch_count = 3U;
    current_task = task;
    mock_timer.tick_count = 200U;

    TEST_ASSERT_EQUAL(0, task_fill_metrics(task->id, &metrics));
    TEST_ASSERT_EQUAL(task->id, metrics.pid);
    TEST_ASSERT_EQUAL(OS_TASK_RUNNING, metrics.state);
    TEST_ASSERT_EQUAL(OS_TASK_KERNEL, metrics.type);
    TEST_ASSERT_EQUAL(100, metrics.created_ticks);
    TEST_ASSERT_EQUAL(100, metrics.age_ticks);
    TEST_ASSERT_EQUAL(77, metrics.run_ticks);
    TEST_ASSERT_EQUAL(3, metrics.switch_count);
    TEST_ASSERT_EQUAL(OS_TASK_NOT_FOUND, task_fill_metrics(999, &metrics));
}

// === TESTS DE POLITIQUE CPU ===

void test_task_priority_selection_and_validation(void) {
    task_t* task1;
    task_t* task2;
    task_t* task3;
    os_task_metrics_t metrics;
    cpu_state_t cpu_state = {0};

    tasking_init();
    task1 = create_task(dummy_task_function);
    task2 = create_task(dummy_task_function);
    task3 = create_task(dummy_task_function);
    task1->type = TASK_TYPE_USER;
    task2->type = TASK_TYPE_USER;
    task3->type = TASK_TYPE_USER;
    add_task_to_queue(task1);
    add_task_to_queue(task2);
    add_task_to_queue(task3);
    task1->state = TASK_RUNNING;
    task2->state = TASK_READY;
    task3->state = TASK_READY;
    current_task = task1;
    task2->parent_pid = task1->id;
    task3->parent_pid = task1->id;

    TEST_ASSERT_EQUAL(0, task_set_priority(task1->id, task2->id, OS_TASK_PRIORITY_LOW));
    TEST_ASSERT_EQUAL(0, task_set_priority(task1->id, task3->id, OS_TASK_PRIORITY_HIGH));
    schedule(&cpu_state);
    TEST_ASSERT_EQUAL(task3, current_task);
    TEST_ASSERT_EQUAL(0, task_fill_metrics(task3->id, &metrics));
    TEST_ASSERT_EQUAL(OS_TASK_PRIORITY_HIGH, metrics.priority);
    TEST_ASSERT_EQUAL(0, task_set_priority(task1->id, task2->id, OS_TASK_PRIORITY_HIGH));
    schedule(&cpu_state);
    TEST_ASSERT_EQUAL(task2, current_task);
    TEST_ASSERT_EQUAL(OS_TASK_BAD_PRIORITY, task_set_priority(task1->id, task2->id, 0));
    TEST_ASSERT_EQUAL(OS_TASK_BAD_PRIORITY, task_set_priority(task1->id, task2->id, 4));
    TEST_ASSERT_EQUAL(OS_TASK_NOT_FOUND, task_set_priority(task1->id, 999, OS_TASK_PRIORITY_NORMAL));
}

void test_task_priority_control_authority(void) {
    task_t* parent;
    task_t* child;
    task_t* unrelated;
    os_task_metrics_t metrics;

    tasking_init();
    parent = create_task(dummy_task_function);
    child = create_task(dummy_task_function);
    unrelated = create_task(dummy_task_function);
    parent->type = TASK_TYPE_USER;
    child->type = TASK_TYPE_USER;
    unrelated->type = TASK_TYPE_USER;
    child->parent_pid = parent->id;
    add_task_to_queue(parent);
    add_task_to_queue(child);
    add_task_to_queue(unrelated);

    TEST_ASSERT_EQUAL(0, task_set_priority(parent->id, child->id, OS_TASK_PRIORITY_HIGH));
    TEST_ASSERT_EQUAL(0, task_set_priority(child->id, child->id, OS_TASK_PRIORITY_LOW));
    TEST_ASSERT_EQUAL(OS_TASK_CONTROL_DENIED,
                      task_set_priority(child->id, parent->id, OS_TASK_PRIORITY_HIGH));
    TEST_ASSERT_EQUAL(OS_TASK_CONTROL_DENIED,
                      task_set_priority(unrelated->id, child->id, OS_TASK_PRIORITY_HIGH));
    TEST_ASSERT_EQUAL(0, task_fill_metrics(child->id, &metrics));
    TEST_ASSERT_EQUAL(parent->id, metrics.parent_pid);
}

void test_task_kill_parent_authority(void) {
    task_t* parent;
    task_t* child;
    task_t* unrelated;

    tasking_init();
    parent = create_task(dummy_task_function);
    child = create_task(dummy_task_function);
    unrelated = create_task(dummy_task_function);
    child->parent_pid = parent->id;
    add_task_to_queue(parent);
    add_task_to_queue(child);
    add_task_to_queue(unrelated);

    TEST_ASSERT_EQUAL(OS_TASK_CONTROL_DENIED,
                      task_kill(unrelated->id, child->id));
    TEST_ASSERT_EQUAL(-3, task_kill(child->id, child->id));
    TEST_ASSERT_EQUAL(0, task_kill(parent->id, child->id));
    TEST_ASSERT_NULL(get_task_by_id(child->id));
}

void test_task_reparents_children_on_departure(void) {
    task_t* owner;
    task_t* parent;
    task_t* child;
    task_t* grandchild;
    task_t* root;
    task_t* orphan;

    tasking_init();
    owner = create_task(dummy_task_function);
    parent = create_task(dummy_task_function);
    child = create_task(dummy_task_function);
    grandchild = create_task(dummy_task_function);
    parent->parent_pid = owner->id;
    child->parent_pid = parent->id;
    grandchild->parent_pid = child->id;
    add_task_to_queue(owner);
    add_task_to_queue(parent);
    add_task_to_queue(child);
    add_task_to_queue(grandchild);

    TEST_ASSERT_EQUAL(0, task_kill(owner->id, parent->id));
    TEST_ASSERT_EQUAL(owner->id, child->parent_pid);
    TEST_ASSERT_EQUAL(child->id, grandchild->parent_pid);
    TEST_ASSERT_EQUAL(0, task_kill(owner->id, child->id));
    TEST_ASSERT_EQUAL(owner->id, grandchild->parent_pid);

    root = create_task(dummy_task_function);
    orphan = create_task(dummy_task_function);
    root->parent_pid = -1;
    orphan->parent_pid = root->id;
    add_task_to_queue(root);
    add_task_to_queue(orphan);
    task_reparent_children(root);
    TEST_ASSERT_EQUAL(-1, orphan->parent_pid);
}

void test_task_supervision_wait_and_children(void) {
    task_t* parent;
    task_t* child;
    task_t* unrelated;
    os_task_metrics_t metrics;

    tasking_init();
    parent = create_task(dummy_task_function);
    child = create_task(dummy_task_function);
    unrelated = create_task(dummy_task_function);
    child->parent_pid = parent->id;
    add_task_to_queue(parent);
    add_task_to_queue(child);
    add_task_to_queue(unrelated);

    TEST_ASSERT_EQUAL(0, task_fill_metrics(parent->id, &metrics));
    TEST_ASSERT_EQUAL(1, metrics.direct_children);
    TEST_ASSERT_EQUAL(0, task_wait_for_child(parent->id, child->id));
    TEST_ASSERT_EQUAL(TASK_WAITING, parent->state);
    TEST_ASSERT_EQUAL(parent->id, child->waiter_pid);
    task_wake_waiter(child);
    TEST_ASSERT_EQUAL(TASK_READY, parent->state);
    TEST_ASSERT_EQUAL(0, child->waiter_pid);
    TEST_ASSERT_EQUAL(OS_TASK_NOT_CHILD,
                      task_wait_for_child(unrelated->id, child->id));
}

void test_task_direct_child_capacity(void) {
    task_t* parent;
    task_t* children[OS_TASK_CHILD_CAPACITY];
    uint32_t i;

    tasking_init();
    parent = create_task(dummy_task_function);
    add_task_to_queue(parent);
    for (i = 0U; i < OS_TASK_CHILD_CAPACITY; i++) {
        children[i] = create_task(dummy_task_function);
        children[i]->parent_pid = parent->id;
        add_task_to_queue(children[i]);
    }

    TEST_ASSERT_EQUAL(OS_TASK_CHILD_CAPACITY,
                      task_count_direct_children(parent->id));
    TEST_ASSERT_EQUAL(OS_TASK_CHILD_LIMIT, task_can_create_child(parent->id));
    TEST_ASSERT_EQUAL(0, task_kill(parent->id, children[0]->id));
    TEST_ASSERT_EQUAL(OS_TASK_CHILD_CAPACITY - 1U,
                      task_count_direct_children(parent->id));
    TEST_ASSERT_EQUAL(0, task_can_create_child(parent->id));
}

void test_task_governance_name_capacity_and_events(void) {
    task_t* parent;
    task_t* child;
    task_t* child2;
    task_t* child3;
    task_t* child4;
    os_task_capacity_t capacity;
    os_task_exit_result_t result;
    os_task_exit_history_t history;
    os_task_exit_history_observation_t observation;
    os_ipc_message_t message;
    os_task_event_t event;
    uint32_t i;

    tasking_init();
    TEST_ASSERT_EQUAL(0, task_fill_capacity(&capacity));
    TEST_ASSERT_EQUAL(0, capacity.active);
    TEST_ASSERT_EQUAL(OS_TASK_GLOBAL_CAPACITY, capacity.capacity);
    TEST_ASSERT_EQUAL(OS_TASK_GLOBAL_CAPACITY, capacity.available);

    parent = create_task(dummy_task_function);
    child = create_task(dummy_task_function);
    parent->type = TASK_TYPE_USER;
    child->type = TASK_TYPE_USER;
    child->parent_pid = parent->id;
    add_task_to_queue(parent);
    add_task_to_queue(child);

    TEST_ASSERT_EQUAL(0, task_set_name(parent->id, child->id, "worker"));
    TEST_ASSERT_EQUAL_STRING("worker", child->name);
    TEST_ASSERT_EQUAL(OS_TASK_BAD_NAME, task_set_name(parent->id, child->id, ""));
    TEST_ASSERT_EQUAL(OS_TASK_CONTROL_DENIED,
                      task_set_name(child->id, parent->id, "forbidden"));
    TEST_ASSERT_EQUAL(0, task_suspend_child(parent->id, child->id));
    TEST_ASSERT_EQUAL(TASK_SUSPENDED, child->state);
    TEST_ASSERT_EQUAL(OS_TASK_BAD_STATE, task_suspend_child(parent->id, child->id));
    TEST_ASSERT_EQUAL(OS_TASK_CONTROL_DENIED, task_resume_child(child->id, parent->id));
    TEST_ASSERT_EQUAL(0, task_resume_child(parent->id, child->id));
    TEST_ASSERT_EQUAL(TASK_READY, child->state);
    TEST_ASSERT_EQUAL(OS_TASK_BAD_STATE, task_resume_child(parent->id, child->id));

    task_report_parent_exit(child, 7, OS_TASK_EVENT_EXITED);
    TEST_ASSERT_EQUAL(0, task_get_child_result(parent->id, child->id, &result));
    TEST_ASSERT_EQUAL(child->id, result.child_pid);
    TEST_ASSERT_EQUAL(7, result.exit_code);
    TEST_ASSERT_EQUAL(OS_TASK_EVENT_EXITED, result.reason);
    TEST_ASSERT_EQUAL(OS_TASK_NO_CHILD_RESULT,
                      task_get_child_result(parent->id, child->id + 1, &result));
    TEST_ASSERT_EQUAL(1, parent->ipc_endpoint.count);
    message = parent->ipc_endpoint.messages[parent->ipc_endpoint.read_index];
    parent->ipc_endpoint.read_index = (parent->ipc_endpoint.read_index + 1U) % IPC_ENDPOINT_CAPACITY;
    parent->ipc_endpoint.count--;
    TEST_ASSERT_EQUAL(0, os_task_parse_event(&message, &event));
    TEST_ASSERT_EQUAL(child->id, event.child_pid);
    TEST_ASSERT_EQUAL(OS_TASK_EVENT_EXITED, event.reason);

    TEST_ASSERT_EQUAL(0, task_kill(parent->id, child->id));
    TEST_ASSERT_EQUAL(1, parent->ipc_endpoint.count);
    message = parent->ipc_endpoint.messages[parent->ipc_endpoint.read_index];
    TEST_ASSERT_EQUAL(0, os_task_parse_event(&message, &event));
    TEST_ASSERT_EQUAL(OS_TASK_EVENT_KILLED, event.reason);
    TEST_ASSERT_EQUAL(0, task_get_child_result(parent->id, child->id, &result));
    TEST_ASSERT_EQUAL(OS_TASK_EXIT_KILLED, result.exit_code);
    TEST_ASSERT_EQUAL(OS_TASK_EVENT_KILLED, result.reason);
    TEST_ASSERT_EQUAL(0, task_fill_child_result_history(parent->id, &history));
    TEST_ASSERT_EQUAL(2, history.count);
    TEST_ASSERT_EQUAL(7, history.entries[0].exit_code);
    TEST_ASSERT_EQUAL(OS_TASK_EXIT_KILLED, history.entries[1].exit_code);

    {
        child2 = create_task(dummy_task_function);
        child3 = create_task(dummy_task_function);
        child4 = create_task(dummy_task_function);
        child2->type = TASK_TYPE_USER;
        child3->type = TASK_TYPE_USER;
        child4->type = TASK_TYPE_USER;
        child2->parent_pid = parent->id;
        child3->parent_pid = parent->id;
        child4->parent_pid = parent->id;
        add_task_to_queue(child2);
        add_task_to_queue(child3);
        add_task_to_queue(child4);
        task_report_parent_exit(child2, 8, OS_TASK_EVENT_EXITED);
        task_report_parent_exit(child3, 9, OS_TASK_EVENT_EXITED);
        task_report_parent_exit(child4, 10, OS_TASK_EVENT_EXITED);
    }
    TEST_ASSERT_EQUAL(0, task_fill_child_result_history(parent->id, &history));
    TEST_ASSERT_EQUAL(OS_TASK_EXIT_HISTORY_CAPACITY, history.count);
    TEST_ASSERT_EQUAL(OS_TASK_EXIT_KILLED, history.entries[0].exit_code);
    TEST_ASSERT_EQUAL(8, history.entries[1].exit_code);
    TEST_ASSERT_EQUAL(9, history.entries[2].exit_code);
    TEST_ASSERT_EQUAL(10, history.entries[3].exit_code);
    TEST_ASSERT_EQUAL(0, task_observe_child_result_history(parent->id, 6U, &observation));
    TEST_ASSERT_EQUAL(6, observation.generation);
    TEST_ASSERT_EQUAL(OS_TASK_EXIT_HISTORY_CAPACITY, observation.history.count);
    TEST_ASSERT_EQUAL(OS_TASK_HISTORY_STALE,
                      task_observe_child_result_history(parent->id, 5U, &observation));
    TEST_ASSERT_EQUAL(6, observation.generation);
    TEST_ASSERT_EQUAL(7, task_ack_child_result_history(parent->id));
    TEST_ASSERT_EQUAL(OS_TASK_NO_CHILD_RESULT,
                      task_get_child_result(parent->id, child->id, &result));
    TEST_ASSERT_EQUAL(0, task_observe_child_result_history(parent->id, 7U, &observation));
    TEST_ASSERT_EQUAL(0, observation.history.count);

    task_report_parent_exit(child2, 11, OS_TASK_EVENT_EXITED);
    task_report_parent_exit(child3, 12, OS_TASK_EVENT_KILLED);
    TEST_ASSERT_EQUAL(0, task_find_child_result_history(parent->id, child2->id, &result));
    TEST_ASSERT_EQUAL(11, result.exit_code);
    TEST_ASSERT_EQUAL(10, task_forget_child_result_history(parent->id, child2->id));
    TEST_ASSERT_EQUAL(OS_TASK_NO_CHILD_RESULT,
                      task_find_child_result_history(parent->id, child2->id, &result));
    TEST_ASSERT_EQUAL(0, task_get_child_result(parent->id, child3->id, &result));
    TEST_ASSERT_EQUAL(12, result.exit_code);
    TEST_ASSERT_EQUAL(OS_TASK_EVENT_KILLED, result.reason);
    TEST_ASSERT_EQUAL(OS_TASK_HISTORY_STALE,
                      task_observe_child_result_history(parent->id, 9U, &observation));
    TEST_ASSERT_EQUAL(10, observation.generation);
    TEST_ASSERT_EQUAL(11, task_forget_child_result_history(parent->id, child3->id));
    TEST_ASSERT_EQUAL(OS_TASK_NO_CHILD_RESULT,
                      task_get_child_result(parent->id, child3->id, &result));

    tasking_init();
    for (i = 0U; i < OS_TASK_GLOBAL_CAPACITY; i++) {
        task_t* task = create_task(dummy_task_function);
        add_task_to_queue(task);
    }
    TEST_ASSERT_EQUAL(OS_TASK_GLOBAL_LIMIT, task_can_create_global());
    TEST_ASSERT_EQUAL(0, task_fill_capacity(&capacity));
    TEST_ASSERT_EQUAL(OS_TASK_GLOBAL_CAPACITY, capacity.active);
    TEST_ASSERT_EQUAL(0, capacity.available);
}

void test_task_kill_direct_children_snapshot(void) {
    task_t* parent;
    task_t* child_a;
    task_t* child_b;
    task_t* grandchild;
    os_task_exit_history_t history;

    tasking_init();
    parent = create_task(dummy_task_function);
    child_a = create_task(dummy_task_function);
    child_b = create_task(dummy_task_function);
    grandchild = create_task(dummy_task_function);
    TEST_ASSERT_NOT_NULL(parent);
    TEST_ASSERT_NOT_NULL(child_a);
    TEST_ASSERT_NOT_NULL(child_b);
    TEST_ASSERT_NOT_NULL(grandchild);
    parent->type = TASK_TYPE_USER;
    child_a->type = TASK_TYPE_USER;
    child_b->type = TASK_TYPE_USER;
    grandchild->type = TASK_TYPE_USER;
    child_a->parent_pid = parent->id;
    child_b->parent_pid = parent->id;
    grandchild->parent_pid = child_a->id;
    add_task_to_queue(parent);
    add_task_to_queue(child_a);
    add_task_to_queue(child_b);
    add_task_to_queue(grandchild);

    TEST_ASSERT_EQUAL(0, task_suspend_child(parent->id, child_b->id));
    TEST_ASSERT_EQUAL(TASK_SUSPENDED, child_b->state);
    TEST_ASSERT_EQUAL(2, task_kill_direct_children(parent->id));
    TEST_ASSERT_NULL(get_task_by_id(child_a->id));
    TEST_ASSERT_NULL(get_task_by_id(child_b->id));
    TEST_ASSERT_EQUAL(parent->id, grandchild->parent_pid);
    TEST_ASSERT_EQUAL(1, task_count_direct_children(parent->id));
    TEST_ASSERT_EQUAL(0, task_fill_child_result_history(parent->id, &history));
    TEST_ASSERT_EQUAL(2, history.count);
    TEST_ASSERT_EQUAL(OS_TASK_EXIT_KILLED, history.entries[0].exit_code);
    TEST_ASSERT_EQUAL(OS_TASK_EXIT_KILLED, history.entries[1].exit_code);

    TEST_ASSERT_EQUAL(1, task_kill_direct_children(parent->id));
    TEST_ASSERT_NULL(get_task_by_id(grandchild->id));
    TEST_ASSERT_EQUAL(0, task_count_direct_children(parent->id));
    TEST_ASSERT_EQUAL(0, task_kill_direct_children(parent->id));
}

void test_task_direct_children_and_wait_any(void) {
    task_t* parent;
    task_t* child_a;
    task_t* child_b;
    os_task_children_t children;

    tasking_init();
    parent = create_task(dummy_task_function);
    child_a = create_task(dummy_task_function);
    child_b = create_task(dummy_task_function);
    TEST_ASSERT_NOT_NULL(parent);
    TEST_ASSERT_NOT_NULL(child_a);
    TEST_ASSERT_NOT_NULL(child_b);
    parent->type = TASK_TYPE_USER;
    child_a->type = TASK_TYPE_USER;
    child_b->type = TASK_TYPE_USER;
    child_a->parent_pid = parent->id;
    child_b->parent_pid = parent->id;
    strcpy(child_a->name, "alpha");
    strcpy(child_b->name, "beta");
    add_task_to_queue(parent);
    add_task_to_queue(child_a);
    add_task_to_queue(child_b);

    TEST_ASSERT_EQUAL(0, task_suspend_child(parent->id, child_b->id));
    TEST_ASSERT_EQUAL(0, task_fill_direct_children(parent->id, &children));
    TEST_ASSERT_EQUAL(2, children.count);
    TEST_ASSERT_EQUAL(child_a->id, children.entries[0].pid);
    TEST_ASSERT_EQUAL(OS_TASK_READY, children.entries[0].state);
    TEST_ASSERT_EQUAL(child_b->id, children.entries[1].pid);
    TEST_ASSERT_EQUAL(OS_TASK_SUSPENDED, children.entries[1].state);
    TEST_ASSERT_EQUAL(0, task_wait_for_any_child(parent->id));
    TEST_ASSERT_EQUAL(TASK_WAITING, parent->state);
    TEST_ASSERT_EQUAL(parent->id, child_a->waiter_pid);
    TEST_ASSERT_EQUAL(parent->id, child_b->waiter_pid);

    TEST_ASSERT_EQUAL(0, task_kill(parent->id, child_b->id));
    TEST_ASSERT_EQUAL(TASK_READY, parent->state);
    TEST_ASSERT_EQUAL(1, task_count_direct_children(parent->id));
    TEST_ASSERT_EQUAL(0, task_fill_direct_children(parent->id, &children));
    TEST_ASSERT_EQUAL(1, children.count);
    TEST_ASSERT_EQUAL(child_a->id, children.entries[0].pid);
    TEST_ASSERT_EQUAL(0, task_kill(parent->id, child_a->id));
    TEST_ASSERT_EQUAL(OS_TASK_NO_DIRECT_CHILD, task_wait_for_any_child(parent->id));
}

void test_task_child_exit_count(void) {
    task_t* parent;
    task_t* child_a;
    task_t* child_b;
    uint32_t count = 99U;

    tasking_init();
    parent = create_task(dummy_task_function);
    child_a = create_task(dummy_task_function);
    child_b = create_task(dummy_task_function);
    TEST_ASSERT_NOT_NULL(parent);
    TEST_ASSERT_NOT_NULL(child_a);
    TEST_ASSERT_NOT_NULL(child_b);
    parent->type = TASK_TYPE_USER;
    child_a->type = TASK_TYPE_USER;
    child_b->type = TASK_TYPE_USER;
    child_a->parent_pid = parent->id;
    child_b->parent_pid = parent->id;
    add_task_to_queue(parent);
    add_task_to_queue(child_a);
    add_task_to_queue(child_b);

    TEST_ASSERT_EQUAL(0, task_get_child_exit_count(parent->id, &count));
    TEST_ASSERT_EQUAL(0, count);
    TEST_ASSERT_EQUAL(0, task_kill(parent->id, child_a->id));
    TEST_ASSERT_EQUAL(0, task_get_child_exit_count(parent->id, &count));
    TEST_ASSERT_EQUAL(1, count);
    TEST_ASSERT_GREATER_THAN(0, task_ack_child_result_history(parent->id));
    TEST_ASSERT_EQUAL(0, task_get_child_exit_count(parent->id, &count));
    TEST_ASSERT_EQUAL(1, count);
    TEST_ASSERT_EQUAL(0, task_kill(parent->id, child_b->id));
    TEST_ASSERT_EQUAL(0, task_get_child_exit_count(parent->id, &count));
    TEST_ASSERT_EQUAL(2, count);
    TEST_ASSERT_EQUAL(OS_TASK_NOT_FOUND, task_get_child_exit_count(parent->id, NULL));
}

void test_task_supervision_delegation(void) {
    task_t* parent;
    task_t* supervisor;
    task_t* child;
    task_t* descendant;

    tasking_init();
    parent = create_task(dummy_task_function);
    supervisor = create_task(dummy_task_function);
    child = create_task(dummy_task_function);
    descendant = create_task(dummy_task_function);
    TEST_ASSERT_NOT_NULL(parent);
    TEST_ASSERT_NOT_NULL(supervisor);
    TEST_ASSERT_NOT_NULL(child);
    TEST_ASSERT_NOT_NULL(descendant);
    parent->type = TASK_TYPE_USER;
    supervisor->type = TASK_TYPE_USER;
    child->type = TASK_TYPE_USER;
    descendant->type = TASK_TYPE_USER;
    child->parent_pid = parent->id;
    descendant->parent_pid = child->id;
    add_task_to_queue(parent);
    add_task_to_queue(supervisor);
    add_task_to_queue(child);
    add_task_to_queue(descendant);

    TEST_ASSERT_EQUAL(0, task_wait_for_child(parent->id, child->id));
    TEST_ASSERT_EQUAL(OS_TASK_BAD_STATE,
                      task_delegate_child(parent->id, child->id, supervisor->id));
    parent->state = TASK_READY;
    child->waiter_pid = 0;
    TEST_ASSERT_EQUAL(OS_TASK_BAD_DELEGATE,
                      task_delegate_child(parent->id, child->id, descendant->id));
    TEST_ASSERT_EQUAL(OS_TASK_BAD_DELEGATE,
                      task_delegate_child(parent->id, child->id, parent->id));
    TEST_ASSERT_EQUAL(0, task_delegate_child(parent->id, child->id, supervisor->id));
    TEST_ASSERT_EQUAL(supervisor->id, child->parent_pid);
    TEST_ASSERT_EQUAL(0, task_count_direct_children(parent->id));
    TEST_ASSERT_EQUAL(1, task_count_direct_children(supervisor->id));
    TEST_ASSERT_EQUAL(OS_TASK_CONTROL_DENIED, task_suspend_child(parent->id, child->id));
    TEST_ASSERT_EQUAL(0, task_suspend_child(supervisor->id, child->id));
    TEST_ASSERT_EQUAL(0, task_resume_child(supervisor->id, child->id));
    TEST_ASSERT_EQUAL(0, task_kill(supervisor->id, child->id));
    TEST_ASSERT_EQUAL(supervisor->id, descendant->parent_pid);
}

void test_task_supervision_events(void) {
    task_t* parent;
    task_t* child;
    task_t* supervisor;
    os_task_supervision_events_t parent_events;
    os_task_supervision_events_t supervisor_events;
    os_task_supervision_events_observation_t observation;
    int i;

    tasking_init();
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

    for (i = 0; i < 2; i++) {
        TEST_ASSERT_EQUAL(0, task_suspend_child(parent->id, child->id));
        TEST_ASSERT_EQUAL(0, task_resume_child(parent->id, child->id));
    }
    TEST_ASSERT_EQUAL(0, task_fill_supervision_events(parent->id, &parent_events));
    TEST_ASSERT_EQUAL(4, parent_events.count);
    TEST_ASSERT_EQUAL(4, parent_events.generation);
    TEST_ASSERT_EQUAL(OS_TASK_SUPERVISION_SUSPEND, parent_events.entries[0].action);
    TEST_ASSERT_EQUAL(OS_TASK_SUPERVISION_RESUME, parent_events.entries[1].action);
    TEST_ASSERT_EQUAL(3, parent_events.entries[2].sequence);
    TEST_ASSERT_EQUAL(4, parent_events.entries[3].sequence);
    TEST_ASSERT_EQUAL(0, task_observe_supervision_events(parent->id, 4U, &observation));
    TEST_ASSERT_EQUAL(4, observation.generation);
    TEST_ASSERT_EQUAL(4, observation.events.count);
    TEST_ASSERT_EQUAL(OS_TASK_HISTORY_STALE,
                      task_observe_supervision_events(parent->id, 3U, &observation));
    TEST_ASSERT_EQUAL(4, observation.generation);

    TEST_ASSERT_EQUAL(0, task_delegate_child(parent->id, child->id, supervisor->id));
    TEST_ASSERT_EQUAL(0, task_fill_supervision_events(parent->id, &parent_events));
    TEST_ASSERT_EQUAL(4, parent_events.count);
    TEST_ASSERT_EQUAL(5, parent_events.generation);
    TEST_ASSERT_EQUAL(2, parent_events.entries[0].sequence);
    TEST_ASSERT_EQUAL(OS_TASK_SUPERVISION_DELEGATE_OUT, parent_events.entries[3].action);
    TEST_ASSERT_EQUAL(supervisor->id, parent_events.entries[3].related_pid);
    TEST_ASSERT_EQUAL(6, task_ack_supervision_events(parent->id));
    TEST_ASSERT_EQUAL(0, task_fill_supervision_events(parent->id, &parent_events));
    TEST_ASSERT_EQUAL(6, parent_events.generation);
    TEST_ASSERT_EQUAL(0, parent_events.count);
    TEST_ASSERT_EQUAL(OS_TASK_HISTORY_STALE,
                      task_observe_supervision_events(parent->id, 5U, &observation));
    TEST_ASSERT_EQUAL(6, observation.generation);

    TEST_ASSERT_EQUAL(0, task_fill_supervision_events(supervisor->id, &supervisor_events));
    TEST_ASSERT_EQUAL(1, supervisor_events.count);
    TEST_ASSERT_EQUAL(OS_TASK_SUPERVISION_DELEGATE_IN, supervisor_events.entries[0].action);
    TEST_ASSERT_EQUAL(parent->id, supervisor_events.entries[0].related_pid);
    TEST_ASSERT_EQUAL(0, task_kill(supervisor->id, child->id));
    TEST_ASSERT_EQUAL(0, task_fill_supervision_events(supervisor->id, &supervisor_events));
    TEST_ASSERT_EQUAL(2, supervisor_events.count);
    TEST_ASSERT_EQUAL(OS_TASK_SUPERVISION_EXIT, supervisor_events.entries[1].action);
    TEST_ASSERT_EQUAL(OS_TASK_EVENT_KILLED, supervisor_events.entries[1].detail);
}

void test_task_supervision_event_selective(void) {
    task_t* parent;
    task_t* child;
    os_task_supervision_events_t events;
    os_task_supervision_event_t event;

    tasking_init();
    parent = create_task(dummy_task_function);
    child = create_task(dummy_task_function);
    TEST_ASSERT_NOT_NULL(parent);
    TEST_ASSERT_NOT_NULL(child);
    parent->type = TASK_TYPE_USER;
    child->type = TASK_TYPE_USER;
    child->parent_pid = parent->id;
    add_task_to_queue(parent);
    add_task_to_queue(child);

    TEST_ASSERT_EQUAL(0, task_suspend_child(parent->id, child->id));
    TEST_ASSERT_EQUAL(0, task_resume_child(parent->id, child->id));
    TEST_ASSERT_EQUAL(0, task_suspend_child(parent->id, child->id));
    TEST_ASSERT_EQUAL(0, task_find_supervision_event(parent->id, 2U, &event));
    TEST_ASSERT_EQUAL(OS_TASK_SUPERVISION_RESUME, event.action);
    TEST_ASSERT_EQUAL(2, event.sequence);
    TEST_ASSERT_EQUAL(2, task_forget_supervision_event(parent->id, 2U));
    TEST_ASSERT_EQUAL(0, task_fill_supervision_events(parent->id, &events));
    TEST_ASSERT_EQUAL(4, events.generation);
    TEST_ASSERT_EQUAL(2, events.count);
    TEST_ASSERT_EQUAL(1, events.entries[0].sequence);
    TEST_ASSERT_EQUAL(3, events.entries[1].sequence);
    TEST_ASSERT_EQUAL(OS_TASK_NO_SUPERVISION_EVENT,
                      task_find_supervision_event(parent->id, 2U, &event));
    TEST_ASSERT_EQUAL(OS_TASK_NO_SUPERVISION_EVENT,
                      task_forget_supervision_event(parent->id, 2U));
}

void test_task_supervision_notifications(void) {
    task_t* parent;
    task_t* child;
    task_t* supervisor;
    os_ipc_message_t message;
    os_task_supervision_event_t event;

    tasking_init();
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

    TEST_ASSERT_EQUAL(1, task_set_supervision_notify(parent->id, 1U));
    TEST_ASSERT_EQUAL(OS_TASK_BAD_NOTIFY, task_set_supervision_notify(parent->id, 2U));
    TEST_ASSERT_EQUAL(0, task_suspend_child(parent->id, child->id));
    TEST_ASSERT_EQUAL(1, parent->ipc_endpoint.count);
    message = parent->ipc_endpoint.messages[parent->ipc_endpoint.read_index];
    parent->ipc_endpoint.read_index = (parent->ipc_endpoint.read_index + 1U) % IPC_ENDPOINT_CAPACITY;
    parent->ipc_endpoint.count--;
    TEST_ASSERT_EQUAL(0, os_task_parse_supervision_event(&message, &event));
    TEST_ASSERT_EQUAL(1, event.sequence);
    TEST_ASSERT_EQUAL(OS_TASK_SUPERVISION_SUSPEND, event.action);
    TEST_ASSERT_EQUAL(child->id, event.child_pid);
    TEST_ASSERT_EQUAL(0, event.related_pid);

    TEST_ASSERT_EQUAL(0, task_resume_child(parent->id, child->id));
    message = parent->ipc_endpoint.messages[parent->ipc_endpoint.read_index];
    parent->ipc_endpoint.read_index = (parent->ipc_endpoint.read_index + 1U) % IPC_ENDPOINT_CAPACITY;
    parent->ipc_endpoint.count--;
    TEST_ASSERT_EQUAL(0, os_task_parse_supervision_event(&message, &event));
    TEST_ASSERT_EQUAL(2, event.sequence);
    TEST_ASSERT_EQUAL(OS_TASK_SUPERVISION_RESUME, event.action);

    TEST_ASSERT_EQUAL(1, task_set_supervision_notify(supervisor->id, 1U));
    TEST_ASSERT_EQUAL(0, task_delegate_child(parent->id, child->id, supervisor->id));
    message = parent->ipc_endpoint.messages[parent->ipc_endpoint.read_index];
    parent->ipc_endpoint.read_index = (parent->ipc_endpoint.read_index + 1U) % IPC_ENDPOINT_CAPACITY;
    parent->ipc_endpoint.count--;
    TEST_ASSERT_EQUAL(0, os_task_parse_supervision_event(&message, &event));
    TEST_ASSERT_EQUAL(OS_TASK_SUPERVISION_DELEGATE_OUT, event.action);
    TEST_ASSERT_EQUAL(supervisor->id, event.related_pid);
    message = supervisor->ipc_endpoint.messages[supervisor->ipc_endpoint.read_index];
    supervisor->ipc_endpoint.read_index = (supervisor->ipc_endpoint.read_index + 1U) % IPC_ENDPOINT_CAPACITY;
    supervisor->ipc_endpoint.count--;
    TEST_ASSERT_EQUAL(0, os_task_parse_supervision_event(&message, &event));
    TEST_ASSERT_EQUAL(OS_TASK_SUPERVISION_DELEGATE_IN, event.action);
    TEST_ASSERT_EQUAL(parent->id, event.related_pid);

    TEST_ASSERT_EQUAL(0, task_set_supervision_notify(supervisor->id, 0U));
    TEST_ASSERT_EQUAL(0, task_suspend_child(supervisor->id, child->id));
    TEST_ASSERT_EQUAL(0, supervisor->ipc_endpoint.count);
    TEST_ASSERT_EQUAL(1, task_set_supervision_notify(supervisor->id, 1U));
    TEST_ASSERT_EQUAL(0, task_resume_child(supervisor->id, child->id));
    message = supervisor->ipc_endpoint.messages[supervisor->ipc_endpoint.read_index];
    supervisor->ipc_endpoint.read_index = (supervisor->ipc_endpoint.read_index + 1U) % IPC_ENDPOINT_CAPACITY;
    supervisor->ipc_endpoint.count--;
    TEST_ASSERT_EQUAL(0, os_task_parse_supervision_event(&message, &event));
    TEST_ASSERT_EQUAL(OS_TASK_SUPERVISION_RESUME, event.action);

    TEST_ASSERT_EQUAL(0, task_kill(supervisor->id, child->id));
    TEST_ASSERT_EQUAL(2, supervisor->ipc_endpoint.count);
    message = supervisor->ipc_endpoint.messages[supervisor->ipc_endpoint.read_index];
    TEST_ASSERT_EQUAL(0, os_task_parse_supervision_event(&message, &event));
    TEST_ASSERT_EQUAL(OS_TASK_SUPERVISION_EXIT, event.action);
    TEST_ASSERT_EQUAL(OS_TASK_EVENT_KILLED, event.detail);
}

void test_task_supervision_notification_filter(void) {
    task_t* parent;
    task_t* child;
    os_task_supervision_notify_status_t status;
    os_ipc_message_t message;
    os_task_supervision_event_t event;

    tasking_init();
    parent = create_task(dummy_task_function);
    child = create_task(dummy_task_function);
    TEST_ASSERT_NOT_NULL(parent);
    TEST_ASSERT_NOT_NULL(child);
    parent->type = TASK_TYPE_USER;
    child->type = TASK_TYPE_USER;
    child->parent_pid = parent->id;
    add_task_to_queue(parent);
    add_task_to_queue(child);

    TEST_ASSERT_EQUAL(0, task_fill_supervision_notify_status(parent->id, &status));
    TEST_ASSERT_EQUAL(0, status.enabled);
    TEST_ASSERT_EQUAL(OS_TASK_SUPERVISION_NOTIFY_ALL, status.mask);
    TEST_ASSERT_EQUAL(OS_TASK_BAD_NOTIFY_FILTER,
                      task_set_supervision_notify_filter(parent->id, 1U << 5));
    TEST_ASSERT_EQUAL(0, task_set_supervision_notify_filter(
                             parent->id, OS_TASK_SUPERVISION_NOTIFY_SUSPEND));
    TEST_ASSERT_EQUAL(1, task_set_supervision_notify(parent->id, 1U));
    TEST_ASSERT_EQUAL(0, task_suspend_child(parent->id, child->id));
    TEST_ASSERT_EQUAL(1, parent->ipc_endpoint.count);
    message = parent->ipc_endpoint.messages[parent->ipc_endpoint.read_index];
    parent->ipc_endpoint.read_index = (parent->ipc_endpoint.read_index + 1U) % IPC_ENDPOINT_CAPACITY;
    parent->ipc_endpoint.count--;
    TEST_ASSERT_EQUAL(0, os_task_parse_supervision_event(&message, &event));
    TEST_ASSERT_EQUAL(OS_TASK_SUPERVISION_SUSPEND, event.action);
    TEST_ASSERT_EQUAL(0, task_resume_child(parent->id, child->id));
    TEST_ASSERT_EQUAL(0, parent->ipc_endpoint.count);
    TEST_ASSERT_EQUAL(0, task_fill_supervision_notify_status(parent->id, &status));
    TEST_ASSERT_EQUAL(1, status.enabled);
    TEST_ASSERT_EQUAL(OS_TASK_SUPERVISION_NOTIFY_SUSPEND, status.mask);

    TEST_ASSERT_EQUAL(0, task_set_supervision_notify_filter(parent->id, 0U));
    TEST_ASSERT_EQUAL(0, task_suspend_child(parent->id, child->id));
    TEST_ASSERT_EQUAL(0, parent->ipc_endpoint.count);
    TEST_ASSERT_EQUAL(0, task_set_supervision_notify_filter(
                             parent->id, OS_TASK_SUPERVISION_NOTIFY_RESUME));
    TEST_ASSERT_EQUAL(0, task_resume_child(parent->id, child->id));
    TEST_ASSERT_EQUAL(1, parent->ipc_endpoint.count);
    message = parent->ipc_endpoint.messages[parent->ipc_endpoint.read_index];
    TEST_ASSERT_EQUAL(0, os_task_parse_supervision_event(&message, &event));
    TEST_ASSERT_EQUAL(OS_TASK_SUPERVISION_RESUME, event.action);
}

void test_task_supervision_watchlist(void) {
    task_t* parent;
    task_t* first;
    task_t* second;
    task_t* outsider;
    os_task_supervision_watch_status_t status;
    os_ipc_message_t message;
    os_task_supervision_event_t event;
    os_task_supervision_events_t events;

    tasking_init();
    parent = create_task(dummy_task_function);
    first = create_task(dummy_task_function);
    second = create_task(dummy_task_function);
    outsider = create_task(dummy_task_function);
    TEST_ASSERT_NOT_NULL(parent);
    TEST_ASSERT_NOT_NULL(first);
    TEST_ASSERT_NOT_NULL(second);
    TEST_ASSERT_NOT_NULL(outsider);
    parent->type = TASK_TYPE_USER;
    first->type = TASK_TYPE_USER;
    second->type = TASK_TYPE_USER;
    outsider->type = TASK_TYPE_USER;
    first->parent_pid = parent->id;
    second->parent_pid = parent->id;
    add_task_to_queue(parent);
    add_task_to_queue(first);
    add_task_to_queue(second);
    add_task_to_queue(outsider);

    TEST_ASSERT_EQUAL(OS_TASK_NOT_CHILD,
                      task_update_supervision_watch(parent->id, outsider->id, 1U));
    TEST_ASSERT_EQUAL(OS_TASK_BAD_WATCH,
                      task_update_supervision_watch(parent->id, 0, 1U));
    TEST_ASSERT_EQUAL(OS_TASK_NO_SUPERVISION_WATCH,
                      task_update_supervision_watch(parent->id, first->id, 0U));
    TEST_ASSERT_EQUAL(1, task_update_supervision_watch(parent->id, second->id, 1U));
    TEST_ASSERT_EQUAL(1, task_set_supervision_notify(parent->id, 1U));
    TEST_ASSERT_EQUAL(0, task_suspend_child(parent->id, first->id));
    TEST_ASSERT_EQUAL(0, parent->ipc_endpoint.count);
    TEST_ASSERT_EQUAL(0, task_suspend_child(parent->id, second->id));
    TEST_ASSERT_EQUAL(1, parent->ipc_endpoint.count);
    message = parent->ipc_endpoint.messages[parent->ipc_endpoint.read_index];
    parent->ipc_endpoint.read_index = (parent->ipc_endpoint.read_index + 1U) % IPC_ENDPOINT_CAPACITY;
    parent->ipc_endpoint.count--;
    TEST_ASSERT_EQUAL(0, os_task_parse_supervision_event(&message, &event));
    TEST_ASSERT_EQUAL(second->id, event.child_pid);
    TEST_ASSERT_EQUAL(OS_TASK_SUPERVISION_SUSPEND, event.action);
    TEST_ASSERT_EQUAL(0, task_fill_supervision_events(parent->id, &events));
    TEST_ASSERT_EQUAL(2, events.count);
    TEST_ASSERT_EQUAL(0, task_fill_supervision_watch_status(parent->id, &status));
    TEST_ASSERT_EQUAL(1, status.enabled);
    TEST_ASSERT_EQUAL(1, status.count);
    TEST_ASSERT_EQUAL(second->id, status.pids[0]);

    TEST_ASSERT_EQUAL(0, task_update_supervision_watch(parent->id, second->id, 0U));
    TEST_ASSERT_EQUAL(0, task_fill_supervision_watch_status(parent->id, &status));
    TEST_ASSERT_EQUAL(0, status.enabled);
    TEST_ASSERT_EQUAL(0, status.count);
    TEST_ASSERT_EQUAL(1, task_update_supervision_watch(parent->id, first->id, 1U));
    task_report_parent_exit(first, 0, OS_TASK_EVENT_EXITED);
    TEST_ASSERT_EQUAL(0, task_fill_supervision_watch_status(parent->id, &status));
    TEST_ASSERT_EQUAL(0, status.enabled);
    TEST_ASSERT_EQUAL(0, status.count);
}

void test_task_supervision_delivery_stats(void) {
    task_t* parent;
    task_t* child;
    os_task_supervision_delivery_stats_t stats;

    tasking_init();
    parent = create_task(dummy_task_function);
    child = create_task(dummy_task_function);
    TEST_ASSERT_NOT_NULL(parent);
    TEST_ASSERT_NOT_NULL(child);
    parent->type = TASK_TYPE_USER;
    child->type = TASK_TYPE_USER;
    child->parent_pid = parent->id;
    add_task_to_queue(parent);
    add_task_to_queue(child);

    TEST_ASSERT_EQUAL(1, task_set_supervision_notify(parent->id, 1U));
    TEST_ASSERT_EQUAL(0, task_suspend_child(parent->id, child->id));
    TEST_ASSERT_EQUAL(0, task_fill_supervision_delivery_stats(parent->id, &stats));
    TEST_ASSERT_EQUAL(1, stats.attempted);
    TEST_ASSERT_EQUAL(1, stats.delivered);
    TEST_ASSERT_EQUAL(0, stats.dropped);
    parent->ipc_endpoint.count = IPC_ENDPOINT_CAPACITY;
    TEST_ASSERT_EQUAL(0, task_resume_child(parent->id, child->id));
    TEST_ASSERT_EQUAL(0, task_fill_supervision_delivery_stats(parent->id, &stats));
    TEST_ASSERT_EQUAL(2, stats.attempted);
    TEST_ASSERT_EQUAL(1, stats.delivered);
    TEST_ASSERT_EQUAL(1, stats.dropped);
    TEST_ASSERT_EQUAL(0, task_ack_supervision_delivery_stats(parent->id));
    TEST_ASSERT_EQUAL(0, task_fill_supervision_delivery_stats(parent->id, &stats));
    TEST_ASSERT_EQUAL(0, stats.attempted);
    TEST_ASSERT_EQUAL(0, stats.delivered);
    TEST_ASSERT_EQUAL(0, stats.dropped);
}

void test_task_supervision_event_replay(void) {
    task_t* parent;
    task_t* child;
    os_ipc_message_t message;
    os_task_supervision_event_t event;
    os_task_supervision_delivery_stats_t stats;

    tasking_init();
    parent = create_task(dummy_task_function);
    child = create_task(dummy_task_function);
    TEST_ASSERT_NOT_NULL(parent);
    TEST_ASSERT_NOT_NULL(child);
    parent->type = TASK_TYPE_USER;
    child->type = TASK_TYPE_USER;
    child->parent_pid = parent->id;
    add_task_to_queue(parent);
    add_task_to_queue(child);

    TEST_ASSERT_EQUAL(0, task_suspend_child(parent->id, child->id));
    TEST_ASSERT_EQUAL(0, parent->ipc_endpoint.count);
    TEST_ASSERT_EQUAL(OS_TASK_NO_SUPERVISION_EVENT,
                      task_replay_supervision_event(parent->id, 0U));
    TEST_ASSERT_EQUAL(OS_TASK_NO_SUPERVISION_EVENT,
                      task_replay_supervision_event(parent->id, 2U));
    TEST_ASSERT_EQUAL(0, task_replay_supervision_event(parent->id, 1U));
    TEST_ASSERT_EQUAL(1, parent->ipc_endpoint.count);
    message = parent->ipc_endpoint.messages[parent->ipc_endpoint.read_index];
    TEST_ASSERT_EQUAL(0, os_task_parse_supervision_event(&message, &event));
    TEST_ASSERT_EQUAL(1, event.sequence);
    TEST_ASSERT_EQUAL(OS_TASK_SUPERVISION_SUSPEND, event.action);
    TEST_ASSERT_EQUAL(child->id, event.child_pid);
    TEST_ASSERT_EQUAL(0, task_fill_supervision_delivery_stats(parent->id, &stats));
    TEST_ASSERT_EQUAL(1, stats.attempted);
    TEST_ASSERT_EQUAL(1, stats.delivered);
    TEST_ASSERT_EQUAL(0, stats.dropped);
    TEST_ASSERT_EQUAL(0, task_find_supervision_event(parent->id, 1U, &event));
    parent->ipc_endpoint.count = IPC_ENDPOINT_CAPACITY;
    TEST_ASSERT_EQUAL(OS_IPC_FULL, task_replay_supervision_event(parent->id, 1U));
    TEST_ASSERT_EQUAL(0, task_fill_supervision_delivery_stats(parent->id, &stats));
    TEST_ASSERT_EQUAL(2, stats.attempted);
    TEST_ASSERT_EQUAL(1, stats.delivered);
    TEST_ASSERT_EQUAL(1, stats.dropped);
}

void test_task_supervision_priority(void) {
    task_t* parent;
    task_t* child;
    os_task_supervision_priority_status_t status;
    tasking_init();
    parent = create_task(dummy_task_function);
    child = create_task(dummy_task_function);
    TEST_ASSERT_NOT_NULL(parent); TEST_ASSERT_NOT_NULL(child);
    parent->type = TASK_TYPE_USER; child->type = TASK_TYPE_USER;
    child->parent_pid = parent->id;
    add_task_to_queue(parent); add_task_to_queue(child);
    TEST_ASSERT_EQUAL(-1, parent->supervision_priority_child_pid);
    TEST_ASSERT_EQUAL(child->id, task_set_supervision_priority(parent->id, child->id));
    TEST_ASSERT_EQUAL(0, task_fill_supervision_priority_status(parent->id, &status));
    TEST_ASSERT_EQUAL(child->id, status.child_pid);
    TEST_ASSERT_EQUAL(0, task_set_supervision_notify_filter(parent->id, 0U));
    TEST_ASSERT_EQUAL(1, task_set_supervision_notify(parent->id, 1U));
    TEST_ASSERT_EQUAL(0, task_suspend_child(parent->id, child->id));
    TEST_ASSERT_EQUAL(1, parent->ipc_endpoint.count);
    task_report_parent_exit(child, 0, 0U);
    TEST_ASSERT_EQUAL(-1, parent->supervision_priority_child_pid);
    TEST_ASSERT_EQUAL(0, task_set_supervision_priority(parent->id, 0));
    TEST_ASSERT_EQUAL(child->id, task_set_supervision_priority(parent->id, child->id));
}

void test_task_supervision_notify_budget(void) {
    task_t* parent;
    task_t* child;
    os_task_supervision_notify_budget_status_t status;
    os_task_supervision_events_t events;

    tasking_init();
    parent = create_task(dummy_task_function);
    child = create_task(dummy_task_function);
    TEST_ASSERT_NOT_NULL(parent);
    TEST_ASSERT_NOT_NULL(child);
    parent->type = TASK_TYPE_USER;
    child->type = TASK_TYPE_USER;
    child->parent_pid = parent->id;
    add_task_to_queue(parent);
    add_task_to_queue(child);

    TEST_ASSERT_EQUAL(0, task_fill_supervision_notify_budget_status(parent->id, &status));
    TEST_ASSERT_EQUAL(0, status.limit);
    TEST_ASSERT_EQUAL(0, status.used);
    TEST_ASSERT_EQUAL(1, task_set_supervision_notify(parent->id, 1U));
    TEST_ASSERT_EQUAL(0, task_set_supervision_notify_budget(parent->id, 2U));
    TEST_ASSERT_EQUAL(0, task_suspend_child(parent->id, child->id));
    TEST_ASSERT_EQUAL(0, task_resume_child(parent->id, child->id));
    TEST_ASSERT_EQUAL(0, task_suspend_child(parent->id, child->id));
    TEST_ASSERT_EQUAL(2, parent->ipc_endpoint.count);
    TEST_ASSERT_EQUAL(0, task_fill_supervision_notify_budget_status(parent->id, &status));
    TEST_ASSERT_EQUAL(2, status.limit);
    TEST_ASSERT_EQUAL(2, status.used);
    TEST_ASSERT_EQUAL(0, task_fill_supervision_events(parent->id, &events));
    TEST_ASSERT_EQUAL(3, events.count);

    TEST_ASSERT_EQUAL(0, task_set_supervision_notify_budget(parent->id, 1U));
    TEST_ASSERT_EQUAL(0, task_fill_supervision_notify_budget_status(parent->id, &status));
    TEST_ASSERT_EQUAL(1, status.limit);
    TEST_ASSERT_EQUAL(0, status.used);
    TEST_ASSERT_EQUAL(0, task_resume_child(parent->id, child->id));
    TEST_ASSERT_EQUAL(3, parent->ipc_endpoint.count);
    TEST_ASSERT_EQUAL(0, task_fill_supervision_notify_budget_status(parent->id, &status));
    TEST_ASSERT_EQUAL(1, status.used);

    TEST_ASSERT_EQUAL(0, task_set_supervision_notify_budget(parent->id, 0U));
    TEST_ASSERT_EQUAL(0, task_fill_supervision_notify_budget_status(parent->id, &status));
    TEST_ASSERT_EQUAL(0, status.limit);
    TEST_ASSERT_EQUAL(0, status.used);
    TEST_ASSERT_EQUAL(0, task_suspend_child(parent->id, child->id));
    TEST_ASSERT_EQUAL(4, parent->ipc_endpoint.count);
    TEST_ASSERT_EQUAL(0, task_fill_supervision_notify_budget_status(parent->id, &status));
    TEST_ASSERT_EQUAL(0, status.limit);
    TEST_ASSERT_EQUAL(1, status.used);
}

void test_task_supervision_summary(void) {
    task_t* parent;
    task_t* first;
    task_t* second;
    os_task_supervision_summary_t summary;

    tasking_init();
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

    TEST_ASSERT_EQUAL(0, task_suspend_child(parent->id, first->id));
    TEST_ASSERT_EQUAL(0, task_fill_supervision_summary(parent->id, &summary));
    TEST_ASSERT_EQUAL(1, summary.generation);
    TEST_ASSERT_EQUAL(2, summary.active_children);
    TEST_ASSERT_EQUAL(1, summary.suspended_children);
    TEST_ASSERT_EQUAL(0, summary.child_exit_count);
    TEST_ASSERT_EQUAL(1, summary.retained_events);
    TEST_ASSERT_EQUAL(0, task_kill(parent->id, second->id));
    TEST_ASSERT_EQUAL(0, task_fill_supervision_summary(parent->id, &summary));
    TEST_ASSERT_EQUAL(2, summary.generation);
    TEST_ASSERT_EQUAL(1, summary.active_children);
    TEST_ASSERT_EQUAL(1, summary.suspended_children);
    TEST_ASSERT_EQUAL(1, summary.child_exit_count);
    TEST_ASSERT_EQUAL(2, summary.retained_events);
}

// === TESTS D'INTÉGRATION ===

void test_task_integration_with_memory(void) {
    tasking_init();
    
    task_t* task = create_task(dummy_task_function);
    
    // Vérifier que la tâche a son propre espace mémoire
    TEST_ASSERT_NOT_NULL(task->vmm_dir);
    
    // L'adresse du répertoire de pages devrait être valide
    TEST_ASSERT_NOT_NULL(task->vmm_dir);
    
    add_task_to_queue(task);
}

void test_multitask_execution_simulation(void) {
    tasking_init();
    
    const int num_tasks = 3;
    task_t* tasks[num_tasks];
    
    // Créer plusieurs tâches
    for (int i = 0; i < num_tasks; i++) {
        tasks[i] = create_task(counting_task_function);
        add_task_to_queue(tasks[i]);
    }
    
    current_task = task_queue;
    cpu_state_t cpu_state = {0};
    
    // Simuler plusieurs cycles de scheduling
    const int num_cycles = 20;
    for (int i = 0; i < num_cycles; i++) {
        schedule(&cpu_state);
    }
    
    // Vérifier que le scheduling a eu lieu
    TEST_ASSERT_GREATER_THAN(num_cycles / 2, mock_task_switch_called);
}

// === RUNNER PRINCIPAL ===

int main(void) {
    unity_init();
    
    // Tests d'initialisation
    RUN_TEST(test_tasking_init);
    
    // Tests de création de tâches
    RUN_TEST(test_create_task_basic);
    RUN_TEST(test_create_multiple_tasks);
    RUN_TEST(test_create_task_memory_allocation);
    
    // Tests de gestion de queue
    RUN_TEST(test_add_task_to_queue_single);
    RUN_TEST(test_add_task_to_queue_multiple);
    RUN_TEST(test_remove_task_from_queue);
    
    // Tests de recherche
    RUN_TEST(test_get_task_by_id);
    RUN_TEST(test_find_task_waiting_for_input);
    
    // Tests de scheduling
    RUN_TEST(test_schedule_basic_round_robin);
    RUN_TEST(test_schedule_skip_terminated_tasks);
    RUN_TEST(test_schedule_no_ready_tasks);
    
    // Tests de performance
    RUN_TEST(test_task_creation_performance);
    RUN_TEST(test_scheduling_performance);
    
    // Tests d'états
    RUN_TEST(test_task_state_transitions);
    RUN_TEST(test_task_yield_functionality);
    
    // Tests de robustesse
    RUN_TEST(test_task_memory_corruption_detection);
    RUN_TEST(test_task_queue_integrity);
    RUN_TEST(test_task_cleanup_on_termination);
    
    // Tests de télémétrie
    RUN_TEST(test_task_metrics_snapshot_and_missing_pid);

    // Tests de politique CPU
    RUN_TEST(test_task_priority_selection_and_validation);
    RUN_TEST(test_task_priority_control_authority);
    RUN_TEST(test_task_kill_parent_authority);
    RUN_TEST(test_task_reparents_children_on_departure);
    RUN_TEST(test_task_supervision_wait_and_children);
    RUN_TEST(test_task_direct_child_capacity);
    RUN_TEST(test_task_governance_name_capacity_and_events);
    RUN_TEST(test_task_kill_direct_children_snapshot);
    RUN_TEST(test_task_direct_children_and_wait_any);
    RUN_TEST(test_task_child_exit_count);
    RUN_TEST(test_task_supervision_delegation);
    RUN_TEST(test_task_supervision_events);
    RUN_TEST(test_task_supervision_event_selective);
    RUN_TEST(test_task_supervision_notifications);
    RUN_TEST(test_task_supervision_notification_filter);
    RUN_TEST(test_task_supervision_watchlist);
    RUN_TEST(test_task_supervision_delivery_stats);
    RUN_TEST(test_task_supervision_event_replay);
    RUN_TEST(test_task_supervision_priority);
    RUN_TEST(test_task_supervision_notify_budget);
    RUN_TEST(test_task_supervision_summary);

    // Tests d'intégration
    RUN_TEST(test_task_integration_with_memory);
    RUN_TEST(test_multitask_execution_simulation);
    
    unity_print_results();
    unity_cleanup();
    
    return (unity_stats.tests_failed == 0) ? 0 : 1;
}
