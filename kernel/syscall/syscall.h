#ifndef SYSCALL_H
#define SYSCALL_H

#include <stdint.h>
#include "../task/task.h"
#include "os_syscalls.h"

typedef struct {
    uint32_t eax, ebx, ecx, edx, esi, edi;
} syscall_params_t;

void syscall_init();
void syscall_handler(cpu_state_t* cpu);

void sys_exit(uint32_t exit_code);
void sys_putc(char c);
char sys_getc();
void sys_puts(const char* str);
void sys_yield();

void sys_gets(char* buffer, uint32_t size);
int sys_exec(const char* path, char* argv[]);
int sys_spawn(const char* path, char* argv[]);

int sys_listdir(const char* path, os_dirent_t* out, int max_n);
int sys_readfile(const char* path, char* buf, uint32_t max);
int sys_getpid(void);
int sys_ps(os_proc_t* out, int max_n);
int sys_kill(int pid);
uint32_t sys_ticks(void);
int sys_meminfo(os_meminfo_t* info);
int sys_task_metrics(int pid, os_task_metrics_t* out);
int sys_task_set_priority(int pid, uint32_t priority);
int sys_task_wait(int pid);
int sys_task_set_name(int pid, const char* name);
int sys_task_capacity(os_task_capacity_t* out);
int sys_task_child_result(int pid, os_task_exit_result_t* out);
int sys_task_child_result_list(os_task_exit_history_t* out);
int sys_task_child_result_ack(void);
int sys_task_child_result_observe(uint32_t expected_generation,
                                  os_task_exit_history_observation_t* out);
int sys_task_child_result_find(int pid, os_task_exit_result_t* out);
int sys_task_child_result_forget(int pid);
int sys_task_suspend(int pid);
int sys_task_resume(int pid);
int sys_task_kill_children(void);
int sys_task_children(os_task_children_t* out);
int sys_task_wait_any(void);
int sys_task_child_exit_count(os_task_child_exit_count_t* out);
int sys_task_delegate_child(int child_pid, int supervisor_pid);
int sys_task_supervision_events(os_task_supervision_events_t* out);
int sys_task_supervision_events_ack(void);
int sys_task_supervision_events_observe(uint32_t expected_generation,
                                        os_task_supervision_events_observation_t* out);
int sys_task_supervision_event_find(uint32_t sequence, os_task_supervision_event_t* out);
int sys_task_supervision_event_forget(uint32_t sequence);
int sys_task_supervision_summary(os_task_supervision_summary_t* out);
int sys_task_supervision_notify(uint32_t enabled);
int sys_task_supervision_notify_filter(uint32_t mask);
int sys_task_supervision_notify_status(os_task_supervision_notify_status_t* out);
int sys_task_supervision_watch(int child_pid, uint32_t enabled);
int sys_task_supervision_watch_status(os_task_supervision_watch_status_t* out);
int sys_task_supervision_delivery_stats(os_task_supervision_delivery_stats_t* out);
int sys_task_supervision_delivery_stats_ack(void);
int sys_task_supervision_event_replay(uint32_t sequence);
int sys_task_supervision_priority(int child_pid);
int sys_task_supervision_priority_status(os_task_supervision_priority_status_t* out);
int sys_task_supervision_notify_budget(uint32_t limit);
int sys_task_supervision_notify_budget_status(os_task_supervision_notify_budget_status_t* out);
int sys_fat16_read(const char* name, char* buffer, uint32_t max);
int sys_fat16_list(os_fat16_dirent_t* out, uint32_t capacity);
int sys_fat16_list_page(os_fat16_dirent_t* out, uint32_t capacity, uint32_t start);
int sys_fat32_read(const char* name, char* buffer, uint32_t max);
int sys_fat32_list(os_fat16_dirent_t* out, uint32_t capacity);
int sys_fat32_list_page(os_fat16_dirent_t* out, uint32_t capacity, uint32_t start);
int sys_fat16_list_path(const char* path, os_fat16_dirent_t* out, uint32_t capacity,
                        uint32_t start);
int sys_fat32_list_path(const char* path, os_fat16_dirent_t* out, uint32_t capacity,
                        uint32_t start);
int sys_socket_open(uint16_t local_port, uint16_t remote_port, uint32_t local_sequence);
int sys_socket_listen(uint16_t local_port, uint32_t local_sequence);
int sys_socket_accept_syn(int socket_id, const os_socket_passive_view_t* view);
int sys_socket_build_syn_ack(int socket_id, uint8_t* segment, uint16_t capacity, uint16_t* out_length);
int sys_socket_accept_ack(int socket_id, const os_socket_passive_view_t* view);
int sys_socket_accept_syn_ack(int socket_id, const os_socket_syn_ack_t* view);
int sys_socket_send(const os_socket_send_request_t* request);
int sys_socket_feed(const os_socket_feed_request_t* request);
int sys_socket_receive(const os_socket_receive_request_t* request);
int sys_socket_close(int socket_id);
int sys_mkdir(const char* path);
int sys_unlink(const char* path);
int sys_writefile(const char* path, const char* buf, uint32_t n);
int sys_stat(const char* path, os_dirent_t* out);
int sys_rename(const char* oldpath, const char* newpath);
int sys_copy(const char* src, const char* dst);
int sys_append(const char* path, const char* buf, uint32_t n);
int sys_gpt2_generate(const char* prompt, char* out, uint32_t max);
int sys_gpt2_gguf_generate(const char* prompt, char* out, uint32_t max);
int sys_gpt2_gguf_continue(char* out, uint32_t max);
int sys_ipc_send(int target_pid, const os_ipc_payload_t* payload);
int sys_ipc_receive(os_ipc_message_t* out);
int sys_service_register(const char* name);
int sys_service_lookup(const char* name);
int sys_service_unregister(const char* name);
int sys_service_grant(const char* name, int target_pid);
int sys_service_backend_grant(const char* name, int target_pid);
int sys_service_backend_grant_scoped(const char* name, int target_pid, uint32_t rights);
int sys_service_backend_grant_scoped_source(const char* name, int target_pid, uint32_t rights,
                                            uint32_t sources);
int sys_service_backend_grant_scoped_source_prefix(const char* name, int target_pid,
                                                   uint32_t rights, uint32_t sources,
                                                   const char* prefix);
int sys_service_backend_revoke(const char* name, int target_pid);
/* Libère, pour le seul appelant Ring 3 courant, sa capacité sur le service nommé. */
int sys_service_backend_release(const char* name);
int sys_service_backend_status(const char* name, int target_pid, uint32_t* out_rights);
int sys_service_backend_scope_status(const char* name, int target_pid,
                                     os_service_backend_scope_t* out_scope);
int sys_service_backend_list(const char* name, os_service_backend_list_t* out_list);
int sys_service_backend_observe(const char* name, uint32_t expected_generation,
                                os_service_backend_snapshot_t* out_snapshot);
int sys_service_notify(const char* name);
int sys_service_status(const char* name, os_service_status_t* out);
int sys_vfs_backend_read(const char* path, char* buffer, uint32_t max);
int sys_vfs_backend_write(const char* path, const char* data, uint32_t size);
int sys_vfs_fat16_create(const char* name, const char* data, uint32_t size);
int sys_vfs_fat16_unlink(const char* name);
int sys_vfs_fat16_rename(const char* old_name, const char* new_name);
int sys_vfs_fat32_create(const char* name, const char* data, uint32_t size);
int sys_vfs_fat32_unlink(const char* name);
int sys_vfs_fat32_rename(const char* old_name, const char* new_name);
int sys_vfs_initrd_read(const char* path, char* buffer, uint32_t max);
int sys_vfs_overlay_read(const char* path, char* buffer, uint32_t max);
int sys_vfs_overlay_unlink(const char* path);
int sys_vfs_overlay_rename(const char* oldpath, const char* newpath);
int sys_vfs_initrd_stat(const char* path, os_dirent_t* out);
int sys_vfs_overlay_stat(const char* path, os_dirent_t* out);
int sys_vfs_initrd_listdir(const char* path, os_dirent_t* out, int max_n);
int sys_vfs_overlay_listdir(const char* path, os_dirent_t* out, int max_n);
int sys_vfs_initrd_listdir_page(const char* path, os_dirent_t* out, uint32_t start);
int sys_vfs_overlay_listdir_page(const char* path, os_dirent_t* out, uint32_t start);
int sys_vfs_overlay_mkdir(const char* path);
int sys_vfs_overlay_rmdir(const char* path);

void keyboard_add_char_to_buffer(char c);

#endif
