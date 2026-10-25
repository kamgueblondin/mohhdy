/* osui_form.c - Reusable GUI Form Component implementation (PR 4).
 * Freestanding C, Ring 3 compatible. */
#include "osui_form.h"
#include "osui_tools.h"

#define K_UP 30
#define K_DOWN 31
#define K_LEFT 28
#define K_RIGHT 29
#define K_TAB 9
#define K_ESC 27

static int f_len(const char *s) {
    int n = 0;
    while (s && s[n]) n++;
    return n;
}

static void f_cpy(char *d, int max, const char *s) {
    int i = 0;
    if (!d || max <= 0) return;
    while (s && s[i] && i < max - 1) { d[i] = s[i]; i++; }
    d[i] = 0;
}

static void f_cat(char *d, int max, const char *s) {
    int n = f_len(d);
    if (n < max) f_cpy(d + n, max - n, s);
}

void osui_form_init(osui_form_t *f, const char *title) {
    int i;
    if (!f) return;
    f_cpy(f->title, sizeof(f->title), title ? title : "Formulaire");
    f->num_fields = 0;
    f->active_field = 0;
    f->status[0] = 0;
    f->cmd_out[0] = 0;
    f->submit_fn = 0;
    f->ctx = 0;
    f->verb[0] = 0;
    f->verb_from_select = 0;
    f->last_cmd[0] = 0;
    f->confirm = 0;
    f->armed = 0;
    for (i = 0; i < OSUI_FORM_MAX_FIELDS; i++) {
        f->fields[i].type = OSUI_FORM_FIELD_TEXT;
        f->fields[i].label[0] = 0;
        f->fields[i].value[0] = 0;
        f->fields[i].val_len = 0;
        f->fields[i].num_options = 0;
        f->fields[i].sel_option = 0;
        f->fields[i].optional = 0;
        f->fields[i].rest = 0;
    }
}

int osui_form_add_text(osui_form_t *f, const char *label, const char *default_val) {
    int idx;
    if (!f || f->num_fields >= OSUI_FORM_MAX_FIELDS) return -1;
    idx = f->num_fields;
    f->fields[idx].type = OSUI_FORM_FIELD_TEXT;
    f_cpy(f->fields[idx].label, sizeof(f->fields[idx].label), label ? label : "Champ:");
    f_cpy(f->fields[idx].value, sizeof(f->fields[idx].value), default_val ? default_val : "");
    f->fields[idx].val_len = f_len(f->fields[idx].value);
    f->num_fields++;
    return idx;
}

int osui_form_add_select(osui_form_t *f, const char *label, const char *const *options, int count, int default_sel) {
    int idx, i;
    if (!f || f->num_fields >= OSUI_FORM_MAX_FIELDS) return -1;
    idx = f->num_fields;
    f->fields[idx].type = OSUI_FORM_FIELD_SELECT;
    f_cpy(f->fields[idx].label, sizeof(f->fields[idx].label), label ? label : "Choix:");
    f->fields[idx].num_options = count > OSUI_FORM_MAX_OPTIONS ? OSUI_FORM_MAX_OPTIONS : count;
    for (i = 0; i < f->fields[idx].num_options; i++) {
        f_cpy(f->fields[idx].options[i], sizeof(f->fields[idx].options[i]), options[i] ? options[i] : "");
    }
    f->fields[idx].sel_option = (default_sel >= 0 && default_sel < f->fields[idx].num_options) ? default_sel : 0;
    if (f->fields[idx].num_options > 0) {
        f_cpy(f->fields[idx].value, sizeof(f->fields[idx].value), f->fields[idx].options[f->fields[idx].sel_option]);
        f->fields[idx].val_len = f_len(f->fields[idx].value);
    }
    f->num_fields++;
    return idx;
}

int osui_form_add_submit(osui_form_t *f, const char *label) {
    int idx;
    if (!f || f->num_fields >= OSUI_FORM_MAX_FIELDS) return -1;
    idx = f->num_fields;
    f->fields[idx].type = OSUI_FORM_FIELD_SUBMIT;
    f_cpy(f->fields[idx].label, sizeof(f->fields[idx].label), label ? label : "[ Valider ]");
    f->num_fields++;
    return idx;
}

static int osui_form_validate_and_submit(osui_form_t *f, void *src) {
    int i;
    /* Check required fields */
    for (i = 0; i < f->num_fields; i++) {
        if (f->fields[i].type == OSUI_FORM_FIELD_TEXT && f->fields[i].val_len == 0 && !f->fields[i].optional) {
            f_cpy(f->status, sizeof(f->status), "Erreur: le champ '");
            f_cat(f->status, sizeof(f->status), f->fields[i].label);
            f_cat(f->status, sizeof(f->status), "' est requis.");
            return -1;
        }
    }
    if (f->confirm && !f->armed) {
        /* same rule as the parity panels: a mutation needs a second Enter */
        char cmd[160];
        f->armed = 1;
        f_cpy(f->status, sizeof(f->status), "Entree encore pour confirmer: ");
        if (osui_form_build_cmd(f, cmd, (int)sizeof(cmd)) == 0) f_cat(f->status, sizeof(f->status), cmd);
        return 0;
    }
    f->armed = 0;
    if (f->submit_fn) {
        return f->submit_fn(f, src, f->cmd_out, sizeof(f->cmd_out));
    }
    f_cpy(f->status, sizeof(f->status), "Formulaire valide et soumis.");
    return 0;
}

int osui_form_key(osui_form_t *f, int key, void *src) {
    osui_form_field_t *fld;
    if (!f || f->num_fields == 0) return 0;
    if (key != '\n' && key != '\r') f->armed = 0;

    if (key == K_TAB || key == K_DOWN) {
        f->active_field = (f->active_field + 1) % f->num_fields;
        return 1;
    }
    if (key == K_UP) {
        f->active_field = (f->active_field + f->num_fields - 1) % f->num_fields;
        return 1;
    }

    fld = &f->fields[f->active_field];

    if (fld->type == OSUI_FORM_FIELD_SUBMIT) {
        if (key == '\n' || key == '\r') {
            osui_form_validate_and_submit(f, src);
            return 1;
        }
    } else if (fld->type == OSUI_FORM_FIELD_SELECT) {
        if (key == K_LEFT && fld->sel_option > 0) {
            fld->sel_option--;
            f_cpy(fld->value, sizeof(fld->value), fld->options[fld->sel_option]);
            fld->val_len = f_len(fld->value);
            return 1;
        }
        if (key == K_RIGHT && fld->sel_option + 1 < fld->num_options) {
            fld->sel_option++;
            f_cpy(fld->value, sizeof(fld->value), fld->options[fld->sel_option]);
            fld->val_len = f_len(fld->value);
            return 1;
        }
        if (key == '\n' || key == '\r') {
            f->active_field = (f->active_field + 1) % f->num_fields;
            return 1;
        }
    } else if (fld->type == OSUI_FORM_FIELD_TEXT) {
        if (key == '\n' || key == '\r') {
            if (f->active_field + 1 < f->num_fields) {
                f->active_field++;
            } else {
                osui_form_validate_and_submit(f, src);
            }
            return 1;
        }
        if (key == '\b' || key == 127) {
            if (fld->val_len > 0) {
                fld->value[--fld->val_len] = 0;
            }
            return 1;
        }
        if (key >= 32 && key <= 126) {
            if (fld->val_len < OSUI_FORM_VAL_MAX - 1) {
                fld->value[fld->val_len++] = (char)key;
                fld->value[fld->val_len] = 0;
            }
            return 1;
        }
    }
    return 0;
}

int osui_form_click(osui_form_t *f, int row, int col, void *src) {
    (void)col;
    if (!f || row < 0 || row >= f->num_fields) return 0;
    f->active_field = row;
    if (f->fields[row].type == OSUI_FORM_FIELD_SUBMIT) {
        osui_form_validate_and_submit(f, src);
    }
    return 1;
}

void osui_form_render(const osui_form_t *f, osui_app_view_t *v) {
    int i, r;
    if (!f || !v) return;
    osui_app_view_clear(v);
    f_cpy(v->title, sizeof(v->title), f->title);
    f_cpy(v->tool, sizeof(v->tool), "[Tab: Champ suivant] [Entree: Soumettre]");
    f_cpy(v->status, sizeof(v->status), f->status[0] ? f->status : "Remplissez le formulaire");

    for (i = 0, r = 0; i < f->num_fields && r < OSUI_APP_ROWS - 2; i++, r++) {
        const osui_form_field_t *fld = &f->fields[i];
        char line[OSUI_APP_COLS];
        line[0] = 0;
        f_cat(line, sizeof(line), fld->label);
        f_cat(line, sizeof(line), " ");
        if (fld->type == OSUI_FORM_FIELD_TEXT) {
            f_cat(line, sizeof(line), "[ ");
            f_cat(line, sizeof(line), fld->value);
            if (i == f->active_field) f_cat(line, sizeof(line), "_");
            f_cat(line, sizeof(line), " ]");
        } else if (fld->type == OSUI_FORM_FIELD_SELECT) {
            f_cat(line, sizeof(line), "< ");
            f_cat(line, sizeof(line), fld->value);
            f_cat(line, sizeof(line), " >");
        } else if (fld->type == OSUI_FORM_FIELD_SUBMIT) {
            f_cat(line, sizeof(line), fld->label);
        }
        f_cpy(v->rows[r], sizeof(v->rows[r]), line);
        for (int k = 0; k < OSUI_APP_COLS; k++) {
            v->attr[r][k] = (i == f->active_field) ? OSUI_APP_A_SEL : OSUI_APP_A_TEXT;
        }
    }

    if (f->last_cmd[0] && r + 1 < OSUI_APP_ROWS) {
        /* the command really run, then its output (the shell's own text) */
        int k = 0, o;
        f_cpy(v->rows[r], sizeof(v->rows[r]), "$ ");
        f_cat(v->rows[r], sizeof(v->rows[r]), f->last_cmd);
        for (o = 0; o < OSUI_APP_COLS; o++) v->attr[r][o] = OSUI_APP_A_DIM;
        r++;
        while (f->cmd_out[k] && r < OSUI_APP_ROWS) {
            o = 0;
            while (f->cmd_out[k] && f->cmd_out[k] != '\n' && o < OSUI_APP_COLS - 1) v->rows[r][o++] = f->cmd_out[k++];
            v->rows[r][o] = 0;
            while (f->cmd_out[k] && f->cmd_out[k] != '\n') k++;
            if (f->cmd_out[k] == '\n') k++;
            r++;
        }
    }

    v->nrows = r;
}

/* Prebuilt collab ledger forms. They build exactly the shell grammar of
 * userspace/shell_collab.c (docs/collab.md) and run it through src->run,
 * the same registry path as a typed command: no second implementation. */

int osui_form_build_cmd(const osui_form_t *f, char *cmd, int max) {
    int i;
    if (!f || !cmd || max <= 0) return -1;
    cmd[0] = 0;
    if (f->num_fields < 1) return -1;
    if (f->fields[0].type == OSUI_FORM_FIELD_SELECT && f->verb_from_select) {
        f_cpy(cmd, max, f->fields[0].value);
        i = 1;
    } else {
        f_cpy(cmd, max, f->verb);
        i = 0;
    }
    for (; i < f->num_fields; i++) {
        const osui_form_field_t *fld = &f->fields[i];
        int k;
        if (fld->type == OSUI_FORM_FIELD_SUBMIT) continue;
        if (!fld->value[0]) { if (fld->optional) continue; return -1; }
        for (k = 0; fld->value[k]; k++) if (fld->value[k] == ' ' && !fld->rest) return -1;
        f_cat(cmd, max, " ");
        f_cat(cmd, max, fld->value);
    }
    return f_len(cmd) < max - 1 ? 0 : -1;
}

static int form_run_submit(osui_form_t *f, void *src_ptr, char *out, int max) {
    const osui_tool_src_t *src = (const osui_tool_src_t *)src_ptr;
    char cmd[256];
    int rc, k;
    if (osui_form_build_cmd(f, cmd, (int)sizeof(cmd)) < 0) {
        f_cpy(f->status, sizeof(f->status), "Erreur: champ vide ou espace interdit");
        return -1;
    }
    f_cpy(f->last_cmd, sizeof(f->last_cmd), cmd);
    if (!src || !src->run) { f_cpy(f->status, sizeof(f->status), "shell indisponible"); return -1; }
    rc = src->run(cmd, out, max);
    if (rc < 0) { f_cpy(f->status, sizeof(f->status), "refusee: "); f_cat(f->status, sizeof(f->status), cmd); return rc; }
    /* first output line in the status: the shell's own ok / error text */
    for (k = 0; out[k] == '\n' || out[k] == ' '; k++) {}
    {
        char line[OSUI_APP_COLS];
        int o = 0;
        while (out[k] && out[k] != '\n' && o < (int)sizeof(line) - 1) line[o++] = out[k++];
        line[o] = 0;
        f_cpy(f->status, sizeof(f->status), line[0] ? line : "(aucune sortie)");
    }
    return rc;
}

static void form_text(osui_form_t *f, const char *label, const char *def, int optional, int rest) {
    int i = osui_form_add_text(f, label, def);
    if (i >= 0) { f->fields[i].optional = optional; f->fields[i].rest = rest; }
}

void osui_form_collab_offer_init(osui_form_t *f) {
    osui_form_init(f, "Formulaire: offre de capacite (collab-offer)");
    f_cpy(f->verb, sizeof(f->verb), "collab-offer");
    form_text(f, "Nom:", "gpu", 0, 0);
    form_text(f, "Prix:", "10", 0, 0);
    form_text(f, "Capacite:", "1", 0, 0);
    osui_form_add_submit(f, "[ Publier l'offre ]");
    f->confirm = 1;
    f->submit_fn = form_run_submit;
}

void osui_form_collab_vote_init(osui_form_t *f) {
    static const char *const vote_opts[] = { "yes", "no" };
    osui_form_init(f, "Formulaire: vote (collab-vote)");
    f_cpy(f->verb, sizeof(f->verb), "collab-vote");
    form_text(f, "Proposant (pair):", "", 0, 0);
    form_text(f, "Sequence:", "", 0, 0);
    osui_form_add_select(f, "Choix:", vote_opts, 2, 0);
    osui_form_add_submit(f, "[ Voter ]");
    f->confirm = 1;
    f->submit_fn = form_run_submit;
}

void osui_form_collab_escrow_init(osui_form_t *f) {
    osui_form_init(f, "Formulaire: escrow (collab-escrow-create)");
    f_cpy(f->verb, sizeof(f->verb), "collab-escrow-create");
    form_text(f, "Beneficiaire (pair):", "", 0, 0);
    form_text(f, "Arbitre (pair|none):", "none", 0, 0);
    form_text(f, "Montant:", "", 0, 0);
    form_text(f, "Nom (option):", "", 1, 0);
    osui_form_add_submit(f, "[ Creer l'escrow ]");
    f->confirm = 1;
    f->submit_fn = form_run_submit;
}

void osui_form_collab_settle_init(osui_form_t *f) {
    static const char *const verbs[] = { "collab-escrow-release", "collab-escrow-refund", "collab-dispute-open" };
    osui_form_init(f, "Formulaire: solder un escrow");
    f->verb_from_select = 1;
    osui_form_add_select(f, "Action:", verbs, 3, 0);
    form_text(f, "Payeur (pair):", "", 0, 0);
    form_text(f, "Sequence:", "", 0, 0);
    form_text(f, "Preuve (litige):", "", 1, 1);
    osui_form_add_submit(f, "[ Executer ]");
    f->confirm = 1;
    f->submit_fn = form_run_submit;
}
