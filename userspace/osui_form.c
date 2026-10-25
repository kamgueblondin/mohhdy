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
    for (i = 0; i < OSUI_FORM_MAX_FIELDS; i++) {
        f->fields[i].type = OSUI_FORM_FIELD_TEXT;
        f->fields[i].label[0] = 0;
        f->fields[i].value[0] = 0;
        f->fields[i].val_len = 0;
        f->fields[i].num_options = 0;
        f->fields[i].sel_option = 0;
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
        if (f->fields[i].type == OSUI_FORM_FIELD_TEXT && f->fields[i].val_len == 0) {
            f_cpy(f->status, sizeof(f->status), "Erreur: le champ '");
            f_cat(f->status, sizeof(f->status), f->fields[i].label);
            f_cat(f->status, sizeof(f->status), "' est requis.");
            return -1;
        }
    }
    if (f->submit_fn) {
        return f->submit_fn(f, src, f->cmd_out, sizeof(f->cmd_out));
    }
    f_cpy(f->status, sizeof(f->status), "Formulaire valide et soumis.");
    return 0;
}

int osui_form_key(osui_form_t *f, int key, void *src) {
    osui_form_field_t *fld;
    if (!f || f->num_fields == 0) return 0;

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

    if (f->cmd_out[0]) {
        /* Render output below fields if any */
        int out_line = r + 1;
        if (out_line < OSUI_APP_ROWS - 1) {
            f_cpy(v->rows[out_line], sizeof(v->rows[out_line]), "--- Resultat ---");
            for (int k = 0; k < OSUI_APP_COLS; k++) v->attr[out_line][k] = OSUI_APP_A_DIM;
        }
    }

    v->nrows = r;
}

/* Prebuilt collab ledger forms */

static int collab_offer_submit(osui_form_t *f, void *src_ptr, char *out, int max) {
    const osui_tool_src_t *src = (const osui_tool_src_t *)src_ptr;
    char cmd[256];
    cmd[0] = 0;
    f_cat(cmd, sizeof(cmd), "collab-offer ");
    f_cat(cmd, sizeof(cmd), f->fields[0].value); /* task */
    f_cat(cmd, sizeof(cmd), " ");
    f_cat(cmd, sizeof(cmd), f->fields[1].value); /* amount */
    f_cat(cmd, sizeof(cmd), " ");
    f_cat(cmd, sizeof(cmd), f->fields[2].value); /* desc */

    if (src && src->run) {
        int rc = src->run(cmd, out, max);
        if (rc >= 0) f_cpy(f->status, sizeof(f->status), "Offre creee avec succes.");
        else f_cpy(f->status, sizeof(f->status), "Echec de la creation de l'offre.");
        return rc;
    }
    f_cpy(f->status, sizeof(f->status), cmd);
    return 0;
}

void osui_form_collab_offer_init(osui_form_t *f) {
    osui_form_init(f, "Formulaire: Creer une offre (Collab)");
    osui_form_add_text(f, "Tache ID:", "t1");
    osui_form_add_text(f, "Montant:", "100");
    osui_form_add_text(f, "Description:", "Developpement feature");
    osui_form_add_submit(f, "[ Soumettre l'Offre ]");
    f->submit_fn = collab_offer_submit;
}

static int collab_vote_submit(osui_form_t *f, void *src_ptr, char *out, int max) {
    const osui_tool_src_t *src = (const osui_tool_src_t *)src_ptr;
    char cmd[256];
    cmd[0] = 0;
    f_cat(cmd, sizeof(cmd), "collab-vote ");
    f_cat(cmd, sizeof(cmd), f->fields[0].value); /* proposal */
    f_cat(cmd, sizeof(cmd), " ");
    f_cat(cmd, sizeof(cmd), f->fields[1].value); /* choice */
    f_cat(cmd, sizeof(cmd), " ");
    f_cat(cmd, sizeof(cmd), f->fields[2].value); /* reason */

    if (src && src->run) {
        int rc = src->run(cmd, out, max);
        if (rc >= 0) f_cpy(f->status, sizeof(f->status), "Vote enregistre avec succes.");
        else f_cpy(f->status, sizeof(f->status), "Echec du vote.");
        return rc;
    }
    f_cpy(f->status, sizeof(f->status), cmd);
    return 0;
}

void osui_form_collab_vote_init(osui_form_t *f) {
    static const char *const vote_opts[] = { "approuver", "rejeter" };
    osui_form_init(f, "Formulaire: Voter une proposition (Collab)");
    osui_form_add_text(f, "Proposition ID:", "p1");
    osui_form_add_select(f, "Choix:", vote_opts, 2, 0);
    osui_form_add_text(f, "Raison:", "validation qualite");
    osui_form_add_submit(f, "[ Soumettre le Vote ]");
    f->submit_fn = collab_vote_submit;
}

static int collab_escrow_submit(osui_form_t *f, void *src_ptr, char *out, int max) {
    const osui_tool_src_t *src = (const osui_tool_src_t *)src_ptr;
    char cmd[256];
    cmd[0] = 0;
    if (f->fields[2].sel_option == 0) f_cat(cmd, sizeof(cmd), "collab-escrow-create ");
    else if (f->fields[2].sel_option == 1) f_cat(cmd, sizeof(cmd), "collab-escrow-release ");
    else f_cat(cmd, sizeof(cmd), "collab-dispute-open ");

    f_cat(cmd, sizeof(cmd), f->fields[0].value); /* task */
    f_cat(cmd, sizeof(cmd), " ");
    f_cat(cmd, sizeof(cmd), f->fields[1].value); /* amount */

    if (src && src->run) {
        int rc = src->run(cmd, out, max);
        if (rc >= 0) f_cpy(f->status, sizeof(f->status), "Operation escrow executee.");
        else f_cpy(f->status, sizeof(f->status), "Echec de l'operation escrow.");
        return rc;
    }
    f_cpy(f->status, sizeof(f->status), cmd);
    return 0;
}

void osui_form_collab_escrow_init(osui_form_t *f) {
    static const char *const escrow_opts[] = { "deposer", "liberer", "litige" };
    osui_form_init(f, "Formulaire: Depot/Escrow (Collab)");
    osui_form_add_text(f, "Tache ID:", "e1");
    osui_form_add_text(f, "Montant:", "500");
    osui_form_add_select(f, "Action:", escrow_opts, 3, 0);
    osui_form_add_submit(f, "[ Executer Escrow ]");
    f->submit_fn = collab_escrow_submit;
}
