#include "os_syscalls.h"
#include "os_vfs_service.h"

static void putc(char c) {
    asm volatile("int $0x80" : : "a"(SYS_PUTC), "b"(c));
}

static void puts(const char* text) {
    int i = 0;
    while (text[i] != '\0') putc(text[i++]);
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

static int service_grant(const char* name, int target_pid) {
    int result;
    asm volatile("int $0x80" : "=a"(result) : "a"(SYS_SERVICE_GRANT), "b"(name), "c"(target_pid));
    return result;
}

static int service_backend_grant(const char* name, int target_pid) {
    int result;
    asm volatile("int $0x80" : "=a"(result) : "a"(SYS_SERVICE_BACKEND_GRANT), "b"(name), "c"(target_pid));
    return result;
}

static int service_backend_grant_scoped(const char* name, int target_pid, uint32_t rights) {
    int result;
    asm volatile("int $0x80" : "=a"(result) : "a"(SYS_SERVICE_BACKEND_GRANT_SCOPED), "b"(name), "c"(target_pid), "d"(rights));
    return result;
}

static int service_backend_grant_scoped_source(const char* name, int target_pid, uint32_t rights,
                                               uint32_t sources) {
    int result;
    asm volatile("int $0x80" : "=a"(result) : "a"(SYS_SERVICE_BACKEND_GRANT_SCOPED_SOURCE),
                 "b"(name), "c"(target_pid), "d"(rights), "S"(sources));
    return result;
}

static int service_backend_grant_scoped_source_prefix(const char* name, int target_pid,
                                                      uint32_t rights, uint32_t sources,
                                                      const char* prefix) {
    int result;
    asm volatile("int $0x80" : "=a"(result) : "a"(SYS_SERVICE_BACKEND_GRANT_SCOPED_SOURCE_PREFIX),
                 "b"(name), "c"(target_pid), "d"(rights), "S"(sources), "D"(prefix));
    return result;
}

static int service_backend_revoke(const char* name, int target_pid) {
    int result;
    asm volatile("int $0x80" : "=a"(result) : "a"(SYS_SERVICE_BACKEND_REVOKE), "b"(name), "c"(target_pid));
    return result;
}

static int service_backend_status(const char* name, int target_pid, uint32_t* rights) {
    int result;
    asm volatile("int $0x80" : "=a"(result) : "a"(SYS_SERVICE_BACKEND_STATUS), "b"(name), "c"(target_pid), "d"(rights));
    return result;
}

static int service_backend_scope_status(const char* name, int target_pid,
                                        os_service_backend_scope_t* scope) {
    int result;
    asm volatile("int $0x80" : "=a"(result) : "a"(SYS_SERVICE_BACKEND_SCOPE_STATUS),
                 "b"(name), "c"(target_pid), "d"(scope));
    return result;
}

static uint32_t vfs_backend_source_mask(uint32_t source) {
    if (source == OS_VFS_MOUNT_SOURCE_INITRD) return OS_SERVICE_BACKEND_SOURCE_INITRD;
    if (source == OS_VFS_MOUNT_SOURCE_OVERLAY) return OS_SERVICE_BACKEND_SOURCE_OVERLAY;
    if (source == OS_VFS_MOUNT_SOURCE_FAT16) return OS_SERVICE_BACKEND_SOURCE_FAT16;
    if (source == OS_VFS_MOUNT_SOURCE_FAT32) return OS_SERVICE_BACKEND_SOURCE_FAT32;
    return 0U;
}

static int service_backend_list(const char* name, os_service_backend_list_t* list) {
    int result;
    asm volatile("int $0x80" : "=a"(result) : "a"(SYS_SERVICE_BACKEND_LIST), "b"(name), "c"(list));
    return result;
}

static int service_backend_observe(const char* name, uint32_t expected_generation,
                                   os_service_backend_snapshot_t* snapshot) {
    int result;
    asm volatile("int $0x80" : "=a"(result) : "a"(SYS_SERVICE_BACKEND_OBSERVE), "b"(name),
                 "c"(expected_generation), "d"(snapshot));
    return result;
}

static void print_int(int value) {
    char digits[12];
    int n = 0;
    unsigned int number;
    if (value < 0) { putc('-'); number = (unsigned int)(-value); }
    else number = (unsigned int)value;
    if (number == 0U) { putc('0'); return; }
    while (number > 0U && n < 11) { digits[n++] = (char)('0' + (number % 10U)); number /= 10U; }
    while (n > 0) putc(digits[--n]);
}

static int string_equal_ascii_fold(const char* left, const char* right) {
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

static int backend_initrd_read(const char* path, char* buffer, uint32_t max) {
    int result;
    asm volatile("int $0x80" : "=a"(result) : "a"(SYS_VFS_INITRD_READ), "b"(path), "c"(buffer), "d"(max));
    return result;
}

static int backend_overlay_read(const char* path, char* buffer, uint32_t max) {
    int result;
    asm volatile("int $0x80" : "=a"(result) : "a"(SYS_VFS_OVERLAY_READ), "b"(path), "c"(buffer), "d"(max));
    return result;
}
static int backend_fat16_read(const char* path, char* buffer, uint32_t max) {
    int result;
    asm volatile("int $0x80" : "=a"(result) : "a"(SYS_FAT16_READ), "b"(path), "c"(buffer), "d"(max));
    return result;
}
static int backend_fat16_listdir(const char* path, os_dirent_t* out, int max_n) {
    int result;
    if (!path || !out || max_n <= 0) return -1;
    asm volatile("int $0x80" : "=a"(result) : "a"(SYS_FAT16_LIST_PATH),
                 "b"(path), "c"(out), "d"(max_n), "S"(0U));
    return result;
}
static int backend_fat16_listdir_page(const char* path, os_dirent_t* out, uint32_t start) {
    int result;
    if (!path || !out) return -1;
    if (path[0] == '/' && path[1] == '\0') {
        asm volatile("int $0x80" : "=a"(result) : "a"(SYS_FAT16_LIST_PAGE), "b"(out),
                     "c"(OS_VFS_LIST_ENTRY_MAX + 1U), "d"(start));
        return result;
    }
    asm volatile("int $0x80" : "=a"(result) : "a"(SYS_FAT16_LIST_PATH),
                 "b"(path), "c"(out), "d"(OS_VFS_LIST_ENTRY_MAX + 1U), "S"(start));
    return result;
}
static int backend_fat16_stat(const char* path, os_dirent_t* out) {
    os_dirent_t entries[OS_VFS_LIST_ENTRY_MAX + 1U];
    char directory[OS_VFS_PATH_MAX];
    const char* list_path = "/";
    const char* leaf = path;
    uint32_t start = 0U, i, slash = OS_VFS_LIST_PAGE_END;
    int count;
    if (!path || !out || path[0] == '\0' || path[0] == '/') return -1;
    for (i = 0U; path[i] != '\0'; i++) {
        if (i + 1U >= OS_VFS_PATH_MAX) return -1;
        if (path[i] == '/') {
            slash = i;
        }
    }
    if (slash != OS_VFS_LIST_PAGE_END) {
        if (slash == 0U || path[slash + 1U] == '\0') return -1;
        for (i = 0U; i <= slash; i++) directory[i] = path[i];
        directory[slash + 1U] = '\0';
        list_path = directory;
        leaf = path + slash + 1U;
    }
    while ((count = backend_fat16_listdir_page(list_path, entries, start)) > 0) {
        for (i = 0U; i < (uint32_t)count; i++) {
            if (string_equal_ascii_fold(entries[i].name, leaf)) { *out = entries[i]; return 0; }
        }
        if (count < (int)OS_VFS_LIST_ENTRY_MAX) break;
        start += (uint32_t)count;
    }
    return -1;
}

/* FAT16 publie la racine 8.3/LFN et un sous-répertoire 8.3, sans remplacement. */
static int backend_fat16_create(const char* path, const uint8_t* data, uint32_t size) {
    os_dirent_t existing;
    int result;
    if (!path || path[0] == '\0' || path[0] == '/') return OS_VFS_STATUS_INVALID;
    if (backend_fat16_stat(path, &existing) == 0) return OS_VFS_STATUS_INVALID;
    asm volatile("int $0x80" : "=a"(result) : "a"(SYS_VFS_FAT16_CREATE),
                 "b"(path), "c"(data), "d"(size));
    return result;
}
static int backend_fat16_remove(const char* path) {
    int result;
    if (!path || path[0] == '\0' || path[0] == '/') return OS_VFS_STATUS_INVALID;
    asm volatile("int $0x80" : "=a"(result) : "a"(SYS_VFS_FAT16_UNLINK), "b"(path));
    return result;
}
static int backend_fat16_rename(const char* oldpath, const char* newpath) {
    int result;
    if (!oldpath || !newpath || oldpath[0] == '\0' || newpath[0] == '\0' ||
        oldpath[0] == '/' || newpath[0] == '/') return OS_VFS_STATUS_INVALID;
    asm volatile("int $0x80" : "=a"(result) : "a"(SYS_VFS_FAT16_RENAME),
                 "b"(oldpath), "c"(newpath));
    return result;
}
static int backend_fat32_read(const char* path, char* buffer, uint32_t max) {
    int result;
    asm volatile("int $0x80" : "=a"(result) : "a"(SYS_FAT32_READ), "b"(path), "c"(buffer), "d"(max));
    return result;
}
static int backend_fat32_listdir(const char* path, os_dirent_t* out, int max_n) {
    int result;
    if (!path || !out || max_n <= 0) return -1;
    asm volatile("int $0x80" : "=a"(result) : "a"(SYS_FAT32_LIST_PATH),
                 "b"(path), "c"(out), "d"(max_n), "S"(0U));
    return result;
}
static int backend_fat32_listdir_page(const char* path, os_dirent_t* out, uint32_t start) {
    int result;
    if (!path || !out) return -1;
    if (path[0] == '/' && path[1] == '\0') {
        asm volatile("int $0x80" : "=a"(result) : "a"(SYS_FAT32_LIST_PAGE), "b"(out),
                     "c"(OS_VFS_LIST_ENTRY_MAX + 1U), "d"(start));
        return result;
    }
    asm volatile("int $0x80" : "=a"(result) : "a"(SYS_FAT32_LIST_PATH),
                 "b"(path), "c"(out), "d"(OS_VFS_LIST_ENTRY_MAX + 1U), "S"(start));
    return result;
}
static int backend_fat32_stat(const char* path, os_dirent_t* out) {
    os_dirent_t entries[OS_VFS_LIST_ENTRY_MAX + 1U];
    char directory[OS_VFS_PATH_MAX];
    const char* list_path = "/";
    const char* leaf = path;
    uint32_t start = 0U, i, slash = OS_VFS_LIST_PAGE_END;
    int count;
    if (!path || !out || path[0] == '\0' || path[0] == '/') return -1;
    for (i = 0U; path[i] != '\0'; i++) {
        if (i + 1U >= OS_VFS_PATH_MAX) return -1;
        if (path[i] == '/') {
            slash = i;
        }
    }
    if (slash != OS_VFS_LIST_PAGE_END) {
        if (slash == 0U || path[slash + 1U] == '\0') return -1;
        for (i = 0U; i <= slash; i++) directory[i] = path[i];
        directory[slash + 1U] = '\0';
        list_path = directory;
        leaf = path + slash + 1U;
    }
    while ((count = backend_fat32_listdir_page(list_path, entries, start)) > 0) {
        for (i = 0U; i < (uint32_t)count; i++) {
            if (string_equal_ascii_fold(entries[i].name, leaf)) { *out = entries[i]; return 0; }
        }
        if (count < (int)OS_VFS_LIST_ENTRY_MAX) break;
        start += (uint32_t)count;
    }
    return -1;
}

/* FAT32 publie la racine 8.3/LFN et un sous-répertoire 8.3, sans remplacement. */
static int backend_fat32_create(const char* path, const uint8_t* data, uint32_t size) {
    os_dirent_t existing;
    int result;
    if (!path || path[0] == '\0' || path[0] == '/') return OS_VFS_STATUS_INVALID;
    if (backend_fat32_stat(path, &existing) == 0) return OS_VFS_STATUS_INVALID;
    asm volatile("int $0x80" : "=a"(result) : "a"(SYS_VFS_FAT32_CREATE),
                 "b"(path), "c"(data), "d"(size));
    return result;
}
static int backend_fat32_remove(const char* path) {
    int result;
    if (!path || path[0] == '\0' || path[0] == '/') return OS_VFS_STATUS_INVALID;
    asm volatile("int $0x80" : "=a"(result) : "a"(SYS_VFS_FAT32_UNLINK), "b"(path));
    return result;
}
static int backend_fat32_rename(const char* oldpath, const char* newpath) {
    int result;
    if (!oldpath || !newpath || oldpath[0] == '\0' || newpath[0] == '\0' ||
        oldpath[0] == '/' || newpath[0] == '/') return OS_VFS_STATUS_INVALID;
    asm volatile("int $0x80" : "=a"(result) : "a"(SYS_VFS_FAT32_RENAME),
                 "b"(oldpath), "c"(newpath));
    return result;
}

static int backend_fat16_mkdir(const char* path) {
    int result;
    if (!path || path[0] == '\0' || path[0] == '/') return OS_VFS_STATUS_INVALID;
    asm volatile("int $0x80" : "=a"(result) : "a"(SYS_VFS_FAT16_CREATE),
                 "b"(path), "c"(0), "d"(0));
    return result;
}

static int backend_fat32_mkdir(const char* path) {
    int result;
    if (!path || path[0] == '\0' || path[0] == '/') return OS_VFS_STATUS_INVALID;
    asm volatile("int $0x80" : "=a"(result) : "a"(SYS_VFS_FAT32_CREATE),
                 "b"(path), "c"(0), "d"(0));
    return result;
}

static int backend_fat16_rmdir(const char* path) {
    char directory[OS_VFS_PATH_MAX];
    uint32_t i;
    int result;
    if (!path || path[0] == '\0' || path[0] == '/') return OS_VFS_STATUS_INVALID;
    for (i = 0U; i + 2U < OS_VFS_PATH_MAX && path[i] != '\0'; i++) directory[i] = path[i];
    if (path[i] != '\0') return OS_VFS_STATUS_INVALID;
    directory[i++] = '/'; directory[i] = '\0';
    asm volatile("int $0x80" : "=a"(result) : "a"(SYS_VFS_FAT16_UNLINK), "b"(directory));
    return result;
}

static int backend_fat32_rmdir(const char* path) {
    char directory[OS_VFS_PATH_MAX];
    uint32_t i;
    int result;
    if (!path || path[0] == '\0' || path[0] == '/') return OS_VFS_STATUS_INVALID;
    for (i = 0U; i + 2U < OS_VFS_PATH_MAX && path[i] != '\0'; i++) directory[i] = path[i];
    if (path[i] != '\0') return OS_VFS_STATUS_INVALID;
    directory[i++] = '/'; directory[i] = '\0';
    asm volatile("int $0x80" : "=a"(result) : "a"(SYS_VFS_FAT32_UNLINK), "b"(directory));
    return result;
}

static int backend_initrd_stat(const char* path, os_dirent_t* out) {
    int result;
    asm volatile("int $0x80" : "=a"(result) : "a"(SYS_VFS_INITRD_STAT), "b"(path), "c"(out));
    return result;
}

static int backend_overlay_stat(const char* path, os_dirent_t* out) {
    int result;
    asm volatile("int $0x80" : "=a"(result) : "a"(SYS_VFS_OVERLAY_STAT), "b"(path), "c"(out));
    return result;
}

static int backend_initrd_listdir(const char* path, os_dirent_t* out, int max_n) {
    int result;
    asm volatile("int $0x80" : "=a"(result) : "a"(SYS_VFS_INITRD_LISTDIR),
                 "b"(path), "c"(out), "d"(max_n));
    return result;
}

static int backend_overlay_listdir(const char* path, os_dirent_t* out, int max_n) {
    int result;
    asm volatile("int $0x80" : "=a"(result) : "a"(SYS_VFS_OVERLAY_LISTDIR),
                 "b"(path), "c"(out), "d"(max_n));
    return result;
}

static int backend_initrd_listdir_page(const char* path, os_dirent_t* out, uint32_t start) {
    int result;
    asm volatile("int $0x80" : "=a"(result) : "a"(SYS_VFS_INITRD_LISTDIR_PAGE),
                 "b"(path), "c"(out), "d"(start));
    return result;
}

static int backend_overlay_listdir_page(const char* path, os_dirent_t* out, uint32_t start) {
    int result;
    asm volatile("int $0x80" : "=a"(result) : "a"(SYS_VFS_OVERLAY_LISTDIR_PAGE),
                 "b"(path), "c"(out), "d"(start));
    return result;
}

static int backend_write(const char* path, const uint8_t* data, uint32_t size) {
    int result;
    asm volatile("int $0x80" : "=a"(result) : "a"(SYS_VFS_BACKEND_WRITE), "b"(path), "c"(data), "d"(size));
    return result;
}

static int backend_mkdir(const char* path) {
    int result;
    asm volatile("int $0x80" : "=a"(result) : "a"(SYS_VFS_OVERLAY_MKDIR), "b"(path));
    return result;
}

static int backend_rmdir(const char* path) {
    int result;
    asm volatile("int $0x80" : "=a"(result) : "a"(SYS_VFS_OVERLAY_RMDIR), "b"(path));
    return result;
}

static int backend_remove(const char* path) {
    int result;
    asm volatile("int $0x80" : "=a"(result) : "a"(SYS_VFS_OVERLAY_UNLINK), "b"(path));
    return result;
}

static int backend_rename(const char* oldpath, const char* newpath) {
    int result;
    asm volatile("int $0x80" : "=a"(result) : "a"(SYS_VFS_OVERLAY_RENAME), "b"(oldpath), "c"(newpath));
    return result;
}

typedef struct {
    uint32_t source;
    int (*read)(const char* path, char* buffer, uint32_t max);
    int (*stat)(const char* path, os_dirent_t* out);
    int (*list)(const char* path, os_dirent_t* out, int max_n);
    int (*list_page)(const char* path, os_dirent_t* out, uint32_t start);
    int (*write)(const char* path, const uint8_t* data, uint32_t size);
    int (*mkdir)(const char* path);
    int (*rmdir)(const char* path);
    int (*remove)(const char* path);
    int (*rename)(const char* oldpath, const char* newpath);
} vfs_backend_ops_t;

static const vfs_backend_ops_t vfs_backend_ops[] = {
    { OS_VFS_MOUNT_SOURCE_INITRD, backend_initrd_read, backend_initrd_stat,
      backend_initrd_listdir, backend_initrd_listdir_page, 0, 0, 0, 0, 0 },
    { OS_VFS_MOUNT_SOURCE_OVERLAY, backend_overlay_read, backend_overlay_stat,
      backend_overlay_listdir, backend_overlay_listdir_page, backend_write, backend_mkdir,
      backend_rmdir, backend_remove, backend_rename },
    { OS_VFS_MOUNT_SOURCE_FAT16, backend_fat16_read, backend_fat16_stat,
      backend_fat16_listdir, backend_fat16_listdir_page, backend_fat16_create, backend_fat16_mkdir,
      backend_fat16_rmdir, backend_fat16_remove, backend_fat16_rename },
    { OS_VFS_MOUNT_SOURCE_FAT32, backend_fat32_read, backend_fat32_stat,
      backend_fat32_listdir, backend_fat32_listdir_page, backend_fat32_create, backend_fat32_mkdir,
      backend_fat32_rmdir, backend_fat32_remove, backend_fat32_rename },
};

static const vfs_backend_ops_t* vfs_backend_ops_for(uint32_t source) {
    uint32_t i;
    for (i = 0U; i < (uint32_t)(sizeof(vfs_backend_ops) / sizeof(vfs_backend_ops[0])); i++) {
        if (vfs_backend_ops[i].source == source) return &vfs_backend_ops[i];
    }
    return 0;
}

static void yield(void) {
    asm volatile("int $0x80" : : "a"(SYS_YIELD));
}

static int string_equal(const char* left, const char* right) {
    uint32_t i = 0U;
    while (left[i] != '\0' && right[i] != '\0') {
        if (left[i] != right[i]) return 0;
        i++;
    }
    return left[i] == right[i];
}

static int string_has_prefix(const char* text, const char* prefix) {
    uint32_t i = 0U;
    while (prefix[i] != '\0') {
        if (text[i] != prefix[i]) return 0;
        i++;
    }
    return 1;
}

/* Table locale de montages : les entrées de démarrage sont protégées ; quatre
 * alias dynamiques restent possibles. Les noms dynamiques sont bornés pour
 * que la source virtuelle `vfs-mounts` tienne toujours dans 80 octets. */
#define VFS_MOUNT_MAX 8U
#define VFS_BOOT_MOUNT_COUNT 4U
#define VFS_DYNAMIC_MOUNT_MAX 13U
typedef struct {
    char prefix[OS_VFS_PATH_MAX];
    uint32_t source;
    uint32_t protected_mount;
} vfs_mount_t;
static vfs_mount_t vfs_mounts[VFS_MOUNT_MAX] = {
    { "initrd/", OS_VFS_MOUNT_SOURCE_INITRD, 1U },
    { "overlay/", OS_VFS_MOUNT_SOURCE_OVERLAY, 1U },
    { "fat16/", OS_VFS_MOUNT_SOURCE_FAT16, 1U },
    { "fat32/", OS_VFS_MOUNT_SOURCE_FAT32, 1U },
};
static uint32_t vfs_mount_count = VFS_BOOT_MOUNT_COUNT;
/* Le worker Ring 3 est l’autorité des alias dynamiques. Le médiateur ne garde
 * qu’un miroir de routage, purgé au remplacement du PID ou dès qu’une mutation
 * envoyée devient incertaine. */
static int vfs_mount_worker_pid = -1;
static uint32_t vfs_mount_worker_trusted = 1U;
/* Génération volatile des contenus et de la table de montages. Elle n’est ni
 * persistante ni atomique : elle avertit seulement le client d’une mutation
 * visible entre deux pages. */
static uint32_t vfs_list_generation = 1U;

static uint32_t string_length(const char* text) {
    uint32_t i = 0U;
    while (text[i] != '\0') i++;
    return i;
}

static int mount_prefixes_overlap(const char* left, const char* right) {
    uint32_t i = 0U;
    while (left[i] != '\0' && right[i] != '\0' && left[i] == right[i]) i++;
    return left[i] == '\0' || right[i] == '\0';
}

static int vfs_mount_add(const char* prefix, uint32_t source) {
    uint32_t i;
    if (string_length(prefix) > VFS_DYNAMIC_MOUNT_MAX) return OS_VFS_STATUS_INVALID;
    for (i = 0U; i < vfs_mount_count; i++) {
        if (string_equal(prefix, vfs_mounts[i].prefix)) return OS_VFS_STATUS_MOUNT_EXISTS;
        if (mount_prefixes_overlap(prefix, vfs_mounts[i].prefix)) return OS_VFS_STATUS_INVALID;
    }
    if (vfs_mount_count >= VFS_MOUNT_MAX) return OS_VFS_STATUS_MOUNT_FULL;
    for (i = 0U; i < OS_VFS_PATH_MAX; i++) vfs_mounts[vfs_mount_count].prefix[i] = prefix[i];
    vfs_mounts[vfs_mount_count].source = source;
    vfs_mounts[vfs_mount_count].protected_mount = 0U;
    vfs_mount_count++;
    return OS_VFS_STATUS_OK;
}

static int vfs_mount_remove(const char* prefix) {
    uint32_t i;
    for (i = 0U; i < vfs_mount_count; i++) {
        if (string_equal(prefix, vfs_mounts[i].prefix)) {
            uint32_t j;
            if (vfs_mounts[i].protected_mount) return OS_VFS_STATUS_INVALID;
            for (j = i; j + 1U < vfs_mount_count; j++) vfs_mounts[j] = vfs_mounts[j + 1U];
            vfs_mount_count--;
            vfs_mounts[vfs_mount_count].prefix[0] = '\0';
            vfs_mounts[vfs_mount_count].source = 0U;
            vfs_mounts[vfs_mount_count].protected_mount = 0U;
            return OS_VFS_STATUS_OK;
        }
    }
    return OS_VFS_STATUS_NOT_MOUNTED;
}

static void vfs_mount_reset_dynamic(void) {
    if (vfs_mount_count > VFS_BOOT_MOUNT_COUNT) vfs_list_generation++;
    while (vfs_mount_count > VFS_BOOT_MOUNT_COUNT) {
        vfs_mount_count--;
        vfs_mounts[vfs_mount_count].prefix[0] = '\0';
        vfs_mounts[vfs_mount_count].source = 0U;
        vfs_mounts[vfs_mount_count].protected_mount = 0U;
    }
}

static int vfs_mount_is_protected(const char* prefix) {
    uint32_t i;
    for (i = 0U; i < vfs_mount_count; i++) {
        if (string_equal(prefix, vfs_mounts[i].prefix)) return vfs_mounts[i].protected_mount ? 1 : 0;
    }
    return 0;
}

static void vfs_mount_mark_worker_unknown(void) {
    vfs_mount_reset_dynamic();
    vfs_mount_worker_trusted = 0U;
}

/* Observabilité locale : compteurs volatils, remis à zéro au démarrage du
 * service. Toute requête VFS reconnue est comptée avant sa validation afin
 * que les refus de politique restent visibles. */
static uint32_t vfs_read_requests;
static uint32_t vfs_write_requests;
static uint32_t vfs_remove_requests;
static uint32_t vfs_rename_requests;
/* Nombre volatile de transactions privées terminées localement après disparition
 * ou remplacement du PID publié par le worker virtuel. */
static uint32_t vfs_virtual_recoveries;
/* Nombre volatile de workers publiés mais silencieux au-delà du budget local. */
static uint32_t vfs_virtual_timeouts;
#define VFS_VIRTUAL_PENDING_SIMPLE 0U
#define VFS_VIRTUAL_PENDING_MOUNTS 1U
#define VFS_VIRTUAL_PENDING_MOUNT_PAGE 2U
#define VFS_VIRTUAL_PENDING_MOUNT_OBSERVE 3U
#define VFS_VIRTUAL_PENDING_WRITE 4U
#define VFS_VIRTUAL_PENDING_REMOVE 5U
#define VFS_VIRTUAL_PENDING_RENAME 6U
#define VFS_VIRTUAL_PENDING_MKDIR 7U
#define VFS_VIRTUAL_PENDING_RMDIR 8U
#define VFS_VIRTUAL_PENDING_MOUNT_ADD 9U
#define VFS_VIRTUAL_PENDING_MOUNT_REMOVE 10U
/* I/O d’alias : chaque réponse privée est recodée vers le client avec le
 * request_id initial. En cas d’incertitude, aucune ne repasse par le miroir. */
#define VFS_VIRTUAL_PENDING_ALIAS_READ 11U
#define VFS_VIRTUAL_PENDING_ALIAS_STAT 12U
#define VFS_VIRTUAL_PENDING_ALIAS_LIST 13U
#define VFS_VIRTUAL_PENDING_ALIAS_LIST_PAGE 14U
#define VFS_VIRTUAL_PENDING_ALIAS_LIST_OBSERVE 15U
#define VFS_VIRTUAL_VIEW_INFO 0U
#define VFS_VIRTUAL_VIEW_STATS 1U
#define VFS_VIRTUAL_VIEW_MOUNTS 2U
#define VFS_VIRTUAL_PENDING_TURNS_MAX 8U

static int read_virtual(const char* path, uint8_t* data, uint32_t* size);
static int list_virtual_mounts_page(uint32_t start, uint8_t* data, uint32_t data_max,
                                    uint32_t* size, uint32_t* count, uint32_t* next_start);
static int write_mounted_backend(const char* path, const uint8_t* data, uint32_t size);
static int remove_mounted_backend(const char* path);
static int rename_mounted_backend(const char* oldpath, const char* newpath);
static int list_path_matches_mount(const char* path, const char* mount,
                                   const char** relative_out);

typedef struct {
    uint32_t active;
    uint32_t kind;
    uint32_t view;
    int worker_pid;
    int client_pid;
    uint32_t request_id;
    uint32_t mount_index;
    uint32_t mount_start;
    uint32_t mount_written;
    uint32_t turns;
    char mutation_path[OS_VFS_PATH_MAX];
    char mutation_new_path[OS_VFS_PATH_MAX];
    uint32_t mutation_size;
    uint32_t mount_source;
    /* Droit backend présent avant une I/O de lecture d’alias. Il est restauré
     * ou révoqué dès la terminaison corrélée pour réduire la fenêtre READ. */
    uint32_t backend_rights_restore;
    uint32_t backend_sources_restore;
    uint32_t backend_rights_active;
    uint8_t mutation_data[OS_VFS_WRITE_MAX];
    uint8_t mount_data[OS_VFS_READ_MAX];
} vfs_virtual_pending_t;
static vfs_virtual_pending_t vfs_virtual_pending;

static void vfs_virtual_reset(void) {
    if (vfs_virtual_pending.active && vfs_virtual_pending.backend_rights_active &&
        vfs_virtual_pending.backend_rights_restore != 0U &&
        service_lookup("vfs-virtual") == vfs_virtual_pending.worker_pid) {
        (void)service_backend_grant_scoped_source("vfs", vfs_virtual_pending.worker_pid,
                                                  vfs_virtual_pending.backend_rights_restore,
                                                  vfs_virtual_pending.backend_sources_restore);
    } else if (vfs_virtual_pending.active && vfs_virtual_pending.backend_rights_active &&
               vfs_virtual_pending.backend_rights_restore == 0U &&
               service_lookup("vfs-virtual") == vfs_virtual_pending.worker_pid) {
        (void)service_backend_revoke("vfs", vfs_virtual_pending.worker_pid);
    }
    vfs_virtual_pending.active = 0U;
    vfs_virtual_pending.kind = VFS_VIRTUAL_PENDING_SIMPLE;
    vfs_virtual_pending.view = VFS_VIRTUAL_VIEW_INFO;
    vfs_virtual_pending.worker_pid = -1;
    vfs_virtual_pending.client_pid = -1;
    vfs_virtual_pending.request_id = 0U;
    vfs_virtual_pending.mount_index = 0U;
    vfs_virtual_pending.mount_start = 0U;
    vfs_virtual_pending.mount_written = 0U;
    vfs_virtual_pending.turns = 0U;
    vfs_virtual_pending.mutation_path[0] = '\0';
    vfs_virtual_pending.mutation_new_path[0] = '\0';
    vfs_virtual_pending.mutation_size = 0U;
    vfs_virtual_pending.mount_source = 0U;
    vfs_virtual_pending.backend_rights_restore = 0U;
    vfs_virtual_pending.backend_sources_restore = 0U;
    vfs_virtual_pending.backend_rights_active = 0U;
}

static int vfs_virtual_lookup(void) {
    int worker_pid = service_lookup("vfs-virtual");
    worker_pid = worker_pid > 0 ? worker_pid : -1;
    if (worker_pid != vfs_mount_worker_pid) {
        vfs_mount_reset_dynamic();
        vfs_mount_worker_pid = worker_pid;
        vfs_mount_worker_trusted = 1U;
    }
    return worker_pid;
}

static int vfs_virtual_begin(int worker_pid, int client_pid, uint32_t request_id, uint32_t kind,
                             uint32_t view) {
    if (vfs_virtual_pending.active) return -1;
    vfs_virtual_pending.active = 1U;
    vfs_virtual_pending.kind = kind;
    vfs_virtual_pending.view = view;
    vfs_virtual_pending.worker_pid = worker_pid;
    vfs_virtual_pending.client_pid = client_pid;
    vfs_virtual_pending.request_id = request_id;
    vfs_virtual_pending.mount_index = 0U;
    vfs_virtual_pending.mount_start = 0U;
    vfs_virtual_pending.mount_written = 0U;
    vfs_virtual_pending.turns = 0U;
    vfs_virtual_pending.mutation_path[0] = '\0';
    vfs_virtual_pending.mutation_new_path[0] = '\0';
    vfs_virtual_pending.mutation_size = 0U;
    vfs_virtual_pending.mount_source = 0U;
    vfs_virtual_pending.backend_rights_restore = 0U;
    vfs_virtual_pending.backend_sources_restore = 0U;
    vfs_virtual_pending.backend_rights_active = 0U;
    return 0;
}

static int vfs_virtual_submit(const char* path, int client_pid, uint32_t request_id) {
    os_ipc_payload_t payload;
    int worker_pid;
    if (vfs_virtual_pending.active) return -1;
    worker_pid = vfs_virtual_lookup();
    if (worker_pid < 0) return -2;
    if (os_vfs_make_worker_read_request(&payload, path, request_id) != OS_VFS_STATUS_OK) return -3;
    if (ipc_send(worker_pid, &payload) != 0) return -4;
    return vfs_virtual_begin(worker_pid, client_pid, request_id, VFS_VIRTUAL_PENDING_SIMPLE,
                             VFS_VIRTUAL_VIEW_INFO);
}

/* Le miroir ne décide que si le chemin doit être délégué. Il ne résout jamais
 * l’I/O : le worker revalide le préfixe dans sa table d’autorité avant syscall. */
static int vfs_path_is_dynamic_alias(const char* path, int list_path) {
    uint32_t index;
    if (!path || !vfs_mount_worker_trusted) return 0;
    for (index = VFS_BOOT_MOUNT_COUNT; index < vfs_mount_count; index++) {
        const char* relative = (const char*)0;
        if (!list_path && os_vfs_match_mount(path, vfs_mounts[index].prefix, &relative)) return 1;
        if (list_path && os_vfs_list_path_is_valid(path)) {
            uint32_t offset = 0U;
            while (vfs_mounts[index].prefix[offset] != '\0' &&
                   path[offset] == vfs_mounts[index].prefix[offset]) offset++;
            if (vfs_mounts[index].prefix[offset] == '\0') return 1;
        }
    }
    return 0;
}

static uint32_t vfs_mount_source_for_path(const char* path, int list_path) {
    uint32_t index;
    if (!path) return 0U;
    for (index = 0U; index < vfs_mount_count; index++) {
        const char* relative = (const char*)0;
        if (!list_path && os_vfs_match_mount(path, vfs_mounts[index].prefix, &relative))
            return vfs_mounts[index].source;
        if (list_path && list_path_matches_mount(path, vfs_mounts[index].prefix, &relative))
            return vfs_mounts[index].source;
    }
    return 0U;
}

/* Le noyau ne voit que le suffixe relatif de la source. Pour un fichier ou une
 * mutation, le scope minimal est son parent ; pour un listage, il est le
 * répertoire observé. La racine est codée par la chaîne vide. */
static int vfs_relative_backend_prefix(const char* relative, int list_path, char* out) {
    uint32_t index = 0U;
    uint32_t parent_end = 0U;
    if (!relative || !out) return 0;
    if (list_path && relative[0] == '/' && relative[1] == '\0') {
        out[0] = '\0';
        return 1;
    }
    while (relative[index] != '\0') {
        if (index + 1U >= OS_SERVICE_BACKEND_PREFIX_MAX) return 0;
        if (relative[index] == '/') parent_end = index + 1U;
        index++;
    }
    if (list_path) parent_end = index;
    for (index = 0U; index < parent_end; index++) out[index] = relative[index];
    out[parent_end] = '\0';
    return 1;
}

static int vfs_backend_prefix_for_path(const char* path, int list_path, char* out) {
    uint32_t index;
    if (!path || !out) return 0;
    for (index = 0U; index < vfs_mount_count; index++) {
        const char* relative = (const char*)0;
        if (!list_path && os_vfs_match_mount(path, vfs_mounts[index].prefix, &relative))
            return vfs_relative_backend_prefix(relative, 0, out);
        if (list_path && list_path_matches_mount(path, vfs_mounts[index].prefix, &relative))
            return vfs_relative_backend_prefix(relative, 1, out);
    }
    return 0;
}

/* Les transactions worker utilisent une unique source. Une capacité déjà
 * présente est refusée plutôt que fusionnée : son union pourrait élargir les
 * droits de lecture ou mutation au-delà de l’alias résolu. */
static int vfs_virtual_prepare_worker_source(int worker_pid, uint32_t rights, uint32_t source,
                                             const char* prefix) {
    os_service_backend_scope_t scope;
    uint32_t sources = vfs_backend_source_mask(source);
    int status;
    if (sources == 0U || !prefix) return -1;
    status = service_backend_scope_status("vfs", worker_pid, &scope);
    if (status == 0 && scope.rights != 0U) return -2;
    if (status != 0 && status != OS_SERVICE_NOT_FOUND) return -3;
    status = service_backend_grant_scoped_source_prefix("vfs", worker_pid, rights, sources, prefix);
    if (status != 0) return -4;
    vfs_virtual_pending.backend_rights_restore = scope.rights;
    vfs_virtual_pending.backend_sources_restore = scope.sources;
    vfs_virtual_pending.backend_rights_active = 1U;
    return 0;
}

static int vfs_virtual_submit_alias_io(uint32_t kind, const char* path, uint32_t start,
                                       int client_pid, uint32_t request_id) {
    os_ipc_payload_t payload;
    char backend_prefix[OS_SERVICE_BACKEND_PREFIX_MAX];
    uint32_t source;
    int worker_pid;
    int status;
    if (vfs_virtual_pending.active) return -1;
    worker_pid = vfs_virtual_lookup();
    if (worker_pid < 0 || !vfs_path_is_dynamic_alias(path,
        kind == VFS_VIRTUAL_PENDING_ALIAS_LIST || kind == VFS_VIRTUAL_PENDING_ALIAS_LIST_PAGE ||
        kind == VFS_VIRTUAL_PENDING_ALIAS_LIST_OBSERVE)) return -2;
    if (kind == VFS_VIRTUAL_PENDING_ALIAS_READ) {
        status = os_vfs_make_worker_read_request(&payload, path, request_id);
    } else if (kind == VFS_VIRTUAL_PENDING_ALIAS_STAT) {
        status = os_vfs_make_worker_stat_request(&payload, path, request_id);
    } else if (kind == VFS_VIRTUAL_PENDING_ALIAS_LIST) {
        status = os_vfs_make_worker_list_request(&payload, path, request_id);
    } else if (kind == VFS_VIRTUAL_PENDING_ALIAS_LIST_PAGE ||
               kind == VFS_VIRTUAL_PENDING_ALIAS_LIST_OBSERVE) {
        status = os_vfs_make_worker_list_page_request(&payload, path, start, request_id);
    } else return -3;
    if (status != OS_VFS_STATUS_OK) return -4;
    if (vfs_virtual_begin(worker_pid, client_pid, request_id, kind, VFS_VIRTUAL_VIEW_INFO) != 0)
        return -5;
    source = vfs_mount_source_for_path(path, kind == VFS_VIRTUAL_PENDING_ALIAS_LIST ||
                                       kind == VFS_VIRTUAL_PENDING_ALIAS_LIST_PAGE ||
                                       kind == VFS_VIRTUAL_PENDING_ALIAS_LIST_OBSERVE);
    if (!vfs_backend_prefix_for_path(path, kind == VFS_VIRTUAL_PENDING_ALIAS_LIST ||
                                     kind == VFS_VIRTUAL_PENDING_ALIAS_LIST_PAGE ||
                                     kind == VFS_VIRTUAL_PENDING_ALIAS_LIST_OBSERVE, backend_prefix)) {
        vfs_virtual_reset();
        return -6;
    }
    status = vfs_virtual_prepare_worker_source(worker_pid, OS_VFS_BACKEND_RIGHT_READ, source,
                                               backend_prefix);
    if (status != 0) { vfs_virtual_reset(); return -7; }
    vfs_virtual_pending.mount_start = start;
    if (ipc_send(worker_pid, &payload) != 0) { vfs_virtual_reset(); return -7; }
    return 0;
}

static int vfs_virtual_mutation_path_is_fixed(const char* path) {
    return path && (string_has_prefix(path, "overlay/") ||
                    string_has_prefix(path, "fat16/") ||
                    string_has_prefix(path, "fat32/"));
}

static int vfs_virtual_mutation_path_is_routed(const char* path) {
    return vfs_virtual_mutation_path_is_fixed(path) || vfs_path_is_dynamic_alias(path, 0);
}

static void vfs_virtual_store_mutation(uint32_t kind, const char* path, const char* new_path,
                                       const uint8_t* data, uint32_t size) {
    uint32_t i;
    vfs_virtual_pending.kind = kind;
    for (i = 0U; i < OS_VFS_PATH_MAX; i++) {
        vfs_virtual_pending.mutation_path[i] = path[i];
        vfs_virtual_pending.mutation_new_path[i] = new_path ? new_path[i] : '\0';
    }
    vfs_virtual_pending.mutation_size = size;
    for (i = 0U; i < size; i++) vfs_virtual_pending.mutation_data[i] = data[i];
}

static int vfs_virtual_submit_mutation(uint32_t kind, const char* path, const char* new_path,
                                       const uint8_t* data, uint32_t size, int client_pid,
                                       uint32_t request_id) {
    os_ipc_payload_t payload;
    char backend_prefix[OS_SERVICE_BACKEND_PREFIX_MAX];
    uint32_t source;
    int worker_pid;
    int status;
    if (vfs_virtual_pending.active) return -1;
    worker_pid = vfs_virtual_lookup();
    if (worker_pid < 0) return -2;
    source = vfs_mount_source_for_path(path, 0);
    if (source == 0U || (new_path && vfs_mount_source_for_path(new_path, 0) != source)) return -3;
    if (!vfs_backend_prefix_for_path(path, 0, backend_prefix)) return -4;
    if (kind == VFS_VIRTUAL_PENDING_WRITE) {
        status = os_vfs_make_worker_write_request(&payload, path, data, size, request_id);
    } else if (kind == VFS_VIRTUAL_PENDING_REMOVE) {
        status = os_vfs_make_worker_remove_request(&payload, path, request_id);
    } else if (kind == VFS_VIRTUAL_PENDING_RENAME) {
        status = os_vfs_make_worker_rename_request(&payload, path, new_path, request_id);
    } else if (kind == VFS_VIRTUAL_PENDING_MKDIR) {
        status = os_vfs_make_worker_directory_request(&payload, OS_IPC_VFS_WORKER_MKDIR,
                                                      path, request_id);
    } else if (kind == VFS_VIRTUAL_PENDING_RMDIR) {
        status = os_vfs_make_worker_directory_request(&payload, OS_IPC_VFS_WORKER_RMDIR,
                                                      path, request_id);
    } else return -5;
    if (status != OS_VFS_STATUS_OK) return -6;
    if (vfs_virtual_begin(worker_pid, client_pid, request_id, kind, VFS_VIRTUAL_VIEW_INFO) != 0)
        return -7;
    status = vfs_virtual_prepare_worker_source(worker_pid, OS_VFS_BACKEND_RIGHT_MUTATE, source,
                                               backend_prefix);
    if (status != 0) { vfs_virtual_reset(); return -8; }
    vfs_virtual_store_mutation(kind, path, new_path, data, size);
    if (ipc_send(worker_pid, &payload) != 0) {
        vfs_virtual_reset();
        return -9;
    }
    return 0;
}

static int vfs_virtual_submit_mount_mutation(uint32_t kind, const char* prefix, uint32_t source,
                                             int client_pid, uint32_t request_id) {
    os_ipc_payload_t payload;
    int worker_pid;
    int status;
    if (vfs_virtual_pending.active) return -1;
    worker_pid = vfs_virtual_lookup();
    if (worker_pid < 0 || !vfs_mount_worker_trusted) return -2;
    if (kind == VFS_VIRTUAL_PENDING_MOUNT_ADD) {
        status = os_vfs_make_worker_mount_add_request(&payload, prefix, source, request_id);
    } else if (kind == VFS_VIRTUAL_PENDING_MOUNT_REMOVE) {
        status = os_vfs_make_worker_mount_remove_request(&payload, prefix, request_id);
    } else return -3;
    if (status != OS_VFS_STATUS_OK) return -4;
    if (vfs_virtual_begin(worker_pid, client_pid, request_id, kind, VFS_VIRTUAL_VIEW_MOUNTS) != 0)
        return -5;
    vfs_virtual_pending.mount_source = source;
    for (uint32_t i = 0U; i < OS_VFS_PATH_MAX; i++) vfs_virtual_pending.mutation_path[i] = prefix[i];
    if (ipc_send(worker_pid, &payload) != 0) {
        vfs_virtual_reset();
        return -6;
    }
    return 0;
}

static int vfs_virtual_submit_stats(int client_pid, uint32_t request_id) {
    os_ipc_payload_t payload;
    int worker_pid;
    if (vfs_virtual_pending.active) return -1;
    worker_pid = vfs_virtual_lookup();
    if (worker_pid < 0) return -2;
    if (os_vfs_make_worker_stats_request(&payload, vfs_read_requests, vfs_write_requests,
                                         vfs_remove_requests, vfs_rename_requests, request_id)
        != OS_VFS_STATUS_OK) return -3;
    if (ipc_send(worker_pid, &payload) != 0) return -4;
    return vfs_virtual_begin(worker_pid, client_pid, request_id, VFS_VIRTUAL_PENDING_SIMPLE,
                             VFS_VIRTUAL_VIEW_STATS);
}

static int vfs_virtual_submit_mounts(int client_pid, uint32_t request_id) {
    os_ipc_payload_t payload;
    int worker_pid;
    if (vfs_virtual_pending.active) return -1;
    worker_pid = vfs_virtual_lookup();
    if (worker_pid < 0) return -2;
    if (vfs_mount_count == 0U) return -3;
    if (os_vfs_make_worker_mount_request(&payload, vfs_mounts[0].prefix,
                                         vfs_mounts[0].source == OS_VFS_MOUNT_SOURCE_OVERLAY,
                                         request_id) != OS_VFS_STATUS_OK) return -4;
    if (ipc_send(worker_pid, &payload) != 0) return -5;
    return vfs_virtual_begin(worker_pid, client_pid, request_id, VFS_VIRTUAL_PENDING_MOUNTS,
                             VFS_VIRTUAL_VIEW_MOUNTS);
}

static int vfs_virtual_submit_mount_page(uint32_t start, int client_pid, uint32_t request_id) {
    os_ipc_payload_t payload;
    int worker_pid;
    if (vfs_virtual_pending.active) return -1;
    if (start >= vfs_mount_count) return -2;
    worker_pid = vfs_virtual_lookup();
    if (worker_pid < 0) return -3;
    if (os_vfs_make_worker_mount_request(&payload, vfs_mounts[start].prefix,
                                         vfs_mounts[start].source == OS_VFS_MOUNT_SOURCE_OVERLAY,
                                         request_id) != OS_VFS_STATUS_OK) return -4;
    if (ipc_send(worker_pid, &payload) != 0) return -5;
    if (vfs_virtual_begin(worker_pid, client_pid, request_id, VFS_VIRTUAL_PENDING_MOUNT_PAGE,
                          VFS_VIRTUAL_VIEW_MOUNTS) != 0) return -6;
    vfs_virtual_pending.mount_start = start;
    vfs_virtual_pending.mount_index = start;
    return 0;
}

static int vfs_virtual_submit_mount_observe(uint32_t start, int client_pid, uint32_t request_id) {
    os_ipc_payload_t payload;
    int worker_pid;
    if (vfs_virtual_pending.active) return -1;
    if (start >= vfs_mount_count) return -2;
    worker_pid = vfs_virtual_lookup();
    if (worker_pid < 0) return -3;
    if (os_vfs_make_worker_mount_request(&payload, vfs_mounts[start].prefix,
                                         vfs_mounts[start].source == OS_VFS_MOUNT_SOURCE_OVERLAY,
                                         request_id) != OS_VFS_STATUS_OK) return -4;
    if (ipc_send(worker_pid, &payload) != 0) return -5;
    if (vfs_virtual_begin(worker_pid, client_pid, request_id, VFS_VIRTUAL_PENDING_MOUNT_OBSERVE,
                          VFS_VIRTUAL_VIEW_MOUNTS) != 0) return -6;
    vfs_virtual_pending.mount_start = start;
    vfs_virtual_pending.mount_index = start;
    return 0;
}

static int vfs_virtual_reply_local(os_ipc_payload_t* reply_payload) {
    static const char info_path[] = "vfs-info";
    static const char stats_path[] = "vfs-stats";
    static const char mounts_path[] = "vfs-mounts";
    uint8_t data[OS_VFS_READ_MAX];
    uint32_t size = 0U;
    const char* path = info_path;
    if (!reply_payload || !vfs_virtual_pending.active) return 0;
    if (vfs_virtual_pending.kind == VFS_VIRTUAL_PENDING_WRITE ||
        vfs_virtual_pending.kind == VFS_VIRTUAL_PENDING_REMOVE ||
        vfs_virtual_pending.kind == VFS_VIRTUAL_PENDING_RENAME ||
        vfs_virtual_pending.kind == VFS_VIRTUAL_PENDING_MKDIR ||
        vfs_virtual_pending.kind == VFS_VIRTUAL_PENDING_RMDIR ||
        vfs_virtual_pending.kind == VFS_VIRTUAL_PENDING_MOUNT_ADD ||
        vfs_virtual_pending.kind == VFS_VIRTUAL_PENDING_MOUNT_REMOVE) {
        /* Une mutation a déjà été envoyée au worker. Après disparition ou délai,
         * son effet est inconnu : ne jamais la rejouer localement, sous peine de
         * doubler une écriture, création, suppression, renommage, dossier ou
         * mise à jour d’alias. Le client reçoit un
         * échec borné et peut vérifier l’état avant toute nouvelle requête. */
        if (vfs_virtual_pending.kind == VFS_VIRTUAL_PENDING_WRITE) {
            if (os_vfs_make_write_reply(reply_payload, OS_VFS_STATUS_INVALID,
                                        vfs_virtual_pending.request_id) == OS_VFS_STATUS_OK)
                (void)ipc_send(vfs_virtual_pending.client_pid, reply_payload);
        } else if (vfs_virtual_pending.kind == VFS_VIRTUAL_PENDING_REMOVE) {
            if (os_vfs_make_remove_reply(reply_payload, OS_VFS_STATUS_INVALID,
                                         vfs_virtual_pending.request_id) == OS_VFS_STATUS_OK)
                (void)ipc_send(vfs_virtual_pending.client_pid, reply_payload);
        } else if (vfs_virtual_pending.kind == VFS_VIRTUAL_PENDING_RENAME) {
            if (os_vfs_make_rename_reply(reply_payload, OS_VFS_STATUS_INVALID,
                                         vfs_virtual_pending.request_id) == OS_VFS_STATUS_OK)
                (void)ipc_send(vfs_virtual_pending.client_pid, reply_payload);
        } else if (vfs_virtual_pending.kind == VFS_VIRTUAL_PENDING_MKDIR) {
            if (os_vfs_make_mkdir_reply(reply_payload, OS_VFS_STATUS_INVALID,
                                        vfs_virtual_pending.request_id) == OS_VFS_STATUS_OK)
                (void)ipc_send(vfs_virtual_pending.client_pid, reply_payload);
        } else if (vfs_virtual_pending.kind == VFS_VIRTUAL_PENDING_RMDIR) {
            if (os_vfs_make_rmdir_reply(reply_payload, OS_VFS_STATUS_INVALID,
                                        vfs_virtual_pending.request_id) == OS_VFS_STATUS_OK) {
                (void)ipc_send(vfs_virtual_pending.client_pid, reply_payload);
            }
        } else {
            uint32_t reply_type = vfs_virtual_pending.kind == VFS_VIRTUAL_PENDING_MOUNT_ADD
                ? OS_IPC_VFS_MOUNT_ADD_REPLY : OS_IPC_VFS_MOUNT_REMOVE_REPLY;
            /* L’ajout ou retrait a déjà atteint le worker ; son état peut avoir
             * changé. Le miroir est donc purgé et la commande ne sera jamais
             * re-déposée localement ni auprès du même worker. */
            vfs_mount_mark_worker_unknown();
            if (os_vfs_make_mount_reply(reply_payload, reply_type, OS_VFS_STATUS_INVALID,
                                        vfs_virtual_pending.request_id) == OS_VFS_STATUS_OK) {
                (void)ipc_send(vfs_virtual_pending.client_pid, reply_payload);
            }
        }
        vfs_virtual_reset();
        return 1;
    }
    if (vfs_virtual_pending.kind == VFS_VIRTUAL_PENDING_ALIAS_READ) {
        if (os_vfs_make_read_reply(reply_payload, OS_VFS_STATUS_INVALID, (const uint8_t*)0, 0U,
                                   vfs_virtual_pending.request_id) == OS_VFS_STATUS_OK)
            (void)ipc_send(vfs_virtual_pending.client_pid, reply_payload);
        vfs_virtual_reset();
        return 1;
    }
    if (vfs_virtual_pending.kind == VFS_VIRTUAL_PENDING_ALIAS_STAT) {
        if (os_vfs_make_stat_reply(reply_payload, OS_VFS_STATUS_INVALID, 0U, 0U,
                                   vfs_virtual_pending.request_id) == OS_VFS_STATUS_OK)
            (void)ipc_send(vfs_virtual_pending.client_pid, reply_payload);
        vfs_virtual_reset();
        return 1;
    }
    if (vfs_virtual_pending.kind == VFS_VIRTUAL_PENDING_ALIAS_LIST) {
        if (os_vfs_make_list_reply(reply_payload, OS_VFS_STATUS_INVALID, 0U, (const uint8_t*)0, 0U,
                                   vfs_virtual_pending.request_id) == OS_VFS_STATUS_OK)
            (void)ipc_send(vfs_virtual_pending.client_pid, reply_payload);
        vfs_virtual_reset();
        return 1;
    }
    if (vfs_virtual_pending.kind == VFS_VIRTUAL_PENDING_ALIAS_LIST_PAGE) {
        if (os_vfs_make_list_page_reply(reply_payload, OS_VFS_STATUS_INVALID, 0U,
                                        OS_VFS_LIST_PAGE_END, (const uint8_t*)0, 0U,
                                        vfs_virtual_pending.request_id) == OS_VFS_STATUS_OK)
            (void)ipc_send(vfs_virtual_pending.client_pid, reply_payload);
        vfs_virtual_reset();
        return 1;
    }
    if (vfs_virtual_pending.kind == VFS_VIRTUAL_PENDING_ALIAS_LIST_OBSERVE) {
        if (os_vfs_make_list_observe_reply(reply_payload, OS_VFS_STATUS_INVALID, 0U,
                                           OS_VFS_LIST_PAGE_END, vfs_list_generation,
                                           (const uint8_t*)0, 0U,
                                           vfs_virtual_pending.request_id) == OS_VFS_STATUS_OK)
            (void)ipc_send(vfs_virtual_pending.client_pid, reply_payload);
        vfs_virtual_reset();
        return 1;
    }
    if (vfs_virtual_pending.kind == VFS_VIRTUAL_PENDING_MOUNT_PAGE) {
        uint32_t count = 0U;
        uint32_t next_start = OS_VFS_LIST_PAGE_END;
        int status = list_virtual_mounts_page(vfs_virtual_pending.mount_start, data,
                                              OS_VFS_LIST_PAGE_DATA_MAX, &size, &count,
                                              &next_start);
        if (os_vfs_make_list_page_reply(reply_payload, status, count, next_start, data, size,
                                        vfs_virtual_pending.request_id) == OS_VFS_STATUS_OK) {
            (void)ipc_send(vfs_virtual_pending.client_pid, reply_payload);
        }
        vfs_virtual_reset();
        return 1;
    }
    if (vfs_virtual_pending.kind == VFS_VIRTUAL_PENDING_MOUNT_OBSERVE) {
        uint32_t count = 0U;
        uint32_t next_start = OS_VFS_LIST_PAGE_END;
        int status = list_virtual_mounts_page(vfs_virtual_pending.mount_start, data,
                                              OS_VFS_LIST_OBSERVE_DATA_MAX, &size, &count,
                                              &next_start);
        if (os_vfs_make_list_observe_reply(reply_payload, status, count, next_start,
                                           vfs_list_generation, data, size,
                                           vfs_virtual_pending.request_id) == OS_VFS_STATUS_OK) {
            (void)ipc_send(vfs_virtual_pending.client_pid, reply_payload);
        }
        vfs_virtual_reset();
        return 1;
    }
    if (vfs_virtual_pending.view == VFS_VIRTUAL_VIEW_STATS) path = stats_path;
    else if (vfs_virtual_pending.view == VFS_VIRTUAL_VIEW_MOUNTS) path = mounts_path;
    if (read_virtual(path, data, &size) &&
        os_vfs_make_read_reply(reply_payload, OS_VFS_STATUS_OK, data, size,
                               vfs_virtual_pending.request_id) == OS_VFS_STATUS_OK) {
        (void)ipc_send(vfs_virtual_pending.client_pid, reply_payload);
    }
    vfs_virtual_reset();
    return 1;
}

/* Le registre est purgé à la terminaison. Dès que le PID publié n’est plus
 * celui de la transaction, le médiateur termine localement la réponse bornée. */
static int vfs_virtual_recover_if_worker_missing(os_ipc_payload_t* reply_payload) {
    if (!vfs_virtual_pending.active || vfs_virtual_lookup() == vfs_virtual_pending.worker_pid) return 0;
    if (vfs_virtual_reply_local(reply_payload)) {
        vfs_virtual_recoveries++;
        return 1;
    }
    return 0;
}

static int vfs_virtual_recover_if_timed_out(os_ipc_payload_t* reply_payload) {
    if (!vfs_virtual_pending.active ||
        vfs_virtual_pending.turns < VFS_VIRTUAL_PENDING_TURNS_MAX) return 0;
    if (vfs_virtual_reply_local(reply_payload)) {
        vfs_virtual_timeouts++;
        return 1;
    }
    return 0;
}

static void vfs_virtual_advance_turn(void) {
    if (vfs_virtual_pending.active &&
        vfs_virtual_pending.turns < VFS_VIRTUAL_PENDING_TURNS_MAX) {
        vfs_virtual_pending.turns++;
    }
}

static int vfs_virtual_complete(const os_ipc_message_t* message, os_ipc_payload_t* reply_payload) {
    uint8_t data[OS_VFS_READ_MAX];
    uint32_t size = 0U;
    int32_t status = OS_VFS_STATUS_INVALID;
    int parsed;
    if (!message || !reply_payload || !vfs_virtual_pending.active ||
        message->sender_pid != vfs_virtual_pending.worker_pid) return 0;
    if (vfs_virtual_pending.kind == VFS_VIRTUAL_PENDING_WRITE) {
        os_vfs_write_reply_t worker_reply;
        if (os_vfs_parse_write_reply(message, &worker_reply,
                                     vfs_virtual_pending.request_id) != OS_VFS_STATUS_OK) return 0;
        if (worker_reply.status == OS_VFS_STATUS_OK) vfs_list_generation++;
        if (os_vfs_make_write_reply(reply_payload, worker_reply.status,
                                    vfs_virtual_pending.request_id) == OS_VFS_STATUS_OK)
            (void)ipc_send(vfs_virtual_pending.client_pid, reply_payload);
        vfs_virtual_reset();
        return 1;
    }
    if (vfs_virtual_pending.kind == VFS_VIRTUAL_PENDING_REMOVE) {
        os_vfs_remove_reply_t worker_reply;
        if (os_vfs_parse_remove_reply(message, &worker_reply,
                                      vfs_virtual_pending.request_id) != OS_VFS_STATUS_OK) return 0;
        if (worker_reply.status == OS_VFS_STATUS_OK) vfs_list_generation++;
        if (os_vfs_make_remove_reply(reply_payload, worker_reply.status,
                                     vfs_virtual_pending.request_id) == OS_VFS_STATUS_OK)
            (void)ipc_send(vfs_virtual_pending.client_pid, reply_payload);
        vfs_virtual_reset();
        return 1;
    }
    if (vfs_virtual_pending.kind == VFS_VIRTUAL_PENDING_RENAME) {
        os_vfs_rename_reply_t worker_reply;
        if (os_vfs_parse_rename_reply(message, &worker_reply,
                                      vfs_virtual_pending.request_id) != OS_VFS_STATUS_OK) return 0;
        if (worker_reply.status == OS_VFS_STATUS_OK) vfs_list_generation++;
        if (worker_reply.status == OS_VFS_BACKEND_DENIED)
            puts("vfsserver backend prefix denied\n");
        if (os_vfs_make_rename_reply(reply_payload, worker_reply.status,
                                     vfs_virtual_pending.request_id) == OS_VFS_STATUS_OK)
            (void)ipc_send(vfs_virtual_pending.client_pid, reply_payload);
        vfs_virtual_reset();
        return 1;
    }
    if (vfs_virtual_pending.kind == VFS_VIRTUAL_PENDING_MKDIR) {
        int32_t status;
        if (os_vfs_parse_mkdir_reply(message, &status,
                                     vfs_virtual_pending.request_id) != OS_VFS_STATUS_OK) return 0;
        if (status == OS_VFS_STATUS_OK) vfs_list_generation++;
        if (os_vfs_make_mkdir_reply(reply_payload, status,
                                    vfs_virtual_pending.request_id) == OS_VFS_STATUS_OK)
            (void)ipc_send(vfs_virtual_pending.client_pid, reply_payload);
        vfs_virtual_reset();
        return 1;
    }
    if (vfs_virtual_pending.kind == VFS_VIRTUAL_PENDING_RMDIR) {
        int32_t status;
        if (os_vfs_parse_rmdir_reply(message, &status,
                                     vfs_virtual_pending.request_id) != OS_VFS_STATUS_OK) return 0;
        if (status == OS_VFS_STATUS_OK) vfs_list_generation++;
        if (os_vfs_make_rmdir_reply(reply_payload, status,
                                    vfs_virtual_pending.request_id) == OS_VFS_STATUS_OK)
            (void)ipc_send(vfs_virtual_pending.client_pid, reply_payload);
        vfs_virtual_reset();
        return 1;
    }
    if (vfs_virtual_pending.kind == VFS_VIRTUAL_PENDING_MOUNT_ADD ||
        vfs_virtual_pending.kind == VFS_VIRTUAL_PENDING_MOUNT_REMOVE) {
        uint32_t worker_reply_type = vfs_virtual_pending.kind == VFS_VIRTUAL_PENDING_MOUNT_ADD
            ? OS_IPC_VFS_WORKER_MOUNT_ADD_REPLY : OS_IPC_VFS_WORKER_MOUNT_REMOVE_REPLY;
        uint32_t client_reply_type = vfs_virtual_pending.kind == VFS_VIRTUAL_PENDING_MOUNT_ADD
            ? OS_IPC_VFS_MOUNT_ADD_REPLY : OS_IPC_VFS_MOUNT_REMOVE_REPLY;
        if (os_vfs_parse_worker_mount_reply(message, worker_reply_type, &status,
                                            vfs_virtual_pending.request_id) != OS_VFS_STATUS_OK) return 0;
        if (status == OS_VFS_STATUS_OK) {
            int mirrored = vfs_virtual_pending.kind == VFS_VIRTUAL_PENDING_MOUNT_ADD
                ? vfs_mount_add(vfs_virtual_pending.mutation_path, vfs_virtual_pending.mount_source)
                : vfs_mount_remove(vfs_virtual_pending.mutation_path);
            if (mirrored != OS_VFS_STATUS_OK) {
                /* Une divergence ne doit jamais autoriser un routage local
                 * potentiellement erroné ; attendre un nouveau worker. */
                vfs_mount_mark_worker_unknown();
                status = OS_VFS_STATUS_INVALID;
            } else {
                vfs_list_generation++;
            }
        }
        if (status == OS_VFS_STATUS_OK) {
            puts(vfs_virtual_pending.kind == VFS_VIRTUAL_PENDING_MOUNT_ADD
                 ? "vfsserver mount added " : "vfsserver mount removed ");
            puts(vfs_virtual_pending.mutation_path);
            if (vfs_virtual_pending.kind == VFS_VIRTUAL_PENDING_MOUNT_ADD) {
                puts(vfs_virtual_pending.mount_source == OS_VFS_MOUNT_SOURCE_OVERLAY ? " overlay\n"
                     : (vfs_virtual_pending.mount_source == OS_VFS_MOUNT_SOURCE_FAT16 ? " fat16\n"
                        : (vfs_virtual_pending.mount_source == OS_VFS_MOUNT_SOURCE_FAT32
                           ? " fat32\n" : " initrd\n")));
            } else {
                puts("\n");
            }
        } else {
            puts(vfs_virtual_pending.kind == VFS_VIRTUAL_PENDING_MOUNT_ADD
                 ? "vfsserver mount add rc " : "vfsserver mount remove rc ");
            print_int(status);
            puts("\n");
        }
        if (os_vfs_make_mount_reply(reply_payload, client_reply_type, status,
                                    vfs_virtual_pending.request_id) == OS_VFS_STATUS_OK) {
            (void)ipc_send(vfs_virtual_pending.client_pid, reply_payload);
        }
        vfs_virtual_reset();
        return 1;
    }
    if (vfs_virtual_pending.kind == VFS_VIRTUAL_PENDING_ALIAS_STAT) {
        os_vfs_stat_reply_t worker_reply;
        if (os_vfs_parse_worker_stat_reply(message, &worker_reply,
                                           vfs_virtual_pending.request_id) != OS_VFS_STATUS_OK) return 0;
        if (os_vfs_make_stat_reply(reply_payload, worker_reply.status, worker_reply.size,
                                   worker_reply.flags, vfs_virtual_pending.request_id)
            == OS_VFS_STATUS_OK) (void)ipc_send(vfs_virtual_pending.client_pid, reply_payload);
        vfs_virtual_reset();
        return 1;
    }
    if (vfs_virtual_pending.kind == VFS_VIRTUAL_PENDING_ALIAS_LIST) {
        os_vfs_list_reply_t worker_reply;
        if (os_vfs_parse_worker_list_reply(message, &worker_reply,
                                           vfs_virtual_pending.request_id) != OS_VFS_STATUS_OK) return 0;
        if (os_vfs_make_list_reply(reply_payload, worker_reply.status, worker_reply.count,
                                   worker_reply.status < 0 ? (const uint8_t*)0 : worker_reply.data,
                                   worker_reply.status < 0 ? 0U : OS_VFS_LIST_DATA_MAX,
                                   vfs_virtual_pending.request_id) == OS_VFS_STATUS_OK) {
            (void)ipc_send(vfs_virtual_pending.client_pid, reply_payload);
        }
        vfs_virtual_reset();
        return 1;
    }
    if (vfs_virtual_pending.kind == VFS_VIRTUAL_PENDING_ALIAS_LIST_PAGE ||
        vfs_virtual_pending.kind == VFS_VIRTUAL_PENDING_ALIAS_LIST_OBSERVE) {
        os_vfs_list_page_reply_t worker_reply;
        if (os_vfs_parse_worker_list_page_reply(message, &worker_reply,
                                                vfs_virtual_pending.request_id) != OS_VFS_STATUS_OK) return 0;
        if (vfs_virtual_pending.kind == VFS_VIRTUAL_PENDING_ALIAS_LIST_OBSERVE) {
            if (os_vfs_make_list_observe_reply(reply_payload, worker_reply.status, worker_reply.count,
                                               worker_reply.next_start, vfs_list_generation,
                                               worker_reply.status < 0 ? (const uint8_t*)0 : worker_reply.data,
                                               worker_reply.status < 0 ? 0U : OS_VFS_LIST_OBSERVE_DATA_MAX,
                                               vfs_virtual_pending.request_id) == OS_VFS_STATUS_OK) {
                (void)ipc_send(vfs_virtual_pending.client_pid, reply_payload);
            }
        } else if (os_vfs_make_list_page_reply(reply_payload, worker_reply.status, worker_reply.count,
                                                worker_reply.next_start,
                                                worker_reply.status < 0 ? (const uint8_t*)0 : worker_reply.data,
                                                worker_reply.status < 0 ? 0U : OS_VFS_LIST_PAGE_DATA_MAX,
                                                vfs_virtual_pending.request_id) == OS_VFS_STATUS_OK) {
            (void)ipc_send(vfs_virtual_pending.client_pid, reply_payload);
        }
        vfs_virtual_reset();
        return 1;
    }
    if (message->type != OS_IPC_VFS_WORKER_READ_REPLY) return 0;
    parsed = os_vfs_parse_worker_read_reply(message, &status, data, &size,
                                            vfs_virtual_pending.request_id);
    /* Un worker repris peut vider après coup une requête expirée. Le PID et le
     * type sont valides, mais un request_id discordant ne doit jamais terminer
     * la nouvelle transaction : il est simplement écarté. */
    if (parsed != OS_VFS_STATUS_OK) return 0;
    if (vfs_virtual_pending.kind == VFS_VIRTUAL_PENDING_ALIAS_READ) {
        if (os_vfs_make_read_reply(reply_payload, status,
                                   status < 0 ? (const uint8_t*)0 : data,
                                   status < 0 ? 0U : size,
                                   vfs_virtual_pending.request_id) == OS_VFS_STATUS_OK) {
            (void)ipc_send(vfs_virtual_pending.client_pid, reply_payload);
        }
        vfs_virtual_reset();
        return 1;
    }
    if (vfs_virtual_pending.kind == VFS_VIRTUAL_PENDING_MOUNT_PAGE ||
        vfs_virtual_pending.kind == VFS_VIRTUAL_PENDING_MOUNT_OBSERVE) {
        os_ipc_payload_t payload;
        uint32_t count;
        uint32_t next_start;
        uint32_t data_max = vfs_virtual_pending.kind == VFS_VIRTUAL_PENDING_MOUNT_OBSERVE
            ? OS_VFS_LIST_OBSERVE_DATA_MAX : OS_VFS_LIST_PAGE_DATA_MAX;
        int32_t page_status;
        uint32_t i;
        if (status != OS_VFS_STATUS_OK ||
            size > data_max - vfs_virtual_pending.mount_written) {
            return vfs_virtual_reply_local(reply_payload);
        }
        for (i = 0U; i < size; i++) {
            vfs_virtual_pending.mount_data[vfs_virtual_pending.mount_written + i] = data[i];
        }
        vfs_virtual_pending.mount_written += size;
        vfs_virtual_pending.mount_index++;
        count = vfs_virtual_pending.mount_index - vfs_virtual_pending.mount_start;
        if (vfs_virtual_pending.mount_index < vfs_mount_count && count < OS_VFS_LIST_ENTRY_MAX) {
            if (os_vfs_make_worker_mount_request(&payload,
                                                  vfs_mounts[vfs_virtual_pending.mount_index].prefix,
                                                  vfs_mounts[vfs_virtual_pending.mount_index].source == OS_VFS_MOUNT_SOURCE_OVERLAY,
                                                  vfs_virtual_pending.request_id) != OS_VFS_STATUS_OK ||
                ipc_send(vfs_virtual_pending.worker_pid, &payload) != 0) {
                return vfs_virtual_reply_local(reply_payload);
            }
            return 1;
        }
        next_start = vfs_virtual_pending.mount_index < vfs_mount_count
            ? vfs_virtual_pending.mount_index : OS_VFS_LIST_PAGE_END;
        page_status = next_start == OS_VFS_LIST_PAGE_END
            ? OS_VFS_STATUS_OK : OS_VFS_STATUS_TRUNCATED;
        if (vfs_virtual_pending.kind == VFS_VIRTUAL_PENDING_MOUNT_OBSERVE) {
            if (os_vfs_make_list_observe_reply(reply_payload, page_status, count, next_start,
                                               vfs_list_generation, vfs_virtual_pending.mount_data,
                                               vfs_virtual_pending.mount_written,
                                               vfs_virtual_pending.request_id) == OS_VFS_STATUS_OK) {
                (void)ipc_send(vfs_virtual_pending.client_pid, reply_payload);
            }
        } else if (os_vfs_make_list_page_reply(reply_payload, page_status, count, next_start,
                                               vfs_virtual_pending.mount_data,
                                               vfs_virtual_pending.mount_written,
                                               vfs_virtual_pending.request_id) == OS_VFS_STATUS_OK) {
            (void)ipc_send(vfs_virtual_pending.client_pid, reply_payload);
        }
        vfs_virtual_reset();
        return 1;
    }
    if (vfs_virtual_pending.kind == VFS_VIRTUAL_PENDING_MOUNTS) {
        os_ipc_payload_t payload;
        uint32_t i;
        if (status != OS_VFS_STATUS_OK) {
            if (os_vfs_make_read_reply(reply_payload, OS_VFS_STATUS_INVALID, data, 0U,
                                       vfs_virtual_pending.request_id) == OS_VFS_STATUS_OK) {
                (void)ipc_send(vfs_virtual_pending.client_pid, reply_payload);
            }
            vfs_virtual_reset();
            return 1;
        }
        if (size > OS_VFS_READ_MAX - vfs_virtual_pending.mount_written) {
            if (os_vfs_make_read_reply(reply_payload, OS_VFS_STATUS_OK, vfs_virtual_pending.mount_data,
                                       vfs_virtual_pending.mount_written,
                                       vfs_virtual_pending.request_id) == OS_VFS_STATUS_OK) {
                (void)ipc_send(vfs_virtual_pending.client_pid, reply_payload);
            }
            vfs_virtual_reset();
            return 1;
        }
        for (i = 0U; i < size; i++) {
            vfs_virtual_pending.mount_data[vfs_virtual_pending.mount_written + i] = data[i];
        }
        vfs_virtual_pending.mount_written += size;
        vfs_virtual_pending.mount_index++;
        if (vfs_virtual_pending.mount_index < vfs_mount_count) {
            if (os_vfs_make_worker_mount_request(&payload,
                                                  vfs_mounts[vfs_virtual_pending.mount_index].prefix,
                                                  vfs_mounts[vfs_virtual_pending.mount_index].source == OS_VFS_MOUNT_SOURCE_OVERLAY,
                                                  vfs_virtual_pending.request_id) != OS_VFS_STATUS_OK ||
                ipc_send(vfs_virtual_pending.worker_pid, &payload) != 0) {
                return vfs_virtual_reply_local(reply_payload);
            }
            return 1;
        }
        if (os_vfs_make_read_reply(reply_payload, OS_VFS_STATUS_OK, vfs_virtual_pending.mount_data,
                                   vfs_virtual_pending.mount_written,
                                   vfs_virtual_pending.request_id) == OS_VFS_STATUS_OK) {
            (void)ipc_send(vfs_virtual_pending.client_pid, reply_payload);
        }
        vfs_virtual_reset();
        return 1;
    }
    if (os_vfs_make_read_reply(reply_payload, status, data, size,
                               vfs_virtual_pending.request_id) == OS_VFS_STATUS_OK) {
        (void)ipc_send(vfs_virtual_pending.client_pid, reply_payload);
    }
    vfs_virtual_reset();
    return 1;
}

static uint32_t append_text(uint8_t* data, uint32_t offset, const char* text) {
    uint32_t i = 0U;
    while (text[i] != '\0') data[offset++] = (uint8_t)text[i++];
    return offset;
}

static uint32_t append_uint(uint8_t* data, uint32_t offset, uint32_t value) {
    char digits[10];
    uint32_t count = 0U;
    if (value == 0U) {
        data[offset++] = (uint8_t)'0';
        return offset;
    }
    while (value > 0U) {
        digits[count++] = (char)('0' + (value % 10U));
        value /= 10U;
    }
    while (count > 0U) data[offset++] = (uint8_t)digits[--count];
    return offset;
}

static int read_virtual(const char* path, uint8_t* data, uint32_t* size) {
    static const char info[] = "vfsserver ring3 policy\n";
    const char* source = 0;
    uint32_t i;
    if (string_equal(path, "vfs-info")) source = info;
    else if (string_equal(path, "vfs-mounts")) {
        i = 0U;
        for (uint32_t mount_index = 0U; mount_index < vfs_mount_count; mount_index++) {
            uint32_t prefix_size = 0U;
            while (vfs_mounts[mount_index].prefix[prefix_size] != '\0') prefix_size++;
            /* suffixe fixe : espace, droits et saut de ligne. */
            if (i + prefix_size + 4U > OS_VFS_READ_MAX) break;
            i = append_text(data, i, vfs_mounts[mount_index].prefix);
            i = append_text(data, i, vfs_mounts[mount_index].source == OS_VFS_MOUNT_SOURCE_OVERLAY
                              ? " rw\n" : " ro\n");
        }
        *size = i;
        return 1;
    } else if (string_equal(path, "vfs-stats")) {
        i = append_text(data, 0U, "reads=");
        i = append_uint(data, i, vfs_read_requests);
        i = append_text(data, i, "\nwrites=");
        i = append_uint(data, i, vfs_write_requests);
        i = append_text(data, i, "\nremoves=");
        i = append_uint(data, i, vfs_remove_requests);
        i = append_text(data, i, "\nrenames=");
        i = append_uint(data, i, vfs_rename_requests);
        data[i++] = (uint8_t)'\n';
        *size = i;
        return 1;
    } else if (string_equal(path, "vfs-worker")) {
        int worker_pid = vfs_virtual_lookup();
        i = append_text(data, 0U, "vfsvirtual ");
        if (worker_pid > 0) {
            i = append_text(data, i, "ready pid=");
            i = append_uint(data, i, (uint32_t)worker_pid);
        } else {
            i = append_text(data, i, "missing");
        }
        i = append_text(data, i, " recoveries=");
        i = append_uint(data, i, vfs_virtual_recoveries);
        i = append_text(data, i, " timeouts=");
        i = append_uint(data, i, vfs_virtual_timeouts);
        data[i++] = (uint8_t)'\n';
        *size = i;
        return 1;
    } else return 0;
    for (i = 0U; source[i] != '\0'; i++) data[i] = (uint8_t)source[i];
    *size = i;
    return 1;
}

/* Le backend ne reçoit jamais un chemin global : uniquement le suffixe d’un
 * montage déclaré par ce médiateur. */
static int read_mounted_backend(const char* path, uint8_t* data, uint32_t* size) {
    uint32_t i;
    for (i = 0U; i < vfs_mount_count; i++) {
        const char* relative = 0;
        if (os_vfs_match_mount(path, vfs_mounts[i].prefix, &relative)) {
            const vfs_backend_ops_t* ops = vfs_backend_ops_for(vfs_mounts[i].source);
            int read;
            if (!ops || !ops->read) return OS_VFS_STATUS_NOT_MOUNTED;
            read = ops->read(relative, (char*)data, OS_VFS_READ_MAX);
            if (read < 0) return read;
            *size = (uint32_t)read;
            return OS_VFS_STATUS_OK;
        }
    }
    return OS_VFS_STATUS_NOT_MOUNTED;
}

/* Les mutations sont déléguées uniquement aux callbacks explicitement publiés par la source. */
static int stat_mounted_backend(const char* path, os_dirent_t* out) {
    uint32_t i;
    for (i = 0U; i < vfs_mount_count; i++) {
        const char* relative = 0;
        if (os_vfs_match_mount(path, vfs_mounts[i].prefix, &relative)) {
            const vfs_backend_ops_t* ops = vfs_backend_ops_for(vfs_mounts[i].source);
            return ops && ops->stat ? ops->stat(relative, out) : OS_VFS_STATUS_NOT_MOUNTED;
        }
    }
    return OS_VFS_STATUS_NOT_MOUNTED;
}

/* Le listage accepte la racine d’un montage ou un sous-répertoire qui lui
 * appartient. Chaque backend ne voit que son suffixe relatif ; la réponse
 * demeure une page texte bornée et la cinquième entrée détecte une page
 * incomplète. */
static int list_path_matches_mount(const char* path, const char* mount,
                                   const char** relative_out) {
    uint32_t i = 0U;
    if (!os_vfs_list_path_is_valid(path) || !os_vfs_mount_prefix_is_valid(mount)) return 0;
    while (mount[i] != '\0') {
        if (path[i] == '\0' || path[i] != mount[i]) return 0;
        i++;
    }
    if (path[i] == '\0') {
        if (relative_out) *relative_out = "/";
        return 1;
    }
    if (relative_out) *relative_out = path + i;
    return 1;
}

static int list_mounted_backend(const char* path, uint8_t* data, uint32_t* size,
                                uint32_t* count) {
    os_dirent_t entries[OS_VFS_LIST_ENTRY_MAX + 1U];
    uint32_t mount_index;
    uint32_t written = 0U;
    uint32_t emitted = 0U;
    int listed;
    int status = OS_VFS_STATUS_OK;
    if (!path || !data || !size || !count) return OS_VFS_STATUS_INVALID;
    for (mount_index = 0U; mount_index < vfs_mount_count; mount_index++) {
        const char* relative = 0;
        if (list_path_matches_mount(path, vfs_mounts[mount_index].prefix, &relative)) {
            const vfs_backend_ops_t* ops = vfs_backend_ops_for(vfs_mounts[mount_index].source);
            if (!ops || !ops->list) return OS_VFS_STATUS_NOT_MOUNTED;
            listed = ops->list(relative, entries, (int)(OS_VFS_LIST_ENTRY_MAX + 1U));
            if (listed < 0) return listed;
            for (uint32_t entry_index = 0U;
                 entry_index < (uint32_t)listed && entry_index < OS_VFS_LIST_ENTRY_MAX;
                 entry_index++) {
                uint32_t name_size = string_length(entries[entry_index].name);
                if (written + name_size + 1U > OS_VFS_LIST_DATA_MAX) {
                    status = OS_VFS_STATUS_TRUNCATED;
                    break;
                }
                for (uint32_t j = 0U; j < name_size; j++) {
                    data[written++] = (uint8_t)entries[entry_index].name[j];
                }
                data[written++] = (uint8_t)'\n';
                emitted++;
            }
            if ((uint32_t)listed > OS_VFS_LIST_ENTRY_MAX) status = OS_VFS_STATUS_TRUNCATED;
            *size = written;
            *count = emitted;
            return status;
        }
    }
    return OS_VFS_STATUS_NOT_MOUNTED;
}

static int list_virtual_mounts_page(uint32_t start, uint8_t* data, uint32_t data_max,
                                    uint32_t* size, uint32_t* count, uint32_t* next_start) {
    uint32_t index;
    uint32_t written = 0U;
    uint32_t emitted = 0U;
    if (!data || !size || !count || !next_start || start > vfs_mount_count) {
        return OS_VFS_STATUS_INVALID;
    }
    *next_start = OS_VFS_LIST_PAGE_END;
    for (index = start; index < vfs_mount_count && emitted < OS_VFS_LIST_ENTRY_MAX; index++) {
        uint32_t prefix_size = string_length(vfs_mounts[index].prefix);
        if (written + prefix_size + 4U > data_max) break;
        for (uint32_t j = 0U; j < prefix_size; j++) data[written++] = (uint8_t)vfs_mounts[index].prefix[j];
        data[written++] = (uint8_t)' ';
        data[written++] = (uint8_t)'r';
        data[written++] = (uint8_t)(vfs_mounts[index].source == OS_VFS_MOUNT_SOURCE_OVERLAY ? 'w' : 'o');
        data[written++] = (uint8_t)'\n';
        emitted++;
    }
    if (index < vfs_mount_count) {
        *next_start = index;
        *size = written;
        *count = emitted;
        return OS_VFS_STATUS_TRUNCATED;
    }
    *size = written;
    *count = emitted;
    return OS_VFS_STATUS_OK;
}

static int list_mounted_backend_page(const char* path, uint32_t start, uint8_t* data,
                                     uint32_t data_max, uint32_t* size, uint32_t* count,
                                     uint32_t* next_start) {
    os_dirent_t entries[OS_VFS_LIST_ENTRY_MAX + 1U];
    uint32_t mount_index;
    uint32_t written = 0U;
    uint32_t emitted = 0U;
    int listed;
    int status = OS_VFS_STATUS_OK;
    if (!path || !data || data_max == 0U || !size || !count || !next_start) return OS_VFS_STATUS_INVALID;
    *next_start = OS_VFS_LIST_PAGE_END;
    for (mount_index = 0U; mount_index < vfs_mount_count; mount_index++) {
        const char* relative = 0;
        if (!list_path_matches_mount(path, vfs_mounts[mount_index].prefix, &relative)) continue;
        {
            const vfs_backend_ops_t* ops = vfs_backend_ops_for(vfs_mounts[mount_index].source);
            if (!ops || !ops->list_page) return OS_VFS_STATUS_NOT_MOUNTED;
            listed = ops->list_page(relative, entries, start);
        }
        if (listed < 0) return listed;
        for (uint32_t entry_index = 0U;
             entry_index < (uint32_t)listed && entry_index < OS_VFS_LIST_ENTRY_MAX;
             entry_index++) {
            uint32_t name_size = string_length(entries[entry_index].name);
            if (written + name_size + 1U > data_max) {
                status = OS_VFS_STATUS_TRUNCATED;
                break;
            }
            for (uint32_t j = 0U; j < name_size; j++) data[written++] = (uint8_t)entries[entry_index].name[j];
            data[written++] = (uint8_t)'\n';
            emitted++;
        }
        if ((uint32_t)listed > emitted) {
            status = OS_VFS_STATUS_TRUNCATED;
            *next_start = start + (emitted == 0U ? 1U : emitted);
        }
        *size = written;
        *count = emitted;
        return status;
    }
    return OS_VFS_STATUS_NOT_MOUNTED;
}

static int write_mounted_backend(const char* path, const uint8_t* data, uint32_t size) {
    uint32_t i;
    for (i = 0U; i < vfs_mount_count; i++) {
        const char* relative = 0;
        if (os_vfs_match_mount(path, vfs_mounts[i].prefix, &relative)) {
            const vfs_backend_ops_t* ops = vfs_backend_ops_for(vfs_mounts[i].source);
            int written;
            if (!ops || !ops->write) return OS_VFS_STATUS_NOT_MOUNTED;
            written = ops->write(relative, data, size);
            return written < 0 ? written : OS_VFS_STATUS_OK;
        }
    }
    return OS_VFS_STATUS_NOT_MOUNTED;
}

static int mkdir_mounted_backend(const char* path) {
    uint32_t i;
    for (i = 0U; i < vfs_mount_count; i++) {
        const char* relative = 0;
        if (os_vfs_match_mount(path, vfs_mounts[i].prefix, &relative)) {
            const vfs_backend_ops_t* ops = vfs_backend_ops_for(vfs_mounts[i].source);
            return ops && ops->mkdir ? ops->mkdir(relative) : OS_VFS_STATUS_NOT_MOUNTED;
        }
    }
    return OS_VFS_STATUS_NOT_MOUNTED;
}

static int rmdir_mounted_backend(const char* path) {
    uint32_t i;
    for (i = 0U; i < vfs_mount_count; i++) {
        const char* relative = 0;
        if (os_vfs_match_mount(path, vfs_mounts[i].prefix, &relative)) {
            const vfs_backend_ops_t* ops = vfs_backend_ops_for(vfs_mounts[i].source);
            return ops && ops->rmdir ? ops->rmdir(relative) : OS_VFS_STATUS_NOT_MOUNTED;
        }
    }
    return OS_VFS_STATUS_NOT_MOUNTED;
}

static int remove_mounted_backend(const char* path) {
    uint32_t i;
    for (i = 0U; i < vfs_mount_count; i++) {
        const char* relative = 0;
        if (os_vfs_match_mount(path, vfs_mounts[i].prefix, &relative)) {
            const vfs_backend_ops_t* ops = vfs_backend_ops_for(vfs_mounts[i].source);
            return ops && ops->remove ? ops->remove(relative) : OS_VFS_STATUS_NOT_MOUNTED;
        }
    }
    return OS_VFS_STATUS_NOT_MOUNTED;
}

static int rename_mounted_backend(const char* oldpath, const char* newpath) {
    uint32_t i;
    for (i = 0U; i < vfs_mount_count; i++) {
        const char* old_relative = 0;
        const char* new_relative = 0;
        if (os_vfs_match_mount(oldpath, vfs_mounts[i].prefix, &old_relative) &&
            os_vfs_match_mount(newpath, vfs_mounts[i].prefix, &new_relative)) {
            const vfs_backend_ops_t* ops = vfs_backend_ops_for(vfs_mounts[i].source);
            return ops && ops->rename ? ops->rename(old_relative, new_relative)
                                      : OS_VFS_STATUS_NOT_MOUNTED;
        }
    }
    return OS_VFS_STATUS_NOT_MOUNTED;
}

void main(void) {
    os_ipc_message_t message;
    os_ipc_payload_t reply_payload;
    char path[OS_VFS_PATH_MAX];
    char new_path[OS_VFS_PATH_MAX];
    uint8_t data[OS_VFS_READ_MAX];
    uint8_t write_data[OS_VFS_WRITE_MAX];
    os_dirent_t metadata;
    if (service_register("vfs") != 0) {
        puts("vfsserver register failed\n");
        for (;;) yield();
    }
    puts("vfsserver ready vfs\n");
    puts("vfsserver mount initrd/ ro\n");
    puts("vfsserver mount overlay/ rw\n");
    for (;;) {
        int received = ipc_receive(&message);
        if (received == 0 && vfs_virtual_complete(&message, &reply_payload)) {
            yield();
            continue;
        }
        if (vfs_virtual_recover_if_worker_missing(&reply_payload)) {
            puts("vfsserver virtual worker fallback local\n");
        } else {
            vfs_virtual_advance_turn();
            if (vfs_virtual_recover_if_timed_out(&reply_payload)) {
                puts("vfsserver virtual worker timeout local\n");
            }
        }
        if (received == 0 && message.type == OS_IPC_VFS_LIST) {
            int status;
            uint32_t size = 0U;
            uint32_t count = 0U;
            puts("vfsserver list request\n");
            status = os_vfs_parse_list_request(&message, path);
            if (status == 0 && vfs_path_is_dynamic_alias(path, 1) &&
                vfs_virtual_submit_alias_io(VFS_VIRTUAL_PENDING_ALIAS_LIST, path, 0U,
                                            message.sender_pid, message.request_id) == 0) {
                puts("vfsserver delegated alias list\n");
                yield();
                continue;
            }
            if (status == 0) {
                status = list_mounted_backend(path, data, &size, &count);
                if (status == OS_VFS_STATUS_NOT_MOUNTED) puts("vfsserver list outside mounts\n");
            }
            if (os_vfs_make_list_reply(&reply_payload, status, count, data, size,
                                       message.request_id) == 0) {
                (void)ipc_send(message.sender_pid, &reply_payload);
            }
        } else if (received == 0 && message.type == OS_IPC_VFS_LIST_PAGE) {
            int status;
            uint32_t size = 0U;
            uint32_t count = 0U;
            uint32_t start = 0U;
            uint32_t next_start = OS_VFS_LIST_PAGE_END;
            puts("vfsserver list page request\n");
            status = os_vfs_parse_list_page_request(&message, path, &start);
            if (status == 0 && string_equal(path, "vfs-mounts")) {
                status = vfs_virtual_submit_mount_page(start, message.sender_pid, message.request_id);
                if (status == 0) {
                    puts("vfsserver delegated mount page\n");
                    yield();
                    continue;
                }
                status = list_virtual_mounts_page(start, data, OS_VFS_LIST_PAGE_DATA_MAX,
                                                  &size, &count, &next_start);
                puts("vfsserver virtual mount page local\n");
            } else if (status == 0 && vfs_path_is_dynamic_alias(path, 1) &&
                       vfs_virtual_submit_alias_io(VFS_VIRTUAL_PENDING_ALIAS_LIST_PAGE, path, start,
                                                   message.sender_pid, message.request_id) == 0) {
                puts("vfsserver delegated alias list page\n");
                yield();
                continue;
            } else if (status == 0) {
                status = list_mounted_backend_page(path, start, data, OS_VFS_LIST_PAGE_DATA_MAX,
                                                   &size, &count, &next_start);
                if (status == OS_VFS_STATUS_NOT_MOUNTED) puts("vfsserver list page outside mounts\n");
            }
            if (os_vfs_make_list_page_reply(&reply_payload, status, count, next_start,
                                            data, size, message.request_id) == 0) {
                (void)ipc_send(message.sender_pid, &reply_payload);
            }
        } else if (received == 0 && message.type == OS_IPC_VFS_LIST_OBSERVE) {
            int status;
            uint32_t size = 0U;
            uint32_t count = 0U;
            uint32_t start = 0U;
            uint32_t expected_generation = 0U;
            uint32_t next_start = OS_VFS_LIST_PAGE_END;
            puts("vfsserver list observe request\n");
            status = os_vfs_parse_list_observe_request(&message, path, &start, &expected_generation);
            if (status == 0 && expected_generation != 0U && expected_generation != vfs_list_generation) {
                status = OS_VFS_STATUS_STALE;
            }
            if (status == 0 && string_equal(path, "vfs-mounts")) {
                status = vfs_virtual_submit_mount_observe(start, message.sender_pid, message.request_id);
                if (status == 0) {
                    puts("vfsserver delegated mount observe\n");
                    yield();
                    continue;
                }
                status = list_virtual_mounts_page(start, data, OS_VFS_LIST_OBSERVE_DATA_MAX,
                                                  &size, &count, &next_start);
                puts("vfsserver virtual mount observe local\n");
            } else if (status == 0 && vfs_path_is_dynamic_alias(path, 1) &&
                       vfs_virtual_submit_alias_io(VFS_VIRTUAL_PENDING_ALIAS_LIST_OBSERVE, path, start,
                                                   message.sender_pid, message.request_id) == 0) {
                puts("vfsserver delegated alias list observe\n");
                yield();
                continue;
            } else if (status == 0) {
                status = list_mounted_backend_page(path, start, data, OS_VFS_LIST_OBSERVE_DATA_MAX,
                                                   &size, &count, &next_start);
                if (status == OS_VFS_STATUS_NOT_MOUNTED) puts("vfsserver list observe outside mounts\n");
            }
            if (os_vfs_make_list_observe_reply(&reply_payload, status, count, next_start,
                                               vfs_list_generation, data, size,
                                               message.request_id) == 0) {
                (void)ipc_send(message.sender_pid, &reply_payload);
            }
        } else if (received == 0 && message.type == OS_IPC_VFS_STAT) {
            int status;
            puts("vfsserver stat request\n");
            status = os_vfs_parse_stat_request(&message, path);
            if (status == 0 && vfs_path_is_dynamic_alias(path, 0) &&
                vfs_virtual_submit_alias_io(VFS_VIRTUAL_PENDING_ALIAS_STAT, path, 0U,
                                            message.sender_pid, message.request_id) == 0) {
                puts("vfsserver delegated alias stat\n");
                yield();
                continue;
            }
            if (status == 0) {
                status = stat_mounted_backend(path, &metadata);
                if (status == OS_VFS_STATUS_NOT_MOUNTED) {
                    puts("vfsserver stat outside mounts\n");
                }
            }
            if (os_vfs_make_stat_reply(&reply_payload, status,
                                       status == 0 ? metadata.size : 0U,
                                       status == 0 ? metadata.flags : 0U,
                                       message.request_id) == 0) {
                (void)ipc_send(message.sender_pid, &reply_payload);
            }
        } else if (received == 0 && message.type == OS_IPC_VFS_READ) {
            int status;
            vfs_read_requests++;
            puts("vfsserver read request\n");
            status = os_vfs_parse_read_request(&message, path);
            uint32_t size = 0U;
            if (status == 0 && string_equal(path, "vfs-info") &&
                vfs_virtual_submit(path, message.sender_pid, message.request_id) == 0) {
                puts("vfsserver delegated vfs-info\n");
                yield();
                continue;
            }
            if (status == 0 && string_equal(path, "vfs-stats") &&
                vfs_virtual_submit_stats(message.sender_pid, message.request_id) == 0) {
                puts("vfsserver delegated vfs-stats\n");
                yield();
                continue;
            }
            if (status == 0 && string_equal(path, "vfs-mounts") &&
                vfs_virtual_submit_mounts(message.sender_pid, message.request_id) == 0) {
                puts("vfsserver delegated vfs-mounts\n");
                yield();
                continue;
            }
            if (status == 0 && vfs_path_is_dynamic_alias(path, 0) &&
                vfs_virtual_submit_alias_io(VFS_VIRTUAL_PENDING_ALIAS_READ, path, 0U,
                                            message.sender_pid, message.request_id) == 0) {
                puts("vfsserver delegated alias read\n");
                yield();
                continue;
            }
            if (status == 0) {
                if (read_virtual(path, data, &size)) {
                    if (string_equal(path, "vfs-info")) puts("vfsserver virtual vfs-info local\n");
                    else if (string_equal(path, "vfs-mounts")) puts("vfsserver virtual vfs-mounts local\n");
                    else if (string_equal(path, "vfs-stats")) puts("vfsserver virtual vfs-stats local\n");
                    else puts("vfsserver virtual vfs-worker local\n");
                } else {
                    status = read_mounted_backend(path, data, &size);
                    if (status == OS_VFS_STATUS_NOT_MOUNTED) {
                        puts("vfsserver path outside mounts\n");
                    }
                }
            }
            if (os_vfs_make_read_reply(&reply_payload, status, data, size,
                                       message.request_id) == 0) {
                (void)ipc_send(message.sender_pid, &reply_payload);
            }
        } else if (received == 0 && message.type == OS_IPC_VFS_WRITE) {
            int status;
            uint32_t size = 0U;
            vfs_write_requests++;
            puts("vfsserver write request\n");
            status = os_vfs_parse_write_request(&message, path, write_data, &size);
            if (status == 0 && vfs_virtual_mutation_path_is_routed(path) &&
                vfs_virtual_submit_mutation(VFS_VIRTUAL_PENDING_WRITE, path, (const char*)0,
                                            write_data, size, message.sender_pid,
                                            message.request_id) == 0) {
                puts("vfsserver delegated write\n");
                yield();
                continue;
            }
            if (status == 0) {
                status = write_mounted_backend(path, write_data, size);
                if (status == OS_VFS_STATUS_NOT_MOUNTED) {
                    puts("vfsserver write outside mounts\n");
                }
            }
            if (status == OS_VFS_STATUS_OK) vfs_list_generation++;
            if (os_vfs_make_write_reply(&reply_payload, status, message.request_id) == 0) {
                (void)ipc_send(message.sender_pid, &reply_payload);
            }
        } else if (received == 0 && message.type == OS_IPC_VFS_MKDIR) {
            int status;
            puts("vfsserver mkdir request\n");
            status = os_vfs_parse_mkdir_request(&message, path);
            if (status == 0 && vfs_virtual_mutation_path_is_routed(path) &&
                vfs_virtual_submit_mutation(VFS_VIRTUAL_PENDING_MKDIR, path, (const char*)0,
                                            (const uint8_t*)0, 0U, message.sender_pid,
                                            message.request_id) == 0) {
                puts("vfsserver delegated mkdir\n");
                yield();
                continue;
            }
            if (status == 0) {
                status = mkdir_mounted_backend(path);
                if (status == OS_VFS_STATUS_NOT_MOUNTED) puts("vfsserver mkdir outside mounts\n");
            }
            if (status == OS_VFS_STATUS_OK) vfs_list_generation++;
            if (os_vfs_make_mkdir_reply(&reply_payload, status, message.request_id) == 0) {
                (void)ipc_send(message.sender_pid, &reply_payload);
            }
        } else if (received == 0 && message.type == OS_IPC_VFS_RMDIR) {
            int status;
            puts("vfsserver rmdir request\n");
            status = os_vfs_parse_rmdir_request(&message, path);
            if (status == 0 && vfs_virtual_mutation_path_is_routed(path) &&
                vfs_virtual_submit_mutation(VFS_VIRTUAL_PENDING_RMDIR, path, (const char*)0,
                                            (const uint8_t*)0, 0U, message.sender_pid,
                                            message.request_id) == 0) {
                puts("vfsserver delegated rmdir\n");
                yield();
                continue;
            }
            if (status == 0) {
                status = rmdir_mounted_backend(path);
                if (status == OS_VFS_STATUS_NOT_MOUNTED) puts("vfsserver rmdir outside mounts\n");
            }
            if (status == OS_VFS_STATUS_OK) vfs_list_generation++;
            if (os_vfs_make_rmdir_reply(&reply_payload, status, message.request_id) == 0) {
                (void)ipc_send(message.sender_pid, &reply_payload);
            }
        } else if (received == 0 && message.type == OS_IPC_VFS_REMOVE) {
            int status;
            vfs_remove_requests++;
            puts("vfsserver remove request\n");
            status = os_vfs_parse_remove_request(&message, path);
            if (status == 0 && vfs_virtual_mutation_path_is_routed(path) &&
                vfs_virtual_submit_mutation(VFS_VIRTUAL_PENDING_REMOVE, path, (const char*)0,
                                            (const uint8_t*)0, 0U, message.sender_pid,
                                            message.request_id) == 0) {
                puts("vfsserver delegated remove\n");
                yield();
                continue;
            }
            if (status == 0) {
                status = remove_mounted_backend(path);
                if (status == OS_VFS_STATUS_NOT_MOUNTED) {
                    puts("vfsserver remove outside mounts\n");
                }
            }
            if (status == OS_VFS_STATUS_OK) vfs_list_generation++;
            if (os_vfs_make_remove_reply(&reply_payload, status, message.request_id) == 0) {
                (void)ipc_send(message.sender_pid, &reply_payload);
            }
        } else if (received == 0 && message.type == OS_IPC_VFS_RENAME) {
            int status;
            vfs_rename_requests++;
            puts("vfsserver rename request\n");
            status = os_vfs_parse_rename_request(&message, path, new_path);
            if (status == 0 && vfs_virtual_mutation_path_is_routed(path) &&
                vfs_virtual_mutation_path_is_routed(new_path) &&
                vfs_virtual_submit_mutation(VFS_VIRTUAL_PENDING_RENAME, path, new_path,
                                            (const uint8_t*)0, 0U, message.sender_pid,
                                            message.request_id) == 0) {
                puts("vfsserver delegated rename\n");
                yield();
                continue;
            }
            if (status == 0) {
                status = rename_mounted_backend(path, new_path);
                if (status == OS_VFS_STATUS_NOT_MOUNTED) {
                    puts("vfsserver rename outside mounts\n");
                }
            }
            if (status == OS_VFS_STATUS_OK) vfs_list_generation++;
            if (os_vfs_make_rename_reply(&reply_payload, status, message.request_id) == 0) {
                (void)ipc_send(message.sender_pid, &reply_payload);
            }
        } else if (received == 0 && message.type == OS_IPC_VFS_MOUNT_ADD) {
            uint32_t source;
            int status;
            puts("vfsserver mount add request\n");
            status = os_vfs_parse_mount_add_request(&message, path, &source);
            if (status == 0 && vfs_virtual_submit_mount_mutation(VFS_VIRTUAL_PENDING_MOUNT_ADD,
                                                                  path, source, message.sender_pid,
                                                                  message.request_id) == 0) {
                puts("vfsserver delegated mount add\n");
                yield();
                continue;
            }
            if (status == 0) status = OS_VFS_STATUS_INVALID;
            puts("vfsserver mount add rc ");
            print_int(status);
            puts("\n");
            if (os_vfs_make_mount_reply(&reply_payload, OS_IPC_VFS_MOUNT_ADD_REPLY,
                                        status, message.request_id) == 0) {
                (void)ipc_send(message.sender_pid, &reply_payload);
            }
        } else if (received == 0 && message.type == OS_IPC_VFS_MOUNT_REMOVE) {
            int status;
            puts("vfsserver mount remove request\n");
            status = os_vfs_parse_mount_remove_request(&message, path);
            if (status == 0 && vfs_mount_is_protected(path)) status = OS_VFS_STATUS_INVALID;
            if (status == 0 && vfs_virtual_submit_mount_mutation(VFS_VIRTUAL_PENDING_MOUNT_REMOVE,
                                                                  path, 0U, message.sender_pid,
                                                                  message.request_id) == 0) {
                puts("vfsserver delegated mount remove\n");
                yield();
                continue;
            }
            if (status == 0) status = OS_VFS_STATUS_INVALID;
            puts("vfsserver mount remove rc ");
            print_int(status);
            puts("\n");
            if (os_vfs_make_mount_reply(&reply_payload, OS_IPC_VFS_MOUNT_REMOVE_REPLY,
                                        status, message.request_id) == 0) {
                (void)ipc_send(message.sender_pid, &reply_payload);
            }
        } else if (received == 0 && message.type == OS_IPC_VFS_BACKEND_GRANT) {
            int target_pid;
            int status;
            puts("vfsserver backend grant request\n");
            status = os_vfs_parse_backend_grant_request(&message, &target_pid);
            if (status == 0) status = service_backend_grant("vfs", target_pid);
            if (os_vfs_make_backend_grant_reply(&reply_payload, status, message.request_id) == 0) {
                (void)ipc_send(message.sender_pid, &reply_payload);
            }
        } else if (received == 0 && message.type == OS_IPC_VFS_BACKEND_GRANT_SCOPED) {
            int target_pid;
            int status;
            uint32_t rights;
            puts("vfsserver backend scoped grant request\n");
            status = os_vfs_parse_backend_grant_scoped_request(&message, &target_pid, &rights);
            if (status == 0) status = service_backend_grant_scoped("vfs", target_pid, rights);
            if (os_vfs_make_backend_grant_scoped_reply(&reply_payload, status, message.request_id) == 0) {
                (void)ipc_send(message.sender_pid, &reply_payload);
            }
        } else if (received == 0 && message.type == OS_IPC_VFS_BACKEND_REVOKE) {
            int target_pid;
            int status;
            puts("vfsserver backend revoke request\n");
            status = os_vfs_parse_backend_revoke_request(&message, &target_pid);
            if (status == 0) status = service_backend_revoke("vfs", target_pid);
            if (os_vfs_make_backend_revoke_reply(&reply_payload, status, message.request_id) == 0) {
                (void)ipc_send(message.sender_pid, &reply_payload);
            }
        } else if (received == 0 && message.type == OS_IPC_VFS_BACKEND_STATUS) {
            int target_pid;
            int status;
            uint32_t rights = 0U;
            puts("vfsserver backend status request\n");
            status = os_vfs_parse_backend_status_request(&message, &target_pid);
            if (status == 0) status = service_backend_status("vfs", target_pid, &rights);
            if (status != 0) rights = 0U;
            if (os_vfs_make_backend_status_reply(&reply_payload, status, rights, message.request_id) == 0) {
                (void)ipc_send(message.sender_pid, &reply_payload);
            }
        } else if (received == 0 && message.type == OS_IPC_VFS_BACKEND_SCOPE) {
            int target_pid;
            int status;
            os_service_backend_scope_t scope;
            scope.rights = 0U;
            scope.sources = 0U;
            puts("vfsserver backend scope request\n");
            status = os_vfs_parse_backend_scope_request(&message, &target_pid);
            if (status == 0) status = service_backend_scope_status("vfs", target_pid, &scope);
            if (status != 0) { scope.rights = 0U; scope.sources = 0U; }
            if (os_vfs_make_backend_scope_reply(&reply_payload, status, scope.rights,
                                                scope.sources, message.request_id) == 0) {
                (void)ipc_send(message.sender_pid, &reply_payload);
            }
        } else if (received == 0 && message.type == OS_IPC_VFS_BACKEND_OBSERVE) {
            os_service_backend_snapshot_t snapshot;
            uint32_t expected_generation;
            int status;
            uint32_t i;
            snapshot.generation = 0U;
            snapshot.list.count = 0U;
            for (i = 0U; i < OS_SERVICE_BACKEND_CAPACITY; i++) {
                snapshot.list.entries[i].pid = 0;
                snapshot.list.entries[i].rights = 0U;
            }
            puts("vfsserver backend observe request\n");
            status = os_vfs_parse_backend_observe_request(&message, &expected_generation);
            if (status == 0) status = service_backend_observe("vfs", expected_generation, &snapshot);
            if (os_vfs_make_backend_observe_reply(&reply_payload, status, &snapshot, message.request_id) == 0) {
                (void)ipc_send(message.sender_pid, &reply_payload);
            }
        } else if (received == 0 && message.type == OS_IPC_VFS_BACKEND_LIST) {
            os_service_backend_list_t list;
            int status;
            puts("vfsserver backend list request\n");
            status = os_vfs_parse_backend_list_request(&message);
            if (status == 0) status = service_backend_list("vfs", &list);
            if (os_vfs_make_backend_list_reply(&reply_payload, status,
                                               status == 0 ? &list : (const os_service_backend_list_t*)0,
                                               message.request_id) == 0) {
                (void)ipc_send(message.sender_pid, &reply_payload);
            }
        } else if (received == 0 && message.type == OS_IPC_VFS_GRANT) {
            int target_pid;
            puts("vfsserver grant request\n");
            int status = os_vfs_parse_grant_request(&message, &target_pid);
            if (status == 0) {
                puts("vfsserver grant vfs ");
                print_int(target_pid);
                puts("\n");
                status = service_grant("vfs", target_pid);
            }
            if (status != 0) {
                puts("vfsserver grant rc ");
                print_int(status);
                puts("\n");
            }
        } else if (received == 0) {
            puts("vfsserver unsupported message\n");
        }
        yield();
    }
}
