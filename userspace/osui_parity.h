/* osui_parity.h - UX5 "GUI parity": one desktop panel per terminal
 * command family. A panel is only a menu of real shell command lines:
 * the window runs them through the same shell code as the terminal
 * (osui_tool_src_t.run -> shell_app_run), so nothing is reimplemented
 * and the kernel checks the rights exactly as for a typed command.
 *
 * Every command named here must exist in shared/multiboot_shell_commands.json
 * (checked by scripts/check_gui_parity.py and test_osui_parity). */
#ifndef OSUI_PARITY_H
#define OSUI_PARITY_H

#define OSUI_PAR_ARG 1   /* needs the text of the input line appended */
#define OSUI_PAR_MUT 2   /* changes state: Enter twice to confirm */

typedef struct {
    const char *label;   /* menu text */
    const char *cmd;     /* shell command line (args appended when ARG) */
    int flags;
    const char *hint;    /* expected input when ARG */
} osui_par_entry_t;

typedef struct {
    int kind;            /* OS_FB_APP_* */
    const char *slash;   /* "/net" */
    const char *title;   /* window title */
    const char *family;  /* family name of the inventory doc */
    const osui_par_entry_t *entries;
    int count;
} osui_par_family_t;

extern const osui_par_family_t osui_par_families[];
extern const int osui_par_family_count;

/* family of an app kind, 0 when the kind is not a parity panel */
const osui_par_family_t *osui_par_family(int kind);
/* kind of a slash command ("/net"), 0 when unknown */
int osui_par_kind_of(const char *slash);
/* the command name (first word) of an entry, into out; returns out */
const char *osui_par_cmd_name(const osui_par_entry_t *e, char *out, int max);
/* builds the line to run: cmd, plus " " input when ARG. Returns 0, or
 * -1 when ARG and the input is empty */
int osui_par_build(const osui_par_entry_t *e, const char *input, char *out, int max);
#endif
