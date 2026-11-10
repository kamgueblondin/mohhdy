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
    int site;                   /* Lot 88: 1 when from another host of the site */
    char host[WF_AF_HOST];      /* the host that holds it */
} wf_fill_offer_t;

/* Host of an absolute URL, lower case, no port. 0, or -1. */
int wf_formfill_host(const char *url, char *host, int cap);
/* Offers for one field: the exact host first, then (Lot 88) the other
 * hosts of the same site (registrable domain), without repeated values.
 * Returns the count. */
int wf_formfill_offers(const wf_autofill_t *a, const wf_form_page_t *p, int field,
                       const char *host, wf_fill_offer_t *out, int cap);
/* Copy one offer into the field. 0, or -1. */
int wf_formfill_apply(wf_form_page_t *p, int field, const wf_fill_offer_t *o);
/* Lot 86: the form editor key that fills the focused field. Text and
 * textarea fields only (not toggles, select, password or hidden). The
 * first offer for the field (exact host and name) goes in, the cursor
 * sits at its end. 0 filled, -1 nothing to offer (the key goes on). */
int wf_formfill_key(const wf_autofill_t *a, wf_form_page_t *p, int field, const char *host);
/* Remember the named, non-secret values of one form (form < 0: every form).
 * Returns how many values were stored. */
int wf_formfill_remember(wf_autofill_t *a, const wf_form_page_t *p, int form, const char *host);
/* The request shell_page_submit would send, without the network.
 * "GET url\n" or "POST url\nbody\n". Returns the length, or -1. */
int wf_formfill_preview(const wf_form_page_t *p, int field, char *out, int cap);
#endif
