/* osui_files.c - see osui_files.h. Freestanding: no libc. */
#include "osui_files.h"
#include "os_syscalls.h"

#define K_UP 30
#define K_DOWN 31
#define K_LEFT 28
#define K_RIGHT 29
#define K_HOME 0x12
#define K_END 0x13
#define K_DEL 0x14
#define K_ESC 27

const char osui_files_toolbar[] =
    "[Haut] [Ouvrir] [+Fich] [+Doss] [Renom] [Suppr] [Copier] [Depl] [Infos]";

static int s_len(const char *s) { int n = 0; while (s && s[n]) n++; return n; }
static void s_cpy(char *d, int max, const char *s) {
    int i = 0;
    if (!d || max <= 0) return;
    while (s && s[i] && i < max - 1) { d[i] = s[i]; i++; }
    d[i] = 0;
}
static void s_cat(char *d, int max, const char *s) { int n = s_len(d); if (n < max) s_cpy(d + n, max - n, s); }
static void s_cat_u(char *d, int max, unsigned v) {
    char t[12], r[12]; int n = 0, i;
    do { t[n++] = (char)('0' + v % 10U); v /= 10U; } while (v && n < 11);
    for (i = 0; i < n; i++) r[i] = t[n - 1 - i];
    r[n] = 0;
    s_cat(d, max, r);
}

static void set_status(osui_files_t *f, const char *a, const char *b) {
    s_cpy(f->status, (int)sizeof(f->status), a);
    if (b) s_cat(f->status, (int)sizeof(f->status), b);
}

static void log_op(osui_files_t *f, const char *op, int rc, const char *path) {
    s_cpy(f->log, (int)sizeof(f->log), "osui files ");
    s_cat(f->log, (int)sizeof(f->log), op);
    if (rc == 0) s_cat(f->log, (int)sizeof(f->log), " ok ");
    else {
        s_cat(f->log, (int)sizeof(f->log), " error=");
        s_cat(f->log, (int)sizeof(f->log), fsops_strerror(rc));
        s_cat(f->log, (int)sizeof(f->log), " ");
    }
    s_cat(f->log, (int)sizeof(f->log), path);
}


/* Lot 45: grid rows of the window (default OSUI_APP_ROWS until resized) */
static int files_rows(const osui_files_t *f) {
    int r = f->vrows > 0 ? f->vrows : OSUI_APP_ROWS;
    if (r > OSUI_APP_MAX_ROWS) r = OSUI_APP_MAX_ROWS;
    if (r < 4) r = 4;
    return r;
}
static int files_view(const osui_files_t *f) { return files_rows(f) - 2; }   /* breadcrumb + header */

void osui_files_set_view(osui_files_t *f, int rows, int cols) {
    if (!f) return;
    f->vrows = rows < 0 ? 0 : rows;
    f->vcols = cols < 0 ? 0 : cols;
    if (f->sel >= f->top + files_view(f)) f->top = f->sel - files_view(f) + 1;
    if (f->top > 0 && f->top + files_view(f) > f->n) f->top = f->n > files_view(f) ? f->n - files_view(f) : 0;
}

static void follow(osui_files_t *f) {
    if (f->sel < 0) f->sel = 0;
    if (f->sel >= f->n) f->sel = f->n > 0 ? f->n - 1 : 0;
    if (f->sel < f->top) f->top = f->sel;
    if (f->sel >= f->top + files_view(f)) f->top = f->sel - files_view(f) + 1;
    if (f->top < 0) f->top = 0;
}

int osui_files_refresh(osui_files_t *f) {
    int n = fsops_list(f->io, f->cwd, f->ent, OSUI_FILES_MAX);
    if (n < 0) { f->n = 0; set_status(f, "lecture impossible: ", fsops_strerror(n)); return n; }
    f->n = n;
    follow(f);
    return n;
}

int osui_files_cd(osui_files_t *f, const char *dir) {
    char old[FSOPS_PATH_MAX];
    int rc;
    s_cpy(old, (int)sizeof(old), f->cwd);
    fsops_resolve(f->cwd, dir, f->cwd, (int)sizeof(f->cwd));
    f->sel = f->top = 0;
    rc = osui_files_refresh(f);
    if (rc < 0) {
        s_cpy(f->cwd, (int)sizeof(f->cwd), old);
        (void)osui_files_refresh(f);
        set_status(f, "dossier inaccessible: ", dir);
        return rc;
    }
    set_status(f, f->n ? "Entree ouvre, Retour arriere remonte, n r Suppr c m i" : "dossier vide", 0);
    s_cpy(f->log, (int)sizeof(f->log), "osui files cd ");
    s_cat(f->log, (int)sizeof(f->log), f->cwd);
    return 0;
}

void osui_files_open(osui_files_t *f, const fsops_io_t *io, const char *dir) {
    int i;
    char *p = (char *)f;
    for (i = 0; i < (int)sizeof(*f); i++) p[i] = 0;
    f->active = 1;
    f->io = io;
    s_cpy(f->cwd, (int)sizeof(f->cwd), "/");
    (void)osui_files_cd(f, dir && dir[0] ? dir : "/");
}

int osui_files_selected(const osui_files_t *f, char *out, int max) {
    if (!f || f->n <= 0 || f->sel < 0 || f->sel >= f->n) return 0;
    fsops_join(out, max, f->cwd, f->ent[f->sel].name);
    return 1;
}

static void select_name(osui_files_t *f, const char *name) {
    int i;
    for (i = 0; i < f->n; i++) {
        const char *a = f->ent[i].name, *b = name;
        while (*a && *a == *b) { a++; b++; }
        if (!*a && !*b) { f->sel = i; break; }
    }
    follow(f);
}

static void start_prompt(osui_files_t *f, int op, const char *initial) {
    f->mode = OSUI_FILES_PROMPT;
    f->op = op;
    s_cpy(f->input, (int)sizeof(f->input), initial ? initial : "");
    f->ilen = s_len(f->input);
}

static int activate(osui_files_t *f) {
    char path[FSOPS_PATH_MAX];
    if (!osui_files_selected(f, path, (int)sizeof(path))) return OSUI_FILES_NONE;
    if (f->ent[f->sel].flags == OS_DIRENT_DIR) { (void)osui_files_cd(f, path); return OSUI_FILES_NONE; }
    s_cpy(f->open_path, (int)sizeof(f->open_path), path);
    s_cpy(f->log, (int)sizeof(f->log), "osui files open ");
    s_cat(f->log, (int)sizeof(f->log), path);
    return OSUI_FILES_OPEN;
}

static void info(osui_files_t *f) {
    char path[FSOPS_PATH_MAX];
    os_dirent_t st;
    if (!osui_files_selected(f, path, (int)sizeof(path))) { set_status(f, "rien de selectionne", 0); return; }
    if (!f->io->stat || f->io->stat(path, &st) != 0) { set_status(f, "stat impossible: ", path); return; }
    s_cpy(f->status, (int)sizeof(f->status), path);
    s_cat(f->status, (int)sizeof(f->status), st.flags == OS_DIRENT_DIR ? "  dossier" : "  fichier ");
    if (st.flags != OS_DIRENT_DIR) { s_cat_u(f->status, (int)sizeof(f->status), st.size); s_cat(f->status, (int)sizeof(f->status), " octets"); }
    s_cpy(f->log, (int)sizeof(f->log), "osui files info ");
    s_cat(f->log, (int)sizeof(f->log), f->status);
}

static void finish_prompt(osui_files_t *f) {
    char src[FSOPS_PATH_MAX], dst[FSOPS_PATH_MAX], fin[FSOPS_PATH_MAX];
    int rc = 0, has = osui_files_selected(f, src, (int)sizeof(src));
    const char *opname = "?";
    f->mode = OSUI_FILES_BROWSE;
    if (!f->input[0]) { set_status(f, "annule", 0); return; }
    fsops_resolve(f->cwd, f->input, dst, (int)sizeof(dst));
    fin[0] = 0;
    switch (f->op) {
    case OSUI_FOP_NEWFILE: opname = "touch"; rc = fsops_touch(f->io, dst); if (rc == 0) rc = FSOPS_ERR_EXISTS; if (rc == 1) rc = 0; s_cpy(fin, (int)sizeof(fin), dst); break;
    case OSUI_FOP_NEWDIR: opname = "mkdir"; rc = fsops_mkdir(f->io, dst); s_cpy(fin, (int)sizeof(fin), dst); break;
    case OSUI_FOP_RENAME: opname = "rename"; rc = has ? fsops_mv(f->io, src, dst, fin, (int)sizeof(fin)) : FSOPS_ERR_FAIL; break;
    case OSUI_FOP_COPY: opname = "copy"; rc = has ? fsops_cp(f->io, src, dst, fin, (int)sizeof(fin)) : FSOPS_ERR_FAIL; break;
    case OSUI_FOP_MOVE: opname = "move"; rc = has ? fsops_mv(f->io, src, dst, fin, (int)sizeof(fin)) : FSOPS_ERR_FAIL; break;
    default: return;
    }
    log_op(f, opname, rc, fin[0] ? fin : dst);
    if (rc != 0) { set_status(f, "erreur: ", fsops_strerror(rc)); (void)osui_files_refresh(f); return; }
    (void)osui_files_refresh(f);
    {
        char par[FSOPS_PATH_MAX];
        fsops_parent(fin, par, (int)sizeof(par));
        {
            const char *a = par, *b = f->cwd;
            while (*a && *a == *b) { a++; b++; }
            if (!*a && !*b) select_name(f, fsops_basename(fin));
        }
    }
    set_status(f, "ok: ", fin);
}

static void do_delete(osui_files_t *f) {
    char path[FSOPS_PATH_MAX];
    int rc, dir;
    f->mode = OSUI_FILES_BROWSE;
    if (!osui_files_selected(f, path, (int)sizeof(path))) return;
    dir = f->ent[f->sel].flags == OS_DIRENT_DIR;
    rc = dir ? fsops_rmdir(f->io, path) : fsops_rm(f->io, path);
    log_op(f, dir ? "rmdir" : "rm", rc, path);
    if (rc != 0) set_status(f, "erreur: ", fsops_strerror(rc));
    else set_status(f, "supprime: ", path);
    (void)osui_files_refresh(f);
}

int osui_files_tool(osui_files_t *f, int b) {
    char path[FSOPS_PATH_MAX];
    if (!f || !f->active) return OSUI_FILES_NONE;
    f->mode = OSUI_FILES_BROWSE;
    switch (b) {
    case OSUI_FT_PARENT: { char par[FSOPS_PATH_MAX]; fsops_parent(f->cwd, par, (int)sizeof(par)); (void)osui_files_cd(f, par); return OSUI_FILES_NONE; }
    case OSUI_FT_OPEN: return activate(f);
    case OSUI_FT_NEWFILE: start_prompt(f, OSUI_FOP_NEWFILE, ""); return OSUI_FILES_NONE;
    case OSUI_FT_NEWDIR: start_prompt(f, OSUI_FOP_NEWDIR, ""); return OSUI_FILES_NONE;
    case OSUI_FT_RENAME:
        if (f->n) start_prompt(f, OSUI_FOP_RENAME, f->ent[f->sel].name);
        return OSUI_FILES_NONE;
    case OSUI_FT_DELETE:
        if (osui_files_selected(f, path, (int)sizeof(path))) { f->mode = OSUI_FILES_CONFIRM; f->op = OSUI_FOP_DELETE; }
        return OSUI_FILES_NONE;
    case OSUI_FT_COPY:
    case OSUI_FT_MOVE:
        if (f->n) { char d[FSOPS_PATH_MAX]; fsops_join(d, (int)sizeof(d), f->cwd, ""); start_prompt(f, b == OSUI_FT_COPY ? OSUI_FOP_COPY : OSUI_FOP_MOVE, d); }
        return OSUI_FILES_NONE;
    case OSUI_FT_INFO: info(f); return OSUI_FILES_NONE;
    default: return OSUI_FILES_NONE;
    }
}

int osui_files_key(osui_files_t *f, int key) {
    if (!f || !f->active) return OSUI_FILES_NONE;
    if (f->mode == OSUI_FILES_PROMPT) {
        if (key == K_ESC) { f->mode = OSUI_FILES_BROWSE; set_status(f, "annule", 0); }
        else if (key == '\n' || key == '\r') finish_prompt(f);
        else if (key == '\b' || key == 127) { if (f->ilen > 0) f->input[--f->ilen] = 0; }
        else if (key >= 32 && key <= 126 && f->ilen < (int)sizeof(f->input) - 1) { f->input[f->ilen++] = (char)key; f->input[f->ilen] = 0; }
        return OSUI_FILES_NONE;
    }
    if (f->mode == OSUI_FILES_CONFIRM) {
        if (key == 'o' || key == 'O' || key == 'y' || key == 'Y') do_delete(f);
        else { f->mode = OSUI_FILES_BROWSE; set_status(f, "suppression annulee", 0); }
        return OSUI_FILES_NONE;
    }
    switch (key) {
    case K_UP: f->sel--; break;
    case K_DOWN: f->sel++; break;
    case K_HOME: f->sel = 0; break;
    case K_END: f->sel = f->n - 1; break;
    case '\n': case '\r': case K_RIGHT: return activate(f);
    case '\b': case 127: case K_LEFT: return osui_files_tool(f, OSUI_FT_PARENT);
    case 'n': return osui_files_tool(f, OSUI_FT_NEWFILE);
    case 'd': return osui_files_tool(f, OSUI_FT_NEWDIR);
    case 'r': return osui_files_tool(f, OSUI_FT_RENAME);
    case K_DEL: case 'x': return osui_files_tool(f, OSUI_FT_DELETE);
    case 'c': return osui_files_tool(f, OSUI_FT_COPY);
    case 'm': return osui_files_tool(f, OSUI_FT_MOVE);
    case 'i': return osui_files_tool(f, OSUI_FT_INFO);
    case 'u': (void)osui_files_refresh(f); set_status(f, "actualise", 0); break;
    case K_ESC: f->active = 0; return OSUI_FILES_CLOSE;
    default: return OSUI_FILES_NONE;
    }
    follow(f);
    return OSUI_FILES_NONE;
}

/* Breadcrumb "/ > home > docs": segment k of the row text at column col. */
static int crumb_path(const osui_files_t *f, int col, char *out, int max) {
    int i = 0, c = 0, seg_start;
    char acc[FSOPS_PATH_MAX];
    if (col >= 0 && col < 2) { s_cpy(out, max, "/"); return 1; }
    c = 2;
    acc[0] = 0;
    while (f->cwd[i]) {
        int k = 0;
        char name[OS_NAME_MAX];
        while (f->cwd[i] == '/') i++;
        if (!f->cwd[i]) break;
        while (f->cwd[i] && f->cwd[i] != '/' && k < OS_NAME_MAX - 1) name[k++] = f->cwd[i++];
        name[k] = 0;
        s_cat(acc, (int)sizeof(acc), "/");
        s_cat(acc, (int)sizeof(acc), name);
        c += 2;                      /* "> " */
        seg_start = c;
        c += k + 1;
        if (col >= seg_start - 2 && col < c) { s_cpy(out, max, acc); return 1; }
    }
    return 0;
}

int osui_files_click(osui_files_t *f, int row, int col, int dbl) {
    if (!f || !f->active) return OSUI_FILES_NONE;
    if (row == 0) {
        char p[FSOPS_PATH_MAX];
        if (crumb_path(f, col, p, (int)sizeof(p))) (void)osui_files_cd(f, p);
        return OSUI_FILES_NONE;
    }
    if (row >= 2) {
        int idx = f->top + row - 2;
        if (idx >= f->n) return OSUI_FILES_NONE;
        f->mode = OSUI_FILES_BROWSE;
        f->sel = idx;
        follow(f);
        if (dbl) return activate(f);
    }
    return OSUI_FILES_NONE;
}

/* Lot 46: a row is as wide as the window grid (77 columns by default) */
static int g_row_cols = OSUI_APP_COLS;
static void put_row(osui_app_view_t *v, int r, const char *s, unsigned char a) {
    int k;
    s_cpy(v->rows[r], g_row_cols, s);
    for (k = 0; k < g_row_cols; k++) v->attr[r][k] = a;
}

/* Lot 46: grid columns of the window and the width of the name column */
static int files_cols(const osui_files_t *f) {
    int c = f->vcols > 0 ? f->vcols : OSUI_APP_COLS - 1;
    if (c > OSUI_APP_MAX_COLS - 1) c = OSUI_APP_MAX_COLS - 1;
    if (c < 40) c = 40;
    return c;
}
static int files_namecol(const osui_files_t *f) {
    int n = 46 + files_cols(f) - (OSUI_APP_COLS - 1);
    return n < 16 ? 16 : n;
}

void osui_files_render(const osui_files_t *f, osui_app_view_t *v) {
    char line[OSUI_APP_MAX_COLS + OS_NAME_MAX + 16];
    int r, i, namecol;
    osui_app_view_clear(v);
    if (!f || !f->active) return;
    g_row_cols = files_cols(f) + 1;
    s_cpy(v->title, (int)sizeof(v->title), "Fichiers - ");
    s_cat(v->title, (int)sizeof(v->title), f->cwd);
    s_cpy(v->tool, OSUI_APP_COLS, osui_files_toolbar);
    /* breadcrumb */
    {
        int i2 = 0;
        s_cpy(line, (int)sizeof(line), "/ ");
        while (f->cwd[i2]) {
            int k = 0; char name[OS_NAME_MAX];
            while (f->cwd[i2] == '/') i2++;
            if (!f->cwd[i2]) break;
            while (f->cwd[i2] && f->cwd[i2] != '/' && k < OS_NAME_MAX - 1) name[k++] = f->cwd[i2++];
            name[k] = 0;
            s_cat(line, (int)sizeof(line), "> ");
            s_cat(line, (int)sizeof(line), name);
            s_cat(line, (int)sizeof(line), " ");
        }
        put_row(v, 0, line, OSUI_APP_A_LINK);
    }
    /* Lot 46: the name column takes the width the window gives (46 at the
     * default 77-column grid, as before); a name wider than it is cut with
     * '~' (a wider window shows it whole) */
    namecol = files_namecol(f);
    s_cpy(line, (int)sizeof(line), "  Nom");
    {
        int k;
        for (k = s_len(line); k < namecol; k++) line[k] = ' ';
        line[namecol] = 0;
    }
    s_cat(line, (int)sizeof(line), "Taille   Type");
    put_row(v, 1, line, OSUI_APP_A_DIM);
    for (r = 2, i = f->top; r < files_rows(f) && i < f->n; r++, i++) {
        const os_dirent_t *e = &f->ent[i];
        int dir = e->flags == OS_DIRENT_DIR, k;
        char num[16];
        s_cpy(line, (int)sizeof(line), dir ? "+ " : "  ");
        s_cat(line, (int)sizeof(line), e->name);
        if (dir) s_cat(line, (int)sizeof(line), "/");
        if (s_len(line) > namecol - 1) { line[namecol - 2] = '~'; line[namecol - 1] = 0; }
        for (k = s_len(line); k < namecol; k++) line[k] = ' ';
        line[namecol] = 0;
        num[0] = 0;
        if (!dir) s_cat_u(num, (int)sizeof(num), e->size);
        for (k = s_len(num); k < 8; k++) s_cat(line, (int)sizeof(line), " ");
        s_cat(line, (int)sizeof(line), num);
        s_cat(line, (int)sizeof(line), dir ? "   dossier" : "   fichier");
        put_row(v, r, line, i == f->sel ? OSUI_APP_A_SEL : (dir ? OSUI_APP_A_DIR : OSUI_APP_A_TEXT));
    }
    if (f->n == 0) put_row(v, 2, "  (dossier vide)", OSUI_APP_A_DIM);
    v->nrows = r > 3 ? r : 3;
    if (f->mode == OSUI_FILES_PROMPT) {
        const char *q = f->op == OSUI_FOP_NEWFILE ? "Nouveau fichier: " : f->op == OSUI_FOP_NEWDIR ? "Nouveau dossier: "
                      : f->op == OSUI_FOP_RENAME ? "Renommer en: " : f->op == OSUI_FOP_COPY ? "Copier vers: " : "Deplacer vers: ";
        s_cpy(v->status, OSUI_APP_COLS, q);
        s_cat(v->status, OSUI_APP_COLS, f->input);
        s_cat(v->status, OSUI_APP_COLS, "_  (Entree valide, Echap annule)");
    } else if (f->mode == OSUI_FILES_CONFIRM) {
        s_cpy(v->status, OSUI_APP_COLS, "Supprimer ");
        s_cat(v->status, OSUI_APP_COLS, f->n ? f->ent[f->sel].name : "");
        s_cat(v->status, OSUI_APP_COLS, " ? o = oui, autre touche = non");
    } else s_cpy(v->status, OSUI_APP_COLS, f->status);
}
