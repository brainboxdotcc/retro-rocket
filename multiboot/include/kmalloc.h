#ifndef __KMALLOC_H__
#define __KMALLOC_H__

/* Unlike JamesM's heap this is not limited to one page (4Mb).
 * In fact, by default the kernel allocates an 8Mb heap.
 */

#define HEAP_MAGIC 0xDEADDEAD
#define KHEAP_START 0x0C000000
#define UHEAP_START 0x0F000000

struct footer;
struct header;

/* Each bloock of used or free memory has a header and footer */
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

/* Definition for a heap */
typedef struct heap {
	/* Free items list */
	header_t *list_free;
	/* Virtual address of the heap */
	u32int	heap_addr;
	/* Virtual address of current heap end */
	u32int	end_addr;
	/* Heap max size */
	u32int	max_size;
	/* Heap minimum size */
	u32int	min_size;
	/* Is this heap user-accessible? */
	u8int	user;
	/* Can this heap be written to? */
	u8int	rw;
}heap_t;


/* Prototypes */
void*	malloc(u32int size);	/* Memory allocator function */
void*	malloc_ext(u32int size,u8int align, u32int *phys);	/* Extended version */
void	free(void* addr);	/* Free allocated memory function */
void*	kmalloc(u32int size);					/* Kernel heap malloc */
void*	kmalloc_ext(u32int size, u8int align, u32int *phys);	/* Extended version */
void	kfree(void* addr);
heap_t*	create_heap(u32int addr, u32int end, u32int max, u32int min, u8int user, u8int rw); /* Create a heap */

#endif

