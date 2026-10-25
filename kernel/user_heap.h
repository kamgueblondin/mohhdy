#ifndef USER_HEAP_H
#define USER_HEAP_H
#include <stdint.h>
/* UX5: SYS_HEAP_GROW core. map_page(ctx, user_va) maps one zeroed RW user
 * page and returns 0, < 0 when out of frames. *size is the mapped size and
 * only grows; pages already mapped stay mapped on failure. */
typedef int (*user_heap_map_fn)(void *ctx, uint32_t user_va);
int user_heap_grow(uint32_t *size, uint32_t want, user_heap_map_fn map_page, void *ctx);
#endif
