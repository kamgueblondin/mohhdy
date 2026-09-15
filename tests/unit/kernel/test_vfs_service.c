#include "../../framework/unity.h"
#include "../../../include/os_vfs_service.h"

static void test_read_request_is_bounded_and_zero_padded(void) {
    os_ipc_payload_t payload;
    TEST_ASSERT_EQUAL(0, os_vfs_make_read_request(&payload, "hello.txt", 17U));
    TEST_ASSERT_EQUAL(OS_IPC_VFS_READ, payload.type);
    TEST_ASSERT_EQUAL(17, payload.request_id);
    TEST_ASSERT_EQUAL(OS_VFS_PATH_MAX, payload.size);
    TEST_ASSERT_EQUAL('h', payload.data[0]);
    TEST_ASSERT_EQUAL('t', payload.data[8]);
    TEST_ASSERT_EQUAL(0, payload.data[9]);
    TEST_ASSERT_EQUAL(0, payload.data[OS_IPC_MAX_DATA - 1U]);
}

static void test_unsafe_or_unterminated_path_is_rejected(void) {
    os_ipc_payload_t payload;
    char unterminated[OS_VFS_PATH_MAX];
    uint32_t i;
    for (i = 0U; i < OS_VFS_PATH_MAX; i++) unterminated[i] = 'a';
    TEST_ASSERT_EQUAL(OS_VFS_STATUS_INVALID, os_vfs_make_read_request(&payload, "../secret", 1U));
    TEST_ASSERT_EQUAL(OS_VFS_STATUS_INVALID, os_vfs_make_read_request(&payload, "", 1U));
    TEST_ASSERT_EQUAL(OS_VFS_STATUS_INVALID, os_vfs_make_read_request(&payload, unterminated, 1U));
}

static void test_mount_match_requires_a_declared_directory_prefix(void) {
    const char* relative = 0;
    TEST_ASSERT_TRUE(os_vfs_match_mount("initrd/hello.txt", "initrd/", &relative));
    TEST_ASSERT_EQUAL_STRING("hello.txt", relative);
    TEST_ASSERT_FALSE(os_vfs_match_mount("initrd", "initrd/", &relative));
    TEST_ASSERT_FALSE(os_vfs_match_mount("initrdx/hello.txt", "initrd/", &relative));
    TEST_ASSERT_FALSE(os_vfs_match_mount("initrd/../secret", "initrd/", &relative));
    TEST_ASSERT_FALSE(os_vfs_match_mount("hello.txt", "initrd/", &relative));
    TEST_ASSERT_FALSE(os_vfs_match_mount("initrd/hello.txt", "initrd", &relative));
}

static void test_mount_match_canonical_relative_prefix_boundaries(void) {
    const char* relative = 0;
    TEST_ASSERT_TRUE(os_vfs_match_mount("fat16/dir1/dir2/file.txt", "fat16/", &relative));
    TEST_ASSERT_EQUAL_STRING("dir1/dir2/file.txt", relative);
    TEST_ASSERT_TRUE(os_vfs_match_mount("fat32/sub/child.txt", "fat32/", &relative));
    TEST_ASSERT_EQUAL_STRING("sub/child.txt", relative);
    TEST_ASSERT_FALSE(os_vfs_match_mount("fat16/../escaped.txt", "fat16/", &relative));
    TEST_ASSERT_FALSE(os_vfs_match_mount("fat32/", "fat32/", &relative));
}

static void test_server_can_parse_valid_request(void) {
    os_ipc_payload_t payload;
    os_ipc_message_t message;
    char path[OS_VFS_PATH_MAX];
    uint32_t i;
    TEST_ASSERT_EQUAL(0, os_vfs_make_read_request(&payload, "config.cfg", 23U));
    message.sender_pid = 7;
    message.type = payload.type;
    message.size = payload.size;
    message.request_id = payload.request_id;
    for (i = 0U; i < OS_IPC_MAX_DATA; i++) message.data[i] = payload.data[i];
    TEST_ASSERT_EQUAL(0, os_vfs_parse_read_request(&message, path));
    TEST_ASSERT_EQUAL('c', path[0]);
    TEST_ASSERT_EQUAL('g', path[9]);
    TEST_ASSERT_EQUAL(0, path[10]);
}

static void test_read_reply_preserves_status_and_data(void) {
    os_ipc_payload_t payload;
    os_ipc_message_t message;
    os_vfs_read_reply_t reply;
    uint8_t data[3] = {'o', 'k', '\n'};
    uint32_t i;
    TEST_ASSERT_EQUAL(0, os_vfs_make_read_reply(&payload, OS_VFS_STATUS_OK, data, 3U, 29U));
    message.sender_pid = 2;
    message.type = payload.type;
    message.size = payload.size;
    message.request_id = payload.request_id;
    for (i = 0U; i < OS_IPC_MAX_DATA; i++) message.data[i] = payload.data[i];
    TEST_ASSERT_EQUAL(0, os_vfs_parse_read_reply(&message, &reply, 29U));
    TEST_ASSERT_EQUAL(OS_VFS_STATUS_OK, reply.status);
    TEST_ASSERT_EQUAL(3, reply.size);
    TEST_ASSERT_EQUAL('o', reply.data[0]);
    TEST_ASSERT_EQUAL('\n', reply.data[2]);
}

static void test_malformed_reply_is_rejected(void) {
    os_ipc_message_t message;
    os_vfs_read_reply_t reply;
    uint32_t i;
    message.type = OS_IPC_VFS_READ_REPLY;
    message.size = 8U + OS_VFS_READ_MAX - 1U;
    for (i = 0U; i < OS_IPC_MAX_DATA; i++) message.data[i] = 0U;
    TEST_ASSERT_EQUAL(OS_VFS_STATUS_INVALID, os_vfs_parse_read_reply(&message, &reply, 1U));
}

static void test_grant_request_is_bounded_and_validated(void) {
    os_ipc_payload_t payload;
    os_ipc_message_t message;
    int32_t target_pid;
    uint32_t i;
    TEST_ASSERT_EQUAL(0, os_vfs_make_grant_request(&payload, 7));
    TEST_ASSERT_EQUAL(OS_IPC_VFS_GRANT, payload.type);
    TEST_ASSERT_EQUAL(OS_VFS_GRANT_REQUEST_SIZE, payload.size);
    TEST_ASSERT_EQUAL(0, payload.request_id);
    TEST_ASSERT_EQUAL(7, os_vfs_decode_i32(&payload.data[0]));
    TEST_ASSERT_EQUAL(0, payload.data[OS_IPC_MAX_DATA - 1U]);
    message.sender_pid = 1;
    message.type = payload.type;
    message.size = payload.size;
    message.request_id = payload.request_id;
    for (i = 0U; i < OS_IPC_MAX_DATA; i++) message.data[i] = payload.data[i];
    TEST_ASSERT_EQUAL(0, os_vfs_parse_grant_request(&message, &target_pid));
    TEST_ASSERT_EQUAL(7, target_pid);
    TEST_ASSERT_EQUAL(OS_VFS_STATUS_INVALID, os_vfs_make_grant_request(&payload, 0));
    message.size = OS_VFS_GRANT_REQUEST_SIZE - 1U;
    TEST_ASSERT_EQUAL(OS_VFS_STATUS_INVALID, os_vfs_parse_grant_request(&message, &target_pid));
}

static void test_write_request_and_reply_are_bounded_and_correlated(void) {
    os_ipc_payload_t payload;
    os_ipc_message_t message;
    os_vfs_write_reply_t reply;
    char path[OS_VFS_PATH_MAX];
    uint8_t data_out[OS_VFS_WRITE_MAX];
    uint8_t data[3] = {'o', 'k', '!'};
    uint32_t size;
    uint32_t i;
    TEST_ASSERT_EQUAL(0, os_vfs_make_write_request(&payload, "overlay/note.txt", data, 3U, 37U));
    TEST_ASSERT_EQUAL(OS_IPC_VFS_WRITE, payload.type);
    TEST_ASSERT_EQUAL(OS_VFS_WRITE_REQUEST_SIZE, payload.size);
    TEST_ASSERT_EQUAL(37, payload.request_id);
    message.sender_pid = 1;
    message.type = payload.type;
    message.size = payload.size;
    message.request_id = payload.request_id;
    for (i = 0U; i < OS_IPC_MAX_DATA; i++) message.data[i] = payload.data[i];
    TEST_ASSERT_EQUAL(0, os_vfs_parse_write_request(&message, path, data_out, &size));
    TEST_ASSERT_EQUAL_STRING("overlay/note.txt", path);
    TEST_ASSERT_EQUAL(3, size);
    TEST_ASSERT_EQUAL('o', data_out[0]);
    TEST_ASSERT_EQUAL('!', data_out[2]);
    TEST_ASSERT_EQUAL(0, os_vfs_make_write_reply(&payload, OS_VFS_STATUS_OK, 37U));
    message.type = payload.type;
    message.size = payload.size;
    message.request_id = payload.request_id;
    for (i = 0U; i < OS_IPC_MAX_DATA; i++) message.data[i] = payload.data[i];
    TEST_ASSERT_EQUAL(0, os_vfs_parse_write_reply(&message, &reply, 37U));
    TEST_ASSERT_EQUAL(OS_VFS_STATUS_OK, reply.status);
}

static void test_write_rejects_oversize_and_unexpected_reply(void) {
    os_ipc_payload_t payload;
    os_ipc_message_t message;
    os_vfs_write_reply_t reply;
    uint8_t data[OS_VFS_WRITE_MAX + 1U];
    uint32_t i;
    for (i = 0U; i < OS_VFS_WRITE_MAX + 1U; i++) data[i] = 'x';
    TEST_ASSERT_EQUAL(OS_VFS_STATUS_INVALID,
                      os_vfs_make_write_request(&payload, "overlay/a", data,
                                                OS_VFS_WRITE_MAX + 1U, 1U));
    TEST_ASSERT_EQUAL(0, os_vfs_make_write_reply(&payload, OS_VFS_STATUS_NOT_MOUNTED, 4U));
    message.sender_pid = 2;
    message.type = payload.type;
    message.size = payload.size;
    message.request_id = payload.request_id;
    for (i = 0U; i < OS_IPC_MAX_DATA; i++) message.data[i] = payload.data[i];
    TEST_ASSERT_EQUAL(OS_VFS_STATUS_INVALID, os_vfs_parse_write_reply(&message, &reply, 5U));
    message.size = OS_VFS_WRITE_REPLY_SIZE - 1U;
    TEST_ASSERT_EQUAL(OS_VFS_STATUS_INVALID, os_vfs_parse_write_reply(&message, &reply, 4U));
}

static void test_remove_request_and_reply_are_bounded_and_correlated(void) {
    os_ipc_payload_t payload;
    os_ipc_message_t message;
    os_vfs_remove_reply_t reply;
    char path[OS_VFS_PATH_MAX];
    uint32_t i;
    TEST_ASSERT_EQUAL(0, os_vfs_make_remove_request(&payload, "overlay/note.txt", 51U));
    TEST_ASSERT_EQUAL(OS_IPC_VFS_REMOVE, payload.type);
    TEST_ASSERT_EQUAL(OS_VFS_PATH_MAX, payload.size);
    TEST_ASSERT_EQUAL(51, payload.request_id);
    message.sender_pid = 2;
    message.type = payload.type;
    message.size = payload.size;
    message.request_id = payload.request_id;
    for (i = 0U; i < OS_IPC_MAX_DATA; i++) message.data[i] = payload.data[i];
    TEST_ASSERT_EQUAL(0, os_vfs_parse_remove_request(&message, path));
    TEST_ASSERT_EQUAL_STRING("overlay/note.txt", path);
    TEST_ASSERT_EQUAL(0, os_vfs_make_remove_reply(&payload, OS_VFS_STATUS_OK, 51U));
    message.type = payload.type;
    message.size = payload.size;
    message.request_id = payload.request_id;
    for (i = 0U; i < OS_IPC_MAX_DATA; i++) message.data[i] = payload.data[i];
    TEST_ASSERT_EQUAL(0, os_vfs_parse_remove_reply(&message, &reply, 51U));
    TEST_ASSERT_EQUAL(OS_VFS_STATUS_OK, reply.status);
}

static void test_remove_rejects_unsafe_path_and_unexpected_reply(void) {
    os_ipc_payload_t payload;
    os_ipc_message_t message;
    os_vfs_remove_reply_t reply;
    uint32_t i;
    TEST_ASSERT_EQUAL(OS_VFS_STATUS_INVALID,
                      os_vfs_make_remove_request(&payload, "overlay/../secret", 1U));
    TEST_ASSERT_EQUAL(0, os_vfs_make_remove_reply(&payload, OS_VFS_STATUS_NOT_MOUNTED, 9U));
    message.sender_pid = 2;
    message.type = payload.type;
    message.size = payload.size;
    message.request_id = payload.request_id;
    for (i = 0U; i < OS_IPC_MAX_DATA; i++) message.data[i] = payload.data[i];
    TEST_ASSERT_EQUAL(OS_VFS_STATUS_INVALID, os_vfs_parse_remove_reply(&message, &reply, 10U));
    message.size = OS_VFS_WRITE_REPLY_SIZE - 1U;
    TEST_ASSERT_EQUAL(OS_VFS_STATUS_INVALID, os_vfs_parse_remove_reply(&message, &reply, 9U));
}

static void test_rename_request_and_reply_are_bounded_and_correlated(void) {
    os_ipc_payload_t payload;
    os_ipc_message_t message;
    os_vfs_rename_reply_t reply;
    char old_path[OS_VFS_PATH_MAX];
    char new_path[OS_VFS_PATH_MAX];
    uint32_t i;
    TEST_ASSERT_EQUAL(0, os_vfs_make_rename_request(&payload, "overlay/note.txt",
                                                     "overlay/moved.txt", 73U));
    TEST_ASSERT_EQUAL(OS_IPC_VFS_RENAME, payload.type);
    TEST_ASSERT_EQUAL(OS_VFS_RENAME_REQUEST_SIZE, payload.size);
    TEST_ASSERT_EQUAL(73, payload.request_id);
    message.sender_pid = 2;
    message.type = payload.type;
    message.size = payload.size;
    message.request_id = payload.request_id;
    for (i = 0U; i < OS_IPC_MAX_DATA; i++) message.data[i] = payload.data[i];
    TEST_ASSERT_EQUAL(0, os_vfs_parse_rename_request(&message, old_path, new_path));
    TEST_ASSERT_EQUAL_STRING("overlay/note.txt", old_path);
    TEST_ASSERT_EQUAL_STRING("overlay/moved.txt", new_path);
    TEST_ASSERT_EQUAL(0, os_vfs_make_rename_reply(&payload, OS_VFS_STATUS_OK, 73U));
    message.type = payload.type;
    message.size = payload.size;
    message.request_id = payload.request_id;
    for (i = 0U; i < OS_IPC_MAX_DATA; i++) message.data[i] = payload.data[i];
    TEST_ASSERT_EQUAL(0, os_vfs_parse_rename_reply(&message, &reply, 73U));
    TEST_ASSERT_EQUAL(OS_VFS_STATUS_OK, reply.status);
}

static void test_rename_rejects_invalid_path_and_reply(void) {
    os_ipc_payload_t payload;
    os_ipc_message_t message;
    os_vfs_rename_reply_t reply;
    uint32_t i;
    TEST_ASSERT_EQUAL(OS_VFS_STATUS_INVALID,
                      os_vfs_make_rename_request(&payload, "overlay/note.txt",
                                                 "overlay/../moved.txt", 1U));
    TEST_ASSERT_EQUAL(0, os_vfs_make_rename_reply(&payload, OS_VFS_STATUS_NOT_MOUNTED, 12U));
    message.sender_pid = 2;
    message.type = payload.type;
    message.size = payload.size;
    message.request_id = payload.request_id;
    for (i = 0U; i < OS_IPC_MAX_DATA; i++) message.data[i] = payload.data[i];
    TEST_ASSERT_EQUAL(OS_VFS_STATUS_INVALID, os_vfs_parse_rename_reply(&message, &reply, 13U));
    message.size = OS_VFS_WRITE_REPLY_SIZE - 1U;
    TEST_ASSERT_EQUAL(OS_VFS_STATUS_INVALID, os_vfs_parse_rename_reply(&message, &reply, 12U));
}

static void test_mount_add_request_and_reply_are_bounded_and_correlated(void) {
    os_ipc_payload_t payload;
    os_ipc_message_t message;
    os_vfs_mount_reply_t reply;
    char mount[OS_VFS_PATH_MAX];
    uint32_t source;
    uint32_t i;
    TEST_ASSERT_EQUAL(0, os_vfs_make_mount_add_request(&payload, "assets/",
                      OS_VFS_MOUNT_SOURCE_INITRD, 83U));
    TEST_ASSERT_EQUAL(OS_IPC_VFS_MOUNT_ADD, payload.type);
    TEST_ASSERT_EQUAL(OS_VFS_MOUNT_ADD_REQUEST_SIZE, payload.size);
    TEST_ASSERT_EQUAL(83, payload.request_id);
    TEST_ASSERT_EQUAL(OS_VFS_MOUNT_SOURCE_INITRD,
                      os_vfs_decode_u32(&payload.data[OS_VFS_PATH_MAX]));
    message.sender_pid = 2;
    message.type = payload.type;
    message.size = payload.size;
    message.request_id = payload.request_id;
    for (i = 0U; i < OS_IPC_MAX_DATA; i++) message.data[i] = payload.data[i];
    TEST_ASSERT_EQUAL(0, os_vfs_parse_mount_add_request(&message, mount, &source));
    TEST_ASSERT_EQUAL_STRING("assets/", mount);
    TEST_ASSERT_EQUAL(OS_VFS_MOUNT_SOURCE_INITRD, source);
    TEST_ASSERT_EQUAL(0, os_vfs_make_mount_reply(&payload, OS_IPC_VFS_MOUNT_ADD_REPLY,
                                                  OS_VFS_STATUS_OK, 83U));
    message.type = payload.type;
    message.size = payload.size;
    message.request_id = payload.request_id;
    for (i = 0U; i < OS_IPC_MAX_DATA; i++) message.data[i] = payload.data[i];
    TEST_ASSERT_EQUAL(0, os_vfs_parse_mount_reply(&message, OS_IPC_VFS_MOUNT_ADD_REPLY,
                                                   &reply, 83U));
    TEST_ASSERT_EQUAL(OS_VFS_STATUS_OK, reply.status);
}

static void test_mount_requests_reject_invalid_prefix_source_and_reply(void) {
    os_ipc_payload_t payload;
    os_ipc_message_t message;
    os_vfs_mount_reply_t reply;
    char mount[OS_VFS_PATH_MAX];
    uint32_t i;
    TEST_ASSERT_EQUAL(OS_VFS_STATUS_INVALID,
                      os_vfs_make_mount_add_request(&payload, "assets",
                      OS_VFS_MOUNT_SOURCE_INITRD, 1U));
    TEST_ASSERT_EQUAL(OS_VFS_STATUS_INVALID,
                      os_vfs_make_mount_add_request(&payload, "assets/", 99U, 1U));
    TEST_ASSERT_EQUAL(0, os_vfs_make_mount_remove_request(&payload, "cache/", 89U));
    TEST_ASSERT_EQUAL(OS_IPC_VFS_MOUNT_REMOVE, payload.type);
    TEST_ASSERT_EQUAL(OS_VFS_MOUNT_REMOVE_REQUEST_SIZE, payload.size);
    message.sender_pid = 2;
    message.type = payload.type;
    message.size = payload.size;
    message.request_id = payload.request_id;
    for (i = 0U; i < OS_IPC_MAX_DATA; i++) message.data[i] = payload.data[i];
    TEST_ASSERT_EQUAL(0, os_vfs_parse_mount_remove_request(&message, mount));
    TEST_ASSERT_EQUAL_STRING("cache/", mount);
    message.size = OS_VFS_MOUNT_REMOVE_REQUEST_SIZE - 1U;
    TEST_ASSERT_EQUAL(OS_VFS_STATUS_INVALID, os_vfs_parse_mount_remove_request(&message, mount));
    TEST_ASSERT_EQUAL(0, os_vfs_make_mount_reply(&payload, OS_IPC_VFS_MOUNT_REMOVE_REPLY,
                                                  OS_VFS_STATUS_NOT_MOUNTED, 89U));
    message.type = payload.type;
    message.size = payload.size;
    message.request_id = payload.request_id;
    for (i = 0U; i < OS_IPC_MAX_DATA; i++) message.data[i] = payload.data[i];
    TEST_ASSERT_EQUAL(OS_VFS_STATUS_INVALID, os_vfs_parse_mount_reply(&message,
                      OS_IPC_VFS_MOUNT_REMOVE_REPLY, &reply, 90U));
}

static void test_reply_with_unexpected_request_id_is_rejected(void) {
    os_ipc_payload_t payload;
    os_ipc_message_t message;
    os_vfs_read_reply_t reply;
    uint8_t data[1] = {'x'};
    uint32_t i;
    TEST_ASSERT_EQUAL(0, os_vfs_make_read_reply(&payload, OS_VFS_STATUS_OK, data, 1U, 41U));
    message.sender_pid = 3;
    message.type = payload.type;
    message.size = payload.size;
    message.request_id = payload.request_id;
    for (i = 0U; i < OS_IPC_MAX_DATA; i++) message.data[i] = payload.data[i];
    TEST_ASSERT_EQUAL(OS_VFS_STATUS_INVALID, os_vfs_parse_read_reply(&message, &reply, 42U));
}

static void test_stat_request_and_reply_are_bounded_and_correlated(void) {
    os_ipc_payload_t payload;
    os_ipc_message_t message;
    os_vfs_stat_reply_t reply;
    char path[OS_VFS_PATH_MAX];
    uint32_t i;
    TEST_ASSERT_EQUAL(0, os_vfs_make_stat_request(&payload, "overlay/note.txt", 97U));
    TEST_ASSERT_EQUAL(OS_IPC_VFS_STAT, payload.type);
    TEST_ASSERT_EQUAL(OS_VFS_PATH_MAX, payload.size);
    TEST_ASSERT_EQUAL(97, payload.request_id);
    message.sender_pid = 2;
    message.type = payload.type;
    message.size = payload.size;
    message.request_id = payload.request_id;
    for (i = 0U; i < OS_IPC_MAX_DATA; i++) message.data[i] = payload.data[i];
    TEST_ASSERT_EQUAL(0, os_vfs_parse_stat_request(&message, path));
    TEST_ASSERT_EQUAL_STRING("overlay/note.txt", path);
    TEST_ASSERT_EQUAL(0, os_vfs_make_stat_reply(&payload, OS_VFS_STATUS_OK,
                                                 5U, OS_DIRENT_FILE, 97U));
    message.type = payload.type;
    message.size = payload.size;
    message.request_id = payload.request_id;
    for (i = 0U; i < OS_IPC_MAX_DATA; i++) message.data[i] = payload.data[i];
    TEST_ASSERT_EQUAL(0, os_vfs_parse_stat_reply(&message, &reply, 97U));
    TEST_ASSERT_EQUAL(OS_VFS_STATUS_OK, reply.status);
    TEST_ASSERT_EQUAL(5, reply.size);
    TEST_ASSERT_EQUAL(OS_DIRENT_FILE, reply.flags);
}

static void test_list_request_and_reply_are_bounded_and_correlated(void) {
    os_ipc_payload_t payload;
    os_ipc_message_t message;
    os_vfs_list_reply_t reply;
    char mount[OS_VFS_PATH_MAX];
    uint8_t data[11] = {'s', 'h', 'e', 'l', 'l', '\n', 'i', 'd', 'l', 'e', '\n'};
    uint32_t i;
    TEST_ASSERT_EQUAL(0, os_vfs_make_list_request(&payload, "initrd/", 101U));
    TEST_ASSERT_EQUAL(OS_IPC_VFS_LIST, payload.type);
    TEST_ASSERT_EQUAL(OS_VFS_PATH_MAX, payload.size);
    TEST_ASSERT_EQUAL(101, payload.request_id);
    message.sender_pid = 2;
    message.type = payload.type;
    message.size = payload.size;
    message.request_id = payload.request_id;
    for (i = 0U; i < OS_IPC_MAX_DATA; i++) message.data[i] = payload.data[i];
    TEST_ASSERT_EQUAL(0, os_vfs_parse_list_request(&message, mount));
    TEST_ASSERT_EQUAL_STRING("initrd/", mount);
    TEST_ASSERT_EQUAL(0, os_vfs_make_list_reply(&payload, OS_VFS_STATUS_OK, 2U,
                                                 data, 11U, 101U));
    TEST_ASSERT_EQUAL(OS_IPC_VFS_LIST_REPLY, payload.type);
    TEST_ASSERT_EQUAL(OS_VFS_LIST_REPLY_SIZE, payload.size);
    TEST_ASSERT_EQUAL(0, payload.data[8U + 11U]);
    message.type = payload.type;
    message.size = payload.size;
    message.request_id = payload.request_id;
    for (i = 0U; i < OS_IPC_MAX_DATA; i++) message.data[i] = payload.data[i];
    TEST_ASSERT_EQUAL(0, os_vfs_parse_list_reply(&message, &reply, 101U));
    TEST_ASSERT_EQUAL(OS_VFS_STATUS_OK, reply.status);
    TEST_ASSERT_EQUAL(2, reply.count);
    TEST_ASSERT_EQUAL('s', reply.data[0]);
    TEST_ASSERT_EQUAL('\n', reply.data[10]);
}

static void test_list_accepts_subdirectory_and_rejects_non_directory_path(void) {
    os_ipc_payload_t payload;
    os_ipc_message_t message;
    char path[OS_VFS_PATH_MAX];
    uint32_t i;
    TEST_ASSERT_EQUAL(0, os_vfs_make_list_request(&payload, "initrd/bin/", 104U));
    message.sender_pid = 2;
    message.type = payload.type;
    message.size = payload.size;
    message.request_id = payload.request_id;
    for (i = 0U; i < OS_IPC_MAX_DATA; i++) message.data[i] = payload.data[i];
    TEST_ASSERT_EQUAL(0, os_vfs_parse_list_request(&message, path));
    TEST_ASSERT_EQUAL_STRING("initrd/bin/", path);
    TEST_ASSERT_EQUAL(OS_VFS_STATUS_INVALID,
                      os_vfs_make_list_request(&payload, "initrd/bin", 105U));
    TEST_ASSERT_EQUAL(OS_VFS_STATUS_INVALID,
                      os_vfs_make_list_request(&payload, "initrd/bin/shell", 105U));
}

static void test_list_rejects_invalid_mount_and_reply(void) {
    os_ipc_payload_t payload;
    os_ipc_message_t message;
    os_vfs_list_reply_t reply;
    char mount[OS_VFS_PATH_MAX];
    uint8_t data[1] = {'x'};
    uint32_t i;
    TEST_ASSERT_EQUAL(OS_VFS_STATUS_INVALID,
                      os_vfs_make_list_request(&payload, "initrd", 1U));
    TEST_ASSERT_EQUAL(OS_VFS_STATUS_INVALID,
                      os_vfs_make_list_request(&payload, "initrd/../", 1U));
    TEST_ASSERT_EQUAL(OS_VFS_STATUS_INVALID,
                      os_vfs_make_list_reply(&payload, OS_VFS_STATUS_OK,
                                             OS_VFS_LIST_ENTRY_MAX + 1U, data, 1U, 1U));
    TEST_ASSERT_EQUAL(0, os_vfs_make_list_reply(&payload, OS_VFS_STATUS_TRUNCATED,
                                                 1U, data, 1U, 102U));
    message.sender_pid = 2;
    message.type = payload.type;
    message.size = payload.size;
    message.request_id = payload.request_id;
    for (i = 0U; i < OS_IPC_MAX_DATA; i++) message.data[i] = payload.data[i];
    TEST_ASSERT_EQUAL(OS_VFS_STATUS_INVALID, os_vfs_parse_list_reply(&message, &reply, 103U));
    message.size = OS_VFS_LIST_REPLY_SIZE - 1U;
    TEST_ASSERT_EQUAL(OS_VFS_STATUS_INVALID, os_vfs_parse_list_reply(&message, &reply, 102U));
    message.type = OS_IPC_VFS_LIST;
    message.size = OS_VFS_PATH_MAX - 1U;
    TEST_ASSERT_EQUAL(OS_VFS_STATUS_INVALID, os_vfs_parse_list_request(&message, mount));
}

static void test_stat_rejects_invalid_request_and_reply(void) {
    os_ipc_payload_t payload;
    os_ipc_message_t message;
    os_vfs_stat_reply_t reply;
    char path[OS_VFS_PATH_MAX];
    uint32_t i;
    TEST_ASSERT_EQUAL(OS_VFS_STATUS_INVALID,
                      os_vfs_make_stat_request(&payload, "overlay/../secret", 1U));
    TEST_ASSERT_EQUAL(0, os_vfs_make_stat_reply(&payload, OS_VFS_STATUS_NOT_MOUNTED,
                                                 0U, 0U, 99U));
    message.sender_pid = 2;
    message.type = payload.type;
    message.size = payload.size;
    message.request_id = payload.request_id;
    for (i = 0U; i < OS_IPC_MAX_DATA; i++) message.data[i] = payload.data[i];
    TEST_ASSERT_EQUAL(OS_VFS_STATUS_INVALID, os_vfs_parse_stat_reply(&message, &reply, 98U));
    message.size = OS_VFS_STAT_REPLY_SIZE - 1U;
    TEST_ASSERT_EQUAL(OS_VFS_STATUS_INVALID, os_vfs_parse_stat_reply(&message, &reply, 99U));
    message.type = OS_IPC_VFS_STAT;
    message.size = OS_VFS_PATH_MAX - 1U;
    TEST_ASSERT_EQUAL(OS_VFS_STATUS_INVALID, os_vfs_parse_stat_request(&message, path));
}

static void test_list_page_request_and_reply_are_bounded_and_correlated(void) {
    os_ipc_payload_t payload;
    os_ipc_message_t message;
    os_vfs_list_page_reply_t reply;
    char path[OS_VFS_PATH_MAX];
    uint32_t start;
    uint8_t data[8] = { 's', 'h', 'e', 'l', 'l', '\n', 0U, 0U };
    uint32_t i;
    TEST_ASSERT_EQUAL(0, os_vfs_make_list_page_request(&payload, "initrd/", 4U, 106U));
    TEST_ASSERT_EQUAL(OS_VFS_LIST_PAGE_REQUEST_SIZE, payload.size);
    message.sender_pid = 2;
    message.type = payload.type;
    message.size = payload.size;
    message.request_id = payload.request_id;
    for (i = 0U; i < OS_IPC_MAX_DATA; i++) message.data[i] = payload.data[i];
    TEST_ASSERT_EQUAL(0, os_vfs_parse_list_page_request(&message, path, &start));
    TEST_ASSERT_EQUAL_STRING("initrd/", path);
    TEST_ASSERT_EQUAL(4U, start);
    TEST_ASSERT_EQUAL(0, os_vfs_make_list_page_reply(&payload, OS_VFS_STATUS_TRUNCATED,
                                                      1U, 8U, data, 6U, 106U));
    message.type = payload.type;
    message.size = payload.size;
    message.request_id = payload.request_id;
    for (i = 0U; i < OS_IPC_MAX_DATA; i++) message.data[i] = payload.data[i];
    TEST_ASSERT_EQUAL(0, os_vfs_parse_list_page_reply(&message, &reply, 106U));
    TEST_ASSERT_EQUAL(1U, reply.count);
    TEST_ASSERT_EQUAL(8U, reply.next_start);
}

static void test_list_observe_request_and_reply_are_bounded_and_correlated(void) {
    os_ipc_payload_t payload;
    os_ipc_message_t message;
    os_vfs_list_observe_reply_t reply;
    char path[OS_VFS_PATH_MAX];
    uint32_t start, generation, i;
    uint8_t data[4] = { 'o', 'k', '\n', 0U };
    TEST_ASSERT_EQUAL(0, os_vfs_make_list_observe_request(&payload, "initrd/", 4U, 7U, 107U));
    message.sender_pid = 2; message.type = payload.type; message.size = payload.size; message.request_id = payload.request_id;
    for (i = 0U; i < OS_IPC_MAX_DATA; i++) message.data[i] = payload.data[i];
    TEST_ASSERT_EQUAL(0, os_vfs_parse_list_observe_request(&message, path, &start, &generation));
    TEST_ASSERT_EQUAL_STRING("initrd/", path); TEST_ASSERT_EQUAL(4U, start); TEST_ASSERT_EQUAL(7U, generation);
    TEST_ASSERT_EQUAL(0, os_vfs_make_list_observe_reply(&payload, OS_VFS_STATUS_STALE, 0U,
                                                         OS_VFS_LIST_PAGE_END, 8U, data, 3U, 107U));
    message.type = payload.type; message.size = payload.size; message.request_id = payload.request_id;
    for (i = 0U; i < OS_IPC_MAX_DATA; i++) message.data[i] = payload.data[i];
    TEST_ASSERT_EQUAL(0, os_vfs_parse_list_observe_reply(&message, &reply, 107U));
    TEST_ASSERT_EQUAL(OS_VFS_STATUS_STALE, reply.status); TEST_ASSERT_EQUAL(8U, reply.generation);
}

static void test_mkdir_request_and_reply_are_bounded_and_correlated(void) {
    os_ipc_payload_t payload;
    os_ipc_message_t message;
    char path[OS_VFS_PATH_MAX];
    int32_t status;
    uint32_t i;
    TEST_ASSERT_EQUAL(0, os_vfs_make_mkdir_request(&payload, "overlay/newdir", 108U));
    message.sender_pid = 2; message.type = payload.type; message.size = payload.size; message.request_id = payload.request_id;
    for (i = 0U; i < OS_IPC_MAX_DATA; i++) message.data[i] = payload.data[i];
    TEST_ASSERT_EQUAL(0, os_vfs_parse_mkdir_request(&message, path));
    TEST_ASSERT_EQUAL_STRING("overlay/newdir", path);
    TEST_ASSERT_EQUAL(0, os_vfs_make_mkdir_reply(&payload, OS_VFS_STATUS_OK, 108U));
    message.type = payload.type; message.size = payload.size; message.request_id = payload.request_id;
    for (i = 0U; i < OS_IPC_MAX_DATA; i++) message.data[i] = payload.data[i];
    TEST_ASSERT_EQUAL(0, os_vfs_parse_mkdir_reply(&message, &status, 108U));
    TEST_ASSERT_EQUAL(OS_VFS_STATUS_OK, status);
}

static void test_rmdir_request_and_reply_are_bounded_and_correlated(void) {
    os_ipc_payload_t payload;
    os_ipc_message_t message;
    char path[OS_VFS_PATH_MAX];
    int32_t status;
    uint32_t i;
    TEST_ASSERT_EQUAL(0, os_vfs_make_rmdir_request(&payload, "overlay/newdir", 109U));
    TEST_ASSERT_EQUAL(OS_IPC_VFS_RMDIR, payload.type);
    TEST_ASSERT_EQUAL(OS_VFS_PATH_MAX, payload.size);
    message.sender_pid = 2; message.type = payload.type; message.size = payload.size; message.request_id = payload.request_id;
    for (i = 0U; i < OS_IPC_MAX_DATA; i++) message.data[i] = payload.data[i];
    TEST_ASSERT_EQUAL(0, os_vfs_parse_rmdir_request(&message, path));
    TEST_ASSERT_EQUAL_STRING("overlay/newdir", path);
    TEST_ASSERT_EQUAL(0, os_vfs_make_rmdir_reply(&payload, OS_VFS_STATUS_NOT_EMPTY, 109U));
    message.type = payload.type; message.size = payload.size; message.request_id = payload.request_id;
    for (i = 0U; i < OS_IPC_MAX_DATA; i++) message.data[i] = payload.data[i];
    TEST_ASSERT_EQUAL(0, os_vfs_parse_rmdir_reply(&message, &status, 109U));
    TEST_ASSERT_EQUAL(OS_VFS_STATUS_NOT_EMPTY, status);
    TEST_ASSERT_EQUAL(OS_VFS_STATUS_INVALID, os_vfs_parse_rmdir_reply(&message, &status, 110U));
}

static void test_backend_list_request_and_reply_are_bounded_and_correlated(void) {
    os_ipc_payload_t payload;
    os_ipc_message_t message;
    os_service_backend_list_t list;
    os_vfs_backend_list_reply_t reply;
    uint32_t i;
    list.count = 2U;
    list.entries[0].pid = 7; list.entries[0].rights = OS_VFS_BACKEND_RIGHT_READ;
    list.entries[1].pid = 9; list.entries[1].rights = OS_VFS_BACKEND_RIGHT_ALL;
    for (i = 2U; i < OS_SERVICE_BACKEND_CAPACITY; i++) { list.entries[i].pid = 0; list.entries[i].rights = 0U; }
    TEST_ASSERT_EQUAL(0, os_vfs_make_backend_list_request(&payload, 113U));
    TEST_ASSERT_EQUAL(OS_IPC_VFS_BACKEND_LIST, payload.type);
    TEST_ASSERT_EQUAL(OS_VFS_BACKEND_LIST_REQUEST_SIZE, payload.size);
    message.sender_pid = 2; message.type = payload.type; message.size = payload.size; message.request_id = payload.request_id;
    for (i = 0U; i < OS_IPC_MAX_DATA; i++) message.data[i] = payload.data[i];
    TEST_ASSERT_EQUAL(0, os_vfs_parse_backend_list_request(&message));
    TEST_ASSERT_EQUAL(0, os_vfs_make_backend_list_reply(&payload, OS_VFS_STATUS_OK, &list, 113U));
    TEST_ASSERT_EQUAL(OS_VFS_BACKEND_LIST_REPLY_SIZE, payload.size);
    message.type = payload.type; message.size = payload.size; message.request_id = payload.request_id;
    for (i = 0U; i < OS_IPC_MAX_DATA; i++) message.data[i] = payload.data[i];
    TEST_ASSERT_EQUAL(0, os_vfs_parse_backend_list_reply(&message, &reply, 113U));
    TEST_ASSERT_EQUAL(2U, reply.count);
    TEST_ASSERT_EQUAL(7, reply.entries[0].pid); TEST_ASSERT_EQUAL(OS_VFS_BACKEND_RIGHT_READ, reply.entries[0].rights);
    TEST_ASSERT_EQUAL(9, reply.entries[1].pid); TEST_ASSERT_EQUAL(OS_VFS_BACKEND_RIGHT_ALL, reply.entries[1].rights);
    TEST_ASSERT_EQUAL(0, os_vfs_make_backend_list_reply(&payload, OS_SERVICE_NOT_FOUND, 0, 114U));
    message.type = payload.type; message.size = payload.size; message.request_id = payload.request_id;
    for (i = 0U; i < OS_IPC_MAX_DATA; i++) message.data[i] = payload.data[i];
    TEST_ASSERT_EQUAL(0, os_vfs_parse_backend_list_reply(&message, &reply, 114U));
    TEST_ASSERT_EQUAL(OS_SERVICE_NOT_FOUND, reply.status); TEST_ASSERT_EQUAL(0U, reply.count);
    message.data[8U] = 1U;
    TEST_ASSERT_EQUAL(OS_VFS_STATUS_INVALID, os_vfs_parse_backend_list_reply(&message, &reply, 114U));
}

static void test_backend_scope_request_and_reply_are_bounded_and_correlated(void) {
    os_ipc_payload_t payload;
    os_ipc_message_t message;
    os_vfs_backend_scope_reply_t reply;
    int32_t target_pid;
    uint32_t i;
    TEST_ASSERT_EQUAL(OS_VFS_STATUS_INVALID, os_vfs_make_backend_scope_request(&payload, 0, 114U));
    TEST_ASSERT_EQUAL(0, os_vfs_make_backend_scope_request(&payload, 7, 114U));
    TEST_ASSERT_EQUAL(OS_IPC_VFS_BACKEND_SCOPE, payload.type);
    TEST_ASSERT_EQUAL(OS_VFS_BACKEND_SCOPE_REQUEST_SIZE, payload.size);
    message.sender_pid = 2; message.type = payload.type; message.size = payload.size;
    message.request_id = payload.request_id;
    for (i = 0U; i < OS_IPC_MAX_DATA; i++) message.data[i] = payload.data[i];
    TEST_ASSERT_EQUAL(0, os_vfs_parse_backend_scope_request(&message, &target_pid));
    TEST_ASSERT_EQUAL(7, target_pid);
    TEST_ASSERT_EQUAL(0, os_vfs_make_backend_scope_reply(
        &payload, OS_VFS_STATUS_OK, OS_VFS_BACKEND_RIGHT_READ,
        OS_SERVICE_BACKEND_SOURCE_INITRD, 114U));
    TEST_ASSERT_EQUAL(OS_IPC_VFS_BACKEND_SCOPE_REPLY, payload.type);
    TEST_ASSERT_EQUAL(OS_VFS_BACKEND_SCOPE_REPLY_SIZE, payload.size);
    message.type = payload.type; message.size = payload.size; message.request_id = payload.request_id;
    for (i = 0U; i < OS_IPC_MAX_DATA; i++) message.data[i] = payload.data[i];
    TEST_ASSERT_EQUAL(0, os_vfs_parse_backend_scope_reply(&message, &reply, 114U));
    TEST_ASSERT_EQUAL(OS_VFS_STATUS_OK, reply.status);
    TEST_ASSERT_EQUAL(OS_VFS_BACKEND_RIGHT_READ, reply.rights);
    TEST_ASSERT_EQUAL(OS_SERVICE_BACKEND_SOURCE_INITRD, reply.sources);
    message.data[8U] = 0U;
    TEST_ASSERT_EQUAL(OS_VFS_STATUS_INVALID, os_vfs_parse_backend_scope_reply(&message, &reply, 114U));
    TEST_ASSERT_EQUAL(0, os_vfs_make_backend_scope_reply(&payload, OS_SERVICE_NOT_FOUND,
                                                          1U, 1U, 115U));
    message.type = payload.type; message.size = payload.size; message.request_id = payload.request_id;
    for (i = 0U; i < OS_IPC_MAX_DATA; i++) message.data[i] = payload.data[i];
    TEST_ASSERT_EQUAL(0, os_vfs_parse_backend_scope_reply(&message, &reply, 115U));
    TEST_ASSERT_EQUAL(OS_SERVICE_NOT_FOUND, reply.status);
    TEST_ASSERT_EQUAL(0U, reply.rights); TEST_ASSERT_EQUAL(0U, reply.sources);
    TEST_ASSERT_EQUAL(OS_VFS_STATUS_INVALID, os_vfs_parse_backend_scope_reply(&message, &reply, 116U));
}

static void test_worker_read_messages_are_bounded_and_correlated(void) {
    os_ipc_payload_t payload;
    os_ipc_message_t message;
    char path[OS_VFS_PATH_MAX];
    uint8_t data[3] = {'o', 'k', '\n'};
    uint8_t copied[OS_VFS_READ_MAX];
    int32_t status;
    uint32_t size;
    uint32_t i;
    TEST_ASSERT_EQUAL(OS_VFS_STATUS_OK,
                      os_vfs_make_worker_read_request(&payload, "vfs-info", 91U));
    TEST_ASSERT_EQUAL(OS_IPC_VFS_WORKER_READ, payload.type);
    TEST_ASSERT_EQUAL(OS_VFS_WORKER_READ_REQUEST_SIZE, payload.size);
    message.sender_pid = 7;
    message.type = payload.type;
    message.size = payload.size;
    message.request_id = payload.request_id;
    for (i = 0U; i < OS_IPC_MAX_DATA; i++) message.data[i] = payload.data[i];
    TEST_ASSERT_EQUAL(OS_VFS_STATUS_OK, os_vfs_parse_worker_read_request(&message, path));
    TEST_ASSERT_EQUAL_STRING("vfs-info", path);
    TEST_ASSERT_EQUAL(OS_VFS_STATUS_OK,
                      os_vfs_make_worker_read_reply(&payload, OS_VFS_STATUS_OK, data, 3U, 91U));
    message.type = payload.type;
    message.size = payload.size;
    message.request_id = payload.request_id;
    for (i = 0U; i < OS_IPC_MAX_DATA; i++) message.data[i] = payload.data[i];
    TEST_ASSERT_EQUAL(OS_VFS_STATUS_OK,
                      os_vfs_parse_worker_read_reply(&message, &status, copied, &size, 91U));
    TEST_ASSERT_EQUAL(OS_VFS_STATUS_OK, status);
    TEST_ASSERT_EQUAL(3U, size);
    TEST_ASSERT_EQUAL('o', copied[0]);
    TEST_ASSERT_EQUAL('\n', copied[2]);
    TEST_ASSERT_EQUAL(OS_VFS_STATUS_INVALID,
                      os_vfs_parse_worker_read_reply(&message, &status, copied, &size, 92U));
    message.request_id = 91U;
    message.data[4] = OS_VFS_READ_MAX + 1U;
    TEST_ASSERT_EQUAL(OS_VFS_STATUS_INVALID,
                      os_vfs_parse_worker_read_reply(&message, &status, copied, &size, 91U));
}

static void test_worker_stats_snapshot_is_bounded_and_decoded(void) {
    os_ipc_payload_t payload;
    os_ipc_message_t message;
    uint32_t reads, writes, removes, renames;
    uint32_t i;
    TEST_ASSERT_EQUAL(OS_VFS_STATUS_OK,
                      os_vfs_make_worker_stats_request(&payload, 3U, 5U, 7U, 11U, 123U));
    TEST_ASSERT_EQUAL(OS_IPC_VFS_WORKER_STATS, payload.type);
    TEST_ASSERT_EQUAL(OS_VFS_WORKER_STATS_REQUEST_SIZE, payload.size);
    TEST_ASSERT_EQUAL(123U, payload.request_id);
    message.sender_pid = 7;
    message.type = payload.type;
    message.size = payload.size;
    message.request_id = payload.request_id;
    for (i = 0U; i < OS_IPC_MAX_DATA; i++) message.data[i] = payload.data[i];
    TEST_ASSERT_EQUAL(OS_VFS_STATUS_OK,
                      os_vfs_parse_worker_stats_request(&message, &reads, &writes, &removes, &renames));
    TEST_ASSERT_EQUAL(3U, reads);
    TEST_ASSERT_EQUAL(5U, writes);
    TEST_ASSERT_EQUAL(7U, removes);
    TEST_ASSERT_EQUAL(11U, renames);
    message.size = OS_VFS_WORKER_STATS_REQUEST_SIZE - 1U;
    TEST_ASSERT_EQUAL(OS_VFS_STATUS_INVALID,
                      os_vfs_parse_worker_stats_request(&message, &reads, &writes, &removes, &renames));
}

static void test_worker_mount_request_is_bounded_and_decoded(void) {
    os_ipc_payload_t payload;
    os_ipc_message_t message;
    char prefix[OS_VFS_PATH_MAX];
    uint32_t writable;
    uint32_t i;
    TEST_ASSERT_EQUAL(OS_VFS_STATUS_OK,
                      os_vfs_make_worker_mount_request(&payload, "overlay/", 1U, 141U));
    TEST_ASSERT_EQUAL(OS_IPC_VFS_WORKER_MOUNT, payload.type);
    TEST_ASSERT_EQUAL(OS_VFS_WORKER_MOUNT_REQUEST_SIZE, payload.size);
    TEST_ASSERT_EQUAL(141U, payload.request_id);
    message.sender_pid = 9;
    message.type = payload.type;
    message.size = payload.size;
    message.request_id = payload.request_id;
    for (i = 0U; i < OS_IPC_MAX_DATA; i++) message.data[i] = payload.data[i];
    TEST_ASSERT_EQUAL(OS_VFS_STATUS_OK,
                      os_vfs_parse_worker_mount_request(&message, prefix, &writable));
    TEST_ASSERT_EQUAL_STRING("overlay/", prefix);
    TEST_ASSERT_EQUAL(1U, writable);
    TEST_ASSERT_EQUAL(OS_VFS_STATUS_INVALID,
                      os_vfs_make_worker_mount_request(&payload, "overlay", 1U, 1U));
    message.data[OS_VFS_PATH_MAX] = 2U;
    TEST_ASSERT_EQUAL(OS_VFS_STATUS_INVALID,
                      os_vfs_parse_worker_mount_request(&message, prefix, &writable));
    message.data[OS_VFS_PATH_MAX] = 1U;
    message.size = OS_VFS_WORKER_MOUNT_REQUEST_SIZE - 1U;
    TEST_ASSERT_EQUAL(OS_VFS_STATUS_INVALID,
                      os_vfs_parse_worker_mount_request(&message, prefix, &writable));
}

static void test_worker_mount_mutations_are_bounded_and_correlated(void) {
    os_ipc_payload_t payload;
    os_ipc_message_t message;
    char prefix[OS_VFS_PATH_MAX];
    uint32_t source;
    int32_t status;
    uint32_t i;
    TEST_ASSERT_EQUAL(OS_VFS_STATUS_OK,
                      os_vfs_make_worker_mount_add_request(&payload, "work/",
                                                           OS_VFS_MOUNT_SOURCE_OVERLAY, 147U));
    TEST_ASSERT_EQUAL(OS_IPC_VFS_WORKER_MOUNT_ADD, payload.type);
    TEST_ASSERT_EQUAL(OS_VFS_WORKER_MOUNT_ADD_REQUEST_SIZE, payload.size);
    message.sender_pid = 9;
    message.type = payload.type;
    message.size = payload.size;
    message.request_id = payload.request_id;
    for (i = 0U; i < OS_IPC_MAX_DATA; i++) message.data[i] = payload.data[i];
    TEST_ASSERT_EQUAL(OS_VFS_STATUS_OK,
                      os_vfs_parse_worker_mount_add_request(&message, prefix, &source));
    TEST_ASSERT_EQUAL_STRING("work/", prefix);
    TEST_ASSERT_EQUAL(OS_VFS_MOUNT_SOURCE_OVERLAY, source);
    TEST_ASSERT_EQUAL(OS_VFS_STATUS_INVALID,
                      os_vfs_make_worker_mount_add_request(&payload, "work/", 99U, 147U));
    message.size = OS_VFS_WORKER_MOUNT_ADD_REQUEST_SIZE - 1U;
    TEST_ASSERT_EQUAL(OS_VFS_STATUS_INVALID,
                      os_vfs_parse_worker_mount_add_request(&message, prefix, &source));
    TEST_ASSERT_EQUAL(OS_VFS_STATUS_OK,
                      os_vfs_make_worker_mount_remove_request(&payload, "work/", 148U));
    TEST_ASSERT_EQUAL(OS_IPC_VFS_WORKER_MOUNT_REMOVE, payload.type);
    TEST_ASSERT_EQUAL(OS_VFS_WORKER_MOUNT_REMOVE_REQUEST_SIZE, payload.size);
    message.type = payload.type;
    message.size = payload.size;
    message.request_id = payload.request_id;
    for (i = 0U; i < OS_IPC_MAX_DATA; i++) message.data[i] = payload.data[i];
    TEST_ASSERT_EQUAL(OS_VFS_STATUS_OK,
                      os_vfs_parse_worker_mount_remove_request(&message, prefix));
    TEST_ASSERT_EQUAL_STRING("work/", prefix);
    TEST_ASSERT_EQUAL(OS_VFS_STATUS_INVALID,
                      os_vfs_make_worker_mount_remove_request(&payload, "work", 148U));
    TEST_ASSERT_EQUAL(OS_VFS_STATUS_OK,
                      os_vfs_make_worker_mount_reply(&payload, OS_IPC_VFS_WORKER_MOUNT_ADD_REPLY,
                                                     OS_VFS_STATUS_MOUNT_FULL, 147U));
    message.type = payload.type;
    message.size = payload.size;
    message.request_id = payload.request_id;
    for (i = 0U; i < OS_IPC_MAX_DATA; i++) message.data[i] = payload.data[i];
    TEST_ASSERT_EQUAL(OS_VFS_STATUS_OK,
                      os_vfs_parse_worker_mount_reply(&message, OS_IPC_VFS_WORKER_MOUNT_ADD_REPLY,
                                                      &status, 147U));
    TEST_ASSERT_EQUAL(OS_VFS_STATUS_MOUNT_FULL, status);
    TEST_ASSERT_EQUAL(OS_VFS_STATUS_INVALID,
                      os_vfs_parse_worker_mount_reply(&message, OS_IPC_VFS_WORKER_MOUNT_ADD_REPLY,
                                                      &status, 148U));
    TEST_ASSERT_EQUAL(OS_VFS_STATUS_OK,
                      os_vfs_make_worker_mount_reply(&payload, OS_IPC_VFS_WORKER_MOUNT_REMOVE_REPLY,
                                                     OS_VFS_STATUS_OK, 148U));
    message.type = payload.type;
    message.size = payload.size;
    message.request_id = payload.request_id;
    for (i = 0U; i < OS_IPC_MAX_DATA; i++) message.data[i] = payload.data[i];
    TEST_ASSERT_EQUAL(OS_VFS_STATUS_OK,
                      os_vfs_parse_worker_mount_reply(&message, OS_IPC_VFS_WORKER_MOUNT_REMOVE_REPLY,
                                                      &status, 148U));
    TEST_ASSERT_EQUAL(OS_VFS_STATUS_OK, status);
}

static void test_worker_write_request_is_bounded_and_decoded(void) {
    os_ipc_payload_t payload;
    os_ipc_message_t message;
    char path[OS_VFS_PATH_MAX];
    uint8_t write_data[OS_VFS_WRITE_MAX];
    uint8_t input_data[4] = {'a', 'b', 'c', '\n'};
    uint32_t write_len = 0U;
    uint32_t i;
    TEST_ASSERT_EQUAL(OS_VFS_STATUS_OK,
                      os_vfs_make_worker_write_request(&payload, "virtual/test.txt", input_data, 4U, 142U));
    TEST_ASSERT_EQUAL(OS_IPC_VFS_WORKER_WRITE, payload.type);
    TEST_ASSERT_EQUAL(OS_VFS_WORKER_WRITE_REQUEST_SIZE, payload.size);
    TEST_ASSERT_EQUAL(142U, payload.request_id);
    message.sender_pid = 9;
    message.type = payload.type;
    message.size = payload.size;
    message.request_id = payload.request_id;
    for (i = 0U; i < OS_IPC_MAX_DATA; i++) message.data[i] = payload.data[i];
    TEST_ASSERT_EQUAL(OS_VFS_STATUS_OK,
                      os_vfs_parse_worker_write_request(&message, path, write_data, &write_len));
    TEST_ASSERT_EQUAL_STRING("virtual/test.txt", path);
    TEST_ASSERT_EQUAL(4U, write_len);
    TEST_ASSERT_EQUAL_MEMORY(input_data, write_data, 4U);
}

static void test_worker_remove_request_is_bounded_and_decoded(void) {
    os_ipc_payload_t payload;
    os_ipc_message_t message;
    char path[OS_VFS_PATH_MAX];
    uint32_t i;
    TEST_ASSERT_EQUAL(OS_VFS_STATUS_OK,
                      os_vfs_make_worker_remove_request(&payload, "virtual/temp.txt", 143U));
    TEST_ASSERT_EQUAL(OS_IPC_VFS_WORKER_REMOVE, payload.type);
    TEST_ASSERT_EQUAL(OS_VFS_WORKER_REMOVE_REQUEST_SIZE, payload.size);
    TEST_ASSERT_EQUAL(143U, payload.request_id);
    message.sender_pid = 9;
    message.type = payload.type;
    message.size = payload.size;
    message.request_id = payload.request_id;
    for (i = 0U; i < OS_IPC_MAX_DATA; i++) message.data[i] = payload.data[i];
    TEST_ASSERT_EQUAL(OS_VFS_STATUS_OK,
                      os_vfs_parse_worker_remove_request(&message, path));
    TEST_ASSERT_EQUAL_STRING("virtual/temp.txt", path);
    TEST_ASSERT_EQUAL(OS_VFS_STATUS_INVALID,
                      os_vfs_make_worker_remove_request(&payload, "../unsafe.txt", 143U));
    message.size = OS_VFS_WORKER_REMOVE_REQUEST_SIZE - 1U;
    TEST_ASSERT_EQUAL(OS_VFS_STATUS_INVALID,
                      os_vfs_parse_worker_remove_request(&message, path));
}

static void test_worker_directory_requests_and_replies_are_bounded_and_correlated(void) {
    os_ipc_payload_t payload;
    os_ipc_message_t message;
    char path[OS_VFS_PATH_MAX];
    int32_t status;
    uint32_t i;
    TEST_ASSERT_EQUAL(OS_VFS_STATUS_OK,
                      os_vfs_make_worker_directory_request(&payload, OS_IPC_VFS_WORKER_MKDIR,
                                                           "fat16/qdir", 145U));
    TEST_ASSERT_EQUAL(OS_IPC_VFS_WORKER_MKDIR, payload.type);
    TEST_ASSERT_EQUAL(OS_VFS_WORKER_MKDIR_REQUEST_SIZE, payload.size);
    TEST_ASSERT_EQUAL(145U, payload.request_id);
    message.sender_pid = 9;
    message.type = payload.type;
    message.size = payload.size;
    message.request_id = payload.request_id;
    for (i = 0U; i < OS_IPC_MAX_DATA; i++) message.data[i] = payload.data[i];
    TEST_ASSERT_EQUAL(OS_VFS_STATUS_OK,
                      os_vfs_parse_worker_directory_request(&message, OS_IPC_VFS_WORKER_MKDIR, path));
    TEST_ASSERT_EQUAL_STRING("fat16/qdir", path);
    TEST_ASSERT_EQUAL(OS_VFS_STATUS_OK,
                      os_vfs_make_worker_directory_request(&payload, OS_IPC_VFS_WORKER_RMDIR,
                                                           "fat32/qdir", 146U));
    message.type = payload.type;
    message.size = payload.size;
    message.request_id = payload.request_id;
    for (i = 0U; i < OS_IPC_MAX_DATA; i++) message.data[i] = payload.data[i];
    TEST_ASSERT_EQUAL(OS_VFS_STATUS_OK,
                      os_vfs_parse_worker_directory_request(&message, OS_IPC_VFS_WORKER_RMDIR, path));
    TEST_ASSERT_EQUAL_STRING("fat32/qdir", path);
    TEST_ASSERT_EQUAL(OS_VFS_STATUS_INVALID,
                      os_vfs_make_worker_directory_request(&payload, OS_IPC_VFS_WORKER_MKDIR,
                                                           "../unsafe", 145U));
    message.size = OS_VFS_WORKER_RMDIR_REQUEST_SIZE - 1U;
    TEST_ASSERT_EQUAL(OS_VFS_STATUS_INVALID,
                      os_vfs_parse_worker_directory_request(&message, OS_IPC_VFS_WORKER_RMDIR, path));
    TEST_ASSERT_EQUAL(OS_VFS_STATUS_OK,
                      os_vfs_make_mkdir_reply(&payload, OS_VFS_STATUS_INVALID, 145U));
    message.type = payload.type;
    message.size = payload.size;
    message.request_id = payload.request_id;
    for (i = 0U; i < OS_IPC_MAX_DATA; i++) message.data[i] = payload.data[i];
    TEST_ASSERT_EQUAL(OS_VFS_STATUS_OK, os_vfs_parse_mkdir_reply(&message, &status, 145U));
    TEST_ASSERT_EQUAL(OS_VFS_STATUS_INVALID, status);
    TEST_ASSERT_EQUAL(OS_VFS_STATUS_INVALID, os_vfs_parse_mkdir_reply(&message, &status, 146U));
    TEST_ASSERT_EQUAL(OS_VFS_STATUS_OK,
                      os_vfs_make_rmdir_reply(&payload, OS_VFS_STATUS_OK, 146U));
    message.type = payload.type;
    message.size = payload.size;
    message.request_id = payload.request_id;
    for (i = 0U; i < OS_IPC_MAX_DATA; i++) message.data[i] = payload.data[i];
    TEST_ASSERT_EQUAL(OS_VFS_STATUS_OK, os_vfs_parse_rmdir_reply(&message, &status, 146U));
    TEST_ASSERT_EQUAL(OS_VFS_STATUS_OK, status);
    TEST_ASSERT_EQUAL(OS_VFS_STATUS_INVALID, os_vfs_parse_rmdir_reply(&message, &status, 145U));
}

static void test_worker_rename_request_is_bounded_and_decoded(void) {
    os_ipc_payload_t payload;
    os_ipc_message_t message;
    char old_path[OS_VFS_PATH_MAX];
    char new_path[OS_VFS_PATH_MAX];
    uint32_t i;
    TEST_ASSERT_EQUAL(OS_VFS_STATUS_OK,
                      os_vfs_make_worker_rename_request(&payload, "virtual/old.txt", "virtual/new.txt", 144U));
    TEST_ASSERT_EQUAL(OS_IPC_VFS_WORKER_RENAME, payload.type);
    TEST_ASSERT_EQUAL(OS_VFS_WORKER_RENAME_REQUEST_SIZE, payload.size);
    TEST_ASSERT_EQUAL(144U, payload.request_id);
    message.sender_pid = 9;
    message.type = payload.type;
    message.size = payload.size;
    message.request_id = payload.request_id;
    for (i = 0U; i < OS_IPC_MAX_DATA; i++) message.data[i] = payload.data[i];
    TEST_ASSERT_EQUAL(OS_VFS_STATUS_OK,
                      os_vfs_parse_worker_rename_request(&message, old_path, new_path));
    TEST_ASSERT_EQUAL_STRING("virtual/old.txt", old_path);
    TEST_ASSERT_EQUAL_STRING("virtual/new.txt", new_path);
    TEST_ASSERT_EQUAL(OS_VFS_STATUS_INVALID,
                      os_vfs_make_worker_rename_request(&payload, "../old.txt", "virtual/new.txt", 144U));
    TEST_ASSERT_EQUAL(OS_VFS_STATUS_INVALID,
                      os_vfs_make_worker_rename_request(&payload, "virtual/old.txt", "../new.txt", 144U));
    message.size = OS_VFS_WORKER_RENAME_REQUEST_SIZE - 1U;
    TEST_ASSERT_EQUAL(OS_VFS_STATUS_INVALID,
                      os_vfs_parse_worker_rename_request(&message, old_path, new_path));
}

static void test_worker_alias_stat_messages_are_bounded_and_correlated(void) {
    os_ipc_payload_t payload;
    os_ipc_message_t message;
    os_vfs_stat_reply_t reply;
    char path[OS_VFS_PATH_MAX];
    uint32_t i;
    TEST_ASSERT_EQUAL(OS_VFS_STATUS_OK,
                      os_vfs_make_worker_stat_request(&payload, "assets/shell", 151U));
    TEST_ASSERT_EQUAL(OS_IPC_VFS_WORKER_STAT, payload.type);
    TEST_ASSERT_EQUAL(OS_VFS_WORKER_STAT_REQUEST_SIZE, payload.size);
    message.sender_pid = 9; message.type = payload.type; message.size = payload.size;
    message.request_id = payload.request_id;
    for (i = 0U; i < OS_IPC_MAX_DATA; i++) message.data[i] = payload.data[i];
    TEST_ASSERT_EQUAL(OS_VFS_STATUS_OK, os_vfs_parse_worker_stat_request(&message, path));
    TEST_ASSERT_EQUAL_STRING("assets/shell", path);
    TEST_ASSERT_EQUAL(OS_VFS_STATUS_OK,
                      os_vfs_make_worker_stat_reply(&payload, OS_VFS_STATUS_OK, 42U,
                                                    OS_DIRENT_FILE, 151U));
    TEST_ASSERT_EQUAL(OS_IPC_VFS_WORKER_STAT_REPLY, payload.type);
    TEST_ASSERT_EQUAL(OS_VFS_STAT_REPLY_SIZE, payload.size);
    message.type = payload.type; message.size = payload.size; message.request_id = payload.request_id;
    for (i = 0U; i < OS_IPC_MAX_DATA; i++) message.data[i] = payload.data[i];
    TEST_ASSERT_EQUAL(OS_VFS_STATUS_OK, os_vfs_parse_worker_stat_reply(&message, &reply, 151U));
    TEST_ASSERT_EQUAL(42U, reply.size); TEST_ASSERT_EQUAL(OS_DIRENT_FILE, reply.flags);
    TEST_ASSERT_EQUAL(OS_VFS_STATUS_INVALID, os_vfs_parse_worker_stat_reply(&message, &reply, 152U));
    message.type = OS_IPC_VFS_STAT_REPLY;
    TEST_ASSERT_EQUAL(OS_VFS_STATUS_INVALID, os_vfs_parse_worker_stat_reply(&message, &reply, 151U));
}

static void test_worker_alias_list_messages_are_bounded_and_correlated(void) {
    os_ipc_payload_t payload;
    os_ipc_message_t message;
    os_vfs_list_reply_t reply;
    char path[OS_VFS_PATH_MAX];
    uint8_t data[7] = {'s', 'h', 'e', 'l', 'l', '\n', 0U};
    uint32_t i;
    TEST_ASSERT_EQUAL(OS_VFS_STATUS_OK,
                      os_vfs_make_worker_list_request(&payload, "assets/", 153U));
    TEST_ASSERT_EQUAL(OS_IPC_VFS_WORKER_LIST, payload.type);
    TEST_ASSERT_EQUAL(OS_VFS_WORKER_LIST_REQUEST_SIZE, payload.size);
    message.sender_pid = 9; message.type = payload.type; message.size = payload.size;
    message.request_id = payload.request_id;
    for (i = 0U; i < OS_IPC_MAX_DATA; i++) message.data[i] = payload.data[i];
    TEST_ASSERT_EQUAL(OS_VFS_STATUS_OK, os_vfs_parse_worker_list_request(&message, path));
    TEST_ASSERT_EQUAL_STRING("assets/", path);
    TEST_ASSERT_EQUAL(OS_VFS_STATUS_OK,
                      os_vfs_make_worker_list_reply(&payload, OS_VFS_STATUS_OK, 1U, data, 6U, 153U));
    TEST_ASSERT_EQUAL(OS_IPC_VFS_WORKER_LIST_REPLY, payload.type);
    TEST_ASSERT_EQUAL(OS_VFS_LIST_REPLY_SIZE, payload.size);
    message.type = payload.type; message.size = payload.size; message.request_id = payload.request_id;
    for (i = 0U; i < OS_IPC_MAX_DATA; i++) message.data[i] = payload.data[i];
    TEST_ASSERT_EQUAL(OS_VFS_STATUS_OK, os_vfs_parse_worker_list_reply(&message, &reply, 153U));
    TEST_ASSERT_EQUAL(1U, reply.count); TEST_ASSERT_EQUAL('s', reply.data[0]);
    TEST_ASSERT_EQUAL(OS_VFS_STATUS_INVALID,
                      os_vfs_make_worker_list_request(&payload, "assets/file", 153U));
    message.size = OS_VFS_LIST_REPLY_SIZE - 1U;
    TEST_ASSERT_EQUAL(OS_VFS_STATUS_INVALID, os_vfs_parse_worker_list_reply(&message, &reply, 153U));
}

static void test_worker_alias_list_page_messages_are_bounded_and_correlated(void) {
    os_ipc_payload_t payload;
    os_ipc_message_t message;
    os_vfs_list_page_reply_t reply;
    char path[OS_VFS_PATH_MAX];
    uint32_t start;
    uint8_t data[4] = {'o', 'k', '\n', 0U};
    uint32_t i;
    TEST_ASSERT_EQUAL(OS_VFS_STATUS_OK,
                      os_vfs_make_worker_list_page_request(&payload, "assets/", 4U, 154U));
    TEST_ASSERT_EQUAL(OS_IPC_VFS_WORKER_LIST_PAGE, payload.type);
    TEST_ASSERT_EQUAL(OS_VFS_WORKER_LIST_PAGE_REQUEST_SIZE, payload.size);
    message.sender_pid = 9; message.type = payload.type; message.size = payload.size;
    message.request_id = payload.request_id;
    for (i = 0U; i < OS_IPC_MAX_DATA; i++) message.data[i] = payload.data[i];
    TEST_ASSERT_EQUAL(OS_VFS_STATUS_OK,
                      os_vfs_parse_worker_list_page_request(&message, path, &start));
    TEST_ASSERT_EQUAL_STRING("assets/", path); TEST_ASSERT_EQUAL(4U, start);
    TEST_ASSERT_EQUAL(OS_VFS_STATUS_OK,
                      os_vfs_make_worker_list_page_reply(&payload, OS_VFS_STATUS_TRUNCATED,
                                                         1U, 5U, data, 3U, 154U));
    TEST_ASSERT_EQUAL(OS_IPC_VFS_WORKER_LIST_PAGE_REPLY, payload.type);
    TEST_ASSERT_EQUAL(OS_VFS_LIST_PAGE_REPLY_SIZE, payload.size);
    message.type = payload.type; message.size = payload.size; message.request_id = payload.request_id;
    for (i = 0U; i < OS_IPC_MAX_DATA; i++) message.data[i] = payload.data[i];
    TEST_ASSERT_EQUAL(OS_VFS_STATUS_OK,
                      os_vfs_parse_worker_list_page_reply(&message, &reply, 154U));
    TEST_ASSERT_EQUAL(OS_VFS_STATUS_TRUNCATED, reply.status);
    TEST_ASSERT_EQUAL(5U, reply.next_start);
    TEST_ASSERT_EQUAL(OS_VFS_STATUS_INVALID,
                      os_vfs_parse_worker_list_page_reply(&message, &reply, 155U));
    message.size = OS_VFS_LIST_PAGE_REPLY_SIZE - 1U;
    TEST_ASSERT_EQUAL(OS_VFS_STATUS_INVALID,
                      os_vfs_parse_worker_list_page_reply(&message, &reply, 154U));
}

int main(void) {
    unity_init();
    RUN_TEST(test_read_request_is_bounded_and_zero_padded);
    RUN_TEST(test_unsafe_or_unterminated_path_is_rejected);
    RUN_TEST(test_mount_match_requires_a_declared_directory_prefix);
    RUN_TEST(test_mount_match_canonical_relative_prefix_boundaries);
    RUN_TEST(test_server_can_parse_valid_request);
    RUN_TEST(test_read_reply_preserves_status_and_data);
    RUN_TEST(test_malformed_reply_is_rejected);
    RUN_TEST(test_grant_request_is_bounded_and_validated);
    RUN_TEST(test_write_request_and_reply_are_bounded_and_correlated);
    RUN_TEST(test_write_rejects_oversize_and_unexpected_reply);
    RUN_TEST(test_remove_request_and_reply_are_bounded_and_correlated);
    RUN_TEST(test_remove_rejects_unsafe_path_and_unexpected_reply);
    RUN_TEST(test_rename_request_and_reply_are_bounded_and_correlated);
    RUN_TEST(test_rename_rejects_invalid_path_and_reply);
    RUN_TEST(test_mount_add_request_and_reply_are_bounded_and_correlated);
    RUN_TEST(test_mount_requests_reject_invalid_prefix_source_and_reply);
    RUN_TEST(test_reply_with_unexpected_request_id_is_rejected);
    RUN_TEST(test_stat_request_and_reply_are_bounded_and_correlated);
    RUN_TEST(test_list_request_and_reply_are_bounded_and_correlated);
    RUN_TEST(test_list_accepts_subdirectory_and_rejects_non_directory_path);
    RUN_TEST(test_list_rejects_invalid_mount_and_reply);
    RUN_TEST(test_list_page_request_and_reply_are_bounded_and_correlated);
    RUN_TEST(test_list_observe_request_and_reply_are_bounded_and_correlated);
    RUN_TEST(test_mkdir_request_and_reply_are_bounded_and_correlated);
    RUN_TEST(test_rmdir_request_and_reply_are_bounded_and_correlated);
    RUN_TEST(test_stat_rejects_invalid_request_and_reply);
    RUN_TEST(test_backend_list_request_and_reply_are_bounded_and_correlated);
    RUN_TEST(test_backend_scope_request_and_reply_are_bounded_and_correlated);
    RUN_TEST(test_worker_read_messages_are_bounded_and_correlated);
    RUN_TEST(test_worker_stats_snapshot_is_bounded_and_decoded);
    RUN_TEST(test_worker_mount_request_is_bounded_and_decoded);
    RUN_TEST(test_worker_mount_mutations_are_bounded_and_correlated);
    RUN_TEST(test_worker_write_request_is_bounded_and_decoded);
    RUN_TEST(test_worker_remove_request_is_bounded_and_decoded);
    RUN_TEST(test_worker_directory_requests_and_replies_are_bounded_and_correlated);
    RUN_TEST(test_worker_rename_request_is_bounded_and_decoded);
    RUN_TEST(test_worker_alias_stat_messages_are_bounded_and_correlated);
    RUN_TEST(test_worker_alias_list_messages_are_bounded_and_correlated);
    RUN_TEST(test_worker_alias_list_page_messages_are_bounded_and_correlated);
    unity_print_results();
    unity_cleanup();
    return unity_stats.tests_failed == 0 ? 0 : 1;
}
