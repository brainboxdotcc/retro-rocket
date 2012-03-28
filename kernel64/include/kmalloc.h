#ifndef __KMALLOC_H__
#define __KMALLOC_H__

/* Unlike JamesM's heap this is not limited to one page (4Mb).
 * In fact, by default the kernel allocates an 8Mb heap.
 */

#define HEAP_MAGIC 0xDEADDEADDEADDEADull
// 16mb
//#define KHEAP_START 0x01000000
// 4mb
#define KHEAP_START 0x00400000
//#define KHEAP_START 0x00200000
#define UHEAP_START 0x0F000000

struct footer;
struct header;

/* Each bloock of used or free memory has a header and footer */
typedef struct header {
	u64 magic;
	u64 size;
	u8  free;
	struct header* prev;
	struct header* next;
}header_t;

typedef struct footer {
	u64 magic;
	header_t *header;
}footer_t;

/* Definition for a heap */
typedef struct heap {
	/* Free items list */
	header_t *list_free;
	/* Virtual address of the heap */
	u64	heap_addr;
	/* Virtual address of current heap end */
	u64	end_addr;
	/* Heap max size */
	u64	max_size;
	/* Heap minimum size */
	u64	min_size;
	/* Is this heap user-accessible? */
	u8	user;
	/* Can this heap be written to? */
	u8	rw;
}heap_t;


/* Prototypes */
void*	malloc(u64 size);	/* Memory allocator function */
void*	malloc_ext(u64 size,u8 align, u64 *phys);	/* Extended version */
void	free(void* addr);	/* Free allocated memory function */
void*	kmalloc(u64 size);					/* Kernel heap malloc */
void*	kmalloc_ext(u64 size, u8 align, u64 *phys);	/* Extended version */
void	kfree(void* addr);
heap_t*	create_heap(u64 addr, u64 end, u64 max, u64 min, u8 user, u8 rw); /* Create a heap */
void heap_init();
void print_heapinfo();

#endif

