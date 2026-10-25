/* umalloc.h - UX5 user heap: first-fit malloc/free/realloc over a region
 * that only grows (SYS_HEAP_GROW in the guest, a static array in host
 * tests). Single-threaded, 16-byte aligned blocks. */
#ifndef UMALLOC_H
#define UMALLOC_H
/* grow(want) maps want bytes at base and returns the bytes mapped (< 0 on
 * failure); base must stay the same across calls. */
typedef int (*umalloc_grow_fn)(unsigned want);
void umalloc_init(void *base, umalloc_grow_fn grow);
void *umalloc(unsigned n);
void ufree(void *p);
void *urealloc(void *p, unsigned n);
unsigned umalloc_mapped(void);   /* bytes the heap has mapped */
unsigned umalloc_in_use(void);   /* bytes in live blocks */
#endif
