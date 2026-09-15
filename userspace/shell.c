// shell.c - Shell Interactif Avancé pour MOHHDY v6.0
// Shell utilisateur complet avec IA intégrée et fonctionnalités modernes

#include <stdint.h>
#include <stddef.h>
#include "ramfs.h"
#include "procsim.h"
#include "os_syscalls.h"
#include "os_vfs_service.h"
#include "os_ipc_deferred.h"

// ==============================================================================
// STRUCTURES ET DÉFINITIONS
// ==============================================================================

#define MAX_COMMAND_LENGTH 512
#define MAX_ARGS 32
#define MAX_HISTORY 50
#define MAX_PATH_LENGTH 256
#define MAX_ENV_VARS 32

/* Fournisseurs IA : la selection est persistante pendant la session du shell. */
#define AI_PROVIDER_LOCAL  0
#define AI_PROVIDER_OPENAI 1
/* Valeurs ABI réseau, maintenues sans dépendance directe au pilote noyau. */
#define AI_NETWORK_PROVIDER_OLLAMA 0U
#define AI_NETWORK_PROVIDER_OPENAI 1U

/* Premier profil local cible : modele GGUF quantifie embarque sur le support de boot. */
#define AI_DEFAULT_MODEL "gpt2_124M.bin"

// Couleurs ANSI pour un affichage moderne
#define COLOR_RESET   "\x1b[0m"
#define COLOR_RED     "\x1b[31m"
#define COLOR_GREEN   "\x1b[32m"
#define COLOR_YELLOW  "\x1b[33m"
#define COLOR_BLUE    "\x1b[34m"
#define COLOR_MAGENTA "\x1b[35m"
#define COLOR_CYAN    "\x1b[36m"
#define COLOR_WHITE   "\x1b[37m"
#define COLOR_BRIGHT  "\x1b[1m"

// Structure pour l'historique des commandes
typedef struct {
    char commands[MAX_HISTORY][MAX_COMMAND_LENGTH];
    int count;
    int current;
} command_history_t;

// Structure pour les variables d'environnement
typedef struct {
    char name[64];
    char value[256];
} env_var_t;

// Structure pour les alias de commandes
typedef struct {
    char alias[64];
    char command[256];
} alias_t;

// Structure principale du shell
typedef struct {
    char current_dir[MAX_PATH_LENGTH];
    char prompt[128];
    command_history_t history;
    env_var_t env_vars[MAX_ENV_VARS];
    alias_t aliases[MAX_ENV_VARS];
    int env_count;
    int alias_count;
    int show_colors;
    int ai_mode;
    int ai_provider;
    char ai_model[128];
    int debug_mode;
    int ai_query_count;
    int cmd_ticks;
    int last_rc;
} shell_context_t;

// ==============================================================================
// APPELS SYSTÈME ET UTILITAIRES DE BASE
// ==============================================================================

// Wrappers pour les appels système
void putc(char c) { 
    asm volatile("int $0x80" : : "a"(1), "b"(c)); 
}

void exit_program(int code) { 
    asm volatile("int $0x80" : : "a"(0), "b"(code)); 
}

void gets(char* buffer, int size) { 
    asm volatile("int $0x80" : : "a"(5), "b"(buffer), "c"(size)); 
}

int sys_getchar(void) {
    int c;
    asm volatile("int $0x80" : "=a"(c) : "a"(2));
    return c;
}

int exec(const char* path, char* argv[]) {
    int result;
    asm volatile("int $0x80" : "=a"(result) : "a"(6), "b"(path), "c"(argv));
    return result;
}

int spawn(const char* path, char* argv[]) {
    int result;
    asm volatile("int $0x80" : "=a"(result) : "a"(7), "b"(path), "c"(argv));
    return result;
}

void yield() {
    asm volatile("int $0x80" : : "a"(4));
}

int sys_listdir(const char* path, os_dirent_t* out, int max_n) {
    int result;
    asm volatile("int $0x80" : "=a"(result) : "a"(SYS_LISTDIR), "b"(path), "c"(out), "d"(max_n));
    return result;
}

int sys_readfile(const char* path, char* buf, int max) {
    int result;
    asm volatile("int $0x80" : "=a"(result) : "a"(SYS_READFILE), "b"(path), "c"(buf), "d"(max));
    return result;
}

int sys_getpid(void) {
    int result;
    asm volatile("int $0x80" : "=a"(result) : "a"(SYS_GETPID));
    return result;
}

int sys_ps(os_proc_t* out, int max_n) {
    int result;
    asm volatile("int $0x80" : "=a"(result) : "a"(SYS_PS), "b"(out), "c"(max_n));
    return result;
}

int sys_kill_pid(int pid) {
    int result;
    asm volatile("int $0x80" : "=a"(result) : "a"(SYS_KILL), "b"(pid));
    return result;
}

unsigned int sys_ticks(void) {
    unsigned int result;
    asm volatile("int $0x80" : "=a"(result) : "a"(SYS_TICKS));
    return result;
}

unsigned int sys_net_status(void) {
    unsigned int result;
    asm volatile("int $0x80" : "=a"(result) : "a"(SYS_NET_STATUS));
    return result;
}

unsigned int sys_llm_session_status(void) {
    unsigned int result;
    asm volatile("int $0x80" : "=a"(result) : "a"(SYS_LLM_SESSION_STATUS));
    return result;
}

int sys_llm_acquire_start(const os_llm_acquire_start_request_t* request) {
    int result;
    asm volatile("int $0x80" : "=a"(result) : "a"(SYS_LLM_ACQUIRE_START), "b"(request));
    return result;
}
int sys_llm_configure_openai(const os_llm_openai_credential_request_t* request) {
    int result;
    asm volatile("int $0x80" : "=a"(result) : "a"(SYS_LLM_OPENAI_CREDENTIAL), "b"(request));
    return result;
}

int sys_llm_poll_tls(void) {
    int result;
    asm volatile("int $0x80" : "=a"(result) : "a"(SYS_LLM_POLL_TLS));
    return result;
}

int sys_llm_request(const os_llm_request_t* request) {
    int result;
    asm volatile("int $0x80" : "=a"(result) : "a"(SYS_LLM_REQUEST), "b"(request));
    return result;
}

int sys_llm_poll_text(os_llm_text_result_t* result) {
    int status;
    asm volatile("int $0x80" : "=a"(status) : "a"(SYS_LLM_POLL_TEXT), "b"(result));
    return status;
}

int sys_llm_poll_sse(os_llm_text_result_t* result) {
    int status;
    asm volatile("int $0x80" : "=a"(status) : "a"(SYS_LLM_POLL_SSE), "b"(result));
    return status;
}

int sys_llm_reset_for_request(void) {
    int status;
    asm volatile("int $0x80" : "=a"(status) : "a"(SYS_LLM_RESET_FOR_REQUEST));
    return status;
}

int sys_llm_close(void) {
    int status;
    asm volatile("int $0x80" : "=a"(status) : "a"(SYS_LLM_CLOSE));
    return status;
}

int sys_meminfo(os_meminfo_t* info) {
    int result;
    asm volatile("int $0x80" : "=a"(result) : "a"(SYS_MEMINFO), "b"(info));
    return result;
}

int sys_task_metrics(int pid, os_task_metrics_t* out) {
    int result;
    asm volatile("int $0x80" : "=a"(result) : "a"(SYS_TASK_METRICS), "b"(pid), "c"(out));
    return result;
}

int sys_task_set_priority(int pid, unsigned int priority) {
    int result;
    asm volatile("int $0x80" : "=a"(result) : "a"(SYS_TASK_SET_PRIORITY), "b"(pid), "c"(priority));
    return result;
}

int sys_task_wait(int pid) {
    int result;
    asm volatile("int $0x80" : "=a"(result) : "a"(SYS_TASK_WAIT), "b"(pid));
    return result;
}

int sys_task_set_name(int pid, const char* name) {
    int result;
    asm volatile("int $0x80" : "=a"(result) : "a"(SYS_TASK_SET_NAME), "b"(pid), "c"(name));
    return result;
}

int sys_task_capacity(os_task_capacity_t* out) {
    int result;
    asm volatile("int $0x80" : "=a"(result) : "a"(SYS_TASK_CAPACITY), "b"(out));
    return result;
}

int sys_task_child_result(int pid, os_task_exit_result_t* out) {
    int result;
    asm volatile("int $0x80" : "=a"(result) : "a"(SYS_TASK_CHILD_RESULT), "b"(pid), "c"(out));
    return result;
}

int sys_task_child_result_list(os_task_exit_history_t* out) {
    int result;
    asm volatile("int $0x80" : "=a"(result) : "a"(SYS_TASK_CHILD_RESULT_LIST), "b"(out));
    return result;
}

int sys_task_child_result_ack(void) {
    int result;
    asm volatile("int $0x80" : "=a"(result) : "a"(SYS_TASK_CHILD_RESULT_ACK));
    return result;
}

int sys_task_child_result_observe(uint32_t expected, os_task_exit_history_observation_t* out) {
    int result;
    asm volatile("int $0x80" : "=a"(result) : "a"(SYS_TASK_CHILD_RESULT_OBSERVE), "b"(expected), "c"(out));
    return result;
}

int sys_task_child_result_find(int pid, os_task_exit_result_t* out) {
    int result;
    asm volatile("int $0x80" : "=a"(result) : "a"(SYS_TASK_CHILD_RESULT_FIND), "b"(pid), "c"(out));
    return result;
}

int sys_task_child_result_forget(int pid) {
    int result;
    asm volatile("int $0x80" : "=a"(result) : "a"(SYS_TASK_CHILD_RESULT_FORGET), "b"(pid));
    return result;
}

int sys_task_suspend(int pid) {
    int result;
    asm volatile("int $0x80" : "=a"(result) : "a"(SYS_TASK_SUSPEND), "b"(pid));
    return result;
}

int sys_task_resume(int pid) {
    int result;
    asm volatile("int $0x80" : "=a"(result) : "a"(SYS_TASK_RESUME), "b"(pid));
    return result;
}

int sys_task_kill_children(void) {
    int result;
    asm volatile("int $0x80" : "=a"(result) : "a"(SYS_TASK_KILL_CHILDREN));
    return result;
}

int sys_task_children(os_task_children_t* out) {
    int result;
    asm volatile("int $0x80" : "=a"(result) : "a"(SYS_TASK_CHILDREN), "b"(out));
    return result;
}

int sys_task_wait_any(void) {
    int result;
    asm volatile("int $0x80" : "=a"(result) : "a"(SYS_TASK_WAIT_ANY));
    return result;
}

int sys_task_child_exit_count(os_task_child_exit_count_t* out) {
    int result;
    asm volatile("int $0x80" : "=a"(result) : "a"(SYS_TASK_CHILD_EXIT_COUNT), "b"(out));
    return result;
}

int sys_task_delegate_child(int child_pid, int supervisor_pid) {
    int result;
    asm volatile("int $0x80" : "=a"(result) : "a"(SYS_TASK_DELEGATE_CHILD),
                 "b"(child_pid), "c"(supervisor_pid));
    return result;
}

int sys_task_supervision_events(os_task_supervision_events_t* out) {
    int result;
    asm volatile("int $0x80" : "=a"(result) : "a"(SYS_TASK_SUPERVISION_EVENTS), "b"(out));
    return result;
}

int sys_task_supervision_events_ack(void) {
    int result;
    asm volatile("int $0x80" : "=a"(result) : "a"(SYS_TASK_SUPERVISION_EVENTS_ACK));
    return result;
}

int sys_task_supervision_events_observe(uint32_t expected_generation,
                                        os_task_supervision_events_observation_t* out) {
    int result;
    asm volatile("int $0x80" : "=a"(result) : "a"(SYS_TASK_SUPERVISION_EVENTS_OBSERVE),
                 "b"(expected_generation), "c"(out));
    return result;
}

int sys_task_supervision_event_find(uint32_t sequence, os_task_supervision_event_t* out) {
    int result;
    asm volatile("int $0x80" : "=a"(result) : "a"(SYS_TASK_SUPERVISION_EVENT_FIND),
                 "b"(sequence), "c"(out));
    return result;
}

int sys_task_supervision_event_forget(uint32_t sequence) {
    int result;
    asm volatile("int $0x80" : "=a"(result) : "a"(SYS_TASK_SUPERVISION_EVENT_FORGET),
                 "b"(sequence));
    return result;
}

int sys_task_supervision_summary(os_task_supervision_summary_t* out) {
    int result;
    asm volatile("int $0x80" : "=a"(result) : "a"(SYS_TASK_SUPERVISION_SUMMARY), "b"(out));
    return result;
}

int sys_task_supervision_notify(uint32_t enabled) {
    int result;
    asm volatile("int $0x80" : "=a"(result) : "a"(SYS_TASK_SUPERVISION_NOTIFY), "b"(enabled));
    return result;
}

int sys_task_supervision_notify_filter(uint32_t mask) {
    int result;
    asm volatile("int $0x80" : "=a"(result) : "a"(SYS_TASK_SUPERVISION_NOTIFY_FILTER), "b"(mask));
    return result;
}

int sys_task_supervision_notify_status(os_task_supervision_notify_status_t* out) {
    int result;
    asm volatile("int $0x80" : "=a"(result) : "a"(SYS_TASK_SUPERVISION_NOTIFY_STATUS), "b"(out));
    return result;
}

int sys_task_supervision_watch(int child_pid, uint32_t enabled) {
    int result;
    asm volatile("int $0x80" : "=a"(result) : "a"(SYS_TASK_SUPERVISION_WATCH),
                 "b"(child_pid), "c"(enabled));
    return result;
}

int sys_task_supervision_watch_status(os_task_supervision_watch_status_t* out) {
    int result;
    asm volatile("int $0x80" : "=a"(result) : "a"(SYS_TASK_SUPERVISION_WATCH_STATUS), "b"(out));
    return result;
}

int sys_task_supervision_delivery_stats(os_task_supervision_delivery_stats_t* out) {
    int result;
    asm volatile("int $0x80" : "=a"(result) : "a"(SYS_TASK_SUPERVISION_DELIVERY_STATS), "b"(out));
    return result;
}

int sys_task_supervision_delivery_stats_ack(void) {
    int result;
    asm volatile("int $0x80" : "=a"(result) : "a"(SYS_TASK_SUPERVISION_DELIVERY_STATS_ACK));
    return result;
}

int sys_task_supervision_event_replay(uint32_t sequence) {
    int result;
    asm volatile("int $0x80" : "=a"(result) : "a"(SYS_TASK_SUPERVISION_EVENT_REPLAY), "b"(sequence));
    return result;
}

int sys_task_supervision_priority(int child_pid) {
    int result;
    asm volatile("int $0x80" : "=a"(result) : "a"(SYS_TASK_SUPERVISION_PRIORITY), "b"(child_pid));
    return result;
}

int sys_task_supervision_priority_status(os_task_supervision_priority_status_t* out) {
    int result;
    asm volatile("int $0x80" : "=a"(result) : "a"(SYS_TASK_SUPERVISION_PRIORITY_STATUS), "b"(out));
    return result;
}

int sys_task_supervision_notify_budget(uint32_t limit) {
    int result;
    asm volatile("int $0x80" : "=a"(result) : "a"(SYS_TASK_SUPERVISION_NOTIFY_BUDGET), "b"(limit));
    return result;
}

int sys_task_supervision_notify_budget_status(os_task_supervision_notify_budget_status_t* out) {
    int result;
    asm volatile("int $0x80" : "=a"(result) : "a"(SYS_TASK_SUPERVISION_NOTIFY_BUDGET_STATUS), "b"(out));
    return result;
}

int sys_fat16_read(const char* name, char* buffer, uint32_t max) {
    int result;
    asm volatile("int $0x80" : "=a"(result) : "a"(SYS_FAT16_READ), "b"(name), "c"(buffer), "d"(max));
    return result;
}

int sys_fat16_list(os_fat16_dirent_t* out, uint32_t capacity) {
    int result;
    asm volatile("int $0x80" : "=a"(result) : "a"(SYS_FAT16_LIST), "b"(out), "c"(capacity));
    return result;
}

int sys_mkdir(const char* path) {
    int result;
    asm volatile("int $0x80" : "=a"(result) : "a"(SYS_MKDIR), "b"(path));
    return result;
}

int sys_unlink(const char* path) {
    int result;
    asm volatile("int $0x80" : "=a"(result) : "a"(SYS_UNLINK), "b"(path));
    return result;
}

int sys_writefile(const char* path, const char* buf, int n) {
    int result;
    asm volatile("int $0x80" : "=a"(result) : "a"(SYS_WRITEFILE), "b"(path), "c"(buf), "d"(n));
    return result;
}

int sys_stat(const char* path, os_dirent_t* out) {
    int result;
    asm volatile("int $0x80" : "=a"(result) : "a"(SYS_STAT), "b"(path), "c"(out));
    return result;
}

int sys_rename(const char* oldpath, const char* newpath) {
    int result;
    asm volatile("int $0x80" : "=a"(result) : "a"(SYS_RENAME), "b"(oldpath), "c"(newpath));
    return result;
}

int sys_copy(const char* src, const char* dst) {
    int result;
    asm volatile("int $0x80" : "=a"(result) : "a"(SYS_COPY), "b"(src), "c"(dst));
    return result;
}

int sys_append(const char* path, const char* buf, int n) {
    int result;
    asm volatile("int $0x80" : "=a"(result) : "a"(SYS_APPEND), "b"(path), "c"(buf), "d"(n));
    return result;
}

int sys_gpt2_generate(const char* prompt, char* out, int max) {
    int result;
    asm volatile("int $0x80" : "=a"(result) : "a"(SYS_GPT2_GENERATE), "b"(prompt), "c"(out), "d"(max));
    return result;
}

int sys_gpt2_gguf_generate(const char* prompt, char* out, int max) {
    int result;
    asm volatile("int $0x80" : "=a"(result) : "a"(SYS_GPT2_GGUF_GENERATE), "b"(prompt), "c"(out), "d"(max));
    return result;
}

int sys_gpt2_gguf_continue(char* out, int max) {
    int result;
    asm volatile("int $0x80" : "=a"(result) : "a"(SYS_GPT2_GGUF_CONTINUE), "c"(out), "d"(max));
    return result;
}

int sys_ipc_send(int target_pid, const os_ipc_payload_t* payload) {
    int result;
    asm volatile("int $0x80" : "=a"(result) : "a"(SYS_IPC_SEND), "b"(target_pid), "c"(payload));
    return result;
}

int sys_ipc_receive(os_ipc_message_t* message) {
    int result;
    asm volatile("int $0x80" : "=a"(result) : "a"(SYS_IPC_RECV), "b"(message));
    return result;
}

int sys_service_register(const char* name) {
    int result;
    asm volatile("int $0x80" : "=a"(result) : "a"(SYS_SERVICE_REGISTER), "b"(name));
    return result;
}

int sys_service_lookup(const char* name) {
    int result;
    asm volatile("int $0x80" : "=a"(result) : "a"(SYS_SERVICE_LOOKUP), "b"(name));
    return result;
}

int sys_service_status(const char* name, os_service_status_t* status) {
    int result;
    asm volatile("int $0x80" : "=a"(result) : "a"(SYS_SERVICE_STATUS), "b"(name), "c"(status));
    return result;
}

int sys_service_grant(const char* name, int target_pid) {
    int result;
    asm volatile("int $0x80" : "=a"(result) : "a"(SYS_SERVICE_GRANT), "b"(name), "c"(target_pid));
    return result;
}

int sys_service_notify(const char* name) {
    int result;
    asm volatile("int $0x80" : "=a"(result) : "a"(SYS_SERVICE_NOTIFY), "b"(name));
    return result;
}

int sys_vfs_backend_read(const char* path, char* buffer, uint32_t max) {
    int result;
    asm volatile("int $0x80" : "=a"(result) : "a"(SYS_VFS_BACKEND_READ), "b"(path), "c"(buffer), "d"(max));
    return result;
}

int sys_vfs_backend_write(const char* path, const char* data, uint32_t size) {
    int result;
    asm volatile("int $0x80" : "=a"(result) : "a"(SYS_VFS_BACKEND_WRITE), "b"(path), "c"(data), "d"(size));
    return result;
}

int sys_vfs_overlay_unlink(const char* path) {
    int result;
    asm volatile("int $0x80" : "=a"(result) : "a"(SYS_VFS_OVERLAY_UNLINK), "b"(path));
    return result;
}

int sys_vfs_overlay_rename(const char* oldpath, const char* newpath) {
    int result;
    asm volatile("int $0x80" : "=a"(result) : "a"(SYS_VFS_OVERLAY_RENAME), "b"(oldpath), "c"(newpath));
    return result;
}

static uint32_t vfs_request_counter = 0U;
static os_ipc_deferred_t ipc_deferred;

static uint32_t next_vfs_request_id(void) {
    vfs_request_counter++;
    if (vfs_request_counter == 0U) vfs_request_counter++;
    return vfs_request_counter;
}

/* Routeur général Ring 3 des réponses IPC. Une corrélation comprend le PID
 * source, le type et l’identifiant : les messages discordants sont conservés
 * dans une file statique pour leur consommateur légitime. */
#define IPC_REPLY_WAIT_TURNS 8U
#define VFS_READ_REPLY_WAIT_TURNS 24U
static int wait_ipc_reply_turns(int expected_sender, uint32_t type, uint32_t request_id,
                                os_ipc_message_t* out, uint32_t turns) {
    int rc;
    uint32_t attempt;
    if (!out || expected_sender <= 0) return OS_IPC_BAD_MESSAGE;
    rc = os_ipc_deferred_take_matching_from(&ipc_deferred, expected_sender, type,
                                            request_id, out);
    for (attempt = 0U; attempt < turns && rc == OS_IPC_EMPTY; attempt++) {
        int saved;
        yield();
        rc = sys_ipc_receive(out);
        if (rc == 0) {
            if (out->sender_pid == expected_sender && out->type == type &&
                out->request_id == request_id) return 0;
            saved = os_ipc_deferred_push(&ipc_deferred, out);
            rc = saved == 0 ? OS_IPC_EMPTY : saved;
        }
    }
    return rc;
}

static int wait_ipc_reply(int expected_sender, uint32_t type, uint32_t request_id,
                          os_ipc_message_t* out) {
    return wait_ipc_reply_turns(expected_sender, type, request_id, out, IPC_REPLY_WAIT_TURNS);
}

static int wait_vfs_read_reply(int expected_sender, uint32_t request_id, os_ipc_message_t* out) {
    return wait_ipc_reply_turns(expected_sender, OS_IPC_VFS_READ_REPLY, request_id, out,
                                VFS_READ_REPLY_WAIT_TURNS);
}

static void print_fs_err(const char* cmd, int rc);

// ==============================================================================
// FONCTIONS UTILITAIRES MODERNES
// ==============================================================================

int strlen(const char* str) {
    int len = 0;
    while (str[len] != '\0') len++;
    return len;
}

int strcmp(const char* s1, const char* s2) {
    int i = 0;
    while (s1[i] != '\0' && s2[i] != '\0') {
        if (s1[i] != s2[i]) return s1[i] - s2[i];
        i++;
    }
    return s1[i] - s2[i];
}

void strcpy(char* dest, const char* src) {
    int i = 0;
    while (src[i] != '\0') {
        dest[i] = src[i];
        i++;
    }
    dest[i] = '\0';
}

int strncmp(const char* s1, const char* s2, int n) {
    for (int i = 0; i < n; i++) {
        if (s1[i] != s2[i]) return s1[i] - s2[i];
        if (s1[i] == '\0') break;
    }
    return 0;
}

char* strstr(const char* haystack, const char* needle) {
    if (*needle == '\0') return (char*)haystack;
    
    for (int i = 0; haystack[i] != '\0'; i++) {
        int j = 0;
        while (haystack[i + j] == needle[j] && needle[j] != '\0') j++;
        if (needle[j] == '\0') return (char*)&haystack[i];
    }
    return NULL;
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

// Fonctions d'affichage modernes
void print_string(const char* str) {
    for (int i = 0; str[i] != '\0'; i++) {
        putc(str[i]);
    }
}

void backspace() {
    putc('');
    putc(' ');
    putc('');
}

void print_colored(const char* str, const char* color) {
    print_string(color);
    print_string(str);
    print_string(COLOR_RESET);
}

void print_info(const char* str) {
    print_colored("[INFO] ", COLOR_BLUE);
    print_string(str);
    print_string("\n");
}

void print_success(const char* str) {
    print_colored("[OK] ", COLOR_GREEN);
    print_string(str);
    print_string("\n");
}

void print_warning(const char* str) {
    print_colored("[WARN] ", COLOR_YELLOW);
    print_string(str);
    print_string("\n");
}

void print_error(const char* str) {
    print_colored("[ERROR] ", COLOR_RED);
    print_string(str);
    print_string("\n");
}

static int parse_int(const char* s) {
    int n = 0;
    int sign = 1;
    int i = 0;
    if (!s || s[0] == '\0') return 0;
    if (s[0] == '-') { sign = -1; i++; }
    for (; s[i] != '\0'; i++) {
        if (s[i] < '0' || s[i] > '9') break;
        n = n * 10 + (s[i] - '0');
    }
    return sign * n;
}

static void print_uint(uint32_t v) {
    char buf[16];
    int i = 0;
    do {
        buf[i++] = (char)('0' + (v % 10U));
        v /= 10U;
    } while (v != 0U);
    while (i > 0) putc(buf[--i]);
}

static void print_int(int n) {
    char buf[16];
    int i = 0;
    unsigned int v;
    if (n < 0) {
        putc('-');
        v = (unsigned int)(-n);
    } else {
        v = (unsigned int)n;
    }
    if (v == 0) {
        putc('0');
        return;
    }
    while (v > 0 && i < 15) {
        buf[i++] = (char)('0' + (v % 10));
        v /= 10;
    }
    while (i--) putc(buf[i]);
}

static char* find_char(const char* s, char c) {
    if (!s) return 0;
    while (*s) {
        if (*s == c) return (char*)s;
        s++;
    }
    return 0;
}

static void print_ramfs_err(const char* cmd, int err) {
    print_colored("[ERROR] ", COLOR_RED);
    print_string(cmd);
    print_string(": ");
    print_string(ramfs_strerror(err));
    print_string("\n");
}

static void resolve_arg(shell_context_t* ctx, const char* arg, char* out) {
    ramfs_resolve(ctx->current_dir, arg ? arg : ".", out, RAMFS_PATH_MAX);
}

// ==============================================================================
// GESTION DE L'HISTORIQUE ET DE L'ENVIRONNEMENT
// ==============================================================================

void init_shell_context(shell_context_t* ctx) {
    strcpy(ctx->current_dir, "/");
    strcpy(ctx->prompt, "MOHHDY>");
    ctx->history.count = 0;
    ctx->history.current = 0;
    ctx->env_count = 0;
    ctx->alias_count = 0;
    ctx->show_colors = 1;
    ctx->ai_mode = 1;
    ctx->ai_provider = AI_PROVIDER_LOCAL;
    strcpy(ctx->ai_model, AI_DEFAULT_MODEL);
    ctx->debug_mode = 0;
    ctx->ai_query_count = 0;
    ctx->cmd_ticks = 0;
    ctx->last_rc = 0;
    os_ipc_deferred_init(&ipc_deferred);
    
    // Initialiser quelques variables d'environnement par défaut
    strcpy(ctx->env_vars[0].name, "PATH");
    strcpy(ctx->env_vars[0].value, "/bin:/usr/bin");
    strcpy(ctx->env_vars[1].name, "HOME");
    strcpy(ctx->env_vars[1].value, "/home/user");
    strcpy(ctx->env_vars[2].name, "SHELL");
    strcpy(ctx->env_vars[2].value, "ai-shell");
    strcpy(ctx->env_vars[3].name, "MOHHDY_VERSION");
    strcpy(ctx->env_vars[3].value, "6.0");
    strcpy(ctx->env_vars[4].name, "USER");
    strcpy(ctx->env_vars[4].value, "root");
    ctx->env_count = 5;

    ramfs_init();
    procsim_init();
}

void add_to_history(shell_context_t* ctx, const char* command) {
    if (strlen(command) == 0) return;
    
    int idx = ctx->history.count % MAX_HISTORY;
    strcpy(ctx->history.commands[idx], command);
    ctx->history.count++;
    if (ctx->history.count > MAX_HISTORY) {
        ctx->history.count = MAX_HISTORY;
    }
}

char* get_env_var(shell_context_t* ctx, const char* name) {
    for (int i = 0; i < ctx->env_count; i++) {
        if (strcmp(ctx->env_vars[i].name, name) == 0) {
            return ctx->env_vars[i].value;
        }
    }
    return NULL;
}

void set_env_var(shell_context_t* ctx, const char* name, const char* value) {
    // Chercher si elle existe déjà
    for (int i = 0; i < ctx->env_count; i++) {
        if (strcmp(ctx->env_vars[i].name, name) == 0) {
            strcpy(ctx->env_vars[i].value, value);
            return;
        }
    }
    
    // Ajouter une nouvelle variable
    if (ctx->env_count < MAX_ENV_VARS) {
        strcpy(ctx->env_vars[ctx->env_count].name, name);
        strcpy(ctx->env_vars[ctx->env_count].value, value);
        ctx->env_count++;
    }
}

// ==============================================================================
// PARSEUR DE COMMANDES AVANCÉ
// ==============================================================================

int parse_command(const char* input, char* command, char args[MAX_ARGS][128], int* arg_count) {
    *arg_count = 0;
    int cmd_len = 0;
    int in_word = 0;
    int in_quotes = 0;
    int arg_idx = 0;
    int char_idx = 0;
    
    // Extraire la commande
    int i = 0;
    while (input[i] == ' ' || input[i] == '\t') i++; // Skip whitespace
    
    while (input[i] != '\0' && input[i] != ' ' && input[i] != '\t') {
        command[cmd_len++] = input[i++];
    }
    command[cmd_len] = '\0';
    
    // Extraire les arguments
    while (input[i] != '\0' && *arg_count < MAX_ARGS) {
        if (input[i] == ' ' || input[i] == '\t') {
            if (in_word && !in_quotes) {
                args[arg_idx][char_idx] = '\0';
                (*arg_count)++;
                arg_idx++;
                char_idx = 0;
                in_word = 0;
            }
        } else if (input[i] == '"') {
            in_quotes = !in_quotes;
            in_word = 1;
        } else {
            if (!in_word) {
                in_word = 1;
                char_idx = 0;
            }
            args[arg_idx][char_idx++] = input[i];
        }
        i++;
    }
    
    if (in_word) {
        args[arg_idx][char_idx] = '\0';
        (*arg_count)++;
    }
    
    return strlen(command) > 0 ? 1 : 0;
}

// ==============================================================================
// COMMANDES SHELL MODERNES
// ==============================================================================

void cmd_help(shell_context_t* ctx, char args[][128], int arg_count) {
    print_colored("\n=== MOHHDY Shell v6.0 - Aide Complète ===\n", COLOR_CYAN);
    
    print_colored("COMMANDES SYSTÈME :\n", COLOR_YELLOW);
    print_string("  ls [path]          - Lister initrd + overlay noyau\n");
    print_string("  cat <file>         - Afficher un fichier (overlay puis initrd)\n");
    print_string("  stat <path>        - Type et taille (syscall SYS_STAT)\n");
    print_string("  test f|d|e <path>  - Tester fichier/dossier (SYS_STAT)\n");
    print_string("  [ f|d|e <path> ]   - Alias de test\n");
    print_string("  cd <path>          - Changer de répertoire\n");
    print_string("  pwd                - Afficher le répertoire courant\n");
    print_string("  mkdir <dir>        - Créer un répertoire (overlay noyau)\n");
    print_string("  rmdir <dir>        - Supprimer un répertoire vide\n");
    print_string("  cp <src> <dest>    - Copier fichier ou dossier overlay\n");
    print_string("  mv <src> <dest>    - Deplacer fichier ou dossier overlay\n");
    print_string("  rm <file>          - Supprimer un fichier\n");
    
    print_colored("\nCOMMANDES PROCESSUS :\n", COLOR_YELLOW);
    print_string("  ps                 - Afficher les processus\n");
    print_string("  spawn <prog>       - Lancer un programme (cede le CPU une fois)\n");
    print_string("  yield              - Ceder le CPU (SYS_YIELD, cooperatif)\n");
    print_string("  ipc-send <pid> <txt> - Envoyer un message IPC borne\n");
    print_string("  ipc-recv           - Lire un message IPC non bloquant\n");
    print_string("  service-publish <nom> - Publier un nom de service detenue par ce shell\n");
    print_string("  service-grant <nom> <pid> - Transferer un nom possede a une tache utilisateur\n");
    print_string("  service-find <nom> - Resoudre un service nomme\n");
    print_string("  service-status <nom> - Afficher la capacite IPC d'un service\n");
    print_string("  service-watch <nom> - S'abonner aux changements de proprietaire\n");
    print_string("  vfs-backend-probe <fichier> - Verifier le backend VFS reserve\n");
    print_string("  vfs-backend-write-probe <fichier> <texte> - Verifier l'ecriture backend reservee\n");
    print_string("  vfs-backend-remove-probe <fichier> - Verifier la suppression backend reservee\n");
    print_string("  vfs-backend-rename-probe <src> <dst> - Verifier le renommage backend reserve\n");
    print_string("  vfs-grant <pid>      - Demander au serveur VFS de transferer son nom\n");
    print_string("  vfs-backend-grant <pid> - Deleguer sans transfert un acces backend VFS\n");
    print_string("  vfs-backend-grant-read <pid> - Deleguer lecture backend VFS seulement\n");
    print_string("  vfs-backend-grant-mutate <pid> - Deleguer mutation backend VFS seulement\n");
    print_string("  vfs-backend-revoke <pid> - Retirer explicitement un acces backend VFS\n");
    print_string("  vfs-backend-status <pid> - Consulter le profil backend VFS d'un PID\n");
    print_string("  vfs-backend-list      - Lister les profils backend VFS actifs\n");
    print_string("  vfs-backend-observe <generation> - Observer un inventaire backend VFS\n");
    print_string("  vfs-read <fichier>   - Lire un fichier via le service VFS nomme\n");
    print_string("  vfs-stat <fichier>   - Lire les metadonnees via le service VFS nomme\n");
    print_string("  vfs-list <repertoire/> - Lister un repertoire monte via le service VFS\n");
    print_string("  vfs-mkdir <chemin> - Creer un repertoire via un montage overlay VFS\n");
    print_string("  vfs-rmdir <chemin> - Supprimer un repertoire vide via un montage overlay VFS\n");
    print_string("  vfs-list-page <repertoire/> <depart> - Lire une page VFS mediee\n");
    print_string("  vfs-list-observe <repertoire/> <depart> <generation> - Lire une page coherente\n");
    print_string("  vfs-stats            - Afficher les compteurs volatils du serveur VFS\n");
    print_string("  vfs-mount-add <prefixe/> <initrd|overlay> - Ajouter un alias VFS\n");
    print_string("  vfs-mount-remove <prefixe/> - Retirer un alias VFS dynamique\n");
    print_string("  vfs-write <chemin> <texte> - Ecrire via le montage VFS overlay/\n");
    print_string("  vfs-remove <chemin>  - Supprimer via le montage VFS overlay/\n");
    print_string("  vfs-rename <src> <dst> - Renommer via le montage VFS overlay/\n");
    print_string("  kill <pid>         - Terminer un processus\n");
    print_string("  jobs               - Afficher les tâches\n");
    print_string("  top                - Moniteur système\n");
    print_string("  getpid             - PID du shell (syscall SYS_GETPID)\n");
    
    print_colored("\nCOMMANDES SYSTÈME :\n", COLOR_YELLOW);
    print_string("  sysinfo            - Informations système\n");
    print_string("  task-metrics <pid> - Télémétrie d’une tâche\n");
    print_string("  task-priority <pid> <1|2|3> - Politique CPU locale\n");
    print_string("  task-name <pid> <nom> - Renommer soi ou un enfant direct\n");
    print_string("  task-capacity       - Capacité globale volatile des tâches\n");
    print_string("  task-suspend <pid> - Suspendre un enfant direct prêt\n");
    print_string("  task-resume <pid>  - Reprendre un enfant direct suspendu\n");
    print_string("  kill-children      - Terminer tous les enfants directs\n");
    print_string("  children           - Instantané des enfants directs actifs\n");
    print_string("  wait-any-result    - Attendre la prochaine sortie enfant\n");
    print_string("  child-exit-count   - Total local des sorties enfants\n");
    print_string("  task-delegate <enfant> <pid> - Déléguer la supervision directe\n");
    print_string("  task-events        - Journal borné de supervision locale\n");
    print_string("  task-events-observe <gen> - Lire le journal à une génération\n");
    print_string("  task-events-clear  - Acquitter le journal de supervision\n");
    print_string("  task-event <seq>   - Consulter une transition retenue\n");
    print_string("  task-events-forget <seq> - Oublier une transition retenue\n");
    print_string("  task-summary       - Instantané consolidé de supervision\n");
    print_string("  task-events-notify <on|off> - Notifications IPC locales de supervision\n");
    print_string("  task-events-filter <all|exit|suspend|resume|delegate-out|delegate-in|none> - Filtrer les notifications\n");
    print_string("  task-events-notify-status - Etat local de souscription et filtre\n");
    print_string("  task-events-watch <pid> - Cibler un enfant direct pour les notifications\n");
    print_string("  task-events-unwatch <pid> - Retirer un enfant de la watchlist\n");
    print_string("  task-events-watch-clear - Désactiver et vider la watchlist\n");
    print_string("  task-events-watch-status - Etat local de la watchlist\n");
    print_string("  task-events-notify-stats - Compteurs locaux de livraison detaillee\n");
    print_string("  task-events-notify-stats-clear - Acquitter les compteurs de livraison\n");
    print_string("  task-event-replay <seq> - Rediffuser une transition retenue\n");
    print_string("  task-priority-child <pid|off> - Choisir l’enfant supervise prioritaire\n");
    print_string("  task-priority-child-status - Lire la priorite de supervision\n");
    print_string("  task-events-budget <n|off> - Limiter les notifications detaillees\n");
    print_string("  task-events-budget-status - Lire le budget de notifications\n");
    print_string("  child-result <pid> - Dernier résultat local d’un enfant terminé\n");
    print_string("  child-results      - Historique borné de résultats enfants\n");
    print_string("  child-results-clear - Acquitter l’historique enfant local\n");
    print_string("  child-results-observe <gen> - Observer l’historique à une génération\n");
    print_string("  child-result-any <pid> - Chercher un résultat enfant retenu\n");
    print_string("  child-results-forget <pid> - Acquitter un résultat enfant\n");
    print_string("  wait <pid>         - Attendre la sortie d’un enfant direct\n");
    print_string("  wait-result <pid>  - Attendre puis afficher le résultat enfant\n");
    print_string("  mem                - Utilisation mémoire\n");
    print_string("  uptime             - Temps de fonctionnement\n");
    print_string("  date               - Date et heure\n");
    print_string("  whoami             - Utilisateur courant\n");
    
    print_colored("\nCOMMANDES SHELL :\n", COLOR_YELLOW);
    print_string("  history            - Historique des commandes\n");
    print_string("  alias <name>=<cmd> - Créer un alias\n");
    print_string("  unalias <name>     - Supprimer un alias\n");
    print_string("  env                - Variables d'environnement\n");
    print_string("  export <var>=<val> - Définir une variable\n");
    print_string("  which <cmd>        - Trouver l'emplacement d'une commande\n");
    print_string("  rc                 - Dernier code retour ($?)\n");
    
    print_colored("\nCOMMANDES INTELLIGENCE ARTIFICIELLE :\n", COLOR_YELLOW);
    print_string("  ai <question>      - Poser une question à l'IA\n");
    print_string("  ai-mode [on|off]   - Activer/désactiver le mode IA\n");
    print_string("  ai-help            - Aide sur l'utilisation de l'IA\n");
    print_string("  ai-stats           - Statistiques de l'IA\n");
    print_string("  ai-provider [nom]  - Choisir local ou openai\n");
    print_string("  ai-model [action]  - Lister ou choisir le modele local\n");
    print_string("  ai-runtime         - Etat du moteur IA et des prerequis\n");
    print_string("  ai-continue        - Poursuivre un token de la session GGUF locale\n");
    print_string("  ai-acquire <hote> [port] - Demarrer DHCP, DNS et TCP LLM sans secret\n");
    print_string("  ai-tls-poll         - Piloter SYN-ACK/TLS avec les materiaux noyau\n");
    print_string("  ai-request <f> <m> <p> <q> - Emettre POST LLM apres TLS authentifie\n");
    print_string("  ai-stream-request <f> <m> <p> <q> - Emettre POST LLM SSE chiffre\n");
    print_string("  ai-text-poll       - Lire le texte LLM extrait par le noyau\n");
    print_string("  ai-sse-poll        - Lire un delta SSE LLM extrait par le noyau\n");
    print_string("  ai-next            - Rearmer une session LLM apres sa reponse\n");
    print_string("  ai-close           - Annuler la session LLM et purger ses secrets\n");
    print_string("  net-status         - Etat reel de la pile reseau bare-metal\n");
    
    print_colored("\nCOMMANDES UTILITAIRES :\n", COLOR_YELLOW);
    print_string("  clear              - Effacer l'écran\n");
    print_string("  echo <text>        - Afficher du texte\n");
    print_string("  write <file> <txt> - Ecrire un fichier overlay (sans >)\n");
    print_string("  append <file> <txt> - Ajouter du texte (SYS_APPEND)\n");
    print_string("  touch <file>       - Creer un fichier overlay vide\n");
    print_string("  fat16-list         - Lister la racine du volume FAT16\n");
    print_string("  fat16-cat <8.3>    - Lire un fichier du volume FAT16\n");
    print_string("  grep <pattern>     - Rechercher dans un texte\n");
    print_string("  wc <file>          - Compter lignes/mots/caractères\n");
    print_string("  sort <file>        - Trier les lignes (sort ok N fichier)\n");
    print_string("  head <file>        - Debut du fichier (head ok N fichier)\n");
    print_string("  tail <file>        - Fin du fichier (tail ok N fichier)\n");
    
    print_colored("\nAFFICHAGE :\n", COLOR_YELLOW);
    print_string("  Page Up / Haut     - Remonter dans l'historique d'ecran\n");
    print_string("  Page Down / Bas    - Redescendre vers la saisie\n");
    print_string("  Curseur bloc       - Marque la position de saisie\n");
    
    print_colored("\nCONTRÔLE :\n", COLOR_YELLOW);
    print_string("  exit [code]        - Quitter le shell\n");
    print_string("  logout             - Se déconnecter\n");
    print_string("  reboot             - Redémarrer le système\n");
    print_string("  shutdown           - Arrêter le système\n");
    
    print_colored("\nTIP: ls/cat/mkdir/rm/cp/mv/write/append parlent au noyau (initrd + overlay RAM).\n", COLOR_GREEN);
    print_colored("    Si le mode IA est activé, posez des questions sans 'ai'.\n\n", COLOR_GREEN);
}

void cmd_ls(shell_context_t* ctx, char args[][128], int arg_count) {
    char path[RAMFS_PATH_MAX];
    os_dirent_t kents[32];
    ramfs_dirent_t rents[RAMFS_MAX_LIST];
    int kn, rn;
    int shown = 0;

    if (arg_count > 0) resolve_arg(ctx, args[0], path);
    else resolve_arg(ctx, ".", path);

    print_colored("\n=== Initrd / VFS ===\n", COLOR_CYAN);
    print_string("chemin: ");
    print_string(path);
    print_string("\n");

    kn = sys_listdir(path, kents, 32);
    if (kn > 0) {
        for (int i = 0; i < kn; i++) {
            if (kents[i].flags == OS_DIRENT_DIR) {
                print_colored("drwxr-xr-x  ", COLOR_BLUE);
                print_colored(kents[i].name, COLOR_BLUE);
                print_string("/\n");
            } else {
                print_string("-rw-r--r--  ");
                print_int((int)kents[i].size);
                print_string("  ");
                print_string(kents[i].name);
                print_string("\n");
            }
            shown++;
        }
    }

    rn = ramfs_list(path, rents, RAMFS_MAX_LIST);
    if (rn > 0) {
        for (int i = 0; i < rn; i++) {
            int dup = 0;
            if (kn > 0) {
                for (int k = 0; k < kn; k++) {
                    if (strcmp(rents[i].name, kents[k].name) == 0) {
                        dup = 1;
                        break;
                    }
                }
            }
            if (dup) continue;
            if (rents[i].is_dir) {
                print_colored("drwxr-xr-x  ", COLOR_BLUE);
                print_colored(rents[i].name, COLOR_BLUE);
                print_string("/\n");
            } else {
                print_string("-rw-r--r--  ");
                print_int(rents[i].size);
                print_string("  ");
                print_string(rents[i].name);
                print_string("\n");
            }
            shown++;
        }
    }

    if (shown == 0 && kn < 0 && rn < 0) {
        print_error("ls: repertoire introuvable");
        return;
    }
    print_string("Total: ");
    print_int(shown);
    print_string(" elements\n\n");
}

static const char* proc_state_str(int st) {
    if (st == OS_TASK_RUNNING) return "R";
    if (st == OS_TASK_READY) return "S";
    if (st == OS_TASK_SUSPENDED) return "P";
    if (st == OS_TASK_TERMINATED) return "Z";
    return "W";
}

void cmd_ps(shell_context_t* ctx, char args[][128], int arg_count) {
    os_proc_t procs[16];
    int n;
    (void)ctx; (void)args; (void)arg_count;
    n = sys_ps(procs, 16);
    print_colored("\n=== Processus (noyau) ===\n", COLOR_CYAN);
    print_colored("  PID  PPID  STAT  TYPE  COMMAND\n", COLOR_YELLOW);
    if (n < 0) n = 0;
    for (int i = 0; i < n; i++) {
        print_string("  ");
        print_int(procs[i].pid);
        print_string("    ");
        print_int(procs[i].parent_pid);
        print_string("    ");
        print_string(proc_state_str(procs[i].state));
        print_string("     ");
        print_string(procs[i].type == OS_TASK_USER ? "user  " : "kern  ");
        print_string(procs[i].name);
        print_string("\n");
    }
    print_string("Total: ");
    print_int(n);
    print_string("\n\n");
}

void cmd_task_metrics(shell_context_t* ctx, char args[][128], int arg_count) {
    os_task_metrics_t metrics;
    os_meminfo_t mem;
    int pid;
    int rc;
    (void)ctx;
    if (arg_count != 1 || (pid = parse_int(args[0])) < 0) {
        print_error("Usage: task-metrics <pid>");
        return;
    }
    rc = sys_task_metrics(pid, &metrics);
    if (rc != 0) {
        print_error("task-metrics: PID absent ou indisponible");
        return;
    }
    print_colored("\n=== Télémétrie tâche ===\n", COLOR_CYAN);
    print_string("PID : "); print_int(metrics.pid);
    print_string("\nParent : "); print_int(metrics.parent_pid);
    print_string("\nÉtat : "); print_string(proc_state_str(metrics.state));
    print_string("\nType : "); print_string(metrics.type == OS_TASK_USER ? "user" : "kernel");
    print_string("\nPriorité CPU : "); print_int((int)metrics.priority);
    print_string("\nÂge : "); print_int((int)metrics.age_ticks); print_string(" ticks");
    print_string("\nExécution cumulée : "); print_int((int)metrics.run_ticks); print_string(" ticks");
    print_string("\nCommutations : "); print_int((int)metrics.switch_count);
    print_string("\nEnfants directs : "); print_int((int)metrics.direct_children); print_string("\n");
    if (sys_meminfo(&mem) == 0) {
        print_string("PMM pages total/utilisées/libres : "); print_int((int)mem.total_pages);
        print_string("/"); print_int((int)mem.used_pages); print_string("/"); print_int((int)mem.free_pages); print_string("\n");
    }
    print_string("task-metrics ok "); print_int(metrics.pid); print_string(" "); print_int((int)metrics.priority); print_string(" "); print_int((int)metrics.run_ticks); print_string(" "); print_int((int)metrics.switch_count); print_string("\n");
}

void cmd_task_priority(shell_context_t* ctx, char args[][128], int arg_count) {
    int pid;
    int priority;
    int rc;
    (void)ctx;
    if (arg_count != 2 || (pid = parse_int(args[0])) < 0 ||
        (priority = parse_int(args[1])) < (int)OS_TASK_PRIORITY_LOW ||
        priority > (int)OS_TASK_PRIORITY_HIGH) {
        print_error("Usage: task-priority <pid> <1|2|3>");
        return;
    }
    rc = sys_task_set_priority(pid, (unsigned int)priority);
    if (rc == OS_TASK_NOT_FOUND) {
        print_error("task-priority: PID absent");
        return;
    }
    if (rc == OS_TASK_BAD_PRIORITY) {
        print_error("task-priority: priorité hors plage");
        return;
    }
    if (rc == OS_TASK_CONTROL_DENIED) {
        print_error("task-priority: autorité limitée à soi ou enfant direct");
        return;
    }
    if (rc != 0) {
        print_error("task-priority: syscall indisponible");
        return;
    }
    print_string("task-priority ok "); print_int(pid); print_string(" "); print_int(priority); print_string("\n");
}

void cmd_task_suspend(shell_context_t* ctx, char args[][128], int arg_count) {
    int pid;
    int rc;
    (void)ctx;
    if (arg_count != 1 || (pid = parse_int(args[0])) < 0) {
        print_error("Usage: task-suspend <pid>");
        return;
    }
    rc = sys_task_suspend(pid);
    if (rc == OS_TASK_NOT_FOUND) {
        print_error("task-suspend: PID absent");
        return;
    }
    if (rc == OS_TASK_CONTROL_DENIED) {
        print_error("task-suspend: autorité limitée à un enfant direct");
        return;
    }
    if (rc == OS_TASK_BAD_STATE) {
        print_error("task-suspend: enfant non prêt");
        return;
    }
    if (rc != 0) {
        print_error("task-suspend: syscall indisponible");
        return;
    }
    print_string("task-suspend ok "); print_int(pid); print_string("\n");
}

void cmd_task_resume(shell_context_t* ctx, char args[][128], int arg_count) {
    int pid;
    int rc;
    (void)ctx;
    if (arg_count != 1 || (pid = parse_int(args[0])) < 0) {
        print_error("Usage: task-resume <pid>");
        return;
    }
    rc = sys_task_resume(pid);
    if (rc == OS_TASK_NOT_FOUND) {
        print_error("task-resume: PID absent");
        return;
    }
    if (rc == OS_TASK_CONTROL_DENIED) {
        print_error("task-resume: autorité limitée à un enfant direct");
        return;
    }
    if (rc == OS_TASK_BAD_STATE) {
        print_error("task-resume: enfant non suspendu");
        return;
    }
    if (rc != 0) {
        print_error("task-resume: syscall indisponible");
        return;
    }
    print_string("task-resume ok "); print_int(pid); print_string("\n");
}

void cmd_kill_children(shell_context_t* ctx, char args[][128], int arg_count) {
    int count;
    (void)ctx;
    if (arg_count != 0) {
        print_error("Usage: kill-children");
        return;
    }
    count = sys_task_kill_children();
    if (count < 0) {
        print_error("kill-children: syscall indisponible");
        return;
    }
    print_string("kill-children ok "); print_int(count); print_string("\n");
}

void cmd_children(shell_context_t* ctx, char args[][128], int arg_count) {
    os_task_children_t children;
    int rc;
    uint32_t i;
    (void)ctx;
    if (arg_count != 0) {
        print_error("Usage: children");
        return;
    }
    rc = sys_task_children(&children);
    if (rc != 0) {
        print_error("children: syscall indisponible");
        return;
    }
    for (i = 0U; i < children.count; i++) {
        print_string("child-entry "); print_int(children.entries[i].pid); print_string(" ");
        print_string(proc_state_str(children.entries[i].state)); print_string(" ");
        print_string(children.entries[i].name); print_string("\n");
    }
    print_string("children ok "); print_int((int)children.count); print_string("\n");
}

void cmd_wait_any_result(shell_context_t* ctx, char args[][128], int arg_count) {
    os_task_exit_history_t history;
    os_task_exit_result_t result;
    int rc;
    (void)ctx;
    if (arg_count != 0) {
        print_error("Usage: wait-any-result");
        return;
    }
    rc = sys_task_wait_any();
    if (rc == OS_TASK_NO_DIRECT_CHILD) {
        print_error("wait-any-result: aucun enfant direct");
        return;
    }
    if (rc != 0) {
        print_error("wait-any-result: syscall d’attente indisponible");
        return;
    }
    rc = sys_task_child_result_list(&history);
    if (rc != 0 || history.count == 0U) {
        print_error("wait-any-result: résultat enfant indisponible");
        return;
    }
    result = history.entries[history.count - 1U];
    print_string("wait-any-result ok "); print_int(result.child_pid); print_string(" ");
    print_int(result.exit_code); print_string(" "); print_int((int)result.reason); print_string("\n");
}

void cmd_child_exit_count(shell_context_t* ctx, char args[][128], int arg_count) {
    os_task_child_exit_count_t snapshot;
    int rc;
    (void)ctx;
    if (arg_count != 0) {
        print_error("Usage: child-exit-count");
        return;
    }
    rc = sys_task_child_exit_count(&snapshot);
    if (rc != 0) {
        print_error("child-exit-count: syscall indisponible");
        return;
    }
    print_string("child-exit-count ok "); print_uint(snapshot.count); print_string("\n");
}

void cmd_task_delegate(shell_context_t* ctx, char args[][128], int arg_count) {
    int child_pid;
    int supervisor_pid;
    int rc;
    (void)ctx;
    if (arg_count != 2 || (child_pid = parse_int(args[0])) < 0 ||
        (supervisor_pid = parse_int(args[1])) < 0) {
        print_error("Usage: task-delegate <pid_enfant> <pid_superviseur>");
        return;
    }
    rc = sys_task_delegate_child(child_pid, supervisor_pid);
    if (rc == OS_TASK_NOT_CHILD) {
        print_error("task-delegate: cible non enfant direct");
        return;
    }
    if (rc == OS_TASK_BAD_DELEGATE) {
        print_error("task-delegate: superviseur invalide ou filiation cyclique");
        return;
    }
    if (rc == OS_TASK_BAD_STATE) {
        print_error("task-delegate: enfant déjà attendu");
        return;
    }
    if (rc == OS_TASK_CHILD_LIMIT) {
        print_error("task-delegate: capacité du superviseur atteinte");
        return;
    }
    if (rc != 0) {
        print_error("task-delegate: syscall indisponible");
        return;
    }
    print_string("task-delegate ok "); print_int(child_pid); print_string(" ");
    print_int(supervisor_pid); print_string("\n");
}

static const char* supervision_action_name(uint32_t action) {
    if (action == OS_TASK_SUPERVISION_EXIT) return "exit";
    if (action == OS_TASK_SUPERVISION_SUSPEND) return "suspend";
    if (action == OS_TASK_SUPERVISION_RESUME) return "resume";
    if (action == OS_TASK_SUPERVISION_DELEGATE_OUT) return "delegate-out";
    if (action == OS_TASK_SUPERVISION_DELEGATE_IN) return "delegate-in";
    return "unknown";
}

void cmd_task_events(shell_context_t* ctx, char args[][128], int arg_count) {
    os_task_supervision_events_t events;
    uint32_t i;
    int rc;
    (void)ctx;
    if (arg_count != 0) {
        print_error("Usage: task-events");
        return;
    }
    rc = sys_task_supervision_events(&events);
    if (rc != 0) {
        print_error("task-events: syscall indisponible");
        return;
    }
    print_string("task-events ok "); print_uint(events.generation); print_string(" ");
    print_uint(events.count); print_string("\n");
    for (i = 0U; i < events.count; i++) {
        os_task_supervision_event_t* event = &events.entries[i];
        print_string("task-event "); print_uint(event->sequence); print_string(" ");
        print_string(supervision_action_name(event->action)); print_string(" ");
        print_int(event->child_pid); print_string(" "); print_int(event->related_pid);
        print_string(" "); print_uint(event->detail); print_string(" ");
        print_uint(event->ticks); print_string("\n");
    }
}

void cmd_task_events_clear(shell_context_t* ctx, char args[][128], int arg_count) {
    int generation;
    (void)ctx;
    if (arg_count != 0) {
        print_error("Usage: task-events-clear");
        return;
    }
    generation = sys_task_supervision_events_ack();
    if (generation < 0) {
        print_error("task-events-clear: syscall indisponible");
        return;
    }
    print_string("task-events-clear ok "); print_uint((uint32_t)generation); print_string("\n");
}

void cmd_task_events_observe(shell_context_t* ctx, char args[][128], int arg_count) {
    int expected;
    int rc;
    uint32_t i;
    os_task_supervision_events_observation_t observation;
    (void)ctx;
    if (arg_count != 1 || (expected = parse_int(args[0])) < 0) {
        print_error("Usage: task-events-observe <generation>");
        return;
    }
    rc = sys_task_supervision_events_observe((uint32_t)expected, &observation);
    if (rc == OS_TASK_HISTORY_STALE) {
        print_string("task-events-observe stale "); print_uint(observation.generation);
        print_string("\n");
        return;
    }
    if (rc != 0) {
        print_error("task-events-observe: syscall indisponible");
        return;
    }
    print_string("task-events-observe ok "); print_uint(observation.generation);
    print_string(" "); print_uint(observation.events.count); print_string("\n");
    for (i = 0U; i < observation.events.count; i++) {
        os_task_supervision_event_t* event = &observation.events.entries[i];
        print_string("task-event "); print_uint(event->sequence); print_string(" ");
        print_string(supervision_action_name(event->action)); print_string(" ");
        print_int(event->child_pid); print_string(" "); print_int(event->related_pid);
        print_string(" "); print_uint(event->detail); print_string(" ");
        print_uint(event->ticks); print_string("\n");
    }
}

void cmd_task_event(shell_context_t* ctx, char args[][128], int arg_count) {
    int sequence;
    int rc;
    os_task_supervision_event_t event;
    (void)ctx;
    if (arg_count != 1 || (sequence = parse_int(args[0])) <= 0) {
        print_error("Usage: task-event <sequence>");
        return;
    }
    rc = sys_task_supervision_event_find((uint32_t)sequence, &event);
    if (rc == OS_TASK_NO_SUPERVISION_EVENT) {
        print_error("task-event: transition absente");
        return;
    }
    if (rc != 0) {
        print_error("task-event: syscall indisponible");
        return;
    }
    print_string("task-event ok "); print_uint(event.sequence); print_string(" ");
    print_string(supervision_action_name(event.action)); print_string(" ");
    print_int(event.child_pid); print_string(" "); print_int(event.related_pid);
    print_string(" "); print_uint(event.detail); print_string(" ");
    print_uint(event.ticks); print_string("\n");
}

void cmd_task_events_forget(shell_context_t* ctx, char args[][128], int arg_count) {
    int sequence;
    int rc;
    (void)ctx;
    if (arg_count != 1 || (sequence = parse_int(args[0])) <= 0) {
        print_error("Usage: task-events-forget <sequence>");
        return;
    }
    rc = sys_task_supervision_event_forget((uint32_t)sequence);
    if (rc == OS_TASK_NO_SUPERVISION_EVENT) {
        print_error("task-events-forget: transition absente");
        return;
    }
    if (rc < 0) {
        print_error("task-events-forget: syscall indisponible");
        return;
    }
    print_string("task-events-forget ok "); print_uint((uint32_t)sequence);
    print_string(" "); print_uint((uint32_t)rc); print_string("\n");
}

void cmd_task_events_notify(shell_context_t* ctx, char args[][128], int arg_count) {
    uint32_t enabled;
    int rc;
    (void)ctx;
    if (arg_count != 1 ||
        (strcmp(args[0], "on") != 0 && strcmp(args[0], "off") != 0)) {
        print_error("Usage: task-events-notify <on|off>");
        return;
    }
    enabled = strcmp(args[0], "on") == 0 ? 1U : 0U;
    rc = sys_task_supervision_notify(enabled);
    if (rc == OS_TASK_BAD_NOTIFY) {
        print_error("task-events-notify: valeur invalide");
        return;
    }
    if (rc < 0) {
        print_error("task-events-notify: syscall indisponible");
        return;
    }
    print_string("task-events-notify ok ");
    print_string(rc == 0 ? "off" : "on");
    print_string("\n");
}

void cmd_task_events_filter(shell_context_t* ctx, char args[][128], int arg_count) {
    uint32_t mask;
    int rc;
    (void)ctx;
    if (arg_count != 1) {
        print_error("Usage: task-events-filter <all|exit|suspend|resume|delegate-out|delegate-in|none>");
        return;
    }
    if (strcmp(args[0], "all") == 0) mask = OS_TASK_SUPERVISION_NOTIFY_ALL;
    else if (strcmp(args[0], "exit") == 0) mask = OS_TASK_SUPERVISION_NOTIFY_EXIT;
    else if (strcmp(args[0], "suspend") == 0) mask = OS_TASK_SUPERVISION_NOTIFY_SUSPEND;
    else if (strcmp(args[0], "resume") == 0) mask = OS_TASK_SUPERVISION_NOTIFY_RESUME;
    else if (strcmp(args[0], "delegate-out") == 0) mask = OS_TASK_SUPERVISION_NOTIFY_DELEGATE_OUT;
    else if (strcmp(args[0], "delegate-in") == 0) mask = OS_TASK_SUPERVISION_NOTIFY_DELEGATE_IN;
    else if (strcmp(args[0], "none") == 0) mask = 0U;
    else {
        print_error("task-events-filter: filtre invalide");
        return;
    }
    rc = sys_task_supervision_notify_filter(mask);
    if (rc == OS_TASK_BAD_NOTIFY_FILTER) {
        print_error("task-events-filter: masque invalide");
        return;
    }
    if (rc != 0) {
        print_error("task-events-filter: syscall indisponible");
        return;
    }
    print_string("task-events-filter ok "); print_string(args[0]); print_string(" ");
    print_uint(mask); print_string("\n");
}

void cmd_task_events_notify_status(shell_context_t* ctx, char args[][128], int arg_count) {
    os_task_supervision_notify_status_t status;
    int rc;
    (void)ctx;
    if (arg_count != 0) {
        print_error("Usage: task-events-notify-status");
        return;
    }
    rc = sys_task_supervision_notify_status(&status);
    if (rc != 0) {
        print_error("task-events-notify-status: syscall indisponible");
        return;
    }
    print_string("task-events-notify-status ok "); print_uint(status.enabled); print_string(" ");
    print_uint(status.mask); print_string("\n");
}

void cmd_task_events_watch_update(shell_context_t* ctx, char args[][128], int arg_count,
                                  uint32_t enabled) {
    int child_pid;
    int rc;
    (void)ctx;
    if (arg_count != 1 || (child_pid = parse_int(args[0])) <= 0) {
        print_error(enabled != 0U ? "Usage: task-events-watch <pid>" :
                                  "Usage: task-events-unwatch <pid>");
        return;
    }
    rc = sys_task_supervision_watch(child_pid, enabled);
    if (rc == OS_TASK_NOT_CHILD) {
        print_error("task-events-watch: enfant direct requis");
        return;
    }
    if (rc == OS_TASK_WATCH_FULL) {
        print_error("task-events-watch: capacite atteinte");
        return;
    }
    if (rc == OS_TASK_NO_SUPERVISION_WATCH) {
        print_error("task-events-unwatch: enfant absent");
        return;
    }
    if (rc < 0) {
        print_error("task-events-watch: syscall indisponible");
        return;
    }
    print_string(enabled != 0U ? "task-events-watch ok " : "task-events-unwatch ok ");
    print_uint((uint32_t)child_pid); print_string(" "); print_uint((uint32_t)rc); print_string("\n");
}

void cmd_task_events_watch_clear(shell_context_t* ctx, char args[][128], int arg_count) {
    int rc;
    (void)ctx;
    if (arg_count != 0) {
        print_error("Usage: task-events-watch-clear");
        return;
    }
    rc = sys_task_supervision_watch(0, 0U);
    if (rc != 0) {
        print_error("task-events-watch-clear: syscall indisponible");
        return;
    }
    print_string("task-events-watch-clear ok\n");
}

void cmd_task_events_watch_status(shell_context_t* ctx, char args[][128], int arg_count) {
    os_task_supervision_watch_status_t status;
    uint32_t i;
    int rc;
    (void)ctx;
    if (arg_count != 0) {
        print_error("Usage: task-events-watch-status");
        return;
    }
    rc = sys_task_supervision_watch_status(&status);
    if (rc != 0) {
        print_error("task-events-watch-status: syscall indisponible");
        return;
    }
    print_string("task-events-watch-status ok "); print_uint(status.enabled); print_string(" ");
    print_uint(status.count);
    for (i = 0U; i < status.count; i++) {
        print_string(" "); print_uint((uint32_t)status.pids[i]);
    }
    print_string("\n");
}

void cmd_task_events_notify_stats(shell_context_t* ctx, char args[][128], int arg_count) {
    os_task_supervision_delivery_stats_t stats;
    int rc;
    (void)ctx;
    if (arg_count != 0) {
        print_error("Usage: task-events-notify-stats");
        return;
    }
    rc = sys_task_supervision_delivery_stats(&stats);
    if (rc != 0) {
        print_error("task-events-notify-stats: syscall indisponible");
        return;
    }
    print_string("task-events-notify-stats ok "); print_uint(stats.attempted); print_string(" ");
    print_uint(stats.delivered); print_string(" "); print_uint(stats.dropped); print_string("\n");
}

void cmd_task_events_notify_stats_clear(shell_context_t* ctx, char args[][128], int arg_count) {
    int rc;
    (void)ctx;
    if (arg_count != 0) {
        print_error("Usage: task-events-notify-stats-clear");
        return;
    }
    rc = sys_task_supervision_delivery_stats_ack();
    if (rc != 0) {
        print_error("task-events-notify-stats-clear: syscall indisponible");
        return;
    }
    print_string("task-events-notify-stats-clear ok\n");
}

void cmd_task_event_replay(shell_context_t* ctx, char args[][128], int arg_count) {
    int sequence;
    int rc;
    (void)ctx;
    if (arg_count != 1 || (sequence = parse_int(args[0])) <= 0) {
        print_error("Usage: task-event-replay <sequence>");
        return;
    }
    rc = sys_task_supervision_event_replay((uint32_t)sequence);
    if (rc == OS_TASK_NO_SUPERVISION_EVENT) {
        print_error("task-event-replay: transition absente");
        return;
    }
    if (rc == OS_IPC_FULL) {
        print_error("task-event-replay: boite IPC pleine");
        return;
    }
    if (rc < 0) {
        print_error("task-event-replay: syscall indisponible");
        return;
    }
    print_string("task-event-replay ok "); print_uint((uint32_t)sequence); print_string("\n");
}

void cmd_task_priority_child(shell_context_t* ctx, char args[][128], int arg_count) {
    int child_pid;
    int rc;
    (void)ctx;
    if (arg_count != 1) {
        print_error("Usage: task-priority-child <pid|off>");
        return;
    }
    child_pid = strcmp(args[0], "off") == 0 ? 0 : parse_int(args[0]);
    if (child_pid < 0) {
        print_error("task-priority-child: PID invalide");
        return;
    }
    rc = sys_task_supervision_priority(child_pid);
    if (rc == OS_TASK_NOT_CHILD) {
        print_error("task-priority-child: enfant direct requis");
        return;
    }
    if (rc < 0) {
        print_error("task-priority-child: syscall indisponible");
        return;
    }
    print_string("task-priority-child ok ");
    print_string(child_pid == 0 ? "off" : args[0]); print_string("\n");
}

void cmd_task_priority_child_status(shell_context_t* ctx, char args[][128], int arg_count) {
    os_task_supervision_priority_status_t status;
    int rc;
    (void)ctx;
    if (arg_count != 0) {
        print_error("Usage: task-priority-child-status");
        return;
    }
    rc = sys_task_supervision_priority_status(&status);
    if (rc != 0) {
        print_error("task-priority-child-status: syscall indisponible");
        return;
    }
    print_string("task-priority-child-status ok ");
    if (status.child_pid < 0) print_string("off");
    else print_uint((uint32_t)status.child_pid);
    print_string("\n");
}

void cmd_task_events_budget(shell_context_t* ctx, char args[][128], int arg_count) {
    int value;
    int rc;
    (void)ctx;
    if (arg_count != 1) { print_error("Usage: task-events-budget <n|off>"); return; }
    value = strcmp(args[0], "off") == 0 ? 0 : parse_int(args[0]);
    if (value < 0) { print_error("task-events-budget: valeur invalide"); return; }
    rc = sys_task_supervision_notify_budget((uint32_t)value);
    if (rc != 0) { print_error("task-events-budget: syscall indisponible"); return; }
    print_string("task-events-budget ok ");
    if (value == 0) print_string("off"); else print_uint((uint32_t)value);
    print_string("\n");
}

/* Les commandes FAT héritées passent par vfs : les syscalls FAT bruts sont
 * désormais backend-protégés et ne doivent pas devenir une voie de contournement. */
static void cmd_vfs_read(shell_context_t* ctx, char args[][128], int arg_count);
static void cmd_vfs_list(shell_context_t* ctx, char args[][128], int arg_count);

/* Les commandes FAT restent utilisables dans une image qui lance seulement le
 * shell. Elles démarrent un couple VFS uniquement pour la requête de lecture,
 * attendent une publication bornée, puis libèrent leurs enfants transitoires. */
static int start_transient_vfs(int* worker_out, int* server_out) {
    int worker_pid;
    int server_pid;
    uint32_t turns;
    if (sys_service_lookup("vfs") > 0) return 0;
    worker_pid = spawn("vfsvirtual", 0);
    if (worker_pid < 0) return -1;
    server_pid = spawn("vfsserver", 0);
    if (server_pid < 0) { (void)sys_kill_pid(worker_pid); return -2; }
    for (turns = 0U; turns < 24U; turns++) {
        if (sys_service_lookup("vfs") > 0) {
            if (worker_out) *worker_out = worker_pid;
            if (server_out) *server_out = server_pid;
            return 1;
        }
        yield();
    }
    (void)sys_kill_pid(server_pid);
    (void)sys_kill_pid(worker_pid);
    return -3;
}

static void stop_transient_vfs(int started, int worker_pid, int server_pid) {
    if (!started) return;
    (void)sys_kill_pid(server_pid);
    (void)sys_kill_pid(worker_pid);
    yield();
    yield();
}

static void cmd_fat16_list(shell_context_t* ctx, char args[][128], int arg_count) {
    char vfs_args[1][128] = { "fat16/" };
    int worker_pid = 0;
    int server_pid = 0;
    int started;
    if (arg_count != 0) { print_error("Usage: fat16-list"); return; }
    started = start_transient_vfs(&worker_pid, &server_pid);
    if (started < 0) { print_error("fat16-list: mediatrice VFS indisponible"); return; }
    cmd_vfs_list(ctx, vfs_args, 1);
    stop_transient_vfs(started, worker_pid, server_pid);
}

static void cmd_fat16_cat(shell_context_t* ctx, char args[][128], int arg_count) {
    char vfs_args[1][128];
    int worker_pid = 0;
    int server_pid = 0;
    int started;
    uint32_t i = 0U;
    if (arg_count != 1) { print_error("Usage: fat16-cat <8.3>"); return; }
    vfs_args[0][i++] = 'f'; vfs_args[0][i++] = 'a'; vfs_args[0][i++] = 't';
    vfs_args[0][i++] = '1'; vfs_args[0][i++] = '6'; vfs_args[0][i++] = '/';
    while (args[0][i - 6U] != '\0' && i + 1U < OS_VFS_PATH_MAX) {
        vfs_args[0][i] = args[0][i - 6U];
        i++;
    }
    if (args[0][i - 6U] != '\0') { print_error("fat16-cat: chemin trop long"); return; }
    vfs_args[0][i] = '\0';
    started = start_transient_vfs(&worker_pid, &server_pid);
    if (started < 0) { print_error("fat16-cat: mediatrice VFS indisponible"); return; }
    cmd_vfs_read(ctx, vfs_args, 1);
    stop_transient_vfs(started, worker_pid, server_pid);
}

void cmd_task_events_budget_status(shell_context_t* ctx, char args[][128], int arg_count) {
    os_task_supervision_notify_budget_status_t status;
    int rc;
    (void)ctx;
    if (arg_count != 0) { print_error("Usage: task-events-budget-status"); return; }
    rc = sys_task_supervision_notify_budget_status(&status);
    if (rc != 0) { print_error("task-events-budget-status: syscall indisponible"); return; }
    print_string("task-events-budget-status ok "); print_uint(status.limit);
    print_string(" "); print_uint(status.used); print_string("\n");
}

void cmd_task_summary(shell_context_t* ctx, char args[][128], int arg_count) {
    int rc;
    os_task_supervision_summary_t summary;
    (void)ctx;
    if (arg_count != 0) {
        print_error("Usage: task-summary");
        return;
    }
    rc = sys_task_supervision_summary(&summary);
    if (rc != 0) {
        print_error("task-summary: syscall indisponible");
        return;
    }
    print_string("task-summary ok "); print_uint(summary.generation); print_string(" ");
    print_uint(summary.active_children); print_string(" ");
    print_uint(summary.suspended_children); print_string(" ");
    print_uint(summary.child_exit_count); print_string(" ");
    print_uint(summary.retained_events); print_string("\n");
}

void cmd_task_wait(shell_context_t* ctx, char args[][128], int arg_count) {
    int pid;
    int rc;
    (void)ctx;
    if (arg_count != 1 || (pid = parse_int(args[0])) < 0) {
        print_error("Usage: wait <pid>");
        return;
    }
    rc = sys_task_wait(pid);
    if (rc == OS_TASK_NOT_FOUND) {
        print_error("wait: PID absent");
        return;
    }
    if (rc == OS_TASK_NOT_CHILD) {
        print_error("wait: cible non enfant direct");
        return;
    }
    if (rc != 0) {
        print_error("wait: syscall indisponible");
        return;
    }
    print_string("wait ok "); print_int(pid); print_string("\n");
}

void cmd_wait_result(shell_context_t* ctx, char args[][128], int arg_count) {
    os_task_exit_result_t result;
    int pid;
    int rc;
    (void)ctx;
    if (arg_count != 1 || (pid = parse_int(args[0])) < 0) {
        print_error("Usage: wait-result <pid>");
        return;
    }
    rc = sys_task_wait(pid);
    if (rc == OS_TASK_NOT_FOUND) {
        rc = sys_task_child_result_find(pid, &result);
        if (rc == 0) {
            print_string("wait-result ok "); print_int(result.child_pid); print_string(" ");
            print_int(result.exit_code); print_string(" "); print_int((int)result.reason); print_string("\n");
            return;
        }
        print_error("wait-result: ni enfant actif ni résultat retenu");
        return;
    }
    if (rc == OS_TASK_NOT_CHILD) {
        print_error("wait-result: cible non enfant direct");
        return;
    }
    if (rc != 0) {
        print_error("wait-result: syscall d’attente indisponible");
        return;
    }
    rc = sys_task_child_result(pid, &result);
    if (rc != 0) {
        print_error("wait-result: résultat enfant indisponible");
        return;
    }
    print_string("wait-result ok "); print_int(result.child_pid); print_string(" ");
    print_int(result.exit_code); print_string(" "); print_int((int)result.reason); print_string("\n");
}

void cmd_task_name(shell_context_t* ctx, char args[][128], int arg_count) {
    int pid;
    int rc;
    (void)ctx;
    if (arg_count != 2 || (pid = parse_int(args[0])) < 0) {
        print_error("Usage: task-name <pid> <nom>");
        return;
    }
    rc = sys_task_set_name(pid, args[1]);
    if (rc == OS_TASK_NOT_FOUND) {
        print_error("task-name: PID absent");
        return;
    }
    if (rc == OS_TASK_BAD_NAME) {
        print_error("task-name: nom invalide");
        return;
    }
    if (rc == OS_TASK_CONTROL_DENIED) {
        print_error("task-name: autorité limitée à soi ou enfant direct");
        return;
    }
    if (rc != 0) {
        print_error("task-name: syscall indisponible");
        return;
    }
    print_string("task-name ok "); print_int(pid); print_string(" "); print_string(args[1]); print_string("\n");
}

void cmd_task_capacity(shell_context_t* ctx, char args[][128], int arg_count) {
    os_task_capacity_t capacity;
    int rc;
    (void)ctx;
    if (arg_count != 0) {
        print_error("Usage: task-capacity");
        return;
    }
    rc = sys_task_capacity(&capacity);
    if (rc != 0) {
        print_error("task-capacity: syscall indisponible");
        return;
    }
    print_string("Tâches actives/capacité/disponibles : ");
    print_int((int)capacity.active); print_string("/");
    print_int((int)capacity.capacity); print_string("/");
    print_int((int)capacity.available); print_string("\n");
    print_string("task-capacity ok "); print_int((int)capacity.active); print_string(" ");
    print_int((int)capacity.capacity); print_string(" "); print_int((int)capacity.available); print_string("\n");
}

void cmd_child_result(shell_context_t* ctx, char args[][128], int arg_count) {
    os_task_exit_result_t result;
    int pid;
    int rc;
    (void)ctx;
    if (arg_count != 1 || (pid = parse_int(args[0])) < 0) {
        print_error("Usage: child-result <pid>");
        return;
    }
    rc = sys_task_child_result(pid, &result);
    if (rc == OS_TASK_NO_CHILD_RESULT) {
        print_error("child-result: aucun résultat local pour cet enfant");
        return;
    }
    if (rc == OS_TASK_NOT_FOUND) {
        print_error("child-result: parent indisponible");
        return;
    }
    if (rc != 0) {
        print_error("child-result: syscall indisponible");
        return;
    }
    print_string("Résultat enfant : PID "); print_int(result.child_pid);
    print_string(" code "); print_int(result.exit_code);
    print_string(" raison "); print_string(result.reason == OS_TASK_EVENT_EXITED ? "exited" : "killed");
    print_string(" tick "); print_int((int)result.finished_ticks); print_string("\n");
    print_string("child-result ok "); print_int(result.child_pid); print_string(" ");
    print_int(result.exit_code); print_string(" "); print_int((int)result.reason); print_string("\n");
}

void cmd_child_result_any(shell_context_t* ctx, char args[][128], int arg_count) {
    os_task_exit_result_t result;
    int pid;
    int rc;
    (void)ctx;
    if (arg_count != 1 || (pid = parse_int(args[0])) < 0) {
        print_error("Usage: child-result-any <pid>");
        return;
    }
    rc = sys_task_child_result_find(pid, &result);
    if (rc == OS_TASK_NO_CHILD_RESULT) {
        print_error("child-result-any: aucun résultat retenu pour cet enfant");
        return;
    }
    if (rc != 0) {
        print_error("child-result-any: syscall indisponible");
        return;
    }
    print_string("child-result-any ok "); print_int(result.child_pid); print_string(" ");
    print_int(result.exit_code); print_string(" "); print_int((int)result.reason); print_string("\n");
}

void cmd_child_results_forget(shell_context_t* ctx, char args[][128], int arg_count) {
    int pid;
    int generation;
    (void)ctx;
    if (arg_count != 1 || (pid = parse_int(args[0])) < 0) {
        print_error("Usage: child-results-forget <pid>");
        return;
    }
    generation = sys_task_child_result_forget(pid);
    if (generation == OS_TASK_NO_CHILD_RESULT) {
        print_error("child-results-forget: aucun résultat retenu pour cet enfant");
        return;
    }
    if (generation < 0) {
        print_error("child-results-forget: syscall indisponible");
        return;
    }
    print_string("child-results-forget ok "); print_int(pid); print_string(" ");
    print_int(generation); print_string("\n");
}

void cmd_child_results(shell_context_t* ctx, char args[][128], int arg_count) {
    os_task_exit_history_t history;
    uint32_t i;
    int rc;
    (void)ctx;
    if (arg_count != 0) {
        print_error("Usage: child-results");
        return;
    }
    rc = sys_task_child_result_list(&history);
    if (rc != 0) {
        print_error("child-results: syscall indisponible");
        return;
    }
    print_string("Historique résultats enfants : "); print_int((int)history.count); print_string("\n");
    for (i = 0U; i < history.count; i++) {
        print_string("  PID "); print_int(history.entries[i].child_pid);
        print_string(" code "); print_int(history.entries[i].exit_code);
        print_string(" raison "); print_string(history.entries[i].reason == OS_TASK_EVENT_EXITED ? "exited" : "killed");
        print_string(" tick "); print_int((int)history.entries[i].finished_ticks); print_string("\n");
        print_string("child-result-entry "); print_int(history.entries[i].child_pid); print_string(" ");
        print_int(history.entries[i].exit_code); print_string(" "); print_int((int)history.entries[i].reason); print_string("\n");
    }
    print_string("child-results ok "); print_int((int)history.count); print_string("\n");
}

void cmd_child_results_clear(shell_context_t* ctx, char args[][128], int arg_count) {
    int generation;
    (void)ctx;
    if (arg_count != 0) {
        print_error("Usage: child-results-clear");
        return;
    }
    generation = sys_task_child_result_ack();
    if (generation < 0) {
        print_error("child-results-clear: syscall indisponible");
        return;
    }
    print_string("child-results-clear ok "); print_int(generation); print_string("\n");
}

void cmd_child_results_observe(shell_context_t* ctx, char args[][128], int arg_count) {
    os_task_exit_history_observation_t observation;
    int expected;
    int rc;
    uint32_t i;
    (void)ctx;
    if (arg_count != 1 || (expected = parse_int(args[0])) < 0) {
        print_error("Usage: child-results-observe <generation>");
        return;
    }
    rc = sys_task_child_result_observe((uint32_t)expected, &observation);
    if (rc == OS_TASK_HISTORY_STALE) {
        print_string("child-results-observe stale "); print_int((int)observation.generation); print_string("\n");
        return;
    }
    if (rc != 0) {
        print_error("child-results-observe: syscall indisponible");
        return;
    }
    for (i = 0U; i < observation.history.count; i++) {
        print_string("child-result-entry "); print_int(observation.history.entries[i].child_pid); print_string(" ");
        print_int(observation.history.entries[i].exit_code); print_string(" "); print_int((int)observation.history.entries[i].reason); print_string("\n");
    }
    print_string("child-results-observe ok "); print_int((int)observation.generation); print_string(" ");
    print_int((int)observation.history.count); print_string("\n");
}

void cmd_sysinfo(shell_context_t* ctx, char args[][128], int arg_count) {
    print_colored("\n=== Informations Système MOHHDY ===\n", COLOR_CYAN);
    
    print_colored("Système d'exploitation : ", COLOR_YELLOW);
    print_string("MOHHDY v6.0\n");
    
    print_colored("Architecture : ", COLOR_YELLOW);
    print_string("i386 (32-bit)\n");
    
    print_colored("Processeur : ", COLOR_YELLOW);
    print_string("Intel compatible x86\n");
    
    print_colored("Mémoire totale : ", COLOR_YELLOW);
    {
        os_meminfo_t mi;
        if (sys_meminfo(&mi) == 0) {
            print_int((int)((mi.total_pages * 4) / 1024));
            print_string(" MB (PMM)\n");
        } else {
            print_string("inconnue\n");
        }
    }

    print_colored("Mémoire utilisée : ", COLOR_YELLOW);
    {
        os_meminfo_t mi;
        if (sys_meminfo(&mi) == 0) {
            print_int((int)((mi.used_pages * 4) / 1024));
            print_string(" MB\n");
        } else {
            print_string("inconnue\n");
        }
    }
    
    print_colored("Noyau : ", COLOR_YELLOW);
    print_string("MOHHDY Kernel v6.0 (Multitâche préemptif)\n");
    
    print_colored("Shell : ", COLOR_YELLOW);
    print_string("AI-Shell v6.0 (IA intégrée)\n");
    
    print_colored("Système de fichiers : ", COLOR_YELLOW);
    print_string("Initrd TAR + overlay RAM\n");
    
    print_colored("Fonctionnalités : ", COLOR_YELLOW);
    print_string("PMM, VMM, Multitâche, IA, Ring 0/3\n");
    
    print_colored("Uptime : ", COLOR_YELLOW);
    {
        unsigned int ticks = sys_ticks();
        print_int((int)(ticks / 100));
        print_string(" s (PIT)\n\n");
    }
}

void cmd_mem(shell_context_t* ctx, char args[][128], int arg_count) {
    os_meminfo_t mi;
    (void)ctx; (void)args; (void)arg_count;
    print_colored("\n=== Utilisation Mémoire (PMM) ===\n", COLOR_CYAN);
    if (sys_meminfo(&mi) != 0) {
        print_error("mem: syscall indisponible");
        return;
    }
    print_colored("Pages physiques :\n", COLOR_YELLOW);
    print_string("  Total : ");
    print_int((int)mi.total_pages);
    print_string("\n  Utilisees : ");
    print_int((int)mi.used_pages);
    print_string("\n  Libres : ");
    print_int((int)mi.free_pages);
    print_string("\n  Taille page : 4 KB\n");
    print_string("mem ok ");
    print_int((int)mi.total_pages);
    print_string(" ");
    print_int((int)mi.used_pages);
    print_string(" ");
    print_int((int)mi.free_pages);
    print_string("\n");
}

void cmd_history(shell_context_t* ctx, char args[][128], int arg_count) {
    print_colored("\n=== Historique des Commandes ===\n", COLOR_CYAN);
    
    if (ctx->history.count == 0) {
        print_string("Aucune commande dans l'historique.\n");
        print_string("history ok 0\n");
        return;
    }
    
    int start = (ctx->history.count > MAX_HISTORY) ? 
        ctx->history.count - MAX_HISTORY : 0;
    
    for (int i = 0; i < ctx->history.count && i < MAX_HISTORY; i++) {
        print_colored("  ", COLOR_YELLOW);
        
        // Afficher le numéro
        char num_str[16];
        int num = start + i + 1;
        int pos = 0;
        if (num == 0) {
            num_str[pos++] = '0';
        } else {
            while (num > 0) {
                num_str[pos++] = '0' + (num % 10);
                num /= 10;
            }
        }
        
        // Inverser la chaîne
        for (int j = 0; j < pos / 2; j++) {
            char tmp = num_str[j];
            num_str[j] = num_str[pos - 1 - j];
            num_str[pos - 1 - j] = tmp;
        }
        num_str[pos] = '\0';
        
        print_string(num_str);
        print_string("  ");
        print_string(ctx->history.commands[i]);
        print_string("\n");
    }
    print_string("history ok ");
    print_int(ctx->history.count);
    print_string("\n");
}

void cmd_env(shell_context_t* ctx, char args[][128], int arg_count) {
    print_colored("\n=== Variables d'Environnement ===\n", COLOR_CYAN);
    
    for (int i = 0; i < ctx->env_count; i++) {
        print_colored(ctx->env_vars[i].name, COLOR_YELLOW);
        print_string("=");
        print_string(ctx->env_vars[i].value);
        print_string("\n");
    }
    print_string("env ok ");
    print_int(ctx->env_count);
    print_string("\n");
}

void cmd_echo(shell_context_t* ctx, char args[][128], int arg_count) {
    int redir = -1;
    for (int i = 0; i < arg_count; i++) {
        if (strcmp(args[i], ">") == 0) {
            redir = i;
            break;
        }
    }
    if (redir >= 0) {
        char path[RAMFS_PATH_MAX];
        char buf[RAMFS_CONTENT_MAX];
        int pos = 0;
        int rc;
        if (redir + 1 >= arg_count) {
            print_error("echo: fichier manquant apres >");
            return;
        }
        for (int i = 0; i < redir; i++) {
            int j = 0;
            if (i > 0 && pos < RAMFS_CONTENT_MAX - 2) buf[pos++] = ' ';
            while (args[i][j] && pos < RAMFS_CONTENT_MAX - 2) buf[pos++] = args[i][j++];
        }
        buf[pos++] = '\n';
        buf[pos] = '\0';
        resolve_arg(ctx, args[redir + 1], path);
        rc = sys_writefile(path, buf, pos);
        if (rc < 0) print_fs_err("echo", rc);
        return;
    }
    for (int i = 0; i < arg_count; i++) {
        if (strcmp(args[i], "$?") == 0) print_int(ctx->last_rc);
        else print_string(args[i]);
        if (i < arg_count - 1) print_string(" ");
    }
    print_string("\n");
    print_string("echo ok\n");
}

void cmd_write(shell_context_t* ctx, char args[][128], int arg_count) {
    char path[RAMFS_PATH_MAX];
    char buf[256];
    int pos = 0;
    int rc;
    if (arg_count == 0) {
        print_error("write: fichier manquant");
        return;
    }
    resolve_arg(ctx, args[0], path);
    for (int i = 1; i < arg_count; i++) {
        int j = 0;
        if (i > 1 && pos < 254) buf[pos++] = ' ';
        while (args[i][j] && pos < 254) buf[pos++] = args[i][j++];
    }
    buf[pos++] = '\n';
    buf[pos] = '\0';
    rc = sys_writefile(path, buf, pos);
    if (rc < 0) {
        print_fs_err("write", rc);
        return;
    }
    print_string("write ok ");
    print_string(args[0]);
    print_string("\n");
}

static void cmd_append(shell_context_t* ctx, char args[][128], int arg_count) {
    char path[RAMFS_PATH_MAX];
    char buf[256];
    int pos = 0;
    int rc;
    if (arg_count < 2) {
        print_error("append: fichier ou texte manquant");
        return;
    }
    resolve_arg(ctx, args[0], path);
    for (int i = 1; i < arg_count; i++) {
        int j = 0;
        if (i > 1 && pos < 254) buf[pos++] = ' ';
        while (args[i][j] && pos < 254) buf[pos++] = args[i][j++];
    }
    buf[pos++] = '\n';
    buf[pos] = '\0';
    rc = sys_append(path, buf, pos);
    if (rc < 0) {
        print_fs_err("append", rc);
        return;
    }
    print_string("append ok ");
    print_string(args[0]);
    print_string("\n");
}

static void cmd_touch(shell_context_t* ctx, char args[][128], int arg_count) {
    char path[RAMFS_PATH_MAX];
    os_dirent_t st;
    int rc;
    if (arg_count == 0) {
        print_error("touch: fichier manquant");
        return;
    }
    resolve_arg(ctx, args[0], path);
    if (sys_stat(path, &st) == 0) {
        if (st.flags == OS_DIRENT_DIR) {
            print_error("touch: est un repertoire");
            return;
        }
        print_string("touch ok ");
        print_string(args[0]);
        print_string("\n");
        return;
    }
    rc = sys_writefile(path, "", 0);
    if (rc < 0) {
        print_fs_err("touch", rc);
        return;
    }
    print_string("touch ok ");
    print_string(args[0]);
    print_string("\n");
}

static void cmd_stat(shell_context_t* ctx, char args[][128], int arg_count) {
    char path[RAMFS_PATH_MAX];
    os_dirent_t st;
    int size = 0;
    int is_dir = 0;
    if (arg_count == 0) {
        print_error("stat: chemin manquant");
        return;
    }
    resolve_arg(ctx, args[0], path);
    if (sys_stat(path, &st) == 0) {
        is_dir = (st.flags == OS_DIRENT_DIR);
        size = (int)st.size;
    } else if (ramfs_is_dir(path)) {
        is_dir = 1;
        size = 0;
    } else if (ramfs_is_file(path)) {
        if (!ramfs_read(path, &size)) size = 0;
        is_dir = 0;
    } else {
        print_error("stat: introuvable");
        return;
    }
    print_string(is_dir ? "stat dir " : "stat file ");
    print_string(args[0]);
    print_string(" ");
    print_int(size);
    print_string("\n");
}

static int lookup_path_kind(shell_context_t* ctx, const char* arg, int* is_dir) {
    char path[RAMFS_PATH_MAX];
    os_dirent_t st;
    resolve_arg(ctx, arg, path);
    if (sys_stat(path, &st) == 0) {
        *is_dir = (st.flags == OS_DIRENT_DIR);
        return 1;
    }
    if (ramfs_is_dir(path)) {
        *is_dir = 1;
        return 1;
    }
    if (ramfs_is_file(path)) {
        *is_dir = 0;
        return 1;
    }
    return 0;
}

static void cmd_test(shell_context_t* ctx, char args[][128], int arg_count) {
    int is_dir = 0;
    int found;
    int ok = 0;
    const char* flag;
    const char* name;
    if (arg_count < 2) {
        print_error("test: usage test f|d|e <chemin>");
        ctx->last_rc = 1;
        return;
    }
    flag = args[0];
    name = args[1];
    found = lookup_path_kind(ctx, name, &is_dir);
    if (strcmp(flag, "-f") == 0 || strcmp(flag, "f") == 0) ok = found && !is_dir;
    else if (strcmp(flag, "-d") == 0 || strcmp(flag, "d") == 0) ok = found && is_dir;
    else if (strcmp(flag, "-e") == 0 || strcmp(flag, "e") == 0) ok = found;
    else {
        print_error("test: flag inconnu");
        ctx->last_rc = 1;
        return;
    }
    if (ok) {
        if (strcmp(flag, "-f") == 0 || strcmp(flag, "f") == 0) print_string("test ok file ");
        else if (strcmp(flag, "-d") == 0 || strcmp(flag, "d") == 0) print_string("test ok dir ");
        else print_string("test ok ");
        print_string(name);
        print_string("\n");
        ctx->last_rc = 0;
    } else {
        print_string("test no ");
        print_string(name);
        print_string("\n");
        ctx->last_rc = 1;
    }
}

void cmd_clear(shell_context_t* ctx, char args[][128], int arg_count) {
    // Séquence ANSI pour effacer l'écran
    print_string("\x1b[2J\x1b[H");
    
    // Banner de bienvenue moderne
    print_colored("===========================================================\n", COLOR_CYAN);
    print_colored("    [MOHHDY] v6.0 - Intelligence artificielle intégrée    \n", COLOR_BRIGHT);
    print_colored("===========================================================\n", COLOR_CYAN);
    print_colored("[shell] Shell avancé", COLOR_GREEN);
    print_string(" | ");
    print_colored("[IA] IA intégrée", COLOR_MAGENTA);
    print_string(" | ");
    print_colored("[perf] Haute performance\n", COLOR_YELLOW);
    print_string("\n");
    print_info("Tapez 'help' pour voir toutes les commandes disponibles");
    print_info("Mode IA activé - Posez vos questions directement !");
    print_string("\n");
}

// === Builtins de navigation et de lecture de fichiers ===
static void cmd_pwd(shell_context_t* ctx) {
    print_string(ctx->current_dir);
    print_string("\n");
}

static void cmd_cd(shell_context_t* ctx, char args[][128], int arg_count) {
    char newdir[RAMFS_PATH_MAX];
    if (arg_count == 0) {
        const char* home = get_env_var(ctx, "HOME");
        if (!home) home = "/";
        ramfs_resolve("/", home, newdir, RAMFS_PATH_MAX);
    } else {
        ramfs_resolve(ctx->current_dir, args[0], newdir, RAMFS_PATH_MAX);
    }
    if (!ramfs_is_dir(newdir)) {
        os_dirent_t st;
        if (sys_stat(newdir, &st) != 0 || st.flags != OS_DIRENT_DIR) {
            print_error("cd: repertoire introuvable");
            return;
        }
    }
    strcpy(ctx->current_dir, newdir);
    print_string("cd ok ");
    if (arg_count == 0) print_string(ctx->current_dir);
    else print_string(args[0]);
    print_string("\n");
}

static void cmd_cat(shell_context_t* ctx, char args[][128], int arg_count) {
    char path[RAMFS_PATH_MAX];
    char kbuf[1024];
    const char* data;
    int size = 0;
    int kn;
    if (arg_count == 0) {
        print_error("cat: fichier manquant");
        return;
    }
    resolve_arg(ctx, args[0], path);
    kn = sys_readfile(path, kbuf, sizeof(kbuf));
    if (kn >= 0) {
        for (int i = 0; i < kn; i++) putc(kbuf[i]);
        if (kn == 0 || kbuf[kn - 1] != '\n') print_string("\n");
        return;
    }
    if (ramfs_is_dir(path)) {
        print_error("cat: est un repertoire");
        return;
    }
    data = ramfs_read(path, &size);
    if (!data) {
        print_error("cat: fichier introuvable");
        return;
    }
    for (int i = 0; i < size; i++) putc(data[i]);
    if (size == 0 || data[size - 1] != '\n') print_string("\n");
}

static int is_builtin(const char* cmd) {
    static const char* names[] = {
        "help", "ls", "dir", "ps", "task-metrics", "task-priority", "task-name", "task-capacity", "task-suspend", "task-resume", "kill-children", "children", "wait-any-result", "child-exit-count", "task-delegate", "task-events", "task-events-observe", "task-events-clear", "task-event", "task-events-forget", "task-summary", "task-events-notify", "task-events-filter", "task-events-notify-status", "task-events-watch", "task-events-unwatch", "task-events-watch-clear", "task-events-watch-status", "task-events-notify-stats", "task-events-notify-stats-clear", "task-event-replay", "task-priority-child", "task-priority-child-status", "task-events-budget", "task-events-budget-status", "fat16-list", "fat16-cat", "child-result", "child-result-any", "child-results", "child-results-clear", "child-results-observe", "child-results-forget", "wait", "wait-result", "sysinfo", "info", "mem", "memory",
        "history", "env", "echo", "write", "append", "touch", "clear", "cls", "exit", "quit",
        "ai", "ai-mode", "ai-help", "ai-test", "ai-stats", "ai-provider", "ai-model", "ai-runtime", "ai-continue", "net-status",
        "cd", "pwd", "cat", "stat", "test", "[", "mkdir", "rmdir", "cp", "mv", "rm",
        "kill", "spawn", "yield", "ipc-send", "ipc-recv", "service-publish", "service-grant", "service-find", "service-status", "service-watch", "vfs-backend-probe", "vfs-backend-write-probe", "vfs-backend-remove-probe", "vfs-backend-rename-probe", "vfs-grant", "vfs-read", "vfs-stat", "vfs-stats", "vfs-mount-add", "vfs-mount-remove", "vfs-write", "vfs-remove", "vfs-rename", "vfs-mkdir", "vfs-rmdir", "jobs", "top", "getpid", "uptime", "date", "whoami",
        "alias", "unalias", "export", "which", "rc",
        "grep", "wc", "sort", "head", "tail",
        "logout", "reboot", "shutdown",
        "aistats", "aimode", "aihelp", "aitest",
        0
    };
    for (int i = 0; names[i]; i++) {
        if (strcmp(cmd, names[i]) == 0) return 1;
    }
    return 0;
}

static void cmd_which(shell_context_t* ctx, const char* cmd) {
    (void)ctx;
    if (is_builtin(cmd)) {
        print_string("which ok builtin ");
        print_string(cmd);
        print_string("\n");
        return;
    }
    print_string("which ok bin/");
    print_string(cmd);
    print_string("\n");
}

static void print_fs_err(const char* cmd, int rc) {
    print_string(cmd);
    print_string(": ");
    if (rc == -2) print_string("existe deja\n");
    else if (rc == -3) print_string("parent invalide\n");
    else if (rc == -4) print_string("est un repertoire\n");
    else if (rc == -5) print_string("repertoire non vide\n");
    else if (rc == -6) print_string("plus de place\n");
    else if (rc == -8) print_string("protege (initrd)\n");
    else print_string("echec\n");
}

static void cmd_mkdir(shell_context_t* ctx, char args[][128], int arg_count) {
    char path[RAMFS_PATH_MAX];
    int rc;
    if (arg_count == 0) {
        print_error("mkdir: repertoire manquant");
        return;
    }
    resolve_arg(ctx, args[0], path);
    rc = sys_mkdir(path);
    if (rc != 0) {
        print_fs_err("mkdir", rc);
        return;
    }
    print_string("mkdir ok ");
    print_string(args[0]);
    print_string("\n");
}

static void cmd_rmdir(shell_context_t* ctx, char args[][128], int arg_count) {
    char path[RAMFS_PATH_MAX];
    os_dirent_t st;
    int rc;
    if (arg_count == 0) {
        print_error("rmdir: repertoire manquant");
        return;
    }
    resolve_arg(ctx, args[0], path);
    if (sys_stat(path, &st) == 0) {
        if (st.flags != OS_DIRENT_DIR) {
            print_error("rmdir: n'est pas un repertoire");
            return;
        }
        rc = sys_unlink(path);
        if (rc != 0) {
            print_fs_err("rmdir", rc);
            return;
        }
        print_string("rmdir ok ");
        print_string(args[0]);
        print_string("\n");
        return;
    }
    rc = ramfs_rmdir(path);
    if (rc != RAMFS_OK) print_ramfs_err("rmdir", rc);
}

static void cmd_rm(shell_context_t* ctx, char args[][128], int arg_count) {
    char path[RAMFS_PATH_MAX];
    os_dirent_t st;
    int rc;
    if (arg_count == 0) {
        print_error("rm: fichier manquant");
        return;
    }
    resolve_arg(ctx, args[0], path);
    if (sys_stat(path, &st) == 0) {
        if (st.flags == OS_DIRENT_DIR) {
            print_error("rm: est un repertoire");
            return;
        }
        rc = sys_unlink(path);
        if (rc != 0) {
            print_fs_err("rm", rc);
            return;
        }
        print_string("rm ok ");
        print_string(args[0]);
        print_string("\n");
        return;
    }
    rc = ramfs_rm(path);
    if (rc != RAMFS_OK) print_ramfs_err("rm", rc);
}

static const char* fs_basename(const char* path) {
    const char* b = path ? path : "";
    int i = 0;
    while (path && path[i]) {
        if (path[i] == '/') b = path + i + 1;
        i++;
    }
    return (b && b[0]) ? b : "/";
}

static void fs_join(char* out, int max, const char* dir, const char* name) {
    int i = 0;
    int j = 0;
    if (!dir) dir = "/";
    if (!name) name = "";
    while (dir[i] && i < max - 2) {
        out[i] = dir[i];
        i++;
    }
    if (i > 0 && out[i - 1] != '/') out[i++] = '/';
    while (name[j] && i < max - 1) out[i++] = name[j++];
    out[i] = '\0';
}

static int kernel_copy_file_to(const char* src, const char* dest) {
    os_dirent_t st;
    char buf[256];
    int n;
    int w;
    if (sys_stat(src, &st) != 0) return -1;
    if (st.flags == OS_DIRENT_DIR) return -4;
    n = sys_readfile(src, buf, (int)sizeof(buf));
    if (n < 0) return n;
    if (sys_stat(dest, &st) == 0 && st.flags == OS_DIRENT_DIR) return -4;
    w = sys_writefile(dest, buf, n);
    if (w < 0) return w;
    return 0;
}

static void cmd_cp(shell_context_t* ctx, char args[][128], int arg_count) {
    char src[RAMFS_PATH_MAX];
    char dst[RAMFS_PATH_MAX];
    os_dirent_t st;
    int src_dir;
    int rc;
    if (arg_count < 2) {
        print_error("cp: usage cp <src> <dest>");
        return;
    }
    resolve_arg(ctx, args[0], src);
    resolve_arg(ctx, args[1], dst);
    if (sys_stat(src, &st) == 0) {
        src_dir = (st.flags == OS_DIRENT_DIR);
        if (sys_stat(dst, &st) == 0 && st.flags == OS_DIRENT_DIR) {
            char joined[RAMFS_PATH_MAX];
            fs_join(joined, RAMFS_PATH_MAX, dst, fs_basename(src));
            strcpy(dst, joined);
        }
        if (src_dir) rc = sys_copy(src, dst);
        else rc = kernel_copy_file_to(src, dst);
        if (rc != 0) {
            print_fs_err("cp", rc);
            return;
        }
        print_string("cp ok ");
        print_string(fs_basename(dst));
        print_string("\n");
        return;
    }
    rc = ramfs_cp(src, dst);
    if (rc != RAMFS_OK) print_ramfs_err("cp", rc);
}

static void cmd_mv(shell_context_t* ctx, char args[][128], int arg_count) {
    char src[RAMFS_PATH_MAX];
    char dst[RAMFS_PATH_MAX];
    os_dirent_t st;
    int rc;
    if (arg_count < 2) {
        print_error("mv: usage mv <src> <dest>");
        return;
    }
    resolve_arg(ctx, args[0], src);
    resolve_arg(ctx, args[1], dst);
    if (sys_stat(src, &st) == 0) {
        if (sys_stat(dst, &st) == 0 && st.flags == OS_DIRENT_DIR) {
            char joined[RAMFS_PATH_MAX];
            fs_join(joined, RAMFS_PATH_MAX, dst, fs_basename(src));
            strcpy(dst, joined);
        }
        rc = sys_rename(src, dst);
        if (rc != 0) {
            print_fs_err("mv", rc);
            return;
        }
        print_string("mv ok ");
        print_string(fs_basename(dst));
        print_string("\n");
        return;
    }
    rc = ramfs_mv(src, dst);
    if (rc != RAMFS_OK) print_ramfs_err("mv", rc);
}

static void cmd_kill(shell_context_t* ctx, char args[][128], int arg_count) {
    int pid;
    int rc;
    (void)ctx;
    if (arg_count == 0) {
        print_error("kill: pid manquant");
        return;
    }
    pid = parse_int(args[0]);
    rc = sys_kill_pid(pid);
    if (rc == -2) print_error("kill: processus protege (kernel)");
    else if (rc == -3) print_error("kill: impossible de tuer le shell courant (exit)");
    else if (rc != 0) print_error("kill: pid introuvable");
    else {
        print_string("Processus ");
        print_int(pid);
        print_string(" termine\n");
    }
}

static void cmd_spawn(shell_context_t* ctx, char args[][128], int arg_count) {
    int pid;
    (void)ctx;
    if (arg_count == 0) {
        print_error("spawn: programme manquant");
        return;
    }
    pid = spawn(args[0], 0);
    if (pid == OS_TASK_CHILD_LIMIT) {
        print_error("spawn: capacité de quatre enfants atteinte");
        return;
    }
    if (pid < 0) {
        char alt[80];
        int i = 0;
        alt[0] = 'b'; alt[1] = 'i'; alt[2] = 'n'; alt[3] = '/';
        while (args[0][i] && i < 70) {
            alt[4 + i] = args[0][i];
            i++;
        }
        alt[4 + i] = '\0';
        pid = spawn(alt, 0);
    }
    if (pid == OS_TASK_CHILD_LIMIT) {
        print_error("spawn: capacité de quatre enfants atteinte");
        return;
    }
    if (pid < 0) {
        print_error("spawn: programme introuvable");
        return;
    }
    print_string("spawn ok pid ");
    print_int(pid);
    print_string(" ");
    print_string(args[0]);
    print_string("\n");
}

static void cmd_ipc_send(shell_context_t* ctx, char args[][128], int arg_count) {
    os_ipc_payload_t payload;
    int pid;
    int rc;
    uint32_t i = 0U;
    if (arg_count != 2) {
        print_error("Usage: ipc-send <pid> <texte_sans_espace>");
        return;
    }
    pid = parse_int(args[0]);
    if (pid <= 0) {
        print_error("ipc-send: pid invalide");
        return;
    }
    payload.type = 0U;
    payload.request_id = 0U;
    while (args[1][i] != '\0' && i < OS_IPC_MAX_DATA) {
        payload.data[i] = (uint8_t)args[1][i];
        i++;
    }
    if (args[1][i] != '\0') {
        print_error("ipc-send: texte trop long");
        return;
    }
    payload.size = i;
    while (i < OS_IPC_MAX_DATA) payload.data[i++] = 0U;
    rc = sys_ipc_send(pid, &payload);
    ctx->last_rc = rc;
    if (rc == OS_IPC_SERVICE_FULL) print_error("ipc-send: capacite du service atteinte");
    else if (rc == OS_IPC_FULL) print_error("ipc-send: boite aux lettres pleine");
    else if (rc == OS_IPC_BAD_TARGET) print_error("ipc-send: cible utilisateur introuvable");
    else if (rc != 0) print_error("ipc-send: message invalide");
    else {
        print_string("ipc-send ok ");
        print_int(pid);
        print_string(" ");
        print_int((int)payload.size);
        print_string("\n");
    }
}

static void cmd_ipc_recv(shell_context_t* ctx, char args[][128], int arg_count) {
    os_ipc_message_t message;
    os_service_event_t event;
    os_task_event_t task_event;
    os_task_supervision_event_t supervision_event;
    int rc;
    uint32_t i;
    (void)args;
    (void)arg_count;
    rc = os_ipc_deferred_take(&ipc_deferred, &message);
    if (rc == OS_IPC_EMPTY) rc = sys_ipc_receive(&message);
    ctx->last_rc = rc;
    if (rc == OS_IPC_EMPTY) {
        print_string("ipc-recv empty\n");
        return;
    }
    if (rc != 0) {
        print_error("ipc-recv: erreur");
        return;
    }
    if (os_service_parse_event(&message, &event) == 0) {
        print_string("service-event ");
        print_string(event.name);
        print_string(" old ");
        print_int(event.old_owner_pid);
        print_string(" new ");
        print_int(event.new_owner_pid);
        print_string(" reason ");
        print_int((int)event.reason);
        print_string("\n");
        return;
    }
    if (os_task_parse_event(&message, &task_event) == 0) {
        print_string("task-event child ");
        print_int(task_event.child_pid);
        print_string(" reason ");
        print_string(task_event.reason == OS_TASK_EVENT_EXITED ? "exited" : "killed");
        print_string("\n");
        return;
    }
    if (os_task_parse_supervision_event(&message, &supervision_event) == 0) {
        print_string("task-supervision-event ");
        print_uint(supervision_event.sequence);
        print_string(" ");
        print_string(supervision_action_name(supervision_event.action));
        print_string(" ");
        print_int(supervision_event.child_pid);
        print_string(" ");
        print_int(supervision_event.related_pid);
        print_string(" ");
        print_uint(supervision_event.detail);
        print_string(" ");
        print_uint(supervision_event.ticks);
        print_string("\n");
        return;
    }
    print_string("ipc-recv from ");
    print_int(message.sender_pid);
    print_string(" type ");
    print_int((int)message.type);
    print_string(" data ");
    for (i = 0U; i < message.size; i++) putc((char)message.data[i]);
    print_string("\n");
}

static void cmd_service_publish(shell_context_t* ctx, char args[][128], int arg_count) {
    int rc;
    if (arg_count != 1) {
        print_error("Usage: service-publish <nom>");
        return;
    }
    rc = sys_service_register(args[0]);
    ctx->last_rc = rc;
    if (rc == 0) {
        print_string("service-publish ok ");
        print_string(args[0]);
        print_string("\n");
    } else if (rc == OS_SERVICE_TAKEN) {
        print_error("service-publish: nom deja reserve");
    } else {
        print_error("service-publish: nom invalide ou registre plein");
    }
}

static void cmd_service_grant(shell_context_t* ctx, char args[][128], int arg_count) {
    int pid;
    int rc;
    if (arg_count != 2) {
        print_error("Usage: service-grant <nom> <pid>");
        return;
    }
    pid = parse_int(args[1]);
    if (pid <= 0) {
        print_error("service-grant: pid invalide");
        return;
    }
    rc = sys_service_grant(args[0], pid);
    ctx->last_rc = rc;
    if (rc == 0) {
        print_string("service-grant ok ");
        print_string(args[0]);
        print_string(" ");
        print_int(pid);
        print_string("\n");
    } else if (rc == OS_SERVICE_NOT_OWNER) {
        print_error("service-grant: nom non detenue par ce shell");
    } else if (rc == OS_SERVICE_BAD_GRANTEE) {
        print_error("service-grant: beneficiaire utilisateur introuvable");
    } else {
        print_error("service-grant: nom invalide ou indisponible");
    }
}

static void cmd_service_find(shell_context_t* ctx, char args[][128], int arg_count) {
    int pid;
    if (arg_count != 1) {
        print_error("Usage: service-find <nom>");
        return;
    }
    pid = sys_service_lookup(args[0]);
    ctx->last_rc = pid < 0 ? pid : 0;
    if (pid > 0) {
        print_string("service-find ok ");
        print_string(args[0]);
        print_string(" ");
        print_int(pid);
        print_string("\n");
    } else {
        print_error("service-find: service indisponible");
    }
}

static void cmd_service_status(shell_context_t* ctx, char args[][128], int arg_count) {
    os_service_status_t status;
    int rc;
    if (arg_count != 1) {
        print_error("Usage: service-status <nom>");
        return;
    }
    rc = sys_service_status(args[0], &status);
    ctx->last_rc = rc;
    if (rc == 0) {
        print_string("service-status ok ");
        print_string(args[0]);
        print_string(" pid ");
        print_int(status.owner_pid);
        print_string(" queued ");
        print_int((int)status.queued_messages);
        print_string(" client-capacity ");
        print_int((int)status.client_capacity);
        print_string(" endpoint-capacity ");
        print_int((int)status.endpoint_capacity);
        print_string("\n");
    } else if (rc == OS_SERVICE_NOT_FOUND) {
        print_error("service-status: service indisponible");
    } else {
        print_error("service-status: requete invalide");
    }
}

static void cmd_service_watch(shell_context_t* ctx, char args[][128], int arg_count) {
    int rc;
    if (arg_count != 1) {
        print_error("Usage: service-watch <nom>");
        return;
    }
    rc = sys_service_notify(args[0]);
    ctx->last_rc = rc;
    if (rc == 0) {
        print_string("service-watch ok ");
        print_string(args[0]);
        print_string("\n");
    } else if (rc == OS_SERVICE_WATCH_FULL) {
        print_error("service-watch: limite d'abonnements atteinte");
    } else {
        print_error("service-watch: nom invalide");
    }
}

static void cmd_vfs_backend_probe(shell_context_t* ctx, char args[][128], int arg_count) {
    char data[OS_VFS_READ_MAX];
    int rc;
    if (arg_count != 1) {
        print_error("Usage: vfs-backend-probe <fichier>");
        return;
    }
    rc = sys_vfs_backend_read(args[0], data, OS_VFS_READ_MAX);
    ctx->last_rc = rc;
    if (rc == OS_VFS_BACKEND_DENIED) {
        print_string("vfs-backend-probe denied\n");
    } else {
        print_error("vfs-backend-probe: acces inattendu ou erreur backend");
    }
}

static void cmd_vfs_backend_write_probe(shell_context_t* ctx, char args[][128], int arg_count) {
    uint32_t size = 0U;
    int rc;
    if (arg_count != 2) {
        print_error("Usage: vfs-backend-write-probe <fichier> <texte>");
        return;
    }
    while (args[1][size] != '\0') size++;
    rc = sys_vfs_backend_write(args[0], args[1], size);
    ctx->last_rc = rc;
    if (rc == OS_VFS_BACKEND_DENIED) {
        print_string("vfs-backend-write-probe denied\n");
    } else if (rc == 0) {
        print_string("vfs-backend-write-probe unexpectedly allowed\n");
    } else {
        print_error("vfs-backend-write-probe: erreur backend");
    }
}

static void cmd_vfs_backend_remove_probe(shell_context_t* ctx, char args[][128], int arg_count) {
    int rc;
    if (arg_count != 1) {
        print_error("Usage: vfs-backend-remove-probe <fichier>");
        return;
    }
    rc = sys_vfs_overlay_unlink(args[0]);
    ctx->last_rc = rc;
    if (rc == OS_VFS_BACKEND_DENIED) {
        print_string("vfs-backend-remove-probe denied\n");
    } else if (rc == 0) {
        print_string("vfs-backend-remove-probe unexpectedly allowed\n");
    } else {
        print_error("vfs-backend-remove-probe: erreur backend");
    }
}

static void cmd_vfs_backend_rename_probe(shell_context_t* ctx, char args[][128], int arg_count) {
    int rc;
    if (arg_count != 2) {
        print_error("Usage: vfs-backend-rename-probe <src> <dst>");
        return;
    }
    rc = sys_vfs_overlay_rename(args[0], args[1]);
    ctx->last_rc = rc;
    if (rc == OS_VFS_BACKEND_DENIED) {
        print_string("vfs-backend-rename-probe denied\n");
    } else if (rc == 0) {
        print_string("vfs-backend-rename-probe unexpectedly allowed\n");
    } else {
        print_error("vfs-backend-rename-probe: erreur backend");
    }
}

static void cmd_vfs_grant(shell_context_t* ctx, char args[][128], int arg_count) {
    os_ipc_payload_t request;
    int pid;
    int target_pid;
    int rc;
    if (arg_count != 1) {
        print_error("Usage: vfs-grant <pid>");
        return;
    }
    target_pid = parse_int(args[0]);
    if (target_pid <= 0) {
        print_error("vfs-grant: pid invalide");
        return;
    }
    pid = sys_service_lookup("vfs");
    if (pid <= 0) {
        print_error("vfs-grant: service vfs indisponible");
        ctx->last_rc = pid;
        return;
    }
    rc = os_vfs_make_grant_request(&request, target_pid);
    if (rc == 0) rc = sys_ipc_send(pid, &request);
    ctx->last_rc = rc;
    if (rc == 0) {
        print_string("vfs-grant sent ");
        print_int(target_pid);
        print_string(" via ");
        print_int(pid);
        print_string("\n");
    } else {
        print_error("vfs-grant: demande refusee");
    }
}

static void cmd_vfs_backend_grant(shell_context_t* ctx, char args[][128], int arg_count) {
    os_ipc_payload_t request; os_ipc_message_t message;
    int pid, target_pid, rc, status; uint32_t request_id;
    if (arg_count != 1 || (target_pid = parse_int(args[0])) <= 0) { print_error("Usage: vfs-backend-grant <pid>"); return; }
    pid = sys_service_lookup("vfs");
    if (pid <= 0) { print_error("vfs-backend-grant: service vfs indisponible"); ctx->last_rc = pid; return; }
    request_id = next_vfs_request_id(); rc = os_vfs_make_backend_grant_request(&request, target_pid, request_id);
    if (rc == 0) rc = sys_ipc_send(pid, &request);
    if (rc != 0) { print_error("vfs-backend-grant: demande refusee"); ctx->last_rc = rc; return; }
    rc = wait_ipc_reply(pid, OS_IPC_VFS_BACKEND_GRANT_REPLY, request_id, &message);
    if (rc == 0) rc = os_vfs_parse_backend_grant_reply(&message, &status, request_id);

    if (rc != 0 || status != 0) { print_error("vfs-backend-grant: delegation refusee"); ctx->last_rc = rc != 0 ? rc : status; return; }
    ctx->last_rc = 0; print_string("vfs-backend-grant ok request "); print_int((int)request_id); print_string("\n");
}

static void cmd_vfs_backend_grant_read(shell_context_t* ctx, char args[][128], int arg_count) {
    os_ipc_payload_t request; os_ipc_message_t message;
    int pid, target_pid, rc, status; uint32_t request_id;
    if (arg_count != 1 || (target_pid = parse_int(args[0])) <= 0) { print_error("Usage: vfs-backend-grant-read <pid>"); return; }
    pid = sys_service_lookup("vfs");
    if (pid <= 0) { print_error("vfs-backend-grant-read: service vfs indisponible"); ctx->last_rc = pid; return; }
    request_id = next_vfs_request_id(); rc = os_vfs_make_backend_grant_scoped_request(&request, target_pid, OS_VFS_BACKEND_RIGHT_READ, request_id);
    if (rc == 0) rc = sys_ipc_send(pid, &request);
    if (rc != 0) { print_error("vfs-backend-grant-read: demande refusee"); ctx->last_rc = rc; return; }
    rc = wait_ipc_reply(pid, OS_IPC_VFS_BACKEND_GRANT_SCOPED_REPLY, request_id, &message);
    if (rc == 0) rc = os_vfs_parse_backend_grant_scoped_reply(&message, &status, request_id);

    if (rc != 0 || status != 0) { print_error("vfs-backend-grant-read: delegation refusee"); ctx->last_rc = rc != 0 ? rc : status; return; }
    ctx->last_rc = 0; print_string("vfs-backend-grant-read ok request "); print_int((int)request_id); print_string("\n");
}

static void cmd_vfs_backend_grant_mutate(shell_context_t* ctx, char args[][128], int arg_count) {
    os_ipc_payload_t request; os_ipc_message_t message;
    int pid, target_pid, rc, status; uint32_t request_id;
    if (arg_count != 1 || (target_pid = parse_int(args[0])) <= 0) { print_error("Usage: vfs-backend-grant-mutate <pid>"); return; }
    pid = sys_service_lookup("vfs");
    if (pid <= 0) { print_error("vfs-backend-grant-mutate: service vfs indisponible"); ctx->last_rc = pid; return; }
    request_id = next_vfs_request_id(); rc = os_vfs_make_backend_grant_scoped_request(&request, target_pid, OS_VFS_BACKEND_RIGHT_MUTATE, request_id);
    if (rc == 0) rc = sys_ipc_send(pid, &request);
    if (rc != 0) { print_error("vfs-backend-grant-mutate: demande refusee"); ctx->last_rc = rc; return; }
    rc = wait_ipc_reply(pid, OS_IPC_VFS_BACKEND_GRANT_SCOPED_REPLY, request_id, &message);
    if (rc == 0) rc = os_vfs_parse_backend_grant_scoped_reply(&message, &status, request_id);

    if (rc != 0 || status != 0) { print_error("vfs-backend-grant-mutate: delegation refusee"); ctx->last_rc = rc != 0 ? rc : status; return; }
    ctx->last_rc = 0; print_string("vfs-backend-grant-mutate ok request "); print_int((int)request_id); print_string("\n");
}

static void cmd_vfs_backend_revoke(shell_context_t* ctx, char args[][128], int arg_count) {
    os_ipc_payload_t request; os_ipc_message_t message;
    int pid, target_pid, rc, status; uint32_t request_id;
    if (arg_count != 1 || (target_pid = parse_int(args[0])) <= 0) { print_error("Usage: vfs-backend-revoke <pid>"); return; }
    pid = sys_service_lookup("vfs");
    if (pid <= 0) { print_error("vfs-backend-revoke: service vfs indisponible"); ctx->last_rc = pid; return; }
    request_id = next_vfs_request_id(); rc = os_vfs_make_backend_revoke_request(&request, target_pid, request_id);
    if (rc == 0) rc = sys_ipc_send(pid, &request);
    if (rc != 0) { print_error("vfs-backend-revoke: demande refusee"); ctx->last_rc = rc; return; }
    rc = wait_ipc_reply(pid, OS_IPC_VFS_BACKEND_REVOKE_REPLY, request_id, &message);
    if (rc == 0) rc = os_vfs_parse_backend_revoke_reply(&message, &status, request_id);

    if (rc != 0 || status != 0) { print_error("vfs-backend-revoke: revocation refusee"); ctx->last_rc = rc != 0 ? rc : status; return; }
    ctx->last_rc = 0; print_string("vfs-backend-revoke ok request "); print_int((int)request_id); print_string("\n");
}

static void cmd_vfs_backend_status(shell_context_t* ctx, char args[][128], int arg_count) {
    os_ipc_payload_t request; os_ipc_message_t message;
    int pid, target_pid, rc, status; uint32_t request_id, rights;
    if (arg_count != 1 || (target_pid = parse_int(args[0])) <= 0) { print_error("Usage: vfs-backend-status <pid>"); return; }
    pid = sys_service_lookup("vfs");
    if (pid <= 0) { print_error("vfs-backend-status: service vfs indisponible"); ctx->last_rc = pid; return; }
    request_id = next_vfs_request_id(); rc = os_vfs_make_backend_status_request(&request, target_pid, request_id);
    if (rc == 0) rc = sys_ipc_send(pid, &request);
    if (rc != 0) { print_error("vfs-backend-status: demande refusee"); ctx->last_rc = rc; return; }
    rc = wait_ipc_reply(pid, OS_IPC_VFS_BACKEND_STATUS_REPLY, request_id, &message);
    if (rc == 0) rc = os_vfs_parse_backend_status_reply(&message, &status, &rights, request_id);

    if (rc != 0 || status != 0) { print_error("vfs-backend-status: capacite absente ou refusee"); ctx->last_rc = rc != 0 ? rc : status; return; }
    ctx->last_rc = 0; print_string("vfs-backend-status ok rights ");
    if (rights == OS_VFS_BACKEND_RIGHT_READ) print_string("read");
    else if (rights == OS_VFS_BACKEND_RIGHT_MUTATE) print_string("mutate");
    else if (rights == OS_VFS_BACKEND_RIGHT_ALL) print_string("full");
    else { print_error("vfs-backend-status: masque invalide"); ctx->last_rc = OS_VFS_STATUS_INVALID; return; }
    print_string(" request "); print_int((int)request_id); print_string("\n");
}

static void cmd_vfs_backend_scope(shell_context_t* ctx, char args[][128], int arg_count) {
    os_ipc_payload_t request;
    os_ipc_message_t message;
    os_vfs_backend_scope_reply_t reply;
    int pid;
    int rc;
    int target_pid;
    uint32_t request_id;
    uint32_t emitted = 0U;
    if (arg_count != 1 || (target_pid = parse_int(args[0])) <= 0) {
        print_error("Usage: vfs-backend-scope <pid>");
        return;
    }
    pid = sys_service_lookup("vfs");
    if (pid <= 0) { print_error("vfs-backend-scope: service vfs indisponible"); ctx->last_rc = pid; return; }
    request_id = next_vfs_request_id();
    rc = os_vfs_make_backend_scope_request(&request, target_pid, request_id);
    if (rc == 0) rc = sys_ipc_send(pid, &request);
    if (rc != 0) { print_error("vfs-backend-scope: demande refusee"); ctx->last_rc = rc; return; }
    rc = wait_ipc_reply(pid, OS_IPC_VFS_BACKEND_SCOPE_REPLY, request_id, &message);
    if (rc == 0) rc = os_vfs_parse_backend_scope_reply(&message, &reply, request_id);
    if (rc != 0 || reply.status != 0) {
        print_error("vfs-backend-scope: capacite absente ou refusee");
        ctx->last_rc = rc != 0 ? rc : reply.status;
        return;
    }
    ctx->last_rc = 0;
    print_string("vfs-backend-scope ok rights ");
    if (reply.rights == OS_VFS_BACKEND_RIGHT_READ) print_string("read");
    else if (reply.rights == OS_VFS_BACKEND_RIGHT_MUTATE) print_string("mutate");
    else if (reply.rights == OS_VFS_BACKEND_RIGHT_ALL) print_string("full");
    else { print_error("vfs-backend-scope: masque droits invalide"); ctx->last_rc = OS_VFS_STATUS_INVALID; return; }
    print_string(" sources ");
    if (reply.sources & OS_SERVICE_BACKEND_SOURCE_INITRD) {
        print_string("initrd"); emitted = 1U;
    }
    if (reply.sources & OS_SERVICE_BACKEND_SOURCE_OVERLAY) {
        if (emitted) print_string(","); print_string("overlay"); emitted = 1U;
    }
    if (reply.sources & OS_SERVICE_BACKEND_SOURCE_FAT16) {
        if (emitted) print_string(","); print_string("fat16"); emitted = 1U;
    }
    if (reply.sources & OS_SERVICE_BACKEND_SOURCE_FAT32) {
        if (emitted) print_string(","); print_string("fat32");
    }
    print_string(" request "); print_int((int)request_id); print_string("\n");
}

static void cmd_vfs_backend_list(shell_context_t* ctx, char args[][128], int arg_count) {
    os_ipc_payload_t request; os_ipc_message_t message; os_vfs_backend_list_reply_t reply;
    int pid, rc; uint32_t request_id, i;
    if (arg_count != 0) { print_error("Usage: vfs-backend-list"); return; }
    pid = sys_service_lookup("vfs");
    if (pid <= 0) { print_error("vfs-backend-list: service vfs indisponible"); ctx->last_rc = pid; return; }
    request_id = next_vfs_request_id(); rc = os_vfs_make_backend_list_request(&request, request_id);
    if (rc == 0) rc = sys_ipc_send(pid, &request);
    if (rc != 0) { print_error("vfs-backend-list: demande refusee"); ctx->last_rc = rc; return; }
    rc = wait_ipc_reply(pid, OS_IPC_VFS_BACKEND_LIST_REPLY, request_id, &message);
    if (rc == 0) rc = os_vfs_parse_backend_list_reply(&message, &reply, request_id);

    if (rc != 0 || reply.status != 0) { print_error("vfs-backend-list: consultation refusee"); ctx->last_rc = rc != 0 ? rc : reply.status; return; }
    ctx->last_rc = 0; print_string("vfs-backend-list ok count "); print_int((int)reply.count); print_string(" request "); print_int((int)request_id); print_string("\n");
    for (i = 0U; i < reply.count; i++) {
        print_string("vfs-backend-list pid "); print_int(reply.entries[i].pid); print_string(" rights ");
        if (reply.entries[i].rights == OS_VFS_BACKEND_RIGHT_READ) print_string("read");
        else if (reply.entries[i].rights == OS_VFS_BACKEND_RIGHT_MUTATE) print_string("mutate");
        else if (reply.entries[i].rights == OS_VFS_BACKEND_RIGHT_ALL) print_string("full");
        else { print_error("vfs-backend-list: masque invalide"); ctx->last_rc = OS_VFS_STATUS_INVALID; return; }
        print_string("\n");
    }
}

static void cmd_vfs_backend_observe(shell_context_t* ctx, char args[][128], int arg_count) {
    os_ipc_payload_t request; os_ipc_message_t message; os_vfs_backend_observe_reply_t reply;
    int pid, rc, expected; uint32_t request_id;
    if (arg_count != 1 || (expected = parse_int(args[0])) < 0) { print_error("Usage: vfs-backend-observe <generation>"); return; }
    pid = sys_service_lookup("vfs");
    if (pid <= 0) { print_error("vfs-backend-observe: service vfs indisponible"); ctx->last_rc = pid; return; }
    request_id = next_vfs_request_id(); rc = os_vfs_make_backend_observe_request(&request, (uint32_t)expected, request_id);
    if (rc == 0) rc = sys_ipc_send(pid, &request);
    if (rc != 0) { print_error("vfs-backend-observe: demande refusee"); ctx->last_rc = rc; return; }
    rc = wait_ipc_reply(pid, OS_IPC_VFS_BACKEND_OBSERVE_REPLY, request_id, &message);
    if (rc == 0) rc = os_vfs_parse_backend_observe_reply(&message, &reply, request_id);

    if (rc != 0) { print_error("vfs-backend-observe: reponse invalide"); ctx->last_rc = rc; return; }
    if (reply.status == OS_SERVICE_STALE) { ctx->last_rc = reply.status; print_string("vfs-backend-observe stale generation "); print_int((int)reply.generation); print_string("\n"); return; }
    if (reply.status != 0) { print_error("vfs-backend-observe: consultation refusee"); ctx->last_rc = reply.status; return; }
    ctx->last_rc = 0; print_string("vfs-backend-observe ok generation "); print_int((int)reply.generation); print_string(" count "); print_int((int)reply.count); print_string("\n");
}

static void cmd_vfs_write(shell_context_t* ctx, char args[][128], int arg_count) {
    os_ipc_payload_t request;
    os_ipc_message_t message;
    os_vfs_write_reply_t reply;
    int pid;
    int rc;
    uint32_t request_id;
    uint32_t size = 0U;
    if (arg_count != 2) {
        print_error("Usage: vfs-write <chemin> <texte>");
        return;
    }
    while (args[1][size] != '\0') size++;
    if (size > OS_VFS_WRITE_MAX) {
        print_error("vfs-write: texte trop long");
        ctx->last_rc = OS_VFS_STATUS_INVALID;
        return;
    }
    pid = sys_service_lookup("vfs");
    if (pid <= 0) {
        print_error("vfs-write: service vfs indisponible");
        ctx->last_rc = pid;
        return;
    }
    request_id = next_vfs_request_id();
    rc = os_vfs_make_write_request(&request, args[0], (const uint8_t*)args[1],
                                   size, request_id);
    if (rc != 0) {
        print_error("vfs-write: chemin invalide ou trop long");
        ctx->last_rc = rc;
        return;
    }
    rc = sys_ipc_send(pid, &request);
    if (rc != 0) {
        print_error("vfs-write: service indisponible");
        ctx->last_rc = rc;
        return;
    }
    rc = wait_ipc_reply(pid, OS_IPC_VFS_WRITE_REPLY, request_id, &message);
    if (rc == 0) rc = os_vfs_parse_write_reply(&message, &reply, request_id);

    if (rc != 0) {
        print_error("vfs-write: reponse VFS absente ou invalide");
        ctx->last_rc = rc;
        return;
    }
    ctx->last_rc = reply.status;
    if (reply.status != OS_VFS_STATUS_OK) {
        if (reply.status == OS_VFS_STATUS_NOT_MOUNTED) {
            print_error("vfs-write: chemin hors montage ecriture");
        } else {
            print_error("vfs-write: ecriture refusee");
        }
        return;
    }
    print_string("vfs-write ok request ");
    print_int((int)request_id);
    print_string("\n");
}

static void cmd_vfs_remove(shell_context_t* ctx, char args[][128], int arg_count) {
    os_ipc_payload_t request;
    os_ipc_message_t message;
    os_vfs_remove_reply_t reply;
    int pid;
    int rc;
    uint32_t request_id;
    if (arg_count != 1) {
        print_error("Usage: vfs-remove <chemin>");
        return;
    }
    pid = sys_service_lookup("vfs");
    if (pid <= 0) {
        print_error("vfs-remove: service vfs indisponible");
        ctx->last_rc = pid;
        return;
    }
    request_id = next_vfs_request_id();
    rc = os_vfs_make_remove_request(&request, args[0], request_id);
    if (rc != 0) {
        print_error("vfs-remove: chemin invalide ou trop long");
        ctx->last_rc = rc;
        return;
    }
    rc = sys_ipc_send(pid, &request);
    if (rc != 0) {
        print_error("vfs-remove: service indisponible");
        ctx->last_rc = rc;
        return;
    }
    rc = wait_ipc_reply(pid, OS_IPC_VFS_REMOVE_REPLY, request_id, &message);
    if (rc == 0) rc = os_vfs_parse_remove_reply(&message, &reply, request_id);

    if (rc != 0) {
        print_error("vfs-remove: reponse VFS absente ou invalide");
        ctx->last_rc = rc;
        return;
    }
    ctx->last_rc = reply.status;
    if (reply.status != OS_VFS_STATUS_OK) {
        if (reply.status == OS_VFS_STATUS_NOT_MOUNTED) {
            print_error("vfs-remove: chemin hors montage ecriture");
        } else {
            print_error("vfs-remove: suppression refusee ou fichier absent");
        }
        return;
    }
    print_string("vfs-remove ok request ");
    print_int((int)request_id);
    print_string("\n");
}

static void cmd_vfs_rename(shell_context_t* ctx, char args[][128], int arg_count) {
    os_ipc_payload_t request;
    os_ipc_message_t message;
    os_vfs_rename_reply_t reply;
    int pid;
    int rc;
    uint32_t request_id;
    if (arg_count != 2) {
        print_error("Usage: vfs-rename <src> <dst>");
        return;
    }
    pid = sys_service_lookup("vfs");
    if (pid <= 0) {
        print_error("vfs-rename: service vfs indisponible");
        ctx->last_rc = pid;
        return;
    }
    request_id = next_vfs_request_id();
    rc = os_vfs_make_rename_request(&request, args[0], args[1], request_id);
    if (rc != 0) {
        print_error("vfs-rename: chemin invalide ou trop long");
        ctx->last_rc = rc;
        return;
    }
    rc = sys_ipc_send(pid, &request);
    if (rc != 0) {
        print_error("vfs-rename: service indisponible");
        ctx->last_rc = rc;
        return;
    }
    rc = wait_ipc_reply(pid, OS_IPC_VFS_RENAME_REPLY, request_id, &message);
    if (rc == 0) rc = os_vfs_parse_rename_reply(&message, &reply, request_id);

    if (rc != 0) {
        print_error("vfs-rename: reponse VFS absente ou invalide");
        ctx->last_rc = rc;
        return;
    }
    ctx->last_rc = reply.status;
    if (reply.status != OS_VFS_STATUS_OK) {
        if (reply.status == OS_VFS_STATUS_NOT_MOUNTED) {
            print_error("vfs-rename: chemins hors montage ecriture");
        } else {
            print_error("vfs-rename: renommage refuse ou fichier absent");
        }
        return;
    }
    print_string("vfs-rename ok request ");
    print_int((int)request_id);
    print_string("\n");
}

static int wait_vfs_mount_reply(int expected_sender, uint32_t type, uint32_t request_id,
                                os_vfs_mount_reply_t* reply) {
    os_ipc_message_t message;
    int rc = wait_ipc_reply(expected_sender, type, request_id, &message);
    if (rc != 0) return rc;
    return os_vfs_parse_mount_reply(&message, type, reply, request_id);
}

static void cmd_vfs_mount_add(shell_context_t* ctx, char args[][128], int arg_count) {
    os_ipc_payload_t request;
    os_vfs_mount_reply_t reply;
    uint32_t source;
    uint32_t request_id;
    int pid;
    int rc;
    if (arg_count != 2) {
        print_error("Usage: vfs-mount-add <prefixe/> <initrd|overlay|fat16|fat32>");
        return;
    }
    if (strcmp(args[1], "initrd") == 0) source = OS_VFS_MOUNT_SOURCE_INITRD;
    else if (strcmp(args[1], "overlay") == 0) source = OS_VFS_MOUNT_SOURCE_OVERLAY;
    else if (strcmp(args[1], "fat16") == 0) source = OS_VFS_MOUNT_SOURCE_FAT16;
    else if (strcmp(args[1], "fat32") == 0) source = OS_VFS_MOUNT_SOURCE_FAT32;
    else {
        print_error("vfs-mount-add: source invalide");
        ctx->last_rc = OS_VFS_STATUS_INVALID;
        return;
    }
    pid = sys_service_lookup("vfs");
    if (pid <= 0) {
        print_error("vfs-mount-add: service vfs indisponible");
        ctx->last_rc = pid;
        return;
    }
    request_id = next_vfs_request_id();
    rc = os_vfs_make_mount_add_request(&request, args[0], source, request_id);
    if (rc == 0) rc = sys_ipc_send(pid, &request);
    if (rc != 0) {
        print_error("vfs-mount-add: prefixe invalide ou service indisponible");
        ctx->last_rc = rc;
        return;
    }
    rc = wait_vfs_mount_reply(pid, OS_IPC_VFS_MOUNT_ADD_REPLY, request_id, &reply);
    if (rc != 0) {
        print_error("vfs-mount-add: reponse VFS absente ou invalide");
        ctx->last_rc = rc;
        return;
    }
    ctx->last_rc = reply.status;
    if (reply.status != OS_VFS_STATUS_OK) {
        if (reply.status == OS_VFS_STATUS_MOUNT_FULL) print_error("vfs-mount-add: table de montages pleine");
        else if (reply.status == OS_VFS_STATUS_MOUNT_EXISTS) print_error("vfs-mount-add: montage deja present");
        else print_error("vfs-mount-add: montage refuse");
        return;
    }
    print_string("vfs-mount-add ok request ");
    print_int((int)request_id);
    print_string("\n");
}

static void cmd_vfs_mount_remove(shell_context_t* ctx, char args[][128], int arg_count) {
    os_ipc_payload_t request;
    os_vfs_mount_reply_t reply;
    uint32_t request_id;
    int pid;
    int rc;
    if (arg_count != 1) {
        print_error("Usage: vfs-mount-remove <prefixe/>");
        return;
    }
    pid = sys_service_lookup("vfs");
    if (pid <= 0) {
        print_error("vfs-mount-remove: service vfs indisponible");
        ctx->last_rc = pid;
        return;
    }
    request_id = next_vfs_request_id();
    rc = os_vfs_make_mount_remove_request(&request, args[0], request_id);
    if (rc == 0) rc = sys_ipc_send(pid, &request);
    if (rc != 0) {
        print_error("vfs-mount-remove: prefixe invalide ou service indisponible");
        ctx->last_rc = rc;
        return;
    }
    rc = wait_vfs_mount_reply(pid, OS_IPC_VFS_MOUNT_REMOVE_REPLY, request_id, &reply);
    if (rc != 0) {
        print_error("vfs-mount-remove: reponse VFS absente ou invalide");
        ctx->last_rc = rc;
        return;
    }
    ctx->last_rc = reply.status;
    if (reply.status != OS_VFS_STATUS_OK) {
        if (reply.status == OS_VFS_STATUS_NOT_MOUNTED) print_error("vfs-mount-remove: montage absent");
        else print_error("vfs-mount-remove: montage protege ou refuse");
        return;
    }
    print_string("vfs-mount-remove ok request ");
    print_int((int)request_id);
    print_string("\n");
}

static void cmd_vfs_read(shell_context_t* ctx, char args[][128], int arg_count) {
    os_ipc_payload_t request;
    os_ipc_message_t message;
    os_vfs_read_reply_t reply;
    int pid;
    int rc;
    uint32_t request_id;
    uint32_t i;
    if (arg_count != 1) {
        print_error("Usage: vfs-read <chemin>");
        return;
    }
    pid = sys_service_lookup("vfs");
    if (pid <= 0) {
        print_error("vfs-read: service vfs indisponible");
        ctx->last_rc = pid;
        return;
    }
    request_id = next_vfs_request_id();
    rc = os_vfs_make_read_request(&request, args[0], request_id);
    if (rc != 0) {
        print_error("vfs-read: chemin invalide ou trop long");
        ctx->last_rc = rc;
        return;
    }
    rc = sys_ipc_send(pid, &request);
    if (rc != 0) {
        print_error("vfs-read: service indisponible");
        ctx->last_rc = rc;
        return;
    }
    rc = wait_vfs_read_reply(pid, request_id, &message);
    if (rc == 0) rc = os_vfs_parse_read_reply(&message, &reply, request_id);
    if (rc != 0) {
        print_error("vfs-read: reponse VFS absente ou invalide");
        ctx->last_rc = rc;
        return;
    }
    ctx->last_rc = reply.status;
    if (reply.status != OS_VFS_STATUS_OK) {
        if (reply.status == OS_VFS_STATUS_NOT_MOUNTED) {
            print_error("vfs-read: chemin hors montage");
        } else {
            print_error("vfs-read: lecture refusee ou fichier absent");
        }
        return;
    }
    print_string("vfs-read ok ");
    print_int((int)reply.size);
    print_string(" request ");
    print_int((int)request_id);
    print_string(" data ");
    for (i = 0U; i < reply.size; i++) putc((char)reply.data[i]);
    if (reply.size == 0U || reply.data[reply.size - 1U] != '\n') print_string("\n");
}

static void cmd_vfs_mkdir(shell_context_t* ctx, char args[][128], int arg_count) {
    os_ipc_payload_t request; os_ipc_message_t message;
    int pid, rc, status; uint32_t request_id;
    if (arg_count != 1) { print_error("Usage: vfs-mkdir <chemin>"); return; }
    pid = sys_service_lookup("vfs");
    if (pid <= 0) { print_error("vfs-mkdir: service vfs indisponible"); ctx->last_rc = pid; return; }
    request_id = next_vfs_request_id(); rc = os_vfs_make_mkdir_request(&request, args[0], request_id);
    if (rc != 0) { print_error("vfs-mkdir: chemin invalide ou trop long"); ctx->last_rc = rc; return; }
    rc = sys_ipc_send(pid, &request);
    if (rc != 0) { print_error("vfs-mkdir: service indisponible"); ctx->last_rc = rc; return; }
    rc = wait_ipc_reply(pid, OS_IPC_VFS_MKDIR_REPLY, request_id, &message);
    if (rc == 0) rc = os_vfs_parse_mkdir_reply(&message, &status, request_id);

    if (rc != 0) { print_error("vfs-mkdir: reponse VFS absente ou invalide"); ctx->last_rc = rc; return; }
    ctx->last_rc = status;
    if (status == OS_VFS_STATUS_OK) { print_string("vfs-mkdir ok request "); print_int((int)request_id); print_string("\n"); }
    else if (status == OS_VFS_STATUS_NOT_MOUNTED) print_error("vfs-mkdir: chemin hors montage overlay");
    else print_error("vfs-mkdir: creation refusee");
}

static void cmd_vfs_rmdir(shell_context_t* ctx, char args[][128], int arg_count) {
    os_ipc_payload_t request; os_ipc_message_t message;
    int pid, rc, status; uint32_t request_id;
    if (arg_count != 1) { print_error("Usage: vfs-rmdir <chemin>"); return; }
    pid = sys_service_lookup("vfs");
    if (pid <= 0) { print_error("vfs-rmdir: service vfs indisponible"); ctx->last_rc = pid; return; }
    request_id = next_vfs_request_id(); rc = os_vfs_make_rmdir_request(&request, args[0], request_id);
    if (rc != 0) { print_error("vfs-rmdir: chemin invalide ou trop long"); ctx->last_rc = rc; return; }
    rc = sys_ipc_send(pid, &request);
    if (rc != 0) { print_error("vfs-rmdir: service indisponible"); ctx->last_rc = rc; return; }
    rc = wait_ipc_reply(pid, OS_IPC_VFS_RMDIR_REPLY, request_id, &message);
    if (rc == 0) rc = os_vfs_parse_rmdir_reply(&message, &status, request_id);

    if (rc != 0) { print_error("vfs-rmdir: reponse VFS absente ou invalide"); ctx->last_rc = rc; return; }
    ctx->last_rc = status;
    if (status == OS_VFS_STATUS_OK) { print_string("vfs-rmdir ok request "); print_int((int)request_id); print_string("\n"); }
    else if (status == OS_VFS_STATUS_NOT_MOUNTED) print_error("vfs-rmdir: chemin hors montage overlay");
    else if (status == OS_VFS_STATUS_NOT_EMPTY) print_error("vfs-rmdir: repertoire non vide");
    else print_error("vfs-rmdir: suppression refusee ou repertoire absent");
}

static void cmd_vfs_list(shell_context_t* ctx, char args[][128], int arg_count) {
    os_ipc_payload_t request;
    os_ipc_message_t message;
    os_vfs_list_reply_t reply;
    int pid;
    int rc;
    uint32_t request_id;
    uint32_t i;
    if (arg_count != 1) {
        print_error("Usage: vfs-list <repertoire/>");
        return;
    }
    pid = sys_service_lookup("vfs");
    if (pid <= 0) {
        print_error("vfs-list: service vfs indisponible");
        ctx->last_rc = pid;
        return;
    }
    request_id = next_vfs_request_id();
    rc = os_vfs_make_list_request(&request, args[0], request_id);
    if (rc != 0) {
        print_error("vfs-list: repertoire invalide ou trop long");
        ctx->last_rc = rc;
        return;
    }
    rc = sys_ipc_send(pid, &request);
    if (rc != 0) {
        print_error("vfs-list: service indisponible");
        ctx->last_rc = rc;
        return;
    }
    rc = wait_ipc_reply(pid, OS_IPC_VFS_LIST_REPLY, request_id, &message);
    if (rc == 0) rc = os_vfs_parse_list_reply(&message, &reply, request_id);

    if (rc != 0) {
        print_error("vfs-list: reponse VFS absente ou invalide");
        ctx->last_rc = rc;
        return;
    }
    ctx->last_rc = reply.status;
    if (reply.status != OS_VFS_STATUS_OK && reply.status != OS_VFS_STATUS_TRUNCATED) {
        if (reply.status == OS_VFS_STATUS_NOT_MOUNTED) {
            print_error("vfs-list: repertoire hors montage");
        } else {
            print_error("vfs-list: listage refuse");
        }
        return;
    }
    print_string("vfs-list ");
    print_string(reply.status == OS_VFS_STATUS_TRUNCATED ? "partiel" : "ok");
    print_string(" count ");
    print_int((int)reply.count);
    print_string(" request ");
    print_int((int)request_id);
    print_string("\n");
    for (i = 0U; i < OS_VFS_LIST_DATA_MAX && reply.data[i] != 0U; i++) {
        putc((char)reply.data[i]);
    }
    if (i == 0U || reply.data[i - 1U] != '\n') print_string("\n");
}

static void cmd_vfs_list_page(shell_context_t* ctx, char args[][128], int arg_count) {
    os_ipc_payload_t request;
    os_ipc_message_t message;
    os_vfs_list_page_reply_t reply;
    int pid;
    int rc;
    int start;
    uint32_t request_id;
    uint32_t i;
    if (arg_count != 2 || (start = parse_int(args[1])) < 0) {
        print_error("Usage: vfs-list-page <repertoire/> <depart>");
        return;
    }
    pid = sys_service_lookup("vfs");
    if (pid <= 0) { print_error("vfs-list-page: service vfs indisponible"); ctx->last_rc = pid; return; }
    request_id = next_vfs_request_id();
    rc = os_vfs_make_list_page_request(&request, args[0], (uint32_t)start, request_id);
    if (rc != 0) { print_error("vfs-list-page: repertoire ou index invalide"); ctx->last_rc = rc; return; }
    rc = sys_ipc_send(pid, &request);
    if (rc != 0) { print_error("vfs-list-page: service indisponible"); ctx->last_rc = rc; return; }
    rc = wait_ipc_reply_turns(pid, OS_IPC_VFS_LIST_PAGE_REPLY, request_id, &message,
                              VFS_READ_REPLY_WAIT_TURNS);
    if (rc == 0) rc = os_vfs_parse_list_page_reply(&message, &reply, request_id);

    if (rc != 0) { print_error("vfs-list-page: reponse VFS absente ou invalide"); ctx->last_rc = rc; return; }
    ctx->last_rc = reply.status;
    if (reply.status != OS_VFS_STATUS_OK && reply.status != OS_VFS_STATUS_TRUNCATED) {
        print_error(reply.status == OS_VFS_STATUS_NOT_MOUNTED ?
                    "vfs-list-page: repertoire hors montage" : "vfs-list-page: listage refuse");
        return;
    }
    print_string("vfs-list-page ");
    print_string(reply.status == OS_VFS_STATUS_TRUNCATED ? "partiel" : "ok");
    print_string(" count "); print_int((int)reply.count);
    print_string(" next ");
    if (reply.next_start == OS_VFS_LIST_PAGE_END) print_string("end");
    else print_int((int)reply.next_start);
    print_string(" request "); print_int((int)request_id); print_string("\n");
    for (i = 0U; i < OS_VFS_LIST_PAGE_DATA_MAX && reply.data[i] != 0U; i++) putc((char)reply.data[i]);
    if (i == 0U || reply.data[i - 1U] != '\n') print_string("\n");
}

static void cmd_vfs_list_observe(shell_context_t* ctx, char args[][128], int arg_count) {
    os_ipc_payload_t request; os_ipc_message_t message; os_vfs_list_observe_reply_t reply;
    int pid, rc, start, generation; uint32_t request_id, i;
    if (arg_count != 3 || (start = parse_int(args[1])) < 0 || (generation = parse_int(args[2])) < 0) {
        print_error("Usage: vfs-list-observe <repertoire/> <depart> <generation>"); return;
    }
    pid = sys_service_lookup("vfs");
    if (pid <= 0) { print_error("vfs-list-observe: service vfs indisponible"); ctx->last_rc = pid; return; }
    request_id = next_vfs_request_id();
    rc = os_vfs_make_list_observe_request(&request, args[0], (uint32_t)start, (uint32_t)generation, request_id);
    if (rc != 0) { print_error("vfs-list-observe: argument invalide"); ctx->last_rc = rc; return; }
    rc = sys_ipc_send(pid, &request);
    if (rc != 0) { print_error("vfs-list-observe: service indisponible"); ctx->last_rc = rc; return; }
    rc = wait_ipc_reply_turns(pid, OS_IPC_VFS_LIST_OBSERVE_REPLY, request_id, &message,
                              VFS_READ_REPLY_WAIT_TURNS);
    if (rc == 0) rc = os_vfs_parse_list_observe_reply(&message, &reply, request_id);

    if (rc != 0) { print_error("vfs-list-observe: reponse VFS absente ou invalide"); ctx->last_rc = rc; return; }
    ctx->last_rc = reply.status;
    if (reply.status == OS_VFS_STATUS_STALE) {
        print_string("vfs-list-observe obsolete generation "); print_int((int)reply.generation); print_string("\n"); return;
    }
    if (reply.status != OS_VFS_STATUS_OK && reply.status != OS_VFS_STATUS_TRUNCATED) {
        print_error("vfs-list-observe: listage refuse"); return;
    }
    print_string("vfs-list-observe "); print_string(reply.status == OS_VFS_STATUS_TRUNCATED ? "partiel" : "ok");
    print_string(" count "); print_int((int)reply.count); print_string(" next ");
    if (reply.next_start == OS_VFS_LIST_PAGE_END) print_string("end"); else print_int((int)reply.next_start);
    print_string(" generation "); print_int((int)reply.generation); print_string(" request "); print_int((int)request_id); print_string("\n");
    for (i = 0U; i < OS_VFS_LIST_OBSERVE_DATA_MAX && reply.data[i] != 0U; i++) putc((char)reply.data[i]);
    if (i == 0U || reply.data[i - 1U] != '\n') print_string("\n");
}

static void cmd_vfs_stat(shell_context_t* ctx, char args[][128], int arg_count) {
    os_ipc_payload_t request;
    os_ipc_message_t message;
    os_vfs_stat_reply_t reply;
    int pid;
    int rc;
    uint32_t request_id;
    if (arg_count != 1) {
        print_error("Usage: vfs-stat <chemin>");
        return;
    }
    pid = sys_service_lookup("vfs");
    if (pid <= 0) {
        print_error("vfs-stat: service vfs indisponible");
        ctx->last_rc = pid;
        return;
    }
    request_id = next_vfs_request_id();
    rc = os_vfs_make_stat_request(&request, args[0], request_id);
    if (rc != 0) {
        print_error("vfs-stat: chemin invalide ou trop long");
        ctx->last_rc = rc;
        return;
    }
    rc = sys_ipc_send(pid, &request);
    if (rc != 0) {
        print_error("vfs-stat: service indisponible");
        ctx->last_rc = rc;
        return;
    }
    rc = wait_ipc_reply(pid, OS_IPC_VFS_STAT_REPLY, request_id, &message);
    if (rc == 0) rc = os_vfs_parse_stat_reply(&message, &reply, request_id);

    if (rc != 0) {
        print_error("vfs-stat: reponse VFS absente ou invalide");
        ctx->last_rc = rc;
        return;
    }
    ctx->last_rc = reply.status;
    if (reply.status != OS_VFS_STATUS_OK) {
        if (reply.status == OS_VFS_STATUS_NOT_MOUNTED) {
            print_error("vfs-stat: chemin hors montage");
        } else {
            print_error("vfs-stat: metadonnees refusees ou fichier absent");
        }
        return;
    }
    print_string("vfs-stat ok size ");
    print_int((int)reply.size);
    print_string(" flags ");
    print_string(reply.flags == OS_DIRENT_DIR ? "dir" : "file");
    print_string(" request ");
    print_int((int)request_id);
    print_string("\n");
}

static void cmd_vfs_stats(shell_context_t* ctx, char args[][128], int arg_count) {
    char stats_args[1][128] = { "vfs-stats" };
    (void)args;
    if (arg_count != 0) {
        print_error("Usage: vfs-stats");
        return;
    }
    cmd_vfs_read(ctx, stats_args, 1);
}

static void cmd_yield(shell_context_t* ctx, char args[][128], int arg_count) {
    (void)args;
    (void)arg_count;
    yield();
    print_string("yield ok\n");
    ctx->last_rc = 0;
}

static void cmd_jobs(shell_context_t* ctx, char args[][128], int arg_count) {
    os_proc_t procs[16];
    int n;
    int shown = 0;
    (void)ctx; (void)args; (void)arg_count;
    n = sys_ps(procs, 16);
    print_colored("\n=== Jobs ===\n", COLOR_CYAN);
    for (int i = 0; i < n; i++) {
        if (procs[i].type != OS_TASK_USER) continue;
        print_string("[");
        print_int(procs[i].pid);
        print_string("]  ");
        print_string(proc_state_str(procs[i].state));
        print_string("  ");
        print_string(procs[i].name);
        print_string("\n");
        shown++;
    }
    if (shown == 0) print_string("Aucun job utilisateur.\n");
    print_string("jobs ok ");
    print_int(shown);
    print_string("\n");
}

static void cmd_top(shell_context_t* ctx, char args[][128], int arg_count) {
    os_proc_t procs[16];
    int n;
    (void)args; (void)arg_count;
    n = sys_ps(procs, 16);
    print_colored("\n=== top (noyau) ===\n", COLOR_CYAN);
    print_string("ticks: ");
    print_int((int)sys_ticks());
    print_string("  pid: ");
    print_int(sys_getpid());
    print_string("  tasks: ");
    print_int(n < 0 ? 0 : n);
    print_string("\n");
    print_colored("  PID  STAT  TYPE  COMMAND\n", COLOR_YELLOW);
    for (int i = 0; i < n; i++) {
        print_string("  ");
        print_int(procs[i].pid);
        print_string("    ");
        print_string(proc_state_str(procs[i].state));
        print_string("     ");
        print_string(procs[i].type == OS_TASK_USER ? "user  " : "kern  ");
        print_string(procs[i].name);
        print_string("\n");
    }
    print_string("top ok ");
    print_int(n < 0 ? 0 : n);
    print_string("\n");
}

static void cmd_getpid(shell_context_t* ctx, char args[][128], int arg_count) {
    (void)ctx; (void)args; (void)arg_count;
    print_string("getpid ok ");
    print_int(sys_getpid());
    print_string("\n");
}

static void cmd_uptime(shell_context_t* ctx, char args[][128], int arg_count) {
    unsigned int ticks = sys_ticks();
    int sec = (int)(ticks / 100);
    int h, m, s;
    (void)ctx; (void)args; (void)arg_count;
    if (sec < 0) sec = 0;
    h = sec / 3600;
    m = (sec % 3600) / 60;
    s = sec % 60;
    print_string("up ");
    print_int(h);
    print_string(":");
    if (m < 10) putc('0');
    print_int(m);
    print_string(":");
    if (s < 10) putc('0');
    print_int(s);
    print_string("  (PIT ticks: ");
    print_int((int)ticks);
    print_string(")\n");
}

static void cmd_date(shell_context_t* ctx, char args[][128], int arg_count) {
    unsigned int ticks = sys_ticks();
    int sec = (int)((ticks / 100) % 86400);
    int h = 5 + (sec / 3600);
    int m = (sec % 3600) / 60;
    int s = sec % 60;
    (void)ctx; (void)args; (void)arg_count;
    if (h >= 24) h %= 24;
    print_string("Thu Aug 13 ");
    if (h < 10) putc('0');
    print_int(h);
    print_string(":");
    if (m < 10) putc('0');
    print_int(m);
    print_string(":");
    if (s < 10) putc('0');
    print_int(s);
    print_string(" UTC 2026\n");
    print_string("date ok\n");
}

static void cmd_whoami(shell_context_t* ctx, char args[][128], int arg_count) {
    const char* user = get_env_var(ctx, "USER");
    (void)args; (void)arg_count;
    print_string("whoami ok ");
    print_string(user ? user : "root");
    print_string("\n");
}

static void cmd_alias(shell_context_t* ctx, char args[][128], int arg_count) {
    if (arg_count == 0) {
        if (ctx->alias_count == 0) {
            print_string("Aucun alias.\n");
            return;
        }
        for (int i = 0; i < ctx->alias_count; i++) {
            print_string("alias ");
            print_string(ctx->aliases[i].alias);
            print_string("='");
            print_string(ctx->aliases[i].command);
            print_string("'\n");
        }
        return;
    }
    {
        char name[64];
        char value[256];
        char* eq = find_char(args[0], '=');
        name[0] = 0;
        value[0] = 0;
        if (eq) {
            int nlen = (int)(eq - args[0]);
            int i;
            if (nlen <= 0 || nlen >= 63) {
                print_error("alias: nom invalide");
                return;
            }
            for (i = 0; i < nlen; i++) name[i] = args[0][i];
            name[nlen] = 0;
            strcpy(value, eq + 1);
            for (int a = 1; a < arg_count; a++) {
                strcat(value, " ");
                strcat(value, args[a]);
            }
        } else if (arg_count >= 2) {
            strcpy(name, args[0]);
            strcpy(value, args[1]);
            for (int a = 2; a < arg_count; a++) {
                strcat(value, " ");
                strcat(value, args[a]);
            }
        } else {
            print_error("alias: usage alias nom=commande");
            return;
        }
        for (int i = 0; i < ctx->alias_count; i++) {
            if (strcmp(ctx->aliases[i].alias, name) == 0) {
                strcpy(ctx->aliases[i].command, value);
                print_string("alias ok ");
                print_string(name);
                print_string("\n");
                return;
            }
        }
        if (ctx->alias_count >= MAX_ENV_VARS) {
            print_error("alias: table pleine");
            return;
        }
        strcpy(ctx->aliases[ctx->alias_count].alias, name);
        strcpy(ctx->aliases[ctx->alias_count].command, value);
        ctx->alias_count++;
        print_string("alias ok ");
        print_string(name);
        print_string("\n");
    }
}

static void cmd_unalias(shell_context_t* ctx, char args[][128], int arg_count) {
    if (arg_count == 0) {
        print_error("unalias: nom manquant");
        return;
    }
    for (int i = 0; i < ctx->alias_count; i++) {
        if (strcmp(ctx->aliases[i].alias, args[0]) == 0) {
            for (int j = i; j < ctx->alias_count - 1; j++) {
                ctx->aliases[j] = ctx->aliases[j + 1];
            }
            ctx->alias_count--;
            print_string("unalias ok ");
            print_string(args[0]);
            print_string("\n");
            return;
        }
    }
    print_error("unalias: alias introuvable");
}

static void cmd_export(shell_context_t* ctx, char args[][128], int arg_count) {
    if (arg_count == 0) {
        cmd_env(ctx, args, arg_count);
        return;
    }
    {
        char* eq = find_char(args[0], '=');
        if (eq) {
            char name[64];
            int nlen = (int)(eq - args[0]);
            int i;
            if (nlen <= 0 || nlen >= 63) {
                print_error("export: nom invalide");
                return;
            }
            for (i = 0; i < nlen; i++) name[i] = args[0][i];
            name[nlen] = 0;
            set_env_var(ctx, name, eq + 1);
            print_string("export ok ");
            print_string(name);
            print_string("\n");
        } else if (arg_count >= 2) {
            set_env_var(ctx, args[0], args[1]);
            print_string("export ok ");
            print_string(args[0]);
            print_string("\n");
        } else {
            print_error("export: usage export VAR=valeur");
        }
    }
}

static int load_file_lines(shell_context_t* ctx, const char* filearg,
                           char lines[][128], int max_lines) {
    char path[RAMFS_PATH_MAX];
    char kbuf[1024];
    const char* data;
    int size = 0;
    int pos = 0;
    int n = 0;
    int kn;
    resolve_arg(ctx, filearg, path);
    kn = sys_readfile(path, kbuf, (int)sizeof(kbuf));
    if (kn >= 0) {
        data = kbuf;
        size = kn;
    } else {
        data = ramfs_read(path, &size);
        if (!data) return -1;
    }
    while (pos < size && n < max_lines) {
        int len = 0;
        while (pos < size && data[pos] != '\n' && len < 127) {
            lines[n][len++] = data[pos++];
        }
        lines[n][len] = 0;
        if (pos < size && data[pos] == '\n') pos++;
        n++;
    }
    return n;
}

static void cmd_grep(shell_context_t* ctx, char args[][128], int arg_count) {
    char lines[32][128];
    int n;
    int hits = 0;
    if (arg_count < 2) {
        print_error("grep: usage grep <motif> <fichier>");
        return;
    }
    n = load_file_lines(ctx, args[1], lines, 32);
    if (n < 0) {
        print_error("grep: fichier introuvable");
        return;
    }
    for (int i = 0; i < n; i++) {
        if (strstr(lines[i], args[0])) {
            print_string(lines[i]);
            print_string("\n");
            hits++;
        }
    }
    if (hits == 0) {
        print_string("(aucune correspondance)\n");
        return;
    }
    print_string("grep hits ");
    print_int(hits);
    print_string("\n");
}

static void cmd_wc(shell_context_t* ctx, char args[][128], int arg_count) {
    char path[RAMFS_PATH_MAX];
    char kbuf[1024];
    const char* data;
    int size = 0;
    int lines = 0, words = 0, chars = 0;
    int in_word = 0;
    int kn;
    if (arg_count == 0) {
        print_error("wc: fichier manquant");
        return;
    }
    resolve_arg(ctx, args[0], path);
    kn = sys_readfile(path, kbuf, (int)sizeof(kbuf));
    if (kn >= 0) {
        data = kbuf;
        size = kn;
    } else {
        data = ramfs_read(path, &size);
        if (!data) {
            print_error("wc: fichier introuvable");
            return;
        }
    }
    chars = size;
    for (int i = 0; i < size; i++) {
        char c = data[i];
        if (c == '\n') lines++;
        if (c == ' ' || c == '\t' || c == '\n') in_word = 0;
        else if (!in_word) {
            in_word = 1;
            words++;
        }
    }
    print_string("wc ok ");
    print_int(lines);
    print_string(" ");
    print_int(words);
    print_string(" ");
    print_int(chars);
    print_string(" ");
    print_string(args[0]);
    print_string("\n");
}

static void cmd_sort(shell_context_t* ctx, char args[][128], int arg_count) {
    char lines[32][128];
    int n;
    if (arg_count == 0) {
        print_error("sort: fichier manquant");
        return;
    }
    n = load_file_lines(ctx, args[0], lines, 32);
    if (n < 0) {
        print_error("sort: fichier introuvable");
        return;
    }
    for (int i = 0; i < n - 1; i++) {
        for (int j = 0; j < n - 1 - i; j++) {
            if (strcmp(lines[j], lines[j + 1]) > 0) {
                char tmp[128];
                strcpy(tmp, lines[j]);
                strcpy(lines[j], lines[j + 1]);
                strcpy(lines[j + 1], tmp);
            }
        }
    }
    for (int i = 0; i < n; i++) {
        print_string(lines[i]);
        print_string("\n");
    }
    print_string("sort ok ");
    print_int(n);
    print_string(" ");
    print_string(args[0]);
    print_string("\n");
}

static int parse_line_count(char args[][128], int arg_count, int* file_idx) {
    int n = 10;
    *file_idx = 0;
    if (arg_count >= 2 && strcmp(args[0], "-n") == 0) {
        n = parse_int(args[1]);
        *file_idx = 2;
    } else if (arg_count >= 1 && args[0][0] == '-' && args[0][1] >= '0' && args[0][1] <= '9') {
        n = parse_int(args[0] + 1);
        *file_idx = 1;
    }
    if (n < 0) n = 0;
    if (n > 32) n = 32;
    return n;
}

static void cmd_head(shell_context_t* ctx, char args[][128], int arg_count) {
    char lines[32][128];
    int file_idx = 0;
    int want;
    int n;
    if (arg_count == 0) {
        print_error("head: fichier manquant");
        return;
    }
    want = parse_line_count(args, arg_count, &file_idx);
    if (file_idx >= arg_count) {
        print_error("head: fichier manquant");
        return;
    }
    n = load_file_lines(ctx, args[file_idx], lines, 32);
    if (n < 0) {
        print_error("head: fichier introuvable");
        return;
    }
    if (want > n) want = n;
    for (int i = 0; i < want; i++) {
        print_string(lines[i]);
        print_string("\n");
    }
    print_string("head ok ");
    print_int(want);
    print_string(" ");
    print_string(args[file_idx]);
    print_string("\n");
}

static void cmd_tail(shell_context_t* ctx, char args[][128], int arg_count) {
    char lines[32][128];
    int file_idx = 0;
    int want;
    int n;
    int start;
    if (arg_count == 0) {
        print_error("tail: fichier manquant");
        return;
    }
    want = parse_line_count(args, arg_count, &file_idx);
    if (file_idx >= arg_count) {
        print_error("tail: fichier manquant");
        return;
    }
    n = load_file_lines(ctx, args[file_idx], lines, 32);
    if (n < 0) {
        print_error("tail: fichier introuvable");
        return;
    }
    if (want > n) want = n;
    start = n - want;
    for (int i = start; i < n; i++) {
        print_string(lines[i]);
        print_string("\n");
    }
    print_string("tail ok ");
    print_int(want);
    print_string(" ");
    print_string(args[file_idx]);
    print_string("\n");
}

static const char* ai_provider_name(const shell_context_t* ctx) {
    return ctx->ai_provider == AI_PROVIDER_OPENAI ? "openai" : "local";
}

static const char* ai_model_name(const shell_context_t* ctx) {
    return ctx->ai_model;
}

static const char* ai_session_phase_name(unsigned int status) {
    switch ((status >> 8) & 0xffU) {
        case 0U: return "IDLE";
        case 1U: return "SYN_SENT";
        case 2U: return "TLS_STARTED";
        case 3U: return "TLS_COMPLETE";
        case 4U: return "REQUEST_SENT";
        case 5U: return "RESPONSE_READY";
        case 6U: return "STREAMING";
        default: return "INCONNUE";
    }
}

static void cmd_ai_stats(shell_context_t* ctx, char args[][128], int arg_count) {
    (void)args; (void)arg_count;
    print_colored("\n=== Statistiques IA ===\n", COLOR_CYAN);
    print_string("Requêtes ai : ");
    print_int(ctx->ai_query_count);
    print_string("\nMode IA     : ");
    print_string(ctx->ai_mode ? "active" : "desactive");
    print_string("\nFournisseur : ");
    print_string(ai_provider_name(ctx));
    print_string("\nModele      : ");
    print_string(ai_model_name(ctx));
    print_string("\nMoteur      : GPT-2 local bare-metal avec echantillonnage top-k\n");
    print_string("aistats ok ");
    print_int(ctx->ai_query_count);
    print_string("\n");
}

static void cmd_rc(shell_context_t* ctx) {
    print_string("rc ok ");
    print_int(ctx->last_rc);
    print_string("\n");
}

static void cmd_ai_provider(shell_context_t* ctx, char args[][128], int arg_count) {
    if (arg_count == 0) {
        print_string("Fournisseur IA : ");
        print_string(ai_provider_name(ctx));
        print_string("\nUsage: ai-provider [local|openai]\n");
        return;
    }
    if (strcmp(args[0], "local") == 0) {
        ctx->ai_provider = AI_PROVIDER_LOCAL;
        print_success("Fournisseur local selectionne");
        return;
    }
    if (strcmp(args[0], "openai") == 0) {
        ctx->ai_provider = AI_PROVIDER_OPENAI;
        print_warning("OpenAI selectionne : pilote reseau, DNS et TLS requis avant appel reel");
        return;
    }
    print_error("Usage: ai-provider [local|openai]");
}

static void cmd_ai_model(shell_context_t* ctx, char args[][128], int arg_count) {
    if (arg_count == 0 || strcmp(args[0], "status") == 0) {
        print_string("Modele local courant : ");
        print_string(ai_model_name(ctx));
        print_string("\nUsage: ai-model [list|use <modele.bin|modele.gguf>]\n");
        return;
    }
    if (strcmp(args[0], "list") == 0) {
        print_string("Modeles locaux declares :\n");
        print_string("  gpt2_124M.bin  [operationnel : checkpoint llm.c v3, CPU bare-metal]\n");
        print_string("  gpt2.gguf      [operationnel : GPT-2 GGUF FAT16, catalogue statique et cache KV]\n");
        return;
    }
    if (strcmp(args[0], "use") == 0 && arg_count == 2) {
        if (strstr(args[1], ".bin") == 0 && strstr(args[1], ".gguf") == 0) {
            print_error("Le profil local doit pointer vers un fichier .bin ou .gguf");
            return;
        }
        strcpy(ctx->ai_model, args[1]);
        if (strstr(args[1], "gpt2") != 0 && strstr(args[1], ".gguf") != 0) {
            print_success("Profil GPT-2 GGUF selectionne; modele FAT16 requis au boot");
        } else if (strstr(args[1], "gpt2") != 0) {
            print_success("Profil GPT-2 FP32 selectionne; validation par le chargeur au boot");
        } else {
            print_warning("Profil memorise; seuls les profils GPT-2 locaux sont executables");
        }
        return;
    }
    print_error("Usage: ai-model [list|use <modele.bin|modele.gguf>]");
}

static int ai_parse_port(const char* value, uint16_t* port) {
    uint32_t parsed = 0U;
    uint16_t index = 0U;
    if (!value || !port || value[0] == '\0') return -1;
    while (value[index] != '\0') {
        if (value[index] < '0' || value[index] > '9') return -1;
        parsed = parsed * 10U + (uint32_t)(value[index] - '0');
        if (parsed > 65535U) return -1;
        ++index;
    }
    if (parsed == 0U) return -1;
    *port = (uint16_t)parsed;
    return 0;
}

static void cmd_ai_acquire(shell_context_t* ctx, char args[][128], int arg_count) {
    os_llm_acquire_start_request_t request = {0};
    uint16_t index;
    int status;
    (void)ctx;
    if (arg_count != 1 && arg_count != 2) {
        print_error("Usage: ai-acquire <hostname> [port]");
        return;
    }
    for (index = 0U; index < OS_LLM_HOSTNAME_MAX; ++index) {
        char value = args[0][index];
        if (value == '\0') break;
        request.hostname[index] = value;
    }
    if (index == 0U || index == OS_LLM_HOSTNAME_MAX) {
        print_error("ai-acquire: hostname absent ou trop long");
        return;
    }
    request.hostname[index] = '\0';
    request.xid = 0xa0650001U;
    request.local_sequence = 1U;
    request.dns_id = 0xa665U;
    request.dhcp_attempts = OS_LLM_ACQUIRE_MAX_ATTEMPTS;
    request.dns_attempts = OS_LLM_ACQUIRE_MAX_ATTEMPTS;
    request.arp_attempts = OS_LLM_ACQUIRE_MAX_ATTEMPTS;
    request.local_port = 49152U;
    request.remote_port = 443U;
    if (arg_count == 2 && ai_parse_port(args[1], &request.remote_port) != 0) {
        print_error("ai-acquire: port invalide");
        return;
    }
    status = sys_llm_acquire_start(&request);
    if (status == 0) {
        print_success("ai-acquire: DHCP, DNS et SYN LLM demarres");
        return;
    }
    if (status == OS_LLM_ACQUIRE_BAD_REQUEST) print_error("ai-acquire: requete invalide");
    else if (status == OS_LLM_ACQUIRE_UNAVAILABLE) print_error("ai-acquire: NE2000 absent; aucun etat reseau publie");
    else if (status == OS_LLM_ACQUIRE_IN_PROGRESS) print_error("ai-acquire: session LLM deja active");
    else if (status == OS_LLM_ACQUIRE_TLS_ENTROPY) print_error("ai-acquire: entropie RDRAND TLS indisponible");
    else if (status == OS_LLM_ACQUIRE_DHCP_DISCOVER_FAILED) print_error("ai-acquire: emission DHCP DISCOVER refusee");
    else if (status == OS_LLM_ACQUIRE_DHCP_OFFER_TIMEOUT) print_error("ai-acquire: offre DHCP absente dans le budget borne");
    else if (status == OS_LLM_ACQUIRE_DHCP_REQUEST_FAILED) print_error("ai-acquire: emission DHCP REQUEST refusee");
    else if (status == OS_LLM_ACQUIRE_DHCP_ACK_TIMEOUT) print_error("ai-acquire: ACK DHCP absent dans le budget borne");
    else if (status == OS_LLM_ACQUIRE_DHCP_FAILED) print_error("ai-acquire: bail DHCP indisponible; contexte conserve");
    else if (status == OS_LLM_ACQUIRE_BOOTSTRAP_FAILED) print_error("ai-acquire: DNS, ARP ou SYN indisponible; contexte conserve");
    else print_error("ai-acquire: bootstrap reseau echoue; contexte conserve");
}

static int ai_copy_field(char* destination, uint16_t capacity, const char* source) {
    uint16_t index;
    if (!destination || !source || capacity == 0U) return -1;
    for (index = 0U; index < capacity; ++index) {
        destination[index] = source[index];
        if (source[index] == '\0') return 0;
    }
    destination[capacity - 1U] = '\0';
    return -1;
}

static void cmd_ai_credential(shell_context_t* ctx, char args[][128], int arg_count) {
    os_llm_openai_credential_request_t request = {0};
    uint16_t index;
    int status;
    (void)ctx;
    if (arg_count != 1 || ai_copy_field(request.bearer, OS_LLM_BEARER_MAX, args[0]) != 0) {
        print_error("Usage: ai-credential <bearer OpenAI borne>");
        return;
    }
    status = sys_llm_configure_openai(&request);
    for (index = 0U; index < OS_LLM_BEARER_MAX; ++index) request.bearer[index] = '\0';
    if (status == 0) {
        print_success("ai-credential: bearer OpenAI provisionne dans le noyau");
        return;
    }
    if (status == OS_LLM_CREDENTIAL_BAD_ARGUMENT) print_error("ai-credential: bearer invalide");
    else if (status == OS_LLM_CREDENTIAL_BAD_PHASE) print_error("ai-credential: session LLM active; fermez-la avant modification");
    else print_error("ai-credential: provisionnement refuse");
}
static void cmd_ai_request(shell_context_t* ctx, char args[][128], int arg_count, uint8_t streaming) {
    os_llm_request_t request = {0};
    uint16_t index = 0U;
    int argument;
    int status;
    (void)ctx;
    if (arg_count < 4) {
        print_error("Usage: ai-request <ollama|openai> <modele> <chemin> <prompt>");
        return;
    }
    if (strcmp(args[0], "ollama") == 0) request.provider = AI_NETWORK_PROVIDER_OLLAMA;
    else if (strcmp(args[0], "openai") == 0) request.provider = AI_NETWORK_PROVIDER_OPENAI;
    else {
        print_error("ai-request: fournisseur attendu ollama ou openai");
        return;
    }
    request.streaming = streaming;
    if (ai_copy_field(request.model, OS_LLM_MODEL_MAX, args[1]) != 0 ||
        ai_copy_field(request.path, OS_LLM_PATH_MAX, args[2]) != 0) {
        print_error("ai-request: modele ou chemin trop long");
        return;
    }
    for (argument = 3; argument < arg_count; ++argument) {
        uint16_t character = 0U;
        if (argument != 3) {
            if (index >= OS_LLM_PROMPT_MAX) {
                print_error("ai-request: prompt trop long");
                return;
            }
            request.prompt[index++] = ' ';
        }
        while (args[argument][character] != '\0') {
            if (index >= OS_LLM_PROMPT_MAX) {
                print_error("ai-request: prompt trop long");
                return;
            }
            request.prompt[index++] = (uint8_t)args[argument][character++];
        }
    }
    request.prompt_length = index;
    status = sys_llm_request(&request);
    if (status == 0) {
        print_success(streaming ? "ai-stream-request: POST SSE LLM chiffre emis" : "ai-request: POST LLM chiffre emis");
        return;
    }
    if (status == OS_LLM_REQUEST_BAD_PHASE) print_error("ai-request: TLS authentifie requis");
    else if (status == OS_LLM_REQUEST_UNCONFIGURED) print_error("ai-request: identifiant OpenAI noyau requis");
    else if (status == OS_LLM_REQUEST_BAD_REQUEST) print_error("ai-request: requete invalide");
    else print_error("ai-request: emission refusee; contexte conserve");
}

static void cmd_ai_text_poll(shell_context_t* ctx, char args[][128], int arg_count) {
    os_llm_text_result_t result = {0};
    char output[OS_LLM_TEXT_MAX + 1U];
    uint16_t index;
    int status;
    (void)ctx;
    if (arg_count != 0) {
        print_error("Usage: ai-text-poll");
        return;
    }
    status = sys_llm_poll_text(&result);
    if (status < 0) {
        if (status == OS_LLM_TEXT_BAD_PHASE) print_error("ai-text-poll: requete LLM non emise");
        else print_error("ai-text-poll: lecture refusee; contexte conserve");
        return;
    }
    if (status > 0) {
        print_warning("ai-text-poll: reponse LLM incomplete");
        return;
    }
    for (index = 0U; index < result.text_length && index < OS_LLM_TEXT_MAX; ++index)
        output[index] = (char)result.text[index];
    output[index] = '\0';
    print_string("LLM : ");
    print_string(output);
    print_string("\nHTTP : ");
    print_int(result.status_code);
    print_string("\n");
}

static void cmd_ai_next(shell_context_t* ctx, char args[][128], int arg_count) {
    int status;
    (void)ctx;
    if (arg_count != 0) {
        print_error("Usage: ai-next");
        return;
    }
    status = sys_llm_reset_for_request();
    if (status == 0) {
        print_success("ai-next: session LLM rearmement terminee");
        return;
    }
    if (status == OS_LLM_RESET_BAD_PHASE) print_error("ai-next: reponse LLM complete requise");
    else print_error("ai-next: rearmement refuse; contexte conserve");
}

static void cmd_ai_close(shell_context_t* ctx, char args[][128], int arg_count) {
    int status;
    (void)ctx;
    if (arg_count != 0) {
        print_error("Usage: ai-close");
        return;
    }
    status = sys_llm_close();
    if (status == 0) {
        print_success("ai-close: session et secrets effaces; bail DHCP conserve");
        return;
    }
    if (status == OS_LLM_CLOSE_FIN_FAILED) {
        print_warning("ai-close: FIN non emis; purge locale terminee et bail DHCP conserve");
        return;
    }
    if (status == OS_LLM_CLOSE_BAD_PHASE) print_error("ai-close: aucune session LLM active");
    else print_error("ai-close: annulation refusee");
}

static void cmd_ai_sse_poll(shell_context_t* ctx, char args[][128], int arg_count) {
    os_llm_text_result_t result = {0};
    char output[OS_LLM_TEXT_MAX + 1U];
    uint16_t index;
    int status;
    (void)ctx;
    if (arg_count != 0) {
        print_error("Usage: ai-sse-poll");
        return;
    }
    status = sys_llm_poll_sse(&result);
    if (status < 0) {
        if (status == OS_LLM_SSE_BAD_PHASE) print_error("ai-sse-poll: flux SSE non emis");
        else print_error("ai-sse-poll: lecture refusee; contexte conserve");
        return;
    }
    if (status > 0 && result.text_length == 0U) {
        print_warning("ai-sse-poll: attente de delta SSE");
        return;
    }
    if (result.text_length > 0U) {
        for (index = 0U; index < result.text_length && index < OS_LLM_TEXT_MAX; ++index)
            output[index] = (char)result.text[index];
        output[index] = '\0';
        print_string("SSE : ");
        print_string(output);
        print_string("\n");
    }
    if (status == 0) {
        print_success("ai-sse-poll: flux SSE termine");
    }
    print_string("HTTP : ");
    print_int(result.status_code);
    print_string("\n");
}

static void cmd_ai_tls_poll(shell_context_t* ctx, char args[][128], int arg_count) {
    int status;
    (void)ctx;
    if (arg_count != 0) {
        print_error("Usage: ai-tls-poll");
        return;
    }
    status = sys_llm_poll_tls();
    if (status == 0) {
        print_success("ai-tls-poll: progression TLS publiee");
        return;
    }
    if (status > 0) {
        print_warning("ai-tls-poll: attente de trame TLS");
        return;
    }
    if (status == OS_LLM_ACQUIRE_UNAVAILABLE) print_error("ai-tls-poll: NE2000 absent; aucun etat TLS publie");
    else if (status == OS_LLM_TLS_BAD_PHASE) print_error("ai-tls-poll: phase LLM non prete pour TLS");
    else if (status == OS_LLM_TLS_UNCONFIGURED) print_error("ai-tls-poll: entropie et ancre X.509 noyau requises");
    else print_error("ai-tls-poll: echec TLS; contexte conserve");
}

static void cmd_ai_runtime(shell_context_t* ctx, char args[][128], int arg_count) {
    unsigned int session_status = sys_llm_session_status();
    (void)args; (void)arg_count;
    print_colored("\n=== Runtime IA bare-metal ===\n", COLOR_CYAN);
    print_string("Architecture active : PC i386 Multiboot, CPU, 1 Gio RAM requis pour GPT-2\n");
    print_string("Fournisseur actif  : ");
    print_string(ai_provider_name(ctx));
    print_string("\nModele declare     : ");
    print_string(ai_model_name(ctx));
    print_string("\nLocal              : GPT-2 FP32 initrd ou GPT-2 GGUF FAT16, generation top-k\n");
    print_string("Limite locale      : 64 jetons de contexte, cache KV actif; GGUF lit les poids par fenetres\n");
    print_string("Session LLM noyau  : ");
    print_string(ai_session_phase_name(session_status));
    print_string((session_status & 1U) ? " (NE2000 pret)\n" : " (NE2000 absent)\n");
    print_string("Bail DHCP noyau    : ");
    print_string((session_status & 2U) ? "present (routes disponibles)\n" : "absent\n");
    print_string("Entropie TLS RDRAND : ");
    print_string((session_status & 4U) ? "disponible (materiel)\n" : "indisponible\n");
    print_string("Ancre X.509 noyau  : ");
    print_string((session_status & 8U) ? "ISRG Root X1 validee\n" : "indisponible\n");
    print_string("Ancre TLS locale   : ");
    print_string((session_status & 16U) ? "prete (example.com / example.test)\n" : "indisponible\n");
    print_string("En ligne           : controle de phase integre; DHCP/DNS/identifiants requis avant appel\n");
    print_string("Secrets OpenAI     : jamais integres a l'image de boot\n\n");
}

static void cmd_net_status(shell_context_t* ctx, char args[][128], int arg_count) {
    (void)ctx;
    if (arg_count > 0 && strcmp(args[0], "json") == 0) {
        unsigned int status = sys_net_status();
        print_string(status & 1U ? "{\"nic\":\"detected\",\"ethernet\":\"configured\",\"arp\":\"on-demand\",\"ipv4\":\"dhcp\",\"dns\":\"on-demand\",\"tcp\":\"socket\",\"tls\":\"authenticated\",\"openai\":\"credential-required\"}\n" : "{\"nic\":\"absent\",\"ethernet\":\"unavailable\",\"arp\":\"unavailable\",\"ipv4\":\"unavailable\",\"dhcp\":\"unavailable\",\"dns\":\"unavailable\",\"tcp\":\"unavailable\",\"tls\":\"unavailable\",\"openai\":\"unavailable\"}\n");
        return;
    }
    (void)args;
    print_colored("\n=== Reseau bare-metal ===\n", COLOR_CYAN);
    print_string(sys_net_status() & 1U ? "Carte Ethernet : detectee (NE2000 initialise)\n" : "Carte Ethernet : absente (aucun pilote NIC initialise)\n");
    print_string("Sockets TCP Ring 3: 4 slots disponibles (multisocket, TLS 1.2 / HTTP / SSE)\n");
    print_string("ARP / IPv4 / DHCP : acquisition et renouvellement caller-owned\n");
    print_string("DNS / TCP / TLS   : socket, X.509 et TLS authentifie disponibles par phase\n");
    print_string("OpenAI en ligne   : bearer, POST Chat Completions et SSE disponibles apres acquisition/TLS\n");
    print_string("net-status ok AOS-1521\n");
}

static void cmd_reboot(shell_context_t* ctx, char args[][128], int arg_count) {
    (void)ctx; (void)args; (void)arg_count;
    print_warning("reboot: simule (QEMU reste actif, tapez exit pour quitter le shell)");
}

static void cmd_shutdown(shell_context_t* ctx, char args[][128], int arg_count) {
    (void)ctx; (void)args; (void)arg_count;
    print_warning("shutdown: simule (QEMU reste actif, tapez exit pour quitter le shell)");
}

void cmd_exit(shell_context_t* ctx, char args[][128], int arg_count) {
    int exit_code = 0;
    
    if (arg_count > 0) {
        // Conversion simple string vers int
        int result = 0;
        char* str = args[0];
        for (int i = 0; str[i] != '\0'; i++) {
            if (str[i] >= '0' && str[i] <= '9') {
                result = result * 10 + (str[i] - '0');
            }
        }
        exit_code = result;
    }
    
    print_colored("\n[MOHHDY] Merci d'avoir utilisé MOHHDY v6.0 !\n", COLOR_CYAN);
    print_colored("   Au revoir et à bientôt !\n\n", COLOR_YELLOW);
    
    exit_program(exit_code);
}

// ==============================================================================
// INTÉGRATION IA AVANCÉE
// ==============================================================================

void call_ai_assistant(shell_context_t* ctx, const char* query) {
    if (ctx->ai_provider == AI_PROVIDER_OPENAI) {
        print_colored("[IA] OpenAI selectionne : utilisez ai-acquire, ai-tls-poll, puis ai-request ou ai-stream-request\n", COLOR_YELLOW);
        return;
    }
    print_colored("[IA] profil local : ", COLOR_CYAN);
    print_string(ai_model_name(ctx));
    print_string("\n");
    if (strstr(ai_model_name(ctx), "gpt2") != 0) {
        char generated[384];
        int use_gguf = strstr(ai_model_name(ctx), ".gguf") != 0;
        int generated_len = use_gguf
            ? sys_gpt2_gguf_generate(query, generated, sizeof(generated))
            : sys_gpt2_generate(query, generated, sizeof(generated));
        if (generated_len >= 0) {
            print_colored(use_gguf ? "[GPT-2 GGUF local] " : "[GPT-2 local] ", COLOR_GREEN);
            if (generated_len == 0) print_string("(fin de sequence)");
            else print_string(generated);
            print_string("\n");
            return;
        }
        print_colored(use_gguf ? "[GPT-2 GGUF local] indisponible (code " : "[GPT-2 local] indisponible (code ", COLOR_YELLOW);
        print_int(generated_len);
        print_colored("); repli de compatibilite.\n", COLOR_YELLOW);
    }
    // Lancer le binaire local de compatibilite en tache bloquante pour garantir l'affichage
    char* argv[3];
    argv[0] = "ai_assistant";
    argv[1] = (char*)query;
    argv[2] = 0;
    // Essayer d'abord dans bin/ en mode bloquant pour garantir l'affichage
    int rc = exec("bin/ai_assistant", argv);
    if (rc != 0) {
        rc = exec("ai_assistant", argv);
    }
    if (rc != 0) {
        print_colored("\n[IA] indisponible\n", COLOR_YELLOW);
        return;
    }
    print_string("ai ok\n");
}

void cmd_ai(shell_context_t* ctx, char args[][128], int arg_count) {
    if (arg_count == 0) {
        print_error("Usage: ai <votre question>");
        return;
    }
    
    // Reconstituer la question complète
    char full_query[MAX_COMMAND_LENGTH] = "";
    for (int i = 0; i < arg_count; i++) {
        strcat(full_query, args[i]);
        if (i < arg_count - 1) strcat(full_query, " ");
    }
    // Echo immediate for visibility
    print_colored("[IA] ", COLOR_MAGENTA);
    print_string("question: ");
    print_string(full_query);
    print_string("\n");
    ctx->ai_query_count++;
    call_ai_assistant(ctx, full_query);
}

void cmd_ai_continue(shell_context_t* ctx, char args[][128], int arg_count) {
    char generated[384];
    int generated_len;
    (void)args;
    if (arg_count != 0) {
        print_error("Usage: ai-continue");
        return;
    }
    if (ctx->ai_provider != AI_PROVIDER_LOCAL || strstr(ai_model_name(ctx), ".gguf") == 0) {
        print_error("ai-continue: selectionnez d'abord ai-model use gpt2.gguf");
        return;
    }
    generated_len = sys_gpt2_gguf_continue(generated, sizeof(generated));
    if (generated_len < 0) {
        print_colored("[GPT-2 GGUF local] session indisponible (lancez d'abord ai <question>) code ", COLOR_YELLOW);
        print_int(generated_len);
        print_string("\n");
        return;
    }
    print_colored("[GPT-2 GGUF local suite] ", COLOR_GREEN);
    if (generated_len == 0) print_string("(fin de sequence)");
    else print_string(generated);
    print_string("\n");
}

void cmd_ai_mode(shell_context_t* ctx, char args[][128], int arg_count) {
    if (arg_count == 0) {
        print_string("aimode ok ");
        print_string(ctx->ai_mode ? "on" : "off");
        print_string("\n");
        return;
    }
    
    if (strcmp(args[0], "on") == 0) {
        ctx->ai_mode = 1;
        print_string("aimode ok on\n");
    } else if (strcmp(args[0], "off") == 0) {
        ctx->ai_mode = 0;
        print_string("aimode ok off\n");
    } else {
        print_error("Usage: ai-mode [on|off]");
    }
}

void cmd_ai_help(shell_context_t* ctx, char args[][128], int arg_count) {
    print_colored("\n=== Guide d'Utilisation de l'IA ===\n", COLOR_CYAN);
    
    print_colored("[IA] FONCTIONNALITES IA :\n", COLOR_MAGENTA);
    print_string("  - Reponses contextuelles intelligentes\n");
    print_string("  - Aide technique et suggestions\n");
    print_string("  - Analyse de commandes et diagnostics\n");
    print_string("  - Assistant personnel intégré\n\n");
    
    print_colored("[exemples] EXEMPLES DE QUESTIONS :\n", COLOR_YELLOW);
    print_string("  ai comment optimiser la mémoire ?\n");
    print_string("  ai explique-moi le multitâche\n");
    print_string("  ai que fait cette commande : ls -la\n");
    print_string("  ai résoudre erreur de compilation\n");
    print_string("  ai créer un script automatique\n\n");
    
    print_colored("[modes] MODES D'UTILISATION :\n", COLOR_YELLOW);
    print_string("  1. Mode explicite : ai <question>\n");
    print_string("  2. Mode automatique : question directe (si activé)\n");
    print_string("  3. Mode intégré : aide contextuelle dans les commandes\n\n");
    
    print_colored("[conseils] CONSEILS :\n", COLOR_GREEN);
    print_string("  - Soyez précis dans vos questions\n");
    print_string("  - Mentionnez le contexte si nécessaire\n");
    print_string("  - L'IA apprend de vos interactions\n\n");
    print_string("aihelp ok\n");
}

// Test IA: lance l'IA avec une requete de sante et verifie le code retour
static void cmd_ai_test(shell_context_t* ctx) {
    char* argv[3];
    int rc;
    argv[0] = "ai_assistant";
    argv[1] = "healthcheck";
    argv[2] = 0;
    rc = exec("bin/ai_assistant", argv);
    if (rc != 0) rc = exec("ai_assistant", argv);
    if (rc != 0) {
        print_string("aitest fail\n");
        ctx->last_rc = 1;
        return;
    }
    print_string("aitest ok\n");
    ctx->last_rc = 0;
}

// ==============================================================================
// GESTIONNAIRE DE COMMANDES PRINCIPAL
// ==============================================================================

int execute_builtin_command(shell_context_t* ctx, const char* command, 
                           char args[][128], int arg_count) {
    if (strcmp(command, "help") == 0) {
        cmd_help(ctx, args, arg_count);
        return 1;
    } else if (strcmp(command, "ls") == 0 || strcmp(command, "dir") == 0) {
        cmd_ls(ctx, args, arg_count);
        return 1;
    } else if (strcmp(command, "ps") == 0) {
        cmd_ps(ctx, args, arg_count);
        return 1;
    } else if (strcmp(command, "task-metrics") == 0) {
        cmd_task_metrics(ctx, args, arg_count);
        return 1;
    } else if (strcmp(command, "task-priority") == 0) {
        cmd_task_priority(ctx, args, arg_count);
        return 1;
    } else if (strcmp(command, "task-name") == 0) {
        cmd_task_name(ctx, args, arg_count);
        return 1;
    } else if (strcmp(command, "task-capacity") == 0) {
        cmd_task_capacity(ctx, args, arg_count);
        return 1;
    } else if (strcmp(command, "task-suspend") == 0) {
        cmd_task_suspend(ctx, args, arg_count);
        return 1;
    } else if (strcmp(command, "task-resume") == 0) {
        cmd_task_resume(ctx, args, arg_count);
        return 1;
    } else if (strcmp(command, "kill-children") == 0) {
        cmd_kill_children(ctx, args, arg_count);
        return 1;
    } else if (strcmp(command, "children") == 0) {
        cmd_children(ctx, args, arg_count);
        return 1;
    } else if (strcmp(command, "wait-any-result") == 0) {
        cmd_wait_any_result(ctx, args, arg_count);
        return 1;
    } else if (strcmp(command, "child-exit-count") == 0) {
        cmd_child_exit_count(ctx, args, arg_count);
        return 1;
    } else if (strcmp(command, "task-delegate") == 0) {
        cmd_task_delegate(ctx, args, arg_count);
        return 1;
    } else if (strcmp(command, "task-events") == 0) {
        cmd_task_events(ctx, args, arg_count);
        return 1;
    } else if (strcmp(command, "task-events-observe") == 0) {
        cmd_task_events_observe(ctx, args, arg_count);
        return 1;
    } else if (strcmp(command, "task-events-clear") == 0) {
        cmd_task_events_clear(ctx, args, arg_count);
        return 1;
    } else if (strcmp(command, "task-event") == 0) {
        cmd_task_event(ctx, args, arg_count);
        return 1;
    } else if (strcmp(command, "task-events-forget") == 0) {
        cmd_task_events_forget(ctx, args, arg_count);
        return 1;
    } else if (strcmp(command, "task-summary") == 0) {
        cmd_task_summary(ctx, args, arg_count);
        return 1;
    } else if (strcmp(command, "task-events-notify") == 0) {
        cmd_task_events_notify(ctx, args, arg_count);
        return 1;
    } else if (strcmp(command, "task-events-filter") == 0) {
        cmd_task_events_filter(ctx, args, arg_count);
        return 1;
    } else if (strcmp(command, "task-events-notify-status") == 0) {
        cmd_task_events_notify_status(ctx, args, arg_count);
        return 1;
    } else if (strcmp(command, "task-events-watch") == 0) {
        cmd_task_events_watch_update(ctx, args, arg_count, 1U);
        return 1;
    } else if (strcmp(command, "task-events-unwatch") == 0) {
        cmd_task_events_watch_update(ctx, args, arg_count, 0U);
        return 1;
    } else if (strcmp(command, "task-events-watch-clear") == 0) {
        cmd_task_events_watch_clear(ctx, args, arg_count);
        return 1;
    } else if (strcmp(command, "task-events-watch-status") == 0) {
        cmd_task_events_watch_status(ctx, args, arg_count);
        return 1;
    } else if (strcmp(command, "task-events-notify-stats") == 0) {
        cmd_task_events_notify_stats(ctx, args, arg_count);
        return 1;
    } else if (strcmp(command, "task-events-notify-stats-clear") == 0) {
        cmd_task_events_notify_stats_clear(ctx, args, arg_count);
        return 1;
    } else if (strcmp(command, "task-event-replay") == 0) {
        cmd_task_event_replay(ctx, args, arg_count);
        return 1;
    } else if (strcmp(command, "task-priority-child") == 0) {
        cmd_task_priority_child(ctx, args, arg_count);
        return 1;
    } else if (strcmp(command, "task-priority-child-status") == 0) {
        cmd_task_priority_child_status(ctx, args, arg_count);
        return 1;
    } else if (strcmp(command, "task-events-budget") == 0) {
        cmd_task_events_budget(ctx, args, arg_count);
        return 1;
    } else if (strcmp(command, "task-events-budget-status") == 0) {
        cmd_task_events_budget_status(ctx, args, arg_count);
        return 1;
    } else if (strcmp(command, "fat16-list") == 0) {
        cmd_fat16_list(ctx, args, arg_count);
        return 1;
    } else if (strcmp(command, "fat16-cat") == 0) {
        cmd_fat16_cat(ctx, args, arg_count);
        return 1;
    } else if (strcmp(command, "child-result") == 0) {
        cmd_child_result(ctx, args, arg_count);
        return 1;
    } else if (strcmp(command, "child-result-any") == 0) {
        cmd_child_result_any(ctx, args, arg_count);
        return 1;
    } else if (strcmp(command, "child-results") == 0) {
        cmd_child_results(ctx, args, arg_count);
        return 1;
    } else if (strcmp(command, "child-results-forget") == 0) {
        cmd_child_results_forget(ctx, args, arg_count);
        return 1;
    } else if (strcmp(command, "child-results-clear") == 0) {
        cmd_child_results_clear(ctx, args, arg_count);
        return 1;
    } else if (strcmp(command, "child-results-observe") == 0) {
        cmd_child_results_observe(ctx, args, arg_count);
        return 1;
    } else if (strcmp(command, "wait") == 0) {
        cmd_task_wait(ctx, args, arg_count);
        return 1;
    } else if (strcmp(command, "wait-result") == 0) {
        cmd_wait_result(ctx, args, arg_count);
        return 1;
    } else if (strcmp(command, "sysinfo") == 0 || strcmp(command, "info") == 0) {
        cmd_sysinfo(ctx, args, arg_count);
        return 1;
    } else if (strcmp(command, "mem") == 0 || strcmp(command, "memory") == 0) {
        cmd_mem(ctx, args, arg_count);
        return 1;
    } else if (strcmp(command, "history") == 0) {
        cmd_history(ctx, args, arg_count);
        return 1;
    } else if (strcmp(command, "env") == 0) {
        cmd_env(ctx, args, arg_count);
        return 1;
    } else if (strcmp(command, "echo") == 0) {
        cmd_echo(ctx, args, arg_count);
        return 1;
    } else if (strcmp(command, "write") == 0) {
        cmd_write(ctx, args, arg_count);
        return 1;
    } else if (strcmp(command, "append") == 0) {
        cmd_append(ctx, args, arg_count);
        return 1;
    } else if (strcmp(command, "touch") == 0) {
        cmd_touch(ctx, args, arg_count);
        return 1;
    } else if (strcmp(command, "clear") == 0 || strcmp(command, "cls") == 0) {
        cmd_clear(ctx, args, arg_count);
        return 1;
    } else if (strcmp(command, "pwd") == 0) {
        cmd_pwd(ctx);
        return 1;
    } else if (strcmp(command, "cd") == 0) {
        cmd_cd(ctx, args, arg_count);
        return 1;
    } else if (strcmp(command, "cat") == 0) {
        cmd_cat(ctx, args, arg_count);
        return 1;
    } else if (strcmp(command, "stat") == 0) {
        cmd_stat(ctx, args, arg_count);
        return 1;
    } else if (strcmp(command, "test") == 0 || strcmp(command, "[") == 0) {
        int n = arg_count;
        if (command[0] == '[' && n > 0 && strcmp(args[n - 1], "]") == 0)
            n--;
        cmd_test(ctx, args, n);
        return 1;
    } else if (strcmp(command, "which") == 0) {
        if (arg_count == 0) {
            print_error("which: commande manquante");
        } else {
            cmd_which(ctx, args[0]);
        }
        return 1;
    } else if (strcmp(command, "exit") == 0 || strcmp(command, "quit") == 0) {
        cmd_exit(ctx, args, arg_count);
        return 1;
    } else if (strcmp(command, "ai") == 0) {
        cmd_ai(ctx, args, arg_count);
        return 1;
    } else if (strcmp(command, "ai-mode") == 0 || strcmp(command, "aimode") == 0) {
        cmd_ai_mode(ctx, args, arg_count);
        return 1;
    } else if (strcmp(command, "ai-help") == 0 || strcmp(command, "aihelp") == 0) {
        cmd_ai_help(ctx, args, arg_count);
        return 1;
    } else if (strcmp(command, "ai-test") == 0 || strcmp(command, "aitest") == 0) {
        cmd_ai_test(ctx);
        return 1;
    } else if (strcmp(command, "mkdir") == 0) {
        cmd_mkdir(ctx, args, arg_count);
        return 1;
    } else if (strcmp(command, "rmdir") == 0) {
        cmd_rmdir(ctx, args, arg_count);
        return 1;
    } else if (strcmp(command, "rm") == 0) {
        cmd_rm(ctx, args, arg_count);
        return 1;
    } else if (strcmp(command, "cp") == 0) {
        cmd_cp(ctx, args, arg_count);
        return 1;
    } else if (strcmp(command, "mv") == 0) {
        cmd_mv(ctx, args, arg_count);
        return 1;
    } else if (strcmp(command, "kill") == 0) {
        cmd_kill(ctx, args, arg_count);
        return 1;
    } else if (strcmp(command, "spawn") == 0) {
        cmd_spawn(ctx, args, arg_count);
        return 1;
    } else if (strcmp(command, "ipc-send") == 0) {
        cmd_ipc_send(ctx, args, arg_count);
        return 1;
    } else if (strcmp(command, "ipc-recv") == 0) {
        cmd_ipc_recv(ctx, args, arg_count);
        return 1;
    } else if (strcmp(command, "service-publish") == 0) {
        cmd_service_publish(ctx, args, arg_count);
        return 1;
    } else if (strcmp(command, "service-grant") == 0) {
        cmd_service_grant(ctx, args, arg_count);
        return 1;
    } else if (strcmp(command, "service-find") == 0) {
        cmd_service_find(ctx, args, arg_count);
        return 1;
    } else if (strcmp(command, "service-status") == 0) {
        cmd_service_status(ctx, args, arg_count);
        return 1;
    } else if (strcmp(command, "service-watch") == 0) {
        cmd_service_watch(ctx, args, arg_count);
        return 1;
    } else if (strcmp(command, "vfs-backend-probe") == 0) {
        cmd_vfs_backend_probe(ctx, args, arg_count);
        return 1;
    } else if (strcmp(command, "vfs-backend-write-probe") == 0) {
        cmd_vfs_backend_write_probe(ctx, args, arg_count);
        return 1;
    } else if (strcmp(command, "vfs-backend-remove-probe") == 0) {
        cmd_vfs_backend_remove_probe(ctx, args, arg_count);
        return 1;
    } else if (strcmp(command, "vfs-backend-rename-probe") == 0) {
        cmd_vfs_backend_rename_probe(ctx, args, arg_count);
        return 1;
    } else if (strcmp(command, "vfs-grant") == 0) {
        cmd_vfs_grant(ctx, args, arg_count);
    } else if (strcmp(command, "vfs-backend-grant") == 0) {
        cmd_vfs_backend_grant(ctx, args, arg_count);
        return 1;
    } else if (strcmp(command, "vfs-backend-grant-read") == 0) {
        cmd_vfs_backend_grant_read(ctx, args, arg_count);
        return 1;
    } else if (strcmp(command, "vfs-backend-grant-mutate") == 0) {
        cmd_vfs_backend_grant_mutate(ctx, args, arg_count);
        return 1;
    } else if (strcmp(command, "vfs-backend-revoke") == 0) {
        cmd_vfs_backend_revoke(ctx, args, arg_count);
        return 1;
    } else if (strcmp(command, "vfs-backend-status") == 0) {
        cmd_vfs_backend_status(ctx, args, arg_count);
        return 1;
    } else if (strcmp(command, "vfs-backend-scope") == 0) {
        cmd_vfs_backend_scope(ctx, args, arg_count);
        return 1;
    } else if (strcmp(command, "vfs-backend-observe") == 0) {
        cmd_vfs_backend_observe(ctx, args, arg_count);
        return 1;
    } else if (strcmp(command, "vfs-backend-list") == 0) {
        cmd_vfs_backend_list(ctx, args, arg_count);
        return 1;
    } else if (strcmp(command, "vfs-read") == 0) {
        cmd_vfs_read(ctx, args, arg_count);
        return 1;
    } else if (strcmp(command, "vfs-stat") == 0) {
        cmd_vfs_stat(ctx, args, arg_count);
        return 1;
    } else if (strcmp(command, "vfs-mkdir") == 0) {
        cmd_vfs_mkdir(ctx, args, arg_count);
    } else if (strcmp(command, "vfs-rmdir") == 0) {
        cmd_vfs_rmdir(ctx, args, arg_count);
        return 1;
    } else if (strcmp(command, "vfs-list") == 0) {
        cmd_vfs_list(ctx, args, arg_count);
    } else if (strcmp(command, "vfs-list-page") == 0) {
        cmd_vfs_list_page(ctx, args, arg_count);
        return 1;
    } else if (strcmp(command, "vfs-list-observe") == 0) {
        cmd_vfs_list_observe(ctx, args, arg_count);
        return 1;
    } else if (strcmp(command, "vfs-stats") == 0) {
        cmd_vfs_stats(ctx, args, arg_count);
        return 1;
    } else if (strcmp(command, "vfs-mount-add") == 0) {
        cmd_vfs_mount_add(ctx, args, arg_count);
        return 1;
    } else if (strcmp(command, "vfs-mount-remove") == 0) {
        cmd_vfs_mount_remove(ctx, args, arg_count);
        return 1;
    } else if (strcmp(command, "vfs-write") == 0) {
        cmd_vfs_write(ctx, args, arg_count);
        return 1;
    } else if (strcmp(command, "vfs-remove") == 0) {
        cmd_vfs_remove(ctx, args, arg_count);
        return 1;
    } else if (strcmp(command, "vfs-rename") == 0) {
        cmd_vfs_rename(ctx, args, arg_count);
        return 1;
    } else if (strcmp(command, "yield") == 0) {
        cmd_yield(ctx, args, arg_count);
        return 1;
    } else if (strcmp(command, "jobs") == 0) {
        cmd_jobs(ctx, args, arg_count);
        return 1;
    } else if (strcmp(command, "top") == 0) {
        cmd_top(ctx, args, arg_count);
        return 1;
    } else if (strcmp(command, "getpid") == 0) {
        cmd_getpid(ctx, args, arg_count);
        return 1;
    } else if (strcmp(command, "uptime") == 0) {
        cmd_uptime(ctx, args, arg_count);
        return 1;
    } else if (strcmp(command, "date") == 0) {
        cmd_date(ctx, args, arg_count);
        return 1;
    } else if (strcmp(command, "whoami") == 0) {
        cmd_whoami(ctx, args, arg_count);
        return 1;
    } else if (strcmp(command, "alias") == 0) {
        cmd_alias(ctx, args, arg_count);
        return 1;
    } else if (strcmp(command, "unalias") == 0) {
        cmd_unalias(ctx, args, arg_count);
        return 1;
    } else if (strcmp(command, "export") == 0) {
        cmd_export(ctx, args, arg_count);
        return 1;
    } else if (strcmp(command, "grep") == 0) {
        cmd_grep(ctx, args, arg_count);
        return 1;
    } else if (strcmp(command, "wc") == 0) {
        cmd_wc(ctx, args, arg_count);
        return 1;
    } else if (strcmp(command, "sort") == 0) {
        cmd_sort(ctx, args, arg_count);
        return 1;
    } else if (strcmp(command, "head") == 0) {
        cmd_head(ctx, args, arg_count);
        return 1;
    } else if (strcmp(command, "tail") == 0) {
        cmd_tail(ctx, args, arg_count);
        return 1;
    } else if (strcmp(command, "ai-stats") == 0 || strcmp(command, "aistats") == 0) {
        cmd_ai_stats(ctx, args, arg_count);
        return 1;
    } else if (strcmp(command, "rc") == 0) {
        cmd_rc(ctx);
        return 1;
    } else if (strcmp(command, "ai-provider") == 0) {
        cmd_ai_provider(ctx, args, arg_count);
        return 1;
    } else if (strcmp(command, "ai-model") == 0) {
        cmd_ai_model(ctx, args, arg_count);
        return 1;
    } else if (strcmp(command, "ai-runtime") == 0) {
        cmd_ai_runtime(ctx, args, arg_count);
        return 1;
    } else if (strcmp(command, "ai-continue") == 0) {
        cmd_ai_continue(ctx, args, arg_count);
        return 1;
    } else if (strcmp(command, "ai-credential") == 0) {
        cmd_ai_credential(ctx, args, arg_count);
        return 1;
    } else if (strcmp(command, "ai-acquire") == 0) {
        cmd_ai_acquire(ctx, args, arg_count);
        return 1;
    } else if (strcmp(command, "ai-tls-poll") == 0) {
        cmd_ai_tls_poll(ctx, args, arg_count);
        return 1;
    } else if (strcmp(command, "ai-request") == 0) {
        cmd_ai_request(ctx, args, arg_count, 0U);
        return 1;
    } else if (strcmp(command, "ai-stream-request") == 0) {
        cmd_ai_request(ctx, args, arg_count, 1U);
        return 1;
    } else if (strcmp(command, "ai-text-poll") == 0) {
        cmd_ai_text_poll(ctx, args, arg_count);
        return 1;
    } else if (strcmp(command, "ai-sse-poll") == 0) {
        cmd_ai_sse_poll(ctx, args, arg_count);
        return 1;
    } else if (strcmp(command, "ai-next") == 0) {
        cmd_ai_next(ctx, args, arg_count);
        return 1;
    } else if (strcmp(command, "ai-close") == 0) {
        cmd_ai_close(ctx, args, arg_count);
        return 1;
    } else if (strcmp(command, "net-status") == 0) {
        cmd_net_status(ctx, args, arg_count);
        return 1;
    } else if (strcmp(command, "logout") == 0) {
        cmd_exit(ctx, args, arg_count);
        return 1;
    } else if (strcmp(command, "reboot") == 0) {
        cmd_reboot(ctx, args, arg_count);
        return 1;
    } else if (strcmp(command, "shutdown") == 0) {
        cmd_shutdown(ctx, args, arg_count);
        return 1;
    }
    
    return 0; // Commande non trouvée
}

int is_question(const char* input) {
    // Détection simple de questions
    if (strstr(input, "?") != NULL) return 1;
    if (strncmp(input, "comment", 7) == 0) return 1;
    if (strncmp(input, "pourquoi", 8) == 0) return 1;
    if (strncmp(input, "qu'est-ce", 9) == 0) return 1;
    if (strncmp(input, "explain", 7) == 0) return 1;
    if (strncmp(input, "what", 4) == 0) return 1;
    if (strncmp(input, "how", 3) == 0) return 1;
    if (strncmp(input, "why", 3) == 0) return 1;
    
    return 0;
}

// ==============================================================================
// BOUCLE PRINCIPALE DU SHELL
// ==============================================================================

// Renvoie le nom de base du chemin (après le dernier '/')
static const char* get_basename(const char* path) {
    if (!path || path[0] == '\0') return "/";
    const char* end = path;
    // Aller à la fin
    while (*end) end++;
    // Sauter éventuels '/'
    while (end > path && *(end - 1) == '/') end--;
    if (end == path) return "/";
    // Chercher le dernier '/'
    const char* p = end;
    while (p > path && *(p - 1) != '/') p--;
    if (p == end) return "/"; // chemin composé uniquement de '/'
    return p;
}

void display_prompt(shell_context_t* ctx) {
    const char* folder = get_basename(ctx->current_dir);
    // Exemple: documents (-.-) :
    print_colored(folder, COLOR_BRIGHT);
    print_string(" (-.-) : ");
}

void handle_line(shell_context_t* ctx, char* input_buffer) {
    char command[128];
    char args[MAX_ARGS][128];
    int arg_count;

    // Ignorer les lignes vides
    if (strlen(input_buffer) == 0) {
        return;
    }
    
    // Ne jamais conserver un bearer OpenAI dans l'historique du shell.
    if (strncmp(input_buffer, "ai-credential ", 14U) == 0)
        add_to_history(ctx, "ai-credential [masque]");
    else
        add_to_history(ctx, input_buffer);

    // Vérifier si c'est une question en mode IA
    if (ctx->ai_mode && is_question(input_buffer)) {
        call_ai_assistant(ctx, input_buffer);
        return;
    }

    // Parser la commande
    if (!parse_command(input_buffer, command, args, &arg_count)) {
        return;
    }

    ctx->cmd_ticks++;

    // Expansion d'alias (une seule fois)
    for (int i = 0; i < ctx->alias_count; i++) {
        if (strcmp(command, ctx->aliases[i].alias) == 0) {
            char rebuilt[MAX_COMMAND_LENGTH];
            int pos = 0;
            const char* ac = ctx->aliases[i].command;
            int j = 0;
            while (ac[j] && pos < MAX_COMMAND_LENGTH - 2) rebuilt[pos++] = ac[j++];
            for (int a = 0; a < arg_count; a++) {
                if (pos < MAX_COMMAND_LENGTH - 2) rebuilt[pos++] = ' ';
                j = 0;
                while (args[a][j] && pos < MAX_COMMAND_LENGTH - 2) rebuilt[pos++] = args[a][j++];
            }
            rebuilt[pos] = '\0';
            parse_command(rebuilt, command, args, &arg_count);
            break;
        }
    }

    // Exécuter la commande builtin
    if (execute_builtin_command(ctx, command, args, arg_count)) {
        if (strcmp(command, "rc") != 0
            && strcmp(command, "test") != 0
            && strcmp(command, "[") != 0
            && strcmp(command, "ai-test") != 0
            && strcmp(command, "aitest") != 0)
            ctx->last_rc = 0;
        return;
    }

    // Si pas de commande builtin, essayer d'exécuter un programme externe
    char* exec_args[MAX_ARGS + 2];
    exec_args[0] = command;
    for (int i = 0; i < arg_count; i++) {
        exec_args[i + 1] = args[i];
    }
    exec_args[arg_count + 1] = NULL;

    // Résolution simple PATH: essayer tel quel (bloquant), puis bin/<cmd>
    int result = exec(command, exec_args);
    if (result != 0) {
        char alt[MAX_PATH_LENGTH];
        strcpy(alt, "bin/");
        strcat(alt, command);
        result = exec(alt, exec_args);
    }

    if (result != 0) {
        ctx->last_rc = 1;
        print_error("Commande non trouvée ou erreur d'exécution");
        print_string("   Tapez 'help' pour voir les commandes disponibles\n");
        
        // Suggestion IA si mode activé
        if (ctx->ai_mode) {
            print_colored("[IA] Suggestion IA : ", COLOR_YELLOW);
            print_string("Voulez-vous que je vous aide avec cette commande ?\n");
        }
    } else {
        ctx->last_rc = 0;
    }
}

void shell_main_loop(shell_context_t* ctx) {
    char buf[MAX_COMMAND_LENGTH];

    while (1) {
        display_prompt(ctx);
        buf[0] = '\0';
        // Lecture bloquante et stable de la ligne par le noyau
        gets(buf, (int)sizeof(buf));
        handle_line(ctx, buf);
    }
}

// ==============================================================================
// POINT D'ENTRÉE PRINCIPAL
// ==============================================================================

void main() {
    shell_context_t shell_ctx;
    
    // Initialiser le contexte du shell
    init_shell_context(&shell_ctx);
    
    // Affichage de bienvenue moderne
    cmd_clear(&shell_ctx, NULL, 0);
    
    print_colored("* Initialisation du Shell IA...", COLOR_CYAN);
    
    // Simulation d'initialisation progressive
    for (int i = 0; i < 3; i++) {
        for (volatile int j = 0; j < 10000000; j++); // Délai
        print_string(".");
    }
    print_string(" ");
    print_colored("TERMINÉ !\n\n", COLOR_GREEN);
    
    print_success("Shell MOHHDY v6.0 prêt à l'utilisation");
    print_info("Mode IA activé - Intelligence artificielle intégrée");
    print_info("Tapez 'help' pour découvrir toutes les fonctionnalités");
    
    print_string("\n");
    
    // Démarrer la boucle principale
    shell_main_loop(&shell_ctx);
    
    // Ne devrait jamais être atteint
    exit_program(0);
}