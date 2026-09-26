#ifndef SERVICE_REGISTRY_H
#define SERVICE_REGISTRY_H

#include <stdint.h>
#include "os_syscalls.h"

#define SERVICE_REGISTRY_CAPACITY 8U
#define SERVICE_REGISTRY_WATCH_CAPACITY 8U
#define SERVICE_REGISTRY_BACKEND_CAPACITY OS_SERVICE_BACKEND_CAPACITY
#define SERVICE_BACKEND_RIGHT_READ 1U
#define SERVICE_BACKEND_RIGHT_MUTATE 2U
#define SERVICE_BACKEND_RIGHT_ALL (SERVICE_BACKEND_RIGHT_READ | SERVICE_BACKEND_RIGHT_MUTATE)

typedef struct {
    int32_t pid;
    char name[OS_SERVICE_NAME_MAX];
    uint32_t backend_generation;
} service_registry_entry_t;

typedef struct {
    int32_t pid;
    char name[OS_SERVICE_NAME_MAX];
} service_registry_watch_t;

void service_registry_init(void);
int service_registry_register(const char* name, int32_t pid);
int service_registry_lookup(const char* name);
int service_registry_remove(const char* name, int32_t pid);
int service_registry_grant(const char* name, int32_t owner_pid, int32_t grantee_pid);
int service_registry_remove_pid(int32_t pid);
#define SERVICE_REGISTRY_NOTIFY_HISTORY_CAPACITY 8U

typedef struct {
    uint32_t sequence;
    int32_t watcher_pid;
    char name[OS_SERVICE_NAME_MAX];
    int32_t old_pid;
    int32_t new_pid;
    uint32_t reason;
    uint8_t acked;
} service_registry_notify_event_t;

int service_registry_subscribe(const char* name, int32_t pid);
int service_registry_collect_watchers(const char* name, int32_t* out, uint32_t max);
int service_registry_remove_watcher_pid(int32_t pid);
int service_registry_notify_record(const char* name, int32_t watcher_pid, int32_t old_pid, int32_t new_pid, uint32_t reason, uint32_t* out_sequence);
int service_registry_notify_ack(int32_t watcher_pid, uint32_t sequence);
int service_registry_notify_history_count(int32_t watcher_pid, uint32_t* out_acked, uint32_t* out_unacked);
int service_registry_collect_owned(int32_t pid, service_registry_entry_t* out, uint32_t max);
int service_registry_pid_is_owner(int32_t pid);
int service_registry_name_valid(const char* name);
int service_registry_backend_grant(const char* name, int32_t owner_pid, int32_t grantee_pid);
int service_registry_backend_grant_scoped(const char* name, int32_t owner_pid, int32_t grantee_pid, uint32_t rights);
int service_registry_backend_grant_scoped_source(const char* name, int32_t owner_pid, int32_t grantee_pid,
                                                 uint32_t rights, uint32_t sources);
/* `prefix` est relatif à la source ; la chaîne vide désigne sa racine entière. */
int service_registry_backend_grant_scoped_source_prefix(const char* name, int32_t owner_pid,
                                                        int32_t grantee_pid, uint32_t rights,
                                                        uint32_t sources, const char* prefix);
int service_registry_backend_revoke(const char* name, int32_t owner_pid, int32_t grantee_pid);
/* Libération autonome : seul le PID actuellement porteur peut retirer sa propre capacité. */
int service_registry_backend_release(const char* name, int32_t grantee_pid);
int service_registry_backend_rights(const char* name, int32_t owner_pid, int32_t grantee_pid, uint32_t* out_rights);
int service_registry_backend_scope(const char* name, int32_t owner_pid, int32_t grantee_pid,
                                   os_service_backend_scope_t* out_scope);
int service_registry_backend_list(const char* name, int32_t owner_pid, os_service_backend_list_t* out_list);
int service_registry_backend_observe(const char* name, int32_t owner_pid, uint32_t expected_generation,
                                     os_service_backend_snapshot_t* out_snapshot);
int service_registry_backend_allowed(const char* name, int32_t pid);
int service_registry_backend_allowed_for(const char* name, int32_t pid, uint32_t right);
int service_registry_backend_allowed_for_source(const char* name, int32_t pid, uint32_t right,
                                                uint32_t source);
/* Une voie sans chemin ne passe que sous le préfixe racine. */
int service_registry_backend_allowed_for_source_path(const char* name, int32_t pid, uint32_t right,
                                                     uint32_t source, const char* path);
/* AOS-2172/2173/2174: owner bypass for any valid backend scope only when vfs-virtual absent. */
int service_registry_owner_bypasses_backend(const char* name, int32_t pid, uint32_t source);
/* AOS-2175: ATA-backed overlay read/stat only exercisable by live vfs-virtual worker. */
int service_registry_ata_overlay_io_via_worker(int32_t pid);
/* AOS-2177: historical SYS_READFILE decision (overlay_hit = path exists in overlay). */
#define SERVICE_HIST_READ_DENIED          0
#define SERVICE_HIST_READ_FULL            1
#define SERVICE_HIST_READ_INITRD_ONLY     2
#define SERVICE_HIST_READ_WORKER_REQUIRED 3
int service_registry_historical_read_decision(int32_t pid, int overlay_hit);
/* Network driver isolation: raw network and socket I/O exercisable by live net-driver worker. */
int service_registry_net_io_via_worker(int32_t pid);
/* Tranche 5: NIC/socket/LLM-network/peer syscall classification and gate. */
int service_registry_net_syscall_gated(uint32_t syscall_number);
int service_registry_net_syscall_allowed(int32_t pid, uint32_t syscall_number);
void service_registry_backend_remove_name(const char* name);
void service_registry_backend_remove_pid(int32_t pid);

#endif
