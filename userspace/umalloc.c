#include "umalloc.h"

/* Block header; size includes the header, low bit = used. */
typedef struct { unsigned size; unsigned pad[3]; } ublk_t;
#define HDR ((unsigned)sizeof(ublk_t))

static unsigned char *g_base;
static unsigned g_mapped, g_end;   /* g_end: bytes covered by blocks */
static umalloc_grow_fn g_grow;

void umalloc_init(void *base, umalloc_grow_fn grow) {
    g_base = (unsigned char *)base; g_grow = grow; g_mapped = 0; g_end = 0;
}

unsigned umalloc_mapped(void) { return g_mapped; }

unsigned umalloc_in_use(void) {
    unsigned off = 0, n = 0;
    while (off < g_end) {
        ublk_t *b = (ublk_t *)(g_base + off);
        if (b->size & 1U) n += (b->size & ~1U) - HDR;
        off += b->size & ~1U;
    }
    return n;
}

static void coalesce(void) {
    unsigned off = 0;
    while (off < g_end) {
        ublk_t *b = (ublk_t *)(g_base + off);
        unsigned s = b->size & ~1U;
        if (!(b->size & 1U)) {
            while (off + s < g_end) {
                ublk_t *nx = (ublk_t *)(g_base + off + s);
                if (nx->size & 1U) break;
                s += nx->size;
            }
            if (off + s == g_end) { g_end = off; return; }  /* free tail returns to the top */
            b->size = s;
        }
        off += s;
    }
}

void *umalloc(unsigned n) {
    unsigned need, off = 0;
    if (!g_base || !g_grow || n == 0 || n > 0x7FFFFFF0U - HDR) return 0;
    need = (n + HDR + 15U) & ~15U;
    while (off < g_end) {
        ublk_t *b = (ublk_t *)(g_base + off);
        unsigned s = b->size & ~1U;
        if (!(b->size & 1U) && s >= need) {
            if (s - need >= HDR + 16U) {
                ublk_t *rest = (ublk_t *)(g_base + off + need);
                rest->size = s - need;
                s = need;
            }
            b->size = s | 1U;
            return g_base + off + HDR;
        }
        off += s;
    }
    if (g_end + need > g_mapped) {
        int got = g_grow(g_end + need);
        if (got < 0 || (unsigned)got < g_end + need) return 0;
        g_mapped = (unsigned)got;
    }
    {
        ublk_t *b = (ublk_t *)(g_base + g_end);
        b->size = need | 1U;
        g_end += need;
        return (unsigned char *)b + HDR;
    }
}

void ufree(void *p) {
    ublk_t *b;
    if (!p) return;
    b = (ublk_t *)((unsigned char *)p - HDR);
    b->size &= ~1U;
    coalesce();
}

void *urealloc(void *p, unsigned n) {
    ublk_t *b;
    unsigned have, i;
    unsigned char *q;
    if (!p) return umalloc(n);
    if (n == 0) { ufree(p); return 0; }
    b = (ublk_t *)((unsigned char *)p - HDR);
    have = (b->size & ~1U) - HDR;
    if (have >= n) return p;
    /* last block: grow in place */
    if ((unsigned char *)b + (b->size & ~1U) == g_base + g_end) {
        unsigned need = (n + HDR + 15U) & ~15U, off = (unsigned)((unsigned char *)b - g_base);
        if (off + need > g_mapped) {
            int got = g_grow(off + need);
            if (got < 0 || (unsigned)got < off + need) return 0;
            g_mapped = (unsigned)got;
        }
        b->size = need | 1U;
        g_end = off + need;
        return p;
    }
    q = (unsigned char *)umalloc(n);
    if (!q) return 0;
    for (i = 0; i < have; i++) q[i] = ((unsigned char *)p)[i];
    ufree(p);
    return q;
}
