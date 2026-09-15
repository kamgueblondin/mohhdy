/* ramfs.c - VFS RAM (table de nœuds en mémoire). Sans libc. */

#include "ramfs.h"

typedef struct {
    int used;
    int is_dir;
    char path[RAMFS_PATH_MAX];
    char content[RAMFS_CONTENT_MAX];
    int size;
} ramfs_node_t;

static ramfs_node_t g_nodes[RAMFS_MAX_NODES];
static int g_count;

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

int ramfs_mkdir(const char *path) {
    if (!path || path[0] == 0 || rf_strcmp(path, "/") == 0) return RAMFS_ERR_INVAL;
    if (find_node(path)) return RAMFS_ERR_EXISTS;
    if (!parent_is_dir(path)) return RAMFS_ERR_NOTDIR;
    return add_dir(path);
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

int ramfs_rm(const char *path) {
    ramfs_node_t *n;
    if (!path || rf_strcmp(path, "/") == 0) return RAMFS_ERR_INVAL;
    n = find_node(path);
    if (!n) return RAMFS_ERR_NOTFOUND;
    if (n->is_dir) return RAMFS_ERR_ISDIR;
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
        if (data) rf_memcpy(n->content, data, len);
        n->content[len] = 0;
        n->size = len;
        return RAMFS_OK;
    }
    if (!parent_is_dir(path)) return RAMFS_ERR_NOTDIR;
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
    if (s->is_dir) return RAMFS_ERR_ISDIR;
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

int ramfs_node_count(void) {
    return g_count;
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
