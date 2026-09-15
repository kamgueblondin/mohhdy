#ifndef STRING_H
#define STRING_H

#include <stddef.h>

void* memset(void* bufptr, int value, size_t size);
void* memcpy(void* dstptr, const void* srcptr, size_t size);
int strcmp(const char *s1, const char *s2);

#endif
