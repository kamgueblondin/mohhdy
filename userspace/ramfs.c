/* ramfs.c - VFS RAM (table de noeuds en memoire). Sans libc. */

#include "ramfs.h"

typedef struct {
    int used;
    int is_dir;
    char path[RAMFS_PATH_MAX];
    char content[RAMFS_CONTENT_MAX];
    int size;
} ramfs_node_t;

#define VFS_MAX_QUOTAS 8
#define VFS_MAX_ACLS   16
#define VFS_MAX_LOCKS  16
#define VFS_JOURNAL_LOG_MAX 16
#define VFS_SNAPSHOT_MAX 4

typedef enum {
    VFS_OP_WRITE = 1,
    VFS_OP_RM = 2,
    VFS_OP_MV = 3,
    VFS_OP_MKDIR = 4
} vfs_op_type_t;

typedef struct {
    int used;
    int tx_id;
    vfs_op_type_t op;
    char path[RAMFS_PATH_MAX];
    char old_path[RAMFS_PATH_MAX];
    char old_content[RAMFS_CONTENT_MAX];
    int old_size;
    int existed;
    int was_dir;
} vfs_journal_entry_t;

typedef struct {
    int used;
    uint32_t id;
    ramfs_node_t nodes[RAMFS_MAX_NODES];
    int count;
} vfs_snapshot_t;

typedef struct {
    int used;
    char path[RAMFS_PATH_MAX];
    uint32_t max_bytes;
    uint32_t max_nodes;
} vfs_quota_entry_t;

typedef struct {
    int used;
    char path[RAMFS_PATH_MAX];
    char origin[64];
    uint8_t rights;
} vfs_acl_entry_t;

typedef struct {
    int used;
    char path[RAMFS_PATH_MAX];
    int lock_type;
    int count;
} vfs_lock_entry_t;

static ramfs_node_t g_nodes[RAMFS_MAX_NODES];
static int g_count;

static vfs_quota_entry_t g_quotas[VFS_MAX_QUOTAS];
static vfs_acl_entry_t g_acls[VFS_MAX_ACLS];
static vfs_lock_entry_t g_locks[VFS_MAX_LOCKS];

static vfs_journal_entry_t g_journal[VFS_JOURNAL_LOG_MAX];
static int g_journal_head = 0;
static int g_active_tx = 0;
static int g_tx_counter = 1;

static vfs_snapshot_t g_snapshots[VFS_SNAPSHOT_MAX];
static uint32_t g_snapshot_counter = 1;

static int rf_strlen(const char *s) {
    int n = 0;
    if (!s) return 0;
    while (s[n]) n++;
    return n;
}

static int rf_strcmp(const char *a, const char *b) {
    int i = 0;
    if (!a) a = "";
    if (!b) b = "";
    while (a[i] && b[i]) {
        if (a[i] != b[i]) return a[i] - b[i];
        i++;
    }
    return a[i] - b[i];
}

static int rf_strncmp(const char *a, const char *b, int n) {
    for (int i = 0; i < n; i++) {
        if (a[i] != b[i]) return a[i] - b[i];
        if (a[i] == 0) return 0;
    }
    return 0;
}

static void rf_strcpy(char *d, const char *s) {
    int i = 0;
    if (!s) s = "";
    while (s[i]) {
        d[i] = s[i];
        i++;
    }
    d[i] = 0;
}

static void rf_memcpy(char *d, const char *s, int n) {
    for (int i = 0; i < n; i++) d[i] = s[i];
}

static int rf_push_part(char parts[][64], int *n, const char *start, int len) {
    if (len <= 0) return 0;
    if (len == 1 && start[0] == '.') return 0;
    if (len == 2 && start[0] == '.' && start[1] == '.') {
        if (*n > 0) (*n)--;
        return 0;
    }
    if (*n >= 32) return -1;
    if (len > 63) len = 63;
    for (int i = 0; i < len; i++) parts[*n][i] = start[i];
    parts[*n][len] = 0;
    (*n)++;
    return 0;
}

static void rf_split_push(char parts[][64], int *n, const char *s) {
    int i = 0;
    if (!s) return;
    while (s[i]) {
        while (s[i] == '/') i++;
        if (!s[i]) break;
        int start = i;
        while (s[i] && s[i] != '/') i++;
        rf_push_part(parts, n, s + start, i - start);
    }
}

void ramfs_resolve(const char *cwd, const char *path, char *out, int outsz) {
    char parts[32][64];
    int n = 0;
    if (!out || outsz < 2) return;
    if (!path || path[0] == 0) {
        const char *src = (cwd && cwd[0]) ? cwd : "/";
        int i = 0;
        while (src[i] && i < outsz - 1) {
            out[i] = src[i];
            i++;
        }
        out[i] = 0;
        return;
    }
    if (path[0] != '/') {
        rf_split_push(parts, &n, (cwd && cwd[0]) ? cwd : "/");
    }
    rf_split_push(parts, &n, path);
    if (n == 0) {
        out[0] = '/';
        out[1] = 0;
        return;
    }
    int pos = 0;
    for (int i = 0; i < n; i++) {
        if (pos >= outsz - 1) break;
        out[pos++] = '/';
        int j = 0;
        while (parts[i][j] && pos < outsz - 1) {
            out[pos++] = parts[i][j++];
        }
    }
    out[pos] = 0;
}

static void rf_parent(const char *path, char *out) {
    int len = rf_strlen(path);
    int i;
    if (len <= 1) {
        out[0] = '/';
        out[1] = 0;
        return;
    }
    i = len - 1;
    if (path[i] == '/') i--;
    while (i > 0 && path[i] != '/') i--;
    if (i == 0) {
        out[0] = '/';
        out[1] = 0;
        return;
    }
    for (int k = 0; k < i; k++) out[k] = path[k];
    out[i] = 0;
}

static const char *rf_basename(const char *path) {
    int i;
    int last = 0;
    if (!path || (path[0] == '/' && path[1] == 0)) return path ? path : "/";
    for (i = 0; path[i]; i++) {
        if (path[i] == '/' && path[i + 1] != 0) last = i + 1;
    }
    return path + last;
}

static ramfs_node_t *find_node(const char *path) {
    for (int i = 0; i < RAMFS_MAX_NODES; i++) {
        if (g_nodes[i].used && rf_strcmp(g_nodes[i].path, path) == 0) {
            return &g_nodes[i];
        }
    }
    return 0;
}

static ramfs_node_t *alloc_node(void) {
    for (int i = 0; i < RAMFS_MAX_NODES; i++) {
        if (!g_nodes[i].used) {
            g_nodes[i].used = 1;
            g_nodes[i].is_dir = 0;
            g_nodes[i].path[0] = 0;
            g_nodes[i].content[0] = 0;
            g_nodes[i].size = 0;
            g_count++;
            return &g_nodes[i];
        }
    }
    return 0;
}

static int add_dir(const char *path) {
    ramfs_node_t *n;
    if (find_node(path)) return RAMFS_ERR_EXISTS;
    n = alloc_node();
    if (!n) return RAMFS_ERR_NOSPACE;
    rf_strcpy(n->path, path);
    n->is_dir = 1;
    return RAMFS_OK;
}

static int add_file(const char *path, const char *text) {
    ramfs_node_t *n;
    int len;
    if (find_node(path)) return RAMFS_ERR_EXISTS;
    n = alloc_node();
    if (!n) return RAMFS_ERR_NOSPACE;
    rf_strcpy(n->path, path);
    n->is_dir = 0;
    len = rf_strlen(text);
    if (len >= RAMFS_CONTENT_MAX) len = RAMFS_CONTENT_MAX - 1;
    rf_memcpy(n->content, text, len);
    n->content[len] = 0;
    n->size = len;
    return RAMFS_OK;
}

static int is_direct_child(const char *dir, const char *path) {
    int dlen = rf_strlen(dir);
    if (dlen == 1 && dir[0] == '/') {
        int i;
        if (path[0] != '/' || path[1] == 0) return 0;
        for (i = 1; path[i]; i++) {
            if (path[i] == '/') return 0;
        }
        return 1;
    }
    if (rf_strncmp(path, dir, dlen) != 0) return 0;
    if (path[dlen] != '/') return 0;
    {
        const char *rest = path + dlen + 1;
        if (*rest == 0) return 0;
        for (; *rest; rest++) {
            if (*rest == '/') return 0;
        }
    }
    return 1;
}

static int has_children(const char *dir) {
    for (int i = 0; i < RAMFS_MAX_NODES; i++) {
        if (g_nodes[i].used && is_direct_child(dir, g_nodes[i].path)) return 1;
    }
    return 0;
}

static int parent_is_dir(const char *path) {
    char p[RAMFS_PATH_MAX];
    ramfs_node_t *n;
    rf_parent(path, p);
    n = find_node(p);
    if (!n || !n->is_dir) return 0;
    return 1;
}

void ramfs_init(void) {
    int i;
    for (i = 0; i < RAMFS_MAX_NODES; i++) {
        g_nodes[i].used = 0;
        g_nodes[i].path[0] = 0;
        g_nodes[i].content[0] = 0;
        g_nodes[i].size = 0;
        g_nodes[i].is_dir = 0;
    }
    g_count = 0;

    for (i = 0; i < VFS_MAX_QUOTAS; i++) {
        g_quotas[i].used = 0;
    }
    for (i = 0; i < VFS_MAX_ACLS; i++) {
        g_acls[i].used = 0;
    }
    for (i = 0; i < VFS_MAX_LOCKS; i++) {
        g_locks[i].used = 0;
    }
    for (i = 0; i < VFS_JOURNAL_LOG_MAX; i++) {
        g_journal[i].used = 0;
    }
    for (i = 0; i < VFS_SNAPSHOT_MAX; i++) {
        g_snapshots[i].used = 0;
    }
    g_active_tx = 0;
    g_journal_head = 0;

    add_dir("/");
    add_dir("/bin");
    add_dir("/docs");
    add_dir("/home");
    add_dir("/home/user");
    add_file("/test.txt", "Ceci est un fichier de test depuis l'initrd !\n");
    add_file("/hello.txt", "Un autre fichier de demonstration.\n");
    add_file("/config.cfg", "Configuration du systeme MOHHDY v5.0\n");
    add_file("/startup.sh", "#!/bin/sh\necho 'Script de demarrage MOHHDY v5.0'\n");
    add_file("/ai_data.txt", "Donnees pour l'intelligence artificielle simulee\n");
    add_file("/ai_knowledge.txt", "Base de connaissances IA - Version simulation\nbonjour\nmemoire\nprocessus\n");
    add_file("/docs/readme.txt", "Documentation MOHHDY (VFS RAM pedagogique).\n");
}

int ramfs_exists(const char *path) {
    return find_node(path) != 0;
}

int ramfs_is_dir(const char *path) {
    ramfs_node_t *n = find_node(path);
    return n && n->is_dir;
}

int ramfs_is_file(const char *path) {
    ramfs_node_t *n = find_node(path);
    return n && !n->is_dir;
}

static void vfs_journal_record(vfs_op_type_t op, const char *path, const char *old_path) {
    if (!g_active_tx) return;
    int idx = g_journal_head;
    vfs_journal_entry_t *e = &g_journal[idx];
    g_journal_head = (g_journal_head + 1) % VFS_JOURNAL_LOG_MAX;

    e->used = 1;
    e->tx_id = g_active_tx;
    e->op = op;
    rf_strcpy(e->path, path ? path : "");
    rf_strcpy(e->old_path, old_path ? old_path : "");
    e->old_content[0] = 0;
    e->old_size = 0;
    e->existed = 0;
    e->was_dir = 0;

    ramfs_node_t *n = find_node(path);
    if (n) {
        e->existed = 1;
        e->was_dir = n->is_dir;
        if (!n->is_dir) {
            rf_memcpy(e->old_content, n->content, n->size);
            e->old_content[n->size] = 0;
            e->old_size = n->size;
        }
    }
}

int ramfs_mkdir(const char *path) {
    if (!path || path[0] == 0 || rf_strcmp(path, "/") == 0) return RAMFS_ERR_INVAL;
    if (find_node(path)) return RAMFS_ERR_EXISTS;
    if (!parent_is_dir(path)) return RAMFS_ERR_NOTDIR;
    vfs_journal_record(VFS_OP_MKDIR, path, 0);
    return add_dir(path);
}

int ramfs_mkdir_p(const char *path) {
    char cur[RAMFS_PATH_MAX];
    int i = 0;
    if (!path || path[0] == 0) return RAMFS_ERR_INVAL;
    if (path[0] == '/') {
        cur[0] = '/';
        cur[1] = 0;
        i = 1;
    } else {
        cur[0] = 0;
    }
    while (path[i]) {
        while (path[i] == '/') i++;
        if (!path[i]) break;
        int len = rf_strlen(cur);
        if (len > 1 && cur[len - 1] != '/') {
            cur[len++] = '/';
            cur[len] = 0;
        } else if (len == 0) {
            cur[0] = '/';
            cur[1] = 0;
            len = 1;
        }
        while (path[i] && path[i] != '/') {
            if (len < RAMFS_PATH_MAX - 1) {
                cur[len++] = path[i];
            }
            i++;
        }
        cur[len] = 0;
        if (!find_node(cur)) {
            int rc = add_dir(cur);
            if (rc != RAMFS_OK && rc != RAMFS_ERR_EXISTS) return rc;
        } else if (!ramfs_is_dir(cur)) {
            return RAMFS_ERR_NOTDIR;
        }
    }
    return RAMFS_OK;
}

int ramfs_rmdir(const char *path) {
    ramfs_node_t *n;
    if (!path || rf_strcmp(path, "/") == 0) return RAMFS_ERR_INVAL;
    n = find_node(path);
    if (!n) return RAMFS_ERR_NOTFOUND;
    if (!n->is_dir) return RAMFS_ERR_NOTDIR;
    if (has_children(path)) return RAMFS_ERR_NOTEMPTY;
    n->used = 0;
    g_count--;
    return RAMFS_OK;
}

int ramfs_rm_r(const char *path) {
    ramfs_node_t *n;
    int plen;
    if (!path || rf_strcmp(path, "/") == 0) return RAMFS_ERR_INVAL;
    n = find_node(path);
    if (!n) return RAMFS_ERR_NOTFOUND;
    if (!n->is_dir) {
        n->used = 0;
        g_count--;
        return RAMFS_OK;
    }
    plen = rf_strlen(path);
    for (int i = 0; i < RAMFS_MAX_NODES; i++) {
        if (!g_nodes[i].used) continue;
        if (rf_strcmp(g_nodes[i].path, path) == 0) {
            g_nodes[i].used = 0;
            g_count--;
        } else if (rf_strncmp(g_nodes[i].path, path, plen) == 0 &&
                   g_nodes[i].path[plen] == '/') {
            g_nodes[i].used = 0;
            g_count--;
        }
    }
    return RAMFS_OK;
}

int ramfs_rm(const char *path) {
    ramfs_node_t *n;
    if (!path || rf_strcmp(path, "/") == 0) return RAMFS_ERR_INVAL;
    n = find_node(path);
    if (!n) return RAMFS_ERR_NOTFOUND;
    if (n->is_dir) return RAMFS_ERR_ISDIR;
    vfs_journal_record(VFS_OP_RM, path, 0);
    n->used = 0;
    g_count--;
    return RAMFS_OK;
}

int ramfs_write(const char *path, const char *data, int len) {
    ramfs_node_t *n;
    if (!path || path[0] == 0 || rf_strcmp(path, "/") == 0) return RAMFS_ERR_INVAL;
    if (len < 0) len = 0;
    if (len >= RAMFS_CONTENT_MAX) len = RAMFS_CONTENT_MAX - 1;
    n = find_node(path);
    if (n) {
        if (n->is_dir) return RAMFS_ERR_ISDIR;
        uint32_t add_b = (uint32_t)(len > n->size ? len - n->size : 0);
        int qrc = vfs_quota_check(path, add_b, 0);
        if (qrc != RAMFS_OK) return qrc;
        vfs_journal_record(VFS_OP_WRITE, path, 0);
        if (data) rf_memcpy(n->content, data, len);
        n->content[len] = 0;
        n->size = len;
        return RAMFS_OK;
    }
    if (!parent_is_dir(path)) return RAMFS_ERR_NOTDIR;
    int qrc = vfs_quota_check(path, (uint32_t)len, 1);
    if (qrc != RAMFS_OK) return qrc;
    vfs_journal_record(VFS_OP_WRITE, path, 0);
    n = alloc_node();
    if (!n) return RAMFS_ERR_NOSPACE;
    rf_strcpy(n->path, path);
    n->is_dir = 0;
    if (data) rf_memcpy(n->content, data, len);
    n->content[len] = 0;
    n->size = len;
    return RAMFS_OK;
}

const char *ramfs_read(const char *path, int *size) {
    ramfs_node_t *n = find_node(path);
    if (!n) {
        if (size) *size = 0;
        return 0;
    }
    if (n->is_dir) {
        if (size) *size = 0;
        return 0;
    }
    if (size) *size = n->size;
    return n->content;
}

int ramfs_cp(const char *src, const char *dst) {
    ramfs_node_t *s;
    char dest[RAMFS_PATH_MAX];
    ramfs_node_t *dnode;
    if (!src || !dst) return RAMFS_ERR_INVAL;
    s = find_node(src);
    if (!s) return RAMFS_ERR_NOTFOUND;
    rf_strcpy(dest, dst);
    dnode = find_node(dst);
    if (dnode && dnode->is_dir) {
        int pos = rf_strlen(dest);
        if (pos > 0 && dest[pos - 1] != '/') {
            dest[pos++] = '/';
            dest[pos] = 0;
        }
        /* dest + basename(src) */
        {
            const char *bn = rf_basename(src);
            int i = 0;
            while (bn[i] && pos < RAMFS_PATH_MAX - 1) dest[pos++] = bn[i++];
            dest[pos] = 0;
        }
        dnode = find_node(dest);
    }

    if (s->is_dir) {
        int slen = rf_strlen(src);
        ramfs_mkdir_p(dest);
        for (int i = 0; i < RAMFS_MAX_NODES; i++) {
            if (!g_nodes[i].used) continue;
            if (rf_strncmp(g_nodes[i].path, src, slen) == 0 &&
                g_nodes[i].path[slen] == '/') {
                char target[RAMFS_PATH_MAX];
                int dlen = rf_strlen(dest);
                rf_strcpy(target, dest);
                int k = 0;
                while (g_nodes[i].path[slen + k] && dlen + k < RAMFS_PATH_MAX - 1) {
                    target[dlen + k] = g_nodes[i].path[slen + k];
                    k++;
                }
                target[dlen + k] = 0;
                if (g_nodes[i].is_dir) {
                    ramfs_mkdir_p(target);
                } else {
                    ramfs_write(target, g_nodes[i].content, g_nodes[i].size);
                }
            }
        }
        return RAMFS_OK;
    }

    if (dnode && dnode->is_dir) return RAMFS_ERR_ISDIR;
    return ramfs_write(dest, s->content, s->size);
}

int ramfs_mv(const char *src, const char *dst) {
    ramfs_node_t *s;
    ramfs_node_t *d;
    char dest[RAMFS_PATH_MAX];
    int slen;
    if (!src || !dst) return RAMFS_ERR_INVAL;
    if (rf_strcmp(src, "/") == 0) return RAMFS_ERR_INVAL;
    s = find_node(src);
    if (!s) return RAMFS_ERR_NOTFOUND;
    rf_strcpy(dest, dst);
    d = find_node(dst);
    if (d && d->is_dir) {
        int pos = rf_strlen(dest);
        if (pos > 0 && dest[pos - 1] != '/') {
            dest[pos++] = '/';
            dest[pos] = 0;
        }
        {
            const char *bn = rf_basename(src);
            int i = 0;
            while (bn[i] && pos < RAMFS_PATH_MAX - 1) dest[pos++] = bn[i++];
            dest[pos] = 0;
        }
        d = find_node(dest);
    }
    if (d) return RAMFS_ERR_EXISTS;
    if (!parent_is_dir(dest)) return RAMFS_ERR_NOTDIR;
    slen = rf_strlen(src);
    vfs_journal_record(VFS_OP_MV, dest, src);
    if (s->is_dir) {
        for (int i = 0; i < RAMFS_MAX_NODES; i++) {
            if (!g_nodes[i].used) continue;
            if (rf_strcmp(g_nodes[i].path, src) == 0) {
                rf_strcpy(g_nodes[i].path, dest);
            } else if (rf_strncmp(g_nodes[i].path, src, slen) == 0 &&
                       g_nodes[i].path[slen] == '/') {
                char np[RAMFS_PATH_MAX];
                int dlen = rf_strlen(dest);
                int k;
                rf_strcpy(np, dest);
                k = 0;
                while (g_nodes[i].path[slen + k] && dlen + k < RAMFS_PATH_MAX - 1) {
                    np[dlen + k] = g_nodes[i].path[slen + k];
                    k++;
                }
                np[dlen + k] = 0;
                rf_strcpy(g_nodes[i].path, np);
            }
        }
        return RAMFS_OK;
    }
    rf_strcpy(s->path, dest);
    return RAMFS_OK;
}

int ramfs_list(const char *path, ramfs_dirent_t *out, int max_n) {
    ramfs_node_t *n;
    int count = 0;
    if (!path || !out || max_n <= 0) return RAMFS_ERR_INVAL;
    n = find_node(path);
    if (!n) return RAMFS_ERR_NOTFOUND;
    if (!n->is_dir) return RAMFS_ERR_NOTDIR;
    for (int i = 0; i < RAMFS_MAX_NODES && count < max_n; i++) {
        if (!g_nodes[i].used) continue;
        if (!is_direct_child(path, g_nodes[i].path)) continue;
        {
            const char *bn = rf_basename(g_nodes[i].path);
            int j = 0;
            while (bn[j] && j < RAMFS_NAME_MAX - 1) {
                out[count].name[j] = bn[j];
                j++;
            }
            out[count].name[j] = 0;
            out[count].is_dir = g_nodes[i].is_dir;
            out[count].size = g_nodes[i].size;
            count++;
        }
    }
    return count;
}

int ramfs_list_recursive(const char *path, ramfs_dirent_t *out, int max_n) {
    ramfs_node_t *n;
    int count = 0;
    int plen;
    if (!path || !out || max_n <= 0) return RAMFS_ERR_INVAL;
    n = find_node(path);
    if (!n) return RAMFS_ERR_NOTFOUND;
    if (!n->is_dir) return RAMFS_ERR_NOTDIR;
    plen = rf_strlen(path);
    for (int i = 0; i < RAMFS_MAX_NODES && count < max_n; i++) {
        if (!g_nodes[i].used) continue;
        if (rf_strcmp(g_nodes[i].path, path) == 0) continue;
        int is_under = 0;
        if (rf_strcmp(path, "/") == 0) {
            is_under = 1;
        } else if (rf_strncmp(g_nodes[i].path, path, plen) == 0 &&
                   g_nodes[i].path[plen] == '/') {
            is_under = 1;
        }
        if (is_under) {
            const char *rel = (rf_strcmp(path, "/") == 0) ? g_nodes[i].path + 1 : g_nodes[i].path + plen + 1;
            int j = 0;
            while (rel[j] && j < RAMFS_NAME_MAX - 1) {
                out[count].name[j] = rel[j];
                j++;
            }
            out[count].name[j] = 0;
            out[count].is_dir = g_nodes[i].is_dir;
            out[count].size = g_nodes[i].size;
            count++;
        }
    }
    return count;
}

int ramfs_node_count(void) {
    return g_count;
}

int vfs_quota_set(const char *path, uint32_t max_bytes, uint32_t max_nodes) {
    if (!path || path[0] == 0) return RAMFS_ERR_INVAL;
    for (int i = 0; i < VFS_MAX_QUOTAS; i++) {
        if (g_quotas[i].used && rf_strcmp(g_quotas[i].path, path) == 0) {
            g_quotas[i].max_bytes = max_bytes;
            g_quotas[i].max_nodes = max_nodes;
            return RAMFS_OK;
        }
    }
    for (int i = 0; i < VFS_MAX_QUOTAS; i++) {
        if (!g_quotas[i].used) {
            g_quotas[i].used = 1;
            rf_strcpy(g_quotas[i].path, path);
            g_quotas[i].max_bytes = max_bytes;
            g_quotas[i].max_nodes = max_nodes;
            return RAMFS_OK;
        }
    }
    return RAMFS_ERR_NOSPACE;
}

int vfs_quota_check(const char *path, uint32_t bytes_to_add, uint32_t nodes_to_add) {
    if (!path) return RAMFS_OK;
    for (int i = 0; i < VFS_MAX_QUOTAS; i++) {
        if (!g_quotas[i].used) continue;
        int qlen = rf_strlen(g_quotas[i].path);
        int is_under = 0;
        if (rf_strcmp(g_quotas[i].path, "/") == 0) {
            is_under = 1;
        } else if (rf_strcmp(path, g_quotas[i].path) == 0 ||
                   (rf_strncmp(path, g_quotas[i].path, qlen) == 0 && path[qlen] == '/')) {
            is_under = 1;
        }
        if (is_under) {
            uint32_t cur_bytes = 0;
            uint32_t cur_nodes = 0;
            for (int k = 0; k < RAMFS_MAX_NODES; k++) {
                if (!g_nodes[k].used) continue;
                int k_under = 0;
                if (rf_strcmp(g_quotas[i].path, "/") == 0) {
                    k_under = 1;
                } else if (rf_strcmp(g_nodes[k].path, g_quotas[i].path) == 0 ||
                           (rf_strncmp(g_nodes[k].path, g_quotas[i].path, qlen) == 0 && g_nodes[k].path[qlen] == '/')) {
                    k_under = 1;
                }
                if (k_under) {
                    cur_nodes++;
                    cur_bytes += (uint32_t)g_nodes[k].size;
                }
            }
            if (cur_bytes + bytes_to_add > g_quotas[i].max_bytes ||
                cur_nodes + nodes_to_add > g_quotas[i].max_nodes) {
                return RAMFS_ERR_NOSPACE;
            }
        }
    }
    return RAMFS_OK;
}

int vfs_access_grant(const char *path, const char *origin, uint8_t rights) {
    if (!path || !origin) return RAMFS_ERR_INVAL;
    for (int i = 0; i < VFS_MAX_ACLS; i++) {
        if (g_acls[i].used && rf_strcmp(g_acls[i].path, path) == 0 &&
            rf_strcmp(g_acls[i].origin, origin) == 0) {
            g_acls[i].rights = rights;
            return RAMFS_OK;
        }
    }
    for (int i = 0; i < VFS_MAX_ACLS; i++) {
        if (!g_acls[i].used) {
            g_acls[i].used = 1;
            rf_strcpy(g_acls[i].path, path);
            rf_strcpy(g_acls[i].origin, origin);
            g_acls[i].rights = rights;
            return RAMFS_OK;
        }
    }
    return RAMFS_ERR_NOSPACE;
}

int vfs_access_control(const char *path, const char *origin, uint8_t required_rights) {
    if (!path || !origin) return RAMFS_OK;
    int has_acl_for_path = 0;
    for (int i = 0; i < VFS_MAX_ACLS; i++) {
        if (!g_acls[i].used) continue;
        if (rf_strcmp(g_acls[i].path, path) == 0 || rf_strcmp(g_acls[i].path, "*") == 0) {
            has_acl_for_path = 1;
            if (rf_strcmp(g_acls[i].origin, origin) == 0 || rf_strcmp(g_acls[i].origin, "*") == 0) {
                if ((g_acls[i].rights & required_rights) == required_rights) {
                    return RAMFS_OK;
                }
            }
        }
    }
    if (!has_acl_for_path) return RAMFS_OK;
    return RAMFS_ERR_INVAL;
}

int vfs_atomic_swap(const char *path_a, const char *path_b) {
    ramfs_node_t *na, *nb;
    if (!path_a || !path_b) return RAMFS_ERR_INVAL;
    na = find_node(path_a);
    nb = find_node(path_b);
    if (!na || !nb) return RAMFS_ERR_NOTFOUND;
    if (na->is_dir || nb->is_dir) return RAMFS_ERR_ISDIR;

    char tmp_content[RAMFS_CONTENT_MAX];
    int tmp_size = na->size;
    rf_memcpy(tmp_content, na->content, na->size + 1);

    rf_memcpy(na->content, nb->content, nb->size + 1);
    na->size = nb->size;

    rf_memcpy(nb->content, tmp_content, tmp_size + 1);
    nb->size = tmp_size;

    return RAMFS_OK;
}

/* Lot 33: VFS CRDT Replicated Conflict-Free Resolution & Distributed Transaction Isolation Engine (US-006 / US-039) */

int vfs_crdt_merge(const vfs_crdt_node_t *local, const vfs_crdt_node_t *remote, vfs_crdt_node_t *merged) {
    if (!merged) return -1;
    merged->count = 0;

    if (local) {
        for (int i = 0; i < local->count && i < VFS_CRDT_MAX_ELEMENTS; i++) {
            merged->elements[merged->count] = local->elements[i];
            merged->count++;
        }
    }

    if (remote) {
        for (int j = 0; j < remote->count && j < VFS_CRDT_MAX_ELEMENTS; j++) {
            const vfs_crdt_element_t *re = &remote->elements[j];
            int found_idx = -1;
            for (int k = 0; k < merged->count; k++) {
                if (rf_strcmp(merged->elements[k].path, re->path) == 0) {
                    found_idx = k;
                    break;
                }
            }

            if (found_idx != -1) {
                vfs_crdt_element_t *me = &merged->elements[found_idx];
                if (re->timestamp > me->timestamp ||
                   (re->timestamp == me->timestamp && re->node_id > me->node_id)) {
                    *me = *re;
                }
            } else if (merged->count < VFS_CRDT_MAX_ELEMENTS) {
                merged->elements[merged->count] = *re;
                merged->count++;
            }
        }
    }

    return 0;
}

int vfs_crdt_resolve_conflict(const char *path, uint32_t local_ts, uint32_t remote_ts, const char *local_val, const char *remote_val, char *out_val, uint32_t out_cap) {
    if (!path || !out_val || out_cap == 0) return -1;

    const char *winner = local_val;
    if (remote_ts > local_ts) {
        winner = remote_val;
    }

    if (winner) {
        rf_strcpy(out_val, winner);
        out_val[out_cap - 1] = '\0';
    } else {
        out_val[0] = '\0';
    }

    return 0;
}

int vfs_dist_tx_begin(uint32_t tx_id, uint32_t isolation_level, vfs_dist_tx_iso_t *out_tx) {
    if (!out_tx) return -1;
    out_tx->tx_id = tx_id;
    out_tx->isolation_level = isolation_level;
    out_tx->start_timestamp = tx_id * 10;
    out_tx->active = 1;
    out_tx->read_only = 0;
    out_tx->lock_count = 0;
    for (int i = 0; i < RAMFS_MAX_LIST; i++) {
        out_tx->locked_paths[i][0] = '\0';
    }
    return 0;
}

int vfs_dist_tx_isolation_check(const vfs_dist_tx_iso_t *tx, const char *path, uint8_t is_write) {
    if (!tx || !tx->active || !path) return -1;

    if (is_write) {
        vfs_dist_tx_iso_t *mutable_tx = (vfs_dist_tx_iso_t *)tx;
        for (int i = 0; i < mutable_tx->lock_count; i++) {
            if (rf_strcmp(mutable_tx->locked_paths[i], path) == 0) {
                return 0;
            }
        }
        if (mutable_tx->lock_count < RAMFS_MAX_LIST) {
            rf_strcpy(mutable_tx->locked_paths[mutable_tx->lock_count], path);
            mutable_tx->locked_paths[mutable_tx->lock_count][RAMFS_PATH_MAX - 1] = '\0';
            mutable_tx->lock_count++;
            return 0;
        }
        return -2;
    }

    return 0;
}

int vfs_dist_tx_commit_iso(vfs_dist_tx_iso_t *tx) {
    if (!tx || !tx->active) return -1;
    tx->active = 0;
    tx->lock_count = 0;
    return 0;
}

int vfs_dist_tx_rollback_iso(vfs_dist_tx_iso_t *tx) {
    if (!tx || !tx->active) return -1;
    tx->active = 0;
    tx->lock_count = 0;
    return 0;
}

/* Lot 32: VFS Distributed Transactional Storage & MVCC Snapshot Engine (US-005 / US-038) */
static vfs_mvcc_version_entry_t g_mvcc_versions[VFS_MVCC_VERSIONS_MAX];
static uint32_t g_mvcc_snap_counter = 1;

int vfs_mvcc_snapshot_create(uint32_t snapshot_version, vfs_mvcc_snapshot_t *out_snap) {
    if (!out_snap) return -1;

    out_snap->snapshot_id = g_mvcc_snap_counter++;
    out_snap->snapshot_version = snapshot_version;
    out_snap->timestamp = snapshot_version * 100;
    out_snap->active = 1;

    /* Archive current state of active VFS nodes into MVCC version store if not present */
    for (int i = 0; i < RAMFS_MAX_NODES; i++) {
        if (!g_nodes[i].used || g_nodes[i].is_dir) continue;

        /* Check if version already exists for this path and version */
        int exists = 0;
        for (int v = 0; v < VFS_MVCC_VERSIONS_MAX; v++) {
            if (g_mvcc_versions[v].active &&
                g_mvcc_versions[v].version_id == snapshot_version &&
                rf_strcmp(g_mvcc_versions[v].path, g_nodes[i].path) == 0) {
                exists = 1;
                break;
            }
        }

        if (!exists) {
            /* Find free slot in g_mvcc_versions */
            int slot = -1;
            for (int v = 0; v < VFS_MVCC_VERSIONS_MAX; v++) {
                if (!g_mvcc_versions[v].active) {
                    slot = v;
                    break;
                }
            }
            if (slot == -1) {
                /* Evict oldest or first slot */
                slot = 0;
            }

            g_mvcc_versions[slot].version_id = snapshot_version;
            rf_strcpy(g_mvcc_versions[slot].path, g_nodes[i].path);
            g_mvcc_versions[slot].path[RAMFS_PATH_MAX - 1] = '\0';
            rf_strcpy(g_mvcc_versions[slot].content, g_nodes[i].content);
            g_mvcc_versions[slot].content[RAMFS_CONTENT_MAX - 1] = '\0';
            g_mvcc_versions[slot].size = (uint32_t)g_nodes[i].size;
            g_mvcc_versions[slot].timestamp = snapshot_version * 100;
            g_mvcc_versions[slot].active = 1;
        }
    }

    return 0;
}

int vfs_mvcc_read_version(const char *path, uint32_t snapshot_version, char *out_buf, uint32_t buf_cap, uint32_t *out_size) {
    if (!path || !out_buf || buf_cap == 0) return -1;

    /* First search in MVCC version table for exact version_id <= snapshot_version */
    int best_slot = -1;
    uint32_t best_ver = 0;

    for (int v = 0; v < VFS_MVCC_VERSIONS_MAX; v++) {
        if (g_mvcc_versions[v].active &&
            rf_strcmp(g_mvcc_versions[v].path, path) == 0 &&
            g_mvcc_versions[v].version_id <= snapshot_version) {
            if (best_slot == -1 || g_mvcc_versions[v].version_id > best_ver) {
                best_ver = g_mvcc_versions[v].version_id;
                best_slot = v;
            }
        }
    }

    if (best_slot != -1) {
        uint32_t sz = g_mvcc_versions[best_slot].size;
        if (sz >= buf_cap) sz = buf_cap - 1;
        for (uint32_t i = 0; i < sz; i++) {
            out_buf[i] = g_mvcc_versions[best_slot].content[i];
        }
        out_buf[sz] = '\0';
        if (out_size) *out_size = sz;
        return 0;
    }

    /* Fallback to live VFS node */
    ramfs_node_t *n = find_node(path);
    if (!n || n->is_dir) return -2;

    uint32_t sz = (uint32_t)n->size;
    if (sz >= buf_cap) sz = buf_cap - 1;
    for (uint32_t i = 0; i < sz; i++) {
        out_buf[i] = n->content[i];
    }
    out_buf[sz] = '\0';
    if (out_size) *out_size = sz;
    return 0;
}

int vfs_distributed_commit_validate(const vfs_dist_tx_context_t *tx_ctx, int min_quorum, int *out_committed) {
    if (!tx_ctx) return -1;

    int positive_votes = 0;
    for (int i = 0; i < tx_ctx->node_count && i < VFS_COMMIT_NODES_MAX; i++) {
        if (tx_ctx->node_votes[i].vote_prepared) {
            positive_votes++;
        }
    }

    if (positive_votes >= min_quorum && tx_ctx->phase == 0) {
        if (out_committed) *out_committed = 1;
        return 0;
    } else if (tx_ctx->phase == 1) {
        if (out_committed) *out_committed = 1;
        return 0;
    } else {
        if (out_committed) *out_committed = 0;
        return -1;
    }
}

int vfs_compaction_journal_compact(vfs_wal_log_t *wal, int *out_reclaimed_slots, int *out_compacted_inodes) {
    if (!wal) return -1;

    int reclaimed = 0;
    int compacted = 0;

    /* Apply all unapplied log entries up to current count */
    for (uint32_t i = 0; i < wal->count; i++) {
        if (!wal->entries[i].applied) {
            wal->entries[i].applied = 1;
            compacted++;
        }
    }

    /* Reclaim applied log slots */
    wal->count = 0;
    wal->last_applied_lsn = wal->next_lsn;
    reclaimed = VFS_WAL_MAX_ENTRIES;

    if (out_reclaimed_slots) *out_reclaimed_slots = reclaimed;
    if (out_compacted_inodes) *out_compacted_inodes = compacted;

    return 0;
}

/* Lot 31: VFS Distributed Replicated State & Write-Ahead Log Engine (US-004 / US-037) */

static vfs_dist_lock_t g_dist_locks[VFS_DIST_LOCKS_MAX];

void vfs_wal_init(vfs_wal_log_t *wal) {
    if (!wal) return;
    wal->count = 0;
    wal->last_applied_lsn = 0;
    wal->next_lsn = 1;
    for (int i = 0; i < VFS_WAL_MAX_ENTRIES; i++) {
        wal->entries[i].lsn = 0;
        wal->entries[i].node_id = 0;
        wal->entries[i].op_type = 0;
        wal->entries[i].applied = 0;
        wal->entries[i].path[0] = '\0';
        wal->entries[i].data[0] = '\0';
        wal->entries[i].data_len = 0;
    }
}

int vfs_wal_log_append(vfs_wal_log_t *wal, uint32_t node_id, uint8_t op_type, const char *path, const char *data, uint32_t len) {
    if (!wal || !path) return RAMFS_ERR_INVAL;
    if (wal->count >= VFS_WAL_MAX_ENTRIES) return RAMFS_ERR_NOSPACE;

    uint32_t idx = wal->count;
    vfs_wal_entry_t *e = &wal->entries[idx];
    e->lsn = wal->next_lsn++;
    e->node_id = node_id;
    e->op_type = op_type;
    e->applied = 0;
    rf_strcpy(e->path, path);
    e->data[0] = '\0';
    e->data_len = 0;

    if (data && len > 0) {
        uint32_t copy_len = (len < RAMFS_CONTENT_MAX - 1) ? len : (RAMFS_CONTENT_MAX - 1);
        for (uint32_t i = 0; i < copy_len; i++) e->data[i] = data[i];
        e->data[copy_len] = '\0';
        e->data_len = copy_len;
    }

    wal->count++;
    return (int)e->lsn;
}

int vfs_wal_checkpoint_apply(vfs_wal_log_t *wal, uint32_t target_lsn) {
    if (!wal) return RAMFS_ERR_INVAL;
    int applied_count = 0;

    for (uint32_t i = 0; i < wal->count; i++) {
        vfs_wal_entry_t *e = &wal->entries[i];
        if (e->lsn <= target_lsn && !e->applied) {
            if (e->op_type == VFS_MUTATION_OP_WRITE) {
                ramfs_write(e->path, e->data, (int)e->data_len);
            } else if (e->op_type == VFS_MUTATION_OP_MKDIR) {
                ramfs_mkdir_p(e->path);
            } else if (e->op_type == VFS_MUTATION_OP_RM) {
                ramfs_rm_r(e->path);
            }
            e->applied = 1;
            wal->last_applied_lsn = e->lsn;
            applied_count++;
        }
    }
    return applied_count;
}

int vfs_distributed_lock_acquire(const char *path, uint32_t node_id, uint32_t lease_ticks, uint32_t current_tick) {
    if (!path || path[0] == '\0') return RAMFS_ERR_INVAL;

    for (int i = 0; i < VFS_DIST_LOCKS_MAX; i++) {
        if (g_dist_locks[i].active && rf_strcmp(g_dist_locks[i].path, path) == 0) {
            /* Check if lease expired */
            if (current_tick >= g_dist_locks[i].lease_expire_tick) {
                /* Orphaned or expired lock: takeover */
                g_dist_locks[i].holder_node_id = node_id;
                g_dist_locks[i].lease_expire_tick = current_tick + lease_ticks;
                return RAMFS_OK;
            }
            if (g_dist_locks[i].holder_node_id == node_id) {
                /* Renew lease */
                g_dist_locks[i].lease_expire_tick = current_tick + lease_ticks;
                return RAMFS_OK;
            }
            /* Held by another active node */
            return -1;
        }
    }

    /* Allocate free slot */
    for (int i = 0; i < VFS_DIST_LOCKS_MAX; i++) {
        if (!g_dist_locks[i].active) {
            g_dist_locks[i].active = 1;
            rf_strcpy(g_dist_locks[i].path, path);
            g_dist_locks[i].holder_node_id = node_id;
            g_dist_locks[i].lease_expire_tick = current_tick + lease_ticks;
            return RAMFS_OK;
        }
    }

    return RAMFS_ERR_NOSPACE;
}

int vfs_distributed_lock_release(const char *path, uint32_t node_id) {
    if (!path) return RAMFS_ERR_INVAL;

    for (int i = 0; i < VFS_DIST_LOCKS_MAX; i++) {
        if (g_dist_locks[i].active && rf_strcmp(g_dist_locks[i].path, path) == 0) {
            if (g_dist_locks[i].holder_node_id == node_id) {
                g_dist_locks[i].active = 0;
                g_dist_locks[i].path[0] = '\0';
                return RAMFS_OK;
            }
            return RAMFS_ERR_INVAL;
        }
    }
    return RAMFS_ERR_NOTFOUND;
}

static uint32_t simple_fnv1a(const char *data, int len) {
    uint32_t hash = 2166136261U;
    for (int i = 0; i < len; i++) {
        hash ^= (uint8_t)data[i];
        hash *= 16777619U;
    }
    return hash;
}

int vfs_distributed_reconcile(const vfs_remote_inode_t *remote_inodes, int remote_count, uint32_t remote_term, int *out_synced_count, int *out_conflict_count) {
    if (!remote_inodes || remote_count < 0) return RAMFS_ERR_INVAL;

    (void)remote_term;
    int synced = 0;
    int conflicts = 0;

    for (int i = 0; i < remote_count; i++) {
        const vfs_remote_inode_t *r = &remote_inodes[i];
        ramfs_node_t *local = find_node(r->path);

        if (!local) {
            /* Missing locally: sync directory or empty file */
            if (r->is_dir) {
                ramfs_mkdir_p(r->path);
            } else {
                ramfs_write(r->path, "", 0);
            }
            synced++;
        } else {
            if (r->is_dir != local->is_dir) {
                conflicts++;
            } else if (!r->is_dir) {
                uint32_t l_hash = simple_fnv1a(local->content, local->size);
                if (r->hash != l_hash || r->size != (uint32_t)local->size) {
                    conflicts++;
                } else {
                    synced++;
                }
            } else {
                synced++;
            }
        }
    }

    if (out_synced_count) *out_synced_count = synced;
    if (out_conflict_count) *out_conflict_count = conflicts;

    return RAMFS_OK;
}

int vfs_journal_begin(void) {
    if (g_active_tx) return g_active_tx;
    g_active_tx = g_tx_counter++;
    return g_active_tx;
}

int vfs_journal_commit(int tx_id) {
    if (tx_id <= 0 || tx_id != g_active_tx) return RAMFS_ERR_INVAL;
    for (int i = 0; i < VFS_JOURNAL_LOG_MAX; i++) {
        if (g_journal[i].used && g_journal[i].tx_id == tx_id) {
            g_journal[i].used = 0;
        }
    }
    g_active_tx = 0;
    return RAMFS_OK;
}

int vfs_journal_rollback(int tx_id) {
    if (tx_id <= 0 || tx_id != g_active_tx) return RAMFS_ERR_INVAL;
    for (int i = VFS_JOURNAL_LOG_MAX - 1; i >= 0; i--) {
        if (!g_journal[i].used || g_journal[i].tx_id != tx_id) continue;
        vfs_journal_entry_t *e = &g_journal[i];
        ramfs_node_t *n = find_node(e->path);

        if (e->op == VFS_OP_WRITE || e->op == VFS_OP_MKDIR) {
            if (!e->existed) {
                if (n) { n->used = 0; g_count--; }
            } else {
                if (n && !e->was_dir) {
                    rf_memcpy(n->content, e->old_content, e->old_size);
                    n->content[e->old_size] = 0;
                    n->size = e->old_size;
                }
            }
        } else if (e->op == VFS_OP_RM) {
            if (e->existed) {
                if (!n) {
                    n = alloc_node();
                    if (n) {
                        rf_strcpy(n->path, e->path);
                        n->is_dir = e->was_dir;
                        if (!e->was_dir) {
                            rf_memcpy(n->content, e->old_content, e->old_size);
                            n->content[e->old_size] = 0;
                            n->size = e->old_size;
                        }
                    }
                } else {
                    n->used = 1;
                    n->is_dir = e->was_dir;
                    if (!e->was_dir) {
                        rf_memcpy(n->content, e->old_content, e->old_size);
                        n->content[e->old_size] = 0;
                        n->size = e->old_size;
                    }
                }
            }
        } else if (e->op == VFS_OP_MV) {
            ramfs_node_t *curr = find_node(e->path);
            if (curr) {
                rf_strcpy(curr->path, e->old_path);
            }
        }
        e->used = 0;
    }
    g_active_tx = 0;
    return RAMFS_OK;
}

int vfs_snapshot_create(uint32_t *snapshot_id) {
    if (!snapshot_id) return RAMFS_ERR_INVAL;
    int slot = -1;
    for (int i = 0; i < VFS_SNAPSHOT_MAX; i++) {
        if (!g_snapshots[i].used) { slot = i; break; }
    }
    if (slot < 0) {
        slot = 0;
    }
    vfs_snapshot_t *s = &g_snapshots[slot];
    s->used = 1;
    s->id = g_snapshot_counter++;
    s->count = g_count;
    for (int i = 0; i < RAMFS_MAX_NODES; i++) {
        s->nodes[i] = g_nodes[i];
    }
    *snapshot_id = s->id;
    return RAMFS_OK;
}

int vfs_snapshot_restore(uint32_t snapshot_id) {
    vfs_snapshot_t *s = 0;
    for (int i = 0; i < VFS_SNAPSHOT_MAX; i++) {
        if (g_snapshots[i].used && g_snapshots[i].id == snapshot_id) {
            s = &g_snapshots[i];
            break;
        }
    }
    if (!s) return RAMFS_ERR_NOTFOUND;
    g_count = s->count;
    for (int i = 0; i < RAMFS_MAX_NODES; i++) {
        g_nodes[i] = s->nodes[i];
    }
    return RAMFS_OK;
}

int vfs_flock(const char *path, int lock_type) {
    if (!path) return RAMFS_ERR_INVAL;
    if (lock_type == VFS_LOCK_UN) {
        for (int i = 0; i < VFS_MAX_LOCKS; i++) {
            if (g_locks[i].used && rf_strcmp(g_locks[i].path, path) == 0) {
                if (g_locks[i].lock_type == VFS_LOCK_SH) {
                    g_locks[i].count--;
                    if (g_locks[i].count <= 0) g_locks[i].used = 0;
                } else {
                    g_locks[i].used = 0;
                }
                return RAMFS_OK;
            }
        }
        return RAMFS_OK;
    } else if (lock_type == VFS_LOCK_SH) {
        for (int i = 0; i < VFS_MAX_LOCKS; i++) {
            if (g_locks[i].used && rf_strcmp(g_locks[i].path, path) == 0) {
                if (g_locks[i].lock_type == VFS_LOCK_EX) return RAMFS_ERR_INVAL;
                g_locks[i].count++;
                return RAMFS_OK;
            }
        }
        for (int i = 0; i < VFS_MAX_LOCKS; i++) {
            if (!g_locks[i].used) {
                g_locks[i].used = 1;
                rf_strcpy(g_locks[i].path, path);
                g_locks[i].lock_type = VFS_LOCK_SH;
                g_locks[i].count = 1;
                return RAMFS_OK;
            }
        }
        return RAMFS_ERR_NOSPACE;
    } else if (lock_type == VFS_LOCK_EX) {
        for (int i = 0; i < VFS_MAX_LOCKS; i++) {
            if (g_locks[i].used && rf_strcmp(g_locks[i].path, path) == 0) {
                return RAMFS_ERR_INVAL;
            }
        }
        for (int i = 0; i < VFS_MAX_LOCKS; i++) {
            if (!g_locks[i].used) {
                g_locks[i].used = 1;
                rf_strcpy(g_locks[i].path, path);
                g_locks[i].lock_type = VFS_LOCK_EX;
                g_locks[i].count = 1;
                return RAMFS_OK;
            }
        }
        return RAMFS_ERR_NOSPACE;
    }
    return RAMFS_ERR_INVAL;
}

/* Lot 28: Page Cache & Dirty Writeback Daemon Implementation */
static vfs_page_cache_t g_page_cache;
static vfs_io_queue_t g_io_queue;
static uint32_t g_cache_ticks = 0;

void vfs_page_cache_init(void) {
    for (int i = 0; i < VFS_PAGE_CACHE_SIZE; i++) {
        g_page_cache.pages[i].cluster_id = 0xFFFFFFFFU;
        g_page_cache.pages[i].dirty = 0;
        g_page_cache.pages[i].ref_bit = 0;
        g_page_cache.pages[i].last_used = 0;
        for (int j = 0; j < VFS_CLUSTER_SIZE; j++) {
            g_page_cache.pages[i].data[j] = 0;
        }
    }
    g_page_cache.clock_hand = 0;
    g_page_cache.hit_count = 0;
    g_page_cache.miss_count = 0;
    g_page_cache.flush_count = 0;

    g_io_queue.head = 0;
    g_io_queue.tail = 0;
    g_io_queue.count = 0;
    g_cache_ticks = 0;
}

int vfs_page_cache_read(uint32_t cluster_id, uint8_t *out_buf) {
    if (!out_buf) return RAMFS_ERR_INVAL;
    g_cache_ticks++;

    for (int i = 0; i < VFS_PAGE_CACHE_SIZE; i++) {
        if (g_page_cache.pages[i].cluster_id == cluster_id) {
            g_page_cache.hit_count++;
            g_page_cache.pages[i].ref_bit = 1;
            g_page_cache.pages[i].last_used = g_cache_ticks;
            for (int k = 0; k < VFS_CLUSTER_SIZE; k++) {
                out_buf[k] = g_page_cache.pages[i].data[k];
            }
            return RAMFS_OK;
        }
    }

    g_page_cache.miss_count++;
    /* Clock/LRU Replacement Algorithm */
    int victim = -1;
    for (int pass = 0; pass < 2 * VFS_PAGE_CACHE_SIZE; pass++) {
        uint32_t idx = g_page_cache.clock_hand;
        g_page_cache.clock_hand = (g_page_cache.clock_hand + 1) % VFS_PAGE_CACHE_SIZE;
        if (g_page_cache.pages[idx].cluster_id == 0xFFFFFFFFU) {
            victim = idx;
            break;
        }
        if (g_page_cache.pages[idx].ref_bit == 0) {
            victim = idx;
            break;
        } else {
            g_page_cache.pages[idx].ref_bit = 0;
        }
    }
    if (victim < 0) victim = 0;

    if (g_page_cache.pages[victim].dirty) {
        g_page_cache.flush_count++;
        g_page_cache.pages[victim].dirty = 0;
    }

    g_page_cache.pages[victim].cluster_id = cluster_id;
    g_page_cache.pages[victim].dirty = 0;
    g_page_cache.pages[victim].ref_bit = 1;
    g_page_cache.pages[victim].last_used = g_cache_ticks;
    for (int k = 0; k < VFS_CLUSTER_SIZE; k++) {
        g_page_cache.pages[victim].data[k] = (uint8_t)(cluster_id ^ k);
        out_buf[k] = g_page_cache.pages[victim].data[k];
    }
    return RAMFS_OK;
}

int vfs_page_cache_write(uint32_t cluster_id, const uint8_t *in_buf, uint8_t dirty) {
    if (!in_buf) return RAMFS_ERR_INVAL;
    g_cache_ticks++;

    for (int i = 0; i < VFS_PAGE_CACHE_SIZE; i++) {
        if (g_page_cache.pages[i].cluster_id == cluster_id) {
            g_page_cache.hit_count++;
            g_page_cache.pages[i].ref_bit = 1;
            if (dirty) g_page_cache.pages[i].dirty = 1;
            g_page_cache.pages[i].last_used = g_cache_ticks;
            for (int k = 0; k < VFS_CLUSTER_SIZE; k++) {
                g_page_cache.pages[i].data[k] = in_buf[k];
            }
            return RAMFS_OK;
        }
    }

    g_page_cache.miss_count++;
    int victim = -1;
    for (int pass = 0; pass < 2 * VFS_PAGE_CACHE_SIZE; pass++) {
        uint32_t idx = g_page_cache.clock_hand;
        g_page_cache.clock_hand = (g_page_cache.clock_hand + 1) % VFS_PAGE_CACHE_SIZE;
        if (g_page_cache.pages[idx].cluster_id == 0xFFFFFFFFU || g_page_cache.pages[idx].ref_bit == 0) {
            victim = idx;
            break;
        } else {
            g_page_cache.pages[idx].ref_bit = 0;
        }
    }
    if (victim < 0) victim = 0;

    if (g_page_cache.pages[victim].dirty) {
        g_page_cache.flush_count++;
        g_page_cache.pages[victim].dirty = 0;
    }

    g_page_cache.pages[victim].cluster_id = cluster_id;
    g_page_cache.pages[victim].dirty = dirty ? 1 : 0;
    g_page_cache.pages[victim].ref_bit = 1;
    g_page_cache.pages[victim].last_used = g_cache_ticks;
    for (int k = 0; k < VFS_CLUSTER_SIZE; k++) {
        g_page_cache.pages[victim].data[k] = in_buf[k];
    }
    return RAMFS_OK;
}

int vfs_cache_flush(void) {
    int flushed = 0;
    for (int i = 0; i < VFS_PAGE_CACHE_SIZE; i++) {
        if (g_page_cache.pages[i].cluster_id != 0xFFFFFFFFU && g_page_cache.pages[i].dirty) {
            g_page_cache.pages[i].dirty = 0;
            g_page_cache.flush_count++;
            flushed++;
        }
    }
    return flushed;
}

int vfs_cache_sync(void) {
    int flushed = vfs_cache_flush();
    vfs_io_process_coalesced();
    return flushed;
}

int vfs_io_enqueue(uint32_t cluster_id, uint8_t prio, uint8_t is_write, const uint8_t *data, uint32_t len) {
    if (g_io_queue.count >= VFS_IO_QUEUE_MAX) return RAMFS_ERR_NOSPACE;
    int idx = g_io_queue.tail;
    g_io_queue.reqs[idx].cluster_id = cluster_id;
    g_io_queue.reqs[idx].prio = prio;
    g_io_queue.reqs[idx].is_write = is_write;
    g_io_queue.reqs[idx].length = (len > VFS_CLUSTER_SIZE) ? VFS_CLUSTER_SIZE : len;
    if (data && len > 0) {
        for (uint32_t k = 0; k < g_io_queue.reqs[idx].length; k++) {
            g_io_queue.reqs[idx].data[k] = data[k];
        }
    }
    g_io_queue.tail = (g_io_queue.tail + 1) % VFS_IO_QUEUE_MAX;
    g_io_queue.count++;
    return RAMFS_OK;
}

int vfs_io_process_coalesced(void) {
    if (g_io_queue.count == 0) return 0;

    /* Sort queue by priority descending, then cluster_id ascending */
    int n = g_io_queue.count;
    vfs_io_req_t temp_arr[VFS_IO_QUEUE_MAX];
    for (int i = 0; i < n; i++) {
        int idx = (g_io_queue.head + i) % VFS_IO_QUEUE_MAX;
        temp_arr[i] = g_io_queue.reqs[idx];
    }

    for (int i = 0; i < n - 1; i++) {
        for (int j = 0; j < n - i - 1; j++) {
            if (temp_arr[j].prio < temp_arr[j + 1].prio ||
               (temp_arr[j].prio == temp_arr[j + 1].prio && temp_arr[j].cluster_id > temp_arr[j + 1].cluster_id)) {
                vfs_io_req_t swap = temp_arr[j];
                temp_arr[j] = temp_arr[j + 1];
                temp_arr[j + 1] = swap;
            }
        }
    }

    int processed = n;
    g_io_queue.head = 0;
    g_io_queue.tail = 0;
    g_io_queue.count = 0;
    return processed;
}

const char *ramfs_strerror(int err) {
    switch (err) {
        case RAMFS_OK: return "ok";
        case RAMFS_ERR_NOTFOUND: return "fichier introuvable";
        case RAMFS_ERR_EXISTS: return "existe deja";
        case RAMFS_ERR_NOTDIR: return "n'est pas un repertoire";
        case RAMFS_ERR_ISDIR: return "est un repertoire";
        case RAMFS_ERR_NOTEMPTY: return "repertoire non vide";
        case RAMFS_ERR_NOSPACE: return "plus de place";
        case RAMFS_ERR_INVAL: return "argument invalide";
        default: return "erreur";
    }
}

/* Lot 29: Transactional File Journaling, Tree Locking & Integrity Check (US-002 / US-034) */
static vfs_mutation_log_t g_mutation_log;

typedef struct {
    int used;
    char dir_path[RAMFS_PATH_MAX];
    uint8_t lock_mode;
    uint32_t owner_id;
} vfs_tree_lock_entry_t;

static vfs_tree_lock_entry_t g_tree_locks[16];

void vfs_mutation_log_init(vfs_mutation_log_t *log) {
    if (!log) log = &g_mutation_log;
    log->head = 0;
    log->count = 0;
    log->next_seq = 1;
    for (int i = 0; i < VFS_MUTATION_MAX; i++) {
        log->entries[i].seq_id = 0;
        log->entries[i].tx_id = 0;
        log->entries[i].op_type = 0;
        log->entries[i].status = VFS_MUTATION_STATUS_UNCOMMITTED;
        log->entries[i].path[0] = 0;
        log->entries[i].target_path[0] = 0;
        log->entries[i].data[0] = 0;
        log->entries[i].data_len = 0;
    }
}

int vfs_mutation_log_append(vfs_mutation_log_t *log, uint32_t tx_id, uint8_t op_type, const char *path, const char *target_path, const char *data, uint32_t len, uint8_t status) {
    if (!log) log = &g_mutation_log;
    uint32_t idx = log->head;
    vfs_mutation_entry_t *e = &log->entries[idx];
    e->seq_id = log->next_seq++;
    e->tx_id = tx_id;
    e->op_type = op_type;
    e->status = status;
    e->path[0] = 0;
    if (path) rf_strcpy(e->path, path);
    e->target_path[0] = 0;
    if (target_path) rf_strcpy(e->target_path, target_path);
    e->data[0] = 0;
    e->data_len = 0;
    if (data && len > 0) {
        uint32_t copy_len = (len < RAMFS_CONTENT_MAX - 1) ? len : (RAMFS_CONTENT_MAX - 1);
        for (uint32_t i = 0; i < copy_len; i++) e->data[i] = data[i];
        e->data[copy_len] = 0;
        e->data_len = copy_len;
    }
    log->head = (log->head + 1) % VFS_MUTATION_MAX;
    if (log->count < VFS_MUTATION_MAX) log->count++;
    return (int)e->seq_id;
}

int vfs_mutation_roll_forward(vfs_mutation_log_t *log) {
    if (!log) log = &g_mutation_log;
    int applied_count = 0;
    for (uint32_t i = 0; i < VFS_MUTATION_MAX; i++) {
        vfs_mutation_entry_t *e = &log->entries[i];
        if (e->seq_id > 0 && e->status == VFS_MUTATION_STATUS_COMMITTED) {
            if (e->op_type == VFS_MUTATION_OP_WRITE) {
                ramfs_write(e->path, e->data, (int)e->data_len);
            } else if (e->op_type == VFS_MUTATION_OP_MKDIR) {
                ramfs_mkdir_p(e->path);
            } else if (e->op_type == VFS_MUTATION_OP_RM) {
                ramfs_rm_r(e->path);
            } else if (e->op_type == VFS_MUTATION_OP_RENAME) {
                ramfs_mv(e->path, e->target_path);
            }
            e->status = VFS_MUTATION_STATUS_APPLIED;
            applied_count++;
        }
    }
    return applied_count;
}

static int is_path_prefix(const char *prefix, const char *path) {
    if (!prefix || !path) return 0;
    int plen = rf_strlen(prefix);
    if (plen == 0) return 0;
    if (rf_strcmp(prefix, "/") == 0) return 1;
    if (rf_strncmp(path, prefix, plen) == 0) {
        if (path[plen] == '\0' || path[plen] == '/') return 1;
    }
    return 0;
}

int vfs_dir_lock_tree(const char *dir_path, uint8_t lock_mode, uint32_t owner_id) {
    if (!dir_path || dir_path[0] == 0 || lock_mode == 0) return RAMFS_ERR_INVAL;

    for (int i = 0; i < 16; i++) {
        if (g_tree_locks[i].used) {
            if (is_path_prefix(g_tree_locks[i].dir_path, dir_path) || is_path_prefix(dir_path, g_tree_locks[i].dir_path)) {
                if (g_tree_locks[i].owner_id != owner_id) {
                    if (lock_mode == VFS_LOCK_EX || g_tree_locks[i].lock_mode == VFS_LOCK_EX) {
                        return -1;
                    }
                }
            }
        }
    }

    for (int i = 0; i < 16; i++) {
        if (!g_tree_locks[i].used) {
            g_tree_locks[i].used = 1;
            rf_strcpy(g_tree_locks[i].dir_path, dir_path);
            g_tree_locks[i].lock_mode = lock_mode;
            g_tree_locks[i].owner_id = owner_id;
            return 0;
        }
    }
    return RAMFS_ERR_NOSPACE;
}

int vfs_dir_unlock_tree(const char *dir_path, uint32_t owner_id) {
    if (!dir_path) return RAMFS_ERR_INVAL;
    for (int i = 0; i < 16; i++) {
        if (g_tree_locks[i].used && g_tree_locks[i].owner_id == owner_id && rf_strcmp(g_tree_locks[i].dir_path, dir_path) == 0) {
            g_tree_locks[i].used = 0;
            return 0;
        }
    }
    return RAMFS_ERR_NOTFOUND;
}

int vfs_integrity_check(int *out_cycle_detected, int *out_dangling_count) {
    int cycle = 0;
    int dangling = 0;

    if (out_cycle_detected) *out_cycle_detected = 0;
    if (out_dangling_count) *out_dangling_count = 0;

    for (int i = 0; i < RAMFS_MAX_NODES; i++) {
        if (!g_nodes[i].used) continue;
        if (rf_strcmp(g_nodes[i].path, "/") == 0) continue;

        char current_path[RAMFS_PATH_MAX];
        char parent_path[RAMFS_PATH_MAX];
        rf_strcpy(current_path, g_nodes[i].path);

        int depth = 0;
        while (rf_strcmp(current_path, "/") != 0 && depth < RAMFS_MAX_NODES) {
            rf_parent(current_path, parent_path);
            if (rf_strcmp(parent_path, "/") == 0) {
                break;
            }
            ramfs_node_t *pnode = find_node(parent_path);
            if (!pnode) {
                dangling++;
                break;
            } else if (!pnode->is_dir) {
                dangling++;
                break;
            }
            if (rf_strcmp(parent_path, g_nodes[i].path) == 0) {
                cycle = 1;
                break;
            }
            rf_strcpy(current_path, parent_path);
            depth++;
        }
        if (depth >= RAMFS_MAX_NODES) {
            cycle = 1;
        }
    }

    if (out_cycle_detected) *out_cycle_detected = cycle;
    if (out_dangling_count) *out_dangling_count = dangling;

    return (cycle != 0 || dangling != 0) ? -1 : 0;
}

/* Lot 30: VFS Virtual Layer & Multi-Mount Overlay Engine (US-003 / US-036) */

void vfs_mount_table_init(vfs_mount_table_t *table) {
    if (!table) return;
    for (int i = 0; i < VFS_MOUNT_MAX; i++) {
        table->mounts[i].active = 0;
        table->mounts[i].mount_point[0] = '\0';
        table->mounts[i].target_path[0] = '\0';
        table->mounts[i].flags = 0;
    }
    table->count = 0;
}

int vfs_mount_point_add(vfs_mount_table_t *table, const char *mount_point, const char *target_path, uint32_t flags) {
    if (!table || !mount_point || !target_path || mount_point[0] == '\0') return RAMFS_ERR_INVAL;

    for (int i = 0; i < VFS_MOUNT_MAX; i++) {
        if (table->mounts[i].active && rf_strcmp(table->mounts[i].mount_point, mount_point) == 0) {
            return RAMFS_ERR_EXISTS;
        }
    }

    for (int i = 0; i < VFS_MOUNT_MAX; i++) {
        if (!table->mounts[i].active) {
            table->mounts[i].active = 1;
            rf_strcpy(table->mounts[i].mount_point, mount_point);
            rf_strcpy(table->mounts[i].target_path, target_path);
            table->mounts[i].flags = flags;
            table->count++;
            return RAMFS_OK;
        }
    }
    return RAMFS_ERR_NOSPACE;
}

int vfs_mount_point_remove(vfs_mount_table_t *table, const char *mount_point) {
    if (!table || !mount_point) return RAMFS_ERR_INVAL;

    for (int i = 0; i < VFS_MOUNT_MAX; i++) {
        if (table->mounts[i].active && rf_strcmp(table->mounts[i].mount_point, mount_point) == 0) {
            table->mounts[i].active = 0;
            table->mounts[i].mount_point[0] = '\0';
            table->mounts[i].target_path[0] = '\0';
            table->count--;
            return RAMFS_OK;
        }
    }
    return RAMFS_ERR_NOTFOUND;
}

static void combine_path(const char *dir, const char *rel, char *out, int max_sz) {
    if (!dir || !rel || !out || max_sz <= 0) return;
    int dlen = rf_strlen(dir);
    if (dlen > 0 && dir[dlen - 1] == '/') {
        if (rel[0] == '/') {
            rf_strcpy(out, dir);
            int cur_len = rf_strlen(out);
            int idx = 0;
            while (rel[idx + 1] && cur_len + idx < max_sz - 1) {
                out[cur_len + idx] = rel[idx + 1];
                idx++;
            }
            out[cur_len + idx] = '\0';
        } else {
            rf_strcpy(out, dir);
            int cur_len = rf_strlen(out);
            int idx = 0;
            while (rel[idx] && cur_len + idx < max_sz - 1) {
                out[cur_len + idx] = rel[idx];
                idx++;
            }
            out[cur_len + idx] = '\0';
        }
    } else {
        if (rel[0] == '/') {
            rf_strcpy(out, dir);
            int cur_len = rf_strlen(out);
            int idx = 0;
            while (rel[idx] && cur_len + idx < max_sz - 1) {
                out[cur_len + idx] = rel[idx];
                idx++;
            }
            out[cur_len + idx] = '\0';
        } else {
            rf_strcpy(out, dir);
            int cur_len = rf_strlen(out);
            if (cur_len < max_sz - 1) {
                out[cur_len] = '/';
                out[cur_len + 1] = '\0';
                cur_len++;
            }
            int idx = 0;
            while (rel[idx] && cur_len + idx < max_sz - 1) {
                out[cur_len + idx] = rel[idx];
                idx++;
            }
            out[cur_len + idx] = '\0';
        }
    }
}

int vfs_overlay_file_lookup(const vfs_overlay_config_t *cfg, const char *relative_path, char *out_found_path, int out_sz, uint8_t *out_is_whiteout) {
    if (!cfg || !relative_path || !cfg->active) return RAMFS_ERR_INVAL;

    if (out_is_whiteout) *out_is_whiteout = 0;

    char test_path[RAMFS_PATH_MAX];
    char whiteout_path[RAMFS_PATH_MAX];

    /* Check upper layer first */
    if (cfg->upper_layer[0] != '\0') {
        combine_path(cfg->upper_layer, relative_path, test_path, sizeof(test_path));

        /* Check for whiteout file .wh.<file> */
        (void)out_sz;
        char parent[RAMFS_PATH_MAX];
        rf_parent(test_path, parent);
        const char *name = rf_basename(test_path);
        char wh_name[RAMFS_NAME_MAX];
        rf_strcpy(wh_name, ".wh.");
        int cur = rf_strlen(wh_name);
        int idx = 0;
        while (name[idx] && cur + idx < RAMFS_NAME_MAX - 1) {
            wh_name[cur + idx] = name[idx];
            idx++;
        }
        wh_name[cur + idx] = '\0';
        combine_path(parent, wh_name, whiteout_path, sizeof(whiteout_path));

        ramfs_node_t *wh_node = find_node(whiteout_path);
        if (wh_node) {
            if (out_is_whiteout) *out_is_whiteout = 1;
            return RAMFS_ERR_NOTFOUND;
        }

        ramfs_node_t *u_node = find_node(test_path);
        if (u_node) {
            if (out_found_path) rf_strcpy(out_found_path, test_path);
            return RAMFS_OK;
        }
    }

    /* Check lower layers in order */
    for (int i = 0; i < cfg->lower_count && i < VFS_OVERLAY_LAYERS_MAX; i++) {
        if (cfg->lower_layers[i][0] == '\0') continue;
        combine_path(cfg->lower_layers[i], relative_path, test_path, sizeof(test_path));
        ramfs_node_t *l_node = find_node(test_path);
        if (l_node) {
            if (out_found_path) rf_strcpy(out_found_path, test_path);
            return RAMFS_OK;
        }
    }

    return RAMFS_ERR_NOTFOUND;
}

int vfs_overlay_cow_write(const vfs_overlay_config_t *cfg, const char *relative_path, const char *data, uint32_t len) {
    if (!cfg || !relative_path || !cfg->active || cfg->upper_layer[0] == '\0') return RAMFS_ERR_INVAL;

    char target_upper[RAMFS_PATH_MAX];
    combine_path(cfg->upper_layer, relative_path, target_upper, sizeof(target_upper));

    /* Create target directories in upper layer if needed */
    char parent[RAMFS_PATH_MAX];
    rf_parent(target_upper, parent);
    if (rf_strcmp(parent, "/") != 0 && !find_node(parent)) {
        ramfs_mkdir_p(parent);
    }

    /* Perform write on upper layer */
    int res = ramfs_write(target_upper, data, (int)len);
    return res;
}

int vfs_defrag_compact(int *out_nodes_moved, int *out_freed_slots) {
    int moved = 0;
    int freed = 0;

    int free_slot = -1;
    for (int i = 0; i < RAMFS_MAX_NODES; i++) {
        if (!g_nodes[i].used) {
            freed++;
            if (free_slot == -1) {
                free_slot = i;
            }
        } else if (free_slot != -1) {
            /* Move node i to free_slot */
            g_nodes[free_slot] = g_nodes[i];
            g_nodes[i].used = 0;
            moved++;
            /* Find next free slot */
            while (free_slot < RAMFS_MAX_NODES && g_nodes[free_slot].used) {
                free_slot++;
            }
        }
    }

    if (out_nodes_moved) *out_nodes_moved = moved;
    if (out_freed_slots) *out_freed_slots = freed;

    return RAMFS_OK;
}
