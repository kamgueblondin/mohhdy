/* wf_form.h - Lot 71: HTML forms of a fetched page in the guest browser.
 *
 * The reader view (wf_html.c) records each <form> and its controls here
 * while it reduces the page: text-like inputs (text, search, email, url,
 * tel, number, password, textarea), hidden inputs, checked checkboxes and
 * radios, the selected option of a <select>, and submit buttons. The
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

enum {
    WF_FIELD_TEXT = 1,     /* text, search, email, url, tel, number, (no type) */
    WF_FIELD_PASSWORD = 2, /* shown as '*' */
    WF_FIELD_TEXTAREA = 3,
    WF_FIELD_HIDDEN = 4,   /* also checked checkbox / radio, select value */
    WF_FIELD_SUBMIT = 5,   /* input type=submit / image, button (type submit) */
};
enum { WF_METHOD_GET = 0, WF_METHOD_POST = 1 };

typedef struct {
    uint8_t type;
    uint8_t form;          /* index of its form */
    uint8_t named;         /* has a name: sent on submit */
    uint8_t pad;
    char name[WF_FORM_NAME];
    char value[WF_FORM_VALUE];   /* display bytes (CP1252) */
    char label[WF_FORM_LABEL];   /* placeholder, aria-label or button text */
    int16_t cursor;              /* edit position in value */
    int16_t maxlen;              /* maxlength or WF_FORM_VALUE - 1 */
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
    int latin1;                  /* page charset Latin-1 / CP1252: submitted as is (browsers
                                    encode a form in the page charset), else UTF-8 */
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
/* Editing the focused field (display bytes). 0 when changed, -1 if not. */
int wf_form_focus(wf_form_page_t *p, int field);
int wf_form_focus_next(wf_form_page_t *p, int dir);
int wf_form_insert(wf_form_page_t *p, char c);
int wf_form_backspace(wf_form_page_t *p);
int wf_form_delete(wf_form_page_t *p);
int wf_form_move(wf_form_page_t *p, int delta);   /* delta -1000 = start, 1000 = end */
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
/* Lot 71: same in the page charset (latin1: display bytes, CP1252). */
int wf_form_urlencode_append_cs(char *out, int cap, int len, const char *s, int latin1);
/* First form that has a visible text field (for "web-input"), or -1. */
int wf_form_first_text(const wf_form_page_t *p, int form);
/* "mohhdy-form:f:N" -> field N; "mohhdy-form:s:N" -> submit field N.
 * Returns 'f' or 's' with *n set, or 0. */
int wf_form_href(const char *href, int *n);
#endif
