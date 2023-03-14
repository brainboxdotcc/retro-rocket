#ifndef __MEMCPY_H__
#define __MEMCPY_H__

#include "kernel.h"

void _memset(void *dest, char val, u64 len);
void *memcpy(void *dest, const void *src, u64 len);
void *memmove(void *dest, const void *src, u64 n);
int memcmp(const void *s1, const void *s2, u64 n);

#endif
