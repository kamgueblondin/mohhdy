/* wf_form.h - Lot 71: HTML forms of a fetched page in the guest browser.
 *
 * The reader view (wf_html.c) records each <form> and its controls here
 * while it reduces the page: text-like inputs (text, search, email, url,
 * tel, number, password, textarea), hidden inputs, checkboxes, radios,
 * <select> with its options (WF_FORM_OPTIONS per select, a shared pool of
 * WF_FORM_OPT_POOL bytes per page), and submit buttons. A click on a
 * checkbox toggles it, on a radio checks it (and unchecks its group), on a
 * select moves to the next option (wrapping). The
 * desktop shows an editable field as a link "[ value ]" whose href is
 * "mohhdy-form:f:N"; a click focuses field N, keys edit it, Enter submits
 * its form. Submission follows the HTML rules for the subset kept:
 * application/x-www-form-urlencoded, GET (query string replaces the
 * action's query) or POST (body), values sent as UTF-8.
 *
 * Bounded: WF_FORM_MAX forms, WF_FORM_FIELDS controls per page, fixed
 * size strings. No JavaScript: forms that need it do not work (said in
 * the docs). Host tested: tests/unit/userspace/test_wf_form.c. */
#ifndef WF_FORM_H
#define WF_FORM_H
#include "wf_http.h"

#define WF_FORM_MAX 8
#define WF_FORM_FIELDS 48
#define WF_FORM_NAME 48
#define WF_FORM_VALUE 256
#define WF_FORM_LABEL 48
#define WF_FORM_ACTION 512
#define WF_FORM_SUBMIT_MAX (WF_URL_HOST_MAX + WF_URL_PATH_MAX + 16)
#define WF_FORM_BODY_MAX 2048
#define WF_FORM_OPTIONS 24      /* options kept per select */
#define WF_FORM_OPT_POOL 4096   /* option values and labels of a page */

enum {
    WF_FIELD_TEXT = 1,     /* text, search, email, url, tel, number, (no type) */
    WF_FIELD_PASSWORD = 2, /* shown as '*' */
    WF_FIELD_TEXTAREA = 3,
    WF_FIELD_HIDDEN = 4,
    WF_FIELD_SUBMIT = 5,   /* input type=submit / image, button (type submit) */
    WF_FIELD_CHECKBOX = 6, /* Lot 71: sent only when checked */
    WF_FIELD_RADIO = 7,    /* sent only when checked; one per (form, name) */
    WF_FIELD_SELECT = 8,   /* value = the selected option's value */
};
enum { WF_METHOD_GET = 0, WF_METHOD_POST = 1 };

typedef struct {
    uint8_t type;
    uint8_t form;          /* index of its form */
    uint8_t named;         /* has a name: sent on submit */
    uint8_t checked;       /* checkbox / radio */
    char name[WF_FORM_NAME];
    char value[WF_FORM_VALUE];   /* display bytes (CP1252) */
    char label[WF_FORM_LABEL];   /* placeholder, aria-label or button text */
    int16_t cursor;              /* edit position in value */
    int16_t maxlen;              /* maxlength or WF_FORM_VALUE - 1 */
    int16_t opt_start;           /* select: offset of its first option in opt_pool */
    uint8_t nopt, sel;           /* select: options kept, selected one */
} wf_field_t;

typedef struct {
    char action[WF_FORM_ACTION]; /* absolute URL (resolved against the page) */
    uint8_t method;
    uint8_t multipart;           /* enctype multipart/form-data: refused on submit */
    uint8_t nfields;
    uint8_t implicit;            /* controls outside any <form> */
} wf_form_t;

typedef struct {
    wf_form_t form[WF_FORM_MAX];
    int nforms;
    wf_field_t field[WF_FORM_FIELDS];
    int nfields;
    int focus;                   /* focused field or -1 */
    int dropped;                 /* controls past the limits */
    int current;                 /* form being parsed, -1 outside any form */
    int charset;                 /* page charset (WF_CS_*, userspace/wf_entity.h): browsers
                                    encode a form in the page charset. Lot 71: Latin-1 / CP1252
                                    (field "latin1"); Lot 72: any single-byte charset, a
                                    character it lacks is sent as &#N; (HTML rule) */
    char opt_pool[WF_FORM_OPT_POOL];   /* "value\0label\0" per option */
    int opt_used;
    int opt_dropped;
} wf_form_page_t;

typedef struct {
    int method;
    char url[WF_FORM_SUBMIT_MAX];
    char body[WF_FORM_BODY_MAX];  /* POST only, NUL-terminated */
    int body_len;
    const char *content_type;     /* POST: "application/x-www-form-urlencoded" */
} wf_form_submit_t;

void wf_form_reset(wf_form_page_t *p);
/* <form action method enctype>: action resolved against base (empty
 * action = the page URL). Returns the form index or -1. */
int wf_form_begin(wf_form_page_t *p, const wf_url_t *base, const char *action, const char *method, const char *enctype);
void wf_form_end(wf_form_page_t *p);
/* A control in the current form (or a form-less control: form -1 is
 * attached to an implicit form with the page URL as action). type is the
 * HTML type attribute (input) or "textarea" / "button" / "select".
 * Returns the field index, or -1 (ignored type or limits). */
int wf_form_add(wf_form_page_t *p, const wf_url_t *base, const char *type, const char *name, const char *value,
                const char *label, int checked, int maxlen);
/* Lot 71: an <option> of select field `field` (the last one added).
 * Returns the option index or -1 (not a select, limits). */
int wf_form_add_option(wf_form_page_t *p, int field, const char *value, const char *label, int selected);
/* option k of a select: value and label (0 when out of range) */
const char *wf_form_option_value(const wf_form_page_t *p, int field, int k);
const char *wf_form_option_label(const wf_form_page_t *p, int field, int k);
/* click / Space on a checkbox, radio or select; 0 changed, -1 not one */
int wf_form_activate(wf_form_page_t *p, int field);
/* 1 for checkbox, radio and select (a click acts, no text to edit) */
int wf_form_is_toggle(const wf_form_page_t *p, int field);
/* Editing the focused field (display bytes). 0 when changed, -1 if not.
 * Lot 71: checkbox, radio and select take the focus too (Tab); Space
 * activates them, LEFT/RIGHT/HOME/END and a letter move a select. */
int wf_form_focus(wf_form_page_t *p, int field);
int wf_form_focus_next(wf_form_page_t *p, int dir);
int wf_form_insert(wf_form_page_t *p, char c);
int wf_form_backspace(wf_form_page_t *p);
int wf_form_delete(wf_form_page_t *p);
int wf_form_move(wf_form_page_t *p, int delta);   /* delta -1000 = start, 1000 = end */
/* text fields: the new text. checkbox: on|off|1|0|yes|no. radio: on
 * (checks it). select: an option value, an option label, or #N (1-based). */
int wf_form_set(wf_form_page_t *p, int field, const char *value);
/* Text shown for a field: "[ value_ ]" with the cursor when focused,
 * '*' for passwords, the label (placeholder) when empty. */
int wf_form_display(const wf_form_page_t *p, int field, char *out, int cap);
/* Builds the request of form f submitted by submitter (a SUBMIT field
 * index or -1). 0, or -1 (no such form, multipart, overflow). */
int wf_form_submit(const wf_form_page_t *p, int f, int submitter, wf_form_submit_t *out);
/* application/x-www-form-urlencoded pair encoding of display bytes
 * (converted to UTF-8 first). Returns the new length or -1 on overflow. */
int wf_form_urlencode_append(char *out, int cap, int len, const char *s);
/* Lot 71: same in the page charset. Lot 72: cs is WF_CS_* (0 = UTF-8,
 * 1 = Latin-1 as before); s is display text with glyph pages, each
 * character is written in cs, or as &#N; when cs has no byte for it. */
int wf_form_urlencode_append_cs(char *out, int cap, int len, const char *s, int cs);
/* First form that has a visible text field (for "web-input"), or -1. */
int wf_form_first_text(const wf_form_page_t *p, int form);
/* "mohhdy-form:f:N" -> field N; "mohhdy-form:s:N" -> submit field N.
 * Returns 'f' or 's' with *n set, or 0. */
int wf_form_href(const char *href, int *n);
#endif
