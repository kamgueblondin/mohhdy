/* wf_formfill.h - Lot 86: autofill inside the form editor, and the
 * GET or POST request a submit would send. Passwords and hidden fields
 * are sent (HTML does that) and are not remembered. */
#ifndef WF_FORMFILL_H
#define WF_FORMFILL_H
#include "wf_form.h"
#include "wf_autofill.h"

#define WF_FILL_OFFERS 8

typedef struct {
    int ent;
    char name[WF_AF_NAME];
    char value[WF_AF_VALUE];
} wf_fill_offer_t;

/* Host of an absolute URL, lower case, no port. 0, or -1. */
int wf_formfill_host(const char *url, char *host, int cap);
/* Offers for one field. Exact host and field name. Returns the count. */
int wf_formfill_offers(const wf_autofill_t *a, const wf_form_page_t *p, int field,
                       const char *host, wf_fill_offer_t *out, int cap);
/* Copy one offer into the field. 0, or -1. */
int wf_formfill_apply(wf_form_page_t *p, int field, const wf_fill_offer_t *o);
/* Remember the named, non-secret values of one form (form < 0: every form).
 * Returns how many values were stored. */
int wf_formfill_remember(wf_autofill_t *a, const wf_form_page_t *p, int form, const char *host);
/* The request shell_page_submit would send, without the network.
 * "GET url\n" or "POST url\nbody\n". Returns the length, or -1. */
int wf_formfill_preview(const wf_form_page_t *p, int field, char *out, int cap);
#endif
