#include "syscall.h"
#include "kernel.h"
#include "../interrupts.h"
#include "../task/task.h"
#include "../keyboard.h"
#include "../elf.h"
#include "../../fs/initrd.h"
#include "../../fs/overlay.h"
#include "../mem/string.h"
#include "../mem/vmm.h"
#include "../mem/pmm.h"
#include "../timer.h"
#include "../llm/gpt2_infer.h"
#include "../llm/gpt2_gguf_infer.h"
#include "../llm/gpt2_model.h"
#include "../llm/gpt2_tokenizer.h"
#include "../service_registry.h"
#include "../fs/fat16.h"
#include "../fs/fat32.h"
#include "../net_socket.h"
/* Completions locales : BPE, top-k basse temperature, arret newline/EOT/repetition. */
#define GPT2_BAREMETAL_GENERATION_STEPS 12U

// Externs VMM
extern vmm_directory_t* current_directory;
extern void vmm_switch_page_directory(uint32_t phys_addr);

// Fonctions externes
extern void print_string_serial(const char* str);
extern uint32_t kernel_net_status(void);
extern uint32_t kernel_llm_session_status(void);
extern int kernel_llm_acquire_start(const os_llm_acquire_start_request_t* request);
extern int kernel_llm_poll_tls(void);
extern int kernel_llm_request(const os_llm_request_t* request);
extern int kernel_llm_poll_text(os_llm_text_result_t* result);
extern int kernel_llm_poll_sse(os_llm_text_result_t* result);
extern int kernel_llm_reset_for_request(void);
extern int kernel_llm_close(void);
extern int kernel_llm_configure_openai(const os_llm_openai_credential_request_t* request);
extern int kernel_llm_dhcp_maintenance(uint32_t now);
extern void print_char(char c, int x, int y, char color);
extern void write_serial(char c);

static void service_notify_change(const char* name, int32_t old_owner_pid,
                                  int32_t new_owner_pid, uint32_t reason) {
    os_ipc_payload_t payload;
    int32_t watchers[SERVICE_REGISTRY_WATCH_CAPACITY];
    int count;
    int i;
    if (os_service_make_event(&payload, name, old_owner_pid, new_owner_pid, reason) != 0) return;
    count = service_registry_collect_watchers(name, watchers, SERVICE_REGISTRY_WATCH_CAPACITY);
    if (count < 0) return;
    for (i = 0; i < count && i < (int)SERVICE_REGISTRY_WATCH_CAPACITY; i++) {
        task_t* watcher = get_task_by_id(watchers[i]);
        if (!watcher || watcher->type != TASK_TYPE_USER || watcher->state == TASK_TERMINATED) continue;
        /* Best effort non bloquant : une boîte pleine ne retarde jamais un changement de registre. */
        (void)ipc_endpoint_send(&watcher->ipc_endpoint, 0, &payload);
    }
}

static void service_notify_purge_pid(int32_t pid) {
    service_registry_entry_t owned[SERVICE_REGISTRY_CAPACITY];
    int count = service_registry_collect_owned(pid, owned, SERVICE_REGISTRY_CAPACITY);
    int i;
    if (count <= 0) return;
    (void)service_registry_remove_pid(pid);
    for (i = 0; i < count && i < (int)SERVICE_REGISTRY_CAPACITY; i++) {
        service_notify_change(owned[i].name, pid, 0, OS_SERVICE_EVENT_PURGED);
    }
}

// ==============================================================================
// GESTIONNAIRE D'APPELS SYSTÈME
// ==============================================================================

void syscall_handler(cpu_state_t* cpu) {
    // Réactive les interruptions pour permettre au clavier de fonctionner
    asm volatile("sti");

    /* Maintenance réseau différée : aucune E/S DHCP n’est réalisée dans IRQ0. */
    (void)kernel_llm_dhcp_maintenance(timer_get_ticks());

    // Le numéro de syscall est dans le registre EAX
    switch (cpu->eax) {
        case SYS_EXIT:
            service_notify_purge_pid(current_task->id);
            service_registry_backend_remove_pid(current_task->id);
            (void)service_registry_remove_watcher_pid(current_task->id);
            task_report_parent_exit(current_task, (int)cpu->ebx, OS_TASK_EVENT_EXITED);
            task_wake_waiter(current_task);
            task_reparent_children(current_task);
            current_task->state = TASK_TERMINATED;
            print_string_serial("[EXIT] task terminated, scheduling...\n");
            schedule(cpu);
            break;
        
        case SYS_PUTC:
            {
                extern void write_serial(char a);
                print_char((char)cpu->ebx, -1, -1, 0x0F); // VGA
                write_serial((char)cpu->ebx);             // Serial
            }
            break;
            
        case SYS_GETC:
            {
                // Réactiver les interruptions avant de lire le clavier
                asm volatile("sti");
                
                // Lecture clavier (ASCII)
                char c = keyboard_getc();
                cpu->eax = c;
                
                // Log uniquement si caractère non nul pour éviter le bruit
                if (c != 0) {
                    print_string_serial("SYS_GETC: caractère retourné: '");
                    write_serial(c);
                    print_string_serial("'\n");
                }
            }
            break;
            
        case SYS_PUTS:
            {
                char* str = (char*)cpu->ebx;
                if (str) {
                    for (int i = 0; i < 1024 && str[i] != '\0'; i++) {
                        char ch = str[i];
                        // Filtrer les non-imprimables (sauf \n, \r, \t)
                        if ((ch >= 32 && ch <= 126) || ch == '\n' || ch == '\r' || ch == '\t') {
                            print_char(ch, -1, -1, 0x0F);
                        }
                    }
                    // Garantir un flush visuel minimal
                    print_char('\n', -1, -1, 0x0F);
                }
            }
            break;
            
        case SYS_YIELD:
            /* Cooperative switch from the int 0x80 user frame (safe).
             * Nested int 0x30 / IRQ0 is not used: that frame has no SS/ESP. */
            cpu->eax = 0;
            schedule(cpu);
            break;
            
        // SYS_GETS - Lire une ligne depuis le clavier
        case SYS_GETS:
            sys_gets((char*)cpu->ebx, cpu->ecx);
            break;
            
        case SYS_EXEC:
            print_string_serial("[EXEC] starting child\n");
            {
                int rc = sys_exec((const char*)cpu->ebx, (char**)cpu->ecx);
                cpu->eax = (uint32_t)rc;
                if (rc >= 0) {
                    print_string_serial("[EXEC] waiting for child\n");
                    current_task->state = TASK_WAITING;
                    schedule(cpu);
                }
            }
            break;
        case SYS_SPAWN:
            print_string_serial("[SPAWN] starting child\n");
            cpu->eax = sys_spawn((const char*)cpu->ebx, (char**)cpu->ecx);
            print_string_serial("[SPAWN] child created\n");
            if ((int)cpu->eax >= 0) {
                schedule(cpu);
            }
            break;
        case SYS_LISTDIR:
            cpu->eax = (uint32_t)sys_listdir((const char*)cpu->ebx, (os_dirent_t*)cpu->ecx, (int)cpu->edx);
            break;
        case SYS_READFILE:
            cpu->eax = (uint32_t)sys_readfile((const char*)cpu->ebx, (char*)cpu->ecx, cpu->edx);
            break;
        case SYS_GETPID:
            cpu->eax = (uint32_t)sys_getpid();
            break;
        case SYS_PS:
            cpu->eax = (uint32_t)sys_ps((os_proc_t*)cpu->ebx, (int)cpu->ecx);
            break;
        case SYS_KILL:
            cpu->eax = (uint32_t)sys_kill((int)cpu->ebx);
            break;
        case SYS_TICKS:
            cpu->eax = sys_ticks();
            break;
        case SYS_MEMINFO:
            cpu->eax = (uint32_t)sys_meminfo((os_meminfo_t*)cpu->ebx);
            break;
        case SYS_TASK_METRICS:
            cpu->eax = (uint32_t)sys_task_metrics((int)cpu->ebx, (os_task_metrics_t*)cpu->ecx);
            break;
        case SYS_TASK_SET_PRIORITY:
            cpu->eax = (uint32_t)sys_task_set_priority((int)cpu->ebx, cpu->ecx);
            break;
        case SYS_TASK_WAIT:
            cpu->eax = (uint32_t)sys_task_wait((int)cpu->ebx);
            if ((int)cpu->eax == 0) schedule(cpu);
            break;
        case SYS_TASK_SET_NAME:
            cpu->eax = (uint32_t)sys_task_set_name((int)cpu->ebx, (const char*)cpu->ecx);
            break;
        case SYS_TASK_CAPACITY:
            cpu->eax = (uint32_t)sys_task_capacity((os_task_capacity_t*)cpu->ebx);
            break;
        case SYS_TASK_CHILD_RESULT:
            cpu->eax = (uint32_t)sys_task_child_result((int)cpu->ebx,
                                                        (os_task_exit_result_t*)cpu->ecx);
            break;
        case SYS_TASK_CHILD_RESULT_LIST:
            cpu->eax = (uint32_t)sys_task_child_result_list((os_task_exit_history_t*)cpu->ebx);
            break;
        case SYS_TASK_CHILD_RESULT_ACK:
            cpu->eax = (uint32_t)sys_task_child_result_ack();
            break;
        case SYS_TASK_CHILD_RESULT_OBSERVE:
            cpu->eax = (uint32_t)sys_task_child_result_observe(cpu->ebx,
                (os_task_exit_history_observation_t*)cpu->ecx);
            break;
        case SYS_TASK_CHILD_RESULT_FIND:
            cpu->eax = (uint32_t)sys_task_child_result_find((int)cpu->ebx,
                (os_task_exit_result_t*)cpu->ecx);
            break;
        case SYS_TASK_CHILD_RESULT_FORGET:
            cpu->eax = (uint32_t)sys_task_child_result_forget((int)cpu->ebx);
            break;
        case SYS_TASK_SUSPEND:
            cpu->eax = (uint32_t)sys_task_suspend((int)cpu->ebx);
            break;
        case SYS_TASK_RESUME:
            cpu->eax = (uint32_t)sys_task_resume((int)cpu->ebx);
            break;
        case SYS_TASK_KILL_CHILDREN:
            cpu->eax = (uint32_t)sys_task_kill_children();
            break;
        case SYS_TASK_CHILDREN:
            cpu->eax = (uint32_t)sys_task_children((os_task_children_t*)cpu->ebx);
            break;
        case SYS_TASK_WAIT_ANY:
            cpu->eax = (uint32_t)sys_task_wait_any();
            if ((int)cpu->eax == 0) schedule(cpu);
            break;
        case SYS_TASK_CHILD_EXIT_COUNT:
            cpu->eax = (uint32_t)sys_task_child_exit_count((os_task_child_exit_count_t*)cpu->ebx);
            break;
        case SYS_TASK_DELEGATE_CHILD:
            cpu->eax = (uint32_t)sys_task_delegate_child((int)cpu->ebx, (int)cpu->ecx);
            break;
        case SYS_TASK_SUPERVISION_EVENTS:
            cpu->eax = (uint32_t)sys_task_supervision_events((os_task_supervision_events_t*)cpu->ebx);
            break;
        case SYS_TASK_SUPERVISION_EVENTS_ACK:
            cpu->eax = (uint32_t)sys_task_supervision_events_ack();
            break;
        case SYS_TASK_SUPERVISION_EVENTS_OBSERVE:
            cpu->eax = (uint32_t)sys_task_supervision_events_observe(cpu->ebx,
                (os_task_supervision_events_observation_t*)cpu->ecx);
            break;
        case SYS_TASK_SUPERVISION_EVENT_FIND:
            cpu->eax = (uint32_t)sys_task_supervision_event_find(cpu->ebx,
                (os_task_supervision_event_t*)cpu->ecx);
            break;
        case SYS_TASK_SUPERVISION_EVENT_FORGET:
            cpu->eax = (uint32_t)sys_task_supervision_event_forget(cpu->ebx);
            break;
        case SYS_TASK_SUPERVISION_SUMMARY:
            cpu->eax = (uint32_t)sys_task_supervision_summary(
                (os_task_supervision_summary_t*)cpu->ebx);
            break;
        case SYS_TASK_SUPERVISION_NOTIFY:
            cpu->eax = (uint32_t)sys_task_supervision_notify(cpu->ebx);
            break;
        case SYS_TASK_SUPERVISION_NOTIFY_FILTER:
            cpu->eax = (uint32_t)sys_task_supervision_notify_filter(cpu->ebx);
            break;
        case SYS_TASK_SUPERVISION_NOTIFY_STATUS:
            cpu->eax = (uint32_t)sys_task_supervision_notify_status(
                (os_task_supervision_notify_status_t*)cpu->ebx);
            break;
        case SYS_TASK_SUPERVISION_WATCH:
            cpu->eax = (uint32_t)sys_task_supervision_watch((int)cpu->ebx, cpu->ecx);
            break;
        case SYS_TASK_SUPERVISION_WATCH_STATUS:
            cpu->eax = (uint32_t)sys_task_supervision_watch_status(
                (os_task_supervision_watch_status_t*)cpu->ebx);
            break;
        case SYS_TASK_SUPERVISION_DELIVERY_STATS:
            cpu->eax = (uint32_t)sys_task_supervision_delivery_stats(
                (os_task_supervision_delivery_stats_t*)cpu->ebx);
            break;
        case SYS_TASK_SUPERVISION_DELIVERY_STATS_ACK:
            cpu->eax = (uint32_t)sys_task_supervision_delivery_stats_ack();
            break;
        case SYS_TASK_SUPERVISION_EVENT_REPLAY:
            cpu->eax = (uint32_t)sys_task_supervision_event_replay(cpu->ebx);
            break;
        case SYS_TASK_SUPERVISION_PRIORITY:
            cpu->eax = (uint32_t)sys_task_supervision_priority((int)cpu->ebx);
            break;
        case SYS_TASK_SUPERVISION_PRIORITY_STATUS:
            cpu->eax = (uint32_t)sys_task_supervision_priority_status(
                (os_task_supervision_priority_status_t*)cpu->ebx);
            break;
        case SYS_TASK_SUPERVISION_NOTIFY_BUDGET:
            cpu->eax = (uint32_t)sys_task_supervision_notify_budget(cpu->ebx);
            break;
        case SYS_TASK_SUPERVISION_NOTIFY_BUDGET_STATUS:
            cpu->eax = (uint32_t)sys_task_supervision_notify_budget_status(
                (os_task_supervision_notify_budget_status_t*)cpu->ebx);
            break;
        case SYS_FAT16_READ:
            cpu->eax = (uint32_t)sys_fat16_read((const char*)cpu->ebx,
                                                 (char*)cpu->ecx, cpu->edx);
            break;
        case SYS_FAT16_LIST:
            cpu->eax = (uint32_t)sys_fat16_list((os_fat16_dirent_t*)cpu->ebx, cpu->ecx);
            break;
        case SYS_FAT16_LIST_PAGE:
            cpu->eax = (uint32_t)sys_fat16_list_page((os_fat16_dirent_t*)cpu->ebx, cpu->ecx, cpu->edx);
            break;
        case SYS_FAT32_READ:
            cpu->eax = (uint32_t)sys_fat32_read((const char*)cpu->ebx, (char*)cpu->ecx, cpu->edx);
            break;
        case SYS_FAT32_LIST:
            cpu->eax = (uint32_t)sys_fat32_list((os_fat16_dirent_t*)cpu->ebx, cpu->ecx);
            break;
        case SYS_FAT32_LIST_PAGE:
            cpu->eax = (uint32_t)sys_fat32_list_page((os_fat16_dirent_t*)cpu->ebx, cpu->ecx, cpu->edx);
            break;
        case SYS_FAT16_LIST_PATH:
            cpu->eax = (uint32_t)sys_fat16_list_path((const char*)cpu->ebx,
                                                       (os_fat16_dirent_t*)cpu->ecx, cpu->edx,
                                                       cpu->esi);
            break;
        case SYS_FAT32_LIST_PATH:
            cpu->eax = (uint32_t)sys_fat32_list_path((const char*)cpu->ebx,
                                                       (os_fat16_dirent_t*)cpu->ecx, cpu->edx,
                                                       cpu->esi);
            break;

        case SYS_SERVICE_BACKEND_RELEASE:
            cpu->eax = (uint32_t)sys_service_backend_release((const char*)cpu->ebx);
            break;
        case SYS_SOCKET_OPEN:
            cpu->eax = (uint32_t)sys_socket_open((uint16_t)cpu->ebx, (uint16_t)cpu->ecx, cpu->edx);
            break;
        case SYS_SOCKET_LISTEN:
            cpu->eax = (uint32_t)sys_socket_listen((uint16_t)cpu->ebx, cpu->ecx);
            break;
        case SYS_SOCKET_ACCEPT_SYN:
            cpu->eax = (uint32_t)sys_socket_accept_syn((int)cpu->ebx,
                (const os_socket_passive_view_t*)cpu->ecx);
            break;
        case SYS_SOCKET_BUILD_SYN_ACK:
            cpu->eax = (uint32_t)sys_socket_build_syn_ack((int)cpu->ebx,
                (uint8_t*)cpu->ecx, (uint16_t)cpu->edx, (uint16_t*)cpu->esi);
            break;
        case SYS_SOCKET_ACCEPT_ACK:
            cpu->eax = (uint32_t)sys_socket_accept_ack((int)cpu->ebx,
                (const os_socket_passive_view_t*)cpu->ecx);
            break;
        case SYS_SOCKET_ACCEPT_SYN_ACK:
            cpu->eax = (uint32_t)sys_socket_accept_syn_ack((int)cpu->ebx,
                (const os_socket_syn_ack_t*)cpu->ecx);
            break;
        case SYS_SOCKET_SEND:
            cpu->eax = (uint32_t)sys_socket_send((const os_socket_send_request_t*)cpu->ebx);
            break;
        case SYS_SOCKET_FEED:
            cpu->eax = (uint32_t)sys_socket_feed((const os_socket_feed_request_t*)cpu->ebx);
            break;
        case SYS_SOCKET_RECEIVE:
            cpu->eax = (uint32_t)sys_socket_receive((const os_socket_receive_request_t*)cpu->ebx);
            break;
        case SYS_SOCKET_CLOSE:
            cpu->eax = (uint32_t)sys_socket_close((int)cpu->ebx);
            break;
        case SYS_NET_STATUS:
            cpu->eax = kernel_net_status();
            break;
        case SYS_LLM_SESSION_STATUS:
            cpu->eax = kernel_llm_session_status();
            break;
        case SYS_LLM_ACQUIRE_START:
            cpu->eax = (uint32_t)kernel_llm_acquire_start(
                (const os_llm_acquire_start_request_t*)cpu->ebx);
            break;
        case SYS_LLM_POLL_TLS:
            cpu->eax = (uint32_t)kernel_llm_poll_tls();
            break;
        case SYS_LLM_REQUEST:
            cpu->eax = (uint32_t)kernel_llm_request((const os_llm_request_t*)cpu->ebx);
            break;
        case SYS_LLM_POLL_TEXT:
            cpu->eax = (uint32_t)kernel_llm_poll_text((os_llm_text_result_t*)cpu->ebx);
            break;
        case SYS_LLM_POLL_SSE:
            cpu->eax = (uint32_t)kernel_llm_poll_sse((os_llm_text_result_t*)cpu->ebx);
            break;
        case SYS_LLM_RESET_FOR_REQUEST:
            cpu->eax = (uint32_t)kernel_llm_reset_for_request();
            break;
        case SYS_LLM_CLOSE:
            cpu->eax = (uint32_t)kernel_llm_close();
            break;
        case SYS_LLM_OPENAI_CREDENTIAL:
            cpu->eax = (uint32_t)kernel_llm_configure_openai((const os_llm_openai_credential_request_t*)cpu->ebx);
            break;
        case SYS_MKDIR:
            cpu->eax = (uint32_t)sys_mkdir((const char*)cpu->ebx);
            break;
        case SYS_UNLINK:
            cpu->eax = (uint32_t)sys_unlink((const char*)cpu->ebx);
            break;
        case SYS_WRITEFILE:
            cpu->eax = (uint32_t)sys_writefile((const char*)cpu->ebx, (const char*)cpu->ecx, cpu->edx);
            break;
        case SYS_STAT:
            cpu->eax = (uint32_t)sys_stat((const char*)cpu->ebx, (os_dirent_t*)cpu->ecx);
            break;
        case SYS_RENAME:
            cpu->eax = (uint32_t)sys_rename((const char*)cpu->ebx, (const char*)cpu->ecx);
            break;
        case SYS_COPY:
            cpu->eax = (uint32_t)sys_copy((const char*)cpu->ebx, (const char*)cpu->ecx);
            break;
        case SYS_APPEND:
            cpu->eax = (uint32_t)sys_append((const char*)cpu->ebx, (const char*)cpu->ecx, cpu->edx);
            break;
        case SYS_GPT2_GENERATE:
            cpu->eax = (uint32_t)sys_gpt2_generate((const char*)cpu->ebx, (char*)cpu->ecx, cpu->edx);
            break;
        case SYS_GPT2_GGUF_GENERATE:
            cpu->eax = (uint32_t)sys_gpt2_gguf_generate((const char*)cpu->ebx, (char*)cpu->ecx, cpu->edx);
            break;
        case SYS_GPT2_GGUF_CONTINUE:
            cpu->eax = (uint32_t)sys_gpt2_gguf_continue((char*)cpu->ecx, cpu->edx);
            break;
        case SYS_IPC_SEND:
            cpu->eax = (uint32_t)sys_ipc_send((int)cpu->ebx,
                                               (const os_ipc_payload_t*)cpu->ecx);
            /* Le shell dort ensuite dans SYS_GETS (Ring 0) et ne peut pas être
             * préempté par IRQ0. Un handoff coopératif livre donc sans délai un
             * message à une autre tâche utilisateur déjà prête. */
            if ((int)cpu->eax == 0 && task_has_other_ready_user()) schedule(cpu);
            break;
        case SYS_IPC_RECV:
            cpu->eax = (uint32_t)sys_ipc_receive((os_ipc_message_t*)cpu->ebx);
            break;
        case SYS_SERVICE_REGISTER:
            cpu->eax = (uint32_t)sys_service_register((const char*)cpu->ebx);
            break;
        case SYS_SERVICE_LOOKUP:
            cpu->eax = (uint32_t)sys_service_lookup((const char*)cpu->ebx);
            break;
        case SYS_SERVICE_UNREGISTER:
            cpu->eax = (uint32_t)sys_service_unregister((const char*)cpu->ebx);
            break;
        case SYS_SERVICE_GRANT:
            cpu->eax = (uint32_t)sys_service_grant((const char*)cpu->ebx, (int)cpu->ecx);
            if ((int)cpu->eax == 0 && task_has_other_ready_user()) schedule(cpu);
            break;
        case SYS_SERVICE_BACKEND_GRANT:
            cpu->eax = (uint32_t)sys_service_backend_grant((const char*)cpu->ebx, (int)cpu->ecx);
            if ((int)cpu->eax == 0 && task_has_other_ready_user()) schedule(cpu);
            break;
        case SYS_SERVICE_BACKEND_REVOKE:
            cpu->eax = (uint32_t)sys_service_backend_revoke((const char*)cpu->ebx, (int)cpu->ecx);
            if ((int)cpu->eax == 0 && task_has_other_ready_user()) schedule(cpu);
            break;
        case SYS_SERVICE_BACKEND_GRANT_SCOPED:
            cpu->eax = (uint32_t)sys_service_backend_grant_scoped((const char*)cpu->ebx, (int)cpu->ecx, cpu->edx);
            if ((int)cpu->eax == 0 && task_has_other_ready_user()) schedule(cpu);
            break;
        case SYS_SERVICE_BACKEND_GRANT_SCOPED_SOURCE:
            cpu->eax = (uint32_t)sys_service_backend_grant_scoped_source((const char*)cpu->ebx,
                                                                          (int)cpu->ecx, cpu->edx, cpu->esi);
            if ((int)cpu->eax == 0 && task_has_other_ready_user()) schedule(cpu);
            break;
        case SYS_SERVICE_BACKEND_SCOPE_STATUS:
            cpu->eax = (uint32_t)sys_service_backend_scope_status((const char*)cpu->ebx,
                                                                    (int)cpu->ecx,
                                                                    (os_service_backend_scope_t*)cpu->edx);
            break;
        case SYS_SERVICE_BACKEND_GRANT_SCOPED_SOURCE_PREFIX:
            cpu->eax = (uint32_t)sys_service_backend_grant_scoped_source_prefix((const char*)cpu->ebx,
                                                                                   (int)cpu->ecx, cpu->edx,
                                                                                   cpu->esi, (const char*)cpu->edi);
            if ((int)cpu->eax == 0 && task_has_other_ready_user()) schedule(cpu);
            break;
        case SYS_SERVICE_BACKEND_STATUS:
            cpu->eax = (uint32_t)sys_service_backend_status((const char*)cpu->ebx, (int)cpu->ecx, (uint32_t*)cpu->edx);
            break;
        case SYS_SERVICE_BACKEND_LIST:
            cpu->eax = (uint32_t)sys_service_backend_list((const char*)cpu->ebx, (os_service_backend_list_t*)cpu->ecx);
            break;
        case SYS_SERVICE_BACKEND_OBSERVE:
            cpu->eax = (uint32_t)sys_service_backend_observe((const char*)cpu->ebx, cpu->ecx,
                                                              (os_service_backend_snapshot_t*)cpu->edx);
            break;
        case SYS_SERVICE_NOTIFY:
            cpu->eax = (uint32_t)sys_service_notify((const char*)cpu->ebx);
            break;
        case SYS_SERVICE_STATUS:
            cpu->eax = (uint32_t)sys_service_status((const char*)cpu->ebx,
                                                    (os_service_status_t*)cpu->ecx);
            break;
        case SYS_VFS_BACKEND_READ:
            cpu->eax = (uint32_t)sys_vfs_backend_read((const char*)cpu->ebx,
                                                       (char*)cpu->ecx, cpu->edx);
            break;
        case SYS_VFS_BACKEND_WRITE:
            cpu->eax = (uint32_t)sys_vfs_backend_write((const char*)cpu->ebx,
                                                         (const char*)cpu->ecx, cpu->edx);
            break;
        case SYS_VFS_FAT16_CREATE:
            cpu->eax = (uint32_t)sys_vfs_fat16_create((const char*)cpu->ebx,
                                                       (const char*)cpu->ecx, cpu->edx);
            break;
        case SYS_VFS_FAT16_UNLINK:
            cpu->eax = (uint32_t)sys_vfs_fat16_unlink((const char*)cpu->ebx);
            break;
        case SYS_VFS_FAT16_RENAME:
            cpu->eax = (uint32_t)sys_vfs_fat16_rename((const char*)cpu->ebx,
                                                       (const char*)cpu->ecx);
            break;
        case SYS_VFS_FAT32_CREATE:
            cpu->eax = (uint32_t)sys_vfs_fat32_create((const char*)cpu->ebx,
                                                       (const char*)cpu->ecx, cpu->edx);
            break;
        case SYS_VFS_FAT32_UNLINK:
            cpu->eax = (uint32_t)sys_vfs_fat32_unlink((const char*)cpu->ebx);
            break;
        case SYS_VFS_FAT32_RENAME:
            cpu->eax = (uint32_t)sys_vfs_fat32_rename((const char*)cpu->ebx,
                                                       (const char*)cpu->ecx);
            break;
        case SYS_VFS_INITRD_READ:
            cpu->eax = (uint32_t)sys_vfs_initrd_read((const char*)cpu->ebx,
                                                      (char*)cpu->ecx, cpu->edx);
            break;
        case SYS_VFS_OVERLAY_READ:
            cpu->eax = (uint32_t)sys_vfs_overlay_read((const char*)cpu->ebx,
                                                       (char*)cpu->ecx, cpu->edx);
            break;
        case SYS_VFS_OVERLAY_UNLINK:
            cpu->eax = (uint32_t)sys_vfs_overlay_unlink((const char*)cpu->ebx);
            break;
        case SYS_VFS_OVERLAY_RENAME:
            cpu->eax = (uint32_t)sys_vfs_overlay_rename((const char*)cpu->ebx,
                                                         (const char*)cpu->ecx);
            break;
        case SYS_VFS_INITRD_STAT:
            cpu->eax = (uint32_t)sys_vfs_initrd_stat((const char*)cpu->ebx,
                                                      (os_dirent_t*)cpu->ecx);
            break;
        case SYS_VFS_OVERLAY_STAT:
            cpu->eax = (uint32_t)sys_vfs_overlay_stat((const char*)cpu->ebx,
                                                       (os_dirent_t*)cpu->ecx);
            break;
        case SYS_VFS_INITRD_LISTDIR:
            cpu->eax = (uint32_t)sys_vfs_initrd_listdir((const char*)cpu->ebx,
                                                         (os_dirent_t*)cpu->ecx,
                                                         (int)cpu->edx);
            break;
        case SYS_VFS_OVERLAY_LISTDIR:
            cpu->eax = (uint32_t)sys_vfs_overlay_listdir((const char*)cpu->ebx,
                                                           (os_dirent_t*)cpu->ecx,
                                                           (int)cpu->edx);
            break;
        case SYS_VFS_INITRD_LISTDIR_PAGE:
            cpu->eax = (uint32_t)sys_vfs_initrd_listdir_page((const char*)cpu->ebx,
                                                               (os_dirent_t*)cpu->ecx,
                                                               cpu->edx);
            break;
        case SYS_VFS_OVERLAY_LISTDIR_PAGE:
            cpu->eax = (uint32_t)sys_vfs_overlay_listdir_page((const char*)cpu->ebx,
                                                                (os_dirent_t*)cpu->ecx,
                                                                cpu->edx);
            break;
        case SYS_VFS_OVERLAY_MKDIR:
            cpu->eax = (uint32_t)sys_vfs_overlay_mkdir((const char*)cpu->ebx);
            break;
        case SYS_VFS_OVERLAY_RMDIR:
            cpu->eax = (uint32_t)sys_vfs_overlay_rmdir((const char*)cpu->ebx);
            break;
        default:

            // Syscall inconnu
            break;
    }
}

int sys_ipc_send(int target_pid, const os_ipc_payload_t* payload) {
    task_t* target;
    if (!current_task || !payload || payload->size > OS_IPC_MAX_DATA) {
        return OS_IPC_BAD_MESSAGE;
    }
    target = get_task_by_id(target_pid);
    if (!target || target->type != TASK_TYPE_USER || target->state == TASK_TERMINATED) {
        return OS_IPC_BAD_TARGET;
    }
    if (service_registry_pid_is_owner(target_pid) &&
        target->ipc_endpoint.count >= IPC_SERVICE_ENDPOINT_CAPACITY) {
        return OS_IPC_SERVICE_FULL;
    }
    return ipc_endpoint_send(&target->ipc_endpoint, current_task->id, payload);
}

int sys_ipc_receive(os_ipc_message_t* out) {
    if (!current_task || current_task->type != TASK_TYPE_USER || !out) {
        return OS_IPC_BAD_MESSAGE;
    }
    return ipc_endpoint_receive(&current_task->ipc_endpoint, out);
}

int sys_task_supervision_notify(uint32_t enabled) {
    if (!current_task || current_task->type != TASK_TYPE_USER) return OS_TASK_NOT_FOUND;
    return task_set_supervision_notify(current_task->id, enabled);
}

int sys_task_supervision_notify_filter(uint32_t mask) {
    if (!current_task || current_task->type != TASK_TYPE_USER) return OS_TASK_NOT_FOUND;
    return task_set_supervision_notify_filter(current_task->id, mask);
}

int sys_task_supervision_notify_status(os_task_supervision_notify_status_t* out) {
    if (!current_task || current_task->type != TASK_TYPE_USER || !out) return OS_TASK_NOT_FOUND;
    return task_fill_supervision_notify_status(current_task->id, out);
}

int sys_task_supervision_watch(int child_pid, uint32_t enabled) {
    if (!current_task || current_task->type != TASK_TYPE_USER) return OS_TASK_NOT_FOUND;
    return task_update_supervision_watch(current_task->id, child_pid, enabled);
}

int sys_task_supervision_watch_status(os_task_supervision_watch_status_t* out) {
    if (!current_task || current_task->type != TASK_TYPE_USER || !out) return OS_TASK_NOT_FOUND;
    return task_fill_supervision_watch_status(current_task->id, out);
}

int sys_task_supervision_delivery_stats(os_task_supervision_delivery_stats_t* out) {
    if (!current_task || current_task->type != TASK_TYPE_USER || !out) return OS_TASK_NOT_FOUND;
    return task_fill_supervision_delivery_stats(current_task->id, out);
}

int sys_task_supervision_delivery_stats_ack(void) {
    if (!current_task || current_task->type != TASK_TYPE_USER) return OS_TASK_NOT_FOUND;
    return task_ack_supervision_delivery_stats(current_task->id);
}

int sys_task_supervision_event_replay(uint32_t sequence) {
    if (!current_task || current_task->type != TASK_TYPE_USER) return OS_TASK_NOT_FOUND;
    return task_replay_supervision_event(current_task->id, sequence);
}

int sys_task_supervision_priority(int child_pid) {
    if (!current_task || current_task->type != TASK_TYPE_USER) return OS_TASK_NOT_FOUND;
    return task_set_supervision_priority(current_task->id, child_pid);
}

int sys_task_supervision_priority_status(os_task_supervision_priority_status_t* out) {
    if (!current_task || current_task->type != TASK_TYPE_USER || !out) return OS_TASK_NOT_FOUND;
    return task_fill_supervision_priority_status(current_task->id, out);
}

int sys_task_supervision_notify_budget(uint32_t limit) {
    if (!current_task || current_task->type != TASK_TYPE_USER) return OS_TASK_NOT_FOUND;
    return task_set_supervision_notify_budget(current_task->id, limit);
}

int sys_task_supervision_notify_budget_status(os_task_supervision_notify_budget_status_t* out) {
    if (!current_task || current_task->type != TASK_TYPE_USER || !out) return OS_TASK_NOT_FOUND;
    return task_fill_supervision_notify_budget_status(current_task->id, out);
}

static int vfs_backend_allowed_for_source(uint32_t right, uint32_t source);
static int vfs_backend_allowed_for_source_path(uint32_t right, uint32_t source,
                                               const char* path);

int sys_fat16_read(const char* name, char* buffer, uint32_t max) {
    if (!vfs_backend_allowed_for_source_path(SERVICE_BACKEND_RIGHT_READ,
                                             OS_SERVICE_BACKEND_SOURCE_FAT16, name))
        return OS_VFS_BACKEND_DENIED;
    if (!name || !buffer || max == 0U) return OS_FAT16_BAD_PATH;
    return fat16_read_path(fat16_root(), name, buffer, max);
}

int sys_fat16_list(os_fat16_dirent_t* out, uint32_t capacity) {
    if (!vfs_backend_allowed_for_source(SERVICE_BACKEND_RIGHT_READ, OS_SERVICE_BACKEND_SOURCE_FAT16))
        return OS_VFS_BACKEND_DENIED;
    if (!out || capacity == 0U) return OS_FAT16_BAD_PATH;
    return fat16_list_root(fat16_root(), out, capacity);
}

int sys_fat16_list_page(os_fat16_dirent_t* out, uint32_t capacity, uint32_t start) {
    if (!vfs_backend_allowed_for_source(SERVICE_BACKEND_RIGHT_READ, OS_SERVICE_BACKEND_SOURCE_FAT16))
        return OS_VFS_BACKEND_DENIED;
    if (!out || capacity == 0U) return OS_FAT16_BAD_PATH;
    return fat16_list_root_page(fat16_root(), start, out, capacity);
}

int sys_fat16_list_path(const char* path, os_fat16_dirent_t* out, uint32_t capacity,
                        uint32_t start) {
    if (!vfs_backend_allowed_for_source_path(SERVICE_BACKEND_RIGHT_READ,
                                             OS_SERVICE_BACKEND_SOURCE_FAT16, path))
        return OS_VFS_BACKEND_DENIED;
    if (!path || !out || capacity == 0U) return OS_FAT16_BAD_PATH;
    return fat16_list_path_page(fat16_root(), path, start, out, capacity);
}

int sys_fat32_read(const char* name, char* buffer, uint32_t max) {
    if (!vfs_backend_allowed_for_source_path(SERVICE_BACKEND_RIGHT_READ,
                                             OS_SERVICE_BACKEND_SOURCE_FAT32, name))
        return OS_VFS_BACKEND_DENIED;
    if (!name || !buffer || max == 0U) return OS_FAT16_BAD_PATH;
    return fat32_read_path(fat32_root(), name, (uint8_t*)buffer, max);
}

int sys_fat32_list(os_fat16_dirent_t* out, uint32_t capacity) {
    if (!vfs_backend_allowed_for_source(SERVICE_BACKEND_RIGHT_READ, OS_SERVICE_BACKEND_SOURCE_FAT32))
        return OS_VFS_BACKEND_DENIED;
    if (!out || capacity == 0U) return OS_FAT16_BAD_PATH;
    return fat32_list_root(fat32_root(), out, capacity);
}

int sys_fat32_list_page(os_fat16_dirent_t* out, uint32_t capacity, uint32_t start) {
    if (!vfs_backend_allowed_for_source(SERVICE_BACKEND_RIGHT_READ, OS_SERVICE_BACKEND_SOURCE_FAT32))
        return OS_VFS_BACKEND_DENIED;
    if (!out || capacity == 0U) return OS_FAT16_BAD_PATH;
    return fat32_list_root_page(fat32_root(), start, out, capacity);
}

int sys_fat32_list_path(const char* path, os_fat16_dirent_t* out, uint32_t capacity,
                        uint32_t start) {
    if (!vfs_backend_allowed_for_source_path(SERVICE_BACKEND_RIGHT_READ,
                                             OS_SERVICE_BACKEND_SOURCE_FAT32, path))
        return OS_VFS_BACKEND_DENIED;
    if (!path || !out || capacity == 0U) return OS_FAT16_BAD_PATH;
    return fat32_list_path_page(fat32_root(), path, start, out, capacity);
}
static int syscall_user_range(const void* pointer, uint32_t length, int write) {
    uint32_t start, end, address;
    page_t* page;
    if (!current_task || current_task->type != TASK_TYPE_USER || !current_task->vmm_dir || !pointer) return 0;
    start = (uint32_t)pointer;
    if (length == 0U) return 1;
    if (start > 0xffffffffU - (length - 1U)) return 0;
    end = start + length - 1U;
    if (start >= 0xc0000000U || end >= 0xc0000000U) return 0;
    for (address = start & ~(PAGE_SIZE - 1U); ; address += PAGE_SIZE) {
        page = vmm_get_page(address, 0, current_task->vmm_dir);
        if (!page || !page->present || !page->user || (write && !page->rw)) return 0;
        if (address > end - (end % PAGE_SIZE)) break;
        if (address > 0xffffffffU - PAGE_SIZE) return 0;
    }
    return 1;
}

int sys_socket_open(uint16_t local_port, uint16_t remote_port, uint32_t local_sequence) {
    if (!current_task || current_task->type != TASK_TYPE_USER) return OS_SOCKET_BAD_ARGUMENT;
    return net_socket_open(local_port, remote_port, local_sequence);
}

int sys_socket_listen(uint16_t local_port, uint32_t local_sequence) {
    if (!current_task || current_task->type != TASK_TYPE_USER) return OS_SOCKET_BAD_ARGUMENT;
    return net_socket_listen(local_port, local_sequence);
}

static int sys_socket_passive_view_copy(const os_socket_passive_view_t* source, net_tcp_view_t* target) {
    if (!syscall_user_range(source, sizeof(*source), 0) || !target) return OS_SOCKET_BAD_ARGUMENT;
    target->source_port = source->source_port; target->destination_port = source->destination_port;
    target->sequence = source->sequence; target->acknowledgment = source->acknowledgment;
    target->flags = source->flags; target->payload = 0; target->payload_length = 0U;
    return 0;
}

int sys_socket_accept_syn(int socket_id, const os_socket_passive_view_t* view) {
    net_tcp_view_t tcp_view;
    if (sys_socket_passive_view_copy(view, &tcp_view) != 0) return OS_SOCKET_BAD_ARGUMENT;
    return net_socket_accept_syn(socket_id, &tcp_view);
}

int sys_socket_build_syn_ack(int socket_id, uint8_t* segment, uint16_t capacity, uint16_t* out_length) {
    if (!syscall_user_range(segment, capacity, 1) || !syscall_user_range(out_length, sizeof(*out_length), 1)) return OS_SOCKET_BAD_ARGUMENT;
    return net_socket_build_syn_ack(socket_id, segment, capacity, out_length);
}

int sys_socket_accept_ack(int socket_id, const os_socket_passive_view_t* view) {
    net_tcp_view_t tcp_view;
    if (sys_socket_passive_view_copy(view, &tcp_view) != 0) return OS_SOCKET_BAD_ARGUMENT;
    return net_socket_accept_ack(socket_id, &tcp_view);
}

int sys_socket_accept_syn_ack(int socket_id, const os_socket_syn_ack_t* view) {
    net_tcp_view_t tcp_view;
    if (!syscall_user_range(view, sizeof(*view), 0)) return OS_SOCKET_BAD_ARGUMENT;
    tcp_view.source_port = view->source_port; tcp_view.destination_port = view->destination_port;
    tcp_view.sequence = view->sequence; tcp_view.acknowledgment = view->acknowledgment;
    tcp_view.flags = view->flags; tcp_view.payload = 0; tcp_view.payload_length = 0U;
    return net_socket_accept_syn_ack(socket_id, &tcp_view);
}

int sys_socket_send(const os_socket_send_request_t* request) {
    if (!syscall_user_range(request, sizeof(*request), 0) || !syscall_user_range(request->payload, request->length, 0) || !syscall_user_range(request->segment, request->capacity, 1) || !syscall_user_range(request->out_length, sizeof(*request->out_length), 1)) return OS_SOCKET_BAD_ARGUMENT;
    return net_socket_send(request->socket_id, request->payload, request->length, request->segment, request->capacity, request->out_length);
}

int sys_socket_feed(const os_socket_feed_request_t* request) {
    if (!syscall_user_range(request, sizeof(*request), 0) || !syscall_user_range(request->segment, request->length, 0)) return OS_SOCKET_BAD_ARGUMENT;
    return net_socket_feed(request->socket_id, request->segment, request->length);
}

int sys_socket_receive(const os_socket_receive_request_t* request) {
    if (!syscall_user_range(request, sizeof(*request), 0) || !syscall_user_range(request->buffer, request->capacity, 1) || !syscall_user_range(request->out_length, sizeof(*request->out_length), 1)) return OS_SOCKET_BAD_ARGUMENT;
    return net_socket_receive(request->socket_id, request->buffer, request->capacity, request->out_length);
}

int sys_socket_close(int socket_id) {
    if (!current_task || current_task->type != TASK_TYPE_USER) return OS_SOCKET_BAD_ARGUMENT;
    return net_socket_close(socket_id);
}

int sys_service_register(const char* name) {
    int owner_pid;
    int rc;
    task_t* owner;
    if (!current_task || current_task->type != TASK_TYPE_USER) return OS_SERVICE_BAD_NAME;
    owner_pid = service_registry_lookup(name);
    if (owner_pid > 0) {
        owner = get_task_by_id(owner_pid);
        if (!owner || owner->type != TASK_TYPE_USER || owner->state == TASK_TERMINATED) {
            (void)service_registry_remove(name, owner_pid);
            service_notify_change(name, owner_pid, 0, OS_SERVICE_EVENT_PURGED);
            owner_pid = OS_SERVICE_NOT_FOUND;
        }
    }
    rc = service_registry_register(name, current_task->id);
    if (rc == 0 && owner_pid == OS_SERVICE_NOT_FOUND) {
        service_notify_change(name, 0, current_task->id, OS_SERVICE_EVENT_PUBLISHED);
    }
    return rc;
}

int sys_service_lookup(const char* name) {
    int owner_pid = service_registry_lookup(name);
    task_t* owner;
    if (owner_pid < 0) return owner_pid;
    owner = get_task_by_id(owner_pid);
    if (!owner || owner->type != TASK_TYPE_USER || owner->state == TASK_TERMINATED) {
        (void)service_registry_remove(name, owner_pid);
        service_notify_change(name, owner_pid, 0, OS_SERVICE_EVENT_PURGED);
        return OS_SERVICE_NOT_FOUND;
    }
    return owner_pid;
}

int sys_service_unregister(const char* name) {
    int rc;
    if (!current_task || current_task->type != TASK_TYPE_USER) return OS_SERVICE_BAD_NAME;
    rc = service_registry_remove(name, current_task->id);
    if (rc == 0) service_notify_change(name, current_task->id, 0, OS_SERVICE_EVENT_UNREGISTERED);
    return rc;
}

int sys_service_grant(const char* name, int target_pid) {
    task_t* target;
    int rc;
    if (!current_task || current_task->type != TASK_TYPE_USER) return OS_SERVICE_BAD_NAME;
    target = get_task_by_id(target_pid);
    if (!target || target->type != TASK_TYPE_USER || target->state == TASK_TERMINATED) {
        return OS_SERVICE_BAD_GRANTEE;
    }
    rc = service_registry_grant(name, current_task->id, target_pid);
    if (rc == 0) service_notify_change(name, current_task->id, target_pid, OS_SERVICE_EVENT_GRANTED);
    return rc;
}

int sys_service_backend_grant(const char* name, int target_pid) {
    task_t* target;
    if (!current_task || current_task->type != TASK_TYPE_USER) return OS_SERVICE_BAD_NAME;
    target = get_task_by_id(target_pid);
    if (!target || target->type != TASK_TYPE_USER || target->state == TASK_TERMINATED) {
        return OS_SERVICE_BAD_GRANTEE;
    }
    return service_registry_backend_grant(name, current_task->id, target_pid);
}

int sys_service_backend_grant_scoped(const char* name, int target_pid, uint32_t rights) {
    task_t* target;
    if (!current_task || current_task->type != TASK_TYPE_USER) return OS_SERVICE_BAD_NAME;
    target = get_task_by_id(target_pid);
    if (!target || target->type != TASK_TYPE_USER || target->state == TASK_TERMINATED) return OS_SERVICE_BAD_GRANTEE;
    return service_registry_backend_grant_scoped(name, current_task->id, target_pid, rights);
}

int sys_service_backend_grant_scoped_source(const char* name, int target_pid, uint32_t rights,
                                            uint32_t sources) {
    task_t* target;
    if (!current_task || current_task->type != TASK_TYPE_USER) return OS_SERVICE_BAD_NAME;
    target = get_task_by_id(target_pid);
    if (!target || target->type != TASK_TYPE_USER || target->state == TASK_TERMINATED) return OS_SERVICE_BAD_GRANTEE;
    return service_registry_backend_grant_scoped_source(name, current_task->id, target_pid, rights, sources);
}

int sys_service_backend_grant_scoped_source_prefix(const char* name, int target_pid,
                                                   uint32_t rights, uint32_t sources,
                                                   const char* prefix) {
    task_t* target;
    if (!current_task || current_task->type != TASK_TYPE_USER) return OS_SERVICE_BAD_NAME;
    target = get_task_by_id(target_pid);
    if (!target || target->type != TASK_TYPE_USER || target->state == TASK_TERMINATED) return OS_SERVICE_BAD_GRANTEE;
    return service_registry_backend_grant_scoped_source_prefix(name, current_task->id, target_pid,
                                                               rights, sources, prefix);
}

int sys_service_backend_revoke(const char* name, int target_pid) {
    task_t* target;
    if (!current_task || current_task->type != TASK_TYPE_USER) return OS_SERVICE_BAD_NAME;
    target = get_task_by_id(target_pid);
    if (!target || target->type != TASK_TYPE_USER || target->state == TASK_TERMINATED) return OS_SERVICE_BAD_GRANTEE;
    return service_registry_backend_revoke(name, current_task->id, target_pid);
}

int sys_service_backend_release(const char* name) {
    if (!current_task || current_task->type != TASK_TYPE_USER) return OS_SERVICE_BAD_NAME;
    return service_registry_backend_release(name, current_task->id);
}

int sys_service_backend_status(const char* name, int target_pid, uint32_t* out_rights) {
    task_t* target;
    if (!current_task || current_task->type != TASK_TYPE_USER || !out_rights) return OS_SERVICE_BAD_NAME;
    target = get_task_by_id(target_pid);
    if (!target || target->type != TASK_TYPE_USER || target->state == TASK_TERMINATED) return OS_SERVICE_BAD_GRANTEE;
    return service_registry_backend_rights(name, current_task->id, target_pid, out_rights);
}

int sys_service_backend_scope_status(const char* name, int target_pid,
                                     os_service_backend_scope_t* out_scope) {
    task_t* target;
    if (!current_task || current_task->type != TASK_TYPE_USER || !out_scope) return OS_SERVICE_BAD_NAME;
    target = get_task_by_id(target_pid);
    if (!target || target->type != TASK_TYPE_USER || target->state == TASK_TERMINATED) return OS_SERVICE_BAD_GRANTEE;
    return service_registry_backend_scope(name, current_task->id, target_pid, out_scope);
}

int sys_service_backend_list(const char* name, os_service_backend_list_t* out_list) {
    if (!current_task || current_task->type != TASK_TYPE_USER || !out_list) return OS_SERVICE_BAD_NAME;
    return service_registry_backend_list(name, current_task->id, out_list);
}

int sys_service_backend_observe(const char* name, uint32_t expected_generation,
                                os_service_backend_snapshot_t* out_snapshot) {
    if (!current_task || current_task->type != TASK_TYPE_USER || !out_snapshot) return OS_SERVICE_BAD_NAME;
    return service_registry_backend_observe(name, current_task->id, expected_generation, out_snapshot);
}

int sys_service_notify(const char* name) {
    if (!current_task || current_task->type != TASK_TYPE_USER) return OS_SERVICE_BAD_NAME;
    return service_registry_subscribe(name, current_task->id);
}

int sys_service_status(const char* name, os_service_status_t* out) {
    int owner_pid;
    task_t* owner;
    if (!current_task || current_task->type != TASK_TYPE_USER || !out) {
        return OS_SERVICE_BAD_NAME;
    }
    owner_pid = sys_service_lookup(name);
    if (owner_pid < 0) return owner_pid;
    owner = get_task_by_id(owner_pid);
    if (!owner || owner->type != TASK_TYPE_USER || owner->state == TASK_TERMINATED) {
        return OS_SERVICE_NOT_FOUND;
    }
    out->owner_pid = owner_pid;
    out->queued_messages = owner->ipc_endpoint.count;
    out->client_capacity = IPC_SERVICE_ENDPOINT_CAPACITY;
    out->endpoint_capacity = IPC_ENDPOINT_CAPACITY;
    return 0;
}

static int vfs_backend_allowed_for_source_path(uint32_t right, uint32_t source,
                                                   const char* path) {
    return current_task && current_task->type == TASK_TYPE_USER &&
        (service_registry_lookup("vfs") == current_task->id ||
         service_registry_backend_allowed_for_source_path("vfs", current_task->id, right,
                                                          source, path));
}

static int vfs_backend_allowed_for_source(uint32_t right, uint32_t source) {
    return vfs_backend_allowed_for_source_path(right, source, (const char*)0);
}

/* La lecture générique peut consulter overlay puis initrd : elle exige donc
 * toutes les sources. L’écriture générique est une primitive overlay seule. */
static int vfs_backend_allowed(uint32_t right, const char* path) {
    return vfs_backend_allowed_for_source_path(right, OS_SERVICE_BACKEND_SOURCE_ALL, path);
}

int sys_vfs_backend_read(const char* path, char* buffer, uint32_t max) {
    if (!vfs_backend_allowed(SERVICE_BACKEND_RIGHT_READ, path)) {
        return OS_VFS_BACKEND_DENIED;
    }
    return sys_readfile(path, buffer, max);
}

int sys_vfs_backend_write(const char* path, const char* data, uint32_t size) {
    if (!vfs_backend_allowed_for_source_path(SERVICE_BACKEND_RIGHT_MUTATE,
                                             OS_SERVICE_BACKEND_SOURCE_OVERLAY, path)) {
        return OS_VFS_BACKEND_DENIED;
    }
    return sys_writefile(path, data, size);
}

static void generate_short_alias(const char* name, char* short_out) {
    uint32_t i = 0U, base = 0U, ext = 0U;
    while (name[i] != '\0' && name[i] != '.') {
        char c = name[i++];
        if (c >= 'a' && c <= 'z') c = (char)(c - 'a' + 'A');
        if ((c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '_') {
            if (base < 6U) short_out[base++] = c;
        }
    }
    if (base == 0U) {
        short_out[base++] = 'F';
        short_out[base++] = 'I';
        short_out[base++] = 'L';
        short_out[base++] = 'E';
    }
    short_out[base++] = '~';
    short_out[base++] = '1';
    if (name[i] == '.') {
        i++;
        while (name[i] != '\0' && ext < 3U) {
            char c = name[i++];
            if (c >= 'a' && c <= 'z') c = (char)(c - 'a' + 'A');
            if ((c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '_') {
                if (ext == 0U) short_out[base++] = '.';
                short_out[base++] = c;
                ext++;
            }
        }
    }
    if (ext == 0U) {
        short_out[base++] = '.';
        short_out[base++] = 'T';
        short_out[base++] = 'X';
        short_out[base++] = 'T';
    }
    short_out[base] = '\0';
}

static int is_strict_short_83(const char* name) {
    uint32_t i = 0U, base = 0U, ext = 0U;
    int dot = 0;
    if (!name || name[0] == '\0') return 0;
    while (name[i] != '\0') {
        char c = name[i];
        if (c == '.') {
            if (dot || base == 0U) return 0;
            dot = 1; i++; continue;
        }
        if (!((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '_' || c == '-')) return 0;
        if (!dot) { if (++base > 8U) return 0; }
        else { if (++ext > 3U) return 0; }
        i++;
    }
    return base > 0U;
}

static int fat_path_has_separator(const char* path) {
    uint32_t i = 0U;
    if (!path) return 0;
    while (path[i] != '\0') {
        if (path[i++] == '/') return 1;
    }
    return 0;
}

static int fat_path_is_directory_request(const char* path) {
    uint32_t i = 0U;
    if (!path || path[0] == '\0') return 0;
    while (path[i] != '\0') i++;
    return i > 0U && path[i - 1U] == '/';
}

static int fat_directory_name(const char* path, char* out) {
    uint32_t i = 0U;
    if (!path || !out || !fat_path_is_directory_request(path)) return OS_FAT16_BAD_PATH;
    while (path[i] != '\0' && path[i + 1U] != '\0') {
        if (i >= OS_NAME_MAX - 1U) return OS_FAT16_BAD_PATH;
        out[i] = path[i];
        i++;
    }
    if (i == 0U) return OS_FAT16_BAD_PATH;
    out[i] = '\0';
    return 0;
}

/* Création FAT16 explicite : gère à la fois le format 8.3 classique et les noms longs LFN. */
int sys_vfs_fat16_create(const char* name, const char* data, uint32_t size) {
    uint16_t first_cluster = 0U;
    char short_alias[16];
    if (!vfs_backend_allowed_for_source_path(SERVICE_BACKEND_RIGHT_MUTATE,
                                             OS_SERVICE_BACKEND_SOURCE_FAT16, name)) {
        return OS_VFS_BACKEND_DENIED;
    }
    if (!name || (size != 0U && !data)) return OS_FAT16_BAD_PATH;
    /* Le worker encode `mkdir` par un buffer nul et une taille nulle : une
     * écriture vide garde un buffer non nul et reste donc un fichier. */
    if (!data && size == 0U) return fat16_create_directory(fat16_root(), name);
    if (fat_path_has_separator(name)) {
        return fat16_create_path_file(fat16_root(), name, (const uint8_t*)data, size,
                                      &first_cluster);
    }
    if (is_strict_short_83(name)) {
        return fat16_create_file(fat16_root(), name, 0x20U, (const uint8_t*)data, size,
                                 &first_cluster);
    }
    generate_short_alias(name, short_alias);
    return fat16_create_lfn_file(fat16_root(), name, short_alias, 0x20U, (const uint8_t*)data, size,
                                 &first_cluster);
}

/* Suppression FAT16 explicite : supporte 8.3 et LFN. */
int sys_vfs_fat16_unlink(const char* name) {
    if (!vfs_backend_allowed_for_source_path(SERVICE_BACKEND_RIGHT_MUTATE,
                                             OS_SERVICE_BACKEND_SOURCE_FAT16, name)) {
        return OS_VFS_BACKEND_DENIED;
    }
    if (!name) return OS_FAT16_BAD_PATH;
    if (fat_path_is_directory_request(name)) {
        char directory[OS_NAME_MAX];
        int rc = fat_directory_name(name, directory);
        return rc == 0 ? fat16_remove_directory(fat16_root(), directory) : rc;
    }
    return fat16_unlink_path_file(fat16_root(), name);
}

/* Renommage FAT16 explicite : supporte 8.3 et LFN. */
int sys_vfs_fat16_rename(const char* old_name, const char* new_name) {
    char new_short[16];
    if (!vfs_backend_allowed_for_source_path(SERVICE_BACKEND_RIGHT_MUTATE,
                                             OS_SERVICE_BACKEND_SOURCE_FAT16, old_name) ||
        !vfs_backend_allowed_for_source_path(SERVICE_BACKEND_RIGHT_MUTATE,
                                             OS_SERVICE_BACKEND_SOURCE_FAT16, new_name)) {
        return OS_VFS_BACKEND_DENIED;
    }
    if (!old_name || !new_name) return OS_FAT16_BAD_PATH;
    if (fat_path_has_separator(old_name) || fat_path_has_separator(new_name)) {
        return fat16_rename_path_file(fat16_root(), old_name, new_name);
    }
    if (is_strict_short_83(old_name) && is_strict_short_83(new_name)) {
        return fat16_rename_file(fat16_root(), old_name, new_name);
    }
    generate_short_alias(new_name, new_short);
    return fat16_rename_lfn_file(fat16_root(), old_name, new_name, new_short);
}

int sys_vfs_fat32_create(const char* name, const char* data, uint32_t size) {
    uint32_t first_cluster = 0U;
    char short_alias[16];
    if (!vfs_backend_allowed_for_source_path(SERVICE_BACKEND_RIGHT_MUTATE,
                                             OS_SERVICE_BACKEND_SOURCE_FAT32, name)) {
        return OS_VFS_BACKEND_DENIED;
    }
    if (!name || (size != 0U && !data)) return OS_FAT16_BAD_PATH;
    if (!data && size == 0U) return fat32_create_directory(fat32_root(), name);
    if (fat_path_has_separator(name)) {
        return fat32_create_path_file(fat32_root(), name, (const uint8_t*)data, size,
                                      &first_cluster);
    }
    if (is_strict_short_83(name)) {
        return fat32_create_file(fat32_root(), name, 0x20U, (const uint8_t*)data, size,
                                 &first_cluster);
    }
    generate_short_alias(name, short_alias);
    return fat32_create_lfn_file(fat32_root(), name, short_alias, 0x20U, (const uint8_t*)data, size,
                                 &first_cluster);
}

int sys_vfs_fat32_unlink(const char* name) {
    if (!vfs_backend_allowed_for_source_path(SERVICE_BACKEND_RIGHT_MUTATE,
                                             OS_SERVICE_BACKEND_SOURCE_FAT32, name)) {
        return OS_VFS_BACKEND_DENIED;
    }
    if (!name) return OS_FAT16_BAD_PATH;
    if (fat_path_is_directory_request(name)) {
        char directory[OS_NAME_MAX];
        int rc = fat_directory_name(name, directory);
        return rc == 0 ? fat32_remove_directory(fat32_root(), directory) : rc;
    }
    return fat32_unlink_path_file(fat32_root(), name);
}

int sys_vfs_fat32_rename(const char* old_name, const char* new_name) {
    char new_short[16];
    if (!vfs_backend_allowed_for_source_path(SERVICE_BACKEND_RIGHT_MUTATE,
                                             OS_SERVICE_BACKEND_SOURCE_FAT32, old_name) ||
        !vfs_backend_allowed_for_source_path(SERVICE_BACKEND_RIGHT_MUTATE,
                                             OS_SERVICE_BACKEND_SOURCE_FAT32, new_name)) {
        return OS_VFS_BACKEND_DENIED;
    }
    if (!old_name || !new_name) return OS_FAT16_BAD_PATH;
    if (fat_path_has_separator(old_name) || fat_path_has_separator(new_name)) {
        return fat32_rename_path_file(fat32_root(), old_name, new_name);
    }
    if (is_strict_short_83(old_name) && is_strict_short_83(new_name)) {
        return fat32_rename_file(fat32_root(), old_name, new_name);
    }
    generate_short_alias(new_name, new_short);
    return fat32_rename_lfn_file(fat32_root(), old_name, new_name, new_short);
}

int sys_vfs_initrd_read(const char* path, char* buffer, uint32_t max) {
    if (!vfs_backend_allowed_for_source_path(SERVICE_BACKEND_RIGHT_READ,
                                             OS_SERVICE_BACKEND_SOURCE_INITRD, path)) {
        return OS_VFS_BACKEND_DENIED;
    }
    if (!path || !buffer || max == 0U) return -1;
    return initrd_read_into(path, buffer, max);
}

int sys_vfs_overlay_read(const char* path, char* buffer, uint32_t max) {
    if (!vfs_backend_allowed_for_source_path(SERVICE_BACKEND_RIGHT_READ,
                                             OS_SERVICE_BACKEND_SOURCE_OVERLAY, path)) {
        return OS_VFS_BACKEND_DENIED;
    }
    if (!path || !buffer || max == 0U) return -1;
    return overlay_read(path, buffer, max);
}

int sys_vfs_overlay_unlink(const char* path) {
    if (!vfs_backend_allowed_for_source_path(SERVICE_BACKEND_RIGHT_MUTATE,
                                             OS_SERVICE_BACKEND_SOURCE_OVERLAY, path)) {
        return OS_VFS_BACKEND_DENIED;
    }
    if (!path) return -1;
    return overlay_unlink(path);
}

int sys_vfs_overlay_rename(const char* oldpath, const char* newpath) {
    if (!vfs_backend_allowed_for_source_path(SERVICE_BACKEND_RIGHT_MUTATE,
                                             OS_SERVICE_BACKEND_SOURCE_OVERLAY, oldpath) ||
        !vfs_backend_allowed_for_source_path(SERVICE_BACKEND_RIGHT_MUTATE,
                                             OS_SERVICE_BACKEND_SOURCE_OVERLAY, newpath)) {
        return OS_VFS_BACKEND_DENIED;
    }
    if (!oldpath || !newpath) return -1;
    return overlay_rename(oldpath, newpath);
}

int sys_vfs_initrd_stat(const char* path, os_dirent_t* out) {
    if (!vfs_backend_allowed_for_source_path(SERVICE_BACKEND_RIGHT_READ,
                                             OS_SERVICE_BACKEND_SOURCE_INITRD, path)) {
        return OS_VFS_BACKEND_DENIED;
    }
    if (!path || !out) return -1;
    return initrd_stat(path, out);
}

int sys_vfs_overlay_stat(const char* path, os_dirent_t* out) {
    if (!vfs_backend_allowed_for_source_path(SERVICE_BACKEND_RIGHT_READ,
                                             OS_SERVICE_BACKEND_SOURCE_OVERLAY, path)) {
        return OS_VFS_BACKEND_DENIED;
    }
    if (!path || !out) return -1;
    return overlay_stat(path, out);
}

int sys_vfs_initrd_listdir(const char* path, os_dirent_t* out, int max_n) {
    if (!vfs_backend_allowed_for_source_path(SERVICE_BACKEND_RIGHT_READ,
                                             OS_SERVICE_BACKEND_SOURCE_INITRD, path)) {
        return OS_VFS_BACKEND_DENIED;
    }
    if (!path || !out || max_n <= 0 || !initrd_is_dir(path)) return -1;
    return initrd_listdir(path, out, max_n);
}

int sys_vfs_overlay_listdir(const char* path, os_dirent_t* out, int max_n) {
    if (!vfs_backend_allowed_for_source_path(SERVICE_BACKEND_RIGHT_READ,
                                             OS_SERVICE_BACKEND_SOURCE_OVERLAY, path)) {
        return OS_VFS_BACKEND_DENIED;
    }
    if (!path || !out || max_n <= 0 || !overlay_is_dir(path)) return -1;
    return overlay_listdir(path, out, 0, max_n);
}

int sys_vfs_initrd_listdir_page(const char* path, os_dirent_t* out, uint32_t start) {
    if (!vfs_backend_allowed_for_source_path(SERVICE_BACKEND_RIGHT_READ,
                                             OS_SERVICE_BACKEND_SOURCE_INITRD, path)) {
        return OS_VFS_BACKEND_DENIED;
    }
    if (!path || !out || !initrd_is_dir(path)) return -1;
    return initrd_listdir_page(path, out, start, 5);
}

int sys_vfs_overlay_listdir_page(const char* path, os_dirent_t* out, uint32_t start) {
    if (!vfs_backend_allowed_for_source_path(SERVICE_BACKEND_RIGHT_READ,
                                             OS_SERVICE_BACKEND_SOURCE_OVERLAY, path)) {
        return OS_VFS_BACKEND_DENIED;
    }
    if (!path || !out || !overlay_is_dir(path)) return -1;
    return overlay_listdir_page(path, out, start, 5);
}

int sys_vfs_overlay_mkdir(const char* path) {
    if (!vfs_backend_allowed_for_source_path(SERVICE_BACKEND_RIGHT_MUTATE,
                                             OS_SERVICE_BACKEND_SOURCE_OVERLAY, path)) {
        return OS_VFS_BACKEND_DENIED;
    }
    if (!path) return -1;
    return overlay_mkdir(path);
}

int sys_vfs_overlay_rmdir(const char* path) {
    if (!vfs_backend_allowed_for_source_path(SERVICE_BACKEND_RIGHT_MUTATE,
                                             OS_SERVICE_BACKEND_SOURCE_OVERLAY, path)) {
        return OS_VFS_BACKEND_DENIED;
    }
    if (!path || !overlay_is_dir(path)) return -1;
    return overlay_unlink(path);
}

/*
 * Generation locale GPT-2. Les tailles sont volontairement bornees : le
 * modele s'execute dans le noyau freestanding et ne doit jamais consommer un
 * buffer utilisateur non borne.
 */
static uint32_t gguf_session_tokens[64];
static uint32_t gguf_session_token_count;
static uint32_t gguf_session_prompt_tokens;
static uint32_t gguf_session_rng;
static uint8_t gguf_session_active;

static int sys_gpt2_gguf_session_step(char* out, uint32_t max) {
    const char* piece;
    uint32_t next_token = 0U;
    uint32_t written = 0U;
    uint32_t generated_count;
    int rc;
    if (!out || max < 2U) return -1;
    if (!gguf_session_active) return -6;
    if (gguf_session_token_count == 0U || gguf_session_token_count >= 64U) {
        gguf_session_active = 0U;
        out[0] = '\0';
        return 0;
    }
    generated_count = gguf_session_token_count - gguf_session_prompt_tokens;
    rc = gpt2_gguf_generate_next_sampled(gguf_session_tokens, gguf_session_token_count,
                                         generated_count, &next_token, &gguf_session_rng);
    if (rc != 0) return -30 + rc;
    if (next_token == gpt2_tokenizer_eot()) {
        gguf_session_active = 0U;
        out[0] = '\0';
        return 0;
    }
    gguf_session_tokens[gguf_session_token_count++] = next_token;
    piece = gpt2_tokenizer_decode(next_token);
    if (!piece) return -4;
    for (uint32_t i = 0U; piece[i] != '\0'; i++) {
        if (written + 1U >= max) break;
        out[written++] = piece[i];
    }
    out[written] = '\0';
    return (int)written;
}

static int sys_gpt2_generate_impl(const char* prompt, char* out, uint32_t max,
                                  uint8_t use_gguf) {
    char prompt_copy[128];
    uint32_t tokens[64];
    uint32_t token_count = 0;
    uint32_t written = 0;
    uint32_t rng_state;
    uint32_t prompt_tokens;
    uint32_t generation_steps;
    uint32_t prev_generated = 0xFFFFFFFFu;
    const gpt2_model_t* model;
    int rc;

    if (!prompt || !out || max < 2) return -1;
    if (use_gguf) gguf_session_active = 0U;
    /* Conserve punctuation, apostrophes, espaces simples et UTF-8. */
    uint32_t input_pos = 0;
    uint32_t output_pos = 0;
    while (input_pos < 255U && prompt[input_pos] != '\0' && output_pos + 1U < sizeof(prompt_copy)) {
        uint8_t ch = (uint8_t)prompt[input_pos++];
        if (ch == '\t' || ch == '\r' || ch == '\n') ch = ' ';
        if (ch < 32U || ch == 127U) continue;
        if (ch == ' ' && (output_pos == 0U || (uint8_t)prompt_copy[output_pos - 1U] == ' ')) continue;
        prompt_copy[output_pos++] = (char)ch;
    }
    prompt_copy[output_pos] = '\0';

    rc = gpt2_tokenizer_encode(prompt_copy, tokens, 64, &token_count);
    if (rc != 0) return -2;
    model = gpt2_model_current();
    if (use_gguf) {
        if (!gpt2_gguf_infer_ready() || token_count > GPT2_GGUF_INFER_MAX_CONTEXT) return -5;
    } else if (!model->ready || token_count > model->config.max_seq_len) return -5;
    prompt_tokens = token_count;

    generation_steps = use_gguf ? 1U : GPT2_BAREMETAL_GENERATION_STEPS;
    rng_state = 0x9e3779b9U;
    for (uint32_t i = 0; prompt_copy[i] != '\0'; i++) {
        rng_state = rng_state * 16777619U + (uint8_t)prompt_copy[i];
    }
    if (rng_state == 0U) rng_state = 1U;
    if (use_gguf) {
        for (uint32_t i = 0U; i < token_count; i++) gguf_session_tokens[i] = tokens[i];
        gguf_session_token_count = token_count;
        gguf_session_prompt_tokens = prompt_tokens;
        gguf_session_rng = rng_state;
        gguf_session_active = 1U;
        return sys_gpt2_gguf_session_step(out, max);
    }

    for (uint32_t step = 0; step < generation_steps && token_count < 64 &&
         (use_gguf || token_count < model->config.max_seq_len); step++) {
        uint32_t next_token = 0;
        const char* piece;
        int saw_newline = 0;
        uint32_t generated_count = token_count - prompt_tokens;
        if (use_gguf)
            rc = gpt2_gguf_generate_next_sampled(tokens, token_count, generated_count,
                                                  &next_token, &rng_state);
        else
            rc = gpt2_generate_next_sampled(tokens, token_count, generated_count,
                                            &next_token, &rng_state);
        if (rc != 0) return -30 + rc;
        if (next_token == gpt2_tokenizer_eot()) break;
        if (next_token == prev_generated) break;
        prev_generated = next_token;
        tokens[token_count++] = next_token;
        piece = gpt2_tokenizer_decode(next_token);
        if (!piece) return -4;
        for (uint32_t i = 0; piece[i] != '\0'; i++) {
            if (written + 1 >= max) {
                out[written] = '\0';
                return (int)written;
            }
            out[written++] = piece[i];
            if (piece[i] == '\n') saw_newline = 1;
        }
        if (saw_newline) break;
    }
    out[written] = '\0';
    return (int)written;
}

int sys_gpt2_generate(const char* prompt, char* out, uint32_t max) {
    return sys_gpt2_generate_impl(prompt, out, max, 0U);
}

int sys_gpt2_gguf_generate(const char* prompt, char* out, uint32_t max) {
    return sys_gpt2_generate_impl(prompt, out, max, 1U);
}

int sys_gpt2_gguf_continue(char* out, uint32_t max) {
    return sys_gpt2_gguf_session_step(out, max);
}

// Cette fonction est maintenant obsolète pour l'entrée clavier
void syscall_add_input_char(char c) {
    (void)c;
}

void syscall_init() {
    gguf_session_active = 0U;
    gguf_session_token_count = 0U;
    gguf_session_prompt_tokens = 0U;
    gguf_session_rng = 0U;
    // Enregistre notre handler pour l'interruption 0x80
    register_interrupt_handler(0x80, (interrupt_handler_t)syscall_handler);
}

/* SYS_EXEC : cree l'enfant, le parent passe TASK_WAITING. Le handler
 * appelle schedule() depuis le cadre user. SYS_EXIT reveille le waiter. */
int sys_exec(const char* path, char* argv[]) {
    int capacity_rc;
    task_t* new_task;
    if (!current_task) return OS_TASK_NOT_FOUND;
    capacity_rc = task_can_create_child(current_task->id);
    if (capacity_rc != 0) return capacity_rc;
    capacity_rc = task_can_create_global();
    if (capacity_rc != 0) return capacity_rc;
    new_task = create_task_from_initrd_file(path);

    if (!new_task) {
        return -1;
    }

    if (argv) {
        char** argv_list = (char**)argv;
        const char* src = 0;
        if (argv_list[1]) src = argv_list[1];
        else if (argv_list[0]) src = argv_list[0];
        if (src) {
            char kbuf[256];
            int n = 0;
            while (n < 255 && src[n] != '\0') { kbuf[n] = src[n]; n++; }
            kbuf[n] = '\0';
            extern vmm_directory_t* current_directory;
            vmm_directory_t* old_dir = current_directory;
            vmm_switch_page_directory(new_task->vmm_dir->physical_addr);
            current_directory = new_task->vmm_dir;
            char* dst = (char*)(0xB0000000 - 512);
            for (int i = 0; i <= n; i++) dst[i] = kbuf[i];
            vmm_switch_page_directory(old_dir->physical_addr);
            current_directory = old_dir;
            new_task->cpu_state.ebx = (uint32_t)(0xB0000000 - 512);
        }
    }
    new_task->parent_pid = current_task ? current_task->id : -1;
    new_task->waiter_pid = current_task ? current_task->id : 0;
    return 0;
}

/* Cree la tache et retourne son pid. Le handler appelle schedule() pour
 * laisser tourner l'enfant jusqu'au prochain SYS_YIELD (cadre user, pas IRQ0). */
int sys_spawn(const char* path, char* argv[]) {
    int capacity_rc;
    task_t* new_task;
    if (!current_task) return OS_TASK_NOT_FOUND;
    capacity_rc = task_can_create_child(current_task->id);
    if (capacity_rc != 0) return capacity_rc;
    capacity_rc = task_can_create_global();
    if (capacity_rc != 0) return capacity_rc;
    new_task = create_task_from_initrd_file(path);
    if (!new_task) {
        return -1;
    }
    // Passer au moins un argument texte (preferer argv[1] si present)
    if (argv) {
        char** argv_list = (char**)argv;
        const char* src = 0;
        if (argv_list[1]) src = argv_list[1];
        else if (argv_list[0]) src = argv_list[0];
        if (src) {
            // Copier jusqu'a 255 octets
            char kbuf[256];
            int n = 0;
            while (n < 255 && src[n] != '\0') { kbuf[n] = src[n]; n++; }
            kbuf[n] = '\0';
            // Ecrire dans la pile utilisateur de la nouvelle tache (en haut - 512)
            vmm_directory_t* old_dir = current_directory;
            vmm_switch_page_directory(new_task->vmm_dir->physical_addr);
            current_directory = new_task->vmm_dir;
            char* dst = (char*)(0xB0000000 - 512);
            for (int i = 0; i <= n; i++) dst[i] = kbuf[i];
            // Restaurer
            vmm_switch_page_directory(old_dir->physical_addr);
            current_directory = old_dir;
            // Placer le pointeur dans EBX
            new_task->cpu_state.ebx = (uint32_t)(0xB0000000 - 512);
        }
    }
    new_task->parent_pid = current_task ? current_task->id : -1;
    return new_task->id;
}


// Implémentation de SYS_GETS - Lire une ligne complète depuis le clavier
void sys_gets(char* buffer, uint32_t size) {
    if (!buffer || size == 0) return;
    
    print_string_serial("SYS_GETS: Debut de la lecture (version corrigee)...\n");
    
    // Réactiver les interruptions
    asm volatile("sti");
    
    uint32_t i = 0;
    
    while (i < size - 1) {
        char c = keyboard_getc(); // Utilise directement keyboard_getc qui est plus robuste
        
        if (c == '\r' || c == '\n') {
            // Fin de ligne - afficher aussi sur écran
            print_char('\n', -1, -1, 0x0F);
            buffer[i] = '\0';
            print_string_serial("SYS_GETS: ligne lue: ");
            print_string_serial(buffer);
            print_string_serial("\n");
            return;
        }
        
        if (c == '\b' && i > 0) {
            // Backspace - effacer sur l'écran aussi
            i--;
            print_char('\b', -1, -1, 0x0F);  // Backspace
            print_char(' ', -1, -1, 0x0F);   // Espace
            print_char('\b', -1, -1, 0x0F);  // Backspace
        } else if (c >= 32 && c <= 126) {
            // Caractère imprimable - l'afficher sur l'écran
            buffer[i++] = c;
            print_char(c, -1, -1, 0x0F);
            print_string_serial("SYS_GETS: caractère ajouté: '");
            write_serial(c);
            print_string_serial("'\n");
        }
    }
    
    buffer[i] = '\0';
    print_string_serial("SYS_GETS: buffer plein, ligne lue: ");
    print_string_serial(buffer);
    print_string_serial("\n");
}

int sys_listdir(const char* path, os_dirent_t* out, int max_n) {
    int n;
    if (!path || !out || max_n <= 0) return -1;
    if (!overlay_is_dir(path) && !initrd_is_dir(path)) return -1;
    n = initrd_listdir(path, out, max_n);
    if (n < 0) n = 0;
    return overlay_listdir(path, out, n, max_n);
}

int sys_readfile(const char* path, char* buf, uint32_t max) {
    int n;
    if (!path || !buf || max == 0) return -1;
    n = overlay_read(path, buf, max);
    if (n >= 0) return n;
    if (n == OV_ERR_ISDIR) return n;
    return initrd_read_into(path, buf, max);
}

int sys_mkdir(const char* path) {
    if (!path) return -1;
    return overlay_mkdir(path);
}

int sys_unlink(const char* path) {
    if (!path) return -1;
    return overlay_unlink(path);
}

int sys_writefile(const char* path, const char* buf, uint32_t n) {
    if (!path || (n > 0 && !buf)) return -1;
    return overlay_write(path, buf, n);
}

int sys_stat(const char* path, os_dirent_t* out) {
    if (!path || !out) return -1;
    if (overlay_stat(path, out) == OV_OK) return 0;
    return initrd_stat(path, out);
}

int sys_rename(const char* oldpath, const char* newpath) {
    if (!oldpath || !newpath) return -1;
    return overlay_rename(oldpath, newpath);
}

int sys_copy(const char* src, const char* dst) {
    if (!src || !dst) return -1;
    return overlay_copy(src, dst);
}

int sys_append(const char* path, const char* buf, uint32_t n) {
    if (!path || (n > 0 && !buf)) return -1;
    return overlay_append(path, buf, n);
}

int sys_getpid(void) {
    if (!current_task) return -1;
    return current_task->id;
}

int sys_ps(os_proc_t* out, int max_n) {
    if (!out || max_n <= 0) return -1;
    return task_fill_ps(out, max_n);
}

int sys_kill(int pid) {
    int rc;
    if (!current_task) return OS_TASK_CONTROL_DENIED;
    rc = task_kill(current_task->id, pid);
    if (rc == 0) {
        service_notify_purge_pid(pid);
        service_registry_backend_remove_pid(pid);
        (void)service_registry_remove_watcher_pid(pid);
    }
    return rc;
}

uint32_t sys_ticks(void) {
    return timer_get_ticks();
}

int sys_meminfo(os_meminfo_t* info) {
    if (!info) return -1;
    info->total_pages = pmm_get_total_pages();
    info->used_pages = pmm_get_used_pages();
    info->free_pages = pmm_get_free_pages();
    return 0;
}

int sys_task_metrics(int pid, os_task_metrics_t* out) {
    if (!out || pid < 0) return OS_TASK_NOT_FOUND;
    return task_fill_metrics(pid, out);
}

int sys_task_set_priority(int pid, uint32_t priority) {
    if (!current_task || pid < 0) return OS_TASK_NOT_FOUND;
    return task_set_priority(current_task->id, pid, priority);
}

int sys_task_wait(int pid) {
    if (!current_task || pid < 0) return OS_TASK_NOT_FOUND;
    return task_wait_for_child(current_task->id, pid);
}

int sys_task_set_name(int pid, const char* name) {
    if (!current_task || pid < 0) return OS_TASK_NOT_FOUND;
    return task_set_name(current_task->id, pid, name);
}

int sys_task_capacity(os_task_capacity_t* out) {
    return task_fill_capacity(out);
}

int sys_task_child_result(int pid, os_task_exit_result_t* out) {
    if (!current_task || pid < 0) return OS_TASK_NOT_FOUND;
    return task_get_child_result(current_task->id, pid, out);
}

int sys_task_child_result_list(os_task_exit_history_t* out) {
    if (!current_task) return OS_TASK_NOT_FOUND;
    return task_fill_child_result_history(current_task->id, out);
}

int sys_task_child_result_ack(void) {
    if (!current_task) return OS_TASK_NOT_FOUND;
    return task_ack_child_result_history(current_task->id);
}

int sys_task_child_result_observe(uint32_t expected_generation,
                                  os_task_exit_history_observation_t* out) {
    if (!current_task) return OS_TASK_NOT_FOUND;
    return task_observe_child_result_history(current_task->id, expected_generation, out);
}

int sys_task_child_result_find(int pid, os_task_exit_result_t* out) {
    if (!current_task || pid < 0) return OS_TASK_NOT_FOUND;
    return task_find_child_result_history(current_task->id, pid, out);
}

int sys_task_child_result_forget(int pid) {
    if (!current_task || pid < 0) return OS_TASK_NOT_FOUND;
    return task_forget_child_result_history(current_task->id, pid);
}

int sys_task_suspend(int pid) {
    if (!current_task || pid < 0) return OS_TASK_NOT_FOUND;
    return task_suspend_child(current_task->id, pid);
}

int sys_task_resume(int pid) {
    if (!current_task || pid < 0) return OS_TASK_NOT_FOUND;
    return task_resume_child(current_task->id, pid);
}

int sys_task_kill_children(void) {
    if (!current_task) return OS_TASK_NOT_FOUND;
    return task_kill_direct_children(current_task->id);
}

int sys_task_children(os_task_children_t* out) {
    if (!current_task || !out) return OS_TASK_NOT_FOUND;
    return task_fill_direct_children(current_task->id, out);
}

int sys_task_wait_any(void) {
    if (!current_task) return OS_TASK_NOT_FOUND;
    return task_wait_for_any_child(current_task->id);
}

int sys_task_child_exit_count(os_task_child_exit_count_t* out) {
    if (!current_task || !out) return OS_TASK_NOT_FOUND;
    return task_get_child_exit_count(current_task->id, &out->count);
}

int sys_task_delegate_child(int child_pid, int supervisor_pid) {
    if (!current_task) return OS_TASK_NOT_FOUND;
    return task_delegate_child(current_task->id, child_pid, supervisor_pid);
}

int sys_task_supervision_events(os_task_supervision_events_t* out) {
    if (!current_task || !out) return OS_TASK_NOT_FOUND;
    return task_fill_supervision_events(current_task->id, out);
}

int sys_task_supervision_events_ack(void) {
    if (!current_task) return OS_TASK_NOT_FOUND;
    return task_ack_supervision_events(current_task->id);
}

int sys_task_supervision_events_observe(uint32_t expected_generation,
                                        os_task_supervision_events_observation_t* out) {
    if (!current_task || !out) return OS_TASK_NOT_FOUND;
    return task_observe_supervision_events(current_task->id, expected_generation, out);
}

int sys_task_supervision_event_find(uint32_t sequence, os_task_supervision_event_t* out) {
    if (!current_task || !out) return OS_TASK_NOT_FOUND;
    return task_find_supervision_event(current_task->id, sequence, out);
}

int sys_task_supervision_event_forget(uint32_t sequence) {
    if (!current_task) return OS_TASK_NOT_FOUND;
    return task_forget_supervision_event(current_task->id, sequence);
}

int sys_task_supervision_summary(os_task_supervision_summary_t* out) {
    if (!current_task || !out) return OS_TASK_NOT_FOUND;
    return task_fill_supervision_summary(current_task->id, out);
}
