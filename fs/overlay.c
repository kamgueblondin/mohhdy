#include "overlay.h"
#include "initrd.h"
#include "kernel/ata.h"

#define OV_MAX_NODES OV_SNAP_NODES
#define OV_PATH_MAX  OV_SNAP_PATH
#define OV_DATA_MAX  OV_SNAP_DATA

typedef struct {
    int used;
    int is_dir;
    char path[OV_PATH_MAX];
    char data[OV_DATA_MAX];
    uint32_t size;
} ov_node_t;

static ov_node_t g_ov[OV_MAX_NODES];
static os_dirent_t overlay_page_entries[OV_MAX_NODES];

static int ov_len(const char* s) {
    int n = 0;
    if (!s) return 0;
    while (s[n]) n++;
    return n;
}

static void ov_copy(char* dest, const char* src, int max) {
    int i = 0;
    if (!src) src = "";
    while (src[i] && i < max - 1) {
        dest[i] = src[i];
        i++;
    }
    dest[i] = '\0';
}

static int ov_eq(const char* a, const char* b) {
    int i = 0;
    if (!a) a = "";
    if (!b) b = "";
    while (a[i] && b[i]) {
        if (a[i] != b[i]) return 0;
        i++;
    }
    return a[i] == b[i];
}

static void ov_normalize(const char* in, char* out, int max) {
    if (!in) {
        out[0] = '\0';
        return;
    }
    if (in[0] == '.' && in[1] == '/') in += 2;
    while (*in == '/') in++;
    ov_copy(out, in, max);
    {
        int n = ov_len(out);
        while (n > 0 && out[n - 1] == '/') {
            out[n - 1] = '\0';
            n--;
        }
    }
}

static void ov_parent(const char* path, char* out, int max) {
    int n = ov_len(path);
    int slash = -1;
    int i;
    for (i = 0; i < n; i++) {
        if (path[i] == '/') slash = i;
    }
    if (slash < 0) {
        out[0] = '\0';
        return;
    }
    if (slash >= max) slash = max - 1;
    for (i = 0; i < slash; i++) out[i] = path[i];
    out[slash] = '\0';
}

static ov_node_t* ov_find(const char* path) {
    char want[OV_PATH_MAX];
    int i;
    ov_normalize(path, want, OV_PATH_MAX);
    if (!want[0]) return 0;
    for (i = 0; i < OV_MAX_NODES; i++) {
        if (g_ov[i].used && ov_eq(g_ov[i].path, want)) {
            return &g_ov[i];
        }
    }
    return 0;
}

static ov_node_t* ov_alloc(void) {
    int i;
    for (i = 0; i < OV_MAX_NODES; i++) {
        if (!g_ov[i].used) return &g_ov[i];
    }
    return 0;
}

static int ov_has_children(const char* path) {
    char want[OV_PATH_MAX];
    int plen;
    int i;
    ov_normalize(path, want, OV_PATH_MAX);
    plen = ov_len(want);
    for (i = 0; i < OV_MAX_NODES; i++) {
        if (!g_ov[i].used) continue;
        if (plen == 0) {
            if (g_ov[i].path[0]) return 1;
        } else {
            int j = 0;
            while (j < plen && g_ov[i].path[j] == want[j]) j++;
            if (j == plen && g_ov[i].path[j] == '/') return 1;
        }
    }
    return 0;
}

static int ov_parent_is_dir(const char* path) {
    char parent[OV_PATH_MAX];
    ov_parent(path, parent, OV_PATH_MAX);
    if (!parent[0]) return 1;
    if (overlay_is_dir(parent)) return 1;
    if (initrd_is_dir(parent)) return 1;
    return 0;
}

static int ov_direct_child(const char* prefix, const char* path, char* name, int nmax) {
    int plen = ov_len(prefix);
    const char* rest;
    int i;
    if (plen == 0) {
        rest = path;
    } else {
        int j = 0;
        while (j < plen && path[j] == prefix[j]) j++;
        if (j != plen || path[j] != '/') return 0;
        rest = path + plen + 1;
    }
    if (!rest[0]) return 0;
    for (i = 0; rest[i]; i++) {
        if (rest[i] == '/') return 0;
    }
    ov_copy(name, rest, nmax);
    return 1;
}

void overlay_init(void) {
    int i;
    for (i = 0; i < OV_MAX_NODES; i++) {
        g_ov[i].used = 0;
        g_ov[i].path[0] = '\0';
        g_ov[i].size = 0;
        g_ov[i].is_dir = 0;
    }
}

int overlay_is_dir(const char* path) {
    ov_node_t* n;
    char want[OV_PATH_MAX];
    ov_normalize(path, want, OV_PATH_MAX);
    if (!want[0]) return 1;
    n = ov_find(want);
    return n && n->is_dir;
}

int overlay_stat(const char* path, os_dirent_t* out) {
    ov_node_t* n;
    char want[OV_PATH_MAX];
    const char* base;
    int i;
    ov_normalize(path, want, OV_PATH_MAX);
    if (!want[0]) {
        if (out) {
            out->name[0] = '/';
            out->name[1] = '\0';
            out->size = 0;
            out->flags = OS_DIRENT_DIR;
        }
        return OV_OK;
    }
    n = ov_find(want);
    if (!n) return OV_ERR_NOTFOUND;
    if (out) {
        base = want;
        for (i = 0; want[i]; i++) {
            if (want[i] == '/') base = want + i + 1;
        }
        ov_copy(out->name, base, OS_NAME_MAX);
        out->size = n->is_dir ? 0 : n->size;
        out->flags = n->is_dir ? OS_DIRENT_DIR : OS_DIRENT_FILE;
    }
    return OV_OK;
}

int overlay_mkdir(const char* path) {
    ov_node_t* n;
    char want[OV_PATH_MAX];
    ov_normalize(path, want, OV_PATH_MAX);
    if (!want[0]) return OV_ERR_EXISTS;
    if (ov_find(want)) return OV_ERR_EXISTS;
    if (initrd_is_file(want)) return OV_ERR_EXISTS;
    if (!ov_parent_is_dir(want)) return OV_ERR_NOTDIR;
    n = ov_alloc();
    if (!n) return OV_ERR_NOSPACE;
    n->used = 1;
    n->is_dir = 1;
    n->size = 0;
    n->data[0] = '\0';
    ov_copy(n->path, want, OV_PATH_MAX);
    overlay_save_disk();
    return OV_OK;
}

int overlay_write(const char* path, const char* data, uint32_t n) {
    ov_node_t* node;
    char want[OV_PATH_MAX];
    uint32_t i;
    ov_normalize(path, want, OV_PATH_MAX);
    if (!want[0] || (n > 0 && !data)) return OV_ERR_INVAL;
    node = ov_find(want);
    if (node && node->is_dir) return OV_ERR_ISDIR;
    if (initrd_is_dir(want)) return OV_ERR_ISDIR;
    if (!node) {
        if (!ov_parent_is_dir(want)) return OV_ERR_NOTDIR;
        node = ov_alloc();
        if (!node) return OV_ERR_NOSPACE;
        node->used = 1;
        node->is_dir = 0;
        ov_copy(node->path, want, OV_PATH_MAX);
    }
    if (n > OV_DATA_MAX) n = OV_DATA_MAX;
    for (i = 0; i < n; i++) node->data[i] = data[i];
    node->size = n;
    overlay_save_disk();
    return (int)n;
}

int overlay_append(const char* path, const char* data, uint32_t n) {
    ov_node_t* node;
    char want[OV_PATH_MAX];
    uint32_t i;
    ov_normalize(path, want, OV_PATH_MAX);
    if (!want[0] || (n > 0 && !data)) return OV_ERR_INVAL;
    if (initrd_is_dir(want)) return OV_ERR_ISDIR;
    node = ov_find(want);
    if (node && node->is_dir) return OV_ERR_ISDIR;
    if (!node) {
        uint32_t have = 0;
        char tmp[OV_DATA_MAX];
        if (initrd_is_file(want)) {
            int r = initrd_read_into(want, tmp, OV_DATA_MAX);
            if (r < 0) return r;
            have = (uint32_t)r;
        } else if (!ov_parent_is_dir(want)) {
            return OV_ERR_NOTDIR;
        }
        if (have + n > OV_DATA_MAX) return OV_ERR_NOSPACE;
        node = ov_alloc();
        if (!node) return OV_ERR_NOSPACE;
        node->used = 1;
        node->is_dir = 0;
        node->size = have;
        ov_copy(node->path, want, OV_PATH_MAX);
        for (i = 0; i < have; i++) node->data[i] = tmp[i];
    }
    if (node->size + n > OV_DATA_MAX) return OV_ERR_NOSPACE;
    for (i = 0; i < n; i++) node->data[node->size + i] = data[i];
    node->size += n;
    overlay_save_disk();
    return (int)n;
}

int overlay_read(const char* path, char* buf, uint32_t max) {
    ov_node_t* n;
    uint32_t i;
    uint32_t copy;
    n = ov_find(path);
    if (!n) return OV_ERR_NOTFOUND;
    if (n->is_dir) return OV_ERR_ISDIR;
    if (!buf || max == 0) return OV_ERR_INVAL;
    copy = n->size;
    if (copy > max) copy = max;
    for (i = 0; i < copy; i++) buf[i] = n->data[i];
    return (int)copy;
}

int overlay_unlink(const char* path) {
    ov_node_t* n;
    char want[OV_PATH_MAX];
    ov_normalize(path, want, OV_PATH_MAX);
    if (!want[0]) return OV_ERR_INVAL;
    n = ov_find(want);
    if (!n) {
        if (initrd_is_file(want) || initrd_is_dir(want)) return OV_ERR_PROTECTED;
        return OV_ERR_NOTFOUND;
    }
    if (n->is_dir && ov_has_children(want)) return OV_ERR_NOTEMPTY;
    n->used = 0;
    n->path[0] = '\0';
    n->size = 0;
    overlay_save_disk();
    return OV_OK;
}

static int ov_under(const char* path, const char* prefix, int plen) {
    int i = 0;
    while (i < plen && path[i] == prefix[i]) i++;
    if (i != plen) return 0;
    return path[plen] == '\0' || path[plen] == '/';
}

int overlay_rename(const char* oldpath, const char* newpath) {
    char oldp[OV_PATH_MAX];
    char newp[OV_PATH_MAX];
    int oldn;
    int newn;
    int i;

    ov_normalize(oldpath, oldp, OV_PATH_MAX);
    ov_normalize(newpath, newp, OV_PATH_MAX);
    if (!oldp[0] || !newp[0]) return OV_ERR_INVAL;
    if (ov_eq(oldp, newp)) return OV_OK;

    if (!ov_find(oldp)) {
        if (initrd_is_file(oldp) || initrd_is_dir(oldp)) return OV_ERR_PROTECTED;
        return OV_ERR_NOTFOUND;
    }
    if (ov_find(newp)) return OV_ERR_EXISTS;
    if (initrd_is_file(newp) || initrd_is_dir(newp)) return OV_ERR_EXISTS;
    if (!ov_parent_is_dir(newp)) return OV_ERR_NOTDIR;

    oldn = ov_len(oldp);
    newn = ov_len(newp);
    if (ov_under(newp, oldp, oldn) && newp[oldn] == '/') return OV_ERR_INVAL;

    for (i = 0; i < OV_MAX_NODES; i++) {
        int rest;
        if (!g_ov[i].used) continue;
        if (!ov_under(g_ov[i].path, oldp, oldn)) continue;
        rest = ov_len(g_ov[i].path) - oldn;
        if (newn + rest >= OV_PATH_MAX) return OV_ERR_INVAL;
    }

    for (i = 0; i < OV_MAX_NODES; i++) {
        char rest[OV_PATH_MAX];
        int r = 0;
        if (!g_ov[i].used) continue;
        if (!ov_under(g_ov[i].path, oldp, oldn)) continue;
        while (g_ov[i].path[oldn + r]) {
            rest[r] = g_ov[i].path[oldn + r];
            r++;
        }
        rest[r] = '\0';
        ov_copy(g_ov[i].path, newp, OV_PATH_MAX);
        {
            int k;
            for (k = 0; rest[k] && newn + k < OV_PATH_MAX - 1; k++) {
                g_ov[i].path[newn + k] = rest[k];
            }
            g_ov[i].path[newn + k] = '\0';
        }
    }
    overlay_save_disk();
    return OV_OK;
}

static int ov_used_count(void) {
    int n = 0;
    int i;
    for (i = 0; i < OV_MAX_NODES; i++) {
        if (g_ov[i].used) n++;
    }
    return n;
}

int overlay_copy(const char* src, const char* dst) {
    ov_node_t* srcnode;
    char oldp[OV_PATH_MAX];
    char newp[OV_PATH_MAX];
    int oldn;
    int newn;
    int i;
    int need = 0;
    int free_n;

    ov_normalize(src, oldp, OV_PATH_MAX);
    ov_normalize(dst, newp, OV_PATH_MAX);
    if (!oldp[0] || !newp[0]) return OV_ERR_INVAL;
    if (ov_eq(oldp, newp)) return OV_ERR_EXISTS;

    srcnode = ov_find(oldp);
    if (!srcnode) {
        if (initrd_is_file(oldp) || initrd_is_dir(oldp)) return OV_ERR_PROTECTED;
        return OV_ERR_NOTFOUND;
    }
    if (ov_find(newp)) return OV_ERR_EXISTS;
    if (initrd_is_file(newp) || initrd_is_dir(newp)) return OV_ERR_EXISTS;
    if (!ov_parent_is_dir(newp)) return OV_ERR_NOTDIR;

    oldn = ov_len(oldp);
    newn = ov_len(newp);
    if (ov_under(newp, oldp, oldn) && newp[oldn] == '/') return OV_ERR_INVAL;

    for (i = 0; i < OV_MAX_NODES; i++) {
        int rest;
        if (!g_ov[i].used) continue;
        if (!ov_under(g_ov[i].path, oldp, oldn)) continue;
        rest = ov_len(g_ov[i].path) - oldn;
        if (newn + rest >= OV_PATH_MAX) return OV_ERR_INVAL;
        need++;
    }
    free_n = OV_MAX_NODES - ov_used_count();
    if (need > free_n) return OV_ERR_NOSPACE;

    for (i = 0; i < OV_MAX_NODES; i++) {
        ov_node_t* n;
        char rest[OV_PATH_MAX];
        int r = 0;
        int k;
        uint32_t b;
        if (!g_ov[i].used) continue;
        if (!ov_under(g_ov[i].path, oldp, oldn)) continue;
        n = ov_alloc();
        if (!n) return OV_ERR_NOSPACE;
        n->used = 1;
        while (g_ov[i].path[oldn + r]) {
            rest[r] = g_ov[i].path[oldn + r];
            r++;
        }
        rest[r] = '\0';
        ov_copy(n->path, newp, OV_PATH_MAX);
        for (k = 0; rest[k] && newn + k < OV_PATH_MAX - 1; k++) {
            n->path[newn + k] = rest[k];
        }
        n->path[newn + k] = '\0';
        n->used = 1;
        n->is_dir = g_ov[i].is_dir;
        n->size = g_ov[i].size;
        for (b = 0; b < n->size && b < OV_DATA_MAX; b++) {
            n->data[b] = g_ov[i].data[b];
        }
    }
    overlay_save_disk();
    return OV_OK;
}

int overlay_listdir(const char* path, os_dirent_t* out, int start, int max_n) {
    char prefix[OV_PATH_MAX];
    int count = start;
    int i;
    if (!out || max_n <= 0) return start < 0 ? 0 : start;
    if (start < 0) start = 0;
    count = start;
    ov_normalize(path ? path : "/", prefix, OV_PATH_MAX);
    for (i = 0; i < OV_MAX_NODES && count < max_n; i++) {
        char name[OS_NAME_MAX];
        int e;
        int dup = 0;
        if (!g_ov[i].used) continue;
        if (!ov_direct_child(prefix, g_ov[i].path, name, OS_NAME_MAX)) continue;
        for (e = 0; e < count; e++) {
            if (ov_eq(out[e].name, name)) {
                out[e].flags = g_ov[i].is_dir ? OS_DIRENT_DIR : OS_DIRENT_FILE;
                out[e].size = g_ov[i].is_dir ? 0 : g_ov[i].size;
                dup = 1;
                break;
            }
        }
        if (dup) continue;
        ov_copy(out[count].name, name, OS_NAME_MAX);
        out[count].flags = g_ov[i].is_dir ? OS_DIRENT_DIR : OS_DIRENT_FILE;
        out[count].size = g_ov[i].is_dir ? 0 : g_ov[i].size;
        count++;
    }
    return count;
}

int overlay_listdir_page(const char* path, os_dirent_t* out, uint32_t start, int max_n) {
    int total;
    int emitted;
    int i;
    if (!out || max_n <= 0) return -1;
    total = overlay_listdir(path, overlay_page_entries, 0, OV_MAX_NODES);
    if (total < 0) return total;
    if (start >= (uint32_t)total) return 0;
    emitted = total - (int)start;
    if (emitted > max_n) emitted = max_n;
    for (i = 0; i < emitted; i++) {
        uint32_t j;
        for (j = 0U; j < OS_NAME_MAX; j++) {
            out[i].name[j] = overlay_page_entries[start + (uint32_t)i].name[j];
        }
        out[i].size = overlay_page_entries[start + (uint32_t)i].size;
        out[i].flags = overlay_page_entries[start + (uint32_t)i].flags;
    }
    return emitted;
}

#define OV_DISK_SECTORS 64
#define OV_DISK_BYTES   (OV_DISK_SECTORS * 512)

static uint8_t g_ov_disk_buf[OV_DISK_BYTES];

static void ov_put_u32(uint8_t* p, uint32_t v) {
    p[0] = (uint8_t)v;
    p[1] = (uint8_t)(v >> 8);
    p[2] = (uint8_t)(v >> 16);
    p[3] = (uint8_t)(v >> 24);
}

static uint32_t ov_get_u32(const uint8_t* p) {
    return (uint32_t)p[0]
        | ((uint32_t)p[1] << 8)
        | ((uint32_t)p[2] << 16)
        | ((uint32_t)p[3] << 24);
}

int overlay_snapshot(uint8_t* buf, uint32_t max, uint32_t* out_size) {
    uint32_t off;
    int i;
    int used = 0;

    if (!buf || max < OV_SNAP_SIZE) return -1;

    for (i = 0; i < OV_MAX_NODES; i++) {
        if (g_ov[i].used) used++;
    }

    ov_put_u32(buf + 0, OV_SNAP_MAGIC);
    ov_put_u32(buf + 4, OV_SNAP_VERSION);
    ov_put_u32(buf + 8, OV_SNAP_NODES);
    ov_put_u32(buf + 12, (uint32_t)used);

    off = 16;
    for (i = 0; i < OV_MAX_NODES; i++) {
        uint32_t b;
        buf[off + 0] = g_ov[i].used ? 1 : 0;
        buf[off + 1] = g_ov[i].is_dir ? 1 : 0;
        buf[off + 2] = 0;
        buf[off + 3] = 0;
        ov_put_u32(buf + off + 4, g_ov[i].used ? g_ov[i].size : 0);
        for (b = 0; b < OV_PATH_MAX; b++) {
            buf[off + 8 + b] = (uint8_t)g_ov[i].path[b];
        }
        for (b = 0; b < OV_DATA_MAX; b++) {
            buf[off + 8 + OV_PATH_MAX + b] = (uint8_t)g_ov[i].data[b];
        }
        off += OV_SNAP_NODE;
    }

    if (out_size) *out_size = OV_SNAP_SIZE;
    return 0;
}

int overlay_restore(const uint8_t* buf, uint32_t n) {
    uint32_t off;
    uint32_t used_count;
    uint32_t seen;
    uint32_t version;
    uint32_t stored_nodes;
    uint32_t stored_path;
    uint32_t stored_data;
    uint32_t stored_node;
    uint32_t stored_size;
    int i;

    if (!buf || n < 16U || ov_get_u32(buf + 0) != OV_SNAP_MAGIC) return -1;
    version = ov_get_u32(buf + 4);
    if (version == OV_SNAP_VERSION) {
        stored_nodes = OV_SNAP_NODES;
        stored_path = OV_SNAP_PATH;
        stored_data = OV_SNAP_DATA;
        stored_node = OV_SNAP_NODE;
        stored_size = OV_SNAP_SIZE;
    } else if (version == OV_SNAP_V1_VERSION) {
        stored_nodes = OV_SNAP_V1_NODES;
        stored_path = OV_SNAP_V1_PATH;
        stored_data = OV_SNAP_V1_DATA;
        stored_node = OV_SNAP_V1_NODE;
        stored_size = OV_SNAP_V1_SIZE;
    } else {
        return -1;
    }
    if (n < stored_size || ov_get_u32(buf + 8) != stored_nodes) return -1;

    used_count = ov_get_u32(buf + 12);
    if (used_count > stored_nodes) return -1;

    seen = 0;
    off = 16U;
    for (i = 0; i < (int)stored_nodes; i++) {
        uint8_t used = buf[off];
        uint8_t is_dir = buf[off + 1U];
        uint32_t size = ov_get_u32(buf + off + 4U);
        if (used > 1U || is_dir > 1U) return -1;
        if (used) {
            if (size > stored_data) return -1;
            seen++;
        }
        off += stored_node;
    }
    if (seen != used_count) return -1;

    overlay_init();
    off = 16U;
    for (i = 0; i < (int)stored_nodes && i < OV_MAX_NODES; i++) {
        uint32_t b;
        if (buf[off]) {
            g_ov[i].used = 1;
            g_ov[i].is_dir = buf[off + 1U] ? 1 : 0;
            g_ov[i].size = ov_get_u32(buf + off + 4U);
            for (b = 0U; b + 1U < OV_PATH_MAX && b < stored_path; b++) {
                g_ov[i].path[b] = (char)buf[off + 8U + b];
            }
            g_ov[i].path[OV_PATH_MAX - 1U] = '\0';
            for (b = 0U; b < g_ov[i].size && b < stored_data; b++) {
                g_ov[i].data[b] = (char)buf[off + 8U + stored_path + b];
            }
        }
        off += stored_node;
    }
    return 0;
}

int overlay_save_disk(void) {
    uint32_t sz = 0;
    uint32_t i;

    if (!ata_present()) return 0;
    if (overlay_snapshot(g_ov_disk_buf, sizeof(g_ov_disk_buf), &sz) != 0) return -1;
    for (i = sz; i < sizeof(g_ov_disk_buf); i++) g_ov_disk_buf[i] = 0;
    return ata_write_sectors(0, OV_DISK_SECTORS, g_ov_disk_buf);
}

int overlay_load_disk(void) {
    if (!ata_present()) return -1;
    if (ata_read_sectors(0, OV_DISK_SECTORS, g_ov_disk_buf) != 0) return -1;
    return overlay_restore(g_ov_disk_buf, sizeof(g_ov_disk_buf));
}
