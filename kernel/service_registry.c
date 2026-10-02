#include "service_registry.h"
#include "kernel/task/task.h"
#include "ata.h"

static service_registry_entry_t service_entries[SERVICE_REGISTRY_CAPACITY];
static service_registry_watch_t service_watches[SERVICE_REGISTRY_WATCH_CAPACITY];
static service_registry_notify_event_t service_notify_history[SERVICE_REGISTRY_NOTIFY_HISTORY_CAPACITY];
static uint32_t service_notify_seq_counter = 0U;

typedef struct {
    int32_t owner_pid;
    uint32_t owner_sequence;
    uint32_t owner_generation;
    int32_t grantee_pid;
    uint32_t grantee_sequence;
    uint32_t grantee_generation;
    uint32_t rights;
    uint32_t sources;
    char prefix[OS_SERVICE_BACKEND_PREFIX_MAX];
    char name[OS_SERVICE_NAME_MAX];
    uint32_t token;
    uint32_t identity_key;
} service_backend_cap_t;
static service_backend_cap_t service_backend_caps[SERVICE_REGISTRY_BACKEND_CAPACITY];
static uint32_t service_backend_token_next = 1U;

static service_registry_persistent_mount_t persistent_mounts[SERVICE_REGISTRY_PERSISTENT_MOUNT_CAPACITY];

static int mount_insert(const char* service_name, const char* prefix, uint32_t source);
static int mount_journal_load(void);
static void event_journal_flush(void);
static int event_journal_load(void);
static void event_journal_bind(const char* name, int32_t pid);
static void ipc_spill_clear_pid(int32_t pid);
static void ipc_spill_reset(void);
static uint32_t mint_right_token(void);
static int identity_key_bound(int32_t pid, uint32_t stored);

static int task_identity_valid(int32_t pid, uint32_t sequence, uint32_t generation) {
    uint32_t cur_seq = 0U, cur_gen = 0U;
    if (pid <= 0) return 0;
    if (task_get_identity(pid, &cur_seq, &cur_gen) == 0) {
        if (sequence != 0U || generation != 0U) {
            return (cur_seq == sequence && cur_gen == generation);
        }
        return 1;
    }
    if (sequence == 0U && generation == 0U) return 1;
    return 0;
}

static int name_equal(const char* left, const char* right) {
    uint32_t i;
    for (i = 0U; i < OS_SERVICE_NAME_MAX; i++) {
        if (left[i] != right[i]) return 0;
        if (left[i] == '\0') return 1;
    }
    return 0;
}

static void service_registry_backend_generation_bump(const char* name) {
    uint32_t i;
    for (i = 0U; i < SERVICE_REGISTRY_CAPACITY; i++) {
        if (service_entries[i].pid > 0 && name_equal(service_entries[i].name, name)) {
            service_entries[i].backend_generation++;
            if (service_entries[i].backend_generation == 0U) service_entries[i].backend_generation = 1U;
            return;
        }
    }
}

static int service_registry_backend_sources_valid(uint32_t sources) {
    return sources != 0U && (sources & ~OS_SERVICE_BACKEND_SOURCE_ALL) == 0U;
}

/* Un préfixe est relatif, canonique et représente un répertoire. La chaîne
 * vide préserve les grants historiques de source entière. */
static int service_registry_backend_prefix_valid(const char* prefix) {
    uint32_t i = 0U;
    if (!prefix) return 0;
    if (prefix[0] == '\0') return 1;
    while (i < OS_SERVICE_BACKEND_PREFIX_MAX) {
        char c = prefix[i];
        if (c == '\0') return i > 0U && prefix[i - 1U] == '/';
        if (c == '/' && (i == 0U || prefix[i - 1U] == '/')) return 0;
        if (c == '.' && i + 1U < OS_SERVICE_BACKEND_PREFIX_MAX && prefix[i + 1U] == '.') return 0;
        i++;
    }
    return 0;
}

static int service_registry_backend_prefix_equal(const char* left, const char* right) {
    uint32_t i;
    if (!left || !right) return 0;
    for (i = 0U; i < OS_SERVICE_BACKEND_PREFIX_MAX; i++) {
        if (left[i] != right[i]) return 0;
        if (left[i] == '\0') return 1;
    }
    return 0;
}

static int service_registry_backend_prefix_matches(const char* prefix, const char* path) {
    uint32_t i = 0U;
    if (!prefix) return 0;
    if (prefix[0] == '\0') return 1;
    if (!path) return 0;
    while (prefix[i] != '\0') {
        if (path[i] == '\0' || path[i] != prefix[i]) return 0;
        i++;
    }
    return 1;
}

static uint32_t service_registry_backend_generation_of(const char* name) {
    uint32_t i;
    for (i = 0U; i < SERVICE_REGISTRY_CAPACITY; i++) {
        if (service_entries[i].pid > 0 && name_equal(service_entries[i].name, name)) {
            return service_entries[i].backend_generation;
        }
    }
    return 0U;
}

static void copy_name(char* destination, const char* source) {
    uint32_t i;
    for (i = 0U; i < OS_SERVICE_NAME_MAX; i++) {
        destination[i] = source[i];
        if (source[i] == '\0') {
            for (i++; i < OS_SERVICE_NAME_MAX; i++) destination[i] = '\0';
            return;
        }
    }
}

static void service_registry_backend_copy_prefix(char* destination, const char* source) {
    uint32_t i;
    for (i = 0U; i < OS_SERVICE_BACKEND_PREFIX_MAX; i++) {
        destination[i] = source[i];
        if (source[i] == '\0') {
            for (i++; i < OS_SERVICE_BACKEND_PREFIX_MAX; i++) destination[i] = '\0';
            return;
        }
    }
}

int service_registry_name_valid(const char* name) {
    uint32_t i;
    if (!name || name[0] == '\0') return 0;
    for (i = 0U; i < OS_SERVICE_NAME_MAX; i++) {
        char c = name[i];
        if (c == '\0') return 1;
        if (!((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
              (c >= '0' && c <= '9') || c == '_' || c == '-')) return 0;
    }
    return 0;
}

void service_registry_init(void) {
    uint32_t i;
    for (i = 0U; i < SERVICE_REGISTRY_CAPACITY; i++) {
        service_entries[i].pid = 0;
        service_entries[i].sequence = 0U;
        service_entries[i].generation = 0U;
        service_entries[i].name[0] = '\0';
        service_entries[i].backend_generation = 1U;
        service_entries[i].right_token = 0U;
        service_entries[i].identity_key = 0U;
    }
    for (i = 0U; i < SERVICE_REGISTRY_WATCH_CAPACITY; i++) {
        service_watches[i].pid = 0;
        service_watches[i].sequence = 0U;
        service_watches[i].generation = 0U;
        service_watches[i].name[0] = '\0';
    }
    for (i = 0U; i < SERVICE_REGISTRY_NOTIFY_HISTORY_CAPACITY; i++) {
        service_notify_history[i].sequence = 0U;
        service_notify_history[i].watcher_pid = 0;
        service_notify_history[i].name[0] = '\0';
        service_notify_history[i].old_pid = 0;
        service_notify_history[i].new_pid = 0;
        service_notify_history[i].reason = 0U;
        service_notify_history[i].acked = 0U;
        service_notify_history[i].replayed = 0U;
    }
    service_notify_seq_counter = 0U;
    for (i = 0U; i < SERVICE_REGISTRY_BACKEND_CAPACITY; i++) {
        service_backend_caps[i].owner_pid = 0;
        service_backend_caps[i].owner_sequence = 0U;
        service_backend_caps[i].owner_generation = 0U;
        service_backend_caps[i].grantee_pid = 0;
        service_backend_caps[i].grantee_sequence = 0U;
        service_backend_caps[i].grantee_generation = 0U;
        service_backend_caps[i].rights = 0U;
        service_backend_caps[i].sources = 0U;
        service_backend_caps[i].prefix[0] = '\0';
        service_backend_caps[i].name[0] = '\0';
        service_backend_caps[i].token = 0U;
        service_backend_caps[i].identity_key = 0U;
    }
    service_backend_token_next = 1U;
    for (i = 0U; i < SERVICE_REGISTRY_PERSISTENT_MOUNT_CAPACITY; i++) {
        persistent_mounts[i].active = 0U;
        persistent_mounts[i].service_name[0] = '\0';
        persistent_mounts[i].prefix[0] = '\0';
        persistent_mounts[i].source = 0U;
    }
    /* Defauts seulement si le journal disque est absent ou illisible.
     * L'ajout par defaut n'ecrit pas le disque : un reboot sans montage
     * custom retrouve ces quatre entrees, pas un journal vide. */
    if (mount_journal_load() != 0) {
        (void)mount_insert("vfs", "initrd/", OS_SERVICE_BACKEND_SOURCE_INITRD);
        (void)mount_insert("vfs", "overlay/", OS_SERVICE_BACKEND_SOURCE_OVERLAY);
        (void)mount_insert("vfs", "fat16/", OS_SERVICE_BACKEND_SOURCE_FAT16);
        (void)mount_insert("vfs", "fat32/", OS_SERVICE_BACKEND_SOURCE_FAT32);
    }
    (void)event_journal_load();
    ipc_spill_reset();
}

static uint32_t mint_right_token(void) {
    uint32_t token = service_backend_token_next;
    service_backend_token_next++;
    if (service_backend_token_next == 0U) service_backend_token_next = 1U;
    if (token == 0U) token = 1U;
    return token;
}

/* Sans tache (tests unitaires sur un PID nu), une cle stockee a 0 reste
 * acceptee. Une tache vivante doit presenter exactement sa cle, non nulle. */
static int identity_key_bound(int32_t pid, uint32_t stored) {
    uint32_t live = 0U;
    if (task_identity_key(pid, &live) != 0) return stored == 0U;
    return live != 0U && stored == live;
}

int service_registry_register(const char* name, int32_t pid) {
    uint32_t i;
    int free_slot = -1;
    uint32_t seq = 0U, gen = 0U;
    if (!service_registry_name_valid(name) || pid <= 0) return OS_SERVICE_BAD_NAME;
    (void)task_get_identity(pid, &seq, &gen);
    for (i = 0U; i < SERVICE_REGISTRY_CAPACITY; i++) {
        if (service_entries[i].pid == 0) {
            if (free_slot < 0) free_slot = (int)i;
            continue;
        }
        if (name_equal(service_entries[i].name, name)) {
            if (service_entries[i].pid == pid &&
                task_identity_valid(pid, service_entries[i].sequence, service_entries[i].generation)) {
                return 0;
            }
            if (!task_identity_valid(service_entries[i].pid, service_entries[i].sequence, service_entries[i].generation)) {
                free_slot = (int)i;
                break;
            }
            return OS_SERVICE_TAKEN;
        }
    }
    if (free_slot < 0) return OS_SERVICE_FULL;
    service_entries[free_slot].pid = pid;
    service_entries[free_slot].sequence = seq;
    service_entries[free_slot].generation = gen;
    copy_name(service_entries[free_slot].name, name);
    service_entries[free_slot].backend_generation = 1U;
    service_entries[free_slot].right_token = mint_right_token();
    {
        uint32_t key = 0U;
        if (task_identity_key(pid, &key) != 0) key = 0U;
        service_entries[free_slot].identity_key = key;
    }
    (void)service_registry_persistent_mount_restore(name);
    return 0;
}

int service_registry_lookup(const char* name) {
    uint32_t i;
    if (!service_registry_name_valid(name)) return OS_SERVICE_BAD_NAME;
    for (i = 0U; i < SERVICE_REGISTRY_CAPACITY; i++) {
        if (service_entries[i].pid > 0 && name_equal(service_entries[i].name, name)) {
            if (task_identity_valid(service_entries[i].pid, service_entries[i].sequence, service_entries[i].generation)) {
                return service_entries[i].pid;
            } else {
                service_entries[i].pid = 0;
                service_entries[i].sequence = 0U;
                service_entries[i].generation = 0U;
                service_entries[i].name[0] = '\0';
                service_registry_backend_remove_name(name);
                return OS_SERVICE_NOT_FOUND;
            }
        }
    }
    return OS_SERVICE_NOT_FOUND;
}

int service_registry_remove(const char* name, int32_t pid) {
    uint32_t i;
    if (!service_registry_name_valid(name) || pid <= 0) return OS_SERVICE_BAD_NAME;
    for (i = 0U; i < SERVICE_REGISTRY_CAPACITY; i++) {
        if (service_entries[i].pid == pid && name_equal(service_entries[i].name, name)) {
            if (task_identity_valid(pid, service_entries[i].sequence, service_entries[i].generation)) {
                service_entries[i].pid = 0;
                service_entries[i].sequence = 0U;
                service_entries[i].generation = 0U;
                service_entries[i].name[0] = '\0';
                service_registry_backend_remove_name(name);
                return 0;
            }
        }
    }
    return OS_SERVICE_NOT_FOUND;
}

int service_registry_grant(const char* name, int32_t owner_pid, int32_t grantee_pid) {
    uint32_t i;
    uint32_t g_seq = 0U, g_gen = 0U;
    if (!service_registry_name_valid(name) || owner_pid <= 0) return OS_SERVICE_BAD_NAME;
    if (grantee_pid <= 0) return OS_SERVICE_BAD_GRANTEE;
    (void)task_get_identity(grantee_pid, &g_seq, &g_gen);
    for (i = 0U; i < SERVICE_REGISTRY_CAPACITY; i++) {
        if (name_equal(service_entries[i].name, name)) {
            if (service_entries[i].pid != owner_pid ||
                !task_identity_valid(owner_pid, service_entries[i].sequence, service_entries[i].generation)) {
                return OS_SERVICE_NOT_OWNER;
            }
            service_entries[i].pid = grantee_pid;
            service_entries[i].sequence = g_seq;
            service_entries[i].generation = g_gen;
            {
                uint32_t key = 0U;
                if (task_identity_key(grantee_pid, &key) != 0) key = 0U;
                service_entries[i].identity_key = key;
            }
            service_registry_backend_remove_name(name);
            /* AOS-2174: with vfs-virtual live, vfs name handoff keeps the
             * ATA-backed generic backend behind an explicit SOURCE_ALL grant
             * (owner bypass for SOURCE_ALL is closed). */
            if (name_equal(name, "vfs") && service_registry_lookup("vfs-virtual") > 0) {
                (void)service_registry_backend_grant_scoped_source(
                    name, grantee_pid, grantee_pid, SERVICE_BACKEND_RIGHT_ALL,
                    OS_SERVICE_BACKEND_SOURCE_ALL);
            }
            return 0;
        }
    }
    return OS_SERVICE_NOT_FOUND;
}

int service_registry_collect_owned(int32_t pid, service_registry_entry_t* out, uint32_t max) {
    uint32_t i;
    uint32_t count = 0U;
    if (pid <= 0 || (!out && max > 0U)) return OS_SERVICE_NOT_FOUND;
    for (i = 0U; i < SERVICE_REGISTRY_CAPACITY; i++) {
        if (service_entries[i].pid == pid &&
            task_identity_valid(pid, service_entries[i].sequence, service_entries[i].generation)) {
            if (count < max) {
                out[count] = service_entries[i];
            }
            count++;
        }
    }
    return (int)count;
}

int service_registry_pid_is_owner(int32_t pid) {
    uint32_t i;
    if (pid <= 0) return 0;
    for (i = 0U; i < SERVICE_REGISTRY_CAPACITY; i++) {
        if (service_entries[i].pid == pid &&
            task_identity_valid(pid, service_entries[i].sequence, service_entries[i].generation)) return 1;
    }
    return 0;
}

int service_registry_remove_pid(int32_t pid) {
    uint32_t i;
    int removed = 0;
    if (pid <= 0) return OS_SERVICE_NOT_FOUND;
    service_registry_backend_remove_pid(pid);
    for (i = 0U; i < SERVICE_REGISTRY_CAPACITY; i++) {
        if (service_entries[i].pid == pid) {
            service_entries[i].pid = 0;
            service_entries[i].name[0] = '\0';
            service_entries[i].right_token = 0U;
            service_entries[i].identity_key = 0U;
            removed++;
        }
    }
    for (i = 0U; i < SERVICE_REGISTRY_NOTIFY_HISTORY_CAPACITY; i++) {
        if (service_notify_history[i].watcher_pid == pid) {
            service_notify_history[i].sequence = 0U;
            service_notify_history[i].watcher_pid = 0;
            service_notify_history[i].name[0] = '\0';
            service_notify_history[i].acked = 0U;
        }
    }
    ipc_spill_clear_pid(pid);
    return removed > 0 ? 0 : OS_SERVICE_NOT_FOUND;
}

int service_registry_notify_record(const char* name, int32_t watcher_pid, int32_t old_pid, int32_t new_pid, uint32_t reason, uint32_t* out_sequence) {
    uint32_t i;
    int slot = -1;
    if (!service_registry_name_valid(name) || watcher_pid <= 0) return OS_SERVICE_BAD_NAME;
    service_notify_seq_counter++;
    if (service_notify_seq_counter == 0U) service_notify_seq_counter = 1U;

    for (i = 0U; i < SERVICE_REGISTRY_NOTIFY_HISTORY_CAPACITY; i++) {
        if (service_notify_history[i].watcher_pid == 0) {
            slot = (int)i;
            break;
        }
    }
    if (slot < 0) {
        slot = (int)(service_notify_seq_counter % SERVICE_REGISTRY_NOTIFY_HISTORY_CAPACITY);
    }
    service_notify_history[slot].sequence = service_notify_seq_counter;
    service_notify_history[slot].watcher_pid = watcher_pid;
    copy_name(service_notify_history[slot].name, name);
    service_notify_history[slot].old_pid = old_pid;
    service_notify_history[slot].new_pid = new_pid;
    service_notify_history[slot].reason = reason;
    service_notify_history[slot].acked = 0U;
    service_notify_history[slot].replayed = 0U;
    if (out_sequence) *out_sequence = service_notify_seq_counter;
    event_journal_flush();
    return 0;
}

int service_registry_notify_pull(int32_t watcher_pid, service_registry_notify_event_t* out) {
    uint32_t i;
    uint32_t best = 0U;
    int found = -1;
    if (watcher_pid <= 0 || !out) return OS_SERVICE_BAD_NAME;
    for (i = 0U; i < SERVICE_REGISTRY_NOTIFY_HISTORY_CAPACITY; i++) {
        if (service_notify_history[i].watcher_pid == watcher_pid &&
            service_notify_history[i].sequence > 0U &&
            service_notify_history[i].acked == 0U) {
            if (found < 0 || service_notify_history[i].sequence < best) {
                found = (int)i;
                best = service_notify_history[i].sequence;
            }
        }
    }
    if (found < 0) return OS_SERVICE_NOT_FOUND;
    *out = service_notify_history[found];
    service_notify_history[found].acked = 1U;
    event_journal_flush();
    return 0;
}

int service_registry_notify_ack(int32_t watcher_pid, uint32_t sequence) {
    uint32_t i;
    if (watcher_pid <= 0 || sequence == 0U) return OS_SERVICE_BAD_NAME;
    for (i = 0U; i < SERVICE_REGISTRY_NOTIFY_HISTORY_CAPACITY; i++) {
        if (service_notify_history[i].watcher_pid == watcher_pid &&
            service_notify_history[i].sequence == sequence) {
            service_notify_history[i].acked = 1U;
            event_journal_flush();
            return 0;
        }
    }
    return OS_SERVICE_NOT_FOUND;
}

int service_registry_notify_history_count(int32_t watcher_pid, uint32_t* out_acked, uint32_t* out_unacked) {
    uint32_t i;
    uint32_t acked = 0U, unacked = 0U;
    if (watcher_pid <= 0) return OS_SERVICE_BAD_NAME;
    for (i = 0U; i < SERVICE_REGISTRY_NOTIFY_HISTORY_CAPACITY; i++) {
        if (service_notify_history[i].watcher_pid == watcher_pid &&
            service_notify_history[i].sequence > 0U) {
            if (service_notify_history[i].acked) acked++;
            else unacked++;
        }
    }
    if (out_acked) *out_acked = acked;
    if (out_unacked) *out_unacked = unacked;
    return 0;
}

int service_registry_notify_replay(int32_t watcher_pid, uint32_t sequence) {
    uint32_t i;
    if (watcher_pid <= 0 || sequence == 0U) return OS_SERVICE_BAD_NAME;
    for (i = 0U; i < SERVICE_REGISTRY_NOTIFY_HISTORY_CAPACITY; i++) {
        if (service_notify_history[i].watcher_pid == watcher_pid &&
            service_notify_history[i].sequence == sequence) {
            if (service_notify_history[i].acked) {
                return OS_SERVICE_STALE;
            }
            if (service_notify_history[i].replayed) {
                return OS_SERVICE_FULL;
            }
            service_notify_history[i].replayed = 1U;
            return 0;
        }
    }
    return OS_SERVICE_NOT_FOUND;
}

int service_registry_notify_is_acked(int32_t watcher_pid, uint32_t sequence) {
    uint32_t i;
    if (watcher_pid <= 0 || sequence == 0U) return OS_SERVICE_BAD_NAME;
    for (i = 0U; i < SERVICE_REGISTRY_NOTIFY_HISTORY_CAPACITY; i++) {
        if (service_notify_history[i].watcher_pid == watcher_pid &&
            service_notify_history[i].sequence == sequence) {
            return service_notify_history[i].acked ? 1 : 0;
        }
    }
    return OS_SERVICE_NOT_FOUND;
}

static int mount_insert(const char* service_name, const char* prefix, uint32_t source) {
    uint32_t i;
    int free_slot = -1;
    if (!service_registry_name_valid(service_name) || !prefix || prefix[0] == '\0') return OS_SERVICE_BAD_NAME;
    if (!service_registry_backend_prefix_valid(prefix)) return OS_SERVICE_BAD_NAME;
    for (i = 0U; i < SERVICE_REGISTRY_PERSISTENT_MOUNT_CAPACITY; i++) {
        if (persistent_mounts[i].active &&
            name_equal(persistent_mounts[i].service_name, service_name) &&
            service_registry_backend_prefix_equal(persistent_mounts[i].prefix, prefix)) {
            persistent_mounts[i].source = source;
            return 0;
        }
        if (!persistent_mounts[i].active && free_slot < 0) free_slot = (int)i;
    }
    if (free_slot < 0) return OS_SERVICE_FULL;
    copy_name(persistent_mounts[free_slot].service_name, service_name);
    service_registry_backend_copy_prefix(persistent_mounts[free_slot].prefix, prefix);
    persistent_mounts[free_slot].source = source;
    persistent_mounts[free_slot].active = 1U;
    return 0;
}

#define MOUNT_JOURNAL_MAGIC 0x4A544E4DU
#define MOUNT_JOURNAL_VERSION 1U
#define MOUNT_JOURNAL_LBA 4222U
#define MOUNT_JOURNAL_BYTES 1024U

typedef struct __attribute__((packed)) {
    uint32_t magic;
    uint32_t version;
    uint32_t count;
    uint32_t checksum;
} mount_journal_header_t;

typedef struct __attribute__((packed)) {
    char service_name[OS_SERVICE_NAME_MAX];
    char prefix[OS_SERVICE_BACKEND_PREFIX_MAX];
    uint32_t source;
    uint8_t active;
} mount_journal_entry_t;

static uint8_t mount_journal_buf[MOUNT_JOURNAL_BYTES];

static uint32_t mount_journal_checksum(const uint8_t* data, uint32_t length) {
    uint32_t i;
    uint32_t sum = 0U;
    for (i = 0U; i < length; i++) sum += data[i];
    return sum;
}

int service_registry_mount_journal_export(uint8_t* buf, uint32_t cap) {
    mount_journal_header_t header;
    uint32_t i;
    uint32_t count = 0U;
    uint32_t need;
    uint8_t* cursor;
    if (!buf) return OS_SERVICE_BAD_NAME;
    for (i = 0U; i < SERVICE_REGISTRY_PERSISTENT_MOUNT_CAPACITY; i++) {
        if (persistent_mounts[i].active) count++;
    }
    need = (uint32_t)sizeof(header) + count * (uint32_t)sizeof(mount_journal_entry_t);
    if (cap < need || need > MOUNT_JOURNAL_BYTES) return OS_SERVICE_FULL;
    for (i = 0U; i < cap; i++) buf[i] = 0U;
    cursor = buf + sizeof(header);
    for (i = 0U; i < SERVICE_REGISTRY_PERSISTENT_MOUNT_CAPACITY; i++) {
        mount_journal_entry_t entry;
        uint32_t b;
        if (!persistent_mounts[i].active) continue;
        for (b = 0U; b < sizeof(entry); b++) ((uint8_t*)&entry)[b] = 0U;
        copy_name(entry.service_name, persistent_mounts[i].service_name);
        service_registry_backend_copy_prefix(entry.prefix, persistent_mounts[i].prefix);
        entry.source = persistent_mounts[i].source;
        entry.active = 1U;
        for (b = 0U; b < sizeof(entry); b++) cursor[b] = ((uint8_t*)&entry)[b];
        cursor += sizeof(entry);
    }
    header.magic = MOUNT_JOURNAL_MAGIC;
    header.version = MOUNT_JOURNAL_VERSION;
    header.count = count;
    header.checksum = mount_journal_checksum(buf + sizeof(header), count * (uint32_t)sizeof(mount_journal_entry_t));
    for (i = 0U; i < sizeof(header); i++) buf[i] = ((uint8_t*)&header)[i];
    return 0;
}

int service_registry_mount_journal_import(const uint8_t* buf, uint32_t len) {
    mount_journal_header_t header;
    uint32_t i;
    uint32_t need;
    const uint8_t* cursor;
    if (!buf || len < sizeof(header)) return OS_SERVICE_BAD_NAME;
    for (i = 0U; i < sizeof(header); i++) ((uint8_t*)&header)[i] = buf[i];
    if (header.magic != MOUNT_JOURNAL_MAGIC || header.version != MOUNT_JOURNAL_VERSION) return OS_SERVICE_NOT_FOUND;
    if (header.count == 0U || header.count > SERVICE_REGISTRY_PERSISTENT_MOUNT_CAPACITY) return OS_SERVICE_NOT_FOUND;
    need = (uint32_t)sizeof(header) + header.count * (uint32_t)sizeof(mount_journal_entry_t);
    if (len < need) return OS_SERVICE_NOT_FOUND;
    if (header.checksum != mount_journal_checksum(buf + sizeof(header),
                                                  header.count * (uint32_t)sizeof(mount_journal_entry_t))) {
        return OS_SERVICE_NOT_FOUND;
    }
    for (i = 0U; i < SERVICE_REGISTRY_PERSISTENT_MOUNT_CAPACITY; i++) {
        persistent_mounts[i].active = 0U;
        persistent_mounts[i].service_name[0] = '\0';
        persistent_mounts[i].prefix[0] = '\0';
        persistent_mounts[i].source = 0U;
    }
    cursor = buf + sizeof(header);
    for (i = 0U; i < header.count; i++) {
        mount_journal_entry_t entry;
        uint32_t b;
        for (b = 0U; b < sizeof(entry); b++) ((uint8_t*)&entry)[b] = cursor[b];
        cursor += sizeof(entry);
        if (!entry.active) continue;
        entry.service_name[OS_SERVICE_NAME_MAX - 1U] = '\0';
        entry.prefix[OS_SERVICE_BACKEND_PREFIX_MAX - 1U] = '\0';
        if (mount_insert(entry.service_name, entry.prefix, entry.source) != 0) return OS_SERVICE_FULL;
    }
    return 0;
}

static void mount_journal_flush(void) {
    if (service_registry_mount_journal_export(mount_journal_buf, MOUNT_JOURNAL_BYTES) != 0) return;
    (void)ata_write_sectors(MOUNT_JOURNAL_LBA, MOUNT_JOURNAL_BYTES / 512U, mount_journal_buf);
}

static int mount_journal_load(void) {
    if (ata_read_sectors(MOUNT_JOURNAL_LBA, MOUNT_JOURNAL_BYTES / 512U, mount_journal_buf) != 0) return -1;
    return service_registry_mount_journal_import(mount_journal_buf, MOUNT_JOURNAL_BYTES);
}

#define EVENT_JOURNAL_MAGIC 0x544E5645U /* 'EVNT' */
#define EVENT_JOURNAL_VERSION 1U
#define EVENT_JOURNAL_LBA 4224U
#define EVENT_JOURNAL_BYTES 512U

typedef struct __attribute__((packed)) {
    uint32_t magic;
    uint32_t version;
    uint32_t count;
    uint32_t seq_counter;
    uint32_t checksum;
} event_journal_header_t;

typedef struct __attribute__((packed)) {
    uint32_t sequence;
    char name[OS_SERVICE_NAME_MAX];
    int32_t old_pid;
    int32_t new_pid;
    uint32_t reason;
} event_journal_entry_t;

static uint8_t event_journal_buf[EVENT_JOURNAL_BYTES];

__attribute__((weak)) int service_registry_event_journal_allowed(void) {
    return 1;
}

int service_registry_event_journal_export(uint8_t* buf, uint32_t cap) {
    event_journal_header_t header;
    uint32_t i;
    uint32_t count = 0U;
    uint32_t need;
    uint8_t* cursor;
    if (!buf) return OS_SERVICE_BAD_NAME;
    for (i = 0U; i < SERVICE_REGISTRY_NOTIFY_HISTORY_CAPACITY; i++) {
        if (service_notify_history[i].sequence > 0U && service_notify_history[i].acked == 0U) count++;
    }
    need = (uint32_t)sizeof(header) + count * (uint32_t)sizeof(event_journal_entry_t);
    if (cap < need || need > EVENT_JOURNAL_BYTES) return OS_SERVICE_FULL;
    for (i = 0U; i < cap; i++) buf[i] = 0U;
    cursor = buf + sizeof(header);
    for (i = 0U; i < SERVICE_REGISTRY_NOTIFY_HISTORY_CAPACITY; i++) {
        event_journal_entry_t entry;
        uint32_t b;
        if (service_notify_history[i].sequence == 0U || service_notify_history[i].acked != 0U) continue;
        entry.sequence = service_notify_history[i].sequence;
        copy_name(entry.name, service_notify_history[i].name);
        entry.old_pid = service_notify_history[i].old_pid;
        entry.new_pid = service_notify_history[i].new_pid;
        entry.reason = service_notify_history[i].reason;
        for (b = 0U; b < sizeof(entry); b++) cursor[b] = ((const uint8_t*)&entry)[b];
        cursor += sizeof(entry);
    }
    header.magic = EVENT_JOURNAL_MAGIC;
    header.version = EVENT_JOURNAL_VERSION;
    header.count = count;
    header.seq_counter = service_notify_seq_counter;
    header.checksum = mount_journal_checksum(buf + sizeof(header),
                                             count * (uint32_t)sizeof(event_journal_entry_t));
    for (i = 0U; i < sizeof(header); i++) buf[i] = ((const uint8_t*)&header)[i];
    return 0;
}

int service_registry_event_journal_import(const uint8_t* buf, uint32_t len) {
    event_journal_header_t header;
    uint32_t i;
    uint32_t need;
    const uint8_t* cursor;
    if (!buf || len < sizeof(header)) return OS_SERVICE_BAD_NAME;
    for (i = 0U; i < sizeof(header); i++) ((uint8_t*)&header)[i] = buf[i];
    if (header.magic != EVENT_JOURNAL_MAGIC || header.version != EVENT_JOURNAL_VERSION) return OS_SERVICE_NOT_FOUND;
    if (header.count > SERVICE_REGISTRY_NOTIFY_HISTORY_CAPACITY) return OS_SERVICE_NOT_FOUND;
    need = (uint32_t)sizeof(header) + header.count * (uint32_t)sizeof(event_journal_entry_t);
    if (len < need) return OS_SERVICE_NOT_FOUND;
    if (header.checksum != mount_journal_checksum(buf + sizeof(header),
                                                  header.count * (uint32_t)sizeof(event_journal_entry_t))) {
        return OS_SERVICE_NOT_FOUND;
    }
    for (i = 0U; i < SERVICE_REGISTRY_NOTIFY_HISTORY_CAPACITY; i++) {
        service_notify_history[i].sequence = 0U;
        service_notify_history[i].watcher_pid = 0;
        service_notify_history[i].name[0] = '\0';
        service_notify_history[i].old_pid = 0;
        service_notify_history[i].new_pid = 0;
        service_notify_history[i].reason = 0U;
        service_notify_history[i].acked = 0U;
        service_notify_history[i].replayed = 0U;
    }
    if (header.seq_counter > service_notify_seq_counter) service_notify_seq_counter = header.seq_counter;
    cursor = buf + sizeof(header);
    for (i = 0U; i < header.count; i++) {
        event_journal_entry_t entry;
        uint32_t b;
        for (b = 0U; b < sizeof(entry); b++) ((uint8_t*)&entry)[b] = cursor[b];
        cursor += sizeof(entry);
        if (entry.sequence == 0U) return OS_SERVICE_NOT_FOUND;
        if (entry.reason < OS_SERVICE_EVENT_PUBLISHED || entry.reason > OS_SERVICE_EVENT_PURGED) return OS_SERVICE_NOT_FOUND;
        entry.name[OS_SERVICE_NAME_MAX - 1U] = '\0';
        if (!service_registry_name_valid(entry.name)) return OS_SERVICE_NOT_FOUND;
        service_notify_history[i].sequence = entry.sequence;
        service_notify_history[i].watcher_pid = 0;
        copy_name(service_notify_history[i].name, entry.name);
        service_notify_history[i].old_pid = entry.old_pid;
        service_notify_history[i].new_pid = entry.new_pid;
        service_notify_history[i].reason = entry.reason;
        service_notify_history[i].acked = 0U;
        service_notify_history[i].replayed = 0U;
    }
    return 0;
}

static void event_journal_flush(void) {
    if (!service_registry_event_journal_allowed()) return;
    if (service_registry_event_journal_export(event_journal_buf, EVENT_JOURNAL_BYTES) != 0) return;
    (void)ata_write_sectors(EVENT_JOURNAL_LBA, EVENT_JOURNAL_BYTES / 512U, event_journal_buf);
}

static int event_journal_load(void) {
    if (ata_read_sectors(EVENT_JOURNAL_LBA, EVENT_JOURNAL_BYTES / 512U, event_journal_buf) != 0) return -1;
    return service_registry_event_journal_import(event_journal_buf, EVENT_JOURNAL_BYTES);
}

static void event_journal_bind(const char* name, int32_t pid) {
    uint32_t i;
    for (i = 0U; i < SERVICE_REGISTRY_NOTIFY_HISTORY_CAPACITY; i++) {
        if (service_notify_history[i].watcher_pid == 0 &&
            service_notify_history[i].sequence > 0U &&
            service_notify_history[i].acked == 0U &&
            name_equal(service_notify_history[i].name, name)) {
            service_notify_history[i].watcher_pid = pid;
        }
    }
}

int service_registry_mount_journal_format(const char* service_name, char* buf, uint32_t max) {
    uint32_t i;
    uint32_t used = 0U;
    if (!buf || max == 0U || !service_registry_name_valid(service_name)) return OS_SERVICE_BAD_NAME;
    buf[0] = '\0';
    for (i = 0U; i < SERVICE_REGISTRY_PERSISTENT_MOUNT_CAPACITY; i++) {
        uint32_t n;
        if (!persistent_mounts[i].active || !name_equal(persistent_mounts[i].service_name, service_name)) continue;
        n = 0U;
        while (persistent_mounts[i].prefix[n] != '\0' && n < OS_SERVICE_BACKEND_PREFIX_MAX) n++;
        if (used + n + 2U > max) return OS_SERVICE_FULL;
        {
            uint32_t c;
            for (c = 0U; c < n; c++) buf[used + c] = persistent_mounts[i].prefix[c];
        }
        used += n;
        buf[used++] = ' ';
        buf[used] = '\0';
    }
    return 0;
}

int service_registry_persistent_mount_add(const char* service_name, const char* prefix, uint32_t source) {
    int rc = mount_insert(service_name, prefix, source);
    if (rc == 0) mount_journal_flush();
    return rc;
}

int service_registry_persistent_mount_remove(const char* service_name, const char* prefix) {
    uint32_t i;
    if (!service_registry_name_valid(service_name) || !prefix) return OS_SERVICE_BAD_NAME;
    for (i = 0U; i < SERVICE_REGISTRY_PERSISTENT_MOUNT_CAPACITY; i++) {
        if (persistent_mounts[i].active &&
            name_equal(persistent_mounts[i].service_name, service_name) &&
            service_registry_backend_prefix_equal(persistent_mounts[i].prefix, prefix)) {
            persistent_mounts[i].active = 0U;
            persistent_mounts[i].service_name[0] = '\0';
            persistent_mounts[i].prefix[0] = '\0';
            mount_journal_flush();
            return 0;
        }
    }
    return OS_SERVICE_NOT_FOUND;
}

int service_registry_persistent_mount_get(const char* service_name, service_registry_persistent_mount_t* out, uint32_t max) {
    uint32_t i, count = 0U;
    if (!service_registry_name_valid(service_name)) return OS_SERVICE_BAD_NAME;
    for (i = 0U; i < SERVICE_REGISTRY_PERSISTENT_MOUNT_CAPACITY; i++) {
        if (persistent_mounts[i].active && name_equal(persistent_mounts[i].service_name, service_name)) {
            if (out && count < max) {
                out[count] = persistent_mounts[i];
            }
            count++;
        }
    }
    return (int)count;
}

int service_registry_persistent_mount_restore(const char* service_name) {
    return service_registry_persistent_mount_get(service_name, (service_registry_persistent_mount_t*)0, 0U);
}

void service_registry_backend_remove_name(const char* name) {
    uint32_t i;
    int changed = 0;
    if (!service_registry_name_valid(name)) return;
    for (i = 0U; i < SERVICE_REGISTRY_BACKEND_CAPACITY; i++) {
        if (service_backend_caps[i].owner_pid > 0 && name_equal(service_backend_caps[i].name, name)) {
            service_backend_caps[i].owner_pid = 0; service_backend_caps[i].grantee_pid = 0; service_backend_caps[i].rights = 0U; service_backend_caps[i].sources = 0U; service_backend_caps[i].prefix[0] = '\0'; service_backend_caps[i].name[0] = '\0'; service_backend_caps[i].token = 0U; service_backend_caps[i].identity_key = 0U;
            changed = 1;
        }
    }
    if (changed) service_registry_backend_generation_bump(name);
}

void service_registry_backend_remove_pid(int32_t pid) {
    uint32_t i;
    if (pid <= 0) return;
    for (i = 0U; i < SERVICE_REGISTRY_BACKEND_CAPACITY; i++) {
        if (service_backend_caps[i].owner_pid == pid || service_backend_caps[i].grantee_pid == pid) {
            service_registry_backend_generation_bump(service_backend_caps[i].name);
            service_backend_caps[i].owner_pid = 0; service_backend_caps[i].grantee_pid = 0; service_backend_caps[i].rights = 0U; service_backend_caps[i].sources = 0U; service_backend_caps[i].prefix[0] = '\0'; service_backend_caps[i].name[0] = '\0'; service_backend_caps[i].token = 0U; service_backend_caps[i].identity_key = 0U;
        }
    }
}

int service_registry_backend_allowed_for_source_path(const char* name, int32_t pid, uint32_t right,
                                                     uint32_t source, const char* path) {
    uint32_t i;
    if (right == 0U || (right & ~SERVICE_BACKEND_RIGHT_ALL) != 0U ||
        !service_registry_backend_sources_valid(source)) return 0;
    for (i = 0U; i < SERVICE_REGISTRY_BACKEND_CAPACITY; i++) {
        if (service_backend_caps[i].grantee_pid == pid &&
            task_identity_valid(pid, service_backend_caps[i].grantee_sequence, service_backend_caps[i].grantee_generation) &&
            name_equal(service_backend_caps[i].name, name) &&
            service_registry_lookup(name) == service_backend_caps[i].owner_pid &&
            task_identity_valid(service_backend_caps[i].owner_pid, service_backend_caps[i].owner_sequence, service_backend_caps[i].owner_generation) &&
            service_backend_caps[i].token != 0U &&
            (service_backend_caps[i].rights & right) == right &&
            (service_backend_caps[i].sources & source) == source &&
            identity_key_bound(pid, service_backend_caps[i].identity_key) &&
            service_registry_backend_prefix_matches(service_backend_caps[i].prefix, path)) return 1;
    }
    return 0;
}

int service_registry_backend_allowed_for_source(const char* name, int32_t pid, uint32_t right,
                                                uint32_t source) {
    return service_registry_backend_allowed_for_source_path(name, pid, right, source, (const char*)0);
}

/* AOS-2172/2173/2174: with live vfs-virtual, every valid backend scope
 * (single source, combinations, or SOURCE_ALL covering ATA-backed overlay/FAT)
 * stays behind grants; owner bypass remains only without the storage worker. */
int service_registry_owner_bypasses_backend(const char* name, int32_t pid, uint32_t source) {
    if (!name || pid <= 0) return 0;
    if (service_registry_lookup(name) != pid) return 0;
    if (source != 0U && (source & ~OS_SERVICE_BACKEND_SOURCE_ALL) == 0U) {
        if (service_registry_lookup("vfs-virtual") > 0) return 0;
    }
    return 1;
}

/* AOS-2175: with vfs-virtual live, ATA-backed overlay I/O (read/stat slice) is
 * worker-mediated — only that PID may exercise the overlay path. Degraded mode
 * without the worker keeps historical local exercise. Drivers stay Ring 0. */
int service_registry_ata_overlay_io_via_worker(int32_t pid) {
    int32_t worker;
    if (pid <= 0) return 0;
    worker = service_registry_lookup("vfs-virtual");
    if (worker <= 0) return 1;
    return pid == worker;
}

/* Tranche 4: the "ata-driver" name carries the Ring 3 ATA port capability
 * (TSS IOPB). Only a task whose binary name is "atadriver" may publish or
 * receive it, and only "ataclient" may hold "ata-client" (the sole sector IPC
 * peer accepted by the driver). Any other service name is unaffected. */
int service_registry_ata_driver_name_allowed(const char* service_name, const char* task_name) {
    if (name_equal(service_name, "ata-driver"))
        return task_name && name_equal(task_name, "atadriver");
    /* The sector IPC client identity is pinned to the ataclient binary too,
     * so an arbitrary task cannot claim it to reach the driver. */
    if (name_equal(service_name, "ata-client"))
        return task_name && name_equal(task_name, "ataclient");
    return 1;
}

/* Tranche 4: TSS IOPB decision for the task being scheduled. */
static int service_right_held(const char* name, int32_t pid) {
    uint32_t i;
    for (i = 0U; i < SERVICE_REGISTRY_CAPACITY; i++) {
        if (service_entries[i].pid == pid &&
            name_equal(service_entries[i].name, name) &&
            service_entries[i].right_token != 0U &&
            task_identity_valid(pid, service_entries[i].sequence, service_entries[i].generation) &&
            identity_key_bound(pid, service_entries[i].identity_key)) return 1;
    }
    return 0;
}

int service_registry_ata_ports_granted(int32_t pid) {
    if (pid <= 0) return 0;
    return service_right_held("ata-driver", pid);
}

/* AOS-2177: decision for the historical SYS_READFILE ABI. Without a live
 * vfs-virtual worker (degraded mode) or for the worker PID itself, the full
 * overlay-then-initrd path stays open. Otherwise the ATA-backed overlay is
 * refused (WORKER_REQUIRED when the path lives there) while RAM-only initrd
 * reads stay available. Drivers stay Ring 0. */
int service_registry_historical_read_decision(int32_t pid, int overlay_hit) {
    if (pid <= 0) return SERVICE_HIST_READ_DENIED;
    if (service_registry_ata_overlay_io_via_worker(pid)) return SERVICE_HIST_READ_FULL;
    if (overlay_hit) return SERVICE_HIST_READ_WORKER_REQUIRED;
    return SERVICE_HIST_READ_INITRD_ONLY;
}

/* Isolation du sous-système réseau NE2000 : avec net-driver présent, l'accès
 * aux primitives réseau est restreint au seul PID du pilote Ring 3 enregistré. */
int service_registry_net_io_via_worker(int32_t pid) {
    int32_t worker;
    if (pid <= 0) return 0;
    worker = service_registry_lookup("net-driver");
    if (worker <= 0) return 1;
    return pid == worker && service_right_held("net-driver", pid);
}

/* Tranche 5: syscalls that drive the NE2000 NIC or the kernel TCP/socket,
 * LLM-network and peer state. Read-only status (SYS_NET_STATUS,
 * SYS_LLM_SESSION_STATUS) stays open for diagnostics. */
int service_registry_net_syscall_gated(uint32_t syscall_number) {
    if (syscall_number >= SYS_LLM_ACQUIRE_START && syscall_number <= SYS_LLM_OPENAI_CREDENTIAL) return 1;
    if (syscall_number >= SYS_SOCKET_OPEN && syscall_number <= SYS_SOCKET_ACCEPT_ACK) return 1;
    /* Tranche 5 slice 3: relayed wire connect. SYS_NET_WIRE_* (139-142) are
     * stricter (live worker only, even in degraded mode) and are checked in
     * the syscall handler itself. */
    if (syscall_number == SYS_SOCKET_CONNECT) return 1;
    if (syscall_number >= SYS_PEER_LISTEN && syscall_number <= SYS_PEER_TLS_POLL) return 1;
    return 0;
}

/* Tranche 5: 1 = dispatch, 0 = refuse with OS_NET_WORKER_REQUIRED. Without a
 * registered net-driver (degraded mode) every task keeps the historical path.
 * The NE2000 driver itself stays in Ring 0. */
int service_registry_net_syscall_allowed(int32_t pid, uint32_t syscall_number) {
    if (!service_registry_net_syscall_gated(syscall_number)) return 1;
    return service_registry_net_io_via_worker(pid);
}

/* Les appels backend génériques restent compatibles, mais exigent explicitement
 * le scope toutes sources : une capacité source-scopée ne peut pas les utiliser. */
int service_registry_backend_allowed_for(const char* name, int32_t pid, uint32_t right) {
    return service_registry_backend_allowed_for_source(name, pid, right,
                                                       OS_SERVICE_BACKEND_SOURCE_ALL);
}

int service_registry_backend_allowed(const char* name, int32_t pid) {
    return service_registry_backend_allowed_for(name, pid, SERVICE_BACKEND_RIGHT_ALL);
}

int service_registry_backend_grant(const char* name, int32_t owner_pid, int32_t grantee_pid) {
    return service_registry_backend_grant_scoped(name, owner_pid, grantee_pid, SERVICE_BACKEND_RIGHT_ALL);
}

int service_registry_backend_grant_scoped(const char* name, int32_t owner_pid, int32_t grantee_pid, uint32_t rights) {
    return service_registry_backend_grant_scoped_source(name, owner_pid, grantee_pid, rights,
                                                        OS_SERVICE_BACKEND_SOURCE_ALL);
}

int service_registry_backend_grant_scoped_source(const char* name, int32_t owner_pid, int32_t grantee_pid,
                                                 uint32_t rights, uint32_t sources) {
    return service_registry_backend_grant_scoped_source_prefix(name, owner_pid, grantee_pid, rights,
                                                               sources, "");
}

int service_registry_backend_grant_scoped_source_prefix(const char* name, int32_t owner_pid,
                                                        int32_t grantee_pid, uint32_t rights,
                                                        uint32_t sources, const char* prefix) {
    uint32_t i; int free_slot = -1;
    uint32_t o_seq = 0U, o_gen = 0U, g_seq = 0U, g_gen = 0U;
    if (!service_registry_name_valid(name) || owner_pid <= 0 || grantee_pid <= 0 || rights == 0U ||
        (rights & ~SERVICE_BACKEND_RIGHT_ALL) != 0U || !service_registry_backend_sources_valid(sources) ||
        !service_registry_backend_prefix_valid(prefix)) return OS_SERVICE_BAD_NAME;
    if (service_registry_lookup(name) != owner_pid) return OS_SERVICE_NOT_OWNER;
    (void)task_get_identity(owner_pid, &o_seq, &o_gen);
    (void)task_get_identity(grantee_pid, &g_seq, &g_gen);
    for (i = 0U; i < SERVICE_REGISTRY_BACKEND_CAPACITY; i++) {
        if (service_backend_caps[i].owner_pid == owner_pid && service_backend_caps[i].grantee_pid == grantee_pid && name_equal(service_backend_caps[i].name, name)) {
            uint32_t combined_rights = service_backend_caps[i].rights | rights;
            uint32_t combined_sources = service_backend_caps[i].sources | sources;
            if (service_backend_caps[i].rights != combined_rights ||
                service_backend_caps[i].sources != combined_sources ||
                !service_registry_backend_prefix_equal(service_backend_caps[i].prefix, prefix)) {
                service_backend_caps[i].rights = combined_rights;
                service_backend_caps[i].sources = combined_sources;
                service_backend_caps[i].owner_sequence = o_seq;
                service_backend_caps[i].owner_generation = o_gen;
                service_backend_caps[i].grantee_sequence = g_seq;
                service_backend_caps[i].grantee_generation = g_gen;
                {
                    uint32_t key = 0U;
                    if (task_identity_key(grantee_pid, &key) != 0) key = 0U;
                    service_backend_caps[i].identity_key = key;
                }
                service_registry_backend_copy_prefix(service_backend_caps[i].prefix, prefix);
                if (service_backend_caps[i].token == 0U) {
                    service_backend_caps[i].token = service_backend_token_next++;
                    if (service_backend_token_next == 0U) service_backend_token_next = 1U;
                }
                service_registry_backend_generation_bump(name);
            }
            return 0;
        }
        if (service_backend_caps[i].owner_pid == 0 && free_slot < 0) free_slot = (int)i;
    }
    if (free_slot < 0) return OS_SERVICE_FULL;
    service_backend_caps[free_slot].owner_pid = owner_pid;
    service_backend_caps[free_slot].owner_sequence = o_seq;
    service_backend_caps[free_slot].owner_generation = o_gen;
    service_backend_caps[free_slot].grantee_pid = grantee_pid;
    service_backend_caps[free_slot].grantee_sequence = g_seq;
    service_backend_caps[free_slot].grantee_generation = g_gen;
    {
        uint32_t key = 0U;
        if (task_identity_key(grantee_pid, &key) != 0) key = 0U;
        service_backend_caps[free_slot].identity_key = key;
    }
    service_backend_caps[free_slot].rights = rights;
    service_backend_caps[free_slot].sources = sources;
    service_backend_caps[free_slot].token = service_backend_token_next++;
    if (service_backend_token_next == 0U) service_backend_token_next = 1U;
    service_registry_backend_copy_prefix(service_backend_caps[free_slot].prefix, prefix);
    copy_name(service_backend_caps[free_slot].name, name);
    service_registry_backend_generation_bump(name);
    return 0;
}

int service_registry_backend_revoke(const char* name, int32_t owner_pid, int32_t grantee_pid) {
    uint32_t i;
    if (!service_registry_name_valid(name) || owner_pid <= 0 || grantee_pid <= 0) return OS_SERVICE_BAD_NAME;
    if (service_registry_lookup(name) != owner_pid) return OS_SERVICE_NOT_OWNER;
    for (i = 0U; i < SERVICE_REGISTRY_BACKEND_CAPACITY; i++) {
        if (service_backend_caps[i].owner_pid == owner_pid && service_backend_caps[i].grantee_pid == grantee_pid &&
            name_equal(service_backend_caps[i].name, name)) {
            service_backend_caps[i].owner_pid = 0; service_backend_caps[i].grantee_pid = 0; service_backend_caps[i].rights = 0U; service_backend_caps[i].sources = 0U; service_backend_caps[i].prefix[0] = '\0'; service_backend_caps[i].name[0] = '\0'; service_backend_caps[i].token = 0U; service_backend_caps[i].identity_key = 0U;
            service_registry_backend_generation_bump(name);
            return 0;
        }
    }
    return OS_SERVICE_NOT_FOUND;
}

int service_registry_backend_token_of(const char* name, int32_t grantee_pid, uint32_t* out_token) {
    uint32_t i;
    if (!out_token) return OS_SERVICE_BAD_NAME;
    *out_token = 0U;
    if (!service_registry_name_valid(name) || grantee_pid <= 0) return OS_SERVICE_BAD_NAME;
    for (i = 0U; i < SERVICE_REGISTRY_BACKEND_CAPACITY; i++) {
        if (service_backend_caps[i].grantee_pid == grantee_pid &&
            service_backend_caps[i].token != 0U &&
            name_equal(service_backend_caps[i].name, name) &&
            task_identity_valid(grantee_pid, service_backend_caps[i].grantee_sequence,
                                service_backend_caps[i].grantee_generation)) {
            *out_token = service_backend_caps[i].token;
            return 0;
        }
    }
    return OS_SERVICE_NOT_FOUND;
}

int service_registry_backend_clear_token(const char* name, int32_t grantee_pid) {
    uint32_t i;
    if (!service_registry_name_valid(name) || grantee_pid <= 0) return OS_SERVICE_BAD_NAME;
    for (i = 0U; i < SERVICE_REGISTRY_BACKEND_CAPACITY; i++) {
        if (service_backend_caps[i].grantee_pid == grantee_pid &&
            name_equal(service_backend_caps[i].name, name)) {
            service_backend_caps[i].token = 0U; service_backend_caps[i].identity_key = 0U;
            return 0;
        }
    }
    return OS_SERVICE_NOT_FOUND;
}

int service_registry_backend_release(const char* name, int32_t grantee_pid) {
    uint32_t i;
    if (!service_registry_name_valid(name) || grantee_pid <= 0) return OS_SERVICE_BAD_NAME;
    for (i = 0U; i < SERVICE_REGISTRY_BACKEND_CAPACITY; i++) {
        if (service_backend_caps[i].grantee_pid == grantee_pid &&
            name_equal(service_backend_caps[i].name, name)) {
            service_backend_caps[i].owner_pid = 0;
            service_backend_caps[i].grantee_pid = 0;
            service_backend_caps[i].rights = 0U;
            service_backend_caps[i].token = 0U; service_backend_caps[i].identity_key = 0U;
            service_backend_caps[i].name[0] = '\0';
            service_registry_backend_generation_bump(name);
            return 0;
        }
    }
    return OS_SERVICE_NOT_FOUND;
}

int service_registry_backend_rights(const char* name, int32_t owner_pid, int32_t grantee_pid, uint32_t* out_rights) {
    uint32_t i;
    if (!service_registry_name_valid(name) || owner_pid <= 0 || grantee_pid <= 0 || !out_rights) return OS_SERVICE_BAD_NAME;
    if (service_registry_lookup(name) != owner_pid) return OS_SERVICE_NOT_OWNER;
    for (i = 0U; i < SERVICE_REGISTRY_BACKEND_CAPACITY; i++) {
        if (service_backend_caps[i].owner_pid == owner_pid && service_backend_caps[i].grantee_pid == grantee_pid &&
            name_equal(service_backend_caps[i].name, name) &&
            service_backend_caps[i].token != 0U) {
            *out_rights = service_backend_caps[i].rights;
            return 0;
        }
    }
    return OS_SERVICE_NOT_FOUND;
}

int service_registry_backend_scope(const char* name, int32_t owner_pid, int32_t grantee_pid,
                                   os_service_backend_scope_t* out_scope) {
    uint32_t i;
    if (!out_scope) return OS_SERVICE_BAD_NAME;
    out_scope->rights = 0U;
    out_scope->sources = 0U;
    if (!service_registry_name_valid(name) || owner_pid <= 0 || grantee_pid <= 0) return OS_SERVICE_BAD_NAME;
    if (service_registry_lookup(name) != owner_pid) return OS_SERVICE_NOT_OWNER;
    for (i = 0U; i < SERVICE_REGISTRY_BACKEND_CAPACITY; i++) {
        if (service_backend_caps[i].owner_pid == owner_pid && service_backend_caps[i].grantee_pid == grantee_pid &&
            name_equal(service_backend_caps[i].name, name) &&
            service_backend_caps[i].token != 0U) {
            out_scope->rights = service_backend_caps[i].rights;
            out_scope->sources = service_backend_caps[i].sources;
            return 0;
        }
    }
    return OS_SERVICE_NOT_FOUND;
}

int service_registry_backend_list(const char* name, int32_t owner_pid, os_service_backend_list_t* out_list) {
    uint32_t i;
    uint32_t count = 0U;
    if (!out_list) return OS_SERVICE_BAD_NAME;
    out_list->count = 0U;
    for (i = 0U; i < OS_SERVICE_BACKEND_CAPACITY; i++) {
        out_list->entries[i].pid = 0;
        out_list->entries[i].rights = 0U;
    }
    if (!service_registry_name_valid(name) || owner_pid <= 0) return OS_SERVICE_BAD_NAME;
    if (service_registry_lookup(name) != owner_pid) return OS_SERVICE_NOT_OWNER;
    for (i = 0U; i < SERVICE_REGISTRY_BACKEND_CAPACITY; i++) {
        if (service_backend_caps[i].owner_pid == owner_pid &&
            service_backend_caps[i].grantee_pid > 0 &&
            service_backend_caps[i].token != 0U &&
            name_equal(service_backend_caps[i].name, name)) {
            out_list->entries[count].pid = service_backend_caps[i].grantee_pid;
            out_list->entries[count].rights = service_backend_caps[i].rights;
            count++;
        }
    }
    out_list->count = count;
    return 0;
}

int service_registry_backend_observe(const char* name, int32_t owner_pid, uint32_t expected_generation,
                                     os_service_backend_snapshot_t* out_snapshot) {
    uint32_t i;
    int status;
    if (!out_snapshot) return OS_SERVICE_BAD_NAME;
    out_snapshot->generation = 0U;
    out_snapshot->list.count = 0U;
    for (i = 0U; i < OS_SERVICE_BACKEND_CAPACITY; i++) {
        out_snapshot->list.entries[i].pid = 0;
        out_snapshot->list.entries[i].rights = 0U;
    }
    status = service_registry_backend_list(name, owner_pid, &out_snapshot->list);
    if (status != 0) return status;
    out_snapshot->generation = service_registry_backend_generation_of(name);
    if (expected_generation != 0U && expected_generation != out_snapshot->generation) {
        out_snapshot->list.count = 0U;
        for (i = 0U; i < OS_SERVICE_BACKEND_CAPACITY; i++) {
            out_snapshot->list.entries[i].pid = 0;
            out_snapshot->list.entries[i].rights = 0U;
        }
        return OS_SERVICE_STALE;
    }
    return 0;
}

int service_registry_subscribe(const char* name, int32_t pid) {
    uint32_t i;
    int free_slot = -1;
    uint32_t seq = 0U, gen = 0U;
    if (!service_registry_name_valid(name) || pid <= 0) return OS_SERVICE_BAD_NAME;
    (void)task_get_identity(pid, &seq, &gen);
    for (i = 0U; i < SERVICE_REGISTRY_WATCH_CAPACITY; i++) {
        if (service_watches[i].pid == 0) {
            if (free_slot < 0) free_slot = (int)i;
            continue;
        }
        if (service_watches[i].pid == pid && name_equal(service_watches[i].name, name)) {
            service_watches[i].sequence = seq;
            service_watches[i].generation = gen;
            event_journal_bind(name, pid);
            return 0;
        }
    }
    if (free_slot < 0) return OS_SERVICE_WATCH_FULL;
    service_watches[free_slot].pid = pid;
    service_watches[free_slot].sequence = seq;
    service_watches[free_slot].generation = gen;
    copy_name(service_watches[free_slot].name, name);
    event_journal_bind(name, pid);
    return 0;
}

int service_registry_collect_watchers(const char* name, int32_t* out, uint32_t max) {
    uint32_t i;
    uint32_t count = 0U;
    if (!service_registry_name_valid(name) || (!out && max > 0U)) return OS_SERVICE_BAD_NAME;
    for (i = 0U; i < SERVICE_REGISTRY_WATCH_CAPACITY; i++) {
        if (service_watches[i].pid > 0 && name_equal(service_watches[i].name, name) &&
            task_identity_valid(service_watches[i].pid, service_watches[i].sequence, service_watches[i].generation)) {
            if (count < max) out[count] = service_watches[i].pid;
            count++;
        }
    }
    return (int)count;
}

int service_registry_remove_watcher_pid(int32_t pid) {
    uint32_t i;
    int removed = 0;
    if (pid <= 0) return OS_SERVICE_NOT_FOUND;
    for (i = 0U; i < SERVICE_REGISTRY_WATCH_CAPACITY; i++) {
        if (service_watches[i].pid == pid) {
            service_watches[i].pid = 0;
            service_watches[i].name[0] = '\0';
            removed++;
        }
    }
    ipc_spill_clear_pid(pid);
    return removed > 0 ? 0 : OS_SERVICE_NOT_FOUND;
}

#define SERVICE_IPC_SPILL_CAPACITY 8U

typedef struct {
    int32_t watcher_pid;
    uint32_t order;
    os_ipc_payload_t payload;
    uint8_t used;
} service_ipc_spill_slot_t;

static service_ipc_spill_slot_t service_ipc_spill[SERVICE_IPC_SPILL_CAPACITY];
static uint32_t service_ipc_spill_order;

static void ipc_spill_reset(void) {
    uint32_t i;
    for (i = 0U; i < SERVICE_IPC_SPILL_CAPACITY; i++) {
        service_ipc_spill[i].used = 0U;
        service_ipc_spill[i].watcher_pid = 0;
        service_ipc_spill[i].order = 0U;
    }
    service_ipc_spill_order = 0U;
}

static void ipc_spill_clear_pid(int32_t pid) {
    uint32_t i;
    for (i = 0U; i < SERVICE_IPC_SPILL_CAPACITY; i++) {
        if (service_ipc_spill[i].used && service_ipc_spill[i].watcher_pid == pid) {
            service_ipc_spill[i].used = 0U;
            service_ipc_spill[i].watcher_pid = 0;
        }
    }
}

int service_registry_ipc_spill_push(int32_t watcher_pid, const os_ipc_payload_t* payload) {
    uint32_t i;
    uint32_t n;
    if (watcher_pid <= 0 || !payload || payload->size > OS_IPC_MAX_DATA) return OS_IPC_BAD_MESSAGE;
    for (i = 0U; i < SERVICE_IPC_SPILL_CAPACITY; i++) {
        if (!service_ipc_spill[i].used) {
            service_ipc_spill_order++;
            if (service_ipc_spill_order == 0U) service_ipc_spill_order = 1U;
            service_ipc_spill[i].used = 1U;
            service_ipc_spill[i].watcher_pid = watcher_pid;
            service_ipc_spill[i].order = service_ipc_spill_order;
            service_ipc_spill[i].payload.type = payload->type;
            service_ipc_spill[i].payload.size = payload->size;
            service_ipc_spill[i].payload.request_id = payload->request_id;
            for (n = 0U; n < payload->size; n++) service_ipc_spill[i].payload.data[n] = payload->data[n];
            for (; n < OS_IPC_MAX_DATA; n++) service_ipc_spill[i].payload.data[n] = 0U;
            return 0;
        }
    }
    return OS_IPC_FULL;
}

int service_registry_ipc_spill_pop(int32_t watcher_pid, os_ipc_payload_t* out) {
    uint32_t i;
    uint32_t n;
    int found = -1;
    uint32_t best = 0U;
    if (watcher_pid <= 0 || !out) return OS_IPC_BAD_MESSAGE;
    for (i = 0U; i < SERVICE_IPC_SPILL_CAPACITY; i++) {
        if (service_ipc_spill[i].used && service_ipc_spill[i].watcher_pid == watcher_pid) {
            if (found < 0 || service_ipc_spill[i].order < best) {
                found = (int)i;
                best = service_ipc_spill[i].order;
            }
        }
    }
    if (found < 0) return OS_IPC_EMPTY;
    out->type = service_ipc_spill[found].payload.type;
    out->size = service_ipc_spill[found].payload.size;
    out->request_id = service_ipc_spill[found].payload.request_id;
    for (n = 0U; n < OS_IPC_MAX_DATA; n++) out->data[n] = service_ipc_spill[found].payload.data[n];
    service_ipc_spill[found].used = 0U;
    service_ipc_spill[found].watcher_pid = 0;
    return 0;
}
