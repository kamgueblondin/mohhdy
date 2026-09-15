#include "os_vfs_service.h"

static void putc(char value) {
    asm volatile("int $0x80" : : "a"(SYS_PUTC), "b"(value));
}

static void puts(const char* text) {
    uint32_t index = 0U;
    while (text[index] != '\0') putc(text[index++]);
}

static int ipc_receive(os_ipc_message_t* message) {
    int result;
    asm volatile("int $0x80" : "=a"(result) : "a"(SYS_IPC_RECV), "b"(message));
    return result;
}

static int ipc_send(int target_pid, const os_ipc_payload_t* payload) {
    int result;
    asm volatile("int $0x80" : "=a"(result) : "a"(SYS_IPC_SEND), "b"(target_pid), "c"(payload));
    return result;
}

static int service_register(const char* name) {
    int result;
    asm volatile("int $0x80" : "=a"(result) : "a"(SYS_SERVICE_REGISTER), "b"(name));
    return result;
}

static int service_lookup(const char* name) {
    int result;
    asm volatile("int $0x80" : "=a"(result) : "a"(SYS_SERVICE_LOOKUP), "b"(name));
    return result;
}

/* Le worker ne reçoit que des chemins VFS globaux contrôlés par le médiateur.
 * Il extrait un suffixe pour les trois montages fixes mutables. La politique
 * des alias dynamiques est aussi conservée ici ; elle ne confère aucun droit
 * backend puisque le worker ne fait que valider la table publiée par `vfs`. */
#define VFS_WORKER_MOUNT_MAX 8U
#define VFS_WORKER_BOOT_MOUNT_COUNT 4U
#define VFS_WORKER_DYNAMIC_MOUNT_MAX 13U
typedef struct {
    char prefix[OS_VFS_PATH_MAX];
    uint32_t source;
    uint32_t protected_mount;
} worker_mount_t;
static worker_mount_t worker_mounts[VFS_WORKER_MOUNT_MAX] = {
    { "initrd/", OS_VFS_MOUNT_SOURCE_INITRD, 1U },
    { "overlay/", OS_VFS_MOUNT_SOURCE_OVERLAY, 1U },
    { "fat16/", OS_VFS_MOUNT_SOURCE_FAT16, 1U },
    { "fat32/", OS_VFS_MOUNT_SOURCE_FAT32, 1U },
};
static uint32_t worker_mount_count = VFS_WORKER_BOOT_MOUNT_COUNT;

/* Les requêtes de mutation sont privées : sans ce contrôle, un client pourrait
 * utiliser le worker comme suppléant de ses droits backend ou désynchroniser
 * directement l’autorité des alias et le miroir du médiateur. */
static int worker_sender_is_vfs(const os_ipc_message_t* message) {
    int vfs_pid = service_lookup("vfs");
    return message && vfs_pid > 0 && message->sender_pid == vfs_pid;
}

static inline int worker_storage_source_is_supported(uint32_t source) {
    return source == OS_VFS_MOUNT_SOURCE_INITRD || source == OS_VFS_MOUNT_SOURCE_OVERLAY ||
           source == OS_VFS_MOUNT_SOURCE_FAT16 || source == OS_VFS_MOUNT_SOURCE_FAT32;
}

static int worker_dynamic_path(const char* path, uint32_t* source_out, const char** relative_out);
static int worker_dynamic_mutate_write(const char* path, const uint8_t* data, uint32_t size);
static int worker_dynamic_mutate_remove(const char* path);
static int worker_dynamic_mutate_rename(const char* old_path, const char* new_path);
static int worker_dynamic_mutate_mkdir(const char* path);
static int worker_dynamic_mutate_rmdir(const char* path);

static const char* path_after_prefix(const char* path, const char* prefix) {
    uint32_t index = 0U;
    if (!path || !prefix) return (const char*)0;
    while (prefix[index] != '\0') {
        if (path[index] != prefix[index]) return (const char*)0;
        index++;
    }
    return path[index] != '\0' ? path + index : (const char*)0;
}

static int syscall_write(int number, const char* path, const uint8_t* data, uint32_t size) {
    int result;
    asm volatile("int $0x80" : "=a"(result) : "a"(number), "b"(path), "c"(data), "d"(size));
    return result < 0 ? result : OS_VFS_STATUS_OK;
}

static int syscall_remove(int number, const char* path) {
    int result;
    asm volatile("int $0x80" : "=a"(result) : "a"(number), "b"(path));
    return result;
}

static int syscall_rename(int number, const char* old_path, const char* new_path) {
    int result;
    asm volatile("int $0x80" : "=a"(result) : "a"(number), "b"(old_path), "c"(new_path));
    return result;
}

static int mutate_write(const char* path, const uint8_t* data, uint32_t size) {
    const char* relative;
    if ((relative = path_after_prefix(path, "overlay/")) != (const char*)0)
        return syscall_write(SYS_VFS_BACKEND_WRITE, relative, data, size);
    if ((relative = path_after_prefix(path, "fat16/")) != (const char*)0)
        return syscall_write(SYS_VFS_FAT16_CREATE, relative, data, size);
    if ((relative = path_after_prefix(path, "fat32/")) != (const char*)0)
        return syscall_write(SYS_VFS_FAT32_CREATE, relative, data, size);
    return worker_dynamic_mutate_write(path, data, size);
}

static int mutate_remove(const char* path) {
    const char* relative;
    if ((relative = path_after_prefix(path, "overlay/")) != (const char*)0)
        return syscall_remove(SYS_VFS_OVERLAY_UNLINK, relative);
    if ((relative = path_after_prefix(path, "fat16/")) != (const char*)0)
        return syscall_remove(SYS_VFS_FAT16_UNLINK, relative);
    if ((relative = path_after_prefix(path, "fat32/")) != (const char*)0)
        return syscall_remove(SYS_VFS_FAT32_UNLINK, relative);
    return worker_dynamic_mutate_remove(path);
}

static int mutate_rename(const char* old_path, const char* new_path) {
    const char* old_relative;
    const char* new_relative;
    if ((old_relative = path_after_prefix(old_path, "overlay/")) != (const char*)0 &&
        (new_relative = path_after_prefix(new_path, "overlay/")) != (const char*)0)
        return syscall_rename(SYS_VFS_OVERLAY_RENAME, old_relative, new_relative);
    if ((old_relative = path_after_prefix(old_path, "fat16/")) != (const char*)0 &&
        (new_relative = path_after_prefix(new_path, "fat16/")) != (const char*)0)
        return syscall_rename(SYS_VFS_FAT16_RENAME, old_relative, new_relative);
    if ((old_relative = path_after_prefix(old_path, "fat32/")) != (const char*)0 &&
        (new_relative = path_after_prefix(new_path, "fat32/")) != (const char*)0)
        return syscall_rename(SYS_VFS_FAT32_RENAME, old_relative, new_relative);
    return worker_dynamic_mutate_rename(old_path, new_path);
}

static int mutate_mkdir(const char* path) {
    const char* relative;
    if ((relative = path_after_prefix(path, "overlay/")) != (const char*)0)
        return syscall_remove(SYS_VFS_OVERLAY_MKDIR, relative);
    if ((relative = path_after_prefix(path, "fat16/")) != (const char*)0)
        return syscall_write(SYS_VFS_FAT16_CREATE, relative, (const uint8_t*)0, 0U);
    if ((relative = path_after_prefix(path, "fat32/")) != (const char*)0)
        return syscall_write(SYS_VFS_FAT32_CREATE, relative, (const uint8_t*)0, 0U);
    return worker_dynamic_mutate_mkdir(path);
}

static int mutate_rmdir(const char* path) {
    const char* relative;
    char directory[OS_VFS_PATH_MAX];
    uint32_t i = 0U;
    int number;
    if ((relative = path_after_prefix(path, "overlay/")) != (const char*)0)
        return syscall_remove(SYS_VFS_OVERLAY_RMDIR, relative);
    if ((relative = path_after_prefix(path, "fat16/")) != (const char*)0) number = SYS_VFS_FAT16_UNLINK;
    else if ((relative = path_after_prefix(path, "fat32/")) != (const char*)0) number = SYS_VFS_FAT32_UNLINK;
    else return worker_dynamic_mutate_rmdir(path);
    while (relative[i] != '\0' && i + 2U < OS_VFS_PATH_MAX) { directory[i] = relative[i]; i++; }
    if (relative[i] != '\0' || i == 0U) return OS_VFS_STATUS_INVALID;
    directory[i++] = '/'; directory[i] = '\0';
    return syscall_remove(number, directory);
}

static void yield(void) {
    asm volatile("int $0x80" : : "a"(SYS_YIELD));
}

static int string_equal(const char* left, const char* right) {
    uint32_t index = 0U;
    while (left[index] != '\0' && right[index] != '\0') {
        if (left[index] != right[index]) return 0;
        index++;
    }
    return left[index] == right[index];
}

static uint32_t string_length(const char* text) {
    uint32_t index = 0U;
    while (text[index] != '\0') index++;
    return index;
}

static int mount_prefixes_overlap(const char* left, const char* right) {
    uint32_t index = 0U;
    while (left[index] != '\0' && right[index] != '\0' && left[index] == right[index]) index++;
    return left[index] == '\0' || right[index] == '\0';
}

static int worker_mount_add(const char* prefix, uint32_t source) {
    uint32_t index;
    if (string_length(prefix) > VFS_WORKER_DYNAMIC_MOUNT_MAX) return OS_VFS_STATUS_INVALID;
    for (index = 0U; index < worker_mount_count; index++) {
        if (string_equal(prefix, worker_mounts[index].prefix)) return OS_VFS_STATUS_MOUNT_EXISTS;
        if (mount_prefixes_overlap(prefix, worker_mounts[index].prefix)) return OS_VFS_STATUS_INVALID;
    }
    if (worker_mount_count >= VFS_WORKER_MOUNT_MAX) return OS_VFS_STATUS_MOUNT_FULL;
    for (index = 0U; index < OS_VFS_PATH_MAX; index++) {
        worker_mounts[worker_mount_count].prefix[index] = prefix[index];
    }
    worker_mounts[worker_mount_count].source = source;
    worker_mounts[worker_mount_count].protected_mount = 0U;
    worker_mount_count++;
    return OS_VFS_STATUS_OK;
}

static int worker_mount_remove(const char* prefix) {
    uint32_t index;
    for (index = 0U; index < worker_mount_count; index++) {
        uint32_t next;
        if (!string_equal(prefix, worker_mounts[index].prefix)) continue;
        if (worker_mounts[index].protected_mount) return OS_VFS_STATUS_INVALID;
        for (next = index; next + 1U < worker_mount_count; next++) {
            worker_mounts[next] = worker_mounts[next + 1U];
        }
        worker_mount_count--;
        worker_mounts[worker_mount_count].prefix[0] = '\0';
        worker_mounts[worker_mount_count].source = 0U;
        worker_mounts[worker_mount_count].protected_mount = 0U;
        return OS_VFS_STATUS_OK;
    }
    return OS_VFS_STATUS_NOT_MOUNTED;
}

static uint32_t append_text(uint8_t* data, uint32_t offset, const char* text) {
    uint32_t index = 0U;
    while (text[index] != '\0' && offset < OS_VFS_READ_MAX) data[offset++] = (uint8_t)text[index++];
    return offset;
}

static uint32_t append_uint(uint8_t* data, uint32_t offset, uint32_t value) {
    char digits[10];
    uint32_t count = 0U;
    if (value == 0U) {
        if (offset < OS_VFS_READ_MAX) data[offset++] = (uint8_t)'0';
        return offset;
    }
    while (value > 0U && count < (uint32_t)sizeof(digits)) {
        digits[count++] = (char)('0' + (value % 10U));
        value /= 10U;
    }
    while (count > 0U && offset < OS_VFS_READ_MAX) data[offset++] = (uint8_t)digits[--count];
    return offset;
}

static uint32_t format_stats(uint8_t* data, uint32_t reads, uint32_t writes,
                             uint32_t removes, uint32_t renames) {
    uint32_t size = 0U;
    size = append_text(data, size, "reads=");
    size = append_uint(data, size, reads);
    size = append_text(data, size, "\nwrites=");
    size = append_uint(data, size, writes);
    size = append_text(data, size, "\nremoves=");
    size = append_uint(data, size, removes);
    size = append_text(data, size, "\nrenames=");
    size = append_uint(data, size, renames);
    if (size < OS_VFS_READ_MAX) data[size++] = (uint8_t)'\n';
    return size;
}

/* Les I/O physiques ne sont routées qu’après une correspondance avec une
 * entrée dynamique détenue par ce worker. Les quatre montages fixes ne sont
 * volontairement pas parcourus ici : le grant backend global est compensé par
 * cette politique d’entrée étroite, vérifiée avant chaque syscall. */
static int worker_dynamic_path(const char* path, uint32_t* source_out, const char** relative_out) {
    uint32_t index;
    const char* relative;
    if (!path || !source_out || !relative_out) return 0;
    for (index = VFS_WORKER_BOOT_MOUNT_COUNT; index < worker_mount_count; index++) {
        if (os_vfs_match_mount(path, worker_mounts[index].prefix, &relative)) {
            if (!worker_storage_source_is_supported(worker_mounts[index].source)) return 0;
            *source_out = worker_mounts[index].source;
            *relative_out = relative;
            return 1;
        }
    }
    return 0;
}

static int worker_dynamic_list_path(const char* path, uint32_t* source_out, const char** relative_out) {
    uint32_t index = 0U;
    if (!path || !source_out || !relative_out || !os_vfs_list_path_is_valid(path)) return 0;
    for (index = VFS_WORKER_BOOT_MOUNT_COUNT; index < worker_mount_count; index++) {
        uint32_t at = 0U;
        const char* mount = worker_mounts[index].prefix;
        while (mount[at] != '\0' && path[at] == mount[at]) at++;
        if (mount[at] != '\0') continue;
        *source_out = worker_mounts[index].source;
        *relative_out = path[at] == '\0' ? "/" : path + at;
        return 1;
    }
    return 0;
}

static int worker_syscall_read(int number, const char* path, uint8_t* data, uint32_t max) {
    int result;
    asm volatile("int $0x80" : "=a"(result) : "a"(number), "b"(path), "c"(data), "d"(max));
    return result;
}

static int worker_syscall_stat(int number, const char* path, os_dirent_t* out) {
    int result;
    asm volatile("int $0x80" : "=a"(result) : "a"(number), "b"(path), "c"(out));
    return result;
}

static int worker_list_source(uint32_t source, const char* path, os_dirent_t* out, int max_n) {
    int result;
    if (!path || !out || max_n <= 0) return OS_VFS_STATUS_INVALID;
    if (source == OS_VFS_MOUNT_SOURCE_INITRD) {
        asm volatile("int $0x80" : "=a"(result) : "a"(SYS_VFS_INITRD_LISTDIR), "b"(path), "c"(out), "d"(max_n));
        return result;
    }
    if (source == OS_VFS_MOUNT_SOURCE_OVERLAY) {
        asm volatile("int $0x80" : "=a"(result) : "a"(SYS_VFS_OVERLAY_LISTDIR), "b"(path), "c"(out), "d"(max_n));
        return result;
    }
    if (source == OS_VFS_MOUNT_SOURCE_FAT16) {
        asm volatile("int $0x80" : "=a"(result) : "a"(SYS_FAT16_LIST_PATH), "b"(path), "c"(out), "d"(max_n), "S"(0U));
        return result;
    }
    if (source == OS_VFS_MOUNT_SOURCE_FAT32) {
        asm volatile("int $0x80" : "=a"(result) : "a"(SYS_FAT32_LIST_PATH), "b"(path), "c"(out), "d"(max_n), "S"(0U));
        return result;
    }
    return OS_VFS_STATUS_NOT_MOUNTED;
}

static int worker_list_source_page(uint32_t source, const char* path, os_dirent_t* out,
                                   uint32_t start) {
    int result;
    if (!path || !out) return OS_VFS_STATUS_INVALID;
    if (source == OS_VFS_MOUNT_SOURCE_INITRD) {
        asm volatile("int $0x80" : "=a"(result) : "a"(SYS_VFS_INITRD_LISTDIR_PAGE), "b"(path), "c"(out), "d"(start));
        return result;
    }
    if (source == OS_VFS_MOUNT_SOURCE_OVERLAY) {
        asm volatile("int $0x80" : "=a"(result) : "a"(SYS_VFS_OVERLAY_LISTDIR_PAGE), "b"(path), "c"(out), "d"(start));
        return result;
    }
    if (source == OS_VFS_MOUNT_SOURCE_FAT16) {
        if (path[0] == '/' && path[1] == '\0') {
            asm volatile("int $0x80" : "=a"(result) : "a"(SYS_FAT16_LIST_PAGE), "b"(out),
                         "c"(OS_VFS_LIST_ENTRY_MAX + 1U), "d"(start));
        } else {
            asm volatile("int $0x80" : "=a"(result) : "a"(SYS_FAT16_LIST_PATH), "b"(path),
                         "c"(OS_VFS_LIST_ENTRY_MAX + 1U), "S"(start));
        }
        return result;
    }
    if (source == OS_VFS_MOUNT_SOURCE_FAT32) {
        if (path[0] == '/' && path[1] == '\0') {
            asm volatile("int $0x80" : "=a"(result) : "a"(SYS_FAT32_LIST_PAGE), "b"(out),
                         "c"(OS_VFS_LIST_ENTRY_MAX + 1U), "d"(start));
        } else {
            asm volatile("int $0x80" : "=a"(result) : "a"(SYS_FAT32_LIST_PATH), "b"(path),
                         "c"(OS_VFS_LIST_ENTRY_MAX + 1U), "S"(start));
        }
        return result;
    }
    return OS_VFS_STATUS_NOT_MOUNTED;
}

static int worker_ascii_fold_equal(const char* left, const char* right) {
    uint32_t i = 0U;
    while (left[i] != '\0' && right[i] != '\0') {
        char a = left[i];
        char b = right[i];
        if (a >= 'a' && a <= 'z') a = (char)(a - ('a' - 'A'));
        if (b >= 'a' && b <= 'z') b = (char)(b - ('a' - 'A'));
        if (a != b) return 0;
        i++;
    }
    return left[i] == right[i];
}

static int worker_fat_stat(uint32_t source, const char* path, os_dirent_t* out) {
    os_dirent_t entries[OS_VFS_LIST_ENTRY_MAX + 1U];
    char directory[OS_VFS_PATH_MAX];
    const char* list_path = "/";
    const char* leaf = path;
    uint32_t start = 0U;
    uint32_t i;
    uint32_t slash = OS_VFS_LIST_PAGE_END;
    int count;
    if (!path || !out || path[0] == '\0' || path[0] == '/') return OS_VFS_STATUS_INVALID;
    for (i = 0U; path[i] != '\0'; i++) {
        if (i + 1U >= OS_VFS_PATH_MAX) return OS_VFS_STATUS_INVALID;
        if (path[i] == '/') {
            if (slash != OS_VFS_LIST_PAGE_END) return OS_VFS_STATUS_INVALID;
            slash = i;
        }
    }
    if (slash != OS_VFS_LIST_PAGE_END) {
        if (slash == 0U || path[slash + 1U] == '\0') return OS_VFS_STATUS_INVALID;
        for (i = 0U; i <= slash; i++) directory[i] = path[i];
        directory[slash + 1U] = '\0';
        list_path = directory;
        leaf = path + slash + 1U;
    }
    while ((count = worker_list_source_page(source, list_path, entries, start)) > 0) {
        for (i = 0U; i < (uint32_t)count; i++) {
            if (worker_ascii_fold_equal(entries[i].name, leaf)) { *out = entries[i]; return OS_VFS_STATUS_OK; }
        }
        if (count < (int)OS_VFS_LIST_ENTRY_MAX) break;
        start += (uint32_t)count;
    }
    return count < 0 ? count : OS_VFS_STATUS_NOT_MOUNTED;
}

static int worker_dynamic_mutate_write(const char* path, const uint8_t* data, uint32_t size) {
    uint32_t source;
    const char* relative;
    if (!worker_dynamic_path(path, &source, &relative)) return OS_VFS_STATUS_NOT_MOUNTED;
    if (source == OS_VFS_MOUNT_SOURCE_OVERLAY) return syscall_write(SYS_VFS_BACKEND_WRITE, relative, data, size);
    if (source == OS_VFS_MOUNT_SOURCE_FAT16) return syscall_write(SYS_VFS_FAT16_CREATE, relative, data, size);
    if (source == OS_VFS_MOUNT_SOURCE_FAT32) return syscall_write(SYS_VFS_FAT32_CREATE, relative, data, size);
    return OS_VFS_STATUS_NOT_MOUNTED;
}

static int worker_dynamic_mutate_remove(const char* path) {
    uint32_t source;
    const char* relative;
    if (!worker_dynamic_path(path, &source, &relative)) return OS_VFS_STATUS_NOT_MOUNTED;
    if (source == OS_VFS_MOUNT_SOURCE_OVERLAY) return syscall_remove(SYS_VFS_OVERLAY_UNLINK, relative);
    if (source == OS_VFS_MOUNT_SOURCE_FAT16) return syscall_remove(SYS_VFS_FAT16_UNLINK, relative);
    if (source == OS_VFS_MOUNT_SOURCE_FAT32) return syscall_remove(SYS_VFS_FAT32_UNLINK, relative);
    return OS_VFS_STATUS_NOT_MOUNTED;
}

static int worker_dynamic_mutate_rename(const char* old_path, const char* new_path) {
    uint32_t old_source;
    uint32_t new_source;
    const char* old_relative;
    const char* new_relative;
    if (!worker_dynamic_path(old_path, &old_source, &old_relative) ||
        !worker_dynamic_path(new_path, &new_source, &new_relative) || old_source != new_source)
        return OS_VFS_STATUS_NOT_MOUNTED;
    if (old_source == OS_VFS_MOUNT_SOURCE_OVERLAY) return syscall_rename(SYS_VFS_OVERLAY_RENAME, old_relative, new_relative);
    if (old_source == OS_VFS_MOUNT_SOURCE_FAT16) return syscall_rename(SYS_VFS_FAT16_RENAME, old_relative, new_relative);
    if (old_source == OS_VFS_MOUNT_SOURCE_FAT32) return syscall_rename(SYS_VFS_FAT32_RENAME, old_relative, new_relative);
    return OS_VFS_STATUS_NOT_MOUNTED;
}

static int worker_dynamic_mutate_mkdir(const char* path) {
    uint32_t source;
    const char* relative;
    if (!worker_dynamic_path(path, &source, &relative)) return OS_VFS_STATUS_NOT_MOUNTED;
    if (source == OS_VFS_MOUNT_SOURCE_OVERLAY) return syscall_remove(SYS_VFS_OVERLAY_MKDIR, relative);
    if (source == OS_VFS_MOUNT_SOURCE_FAT16) return syscall_write(SYS_VFS_FAT16_CREATE, relative, (const uint8_t*)0, 0U);
    if (source == OS_VFS_MOUNT_SOURCE_FAT32) return syscall_write(SYS_VFS_FAT32_CREATE, relative, (const uint8_t*)0, 0U);
    return OS_VFS_STATUS_NOT_MOUNTED;
}

static int worker_dynamic_mutate_rmdir(const char* path) {
    uint32_t source;
    const char* relative;
    char directory[OS_VFS_PATH_MAX];
    uint32_t i = 0U;
    int number;
    if (!worker_dynamic_path(path, &source, &relative)) return OS_VFS_STATUS_NOT_MOUNTED;
    if (source == OS_VFS_MOUNT_SOURCE_OVERLAY) return syscall_remove(SYS_VFS_OVERLAY_RMDIR, relative);
    if (source == OS_VFS_MOUNT_SOURCE_FAT16) number = SYS_VFS_FAT16_UNLINK;
    else if (source == OS_VFS_MOUNT_SOURCE_FAT32) number = SYS_VFS_FAT32_UNLINK;
    else return OS_VFS_STATUS_NOT_MOUNTED;
    while (relative[i] != '\0' && i + 2U < OS_VFS_PATH_MAX) { directory[i] = relative[i]; i++; }
    if (relative[i] != '\0' || i == 0U) return OS_VFS_STATUS_INVALID;
    directory[i++] = '/'; directory[i] = '\0';
    return syscall_remove(number, directory);
}

static int worker_alias_read(const char* path, uint8_t* data, uint32_t* size_out) {
    uint32_t source;
    const char* relative;
    int read;
    if (!data || !size_out || !worker_dynamic_path(path, &source, &relative)) return OS_VFS_STATUS_NOT_MOUNTED;
    if (source == OS_VFS_MOUNT_SOURCE_INITRD) read = worker_syscall_read(SYS_VFS_INITRD_READ, relative, data, OS_VFS_READ_MAX);
    else if (source == OS_VFS_MOUNT_SOURCE_OVERLAY) read = worker_syscall_read(SYS_VFS_OVERLAY_READ, relative, data, OS_VFS_READ_MAX);
    else if (source == OS_VFS_MOUNT_SOURCE_FAT16) read = worker_syscall_read(SYS_FAT16_READ, relative, data, OS_VFS_READ_MAX);
    else if (source == OS_VFS_MOUNT_SOURCE_FAT32) read = worker_syscall_read(SYS_FAT32_READ, relative, data, OS_VFS_READ_MAX);
    else return OS_VFS_STATUS_NOT_MOUNTED;
    if (read < 0) return read;
    *size_out = (uint32_t)read;
    return OS_VFS_STATUS_OK;
}

static int worker_alias_stat(const char* path, os_dirent_t* out) {
    uint32_t source;
    const char* relative;
    if (!out || !worker_dynamic_path(path, &source, &relative)) return OS_VFS_STATUS_NOT_MOUNTED;
    if (source == OS_VFS_MOUNT_SOURCE_INITRD) return worker_syscall_stat(SYS_VFS_INITRD_STAT, relative, out);
    if (source == OS_VFS_MOUNT_SOURCE_OVERLAY) return worker_syscall_stat(SYS_VFS_OVERLAY_STAT, relative, out);
    if (source == OS_VFS_MOUNT_SOURCE_FAT16 || source == OS_VFS_MOUNT_SOURCE_FAT32)
        return worker_fat_stat(source, relative, out);
    return OS_VFS_STATUS_NOT_MOUNTED;
}

static int worker_alias_list(const char* path, uint32_t start, uint8_t* data, uint32_t data_max,
                             uint32_t* size_out, uint32_t* count_out, uint32_t* next_out) {
    os_dirent_t entries[OS_VFS_LIST_ENTRY_MAX + 1U];
    uint32_t source;
    const char* relative;
    uint32_t written = 0U;
    uint32_t emitted = 0U;
    uint32_t entry_index;
    int listed;
    int status = OS_VFS_STATUS_OK;
    if (!data || !size_out || !count_out || !next_out || !worker_dynamic_list_path(path, &source, &relative))
        return OS_VFS_STATUS_NOT_MOUNTED;
    *next_out = OS_VFS_LIST_PAGE_END;
    listed = start == OS_VFS_LIST_PAGE_END
        ? worker_list_source(source, relative, entries, (int)(OS_VFS_LIST_ENTRY_MAX + 1U))
        : worker_list_source_page(source, relative, entries, start);
    if (listed < 0) return listed;
    for (entry_index = 0U; entry_index < (uint32_t)listed && entry_index < OS_VFS_LIST_ENTRY_MAX; entry_index++) {
        uint32_t name_size = string_length(entries[entry_index].name);
        if (written + name_size + 1U > data_max) { status = OS_VFS_STATUS_TRUNCATED; break; }
        for (uint32_t j = 0U; j < name_size; j++) data[written++] = (uint8_t)entries[entry_index].name[j];
        data[written++] = (uint8_t)'\n';
        emitted++;
    }
    if (start != OS_VFS_LIST_PAGE_END && (uint32_t)listed > emitted) {
        status = OS_VFS_STATUS_TRUNCATED;
        *next_out = start + (emitted == 0U ? 1U : emitted);
    } else if (start == OS_VFS_LIST_PAGE_END && (uint32_t)listed > OS_VFS_LIST_ENTRY_MAX) {
        status = OS_VFS_STATUS_TRUNCATED;
    }
    *size_out = written;
    *count_out = emitted;
    return status;
}

void main(void) {
    static const uint8_t info[] = "vfsserver ring3 policy\n";
    os_ipc_message_t message;
    os_ipc_payload_t reply;
    char path[OS_VFS_PATH_MAX];
    uint8_t data[OS_VFS_READ_MAX];
    if (service_register("vfs-virtual") != 0) {
        puts("vfsvirtual register failed\n");
        for (;;) yield();
    }
    puts("vfsvirtual ready\n");
    for (;;) {
        int received = ipc_receive(&message);
        if (received == 0 && message.type == OS_IPC_VFS_WORKER_READ &&
            worker_sender_is_vfs(&message) &&
            os_vfs_parse_worker_read_request(&message, path) == OS_VFS_STATUS_OK) {
            const uint8_t* reply_data = (const uint8_t*)0;
            uint32_t size = 0U;
            int32_t status = OS_VFS_STATUS_NOT_MOUNTED;
            if (string_equal(path, "vfs-info")) {
                puts("vfsvirtual read vfs-info\n");
                reply_data = info;
                size = (uint32_t)(sizeof(info) - 1U);
                status = OS_VFS_STATUS_OK;
            } else {
                status = worker_alias_read(path, data, &size);
                if (status == OS_VFS_STATUS_OK) {
                    reply_data = data;
                    puts("vfsvirtual alias read "); puts(path); puts("\n");
                }
            }
            if (os_vfs_make_worker_read_reply(&reply, status, reply_data, size, message.request_id)
                == OS_VFS_STATUS_OK) (void)ipc_send(message.sender_pid, &reply);
        } else if (received == 0 && message.type == OS_IPC_VFS_WORKER_STATS &&
                   worker_sender_is_vfs(&message)) {
            uint32_t reads, writes, removes, renames;
            if (os_vfs_parse_worker_stats_request(&message, &reads, &writes, &removes, &renames)
                == OS_VFS_STATUS_OK) {
                uint32_t size = format_stats(data, reads, writes, removes, renames);
                puts("vfsvirtual format stats\n");
                if (os_vfs_make_worker_read_reply(&reply, OS_VFS_STATUS_OK, data, size,
                                                  message.request_id) == OS_VFS_STATUS_OK) {
                    (void)ipc_send(message.sender_pid, &reply);
                }
            }
        } else if (received == 0 && message.type == OS_IPC_VFS_WORKER_MOUNT &&
                   worker_sender_is_vfs(&message)) {
            uint32_t writable;
            if (os_vfs_parse_worker_mount_request(&message, path, &writable) == OS_VFS_STATUS_OK) {
                uint32_t size = append_text(data, 0U, path);
                size = append_text(data, size, writable ? " rw\n" : " ro\n");
                puts("vfsvirtual format mount\n");
                if (os_vfs_make_worker_read_reply(&reply, OS_VFS_STATUS_OK, data, size,
                                                  message.request_id) == OS_VFS_STATUS_OK) {
                    (void)ipc_send(message.sender_pid, &reply);
                }
            }
        } else if (received == 0 && message.type == OS_IPC_VFS_WORKER_STAT &&
                   worker_sender_is_vfs(&message)) {
            os_dirent_t entry;
            if (os_vfs_parse_worker_stat_request(&message, path) == OS_VFS_STATUS_OK) {
                int32_t status = worker_alias_stat(path, &entry);
                puts("vfsvirtual alias stat "); puts(path); puts("\n");
                if (os_vfs_make_worker_stat_reply(&reply, status,
                                                   status == OS_VFS_STATUS_OK ? entry.size : 0U,
                                                   status == OS_VFS_STATUS_OK ? entry.flags : 0U,
                                                   message.request_id) == OS_VFS_STATUS_OK) {
                    (void)ipc_send(message.sender_pid, &reply);
                }
            }
        } else if (received == 0 && message.type == OS_IPC_VFS_WORKER_LIST &&
                   worker_sender_is_vfs(&message)) {
            uint32_t size = 0U;
            uint32_t count = 0U;
            uint32_t ignored_next = OS_VFS_LIST_PAGE_END;
            if (os_vfs_parse_worker_list_request(&message, path) == OS_VFS_STATUS_OK) {
                int32_t status = worker_alias_list(path, OS_VFS_LIST_PAGE_END, data,
                                                    OS_VFS_LIST_DATA_MAX, &size, &count,
                                                    &ignored_next);
                puts("vfsvirtual alias list "); puts(path); puts("\n");
                if (os_vfs_make_worker_list_reply(&reply, status, count,
                                                  status < 0 ? (const uint8_t*)0 : data,
                                                  status < 0 ? 0U : size,
                                                  message.request_id) == OS_VFS_STATUS_OK) {
                    (void)ipc_send(message.sender_pid, &reply);
                }
            }
        } else if (received == 0 && message.type == OS_IPC_VFS_WORKER_LIST_PAGE &&
                   worker_sender_is_vfs(&message)) {
            uint32_t size = 0U;
            uint32_t count = 0U;
            uint32_t start = 0U;
            uint32_t next = OS_VFS_LIST_PAGE_END;
            if (os_vfs_parse_worker_list_page_request(&message, path, &start) == OS_VFS_STATUS_OK) {
                int32_t status = OS_VFS_STATUS_OK;
                if (start != OS_VFS_LIST_PAGE_END) {
                    status = worker_alias_list(path, start, data, OS_VFS_LIST_PAGE_DATA_MAX,
                                               &size, &count, &next);
                }
                puts("vfsvirtual alias list page "); puts(path); puts("\n");
                if (os_vfs_make_worker_list_page_reply(&reply, status, count, next,
                                                       status < 0 ? (const uint8_t*)0 : data,
                                                       status < 0 ? 0U : size,
                                                       message.request_id) == OS_VFS_STATUS_OK) {
                    (void)ipc_send(message.sender_pid, &reply);
                }
            }
        } else if (received == 0 && message.type == OS_IPC_VFS_WORKER_MOUNT_ADD &&
                   worker_sender_is_vfs(&message)) {
            uint32_t source;
            if (os_vfs_parse_worker_mount_add_request(&message, path, &source) == OS_VFS_STATUS_OK) {
                int32_t status = worker_mount_add(path, source);
                puts("vfsvirtual mount add "); puts(path); puts("\n");
                if (os_vfs_make_worker_mount_reply(&reply, OS_IPC_VFS_WORKER_MOUNT_ADD_REPLY,
                                                   status, message.request_id) == OS_VFS_STATUS_OK) {
                    (void)ipc_send(message.sender_pid, &reply);
                }
            }
        } else if (received == 0 && message.type == OS_IPC_VFS_WORKER_MOUNT_REMOVE &&
                   worker_sender_is_vfs(&message)) {
            if (os_vfs_parse_worker_mount_remove_request(&message, path) == OS_VFS_STATUS_OK) {
                int32_t status = worker_mount_remove(path);
                puts("vfsvirtual mount remove "); puts(path); puts("\n");
                if (os_vfs_make_worker_mount_reply(&reply, OS_IPC_VFS_WORKER_MOUNT_REMOVE_REPLY,
                                                   status, message.request_id) == OS_VFS_STATUS_OK) {
                    (void)ipc_send(message.sender_pid, &reply);
                }
            }
        } else if (received == 0 && message.type == OS_IPC_VFS_WORKER_WRITE &&
                   worker_sender_is_vfs(&message)) {
            uint8_t write_buf[OS_VFS_WRITE_MAX];
            uint32_t write_len = 0U;
            if (os_vfs_parse_worker_write_request(&message, path, write_buf, &write_len) == OS_VFS_STATUS_OK) {
                int32_t status = mutate_write(path, write_buf, write_len);
                puts("vfsvirtual write ");
                puts(path);
                puts("\n");
                if (os_vfs_make_write_reply(&reply, status, message.request_id) == OS_VFS_STATUS_OK) {
                    (void)ipc_send(message.sender_pid, &reply);
                }
            }
        } else if (received == 0 && message.type == OS_IPC_VFS_WORKER_REMOVE &&
                   worker_sender_is_vfs(&message)) {
            if (os_vfs_parse_worker_remove_request(&message, path) == OS_VFS_STATUS_OK) {
                int32_t status = mutate_remove(path);
                puts("vfsvirtual remove ");
                puts(path);
                puts("\n");
                if (os_vfs_make_remove_reply(&reply, status, message.request_id) == OS_VFS_STATUS_OK) {
                    (void)ipc_send(message.sender_pid, &reply);
                }
            }
        } else if (received == 0 && message.type == OS_IPC_VFS_WORKER_RENAME &&
                   worker_sender_is_vfs(&message)) {
            char new_path[OS_VFS_PATH_MAX];
            if (os_vfs_parse_worker_rename_request(&message, path, new_path) == OS_VFS_STATUS_OK) {
                int32_t status = mutate_rename(path, new_path);
                puts("vfsvirtual rename ");
                puts(path);
                puts(" -> ");
                puts(new_path);
                puts("\n");
                if (os_vfs_make_rename_reply(&reply, status, message.request_id) == OS_VFS_STATUS_OK) {
                    (void)ipc_send(message.sender_pid, &reply);
                }
            }
        } else if (received == 0 && message.type == OS_IPC_VFS_WORKER_MKDIR &&
                   worker_sender_is_vfs(&message)) {
            if (os_vfs_parse_worker_directory_request(&message, OS_IPC_VFS_WORKER_MKDIR, path)
                == OS_VFS_STATUS_OK) {
                int32_t status = mutate_mkdir(path);
                puts("vfsvirtual mkdir "); puts(path); puts("\n");
                if (os_vfs_make_mkdir_reply(&reply, status, message.request_id) == OS_VFS_STATUS_OK)
                    (void)ipc_send(message.sender_pid, &reply);
            }
        } else if (received == 0 && message.type == OS_IPC_VFS_WORKER_RMDIR &&
                   worker_sender_is_vfs(&message)) {
            if (os_vfs_parse_worker_directory_request(&message, OS_IPC_VFS_WORKER_RMDIR, path)
                == OS_VFS_STATUS_OK) {
                int32_t status = mutate_rmdir(path);
                puts("vfsvirtual rmdir "); puts(path); puts("\n");
                if (os_vfs_make_rmdir_reply(&reply, status, message.request_id) == OS_VFS_STATUS_OK)
                    (void)ipc_send(message.sender_pid, &reply);
            }
        }
        yield();
    }
}
