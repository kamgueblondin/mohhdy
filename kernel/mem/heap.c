#include "heap.h"
#include "pmm.h"
#include <stdint.h>

/*
 * Allocation par plages physiques contigues. Chaque allocation est arrondie a
 * la page; le moteur GPT-2 peut ainsi reserver ses activations sans dependre
 * d'une bibliotheque d'execution hote. Les allocations restent non liberables
 * par taille inconnue dans cette premiere version du noyau.
 */

void init_heap() {
    // No-op, the PMM is initialized before the VMM/heap.
}

void* kmalloc(size_t size) {
    if (size == 0) return NULL;
    uint32_t pages = (uint32_t)((size + PAGE_SIZE - 1) / PAGE_SIZE);
    return pmm_alloc_pages(pages);
}

void* kmalloc_aligned(size_t size) {
    return kmalloc(size);
}

void kfree(void* ptr) {
    /*
     * A size-aware free API is required to return a multi-page allocation.
     * Keep this compatibility entry point as a no-op for now; GPT-2 buffers
     * are intentionally retained for the lifetime of the boot session.
     */
    (void)ptr;
}
