/* wf_autofill.h - Lot 86: remembered form values, per host.
 * Passwords are refused. The value is the text the operator typed, kept
 * so the next visit of the same host and field name can fill it. Session
 * file only when the caller saves it. No heap. */
#ifndef WF_AUTOFILL_H
#define WF_AUTOFILL_H
#include <stdint.h>

#define WF_AF_MAX 32
#define WF_AF_HOST 96
#define WF_AF_NAME 48
#define WF_AF_VALUE 120
/* Lot 87: values kept per host and field name, newest first (the current
 * value and two older ones). A key still counts once in n. */
#define WF_AF_HIST 3

typedef struct {
    int used;
    char host[WF_AF_HOST];
    char name[WF_AF_NAME];
    char value[WF_AF_VALUE];
    int nold;                                  /* Lot 87: 0..WF_AF_HIST-1 */
    char old[WF_AF_HIST - 1][WF_AF_VALUE];     /* older values, newest first */
} wf_af_ent_t;

typedef struct {
    wf_af_ent_t e[WF_AF_MAX];
    int n;
    uint32_t stored, refused, hits;
} wf_autofill_t;

void wf_autofill_init(wf_autofill_t *a);
/* 1 when this HTML type must not be remembered. */
int wf_autofill_secret(const char *type);
/* 0 stored or replaced, -1 refused. */
int wf_autofill_put(wf_autofill_t *a, const char *host, const char *name, const char *value, const char *type);
/* 0 and out receives the value, -1 none. */
int wf_autofill_get(wf_autofill_t *a, const char *host, const char *name, char *out, int cap);
/* Lot 87: value idx of a key, 0 the newest. 0, or -1 when there is none. */
int wf_autofill_get_at(wf_autofill_t *a, const char *host, const char *name, int idx, char *out, int cap);
/* Lot 87: how many values a key holds (0 when unknown). */
int wf_autofill_count(const wf_autofill_t *a, const char *host, const char *name);
int wf_autofill_forget(wf_autofill_t *a, const char *host);
int wf_autofill_save(const wf_autofill_t *a, char *out, int cap);
int wf_autofill_load(wf_autofill_t *a, const char *text, int len);
int wf_autofill_text(const wf_autofill_t *a, char *out, int cap);
#endif
