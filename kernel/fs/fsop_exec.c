#include "fsop_exec.h"
#include "../../fs/overlay.h"

/* Tranche 4 suite: see fsop_exec.h. The FAT naming rules below moved here
 * unchanged from kernel/syscall/syscall.c so the kernel fallback and the
 * Ring 3 driver apply exactly the same ones. */

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

int fatvfs_fat16_create(fat16_volume_t* v, const char* name, const char* data, uint32_t size, int is_dir) {
    uint16_t first_cluster = 0U;
    char short_alias[16];
    if (!name || (size != 0U && !data)) return OS_FAT16_BAD_PATH;
    /* Le worker encode `mkdir` par un buffer nul et une taille nulle : une
     * écriture vide garde un buffer non nul et reste donc un fichier. */
    if (is_dir) return fat16_create_directory(v, name);
    if (fat_path_has_separator(name))
        return fat16_create_path_file(v, name, (const uint8_t*)data, size, &first_cluster);
    if (is_strict_short_83(name))
        return fat16_create_file(v, name, 0x20U, (const uint8_t*)data, size, &first_cluster);
    generate_short_alias(name, short_alias);
    return fat16_create_lfn_file(v, name, short_alias, 0x20U, (const uint8_t*)data, size,
                                 &first_cluster);
}

int fatvfs_fat16_unlink(fat16_volume_t* v, const char* name) {
    if (!name) return OS_FAT16_BAD_PATH;
    if (fat_path_is_directory_request(name)) {
        char directory[OS_NAME_MAX];
        int rc = fat_directory_name(name, directory);
        return rc == 0 ? fat16_remove_directory(v, directory) : rc;
    }
    return fat16_unlink_path_file(v, name);
}

int fatvfs_fat16_rename(fat16_volume_t* v, const char* old_name, const char* new_name) {
    char new_short[16];
    if (!old_name || !new_name) return OS_FAT16_BAD_PATH;
    if (fat_path_has_separator(old_name) || fat_path_has_separator(new_name))
        return fat16_rename_path_file(v, old_name, new_name);
    if (is_strict_short_83(old_name) && is_strict_short_83(new_name))
        return fat16_rename_file(v, old_name, new_name);
    generate_short_alias(new_name, new_short);
    return fat16_rename_lfn_file(v, old_name, new_name, new_short);
}

int fatvfs_fat32_create(fat32_volume_t* v, const char* name, const char* data, uint32_t size, int is_dir) {
    uint32_t first_cluster = 0U;
    char short_alias[16];
    if (!name || (size != 0U && !data)) return OS_FAT16_BAD_PATH;
    if (is_dir) return fat32_create_directory(v, name);
    if (fat_path_has_separator(name))
        return fat32_create_path_file(v, name, (const uint8_t*)data, size, &first_cluster);
    if (is_strict_short_83(name))
        return fat32_create_file(v, name, 0x20U, (const uint8_t*)data, size, &first_cluster);
    generate_short_alias(name, short_alias);
    return fat32_create_lfn_file(v, name, short_alias, 0x20U, (const uint8_t*)data, size,
                                 &first_cluster);
}

int fatvfs_fat32_unlink(fat32_volume_t* v, const char* name) {
    if (!name) return OS_FAT16_BAD_PATH;
    if (fat_path_is_directory_request(name)) {
        char directory[OS_NAME_MAX];
        int rc = fat_directory_name(name, directory);
        return rc == 0 ? fat32_remove_directory(v, directory) : rc;
    }
    return fat32_unlink_path_file(v, name);
}

int fatvfs_fat32_rename(fat32_volume_t* v, const char* old_name, const char* new_name) {
    char new_short[16];
    if (!old_name || !new_name) return OS_FAT16_BAD_PATH;
    if (fat_path_has_separator(old_name) || fat_path_has_separator(new_name))
        return fat32_rename_path_file(v, old_name, new_name);
    if (is_strict_short_83(old_name) && is_strict_short_83(new_name))
        return fat32_rename_file(v, old_name, new_name);
    generate_short_alias(new_name, new_short);
    return fat32_rename_lfn_file(v, old_name, new_name, new_short);
}

/* ------------------------------------------------------------------------ */

uint32_t fsop_store_for(uint32_t op) {
    if (op >= OS_ATA_FSOP_FAT16_READ && op <= OS_ATA_FSOP_FAT16_RENAME) return OS_ATA_FS_STORE_FAT16;
    if (op >= OS_ATA_FSOP_FAT16_READ + OS_ATA_FSOP_FAT32_BASE &&
        op <= OS_ATA_FSOP_FAT16_RENAME + OS_ATA_FSOP_FAT32_BASE) return OS_ATA_FS_STORE_FAT32;
    if (op >= OS_ATA_FSOP_OVL_READ && op <= OS_ATA_FSOP_OVL_IS_DIR) return OS_ATA_FS_STORE_OVERLAY;
    return 0U;
}

int fsop_is_fat_mutation(uint32_t op) {
    uint32_t base;
    if (fsop_store_for(op) == OS_ATA_FS_STORE_FAT32) op -= OS_ATA_FSOP_FAT32_BASE;
    else if (fsop_store_for(op) != OS_ATA_FS_STORE_FAT16) return 0;
    base = op;
    return base == OS_ATA_FSOP_FAT16_CREATE || base == OS_ATA_FSOP_FAT16_UNLINK ||
           base == OS_ATA_FSOP_FAT16_RENAME;
}

int fsop_is_mutation(uint32_t op) {
    if (fsop_is_fat_mutation(op)) return 1;
    switch (op) {
        case OS_ATA_FSOP_OVL_WRITE: case OS_ATA_FSOP_OVL_APPEND: case OS_ATA_FSOP_OVL_MKDIR:
        case OS_ATA_FSOP_OVL_UNLINK: case OS_ATA_FSOP_OVL_RENAME: case OS_ATA_FSOP_OVL_COPY:
            return 1;
        default:
            return 0;
    }
}

static uint32_t fsop_strnlen(const char* s, uint32_t max) {
    uint32_t n = 0U;
    if (!s) return 0U;
    while (n <= max && s[n] != '\0') n++;
    return n;
}

static void fsop_copy(uint8_t* dst, const uint8_t* src, uint32_t n) {
    uint32_t i;
    for (i = 0U; i < n; i++) dst[i] = src[i];
}

int fsop_encode(uint8_t* buf, uint32_t capacity, uint32_t op, uint32_t arg0, uint32_t arg1,
                const char* path, const char* path2, const void* in, uint32_t in_len,
                uint32_t out_cap) {
    os_ata_fsop_request_t h;
    uint32_t p1 = fsop_strnlen(path, OS_ATA_FSOP_PATH_MAX);
    uint32_t p2 = fsop_strnlen(path2, OS_ATA_FSOP_PATH_MAX);
    uint32_t total;
    uint8_t* w;
    if (!buf || fsop_store_for(op) == 0U || p1 > OS_ATA_FSOP_PATH_MAX - 1U ||
        p2 > OS_ATA_FSOP_PATH_MAX - 1U || (in_len != 0U && !in) ||
        in_len > OS_ATA_FSOP_BUFFER_SIZE ||
        out_cap > OS_ATA_FSOP_BUFFER_SIZE - (uint32_t)sizeof(os_ata_fsop_reply_t)) return -1;
    total = (uint32_t)sizeof(h) + p1 + 1U + p2 + 1U + in_len;
    if (total > capacity || total > OS_ATA_FSOP_BUFFER_SIZE) return -1;
    h.magic = OS_ATA_FSOP_MAGIC;
    h.op = op;
    h.arg0 = arg0;
    h.arg1 = arg1;
    h.path_len = p1;
    h.path2_len = p2;
    h.in_len = in_len;
    h.out_cap = out_cap;
    fsop_copy(buf, (const uint8_t*)&h, sizeof(h));
    w = buf + sizeof(h);
    if (p1) fsop_copy(w, (const uint8_t*)path, p1);
    w[p1] = 0U;
    w += p1 + 1U;
    if (p2) fsop_copy(w, (const uint8_t*)path2, p2);
    w[p2] = 0U;
    w += p2 + 1U;
    if (in_len) fsop_copy(w, (const uint8_t*)in, in_len);
    return (int)total;
}

int fsop_decode(const uint8_t* buf, uint32_t length, os_ata_fsop_request_t* req,
                const char** path, const char** path2, const uint8_t** in) {
    const uint8_t* p;
    uint32_t need;
    if (!buf || !req || !path || !path2 || !in || length < sizeof(*req) ||
        length > OS_ATA_FSOP_BUFFER_SIZE) return -1;
    fsop_copy((uint8_t*)req, buf, sizeof(*req));
    if (req->magic != OS_ATA_FSOP_MAGIC || fsop_store_for(req->op) == 0U ||
        req->path_len > OS_ATA_FSOP_PATH_MAX - 1U || req->path2_len > OS_ATA_FSOP_PATH_MAX - 1U ||
        req->in_len > OS_ATA_FSOP_BUFFER_SIZE ||
        req->out_cap > OS_ATA_FSOP_BUFFER_SIZE - (uint32_t)sizeof(os_ata_fsop_reply_t)) return -1;
    need = (uint32_t)sizeof(*req) + req->path_len + 1U + req->path2_len + 1U + req->in_len;
    if (need > length) return -1;
    p = buf + sizeof(*req);
    if (p[req->path_len] != 0U || p[req->path_len + 1U + req->path2_len] != 0U) return -1;
    *path = (const char*)p;
    *path2 = (const char*)(p + req->path_len + 1U);
    *in = p + req->path_len + 1U + req->path2_len + 1U;
    return 0;
}

/* Bytes the op may return for count entries of size each, bounded by out_cap. */
static uint32_t fsop_entries_fit(uint32_t wanted, uint32_t each, uint32_t out_cap) {
    uint32_t fit = out_cap / each;
    return wanted < fit ? wanted : fit;
}

static int32_t fsop_fat16(uint32_t op, const os_ata_fsop_request_t* r, const char* path,
                          const char* path2, const uint8_t* in, uint8_t* out, uint32_t out_cap,
                          uint32_t* out_len, fat16_volume_t* v16, fat32_volume_t* v32, int fat32) {
    uint32_t cap;
    int32_t rc;
    const uint32_t each = (uint32_t)sizeof(os_fat16_dirent_t);
    if ((!fat32 && !v16) || (fat32 && !v32)) return OS_FAT16_NOT_MOUNTED;
    switch (op) {
        case OS_ATA_FSOP_FAT16_READ:
            cap = r->arg0 < out_cap ? r->arg0 : out_cap;
            if (!path[0] || cap == 0U) return OS_FAT16_BAD_PATH;
            rc = fat32 ? fat32_read_path(v32, path, out, cap) : fat16_read_path(v16, path, (char*)out, cap);
            if (rc > 0) *out_len = (uint32_t)rc;
            return rc;
        case OS_ATA_FSOP_FAT16_LIST:
        case OS_ATA_FSOP_FAT16_LIST_PAGE:
        case OS_ATA_FSOP_FAT16_LIST_PATH:
            cap = fsop_entries_fit(r->arg0, each, out_cap);
            if (cap == 0U) return OS_FAT16_BAD_PATH;
            if (op == OS_ATA_FSOP_FAT16_LIST)
                rc = fat32 ? fat32_list_root(v32, (os_fat16_dirent_t*)out, cap)
                           : fat16_list_root(v16, (os_fat16_dirent_t*)out, cap);
            else if (op == OS_ATA_FSOP_FAT16_LIST_PAGE)
                rc = fat32 ? fat32_list_root_page(v32, r->arg1, (os_fat16_dirent_t*)out, cap)
                           : fat16_list_root_page(v16, r->arg1, (os_fat16_dirent_t*)out, cap);
            else
                rc = fat32 ? fat32_list_path_page(v32, path, r->arg1, (os_fat16_dirent_t*)out, cap)
                           : fat16_list_path_page(v16, path, r->arg1, (os_fat16_dirent_t*)out, cap);
            if (rc > 0) *out_len = (uint32_t)rc * each;
            return rc;
        case OS_ATA_FSOP_FAT16_CREATE:
            return fat32 ? fatvfs_fat32_create(v32, path, (const char*)in, r->in_len, r->arg0 == 1U)
                         : fatvfs_fat16_create(v16, path, (const char*)in, r->in_len, r->arg0 == 1U);
        case OS_ATA_FSOP_FAT16_UNLINK:
            return fat32 ? fatvfs_fat32_unlink(v32, path) : fatvfs_fat16_unlink(v16, path);
        case OS_ATA_FSOP_FAT16_RENAME:
            return fat32 ? fatvfs_fat32_rename(v32, path, path2) : fatvfs_fat16_rename(v16, path, path2);
        default:
            return OS_FAT16_BAD_PATH;
    }
}

int32_t fsop_execute(const os_ata_fsop_request_t* req, const char* path, const char* path2,
                     const uint8_t* in, uint8_t* out, uint32_t out_cap, uint32_t* out_len,
                     fat16_volume_t* v16, fat32_volume_t* v32) {
    uint32_t store, cap;
    int32_t rc;
    const uint32_t each = (uint32_t)sizeof(os_dirent_t);
    if (!req || !path || !path2 || !out || !out_len) return -1;
    *out_len = 0U;
    if (out_cap > req->out_cap) out_cap = req->out_cap;
    store = fsop_store_for(req->op);
    if (store == OS_ATA_FS_STORE_FAT16)
        return fsop_fat16(req->op, req, path, path2, in, out, out_cap, out_len, v16, v32, 0);
    if (store == OS_ATA_FS_STORE_FAT32)
        return fsop_fat16(req->op - OS_ATA_FSOP_FAT32_BASE, req, path, path2, in, out, out_cap,
                          out_len, v16, v32, 1);
    switch (req->op) {
        case OS_ATA_FSOP_OVL_READ:
            cap = req->arg0 < out_cap ? req->arg0 : out_cap;
            if (cap == 0U) return -1;
            rc = overlay_read(path, (char*)out, cap);
            if (rc > 0) *out_len = (uint32_t)rc;
            return rc;
        case OS_ATA_FSOP_OVL_WRITE:
            return overlay_write(path, (const char*)in, req->in_len);
        case OS_ATA_FSOP_OVL_APPEND:
            return overlay_append(path, (const char*)in, req->in_len);
        case OS_ATA_FSOP_OVL_MKDIR:
            return overlay_mkdir(path);
        case OS_ATA_FSOP_OVL_UNLINK:
            return overlay_unlink(path);
        case OS_ATA_FSOP_OVL_RENAME:
            return overlay_rename(path, path2);
        case OS_ATA_FSOP_OVL_COPY:
            return overlay_copy(path, path2);
        case OS_ATA_FSOP_OVL_STAT:
            if (out_cap < each) return -1;
            rc = overlay_stat(path, (os_dirent_t*)out);
            if (rc == OV_OK) *out_len = each;
            return rc;
        case OS_ATA_FSOP_OVL_LISTDIR: {
            /* arg0 = entries already filled by the caller (initrd), passed in. */
            uint32_t start = req->arg0, max_n = req->arg1;
            if (max_n == 0U || max_n * each > out_cap || start > max_n ||
                req->in_len != start * each) return -1;
            fsop_copy(out, in, req->in_len);
            rc = overlay_listdir(path, (os_dirent_t*)out, (int)start, (int)max_n);
            if (rc > 0) *out_len = (uint32_t)rc * each;
            return rc;
        }
        case OS_ATA_FSOP_OVL_LISTDIR_PAGE:
            if (req->arg1 == 0U || req->arg1 * each > out_cap) return -1;
            rc = overlay_listdir_page(path, (os_dirent_t*)out, req->arg0, (int)req->arg1);
            if (rc > 0) *out_len = (uint32_t)rc * each;
            return rc;
        case OS_ATA_FSOP_OVL_IS_DIR:
            return overlay_is_dir(path);
        default:
            return -1;
    }
}
