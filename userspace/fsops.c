/* fsops.c - see fsops.h. Freestanding: no libc. */
#include "fsops.h"

static int f_len(const char *s) { int n = 0; while (s && s[n]) n++; return n; }
static void f_cpy(char *d, int max, const char *s) {
    int i = 0;
    if (!d || max <= 0) return;
    while (s && s[i] && i < max - 1) { d[i] = s[i]; i++; }
    d[i] = 0;
}
static int f_cmp(const char *a, const char *b) {
    while (*a && *a == *b) { a++; b++; }
    return (unsigned char)*a - (unsigned char)*b;
}

void fsops_join(char *out, int max, const char *dir, const char *name) {
    int i = 0, j = 0;
    if (!out || max <= 0) return;
    if (!dir || !dir[0]) dir = "/";
    if (!name) name = "";
    while (dir[i] && i < max - 2) { out[i] = dir[i]; i++; }
    if (i > 0 && out[i - 1] != '/') out[i++] = '/';
    while (name[j] && i < max - 1) out[i++] = name[j++];
    out[i] = 0;
}

const char *fsops_basename(const char *path) {
    const char *b = path ? path : "";
    int i;
    for (i = 0; path && path[i]; i++)
        if (path[i] == '/' && path[i + 1]) b = path + i + 1;
    return (b && b[0] && !(b[0] == '/' && !b[1])) ? b : "/";
}

void fsops_parent(const char *path, char *out, int max) {
    int n, cut = 0, i;
    f_cpy(out, max, path && path[0] ? path : "/");
    n = f_len(out);
    while (n > 1 && out[n - 1] == '/') out[--n] = 0;
    for (i = 0; i < n; i++) if (out[i] == '/') cut = i;
    if (cut == 0) { f_cpy(out, max, "/"); return; }
    out[cut] = 0;
}

void fsops_resolve(const char *cwd, const char *arg, char *out, int max) {
    char tmp[FSOPS_PATH_MAX * 2];
    int i = 0, o = 0;
    if (!arg || !arg[0]) arg = ".";
    if (arg[0] == '/') f_cpy(tmp, (int)sizeof(tmp), arg);
    else fsops_join(tmp, (int)sizeof(tmp), cwd && cwd[0] ? cwd : "/", arg);
    /* normalise: collapse //, ., .. */
    out[o++] = '/';
    while (tmp[i]) {
        int s, l;
        while (tmp[i] == '/') i++;
        s = i;
        while (tmp[i] && tmp[i] != '/') i++;
        l = i - s;
        if (l == 0 || (l == 1 && tmp[s] == '.')) continue;
        if (l == 2 && tmp[s] == '.' && tmp[s + 1] == '.') {
            if (o > 1) { o--; while (o > 1 && out[o - 1] != '/') o--; if (o > 1) o--; }
            continue;
        }
        if (o > 1 && o < max - 1) out[o++] = '/';
        while (l-- > 0 && o < max - 1) out[o++] = tmp[s++];
    }
    out[o] = 0;
}

int fsops_valid_name(const char *name) {
    int i;
    if (!name || !name[0] || f_len(name) >= OS_NAME_MAX) return 0;
    if (!f_cmp(name, ".") || !f_cmp(name, "..")) return 0;
    for (i = 0; name[i]; i++) if (name[i] == '/' || (unsigned char)name[i] < 32) return 0;
    return 1;
}

static int ent_before(const os_dirent_t *a, const os_dirent_t *b) {
    int ad = a->flags == OS_DIRENT_DIR, bd = b->flags == OS_DIRENT_DIR;
    if (ad != bd) return ad;
    return f_cmp(a->name, b->name) < 0;
}

int fsops_list(const fsops_io_t *io, const char *dir, os_dirent_t *out, int max) {
    os_dirent_t st;
    int n, i, j;
    if (!io || !io->listdir) return FSOPS_ERR_FAIL;
    if (io->stat && io->stat(dir, &st) == 0 && st.flags != OS_DIRENT_DIR) return FSOPS_ERR_NOTDIR;
    n = io->listdir(dir, out, max);
    if (n < 0) return n;
    for (i = 1; i < n; i++) {           /* insertion sort, n is small */
        os_dirent_t k = out[i];
        for (j = i - 1; j >= 0 && ent_before(&k, &out[j]); j--) out[j + 1] = out[j];
        out[j + 1] = k;
    }
    return n;
}

int fsops_read(const fsops_io_t *io, const char *path, char *buf, int max) {
    os_dirent_t st;
    if (!io || !io->readfile) return FSOPS_ERR_FAIL;
    if (io->stat && io->stat(path, &st) == 0) {
        if (st.flags == OS_DIRENT_DIR) return FSOPS_ERR_ISDIR;
        if ((int)st.size > max) return FSOPS_ERR_TOOBIG;
    }
    return io->readfile(path, buf, max);
}

int fsops_write(const fsops_io_t *io, const char *path, const char *buf, int n) {
    os_dirent_t st;
    int rc;
    if (!io || !io->writefile) return FSOPS_ERR_FAIL;
    if (io->stat && io->stat(path, &st) == 0 && st.flags == OS_DIRENT_DIR) return FSOPS_ERR_ISDIR;
    rc = io->writefile(path, buf, n);
    /* UX5: the overlay store keeps a prefix of a too large file and returns
     * its length: report it, the caller must not think the file is saved */
    if (rc > 0 && rc < n) return FSOPS_ERR_TOOBIG;
    return rc < 0 ? rc : 0;
}

int fsops_mkdir(const fsops_io_t *io, const char *path) {
    if (!io || !io->mkdir) return FSOPS_ERR_FAIL;
    if (!fsops_valid_name(fsops_basename(path))) return FSOPS_ERR_NAME;
    return io->mkdir(path);
}

int fsops_touch(const fsops_io_t *io, const char *path) {
    os_dirent_t st;
    int rc;
    if (!io || !io->writefile) return FSOPS_ERR_FAIL;
    if (io->stat && io->stat(path, &st) == 0) return st.flags == OS_DIRENT_DIR ? FSOPS_ERR_ISDIR : 0;
    if (!fsops_valid_name(fsops_basename(path))) return FSOPS_ERR_NAME;
    rc = io->writefile(path, "", 0);
    return rc < 0 ? rc : 1;
}

int fsops_rm(const fsops_io_t *io, const char *path) {
    os_dirent_t st;
    if (!io || !io->unlink || !io->stat) return FSOPS_ERR_FAIL;
    if (io->stat(path, &st) != 0) return FSOPS_ERR_FAIL;
    if (st.flags == OS_DIRENT_DIR) return FSOPS_ERR_ISDIR;
    return io->unlink(path);
}

int fsops_rmdir(const fsops_io_t *io, const char *path) {
    os_dirent_t st;
    if (!io || !io->unlink || !io->stat) return FSOPS_ERR_FAIL;
    if (io->stat(path, &st) != 0) return FSOPS_ERR_FAIL;
    if (st.flags != OS_DIRENT_DIR) return FSOPS_ERR_NOTDIR;
    return io->unlink(path);
}

static void dest_into_dir(const fsops_io_t *io, const char *src, const char *dst, char *out, int max) {
    os_dirent_t st;
    if (io->stat && io->stat(dst, &st) == 0 && st.flags == OS_DIRENT_DIR)
        fsops_join(out, max, dst, fsops_basename(src));
    else f_cpy(out, max, dst);
}

static char g_copy_buf[FSOPS_FILE_MAX];

int fsops_cp(const fsops_io_t *io, const char *src, const char *dst, char *final_dst, int max) {
    char d[FSOPS_PATH_MAX];
    os_dirent_t st;
    int n, rc;
    if (!io || !io->stat) return FSOPS_ERR_FAIL;
    if (io->stat(src, &st) != 0) return FSOPS_ERR_FAIL;
    dest_into_dir(io, src, dst, d, (int)sizeof(d));
    if (!f_cmp(d, src)) return FSOPS_ERR_EXISTS;
    if (st.flags == OS_DIRENT_DIR) {
        if (!io->copy) return FSOPS_ERR_FAIL;
        rc = io->copy(src, d);
    } else {
        os_dirent_t dt;
        if ((int)st.size > FSOPS_FILE_MAX) return FSOPS_ERR_TOOBIG;
        if (io->stat(d, &dt) == 0 && dt.flags == OS_DIRENT_DIR) return FSOPS_ERR_ISDIR;
        n = io->readfile(src, g_copy_buf, FSOPS_FILE_MAX);
        if (n < 0) return n;
        rc = io->writefile(d, g_copy_buf, n);
        if (rc > 0) rc = 0;
    }
    if (rc == 0 && final_dst) f_cpy(final_dst, max, d);
    return rc;
}

int fsops_mv(const fsops_io_t *io, const char *src, const char *dst, char *final_dst, int max) {
    char d[FSOPS_PATH_MAX];
    os_dirent_t st;
    int rc;
    if (!io || !io->stat || !io->rename) return FSOPS_ERR_FAIL;
    if (io->stat(src, &st) != 0) return FSOPS_ERR_FAIL;
    dest_into_dir(io, src, dst, d, (int)sizeof(d));
    if (!fsops_valid_name(fsops_basename(d))) return FSOPS_ERR_NAME;
    rc = io->rename(src, d);
    if (rc == 0 && final_dst) f_cpy(final_dst, max, d);
    return rc;
}

const char *fsops_strerror(int rc) {
    switch (rc) {
    case FSOPS_ERR_EXISTS: return "existe deja";
    case FSOPS_ERR_PARENT: return "parent invalide";
    case FSOPS_ERR_ISDIR: return "est un repertoire";
    case FSOPS_ERR_NOTEMPTY: return "repertoire non vide";
    case FSOPS_ERR_NOSPACE: return "plus de place";
    case FSOPS_ERR_RDONLY: return "protege (initrd)";
    case FSOPS_ERR_NOTDIR: return "n'est pas un repertoire";
    case FSOPS_ERR_TOOBIG: return "fichier trop grand";
    case FSOPS_ERR_NAME: return "nom invalide";
    case OS_VFS_BACKEND_WORKER_REQUIRED: return "stockage indisponible";
    default: return "echec";
    }
}
