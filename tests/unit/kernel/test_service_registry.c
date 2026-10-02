#include "../../framework/unity.h"
#include "../../../kernel/service_registry.h"
#include "kernel/task/task.h"

static void test_registry_rejects_invalid_names(void) {
    char too_long[OS_SERVICE_NAME_MAX];
    uint32_t i;
    for (i = 0U; i < OS_SERVICE_NAME_MAX; i++) too_long[i] = 'a';
    service_registry_init();
    TEST_ASSERT_FALSE(service_registry_name_valid(""));
    TEST_ASSERT_FALSE(service_registry_name_valid("vfs/unsafe"));
    TEST_ASSERT_FALSE(service_registry_name_valid("bad name"));
    TEST_ASSERT_FALSE(service_registry_name_valid(too_long));
    TEST_ASSERT_EQUAL(OS_SERVICE_BAD_NAME, service_registry_register("bad name", 1));
}

static void test_registry_binds_name_to_owner_and_is_idempotent(void) {
    service_registry_init();
    TEST_ASSERT_EQUAL(0, service_registry_register("vfs", 7));
    TEST_ASSERT_EQUAL(7, service_registry_lookup("vfs"));
    TEST_ASSERT_EQUAL(0, service_registry_register("vfs", 7));
    TEST_ASSERT_EQUAL(OS_SERVICE_TAKEN, service_registry_register("vfs", 8));
}

static void test_registry_handles_capacity(void) {
    char name[3];
    uint32_t i;
    service_registry_init();
    name[2] = '\0';
    for (i = 0U; i < SERVICE_REGISTRY_CAPACITY; i++) {
        name[0] = 's';
        name[1] = (char)('0' + i);
        TEST_ASSERT_EQUAL(0, service_registry_register(name, (int32_t)(i + 1U)));
    }
    TEST_ASSERT_EQUAL(OS_SERVICE_FULL, service_registry_register("extra", 42));
}

static void test_registry_removal_allows_reuse(void) {
    service_registry_init();
    TEST_ASSERT_EQUAL(0, service_registry_register("vfs", 4));
    TEST_ASSERT_EQUAL(0, service_registry_remove("vfs", 4));
    TEST_ASSERT_EQUAL(OS_SERVICE_NOT_FOUND, service_registry_lookup("vfs"));
    TEST_ASSERT_EQUAL(0, service_registry_register("vfs", 9));
    TEST_ASSERT_EQUAL(9, service_registry_lookup("vfs"));
}

static void test_registry_refuses_removal_by_other_owner(void) {
    service_registry_init();
    TEST_ASSERT_EQUAL(0, service_registry_register("vfs", 4));
    TEST_ASSERT_EQUAL(OS_SERVICE_NOT_FOUND, service_registry_remove("vfs", 5));
    TEST_ASSERT_EQUAL(4, service_registry_lookup("vfs"));
    TEST_ASSERT_EQUAL(0, service_registry_remove("vfs", 4));
}

static void test_owner_can_grant_name_to_another_pid(void) {
    service_registry_init();
    TEST_ASSERT_EQUAL(0, service_registry_register("demo", 4));
    TEST_ASSERT_EQUAL(0, service_registry_grant("demo", 4, 9));
    TEST_ASSERT_EQUAL(9, service_registry_lookup("demo"));
    TEST_ASSERT_EQUAL(OS_SERVICE_TAKEN, service_registry_register("demo", 4));
    TEST_ASSERT_EQUAL(0, service_registry_register("demo", 9));
}

static void test_registry_refuses_grant_by_non_owner(void) {
    service_registry_init();
    TEST_ASSERT_EQUAL(0, service_registry_register("demo", 4));
    TEST_ASSERT_EQUAL(OS_SERVICE_NOT_OWNER, service_registry_grant("demo", 5, 9));
    TEST_ASSERT_EQUAL(OS_SERVICE_BAD_GRANTEE, service_registry_grant("demo", 4, 0));
    TEST_ASSERT_EQUAL(4, service_registry_lookup("demo"));
}

static void test_transferred_name_is_removed_with_grantee(void) {
    service_registry_init();
    TEST_ASSERT_EQUAL(0, service_registry_register("demo", 4));
    TEST_ASSERT_EQUAL(0, service_registry_grant("demo", 4, 9));
    TEST_ASSERT_EQUAL(0, service_registry_remove_pid(9));
    TEST_ASSERT_EQUAL(OS_SERVICE_NOT_FOUND, service_registry_lookup("demo"));
}

static void test_watchers_are_collected_and_idempotent(void) {
    int32_t watchers[2];
    service_registry_init();
    TEST_ASSERT_EQUAL(0, service_registry_subscribe("demo", 4));
    TEST_ASSERT_EQUAL(0, service_registry_subscribe("demo", 4));
    TEST_ASSERT_EQUAL(0, service_registry_subscribe("demo", 9));
    TEST_ASSERT_EQUAL(2, service_registry_collect_watchers("demo", watchers, 2U));
    TEST_ASSERT_EQUAL(4, watchers[0]);
    TEST_ASSERT_EQUAL(9, watchers[1]);
}

static void test_watcher_capacity_and_cleanup_are_bounded(void) {
    int32_t watchers[SERVICE_REGISTRY_WATCH_CAPACITY];
    uint32_t i;
    service_registry_init();
    for (i = 0U; i < SERVICE_REGISTRY_WATCH_CAPACITY; i++) {
        TEST_ASSERT_EQUAL(0, service_registry_subscribe("demo", (int32_t)(i + 1U)));
    }
    TEST_ASSERT_EQUAL(OS_SERVICE_WATCH_FULL, service_registry_subscribe("demo", 99));
    TEST_ASSERT_EQUAL(0, service_registry_remove_watcher_pid(4));
    TEST_ASSERT_EQUAL((int)SERVICE_REGISTRY_WATCH_CAPACITY - 1,
                      service_registry_collect_watchers("demo", watchers, SERVICE_REGISTRY_WATCH_CAPACITY));
}

static void test_owned_snapshot_survives_removal_for_notification(void) {
    service_registry_entry_t owned[2];
    service_registry_init();
    TEST_ASSERT_EQUAL(0, service_registry_register("vfs", 3));
    TEST_ASSERT_EQUAL(0, service_registry_register("logger", 3));
    TEST_ASSERT_EQUAL(2, service_registry_collect_owned(3, owned, 2U));
    TEST_ASSERT_EQUAL(3, owned[0].pid);
    TEST_ASSERT_EQUAL('v', owned[0].name[0]);
    TEST_ASSERT_EQUAL(0, service_registry_remove_pid(3));
    TEST_ASSERT_EQUAL('l', owned[1].name[0]);
}

static void test_owner_predicate_tracks_register_grant_and_remove(void) {
    service_registry_init();
    TEST_ASSERT_FALSE(service_registry_pid_is_owner(3));
    TEST_ASSERT_EQUAL(0, service_registry_register("demo", 3));
    TEST_ASSERT_TRUE(service_registry_pid_is_owner(3));
    TEST_ASSERT_EQUAL(0, service_registry_grant("demo", 3, 8));
    TEST_ASSERT_FALSE(service_registry_pid_is_owner(3));
    TEST_ASSERT_TRUE(service_registry_pid_is_owner(8));
    TEST_ASSERT_EQUAL(0, service_registry_remove_pid(8));
    TEST_ASSERT_FALSE(service_registry_pid_is_owner(8));
}

static void test_service_event_is_bounded_and_parsed(void) {
    os_ipc_payload_t payload;
    os_ipc_message_t message;
    os_service_event_t event;
    uint32_t i;
    TEST_ASSERT_EQUAL(0, os_service_make_event(&payload, "demo", 4, 9,
                                                OS_SERVICE_EVENT_GRANTED));
    TEST_ASSERT_EQUAL(OS_IPC_SERVICE_EVENT, payload.type);
    TEST_ASSERT_EQUAL(OS_SERVICE_EVENT_SIZE, payload.size);
    message.sender_pid = 0;
    message.type = payload.type;
    message.size = payload.size;
    message.request_id = payload.request_id;
    for (i = 0U; i < OS_IPC_MAX_DATA; i++) message.data[i] = payload.data[i];
    TEST_ASSERT_EQUAL(0, os_service_parse_event(&message, &event));
    TEST_ASSERT_EQUAL_STRING("demo", event.name);
    TEST_ASSERT_EQUAL(4, event.old_owner_pid);
    TEST_ASSERT_EQUAL(9, event.new_owner_pid);
    TEST_ASSERT_EQUAL(OS_SERVICE_EVENT_GRANTED, event.reason);
    message.sender_pid = 3;
    TEST_ASSERT_TRUE(os_service_parse_event(&message, &event) != 0);
}

static void test_remove_pid_clears_all_services_owned_by_task(void) {
    service_registry_init();
    TEST_ASSERT_EQUAL(0, service_registry_register("vfs", 3));
    TEST_ASSERT_EQUAL(0, service_registry_register("logger", 3));
    TEST_ASSERT_EQUAL(0, service_registry_remove_pid(3));
    TEST_ASSERT_EQUAL(OS_SERVICE_NOT_FOUND, service_registry_lookup("vfs"));
    TEST_ASSERT_EQUAL(OS_SERVICE_NOT_FOUND, service_registry_lookup("logger"));
}

static void test_backend_capability_is_revoked_on_transfer_and_pid_cleanup(void) {
    service_registry_init();
    TEST_ASSERT_EQUAL(0, service_registry_register("vfs", 3));
    TEST_ASSERT_EQUAL(0, service_registry_backend_grant("vfs", 3, 7));
    TEST_ASSERT_TRUE(service_registry_backend_allowed("vfs", 7));
    TEST_ASSERT_EQUAL(0, service_registry_grant("vfs", 3, 9));
    TEST_ASSERT_FALSE(service_registry_backend_allowed("vfs", 7));
    TEST_ASSERT_EQUAL(0, service_registry_backend_grant("vfs", 9, 7));
    TEST_ASSERT_TRUE(service_registry_backend_allowed("vfs", 7));
    service_registry_backend_remove_pid(7);
    TEST_ASSERT_FALSE(service_registry_backend_allowed("vfs", 7));
}

static void test_backend_capability_can_be_explicitly_revoked_without_name_transfer(void) {
    service_registry_init();
    TEST_ASSERT_EQUAL(0, service_registry_register("vfs", 3));
    TEST_ASSERT_EQUAL(0, service_registry_backend_grant("vfs", 3, 7));
    TEST_ASSERT_TRUE(service_registry_backend_allowed("vfs", 7));
    TEST_ASSERT_EQUAL(0, service_registry_backend_revoke("vfs", 3, 7));
    TEST_ASSERT_FALSE(service_registry_backend_allowed("vfs", 7));
    TEST_ASSERT_EQUAL(3, service_registry_lookup("vfs"));
    TEST_ASSERT_EQUAL(OS_SERVICE_NOT_FOUND, service_registry_backend_revoke("vfs", 3, 7));
    TEST_ASSERT_EQUAL(OS_SERVICE_NOT_OWNER, service_registry_backend_revoke("vfs", 9, 7));
}

static void test_backend_capability_scoped_read_only_enforces_least_privilege(void) {
    service_registry_init();
    TEST_ASSERT_EQUAL(0, service_registry_register("vfs", 3));
    TEST_ASSERT_EQUAL(0, service_registry_backend_grant_scoped("vfs", 3, 7, SERVICE_BACKEND_RIGHT_READ));
    TEST_ASSERT_TRUE(service_registry_backend_allowed_for("vfs", 7, SERVICE_BACKEND_RIGHT_READ));
    TEST_ASSERT_FALSE(service_registry_backend_allowed_for("vfs", 7, SERVICE_BACKEND_RIGHT_MUTATE));
    TEST_ASSERT_FALSE(service_registry_backend_allowed("vfs", 7));
    TEST_ASSERT_EQUAL(0, service_registry_backend_grant("vfs", 3, 8));
    TEST_ASSERT_TRUE(service_registry_backend_allowed_for("vfs", 8, SERVICE_BACKEND_RIGHT_READ));
    TEST_ASSERT_TRUE(service_registry_backend_allowed_for("vfs", 8, SERVICE_BACKEND_RIGHT_MUTATE));
    TEST_ASSERT_EQUAL(0, service_registry_backend_grant_scoped("vfs", 3, 9, SERVICE_BACKEND_RIGHT_MUTATE));
    TEST_ASSERT_FALSE(service_registry_backend_allowed_for("vfs", 9, SERVICE_BACKEND_RIGHT_READ));
    TEST_ASSERT_TRUE(service_registry_backend_allowed_for("vfs", 9, SERVICE_BACKEND_RIGHT_MUTATE));
    TEST_ASSERT_EQUAL(OS_SERVICE_BAD_NAME, service_registry_backend_grant_scoped("vfs", 3, 10, 4U));
}

static void test_backend_capability_source_scope_blocks_other_sources_and_generic_calls(void) {
    os_service_backend_scope_t scope;
    service_registry_init();
    TEST_ASSERT_EQUAL(0, service_registry_register("vfs", 3));
    TEST_ASSERT_EQUAL(0, service_registry_backend_grant_scoped_source(
        "vfs", 3, 7, SERVICE_BACKEND_RIGHT_READ, OS_SERVICE_BACKEND_SOURCE_INITRD));
    TEST_ASSERT_TRUE(service_registry_backend_allowed_for_source(
        "vfs", 7, SERVICE_BACKEND_RIGHT_READ, OS_SERVICE_BACKEND_SOURCE_INITRD));
    TEST_ASSERT_FALSE(service_registry_backend_allowed_for_source(
        "vfs", 7, SERVICE_BACKEND_RIGHT_READ, OS_SERVICE_BACKEND_SOURCE_OVERLAY));
    TEST_ASSERT_FALSE(service_registry_backend_allowed_for_source(
        "vfs", 7, SERVICE_BACKEND_RIGHT_READ, OS_SERVICE_BACKEND_SOURCE_FAT16));
    TEST_ASSERT_FALSE(service_registry_backend_allowed_for_source(
        "vfs", 7, SERVICE_BACKEND_RIGHT_MUTATE, OS_SERVICE_BACKEND_SOURCE_INITRD));
    TEST_ASSERT_FALSE(service_registry_backend_allowed_for("vfs", 7, SERVICE_BACKEND_RIGHT_READ));
    TEST_ASSERT_EQUAL(0, service_registry_backend_scope("vfs", 3, 7, &scope));
    TEST_ASSERT_EQUAL(SERVICE_BACKEND_RIGHT_READ, scope.rights);
    TEST_ASSERT_EQUAL(OS_SERVICE_BACKEND_SOURCE_INITRD, scope.sources);
    TEST_ASSERT_EQUAL(0, service_registry_backend_grant_scoped_source(
        "vfs", 3, 7, SERVICE_BACKEND_RIGHT_MUTATE, OS_SERVICE_BACKEND_SOURCE_FAT32));
    TEST_ASSERT_TRUE(service_registry_backend_allowed_for_source(
        "vfs", 7, SERVICE_BACKEND_RIGHT_READ, OS_SERVICE_BACKEND_SOURCE_INITRD));
    TEST_ASSERT_TRUE(service_registry_backend_allowed_for_source(
        "vfs", 7, SERVICE_BACKEND_RIGHT_MUTATE, OS_SERVICE_BACKEND_SOURCE_FAT32));
    TEST_ASSERT_FALSE(service_registry_backend_allowed_for_source(
        "vfs", 7, SERVICE_BACKEND_RIGHT_MUTATE, OS_SERVICE_BACKEND_SOURCE_FAT16));
    TEST_ASSERT_EQUAL(OS_SERVICE_BAD_NAME, service_registry_backend_grant_scoped_source(
        "vfs", 3, 7, SERVICE_BACKEND_RIGHT_READ, 0U));
    TEST_ASSERT_EQUAL(OS_SERVICE_BAD_NAME, service_registry_backend_grant_scoped_source(
        "vfs", 3, 7, SERVICE_BACKEND_RIGHT_READ, OS_SERVICE_BACKEND_SOURCE_ALL | 16U));
    TEST_ASSERT_EQUAL(0, service_registry_backend_grant_scoped("vfs", 3, 8,
                                                                SERVICE_BACKEND_RIGHT_READ));
    TEST_ASSERT_TRUE(service_registry_backend_allowed_for_source(
        "vfs", 8, SERVICE_BACKEND_RIGHT_READ, OS_SERVICE_BACKEND_SOURCE_ALL));
    TEST_ASSERT_TRUE(service_registry_backend_allowed_for("vfs", 8, SERVICE_BACKEND_RIGHT_READ));
}

static void test_backend_capability_prefix_scope_blocks_siblings_and_pathless_calls(void) {
    service_registry_init();
    TEST_ASSERT_EQUAL(0, service_registry_register("vfs", 3));
    TEST_ASSERT_EQUAL(0, service_registry_backend_grant_scoped_source_prefix(
        "vfs", 3, 7, SERVICE_BACKEND_RIGHT_READ, OS_SERVICE_BACKEND_SOURCE_FAT16, "apps/"));
    TEST_ASSERT_TRUE(service_registry_backend_allowed_for_source_path(
        "vfs", 7, SERVICE_BACKEND_RIGHT_READ, OS_SERVICE_BACKEND_SOURCE_FAT16, "apps/demo.txt"));
    TEST_ASSERT_TRUE(service_registry_backend_allowed_for_source_path(
        "vfs", 7, SERVICE_BACKEND_RIGHT_READ, OS_SERVICE_BACKEND_SOURCE_FAT16, "apps/bin/demo.txt"));
    TEST_ASSERT_TRUE(service_registry_backend_allowed_for_source_path(
        "vfs", 7, SERVICE_BACKEND_RIGHT_READ, OS_SERVICE_BACKEND_SOURCE_FAT16, "apps/"));
    TEST_ASSERT_FALSE(service_registry_backend_allowed_for_source_path(
        "vfs", 7, SERVICE_BACKEND_RIGHT_READ, OS_SERVICE_BACKEND_SOURCE_FAT16, "appstore/demo.txt"));
    TEST_ASSERT_FALSE(service_registry_backend_allowed_for_source_path(
        "vfs", 7, SERVICE_BACKEND_RIGHT_READ, OS_SERVICE_BACKEND_SOURCE_FAT16, "other/demo.txt"));
    TEST_ASSERT_FALSE(service_registry_backend_allowed_for_source(
        "vfs", 7, SERVICE_BACKEND_RIGHT_READ, OS_SERVICE_BACKEND_SOURCE_FAT16));
    TEST_ASSERT_FALSE(service_registry_backend_allowed_for_source_path(
        "vfs", 7, SERVICE_BACKEND_RIGHT_MUTATE, OS_SERVICE_BACKEND_SOURCE_FAT16, "apps/demo.txt"));
    TEST_ASSERT_FALSE(service_registry_backend_allowed_for_source_path(
        "vfs", 7, SERVICE_BACKEND_RIGHT_READ, OS_SERVICE_BACKEND_SOURCE_FAT32, "apps/demo.txt"));
    TEST_ASSERT_EQUAL(OS_SERVICE_BAD_NAME, service_registry_backend_grant_scoped_source_prefix(
        "vfs", 3, 7, SERVICE_BACKEND_RIGHT_READ, OS_SERVICE_BACKEND_SOURCE_FAT16, "apps"));
    TEST_ASSERT_EQUAL(OS_SERVICE_BAD_NAME, service_registry_backend_grant_scoped_source_prefix(
        "vfs", 3, 7, SERVICE_BACKEND_RIGHT_READ, OS_SERVICE_BACKEND_SOURCE_FAT16, "/apps/"));
    TEST_ASSERT_EQUAL(OS_SERVICE_BAD_NAME, service_registry_backend_grant_scoped_source_prefix(
        "vfs", 3, 7, SERVICE_BACKEND_RIGHT_READ, OS_SERVICE_BACKEND_SOURCE_FAT16, "apps//bin/"));
    TEST_ASSERT_EQUAL(OS_SERVICE_BAD_NAME, service_registry_backend_grant_scoped_source_prefix(
        "vfs", 3, 7, SERVICE_BACKEND_RIGHT_READ, OS_SERVICE_BACKEND_SOURCE_FAT16, "apps/../other/"));
    TEST_ASSERT_EQUAL(0, service_registry_backend_grant_scoped_source_prefix(
        "vfs", 3, 7, SERVICE_BACKEND_RIGHT_MUTATE, OS_SERVICE_BACKEND_SOURCE_FAT16, "other/"));
    TEST_ASSERT_TRUE(service_registry_backend_allowed_for_source_path(
        "vfs", 7, SERVICE_BACKEND_RIGHT_READ, OS_SERVICE_BACKEND_SOURCE_FAT16, "other/demo.txt"));
    TEST_ASSERT_TRUE(service_registry_backend_allowed_for_source_path(
        "vfs", 7, SERVICE_BACKEND_RIGHT_MUTATE, OS_SERVICE_BACKEND_SOURCE_FAT16, "other/demo.txt"));
}

static void test_backend_capability_cumulative_grants(void) {
    service_registry_init();
    TEST_ASSERT_EQUAL(0, service_registry_register("vfs", 3));
    TEST_ASSERT_EQUAL(0, service_registry_backend_grant_scoped_source(
        "vfs", 3, 7, SERVICE_BACKEND_RIGHT_READ, OS_SERVICE_BACKEND_SOURCE_INITRD));
    TEST_ASSERT_TRUE(service_registry_backend_allowed_for_source(
        "vfs", 7, SERVICE_BACKEND_RIGHT_READ, OS_SERVICE_BACKEND_SOURCE_INITRD));
    TEST_ASSERT_FALSE(service_registry_backend_allowed_for_source(
        "vfs", 7, SERVICE_BACKEND_RIGHT_MUTATE, OS_SERVICE_BACKEND_SOURCE_FAT16));

    /* Seconde attribution cumulée : ajoute le droit MUTATE et la source FAT16 */
    TEST_ASSERT_EQUAL(0, service_registry_backend_grant_scoped_source(
        "vfs", 3, 7, SERVICE_BACKEND_RIGHT_MUTATE, OS_SERVICE_BACKEND_SOURCE_FAT16));
    TEST_ASSERT_TRUE(service_registry_backend_allowed_for_source(
        "vfs", 7, SERVICE_BACKEND_RIGHT_READ, OS_SERVICE_BACKEND_SOURCE_INITRD));
    TEST_ASSERT_TRUE(service_registry_backend_allowed_for_source(
        "vfs", 7, SERVICE_BACKEND_RIGHT_MUTATE, OS_SERVICE_BACKEND_SOURCE_FAT16));
}

static void test_backend_capability_rights_are_owner_scoped_and_revocable(void) {
    uint32_t rights = 0U;
    service_registry_init();
    TEST_ASSERT_EQUAL(0, service_registry_register("vfs", 3));
    TEST_ASSERT_EQUAL(0, service_registry_backend_grant_scoped("vfs", 3, 7, SERVICE_BACKEND_RIGHT_MUTATE));
    TEST_ASSERT_EQUAL(0, service_registry_backend_rights("vfs", 3, 7, &rights));
    TEST_ASSERT_EQUAL(SERVICE_BACKEND_RIGHT_MUTATE, rights);
    TEST_ASSERT_EQUAL(OS_SERVICE_NOT_OWNER, service_registry_backend_rights("vfs", 9, 7, &rights));
    TEST_ASSERT_EQUAL(0, service_registry_backend_revoke("vfs", 3, 7));
    TEST_ASSERT_EQUAL(OS_SERVICE_NOT_FOUND, service_registry_backend_rights("vfs", 3, 7, &rights));
}

static void test_backend_capability_list_is_owner_scoped_and_tracks_revocation(void) {
    os_service_backend_list_t list;
    service_registry_init();
    TEST_ASSERT_EQUAL(0, service_registry_register("vfs", 3));
    TEST_ASSERT_EQUAL(0, service_registry_backend_grant_scoped("vfs", 3, 7, SERVICE_BACKEND_RIGHT_READ));
    TEST_ASSERT_EQUAL(0, service_registry_backend_grant_scoped("vfs", 3, 8, SERVICE_BACKEND_RIGHT_MUTATE));
    TEST_ASSERT_EQUAL(0, service_registry_backend_grant("vfs", 3, 9));
    TEST_ASSERT_EQUAL(0, service_registry_backend_list("vfs", 3, &list));
    TEST_ASSERT_EQUAL(3U, list.count);
    TEST_ASSERT_EQUAL(7, list.entries[0].pid); TEST_ASSERT_EQUAL(SERVICE_BACKEND_RIGHT_READ, list.entries[0].rights);
    TEST_ASSERT_EQUAL(8, list.entries[1].pid); TEST_ASSERT_EQUAL(SERVICE_BACKEND_RIGHT_MUTATE, list.entries[1].rights);
    TEST_ASSERT_EQUAL(9, list.entries[2].pid); TEST_ASSERT_EQUAL(SERVICE_BACKEND_RIGHT_ALL, list.entries[2].rights);
    TEST_ASSERT_EQUAL(OS_SERVICE_NOT_OWNER, service_registry_backend_list("vfs", 4, &list));
    TEST_ASSERT_EQUAL(0U, list.count); TEST_ASSERT_EQUAL(0, list.entries[0].pid); TEST_ASSERT_EQUAL(0U, list.entries[0].rights);
    TEST_ASSERT_EQUAL(0, service_registry_backend_revoke("vfs", 3, 8));
    TEST_ASSERT_EQUAL(0, service_registry_backend_list("vfs", 3, &list));
    TEST_ASSERT_EQUAL(2U, list.count);
    TEST_ASSERT_EQUAL(7, list.entries[0].pid); TEST_ASSERT_EQUAL(9, list.entries[1].pid);
}

static void test_backend_capability_can_be_released_by_its_grantee(void) {
    os_service_backend_snapshot_t snapshot;
    service_registry_init();
    TEST_ASSERT_EQUAL(0, service_registry_register("vfs", 3));
    TEST_ASSERT_EQUAL(0, service_registry_backend_grant_scoped("vfs", 3, 7,
                                                                SERVICE_BACKEND_RIGHT_READ));
    TEST_ASSERT_TRUE(service_registry_backend_allowed_for("vfs", 7, SERVICE_BACKEND_RIGHT_READ));
    TEST_ASSERT_EQUAL(0, service_registry_backend_release("vfs", 7));
    TEST_ASSERT_FALSE(service_registry_backend_allowed_for("vfs", 7, SERVICE_BACKEND_RIGHT_READ));
    TEST_ASSERT_EQUAL(OS_SERVICE_NOT_FOUND, service_registry_backend_release("vfs", 7));
    TEST_ASSERT_EQUAL(OS_SERVICE_BAD_NAME, service_registry_backend_release("bad name", 7));
    TEST_ASSERT_EQUAL(0, service_registry_backend_observe("vfs", 3, 0U, &snapshot));
    TEST_ASSERT_EQUAL(3U, snapshot.generation);
    TEST_ASSERT_EQUAL(0U, snapshot.list.count);
}

static void test_backend_observe_detects_stale_generations_without_disclosure(void) {
    os_service_backend_snapshot_t snapshot;
    service_registry_init();
    TEST_ASSERT_EQUAL(0, service_registry_register("vfs", 3));
    TEST_ASSERT_EQUAL(0, service_registry_register("demo", 8));
    TEST_ASSERT_EQUAL(0, service_registry_backend_observe("vfs", 3, 0U, &snapshot));
    TEST_ASSERT_EQUAL(1U, snapshot.generation); TEST_ASSERT_EQUAL(0U, snapshot.list.count);
    TEST_ASSERT_EQUAL(0, service_registry_backend_grant_scoped("demo", 8, 9, SERVICE_BACKEND_RIGHT_READ));
    TEST_ASSERT_EQUAL(0, service_registry_backend_observe("vfs", 3, 1U, &snapshot));
    TEST_ASSERT_EQUAL(1U, snapshot.generation); TEST_ASSERT_EQUAL(0U, snapshot.list.count);
    TEST_ASSERT_EQUAL(0, service_registry_backend_grant_scoped("vfs", 3, 7, SERVICE_BACKEND_RIGHT_READ));
    TEST_ASSERT_EQUAL(OS_SERVICE_STALE, service_registry_backend_observe("vfs", 3, 1U, &snapshot));
    TEST_ASSERT_EQUAL(2U, snapshot.generation); TEST_ASSERT_EQUAL(0U, snapshot.list.count);
    TEST_ASSERT_EQUAL(0, service_registry_backend_observe("vfs", 3, 2U, &snapshot));
    TEST_ASSERT_EQUAL(1U, snapshot.list.count); TEST_ASSERT_EQUAL(7, snapshot.list.entries[0].pid);
    TEST_ASSERT_EQUAL(0, service_registry_backend_revoke("vfs", 3, 7));
    TEST_ASSERT_EQUAL(OS_SERVICE_STALE, service_registry_backend_observe("vfs", 3, 2U, &snapshot));
    TEST_ASSERT_EQUAL(3U, snapshot.generation); TEST_ASSERT_EQUAL(0U, snapshot.list.count);
    TEST_ASSERT_EQUAL(OS_SERVICE_NOT_OWNER, service_registry_backend_observe("vfs", 4, 0U, &snapshot));
    TEST_ASSERT_EQUAL(0U, snapshot.generation); TEST_ASSERT_EQUAL(0U, snapshot.list.count);
}


static void test_owner_fat_bypass_closes_when_storage_worker_live(void) {
    /* AOS-2172: ATA/FAT owner bypass only in degraded mode (no vfs-virtual). */
    service_registry_init();
    TEST_ASSERT_EQUAL(0, service_registry_register("vfs", 3));
    TEST_ASSERT_TRUE(service_registry_owner_bypasses_backend(
        "vfs", 3, OS_SERVICE_BACKEND_SOURCE_FAT16));
    TEST_ASSERT_TRUE(service_registry_owner_bypasses_backend(
        "vfs", 3, OS_SERVICE_BACKEND_SOURCE_FAT32));
    TEST_ASSERT_FALSE(service_registry_owner_bypasses_backend(
        "vfs", 9, OS_SERVICE_BACKEND_SOURCE_FAT16));
    TEST_ASSERT_EQUAL(0, service_registry_register("vfs-virtual", 11));
    TEST_ASSERT_FALSE(service_registry_owner_bypasses_backend(
        "vfs", 3, OS_SERVICE_BACKEND_SOURCE_FAT16));
    TEST_ASSERT_FALSE(service_registry_owner_bypasses_backend(
        "vfs", 3, OS_SERVICE_BACKEND_SOURCE_FAT32));
    /* Explicit grant still authorizes the worker path separately. */
    TEST_ASSERT_EQUAL(0, service_registry_backend_grant_scoped_source_prefix(
        "vfs", 3, 11, SERVICE_BACKEND_RIGHT_READ, OS_SERVICE_BACKEND_SOURCE_FAT16, ""));
    TEST_ASSERT_TRUE(service_registry_backend_allowed_for_source_path(
        "vfs", 11, SERVICE_BACKEND_RIGHT_READ, OS_SERVICE_BACKEND_SOURCE_FAT16, "HI.TXT"));
    TEST_ASSERT_EQUAL(0, service_registry_remove("vfs-virtual", 11));
    TEST_ASSERT_TRUE(service_registry_owner_bypasses_backend(
        "vfs", 3, OS_SERVICE_BACKEND_SOURCE_FAT16));
}

static void test_owner_initrd_overlay_bypass_closes_when_storage_worker_live(void) {
    /* AOS-2173: initrd/overlay owner bypass only in degraded mode (no vfs-virtual). */
    service_registry_init();
    TEST_ASSERT_EQUAL(0, service_registry_register("vfs", 3));
    TEST_ASSERT_TRUE(service_registry_owner_bypasses_backend(
        "vfs", 3, OS_SERVICE_BACKEND_SOURCE_INITRD));
    TEST_ASSERT_TRUE(service_registry_owner_bypasses_backend(
        "vfs", 3, OS_SERVICE_BACKEND_SOURCE_OVERLAY));
    TEST_ASSERT_FALSE(service_registry_owner_bypasses_backend(
        "vfs", 9, OS_SERVICE_BACKEND_SOURCE_INITRD));
    TEST_ASSERT_EQUAL(0, service_registry_register("vfs-virtual", 11));
    TEST_ASSERT_FALSE(service_registry_owner_bypasses_backend(
        "vfs", 3, OS_SERVICE_BACKEND_SOURCE_INITRD));
    TEST_ASSERT_FALSE(service_registry_owner_bypasses_backend(
        "vfs", 3, OS_SERVICE_BACKEND_SOURCE_OVERLAY));
    /* Explicit grant still authorizes the worker path separately. */
    TEST_ASSERT_EQUAL(0, service_registry_backend_grant_scoped_source_prefix(
        "vfs", 3, 11, SERVICE_BACKEND_RIGHT_READ, OS_SERVICE_BACKEND_SOURCE_INITRD, ""));
    TEST_ASSERT_TRUE(service_registry_backend_allowed_for_source_path(
        "vfs", 11, SERVICE_BACKEND_RIGHT_READ, OS_SERVICE_BACKEND_SOURCE_INITRD, "hello.txt"));
    TEST_ASSERT_EQUAL(0, service_registry_backend_grant_scoped_source_prefix(
        "vfs", 3, 11, SERVICE_BACKEND_RIGHT_MUTATE, OS_SERVICE_BACKEND_SOURCE_OVERLAY, ""));
    TEST_ASSERT_TRUE(service_registry_backend_allowed_for_source_path(
        "vfs", 11, SERVICE_BACKEND_RIGHT_MUTATE, OS_SERVICE_BACKEND_SOURCE_OVERLAY, "note.txt"));
    TEST_ASSERT_EQUAL(0, service_registry_remove("vfs-virtual", 11));
    TEST_ASSERT_TRUE(service_registry_owner_bypasses_backend(
        "vfs", 3, OS_SERVICE_BACKEND_SOURCE_INITRD));
    TEST_ASSERT_TRUE(service_registry_owner_bypasses_backend(
        "vfs", 3, OS_SERVICE_BACKEND_SOURCE_OVERLAY));
}

static void test_owner_ata_generic_bypass_closes_when_storage_worker_live(void) {
    /* AOS-2174: SOURCE_ALL / ATA-backed generic backend owner bypass only
     * in degraded mode (no vfs-virtual). Combinations close the same way. */
    uint32_t ata_backed = OS_SERVICE_BACKEND_SOURCE_OVERLAY | OS_SERVICE_BACKEND_SOURCE_FAT16;
    service_registry_init();
    TEST_ASSERT_EQUAL(0, service_registry_register("vfs", 3));
    TEST_ASSERT_TRUE(service_registry_owner_bypasses_backend(
        "vfs", 3, OS_SERVICE_BACKEND_SOURCE_ALL));
    TEST_ASSERT_TRUE(service_registry_owner_bypasses_backend(
        "vfs", 3, ata_backed));
    TEST_ASSERT_FALSE(service_registry_owner_bypasses_backend(
        "vfs", 9, OS_SERVICE_BACKEND_SOURCE_ALL));
    TEST_ASSERT_EQUAL(0, service_registry_register("vfs-virtual", 11));
    TEST_ASSERT_FALSE(service_registry_owner_bypasses_backend(
        "vfs", 3, OS_SERVICE_BACKEND_SOURCE_ALL));
    TEST_ASSERT_FALSE(service_registry_owner_bypasses_backend(
        "vfs", 3, ata_backed));
    /* Explicit full-scope grant still authorizes the worker path. */
    TEST_ASSERT_EQUAL(0, service_registry_backend_grant_scoped_source_prefix(
        "vfs", 3, 11, SERVICE_BACKEND_RIGHT_READ, OS_SERVICE_BACKEND_SOURCE_ALL, ""));
    TEST_ASSERT_TRUE(service_registry_backend_allowed_for_source_path(
        "vfs", 11, SERVICE_BACKEND_RIGHT_READ, OS_SERVICE_BACKEND_SOURCE_ALL, "note.txt"));
    TEST_ASSERT_EQUAL(0, service_registry_remove("vfs-virtual", 11));
    TEST_ASSERT_TRUE(service_registry_owner_bypasses_backend(
        "vfs", 3, OS_SERVICE_BACKEND_SOURCE_ALL));
    TEST_ASSERT_TRUE(service_registry_owner_bypasses_backend(
        "vfs", 3, ata_backed));
    /* Handoff while worker live: name grant issues SOURCE_ALL to new owner. */
    TEST_ASSERT_EQUAL(0, service_registry_register("vfs-virtual", 11));
    TEST_ASSERT_EQUAL(0, service_registry_grant("vfs", 3, 9));
    TEST_ASSERT_FALSE(service_registry_owner_bypasses_backend(
        "vfs", 9, OS_SERVICE_BACKEND_SOURCE_ALL));
    TEST_ASSERT_TRUE(service_registry_backend_allowed_for_source_path(
        "vfs", 9, SERVICE_BACKEND_RIGHT_READ, OS_SERVICE_BACKEND_SOURCE_ALL, "hello.txt"));
}

static void test_ata_overlay_io_only_via_worker_when_live(void) {
    /* AOS-2175: ATA-backed overlay read/stat slice is worker-mediated when
     * vfs-virtual is published. Owner grants do not authorize local overlay
     * exercise; degraded mode without the worker remains open. */
    service_registry_init();
    TEST_ASSERT_EQUAL(0, service_registry_register("vfs", 3));
    /* No worker: any positive pid may exercise (historical local path). */
    TEST_ASSERT_TRUE(service_registry_ata_overlay_io_via_worker(3));
    TEST_ASSERT_TRUE(service_registry_ata_overlay_io_via_worker(11));
    TEST_ASSERT_FALSE(service_registry_ata_overlay_io_via_worker(0));
    TEST_ASSERT_EQUAL(0, service_registry_register("vfs-virtual", 11));
    /* Worker live: only the published vfs-virtual PID. */
    TEST_ASSERT_TRUE(service_registry_ata_overlay_io_via_worker(11));
    TEST_ASSERT_FALSE(service_registry_ata_overlay_io_via_worker(3));
    TEST_ASSERT_FALSE(service_registry_ata_overlay_io_via_worker(9));
    /* Owner grant does not change the mediation gate. */
    TEST_ASSERT_EQUAL(0, service_registry_backend_grant_scoped_source_prefix(
        "vfs", 3, 3, SERVICE_BACKEND_RIGHT_READ, OS_SERVICE_BACKEND_SOURCE_OVERLAY, ""));
    TEST_ASSERT_FALSE(service_registry_ata_overlay_io_via_worker(3));
    TEST_ASSERT_EQUAL(0, service_registry_remove("vfs-virtual", 11));
    TEST_ASSERT_TRUE(service_registry_ata_overlay_io_via_worker(3));
}

static void test_historical_readfile_writefile_gate_when_worker_live(void) {
    /* AOS-2177: historical SYS_READFILE keeps the full overlay+initrd path in
     * degraded mode and for the worker; otherwise overlay hits need the worker
     * and initrd-only reads stay open. SYS_WRITEFILE uses the shared gate. */
    service_registry_init();
    TEST_ASSERT_EQUAL(0, service_registry_register("vfs", 3));
    TEST_ASSERT_EQUAL(SERVICE_HIST_READ_DENIED, service_registry_historical_read_decision(0, 0));
    TEST_ASSERT_EQUAL(SERVICE_HIST_READ_FULL, service_registry_historical_read_decision(7, 1));
    TEST_ASSERT_EQUAL(SERVICE_HIST_READ_FULL, service_registry_historical_read_decision(7, 0));
    TEST_ASSERT_TRUE(service_registry_ata_overlay_io_via_worker(7));
    TEST_ASSERT_EQUAL(0, service_registry_register("vfs-virtual", 11));
    TEST_ASSERT_EQUAL(SERVICE_HIST_READ_FULL, service_registry_historical_read_decision(11, 1));
    TEST_ASSERT_EQUAL(SERVICE_HIST_READ_WORKER_REQUIRED, service_registry_historical_read_decision(7, 1));
    TEST_ASSERT_EQUAL(SERVICE_HIST_READ_WORKER_REQUIRED, service_registry_historical_read_decision(3, 1));
    TEST_ASSERT_EQUAL(SERVICE_HIST_READ_INITRD_ONLY, service_registry_historical_read_decision(7, 0));
    TEST_ASSERT_FALSE(service_registry_ata_overlay_io_via_worker(7));
    TEST_ASSERT_EQUAL(0, service_registry_remove("vfs-virtual", 11));
    TEST_ASSERT_EQUAL(SERVICE_HIST_READ_FULL, service_registry_historical_read_decision(7, 1));
    TEST_ASSERT_TRUE(service_registry_ata_overlay_io_via_worker(7));
}

static void test_historical_overlay_entry_points_gate_when_worker_live(void) {
    /* AOS-2178: SYS_STAT/SYS_LISTDIR reuse the historical read split (overlay
     * hit or overlay-only dir needs the worker, initrd stays open); SYS_MKDIR,
     * SYS_UNLINK, SYS_RENAME, SYS_COPY, SYS_APPEND use the shared worker gate. */
    service_registry_init();
    TEST_ASSERT_EQUAL(0, service_registry_register("vfs", 3));
    TEST_ASSERT_EQUAL(0, service_registry_register("vfs-virtual", 11));
    /* stat of an overlay file / list of an overlay-only directory. */
    TEST_ASSERT_EQUAL(SERVICE_HIST_READ_WORKER_REQUIRED, service_registry_historical_read_decision(5, 1));
    /* stat of an initrd file / list of an initrd directory. */
    TEST_ASSERT_EQUAL(SERVICE_HIST_READ_INITRD_ONLY, service_registry_historical_read_decision(5, 0));
    /* mutations. */
    TEST_ASSERT_FALSE(service_registry_ata_overlay_io_via_worker(5));
    TEST_ASSERT_FALSE(service_registry_ata_overlay_io_via_worker(3));
    TEST_ASSERT_TRUE(service_registry_ata_overlay_io_via_worker(11));
    /* worker disappears: degraded historical path reopens for everyone. */
    service_registry_remove_pid(11);
    TEST_ASSERT_EQUAL(SERVICE_HIST_READ_FULL, service_registry_historical_read_decision(5, 1));
    TEST_ASSERT_TRUE(service_registry_ata_overlay_io_via_worker(5));
}

static void test_notify_ack_and_history_persistence(void) {
    uint32_t seq1 = 0U, seq2 = 0U;
    uint32_t acked = 0U, unacked = 0U;
    service_registry_init();

    TEST_ASSERT_EQUAL(0, service_registry_notify_record("vfs", 4, 3, 9, OS_SERVICE_EVENT_GRANTED, &seq1));
    TEST_ASSERT_TRUE(seq1 > 0U);
    TEST_ASSERT_EQUAL(0, service_registry_notify_record("vfs", 4, 9, 0, OS_SERVICE_EVENT_UNREGISTERED, &seq2));
    TEST_ASSERT_TRUE(seq2 > seq1);

    TEST_ASSERT_EQUAL(0, service_registry_notify_history_count(4, &acked, &unacked));
    TEST_ASSERT_EQUAL(0U, acked);
    TEST_ASSERT_EQUAL(2U, unacked);

    TEST_ASSERT_EQUAL(0, service_registry_notify_ack(4, seq1));
    TEST_ASSERT_EQUAL(0, service_registry_notify_history_count(4, &acked, &unacked));
    TEST_ASSERT_EQUAL(1U, acked);
    TEST_ASSERT_EQUAL(1U, unacked);

    TEST_ASSERT_EQUAL(0, service_registry_notify_ack(4, seq2));
    TEST_ASSERT_EQUAL(0, service_registry_notify_history_count(4, &acked, &unacked));
    TEST_ASSERT_EQUAL(2U, acked);
    TEST_ASSERT_EQUAL(0U, unacked);

    TEST_ASSERT_EQUAL(OS_SERVICE_NOT_FOUND, service_registry_notify_ack(4, 9999U));
}

static void test_ata_driver_name_and_port_grant(void) {
    /* Tranche 4: only the atadriver binary may hold "ata-driver", and only
     * that live owner gets the ATA ports opened in the TSS IOPB. */
    service_registry_init();
    TEST_ASSERT_TRUE(service_registry_ata_driver_name_allowed("ata-driver", "atadriver"));
    TEST_ASSERT_FALSE(service_registry_ata_driver_name_allowed("ata-driver", "atarogue"));
    TEST_ASSERT_FALSE(service_registry_ata_driver_name_allowed("ata-driver", "shell"));
    TEST_ASSERT_FALSE(service_registry_ata_driver_name_allowed("ata-driver", "atadriverx"));
    TEST_ASSERT_FALSE(service_registry_ata_driver_name_allowed("ata-driver", 0));
    TEST_ASSERT_TRUE(service_registry_ata_driver_name_allowed("vfs", "shell"));
    TEST_ASSERT_TRUE(service_registry_ata_driver_name_allowed("ata-client", "ataclient"));
    TEST_ASSERT_FALSE(service_registry_ata_driver_name_allowed("ata-client", "atarogue"));
    TEST_ASSERT_FALSE(service_registry_ata_driver_name_allowed("ata-client", 0));
    /* No driver: nobody gets the ports. */
    TEST_ASSERT_FALSE(service_registry_ata_ports_granted(4));
    TEST_ASSERT_FALSE(service_registry_ata_ports_granted(0));
    TEST_ASSERT_EQUAL(0, service_registry_register("ata-driver", 4));
    TEST_ASSERT_TRUE(service_registry_ata_ports_granted(4));
    TEST_ASSERT_FALSE(service_registry_ata_ports_granted(5));
    /* A vfs-virtual worker gets no port capability. */
    TEST_ASSERT_EQUAL(0, service_registry_register("vfs-virtual", 6));
    TEST_ASSERT_FALSE(service_registry_ata_ports_granted(6));
    /* Driver gone: grant disappears. */
    service_registry_remove_pid(4);
    TEST_ASSERT_FALSE(service_registry_ata_ports_granted(4));
    TEST_ASSERT_EQUAL(-58, OS_ATA_DRIVER_REQUIRED);
    TEST_ASSERT_TRUE(OS_ATA_DRIVER_REQUIRED != OS_VFS_BACKEND_DENIED);
    TEST_ASSERT_TRUE(OS_ATA_DRIVER_REQUIRED != OS_TASK_NOT_CHILD);
}

static void test_stable_identity_blocks_stale_reused_pid_acl(void) {
    task_t* task_owner;
    task_t* task_grantee1;
    task_t* task_grantee2;

    tasking_init();
    service_registry_init();

    task_owner = create_task((void*)0);
    add_task_to_queue(task_owner);

    task_grantee1 = create_task((void*)0);
    add_task_to_queue(task_grantee1);

    TEST_ASSERT_EQUAL(0, service_registry_register("vfs", task_owner->id));
    TEST_ASSERT_EQUAL(0, service_registry_backend_grant("vfs", task_owner->id, task_grantee1->id));
    TEST_ASSERT_TRUE(service_registry_backend_allowed_for("vfs", task_grantee1->id, SERVICE_BACKEND_RIGHT_READ));

    /* Simulate task_grantee1 termination/removal and slot reuse with same PID */
    int grantee_pid = task_grantee1->id;
    remove_task(task_grantee1);

    /* Create new task that happens to get same PID or slot */
    next_task_id = grantee_pid;
    task_grantee2 = create_task((void*)0);
    add_task_to_queue(task_grantee2);
    TEST_ASSERT_EQUAL(grantee_pid, task_grantee2->id);

    /* Stable identity check must deny task_grantee2 because sequence/generation differ! */
    TEST_ASSERT_FALSE(service_registry_backend_allowed_for("vfs", grantee_pid, SERVICE_BACKEND_RIGHT_READ));
}

static void test_acknowledged_event_replay_and_bounded_retry(void) {
    uint32_t seq = 0U;
    service_registry_init();

    TEST_ASSERT_EQUAL(0, service_registry_notify_record("vfs", 4, 3, 9, OS_SERVICE_EVENT_GRANTED, &seq));
    TEST_ASSERT_EQUAL(0, service_registry_notify_is_acked(4, seq));

    /* Replay event on timeout */
    TEST_ASSERT_EQUAL(0, service_registry_notify_replay(4, seq));

    /* Second replay attempt must be rejected (bounded to once) */
    TEST_ASSERT_EQUAL(OS_SERVICE_FULL, service_registry_notify_replay(4, seq));

    /* Acknowledge event */
    TEST_ASSERT_EQUAL(0, service_registry_notify_ack(4, seq));
    TEST_ASSERT_EQUAL(1, service_registry_notify_is_acked(4, seq));

    /* Replay after ack must be rejected */
    TEST_ASSERT_EQUAL(OS_SERVICE_STALE, service_registry_notify_replay(4, seq));
}

static void test_backend_token_is_nonzero_unique_and_required(void) {
    uint32_t first = 0U;
    uint32_t second = 0U;
    service_registry_init();
    TEST_ASSERT_EQUAL(0, service_registry_register("vfs", 3));
    TEST_ASSERT_EQUAL(OS_SERVICE_NOT_FOUND, service_registry_backend_token_of("vfs", 7, &first));
    TEST_ASSERT_EQUAL(0, service_registry_backend_grant("vfs", 3, 7));
    TEST_ASSERT_EQUAL(0, service_registry_backend_grant("vfs", 3, 8));
    TEST_ASSERT_EQUAL(0, service_registry_backend_token_of("vfs", 7, &first));
    TEST_ASSERT_EQUAL(0, service_registry_backend_token_of("vfs", 8, &second));
    TEST_ASSERT_TRUE(first != 0U);
    TEST_ASSERT_TRUE(second != 0U);
    TEST_ASSERT_TRUE(first != second);
    TEST_ASSERT_TRUE(service_registry_backend_allowed("vfs", 7));
    TEST_ASSERT_EQUAL(0, service_registry_backend_clear_token("vfs", 7));
    TEST_ASSERT_FALSE(service_registry_backend_allowed("vfs", 7));
    TEST_ASSERT_EQUAL(OS_SERVICE_NOT_FOUND, service_registry_backend_token_of("vfs", 7, &first));
    TEST_ASSERT_TRUE(service_registry_backend_allowed("vfs", 8));
}

static void test_notify_pull_returns_oldest_without_ipc(void) {
    service_registry_notify_event_t event;
    uint32_t first = 0U;
    uint32_t second = 0U;
    service_registry_init();
    TEST_ASSERT_EQUAL(0, service_registry_notify_record("vfs", 4, 3, 9, OS_SERVICE_EVENT_GRANTED, &first));
    TEST_ASSERT_EQUAL(0, service_registry_notify_record("vfs", 4, 9, 11, OS_SERVICE_EVENT_PUBLISHED, &second));
    TEST_ASSERT_EQUAL(OS_SERVICE_NOT_FOUND, service_registry_notify_pull(5, &event));
    TEST_ASSERT_EQUAL(0, service_registry_notify_pull(4, &event));
    TEST_ASSERT_EQUAL(first, event.sequence);
    TEST_ASSERT_EQUAL(3, event.old_pid);
    TEST_ASSERT_EQUAL(9, event.new_pid);
    TEST_ASSERT_EQUAL(1, service_registry_notify_is_acked(4, first));
    TEST_ASSERT_EQUAL(0, service_registry_notify_pull(4, &event));
    TEST_ASSERT_EQUAL(second, event.sequence);
    TEST_ASSERT_EQUAL(OS_SERVICE_NOT_FOUND, service_registry_notify_pull(4, &event));
}

static void test_identity_key_blocks_cloned_sequence(void) {
    task_t* owner;
    task_t* first;
    task_t* second;
    uint32_t saved_seq;
    uint32_t saved_gen;
    uint32_t saved_key;
    int pid;
    tasking_init();
    service_registry_init();
    owner = create_task((void*)0);
    add_task_to_queue(owner);
    first = create_task((void*)0);
    add_task_to_queue(first);
    saved_seq = first->sequence;
    saved_gen = first->generation;
    saved_key = first->identity_key;
    TEST_ASSERT_TRUE(saved_key != 0U);
    TEST_ASSERT_EQUAL(0, service_registry_register("vfs", owner->id));
    TEST_ASSERT_EQUAL(0, service_registry_backend_grant("vfs", owner->id, first->id));
    TEST_ASSERT_TRUE(service_registry_backend_allowed_for("vfs", first->id, SERVICE_BACKEND_RIGHT_READ));
    TEST_ASSERT_EQUAL(0, service_registry_register("ata-driver", first->id));
    TEST_ASSERT_TRUE(service_registry_ata_ports_granted(first->id));
    TEST_ASSERT_EQUAL(0, service_registry_register("net-driver", first->id));
    TEST_ASSERT_TRUE(service_registry_net_io_via_worker(first->id));
    pid = first->id;
    remove_task(first);
    next_task_id = pid;
    second = create_task((void*)0);
    add_task_to_queue(second);
    second->sequence = saved_seq;
    second->generation = saved_gen;
    TEST_ASSERT_TRUE(second->identity_key != saved_key);
    TEST_ASSERT_FALSE(service_registry_backend_allowed_for("vfs", pid, SERVICE_BACKEND_RIGHT_READ));
    TEST_ASSERT_FALSE(service_registry_ata_ports_granted(pid));
    TEST_ASSERT_FALSE(service_registry_net_io_via_worker(pid));
}

static void test_ipc_spill_keeps_event_when_mailbox_is_full(void) {
    os_ipc_payload_t in;
    os_ipc_payload_t out;
    uint32_t i;
    service_registry_init();
    in.type = OS_IPC_SERVICE_EVENT;
    in.size = 4U;
    in.request_id = 0U;
    for (i = 0U; i < OS_IPC_MAX_DATA; i++) in.data[i] = 0U;
    for (i = 0U; i < 7U; i++) {
        in.data[0] = (uint8_t)i;
        TEST_ASSERT_EQUAL(0, service_registry_ipc_spill_push(4, &in));
    }
    in.data[0] = 9U;
    TEST_ASSERT_EQUAL(0, service_registry_ipc_spill_push(5, &in));
    in.data[0] = 7U;
    TEST_ASSERT_EQUAL(OS_IPC_FULL, service_registry_ipc_spill_push(4, &in));
    for (i = 0U; i < 7U; i++) {
        TEST_ASSERT_EQUAL(0, service_registry_ipc_spill_pop(4, &out));
        TEST_ASSERT_EQUAL(i, out.data[0]);
        TEST_ASSERT_EQUAL(OS_IPC_SERVICE_EVENT, out.type);
    }
    TEST_ASSERT_EQUAL(OS_IPC_EMPTY, service_registry_ipc_spill_pop(4, &out));
    TEST_ASSERT_EQUAL(0, service_registry_ipc_spill_pop(5, &out));
    TEST_ASSERT_EQUAL(9U, out.data[0]);
}

static void test_event_journal_survives_without_ata_and_rebinds(void) {
    uint8_t image[512];
    service_registry_notify_event_t event;
    uint32_t seq = 0U;
    service_registry_init();
    TEST_ASSERT_EQUAL(0, service_registry_notify_record("keep", 4, 0, 9, OS_SERVICE_EVENT_PUBLISHED, &seq));
    TEST_ASSERT_EQUAL(0, service_registry_notify_record("keep", 4, 9, 0, OS_SERVICE_EVENT_UNREGISTERED, &seq));
    TEST_ASSERT_EQUAL(0, service_registry_notify_pull(4, &event));
    TEST_ASSERT_EQUAL(0, service_registry_event_journal_export(image, sizeof(image)));
    service_registry_init();
    TEST_ASSERT_EQUAL(OS_SERVICE_NOT_FOUND, service_registry_notify_pull(8, &event));
    TEST_ASSERT_EQUAL(0, service_registry_event_journal_import(image, sizeof(image)));
    TEST_ASSERT_EQUAL(OS_SERVICE_NOT_FOUND, service_registry_notify_pull(8, &event));
    TEST_ASSERT_EQUAL(0, service_registry_subscribe("keep", 8));
    TEST_ASSERT_EQUAL(0, service_registry_notify_pull(8, &event));
    TEST_ASSERT_EQUAL('k', event.name[0]);
    TEST_ASSERT_EQUAL('e', event.name[1]);
    TEST_ASSERT_EQUAL('e', event.name[2]);
    TEST_ASSERT_EQUAL('p', event.name[3]);
    TEST_ASSERT_EQUAL(0, event.name[4]);
    TEST_ASSERT_EQUAL(9, event.old_pid);
    TEST_ASSERT_EQUAL(0, event.new_pid);
    TEST_ASSERT_EQUAL(OS_SERVICE_EVENT_UNREGISTERED, event.reason);
    TEST_ASSERT_EQUAL(OS_SERVICE_NOT_FOUND, service_registry_notify_pull(8, &event));
    image[0] ^= 0xFFU;
    TEST_ASSERT_TRUE(service_registry_event_journal_import(image, sizeof(image)) != 0);
}

static void test_mount_journal_roundtrip_without_ata(void) {
    uint8_t image[1024];
    service_registry_persistent_mount_t mounts[SERVICE_REGISTRY_PERSISTENT_MOUNT_CAPACITY];
    int count;
    int i;
    int found = 0;
    service_registry_init();
    TEST_ASSERT_EQUAL(0, service_registry_persistent_mount_add("vfs", "alias/", OS_SERVICE_BACKEND_SOURCE_OVERLAY));
    TEST_ASSERT_EQUAL(0, service_registry_mount_journal_export(image, sizeof(image)));
    service_registry_init();
    count = service_registry_persistent_mount_get("vfs", mounts, SERVICE_REGISTRY_PERSISTENT_MOUNT_CAPACITY);
    TEST_ASSERT_EQUAL(4, count);
    TEST_ASSERT_EQUAL(0, service_registry_mount_journal_import(image, sizeof(image)));
    count = service_registry_persistent_mount_get("vfs", mounts, SERVICE_REGISTRY_PERSISTENT_MOUNT_CAPACITY);
    TEST_ASSERT_EQUAL(5, count);
    for (i = 0; i < count; i++) {
        if (mounts[i].prefix[0] == 'a' && mounts[i].prefix[1] == 'l') found = 1;
    }
    TEST_ASSERT_TRUE(found);
    image[0] ^= 0xFFU;
    TEST_ASSERT_TRUE(service_registry_mount_journal_import(image, sizeof(image)) != 0);
}

static void test_persistent_mounts_auto_restore_on_service_reregistration(void) {
    service_registry_persistent_mount_t mounts[SERVICE_REGISTRY_PERSISTENT_MOUNT_CAPACITY];
    int count;

    service_registry_init();
    TEST_ASSERT_EQUAL(0, service_registry_register("vfs", 3));

    TEST_ASSERT_EQUAL(0, service_registry_persistent_mount_add("vfs", "custom/", OS_SERVICE_BACKEND_SOURCE_OVERLAY));

    count = service_registry_persistent_mount_get("vfs", mounts, SERVICE_REGISTRY_PERSISTENT_MOUNT_CAPACITY);
    TEST_ASSERT_EQUAL(5, count); /* 4 default boot mounts + 1 custom */

    /* Simulate service crash/purge */
    TEST_ASSERT_EQUAL(0, service_registry_remove_pid(3));
    TEST_ASSERT_EQUAL(OS_SERVICE_NOT_FOUND, service_registry_lookup("vfs"));

    /* Re-register service after crash */
    TEST_ASSERT_EQUAL(0, service_registry_register("vfs", 10));
    TEST_ASSERT_EQUAL(10, service_registry_lookup("vfs"));

    /* Mounts restored automatically */
    count = service_registry_persistent_mount_get("vfs", mounts, SERVICE_REGISTRY_PERSISTENT_MOUNT_CAPACITY);
    TEST_ASSERT_EQUAL(5, count);
}

static void test_net_syscalls_only_via_net_driver_when_live(void) {
    /* Tranche 5: with net-driver registered, NIC/socket/LLM-network/peer
     * syscalls are reserved to that PID; status stays open; degraded mode
     * without the worker keeps the historical local path. */
    service_registry_init();
    TEST_ASSERT_TRUE(service_registry_net_syscall_gated(SYS_SOCKET_OPEN));
    TEST_ASSERT_TRUE(service_registry_net_syscall_gated(SYS_SOCKET_ACCEPT_ACK));
    TEST_ASSERT_TRUE(service_registry_net_syscall_gated(SYS_LLM_ACQUIRE_START));
    TEST_ASSERT_TRUE(service_registry_net_syscall_gated(SYS_LLM_OPENAI_CREDENTIAL));
    TEST_ASSERT_TRUE(service_registry_net_syscall_gated(SYS_PEER_LISTEN));
    TEST_ASSERT_TRUE(service_registry_net_syscall_gated(SYS_PEER_TLS_POLL));
    TEST_ASSERT_FALSE(service_registry_net_syscall_gated(SYS_NET_STATUS));
    TEST_ASSERT_FALSE(service_registry_net_syscall_gated(SYS_LLM_SESSION_STATUS));
    TEST_ASSERT_FALSE(service_registry_net_syscall_gated(SYS_PUTC));
    TEST_ASSERT_FALSE(service_registry_net_syscall_gated(SYS_SERVICE_REGISTER));
    TEST_ASSERT_FALSE(service_registry_net_syscall_gated(SYS_VGA_BLIT));
    /* Degraded: no worker. */
    TEST_ASSERT_TRUE(service_registry_net_syscall_allowed(4, SYS_SOCKET_LISTEN));
    TEST_ASSERT_TRUE(service_registry_net_syscall_allowed(4, SYS_PEER_LISTEN));
    TEST_ASSERT_EQUAL(0, service_registry_register("net-driver", 9));
    TEST_ASSERT_TRUE(service_registry_net_syscall_allowed(9, SYS_SOCKET_LISTEN));
    TEST_ASSERT_TRUE(service_registry_net_syscall_allowed(9, SYS_LLM_POLL_TLS));
    TEST_ASSERT_FALSE(service_registry_net_syscall_allowed(4, SYS_SOCKET_LISTEN));
    TEST_ASSERT_FALSE(service_registry_net_syscall_allowed(4, SYS_LLM_POLL_TLS));
    TEST_ASSERT_FALSE(service_registry_net_syscall_allowed(4, SYS_PEER_ACCEPT));
    TEST_ASSERT_TRUE(service_registry_net_syscall_allowed(4, SYS_NET_STATUS));
    TEST_ASSERT_TRUE(service_registry_net_syscall_allowed(4, SYS_PUTC));
    TEST_ASSERT_FALSE(service_registry_net_syscall_allowed(0, SYS_SOCKET_LISTEN));
    /* A vfs-virtual worker gets no network privilege. */
    TEST_ASSERT_EQUAL(0, service_registry_register("vfs-virtual", 11));
    TEST_ASSERT_FALSE(service_registry_net_syscall_allowed(11, SYS_SOCKET_OPEN));
    /* Worker gone: degraded path reopens. */
    service_registry_remove_pid(9);
    TEST_ASSERT_TRUE(service_registry_net_syscall_allowed(4, SYS_SOCKET_LISTEN));
    /* Error code does not collide with task/VFS codes. */
    TEST_ASSERT_EQUAL(-59, OS_NET_WORKER_REQUIRED);
    TEST_ASSERT_TRUE(OS_NET_WORKER_REQUIRED != OS_VFS_BACKEND_DENIED);
    TEST_ASSERT_TRUE(OS_NET_WORKER_REQUIRED != OS_TASK_NOT_CHILD);
}

int main(void) {
    unity_init();
    RUN_TEST(test_registry_rejects_invalid_names);
    RUN_TEST(test_registry_binds_name_to_owner_and_is_idempotent);
    RUN_TEST(test_registry_handles_capacity);
    RUN_TEST(test_registry_removal_allows_reuse);
    RUN_TEST(test_registry_refuses_removal_by_other_owner);
    RUN_TEST(test_owner_can_grant_name_to_another_pid);
    RUN_TEST(test_registry_refuses_grant_by_non_owner);
    RUN_TEST(test_transferred_name_is_removed_with_grantee);
    RUN_TEST(test_watchers_are_collected_and_idempotent);
    RUN_TEST(test_watcher_capacity_and_cleanup_are_bounded);
    RUN_TEST(test_owned_snapshot_survives_removal_for_notification);
    RUN_TEST(test_owner_predicate_tracks_register_grant_and_remove);
    RUN_TEST(test_service_event_is_bounded_and_parsed);
    RUN_TEST(test_remove_pid_clears_all_services_owned_by_task);
    RUN_TEST(test_backend_capability_is_revoked_on_transfer_and_pid_cleanup);
    RUN_TEST(test_backend_capability_can_be_explicitly_revoked_without_name_transfer);
    RUN_TEST(test_backend_capability_scoped_read_only_enforces_least_privilege);
    RUN_TEST(test_backend_capability_source_scope_blocks_other_sources_and_generic_calls);
    RUN_TEST(test_backend_capability_prefix_scope_blocks_siblings_and_pathless_calls);
    RUN_TEST(test_backend_capability_cumulative_grants);
    RUN_TEST(test_backend_capability_rights_are_owner_scoped_and_revocable);
    RUN_TEST(test_backend_capability_list_is_owner_scoped_and_tracks_revocation);
    RUN_TEST(test_backend_capability_can_be_released_by_its_grantee);
    RUN_TEST(test_backend_observe_detects_stale_generations_without_disclosure);
    RUN_TEST(test_owner_fat_bypass_closes_when_storage_worker_live);
    RUN_TEST(test_owner_initrd_overlay_bypass_closes_when_storage_worker_live);
    RUN_TEST(test_owner_ata_generic_bypass_closes_when_storage_worker_live);
    RUN_TEST(test_ata_overlay_io_only_via_worker_when_live);
    RUN_TEST(test_historical_readfile_writefile_gate_when_worker_live);
    RUN_TEST(test_historical_overlay_entry_points_gate_when_worker_live);
    RUN_TEST(test_notify_ack_and_history_persistence);
    /* Tranche 4 Ring 3 ATA driver. */
    RUN_TEST(test_ata_driver_name_and_port_grant);
    /* Tranche 5 net-driver gate. */
    RUN_TEST(test_net_syscalls_only_via_net_driver_when_live);
    /* Task 2 Increments */
    RUN_TEST(test_stable_identity_blocks_stale_reused_pid_acl);
    RUN_TEST(test_acknowledged_event_replay_and_bounded_retry);
    RUN_TEST(test_persistent_mounts_auto_restore_on_service_reregistration);
    RUN_TEST(test_backend_token_is_nonzero_unique_and_required);
    RUN_TEST(test_notify_pull_returns_oldest_without_ipc);
    RUN_TEST(test_identity_key_blocks_cloned_sequence);
    RUN_TEST(test_ipc_spill_keeps_event_when_mailbox_is_full);
    RUN_TEST(test_event_journal_survives_without_ata_and_rebinds);
    RUN_TEST(test_mount_journal_roundtrip_without_ata);
    unity_print_results();
    unity_cleanup();
    return unity_stats.tests_failed == 0 ? 0 : 1;
}
