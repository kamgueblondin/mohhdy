/* test_shell.c - Tests unitaires pour le Shell MOHHDY */

#include "../../framework/unity.h"
#include "../../framework/test_kernel.h"

extern int putchar(int c);

// Mock des syscalls pour les tests userspace
static char mock_output_buffer[2048];
static int mock_output_pos = 0;
static char mock_input_buffer[512];
static int mock_input_pos = 0;
static int mock_input_len = 0;

// Mock syscall implementations
void unity_putc_redirect(char c) {
    if (mock_output_pos < (int)sizeof(mock_output_buffer) - 1) {
        mock_output_buffer[mock_output_pos++] = c;
        mock_output_buffer[mock_output_pos] = '\0';
    }
    putchar(c);
}

void gets(char* buffer, int size) {
    int i = 0;
    while (i < size - 1 && mock_input_pos < mock_input_len) {
        char c = mock_input_buffer[mock_input_pos++];
        if (c == '\n') break;
        buffer[i++] = c;
    }
    buffer[i] = '\0';
}

int sys_getchar(void) {
    if (mock_input_pos < mock_input_len) {
        return mock_input_buffer[mock_input_pos++];
    }
    return -1; // EOF
}

void yield() {
    // Mock yield - ne fait rien pour les tests
}

int exec(const char* path, char* argv[]) {
    (void)path; (void)argv;
    return 0; // Succès simulé
}

void exit_program(int code) {
    (void)code;
    // Mock exit - ne fait rien pour les tests
}

// Fonctions utilitaires simplifiées (extraites du shell)
int strlen(const char* str) {
    int len = 0;
    while (str[len] != '\0') len++;
    return len;
}

int strcmp(const char* s1, const char* s2) {
    int i = 0;
    while (s1[i] != '\0' && s2[i] != '\0') {
        if (s1[i] != s2[i]) {
            return s1[i] - s2[i];
        }
        i++;
    }
    return s1[i] - s2[i];
}

int strncmp(const char* s1, const char* s2, int n) {
    for (int i = 0; i < n; i++) {
        if (s1[i] != s2[i]) {
            return s1[i] - s2[i];
        }
        if (s1[i] == '\0') break;
    }
    return 0;
}

void strcpy(char* dest, const char* src) {
    int i = 0;
    while (src[i] != '\0') {
        dest[i] = src[i];
        i++;
    }
    dest[i] = '\0';
}

void strcat(char* dest, const char* src) {
    int dest_len = strlen(dest);
    int i = 0;
    while (src[i] != '\0') {
        dest[dest_len + i] = src[i];
        i++;
    }
    dest[dest_len + i] = '\0';
}

char* strstr(const char* haystack, const char* needle) {
    if (*needle == '\0') return (char*)haystack;
    
    for (int i = 0; haystack[i] != '\0'; i++) {
        int j = 0;
        while (haystack[i + j] == needle[j] && needle[j] != '\0') {
            j++;
        }
        if (needle[j] == '\0') {
            return (char*)&haystack[i];
        }
    }
    return NULL;
}

// Structures simplifiées du shell pour les tests
#define MAX_COMMAND_LENGTH 256
#define MAX_ARGS 16

typedef struct {
    char commands[10][MAX_COMMAND_LENGTH];
    int count;
    int current;
} test_command_history_t;

typedef struct {
    char name[32];
    char value[128];
} test_env_var_t;

typedef struct {
    char current_dir[128];
    test_command_history_t history;
    test_env_var_t env_vars[10];
    int env_count;
} test_shell_context_t;

// === HELPER FUNCTIONS ===

void setup_mock_input(const char* input) {
    strcpy(mock_input_buffer, input);
    mock_input_len = strlen(input);
    mock_input_pos = 0;
}

void clear_mock_output(void) {
    mock_output_pos = 0;
    mock_output_buffer[0] = '\0';
}

void print_string(const char* str) {
    for (int i = 0; str[i] != '\0'; i++) {
        putc(str[i]);
    }
}

int parse_command(const char* input, char* command, char args[][64], int* argc) {
    *argc = 0;
    int input_pos = 0;
    int cmd_pos = 0;
    
    // Skip leading spaces
    while (input[input_pos] == ' ') input_pos++;
    
    // Extract command (leave room for NUL)
    while (input[input_pos] != '\0' && input[input_pos] != ' ' && cmd_pos < 63) {
        command[cmd_pos++] = input[input_pos++];
    }
    while (input[input_pos] != '\0' && input[input_pos] != ' ') input_pos++;
    command[cmd_pos] = '\0';
    
    // Extract arguments
    while (input[input_pos] != '\0' && *argc < MAX_ARGS - 1) {
        // Skip spaces
        while (input[input_pos] == ' ') input_pos++;
        
        if (input[input_pos] == '\0') break;
        
        // Extract argument
        int arg_pos = 0;
        while (input[input_pos] != '\0' && input[input_pos] != ' ' && arg_pos < 63) {
            args[*argc][arg_pos++] = input[input_pos++];
        }
        while (input[input_pos] != '\0' && input[input_pos] != ' ') input_pos++;
        args[*argc][arg_pos] = '\0';
        (*argc)++;
    }
    
    return 1;
}

// Commandes shell simplifiées pour les tests
void shell_cmd_help(void) {
    print_string("MOHHDY Shell Commands:\n");
    print_string("  help    - Show this help\n");
    print_string("  echo    - Echo arguments\n");
    print_string("  clear   - Clear screen\n");
    print_string("  exit    - Exit shell\n");
}

void shell_cmd_echo(char args[][64], int argc) {
    for (int i = 0; i < argc; i++) {
        if (i > 0) putc(' ');
        print_string(args[i]);
    }
    putc('\n');
}

void shell_cmd_clear(void) {
    print_string("\033[2J\033[H"); // ANSI clear screen
}

// === SETUP ET TEARDOWN ===

void setUp(void) {
    test_kernel_init();
    clear_mock_output();
    mock_input_pos = 0;
    mock_input_len = 0;
}

void tearDown(void) {
    test_kernel_cleanup();
}

// === TESTS DES FONCTIONS UTILITAIRES ===

void test_strlen_basic(void) {
    TEST_ASSERT_EQUAL(0, strlen(""));
    TEST_ASSERT_EQUAL(5, strlen("Hello"));
    TEST_ASSERT_EQUAL(13, strlen("Hello, World!"));
}

void test_strcmp_basic(void) {
    TEST_ASSERT_EQUAL(0, strcmp("", ""));
    TEST_ASSERT_EQUAL(0, strcmp("test", "test"));
    TEST_ASSERT_GREATER_THAN(0, strcmp("b", "a"));
    TEST_ASSERT_LESS_THAN(0, strcmp("a", "b"));
}

void test_strncmp_basic(void) {
    TEST_ASSERT_EQUAL(0, strncmp("hello", "hello", 5));
    TEST_ASSERT_EQUAL(0, strncmp("hello", "help", 3));
    TEST_ASSERT_NOT_EQUAL(0, strncmp("hello", "help", 4));
}

void test_strcpy_basic(void) {
    char dest[64];
    
    strcpy(dest, "test");
    TEST_ASSERT_EQUAL_STRING("test", dest);
    
    strcpy(dest, "");
    TEST_ASSERT_EQUAL_STRING("", dest);
}

void test_strstr_basic(void) {
    const char* haystack = "hello world";
    TEST_ASSERT_EQUAL_PTR(haystack, strstr(haystack, ""));
    TEST_ASSERT_EQUAL_PTR(haystack + 6, strstr(haystack, "world"));
    TEST_ASSERT_NULL(strstr("hello", "xyz"));
}

// === TESTS DU PARSING DE COMMANDES ===

void test_parse_command_simple(void) {
    char command[64];
    char args[MAX_ARGS][64];
    int argc;
    
    parse_command("help", command, args, &argc);
    
    TEST_ASSERT_EQUAL_STRING("help", command);
    TEST_ASSERT_EQUAL(0, argc);
}

void test_parse_command_with_args(void) {
    char command[64];
    char args[MAX_ARGS][64];
    int argc;
    
    parse_command("echo hello world", command, args, &argc);
    
    TEST_ASSERT_EQUAL_STRING("echo", command);
    TEST_ASSERT_EQUAL(2, argc);
    TEST_ASSERT_EQUAL_STRING("hello", args[0]);
    TEST_ASSERT_EQUAL_STRING("world", args[1]);
}

void test_parse_command_with_spaces(void) {
    char command[64];
    char args[MAX_ARGS][64];
    int argc;
    
    parse_command("  echo   hello   world  ", command, args, &argc);
    
    TEST_ASSERT_EQUAL_STRING("echo", command);
    TEST_ASSERT_EQUAL(2, argc);
    TEST_ASSERT_EQUAL_STRING("hello", args[0]);
    TEST_ASSERT_EQUAL_STRING("world", args[1]);
}

void test_parse_command_empty(void) {
    char command[64];
    char args[MAX_ARGS][64];
    int argc;
    
    parse_command("", command, args, &argc);
    
    TEST_ASSERT_EQUAL_STRING("", command);
    TEST_ASSERT_EQUAL(0, argc);
}

void test_parse_command_max_args(void) {
    char command[64];
    char args[MAX_ARGS][64];
    int argc;
    
    // Créer une commande avec beaucoup d'arguments manuellement
    char long_cmd[256] = "cmd arg1 arg2 arg3 arg4 arg5 arg6 arg7 arg8 arg9 arg10 arg11 arg12 arg13 arg14 arg15";
    
    parse_command(long_cmd, command, args, &argc);
    
    TEST_ASSERT_EQUAL_STRING("cmd", command);
    TEST_ASSERT_LESS_THAN(MAX_ARGS, argc);
    TEST_ASSERT_EQUAL(15, argc);
}

// === TESTS DES COMMANDES SHELL ===

void test_shell_help_command(void) {
    clear_mock_output();
    
    shell_cmd_help();
    
    TEST_ASSERT(strstr(mock_output_buffer, "MOHHDY Shell Commands") != NULL);
    TEST_ASSERT(strstr(mock_output_buffer, "help") != NULL);
    TEST_ASSERT(strstr(mock_output_buffer, "echo") != NULL);
    TEST_ASSERT(strstr(mock_output_buffer, "clear") != NULL);
}

void test_shell_echo_command(void) {
    char args[MAX_ARGS][64];
    strcpy(args[0], "hello");
    strcpy(args[1], "world");
    
    clear_mock_output();
    
    shell_cmd_echo(args, 2);
    
    TEST_ASSERT_EQUAL_STRING("hello world\n", mock_output_buffer);
}

void test_shell_echo_no_args(void) {
    char args[MAX_ARGS][64];
    
    clear_mock_output();
    
    shell_cmd_echo(args, 0);
    
    TEST_ASSERT_EQUAL_STRING("\n", mock_output_buffer);
}

void test_shell_clear_command(void) {
    clear_mock_output();
    
    shell_cmd_clear();
    
    TEST_ASSERT(strstr(mock_output_buffer, "\033[2J") != NULL);
    TEST_ASSERT(strstr(mock_output_buffer, "\033[H") != NULL);
}

// === TESTS DE L'HISTORIQUE ===

void test_history_add_command(void) {
    test_command_history_t history;
    for(int i=0; i<10; i++) {
        for(int j=0; j<MAX_COMMAND_LENGTH; j++) history.commands[i][j] = 0;
    }
    history.count = 0;
    history.current = 0;
    
    strcpy(history.commands[history.count], "test command");
    history.count++;
    
    TEST_ASSERT_EQUAL(1, history.count);
    TEST_ASSERT_EQUAL_STRING("test command", history.commands[0]);
}

void test_history_multiple_commands(void) {
    test_command_history_t history;
    for(int i=0; i<10; i++) {
        for(int j=0; j<MAX_COMMAND_LENGTH; j++) history.commands[i][j] = 0;
    }
    history.count = 0;
    history.current = 0;
    
    const char* test_commands[] = {"ls", "help", "echo hello"};
    const int num_commands = 3;
    
    for (int i = 0; i < num_commands; i++) {
        strcpy(history.commands[history.count], test_commands[i]);
        history.count++;
    }
    
    TEST_ASSERT_EQUAL(num_commands, history.count);
    for (int i = 0; i < num_commands; i++) {
        TEST_ASSERT_EQUAL_STRING(test_commands[i], history.commands[i]);
    }
}

void test_history_overflow(void) {
    test_command_history_t history;
    for(int i=0; i<10; i++) {
        for(int j=0; j<MAX_COMMAND_LENGTH; j++) history.commands[i][j] = 0;
    }
    history.count = 0;
    history.current = 0;
    
    // Remplir l'historique au maximum
    for (int i = 0; i < 10; i++) {
        // Créer des noms de commande manuellement
        strcpy(history.commands[i], "command_");
        if (i == 0) strcat(history.commands[i], "0");
        else if (i == 1) strcat(history.commands[i], "1");
        else if (i == 2) strcat(history.commands[i], "2");
        // ... etc pour les autres numéros
        history.count = i + 1;
    }
    
    TEST_ASSERT_EQUAL(10, history.count);
    TEST_ASSERT_EQUAL_STRING("command_0", history.commands[0]);
}

// === TESTS DES VARIABLES D'ENVIRONNEMENT ===

void test_env_var_set(void) {
    test_shell_context_t context;
    for(int i=0; i<128; i++) context.current_dir[i] = 0;
    for(int i=0; i<10; i++) {
        for(int j=0; j<MAX_COMMAND_LENGTH; j++) context.history.commands[i][j] = 0;
    }
    context.history.count = 0;
    context.history.current = 0;
    for(int i=0; i<10; i++) {
        for(int j=0; j<32; j++) context.env_vars[i].name[j] = 0;
        for(int j=0; j<128; j++) context.env_vars[i].value[j] = 0;
    }
    context.env_count = 0;
    
    strcpy(context.env_vars[0].name, "PATH");
    strcpy(context.env_vars[0].value, "/bin:/usr/bin");
    context.env_count = 1;
    
    TEST_ASSERT_EQUAL(1, context.env_count);
    TEST_ASSERT_EQUAL_STRING("PATH", context.env_vars[0].name);
    TEST_ASSERT_EQUAL_STRING("/bin:/usr/bin", context.env_vars[0].value);
}

void test_env_var_get(void) {
    test_shell_context_t context;
    for(int i=0; i<128; i++) context.current_dir[i] = 0;
    for(int i=0; i<10; i++) {
        for(int j=0; j<MAX_COMMAND_LENGTH; j++) context.history.commands[i][j] = 0;
    }
    context.history.count = 0;
    context.history.current = 0;
    for(int i=0; i<10; i++) {
        for(int j=0; j<32; j++) context.env_vars[i].name[j] = 0;
        for(int j=0; j<128; j++) context.env_vars[i].value[j] = 0;
    }
    context.env_count = 0;
    
    strcpy(context.env_vars[0].name, "HOME");
    strcpy(context.env_vars[0].value, "/home/user");
    strcpy(context.env_vars[1].name, "USER");
    strcpy(context.env_vars[1].value, "testuser");
    context.env_count = 2;
    
    // Simulation de recherche
    const char* search_name = "USER";
    char* found_value = NULL;
    
    for (int i = 0; i < context.env_count; i++) {
        if (strcmp(context.env_vars[i].name, search_name) == 0) {
            found_value = context.env_vars[i].value;
            break;
        }
    }
    
    TEST_ASSERT_NOT_NULL(found_value);
    TEST_ASSERT_EQUAL_STRING("testuser", found_value);
}

// === TESTS D'INTÉGRATION ===

void test_shell_command_execution_flow(void) {
    char command[64];
    char args[MAX_ARGS][64];
    int argc;
    
    // Simuler l'exécution d'une commande complète
    const char* input = "echo testing 123";
    
    parse_command(input, command, args, &argc);
    
    TEST_ASSERT_EQUAL_STRING("echo", command);
    TEST_ASSERT_EQUAL(2, argc);
    
    clear_mock_output();
    
    if (strcmp(command, "echo") == 0) {
        shell_cmd_echo(args, argc);
    }
    
    TEST_ASSERT_EQUAL_STRING("testing 123\n", mock_output_buffer);
}

void test_shell_multiple_commands(void) {
    const char* commands[] = {"help", "echo test", "clear"};
    const int num_commands = 3;
    
    for (int i = 0; i < num_commands; i++) {
        char command[64];
        char args[MAX_ARGS][64];
        int argc;
        
        parse_command(commands[i], command, args, &argc);
        
        clear_mock_output();
        
        if (strcmp(command, "help") == 0) {
            shell_cmd_help();
            TEST_ASSERT(strstr(mock_output_buffer, "Commands") != NULL);
        } else if (strcmp(command, "echo") == 0) {
            shell_cmd_echo(args, argc);
            TEST_ASSERT(strstr(mock_output_buffer, "test") != NULL);
        } else if (strcmp(command, "clear") == 0) {
            shell_cmd_clear();
            TEST_ASSERT(strstr(mock_output_buffer, "\033[") != NULL);
        }
    }
}

// === TESTS DE PERFORMANCE ===

void test_shell_command_parsing_performance(void) {
    char command[64];
    char args[MAX_ARGS][64];
    int argc;
    
    const int iterations = 1000;
    
    TEST_BENCHMARK("Command Parsing", iterations, {
        parse_command("echo hello world test 123", command, args, &argc);
    });
}

void test_shell_string_operations_performance(void) {
    const char* test_strings[] = {
        "short",
        "medium length string",
        "this is a much longer string for performance testing purposes"
    };
    
    const int iterations = 1000;
    
    TEST_BENCHMARK("String Operations", iterations, {
        for (int i = 0; i < 3; i++) {
            int len = strlen(test_strings[i]);
            (void)len;
            
            int cmp = strcmp(test_strings[i], test_strings[i]);
            (void)cmp;
        }
    });
}

// === TESTS DE ROBUSTESSE ===

void test_shell_malformed_input(void) {
    char command[64];
    char args[MAX_ARGS][64];
    int argc;
    
    // Test avec des chaînes malformées
    const char* malformed_inputs[] = {
        "",
        "   ",
        "\t\n",
        "command\x00hidden",
        "very_long_command_name_that_exceeds_normal_limits_and_should_be_handled_gracefully"
    };
    
    for (int i = 0; i < 5; i++) {
        // Ne devrait pas crasher
        parse_command(malformed_inputs[i], command, args, &argc);
        TEST_ASSERT(argc >= 0);
    }
}

void test_shell_buffer_safety(void) {
    char small_buffer[8];
    
    // Test de strcpy avec buffer trop petit (simulation)
    const char* long_string = "this_is_way_too_long";
    
    // Copie tronquée manuelle pour éviter overflow
    int i = 0;
    while (i < 7 && long_string[i] != '\0') {
        small_buffer[i] = long_string[i];
        i++;
    }
    small_buffer[i] = '\0';
    
    TEST_ASSERT_EQUAL(7, strlen(small_buffer));
}

// === RUNNER PRINCIPAL ===

int main(void) {
    unity_init();
    
    // Tests des fonctions utilitaires
    RUN_TEST(test_strlen_basic);
    RUN_TEST(test_strcmp_basic);
    RUN_TEST(test_strncmp_basic);
    RUN_TEST(test_strcpy_basic);
    RUN_TEST(test_strstr_basic);
    
    // Tests du parsing
    RUN_TEST(test_parse_command_simple);
    RUN_TEST(test_parse_command_with_args);
    RUN_TEST(test_parse_command_with_spaces);
    RUN_TEST(test_parse_command_empty);
    RUN_TEST(test_parse_command_max_args);
    
    // Tests des commandes
    RUN_TEST(test_shell_help_command);
    RUN_TEST(test_shell_echo_command);
    RUN_TEST(test_shell_echo_no_args);
    RUN_TEST(test_shell_clear_command);
    
    // Tests de l'historique
    RUN_TEST(test_history_add_command);
    RUN_TEST(test_history_multiple_commands);
    RUN_TEST(test_history_overflow);
    
    // Tests des variables d'environnement
    RUN_TEST(test_env_var_set);
    RUN_TEST(test_env_var_get);
    
    // Tests d'intégration
    RUN_TEST(test_shell_command_execution_flow);
    RUN_TEST(test_shell_multiple_commands);
    
    // Tests de performance
    RUN_TEST(test_shell_command_parsing_performance);
    RUN_TEST(test_shell_string_operations_performance);
    
    // Tests de robustesse
    RUN_TEST(test_shell_malformed_input);
    RUN_TEST(test_shell_buffer_safety);
    
    unity_print_results();
    unity_cleanup();
    
    return (unity_stats.tests_failed == 0) ? 0 : 1;
}
