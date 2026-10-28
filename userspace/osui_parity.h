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

/* Lot 45: the slash programs of the desktop chat that are not a parity
 * family (tool windows, files, editor, panes). kind is the OS_FB_APP_* of
 * a tool window opened by tool_slash, 0 for the other handlers. One table
 * for the dispatch and for the "gui-help" listing (no duplicated list). */
typedef struct {
    const char *slash;
    int kind;
    const char *what;
} osui_par_builtin_t;
extern const osui_par_builtin_t osui_par_builtins[];
extern const int osui_par_builtin_count;
/* writes the slash program list (builtins then families) through put;
 * returns the number of entries listed */
int osui_par_slash_list(void (*put)(const char *s));

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
/* 1 when a path ends in a format the shared viewer decodes
 * (.pgm .ppm .pnm .bmp .png .jpg). .jpeg is not a shipped name. */
int osui_path_is_image(const char *p);
#endif
