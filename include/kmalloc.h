/**
 * @file kmalloc.h
 * @author Craig Edwards (craigedwards@brainbox.cc)
 * @copyright Copyright (c) 2012-2023
 */
#ifndef __KMALLOC_H__
#define __KMALLOC_H__

#include <stddef.h>

/* Unlike JamesM's heap this is not limited to one page (4Mb).
 * In fact, by default the kernel allocates an 8Mb heap.
 */

#define HEAP_MAGIC 0xDEADDEADDEADDEADull

struct footer;
struct header;

/* Each bloock of used or free memory has a header and footer */
typedef struct header {
	uint64_t magic;
	uint64_t size;
	uint8_t  free;
	struct header* prev;
	struct header* next;
}header_t;

typedef struct footer {
	uint64_t magic;
	header_t *header;
}footer_t;

/* Definition for a heap */
typedef struct heap {
	/* Free items list */
	header_t *list_free;
	/* Virtual address of the heap */
	uint64_t heap_addr;
	/* Virtual address of current heap end */
	uint64_t end_addr;
	/* Heap max size */
	uint64_t max_size;
	/* Heap minimum size */
	uint64_t min_size;
	/* Is this heap user-accessible? */
	uint8_t	user;
	/* Can this heap be written to? */
	uint8_t	rw;
} heap_t;


/* Prototypes */
void* kmalloc(uint64_t size);					/* Kernel heap malloc */
void* kmalloc_ext(uint64_t size, uint8_t align, uint64_t *phys);	/* Extended version */
void kfree(void* addr);
heap_t*	create_heap(uint64_t addr, uint64_t end, uint64_t max, uint64_t min, uint8_t user, uint8_t rw); /* Create a heap */
void init_heap();
void print_heapinfo();
void preboot_fail(char* msg);
void* kcalloc(size_t num, size_t size);
void* krealloc(void* ptr, size_t new_size);
uint32_t kmalloc_low(uint32_t size);

#endif

