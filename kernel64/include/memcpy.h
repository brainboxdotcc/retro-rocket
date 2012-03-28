#ifndef __MEMCPY_H__
#define __MEMCPY_H__

#include "kernel.h"

void _memset(void *dest, char val, u64 len);
void *memcpy(void *dest, const void *src, u64 len);

#endif
