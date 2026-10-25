/* fsops.h - file operations shared by the shell commands (ls, cat, mkdir,
 * touch, rm, rmdir, cp, mv, stat) and the desktop apps (file explorer,
 * text editor). One implementation over the kernel VFS syscalls; the
 * syscalls are injected so the logic is host testable.
 *
 * Errors are the kernel VFS codes (negative): see fsops_strerror. */
#ifndef FSOPS_H
#define FSOPS_H
#include "os_syscalls.h"

#define FSOPS_PATH_MAX 128
#define FSOPS_FILE_MAX 8192      /* largest file read/copied/edited at once */

#define FSOPS_ERR_FAIL     (-1)  /* not found / generic failure */
#define FSOPS_ERR_EXISTS   (-2)
#define FSOPS_ERR_PARENT   (-3)
#define FSOPS_ERR_ISDIR    (-4)
#define FSOPS_ERR_NOTEMPTY (-5)
#define FSOPS_ERR_NOSPACE  (-6)
#define FSOPS_ERR_RDONLY   (-8)  /* initrd: read only */
#define FSOPS_ERR_NOTDIR   (-20)
#define FSOPS_ERR_TOOBIG   (-21)
#define FSOPS_ERR_NAME     (-22) /* empty name, '/' in a name, too long */

typedef struct {
    int (*stat)(const char *path, os_dirent_t *out);
    int (*listdir)(const char *path, os_dirent_t *out, int max_n);
    int (*readfile)(const char *path, char *buf, int max);
    int (*writefile)(const char *path, const char *buf, int n);
    int (*mkdir)(const char *path);
    int (*unlink)(const char *path);
    int (*rename)(const char *from, const char *to);
    int (*copy)(const char *from, const char *to);   /* directories */
} fsops_io_t;

/* Paths. */
void fsops_join(char *out, int max, const char *dir, const char *name);
const char *fsops_basename(const char *path);
void fsops_parent(const char *path, char *out, int max);
/* Resolve arg against cwd ("..", ".", absolute). */
void fsops_resolve(const char *cwd, const char *arg, char *out, int max);
int fsops_valid_name(const char *name);

/* Listing: directories first, then files, each sorted by name. */
int fsops_list(const fsops_io_t *io, const char *dir, os_dirent_t *out, int max);

int fsops_read(const fsops_io_t *io, const char *path, char *buf, int max);
int fsops_write(const fsops_io_t *io, const char *path, const char *buf, int n);
int fsops_mkdir(const fsops_io_t *io, const char *path);
int fsops_touch(const fsops_io_t *io, const char *path);   /* 1 created, 0 existed */
int fsops_rm(const fsops_io_t *io, const char *path);      /* files */
int fsops_rmdir(const fsops_io_t *io, const char *path);   /* empty dirs */
/* cp/mv: a destination directory receives the source basename; the final
 * destination path is written to final_dst when not null. */
int fsops_cp(const fsops_io_t *io, const char *src, const char *dst, char *final_dst, int max);
int fsops_mv(const fsops_io_t *io, const char *src, const char *dst, char *final_dst, int max);

/* UX5 recursive search (shell "grep -r", GUI /search panel): every line
 * of the files under dir (depth <= FSOPS_GREP_DEPTH, FSOPS_GREP_DIR
 * entries per directory) that contains pat goes to hit(). Only the first
 * FSOPS_FILE_MAX bytes of a file are searched. Stops after max_hits.
 * Returns the hit count (< 0 when dir cannot be listed); *files and
 * *truncated (may be null) count the searched and the cut files. */
#define FSOPS_GREP_DEPTH 6
#define FSOPS_GREP_DIR 32
typedef void (*fsops_grep_hit_fn)(void *ctx, const char *path, int line, const char *text, int len);
int fsops_grep(const fsops_io_t *io, const char *dir, const char *pat, int max_hits,
               fsops_grep_hit_fn hit, void *ctx, int *files, int *truncated);

/* "existe deja", "protege (initrd)" ... */
const char *fsops_strerror(int rc);
#endif
