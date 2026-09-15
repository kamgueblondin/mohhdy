#ifndef HEAP_H
#define HEAP_H

#include <stddef.h>
#include <stdint.h>

void* kmalloc(size_t size);
void* kmalloc_aligned(size_t size);
void kfree(void* ptr);

void init_heap();

#endif
