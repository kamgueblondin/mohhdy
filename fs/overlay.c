#include "overlay.h"
#include "initrd.h"
#include "kernel/ata.h"

#define OV_MAX_NODES OV_SNAP_NODES
#define OV_PATH_MAX  OV_SNAP_PATH
#define OV_DATA_MAX  OV_SNAP_DATA

typedef struct {
    int used;
    int is_dir;
    int is_continuation;
    int next_node;
    char path[OV_PATH_MAX];
    char data[OV_DATA_MAX];
    uint32_t size;
} ov_node_t;

static ov_node_t g_ov[OV_MAX_NODES];
static os_dirent_t overlay_page_entries[OV_MAX_NODES];
/* Lot 46: disk budget of the mutations (see overlay_set_capacity). */
static uint32_t g_ov_capacity = OV_SNAP_MAX_SIZE;

void overlay_set_capacity(uint32_t bytes) {
    if (bytes < OV_SNAP_SIZE) bytes = OV_SNAP_SIZE;
    if (bytes > OV_SNAP_MAX_SIZE) bytes = OV_SNAP_MAX_SIZE;
    g_ov_capacity = bytes;
}

uint32_t overlay_capacity(void) { return g_ov_capacity; }

/* Largest file of the current budget: a v3-only disk keeps the Lot 44
 * bound (30 KiB), a disk with the extension area takes 64 KiB. */
static uint32_t ov_file_max(void) {
    return g_ov_capacity > OV_SNAP_SIZE ? OV_FILE_MAX_BYTES : OV_FILE_MAX_V3_BYTES;
}

uint32_t overlay_image_lba(uint32_t s) {
    return s < OS_ATA_OVERLAY_SECTORS ? s : OS_ATA_OVERLAY_EXT_LBA + (s - OS_ATA_OVERLAY_SECTORS);
}

static uint32_t ov_free_count(void);

static void ov_free_chain(ov_node_t *head) {
    ov_node_t *curr = head;
    while (curr) {
        int next_idx = curr->next_node;
        curr->used = 0;
        curr->is_dir = 0;
        curr->is_continuation = 0;
        curr->next_node = -1;
        curr->size = 0;
        curr->path[0] = '\0';
        curr = (next_idx >= 0 && next_idx < OV_MAX_NODES) ? &g_ov[next_idx] : 0;
    }
}

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
        if (g_ov[i].used && !g_ov[i].is_continuation && ov_eq(g_ov[i].path, want)) {
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
        g_ov[i].is_continuation = 0;
        g_ov[i].next_node = -1;
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
    if (overlay_snapshot_bytes() + OV_SNAP_REC_HDR + (uint32_t)ov_len(want) > g_ov_capacity)
        return OV_ERR_NOSPACE;
    if (ov_free_count() == 0U) return OV_ERR_NOSPACE;
    n = ov_alloc();
    if (!n) return OV_ERR_NOSPACE;
    n->used = 1;
    n->is_dir = 1;
    n->is_continuation = 0;
    n->next_node = -1;
    n->size = 0;
    n->data[0] = '\0';
    ov_copy(n->path, want, OV_PATH_MAX);
    overlay_save_disk();
    return OV_OK;
}

static ov_node_t* ov_next(const ov_node_t* n) {
    if (!n || n->next_node < 0 || n->next_node >= OV_MAX_NODES) return 0;
    return &g_ov[n->next_node];
}

static uint32_t ov_blocks_for(uint32_t n) {
    return n == 0U ? 1U : (n + OV_DATA_MAX - 1U) / OV_DATA_MAX;
}

/* v3 image bytes of one file of n bytes at a path of plen chars. */
static uint32_t ov_file_bytes(uint32_t plen, uint32_t n) {
    return ov_blocks_for(n) * OV_SNAP_REC_HDR + plen + n;
}

static uint32_t ov_chain_bytes(const ov_node_t* head) {
    if (!head) return 0U;
    if (head->is_dir) return OV_SNAP_REC_HDR + (uint32_t)ov_len(head->path);
    return ov_file_bytes((uint32_t)ov_len(head->path), head->size);
}

static uint32_t ov_chain_len(const ov_node_t* head) {
    uint32_t k = 0U;
    while (head && k < OV_MAX_NODES) { k++; head = ov_next(head); }
    return k;
}

uint32_t overlay_snapshot_bytes(void) {
    uint32_t total = 16U;
    int i;
    for (i = 0; i < OV_MAX_NODES; i++) {
        if (g_ov[i].used && !g_ov[i].is_continuation) total += ov_chain_bytes(&g_ov[i]);
    }
    return total;
}

/* Free slots the mutations may use. With a v3-only disk (capacity of the
 * 32 KiB area) the image must keep at most OV_SNAP_V3_NODES records. */
static uint32_t ov_free_count(void) {
    uint32_t k = 0U, lim;
    int i;
    for (i = 0; i < OV_MAX_NODES; i++) if (!g_ov[i].used) k++;
    lim = g_ov_capacity > OV_SNAP_SIZE ? (uint32_t)OV_MAX_NODES : OV_SNAP_V3_NODES;
    if ((uint32_t)OV_MAX_NODES - k >= lim) return 0U;
    if (k > lim - ((uint32_t)OV_MAX_NODES - k)) k = lim - ((uint32_t)OV_MAX_NODES - k);
    return k;
}

/* Builds a fresh chain; the caller checked the node count. The head gets
 * its path last, so a half built chain is never visible to ov_find. */
static ov_node_t* ov_build(const char* want, const char* data, uint32_t n) {
    ov_node_t* head = 0;
    ov_node_t* prev = 0;
    uint32_t off = 0U;
    do {
        ov_node_t* cur = ov_alloc();
        uint32_t chunk = n - off > OV_DATA_MAX ? OV_DATA_MAX : n - off;
        uint32_t i;
        if (!cur) { if (head) ov_free_chain(head); return 0; }
        cur->used = 1;
        cur->is_dir = 0;
        cur->is_continuation = head ? 1 : 0;
        cur->next_node = -1;
        cur->path[0] = '\0';
        cur->size = chunk;
        for (i = 0U; i < chunk; i++) cur->data[i] = data[off + i];
        off += chunk;
        if (prev) prev->next_node = (int)(cur - g_ov);
        else head = cur;
        prev = cur;
    } while (off < n);
    head->size = n;
    ov_copy(head->path, want, OV_PATH_MAX);
    return head;
}

/* Replaces (or creates) WANT with n bytes. Every check happens first: on a
 * refusal the old file is untouched (no half written chain, no data loss). */
static int ov_store(const char* want, const char* data, uint32_t n) {
    ov_node_t* old = ov_find(want);
    uint32_t need_nodes = ov_blocks_for(n);
    uint32_t old_nodes = old ? ov_chain_len(old) : 0U;
    uint32_t bytes = overlay_snapshot_bytes() - (old ? ov_chain_bytes(old) : 0U);
    ov_node_t* head;
    if (n > ov_file_max()) return OV_ERR_NOSPACE;
    if (bytes + ov_file_bytes((uint32_t)ov_len(want), n) > g_ov_capacity) return OV_ERR_NOSPACE;
    if (ov_free_count() + old_nodes < need_nodes) return OV_ERR_NOSPACE;
    if (ov_free_count() >= need_nodes) {
        if (old) old->path[0] = '\0';   /* hidden while the new chain is built */
        head = ov_build(want, data, n);
        if (!head) { if (old) ov_copy(old->path, want, OV_PATH_MAX); return OV_ERR_NOSPACE; }
        if (old) ov_free_chain(old);
    } else {
        ov_free_chain(old);             /* enough nodes once freed (checked) */
        head = ov_build(want, data, n);
        if (!head) return OV_ERR_NOSPACE;
    }
    return (int)n;
}

int overlay_write(const char* path, const char* data, uint32_t n) {
    ov_node_t* node;
    char want[OV_PATH_MAX];
    int rc;
    ov_normalize(path, want, OV_PATH_MAX);
    if (!want[0] || (n > 0 && !data)) return OV_ERR_INVAL;
    if (n > ov_file_max()) return OV_ERR_NOSPACE;
    node = ov_find(want);
    if (node && node->is_dir) return OV_ERR_ISDIR;
    if (initrd_is_dir(want)) return OV_ERR_ISDIR;
    if (!node && !ov_parent_is_dir(want)) return OV_ERR_NOTDIR;
    rc = ov_store(want, data, n);
    if (rc >= 0) overlay_save_disk();
    return rc;
}

static char g_ov_tmp[OV_FILE_MAX_BYTES];

int overlay_append(const char* path, const char* data, uint32_t n) {
    ov_node_t* head;
    char want[OV_PATH_MAX];
    uint32_t have = 0U, i;
    int rc;
    ov_normalize(path, want, OV_PATH_MAX);
    if (!want[0] || (n > 0 && !data)) return OV_ERR_INVAL;
    if (initrd_is_dir(want)) return OV_ERR_ISDIR;
    head = ov_find(want);
    if (head && head->is_dir) return OV_ERR_ISDIR;
    if (head) {
        if (head->size + n > ov_file_max()) return OV_ERR_NOSPACE;
        rc = overlay_read(want, g_ov_tmp, OV_FILE_MAX_BYTES);
        if (rc < 0) return rc;
        have = (uint32_t)rc;
    } else if (initrd_is_file(want)) {
        rc = initrd_read_into(want, g_ov_tmp, OV_FILE_MAX_BYTES);
        if (rc < 0) return rc;
        have = (uint32_t)rc;
    } else if (!ov_parent_is_dir(want)) {
        return OV_ERR_NOTDIR;
    }
    if (have + n > ov_file_max()) return OV_ERR_NOSPACE;
    for (i = 0U; i < n; i++) g_ov_tmp[have + i] = data[i];
    rc = ov_store(want, g_ov_tmp, have + n);
    if (rc < 0) return rc;
    overlay_save_disk();
    return (int)n;
}

int overlay_read_at(const char* path, char* buf, uint32_t max, uint32_t offset) {
    ov_node_t* cur = ov_find(path);
    uint32_t pos = 0U, out = 0U;
    if (!cur) return OV_ERR_NOTFOUND;
    if (cur->is_dir) return OV_ERR_ISDIR;
    if (!buf || max == 0) return OV_ERR_INVAL;
    if (offset >= cur->size) return 0;
    while (cur && out < max) {
        uint32_t blk = cur->is_continuation ? cur->size
                                            : (cur->size > OV_DATA_MAX ? OV_DATA_MAX : cur->size);
        if (pos + blk > offset) {
            uint32_t i = offset > pos ? offset - pos : 0U;
            for (; i < blk && out < max; i++) buf[out++] = cur->data[i];
        }
        pos += blk;
        cur = ov_next(cur);
    }
    return (int)out;
}

int overlay_read(const char* path, char* buf, uint32_t max) {
    return overlay_read_at(path, buf, max, 0U);
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
    ov_free_chain(n);
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

    {
        uint32_t grow = 0U;
        for (i = 0; i < OV_MAX_NODES; i++) {
            int rest;
            if (!g_ov[i].used || g_ov[i].is_continuation) continue;
            if (!ov_under(g_ov[i].path, oldp, oldn)) continue;
            rest = ov_len(g_ov[i].path) - oldn;
            if (newn + rest >= OV_PATH_MAX) return OV_ERR_INVAL;
            if (newn > oldn) grow += (uint32_t)(newn - oldn);
        }
        if (overlay_snapshot_bytes() + grow > g_ov_capacity) return OV_ERR_NOSPACE;
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

int overlay_copy(const char* src, const char* dst) {
    ov_node_t* srcnode;
    char oldp[OV_PATH_MAX];
    char newp[OV_PATH_MAX];
    int oldn, newn, i, pass;
    uint32_t need_nodes = 0U, need_bytes = 0U;

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
        if (!g_ov[i].used || g_ov[i].is_continuation) continue;
        if (!ov_under(g_ov[i].path, oldp, oldn)) continue;
        rest = ov_len(g_ov[i].path) - oldn;
        if (newn + rest >= OV_PATH_MAX) return OV_ERR_INVAL;
        need_nodes += g_ov[i].is_dir ? 1U : ov_chain_len(&g_ov[i]);
        need_bytes += g_ov[i].is_dir ? OV_SNAP_REC_HDR + (uint32_t)(newn + rest)
                                     : ov_file_bytes((uint32_t)(newn + rest), g_ov[i].size);
    }
    if (need_nodes > ov_free_count()) return OV_ERR_NOSPACE;
    if (overlay_snapshot_bytes() + need_bytes > g_ov_capacity) return OV_ERR_NOSPACE;

    /* Directories first so every copied file finds its parent. New heads
     * live under newp, never under oldp (newp is not inside oldp). */
    for (pass = 0; pass < 2; pass++) {
        for (i = 0; i < OV_MAX_NODES; i++) {
            char target[OV_PATH_MAX];
            int k, r;
            if (!g_ov[i].used || g_ov[i].is_continuation) continue;
            if ((pass == 0) != (g_ov[i].is_dir != 0)) continue;
            if (!ov_under(g_ov[i].path, oldp, oldn)) continue;
            ov_copy(target, newp, OV_PATH_MAX);
            for (k = 0, r = oldn; g_ov[i].path[r] && newn + k < OV_PATH_MAX - 1; k++, r++)
                target[newn + k] = g_ov[i].path[r];
            target[newn + k] = '\0';
            if (g_ov[i].is_dir) {
                ov_node_t* n = ov_alloc();
                if (!n) return OV_ERR_NOSPACE;
                n->used = 1; n->is_dir = 1; n->is_continuation = 0; n->next_node = -1;
                n->size = 0; ov_copy(n->path, target, OV_PATH_MAX);
            } else {
                int got = overlay_read(g_ov[i].path, g_ov_tmp, OV_FILE_MAX_BYTES);
                if (got < 0 || !ov_build(target, g_ov_tmp, (uint32_t)got)) return OV_ERR_NOSPACE;
            }
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

#define OV_DISK_SECTORS 64   /* LBA 0-63, see OV_SNAP_SIZE */
#define OV_DISK_BYTES   OV_SNAP_MAX_SIZE /* v4: + extension area */

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

/* v1 / v2: fixed slot images written before Lot 44 (read only). */
static int ov_restore_fixed(const uint8_t* buf, uint32_t n) {
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
    if (version == OV_SNAP_V2_VERSION) {
        stored_nodes = OV_SNAP_V2_NODES;
        stored_path = OV_SNAP_V2_PATH;
        stored_data = OV_SNAP_V2_DATA;
        stored_node = OV_SNAP_V2_NODE;
        stored_size = OV_SNAP_V2_SIZE;
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
    if (used_count > stored_nodes || used_count > OV_MAX_NODES) return -1;

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

    /* Pack the used slots into the first RAM slots (v2 had 64 slots). */
    overlay_init();
    off = 16U;
    {
        int slot = 0;
        for (i = 0; i < (int)stored_nodes; i++) {
            uint32_t b;
            if (buf[off]) {
                ov_node_t* nd = &g_ov[slot++];
                nd->used = 1;
                nd->is_dir = buf[off + 1U] ? 1 : 0;
                nd->is_continuation = 0;
                nd->next_node = -1;
                nd->size = ov_get_u32(buf + off + 4U);
                for (b = 0U; b + 1U < OV_PATH_MAX && b < stored_path; b++) {
                    nd->path[b] = (char)buf[off + 8U + b];
                }
                nd->path[OV_PATH_MAX - 1U] = '\0';
                for (b = 0U; b < nd->size && b < stored_data; b++) {
                    nd->data[b] = (char)buf[off + 8U + stored_path + b];
                }
            }
            off += stored_node;
        }
    }
    return 0;
}

static void ov_put_u16(uint8_t* p, uint32_t v) {
    p[0] = (uint8_t)v;
    p[1] = (uint8_t)(v >> 8);
}

static uint32_t ov_get_u16(const uint8_t* p) {
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8);
}

static uint32_t ov_put_rec(uint8_t* buf, uint32_t off, uint8_t type, const char* path,
                           const char* data, uint32_t dlen, uint32_t size) {
    uint32_t plen = (uint32_t)ov_len(path), b;
    buf[off] = type;
    buf[off + 1U] = (uint8_t)plen;
    ov_put_u16(buf + off + 2U, dlen);
    ov_put_u32(buf + off + 4U, size);
    off += OV_SNAP_REC_HDR;
    for (b = 0U; b < plen; b++) buf[off++] = (uint8_t)path[b];
    for (b = 0U; b < dlen; b++) buf[off++] = (uint8_t)data[b];
    return off;
}

/* v3: dense records, zero padded to OV_SNAP_SIZE (the disk area).
 * Lot 46 v4: same records, zero padded to OV_SNAP_MAX_SIZE, used only when
 * the overlay does not fit v3 (bytes or record count). */
int overlay_snapshot(uint8_t* buf, uint32_t max, uint32_t* out_size) {
    uint32_t off = 16U, recs = 0U, b, used = 0U, bytes = overlay_snapshot_bytes(), image;
    uint32_t version = OV_SNAP_VERSION, nodes = OV_SNAP_V3_NODES;
    int i;
    for (i = 0; i < OV_MAX_NODES; i++) if (g_ov[i].used) used++;
    image = OV_SNAP_SIZE;
    if (bytes > OV_SNAP_SIZE || used > OV_SNAP_V3_NODES) {
        image = OV_SNAP_MAX_SIZE;
        version = OV_SNAP_V4_VERSION;
        nodes = OV_SNAP_NODES;
    }
    if (!buf || max < image || bytes > image) return -1;
    for (i = 0; i < OV_MAX_NODES; i++) {
        const ov_node_t* h = &g_ov[i];
        if (!h->used || h->is_continuation) continue;
        if (h->is_dir) {
            off = ov_put_rec(buf, off, 1U, h->path, 0, 0U, 0U);
            recs++;
            continue;
        }
        {
            const ov_node_t* c = h;
            uint32_t left = h->size;
            int first = 1;
            do {
                uint32_t blk = left > OV_DATA_MAX ? OV_DATA_MAX : left;
                off = ov_put_rec(buf, off, first ? 0U : 2U, first ? h->path : "", c->data, blk,
                                 first ? h->size : 0U);
                recs++;
                left -= blk;
                first = 0;
                c = ov_next(c);
            } while (left > 0U && c);
        }
    }
    for (b = off; b < image; b++) buf[b] = 0U;
    ov_put_u32(buf + 0, OV_SNAP_MAGIC);
    ov_put_u32(buf + 4, version);
    ov_put_u32(buf + 8, nodes);
    ov_put_u32(buf + 12, recs);
    if (out_size) *out_size = image;
    return 0;
}

uint32_t overlay_image_bytes(const uint8_t* buf) {
    if (!buf || ov_get_u32(buf + 0) != OV_SNAP_MAGIC) return 0U;
    return ov_get_u32(buf + 4) == OV_SNAP_V4_VERSION ? OV_SNAP_MAX_SIZE : OV_SNAP_SIZE;
}

/* Two passes: validate the whole image, then apply it (a bad image changes
 * nothing in RAM). */
static int ov_restore_v3(const uint8_t* buf, uint32_t n) {
    uint32_t recs, pass, nodes;
    /* v3 headers store 128 (OV_SNAP_V3_NODES), v4 headers OV_SNAP_NODES. A
     * v4 image cut to the 32 KiB area (extension not read) is refused by
     * the record bounds below, nothing changes in RAM. */
    nodes = ov_get_u32(buf + 4) == OV_SNAP_V4_VERSION ? OV_SNAP_NODES : OV_SNAP_V3_NODES;
    if (n < 16U || ov_get_u32(buf + 8) != nodes) return -1;
    if (nodes == OV_SNAP_NODES && n < OV_SNAP_MAX_SIZE) return -1;
    recs = ov_get_u32(buf + 12);
    if (recs > nodes) return -1;
    for (pass = 0U; pass < 2U; pass++) {
        uint32_t off = 16U, r, left = 0U, slot = 0U;
        ov_node_t* prev = 0;
        if (pass == 1U) overlay_init();
        for (r = 0U; r < recs; r++) {
            uint32_t type, plen, dlen, size, b;
            if (off + OV_SNAP_REC_HDR > n) return -1;
            type = buf[off];
            plen = buf[off + 1U];
            dlen = ov_get_u16(buf + off + 2U);
            size = ov_get_u32(buf + off + 4U);
            if (type > 2U || plen >= OV_PATH_MAX || dlen > OV_DATA_MAX) return -1;
            if (off + OV_SNAP_REC_HDR + plen + dlen > n) return -1;
            for (b = 0U; b < plen; b++) if (buf[off + OV_SNAP_REC_HDR + b] == 0U) return -1;
            if (type == 2U) {
                if (left == 0U || plen != 0U || dlen == 0U || dlen > left) return -1;
            } else {
                if (left != 0U || plen == 0U) return -1;
                if (type == 1U && (dlen != 0U || size != 0U)) return -1;
                if (type == 0U && (size > OV_FILE_MAX_BYTES ||
                                   dlen != (size > OV_DATA_MAX ? OV_DATA_MAX : size))) return -1;
            }
            if (pass == 1U) {
                ov_node_t* nd = &g_ov[slot];
                nd->used = 1;
                nd->is_dir = type == 1U;
                nd->is_continuation = type == 2U;
                nd->next_node = -1;
                nd->size = type == 0U ? size : dlen;
                for (b = 0U; b < plen; b++) nd->path[b] = (char)buf[off + OV_SNAP_REC_HDR + b];
                nd->path[plen] = '\0';
                for (b = 0U; b < dlen; b++) nd->data[b] = (char)buf[off + OV_SNAP_REC_HDR + plen + b];
                if (type == 2U && prev) prev->next_node = (int)slot;
                prev = nd;
            }
            slot++;
            if (type == 0U) left = size - dlen;
            else if (type == 2U) left -= dlen;
            else left = 0U;
            off += OV_SNAP_REC_HDR + plen + dlen;
        }
        if (left != 0U) return -1;
    }
    return 0;
}

int overlay_restore(const uint8_t* buf, uint32_t n) {
    if (buf && n >= 16U && ov_get_u32(buf + 0) == OV_SNAP_MAGIC &&
        (ov_get_u32(buf + 4) == OV_SNAP_VERSION || ov_get_u32(buf + 4) == OV_SNAP_V4_VERSION))
        return ov_restore_v3(buf, n);
    return ov_restore_fixed(buf, n);
}

/* Tranche 4 slice 2: when the Ring 3 atadriver is live, redirect() queues the
 * snapshot for the driver and returns 1; the kernel PIO write below then only
 * runs when no driver exists (degraded fallback). */
static int (*g_ov_save_redirect)(void);
static void (*g_ov_kernel_written)(void);

void overlay_set_disk_hooks(int (*redirect)(void), void (*kernel_written)(void)) {
    g_ov_save_redirect = redirect;
    g_ov_kernel_written = kernel_written;
}

int overlay_save_disk(void) {
    uint32_t sz = 0;
    uint32_t i;
    int rc;

    if (!ata_present()) return 0;
    if (g_ov_save_redirect && g_ov_save_redirect()) return 0;
    if (overlay_snapshot(g_ov_disk_buf, sizeof(g_ov_disk_buf), &sz) != 0) return -1;
    for (i = sz; i < sizeof(g_ov_disk_buf); i++) g_ov_disk_buf[i] = 0;
    rc = ata_write_sectors(0, OV_DISK_SECTORS, g_ov_disk_buf);
    /* Lot 46: a v4 image also fills the extension area (written after the
     * primary area: a torn write leaves a v4 header with a stale extension,
     * the record checks of the next restore refuse it). */
    if (rc == 0 && sz > OV_SNAP_SIZE)
        rc = ata_write_sectors(OS_ATA_OVERLAY_EXT_LBA, OS_ATA_OVERLAY_EXT_SECTORS,
                               g_ov_disk_buf + OV_SNAP_SIZE);
    if (rc == 0 && g_ov_kernel_written) g_ov_kernel_written();
    return rc;
}

int overlay_load_disk(void) {
    uint32_t i;
    if (!ata_present()) return -1;
    if (ata_read_sectors(0, OV_DISK_SECTORS, g_ov_disk_buf) != 0) return -1;
    for (i = OV_SNAP_SIZE; i < sizeof(g_ov_disk_buf); i++) g_ov_disk_buf[i] = 0;
    if (overlay_image_bytes(g_ov_disk_buf) > OV_SNAP_SIZE &&
        ata_read_sectors(OS_ATA_OVERLAY_EXT_LBA, OS_ATA_OVERLAY_EXT_SECTORS,
                         g_ov_disk_buf + OV_SNAP_SIZE) != 0) return -1;
    return overlay_restore(g_ov_disk_buf, sizeof(g_ov_disk_buf));
}
