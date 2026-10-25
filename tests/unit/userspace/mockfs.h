/* In-memory VFS for host tests of fsops and the desktop apps. Paths under
 * /ro behave like the read-only initrd (-8 on every mutation). */
#ifndef MOCKFS_H
#define MOCKFS_H
#include <string.h>
#include "fsops.h"

#define MFS_N 64
typedef struct { int used, dir; char path[128]; char data[40000]; int len; } mfs_ent_t;
static mfs_ent_t mfs[MFS_N];

static int mfs_find(const char *p) {
    int i;
    for (i = 0; i < MFS_N; i++) if (mfs[i].used && !strcmp(mfs[i].path, p)) return i;
    return -1;
}
static int mfs_ro(const char *p) { return !strncmp(p, "/ro", 3) && (p[3] == 0 || p[3] == '/'); }
static void mfs_parent(const char *p, char *o) { fsops_parent(p, o, 128); }
static int mfs_add(const char *p, int dir, const char *d, int n) {
    int i;
    for (i = 0; i < MFS_N; i++) if (!mfs[i].used) {
        mfs[i].used = 1; mfs[i].dir = dir; strcpy(mfs[i].path, p);
        if (d && n > 0) memcpy(mfs[i].data, d, (size_t)n);
        mfs[i].len = n > 0 ? n : 0;
        return 0;
    }
    return -6;
}
static void mfs_reset(void) {
    memset(mfs, 0, sizeof(mfs));
    mfs_add("/", 1, 0, 0);
    mfs_add("/ro", 1, 0, 0);
    mfs_add("/ro/readme.txt", 0, "lecture seule\n", 14);
    mfs_add("/home", 1, 0, 0);
    mfs_add("/home/notes.txt", 0, "ligne 1\nligne 2\n", 16);
}
static int m_stat(const char *p, os_dirent_t *o) {
    int i = mfs_find(p);
    if (i < 0) return -1;
    memset(o, 0, sizeof(*o));
    strcpy(o->name, fsops_basename(p));
    o->flags = mfs[i].dir ? OS_DIRENT_DIR : OS_DIRENT_FILE;
    o->size = (uint32_t)mfs[i].len;
    return 0;
}
static int m_list(const char *p, os_dirent_t *o, int max) {
    int i, n = 0;
    char par[128];
    if (mfs_find(p) < 0) return -1;
    for (i = 0; i < MFS_N && n < max; i++) {
        if (!mfs[i].used || !strcmp(mfs[i].path, "/")) continue;
        mfs_parent(mfs[i].path, par);
        if (strcmp(par, p)) continue;
        m_stat(mfs[i].path, &o[n++]);
    }
    return n;
}
static int m_read(const char *p, char *b, int max) {
    int i = mfs_find(p), n;
    if (i < 0 || mfs[i].dir) return -1;
    n = mfs[i].len < max ? mfs[i].len : max;
    memcpy(b, mfs[i].data, (size_t)n);
    return n;
}
static int m_write(const char *p, const char *b, int n) {
    int i = mfs_find(p);
    char par[128];
    if (mfs_ro(p)) return -8;
    mfs_parent(p, par);
    if (mfs_find(par) < 0) return -3;
    if (i < 0) return mfs_add(p, 0, b, n) == 0 ? n : -6;
    if (mfs[i].dir) return -4;
    memcpy(mfs[i].data, b, (size_t)n);
    mfs[i].len = n;
    return n;
}
static int m_mkdir(const char *p) {
    char par[128];
    if (mfs_ro(p)) return -8;
    if (mfs_find(p) >= 0) return -2;
    mfs_parent(p, par);
    if (mfs_find(par) < 0) return -3;
    return mfs_add(p, 1, 0, 0);
}
static int m_unlink(const char *p) {
    int i = mfs_find(p), k;
    size_t l = strlen(p);
    if (mfs_ro(p)) return -8;
    if (i < 0) return -1;
    for (k = 0; k < MFS_N; k++)
        if (mfs[k].used && !strncmp(mfs[k].path, p, l) && mfs[k].path[l] == '/') return -5;
    mfs[i].used = 0;
    return 0;
}
static int m_rename(const char *a, const char *b) {
    int i = mfs_find(a);
    char par[128];
    if (mfs_ro(a) || mfs_ro(b)) return -8;
    if (i < 0) return -1;
    if (mfs_find(b) >= 0) return -2;
    mfs_parent(b, par);
    if (mfs_find(par) < 0) return -3;
    strcpy(mfs[i].path, b);
    return 0;
}
static int m_copy(const char *a, const char *b) {
    int i = mfs_find(a);
    if (mfs_ro(b)) return -8;
    if (i < 0) return -1;
    return mfs_add(b, mfs[i].dir, mfs[i].data, mfs[i].len);
}
static const fsops_io_t mfs_io = { m_stat, m_list, m_read, m_write, m_mkdir, m_unlink, m_rename, m_copy };
#endif
