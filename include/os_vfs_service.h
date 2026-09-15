#ifndef OS_VFS_SERVICE_H
#define OS_VFS_SERVICE_H

#include <stdint.h>
#include "os_syscalls.h"

#define OS_IPC_VFS_READ       0x56465301U
#define OS_IPC_VFS_READ_REPLY 0x56465302U
#define OS_IPC_VFS_GRANT      0x56465303U
#define OS_IPC_VFS_WRITE      0x56465304U
#define OS_IPC_VFS_WRITE_REPLY 0x56465305U
#define OS_IPC_VFS_REMOVE      0x56465306U
#define OS_IPC_VFS_REMOVE_REPLY 0x56465307U
#define OS_IPC_VFS_RENAME       0x56465308U
#define OS_IPC_VFS_RENAME_REPLY 0x56465309U
#define OS_IPC_VFS_MOUNT_ADD       0x5646530aU
#define OS_IPC_VFS_MOUNT_ADD_REPLY 0x5646530bU
#define OS_IPC_VFS_MOUNT_REMOVE       0x5646530cU
#define OS_IPC_VFS_MOUNT_REMOVE_REPLY 0x5646530dU
#define OS_IPC_VFS_STAT       0x5646530eU
#define OS_IPC_VFS_STAT_REPLY 0x5646530fU
#define OS_IPC_VFS_LIST       0x56465310U
#define OS_IPC_VFS_LIST_REPLY 0x56465311U
#define OS_IPC_VFS_LIST_PAGE       0x56465312U
#define OS_IPC_VFS_LIST_PAGE_REPLY 0x56465313U
#define OS_IPC_VFS_LIST_OBSERVE       0x56465314U
#define OS_IPC_VFS_LIST_OBSERVE_REPLY 0x56465315U
#define OS_IPC_VFS_MKDIR       0x56465316U
#define OS_IPC_VFS_MKDIR_REPLY 0x56465317U
#define OS_IPC_VFS_RMDIR       0x56465318U
#define OS_IPC_VFS_RMDIR_REPLY 0x56465319U
#define OS_IPC_VFS_BACKEND_GRANT       0x5646531aU
#define OS_IPC_VFS_BACKEND_GRANT_REPLY 0x5646531bU
#define OS_IPC_VFS_BACKEND_REVOKE       0x5646531cU
#define OS_IPC_VFS_BACKEND_REVOKE_REPLY 0x5646531dU
#define OS_IPC_VFS_BACKEND_GRANT_SCOPED       0x5646531eU
#define OS_IPC_VFS_BACKEND_GRANT_SCOPED_REPLY 0x5646531fU
#define OS_IPC_VFS_BACKEND_STATUS       0x56465320U
#define OS_IPC_VFS_BACKEND_STATUS_REPLY 0x56465321U
#define OS_IPC_VFS_BACKEND_LIST       0x56465322U
#define OS_IPC_VFS_BACKEND_LIST_REPLY 0x56465323U
#define OS_IPC_VFS_BACKEND_OBSERVE       0x56465324U
#define OS_IPC_VFS_BACKEND_OBSERVE_REPLY 0x56465325U
/* Consultation réservée au propriétaire courant de vfs : droit et sources
 * d’un seul bénéficiaire, sans dévoiler de chemin ou d’alias. */
#define OS_IPC_VFS_BACKEND_SCOPE       0x56465326U
#define OS_IPC_VFS_BACKEND_SCOPE_REPLY 0x56465327U
/* Canal privé entre le médiateur `vfs` et le worker Ring 3 `vfs-virtual`. */
#define OS_IPC_VFS_WORKER_READ       0x56465701U
#define OS_IPC_VFS_WORKER_READ_REPLY 0x56465702U
#define OS_IPC_VFS_WORKER_STATS      0x56465703U
#define OS_IPC_VFS_WORKER_MOUNT      0x56465704U
#define OS_IPC_VFS_WORKER_WRITE      0x56465705U
#define OS_IPC_VFS_WORKER_REMOVE     0x56465706U
#define OS_IPC_VFS_WORKER_RENAME     0x56465707U
#define OS_IPC_VFS_WORKER_MKDIR      0x56465708U
#define OS_IPC_VFS_WORKER_RMDIR      0x56465709U
/* Les alias dynamiques sont mutés dans le worker ; le médiateur conserve
 * seulement un miroir volatil de routage, invalidé au remplacement du PID. */
#define OS_IPC_VFS_WORKER_MOUNT_ADD       0x5646570aU
#define OS_IPC_VFS_WORKER_MOUNT_ADD_REPLY 0x5646570bU
#define OS_IPC_VFS_WORKER_MOUNT_REMOVE       0x5646570cU
#define OS_IPC_VFS_WORKER_MOUNT_REMOVE_REPLY 0x5646570dU
/* I/O d’alias dynamique : messages privés, strictement corrélés au client.
 * Les réponses reprennent les bornes des réponses VFS publiques. */
#define OS_IPC_VFS_WORKER_STAT       0x5646570eU
#define OS_IPC_VFS_WORKER_STAT_REPLY 0x5646570fU
#define OS_IPC_VFS_WORKER_LIST       0x56465710U
#define OS_IPC_VFS_WORKER_LIST_REPLY 0x56465711U
#define OS_IPC_VFS_WORKER_LIST_PAGE       0x56465712U
#define OS_IPC_VFS_WORKER_LIST_PAGE_REPLY 0x56465713U

#define OS_VFS_PATH_MAX 48U
#define OS_VFS_GRANT_REQUEST_SIZE 4U
#define OS_VFS_READ_MAX 80U
#define OS_VFS_WORKER_READ_REQUEST_SIZE OS_VFS_PATH_MAX
#define OS_VFS_WORKER_READ_REPLY_SIZE (8U + OS_VFS_READ_MAX)
#define OS_VFS_WORKER_STATS_REQUEST_SIZE 16U
#define OS_VFS_WORKER_MOUNT_REQUEST_SIZE (OS_VFS_PATH_MAX + 1U)
#define OS_VFS_WORKER_WRITE_REQUEST_SIZE (OS_VFS_PATH_MAX + 4U + OS_VFS_WRITE_MAX)
#define OS_VFS_WORKER_REMOVE_REQUEST_SIZE OS_VFS_PATH_MAX
#define OS_VFS_WORKER_RENAME_REQUEST_SIZE (OS_VFS_PATH_MAX * 2U)
#define OS_VFS_WORKER_MKDIR_REQUEST_SIZE OS_VFS_PATH_MAX
#define OS_VFS_WORKER_RMDIR_REQUEST_SIZE OS_VFS_PATH_MAX
#define OS_VFS_WORKER_MOUNT_ADD_REQUEST_SIZE (OS_VFS_PATH_MAX + 4U)
#define OS_VFS_WORKER_MOUNT_REMOVE_REQUEST_SIZE OS_VFS_PATH_MAX
#define OS_VFS_WORKER_MOUNT_REPLY_SIZE 4U
#define OS_VFS_WORKER_STAT_REQUEST_SIZE OS_VFS_PATH_MAX
#define OS_VFS_WORKER_LIST_REQUEST_SIZE OS_VFS_PATH_MAX
#define OS_VFS_WORKER_LIST_PAGE_REQUEST_SIZE (OS_VFS_PATH_MAX + 4U)
#define OS_VFS_WRITE_MAX 44U
#define OS_VFS_WRITE_REQUEST_SIZE (OS_VFS_PATH_MAX + 4U + OS_VFS_WRITE_MAX)
#define OS_VFS_WRITE_REPLY_SIZE 4U
#define OS_VFS_RENAME_REQUEST_SIZE (OS_VFS_PATH_MAX * 2U)
#define OS_VFS_MOUNT_ADD_REQUEST_SIZE (OS_VFS_PATH_MAX + 4U)
#define OS_VFS_MOUNT_REMOVE_REQUEST_SIZE OS_VFS_PATH_MAX
#define OS_VFS_MOUNT_REPLY_SIZE 4U
#define OS_VFS_MKDIR_REPLY_SIZE 4U
#define OS_VFS_RMDIR_REPLY_SIZE 4U
#define OS_VFS_BACKEND_GRANT_REPLY_SIZE 4U
#define OS_VFS_BACKEND_REVOKE_REPLY_SIZE 4U
#define OS_VFS_BACKEND_GRANT_SCOPED_REQUEST_SIZE 8U
#define OS_VFS_BACKEND_GRANT_SCOPED_REPLY_SIZE 4U
#define OS_VFS_BACKEND_STATUS_REQUEST_SIZE 4U
#define OS_VFS_BACKEND_STATUS_REPLY_SIZE 8U
#define OS_VFS_BACKEND_LIST_REQUEST_SIZE 0U
#define OS_VFS_BACKEND_LIST_REPLY_SIZE (8U + OS_SERVICE_BACKEND_CAPACITY * 8U)
#define OS_VFS_BACKEND_OBSERVE_REQUEST_SIZE 4U
#define OS_VFS_BACKEND_OBSERVE_REPLY_SIZE (12U + OS_SERVICE_BACKEND_CAPACITY * 8U)
#define OS_VFS_BACKEND_SCOPE_REQUEST_SIZE 4U
#define OS_VFS_BACKEND_SCOPE_REPLY_SIZE 12U
#define OS_VFS_BACKEND_RIGHT_READ 1U
#define OS_VFS_BACKEND_RIGHT_MUTATE 2U
#define OS_VFS_BACKEND_RIGHT_ALL (OS_VFS_BACKEND_RIGHT_READ | OS_VFS_BACKEND_RIGHT_MUTATE)
#define OS_VFS_STAT_REPLY_SIZE 12U
/* Réponse LIST : statut (4), nombre d’entrées retournées (4) et texte
 * NUL-paddé de noms séparés par '\n' (72), soit 80 octets au total. */
#define OS_VFS_LIST_DATA_MAX 72U
#define OS_VFS_LIST_REPLY_SIZE (8U + OS_VFS_LIST_DATA_MAX)
#define OS_VFS_LIST_ENTRY_MAX 4U
#define OS_VFS_LIST_PAGE_REQUEST_SIZE (OS_VFS_PATH_MAX + 4U)
#define OS_VFS_LIST_PAGE_DATA_MAX 68U
#define OS_VFS_LIST_PAGE_REPLY_SIZE (12U + OS_VFS_LIST_PAGE_DATA_MAX)
#define OS_VFS_LIST_PAGE_END 0xffffffffU
#define OS_VFS_LIST_OBSERVE_REQUEST_SIZE (OS_VFS_PATH_MAX + 8U)
#define OS_VFS_LIST_OBSERVE_DATA_MAX 64U
#define OS_VFS_LIST_OBSERVE_REPLY_SIZE (16U + OS_VFS_LIST_OBSERVE_DATA_MAX)
#define OS_VFS_STATUS_STALE (-64)
#define OS_VFS_STATUS_NOT_EMPTY (-5)
#define OS_VFS_MOUNT_SOURCE_INITRD 1U
#define OS_VFS_MOUNT_SOURCE_OVERLAY 2U
#define OS_VFS_MOUNT_SOURCE_FAT16 3U
#define OS_VFS_MOUNT_SOURCE_FAT32 4U

#define OS_VFS_STATUS_OK          0
#define OS_VFS_STATUS_INVALID    (-60)
#define OS_VFS_STATUS_NOT_MOUNTED (-61)
#define OS_VFS_STATUS_MOUNT_FULL  (-62)
#define OS_VFS_STATUS_MOUNT_EXISTS (-63)
#define OS_VFS_STATUS_TRUNCATED   1

typedef struct {
    char path[OS_VFS_PATH_MAX];
} os_vfs_read_request_t;

typedef struct {
    int32_t status;
    uint32_t size;
    uint8_t data[OS_VFS_READ_MAX];
} os_vfs_read_reply_t;

typedef struct {
    int32_t status;
    uint32_t size;
    uint32_t flags;
} os_vfs_stat_reply_t;

typedef struct {
    int32_t status;
    uint32_t count;
    uint8_t data[OS_VFS_LIST_DATA_MAX];
} os_vfs_list_reply_t;

typedef struct {
    int32_t status;
    uint32_t count;
    os_service_backend_entry_t entries[OS_SERVICE_BACKEND_CAPACITY];
} os_vfs_backend_list_reply_t;

typedef struct {
    int32_t status;
    uint32_t rights;
    uint32_t sources;
} os_vfs_backend_scope_reply_t;

typedef struct {
    int32_t status;
    uint32_t generation;
    uint32_t count;
    os_service_backend_entry_t entries[OS_SERVICE_BACKEND_CAPACITY];
} os_vfs_backend_observe_reply_t;

typedef struct {
    int32_t status;
    uint32_t count;
    uint32_t next_start;
    uint8_t data[OS_VFS_LIST_PAGE_DATA_MAX];
} os_vfs_list_page_reply_t;

typedef struct {
    int32_t status;
    uint32_t count;
    uint32_t next_start;
    uint32_t generation;
    uint8_t data[OS_VFS_LIST_OBSERVE_DATA_MAX];
} os_vfs_list_observe_reply_t;

typedef struct {
    int32_t status;
} os_vfs_write_reply_t;

typedef struct {
    int32_t status;
} os_vfs_remove_reply_t;

typedef struct {
    int32_t status;
} os_vfs_rename_reply_t;

typedef struct {
    int32_t status;
} os_vfs_mount_reply_t;

static inline int os_vfs_path_is_safe(const char* path) {
    uint32_t i;
    if (!path || path[0] == '\0') return 0;
    for (i = 0U; i < OS_VFS_PATH_MAX; i++) {
        if (path[i] == '\0') return 1;
        if (path[i] == '.' && i + 1U < OS_VFS_PATH_MAX && path[i + 1U] == '.') return 0;
    }
    return 0;
}

/* Un montage est un préfixe de répertoire terminé par '/'. Le chemin relatif
 * retourné ne peut pas être vide : lire la racine d'un montage n'est pas une
 * opération de lecture de fichier. */
static inline int os_vfs_mount_prefix_is_valid(const char* mount) {
    uint32_t i = 0U;
    if (!os_vfs_path_is_safe(mount)) return 0;
    while (mount[i] != '\0') i++;
    return i > 0U && mount[i - 1U] == '/';
}

static inline int os_vfs_mount_source_is_valid(uint32_t source) {
    return source == OS_VFS_MOUNT_SOURCE_INITRD || source == OS_VFS_MOUNT_SOURCE_OVERLAY ||
           source == OS_VFS_MOUNT_SOURCE_FAT16 || source == OS_VFS_MOUNT_SOURCE_FAT32;
}

/* Le listage cible un répertoire : il accepte la racine du montage ou un
 * sous-répertoire, mais impose toujours le séparateur terminal. */
static inline int os_vfs_list_path_is_valid(const char* path) {
    uint32_t i = 0U;
    if (!os_vfs_path_is_safe(path)) return 0;
    while (path[i] != '\0') i++;
    return i > 0U && path[i - 1U] == '/';
}

/* La pagination peut aussi observer la table virtuelle de montages. Cette
 * exception ne s’applique ni aux listes ordinaires ni aux chemins physiques. */
static inline int os_vfs_list_page_path_is_valid(const char* path) {
    if (os_vfs_list_path_is_valid(path)) return 1;
    return path && path[0] == 'v' && path[1] == 'f' && path[2] == 's' &&
           path[3] == '-' && path[4] == 'm' && path[5] == 'o' &&
           path[6] == 'u' && path[7] == 'n' && path[8] == 't' &&
           path[9] == 's' && path[10] == '\0';
}

static inline int os_vfs_match_mount(const char* path, const char* mount,
                                     const char** relative_out) {
    uint32_t i = 0U;
    if (!os_vfs_path_is_safe(path) || !os_vfs_path_is_safe(mount)) return 0;
    while (mount[i] != '\0') {
        if (path[i] == '\0' || path[i] != mount[i]) return 0;
        i++;
    }
    if (i == 0U || mount[i - 1U] != '/' || path[i] == '\0') return 0;
    if (relative_out) *relative_out = path + i;
    return 1;
}

static inline int os_vfs_make_worker_read_request(os_ipc_payload_t* payload, const char* path,
                                                  uint32_t request_id) {
    uint32_t i;
    if (!payload || !os_vfs_path_is_safe(path)) return OS_VFS_STATUS_INVALID;
    payload->type = OS_IPC_VFS_WORKER_READ;
    payload->size = OS_VFS_WORKER_READ_REQUEST_SIZE;
    payload->request_id = request_id;
    for (i = 0U; i < OS_VFS_PATH_MAX; i++) payload->data[i] = (uint8_t)path[i];
    for (i = OS_VFS_PATH_MAX; i < OS_IPC_MAX_DATA; i++) payload->data[i] = 0U;
    return OS_VFS_STATUS_OK;
}

static inline int os_vfs_parse_worker_read_request(const os_ipc_message_t* message, char* path_out) {
    uint32_t i;
    if (!message || !path_out || message->type != OS_IPC_VFS_WORKER_READ ||
        message->size != OS_VFS_WORKER_READ_REQUEST_SIZE) return OS_VFS_STATUS_INVALID;
    for (i = 0U; i < OS_VFS_PATH_MAX; i++) path_out[i] = (char)message->data[i];
    return os_vfs_path_is_safe(path_out) ? OS_VFS_STATUS_OK : OS_VFS_STATUS_INVALID;
}

static inline int os_vfs_make_worker_read_reply(os_ipc_payload_t* payload, int32_t status,
                                                const uint8_t* data, uint32_t size,
                                                uint32_t request_id) {
    uint32_t i, raw = (uint32_t)status;
    if (!payload || size > OS_VFS_READ_MAX || (size > 0U && !data)) return OS_VFS_STATUS_INVALID;
    payload->type = OS_IPC_VFS_WORKER_READ_REPLY;
    payload->size = OS_VFS_WORKER_READ_REPLY_SIZE;
    payload->request_id = request_id;
    payload->data[0] = (uint8_t)(raw & 0xffU);
    payload->data[1] = (uint8_t)((raw >> 8) & 0xffU);
    payload->data[2] = (uint8_t)((raw >> 16) & 0xffU);
    payload->data[3] = (uint8_t)((raw >> 24) & 0xffU);
    payload->data[4] = (uint8_t)(size & 0xffU);
    payload->data[5] = (uint8_t)((size >> 8) & 0xffU);
    payload->data[6] = (uint8_t)((size >> 16) & 0xffU);
    payload->data[7] = (uint8_t)((size >> 24) & 0xffU);
    for (i = 0U; i < OS_VFS_READ_MAX; i++) payload->data[8U + i] = i < size ? data[i] : 0U;
    return OS_VFS_STATUS_OK;
}

static inline int os_vfs_parse_worker_read_reply(const os_ipc_message_t* message,
                                                 int32_t* status_out, uint8_t* data_out,
                                                 uint32_t* size_out, uint32_t request_id) {
    uint32_t i, size, raw;
    if (!message || !status_out || !data_out || !size_out || message->type != OS_IPC_VFS_WORKER_READ_REPLY ||
        message->size != OS_VFS_WORKER_READ_REPLY_SIZE || message->request_id != request_id)
        return OS_VFS_STATUS_INVALID;
    raw = (uint32_t)message->data[0] | ((uint32_t)message->data[1] << 8) |
          ((uint32_t)message->data[2] << 16) | ((uint32_t)message->data[3] << 24);
    size = (uint32_t)message->data[4] | ((uint32_t)message->data[5] << 8) |
           ((uint32_t)message->data[6] << 16) | ((uint32_t)message->data[7] << 24);
    if (size > OS_VFS_READ_MAX) return OS_VFS_STATUS_INVALID;
    *status_out = (int32_t)raw;
    *size_out = size;
    for (i = 0U; i < size; i++) data_out[i] = message->data[8U + i];
    return OS_VFS_STATUS_OK;
}

static inline int os_vfs_make_worker_stats_request(os_ipc_payload_t* payload,
                                                   uint32_t reads, uint32_t writes,
                                                   uint32_t removes, uint32_t renames,
                                                   uint32_t request_id) {
    uint32_t i;
    if (!payload) return OS_VFS_STATUS_INVALID;
    payload->type = OS_IPC_VFS_WORKER_STATS;
    payload->size = OS_VFS_WORKER_STATS_REQUEST_SIZE;
    payload->request_id = request_id;
    payload->data[0] = (uint8_t)(reads & 0xffU);
    payload->data[1] = (uint8_t)((reads >> 8) & 0xffU);
    payload->data[2] = (uint8_t)((reads >> 16) & 0xffU);
    payload->data[3] = (uint8_t)((reads >> 24) & 0xffU);
    payload->data[4] = (uint8_t)(writes & 0xffU);
    payload->data[5] = (uint8_t)((writes >> 8) & 0xffU);
    payload->data[6] = (uint8_t)((writes >> 16) & 0xffU);
    payload->data[7] = (uint8_t)((writes >> 24) & 0xffU);
    payload->data[8] = (uint8_t)(removes & 0xffU);
    payload->data[9] = (uint8_t)((removes >> 8) & 0xffU);
    payload->data[10] = (uint8_t)((removes >> 16) & 0xffU);
    payload->data[11] = (uint8_t)((removes >> 24) & 0xffU);
    payload->data[12] = (uint8_t)(renames & 0xffU);
    payload->data[13] = (uint8_t)((renames >> 8) & 0xffU);
    payload->data[14] = (uint8_t)((renames >> 16) & 0xffU);
    payload->data[15] = (uint8_t)((renames >> 24) & 0xffU);
    for (i = OS_VFS_WORKER_STATS_REQUEST_SIZE; i < OS_IPC_MAX_DATA; i++) payload->data[i] = 0U;
    return OS_VFS_STATUS_OK;
}

static inline int os_vfs_parse_worker_stats_request(const os_ipc_message_t* message,
                                                    uint32_t* reads, uint32_t* writes,
                                                    uint32_t* removes, uint32_t* renames) {
    if (!message || !reads || !writes || !removes || !renames ||
        message->type != OS_IPC_VFS_WORKER_STATS ||
        message->size != OS_VFS_WORKER_STATS_REQUEST_SIZE) return OS_VFS_STATUS_INVALID;
    *reads = (uint32_t)message->data[0] | ((uint32_t)message->data[1] << 8) |
             ((uint32_t)message->data[2] << 16) | ((uint32_t)message->data[3] << 24);
    *writes = (uint32_t)message->data[4] | ((uint32_t)message->data[5] << 8) |
              ((uint32_t)message->data[6] << 16) | ((uint32_t)message->data[7] << 24);
    *removes = (uint32_t)message->data[8] | ((uint32_t)message->data[9] << 8) |
               ((uint32_t)message->data[10] << 16) | ((uint32_t)message->data[11] << 24);
    *renames = (uint32_t)message->data[12] | ((uint32_t)message->data[13] << 8) |
               ((uint32_t)message->data[14] << 16) | ((uint32_t)message->data[15] << 24);
    return OS_VFS_STATUS_OK;
}

static inline void os_vfs_encode_i32(uint8_t* out, int32_t value) {
    uint32_t raw = (uint32_t)value;
    out[0] = (uint8_t)(raw & 0xffU);
    out[1] = (uint8_t)((raw >> 8) & 0xffU);
    out[2] = (uint8_t)((raw >> 16) & 0xffU);
    out[3] = (uint8_t)((raw >> 24) & 0xffU);
}

static inline int32_t os_vfs_decode_i32(const uint8_t* in) {
    uint32_t raw = (uint32_t)in[0] | ((uint32_t)in[1] << 8) |
                   ((uint32_t)in[2] << 16) | ((uint32_t)in[3] << 24);
    return (int32_t)raw;
}

static inline void os_vfs_encode_u32(uint8_t* out, uint32_t value) {
    out[0] = (uint8_t)(value & 0xffU);
    out[1] = (uint8_t)((value >> 8) & 0xffU);
    out[2] = (uint8_t)((value >> 16) & 0xffU);
    out[3] = (uint8_t)((value >> 24) & 0xffU);
}

static inline uint32_t os_vfs_decode_u32(const uint8_t* in) {
    return (uint32_t)in[0] | ((uint32_t)in[1] << 8) |
           ((uint32_t)in[2] << 16) | ((uint32_t)in[3] << 24);
}

static inline int os_vfs_make_worker_mount_request(os_ipc_payload_t* payload, const char* prefix,
                                                   uint32_t writable, uint32_t request_id) {
    uint32_t i;
    if (!payload || !os_vfs_mount_prefix_is_valid(prefix) || writable > 1U) return OS_VFS_STATUS_INVALID;
    payload->type = OS_IPC_VFS_WORKER_MOUNT;
    payload->size = OS_VFS_WORKER_MOUNT_REQUEST_SIZE;
    payload->request_id = request_id;
    for (i = 0U; i < OS_VFS_PATH_MAX; i++) payload->data[i] = (uint8_t)prefix[i];
    payload->data[OS_VFS_PATH_MAX] = (uint8_t)writable;
    for (i = OS_VFS_WORKER_MOUNT_REQUEST_SIZE; i < OS_IPC_MAX_DATA; i++) payload->data[i] = 0U;
    return OS_VFS_STATUS_OK;
}

static inline int os_vfs_parse_worker_mount_request(const os_ipc_message_t* message, char* prefix_out,
                                                    uint32_t* writable_out) {
    uint32_t i;
    if (!message || !prefix_out || !writable_out || message->type != OS_IPC_VFS_WORKER_MOUNT ||
        message->size != OS_VFS_WORKER_MOUNT_REQUEST_SIZE) return OS_VFS_STATUS_INVALID;
    for (i = 0U; i < OS_VFS_PATH_MAX; i++) prefix_out[i] = (char)message->data[i];
    if (!os_vfs_mount_prefix_is_valid(prefix_out) || message->data[OS_VFS_PATH_MAX] > 1U)
        return OS_VFS_STATUS_INVALID;
    *writable_out = (uint32_t)message->data[OS_VFS_PATH_MAX];
    return OS_VFS_STATUS_OK;
}

static inline int os_vfs_make_worker_mount_add_request(os_ipc_payload_t* payload,
                                                        const char* prefix, uint32_t source,
                                                        uint32_t request_id) {
    uint32_t i;
    if (!payload || !os_vfs_mount_prefix_is_valid(prefix) || !os_vfs_mount_source_is_valid(source))
        return OS_VFS_STATUS_INVALID;
    payload->type = OS_IPC_VFS_WORKER_MOUNT_ADD;
    payload->size = OS_VFS_WORKER_MOUNT_ADD_REQUEST_SIZE;
    payload->request_id = request_id;
    for (i = 0U; i < OS_VFS_PATH_MAX; i++) payload->data[i] = (uint8_t)prefix[i];
    os_vfs_encode_u32(&payload->data[OS_VFS_PATH_MAX], source);
    for (i = OS_VFS_WORKER_MOUNT_ADD_REQUEST_SIZE; i < OS_IPC_MAX_DATA; i++) payload->data[i] = 0U;
    return OS_VFS_STATUS_OK;
}

static inline int os_vfs_parse_worker_mount_add_request(const os_ipc_message_t* message,
                                                         char* prefix_out, uint32_t* source_out) {
    uint32_t i;
    if (!message || !prefix_out || !source_out || message->type != OS_IPC_VFS_WORKER_MOUNT_ADD ||
        message->size != OS_VFS_WORKER_MOUNT_ADD_REQUEST_SIZE) return OS_VFS_STATUS_INVALID;
    for (i = 0U; i < OS_VFS_PATH_MAX; i++) prefix_out[i] = (char)message->data[i];
    *source_out = os_vfs_decode_u32(&message->data[OS_VFS_PATH_MAX]);
    return os_vfs_mount_prefix_is_valid(prefix_out) && os_vfs_mount_source_is_valid(*source_out)
           ? OS_VFS_STATUS_OK : OS_VFS_STATUS_INVALID;
}

static inline int os_vfs_make_worker_mount_remove_request(os_ipc_payload_t* payload,
                                                           const char* prefix, uint32_t request_id) {
    uint32_t i;
    if (!payload || !os_vfs_mount_prefix_is_valid(prefix)) return OS_VFS_STATUS_INVALID;
    payload->type = OS_IPC_VFS_WORKER_MOUNT_REMOVE;
    payload->size = OS_VFS_WORKER_MOUNT_REMOVE_REQUEST_SIZE;
    payload->request_id = request_id;
    for (i = 0U; i < OS_VFS_PATH_MAX; i++) payload->data[i] = (uint8_t)prefix[i];
    for (i = OS_VFS_WORKER_MOUNT_REMOVE_REQUEST_SIZE; i < OS_IPC_MAX_DATA; i++) payload->data[i] = 0U;
    return OS_VFS_STATUS_OK;
}

static inline int os_vfs_parse_worker_mount_remove_request(const os_ipc_message_t* message,
                                                            char* prefix_out) {
    uint32_t i;
    if (!message || !prefix_out || message->type != OS_IPC_VFS_WORKER_MOUNT_REMOVE ||
        message->size != OS_VFS_WORKER_MOUNT_REMOVE_REQUEST_SIZE) return OS_VFS_STATUS_INVALID;
    for (i = 0U; i < OS_VFS_PATH_MAX; i++) prefix_out[i] = (char)message->data[i];
    return os_vfs_mount_prefix_is_valid(prefix_out) ? OS_VFS_STATUS_OK : OS_VFS_STATUS_INVALID;
}

static inline int os_vfs_make_worker_mount_reply(os_ipc_payload_t* payload, uint32_t type,
                                                  int32_t status, uint32_t request_id) {
    uint32_t i;
    if (!payload || (type != OS_IPC_VFS_WORKER_MOUNT_ADD_REPLY &&
                     type != OS_IPC_VFS_WORKER_MOUNT_REMOVE_REPLY)) return OS_VFS_STATUS_INVALID;
    payload->type = type;
    payload->size = OS_VFS_WORKER_MOUNT_REPLY_SIZE;
    payload->request_id = request_id;
    os_vfs_encode_i32(payload->data, status);
    for (i = OS_VFS_WORKER_MOUNT_REPLY_SIZE; i < OS_IPC_MAX_DATA; i++) payload->data[i] = 0U;
    return OS_VFS_STATUS_OK;
}

static inline int os_vfs_parse_worker_mount_reply(const os_ipc_message_t* message, uint32_t type,
                                                   int32_t* status_out, uint32_t expected_request_id) {
    if (!message || !status_out || (type != OS_IPC_VFS_WORKER_MOUNT_ADD_REPLY &&
                                   type != OS_IPC_VFS_WORKER_MOUNT_REMOVE_REPLY) ||
        message->type != type || message->size != OS_VFS_WORKER_MOUNT_REPLY_SIZE ||
        message->request_id != expected_request_id) return OS_VFS_STATUS_INVALID;
    *status_out = os_vfs_decode_i32(message->data);
    return OS_VFS_STATUS_OK;
}

static inline int os_vfs_make_worker_write_request(os_ipc_payload_t* payload, const char* path,
                                                   const uint8_t* data, uint32_t size,
                                                   uint32_t request_id) {
    uint32_t i;
    if (!payload || !os_vfs_path_is_safe(path) || size > OS_VFS_WRITE_MAX || (size > 0U && !data))
        return OS_VFS_STATUS_INVALID;
    payload->type = OS_IPC_VFS_WORKER_WRITE;
    payload->size = OS_VFS_WORKER_WRITE_REQUEST_SIZE;
    payload->request_id = request_id;
    for (i = 0U; i < OS_VFS_PATH_MAX; i++) payload->data[i] = (uint8_t)path[i];
    os_vfs_encode_u32(&payload->data[OS_VFS_PATH_MAX], size);
    for (i = 0U; i < size; i++) payload->data[OS_VFS_PATH_MAX + 4U + i] = data[i];
    for (; i < OS_VFS_WRITE_MAX; i++) payload->data[OS_VFS_PATH_MAX + 4U + i] = 0U;
    return OS_VFS_STATUS_OK;
}

static inline int os_vfs_parse_worker_write_request(const os_ipc_message_t* message, char* path_out,
                                                    uint8_t* data_out, uint32_t* size_out) {
    uint32_t i, size;
    if (!message || !path_out || !data_out || !size_out || message->type != OS_IPC_VFS_WORKER_WRITE ||
        message->size != OS_VFS_WORKER_WRITE_REQUEST_SIZE) return OS_VFS_STATUS_INVALID;
    for (i = 0U; i < OS_VFS_PATH_MAX; i++) path_out[i] = (char)message->data[i];
    if (!os_vfs_path_is_safe(path_out)) return OS_VFS_STATUS_INVALID;
    size = os_vfs_decode_u32(&message->data[OS_VFS_PATH_MAX]);
    if (size > OS_VFS_WRITE_MAX) return OS_VFS_STATUS_INVALID;
    for (i = 0U; i < size; i++) data_out[i] = message->data[OS_VFS_PATH_MAX + 4U + i];
    *size_out = size;
    return OS_VFS_STATUS_OK;
}

static inline int os_vfs_make_worker_remove_request(os_ipc_payload_t* payload, const char* path,
                                                     uint32_t request_id) {
    uint32_t i;
    if (!payload || !os_vfs_path_is_safe(path)) return OS_VFS_STATUS_INVALID;
    payload->type = OS_IPC_VFS_WORKER_REMOVE;
    payload->size = OS_VFS_WORKER_REMOVE_REQUEST_SIZE;
    payload->request_id = request_id;
    for (i = 0U; i < OS_VFS_PATH_MAX; i++) payload->data[i] = (uint8_t)path[i];
    return OS_VFS_STATUS_OK;
}

static inline int os_vfs_parse_worker_remove_request(const os_ipc_message_t* message, char* path_out) {
    uint32_t i;
    if (!message || !path_out || message->type != OS_IPC_VFS_WORKER_REMOVE ||
        message->size != OS_VFS_WORKER_REMOVE_REQUEST_SIZE) return OS_VFS_STATUS_INVALID;
    for (i = 0U; i < OS_VFS_PATH_MAX; i++) path_out[i] = (char)message->data[i];
    if (!os_vfs_path_is_safe(path_out)) return OS_VFS_STATUS_INVALID;
    return OS_VFS_STATUS_OK;
}

static inline int os_vfs_make_worker_directory_request(os_ipc_payload_t* payload, uint32_t type,
                                                        const char* path, uint32_t request_id) {
    uint32_t i;
    if (!payload || !os_vfs_path_is_safe(path) ||
        (type != OS_IPC_VFS_WORKER_MKDIR && type != OS_IPC_VFS_WORKER_RMDIR)) {
        return OS_VFS_STATUS_INVALID;
    }
    payload->type = type;
    payload->size = OS_VFS_PATH_MAX;
    payload->request_id = request_id;
    for (i = 0U; i < OS_VFS_PATH_MAX; i++) payload->data[i] = (uint8_t)path[i];
    return OS_VFS_STATUS_OK;
}

static inline int os_vfs_parse_worker_directory_request(const os_ipc_message_t* message,
                                                         uint32_t type, char* path_out) {
    uint32_t i;
    if (!message || !path_out || message->type != type ||
        (type != OS_IPC_VFS_WORKER_MKDIR && type != OS_IPC_VFS_WORKER_RMDIR) ||
        message->size != OS_VFS_PATH_MAX) return OS_VFS_STATUS_INVALID;
    for (i = 0U; i < OS_VFS_PATH_MAX; i++) path_out[i] = (char)message->data[i];
    return os_vfs_path_is_safe(path_out) ? OS_VFS_STATUS_OK : OS_VFS_STATUS_INVALID;
}

static inline int os_vfs_make_worker_rename_request(os_ipc_payload_t* payload, const char* old_path,
                                                     const char* new_path, uint32_t request_id) {
    uint32_t i;
    if (!payload || !os_vfs_path_is_safe(old_path) || !os_vfs_path_is_safe(new_path))
        return OS_VFS_STATUS_INVALID;
    payload->type = OS_IPC_VFS_WORKER_RENAME;
    payload->size = OS_VFS_WORKER_RENAME_REQUEST_SIZE;
    payload->request_id = request_id;
    for (i = 0U; i < OS_VFS_PATH_MAX; i++) payload->data[i] = (uint8_t)old_path[i];
    for (i = 0U; i < OS_VFS_PATH_MAX; i++) payload->data[OS_VFS_PATH_MAX + i] = (uint8_t)new_path[i];
    return OS_VFS_STATUS_OK;
}

static inline int os_vfs_parse_worker_rename_request(const os_ipc_message_t* message, char* old_path_out,
                                                     char* new_path_out) {
    uint32_t i;
    if (!message || !old_path_out || !new_path_out || message->type != OS_IPC_VFS_WORKER_RENAME ||
        message->size != OS_VFS_WORKER_RENAME_REQUEST_SIZE) return OS_VFS_STATUS_INVALID;
    for (i = 0U; i < OS_VFS_PATH_MAX; i++) {
        old_path_out[i] = (char)message->data[i];
        new_path_out[i] = (char)message->data[OS_VFS_PATH_MAX + i];
    }
    if (!os_vfs_path_is_safe(old_path_out) || !os_vfs_path_is_safe(new_path_out))
        return OS_VFS_STATUS_INVALID;
    return OS_VFS_STATUS_OK;
}

static inline int os_vfs_make_read_request(os_ipc_payload_t* payload, const char* path,
                                           uint32_t request_id) {
    uint32_t i;
    if (!payload || !os_vfs_path_is_safe(path)) return OS_VFS_STATUS_INVALID;
    payload->type = OS_IPC_VFS_READ;
    payload->size = OS_VFS_PATH_MAX;
    payload->request_id = request_id;
    for (i = 0U; i < OS_VFS_PATH_MAX; i++) {
        if (path[i] == '\0') break;
        payload->data[i] = (uint8_t)path[i];
    }
    for (; i < OS_IPC_MAX_DATA; i++) payload->data[i] = 0U;
    return 0;
}

static inline int os_vfs_parse_read_request(const os_ipc_message_t* message, char* path_out) {
    uint32_t i;
    if (!message || !path_out || message->type != OS_IPC_VFS_READ ||
        message->size != OS_VFS_PATH_MAX) return OS_VFS_STATUS_INVALID;
    for (i = 0U; i < OS_VFS_PATH_MAX; i++) path_out[i] = (char)message->data[i];
    if (!os_vfs_path_is_safe(path_out)) return OS_VFS_STATUS_INVALID;
    return 0;
}

static inline int os_vfs_make_list_request(os_ipc_payload_t* payload, const char* path,
                                           uint32_t request_id) {
    uint32_t i;
    if (!payload || !os_vfs_list_path_is_valid(path)) return OS_VFS_STATUS_INVALID;
    payload->type = OS_IPC_VFS_LIST;
    payload->size = OS_VFS_PATH_MAX;
    payload->request_id = request_id;
    for (i = 0U; i < OS_VFS_PATH_MAX; i++) {
        payload->data[i] = (uint8_t)path[i];
        if (path[i] == '\0') {
            for (i++; i < OS_IPC_MAX_DATA; i++) payload->data[i] = 0U;
            break;
        }
    }
    return 0;
}

static inline int os_vfs_parse_list_request(const os_ipc_message_t* message, char* path_out) {
    uint32_t i;
    if (!message || !path_out || message->type != OS_IPC_VFS_LIST ||
        message->size != OS_VFS_PATH_MAX) return OS_VFS_STATUS_INVALID;
    for (i = 0U; i < OS_VFS_PATH_MAX; i++) path_out[i] = (char)message->data[i];
    return os_vfs_list_path_is_valid(path_out) ? 0 : OS_VFS_STATUS_INVALID;
}

static inline int os_vfs_make_list_page_request(os_ipc_payload_t* payload, const char* path,
                                                uint32_t start, uint32_t request_id) {
    uint32_t i;
    if (!payload || !os_vfs_list_page_path_is_valid(path)) return OS_VFS_STATUS_INVALID;
    payload->type = OS_IPC_VFS_LIST_PAGE;
    payload->size = OS_VFS_LIST_PAGE_REQUEST_SIZE;
    payload->request_id = request_id;
    for (i = 0U; i < OS_VFS_PATH_MAX; i++) payload->data[i] = (uint8_t)path[i];
    payload->data[48] = (uint8_t)(start & 0xffU);
    payload->data[49] = (uint8_t)((start >> 8) & 0xffU);
    payload->data[50] = (uint8_t)((start >> 16) & 0xffU);
    payload->data[51] = (uint8_t)((start >> 24) & 0xffU);
    for (i = 52U; i < OS_IPC_MAX_DATA; i++) payload->data[i] = 0U;
    return 0;
}

static inline int os_vfs_parse_list_page_request(const os_ipc_message_t* message,
                                                  char* path_out, uint32_t* start_out) {
    uint32_t i;
    if (!message || !path_out || !start_out || message->type != OS_IPC_VFS_LIST_PAGE ||
        message->size != OS_VFS_LIST_PAGE_REQUEST_SIZE) return OS_VFS_STATUS_INVALID;
    for (i = 0U; i < OS_VFS_PATH_MAX; i++) path_out[i] = (char)message->data[i];
    if (!os_vfs_list_page_path_is_valid(path_out)) return OS_VFS_STATUS_INVALID;
    *start_out = (uint32_t)message->data[48] | ((uint32_t)message->data[49] << 8) |
                 ((uint32_t)message->data[50] << 16) | ((uint32_t)message->data[51] << 24);
    return 0;
}

static inline int os_vfs_make_mkdir_request(os_ipc_payload_t* payload, const char* path,
                                            uint32_t request_id) {
    uint32_t i;
    if (!payload || !os_vfs_path_is_safe(path)) return OS_VFS_STATUS_INVALID;
    payload->type = OS_IPC_VFS_MKDIR; payload->size = OS_VFS_PATH_MAX; payload->request_id = request_id;
    for (i = 0U; i < OS_VFS_PATH_MAX; i++) payload->data[i] = (uint8_t)path[i];
    for (i = OS_VFS_PATH_MAX; i < OS_IPC_MAX_DATA; i++) payload->data[i] = 0U;
    return 0;
}

static inline int os_vfs_parse_mkdir_request(const os_ipc_message_t* message, char* path_out) {
    uint32_t i;
    if (!message || !path_out || message->type != OS_IPC_VFS_MKDIR || message->size != OS_VFS_PATH_MAX) return OS_VFS_STATUS_INVALID;
    for (i = 0U; i < OS_VFS_PATH_MAX; i++) path_out[i] = (char)message->data[i];
    return os_vfs_path_is_safe(path_out) ? 0 : OS_VFS_STATUS_INVALID;
}

static inline int os_vfs_make_mkdir_reply(os_ipc_payload_t* payload, int32_t status, uint32_t request_id) {
    uint32_t i;
    uint32_t raw = (uint32_t)status;
    if (!payload) return OS_VFS_STATUS_INVALID;
    payload->type = OS_IPC_VFS_MKDIR_REPLY; payload->size = OS_VFS_MKDIR_REPLY_SIZE; payload->request_id = request_id;
    payload->data[0] = (uint8_t)(raw & 0xffU); payload->data[1] = (uint8_t)((raw >> 8) & 0xffU);
    payload->data[2] = (uint8_t)((raw >> 16) & 0xffU); payload->data[3] = (uint8_t)((raw >> 24) & 0xffU);
    for (i = OS_VFS_MKDIR_REPLY_SIZE; i < OS_IPC_MAX_DATA; i++) payload->data[i] = 0U;
    return 0;
}

static inline int os_vfs_parse_mkdir_reply(const os_ipc_message_t* message, int32_t* status_out, uint32_t expected_request_id) {
    if (!message || !status_out || message->type != OS_IPC_VFS_MKDIR_REPLY ||
        message->size != OS_VFS_MKDIR_REPLY_SIZE || message->request_id != expected_request_id) return OS_VFS_STATUS_INVALID;
    *status_out = (int32_t)((uint32_t)message->data[0] | ((uint32_t)message->data[1] << 8) |
                            ((uint32_t)message->data[2] << 16) | ((uint32_t)message->data[3] << 24));
    return 0;
}

static inline int os_vfs_make_rmdir_request(os_ipc_payload_t* payload, const char* path,
                                            uint32_t request_id) {
    uint32_t i;
    if (!payload || !os_vfs_path_is_safe(path)) return OS_VFS_STATUS_INVALID;
    payload->type = OS_IPC_VFS_RMDIR; payload->size = OS_VFS_PATH_MAX; payload->request_id = request_id;
    for (i = 0U; i < OS_VFS_PATH_MAX; i++) payload->data[i] = (uint8_t)path[i];
    for (i = OS_VFS_PATH_MAX; i < OS_IPC_MAX_DATA; i++) payload->data[i] = 0U;
    return 0;
}

static inline int os_vfs_parse_rmdir_request(const os_ipc_message_t* message, char* path_out) {
    uint32_t i;
    if (!message || !path_out || message->type != OS_IPC_VFS_RMDIR || message->size != OS_VFS_PATH_MAX) return OS_VFS_STATUS_INVALID;
    for (i = 0U; i < OS_VFS_PATH_MAX; i++) path_out[i] = (char)message->data[i];
    return os_vfs_path_is_safe(path_out) ? 0 : OS_VFS_STATUS_INVALID;
}

static inline int os_vfs_make_rmdir_reply(os_ipc_payload_t* payload, int32_t status, uint32_t request_id) {
    uint32_t i;
    uint32_t raw = (uint32_t)status;
    if (!payload) return OS_VFS_STATUS_INVALID;
    payload->type = OS_IPC_VFS_RMDIR_REPLY; payload->size = OS_VFS_RMDIR_REPLY_SIZE; payload->request_id = request_id;
    payload->data[0] = (uint8_t)(raw & 0xffU); payload->data[1] = (uint8_t)((raw >> 8) & 0xffU);
    payload->data[2] = (uint8_t)((raw >> 16) & 0xffU); payload->data[3] = (uint8_t)((raw >> 24) & 0xffU);
    for (i = OS_VFS_RMDIR_REPLY_SIZE; i < OS_IPC_MAX_DATA; i++) payload->data[i] = 0U;
    return 0;
}

static inline int os_vfs_parse_rmdir_reply(const os_ipc_message_t* message, int32_t* status_out, uint32_t expected_request_id) {
    if (!message || !status_out || message->type != OS_IPC_VFS_RMDIR_REPLY ||
        message->size != OS_VFS_RMDIR_REPLY_SIZE || message->request_id != expected_request_id) return OS_VFS_STATUS_INVALID;
    *status_out = (int32_t)((uint32_t)message->data[0] | ((uint32_t)message->data[1] << 8) |
                            ((uint32_t)message->data[2] << 16) | ((uint32_t)message->data[3] << 24));
    return 0;
}

static inline int os_vfs_make_list_observe_request(os_ipc_payload_t* payload, const char* path,
                                                   uint32_t start, uint32_t expected_generation,
                                                   uint32_t request_id) {
    uint32_t i;
    if (!payload || !os_vfs_list_page_path_is_valid(path)) return OS_VFS_STATUS_INVALID;
    payload->type = OS_IPC_VFS_LIST_OBSERVE;
    payload->size = OS_VFS_LIST_OBSERVE_REQUEST_SIZE;
    payload->request_id = request_id;
    for (i = 0U; i < OS_VFS_PATH_MAX; i++) payload->data[i] = (uint8_t)path[i];
    payload->data[48] = (uint8_t)(start & 0xffU); payload->data[49] = (uint8_t)(start >> 8);
    payload->data[50] = (uint8_t)(start >> 16); payload->data[51] = (uint8_t)(start >> 24);
    payload->data[52] = (uint8_t)(expected_generation & 0xffU); payload->data[53] = (uint8_t)(expected_generation >> 8);
    payload->data[54] = (uint8_t)(expected_generation >> 16); payload->data[55] = (uint8_t)(expected_generation >> 24);
    for (i = 56U; i < OS_IPC_MAX_DATA; i++) payload->data[i] = 0U;
    return 0;
}

static inline int os_vfs_parse_list_observe_request(const os_ipc_message_t* message, char* path_out,
                                                    uint32_t* start_out, uint32_t* generation_out) {
    uint32_t i;
    if (!message || !path_out || !start_out || !generation_out || message->type != OS_IPC_VFS_LIST_OBSERVE ||
        message->size != OS_VFS_LIST_OBSERVE_REQUEST_SIZE) return OS_VFS_STATUS_INVALID;
    for (i = 0U; i < OS_VFS_PATH_MAX; i++) path_out[i] = (char)message->data[i];
    if (!os_vfs_list_page_path_is_valid(path_out)) return OS_VFS_STATUS_INVALID;
    *start_out = (uint32_t)message->data[48] | ((uint32_t)message->data[49] << 8) |
                 ((uint32_t)message->data[50] << 16) | ((uint32_t)message->data[51] << 24);
    *generation_out = (uint32_t)message->data[52] | ((uint32_t)message->data[53] << 8) |
                      ((uint32_t)message->data[54] << 16) | ((uint32_t)message->data[55] << 24);
    return 0;
}


static inline int os_vfs_make_list_observe_reply(os_ipc_payload_t* payload, int32_t status,
                                                  uint32_t count, uint32_t next_start, uint32_t generation,
                                                  const uint8_t* data, uint32_t size, uint32_t request_id) {
    uint32_t i;
    if (!payload || count > OS_VFS_LIST_ENTRY_MAX || size > OS_VFS_LIST_OBSERVE_DATA_MAX ||
        (size > 0U && !data)) return OS_VFS_STATUS_INVALID;
    payload->type = OS_IPC_VFS_LIST_OBSERVE_REPLY; payload->size = OS_VFS_LIST_OBSERVE_REPLY_SIZE;
    payload->request_id = request_id;
    os_vfs_encode_i32(&payload->data[0], status); os_vfs_encode_u32(&payload->data[4], count);
    os_vfs_encode_u32(&payload->data[8], next_start); os_vfs_encode_u32(&payload->data[12], generation);
    for (i = 0U; i < size; i++) payload->data[16U + i] = data[i];
    for (; i < OS_VFS_LIST_OBSERVE_DATA_MAX; i++) payload->data[16U + i] = 0U;
    for (i = OS_VFS_LIST_OBSERVE_REPLY_SIZE; i < OS_IPC_MAX_DATA; i++) payload->data[i] = 0U;
    return 0;
}

static inline int os_vfs_parse_list_observe_reply(const os_ipc_message_t* message,
                                                  os_vfs_list_observe_reply_t* reply_out,
                                                  uint32_t expected_request_id) {
    uint32_t i;
    if (!message || !reply_out || message->type != OS_IPC_VFS_LIST_OBSERVE_REPLY ||
        message->size != OS_VFS_LIST_OBSERVE_REPLY_SIZE || message->request_id != expected_request_id) return OS_VFS_STATUS_INVALID;
    reply_out->status = os_vfs_decode_i32(&message->data[0]); reply_out->count = os_vfs_decode_u32(&message->data[4]);
    reply_out->next_start = os_vfs_decode_u32(&message->data[8]); reply_out->generation = os_vfs_decode_u32(&message->data[12]);
    if (reply_out->count > OS_VFS_LIST_ENTRY_MAX) return OS_VFS_STATUS_INVALID;
    for (i = 0U; i < OS_VFS_LIST_OBSERVE_DATA_MAX; i++) reply_out->data[i] = message->data[16U + i];
    return 0;
}

static inline int os_vfs_make_list_page_reply(os_ipc_payload_t* payload, int32_t status,
                                               uint32_t count, uint32_t next_start,
                                               const uint8_t* data, uint32_t size,
                                               uint32_t request_id) {
    uint32_t i;
    if (!payload || count > OS_VFS_LIST_ENTRY_MAX || size > OS_VFS_LIST_PAGE_DATA_MAX ||
        (size > 0U && !data)) return OS_VFS_STATUS_INVALID;
    payload->type = OS_IPC_VFS_LIST_PAGE_REPLY;
    payload->size = OS_VFS_LIST_PAGE_REPLY_SIZE;
    payload->request_id = request_id;
    os_vfs_encode_i32(&payload->data[0], status);
    os_vfs_encode_u32(&payload->data[4], count);
    os_vfs_encode_u32(&payload->data[8], next_start);
    for (i = 0U; i < size; i++) payload->data[12U + i] = data[i];
    for (; i < OS_VFS_LIST_PAGE_DATA_MAX; i++) payload->data[12U + i] = 0U;
    for (i = OS_VFS_LIST_PAGE_REPLY_SIZE; i < OS_IPC_MAX_DATA; i++) payload->data[i] = 0U;
    return 0;
}

static inline int os_vfs_parse_list_page_reply(const os_ipc_message_t* message,
                                                os_vfs_list_page_reply_t* reply_out,
                                                uint32_t expected_request_id) {
    uint32_t i;
    if (!message || !reply_out || message->type != OS_IPC_VFS_LIST_PAGE_REPLY ||
        message->size != OS_VFS_LIST_PAGE_REPLY_SIZE || message->request_id != expected_request_id) {
        return OS_VFS_STATUS_INVALID;
    }
    reply_out->status = os_vfs_decode_i32(&message->data[0]);
    reply_out->count = os_vfs_decode_u32(&message->data[4]);
    reply_out->next_start = os_vfs_decode_u32(&message->data[8]);
    if (reply_out->count > OS_VFS_LIST_ENTRY_MAX) return OS_VFS_STATUS_INVALID;
    for (i = 0U; i < OS_VFS_LIST_PAGE_DATA_MAX; i++) reply_out->data[i] = message->data[12U + i];
    return 0;
}

static inline int os_vfs_make_list_reply(os_ipc_payload_t* payload, int32_t status,
                                          uint32_t count, const uint8_t* data, uint32_t size,
                                          uint32_t request_id) {
    uint32_t i;
    if (!payload || count > OS_VFS_LIST_ENTRY_MAX || size > OS_VFS_LIST_DATA_MAX ||
        (size > 0U && !data)) {
        return OS_VFS_STATUS_INVALID;
    }
    payload->type = OS_IPC_VFS_LIST_REPLY;
    payload->size = OS_VFS_LIST_REPLY_SIZE;
    payload->request_id = request_id;
    os_vfs_encode_i32(&payload->data[0], status);
    os_vfs_encode_u32(&payload->data[4], count);
    for (i = 0U; i < size; i++) payload->data[8U + i] = data[i];
    for (; i < OS_VFS_LIST_DATA_MAX; i++) payload->data[8U + i] = 0U;
    for (i = OS_VFS_LIST_REPLY_SIZE; i < OS_IPC_MAX_DATA; i++) payload->data[i] = 0U;
    return 0;
}

static inline int os_vfs_parse_list_reply(const os_ipc_message_t* message,
                                          os_vfs_list_reply_t* reply_out,
                                          uint32_t expected_request_id) {
    uint32_t i;
    if (!message || !reply_out || message->type != OS_IPC_VFS_LIST_REPLY ||
        message->size != OS_VFS_LIST_REPLY_SIZE || message->request_id != expected_request_id) {
        return OS_VFS_STATUS_INVALID;
    }
    reply_out->status = os_vfs_decode_i32(&message->data[0]);
    reply_out->count = os_vfs_decode_u32(&message->data[4]);
    if (reply_out->count > OS_VFS_LIST_ENTRY_MAX) return OS_VFS_STATUS_INVALID;
    for (i = 0U; i < OS_VFS_LIST_DATA_MAX; i++) reply_out->data[i] = message->data[8U + i];
    return 0;
}

static inline int os_vfs_make_write_request(os_ipc_payload_t* payload, const char* path,
                                            const uint8_t* data, uint32_t size,
                                            uint32_t request_id) {
    uint32_t i;
    if (!payload || !os_vfs_path_is_safe(path) || size > OS_VFS_WRITE_MAX ||
        (size > 0U && !data)) return OS_VFS_STATUS_INVALID;
    payload->type = OS_IPC_VFS_WRITE;
    payload->size = OS_VFS_WRITE_REQUEST_SIZE;
    payload->request_id = request_id;
    for (i = 0U; i < OS_VFS_PATH_MAX; i++) {
        payload->data[i] = (uint8_t)path[i];
        if (path[i] == '\0') {
            for (i++; i < OS_VFS_PATH_MAX; i++) payload->data[i] = 0U;
            break;
        }
    }
    os_vfs_encode_u32(&payload->data[OS_VFS_PATH_MAX], size);
    for (i = 0U; i < size; i++) payload->data[OS_VFS_PATH_MAX + 4U + i] = data[i];
    for (; i < OS_VFS_WRITE_MAX; i++) payload->data[OS_VFS_PATH_MAX + 4U + i] = 0U;
    return 0;
}

static inline int os_vfs_parse_write_request(const os_ipc_message_t* message,
                                             char* path_out, uint8_t* data_out,
                                             uint32_t* size_out) {
    uint32_t i;
    uint32_t size;
    if (!message || !path_out || !data_out || !size_out || message->type != OS_IPC_VFS_WRITE ||
        message->size != OS_VFS_WRITE_REQUEST_SIZE) return OS_VFS_STATUS_INVALID;
    for (i = 0U; i < OS_VFS_PATH_MAX; i++) path_out[i] = (char)message->data[i];
    if (!os_vfs_path_is_safe(path_out)) return OS_VFS_STATUS_INVALID;
    size = os_vfs_decode_u32(&message->data[OS_VFS_PATH_MAX]);
    if (size > OS_VFS_WRITE_MAX) return OS_VFS_STATUS_INVALID;
    for (i = 0U; i < size; i++) data_out[i] = message->data[OS_VFS_PATH_MAX + 4U + i];
    *size_out = size;
    return 0;
}

static inline int os_vfs_make_write_reply(os_ipc_payload_t* payload, int32_t status,
                                          uint32_t request_id) {
    uint32_t i;
    if (!payload) return OS_VFS_STATUS_INVALID;
    payload->type = OS_IPC_VFS_WRITE_REPLY;
    payload->size = OS_VFS_WRITE_REPLY_SIZE;
    payload->request_id = request_id;
    os_vfs_encode_i32(&payload->data[0], status);
    for (i = OS_VFS_WRITE_REPLY_SIZE; i < OS_IPC_MAX_DATA; i++) payload->data[i] = 0U;
    return 0;
}

static inline int os_vfs_parse_write_reply(const os_ipc_message_t* message,
                                           os_vfs_write_reply_t* reply_out,
                                           uint32_t expected_request_id) {
    if (!message || !reply_out || message->type != OS_IPC_VFS_WRITE_REPLY ||
        message->size != OS_VFS_WRITE_REPLY_SIZE || message->request_id != expected_request_id) {
        return OS_VFS_STATUS_INVALID;
    }
    reply_out->status = os_vfs_decode_i32(&message->data[0]);
    return 0;
}

static inline int os_vfs_make_remove_request(os_ipc_payload_t* payload, const char* path,
                                             uint32_t request_id) {
    uint32_t i;
    if (!payload || !os_vfs_path_is_safe(path)) return OS_VFS_STATUS_INVALID;
    payload->type = OS_IPC_VFS_REMOVE;
    payload->size = OS_VFS_PATH_MAX;
    payload->request_id = request_id;
    for (i = 0U; i < OS_VFS_PATH_MAX; i++) {
        payload->data[i] = (uint8_t)path[i];
        if (path[i] == '\0') {
            for (i++; i < OS_IPC_MAX_DATA; i++) payload->data[i] = 0U;
            break;
        }
    }
    return 0;
}

static inline int os_vfs_parse_remove_request(const os_ipc_message_t* message,
                                              char* path_out) {
    uint32_t i;
    if (!message || !path_out || message->type != OS_IPC_VFS_REMOVE ||
        message->size != OS_VFS_PATH_MAX) return OS_VFS_STATUS_INVALID;
    for (i = 0U; i < OS_VFS_PATH_MAX; i++) path_out[i] = (char)message->data[i];
    return os_vfs_path_is_safe(path_out) ? 0 : OS_VFS_STATUS_INVALID;
}

static inline int os_vfs_make_remove_reply(os_ipc_payload_t* payload, int32_t status,
                                           uint32_t request_id) {
    uint32_t i;
    if (!payload) return OS_VFS_STATUS_INVALID;
    payload->type = OS_IPC_VFS_REMOVE_REPLY;
    payload->size = OS_VFS_WRITE_REPLY_SIZE;
    payload->request_id = request_id;
    os_vfs_encode_i32(&payload->data[0], status);
    for (i = OS_VFS_WRITE_REPLY_SIZE; i < OS_IPC_MAX_DATA; i++) payload->data[i] = 0U;
    return 0;
}

static inline int os_vfs_parse_remove_reply(const os_ipc_message_t* message,
                                            os_vfs_remove_reply_t* reply_out,
                                            uint32_t expected_request_id) {
    if (!message || !reply_out || message->type != OS_IPC_VFS_REMOVE_REPLY ||
        message->size != OS_VFS_WRITE_REPLY_SIZE || message->request_id != expected_request_id) {
        return OS_VFS_STATUS_INVALID;
    }
    reply_out->status = os_vfs_decode_i32(&message->data[0]);
    return 0;
}

static inline int os_vfs_make_rename_request(os_ipc_payload_t* payload,
                                             const char* old_path, const char* new_path,
                                             uint32_t request_id) {
    uint32_t i;
    if (!payload || !os_vfs_path_is_safe(old_path) || !os_vfs_path_is_safe(new_path)) {
        return OS_VFS_STATUS_INVALID;
    }
    payload->type = OS_IPC_VFS_RENAME;
    payload->size = OS_VFS_RENAME_REQUEST_SIZE;
    payload->request_id = request_id;
    for (i = 0U; i < OS_VFS_PATH_MAX; i++) {
        payload->data[i] = (uint8_t)old_path[i];
        if (old_path[i] == '\0') {
            for (i++; i < OS_VFS_PATH_MAX; i++) payload->data[i] = 0U;
            break;
        }
    }
    for (i = 0U; i < OS_VFS_PATH_MAX; i++) {
        payload->data[OS_VFS_PATH_MAX + i] = (uint8_t)new_path[i];
        if (new_path[i] == '\0') {
            for (i++; i < OS_VFS_PATH_MAX; i++) payload->data[OS_VFS_PATH_MAX + i] = 0U;
            break;
        }
    }
    return 0;
}

static inline int os_vfs_parse_rename_request(const os_ipc_message_t* message,
                                              char* old_path_out, char* new_path_out) {
    uint32_t i;
    if (!message || !old_path_out || !new_path_out || message->type != OS_IPC_VFS_RENAME ||
        message->size != OS_VFS_RENAME_REQUEST_SIZE) return OS_VFS_STATUS_INVALID;
    for (i = 0U; i < OS_VFS_PATH_MAX; i++) {
        old_path_out[i] = (char)message->data[i];
        new_path_out[i] = (char)message->data[OS_VFS_PATH_MAX + i];
    }
    return os_vfs_path_is_safe(old_path_out) && os_vfs_path_is_safe(new_path_out)
        ? 0 : OS_VFS_STATUS_INVALID;
}

static inline int os_vfs_make_rename_reply(os_ipc_payload_t* payload, int32_t status,
                                           uint32_t request_id) {
    uint32_t i;
    if (!payload) return OS_VFS_STATUS_INVALID;
    payload->type = OS_IPC_VFS_RENAME_REPLY;
    payload->size = OS_VFS_WRITE_REPLY_SIZE;
    payload->request_id = request_id;
    os_vfs_encode_i32(&payload->data[0], status);
    for (i = OS_VFS_WRITE_REPLY_SIZE; i < OS_IPC_MAX_DATA; i++) payload->data[i] = 0U;
    return 0;
}

static inline int os_vfs_parse_rename_reply(const os_ipc_message_t* message,
                                            os_vfs_rename_reply_t* reply_out,
                                            uint32_t expected_request_id) {
    if (!message || !reply_out || message->type != OS_IPC_VFS_RENAME_REPLY ||
        message->size != OS_VFS_WRITE_REPLY_SIZE || message->request_id != expected_request_id) {
        return OS_VFS_STATUS_INVALID;
    }
    reply_out->status = os_vfs_decode_i32(&message->data[0]);
    return 0;
}

static inline int os_vfs_make_mount_add_request(os_ipc_payload_t* payload,
                                                const char* mount, uint32_t source,
                                                uint32_t request_id) {
    uint32_t i;
    if (!payload || !os_vfs_mount_prefix_is_valid(mount) || !os_vfs_mount_source_is_valid(source)) {
        return OS_VFS_STATUS_INVALID;
    }
    payload->type = OS_IPC_VFS_MOUNT_ADD;
    payload->size = OS_VFS_MOUNT_ADD_REQUEST_SIZE;
    payload->request_id = request_id;
    for (i = 0U; i < OS_VFS_PATH_MAX; i++) payload->data[i] = (uint8_t)mount[i];
    os_vfs_encode_u32(&payload->data[OS_VFS_PATH_MAX], source);
    for (i = OS_VFS_MOUNT_ADD_REQUEST_SIZE; i < OS_IPC_MAX_DATA; i++) payload->data[i] = 0U;
    return 0;
}

static inline int os_vfs_parse_mount_add_request(const os_ipc_message_t* message,
                                                 char* mount_out, uint32_t* source_out) {
    uint32_t i;
    if (!message || !mount_out || !source_out || message->type != OS_IPC_VFS_MOUNT_ADD ||
        message->size != OS_VFS_MOUNT_ADD_REQUEST_SIZE) return OS_VFS_STATUS_INVALID;
    for (i = 0U; i < OS_VFS_PATH_MAX; i++) mount_out[i] = (char)message->data[i];
    *source_out = os_vfs_decode_u32(&message->data[OS_VFS_PATH_MAX]);
    return os_vfs_mount_prefix_is_valid(mount_out) && os_vfs_mount_source_is_valid(*source_out)
        ? 0 : OS_VFS_STATUS_INVALID;
}

static inline int os_vfs_make_mount_remove_request(os_ipc_payload_t* payload,
                                                    const char* mount, uint32_t request_id) {
    uint32_t i;
    if (!payload || !os_vfs_mount_prefix_is_valid(mount)) return OS_VFS_STATUS_INVALID;
    payload->type = OS_IPC_VFS_MOUNT_REMOVE;
    payload->size = OS_VFS_MOUNT_REMOVE_REQUEST_SIZE;
    payload->request_id = request_id;
    for (i = 0U; i < OS_VFS_PATH_MAX; i++) payload->data[i] = (uint8_t)mount[i];
    for (i = OS_VFS_PATH_MAX; i < OS_IPC_MAX_DATA; i++) payload->data[i] = 0U;
    return 0;
}

static inline int os_vfs_parse_mount_remove_request(const os_ipc_message_t* message,
                                                    char* mount_out) {
    uint32_t i;
    if (!message || !mount_out || message->type != OS_IPC_VFS_MOUNT_REMOVE ||
        message->size != OS_VFS_MOUNT_REMOVE_REQUEST_SIZE) return OS_VFS_STATUS_INVALID;
    for (i = 0U; i < OS_VFS_PATH_MAX; i++) mount_out[i] = (char)message->data[i];
    return os_vfs_mount_prefix_is_valid(mount_out) ? 0 : OS_VFS_STATUS_INVALID;
}

static inline int os_vfs_make_mount_reply(os_ipc_payload_t* payload, uint32_t type,
                                          int32_t status, uint32_t request_id) {
    uint32_t i;
    if (!payload || (type != OS_IPC_VFS_MOUNT_ADD_REPLY && type != OS_IPC_VFS_MOUNT_REMOVE_REPLY)) {
        return OS_VFS_STATUS_INVALID;
    }
    payload->type = type;
    payload->size = OS_VFS_MOUNT_REPLY_SIZE;
    payload->request_id = request_id;
    os_vfs_encode_i32(&payload->data[0], status);
    for (i = OS_VFS_MOUNT_REPLY_SIZE; i < OS_IPC_MAX_DATA; i++) payload->data[i] = 0U;
    return 0;
}

static inline int os_vfs_parse_mount_reply(const os_ipc_message_t* message, uint32_t type,
                                           os_vfs_mount_reply_t* reply_out,
                                           uint32_t expected_request_id) {
    if (!message || !reply_out ||
        (type != OS_IPC_VFS_MOUNT_ADD_REPLY && type != OS_IPC_VFS_MOUNT_REMOVE_REPLY) ||
        message->type != type || message->size != OS_VFS_MOUNT_REPLY_SIZE ||
        message->request_id != expected_request_id) return OS_VFS_STATUS_INVALID;
    reply_out->status = os_vfs_decode_i32(&message->data[0]);
    return 0;
}

static inline int os_vfs_make_grant_request(os_ipc_payload_t* payload, int32_t target_pid) {
    uint32_t i;
    if (!payload || target_pid <= 0) return OS_VFS_STATUS_INVALID;
    payload->type = OS_IPC_VFS_GRANT;
    payload->size = OS_VFS_GRANT_REQUEST_SIZE;
    payload->request_id = 0U;
    os_vfs_encode_i32(&payload->data[0], target_pid);
    for (i = OS_VFS_GRANT_REQUEST_SIZE; i < OS_IPC_MAX_DATA; i++) payload->data[i] = 0U;
    return 0;
}

static inline int os_vfs_parse_grant_request(const os_ipc_message_t* message, int32_t* target_pid) {
    if (!message || !target_pid || message->type != OS_IPC_VFS_GRANT ||
        message->size != OS_VFS_GRANT_REQUEST_SIZE) return OS_VFS_STATUS_INVALID;
    *target_pid = os_vfs_decode_i32(&message->data[0]);
    return *target_pid > 0 ? 0 : OS_VFS_STATUS_INVALID;
}

static inline int os_vfs_make_backend_grant_request(os_ipc_payload_t* payload, int32_t target_pid,
                                                    uint32_t request_id) {
    uint32_t i;
    if (!payload || target_pid <= 0) return OS_VFS_STATUS_INVALID;
    payload->type = OS_IPC_VFS_BACKEND_GRANT; payload->size = OS_VFS_GRANT_REQUEST_SIZE; payload->request_id = request_id;
    os_vfs_encode_i32(&payload->data[0], target_pid);
    for (i = OS_VFS_GRANT_REQUEST_SIZE; i < OS_IPC_MAX_DATA; i++) payload->data[i] = 0U;
    return 0;
}

static inline int os_vfs_parse_backend_grant_request(const os_ipc_message_t* message, int32_t* target_pid) {
    if (!message || !target_pid || message->type != OS_IPC_VFS_BACKEND_GRANT ||
        message->size != OS_VFS_GRANT_REQUEST_SIZE) return OS_VFS_STATUS_INVALID;
    *target_pid = os_vfs_decode_i32(&message->data[0]);
    return *target_pid > 0 ? 0 : OS_VFS_STATUS_INVALID;
}

static inline int os_vfs_make_backend_grant_reply(os_ipc_payload_t* payload, int32_t status, uint32_t request_id) {
    uint32_t i;
    if (!payload) return OS_VFS_STATUS_INVALID;
    payload->type = OS_IPC_VFS_BACKEND_GRANT_REPLY; payload->size = OS_VFS_BACKEND_GRANT_REPLY_SIZE; payload->request_id = request_id;
    os_vfs_encode_i32(&payload->data[0], status);
    for (i = OS_VFS_BACKEND_GRANT_REPLY_SIZE; i < OS_IPC_MAX_DATA; i++) payload->data[i] = 0U;
    return 0;
}

static inline int os_vfs_parse_backend_grant_reply(const os_ipc_message_t* message, int32_t* status_out, uint32_t expected_request_id) {
    if (!message || !status_out || message->type != OS_IPC_VFS_BACKEND_GRANT_REPLY ||
        message->size != OS_VFS_BACKEND_GRANT_REPLY_SIZE || message->request_id != expected_request_id) return OS_VFS_STATUS_INVALID;
    *status_out = os_vfs_decode_i32(&message->data[0]);
    return 0;
}

static inline int os_vfs_make_backend_revoke_request(os_ipc_payload_t* payload, int32_t target_pid,
                                                     uint32_t request_id) {
    uint32_t i;
    if (!payload || target_pid <= 0) return OS_VFS_STATUS_INVALID;
    payload->type = OS_IPC_VFS_BACKEND_REVOKE; payload->size = OS_VFS_GRANT_REQUEST_SIZE; payload->request_id = request_id;
    os_vfs_encode_i32(&payload->data[0], target_pid);
    for (i = OS_VFS_GRANT_REQUEST_SIZE; i < OS_IPC_MAX_DATA; i++) payload->data[i] = 0U;
    return 0;
}

static inline int os_vfs_parse_backend_revoke_request(const os_ipc_message_t* message, int32_t* target_pid) {
    if (!message || !target_pid || message->type != OS_IPC_VFS_BACKEND_REVOKE ||
        message->size != OS_VFS_GRANT_REQUEST_SIZE) return OS_VFS_STATUS_INVALID;
    *target_pid = os_vfs_decode_i32(&message->data[0]);
    return *target_pid > 0 ? 0 : OS_VFS_STATUS_INVALID;
}

static inline int os_vfs_make_backend_revoke_reply(os_ipc_payload_t* payload, int32_t status, uint32_t request_id) {
    uint32_t i;
    if (!payload) return OS_VFS_STATUS_INVALID;
    payload->type = OS_IPC_VFS_BACKEND_REVOKE_REPLY; payload->size = OS_VFS_BACKEND_REVOKE_REPLY_SIZE; payload->request_id = request_id;
    os_vfs_encode_i32(&payload->data[0], status);
    for (i = OS_VFS_BACKEND_REVOKE_REPLY_SIZE; i < OS_IPC_MAX_DATA; i++) payload->data[i] = 0U;
    return 0;
}

static inline int os_vfs_parse_backend_revoke_reply(const os_ipc_message_t* message, int32_t* status_out, uint32_t expected_request_id) {
    if (!message || !status_out || message->type != OS_IPC_VFS_BACKEND_REVOKE_REPLY ||
        message->size != OS_VFS_BACKEND_REVOKE_REPLY_SIZE || message->request_id != expected_request_id) return OS_VFS_STATUS_INVALID;
    *status_out = os_vfs_decode_i32(&message->data[0]);
    return 0;
}

static inline int os_vfs_make_backend_grant_scoped_request(os_ipc_payload_t* payload, int32_t target_pid,
                                                           uint32_t rights, uint32_t request_id) {
    uint32_t i;
    if (!payload || target_pid <= 0 || rights == 0U || (rights & ~OS_VFS_BACKEND_RIGHT_ALL) != 0U) return OS_VFS_STATUS_INVALID;
    payload->type = OS_IPC_VFS_BACKEND_GRANT_SCOPED; payload->size = OS_VFS_BACKEND_GRANT_SCOPED_REQUEST_SIZE; payload->request_id = request_id;
    os_vfs_encode_i32(&payload->data[0], target_pid); os_vfs_encode_u32(&payload->data[4], rights);
    for (i = OS_VFS_BACKEND_GRANT_SCOPED_REQUEST_SIZE; i < OS_IPC_MAX_DATA; i++) payload->data[i] = 0U;
    return 0;
}

static inline int os_vfs_parse_backend_grant_scoped_request(const os_ipc_message_t* message, int32_t* target_pid, uint32_t* rights_out) {
    if (!message || !target_pid || !rights_out || message->type != OS_IPC_VFS_BACKEND_GRANT_SCOPED ||
        message->size != OS_VFS_BACKEND_GRANT_SCOPED_REQUEST_SIZE) return OS_VFS_STATUS_INVALID;
    *target_pid = os_vfs_decode_i32(&message->data[0]); *rights_out = os_vfs_decode_u32(&message->data[4]);
    return *target_pid > 0 && *rights_out != 0U && (*rights_out & ~OS_VFS_BACKEND_RIGHT_ALL) == 0U ? 0 : OS_VFS_STATUS_INVALID;
}

static inline int os_vfs_make_backend_grant_scoped_reply(os_ipc_payload_t* payload, int32_t status, uint32_t request_id) {
    uint32_t i;
    if (!payload) return OS_VFS_STATUS_INVALID;
    payload->type = OS_IPC_VFS_BACKEND_GRANT_SCOPED_REPLY; payload->size = OS_VFS_BACKEND_GRANT_SCOPED_REPLY_SIZE; payload->request_id = request_id;
    os_vfs_encode_i32(&payload->data[0], status);
    for (i = OS_VFS_BACKEND_GRANT_SCOPED_REPLY_SIZE; i < OS_IPC_MAX_DATA; i++) payload->data[i] = 0U;
    return 0;
}

static inline int os_vfs_parse_backend_grant_scoped_reply(const os_ipc_message_t* message, int32_t* status_out, uint32_t expected_request_id) {
    if (!message || !status_out || message->type != OS_IPC_VFS_BACKEND_GRANT_SCOPED_REPLY ||
        message->size != OS_VFS_BACKEND_GRANT_SCOPED_REPLY_SIZE || message->request_id != expected_request_id) return OS_VFS_STATUS_INVALID;
    *status_out = os_vfs_decode_i32(&message->data[0]);
    return 0;
}

static inline int os_vfs_make_backend_status_request(os_ipc_payload_t* payload, int32_t target_pid, uint32_t request_id) {
    uint32_t i;
    if (!payload || target_pid <= 0) return OS_VFS_STATUS_INVALID;
    payload->type = OS_IPC_VFS_BACKEND_STATUS; payload->size = OS_VFS_BACKEND_STATUS_REQUEST_SIZE; payload->request_id = request_id;
    os_vfs_encode_i32(&payload->data[0], target_pid);
    for (i = OS_VFS_BACKEND_STATUS_REQUEST_SIZE; i < OS_IPC_MAX_DATA; i++) payload->data[i] = 0U;
    return 0;
}

static inline int os_vfs_parse_backend_status_request(const os_ipc_message_t* message, int32_t* target_pid) {
    if (!message || !target_pid || message->type != OS_IPC_VFS_BACKEND_STATUS ||
        message->size != OS_VFS_BACKEND_STATUS_REQUEST_SIZE) return OS_VFS_STATUS_INVALID;
    *target_pid = os_vfs_decode_i32(&message->data[0]);
    return *target_pid > 0 ? 0 : OS_VFS_STATUS_INVALID;
}

static inline int os_vfs_make_backend_status_reply(os_ipc_payload_t* payload, int32_t status, uint32_t rights, uint32_t request_id) {
    uint32_t i;
    if (!payload || (status == 0 && (rights == 0U || (rights & ~OS_VFS_BACKEND_RIGHT_ALL) != 0U))) return OS_VFS_STATUS_INVALID;
    payload->type = OS_IPC_VFS_BACKEND_STATUS_REPLY; payload->size = OS_VFS_BACKEND_STATUS_REPLY_SIZE; payload->request_id = request_id;
    os_vfs_encode_i32(&payload->data[0], status); os_vfs_encode_u32(&payload->data[4], rights);
    for (i = OS_VFS_BACKEND_STATUS_REPLY_SIZE; i < OS_IPC_MAX_DATA; i++) payload->data[i] = 0U;
    return 0;
}

static inline int os_vfs_parse_backend_status_reply(const os_ipc_message_t* message, int32_t* status_out, uint32_t* rights_out, uint32_t expected_request_id) {
    if (!message || !status_out || !rights_out || message->type != OS_IPC_VFS_BACKEND_STATUS_REPLY ||
        message->size != OS_VFS_BACKEND_STATUS_REPLY_SIZE || message->request_id != expected_request_id) return OS_VFS_STATUS_INVALID;
    *status_out = os_vfs_decode_i32(&message->data[0]); *rights_out = os_vfs_decode_u32(&message->data[4]);
    if (*status_out == 0 && (*rights_out == 0U || (*rights_out & ~OS_VFS_BACKEND_RIGHT_ALL) != 0U)) return OS_VFS_STATUS_INVALID;
    return 0;
}

static inline int os_vfs_make_backend_scope_request(os_ipc_payload_t* payload, int32_t target_pid,
                                                     uint32_t request_id) {
    uint32_t i;
    if (!payload || target_pid <= 0) return OS_VFS_STATUS_INVALID;
    payload->type = OS_IPC_VFS_BACKEND_SCOPE;
    payload->size = OS_VFS_BACKEND_SCOPE_REQUEST_SIZE;
    payload->request_id = request_id;
    os_vfs_encode_i32(&payload->data[0], target_pid);
    for (i = OS_VFS_BACKEND_SCOPE_REQUEST_SIZE; i < OS_IPC_MAX_DATA; i++) payload->data[i] = 0U;
    return 0;
}

static inline int os_vfs_parse_backend_scope_request(const os_ipc_message_t* message,
                                                      int32_t* target_pid_out) {
    if (!message || !target_pid_out || message->type != OS_IPC_VFS_BACKEND_SCOPE ||
        message->size != OS_VFS_BACKEND_SCOPE_REQUEST_SIZE) return OS_VFS_STATUS_INVALID;
    *target_pid_out = os_vfs_decode_i32(&message->data[0]);
    return *target_pid_out > 0 ? 0 : OS_VFS_STATUS_INVALID;
}

static inline int os_vfs_make_backend_scope_reply(os_ipc_payload_t* payload, int32_t status,
                                                   uint32_t rights, uint32_t sources,
                                                   uint32_t request_id) {
    uint32_t i;
    if (!payload || (status == 0 && (rights == 0U ||
        (rights & ~OS_VFS_BACKEND_RIGHT_ALL) != 0U || sources == 0U ||
        (sources & ~OS_SERVICE_BACKEND_SOURCE_ALL) != 0U))) return OS_VFS_STATUS_INVALID;
    if (status != 0) { rights = 0U; sources = 0U; }
    payload->type = OS_IPC_VFS_BACKEND_SCOPE_REPLY;
    payload->size = OS_VFS_BACKEND_SCOPE_REPLY_SIZE;
    payload->request_id = request_id;
    os_vfs_encode_i32(&payload->data[0], status);
    os_vfs_encode_u32(&payload->data[4], rights);
    os_vfs_encode_u32(&payload->data[8], sources);
    for (i = OS_VFS_BACKEND_SCOPE_REPLY_SIZE; i < OS_IPC_MAX_DATA; i++) payload->data[i] = 0U;
    return 0;
}

static inline int os_vfs_parse_backend_scope_reply(const os_ipc_message_t* message,
                                                    os_vfs_backend_scope_reply_t* reply_out,
                                                    uint32_t expected_request_id) {
    if (!message || !reply_out || message->type != OS_IPC_VFS_BACKEND_SCOPE_REPLY ||
        message->size != OS_VFS_BACKEND_SCOPE_REPLY_SIZE ||
        message->request_id != expected_request_id) return OS_VFS_STATUS_INVALID;
    reply_out->status = os_vfs_decode_i32(&message->data[0]);
    reply_out->rights = os_vfs_decode_u32(&message->data[4]);
    reply_out->sources = os_vfs_decode_u32(&message->data[8]);
    if (reply_out->status == 0 && (reply_out->rights == 0U ||
        (reply_out->rights & ~OS_VFS_BACKEND_RIGHT_ALL) != 0U || reply_out->sources == 0U ||
        (reply_out->sources & ~OS_SERVICE_BACKEND_SOURCE_ALL) != 0U)) return OS_VFS_STATUS_INVALID;
    if (reply_out->status != 0 && (reply_out->rights != 0U || reply_out->sources != 0U))
        return OS_VFS_STATUS_INVALID;
    return 0;
}

static inline int os_vfs_make_backend_list_request(os_ipc_payload_t* payload, uint32_t request_id) {
    uint32_t i;
    if (!payload) return OS_VFS_STATUS_INVALID;
    payload->type = OS_IPC_VFS_BACKEND_LIST;
    payload->size = OS_VFS_BACKEND_LIST_REQUEST_SIZE;
    payload->request_id = request_id;
    for (i = 0U; i < OS_IPC_MAX_DATA; i++) payload->data[i] = 0U;
    return 0;
}

static inline int os_vfs_parse_backend_list_request(const os_ipc_message_t* message) {
    if (!message || message->type != OS_IPC_VFS_BACKEND_LIST ||
        message->size != OS_VFS_BACKEND_LIST_REQUEST_SIZE) return OS_VFS_STATUS_INVALID;
    return 0;
}

static inline int os_vfs_make_backend_list_reply(os_ipc_payload_t* payload, int32_t status,
                                                  const os_service_backend_list_t* list,
                                                  uint32_t request_id) {
    uint32_t i;
    uint32_t count = 0U;
    if (!payload || (status == 0 && !list)) return OS_VFS_STATUS_INVALID;
    if (status == 0) {
        if (list->count > OS_SERVICE_BACKEND_CAPACITY) return OS_VFS_STATUS_INVALID;
        count = list->count;
        for (i = 0U; i < count; i++) {
            if (list->entries[i].pid <= 0 || list->entries[i].rights == 0U ||
                (list->entries[i].rights & ~OS_VFS_BACKEND_RIGHT_ALL) != 0U) return OS_VFS_STATUS_INVALID;
        }
    }
    payload->type = OS_IPC_VFS_BACKEND_LIST_REPLY;
    payload->size = OS_VFS_BACKEND_LIST_REPLY_SIZE;
    payload->request_id = request_id;
    os_vfs_encode_i32(&payload->data[0], status);
    os_vfs_encode_u32(&payload->data[4], count);
    for (i = 0U; i < OS_SERVICE_BACKEND_CAPACITY; i++) {
        int32_t pid = i < count ? list->entries[i].pid : 0;
        uint32_t rights = i < count ? list->entries[i].rights : 0U;
        os_vfs_encode_i32(&payload->data[8U + i * 8U], pid);
        os_vfs_encode_u32(&payload->data[12U + i * 8U], rights);
    }
    for (i = OS_VFS_BACKEND_LIST_REPLY_SIZE; i < OS_IPC_MAX_DATA; i++) payload->data[i] = 0U;
    return 0;
}

static inline int os_vfs_parse_backend_list_reply(const os_ipc_message_t* message,
                                                   os_vfs_backend_list_reply_t* reply_out,
                                                   uint32_t expected_request_id) {
    uint32_t i;
    if (!message || !reply_out || message->type != OS_IPC_VFS_BACKEND_LIST_REPLY ||
        message->size != OS_VFS_BACKEND_LIST_REPLY_SIZE || message->request_id != expected_request_id) return OS_VFS_STATUS_INVALID;
    reply_out->status = os_vfs_decode_i32(&message->data[0]);
    reply_out->count = os_vfs_decode_u32(&message->data[4]);
    if (reply_out->count > OS_SERVICE_BACKEND_CAPACITY || (reply_out->status != 0 && reply_out->count != 0U)) return OS_VFS_STATUS_INVALID;
    for (i = 0U; i < OS_SERVICE_BACKEND_CAPACITY; i++) {
        reply_out->entries[i].pid = os_vfs_decode_i32(&message->data[8U + i * 8U]);
        reply_out->entries[i].rights = os_vfs_decode_u32(&message->data[12U + i * 8U]);
        if (i < reply_out->count && (reply_out->entries[i].pid <= 0 || reply_out->entries[i].rights == 0U ||
            (reply_out->entries[i].rights & ~OS_VFS_BACKEND_RIGHT_ALL) != 0U)) return OS_VFS_STATUS_INVALID;
        if (i >= reply_out->count && (reply_out->entries[i].pid != 0 || reply_out->entries[i].rights != 0U)) return OS_VFS_STATUS_INVALID;
    }
    return 0;
}

static inline int os_vfs_make_backend_observe_request(os_ipc_payload_t* payload,
                                                       uint32_t expected_generation,
                                                       uint32_t request_id) {
    uint32_t i;
    if (!payload) return OS_VFS_STATUS_INVALID;
    payload->type = OS_IPC_VFS_BACKEND_OBSERVE;
    payload->size = OS_VFS_BACKEND_OBSERVE_REQUEST_SIZE;
    payload->request_id = request_id;
    os_vfs_encode_u32(&payload->data[0], expected_generation);
    for (i = OS_VFS_BACKEND_OBSERVE_REQUEST_SIZE; i < OS_IPC_MAX_DATA; i++) payload->data[i] = 0U;
    return 0;
}

static inline int os_vfs_parse_backend_observe_request(const os_ipc_message_t* message,
                                                        uint32_t* expected_generation_out) {
    if (!message || !expected_generation_out || message->type != OS_IPC_VFS_BACKEND_OBSERVE ||
        message->size != OS_VFS_BACKEND_OBSERVE_REQUEST_SIZE) return OS_VFS_STATUS_INVALID;
    *expected_generation_out = os_vfs_decode_u32(&message->data[0]);
    return 0;
}

static inline int os_vfs_make_backend_observe_reply(os_ipc_payload_t* payload, int32_t status,
                                                     const os_service_backend_snapshot_t* snapshot,
                                                     uint32_t request_id) {
    uint32_t i, count = 0U, generation = 0U;
    if (!payload || !snapshot) return OS_VFS_STATUS_INVALID;
    generation = snapshot->generation;
    if (status == 0) {
        if (snapshot->list.count > OS_SERVICE_BACKEND_CAPACITY) return OS_VFS_STATUS_INVALID;
        count = snapshot->list.count;
        for (i = 0U; i < count; i++) {
            if (snapshot->list.entries[i].pid <= 0 || snapshot->list.entries[i].rights == 0U ||
                (snapshot->list.entries[i].rights & ~OS_VFS_BACKEND_RIGHT_ALL) != 0U) return OS_VFS_STATUS_INVALID;
        }
    }
    payload->type = OS_IPC_VFS_BACKEND_OBSERVE_REPLY;
    payload->size = OS_VFS_BACKEND_OBSERVE_REPLY_SIZE;
    payload->request_id = request_id;
    os_vfs_encode_i32(&payload->data[0], status);
    os_vfs_encode_u32(&payload->data[4], generation);
    os_vfs_encode_u32(&payload->data[8], count);
    for (i = 0U; i < OS_SERVICE_BACKEND_CAPACITY; i++) {
        int32_t pid = i < count ? snapshot->list.entries[i].pid : 0;
        uint32_t rights = i < count ? snapshot->list.entries[i].rights : 0U;
        os_vfs_encode_i32(&payload->data[12U + i * 8U], pid);
        os_vfs_encode_u32(&payload->data[16U + i * 8U], rights);
    }
    for (i = OS_VFS_BACKEND_OBSERVE_REPLY_SIZE; i < OS_IPC_MAX_DATA; i++) payload->data[i] = 0U;
    return 0;
}

static inline int os_vfs_parse_backend_observe_reply(const os_ipc_message_t* message,
                                                      os_vfs_backend_observe_reply_t* reply_out,
                                                      uint32_t expected_request_id) {
    uint32_t i;
    if (!message || !reply_out || message->type != OS_IPC_VFS_BACKEND_OBSERVE_REPLY ||
        message->size != OS_VFS_BACKEND_OBSERVE_REPLY_SIZE || message->request_id != expected_request_id) return OS_VFS_STATUS_INVALID;
    reply_out->status = os_vfs_decode_i32(&message->data[0]);
    reply_out->generation = os_vfs_decode_u32(&message->data[4]);
    reply_out->count = os_vfs_decode_u32(&message->data[8]);
    if (reply_out->count > OS_SERVICE_BACKEND_CAPACITY || (reply_out->status != 0 && reply_out->count != 0U)) return OS_VFS_STATUS_INVALID;
    for (i = 0U; i < OS_SERVICE_BACKEND_CAPACITY; i++) {
        reply_out->entries[i].pid = os_vfs_decode_i32(&message->data[12U + i * 8U]);
        reply_out->entries[i].rights = os_vfs_decode_u32(&message->data[16U + i * 8U]);
        if (i < reply_out->count && (reply_out->entries[i].pid <= 0 || reply_out->entries[i].rights == 0U ||
            (reply_out->entries[i].rights & ~OS_VFS_BACKEND_RIGHT_ALL) != 0U)) return OS_VFS_STATUS_INVALID;
        if (i >= reply_out->count && (reply_out->entries[i].pid != 0 || reply_out->entries[i].rights != 0U)) return OS_VFS_STATUS_INVALID;
    }
    return 0;
}

static inline int os_vfs_make_stat_request(os_ipc_payload_t* payload, const char* path,
                                           uint32_t request_id) {
    uint32_t i;
    if (!payload || !os_vfs_path_is_safe(path)) return OS_VFS_STATUS_INVALID;
    payload->type = OS_IPC_VFS_STAT;
    payload->size = OS_VFS_PATH_MAX;
    payload->request_id = request_id;
    for (i = 0U; i < OS_VFS_PATH_MAX; i++) {
        payload->data[i] = (uint8_t)path[i];
        if (path[i] == '\0') break;
    }
    for (; i < OS_IPC_MAX_DATA; i++) payload->data[i] = 0U;
    return 0;
}

static inline int os_vfs_parse_stat_request(const os_ipc_message_t* message, char* path_out) {
    uint32_t i;
    if (!message || !path_out || message->type != OS_IPC_VFS_STAT ||
        message->size != OS_VFS_PATH_MAX) return OS_VFS_STATUS_INVALID;
    for (i = 0U; i < OS_VFS_PATH_MAX; i++) path_out[i] = (char)message->data[i];
    return os_vfs_path_is_safe(path_out) ? 0 : OS_VFS_STATUS_INVALID;
}

static inline int os_vfs_make_stat_reply(os_ipc_payload_t* payload, int32_t status,
                                         uint32_t size, uint32_t flags,
                                         uint32_t request_id) {
    uint32_t i;
    if (!payload) return OS_VFS_STATUS_INVALID;
    payload->type = OS_IPC_VFS_STAT_REPLY;
    payload->size = OS_VFS_STAT_REPLY_SIZE;
    payload->request_id = request_id;
    os_vfs_encode_i32(&payload->data[0], status);
    os_vfs_encode_u32(&payload->data[4], size);
    os_vfs_encode_u32(&payload->data[8], flags);
    for (i = OS_VFS_STAT_REPLY_SIZE; i < OS_IPC_MAX_DATA; i++) payload->data[i] = 0U;
    return 0;
}

static inline int os_vfs_parse_stat_reply(const os_ipc_message_t* message,
                                          os_vfs_stat_reply_t* reply_out,
                                          uint32_t expected_request_id) {
    if (!message || !reply_out || message->type != OS_IPC_VFS_STAT_REPLY ||
        message->size != OS_VFS_STAT_REPLY_SIZE ||
        message->request_id != expected_request_id) return OS_VFS_STATUS_INVALID;
    reply_out->status = os_vfs_decode_i32(&message->data[0]);
    reply_out->size = os_vfs_decode_u32(&message->data[4]);
    reply_out->flags = os_vfs_decode_u32(&message->data[8]);
    return 0;
}

static inline int os_vfs_make_read_reply(os_ipc_payload_t* payload, int32_t status,
                                  const uint8_t* data, uint32_t size, uint32_t request_id) {
    uint32_t i;
    if (!payload || size > OS_VFS_READ_MAX || (size > 0U && !data)) return OS_VFS_STATUS_INVALID;
    payload->type = OS_IPC_VFS_READ_REPLY;
    payload->size = 8U + OS_VFS_READ_MAX;
    payload->request_id = request_id;
    os_vfs_encode_i32(&payload->data[0], status);
    os_vfs_encode_u32(&payload->data[4], size);
    for (i = 0U; i < size; i++) payload->data[8U + i] = data[i];
    for (; i < OS_VFS_READ_MAX; i++) payload->data[8U + i] = 0U;
    for (i = 8U + OS_VFS_READ_MAX; i < OS_IPC_MAX_DATA; i++) payload->data[i] = 0U;
    return 0;
}

static inline int os_vfs_parse_read_reply(const os_ipc_message_t* message,
                                   os_vfs_read_reply_t* reply_out,
                                   uint32_t expected_request_id) {
    uint32_t i;
    if (!message || !reply_out || message->type != OS_IPC_VFS_READ_REPLY ||
        message->size != 8U + OS_VFS_READ_MAX ||
        message->request_id != expected_request_id) return OS_VFS_STATUS_INVALID;
    reply_out->status = os_vfs_decode_i32(&message->data[0]);
    reply_out->size = os_vfs_decode_u32(&message->data[4]);
    if (reply_out->size > OS_VFS_READ_MAX) return OS_VFS_STATUS_INVALID;
    for (i = 0U; i < OS_VFS_READ_MAX; i++) reply_out->data[i] = message->data[8U + i];
    return 0;
}

static inline int os_vfs_make_worker_stat_request(os_ipc_payload_t* payload, const char* path,
                                                   uint32_t request_id) {
    uint32_t i;
    if (!payload || !os_vfs_path_is_safe(path)) return OS_VFS_STATUS_INVALID;
    payload->type = OS_IPC_VFS_WORKER_STAT;
    payload->size = OS_VFS_WORKER_STAT_REQUEST_SIZE;
    payload->request_id = request_id;
    for (i = 0U; i < OS_VFS_PATH_MAX; i++) payload->data[i] = (uint8_t)path[i];
    for (i = OS_VFS_PATH_MAX; i < OS_IPC_MAX_DATA; i++) payload->data[i] = 0U;
    return OS_VFS_STATUS_OK;
}

static inline int os_vfs_parse_worker_stat_request(const os_ipc_message_t* message, char* path_out) {
    uint32_t i;
    if (!message || !path_out || message->type != OS_IPC_VFS_WORKER_STAT ||
        message->size != OS_VFS_WORKER_STAT_REQUEST_SIZE) return OS_VFS_STATUS_INVALID;
    for (i = 0U; i < OS_VFS_PATH_MAX; i++) path_out[i] = (char)message->data[i];
    return os_vfs_path_is_safe(path_out) ? OS_VFS_STATUS_OK : OS_VFS_STATUS_INVALID;
}

static inline int os_vfs_make_worker_stat_reply(os_ipc_payload_t* payload, int32_t status,
                                                 uint32_t size, uint32_t flags,
                                                 uint32_t request_id) {
    uint32_t i;
    if (!payload) return OS_VFS_STATUS_INVALID;
    payload->type = OS_IPC_VFS_WORKER_STAT_REPLY;
    payload->size = OS_VFS_STAT_REPLY_SIZE;
    payload->request_id = request_id;
    os_vfs_encode_i32(&payload->data[0], status);
    os_vfs_encode_u32(&payload->data[4], size);
    os_vfs_encode_u32(&payload->data[8], flags);
    for (i = OS_VFS_STAT_REPLY_SIZE; i < OS_IPC_MAX_DATA; i++) payload->data[i] = 0U;
    return OS_VFS_STATUS_OK;
}

static inline int os_vfs_parse_worker_stat_reply(const os_ipc_message_t* message,
                                                  os_vfs_stat_reply_t* reply_out,
                                                  uint32_t expected_request_id) {
    if (!message || !reply_out || message->type != OS_IPC_VFS_WORKER_STAT_REPLY ||
        message->size != OS_VFS_STAT_REPLY_SIZE || message->request_id != expected_request_id)
        return OS_VFS_STATUS_INVALID;
    reply_out->status = os_vfs_decode_i32(&message->data[0]);
    reply_out->size = os_vfs_decode_u32(&message->data[4]);
    reply_out->flags = os_vfs_decode_u32(&message->data[8]);
    return OS_VFS_STATUS_OK;
}

static inline int os_vfs_make_worker_list_request(os_ipc_payload_t* payload, const char* path,
                                                   uint32_t request_id) {
    uint32_t i;
    if (!payload || !os_vfs_list_path_is_valid(path)) return OS_VFS_STATUS_INVALID;
    payload->type = OS_IPC_VFS_WORKER_LIST;
    payload->size = OS_VFS_WORKER_LIST_REQUEST_SIZE;
    payload->request_id = request_id;
    for (i = 0U; i < OS_VFS_PATH_MAX; i++) payload->data[i] = (uint8_t)path[i];
    for (i = OS_VFS_PATH_MAX; i < OS_IPC_MAX_DATA; i++) payload->data[i] = 0U;
    return OS_VFS_STATUS_OK;
}

static inline int os_vfs_parse_worker_list_request(const os_ipc_message_t* message, char* path_out) {
    uint32_t i;
    if (!message || !path_out || message->type != OS_IPC_VFS_WORKER_LIST ||
        message->size != OS_VFS_WORKER_LIST_REQUEST_SIZE) return OS_VFS_STATUS_INVALID;
    for (i = 0U; i < OS_VFS_PATH_MAX; i++) path_out[i] = (char)message->data[i];
    return os_vfs_list_path_is_valid(path_out) ? OS_VFS_STATUS_OK : OS_VFS_STATUS_INVALID;
}

static inline int os_vfs_make_worker_list_reply(os_ipc_payload_t* payload, int32_t status,
                                                 uint32_t count, const uint8_t* data,
                                                 uint32_t size, uint32_t request_id) {
    uint32_t i;
    if (!payload || count > OS_VFS_LIST_ENTRY_MAX || size > OS_VFS_LIST_DATA_MAX ||
        (size > 0U && !data)) return OS_VFS_STATUS_INVALID;
    payload->type = OS_IPC_VFS_WORKER_LIST_REPLY;
    payload->size = OS_VFS_LIST_REPLY_SIZE;
    payload->request_id = request_id;
    os_vfs_encode_i32(&payload->data[0], status);
    os_vfs_encode_u32(&payload->data[4], count);
    for (i = 0U; i < size; i++) payload->data[8U + i] = data[i];
    for (; i < OS_VFS_LIST_DATA_MAX; i++) payload->data[8U + i] = 0U;
    for (i = OS_VFS_LIST_REPLY_SIZE; i < OS_IPC_MAX_DATA; i++) payload->data[i] = 0U;
    return OS_VFS_STATUS_OK;
}

static inline int os_vfs_parse_worker_list_reply(const os_ipc_message_t* message,
                                                  os_vfs_list_reply_t* reply_out,
                                                  uint32_t expected_request_id) {
    uint32_t i;
    if (!message || !reply_out || message->type != OS_IPC_VFS_WORKER_LIST_REPLY ||
        message->size != OS_VFS_LIST_REPLY_SIZE || message->request_id != expected_request_id)
        return OS_VFS_STATUS_INVALID;
    reply_out->status = os_vfs_decode_i32(&message->data[0]);
    reply_out->count = os_vfs_decode_u32(&message->data[4]);
    if (reply_out->count > OS_VFS_LIST_ENTRY_MAX) return OS_VFS_STATUS_INVALID;
    for (i = 0U; i < OS_VFS_LIST_DATA_MAX; i++) reply_out->data[i] = message->data[8U + i];
    return OS_VFS_STATUS_OK;
}

static inline int os_vfs_make_worker_list_page_request(os_ipc_payload_t* payload, const char* path,
                                                        uint32_t start, uint32_t request_id) {
    uint32_t i;
    if (!payload || !os_vfs_list_page_path_is_valid(path)) return OS_VFS_STATUS_INVALID;
    payload->type = OS_IPC_VFS_WORKER_LIST_PAGE;
    payload->size = OS_VFS_WORKER_LIST_PAGE_REQUEST_SIZE;
    payload->request_id = request_id;
    for (i = 0U; i < OS_VFS_PATH_MAX; i++) payload->data[i] = (uint8_t)path[i];
    os_vfs_encode_u32(&payload->data[OS_VFS_PATH_MAX], start);
    for (i = OS_VFS_WORKER_LIST_PAGE_REQUEST_SIZE; i < OS_IPC_MAX_DATA; i++) payload->data[i] = 0U;
    return OS_VFS_STATUS_OK;
}

static inline int os_vfs_parse_worker_list_page_request(const os_ipc_message_t* message,
                                                         char* path_out, uint32_t* start_out) {
    uint32_t i;
    if (!message || !path_out || !start_out || message->type != OS_IPC_VFS_WORKER_LIST_PAGE ||
        message->size != OS_VFS_WORKER_LIST_PAGE_REQUEST_SIZE) return OS_VFS_STATUS_INVALID;
    for (i = 0U; i < OS_VFS_PATH_MAX; i++) path_out[i] = (char)message->data[i];
    if (!os_vfs_list_page_path_is_valid(path_out)) return OS_VFS_STATUS_INVALID;
    *start_out = os_vfs_decode_u32(&message->data[OS_VFS_PATH_MAX]);
    return OS_VFS_STATUS_OK;
}

static inline int os_vfs_make_worker_list_page_reply(os_ipc_payload_t* payload, int32_t status,
                                                      uint32_t count, uint32_t next_start,
                                                      const uint8_t* data, uint32_t size,
                                                      uint32_t request_id) {
    uint32_t i;
    if (!payload || count > OS_VFS_LIST_ENTRY_MAX || size > OS_VFS_LIST_PAGE_DATA_MAX ||
        (size > 0U && !data)) return OS_VFS_STATUS_INVALID;
    payload->type = OS_IPC_VFS_WORKER_LIST_PAGE_REPLY;
    payload->size = OS_VFS_LIST_PAGE_REPLY_SIZE;
    payload->request_id = request_id;
    os_vfs_encode_i32(&payload->data[0], status);
    os_vfs_encode_u32(&payload->data[4], count);
    os_vfs_encode_u32(&payload->data[8], next_start);
    for (i = 0U; i < size; i++) payload->data[12U + i] = data[i];
    for (; i < OS_VFS_LIST_PAGE_DATA_MAX; i++) payload->data[12U + i] = 0U;
    for (i = OS_VFS_LIST_PAGE_REPLY_SIZE; i < OS_IPC_MAX_DATA; i++) payload->data[i] = 0U;
    return OS_VFS_STATUS_OK;
}

static inline int os_vfs_parse_worker_list_page_reply(const os_ipc_message_t* message,
                                                       os_vfs_list_page_reply_t* reply_out,
                                                       uint32_t expected_request_id) {
    uint32_t i;
    if (!message || !reply_out || message->type != OS_IPC_VFS_WORKER_LIST_PAGE_REPLY ||
        message->size != OS_VFS_LIST_PAGE_REPLY_SIZE || message->request_id != expected_request_id)
        return OS_VFS_STATUS_INVALID;
    reply_out->status = os_vfs_decode_i32(&message->data[0]);
    reply_out->count = os_vfs_decode_u32(&message->data[4]);
    reply_out->next_start = os_vfs_decode_u32(&message->data[8]);
    if (reply_out->count > OS_VFS_LIST_ENTRY_MAX) return OS_VFS_STATUS_INVALID;
    for (i = 0U; i < OS_VFS_LIST_PAGE_DATA_MAX; i++) reply_out->data[i] = message->data[12U + i];
    return OS_VFS_STATUS_OK;
}

#endif
