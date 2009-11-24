#ifndef __KMALLOC_H__
#define __KMALLOC_H__

#include "ordered_array.h"

#define HEAP_MAGIC 0xDEADDEAD
#define KHEAP_START 0x0C000000
#define UHEAP_START 0x0F000000

struct footer;
struct header;

/*Ta structs gia header & footer tou kathe block */
typedef struct header {
	u32int magic;
	u32int size;
	u8int  free;
	struct header* prev;
	struct header* next;
}header_t;

typedef struct footer {
	u32int magic;
	header_t *header;
}footer_t;

/*To struct enos heap */
typedef struct heap {
	header_t *list_free;
	u32int	heap_addr;
	u32int	end_addr;
	u32int	max_size;
	u32int	min_size;
	u8int	user;
	u8int	rw;
}heap_t;


/*Prototypes */
void*	malloc(u32int size);	/*Memory Allocator function */
void*	malloc_ext(u32int size,u8int align, u32int *phys);	/*extended version */
void	free(void* addr);	/*Free allocated memory function */
void*	kmalloc(u32int size);					/*Kernel heap malloc */
void*	kmalloc_ext(u32int size, u8int align, u32int *phys);	/*extended version */
void	kfree(void* addr);
heap_t*	create_heap(u32int addr, u32int end, u32int max, u32int min, u8int user, u8int rw); /*Create a heap */

#endif
