#include "user_heap.h"
#include "../include/os_syscalls.h"

int user_heap_grow(uint32_t *size, uint32_t want, user_heap_map_fn map_page, void *ctx) {
    uint32_t target;
    if (!size || !map_page) return OS_HEAP_NO_MEMORY;
    if (want > OS_USER_HEAP_MAX) return OS_HEAP_TOO_BIG;
    target = (want + 0xFFFU) & ~0xFFFU;
    while (*size < target) {
        if (map_page(ctx, OS_USER_HEAP_BASE + *size) != 0) return OS_HEAP_NO_MEMORY;
        *size += 0x1000U;
    }
    return (int)*size;
}
