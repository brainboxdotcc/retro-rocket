/**
 * @file kmalloc.h
 * @author Craig Edwards (craigedwards@brainbox.cc)
 * @copyright Copyright (c) 2012-2025
 */
/**
 * @file kmalloc.h
 * @author Craig Edwards
 * @copyright Copyright (c) 2012-2025
 */
#pragma once

#include <stddef.h>

/* Initialize the heap system */
void init_heap();
void print_heapinfo();

/* Core alloc/free */
void* kmalloc(uint64_t size);
void kfree(const void* addr);

/* calloc/realloc helpers */
void* kcalloc(size_t num, size_t size);
void* krealloc(void* ptr, size_t new_size);

/* Low-memory special allocator: for <4GB identity mapped area */
uint32_t kmalloc_low(uint32_t size);
void kfree_low(uint32_t addr);  /* New: explicitly free low memory if needed */

/* Debug/statistics */
uint64_t get_free_memory();
uint64_t get_used_memory();
uint64_t get_total_memory();
void preboot_fail(const char* msg);

