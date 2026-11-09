/* wf_qstring.h - Lot 86: application/x-www-form-urlencoded pairs.
 * The form submit path builds this encoding. This codec parses it back
 * so a preview can be checked, and rejects a broken body before it is
 * sent. No heap. '+' is a space. %HH is a byte. */
#ifndef WF_QSTRING_H
#define WF_QSTRING_H

#define WF_QS_PAIRS 24
#define WF_QS_KEY 48
#define WF_QS_VAL 180

typedef struct {
    char key[WF_QS_KEY];
    char val[WF_QS_VAL];
} wf_qs_pair_t;

typedef struct {
    wf_qs_pair_t pair[WF_QS_PAIRS];
    int n;
    int dropped;
    int bad;
} wf_qs_t;

/* 0 parsed, -1 bad percent-encoding or a missing argument. */
int wf_qs_parse(const char *s, int n, wf_qs_t *out);
int wf_qs_get(const wf_qs_t *q, const char *key, char *out, int cap);
/* Writes the pairs back. Returns the length, or -1 if it does not fit. */
int wf_qs_format(const wf_qs_t *q, char *out, int cap);
/* 1 when both bodies carry the same pairs, ignoring order. */
int wf_qs_same(const char *a, int an, const char *b, int bn);
#endif
