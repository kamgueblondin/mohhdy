/* clipboard.c - see clipboard.h. */
#include "clipboard.h"

static char g_text[OS_CLIP_MAX];
static int g_len;
static unsigned g_seq;

int clipboard_set(const char *text, int len) {
    int i;
    if (!text || len < 0) len = 0;
    if (len > OS_CLIP_MAX) len = OS_CLIP_MAX;
    for (i = 0; i < len; i++) g_text[i] = text[i];
    g_len = len;
    g_seq++;
    return len;
}

int clipboard_get(char *out, int max) {
    int i, n;
    if (!out || max <= 0) return 0;
    n = g_len < max - 1 ? g_len : max - 1;
    for (i = 0; i < n; i++) out[i] = g_text[i];
    out[n] = 0;
    return n;
}

int clipboard_len(void) { return g_len; }
unsigned clipboard_seq(void) { return g_seq; }
