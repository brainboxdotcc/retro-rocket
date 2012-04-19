#include <kernel.h>
#include <kmalloc.h>
#include <hydrogen.h>

extern u64 k_end;		/* Heap straight after kernel */
u64 heap_pos = (u64)&k_end;
//u64 heap_pos = 0x00200000;

u64 allocated = 0;
u64 allocations = 0;

extern page_directory_t *kernel_directory;
extern page_directory_t *current_directory;

spinlock mlock = 0;

/* Two heaps, one for kernel and one for user */
heap_t*	kheap = 0;	/* Kernel Heap */
heap_t*	uheap = 0;	/* User Heap */

u64 heapstart = KHEAP_START;
u64 heaplen = 0;

/* Our list will be ordered and will construct the internal structures */
header_t* ord_list_insert(header_t *insert,header_t *list);	/* Insert into ordered list */
header_t* ord_list_remove(header_t *remove,header_t *list);	/* Remove item from list, return list */
header_t* ord_list_get_nnode(u64 n,header_t *list);		/* Get N'th ordered node */
header_t* ord_list_get_last(header_t *list);			/* Get last ordered node */
header_t* ord_list_src_size(u64 sz,header_t *list);		/* We are looking for sz <BLOCK size */
header_t* ord_list_src_head(header_t* head,header_t *list);	/* Looking for node header */

/*Voithitikes routines tou memory manager */
static void* alloc(u64 size,u8 align,heap_t *heap);/* Allocate memory on the heap */
static void free_int(void*, heap_t *heap);		/* Free memory on the heap */
void expand_heap(u64 size, heap_t *heap);	/* Extend the heap by size bytes (paligned)*/
void shrink_heap(u64 size, heap_t *heap);	/* Shrink the heap to size bytes (paligned) */
void fix_heap_list(heap_t *heap);		/* Always ensure the block is within the heap */
header_t* palign_block(u64 size, heap_t *heap);	/* Page align block */

/********************************************************/

/* Returns non-zero if a given physical address is unusable for page allocation */
int invalid_frame(u64 physaddr)
{
	HydrogenInfoMemory* mi = hydrogen_mmap;
	int memcnt = 0;
	while (memcnt++ < hydrogen_info->mmap_count)
	{
		if (physaddr >= mi->begin && physaddr < mi->begin + mi->length - 1 && mi->available != 1)
			return 1;
		if (mi->length + mi->begin == 0x0) /* Wrapped around 64-bit value */
			break;
		else
			mi =  (HydrogenInfoMemory*)((u64)mi + (u64)sizeof(HydrogenInfoMemory));
	}
	return 0;
}

void preboot_clrscr()
{
	u8* clr = (u8*)0xB8000;
	for (; (u64)clr < 0xB8FFFull; ++clr)
		*clr = 0;
}

void preboot_fail(char* msg)
{
	kprintf("%s\n", msg);
	/*preboot_clrscr();
	u8* screen = (u8*)0xB8000;
	for (; *msg; msg++)
	{
		*screen++ = *msg;
		*screen++ = 0x0F;
	}
	wait_forever();*/
}

void heap_init()
{
	u64 bestlen = 0;
	u64 bestaddr = 0;

	heapstart = hydrogen_info->free_mem_begin;

	HydrogenInfoMemory* mi = hydrogen_mmap;
	int memcnt = 0;
	while (memcnt++ < hydrogen_info->mmap_count)
	{
		if (mi->length > bestlen && mi->available == 1)
		{
			bestaddr = mi->begin;
			bestlen = mi->length;
		}
		if (mi->length + mi->begin == 0x0) /* Wrapped around 64-bit value */
			break;
		else
			mi =  (HydrogenInfoMemory*)((u64)mi + (u64)sizeof(HydrogenInfoMemory));
	}

	// Nothing to speak of. less than 8mb free; give up!
	if (bestlen < 0x800000)
		preboot_fail("Less than 8mb of ram available, system halted.");

	if (bestaddr > heapstart)       // Best block somehow above 4mb default heap pos
		heapstart = bestaddr;

	// Default heap to 128mb. If theres less ram than this in the machine, lower it.
	u64 min = 0x100000 * 128;
	if (bestlen < min)
		min = bestlen - 0x1000;

	heaplen = bestlen;

	//kprintf("%016llx\n", heap_pos);

	kheap = create_heap(heapstart, min, bestlen - 0x1000, min, 0, 1);

	print_heapinfo();
}

void print_heapinfo()
{
	setforeground(current_console, COLOUR_LIGHTYELLOW);
	kprintf("HEAP: ");
	setforeground(current_console, COLOUR_WHITE);
	kprintf("Best fit; start=0x%llx max=0x%llx length=%ld kb\n", heapstart, heaplen, (heaplen - heapstart) / 1024);
}


heap_t*	create_heap(u64 addr, u64 end, u64 max, u64 min, u8 user, u8 rw)
{
	heap_t* heap = kmalloc(sizeof(heap_t));
	header_t* header;
	footer_t* footer;

	if ((addr % 0x1000) != 0 || (end % 0x1000) != 0)
		preboot_fail("Non-page-aligned heap");

	if (addr > end)
		preboot_fail("Start of heap is greater than end of heap");

	if (end > max)
		preboot_fail("End of heap is beyond maximum heap");

	int i = addr;
	for (; i < end; i += 0x1000)
	{
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
	footer = (footer_t*)((u64)header + sizeof(header_t) + header->size);	/* seek */
	footer->magic = HEAP_MAGIC;		/* ID byte */
	footer->header = header;		/* point to header */

	return heap;
}

static void *alloc(u64 size,u8 palign, heap_t *heap)
{
	void *ret;		/* return address */
	u64 b_total, b_new;	/* Helpful for split */
	header_t *th;		/* pointer to block header */
	footer_t *tf;		/* pointer to block footer */

	lock_spinlock(&mlock);

	if (!heap)
	{
		unlock_spinlock(&mlock);
		return 0;	/* null heap */
	}

	/* We are looking for a free block that fits the size */
	if (!ord_list_src_size(size, heap->list_free))	/* ... if there is no large enough free block */
		expand_heap(size - (ord_list_get_last(heap->list_free))->size, heap);	/* expand heap */
	if (palign)	/* If requested page aligned address */
		th = palign_block(size, heap);	/* Pass page aligned block */
	else		/* otherwise... */
		th = ord_list_src_size(size, heap->list_free);	/* Pass the first available block */

	if (!th)
	{
		unlock_spinlock(&mlock);
		return 0;
	}		/* We could not find space */

	heap->list_free = ord_list_remove(th, heap->list_free);	/* Remove the entry */
	tf = (footer_t*)((u64)th + sizeof(header_t) + th->size);
	if (th->magic != HEAP_MAGIC)
	{
		unlock_spinlock(&mlock);
		return 0;	/* Check if header */
	}
	if (tf->magic != HEAP_MAGIC)
	{
		unlock_spinlock(&mlock);
		return 0;	/* Check if footer */
	}

	/* There are two possibilities, either it fits into the block, or split */
	b_total = sizeof(header_t) + th->size + sizeof(footer_t);	/* total size of the block */
	b_new	= sizeof(header_t) + size + sizeof(footer_t);		/* total size of the new entry */
	if (b_total - b_new > sizeof(header_t) +  sizeof(footer_t))
	{
		/* Fits in the empty block */
		/*  we split, copy the original block */
		th->free = 0;		/* Mark it used */
		th->size = size;	/* New size */
		tf = (footer_t*)((u64)th + sizeof(header_t) + th->size);
		tf->magic = HEAP_MAGIC;	/* New footer */
		tf->header = th;
		/* The address of the header to return */
		ret = ((void*)((u64)th+sizeof(header_t)));

		/* Make aa new free block in the space */
		th = (header_t*)((u64)tf + sizeof(footer_t));	/* New header for the free block */
		th->magic = HEAP_MAGIC;					/* ID byte */
		th->size = b_total - b_new - sizeof(header_t) - sizeof(footer_t);
		th->free = 1;						/* Make free */
		tf = (footer_t*)((u64)th + sizeof(header_t) + th->size);	/* new footer */
		tf->magic = HEAP_MAGIC;	/* not necessary since there was already a footer */
		tf->header = th;	/* point to new header */

		/* Put a new entry for the new free block */
		heap->list_free = ord_list_insert(th,heap->list_free);
	}
	else 
	{
		/* It can be split */
		th->free = 0;		/* Mark as used */
		ret = ((void*)((u64)th+sizeof(header_t)));
	}
	/* Finally, return the address in the header we created */
	unlock_spinlock(&mlock);
	return ret;
}

static void free_int(void *addr, heap_t *heap)
{
	header_t *th,*th_left, *th_right;	/* pointers to the current, previous and next block */
	footer_t *tf,*tf_left, *tf_right;	/* will use the prev and next to merge the list on deletion */

	lock_spinlock(&mlock);

	if (!heap)
	{
		unlock_spinlock(&mlock);
		return;	/* null heap */
	}
	if (addr < (void*)heap->heap_addr || addr > (void*)heap->end_addr)
	{
		unlock_spinlock(&mlock);
		return; /* addr not in heap */
	}

	th = (header_t*)((u64)addr - sizeof(header_t));		/* Pass the header address */
	tf = (footer_t*)((u64)th + sizeof(header_t) + th->size);	/* and the footer */

	if (th->magic != HEAP_MAGIC)
	{
		unlock_spinlock(&mlock);
		return;	/* Check if header */
	}
	if (tf->magic != HEAP_MAGIC)
	{
		unlock_spinlock(&mlock);
		return;	/* Check if footer */
	}
	/* Find associated free blocks */
	/* Check if a free block left */
	if ((u64)th != (u64)heap->heap_addr)
	{
		/* if there is space left */
		tf_left = (footer_t*)((u64)th - sizeof(footer_t));
		if (tf_left->magic == HEAP_MAGIC && tf_left->header->magic == HEAP_MAGIC)
		{
			/* Although there is a block... */
			if (tf_left->header->free)
			{
				/* If its free */
				th_left = tf_left->header;	/* Assign to left */
				heap->list_free = ord_list_remove(th_left,heap->list_free);
				th = th_left;
			}
		}
	}

	/* Check if a free block */
	if ((u64)tf + sizeof(footer_t) != (u64)heap->end_addr)
	{
		/* ... if there is space to the right */
		th_right = (header_t*)((u64)tf + sizeof(footer_t));
		if (th_right->magic == HEAP_MAGIC)
		{
			/* a free block... */
			tf_right = (footer_t*)((u64)th_right + sizeof(header_t) + th_right->size);
			if (tf_right->magic == HEAP_MAGIC)
			{
				/*double check */
				if (th_right->free == 1)
				{
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
	th->size = (u64)tf - ((u64)th + sizeof(header_t));
	tf->magic = HEAP_MAGIC;
	tf->header = th;

	if (heap->end_addr == (u64)tf + sizeof(footer_t))
	{	
		/* If it was the Last block */
		shrink_heap(th->size + sizeof(header_t) + sizeof(footer_t), heap);	/*Shrink */
	}
	else
	{	
		/* Otherwise we insert the entry */
		heap->list_free = ord_list_insert(th,heap->list_free);
	}

	allocated -= th->size;

	unlock_spinlock(&mlock);
}

void expand_heap(u64 size, heap_t *heap)
{
	/* Expand to make the size of the heap a new aligned size */
	//kprintf("expand_heap by %d from %08x\n", size, heap->end_addr);
	//blitconsole(current_console);
	if (size & 0xFFF)
	{
		/* make it page aligned if it is not already */
		size &= 0xFFFFF000;
		size += 0x1000;
	}
	assert(heap != NULL, "HEAP EXPAND - NULL HEAP");
	/*expand up to max size */
	if (heap->end_addr + size > heap->max_size || heap->end_addr + size < heap->heap_addr) 
		size = heap->max_size - heap->end_addr;
	/* We now define the pages */
	//sign_sect(heap->end_addr, heap->end_addr+size, heap->user, heap->rw,current_directory);
	_memset((char*)heap->end_addr, 0 , size);	/* Nullify */
	heap->end_addr += size;				/* Adjust to end */
	fix_heap_list(heap);				/* Fix list */
}

void shrink_heap(u64 size, heap_t* heap)
{
	/* Shrink a heap, but not below min_size */
	u64 tmp = (u64)heap->end_addr - size;
	tmp &= 0xFFFFF000;

	assert(heap != NULL, "HEAP SHRINK - NULL HEAP");
	if (tmp < heap->min_size)	/* Check for shrinking below min_size */
		tmp = heap->min_size;

	//release_sect(tmp, heap->end_addr,current_directory);	/*Release pages */
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
	u8 *p;				/* seek byte */
	p = (u8*)heap->end_addr - sizeof(footer_t);		/* We are looking for the 1-byte Magic Number */

	for (tf = (footer_t*)p; tf >= (footer_t*)heap->heap_addr; tf = (footer_t*)--p)
	{
		if (tf->magic == HEAP_MAGIC)
		{
			/* link check */
			if ((u64)tf->header >= (u64)heap->heap_addr &&
					(u64)tf->header <= (u64)heap->end_addr &&
					(u64)tf->header->magic == HEAP_MAGIC)
			{
				/* Double check */
				goto bbreak;			/* Found the real block end */
			}
		}
	}

bbreak:

	/* In tf we find the last footer, or the start of the heap */
	if (tf == (footer_t*)heap->heap_addr)
	{
		/* If not found or 1 block */
		/* Make 1 full free block, similar to heap_create*/
		th = (header_t*)(heap->heap_addr);
		th->magic = HEAP_MAGIC;
		th->size = (heap->end_addr-heap->heap_addr - sizeof(header_t) - sizeof(footer_t));
		th->free = 1;
		th->prev = 0;
		th->next = 0;
		/* define footer */
		tf = (footer_t*)((u64)th + sizeof(header_t) + th->size);
		tf->magic = HEAP_MAGIC;
		tf->header = th;

		heap->list_free = th;

		return;
	}
	else
	{
		/* If we are the real block footer */
		th = tf->header;
		if (th->free)
		{
			/* We are the free block, expand */
			heap->list_free = ord_list_remove(th,heap->list_free);	/* remove the old entry */
			tf = (footer_t*)((u64)heap->end_addr - sizeof(footer_t));	/* Footer at the end of the heap */
			tf->magic = HEAP_MAGIC;
			tf->header = th;
			th->size = (u64)tf - (u64)th - sizeof(header_t);	/* fix size */
			heap->list_free = ord_list_insert(th,heap->list_free);	/* put entry into list */
			return;
		}
		else
		{
			/* If not, was used */
			th = (header_t*)((u64)tf + sizeof(footer_t));		/* We make a new block immediately after */

			th->magic = HEAP_MAGIC;
			th->free = 1;
			tf = (footer_t*)((u64)heap->end_addr - sizeof(footer_t));	/* Footer at the end of the heap */

			tf->magic = HEAP_MAGIC;					/* Make new footer */
			tf->header = th;
			th->size = (u64)tf - (u64)th - sizeof(header_t);	/* fix size */
			heap->list_free = ord_list_insert(th, heap->list_free);	/* Insert entry */
			return;
		}
	}
}

header_t* palign_block(u64 size, heap_t *heap)
{
	header_t* th = ord_list_src_size(size, heap->list_free);	/* We pass the first available block */
	header_t *tmp_h;
	footer_t *tmp_f, *tmp_f2;

	/* If and when we need page align, We must start searching for a page aligned block, Creating one if neccessary */
	while(th)
	{
		if ( ((u64)th + sizeof(header_t)) % 0x1000  == 0 )
			return th;	/* Already page aligned */

		/* If block isn't page aligned, lets see if we can make one */
		tmp_f = (footer_t*)((u64)th + sizeof(header_t) + th->size);	/* Add footer to block */
		tmp_h = th;
		tmp_h = (header_t*)((u64)tmp_h & 0xFFFFF000);	/* Set tmp_h to a page boundry */
		tmp_h = (header_t*)((u64)tmp_h + 0x1000);		/* the following one, specifically */
		tmp_h = (header_t*)((u64)tmp_h - sizeof(header_t));  /* The DATA address must be page-aligned, not the header */
		while ((u64)tmp_h < ((u64)tmp_f - sizeof(header_t)))
		{	/* Magic number is before the footer */
			if (((u64)tmp_f - (u64)tmp_h - sizeof(header_t) >= size ) && /* Is there enough room after */
			    ((u64)tmp_h - (u64)th > sizeof(header_t)+sizeof(footer_t))) /* Is there enough room before */
			{
				/* Then we split the block to 2 free blocks and we alloc the second */
				heap->list_free = ord_list_remove(th,heap->list_free);
				tmp_f2 = (footer_t*)((u64)tmp_h - sizeof(footer_t));
				tmp_f2->magic = HEAP_MAGIC;
				tmp_f2->header = th;
				th->size = (u64)tmp_f2 - (u64)th - sizeof(header_t);
				heap->list_free = ord_list_insert(th,heap->list_free);
		
				th = tmp_h;
				th->magic = HEAP_MAGIC;
				th->size = (u64)tmp_f - (u64)th - sizeof(header_t);
				th->free = 1;
				tmp_f->header = th;
				tmp_f->magic = HEAP_MAGIC;
				heap->list_free = ord_list_insert(th,heap->list_free);
				return th;	/* All ready */
			}
			tmp_h = (header_t*)((u64)tmp_h + 0x1000);	/* The next magic number... */
		}
		th = th->next;	/* not find the right bound means next block */
	}
	return th;	/* here the header is either 0 or page aligned */
}

void* kmalloc_org(u64 size, u8 align,u64 *phys)
{
	void* ret;

	if (align)
	{
		/* Align to page boundaries */
		heap_pos &= 0xFFFFF000;
		heap_pos += 0x1000;	/* Next page */
	}
	if (phys) /* Return physical address of allocated block */
		*phys = heap_pos;
	ret = (void*)heap_pos;
	heap_pos += size;
	return ret;
}

void* kmalloc_ext(u64 size, u8 align, u64 *phys)
{
	allocated += size;
	void* ret;
	if (kheap)
	{
		ret = alloc(size,align,kheap);
		if (phys)
		{	
			/* Allocate physical address */
			page_t *page = get_page((u64)ret, 0, current_directory);
			*phys = (page->frame * 0x1000) + (((u64)ret) & 0xFFF);
		}
		return ret;
	}
	else
	{
		if (align)
		{
			/* Alignn to page boundries */
			heap_pos &= 0xFFFFF000;
			heap_pos += 0x1000;	/* Next page */
		}
		if (phys) /* Return the physical addres */
			*phys = heap_pos;
		ret = (void*)heap_pos;
		heap_pos += size;
		return ret;
	}
}

void* kmalloc(u64 size)
{	/* More standard */
	return kmalloc_ext(size, 0, 0);
}

void kfree(void* addr)
{
	if (kheap)
		free_int(addr,kheap);
}

void* malloc_ext(u64 size, u8 align, u64 *phys)
{
	void* ret;
	if (uheap)
	{
		ret = alloc(size,align,uheap);
		if (phys)
		{	/* Allocate a physical address */
			page_t *page = get_page((u64)ret, 0, current_directory);
			*phys = page->frame*0x1000 + (((u64)ret) & 0xFFF);	/* +offset */
		}
		return ret;
	}
	return 0;
}

void* malloc(u64 size)
{	
	/* More standard */
	return malloc_ext(size, 0, 0);
}

void free(void* addr)
{
	if (uheap)
		free_int(addr,uheap);
}

/* Insert item into ordered list */
header_t* ord_list_insert(header_t *insert,header_t *list)
{
	header_t *tmp = list,*tmp2;
	if (!tmp)
	{
		/* Empty list */
		insert->prev = 0;
		insert->next = 0;
		return insert;
	}
	if (tmp->size >= insert->size)
	{	
		/* Expand list */
		tmp->prev = insert;
		insert->next = tmp;
		insert->prev = 0;
		return insert;
	}
	tmp2 = tmp;
	tmp = tmp->next;
	while (tmp)
	{
		/* Find correct position in list */
		if (tmp->size >= insert->size)
		{
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
	if (next)
		next->prev = prev;
	if (prev)
		prev->next = next;
	remove->next = 0;
	remove->prev = 0;
	if (remove == list)
		return next;
	return list;
}

header_t* ord_list_src_size(u64 sz,header_t *list)
{
	header_t*tmp = list;
	if (!list)
		return 0;
	if (tmp->size >= sz)
		return tmp;
	tmp = tmp->next;
	while (tmp)
	{
		if (sz <= tmp->size)
			return tmp;
		tmp = tmp->next;
	}
	return 0;
}

header_t* ord_list_get_nnode(u64 n,header_t *list)
{
	header_t*tmp = list;
	u64 i;
	if (!list)
		return 0;
	for (i = 0; i < n && tmp->next; i++, tmp=tmp->next);
	if (i == n)
		return tmp;
	return 0;
}

header_t* ord_list_get_last(header_t *list)
{
	header_t*tmp = list;
	if (!list)
		return 0;	/* Empty list */
	while (tmp->next)
		tmp = tmp->next;
	return tmp;
}

