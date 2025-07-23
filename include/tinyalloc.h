#ifndef _TINYALLOC_H_
#define _TINYALLOC_H_

#include <kernel.h>

void ta_init(void* heap, size_t size, size_t alignment, size_t pagesize);
void* ta_alloc(size_t size);
size_t ta_free(void* ptr);
size_t ta_usable_size(void *ptr);

#endif

