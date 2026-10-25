/* osui_form.h - Reusable GUI Form Component for MOHHDY OS (PR 4)
 * Supports text fields, option selection, field navigation and validation.
 * Used for collaborative ledger forms (offers, votes, escrow) and generic inputs. */
#ifndef OSUI_FORM_H
#define OSUI_FORM_H

#include "os_syscalls.h"
#include "osui_app.h"

#define OSUI_FORM_MAX_FIELDS 8
#define OSUI_FORM_MAX_OPTIONS 8
#define OSUI_FORM_VAL_MAX 64

typedef enum {
    OSUI_FORM_FIELD_TEXT = 0,
    OSUI_FORM_FIELD_SELECT = 1,
    OSUI_FORM_FIELD_SUBMIT = 2
} osui_form_field_type_t;

typedef struct {
    osui_form_field_type_t type;
    char label[32];
    char value[OSUI_FORM_VAL_MAX];
    int val_len;
    /* SELECT options */
    char options[OSUI_FORM_MAX_OPTIONS][32];
    int num_options;
    int sel_option;
    int optional;   /* may stay empty (then left out of the command) */
    int rest;       /* last word: spaces allowed */
} osui_form_field_t;

typedef struct osui_form_s osui_form_t;

struct osui_form_s {
    char title[64];
    osui_form_field_t fields[OSUI_FORM_MAX_FIELDS];
    int num_fields;
    int active_field;
    char status[OSUI_APP_COLS];
    char cmd_out[OSUI_APP_COLS * 4];
    int (*submit_fn)(osui_form_t *f, void *src, char *out, int max);
    void *ctx;
    char verb[40];          /* shell command the fields complete */
    int verb_from_select;   /* field 0 (a list) names the command */
    char last_cmd[160];     /* command line really run on submit */
    int confirm;            /* 1: submit needs a second Enter (mutation) */
    int armed;
};

/* Core form functions */
void osui_form_init(osui_form_t *f, const char *title);
int osui_form_add_text(osui_form_t *f, const char *label, const char *default_val);
int osui_form_add_select(osui_form_t *f, const char *label, const char *const *options, int count, int default_sel);
int osui_form_add_submit(osui_form_t *f, const char *label);
int osui_form_key(osui_form_t *f, int key, void *src);
int osui_form_click(osui_form_t *f, int row, int col, void *src);
void osui_form_render(const osui_form_t *f, osui_app_view_t *v);

/* Pre-built collaborative ledger forms */
void osui_form_collab_offer_init(osui_form_t *f);
void osui_form_collab_vote_init(osui_form_t *f);
void osui_form_collab_escrow_init(osui_form_t *f);
void osui_form_collab_settle_init(osui_form_t *f);
/* verb + non empty fields; -1 on a missing required field or a space in a
 * one-word field (it would shift the shell arguments). */
int osui_form_build_cmd(const osui_form_t *f, char *cmd, int max);

#endif
