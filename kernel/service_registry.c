#include "service_registry.h"

static service_registry_entry_t service_entries[SERVICE_REGISTRY_CAPACITY];
static service_registry_watch_t service_watches[SERVICE_REGISTRY_WATCH_CAPACITY];
typedef struct { int32_t owner_pid; int32_t grantee_pid; uint32_t rights; uint32_t sources; char prefix[OS_SERVICE_BACKEND_PREFIX_MAX]; char name[OS_SERVICE_NAME_MAX]; } service_backend_cap_t;
static service_backend_cap_t service_backend_caps[SERVICE_REGISTRY_BACKEND_CAPACITY];

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
        service_entries[i].name[0] = '\0';
        service_entries[i].backend_generation = 1U;
    }
    for (i = 0U; i < SERVICE_REGISTRY_WATCH_CAPACITY; i++) {
        service_watches[i].pid = 0;
        service_watches[i].name[0] = '\0';
    }
    for (i = 0U; i < SERVICE_REGISTRY_BACKEND_CAPACITY; i++) {
        service_backend_caps[i].owner_pid = 0;
        service_backend_caps[i].grantee_pid = 0;
        service_backend_caps[i].rights = 0U;
        service_backend_caps[i].sources = 0U;
        service_backend_caps[i].prefix[0] = '\0';
        service_backend_caps[i].name[0] = '\0';
    }
}

int service_registry_register(const char* name, int32_t pid) {
    uint32_t i;
    int free_slot = -1;
    if (!service_registry_name_valid(name) || pid <= 0) return OS_SERVICE_BAD_NAME;
    for (i = 0U; i < SERVICE_REGISTRY_CAPACITY; i++) {
        if (service_entries[i].pid == 0) {
            if (free_slot < 0) free_slot = (int)i;
            continue;
        }
        if (name_equal(service_entries[i].name, name)) {
            if (service_entries[i].pid == pid) return 0;
            return OS_SERVICE_TAKEN;
        }
    }
    if (free_slot < 0) return OS_SERVICE_FULL;
    service_entries[free_slot].pid = pid;
    copy_name(service_entries[free_slot].name, name);
    service_entries[free_slot].backend_generation = 1U;
    return 0;
}

int service_registry_lookup(const char* name) {
    uint32_t i;
    if (!service_registry_name_valid(name)) return OS_SERVICE_BAD_NAME;
    for (i = 0U; i < SERVICE_REGISTRY_CAPACITY; i++) {
        if (service_entries[i].pid > 0 && name_equal(service_entries[i].name, name)) {
            return service_entries[i].pid;
        }
    }
    return OS_SERVICE_NOT_FOUND;
}

int service_registry_remove(const char* name, int32_t pid) {
    uint32_t i;
    if (!service_registry_name_valid(name) || pid <= 0) return OS_SERVICE_BAD_NAME;
    for (i = 0U; i < SERVICE_REGISTRY_CAPACITY; i++) {
        if (service_entries[i].pid == pid && name_equal(service_entries[i].name, name)) {
            service_entries[i].pid = 0;
            service_entries[i].name[0] = '\0';
            service_registry_backend_remove_name(name);
            return 0;
        }
    }
    return OS_SERVICE_NOT_FOUND;
}

int service_registry_grant(const char* name, int32_t owner_pid, int32_t grantee_pid) {
    uint32_t i;
    if (!service_registry_name_valid(name) || owner_pid <= 0) return OS_SERVICE_BAD_NAME;
    if (grantee_pid <= 0) return OS_SERVICE_BAD_GRANTEE;
    for (i = 0U; i < SERVICE_REGISTRY_CAPACITY; i++) {
        if (name_equal(service_entries[i].name, name)) {
            if (service_entries[i].pid != owner_pid) return OS_SERVICE_NOT_OWNER;
            service_entries[i].pid = grantee_pid;
            service_registry_backend_remove_name(name);
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
        if (service_entries[i].pid == pid) {
            if (count < max) {
                out[count].pid = pid;
                copy_name(out[count].name, service_entries[i].name);
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
        if (service_entries[i].pid == pid) return 1;
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
            removed++;
        }
    }
    return removed > 0 ? 0 : OS_SERVICE_NOT_FOUND;
}

void service_registry_backend_remove_name(const char* name) {
    uint32_t i;
    int changed = 0;
    if (!service_registry_name_valid(name)) return;
    for (i = 0U; i < SERVICE_REGISTRY_BACKEND_CAPACITY; i++) {
        if (service_backend_caps[i].owner_pid > 0 && name_equal(service_backend_caps[i].name, name)) {
            service_backend_caps[i].owner_pid = 0; service_backend_caps[i].grantee_pid = 0; service_backend_caps[i].rights = 0U; service_backend_caps[i].sources = 0U; service_backend_caps[i].prefix[0] = '\0'; service_backend_caps[i].name[0] = '\0';
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
            service_backend_caps[i].owner_pid = 0; service_backend_caps[i].grantee_pid = 0; service_backend_caps[i].rights = 0U; service_backend_caps[i].sources = 0U; service_backend_caps[i].prefix[0] = '\0'; service_backend_caps[i].name[0] = '\0';
        }
    }
}

int service_registry_backend_allowed_for_source_path(const char* name, int32_t pid, uint32_t right,
                                                     uint32_t source, const char* path) {
    uint32_t i;
    if (right == 0U || (right & ~SERVICE_BACKEND_RIGHT_ALL) != 0U ||
        !service_registry_backend_sources_valid(source)) return 0;
    for (i = 0U; i < SERVICE_REGISTRY_BACKEND_CAPACITY; i++) {
        if (service_backend_caps[i].grantee_pid == pid && name_equal(service_backend_caps[i].name, name) &&
            service_registry_lookup(name) == service_backend_caps[i].owner_pid &&
            (service_backend_caps[i].rights & right) == right &&
            (service_backend_caps[i].sources & source) == source &&
            service_registry_backend_prefix_matches(service_backend_caps[i].prefix, path)) return 1;
    }
    return 0;
}

int service_registry_backend_allowed_for_source(const char* name, int32_t pid, uint32_t right,
                                                uint32_t source) {
    return service_registry_backend_allowed_for_source_path(name, pid, right, source, (const char*)0);
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
    if (!service_registry_name_valid(name) || owner_pid <= 0 || grantee_pid <= 0 || rights == 0U ||
        (rights & ~SERVICE_BACKEND_RIGHT_ALL) != 0U || !service_registry_backend_sources_valid(sources) ||
        !service_registry_backend_prefix_valid(prefix)) return OS_SERVICE_BAD_NAME;
    if (service_registry_lookup(name) != owner_pid) return OS_SERVICE_NOT_OWNER;
    for (i = 0U; i < SERVICE_REGISTRY_BACKEND_CAPACITY; i++) {
        if (service_backend_caps[i].owner_pid == owner_pid && service_backend_caps[i].grantee_pid == grantee_pid && name_equal(service_backend_caps[i].name, name)) {
            uint32_t combined_rights = service_backend_caps[i].rights | rights;
            uint32_t combined_sources = service_backend_caps[i].sources | sources;
            if (service_backend_caps[i].rights != combined_rights ||
                service_backend_caps[i].sources != combined_sources ||
                !service_registry_backend_prefix_equal(service_backend_caps[i].prefix, prefix)) {
                service_backend_caps[i].rights = combined_rights;
                service_backend_caps[i].sources = combined_sources;
                service_registry_backend_copy_prefix(service_backend_caps[i].prefix, prefix);
                service_registry_backend_generation_bump(name);
            }
            return 0;
        }
        if (service_backend_caps[i].owner_pid == 0 && free_slot < 0) free_slot = (int)i;
    }
    if (free_slot < 0) return OS_SERVICE_FULL;
    service_backend_caps[free_slot].owner_pid = owner_pid; service_backend_caps[free_slot].grantee_pid = grantee_pid;
    service_backend_caps[free_slot].rights = rights; service_backend_caps[free_slot].sources = sources;
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
            service_backend_caps[i].owner_pid = 0; service_backend_caps[i].grantee_pid = 0; service_backend_caps[i].rights = 0U; service_backend_caps[i].sources = 0U; service_backend_caps[i].prefix[0] = '\0'; service_backend_caps[i].name[0] = '\0';
            service_registry_backend_generation_bump(name);
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
            name_equal(service_backend_caps[i].name, name)) {
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
            name_equal(service_backend_caps[i].name, name)) {
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
    if (!service_registry_name_valid(name) || pid <= 0) return OS_SERVICE_BAD_NAME;
    for (i = 0U; i < SERVICE_REGISTRY_WATCH_CAPACITY; i++) {
        if (service_watches[i].pid == 0) {
            if (free_slot < 0) free_slot = (int)i;
            continue;
        }
        if (service_watches[i].pid == pid && name_equal(service_watches[i].name, name)) {
            return 0;
        }
    }
    if (free_slot < 0) return OS_SERVICE_WATCH_FULL;
    service_watches[free_slot].pid = pid;
    copy_name(service_watches[free_slot].name, name);
    return 0;
}

int service_registry_collect_watchers(const char* name, int32_t* out, uint32_t max) {
    uint32_t i;
    uint32_t count = 0U;
    if (!service_registry_name_valid(name) || (!out && max > 0U)) return OS_SERVICE_BAD_NAME;
    for (i = 0U; i < SERVICE_REGISTRY_WATCH_CAPACITY; i++) {
        if (service_watches[i].pid > 0 && name_equal(service_watches[i].name, name)) {
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
    return removed > 0 ? 0 : OS_SERVICE_NOT_FOUND;
}
