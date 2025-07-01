#ifndef _TINYALLOC_H_
#define _TINYALLOC_H_

#include <kernel.h>

void ta_init(void* heap, size_t size, size_t alignment, size_t pagesize);
void* ta_alloc(size_t size);
void ta_free(void* ptr);

#endif

