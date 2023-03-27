#include <kernel.h>
#include <kmalloc.h>
#include <limine.h>

extern uint64_t k_end;		/* Heap straight after kernel */
//uint64_t heap_pos = (uint64_t)&k_end;
uint64_t heap_pos = 0x00200000;

volatile struct limine_memmap_request memory_map_request = {
    .id = LIMINE_MEMMAP_REQUEST,
    .revision = 0,
};

uint64_t allocated = 0;
uint64_t allocations = 0;

/* Two heaps, one for kernel and one for user */
heap_t*	kheap = NULL;	/* Kernel Heap */
heap_t*	uheap = NULL;	/* User Heap */

uint64_t heapstart = 0;
uint64_t heaplen = 0;

const uint32_t low_mem_start = 0x10000;
const uint32_t low_mem_max = 0x40000;
uint32_t low_mem_cur = low_mem_start;

/* Our list will be ordered and will construct the internal structures */
header_t* ord_list_insert(header_t *insert,header_t *list);	/* Insert into ordered list */
header_t* ord_list_remove(header_t *remove,header_t *list);	/* Remove item from list, return list */
header_t* ord_list_get_nnode(uint64_t n,header_t *list);		/* Get N'th ordered node */
header_t* ord_list_get_last(header_t *list);			/* Get last ordered node */
header_t* ord_list_src_size(uint64_t sz,header_t *list);		/* We are looking for sz <BLOCK size */
header_t* ord_list_src_head(header_t* head,header_t *list);	/* Looking for node header */

/* Memory manager utility routines */
static void* alloc(uint64_t size,uint8_t align,heap_t *heap);/* Allocate memory on the heap */
static void free_int(void*, heap_t *heap);		/* Free memory on the heap */
void expand_heap(uint64_t size, heap_t *heap);	/* Extend the heap by size bytes (paligned)*/
void shrink_heap(uint64_t size, heap_t *heap);	/* Shrink the heap to size bytes (paligned) */
void fix_heap_list(heap_t *heap);		/* Always ensure the block is within the heap */
header_t* palign_block(uint64_t size, heap_t *heap);	/* Page align block */

/********************************************************/

/* Returns non-zero if a given physical address is unusable for page allocation */
char invalid_frame(uint64_t physaddr)
{
	return 0;
	int memcnt = 0;
	while (memcnt < memory_map_request.response->entry_count) {
		uint64_t base = memory_map_request.response->entries[memcnt]->base;
		uint64_t len = memory_map_request.response->entries[memcnt]->length;
		if (physaddr >= base && physaddr < base + len - 1 && 
			memory_map_request.response->entries[memcnt]->type != LIMINE_MEMMAP_USABLE /*&&
			memory_map_request.response->entries[memcnt]->type != LIMINE_MEMMAP_KERNEL_AND_MODULES*/
		) {
			return 1;
		}
		memcnt++;
	}
	return 0;
}

void preboot_fail(char* msg)
{
	setforeground(current_console, COLOUR_LIGHTRED);
	kprintf("%s\n", msg);
	wait_forever();
}

void heap_init()
{
	uint64_t bestlen = 0;
	uint64_t bestaddr = 0;

	heapstart = 0;

	int memcnt = 0;
	while (memcnt < memory_map_request.response->entry_count) {
		if (memory_map_request.response->entries[memcnt]->length > bestlen && memory_map_request.response->entries[memcnt]->type == LIMINE_MEMMAP_USABLE) {
			bestaddr = memory_map_request.response->entries[memcnt]->base;
			bestlen = memory_map_request.response->entries[memcnt]->length;
		}
		memcnt++;
	}
	heapstart = bestaddr;

	// Nothing to speak of. less than 8mb free; give up!
	if (bestlen < 0x800000) {
		preboot_fail("Less than 8mb of ram available, system halted.");
	}

	if (!heapstart) {
		preboot_fail("No usable block of memory found for heap");
	}


	// Default heap to 128mb. If theres less ram than this in the machine, lower it.
	uint64_t min = 0x100000 * 128;
	if (bestlen < min) {
		min = bestlen - 0x1000;
	}

	heaplen = bestlen;

	heap_pos = heapstart;
	heapstart += low_mem_max;

	kheap = create_heap(heapstart, heapstart + heaplen, heapstart + heaplen, min, 0, 1);

	print_heapinfo();
}

void print_heapinfo()
{
	setforeground(current_console, COLOUR_LIGHTYELLOW);
	kprintf("HEAP: ");
	setforeground(current_console, COLOUR_WHITE);
	kprintf("Best fit; start=0x%llx max=0x%llx length=%ldMB\n", heapstart, heaplen, (heaplen - heapstart) / 1048576);
}


heap_t*	create_heap(uint64_t addr, uint64_t end, uint64_t max, uint64_t min, uint8_t user, uint8_t rw)
{
	heap_t* heap = kmalloc(sizeof(heap_t));
	header_t* header;
	footer_t* footer;

	if ((addr % 0x1000) != 0 || (end % 0x1000) != 0) {
		preboot_fail("Non-page-aligned heap");
	}

	if (addr > end) {
		preboot_fail("Start of heap is greater than end of heap");
	}

	if (end > max) {
		preboot_fail("End of heap is beyond maximum heap");
	}

	uint64_t i = addr;
	for (; i < end; i += 0x1000) {
		if (invalid_frame(i))
			preboot_fail("Internal error: (BUG) Initial heap overlays reserved RAM");
	}

	_memset((char*)heap, 0, sizeof(heap_t));	/* Nullify */
 	heap->list_free = (header_t*)addr;	/* In the first (unique) header */
	heap->heap_addr = addr;			/* Define heap base address */
	heap->end_addr = end;			/* Heap end */
	heap->max_size = max;			/* Max size */
	heap->min_size = min;			/* Min size */
	heap->user = user;			/* User or kernel heap */
	heap->rw = rw;				/* R/W */

	/* Define new */
	header = (header_t*)addr;		/* Header at the beginning of the heap */
	header->magic = HEAP_MAGIC;		/* ID byte */
	header->size = (end-addr - sizeof(header_t) - sizeof(footer_t));	/* size of the heap */
	header->free = 1;			/* Available */
	header->prev = 0;			/* Previous entry (prev==0)  */
	header->next = 0;			/* Next entry (next==0) */
	/* Define footer */
	footer = (footer_t*)((uint64_t)header + sizeof(header_t) + header->size);	/* seek */
	footer->magic = HEAP_MAGIC;		/* ID byte */
	footer->header = header;		/* point to header */

	return heap;
}

static void *alloc(uint64_t size,uint8_t palign, heap_t *heap)
{
	void *ret = NULL;	/* return address */
	uint64_t b_total, b_new;	/* Helpful for split */
	header_t *th = NULL;	/* pointer to block header */
	footer_t *tf = NULL;	/* pointer to block footer */

	if (!heap) {
		return NULL;	/* null heap */
	}

	/* We are looking for a free block that fits the size */
	if (!ord_list_src_size(size, heap->list_free)) {
		/* ... if there is no large enough free block */
		expand_heap(size - (ord_list_get_last(heap->list_free))->size, heap);	/* expand heap */
	}
	if (palign) {
		/* If requested page aligned address */
		th = palign_block(size, heap);	/* Pass page aligned block */
	} else {
		/* otherwise... */
		th = ord_list_src_size(size, heap->list_free);	/* Pass the first available block */
	}

	if (!th) {
		/* We could not find space */
		return NULL;
	}		

	heap->list_free = ord_list_remove(th, heap->list_free);	/* Remove the entry */
	tf = (footer_t*)((uint64_t)th + sizeof(header_t) + th->size);

	if (th->magic != HEAP_MAGIC) {
		return NULL;	/* Check if header */
	}
	if (tf->magic != HEAP_MAGIC) {
		return NULL;	/* Check if footer */
	}

	/* There are two possibilities, either it fits into the block, or split */
	b_total = sizeof(header_t) + th->size + sizeof(footer_t);	/* total size of the block */
	b_new	= sizeof(header_t) + size + sizeof(footer_t);		/* total size of the new entry */

	if (b_total - b_new > sizeof(header_t) +  sizeof(footer_t)) {
		/* Fits in the empty block */
		/*  we split, copy the original block */
		th->free = 0;		/* Mark it used */
		th->size = size;	/* New size */
		tf = (footer_t*)((uint64_t)th + sizeof(header_t) + th->size);
		tf->magic = HEAP_MAGIC;	/* New footer */
		tf->header = th;
		/* The address of the header to return */
		ret = ((void*)((uint64_t)th+sizeof(header_t)));

		/* Make aa new free block in the space */
		th = (header_t*)((uint64_t)tf + sizeof(footer_t));	/* New header for the free block */
		th->magic = HEAP_MAGIC;					/* ID byte */
		th->size = b_total - b_new - sizeof(header_t) - sizeof(footer_t);
		th->free = 1;						/* Make free */
		tf = (footer_t*)((uint64_t)th + sizeof(header_t) + th->size);	/* new footer */
		tf->magic = HEAP_MAGIC;	/* not necessary since there was already a footer */
		tf->header = th;	/* point to new header */

		/* Put a new entry for the new free block */
		heap->list_free = ord_list_insert(th,heap->list_free);
	} else {
		/* It can be split */
		th->free = 0;		/* Mark as used */
		ret = ((void*)((uint64_t)th+sizeof(header_t)));
	}
	/* Finally, return the address in the header we created */
	return ret;
}

static void free_int(void *addr, heap_t *heap)
{
	/* pointers to the current, previous and next block */
	header_t *th = NULL, *th_left = NULL, *th_right = NULL;
	/* will use the prev and next to merge the list on deletion */
	footer_t *tf = NULL, *tf_left = NULL, *tf_right = NULL;	

	if (!heap) {
		return;	/* null heap */
	}
	if (addr < (void*)heap->heap_addr || addr > (void*)heap->end_addr) {
		return; /* addr not in heap */
	}

	th = (header_t*)((uint64_t)addr - sizeof(header_t));		/* Pass the header address */
	tf = (footer_t*)((uint64_t)th + sizeof(header_t) + th->size);	/* and the footer */

	if (th->magic != HEAP_MAGIC) {
		return;	/* Check if header */
	}
	if (tf->magic != HEAP_MAGIC) {
		return;	/* Check if footer */
	}
	/* Find associated free blocks */
	/* Check if a free block left */
	if ((uint64_t)th != (uint64_t)heap->heap_addr) {
		/* if there is space left */
		tf_left = (footer_t*)((uint64_t)th - sizeof(footer_t));
		if (tf_left->magic == HEAP_MAGIC && tf_left->header->magic == HEAP_MAGIC) {
			/* Although there is a block... */
			if (tf_left->header->free) {
				/* If its free */
				th_left = tf_left->header;	/* Assign to left */
				heap->list_free = ord_list_remove(th_left,heap->list_free);
				th = th_left;
			}
		}
	}

	/* Check if a free block */
	if ((uint64_t)tf + sizeof(footer_t) != (uint64_t)heap->end_addr) {
		/* ... if there is space to the right */
		th_right = (header_t*)((uint64_t)tf + sizeof(footer_t));
		if (th_right->magic == HEAP_MAGIC) {
			/* a free block... */
			tf_right = (footer_t*)((uint64_t)th_right + sizeof(header_t) + th_right->size);
			if (tf_right->magic == HEAP_MAGIC) {
				/*double check */
				if (th_right->free == 1) {
					/* if free */
					heap->list_free = ord_list_remove(th_right,heap->list_free);
					tf = tf_right;
				}
			}
		}
	}

	/* So now we have a free block where header == th and footer == tf */
	th->free = 1;
	th->magic = HEAP_MAGIC;
	th->size = (uint64_t)tf - ((uint64_t)th + sizeof(header_t));
	tf->magic = HEAP_MAGIC;
	tf->header = th;

	if (heap->end_addr == (uint64_t)tf + sizeof(footer_t)) {	
		/* If it was the Last block */
		shrink_heap(th->size + sizeof(header_t) + sizeof(footer_t), heap);	/*Shrink */
	} else {	
		/* Otherwise we insert the entry */
		heap->list_free = ord_list_insert(th,heap->list_free);
	}

	allocated -= th->size;
}

void expand_heap(uint64_t size, heap_t *heap)
{
	/* Expand to make the size of the heap a new aligned size */
	if (size & 0xFFF) {
		/* make it page aligned if it is not already */
		size &= 0xFFFFF000;
		size += 0x1000;
	}
	assert(heap != NULL, "HEAP EXPAND - NULL HEAP");
	/*expand up to max size */
	if (heap->end_addr + size > heap->max_size || heap->end_addr + size < heap->heap_addr) {
		size = heap->max_size - heap->end_addr;
	}
	/* We now define the pages */
	_memset((char*)heap->end_addr, 0 , size);	/* Nullify */
	heap->end_addr += size;				/* Adjust to end */
	fix_heap_list(heap);				/* Fix list */
}

void shrink_heap(uint64_t size, heap_t* heap)
{
	/* Shrink a heap, but not below min_size */
	uint64_t tmp = (uint64_t)heap->end_addr - size;
	tmp &= 0xFFFFF000;

	assert(heap != NULL, "HEAP SHRINK - NULL HEAP");
	if (tmp < heap->min_size) {	/* Check for shrinking below min_size */
		tmp = heap->min_size;
	}

	heap->end_addr = tmp;					/*Adjust end */

	fix_heap_list(heap);
}

void fix_heap_list(heap_t *heap)
{
	/* Makes sure that if and when there is space in the unattached end of the heap
	 * to contain the block or extend the existing heap. Can be used with any
	 * size of block the even when shrinking below min_size
	 */
	header_t* th;
	footer_t* tf;

	assert(heap != NULL, "HEAP FIX LIST - NULL HEAP");

	/* Adjust Blocks */
	/* Find the footer of the heap */
	uint8_t *p;				/* seek byte */
	p = (uint8_t*)heap->end_addr - sizeof(footer_t);		/* We are looking for the 1-byte Magic Number */

	for (tf = (footer_t*)p; tf >= (footer_t*)heap->heap_addr; tf = (footer_t*)--p) {
		if (tf->magic == HEAP_MAGIC) {
			/* link check */
			if (
				(uint64_t)tf->header >= (uint64_t)heap->heap_addr &&
				(uint64_t)tf->header <= (uint64_t)heap->end_addr &&
				(uint64_t)tf->header->magic == HEAP_MAGIC
			) {
				/* Double check, found the real block end */
				goto bbreak;
			}
		}
	}

bbreak:

	/* In tf we find the last footer, or the start of the heap */
	if (tf == (footer_t*)heap->heap_addr) {
		/* If not found or 1 block */
		/* Make 1 full free block, similar to heap_create*/
		th = (header_t*)(heap->heap_addr);
		th->magic = HEAP_MAGIC;
		th->size = (heap->end_addr-heap->heap_addr - sizeof(header_t) - sizeof(footer_t));
		th->free = 1;
		th->prev = 0;
		th->next = 0;
		/* define footer */
		tf = (footer_t*)((uint64_t)th + sizeof(header_t) + th->size);
		tf->magic = HEAP_MAGIC;
		tf->header = th;

		heap->list_free = th;

		return;
	} else {
		/* If we are the real block footer */
		th = tf->header;
		if (th->free) {
			/* We are the free block, expand */
			heap->list_free = ord_list_remove(th,heap->list_free);	/* remove the old entry */
			tf = (footer_t*)((uint64_t)heap->end_addr - sizeof(footer_t));	/* Footer at the end of the heap */
			tf->magic = HEAP_MAGIC;
			tf->header = th;
			th->size = (uint64_t)tf - (uint64_t)th - sizeof(header_t);	/* fix size */
			heap->list_free = ord_list_insert(th,heap->list_free);	/* put entry into list */
			return;
		} else {
			/* If not, was used */
			th = (header_t*)((uint64_t)tf + sizeof(footer_t));		/* We make a new block immediately after */

			th->magic = HEAP_MAGIC;
			th->free = 1;
			tf = (footer_t*)((uint64_t)heap->end_addr - sizeof(footer_t));	/* Footer at the end of the heap */

			tf->magic = HEAP_MAGIC;					/* Make new footer */
			tf->header = th;
			th->size = (uint64_t)tf - (uint64_t)th - sizeof(header_t);	/* fix size */
			heap->list_free = ord_list_insert(th, heap->list_free);	/* Insert entry */
			return;
		}
	}
}

header_t* palign_block(uint64_t size, heap_t *heap)
{
	header_t* th = ord_list_src_size(size, heap->list_free);	/* We pass the first available block */
	header_t *tmp_h = NULL;
	footer_t *tmp_f = NULL, *tmp_f2 = NULL;

	/* If and when we need page align, We must start searching for a page aligned block, Creating one if neccessary */
	while(th) {
		if (((uint64_t)th + sizeof(header_t)) % 0x1000  == 0) {
			/* Already page aligned */
			return th;
		}

		/* If block isn't page aligned, lets see if we can make one */
		tmp_f = (footer_t*)((uint64_t)th + sizeof(header_t) + th->size);	/* Add footer to block */
		tmp_h = th;
		tmp_h = (header_t*)((uint64_t)tmp_h & 0xFFFFF000);	/* Set tmp_h to a page boundry */
		tmp_h = (header_t*)((uint64_t)tmp_h + 0x1000);		/* the following one, specifically */
		tmp_h = (header_t*)((uint64_t)tmp_h - sizeof(header_t));  /* The DATA address must be page-aligned, not the header */
		while ((uint64_t)tmp_h < ((uint64_t)tmp_f - sizeof(header_t))) {	/* Magic number is before the footer */
			if (
				((uint64_t)tmp_f - (uint64_t)tmp_h - sizeof(header_t) >= size ) && /* Is there enough room after */
				((uint64_t)tmp_h - (uint64_t)th > sizeof(header_t)+sizeof(footer_t))
			) {
				/* Is there enough room before? Then we split the block to 2 free blocks and we alloc the second */
				heap->list_free = ord_list_remove(th,heap->list_free);
				tmp_f2 = (footer_t*)((uint64_t)tmp_h - sizeof(footer_t));
				tmp_f2->magic = HEAP_MAGIC;
				tmp_f2->header = th;
				th->size = (uint64_t)tmp_f2 - (uint64_t)th - sizeof(header_t);
				heap->list_free = ord_list_insert(th,heap->list_free);
		
				th = tmp_h;
				th->magic = HEAP_MAGIC;
				th->size = (uint64_t)tmp_f - (uint64_t)th - sizeof(header_t);
				th->free = 1;
				tmp_f->header = th;
				tmp_f->magic = HEAP_MAGIC;
				heap->list_free = ord_list_insert(th,heap->list_free);
				return th;	/* All ready */
			}
			/* The next magic number... */
			tmp_h = (header_t*)((uint64_t)tmp_h + 0x1000);
		}
		th = th->next;	/* not find the right bound means next block */
	}
	return th;	/* here the header is either 0 or page aligned */
}

void* kmalloc_org(uint64_t size, uint8_t align,uint64_t *phys)
{
	void* ret;

	if (align) {
		/* Align to page boundaries */
		heap_pos &= 0xFFFFF000;
		heap_pos += 0x1000;	/* Next page */
	}
	if (phys)  {
		/* Return physical address of allocated block */
		*phys = heap_pos;
	}
	ret = (void*)heap_pos;
	heap_pos += size;
	return ret;
}

void* kmalloc_ext(uint64_t size, uint8_t align, uint64_t *phys)
{
	allocated += size;
	void* ret;
	if (kheap) {
		return alloc(size,align,kheap);
	} else {
		if (align) {
			/* Alignn to page boundries */
			heap_pos &= 0xFFFFFFFFFFFFF000;
			heap_pos += 0x1000;	/* Next page */
		}
		if (phys) {
			/* Return the physical addres */
			*phys = heap_pos;
		}
		ret = (void*)heap_pos;
		heap_pos += size;
		//wait_forever();
		return ret;
	}
}

void* kmalloc(uint64_t size)
{	/* More standard */
	return kmalloc_ext(size, 0, 0);
}

void kfree(void* addr)
{
	if (kheap) {
		free_int(addr, kheap);
	}
}

/* Insert item into ordered list */
header_t* ord_list_insert(header_t *insert, header_t *list)
{
	header_t *tmp = list, *tmp2;
	if (!tmp) {
		/* Empty list */
		insert->prev = 0;
		insert->next = 0;
		return insert;
	}
	if (tmp->size >= insert->size) {	
		/* Expand list */
		tmp->prev = insert;
		insert->next = tmp;
		insert->prev = 0;
		return insert;
	}
	tmp2 = tmp;
	tmp = tmp->next;
	while (tmp) {
		/* Find correct position in list */
		if (tmp->size >= insert->size) {
			tmp->prev = insert;
			insert->next = tmp;
			tmp2->next = insert;
			insert->prev = tmp2;
			return list;
		}
		tmp2 = tmp;
		tmp = tmp->next;
	}
	/* Add item */
	insert->next = 0;
	tmp2->next = insert;
	insert->prev = tmp2;

	return list;	
}

header_t* ord_list_remove(header_t *remove,header_t *list)
{
	header_t *prev,*next;
	prev = remove->prev;
	next = remove->next;
	if (next) {
		next->prev = prev;
	}
	if (prev) {
		prev->next = next;
	}
	remove->next = 0;
	remove->prev = 0;
	if (remove == list) {
		return next;
	}
	return list;
}

header_t* ord_list_src_size(uint64_t sz,header_t *list)
{
	header_t*tmp = list;
	if (!list) {
		return NULL;
	}
	if (tmp->size >= sz) {
		return tmp;
	}
	tmp = tmp->next;
	while (tmp) {
		if (sz <= tmp->size) {
			return tmp;
		}
		tmp = tmp->next;
	}
	return NULL;
}

header_t* ord_list_get_nnode(uint64_t n,header_t *list)
{
	header_t*tmp = list;
	uint64_t i;
	if (!list) {
		return NULL;
	}
	for (i = 0; i < n && tmp->next; i++, tmp = tmp->next);
	if (i == n) {
		return tmp;
	}
	return NULL;
}

header_t* ord_list_get_last(header_t *list)
{
	header_t*tmp = list;
	if (!list) {
		return NULL;	/* Empty list */
	}
	while (tmp->next) {
		tmp = tmp->next;
	}
	return tmp;
}

void* kcalloc(size_t num, size_t size)
{
	void* ptr = kmalloc(num * size);
	_memset(ptr, 0, num * size);
	return ptr;
}

void* krealloc(void* ptr, size_t new_size)
{
	void* new_ptr = kmalloc(new_size);
	if (new_ptr) {
		/* allocate a new memory block of size new_size bytes,
		 * copy memory area from old to new, and free the old block.
		 * It is faster to not obtain the old size, and safe to do so, as both addresses
		 * fall within our heap and are safe to use if the new allocation succeeded.
		 */
		if (ptr) {
			/* If NULL is passed in as the old ptr, this acts just like malloc */
			memcpy(new_ptr, ptr, new_size);
			kfree(ptr);
		}
		return new_ptr;
	}
	/* If there is not enough memory, the old memory block is not freed and null pointer is returned */
	return NULL;
}

uint32_t kmalloc_low(uint32_t size)
{
	if (size == 0) {
		return 0;
	}

	if (low_mem_cur + size >= low_mem_max) {
		return 0;
	}

	uint32_t allocated = low_mem_cur;
	low_mem_cur += size;

	return allocated;
}


