#pragma once
#ifdef MEMORY_TRACE

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#define MEMORY_TRACE_BUCKETS 32768
#define MEMORY_TRACE_BUCKET_DEPTH 4
#define MEMORY_TRACE_MAX_STACK 16

typedef enum memory_trace_owner_type {
	memory_trace_owner_buddy,
	memory_trace_owner_kmalloc
} memory_trace_owner_type_t;

typedef struct memory_trace_entry {
	void *ptr;
	void *owner;
	memory_trace_owner_type_t owner_type;

	size_t requested_size;
	size_t allocated_size;
	uint64_t ticks;

	size_t trace_count;
	uint64_t trace_addresses[MEMORY_TRACE_MAX_STACK];

	bool used;
} memory_trace_entry_t;

void memory_trace_add(void *ptr, void *owner, memory_trace_owner_type_t owner_type, size_t requested_size, size_t allocated_size, const uint64_t *trace_addresses, size_t trace_count);

void memory_trace_remove(void *ptr);

#define BUDDY_TRACE_ALLOC(ptr, req_size)						\
	do {										\
		uintptr_t _trace_addrs[MEMORY_TRACE_MAX_STACK];				\
		size_t _trace_count = 0;						\
		backtrace_collect(_trace_addrs, MEMORY_TRACE_MAX_STACK, &_trace_count);	\
		memory_trace_add((ptr), alloc, memory_trace_owner_buddy,		\
				 (req_size), order_size(order),				\
				 (uintptr_t*)_trace_addrs, _trace_count);		\
	} while (0)

#define BUDDY_TRACE_FREE(ptr)			\
	do {					\
		memory_trace_remove((ptr));	\
	} while (0)

#define KMALLOC_TRACE_ALLOC(ptr, req_size)						\
	do {										\
		uintptr_t _trace_addrs[MEMORY_TRACE_MAX_STACK];				\
		size_t _trace_count = 0;						\
		backtrace_collect(_trace_addrs, MEMORY_TRACE_MAX_STACK, &_trace_count);	\
		memory_trace_add((ptr), NULL, memory_trace_owner_kmalloc,		\
				 (req_size), (req_size),				\
				 (uintptr_t*)_trace_addrs, _trace_count);		\
	} while (0)

#define KMALLOC_TRACE_FREE(ptr)			\
	do {					\
		memory_trace_remove((ptr));	\
	} while (0)

void memory_trace_dump_leaks(memory_trace_owner_type_t owner_type, void *owner);

#else

#define BUDDY_TRACE_ALLOC(ptr, req_size) ((void)0)
#define BUDDY_TRACE_FREE(ptr) ((void)0)
#define KMALLOC_TRACE_ALLOC(ptr, req_size) ((void)0)
#define KMALLOC_TRACE_FREE(ptr) ((void)0)

#endif

